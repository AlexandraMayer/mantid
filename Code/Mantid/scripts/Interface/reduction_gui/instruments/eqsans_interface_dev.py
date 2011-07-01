"""
    This module defines the interface control for EQSANS.
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (SNSReduction object)
"""
from interface import InstrumentInterface
from reduction_gui.widgets.sans.hfir_detector import DetectorWidget
from reduction_gui.widgets.sans.eqsans_instrument import SANSInstrumentWidget
from reduction_gui.widgets.sans.eqsans_sample_data import SampleDataWidget
from reduction_gui.widgets.sans.eqsans_background import BackgroundWidget
from reduction_gui.widgets.output import OutputWidget
from reduction_gui.reduction.eqsans_reduction import EQSANSReductionScripter

from reduction_gui.reduction.sans.eqsans_data_proxy import DataProxy

class EQSANSInterface(InstrumentInterface):
    """
        Defines the widgets for EQSANS reduction
    """
    data_type = "Data files *.nxs *.dat (*.nxs *.dat)"
    
    def __init__(self, name, settings):
        super(EQSANSInterface, self).__init__(name, settings)
        
        # Scripter object to interface with Mantid 
        self.scripter = EQSANSReductionScripter(name=name)        

        # Instrument description
        self.attach(SANSInstrumentWidget(settings = self._settings, data_proxy=DataProxy, data_type = self.data_type))
        
        # Detector
        self.attach(DetectorWidget(settings = self._settings, data_proxy=None, data_type = self.data_type))

        # Sample
        self.attach(SampleDataWidget(settings = self._settings, data_proxy=None, data_type = self.data_type))
        
        # Background
        self.attach(BackgroundWidget(settings = self._settings, data_proxy=None, data_type = self.data_type))
        
        # Reduction output
        self.attach(OutputWidget(settings = self._settings))
