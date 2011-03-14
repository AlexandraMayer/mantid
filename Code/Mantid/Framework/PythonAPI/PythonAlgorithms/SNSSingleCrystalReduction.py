from MantidFramework import *
from mantidsimple import *
import os

COMPRESS_TOL_TOF = .01

class SNSSingleCrystalReduction(PythonAlgorithm):
    def category(self):
        return "Diffraction"

    def name(self):
        return "SNSSingleCrystalReduction"

    def PyInit(self):
        instruments = ["TOPAZ"]
        self.declareProperty("Instrument", "TOPAZ",
                             Validator=ListValidator(instruments))
        #types = ["Event preNeXus", "Event NeXus"]
        #self.declareProperty("FileType", "Event NeXus",
        #                     Validator=ListValidator(types))
        self.declareProperty("RunNumber", 0, Validator=BoundedValidator(Lower=0))
        self.declareProperty("CompressOnRead", False,
                             Description="Compress the event list when reading in the data")
        self.declareProperty("BackgroundNumber", 0, Validator=BoundedValidator(Lower=0))
        self.declareProperty("EmptyInstrumentNumber", 0, Validator=BoundedValidator(Lower=0))
        self.declareProperty("VanadiumNumber", 0, Validator=BoundedValidator(Lower=0))
        self.declareProperty("FilterByTimeMin", 0.,
                             Description="Relative time to start filtering by in seconds. Applies only to sample.")
        self.declareProperty("FilterByTimeMax", 0.,
                             Description="Relative time to stop filtering by in seconds. Applies only to sample.")
        self.declareProperty("TOFBinParams", "1000.0,-0.004,16666.0",
                             Description="Binning parameters to use in TOF.")
        self.declareProperty("dspaceBinParams", "0.2,-0.004,10.00",
                             Description="Binning parameters to use in d-space when calculating the vanadium spectra.")
        
        self.declareProperty("VanadiumPeakWidthPercentage", 5.)
        self.declareProperty("VanadiumSmoothNumPoints", 11)
        self.declareProperty("FilterBadPulses", True, Description="Filter out events measured while proton charge is more than 5% below average")
        outfiletypes = ['nxs'] #, 'gsas', 'fullprof', 'gsas and fullprof']
        self.declareProperty("FilterByLogValue", "", Description="Name of log value to filter by")
        self.declareProperty("FilterMinimumValue", 0.0, Description="Minimum log value for which to keep events.")
        self.declareProperty("FilterMaximumValue", 0.0, Description="Maximum log value for which to keep events.")
        self.declareProperty("SaveAs", "nxs", ListValidator(outfiletypes))
        self.declareFileProperty("OutputDirectory", "", FileAction.Directory)

    def _findData(self, runnumber, extension):
        #self.log().information(str(dir()))
        #self.log().information(str(dir(mantidsimple)))
        result = FindSNSNeXus(Instrument=self._instrument,
                              RunNumber=runnumber, Extension=extension)
#        result = self.executeSubAlg("FindSNSNeXus", Instrument=self._instrument,
#                                    RunNumber=runnumber, Extension=extension)
        return result["ResultPath"].value

    def _loadPreNeXusData(self, runnumber, extension):
        # find the file to load
        filename = self._findData(runnumber, extension)

        # generate the workspace name
        (path, name) = os.path.split(filename)
        name = name.split('.')[0]
        (name, num) = name.split('_neutron')
        num = num.replace('_event', '') # TODO should do something with this

        # load the prenexus file
        alg = LoadEventPreNeXus(EventFilename=filename, OutputWorkspace=name)
        wksp = alg['OutputWorkspace']

        # add the logs to it
        nxsfile = self._findData(runnumber, ".nxs")
        LoadLogsFromSNSNexus(Workspace=wksp, Filename=nxsfile)
        # TODO filter out events using timemin and timemax

        return wksp

    def _loadNeXusData(self, runnumber, extension, **kwargs):
        if self.getProperty("CompressOnRead"):
            kwargs["CompressTolerance"] = COMPRESS_TOL_TOF
        else:
            kwargs["Precount"] = True
        name = "%s_%d" % (self._instrument, runnumber)
        filename = name + extension

        try: # first just try loading the file
            alg = LoadEventNexus(Filename=filename, OutputWorkspace=name, **kwargs)

            return alg.workspace()
        except:
            pass

        # find the file to load
        filename = self._findData(runnumber, extension)

        # generate the workspace name
        (path, name) = os.path.split(filename)
        name = name.split('.')[0] # remove the extension
        if "_event" in name:
            name = name[0:-1*len("_event")]

        # TODO use timemin and timemax to filter what events are being read
        alg = LoadEventNexus(Filename=filename, OutputWorkspace=name, **kwargs)

        return alg.workspace()

    def _loadData(self, runnumber, extension, filterWall=None):
        filter = {}
        if filterWall is not None:
            if filterWall[0] > 0.:
                filter["FilterByTimeStart"] = filterWall[0]
            if filterWall[1] > 0.:
                filter["FilterByTimeStop"] = filterWall[1]

        if  runnumber is None or runnumber <= 0:
            return None

        if extension.endswith(".nxs"):
            return self._loadNeXusData(runnumber, extension, **filter)
        else:
            return self._loadPreNeXusData(runnumber, extension)



    def _comp(self, wksp, filterLogs=None):
        if wksp is None:
            return None
        # take care of filtering events
        if self._filterBadPulses and not self.getProperty("CompressOnRead"):
            FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp)
        if filterLogs is not None:
            try:
                logparam = wksp.getRun()[filterLogs[0]]
                if logparam is not None:
                    FilterByLogValue(InputWorkspace=wksp, OutputWorkspace=wksp, LogName=filterLogs[0],
                                     MinimumValue=filterLogs[1], MaximumValue=filterLogs[2])
            except KeyError, e:
                raise RuntimeError("Failed to find log '%s' in workspace '%s'" \
                                   % (filterLogs[0], str(wksp)))
        
        Sort(InputWorkspace=wksp, SortBy="Time of Flight")

        # Compress the events to free up memory
        if not self.getProperty("CompressOnRead"):
            CompressEvents(InputWorkspace=wksp, OutputWorkspace=wksp, Tolerance=COMPRESS_TOL_TOF) # 100ns

        return wksp

        
    def _bin(self, wksp, filterLogs=None):
        # Rebin in place as weighted events
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=self._TOFBinParams)

        return wksp




    def _vanbin(self, wksp, filterLogs=None):
        if wksp is None:
            return None
        
        groups = ""
        numrange = 200
        for num in xrange(1,numrange):
            comp = wksp.getInstrument().getComponentByName("bank%d" % (num) )
            if not comp == None:
               groups+=("bank%d," % (num) )
        print groups
        
        # take care of filtering events
        if self._filterBadPulses:
            pcharge = wksp.getRun()['proton_charge']
            pcharge = pcharge.getStatistics().mean
            FilterByLogValue(InputWorkspace=wksp, OutputWorkspace=wksp, LogName="proton_charge",
                             MinimumValue=.95*pcharge, MaximumValue=2.*pcharge)
        if filterLogs is not None:
            try:
                logparam = wksp.getRun()[filterLogs[0]]
                if logparam is not None:
                    FilterByLogValue(InputWorkspace=wksp, OutputWorkspace=wksp, LogName=filterLogs[0],
                                     MinimumValue=filterLogs[1], MaximumValue=filterLogs[2])
            except KeyError, e:
                raise RuntimeError("Failed to find log '%s' in workspace '%s'" \
                                   % (filterLogs[0], str(wksp)))
        

        # Sorting at this point will increase speed a lot
        Sort(InputWorkspace=wksp, SortBy="Time of Flight")
        
        # Remove old calibration files
        cmd = "rm temp.cal*"
        os.system(cmd)
        CreateCalFileByNames(wksp, "temp.cal", groups)
        # Align detectors using new calibration file with offsets
        AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp,CalibrationFile="temp.cal")
        
        # Diffraction focusing using new calibration file with offsets
        DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp,GroupingFileName="temp.cal")
        
        # This will rebin into a workspace 2D
        oldName = wksp.getName()
        Rebin(InputWorkspace=wksp, OutputWorkspace="temp", Params=self._dspaceBinParams)
        # Delete the old event workspace, free up memory hopefully
        DeleteWorkspace(wksp)
        # Go back to the original name, but as a workspace 2D from now on
        wksp = RenameWorkspace(InputWorkspace="temp", OutputWorkspace=oldName).workspace()
        
        # This will make a Workspace2D out of the EventWorkspace
        StripVanadiumPeaks(InputWorkspace=wksp, OutputWorkspace=wksp, PeakWidthPercent=self._vanPeakWidthPercent)
        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="TOF")
        
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=self._TOFBinParams)
        SmoothData(InputWorkspace=wksp, OutputWorkspace=wksp, NPoints=self._vanSmoothPoints)
        # Normalize to one pixel of detector (256 X 256)
        wksp /= 65536.0

        return wksp

    def _save(self, wksp, normalized):
        filename = os.path.join(self._outDir, str(wksp))
        name = "%s_%d" % (self._instrument, self.getProperty("RunNumber"))
        nxsfile = name + ".nxs"

        if not os.path.isfile(nxsfile):
            nxsfile = self._findData(self.getProperty("RunNumber"), ".nxs")
        self.log().information(nxsfile)
        if "nxs" in self._outTypes:
            SaveSNSNexus(InputFilename=nxsfile,InputWorkspace=wksp, OutputFilename=filename+"_mantid.nxs", Compress=True)
        if "gsas" in self._outTypes:
            ReplaceSpecialValues(wksp, wksp, NaNValue="0.0", InfinityValue="0.0")
            SaveGSS(InputWorkspace=wksp, Filename=filename+".gsa", SplitFiles="False", Append=False, MultiplyByTOFBinWidth=normalized, Bank=self._bank, Format="SLOG")
        if "fullprof" in self._outTypes:
            ReplaceSpecialValues(wksp, wksp, NaNValue="0.0", InfinityValue="0.0")
            SaveFocusedXYE(InputWorkspace=wksp, Filename=filename+".dat")

    def PyExec(self):
        # temporary hack for getting python algorithms working
        import mantidsimple
        reload(mantidsimple)
        globals()["FindSNSNeXus"] = mantidsimple.FindSNSNeXus

        # get generic information
        SUFFIX = "_event.nxs"
        self._instrument = self.getProperty("Instrument")
        self._bank = 0 #self.getProperty("BankNumber")
        self._TOFBinParams = self.getProperty("TOFBinParams")
        self._dspaceBinParams = self.getProperty("dspaceBinParams")
        self._filterBadPulses = self.getProperty("FilterBadPulses")
        filterLogs = self.getProperty("FilterByLogValue")
        if len(filterLogs.strip()) <= 0:
            filterLogs = None
        else:
            filterLogs = [filterLogs, 
                          self.getProperty("FilterMinimumValue"), self.getProperty("FilterMaximumValue")]
        self._vanPeakWidthPercent = self.getProperty("VanadiumPeakWidthPercentage")
        self._vanSmoothPoints = self.getProperty("VanadiumSmoothNumPoints")
        self._outDir = self.getProperty("OutputDirectory")
        self._outTypes = self.getProperty("SaveAs")
        sam = self.getProperty("RunNumber")
        bkg = self.getProperty("BackgroundNumber")
        van = self.getProperty("VanadiumNumber")
        empty = self.getProperty("EmptyInstrumentNumber")
        filterWall = (self.getProperty("FilterByTimeMin"), self.getProperty("FilterByTimeMax"))

        # first round of processing the sample 
        samRun = self._loadData(sam, SUFFIX, filterWall)
        samRun = RenameWorkspace(samRun, samRun.getName() + "_sample").workspace()
        samRun = self._comp(samRun, filterLogs)
        
        # process the background
        if bkg > 0:
            bkgRun = mtd["%s_%d" % (self._instrument, bkg)]
            bkgRun = self._loadData(bkg, SUFFIX)
            bkgRun = RenameWorkspace(bkgRun, bkgRun.getName() + "_background").workspace()
            bkgRun = self._comp(bkgRun)
            Minus(LHSWorkspace=samRun, RHSWorkspace=bkgRun, OutputWorkspace=samRun, ClearRHSWorkspace="1")
            CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                           Tolerance=COMPRESS_TOL_TOF) # 10ns
            mtd.deleteWorkspace(bkgRun.getName())
        else:
            bkgRun = None 

        # process the empty instrument
        if empty > 0:
            emptyRun = mtd["%s_%d" % (self._instrument, empty)]
            emptyRun = self._loadData(empty, SUFFIX)
            emptyRun = RenameWorkspace(emptyRun, emptyRun.getName() + "_empty").workspace()
            emptyRun = self._comp(emptyRun)
            Minus(LHSWorkspace=samRun, RHSWorkspace=emptyRun, OutputWorkspace=samRun, ClearRHSWorkspace="1")
            CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                           Tolerance=COMPRESS_TOL_TOF) # 10ns
            mtd.deleteWorkspace(emptyRun.getName())
        else:
            emptyRun = None 

        # process the vanadium run
        if van > 0:
            vanRun = mtd["%s_%d" % (self._instrument, van)]
            vanRun = self._loadData(van, SUFFIX)
            vanRun = RenameWorkspace(vanRun, vanRun.getName() + "_vanadium").workspace()
            vanRun = self._vanbin(vanRun)
        else:
            vanRun = None        


        # the final bit of math
        if vanRun is not None:
            samRun = self._bin(samRun)
            Divide(samRun, vanRun, samRun, AllowDifferentNumberSpectra=True) 
            mtd.deleteWorkspace(vanRun.getName())
            normalized = True
        else:
            normalized = False

        # write out the files
        CompressEvents(InputWorkspace=samRun, OutputWorkspace=samRun,
                       Tolerance=COMPRESS_TOL_TOF) # 5ns
        self._save(samRun, normalized)
        mtd.deleteWorkspace(samRun.getName())
        raise Exception("End of script") 

mtd.registerPyAlgorithm(SNSSingleCrystalReduction())
