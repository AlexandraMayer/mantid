from collections import namedtuple
from SANS2.Common.SANSType import sans_type


# ----------------------------------------------------------------------------------------------------------------------
#  Named tuples for passing around data in a structured way, a bit like a plain old c-struct.
# ----------------------------------------------------------------------------------------------------------------------
# General
range_entry = namedtuple('range_entry', 'start, stop')
range_entry_with_detector = namedtuple('range_entry_with_detector', 'start, stop, detector_type')
single_entry_with_detector = namedtuple('range_entry_with_detector', 'entry, detector_type')

# Back
back_single_monitor_entry = namedtuple('back_single_monitor_entry', 'monitor, start, stop')

# Limits
mask_angle_entry = namedtuple('mask_angle_entry', 'min, max, use_mirror')
simple_range = namedtuple('simple_range', 'start, stop, step, step_type')
complex_range = namedtuple('complex_steps', 'start, step1, mid, step2, stop, step_type1, step_type2')
rebin_string_values = namedtuple('rebin_string_values', 'value')
event_binning_string_values = namedtuple('event_binning_string_values', 'value')

# Mask
mask_line = namedtuple('mask_line', 'width, angle, x, y')
mask_block = namedtuple('mask_block', 'horizontal1, horizontal2, vertical1, vertical2, detector_type')
mask_block_cross = namedtuple('mask_block_cross', 'horizontal, vertical, detector_type')

# Set
position_entry = namedtuple('position_entry', 'pos1, pos2, detector_type')
set_scales_entry = namedtuple('set_scales_entry', 's, a, b, c, d')

# Fit
range_entry_fit = namedtuple('range_entry_fit', 'start, stop, fit_type')
fit_general = namedtuple('fit_general', 'start, stop, fit_type, data_type, polynomial_order')

# Mon
monitor_length = namedtuple('monitor_length', 'length, spectrum, interpolate')
monitor_spectrum = namedtuple('monitor_spectrum', 'spectrum, is_trans, interpolate')
monitor_file = namedtuple('monitor_file', 'file_path, detector_type')


# ------------------------------------------------------------------
# --- State director keys ------------------------------------------
# ------------------------------------------------------------------

# --- DET
@sans_type("reduction_mode", "rescale", "shift", "rescale_fit", "shift_fit", "correction_x", "correction_y",
           "correction_z", "correction_rotation", "correction_radius", "correction_translation", "correction_x_tilt",
           "correction_y_tilt")
class DetectorId(object):
    pass


# --- LIMITS
@sans_type("angle", "events_binning", "events_binning_range", "radius_cut", "wavelength_cut", "radius", "q",
           "qxy", "wavelength")
class LimitsId(object):
    pass


# --- MASK
@sans_type("line", "time", "time_detector", "clear_detector_mask", "clear_time_mask", "single_spectrum_mask",
           "spectrum_range_mask", "vertical_single_strip_mask", "vertical_range_strip_mask", "file",
           "horizontal_single_strip_mask", "horizontal_range_strip_mask", "block", "block_cross")
class MaskId(object):
    pass


# --- SAMPLE
@sans_type("path", "offset")
class SampleId(object):
    pass


# --- SET
@sans_type("scales", "centre")
class SetId(object):
    pass


# --- TRANS
@sans_type("spec", "spec_shift", "radius", "roi", "mask", "sample_workspace", "can_workspace")
class TransId(object):
    pass


# --- TUBECALIBFILE
@sans_type("file")
class TubeCalibrationFileId(object):
    pass


# -- QRESOLUTION
@sans_type("on", "delta_r", "collimation_length", "a1", "a2", "h1", "w1", "h2", "w2", "moderator")
class QResolutionId(object):
    pass


# --- FIT
@sans_type("clear", "monitor_times", "general")
class FitId(object):
    pass


# --- GRAVITY
@sans_type("on_off", "extra_length")
class GravityId(object):
    pass


# --- MON
@sans_type("length", "direct", "flat", "hab", "spectrum", "spectrum_trans", "interpolate")
class MonId(object):
    pass


# --- PRINT
@sans_type("print_line")
class PrintId(object):
    pass


# -- BACK
@sans_type("all_monitors", "single_monitors", "monitor_off", "trans")
class BackId(object):
    pass


# -- OTHER - not settable in user file
@sans_type("reduction_dimensionality")
class OtherId(object):
    pass
