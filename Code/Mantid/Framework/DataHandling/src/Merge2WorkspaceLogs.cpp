#include "MantidDataHandling/Merge2WorkspaceLogs.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(Merge2WorkspaceLogs)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Merge2WorkspaceLogs::Merge2WorkspaceLogs()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Merge2WorkspaceLogs::~Merge2WorkspaceLogs()
  {
  }
  
  void Merge2WorkspaceLogs::initDocs(){

    this->setWikiSummary("Merge 2 TimeSeries logs in a given Workspace. ");
    this->setOptionalMessage("Merge 2 TimeSeries logs in a given Workspace.");

    return;
  }

  void Merge2WorkspaceLogs::init(){

    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("Workspace", "Anonymous", Direction::InOut),
        "Workspace to have logs merged");
    declareProperty("LogName1", "", "One of the log to be merged");
    declareProperty("LogName2", "", "One of the log to be merged");
    declareProperty("MergedLogName", "", "Name of the merged log.");

    return;
  }

  void Merge2WorkspaceLogs::exec(){

    // 1. Get value
    matrixWS = this->getProperty("Workspace");
    std::string logname1 = this->getProperty("LogName1");
    std::string logname2 = this->getProperty("LogName2");
    std::string mlogname = this->getProperty("MergedLogName");

    // 2. Check
    if (logname1.size() == 0 || logname2.size() == 0 || mlogname.size() == 0){
      g_log.error() << "One or more than one log name is not given!" << std::endl;
      throw std::invalid_argument("One or more than one log name is not give");
    }

    // 3. Merge log
    this->mergeLogs(logname1, logname2, mlogname);

    return;
  }


  /*
   * Merge 2 TimeSeries log together for the third one
   * @param ilogname1:  name of log 1 to be merged
   * @param ilogname2:  name of log 2 to be merged
   * @param ologname:   name of the merged log to be added to workspace
   */
  void Merge2WorkspaceLogs::mergeLogs(std::string ilogname1, std::string ilogname2, std::string ologname){

    // 1. Get log
    Kernel::TimeSeriesProperty<double> *p1 = getTimeSeriesLog(ilogname1);
    Kernel::TimeSeriesProperty<double> *p2 = getTimeSeriesLog(ilogname2);

    std::vector<Kernel::DateAndTime> times1 = p1->timesAsVector();
    std::vector<Kernel::DateAndTime> times2 = p2->timesAsVector();

    Kernel::TimeSeriesProperty<double> *rp = new Kernel::TimeSeriesProperty<double>(ologname);

    // 2. Merge
    size_t index1 = 0;
    size_t index2 = 0;
    bool icont = true;

    Kernel::DateAndTime tmptime;
    double tmpvalue;
    bool launch1 = true;;
    bool nocomparison = false;

    std::cout << "Merging!!" << std::endl;

    while (icont)
    {
      // std::cout << "index1 = " << index1 << ", index2 = " << index2 << ", launch1 = " << launch1 << ", nocomparison = " << nocomparison << std::endl;

      // i. Determine which log to work on
      if (!nocomparison){
        if (times1[index1] < times2[index2]){
          launch1 = true;
        } else {
          launch1 = false;
        }
      }

      // ii. Retrieve data from source log
      if (launch1){
        // Add log1
        tmptime = times1[index1];
        tmpvalue = p1->getSingleValue(tmptime);
      } else {
        // Add log 2
        tmptime = times2[index2];
        tmpvalue = p2->getSingleValue(tmptime);
      }

      // iii. Add log
      rp->addValue(tmptime, tmpvalue);

      // iv. Increase step
      if (launch1){
        index1 ++;
      } else {
        index2 ++;
      }

      // v. Determine status
      if (nocomparison){
        // no comparison case: transition to terminate while
        if (launch1 && index1 >= times1.size()){
          icont = false;
        }
        else if (!launch1 && index2 >= times2.size()){
          icont = false;
        }
      } else {
        // still in comparison: transition to no-comparison
        if (launch1 && index1 >= times1.size()){
          nocomparison = true;
          launch1 = false;
        }
        else if (!launch1 && index2 >= times2.size()){
          nocomparison = true;
          launch1 = true;
        }
      } // ENDIFELSE nocomparison
    } // ENDWHILE

    // 3. Check and add new log
    int newlogsize = rp->size();
    if (size_t(newlogsize) != (times1.size() + times2.size())){
      g_log.error() << "Resulted log size is not equal to the sum of two source log sizes" << std::endl;
      throw;
    }

    matrixWS->mutableRun().addProperty(rp);

    return;
  }

  /*
   * Get reference to Time Sereis log
   */
  Kernel::TimeSeriesProperty<double>* Merge2WorkspaceLogs::getTimeSeriesLog(std::string logname){

    // 1. Get property
    Kernel::Property *prop = matrixWS->run().getLogData(logname);
    if (!prop){
      g_log.error() << "Unable to find log " << logname << " of workspace " << matrixWS->getName() << std::endl;
      throw;
    }

    // 2. Convert to TimeSeries

    Kernel::TimeSeriesProperty<double> *timeprop = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(prop);
    if (!timeprop){
      g_log.error() << "Property (log) " << logname << " is not of class TimeSeriesProperty!" << std::endl;
      throw;
    }

    return timeprop;
  }



} // namespace Mantid
} // namespace DataHandling
