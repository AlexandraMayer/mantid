//---------------------------------------
// Includes
//------------------------------------
#include "MantidPythonAPI/FrameworkManagerProxy.h"

#include <boost/python/handle.hpp>
#include <boost/python/extract.hpp>
#include <fstream>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"

#include "MantidPythonAPI/PyAlgorithmWrapper.h"
#include "MantidPythonAPI/SimplePythonAPI.h"

#include "MantidKernel/ConfigService.h"

namespace Mantid
{
namespace PythonAPI
{

// Initialize the logger
Mantid::Kernel::Logger& FrameworkManagerProxy::g_log = Mantid::Kernel::Logger::get("MantidPython");

// Initialize GIL flag
bool FrameworkManagerProxy::g_gil_required = false;

// Save the flag used on the last creation of the simple API
bool FrameworkManagerProxy::g_last_api_flag = false;

/// Default constructor
FrameworkManagerProxy::FrameworkManagerProxy() 
  : m_delete_observer(*this, &FrameworkManagerProxy::deleteNotificationReceived),
    m_add_observer(*this, &FrameworkManagerProxy::addNotificationReceived),
    m_replace_observer(*this, &FrameworkManagerProxy::replaceNotificationReceived),
    m_clear_observer(*this, &FrameworkManagerProxy::clearNotificationReceived),
    m_algupdate_observer(*this, &FrameworkManagerProxy::handleAlgorithmFactoryUpdate)
{
  API::FrameworkManager::Instance();
  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);
  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_add_observer);
  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_replace_observer);
  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_clear_observer);

  // Create a blank module to satisfy Python algorithm dependencies on startup so that we don't have to create the
  // actual module twice on startup
  std::string simpleapi = SimplePythonAPI::getModuleFilename();
  std::ofstream file(simpleapi.c_str(), std::ios_base::out);
}

///Destructor
FrameworkManagerProxy::~FrameworkManagerProxy()
{
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_clear_observer);
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replace_observer);
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_add_observer);
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_delete_observer);
  API::AlgorithmFactory::Instance().notificationCenter.removeObserver(m_algupdate_observer);
}

void FrameworkManagerProxy::observeAlgFactoryUpdates(bool listen, bool force_update)
{
  if( listen )
  {
    API::AlgorithmFactory::Instance().notificationCenter.addObserver(m_algupdate_observer);
    if( force_update )
    {
      API::AlgorithmFactory::Instance().notificationCenter.postNotification(new API::AlgorithmFactoryUpdateNotification);
    }
  }
  else
  {
    API::AlgorithmFactory::Instance().notificationCenter.removeObserver(m_algupdate_observer);
  }
}

/// Clear the FrameworkManager	
void FrameworkManagerProxy::clear()
{
  API::FrameworkManager::Instance().clear();
}

/**
 * Clear memory associated with the AlgorithmManager
 */
void FrameworkManagerProxy::clearAlgorithms()
{
  API::FrameworkManager::Instance().clearAlgorithms();
}

/**
 * Clear memory associated with the ADS
 */
void FrameworkManagerProxy::clearData()
{
  API::FrameworkManager::Instance().clearData();
}

/**
 * Clear memory associated with the IDS
 */
void FrameworkManagerProxy::clearInstruments()
{
  API::FrameworkManager::Instance().clearInstruments();
}

/**
 * Return a string property from the ConfigService
 * @param key the key of the config value you require
 * @returns the value of the config item you require
 */
std::string FrameworkManagerProxy::getConfigProperty(const std::string & key) const
{
  return Kernel::ConfigService::Instance().getString(key);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
  * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::createAlgorithm(const std::string& algName)
{
  return API::FrameworkManager::Instance().createAlgorithm(algName);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param version :: The version of the algorithm to use.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::createAlgorithm(const std::string& algName, const int& version)
{
  return API::FrameworkManager::Instance().createAlgorithm(algName, version);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
  * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::createAlgorithm(const std::string& algName, const std::string& propertiesArray)
{
  return API::FrameworkManager::Instance().createAlgorithm(algName, propertiesArray);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
 * \param version :: The version of the algorithm to use.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::createAlgorithm(const std::string& algName, const std::string& propertiesArray,const int& version)
{
  return API::FrameworkManager::Instance().createAlgorithm(algName, propertiesArray, version);
}

/**
 * Returns a specified MatrixWorkspace.
 * \param wsName :: The name of the workspace to retrieve.
 * \return Shared pointer to workspace.
 **/
boost::shared_ptr<API::MatrixWorkspace> FrameworkManagerProxy::retrieveMatrixWorkspace(const std::string& wsName)
{
  API::MatrixWorkspace_sptr matrix_wksp =  boost::dynamic_pointer_cast<API::MatrixWorkspace>(retrieveWorkspace(wsName));
  if( matrix_wksp.get() )
  {
    return matrix_wksp;
  }
  else
  {
    throw std::runtime_error("\"" + wsName + "\" is not a matrix workspace. "
        "This may be a table workspace, try getTableWorkspace().");
  }
}

/**
* Returns a specified TableWorkspace.
* @param wsName :: The name of the workspace to retrieve.
* @return Shared pointer to workspace.
**/
boost::shared_ptr<API::ITableWorkspace> FrameworkManagerProxy::retrieveTableWorkspace(const std::string& wsName)
{
  API::ITableWorkspace_sptr table_wksp =  boost::dynamic_pointer_cast<API::ITableWorkspace>(retrieveWorkspace(wsName));
  if( table_wksp.get() )
  {
    return table_wksp;
  }
  else
  {
    throw std::runtime_error("\"" + wsName + "\" is not a table workspace. "
        "This may be a matrix workspace, try getMatrixWorkspace().");
  }
}

/**
 * Return a pointer to a WorkspaceGroup
 * @param group_name The name of the group
 * @return A pointer to API::WorkspaceGroup object
 */
boost::shared_ptr<API::WorkspaceGroup> FrameworkManagerProxy::retrieveWorkspaceGroup(const std::string& group_name)
{
  API::WorkspaceGroup_sptr wksp_group = boost::dynamic_pointer_cast<API::WorkspaceGroup>(retrieveWorkspace(group_name));
  if( wksp_group.get() )
  {
    return wksp_group;
  }
  else
  {
    throw std::runtime_error("\"" + group_name + "\" is not a group workspace. "
        "This may be a matrix workspace, try getMatrixWorkspace().");
  }
}

/**
 * Deletes a specified workspace.
 * \param wsName :: The name of the workspace to delete.
 * \return Boolean result.
 **/
bool FrameworkManagerProxy::deleteWorkspace(const std::string& wsName)
{
  return API::FrameworkManager::Instance().deleteWorkspace(wsName);
}

/**
 * Returns the name of all the workspaces.
 * \return Vector of strings.
 **/
std::set<std::string> FrameworkManagerProxy::getWorkspaceNames() const
{
  return API::AnalysisDataService::Instance().getObjectNames();
}

/**
 * Returns the names of all the workspace groups
 * \return A vector of strings.
 **/
std::set<std::string> FrameworkManagerProxy::getWorkspaceGroupNames() const
{
  std::set<std::string> ws_names = getWorkspaceNames();
  std::set<std::string> grp_names;
  std::set<std::string>::const_iterator iend = ws_names.end();
  for( std::set<std::string>::const_iterator itr = ws_names.begin(); itr != iend;
       ++itr )
  {
    API::Workspace *wksp =  API::FrameworkManager::Instance().getWorkspace(*itr);
    if( dynamic_cast<API::WorkspaceGroup*>(wksp) )
    {
      grp_names.insert(*itr);
    }

  }
  return grp_names;
}

/**
 * Get the names within a workspace group
 * @param group_name The name of the group
 * @return Vector of the names of the contained workspaces
 */
std::vector<std::string> FrameworkManagerProxy::getWorkspaceGroupEntries(const std::string & group_name) const
{
  API::WorkspaceGroup* ws_group = 
    dynamic_cast<API::WorkspaceGroup*>( API::FrameworkManager::Instance().getWorkspace(group_name) );
  std::vector<std::string> entries;
  if( ws_group )
  {
    entries = ws_group->getNames();
  }
  return entries;
}

/**
  * Create the simple Python API module
  * @param gui Whether the module is being made for use with qtiplot or not
  **/
void FrameworkManagerProxy::createPythonSimpleAPI(bool gui)
{
  //Redirect to static helper class
  SimplePythonAPI::createModule(gui);
  //Save the flag so that the module can be recreated
  g_last_api_flag = gui;
}

/**
 * Send a log message to Mantid
 * @param msg The log message
 */
void FrameworkManagerProxy::sendLogMessage(const std::string & msg) 
{
  g_log.notice(msg); 
}

/**
 * Check if a given workspace name exists in the ADS 
 * @param name A string specifying a name to check
 * @return true if it exists
 */
bool FrameworkManagerProxy::workspaceExists(const std::string & name) const
{
  return API::AnalysisDataService::Instance().doesExist(name);
}

/**
 * Add a python algorithm to the algorithm factory
 * @param pyobj The python algorithm object wrapped in a boost object
 */
void FrameworkManagerProxy::registerPyAlgorithm(boost::python::object pyobj)
{
  Mantid::API::CloneableAlgorithm* cppobj = boost::python::extract<Mantid::API::CloneableAlgorithm*>(pyobj);
  if( cppobj )
  {
    setGILRequired(true);
    if( ! Mantid::API::AlgorithmFactory::Instance().storeCloneableAlgorithm(cppobj) )
    {
      g_log.error("Unable to register Python algorithm \"" + cppobj->name() + "\"");
    }
    setGILRequired(false);
  }
  else
  {
    throw std::runtime_error("Unrecognized object type in Python algorithm registration.");
  }
}
  
//--------------------------------------------------------------------------
//
// Private member functions
//
//--------------------------------------------------------------------------
/**
 * Get a workspace pointer from the ADS
 * @param wsName The name of the workspace to retrieve, throws if the pointer is invalid
 * @return The workspace requested, a NotFoundError is thrown is it is not present
 */
boost::shared_ptr<Mantid::API::Workspace> FrameworkManagerProxy::retrieveWorkspace(const std::string & wsName)
{
  try
  {
    API::Workspace_sptr wksp = API::AnalysisDataService::Instance().retrieve(wsName);
    return wksp;
  }
  catch(Mantid::Kernel::Exception::NotFoundError &)
  {
    throw std::runtime_error("Workspace \"" + wsName + "\" not found.");
  }
}

/**
 * Utility function called when a workspace is deleted within the service
 * @param notice A pointer to a WorkspaceDeleteNotification object
 */
void FrameworkManagerProxy::
deleteNotificationReceived(Mantid::API::WorkspaceDeleteNotification_ptr notice)
{
  /// This function may be overridden in Python
  workspaceRemoved(notice->object_name());  
}

/**
 * Utility function called when a workspace is added within the service
 * @param notice A pointer to a WorkspaceDeleteNotification object
 */
void FrameworkManagerProxy::
addNotificationReceived(Mantid::API::WorkspaceAddNotification_ptr notice)
{
  (void)notice;
}

/**
 * Utility function called when a workspace is replaced within the service
 * @param notice A pointer to a WorkspaceAfterReplaceNotification object
 */
void FrameworkManagerProxy::
replaceNotificationReceived(Mantid::API::WorkspaceAfterReplaceNotification_ptr notice)
{  
  /// This function may be overridden in Python
  workspaceReplaced(notice->object_name());
}

/**
 * Utility function called when a workspace is replaced within the service
 * @param notice A pointer to a ClearADSNotification object
 */
void FrameworkManagerProxy::clearNotificationReceived(Mantid::API::ClearADSNotification_ptr)
{
  /// This function may be overridden in Python
  workspaceStoreCleared();
}

/**
 * Called by AlgorithmFactory updates
 * @param notice The nofification object
 */
void FrameworkManagerProxy::
handleAlgorithmFactoryUpdate(Mantid::API::AlgorithmFactoryUpdateNotification_ptr)
{
  // First rewrite the simple API
  createPythonSimpleAPI(g_last_api_flag);
  //Call up to python via a virtual function
  algorithmFactoryUpdated();
}

}
}

