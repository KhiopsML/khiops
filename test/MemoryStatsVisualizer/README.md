# Memory stats visualizer

Visualizer for memory stats logs produced by the Khiops tool

It takes log file as input and visualize them in a browser, with curves per memory stats and summary bars per process.
It also build some statistics summary of the log in tabular or Excel files.

## Producing memory stats logs

Khiops binaries can produce logs that summarise resource consumption for cores, CPU, memory and I/O.

The log are produced on demands, depending on environment variables.
These environment variables are recalled using the kht_env script of the LearningTestTool:
- KhiopsMemStatsLogFileName: None, memory stats log file name
- KhiopsMemStatsLogFrequency: None, frequency of allocator stats collection (0, 100000, 1000000,...)
- KhiopsMemStatsLogToCollect: None, stats to collect (8193: only time and labels, 16383: all,...)
- KhiopsIOTraceMode: None, to collect IO trace (false, true)

The detailed valued usable for KhiopsMemStatsLogToCollect are detailed in the appendix at the end of this document.

Warning: the KhiopsIOTraceMode produces a lot of logs: use it only when necessary.


## Samples of stats logs

Log directories:
- AdultSample: logs obtained in sequential for Adult dataset
- AdultParallelSample: logs obtained in parallel for Adult dataset

Producing the log using kht_test:
- have the LearningTestTool script in the path
- go to the MemoryStatsVisualizer directory
- set environment variables and run the test in sequential mode
  (under Windows in the example below)
~~~~
set KhiopsMemStatsLogFileName=%cd%\AdultSample\KhiopsMemoryStats.log
set KhiopsMemStatsLogFrequency=100000
set KhiopsMemStatsLogToCollect=16383
set KhiopsIOTraceMode=true
kht_test ..\LearningTest\TestKhiops\Standard\Adult r
~~~~
- idem in parallel mode
~~~~
set KhiopsMemStatsLogFileName=%cd%\AdultParallelSample\KhiopsMemoryStats.log
kht_test ..\LearningTest\TestKhiops\Standard\Adult r -p 6
~~~~

## Exploiting memory stats logs

Once the logs are available, the memory stats visualizer python scripts can be used to
produce synthetic visualization and summaries of the logs:
- memory_stats_visualizer: build a synthetic visualization of the logs and open it in a browers
- collect_stats_by_task: build a summary text file with stats per task
- compute_and_write_aggregates: build a summary Excel file with I/O stats per task

See sample.py for an example of the use of each python script.

This is an interactive display tool:
- widgets are available in the top right-hand corner, for example to zoom in on a sub-section of the display
- the legend on the right can be used to select what needs to be viewed and the statistics to be displayed
- tooltips are available, showing detailed statistics for each selected part of the display
- ...


#### Appendix: detailed settings for the logs to be collected

In the following example, "LogTime=1 : Time (*time associated with the log*)":
- Time: identifier
- 1: value to use for environment variable KhiopsMemStatsLogToCollect
- Time: label of the column if the file containing the logs (cf. KhiopsMemStatsLogFileName)
- *time associated with the log*: label

The following constants are used to select the statistics to be collected:
- LogTime=1 : Time (*time associated with the log*)
- HeapMemory=2 : Heap mem (*current size of the heap*)
- MaxHeapRequestedMemory=4 : Max heap mem (*maximum size requested for the heap*)
- TotalHeapRequestedMemory=8 : Total heap mem (*total cumulative size requested for the heap*)
- AllocNumber=16 : Alloc (*current number of allocations*)
- MaxAllocNumber=32: Max alloc (*maximum number of allocations*)
- TotalAllocNumber=64: Total alloc (*total number of allocations*)
- TotalFreeNumber=128: Total free (*total number of memory free*)
- GrantedSize=256 : Granted (*current size allocated*)
- MaxGrantedSize=512 : Max granted (*maximum size allocated*)
- TotalRequestedSize=1024 : Total requested size
- TotalGrantedSize=2048 : Total granted size
- TotalFreeSize=4096 : Total free size (*total size freed*)
- LogLabel=8192 : Label (*user label*)

To collect several statistics simultaneously, the corresponding constants must be added together:
  - NoStats=0 (*no statistics*)
  - AllStats=16383 (*all statistics*)
- Some potential useful combinations
  - LogInfo=8193 (*info per log*)
	- LogTime+LogLabel
  - HeapStats=14 (*statitics on the heap*)
    - HeapMemory + MaxHeapRequestedMemory + TotalHeapRequestedMemory
  - AllocStats=8176 (*statistics on allocations*)
	- AllStats - LogInfo - HeapStats
  - AllocCurrentStats=272 (*statistics on current allocations*)
	- AllocNumber + GrantedSize
  - AllocMaxStats=544 (*statistics on max allocations*)
	- MaxAllocNumber + MaxGrantedSize (*stats on max allocations*)
  - AllocNumberStats=240 (*statistics on the number of allocations*)
	- AllocNumber + MaxAllocNumber + TotalAllocNumber + TotalFreeNumber
  - AllocSizeStats=7936 (*statistics on allocation sizes*)
	- GrantedSize + MaxGrantedSize + TotalRequestedSize + TotalGrantedSize +

Reference: src\Norm\base\MemoryStatsManager.h