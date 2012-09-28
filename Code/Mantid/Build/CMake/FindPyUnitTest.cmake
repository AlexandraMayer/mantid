macro ( PYUNITTEST_ADD_TEST _pyunit_testname_file )
  # decide where to copy the unit tests
  get_filename_component ( _pyunit_testname ${_pyunit_testname_file} NAME_WE )
  set ( _pyunit_outputdir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${_pyunit_testname} )

  # setup the output directory
  add_custom_command ( OUTPUT ${_pyunit_outputdir}
                       COMMAND ${CMAKE_COMMAND} ARGS -E make_directory ${_pyunit_outputdir})
  add_custom_command ( OUTPUT ${_pyunit_outputdir}/__init__.py
                       DEPENDS ${_pyunit_outputdir}
                       COMMAND ${CMAKE_COMMAND} ARGS -E touch ${_pyunit_outputdir}/__init__.py )

  # copy the unit tests
  set ( _pyunit_testfiles "" )
  foreach (part ${ARGN})
    get_filename_component(_pyunit_file ${part} NAME)
    add_custom_command ( OUTPUT ${_pyunit_outputdir}/${_pyunit_file}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${part}
                     COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/${part}
                         ${_pyunit_outputdir}/${_pyunit_file} )
    set ( _pyunit_testfiles ${_pyunit_testfiles} ${_pyunit_outputdir}/${_pyunit_file} )
  endforeach (part ${ARGN})

  add_custom_target ( ${_pyunit_testname_file}
                      DEPENDS ${_pyunit_outputdir}/__init__.py ${_pyunit_testfiles}
                      COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${_pyunit_testname_file}
                      COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${_pyunit_outputdir}
                      COMMAND ${PYTHON_EXECUTABLE} ${PYUNITTEST_GEN_EXEC}
                              -o ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${_pyunit_testname_file}
                              -d ${_pyunit_outputdir}
                              --xmlrunner=${PYUNITTEST_XMLRUNNER}
                              --python="${PYTHON_EXECUTABLE}" )
  set_source_files_properties( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${_pyunit_testname_file}
                               PROPERTIES GENERATED true)

  if( MSVC )
    # We need to call the debug executable for the debug builds
    add_test (NAME ${_pyunit_testname}_py_Debug CONFIGURATIONS Debug
              COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin/Debug"
              ${PYTHON_EXECUTABLE_DEBUG} -B $<TARGET_FILE_DIR:PythonAPI>/${_pyunit_testname_file} )
    add_test (NAME ${_pyunit_testname}_py CONFIGURATIONS Release
              COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin/Release"
              ${PYTHON_EXECUTABLE} -B $<TARGET_FILE_DIR:PythonAPI>/${_pyunit_testname_file} )
  else()
    add_test (NAME ${_pyunit_testname}_py
              COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin"
              ${PYTHON_EXECUTABLE} -B $<TARGET_FILE_DIR:PythonAPI>/${_pyunit_testname_file} )
  endif()

  # add all of the individual tests - this introduces a race condition
  #foreach (part ${ARGN})
  #  get_filename_component(_suitename ${part} NAME_WE)
  #  set (_pyunit_separate_name "${_pyunit_testname}_${_suitename}")
  #  add_test ( NAME ${_pyunit_separate_name}
  #             COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin"
  #                 ${PYTHON_EXECUTABLE} ${_pyunit_testname_file} ${_suitename} )
  #endforeach (part ${ARGN})
endmacro ( PYUNITTEST_ADD_TEST )

#=============================================================
# main()
#=============================================================

# find the driver script
find_program ( PYUNITTEST_GEN_EXEC pyunit_gen.py
               PATHS ${PROJECT_SOURCE_DIR}/TestingTools/pyunit_gen
                     ${PROJECT_SOURCE_DIR}/../TestingTools/pyunit_gen )

# determine where the xmlrunner lives
find_path ( PYUNITTEST_XMLRUNNER xmlrunner/__init__.py
            PATHS ${PROJECT_SOURCE_DIR}/TestingTools/unittest-xml-reporting/src/
                  ${PROJECT_SOURCE_DIR}/../TestingTools/unittest-xml-reporting/src/ )

# let people know whether or not it was found
if (PYUNITTEST_GEN_EXEC)
  set ( PYUNITTEST_FOUND TRUE )
else ()
  set ( PYUNITTEST_FOUND FALSE )
endif ()

mark_as_advanced ( PYUNITTEST_GEN_EXEC PYUNITTEST_XMLRUNNER )
