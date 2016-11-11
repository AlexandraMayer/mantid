# pylint: disable=too-few-public-methods

""" Multiplies a SANS workspace by an absolute scale and divides it by the sample volume. """

from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator,
                           FloatArrayProperty)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)

from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Scale.ScaleHelpers import (DivideByVolumeFactory, MultiplyByAbsoluteScaleFactory)


class SANSScale(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Scale'

    def summary(self):
        return 'Multiplies a SANS workspace by an absolute scale and divides it by the sample volume.'

    def PyInit(self):
        # State
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.input_workspace, '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The input workspace')

        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.output_workspace, '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc='The scaled output workspace')

        self.declareProperty("IsCan", False, direction=Direction.Input,
                             doc="Set true if the input is a can workspace, else false.")

    def PyExec(self):
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        # Get the correct SANS move strategy from the SANSMaskFactory
        workspace = self.getProperty(SANSConstants.input_workspace).value

        progress = Progress(self, start=0.0, end=1.0, nreports=3)

        # Divide by the sample volume
        progress.report("Dividing by the sample volume.")
        is_can = self.getProperty("IsCan").value
        workspace = self._divide_by_volume(workspace, state, is_can)

        # Multiply by the absolute scale
        progress.report("Applying absolute scale.")
        workspace = self._multiply_by_absolute_scale(workspace, state)

        self.setProperty(SANSConstants.output_workspace, workspace)
        progress.report("Finished applying absolute scale")

    def _divide_by_volume(self, workspace, state, is_can):
        divide_factory = DivideByVolumeFactory()
        divider = divide_factory.create_divide_by_volume(state, is_can)
        scale_info = state.scale
        return divider.divide_by_volume(workspace, scale_info)

    def _multiply_by_absolute_scale(self, workspace, state):
        multiply_factory = MultiplyByAbsoluteScaleFactory()
        multiplier = multiply_factory.create_multiply_by_absolute(state)
        scale_info = state.scale
        return multiplier.multiply_by_absolute_scale(workspace, scale_info)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSScale)
