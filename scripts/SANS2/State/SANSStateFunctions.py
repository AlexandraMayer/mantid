from SANS2.Common.SANSType import (ReductionDimensionality, ISISReductionMode)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSFunctions import add_to_sample_log


def add_workspace_name(workspace, state, reduction_mode):
    """
    Adds the default reduced workspace name to the sample logs

    :param workspace: The output workspace
    :param state: a SANSState object
    :param reduction_mode: the reduction mode, i.e. LAB, HAB, MERGED
    """
    reduced_workspace_name = get_output_workspace_name(state, reduction_mode)
    add_to_sample_log(workspace, SANSConstants.reduced_workspace_name_in_logs, reduced_workspace_name, "String")


def get_output_workspace_name_from_workspace(workspace):
    run = workspace.run()
    if not run.hasProperty(SANSConstants.reduced_workspace_name_in_logs):
        raise RuntimeError("The workspace does not seem to contain an entry for the output workspace name.")
    return run.getProperty(SANSConstants.reduced_workspace_name_in_logs).value


def get_output_workspace_name(state, reduction_mode):
    """
    Creates the name of the output workspace from a state object.

    The name of the output workspace is:
    1. The short run number
    2. If specific period is being reduced: 'p' + number
    3. Short detector name of the current reduction or "merged"
    4. The reduction dimensionality: "_" + dimensionality
    5. A wavelength range: wavelength_low + "_" + wavelength_high
    6. In case of a 1D reduction, then add phi limits
    7. If we are dealing with an actual slice limit, then specify it: "_tXX_TYY" Note that the time set to
       two decimals
    :param state: a SANSState object
    :param reduction_mode: which reduction is being looked at
    :return: the name of the reduced workspace
    """
    # 1. Short run number
    data = state.data
    short_run_number = data.sample_scatter_run_number
    short_run_number_as_string = str(short_run_number)

    # 2. Multiperiod
    if state.data.sample_scatter_period != SANSConstants.ALL_PERIODS:
        period = data.sample_scatter_period
        period_as_string = "p"+str(period)
    else:
        period_as_string = ""

    # 3. Detector name
    move = state.move
    detectors = move.detectors
    if reduction_mode is ISISReductionMode.Merged:
        detector_name_short = "merged"
    elif reduction_mode is ISISReductionMode.Hab:
        detector_name_short = detectors[SANSConstants.high_angle_bank].detector_name_short
    elif reduction_mode is ISISReductionMode.Lab:
        detector_name_short = detectors[SANSConstants.low_angle_bank].detector_name_short
    else:
        raise RuntimeError("SANSStateFunctions: Unknown reduction mode {0} cannot be used to "
                           "create an output name".format(reduction_mode))

    # 4. Dimensionality
    reduction = state.reduction
    if reduction.dimensionality is ReductionDimensionality.OneDim:
        dimensionality_as_string = "_1D"
    else:
        dimensionality_as_string = "_2D"

    # 5. Wavelength range
    wavelength = state.wavelength
    wavelength_range_string = str(wavelength.wavelength_low) + "_" + str(wavelength.wavelength_high)

    # 6. Phi Limits
    mask = state.mask
    if reduction.dimensionality is ReductionDimensionality.OneDim:
        if mask.phi_min and mask.phi_max and (abs(mask.phi_max - mask.phi_min) != 180.0):
            phi_limits_as_string = 'Phi' + str(mask.phi_min) + '_' + str(mask.phi_max)
        else:
            phi_limits_as_string = ""
    else:
        phi_limits_as_string = ""

    # 7. Slice limits
    slice_state = state.slice
    start_time = slice_state.start_time
    end_time = slice_state.end_time
    if start_time and end_time:
        start_time_as_string = '_t%.2f' % start_time[0]
        end_time_as_string = '_T%.2f' % end_time[0]
    else:
        start_time_as_string = ""
        end_time_as_string = ""

    # Piece it all together
    output_workspace_name = (short_run_number_as_string + period_as_string + detector_name_short +
                             dimensionality_as_string + wavelength_range_string + phi_limits_as_string +
                             start_time_as_string + end_time_as_string)
    return output_workspace_name


def is_pure_none_or_not_none(elements_to_check):
    """
    Checks a list of elements contains None entries and non-None entries

    @param elements_to_check: a list with entries to check
    @return: True if the list contains either only None or only non-None elements, else False
    """
    are_all_none_or_all_not_none = True

    if len(elements_to_check) == 0:
        return are_all_none_or_all_not_none
    return all(element is not None for element in elements_to_check) or \
           all(element is None for element in elements_to_check)


def is_not_none_and_first_larger_than_second(elements_to_check):
    """
    This function checks if both are not none and then checks if the first element is smaller than the second element.

    @param elements_to_check: a list with two entries. The first is the lower bound and the second entry is the upper
                              bound
    @return: False if at least one input is None or if both are not None and the first element is smaller than the
             second else True
    """
    is_invalid = True
    if len(elements_to_check) != 2:
        return is_invalid
    if any(element is None for element in elements_to_check):
        is_invalid = False
        return is_invalid
    if elements_to_check[0] < elements_to_check[1]:
        is_invalid = False
    return is_invalid


def one_is_none(elements_to_check):
    return any(element is None for element in elements_to_check)


def validation_message(error_message, instruction, variables):
    message = ""
    for key, value in variables.items():
        message += "{0}: {1}\n".format(key, value)
    message += instruction
    return {error_message: message}