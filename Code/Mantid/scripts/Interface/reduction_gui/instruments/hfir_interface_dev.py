"""
    This module defines the interface control for HFIR SANS.
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (HFIRReduction object)
"""
from interface import InstrumentInterface
from reduction_gui.widgets.sans.hfir_instrument import SANSInstrumentWidget
from reduction_gui.widgets.sans.hfir_detector import DetectorWidget
from reduction_gui.widgets.sans.hfir_sample_data import SampleDataWidget
from reduction_gui.widgets.sans.hfir_background import BackgroundWidget
from reduction_gui.widgets.output import OutputWidget
from reduction_gui.reduction.hfir_reduction import HFIRReductionScripter


class HFIRInterface(InstrumentInterface):
    """
        Defines the widgets for HFIR reduction
    """
    
    def __init__(self, name, settings):
        super(HFIRInterface, self).__init__(name, settings)
        
        # Scripter object to interface with Mantid 
        self.scripter = HFIRReductionScripter(name=name)        

        # Instrument description
        self.attach(SANSInstrumentWidget(settings = self._settings, name=name))
        
        # Detector
        self.attach(DetectorWidget(settings = self._settings))

        # Sample
        self.attach(SampleDataWidget(settings = self._settings))
        
        # Background
        self.attach(BackgroundWidget(settings = self._settings))
        
        # Reduction output
        self.attach(OutputWidget(settings = self._settings))
