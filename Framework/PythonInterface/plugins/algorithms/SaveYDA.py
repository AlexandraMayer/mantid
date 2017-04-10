from mantid.api import *
from mantid.kernel import*

import ruamel.yaml

from ruamel.yaml.comments import CommentedMap


import yaml
from yaml import CDumper as Dumper
import math

class SaveYDAPy(PythonAlgorithm):
    
    def category(self):
        return 'DataHandling'   
    
    def PyInit(self):
        
        wsValidators = CompositeValidator()
        wsValidators.add(WorkspaceUnitValidator("DeltaE"))
        wsValidators.add(InstrumentValidator())
        
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace","",validator=wsValidators,direction=Direction.Input),doc="Workspace to save")
        self.declareProperty(FileProperty(name="Filename",defaultValue="",action=FileAction.Save,extensions=""),"The name to use when writing the file")
        
        
    def getBinCenters(self,ax,bin):
        
        #ax = None
        bin = []
        
        for i in range(1,ax.size):
                bin.append((ax[i]+ax[i-1])/2)     
            
        
        return bin
        
        
    def validateInputs(self):
        
        issues = dict()
        
        #allowedUnits = ['q']
        ws = self.getProperty("InputWorkspace").value
        
        allowedUnit = 'MomentumTransfer'
        ax = ws.getAxis(1)
        
        if not ax.isSpectra() and ax.getUnit().unitID() != allowedUnit:
            issues["InputWorkspace"] = "Y axis is not 'Spectrum Axis' or 'Momentum Transfer'"
        
        if isinstance(ws,IEventWorkspace):
            issues["InputWorkspace"] = "The InputWorkspace must be a Workspace2D"

        return issues

        
    def PyExec(self):
        
        ws = mtd[self.getPropertyValue('InputWorkspace')]
        filename = self.getProperty("Filename").value
        
        #file = open(filename,"w")
        
        run = ws.getRun()
        ax = ws.getAxis(1)
        nHist = ws.getNumberHistograms()
        
        metadata = CommentedMap()
       
        metadata["format"] = "yaml/frida 2.0"
        metadata["type"] = "gerneric tabular data"
        
        
        hist = []
        
        propn ="Proposal number " + run.getLogData("proposal_number").value
        propt = run.getLogData("proposal_title").value
        expt = run.getLogData("experiment_team").value
        
        hist.append(propn)
        hist.append(propt)
        hist.append(expt)
        hist.append("data reduced with mantid")
        
        
        rpar = []
        
        temperature = run.getLogData("temperature").value
        eimeV = run.getLogData("Ei").value
        
        temp = dict()
        temp["name"] = "T"
        temp["unit"] = "K"
        temp["val"] =  temperature
        temp["stdv"] = 0
        
        ei = dict()
        ei["name"] = "Ei"
        ei["unit"] = "meV"
        ei["val"] = eimeV
        ei["stdv"] = 0
        
        rpar.append(temp)
        rpar.append(ei)
        
        coord = []
        
        xc = dict()
        x = dict()
        
        x["name"] = "w"
        x["unit"] = "meV"
        
        xc["x"] =  x
     
        yc = dict()
        y = dict()
        
        y["name"] = "S(q,w)"
        y["unit"] = "meV-1"
       
        yc["y"] = y
        
        zc = dict()
        z = dict()
        
        if(ax.isSpectra):
            zname = "2th"
            zunit = "deg"
        else:
            zname = "q"
            zunit = "A-1"
        
        z["name"] =  zname
        z["unit"] = zunit
        
        zc["z"] = z
        
        coord.append(xc)
        coord.append(yc)
        coord.append(zc)
        
        slices = []
        
        bin = []
        
        if(ax.isSpectra):
            samplePos = ws.getInstrument().getSample().getPos()
            sourcePos = ws.getInstrument().getSource().getPos()
            beamPos = samplePos - sourcePos
            for i in range(nHist):
                detector = ws.getDetector(i)
                twoTheta = detector.getTwoTheta(samplePos,beamPos)*180/math.pi
                bin.append(twoTheta)
        elif(ax.length() == nHist):
            for i in range(ax.length()):
                bin.append(ax.getValue())
        else:
            bin = self.getBinCenters(ax,bin).bin
       
        for i in range(nHist):
            
            #ys = ws.dataY(i)
            ys = ws.dataY(0)
            test = ax.extractValues()
            #bin?M  self.log().debug(str(ws.readX(0)[0]))
            #self.log().debug(str(ys[0]))
            #self.log().debug(str(test[0]))
            
            
            yv = []
            xcenters = []
  
            for j in range(ys.size):
                yv.append(ys[j])
            
            xax = ws.readX(i)
            #self.log().debug(str(xax[1]))
            xcenters = self.getBinCenters(xax,xcenters)
            
            
           # self.log().debug(str(xcenters[0]))
            
            
            slicethis = dict()
            
            val = dict()
            slicethis["j"] =  i
            val["val"] = bin[i]
            slicethis["z"] =  dict( val = bin[i])
            slicethis ["x"] = str(xcenters)
            slicethis["y"] =  str(yv)
            
            slices.append(slicethis)

        
        data = CommentedMap()
        
        data["Meta"] = metadata
        data["History"]  = hist
        data["Coord"] = coord
        data["RPar"] = rpar
        
        data["Slices"] = slices
        
        with open(filename,'w') as outfile:
            yaml.dump(data,outfile,Dumper=Dumper, default_flow_style=False)
        
            #ruamel.yaml.round_trip_dump(data,outfile)   
  
        
        
AlgorithmFactory.subscribe(SaveYDAPy) 