# cmake-format: off
# Detect MPI implementation. If an implementation is detected:
# - MPI_IMPL is set with "openmpi", "mpich" or "intel" 
# - MPI_FULL_NAME is set to the full name of the MPI implementation (e.g., "Open MPI", "MPICH", "Intel MPI")
# - and the appropriate variable is set to TRUE among IS_OPEN_MPI, IS_MPICH, IS_MS_MPI and IS_INTEL_MPI. 
# cmake-format: on
function(get_mpi_implementation)
  # On standard environments, search implementation names in MPI_LIBRARIES provided by FindMPI. Conda and pip
  # environments need package metadata because their library paths do not identify the implementation reliably.
  if(IS_CONDA)
    if(DEFINED ENV{mpi})
      set(DETECTION_MESSAGE "from conda build environment")
      set(VAR_MPI_INFO $ENV{mpi})
    else()
      set(DETECTION_MESSAGE "from conda standard environment")
      execute_process(
        COMMAND conda list
        COMMAND grep mpi
        OUTPUT_VARIABLE VAR_MPI_INFO)
    endif()
    if(VAR_MPI_INFO STREQUAL "")
      message(
        FATAL_ERROR
          "No MPI implementation found in conda environment. Please make sure to install an MPI implementation (e.g. `conda install openmpi`)"
      )
    endif()
  elseif(IS_PIP)
    set(DETECTION_MESSAGE "from pip environment")

    # Use importlib.metadata instead of pip list because pip may not be available and pip list output is not stable
    # across versions.
    find_package(Python 3.8 REQUIRED)
    execute_process(
      COMMAND
        python -c
        "import importlib.metadata as m; names=[]; [names.append((d.metadata.get('Name') or '').strip().lower()) for d in m.distributions() if (d.metadata.get('Name') or '').strip()]; print(' '.join(sorted(set(names))))"
      OUTPUT_VARIABLE PIP_DISTRIBUTIONS_INFO
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(VAR_MPI_INFO "${PIP_DISTRIBUTIONS_INFO}")
  else()
    set(DETECTION_MESSAGE "from standard environment")
    set(VAR_MPI_INFO "${MPI_LIBRARIES}")
  endif()

  # ERROR if VAR_MPI_INFO is not defined, it means either: - in standard environment FindMPI provides no MPI path (MPI
  # is not installed) - in conda build, the 'mpi' variable is missing and FindMPI may find the system-wide MPI, which is
  # not what we want - in conda, outside the build process, the mpi package is not installed and FindMPI may find the
  # system-wide MPI
  if(NOT DEFINED VAR_MPI_INFO OR "${VAR_MPI_INFO}" STREQUAL "")
    message(FATAL_ERROR "Missing information to discover the MPI implementation")
  endif()

  # Match implementation names case-insensitively because command outputs often use mixed case.
  string(TOLOWER "${VAR_MPI_INFO}" VAR_MPI_INFO_LOWER)

  # Find "openmpi", "mpich" or "intel" in VAR_MPI_INFO_LOWER.
  string(REGEX MATCH "openmpi|open-mpi" VAR_MATCH "${VAR_MPI_INFO_LOWER}")
  if(VAR_MATCH)
    set(MPI_IMPL "openmpi")
    set(MPI_FULL_NAME "Open MPI")
    set(IS_OPEN_MPI TRUE)
  endif()

  string(REGEX MATCH "mpich" VAR_MATCH "${VAR_MPI_INFO_LOWER}")
  if(VAR_MATCH)
    set(MPI_IMPL "mpich")
    set(MPI_FULL_NAME "MPICH")
    set(IS_MPICH TRUE)
  endif()

  string(REGEX MATCH "msmpi" VAR_MATCH "${VAR_MPI_INFO_LOWER}")
  if(VAR_MATCH)
    set(MPI_IMPL "msmpi")
    set(MPI_FULL_NAME "Microsoft MPI")
    set(IS_MS_MPI TRUE)
  endif()

  string(REGEX MATCH "intel|impi" VAR_MATCH "${VAR_MPI_INFO_LOWER}")
  if(VAR_MATCH)
    set(MPI_IMPL "intel")
    set(MPI_FULL_NAME "Intel MPI")
    set(IS_INTEL_MPI TRUE)
  endif()

  if(MPI_IMPL)
    message(STATUS "Auto-detected MPI implementation: ${MPI_IMPL} (${DETECTION_MESSAGE})")
  else()
    message(STATUS "Unable to detect the MPI implementation: no suffix will be added to binaries name")
  endif()

  # Transmit variables to the parent scope.
  set(MPI_IMPL
      ${MPI_IMPL}
      PARENT_SCOPE)
  set(IS_OPEN_MPI
      ${IS_OPEN_MPI}
      PARENT_SCOPE)
  set(IS_MPICH
      ${IS_MPICH}
      PARENT_SCOPE)
  set(IS_MS_MPI
      ${IS_MS_MPI}
      PARENT_SCOPE)
  set(IS_INTEL_MPI
      ${IS_INTEL_MPI}
      PARENT_SCOPE)
endfunction()
