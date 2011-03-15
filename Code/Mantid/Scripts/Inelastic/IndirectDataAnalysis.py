from mantidsimple import *
from mantidplot import *
import IndirectEnergyConversion as IEC

import math
import re
import os.path

def abscyl(inWS_n, outWS_n, efixed, sample, can):
    ConvertUnits(inWS_n, 'wavelength', 'Wavelength', 'Indirect', efixed)
    CylinderAbsorption('wavelength', outWS_n, sample[0], sample[1], sample[2],
        can[0],can[1], EMode='Indirect', EFixed=efixed, NumberOfSlices=can[2],
        NumberOfAnnuli=can[3])
    mantid.deleteWorkspace('wavelength')

def absflat(inWS_n, outWS_n, efixed, sample, can):
    ConvertUnits(inWS_n, 'wlength', 'Wavelength', 'Indirect', efixed)
    FlatPlateAbsorption('wlength', outWS_n, sample[0], sample[1], sample[2],
        can[0], can[1], can[2], EMode='Indirect', EFixed=efixed,
        ElementSize=can[3])
    mantid.deleteWorkspace('wlength')

def absorption(input, mode, sample, can, Save=False, Verbose=False,
        Plot=False):
    root = loadNexus(input)
    efixed = getEfixed(root)
    outWS_n = root[:-3] + 'abs'
    if mode == 'Flat Plate':
        absflat(root, outWS_n, efixed, sample, can)
    if mode == 'Cylinder':
        abscyl(root, outWS_n, efixed, sample, can)
    if Save:
        SaveNexus(outWS_n, outWS_n+'.nxs')
    mantid.deleteWorkspace(root)
    if Plot:
        graph = plotSpectrum(outWS_n,0)

def concatWSs(workspaces, unit, name):
    dataX = []
    dataY = []
    dataE = []
    for ws in workspaces:
        readX = mtd[ws].readX(0)
        readY = mtd[ws].readY(0)
        readE = mtd[ws].readE(0)
        for i in range(0, len(readX)):
            dataX.append(readX[i])
        for i in range(0, len(readY)):
            dataY.append(readY[i])
            dataE.append(readE[i])
    CreateWorkspace(name, dataX, dataY, dataE, NSpec=len(workspaces),
        UnitX=unit)

def confitParsToWS(Table, Data, BackG='FixF', specMin=0, specMax=-1):
    if ( specMax == -1 ):
        specMax = mtd[Data].getNumberHistograms() - 1
    dataX = createQaxis(Data)
    xAxisVals = []
    dataY = []
    dataE = []
    names = ''
    ws = mtd[Table]
    cName =  ws.getColumnNames()
    nSpec = ( ws.getColumnCount() - 1 ) / 2
    for spec in range(0,nSpec):
        yAxis = cName[(spec*2)+1]
        if re.search('HWHM$', yAxis) or re.search('Height$', yAxis):
            xAxisVals += dataX        
            if (len(names) > 0):
                names += ","
            names += yAxis
            eAxis = cName[(spec*2)+2]
            for row in range(0, ws.getRowCount()):
                dataY.append(ws.getDouble(yAxis,row))
                dataE.append(ws.getDouble(eAxis,row))
        else:
            nSpec -= 1
    suffix = str(nSpec / 2) + 'L' + BackG
    outNm = Table + suffix
    CreateWorkspace(outNm, xAxisVals, dataY, dataE, nSpec,
        UnitX='MomentumTransfer', VerticalAxisUnit='Text',
        VerticalAxisValues=names)
    return outNm

def confitPlotSeq(inputWS, plot):
    nhist = mtd[inputWS].getNumberHistograms()
    if ( plot == 'All' ):
        plotSpectrum(inputWS, range(0, nhist))
        return    
    plotSpecs = []
    if ( plot == 'Intensity' ):
        res = 'Height$'
    elif ( plot == 'HWHM' ):
        res = 'HWHM$'
    for i in range(0,nhist):
        title = mtd[inputWS].getAxis(1).label(i)
        if re.search(res, title):
            plotSpecs.append(i)
    plotSpectrum(inputWS, plotSpecs)

def confitSeq(inputWS, func, startX, endX, save, plot, bg, specMin, specMax):
    input = inputWS+',i' + str(specMin)
    if (specMax == -1):
        specMax = mtd[inputWS].getNumberHistograms() - 1
    for i in range(specMin + 1, specMax + 1):
        input += ';'+inputWS+',i'+str(i)
    outNm = getWSprefix(inputWS) + 'conv_'
    PlotPeakByLogValue(input, outNm, func, StartX=startX, EndX=endX)
    wsname = confitParsToWS(outNm, inputWS, bg, specMin, specMax)
    if save:
            SaveNexusProcessed(wsname, wsname+'.nxs')
    if plot != 'None':
        confitPlotSeq(wsname, plot)

def demon(rawfiles, first, last, instrument, Smooth=False, SumFiles=False,
        grouping='Individual', cal='', CleanUp=True, Verbose=False, 
        Plot='None', Save=True, Real=False, Vanadium='', Monitor=True):
    '''DEMON routine for diffraction reduction on indirect instruments (IRIS /
    OSIRIS).'''
    # Get instrument workspace for gathering parameters and such
    wsInst = mtd['__empty_' + instrument]
    if wsInst is None:
        IEC.loadInst(instrument)
        wsInst = mtd['__empty_' + instrument]
    # short name of instrument for saving etc
    isn = ConfigService().facility().instrument(instrument).shortName().lower()
    # parameters to do with monitor
    if Monitor:
        fmon, fdet = IEC.getFirstMonFirstDet('__empty_'+instrument)
        try:
            inst = wsInst.getInstrument()
            area = inst.getNumberParameter('mon-area')[0]
            thickness = inst.getNumberParameter('mon-thickness')[0]
        except IndexError, message:
            print message
            sys.exit(message)
        ws_mon_l = IEC.loadData(rawfiles, Sum=SumFiles, Suffix='_mon',
            SpecMin=fmon+1, SpecMax=fmon+1)
    ws_det_l = IEC.loadData(rawfiles, Sum=SumFiles, SpecMin=first, 
        SpecMax=last)
    workspaces = []
    for i in range(0, len(ws_det_l)):
        det_ws = ws_det_l[i]
        # Get Run No
        runNo = mtd[det_ws].getRun().getLogData("run_number").value
        savefile = isn + runNo + '_diff'
        if Monitor:
            mon_ws = ws_mon_l[i]
            # Get Monitor WS
            IEC.timeRegime(monitor=mon_ws, detectors=det_ws, Smooth=Smooth)
            IEC.monitorEfficiency(mon_ws, area, thickness)
            IEC.normToMon(Factor=1e6, monitor=mon_ws, detectors=det_ws)
            # Remove monitor workspace
            DeleteWorkspace(mon_ws)
        if ( cal != '' ): # AlignDetectors and Group by .cal file
            if Monitor:
                ConvertUnits(det_ws, det_ws, 'TOF')
            if ( mtd[det_ws].isDistribution() ):
                ConvertFromDistribution(det_ws)
            AlignDetectors(det_ws, det_ws, cal)
            CloneWorkspace(det_ws, 'demon_CorByMon-and-Aligned')
            DiffractionFocussing(det_ws, savefile, cal)
            DeleteWorkspace(det_ws)
            if ( Vanadium != '' ):
                print "NotImplemented: divide by vanadium."
        else: ## Do it the old fashioned way
            # Convert to dSpacing - need to AlignBins so we can group later
            ConvertUnits(det_ws, det_ws, 'dSpacing', AlignBins=True)
            IEC.groupData(grouping, savefile, detectors=det_ws)
        if Save:
            SaveNexusProcessed(savefile, savefile+'.nxs')
        workspaces.append(savefile)
    if ( Plot != 'None' ):
        for demon in workspaces:
            if ( Plot == 'Contour' ):
                importMatrixWorkspace(demon).plotGraph2D()
            else:
                nspec = mtd[demon].getNumberHistograms()
                plotSpectrum(demon, range(0, nspec))
    return workspaces

def elwin(inputFiles, eRange, Save=False, Verbose=False, Plot=False):
    eq1 = [] # output workspaces with units in Q
    eq2 = [] # output workspaces with units in Q^2
    for file in inputFiles:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        LoadNexus(file, root)
        run = mtd[root].getRun().getLogData("run_number").value
        savefile = root[:3] + run + root[8:-3]
        if ( len(eRange) == 4 ):
            ElasticWindow(root, savefile+'eq1', savefile+'eq2',eRange[0],
                eRange[1], eRange[2], eRange[3])
        elif ( len(eRange) == 2 ):
            ElasticWindow(root, savefile+'eq1', savefile+'eq2', 
            eRange[0], eRange[1])
        if Save:
            SaveNexusProcessed(savefile+'eq1', savefile+'eq1.nxs')
            SaveNexusProcessed(savefile+'eq2', savefile+'eq2.nxs')
        eq1.append(savefile+'eq1')
        eq2.append(savefile+'eq2')
        mantid.deleteWorkspace(root)
    if Plot:
        nBins = mtd[eq1[0]].getNumberBins()
        lastXeq1 = mtd[eq1[0]].readX(0)[nBins-1]
        graph1 = plotSpectrum(eq1, 0)
        layer = graph1.activeLayer()
        layer.setScale(Layer.Bottom, 0.0, lastXeq1)
        nBins = mtd[eq2[0]].getNumberBins()
        lastXeq2 = mtd[eq2[0]].readX(0)[nBins-1]
        graph2 = plotSpectrum(eq2, 0)
        layer = graph2.activeLayer()
        layer.setScale(Layer.Bottom, 0.0, lastXeq2)
    return eq1, eq2

def fury(sam_files, res_file, rebinParam, RES=True, Save=False, Verbose=False,
        Plot=False):
    outWSlist = []
    # Process RES Data Only Once
    LoadNexus(res_file, 'res_data') # RES
    Rebin('res_data', 'res_data', rebinParam)
    ExtractFFTSpectrum('res_data', 'res_fft', 2)
    Integration('res_data', 'res_int')
    Divide('res_fft', 'res_int', 'res')
    for sam_file in sam_files:
        (direct, filename) = os.path.split(sam_file)
        (root, ext) = os.path.splitext(filename)
        if (ext == '.nxs'):
            LoadNexus(sam_file, 'sam_data') # SAMPLE
            Rebin('sam_data', 'sam_data', rebinParam)
        else: #input is workspace
            Rebin(sam_file, 'sam_data', rebinParam)
        ExtractFFTSpectrum('sam_data', 'sam_fft', 2)
        Integration('sam_data', 'sam_int')
        Divide('sam_fft', 'sam_int', 'sam')
        # Create save file name
        savefile = getWSprefix('sam_data') + 'iqt'
        outWSlist.append(savefile)
        Divide('sam', 'res', savefile)
        #Cleanup Sample Files
        mantid.deleteWorkspace('sam_data')
        mantid.deleteWorkspace('sam_int')
        mantid.deleteWorkspace('sam_fft')
        mantid.deleteWorkspace('sam')
        # Crop nonsense values off workspace
        bin = int(math.ceil(mtd[savefile].getNumberBins()/ 2.0))
        binV = mtd[savefile].dataX(0)[bin]
        CropWorkspace(savefile, savefile, XMax=binV)
        if Save:
            SaveNexusProcessed(savefile, savefile + '.nxs')
    # Clean Up RES files
    mantid.deleteWorkspace('res_data')
    mantid.deleteWorkspace('res_int')
    mantid.deleteWorkspace('res_fft')
    mantid.deleteWorkspace('res')
    if Plot:
        specrange = range(0,mtd[outWSlist[0]].getNumberHistograms())
        plotFury(outWSlist, specrange)
    return outWSlist

def furyfitParsToWS(Table, Data):
    dataX = createQaxis(Data)
    dataY = []
    dataE = []
    names = ""
    xAxisVals = []
    ws = mtd[Table]
    cCount = ws.getColumnCount()
    rCount = ws.getRowCount()
    cName =  ws.getColumnNames()
    nSpec = ( cCount - 1 ) / 2
    xAxis = cName[0]
    stretched = 0
    for spec in range(0,nSpec):
        yAxis = cName[(spec*2)+1]
        if ( re.search('Intensity$', yAxis) or re.search('Tau$', yAxis)
            or re.search('Beta$', yAxis) ):
            xAxisVals += dataX
            if (len(names) > 0):
                names += ","
            names += yAxis
            eAxis = cName[(spec*2)+2]
            for row in range(0, rCount):
                dataY.append(ws.getDouble(yAxis,row))
                dataE.append(ws.getDouble(eAxis,row))
            if ( re.search('Beta$', yAxis) ): # need to know how many of curves
                stretched += 1                # are stretched exponentials
        else:
            nSpec -= 1
    suffix = ''
    nE = ( nSpec / 2 ) - stretched
    if ( nE > 0 ):
        suffix += str(nE) + 'E'
    if ( stretched > 0 ):
        suffix += str(stretched) + 'S'
    wsname = Table + suffix
    CreateWorkspace(wsname, xAxisVals, dataY, dataE, nSpec,
        UnitX='MomentumTransfer', VerticalAxisUnit='Text',
        VerticalAxisValues=names)
    return wsname

def furyfitPlotSeq(inputWS, plot):
    nHist = mtd[inputWS].getNumberHistograms()
    if ( plot == 'All' ):
        plotSpectrum(inputWS, range(0, nHist))
        return
    plotSpecs = []
    if ( plot == 'Intensity' ):
        res = 'Intensity$'
    if ( plot == 'Tau' ):
        res = 'Tau$'
    elif ( plot == 'Beta' ):
        res = 'Beta$'    
    for i in range(0, nHist):
        title = mtd[inputWS].getAxis(1).label(i)
        if ( re.search(res, title) ):
            plotSpecs.append(i)
    plotSpectrum(inputWS, plotSpecs)

def furyfitSeq(inputWS, func, startx, endx, save, plot):
    input = inputWS+',i0'
    nHist = mtd[inputWS].getNumberHistograms()
    for i in range(1,nHist):
        input += ';'+inputWS+',i'+str(i)
    outNm = getWSprefix(inputWS) + 'fury_'
    PlotPeakByLogValue(input, outNm, func, StartX=startx, EndX=endx)
    wsname = furyfitParsToWS(outNm, inputWS)
    if save:
        SaveNexusProcessed(wsname, wsname+'.nxs')
    if ( plot != 'None' ):
        furyfitPlotSeq(wsname, plot)

def msdfit(inputs, startX, endX, Save=False, Verbose=False, Plot=False):
    output = []
    for file in inputs:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        LoadNexusProcessed(file, root)
        outWS_n = root[:-3] + 'msd'
        fit_alg = Linear(root, outWS_n, WorkspaceIndex=0, StartX=startX, 
            EndX=endX)
        output.append(outWS_n)
        A0 = fit_alg.getPropertyValue("FitIntercept")
        A1 = fit_alg.getPropertyValue("FitSlope")
        title = 'Intercept: '+A0+' ; Slope: '+A1
        if Plot:
            graph=plotSpectrum([root,outWS_n],0, 1)
            graph.activeLayer().setTitle(title)
        if Save:
            SaveNexusProcessed(outWS_n, outWS_n+'.nxs', Title=title)
    return output

def plotFury(inWS_n, spec):
    inWS = mtd[inWS_n[0]]
    nbins = inWS.getNumberBins()
    graph = plotSpectrum(inWS_n, spec)
    layer = graph.activeLayer()
    layer.setScale(Layer.Left, 0, 1.0)

def plotInput(inputfiles,spectra=[]):
    OneSpectra = False
    if len(spectra) != 2:
        spectra = [spectra[0], spectra[0]]
        OneSpectra = True
    workspaces = []
    for file in inputfiles:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        LoadNexusProcessed(file, root)
        if not OneSpectra:
            GroupDetectors(root, root,
                DetectorList=range(spectra[0],spectra[1]+1) )
        workspaces.append(root)
    if len(workspaces) > 0:
        graph = plotSpectrum(workspaces,0)
        layer = graph.activeLayer().setTitle(", ".join(workspaces))
        
###############################################################################
## abscor (previously in SpencerAnalysis) #####################################
###############################################################################

def CubicFit(inputWS, spec):
    '''Uses the Mantid Fit Algorithm to fit a cubic function to the inputWS
    parameter. Returns a list containing the fitted parameter values.'''
    function = 'name=UserFunction, Formula=A0+A1*x+A2*x*x, A0=1, A1=0, A2=0'
    fit = Fit(inputWS, spec, Function=function)
    return fit.getPropertyValue('Parameters')

def applyCorrections(inputWS, cannisterWS, corrections, efixed):
    '''Through the PolynomialCorrection algorithm, makes corrections to the
    input workspace based on the supplied correction values.'''
    # Corrections are applied in Lambda (Wavelength)
    ConvertUnits(inputWS, inputWS, 'Wavelength', 'Indirect',
        EFixed=efixed)
    if cannisterWS != '':
        ConvertUnits(cannisterWS, cannisterWS, 'Wavelength', 'Indirect',
            EFixed=efixed)
    nHist = mtd[inputWS].getNumberHistograms()
    # Check that number of histograms in each corrections workspace matches
    # that of the input (sample) workspace
    for ws in corrections:
        if ( mtd[ws].getNumberHistograms() != nHist ):
            raise ValueError('Mismatch: num of spectra in '+ws+' and inputWS')
    # Workspaces that hold intermediate results
    CorrectedWorkspace = getWSprefix(inputWS) + 'csam'
    CorrectedSampleWorkspace = '__csamws'
    CorrectedCanWorkspace = getWSprefix(cannisterWS) + 'ccan'
    for i in range(0, nHist): # Loop through each spectra in the inputWS
        ExtractSingleSpectrum(inputWS, CorrectedSampleWorkspace, i)
        if ( len(corrections) == 1 ):
            Ass = CubicFit(corrections[0], i)
            PolynomialCorrection(CorrectedSampleWorkspace, 
                CorrectedSampleWorkspace, Ass, 'Divide')
            if ( i == 0 ):
                CloneWorkspace(CorrectedSampleWorkspace, CorrectedWorkspace)
            else:
                ConjoinWorkspaces(CorrectedWorkspace, CorrectedSampleWorkspace)
        else:
            ExtractSingleSpectrum(cannisterWS, CorrectedCanWorkspace, i)
            Acc = CubicFit(corrections[3], i)
            PolynomialCorrection(CorrectedCanWorkspace, CorrectedCanWorkspace,
                Acc, 'Divide')
            Acsc = CubicFit(corrections[2], i)
            PolynomialCorrection(CorrectedCanWorkspace, CorrectedCanWorkspace,
                Acsc, 'Multiply')
            Minus(CorrectedSampleWorkspace, CorrectedCanWorkspace,
                CorrectedSampleWorkspace)
            Assc = CubicFit(corrections[1], i)
            PolynomialCorrection(CorrectedSampleWorkspace, 
                CorrectedSampleWorkspace, Assc, 'Divide')
            if ( i == 0 ):
                CloneWorkspace(CorrectedSampleWorkspace, CorrectedWorkspace)
            else:
                ConjoinWorkspaces(CorrectedWorkspace, CorrectedSampleWorkspace)
    ConvertUnits(CorrectedWorkspace, CorrectedWorkspace, 'DeltaE', 'Indirect',
        EFixed=efixed)
    if cannisterWS != '':
        ConvertUnits(CorrectedCanWorkspace, CorrectedCanWorkspace, 'DeltaE',
            'Indirect', EFixed=efixed)
                
def abscorFeeder(sample, container, geom, useCor):
    '''Load up the necessary files and then passes them into the main
    applyCorrections routine.'''
    if useCor:
        ## Files named: (ins)(runNo)_(geom)_(suffix)
        ins = mtd[sample].getInstrument().getName()
        ins = ConfigService().facility().instrument(ins).shortName().lower()
        run = mtd[sample].getRun().getLogData('run_number').value
        name = ins + run + '_' + geom + '_'
        corrections = [loadNexus(name+'ass.nxs')]
        if container != '': # if container is given we have 3 more corrections
            corrections.append(loadNexus(name+'assc.nxs'))
            corrections.append(loadNexus(name+'acsc.nxs'))
            corrections.append(loadNexus(name+'acc.nxs'))
    else:
        if ( container == '' ):
            sys.exit("What do you want me to do?")
        else:
            result = getWSprefix(sample) + 'bgd'
            Minus(sample, container, result)
            return
    # Get efixed
    efixed = getEfixed(sample)
    # Fire off main routine
    try:
        applyCorrections(sample, container, corrections, efixed)
    except ValueError:
        print """Number of histograms in corrections workspaces do not match
            the sample workspace."""
        raise

###############################################################################        
## utility functions - can probably be moved elsewhere to share better ########
###############################################################################

def loadNexus(filename):
    '''Loads a Nexus file into a workspace with the name based on the
    filename. Convenience function for not having to play around with paths
    in every function.'''
    name = os.path.splitext( os.path.split(filename)[1] )[0]
    LoadNexus(filename, name)
    return name
    
def getWSprefix(workspace):
    '''Returns a string of the form '<ins><run>_<analyser><refl>_' on which
    all of our other naming conventions are built.'''
    if workspace == '':
        return ''
    ws = mtd[workspace]
    ins = ws.getInstrument().getName()
    ins = ConfigService().facility().instrument(ins).shortName().lower()
    run = ws.getRun().getLogData('run_number').value
    try:
        analyser = ws.getInstrument().getStringParameter('analyser')[0]
        reflection = ws.getInstrument().getStringParameter('reflection')[0]
    except IndexError:
        analyser = ''
        reflection = ''
    prefix = ins + run + '_' + analyser + reflection + '_'
    return prefix

def getEfixed(workspace, detIndex=0):
    det = mtd[workspace].getDetector(detIndex)
    try:
        efixed = det.getNumberParameter('Efixed')[0]
    except AttributeError:
        ids = det.getDetectorIDs()
        det = mtd[workspace].getInstrument().getDetector(ids[0])
        efixed = det.getNumberParameter('Efixed')[0]
    return efixed

def createQaxis(inputWS):
    result = []
    ws = mtd[inputWS]
    nHist = ws.getNumberHistograms()
    if ws.getAxis(1).isSpectra():
        inst = ws.getInstrument()
        samplePos = inst.getSample().getPos()
        beamPos = samplePos - inst.getSource().getPos()
        for i in range(0,nHist):
            efixed = getEfixed(inputWS, i)
            detector = ws.getDetector(i)
            theta = detector.getTwoTheta(samplePos, beamPos) / 2
            lamda = math.sqrt(81.787/efixed)
            q = 4 * math.pi * math.sin(theta) / lamda
            result.append(q)
    else:
        axis = ws.getAxis(1)
        msg = 'Creating Axis based on Detector Q value: '
        if not axis.isNumeric():
            msg += 'Input workspace must have either spectra or numeric axis.'
            print msg
            sys.exit(msg)
        if ( axis.getUnit().name() != 'MomentumTransfer' ):
            msg += 'Input must have axis values of Q'
            print msg
            sys.exit(msg)
        for i in range(0, nHist):
            result.append(float(axis.label(i)))
    return result
