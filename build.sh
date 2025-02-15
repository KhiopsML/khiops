#!/bin/bash
# build.sh - Configure the Khiops build in Debug mode with OpenMPI

# Make sure you're in the Khiops root directory (where CMakeLists.txt is located)
# For example: ~/Document/gitkhyops/khiops

# Create the build directory if it doesn't already exist
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR" || { echo "Failed to enter build directory"; exit 1; }

# Set the environment variable to help CMake detect the MPI implementation
export mpi=openmpi

# Define the MPI options
MPI_IMPL="openmpi"
MPI_EXECUTABLE="/usr/bin/mpiexec"
MPI_CXX_INCLUDE_PATH="/usr/lib/x86_64-linux-gnu/openmpi/include"
MPI_CXX_LIBRARIES="/usr/lib/x86_64-linux-gnu/openmpi/lib/libmpi_cxx.so"
MPI_CXX_COMPILER="/usr/bin/mpic++"

# Run CMake with the defined options
echo "Running CMake configuration..."
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DMPI_IMPL=${MPI_IMPL} \
      -DMPI_EXECUTABLE=${MPI_EXECUTABLE} \
      -DMPI_CXX_INCLUDE_PATH=${MPI_CXX_INCLUDE_PATH} \
      -DMPI_CXX_LIBRARIES=${MPI_CXX_LIBRARIES} \
      -DMPI_CXX_COMPILER=${MPI_CXX_COMPILER} \
      ..

# Check if configuration was successful
if [ $? -eq 0 ]; then
    echo "Configuration successful!"
else
    echo "Configuration failed. Please check the error messages above."
fi
