# Khiops tests suite: LearningTest

LearningTest
- created in March 2009
- automated testing of Khiops software
- versions synchronized with delivered Khiops versions
  - one version per delivered Khiops version, with same tag
  - one version for the current branch under development

Non-regression tests consist of over 600 test sets organized into around 40 families, mainly for Khiops, but also for Khiops coclustering and the KNI DLL.
They collectively occupy around 11 Gb, including 5 Gb for the databases and 3.5 Gb for the test scripts with the reference results.

## LearningTest directory contents

### Main directories

Directories of LearningTest:
- doc: documentation
- cmd: command file for test management under windows
  - cmd/pythons: python scripts for running tests and managing test results
  - cmd/modl: directory that contains specific versions of exe files to test
- datasets: standard datasets
- MTdatasets: multi-table datasets
- TextDatasets: text datasets
- UnusedDatasets: datasets not currently in use
- TestKhiops: tests for Khiops
- TestCoclustering: tests for Coclustering
- TestKNITransfer: test for KNI

Each test directory tree, TestKhiops, TestCoclustering, TestKNITransfer is a two-level tree:
- Level 1: one directory per test family
- Level 2: one directory per test set

Subdirectories prefixed with y_ or z_ (e.g. z_Work) are temporary test directories.


### Test set directories

Each test directory is organized as follows:
- test.prm: scenario file to run the test
- ... : test-specific local data files, if any
- results: sub-directory containing the results of running the tool with the script
- results.ref: sub-directory containing reference results
- comparisonResults.log: test results obtained by comparing results and results.ref

The test.prm scenario files must be written independently of the LearningTest localization, by modifying the paths of the files concerned, which must be relative to the LearningTest tree, with linux-like syntax.
For example:
- `./SNB_Modeling.kdic` to access a specific dictionary local to the test directory
  - except for datasets defined in LearningTest/dataset root trees,
       datasets can have specific dictionaries or data per test directory
- `../../../datasets/Adult/Adult.txt` to access a dataset data file
- `/results/T_Adult.txt` for a result in the results sub-directory


## Running Khiops tests

Installing LearningTest on a new machine
- copy the LearningTest directory tree
- Install python
- put python in the path

### Personnalisation if necessary
- modify learning_test.config file in directory LearningTest/cmd/python
~~~~
# The config file learning_test.config must be in directory LearningTest\cmd\python
# It is optional, in which case all keys are set to empty
# It contains the following key=value pairs that allows a personnalisation of the environment:
#  - path: additional path (eg: to access to java runtime)
#  - classpath: additional classpath for java libraries
#  - learningtest_root: alternative root dir to use where LearningTest is located
#  - learning_release_dir: dir where the release developement binaries are located (to enable the 'r' alias')
#  - learning_debug_dir: dir where the debug developement binaries are located (to enable the 'd' alias')
~~~~

## Using LearningTest on any platform

The commands are are available using python scripts located in directory LearningTest/cmd/python (alias [ScriptPath]).
~~~~
python [ScriptPath]/[script].py
~~~~

Commands launched without arguments are self-documented

#### help_options.py
Show the status of environnement variables used by the scripts.
Main environment variables are:
- KhiopsMPIProcessNumber: Number of MPI processes in parallel mode
- KhiopsCompleteTests: perform all tests, even the longest ones (more than one day overall)
- KhiopsMinTestTime: run only tests where run time (in file time.log) is beyond a threshold
- KhiopsMaxTestTime: run only tests where run time (in file time.log) is below a threshold

#### test_khiops.py
Example:
- python [ScriptPath]/test_khiops.py Khiops r Standard
- python [ScriptPath]/test_khiops.py Khiops r Standard IrisLight
- python [ScriptPath]/test_khiops.py Coclustering r Standard Iris
~~~~
test [toolName] [version] [testName] ([subTestName])
  run tests of one of the Khiops tools
	tool_name: name of the tool, among Khiops, Coclustering, KNI
	version: version of the tool, one of the following options
	  <path_name>: full path of the executable
	  d: debug version in developpement environnement
	  r: release version in developpement environnement
	  ver: <toolname>.<ver>.exe in directory LearningTest\cmd\modl
	  nul: for comparison with the test results only
	testName: name of the tool test directory (Standard, MultiTables...)
	subTestName: optional, name of the tool test sub-directory (Adult,Iris...)
~~~~

#### test_khiops_all.py
Example
- python [ScriptPath]/test_khiops_all.py r
- python [ScriptPath]/test_khiops_all.py [MODL_PATH] Khiops
~~~~
testAll [version] <tool>
  run all tests for all Khiops tools
	version: version of the tool
	  d: debug version in developpement environnement
	  r: release version in developpement environnement
	  ver: <toolname>.<ver>.exe in directory LearningTest\cmd\modl
	  nul: for comparison with the test results only
	  full exe path, if <tool> parameter is used
	tool: all tools if not specified, one specified tool otherwise
	  Khiops
	  Coclustering
	  KNI
~~~~

#### apply_command.py
Example:
- python [ScriptPath]/apply_command.py errors TestKhiops/Standard

~~~~
apply_command [command] [root_path] ([dir_name])
  apply command on a directory structure
	command: name of the command
	rootPath is the path of the root directory
	dirName is the name of one specific sub-directory
	         or all (default) for executing on all sub-directories
   example: applyCommand list TestKhiops\Standard
   example: applyCommand list TestKhiops\Standard Adult

 List of available standard commands (* for all commands):
	list: list of sub-directories
	errors: report errors and warnings
	logs: detailed report errors and warnings
	compareTimes: compare time with ref time and report warnings only
	compareTimesVerbose: compare time with ref time and report all
	performance: report SNB test accuracy
	performanceRef: report ref SNB test accuracy
	clean: delete results files
	cleanref: delete results.ref files
	makeref: copy results files to results.ref
	copyref: copy results.ref files to results
	checkHDFS: check if parameter files are compliant with HDFS
	transformHDFS: transform parameter files to be compliant with HDFS
	transformHDFSresults: transform results files to be compliant with HDFS
~~~~

#### apply_command_all.py
Example:
- python [ScriptPath]/apply_command_all.py errors

~~~~
applyCommandAll [command] <*>
  apply command on all test sub-directories
	command: name of the command
	*: to include 'unofficial' sub-directories, such as z_work
  Type applyCommand to see available commands
~~~~


### Using LearningTest under Windows

Commands are available using command files (.cmd) located in  directory LearningTest/cmd, that are simply wrappers to the python scripts:
- helpOptions
- testKhiops
- testCoclustering
- testKNI
- testAll
- applyCommand
- applyCommandAll

Typical use
- open a shell
- run a command found in learningTest/cmd
- run the tests, for example
~~~
	TestKhiops r Standard Adult
	TestKhiops r Standard
	TestAll r
~~~
- analyze results, for example
~~~
	ApplyCommand errors TestKhiops/Standard
	ApplyCommandAll errors
~~~


## Test methodology

### Test hierarchy

The set of non-regression tests is voluminous. In practice, the tests are run in stages:
- elementary: TestKhiops Standard IrisLight, less than one second
- standard: TestKhiops Standard, less than one minute
- all : TestAll, less than two hours
- complete: TestAll in KhiopsCompleteTests mode (see help_options), more than one day
- release: the multiplication of test conditions reinforces the tool's robustness
  - TestAll under different platforms
  - TestAll in sequential or parallel mode (cf. KhiopMPIProcessNumber)
  - Test in debug mode for short test runs (cf KhiopsMinTestTime, KhiopsMaxTestTime)



