Khiops 11
==============
  (c) 2023-2025 Orange. All rights reserved.
  https://khiops.org

Khiops is a fully automatic tool for mining large multi-table databases,
winner of several data mining challenges.

Khiops components
- Khiops: supervised analysis (classification, regression) and correlation study
- Khiops Visualization: to visualize data preparation, modeling and evaluation results of Khiops
- Khiops Coclustering: exploratory analysis using hierarchical coclustering
- Khiops Covisualization: to visualize, explore and annotate coclustering results

Main features
- fully automatic data preparation (variable construction and preprocessing) and modeling
- mining of data with single table and multi-table schemas
- data preparation and modeling for classification, irrespective of the number of classes
- interpretation and reinforcement of classification models
- data preparation and modeling for regression
- descriptive statistics as well as correlation analysis for unsupervised data exploration
- advanced unsupervised analysis via coclustering
- enhanced recoding capabilities for data preparation
- post-evaluation of trained predictors
- robustness and scalability, with multi-gigabytes train datasets and no deployment limit
- parallelization of data management, data preparation, modeling and deployment tasks
- ease of use via simple user interface
- interactive visualization tools for easily interpretable results
- easy integration in information systems via batch mode, python library and online deployment library


Khiops 11 - what's new
----------------------
Text data:
- new Text type for variables in tabular or multi-table schema
- Automatic feature construction from Text variables

SNB classifier for sparse data: extension to sparse data

Random forests for regression

Khiops interpretation and reinforcement:
- Instance-based interpretation of scores
- Exact computation of Shapley values
- Importance of SNB selected variables is now computing using their mean absolute Shapley value
- Build an interpretation dictionary, to deploy interpretation values
- Build a reinforcement dictionary, to deploy reinforcement scores based on lever variables

Histograms: Optimal histograms for univariate data exploration

Coclustering instances x variables: extension of existing variable x variable coclustering 
  for joint density estimation, to instances x variables coclustering for exploratory analysis.

Visualization tools:
- visualization: new panel to visualize histograms
- covisualization: accounting for the case of instances x variables coclustering

Simplified ergonomy
- simplification of panels and fields, everywhere, as much as possible
- fast path: to train a model without a dictionary
- visualization of reports and editing of dictionaries from the graphical interface

Extended scenario-based management of Khiops, with control structures and a parameter file in JSON format


Technical prerequisites
-----------------------
Configuration:
- Windows 10 or Higher, 64 bits
- Linux (supported distributions: see https://khiops.org), 64 bits
- macOS (via conda, see https://khiops.org), intel and ARM, 64 bits

Windows software:
- Microsoft MPI 10.1.3
- Automatic detection and silent installation during the Khiops installation process on Windows
- For a silent installation, run the installer with /S option, and /D=installationDir to choose a
  specific installation directory.

Linux software:
- Java Runtime Environment V7 or higher, mpich (>3.0), libstdc++6
- Automatic detection and silent installation during the Khiops installation process

macOS and other platforms: see https://khiops.org


Khiops files on Windows
-----------------------
Install location (usually C:\Program Files\khiops):
- README.txt: this file
- WHATSNEW.txt: detailed release notes
- LICENSE.txt: license file
- Shortcuts to Khiops components
- bin sub-directory:
  - executable, batch files and libraries
- jre sub-directory:
  - Java Runtime Environment provided by justj

Other locations:
- %USERPROFILE%\khiops_data\lastrun directory (usually C:\Users\<USERNAME>\khiops_data\lastrun):
  - contains scenario and log files for last run of Khiops and Khiops Coclustering

- %PUBLIC%\khiops_data\samples (usually C:\Users\Public\khiops_data\samples)
  - directory tree with database samples (database files `.txt` and
    dictionary files `.kdic`) see samples/README.txt


Khiops files on Linux
---------------------
/usr/bin:
- executable, batch files

/usr/share/doc/khiops:
- README.txt: this file
- WHATSNEW.txt: detailed release notes

/tmp/khiops/$USER:
- contains scenario and log files for last run of Khiops and Khiops Coclustering
