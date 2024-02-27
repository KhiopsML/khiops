import os.path
import sys
import stat
import subprocess
import time
import learning_test_env
import check_results

# mpiexec sous Windows
if os.name == "nt":
    mpiExecPath = "c:\\Program Files\\Microsoft MPI\\Bin\\mpiexec.exe"
# mpiexec sous Linux
else:
    mpiExecPath = "mpiexec"

"""
A chaque nom d'outil Khiops correspond un nom d'exe et un sous-repertoire de LearningTest associe.
On peut egalement specifier si l'outil est lancable en parallel ou non.

Les listes et dictionnaires ci-dessous permettent d'ajouter des outils si besoin.
"""

""" Liste des noms des outils Khiops """
khiops_tool_names = ["Khiops", "Coclustering", "KNI"]

""" Dictionnaire des noms d'executable avec le nom d'outil en cle """
khiops_exe_names = {
    "Khiops": "MODL",
    "Coclustering": "MODL_Coclustering",
    "KNI": "KNITransfer",
}

""" Dictionnaire des noms des sous-repertoires de LearningTest avec le nom d'outil en cle """
khiops_test_sub_dirs = {
    "Khiops": "TestKhiops",
    "Coclustering": "TestCoclustering",
    "KNI": "TestKNITransfer",
}

""" Liste des outils de Khiops qui tournent en parallele (les seuls que l'on peut lancer avec mpiexec) """
khiops_parallel_tools = ["Khiops"]


def retrieve_tool_info(khiops_tool_name):
    """Retrieve tool info from a Khiops tool name
    return exe name, test sub dir related to tool
    """
    assert khiops_tool_name in khiops_tool_names, print(
        "toolName must in " + str(khiops_tool_names)
    )
    exe_name = khiops_exe_names.get(khiops_tool_name)
    test_sub_dir = khiops_test_sub_dirs.get(khiops_tool_name)
    assert exe_name is not None and test_sub_dir is not None
    return exe_name, test_sub_dir


def build_tool_exe_path(khiops_tool_exe_name, khiops_tool_version):
    """Build tool exe path name from exe name and version"""
    assert khiops_tool_version is not None

    # Version "nul" for results comparison only
    if khiops_tool_version == "nul":
        khiops_tool_exe_path = "nul"
    # Version "d" or "r" for debug or release development version on windows
    elif khiops_tool_version in ["d", "r"]:
        khiops_tool_exe_path = learning_test_env.build_dev_tool_exe_path(
            khiops_tool_exe_name, khiops_tool_version
        )
    # Case where the full path of the exe if given
    elif os.path.isfile(khiops_tool_version):
        khiops_tool_exe_path = khiops_tool_version
    # Case when the exe is in the LearningTest/cmd/mod directly with a name suffixed by "." + khiops_tool_version
    # Actually, the LearningTest/cmd/mod can contains executables, such as e.g. MODL.V10.0.exe, to
    # launch previous version of the tools, using the version in the command (ex: testkhiops V10.0 Standard)
    else:
        khiops_tool_exe_path = os.path.join(
            learning_test_env.learning_test_root,
            "LearningTest",
            "cmd",
            "modl",
            khiops_tool_exe_name + "." + khiops_tool_version,
        )
        if os.name == "nt":
            khiops_tool_exe_path += ".exe"

    # Test if tool exe dir exists
    if khiops_tool_version != "nul":
        if not os.path.isfile(khiops_tool_exe_path):
            print("Khiops tool path : " + khiops_tool_exe_path + " does not exist")
            exit(0)
    return khiops_tool_exe_path


def test(modl_path, samples_path, sample_test):
    # verifie l'existence du repertoire et du fichier de sample_test
    # et lance la comparaison pour le sample 'sample_test'

    # check MODL path
    if modl_path != "nul":
        if not os.path.isfile(modl_path):
            print("MODL path : " + modl_path + " is not correct")
            return 0
    # workingDir=os.getcwd()
    test_dir = os.path.join(samples_path, sample_test)

    # get executable path and set path for exe and dll
    modl_dir = os.path.dirname(modl_path)
    if os.name == "nt":
        initial_path = os.getenv("path")
        os.environ["path"] = modl_dir + ";" + os.getenv("path")
    else:
        initial_path = os.getenv("LD_LIBRARY_PATH", "")
        os.environ["LD_LIBRARY_PATH"] = (
            modl_dir + ":" + os.getenv("LD_LIBRARY_PATH", "")
        )

    # verification de l'integrite du sample_test
    if not os.path.isdir(test_dir):
        print("test " + sample_test + " is not available")
        return 0
    os.chdir(test_dir)
    if not os.path.isfile("test.prm"):
        print("file test.prm not available in " + test_dir)
        return 0

    # creation si necessaire des repertoires de resultats
    if not os.path.isdir("results"):
        os.mkdir(os.path.join(test_dir, "results"))
    if not os.path.isdir("results"):
        print(
            "test directory (" + os.path.join(test_dir, "results") + ") not available"
        )
        return 0
    if not os.path.isdir("results.ref"):
        os.mkdir(os.path.join(test_dir, "results.ref"))
    if not os.path.isdir("results.ref"):
        print(
            "reference directory ("
            + os.path.join(test_dir, "results.ref")
            + ") not available"
        )
        return 0

    # nettoyage du repertoire de resultats
    # if modl_path != 'nul':
    #   result_dir = os.path.join(test_dir,"results")
    #  for file_name in os.listdir(result_dir) :
    #     file_path = os.path.join(result_dir, file_name)
    #    try:
    #       os.chmod(file_path, stat.S_IWRITE)
    #      os.remove(file_path)
    # except:
    #    print("error: unable to remove file " + file_path)

    # Lancement des tests
    if modl_path != "nul":
        # Recherche du nom du module Khiops
        module_name = os.path.basename(modl_path)
        module_name = module_name.lower()
        if "." in module_name:
            fields = module_name.split(".")
            module_name = fields[0]

        # Test si l'on ne lance le module que si son temps de calcul est suffisament petit
        khiops_max_test_time = os.getenv("KhiopsMaxTestTime")
        if khiops_max_test_time is not None:
            try:
                khiops_max_test_time = float(khiops_max_test_time)
            except ValueError:
                khiops_max_test_time = None

        # Test si l'on ne lance le module que si son temps de calcul est suffisament grand
        khiops_min_test_time = os.getenv("KhiopsMinTestTime")
        if khiops_min_test_time is not None:
            try:
                khiops_min_test_time = float(khiops_min_test_time)
            except ValueError:
                khiops_min_test_time = None

        # On ne lance pas les test trop long ou trop court
        if khiops_max_test_time is not None and khiops_min_test_time is not None:
            # Recherche du temps de test de reference
            test_time = None
            time_file_name = os.path.join(
                os.getcwd(), os.path.join(test_dir, "results.ref", "time.log")
            )
            print(time_file_name)
            if os.path.isfile(time_file_name):
                file_time = open(time_file_name, "r")
                lines = file_time.readlines()
                file_time.close()
                if len(lines) > 0:
                    line = lines[0]
                    line = line[:-1]
                    fields = line.split(":")
                    if len(fields) == 2:
                        time_field = fields[1]
                        try:
                            test_time = float(time_field)
                        except ValueError:
                            test_time = None
            # Arret si test trop long ou trop court
            if test_time is None or (
                test_time > khiops_max_test_time or test_time < khiops_min_test_time
            ):
                print(
                    sample_test.upper()
                    + " test not launched (test time: "
                    + str(test_time)
                    + ")"
                )
                return

        # Nettoyage du repertoire de resultats uniquement si le test est lance
        result_dir = os.path.join(test_dir, "results")
        for file_name in os.listdir(result_dir):
            file_path = os.path.join(result_dir, file_name)
            try:
                os.chmod(file_path, stat.S_IWRITE)
                os.remove(file_path)
            except Exception as e:
                print("error: unable to remove file " + file_path + " : " + str(e))

        # Recherche des exe correspondant à des outils pouvant tourner en parallel
        khiops_parallel_modules = []
        for name in khiops_tool_names:
            if name in khiops_parallel_tools:
                exe_name = khiops_exe_names[name]
                khiops_parallel_modules.append(exe_name.lower())

        # Test si le module peut se lancer en parallel
        khiops_mpi_process_number = None
        if module_name in khiops_parallel_modules:
            khiops_mpi_process_number = os.getenv("KhiopsMPIProcessNumber")
        print(
            "starting Test "
            + module_name
            + " "
            + os.path.basename(samples_path).upper()
            + " "
            + sample_test.upper()
            + " test (MPI: "
            + str(khiops_mpi_process_number)
            + ")"
        )

        # khiops en mode expert via une variable d'environnement
        os.putenv("KhiopsExpertMode", "true")
        # os.putenv('KhiopsForestExpertMode', 'true')

        # khiops en mode HardMemoryLimit via une variable d'environnement pour provoquer
        # un plantagephysique de l'allocateur en cas de depassement des contraintes memoires des scenarios
        os.putenv("KhiopsHardMemoryLimitMode", "true")

        # khiops en mode crash test via une variable d'environnement
        os.putenv("KhiopsCrashTestMode", "true")

        # Construction des parametres
        khiops_params = []
        if khiops_mpi_process_number is not None:
            khiops_params.append(mpiExecPath)
            if os.name == "nt":
                khiops_params.append("-l")
            khiops_params.append("-n")
            khiops_params.append(khiops_mpi_process_number)
        khiops_params.append(modl_path)
        if os.getenv("KhiopsBatchMode") != "false":
            khiops_params.append("-b")
        khiops_params.append("-i")
        khiops_params.append(os.path.join(os.getcwd(), "test.prm"))
        khiops_params.append("-e")
        khiops_params.append(
            os.path.join(os.getcwd(), os.path.join(test_dir, "results", "err.txt"))
        )
        if os.getenv("KhiopsOutputScenarioMode") == "true":
            khiops_params.append("-o")
            khiops_params.append(os.path.join(os.getcwd(), "test.output.prm"))
        if os.getenv("KhiopsTaskFileMode") == "true":
            khiops_params.append("-p")
            khiops_params.append(os.path.join(os.getcwd(), "task.log"))

        # Lancement de khiops
        time_start = time.time()
        try:
            subprocess.run(khiops_params)
        except Exception as error:
            print("Execution failed:" + str(error))
        time_stop = time.time()
        print(sample_test.upper() + " test done")

        # Memorisation d'un fichier contenant le temp global
        file_time = open(
            os.path.join(os.getcwd(), os.path.join(test_dir, "results", "time.log")),
            "w",
        )
        file_time.write("Overal time: " + str(time_stop - time_start) + "\n")
        file_time.close()

    # Restore initial path
    if os.name == "nt":
        os.environ["path"] = initial_path
    else:
        os.environ["LD_LIBRARY_PATH"] = initial_path

    # Comparaison des resultats
    os.chdir(samples_path)
    check_results.check_results(sample_test)


# encapsulation de l'appel a la methode test
def run_test(modl_path, khiops_tool_samples_path, khiops_sub_test_name=None):
    # Collect sub-directories of samples_path
    test_list = []
    for file_name in os.listdir(khiops_tool_samples_path):
        if os.path.isdir(os.path.join(khiops_tool_samples_path, file_name)):
            test_list.append(file_name)

    # Error if no sub-directory
    if len(test_list) == 0:
        print("no test is available in " + khiops_tool_samples_path)
        exit(0)

    # Case of a specific sub-directory
    if khiops_sub_test_name is not None:
        test(modl_path, khiops_tool_samples_path, khiops_sub_test_name)
    # Case of all sub-directories
    else:
        for name in test_list:
            test(modl_path, khiops_tool_samples_path, name)


# Pour ouvir un fichier avec un flush systematique
class Unbuffered(object):
    def __init__(self, stream):
        self.stream = stream

    def write(self, data):
        # on encode en utf-8 en ignorant les erreurs pour eviter un erreur lors de l'encodage automatique
        self.stream.write(data.encode("utf-8", "ignore").decode("utf-8"))
        self.stream.flush()

    def writelines(self, datas):
        # on encode en utf-8 en ignorant les erreurs pour eviter un erreur lors de l'encodage automatique
        self.stream.writelines(
            [data.encode("utf-8", "ignore").decode("utf-8") for data in datas]
        )
        self.stream.flush()

    def __getattr__(self, attr):
        return getattr(self.stream, attr)


if __name__ == "__main__":
    if len(sys.argv) < 4 or len(sys.argv) > 5:
        # Specific help if only one parameter with the tool name
        if len(sys.argv) == 2:
            tool_name = sys.argv[1]
            print("test" + tool_name + " [version] [testName] ([subTestName])")
            print("  run tests for the " + tool_name + " tool")
        else:
            print("test [toolName] [version] [testName] ([subTestName])")
            print("  run tests of one of the Khiops tools")
            print("\ttool_name: name of the tool, among Khiops, Coclustering, KNI")
        print("\tversion: version of the tool, one of the following options")
        print("\t  <path_name>: full path of the executable")
        print("\t  d: debug version in developpement environnement")
        print("\t  r: release version in developpement environnement")
        print("\t  ver: <toolname>.<ver>.exe in directory LearningTest\\cmd\\modl")
        print("\t  nul: for comparison with the test results only")
        print("\ttestName: name of the tool test directory (Standard, MultiTables...)")
        print(
            "\tsubTestName: optional, name of the tool test sub-directory (Adult,Iris...)"
        )
        exit(0)

    sys.stdout = Unbuffered(sys.stdout)
    # sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
    # sys.stdout.flush()

    # Retrieve tool info
    tool_name = sys.argv[1]
    tool_exe_name, tool_test_sub_dir = retrieve_tool_info(tool_name)

    # Build tool exe path name from version
    version = sys.argv[2]
    tool_exe_path = build_tool_exe_path(tool_exe_name, version)

    # Test is tool test dir exists
    test_name = sys.argv[3]
    assert test_name is not None
    tool_samples_path = os.path.join(
        learning_test_env.learning_test_root,
        "LearningTest",
        tool_test_sub_dir,
        test_name,
    )
    if not os.path.isdir(tool_samples_path):
        print("samples directory " + tool_samples_path + " does not exist")
        exit(0)

    # Test is tool test sub dir exists
    sub_test_name = None
    if len(sys.argv) == 5:
        sub_test_name = sys.argv[4]
        assert sub_test_name is not None
        samples_sub_path = os.path.join(
            learning_test_env.learning_test_root,
            "LearningTest",
            tool_test_sub_dir,
            test_name,
            sub_test_name,
        )
        if not os.path.isdir(samples_sub_path):
            print("samples sub directory " + samples_sub_path + " does not exist")
            exit(0)

    # Start test
    run_test(tool_exe_path, tool_samples_path, sub_test_name)

    print("DONE")
