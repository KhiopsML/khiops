# Khiops Test Tool: LearningTest and LearningTestTool
LearningTest
- created in March 2009
- automated tests for Khiops tools
- versions synchronized with delivered Khiops versions
  - one version per delivered Khiops version, with same tag
  - one version for the current branch under development

Non-regression tests consist of over 600 test sets organized into around 40 suites, mainly for Khiops, but also for Khiops coclustering and the KNI DLL.
They collectively occupy around 10 GB, including 5 GB for the databases and 3.5 GB for the test scripts with the reference results.

The following components are involved in the Khiops tests:
- tool binaries: binaries of the tools (Khiops, Coclustering and KNI) to test
- LearningTestTool: scripts that launch the tests and analyze the results
- LearningTest: datasets, tool scenarios that describe the tests and reference results

## Table of Contents

- [LearningTest](#learningtest)
  - [LearningTest directory tree](#learningtest-directory-tree)
  - [Test dirs](#test-dirs)
  - [Normalisation of paths in scenarios](#normalisation-of-paths-in-scenarios)
  - [Variants of reference results](#variants-of-reference-results)
- [LearningTestTool](#learningtesttool)
  - [Terminology](#terminology)
- [LearningTest commands](#learningtest-commands)
  - [LearningTestTool directory tree](#learningtesttool-directory-tree)
  - [Implementation of LearningTestTool](#implementation-of-learningtesttool)
- [Running Khiops tests](#running-khiops-tests)
- [Main usages](#main-usages)
  - [Test methodology](#test-methodology)
  - [Non-regression tests for development](#non-regression-tests-for-development)
  - [Non-regression tests for release](#non-regression-tests-for-release)
  - [Portability on new platform](#portability-on-new-platform)
  - [CI/CD](#cicd)
- [Evolutions of LearningTest](#evolutions-of-learningtest)
  - [New test dir](#new-test-dir)
  - [New test suite](#new-test-suite)
  - [Evolution of scenarios](#evolution-of-scenarios)
  - [Evolution of reference results](#evolution-of-reference-results)
- [Management of LearningTest and LearningTestTool](#management-of-learningtest-and-learningtesttool)
  - [Cloud URI Support in LearningTest](#cloud-uri-support-in-learningtest)
    - [Cloudification process](#cloudification-process)
    - [Step-by-step workflow](#step-by-step-workflow)




## LearningTest

The LearningTest directory stores all the datasets, tool scenarios and reference results.

### LearningTest directory tree

The _LearningTest dir_ contains one subdirectory per _dataset collection_:
- datasets: standard datasets
- MTdatasets: multi-table datasets
- TextDatasets: text datasets
- UnusedDatasets: datasets not currently in use

And one _tool dir_ per set of tests related to each tool:
- TestKhiops: tests for Khiops
- TestCoclustering: tests for Coclustering
- TestKNI: tests for KNI

Each tool dir is a two-level tree:
- Level 1: one _suite dir_ per test suite, e.g. _TestKhiops/Standard_
- Level 2: one _test dir_ per test, e.g. _TestKhiops/Standard/Iris_

Suite dirs prefixed with y_ or z_ (e.g. z_Work) are temporary suites.


### Test dirs

Each test dir is organized as follows:
- test.prm: scenario file to run the test
- ... : test-specific local data files, if any
- readme.txt: optional file with short information relative to the test
- results: sub-directory containing the results of running the tool with the script
- results.ref: sub-directory containing reference results
- comparisonResults.log: comparison report obtained by comparing results and results.ref

### Normalisation of paths in scenarios

The test.prm scenario files must be written independently of the LearningTest localization, by specifying
the paths of the relevant files, which must be relative to the LearningTest tree structure, using Linux-style syntax.

For example:
- `./SNB_Modeling.kdic` to access a specific local dictionary in the test dir
  - except for datasets defined in LearningTest/dataset root trees,
       datasets can have specific dictionaries or data per test dir
- `../../../datasets/Adult/Adult.txt` to access a dataset data file from a collection of datasets
- `./results/T_Adult.txt` for a result in the results sub-directory

### Variants of reference results

The reference results in the `results.ref` dir may vary depending on the context:
- computing type: sequential, parallel,
- platform: Windows, Linux, Darwin (macOS)

In this case, multiple reference result directories are used to store the results:
- naming convention: `results_ref<-(computing type)><-(platforms)>`
  - the '-' character is used to separate context information
  - the '_' character is used when several values exist for a given context
- the set of `results.ref` directories must cover all possible contexts
  - the base `results.ref` dir (without context) is used when specific contexts are not necessary
- examples of valid results.ref directories:
  - `results.ref-sequential`, `results.ref-parallel`
  - `results.ref-Windows`, `results.ref-Darwin_Linux`
  - `results.ref-parallel-Darwin_Linux`
- examples of valid sets of results.ref directories
  - [results.ref]: standard case
  - [results.ref, results.ref-parallel]: specialization for parallel computing
  - [results.ref, results.ref-Darwin_Linux]: specialization for Linux and Mac platforms
  - [results.ref, results.ref-Parallel, results.ref-Parallel-Darwin_Linux]: more complex contexts


## LearningTestTool

The LearningTestTool directory stores all the scripts used to test Khiops tools and analyze the results using a LearningTest directory.

### Terminology

The following terminology is used throughout the test environment:
- tools
  - `Khiops`: Khiops tool, with tool binary name `MODL`
  - `Coclustering`: Khiops tool, with tool binary name `MODL_Coclustering`
  - `KNI`: Khiops tool, with tool binary name `KNITransfer`
- typology of directories
  - `test dir`: one terminal test, e.g. `<path>/LearningTest/TestKhiops/Standard/Iris`
  - `suite dir`: a suite of tests, e.g. `<path>/LearningTest/TestKhiops/Standard`
  - `tool dir`: all suites for a given tool, e.g. `<path>/LearningTest/TestKhiops`
  - `learning test dir`: home directory of LearningTest, containing all tool dirs, e.g. `<path>/LearningTest`
- test family: subset of test suites per test dir
  - `basic`: `Standard` suite only per tool, around one minute of overall execution time
  - `full`: suites used for non-regression tests (default family), around one hour of overall execution time
  - `fullNoKNI`: same as `full`, without the KNI tool (e.g. to test the tool binaries of an installed Khiops desktop application)
  - `complete`: same as `full`, plus large scale tests, around one day of overall execution time
  - `all`: all suites (used to manage test dirs exhaustively, not to run tests)
- miscellaneous
  - `command`: one of the LearningTest scripts
  - `instruction`: instruction to perform on test dirs (e.g. `errors` to collect error stats)
  - `dataset collection`: collection of datasets (e.g. `datasets`, `MTdatasets`,...)

A subset of test dirs is defined using both a `LearningTest source dir` and a family:
- test dir: a single test dir
- suite dir: all test dirs of a test suite
- tool dir: all test suites for a specific tool in a given family
- LearningTest dir: all test suites for all tools in a given family

## LearningTest commands

All LearningTest commands are prefixed by 'kht_'.

Available LearningTest commands are
- **_kht_test_** (LearningTest source dir) (tool binaries dir) [options]
  - test a tool on a subset of test dirs
  - options:
	- family,
	- processes
	- forced-platform: for the context of reference results
	- min-test-time, max-test-time, test-timeout-limit
	- user-interface, task-file, output-scenario, nop-output-scenario
- **_kht_apply_** (LearningTest source dir) (instruction) [options]
  - apply an instruction on a subset of test dirs
  - options:
	- family,
	- processes,
	- forced-platform: for the context of reference results
	- min-test-time, max-test-time
- **_kht_collect_results_** (LearningTest source dir) (target dir) [options]
  - collect results from a subset of test dirs in a sub-dir of the target dir named LearningTest_results
  - options:
        - collect types: _errors_, _messages_, _all_ (default: _errors_)
	- family
- **_kht_export_** (LearningTest source dir) (target dir) [options]
  - export LearningTest tree for a subset of test dirs, to a sub-dir of the target dir named:
	- LearningTest if export type is _all_
	- LearningTest_(export type) otherwise
  - options:
        - export types: _all_, _scripts_, _references_, _datasets_ (default: _all_)
	- family
	- cloud-directory: cloudify dataset paths to a cloud URI (gs://, s3://, https://, etc.)
    - export is performed first, then the exported tree is transformed in a second step
	  - generates test-cloud.prm with cloudified dataset paths (alongside test.prm)
	  - cloudifies dataset paths in results.ref* reference result files
	  - detects and prevents re-cloudification of already-processed test trees
	  - requires --binaries option for JSON parameter expansion via khiops -j
	- binaries: required when --cloud-directory is specified; path to Khiops tool binaries directory or release/debug alias
- **_kht_env_**
  - show the status of the main environment variables used by the tool binaries
- **_kht_help_**
  - show this help message

Detailed help is available by typing in the name of a specific command.

Notes regarding the directory parameters:
- the kht commands can be applied on any LearningTest directory tree
- the directories can be absolute or relative
- commands benefit from shell autocompletion on most file systems to complete the parameters for the LearningTest|tool|family|test directory


### LearningTestTool directory tree

The LearningTestTool directory contains:
- files
  - README.md
- directories
  - py: all commands written in python
	- callable using `python <command> <operands>`
  - cmd: all commands contained in Windows shell files
	- callable from a Windows shell using `<command> <operands>`
  - sh: all commands contained in Linux shell files
	- callable from a Linux (or Mac) shell using `<command> <operands>`

### Implementation of LearningTestTool

Naming conventions
- follow the terminology presented above
  - except for the LearningTest dir, mostly named home dir in the code for concision reasons
- file and directory names and paths
  - files: error_file_name vs error_file_path
  - directories: suite_dir_name vs suite_dir (path not used in variable name)

All commands are implemented in Python:
- LearningTestTool/py
  - one `<command>.py` file per command
  - plus internal python files, prefixed by '_'
	- _kht_constants.py: common constants
	- _kht_utils.py: common utility functions
	- _kht_families.py: definition of families
	- _kht_results_management.py: management of context of reference results, sequential vs parallel and platform
  - _kht_standard_instructions.py: standard maintained instruction functions for the kht_apply command
  - _kht_one_shot_instructions.py: one-shot instruction functions for the kht_apply command
	- _kht_check_results.py: deep check by comparison of test and reference results
- LearningTestTool/cmd
  - one (command).cmd file per command
  - same implementation: redirection to the python command file
- LearningTestTool/sh
  - one (command) file per command
  - same implementation: redirection to the python command file

The kht_test.py source file:
- starts the tool according to execution options
- collect the standard outputs from stdout and stderr and the return code
- manages a timeout for overlengthy execution times
- is resilient to some expected output (ex: memory messages in debug mode)
- compares test and references results, with a summary in file comparisonResults.log
- ...

The _kht_check_results.py source file is the most critical one
- objective:
  - resilient cross-platform comparison of test and reference results
  - fully automatic, while being as robust as a human expert comparison
- by far the most complex portion of code
  - but pragmatic code fast to implement and test, avoiding over-design
  - constrained by fast evolution; reuse by copy-paste is acceptable
  - it is sometimes simpler to rework the content of specific test dirs
- main features to remain robust across contexts, with resilience to
  - value of the tool version is ignored everywhere
  - computing times in err.txt file are ignored
  - varying results depending on the context, sequential vs parallel, plus platform, with dedicated `results.ref` directories
  - messages in relation to resource limits, which may vary slightly depending on execution modes and platforms
  - "normal" failure in scenario ("Batch mode failure")
  - byte encoding of file names across platforms
  - poor handling of accented file names by zip
  - varying patterns in error messages
  - specific messages that occur only in sequential mode (100th, 1000th warning...)
  - varying order of messages in some rare cases, in parallel mode
  - approximate AUC estimate in the event of limited resources
  - epsilon variation of numerical results
  - ...

A difficult task is to design the Khiops algorithms with the aim of being reproducible, which has so far been achieved:
- using a portability layer that provides an abstraction of the underlying platform
- using the Parallel library that guarantees the same results whatever the resources are (RAM, number of cores)
  - a few exceptions:
  - for optimization purposes, some tasks are only run in parallel mode (e.g. pre-indexation of files)
	- the AUC criterion can be evaluated on a subset of the database when there is not enough RAM
- using truncations in some numerical results
- fixing the random seeds
- using sort with secondary sort criteria in case of equality, including fixed random values when a random choice is used
  - this is important because the ANSI sort function exploits a stable sort only on some platforms
- ...

## Running Khiops tests

Installing LearningTest on a new machine
- LearningTest
  - copy the LearningTest tree on local disk
  - or obtain it using a clone from gitlab (TODO)
- LearningTestTool
  - copy the LearningTestTool tree on local disk
  - or obtain it using a clone from the KhiopsML/khiops repo on github (TODO)
  - add the script directory (cmd or sh) to the path
- Install python
  - add python to the path

All commands can be called directly from any platform.

## Main usages

### Test methodology

The set of non-regression tests is very large, up to 10 GB and one day of computation time.
In practice, the tests are run in stages:
- **scale**
  - elementary: TestKhiops/Standard/IrisLight, around one second, even in debug mode
  - standard: TestKhiops/Standard, around one minute
  - _full_ family (default family): all non-regression tests, around one hour
  - _complete_ family: same as full, plus large scale tests, around one day
- **execution mode**
  - _full_ using sequential or parallel computing (use process number = 4)
  - debug mode for test dirs with short test time (typically max-test-time=5)
- **portability**
  - _full_ under different platforms
- **stress tests**
  - huge scale: monster datasets (tens to hundreds of GB) on powerful servers are sometimes used for stress tests:
	- datasets from the large scale learning challenge (DNA, 50 millions of instances with 200 records per instances)
	- Criteo (45 millions of instances)
	- OrangeCDR: 100000 instances with 35 million CDRs
	- ...
  - parallelization and performance: the same test scenario is used, but with different resources, RAM and number of processes.
  - ...

### Non-regression tests for development

The aim is to quickly check that the new source code has not introduced any regressions.

The main use of LearningTest is to carry out non-regression tests regularly during development.
The kht_test command is by far the most commonly used.

It is used in stage by increasing _scale_, up to the _full_ family.

In case of bug fix or new feature, new test dirs may be added to LearningTest: see section "Evolutions of LearningTest"

### Non-regression tests for release

The aim is to detect as many potential bugs as possible, in an extensive manner, in order to deliver a high-quality version of the tool.

In case of a release, massive tests are performed by _scale_, _execution mode_, _portability_, _stress tests_.

### Portability on new platform

The aim is check that the new platform is supported by the tool.

The tests must be run on the new platform, using the _full_ family.

### CI/CD

The tests can be run at different steps of the development process:
- pull request: automatic run of the tests using the _basic_ family
  - minimum check that the code compiles and runs on three platforms on standard test dirs
  - rapid and sufficient in most cases
  - in some cases, the requirement may be higher
	- for example, in the case of a major feature
	- it is up to the reviewer to impose this requirement, in agreement with the developer
	- the tests can be run manually using the _full_ family on the developer's platform
- release
  - the test must be run using the _full_ family on all supported platforms
  - this must be triggered manually before the release
  - this can also be triggered periodically (for example, once a month), for verification purposes

## Evolutions of LearningTest

### New test dir

A new test dir may be necessary in the following case:
- fix a bug
- new minor feature

In this case, a new test dir is first developed in a temporary working suite,
(e.g. `LearningTest/TestKhiops/z_Work`).

When the new test dir is completed with its scenario and reference results dir, it can be copied in an existing suite,
(e.g. `LearningTest/TestKhiops/BugsMultiTables`).

It might be referenced in the commit notes of the relevant github issue.

### New test suite

A new test dir may be necessary in the following case:
- new major feature

In this case, a new test suite is first developed in a new temporary working suite,
(e.g. `LearningTest/TestKhiops/z_TextVariables`).

When the development of the major feature is completed, the new test suite can become an "official suite"
(e.g. `LearningTest/TestKhiops/TextVariables`), and referenced in the _kht_families.py source of LearningTestTool.


### Evolution of scenarios

The khiops scenarios mirror the structure of the user interface, and are likely to evolve in a major release:
- when the user interface is redesigned for greater simplicity
- when major features are added, with new user parameters

In this case, the scenarios `test.prm` within each test dir have to be updated:
- first, save all the current LearningTest directory tree
  - using git lab
  - or locally on your disk using the kht_export command
- implement manually a prototype of the new scenario
  - copy one representative test dir in working suite
    (ex: `LearningTest/TestKhiops/Standard/Adult` in `LearningTest/TestKhiops/z_Work`)
  - register the new scenario using the tool in user interface mode with the `-o` option
  - adapt the scenario to the LearningTest constraints, such as normalisation of paths...
- implement a one-shot instruction (ex: `transformPrm`) in python source file `_kht_one_shot_instructions.py`
  to automatically transform the scenario from its old version to its new version
- use the test methodology to apply the `transformPrm` instruction by increasing scale, up to all the test dirs
- the reference results should remain the same, except in the case of a new feature
- _be pragmatic: coding the one-shot instruction may be tedious, and manual transformation of a
  small proportion of scenarios is acceptable and more time efficient_


### Evolution of reference results

Caution:
- avoid mixing up the evolution of scenarios and reference results
- updating the reference results is:
  - critical to keep quality
  - tedious and time-consuming to avoid introducing regressions into the tests
- wait for stabilization of the new algorithms before updating the reference results

When Khiops algorithms evolve, such as for example a new version of the SNB method, reference results are likely to evolve.

In this case, the reference results within each dir have to be updated:
- first, save all the current LearningTest directory tree
  - using git lab
  - or locally on your disk using the kht_export command
- test your new algorithm on temporary work suites first
- compare your test and results manually by double-checking the results
  - on one representative test dir (ex: `LearningTest/TestKhiops/Standard/Adult`)
  - on one representative test suite (ex: `LearningTest/TestKhiops/Standard`)
  - on some carefully chosen additional test dirs, representative of potential difficulties
- implement a one-shot instruction (ex: `compareSNBResults`) in python source file `_kht_one_shot_instructions.py`
  to automatically compare the new test results to the old reference results
  - ensure that anything except the new feature is unchanged (ex: data preparation results should be unchanged if the SNB only has evolved)
  - check that the new performance indicators (accuracy, AUC...) remain close to the old ones
  - check that the new computing times remain close to the old ones
- use the test methodology to apply the `compareSNBResults` instruction by increasing scale, up to all the test dirs
- when all is validated and double checked, update the new references using the `makeref` instruction
- _be rigorous and paranoid: the quality of the reference results is critical_

## Management of LearningTest and LearningTestTool

The LearningTestTool and LearningTest projects are closely linked to the development of Khiops core,
but their pace of development is different:
- Khiops core evolves rapidly
- LearningTest evolves slowly:
  - its main use is "read-only" for non-regression tests
  - it sometimes undergoes changes to fix bugs or develop minor or major features
- LearningTestTool rarely evolves
  - resilience methods sometimes need to be maintained as Khiops algorithms or output reports evolve

Currently, LearningTestTool and a tiny part of LearningTest (`basic` family) are maintained in the [Khiops github repo](https://github.com/KhiopsML/khiops)

The full LearningTest directory tree cannot be embedded in the Khiops github repo for the following reasons:
- scalability issue: around ten GB are necessary to store a snapshot of one version of LearningTest
- confidentiality: some datasets or test dir contain confidential data
- cost: storing and managing a large number of tests on dozens of platforms can be expensive in an external cloud

The full LearningTest is stored and versioned in an internal GitLab repository.

### Cloud URI Support in LearningTest

The LearningTest export tool supports exporting test trees with cloudified paths, where datasets are
read from a cloud storage URI (gs://, s3://, https://, etc.) while Khiops runs locally.

**What needs to be on the cloud**: datasets and directory hierarchy without results.ref.*
Reference results (results.ref* directories) and scenario files can remain local.

**Recommended workflow** (simplified): keep a single cloudified LearningTest tree on the local machine,
mirroring what is on the cloud. This simplifies path management and avoids maintaining separate trees.

#### Cloudification process

When exporting with `--cloud-directory`, the tool uses a two-step process:
1. **Standard export**: the requested LearningTest subtree is exported locally exactly as in a normal `kht_export` run
2. **Cloudification of the exported tree**: the exported files are then transformed in place

The cloudification step itself performs two operations:
1. **JSON parameter expansion**: Khiops scenarios (`test.prm`) that include JSON parameter files (`test.json`) are expanded using `khiops -j`
2. **Path substitution**: local dataset paths are replaced with cloud URIs in both scenario files and reference result files

Generated files per test dir:
- `test-cloud.prm`: scenario file with cloudified dataset paths, placed alongside test.prm
- `results.ref*/` files: reference result files updated with cloudified paths

When `kht_test` detects a `test-cloud.prm` file in a test dir, it uses that instead of `test.prm`.

#### Step-by-step workflow

1. **Export the LearningTest tree locally, then cloudify that exported tree automatically in a second step**:
   ```bash
   kht_export /home/bob/LearningTest -f basic --cloud-directory gs://khiops-test/LearningTest \
   --binaries build/macos-clang-release/bin/ /tmp
   ```
  This creates `/tmp/LearningTest`. The command first copies the selected LearningTest content, then rewrites the exported tree to add `test-cloud.prm` and cloudified `results.ref*` files.

2. **Upload the whole cloudified LearningTest tree** to cloud storage (datasets, scenarios, reference results):
   ```bash
   # Google Cloud Storage
   gcloud storage cp -r /tmp/LearningTest gs://khiops-test/LearningTest
   # Amazon S3
   aws s3 sync /tmp/LearningTest s3://khiops-test/LearningTest
   # Azure Blob Storage
   azcopy sync /tmp/LearningTest https://<account>.blob.core.windows.net/khiops-test/LearningTest
   ```
   The local `/tmp/LearningTest` and the cloud bucket are now synchronized mirrors.

3. **Run tests** with Khiops reading datasets from the cloud:
   ```bash
   kht_test /tmp/LearningTest -f basic build/macos-clang-release/bin/
   ```
   Scenario-driven output files whose paths are defined inside the scenario, such as `.khj`, `.kdic`, `.xls`, and other files written under `./results/`, are written to the cloudified result paths. On the other hand, `err.txt`, output scenarios (`output_test.prm`, `nop_output_test.prm`), the task file (`task_progression.log`), and all Python diagnostic files (`stdout_error.log`, `stderr_error.log`, `return_code_error.log`, `process_timeout_error.log`, `time.log`) are written **locally**. The automatic comparison at the end of `kht_test` is skipped in this mode, because the full `results/` directory is not yet available locally.
    > [!NOTE] Automatically clean results with native cloud command
    > Before launching Khiops, `kht_test` cleans the local `results/` directory and also removes the remote cloud `results/` directory using the native cloud command for the corresponding URI scheme (`gcloud storage rm` for `gs://`, `aws s3 rm` for `s3://`, `azcopy rm` for `https://`). This avoids stale result files from a previous run remaining on the cloud. Khiops then resolves dataset paths from the cloud URIs in `test-cloud.prm`.
4. **Sync the cloudified LearningTest tree back** before comparison.
   To avoid unnecessary recopies, prefer checksum-based comparison when the cloud tool supports it.
   ```bash
   # Google Cloud Storage
   gcloud storage rsync -r --checksums-only gs://khiops-test/LearningTest /tmp/LearningTest

   # Amazon S3
   # No exact checksum-only equivalent in aws s3 sync
   aws s3 sync s3://khiops-test/LearningTest /tmp/LearningTest

   # Azure Blob Storage
   azcopy sync 'https://<account>.blob.core.windows.net/khiops-test/LearningTest' '/tmp/LearningTest' \
     --compare-hash=MD5
   ```

5. **Compare results** against reference explicitly after synchronization:
   ```bash
   kht_test /tmp/LearningTest check -f basic
   ```

6. **Inspect failures** as usual if needed:
   ```bash
   kht_apply errors /tmp/LearningTest -f basic
   ```
