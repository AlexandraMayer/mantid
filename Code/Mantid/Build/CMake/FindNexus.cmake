###############################################################################
# - Attempt to find NeXus C/C++ libraries and include files
# NEXUS_INCLUDE_DIR where to find napi.h, etc.
# NEXUS_C_LIBRARIES C libraries to link against
# NEXUS_CPP_LIBRARIES C++ libraries to link against
# NEXUS_LIBRARIES All libraries that have been found
# NEXUS_VERSION The NeXus version 
# NEXUS_FOUND If false, do not try to use NeXus
###############################################################################

# Search for the include files. Newer versions of Nexus have a nexus sub directory 
# containing the headers but older versions just have napi.h in the root include path
# Try for nexus/napi.h first
find_path ( NEXUS_INCLUDE_DIR napi.h ${CMAKE_INCLUDE_PATH}/nexus )
	  
# Find the C libraries
find_library ( NEXUS_C_LIBRARIES NAMES NeXus libNeXus-0 )
# Find the C++ libraries
find_library ( NEXUS_CPP_LIBRARIES NAMES NeXusCPP libNeXusCPP-0)
set ( NEXUS_LIBRARIES ${NEXUS_C_LIBRARIES} ${NEXUS_CPP_LIBRARIES} )

message (status "NEXUS_INCLUDE_DIR=${NEXUS_INCLUDE_DIR}")

# Set a version string by examining the napi.h header
if( NEXUS_INCLUDE_DIR ) 
  # Extract the line containing the version string which will look like this "#define NEXUS_VERSION   "X.X.X"                /* major.minor.patch */"
  file ( STRINGS ${NEXUS_INCLUDE_DIR}/napi.h NEXUS_VERSION_TMP REGEX "^#define[ \t]+NEXUS_VERSION[ \t]+\"[0-9]+.[0-9]+.[0-9]+\"[ \t]+/\\* major\\.minor\\.patch \\*/$" )
  # Hack off the portion up to and including the first double quote 
  string( REGEX REPLACE "^#define[ \t]+NEXUS_VERSION[ \t]+\"" "" NEXUS_VERSION_TMP ${NEXUS_VERSION_TMP} )
  # Hack off the portion from the second double quote to the end of the line
  string( REGEX REPLACE "\"[ \t]+/\\* major\\.minor\\.patch \\*/$" "" NEXUS_VERSION_TMP ${NEXUS_VERSION_TMP} )
  set( NEXUS_VERSION ${NEXUS_VERSION_TMP} CACHE STRING "" FORCE )
endif()

# Handle the QUIETLY and REQUIRED arguments and set NEXUS_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
if (NEXUS_VERSION)
  find_package_handle_standard_args( Nexus REQUIRED_VARS NEXUS_LIBRARIES NEXUS_INCLUDE_DIR
				     VERSION_VAR NEXUS_VERSION )
else (NEXUS_VERSION)
  message (status "Failed to determine version: Ignoring version requirement")
  find_package_handle_standard_args( Nexus DEFAULT_MSG NEXUS_LIBRARIES NEXUS_INCLUDE_DIR )
endif (NEXUS_VERSION)