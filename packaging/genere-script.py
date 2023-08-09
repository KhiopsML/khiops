import sys
import os
from subprocess import Popen, PIPE


# script called from genere-views.cmake
# The argument is the command line to launch genere, for example:
# /bin//genere -outputdir /khiops/src/Learning/KWLearningProblem KWAnalysisSpec Parameters \
#  KWAnalysisSpec.dd KWAnalysisSpec.dd.log
# The command line is executed as it is and the stdout is redirect to the log file (last element of the command line).
# The content of the log is displayed to stdout
# If the command fails, the log file is removed and it exits with an error
def main():
    args = sys.argv[1:]
    log_path = args[-1]
    with open(log_path, "w") as log_file:
        process = Popen(args[:-1], stdout=log_file)
        process.wait()
        log_file.close()
        with open(log_path, "r") as log_file:
            for line in log_file:
                print("genere: " + line.strip())
        if process.returncode != 0:
            os.remove(log_path)
            exit(1)
        else:
            exit(0)


if __name__ == "__main__":
    main()
