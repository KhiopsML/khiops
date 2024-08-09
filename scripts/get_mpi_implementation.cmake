# Detect MPI implementation. If an implementation is detected, MPI_IMPL is set with "openmpi", "mpich" or "intel". Use
# the global variable IS_CONDA
function(get_mpi_implementation)
  # On standard environment, we search implementation names in the MPI_LIBRARIES variable provided by find_mpi. Inside
  # conda environment, the library path is the same for all libraries. So we use the 'mpi' variable that is defined
  # (only) in the build environment. Outside the build environment, the 'mpi' variable is not defined, we use the list
  # of installed packages. It works because, the mpi packages are in conflict (only one implementation on the same
  # environment)
  if(IS_CONDA)
    if(DEFINED ENV{mpi})
      set(DETECTION_MESSAGE "from conda build environment")
      set(VAR_CONDA_MPI $ENV{mpi})
    else()
      set(DETECTION_MESSAGE "from conda standard environment")
      execute_process(
        COMMAND conda list
        COMMAND grep mpi
        OUTPUT_VARIABLE VAR_CONDA_MPI)
    endif()
  endif()

  # Outside conda or inside conda without mpi packages, we use the path given by find_mpi
  if(NOT DEFINED VAR_CONDA_MPI OR "${VAR_CONDA_MPI}" STREQUAL "")
    set(DETECTION_MESSAGE "from standard environment")
    set(VAR_CONDA_MPI "${MPI_LIBRARIES}")
  endif()

  if(NOT DEFINED VAR_CONDA_MPI OR "${VAR_CONDA_MPI}" STREQUAL "")
    message(FATAL_ERROR "Missing information to discover the MPI implementation")
  endif()

  # Find "openmpi", "mpich" or "intel" in the variable VAR_CONDA_MPI
  string(FIND "${VAR_CONDA_MPI}" openmpi POS)
  if(POS GREATER -1)
    set(MPI_IMPL "openmpi")
  endif()

  string(FIND "${VAR_CONDA_MPI}" mpich POS)
  if(POS GREATER -1)
    set(MPI_IMPL "mpich")
  endif()

  string(FIND "${VAR_CONDA_MPI}" intel POS)
  if(POS GREATER -1)
    set(MPI_IMPL "intel")
  endif()

  if(MPI_IMPL)
    message(STATUS "Auto-detected MPI implementation: ${MPI_IMPL} (${DETECTION_MESSAGE})")
  else()
    message(STATUS "Unable to detect the MPI implementation: no suffix will be added to binaries name")
  endif()

  # Transmits MPI_IMPL to parent scope
  set(MPI_IMPL
      ${MPI_IMPL}
      PARENT_SCOPE)
endfunction()
