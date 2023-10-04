Khiops 10.0
===========
  (c) 2023 Orange Labs - All rights reserved.
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
  - data preparation and modeling for regression
  - descriptive statistics as well as correlation analysis for unsupervised data exploration
  - advanced unsupervised analysis via coclustering
  - enhanced recoding capabilities for data preparation
  - post-evaluation of trained predictors
  - robustness and scalability, with multi-gigabytes train datasets and no deployment limit
  - parallelization of data management, data preparation, modeling and deployment tasks
  - ease of use via simple user interface
  - interactive visualization tools for easily interpretable results
  - easy integration in information systems via batch mode, python library and
    online deployment library

Khiops 10.0 - what's new
    - New algorithm for Selective Naive Bayes predictor
        - improved accuracy using a direct optimization of variable weights,
        - improved interpretability and faster deployment time, with less variables selected,
        - faster training time, using the new algorithm and exploiting parallelization,
    - Improved random forests
        - faster and more accurate preprocessing,
        - biased random selection of variable to better deal with large numbers of variables,
    - Management of sparse data
        - fully automatic,
        - potentially faster algorithms in case of many constructed variables,
    - New visualization tool
        - available on any platform: windows, linux, mac, both in standalone and using a browser
    - Parallelization on clusters of machine
        - available on Hadoop (Yarn, HDFS)
    - New version of pykhiops
        - more compliant with python PEP8 standard, using snake case
        - distributed as a python package
        - new features are available: see pykhiops release notes

Upward compatibility with Khiops 9
  - dictionaries of Khiops 9 are readable with Khiops 10
  - visualization reports of Khiops 9 are usable with the former visualization tool
  - python scripts using pykhiops 9 are running, with warnings for the deprecated features
  - scenarios of Khiops 9 are compatible, with warnings for the deprecated features
  - removed features in Khiops 10, that still work when used from former python scripts or scenarios:
    - MAP Naive Bayes is removed
    - Naive Bayes predictor is removed
    - Preprocessing options MODLEqualWidth, MODLEqualFrequency and MODLBasic are removed
  - deprecated features, that still work but will be removed in next versions after Khiops 10:
    - pykhiops 9 is replaced by pykhiops 10 (see migration guide within pykhiops release notes)


Technical prerequisites
-----------------------
Configuration:
   - PC Windows (Seven and higher), 64 bits
   - Linux (supported distributions: see https://khiops.org), 64 bits

Windows software:
   - Java Runtime Environment V7 or higher
   - Automatic detection and silent installation during the Khiops installation process on Windows
   - For specific installation (other than default installation), search "get Java" on your search
     engine to obtain the installer for each prerequisite component, and proceed with a manual
     installation
   - For a silent installation, run the installer with /S option, and /D=installationDir to choose a
     specific installation directory.

Linux software:
	Java Runtime Environment V7 or higher, mpich (>3.0), libstdc++6
	Automatic detection and silent installation during the Khiops installation process


Other configurations (ex. macOS): see https://khiops.org


Khiops files on Windows
-----------------------
Install location (usually C:\Program Files\khiops):
  - README.txt: this file
  - WHATSNEW.txt: detailed release notes
  - LICENSE.txt: license file
  - Shortcuts to Khiops components
  - bin sub-directory:
    - executable, batch files and libraries
  - doc sub-directory:
    - reference guides and tutorial for Khiops components
    - start with KhiopsTutorial.pdf

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
  - reference guides and tutorial for Khiops components
  - start with KhiopsTutorial.pdf
  - README.txt: this file
  - WHATSNEW.txt: detailed release notes
  - samples directory:
	  directory tree with database samples (database files `.txt` and dictionary files `.kdic`)
	  see samples/README.txt

/tmp/khiops/$USER:
  - contains scenario and log files for last run of Khiops and Khiops Coclustering
