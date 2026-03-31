# Detect MPI implementation. If an implementation is detected, MPI_IMPL is set with "openmpi", "mpich" or "intel". And
# set the appropriate variable to TRUE among IS_OPEN_MPI, IS_MPICH and IS_INTEL_MPI. If no implementation is detected,
# MPI_IMPL is empty and no variable is set to TRUE. Use the global variable IS_CONDA
function(get_mpi_implementation)
  # On standard environment, we search implementation names in the MPI_LIBRARIES variable provided by find_mpi. Inside
  # conda environment, the library path is the same for all libraries. So we use the 'mpi' variable that is defined
  # (only) in the build environment. Outside the build environment, the 'mpi' variable is not defined, we use the list
  # of installed packages. It works because, the mpi packages are in conflict (only one implementation on the same
  # environment)
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
  elseif(IS_PIP)
    set(DETECTION_MESSAGE "from pip environment")

    # Get the list of installed packages via importlib.metadata. We use it instead of pip list because pip may not be
    # available in the environment, and because pip list output is not stable across versions. The command outputs the
    # list of installed packages names in lower case, separated by space.
    find_package(Python 3.8 REQUIRED) # We need Python >=3.8 to get the list of installed packages via
                                      # importlib.metadata.
    execute_process(
      COMMAND
        python -c
        "import importlib.metadata as m; names=[]; [names.append((d.metadata.get('Name') or '').strip().lower()) for d in m.distributions() if (d.metadata.get('Name') or '').strip()]; print(' '.join(sorted(set(names))))"
      OUTPUT_VARIABLE PIP_DISTRIBUTIONS_INFO
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(VAR_MPI_INFO "${PIP_DISTRIBUTIONS_INFO}")
  else()
    # Outside conda and pip, we use the path given by find_mpi
    set(DETECTION_MESSAGE "from standard environment")
    set(VAR_MPI_INFO "${MPI_LIBRARIES}")
  endif(IS_CONDA)

  # ERROR if VAR_MPI_INFO is not defined, it means either: - in standard environment find_mpi provides no MPI path (MPI
  # is not installed) - or in conda build, the 'mpi' variable is missing and find_mpi may find the system wide mpi, this
  # is not not what we want. - or in conda, outside of the build process, the mpi package is not installed and find_mpi
  # may find the system wide mpi.
  if(NOT DEFINED VAR_MPI_INFO OR "${VAR_MPI_INFO}" STREQUAL "")
    message(FATAL_ERROR "Missing information to discover the MPI implementation")
  endif()

  # Match implementation names case-insensitively because command outputs often use mixed case.
  string(TOLOWER "${VAR_MPI_INFO}" VAR_MPI_INFO_LOWER)

  # Find "openmpi", "mpich" or "intel" in the variable VAR_MPI_INFO_LOWER
  string(REGEX MATCH "openmpi|open-mpi" VAR_MATCH "${VAR_MPI_INFO_LOWER}")
  if(VAR_MATCH)
    set(MPI_IMPL "openmpi")
    set(IS_OPEN_MPI TRUE)
  endif()

  string(REGEX MATCH "mpich" VAR_MATCH "${VAR_MPI_INFO_LOWER}")
  if(VAR_MATCH)
    set(MPI_IMPL "mpich")
    set(IS_MPICH TRUE)
  endif()

  string(REGEX MATCH "intel|impi" VAR_MATCH "${VAR_MPI_INFO_LOWER}")
  if(VAR_MATCH)
    set(MPI_IMPL "intel")
    set(IS_INTEL_MPI TRUE)
  endif()

  if(MPI_IMPL)
    message(STATUS "Auto-detected MPI implementation: ${MPI_IMPL} (${DETECTION_MESSAGE})")
  else()
    message(STATUS "Unable to detect the MPI implementation: no suffix will be added to binaries name")
  endif()

  # Transmits variables to parent scope
  set(MPI_IMPL
      ${MPI_IMPL}
      PARENT_SCOPE)
  set(IS_OPEN_MPI
      ${IS_OPEN_MPI}
      PARENT_SCOPE)
  set(IS_MPICH
      ${IS_MPICH}
      PARENT_SCOPE)
  set(IS_INTEL_MPI
      ${IS_INTEL_MPI}
      PARENT_SCOPE)
endfunction()
