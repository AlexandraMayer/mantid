from mantidsimple import *
from mantidplot import *
from IndirectCommon import *

import math, re, os.path

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

def split(l, n):
    #Yield successive n-sized chunks from l.
    for i in xrange(0, len(l), n):
        yield l[i:i+n]

def segment(l, fromIndex, toIndex):
    for i in xrange(fromIndex, toIndex + 1):
        yield l[i]

def trimData(nSpec, vals, min, max):
    result = []
    chunkSize = len(vals) / nSpec
    assert min >= 0, 'trimData: min is less then zero'
    assert max <= chunkSize - 1, 'trimData: max is greater than the number of spectra'
    assert min <= max, 'trimData: min is greater than max'
    chunks = split(vals,chunkSize)
    for chunk in chunks:
        seg = segment(chunk,min,max)
        for val in seg:
            result.append(val)
    return result

def confitParsToWS(Table, Data, BackG='FixF', specMin=0, specMax=-1):
    if ( specMax == -1 ):
        specMax = mtd[Data].getNumberHistograms() - 1
    dataX = createQaxis(Data)
    xAxisVals = []
    xAxisTrimmed = []
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
    xAxisTrimmed = trimData(nSpec, xAxisVals, specMin, specMax)
    CreateWorkspace(outNm, xAxisTrimmed, dataY, dataE, nSpec,
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

def elwin(inputFiles, eRange, Save=False, Verbose=False, Plot=False):
    eq1 = [] # output workspaces with units in Q
    eq2 = [] # output workspaces with units in Q^2
    for file in inputFiles:
        root = loadNexus(file)
        savefile = getWSprefix(root)
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
        DeleteWorkspace(root)
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
        DeleteWorkspace('sam_data')
        DeleteWorkspace('sam_int')
        DeleteWorkspace('sam_fft')
        DeleteWorkspace('sam')
        # Crop nonsense values off workspace
        bin = int(math.ceil(mtd[savefile].getNumberBins()/ 2.0))
        binV = mtd[savefile].dataX(0)[bin]
        CropWorkspace(savefile, savefile, XMax=binV)
        if Save:
            SaveNexusProcessed(savefile, savefile+'.nxs')
    # Clean Up RES files
    DeleteWorkspace('res_data')
    DeleteWorkspace('res_int')
    DeleteWorkspace('res_fft')
    DeleteWorkspace('res')
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
        if ( re.search('Intensity$', yAxis) or re.search('Tau$', yAxis) or
            re.search('Beta$', yAxis) ):
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

def msdfit(inputs, startX, endX, Save=False, Verbose=True, Plot=False):
    verbOp = 'True'
    if verbOp:
        mtd.sendLogMessage('Starting MSDfit')
    workdir = mantid.getConfigProperty('defaultsave.directory')
    log_type = 'sample'
    output = []
    DataX = []
    DataY = []
    DataE = []
    np = 0
    runs = sorted(inputs)
    for file in runs:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        LoadNexusProcessed(file, root)
        inX = mtd[root].readX(0)
        inY = mtd[root].readY(0)
        inE = mtd[root].readE(0)
        logy =[]
        loge =[]
        for i in range(0, len(inY)):
            if(inY[i] == 0):
                ly = math.log(0.000000000001)
            else:
                ly = math.log(inY[i])
            logy.append(ly)
            if( inY[i]+inE[i] == 0 ):
                le = math.log(0.000000000001)-ly
            else:
                le = math.log(inY[i]+inE[i])-ly
            loge.append(le)
        lnWS = root[:-3] + 'lnI'
        CreateWorkspace(lnWS, inX, logy, loge, 1)
        log_name = root[0:8]+'_'+log_type
        log_file = log_name+'.txt'
        log_path = FileFinder.getFullPath(log_file)
        if (log_path == ''):
            mtd.sendLogMessage('Run : '+root[0:8] +' ; Temperature file not found')
        else:			
            mtd.sendLogMessage('Found '+log_path)
            LoadLog(root,log_path)
            run_logs = mtd[root].getRun()
            tmp = run_logs[log_name].value
            temp = tmp[len(tmp)-1]
            mtd.sendLogMessage('Run : '+root[0:8] +' ; Temperature = '+str(temp))
        outWS = root[:-3] + 'msd_' + str(np)
        function = 'name=LinearBackground, A0=0, A1=0'
        fit_alg = Fit(lnWS, 0, startX, endX, function, Output=outWS)
        output.append(outWS)
        params = mtd[outWS+'_Parameters'] #  get a TableWorkspace with the parameters and errors
        A0 = params.getDouble('Value',0) # get the value of the first parameter
        A0_Err = params.getDouble('Error',0) # get the error of the first parameter
        A1 = params.getDouble('Value',1) # get the value of the second parameter
        A1_Err = params.getDouble('Error',1) # get the error of the second parameter
        title = 'Intercept: '+str(A0)+' ; Slope: '+str(A1)
        fit = 'Intercept: '+str(A0)+' +- '+str(A0_Err)+' ; Slope: '+str(A1)+' +- '+str(A1_Err)
        if verbOp:
            mtd.sendLogMessage(fit)
        if (log_path == ''):
            DataX.append(int(root[3:8]))
            xlabel = 'Run number'
        else:
            DataX.append(temp)
            xlabel = 'Temperature (K)'
        DataY.append(-float(A1)*3.0)
        DataE.append(A1_Err*3.0)
        np += 1
        if Plot:
            data_plot=plotSpectrum(lnWS,0, True)
            data_plot.activeLayer().setTitle(title)
            data_plot.activeLayer().setAxisTitle(Layer.Bottom,'Q^2')
            data_plot.activeLayer().setAxisTitle(Layer.Left,'ln(intensity)')
            fit_plot=plotSpectrum(outWS+'_Workspace',(0,1),True)
            mergePlots(data_plot,fit_plot)
    fitWS = root[0:8]+'_MsdFit'
    DataX.append(2*DataX[np-1]-DataX[np-2])
    CreateWorkspace(fitWS,DataX,DataY,DataE,1)
    if Plot:
        msd_plot=plotSpectrum(fitWS,0,True)
        msd_plot.activeLayer().setAxisTitle(Layer.Bottom,xlabel)
        msd_plot.activeLayer().setAxisTitle(Layer.Left,'<u2>')
    if Save:
        fit_path = os.path.join(workdir, fitWS+'.nxs')					# path name for nxs file
        SaveNexusProcessed(fitWS, fit_path, Title=fitWS)
        if verbOp:
            mtd.sendLogMessage('Output file : '+fit_path)  
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
        root = loadNexus(file)
        if not OneSpectra:
            GroupDetectors(root, root,
                DetectorList=range(spectra[0],spectra[1]+1) )
        workspaces.append(root)
    if len(workspaces) > 0:
        graph = plotSpectrum(workspaces,0)
        layer = graph.activeLayer().setTitle(", ".join(workspaces))
        
###############################################################################
## abscor #####################################################################
###############################################################################

def CubicFit(inputWS, spec, verbose=False):
    '''Uses the Mantid Fit Algorithm to fit a quadratic to the inputWS
    parameter. Returns a list containing the fitted parameter values.'''
    function = 'name=Quadratic, A0=1, A1=0, A2=0'
    fit = Fit(inputWS, spec, Function=function)
    Abs = fit.getPropertyValue('Parameters')
    if verbose:
        mtd.sendLogMessage('Group '+str(spec)+' of '+inputWS+' ; fit coefficients are : '+Abs)
    return Abs

def applyCorrections(inputWS, canWS, corr, verbose=False):
    '''Through the PolynomialCorrection algorithm, makes corrections to the
    input workspace based on the supplied correction values.'''
    # Corrections are applied in Lambda (Wavelength)
    efixed = getEfixed(inputWS)                # Get efixed
    ConvertUnits(inputWS, inputWS, 'Wavelength', 'Indirect', EFixed=efixed)
    corrections = [corr[0]+'_1']
    CorrectedWS = inputWS[0:-3] +'Corrected'
    if canWS != '':
        corrections = [corr[0]+'_1', corr[0]+'_2', corr[0]+'_3', corr[0]+'_4']
        CorrectedWS = inputWS[0:-3] +'Correct_'+ canWS[3:8]
        ConvertUnits(canWS, canWS, 'Wavelength', 'Indirect', EFixed=efixed)
    nHist = mtd[inputWS].getNumberHistograms()
    # Check that number of histograms in each corrections workspace matches
    # that of the input (sample) workspace
    for ws in corrections:
        if ( mtd[ws].getNumberHistograms() != nHist ):
            raise ValueError('Mismatch: num of spectra in '+ws+' and inputWS')
    # Workspaces that hold intermediate results
    CorrectedSampleWS = '__csam'
    CorrectedCanWS = '__ccan'
    for i in range(0, nHist): # Loop through each spectra in the inputWS
        ExtractSingleSpectrum(inputWS, CorrectedSampleWS, i)
        if ( len(corrections) == 1 ):
            Ass = CubicFit(corrections[0], i, verbose)
            PolynomialCorrection(CorrectedSampleWS, CorrectedSampleWS, Ass, 'Divide')
            if ( i == 0 ):
                CloneWorkspace(InputWorkspace1=CorrectedSampleWS, InputWorkspace2=CorrectedWS, OutputWorkspace=CorrectedSampleWS)
            else:
                ConjoinWorkspaces(InputWorkspace1=CorrectedWS, InputWorkspace2=CorrectedSampleWS, OutputWorkspace=CorrectedWS)
        else:
            ExtractSingleSpectrum(canWS, CorrectedCanWS, i)
            Acc = CubicFit(corrections[3], i, verbose)
            PolynomialCorrection(CorrectedCanWS, CorrectedCanWS, Acc, 'Divide')
            Acsc = CubicFit(corrections[2], i, verbose)
            PolynomialCorrection(CorrectedCanWS, CorrectedCanWS, Acsc, 'Multiply')
            Minus(CorrectedSampleWS, CorrectedCanWS, CorrectedSampleWS)
            Assc = CubicFit(corrections[1], i, verbose)
            PolynomialCorrection(CorrectedSampleWS, CorrectedSampleWS, Assc, 'Divide')
            if ( i == 0 ):
                CloneWorkspace(CorrectedSampleWS, CorrectedWS)
            else:
                ConjoinWorkspaces(CorrectedWS, CorrectedSampleWS)
    ConvertUnits(inputWS, inputWS, 'DeltaE', 'Indirect', EFixed=efixed)
    ConvertUnits(CorrectedWS, CorrectedWS, 'DeltaE', 'Indirect', EFixed=efixed)
    if canWS != '':
        mantid.deleteWorkspace(CorrectedCanWS)
        ConvertUnits(canWS, canWS, 'DeltaE', 'Indirect', EFixed=efixed)
    return CorrectedWS
                
def abscorFeeder(sample, container, geom, useCor):
    '''Load up the necessary files and then passes them into the main
    applyCorrections routine.'''
    verbOp = True
    Plot = True
    workdir = mantid.getConfigProperty('defaultsave.directory')
    s_hist = mtd[sample].getNumberHistograms()       # no. of hist/groups in sam
    Xin = mtd[sample].readX(0)
    sxlen = len(Xin)
    if container != '':
        c_hist = mtd[container].getNumberHistograms()
        Xin = mtd[container].readX(0)
        cxlen = len(Xin)
        if s_hist != c_hist:	# check that no. groups are the same
            error = 'Can histograms (' +str(c_hist) + ') not = Sample (' +str(s_hist) +')'	
            exit(error)
        else:
            if sxlen != cxlen:	# check that array lengths are the same
                error = 'Can array length (' +str(cxlen) + ') not = Sample (' +str(sxlen) +')'	
                exit(error)
    if useCor:
        if verbOp:
            mtd.sendLogMessage('Correcting sample ' + sample + ' with ' + container)
        file = sample[:-3] + geom +'_Abs.nxs'
        abs_path = os.path.join(workdir, file)					# path name for nxs file
        if verbOp:
            mtd.sendLogMessage('Correction file :'+abs_path)
        corrections = [loadNexus(abs_path)]
        cor_result = applyCorrections(sample, container, corrections, verbOp)
        cor_path = os.path.join(workdir,cor_result+'.nxs')
        SaveNexusProcessed(cor_result,cor_path)
        plot_list = [cor_result,sample]
        if ( container != '' ):
            plot_list.append(container)
        if verbOp:
            mtd.sendLogMessage('Output file created : '+cor_path)
        if Plot:
           cor_plot=plotSpectrum(plot_list,0)
    else:
        if ( container == '' ):
            sys.exit('Invalid options - nothing to do!')
        else:
            sub_result = sample[0:8] +'_Subtract_'+ container[3:8]
            Minus(sample,container,sub_result)
            sub_path = os.path.join(workdir,sub_result+'.nxs')
            SaveNexusProcessed(sub_result,sub_path)
            if verbOp:
	            mtd.sendLogMessage('Subtracting '+container+' from '+sample)
	            mtd.sendLogMessage('Output file created : '+sub_path)
            if Plot:
                sub_plot=plotSpectrum([sub_result,sample,container],0)
    return
