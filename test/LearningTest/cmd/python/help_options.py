import os
from test_dir_management import *

print(
    "KhiopsBatchMode: "
    + str(os.getenv("KhiopsBatchMode"))
    + "\n\ttrue, false (default: true)"
)

print(
    "KhiopsMinTestTime: "
    + str(os.getenv("KhiopsMinTestTime"))
    + "\n\trun only tests where run time (in file "
    + TIME_LOG
    + ") is beyond a threshold"
)

print(
    "KhiopsMaxTestTime: "
    + str(os.getenv("KhiopsMaxTestTime"))
    + "\n\trun only tests where run time (in file "
    + TIME_LOG
    + ") is below a threshold"
)

print(
    "KhiopsTestTimeoutLimit: "
    + str(os.getenv("KhiopsTestTimeoutLimit"))
    + "\n\tkill overlengthy process (default: 300 s)"
)

print(
    "KhiopsMPIProcessNumber: "
    + str(os.getenv("KhiopsMPIProcessNumber"))
    + "\n\tNumber of MPI process in paralle mode (default: None)"
)

print(
    "KhiopsExpertMode: "
    + str(os.getenv("KhiopsExpertMode"))
    + "\n\tKhiops expert mode true, false (default: false)"
)

print(
    "KhiopsTaskFileMode: "
    + str(os.getenv("KhiopsTaskFileMode"))
    + "\n\tCreate a task file task.log (-t option) (default: None)"
)

print(
    "KhiopsOutputScenarioMode: "
    + str(os.getenv("KhiopsOutputScenarioMode"))
    + "\n\tCreate an output scenario test.output.prm (-o option) (default: None)"
)

print(
    "KhiopsCompleteTests: "
    + str(os.getenv("KhiopsCompleteTests"))
    + "\n\tPerform all tests, even the longest ones (default: false)"
)


print("")
print(
    "KhiopsComparisonPlatform: "
    + str(os.getenv("KhiopsComparisonPlatform"))
    + "\n\tplatform (Windows, Linux, Darwin, WSL) used to compare test results (default: None, to use that of current OS)"
)

print(
    "KhiopsPreparationTraceMode: "
    + str(os.getenv("KhiopsPreparationTraceMode"))
    + "\n\tTrace for dimensionnining of preparation tasks (default: false)"
)
print(
    "KhiopsParallelTrace: "
    + str(os.getenv("KhiopsParallelTrace"))
    + "\n\tTrace for parallel tasks (0 to 3)"
)

print(
    "Analysis of memory stats\n"
    + "\tKhiopsMemStatsLogFileName: "
    + str(os.getenv("KhiopsMemStatsLogFileName"))
    + ", memory stats log file name\n"
    + "\tKhiopsMemStatsLogFrequency: "
    + str(os.getenv("KhiopsMemStatsLogFrequency"))
    + ", frequency of allocator stats collection (0, 100000, 1000000,...) \n"
    + "\tKhiopsMemStatsLogToCollect: "
    + str(os.getenv("KhiopsMemStatsLogToCollect"))
    + ", stats to collect (8193: only time and labels, 16383: all,...)\n"
    + "\tKhiopsIOTraceMode: "
    + str(os.getenv("KhiopsIOTraceMode"))
    + ", to collect IO trace (false, true)"
)
