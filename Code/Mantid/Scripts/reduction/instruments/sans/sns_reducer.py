"""
    Reducer class for EQSANS reduction
    
    Implemented steps:
        _beam_finder  
        _normalizer
        _dark_current_subtracter
        _sensitivity_correcter
        _solid_angle_correcter 
        _azimuthal_averager
        _transmission_calculator
        _mask - needs more shapes, such as ellipse
        _data_loader
        _background_subtracter

    
    Notes: 
        - Default mask on EQ-SANS: read it from logs?

"""
from sans_reducer import SANSReducer
from reduction import ReductionStep
import sns_reduction_steps
import sans_reduction_steps

class EqSansReducer(SANSReducer):

    ## Transform TOF to wavelength
    tof_to_wavelength = None
    ## Frame skipping
    frame_skipping = False
    
    def __init__(self):
        super(EqSansReducer, self).__init__()
        ## Default beam finder
        self._beam_finder = sans_reduction_steps.BaseBeamFinder(96.29, 126.15)
        ## Default data loader
        self._data_loader = sns_reduction_steps.LoadRun()
        ## Normalization
        self._normalizer = sns_reduction_steps.Normalize()
        ## Transmission calculator
        self._transmission_calculator = sns_reduction_steps.Transmission(True)
        
        self._solid_angle_correcter = sans_reduction_steps.SolidAngle()
        
        # Default dark current subtracter class
        self._dark_current_subtracter_class = sns_reduction_steps.SubtractDarkCurrent

    def set_instrument(self, configuration):
        super(SANSReducer, self).set_instrument(configuration)
        
    def set_normalizer(self, normalizer):
        """
            Set the ReductionStep object that takes care of normalization
            @param normalizer: ReductionStep object
        """
        if issubclass(normalizer.__class__, ReductionStep) or normalizer is None:
            self._normalizer = normalizer
        else:
            raise RuntimeError, "Reducer.set_normalizer expects an object of class ReductionStep"
        
    def set_frame_skipping(self, value):
        """
            
        """
        if value not in [True, False]:
            raise ValueError, "Set_frame_skipping only accepts True or False"
        self.frame_skipping = value

    def _2D_steps(self):
        """
            Creates a list of reduction steps to be applied to
            each data set, including the background file. 
            Only the steps applied to a data set
            before azimuthal averaging are included.
        """
        reduction_steps = []
        
        # Load file
        reduction_steps.append(self._data_loader)
        
        # Apply transmission correction
        if self._transmission_calculator is not None:
            reduction_steps.append(self._transmission_calculator) 
        
        # Dark current subtraction
        if self._dark_current_subtracter is not None:
            reduction_steps.append(self._dark_current_subtracter)
        
        # Normalize
        if self._normalizer is not None:
            reduction_steps.append(self._normalizer)
        
        # Mask
        if self._mask is not None:
            reduction_steps.append(self._mask)
        
        # Sensitivity correction
        if self._sensitivity_correcter is not None:
            reduction_steps.append(self._sensitivity_correcter)
            
        # Solid angle correction
        if self._solid_angle_correcter is not None:
            reduction_steps.append(self._solid_angle_correcter)
        
        return reduction_steps
    
    def _to_steps(self):
        """
            Creates a list of reduction steps for each data set
            following a predefined reduction approach. For each 
            predefined step, we check that a ReductionStep object 
            exists to take of it. If one does, we append it to the 
            list of steps to be executed.
        """
        # Get the basic 2D steps
        self._reduction_steps = self._2D_steps()
        
        # Subtract the background
        if self._background_subtracter is not None:
            self.append_step(self._background_subtracter)
        
        # Perform azimuthal averaging
        if self._azimuthal_averager is not None:
            self.append_step(self._azimuthal_averager)
            
        # Save output to file
        if self._save_iq is not None:
            self.append_step(self._save_iq)
            
        
