#include "MantidQtMantidWidgets/MuonFitDataSelector.h"

namespace {
Mantid::Kernel::Logger g_log("MuonFitDataSelector");
}

namespace MantidQt {
namespace MantidWidgets {

/**
 * Basic constructor
 * @param parent :: [input] Parent dialog for the widget
 */
MuonFitDataSelector::MuonFitDataSelector(QWidget *parent)
    : MantidWidget(parent) {
  m_ui.setupUi(this);
  this->setUpValidators();
  this->setDefaultValues();
  this->setUpConnections();
  m_groupBoxes.insert("fwd", m_ui.chkFwd);
  m_groupBoxes.insert("bwd", m_ui.chkBwd);
  m_periodBoxes.insert("1", m_ui.chk1);
  m_periodBoxes.insert("2", m_ui.chk2);
}

/**
 * Constructor for the widget
 * @param parent :: [input] Parent dialog for the widget
 * @param runNumber :: [input] Run number of initial workspace
 * @param instName :: [input] Name of instrument from initial workspace
 * @param numPeriods :: [input] Number of periods from initial workspace
 * @param groups :: [input] Group names from initial workspace
 */
MuonFitDataSelector::MuonFitDataSelector(QWidget *parent, int runNumber,
                                         const QString &instName,
                                         size_t numPeriods,
                                         const QStringList &groups)
    : MuonFitDataSelector(parent) {
  this->setWorkspaceDetails(QString::number(runNumber), instName);
  this->setNumPeriods(numPeriods);
  this->setAvailableGroups(groups);
}

/**
 * Connect signals from UI elements to re-emit a signal that "user has changed
 * something"
 */
void MuonFitDataSelector::setUpConnections() {
  connect(m_ui.runs, SIGNAL(filesFound()), this, SLOT(userChangedRuns()));
  connect(m_ui.rbCoAdd, SIGNAL(toggled(bool)), this,
          SLOT(fitTypeChanged(bool)));
  connect(m_ui.txtWSIndex, SIGNAL(editingFinished()), this,
          SIGNAL(dataPropertiesChanged()));
  connect(m_ui.txtStart, SIGNAL(editingFinished()), this,
          SIGNAL(dataPropertiesChanged()));
  connect(m_ui.txtEnd, SIGNAL(editingFinished()), this,
          SIGNAL(dataPropertiesChanged()));
  connect(m_ui.chkCombine, SIGNAL(stateChanged(int)), this,
          SLOT(periodCombinationStateChanged(int)));
  connect(m_ui.chkCombine, SIGNAL(clicked()), this,
          SIGNAL(selectedPeriodsChanged()));
}

/**
 * Called when fit type changed. Emit a signal.
 * @param state :: [input] Unused.
 */
void MuonFitDataSelector::fitTypeChanged(bool state) {
  (void)state;
  emit workspaceChanged();
}

/**
 * Slot: called when user edits runs box.
 * Check for single run and enable/disable radio buttons,
 * and emit signal that runs have changed.
 */
void MuonFitDataSelector::userChangedRuns() {
  // check for single run and enable/disable radio buttons
  const auto runs = getRuns();
  if (runs.contains(',') || runs.contains('-')) {
    // if buttons are disabled, enable them
    m_ui.rbCoAdd->setEnabled(true);
    m_ui.rbSimultaneous->setEnabled(true);
  } else {
    setFitType(FitType::Single);
  }
  emit workspaceChanged();
}

/**
 * Sets group names and updates checkboxes on UI
 * By default sets all checked
 * @param groups :: [input] List of group names
 */
void MuonFitDataSelector::setAvailableGroups(const QStringList &groups) {
  clearGroupCheckboxes();
  for (const auto group : groups) {
    addGroupCheckbox(group);
  }
}

/**
 * Get the user's supplied workspace index (default 0)
 * Returns an unsigned int so it can be put into a QVariant
 * @returns :: Workspace index input by user
 */
unsigned int MuonFitDataSelector::getWorkspaceIndex() const {
  // Validator ensures this can be cast to a positive integer
  const QString index = m_ui.txtWSIndex->text();
  return index.toUInt();
}

/**
 * Set the workspace index in the UI
 * @param index :: [input] Workspace index to set
 */
void MuonFitDataSelector::setWorkspaceIndex(unsigned int index) {
  m_ui.txtWSIndex->setText(QString::number(index));
  emit dataPropertiesChanged();
}

/**
 * Get the user's supplied start time (default 0)
 * @returns :: start time input by user in microseconds
 */
double MuonFitDataSelector::getStartTime() const {
  // Validator ensures cast to double will succeed
  const QString start = m_ui.txtStart->text();
  return start.toDouble();
}

/**
 * Set the start time in the UI WITHOUT sending signal
 * @param start :: [input] Start time in microseconds
 */
void MuonFitDataSelector::setStartTimeQuietly(double start) {
  m_ui.txtStart->setText(QString::number(start));
}

/**
 * Set the start time in the UI, and send signal
 * @param start :: [input] Start time in microseconds
 */
void MuonFitDataSelector::setStartTime(double start) {
  setStartTimeQuietly(start);
  emit dataPropertiesChanged();
}

/**
 * Get the user's supplied end time (default 10)
 * @returns :: start time input by user in microseconds
 */
double MuonFitDataSelector::getEndTime() const {
  // Validator ensures cast to double will succeed
  const QString end = m_ui.txtEnd->text();
  return end.toDouble();
}

/**
 * Set the end time in the UI WITHOUT sending signal
 * @param end :: [input] End time in microseconds
 */
void MuonFitDataSelector::setEndTimeQuietly(double end) {
  m_ui.txtEnd->setText(QString::number(end));
}

/**
 * Set the end time in the UI, and send signal
 * @param end :: [input] End time in microseconds
 */
void MuonFitDataSelector::setEndTime(double end) {
  setEndTimeQuietly(end);
  emit dataPropertiesChanged();
}

/**
 * Get the filenames of the supplied run numbers
 * @returns :: list of run filenames
 */
QStringList MuonFitDataSelector::getFilenames() const {
  return m_ui.runs->getFilenames();
}

/**
 * Set up input validation on UI controls
 * e.g. some boxes should only accept numeric input
 */
void MuonFitDataSelector::setUpValidators() {
  // WS index: non-negative integers only
  m_ui.txtWSIndex->setValidator(new QIntValidator(0, 1000, this));
  // Start/end times: numeric values only
  m_ui.txtStart->setValidator(new QDoubleValidator(this));
  m_ui.txtEnd->setValidator(new QDoubleValidator(this));
}

/**
 * Set up run finder with initial run number and instrument
 * @param runNumbers :: [input] Run numbers from loaded workspace
 * @param instName :: [input] Instrument name from loaded workspace
 */
void MuonFitDataSelector::setWorkspaceDetails(const QString &runNumbers,
                                              const QString &instName) {
  // Set the file finder to the correct instrument (not Mantid's default)
  m_ui.runs->setInstrumentOverride(instName);

  QString runs(runNumbers);
  runs.remove(QRegExp("^[0]*")); // remove leading zeros, if any
  // Set fit type - co-add (as this comes from Home tab) or single
  if (runs.contains('-') || runs.contains(',')) {
    setFitType(FitType::CoAdd);
  } else {
    setFitType(FitType::Single);
  }

  // Set initial run to be run number of the workspace loaded in Home tab
  // and search for filenames. Use busy cursor until search finished.
  setBusyState();
  m_ui.runs->setFileTextWithSearch(runs);
}

/**
 * Set default values in some input controls
 * Defaults copy those previously used in muon fit property browser
 */
void MuonFitDataSelector::setDefaultValues() {
  m_ui.lblStart->setText(QString("Start (%1s)").arg(QChar(0x03BC)));
  m_ui.lblEnd->setText(QString("End (%1s)").arg(QChar(0x03BC)));
  this->setWorkspaceIndex(0);
  this->setStartTime(0.0);
  this->setEndTime(0.0);
  setPeriodCombination(false);
}

/**
 * Set visibility of the "Periods" group box
 * (if single-period, hide to not confuse the user)
 * @param visible :: [input] Whether to show or hide the options
 */
void MuonFitDataSelector::setPeriodVisibility(bool visible) {
  m_ui.groupBoxPeriods->setVisible(visible);
}

/**
 * Add a new checkbox to the list of groups with given name
 * The new checkbox is checked by default
 * @param name :: [input] Name of group to add
 */
void MuonFitDataSelector::addGroupCheckbox(const QString &name) {
  auto checkBox = new QCheckBox(name);
  m_groupBoxes.insert(name, checkBox);
  checkBox->setChecked(true);
  m_ui.verticalLayoutGroups->addWidget(checkBox);
  connect(checkBox, SIGNAL(clicked(bool)), this,
          SIGNAL(selectedGroupsChanged()));
}

/**
 * Clears all group names and checkboxes
 * (ready to add new ones)
 */
void MuonFitDataSelector::clearGroupCheckboxes() {
  for (const auto &checkbox : m_groupBoxes) {
    m_ui.verticalLayoutGroups->removeWidget(checkbox);
    checkbox->deleteLater(); // will disconnect signal automatically
  }
  m_groupBoxes.clear();
}

/**
 * Sets checkboxes on UI for given number
 * of periods plus "combination" boxes.
 * Hides control for single-period data.
 * @param numPeriods :: [input] Number of periods
 */
void MuonFitDataSelector::setNumPeriods(size_t numPeriods) {
  const size_t currentPeriods = static_cast<size_t>(m_periodBoxes.size());
  if (numPeriods > currentPeriods) {
    // create more boxes
    for (size_t i = currentPeriods; i != numPeriods; i++) {
      QString name = QString::number(i + 1);
      auto checkbox = new QCheckBox(name);
      m_periodBoxes.insert(name, checkbox);
      m_ui.verticalLayoutPeriods->addWidget(checkbox);
      connect(checkbox, SIGNAL(clicked()), this,
              SIGNAL(selectedPeriodsChanged()));
    }
  } else if (numPeriods < currentPeriods) {
    // delete the excess
    QStringList toRemove;
    for (const QString name : m_periodBoxes.keys()) {
      const size_t periodNum = static_cast<size_t>(name.toInt());
      if (periodNum > numPeriods) {
        m_ui.verticalLayoutPeriods->removeWidget(m_periodBoxes.value(name));
        m_periodBoxes.value(name)->deleteLater(); // will disconnect signal
        toRemove.append(name);
      }
    }
    for (const QString name : toRemove) {
      m_periodBoxes.remove(name);
    }
  }
  // Always put the combination at the bottom ("-1" = at end)
  m_ui.verticalLayoutPeriods->removeItem(m_ui.horizontalLayoutPeriodsCombine);
  m_ui.verticalLayoutPeriods->insertLayout(-1,
                                           m_ui.horizontalLayoutPeriodsCombine);

  // Hide box if single-period
  this->setPeriodVisibility(numPeriods > 1);
}

/**
 * Returns a list of periods and combinations chosen in UI
 * @returns :: list of periods e.g. "1", "3", "1+2-3+4"
 */
QStringList MuonFitDataSelector::getPeriodSelections() const {
  QStringList checked;
  for (auto iter = m_periodBoxes.constBegin(); iter != m_periodBoxes.constEnd();
       ++iter) {
    if (iter.value()->isChecked()) {
      checked.append(iter.key());
    }
  }

  // combination
  if (m_ui.chkCombine->isChecked()) {
    QString combination = m_ui.txtFirst->text();
    combination.append("-").append(m_ui.txtSecond->text());
    combination.replace(" ", "");
    combination.replace(",", "+");
    checked.append(combination);
  }

  return checked;
}

/**
 * Returns a list of the selected groups (checked boxes)
 * @returns :: list of selected groups
 */
QStringList MuonFitDataSelector::getChosenGroups() const {
  QStringList chosen;
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    if (iter.value()->isChecked()) {
      chosen.append(iter.key());
    }
  }
  return chosen;
}

/**
 * Set the chosen group ticked and all others off
 * Used when switching from Home tab to Data Analysis tab
 * @param group :: [input] Name of group to select
 */
void MuonFitDataSelector::setChosenGroup(const QString &group) {
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    iter.value()->setChecked(iter.key() == group);
  }
}

/**
 * Set the chosen period/combination ticked and all others off
 * Used when switching from Home tab to Data Analysis tab
 * @param period :: [input] Period string to set selected
 * (can be just one period or a combination)
 */
void MuonFitDataSelector::setChosenPeriod(const QString &period) {
  // Begin by unchecking everything
  for (auto checkbox : m_periodBoxes) {
    checkbox->setChecked(false);
  }

  // If single-period or all periods, string will be empty
  if (period.isEmpty()) {
    if (m_periodBoxes.size() == 1) { // single-period
      setPeriodCombination(false);
      m_periodBoxes.begin().value()->setChecked(true);
    } else { // all periods selected
      setPeriodCombination(true);
      QString combination;
      for (int i = 0; i < m_periodBoxes.count() - 1; i++) {
        combination.append(QString::number(i + 1)).append(", ");
      }
      m_ui.txtFirst->setText(
          combination.append(QString::number(m_periodBoxes.count())));
      m_ui.txtSecond->clear();
    }
  } else {
    bool onePeriod(false);
    const int chosenPeriod = period.toInt(&onePeriod);
    if (onePeriod) {
      // set just one
      for (auto iter = m_periodBoxes.constBegin();
           iter != m_periodBoxes.constEnd(); ++iter) {
        if (iter.key() == period) {
          iter.value()->setChecked(true);
        }
      }
      setPeriodCombination(false);
    } else {
      // set the combination
      setPeriodCombination(true);
      QStringList parts = period.split('-');
      if (parts.size() == 2) {
        m_ui.txtFirst->setText(parts[0].replace("+", ", "));
        m_ui.txtSecond->setText(parts[1].replace("+", ", "));
      }
    }
  }
}

/**
*Gets user input in the form of a QVariant
*
*This is implemented as the "standard" way of getting input from a
*MantidWidget. In practice it is probably easier to get the input
*using other methods.
*
*The returned QVariant is a QVariantMap of (parameter, value) pairs.
*@returns :: QVariant containing a QVariantMap
*/
QVariant MuonFitDataSelector::getUserInput() const {
  QVariant ret(QVariant::Map);
  auto map = ret.toMap();
  map.insert("Workspace index", getWorkspaceIndex());
  map.insert("Start", getStartTime());
  map.insert("End", getEndTime());
  map.insert("Runs", getRuns());
  map.insert("Groups", getChosenGroups());
  map.insert("Periods", getPeriodSelections());
  return map;
}

/**
 * Sets user input in the form of a QVariant
 *
 * This is implemented as the "standard" way of setting input in a
 * MantidWidget. In practice it is probably easier to set the input
 * using other methods.
 *
 * This function doesn't support setting runs, groups or periods.
 *
 * The input QVariant is a QVariantMap of (parameter, value) pairs.
 * @param value :: [input] QVariant containing a QVariantMap
 */
void MuonFitDataSelector::setUserInput(const QVariant &value) {
  if (value.canConvert(QVariant::Map)) {
    const auto map = value.toMap();
    if (map.contains("Workspace index")) {
      setWorkspaceIndex(map.value("Workspace index").toUInt());
    }
    if (map.contains("Start")) {
      setStartTime(map.value("Start").toDouble());
    }
    if (map.contains("End")) {
      setEndTime(map.value("End").toDouble());
    }
  }
}

/**
 * Returns the selected fit type.
 * - If only one run is selected, this is a single fit.
 * - If multiple runs are selected, the user has the option of co-adding them or
 * doing a simultaneous fit, chosen via the radio buttons.
 * @returns :: fit type from enum
 */
IMuonFitDataSelector::FitType MuonFitDataSelector::getFitType() const {
  // If radio buttons disabled, it's a single fit
  if (!m_ui.rbCoAdd->isEnabled()) {
    return FitType::Single;
  } else {
    // which button is selected
    if (m_ui.rbCoAdd->isChecked()) {
      return FitType::CoAdd;
    } else {
      return FitType::Simultaneous;
    }
  }
}

/**
 * Sets the fit type.
 * If single, disables radio buttons.
 * Otherwise, enables buttons and selects the correct one.
 * @param type :: [input] Fit type to set (from enum)
 */
void MuonFitDataSelector::setFitType(IMuonFitDataSelector::FitType type) {
  if (type == FitType::Single) {
    m_ui.rbCoAdd->setEnabled(false);
    m_ui.rbSimultaneous->setEnabled(false);
  } else {
    m_ui.rbCoAdd->setEnabled(true);
    m_ui.rbSimultaneous->setEnabled(true);
    m_ui.rbCoAdd->setChecked(type == FitType::CoAdd);
    m_ui.rbSimultaneous->setChecked(type == FitType::Simultaneous);
  }
}

/**
 * Check/uncheck period combination checkbox and set the textboxes
 * enabled/disabled
 * @param on :: [input] Turn on or off
 */
void MuonFitDataSelector::setPeriodCombination(bool on) {
  m_ui.chkCombine->setChecked(on);
  m_ui.txtFirst->setEnabled(on);
  m_ui.txtSecond->setEnabled(on);
}

/**
 * Slot: Keeps enabled/disabled state of textboxes in sync with checkbox
 * for period combination choices
 * @param state :: [input] New check state of box
 */
void MuonFitDataSelector::periodCombinationStateChanged(int state) {
  m_ui.txtFirst->setEnabled(state == Qt::Checked);
  m_ui.txtSecond->setEnabled(state == Qt::Checked);
}

/**
 * Return the instrument name currently set as the override
 * for the data selector
 * @returns :: instrument name
 */
QString MuonFitDataSelector::getInstrumentName() const {
  return m_ui.runs->getInstrumentOverride();
}

/**
 * Return the runs entered by the user
 * @returns :: run number string if valid, else empty string
 */
QString MuonFitDataSelector::getRuns() const {
  if (m_ui.runs->isValid()) {
    return m_ui.runs->getText();
  } else {
    return "";
  }
}

/**
 * Slot: called when file finding finished. Resets the cursor for this widget
 * back to the normal, non-busy state.
 */
void MuonFitDataSelector::unsetBusyState() {
  disconnect(m_ui.runs, SIGNAL(fileInspectionFinished()), this,
             SLOT(unsetBusyState()));
  this->setCursor(Qt::ArrowCursor);
  m_ui.groupBoxDataSelector->setEnabled(true);
  m_ui.groupBoxGroups->setEnabled(true);
  if (m_ui.groupBoxPeriods->isVisible()) {
    m_ui.groupBoxPeriods->setEnabled(true);
  }
}

/**
 * Sets busy cursor and disables input while file search in progress.
 * Connects up slot to reset busy state when search done.
 */
void MuonFitDataSelector::setBusyState() {
  connect(m_ui.runs, SIGNAL(fileInspectionFinished()), this,
          SLOT(unsetBusyState()));
  this->setCursor(Qt::WaitCursor);
  m_ui.groupBoxDataSelector->setEnabled(false);
  m_ui.groupBoxGroups->setEnabled(false);
  if (m_ui.groupBoxPeriods->isVisible()) {
    m_ui.groupBoxPeriods->setEnabled(false);
  }
}

} // namespace MantidWidgets
} // namespace MantidQt