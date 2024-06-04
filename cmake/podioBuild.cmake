#---------------------------------------------------------------------------------------------------
#---podio_set_rpath
#
#  Set optional rpath on linux and mandatory rpath on mac
#
#---------------------------------------------------------------------------------------------------
macro(podio_set_rpath)
  #  When building, don't use the install RPATH already (but later on when installing)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)         # don't skip the full RPATH for the build tree
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) # use always the build RPATH for the build tree
  set(CMAKE_MACOSX_RPATH TRUE)              # use RPATH for MacOSX
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) # point to directories outside the build tree to the install RPATH

  # Check whether to add RPATH to the installation (the build tree always has the RPATH enabled)
  if(APPLE)
    set(CMAKE_INSTALL_NAME_DIR "@rpath")
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")    # self relative LIBDIR
    # the RPATH to be used when installing, but only if it's not a system directory
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
      set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
    endif("${isSystemDir}" STREQUAL "-1")
  elseif(PODIO_SET_RPATH)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIBDIR}") # install LIBDIR
    # the RPATH to be used when installing, but only if it's not a system directory
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
      set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIBDIR}")
    endif("${isSystemDir}" STREQUAL "-1")
    # Make sure to actually set RPATH and not RUNPATH by disabling the new dtags
    # explicitly. Set executable and shared library linker flags for this
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--disable-new-dtags")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
  else()
    set(CMAKE_SKIP_INSTALL_RPATH TRUE)           # skip the full RPATH for the install tree
  endif()
endmacro(podio_set_rpath)

#---------------------------------------------------------------------------------------------------
#---podio_set_compiler_flags
#
#  Set compiler and linker flags
#
#---------------------------------------------------------------------------------------------------

macro(podio_set_compiler_flags)
  include(CheckCXXCompilerFlag)

  # Store the default flags here, before we add all the warning, so that we can
  # build Catch2 locally without them
  SET(CXX_FLAGS_CMAKE_DEFAULTS "${CMAKE_CXX_FLAGS}")

  SET(COMPILER_FLAGS -Wshadow -Wformat-security -Wno-long-long -Wdeprecated -fdiagnostics-color=auto -Wall -Wextra -pedantic -Weffc++)

  # AppleClang/Clang specific warning flags
  if(CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$")
    set ( COMPILER_FLAGS ${COMPILER_FLAGS} -Winconsistent-missing-override -Wheader-hygiene )
  endif()

  FOREACH( FLAG ${COMPILER_FLAGS} )
    ## need to replace the minus or plus signs from the variables, because it is passed
    ## as a macro to g++ which causes a warning about no whitespace after macro
    ## definition
    STRING(REPLACE "-" "_" FLAG_WORD ${FLAG} )
    STRING(REPLACE "+" "P" FLAG_WORD ${FLAG_WORD} )

    CHECK_CXX_COMPILER_FLAG( "${FLAG}" CXX_FLAG_WORKS_${FLAG_WORD} )
    IF( ${CXX_FLAG_WORKS_${FLAG_WORD}} )
      message( STATUS "Adding ${FLAG} to CXX_FLAGS" )
      SET ( CMAKE_CXX_FLAGS "${FLAG} ${CMAKE_CXX_FLAGS} ")
    ELSE()
      message( STATUS "NOT Adding ${FLAG} to CXX_FLAGS" )
    ENDIF()
  ENDFOREACH()

  # resolve which linker we use
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -Wl,--version OUTPUT_VARIABLE stdout ERROR_QUIET)
  if("${stdout}" MATCHES "GNU ")
    set(LINKER_TYPE "GNU")
    message( STATUS "Detected GNU compatible linker" )
  else()
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -Wl,-v ERROR_VARIABLE stderr )
    if(("${stderr}" MATCHES "PROGRAM:ld") AND ("${stderr}" MATCHES "PROJECT:ld64"))
      set(LINKER_TYPE "APPLE")
      message( STATUS "Detected Apple linker" )
    else()
      set(LINKER_TYPE "unknown")
      message( STATUS "Detected unknown linker" )
    endif()
  endif()

  if("${LINKER_TYPE}" STREQUAL "APPLE")
    SET ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,dynamic_lookup")
  elseif("${LINKER_TYPE}" STREQUAL "GNU")
    SET ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--allow-shlib-undefined")
  else()
    MESSAGE( WARNING "No known linker (GNU or Apple) has been detected, pass no flags to linker" )
  endif()

endmacro(podio_set_compiler_flags)

#--- add sanitizer flags, depending on the setting of USE_SANITIZER and the
#--- availability of the different sanitizers
macro(ADD_SANITIZER_FLAGS)
  if(USE_SANITIZER)
    if(USE_SANITIZER MATCHES "Address;Undefined" OR USE_SANITIZE MATCHES "Undefined;Address")
      message(STATUS "Building with Address and Undefined behavior sanitizers")
      add_compile_options("-fsanitize=address,undefined")
      add_link_options("-fsanitize=address,undefined")

    elseif(USE_SANITIZER MATCHES "Address")
      message(STATUS "Building with Address sanitizer")
      add_compile_options("-fsanitize=address")
      add_link_options("-fsanitize=address")

    elseif(USE_SANITIZER MATCHES "Undefined")
      message(STATUS "Building with Undefined behaviour sanitizer")
      add_compile_options("-fsanitize=undefined")
      add_link_options("-fsanitize=undefined")

    elseif(USE_SANITIZER MATCHES "Thread")
      message(STATUS "Building with Thread sanitizer")
      add_compile_options("-fsanitize=thread")
      add_link_options("-fsanitize=thread")

    elseif(USE_SANITIZER MATCHES "Leak")
      # Usually already part of Address sanitizer
      message(STATUS "Building with Leak sanitizer")
      add_compile_options("-fsanitize=leak")
      add_link_options("-fsanitize=leak")

    elseif(USE_SANITIZER MATCHES "Memory(WithOrigins)?")
      if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "GCC does not have memory sanitizer support")
      endif()
      message(STATUS "Building with memory sanitizer")
      add_compile_options("-fsanitize=memory")
      add_link_options("-fsanitize=memory")
      if(USE_SANITIZER MATCHES "MemoryWithOrigins")
        message(STATUS "Adding origin tracking for memory sanitizer")
        add_compile_options("-fsanitize-memory-track-origins")
      endif()

    else()
      message(FATAL_ERROR "Unsupported value for USE_SANITIZER: ${USE_SANITIZER}")
    endif()

    # Set a few more flags if we have successfully configured a sanitizer
    # For nicer stack-traces in the output
    add_compile_options("-fno-omit-frame-pointer")
    # append_flag( CMAKE_CXX_FLAGS CMAKE_C_FLAGS)

    # Make it even easier to interpret stack-traces
    if(uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
      add_compile_options("-O0")
    endif()

  endif(USE_SANITIZER)
endmacro(ADD_SANITIZER_FLAGS)

#--- Add clang-tidy to the compilation step
macro(ADD_CLANG_TIDY)
  if (CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    mark_as_advanced(FORCE CLANG_TIDY_EXE)
    if (NOT CLANG_TIDY_EXE)
      message(FATAL_ERROR "CLANG_TIDY required but cannot find clang-tidy executable")
    endif()

    # We simply use cmakes clang-tidy integration here for clang-tidy which
    # would work with the Ninja and Makefile generators
    message(STATUS "Enabling clang-tidy, using clang-tidy: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
  endif(CLANG_TIDY)
endmacro(ADD_CLANG_TIDY)

# --- Macro to find a python version that is compatible with ROOT and to setup
# --- the python install directory for the podio python sources
#
# The python install directory is exposed via the podio_PYTHON_INSTALLDIR cmake
# variable. It defaults to
# CMAKE_INSTALL_PREFIX/lib[64]/pythonX.YY/site-packages/, where pythonX.YY is
# the major.minor version of python and lib or lib64 is decided from checking
# Python3_SITEARCH
#
# NOTE: This macro needs to be called **AFTER** find_package(ROOT) since that is
# necessary to expose
macro(podio_python_setup)
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

  if(NOT PODIO_RELAX_PYVER)
    find_package(Python3 ${REQUIRE_PYTHON_VERSION} EXACT REQUIRED COMPONENTS Development Interpreter)
  else()
    find_package(Python3 ${REQUIRE_PYTHON_VERSION} REQUIRED COMPONENTS Development Interpreter)
    string(REPLACE "." ";" _root_pyver_tuple ${REQUIRE_PYTHON_VERSION})
    list(GET _root_pyver_tuple 0 _root_pyver_major)
    list(GET _root_pyver_tuple 1 _root_pyver_minor)
    if(NOT "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}" VERSION_EQUAL "${_root_pyver_major}.${_root_pyver_minor}")
      message(FATAL_ERROR "There is a mismatch between the major and minor versions in Python"
        " (found ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}) and ROOT, compiled with "
        "Python ${_root_pyver_major}.${_root_pyver_minor}")
    endif()
  endif()
else()
  find_package(Python3 COMPONENTS Development Interpreter)
endif()

# Setup the python install dir. See the discussion in
# https://github.com/AIDASoft/podio/pull/599 for more details on why this is
# done the way it is
set(podio_python_lib_dir lib)
if("${Python3_SITEARCH}" MATCHES "/lib64/")
  set(podio_python_lib_dir lib64)
endif()

set(podio_PYTHON_INSTALLDIR
  "${CMAKE_INSTALL_PREFIX}/${podio_python_lib_dir}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages"
  CACHE STRING
  "The install prefix for the python bindings and the generator and templates"
)
endmacro(podio_python_setup)
