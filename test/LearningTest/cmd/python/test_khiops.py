import os.path
import sys
import stat
import subprocess
import time
import platform
import learning_test_env
import check_results
from test_dir_management import *


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


def evaluate_tool(tool_exe_path, tool_test_family_path, test_name):
    """Evaluation d'un outil sur un repertoire de test terminal et comparaison des resultats
    Parametres:
    - tool_exe_path: path de l'outil a tester, ou nul si on ne veut faire que la comparaison
    - tool_test_family_path: repertoire racine du repertoire de test
    - test_name: repertoire de test terminal"""

    def filter_lines(lines, filtered_pattern):
        """retourne les lignes sans celles contenant le pattern en parametre"""
        output_lines = []
        for line in lines:
            if filtered_pattern not in line:
                output_lines.append(line)
        return output_lines

    def filter_empty_lines(lines):
        """retourne les lignes sans les lignes vides"""
        output_lines = []
        for line in lines:
            line = line.strip()
            if line != "":
                # En parallelle, une ligne vide contient le numero du process entre crochets
                is_process_id = line[0] == "[" and line[-1] == "]"
                if is_process_id:
                    is_process_id = line[1:-1].isdigit()
                if not is_process_id:
                    output_lines.append(line)
        return output_lines

    def filter_copyright_lines(lines):
        """retourne les lignes sans les lignes de copyright, presentes en mode UI"""
        output_lines = lines
        is_copyright = False
        if len(lines) >= 2:
            copyright_line = lines[1].strip()
            is_copyright = (
                copyright_line.find("(c)") >= 0
                and copyright_line.find("Orange - All rights reserved.") >= 0
            )
        if is_copyright:
            output_lines = lines[2:]
        return output_lines

    # check MODL path
    if tool_exe_path != "nul":
        if not os.path.isfile(tool_exe_path):
            print("MODL path : " + tool_exe_path + " is not correct")
            return 0
    test_dir = os.path.join(tool_test_family_path, test_name)

    # get executable path and set path for exe and dll
    tool_dir = os.path.dirname(tool_exe_path)
    if os.name == "nt":
        initial_path = os.getenv("path")
        os.environ["path"] = tool_dir + ";" + os.getenv("path")
    else:
        initial_path = os.getenv("LD_LIBRARY_PATH", "")
        os.environ["LD_LIBRARY_PATH"] = (
            tool_dir + ":" + os.getenv("LD_LIBRARY_PATH", "")
        )

    # verification de l'integrite du repertoire de test
    if not os.path.isdir(test_dir):
        print("error: test " + test_name + " is not available")
        return 0
    os.chdir(test_dir)

    # Extraction des repertoires principaux
    family_dir_name = os.path.basename(tool_test_family_path)
    tool_test_sub_dir = os.path.basename(os.path.dirname(tool_test_family_path))

    # Recherche du nom du module Khiops
    module_name = os.path.basename(tool_exe_path)
    module_name = module_name.lower()
    if "." in module_name:
        fields = module_name.split(".")
        module_name = fields[0]

    # Recherche des exe correspondant a des outils pouvant tourner en parallel
    khiops_parallel_modules = []
    for name in khiops_tool_names:
        if name in khiops_parallel_tools:
            exe_name = khiops_exe_names[name]
            khiops_parallel_modules.append(exe_name.lower())

    # Recherche du contexte parallele
    khiops_mpi_process_number = None
    if module_name in khiops_parallel_modules:
        khiops_mpi_process_number = os.getenv("KhiopsMPIProcessNumber")

    # Affichage du debut des tests ou de la comparaison
    action_name = "Test"
    if tool_exe_path == "nul":
        action_name = "Comparison"
    print(
        "starting "
        + action_name
        + " "
        + module_name
        + " "
        + family_dir_name
        + " "
        + test_name
        + " (MPI: "
        + str(khiops_mpi_process_number)
        + ", platform: "
        + platform.system()
        + ")"
    )

    # Lancement des tests
    if tool_exe_path != "nul":
        # Test si l'on ne lance le module que si son temps de calcul est suffisament petit
        khiops_max_test_time = os.getenv("KhiopsMaxTestTime")
        if khiops_max_test_time is not None:
            try:
                khiops_max_test_time = float(khiops_max_test_time)
            except ValueError:
                khiops_max_test_time = None
                print(
                    "error: khiops_max_test_time ("
                    + os.getenv("KhiopsMaxTestTime")
                    + ") should be numeric"
                )

        # Test si l'on ne lance le module que si son temps de calcul est suffisament grand
        khiops_min_test_time = os.getenv("KhiopsMinTestTime")
        if khiops_min_test_time is not None:
            try:
                khiops_min_test_time = float(khiops_min_test_time)
            except ValueError:
                khiops_min_test_time = None
                print(
                    "error: khiops_min_test_time ("
                    + os.getenv("KhiopsMinTestTime")
                    + ") should be numeric"
                )

        # On ne lance pas les test trop long ou trop court
        if khiops_max_test_time is not None or khiops_min_test_time is not None:
            # Recherche du repertoire de reference
            results_ref, _ = get_results_ref_dir(test_dir, show=True)
            # Recherche du temps de test de reference
            test_time = None
            if results_ref is not None:
                time_file_name = os.path.join(
                    os.getcwd(), os.path.join(test_dir, results_ref, TIME_LOG)
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
                (khiops_max_test_time is not None and test_time > khiops_max_test_time)
                or (
                    khiops_min_test_time is not None
                    and test_time < khiops_min_test_time
                )
            ):
                print(
                    test_name
                    + " test not launched (test time: "
                    + str(test_time)
                    + ")\n"
                )
                return

        # Nettoyage du repertoire de resultats
        result_dir = os.path.join(test_dir, RESULTS)
        if os.path.isdir(result_dir):
            for file_name in os.listdir(result_dir):
                file_path = os.path.join(result_dir, file_name)
                try:
                    os.chmod(file_path, stat.S_IWRITE)
                    os.remove(file_path)
                except Exception as e:
                    print("error: unable to remove file " + file_path + " : " + str(e))

        # khiops en mode expert via une variable d'environnement
        os.putenv("KhiopsExpertMode", "true")
        # os.putenv('KhiopsForestExpertMode', 'true')

        # khiops en mode HardMemoryLimit via une variable d'environnement pour provoquer
        # un plantage physique de l'allocateur en cas de depassement des contraintes memoires des scenarios
        os.putenv("KhiopsHardMemoryLimitMode", "true")

        # khiops en mode crash test via une variable d'environnement
        os.putenv("KhiopsCrashTestMode", "true")

        # Construction des parametres
        khiops_params = []
        if khiops_mpi_process_number is not None:
            khiops_params.append(mpiExecPath)
            if os.name == "nt":
                khiops_params.append("-l")
            if platform.system() == "Darwin":
                khiops_params.append("-host")
                khiops_params.append("localhost")
            khiops_params.append("-n")
            khiops_params.append(khiops_mpi_process_number)
        khiops_params.append(tool_exe_path)
        if os.getenv("KhiopsBatchMode") != "false":
            khiops_params.append("-b")
        khiops_params.append("-i")
        khiops_params.append(os.path.join(os.getcwd(), TEST_PRM))
        khiops_params.append("-e")
        khiops_params.append(os.path.join(os.getcwd(), test_dir, RESULTS, ERR_TXT))
        if os.getenv("KhiopsOutputScenarioMode") == "true":
            khiops_params.append("-o")
            khiops_params.append(os.path.join(os.getcwd(), "test.output.prm"))
        if os.getenv("KhiopsTaskFileMode") == "true":
            khiops_params.append("-p")
            khiops_params.append(os.path.join(os.getcwd(), "task.log"))

        # Lancement de khiops
        time_start = time.time()
        with subprocess.Popen(
            khiops_params,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
        ) as khiops_process:
            stdout, stderr = khiops_process.communicate()

        # En cas d'anomalie, memorisation du contenu de des sorties standard
        if stdout != "":
            is_kni = "KNI" in tool_exe_path
            is_coclustering = "Coclustering" in tool_exe_path
            lines = stdout.split("\n")
            lines = filter_empty_lines(lines)
            lines = filter_copyright_lines(lines)
            # Pour les test KNI, le stdout contient une ligne avec le nombre de records
            if is_kni:
                lines = filter_lines(lines, "Recoded record number:")
                lines = filter_lines(lines, "Error : Finish opening stream error:")
            # Cas particulier du coclustering en mode debug
            if is_coclustering:
                lines = filter_lines(
                    lines, "BEWARE: Optimization level set to 0 in debug mode only!!!"
                )
            # Exception egalement pour cas des lancement en mode parallele simule
            lines = filter_lines(lines, "Warning : simulated parallel mode")
            # Exception en mode debug, pour les stats memoire
            if "Memory stats (number of pointers, and memory space)" in stdout:
                ok = True
                # Parcours des lignes pour voir si ce sont bien des messages de stats, y compris en parallel
                # En parallele, on a l'id du process entre crochets en tete de chaque ligne
                for line in lines:
                    ok = (
                        "Memory stats (number of pointers, and memory space)" in line
                        or "Alloc: " in line
                        or "Requested: " in line
                    )
                    if not ok:
                        break
            else:
                ok = len(lines) == 0
            if not ok:
                with open(
                    os.path.join(os.getcwd(), test_dir, RESULTS, STDOUT_ERROR_LOG),
                    "w",
                ) as stdout_file:
                    stdout_file.write(stdout)
        if stderr != "":
            with open(
                os.path.join(os.getcwd(), test_dir, RESULTS, STDERR_ERROR_LOG), "w"
            ) as stderr_file:
                stderr_file.write(stderr)
        if khiops_process.returncode != 0 and khiops_process.returncode != 2:
            with open(
                os.path.join(os.getcwd(), test_dir, RESULTS, RETURN_CODE_ERROR_LOG),
                "w",
            ) as return_code_file:
                return_code_file.write(
                    "Wrong return code: "
                    + str(khiops_process.returncode)
                    + " (should be 0 or 2)"
                )

        time_stop = time.time()
        print(
            tool_test_sub_dir + " " + family_dir_name + " " + test_name + " test done"
        )

        # Memorisation d'un fichier contenant le temp global
        file_time = open(
            os.path.join(os.getcwd(), os.path.join(test_dir, RESULTS, TIME_LOG)),
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
    os.chdir(tool_test_family_path)
    check_results.check_results(test_name)


def evaluate_tool_on_family(tool_exe_path, tool_test_family_path, test_name=None):
    """Evaluation d'un outil sur une famille de test et comparaison des resultats
    Parametres:
    - tool_exe_path: path de l'outil a tester, ou nul si on ne veut faire que la comparaison
    - tool_test_family_path: repertoire racine du repertoire de test
    - test_name: repertoire de test terminal"""

    # Collect sub-directories of samples_path
    test_list = []
    for file_name in os.listdir(tool_test_family_path):
        if os.path.isdir(os.path.join(tool_test_family_path, file_name)):
            test_list.append(file_name)

    # Error if no sub-directory
    if len(test_list) == 0:
        print("no test is available in " + tool_test_family_path)
        exit(0)

    # Case of a specific sub-directory
    if test_name is not None:
        evaluate_tool(tool_exe_path, tool_test_family_path, test_name)
    # Case of all sub-directories
    else:
        for name in test_list:
            evaluate_tool(tool_exe_path, tool_test_family_path, name)
        # Message global
        family_dir_name = os.path.basename(tool_test_family_path)
        tool_test_sub_dir = os.path.basename(os.path.dirname(tool_test_family_path))
        action_name = "TEST"
        if tool_exe_path == "nul":
            action_name = "COMPARISON"
        print(action_name + " DONE\t" + tool_test_sub_dir + "\t" + family_dir_name)


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
            print("test" + tool_name + " [version] [testFamilyName] ([testName])")
            print("  run tests for the " + tool_name + " tool")
        else:
            print("test [toolName] [version] [testFamilyName] ([testName])")
            print("  run tests of one of the Khiops tools")
            print("\ttool_name: name of the tool, among Khiops, Coclustering, KNI")
        print("\tversion: version of the tool, one of the following options")
        print("\t  <path_name>: full path of the executable")
        print("\t  d: debug version in developpement environnement")
        print("\t  r: release version in developpement environnement")
        print("\t  ver: <toolname>.<ver>.exe in directory LearningTest\\cmd\\modl")
        print("\t  nul: for comparison only with test results")
        print(
            "\ttestFamilyName: name of the tool test family directory (Standard, MultiTables...)"
        )
        print("\ttestName: optional, name of the tool test directory (Adult,Iris...)")
        exit(0)

    sys.stdout = Unbuffered(sys.stdout)

    # Retrieve tool info
    tool_name = sys.argv[1]
    tool_exe_name, tool_test_sub_dir = retrieve_tool_info(tool_name)

    # Build tool exe path name from version
    version = sys.argv[2]
    tool_exe_path = build_tool_exe_path(tool_exe_name, version)

    # Test is tool test dir exists
    test_family_name = sys.argv[3]
    assert test_family_name is not None
    tool_test_family_path = os.path.join(
        learning_test_env.learning_test_root,
        "LearningTest",
        tool_test_sub_dir,
        test_family_name,
    )
    if not os.path.isdir(tool_test_family_path):
        print("test family directory " + tool_test_family_path + " does not exist")
        exit(0)

    # Test is tool test sub dir exists
    test_name = None
    if len(sys.argv) == 5:
        test_name = sys.argv[4]
        assert test_name is not None
        samples_sub_path = os.path.join(
            learning_test_env.learning_test_root,
            "LearningTest",
            tool_test_sub_dir,
            test_family_name,
            test_name,
        )
        if not os.path.isdir(samples_sub_path):
            print("samples sub directory " + samples_sub_path + " does not exist")
            exit(0)

    # Start evaluation
    evaluate_tool_on_family(tool_exe_path, tool_test_family_path, test_name)
