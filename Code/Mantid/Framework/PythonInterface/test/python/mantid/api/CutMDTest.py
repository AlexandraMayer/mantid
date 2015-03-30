import unittest
import testhelpers
import numpy as np
from mantid.simpleapi import *
from mantid.api import IMDHistoWorkspace, IMDEventWorkspace


class CutMDTest(unittest.TestCase):
    

    def setUp(self):
        # Create a workspace
        data_ws = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="A,B,C", Units="U,U,U")
        # Mark the workspace as being in HKL
        SetSpecialCoordinates(InputWorkspace=data_ws, SpecialCoordinates='HKL')
        # Set the UB
        SetUB(Workspace=data_ws, a = 1, b = 1, c = 1, alpha =90, beta=90, gamma = 90)
        # Add some data to the workspace
        FakeMDEventData(InputWorkspace=data_ws, PeakParams=[10000,0,0,0,1])
        self.__in_md  = data_ws
        
    def tearDown(self):
        DeleteWorkspace(self.__in_md )
        
    def test_orthogonal_slice_4D(self):
         # We create a fake 4-D workspace and check to see that using bin inputs for cropping works
        to_cut = CreateMDWorkspace(Dimensions=4, Extents=[-1,1,-1,1,-1,1,-10,10], Names='H,K,L,E', Units='U,U,U,V')
        # Set the UB
        SetUB(Workspace=to_cut, a = 1, b = 1, c = 1, alpha =90, beta=90, gamma = 90)
        SetSpecialCoordinates(InputWorkspace=to_cut, SpecialCoordinates='HKL')
        
        '''
        Process the 4D workspace
        '''
        out_md = CutMD(to_cut, P1Bin=[-0.5,0.5], P2Bin=[-0.1,0.1], P3Bin=[-0.3,0.3], P4Bin=[1],  NoPix=True)
        
        
        self.assertAlmostEqual(-0.5, out_md.getDimension(0).getMinimum(), 6) 
        self.assertAlmostEqual(0.5, out_md.getDimension(0).getMaximum(), 6) 
        self.assertAlmostEqual(-0.1, out_md.getDimension(1).getMinimum(), 6) 
        self.assertAlmostEqual(0.1, out_md.getDimension(1).getMaximum(), 6) 
        self.assertAlmostEqual(-0.3, out_md.getDimension(2).getMinimum(), 6) 
        self.assertAlmostEqual(0.3, out_md.getDimension(2).getMaximum(), 6)
        self.assertAlmostEqual(-10, out_md.getDimension(3).getMinimum(), 6) 
        self.assertAlmostEqual(10, out_md.getDimension(3).getMaximum(), 6)
        self.assertEqual(20, out_md.getDimension(3).getNBins())
        
        self.assertEquals("['zeta', 0, 0]",  out_md.getDimension(0).getName() )
        self.assertEquals("[0, 'eta', 0]",  out_md.getDimension(1).getName() )
        self.assertEquals("[0, 0, 'xi']",  out_md.getDimension(2).getName() )
        self.assertEquals("E",  out_md.getDimension(3).getName() )
        
        self.assertTrue(isinstance(out_md, IMDHistoWorkspace), "Expect that the output was an IMDHistoWorkspace given the NoPix flag.")
                
        '''
        Process the 4D workspace again, this time with different binning
        '''
        out_md = CutMD(to_cut, P1Bin=[-0.5,0.5], P2Bin=[-0.1,0.1], P3Bin=[-0.3,0.3], P4Bin=[-8,1,8],  NoPix=True) 
        self.assertEqual(16, out_md.getDimension(3).getNBins())
        self.assertTrue(isinstance(out_md, IMDHistoWorkspace), "Expect that the output was an IMDHistoWorkspace given the NoPix flag.")
                

if __name__ == '__main__':
    unittest.main()
