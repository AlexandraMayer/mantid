from __future__ import (absolute_import, division, print_function)

import mantid
from mantid.api import mtd
from mantid.simpleapi import CreateWorkspace, CreateSampleWorkspace, SaveYDA, ConvertSpectrumAxis, \
    LoadInstrument, AddSampleLog
import numpy as np
import os
import unittest


class SaveYDATest(unittest.TestCase):

    propn = 3
    propt = "PropTitle"
    expt = "Experiment Team"
    temperature = 100.0
    Ei = 1.0
    datax = [1, 2, 3]
    datay = [2, 3, 4]

    def cleanup(self, ws_name, filename):
        if os.path.exists(filename):
            os.remove(filename)
        if mantid.mtd.doesExist(ws_name):
            mantid.api.AnalysisDataService.remove(ws_name)

    def test_meta_data(self):
        ws = self._createWorkspace()
        f = self._file(ws, 'File')
        meta = []
        for i in range(3):
            s = f.readline()
            meta.append(s)

        self.assertEqual(meta[0], "Meta:\n")
        self.assertEqual(meta[1], "    format: yaml/frida 2.0\n")
        self.assertEqual(meta[2], "    type: gerneric tabular data\n")

    def test_history_all_samples(self):
        ws = self._createWorkspace()
        f = self._file(ws, 'File')
        history = []
        for i in range(0, 8):
            s = f.readline()
            if i >= 3:
                history.append(s)
        print(history)

        self.assertEqual(history[0], "History:\n")
        self.assertEqual(history[1], "  - Proposal number " + str(self.propn) + "\n")
        self.assertEqual(history[2], "  - " + self.propt + "\n")
        self.assertEqual(history[3], "  - " + self.expt + "\n")
        self.assertEqual(history[4], "  - data reduced with mantid\n")

    def test_coord(self):
        ws = self._createWorkspace()
        f = self._file(ws, 'File')
        coord = []
        for i in range(0, 12):
            s = f.readline()
            if i >= 8:
                coord.append(s)
        print(coord)

        self.assertEqual(coord[0], "Coord:\n")
        self.assertEqual(coord[1], "    x: {name: w, unit: meV}\n")
        self.assertEqual(coord[2], "    y: {name: \'S(q,w)\', unit: meV-1}\n")
        self.assertEqual(coord[3], "    z: {name: 2th, unit: deg}\n")

        ws = self._createWorkspace(yAxSpec=False)
        f = self._file(ws, 'File')
        coord = []
        for i in range(0, 12):
            s = f.readline()
            if i >= 8:
                coord.append(s)
        print(coord)

        self.assertEqual(coord[0], "Coord:\n")
        self.assertEqual(coord[1], "    x: {name: w, unit: meV}\n")
        self.assertEqual(coord[2], "    y: {name: \'S(q,w)\', unit: meV-1}\n")
        self.assertEqual(coord[3], "    z: {name: q, unit: A-1}\n")

    def test_rpar(self):
        ws = self._createWorkspace()
        f = self._file(ws, 'File')
        rpar = []
        for i in range(21):
            s = f.readline()
            if i >= 12:
                rpar.append(s)
        print(rpar)
        self.assertEqual(rpar[0], "RPar:\n")
        self.assertEqual(rpar[1], "  - name: T\n")
        self.assertEqual(rpar[2], "    unit: K\n")
        self.assertEqual(rpar[3], "    val: " + str(self.temperature) + "\n")
        self.assertEqual(rpar[4], "    stdv: 0\n")
        self.assertEqual(rpar[5], "  - name: Ei\n")
        self.assertEqual(rpar[6], "    unit: meV\n")
        self.assertEqual(rpar[7], "    val: " + str(self.Ei) + "\n")
        self.assertEqual(rpar[8], "    stdv: 0\n")

    def test_event_ws(self):
        ws = self._createWorkspace(False)
        print(ws.name())
        self.assertRaises(RuntimeError, SaveYDA, InputWorkspace= ws, Filename='File')

    def test_x_not_detaE(self):
        ws = self._createWorkspace(xAx=False)
        self.assertRaises(ValueError, SaveYDA, InputWorkspace= ws, Filename='File')

    def test_no_Instrument(self):
        ws = self._createWorkspace(instrument=False)
        self.assertRaises(ValueError, SaveYDA, InputWorkspace= ws, Filename='File')

    def test_y_not_mt_or_spec(self):
        ws = self._createWorkspace(yAxMt=False,yAxSpec=False)
        self.assertRaises(RuntimeError, SaveYDA, InputWorkspace= ws, Filename='File')

    def _add_all_sample_logs(self, ws):
        AddSampleLog(ws, "proposal_number", str(self.propn))
        AddSampleLog(ws, "proposal_title", self.propt)
        AddSampleLog(ws, "experiment_team", self.expt)
        AddSampleLog(ws, "temperature", str(self.temperature), LogUnit="F")
        AddSampleLog(ws, "Ei", str(self.Ei), LogUnit="meV")

    def _file(self, ws, filename):
        path = os.path.expanduser("~/" + filename + ".yaml")
        SaveYDA(InputWorkspace=ws, Filename=path)
        f = open(path, 'r')
        return f

    def _createWorkspace(self, ws_2D=True, sample=True, xAx=True, yAxSpec=True,
                         yAxMt=True, instrument=True, file=True):
        if not ws_2D:
            ws = CreateSampleWorkspace('Event', 'One Peak', XUnit='DeltaE')
            return ws
        if not xAx:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="TOF")
            return ws
        if not instrument:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            return ws
        if not yAxMt and not yAxSpec:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws,False,InstrumentName="TOFTOF")
            ConvertSpectrumAxis(InputWorkspace=ws,OutputWorkspace=ws,Target ='theta', EMode="Direct")
            return ws
        if not yAxSpec and yAxMt:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws,False,InstrumentName="TOFTOF")
            self._add_all_sample_logs(ws)
            ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace='ws2', Target ='ElasticQ', EMode="Direct")
            ws2 = mtd['ws2']
            print(str(ws2.getAxis(1).getUnit().caption()))
            return ws2
        if not sample:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws,False,InstrumentName="TOFTOF")
            return ws
        else:
            ws = CreateWorkspace(DataX=self.datax, DataY=self.datay, DataE=np.sqrt(self.datay), NSpec=1, UnitX="DeltaE")
            LoadInstrument(ws,False,InstrumentName="TOFTOF")
            self._add_all_sample_logs(ws)
            return ws

if __name__ == '__main__':
    unittest.main()
