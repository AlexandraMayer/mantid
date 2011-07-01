#include "vtkEventNexusReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkFloatArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPlane.h"

#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include "MantidVatesAPI/vtkThresholdingHexahedronFactory.h"
#include "MantidVatesAPI/vtkThresholdingQuadFactory.h"
#include "MantidVatesAPI/vtkThresholdingLineFactory.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"

#include "MantidDataHandling/LoadEventNexus.h"
#include <boost/format.hpp>
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidMDEvents/OneStepMDEW.h"
#include "MantidAPI/IMDEventWorkspace.h"

#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

vtkCxxRevisionMacro(vtkEventNexusReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkEventNexusReader);

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::MDHistoDimension;
using Mantid::Geometry::MDHistoDimension_sptr;

vtkEventNexusReader::vtkEventNexusReader() : 
  m_presenter(), 
  m_isSetup(false), 
  m_clipFunction(NULL),
  m_mdEventWsId("eventWsId"),
  m_histogrammedWsId("histogramWsId")
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  //On first-pass rebinning is necessary.
  m_actionManager.ask(RecalculateAll);
}

vtkEventNexusReader::~vtkEventNexusReader()
{
  this->SetFileName(0);
}

/**
  Sets number of bins for x dimension.
  @param nbins : Number of bins for the x dimension.
*/
void vtkEventNexusReader::SetXBins(int nbins)
{
  if(nbins != m_nXBins)
  {
    m_nXBins = nbins;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

/**
  Sets number of bins for x dimension.
  @param nbins : Number of bins for the x dimension.
*/
void vtkEventNexusReader::SetYBins(int nbins)
{
  if(nbins != m_nYBins)
  {
    m_nYBins = nbins;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

/**
  Sets number of bins for y dimension.
  @param nbins : Number of bins for the x dimension.
*/
void vtkEventNexusReader::SetZBins(int nbins)
{
  if(nbins != m_nZBins)
  {
    m_nZBins = nbins;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

/**
  Sets maximum threshold for rendering.
  @param maxThreshold : Maximum threshold value.
*/
void vtkEventNexusReader::SetMaxThreshold(double maxThreshold)
{
  if(maxThreshold != m_maxThreshold)
  {
    m_maxThreshold = maxThreshold;
    this->Modified();
    m_actionManager.ask(RecalculateVisualDataSetOnly);
  }
}

/**
  Sets minimum threshold for rendering.
  @param minThreshold : Minimum threshold value.
*/
void vtkEventNexusReader::SetMinThreshold(double minThreshold)
{
  if(minThreshold != m_minThreshold)
  {
    m_minThreshold = minThreshold;
    this->Modified();
    m_actionManager.ask(RecalculateVisualDataSetOnly);
  }
}

/**
  Sets clipping.
  @param applyClip : true if clipping should be applied.
*/
void vtkEventNexusReader::SetApplyClip(bool applyClip)
{
  if(m_applyClip != applyClip)
  {
    m_applyClip = applyClip;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

/**
  Sets width for plane.
  @param width: width for plane.
*/
void vtkEventNexusReader::SetWidth(double width)
{
  if(m_width.getValue() != width)
  {
    m_width = width;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}

/**
  Sets maximum threshold for rendering.
  @param maxThreshold : Maximum threshold value.
*/
void vtkEventNexusReader::SetClipFunction(vtkImplicitFunction* func)
{
  if(m_clipFunction != func)
  {
    m_clipFunction = func;
    this->Modified();
    m_actionManager.ask(RecalculateAll);
  }
}
  
/**
  Sets applied geometry xml. Provided by object panel.
  @xml. Dimension xml.
*/
void vtkEventNexusReader::SetAppliedGeometryXML(std::string appliedGeometryXML)
{
  if(m_isSetup)
  {
    //Create xml to represent the current applied geometry.
    using namespace Mantid::Geometry;
    MDGeometryBuilderXML<StrictDimensionPolicy> xmlBuilder;
    xmlBuilder.addXDimension(m_appliedXDimension);
    xmlBuilder.addYDimension(m_appliedYDimension);
    xmlBuilder.addZDimension(m_appliedZDimension);
    xmlBuilder.addTDimension(m_appliedTDimension);
    const std::string existingGeometryXML = xmlBuilder.create();
    //If new xml has been provided and if that is different in any way from the existing.
    if(!appliedGeometryXML.empty() && existingGeometryXML != appliedGeometryXML)
    {
      Mantid::Geometry::MDGeometryXMLParser xmlParser(appliedGeometryXML);
      xmlParser.execute();

      this->m_appliedXDimension = xmlParser.getXDimension();
      this->m_appliedYDimension = xmlParser.getYDimension();
      this->m_appliedZDimension = xmlParser.getZDimension();
      this->m_appliedTDimension = xmlParser.getTDimension();
	  //TODO: This needs to be reworked. 
	  MDGeometryBuilderXML<StrictDimensionPolicy> temp;
	  temp.addXDimension(m_appliedXDimension);
	  temp.addYDimension(m_appliedYDimension);
	  temp.addZDimension(m_appliedZDimension);
	  temp.addTDimension(m_appliedTDimension);
	  m_geometryXmlBuilder.overWriteWith(temp);

      m_actionManager.ask(RecalculateAll);
      this->Modified();
    }
  }
}

/**
  Gets the geometry xml from the workspace. Allows object panels to configure themeselves.
  @return geometry xml const * char reference.
*/
const char* vtkEventNexusReader::GetInputGeometryXML()
{
  return this->m_geometryXmlBuilder.create().c_str();
}

/**
   Mantid properties for rebinning algorithm require formatted information.
   @param dimension : dimension to extract property value for.
   @return true available, false otherwise.
 */
std::string vtkEventNexusReader::extractFormattedPropertyFromDimension(Mantid::Geometry::IMDDimension_sptr dimension) const
{
  double min = dimension->getMinimum();
  double max = dimension->getMaximum();
  size_t nbins = dimension->getNBins();
  std::string id = dimension->getDimensionId();
  return boost::str(boost::format("%s, %f, %f, %d") %id %min %max % nbins);
}

/**
   Actually perform the rebinning. Configure the rebinning algorithm and pass to presenter.
 */
void vtkEventNexusReader::doRebinning()
{
  Mantid::API::AnalysisDataService::Instance().remove(m_histogrammedWsId);

  Mantid::MDEvents::BinToMDHistoWorkspace hist_alg;
  hist_alg.initialize();
  hist_alg.setPropertyValue("InputWorkspace", m_mdEventWsId);
  std::string id; 
  if(this->hasXDimension())
  {
    hist_alg.setPropertyValue("DimX",  extractFormattedPropertyFromDimension(m_appliedXDimension)); 
  }
  if(this->hasYDimension())
  {
    hist_alg.setPropertyValue("DimY",  extractFormattedPropertyFromDimension(m_appliedYDimension)); 
  }
  if(this->hasZDimension())
  {
    hist_alg.setPropertyValue("DimZ",  extractFormattedPropertyFromDimension(m_appliedZDimension)); 
  }
  if(this->hasTDimension())
  {
    hist_alg.setPropertyValue("DimT",  extractFormattedPropertyFromDimension(m_appliedTDimension)); 
  }
  hist_alg.setPropertyValue("OutputWorkspace", m_histogrammedWsId);

  if(true == m_applyClip)
  {
    vtkPlane* plane = dynamic_cast<vtkPlane*>(this->m_clipFunction);
    if(NULL != plane)
    {
      //user has requested the use of implicit functions as part of rebinning. only planes understood for time being.
      using namespace Mantid::MDAlgorithms;
      double* pNormal = plane->GetNormal();
      double* pOrigin = plane->GetOrigin();
      NormalParameter normal(pNormal[0], pNormal[1], pNormal[2]);
      OriginParameter origin(pOrigin[0], pOrigin[1], pOrigin[2]);
      PlaneImplicitFunction func(normal, origin, m_width);
      hist_alg.setPropertyValue("ImplicitFunctionXML", func.toXMLString());
    }
  }

  FilterUpdateProgressAction<vtkEventNexusReader> updatehandler(this);
  // Run the algorithm and cache the output.
  m_presenter.execute(hist_alg, m_histogrammedWsId, updatehandler);
}


int vtkEventNexusReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
	try
	{
  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);


  //  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
  //    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet *output = vtkDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));


  int time = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
  {
    // usually only one actual step requested
    time = static_cast<int>(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0]);
  } 

  //When RecalculateAll wins-out, configure and run the rebinning algorithm.
  if(RecalculateAll == m_actionManager.action())
  {
    doRebinning();
  }

  // Chain of resposibility setup for visualisation. Encapsulates decision making on how workspace will be rendered.
  std::string scalarName = "signal";
  vtkThresholdingLineFactory vtkGridFactory(scalarName, m_minThreshold, m_maxThreshold);
  vtkThresholdingQuadFactory* p_2dSuccessorFactory = new vtkThresholdingQuadFactory(scalarName, m_minThreshold, m_maxThreshold);
  vtkThresholdingHexahedronFactory* p_3dSuccessorFactory = new vtkThresholdingHexahedronFactory(scalarName, m_minThreshold, m_maxThreshold);
  vtkGridFactory.SetSuccessor(p_2dSuccessorFactory);
  p_2dSuccessorFactory->SetSuccessor(p_3dSuccessorFactory);
  
  RebinningKnowledgeSerializer serializer(LocationNotRequired); //Object handles serialization of meta data.

//  vtkUnstructuredGrid* structuredMesh = vtkUnstructuredGrid::SafeDownCast(m_presenter.getMesh(serializer, vtkGridFactory));
  vtkDataSet * structuredMesh = vtkDataSet::SafeDownCast(m_presenter.getMesh(serializer, vtkGridFactory));
  output->ShallowCopy(structuredMesh);

  // Reset the action manager fresh for next cycle.
  m_actionManager.reset();
  return 1;
	}
	catch(std::exception& ex)
	{
		std::string message = ex.what();
	}
}

int vtkEventNexusReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  using namespace Mantid::API;
  using namespace Mantid::MDEvents;
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
 
  //Ensure that the Event Workspace is only generated once
  if(!m_isSetup) 
  {
    AnalysisDataService::Instance().remove(m_mdEventWsId);

    Mantid::MDEvents::OneStepMDEW alg;
    alg.initialize();
    alg.setPropertyValue("Filename", this->FileName);
    alg.setPropertyValue("OutputWorkspace", m_mdEventWsId);
    alg.execute();

    Workspace_sptr result=AnalysisDataService::Instance().retrieve(m_mdEventWsId);
    Mantid::API::IMDEventWorkspace_sptr eventWs = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(result);

    // Now, we get the minimum extents in order to get nice default sizes
    std::vector<Mantid::Geometry::MDDimensionExtents> ext = eventWs->getMinimumExtents(5);
    std::vector<IMDDimension_sptr> defaultDimensions;
    size_t nDimensions = eventWs->getNumDims();
    for (size_t d=0; d<nDimensions; d++)
    {
      IMDDimension_sptr inDim = eventWs->getDimension(d);
      double min = (ext[d].min);
      double max = (ext[d].max);
      if (min > max)
      {
        min = 0.0;
        max = 1.0;
      }
      //std::cout << "dim " << d << min << " to " <<  max << std::endl;
      MDHistoDimension_sptr dim(new MDHistoDimension(inDim->getName(), inDim->getName(), inDim->getUnits(), min, max, size_t(10)));
      defaultDimensions.push_back(dim);
    }
    
    //Configuring the geometry xml builder allows the object panel associated with this reader to later
    //determine how to display all geometry related properties.
    if(nDimensions > 0)
    {
      m_appliedXDimension = defaultDimensions[0];
      m_geometryXmlBuilder.addXDimension( m_appliedXDimension );
    }
    if(nDimensions > 1)
    {
      m_appliedYDimension = defaultDimensions[1];
      m_geometryXmlBuilder.addYDimension( m_appliedYDimension );
    }
    if(nDimensions > 2)
    {
      m_appliedZDimension = defaultDimensions[2];
      m_geometryXmlBuilder.addZDimension( m_appliedZDimension );
    }
    if(nDimensions > 3)
    {
      m_appliedTDimension = defaultDimensions[3];
      m_geometryXmlBuilder.addTDimension( m_appliedTDimension );
    }

    m_isSetup = true;
  }

  std::vector<double> timeStepValues(1); //TODO set time-step information.
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeStepValues[0], static_cast<int>(timeStepValues.size()));
  double timeRange[2];
  timeRange[0] = timeStepValues.front();
  timeRange[1] = timeStepValues.back();

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  return 1;
}

void vtkEventNexusReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkEventNexusReader::CanReadFile(const char* vtkNotUsed(fname))
{
  return 1; //TODO: Apply checks here.
}

unsigned long vtkEventNexusReader::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if (this->m_clipFunction != NULL)
  {

    time = this->m_clipFunction->GetMTime();
    if(time > mTime)
    {
      mTime = time;
    }
  }

  return mTime;
}

/**
  Update/Set the progress.
  @parameter progress : progress increment.
*/
void vtkEventNexusReader::UpdateAlgorithmProgress(double progress)
{
  this->SetProgressText("Executing Mantid MDEvent Rebinning Algorithm...");
  this->UpdateProgress(progress);
}
