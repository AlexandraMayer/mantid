from PyQt4 import QtCore, QtGui
import sys
import mantid
import numpy
from ValidateOL import ValidateOL

class ClassicUBInputWidget(QtGui.QWidget): 
    #signal when lattice is changed and valid
    changed=QtCore.pyqtSignal(mantid.geometry.OrientedLattice)   
    def __init__(self,ol=None,parent=None):
        super(ClassicUBInputWidget,self).__init__()
        #validators
        self.latticeLengthValidator=QtGui.QDoubleValidator(0.1,1000.,5,self)
        self.latticeAngleValidator=QtGui.QDoubleValidator(5.,175.,5,self)
        self.doubleValidator=QtGui.QDoubleValidator(self)
        #OrientedLattice
        if ValidateOL(ol):
            self.ol=ol
        else:
            self.ol=mantid.geometry.OrientedLattice()  
        self.latt_a=self.ol.a()
        self.latt_b=self.ol.b()
        self.latt_c=self.ol.c()
        self.latt_alpha=self.ol.alpha()
        self.latt_beta=self.ol.beta()
        self.latt_gamma=self.ol.gamma()
        uvec=self.ol.getuVector()
        self.latt_ux=uvec.X()
        self.latt_uy=uvec.Y()
        self.latt_uz=uvec.Z()      
        vvec=self.ol.getvVector()
        self.latt_vx=vvec.X()
        self.latt_vy=vvec.Y()
        self.latt_vz=vvec.Z()
        #labels
        self._labela=QtGui.QLabel('a')
        self._labelb=QtGui.QLabel('b')
        self._labelc=QtGui.QLabel('c')
        self._labelalpha=QtGui.QLabel('alpha')
        self._labelbeta=QtGui.QLabel(' beta')
        self._labelgamma=QtGui.QLabel('gamma')        
        self._labelux=QtGui.QLabel('ux')
        self._labeluy=QtGui.QLabel('uy')
        self._labeluz=QtGui.QLabel('uz')
        self._labelvx=QtGui.QLabel('vx')
        self._labelvy=QtGui.QLabel('vy')
        self._labelvz=QtGui.QLabel('vz')
        #lineedits
        self._edita=QtGui.QLineEdit() 
        self._edita.setValidator(self.latticeLengthValidator)
        self._editb=QtGui.QLineEdit() 
        self._editb.setValidator(self.latticeLengthValidator)
        self._editc=QtGui.QLineEdit()
        self._editc.setValidator(self.latticeLengthValidator)  
        self._editalpha=QtGui.QLineEdit() 
        self._editalpha.setValidator(self.latticeAngleValidator)
        self._editbeta=QtGui.QLineEdit() 
        self._editbeta.setValidator(self.latticeAngleValidator)
        self._editgamma=QtGui.QLineEdit() 
        self._editgamma.setValidator(self.latticeAngleValidator)
        self._editux=QtGui.QLineEdit() 
        self._editux.setValidator(self.doubleValidator)
        self._edituy=QtGui.QLineEdit() 
        self._edituy.setValidator(self.doubleValidator)
        self._edituz=QtGui.QLineEdit() 
        self._edituz.setValidator(self.doubleValidator)
        self._editvx=QtGui.QLineEdit() 
        self._editvx.setValidator(self.doubleValidator)
        self._editvy=QtGui.QLineEdit() 
        self._editvy.setValidator(self.doubleValidator)
        self._editvz=QtGui.QLineEdit() 
        self._editvz.setValidator(self.doubleValidator)
        #layout
        grid = QtGui.QGridLayout()
        self.setLayout(grid)
        grid.addWidget(self._labela,0,0,QtCore.Qt.AlignRight)
        grid.addWidget(self._edita,0,1)
        grid.addWidget(self._labelb,0,2,QtCore.Qt.AlignRight)
        grid.addWidget(self._editb,0,3)
        grid.addWidget(self._labelc,0,4,QtCore.Qt.AlignRight)
        grid.addWidget(self._editc,0,5)
        grid.addWidget(self._labelalpha,1,0,QtCore.Qt.AlignRight)
        grid.addWidget(self._editalpha,1,1)
        grid.addWidget(self._labelbeta,1,2,QtCore.Qt.AlignRight)
        grid.addWidget(self._editbeta,1,3)
        grid.addWidget(self._labelgamma,1,4,QtCore.Qt.AlignRight)
        grid.addWidget(self._editgamma,1,5)
        grid.addWidget(self._labelux,2,0,QtCore.Qt.AlignRight)
        grid.addWidget(self._editux,2,1)
        grid.addWidget(self._labeluy,2,2,QtCore.Qt.AlignRight)
        grid.addWidget(self._edituy,2,3)
        grid.addWidget(self._labeluz,2,4,QtCore.Qt.AlignRight)
        grid.addWidget(self._edituz,2,5)        
        grid.addWidget(self._labelvx,3,0,QtCore.Qt.AlignRight)
        grid.addWidget(self._editvx,3,1)
        grid.addWidget(self._labelvy,3,2,QtCore.Qt.AlignRight)
        grid.addWidget(self._editvy,3,3)
        grid.addWidget(self._labelvz,3,4,QtCore.Qt.AlignRight)
        grid.addWidget(self._editvz,3,5) 
        #update the GUI
        self.updateGui()
        #connections
        self._edita.textEdited.connect(self.check_state_latt)
        self._editb.textEdited.connect(self.check_state_latt)
        self._editc.textEdited.connect(self.check_state_latt)
        self._editalpha.textEdited.connect(self.check_state_latt)
        self._editbeta.textEdited.connect(self.check_state_latt)
        self._editgamma.textEdited.connect(self.check_state_latt)
        self._editux.textEdited.connect(self.check_state_orientation)
        self._edituy.textEdited.connect(self.check_state_orientation)
        self._edituz.textEdited.connect(self.check_state_orientation) 
        self._editvx.textEdited.connect(self.check_state_orientation)
        self._editvy.textEdited.connect(self.check_state_orientation)
        self._editvz.textEdited.connect(self.check_state_orientation)       
        
    def updateGui(self):
        self._edita.setText(QtCore.QString(format(self.latt_a,'.5f')))   
        self._editb.setText(QtCore.QString(format(self.latt_b,'.5f'))) 
        self._editc.setText(QtCore.QString(format(self.latt_c,'.5f')))  
        self._editalpha.setText(QtCore.QString(format(self.latt_alpha,'.5f')))   
        self._editbeta.setText(QtCore.QString(format(self.latt_beta,'.5f'))) 
        self._editgamma.setText(QtCore.QString(format(self.latt_gamma,'.5f')))
        self._editux.setText(QtCore.QString(format(self.latt_ux,'.5f')))   
        self._edituy.setText(QtCore.QString(format(self.latt_uy,'.5f'))) 
        self._edituz.setText(QtCore.QString(format(self.latt_uz,'.5f')))        
        self._editvx.setText(QtCore.QString(format(self.latt_vx,'.5f')))   
        self._editvy.setText(QtCore.QString(format(self.latt_vy,'.5f'))) 
        self._editvz.setText(QtCore.QString(format(self.latt_vz,'.5f')))   
        
    def check_state_orientation(self, *args, **kwargs):  
        edits=[self._editux,self._edituy,self._edituz,self._editvx,self._editvy,self._editvz]
        uvector=numpy.array([float(self._editux.text()),float(self._edituy.text()),float(self._edituz.text())])
        vvector=numpy.array([float(self._editvx.text()),float(self._editvy.text()),float(self._editvz.text())]) 
        if numpy.linalg.norm(numpy.cross(uvector,vvector))>1e-5:
            #change all colors to white
            for edit in edits:
                edit.setStyleSheet('QLineEdit { background-color: #ffffff }')
            self.validateAll()
        else:
            self.sender().setStyleSheet('QLineEdit { background-color: #ffaaaa }')
            
    def check_state_latt(self, *args, **kwargs):
        sender = self.sender()
        validator = sender.validator()
        state = validator.validate(sender.text(), 0)[0]
   
        if state == QtGui.QValidator.Acceptable:
            color = '#ffffff' 
        elif state == QtGui.QValidator.Intermediate:
            color = '#ffaaaa' 
        else:
            color = '#ffaaaa' 
        sender.setStyleSheet('QLineEdit { background-color: %s }' % color)
        if state == QtGui.QValidator.Acceptable:
            self.validateAll()

    def validateAll(self):
        self.latt_a=float(self._edita.text())
        if(self.latt_a<0.1):
            return
        self.latt_b=float(self._editb.text())
        if(self.latt_b<0.1):
            return
        self.latt_c=float(self._editc.text())
        if(self.latt_c<0.1):
            return
        self.latt_alpha=float(self._editalpha.text())
        if(self.latt_alpha<5 or self.latt_alpha>175):
            return
        self.latt_beta=float(self._editbeta.text())
        if(self.latt_beta<5 or self.latt_beta>175):
            return
        self.latt_gamma=float(self._editgamma.text())
        if(self.latt_gamma<5 or self.latt_gamma>175):
            return
        self.latt_ux=float(self._editux.text())
        self.latt_uy=float(self._edituy.text())
        self.latt_uz=float(self._edituz.text())    
        self.latt_vx=float(self._editvx.text())
        self.latt_vy=float(self._editvy.text())
        self.latt_vz=float(self._editvz.text())   
        uvec=numpy.array([self.latt_ux,self.latt_uy,self.latt_uz])  
        vvec=numpy.array([self.latt_vx,self.latt_vy,self.latt_vz])
        if numpy.linalg.norm(numpy.cross(uvec,vvec))<1e-5:
            return
        self.ol=mantid.geometry.OrientedLattice(self.latt_a,self.latt_b,self.latt_c,self.latt_alpha,self.latt_beta,self.latt_gamma)
        self.ol.setUFromVectors(uvec,vvec)
        self.changed.emit(self.ol)
        
if __name__=='__main__':
    app=QtGui.QApplication(sys.argv)
    mainForm=ClassicUBInputWidget()
    mainForm.show()
    sys.exit(app.exec_())
        
        
