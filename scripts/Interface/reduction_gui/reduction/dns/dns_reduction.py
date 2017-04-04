import xml.dom.minidom

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from reduction_gui.reduction.scripter import BaseScriptElement, BaseReductionScripter


class DNSScriptElement(BaseScriptElement):

    NORM_DURATION = 0
    NORM_MONITOR  = 1

    OUT_SOFT         = 0
    OUT_MAGNETIC     = 1
    OUT_SINGLE_CRYST = 2

    DEF_normalise = NORM_DURATION

    DEF_NEUT_WAVE_LEN = 0.0

    DEF_MASK_MIN_ANGLE = 0.0
    DEF_MASK_MAX_ANGLE = 0.0

    DEF_output = OUT_SOFT

    DEF_DetEffi    = True
    DEF_SumVan     = False
    DEF_SubInst    = True
    DEF_FlippRatio = True

    DEF_MultiSF = 0.0

    DEF_Intermadiate = False

    DEF_VANSuffix  = 'vana'
    DEF_NiCrSuffix = 'nicr'
    DEF_BackSuffix = 'leer'


    XML_TAG = 'DNSReducton'

    def reset(self):
        self.facility_name   = ''
        self.instrument_name = ''

        self.currTable = ''
        self.twoTheta

        self.normalise = self.DEF_normalise

        self.neutronWaveLen = self.DEF_NEUT_WAVE_LEN

        self.maskMinAngle = self.DEF_MASK_MIN_ANGLE
        self.maskMaxAngle = self.DEF_MASK_MAX_ANGLE

        self.outDir = ''

        self.outPrefix = ''

        self.out = self.DEF_output

        self.detEffi    = self.DEF_DetEffi
        self.sumVan     = self.DEF_SumVan
        self.subInst    = self.DEF_SubInst
        self.flippRatio = self.DEF_FlippRatio

        self.multiSF = self.DEF_MultiSF

        self.intermadiate = self.DEF_Intermadiate

        self.standardDataPath = ''
        self.VanSuffix        = self.DEF_VANSuffix
        self.NiCrSuffix       = self.DEF_NiCrSuffix
        self.backSuffix       = self.DEF_BackSuffix

        self.sampleDataPath = ''

        self.filePrefix = ''
        self.fileSuffix = ''

        self.dataRuns = []

    def to_xml(self):

        res =['']

        def put(tag, val):
            res[0] += ' <{0}>{1}</{0}>\n'.format(tag, str(val))

        put('current_table',       self.currTable)
        put('two_Theta',           self.twoTheta)
        put('normalise',           self.normalise)
        put('neutron_wave_length', self.neutronWaveLen)

        put('mask_detector_min_angle', self.maskMinAngle)
        put('mask_detector_max_angle', self.maskMaxAngle)

        put('output_directory',   self.outDir)
        put('output_file_prefix', self.outPrefix)
        put('output',             self.out)

        put('detector_efficiency',    self.detEffi)
        put('sum_Vanadium',           self.sumVan)
        put('subtract_instrument',    self.subInst)
        put('flipping_ratio',         self.flippRatio)
        put('multiple_SF_scattering', self.multiSF)
        put('keep_intermediate',      self.intermadiate)

        put('standard_data_path', self.standardDataPath)
        put('Vanadium_suffix',    self.VanSuffix)
        put('NiCr_suffix',        self.NiCrSuffix)
        put('background_suffix',  self.backSuffix)

        put('sample_data_path', self.sampleDataPath)
        put('file_prefix',      self.filePrefix)
        put('file_suffix',      self.fileSuffix)

        for (runs, cmnt) in self.dataRuns:
            put('data_runs',    runs)
            put('data_comment', cmnt)

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

            self.currTable      = get_str('current_table')
            self.twoTheta       = get_flt('two_Theta')
            self.normalise      = get_int('normalise',           self.DEF_normalise)
            self.neutronWaveLen = get_flt('neutron_wave_length', self.DEF_NEUT_WAVE_LEN)

            self.maskMinAngle = get_flt('mask_detector_min_angle', self.DEF_MASK_MIN_ANGLE)
            self.maskMaxAngle = get_flt('mask_detector_max_angle', self.DEF_MASK_MAX_ANGLE)

            self.outDir    = get_str('output_directory')
            self.outPrefix = get_str('output_file_prefix')
            self.out       = get_int('output', self.DEF_output)

            self.detEffi      = get_bol('detector_efficiency',    self.DEF_DetEffi)
            self.sumVan       = get_bol('sum_Vanadium',           self.DEF_SumVan)
            self.subInst      = get_bol('subtract_instrument',    self.DEF_SubInst)
            self.flippRatio   = get_bol('flipping_ratio',         self.DEF_FlippRatio)
            self.multiSF      = get_flt('multiple_SF_scattering', self.DEF_MultiSF)
            self.intermadiate = get_bol('keep_intermediate',      self.DEF_Intermadiate)

            self.standardDataPath = get_str('standard_data_path')
            self.VanSuffix        = get_str('Vanadium_suffix',   self.DEF_VANSuffix)
            self.NiCrSuffix       = get_str('NiCr_suffix',       self.DEF_NiCrSuffix)
            self.backSuffix       = get_str('background_suffix', self.DEF_BackSuffix)

            self.sampleDataPath = get_str('sample_data_path')
            self.fileSuffix     = get_str('file_prefix')
            self.filePrefix     = get_str('file_suffix')

            dataRuns = get_str('data_runs')
            dataCmts = get_str('data_comment')

            for i in range(min(len(dataRuns), len(dataCmts))):
                self.dataRuns.append((dataRuns[i], dataCmts[i]))


    def to_script(self):

        def error(message):
            raise RuntimeError('DNS reduction error: ' + message)

        if not(self.maskMinAngle < self.maskMaxAngle):
            error('incorrect mask detector angles')

        script = ['']

        def l(line = ''):
            script[0] += line + '\n'

        def get_log(workspace, tag):
            return "{}.getRun().getLogData('{}').value".format(workspace, tag)

        def get_time(workspace):
            return get_log(workspace, 'duration')

        l("import numpy as np")
        l()





class DNSReductionScripter(BaseReductionScripter):

    def __init__(self, name, facility):
        BaseReductionScripter.__init__(self, name, facility)