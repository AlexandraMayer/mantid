from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as sapi
import scipy.constants as const

from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, \
    FileProperty, FileAction
from mantid.kernel import Direction, FloatBoundedValidator

from yaml import load
import math
import numpy as np
from yaml import CLoader as Loader


class LoadYaml(PythonAlgorithm):

    def __init__(self):

        PythonAlgorithm.__init__(self)

    def name(self):
        return "LoadYaml"

    def PyInit(self):

        self.declareProperty(FileProperty("Filename", "", FileAction.Load, '.yaml'), doc="The File to load from")

        self.declareProperty(WorkspaceProperty("OutputWorkspace",
                                               "", direction= Direction.Output),
                             doc="The name to use when saving the Workspace")

        self.declareProperty(name='SourceSampleDistance', defaultValue=0.36325,
                             validator=FloatBoundedValidator(lower=0.0),
                             doc='Distance from chopper to sample in meters')

        self.declareProperty(name='TofDelayFactor', defaultValue=1.0, validator=FloatBoundedValidator(lower=0.0))

        self.declareProperty(name='ChannelWidthsFactor', defaultValue=1.0, validator=FloatBoundedValidator(lower=0.0))

        return

    def PyExec(self):

        warnings = []

        workspace_name = self.getPropertyValue("OutputWorkspace")
        filename = self.getPropertyValue("Filename")
        L1 = float(self.getPropertyValue("SourceSampleDistance"))
        tof_delay_factor = float(self.getPropertyValue("TofDelayFactor"))
        channel_widths_factor = float(self.getPropertyValue("ChannelWidthsFactor"))

        file = open(filename)

        if not file:
            self.log().warning("no file open")

        lines = file.readlines()[0:2]

        file.close()

        if not lines[0] == 'instrument:\n' or not lines[1] == '    name: DNS\n':
            self.log().error(filename + " is not a valid DNS data file")
        else:
            file = open(filename)
            dataMap = load(file, Loader=Loader)

            sample_logs = []
            counts_bool = True
            angle_tube0_bool = True

            file.close()
            if not dataMap:
                self.log().error("No nodes in File")
            else:
                if 'format' not in dataMap:
                    warnings.append("no format found")
                    units = self.emptyUnits()
                else:
                    if not dataMap["format"] or 'units' not in dataMap['format'] or not dataMap['format']['units']:
                        warnings.append("no units found")
                        units = self.emptyUnits()
                    else:
                        units = dataMap['format']['units']
                        if 'angle' not in units or not units['angle']:
                            units['angle'] = ''
                            warnings.append('no unit for angle found')
                        if 'clearance'not in units or not units['clearance']:
                            units['clearance'] = ''
                            warnings.append('no unit for clearance found')
                        if 'current' not in units or not units['current']:
                            units['current'] = ''
                            warnings.append('no unit for current found')
                        if 'duration' not in units or not units['duration']:
                            units['duration'] = ''
                            warnings.append('no unit for duration foud')
                        if 'energy' not in units or not units['energy']:
                            units['energy'] = ''
                            warnings.append('no unit for energy found')
                        if 'temperature' not in units or not units['temperature']:
                            units['temperature'] = ''
                            warnings.append('no unit for temperatur found')
                        if 'wavelength' not in units or not units['wavelength']:
                            units['wavelength'] = ''
                            warnings.append('no unit for wavlength found')

                if 'measurement' not in dataMap or not dataMap['measurement']:
                    self.log().error('No measurements, can not create workspace')
                else:
                    measurement = dataMap['measurement']
                    if 'detectors' not in measurement or not measurement['detectors'] or not measurement['detectors'][0]:
                        self.log().error("no counts found")
                        counts_bool = False
                    else:
                        detector = measurement['detectors'][0]
                        if 'angle_tube0' not in detector or (not detector['angle_tube0'] and detector['angle_tube0'] !=
                        0.0):
                            warnings.append("no deterota found")
                            deterota = ''
                            angle_tube0_bool = False
                        else:
                            deterota = detector['angle_tube0']
                        if 'counts' not in detector or not detector['counts']:
                            self.log().error("no counst found")
                            counts_bool = False
                        else:
                            ndet = len(detector['counts'])

                            if 'unique_identifier' not in measurement or not measurement['unique_identifier']:
                                warnings.append("no run title found")
                                run_title = ''
                            else:
                                run_title = measurement['unique_identifier']

                            if 'sample' not in measurement or not measurement['sample']:
                                warnings.append('no sample found')
                                sample_bool = False
                                Tsp   = ''
                                T1    = ''
                                T2    = ''
                                huber = ''
                            else:
                                sample_bool = True
                                if 'temperature' not in measurement['sample'] or not measurement['sample']['temperature']:
                                    warnings.append('no temperature for sample found')
                                    Tsp = ''
                                    T1  = ''
                                    T2  = ''
                                else:
                                    temperature = measurement['sample']['temperature']
                                    if 'setpoint' not in temperature or 'mean' not in temperature['setpoint'] or\
                                            (not temperature['setpoint']['mean'] and temperature['setpoint']['mean'] != 0.0):
                                        warnings.append('no Tsp found')
                                        Tsp = ''
                                    else:
                                        Tsp = temperature['setpoint']['mean']
                                    if 'T1' not in temperature or 'mean' not in temperature['T1'] or \
                                            (not temperature['T1']['mean'] and temperature['T1']['mean'] != 0.0):
                                        warnings.append('no T1 found')
                                        T1 = ''
                                    else:
                                        T1 = temperature['T1']['mean']
                                    if 'T2' not in temperature or 'mean' not in temperature['T2'] or  \
                                            (not temperature['T2']['mean'] and temperature['T2']['mean'] != 0.0):
                                        warnings.append('no T2 found')
                                        T2 = ''
                                    else:
                                        T2 = temperature['T2']['mean']

                                if 'orientation' not in measurement['sample'] or not measurement['sample']['orientation'] or 'rotation_angle' not in \
                                        measurement['sample']['orientation'] or  \
                                        (not measurement['sample']['orientation']['rotation_angle'] and
                                        measurement['sample']['orientation']['rotation_angle'] != 0.0):
                                    warnings.append('no orientation for sample found')
                                    sample_bool = False
                                    huber       = ''
                                else:
                                    huber = measurement['sample']['orientation']['rotation_angle']

                            if 'history' not in measurement or not measurement['history']:
                                warnings.append('no history found')
                                duration  = ''
                                run_start = ''
                                run_end   = ''
                                omega     = ''
                            else:
                                history = measurement['history']
                                if 'duration_counted' not in history or (not history['duration_counted'] and history['duration_counted'] != 0.0) :
                                    warnings.append("no duration found in history")
                                    duration = ''
                                    omega    = ''
                                else:
                                    duration = history['duration_counted']
                                    if sample_bool:
                                        omega = huber - duration
                                    else:
                                        omega = ''

                                if 'started' not in history or not history['started']:
                                    warnings.append('no run start found')
                                    run_start = ''
                                else:
                                    run_start = history['started']
                                if 'stopped' not in history or not history['stopped']:
                                    warnings.append('no run end found')
                                    run_end = ''
                                else:
                                    run_end = history['stopped']

                            if 'setup' not in measurement or not measurement['setup']:
                                warnings.append("no setup found")
                                wavelength           = 4.2
                                Ei                   = ''
                                flipp                = ''
                                flipp_pre            = ''
                                flipp_z              = ''
                                slit_upper           = ''
                                slit_lower           = ''
                                slit_left            = ''
                                slit_right           = ''
                                coil_a               = ''
                                coil_b               = ''
                                coil_c               = ''
                                coil_zb              = ''
                                coil_zt              = ''
                                tof_delay_duration   = 0.0
                                if counts_bool:
                                    tof_channels         = len(measurement["detectors"][0]['counts'][0])
                                else:
                                    self.log().error('no counst found')
                                tof_channel_duration = 1.0
                                pol                  = ''
                            else:
                                setup = measurement['setup']
                                if 'settings_for' not in setup or not setup['settings_for']:
                                    warnings.append('no settings in setup found')
                                    wavelength = 4.2
                                    Ei         = ''
                                else:
                                    settings_for = setup['settings_for']
                                    if 'incident_wavelength' not in settings_for or not settings_for['incident_wavelength']:
                                        warnings.append('no settings for wavelength, using default 4.2 Angstrom')
                                        wavelength = 4.2
                                    elif settings_for['incident_wavelength'] == 0.0:
                                        warnings.append('wavelength cant be zero, using default 4.2 Angstrom')
                                        wavelength = 4.2
                                    else:
                                        wavelength = settings_for['incident_wavelength']
                                    if 'incident_energy' not in settings_for or (not settings_for['incident_energy']
                                    and settings_for['incident_energy'] != 0.0):
                                        warnings.append('no Ei found')
                                        Ei = ''
                                    else:
                                        Ei = settings_for['incident_energy']

                                if 'flipper' not in setup or not setup['flipper']:
                                    warnings.append('no flipper found')
                                    flipp     = ''
                                    flipp_pre = ''
                                    flipp_z   = ''
                                else:
                                    flipper      = setup['flipper']
                                    if 'setting' not in flipper or not flipper['setting']:
                                        warnings.append('no setting for flipper found')
                                        flipp = ''
                                    else:
                                        flipp    = str(flipper['setting']).upper()
                                    if 'precession_current' not in flipper or (not flipper['precession_current']
                                    and flipper['precession_current'] != 0.0):
                                        warnings.append('no precession for flipper found')
                                        flipp_pre = ''
                                    else:
                                        flipp_pre = flipper['precession_current']
                                    if 'z_compensation_current' not in flipper or (not flipper['z_compensation_current']
                                    and flipper['z_compensation_current'] != 0.0):
                                        warnings.append('no z compensation for flipper found')
                                        flipp_z = ''
                                    else:
                                        flipp_z = flipper['z_compensation_current']

                                if 'slit_i' not in setup or not setup['slit_i']:
                                    warnings.append('not slit i found')
                                    slit_upper = ''
                                    slit_lower = ''
                                    slit_left  = ''
                                    slit_right = ''
                                else:
                                    slit_i = setup['slit_i']
                                    if 'upper_clearance' not in slit_i or (not slit_i['upper_clearance'] and
                                    slit_i['upper_clearance'] != 0.0):
                                        warnings.append('no upper slit i found')
                                        slit_upper = ''
                                    else:
                                        slit_upper = slit_i['upper_clearance']
                                    if 'lower_clearance' not in slit_i or (not slit_i['lower_clearance'] and
                                    slit_i['lower_clearance'] != 0.0):
                                        warnings.append('no lower slit i found')
                                        slit_lower = ''
                                    else:
                                        slit_lower = slit_i['lower_clearance']
                                    if 'left_clearance' not in slit_i or (not slit_i['left_clearance'] and
                                    slit_i['left_clearance'] != 0.0):
                                        warnings.append('no left slit i found')
                                        slit_left = ''
                                    else:
                                        slit_left = slit_i['left_clearance']
                                    if 'right_clearance' not in slit_i or (not slit_i['right_clearance'] and
                                    slit_i['right_clearance'] != 0.0):
                                        warnings.append('no right slit i found')
                                        slit_right = ''
                                    else:
                                        slit_right = slit_i['right_clearance']

                                if 'xyz_coil' not in setup or not setup['xyz_coil']:
                                    warnings.append('no xyz coil found')
                                    coil_a  = ''
                                    coil_b  = ''
                                    coil_c  = ''
                                    coil_zb = ''
                                    coil_zt = ''
                                else:
                                    xyz_coil = setup['xyz_coil']
                                    if 'coil_a_current' not in xyz_coil or (not xyz_coil['coil_a_current'] and
                                    xyz_coil['coil_a_current'] != 0.0):
                                        warnings.append('no coil a found')
                                        coil_a = ''
                                    else:
                                        coil_a = xyz_coil['coil_a_current']
                                    if 'coil_b_current' not in xyz_coil or (not xyz_coil['coil_b_current'] and
                                    xyz_coil['coil_b_current'] != 0.0):
                                        warnings.append('no coil b found')
                                        coil_b = ''
                                    else:
                                        coil_b = xyz_coil['coil_b_current']
                                    if 'coil_c_current' not in xyz_coil or (not xyz_coil['coil_c_current'] and
                                    xyz_coil['coil_c_current'] != 0.0):
                                        warnings.append('no coil c found')
                                        coil_c = ''
                                    else:
                                        coil_c = xyz_coil['coil_c_current']
                                    if 'coil_zb_current' not in xyz_coil or (not xyz_coil['coil_zb_current'] and
                                    xyz_coil['coil_zb_current'] != 0.0):
                                        warnings.append('no coil zb found')
                                        coil_zb = ''
                                    else:
                                        coil_zb = xyz_coil['coil_zb_current']
                                    if 'coil_zt_current' not in xyz_coil or (not xyz_coil['coil_zt_current'] and
                                        xyz_coil['coil_zt_current'] != 0.0):
                                        warnings.append('no coil zt found')
                                        coil_zt = ''
                                    else:
                                        coil_zt = xyz_coil['coil_zt_current']

                                if 'time_of_flight' not in setup or not setup['time_of_flight']:
                                    warnings.append('no time of flight data found using defaults')
                                    tof_channels = ndet
                                    tof_delay_duration = 0.0
                                    tof_channel_duration = 1.0
                                else:
                                    tof = setup['time_of_flight']

                                    if 'number_of_channels' not in tof or (not tof['number_of_channels'] and
                                    tof['number_of_channels'] != 0.0):
                                        warnings.append('no number of channels found, using number of counts')
                                        tof_channels = ndet
                                    else:
                                        tof_channels = tof['number_of_channels']
                                    if 'delay_duration' not in tof or (not tof['delay_duration'] and
                                    tof['delay_duration'] != 0.0):
                                        warnings.append('no delay duration for time of flight found, using default 0.0')
                                        tof_delay_duration = 0.0
                                    else:
                                        tof_delay_duration = tof['delay_duration'] * tof_delay_factor
                                    if 'channel_duration' not in tof or (not tof['channel_duration'] and
                                    tof['channel_duration'] != 0.0):
                                        warnings.append('no channel widths for time of flight found, using default 1.0')
                                        tof_channel_duration = 1.0
                                    else:
                                        tof_channel_duration_str = tof['channel_duration']
                                        tof_channel_duration = float(tof_channel_duration_str)*1e+06*channel_widths_factor

                                if 'polarization' not in setup or not setup['polarization']:
                                    warnings.append('no polarization found')
                                    pol = ''
                                elif setup['polarization'] == 'off':
                                    pol = '0'
                                else:
                                    pol = setup['polarization']

                            if units['duration'] == 's':
                                units['duration'] = "Second"
                            if units['wavelength'] == 'A':
                                units['wavelength'] = 'Angstrom'
                            if units['angle'] == 'deg':
                                units['angle'] = 'Degrees'

                            sample_logs = [('run_title',                   run_title,            ''),
                                           ('run_start',                   run_start,            ''),
                                           ('run_end',                     run_end,              ''),
                                           ('duration',                    duration,             units['duration']),
                                           ('Tsp',                         Tsp,                  units['temperature']),
                                           ('T1',                          T1,                   units['temperature']),
                                           ('T2',                          T2,                   units['temperature']),
                                           ('huber',                       huber,                units['angle']),
                                           ('omega',                       omega,                units['angle']),
                                           ('wavelength',                  wavelength,           units['wavelength']),
                                           ('Ei',                          Ei,                   units['energy']),
                                           ('flipper',                     flipp,                ''),
                                           ('flipper_precession',          flipp_pre,            units['current']),
                                           ('flipper_z_compensation',      flipp_z,              units['current']),
                                           ('slit_i_upper_blade_position', slit_upper,           units['clearance']),
                                           ('slit_i_lower_blade_position', slit_lower,           units['clearance']),
                                           ('slit_i_left_blade_position',  slit_left,            units['clearance']),
                                           ('slit_i_right_blade_position', slit_right,           units['clearance']),
                                           ('C_a',                         coil_a,               units['current']),
                                           ('C_b',                         coil_b,               units['current']),
                                           ('C_c',                         coil_c,               units['current']),
                                           ('C_zb',                        coil_zb,              units['current']),
                                           ('C_zt',                        coil_zt,              units['current']),
                                           ('polarisation',                pol,                  ''),
                                           ('tof_channels',                tof_channels,         ''),
                                           ('tof_delay',                   tof_delay_duration,   units['duration']),
                                           ('channel_width',               tof_channel_duration, units['duration']),
                                           ('deterota',                    deterota,             units['angle'])]


                            e = []
                            for i in detector['counts']:
                                es = []
                                for y in detector['counts'][i]:
                                    es.append(math.sqrt(y))
                                e.append(es)

                            v = const.h/(const.m_n*wavelength*1e-10)
                            tof1 = L1/v
                            x0 = tof1 + tof_delay_duration

                            if tof_channels != len(detector['counts'][0]):
                                tof_channels = len(detector['counts'][0])

                            dataX = np.linspace(x0, x0 + tof_channels*tof_channel_duration, tof_channels+1)


                            dataY = np.array([detector['counts'][i] for i in detector['counts']], np.float)
                            dataE = np.array(e, np.float)

                            log_names  = [item[0] for item in sample_logs]
                            log_values = [item[1] for item in sample_logs]
                            log_units  = [item[2] for item in sample_logs]

                            sapi.CreateWorkspace(OutputWorkspace=workspace_name, DataX=dataX, DataY=dataY,
                                             DataE=dataE, NSpec=ndet, UnitX="TOF")

                            out_ws = sapi.AnalysisDataService.retrieve(workspace_name)

                            if 'instrument' not in dataMap:
                                warnings.append('no instrument in file')
                            else:
                                if not dataMap['instrument'] or 'name' not in dataMap['instrument']:
                                    warnings.append('no name for instrument in file')
                                else:
                                    sapi.LoadInstrument(out_ws, InstrumentName=dataMap['instrument']['name'], RewriteSpectraMap=True)

                                    if angle_tube0_bool:
                                        sapi.RotateInstrumentComponent(out_ws, "bank0", X=0, Y=1, Z=0, Angle=detector['angle_tube0'])

                            sapi.AddSampleLogMultiple(Workspace=out_ws, LogNames=log_names,
                                                      LogValues=log_values, LogUnits = log_units)


                            out_ws.setYUnit('Counts')

                            self.setProperty("OutputWorkspace", out_ws)

                            for warn in warnings:
                                self.log().warning(warn)

    def emptyUnits(self):
        units = {'duration': '', 'wavelength': '', 'angle': 'Degrees', 'temperature': '', 'energy': '', 'current': '', 'clearance': ''}
        return units

##########################################################################################

AlgorithmFactory.subscribe(LoadYaml)
