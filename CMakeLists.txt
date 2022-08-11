# HSF recommends 3.3 to support C/C++ compile features for C/C++11 across all
# platforms
cmake_minimum_required(VERSION 3.12)

#--- Project name --------------------------------------------------------------
project(podio)

#--- Version -------------------------------------------------------------------
SET( ${PROJECT_NAME}_VERSION_MAJOR 0 )
SET( ${PROJECT_NAME}_VERSION_MINOR 15 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )

SET( ${PROJECT_NAME}_VERSION  "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}" )

#--- Define basic build settings -----------------------------------------------
# Provides install directory variables as defined for GNU software
include(GNUInstallDirs)

# Define a default build type can be overriden by passing
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
set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

if(NOT CMAKE_CXX_STANDARD MATCHES "17|20|23")
  message(FATAL_ERROR "Unsupported C++ standard: ${CMAKE_CXX_STANDARD}")
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
option(CREATE_DOC "Whether or not to create doxygen doc target." OFF)
option(ENABLE_SIO "Build SIO I/O support" OFF)


#--- Declare ROOT dependency ---------------------------------------------------
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(ROOT REQUIRED COMPONENTS RIO Tree)

# Check that root is compiled with a modern enough c++ standard
get_target_property(ROOT_COMPILE_FEATURES ROOT::Core INTERFACE_COMPILE_FEATURES)
if (NOT "cxx_std_17" IN_LIST ROOT_COMPILE_FEATURES)
  message(FATAL_ERROR "You are trying to build podio against a version of ROOT that has not been built with a sufficient c++ standard. podio requires c++17")
endif()
#Check if Python version detected matches the version used to build ROOT
SET(Python_FIND_FRAMEWORK LAST)
IF((TARGET ROOT::PyROOT OR TARGET ROOT::ROOTTPython) AND ${ROOT_VERSION} VERSION_GREATER_EQUAL 6.19)
  # some cmake versions don't include python patch level in PYTHON_VERSION
  IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16.0 AND CMAKE_VERSION VERSION_LESS_EQUAL 3.17.2)
    string(REGEX MATCH [23]\.[0-9]+ REQUIRE_PYTHON_VERSION ${ROOT_PYTHON_VERSION})
  ELSE()
    SET(REQUIRE_PYTHON_VERSION ${ROOT_PYTHON_VERSION})
  ENDIF()
  message( STATUS "Python version used for building ROOT ${ROOT_PYTHON_VERSION}" )
  message( STATUS "Required python version ${REQUIRE_PYTHON_VERSION}")
  FIND_PACKAGE(Python ${REQUIRE_PYTHON_VERSION} EXACT REQUIRED COMPONENTS Development Interpreter)
ELSE()
  FIND_PACKAGE(Python COMPONENTS Development Interpreter)
ENDIF()

# ROOT only sets usage requirements from 6.14, so for
# earlier versions need to hack in INTERFACE_INCLUDE_DIRECTORIES
if(ROOT_VERSION VERSION_LESS 6.14)
  set_property(TARGET ROOT::Core APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${ROOT_INCLUDE_DIRS}")
endif()
list(APPEND PODIO_IO_HANDLERS ROOT)

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

  PODIO_CHECK_CPP_FS(PODIO_FS_LIBS)
  if (SIO_FOUND)
    MESSAGE( STATUS "Found SIO library - will build SIO I/O support" )
    list(APPEND PODIO_IO_HANDLERS SIO)
  endif()
endif()

#--- enable unit testing capabilities ------------------------------------------
include(CTest)
option(USE_EXTERNAL_CATCH2 "Link against an external Catch2 v3 static library, otherwise build it locally" ON)

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

#--- add license files ---------------------------------------------------------
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
              ${CMAKE_CURRENT_SOURCE_DIR}/NOTICE
        DESTINATION ${CMAKE_INSTALL_DOCDIR})

#--- project specific subdirectories -------------------------------------------
add_subdirectory(python)
add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

#--- add CMake infrastructure --------------------------------------------------
include(cmake/podioCreateConfig.cmake)


