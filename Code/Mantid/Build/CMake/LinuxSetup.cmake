###########################################################################
# Use the system-installed version of Python.
###########################################################################
find_package ( PythonLibs REQUIRED )
include_directories ( ${PYTHON_INCLUDE_PATH} )
# If found, need to add debug library into libraries variable
if ( PYTHON_DEBUG_LIBRARIES )
  set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} debug ${PYTHON_DEBUG_LIBRARIES} )
endif ()

###########################################################################
# tcmalloc stuff. Only used on linux for now.
###########################################################################

# Look for tcmalloc. Make it optional, for now at least, but on by default
set ( USE_TCMALLOC ON CACHE BOOL "Flag for replacing regular malloc with tcmalloc" )
# Note that this is not mandatory, so no REQUIRED
find_package ( Tcmalloc )
# If not found, or not wanted, just carry on without it
if ( USE_TCMALLOC AND TCMALLOC_FOUND )
  set ( TCMALLOC_LIBRARY ${TCMALLOC_LIBRARIES} )
  # Make a C++ define to use as flags in, e.g. MemoryManager.cpp
  add_definitions ( -DUSE_TCMALLOC )
endif ()

###########################################################################
# Set installation variables
###########################################################################

set ( BIN_DIR bin )
set ( LIB_DIR lib )
set ( PLUGINS_DIR plugins )
set ( PVPLUGINS_DIR pvplugins )

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set ( CMAKE_INSTALL_PREFIX /opt/${CMAKE_PROJECT_NAME} CACHE PATH "Install path" FORCE )
ENDIF()

set ( CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${LIB_DIR};${CMAKE_INSTALL_PREFIX}/${PLUGINS_DIR} )

file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh  "#!/bin/sh\n"
                                                    "MANTIDPATH=${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\n"
                                                    "PATH=$PATH:$MANTIDPATH\n"
                                                    "export MANTIDPATH PATH"
)
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh  "#!/bin/csh\n"
                                                    "setenv MANTIDPATH \"${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\"\n"
                                                    "setenv PATH \"\${PATH}:\${MANTIDPATH}\"\n"
)

set ( ENVVARS_ON_INSTALL ON CACHE BOOL "Whether to include the scripts in /etc/profile.d to set the MANTIDPATH variable and add it to PATH. Turning this off allows installing locally without being root." )
if ( ENVVARS_ON_INSTALL )
  # Note: On older versions of CMake, this line may mean that to do a "make package" without being root
  # you will need to set the cache variable CPACK_SET_DESTDIR to ON.
  install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
            DESTINATION /etc/profile.d 
  )
endif ()
