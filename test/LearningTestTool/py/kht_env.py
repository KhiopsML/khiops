import os

import _kht_constants as kht
import _kht_utils as utils


def print_env_var_help(env_var, help_text):
    """Affichage de l'aide sur une variable d'environnement"""
    print(env_var + ": " + str(os.getenv(env_var)) + "\n\t" + help_text)


def get_env_var_help_label(env_var, help_text):
    """Renvoie un libelle d'aide sur une variable d'environnement"""
    return env_var + ": " + str(os.getenv(env_var)) + ", " + help_text


# Aide sur les variables d'environnement de pilotage des tests
print_env_var_help(
    kht.KHIOPS_MPI_PROCESS_NUMBER,
    "number of MPI process in paralle mode (default: None)",
)
print_env_var_help(
    kht.KHIOPS_MIN_TEST_TIME,
    "run only tests where run time (in file "
    + kht.TIME_LOG
    + ") is beyond a threshold",
)
print_env_var_help(
    kht.KHIOPS_MAX_TEST_TIME,
    "run only tests where run time (in file " + kht.TIME_LOG + ") is below a threshold",
)
print_env_var_help(
    kht.KHIOPS_TEST_TIMEOUT_LIMIT, "kill overlengthy process (default: automatic)"
)
print_env_var_help(
    kht.KHIOPS_BATCH_MODE,
    "true, false (default: true); "
    "the path to java and the classpath to norm.jar must be defined",
)
print_env_var_help(
    kht.KHIOPS_TASK_FILE_MODE, "create a task file task.log (-t option) (default: None)"
)
print_env_var_help(
    kht.KHIOPS_OUTPOUT_SCENARIO_MODE,
    "create an output scenario test.output.prm (-o option) (default: None)",
)
print_env_var_help(
    kht.KHIOPS_COMPARISON_PLATFORM,
    "platform "
    + utils.list_to_label(kht.RESULTS_REF_TYPE_VALUES[kht.PLATFORM])
    + " used to compare results (default: None, use current platform)",
)
print_env_var_help(
    kht.KHIOPS_COMPLETE_TESTS,
    "perform all tests, even the longest ones (default: false)",
)

# Aide sur les variables d'environnement influant le comportement des outils Khiops
print("")
print_env_var_help(
    kht.KHIOPS_PREPARATION_TRACE_MODE,
    "trace for dimensionnining of preparation tasks (default: false)",
)
print_env_var_help(kht.KHIOPS_PARALLEL_TRACE, "trace for parallel tasks (0 to 3)")

# Aide particulier sur le piloptage des traces memoire
print(
    "Analysis of memory stats"
    + "\n\t"
    + get_env_var_help_label(
        kht.KHIOPS_MEM_STATS_LOG_FILE_NAME, "memory stats log file name"
    )
    + "\n\t"
    + get_env_var_help_label(
        kht.KHIOPS_MEM_STATS_LOG_FREQUENCY,
        "frequency of allocator stats collection (0, 100000, 1000000,...)",
    )
    + "\n\t"
    + get_env_var_help_label(
        kht.KHIOPS_MEM_STATS_LOG_TO_COLLECT,
        "stats to collect (8193: only time and labels, 16383: all,...)",
    )
    + "\n\t"
    + get_env_var_help_label(
        kht.KHIOPS_IO_TRACE_MODE, "to collect IO trace (false, true)"
    )
)
