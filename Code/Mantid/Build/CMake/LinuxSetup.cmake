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

set ( CMAKE_INSTALL_PREFIX /opt/${CMAKE_PROJECT_NAME} )
set ( CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${LIB_DIR};${CMAKE_INSTALL_PREFIX}/${PLUGINS_DIR} )