# pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *
from mantid.geometry import CrystalStructure


# The reference data for these tests were created with PredictPeaks in the state at Release 3.5,
# if PredictPeaks changes significantly, both reference data and test may need to be adjusted.

# The WISH test has a data mismatch which might be caused by the 'old' code having a bug (issue #14105).
# The difference is that peaks may have different d-values because they are assigned to a different detector.
# Instead of using the CheckWorkspacesMatch, only H, K and L are compared.
class PredictPeaksTestWISH(stresstesting.MantidStressTest):
    def runTest(self):
        ws = CreateSimulationWorkspace(Instrument='WISH',
                                       BinParams='0,1,2',
                                       UnitX='TOF')

        SetUB(ws, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')
        peaks = PredictPeaks(ws,
                             WavelengthMin=0.5, WavelengthMax=6,
                             MinDSpacing=0.5, MaxDSpacing=10)

        reference = LoadNexus('predict_peaks_test_random_ub.nxs')

        hkls_predicted = self._get_hkls(peaks)
        hkls_reference = self._get_hkls(reference)

        lists_match, message = self._compare_hkl_lists(hkls_predicted, hkls_reference)

        self.assertEquals(lists_match, True, message)

    def _get_hkls(self, peaksWorkspace):
        h = peaksWorkspace.column('h')
        k = peaksWorkspace.column('k')
        l = peaksWorkspace.column('l')

        return [(x, y, z) for x, y, z in zip(h, k, l)]

    def _compare_hkl_lists(self, lhs, rhs):
        if len(lhs) != len(rhs):
            return False, 'Lengths do not match: {} vs. {}'.format(len(lhs), len(rhs))

        lhs_sorted = sorted(lhs)
        rhs_sorted = sorted(rhs)

        for i in range(len(lhs)):
            if lhs_sorted[i] != rhs_sorted[i]:
                return False, 'Mismatch at position {}: {} vs. {}'.format(i, lhs_sorted[i], rhs_sorted[i])

        return True, None


class PredictPeaksTestTOPAZ(stresstesting.MantidStressTest):
    def runTest(self):
        ws = CreateSimulationWorkspace(Instrument='TOPAZ',
                                       BinParams='0,1,2',
                                       UnitX='TOF')

        SetUB(ws, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')
        peaks = PredictPeaks(ws,
                             WavelengthMin=0.5, WavelengthMax=6,
                             MinDSpacing=0.5, MaxDSpacing=10)

        reference = LoadNexus('predict_peaks_test_random_ub_topaz.nxs')

        wsMatch = CheckWorkspacesMatch(peaks, reference)

        self.assertEquals(wsMatch, 'Success!')


class PredictPeaksCalculateStructureFactorsTest(stresstesting.MantidStressTest):
    def runTest(self):
        ws = CreateSimulationWorkspace(Instrument='WISH',
                                       BinParams='0,1,2',
                                       UnitX='TOF')

        SetUB(ws, a=5.5, b=6.5, c=8.1, u='12,1,1', v='0,4,9')

        # Setting some random crystal structure. Correctness of structure factor calculations is ensured in the
        # test suite of StructureFactorCalculator and does not need to be tested here.
        ws.sample().setCrystalStructure(
            CrystalStructure('5.5 6.5 8.1', 'P m m m', 'Fe 0.121 0.234 0.899 1.0 0.01'))

        peaks = PredictPeaks(ws,
                             WavelengthMin=0.5, WavelengthMax=6,
                             MinDSpacing=0.5, MaxDSpacing=10,
                             CalculateStructureFactors=True)

        self.assertEquals(peaks.getNumberPeaks(), 540)

        for i in range(540):
            peak = peaks.getPeak(i)
            self.assertLessThan(0.0, peak.getIntensity())

        peaks_no_sf = PredictPeaks(ws,
                                   WavelengthMin=0.5, WavelengthMax=6,
                                   MinDSpacing=0.5, MaxDSpacing=10,
                                   CalculateStructureFactors=False)

        for i in range(540):
            peak = peaks_no_sf.getPeak(i)
            self.assertEquals(0.0, peak.getIntensity())
