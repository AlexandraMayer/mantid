# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui_MainWindow.ui'
#
# Created: Fri Mar  6 22:24:29 2015
#      by: PyQt4 UI code generator 4.10.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName(_fromUtf8("MainWindow"))
        MainWindow.resize(800, 600)
        self.centralwidget = QtGui.QWidget(MainWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.gridLayout = QtGui.QGridLayout(self.centralwidget)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.verticalLayout_3 = QtGui.QVBoxLayout()
        self.verticalLayout_3.setObjectName(_fromUtf8("verticalLayout_3"))
        self.horizontalLayout_General = QtGui.QHBoxLayout()
        self.horizontalLayout_General.setObjectName(_fromUtf8("horizontalLayout_General"))
        self.label_exp = QtGui.QLabel(self.centralwidget)
        self.label_exp.setObjectName(_fromUtf8("label_exp"))
        self.horizontalLayout_General.addWidget(self.label_exp)
        self.lineEdit_exp = QtGui.QLineEdit(self.centralwidget)
        self.lineEdit_exp.setObjectName(_fromUtf8("lineEdit_exp"))
        self.horizontalLayout_General.addWidget(self.lineEdit_exp)
        self.label_runPtPlot = QtGui.QLabel(self.centralwidget)
        self.label_runPtPlot.setObjectName(_fromUtf8("label_runPtPlot"))
        self.horizontalLayout_General.addWidget(self.label_runPtPlot)
        self.lineEdit_run = QtGui.QLineEdit(self.centralwidget)
        self.lineEdit_run.setObjectName(_fromUtf8("lineEdit_run"))
        self.horizontalLayout_General.addWidget(self.lineEdit_run)
        self.pushButton_load = QtGui.QPushButton(self.centralwidget)
        self.pushButton_load.setObjectName(_fromUtf8("pushButton_load"))
        self.horizontalLayout_General.addWidget(self.pushButton_load)
        self.verticalLayout_3.addLayout(self.horizontalLayout_General)
        self.gridLayout_3 = QtGui.QGridLayout()
        self.gridLayout_3.setObjectName(_fromUtf8("gridLayout_3"))
        self.graphicsView = QtGui.QGraphicsView(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.graphicsView.sizePolicy().hasHeightForWidth())
        self.graphicsView.setSizePolicy(sizePolicy)
        self.graphicsView.setObjectName(_fromUtf8("graphicsView"))
        self.gridLayout_3.addWidget(self.graphicsView, 0, 0, 1, 1)
        self.verticalLayout_5 = QtGui.QVBoxLayout()
        self.verticalLayout_5.setObjectName(_fromUtf8("verticalLayout_5"))
        self.lineEdit_ptPlot = QtGui.QLineEdit(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.lineEdit_ptPlot.sizePolicy().hasHeightForWidth())
        self.lineEdit_ptPlot.setSizePolicy(sizePolicy)
        self.lineEdit_ptPlot.setObjectName(_fromUtf8("lineEdit_ptPlot"))
        self.verticalLayout_5.addWidget(self.lineEdit_ptPlot)
        self.pushButton_plotScan = QtGui.QPushButton(self.centralwidget)
        self.pushButton_plotScan.setObjectName(_fromUtf8("pushButton_plotScan"))
        self.verticalLayout_5.addWidget(self.pushButton_plotScan)
        self.line = QtGui.QFrame(self.centralwidget)
        self.line.setFrameShape(QtGui.QFrame.HLine)
        self.line.setFrameShadow(QtGui.QFrame.Sunken)
        self.line.setObjectName(_fromUtf8("line"))
        self.verticalLayout_5.addWidget(self.line)
        self.pushButton_prevScan = QtGui.QPushButton(self.centralwidget)
        self.pushButton_prevScan.setObjectName(_fromUtf8("pushButton_prevScan"))
        self.verticalLayout_5.addWidget(self.pushButton_prevScan)
        self.label_5 = QtGui.QLabel(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.label_5.sizePolicy().hasHeightForWidth())
        self.label_5.setSizePolicy(sizePolicy)
        self.label_5.setObjectName(_fromUtf8("label_5"))
        self.verticalLayout_5.addWidget(self.label_5)
        self.pushButton_nextScan = QtGui.QPushButton(self.centralwidget)
        self.pushButton_nextScan.setObjectName(_fromUtf8("pushButton_nextScan"))
        self.verticalLayout_5.addWidget(self.pushButton_nextScan)
        spacerItem = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_5.addItem(spacerItem)
        self.gridLayout_3.addLayout(self.verticalLayout_5, 0, 3, 1, 1)
        self.verticalLayout_3.addLayout(self.gridLayout_3)
        self.horizontalLayout_Tab = QtGui.QHBoxLayout()
        self.horizontalLayout_Tab.setObjectName(_fromUtf8("horizontalLayout_Tab"))
        self.tabWidget = QtGui.QTabWidget(self.centralwidget)
        self.tabWidget.setObjectName(_fromUtf8("tabWidget"))
        self.tab = QtGui.QWidget()
        self.tab.setObjectName(_fromUtf8("tab"))
        self.verticalLayoutWidget_2 = QtGui.QWidget(self.tab)
        self.verticalLayoutWidget_2.setGeometry(QtCore.QRect(10, 10, 591, 241))
        self.verticalLayoutWidget_2.setObjectName(_fromUtf8("verticalLayoutWidget_2"))
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.verticalLayoutWidget_2)
        self.verticalLayout_2.setMargin(0)
        self.verticalLayout_2.setObjectName(_fromUtf8("verticalLayout_2"))
        self.horizontalLayout_6 = QtGui.QHBoxLayout()
        self.horizontalLayout_6.setObjectName(_fromUtf8("horizontalLayout_6"))
        self.label_2 = QtGui.QLabel(self.verticalLayoutWidget_2)
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.horizontalLayout_6.addWidget(self.label_2)
        self.lineEdit_dirSave = QtGui.QLineEdit(self.verticalLayoutWidget_2)
        self.lineEdit_dirSave.setObjectName(_fromUtf8("lineEdit_dirSave"))
        self.horizontalLayout_6.addWidget(self.lineEdit_dirSave)
        self.pushButton_browseSaveDir = QtGui.QPushButton(self.verticalLayoutWidget_2)
        self.pushButton_browseSaveDir.setObjectName(_fromUtf8("pushButton_browseSaveDir"))
        self.horizontalLayout_6.addWidget(self.pushButton_browseSaveDir)
        spacerItem1 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_6.addItem(spacerItem1)
        self.verticalLayout_2.addLayout(self.horizontalLayout_6)
        self.horizontalLayout_4 = QtGui.QHBoxLayout()
        self.horizontalLayout_4.setObjectName(_fromUtf8("horizontalLayout_4"))
        self.label_3 = QtGui.QLabel(self.verticalLayoutWidget_2)
        self.label_3.setObjectName(_fromUtf8("label_3"))
        self.horizontalLayout_4.addWidget(self.label_3)
        self.comboBox_mode = QtGui.QComboBox(self.verticalLayoutWidget_2)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.comboBox_mode.sizePolicy().hasHeightForWidth())
        self.comboBox_mode.setSizePolicy(sizePolicy)
        self.comboBox_mode.setObjectName(_fromUtf8("comboBox_mode"))
        self.comboBox_mode.addItem(_fromUtf8(""))
        self.comboBox_mode.addItem(_fromUtf8(""))
        self.horizontalLayout_4.addWidget(self.comboBox_mode)
        spacerItem2 = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_4.addItem(spacerItem2)
        self.verticalLayout_2.addLayout(self.horizontalLayout_4)
        self.tabWidget.addTab(self.tab, _fromUtf8(""))
        self.tab_advsetup = QtGui.QWidget()
        self.tab_advsetup.setObjectName(_fromUtf8("tab_advsetup"))
        self.gridLayout_4 = QtGui.QGridLayout(self.tab_advsetup)
        self.gridLayout_4.setObjectName(_fromUtf8("gridLayout_4"))
        self.verticalLayout = QtGui.QVBoxLayout()
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.label = QtGui.QLabel(self.tab_advsetup)
        self.label.setObjectName(_fromUtf8("label"))
        self.horizontalLayout.addWidget(self.label)
        self.lineEdit_url = QtGui.QLineEdit(self.tab_advsetup)
        self.lineEdit_url.setObjectName(_fromUtf8("lineEdit_url"))
        self.horizontalLayout.addWidget(self.lineEdit_url)
        self.pushButton_testURLs = QtGui.QPushButton(self.tab_advsetup)
        self.pushButton_testURLs.setObjectName(_fromUtf8("pushButton_testURLs"))
        self.horizontalLayout.addWidget(self.pushButton_testURLs)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.horizontalLayout_8 = QtGui.QHBoxLayout()
        self.horizontalLayout_8.setObjectName(_fromUtf8("horizontalLayout_8"))
        self.label_4 = QtGui.QLabel(self.tab_advsetup)
        self.label_4.setObjectName(_fromUtf8("label_4"))
        self.horizontalLayout_8.addWidget(self.label_4)
        self.lineEdit_localSrcDir = QtGui.QLineEdit(self.tab_advsetup)
        self.lineEdit_localSrcDir.setObjectName(_fromUtf8("lineEdit_localSrcDir"))
        self.horizontalLayout_8.addWidget(self.lineEdit_localSrcDir)
        self.pushButton_browseLocalData = QtGui.QPushButton(self.tab_advsetup)
        self.pushButton_browseLocalData.setObjectName(_fromUtf8("pushButton_browseLocalData"))
        self.horizontalLayout_8.addWidget(self.pushButton_browseLocalData)
        self.checkBox_dataLocal = QtGui.QCheckBox(self.tab_advsetup)
        self.checkBox_dataLocal.setText(_fromUtf8(""))
        self.checkBox_dataLocal.setObjectName(_fromUtf8("checkBox_dataLocal"))
        self.horizontalLayout_8.addWidget(self.checkBox_dataLocal)
        self.verticalLayout.addLayout(self.horizontalLayout_8)
        self.horizontalLayout_3 = QtGui.QHBoxLayout()
        self.horizontalLayout_3.setObjectName(_fromUtf8("horizontalLayout_3"))
        self.textBrowser = QtGui.QTextBrowser(self.tab_advsetup)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.textBrowser.sizePolicy().hasHeightForWidth())
        self.textBrowser.setSizePolicy(sizePolicy)
        self.textBrowser.setObjectName(_fromUtf8("textBrowser"))
        self.horizontalLayout_3.addWidget(self.textBrowser)
        self.verticalLayout.addLayout(self.horizontalLayout_3)
        self.gridLayout_4.addLayout(self.verticalLayout, 0, 0, 1, 1)
        self.tabWidget.addTab(self.tab_advsetup, _fromUtf8(""))
        self.horizontalLayout_Tab.addWidget(self.tabWidget)
        self.verticalLayout_3.addLayout(self.horizontalLayout_Tab)
        self.gridLayout.addLayout(self.verticalLayout_3, 0, 0, 1, 1)
        MainWindow.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(MainWindow)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 800, 22))
        self.menubar.setObjectName(_fromUtf8("menubar"))
        self.menuFile = QtGui.QMenu(self.menubar)
        self.menuFile.setObjectName(_fromUtf8("menuFile"))
        self.menuView = QtGui.QMenu(self.menubar)
        self.menuView.setObjectName(_fromUtf8("menuView"))
        self.menuHelp = QtGui.QMenu(self.menubar)
        self.menuHelp.setObjectName(_fromUtf8("menuHelp"))
        MainWindow.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(MainWindow)
        self.statusbar.setObjectName(_fromUtf8("statusbar"))
        MainWindow.setStatusBar(self.statusbar)
        self.dockWidget = QtGui.QDockWidget(MainWindow)
        self.dockWidget.setObjectName(_fromUtf8("dockWidget"))
        self.dockWidgetContents = QtGui.QWidget()
        self.dockWidgetContents.setObjectName(_fromUtf8("dockWidgetContents"))
        self.gridLayout_2 = QtGui.QGridLayout(self.dockWidgetContents)
        self.gridLayout_2.setObjectName(_fromUtf8("gridLayout_2"))
        self.treeWidget_scan = QtGui.QTreeWidget(self.dockWidgetContents)
        self.treeWidget_scan.setObjectName(_fromUtf8("treeWidget_scan"))
        self.treeWidget_scan.headerItem().setText(0, _fromUtf8("1"))
        self.gridLayout_2.addWidget(self.treeWidget_scan, 0, 0, 1, 1)
        self.dockWidget.setWidget(self.dockWidgetContents)
        MainWindow.addDockWidget(QtCore.Qt.DockWidgetArea(1), self.dockWidget)
        self.actionOpen = QtGui.QAction(MainWindow)
        self.actionOpen.setObjectName(_fromUtf8("actionOpen"))
        self.actionOpen_2 = QtGui.QAction(MainWindow)
        self.actionOpen_2.setObjectName(_fromUtf8("actionOpen_2"))
        self.actionSave = QtGui.QAction(MainWindow)
        self.actionSave.setObjectName(_fromUtf8("actionSave"))
        self.actionLog = QtGui.QAction(MainWindow)
        self.actionLog.setObjectName(_fromUtf8("actionLog"))
        self.menuFile.addSeparator()
        self.menuFile.addAction(self.actionOpen)
        self.menuFile.addAction(self.actionOpen_2)
        self.menuFile.addAction(self.actionSave)
        self.menuView.addAction(self.actionLog)
        self.menubar.addAction(self.menuFile.menuAction())
        self.menubar.addAction(self.menuView.menuAction())
        self.menubar.addAction(self.menuHelp.menuAction())

        self.retranslateUi(MainWindow)
        self.tabWidget.setCurrentIndex(0)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(_translate("MainWindow", "MainWindow", None))
        self.label_exp.setText(_translate("MainWindow", "Experiment", None))
        self.label_runPtPlot.setText(_translate("MainWindow", "Scan", None))
        self.pushButton_load.setText(_translate("MainWindow", "Load", None))
        self.pushButton_plotScan.setText(_translate("MainWindow", "Plot", None))
        self.pushButton_prevScan.setText(_translate("MainWindow", "Previous", None))
        self.label_5.setText(_translate("MainWindow", "[Current Pt.]", None))
        self.pushButton_nextScan.setText(_translate("MainWindow", "Next", None))
        self.label_2.setToolTip(_translate("MainWindow", "<html><head/><body><p>Directory to save the SPICE data file</p></body></html>", None))
        self.label_2.setText(_translate("MainWindow", "Location", None))
        self.pushButton_browseSaveDir.setText(_translate("MainWindow", "Browse", None))
        self.label_3.setText(_translate("MainWindow", "Mode", None))
        self.comboBox_mode.setItemText(0, _translate("MainWindow", "Download and Reduce", None))
        self.comboBox_mode.setItemText(1, _translate("MainWindow", "Download Only", None))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab), _translate("MainWindow", "Reduction", None))
        self.label.setText(_translate("MainWindow", "URL", None))
        self.pushButton_testURLs.setText(_translate("MainWindow", "Test", None))
        self.label_4.setText(_translate("MainWindow", "Local Disk", None))
        self.pushButton_browseLocalData.setText(_translate("MainWindow", "Browse", None))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab_advsetup), _translate("MainWindow", "Advanced Setup", None))
        self.menuFile.setTitle(_translate("MainWindow", "File", None))
        self.menuView.setTitle(_translate("MainWindow", "View", None))
        self.menuHelp.setTitle(_translate("MainWindow", "Help", None))
        self.actionOpen.setText(_translate("MainWindow", "New", None))
        self.actionOpen_2.setText(_translate("MainWindow", "Open", None))
        self.actionSave.setText(_translate("MainWindow", "Save", None))
        self.actionLog.setText(_translate("MainWindow", "Log", None))
        self.actionLog.setShortcut(_translate("MainWindow", "Ctrl+L", None))

