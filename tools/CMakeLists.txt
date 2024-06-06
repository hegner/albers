add_executable(podio-dump-cpp src/podio-dump.cpp)
target_link_libraries(podio-dump-cpp PRIVATE podio::podio podio::podioIO)


install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/podio-dump DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/podio-vis DESTINATION ${CMAKE_INSTALL_BINDIR})
if(ENABLE_RNTUPLE)
  install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/podio-ttree-to-rntuple DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/podio-merge-files DESTINATION ${CMAKE_INSTALL_BINDIR})

# Add a very basic test of podio-vis
if(BUILD_TESTING)
  # Helper function for easily creating "tests" that simply execute podio-vis
  # with different arguments. Not crashing is considered success.
  #
  # Args:
  #     name        the name of the test
  #     depends_on  the target name of the test that produces the required input file
  function(CREATE_VIS_TEST name depends_on)
    add_test(NAME ${name} COMMAND ./podio-vis ${ARGN})
    PODIO_SET_TEST_ENV(${name})

    set_tests_properties(${name} PROPERTIES
       WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
       )
    if (depends_on)
      set_tests_properties(${name} PROPERTIES
        DEPENDS ${depends_on}
        )
    endif()
  endfunction()

  CREATE_VIS_TEST(podio-vis-help _dummy_target_
    --help)
  CREATE_VIS_TEST(podio-vis-datamodel datamodel
    --dot ${PROJECT_SOURCE_DIR}/tests/datalayout.yaml)
  CREATE_VIS_TEST(podio-vis-datamodel-extension extension_model
    --dot --upstream-edm datamodel:${PROJECT_SOURCE_DIR}/tests/datalayout.yaml ${PROJECT_SOURCE_DIR}/tests/datalayout_extension.yaml)
endif()

# Add a very basic tests here to make sure that podio-dump at least runs
if(BUILD_TESTING)
  # Helper function for easily creating "tests" that simply execute podio-dump
  # with different arguments. Not crashing is considered success.
  #
  # Args:
  #     name        the name of the test
  #     depends_on  the target name of the test that produces the required input file
  function(CREATE_DUMP_TEST name depends_on)
    add_test(NAME ${name} COMMAND ./podio-dump ${ARGN})
    PODIO_SET_TEST_ENV(${name})

    set_tests_properties(${name} PROPERTIES
       WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
       )
    if (depends_on)
      set_tests_properties(${name} PROPERTIES
        DEPENDS ${depends_on}
        )
    endif()
  endfunction()

  # Helper function for easily creating "tests" that simply execute podio-dump
  # with different arguments on legacy input files. Not crashing is considered
  # success.
  #
  # Args:
  #     name        the name suffix of the test (the actual name will be assembled)
  #     version     the legacy version of the input
  #     input_file  the input file to use
  function(CREATE_LEGACY_DUMP_TEST name version input_file)
    set(_name podio-dump-legacy_${name}_${version})
    ExternalData_Add_Test(legacy_test_cases
      NAME ${_name}
      COMMAND ./podio-dump ${ARGN} DATA{${PROJECT_SOURCE_DIR}/tests/input_files/${input_file}}
    )
    PODIO_SET_TEST_ENV(${_name})

    set_tests_properties(${_name} PROPERTIES
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )
  endfunction()

  CREATE_DUMP_TEST(podio-dump-help _dummy_target_ --help)
  CREATE_DUMP_TEST(podio-dump-root "write_frame_root" ${PROJECT_BINARY_DIR}/tests/root_io/example_frame.root)
  CREATE_DUMP_TEST(podio-dump-detailed-root "write_frame_root" --detailed --category other_events --entries 2:3 ${PROJECT_BINARY_DIR}/tests/root_io/example_frame.root)

  CREATE_LEGACY_DUMP_TEST("root" v00-16-06 v00-16-06-example.root)
  CREATE_LEGACY_DUMP_TEST("root-detailed" v00-16-06 v00-16-06-example.root --detailed --entries 2:3)

  if (ENABLE_SIO)
    CREATE_DUMP_TEST(podio-dump-sio "write_frame_sio" --entries 4:7 ${PROJECT_BINARY_DIR}/tests/sio_io/example_frame.sio)
    CREATE_DUMP_TEST(podio-dump-detailed-sio "write_frame_sio" --detailed --entries 9 ${PROJECT_BINARY_DIR}/tests/sio_io/example_frame.sio)

    CREATE_LEGACY_DUMP_TEST("sio" v00-16-06 v00-16-06-example.sio)
    CREATE_LEGACY_DUMP_TEST("sio-detailed" v00-16-06 v00-16-06-example.sio --detailed --entries 2:3)
  endif()

  if (ENABLE_RNTUPLE)
    CREATE_DUMP_TEST(podio-dump-rntuple "write_rntuple" ${PROJECT_BINARY_DIR}/tests/root_io/example_rntuple.root)
    CREATE_DUMP_TEST(podio-dump-detailed-rntuple "write_rntuple" --detailed --category events --entries 1:3 ${PROJECT_BINARY_DIR}/tests/root_io/example_rntuple.root)
  endif()

endif()
