import sys
from PyQt4.QtGui import *
from PyQt4.QtCore import *

from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.dns.dns_reduction import DNSScriptElement


class DNSSetupWidget(BaseWidget):
    
    name = 'DNS Reduction'

    class DataTable(QAbstractTableModel):

        def __init__(self, parent):
            QAbstractTableModel.__init__(self, parent)
            self.tableData = []

        def _getRowNumbers(self):
            return len(self.tableData)

        def _getRow(self, row):
            return self.tableData[row] if row < self._getRowNumbers() else ('', '')

        def _isRowEmpty(self, row):
            (runs, comment) = self._getRow(row)
            return not str(runs).strip() and not str(comment).strip()

        def _removeTrailingEmptyRows(self):
            for row in reversed(range(self._getRowNumbers())):
                if self._isRowEmpty(row):
                    del self.tableData[row]
                else:
                    break

        def _setNumRows(self, numRows):
            while self._getRowNumbers() < numRows:
                self.tableData.append(('',''))

        def _setCell(self, row, col, text):
            self._setNumRows(row + 1)
            (runNumbers, comment) = self.tableData[row]

            text = str(text).strip()
            if col == 0:
                runNumbers = text
            else:
                comment = text

            self.tableData[row] = (runNumbers, comment)

        def _getCell(self, row, col):
            return str(self._getRow(row)[col]).strip()


        headers = ('Run numbers', 'Comment')
        selectCell = pyqtSignal(int, int)

        def rowCount(self, _ = QModelIndex()):
            return self._getRowNumbers() + 1

        def columnCount(self, _ = QModelIndex()):
            return 2

        def headerData(self, selction, orientation, role):
            if Qt.Horizontal == orientation and Qt.DisplayRole == role:
                return self.headers[selction]

            return None

        def data(self, index, role):
            if Qt.DisplayRole == role or Qt.EditRole == role:
                return self._getCell(index.row(), index.column())

            return None

        def setData(self, index, text, _):
            row = index.row()
            col = index.column()

            self._setCell(row, col, text)
            self._removeTrailingEmptyRows()

            self.reset()

            col = col + 1

            if col >= 2:
                row = row + 1
                col = 0

            row = min(row, self.rowCount() - 1)
            self.selectCell.emit(row, col)

            return True

        def flags(self, _):
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable

    #TIP_currentTable         = ''
    #TIP_btnCurrentTable      = ''
    TIP_twoTheta             = ''
    TIP_rbnNormaliseDuration = ''
    TIP_rbnNormaliseMonitor  = ''
    TIP_neutronWaveLen       = ''

    TIP_maskMinAngle = ''
    TIP_maskMaxAngle = ''

    TIP_outDir         = ''
    TIP_btnOutDir      = ''
    TIP_outFile        = ''
    TIP_rbnOutSoftMag  = ''
    TIP_rbnOutSingle   = ''
    TIP_chkOutAxQ      = ''
    TIP_chkOutAxD      = ''
    TIP_chkOutAx2Theta = ''
    TIP_omegaOffset    = ''
    TIP_latticeA       = ''
    TIP_latticeB       = ''
    TIP_latticeC       = ''
    TIP_latticeAlpha   = ''
    TIP_latticeBeta    = ''
    TIP_latticeGamma   = ''
    TIP_scatterU1      = ''
    TIP_scatterU2      = ''
    TIP_scatterU3      = ''
    TIP_scatterV1      = ''
    TIP_scatterV2      = ''
    TIP_scatterV3      = ''

    TIP_standardDataPath    = ''
    TIP_btnStandardDataPath = ''
    #TIP_vanSuff             = ''
    #TIP_NiCrSuff            = ''
    #TIP_backSuff            = ''

    TIP_chkDetEffi          = ''
    TIP_chkSumVan           = ''
    TIP_chkSubInst          = ''
    TIP_chkFlippRatio       = ''
    TIP_multiSF             = ''
    TIP_chkKeepIntermadiate = ''

    TIP_sampleDataPath    = ''
    TIP_btnSampleDataPath = ''
    TIP_sampleFilePre     = ''
    TIP_sampleFileSuff    = ''
    TIP_runsView          = ''


    def __init__(self, settings):

        BaseWidget.__init__(self, settings = settings)

        inf = float('inf')

        def tip(widget, text):
            if text:
                widget.setToolTip(text)
            return widget

        def set_spin(spin, minVal = -inf, maxVal= +inf):
            spin.setRange(minVal, maxVal)
            spin.setDecimals(2)
            spin.setSingleStep(0.01)

        def set_spinLattice(spin, minVal = -inf, maxVal= +inf):
            spin.setRange(minVal, maxVal)
            spin.setDecimals(4)
            spin.setSingleStep(0.01)

        #self.currTable            = tip(QLineEdit(), self.TIP_currentTable)
        #self.btnCurrTable         = tip(QPushButton('Browse'), self.TIP_btnCurrentTable)
        self.twoTheta             = tip(QDoubleSpinBox(), self.TIP_twoTheta)
        self.rbnNormaliseDuration = tip(QRadioButton('duration'), self.TIP_rbnNormaliseDuration)
        self.rbnNormaliseMonitor  = tip(QRadioButton('monitor'), self.TIP_rbnNormaliseMonitor)
        self.neutronWaveLength    = tip(QDoubleSpinBox(), self.TIP_neutronWaveLen)

        set_spin(self.twoTheta,          0.0, 0.1)
        set_spin(self.neutronWaveLength, 0.0)


        self.maskMinAngle = tip(QDoubleSpinBox(), self.TIP_maskMinAngle)
        self.maskMaxAngle = tip(QDoubleSpinBox(), self.TIP_maskMinAngle)

        set_spin(self.maskMinAngle)
        set_spin(self.maskMaxAngle)

        self.outDir         = tip(QLineEdit(), self.TIP_outDir)
        self.btnOutDir      = tip(QPushButton('Browse'), self.TIP_btnOutDir)
        self.outFile        = tip(QLineEdit(), self.TIP_outFile)
        self.rbnOutSoftMag  = tip(QRadioButton('Soft matter/Magnetic powder'), self.TIP_rbnOutSoftMag)
        self.chkOutAxQ      = tip(QCheckBox('q'), self.TIP_chkOutAxQ)
        self.chkOutAxD      = tip(QCheckBox('d'), self.TIP_chkOutAxD)
        self.chkOutAx2Theta = tip(QCheckBox(u'2\u0398'), self.TIP_chkOutAx2Theta)
        self.rbnOutSingle   = tip(QRadioButton('Single Crystal'), self.TIP_rbnOutSingle)
        self.omegaOffset    = tip(QDoubleSpinBox(), self.TIP_omegaOffset)
        self.latticeA       = tip(QDoubleSpinBox(), self.TIP_latticeA)
        self.latticeB       = tip(QDoubleSpinBox(), self.TIP_latticeB)
        self.latticeC       = tip(QDoubleSpinBox(), self.TIP_latticeC)
        self.latticeAlpha   = tip(QDoubleSpinBox(), self.TIP_latticeAlpha)
        self.latticeBeta    = tip(QDoubleSpinBox(), self.TIP_latticeBeta)
        self.latticeGamma   = tip(QDoubleSpinBox(), self.TIP_latticeGamma)
        self.scatterU1      = tip(QDoubleSpinBox(), self.TIP_scatterU1)
        self.scatterU2      = tip(QDoubleSpinBox(), self.TIP_scatterU2)
        self.scatterU3      = tip(QDoubleSpinBox(), self.TIP_scatterU3)
        self.scatterV1      = tip(QDoubleSpinBox(), self.TIP_scatterV1)
        self.scatterV2      = tip(QDoubleSpinBox(), self.TIP_scatterV2)
        self.scatterV3      = tip(QDoubleSpinBox(), self.TIP_scatterV3)

        set_spin(self.omegaOffset)
        set_spinLattice(self.latticeA)
        set_spinLattice(self.latticeB)
        set_spinLattice(self.latticeC)
        set_spin(self.latticeAlpha, 0)
        set_spin(self.latticeBeta,  0)
        set_spin(self.latticeGamma, 0)
        set_spin(self.scatterU1)
        set_spin(self.scatterU2)
        set_spin(self.scatterU3)
        set_spin(self.scatterV1)
        set_spin(self.scatterV2)
        set_spin(self.scatterV3)

        self.standardDataPath    = tip(QLineEdit(), self.TIP_standardDataPath)
        self.btnStandardDataPath = tip(QPushButton('Browse'), self.TIP_btnStandardDataPath)
        #self.vanSuff             = tip(QLineEdit(), self.TIP_vanSuff)
        #self.NiCrSuff            = tip(QLineEdit(), self.TIP_NiCrSuff)
        #self.backSuff            = tip(QLineEdit(), self.TIP_backSuff)

        self.chkdetEffi          = tip(QCheckBox('Detector efficiency correction'), self.TIP_chkDetEffi)
        self.chksumVan           = tip(QCheckBox('Sum Vanadium'), self.TIP_chkSumVan)
        self.chksubInst          = tip(QCheckBox('Subtract instrument background'), self.TIP_chkSubInst)
        self.chkFlippRatio       = tip(QCheckBox('Flipping ratio correction'),self.TIP_chkFlippRatio)
        self.multiSF             = tip(QDoubleSpinBox(), self.TIP_multiSF)
        self.chkKeepIntermediate = tip(QCheckBox('Keep intermediate workspaces'), self.TIP_chkKeepIntermadiate)

        set_spin(self.multiSF, 0.0, 1.0)

        self.sampleDataPath    = tip(QLineEdit(), self.TIP_sampleDataPath)
        self.btnSampleDataPath = tip(QPushButton('Browse'), self.TIP_btnSampleDataPath)
        self.sampleFilePre     = tip(QLineEdit(), self.TIP_sampleFilePre)
        self.sampleFileSuff    = tip(QLineEdit(), self.TIP_sampleFileSuff)

        self.runsView = tip(QTableView(self), self.TIP_runsView)
        self.runsView.horizontalHeader().setStretchLastSection(True)
        self.runsView.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)

        self.runNumbersModel = DNSSetupWidget.DataTable(self)
        self.runsView.setModel(self.runNumbersModel)

        def _box(cls, widgets):
            box = cls()
            for wgt in widgets:
                if isinstance(wgt,QLayout):
                    box.addLayout(wgt)
                elif isinstance(wgt, QWidget):
                    box.addWidget(wgt)
                #else:
                #    box.addStrech(wgt)
            return box

        def hbox(widgets):
            return _box(QHBoxLayout, widgets)

        def vbox(widgets):
            return _box(QVBoxLayout, widgets)

        def lable(text, tip):
            lable = QLabel(text)
            if tip:
                lable.setToolTip(tip)
            return lable

        gbGeneral    = QGroupBox('General')
        gbMaskDet    = QGroupBox('Mask Detectors')
        gbOutput     = QGroupBox('Output')
        gbStdData    = QGroupBox('Standard data')
        gbDataRed    = QGroupBox('Data reduction settings')
        gbSampleData = QGroupBox('Sample data')
        gbRuns       = QGroupBox('')
        gbOut        = QGroupBox('')
        gbOutSoftMag = QGroupBox('')
        gbOutSingle   = QGroupBox('')

        box = QHBoxLayout()
        self._layout.addLayout(box)

        box.addLayout(vbox((gbGeneral, gbMaskDet, gbOutput)))
        box.addLayout(vbox((gbDataRed, gbStdData, gbSampleData, gbRuns)))

        bntGroup = QButtonGroup(self)
        bntGroup.addButton(self.rbnNormaliseMonitor)
        bntGroup.addButton(self.rbnNormaliseDuration)

        grid = QGridLayout()
        #grid.addWidget(QLabel('Coil currents table'),       0, 0)
        #grid.addWidget(self.currTable,                      0, 1)
        #grid.addWidget(self.btnCurrTable,                   0, 2)
        grid.addWidget(QLabel( u'2\u0398 tolerance'),       0, 0)
        grid.addWidget(self.twoTheta,                       0, 1)
        grid.addWidget(QLabel('Normalization'),             1, 0)
        grid.addWidget(self.rbnNormaliseDuration,           1, 1)
        grid.addWidget(self.rbnNormaliseMonitor,            1, 2, 1, 20)
        grid.addWidget(QLabel('Neutron wavelength (\305)'), 2, 0)
        grid.addWidget(self.neutronWaveLength,              2, 1)

        gbGeneral.setLayout(grid)

        grid = QGridLayout(self)

        grid.addWidget(QLabel('Min Angle'), 0, 0, 1, 7)
        grid.addWidget(self.maskMinAngle,   0, 1)
        grid.addWidget(QLabel('Max Angle'), 0, 2)
        grid.addWidget(self.maskMaxAngle,   0, 3)


        gbMaskDet.setLayout(grid)

        bntGroup = QButtonGroup(self)
        bntGroup.addButton(self.rbnOutSoftMag)
        bntGroup.addButton(self.rbnOutSingle)


        grid = QGridLayout()
        grid.addWidget(QLabel('Output directory'),   0, 0)
        grid.addWidget(self.outDir,                  0, 1)
        grid.addWidget(self.btnOutDir,               0, 2)
        grid.addWidget(QLabel('Output file prefix'), 1, 0)
        grid.addWidget(self.outFile,                 1, 1)
        #grid.setRowStretch(0, 30)

        gbOut.setLayout(grid)

        grid = QGridLayout()
        grid.addWidget(QLabel('Output Axis'), 0, 1)
        grid.addWidget(self.chkOutAxQ,        1, 1)
        grid.addWidget(self.chkOutAxD,        1, 2)
        grid.addWidget(self.chkOutAx2Theta,   1, 3)

        grid.setColumnMinimumWidth(0, 30)

        gbOutSoftMag.setLayout(grid)

        grid = QGridLayout()
        grid.addWidget(QLabel('Omega offset'),       0, 1)
        grid.addWidget(self.omegaOffset,             0, 2)
        grid.addWidget(QLabel("Lattice parameters"), 1, 1)
        grid.addWidget(QLabel('a[\305]'),            2, 1)
        grid.addWidget(self.latticeA,                2, 2)
        grid.addWidget(QLabel('b[\305]'),            2, 3)
        grid.addWidget(self.latticeB,                2, 4)
        grid.addWidget(QLabel('c[\305]'),            2, 5)
        grid.addWidget(self.latticeC,                2, 6)
        grid.addWidget(QLabel(u'\u03B1[\u00B0]'),    3, 1)
        grid.addWidget(self.latticeAlpha,            3, 2)
        grid.addWidget(QLabel(u'\u03B2[\u00B0]'),    3, 3)
        grid.addWidget(self.latticeBeta,             3, 4)
        grid.addWidget(QLabel(u'\u03B3[\u00B0]'),    3, 5)
        grid.addWidget(self.latticeGamma,            3, 6)

        grid.setColumnMinimumWidth(0, 30)

        gbOutSingle.setLayout(grid)

        grid = QGridLayout()
        grid.addWidget(QLabel('Scattering Plane'),   0, 1)
        grid.addWidget(QLabel('u'),                  1, 1)
        grid.addWidget(self.scatterU1,               1, 2)
        grid.addWidget(self.scatterU2,               1, 3)
        grid.addWidget(self.scatterU3,               1, 4)
        grid.addWidget(QLabel('v'),                  1, 5)
        grid.addWidget(self.scatterV1,               1, 6)
        grid.addWidget(self.scatterV2,               1, 7)
        grid.addWidget(self.scatterV3,               1, 8)


        grid.setColumnMinimumWidth(0, 50)

        self.ChecksSingle = []
        self.ChecksSingle.append(self.chkOutAxD)
        self.ChecksSingle.append(self.chkOutAxQ)
        self.ChecksSingle.append(self.chkOutAx2Theta)

        self.SpinsSingle = []
        self.SpinsSingle.append(self.omegaOffset)
        self.SpinsSingle.append(self.latticeA)
        self.SpinsSingle.append(self.latticeB)
        self.SpinsSingle.append(self.latticeC)
        self.SpinsSingle.append(self.latticeAlpha)
        self.SpinsSingle.append(self.latticeBeta)
        self.SpinsSingle.append(self.latticeGamma)
        self.SpinsSingle.append(self.scatterU1)
        self.SpinsSingle.append(self.scatterU2)
        self.SpinsSingle.append(self.scatterU3)
        self.SpinsSingle.append(self.scatterV1)
        self.SpinsSingle.append(self.scatterV2)
        self.SpinsSingle.append(self.scatterV3)

        box = vbox((gbOut, self.rbnOutSoftMag, gbOutSoftMag, self.rbnOutSingle, gbOutSingle, grid))
        box.setStretch(0, 7)
        gbOutput.setLayout(box)

        bntGroup = QButtonGroup(self)
        bntGroup.addButton(self.chkdetEffi)
        bntGroup.addButton(self.chksumVan)

        grid = QGridLayout()
        grid.addWidget(self.chkdetEffi,                              0, 0)
        grid.addWidget(self.chksumVan,                               1, 0)
        grid.addWidget(self.chksubInst,                              2, 0)
        grid.addWidget(self.chkFlippRatio,                           3, 0)
        grid.addWidget(QLabel('Multiple SF scattering probability'), 4, 0, 1, 5)
        grid.addWidget(self.multiSF,                                 4, 1)
        grid.addWidget(self.chkKeepIntermediate,                     5, 0)

        grid.setColumnMinimumWidth(1, 0)

        gbDataRed.setLayout(grid)

        grid = QGridLayout()
        grid.addWidget(QLabel('Path'),              0, 0)
        grid.addWidget(self.standardDataPath,       0, 1)
        grid.addWidget(self.btnStandardDataPath,    0, 2)
        #grid.addWidget(QLabel('Vanadium Suffix'),   1, 0)
        #grid.addWidget(self.vanSuff,                1, 1)
        #grid.addWidget(QLabel('NiCr Suffix'),       2, 0)
        #grid.addWidget(self.NiCrSuff,               2, 1)
        #grid.addWidget(QLabel('Background Suffix'), 3, 0)
        #grid.addWidget(self.backSuff,               3, 1)

        gbStdData.setLayout(grid)

        grid = QGridLayout()
        grid.addWidget(QLabel('Data path'),    0, 0)
        grid.addWidget(self.sampleDataPath,    0, 1)
        grid.addWidget(self.btnSampleDataPath, 0, 2)
        grid.addWidget(QLabel('File prefix'),  1, 0)
        grid.addWidget(self.sampleFileSuff,    1, 1)
        grid.addWidget(QLabel('suffix'),       1, 2)
        grid.addWidget(self.sampleFilePre,     1, 3)

        gbSampleData.setLayout(grid)

        gbRuns.setLayout(hbox((self.runsView,)))

        for spin in self.SpinsSingle:
            spin.setEnabled(False)

        #for line in self.LinesSingle:
        #    line.setEnabled(False)

        for button in self.ChecksSingle:
            button.setCheckable(True)

        #self.btnCurrTable.clicked.connect(self._currTableFile)
        self.btnOutDir.clicked.connect(self._outputDir)
        self.rbnOutSingle.clicked.connect(self._rbnOutChanged)
        self.rbnOutSoftMag.clicked.connect(self._rbnOutChanged)
        self.btnStandardDataPath.clicked.connect(self._stdDataDir)
        self.btnSampleDataPath.clicked.connect(self._sampleDataDir)
        self.runNumbersModel.selectCell.connect(self._onSelectedCell)

    def _onSelectedCell(self, row, column):
        index = self.runNumbersModel.index(row, column)
        self.runsView.setCurrentIndex(index)
        self.runsView.setFocus()

    def _rbnOutChanged(self):
        if self.rbnOutSingle.isChecked():
            self._SingleChecked()
            self._SoftMagNotChecked()
        elif self.rbnOutSoftMag.isChecked():
            self._SingleNotChecked()
            self._SoftMagChecked()

    def _SingleChecked(self):

        for spin in self.SpinsSingle:
            spin.setEnabled(True)

        for line in self.LinesSingle:
            line.setEnabled(True)

    def _SoftMagChecked(self):

        for chkButton in self.ChecksSingle:
            chkButton.setEnabled(True)

    def _SingleNotChecked(self):

        for spin in self.SpinsSingle:
            spin.setEnabled(False)

        for line in self.LinesSingle:
            line.setEnabled(False)

    def _SoftMagNotChecked(self):

        for chkButton in self.ChecksSingle:
            chkButton.setEnabled(False)

    #def _currTableFile(self):

        #fname = QFileDialog.getOpenFileName(self, 'Open file', '/home')

        #if fname:
        #   self.currTable.setText(fname)

    def _outputDir(self):

        fname = QFileDialog.getExistingDirectory(self, 'Open Directory', '/home', QFileDialog.ShowDirsOnly)

        if fname:
            self.outDir.setText(fname)


    def _stdDataDir(self):

        fname = QFileDialog.getExistingDirectory(self, 'Open Directory', '/home', QFileDialog.ShowDirsOnly)

        if fname:
            self.standardDataPath.setText(fname)

    def _sampleDataDir(self):

        fname = QFileDialog.getExistingDirectory(self, 'Open Directory', '/home', QFileDialog.ShowDirsOnly)

        if fname:
            self.sampleDataPath.setText(fname)

    def get_state(self):

        elem = DNSScriptElement()

        def line_text(lineEdit):
            return lineEdit.text().strip()


        elem.facility_name   = self._settings.facility_name
        elem.instrument_name = self._settings.instrument_name

        #elem.currTable      = line_text(self.currTable)
        elem.twoTheta       = self.twoTheta.value()
        elem.normalise      = elem.NORM_DURATION if self.rbnNormaliseDuration.isChecked() else \
                              elem.NORM_MONITOR
        elem.neutronWaveLen = self.neutronWaveLength.value()

        elem.maskMinAngle = self.maskMinAngle.value()
        elem.maskMaxAngle = self.maskMaxAngle.value()

        elem.outDir     = line_text(self.outDir)
        elem.outPrefix  = line_text(self.outFile)
        elem.out        = elem.OUT_SOFT_MAG if self.rbnOutSoftMag.isChecked() else \
                          elem.OUT_SINGLE_CRYST

        elem.outAxisQ      = self.chkOutAxQ.checkState()
        elem.outAxisD      = self.chkOutAxD.checkState()
        elem.outAxis2Theta = self.chkOutAx2Theta.checkState()

        elem.omegaOffset = self.omegaOffset.value()

        elem.latticeA     = self.latticeA.value()
        elem.latticeB     = self.latticeB.value()
        elem.latticeC     = self.latticeC.value()
        elem.latticeAlpha = self.latticeAlpha.value()
        elem.latticeBeta  = self.latticeBeta.value()
        elem.latticeGamma = self.latticeGamma.value()

        elem.scatterU1 = (self.scatterU1)
        elem.scatterU2 = (self.scatterU2)
        elem.scatterU3 = (self.scatterU3)
        elem.scatterV1 = (self.scatterV1)
        elem.scatterV2 = (self.scatterV2)
        elem.scatterV3 = (self.scatterV3)


        elem.standardDataPath   = line_text(self.standardDataPath)
        #elem.VanSuffix          = line_text(self.vanSuff)
        #elem.NiCrSuffix         = line_text(self.NiCrSuff)
        #elem.backSuffix         = line_text(self.backSuff)

        elem.detEffi        = self.chkdetEffi.isChecked()
        elem.sumVan         = self.chksumVan.isChecked()
        elem.subInst        = self.chksubInst.isChecked()
        elem.flippRatio     = self.chkFlippRatio.isChecked()
        elem.multiSF        = self.multiSF.value()
        elem.intermadiate   = self.chkKeepIntermediate.isChecked()

        elem.sampleDataPath = line_text(self.sampleDataPath)
        elem.filePrefix     = line_text(self.sampleFilePre)
        elem.fileSuffix     = line_text(self.sampleFileSuff)

        elem.dataRuns       = self.runNumbersModel.tableData

        return elem

    def set_state(self, dnsScriptElement):

        elem = dnsScriptElement

        # self.currTable.setText(elem.currTable)
        self.twoTheta.setValue(elem.twoTheta)

        if elem.normalise == elem.NORM_DURATION:
            self.rbnNormaliseDuration.setChecked(True)
        elif elem.normalise == elem.NORM_MONITOR:
            self.rbnNormaliseMonitor.setChecked(True)

        self.neutronWaveLength.setValue(elem.neutronWaveLen)

        self.maskMinAngle.setValue(elem.maskMinAngle)
        self.maskMaxAngle.setValue(elem.maskMaxAngle)

        self.outDir.setText(elem.outDir)
        self.outFile.setText(elem.outPrefix)

        if elem.out == elem.OUT_SOFT_MAG:
            self.rbnOutSoftMag.setChecked(True)
        else:
            self.rbnOutSingle.setChecked(True)

        if elem.outAxisQ:
            self.chkOutAxQ.setChecked(True)

        if elem.outAxisD:
            self.chkOutAxD.setChecked(True)

        if elem.outAxis2Theta:
            self.chkOutAx2Theta.setChecked(True)

        self.latticeA.setValue(elem.latticeA)
        self.latticeB.setValue(elem.latticeB)
        self.latticeC.setValue(elem.latticeC)
        self.latticeAlpha.setValue(elem.latticeAlpha)
        self.latticeBeta.setValue(elem.latticeBeta)
        self.latticeGamma.setValue(elem.latticeGamma)

        #self.scatterU.setText(elem.scatterU)
        #self.scatterV.setText(elem.scatterV)

        self.scatterU1.setValue(elem.scatterU1)
        self.scatterU2.setValue(elem.scatterU2)
        self.scatterU3.setValue(elem.scatterU3)
        self.scatterV1.setValue(elem.scatterV1)
        self.scatterV2.setValue(elem.scatterV2)
        self.scatterV3.setValue(elem.scatterV3)

        self.standardDataPath.setText(elem.standardDataPath)
        #self.vanSuff.setText(elem.VanSuffix)
        #self.NiCrSuff.setText(elem.NiCrSuffix)
        #self.backSuff.setText(elem.backSuffix)

        self.chkdetEffi.setChecked(elem.detEffi)
        self.chksumVan.setChecked(elem.sumVan)
        self.chksubInst.setChecked(elem.subInst)
        self.chkFlippRatio.setChecked(elem.flippRatio)
        self.multiSF.setValue(elem.multiSF)
        self.chkKeepIntermediate.setChecked(elem.intermadiate)

        self.runNumbersModel.tableData = elem.dataRuns
        self.runNumbersModel.reset()