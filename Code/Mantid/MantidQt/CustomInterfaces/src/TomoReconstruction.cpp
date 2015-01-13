#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtCustomInterfaces/TomoReconstruction.h"
#include "MantidAPI/TableRow.h"

#include "QFileDialog"
#include "QMessageBox"

#include <boost/uuid/uuid.hpp>            
#include <boost/uuid/uuid_io.hpp>

#include <nexus/NeXusException.hpp>
#include <nexus/NeXusFile.hpp>

#include <algorithm>
#include <jsoncpp/json/json.h>
#include <Poco/File.h>

using namespace Mantid::API;

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(TomoReconstruction);
  }
}

using namespace MantidQt::CustomInterfaces;


TomoReconstruction::TomoReconstruction(QWidget *parent) : UserSubWindow(parent)
{
  m_currentParamPath = "";
  m_rng = boost::uuids::random_generator();
}

void TomoReconstruction::initLayout()
{
  // TODO: should split the tabs out into their own files
  m_uiForm.setupUi(this);  

  // Setup Parameter editor tab  
  loadAvailablePlugins();
  m_uiForm.treeCurrentPlugins->setHeaderHidden(true);

  // Setup the setup tab

  // Setup Run tab
  loadSettings();

  // Connect slots  
  // Menu Items
  connect(m_uiForm.actionOpen, SIGNAL(triggered()), this, SLOT(menuOpenClicked()));
  connect(m_uiForm.actionSave, SIGNAL(triggered()), this, SLOT(menuSaveClicked()));
  connect(m_uiForm.actionSaveAs, SIGNAL(triggered()), this, SLOT(menuSaveAsClicked()));  

  // Lists/trees
  connect(m_uiForm.listAvailablePlugins, SIGNAL(itemSelectionChanged()), this, SLOT(availablePluginSelected()));  
  connect(m_uiForm.treeCurrentPlugins, SIGNAL(itemSelectionChanged()), this, SLOT(currentPluginSelected())); 
  connect(m_uiForm.treeCurrentPlugins, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(expandedItem(QTreeWidgetItem*))); 
  
  // Buttons    
  connect(m_uiForm.btnTransfer, SIGNAL(released()), this, SLOT(transferClicked()));  
  connect(m_uiForm.btnMoveUp, SIGNAL(released()), this, SLOT(moveUpClicked()));  
  connect(m_uiForm.btnMoveDown, SIGNAL(released()), this, SLOT(moveDownClicked()));  
  connect(m_uiForm.btnRemove, SIGNAL(released()), this, SLOT(removeClicked()));  
}


/**
 * Load the setting for each tab on the interface.
 *
 * This includes setting the default browsing directory to be the default save directory.
 */
void TomoReconstruction::loadSettings()
{
  // TODO:
}

void TomoReconstruction::loadAvailablePlugins()
{
  // TODO:: load actual plugins - creating a couple of test choices for now (should fetch from remote api when implemented)
  // - Should also verify the param string is valid json when setting
  // Create plugin tables
 
  auto plug1 = Mantid::API::WorkspaceFactory::Instance().createTable();
  auto plug2 = Mantid::API::WorkspaceFactory::Instance().createTable();
  plug1->addColumns("str","name",4);
  plug2->addColumns("str","name",4);
  Mantid::API::TableRow plug1row = plug1->appendRow();
  Mantid::API::TableRow plug2row = plug2->appendRow();
  plug1row << "10001" << "{\"key\":\"val\",\"key2\":\"val2\"}" << "Plugin #1" << "Citation info";
  plug2row << "10002" << "{\"key\":\"val\",\"key2\":\"val2\"}" << "Plugin #2" << "Citation info";
  
  m_availPlugins.push_back(plug1);
  m_availPlugins.push_back(plug2);

  // Update the UI
  refreshAvailablePluginListUI();
}

// Reloads the GUI list of available plugins from the data object :: Populating only through this ensures correct indexing.
void TomoReconstruction::refreshAvailablePluginListUI()
{
  // Table WS structure, id/params/name/cite
  m_uiForm.listAvailablePlugins->clear();
  for(auto it=m_availPlugins.begin();it!=m_availPlugins.end();++it)
  {
    QString str = QString::fromStdString((*it)->cell<std::string>(0,2));
    m_uiForm.listAvailablePlugins->addItem(str);
  }
}

// Reloads the GUI list of current plugins from the data object :: Populating only through this ensures correct indexing.
void TomoReconstruction::refreshCurrentPluginListUI()
{
  // Table WS structure, id/params/name/cite
  m_uiForm.treeCurrentPlugins->clear();
  for(auto it=m_currPlugins.begin();it!=m_currPlugins.end();++it)
  {
    createPluginTreeEntry(*it);
  }
}

// Updates the selected plugin info from Available plugins list.
void TomoReconstruction::availablePluginSelected()
{
  if(m_uiForm.listAvailablePlugins->selectedItems().count() != 0)
  {  
    int currInd = m_uiForm.listAvailablePlugins->currentIndex().row();
    m_uiForm.availablePluginDesc->setText(tableWSToString(m_availPlugins[currInd]));
  }
}

// Updates the selected plugin info from Current plugins list.
void TomoReconstruction::currentPluginSelected()
{
  if(m_uiForm.treeCurrentPlugins->selectedItems().count() != 0 )
  { 
    auto currItem = m_uiForm.treeCurrentPlugins->selectedItems()[0];

    while(currItem->parent() != NULL)
      currItem = currItem->parent();

    int topLevelIndex = m_uiForm.treeCurrentPlugins->indexOfTopLevelItem(currItem);

    m_uiForm.currentPluginDesc->setText(tableWSToString(m_currPlugins[topLevelIndex]));
  }
}

// On user editing a parameter tree item, update the data object to match.
void TomoReconstruction::paramValModified(QTreeWidgetItem* item, int /*column*/)
{  
  OwnTreeWidgetItem *ownItem = dynamic_cast<OwnTreeWidgetItem*>(item);
  int topLevelIndex = -1;

  if(ownItem->getRootParent() != NULL)
  {
    topLevelIndex = m_uiForm.treeCurrentPlugins->indexOfTopLevelItem(ownItem->getRootParent());
  }
  
  if(topLevelIndex != -1)
  {
    // Recreate the json string from the nodes and write back
    ::Json::Value root;
    std::string json = m_currPlugins[topLevelIndex]->cell<std::string>(0,1);
    ::Json::Reader r;

    if(r.parse(json,root))
    {
      // Look for the key and replace it
      root[ownItem->getKey()] = ownItem->text(0).toStdString();
    }
    
    m_currPlugins[topLevelIndex]->cell<std::string>(0,1) = ::Json::FastWriter().write(root);
    currentPluginSelected();
  }
}

// When a top level item is expanded, also expand its child items - if tree items
void TomoReconstruction::expandedItem(QTreeWidgetItem* item)
{
  if(item->parent() == NULL)
  {
    for(int i=0; i<item->childCount();++i)
    {
      item->child(i)->setExpanded(true); 
    }
  }
}



// Clones the selected available plugin object into the current plugin vector and refreshes the UI.
void TomoReconstruction::transferClicked()
{
  if(m_uiForm.listAvailablePlugins->selectedItems().count() != 0)
  {  
    int currInd = m_uiForm.listAvailablePlugins->currentIndex().row();
    
    ITableWorkspace_sptr newPlugin(m_availPlugins.at(currInd)->clone());

    // Creates a hidden ws entry (with name) in the ADS    
    AnalysisDataService::Instance().add(createUniqueNameHidden(), newPlugin);
   
    m_currPlugins.push_back(newPlugin);
    
    createPluginTreeEntry(newPlugin);
  }
}

void TomoReconstruction::moveUpClicked()
{
  if(m_uiForm.treeCurrentPlugins->selectedItems().count() != 0)
  {      
    int currInd = m_uiForm.treeCurrentPlugins->currentIndex().row();
    if(currInd > 0)
    {
      std::iter_swap(m_currPlugins.begin()+currInd,m_currPlugins.begin()+currInd-1);    
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::moveDownClicked()
{
  if(m_uiForm.treeCurrentPlugins->selectedItems().count() != 0)
  {      
    unsigned int currInd = m_uiForm.treeCurrentPlugins->currentIndex().row();
    if(currInd < m_currPlugins.size()-1 )
    {
      std::iter_swap(m_currPlugins.begin()+currInd,m_currPlugins.begin()+currInd+1);    
      refreshCurrentPluginListUI();
    }
  }
}

void TomoReconstruction::removeClicked()
{
  // Also clear ADS entries
  if(m_uiForm.treeCurrentPlugins->selectedItems().count() != 0)
  {  
    int currInd = m_uiForm.treeCurrentPlugins->currentIndex().row();
    auto curr = *(m_currPlugins.begin()+currInd);    

    if(AnalysisDataService::Instance().doesExist(curr->getName()))
    {
        AnalysisDataService::Instance().remove(curr->getName());
    }
    m_currPlugins.erase(m_currPlugins.begin()+currInd);
    
    refreshCurrentPluginListUI();
  }
}

void TomoReconstruction::menuOpenClicked()
{ 
  QString s = QFileDialog::getOpenFileName(0,"Open file",QDir::currentPath(),
                                           "NeXus files (*.nxs);;All files (*.*)",
                                           new QString("NeXus files (*.nxs)"));
  std::string returned = s.toStdString();
  if(returned != "")
  {    
    if(!Poco::File(returned).exists())
    {
      // File not found, alert and return 
      QMessageBox::information(this, tr("Unable to open file"), "The selected file doesn't exist.");
      return;
    }
    
    bool opening = true;
    
    if(m_currPlugins.size() > 0)
    {
      QMessageBox::StandardButton reply = QMessageBox::question(this, 
          "Open file confirmation", "Opening the configuration file will clear the current list.\nWould you like to continue?",
          QMessageBox::Yes|QMessageBox::No);
      if (reply == QMessageBox::No) 
      {
        opening = false;
      }     
    } 

    if(opening)
    {
      loadTomoConfig(returned, m_currPlugins);

      m_currentParamPath = returned;
      refreshCurrentPluginListUI();  
    }
  }
}

void TomoReconstruction::menuSaveClicked()
{
  if(m_currentParamPath == "")
  {
    menuSaveAsClicked();
    return;
  }
  
  if(m_currPlugins.size() != 0)
  {
    std::string csvWorkspaceNames = "";
    for(auto it=m_currPlugins.begin();it!=m_currPlugins.end();++it)
    {
      csvWorkspaceNames = csvWorkspaceNames + (*it)->name();
      if(it!=m_currPlugins.end()-1)
        csvWorkspaceNames = csvWorkspaceNames + ",";
    }
  
    auto alg = Algorithm::fromString("SaveTomoConfig");
    alg->initialize();
    alg->setPropertyValue("Filename", m_currentParamPath);
    alg->setPropertyValue("InputWorkspaces", csvWorkspaceNames);
    alg->execute();

    if (!alg->isExecuted())
    {
      throw std::runtime_error("Error when trying to save config file");
    }
  }
  else
  {
    // Alert that the plugin list is empty
    QMessageBox::information(this, tr("Unable to save file"), "The current plugin list is empty, please add one or more to the list.");
  }
}

void TomoReconstruction::menuSaveAsClicked()
{
  QString s = QFileDialog::getSaveFileName(0,"Save file",QDir::currentPath(),
                                           "NeXus files (*.nxs);;All files (*.*)",
                                           new QString("NeXus files (*.nxs)"));
  std::string returned = s.toStdString();
  if(returned != "")
  {
    m_currentParamPath = returned;
    menuSaveClicked();
  }
}

QString TomoReconstruction::tableWSToString(ITableWorkspace_sptr table)
{
  std::stringstream msg;
  TableRow row = table->getFirstRow();
  msg << "ID: " << 
    table->cell<std::string>(0,0) << "\nParams: " << 
    table->cell<std::string>(0,1) << "\nName: " << 
    table->cell<std::string>(0,2) << "\nCite: " << 
    table->cell<std::string>(0,3);
  return QString::fromStdString(msg.str());
}

/// Load a tomo config file into the current plugin list, overwriting it.
void TomoReconstruction::loadTomoConfig(std::string &filePath, std::vector<Mantid::API::ITableWorkspace_sptr> &currentPlugins)
{
  // TODO: update with finalised config file structure
  Poco::File file(filePath);
  if(file.exists())
  {
    // Create the file handle
    NXhandle fileHandle;
    NXstatus status = NXopen(filePath.c_str(), NXACC_READ, &fileHandle);
      
    if(status==NX_ERROR)
      throw std::runtime_error("Unable to open file.");   
 
    // Clear the previous plugin list and remove any ADS entries
    for(auto it = currentPlugins.begin(); it!=currentPlugins.end();++it)
    {
      ITableWorkspace_sptr curr = boost::dynamic_pointer_cast<ITableWorkspace>((*it));
      if(AnalysisDataService::Instance().doesExist(curr->getName()))
      {
        AnalysisDataService::Instance().remove(curr->getName());
      }
    }
    currentPlugins.clear();

    ::NeXus::File nxFile(fileHandle);    
   
    nxFile.openPath("/entry1/processing");
    std::map<std::string,std::string> plugins = nxFile.getEntries();
    for(auto it = plugins.begin(); it != plugins.end(); it++) 
    {
      // Create a new plugin table object and read the file information in to it.
      nxFile.openGroup(it->first,"NXsubentry");

      auto plug = Mantid::API::WorkspaceFactory::Instance().createTable();

      plug->addColumns("str","name",4);
      Mantid::API::TableRow plug1row = plug->appendRow();

      // Column info order is [ID / Params {as json string} / name {description} / citation info]
      std::string id, params, name, cite;
      nxFile.readData<std::string>("id", id);
      nxFile.readData<std::string>("params", params);
      nxFile.readData<std::string>("name", name);
      nxFile.readData<std::string>("cite", cite);

      plug1row << id << params << name << cite;      

      // Creates a hidden ws entry (with name) in the ADS
      AnalysisDataService::Instance().add(createUniqueNameHidden(), plug);
      currentPlugins.push_back(plug);    

      nxFile.closeGroup();
    }
    
    nxFile.close();
  }
  else
  {
    // Alert invalid path
    QMessageBox::information(this, tr("Unable to open file"), "The selected file doesn't exist.");
  }
}

// Find a unique name for the table ws with a __ prefix to indicate it should be hidden
std::string TomoReconstruction::createUniqueNameHidden()
{
  std::string name;

  do 
  { 
    boost::uuids::uuid rndUuid = m_rng();    
    name = "__TomoConfigTableWS_" +  boost::uuids::to_string(rndUuid);
  } 
  while( AnalysisDataService::Instance().doesExist(name) );

  return name;
}

// Creates a treewidget item for a table workspace
void TomoReconstruction::createPluginTreeEntry(Mantid::API::ITableWorkspace_sptr table)
{ 
  QStringList idStr, paramsStr, nameStr, citeStr;
  idStr.push_back(QString::fromStdString("ID: " + table->cell<std::string>(0,0)));
  paramsStr.push_back(QString::fromStdString("Params:"));
  nameStr.push_back(QString::fromStdString("Name: " + table->cell<std::string>(0,2)));
  citeStr.push_back(QString::fromStdString("Cite: " + table->cell<std::string>(0,3)));

  // Setup editable tree items
  QList<QTreeWidgetItem*> items;
  OwnTreeWidgetItem *pluginBaseItem = new OwnTreeWidgetItem(nameStr);
  OwnTreeWidgetItem *pluginParamsItem = new OwnTreeWidgetItem(pluginBaseItem, paramsStr, pluginBaseItem);

  // Add to the tree list. Adding now to build hierarchy for later setItemWidget call
  items.push_back(new OwnTreeWidgetItem(pluginBaseItem, idStr, pluginBaseItem));
  items.push_back(pluginParamsItem);
  items.push_back(new OwnTreeWidgetItem(pluginBaseItem, nameStr, pluginBaseItem));
  items.push_back(new OwnTreeWidgetItem(pluginBaseItem, citeStr, pluginBaseItem));

  pluginBaseItem->addChildren(items);
  m_uiForm.treeCurrentPlugins->addTopLevelItem(pluginBaseItem);
  
  // Params will be a json string which needs splitting into child tree items [key/value]
  ::Json::Value root;
  std::string json = table->cell<std::string>(0,1);
  ::Json::Reader r;
  if(r.parse(json,root))
  {
    auto members = root.getMemberNames();
    for(auto it=members.begin();it!=members.end();++it)
    {
      OwnTreeWidgetItem *container = new OwnTreeWidgetItem(pluginParamsItem, pluginBaseItem);
      
      QWidget *w = new QWidget();
      w->setAutoFillBackground(true);
    
      QHBoxLayout *layout = new QHBoxLayout(w);
      layout->setMargin(1);
      QLabel* label1 = new QLabel(QString::fromStdString((*it) + ": ")); 

      QTreeWidget *paramContainerTree = new QTreeWidget(w);    
      connect(paramContainerTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(paramValModified(QTreeWidgetItem*,int))); 
      paramContainerTree->setHeaderHidden(true);
      paramContainerTree->setIndentation(0);
      
      QStringList paramVal(QString::fromStdString(root[*it].asString()));
      OwnTreeWidgetItem *paramValueItem = new OwnTreeWidgetItem(paramVal, pluginBaseItem, *it);
      paramValueItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled );

      paramContainerTree->addTopLevelItem(paramValueItem);
      QRect rect = paramContainerTree->visualItemRect(paramValueItem);    
      paramContainerTree->setMaximumHeight(rect.height());
      paramContainerTree->setFrameShape(QFrame::NoFrame);

      layout->addWidget(label1); 
      layout->addWidget(paramContainerTree);

      pluginParamsItem->addChild(container); 
      m_uiForm.treeCurrentPlugins->setItemWidget(container,0,w);
    }     
  }  
}