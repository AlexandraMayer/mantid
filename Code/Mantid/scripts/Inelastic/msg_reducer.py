# msg_reducer.py
# Reducers for use by ISIS Molecular Spectroscopy Group
import os.path

from mantidsimple import *
import reducer
import inelastic_indirect_reduction_steps as steps

class MSGReducer(reducer.Reducer):
    """This is the base class for the reducer classes to be used by the ISIS
    Molecular Spectroscopy Group (MSG). It exists to serve the functions that
    are common to both spectroscopy and diffraction workflows in the hopes of
    providing a semi-consistent interface to both.
    """
    
    _instrument_name = None #: Name of the instrument used in experiment
    _sum = False #: Whether to sum input files or treat them sequentially
    _monitor_index = None #: Index of Monitor specturm
    _multiple_frames = False
    _detector_range = [-1, -1]
    _masking_detectors = []
    _parameter_file = None
    
    def __init__(self):
        super(MSGReducer, self).__init__()
        
    def pre_process(self):
        self._reduction_steps = []
        
        loadData = steps.LoadData()
        loadData.set_ws_list(self._data_files)
        loadData.set_sum(self._sum)
        loadData.set_monitor_index(self._monitor_index)
        loadData.set_detector_range(self._detector_range[0],
            self._detector_range[1])
        loadData.set_parameter_file(self._parameter_file)
        loadData.execute(self, None)
        
        self._multiple_frames = loadData.is_multiple_frames()
        self._masking_detectors = loadData.get_mask_list()
        
        if ( self._sum ):
            self._data_files = loadData.get_ws_list()
            
        self._setup_steps()

    def set_detector_range(self, start, end):
        """Sets the start and end detector points for the reduction process.
        These numbers are to be the workspace index, not the spectrum number.
        """
        if ( not isinstance(start, int) ) or ( not isinstance(end, int) ):
            raise TypeError("start and end must be integer values")
        self._detector_range = [ start, end ]
        
    def set_instrument_name(self, instrument):
        """Unlike the SANS reducers, we do not create a class to describe the
        instruments. Instead, we load the instrument and parameter file and
        query it for information.
        Raises:
            * ValueError if an instrument name is not provided.
            * RuntimeError if IDF could not be found or is invalid.
            * RuntimeError if workspace index of the Monitor could not be
                determined.
        Example use:
            reducer.set_instrument_name("IRIS")
        """
        if not isinstance(instrument, str):
            raise ValueError("Instrument name must be given.")
        self._instrument_name = instrument
        self._load_empty_instrument()
        self._get_monitor_index()
        if ( self._monitor_index is None ):
            raise RuntimeError("Could not find Monitor in Instrument.")
        
    def set_parameter_file(self, file):
        """Sets the parameter file to be used in the reduction. The parameter
        file will contain some settings that are used throughout the reduction
        process.
        Note: This is *not* the base parameter file, ie "IRIS_Parameters.xml"
        but, rather, the additional parameter file.
        """
        if self._instrument_name is None:
            raise ValueError("Instrument name not set.")
        self._parameter_file = \
            os.path.join(mtd.settings["parameterDefinition.directory"], file)
        LoadParameterFile(self._workspace_instrument, 
            self._parameter_file)
        
    def set_sum_files(self, value):
        """Mark whether multiple runs should be summed together for the process
        or treated individually.
        The default value for this is False.
        """
        if not isinstance(value, bool):
            raise TypeError("value must be either True or False (boolean)")
        self._sum = value
        
    def _load_empty_instrument(self):
        """Returns an empty workspace for the instrument.
        Raises: 
            * ValueError if no instrument is selected.
            * RuntimeError if there is a problem with the IDF.
        """
        if self._instrument_name is None:
            raise ValueError('No instrument selected.')
        self._workspace_instrument = '__empty_' + self._instrument_name
        if mtd[self._workspace_instrument] is None:
            idf_dir = mtd.getConfigProperty('instrumentDefinition.directory')
            idf = idf_dir + self._instrument_name + '_Definition.xml'
            try:
                LoadEmptyInstrument(idf, self._workspace_instrument)
            except RuntimeError:
                raise ValueError('Invalid IDF')
        return mtd[self._workspace_instrument]

    def _get_monitor_index(self):
        """Determined the workspace index of the first monitor spectrum.
        """
        workspace = self._load_empty_instrument()
        for counter in range(0, workspace.getNumberHistograms()):
            try:
                detector = workspace.getDetector(counter)
            except RuntimeError:
                pass
            if detector.isMonitor():
                self._monitor_index = counter
                return
