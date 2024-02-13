# Khiops Unit tests

5 directories :

- Norm: to test Norm libraries
- Learning: to test Learning libraries, produces the binary `learning_test`
- Parallel: to test sequential features of the Parallel library, produces `parallel_test`
- Parallel-mpi: to test parallel features of the Parallel library, produces `parallel_mpi_test`. The binary can be launched in standalone mode (run sequential and simulated parallel) and with `mpiexec`.
- Utils: utils to run parallel test and new macros to run old style tests (compare stdout with a ref file).
