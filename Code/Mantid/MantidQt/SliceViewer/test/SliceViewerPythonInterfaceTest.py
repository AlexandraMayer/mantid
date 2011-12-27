import sys
import sys
import os
import unittest
import time
from PyQt4 import Qt
#from PyQt4.QtTest import QTest
        
# Import the Mantid framework
import MantidFramework
from MantidFramework import mtd
from mantidsimple import *
import mantidqtpython

from mantidqtpython import StdRuntimeError, StdInvalidArgument

# Create the application only once per test; otherwise I get a segfault
app = Qt.QApplication(sys.argv)


class SliceViewerPythonInterfaceTest(unittest.TestCase):
    """Test for accessing SliceViewer widgets from MantidPlot
    python interpreter"""
    
    def setUp(self):
        """ Set up and create a SliceViewer widget """
        # Create a test data set
        CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z', 
            Units='m,m,m',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='mdw')
        FakeMDEventData("mdw",  UniformParams="1e4")
        FakeMDEventData("mdw",  PeakParams="1e3, 1, 2, 3, 1.0")
        BinMD("mdw", "uniform",  AxisAligned=1, AlignedDimX="x,0,10,30",  AlignedDimY="y,0,10,30",  AlignedDimZ="z,0,10,30", IterateEvents="1", Parallel="0")
        CreateWorkspace('workspace2d', '1,2,3', '2,3,4')
        CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z', Units='m,m,m',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='empty')
        # Get the factory to create the SliceViewerWindow in C++
        self.svw = mantidqtpython.MantidQt.Factory.WidgetFactory.Instance().createSliceViewerWindow("uniform", "")
        # Retrieve the SliceViewer widget alone.
        self.sv = self.svw.getSlicer()
        pass
    
    def setUpXML(self):
        """Special set up for the XML version """
        CreateMDWorkspace(Dimensions='3',Extents='-15,15, -15,15, -15,15',Names='Q_lab_x,Q_lab_y,Q_lab_z', 
            Units='m,m,m',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='TOPAZ_3680')
        CreateMDWorkspace(Dimensions='4',Extents='-15,15, -15,15, -15,15, -10, 100',Names='Q_x,Q_y,Q_z,E', 
            Units='A,A,A,meV',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='WS_4D')
        FakeMDEventData("TOPAZ_3680",  UniformParams="1e4")
        FakeMDEventData("WS_4D",  UniformParams="1e4")
        
        self.xml_3d = """<MDInstruction><MDWorkspaceName>TOPAZ_3680</MDWorkspaceName>
<DimensionSet>
    <Dimension ID="Q_lab_x"><Name>Q_lab_x</Name><Units>Angstroms^-1</Units><UpperBounds>15.0000</UpperBounds><LowerBounds>-15.0000</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="Q_lab_y"><Name>Q_lab_y</Name><Units>Angstroms^-1</Units><UpperBounds>15.0000</UpperBounds><LowerBounds>-15.0000</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="Q_lab_z"><Name>Q_lab_z</Name><Units>Angstroms^-1</Units><UpperBounds>15.0000</UpperBounds><LowerBounds>-15.0000</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <XDimension><RefDimensionId>Q_lab_x</RefDimensionId></XDimension>
    <YDimension><RefDimensionId>Q_lab_y</RefDimensionId></YDimension>
    <ZDimension><RefDimensionId>Q_lab_z</RefDimensionId></ZDimension>
    <TDimension><RefDimensionId/></TDimension>
</DimensionSet>
<Function><Type>PlaneImplicitFuction</Type>
<ParameterList>
    <Parameter><Type>NormalParameter</Type><Value>1 0 0</Value></Parameter>
    <Parameter><Type>OriginParameter</Type><Value>4.84211 0 0</Value></Parameter>
</ParameterList></Function>
</MDInstruction>"""

        self.xml_4d = """<MDInstruction><MDWorkspaceName>WS_4D</MDWorkspaceName>
<DimensionSet>
    <Dimension ID="Q_x"><Name>Q_x</Name><Units>Ang</Units><UpperBounds>5.7415</UpperBounds><LowerBounds>-1.5197</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="Q_y"><Name>Q_y</Name><Units>Ang</Units><UpperBounds>6.7070</UpperBounds><LowerBounds>-6.6071</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="Q_z"><Name>Q_z</Name><Units>Ang</Units><UpperBounds>6.6071</UpperBounds><LowerBounds>-6.6071</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <Dimension ID="E"><Name>E</Name><Units>MeV</Units><UpperBounds>150.0000</UpperBounds><LowerBounds>0.0000</LowerBounds><NumberOfBins>10</NumberOfBins></Dimension>
    <XDimension><RefDimensionId>Q_x</RefDimensionId></XDimension>
    <YDimension><RefDimensionId>Q_y</RefDimensionId></YDimension>
    <ZDimension><RefDimensionId>E</RefDimensionId></ZDimension>
    <TDimension><RefDimensionId>Q_z</RefDimensionId><Value>4.567</Value></TDimension>
</DimensionSet>
<Function><Type>PlaneImplicitFuction</Type><ParameterList>
    <Parameter><Type>NormalParameter</Type><Value>0 1 0</Value></Parameter>
    <Parameter><Type>OriginParameter</Type><Value>0 1.234 0</Value></Parameter>
</ParameterList></Function></MDInstruction>"""


    
    def tearDown(self):
        """ Close the created widget """
        # This is crucial! Forces the object to be deleted NOW, not when python exits
        # This prevents a segfault in Ubuntu 10.04, and is good practice.
        self.svw.deleteLater()
        #self.svw.show()
        # Schedule quit at the next event
        Qt.QTimer.singleShot(0, app, Qt.SLOT("quit()"))
        # This is required for deleteLater() to do anything (it deletes at the next event loop)
        app.quitOnLastWindowClosed = True
       	app.exec_()


    #==========================================================================
    #======================= Basic Tests ======================================
    #==========================================================================

    def test_setWorkspace(self):
        sv = self.sv
        assert (sv is not None) 
    
    def test_getWorkspace(self):
        sv = self.sv
        self.assertEqual(sv.getWorkspaceName(), "uniform")
        assert (sv is not None)
        
    def test_setWorkspace_MDEventWorkspace(self):
        sv = self.sv
        sv.setWorkspace('mdw')
    
    def test_setWorkspace_throwsOnBadInputs(self):
        sv = self.sv
        #sv.setWorkspace('workspace2d')
        self.assertRaises(StdRuntimeError, sv.setWorkspace, '')
        self.assertRaises(StdRuntimeError, sv.setWorkspace, 'non_existent_workspace')
        self.assertRaises(StdRuntimeError, sv.setWorkspace, 'workspace2d')
    
    #==========================================================================
    #======================= XML Tests ======================================
    #==========================================================================
    def test_openFromXML_3D(self):
        sv = self.sv
        self.setUpXML()
        # Read the XML and set the view
        sv.openFromXML(self.xml_3d)
        # Check the settings
        self.assertEqual(sv.getWorkspaceName(), "TOPAZ_3680")
        self.assertEqual(sv.getDimX(), 1)
        self.assertEqual(sv.getDimY(), 2)
        self.assertAlmostEqual( sv.getSlicePoint(0), 4.84211, 3)
        pass
    
    def test_openFromXML_4D(self):
        sv = self.sv
        self.setUpXML()

        # Read the XML and set the view
        sv.openFromXML(self.xml_4d)
        # Check the settings
        self.assertEqual(sv.getWorkspaceName(), "WS_4D")
        self.assertEqual(sv.getDimX(), 0) # Q_x is X dimension
        self.assertEqual(sv.getDimY(), 3) # Energy is Y
        self.assertAlmostEqual( sv.getSlicePoint(1), 1.234, 3) # Slice point in Q_y
        self.assertAlmostEqual( sv.getSlicePoint(2), 4.567, 3) # Slice point in Q_z
        
    def test_openFromXML_3D_binned(self):
        sv = self.sv
        self.setUpXML()
        BinMD(InputWorkspace="TOPAZ_3680", OutputWorkspace="__TOPAZ_3680_mdhisto",
              AxisAligned=1, AlignedDimX="Q_lab_x,0,10,20", AlignedDimY="Q_lab_y,0,10,20", AlignedDimZ="Q_lab_z,0,10,20")
        # Read the XML and set the view
        sv.openFromXML(self.xml_3d)
        # Check the settings
        # Automatically grabbed the histo version
        self.assertEqual(sv.getWorkspaceName(), "__TOPAZ_3680_mdhisto")
        self.assertEqual(sv.getDimX(), 1)
        self.assertEqual(sv.getDimY(), 2)
        self.assertAlmostEqual( sv.getSlicePoint(0), 4.84211, 3)
        pass
    

    #==========================================================================
    #======================= Setting Dimensions, etc ==========================
    #==========================================================================
    def test_setXYDim(self):
        sv = self.sv
        sv.setXYDim(0,2)
        self.assertEqual( sv.getDimX(), 0, "X dimension was set")
        self.assertEqual( sv.getDimY(), 2, "Y dimension was set")
        #sv.show()
        #app.exec_()
        
    def test_setXYDim_strings(self):
        sv = self.sv
        sv.setXYDim("x", "z")
        self.assertEqual( sv.getDimX(), 0, "X dimension was set")
        self.assertEqual( sv.getDimY(), 2, "Y dimension was set")
        
    def test_setXYDim_strings_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdRuntimeError, sv.setXYDim, "monkey", "y")
        self.assertRaises(StdRuntimeError, sv.setXYDim, "x", "monkey")
        
            
    def test_setXYDim_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdInvalidArgument, sv.setXYDim, -1, 0)
        self.assertRaises(StdInvalidArgument, sv.setXYDim, 5, 0)
        self.assertRaises(StdInvalidArgument, sv.setXYDim, 0, -1)
        self.assertRaises(StdInvalidArgument, sv.setXYDim, 0, 3)
        self.assertRaises(StdInvalidArgument, sv.setXYDim, 0, 0)
        
    def test_setSlicePoint(self):
        sv = self.sv
        # Set the slice point and got back the value?
        sv.setSlicePoint(2, 7.6)
        self.assertAlmostEqual( sv.getSlicePoint(2), 7.6, 2)
        # Go to too small a value
        sv.setSlicePoint(2, -12.3)
        self.assertAlmostEqual( sv.getSlicePoint(2), 0.0, 2)
        # Go to too big a value
        sv.setSlicePoint(2, 22.3)
        self.assertAlmostEqual( sv.getSlicePoint(2), 10.0, 2)
        
    def test_setSlicePoint_strings(self):
        sv = self.sv
        sv.setSlicePoint("z", 7.6)
        self.assertAlmostEqual( sv.getSlicePoint("z"), 7.6, 2)
        
    def test_setSlicePoint_strings_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdRuntimeError, sv.setSlicePoint, "monkey", 2.34)
        self.assertRaises(StdRuntimeError, sv.getSlicePoint, "monkey")
                
    def test_setSlicePoint_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdInvalidArgument, sv.setSlicePoint, -1, 7.6)
        self.assertRaises(StdInvalidArgument, sv.setSlicePoint, 3, 7.6)
                    
    def test_getSlicePoint_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdInvalidArgument, sv.getSlicePoint, -1)
        self.assertRaises(StdInvalidArgument, sv.getSlicePoint, 3)
        
    def test_setXYLimits(self):
        sv = self.sv
        sv.setXYLimits(5,10, 7,8)
        sv.setXYLimits(5,2, 7, 8)
        self.assertEqual(sv.getXLimits(), [5, 2])
        self.assertEqual(sv.getYLimits(), [7, 8])
        #sv.show()
        #app.exec_()
                
    def test_zoomBy(self):
        sv = self.sv
        self.assertEqual(sv.getXLimits(), [0, 10])
        self.assertEqual(sv.getYLimits(), [0, 10])
        # Zoom in by a factor of 2
        sv.zoomBy(2.0)
        self.assertEqual(sv.getXLimits(), [2.5, 7.5])
        self.assertEqual(sv.getYLimits(), [2.5, 7.5])
        # Zoom out to the original size
        sv.zoomBy(0.5)
        self.assertEqual(sv.getXLimits(), [0, 10])
        self.assertEqual(sv.getYLimits(), [0, 10])
                                
    def test_setXYCenter(self):
        sv = self.sv
        self.assertEqual(sv.getXLimits(), [0, 10])
        self.assertEqual(sv.getYLimits(), [0, 10])
        # Move to a new spot
        sv.setXYCenter(2.0, 6.0)
        self.assertEqual(sv.getXLimits(), [-3, 7])
        self.assertEqual(sv.getYLimits(), [1, 11])
        
    def test_resetZoom(self):
        sv = self.sv
        sv.zoomBy(2.0)
        self.assertEqual(sv.getXLimits(), [2.5, 7.5])
        self.assertEqual(sv.getYLimits(), [2.5, 7.5])
        # Go back automatically to full range
        sv.resetZoom()
        self.assertEqual(sv.getXLimits(), [0, 10])
        self.assertEqual(sv.getYLimits(), [0, 10])
                
    #==========================================================================
    #======================= ColorMap and range ===============================
    #==========================================================================
    def test_loadColorMap(self):
        """ Needs an absolute path - can't readily do unit test """
        sv = self.sv
        #sv.loadColorMap('')
        
    def test_setColorScale(self):
        sv = self.sv
        sv.setColorScale(10, 30, False)
        self.assertEqual(sv.getColorScaleMin(), 10)
        self.assertEqual(sv.getColorScaleMax(), 30)
        self.assertEqual(sv.getColorScaleLog(), False)
        sv.setColorScale(20, 1000, True)
        self.assertEqual(sv.getColorScaleMin(), 20)
        self.assertEqual(sv.getColorScaleMax(), 1000)
        self.assertEqual(sv.getColorScaleLog(), True)
                    
    def test_setColorScale_throwsOnBadInputs(self):
        sv = self.sv
        self.assertRaises(StdInvalidArgument, sv.setColorScale, 10, 5, False)
        self.assertRaises(StdInvalidArgument, sv.setColorScale, 0, 5, True)
        self.assertRaises(StdInvalidArgument, sv.setColorScale, -3, -1, True)
                   
    def test_setColorScaleAutoFull(self):
        sv = self.sv
        sv.setColorScaleAutoFull()
        self.assertEqual(sv.getColorScaleMin(), 27.0)
        self.assertEqual(sv.getColorScaleMax(), 540.0)
                   
    def test_setColorScaleAutoSlice(self):
        sv = self.sv
        sv.setColorScaleAutoSlice()
        self.assertEqual(sv.getColorScaleMin(), 27.0)
        self.assertEqual(sv.getColorScaleMax(), 81.0)
            
    