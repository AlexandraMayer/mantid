import ClassicUBInputWidget
import MatrixUBInputWidget
from PyQt4 import QtCore, QtGui
import sys
import mantid
from ValidateOL import ValidateOL

class DGSPlannerGUI(QtGui.QWidget):
    def __init__(self,ol=None,parent=None):
        super(DGSPlannerGUI,self).__init__(parent)
        #OrientedLattice
        if ValidateOL(ol):
            self.ol=ol
        else:
            self.ol=mantid.geometry.OrientedLattice()  
        self.classic=ClassicUBInputWidget.ClassicUBInputWidget(self.ol)
        self.setLayout(QtGui.QHBoxLayout())
        self.layout().addWidget(self.classic)
        self.classic.changed.connect(self.printUB)
        self.matrix=MatrixUBInputWidget.MatrixUBInputWidget(self.ol)
        self.layout().addWidget(self.matrix)
        
    @QtCore.pyqtSlot(mantid.geometry.OrientedLattice)
    def printUB(self,ol):
        print ol.getUB()   

if __name__=='__main__':
    app=QtGui.QApplication(sys.argv)
    ol=mantid.geometry.OrientedLattice(2,3,4,90,90,90)
    mainForm=DGSPlannerGUI(ol)
    mainForm.show()
    sys.exit(app.exec_())
