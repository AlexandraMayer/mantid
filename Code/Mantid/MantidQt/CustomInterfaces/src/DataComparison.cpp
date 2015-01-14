//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/DataComparison.h"

#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"


namespace
{
  Mantid::Kernel::Logger g_log("DataComparison");
}

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(DataComparison);
}
}

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;


//----------------------
// Public member functions
//----------------------
///Constructor
DataComparison::DataComparison(QWidget *parent) :
  UserSubWindow(parent),
  m_plot(new QwtPlot(parent))
{
}


/// Set up the dialog layout
void DataComparison::initLayout()
{
  m_uiForm.setupUi(this);

  // Add the plot to the UI
  m_plot->setCanvasBackground(Qt::white);
  m_uiForm.loPlot->addWidget(m_plot);

  // Connect push buttons
  connect(m_uiForm.pbAddData, SIGNAL(clicked()), this, SLOT(addData()));

  connect(m_uiForm.pbRemoveSelectedData, SIGNAL(clicked()), this, SLOT(removeSelectedData()));
  connect(m_uiForm.pbRemoveAllData, SIGNAL(clicked()), this, SLOT(removeAllData()));

  connect(m_uiForm.pbDiffSelected, SIGNAL(clicked()), this, SLOT(diffSelected()));
  connect(m_uiForm.pbClearDiff, SIGNAL(clicked()), this, SLOT(clearDiff()));

  // Replot spectra when the spectrum index is changed
  connect(m_uiForm.sbSpectrum, SIGNAL(currentIndexChanged(int)), this, SLOT(plotWorkspaces()));

  // Add headers to data table
  QStringList headerLabels;
  headerLabels << "Colour" << "Workspace" << "Offset" << "Spec.";
  m_uiForm.twCurrentData->setColumnCount(headerLabels.size());
  m_uiForm.twCurrentData->setHorizontalHeaderLabels(headerLabels);

  // Select entire rows when a cell is selected
  m_uiForm.twCurrentData->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Fit columns
  m_uiForm.twCurrentData->resizeColumnsToContents();
}


/**
 * Adds the data currently selected by the data selector to the plot.
 */
void DataComparison::addData()
{
  QString dataName = m_uiForm.dsData->getCurrentDataName();

  // Append a new row to the data table
  int currentRows = m_uiForm.twCurrentData->rowCount();
  m_uiForm.twCurrentData->insertRow(currentRows);

  // Insert the workspace name
  QTableWidgetItem *wsNameItem = new QTableWidgetItem(tr(dataName));
  wsNameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_uiForm.twCurrentData->setItem(currentRows, WORKSPACE_NAME, wsNameItem);

  // Insert the colour
  QTableWidgetItem *colourItem = new QTableWidgetItem(tr(""));
  colourItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_uiForm.twCurrentData->setItem(currentRows, COLOUR, colourItem);

  // Insert the spectra offset
  QTableWidgetItem *offsetItem = new QTableWidgetItem(tr("0"));
  offsetItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  m_uiForm.twCurrentData->setItem(currentRows, SPEC_OFFSET, offsetItem);

  // Insert the current displayed spectra
  QTableWidgetItem *currentSpecItem = new QTableWidgetItem(tr(""));
  currentSpecItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_uiForm.twCurrentData->setItem(currentRows, CURRENT_SPEC, currentSpecItem);

  // Fit columns
  m_uiForm.twCurrentData->resizeColumnsToContents();

  // Replot the workspaces
  plotWorkspaces();
}


/**
 * Removes the data currently selected in the table from the plot.
 */
void DataComparison::removeSelectedData()
{
  QList<QTableWidgetItem *> selectedItems = m_uiForm.twCurrentData->selectedItems();

  while(!selectedItems.isEmpty())
  {
    // Get the row number of the item
    int row = selectedItems[0]->row();

    // Get workspace name
    QString workspaceName = m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();

    // Remove from data tabel
    m_uiForm.twCurrentData->removeRow(row);

    // Detach the old curve from the plot if it exists
    if(m_curves.contains(workspaceName))
      m_curves[workspaceName]->attach(NULL);

    selectedItems = m_uiForm.twCurrentData->selectedItems();
  }

  // Replot the workspaces
  plotWorkspaces();
}


/**
 * Removed all loaded data from the plot.
 */
void DataComparison::removeAllData()
{
  int numRows = m_uiForm.twCurrentData->rowCount();
  for(int row = 0; row < numRows; row++)
  {
    // Get workspace name
    QString workspaceName = m_uiForm.twCurrentData->item(0, WORKSPACE_NAME)->text();

    // Remove from data tabel
    m_uiForm.twCurrentData->removeRow(0);

    // Detach the old curve from the plot if it exists
    if(m_curves.contains(workspaceName))
      m_curves[workspaceName]->attach(NULL);

    // Replot the workspaces
    plotWorkspaces();
  }
}


/**
 * Replots the currently loaded workspaces.
 */
void DataComparison::plotWorkspaces()
{
  int numRows = m_uiForm.twCurrentData->rowCount();
  for(int row = 0; row < numRows; row++)
  {
    int specIndex = 0;  //TODO

    // Get workspace
    QString workspaceName = m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();
    MatrixWorkspace_const_sptr workspace =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());

    // Create the curve data
    const bool logScale(false), distribution(false);
    QwtWorkspaceSpectrumData wsData(*workspace, static_cast<int>(specIndex), logScale, distribution);

    // Check the spectrum index is in range
    int numSpec = static_cast<int>(workspace->getNumberHistograms());
    if(specIndex >= numSpec)
    {
      g_log.debug() << "Workspace " << workspaceName.toStdString() << ", spectrum index out of range.";
      continue;
    }

    // Detach the old curve from the plot if it exists
    if(m_curves.contains(workspaceName))
      m_curves[workspaceName]->attach(NULL);

    // Create a new curve and attach it to the plot
    boost::shared_ptr<QwtPlotCurve> curve(new QwtPlotCurve);
    curve->setData(wsData);
    curve->attach(m_plot);
    m_curves[workspaceName] = curve;
  }

  // Update the plot
  m_plot->replot();
}


/**
 * Creates a diff workspace of the two currently selected workspaces in the table
 * and plots it on the plot.
 *
 * Does nothing if there are not 2 workspaces selected.
 */
void DataComparison::diffSelected()
{
  //TODO
}


/**
 * Removes the diff workspace form the plot.
 *
 * Does not remove it from ADS.
 */
void DataComparison::clearDiff()
{
  //TODO
}
