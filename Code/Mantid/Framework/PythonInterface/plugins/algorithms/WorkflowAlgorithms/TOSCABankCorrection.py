#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import numpy as np


class TOSCABankCorrection(DataProcessorAlgorithm):

    _input_ws = None
    _output_ws = None
    _search_range = None
    _peak_tolerance = None
    _mode = None
    _peak_function = None


    def category(self):
        return 'PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Corrects TOSCA reductions where the peaks across banks are not in alignment.'


    def PyInit(self):
        self.declareProperty(WorkspaceProperty(name='InputWorkspace', defaultValue='',
                             direction=Direction.Input),
                             doc='Input reduced workspace')

        self.declareProperty(FloatArrayProperty(name='SearchRange'),
                             doc='Range over which to find peaks')

        self.declareProperty(name='ClosePeakTolerance', defaultValue=20.0,
                             doc='Tolerance under which peaks are considered to be the same')

        self.declareProperty(name='CorrectionMode', defaultValue='Average',
                             validator=StringListValidator(['Average', 'TallestPeak']),
                             doc='Type of correction to use')

        self.declareProperty(name='PeakFunction', defaultValue='Lorentzian',
                             validator=StringListValidator(['Lorentzian', 'Gaussian']),
                             doc='Type of peak to search for')

        self.declareProperty(MatrixWorkspaceProperty(name='OutputWorkspace', defaultValue='',
                             direction=Direction.Output),
                             doc='Output corrected workspace')

        self.declareProperty(name='CalculatedOffset', defaultValue=0.0,
                             direction=Direction.Output,
                             doc='Calculated spectrum offset')


    def _validate_range(self, name):
        """
        Validates a range property

        @param name Name of the property
        """

        range_prop = self.getProperty(name).value

        if len(range_prop) != 2:
            return 'Range must have two values'

        if range_prop[0] > range_prop[1]:
            return 'Range must be in format "low,high"'

        return ''


    def validateInput(self):
        issues = dict()

        # Validate search range
        search_range_valid = self._validate_range('SearchRange')
        if search_range_valid != '':
            issues['SearchRange'] = search_range_valid

        return issues


    def PyExec(self):
        self._get_properties()

        # Crop the sample workspace to the search range
        CropWorkspace(InputWorkspace=self._input_ws,
                      OutputWorkspace='__search_ws',
                      XMin=self._search_range[0],
                      XMax=self._search_range[1])

        peaks = self._get_peaks('__search_ws')
        DeleteWorkspace('__search_ws')

        delta = self._get_delta(peaks)
        offset = delta / 2

        logger.information('Offset for each spectrum is %f' % offset)

        self._apply_correction(offset)

        self.setPropertyValue('OutputWorkspace', self._output_ws)
        self.setProperty('CalculatedOffset', offset)


    def _get_properties(self):
        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._output_ws = self.getPropertyValue('OutputWorkspace')

        self._search_range = self.getProperty('SearchRange').value
        self._peak_tolerance = self.getProperty('ClosePeakTolerance').value

        self._mode = self.getPropertyValue('CorrectionMode')
        self._peak_function = self.getPropertyValue('PeakFunction')


    def _get_peaks(self, search_ws):
        """
        Finds matching peaks over the two banks.

        @param search_ws Workspace to search
        @return List of peak centres for matching peaks over both banks
        """

        # Find the peaks in each bank
        FindPeaks(InputWorkspace=search_ws,
                  PeaksList='__bank_1_peaks',
                  WorkspaceIndex=0,
                  PeakFunction=self._peak_function)

        FindPeaks(InputWorkspace=search_ws,
                  PeaksList='__bank_2_peaks',
                  WorkspaceIndex=1,
                  PeakFunction=self._peak_function)

        # Sort peaks by height, prefer to match tall peaks
        SortTableWorkspace(InputWorkspace='__bank_1_peaks',
                           OutputWorkspace='__bank_1_peaks',
                           Columns='height',
                           Ascending=False)

        bank_1_ws = mtd['__bank_1_peaks']
        bank_2_ws = mtd['__bank_2_peaks']

        matching_peaks = list()

        # Find the centres of two peaks that are close to each other on both banks
        for peak_idx in range(0, bank_1_ws.rowCount()):
            bank_1_centre = bank_1_ws.cell('centre', peak_idx)

            for other_peak_idx in range(0, bank_2_ws.rowCount()):
                bank_2_centre = bank_2_ws.cell('centre', other_peak_idx)

                if abs(bank_1_centre - bank_2_centre) < self._peak_tolerance:
                    matching_peaks.append((bank_1_centre, bank_2_centre))

        # Remove temporary workspaces
        DeleteWorkspace('__bank_1_peaks')
        DeleteWorkspace('__bank_2_peaks')

        logger.debug('Found matching peaks at: %s' % (str(matching_peaks)))
        return matching_peaks


    def _get_delta(self, peaks):
        """
        Gets the offset given a set of peaks.

        @param peaks List of peaks
        @return Spectrum offset
        """

        # Just use the first peak
        if self._mode == 'TallestPeak':
            delta = abs(peaks[0][1] - peaks[0][0])

        # Average difference of all peaks
        elif self._mode == 'Average':
            all_deltas = [abs(peak[1] - peak[0]) for peak in peaks]
            delta = np.average(all_deltas)

        logger.information('Calculated delta %f using method %s' % (delta, self._mode))
        return delta


    def _apply_correction(self, offset):
        """
        Applies correction to a copy of the input workspace.

        @param offset Spectrum offset
        """

        # Get the spectra for each bank plus sum of all banks
        ExtractSingleSpectrum(InputWorkspace=self._input_ws,
                              OutputWorkspace='__bank_1',
                              WorkspaceIndex=0)

        ExtractSingleSpectrum(InputWorkspace=self._input_ws,
                              OutputWorkspace='__bank_2',
                              WorkspaceIndex=1)

        ExtractSingleSpectrum(InputWorkspace=self._input_ws,
                              OutputWorkspace='__summed',
                              WorkspaceIndex=2)

        # Correct with shift in X
        ConvertAxisByFormula(InputWorkspace='__bank_1',
                             OutputWorkspace='__bank_1',
                             Axis='X', Formula='x=x+%f' % offset)

        ConvertAxisByFormula(InputWorkspace='__bank_2',
                             OutputWorkspace='__bank_2',
                             Axis='X', Formula='x=x-%f' % offset)

        # Rebin the two corrected spectra to the original workspace binning
        RebinToWorkspace(WorkspaceToRebin='__bank_1',
                         WorkspaceToMatch='__summed',
                         OutputWorkspace='__bank_1')

        RebinToWorkspace(WorkspaceToRebin='__bank_2',
                         WorkspaceToMatch='__summed',
                         OutputWorkspace='__bank_2')

        # Append spectra to get output workspace
        AppendSpectra(InputWorkspace1='__bank_1',
                      InputWorkspace2='__bank_2',
                      OutputWorkspace=self._output_ws)

        AppendSpectra(InputWorkspace1=self._output_ws,
                      InputWorkspace2='__summed',
                      OutputWorkspace=self._output_ws)

        # Remove temporary workspaces
        DeleteWorkspace('__bank_1')
        DeleteWorkspace('__bank_2')
        DeleteWorkspace('__summed')


AlgorithmFactory.subscribe(TOSCABankCorrection)
