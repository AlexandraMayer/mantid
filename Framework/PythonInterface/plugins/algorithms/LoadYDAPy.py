import mantid
import mantid.simpleapi as sapi
import os

from mantid.api import *
from mantid.kernel import*

from yaml import load
import math
import numpy as np
from yaml import CLoader as Loader, CDumper as Dumper

class LoadYDAPy(PythonAlgorithm):

    def category(self):
        return 'DataHandling'   
        
    def name(self):
        return 'LoadYDAPy'

    def PyInit(self):
        
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace","",direction=Direction.Output),doc="The name to use when saving the Workspace")
        self.declareProperty(FileProperty(name="Filename",defaultValue="",action=FileAction.Load,extensions=""),"The File to load from")
        
    def PyExec(self):
        
        ws = mtd[self.getPropertyValue('OutputWorkspace')]
        filename = self.getProperty("Filename").value
        
        f = open(filename)
        #dataMap = yaml.safe_load(f)
        #f.close()
        dataMap = load(f, Loader=Loader)
        f.close()

        #----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        slices = dataMap["Slices"]
        self.log().debug( "Slices")
        x = []
        y = []
        e = []

        for i in range(len(slices)):
            #xati = self._StrToFltList(slices[i]["x"])
            xati = slices[i]["x"]
            #strlis = strlis.replace('[','')
            #strlis = strlis.replace(']','')
            #print strlis
            #xati = [float(num) for num in strlis.split(',')]
            #print xati[0]
            diff = xati[1]-xati[0]
            first = np.round(((xati[0] -diff)+xati[0])/2,4)
            xs = []
            xs.append(first)
            for k in range(len(xati)):
                xs.append(first + diff)
                first = first + diff
            x.append(xs)
            #yati = self._StrToFltList(slices[i]["y"])
            yati = slices[i]["y"]
            y.append(yati)
            eati = []
            for j in yati:
                eati.append(math.sqrt(j))
            e.append(eati)
            
            
        dataX = np.array(x,np.float)
        dataY = np.array(y,np.float)
        dataE = np.array(e,np.float)

        outws = sapi.CreateWorkspace(Outputworkspace=ws,DataX=dataX,DataY=dataY,DataE=dataE,NSpec=len(slices),UnitX="DeltaE")
        self.log().debug( "created workspace")
        #----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        hist = dataMap["History"]
        
        self.log().debug(str(hist))
        
        sapi.AddSampleLog(outws,LogName='proposal_number',LogText=hist[0][-1:],LogType='Number')
        self.log().debug(str(  hist[0][-1:]))
        sapi.AddSampleLog(outws,LogName='proposal_title',LogText=hist[1],LogType='String')
        self.log().debug(str( hist[1]))
        sapi.AddSampleLog(outws,LogName='experiment_team',LogText=hist[2],LogType='String')
        self.log().debug(str( hist[2]))
        
        #----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        rpar = dataMap["RPar"]
        
        sapi.AddSampleLog(outws,LogName='temperature',LogText=str(rpar[0]['val']),LogType='Number',LogUnit=rpar[0]['unit'])
        self.log().debug('temperature')
        self.log().debug(str(rpar[0]['val']))
        self.log().debug(str(rpar[0]['unit']))
        sapi.AddSampleLog(outws,LogName='Ei',LogText=str(rpar[1]['val']),LogType='Number',LogUnit=rpar[1]['unit'])
        self.log().debug('Ei')
        self.log().debug(str(rpar[1]['val']))
        self.log().debug(str(rpar[1]['unit']))
        
        #----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        coord = dataMap["Coord"]
        z = coord["z"]["name"]
        self.log().debug(str(coord["z"]["name"]))
        if (z == 'q'):
            outws.getAxis(1).setUnit('MomentumTransfer')
            
    def _StrToFltList(self, str):

        str = str.replace('[','')
        str = str.replace(']','')
        return [float(num) for num in str.split(',')]
        
##########################################################################################
AlgorithmFactory.subscribe(LoadYDAPy) 
    