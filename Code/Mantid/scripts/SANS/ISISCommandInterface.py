"""
    Enables the SANS commands (listed at http://www.mantidproject.org/SANS) to
    be run
"""
import isis_instrument
from reduction.command_interface import ReductionSingleton  
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
import isis_reduction_steps
import isis_reducer
from centre_finder import CentreFinder as CentreFinder
#import SANSReduction
import MantidFramework
import copy
from SANSadd2 import *

# disable plotting if running outside Mantidplot
try:
    import mantidplot
except:
    #this should happen when this is called from outside Mantidplot and only then, the result is that attempting to plot will raise an exception
    pass

try:
    from PyQt4.QtGui import qApp
    def appwidgets():
        return qApp.allWidgets()
except ImportError:
    def appwidgets():
        return []

_VERBOSE_ = False
LAST_SAMPLE = None
def SetVerboseMode(state):
#TODO: this needs to be on the reducer
    _VERBOSE_ = state

# Print a message and log it if the 
def _printMessage(msg, log = True, no_console=False):
    if log == True and _VERBOSE_ == True:
        MantidFramework.mtd.sendLogMessage('::SANS::' + msg)
    if not no_console:
        print msg
    
def issueWarning(msg):
    """
        Issues a Mantid message
        @param msg: message to be issued
    """
    isis_reduction_steps._issueWarning(msg)

def _refresh_singleton():
    ReductionSingleton.clean(isis_reducer.ISISReducer)
    ReductionSingleton().remove_settings()
                
def UserPath(path):
    """
        Sets the directory in which Mantid should look for the mask file if a
        full path was not specified
        @param path: the full path to the directory
    """
    _printMessage('UserPath("' + path + '") #Will look for mask file here')
    ReductionSingleton().user_file_path = path

def DataPath(path):
    """
        Sets an extra directory for Mantid to look for run files
        @param path: the full path to a directory containing the run files to analyse
    """
    ReductionSingleton().set_data_path(path)

def SANS2D():
    """
        Initialises the instrument settings for SANS2D
        @return True on success
    """
    _printMessage('SANS2D()')
    try:
        instrument = isis_instrument.SANS2D()
        
        ReductionSingleton().set_instrument(instrument)
    except:
        return False
    return True

def LOQ():
    """
        Initialises the instrument settings for LOQ
        @return True on success
    """
    _printMessage('LOQ()')
    try:
        instrument = isis_instrument.LOQ()

        ReductionSingleton().set_instrument(instrument)
    except:
        return False
    return True
    
def Detector(det_name):
    """
        Sets the detector bank to use for the reduction e.g. 'front-detector'. The
        main detector is assumed if this line is not given
        @param det_name: the detector's name
    """
    _printMessage('Detector("' + det_name + '")')
    ReductionSingleton().instrument.setDetector(det_name)
    
def Mask(details):
    """
        Specify regions of the detector to mask using the same syntax
        as used in the user file
        @param details: a string that specifies masking as it would appear in a mask file 
    """ 
    _printMessage('Mask("' + details + '")')
    ReductionSingleton().mask.parse_instruction(details)
    
def MaskFile(file_name):
    """
        Loads the settings file. The settings are loaded as soon as this line is encountered
        and are overridden by other Python commands
        @param file_name: the settings file
    """
    _printMessage('#Opening "'+file_name+'"')
    ReductionSingleton().user_settings = isis_reduction_steps.UserFile(
        file_name)
    status = ReductionSingleton().user_settings.execute(
        ReductionSingleton(), None)
    _printMessage('#Success reading "'+file_name+'"'+' is '+str(status))
    return status
    
def SetMonitorSpectrum(specNum, interp=False):
    ReductionSingleton().set_monitor_spectrum(specNum, interp)

def SetTransSpectrum(specNum, interp=False):
    ReductionSingleton().set_trans_spectrum(specNum, interp)
      
def SetSampleOffset(value):
    ReductionSingleton().instrument.set_sample_offset(value)
    
def Gravity(flag):
    _printMessage('Gravity(' + str(flag) + ')')
    ReductionSingleton().to_Q.set_gravity(flag)
    
def TransFit(mode,lambdamin=None,lambdamax=None):
    """
        Sets the fit method to calculate the transmission fit and the wavelength range
        over which to do the fit. These arguments are passed to the algorithm
        CalculateTransmission.If mode is set to 'Off' then the unfitted workspace is
        used and ??? not yet true, can we make it so??? lambdamin and max have no effect
        @param mode: can be 'Logarithmic' ('YLOG', 'LOG') 'OFF' ('CLEAR') or 'LINEAR' (STRAIGHT', LIN')
        @param lambdamin: the lowest wavelength to use in any fit
        @param lambdamax: the end of the fit range
    """
    mode = str(mode).strip().upper()
    message = mode
    if lambdamin: message += ', ' + str(lambdamin)
    if lambdamax: message += ', ' + str(lambdamax)
    _printMessage("TransFit(\"" + message + "\")")

    ReductionSingleton().set_trans_fit(lambdamin, lambdamax, mode)
    
def TransWorkspace(sample, can = None):
    """
        Use a given workpspace that contains pre-calculated transmissions
        @param sample the workspace to use for the sample
        @param can calculated transmission for the can 
    """
    ReductionSingleton().transmission_calculator.calculated_samp = sample 
    ReductionSingleton().transmission_calculator.calculated_can = can 

def AssignCan(can_run, reload = True, period = isis_reduction_steps.LoadRun.UNSET_PERIOD):
    mes = 'AssignCan("' + can_run + '"'
    if period != isis_reduction_steps.LoadRun.UNSET_PERIOD:
        mes += ', ' + str(period)
    mes += ')'
    _printMessage(mes)

    if (not can_run) or can_run.startswith('.'):
        ReductionSingleton().background_subtracter = None
        return '', None

    ReductionSingleton().background_subtracter = \
        isis_reduction_steps.CanSubtraction(
                                can_run, reload=reload, period=period)
    #ideally this code should live in a separate load can object 
    logs = ReductionSingleton().background_subtracter.assign_can(
        ReductionSingleton())
    return ReductionSingleton().background_subtracter.workspace.wksp_name, logs

def TransmissionSample(sample, direct, reload = True, period_t = -1, period_d = -1):
    """
        Specify the transmission and direct runs for the sample
        @param sample: the transmission run
        @param direct: direct run
        @param reload: if to replace the workspace if it is already there
        @param period_t: the entry number of the transmission run (default single entry file)  
        @param period_d: the entry number of the direct run (default single entry file)
    """  
    _printMessage('TransmissionSample("' + sample + '","' + direct + '")')
    ReductionSingleton().set_trans_sample(sample, direct, True, period_t, period_d)
    return ReductionSingleton().samp_trans_load.execute(
                                        ReductionSingleton(), None)

def TransmissionCan(can, direct, reload = True, period_t = -1, period_d = -1):
    """
        Specify the transmission and direct runs for the can
        @param can: the transmission run
        @param direct: direct run
        @param reload: if to replace the workspace if it is already there
        @param period_t: the entry number of the transmission run (default single entry file)  
        @param period_d: the entry number of the direct run (default single entry file)
    """
    _printMessage('TransmissionCan("' + can + '","' + direct + '")')
    ReductionSingleton().set_trans_can(can, direct, True, period_t, period_d)
    return ReductionSingleton().can_trans_load.execute(
                                            ReductionSingleton(), None)
    
def AssignSample(sample_run, reload = True, period = isis_reduction_steps.LoadRun.UNSET_PERIOD):
    mes = 'AssignSample("' + sample_run + '"'
    if period != isis_reduction_steps.LoadRun.UNSET_PERIOD:
        mes += ', ' + str(period)
    mes += ')'
    _printMessage(mes)

    ReductionSingleton().set_sample(sample_run, reload, period)
    
    global LAST_SAMPLE
    LAST_SAMPLE = ReductionSingleton().get_sample().wksp_name
    return ReductionSingleton().get_sample().wksp_name, \
        ReductionSingleton().get_sample().log

def SetCentre(xcoord, ycoord):
    _printMessage('SetCentre(' + str(xcoord) + ', ' + str(ycoord) + ')')

    ReductionSingleton().set_beam_finder(sans_reduction_steps.BaseBeamFinder(
                                float(xcoord)/1000.0, float(ycoord)/1000.0))

def GetMismatchedDetList():
    """
        Return the list of mismatched detector names
    """
    return ReductionSingleton().instrument.get_marked_dets()

def _setUpPeriod(i):
    trans_samp = ReductionSingleton().samp_trans_load
    can = ReductionSingleton().background_subtracter
    trans_can = ReductionSingleton().can_trans_load
    AssignSample(ReductionSingleton().get_sample().loader._data_file, period=i)
    if can:
        #replace one thing that gets overwritten
        AssignCan(can.workspace._data_file, True, period=can.workspace.getCorrospondingPeriod(i, ReductionSingleton()))
    if trans_samp:
        trans = trans_samp.trans
        direct = trans_samp.direct
        TransmissionSample(trans._data_file, direct._data_file, True, period_t=trans.getCorrospondingPeriod(i, ReductionSingleton()),period_d=direct.getCorrospondingPeriod(i, ReductionSingleton()))  
    if trans_can:
        trans = trans_can.trans
        direct = trans_can.direct
        TransmissionCan(trans._data_file, direct._data_file, True, period_t=trans.getCorrospondingPeriod(i, ReductionSingleton()),period_d=direct.getCorrospondingPeriod(i, ReductionSingleton()))  

def WavRangeReduction(wav_start=None, wav_end=None, full_trans_wav=None, name_suffix=None):
    """
        Run a reduction that has been set up, from loading the raw data to calculating Q,  and
        reset the old setup
        @param wav_start: the first wavelength to be in the output data
        @param wav_end: the last wavelength in the output data
        @param full_trans_wav: if to use a wide wavelength range, the instrument's default wavelength range, for the transmission correction, false by default
    """
    _printMessage('WavRangeReduction(' + str(wav_start) + ', ' + str(wav_end) + ', '+str(full_trans_wav)+')')

    if not full_trans_wav is None:
        ReductionSingleton().full_trans_wav = full_trans_wav

    ReductionSingleton().to_wavelen.set_range(wav_start, wav_end)
    _printMessage('Running reduction for ' + str(ReductionSingleton().to_wavelen))

    try:
        calculated = [ReductionSingleton()._reduce()]

        periods = ReductionSingleton().get_sample().loader.entries    
        if len(periods) > 1:
            run_setup = ReductionSingleton().settings()
            for i in periods[1:len(periods)]:
                ReductionSingleton.replace(copy.deepcopy(run_setup))
                _setUpPeriod(i)
                calculated.append(ReductionSingleton()._reduce())
            result = ReductionSingleton().get_out_ws_name(show_period=False)
            all_results = calculated[0]
            for name in calculated[1:len(calculated)]:
                all_results += ',' + name
            GroupWorkspaces(OutputWorkspace=result, InputWorkspaces=all_results)
        else:
            result = calculated[0] 
               
    finally:
        _refresh_singleton()

    if name_suffix:
        old = result
        result += name_suffix
        RenameWorkspace(old, result)
        
    return result

def CompWavRanges(wavelens, plot=True):
    """
        Compares the momentum transfer results calculated from different wavelength ranges. Given
        the list of wave ranges [a, b, c] it reduces for wavelengths a-b, b-c and a-c.
        @param wavelens: the list of wavelength ranges
        @param plot: set this to true to plot the result (must be run in Mantid), default is true
    """ 

    _printMessage('CompWavRanges( %s,plot=%s)'%(str(wavelens),plot))

    #this only makes sense for 1D reductions
    if ReductionSingleton().to_Q.output_type == '2D':
        issueWarning('This wave ranges check is a 1D analysis, ignoring 2D setting')
        _printMessage('Set1D()')
        ReductionSingleton().to_Q.output_type = '1D'
    
    if type(wavelens) != type([]) or len(wavelens) < 2:
        if type(wavelens) != type((1,)):
            raise RuntimeError('Error CompWavRanges() requires a list of wavelengths between which reductions will be performed.')
    
    try:
        ReductionSingleton().to_wavelen.set_rebin(w_low=wavelens[0],
            w_high=wavelens[len(wavelens)-1])
        #run the reductions, calculated will be an array with the names of all the workspaces produced
        calculated = [ReductionSingleton()._reduce()]
        for i in range(0, len(wavelens)-1):
            ReductionSingleton.replace(ReductionSingleton().settings())
            ReductionSingleton().to_wavelen.set_rebin(
                            w_low=wavelens[i], w_high=wavelens[i+1])
            calculated.append(ReductionSingleton()._reduce())
    finally:
        _refresh_singleton()

    if plot:
        mantidplot.plotSpectrum(calculated, 0)
    
    #return just the workspace name of the full range
    return calculated[0]

def Reduce():
    try:
        result = ReductionSingleton()._reduce()
    finally:
        _refresh_singleton()

    return result
            
def _SetWavelengthRange(start, end):
    ReductionSingleton().to_wavelen.set_range(start, end)

def Set1D():
    _printMessage('Set1D()')
    ReductionSingleton().set_Q_output_type('1D')

def Set2D():
    _printMessage('Set2D()')
    ReductionSingleton().set_Q_output_type('2D')

def SetRearEfficiencyFile(filename):
    rear_det = ReductionSingleton().instrument.getDetector('rear')
    rear_det.correction_file = filename

def SetFrontEfficiencyFile(filename):
    front_det = ReductionSingleton().instrument.getDetector('front')
    front_det.correction_file = filename

def SetDetectorFloodFile(filename):
    ReductionSingleton().prep_normalize.setPixelCorrFile(filename)

def displayUserFile():
    print '-- Mask file defaults --'
    print ReductionSingleton().to_wavlen
    print ReductionSingleton().Q_string()
#    print correction_files()
    print '    direct beam file rear:',
    print ReductionSingleton().instrument.detector_file('rear')
    print '    direct beam file front:',
    print ReductionSingleton().instrument.detector_file('front')
    print ReductionSingleton().mask

def displayMaskFile():
    displayUserFile()

def displayGeometry():
    [x, y] = ReductionSingleton()._beam_finder.get_beam_center()
    print 'Beam centre: [' + str(x) + ',' + str(y) + ']'
    print ReductionSingleton().get_sample().geometry

def SetPhiLimit(phimin, phimax, use_mirror=True):
    """
        Call this function to restrict the analyse segments of the detector. Phimin and
        phimax define the limits of the segment where phi=0 is the -x axis and phi = 90
        is the y-axis. Setting use_mirror to true includes a second segment to be included
        it is the same as the first but rotated 180 degrees.
        @param phimin: the minimum phi angle to include
        @param phimax: the upper limit on phi for the segment
        @param use_mirror: when True (default) another segment is included, rotated 180 degrees from the first
    """  
    _printMessage("SetPhiLimit(" + str(phimin) + ', ' + str(phimax) + ',use_mirror='+str(use_mirror)+')')
    #a beam centre of [0,0,0] makes sense if the detector has been moved such that beam centre is at [0,0,0]
    ReductionSingleton().mask.set_phi_limit(phimin, phimax, use_mirror)
    
def LimitsPhi(phimin, phimax, use_mirror=True):
    '''
        !!DEPRECIATED by the function above, remove once I've check with Richard
    '''
    if use_mirror :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=True)')
        ReductionSingleton().mask.set_phi_limit(phimin, phimax, True)
    else :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=False)')
        ReductionSingleton().mask.set_phi_limit(phimin, phimax, False)

def LimitsR(rmin, rmax, quiet=False, reducer=None):
    if reducer == None:
        reducer = ReductionSingleton().reference()

    if not quiet:
        _printMessage('LimitsR(' + str(rmin) + ', ' +str(rmax) + ')', reducer)

    reducer.mask.set_radi(rmin, rmax)
    reducer.CENT_FIND_RMIN = float(rmin)/1000.
    reducer.CENT_FIND_RMAX = float(rmax)/1000.    

def LimitsWav(lmin, lmax, step, bin_type):
    _printMessage('LimitsWav(' + str(lmin) + ', ' + str(lmax) + ', ' + str(step) + ', '  + bin_type + ')')
    
    if ( bin_type.upper().strip() == 'LINEAR'): bin_type = 'LIN'
    if ( bin_type.upper().strip() == 'LOGARITHMIC'): bin_type = 'LOG'
    if bin_type == 'LOG':
        bin_sym = '-'
    else:
        bin_sym = ''
    
    ReductionSingleton().to_wavelen.set_rebin(lmin, bin_sym + str(step), lmax)

def LimitsQ(*args):
    settings = ReductionSingleton().user_settings
    if settings is None:
        raise RuntimeError('MaskFile() first')

    # If given one argument it must be a rebin string
    if len(args) == 1:
        val = args[0]
        if type(val) == str:
            _printMessage("LimitsQ(" + val + ")")
            settings.readLimitValues("L/Q " + val, ReductionSingleton())
        else:
            issueWarning("LimitsQ can only be called with a single string or 4 values")
    elif len(args) == 4:
        qmin,qmax,step,step_type = args
        _printMessage('LimitsQ(' + str(qmin) + ', ' + str(qmax) +', ' + str(step) + ','  + str(step_type) + ')')
        settings.readLimitValues('L/Q ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + step_type, ReductionSingleton())
    else:
        issueWarning("LimitsQ called with " + str(len(args)) + " arguments, 1 or 4 expected.")

def LimitsQXY(qmin, qmax, step, type):
    """
        To set the bin parameters for the algorithm Qxy()
        @param qmin: the first Q value to include
        @param qmaz: the last Q value to include
        @param step: bin width
        @param type: pass LOG for logarithmic binning
    """
    _printMessage('LimitsQXY(' + str(qmin) + ', ' + str(qmax) +', ' + str(step) + ', ' + str(type) + ')')
    settings = ReductionSingleton().user_settings
    if settings is None:
        raise RuntimeError('MaskFile() first')

    settings.readLimitValues('L/QXY ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + type, ReductionSingleton())

def PlotResult(workspace, canvas=None):
    """
        Draws a graph of the passed workspace. If the workspace is 2D (has many spectra
        a contour plot is written
        @param workspace: a workspace name or handle to plot
        @param canvas: optional handle to an existing graph to write the plot to
        @return: a handle to the graph that was written to
    """ 
    #ensure that we are dealing with a workspace handle rather than its name
    workspace = mtd[str(workspace)]
    if workspace.isGroup():
        numSpecs = workspace[0].getNumberHistograms()
    else:
        numSpecs = workspace.getNumberHistograms()

    try:
        if numSpecs == 1:
            graph = mantidplot.plotSpectrum(workspace,0)
        else:        
            graph = mantidplot.importMatrixWorkspace(workspace.getName()).plotGraph2D()

    except NameError:
        issueWarning('Plot functions are not available, is this being run from outside Mantidplot?')
        
    if not canvas is None:
        #we were given a handle to an existing graph, use it
        mantidplot.mergePlots(canvas, graph)
        graph = canvas
    
    return graph

##################### View mask details #####################################################

def ViewCurrentMask():
    """
        In MantidPlot this opens InstrumentView to display the masked
        detectors in the bank in a different colour
    """
    ReductionSingleton().ViewCurrentMask()

def DisplayMask(mask_worksp=None):
    """
        Displays masking by applying it to a workspace and displaying
        it in instrument view. If no workspace is passed a copy of the
        sample workspace is used, unless no sample was loaded and then
        an empty instrument will be shown
        @param mask_worksp: optional this named workspace will be modified and should be from the currently selected instrument
        @return the name of the workspace that was displayed
    """
    #this will be copied from a sample work space if one exists
    counts_data = None
    instrument = ReductionSingleton().instrument
    
    if not mask_worksp:
        mask_worksp = '__CurrentMask'
        samp = LAST_SAMPLE 
        
        if samp:
            counts_data = '__DisplayMasked_tempory_wksp'
            Integration(samp, counts_data)
            CloneWorkspace(samp, mask_worksp)
        else:
            instrument.load_empty(mask_worksp)
        
    ReductionSingleton().mask.display(mask_worksp, instrument, counts_data)
    if counts_data:
        mantid.deleteWorkspace(counts_data)
        
    return mask_worksp

# Print a test script for Colette if asked
def createColetteScript(inputdata, format, reduced, centreit , plotresults, csvfile = '', savepath = ''):
    script = ''
    if csvfile != '':
        script += '[COLETTE]  @ ' + csvfile + '\n'
    file_1 = inputdata['sample_sans'] + format
    script += '[COLETTE]  ASSIGN/SAMPLE ' + file_1 + '\n'
    file_1 = inputdata['sample_trans'] + format
    file_2 = inputdata['sample_direct_beam'] + format
    if file_1 != format and file_2 != format:
        script += '[COLETTE]  TRANSMISSION/SAMPLE/MEASURED ' + file_1 + ' ' + file_2 + '\n'
    file_1 = inputdata['can_sans'] + format
    if file_1 != format:
        script +='[COLETTE]  ASSIGN/CAN ' + file_1 + '\n'
    file_1 = inputdata['can_trans'] + format
    file_2 = inputdata['can_direct_beam'] + format
    if file_1 != format and file_2 != format:
        script += '[COLETTE]  TRANSMISSION/CAN/MEASURED ' + file_1 + ' ' + file_2 + '\n'
    if centreit:
        script += '[COLETTE]  FIT/MIDDLE'
    # Parameters
    script += '[COLETTE]  LIMIT/RADIUS ' + str(ReductionSingleton().mask.min_radius)
    script += ' ' + str(ReductionSingleton().mask.max_radius) + '\n'
    script += '[COLETTE]  LIMIT/WAVELENGTH ' + ReductionSingleton().to_wavelen.get_range() + '\n'
    if ReductionSingleton().DWAV <  0:
        script += '[COLETTE]  STEP/WAVELENGTH/LOGARITHMIC ' + str(ReductionSingleton().to_wavelen.w_step)[1:] + '\n'
    else:
        script += '[COLETTE]  STEP/WAVELENGTH/LINEAR ' + str(ReductionSingleton().to_wavelen.w_step) + '\n'
    # For the moment treat the rebin string as min/max/step
    qbins = ReductionSingleton().Q_REBEIN.split(",")
    nbins = len(qbins)
    if ReductionSingleton().to_Q.output_type == '1D':
        script += '[COLETTE]  LIMIT/Q ' + str(qbins[0]) + ' ' + str(qbins[nbins-1]) + '\n'
        dq = float(qbins[1])
        if dq <  0:
            script += '[COLETTE]  STEP/Q/LOGARITHMIC ' + str(dq)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/Q/LINEAR ' + str(dq) + '\n'
    else:
        script += '[COLETTE]  LIMIT/QXY ' + str(0.0) + ' ' + str(ReductionSingleton().QXY2) + '\n'
        if ReductionSingleton().DQXY <  0:
            script += '[COLETTE]  STEP/QXY/LOGARITHMIC ' + str(ReductionSingleton().DQXY)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/QXY/LINEAR ' + str(ReductionSingleton().DQXY) + '\n'
    
    # Correct
    script += '[COLETTE] CORRECT\n'
    if plotresults:
        script += '[COLETTE]  DISPLAY/HISTOGRAM ' + reduced + '\n'
    if savepath != '':
        script += '[COLETTE]  WRITE/LOQ ' + reduced + ' ' + savepath + '\n'
        
    return script

def FindBeamCentre(rlow, rupp, MaxIter = 10, xstart = None, ystart = None):
    """
        Estimates the location of the effective beam centre given a good initial estimate. For more
        information go to this page
        mantidproject.org/Using_the_SANS_GUI_Beam_Centre_Finder
        @param rlow: mask around the (estimated) centre to this radius (in millimetres)
        @param rupp: don't include further out than this distance (mm) from the centre point
        @param MaxInter: don't calculate more than this number of iterations (default = 10)
        @param xstart: initial guess for the horizontal distance of the beam centre from the detector centre in meters (default the values in the mask file)
        @param ystart: initial guess for the distance of the beam centre from the detector centre vertically in metres (default the values in the mask file)
        @return: the best guess for the beam centre point
    """
    XSTEP = ReductionSingleton().inst.cen_find_step
    YSTEP = ReductionSingleton().inst.cen_find_step

    original = ReductionSingleton()._beam_finder.get_beam_center()
    if xstart or ystart:
        ReductionSingleton().set_beam_finder(
            sans_reduction_steps.BaseBeamFinder(
            float(xstart), float(ystart)))

    beamcoords = ReductionSingleton()._beam_finder.get_beam_center()
    XNEW = beamcoords[0]
    YNEW = beamcoords[1]
    xstart = beamcoords[0]
    ystart = beamcoords[1]
    
    mantid.sendLogMessage("::SANS:: xstart,ystart="+str(XNEW*1000.)+" "+str(YNEW*1000.)) 
    _printMessage("Starting centre finding routine ...")

    #remove this if we know running the Reducer() doesn't change i.e. all execute() methods are const
    centre_reduction = copy.deepcopy(ReductionSingleton().reference())
    LimitsR(str(float(rlow)), str(float(rupp)), quiet=True, reducer=centre_reduction)

    centre = CentreFinder(original)
    #this function moves the detector to the beam center positions defined above and returns an estimate of where the beam center is relative to the new center  
    resX_old, resY_old = centre.SeekCentre(centre_reduction, [XNEW, YNEW])
    centre_reduction = copy.deepcopy(ReductionSingleton().reference())
    LimitsR(str(float(rlow)), str(float(rupp)), quiet=True, reducer=centre_reduction)

    mantid.sendLogMessage(centre.status_str(0, resX_old, resY_old))
    
    # take first trial step
    XNEW = xstart + XSTEP
    YNEW = ystart + YSTEP
    graph_handle = None
    for i in range(1, MaxIter+1):
        it = i
        
        centre_reduction.set_beam_finder(
            sans_reduction_steps.BaseBeamFinder(XNEW, YNEW))

        resX, resY = centre.SeekCentre(centre_reduction, [XNEW, YNEW])
        centre_reduction = copy.deepcopy(ReductionSingleton().reference())
        LimitsR(str(float(rlow)), str(float(rupp)), quiet=True, reducer=centre_reduction)

        mantid.sendLogMessage(centre.status_str(it, resX, resY))
        
        try :
            if not graph_handle:
                #once we have a plot it will be updated automatically when the workspaces are updated
                graph_handle = mantidplot.plotSpectrum(centre.QUADS, 0)
            graph_handle.activeLayer().setTitle(
                        centre.status_str(it, resX, resY))
        except :
            #if plotting is not available it probably means we are running outside a GUI, in which case do everything but don't plot
            pass

        #have we stepped across the y-axis that goes through the beam center?  
        if resX > resX_old:
            # yes with stepped across the middle, reverse direction and half the step size 
            XSTEP = -XSTEP/2.
        if resY > resY_old:
            YSTEP = -YSTEP/2.
        if abs(XSTEP) < 0.1251/1000. and abs(YSTEP) < 0.1251/1000. :
            # this is the success criteria, we've close enough to the center
            mantid.sendLogMessage("::SANS:: Converged - check if stuck in local minimum!")
            break
        
        resX_old = resX
        resY_old = resY
        XNEW += XSTEP
        YNEW += YSTEP
    
    if it == MaxIter:
        mantid.sendLogMessage("::SANS:: Out of iterations, new coordinates may not be the best!")
        XNEW -= XSTEP
        YNEW -= YSTEP
    
    ReductionSingleton().set_beam_finder(
        sans_reduction_steps.BaseBeamFinder(XNEW, YNEW))
    _printMessage("Centre coordinates updated: [" + str(XNEW)+ ", "+ str(YNEW) + ']')
    
    return XNEW, YNEW

#this is like a #define I'd like to get rid of it because it seems meaningless here
DefaultTrans = 'True'
NewTrans = 'False'

_refresh_singleton()

#if __name__ != '__main__':
#    AssignSample('c:\\mantid\\test\\data\\SANS2D\\SANS2D000992.raw')
