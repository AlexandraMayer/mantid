set ( SRC_FILES
	src/Extract.cpp
	src/Group.cpp
	src/IndexInfo.cpp
)

set ( INC_FILES
	inc/MantidIndexing/DllConfig.h
	inc/MantidIndexing/Extract.h
	inc/MantidIndexing/Group.h
	inc/MantidIndexing/IndexInfo.h
	inc/MantidIndexing/MakeRange.h
)

set ( TEST_FILES
	ExtractTest.h
	GroupTest.h
	IndexInfoTest.h
	MakeRangeTest.h
)

if (COVERALLS)
  foreach( loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Indexing SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( Indexing ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( Indexing PROPERTIES OUTPUT_NAME MantidIndexing
  COMPILE_DEFINITIONS IN_MANTID_INDEXING )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( Indexing PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

# Add to the 'Framework' group in VS
set_property ( TARGET Indexing PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( Indexing LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Indexing ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
