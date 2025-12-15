# Support for the CMake of the oldest supported Ubuntu LTS
cmake_minimum_required(VERSION 3.16.3)

# Get project version from the cpp file
file(READ "src/Learning/KWUtils/KWKhiopsVersion.h" CPP_FILE)
string(REGEX MATCH "#define KHIOPS_VERSION KHIOPS_STR\\(([^)]*)\\)" _ ${CPP_FILE})
set(KHIOPS_VERSION ${CMAKE_MATCH_1})
string(REGEX REPLACE "-.*" "" VERSION ${KHIOPS_VERSION})
message(STATUS "Building Khiops Version: ${KHIOPS_VERSION} (CMake version : ${VERSION})")

# Specify the project's language to C++
project(
  Khiops
  LANGUAGES CXX
  VERSION ${VERSION}
  HOMEPAGE_URL "https://khiops.org")

# Specify CMake policies
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0053 NEW)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0058 NEW)
cmake_policy(SET CMP0063 NEW)
cmake_policy(SET CMP0083 NEW)

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

# Set up the custom configuration options (if customisation is required, update the options in CMakePresets.json)
option(MPI "Use MPI libraries (ON,OFF)" ON)
option(TESTING "Build unit tests (ON,OFF)" ON)
option(BUILD_LEX_YACC "Re-generate parsing files with lex & yacc" OFF)
option(BUILD_JARS "Re-generate norm.jar and khiops.jar" OFF)
option(GENERATE_VIEWS "Generate views sources from dd files" OFF)
option(C11 "Build whith C++11 libraries (ON,OFF)" ON)

# Print the options
message("\nKhiops build options:\n")
message("  CMAKE_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\"")
message("  MPI=\"${MPI}\"")
message("  TESTING=\"${TESTING}\"")
message("  C11=\"${C11}\"")
message("  BUILD_LEX_YACC=\"${BUILD_LEX_YACC}\"")
message("  BUILD_JARS=\"${BUILD_JARS}\"")
message("  GENERATE_VIEWS=\"${GENERATE_VIEWS}\"")
message("\n")

# Check if the os is a linux fedora-like distro (CentOS, Rocky etc...)
#
# The good way to do this is to use cmake_host_system_information but the os-name is avalaible starting from the cmake
# version 3.22. Below, we just grep 'fedora' on /etc/os-release file. It is present on the line ID_LIKE="rhel centos
# fedora"
set(IS_FEDORA_LIKE false)
if(EXISTS "/etc/os-release")
  file(READ "/etc/os-release" OS_RELEASE)
  string(FIND "${OS_RELEASE}" "fedora" FEDORA_FOUND)
  if(FEDORA_FOUND GREATER -1)
    set(IS_FEDORA_LIKE true)
  endif()
endif()

# Check if it is conda environment
set(IS_CONDA false)
if(DEFINED ENV{CONDA_PREFIX} AND NOT $ENV{CONDA_PREFIX} STREQUAL "")
  set(IS_CONDA true)
  message(STATUS "Auto-detected conda environment")
endif()

if(IS_CONDA AND MPI)
  if(NOT DEFINED ENV{mpi})
    message(
      WARNING "You are building in a conda environment without using `conda build`. find_mpi may not work as expected.")
  endif()
endif()

# On Fedora distros, the binaries compiled with mpi must be located under the mpi location furthermore they have to be
# suffixed by the name of the mpi implementation. These 2 items are given by MPI_BIN and MPI_SUFFIX when the module
# environment is loaded source /etc/profile.d/modules.sh module load mpi/mpich-x86_64
if(IS_FEDORA_LIKE AND NOT IS_CONDA)
  if(DEFINED ENV{MPI_BIN})
    set(MPI_BIN $ENV{MPI_BIN})
  else()
    message(
      FATAL_ERROR
        "MPI_BIN is not defined, you have to load the mpi module e.g.:\n source /etc/profile.d/modules.sh && module load mpi/mpich-x86_64"
    )
  endif(DEFINED ENV{MPI_BIN})
  if(DEFINED ENV{MPI_SUFFIX})
    set(MPI_SUFFIX $ENV{MPI_SUFFIX})
  else()
    message(
      FATAL_ERROR
        "MPI_SUFFIX is not defined, you have to load the mpi module e.g.:\n source /etc/profile.d/modules.sh && module load mpi/mpich-x86_64"
    )
  endif(DEFINED ENV{MPI_SUFFIX})
endif(IS_FEDORA_LIKE AND NOT IS_CONDA)

# Set the location of the built artifacts:
#
# - Shared and static library target directory: lib
# - Executable target directory: bin
# - We must use these weird generator expressions to avoid the Debug and Release directories in VS
# - More info: https://stackoverflow.com/q/47175912
#
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/lib/>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/lib/>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/bin/>)
message(STATUS "Executables will be stored in ${CMAKE_BINARY_DIR}/bin/")
message(STATUS "Libraries will be stored in ${CMAKE_BINARY_DIR}/lib/")

# Temporary directory used in install and packaging steps
set(TMP_DIR ${PROJECT_BINARY_DIR}/tmp)

# Set module path for the project
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/packaging" ; "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Force clang to use RTTI on Debug
if(CMAKE_BUILD_TYPE EQUAL "Debug")
  set(LLVM_REQUIRES_RTTI 1)
endif()

# Process dependencies: Find MPI if required
if(MPI)
  # Ask `find_mpi` to determine installed version of the MPI library, via `MPI_Get_library_version` Only ask to
  # determine the MPI library version for Linux setups, where OpenMPI is used and knowing its version at build time is
  # relevant. Note that testing on CMAKE_CROSSCOMPILING is not recommended for projects that target MacOS systems
  # (https://cmake.org/cmake/help/latest/variable/CMAKE_CROSSCOMPILING.html#variable:CMAKE_CROSSCOMPILING)
  if(UNIX AND NOT APPLE)
    set(MPI_DETERMINE_LIBRARY_VERSION true)
  endif()
  find_package(MPI 2.0 REQUIRED)
  message(
    STATUS
      "MPI command line: ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS"
  )
endif()

# Detects the mpi implementation
if(UNIX AND MPI)

  include(get_mpi_implementation)
  get_mpi_implementation()

  # Set MPI suffix if it is not defined with the environment variables (like on Fedora distros)
  if(NOT MPI_SUFFIX AND MPI_IMPL)
    set(MPI_SUFFIX "_${MPI_IMPL}")
  endif()
endif()

# Set C++ revision globally if required
if(TESTING)
  # Google test requires C++14
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
elseif(C11)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Find Bison/Flex if required
if(BUILD_LEX_YACC)
  find_package(BISON REQUIRED)
  find_package(FLEX REQUIRED)
endif()

# Find Java if required
if(BUILD_JARS)
  find_package(Java REQUIRED COMPONENTS Development)
  include(UseJava)
  set(KHIOPS_BUILD_JAR_DIR "${CMAKE_BINARY_DIR}/jars")
  message(STATUS "Java JARs will be stored at ${KHIOPS_BUILD_JAR_DIR}")
  # Note: For final builds we recommend to use java 1.8 to ensure that the jar is compatible with all posterior java
  # versions (from # 1.8 to the most recent ones). The following setting accomplish that:
  #
  # - find_package(Java 1.8 EXACT REQUIRED COMPONENTS Development)
endif()

# Check for support of position independent code/executable
include(CheckPIESupported)
check_pie_supported()

# Message the current C++ configuration
message(STATUS "CMake generator is: ${CMAKE_GENERATOR}")
message(STATUS "CMake compiler is: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMake C++ standard is: ${CMAKE_CXX_STANDARD}")

# MSVC: Eliminate some options the old CMake way because VS sets them with undesired default values
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

  # General compiler properties and definitions
  set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  target_compile_definitions(${target} PRIVATE UNICODE _UNICODE)
  if(MPI)
    target_compile_definitions(${target} PRIVATE USE_MPI)
  endif()
  target_compile_definitions(${target} PRIVATE $<$<CONFIG:RELEASE,RELWITHDEBINFO>:NOALL>)
  target_compile_definitions(${target} PRIVATE $<$<CONFIG:RELWITHDEBINFO>:__ALPHA__>)

  # Delegate to specialized function for Windows and Unix-like
  if(MSVC)
    set_msvc_khiops_options(${target})
  else()
    set_unix_khiops_options(${target})
  endif()
endfunction(set_khiops_options)

# Sets the compiling options for unix environments
function(set_unix_khiops_options target)

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
  # - noexecstack: do not use executable stack
  #
  if(NOT APPLE) # TODO bring back macOS to hardening
    target_link_options(${target} PRIVATE "LINKER:SHELL:-z noexecstack" "LINKER:SHELL:-z relro")
  endif()

  # Stripping only in Linux + Release (-s is not supported in macOS)
  if(NOT APPLE)
    target_link_options(${target} PRIVATE $<$<CONFIG:RELEASE>:-s>)
  endif()

  #
  # - fno-rtti: no run-time type information in Release or RelWithDebInfo .
  # - fno-exceptions: disables exceptions support
  #
  target_compile_options(${target} PRIVATE $<$<CONFIG:RELEASE,RELWITHDEBINFO>:-fno-rtti> -fno-exceptions)

  # Disable "floating-point expression contraction" on clang and gcc to improve reproducibility beetween x86_64 and
  # arm64
  target_compile_options(${target} PRIVATE -ffp-contract=off)

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

# This function set up a JAR target. Arguments:
#
# - NAME: Name of the JAR target
# - IMAGES: Paths of the image files to include in the JAR
function(add_khiops_jar)
  # Parse the function arguments
  set(options "")
  set(oneValueArgs "NAME")
  set(multiValueKwargs "JAVA_FILES" "IMAGES")
  cmake_parse_arguments(ADD_KHIOPS_JAR "${options}" "${oneValueArgs}" "${multiValueKwargs}" ${ARGN})

  # Add the JAR target with its resources and install
  message(STATUS "Setting up JAR ${ADD_KHIOPS_JAR_NAME}.jar")
  add_jar(
    "${ADD_KHIOPS_JAR_NAME}_jar" ${ADD_KHIOPS_JAR_JAVA_FILES}
    OUTPUT_NAME ${ADD_KHIOPS_JAR_NAME} RESOURCES NAMESPACE "images" ${ADD_KHIOPS_JAR_IMAGES}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/jars" COMPONENT KHIOPS)
  install_jar("${ADD_KHIOPS_JAR_NAME}_jar" DESTINATION usr/share/khiops)
endfunction()

# Load module to generate views from .dd files
include(generate_gui)

# Add targets for Norm Parallel and Learning modules
add_subdirectory(src/Norm)
add_subdirectory(src/Parallel)
add_subdirectory(src/Learning)

# Testing settings
if(TESTING)
  # Disable gMock from the building of googletest
  set(BUILD_GMOCK OFF)

  # Fetch googletest from its Git repo
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

  # Include and enable googletest
  include(GoogleTest)
  enable_testing()

  # Add testing targets
  add_subdirectory(test/UnitTests/Norm)
  add_subdirectory(test/UnitTests/Parallel)
  add_subdirectory(test/UnitTests/Parallel-mpi)
  add_subdirectory(test/UnitTests/Learning)
  add_subdirectory(test/UnitTests/KNITest)
  add_subdirectory(test/UnitTests/Utils)
endif(TESTING)

# Exclude googletest from the installation
set(INSTALL_GTEST OFF)

# Add packaging directory for CPack scripts
include(packaging)
