from PyQt4 import QtGui, uic, QtCore
import util
import os
import functools
from reduction.hfir_reduction_steps import DataSets
from application_settings import GeneralSettings
from base_widget import BaseWidget
from mask import MaskWidget

class DataWidget(BaseWidget):    
    """
        Widget that presents the transmission options to the user
    """
    
    def __init__(self, parent=None, state=None, settings=None):
        BaseWidget.__init__(self, parent=parent, state=state, settings=settings)

        #f = QtCore.QFile(":/hfir_data.ui")
        f = QtCore.QFile("ui/hfir_data.ui")
        f.open(QtCore.QIODevice.ReadOnly)
        uic.loadUi(f, self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(DataSets())
            
        self._mask_widget = None

    
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        self._mask_widget = MaskWidget()
        
        self._content.placeholder_layout.addWidget(self._mask_widget)
        #self._content.placeholder_layout.setGeometry(QtCore.QRect(200,200,200,200))
        #self._mask_widget.resize(400, 400)
        self._mask_widget.setMinimumSize(400, 400)
        self._content.repaint()
        print self._mask_widget.height(), self._mask_widget.width()

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = DataSets()
        return m
        
        