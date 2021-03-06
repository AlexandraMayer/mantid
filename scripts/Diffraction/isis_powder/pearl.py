from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.routines import common, InstrumentSettings, yaml_parser
from isis_powder.routines.common_enums import InputBatchingEnum
from isis_powder.abstract_inst import AbstractInst
from isis_powder.pearl_routines import pearl_algs, pearl_output, pearl_advanced_config, pearl_param_mapping


class Pearl(AbstractInst):
    def __init__(self, **kwargs):
        basic_config_dict = yaml_parser.open_yaml_file_as_dictionary(kwargs.get("config_file", None))

        self._inst_settings = InstrumentSettings.InstrumentSettings(
           attr_mapping=pearl_param_mapping.attr_mapping, adv_conf_dict=pearl_advanced_config.get_all_adv_variables(),
           basic_conf_dict=basic_config_dict, kwargs=kwargs)

        super(Pearl, self).__init__(user_name=self._inst_settings.user_name,
                                    calibration_dir=self._inst_settings.calibration_dir,
                                    output_dir=self._inst_settings.output_dir)

        self._cached_run_details = None
        self._cached_run_details_number = None

    def focus(self, run_number, **kwargs):
        self._switch_long_mode_inst_settings(kwargs.get("long_mode"))
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(run_number=run_number, input_batching=InputBatchingEnum.Summed,
                           do_van_normalisation=self._inst_settings.van_norm)

    def create_calibration_vanadium(self, run_in_range, **kwargs):
        self._switch_long_mode_inst_settings(kwargs.get("long_mode"))
        kwargs["perform_attenuation"] = False
        self._inst_settings.update_attributes(kwargs=kwargs)

        run_details = self.get_run_details(run_number_string=run_in_range)
        run_details.run_number = run_details.vanadium_run_numbers

        return self._create_calibration_vanadium(vanadium_runs=run_details.vanadium_run_numbers,
                                                 empty_runs=run_details.empty_runs,
                                                 do_absorb_corrections=self._inst_settings.absorb_corrections)

    # Params #

    def get_run_details(self, run_number_string):
        input_run_number_list = common.generate_run_numbers(run_number_string=run_number_string)
        first_run = input_run_number_list[0]
        if self._cached_run_details_number == first_run:
            return self._cached_run_details

        run_details = pearl_algs.get_run_details(run_number_string=run_number_string, inst_settings=self._inst_settings)

        self._cached_run_details_number = first_run
        self._cached_run_details = run_details
        return run_details

    @staticmethod
    def generate_input_file_name(run_number):
        return _generate_file_name(run_number=run_number)

    def generate_output_file_name(self, run_number_string):

        output_name = "PRL" + str(run_number_string)
        # Append each mode of operation
        output_name += "_" + self._inst_settings.tt_mode
        output_name += "_absorb" if self._inst_settings.absorb_corrections else ""
        output_name += "_long" if self._inst_settings.long_mode else ""
        return output_name

    def attenuate_workspace(self, input_workspace):
        attenuation_path = self._attenuation_full_path
        return pearl_algs.attenuate_workspace(attenuation_file_path=attenuation_path, ws_to_correct=input_workspace)

    def normalise_ws_current(self, ws_to_correct, run_details=None):
        monitor_ws = common.get_monitor_ws(ws_to_process=ws_to_correct, run_number_string=run_details.run_number,
                                           instrument=self)
        normalised_ws = pearl_algs.normalise_ws_current(ws_to_correct=ws_to_correct, monitor_ws=monitor_ws,
                                                        spline_coeff=self._inst_settings.monitor_spline,
                                                        integration_range=self._inst_settings.monitor_integration_range,
                                                        lambda_values=self._inst_settings.monitor_lambda)
        common.remove_intermediate_workspace(monitor_ws)
        return normalised_ws

    def get_monitor_spectra_index(self, run_number):
        return self._inst_settings.monitor_spec_no

    def spline_vanadium_ws(self, focused_vanadium_spectra):
        return common.spline_vanadium_for_focusing(focused_vanadium_spectra=focused_vanadium_spectra,
                                                   num_splines=self._inst_settings.spline_coefficient)

    def _focus_processing(self, run_number, input_workspace, perform_vanadium_norm):
        return self._perform_focus_loading(run_number, input_workspace, perform_vanadium_norm)

    def output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        if not output_mode:
            output_mode = self._inst_settings.focus_mode
        output_spectra = \
            pearl_output.generate_and_save_focus_output(self, processed_spectra=processed_spectra,
                                                        run_details=run_details, focus_mode=output_mode,
                                                        perform_attenuation=self._inst_settings.perform_atten)
        group_name = "PEARL" + str(run_details.run_number) + "-Results-D-Grp"
        grouped_d_spacing = mantid.GroupWorkspaces(InputWorkspaces=output_spectra, OutputWorkspace=group_name)
        return grouped_d_spacing

    def crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_in_tof(focused_banks, self._inst_settings.tof_cropping_values)

    def crop_raw_to_expected_tof_range(self, ws_to_crop):
        out_ws = common.crop_in_tof(ws_to_crop=ws_to_crop, x_min=self._inst_settings.raw_data_crop_vals[0],
                                    x_max=self._inst_settings.raw_data_crop_vals[-1])
        return out_ws

    def crop_van_to_expected_tof_range(self, van_ws_to_crop):
        cropped_ws = common.crop_in_tof(ws_to_crop=van_ws_to_crop, x_min=self._inst_settings.van_tof_cropping[0],
                                        x_max=self._inst_settings.van_tof_cropping[-1])
        return cropped_ws

    def apply_absorb_corrections(self, run_details, van_ws, gen_absorb=False):
        if gen_absorb:
            pearl_algs.generate_vanadium_absorb_corrections(van_ws=van_ws)

        return pearl_algs.apply_vanadium_absorb_corrections(van_ws=van_ws, run_details=run_details)

    def _switch_long_mode_inst_settings(self, long_mode_on):
        self._inst_settings.update_attributes(advanced_config=pearl_advanced_config.get_long_mode_dict(long_mode_on),
                                              suppress_warnings=True)


def _generate_file_name(run_number):
    digit = len(str(run_number))

    number_of_digits = 8
    filename = "PEARL"

    for i in range(0, number_of_digits - digit):
        filename += "0"

    filename += str(run_number)
    return filename
