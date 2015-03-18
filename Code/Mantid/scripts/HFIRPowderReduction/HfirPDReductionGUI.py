################################################################################
#
# Main class for HFIR powder reduction GUI
#
################################################################################

#pylint: disable=invalid-name
import numpy
import sys
import os
import urllib2

from Ui_MainWindow import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui
try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

from matplotlib.pyplot import setp

# FIXME - Remove after debugging
NOMANTID = None

try:
    import mantid
    IMPORT_MANTID = True
except ImportError as e:
    sys.path.append('/home/wzz/Mantid_Project/Mantid2/Code/release/bin')
    sys.path.append('/Users/wzz/Mantid/Code/debug/bin')
    try:
        import mantid
    except ImportError as e2:
        if NOMANTID is False: 
            print "Unable to import Mantid: %s." % (str(e))
            raise e
        else:
            print "NO MANTID IS USED FOR DEBUGGING PURPOSE."
            print sys.path
            IMPORT_MANTID = False
    else:
        IMPORT_MANTID = True
        NOMANTID = True
finally:
    if IMPORT_MANTID is True:
        import mantid.simpleapi as api
        import mantid.kernel
        from mantid.simpleapi import AnalysisDataService
        from mantid.kernel import ConfigService


#----- default configuration ---------------
DEFAULT_SERVER = 'http://neutron.ornl.gov/user_data'
DEFAULT_INSTRUMENT = 'hb2a'
DEFAULT_WAVELENGTH = 2.4100
#-------------------------------------------


class MainWindow(QtGui.QMainWindow):
    """ Class of Main Window (top)

    Copy to ui.setupUI
    # Version 3.0 + Import for Ui_MainWindow.py
        from MplFigureCanvas import Qt4MplCanvas

        # Replace 'self.graphicsView = QtGui.QtGraphicsView' with the following
        self.graphicsView = Qt4MplCanvas(self.centralwidget)
        self.mainplot = self.graphicsView.getPlot()

    """
    def __init__(self, parent=None):
        """ Intialization and set up
        """
        # Base class
        QtGui.QMainWindow.__init__(self,parent)

        # UI Window (from Qt Designer)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # Define gui-event handling 
        self.connect(self.ui.pushButton_loadData, QtCore.SIGNAL('clicked()'), 
                self.doLoadData)

        self.connect(self.ui.pushButton_prevScan, QtCore.SIGNAL('clicked()'),
                self.doLoadPrevScan)

        self.connect(self.ui.pushButton_nextScan, QtCore.SIGNAL('clicked()'),
                self.doLoadNextScan)

        self.connect(self.ui.pushButton_unit2theta, QtCore.SIGNAL('clicked()'),
                self.doPlot2Theta)

        self.connect(self.ui.pushButton_unitD, QtCore.SIGNAL('clicked()'),
                self.doPlotDspacing)

        self.connect(self.ui.pushButton_unitQ, QtCore.SIGNAL('clicked()'),
                self.doPlotQ)

        self.connect(self.ui.pushButton_saveData, QtCore.SIGNAL('clicked()'),
                self.doSaveData)

        self.connect(self.ui.pushButton_browseCache, QtCore.SIGNAL('clicked()'),
                self.doBrowseCache)

        self.connect(self.ui.pushButton_plotRaw, QtCore.SIGNAL('clicked()'),
                self.doPlotCurrentRawDet)

        self.connect(self.ui.pushButton_ptUp, QtCore.SIGNAL('clicked()'),
                self.doPlotPrevPtRaw)

        self.connect(self.ui.pushButton_ptDown, QtCore.SIGNAL('clicked()'),
                self.doPlotNextPtRaw)

        self.connect(self.ui.pushButton_detUp, QtCore.SIGNAL('clicked()'),
                self.doPlotPrevDetRaw)

        self.connect(self.ui.pushButton_detDown, QtCore.SIGNAL('clicked()'),
                self.doPlotNextDetRaw)
                
        self.connect(self.ui.radioButton_useServer, QtCore.SIGNAL('clicked()'),
                self.doChangeSrcLocation)
                
        self.connect(self.ui.radioButton_useLocal, QtCore.SIGNAL('clicked()'),
                self.doChangeSrcLocation)
                
        self.connect(self.ui.pushButton_browseLocalSrc, QtCore.SIGNAL('clicked()'),
                self.doBrowseLocalDataSrc)
                
        self.connect(self.ui.pushButton_chkServer, QtCore.SIGNAL('clicked()'),
                self.doCheckSrcServer)
                
        self.connect(self.ui.comboBox_wavelength, QtCore.SIGNAL('currentIndexChanged(int)'),
                self.doUpdateWavelength)

        self.connect(self.ui.actionQuit, QtCore.SIGNAL('triggered()'),
                self.doExist)


        # TODO - Add event handling
        """
        pushButton_stripVanPeaks
        pushButton_saveVanRun

        """


        # Define signal-event handling


        # Widget type definition
        validator0 = QtGui.QIntValidator(self.ui.lineEdit_expNo)
        validator0.setBottom(1)
        self.ui.lineEdit_expNo.setValidator(validator0)

        validator1 = QtGui.QIntValidator(self.ui.lineEdit_expNo)
        validator1.setBottom(1)
        self.ui.lineEdit_scanNo.setValidator(validator1)

        validator2 = QtGui.QDoubleValidator(self.ui.lineEdit_wavelength)
        validator2.setBottom(0.)
        self.ui.lineEdit_wavelength.setValidator(validator2)

        validator3 = QtGui.QDoubleValidator(self.ui.lineEdit_xmin)
        validator3.setBottom(0.)
        self.ui.lineEdit_xmin.setValidator(validator3)

        validator4 = QtGui.QDoubleValidator(self.ui.lineEdit_xmax)
        validator4.setBottom(0.)
        self.ui.lineEdit_xmax.setValidator(validator4)

        validator5 = QtGui.QDoubleValidator(self.ui.lineEdit_binsize)
        validator5.setBottom(0.)
        self.ui.lineEdit_binsize.setValidator(validator5)

        validator6 = QtGui.QDoubleValidator(self.ui.lineEdit_ptNo)
        validator6.setBottom(0)
        self.ui.lineEdit_ptNo.setValidator(validator6)

        validator7 = QtGui.QDoubleValidator(self.ui.lineEdit_detNo)
        validator7.setBottom(0)
        self.ui.lineEdit_detNo.setValidator(validator7)

        # TODO - Add valdiators
        """ 
        lineEdit_minD 
        lineEdit_maxD
        lineEdit_binsizeD 
        pushButton_rebinD
        lineEdit_scanStart
        """
        # Get initial setup
        self._initSetup()

        return


    def _initSetup(self):
        """ Initial setup
        """
        # FIXME - This part will be implemented soon as default configuration is made
        # Mantid configuration
        self._instrument = str(self.ui.comboBox_instrument.currentText())
        #if IMPORT_MANTID is True:
        #    config = ConfigService.Instance()
        #    self._instrument = config["default.instrument"]
        #else:
        #    self._instrument = DEFAULT_INSTRUMENT

        # UI widgets setup
        self.ui.comboBox_outputFormat.addItems(['Fullprof', 'GSAS', 'Fullprof+GSAS'])

        # FIXME : Need to disable some widgets... consider to refactor the code
        self.ui.radioButton_useServer.setChecked(True)
        self.ui.radioButton_useLocal.setChecked(False)
        
        self.ui.comboBox_wavelength.setCurrentIndex(0)
        self.ui.lineEdit_wavelength.setText('2.41')

        self.ui.pushButton_unit2theta.setText(r'$2\theta$')

        # Set up data source
        self._serverAddress = DEFAULT_SERVER 
        self._srcFromServer = True
        self._localSrcDataDir = None
        self._srcAtLocal = False

        self._currUnit = '2theta'

        # Workspaces
        self._myReducedPDWs  = None
        self._prevoutws = None

        self._ptNo = None
        self._detNo = None
        
        # set up for plotting
        self._myLineMarkerColorList = self.ui.graphicsView_reducedData.getDefaultColorMarkerComboList()
        self._myLineMarkerColorIndex = 0

        # workspaces
        self._myDataMDWS = None
        self._myMonitorMDWS = None

        # State machine
        # self._inPlotState = False


        return


    #-- Event Handling ----------------------------------------------------

    def doBrowseCache(self):
        """ Pop out a dialog to let user specify the directory to
        cache downloaded data
        """
        # home directory
        homedir = str(self.ui.lineEdit_cache.text()).strip()
        if len(homedir) > 0 and os.path.exists(homedir):
            home = homedir
        else:
            home = os.getcwd()

        # pop out a dialog
        dirs = str(QtGui.QFileDialog.getExistingDirectory(self,'Get Directory',home))

        # set to line edit
        if dirs != home:
            self.ui.lineEdit_cache.setText(dirs)

        return
        
    def doBrowseLocalDataSrc(self):
        """ Browse local data storage
        """
        print "Browse local data storage location."
        
        return
        
        
    def doChangeSrcLocation(self):
        """ Source file location is changed
        """
        useserver = self.ui.radioButton_useServer.isChecked()
        uselocal = self.ui.radioButton_useLocal.isChecked()
        
        print "Use Server: ", useserver
        print "Use Local : ", uselocal
        
        if (useserver is True and uselocal is True) or \
            (useserver is False and uselocal is False):
            raise NotImplementedError("Impossible for radio buttons")
        
        self._srcAtLocal = uselocal
        self._srcFromServer = useserver
        
        if uselocal is True:
            self.ui.lineEdit_dataIP.setDisabled(True)
            self.ui.pushButton_chkServer.setDisabled(True)
            self.ui.lineEdit_localSrc.setDisabled(False)
            self.ui.pushButton_browseLocalSrc.setDisabled(False)
            
        else:
            self.ui.lineEdit_dataIP.setDisabled(False)
            self.ui.pushButton_chkServer.setDisabled(False)
            self.ui.lineEdit_localSrc.setDisabled(True)
            self.ui.pushButton_browseLocalSrc.setDisabled(True)
            
        return     
        
        
    def doCheckSrcServer(self):
        """" Check source data server's availability
        """
        
        print "Check source data server!"
        
        return

    def doLoadData(self):
        """ Load data 
        """
        # Get information
        try:
            expno = int(self.ui.lineEdit_expNo.text())
            scanno = int(self.ui.lineEdit_scanNo.text())
        except ValueError:
            self._logError("Either Exp No or Scan No is not set up right as integer.")
            return

        self._logDebug("Attending to load Exp %d Scan %d." % (expno, scanno))

        # Form data file name and download data
        datafilename = self._uiLoadDataFile(exp=expno, scan=scanno)

        # Get other information
        xmin, xmax, binsize = self._getBinningParams()
        if binsize is None:
            self._logError("Bin size must be specified.")

        unit = self._currUnit
        wavelength = float(self.ui.lineEdit_wavelength.text())

        # Reduce data
        execstatus = self._reduceSpicePDData(datafilename, unit, xmin, xmax, binsize, wavelength)
        print "[DB] reduction status = ", execstatus

        # Plot data
        
        clearcanvas = self.ui.checkBox_clearPrevious.isChecked() 
        if execstatus is True:
            self._plotReducedData(self._currUnit, 0, clearcanvas)

        return execstatus


    def doLoadPrevScan(self):
        """
        """
        # Advance scan number by 1
        try:
            scanno = int(self.ui.lineEdit_scanNo.text())
        except ValueError:
            self._logError("Either Exp No or Scan No is not set up right as integer.")
            return
        else:
            scanno = scanno - 1
            if scanno < 1:
                self._logWarning("Scan number is 1 already.  Cannot have previous scan")
                return
            self.ui.lineEdit_scanNo.setText(str(scanno))

        # call load data
        execstatus = self.doLoadData()

        return execstatus


    def doLoadNextScan(self):
        """
        """
        # Advance scan number by 1
        try:
            scanno = int(self.ui.lineEdit_scanNo.text())
        except ValueError:
            self._logError("Either Exp No or Scan No is not set up right as integer.")
            return
        else:
            scanno = scanno + 1
            if scanno < 1:
                self._logWarning("Scan number is 1 already.  Cannot have previous scan")
                return
            self.ui.lineEdit_scanNo.setText(str(scanno))

        # call load data
        execstatus = self.doLoadData()
        if execstatus is False:
            scanno = scanno - 1
            self.ui.lineEdit_scanNo.setText(str(scanno))

        return execstatus


    def doExist(self):
        """ Exist the application
        """
        clearcache = self.ui.checkBox_delCache.isChecked()

        if clearcache is True:
            # TODO - Clear cache
            print "Clear Cache!"

        self.close()

        return


    def doPlot2Theta(self):
        """ Rebin the data and plot in 2theta
        """
        # check binning parameters and target unit
        change, xmin, xmax, binsize = self._uiCheckBinningParameters(self._myMinX, self._myMaxX, 
            self._myBinSize, self._myCurrentUnit, "2theta")

        print "[DB] Rebin is required: ", change
        
        # no change,  return with a notice
        if change is False:
            self._logDebug("All binning parameters and target unit are same as current values. No need to plot again.")
            return
            
        # Rebin
        self._rebin('2theta', xmin, binsize, xmax)
        
        xlabel = r'$2\theta$' 
        self._plotReducedData(xlabel, 0, True)

        return

    def doPlotDspacing(self):
        """ Rebin the data and plot in d-spacing
        """
        # check binning parameters and target unit
        change, xmin, xmax, binsize = self._uiCheckBinningParameters(self._myMinX, self._myMaxX, 
            self._myBinSize, self._myCurrentUnit, "dSpacing")
        
        # no change,  return with a notice
        if change is False:
            self._logDebug("All binning parameters and target unit are same as current values. No need to plot again.")
            return
            
        # Rebin
        self._rebin('dSpacing', xmin, binsize, xmax)
        xlabel = r"$d (\AA)$"
        self._plotReducedData(xlabel, 0, True)

        return

    def doPlotQ(self):
        """ Rebin the data and plot in momentum transfer Q
        """
        # check binning parameters and target unit
        change, xmin, xmax, binsize = self._uiCheckBinningParameters(self._myMinX, self._myMaxX, 
            self._myBinSize, self._myCurrentUnit, "Momentum Transfer (Q)")
        
        # no change,  return with a notice
        if change is False:
            self._logDebug("All binning parameters and target unit are same as current values. No need to plot again.")
            return
            
        # Rebin
        self._rebin('Momentum Transfer (Q)', xmin, binsize, xmax)
        xlabel = r"$Q \AA^{-1}$"
        self._plotReducedData(xlabel, 0, True)  

        return

    def doPlotCurrentRawDet(self):
        """ Plot current raw detector signals
        """
        ptstr = str(self.ui.lineEdit_ptNo.text())
        detstr = str(self.ui.lineEdit_detNo.text())
        if len(ptstr) == 0 or len(detstr) == 0:
            self._logError("Neither Pt. nor detector ID can be left blank to plot raw detector signal.")
            return

        ptno = int(ptstr)
        detno = int(detstr)

        status, errmsg = self._checkValidPtDet(ptno, detno) 
        if status is False:
            self._logError(errmsg)

        self._ptNo = ptno
        self._detNo = detno

        self._plotRawDetSignal(self._ptNo, self._detNo)

        return

    def doPlotPrevDetRaw(self):
        """ Plot previous raw detector
        """
        # check
        if self._ptNo is not None and self._detNo is not None:
            detno = self._detNo + 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return

        # det number minus 1
        status, errmsg = self._checkValidPtDet(self._ptNo, detno)
        if status is False:
            self._logError(errmsg)
        else:
            self._detNo = detno
            self.ui.lineEdit_detNo.setText(str(self._detNo))

        # plot
        self._plotRawDetSignal(self._ptNo, self._detNo)

        return


    def doPlotNextDetRaw(self):
        """ Plot next raw detector signals
        """
        # check
        if self._ptNo is not None and self._detNo is not None:
            detno = self._detNo + 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return

        # det number minus 1
        status, errmsg = self._checkValidPtDet(self._ptNo, detno)
        if status is False:
            self._logError(errmsg)
        else:
            self._detNo = detno
            self.ui.lineEdit_detNo.setText(str(self._detNo))

        # plot
        self._plotRawDetSignal(self._ptNo, self._detNo)

        return


    def doPlotPrevPtRaw(self):
        """ Plot previous raw detector
        """
        # check
        if self._ptNo is not None and self._detNo is not None:
            ptno = self._ptNo - 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return

        # det number minus 1
        status, errmsg = self._checkValidPtDet(self._ptNo, detno)
        if status is False:
            self._logError(errmsg)
        else:
            self._ptNo = ptno
            self.ui.lineEdit_ptNo.setText(str(self._ptNo))

        # plot
        self._plotRawDetSignal(self._ptNo, self._detNo)

        return


    def doPlotNextPtRaw(self):
        """ Plot next raw detector signals
        """
        # check
        if self._ptNo is not None and self._detNo is not None:
            ptno = self._ptN + 1
        else:
            self._logError("Unable to plot previous raw detector \
                    because Pt. or Detector ID has not been set up yet.")
            return

        # det number minus 1
        status, errmsg = self._checkValidPtDet(self._ptNo, detno)
        if status is False:
            self._logError(errmsg)
        else:
            self._ptNo = ptno
            self.ui.lineEdit_ptNo.setText(str(self._ptNo))

        # plot
        self._plotRawDetSignal(self._ptNo, self._detNo)

        return


    def doSaveData(self):
        """ Save data
        """
        # check whether it is fine to save 
        if self._myReducedPDWs is None:
            self._logError("No reduced diffraction data to save.")
            #return

        # file type
        filetype = str(self.ui.comboBox_outputFormat.currentText())

        # get line edit for save data location
        savedatadir = str(self.ui.lineEdit_outputFileName.text()).strip()
        if savedatadir != None and os.path.exists(savedatadir) is True:
            homedir = savedatadir
        else:
            homedir = os.getcwd()

        # launch a dialog to get data
        filter = "All files (*.*);;Fullprof (*.dat);;GSAS (*.gsa)"
        sfilename = str(QtGui.QFileDialog.getSaveFileName(self, 'Save File', homedir, filter))

        print "Get file name: ", sfilename
   
        # save
        # FIXME - ASAP
        if filetype.lower().count("fullprof") == 1:
            print "going to save for Fullprof"

        if filetype.lower().count("gsas") == 1:
            print "going to save GSAS"

        return

    def doUpdateWavelength(self):
        """ Update the wavelength to line edit
        """
        index = self.ui.comboBox_wavelength.currentIndex()
        
        print "Update wavelength to ", index
        
        if index == 0:
            wavelength = 2.41
        elif index == 1:
            wavelength = 1.54
        elif index == 2:
            wavelength = 1.12
        else:
            wavelength = None
            
        self.ui.lineEdit_wavelength.setText(str(wavelength))
        
        return


    #--------------------------------------------------------
    #
    #--------------------------------------------------------

    def _getBinningParams(self):
        """ Get binning parameters
        """
        xminstr = str(self.ui.lineEdit_xmin.text()).strip()
        if len(xminstr) == 0:
            xmin = None
        else:
            xmin = float(xminstr)

        xmaxstr = str(self.ui.lineEdit_xmax.text()).strip()
        if len(xmaxstr) == 0:
            xmax = None
        else:
            xmax = float(xmaxstr)

        binsizestr = str(self.ui.lineEdit_binsize.text()).strip()
        if len(binsizestr) == 0:
            binsize = None
        else:
            binsize = float(binsizestr)

        return xmin, xmax, binsize

    
    def _uiLoadDataFile(self, exp, scan):
        """ Load data file according to its exp and scan 
        Either download the data from a server or copy the data file from local 
        disk
        """
        # Get on hold of raw data file
        useserver = self.ui.radioButton_useServer.isChecked()
        uselocal = self.ui.radioButton_useLocal.isChecked()
        if (useserver and uselocal) is False:
            self._logError("It is logically wrong to set up server/local dir for data.")
            useserver = True
            uselocal = False
            self.ui.radioButton_useServer.setChecked(True)
            self.ui.radioButton_useLocal.setChecked(False)
        
        
        if self._srcFromServer is True:
            # Use server: build the URl to download data
            if self._serverAddress.endswith('/') is False:
                self._serverAddress += '/'
            fullurl = "%s%s/exp%d/Datafiles/%s_exp%04d_scan%04d.dat" % (self._serverAddress,
                    self._instrument.lower(), exp, self._instrument.upper(), exp, scan)
            print "URL: ", fullurl
    
            cachedir = str(self.ui.lineEdit_cache.text()).strip()
            if os.path.exists(cachedir) is False:
                self._logError("Cache directory is not valid.")
                return
    
            filename = '%s_exp%04d_scan%04d.dat' % (self._instrument.upper(), exp, scan)
            self._srcFileName = os.path.join(cachedir, filename)
            status, errmsg = self._downloadFile(fullurl, self._srcFileName)
            if status is False:
                self._logError(errmsg)
                self._srcFileName = None

        elif self._srcAtLocal is True:
            # Data from local
            self._srcFileName = os.path.join(self._localSrcDataDir, "%s/Exp%d_Scan%04d.dat" % (self._instrument, exp, scan))

        else:
            raise NotImplementedError("XXXXXX")

        return self._srcFileName


    def _reduceSpicePDData(self, datafilename, unit, xmin, xmax, binsize, wavelength):
        """ Reduce SPICE powder diffraction data. 
        """
        # cache the previous one
        self._prevoutws = self._myReducedPDWs
        self._myReducedPDWs = None

        # Rebin
        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        # base workspace name
        print "base workspace name: ", datafilename
        basewsname = os.path.basename(datafilename).split(".")[0]

        # load SPICE
        tablewsname = basewsname + "_RawTable"
        infowsname  = basewsname + "ExpInfo"
        api.LoadSpiceAscii(Filename=datafilename, 
                OutputWorkspace=tablewsname, RunInfoWorkspace=infowsname)

        # Build MDWorkspace
        datamdwsname = basewsname + "_DataMD"
        monitorwsname = basewsname + "_MonitorMD"
        api.ConvertSpiceDataToRealSpace(InputWorkspace=tablewsname,
                RunInfoWorkspace=infowsname,
                OutputWorkspace=datamdwsname,
                OutputMonitorWorkspace=monitorwsname)

        self._myDataMDWS = AnalysisDataService.retrieve(datamdwsname)
        self._myMonitorMDWS = AnalysisDataService.retrieve(monitorwsname)

        # Rebin
        if xmin is None or xmax is None:
            binpar = "%.7f" % (binsize)
        else:
            binpar = "%.7f, %.7f, %.7f" % (xmin, binsize, xmax)

        outwsname = basewsname + "_Reduced_" + unit
        api.ConvertCWPDMDToSpectra(InputWorkspace=datamdwsname,
                InputMonitorWorkspace=monitorwsname,
                OutputWorkspace=outwsname,
                BinningParams=binpar,
                UnitOutput = unit, 
                NeutronWaveLength=wavelength)

        print "[DB] Reduction is finished.  Data is in workspace %s. " % (datamdwsname)

        # Set up class variable for min/max and 
        outws = AnalysisDataService.retrieve(outwsname)
        if outws is not None:
            self._myReducedPDWs = outws

        self._myMinX = self._myReducedPDWs.readX(0)[0]
        self._myMaxX = self._myReducedPDWs.readX(0)[-1]
        self._myBinSize = binsize
        self._myCurrentUnit = unit
        
        return True


    def _plotReducedData(self, targetunit, spectrum, clearcanvas):
        """ Plot reduced data stored in self._reducedWS to 
        """
        # print "[DB] Plot reduced data!", "_inPlotState = ", str(self._inPlotState)

        # whether the data is load?
        if self._myReducedPDWs is None:
            self._logWarning("No data to plot!")
            return
            
        # get to know whether it is required to clear the image
        if clearcanvas is True:
            self.ui.graphicsView_reducedData.clearAllLines()
            self._myLineMarkerColorIndex = 0
            
        # plot
        # FIXME - Should not modify the original workspace
        wsname = str(self._myReducedPDWs)
        api.ConvertToPointData(InputWorkspace=self._myReducedPDWs, OutputWorkspace=wsname)
        self._myReducedPDWs = AnalysisDataService.retrieve(wsname)
        
        # get the marker color for the line
        marker, color = self._myLineMarkerColorList[self._myLineMarkerColorIndex]
        if marker.count(' (') > 0:
            marker = marker.split(' (')[0]
        print "[DB] Print line %d: marker = %s, color = %s" % (self._myLineMarkerColorIndex, marker, color)
        self._myLineMarkerColorIndex += 1
        
        self.ui.graphicsView_reducedData.addPlot(self._myReducedPDWs.readX(spectrum),
            self._myReducedPDWs.readY(spectrum), marker=marker, color=color,ylabel='intensity',
            xlabel=targetunit,label=str(self._myReducedPDWs))
            
        return
        
     
    def _uiCheckBinningParameters(self, curxmin=None, curxmax=None, curbinsize=None, curunit=None, targetunit=None):
        """ check the binning parameters including xmin, xmax, bin size and target unit
        
        Return: True or false
        """
        # get value
        xmin = str(self.ui.lineEdit_xmin.text())
        xmax = str(self.ui.lineEdit_xmax.text())
        binsize = str(self.ui.lineEdit_binsize.text())

        change = False        
        # check x-min
        if len(xmin) > 0:
            xmin = float(xmin)
            if ( (self._myMinX is None) or (self._myMinX is not None and abs(xmin-self._myMinX) > 1.0E-5) ):
                change = True
        else:
            xmin = None

        # check x-max
        if len(xmax) > 0: 
            xmax = float(xmax)
            if ( (self._myMaxX is None) or (self._myMaxX is not None and 
                abs(xmax-self._myMaxX) > 1.0E-5) ):
                change = True
        else:
            xmax = None
        
        # check binsize
        if len(binsize) > 0: 
            binsize = float(binsize)
            if ( (self._myBinSize is None) or (self._myBinSize is not None and 
                abs(binsize-self._myBinSize) > 1.0E-5) ):
                change = True
        else:
            binsize = None
        
        # whether the unit should be changed or bin be changed?
        if curunit != targetunit:
            change = True
            
        return (change, xmin, xmax, binsize)


    def _rebin(self, unit, xmin, binsize, xmax):
        """ Rebin the data MD workspace and monitor MD workspace for new bin parameter and/or 
        units
        """
        if self._myDataMDWS is None or self._myMonitorMDWS is None:
            self._logError("Unable to rebin the data because either data MD workspace and \
                monitor MD workspace is not present.")
            return

        try: 
            wavelength = float(self.ui.lineEdit_wavelength.text())
        except ValueError as e:
            print e
            return
       
        reducedwsname = self._myDataMDWS.name() + "_" + unit
        api.ConvertCWPDMDToSpectra(InputWorkspace=self._myDataMDWS,
            InputMonitorWorkspace=self._myMonitorMDWS,
            OutputWorkspace=reducedwsname,
            UnitOutput=unit,
            BinningParams = "%f, %f, %f" % (xmin, binsize, xmax),
            NeutronWaveLength=wavelength)
        outws = AnalysisDataService.retrieve(reducedwsname)
        if outws is not None: 
            self._myReducedPDWs = outws
        else:
            raise NotImplementedError("Failed to convert unit to %s." % (unit))
        
        return 


    def _excludeDetectors(self, detids):
        """
        """
        # TODO 

        return


    def _excludePt(self, pts):
        """
        """ 
        # TODO

        return




    def _logDebug(self, dbinfo):
        """ Log debug information
        """
        print dbinfo


    def _logError(self, errinfo):
        """ Log error information
        """
        print "Log(Error): %s" % (errinfo)


    def _logWarning(self, errinfo):
        """ Log error information
        """
        print "Log(Warning): %s" % (errinfo)


    def _downloadFile(self, url, localfilepath):
        """
        Test: 'http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400_scan0001.dat'
        """
        # open URL
        response = urllib2.urlopen(url)
        wbuf = response.read()

        if wbuf.count('not found') > 0:
            return (False, "File cannot be found at %s." % (url))

        ofile = open(localfilepath, 'w')
        ofile.write(wbuf)
        ofile.close()

        return (True, "")


    def _plotRawDetSignal(self, ptno, detno):
        """ Retrieve and plot raw detector signal 
        """
        # TODO - Implement ASAP

