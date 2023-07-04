import os

print(
    "KhiopsBatchMode: "
    + str(os.getenv("KhiopsBatchMode"))
    + "\n\t true, false (default: false)"
)

print(
    "KhiopsMinTestTime: "
    + str(os.getenv("KhiopsMinTestTime"))
    + "\n\t run only tests where run time (in file time.log) is beyond a threshold"
)

print(
    "KhiopsMaxTestTime: "
    + str(os.getenv("KhiopsMaxTestTime"))
    + "\n\t run only tests where run time (in file time.log) is below a threshold"
)

print(
    "KhiopsMPIProcessNumber: "
    + str(os.getenv("KhiopsMPIProcessNumber"))
    + "\n\t Number of MPI process in paralle mode (default: none)"
)

print(
    "KhiopsExpertMode: "
    + str(os.getenv("KhiopsExpertMode"))
    + "\n\t Khiops expert mode true, false (default: false)"
)

print(
    "KhiopsTaskFileMode: "
    + str(os.getenv("KhiopsTaskFileMode"))
    + "\n\t Create a task file task.log (-t option) (default: none)"
)

print(
    "KhiopsOutputScenarioMode: "
    + str(os.getenv("KhiopsOutputScenarioMode"))
    + "\n\t Create an output scenario test.output.prm (-o option) (default: none)"
)

print(
    "KhiopsCompleteTests: "
    + str(os.getenv("KhiopsCompleteTests"))
    + "\n\t Perform all tests, even the longest ones (default: false)"
)


print("")
print(
    "KhiopsPreparationTraceMode: "
    + str(os.getenv("KhiopsPreparationTraceMode"))
    + "\n\t Trace for dimensionnining of preparation tasks (default: false)"
)
print(
    "KhiopsParallelTrace: "
    + str(os.getenv("KhiopsParallelTrace"))
    + "\n\t Trace for parallel tasks (0 to 3)"
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
