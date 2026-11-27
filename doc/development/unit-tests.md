# Unit tests in the Khiops project

We use the [GoogleTest framework](http://google.github.io/googletest/). It offers many features, but we use only a few. The main idea is to keep the test framework easy to replace.

To facilitate the conversion of in-class test methods to GoogleTest, we have defined the `KHIOPS_TEST` macro in [test/UnitTests/Utils/TestServices.h](../../test/UnitTests/Utils/TestServices.h). This macro makes it easier to write a unit test when you already have a test method that writes its results to standard output. Examples are available in the base library's unit tests: [test/UnitTests/Norm/Nom_test.cpp](../../test/UnitTests/Norm/Nom_test.cpp).

The test directory contains five subdirectories dedicated to unit testing:
- Norm: tests the Norm libraries
- Learning: tests the Learning libraries and produces the `learning_test` executable
- Parallel: tests sequential features of the Parallel library and produces the `parallel_test` executable
- Parallel-mpi: tests parallel features and produces the `parallel_mpi_test` executable. It can run in standalone mode (running sequential and simulated parallel tests) or with `mpiexec`.
- Utils: contains utilities for running parallel tests and macros for running legacy tests (which compare standard output with a reference file).

## Unit Tests and MPI

To run unit tests with MPI and the Parallel library, we provide an [environment](../../test/UnitTests/Utils/MpiEnvironment.h) and a [fixture](../../test/UnitTests/Utils/ParallelTest.h). They are used in `test/UnitTests/Parallel-mpi`, which is intended to contain all parallel tests. Any new GoogleTest case in this project can be run in parallel (the executable must be launched with `mpiexec`).

## Launch unit tests

### From the test executables

Once the project is built, the following test executables are placed in the `bin` directory (for example, `build/windows-msvc-release/bin` on Windows):
- norm_test
- parallel_test
- parallel_mpi_test
- learning_test

To run a test executable, open a shell and run the executable: all associated tests will be executed and a report written to the shell.

Test results are written to a unit test directory (e.g., `test/UnitTests/Learning/results`). If any errors occur, you can compare the results with the reference results in the `results.ref` directory.

### From the Visual Studio IDE on Windows

Go to **Test > Test Explorer** to select individual tests to run.



