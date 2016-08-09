
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from reduction_workflow.instruments.sans import hfir_instrument
from reduction_workflow.instruments.sans import sns_instrument
import sys
import numpy as np

class SANSPatchSensitivity(PythonAlgorithm):
    """
       Calculate the detector sensitivity and patch the pixels that are masked in a second workspace.
       
       Patchs the InputWorkspace based on the mask difined in PatchWorkspace.
       For the masked peaks calculates a regression
       
    """

    def category(self):
        return "Workflow\\SANS"

    def name(self):
        return "SANSPatchSensitivity"

    def summary(self):
        return "Calculate the detector sensitivity and patch the pixels that are masked in a second workspace. "

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input),
                             doc = "Input sensitivity workspace to be patched")
        self.declareProperty(WorkspaceProperty("PatchWorkspace", "",
                                                     direction=Direction.Input),
                             doc = "Workspace defining the patch. Masked detectors will be patched.")
        self.declareProperty("ComponentName", "",
            doc="Component Name to apply the patch.")
        
        self.declareProperty("DegreeOfThePolynomial", defaultValue=1,
                             validator=IntBoundedValidator(0),
                             doc="Degree of the polynomial to fit the patch zone")
        
        self.declareProperty("OutputMessage", "",
                             direction=Direction.Output, doc="Output message")


    def PyExec(self):
        in_ws = self.getProperty("InputWorkspace").value
        patch_ws = self.getProperty("PatchWorkspace").value
        
        component = self.__get_component_to_patch(in_ws)
        number_of_tubes = component.nelements()
        
        for tube_idx in range(number_of_tubes):
            if component[0].nelements() <=1:
                # Handles EQSANS
                tube = component[tube_idx][0]
            else:
                # Handles Biosans/GPSANS
                tube = component[tube_idx]
            self.__patch_workspace(tube, in_ws, patch_ws)
        
        api.ClearMaskFlag(Workspace=in_ws)
        
    
    def __get_component_to_patch(self, workspace):
        '''
        Get the component name to apply the pacth
        Either from the field or from the IDF parameters
        '''
        instrument = workspace.getInstrument()
        component_name = self.getProperty("ComponentName").value
        
        # Get the default from the parameters file
        if component_name is None or component_name == "":
            component_name = instrument.getStringParameter('detector-name')[0]
        try:
            component = instrument.getComponentByName(component_name)
        except:
            Logger("SANSPatchSensitivity").error("Component not valid! %s" % component_name)
            return
        return component
    
    def __patch_workspace(self, tube_in_input_ws, in_ws, patch_ws):
        '''
        @param tube_in_input_ws :: Tube to patch
        @param in_ws: Workspace to patch
        @param patch_ws: where the mask is defined
        
        For every tube:
            In patch_ws : finds the masked pixels_ids
            In in_ws : Finds (id, Y, E) for the non-masked pixels in patch_ws
            Calculates the polynomial for the non-masked pixels
            Fits the  masked pixels_ids and sets Y and E in the in_ws
        '''
        
        #Arrays to calculate the polynomial
        id_to_calculate_fit = []
        y_to_calculate_fit = []
        e_to_calculate_fit = []
        # Array that will be fit
        id_to_fit =[]
        
        for pixel_idx in range(tube_in_input_ws.nelements()):
                pixel_in_input_ws = tube_in_input_ws[pixel_idx]
                # ID will be the same in both WS
                detector_id = pixel_in_input_ws.getID()
                pixel_in_patch_ws  = patch_ws.getDetector(detector_id)

                if pixel_in_patch_ws.isMasked():
                    id_to_fit.append(detector_id)
                elif not pixel_in_input_ws.isMasked():
                    id_to_calculate_fit.append(detector_id)
                    y_to_calculate_fit.append(in_ws.readY(detector_id).sum())
                    e_to_calculate_fit.append(in_ws.readE(detector_id).sum())
        
        degree = self.getProperty("DegreeOfThePolynomial").value
        # Returns coeffcients for the polynomial fit
        py =  np.polyfit(id_to_calculate_fit, y_to_calculate_fit, degree)
        pe =  np.polyfit(id_to_calculate_fit, e_to_calculate_fit, degree)
        
        for id in id_to_fit:
            vy = np.polyval(py,[id])
            ve = np.polyval(pe,[id])
            in_ws.setY(id,vy)
            in_ws.setE(id,ve)



AlgorithmFactory.subscribe(SANSPatchSensitivity())
