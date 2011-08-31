"""
    Implementation of reduction steps for SANS
"""
import os
import sys
import math
import pickle
from reduction import ReductionStep
from reduction import extract_workspace_name, find_data
from reduction import validate_step

# Mantid imports
from mantidsimple import *
    
class BaseBeamFinder(ReductionStep):
    """
        Base beam finder. Holds the position of the beam center
        and the algorithm for calculates it using the beam's
        displacement under gravity
    """
    def __init__(self, beam_center_x=None, beam_center_y=None):
        """
            Initial beam center is given in pixel coordinates
            @param beam_center_x: pixel position of the beam in x
            @param beam_center_y: pixel position of the beam in y
        """
        super(BaseBeamFinder, self).__init__()
        self._beam_center_x = beam_center_x
        self._beam_center_y = beam_center_y
        self._beam_radius = None
        
        # Define detector edges to be be masked
        self._x_mask_low = 0
        self._x_mask_high = 0
        self._y_mask_low = 0
        self._y_mask_high = 0 
        
    def set_masked_edges(self, x_low=0, x_high=0, y_low=0, y_high=0):
        """
            Sets the number of pixels to mask on the edges before
            doing the center of mass computation
        """
        self._x_mask_low = x_low
        self._x_mask_high = x_high
        self._y_mask_low = y_low
        self._y_mask_high = y_high
        return self
        
    def get_beam_center(self):
        """
            Returns the beam center
        """
        return [self._beam_center_x, self._beam_center_y]
    
    def execute(self, reducer, workspace=None):
        return "Beam Center set at: %s %s" % (str(self._beam_center_x), str(self._beam_center_y))
        
    def _find_beam(self, direct_beam, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        if self._beam_center_x is not None and self._beam_center_y is not None:
            return "Using Beam Center at: %g %g" % (self._beam_center_x, self._beam_center_y)
        
        # Load the file to extract the beam center from, and process it.
        filepath = find_data(self._datafile, instrument=reducer.instrument.name())
        
        # Check whether that file was already meant to be processed
        workspace_default = "beam_center_"+extract_workspace_name(filepath)
        workspace = workspace_default
        if filepath in reducer._data_files.values():
            for k in reducer._data_files.iterkeys():
                if reducer._data_files[k]==filepath:
                    workspace = k                    
                   
        reducer._data_loader.clone(filepath).execute(reducer, workspace)

        # Mask edges of the detector
        # This is here mostly to allow a direct comparison with old HFIR code and 
        # ensure that we reproduce the same results
        if self._x_mask_low!=0 or self._x_mask_high!=0 or self._y_mask_low!=0 or self._y_mask_high!=0: 
            mantid.sendLogMessage("Masking beam data: %g %g %g %g" % (self._x_mask_low, self._x_mask_high, self._y_mask_low, self._y_mask_high))  
            mask = Mask()
            mask.ignore_run_properties()
            mask.mask_edges(self._x_mask_low, self._x_mask_high, self._y_mask_low, self._y_mask_high)
            mask.execute(reducer, workspace)
                        
        # We must convert the beam radius from pixels to meters
        pixel_size_x = mtd[workspace].getInstrument().getNumberParameter("x-pixel-size")[0]
        if self._beam_radius is not None:
            self._beam_radius *= pixel_size_x/1000.0
        beam_center = FindCenterOfMassPosition(workspace,
                                               Output = None,
                                               DirectBeam = direct_beam,
                                               BeamRadius = self._beam_radius)
        ctr_str = beam_center.getPropertyValue("CenterOfMass")
        ctr = ctr_str.split(',')
        mantid.sendLogMessage("Beam coordinate in real-space: %s" % str(ctr))  
        
        # Compute the relative distance to the current beam center in pixels
        # Move detector array to correct position. Do it here so that we don't need to
        # move it if we need to load that data set for analysis later.
        # Note: the position of the detector in Z is now part of the load
        # Note: if the relative translation is correct, we shouldn't have to keep 
        #       the condition below. Check with EQSANS data (where the beam center and the data are the same workspace)
        if not workspace == workspace_default:
            if self._beam_center_x is None or self._beam_center_y is None:
                old_ctr = [0.0,0.0]
            else:
                old_ctr = reducer.instrument.get_coordinate_from_pixel(self._beam_center_x, self._beam_center_y, workspace)
            detector_ID=mtd[workspace].getInstrument().getStringParameter("detector-name")[0]
            MoveInstrumentComponent(workspace, detector_ID, 
                                    X = old_ctr[0]-float(ctr[0]),
                                    Y = old_ctr[1]-float(ctr[1]),
                                    RelativePosition="1")        

        [self._beam_center_x, self._beam_center_y] = reducer.instrument.get_pixel_from_coordinate(float(ctr[0]), float(ctr[1]), workspace)
        mantid[workspace].getRun().addProperty_dbl("beam_center_x", self._beam_center_x, 'pixel', True)            
        mantid[workspace].getRun().addProperty_dbl("beam_center_y", self._beam_center_y, 'pixel', True)                                
        
        return "Beam Center found at: %g %g" % (self._beam_center_x, self._beam_center_y)


class ScatteringBeamCenter(BaseBeamFinder):
    """
        Find the beam center using the scattering data
    """  
    def __init__(self, datafile, beam_radius=3):
        """
            @param datafile: beam center data file
            @param beam_radius: beam radius in pixels
        """
        super(ScatteringBeamCenter, self).__init__()
        ## Location of the data file used to find the beam center
        self._datafile = datafile
        ## Beam radius in pixels
        self._beam_radius = beam_radius
        
    def execute(self, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        return super(ScatteringBeamCenter, self)._find_beam(False, reducer, workspace)

class DirectBeamCenter(BaseBeamFinder):
    """
        Find the beam center using the direct beam
    """  
    def __init__(self, datafile):
        super(DirectBeamCenter, self).__init__()
        ## Location of the data file used to find the beam center
        self._datafile = datafile
        
    def execute(self, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        return super(DirectBeamCenter, self)._find_beam(True, reducer, workspace)

class BaseTransmission(ReductionStep):
    """
        Base transmission. Holds the transmission value
        as well as the algorithm for calculating it.
        TODO: ISIS doesn't use ApplyTransmissionCorrection, perhaps it's in Q1D, can we remove it from here?
    """
    def __init__(self, trans=0.0, error=0.0, theta_dependent=True):
        super(BaseTransmission, self).__init__()
        self._trans = float(trans)
        self._error = float(error)
        self._theta_dependent = theta_dependent
        self._dark_current_data = None
        
    def set_theta_dependence(self, theta_dependence=True):
        """
            Set the flag for whether or not we want the full theta-dependence
            included in the correction. Setting this flag to false will result
            in simply dividing by the zero-angle transmission.
            @param theta_dependence: theta-dependence included if True
        """
        self._theta_dependent = theta_dependence
        
    def set_dark_current(self, dark_current=None):
        """
            Set the dark current data file to be subtracted from each tranmission data file
            @param dark_current: path to dark current data file
        """
        self._dark_current_data = dark_current
                
    def get_transmission(self):
        return [self._trans, self._error]
    
    def execute(self, reducer, workspace):
        if self._theta_dependent:
            ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                        TransmissionValue=self._trans,
                                        TransmissionError=self._error, 
                                        OutputWorkspace=workspace) 
        else:
            CreateSingleValuedWorkspace("transmission", self._trans, self._error)
            Divide(workspace, "transmission", workspace)
        
        return "Transmission correction applied for T = %g +- %g" % (self._trans, self._error)
  
class BeamSpreaderTransmission(BaseTransmission):
    """
        Calculate transmission using the beam-spreader method
    """
    def __init__(self, sample_spreader, direct_spreader,
                       sample_scattering, direct_scattering,
                       spreader_transmission=1.0, spreader_transmission_err=0.0,
                       theta_dependent=True, dark_current=None): 
        super(BeamSpreaderTransmission, self).__init__(theta_dependent=theta_dependent)
        self._sample_spreader = sample_spreader
        self._direct_spreader = direct_spreader
        self._sample_scattering = sample_scattering
        self._direct_scattering = direct_scattering
        self._spreader_transmission = spreader_transmission
        self._spreader_transmission_err = spreader_transmission_err
        self._dark_current_data = dark_current
        ## Transmission workspace (output of transmission calculation)
        self._transmission_ws = None
        
    def execute(self, reducer, workspace=None):
        """
            Calculate transmission and apply correction
            @param reducer: Reducer object for which this step is executed
            @param workspace: workspace to apply correction to
        """
        if self._transmission_ws is None:
            # 1- Compute zero-angle transmission correction (Note: CalcTransCoef)
            self._transmission_ws = "transmission_fit_"+workspace
            
            sample_spreader_ws = "_trans_sample_spreader"
            filepath = find_data(self._sample_spreader, instrument=reducer.instrument.name())
            reducer._data_loader.clone(filepath).execute(reducer, sample_spreader_ws)
            
            direct_spreader_ws = "_trans_direct_spreader"
            filepath = find_data(self._direct_spreader, instrument=reducer.instrument.name())
            reducer._data_loader.clone(filepath).execute(reducer, direct_spreader_ws)
            
            sample_scatt_ws = "_trans_sample_scatt"
            filepath = find_data(self._sample_scattering, instrument=reducer.instrument.name())
            reducer._data_loader.clone(filepath).execute(reducer, sample_scatt_ws)
            
            direct_scatt_ws = "_trans_direct_scatt"
            filepath = find_data(self._direct_scattering, instrument=reducer.instrument.name())
            reducer._data_loader.clone(filepath).execute(reducer, direct_scatt_ws)
            
            # Subtract dark current
            if self._dark_current_data is not None and len(str(self._dark_current_data).strip())>0:
                reducer._dark_current_subtracter_class(self._dark_current_data).execute(reducer, sample_spreader_ws)
                reducer._dark_current_subtracter_class(self._dark_current_data).execute(reducer, direct_spreader_ws)
                reducer._dark_current_subtracter_class(self._dark_current_data).execute(reducer, sample_scatt_ws)
                reducer._dark_current_subtracter_class(self._dark_current_data).execute(reducer, direct_scatt_ws)
                        
            # Get normalization for transmission calculation
            norm_detector = reducer.instrument.get_incident_mon(workspace, reducer.NORMALIZATION_TIME)
            if reducer._normalizer is not None:
                norm_option = reducer._normalizer.get_normalization_spectrum()
                norm_detector = reducer.instrument.get_incident_mon(workspace, norm_option)
            
            # Calculate transmission. Use the reduction method's normalization channel (time or beam monitor)
            # as the monitor channel.
            CalculateTransmissionBeamSpreader(SampleSpreaderRunWorkspace=sample_spreader_ws, 
                                              DirectSpreaderRunWorkspace=direct_spreader_ws,
                                              SampleScatterRunWorkspace=sample_scatt_ws, 
                                              DirectScatterRunWorkspace=direct_scatt_ws, 
                                              IncidentBeamMonitor=norm_detector,
                                              OutputWorkspace=self._transmission_ws,
                                              SpreaderTransmissionValue=str(self._spreader_transmission), 
                                              SpreaderTransmissionError=str(self._spreader_transmission_err))

        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        if self._theta_dependent:
            ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                        TransmissionWorkspace=self._transmission_ws, 
                                        OutputWorkspace=workspace)          
        else:
            Divide(workspace, self._transmission_ws, workspace)  
        
        trans_ws = mtd[self._transmission_ws]
        self._trans = trans_ws.dataY(0)[0]
        self._error = trans_ws.dataE(0)[0]

        return "Transmission correction applied for T = %g +- %g" % (self._trans, self._error)
          
            
class DirectBeamTransmission(BaseTransmission):
    """
        Calculate transmission using the direct beam method
    """
    def __init__(self, sample_file, empty_file, beam_radius=3.0, theta_dependent=True, dark_current=None, use_sample_dc=False):
        super(DirectBeamTransmission, self).__init__(theta_dependent=theta_dependent)
        ## Location of the data files used to calculate transmission
        self._sample_file = sample_file
        self._empty_file = empty_file
        ## Dark current data file
        self._dark_current_data = dark_current
        ## Radius of the beam
        self._beam_radius = beam_radius
        ## Transmission workspace (output of transmission calculation)
        self._transmission_ws = None
        ## Flag to tell us whether we should normalise to the monitor channel
        self._monitor_det_ID = None
        self._use_sample_dc = use_sample_dc        
        
    def _load_monitors(self, reducer, workspace):
        """
            Load files necessary to compute transmission.
            Return their names.
        """
        sample_ws = "__transmission_sample"
        filepath = find_data(self._sample_file, instrument=reducer.instrument.name())
        reducer._data_loader.clone(data_file=filepath).execute(reducer, sample_ws)
        output_str = "   Sample: %s\n" % extract_workspace_name(self._sample_file)
        
        empty_ws = "__transmission_empty"
        filepath = find_data(self._empty_file, instrument=reducer.instrument.name())
        reducer._data_loader.clone(data_file=filepath).execute(reducer, empty_ws)
        output_str += "   Empty: %s" % extract_workspace_name(self._empty_file)
        
        # Subtract dark current
        if self._use_sample_dc:
            if reducer._dark_current_subtracter is not None:
                partial_out = reducer._dark_current_subtracter.execute(reducer, sample_ws)
                partial_out2 = reducer._dark_current_subtracter.execute(reducer, empty_ws)
                partial_out = "\n   Sample: %s\n   Empty: %s" % (partial_out, partial_out2)
                partial_out.replace('\n', '   \n')
                output_str += partial_out
        
        elif self._dark_current_data is not None and len(str(self._dark_current_data).strip())>0:
            partial_out = reducer._dark_current_subtracter_class(self._dark_current_data).execute(reducer, sample_ws)
            partial_out2 = reducer._dark_current_subtracter_class(self._dark_current_data).execute(reducer, empty_ws)
            partial_out = "\n%s\n%s" % (partial_out, partial_out2)
            partial_out.replace('\n', '   \n')
            output_str += partial_out
            
        # Find which pixels to sum up as our "monitor". At this point we have moved the detector
        # so that the beam is at (0,0), so all we need is to sum the area around that point.
        #TODO: in IGOR, the error-weighted average is computed instead of simply summing up the pixels
        pixel_size_x = mtd[workspace].getInstrument().getNumberParameter("x-pixel-size")[0]
        cylXML = '<infinite-cylinder id="transmission_monitor">' + \
                   '<centre x="0.0" y="0.0" z="0.0" />' + \
                   '<axis x="0.0" y="0.0" z="1.0" />' + \
                   '<radius val="%12.10f" />' % (self._beam_radius*pixel_size_x/1000.0) + \
                 '</infinite-cylinder>\n'
                 
        det_finder = FindDetectorsInShape(Workspace=workspace, ShapeXML=cylXML)
        det_list = det_finder.getPropertyValue("DetectorList")
        
        first_det_str = det_list.split(',')[0]
        if len(first_det_str.strip())==0:
            raise RuntimeError, "Could not find detector pixels near the beam center: check that the beam center is placed at (0,0)."
        first_det = int(first_det_str)
        
        empty_mon_ws = "__empty_mon"
        sample_mon_ws = "__sample_mon"
        GroupDetectors(InputWorkspace=empty_ws,  OutputWorkspace=empty_mon_ws,  DetectorList=det_list, KeepUngroupedSpectra="1")
        GroupDetectors(InputWorkspace=sample_ws, OutputWorkspace=sample_mon_ws, DetectorList=det_list, KeepUngroupedSpectra="1")
        
        #TODO: check that both workspaces have the same masked spectra
        
        # Get normalization for transmission calculation
        self._monitor_det_ID = reducer.instrument.get_incident_mon(workspace, reducer.NORMALIZATION_TIME)
        if reducer._normalizer is not None:
            norm_option = reducer._normalizer.get_normalization_spectrum()
            self._monitor_det_ID = reducer.instrument.get_incident_mon(workspace, norm_option)
            if self._monitor_det_ID<0:
                reducer._normalizer.execute(reducer, empty_mon_ws)
                reducer._normalizer.execute(reducer, sample_mon_ws)
        
        # Calculate transmission. Use the reduction method's normalization channel (time or beam monitor)
        # as the monitor channel.
        RebinToWorkspace(empty_mon_ws, sample_mon_ws, OutputWorkspace=empty_mon_ws)

        # Clean up
        reducer._data_loader.__class__.delete_workspaces(empty_ws)
        reducer._data_loader.__class__.delete_workspaces(sample_ws)
            
        return sample_mon_ws, empty_mon_ws, first_det, output_str
        
    def _calculate_transmission(self, sample_mon_ws, empty_mon_ws, first_det, trans_output_workspace):
        """
            Compute zero-angle transmission
        """
        if self._monitor_det_ID is not None and self._monitor_det_ID>=0:
            CalculateTransmission(DirectRunWorkspace=empty_mon_ws, SampleRunWorkspace=sample_mon_ws, 
                                  OutputWorkspace=trans_output_workspace,
                                  IncidentBeamMonitor=str(self._monitor_det_ID), 
                                  TransmissionMonitor=str(first_det),
                                  OutputUnfittedData=True)
        else:
            CalculateTransmission(DirectRunWorkspace=empty_mon_ws, SampleRunWorkspace=sample_mon_ws, 
                                  OutputWorkspace=trans_output_workspace,
                                  TransmissionMonitor=str(first_det),
                                  OutputUnfittedData=True)
        
        for ws in [empty_mon_ws, sample_mon_ws]:
            if mtd.workspaceExists(ws):
                mtd.deleteWorkspace(ws)          
                
    def execute(self,reducer, workspace=None):
        """
            Calculate transmission and apply correction
            @param reducer: Reducer object for which this step is executed
            @param workspace: workspace to apply correction to
        """
        output_str = ""
        if self._transmission_ws is None:
            # 1- Compute zero-angle transmission correction (Note: CalcTransCoef)
            self._transmission_ws = "transmission_fit_"+workspace
            # Load data files
            sample_mon_ws, empty_mon_ws, first_det, output_str = self._load_monitors(reducer, workspace)
            self._calculate_transmission(sample_mon_ws, empty_mon_ws, first_det, self._transmission_ws)
            
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

        trans_ws = mtd[self._transmission_ws]
        self._trans = trans_ws.dataY(0)[0]
        self._error = trans_ws.dataE(0)[0]
        if len(trans_ws.dataY(0))==1:
            output_str = "%s   T = %6.2g += %6.2g\n" % (output_str, self._trans, self._error)
        return "Transmission correction applied [%s]\n%s\n" % (self._transmission_ws, output_str)
            

class SubtractDarkCurrent(ReductionStep):
    """
        ReductionStep class that subtracts the dark current from the data.
        The loaded dark current is stored by the ReductionStep object so that the
        subtraction can be applied to multiple data sets without reloading it.
    """
    def __init__(self, dark_current_file, timer_ws=None):
        """
            @param timer_ws: if provided, will be used to scale the dark current
        """
        super(SubtractDarkCurrent, self).__init__()
        self._timer_ws = timer_ws
        self._dark_current_file = dark_current_file
        self._dark_current_ws = None
        
    def execute(self, reducer, workspace):
        """
            Subtract the dark current from the input workspace.
            If no timer workspace is provided, the counting time will be extracted
            from the input workspace.
            
            @param reducer: Reducer object for which this step is executed
            @param workspace: input workspace
        """
        # Sanity check
        if self._dark_current_file is None:
            raise RuntimeError, "SubtractDarkCurrent called with no defined dark current file"

        # Check whether the dark current was already loaded, otherwise load it
        # Load dark current, which will be used repeatedly
        if self._dark_current_ws is None:
            filepath = find_data(self._dark_current_file, instrument=reducer.instrument.name())
            self._dark_current_ws = "_dark_"+extract_workspace_name(filepath)
            reducer._data_loader.clone(filepath).execute(reducer, self._dark_current_ws)
            #Load(filepath, self._dark_current_ws)
            
            # Normalize the dark current data to counting time
            darktimer_ws = self._dark_current_ws+"_timer"
            CropWorkspace(self._dark_current_ws, darktimer_ws,
                          StartWorkspaceIndex = str(reducer.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(reducer.NORMALIZATION_TIME))        
            
            Divide(self._dark_current_ws, darktimer_ws, self._dark_current_ws)      
    
        # If no timer workspace was provided, get the counting time from the data
        timer_ws = self._timer_ws
        if timer_ws is None:
            timer_ws = "_tmp_timer"     
            CropWorkspace(workspace, timer_ws,
                          StartWorkspaceIndex = str(reducer.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(reducer.NORMALIZATION_TIME))  
            
        # Scale the stored dark current by the counting time
        scaled_dark_ws = "_scaled_dark_current"
        Multiply(self._dark_current_ws, timer_ws, scaled_dark_ws)
        
        # Set time and monitor channels to zero, so that we subtract only detectors
        mtd[scaled_dark_ws].dataY(reducer.NORMALIZATION_TIME)[0] = 0
        mtd[scaled_dark_ws].dataE(reducer.NORMALIZATION_TIME)[0] = 0
        mtd[scaled_dark_ws].dataY(reducer.NORMALIZATION_MONITOR)[0] = 0
        mtd[scaled_dark_ws].dataE(reducer.NORMALIZATION_MONITOR)[0] = 0
        
        # Perform subtraction
        Minus(workspace, scaled_dark_ws, workspace)  
        if mtd.workspaceExists(scaled_dark_ws):
            mtd.deleteWorkspace(scaled_dark_ws)
        
        return "Dark current subtracted [%s]" % extract_workspace_name(self._dark_current_file)
          
class LoadRun(ReductionStep):
    """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
    """
    #TODO: Move this to HFIR-specific module 
    def __init__(self, datafile=None, sample_det_dist=None, sample_det_offset=0, beam_center=None,
                 wavelength=None, wavelength_spread=None):
        """
            @param datafile: file path of data to load
            @param sample_det_dist: sample-detector distance [mm] (will overwrite header info)
            @param sample_det_offset: sample-detector distance offset [mm]
            @param beam_center: [center_x, center_y] [pixels]
            @param wavelength: if provided, wavelength will be fixed at that value [A]
            @param wavelength_spread: if provided along with wavelength, it will be fixed at that value [A] 
        """
        super(LoadRun, self).__init__()
        self._data_file = datafile
        self.set_sample_detector_distance(sample_det_dist)
        self.set_sample_detector_offset(sample_det_offset)
        self.set_beam_center(beam_center)
        self.set_wavelength(wavelength, wavelength_spread)
        
    def clone(self, data_file=None):
        if data_file is None:
            data_file = self._data_file
        return LoadRun(datafile=data_file, sample_det_dist=self._sample_det_dist,
                       sample_det_offset=self._sample_det_offset, beam_center=self._beam_center,
                       wavelength=self._wavelength, wavelength_spread=self._wavelength_spread)
        
    def set_wavelength(self, wavelength=None, wavelength_spread=None):
        if wavelength is not None and type(wavelength) != int and type(wavelength) != float:
            raise RuntimeError, "LoadRun.set_wavelength expects a float: %s" % str(wavelength)
        self._wavelength = wavelength

        if wavelength_spread is not None and type(wavelength_spread) != int and type(wavelength_spread) != float:
            raise RuntimeError, "LoadRun.set_wavelength expects a float for wavelength_spread: %s" % str(wavelength_spread)
        self._wavelength_spread = wavelength_spread
        
    def set_sample_detector_distance(self, distance):
        # Check that the distance given is either None of a float
        if distance is not None and type(distance) != int and type(distance) != float:
            raise RuntimeError, "LoadRun.set_sample_detector_distance expects a float: %s" % str(distance)
        self._sample_det_dist = distance
        
    def set_sample_detector_offset(self, offset):
        # Check that the offset given is either None of a float
        if offset is not None and type(offset) != int and type(offset) != float:
            raise RuntimeError, "LoadRun.set_sample_detector_offset expects a float: %s" % str(offset)
        self._sample_det_offset = offset
        
    def set_beam_center(self, beam_center):
        """
            Sets the beam center to be used when loading the file
            @param beam_center: [pixel_x, pixel_y]
        """
        if beam_center is None:
            self._beam_center = None
        
        # Check that we have pixel numbers (int)            
        elif type(beam_center) == list:
            if len(beam_center) == 2:
                try:
                    int(beam_center[0])
                    int(beam_center[1])
                    self._beam_center = [beam_center[0], beam_center[1]]
                except:
                    raise RuntimeError, "LoadRun.set_beam_center expects a list of two integers\n  %s" % sys.exc_value
            else:
                raise RuntimeError, "LoadRun.set_beam_center expects a list of two integers. Found length %d" % len(beam_center)
            
        else:
            raise RuntimeError, "LoadRun.set_beam_center expects a list of two integers. Found %s" % type(beam_center)
        
    def execute(self, reducer, inputworkspace, outputworkspace=None):
        """
            Loads a data file.
            Note: Files are ALWAYS reloaded when this method is called.
            We do this because speed is not an issue and we ensure that the data
            is always pristine. We could only load files that are not already loaded
            by using the 'dirty' flag and checking for the existence of the workspace.
        """
        if outputworkspace is not None:
            workspace = outputworkspace 
        else:
            workspace = inputworkspace
        # If we don't have a data file, look up the workspace handle
        if self._data_file is None:
            if workspace in reducer._data_files:
                data_file = reducer._data_files[workspace]
            elif workspace in reducer._extra_files:
                data_file = reducer._extra_files[workspace]
            else:
                raise RuntimeError, "SANSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
        else:
            data_file = self._data_file
        
        def _load_data_file(file_name, wks_name):
            filepath = find_data(file_name, instrument=reducer.instrument.name())
            data_dir,_ = os.path.split(filepath)
            if self._wavelength is None:
                LoadSpice2D(filepath, wks_name)
            else:
                LoadSpice2D(filepath, wks_name, 
                            Wavelength=self._wavelength,
                            WavelengthSpread=self._wavelength_spread)
            return data_dir

        # Check whether we have a list of files that need merging
        data_dir = ''
        if type(data_file)==list:
            for i in range(len(data_file)):
                if i==0:
                    data_dir = _load_data_file(data_file[i], workspace)
                else:
                    data_dir = _load_data_file(data_file[i], '__tmp_wksp')
                    Plus(LHSWorkspace=workspace,
                         RHSWorkspace='__tmp_wksp',
                         OutputWorkspace=workspace)
            if mtd.workspaceExists('__tmp_wksp'):
                mtd.deleteWorkspace('__tmp_wksp')
        else:
            data_dir = _load_data_file(data_file, workspace)
        
        # Get the original sample-detector distance from the data file
        sdd = mtd[workspace].getRun().getProperty("sample-detector-distance").value
        detector_ID=mtd[workspace].getInstrument().getStringParameter("detector-name")[0]
        
        if self._sample_det_dist is not None:
            MoveInstrumentComponent(workspace, detector_ID,
                                    Z = (self._sample_det_dist-sdd)/1000.0, 
                                    RelativePosition="1")
            sdd = self._sample_det_dist            
        elif not self._sample_det_offset == 0:
            MoveInstrumentComponent(workspace, detector_ID,
                                    Z = self._sample_det_offset/1000.0, 
                                    RelativePosition="1")
            sdd += self._sample_det_offset

        # Store the sample-detector distance.
        mantid[workspace].getRun().addProperty_dbl("sample_detector_distance", sdd, 'mm', True)
        
        # Compute beam diameter at the detector
        src_to_sample = mtd[workspace].getRun().getProperty("source-sample-distance").value
        sample_apert = mtd[workspace].getRun().getProperty("sample-aperture-diameter").value
        source_apert = mtd[workspace].getRun().getProperty("source-aperture-diameter").value
        beam_diameter = sdd/src_to_sample*(source_apert+sample_apert)+sample_apert
        mantid[workspace].getRun().addProperty_dbl("beam-diameter", beam_diameter, "mm", True)
        
            
        # Move detector array to correct position
        # Note: the position of the detector in Z is now part of the load
        if self._beam_center is not None:            
            [pixel_ctr_x, pixel_ctr_y] = self._beam_center
        else:
            [pixel_ctr_x, pixel_ctr_y] = reducer.get_beam_center()
        if pixel_ctr_x is not None and pixel_ctr_y is not None:
            [beam_ctr_x, beam_ctr_y] = reducer.instrument.get_coordinate_from_pixel(pixel_ctr_x, pixel_ctr_y, workspace)
            [default_pixel_x, default_pixel_y] = reducer.instrument.get_default_beam_center(workspace)
            [default_x, default_y] = reducer.instrument.get_coordinate_from_pixel(default_pixel_x, default_pixel_y, workspace)
            MoveInstrumentComponent(workspace, detector_ID, 
                                    X = default_x-beam_ctr_x,
                                    Y = default_y-beam_ctr_y,
                                    RelativePosition="1")
            mantid[workspace].getRun().addProperty_dbl("beam_center_x", pixel_ctr_x, 'pixel', True)            
            mantid[workspace].getRun().addProperty_dbl("beam_center_y", pixel_ctr_y, 'pixel', True)            
        else:
            # Don't move the detector and use the default beam center
            [default_pixel_x, default_pixel_y] = reducer.instrument.get_default_beam_center(workspace)
            mantid[workspace].getRun().addProperty_dbl("beam_center_x", default_pixel_x, 'pixel', True)            
            mantid[workspace].getRun().addProperty_dbl("beam_center_y", default_pixel_y, 'pixel', True)            
            if type(reducer._beam_finder) is BaseBeamFinder:
                reducer.set_beam_finder(BaseBeamFinder(default_pixel_x, default_pixel_y))
                mantid.sendLogMessage("No beam finding method: setting to default [%-6.1f, %-6.1f]" % (default_pixel_x, 
                                                                                                       default_pixel_y))
                
            mantid.sendLogMessage("Beam center isn't defined: using default [%-6.1f, %-6.1f] for %s" % (default_pixel_x, 
                                                                                                        default_pixel_y,
                                                                                                        workspace))
        
        n_files = 1
        if type(data_file)==list:
            n_files = len(data_file)
        return "Data set loaded: %s [%g file(s)]\n   Data directory: %s" % (workspace, n_files, str(data_dir))
    
class Normalize(ReductionStep):
    """
        Normalize the data to timer or a spectrum, typically a monitor, 
        with in the workspace. By default the normalization is done with 
        respect to the Instrument's incident monitor
    """
    def __init__(self, normalization_spectrum=None):
        super(Normalize, self).__init__()
        self._normalization_spectrum = normalization_spectrum
        
    def get_normalization_spectrum(self):
        return self._normalization_spectrum
        
    def execute(self, reducer, workspace):
        if self._normalization_spectrum is None:
            self._normalization_spectrum = reducer.instrument.get_incident_mon()
        
        # Get counting time or monitor
        norm_ws = workspace+"_normalization"

        CropWorkspace(workspace, norm_ws,
                      StartWorkspaceIndex = self._normalization_spectrum, 
                      EndWorkspaceIndex   = self._normalization_spectrum)      

        Divide(workspace, norm_ws, workspace)
        
        # HFIR-specific: If we count for monitor we need to multiply by 1e8
        if self._normalization_spectrum == reducer.NORMALIZATION_MONITOR:         
            Scale(workspace, workspace, 1.0e8, 'Multiply')

        return "Normalization done [spectrum %g]" % self._normalization_spectrum
                        
    def clean(self):
        mtd.deleteWorkspace(norm_ws)
            
class WeightedAzimuthalAverage(ReductionStep):
    """
        ReductionStep class that performs azimuthal averaging
        and transforms the 2D reduced data set into I(Q).
    """
    def __init__(self, binning=None, suffix="_Iq", error_weighting=False, n_bins=100, n_subpix=1, log_binning=False):
        super(WeightedAzimuthalAverage, self).__init__()
        self._binning = binning
        self._suffix = suffix
        self._error_weighting = error_weighting
        self._nbins = n_bins
        self._nsubpix = n_subpix
        self._log_binning = log_binning
        
    def _get_binning(self, reducer, workspace, wavelength_min, wavelength_max):
        
        sample_detector_distance = mtd[workspace].getRun().getProperty("sample_detector_distance").value
        nx_pixels = int(mtd[workspace].getInstrument().getNumberParameter("number-of-x-pixels")[0])
        ny_pixels = int(mtd[workspace].getInstrument().getNumberParameter("number-of-y-pixels")[0])
        pixel_size_x = mtd[workspace].getInstrument().getNumberParameter("x-pixel-size")[0]
        pixel_size_y = mtd[workspace].getInstrument().getNumberParameter("y-pixel-size")[0]

        # Q min is one pixel from the center, unless we have the beam trap size
        if mtd[workspace].getRun().hasProperty("beam-trap-radius"):
            mindist = mtd[workspace].getRun().getProperty("beam-trap-radius").value
        else:
            mindist = min(pixel_size_x, pixel_size_y)
        qmin = 4*math.pi/wavelength_max*math.sin(0.5*math.atan(mindist/sample_detector_distance))
        
        beam_ctr = reducer._beam_finder.get_beam_center()
        dxmax = pixel_size_x*max(beam_ctr[0],nx_pixels-beam_ctr[0])
        dymax = pixel_size_y*max(beam_ctr[1],ny_pixels-beam_ctr[1])
        maxdist = math.sqrt(dxmax*dxmax+dymax*dymax)
        qmax = 4*math.pi/wavelength_min*math.sin(0.5*math.atan(maxdist/sample_detector_distance))
        
        if not self._log_binning:
            qstep = (qmax-qmin)/self._nbins
            f_step = (qmax-qmin)/qstep
            n_step = math.floor(f_step)
            if f_step-n_step>10e-10:
                qmax = qmin+qstep*n_step
            return qmin, qstep, qmax
        else:
            # Note: the log binning in Mantid is x_i+1 = x_i * ( 1 + dx )
            qstep = (math.log10(qmax)-math.log10(qmin))/self._nbins
            f_step = (math.log10(qmax)-math.log10(qmin))/qstep
            n_step = math.floor(f_step)
            if f_step-n_step>10e-10:
                qmax = math.pow(10.0, math.log10(qmin)+qstep*n_step)
            return qmin, -(math.pow(10.0,qstep)-1.0), qmax
        
    def execute(self, reducer, workspace):
        # Q range                        
        beam_ctr = reducer._beam_finder.get_beam_center()
        pixel_size_x = mtd[workspace].getInstrument().getNumberParameter("x-pixel-size")[0]
        pixel_size_y = mtd[workspace].getInstrument().getNumberParameter("y-pixel-size")[0]
        if beam_ctr[0] is None or beam_ctr[1] is None:
            raise RuntimeError, "Azimuthal averaging could not proceed: beam center not set"
        if self._binning is None:
            # Wavelength. Read in the wavelength bins. Skip the first one which is not set up properly for EQ-SANS
            x = mtd[workspace].dataX(1)
            x_length = len(x)
            if x_length < 2:
                raise RuntimeError, "Azimuthal averaging expects at least one wavelength bin"
            wavelength_max = (x[x_length-2]+x[x_length-1])/2.0
            wavelength_min = (x[0]+x[1])/2.0
            if wavelength_min==0 or wavelength_max==0:
                raise RuntimeError, "Azimuthal averaging needs positive wavelengths"
            qmin, qstep, qmax = self._get_binning(reducer, workspace, wavelength_min, wavelength_max)
            self._binning = "%g, %g, %g" % (qmin, qstep, qmax)
        else:
            toks = self._binning.split(',')
            if len(toks)<3:
                raise RuntimeError, "Invalid binning provided: %s" % str(self._binning)
            qmin = float(toks[0])
            qmax = float(toks[2])
            
        output_ws = workspace+str(self._suffix)
        if mtd[workspace].getRun().hasProperty("is_separate_corrections"):
            pixel_adj = mtd[workspace].getRun().getProperty("pixel_adj").value
            wl_adj = mtd[workspace].getRun().getProperty("wl_adj").value
            Q1D(DetBankWorkspace=workspace, OutputWorkspace=output_ws,
                OutputBinning=self._binning, 
                PixelAdj=pixel_adj, WavelengthAdj=wl_adj)
        else:
            #Q1D(InputWorkspace=workspace, InputForErrors=workspace, OutputWorkspace=output_ws, OutputBinning=self._binning)
            Q1DWeighted(workspace, output_ws, self._binning,
                    NPixelDivision=self._nsubpix,
                    PixelSizeX=pixel_size_x,
                    PixelSizeY=pixel_size_y, ErrorWeighting=self._error_weighting)  
        ReplaceSpecialValues(output_ws, output_ws, NaNValue=0.0, NaNError=0.0, InfinityValue=0.0, InfinityError=0.0)
                
        # Q resolution
        if reducer._resolution_calculator is not None:
            reducer._resolution_calculator(output_ws, output_ws)
            
        # Add output workspace to the list of important output workspaces
        reducer.output_workspaces.append(output_ws)
        return "Performed radial averaging between Q=%g and Q=%g" % (qmin, qmax)
        
    def get_output_workspace(self, workspace):
        return workspace+str(self._suffix)
    
    def get_data(self, workspace):
        class DataSet(object):
            x=[]
            y=[]
            dy=[]
        
        d = DataSet()
        d.x = mtd[self.get_output_workspace(workspace)].dataX(0)[1:]
        d.y = mtd[self.get_output_workspace(workspace)].dataY(0)
        d.dx = mtd[self.get_output_workspace(workspace)].dataE(0)
        return d
            
class SensitivityCorrection(ReductionStep):
    """
        Compute the sensitivity correction and apply it to the input workspace.
        
        The ReductionStep object stores the sensitivity, so that the object
        be re-used on multiple data sets and the sensitivity will not be
        recalculated.
    """
    def __init__(self, flood_data, min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None, 
                 beam_center=None, use_sample_dc=False):
        super(SensitivityCorrection, self).__init__()
        self._flood_data = flood_data
        self._dark_current_data = dark_current
        self._efficiency_ws = None
        self._min_sensitivity = min_sensitivity
        self._max_sensitivity = max_sensitivity
        self._use_sample_dc = use_sample_dc
        
        # Beam center for flood data
        self._beam_center = beam_center
        
    @validate_step
    def set_beam_center(self, beam_center):
        """
             Set the reduction step that will find the beam center position
             @param beam_center: ReductionStep object
        """
        self._beam_center = beam_center
        
    def get_beam_center(self):
        """
            Returns the beam center found by the beam center finder
        """
        if self._beam_center is not None:
            return self._beam_center.get_beam_center()
        else:
            return None
    
    def _compute_efficiency(self, reducer, workspace):
        output_str = ""
        if self._efficiency_ws is None:
            # Load the flood data
            filepath = find_data(self._flood_data, instrument=reducer.instrument.name())
            flood_ws = "flood_"+extract_workspace_name(filepath)
            
            beam_center = None
            # Find the beam center if we need to
            if self._beam_center is not None:
                self._beam_center.execute(reducer)
                beam_center = self._beam_center.get_beam_center()
                
            loader = reducer._data_loader.clone(data_file=filepath)
            loader.set_beam_center(beam_center)
            loader.execute(reducer, flood_ws)

            # Subtract dark current
            if self._use_sample_dc:
                if reducer._dark_current_subtracter is not None:
                    partial_output = reducer._dark_current_subtracter.execute(reducer, flood_ws)
                    output_str = "%s\n   %s" % (output_str, partial_output) 
                    
            elif self._dark_current_data is not None and len(str(self._dark_current_data).strip())>0 \
                and reducer._dark_current_subtracter_class is not None:
                partial_output = reducer._dark_current_subtracter_class(self._dark_current_data).execute(reducer, flood_ws)
                output_str = "%s\n   %s" % (output_str, partial_output) 
            
            # Correct flood data for solid angle effects (Note: SA_Corr_2DSAS)
            # Note: Moving according to the beam center is done in LoadRun above
            if reducer._solid_angle_correcter is not None:
                partial_output = reducer._solid_angle_correcter.execute(reducer, flood_ws)
                output_str = "%s\n   %s" % (output_str, partial_output) 
        
            # Create efficiency profile: 
            # Divide each pixel by average signal, and mask high and low pixels.
            self._efficiency_ws = "__efficiency"
            CalculateEfficiency(flood_ws, self._efficiency_ws, self._min_sensitivity, self._max_sensitivity)
            
            # Clean up
            reducer._data_loader.__class__.delete_workspaces(flood_ws)
            
        return output_str
        
    def execute(self, reducer, workspace):
        # If the sensitivity correction workspace exists, just apply it.
        # Otherwise create it.      
        #TODO: check that the workspaces have the same binning!
        output_str = "   Using data set: %s" % extract_workspace_name(self._flood_data)
        if self._efficiency_ws is None:
            output_str += self._compute_efficiency(reducer, workspace)
            
        # Divide by detector efficiency
        Divide(workspace, self._efficiency_ws, workspace)
        #ReplaceSpecialValues(workspace, workspace, InfinityValue=0.0, InfinityError=0.0)
        
        # Copy over the efficiency's masked pixels to the reduced workspace
        masked_detectors = GetMaskedDetectors(self._efficiency_ws)
        MaskDetectors(workspace, None, masked_detectors.getPropertyValue("DetectorList"))        
    
        return "Sensitivity correction applied\n%s\n" % output_str

class Mask(ReductionStep):
    """
        Marks some spectra so that they are not included in the analysis
    """
    def __init__(self):
        """
            Initalize masking 
        """
        super(Mask, self).__init__()
        self._nx_low = 0
        self._nx_high = 0
        self._ny_low = 0
        self._ny_high = 0
        
        self._xml = []

        #these spectra will be masked by the algorithm MaskDetectors
        self.detect_list = []
        
        # List of pixels to mask
        self.masked_pixels = []
        
        # Only apply mask defined in the class and ignore additional
        # information from the run properties
        self._ignore_run_properties = False
        
    def mask_edges(self, nx_low=0, nx_high=0, ny_low=0, ny_high=0):
        """
            Define a "picture frame" outside of which the spectra from all detectors are to be masked.
            @param nx_low: number of pixels to mask on the lower-x side of the detector
            @param nx_high: number of pixels to mask on the higher-x side of the detector
            @param ny_low: number of pixels to mask on the lower-y side of the detector
            @param ny_high: number of pixels to mask on the higher-y side of the detector        
        """
        self._nx_low = nx_low
        self._nx_high = nx_high
        self._ny_low = ny_low
        self._ny_high = ny_high        
        
    def add_xml_shape(self, complete_xml_element):
        """
            Add an arbitrary shape to region to be masked
            @param complete_xml_element: description of the shape to add
        """
        if not complete_xml_element.startswith('<') :
            raise ValueError('Excepted xml string but found: ' + str(complete_xml_element))
        self._xml.append(complete_xml_element)

    def _infinite_plane(self, id, plane_pt, normal_pt, complement = False):
        """
            Generates xml code for an infinte plane
            @param id: a string to refer to the shape by
            @param plane_pt: a point in the plane
            @param normal_pt: the direction of a normal to the plane
            @param complement: mask in the direction of the normal or away
            @return the xml string
        """
        if complement:
            addition = '#'
        else:
            addition = ''
        return '<infinite-plane id="' + str(id) + '">' + \
            '<point-in-plane x="' + str(plane_pt[0]) + '" y="' + str(plane_pt[1]) + '" z="' + str(plane_pt[2]) + '" />' + \
            '<normal-to-plane x="' + str(normal_pt[0]) + '" y="' + str(normal_pt[1]) + '" z="' + str(normal_pt[2]) + '" />'+ \
            '</infinite-plane>\n'

    def _infinite_cylinder(self, centre, radius, axis, id='shape'):
        """
            Generates xml code for an infintely long cylinder
            @param centre: a tupple for a point on the axis
            @param radius: cylinder radius
            @param axis: cylinder orientation
            @param id: a string to refer to the shape by
            @return the xml string
        """
        return '<infinite-cylinder id="' + str(id) + '">' + \
            '<centre x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + '" />' + \
            '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
            '<radius val="' + str(radius) + '" /></infinite-cylinder>\n'

    def _finite_cylinder(self, centre, radius, height, axis, id='shape'):
        """
            Generates xml code for an infintely long cylinder
            @param centre: a tupple for a point on the axis
            @param radius: cylinder radius
            @param height: cylinder height            
            @param axis: cylinder orientation
            @param id: a string to refer to the shape by
            @return the xml string
        """
        return '<cylinder id="' + str(id) + '">' + \
            '<centre-of-bottom-base x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + '" />' + \
            '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
            '<radius val="' + str(radius) + '" /><height val="' + str(height) + '" /></cylinder>\n'

    def add_cylinder(self, radius, xcentre, ycentre, ID='shape'):
        '''Mask the inside of an infinite cylinder on the input workspace.'''
        self.add_xml_shape(
            self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0,0,1], id=ID)+'<algebra val="' + str(ID) + '"/>')
            

    def add_outside_cylinder(self, radius, xcentre = 0.0, ycentre = 0.0, ID='shape'):
        '''Mask out the outside of a cylinder or specified radius'''
        self.add_xml_shape(
            self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0,0,1], id=ID)+'<algebra val="#' + str(ID) + '"/>')

    def add_pixel_rectangle(self, x_min, x_max, y_min, y_max):
        """
            Mask out a rectangle area defined in pixel coordinates.
            @param x_min: Minimum x to mask
            @param x_max: Maximum x to mask
            @param y_min: Minimum y to mask
            @param y_max: Maximum y to mask
        """
        for ix in range(x_min, x_max+1):
            for iy in range(y_min, y_max+1):
                self.masked_pixels.append([ix, iy])

    def add_detector_list(self, det_list):
        """
            Mask the given detectors
            @param det_list: list of detector IDs
        """
        self.detect_list.extend(det_list) 

    def ignore_run_properties(self, ignore=True):
        """
            Only use the mask information set in the current object 
            and ignore additional information that may have been
            stored in the workspace properties.
        """
        self._ignore_run_properties = ignore
        
    def execute(self, reducer, workspace):
        
        # Check whether the workspace has mask information
        if not self._ignore_run_properties and mtd[workspace].getRun().hasProperty("rectangular_masks"):
            mask_str = mtd[workspace].getRun().getProperty("rectangular_masks").value
            try:
                rectangular_masks = pickle.loads(mask_str)
            except:
                rectangular_masks = []
                toks = mask_str.split(',')
                for item in toks:
                    if len(item)>0:
                        c = item.split(' ')
                        if len(c)==4:
                            print c
                            rectangular_masks.append([int(c[0]), int(c[1]), int(c[2]), int(c[3])])
                
            for rec in rectangular_masks:
                try:
                    self.add_pixel_rectangle(rec[0], rec[1], rec[2], rec[3])
                except:
                    mantid.sendLogMessage("Badly defined mask from configuration file: %s" % str(rec))
        
        for shape in self._xml:
            MaskDetectorsInShape(workspace, shape)
        
        instrument = reducer.instrument
        # Get a list of detector pixels to mask
        if self._nx_low != 0 or self._nx_high != 0 or self._ny_low != 0 or self._ny_high != 0:            
            self.masked_pixels.extend(instrument.get_masked_pixels(self._nx_low,
                                                                   self._nx_high,
                                                                   self._ny_low,
                                                                   self._ny_high,
                                                                   workspace))

        if len(self.detect_list)>0:
            MaskDetectors(workspace, DetectorList = self.detect_list)
            
        # Mask out internal list of pixels
        if len(self.masked_pixels)>0:
            # Transform the list of pixels into a list of Mantid detector IDs
            masked_detectors = instrument.get_detector_from_pixel(self.masked_pixels)
            # Mask the pixels by passing the list of IDs
            MaskDetectors(workspace, DetectorList = masked_detectors)
            
        masked_detectors = GetMaskedDetectors(workspace)
        mantid.sendLogMessage("Mask check %s: %g masked pixels" % (workspace, len(masked_detectors.getPropertyValue("DetectorList"))))  
            
        return "Mask applied %s: %g masked pixels" % (workspace, len(masked_detectors.getPropertyValue("DetectorList")))

class CorrectToFileStep(ReductionStep):
    """
        Runs the algorithm CorrectToFile()
    """
    def __init__(self, file='', corr_type='', operation=''):
        """
            Parameters passed to this function are passed to
            the CorrectToFile() algorithm
            @param file: full path of the correction file
            @param corr_type: "deltaE", "TOF", "SpectrumNumber" or any valid setting for CorrectToFile()'s as FirstColumnValue property
            @param operation: set to "Divide" or "Multiply"
        """
        super(CorrectToFileStep, self).__init__()
        self._filename = file
        self._corr_type = corr_type
        self._operation = operation

    def get_filename(self):
        return self._filename

    def set_filename(self, filename):
        self._filename = filename

    def execute(self, reducer, workspace):
        if self._filename:
            CorrectToFile(workspace, self._filename, workspace,
                          self._corr_type, self._operation)

class CalculateNorm(object):
    """
        Generates the normalization workspaces required by Q1D from output
        of other, sometimes optional, reduction_steps or specified workspaces.
        Workspaces for wavelength adjustment must have their
        distribution/non-distribution flag set correctly as they maybe converted
    """
    TMP_WORKSPACE_NAME = '__CalculateNorm_loaded_temp'
    WAVE_CORR_NAME = '__Q_WAVE_conversion_temp'
    PIXEL_CORR_NAME = '__Q_pixel_conversion_temp'
    
    def  __init__(self, wavelength_deps=[]):
        super(CalculateNorm, self).__init__()
        self._wave_steps = wavelength_deps
        self._wave_adjs = []
        self._pixel_file = ''

        #algorithm to be used to load correction files
        self._load='Load'
        #a parameters string to add as the last argument to the above algorithm
        self._load_params=''

    def setPixelCorrFile(self, filename):
        """
            Adds a scaling that is a function of the detector (spectrum index)
            from a file
        """
        self._pixel_file = filename

    def getPixelCorrFile(self):
        """
            @return the file that has been set to load as the pixelAdj workspace or '' if none has been set
        """
        return self._pixel_file
    
    def _is_point_data(self, wksp):
        """
            Tests if the workspace whose name is passed contains point or histogram data
            The test is if the X and Y array lengths are the same = True, different = false
            @param wksp: name of the workspace to test
            @return True for point data, false for histogram
        """
        handle = mtd[wksp]
        if len(handle.readX(0)) == len(handle.readY(0)):
            return True
        else:
            return False 

    def calculate(self, reducer, wave_wksps=[]):
        """
            Multiplies all the wavelength scalings into one workspace and all the detector
            dependent scalings into another workspace that can be used by ConvertToQ. It is important
            that the wavelength correction workspaces have a know distribution/non-distribution state
            @param reducer: settings used for this reduction
            @param wave_wksps: additional wavelength dependent correction workspaces to include   
        """
        for step in self._wave_steps:
            if step.output_wksp:
                wave_wksps.append(step.output_wksp)

        wave_adj = None
        for wksp in wave_wksps:
            #before the workspaces can be combined they all need to match 
            RebinToWorkspace(wksp, reducer.output_wksp, self.TMP_WORKSPACE_NAME)

            if not wave_adj:
                #first time around this loop
                wave_adj = self.WAVE_CORR_NAME
                RenameWorkspace(self.TMP_WORKSPACE_NAME, wave_adj)
            else:
                #multiplying two raw counts workspaces gives a dependence on the bin width^2 which Mantid isn't set up to handle (dependence on the bin width = is distribution = is handled)
                if not mtd[wave_adj].isDistribution():
                    if not mtd[self.TMP_WORKSPACE_NAME].isDistribution():
                        ConvertToDistribution(self.TMP_WORKSPACE_NAME)
                Multiply(self.TMP_WORKSPACE_NAME, wave_adj, wave_adj)

        pixel_adj = ''
        if self._pixel_file:
            pixel_adj = self.PIXEL_CORR_NAME
            load_com = self._load+'("'+self._pixel_file+'","'+pixel_adj+'"'
            if self._load_params:
                load_com  += ','+self._load_params
            load_com += ')'
            eval(load_com)
        
        if mtd.workspaceExists(self.TMP_WORKSPACE_NAME):
            DeleteWorkspace(self.TMP_WORKSPACE_NAME)
        
        return wave_adj, pixel_adj

class ConvertToQ(ReductionStep):
    """
        Runs the Q1D or Qxy algorithms to convert wavelength data into momentum transfer, Q
    """
    # the list of possible Q conversion algorithms to use
    _OUTPUT_TYPES = {'1D' : 'Q1D',
                     '2D' : 'Qxy'}
    # defines if Q1D should correct for gravity by default
    _DEFAULT_GRAV = False    
    def __init__(self, normalizations):
        """
            @param normalizations: the CalcNorm object contains the workspace, ReductionSteps or files require for the optional normalization arguments to Q1D
        """
        super(ConvertToQ, self).__init__()
        
        if not issubclass(normalizations.__class__, CalculateNorm):
            raise RuntimeError('Error initializing ConvertToQ, invalid normalization object')
        #contains the normalization optional workspaces to pass to the Q algorithm 
        self._norms = normalizations
        
        #this should be set to 1D or 2D
        self._output_type = '1D'
        #the algorithm that corresponds to the above choice
        self._Q_alg = self._OUTPUT_TYPES[self._output_type]
        #if true gravity is taken into account in the Q1D calculation
        self._use_gravity = self._DEFAULT_GRAV
        #used to implement a default setting for gravity that can be over written but doesn't over write
        self._grav_set = False
        #this should contain the rebin parameters
        self.binning = None
        #if set to true the normalization is done out side of the convert to Q algorithm
        self.prenorm = False
        #The minimum distance in metres from the beam center at which all wavelengths are used in the calculation in mm
        self.r_cut = 0.0
        #The shortest wavelength in angstrom at which counts should be summed from all detector pixels in Angstrom
        self.w_cut = 0.0
    
    def set_output_type(self, descript):
        """
            Requests the given output from the Q conversion, either 1D or 2D. For
            the 1D calculation it asks the reducer to keep a workspace for error
            estimates
            @param descript: 1D or 2D
        """
        self._Q_alg = self._OUTPUT_TYPES[descript]
        self._output_type = descript
                    
    def get_output_type(self):
        return self._output_type

    output_type = property(get_output_type, set_output_type, None, None)

    def get_gravity(self):
        return self._use_gravity

    def set_gravity(self, flag, override=True):
        """
            Enable or disable including gravity when calculating Q
            @param flag: set to True to enable the gravity correction
            @param override: over write the setting from a previous call to this method (default is True)
        """
        if override:
            self._grav_set = True

        if (not self._grav_set) or override:
                self._use_gravity = bool(flag)
        else:
            msg = "User file can't override previous gravity setting, do gravity correction remains " + str(self._use_gravity) 
            print msg
            mantid.sendLogMessage('::SANS::Warning: ' + msg)

    def execute(self, reducer, workspace):
        """
            Calculate the normalization workspaces and then call the chosen Q conversion algorithm
        """
        #create normalization workspaces
        if self._norms:
            # the empty list at the end appears to be needed (the system test SANS2DWaveloops) is this a bug in Python?
            wave_adj, pixel_adj = self._norms.calculate(reducer, [])
        else:
            raise RuntimeError('Normalization workspaces must be created by CalculateNorm() and passed to this step')

        #the last term maintains compatibility with the original Qxy, remove when it is fixed
        if self.prenorm or (self._Q_alg == 'Qxy'):
            data = mtd[workspace]
            if wave_adj:
                data /= mtd[wave_adj]
            if pixel_adj:
                data /= mtd[pixel_adj]
            self._deleteWorkspaces([wave_adj, pixel_adj])
            wave_adj, pixel_adj = '', ''

        try:
            if self._Q_alg == 'Q1D':
                Q1D(workspace, workspace, OutputBinning=self.binning, WavelengthAdj=wave_adj, PixelAdj=pixel_adj, AccountForGravity=self._use_gravity, RadiusCut=self.r_cut/1000., WaveCut=self.w_cut)
    
            elif self._Q_alg == 'Qxy':
                Qxy(workspace, workspace, reducer.QXY2, reducer.DQXY, AccountForGravity=self._use_gravity)
                ReplaceSpecialValues(workspace, workspace, NaNValue="0", InfinityValue="0")
            else:
                raise NotImplementedError('The type of Q reduction has not been set, e.g. 1D or 2D')
        except:
            #when we are all up to Python 2.5 replace the duplicated code below with one finally:        
            self._deleteWorkspaces([wave_adj, pixel_adj])
            raise

        self._deleteWorkspaces([wave_adj, pixel_adj])

    def _deleteWorkspaces(self, workspaces):
        """
            Deletes a list of workspaces if they exist but ignores any errors
            @param workspaces: list of workspaces to try to delete
        """
        for wk in workspaces:
            try:
                if wk and mantid.workspaceExists(wk):
                    DeleteWorkspace(wk)
            except:
                #if the workspace can't be deleted this function does nothing
                pass
            
class SaveIqAscii(ReductionStep):
    """
        Save the reduced data to ASCII files
    """
    def __init__(self):
        super(SaveIqAscii, self).__init__()
        
    def execute(self, reducer, workspace):
        # Determine which directory to use
        output_dir = reducer._data_path
        if reducer._output_path is not None:
            if os.path.isdir(reducer._output_path):
                output_dir = reducer._output_path
            else:
                raise RuntimeError, "SaveIqAscii could not save in the following directory: %s" % reducer._output_path
            
        log_text = ""
        if reducer._azimuthal_averager is not None:
            output_list = reducer._azimuthal_averager.get_output_workspace(workspace)
            if type(output_list) is not list:
                output_list = [output_list]
            for output_ws in output_list:
                if mtd.workspaceExists(output_ws):
                    filename = os.path.join(output_dir, output_ws+'.txt')
                    SaveAscii(Filename=filename, InputWorkspace=output_ws, Separator="\t", CommentIndicator="# ", WriteXError=True)
                    filename = os.path.join(output_dir, output_ws+'.xml')
                    SaveCanSAS1D(Filename=filename, InputWorkspace=output_ws)
                    
                    log_text = "I(Q) saved in %s" % (filename)
        
        if reducer._two_dim_calculator is not None:
            if hasattr(reducer._two_dim_calculator, "get_output_workspace"):
                output_ws = reducer._two_dim_calculator.get_output_workspace(workspace)
                if mtd.workspaceExists(output_ws):
                    filename = os.path.join(output_dir, output_ws+'.dat')
                    SaveNISTDAT(InputWorkspace=output_ws, Filename=filename)
                    
                    if len(log_text)>0:
                        log_text += '\n'
                    log_text += "I(Qx,Qy) saved in %s" % (filename)
            
        return log_text
            
class SubtractBackground(ReductionStep):
    """
        Subtracts background from a sample data workspace.
        The processed background workspace is stored for later use.
    """
    def __init__(self, background_file, transmission=None, geometry=None):
        """
            @param background_file: file name of background data set
            @param transmission: ReductionStep object to compute the background transmission
        """
        super(SubtractBackground, self).__init__()
        self._background_file = background_file
        self._background_ws = None
        self._transmission = transmission
        self._geometry_correcter = geometry
    
    @validate_step
    def set_geometry_correcter(self, correcter):    
        """
            Set the ReductionStep object that takes care of the geometry correction
            @param subtracter: ReductionStep object
        """
        self._geometry_correcter = correcter
        
    def set_transmission(self, trans):
        """
             Set the reduction step that will apply the transmission correction
             @param trans: ReductionStep object
        """
        if issubclass(trans.__class__, BaseTransmission) or trans is None:
            self._transmission = trans
        else:
            raise RuntimeError, "SubtractBackground.set_transmission expects an object of class ReductionStep"
        
    def get_transmission_calculator(self):
        return self._transmission
    
    def get_transmission(self):
        if self._transmission is not None:
            return self._transmission.get_transmission()
        else:
            return None
        
    def set_trans_theta_dependence(self, theta_dependence):
        if self._transmission is not None:
            self._transmission.set_theta_dependence(theta_dependence)
        else:
            raise RuntimeError, "A transmission algorithm must be selected before setting the theta-dependence of the correction."
            
    def set_trans_dark_current(self, dark_current):
        if self._transmission is not None:
            self._transmission.set_dark_current(dark_current)
        else:
            raise RuntimeError, "A transmission algorithm must be selected before setting its dark current correction."
                    
    def execute(self, reducer, workspace):
        log_text = ''
        if self._background_ws is None:
            # Apply the reduction steps that we normally apply to the data
            self._background_ws = "bck_"+extract_workspace_name(self._background_file)
            reducer._extra_files[self._background_ws] = self._background_file
            for item in reducer._2D_steps():
                output = item.execute(reducer, self._background_ws)
                if output is not None:
                    output = output.replace('\n', '\n   |')
                    log_text = "%s\n   %s" % (log_text, output)
        
            # Geometry correction
            if self._geometry_correcter is not None:
                output = self._geometry_correcter.execute(reducer, self._background_ws)
                if output is not None:
                    output = output.replace('\n', '\n   |')
                    log_text = "%s\n   %s" % (log_text, output)                    
                
            # The transmission correction is set separately
            if self._transmission is not None:
                output = self._transmission.execute(reducer, self._background_ws)
                if output is not None:
                    output = output.replace('\n', '\n   |')
                    log_text = "%s\n   %s" % (log_text, output)
        
        # Make sure that we have the same binning
        RebinToWorkspace(self._background_ws, workspace, OutputWorkspace="__tmp_bck")
        if mtd[workspace].getRun().hasProperty("is_separate_corrections"):
            if reducer._absolute_scale is not None:
                reducer._absolute_scale.execute(reducer, "__tmp_bck")
            if reducer._azimuthal_averager is not None:
                reducer._azimuthal_averager.execute(reducer, "__tmp_bck")
        else:
            Minus(workspace, "__tmp_bck", workspace)
        if mtd.workspaceExists("__tmp_bck"):
            mtd.deleteWorkspace("__tmp_bck")        
        
        log_text = log_text.replace('\n','\n   |')
        return "Background subtracted [%s]%s\n" % (self._background_ws, log_text)
        
 
class GetSampleGeom(ReductionStep):
    """
        Loads, stores, retrieves, etc. data about the geometry of the sample
        On initialisation this class will return default geometry values (compatible with the Colette software)
        There are functions to override these settings
        On execute if there is geometry information in the workspace this will override any unset attributes
    """
    # IDs for each shape as used by the Colette software
    _shape_ids = {1 : 'cylinder-axis-up',
                  2 : 'cuboid',
                  3 : 'cylinder-axis-along'}
    _default_shape = 'cylinder-axis-along'
    
    def __init__(self):
        super(GetSampleGeom, self).__init__()

        # string specifies the sample's shape
        self._shape = None
        # sample's width
        self._width = None
        self._thickness = None
        self._height = None
        
        self._use_wksp_shape = True
        self._use_wksp_width = True
        self._use_wksp_thickness = True
        self._use_wksp_height = True

    def _get_default(self, attrib):
        if attrib == 'shape':
            return self._default_shape
        elif attrib == 'width' or attrib == 'thickness' or attrib == 'height':
            return 1.0

    def set_shape(self, new_shape):
        """
            Sets the sample's shape from a string or an ID. If the ID is not
            in the list of allowed values the shape is set to the default but
            shape strings not in the list are not checked
        """
        try:
            # deal with ID numbers as arguments
            new_shape = self._shape_ids[int(new_shape)]
        except ValueError:
            # means that we weren't passed an ID number, the code below treats it as a shape name
            pass
        except KeyError:
            mantid.sendLogMessage("::SANS::Warning: Invalid geometry type for sample: " + str(new_shape) + ". Setting default to " + self._default_shape)
            new_shape = self._default_shape

        self._shape = new_shape
        self._use_wksp_shape = False

        #check that the dimensions that we have make sense for our new shape
        if self._width:
            self.width = self._width
        if self._thickness:
            self.thickness = self._thickness
    
    def get_shape(self):
        if self._shape is None:
            return self._get_default('shape')
        else:
            return self._shape
        
    def set_width(self, width):
        self._width = float(width)
        self._use_wksp_width = False
        # For a disk the height=width
        if self._shape and self._shape.startswith('cylinder'):
            self._height = self._width
            self._use_wksp_height = False

    def get_width(self):
        if self._width is None:
            return self._get_default('width')
        else:
            return self._width
            
    def set_height(self, height):
        self._height = float(height)
        self._use_wksp_height = False
        
        # For a cylinder and sphere the height=width=radius
        if (not self._shape is None) and (self._shape.startswith('cylinder')):
            self._width = self._height
        self._use_wksp_widtht = False

    def get_height(self):
        if self._height is None:
            return self._get_default('height')
        else:
            return self._height

    def set_thickness(self, thickness):
        """
            Simply sets the variable _thickness to the value passed
        """
        #as only cuboids use the thickness the warning below may be informative
        #if (not self._shape is None) and (not self._shape == 'cuboid'):
        #    mantid.sendLogMessage('::SANS::Warning: Can\'t set thickness for shape "'+self._shape+'"')
        self._thickness = float(thickness)
        self._use_wksp_thickness = False

    def get_thickness(self):
        if self._thickness is None:
            return self._get_default('thickness')
        else:
            return self._thickness

    shape = property(get_shape, set_shape, None, None)
    width = property(get_width, set_width, None, None)
    height = property(get_height, set_height, None, None)
    thickness = property(get_thickness, set_thickness, None, None)

    def execute(self, reducer, workspace):
        """
            Reads the geometry information stored in the workspace
            but doesn't replace values that have been previously set
        """
        wksp = mtd[workspace] 
        if wksp.isGroup():
            wksp = wksp[0]
        sample_details = wksp.getSampleInfo()

        if self._use_wksp_shape:
            self.shape = sample_details.getGeometryFlag()
        if self._use_wksp_thickness:
            self.thickness = sample_details.getThickness()
        if self._use_wksp_width:
            self.width = sample_details.getWidth()
        if self._use_wksp_height:
            self.height = sample_details.getHeight()

    def __str__(self):
        return '-- Sample Geometry --\n' + \
               '    Shape: ' + self.shape+'\n'+\
               '    Width: ' + str(self.width)+'\n'+\
               '    Height: ' + str(self.height)+'\n'+\
               '    Thickness: ' + str(self.thickness)+'\n'

class SampleGeomCor(ReductionStep):
    """
        Correct the neutron count rates for the size of the sample
    """
    def __init__(self, geometry):
        """
            Takes a reference to the sample geometry
            @param geometry: A GetSampleGeom object to load the sample dimensions from
            @raise TypeError: if an object of the wrong type is passed to it
        """
        super(SampleGeomCor, self).__init__()

        if issubclass(geometry.__class__, GetSampleGeom):
            self.geo = geometry
        else:
            raise TypeError, 'Sample geometry correction requires a GetSampleGeom object'

    def execute(self, reducer, workspace):
        """
            Divide the counts by the volume of the sample
        """

        try:
            if self.geo.shape == 'cylinder-axis-up':
                # Volume = circle area * height
                # Factor of four comes from radius = width/2
                volume = self.geo.height*math.pi
                volume *= math.pow(self.geo.width,2)/4.0
            elif self.geo.shape == 'cuboid':
                volume = self.geo.width
                volume *= self.geo.height*self.geo.thickness
            elif self.geo.shape == 'cylinder-axis-along':
                # Factor of four comes from radius = width/2
                volume = self.geo.thickness*math.pi
                volume *= math.pow(self.geo.width, 2)/4.0
            else:
                raise NotImplemented('Shape "'+self.geo.shape+'" is not in the list of supported shapes')
        except TypeError:
            raise TypeError('Error calculating sample volume with width='+str(self._width) + ' height='+str(self._height) + 'and thickness='+str(self._thickness)) 
        
        ws = mtd[workspace]
        ws /= volume

class StripEndZeros(ReductionStep):
    def __init__(self, flag_value = 0.0):
        super(StripEndZeros, self).__init__()
        self._flag_value = flag_value
        
    def execute(self, reducer, workspace):
        result_ws = mantid.getMatrixWorkspace(workspace)
        if result_ws.getNumberHistograms() != 1:
            #Strip zeros is only possible on 1D workspaces
            return

        y_vals = result_ws.readY(0)
        length = len(y_vals)
        # Find the first non-zero value
        start = 0
        for i in range(0, length):
            if ( y_vals[i] != self._flag_value ):
                start = i
                break
        # Now find the last non-zero value
        stop = 0
        length -= 1
        for j in range(length, 0,-1):
            if ( y_vals[j] != self._flag_value ):
                stop = j
                break
        # Find the appropriate X values and call CropWorkspace
        x_vals = result_ws.readX(0)
        startX = x_vals[start]
        # Make sure we're inside the bin that we want to crop
        endX = 1.001*x_vals[stop + 1]
        CropWorkspace(workspace,workspace,startX,endX)

class StripEndNans(ReductionStep):
    def __init__(self):
        super(StripEndNans, self).__init__()
        
    def _isNan(self, val):
        """
            Can replaced by isNaN in Python 2.6
            @param val: float to check
        """
        if val != val:
            return True
        else:
            return False
        
    def execute(self, reducer, workspace):
        """
            Trips leading and trailing Nan values from workspace
            @param reducer: unused
            @param workspace: the workspace to convert
        """
        result_ws = mantid.getMatrixWorkspace(workspace)
        if result_ws.getNumberHistograms() != 1:
            #Strip zeros is only possible on 1D workspaces
            return

        y_vals = result_ws.readY(0)
        length = len(y_vals)
        # Find the first non-zero value
        start = 0
        for i in range(0, length):
            if not self._isNan(y_vals[i]):
                start = i
                break
        # Now find the last non-zero value
        stop = 0
        length -= 1
        for j in range(length, 0,-1):
            if not self._isNan(y_vals[j]):
                stop = j
                break
        # Find the appropriate X values and call CropWorkspace
        x_vals = result_ws.readX(0)
        startX = x_vals[start]
        # Make sure we're inside the bin that we want to crop
        endX = 1.001*x_vals[stop + 1]
        CropWorkspace(workspace,workspace,startX,endX)
