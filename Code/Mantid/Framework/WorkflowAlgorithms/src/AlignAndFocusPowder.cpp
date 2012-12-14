/*WIKI* 

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/AlignAndFocusPowder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileFinder.h"

using Mantid::Geometry::Instrument_const_sptr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(AlignAndFocusPowder)

/// Sets documentation strings for this algorithm
void AlignAndFocusPowder::initDocs()
{
  this->setWikiSummary("Algorithm to focus powder diffraction data into a number of histograms according to a grouping scheme defined in a [[CalFile]]. ");
  this->setOptionalMessage("Algorithm to focus powder diffraction data into a number of histograms according to a grouping scheme defined in a CalFile.");
}


using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace;
using API::FileProperty;

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void AlignAndFocusPowder::init()
{
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "The input workspace" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The result of diffraction focussing of InputWorkspace" );
  declareProperty(new FileProperty("CalFileName", "", FileProperty::OptionalLoad, ".cal"),
		  "The name of the CalFile with offset, masking, and grouping data" );
  declareProperty(new WorkspaceProperty<GroupingWorkspace>("GroupingWorkspace","",Direction::Input, PropertyMode::Optional),
    "Optional: An GroupingWorkspace workspace giving the grouping info.");
  declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OffsetsWorkspace","",Direction::Input, PropertyMode::Optional),
    "Optional: An OffsetsWorkspace workspace giving the detector calibration values.");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("MaskWorkspace","",Direction::Input, PropertyMode::Optional),
    "Optional: An Workspace workspace giving which detectors are masked.");
  declareProperty(new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>()),
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be followed by a comma and more widths and last boundary pairs.\n"
        "Negative width values indicate logarithmic binning.");
  declareProperty("Dspacing", true,"Bin in Dspace. (Default true)");
  declareProperty("DMin", 0.0, "Minimum for Dspace axis. (Default 0.) ");
  declareProperty("DMax", 0.0, "Maximum for Dspace axis. (Default 0.) ");
  declareProperty("TMin", 0.0, "Minimum for TOF axis. (Default 0.) ");
  declareProperty("TMax", 0.0, "Maximum for TOF or dspace axis. (Default 0.) ");
  declareProperty("PreserveEvents", true,
    "If the InputWorkspace is an EventWorkspace, this will preserve the full event list (warning: this will use much more memory!).");
  declareProperty("RemovePromptPulseWidth", 0.,
    "Width of events (in microseconds) near the prompt pulse to remove. 0 disables");
  declareProperty("CompressTolerance", 0.01,
    "Compress events (in microseconds) within this tolerance. (Default 0.01) ");
  declareProperty("UnwrapRef", 0., "Reference total flight path for frame unwrapping. Zero skips the correction");
  declareProperty("LowResRef", 0., "Reference DIFC for resolution removal. Zero skips the correction");
  declareProperty("CropWavelengthMin", 0., "Crop the data at this minimum wavelength. Overrides LowResRef.");
  declareProperty("PrimaryFlightPath", -1.0, "If positive, focus positions are changed.  (Default -1) ");
  declareProperty(new ArrayProperty<int32_t>("SpectrumIDs"),
    "Optional: Spectrum IDs (note that it is not detector ID or workspace indices).");
  declareProperty(new ArrayProperty<double>("L2"), "Optional: Secondary flight (L2) paths for each detector");
  declareProperty(new ArrayProperty<double>("Polar"), "Optional: Polar angles (two thetas) for detectors");
  declareProperty(new ArrayProperty<double>("Azimuthal"), "Azimuthal angles (out-of-plain) for detectors");

}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw runtime_error If unable to run one of the sub-algorithms successfully
 */
void AlignAndFocusPowder::exec()
{
  // retrieve the properties
  m_inputW = getProperty("InputWorkspace");
  m_inputEW = boost::dynamic_pointer_cast<EventWorkspace>( m_inputW );
  m_instName = m_inputW->getInstrument()->getName();
  m_instName = Kernel::ConfigService::Instance().getInstrument(m_instName).shortName();
  std::string calFileName = getPropertyValue("CalFileName");
  m_offsetsWS = getProperty("OffsetsWorkspace");
  m_maskWS = getProperty("MaskWorkspace");
  m_groupWS = getProperty("GroupingWorkspace");
  l1 = getProperty("PrimaryFlightPath");
  specids = getProperty("SpectrumIDs");
  l2s = getProperty("L2");
  tths = getProperty("Polar");
  phis = getProperty("Azimuthal");
  params=getProperty("Params");
  dspace = getProperty("DSpacing");
  double dmin = getProperty("DMin");
  double dmax = getProperty("DMax");
  LRef = getProperty("UnwrapRef");
  DIFCref = getProperty("LowResRef");
  minwl = getProperty("CropWavelengthMin");
  tmin = getProperty("TMin");
  tmax = getProperty("TMax");
  m_preserveEvents = getProperty("PreserveEvents");
  // determine some bits about d-space and binning
  if (params.size() == 1)
  {
    if (dmax > 0) dspace = true;
    else dspace=false;
  }
  if (dspace)
  {
    if (params.size() == 1 && dmax > 0)
    {
    	double step = params[0];
        if (step > 0 || dmin > 0)
        {
          params[0] = dmin;
          params.push_back(step);
          params.push_back(dmax);
          g_log.information() << "d-Spacing Binning: " << params[0] << "  " << params[1] << "  " << params[2] <<"\n";
        }
    }
  }
  else
  {
    if (params.size() == 1 && tmax > 0)
    {
    	double step = params[0];
        if (step > 0 || tmin > 0)
        {
          params[0] = tmin;
          params.push_back(step);
          params.push_back(tmax);
          g_log.information() << "TOF Binning: " << params[0] << "  " << params[1] << "  " << params[2] <<"\n";
        }
    }
  }
  xmin = 0;
  xmax = 0;
  if (tmin > 0.)
  {
    xmin = tmin;
  }
  if (tmax > 0.)
  {
    xmax = tmax;
  }
  if (!dspace && params.size() == 3)
  {
    xmin = params[0];
    xmax = params[2];
  }

  loadCalFile(calFileName);


  // Now setup the output workspace
  m_outputW = getProperty("OutputWorkspace");
  if ( m_outputW == m_inputW )
  {
    if (m_inputEW)
    {
      m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
    }
  }
  else
  {
    if (m_inputEW)
    {
      m_outputW = WorkspaceFactory::Instance().create(m_inputW);
      m_outputW->setName(getProperty("OutputWorkspace"));
    }
    else
    {
      //Make a brand new EventWorkspace
      m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", m_inputEW->getNumberHistograms(), 2, 1));
      //Copy geometry over.
      WorkspaceFactory::Instance().initializeFromParent(m_inputEW, m_outputEW, false);
      //You need to copy over the data as well.
      m_outputEW->copyDataFrom( (*m_inputEW) );

      //Cast to the matrixOutputWS and save it
      m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEW);
      m_outputW->setName(getProperty("OutputWorkspace"));
    }
  }

  // filter the input events if appropriate
  if (m_inputEW)
  {
    double removePromptPulseWidth = getProperty("RemovePromptPulseWidth");
    if (removePromptPulseWidth > 0.)
    {
      g_log.information() << "running RemovePromptPulse(Width="
                          << removePromptPulseWidth << ")\n";
      API::IAlgorithm_sptr filterPAlg = createSubAlgorithm("RemovePromptPulse");
      filterPAlg->setProperty("InputWorkspace", m_outputW);
      filterPAlg->setProperty("OutputWorkspace", m_outputW);
      filterPAlg->setProperty("Width", removePromptPulseWidth);
      filterPAlg->executeAsSubAlg();
      m_outputW = filterPAlg->getProperty("OutputWorkspace");
      m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
    }

    double tolerance = getProperty("CompressTolerance");
    if (tolerance > 0.)
    {
      g_log.information() << "running CompressEvents(Tolerance=" << tolerance << ")\n";
      API::IAlgorithm_sptr compressAlg = createSubAlgorithm("CompressEvents");
      compressAlg->setProperty("InputWorkspace", m_outputEW);
      compressAlg->setProperty("OutputWorkspace", m_outputEW);
      compressAlg->setProperty("OutputWorkspace", m_outputEW);
      compressAlg->setProperty("Tolerance",tolerance);
      compressAlg->executeAsSubAlg();
      m_outputEW = compressAlg->getProperty("OutputWorkspace");
      m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEW);
    }
    else
    {
      g_log.information() << "Not compressing event list\n";
      doSortEvents(m_outputW); // still sort to help some thing out
    }
  }

  if (xmin > 0. || xmax > 0.)
  {
    bool doCorrection(true);
    if (m_outputEW) { // extra check for event workspaces
      doCorrection = (m_outputEW->getNumberEvents() > 0);
    }

    if (doCorrection) {
      g_log.information() << "running CropWorkspace(Xmin=" << xmin
                          << ", Xmax=" << xmax << ")\n" ;
	  API::IAlgorithm_sptr cropAlg = createSubAlgorithm("CropWorkspace");
	  cropAlg->setProperty("InputWorkspace", m_outputW);
	  cropAlg->setProperty("OutputWorkspace", m_outputW);
	  if (xmin > 0.)cropAlg->setProperty("Xmin", xmin);
	  if (xmax > 0.)cropAlg->setProperty("Xmax", xmax);
	  cropAlg->executeAsSubAlg();
	  m_outputW = cropAlg->getProperty("OutputWorkspace");
    }
  }

  g_log.information() << "running MaskDetectors\n";
  API::IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors");
  maskAlg->setProperty("Workspace", m_outputW);
  maskAlg->setProperty("MaskedWorkspace", m_maskWS);
  maskAlg->executeAsSubAlg();
  m_outputW = maskAlg->getProperty("Workspace");

  if(!dspace)
  {
      g_log.information() << "running Rebin\n";
	  API::IAlgorithm_sptr rebin1Alg = createSubAlgorithm("Rebin");
	  rebin1Alg->setProperty("InputWorkspace", m_outputW);
	  rebin1Alg->setProperty("OutputWorkspace", m_outputW);
	  rebin1Alg->setProperty("Params",params);
	  rebin1Alg->executeAsSubAlg();
	  m_outputW = rebin1Alg->getProperty("OutputWorkspace");
  }

  g_log.information() << "running AlignDetectors\n";
  API::IAlgorithm_sptr alignAlg = createSubAlgorithm("AlignDetectors");
  alignAlg->setProperty("InputWorkspace", m_outputW);
  alignAlg->setProperty("OutputWorkspace", m_outputW);
  alignAlg->setProperty("OffsetsWorkspace", m_offsetsWS);
  alignAlg->executeAsSubAlg();
  m_outputW = alignAlg->getProperty("OutputWorkspace");

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
      g_log.information() << "running ConvertUnits(Target=TOF)\n";
	  API::IAlgorithm_sptr convert1Alg = createSubAlgorithm("ConvertUnits");
	  convert1Alg->setProperty("InputWorkspace", m_outputW);
	  convert1Alg->setProperty("OutputWorkspace", m_outputW);
	  convert1Alg->setProperty("Target","TOF");
	  convert1Alg->executeAsSubAlg();
	  m_outputW = convert1Alg->getProperty("OutputWorkspace");
  }

  if(LRef > 0.)
  {
      g_log.information() << "running UnwrapSNS(LRef=" << LRef
                          << ",Tmin=" << tmin << ",Tmax=" << tmax <<")\n";
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("UnwrapSNS");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("OutputWorkspace", m_outputW);
	  removeAlg->setProperty("LRef",LRef);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  if(tmax > tmin) removeAlg->setProperty("Tmax",tmax);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }

  if(minwl > 0.)
  {
      g_log.information() << "running RemoveLowResTOF(MinWavelength=" << minwl
                          << ",Tmin=" << tmin << ")\n";
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("OutputWorkspace", m_outputW);
	  removeAlg->setProperty("MinWavelength",minwl);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }
  else if(DIFCref > 0.)
  {
      g_log.information() << "running RemoveLowResTof(RefDIFC=" << DIFCref
                          << ",K=3.22)\n";
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("OutputWorkspace", m_outputW);
	  removeAlg->setProperty("ReferenceDIFC",DIFCref);
	  removeAlg->setProperty("K",3.22);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
      g_log.information() << "running ConvertUnits(Target=dSpacing)\n";
	  API::IAlgorithm_sptr convert2Alg = createSubAlgorithm("ConvertUnits");
	  convert2Alg->setProperty("InputWorkspace", m_outputW);
	  convert2Alg->setProperty("OutputWorkspace", m_outputW);
	  convert2Alg->setProperty("Target","dSpacing");
	  convert2Alg->executeAsSubAlg();
	  m_outputW = convert2Alg->getProperty("OutputWorkspace");
  }

  if(dspace)
  {
      g_log.information() << "running Rebin\n";
	  API::IAlgorithm_sptr rebin2Alg = createSubAlgorithm("Rebin");
	  rebin2Alg->setProperty("InputWorkspace", m_outputW);
	  rebin2Alg->setProperty("OutputWorkspace", m_outputW);
	  rebin2Alg->setProperty("Params",params);
	  rebin2Alg->executeAsSubAlg();
	  m_outputW = rebin2Alg->getProperty("OutputWorkspace");
  }

  doSortEvents(m_outputW);

  g_log.information() << "running DiffractionFocussing\n";
  API::IAlgorithm_sptr focusAlg = createSubAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", m_outputW);
  focusAlg->setProperty("OutputWorkspace", m_outputW);
  focusAlg->setProperty("GroupingWorkspace", m_groupWS);
  focusAlg->setProperty("PreserveEvents", m_preserveEvents);
  focusAlg->executeAsSubAlg();
  m_outputW = focusAlg->getProperty("OutputWorkspace");

  doSortEvents(m_outputW);

  if (l1 > 0)
  {
    g_log.information() << "running EditInstrumentGeometry\n";
    API::IAlgorithm_sptr editAlg = createSubAlgorithm("EditInstrumentGeometry");
    editAlg->setProperty("Workspace", m_outputW);
    editAlg->setProperty("NewInstrument", false);
    editAlg->setProperty("PrimaryFlightPath", l1);
    editAlg->setProperty("Polar", tths);
    editAlg->setProperty("SpectrumIDs", specids);
    editAlg->setProperty("L2", l2s);
    editAlg->setProperty("Azimuthal", phis);
    editAlg->executeAsSubAlg();
    m_outputW = editAlg->getProperty("Workspace");
  }

  g_log.information() << "running ConvertUnits\n";
  API::IAlgorithm_sptr convert3Alg = createSubAlgorithm("ConvertUnits");
  convert3Alg->setProperty("InputWorkspace", m_outputW);
  convert3Alg->setProperty("OutputWorkspace", m_outputW);
  convert3Alg->setProperty("Target","TOF");
  convert3Alg->executeAsSubAlg();
  m_outputW = convert3Alg->getProperty("OutputWorkspace");

  if (params.size() != 1)
  {
  	params.erase(params.begin());
  	params.pop_back();
  }
  g_log.information() << "running Rebin\n";
  API::IAlgorithm_sptr rebin3Alg = createSubAlgorithm("Rebin");
  rebin3Alg->setProperty("InputWorkspace", m_outputW);
  rebin3Alg->setProperty("OutputWorkspace", m_outputW);
  rebin3Alg->setProperty("Params",params);
  rebin3Alg->executeAsSubAlg();
  m_outputW = rebin3Alg->getProperty("OutputWorkspace");

  // return the output workspace
  setProperty("OutputWorkspace",m_outputW);
}

/**
 * Loads the .cal file if necessary.
 */
void AlignAndFocusPowder::loadCalFile(const std::string &calFileName)
{

  // check if the workspaces exist with their canonical names so they are not reloaded for chunks
  if (!m_groupWS)
  {
    try {
      m_groupWS = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(m_instName+"_group");
    } catch (Exception::NotFoundError&) {
      ; // not noteworthy
    }
  }
  if (!m_offsetsWS)
  {
    try {
      m_offsetsWS = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(m_instName+"_offsets");
    } catch (Exception::NotFoundError&) {
      ; // not noteworthy
    }
  }
  if (!m_maskWS)
  {
    try {
      m_maskWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_instName+"_mask");
    } catch (Exception::NotFoundError&) {
      ; // not noteworthy
    }
  }

  // see if everything exists to exit early
  if (m_groupWS && m_offsetsWS && m_maskWS)
    return;

  g_log.information() << "Loading Calibration file \"" << calFileName << "\"\n";

  // bunch of booleans to keep track of things
  bool loadGrouping = !m_groupWS;
  bool loadOffsets  = !m_offsetsWS;
  bool loadMask     = !m_maskWS;

  // Load the .cal file
  IAlgorithm_sptr alg = createSubAlgorithm("LoadCalFile");
  alg->setPropertyValue("CalFilename", calFileName);
  alg->setProperty("InputWorkspace", m_inputW);
  alg->setProperty<std::string>("WorkspaceName", m_instName);
  alg->setProperty("MakeGroupingWorkspace", loadGrouping);
  alg->setProperty("MakeOffsetsWorkspace",  loadOffsets);
  alg->setProperty("MakeMaskWorkspace",     loadMask);
  alg->setLogging(true);
  alg->executeAsSubAlg();

  // replace workspaces as appropriate
  if (loadGrouping)
  {
    m_groupWS = alg->getProperty("OutputGroupingWorkspace");
    AnalysisDataService::Instance().addOrReplace(m_instName+"_group", m_groupWS);
  }
  if (loadOffsets)
  {
    m_offsetsWS = alg->getProperty("OutputOffsetsWorkspace");
    AnalysisDataService::Instance().addOrReplace(m_instName+"_offsets", m_offsetsWS);
  }
  if (loadMask)
  {
    m_maskWS = alg->getProperty("OutputMaskWorkspace");
    AnalysisDataService::Instance().addOrReplace(m_instName+"_mask", m_maskWS);
  }
}

  /** Perform SortEvents on the output workspaces
   * but only if they are EventWorkspaces. 
   *
   * @param ws :: any Workspace. Does nothing if not EventWorkspace.
   */
  void AlignAndFocusPowder::doSortEvents(Mantid::API::Workspace_sptr ws)
  {
    EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(ws);
    if (!eventWS)
      return;
    Algorithm_sptr alg = this->createSubAlgorithm("SortEvents");
    alg->setProperty("InputWorkspace", eventWS);
    alg->setPropertyValue("SortBy", "X Value");
    alg->executeAsSubAlg();
  }


} // namespace WorkflowAlgorithm
} // namespace Mantid
