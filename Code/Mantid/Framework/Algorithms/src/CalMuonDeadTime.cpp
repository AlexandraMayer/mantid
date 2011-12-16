//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>

#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/CalMuonDeadTime.h"
#include "MantidAPI/TableRow.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using API::Progress;

// Register the class into the algorithm factory
DECLARE_ALGORITHM( CalMuonDeadTime)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CalMuonDeadTime::init()
{
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "",
      Direction::Input), "Name of the input workspace");

  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>("DeadTimeTable","",Direction::Output),
    "The name of the TableWorkspace in which to store the list of deadtimes for each spectrum" ); 

  declareProperty("FirstGoodData", 0.5, 
    "The first good data point in units of micro-seconds as measured from time zero (default to 0.5)", 
     Direction::Input); 

  declareProperty("LastGoodData", 5.0, 
    "The last good data point in units of micro-seconds as measured from time zero (default to 5.0)", 
     Direction::Input); 

  declareProperty(new API::WorkspaceProperty<API::Workspace>("DataFitted","",Direction::Output),
    "The data which the deadtime equation is fitted to" ); 

}

/** Executes the algorithm
 *
 */
void CalMuonDeadTime::exec()
{
  // Muon decay constant

  const double muonDecay = 2.2; // in units of micro-seconds
  

  // get input properties

  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const double firstgooddata = getProperty("FirstGoodData");
  const double lastgooddata = getProperty("LastGoodData");


  // Get number of good frames from Run object. This also serves as
  // a test to see if valid input workspace has been provided

  double numGoodFrames = 1.0;
  const API::Run & run = inputWS->run();
  if ( run.hasProperty("goodfrm") )
  {
    numGoodFrames = boost::lexical_cast<double>(run.getProperty("goodfrm")->value());
  }
  else
  {
    g_log.error() << "To calculate Muon deadtime requires that goodfrm (number of good frames) "
                  << "is stored in InputWorkspace Run object\n"; 
  }


  // Do the initial setup of the ouput table-workspace

  API::ITableWorkspace_sptr outTable = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  outTable->addColumn("int","spectrum");
  outTable->addColumn("double","dead-time");


  // Start created a temperary workspace with data we are going to fit
  // against. First step is to crop to only include data from firstgooddata

  std::string wsName = "TempForMuonCalDeadTime";
  API::IAlgorithm_sptr cropWS;
  cropWS = createSubAlgorithm("CropWorkspace", -1, -1);  
  cropWS->setProperty("InputWorkspace", inputWS);
  cropWS->setPropertyValue("OutputWorkspace", "croppedWS"); 
  cropWS->setProperty("XMin", firstgooddata); 
  cropWS->setProperty("XMax", lastgooddata); 
  cropWS->executeAsSubAlg();

  boost::shared_ptr<API::MatrixWorkspace> wsCrop =
        cropWS->getProperty("OutputWorkspace");


  // next step is to take these data. Create a point workspace
  // which will change the x-axis values to mid-point time values
  // and populate
  // x-axis with measured counts
  // y-axis with measured counts * exp(t/t_mu)

  API::IAlgorithm_sptr convertToPW;
  convertToPW = createSubAlgorithm("ConvertToPointData", -1, -1);  
  convertToPW->setProperty("InputWorkspace", wsCrop);
  convertToPW->setPropertyValue("OutputWorkspace", wsName); 
  convertToPW->executeAsSubAlg();

  boost::shared_ptr<API::MatrixWorkspace> wsFitAgainst =
        convertToPW->getProperty("OutputWorkspace");

  const size_t numSpec = wsFitAgainst->getNumberHistograms();
  size_t timechannels = wsFitAgainst->readY(0).size();
  for (size_t i = 0; i < numSpec; i++)
  {
    for (size_t t = 0; t < timechannels; t++)
    {
      const double time = wsFitAgainst->dataX(i)[t];  // mid-point time value because point WS
      const double decayFac = exp(time/muonDecay);
      wsFitAgainst->dataY(i)[t] = wsCrop->dataY(i)[t]*decayFac; 
      wsFitAgainst->dataX(i)[t] = wsCrop->dataY(i)[t]; 
      wsFitAgainst->dataE(i)[t] = wsCrop->dataE(i)[t]*decayFac; 
    }

  }  


  // This property is returned for instrument scientists to 
  // play with on the odd occasion

  setProperty("DataFitted", wsFitAgainst);


  // cal deadtime for each spectrum

  for (size_t i = 0; i < numSpec; i++)
  {
    // Do linear fit 

    const double in_bg0 = inputWS->dataY(i)[0];
    const double in_bg1 = 0.0;

    API::IAlgorithm_sptr fit;
    fit = createSubAlgorithm("Fit", -1, -1, true);

    const int wsindex = static_cast<int>(i);
    fit->setProperty("InputWorkspace", wsFitAgainst);
    fit->setProperty("WorkspaceIndex", wsindex);

    std::stringstream ss;
    ss << "name=LinearBackground,A0=" << in_bg0 << ",A1=" << in_bg1;
    std::string function = ss.str();

    fit->setProperty("Function", function);
    fit->executeAsSubAlg();

    std::string fitStatus = fit->getProperty("OutputStatus");
    std::vector<double> params = fit->getProperty("Parameters");
    std::vector<std::string> paramnames = fit->getProperty("ParameterNames");

    // Check order of names
    if (paramnames[0].compare("A0") != 0)
    {
      g_log.error() << "Parameter 0 should be A0, but is " << paramnames[0] << std::endl;
      throw std::invalid_argument("Parameters are out of order @ 0, should be A0");
    }
    if (paramnames[1].compare("A1") != 0)
    {
      g_log.error() << "Parameter 1 should be A1, but is " << paramnames[1]
            << std::endl;
      throw std::invalid_argument("Parameters are out of order @ 0, should be A1");
    }

    // time bin - assumed constant for histogram
    const double time_bin = inputWS->dataX(i)[1]-inputWS->dataX(i)[0];

    if (!fitStatus.compare("success"))
    {
      const double A0 = params[0];
      const double A1 = params[1];

      // add row to output table
      API::TableRow t = outTable->appendRow();
      t << wsindex+1 << -(A1/A0)*time_bin*numGoodFrames;
    } 
    else
    {
      g_log.warning() << "Fit falled. Status = " << fitStatus << std::endl
                        << "For workspace index " << i << std::endl;
    } 

  } 


  // finally calculate alpha

  setProperty("DeadTimeTable", outTable);
}

} // namespace Algorithm
} // namespace Mantid


