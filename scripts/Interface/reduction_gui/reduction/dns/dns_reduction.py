import xml.dom.minidom

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from ruamel.yaml.comments import CommentedMap

from reduction_gui.reduction.scripter import BaseScriptElement, BaseReductionScripter


class DNSScriptElement(BaseScriptElement):

    NORM_DURATION = 0
    NORM_MONITOR  = 1

    OUT_SOFT_MAG     = 0
    OUT_SINGLE_CRYST = 1

    DEF_TwoTheata = 0.05
    DEF_normalise = NORM_DURATION

    DEF_NEUT_WAVE_LEN = 0.0

    DEF_MASK_MIN_ANGLE = 0.0
    DEF_MASK_MAX_ANGLE = 0.0

    DEF_output = OUT_SOFT_MAG

    DEF_OutAxisQ      = True
    DEF_OutAxisD      = False
    DEF_OutAxis2Theta = False

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

    DEF_DetEffi    = True
    DEF_SumVan     = False
    DEF_SubInst    = True
    DEF_FlippRatio = True

    DEF_MultiSF = 0.0

    DEF_Intermadiate = False

    #DEF_VANSuffix  = 'vana'
    #DEF_NiCrSuffix = 'nicr'
    #DEF_BackSuffix = 'leer'


    XML_TAG = 'DNSReducton'

    def reset(self):

        self.facility_name   = ''
        self.instrument_name = ''

        #self.currTable = ''
        self.twoTheta  = self.DEF_TwoTheata

        self.normalise = self.DEF_normalise

        self.neutronWaveLen = self.DEF_NEUT_WAVE_LEN

        self.maskMinAngle = self.DEF_MASK_MIN_ANGLE
        self.maskMaxAngle = self.DEF_MASK_MAX_ANGLE

        self.outDir = ''

        self.outPrefix = ''

        self.out = self.DEF_output

        self.outAxisQ      = self.DEF_OutAxisQ
        self.outAxisD      = self.DEF_OutAxisD
        self.outAxis2Theta = self.DEF_OutAxis2Theta

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

        self.detEffi    = self.DEF_DetEffi
        self.sumVan     = self.DEF_SumVan
        self.subInst    = self.DEF_SubInst
        self.flippRatio = self.DEF_FlippRatio

        self.multiSF = self.DEF_MultiSF

        self.intermadiate = self.DEF_Intermadiate

        self.standardDataPath = ''
        #self.VanSuffix        = self.DEF_VANSuffix
        #self.NiCrSuffix       = self.DEF_NiCrSuffix
        #self.backSuffix       = self.DEF_BackSuffix

        self.sampleDataPath = ''

        self.filePrefix = ''
        self.fileSuffix = ''

        self.dataRuns = []

    def to_xml(self):

        res =['']

        def put(tag, val):
            res[0] += ' <{0}>{1}</{0}>\n'.format(tag, str(val))

        #put('current_table',       self.currTable)
        put('two_Theta',           self.twoTheta)
        put('normalise',           self.normalise)
        put('neutron_wave_length', self.neutronWaveLen)

        put('mask_detector_min_angle', self.maskMinAngle)
        put('mask_detector_max_angle', self.maskMaxAngle)

        put('output_directory',   self.outDir)
        put('output_file_prefix', self.outPrefix)
        put('output',             self.out)

        put('output_Axis_q',      self.outAxisQ)
        put('output_Axis_d',      self.outAxisD)
        put('output_Axis_2Theta', self.outAxis2Theta)

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

        put('detector_efficiency',    self.detEffi)
        put('sum_Vanadium',           self.sumVan)
        put('subtract_instrument',    self.subInst)
        put('flipping_ratio',         self.flippRatio)
        put('multiple_SF_scattering', self.multiSF)
        put('keep_intermediate',      self.intermadiate)

        put('standard_data_path', self.standardDataPath)
        #put('Vanadium_suffix',    self.VanSuffix)
        #put('NiCr_suffix',        self.NiCrSuffix)
        #put('background_suffix',  self.backSuffix)

        put('sample_data_path', self.sampleDataPath)
        put('file_prefix',      self.filePrefix)
        put('file_suffix',      self.fileSuffix)

        for (runs, cmnt) in self.dataRuns:
            put('data_runs',    runs)
            put('data_comment', cmnt)

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

            #self.currTable      = get_str('current_table')
            self.twoTheta       = get_flt('two_Theta',           self.DEF_TwoTheata)
            self.normalise      = get_int('normalise',           self.DEF_normalise)
            self.neutronWaveLen = get_flt('neutron_wave_length', self.DEF_NEUT_WAVE_LEN)

            self.maskMinAngle = get_flt('mask_detector_min_angle', self.DEF_MASK_MIN_ANGLE)
            self.maskMaxAngle = get_flt('mask_detector_max_angle', self.DEF_MASK_MAX_ANGLE)

            self.outDir    = get_str('output_directory')
            self.outPrefix = get_str('output_file_prefix')
            self.out       = get_int('output', self.DEF_output)

            self.outAxisQ      = get_bol('output_Axis_q',      self.DEF_OutAxisQ)
            self.outAxisD      = get_bol('output_Axis_d',      self.DEF_OutAxisD)
            self.outAxis2Theta = get_bol('output_Axis_2Theta', self.DEF_OutAxis2Theta)

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

            self.detEffi      = get_bol('detector_efficiency',    self.DEF_DetEffi)
            self.sumVan       = get_bol('sum_Vanadium',           self.DEF_SumVan)
            self.subInst      = get_bol('subtract_instrument',    self.DEF_SubInst)
            self.flippRatio   = get_bol('flipping_ratio',         self.DEF_FlippRatio)
            self.multiSF      = get_flt('multiple_SF_scattering', self.DEF_MultiSF)
            self.intermadiate = get_bol('keep_intermediate',      self.DEF_Intermadiate)

            self.standardDataPath = get_str('standard_data_path')
            #self.VanSuffix        = get_str('Vanadium_suffix',   self.DEF_VANSuffix)
            #self.NiCrSuffix       = get_str('NiCr_suffix',       self.DEF_NiCrSuffix)
            #self.backSuffix       = get_str('background_suffix', self.DEF_BackSuffix)

            self.sampleDataPath = get_str('sample_data_path')
            self.filePrefix     = get_str('file_prefix')
            self.fileSuffix     = get_str('file_suffix')

            dataRuns = get_strlst('data_runs')
            dataCmts = get_strlst('data_comment')

            for i in range(min(len(dataRuns), len(dataCmts))):
                self.dataRuns.append((dataRuns[i], dataCmts[i]))


    def to_script(self):

        def error(message):
            raise RuntimeError('DNS reduction error: ' + message)

        if not(self.maskMinAngle <= self.maskMaxAngle):
            error('incorrect mask detector angles')

        if not self.dataRuns:
            error('missing data runs')

        parameters = CommentedMap()

        general = CommentedMap()

        #general["Current table path"] = self.currTable
        general['2 Theta tolerance'] = self.twoTheta
        if self.normalise == self.NORM_MONITOR:
            norm = 'monitor'
        else:
            norm = 'duration'
        general['Normalization'] = norm
        general['Neutron wavelength'] = self.neutronWaveLen

        parameters['General'] = general

        maskDet = CommentedMap()

        maskDet['Min Angle'] = self.maskMinAngle
        maskDet['Max Angle'] = self.maskMaxAngle

        parameters['Mask Detectors'] = maskDet

        output = CommentedMap()

        output['Output directory'] = self.outDir
        output['Output file prefix'] = self.outPrefix

        if self.out == self.OUT_SOFT_MAG:
            output['Output settings'] = 'Soft matter/Magnetic powder'
            outAx = ''
            if self.outAxisQ:
                outAx += 'q, '

            if self.outAxisD:
                outAx += 'd, '

            if self.outAxis2Theta:
                outAx += '2Theta'

            output['Output Axis'] = outAx

        if self.out == self.OUT_SINGLE_CRYST:
            output['Output settings'] = 'Single Crystal'

            output['Omega offset'] = self.omegaOffset

            lattice = {}
            lattice['a'] = self.latticeA
            lattice['b'] = self.latticeB
            lattice['c'] = self.latticeC
            lattice['alpha'] = self.latticeAlpha
            lattice['beta'] = self.latticeBeta
            lattice['gamma'] = self.latticeGamma

            output['Lattice parameters'] = lattice

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

            output['Scattering Plane'] = scatter

        parameters['Output'] = output

        datRedSettings = CommentedMap()

        datRedSettings['Detector efficiency correction'] =  str(self.detEffi)
        datRedSettings['Sum Vandium'] = str(self.sumVan)
        datRedSettings['Substract instrument background'] = str(self.subInst)
        datRedSettings['Flipping ratio correction'] = str(self.flippRatio)
        datRedSettings['Multiple SF scattering probability'] = self.multiSF
        datRedSettings['Keep intermediat workspaces'] = str(self.intermadiate)

        parameters['Data reduction settings'] = datRedSettings

        stdData = CommentedMap()

        stdData['Path'] = self.standardDataPath
        #stdData['Vanadium Suffix'] = self.VanSuffix
        #stdData['NiCr Suffix'] = self.NiCrSuffix
        #stdData['Background Suffix'] = self.backSuffix

        parameters['Standard Data'] = stdData

        sampleData = CommentedMap()

        sampleData['Data path'] = self.sampleDataPath
        sampleData['File prefix'] = self.filePrefix
        sampleData['File suffix'] = self.fileSuffix
        sampleData['Table'] = self.dataRuns

        parameters['Sample Data'] = sampleData


        print parameters
        script = ['']

        def l(line = ''):
            script[0] += line + '\n'

        #def get_log(workspace, tag):
        #    return "{}.getRun().getLogData('{}').value".format(workspace, tag)

        #def get_time(workspace):
        #    return get_log(workspace, 'duration')

        l("import numpy as np")
        l()

        return script[0]




class DNSReductionScripter(BaseReductionScripter):

    def __init__(self, name, facility):
        BaseReductionScripter.__init__(self, name, facility)