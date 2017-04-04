import sys
from PyQt4.QtGui import *
from PyQt4.QtCore import *

from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.dns.dns_reduction import DNSScriptElement


class DNSSetupWidget(BaseWidget):
    
    name = 'DNS Reduction'

    TIP_currentTable         = ''
    TIP_btnCurrentTable      = ''
    TIP_twoTheta             = ''
    TIP_rbnNormaliseDuration = ''
    TIP_rbnNormaliseMonitor  = ''
    TIP_neutronWaveLen       = ''

    TIP_maskMinAngle = ''
    TIP_maskMaxAngle = ''

    TIP_outDir         = ''
    TIP_btnOutDir      = ''
    TIP_outFile        = ''
    TIP_rbnOutSoft     = ''
    TIP_rbnOutMagnetic = ''
    TIP_rbnOutSingle   = ''
    TIP_chkOutAxQ        = ''
    TIP_chkOutAxD        = ''
    TIP_chkOutAx2Theta   = ''
    TIP_omegaOffset    = ''
    TIP_latticeA       = ''
    TIP_latticeB       = ''
    TIP_latticeC       = ''
    TIP_latticeAlpha   = ''
    TIP_latticeBeta    = ''
    TIP_latticeGamma   = ''
    TIP_scatterU       = ''
    TIP_scatterV       = ''

    TIP_standardDataPath    = ''
    TIP_btnStandardDataPath = ''
    TIP_vanSuff             = ''
    TIP_NiCrSuff            = ''
    TIP_backSuff            = ''

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
    TIP_dataRuns          = ''


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

        self.currTable            = tip(QLineEdit(), self.TIP_currentTable)
        self.btnCurrTable         = tip(QPushButton('Browse'), self.TIP_btnCurrentTable)
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
        self.rbnOutSoft     = tip(QRadioButton('Soft matter'), self.TIP_rbnOutSoft)
        self.rbnOutMagnetic = tip(QRadioButton('Magnetic powder'), self.TIP_rbnOutMagnetic)
        self.rbnOutSingle   = tip(QRadioButton('Single Crystal'), self.TIP_rbnOutSingle)
        self.chkOutAxQ      = tip(QCheckBox('q'), self.TIP_chkOutAxQ)
        self.chkOutAxD      = tip(QCheckBox('d'), self.TIP_chkOutAxD)
        self.chkOutAx2Theta = tip(QCheckBox(u'2\u0398'), self.TIP_chkOutAx2Theta)
        self.omegaOffset    = tip(QDoubleSpinBox(), self.TIP_omegaOffset)
        self.latticeA       = tip(QDoubleSpinBox(), self.TIP_latticeA)
        self.latticeB       = tip(QDoubleSpinBox(), self.TIP_latticeB)
        self.latticeC       = tip(QDoubleSpinBox(), self.TIP_latticeC)
        self.latticeAlpha   = tip(QDoubleSpinBox(), self.TIP_latticeAlpha)
        self.latticeBeta    = tip(QDoubleSpinBox(), self.TIP_latticeBeta)
        self.latticeGamma   = tip(QDoubleSpinBox(), self.TIP_latticeGamma)
        self.scatterU       = tip(QLineEdit(), self.TIP_scatterU)
        self.scatterV       = tip(QLineEdit(), self.TIP_scatterV)


        self.standardDataPath    = tip(QLineEdit(), self.TIP_standardDataPath)
        self.btnStandardDataPath = tip(QPushButton('Browse'), self.TIP_btnStandardDataPath)
        self.vanSuff             = tip(QLineEdit(), self.TIP_vanSuff)
        self.NiCrSuff            = tip(QLineEdit(), self.TIP_NiCrSuff)
        self.backSuff            = tip(QLineEdit(), self.TIP_backSuff)

        self.chkdetEffi          = tip(QCheckBox('Detector efficiency correction'), self.TIP_chkDetEffi)
        self.chksumVan           = tip(QCheckBox('Sum Vanadium'), self.TIP_chkSumVan)
        self.chksubInst          = tip(QCheckBox('Subtract instrument background'), self.TIP_chkSubInst)
        self.chkFlippRatio       = tip(QCheckBox('Flipping ratio conrection'),self.TIP_chkFlippRatio)
        self.multiSF             = tip(QDoubleSpinBox(), self.TIP_multiSF)
        self.chkKeepIntermediate = tip(QCheckBox('Keep intermediate workspaces'), self.TIP_chkKeepIntermadiate)

        set_spin(self.multiSF, 0.0, 1.0)

        self.sampleDataPath    = tip(QLineEdit(), self.TIP_sampleDataPath)
        self.btnSampleDataPath = tip(QPushButton('Browse'), self.TIP_btnSampleDataPath)
        self.sampleFilePre     = tip(QLineEdit(), self.TIP_sampleFilePre)
        self.sampleFileSuff    = tip(QLineEdit(), self.TIP_sampleFileSuff)

        self.dataRuns= tip(DNSSetupWidget.DataTable(self), self.TIP_dataRuns)

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
        gbOutRbn     = QGroupBox('')

        box = QHBoxLayout()
        self._layout.addLayout(box)

        box.addLayout(vbox((gbGeneral, gbMaskDet,gbOutput, gbOutRbn)))
        box.addLayout(vbox((gbDataRed, gbStdData, gbSampleData, gbRuns)))

        bntGroup = QButtonGroup(self)
        bntGroup.addButton(self.rbnNormaliseMonitor)
        bntGroup.addButton(self.rbnNormaliseDuration)

        grid = QGridLayout()
        grid.addWidget(QLabel('Coil currents table'),       0, 0)
        grid.addWidget(self.currTable,                      0, 1)
        grid.addWidget(self.btnCurrTable,                   0, 2)
        grid.addWidget(QLabel('2-Theta tolerance'),         1, 0)
        grid.addWidget(self.twoTheta,                       1, 1)
        grid.addWidget(QLabel('Normalization'),             2, 0)
        grid.addWidget(self.rbnNormaliseDuration,           2, 1)
        grid.addWidget(self.rbnNormaliseMonitor,            2, 2, 1, 20)
        grid.addWidget(QLabel('Neutron wavelength (\305)'), 3, 0)
        grid.addWidget(self.neutronWaveLength,              3, 1)

        gbGeneral.setLayout(grid)

        grid = QGridLayout(self)

        grid.addWidget(QLabel('Min Angle'), 0, 0, 1, 7)
        grid.addWidget(self.maskMinAngle,   0, 1)
        grid.addWidget(QLabel('Max Angle'), 0, 2)
        grid.addWidget(self.maskMaxAngle,   0, 3)


        gbMaskDet.setLayout(grid)

        bntGroup = QButtonGroup(self)
        bntGroup.addButton(self.rbnOutSoft)
        bntGroup.addButton(self.rbnOutMagnetic)
        bntGroup.addButton(self.rbnOutSingle)


        grid = QGridLayout()
        grid.addWidget(QLabel('Output directory'),   0, 0)
        grid.addWidget(self.outDir,                  0, 1)
        grid.addWidget(self.btnOutDir,               0, 2)
        grid.addWidget(QLabel('Output file prefix'), 1, 0)
        grid.addWidget(self.outFile, 1, 1)


        gbOutput.setLayout(grid)

        grid = QGridLayout()
        grid.addWidget(self.rbnOutSoft,              0, 0)
        grid.addWidget(self.rbnOutMagnetic,          1, 0)
        grid.addWidget(self.rbnOutSingle,            2, 0)
        grid.addWidget(QLabel('Output Axis'),        3, 0)
        grid.addWidget(self.chkOutAxQ,               4, 0)
        grid.addWidget(self.chkOutAxD,               4, 1)
        grid.addWidget(self.chkOutAx2Theta,          4, 2)
        grid.addWidget(QLabel('Omega offset'),       5, 0)
        grid.addWidget(self.omegaOffset,             5, 1)
        grid.addWidget(QLabel("Lattice parameters"), 6, 0)
        grid.addWidget(QLabel('a[\305]'),            7, 0)
        grid.addWidget(self.latticeA,                7, 1)
        grid.addWidget(QLabel('a[\305]'),            7, 2)
        grid.addWidget(self.latticeB,                7, 3)
        grid.addWidget(QLabel('c[\305]'),            7, 4)
        grid.addWidget(self.latticeC,                7, 5)
        grid.addWidget(QLabel(u'\u03B1[\u00B0]'),    8, 0)
        grid.addWidget(self.latticeAlpha,            8, 1)
        grid.addWidget(QLabel(u'\u03B2[\u00B0]'),    8, 2)
        grid.addWidget(self.latticeBeta,             8, 3)
        grid.addWidget(QLabel(u'\u03B3[\u00B0]'),    8, 4)
        grid.addWidget(self.latticeGamma,            8, 5)
        grid.addWidget(QLabel('Scattering Plane'),   9, 0)
        grid.addWidget(QLabel('u'),                 10, 0)
        grid.addWidget(self.scatterU,               10, 1)
        grid.addWidget(QLabel('v'),                 10, 2)
        grid.addWidget(self.scatterV,               10, 3)

        self.ChecksSingle = QButtonGroup()
        self.ChecksSingle.addButton(self.chkOutAxD)
        self.ChecksSingle.addButton(self.chkOutAxQ)
        self.ChecksSingle.addButton(self.chkOutAx2Theta)

        self.SpinsSingle = []
        self.SpinsSingle.append(self.omegaOffset)
        self.SpinsSingle.append(self.latticeA)
        self.SpinsSingle.append(self.latticeB)
        self.SpinsSingle.append(self.latticeC)
        self.SpinsSingle.append(self.latticeAlpha)
        self.SpinsSingle.append(self.latticeBeta)
        self.SpinsSingle.append(self.latticeGamma)

        self.LinesSingle = []
        self.LinesSingle.append(self.scatterU)
        self.LinesSingle.append(self.scatterV)

        for spin in self.SpinsSingle:
            spin.setEnabled(False)

        for line in self.LinesSingle:
            line.setEnabled(False)

        for button in self.ChecksSingle.buttons():
            button.setCheckable(False)
        #self.chkOutAxQ.setCheckable(False)
        self.rbnOutSingle.clicked.connect(self._rbnOutChanged)
        self.rbnOutMagnetic.clicked.connect(self._rbnOutChanged)
        self.rbnOutSoft.clicked.connect(self._rbnOutChanged)


        gbOutRbn.setLayout(grid)
        #grid.setRowStretch(0, 20)
        #grid.setRowStretch(4, 1)
        #grid.setRowStretch(3, 1)
        #grid.setRowStretch(2, 1)




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
        grid.addWidget(QLabel('Vanadium Suffix'),   1, 0)
        grid.addWidget(self.vanSuff,                1, 1)
        grid.addWidget(QLabel('NiCr Suffix'),       2, 0)
        grid.addWidget(self.NiCrSuff,               2, 1)
        grid.addWidget(QLabel('Background Suffix'), 3, 0)
        grid.addWidget(self.backSuff,               3, 1)

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

        grid = QGridLayout()
        grid.addWidget(self.dataRuns, 0, 0)
        gbRuns.setLayout(grid)

        self.btnCurrTable.clicked.connect(self._currTableFile)
        self.btnOutDir.clicked.connect(self._outputDir)
        self.btnStandardDataPath.clicked.connect(self._stdDataDir)
        self.btnSampleDataPath.clicked.connect(self._sampleDataDir)


    class DataTable(QTableWidget):

        headers = ('Run numbers', 'Comment')


        def __init__(self, frame):

            QTableWidget.__init__(self, 1, 2, frame)
            self.resizeRowsToContents()
            self.itemChanged.connect(self.someFunc)
            self.setHorizontalHeaderLabels(self.headers)
            self.horizontalHeader().setResizeMode(QHeaderView.Stretch)
            print self.sizeHint()
            #self.setColumnWidth(self.width(),self.width()/2)

            self.tableData = []

        def setTableData(self, data):

            for i, (runs, cmnt) in enumerate(self.tableData):
                self.setItem(i,0,runs)
                self.setItem(i,1,cmnt)

        def tableToMap(self):

            for i in range(self.rowCount() - 1):
                self.tableData.append((self.rowAt(0),self.rowAt(1)))

        def someFunc(self, item):

            if item.row() == self.rowCount() - 1:
                if item.text():
                    self.insertRow(self.rowCount())

    def _rbnOutChanged(self):
        if self.rbnOutSingle.isChecked():
            self._SingleChecked()
        else:
            self._SingleNotChecked()

    def _SingleChecked(self):
        for chkButton in self.ChecksSingle.buttons():
            chkButton.setCheckable(True)

        for spin in self.SpinsSingle:
            spin.setEnabled(True)

        for line in self.LinesSingle:
            line.setEnabled(True)
        #self.chkOutAxQ.setCheckable(True)
        #self.chkOutAxD.setCheckable(True)

    def _SingleNotChecked(self):
        for chkButton in self.ChecksSingle.buttons():
            chkButton.setChecked(False)
            chkButton.setCheckable(False)

        for spin in self.SpinsSingle:
            spin.setEnabled(False)

        for line in self.LinesSingle:
            line.setEnabled(False)


    def _currTableFile(self):

        fname = QFileDialog.getOpenFileName(self, 'Open file', '/home')

        if fname:
           self.currTable.setText(fname)

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

        elem.facility_name = self._settings.facility_name
        elem.instrument_name = self._settings.instrument_name

        elem.currTable      = line_text(self.currTable)
        elem.twoTheta       = self.twoTheta.value()
        elem.normalise      = elem.NORM_DURATION if self.rbnNormaliseDuration.isChecked() else \
                              elem.NORM_MONITOR
        elem.neutronWaveLen = self.neutronWaveLength.value()

        elem.maskMinAngle = self.maskMinAngle.value()
        elem.maskMaxAngle = self.maskMaxAngle.value()

        elem.outDir     = line_text(self.outDir)
        elem.outPrefix  = line_text(self.outFile)
        elem.out        = elem.OUT_SOFT if self.rbnOutSoft.isChecked() else \
                          elem.OUT_MAGNETIC if self.rbnOutMagnetic.isChecked() else \
                          elem.OUT_SINGLE_CRYST

        elem.standardDataPath   = line_text(self.standardDataPath)
        elem.VanSuffix          = line_text(self.vanSuff)
        elem.NiCrSuffix         = line_text(self.NiCrSuff)
        elem.backSuffix         = line_text(self.backSuff)

        elem.detEffi        = self.chkdetEffi.isChecked()
        elem.sumVan         = self.chksumVan.isChecked()
        elem.subInst        = self.chksubInst.isChecked()
        elem.flippRatio     = self.chkFlippRatio.isChecked()
        elem.multiSF        = self.multiSF.value()
        elem.intermadiate   = self.chkKeepIntermediate.isChecked()

        elem.sampleDataPath = line_text(self.sampleDataPath)
        elem.filePrefix     = line_text(self.sampleFilePre)
        elem.fileSuffix     = line_text(self.sampleFileSuff)
        elem.dataRuns       = self.dataRuns


        return elem

    def set_state(self, dnsScriptElement):

        elem = dnsScriptElement

        self.currTable.setText(elem.currTable)
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

        if elem.out == elem.OUT_SOFT:
            self.rbnOutSoft.setChecked(True)
        elif elem.out == elem.OUT_MAGNETIC:
            self.rbnOutMagnetic.setChecked(True)
        else:
            self.rbnOutSingle.setChecked(True)

        self.standardDataPath.setText(elem.standardDataPath)
        self.vanSuff.setText(elem.VanSuffix)
        self.NiCrSuff.setText(elem.NiCrSuffix)
        self.backSuff.setText(elem.backSuffix)

        self.chkdetEffi.setChecked(elem.detEffi)
        self.chksumVan.setChecked(elem.sumVan)
        self.chksubInst.setChecked(elem.subInst)
        self.chkFlippRatio.setChecked(elem.flippRatio)
        self.multiSF.setValue(elem.multiSF)
        self.chkKeepIntermediate.setChecked(elem.intermadiate)

        self.dataRuns = elem.dataRuns