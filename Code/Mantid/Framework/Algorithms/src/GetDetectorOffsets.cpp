#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>


namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(GetDetectorOffsets)
    
    /// Sets documentation strings for this algorithm
    void GetDetectorOffsets::initDocs()
    {
      this->setWikiSummary("Creates an [[OffsetsWorkspace]] containing offsets for each detector. You can then save these to a .cal file using SaveCalFile.");
      this->setOptionalMessage("Creates an OffsetsWorkspace containing offsets for each detector. You can then save these to a .cal file using SaveCalFile.");
    }
    

    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;

    /// Constructor
    GetDetectorOffsets::GetDetectorOffsets() :
      API::Algorithm()
    {}

    /// Destructor
    GetDetectorOffsets::~GetDetectorOffsets()
    {}


    //-----------------------------------------------------------------------------------------
    /** Initialisation method. Declares properties to be used in algorithm.
     */
    void GetDetectorOffsets::init()
    {

      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,
          new WorkspaceUnitValidator<>("dSpacing")),"A 2D workspace with X values of d-spacing");

      BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
      mustBePositive->setLower(0);

      declareProperty("Step",0.001, mustBePositive,
        "Step size used to bin d-spacing data");
      declareProperty("DReference",2.0, mustBePositive->clone(),
         "Center of reference peak in d-space");
      declareProperty("XMin",0.0, "Minimum of CrossCorrelation data to search for peak, usually negative");
      declareProperty("XMax",0.0, "Maximum of CrossCorrelation data to search for peak, usually positive");

      declareProperty(new FileProperty("GroupingFileName","", FileProperty::OptionalSave, ".cal"),
          "Optional: The name of the output CalFile to save the generated OffsetsWorkspace." );

      declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OutputWorkspace","",Direction::Output),
          "An output workspace containing the offsets.");
    }

    //-----------------------------------------------------------------------------------------
    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void GetDetectorOffsets::exec()
    {
      inputW=getProperty("InputWorkspace");
      Xmin=getProperty("XMin");
      Xmax=getProperty("XMax");
      if (Xmin>=Xmax)
        throw std::runtime_error("Must specify Xmin<Xmax");
      dreference=getProperty("DReference");
      step=getProperty("Step");
      nspec=inputW->getNumberHistograms();
      // Create the output OffsetsWorkspace
      OffsetsWorkspace_sptr outputW(new OffsetsWorkspace(inputW->getInstrument()));

      // Fit all the spectra with a gaussian
      Progress prog(this, 0, 1.0, nspec);
      PARALLEL_FOR1(inputW)
      for (int i=0;i<nspec;++i)
      {
        PARALLEL_START_INTERUPT_REGION
        double offset=fitSpectra(i);
        int detID = inputW->getDetector(i)->getID();

        PARALLEL_CRITICAL(GetDetectorOffsets_setValue)
        { // Most of the exec time is in FitSpectra, so this critical block should not be a problem.
          outputW->setValue(detID, offset);
        }
        prog.report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Return the output
      setProperty("OutputWorkspace",outputW);

      // Also save to .cal file, if requested
      std::string filename=getProperty("GroupingFileName");
      if (!filename.empty())
      {
        progress(0.9, "Saving .cal file");
        IAlgorithm_sptr childAlg = createSubAlgorithm("SaveCalFile");
        childAlg->setProperty("OffsetsWorkspace", outputW);
        childAlg->setPropertyValue("Filename", filename);
        childAlg->executeAsSubAlg();
      }

    }


    //-----------------------------------------------------------------------------------------
   /** Calls Gaussian1D as a child algorithm to fit the offset peak in a spectrum
    *
    *  @param s :: The Workspace Index to fit
    *  @return The calculated offset value
    */
    double GetDetectorOffsets::fitSpectra(const int s)
    {
      // Find point of peak centre
      const MantidVec & yValues = inputW->readY(s);
      MantidVec::const_iterator it = std::max_element(yValues.begin(), yValues.end());
      const double peakHeight = *it; 
      const double peakLoc = inputW->readX(s)[it - yValues.begin()];
      // Return offset of 0 if peak of Cross Correlation is nan (Happens when spectra is zero)
      if ( peakHeight < 0.75 || boost::math::isnan(peakHeight) ) return (0.);

      IAlgorithm_sptr fit_alg;
      try
      {
        //set the subalgorithm no to log as this will be run once per spectra
        fit_alg = createSubAlgorithm("Fit",-1,-1,false);
      } catch (Exception::NotFoundError&)
      {
        g_log.error("Can't locate Fit algorithm");
        throw ;
      }
      fit_alg->setProperty("InputWorkspace",inputW);
      fit_alg->setProperty("WorkspaceIndex",s);
      fit_alg->setProperty("StartX",Xmin);
      fit_alg->setProperty("EndX",Xmax);
      fit_alg->setProperty("MaxIterations",100);

      std::ostringstream fun_str;
      fun_str << "name=LinearBackground;name=Gaussian,Height="<<peakHeight<<",";
      fun_str << "PeakCentre="<<peakLoc<<",Sigma=10.0";

      fit_alg->setProperty("Function",fun_str.str());
      fit_alg->executeAsSubAlg();
      std::string fitStatus = fit_alg->getProperty("OutputStatus");
      if ( fitStatus.compare("success") ) return (0.);

      std::vector<double> params = fit_alg->getProperty("Parameters");
      const double offset = params[3]; // f1.PeakCentre
      return (-offset*step/(dreference+offset*step));
    }



  } // namespace Algorithm
} // namespace Mantid
