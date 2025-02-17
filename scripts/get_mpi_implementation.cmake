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
      set(VAR_MPI_INFO $ENV{mpi})
    else()
      set(DETECTION_MESSAGE "from conda standard environment")
      execute_process(
        COMMAND conda list
        COMMAND grep mpi
        OUTPUT_VARIABLE VAR_MPI_INFO)
    endif()
  else(IS_CONDA)
    # Outside conda, we use the path given by find_mpi
    set(DETECTION_MESSAGE "from standard environment")
    set(VAR_MPI_INFO "${MPI_LIBRARIES}")
  endif(IS_CONDA)

  # If no MPI library information is detected (i.e. VAR_MPI_INFO is undefined or empty),
  # then we check if MPI_IMPL was provided externally. If so, we use the provided MPI_IMPL,
  # otherwise, we terminate with a fatal error. This allows us to use a system-defined MPI
  # implementation when building in a conda environment that does not list MPI via "conda list | grep mpi".
  if(NOT DEFINED VAR_MPI_INFO OR "${VAR_MPI_INFO}" STREQUAL "")
    if(DEFINED MPI_IMPL)
      message(STATUS "Using provided MPI_IMPL: ${MPI_IMPL}")
    else()
      message(FATAL_ERROR "Missing information to discover the MPI implementation")
    endif()
  endif()

  # Find "openmpi", "mpich" or "intel" in the variable VAR_MPI_INFO
  string(FIND "${VAR_MPI_INFO}" openmpi POS)
  if(POS GREATER -1)
    set(MPI_IMPL "openmpi")
  endif()

  string(FIND "${VAR_MPI_INFO}" mpich POS)
  if(POS GREATER -1)
    set(MPI_IMPL "mpich")
  endif()

  string(FIND "${VAR_MPI_INFO}" intel POS)
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
