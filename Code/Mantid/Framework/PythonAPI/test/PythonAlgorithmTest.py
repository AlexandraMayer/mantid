import unittest

from MantidFramework import mtd
from MantidFramework import PythonAlgorithm
mtd.initialise()
from mantidsimple import *

class DummyAlg(PythonAlgorithm):
    """
        Dummy algorithm that uses executeSubAlg()
    """
     
    def category(self):
        return "SANS"
    
    def name(self):
        return "DummyAlg"
    
    def PyInit(self):
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction = Direction.Output, Description = '')
    
    def PyExec(self):
        output_ws = self.getPropertyValue("OutputWorkspace")
        x = [0,1,2]
        y = [0,1,2]
        e = [0,0,0]
        a = self.executeSubAlg(CreateWorkspace, output_ws, x, y, e)
        self._setWorkspaceProperty("OutputWorkspace", a._getWorkspaceProperty("OutputWorkspace"))


class PythonAlgorithmTest(unittest.TestCase):
    """
        Tests for python algorithms
    """
    
    def setUp(self):
        pass
        
    def test_sub_alg_wksp_transfer(self):
        """
            Check that we can execute a sub-algorithm and pass
            ownership of an output workspace to the parent algo. 
        """
        algm = DummyAlg()
        algm.initialize()
        algm.setPropertyValue("OutputWorkspace", "subalgtest")
        algm.setRethrows(True)
        algm.execute()
        self.assertTrue(mtd.workspaceExists("subalgtest"))
        mtd.deleteWorkspace("subalgtest")
    
    def test_sub_alg_variation(self):
        """
            Call signature variation for sub-algorithm execution
        """
        class DummyAlg2(DummyAlg):
            def name(self):
                return "DummyAlg2"
            def PyExec(self):
                output_ws = self.getPropertyValue("OutputWorkspace")
                a = self.executeSubAlg(CreateWorkspace, OutputWorkspace=output_ws, DataX=[0,1,2], DataY=[0,1,2], DataE=[0,0,0])
                self._setWorkspaceProperty("OutputWorkspace", a._getWorkspaceProperty("OutputWorkspace"))
        algm = DummyAlg2()
        algm.initialize()
        algm.setPropertyValue("OutputWorkspace", "subalgtest2")
        algm.setRethrows(True)
        algm.execute()
        self.assertTrue(mtd.workspaceExists("subalgtest2"))
        mtd.deleteWorkspace("subalgtest2")

if __name__ == '__main__':
    unittest.main()
