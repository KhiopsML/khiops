# Unit tests in the Khiops project

We use the [GoogleTest framework](http://google.github.io/googletest/). This framework offers many features but we use only few of them. The main idea is that we can change the test framework easily.

To facilitate the transformation of the in-class methods into the googletest framework, we have defined a macro KHIOPS_TEST in [test/UnitTests/Utils/TestServices.h](../../test/UnitTests/Utils/TestServices.h). This macro makes it easier to write a unit test when you already have a test method that writes its results to standard output. Some examples are available in the unit tests of the base library: [test/UnitTests/Norm/Nom_test.cpp](../../test/UnitTests/Norm/Nom_test.cpp).

The test directory contains 5 directories dedicated to the unit tests:
- Norm: to test Norm libraries
- Learning: to test Learning libraries, produces the binary `learning_test`
- Parallel: to test sequential features of the Parallel library, produces `parallel_test`
- Parallel-mpi: to test parallel features, produces `parallel_mpi_test`. The binary can be launched in standalone mode (run sequential and simulated parallel) and with `mpiexec`.
- Utils: utils to run parallel test and new macros to run old style tests (compare stdout with a ref file).

## Unit Tests and mpi

To run unit tests with mpi and the parallel library, we provide an [environment](../../test/UnitTests/Utils/MpiEnvironment.h) and a [fixture](../../test/UnitTests/Utils/ParallelTest.h). They are used in the project [test/UnitTests/Parallel-mpi](../../test/UnitTests/Parallel-mpi). This project is intended to contain all the parallel tests: any new google test on this binary could be run in parallel (the binary must be launched with the mpiexec command)

## Launch unit tests

### From the test executables

When the project is compiled, the following test executables are compiled and made available in the bin directory ("build/windows-msvc-release/bin" under Windows):
- norm_test
- parallel_test
- parallel_mpi_test
- learning_test

To run a test executable, open a shell and run the executable: all associated tests will be executed and a report written to the shell.

The test results are produced in a unit test directory (e.g. "test/UnitTests/Learning/results"). In the events of errors, you can compare the results with the reference results available in the "results.ref" directory.

### From the Visual C++ IDE under Windows

Go to the test menu, then to the test explorer to select individual tests to run.



