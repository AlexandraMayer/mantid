//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysisFitDataTab.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"

#include <boost/shared_ptr.hpp>

#include <QDesktopServices>
#include <QUrl>
//-----------------------------------------------------------------------------

namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{


void MuonAnalysisFitDataTab::init()
{
  connect(m_uiForm.muonAnalysisHelpDataAnalysis, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpDataAnalysisClicked()));
}


/**
* Muon Analysis Data Analysis help (slot)
*/
void MuonAnalysisFitDataTab::muonAnalysisHelpDataAnalysisClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysisDataAnalysis"));
}


/**
* Make a raw workspace by cloning the workspace given which isn't bunched.
*
* @params wsName :: The name of the current data (shouldn't be bunched) to clone.
*/
void MuonAnalysisFitDataTab::makeRawWorkspace(const std::string& wsName)
{
  Mantid::API::Workspace_sptr inputWs = boost::dynamic_pointer_cast<Mantid::API::Workspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsName) );
  Mantid::API::IAlgorithm_sptr duplicate = Mantid::API::AlgorithmManager::Instance().create("CloneWorkspace");
  duplicate->setProperty<Mantid::API::Workspace_sptr>("InputWorkspace", inputWs);
  duplicate->setPropertyValue("OutputWorkspace", wsName + "_Raw");
  duplicate->execute();

  Mantid::API::Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
  Mantid::API::MatrixWorkspace_sptr outputWs = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(temp);
  //Mantid::API::AnalysisDataService::Instance().add(wsName + "_Raw", outputWs);
}


/**
* Groups the given workspace group with the raw workspace that is associated with
* the workspace name which is also given.
*
* @params wsName :: The name of the workspace the raw file is associated to.
* @params wsGroupName :: The name of the workspaceGroup to join with and what to call the output.
*/
void MuonAnalysisFitDataTab::groupRawWorkspace(const std::vector<std::string> & inputWorkspaces, const std::string & groupName)
{
  Mantid::API::IAlgorithm_sptr groupingAlg = Mantid::API::AlgorithmManager::Instance().create("GroupWorkspaces");
  groupingAlg->setProperty("InputWorkspaces", inputWorkspaces);
  groupingAlg->setPropertyValue("OutputWorkspace", groupName);
  groupingAlg->execute();
}


/**
* Group the fitted workspaces that are created from the 'fit' algorithm
*
* @params workspaceName :: The workspaceName that the fit has been done against
*/
void MuonAnalysisFitDataTab::groupFittedWorkspaces(QString workspaceName)
{
  std::string groupName = workspaceName.left(workspaceName.find(';')).toStdString();
  std::string wsNormalised = workspaceName.toStdString() + "_NormalisedCovarianceMatrix";
  std::string wsParameters = workspaceName.toStdString() + "_Parameters";
  std::string wsWorkspace = workspaceName.toStdString() + "_Workspace";
  std::vector<std::string> inputWorkspaces;

  if ( Mantid::API::AnalysisDataService::Instance().doesExist(groupName) )
  {
    inputWorkspaces.push_back(groupName);

    if ( Mantid::API::AnalysisDataService::Instance().doesExist(wsNormalised) )
    {
      inputWorkspaces.push_back(wsNormalised);
    }
    if ( Mantid::API::AnalysisDataService::Instance().doesExist(wsParameters) )
    {
      inputWorkspaces.push_back(wsParameters);
    }
    if ( Mantid::API::AnalysisDataService::Instance().doesExist(wsWorkspace) )
    {
      inputWorkspaces.push_back(wsWorkspace);
    }
  }

  if (inputWorkspaces.size() > 1)
  {
    groupRawWorkspace(inputWorkspaces, groupName);
  }
}


/**
* Set up the string that will contain all the data needed for changing the data.
* [wsName, axisLabel, connectType, plotType, Errors, Color]
*
* @params plotDetails :: The workspace name of the plot to be created and axis label. 
*/
QStringList MuonAnalysisFitDataTab::getAllPlotDetails(const QStringList & plotDetails)
{
  QStringList allPlotDetails(plotDetails);

  QString fitType("");
  fitType.setNum(m_uiForm.connectPlotType->currentIndex());

  allPlotDetails.push_back(fitType);
  allPlotDetails.push_back("Data");
  if(m_uiForm.showErrorBars->isChecked())
  {
    allPlotDetails.push_back("AllErrors");
  }
  else
  {
    allPlotDetails.push_back("NoErrors");
  }
  allPlotDetails.push_back("Black");

  return(allPlotDetails);
}

}
}
}