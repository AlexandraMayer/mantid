if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  cxxtest_add_test ( IndexingTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries( IndexingTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
    Indexing
    ${Boost_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES} )

  add_dependencies ( FrameworkTests IndexingTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET IndexingTest PROPERTY FOLDER "UnitTests" )
endif ()
