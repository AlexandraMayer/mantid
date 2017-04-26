import xml.dom.minidom

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from ruamel.yaml.comments import CommentedMap
import os
import re

from reduction_gui.reduction.scripter import BaseScriptElement, BaseReductionScripter


class DNSScriptElement(BaseScriptElement):

    NORM_TIME     = 0
    NORM_MONITOR  = 1

    OUT_POLY_AMOR     = 0
    OUT_SINGLE_CRYST  = 1

    SEP_XYZ = 0
    SEP_COH = 1
    SEP_NO  = 2

    DEF_SaveToFile = False

    DEF_DetEffi      = True
    DEF_SumVan       = False
    DEF_SubInst      = True
    DEF_SubFac       = 1.0
    DEF_FlippRatio   = True
    DEF_FlippFac     = 1.0
    DEF_MultiSF      = 0.0
    DEF_Normalise    = NORM_TIME
    DEF_NeutWaveLen  = 0.0
    DEF_Intermadiate = False

    DEF_Output = OUT_POLY_AMOR

    DEF_OutAxisQ      = True
    DEF_OutAxisD      = True
    DEF_OutAxis2Theta = True
    DEF_Seperation    = SEP_XYZ

    DEF_OmegaOffset  = 0.0
    DEF_LatticeA     = 0.0
    DEF_LatticeB     = 0.0
    DEF_LatticeC     = 0.0
    DEF_LatticeAlpha = 0.0
    DEF_LatticeBeta  = 0.0
    DEF_LatticeGamma = 0.0
    DEF_ScatterU1    = 0.0
    DEF_ScatterU2    = 0.0
    DEF_ScatterU3    = 0.0
    DEF_ScatterV1    = 0.0
    DEF_ScatterV2    = 0.0
    DEF_ScatterV3    = 0.0

    XML_TAG = 'DNSReduction'

    def reset(self):

        self.facility_name   = ''
        self.instrument_name = ''

        self.sampleDataPath = ''
        self.filePrefix     = ''
        self.fileSuffix     = ''

        self.dataRuns = []

        self.maskAngles = []

        self.saveToFile = self.DEF_SaveToFile
        self.outDir = ''
        self.outPrefix = ''

        self.standardDataPath = ''

        self.detEffi        = self.DEF_DetEffi
        self.sumVan         = self.DEF_SumVan
        self.subInst        = self.DEF_SubInst
        self.subFac         = self.DEF_SubFac
        self.flippRatio     = self.DEF_FlippRatio
        self.flippFac       = self.DEF_FlippFac
        self.multiSF        = self.DEF_MultiSF
        self.normalise      = self.DEF_Normalise
        self.neutronWaveLen = self.DEF_NeutWaveLen
        self.intermadiate   = self.DEF_Intermadiate

        self.out = self.DEF_Output

        self.outAxisQ      = self.DEF_OutAxisQ
        self.outAxisD      = self.DEF_OutAxisD
        self.outAxis2Theta = self.DEF_OutAxis2Theta
        self.seperation    = self.DEF_Seperation

        self.omegaOffset    = self.DEF_OmegaOffset
        self.latticeA       = self.DEF_LatticeA
        self.latticeB       = self.DEF_LatticeB
        self.latticeC       = self.DEF_LatticeC
        self.latticeAlpha   = self.DEF_LatticeAlpha
        self.latticeBeta    = self.DEF_LatticeBeta
        self.latticeGamma   = self.DEF_LatticeGamma
        self.scatterU1      = self.DEF_ScatterU1
        self.scatterU2      = self.DEF_ScatterU2
        self.scatterU3      = self.DEF_ScatterU3
        self.scatterV1      = self.DEF_ScatterV1
        self.scatterV2      = self.DEF_ScatterV2
        self.scatterV3      = self.DEF_ScatterV3

    def to_xml(self):

        res =['']

        def put(tag, val):
            res[0] += ' <{0}>{1}</{0}>\n'.format(tag, str(val))

        put('sample_data_path', self.sampleDataPath)
        put('file_prefix',      self.filePrefix)
        put('file_suffix',      self.fileSuffix)

        for (runs, ws, cmnt) in self.dataRuns:
            put('data_runs',             runs)
            put('data_output_worksoace', ws)
            put('data_comment',          cmnt)

        for(min, max) in self.maskAngles:
            put('mask_min_Angle', min)
            put('mask_max_Angle', max)

        put('save_to_file',       self.saveToFile)
        put('output_directory',   self.outDir)
        put('output_file_prefix', self.outPrefix)

        put('standard_data_path', self.standardDataPath)

        put('detector_efficiency',        self.detEffi)
        put('sum_Vanadium',               self.sumVan)
        put('subtract_instrument',        self.subInst)
        put('subtract_instrument_factor', self.subFac)
        put('flipping_ratio',             self.flippRatio)
        put('flipping_ratio_factor',      self.flippFac)
        put('multiple_SF_scattering',     self.multiSF)
        put('normalise',                  self.normalise)
        put('neutron_wave_length',        self.neutronWaveLen)
        put('keep_intermediate',          self.intermadiate)

        put('output',             self.out)

        put('output_Axis_q',      self.outAxisQ)
        put('output_Axis_d',      self.outAxisD)
        put('output_Axis_2Theta', self.outAxis2Theta)
        put('seperation',         self.seperation)

        put('lattice_parameters_a',     self.latticeA)
        put('lattice_parameters_b',     self.latticeB)
        put('lattice_patameters_c',     self.latticeC)
        put('lattice_patameters_alpha', self.latticeAlpha)
        put('lattice_parameters_beta',  self.latticeBeta)
        put('lattice_parameters_gamma', self.latticeGamma)
        put('scattering_Plane_u_1',     self.scatterU1)
        put('scattering_Plane_u_2',     self.scatterU2)
        put('scattering_Plane_u_3',     self.scatterU3)
        put('scattering_Plane_v_1',     self.scatterV1)
        put('scattering_Plane_v_2',     self.scatterV2)
        put('scattering_Plane_v_3',     self.scatterV3)

        return '<{0}>\n{1}</{0}>\n'.format(self.XML_TAG, res[0])

    def from_xml(self, xmlStr):

        self.reset()

        dom = xml.dom.minidom.parseString(xmlStr)
        els = dom.getElementsByTagName(self.XML_TAG)

        if els:

            dom = els[0]

            def get_str(tag, default=''):
                return BaseScriptElement.getStringElement(dom, tag, default=default)

            def get_int(tag,default):
                return BaseScriptElement.getIntElement(dom, tag, default=default)

            def get_flt(tag, default):
                return BaseScriptElement.getFloatElement(dom, tag, default=default)

            def get_strlst(tag):
                return BaseScriptElement.getStringList(dom, tag)

            def get_bol(tag,default):
                return BaseScriptElement.getBoolElement(dom, tag, default=default)

            self.sampleDataPath = get_str('sample_data_path')
            self.filePrefix     = get_str('file_prefix')
            self.fileSuffix     = get_str('file_suffix')

            dataRuns  = get_strlst('data_runs')
            dataOutWs = get_strlst('data_output_worksoace')
            dataCmts  = get_strlst('data_comment')

            for i in range(min(len(dataRuns), len(dataOutWs), len(dataCmts))):
                self.dataRuns.append((dataRuns[i], dataOutWs[i], dataCmts[i]))

            maskMin = get_strlst('mask_min_Angle')
            maskMax = get_strlst('mask_max_Angle')

            for i in range(min(len(maskMin), len(maskMax))):
                self.maskAngles.append((maskMin[i], maskMax[i]))

            self.saveToFile = get_bol('save_to_file', self.DEF_SaveToFile)
            self.outDir     = get_str('output_directory')
            self.outPrefix  = get_str('output_file_prefix')

            self.standardDataPath = get_str('standard_data_path')

            self.detEffi        = get_bol('detector_efficiency',        self.DEF_DetEffi)
            self.sumVan         = get_bol('sum_Vanadium',               self.DEF_SumVan)
            self.subInst        = get_bol('subtract_instrument',        self.DEF_SubInst)
            self.subFac         = get_flt('subtract_instrument_factor', self.subFac)
            self.flippRatio     = get_bol('flipping_ratio',             self.DEF_FlippRatio)
            self.multiSF        = get_flt('multiple_SF_scattering',     self.DEF_MultiSF)
            self.normalise      = get_int('normalise',                  self.DEF_Normalise)
            self.neutronWaveLen = get_flt('neutron_wave_length',        self.DEF_NeutWaveLen)
            self.intermadiate   = get_bol('keep_intermediate',          self.DEF_Intermadiate)

            self.out        = get_int('output', self.DEF_Output)

            self.outAxisQ      = get_bol('output_Axis_q',      self.DEF_OutAxisQ)
            self.outAxisD      = get_bol('output_Axis_d',      self.DEF_OutAxisD)
            self.outAxis2Theta = get_bol('output_Axis_2Theta', self.DEF_OutAxis2Theta)
            self.seperation    = get_int('seperation',         self.DEF_Seperation)

            self.latticeA     = get_flt('lattice_parameters_a',     self.DEF_LatticeA)
            self.latticeB     = get_flt('lattice_parameters_b',     self.DEF_LatticeB)
            self.latticeC     = get_flt('lattice_parameters_c',     self.DEF_LatticeC)
            self.latticeAlpha = get_flt('lattice_parameters_alpha', self.DEF_LatticeAlpha)
            self.latticeBeta  = get_flt('lattice_parameters_beta',  self.DEF_LatticeBeta)
            self.latticeGamma = get_flt('lattice_parameters_gamma', self.DEF_LatticeGamma)
            self.scatterU1    = get_flt('scattering_Plane_u_1',     self.DEF_ScatterU1)
            self.scatterU2    = get_flt('scattering_Plane_u_2',     self.DEF_ScatterU2)
            self.scatterU3    = get_flt('scattering_Plane_u_3',     self.DEF_ScatterU3)
            self.scatterV1    = get_flt('scattering_Plane_v_1',     self.DEF_ScatterV1)
            self.scatterV2    = get_flt('scattering_Plane_v_2',     self.DEF_ScatterV2)
            self.scatterV3    = get_flt('scattering_Plane_v_3',     self.DEF_ScatterV3)



    def to_script(self):

        def _searchFile(path, name):
            files = os.listdir(path)
            for file in files:
                if file.__contains__(name):
                    return True
            return False

        def _searchFiles(path, prefix, suffix, dataRun):
            fs = []
            for runs in dataRun:
                (runNums, outWs, comment) = runs
                numbers = runNums.split(',')
                for number in numbers:
                    if number.__contains__(':'):
                        (first, last) = number.split(':')
                        first = int(first)
                        last = int(last)
                        for i in range(first, last + 1):
                            files = [f for f in os.listdir(path) if
                                     re.match(r"{}0*{}{}.d_dat".format(prefix, int(i), suffix), f)]
                            fs.append(files)
                            if not files:
                                error('file with prefix ' + prefix + ', run number '
                                      + str(i) + ' and suffix ' + suffix + ' not found')
                    else:
                        files = [f for f in os.listdir(path) if
                                 re.match(r"{}0*{}{}.d_dat".format(prefix, int(number), suffix), f)]
                        fs.append(files)
                        if not files:
                            error('file with prefix ' + prefix + ', run number '
                                  + str(number) + ' and suffix ' + suffix + ' not found')
            return fs

        def error(message):
            raise RuntimeError('DNS reduction error: ' + message)

        files = None

        if not os.path.lexists(self.sampleDataPath):
            error('sample data path not found')

        if not self.filePrefix:
            error('missing sample data file prefix')

        if not self.fileSuffix:
            error('missing sample data file suffix')

        if not self.dataRuns:
            error('missing data runs')
        else:
            files = _searchFiles(self.sampleDataPath, self.filePrefix, self.fileSuffix, self.dataRuns )

        for i in range(len(self.maskAngles)):
            (minA, maxA) = self.maskAngles[i]
            if not float(minA) < float(maxA):
                errormess = 'Min Angle must be smaller than max Angle (error in row ' + str(i+1) + ')'
                error(errormess)

        if self.saveToFile:
            if not os.path.lexists(self.outDir):
                error('output directory not found')
            elif not os.access(self.outDir, os.W_OK):
                error('cant write in directory ' + str(self.outDir))

            if not self.outPrefix:
                error('missing output file prefix')

        if not os.path.lexists(self.standardDataPath):
            error('standard data path not found')
        else:
            if self.detEffi:
                found = _searchFile(self.standardDataPath, 'vana')
                if not found:
                    error('no file for detector efficiency correction in '
                          + str(self.standardDataPath) + ' found')
            if self.subInst:
                found = _searchFile(self.standardDataPath, 'leer')
                if not found:
                    error('no file to substract of instrument background for sample in '
                          + str(self.standardDataPath) + ' found')
            if self.flippRatio:
                found = _searchFile(self.standardDataPath, 'nicr')
                if not found:
                    error('no file for flipping ratio correction in '
                          + str(self.standardDataPath) + ' found')

        if self.out == self.OUT_POLY_AMOR and not self.outAxisQ and not self.outAxisD and not self.outAxis2Theta:
            error('no abscissa selected')

        parameters = CommentedMap()

        sampleData = CommentedMap()

        sampleData['Data path']   = self.sampleDataPath
        sampleData['File prefix'] = self.filePrefix
        sampleData['File suffix'] = self.fileSuffix
        runs = CommentedMap()
        for i in range(len(self.dataRuns)):
            run = CommentedMap()
            (runNumber, outWs, comment) = self.dataRuns[i]
            run["Run numbers"] = runNumber
            run["Output Workspace"] = outWs
            run["Comment"] = comment
            string = "Runs in row: " + str(i)
            runs[string] = run

        if files:
            runs['files'] = files
        sampleData['Data Table']  = runs

        parameters['Sample Data'] = sampleData

        maskDet = CommentedMap()
        for i in range(len(self.maskAngles)):
            mask = CommentedMap()
            (minA, maxA) = self.maskAngles[i]
            mask['Min Angle'] = minA
            mask['Max Angle'] = maxA
            string = "Angles row: " + str(i)
            maskDet[string] = mask

        parameters['Mask Detectors'] = maskDet

        saveToFile = CommentedMap()

        saveToFile['Save to file?']       = str(self.saveToFile)
        if self.saveToFile:
         saveToFile['Output directory']   = self.outDir
         saveToFile['Output file prefix'] = self.outPrefix

        parameters['Save'] = saveToFile

        stdData = CommentedMap()

        stdData['Path'] = self.standardDataPath

        parameters['Standard Data'] = stdData

        datRedSettings = CommentedMap()

        datRedSettings['Detector efficiency correction']             =  str(self.detEffi)
        datRedSettings['Sum Vandium over detector position']         = str(self.sumVan)
        datRedSettings['Substract instrument background for sample'] = str(self.subInst)
        if self.subInst:
            datRedSettings['Substract factor']                       = self.subFac
        datRedSettings['Flipping ratio correction']                  = str(self.flippRatio)
        if self.flippRatio:
            datRedSettings['Flipping ratio factor']                  = self.flippFac
        datRedSettings['Multiple SF scattering probability']         = self.multiSF
        if self.normalise == self.NORM_MONITOR:
            norm = 'monitor'
        else:
            norm = 'time'
        datRedSettings['Normalization']                              = norm
        datRedSettings['Neutron wavelength']                         = self.neutronWaveLen
        datRedSettings['Keep intermediat workspaces']                = str(self.intermadiate)

        parameters['Data reduction settings'] = datRedSettings

        type = CommentedMap()

        if self.out == self.OUT_POLY_AMOR:
            type['Type'] = 'Polycrystal/Amorphous'
            outAx = ''
            if self.outAxisQ:
                outAx += 'q, '

            if self.outAxisD:
                outAx += 'd, '

            if self.outAxis2Theta:
                outAx += '2Theta'

            type['Abscissa'] = outAx
            if self.seperation == self.SEP_XYZ:
                type['Seperation'] = 'XYZ'
            elif self.seperation == self.SEP_COH:
                type['Seperation'] = 'Coherent/Incoherent'
            else:
                type['Seperation'] = 'No'

        if self.out == self.OUT_SINGLE_CRYST:
            type['Type'] = 'Single Crystal'

            type['Omega offset'] = self.omegaOffset

            lattice = {}
            lattice['a'] = self.latticeA
            lattice['b'] = self.latticeB
            lattice['c'] = self.latticeC
            lattice['alpha'] = self.latticeAlpha
            lattice['beta'] = self.latticeBeta
            lattice['gamma'] = self.latticeGamma

            type['Lattice parameters'] = lattice

            scatter = {}
            u = []
            u.append(self.scatterU1)
            u.append(self.scatterU2)
            u.append(self.scatterU3)
            v = []
            v.append(self.scatterV1)
            v.append(self.scatterV2)
            v.append(self.scatterV3)
            scatter['u'] = u
            scatter['v'] = v

            type['Scattering Plane'] = scatter

        parameters['Sample'] = type




        print parameters

        script = ['']

        def l(line = ''):
            script[0] += line + '\n'

        l("import numpy as np")
        l()

        return script[0]




class DNSReductionScripter(BaseReductionScripter):

    def __init__(self, name, facility):
        BaseReductionScripter.__init__(self, name, facility)