import unittest
from mantid.simpleapi import FitGaussian, CreateSampleWorkspace, DeleteWorkspace
import logging

class FitGaussianTest(unittest.TestCase):
    def setUp(self):
        self.ws = None

    def tearDown(self):
        if self.ws is not None: DeleteWorkspace(self.ws)

    def _workspace(self,userFun):
        if self.ws is not None: DeleteWorkspace(self.ws)

        self.ws = CreateSampleWorkspace(OutputWorkspace="out",
            Function="User Defined",UserDefinedFunction=userFun,
            NumBanks=1, BankPixelWidth=1, XMin=0, XMax=10, BinWidth=0.1)

    def _gaussianWorkspace(self,peakCentre,height,sigma):
        self._workspace("name=Gaussian,PeakCentre=%s,Height=%s,Sigma=%s" %(peakCentre,height,sigma))

    def _linearWorkspace(self,A0):
        self._workspace("name=LinearBackground,A0=%s;" % A0)

    def _veryNarrowPeakWorkspace(self):
        self._gaussianWorkspace(5,1,.05)

    def test_errors(self):
        """Conditions that raise RuntimeError.
        """
        self._linearWorkspace(0)
        with self.assertRaises(RuntimeError):
            # bad index
            fitResult = FitGaussian(self.ws,1)

    def test_noFit(self):
        """Cases where fit is not possible.
        """
        self._linearWorkspace(0)
        fitResult = FitGaussian(self.ws,0)
        self.assertTupleEqual((0.0,0.0), fitResult)

        self._veryNarrowPeakWorkspace()
        fitResult = FitGaussian(self.ws,0)
        self.assertTupleEqual((0.0,0.0), fitResult)

    def _guessPeak(self,peakCentre,height,sigma):
        """Test-fitting one generated Gaussian peak.
        """
        self._gaussianWorkspace(peakCentre,height,sigma)
        fitPeakCentre,fitSigma = FitGaussian(self.ws,0)

        # require a close relative match between given and fitted values
        diffPeakCentre = abs((fitPeakCentre - peakCentre) / peakCentre)
        diffSigma      = abs((fitSigma      - sigma)      / sigma)

        self.assertLess(diffPeakCentre,.03)
        self.assertLess(diffSigma,     1e-6)

    def test_guessedPeaks(self):
        """Test that generated Gaussian peaks are reasonably well guessed.
        """
        self._guessPeak(2,10,.7)
        self._guessPeak(5,10,.7)
        self._guessPeak(9,10,.7)

if __name__ == "__main__":
    unittest.main()