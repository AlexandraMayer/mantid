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

        workspace_name = self.getPropertyValue("OutputWorkspace")
        filename = self.getPropertyValue("Filename")
        L1 = float(self.getPropertyValue("SourceSampleDistance"))
        tof_delay_factor = float(self.getPropertyValue("TofDelayFactor"))
        channel_widths_factor = float(self.getPropertyValue("ChannelWidthsFactor"))

        file = open(filename)

        dataMap = load(file, Loader=Loader)

        file.close()

        units        = dataMap['format']['units']
        measurement  = dataMap['measurement']
        history      = measurement['history']
        temperature  = measurement['sample']['temperature']
        orientation  = measurement['sample']['orientation']
        setup        = measurement['setup']
        settings_for = setup['settings_for']
        flipper      = setup['flipper']
        slit_i       = setup['slit_i']
        xyz_coil     = setup['xyz_coil']
        tof          = setup['time_of_flight']

        tof_channels             = tof['number_of_channels']
        tof_delay_duration       = tof['delay_duration'] * tof_delay_factor
        tof_channel_duration_str = tof['channel_duration'] * channel_widths_factor
        tof_channel_duration     = float(tof_channel_duration_str)*1e+06

        detector = measurement['detectors'][0]

        if measurement['setup']['polarization'] == 'off':
            pol = '0'
        else:
            pol = measurement['setup']['polarization']

        duration = history['duration_counted']
        huber    = orientation['rotation_angle']
        flipp    = str(flipper['setting']).upper()

        units['duration']   = "Second"
        units['wavelength'] = 'Angstrom'
        units['angle']      = 'Degrees'

        wavelength = settings_for['incident_wavelength']

        sample_logs = [('run_title',                   measurement['unique_identifier'],    ''),
                       ('run_start',                   history['started'],                  ''),
                       ('run_end',                     history['stopped'],                  ''),
                       ('duration',                    duration,                            units['duration']),
                       ('Tsp',                         temperature['setpoint']['mean'],     units['temperature']),
                       ('T1',                          temperature['T1']['mean'],           units['temperature']),
                       ('T2',                          temperature['T2']['mean'],           units['temperature']),
                       ('huber',                       huber,                               units['angle']),
                       ('omega',                       huber - duration,                    units['angle']),
                       ('wavelength',                  wavelength,                          units['wavelength']),
                       ('Ei',                          settings_for['incident_energy'],     units['energy']),
                       ('flipper',                     flipp,                               ''),
                       ('flipper_precession',          flipper['precession_current'],       units['current']),
                       ('flipper_z_compensation',      flipper['z_compensation_current'],   units['current']),
                       ('slit_i_upper_blade_position', slit_i['upper_clearance'],           units['clearance']),
                       ('slit_i_lower_blade_position', slit_i['lower_clearance'],           units['clearance']),
                       ('slit_i_left_blade_position',  slit_i['left_clearance'],            units['clearance']),
                       ('slit_i_right_blade_position', slit_i['right_clearance'],           units['clearance']),
                       ('C_a',                         xyz_coil['coil_a_current'],          units['current']),
                       ('C_b',                         xyz_coil['coil_b_current'],          units['current']),
                       ('C_c',                         xyz_coil['coil_c_current'],          units['current']),
                       ('C_zb',                        xyz_coil['coil_zb_current'],         units['current']),
                       ('C_zt',                        xyz_coil['coil_zt_current'],         units['current']),
                       ('polarisation',                pol,                                 ''),
                       ('tof_channels',                tof_channels,                        ''),
                       ('tof_delay',                   tof_delay_duration,                  units['duration']),
                       ('channel_width',               tof_channel_duration,                units['duration']),
                       ('deterota',                    detector['angle_tube0'],             units['angle'])]

        log_names  = [item[0] for item in sample_logs]
        log_values = [item[1] for item in sample_logs]
        log_units  = [item[2] for item in sample_logs]

        ndet = len(detector['counts'])

        e = []
        for i in detector['counts']:
            es = []
            for y in detector['counts'][i]:
                es.append(math.sqrt(y))
            e.append(es)

        v = const.h/(const.m_n*wavelength*1e-10)
        tof1 = L1/v
        x0 = tof1 + tof_delay_duration

        dataX = np.linspace(x0, x0 + tof_channels*tof_channel_duration, tof_channels+1)
        dataY = np.array([detector['counts'][i] for i in detector['counts']], np.float)
        dataE = np.array(e, np.float)

        sapi.CreateWorkspace(OutputWorkspace=workspace_name, DataX=dataX, DataY=dataY,
                             DataE=dataE, NSpec=ndet, UnitX="TOF")

        out_ws = sapi.AnalysisDataService.retrieve(workspace_name)

        sapi.LoadInstrument(out_ws, InstrumentName=dataMap['instrument']['name'], RewriteSpectraMap=True)

        sapi.RotateInstrumentComponent(out_ws, "bank0", X=0, Y=1, Z=0, Angle=detector['angle_tube0'])

        sapi.AddSampleLogMultiple(Workspace=out_ws, LogNames=log_names,
                                  LogValues=log_values, LogUnits = log_units)

        out_ws.setYUnit('Counts')

        self.setProperty("OutputWorkspace", out_ws)

##########################################################################################

AlgorithmFactory.subscribe(LoadYaml)
