import sys
from PyQt4 import QtGui, QtCore

from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.dns.dns_reduction import DNSScriptElement


class DNSSetupWidget(BaseWidget):
    
    name = 'DNS Reduction'

    font = QtGui.QFont()
    font.setBold(True)
    font.setUnderline(True)

    def __init__(self, settings):

        BaseWidget.__init__(self, settings = settings)

	inf = float('inf')

        self.initWindow()

    def initWindow(self):

        Layout = QtGui.QHBoxLayout(self)

        Lbox = QtGui.QVBoxLayout(self)

        topleft = QtGui.QFrame(self)
        topleft.setFrameStyle(QtGui.QFrame.StyledPanel)
        topleft.setGeometry(10, 10, 440, 140)

        Lbox.addWidget(topleft)

        self.initTopLeft(topleft)

        midleft = QtGui.QFrame(self)
        midleft.setFrameStyle(QtGui.QFrame.StyledPanel)
        midleft.setGeometry(10, 160, 440, 60)

        Lbox.addWidget(midleft)

        self.initMidLeft(midleft)

        bottomleft = QtGui.QFrame(self)
        bottomleft.setFrameStyle(QtGui.QFrame.StyledPanel)
        bottomleft.setGeometry(10, 230, 440, 320)

        Lbox.addWidget(bottomleft)

        self.initBottomLeft(bottomleft)

        Rbox = QtGui.QVBoxLayout(self)

        topright = QtGui.QFrame(self)
        topright.setFrameStyle(QtGui.QFrame.StyledPanel)
        topright.setGeometry(460, 10, 440, 160)

        Rbox.addWidget(topright)

        self.initTopRight(topright)

        midright = QtGui.QFrame(self)
        midright.setFrameStyle(QtGui.QFrame.StyledPanel)
        midright.setGeometry(460, 180, 440, 150)

        self.initMidRight(midright)

        bottomright = QtGui.QFrame(self)
        bottomright.setFrameStyle(QtGui.QFrame.StyledPanel)
        bottomright.setGeometry(460, 340, 440, 210)

        Rbox.addWidget(bottomright)

        self.initBottomRight(bottomright)

        run = QtGui.QPushButton("Run", self)
        run.move(690, 555)

        cancel = QtGui.QPushButton("Cancel", self)
        cancel.move(800, 555)

        self.setLayout(Layout)
        self.setGeometry(300, 300, 910, 600)
        self.show()


    def initTopLeft(self, top):

        lbl = QtGui.QLabel('General', top)
        lbl.move(15, 5)

        lbl.setFont(self.font)

        currtabel = QtGui.QLabel('Coil currents table', top)
        currtabel.move(15,30)

        self.currtabelLine = QtGui.QLineEdit(top)
        self.currtabelLine.move(180, 25)


        openfile = QtGui.QPushButton('Browse', top)
        openfile.move(350, 25)

        openfile.clicked.connect(self.searchCurrTable)

        theta = QtGui.QLabel('2-Theta tolerance', top)
        theta.move(15, 60)

        thetaLine = QtGui.QLineEdit(top)
        thetaLine.move(180, 55)

        normal = QtGui.QLabel('Normalization', top)
        normal.move(15, 85)

        buttongroup = QtGui.QButtonGroup()

        normalradio = QtGui.QRadioButton('duration', top)
        normalradio.setCheckable(True)
        normalradio.setChecked(True)
        normalradio.move(180, 82)

        normalradio2 = QtGui.QRadioButton('monitor', top)
        normalradio2.setCheckable(True)
        normalradio2.move(280, 82)

        buttongroup.addButton(normalradio)
        buttongroup.addButton(normalradio2)

        wave = QtGui.QLabel("Neutron wavelength (\305)", top)
        wave.move(15, 110)

        waveline = QtGui.QLineEdit(top)
        waveline.move(200, 105)
        waveline.setText("0.0")

    def initMidLeft(self, mid):

        lbl2 = QtGui.QLabel("Mask Detectors", mid)
        lbl2.move(15, 5)
        lbl2.setFont(self.font)

        minAngle = QtGui.QLabel("Min Angle", mid)
        minAngle.move(15, 30)

        minAngleLine = QtGui.QLineEdit(mid)
        minAngleLine.setGeometry(100, 25, 80, 27)
        minAngleLine.setText("0.0")

        maxAngle = QtGui.QLabel("Max Angle", mid)
        maxAngle.move(190, 30)

        maxAngleLine = QtGui.QLineEdit(mid)
        maxAngleLine.setGeometry(275, 25, 80, 27)
        maxAngleLine.setText("0.0")

    def initBottomLeft(self, bot):

        lbl3 = QtGui.QLabel("Output", bot)
        lbl3.move(15, 5)
        lbl3.setFont(self.font)

        outdir = QtGui.QLabel("Output directory", bot)
        outdir.move(15, 30)

        self.outdirLine = QtGui.QLineEdit(bot)
        self.outdirLine.move(180, 25)

        openFile = QtGui.QPushButton("Browse", bot)
        openFile.move(350, 25)
        openFile.clicked.connect(self.outputDir)

        outfile = QtGui.QLabel("Output file prefix", bot)
        outfile.move(15, 60)

        outfileLine = QtGui.QLineEdit(bot)
        outfileLine.move(180, 55)

        buttongroup2 = QtGui.QButtonGroup()

        soft = QtGui.QRadioButton("Soft matter", bot)
        soft.move(15, 130)
        soft.setChecked(True)

        buttongroup2.addButton(soft)

        magnetic = QtGui.QRadioButton("Magnetic powder", bot)
        magnetic.move(15, 175)

        buttongroup2.addButton(magnetic)

        single = QtGui.QRadioButton("Single Crystal", bot)
        single.move(15, 220)

        buttongroup2.addButton(single)

    def initTopRight(self, top):

        lbl5 = QtGui.QLabel("Data reduction settings", top)
        lbl5.move(15, 5)
        lbl5.setFont(self.font)

        buttons = QtGui.QButtonGroup(top)

        corr = QtGui.QCheckBox("Detector efficiency correction", top)
        corr.move(15, 25)
        corr.setChecked(True)

        buttons.addButton(corr)

        sum = QtGui.QCheckBox("Sum Vanadium", top)
        sum.move(15, 45)

        buttons.addButton(sum)

        sub = QtGui.QCheckBox("Subtract instrument background", top)
        sub.move(15, 65)
        sub.setChecked(True)

        flip = QtGui.QCheckBox("Flipping ratio conrection", top)
        flip.move(15, 85)
        flip.setChecked(True)

        multi = QtGui.QLabel("Multiple SF scattering probability", top)
        multi.move(15, 110)

        multiLine = QtGui.QLineEdit(top)
        multiLine.setGeometry(270, 105, 80, 27)
        multiLine.setText("0.0")

        ws = QtGui.QCheckBox("Keep intermediate workspaces", top)
        ws.move(15, 130)

    def initMidRight(self, mid):

        lbl4 = QtGui.QLabel("Standard data", mid)
        lbl4.move(15, 5)
        lbl4.setFont(self.font)

        path = QtGui.QLabel("Path", mid)
        path.move(15,30)

        self.pathLine = QtGui.QLineEdit(mid)
        self.pathLine.move(180, 25)

        openFile = QtGui.QPushButton("Browse", mid)
        openFile.move(350, 25)
        openFile.clicked.connect(self.searchPath)

        vand = QtGui.QLabel("Vanadium Suffix", mid)
        vand.move(15, 60)

        vandLine = QtGui.QLineEdit(mid)
        vandLine.move(180, 55)
        vandLine.setText("vana")

        nicr = QtGui.QLabel("NiCr Suffix", mid)
        nicr.move(15, 90)

        nircLine = QtGui.QLineEdit(mid)
        nircLine.move(180, 85)
        nircLine.setText("nicr")

        background = QtGui.QLabel("Background Suffix", mid)
        background.move(15, 120)

        backgroundLine = QtGui.QLineEdit(mid)
        backgroundLine.move(180, 115)
        backgroundLine.setText("leer")


    def initBottomRight(self, bot):

        lbl4 = QtGui.QLabel("Sample data", bot)
        lbl4.move(15, 5)
        lbl4.setFont(self.font)

        data = QtGui.QLabel("Data path", bot)
        data.move(15, 30)

        self.dataLine = QtGui.QLineEdit(bot)
        self.dataLine.move(180, 25)

        openFile = QtGui.QPushButton("Browse", bot)
        openFile.move(350, 25)
        openFile.clicked.connect(self.searchData)

        file = QtGui.QLabel("File", bot)
        file.move(15, 60)

        pre = QtGui.QLabel("prefix", bot)
        pre.move(60, 60)

        preLine = QtGui.QLineEdit(bot)
        preLine.setGeometry(125, 55, 80, 27)

        suf = QtGui.QLabel("suffix", bot)
        suf.move(225, 60)

        sufLine = QtGui.QLineEdit(bot)
        sufLine.setGeometry(290, 55, 80, 27)

        self.Table = QtGui.QTableWidget(3, 2, bot)
        self.Table.setGeometry(15, 90, 400, 115)
        self.Table.setHorizontalHeaderLabels(["Run numbers", "Comment"])
        #Table.setItem(0,0,QtGui.QTableWidgetItem("110:115"))
        self.Table.setColumnWidth(0, 181.5)
        self.Table.setColumnWidth(1, 181.5)

        self.Table.itemChanged.connect(self.someFunc)

    def someFunc(self, item):

        if item.row() == self.Table.rowCount() -1:
            if item.text():
                self.Table.insertRow(self.Table.rowCount())


    def searchCurrTable(self):

        fname = QtGui.QFileDialog.getOpenFileName(self, 'Open file', '/home')

        if fname:
            self.currtabelLine.setText(fname)

    def outputDir(self):

        fname = QtGui.QFileDialog.getExistingDirectory(self, 'Open Directory', '/home', QtGui.QFileDialog.ShowDirsOnly)

        if fname:
            self.outdirLine.setText(fname)


    def searchPath(self):

        fname = QtGui.QFileDialog.getExistingDirectory(self, 'Open Directory', '/home', QtGui.QFileDialog.ShowDirsOnly)

        if fname:
            self.pathLine.setText(fname)

    def searchData(self):

        fname = QtGui.QFileDialog.getExistingDirectory(self, 'Open Directory', '/home', QtGui.QFileDialog.ShowDirsOnly)

        if fname:
            self.dataLine.setText(fname)



def main():

    app = QtGui.QApplication(sys.argv)
    ex = Window()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
