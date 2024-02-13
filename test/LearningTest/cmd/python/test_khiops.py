import os.path
import sys
import shutil
import stat
import subprocess
import time
import learning_test_env
import check_results
from test_dir_management import *


# mpiexec sous Windows
if os.name == "nt":
    mpi_exe_name = "mpiexec.exe"
# mpiexec sous Linux
else:
    mpi_exe_name = "mpiexec"

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
            exit(1)
    return khiops_tool_exe_path


def evaluate_tool(tool_exe_path, tool_test_family_path, test_name):
    """Evaluation d'un outil sur un repertoire de test terminal et comparaison des resultats
    Parametres:
    - tool_exe_path: path de l'outil a tester, ou nul si on ne veut faire que la comparaison
    - tool_test_family_path: repertoire racine du repertoire de test
    - test_name: repertoire de test terminal"""

    def get_env_var_positive_value(env_var_name, is_int=False):
        """Retourne la valeur numerique d'une variable d'environnement representant une duree
        Renvoie None si la variable n'est pas definie
        Sort du programme avec une erreur si elle ne correspond pas a une valeur numerique positive
        """
        value = os.getenv(env_var_name)
        if value is not None:
            try:
                if is_int:
                    value = int(value)
                else:
                    value = float(value)
                if value < 0:
                    raise ValueError("should be positive")
            except ValueError as exception:
                value = None
                print(
                    "error : env var "
                    + env_var_name
                    + " ("
                    + str(os.getenv(env_var_name))
                    + ") :",
                    exception,
                )
                exit(1)
        return value

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
    khiops_mpi_process_number = get_env_var_positive_value(
        "KhiopsMPIProcessNumber", is_int=True
    )
    if module_name not in khiops_parallel_modules:
        khiops_mpi_process_number = None

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
        + get_context_platform_type()
        + ")"
    )

    # Lancement des tests
    if tool_exe_path != "nul":
        # Recherche dans les variable d'environnement du paramtrage des temps min et max
        # pour declencher les test selon le temps des resultat de reference, et de la limite de timeout
        khiops_min_test_time = get_env_var_positive_value("KhiopsMinTestTime")
        khiops_max_test_time = get_env_var_positive_value("KhiopsMaxTestTime")
        khiops_test_timeout_limit = get_env_var_positive_value("KhiopsTestTimeoutLimit")

        # Recherche du temps des resultats de reference dans le fichier de temps
        results_ref, _ = get_results_ref_dir(test_dir)
        results_ref_test_time = None
        if results_ref is not None:
            time_file_name = os.path.join(
                os.getcwd(), os.path.join(test_dir, results_ref, TIME_LOG)
            )
            if os.path.isfile(time_file_name):
                file_time = open(time_file_name, "r", errors="ignore")
                lines = file_time.readlines()
                file_time.close()
                if len(lines) > 0:
                    line = lines[0]
                    line = line[:-1]
                    fields = line.split(":")
                    if len(fields) == 2:
                        time_field = fields[1]
                        try:
                            results_ref_test_time = float(time_field)
                        except ValueError:
                            results_ref_test_time = None

        # Arret si test trop long ou trop court
        if results_ref_test_time is not None and (
            (
                (
                    khiops_max_test_time is not None
                    and results_ref_test_time > khiops_max_test_time
                )
                or (
                    khiops_min_test_time is not None
                    and results_ref_test_time < khiops_min_test_time
                )
            )
        ):
            print(
                test_name
                + " test not launched (test time: "
                + str(results_ref_test_time)
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
            khiops_params.append(mpi_exe_name)
            # Option -l, specifique a mpich, valide au moins pour Windows
            #  "Label standard out and standard error (stdout and stderr) with the rank of the process"
            khiops_params.append("-l")
            if platform.system() == "Darwin":
                khiops_params.append("-host")
                khiops_params.append("localhost")
            khiops_params.append("-n")
            khiops_params.append(str(khiops_mpi_process_number))
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

        # Calcul d'un time_out en fonction du temps de reference, uniquement si celui est disponible
        MIN_TIMEOUT = 300
        TIMEOUT_RATIO = 5
        MAX_TIMEOUT = 3600
        timeout = None
        if results_ref_test_time is not None:
            if khiops_test_timeout_limit is None:
                khiops_test_timeout_limit = MIN_TIMEOUT
            timeout = khiops_test_timeout_limit + TIMEOUT_RATIO * results_ref_test_time

        # Lancement de khiops
        MAX_RUN_NUMBER = 3
        timeout_expiration_lines = []
        overall_time_start = time.time()
        for run_number in range(MAX_RUN_NUMBER):
            run_completed = True
            time_start = time.time()
            with subprocess.Popen(
                khiops_params,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
            ) as khiops_process:
                try:
                    stdout, stderr = khiops_process.communicate(timeout=timeout)
                except subprocess.TimeoutExpired:
                    run_completed = False
                    khiops_process.kill()
                    stdout, stderr = khiops_process.communicate()
            time_stop = time.time()
            # Memorisation du probleme en cas d'echec
            if not run_completed:
                killing_time = time_stop - time_start
                timeout_expiration_lines.append(
                    "Trial "
                    + str(run_number + 1)
                    + " : process killed after "
                    + "{:.1f}".format(killing_time)
                    + "s (reference time="
                    + "{:.1f}".format(results_ref_test_time)
                    + "s)"
                )
            # Arret si ok
            if run_completed:
                break
            # Arret si on a depense globalement trop de temps
            overall_time = time_stop - overall_time_start
            if overall_time > MAX_TIMEOUT and run_number < MAX_RUN_NUMBER - 1:
                timeout_expiration_lines.append(
                    "No more trial: overall trial time is "
                    + "{:.1f}".format(overall_time)
                    + "s (limit="
                    + "{:.1f}".format(MAX_TIMEOUT)
                    + "s)"
                )
                break

        # Memorisation des infos sur les run en cas de timeout
        if len(timeout_expiration_lines) > 0:
            with open(
                os.path.join(os.getcwd(), test_dir, RESULTS, PROCESS_TIMEOUT_ERROR_LOG),
                "w",
                errors="ignore",
            ) as timeout_file:
                for line in timeout_expiration_lines:
                    timeout_file.write(line + "\n")

        # En cas d'anomalie, memorisation du contenu des sorties standard
        if stdout != "":
            # Affichage sur la console, utile par exemple en mode debug pour avoir les stats memoire
            print(stdout)

            # Pretraitement des lignes pour supprimer les lignes normales
            # parfois specifiques a certains outils
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
                    # Recherche d'un pattern de message de l'allocateur
                    ok = (
                        "Memory stats (number of pointers, and memory space)" in line
                        or "Alloc: " in line
                        or "Requested: " in line
                    )
                    # Recherche additionnelle de "Process " en tete de ligne
                    # En effet, parfois en parallel, le debut d'un message commencant par "Process " <id>
                    # est emis sur une lige de stdout, et la fin sur une autre ligne
                    if not ok:
                        ok = line.find("Process ") >= 0
                        break
                    if not ok:
                        break
            else:
                ok = len(lines) == 0
            if not ok:
                try:
                    with open(
                        os.path.join(os.getcwd(), test_dir, RESULTS, STDOUT_ERROR_LOG),
                        "w",
                        errors="ignore",
                    ) as stdout_file:
                        stdout_file.write(stdout)
                except Exception as exception:
                    print(
                        "Enable to write file "
                        + STDOUT_ERROR_LOG
                        + " in "
                        + RESULTS
                        + " dir",
                        exception,
                    )
        if stderr != "":
            print(stderr, file=sys.stderr)
            try:
                with open(
                    os.path.join(os.getcwd(), test_dir, RESULTS, STDERR_ERROR_LOG),
                    "w",
                    errors="ignore",
                ) as stderr_file:
                    stderr_file.write(stderr)
            except Exception as exception:
                print(
                    "Enable to write file "
                    + STDERR_ERROR_LOG
                    + " in "
                    + RESULTS
                    + " dir",
                    exception,
                )
        if khiops_process.returncode != 0 and khiops_process.returncode != 2:
            try:
                with open(
                    os.path.join(os.getcwd(), test_dir, RESULTS, RETURN_CODE_ERROR_LOG),
                    "w",
                    errors="ignore",
                ) as return_code_file:
                    return_code_file.write(
                        "Wrong return code: "
                        + str(khiops_process.returncode)
                        + " (should be 0 or 2)"
                    )
            except Exception as exception:
                print(
                    "Enable to write file "
                    + RETURN_CODE_ERROR_LOG
                    + " in "
                    + RESULTS
                    + " dir",
                    exception,
                )
        # Message de fin de test
        print(
            tool_test_sub_dir + " " + family_dir_name + " " + test_name + " test done"
        )

        # Memorisation d'un fichier contenant le temp global
        try:
            with open(
                os.path.join(os.getcwd(), os.path.join(test_dir, RESULTS, TIME_LOG)),
                "w",
                errors="ignore",
            ) as time_file:
                time_file.write("Overal time: " + str(time_stop - time_start) + "\n")
        except Exception as exception:
            print(
                "Enable to write file " + TIME_LOG + " in " + RESULTS + " dir",
                exception,
            )

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

    # Echec si le nombre de processus est parametre et mpiexec n'est pas dans le path
    if shutil.which(mpi_exe_name) is None and "KhiopsMPIProcessNumber" in os.environ:
        print("error : KhiopsMPIProcessNumber set but mpiexec not found in path.")
        exit(1)

    # Collect sub-directories of samples_path
    test_list = []
    for file_name in os.listdir(tool_test_family_path):
        if os.path.isdir(os.path.join(tool_test_family_path, file_name)):
            test_list.append(file_name)

    # Error if no sub-directory
    if len(test_list) == 0:
        print("error : no test is available in " + tool_test_family_path)
        exit(1)

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
        exit(1)

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
        print(
            "error : test family directory " + tool_test_family_path + " does not exist"
        )
        exit(1)

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
            print(
                "error : samples sub directory " + samples_sub_path + " does not exist"
            )
            exit(1)

    # Start evaluation
    evaluate_tool_on_family(tool_exe_path, tool_test_family_path, test_name)
