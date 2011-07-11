"""
    ISIS-specific implementation of the SANS Reducer. 
    
    WARNING: I'm still playing around with the ISIS reduction to try to 
    understand what's happening and how best to fit it in the Reducer design. 
     
"""
from reduction.instruments.sans.sans_reducer import SANSReducer
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
import isis_reduction_steps
from mantidsimple import *
import os
import copy

import sys

################################################################################
# Avoid a bug with deepcopy in python 2.6, details and workaround here:
# http://bugs.python.org/issue1515
if sys.version_info[0] == 2 and sys.version_info[1] == 6:
    import types
    def _deepcopy_method(x, memo):
        return type(x)(x.im_func, copy.deepcopy(x.im_self, memo), x.im_class)
    copy._deepcopy_dispatch[types.MethodType] = _deepcopy_method
################################################################################

## Version number
__version__ = '0.0'

current_settings = None

class Sample(object):
    def __init__(self):
        #will contain a LoadSample() object that converts the run number into a file name and loads that file  
        self.loader = None
        #the logs from the run file
        self.log = None
        #geometry that comes from the run and can be overridden by user settings
        self.geometry = sans_reduction_steps.GetSampleGeom()

    def set_run(self, run, reload, period, reducer):
        self.loader = isis_reduction_steps.LoadSample(run, reload, period)
        self.log = self.loader.execute(reducer, None)

        self.geometry.execute(None, self.get_wksp_name())
        
    def get_wksp_name(self):
        return self.loader.wksp_name
    
    wksp_name = property(get_wksp_name, None, None, None)

class ISISReducer(SANSReducer):
    """
        ISIS Reducer
        TODO: need documentation for all the data member
        TODO: need to see whether all those data members really belong here
    """    
    QXY2 = None
    DQY = None

    # Component positions
    PHIMIN=-90.0
    PHIMAX=90.0
    PHIMIRROR=True
    
    ## Path for user settings files
    _user_file_path = '.'

    def _to_steps(self):
        """
            Defines the steps that are run and their order
        """
        proc_TOF = [self.crop_detector]
        proc_TOF.append(self.mask)
        proc_TOF.append(self.to_wavelen)

        proc_wav = [self.norm_mon]
        proc_wav.append(self.transmission_calculator)
        proc_wav.append(self._corr_and_scale)
        proc_wav.append(self._geo_corr)

        self._can = [self.background_subtracter]
        
#        self._tidy = [self._zero_error_flags]
        self._tidy = [self._rem_nans]
        
        #the last step in the list must be ConvertToQ or can processing wont work
        self._conv_Q = proc_TOF + proc_wav + [self.to_Q]

        #list of steps to completely reduce a workspace
        self._reduction_steps = (self._conv_Q + self._can + self._tidy)

    def _init_steps(self):
        """
            Initialises the steps that are not initialised by (ISIS)CommandInterface.
        """       
        #these steps are not executed by reduce
        self.user_settings =   None
        self.place_det_sam =   isis_reduction_steps.MoveComponents()
        self._out_name =       isis_reduction_steps.GetOutputName()

        #except self.prep_normalize all the steps below are used by the reducer
        self.crop_detector =   isis_reduction_steps.CropDetBank(crop_sample=True)
        self.samp_trans_load = None
        self.can_trans_load =  None
        self.mask =self._mask= isis_reduction_steps.Mask_ISIS()
        self.to_wavelen =      isis_reduction_steps.UnitsConvert('Wavelength')
        self.norm_mon =        isis_reduction_steps.NormalizeToMonitor()
        self.transmission_calculator =\
                               isis_reduction_steps.TransmissionCalc(loader=None)
        self._corr_and_scale = isis_reduction_steps.AbsoluteUnitsISIS()
        
        self.prep_normalize = isis_reduction_steps.CalculateNormISIS(
                            [self.norm_mon, self.transmission_calculator])

        self.to_Q =            sans_reduction_steps.ConvertToQ(
                                                        self.prep_normalize)
        self.background_subtracter = None
        self._geo_corr =       sans_reduction_steps.SampleGeomCor(
                                                self._sample_run.geometry)
#        self._zero_error_flags=isis_reduction_steps.ReplaceErrors()
        self._rem_nans =      sans_reduction_steps.StripEndNans()

        self.set_Q_output_type(self.to_Q.output_type)
	
    def __init__(self):
        SANSReducer.__init__(self)
        self._sample_run = Sample()
        self.output_wksp = None
        self.full_trans_wav = False
        self._monitor_set = False
        #workspaces that this reducer uses and will delete at the end
        self._temporys = {}
        #the output workspaces created by a data analysis
        self._outputs = {}
        #all workspaces created by this reducer
        self._workspace = [self._temporys, self._outputs] 

        self._init_steps()

        #process the background (can) run instead of the sample 
        self._process_can = False

    def set_sample(self, run, reload, period):
        """
            Assigns the run that this reduction chain will analysis
            @param run: the run in a number.raw|nxs format
            @param reload: if this sample should be reloaded before the first reduction  
            @param period: the period within the sample to be analysed
        """
        self._sample_run.set_run(run, reload, period, self)

    def get_sample(self):
        """
            Gets information about the experimental run that is to be reduced 
            @return: the object with information about the sample
        """
        if not self._process_can:
            return self._sample_run
        else:
            return self.background_subtracter
        
    def get_out_ws_name(self, show_period=True):
        """
            Returns name of the workspace that will be created by this reduction
            which is based on the settings passed to the chain
            @param show_period: if True (default) the period (entry) number of the run is included in the name after a p
            @return: the workspace name to create
        """
        sample_obj = self.get_sample().loader
        name = str(sample_obj.shortrun_no)
        if show_period and sample_obj.periods_in_file > 1:
            if sample_obj._period == sample_obj.UNSET_PERIOD:
                period = 1
            else:
                period = sample_obj._period
            name += 'p'+str(period)
        
        name += self.instrument.cur_detector().name('short')
        name += '_' + self.to_Q.output_type
        name += '_' + self.to_wavelen.get_range()

        return name

    def deep_copy(self):
        """
            Returns a copy of the reducer that was created when the settings were set but
            before first execution
            @return: deep copy of the settings
        """
        global current_settings
        return copy.deepcopy(current_settings)
    
    def remove_settings(self):
        global current_settings
        current_settings = None
        
    def settings(self):
        """
            Retrieves the state of the reducer after it was setup and before running or None
            if the reducer hasn't been setup
        """
        return current_settings

    def is_can(self):
        """
            Indicates if the current run is a can reduction or not
            @return: True if the can is being processed
        """
        return self._process_can

    def _reduce(self, init=True, post=True, steps=None):
        """
            Execute the list of all reduction steps
            @param init: if False it assumes that the reducer is fully setup, default=True
            @param post: if to run the post run commands, default True
            @param steps: the list of ReductionSteps to execute, defaults to _reduction_steps if not set
            @return: name of the output workspace
        """
        if init:
            self.pre_process()

        if not steps:
            steps = self._reduction_steps
        #the main part of the reduction is done here, go through and execute each step
        for item in steps:
            if item:
                item.execute(self, self.output_wksp)

        #any clean up, possibly removing workspaces 
        if post:
            self.post_process()
        
        return self.output_wksp

    def reduce_can(self, new_wksp=None, run_Q=True):
        """
            Apply the sample corrections to a can workspace. This reducer is deep copied
            and the output workspace name, transmission and monitor workspaces are changed.
            Then the reduction is applied to the given workspace 
            @param new_wksp: the name of the workspace that will store the result
            @param run_Q: set to false to stop just before converting to Q, default is convert to Q
        """
        # copy all the run settings from the sample, these settings can come from the user file, Python scripting or the GUI
        new_reducer = self.deep_copy()

        new_reducer._process_can = True
        #set the workspace that we've been setting up as the one to be processed 
        new_reducer.output_wksp = new_wksp
        
        can_steps = new_reducer._conv_Q
        if not run_Q:
            #the last step in the list must be ConvertToQ or this wont work
            can_steps = can_steps[0:len(can_steps)-1]

        #the reducer is completely setup, run it
        new_reducer._reduce(init=False, post=False, steps=can_steps)

    def run_from_raw(self):
        """
            Assumes the reducer is copied from a running one
            Executes all the steps after moving the components
        """
        self._out_name.execute(self)
        return self._reduce(init=False, post=True)

    def set_Q_output_type(self, out_type):
       self.to_Q.set_output_type(out_type)

    def pre_process(self):
        super(ISISReducer, self).pre_process()
        self._out_name.execute(self)
        global current_settings
        current_settings = copy.deepcopy(self)

    def post_process(self):
        # Store the mask file within the final workspace so that it is saved to the CanSAS file
        if self.user_settings is None:
            user_file = 'None'
        else:
            user_file = self.user_settings.filename
        AddSampleLog(self.output_wksp, "UserFile", LogText=user_file)
	
        for role in self._temporys.keys():
            try:
                DeleteWorkspace(self._temporys[role])
            except:
            #if cleaning up isn't possible there is probably nothing we can do
                pass

    def set_user_path(self, path):
        """
            Set the path for user files
            @param path: user file path
        """
        if os.path.isdir(path):
            self._user_file_path = path
        else:
            raise RuntimeError, "ISISReducer.set_user_path: provided path is not a directory (%s)" % path

    def get_user_path(self):
        return self._user_file_path
    
    user_file_path = property(get_user_path, set_user_path, None, None)

    def set_trans_fit(self, lambda_min=None, lambda_max=None, fit_method="Log"):
        self.transmission_calculator.set_trans_fit(lambda_min, lambda_max, fit_method, override=True)
        
    def set_trans_sample(self, sample, direct, reload=True, period_t = -1, period_d = -1):
        if not issubclass(self.samp_trans_load.__class__, sans_reduction_steps.BaseTransmission):
            self.samp_trans_load = isis_reduction_steps.LoadTransmissions(reload=reload)
        self.samp_trans_load.set_trans(sample, period_t)
        self.samp_trans_load.set_direc(direct, period_d)
        self.transmission_calculator.samp_loader = self.samp_trans_load

    def set_trans_can(self, can, direct, reload = True, period_t = -1, period_d = -1):
        if not issubclass(self.can_trans_load.__class__, sans_reduction_steps.BaseTransmission):
            self.can_trans_load = isis_reduction_steps.LoadTransmissions(is_can=True, reload=reload)
        self.can_trans_load.set_trans(can, period_t)
        self.can_trans_load.set_direc(direct, period_d)
        self.transmission_calculator.can_loader = self.can_trans_load

    def set_monitor_spectrum(self, specNum, interp=False, override=True):
        if override:
            self._monitor_set=True
        
        self.instrument.set_interpolating_norm(interp)
        
        if not self._monitor_set or override:
            self.instrument.set_incident_mon(specNum)
                        
    def set_trans_spectrum(self, specNum, interp=False, override=True):
        self.instrument.incid_mon_4_trans_calc = int(specNum)

        self.transmission_calculator.interpolate = interp

    def step_num(self, step):
        """
            Returns the index number of a step in the
            list of steps that have _so_ _far_ been
            added to the chain
        """
        return self._reduction_steps.index(step)

    def get_instrument(self):
        """
            Convenience function used by the inst property to make
            calls shorter
        """
        return self.instrument
 
    #quicker to write than .instrument 
    inst = property(get_instrument, None, None, None)
 
    def Q_string(self):
        return '    Q range: ' + self.to_Q.binning +'\n    QXY range: ' + self.QXY2+'-'+self.DQXY

    def ViewCurrentMask(self):
        """
            In MantidPlot this opens InstrumentView to display the masked
            detectors in the bank in a different colour
        """
        self._mask.view(self.instrument)

    def reference(self):
        return self

    CENT_FIND_RMIN = None
    CENT_FIND_RMAX = None
    
def deleteWorkspaces(workspaces):
    """
        Deletes a list of workspaces if they exist but ignores any errors
    """
    for wk in workspaces:
        try:
            if wk and mantid.workspaceExists(wk):
                DeleteWorkspace(wk)
        except:
            #if the workspace can't be deleted this function does nothing
            pass

