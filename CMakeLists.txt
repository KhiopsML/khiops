# Support for the CMake of the oldest supported Ubuntu LTS
cmake_minimum_required(VERSION 3.16.3)

# Get project version from the cpp file
file(READ "src/Learning/KWUtils/KWKhiopsVersion.h" CPP_FILE)
string(REGEX MATCH "VERSION str\\(([0-9]*.[0-9]*.[0-9]*)" _ ${CPP_FILE})
set(VERSION ${CMAKE_MATCH_1})

# Specify the project's language to C++
project(
  Khiops
  LANGUAGES CXX
  VERSION ${VERSION}
  HOMEPAGE_URL "khiops.com")

# Specify CMake policies
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0053 NEW)
cmake_policy(SET CMP0063 NEW)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0058 NEW)

# Do not add warning flags for MSVC cmake_policy(SET CMP0092 NEW)

# Warn about in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(
    FATAL_ERROR
      "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

# Set the build type to Debug if not set
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)

# Remove always the files from a precedent installation
set(CMAKE_INSTALL_ALWAYS TRUE)

# Set up the custom configuration options
option(MPI "Use MPI libraries (ON,OFF)" ON)
option(FULL "Build test binaries (basetest,PLtest...) (ON,OFF)" ON)
option(LEARNING "Build Learning project; also builds Parallel (ON,OFF)" ON)
option(PARALLEL "Build Parallel project only (ON,OFF)" ON)
option(TESTING "Build unit tests (ON,OFF)" OFF)
option(BUILD_LEX_YACC "Re-generate parsing files with lex & yacc" OFF)
option(BUILD_JARS "Re-generate norm.jar and khiops.jar" OFF)
option(C11 "Build whith C++11 libraries (ON,OFF)" ON)
option(MODL_SO "Build MODL binaries as shared libraries" OFF)
option(FEDORA "Build and package on Fedora distros" OFF)

# Print the options
message("\nKhiops build options:\n")
message("  CMAKE_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\"")
message("  MPI=\"${MPI}\"")
message("  FULL=\"${FULL}\"")
message("  LEARNING=\"${LEARNING}\"")
message("  PARALLEL=\"${PARALLEL}\"")
message("  TESTING=\"${TESTING}\"")
message("  C11=\"${C11}\"")
message("  BUILD_LEX_YACC=\"${BUILD_LEX_YACC}\"")
message("  BUILD_JARS=\"${BUILD_JARS}\"")
message("  MODL_SO=\"${MODL_SO}\"")
message("  FEDORA=\"${FEDORA}\"")
message("\n")

# Set built products location:
#
# - Shared and static library target directory: lib
# - Executable target directory: bin
#
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
message(STATUS "Executables will be stored in ${CMAKE_BINARY_DIR}/bin/")
message(STATUS "Libraries will be stored in ${CMAKE_BINARY_DIR}/lib/")

# set module path for the project
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/packaging")

# Use "-fPIC" / "-fPIE" for all targets by default, including static libs
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CMake doesn't add "-pie" by default for executables (CMake issue #14983)
if(NOT MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif(NOT MSVC)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Force clang to use RTTI on Debug
if(CMAKE_BUILD_TYPE EQUAL "Debug")
  set(LLVM_REQUIRES_RTTI 1)
endif(CMAKE_BUILD_TYPE EQUAL "Debug")

# Process dependencies Find MPI if required
if(MPI)
  find_package(MPI 2.0 REQUIRED)
endif()

# On Fedora distros, the binaries compiled with mpi must be located under the mpi location furthermore they have to be
# suffixed by the name of the mpi implementation. These 2 items are given by MPI_BIN and MPI_SUFFIX when the module
# environment is loaded source /etc/profile.d/modules.sh module load mpi/mpich-x86_64
if(FEDORA)
  if(DEFINED ENV{MPI_BIN})
    set(MPI_BIN $ENV{MPI_BIN})
  else()
    message(FATAL "MPI_BIN is not defined, you have to load the mpi module")
  endif(DEFINED ENV{MPI_BIN})
  if(DEFINED ENV{MPI_SUFFIX})
    set(MPI_SUFFIX $ENV{MPI_SUFFIX})
  else()
    message(FATAL "MPI_SUFFIX is not defined, you have to load the mpi module")
  endif(DEFINED ENV{MPI_SUFFIX})
endif(FEDORA)

# Find Bison/Flex if required
if(BUILD_LEX_YACC)
  find_package(BISON REQUIRED)
  find_package(FLEX REQUIRED)
endif()

# Find Java
if(BUILD_JARS)
  find_package(Java REQUIRED COMPONENTS Development)
endif()

# MSVC: Eliminate some options the old CMake way because VS sets them with default values
#
# - the C++ exception flags
# - the runtime information flags
# - the debug information generation flags
#
if(MSVC)
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

# Sets the compiling options for Khiops
function(set_khiops_options target)
  # Show the current target information
  get_target_property(target_type ${target} TYPE)
  message(STATUS "Setting up ${target_type} ${target}")

  # General compiler definitions
  target_compile_definitions(${target} PRIVATE UNICODE _UNICODE)
  if(MPI)
    target_compile_definitions(${target} PRIVATE USE_MPI)
  endif()

  if(C11)
    target_compile_features(${target} PRIVATE cxx_std_11)
  endif()

  target_compile_definitions(${target} PRIVATE $<$<CONFIG:RELEASE>:NDEBUG NOALL>)
  target_compile_definitions(${target} PRIVATE $<$<CONFIG:RELWITHDEBINFO>:__ALPHA__>)

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(${target} PRIVATE _CPPRTTI) # TODO TO REMOVE for next release
  endif()

  if(MSVC)
    set_msvc_khiops_options(${target})
  else()
    set_unix_khiops_options(${target})
  endif()
endfunction(set_khiops_options)

# Sets the compiling options for unix environments
function(set_unix_khiops_options target)

  target_compile_definitions(${target} PRIVATE __UNIX__) # TODO TO REMOVE for next release

  # Improve size optimisation, to ckeck, MinSizeRel is perhaps enough with -Os
  if(CMAKE_BUILD_TYPE MATCHES MinSizeRel)
    target_link_options(${target} PRIVATE "LINKER:--gc-sections")
    target_compile_options(${target} PRIVATE -ffunction-sections -fdata-sections)
  endif()

  # Do not include RPATHs in the build tree (Debian policy).
  set(CMAKE_SKIP_BUILD_RPATH TRUE)

  # Hardening (mandatory by debian policy)
  #
  # - relro: (relocation read-Only linking) the linker resolves all dynamically linked functions at the beginning of the
  #   execution
  # - noexecstack:do not use executable stack
  #
  target_link_options(${target} PRIVATE "LINKER:SHELL:-z noexecstack" "LINKER:SHELL:-z relro")

  # stripping
  target_link_options(${target} PRIVATE $<$<CONFIG:RELEASE>:-s>)

  #
  # - fno-rtti: no run-time type information in Release .
  # - fno-exceptions: disables exceptions support
  #
  target_compile_options(${target} PRIVATE $<$<CONFIG:RELEASE>:-fno-rtti> -fno-exceptions)

  # Special options for clang, we remove the following too verbose warnings:
  #
  # - unsequenced modification
  # - overloaded virtual functions
  # - inconsistent missing override
  #
  # These warnings should be add back, but we have a lot of work in Khiops
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(${target} PRIVATE -Wno-unsequenced -Wno-overloaded-virtual
                                             -Wno-inconsistent-missing-override)
  endif()

endfunction(set_unix_khiops_options)

# Sets the compiling options for MSVC
function(set_msvc_khiops_options target)

  # General options:
  #
  # - /W4: Show warnings up to level 4.
  # - /MP: Build with multiple processes.
  # - /EHs-c-: C++ Exceptions fully dissabled.
  #
  target_compile_options(${target} PRIVATE /W4 /MP /EHs-c-)

  # Debug only compiling options
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Definitions
    target_compile_definitions(${target} PRIVATE _DEBUG _CONSOLE)

    # Options
    #
    # - /RTC1: Enable run-time error checks.
    # - /ZI: Generates "Format Database for 'Edit and Continue'" debug information format.
    # - /MTd: Use multithread debug runtime (set via MSVC_RUNTIME_LIBRARY property).
    #
    target_compile_options(${target} PRIVATE /RTC1 /ZI)
    set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreadedDebug)

    # Release only compiling options
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Definitions
    target_compile_definitions(${target} PRIVATE NDEBUG NOALL)

    # Options
    #
    # - /GR-: Do not generate runtime information.
    # - /Oi: Generate instrinsic functions.
    # - /Zi: Generates "Format Database" debug information format.
    # - /MT: Use multithread runtime (set via MSVC_RUNTIME_LIBRARY property).
    #
    target_compile_options(${target} PRIVATE /GR- /Oi /Zi)
    set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreaded)
  else()
    message(ERROR "Unsuported MSVC CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
  endif()
endfunction(set_msvc_khiops_options)

# Display all variables
function(display_variables)
  get_cmake_property(_variableNames VARIABLES)
  list(SORT _variableNames)
  foreach(_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
  endforeach()
endfunction()

# Always build Norm
add_subdirectory(src/Norm)

# Build Parallel if specified or when building Learning
if(PARALLEL OR LEARNING)
  add_subdirectory(src/Parallel)
endif(PARALLEL OR LEARNING)

# Build Learning if required
if(LEARNING)
  add_subdirectory(src/Learning)
endif(LEARNING)

if(TESTING)
  # there are two options for googletest, the recommanded one is Fetch and the simple one is find_package both of them
  # are successfuly tested on Linux, whe have to test them on windows and mac (TODO) On windows (with visual studio):
  #
  # - the git fetch is not working
  # - we have to defined http_proxy and https_proxy environment variables
  #
  # GoogleTest requires at least C++14
  set(CMAKE_CXX_STANDARD 14)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG "v1.13.0")
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  #
  # - set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  # - ** This doesn't work due to a bug ** see below for the workaround
  FetchContent_MakeAvailable(googletest)

  # Workaround for windows: https://github.com/actions/virtual-environments/issues/5900
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET gtest PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
    set_property(TARGET gtest_main PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
  else()
    set_property(TARGET gtest PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDebug)
    set_property(TARGET gtest_main PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDebug)
  endif()
  include(GoogleTest)
  enable_testing()

  # Always build the tests for Norm
  add_subdirectory(test/Norm)
  if(PARALLEL OR LEARNING)
    add_subdirectory(test/Parallel)
    # add_subdirectory(test/Parallel-mpi) TODO activate for next release
  endif(PARALLEL OR LEARNING)

  # Build the test for Learning if required
  if(LEARNING)
    add_subdirectory(test/Learning)
  endif(LEARNING)
endif(TESTING)

# Exclude googletest from the installation
set(INSTALL_GTEST OFF)

include(packaging)
