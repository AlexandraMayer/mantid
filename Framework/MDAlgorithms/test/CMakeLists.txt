if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} ${HDF5_INCLUDE_DIRS} )

  include_directories( ../../TestHelpers/inc ../../DataHandling/inc ../../CurveFitting/inc)
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/TearDownWorld.cpp
                        ../../TestHelpers/src/BinaryOperationMDTestHelper.cpp
                        ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/MDAlgorithmsTestHelper.cpp
                        ../../TestHelpers/src/MDEventsTestHelper.cpp
                        ../../TestHelpers/src/ScopedFileHelper.cpp
                        ../../TestHelpers/src/InstrumentCreationHelper.cpp
                        ../../TestHelpers/src/WorkspaceCreationHelper.cpp
                         )

  cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries( MDAlgorithmsTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            API
            DataHandling
            DataObjects
            CurveFitting
            Geometry
            HistogramData
            Kernel
            MDAlgorithms
            Nexus
            ${Boost_LIBRARIES}
            ${POCO_LIBRARIES}
            ${MUPARSER_LIBRARIES}
            ${GMOCK_LIBRARIES}
            ${GTEST_LIBRARIES}
            ${NEXUS_LIBRARIES}
            ${HDF5_LIBRARIES} )

  add_dependencies ( MDAlgorithmsTest DataHandling Algorithms CurveFitting )
  add_dependencies ( FrameworkTests MDAlgorithmsTest )
  # Test data
  add_dependencies ( MDAlgorithmsTest StandardTestData )

  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDAlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()
