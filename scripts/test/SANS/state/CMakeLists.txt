#
## Tests for SANSState
##

set ( TEST_PY_FILES
   adjustment_test.py
   calculate_transmission_test.py
   convert_to_q_test.py
   data_test.py
   mask_test.py
   move_test.py
   normalize_to_monitor_test.py
   reduction_mode_test.py
   save_test.py
   scale_test.py
   slice_event_test.py
   state_base_test.py
   state_functions_test.py
   state_test.py
   wavelength_and_pixel_adjustment_test.py
   wavelength_test.py
)
check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=Python
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonSANS ${TEST_PY_FILES} )
