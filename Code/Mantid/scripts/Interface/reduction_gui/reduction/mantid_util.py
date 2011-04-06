import sys
 
# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    import mantidsimple
    HAS_MANTID = True
except:
    HAS_MANTID = False 
    
# Check whether we have numpy
try:
    import numpy
    HAS_NUMPY = True
except:
    HAS_NUMPY = False
    
def get_data_proxy(settings, data_file):
    """
        Return a data proxy object that standardizes access to workspace information
        @param settings: GeneralSettings object
        @param data_file: Data file for which we want a data proxy object
    """
    pass

class DataFileProxy(object):
    
    wavelength = None
    wavelength_spread = None
    sample_detector_distance = None
    data = None
    data_ws = ''
    
    ## Error log
    errors = []
    
    def __init__(self, data_file):
        if HAS_MANTID:
            try:
                from reduction.instruments.sans.sans_reduction_steps import LoadRun
                from reduction.instruments.sans.sans_reducer import SANSReducer
                import reduction.instruments.sans.hfir_instrument as hfir_instrument
                self.data_ws = "raw_data_file"
                reducer = SANSReducer()
                reducer.set_instrument(hfir_instrument.HFIRSANS())
                loader = LoadRun(str(data_file))
                loader.execute(reducer, self.data_ws)
                x = mtd[self.data_ws].dataX(0)
                self.wavelength = (x[0]+x[1])/2.0
                self.wavelength_spread = x[1]-x[0]
                self.sample_detector_distance = mtd[self.data_ws].getRun().getProperty("sample_detector_distance").value
                
                if HAS_NUMPY:
                    raw_data = numpy.zeros(reducer.instrument.nx_pixels*reducer.instrument.ny_pixels)
                    for i in range(reducer.instrument.nMonitors-1, reducer.instrument.nx_pixels*reducer.instrument.ny_pixels+reducer.instrument.nMonitors ):
                        raw_data[i-reducer.instrument.nMonitors] = mtd[self.data_ws].readY(i)[0]
                        
                    self.data = numpy.reshape(raw_data, (reducer.instrument.nx_pixels, reducer.instrument.ny_pixels), order='F')
            except:
                self.errors.append("Error loading data file:\n%s" % sys.exc_value)
            
            
class EQSANSDataProxy(DataFileProxy):
    
    def __init__(self, data_file):
        self.data_file = data_file
        self.data_ws = None
        
    def load(self):
        if HAS_MANTID:
            try:
                from reduction.instruments.sans.sns_reduction_steps import QuickLoad
                from reduction.instruments.sans.sns_reducer import EqSansReducer
                import reduction.instruments.sans.sns_instrument as sns_instrument
                self.data_ws = "raw_data_file"
                reducer = EqSansReducer()
                reducer.set_instrument(sns_instrument.EQSANS())
                loader = QuickLoad(str(self.data_file))
                loader.execute(reducer, self.data_ws)                
            except:
                self.errors.append("Error loading data file:\n%s" % sys.exc_value)  
                
    def get_masked(self):
        if self.data_ws is not None:
            return mantidsimple.GetMaskedDetectors(self.data_ws).getPropertyValue("DetectorList")
        else:
            return ''

    def mask(self, state):
        pass
                
                  