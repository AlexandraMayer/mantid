#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffFittingViewQtWidget.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtMantidWidgets/PeakPicker.h"

#include <fstream>
#include <iomanip>

#include <boost/algorithm/string.hpp>

#include <Poco/Path.h>

#include <QFileDialog>
#include <QSettings>

#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>
#include <qwt_symbol.h>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

std::vector<std::string> EnggDiffFittingViewQtWidget::m_fitting_runno_dir_vec;
bool EnggDiffFittingViewQtWidget::m_fittingMutliRunMode = false;

EnggDiffFittingViewQtWidget::EnggDiffFittingViewQtWidget()
    : IEnggDiffFittingView(), m_fittedDataVector(), m_peakPicker(nullptr),
      m_zoomTool(nullptr), m_presenter(nullptr) {
  initLayout();
}

EnggDiffFittingViewQtWidget::~EnggDiffFittingViewQtWidget() {
  for (auto curves : m_focusedDataVector) {
    curves->detach();
    delete curves;
  }

  for (auto curves : m_fittedDataVector) {
    curves->detach();
    delete curves;
  }
}

EnggDiffFittingViewQtWidget::initLayout() {
  m_ui.setupUi(this);

  readSettings();
  doSetup();

  m_presenter.reset(new EnggDiffFittingPresenter(this));
  m_presenter->notify(IEnggDiffFittingPresenter::Start);
}

void EnggDiffFittingViewQtWidget::doSetup() {
  connect(m_ui.pushButton_fitting_browse_run_num, SIGNAL(released()), this,
          SLOT(browseFitFocusedRun()));

  connect(m_ui.lineEdit_pushButton_run_num, SIGNAL(textEdited(const QString &)),
          this, SLOT(resetFittingMultiMode()));

  connect(m_ui.lineEdit_pushButton_run_num, SIGNAL(editingFinished()), this,
          SLOT(FittingRunNo()));

  connect(m_ui.lineEdit_pushButton_run_num, SIGNAL(returnPressed()), this,
          SLOT(FittingRunNo()));

  connect(this, SIGNAL(getBanks()), this, SLOT(FittingRunNo()));

  connect(this, SIGNAL(setBank()), this, SLOT(listViewFittingRun()));

  connect(m_ui.listWidget_fitting_run_num, SIGNAL(itemSelectionChanged()), this,
          SLOT(listViewFittingRun()));

  connect(m_ui.comboBox_bank, SIGNAL(currentIndexChanged(int)), this,
          SLOT(setBankDir(int)));

  connect(m_ui.pushButton_fitting_browse_peaks, SIGNAL(released()), this,
          SLOT(browsePeaksToFit()));

  connect(m_ui.pushButton_fit, SIGNAL(released()), this, SLOT(fitClicked()));

  // add peak by clicking the button
  connect(m_ui.pushButton_select_peak, SIGNAL(released()), SLOT(setPeakPick()));

  connect(m_ui.pushButton_add_peak, SIGNAL(released()), SLOT(addPeakToList()));

  connect(m_ui.pushButton_save_peak_list, SIGNAL(released()),
          SLOT(savePeakList()));

  connect(m_ui.pushButton_clear_peak_list, SIGNAL(released()),
          SLOT(clearPeakList()));

  connect(m_ui.pushButton_plot_separate_window, SIGNAL(released()),
          SLOT(plotSeparateWindow()));

  m_ui.dataPlot->setCanvasBackground(Qt::white);
  m_ui.dataPlot->setAxisTitle(QwtPlot::xBottom, "d-Spacing (A)");
  m_ui.dataPlot->setAxisTitle(QwtPlot::yLeft, "Counts (us)^-1");
  QFont font("MS Shell Dlg 2", 8);
  m_ui.dataPlot->setAxisFont(QwtPlot::xBottom, font);
  m_ui.dataPlot->setAxisFont(QwtPlot::yLeft, font);

  // constructor of the peakPicker
  // XXX: Being a QwtPlotItem, should get deleted when m_ui.plot gets deleted
  // (auto-delete option)
  m_peakPicker = new MantidWidgets::PeakPicker(m_ui.dataPlot, Qt::red);
  setPeakPickerEnabled(false);

  m_zoomTool =
      new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
                        QwtPicker::DragSelection | QwtPicker::CornerToCorner,
                        QwtPicker::AlwaysOff, m_ui.dataPlot->canvas());
  m_zoomTool->setRubberBandPen(QPen(Qt::black));
  setZoomTool(false);
}

void EnggDiffFittingViewQtWidget::readSettings() {
  // user params
  m_ui.lineEdit_pushButton_run_num->setText(
      qs.value("user-params-fitting-focused-file", "").toString());
  m_ui.comboBox_bank->setCurrentIndex(0);
  m_ui.lineEdit_fitting_peaks->setText(
      qs.value("user-params-fitting-peaks-to-fit", "").toString());
}

void EnggDiffFittingViewQtWidget::saveSettings() {
  qs.setValue("user-params-fitting-focused-file",
              m_ui.lineEdit_pushButton_run_num->text());
  qs.setValue("user-params-fitting-peaks-to-fit",
              m_ui.lineEdit_fitting_peaks->text());
}

void EnggDiffFittingViewQtWidget::enable(bool enable) {
  m_ui.pushButton_fitting_browse_run_num->setEnabled(enable);
  m_ui.lineEdit_pushButton_run_num->setEnabled(enable);
  m_ui.pushButton_fitting_browse_peaks->setEnabled(enable);
  m_ui.lineEdit_fitting_peaks->setEnabled(enable);
  m_ui.pushButton_fit->setEnabled(enable);
  m_ui.pushButton_clear_peak_list->setEnabled(enable);
  m_ui.pushButton_save_peak_list->setEnabled(enable);
  m_ui.comboBox_bank->setEnabled(enable);
  m_ui.groupBox_fititng_preview->setEnabled(enable);
}

void EnggDiffractionViewQtGUI::fitClicked() {
  m_presenter->notify(IEnggDiffFittingPresenter::FitPeaks);
}

void EnggDiffractionViewQtGUI::FittingRunNo() {
  m_presenter->notify(IEnggDiffFittingPresenter::FittingRunNo);
}

void EnggDiffFittingViewQtWidget::setBankDir(int idx) {

  if (m_fitting_runno_dir_vec.size() >= size_t(idx)) {

    std::string bankDir = m_fitting_runno_dir_vec[idx];
    Poco::Path fpath(bankDir);

    setFittingRunNo(bankDir);
  }
}

void EnggDiffFittingViewQtWidget::listViewFittingRun() {

  if (m_fittingMutliRunMode) {
    auto listView = m_ui.listWidget_fitting_run_num;
    auto currentRow = listView->currentRow();
    auto item = listView->item(currentRow);
    QString itemText = item->text();

    setFittingRunNo(itemText.toStdString());
    FittingRunNo();
  }
}

void EnggDiffFittingViewQtWidget::resetFittingMultiMode() {
  // resets the global variable so the list view widgets
  // adds the run number to for single runs too
  m_fittingMutliRunMode = false;
}

std::string EnggDiffFittingViewQtWidget::fittingRunNoFactory(
    std::string bank, std::string fileName, std::string &bankDir,
    std::string fileDir) {

  std::string genDir = fileName.substr(0, fileName.size() - 1);
  Poco::Path bankFile(genDir + bank + ".nxs");
  if (bankFile.isFile()) {
    bankDir = fileDir + genDir + bank + ".nxs";
  }
  return bankDir;
}

std::string EnggDiffFittingViewQtWidget::readPeaksFile(std::string fileDir) {
  std::string fileData = "";
  std::string line;
  std::string comma = ", ";

  std::ifstream peakFile(fileDir);

  if (peakFile.is_open()) {
    while (std::getline(peakFile, line)) {
      fileData += line;
      if (!peakFile.eof())
        fileData += comma;
    }
    peakFile.close();
  }

  else
    fileData = "";

  return fileData;
}

void EnggDiffFittingViewQtWidget::setDataVector(
    std::vector<boost::shared_ptr<QwtData>> &data, bool focused,
    bool plotSinglePeaks) {

  if (!plotSinglePeaks) {
    // clear vector and detach curves to avoid plot crash
    // when only plotting focused workspace
    for (auto curves : m_fittedDataVector) {
      if (curves) {
        curves->detach();
        delete curves;
      }
    }

    if (m_fittedDataVector.size() > 0)
      m_fittedDataVector.clear();

    // set it as false as there will be no valid workspace to plot
    m_ui.pushButton_plot_separate_window->setEnabled(false);
  }

  if (focused) {
    dataCurvesFactory(data, m_focusedDataVector, focused);
  } else {
    dataCurvesFactory(data, m_fittedDataVector, focused);
  }
}

void EnggDiffFittingViewQtWidget::dataCurvesFactory(
    std::vector<boost::shared_ptr<QwtData>> &data,
    std::vector<QwtPlotCurve *> &dataVector, bool focused) {

  // clear vector
  for (auto curves : dataVector) {
    if (curves) {
      curves->detach();
      delete curves;
    }
  }

  if (dataVector.size() > 0)
    dataVector.clear();
  resetView();

  // dark colours could be removed so that the coloured peaks stand out more
  const std::array<QColor, 16> QPenList{
      {Qt::white, Qt::red, Qt::darkRed, Qt::green, Qt::darkGreen, Qt::blue,
       Qt::darkBlue, Qt::cyan, Qt::darkCyan, Qt::magenta, Qt::darkMagenta,
       Qt::yellow, Qt::darkYellow, Qt::gray, Qt::lightGray, Qt::black}};

  std::mt19937 gen;
  std::uniform_int_distribution<std::size_t> dis(0, QPenList.size() - 1);

  for (size_t i = 0; i < data.size(); i++) {
    auto *peak = data[i].get();

    QwtPlotCurve *dataCurve = new QwtPlotCurve();
    if (!focused) {
      dataCurve->setStyle(QwtPlotCurve::Lines);
      auto randIndex = dis(gen);
      dataCurve->setPen(QPen(QPenList[randIndex], 2));

      // only set enabled when single peak workspace plotted
      m_ui.pushButton_plot_separate_window->setEnabled(true);
    } else {
      dataCurve->setStyle(QwtPlotCurve::NoCurve);
      // focused workspace in bg set as darkGrey crosses insted of line
      dataCurve->setSymbol(QwtSymbol(QwtSymbol::XCross, QBrush(),
                                     QPen(Qt::darkGray, 1), QSize(3, 3)));
    }
    dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    dataVector.push_back(dataCurve);

    dataVector[i]->setData(*peak);
    dataVector[i]->attach(m_ui.dataPlot);
  }

  m_ui.dataPlot->replot();
  m_zoomTool->setZoomBase();
  // enable zoom & select peak btn after the plotting on graph
  setZoomTool(true);
  m_ui.pushButton_select_peak->setEnabled(true);
  data.clear();
}

void EnggDiffFittingViewQtWidget::setPeakPickerEnabled(bool enabled) {
  m_peakPicker->setEnabled(enabled);
  m_peakPicker->setVisible(enabled);
  m_ui.dataPlot->replot(); // PeakPicker might get hidden/shown
  m_ui.pushButton_add_peak->setEnabled(enabled);
  if (enabled) {
    QString btnText = "Reset Peak Selector";
    m_ui.pushButton_select_peak->setText(btnText);
  }
}

void EnggDiffFittingViewQtWidget::setPeakPicker(
    const IPeakFunction_const_sptr &peak) {
  m_peakPicker->setPeak(peak);
  m_ui.dataPlot->replot();
}

double EnggDiffFittingViewQtWidget::getPeakCentre() const {
  auto peak = m_peakPicker->peak();
  auto centre = peak->centre();
  return centre;
}

void EnggDiffFittingViewQtWidget::fittingWriteFile(const std::string &fileDir) {
  std::ofstream outfile(fileDir.c_str());
  if (!outfile) {
    userWarning("File not found",
                "File " + fileDir + " , could not be found. Please try again!");
  } else {
    auto expPeaks = m_ui.lineEdit_fitting_peaks->text();
    outfile << expPeaks.toStdString();
  }
}

void EnggDiffFittingViewQtWidget::setZoomTool(bool enabled) {
  m_zoomTool->setEnabled(enabled);
}

void EnggDiffFittingViewQtWidget::resetView() {
  // Resets the view to a sensible default
  // Auto scale the axis
  m_ui.dataPlot->setAxisAutoScale(QwtPlot::xBottom);
  m_ui.dataPlot->setAxisAutoScale(QwtPlot::yLeft);

  // Set this as the default zoom level
  m_zoomTool->setZoomBase(true);
}

void EnggDiffFittingViewQtWidget::browsePeaksToFit() {

  try {
    QString prevPath = QString::fromStdString(m_focusDir);
    if (prevPath.isEmpty()) {
      prevPath = MantidQt::API::AlgorithmInputHistory::Instance()
                     .getPreviousDirectory();
    }

    QString path(
        QFileDialog::getOpenFileName(this, tr("Open Peaks To Fit"), prevPath,
                                     QString::fromStdString(g_DetGrpExtStr)));

    if (path.isEmpty()) {
      return;
    }

    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);

    std::string peaksData = readPeaksFile(path.toStdString());

    m_ui.lineEdit_fitting_peaks->setText(QString::fromStdString(peaksData));
  } catch (...) {
    userWarning("Unable to import the peaks from a file: ",
                "File corrupted or could not be opened. Please try again");
    return;
  }
}

void EnggDiffractionViewQtGUI::browseFitFocusedRun() {
  resetFittingMultiMode();
  QString prevPath = QString::fromStdString(m_focusDir);
  if (prevPath.isEmpty()) {
    prevPath =
        MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  }
  std::string nexusFormat = "Nexus file with calibration table: NXS, NEXUS"
                            "(*.nxs *.nexus);;";

  QString path(
      QFileDialog::getOpenFileName(this, tr("Open Focused File "), prevPath,
                                   QString::fromStdString(nexusFormat)));

  if (path.isEmpty()) {
    return;
  }

  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(path);
  setFittingRunNo(path.toStdString());
  getBanks();
}

void EnggDiffFittingViewQtWidget::setFittingRunNo(const std::string &path) {
  m_ui.lineEdit_pushButton_run_num->setText(QString::fromStdString(path));
}

std::string EnggDiffFittingViewQtWidget::getFittingRunNo() const {
  return m_ui.lineEdit_pushButton_run_num->text().toStdString();
}

void EnggDiffFittingViewQtWidget::clearFittingComboBox() const {
  m_ui.comboBox_bank->clear();
}

void EnggDiffFittingViewQtWidget::enableFittingComboBox(bool enable) const {
  m_ui.comboBox_bank->setEnabled(enable);
}

void EnggDiffFittingViewQtWidget::clearFittingListWidget() const {
  m_ui.listWidget_fitting_run_num->clear();
}

void EnggDiffFittingViewQtWidget::enableFittingListWidget(bool enable) const {
  m_ui.listWidget_fitting_run_num->setEnabled(enable);
}

int EnggDiffFittingViewQtWidget::getFittingListWidgetCurrentRow() const {
  return m_ui.listWidget_fitting_run_num->currentRow();
}

void EnggDiffFittingViewQtWidget::setFittingListWidgetCurrentRow(
    int idx) const {
  m_ui.listWidget_fitting_run_num->setCurrentRow(idx);
}

int EnggDiffFittingViewQtWidget::getFittingComboIdx(std::string bank) const {
  return m_ui.comboBox_bank->findText(QString::fromStdString(bank));
}

void EnggDiffFittingViewQtWidget::plotSeparateWindow() {
  std::string pyCode =

      "fitting_single_peaks_twin_ws = \"__engggui_fitting_single_peaks_twin\"\n"
      "if (mtd.doesExist(fitting_single_peaks_twin_ws)):\n"
      " DeleteWorkspace(fitting_single_peaks_twin_ws)\n"

      "single_peak_ws = CloneWorkspace(InputWorkspace = "
      "\"engggui_fitting_single_peaks\", OutputWorkspace = "
      "fitting_single_peaks_twin_ws)\n"
      "tot_spec = single_peak_ws.getNumberHistograms()\n"

      "spec_list = []\n"
      "for i in range(0, tot_spec):\n"
      " spec_list.append(i)\n"

      "fitting_plot = plotSpectrum(single_peak_ws, spec_list).activeLayer()\n"
      "fitting_plot.setTitle(\"Engg GUI Single Peaks Fitting Workspace\")\n";

  std::string status =
      runPythonCode(QString::fromStdString(pyCode), false).toStdString();
  m_logMsgs.emplace_back("Plotted output focused data, with status string " +
                         status);
  m_presenter->notify(IEnggDiffFittingPresenter::LogMsg);
}

std::string EnggDiffFittingViewQtWidget::fittingPeaksData() const {

  return m_ui.lineEdit_fitting_peaks->text().toStdString();
}

void EnggDiffFittingViewQtWidget::setPeakList(std::string peakList) const {
  m_ui.lineEdit_fitting_peaks->setText(QString::fromStdString(peakList));
}

std::vector<std::string>
EnggDiffFittingViewQtWidget::splitFittingDirectory(std::string &selectedfPath) {

  Poco::Path PocofPath(selectedfPath);
  std::string selectedbankfName = PocofPath.getBaseName();
  std::vector<std::string> splitBaseName;
  if (selectedbankfName.find("ENGINX_") != std::string::npos) {
    boost::split(splitBaseName, selectedbankfName, boost::is_any_of("_."));
  }
  return splitBaseName;
}

void EnggDiffFittingViewQtWidget::setBankEmit() { emit setBank(); }

void EnggDiffFittingViewQtWidget::setBankIdComboBox(int idx) {
  QComboBox *bankName = m_ui.comboBox_bank;
  bankName->setCurrentIndex(idx);
}

std::string EnggDiffFittingViewQtWidget::getFocusDir() { return m_focusDir; }

void EnggDiffFittingViewQtWidget::addBankItem(std::string bankID) {

  m_ui.comboBox_bank->addItem(QString::fromStdString(bankID));
}

void EnggDiffFittingViewQtWidget::addRunNoItem(std::string runNo) {
  m_ui.listWidget_fitting_run_num->addItem(QString::fromStdString(runNo));
}

std::vector<std::string> EnggDiffFittingViewQtWidget::getFittingRunNumVec() {
  return m_fitting_runno_dir_vec;
}

void EnggDiffFittingViewQtWidget::setFittingRunNumVec(
    std::vector<std::string> assignVec) {
  m_fitting_runno_dir_vec.clear();
  m_fitting_runno_dir_vec = assignVec;
}

void EnggDiffFittingViewQtWidget::setFittingMultiRunMode(bool mode) {
  m_fittingMutliRunMode = mode;
}

bool EnggDiffFittingViewQtWidget::getFittingMultiRunMode() {
  return m_fittingMutliRunMode;
}

void EnggDiffFittingViewQtWidget::setPeakPick() {
  auto bk2bk =
      FunctionFactory::Instance().createFunction("BackToBackExponential");
  auto bk2bkFunc = boost::dynamic_pointer_cast<IPeakFunction>(bk2bk);
  // set the peak to BackToBackExponential function
  setPeakPicker(bk2bkFunc);
  setPeakPickerEnabled(true);
}

void EnggDiffFittingViewQtWidget::addPeakToList() {

  if (m_peakPicker->isEnabled()) {
    auto peakCentre = getPeakCentre();

    std::stringstream stream;
    stream << std::fixed << std::setprecision(4) << peakCentre;
    auto strPeakCentre = stream.str();

    auto curExpPeaksList = m_ui.lineEdit_fitting_peaks->text();
    QString comma = ",";

    if (!curExpPeaksList.isEmpty()) {
      // when further peak added to list
      std::string expPeakStr = curExpPeaksList.toStdString();
      std::string lastTwoChr = expPeakStr.substr(expPeakStr.size() - 2);
      auto lastChr = expPeakStr.back();
      if (lastChr == ',' || lastTwoChr == ", ") {
        curExpPeaksList.append(QString::fromStdString(strPeakCentre));
      } else {
        curExpPeaksList.append(comma + QString::fromStdString(strPeakCentre));
      }
      m_ui.lineEdit_fitting_peaks->setText(curExpPeaksList);
    } else {
      // when new peak given when list is empty
      curExpPeaksList.append(QString::fromStdString(strPeakCentre));
      curExpPeaksList.append(comma);
      m_ui.lineEdit_fitting_peaks->setText(curExpPeaksList);
    }
  }
}

void EnggDiffFittingViewQtWidget::savePeakList() {
  // call function in EnggPresenter..

  try {
    QString prevPath = QString::fromStdString(m_focusDir);
    if (prevPath.isEmpty()) {
      prevPath = MantidQt::API::AlgorithmInputHistory::Instance()
                     .getPreviousDirectory();
    }

    QString path(QFileDialog::getSaveFileName(
        this, tr("Save Expected Peaks List"), prevPath,
        QString::fromStdString(g_DetGrpExtStr)));

    if (path.isEmpty()) {
      return;
    }
    const std::string strPath = path.toStdString();
    fittingWriteFile(strPath);
  } catch (...) {
    userWarning("Unable to save the peaks file: ",
                "Invalid file path or or could not be saved. Please try again");
    return;
  }
}

void EnggDiffFittingViewQtWidget::clearPeakList() {
  m_ui.lineEdit_fitting_peaks->clear();
}

} // namespace CustomInterfaces
} // namespace MantidQt
