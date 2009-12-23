//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <boost/tokenizer.hpp>
#include <string>
#include <iostream>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/LibraryManager.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

using namespace std;

namespace Mantid
{
namespace API
{
/// Default constructor
FrameworkManagerImpl::FrameworkManagerImpl() : g_log(Kernel::Logger::get("FrameworkManager"))
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

#ifdef _MSC_VER
  // This causes the exponent to consist of two digits (Windows Visual Studio normally 3, Linux default 2), where two digits are not sufficient I presume it uses more
  _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  std::string pluginDir = Kernel::ConfigService::Instance().getString("plugins.directory");
  if (pluginDir.length() > 0)
  {
    Mantid::Kernel::LibraryManager::Instance().OpenAllLibraries(pluginDir, false);
  }
  g_log.debug() << "FrameworkManager created." << std::endl;
}

/// Destructor
FrameworkManagerImpl::~FrameworkManagerImpl()
{
//	std::cerr << "FrameworkManager destroyed." << std::endl;
//	g_log.debug() << "FrameworkManager destroyed." << std::endl;
}

/** Clears all memory associated with the AlgorithmManager
 *  and with the Analysis & Instrument data services.
 */
void FrameworkManagerImpl::clear()
{
  clearAlgorithms();
  clearData();
  clearInstruments();
}

/**
 * Clear memory associated with the AlgorithmManager
 */
void FrameworkManagerImpl::clearAlgorithms()
{
  AlgorithmManager::Instance().clear();
}

/**
 * Clear memory associated with the ADS
 */
void FrameworkManagerImpl::clearData()
{
  AnalysisDataService::Instance().clear();
}

/**
 * Clear memory associated with the IDS
 */
void FrameworkManagerImpl::clearInstruments()
{
  InstrumentDataService::Instance().clear();
}

/** Creates and initialises an instance of an algorithm
 * 
 *  @param algName The name of the algorithm required
 *  @param version The version of the algorithm
 *  @return A pointer to the created algorithm
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 */
IAlgorithm* FrameworkManagerImpl::createAlgorithm(const std::string& algName, const int& version)
{ 
   IAlgorithm* alg = AlgorithmManager::Instance().create(algName,version).get();
   return alg;
}

/** Creates an instance of an algorithm and sets the properties provided
 * 
 *  @param algName The name of the algorithm required
 *  @param propertiesArray A single string containing properties in the 
 *                         form "Property1=Value1;Property2=Value2;..."
 *  @param version The version of the algorithm
 *  @return A pointer to the created algorithm
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 *  @throw std::invalid_argument Thrown if properties string is ill-formed
 */ 
IAlgorithm* FrameworkManagerImpl::createAlgorithm(const std::string& algName,const std::string& propertiesArray, const int& version)
{
  // Use the previous method to create the algorithm
  IAlgorithm *alg = AlgorithmManager::Instance().create(algName,version).get();//createAlgorithm(algName);
  alg->setProperties(propertiesArray);
  return alg;
}

/** Creates an instance of an algorithm, sets the properties provided and
 *       then executes it.
 * 
 *  @param algName The name of the algorithm required
 *  @param propertiesArray A single string containing properties in the 
 *                         form "Property1=Value1;Property2=Value2;..."
 *  @param version The version of the algorithm
 *  @return A pointer to the executed algorithm
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 *  @throw std::invalid_argument Thrown if properties string is ill-formed
 *  @throw runtime_error Thrown if algorithm cannot be executed
 */ 
IAlgorithm* FrameworkManagerImpl::exec(const std::string& algName, const std::string& propertiesArray, const int& version)
{
  // Make use of the previous method for algorithm creation and property setting
  IAlgorithm *alg = createAlgorithm(algName, propertiesArray,version);
  
  // Now execute the algorithm
  alg->execute();
  
  return alg;
}

/** Returns a shared pointer to the workspace requested
 * 
 *  @param wsName The name of the workspace
 *  @return A pointer to the workspace
 * 
 *  @throw NotFoundError If workspace is not registered with analysis data service
 */
Workspace* FrameworkManagerImpl::getWorkspace(const std::string& wsName)
{
  Workspace *space;
  try
  {
    space = AnalysisDataService::Instance().retrieve(wsName).get();
  }
  catch (Kernel::Exception::NotFoundError&)
  {
    throw Kernel::Exception::NotFoundError("Unable to retrieve workspace",wsName);
  }
  return space;
}

/** Removes and deletes a workspace from the data service store.
 * 
 *  @param wsName The user-given name for the workspace 
 *  @return true if the workspace was found and deleted
 * 
 *  @throw NotFoundError Thrown if workspace cannot be found
 */
bool FrameworkManagerImpl::deleteWorkspace(const std::string& wsName)
{
  bool retVal = false;
  try
  {
    AnalysisDataService::Instance().remove(wsName);
    retVal = true;
  }
  catch (Kernel::Exception::NotFoundError&)
  {
    //workspace was not found
    g_log.error()<<"Workspace "<<wsName<<" could not be found."<<std::endl;
    retVal = false;
  }
  return retVal;
}

} // namespace API
} // Namespace Mantid
