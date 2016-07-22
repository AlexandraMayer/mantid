//----------------------------------
// Includes
//----------------------------------

#include "SampleLogDialogBase.h"

// Mantid API
#include <MantidAPI/MultipleExperimentInfos.h>
#include <MantidAPI/IMDWorkspace.h>

// Mantid Kernel
#include <MantidKernel/TimeSeriesProperty.h>
#include <MantidKernel/ArrayProperty.h>

// Qt
#include <QTreeWidget>
#include <QHeaderView>
#include <QFileInfo>
#include <QLineEdit>
#include <QMenu>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

/** Default constructor, initialises the variables but does not initialise
* any widgets on the window. Child classes must provide their own constructor.
*
* This is done in order to avoid confusion and provide full control
* over the initialisation order and placement of the widgets inside
* the window.
*
*	@param wsname The name of the workspace for which the logs will be
*displayed
*	@param parentContainer The parent container relative to which the window
*will be centered.
*						Also delegates the parentContainer
*to
*the QDialog(QWidget, flags) constructor.
*	@param flags Flags for QT
*				Also delegates the flags to the QDialog(QWidget,
*flags) constructor
*	@param experimentInfoIndex Index into the ExperimentInfo list.
*
*	@author Martyn Gigg, Tessella Support Services plc
*	@date 05/11/2009
*
*/
SampleLogDialogBase::SampleLogDialogBase(const QString &wsname,
                                         QWidget *parentContainer,
                                         Qt::WFlags flags,
                                         size_t experimentInfoIndex)
    : QDialog(parentContainer, flags), m_parentContainer(parentContainer),
      m_wsname(wsname.toStdString()),
      m_experimentInfoIndex(experimentInfoIndex), buttonPlot(nullptr),
      buttonClose(nullptr), m_spinNumber(nullptr), m_tree(nullptr) {

  // No further initialisation provided, must be done in derived classes
}

SampleLogDialogBase::~SampleLogDialogBase() {}

//----------------------------------
// Protected methods
//----------------------------------
/**
* Plot the selected log entries (TimeSeriesProperty or PropertyWithValue)
*
*	@author Martyn Gigg, Tessella Support Services plc
*	@date 05/11/2009
*/
void SampleLogDialogBase::importSelectedLogs() {
  QList<QTreeWidgetItem *> items = m_tree->selectedItems();
  QListIterator<QTreeWidgetItem *> pItr(items);
  while (pItr.hasNext()) {
    importItem(pItr.next());
  }
}

/**
* Show Log Statistics when a line is selected
*
*	@author Martyn Gigg, Tessella Support Services plc
*	@date 05/11/2009
*/
void SampleLogDialogBase::showLogStatistics() {
  QList<QTreeWidgetItem *> items = m_tree->selectedItems();
  QListIterator<QTreeWidgetItem *> pItr(items);
  if (pItr.hasNext()) {
    // Show only the first one
    showLogStatisticsOfItem(pItr.next());
  }
}

//------------------------------------------------------------------------------------------------
/**
* Show the stats of the log for the selected item
*
*	@param item :: The item to be imported
*	@throw invalid_argument if format identifier for the item is wrong
*
*	@author Martyn Gigg, Tessella Support Services plc
*	@date 05/11/2009
*/
void SampleLogDialogBase::showLogStatisticsOfItem(QTreeWidgetItem *item) {
  // Assume that you can't show the stats
  for (size_t i = 0; i < NUM_STATS; i++) {
    statValues[i]->setText(QString(""));
  }

  // used in numeric time series below, the default filter value
  int key = item->data(1, Qt::UserRole).toInt();
  switch (key) {
  case numeric:
  case string:
  case stringTSeries:
  case numericArray:
    return;
    break;

  case numTSeries:
    // Calculate the stats
    // Get the workspace
    if (!m_ei)
      return;

    // Now the log
    Mantid::Kernel::TimeSeriesPropertyStatistics stats;
    Mantid::Kernel::Property *logData =
        m_ei->run().getLogData(item->text(0).toStdString());
    // Get the stas if its a series of int or double; fail otherwise
    Mantid::Kernel::TimeSeriesProperty<double> *tspd =
        dynamic_cast<TimeSeriesProperty<double> *>(logData);
    Mantid::Kernel::TimeSeriesProperty<int> *tspi =
        dynamic_cast<TimeSeriesProperty<int> *>(logData);
    double timeAvg = 0.;
    if (tspd) {
      stats = tspd->getStatistics();
      timeAvg = tspd->timeAverageValue();
    } else if (tspi) {
      stats = tspi->getStatistics();
      timeAvg = tspi->timeAverageValue();
    } else
      return;

    // --- Show the stats ---
    statValues[0]->setText(QString::number(stats.minimum));
    statValues[1]->setText(QString::number(stats.maximum));
    statValues[2]->setText(QString::number(stats.mean));
    statValues[3]->setText(QString::number(timeAvg));
    statValues[4]->setText(QString::number(stats.median));
    statValues[5]->setText(QString::number(stats.standard_deviation));
    statValues[6]->setText(QString::number(stats.duration));
    return;
    break;
  }
  throw std::invalid_argument("Error importing log entry, wrong data type");
}

//------------------------------------------------------------------------------------------------
/**
* Popup a custom context menu
*
*	@author Martyn Gigg, Tessella Support Services plc
*	@date 05/11/2009
*/
void SampleLogDialogBase::popupMenu(const QPoint &pos) {
  if (!m_tree->itemAt(pos)) {
    m_tree->selectionModel()->clear();
    return;
  }

  QMenu *menu = new QMenu(m_tree);

  QAction *action = new QAction("Import", m_tree);
  connect(action, SIGNAL(triggered()), this, SLOT(importSelectedLogs()));
  menu->addAction(action);

  menu->popup(QCursor::pos());
}

//------------------------------------------------------------------------------------------------
/**
* Initialize everything in the tree. Must be called after a QTreeWidget
*initialisation
*	@author Martyn Gigg, Tessella Support Services plc
*	@date 05/11/2009
*/
void SampleLogDialogBase::init() {
  m_tree->clear();

  // ------------------- Retrieve the proper ExperimentInfo workspace
  // -------------------------------
  IMDWorkspace_sptr ws =
      AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(m_wsname);
  if (!ws)
    throw std::runtime_error("Wrong type of a workspace (" + m_wsname +
                             " is not an IMDWorkspace)");
  // Is it MatrixWorkspace, which itself is ExperimentInfo?
  m_ei = boost::dynamic_pointer_cast<const ExperimentInfo>(ws);
  ;
  if (!m_ei) {
    boost::shared_ptr<MultipleExperimentInfos> mei =
        boost::dynamic_pointer_cast<MultipleExperimentInfos>(ws);
    if (mei) {
      if (m_experimentInfoIndex >= mei->getNumExperimentInfo()) {
        std::cerr << "ExperimentInfo requested (#" +
                         Strings::toString(m_experimentInfoIndex) +
                         ") is not available. There are " +
                         Strings::toString(mei->getNumExperimentInfo()) +
                         " in the workspace\n";
        // Make a blank experiment info object
        m_ei = ExperimentInfo_const_sptr(new ExperimentInfo());
      } else
        m_ei = mei->getExperimentInfo(
            static_cast<uint16_t>(m_experimentInfoIndex));
    }
  }
  if (!m_ei)
    throw std::runtime_error("Wrong type of a workspace (no ExperimentInfo)");

  const std::vector<Mantid::Kernel::Property *> &logData =
      m_ei->run().getLogData();
  auto pEnd = logData.end();
  int max_length(0);
  for (auto pItr = logData.begin(); pItr != pEnd; ++pItr) {
    // name() contains the full path, so strip to file name
    QString filename = QFileInfo((**pItr).name().c_str()).fileName();
    if (filename.size() > max_length)
      max_length = filename.size();
    QTreeWidgetItem *treeItem = new QTreeWidgetItem(QStringList(filename));

    // store the log contents in the treeItem
    // treeItem->setData(0, Qt::UserRole,
    // QString::fromStdString((*pItr)->value()));

    // NOTE: The line above appears to be completely unused since it is
    // overwritten. And it is real slow.
    //  So commented out, and putting this placeholder instead
    treeItem->setData(0, Qt::UserRole, "value");

    // Set the units text
    treeItem->setText(3, QString::fromStdString((*pItr)->units()));

    // this specifies the format of the data it should be overridden below or
    // there is a problem
    treeItem->setData(1, Qt::UserRole, -1);

    Mantid::Kernel::TimeSeriesProperty<double> *tspd =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(*pItr);
    Mantid::Kernel::TimeSeriesProperty<int> *tspi =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<int> *>(*pItr);
    Mantid::Kernel::TimeSeriesProperty<bool> *tspb =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(*pItr);

    // See what type of data we have
    if (tspd || tspi || tspb) {
      treeItem->setText(1, "num. series");
      // state that the string we passed into data[0] is a time series -multiple
      // lines with a time and then a number
      treeItem->setData(1, Qt::UserRole, static_cast<int>(numTSeries));
      std::ostringstream msg;
      if ((*pItr)->size() == 1) {
        // Print out the only entry
        if (tspd)
          msg << tspd->nthValue(0);
        else if (tspi)
          msg << tspi->nthValue(0);
        else if (tspb)
          msg << tspb->nthValue(0);
      } else {
        // Show the # of entries
        msg << "(" << (*pItr)->size() << " entries)";
      }
      treeItem->setText(2, QString::fromStdString(msg.str()));
    } else if (auto strSeries = dynamic_cast<
                   Mantid::Kernel::TimeSeriesProperty<std::string> *>(*pItr)) {
      treeItem->setText(1, "str. series");
      treeItem->setData(1, Qt::UserRole, static_cast<int>(stringTSeries));
      treeItem->setData(0, Qt::UserRole,
                        QString::fromStdString((*pItr)->value()));
      std::ostringstream msg;
      if ((*pItr)->size() == 1) {
        // Print out the only entry
        strSeries->nthValue(1);
      } else {
        // Show the # of entries
        msg << "(" << (*pItr)->size() << " entries)";
      }
      treeItem->setText(2, QString::fromStdString(msg.str()));
    } else if (dynamic_cast<Mantid::Kernel::PropertyWithValue<std::string> *>(
                   *pItr)) {
      treeItem->setText(1, "string");
      treeItem->setData(1, Qt::UserRole, static_cast<int>(string));
      treeItem->setData(0, Qt::UserRole,
                        QString::fromStdString((*pItr)->value()));
      treeItem->setText(2, QString::fromStdString((*pItr)->value()));

    } else if (dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(*pItr) ||
               dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(
                   *pItr)) {
      treeItem->setText(1, "numeric");
      treeItem->setData(
          1, Qt::UserRole,
          static_cast<int>(numeric)); // Save the "role" as numeric.
      treeItem->setData(0, Qt::UserRole,
                        QString::fromStdString((*pItr)->value()));
      treeItem->setText(2, QString::fromStdString((*pItr)->value()));
    } else if (dynamic_cast<Mantid::Kernel::ArrayProperty<int> *>(*pItr) ||
               dynamic_cast<ArrayProperty<double> *>(*pItr) ||
               dynamic_cast<
                   Mantid::Kernel::PropertyWithValue<std::vector<double>> *>(
                   *pItr) ||
               dynamic_cast<
                   Mantid::Kernel::PropertyWithValue<std::vector<int>> *>(
                   *pItr)) {
      treeItem->setText(1, "numeric array");
      treeItem->setData(
          1, Qt::UserRole,
          static_cast<int>(numericArray)); // Save the "role" as numeric array.
      treeItem->setData(0, Qt::UserRole,
                        QString::fromStdString((*pItr)->value()));
      std::ostringstream msg;
      msg << "(" << (*pItr)->size() << " entries)";
      treeItem->setText(2, QString::fromStdString(msg.str()));
    }

    // Add tree item
    m_tree->addTopLevelItem(treeItem);
  }

  // Resize the columns
  m_tree->header()->resizeSection(0, max_length * 10);
  m_tree->header()->resizeSection(1, 100);
  m_tree->header()->resizeSection(2, 170);
  m_tree->header()->resizeSection(3, 90); // units column
  m_tree->header()->setMovable(false);
  m_tree->setSortingEnabled(true);
  m_tree->sortByColumn(0, Qt::AscendingOrder);
}

/** Slot called when selecting a different experiment info number
*
*	@author Martyn Gigg, Tessella Support Services plc
*	@date 05/11/2009
*/
void SampleLogDialogBase::selectExpInfoNumber(int num) {
  m_experimentInfoIndex = size_t(num);
  m_tree->blockSignals(true);
  this->init();
  m_tree->blockSignals(false);
}
