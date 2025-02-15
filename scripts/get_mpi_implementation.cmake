# Detect MPI implementation. If an implementation is detected, MPI_IMPL is set with "openmpi", "mpich" or "intel".
# Uses the global variable IS_CONDA.
function(get_mpi_implementation)
  # On a standard environment, we search for implementation names in the MPI_LIBRARIES variable provided by find_mpi.
  # In a conda environment, the library path is the same for all libraries. So we use the 'mpi' variable (if defined)
  # in the build environment; if not, we use the list of installed packages.
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
  else()
    # Outside conda, use the information from find_mpi.
    set(DETECTION_MESSAGE "from standard environment")
    set(VAR_MPI_INFO "${MPI_LIBRARIES}")
  endif()

  # Error out if VAR_MPI_INFO is not defined or empty.
  if(NOT DEFINED VAR_MPI_INFO OR "${VAR_MPI_INFO}" STREQUAL "")
    message(FATAL_ERROR "Missing information to discover the MPI implementation")
  endif()

  # Convert the MPI information to lowercase to simplify matching.
  string(TOLOWER "${VAR_MPI_INFO}" VAR_MPI_INFO_LOWER)

  # Check for OpenMPI. Some systems report "openrte" (which is part of OpenMPI), so check for both.
  string(FIND "${VAR_MPI_INFO_LOWER}" "openmpi" POS_OPENMPI)
  string(FIND "${VAR_MPI_INFO_LOWER}" "openrte" POS_OPENRTE)
  if(POS_OPENMPI GREATER -1 OR POS_OPENRTE GREATER -1)
    set(MPI_IMPL "openmpi")
  endif()

  # Check for MPICH.
  string(FIND "${VAR_MPI_INFO_LOWER}" "mpich" POS_MPICH)
  if(POS_MPICH GREATER -1)
    set(MPI_IMPL "mpich")
  endif()

  # Check for Intel MPI.
  string(FIND "${VAR_MPI_INFO_LOWER}" "intel" POS_INTEL)
  if(POS_INTEL GREATER -1)
    set(MPI_IMPL "intel")
  endif()

  if(MPI_IMPL)
    message(STATUS "Auto-detected MPI implementation: ${MPI_IMPL} (${DETECTION_MESSAGE})")
  else()
    message(STATUS "Unable to detect the MPI implementation: no suffix will be added to binaries name")
  endif()

  # Transmit MPI_IMPL to the parent scope.
  set(MPI_IMPL ${MPI_IMPL} PARENT_SCOPE)
endfunction()
