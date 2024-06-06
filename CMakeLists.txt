# HSF recommends 3.3 to support C/C++ compile features for C/C++11 across all
# platforms
cmake_minimum_required(VERSION 3.12)

#--- Project name --------------------------------------------------------------
project(podio)

#--- Version -------------------------------------------------------------------
SET( ${PROJECT_NAME}_VERSION_MAJOR 1 )
SET( ${PROJECT_NAME}_VERSION_MINOR 1 )
SET( ${PROJECT_NAME}_VERSION_PATCH 99 )

SET( ${PROJECT_NAME}_VERSION  "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}" )

#--- Define basic build settings -----------------------------------------------
# Provides install directory variables as defined for GNU software
include(GNUInstallDirs)

# Define a default build type can be overridden by passing
# ``-DCMAKE_BUILD_TYPE=<type>`` when invoking CMake
if(NOT CMAKE_CONFIGURATION_TYPES)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo
      CACHE STRING "Choose the type of build, options are: None Release MinSizeRel Debug RelWithDebInfo"
      FORCE
      )
  else()
    set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"
      CACHE STRING "Choose the type of build, options are: None Release MinSizeRel Debug RelWithDebInfo"
      FORCE
      )
  endif()
endif()

# Set up C++ Standard
# ``-DCMAKE_CXX_STANDARD=<standard>`` when invoking CMake
set(CMAKE_CXX_STANDARD 20 CACHE STRING "")

if(NOT CMAKE_CXX_STANDARD MATCHES "17|20")
  message(FATAL_ERROR "Unsupported C++ standard: ${CMAKE_CXX_STANDARD}")
endif()

if(CMAKE_CXX_STANDARD EQUAL 17)
  message(WARNING "C++17 is deprecated and support for it will be removed in v01-03. Please use at least C++20.")
endif()

# Prevent CMake falls back to the latest standard the compiler does support
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disables the use of compiler-specific extensions, hence makes sure the code
# works for a wider range of compilers
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "Setting C++ standard: '${CMAKE_CXX_STANDARD}'.")

set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

option(PODIO_SET_RPATH "Link libraries with built-in RPATH (run-time search path)" OFF)
include(cmake/podioBuild.cmake)
podio_set_compiler_flags()
podio_set_rpath()

set(USE_SANITIZER ""
  CACHE STRING "Compile with a sanitizer. Options are Address, Memory, MemoryWithOrigin, Undefined, Thread, Leak, Address;Undefined")
ADD_SANITIZER_FLAGS()
option(FORCE_RUN_ALL_TESTS "Run all the tests even those with known problems" OFF)

option(CLANG_TIDY "Run clang-tidy after compilation." OFF)
ADD_CLANG_TIDY()

#--- Declare options -----------------------------------------------------------
option(CREATE_DOC        "Whether or not to create doxygen doc target." OFF)
option(ENABLE_SIO        "Build SIO I/O support" OFF)
option(PODIO_RELAX_PYVER "Do not require exact python version match with ROOT" OFF)
option(ENABLE_RNTUPLE    "Build with support for the new ROOT NTtuple format" OFF)
option(ENABLE_DATASOURCE "Build podio's ROOT DataSource" OFF)
option(PODIO_USE_CLANG_FORMAT "Use clang-format to format the code" OFF)
option(ENABLE_JULIA      "Enable Julia support. When enabled, Julia datamodels will be generated, and Julia tests will run." OFF)


#--- Declare ROOT dependency ---------------------------------------------------
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(root_components_needed RIO Tree)
if(ENABLE_RNTUPLE)
  list(APPEND root_components_needed ROOTNTuple)
endif()
if(ENABLE_DATASOURCE)
  list(APPEND root_components_needed ROOTDataFrame)
endif()
find_package(ROOT REQUIRED COMPONENTS ${root_components_needed})
if((ENABLE_RNTUPLE) AND (${ROOT_VERSION} VERSION_LESS 6.28.02))
    message(FATAL_ERROR "You are trying to build podio with support for the new ROOT NTuple format, but your ROOT version is too old. Please update ROOT to at least version 6.28.02")
endif()

# ROOT_CXX_STANDARD was introduced in https://github.com/root-project/root/pull/6466
# before that it's an empty variable so we check if it's any number > 0
if(NOT DEFINED ROOT_CXX_STANDARD)
  message(STATUS "ROOT c++ standard not yet available. Determining from compile features")
  get_target_property(ROOT_COMPILE_FEATURES ROOT::Core INTERFACE_COMPILE_FEATURES)
  foreach(cxxstd IN ITEMS 20;17;14;11)
    if ("cxx_std_${cxxstd}" IN_LIST ROOT_COMPILE_FEATURES)
      set(ROOT_CXX_STANDARD ${cxxstd})
      break()
    endif()
  endforeach()
endif()

if(NOT DEFINED ROOT_CXX_STANDARD)
  message(WARNING "ROOT c++ standard could not be detected")
else()
  message(STATUS "Determined ROOT c++ standard: " ${ROOT_CXX_STANDARD})
endif()

if(ROOT_CXX_STANDARD VERSION_LESS 17)
  message(FATAL_ERROR "You are trying to build podio against a version of ROOT that has not been built with a sufficient c++ standard. podio requires c++17 or higher")
endif()
if(NOT ROOT_CXX_STANDARD VERSION_EQUAL CMAKE_CXX_STANDARD)
  message(WARNING "You are trying to build podio with a different c++ standard than ROOT. C++${CMAKE_CXX_STANDARD} was required but ROOT was built with C++${ROOT_CXX_STANDARD}")
endif()

# ROOT only sets usage requirements from 6.14, so for
# earlier versions need to hack in INTERFACE_INCLUDE_DIRECTORIES
if(ROOT_VERSION VERSION_LESS 6.14)
  set_property(TARGET ROOT::Core APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${ROOT_INCLUDE_DIRS}")
endif()
list(APPEND PODIO_IO_HANDLERS ROOT)

# python setup (including python package discovery and install dir)
podio_python_setup()

#--- enable podio macros--------------------------------------------------------
include(cmake/podioMacros.cmake)

# optionally build with SIO -------------------------------------------------
if(ENABLE_SIO)
  find_package( SIO REQUIRED)
  # Targets from SIO only become available with v00-01 so we rig them here to be
  # able to use them
  if (SIO_VERSION VERSION_LESS 0.1)
    message(STATUS "Found SIO without Targets, creating them on the fly")
    add_library(SIO::sio SHARED IMPORTED)
    set_target_properties(SIO::sio PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${SIO_INCLUDE_DIRS}
      INTERFACE_LINK_LIBRARIES ${SIO_LIBRARIES}
      IMPORTED_LOCATION ${SIO_LIBRARIES})
  endif()

  if (SIO_FOUND)
    MESSAGE( STATUS "Found SIO library - will build SIO I/O support" )
    list(APPEND PODIO_IO_HANDLERS SIO)
  endif()
endif()

#--- enable unit testing capabilities ------------------------------------------
include(CTest)
set(USE_EXTERNAL_CATCH2 AUTO CACHE STRING "Link against an external Catch2 v3 static library, otherwise build it locally")
set_property(CACHE USE_EXTERNAL_CATCH2 PROPERTY STRINGS AUTO ON OFF)

#--- enable CPack --------------------------------------------------------------

option(ENABLE_CPACK "Whether or not to use cpack config" OFF)
if(ENABLE_CPACK)
  include(cmake/podioCPack.cmake)
endif()

#--- target for Doxygen documentation ------------------------------------------
if(CREATE_DOC)
	include(cmake/podioDoxygen.cmake)
endif()
#--- add version files ---------------------------------------------------------
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/podioVersion.in.h
               ${CMAKE_CURRENT_SOURCE_DIR}/include/podio/podioVersion.h )
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/podio/podioVersion.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/podio )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/python/__version__.py.in
               ${CMAKE_CURRENT_SOURCE_DIR}/python/podio/__version__.py)
# Quality of life improvement to be able to use podio-dump --help
# without having to import ROOT
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/python/__version__.py.in
               ${CMAKE_CURRENT_SOURCE_DIR}/python/podio_version.py)


#--- add license files ---------------------------------------------------------
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/NOTICE
        DESTINATION ${CMAKE_INSTALL_DOCDIR})

#--- project specific subdirectories -------------------------------------------
add_subdirectory(src)

# Set the podio_PYTHON_DIR manually here because the macros in tests expect it
# If testing is not build this helps when building together with other packages
SET(podio_PYTHON_DIR ${PROJECT_SOURCE_DIR}/python CACHE PATH "Path to the podio python directory")

if(BUILD_TESTING)
  include(ExternalData)
  list(APPEND ExternalData_URL_TEMPLATES
    "https://key4hep.web.cern.ch:443/testFiles/podio/%(hash)"
  )
  include(cmake/podioTest.cmake)
  add_subdirectory(tests)
endif()

find_package(fmt REQUIRED)

add_subdirectory(tools)
add_subdirectory(python)


if(BUILD_TESTING)
  # Make sure to fetch all data, after all legacy test cases have been added
  ExternalData_Add_Target(legacy_test_cases)
  message(STATUS "Test inputs will be stored in: ${ExternalData_OBJECT_STORES} if they are not already present")
endif()
#--- add CMake infrastructure --------------------------------------------------
include(cmake/podioCreateConfig.cmake)

#--- code format targets -------------------------------------------------------
include(cmake/pythonFormat.cmake)
