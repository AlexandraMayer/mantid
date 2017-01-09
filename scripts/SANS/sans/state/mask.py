# pylint: disable=too-few-public-methods

"""State describing the masking behaviour of the SANS reduction."""

import json
import copy
from sans.state.state_base import (StateBase, BoolParameter, StringListParameter, StringParameter,
                                   PositiveFloatParameter, FloatParameter, FloatListParameter,
                                   DictParameter, PositiveIntegerListParameter, rename_descriptor_names)
from sans.state.state_functions import (is_pure_none_or_not_none, validation_message, set_detector_names)
from sans.state.automatic_setters import (automatic_setters)
from sans.common.file_information import find_full_file_path
from sans.common.enums import (SANSInstrument, DetectorType)
from sans.common.file_information import (get_instrument_paths_for_sans_file)


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
def range_check(start, stop, invalid_dict, start_name, stop_name, general_name=None):
    """
    Checks a start container against a stop container

    :param start: The start container
    :param stop:  the stop container
    :param invalid_dict: The invalid dict to which we write our error messages
    :param start_name: The name of the start container
    :param stop_name: The name of the stop container
    :param general_name: The general name of this container family
    :return: A (potentially) updated invalid_dict
    """
    if not is_pure_none_or_not_none([start, stop]):
        entry = validation_message("A range element has not been set.",
                                   "Make sure that all entries are set.",
                                   {start_name: start,
                                    stop_name: stop})
        invalid_dict.update(entry)

    if start is not None and stop is not None:
        # Start and stop need to have the same length
        if len(start) != len(stop):
            entry = validation_message("Start and stop ranges have different lengths.",
                                       "Make sure that all entries for {0} he same length.".format(general_name),
                                       {start_name: start,
                                        stop_name: stop})
            invalid_dict.update(entry)
        # Start values need to be smaller than the stop values
        for a, b in zip(start, stop):
            if a > b:
                entry = validation_message("Incorrect start-stop bounds.",
                                           "Make sure the lower bound is smaller than the upper bound for {0}."
                                           "".format(general_name),
                                           {start_name: start,
                                            stop_name: stop})
                invalid_dict.update(entry)
    return invalid_dict


# ------------------------------------------------
# StateData
# ------------------------------------------------
@rename_descriptor_names
class StateMaskDetector(StateBase):
    # Vertical strip masks
    single_vertical_strip_mask = PositiveIntegerListParameter()
    range_vertical_strip_start = PositiveIntegerListParameter()
    range_vertical_strip_stop = PositiveIntegerListParameter()

    # Horizontal strip masks
    single_horizontal_strip_mask = PositiveIntegerListParameter()
    range_horizontal_strip_start = PositiveIntegerListParameter()
    range_horizontal_strip_stop = PositiveIntegerListParameter()

    # Spectrum Block
    block_horizontal_start = PositiveIntegerListParameter()
    block_horizontal_stop = PositiveIntegerListParameter()
    block_vertical_start = PositiveIntegerListParameter()
    block_vertical_stop = PositiveIntegerListParameter()

    # Spectrum block cross
    block_cross_horizontal = PositiveIntegerListParameter()
    block_cross_vertical = PositiveIntegerListParameter()

    # Time/Bin mask
    bin_mask_start = FloatListParameter()
    bin_mask_stop = FloatListParameter()

    # Name of the detector
    detector_name = StringParameter()
    detector_name_short = StringParameter()

    def __init__(self):
        super(StateMaskDetector, self).__init__()

    def validate(self):
        is_invalid = {}
        # --------------------
        # Vertical strip mask
        # --------------------
        range_check(self.range_vertical_strip_start, self.range_vertical_strip_stop,
                    is_invalid, "range_vertical_strip_start", "range_vertical_strip_stop", "range_vertical_strip")

        # --------------------
        # Horizontal strip mask
        # --------------------
        range_check(self.range_horizontal_strip_start, self.range_horizontal_strip_stop,
                    is_invalid, "range_horizontal_strip_start", "range_horizontal_strip_stop", "range_horizontal_strip")

        # --------------------
        # Block mask
        # --------------------
        range_check(self.block_horizontal_start, self.block_horizontal_stop,
                    is_invalid, "block_horizontal_start", "block_horizontal_stop", "block_horizontal")
        range_check(self.block_vertical_start, self.block_vertical_stop,
                    is_invalid, "block_vertical_start", "block_vertical_stop", "block_vertical")

        # --------------------
        # Time/Bin mask
        # --------------------
        range_check(self.bin_mask_start, self.bin_mask_stop,
                    is_invalid, "bin_mask_start", "bin_mask_stop", "bin_mask")

        if not self.detector_name:
            entry = validation_message("Missing detector name.",
                                       "Make sure that the detector names are set.",
                                       {"detector_name": self.detector_name})
            is_invalid.update(entry)
        if not self.detector_name_short:
            entry = validation_message("Missing short detector name.",
                                       "Make sure that the short detector names are set.",
                                       {"detector_name_short": self.detector_name_short})
            is_invalid.update(entry)
        if is_invalid:
            raise ValueError("StateMoveDetectorISIS: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


@rename_descriptor_names
class StateMask(StateBase):
    # Radius Mask
    radius_min = FloatParameter()
    radius_max = FloatParameter()

    # Bin mask
    bin_mask_general_start = FloatListParameter()
    bin_mask_general_stop = FloatListParameter()

    # Mask files
    mask_files = StringListParameter()

    # Angle masking
    phi_min = FloatParameter()
    phi_max = FloatParameter()
    use_mask_phi_mirror = BoolParameter()

    # Beam stop
    beam_stop_arm_width = PositiveFloatParameter()
    beam_stop_arm_angle = FloatParameter()
    beam_stop_arm_pos1 = FloatParameter()
    beam_stop_arm_pos2 = FloatParameter()

    # Clear commands
    clear = BoolParameter()
    clear_time = BoolParameter()

    # Single Spectra
    single_spectra = PositiveIntegerListParameter()

    # Spectrum Range
    spectrum_range_start = PositiveIntegerListParameter()
    spectrum_range_stop = PositiveIntegerListParameter()

    # The detector dependent masks
    detectors = DictParameter()

    # The idf path of the instrument
    idf_path = StringParameter()

    def __init__(self):
        super(StateMask, self).__init__()
        # Setup the detectors
        self.detectors = {DetectorType.to_string(DetectorType.LAB): StateMaskDetector(),
                          DetectorType.to_string(DetectorType.HAB): StateMaskDetector()}

        # IDF Path
        self.idf_path = ""

    def validate(self):
        is_invalid = dict()

        # --------------------
        # Radius Mask
        # --------------------
        # Radius mask rule: the min radius must be less or equal to the max radius
        if self.radius_max is not None and self.radius_min is not None and\
           self.radius_max != -1 and self.radius_min != -1:  # noqa
            if self.radius_min > 0 and self.radius_max > 0 and (self.radius_min > self.radius_max):
                entry = validation_message("Incorrect radius bounds.",
                                           "Makes sure that the lower radius bound is smaller than the"
                                           " upper radius bound.",
                                           {"radius_min": self.radius_min,
                                            "radius_max": self.radius_max})
                is_invalid.update(entry)

        # --------------------
        # General bin mask
        # --------------------
        range_check(self.bin_mask_general_start, self.bin_mask_general_stop,
                    is_invalid, "bin_mask_general_start", "bin_mask_general_stop", "bin_mask_general")

        # --------------------
        # Mask files
        # --------------------
        if self.mask_files:
            for mask_file in self.mask_files:
                if not find_full_file_path(mask_file):
                    entry = validation_message("Mask file not found.",
                                               "Makes sure that the mask file is in your path",
                                               {"mask_file": self.mask_files})
                    is_invalid.update(entry)

        # --------------------
        # Spectrum Range
        # --------------------
        range_check(self.spectrum_range_start, self.spectrum_range_stop,
                    is_invalid, "spectrum_range_start", "spectrum_range_stop", "spectrum_range")

        # --------------------
        # Detectors
        # --------------------
        for _, value in self.detectors.items():
            value.validate()

        if is_invalid:
            raise ValueError("StateMask: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
def setup_idf_and_ipf_content(move_info, data_info):
    # Get the IDF and IPF path since they contain most of the import information
    file_name = data_info.sample_scatter
    idf_path, ipf_path = get_instrument_paths_for_sans_file(file_name)
    # Set the detector names
    set_detector_names(move_info, ipf_path)
    # Set the idf path
    move_info.idf_path = idf_path


class StateMaskBuilder(object):
    @automatic_setters(StateMask)
    def __init__(self, data_info):
        super(StateMaskBuilder, self).__init__()
        self._data = data_info
        self.state = StateMask()
        setup_idf_and_ipf_content(self.state, data_info)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


def get_mask_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return StateMaskBuilder(data_info)
    else:
        raise NotImplementedError("StateMaskBuilder: Could not find any valid mask builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
