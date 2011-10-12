"""
    Implementation of reduction steps for SNS EQSANS    
"""
import os
import sys
import pickle
import math
from reduction import ReductionStep
from sans_reduction_steps import WeightedAzimuthalAverage
from sans_reduction_steps import DirectBeamTransmission as SingleFrameDirectBeamTransmission
from sans_reduction_steps import SaveIqAscii as BaseSaveIqAscii
from sans_reduction_steps import SensitivityCorrection as BaseSensitivityCorrection
from reduction import extract_workspace_name, find_file, find_data
from eqsans_load import LoadRun

# Mantid imports
from MantidFramework import *
from mantidsimple import *
    
class SubtractDarkCurrent(ReductionStep):
    """
        Subtract the dark current from the input workspace.
        TODO: get rid of this by putting the EQSANSDarkCurrentSubtraction in the reduction table
    """
    def __init__(self, dark_current_file):
        self._dark_current_file = dark_current_file
        
    def execute(self, reducer, workspace):
        """
            Subtract the dark current from the input workspace.
            @param reducer: Reducer object for which this step is executed
            @param workspace: input workspace
        """
        alg = EQSANSDarkCurrentSubtraction(InputWorkspace=workspace, Filename=self._dark_current_file, OutputWorkspace=workspace,
                                     ReductionTableWorkspace=reducer.get_reduction_table_name())        
        return alg.getPropertyValue("OutputMessage")
    
class AzimuthalAverageByFrame(WeightedAzimuthalAverage):
    """
        ReductionStep class that performs azimuthal averaging
        and transforms the 2D reduced data set into I(Q).
        Done for each frame independently.
    """
    def __init__(self, binning=None, suffix="_Iq", error_weighting=False, n_bins=100, n_subpix=1, log_binning=False, scale=False):
        super(AzimuthalAverageByFrame, self).__init__(binning, suffix, error_weighting, n_bins, n_subpix, log_binning)
        self._is_frame_skipping = False
        self._scale = scale
        self._tolerance = 1.3
        self._compute_resolution = False
        self._sample_aperture_radius = 5.0
        self._independent_binning = True
        
    def use_independent_binning(self, flag=True):
        self._independent_binning = flag
        
    def compute_resolution(self, sample_aperture_diameter=10.0):
        """
            Sets the flag to compute the Q resolution
            @param sample_aperture_diameter: diameter of the sample aperture, in mm
        """
        self._compute_resolution = True
        self._sample_aperture_radius = sample_aperture_diameter/2.0
        
    def get_output_workspace(self, workspace):
        if not self._is_frame_skipping:
            return workspace+self._suffix
        return [workspace+'_frame1'+self._suffix, workspace+'_frame2'+self._suffix]
        
    def execute(self, reducer, workspace):
        # We will need the pixel dimensions to compute the Q resolution        
        pixel_size_x = mtd[workspace].getInstrument().getNumberParameter("x-pixel-size")[0]
        pixel_size_y = mtd[workspace].getInstrument().getNumberParameter("y-pixel-size")[0]
        
        # Get the source aperture radius
        source_aperture_radius = 10.0
        if mtd[workspace].getRun().hasProperty("source-aperture-diameter"):
            source_aperture_radius = mtd[workspace].getRun().getProperty("source-aperture-diameter").value/2.0

        if mtd[workspace].getRun().hasProperty("is_frame_skipping") \
            and mtd[workspace].getRun().getProperty("is_frame_skipping").value==0:
            self._is_frame_skipping = False
            output_str = super(AzimuthalAverageByFrame, self).execute(reducer, workspace)
            if self._compute_resolution:
                EQSANSResolution(InputWorkspace=workspace+self._suffix, 
                                  ReducedWorkspace=workspace, OutputBinning=self._binning,
                                  PixelSizeX=pixel_size_x, PixelSizeY=pixel_size_y,
                                  SourceApertureRadius=source_aperture_radius,
                                  SampleApertureRadius=self._sample_aperture_radius)
            return output_str
        
        self._is_frame_skipping = True
        
        # Get wavelength bands
        # First frame
        wl_min_f1 = None
        wl_max_f1 = None
        if mtd[workspace].getRun().hasProperty("wavelength_min"):
            wl_min_f1 = mtd[workspace].getRun().getProperty("wavelength_min").value
        if mtd[workspace].getRun().hasProperty("wavelength_max"):
            wl_max_f1 = mtd[workspace].getRun().getProperty("wavelength_max").value
        if wl_min_f1 is None and wl_max_f1 is None:
            raise RuntimeError, "Could not get the wavelength band for frame 1"
        
        # Second frame
        wl_min_f2 = None
        wl_max_f2 = None
        if mtd[workspace].getRun().hasProperty("wavelength_min_frame2"):
            wl_min_f2 = mtd[workspace].getRun().getProperty("wavelength_min_frame2").value
        if mtd[workspace].getRun().hasProperty("wavelength_max_frame2"):
            wl_max_f2 = mtd[workspace].getRun().getProperty("wavelength_max_frame2").value
        if wl_min_f2 is None and wl_max_f2 is None:
            raise RuntimeError, "Could not get the wavelength band for frame 2"
        
        # Compute binning
        if self._independent_binning:
            self._binning = None
        else:
            qmin, qstep, qmax = self._get_binning(reducer, workspace, min(wl_min_f1, wl_min_f2), max(wl_max_f1, wl_max_f2))
            self._binning = "%g, %g, %g" % (qmin, qstep, qmax)
        # Average second frame
        Rebin(workspace, workspace+'_frame2', "%4.2f,%4.2f,%4.2f" % (wl_min_f2, 0.1, wl_max_f2), False)
        ReplaceSpecialValues(workspace+'_frame2', workspace+'_frame2', NaNValue=0.0,NaNError=0.0)
        
        super(AzimuthalAverageByFrame, self).execute(reducer, workspace+'_frame2')
        if self._compute_resolution:
            EQSANSResolution(InputWorkspace=workspace+'_frame2'+self._suffix, 
                              ReducedWorkspace=workspace, OutputBinning=self._binning,
                              MinWavelength=wl_min_f2, MaxWavelength=wl_max_f2,
                              PixelSizeX=pixel_size_x, PixelSizeY=pixel_size_y,
                              SourceApertureRadius=source_aperture_radius,
                              SampleApertureRadius=self._sample_aperture_radius)                                                                        
            
        # Average first frame
        if self._independent_binning:
            self._binning = None
        Rebin(workspace, workspace+'_frame1', "%4.2f,%4.2f,%4.2f" % (wl_min_f1, 0.1, wl_max_f1), False)
        ReplaceSpecialValues(workspace+'_frame1', workspace+'_frame1', NaNValue=0.0,NaNError=0.0)
        
        super(AzimuthalAverageByFrame, self).execute(reducer, workspace+'_frame1')
        
        # Workspace operations do not keep Dx, so scale frame 1 before putting
        # in the Q resolution
        # Scale frame 1 to frame 2
        if self._scale:
            iq_f1 = mtd[workspace+'_frame1'+self._suffix].dataY(0)
            iq_f2 = mtd[workspace+'_frame2'+self._suffix].dataY(0)
            q_f1 = mtd[workspace+'_frame1'+self._suffix].dataX(0)
            q_f2 = mtd[workspace+'_frame2'+self._suffix].dataX(0)
            
            scale_f1 = 0.0
            scale_f2 = 0.0
            scale_factor = 1.0
            qmin = None
            qmax = None
            
            # Get Q range to consider
            for i in range(len(iq_f1)):
                if iq_f1[i]<=0:
                    break
                else:
                    if qmin is None or q_f1[i]<qmin: qmin = q_f1[i]
                    if qmax is None or q_f1[i]>qmax: qmax = q_f1[i]
            
            qmin2 = q_f2[len(q_f2)-1]
            qmax2 = q_f2[0]
            for i in range(len(iq_f2)):
                if iq_f2[i]<=0:
                    break
                else:
                    if qmin2 is None or q_f2[i]<qmin2: qmin2 = q_f2[i]
                    if qmax2 is None or q_f2[i]>qmax2: qmax2 = q_f2[i]
                    
            qmin = max(qmin, qmin2)
            qmax = min(qmax, qmax2)

            for i in range(len(iq_f1)):
                if q_f1[i]>=qmin and q_f1[i]<=qmax:
                    scale_f1 += iq_f1[i]*(q_f1[i+1]-q_f1[i])
            
            for i in range(len(iq_f2)):
                if q_f2[i]>=qmin and q_f2[i]<=qmax:
                    scale_f2 += iq_f2[i]*(q_f2[i+1]-q_f2[i])
            
            if scale_f1>0 and scale_f2>0:
                scale_factor = scale_f2/scale_f1
            
            Scale(InputWorkspace=workspace+'_frame1'+self._suffix, OutputWorkspace=workspace+'_frame1'+self._suffix, Factor=scale_factor, Operation="Multiply")
            
        if self._compute_resolution:
            EQSANSResolution(InputWorkspace=workspace+'_frame1'+self._suffix, 
                             ReducedWorkspace=workspace, OutputBinning=self._binning,
                             MinWavelength=wl_min_f1, MaxWavelength=wl_max_f1,
                             PixelSizeX=pixel_size_x, PixelSizeY=pixel_size_y,
                             SourceApertureRadius=source_aperture_radius,
                             SampleApertureRadius=self._sample_aperture_radius)       
                    
        # Add output workspaces to the list of important output workspaces
        for item in self.get_output_workspace(workspace):
            if item in reducer.output_workspaces:
                reducer.output_workspaces.remove(item)
        reducer.output_workspaces.append(self.get_output_workspace(workspace))                

        # Clean up 
        for ws in [workspace+'_frame1', workspace+'_frame2']:
            if mtd.workspaceExists(ws):
                mtd.deleteWorkspace(ws)
        
        return "Performed radial averaging for two frames"
        
    def get_data(self, workspace):
        if not self._is_frame_skipping:
            return super(AzimuthalAverageByFrame, self).get_data(workspace)
        
        class DataSet(object):
            x=[]
            y=[]
            dy=[]
        
        d = DataSet()
        d.x = mtd[self.get_output_workspace(workspace)[0]].dataX(0)[1:]
        d.y = mtd[self.get_output_workspace(workspace)[0]].dataY(0)
        d.dx = mtd[self.get_output_workspace(workspace)[0]].dataE(0)
        return d
        
class DirectBeamTransmission(SingleFrameDirectBeamTransmission):
    """
        Calculate transmission using the direct beam method
    """
    def __init__(self, sample_file, empty_file, beam_radius=3.0, theta_dependent=True, dark_current=None, 
                 use_sample_dc=False, combine_frames=True):
        super(DirectBeamTransmission, self).__init__(sample_file, empty_file, beam_radius, theta_dependent, 
                                                     dark_current, use_sample_dc)
        ## Whether of not we need to combine the two frames in frame-skipping mode when performing the fit
        self._combine_frames = combine_frames
        
    def set_combine_frames(self, combine_frames=True):
        self._combine_frames = combine_frames
        
    def get_transmission(self):
        #mtd.sendLogMessage("TOF SANS doesn't have a single zero-angle transmission. DirectBeamTransmission.get_transmission() should not be called.")
        return [0, 0]
        
    def execute(self, reducer, workspace):
        if self._combine_frames or \
            (mtd[workspace].getRun().hasProperty("is_frame_skipping") \
             and mtd[workspace].getRun().getProperty("is_frame_skipping").value==0):
            return super(DirectBeamTransmission, self).execute(reducer, workspace)
    
        output_str = ""
        if self._transmission_ws is None:
            self._transmission_ws = "transmission_fit_"+workspace
            # Load data files
            sample_mon_ws, empty_mon_ws, first_det, output_str = self._load_monitors(reducer, workspace)
            
            def _crop_and_compute(wl_min_prop, wl_max_prop, suffix):
                # Get the wavelength band from the run properties
                if mtd[workspace].getRun().hasProperty(wl_min_prop):
                    wl_min = mtd[workspace].getRun().getProperty(wl_min_prop).value
                else:
                    raise RuntimeError, "DirectBeamTransmission could not retrieve the %s property" % wl_min_prop
                
                if mtd[workspace].getRun().hasProperty(wl_max_prop):
                    wl_max = mtd[workspace].getRun().getProperty(wl_max_prop).value
                else:
                    raise RuntimeError, "DirectBeamTransmission could not retrieve the %s property" % wl_max_prop
                
                Rebin(workspace, workspace+suffix, "%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max), False)
                Rebin(sample_mon_ws, sample_mon_ws+suffix, "%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max), False)
                Rebin(empty_mon_ws, empty_mon_ws+suffix, "%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max), False)
                self._calculate_transmission(sample_mon_ws+suffix, empty_mon_ws+suffix, first_det, self._transmission_ws+suffix)
                RebinToWorkspace(self._transmission_ws+suffix, workspace, OutputWorkspace=self._transmission_ws+suffix)
                RebinToWorkspace(self._transmission_ws+suffix+'_unfitted', workspace, OutputWorkspace=self._transmission_ws+suffix+'_unfitted')
                return self._transmission_ws+suffix
                
            # First frame
            trans_frame_1 = _crop_and_compute("wavelength_min", "wavelength_max", "_frame1")
            
            # Second frame
            trans_frame_2 = _crop_and_compute("wavelength_min_frame2", "wavelength_max_frame2", "_frame2")
            
            Plus(trans_frame_1, trans_frame_2, self._transmission_ws)
            Plus(trans_frame_1+'_unfitted', trans_frame_2+'_unfitted', self._transmission_ws+'_unfitted')

            # Clean up            
            for ws in [trans_frame_1, trans_frame_2, 
                       trans_frame_1+'_unfitted', trans_frame_2+'_unfitted',
                       sample_mon_ws, empty_mon_ws]:
                if mtd.workspaceExists(ws):
                    mtd.deleteWorkspace(ws)
            
        # Add output workspace to the list of important output workspaces
        reducer.output_workspaces.append([self._transmission_ws, self._transmission_ws+'_unfitted'])
            
        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        if self._theta_dependent:
            ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                        TransmissionWorkspace=self._transmission_ws, 
                                        OutputWorkspace=workspace)          
        else:
            Divide(workspace, self._transmission_ws, workspace)  
        
        return "Transmission correction applied for both frame independently [%s]\n%s\n" % (self._transmission_ws, output_str)
    
class SaveIqAscii(BaseSaveIqAscii):
    """
        Save the reduced data to ASCII files
    """
    def __init__(self, process=None):
        super(SaveIqAscii, self).__init__(process)
        
    def execute(self, reducer, workspace):
        log_text = super(SaveIqAscii, self).execute(reducer, workspace)
        
        # Determine which directory to use
        output_dir = reducer._data_path
        if reducer._output_path is not None:
            if os.path.isdir(reducer._output_path):
                output_dir = reducer._output_path
            else:
                raise RuntimeError, "SaveIqAscii could not save in the following directory: %s" % reducer._output_path
            
        if reducer._two_dim_calculator is not None:
            if not hasattr(reducer._two_dim_calculator, "get_output_workspace"):
                
                for output_ws in [workspace+'_Iqxy', workspace+'_frame1_Iqxy', workspace+'_frame2_Iqxy']:
                    if mtd.workspaceExists(output_ws):
                        filename = os.path.join(output_dir, output_ws+'.dat')
                        SaveNISTDAT(InputWorkspace=output_ws, Filename=filename)
                        
                        if len(log_text)>0:
                            log_text += '\n'
                        log_text += "I(Qx,Qy) for %s saved in %s" % (output_ws, filename)
            
        return log_text
            
class SensitivityCorrection(BaseSensitivityCorrection):
    """
        Perform sensitivity correction as a function of wavelength
    """
    def __init__(self, flood_data, min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None, 
                 beam_center=None, use_sample_dc=False):
        super(SensitivityCorrection, self).__init__(flood_data=flood_data, 
                                                    min_sensitivity=min_sensitivity, max_sensitivity=max_sensitivity, 
                                                    dark_current=dark_current, beam_center=beam_center, use_sample_dc=use_sample_dc)
    
    def execute(self, reducer, workspace):
        # Perform standard sensitivity correction
        # If the sensitivity correction workspace exists, just apply it. Otherwise create it.      
        output_str = "   Using data set: %s" % extract_workspace_name(self._flood_data)
        if self._efficiency_ws is None:
            self._compute_efficiency(reducer, workspace)
            
        # Modify for wavelength dependency of the efficiency of the detector tubes
        EQSANSSensitivityCorrection(InputWorkspace=workspace, EfficiencyWorkspace=self._efficiency_ws,     
                                    Factor=0.95661, Error=0.005, OutputWorkspace=workspace,
                                    OutputEfficiencyWorkspace="__wl_efficiency")
        
        # Copy over the efficiency's masked pixels to the reduced workspace
        masked_detectors = GetMaskedDetectors(self._efficiency_ws)
        MaskDetectors(workspace, None, masked_detectors.getPropertyValue("DetectorList"))        
        
        return "Wavelength-dependent sensitivity correction applied\n%s\n" % output_str
        