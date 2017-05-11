from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as sapi
import os
import sys
import scipy.constants as const

#from mantid.api import *
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, \
    FileProperty, FileAction
from mantid.kernel import Direction, StringListValidator, DateAndTime

from yaml import load
import math
import numpy as np
from yaml import CLoader as Loader, CDumper as Dumper

#!/usr/bin/env python
# -*- coding: utf8 -*-

class LoadYaml(PythonAlgorithm):

    def __init__(self):

        PythonAlgorithm.__init__(self)

    def name(self):
        return "LoadYaml"

    def PyInit(self):

        self.declareProperty(FileProperty("Filename","",FileAction.Load, '.yaml'),"The File to load from")

        self.declareProperty(WorkspaceProperty("OutputWorkspace",
                                               "", direction= Direction.Output),
                             doc="The name to use when saving the Workspace")
        return

    def PyExec(self):

        workspace_name = self.getPropertyValue("OutputWorkspace")
        filename = self.getPropertyValue("Filename")
        self.log().debug("Filename: " + filename)

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
        tof_delay_duration       = tof['delay_duration']*10e6
        tof_channel_duration_str = tof['channel_duration']
        tof_channel_duration     = float(tof_channel_duration_str)*10e6
        detector     = measurement['detectors'][0]

        if measurement['setup']['polarization'] == 'off':
            pol = '0'
        else:
            pol = measurement['setup']['polarization']


        duration = history['duration_counted']
        huber = orientation['rotation_angle']
        flipp = str(flipper['setting']).upper()

        units['duration'] = "Second"
        units['wavelength'] = 'Angstrom'
        units['angle'] = 'Degrees'


        sample_logs = [('run_title',                   measurement['unique_identifier'],    ''),
                       ('run_start',                   history['started'],                  ''),
                       ('run_end',                     history['stopped'],                  ''),
                       ('duration',                    duration,                            units['duration']),
                       ('Tsp',                         temperature['setpoint']['mean'],     units['temperature']),
                       ('T1',                          temperature['T1']['mean'],           units['temperature']),
                       ('T2',                          temperature['T2']['mean'],           units['temperature']),
                       ('huber',                       huber,                               units['angle']),
                       ('omega',                       huber - duration,                    units['angle']),
                       ('wavelength',                  settings_for['incident_wavelength'], units['wavelength']),
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
                       ('tof_cannels',                 tof_channels*10e-6,                  ''),
                       ('tof_delay',                   tof_delay_duration,                  units['duration']),
                       ('channel_width',               tof_channel_duration*10e-6,          units['duration']),
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

        x = []
        ts = []
        t_last = tof_delay_duration
        ts.append(t_last)
        for i in range(1,tof_channels):
            t = t_last + tof_channel_duration
            ts.append(t)
            t_last = t

        x.append(ts[0] - (tof_channel_duration / 2))
        for t in ts:
            x.append(t+(tof_channel_duration/2))

        dataX = np.array(x*ndet, np.float)
        dataY = np.array([detector['counts'][i] for i in detector['counts']], np.float)
        dataE = np.array(e, np.float)



        sapi.CreateWorkspace(OutputWorkspace = workspace_name, DataX = dataX, DataY = dataY,
                             DataE = dataE, NSpec = ndet, UnitX="TOF")

        out_ws = sapi.AnalysisDataService.retrieve(workspace_name)
        self.log().debug("Workspace name: " + workspace_name)

        sapi.LoadInstrument(out_ws, InstrumentName=dataMap['instrument']['name'], RewriteSpectraMap=True)

        sapi.RotateInstrumentComponent(out_ws, "bank0", X=0, Y=1, Z=0, Angle=detector['angle_tube0'])


        sapi.AddSampleLogMultiple(Workspace = out_ws, LogNames = log_names, LogValues = log_values, LogUnits = log_units)

        out_ws.setYUnit('Counts')

        self.setProperty("OutputWorkspace", out_ws)

        self.log().debug('channels' + str(tof_channels))
        self.log().debug('delay duration' + str(tof_channel_duration))
        self.log().debug('channel duration' + str(tof_channel_duration_str))
        self.log().debug(str(const.eV))

##########################################################################################
AlgorithmFactory.subscribe(LoadYaml)