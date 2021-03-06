from __future__ import (absolute_import, division, print_function)
import os
import numpy as numpy
import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines import yaml_parser
from isis_powder.routines.RunDetails import RunDetails


def attenuate_workspace(attenuation_file_path, ws_to_correct):
    wc_attenuated = mantid.PearlMCAbsorption(attenuation_file_path)
    wc_attenuated = mantid.ConvertToHistogram(InputWorkspace=wc_attenuated, OutputWorkspace=wc_attenuated)
    wc_attenuated = mantid.RebinToWorkspace(WorkspaceToRebin=wc_attenuated, WorkspaceToMatch=ws_to_correct,
                                            OutputWorkspace=wc_attenuated)
    pearl_attenuated_ws = mantid.Divide(LHSWorkspace=ws_to_correct, RHSWorkspace=wc_attenuated)
    common.remove_intermediate_workspace(workspaces=wc_attenuated)
    return pearl_attenuated_ws


def apply_vanadium_absorb_corrections(van_ws, run_details):
    absorb_ws = mantid.Load(Filename=run_details.vanadium_absorption_path)

    van_original_units = van_ws.getAxis(0).getUnit().unitID()
    absorb_units = absorb_ws.getAxis(0).getUnit().unitID()
    if van_original_units != absorb_units:
        van_ws = mantid.ConvertUnits(InputWorkspace=van_ws, Target=absorb_units, OutputWorkspace=van_ws)

    van_ws = mantid.RebinToWorkspace(WorkspaceToRebin=van_ws, WorkspaceToMatch=absorb_ws, OutputWorkspace=van_ws)
    van_ws = mantid.Divide(LHSWorkspace=van_ws, RHSWorkspace=absorb_ws, OutputWorkspace=van_ws)

    if van_original_units != absorb_units:
        van_ws = mantid.ConvertUnits(InputWorkspace=van_ws, Target=van_original_units, OutputWorkspace=van_ws)

    common.remove_intermediate_workspace(absorb_ws)
    return van_ws


def generate_vanadium_absorb_corrections(van_ws):
    raise NotImplementedError("Generating absorption corrections needs to be implemented correctly")

    # TODO are these values applicable to all instruments
    shape_ws = mantid.CloneWorkspace(InputWorkspace=van_ws)
    mantid.CreateSampleShape(InputWorkspace=shape_ws, ShapeXML='<sphere id="sphere_1"> <centre x="0" y="0" z= "0" />\
                                                      <radius val="0.005" /> </sphere>')

    calibration_full_paths = None
    absorb_ws = \
        mantid.AbsorptionCorrection(InputWorkspace=shape_ws, AttenuationXSection="5.08",
                                    ScatteringXSection="5.1", SampleNumberDensity="0.072",
                                    NumberOfWavelengthPoints="25", ElementSize="0.05")
    mantid.SaveNexus(Filename=calibration_full_paths["vanadium_absorption"],
                     InputWorkspace=absorb_ws, Append=False)
    common.remove_intermediate_workspace(shape_ws)
    return absorb_ws


def get_run_details(run_number_string, inst_settings):
    mapping_dict = yaml_parser.get_run_dictionary(run_number=run_number_string, file_path=inst_settings.cal_map_path)

    calibration_file_name = mapping_dict["calibration_file"]
    empty_run_numbers = mapping_dict["empty_run_numbers"]
    label = mapping_dict["label"]
    vanadium_run_numbers = mapping_dict["vanadium_run_numbers"]

    splined_vanadium_name = _generate_splined_van_name(absorb_on=inst_settings.absorb_corrections,
                                                       vanadium_run_string=vanadium_run_numbers,
                                                       long_mode_on=inst_settings.long_mode)

    calibration_dir = inst_settings.calibration_dir
    cycle_calibration_dir = os.path.join(calibration_dir, label)

    absorption_file_path = os.path.join(calibration_dir, inst_settings.van_absorb_file)
    calibration_file_path = os.path.join(cycle_calibration_dir, calibration_file_name)
    tt_grouping_key = inst_settings.tt_mode.lower() + '_grouping'
    grouping_file_path = os.path.join(calibration_dir, getattr(inst_settings, tt_grouping_key))
    splined_vanadium_path = os.path.join(cycle_calibration_dir, splined_vanadium_name)

    run_details = RunDetails(run_number=run_number_string)
    run_details.calibration_file_path = calibration_file_path
    run_details.grouping_file_path = grouping_file_path
    run_details.empty_runs = empty_run_numbers
    run_details.label = label
    run_details.splined_vanadium_file_path = splined_vanadium_path
    run_details.vanadium_absorption_path = absorption_file_path
    run_details.vanadium_run_numbers = vanadium_run_numbers

    return run_details


def normalise_ws_current(ws_to_correct, monitor_ws, spline_coeff, lambda_values, integration_range):
    processed_monitor_ws = mantid.ConvertUnits(InputWorkspace=monitor_ws, Target="Wavelength")
    processed_monitor_ws = mantid.CropWorkspace(InputWorkspace=processed_monitor_ws,
                                                XMin=lambda_values[0], XMax=lambda_values[-1])
    ex_regions = numpy.zeros((2, 4))
    ex_regions[:, 0] = [3.45, 3.7]
    ex_regions[:, 1] = [2.96, 3.2]
    ex_regions[:, 2] = [2.1, 2.26]
    ex_regions[:, 3] = [1.73, 1.98]

    for reg in range(0, 4):
        processed_monitor_ws = mantid.MaskBins(InputWorkspace=processed_monitor_ws, XMin=ex_regions[0, reg],
                                               XMax=ex_regions[1, reg])

    splined_monitor_ws = mantid.SplineBackground(InputWorkspace=processed_monitor_ws,
                                                 WorkspaceIndex=0, NCoeff=spline_coeff)

    normalised_ws = mantid.ConvertUnits(InputWorkspace=ws_to_correct, Target="Wavelength", OutputWorkspace=ws_to_correct)
    normalised_ws = mantid.NormaliseToMonitor(InputWorkspace=normalised_ws, MonitorWorkspace=splined_monitor_ws,
                                              IntegrationRangeMin=integration_range[0],
                                              IntegrationRangeMax=integration_range[-1],
                                              OutputWorkspace=normalised_ws)

    normalised_ws = mantid.ConvertUnits(InputWorkspace=normalised_ws, Target="TOF", OutputWorkspace=normalised_ws)

    common.remove_intermediate_workspace(processed_monitor_ws)
    common.remove_intermediate_workspace(splined_monitor_ws)

    return normalised_ws


def _generate_splined_van_name(absorb_on, long_mode_on, vanadium_run_string):
    output_string = "SVan_" + str(vanadium_run_string)
    output_string += "_absorb" if absorb_on else ""
    output_string += "_long" if long_mode_on else ""

    output_string += ".nxs"
    return output_string
