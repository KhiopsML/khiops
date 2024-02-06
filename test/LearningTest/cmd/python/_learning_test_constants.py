"""
Constantes permettant la gestion de la structure des repertoires de LearningTest
et l'analyse des resultats par repertoire de test
"""

""" Repertoire racine de l'arborescence de test """
LEARNING_TEST = "LearningTest"

""" Repertoires des resultats de test et de reference """
RESULTS = "results"
RESULTS_REF = "results.ref"

""" Fichiers se trouvant d'un repertoire de test """
TEST_PRM = "test.prm"
COMPARISON_RESULTS_LOG = "comparisonResults.log"

""" Fichiers se trouvant d'un repertoire de resultats """
ERR_TXT = "err.txt"
TIME_LOG = "time.log"

""" Fichiers speciaux, par priorite decroissante """
PROCESS_TIMEOUT_ERROR_LOG = "process_timeout_error.log"
RETURN_CODE_ERROR_LOG = "return_code_error.log"
STDOUT_ERROR_LOG = "stdout_error.log"
STDERR_ERROR_LOG = "stderr_error.log"
SPECIAL_ERROR_FILES = [
    PROCESS_TIMEOUT_ERROR_LOG,
    RETURN_CODE_ERROR_LOG,
    STDOUT_ERROR_LOG,
    STDERR_ERROR_LOG,
]

"""
Liste des outils de Khiops Core
A chaque nom d'outil Khiops correspond un nom d'exe et un sous-repertoire de LearningTest associe.
On peut egalement specifier si l'outil est lancable en parallel ou non.

Les listes et dictionnaires ci-dessous permettent d'ajouter des outils si besoin.
"""

""" Liste des noms des outils """
KHIOPS = "Khiops"
COCLUSTERING = "Coclustering"
KNI = "KNI"
TOOL_NAMES = [KHIOPS, COCLUSTERING, KNI]

""" Dictionnaire des noms d'executable avec le nom d'outil en cle """
TOOL_EXE_NAMES = {
    KHIOPS: "MODL",
    COCLUSTERING: "MODL_Coclustering",
    KNI: "KNITransfer",
}
assert set(TOOL_EXE_NAMES) == set(TOOL_NAMES), "Exe names must be defined for each tool"

""" Dictionnaire des noms des sous-repertoires de LearningTest avec le nom d'outil en cle """
TOOL_DIR_NAMES = {
    KHIOPS: "TestKhiops",
    COCLUSTERING: "TestCoclustering",
    KNI: "TestKNITransfer",
}
assert set(TOOL_DIR_NAMES) == set(TOOL_NAMES), "Sub-dir must be defined for each tool"

""" Dictionnaire inverse des noms d'outil avec les nom d'exe en cle """
TOOL_NAMES_PER_EXE_NAME = dict((v, k) for k, v in TOOL_EXE_NAMES.items())
assert set(TOOL_NAMES_PER_EXE_NAME.values()) == set(TOOL_NAMES)

""" Dictionnaire inverse des noms d'outil avec les nom des sous-repertoires en cle """
TOOL_NAMES_PER_DIR_NAME = dict((v, k) for k, v in TOOL_DIR_NAMES.items())
assert set(TOOL_NAMES_PER_DIR_NAME.values()) == set(TOOL_NAMES)

""" Liste des outils de Khiops qui tournent en parallele (les seuls que l'on peut lancer avec mpiexec) """
PARALLEL_TOOL_NAMES = [KHIOPS]
assert set(PARALLEL_TOOL_NAMES) <= set(TOOL_NAMES), (
    "Parallel tools " + str(PARALLEL_TOOL_NAMES) + " must be a subset of Khiops tools"
)

"""
Typologie des resultats de reference
"""

""" Type de resultats de reference """
COMPUTING = "computing"
PLATFORM = "platform"
RESULTS_REF_TYPES = [COMPUTING, PLATFORM]

""" Valeurs par type de resultats de refences """
RESULTS_REF_TYPE_VALUES = {
    COMPUTING: ["parallel", "sequential"],
    PLATFORM: ["Darwin", "Linux", "Windows", "WSL"],
}
assert set(RESULTS_REF_TYPE_VALUES) == set(
    RESULTS_REF_TYPES
), "Values must be defined for each type or reference results"

# Caracteres separateurs utilises dans l'analyse des type de repertoire de reference
AND = "-"
OR = "_"


"""
Variables d'environnement de pilotage des tests
"""
KHIOPS_MPI_PROCESS_NUMBER = "KhiopsMPIProcessNumber"
KHIOPS_MIN_TEST_TIME = "KhiopsMinTestTime"
KHIOPS_MAX_TEST_TIME = "KhiopsMaxTestTime"
KHIOPS_TEST_TIMEOUT_LIMIT = "KhiopsTestTimeoutLimit"
KHIOPS_BATCH_MODE = "KhiopsBatchMode"
KHIOPS_TASK_FILE_MODE = "KhiopsTaskFileMode"
KHIOPS_OUTPOUT_SCENARIO_MODE = "KhiopsOutputScenarioMode"
KHIOPS_COMPARISON_PLATFORM = "KhiopsComparisonPlatform"
KHIOPS_COMPLETE_TESTS = "KhiopsCompleteTests"

"""
Variables d'environnement influant le comportement des outils Khiops
"""
# Variables documentees pour l'utilisateur
KHIOPS_PREPARATION_TRACE_MODE = "KhiopsPreparationTraceMode"
KHIOPS_PARALLEL_TRACE = "KhiopsParallelTrace"
KHIOPS_MEM_STATS_LOG_FILE_NAME = "KhiopsMemStatsLogFileName"
KHIOPS_MEM_STATS_LOG_FREQUENCY = "KhiopsMemStatsLogFrequency"
KHIOPS_MEM_STATS_LOG_TO_COLLECT = "KhiopsMemStatsLogToCollect"
KHIOPS_IO_TRACE_MODE = "KhiopsIOTraceMode"

# Variables non documentee documentees, utilisee systematiquyement pour les tests
KHIOPS_EXPERT_MODE = "KhiopsExpertMode"
KHIOPS_CRASH_TEST_MODE = "KhiopsCrashTestMode"
KHIOPS_HARD_MEMORY_LIMIT_MODE = "KhiopsHardMemoryLimitMode"

"""
Gestion du timeout pour un jeu de test
"""
# Temps minimum avant un timeout
MIN_TIMEOUT = 300

# Ratio de temps par rapport au temps des resultats de reference avant le timeout
TIMEOUT_RATIO = 5

# Temps au dela duquel on ne tente pas de relancer un test
MAX_TIMEOUT = 3600

# Nombre maximum de lancement de test dans le cas d'un depassement du timeout
MAX_RUN_NUMBER = 3
