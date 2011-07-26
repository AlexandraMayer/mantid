import unittest
from mantidsimple import *
import ISISCommandInterface as ISIS

class SANSMaskCommands(unittest.TestCase):
    """
        The masking commands that are used in the ISIS SANS settings file
        or on the Python command prompt
    """
    
    def setUp(self):
        
        self.test_ws_name = "SANSMaskCommands_ws"
        LoadEmptyInstrument('LOQ_Definition_20020226-.xml', self.test_ws_name)
        self.test_ws = mantid[self.test_ws_name]

        # Use the LOQ setttings
        ISIS.LOQ()

        # Test the LOQ instrument is what it suppose to be 
        self.assertEqual(self.test_ws.getNumberHistograms(), 17778)
        self.assertTrue(self.test_ws.readY(0)[0] > 0)
        
    def test_single_spectrum(self):
        """
            Checks the ISIS specfic mask command for spectra numbers
        """
        spec_nums1 = 7341
        spec_nums2 = 17341

        # flags some whole spectra for masking
        ISIS.Mask('mask S'+str(spec_nums1))
        ISIS.Mask('MASK S'+str(spec_nums2))

        ISIS.ReductionSingleton().mask.execute(ISIS.ReductionSingleton(), self.test_ws_name)

        #sanity check the unmasked, most pixels should start at 1, check a random number
        self.assertEqual(self.test_ws.readY(500)[0], 1)


        # note spectrum index is one less than the spectrum number for the main detector
        self.assertEqual(self.test_ws.readY(spec_nums1-1)[0], 0)
        # for the front detector (HAB) the offset is 5
        self.assertEqual(self.test_ws.readY(spec_nums2-5)[0], 0)
        
    def test_masking_timebins(self):
        """
            Time bin masking uses the MaskBins algorithm
        """
        ws=Load("LOQ48094.raw","testMaskingTimebins")[0]
       

        # flags some whole spectra for masking
        ISIS.Mask('mask/T '+str(3500)+' '+str(3700))
        ISIS.Mask('mask/time '+str(13500)+' '+str(13700))

        ISIS.ReductionSingleton().mask.execute(ISIS.ReductionSingleton(), "testMaskingTimebins")
        # 3500-3700 microseconds masks the first bins
        self.assertEqual(ws.readY(0)[0], 0)
        self.assertEqual(ws.readY(0)[1], 0)
        self.assertEqual(ws.readY(0)[3], 218284)       
        # bin 54 is in the 13500-13700 microsecond range
        self.assertEqual(ws.readY(0)[54], 0)       

    def test_phi_mask(self):
        """
            Phi masks use the MaskDetectorsInShape algorithm and are controlled using the L/PHI
            user file command
        """
        #test with and without mirror, is there a problem with mirror?
        ISIS.ReductionSingleton().mask.set_phi_limit(-10.0, 55.5, False)
        ISIS.ReductionSingleton().mask.execute(ISIS.ReductionSingleton(), self.test_ws_name)
        
        self.assertEqual(self.test_ws.readY(16601)[0], 1)
        self.assertEqual(self.test_ws.readY(16600)[0], 0)
        self.assertEqual(self.test_ws.readY(9782)[0], 1)
        self.assertEqual(self.test_ws.readY(9910)[0], 0)
        self.assertEqual(self.test_ws.readY(5376)[0], 0)
        self.assertEqual(self.test_ws.readY(14468)[0], 1)
        
    def test_mask_line(self):
        """
            The line is a semi-infinite line that starts at the beam centre, it is implemented using the
            MaskDetectorsInShape algorithm and are controlled with MASK/LINE user file command
        """
        # this is a thin vertical line that will mask a single column of detectors
        ISIS.Mask('MASK/LINE '+str(2)+' '+str(90))

        ISIS.ReductionSingleton().mask.execute(ISIS.ReductionSingleton(), self.test_ws_name)
        
        num_monitors = 4
        bank_square = 128
        centre_main_bank = num_monitors+(bank_square/2)*(bank_square+1)
        end_main_bank = bank_square*bank_square
        #test every tenth pixel in the line
#enable these tests once the functionality has been added under ticket #3403
        for i in range(centre_main_bank, end_main_bank, 10*bank_square):
            #first test the unmasked
            self.assertEqual(self.test_ws.readY(i-1)[0], 1)
            self.assertEqual(self.test_ws.readY(i+1)[0], 1)
            #now the masked
            self.assertEqual(self.test_ws.readY(i)[0], 1)

        ISIS.Mask('mask/line '+str(10)+' '+str(47))

    def tearDown(self):
        ISIS._refresh_singleton()
        
if __name__ == '__main__':
    unittest.main()