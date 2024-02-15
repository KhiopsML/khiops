import os.path
import sys
import platform
import shutil
import subprocess
import time

import _kht_constants as kht
import _kht_utils as utils
import _kht_check_results as check
import _kht_results_management as results

# mpiexec sous Windows
if os.name == "nt":
    mpi_exe_name = "mpiexec.exe"
# mpiexec sous Linux
else:
    mpi_exe_name = "mpiexec"


def build_tool_exe_path(tool_binaries_dir, tool_name):
    """Construction du chemin de l'executable d'un outil a partir du repertoire des binaire
    Le premier parametre peut contenir plusieurs types de valeurs
    - un repertoire devant contenir les binaire de l'outil a tester
    - 'r' ou 'd', alias pour le repertoire des binaires en release ou debug de l'envbironnement de developpement
    - 'check': pour effectuer seulment une comparaison entre resultats de test et de reference
    On renvoie le path complet d'un binaire d'un outil si un repertoire est specifie, 'check' sinon
    On sort en erreur fatale avec un message d'erreur utilisateur si les parametres sont invalides
    """
    assert tool_name in kht.TOOL_NAMES
    # Cas particulier de la comparaison seulement
    if tool_binaries_dir == "check":
        return "check"

    # Recherche du repertoire des binaires de l'environnement de developpement
    alias_info = ""
    current_platform = results.get_context_platform_type()
    assert current_platform in kht.RESULTS_REF_TYPE_VALUES[kht.PLATFORM]
    # Cas d'un alias pour rechercher le repertoire des binaires dans l'environnement de developpement
    actual_tool_binaries_dir = ""
    if tool_binaries_dir in [kht.ALIAS_D, kht.ALIAS_R]:
        script_path = __file__
        debug_tool_binaries_dir = ""
        release_tool_binaries_dir = ""
        # Repertoire ou sont construit les produits de compilation
        build_dir = os.path.realpath(
            os.path.join(
                script_path,
                "..",
                "..",
                "..",
                "..",
                "build",
            )
        )
        # Suffixe du nom du repertoire contenant les binaires
        if tool_binaries_dir == kht.ALIAS_D:
            alias_label = "debug"
            searched_suffix = "-debug"
        elif tool_binaries_dir == kht.ALIAS_R:
            alias_label = "release"
            searched_suffix = "-release"
        # Recherche des sous repertoire contenant le bon suffixe, plus un sous-repertoire bin
        candidate_binaries_dirs = []
        if os.path.isdir(build_dir):
            for name in os.listdir(build_dir):
                if (
                    searched_suffix in name
                    and name[len(name) - len(searched_suffix) :] == searched_suffix
                ):
                    binaries_dir = os.path.join(build_dir, name, "bin")
                    if os.path.isdir(binaries_dir):
                        candidate_binaries_dirs.append(binaries_dir)
        # Erreur si repertoire des binaires non trouve
        if len(candidate_binaries_dirs) == 0:
            utils.fatal_error(
                "Tool binaries dir for alias '"
                + tool_binaries_dir
                + " not found in current khiops repo under the bin dir "
                + build_dir
            )
        # Erreur si plusieurs repertoires des binaires non trouve
        if len(candidate_binaries_dirs) > 1:
            utils.fatal_error(
                "Multiple tool binaries dir found for alias '"
                + tool_binaries_dir
                + " in current khiops repo under the bin dir :"
                + utils.list_to_label(candidate_binaries_dirs)
            )
        # On a trouve un repertoire des binaires
        assert len(candidate_binaries_dirs) == 1
        actual_tool_binaries_dir = candidate_binaries_dirs[0]

        # Infos sur l'alias, pour les messages d'erreur
        alias_info = (
            " (used for alias '"
            + tool_binaries_dir
            + "' on platform "
            + current_platform
            + ")"
        )
    # Cas d'un repertoire des binaires specifie directement
    else:
        actual_tool_binaries_dir = os.path.realpath(tool_binaries_dir)
    assert actual_tool_binaries_dir != ""

    # Test qu'il s'agit bien d'un repertoire
    if not os.path.isdir(actual_tool_binaries_dir):
        utils.fatal_error(
            tool_name
            + " binary "
            + actual_tool_binaries_dir
            + " dir"
            + alias_info
            + " is not a valid directory"
        )

    # Construction du parth du binaire de l'outil
    tool_exe_name = kht.TOOL_EXE_NAMES[tool_name]
    if current_platform == "Windows":
        tool_exe_name += ".exe"
    tool_exe_path = os.path.join(actual_tool_binaries_dir, tool_exe_name)
    if not os.path.isfile(tool_exe_path):
        utils.fatal_error(
            tool_name
            + " binary ("
            + tool_exe_name
            + ") not found in tool binaries dir "
            + actual_tool_binaries_dir
            + alias_info
        )
    return tool_exe_path


def evaluate_tool_on_test_dir(tool_exe_path, suite_dir, test_name):
    """Evaluation d'un outil sur un repertoire de test terminal et comparaison des resultats
    Parametres:
    - tool_exe_path: path de l'outil a tester, ou nul si on ne veut faire que la comparaison
    - suite_dir: repertoire racine du repertoire de test
    - test_name: repertoire de test terminal"""

    # Verification du chemin de l'exe
    if tool_exe_path != kht.ALIAS_CHECK:
        if not os.path.isfile(tool_exe_path):
            utils.fatal_error("tool path : " + tool_exe_path + " is not correct")

    # Verification de l'integrite du repertoire de test
    test_dir = os.path.join(suite_dir, test_name)
    utils.check_test_dir(test_dir)

    # Extraction des repertoires principaux
    suite_dir_name = utils.dir_name(suite_dir)
    tool_dir_name = utils.parent_dir_name(suite_dir, 1)

    # Nom de l'outil
    tool_name = kht.TOOL_NAMES_PER_DIR_NAME.get(tool_dir_name)

    # Recherche du chemin de l'executable et positionnement du path pour l'exe et la dll
    tool_exe_dir = os.path.dirname(tool_exe_path)
    if os.name == "nt":
        initial_path = os.getenv("path")
        os.environ["path"] = tool_exe_dir + ";" + os.getenv("path")
    else:
        initial_path = os.getenv("LD_LIBRARY_PATH", "")
        os.environ["LD_LIBRARY_PATH"] = (
            tool_exe_dir + ":" + os.getenv("LD_LIBRARY_PATH", "")
        )

    # On se met dans le repertoire de test
    os.chdir(test_dir)

    # Recherche du contexte parallele
    khiops_mpi_process_number = utils.get_env_var_positive_value(
        kht.KHIOPS_MPI_PROCESS_NUMBER, is_int=True
    )
    if tool_name not in kht.PARALLEL_TOOL_NAMES:
        khiops_mpi_process_number = None

    # Affichage du debut des tests ou de la comparaison
    action_name = "Test"
    exe_path_info = "\n  exe: " + tool_exe_path
    if tool_exe_path == kht.ALIAS_CHECK:
        action_name = "Comparison"
        exe_path_info = ""
    print(
        "starting "
        + action_name
        + " "
        + tool_dir_name
        + " "
        + suite_dir_name
        + " "
        + test_name
        + " (MPI: "
        + str(khiops_mpi_process_number)
        + ", platform: "
        + results.get_context_platform_type()
        + ")"
        + exe_path_info
    )

    # Lancement des tests
    if tool_exe_path != kht.ALIAS_CHECK:
        # Recherche du nom du l'executable Khiops (sans l'extension)
        tool_exe_name, _ = os.path.splitext(os.path.basename(tool_exe_path))

        # Recherche du nom de l'outil correspondant
        if tool_exe_name not in kht.TOOL_EXE_NAMES.values():
            utils.fatal_error(
                "tool exe "
                + tool_exe_name
                + " from "
                + tool_exe_path
                + " should be in "
                + utils.list_to_label(kht.TOOL_EXE_NAMES.values())
            )
        tool_name_per_exe_name = kht.TOOL_NAMES_PER_EXE_NAME.get(tool_exe_name)
        if tool_name_per_exe_name != tool_name:
            utils.fatal_error(
                "Tool exe "
                + tool_exe_path
                + " inconsistent with tool directory "
                + tool_dir_name
            )

        # Recherche dans les variables d'environnement du paramtrage des temps min et max
        # pour declencher les test selon le temps des resultat de reference, et de la limite de timeout
        khiops_min_test_time = utils.get_env_var_positive_value(
            kht.KHIOPS_MIN_TEST_TIME
        )
        khiops_max_test_time = utils.get_env_var_positive_value(
            kht.KHIOPS_MAX_TEST_TIME
        )
        khiops_test_timeout_limit = utils.get_env_var_positive_value(
            kht.KHIOPS_TEST_TIMEOUT_LIMIT
        )

        # Recherche du temps des resultats de reference dans le fichier de temps
        results_ref_dir, _ = results.get_results_ref_dir(test_dir)
        results_ref_test_time = None
        if results_ref_dir is not None:
            time_file_path = os.path.join(
                os.getcwd(), os.path.join(test_dir, results_ref_dir, kht.TIME_LOG)
            )
            if os.path.isfile(time_file_path):
                file_time = open(time_file_path, "r", errors="ignore")
                lines = file_time.readlines()
                file_time.close()
                if len(lines) > 0:
                    line = lines[0]
                    line = line[:-1]
                    fields = line.split(
                        " "
                    )  # Pour etre resilient aux formats 'Overal time: <time>' ou '<time>'
                    time_field = fields[-1]
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
        results_dir = os.path.join(test_dir, kht.RESULTS)
        if os.path.isdir(results_dir):
            for file_name in os.listdir(results_dir):
                file_path = os.path.join(results_dir, file_name)
                utils.remove_file(file_path)

        # khiops en mode expert via une variable d'environnement
        os.putenv(kht.KHIOPS_EXPERT_MODE, "true")
        # os.putenv('KhiopsForestExpertMode', 'true')

        # khiops en mode HardMemoryLimit via une variable d'environnement pour provoquer
        # un plantage physique de l'allocateur en cas de depassement des contraintes memoires des scenarios
        os.putenv(kht.KHIOPS_HARD_MEMORY_LIMIT_MODE, "true")

        # khiops en mode crash test via une variable d'environnement
        os.putenv(kht.KHIOPS_CRASH_TEST_MODE, "true")

        # Construction des parametres
        khiops_params = []
        if khiops_mpi_process_number is not None:
            khiops_params.append(mpi_exe_name)
            # Option -l, specifique a mpich, valide au moins pour Windows:
            #    "Label standard out and standard error (stdout and stderr) with the rank of the process"
            khiops_params.append("-l")
            if platform.system() == "Darwin":
                khiops_params.append("-host")
                khiops_params.append("localhost")
            khiops_params.append("-n")
            khiops_params.append(str(khiops_mpi_process_number))
        khiops_params.append(tool_exe_path)
        if utils.get_env_var_boolean_value(kht.KHIOPS_BATCH_MODE, True):
            khiops_params.append("-b")
        khiops_params.append("-i")
        khiops_params.append(os.path.join(os.getcwd(), kht.TEST_PRM))
        khiops_params.append("-e")
        khiops_params.append(
            os.path.join(os.getcwd(), test_dir, kht.RESULTS, kht.ERR_TXT)
        )
        if utils.get_env_var_boolean_value(kht.KHIOPS_OUTPOUT_SCENARIO_MODE, False):
            khiops_params.append("-o")
            khiops_params.append(os.path.join(os.getcwd(), "test.output.prm"))
        if utils.get_env_var_boolean_value(kht.KHIOPS_TASK_FILE_MODE, False):
            khiops_params.append("-p")
            khiops_params.append(os.path.join(os.getcwd(), "task.log"))

        # Calcul d'un time_out en fonction du temps de reference, uniquement si celui est disponible
        timeout = None
        if results_ref_test_time is not None:
            if khiops_test_timeout_limit is None:
                khiops_test_timeout_limit = kht.MIN_TIMEOUT
            timeout = (
                khiops_test_timeout_limit + kht.TIMEOUT_RATIO * results_ref_test_time
            )

        # Lancement de khiops
        timeout_expiration_lines = []
        overall_time_start = time.time()
        for run_number in range(kht.MAX_RUN_NUMBER):
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
                results_ref_test_time_info = ""
                if results_ref_test_time is not None:
                    results_ref_test_time_info = (
                        " (reference time="
                        + "{:.1f}".format(results_ref_test_time)
                        + "s)"
                    )
                timeout_expiration_lines.append(
                    "Trial "
                    + str(run_number + 1)
                    + " : process killed after "
                    + "{:.1f}".format(killing_time)
                    + "s"
                    + results_ref_test_time_info
                )
            # Arret si ok
            if run_completed:
                break
            # Arret si on a depense globalement trop de temps
            overall_time = time_stop - overall_time_start
            if overall_time > kht.MAX_TIMEOUT and run_number < kht.MAX_RUN_NUMBER - 1:
                timeout_expiration_lines.append(
                    "No more trial: overall trial time is "
                    + "{:.1f}".format(overall_time)
                    + "s (limit="
                    + "{:.1f}".format(kht.MAX_TIMEOUT)
                    + "s)"
                )
                break
        overall_time_stop = time.time()

        # Memorisation des infos sur les run en cas de timeout
        if len(timeout_expiration_lines) > 0:
            with open(
                os.path.join(
                    os.getcwd(), test_dir, kht.RESULTS, kht.PROCESS_TIMEOUT_ERROR_LOG
                ),
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
            is_kni = kht.KNI in tool_exe_path
            is_coclustering = kht.COCLUSTERING in tool_exe_path
            lines = stdout.split("\n")
            lines = utils.filter_process_id_prefix_from_lines(
                lines
            )  # Supression de l'eventuel prefix de type '[0] '
            lines = utils.filter_copyright_lines(
                lines
            )  # Supression eventuelle des lignes de copyright
            lines = utils.filter_empty_lines(lines)  # Suopression des lignes vides

            # Pour les test KNI, le stdout contient une ligne avec le nombre de records
            if is_kni:
                lines = utils.filter_lines_with_pattern(
                    lines, ["Recoded record number:"]
                )
                lines = utils.filter_lines_with_pattern(
                    lines, ["Error : Finish opening stream error:"]
                )
            # Cas particulier du coclustering en mode debug
            if is_coclustering:
                lines = utils.filter_lines_with_pattern(
                    lines, ["BEWARE: Optimization level set to 0 in debug mode only!!!"]
                )
            # Exception egalement pour cas des lancement en mode parallele simule
            lines = utils.filter_lines_with_pattern(
                lines, ["Warning : simulated parallel mode"]
            )
            # Exception en mode debug, pour les stats memoire
            if "Memory stats (number of pointers, and memory space)" in stdout:
                ok = True
                # Parcours des lignes pour voir si ce sont bien des messages de stats, y compris en parallel
                # En parallele, on a l'id du process entre crochets en tete de chaque ligne
                for line in lines:
                    # Ok si ligne vide
                    if line == "":
                        ok = True
                    # Recherche d'un pattern de message de l'allocateur
                    else:
                        ok = (
                            "Memory stats (number of pointers, and memory space)"
                            in line
                            or "Alloc: " in line
                            or "Requested: " in line
                        )
                    # Recherche additionnelle de "Process " en tete de ligne
                    # En effet, parfois en parallele, le debut d'un message commencant par "Process " <id>
                    # est emis sur une ligne de stdout, et la fin sur une autre ligne
                    if not ok:
                        ok = line.find("Process ") >= 0
                    if not ok:
                        break
            else:
                ok = len(lines) == 0
            if not ok:
                try:
                    with open(
                        os.path.join(
                            os.getcwd(), test_dir, kht.RESULTS, kht.STDOUT_ERROR_LOG
                        ),
                        "w",
                        errors="ignore",
                    ) as stdout_file:
                        stdout_file.write(stdout)
                except Exception as exception:
                    print(
                        "Enable to write file "
                        + kht.STDOUT_ERROR_LOG
                        + " in "
                        + kht.RESULTS
                        + " dir ",
                        exception,
                    )
        # Cas de la sortie d'erreur standard
        if stderr != "":
            print(stderr, file=sys.stderr)
            try:
                with open(
                    os.path.join(
                        os.getcwd(), test_dir, kht.RESULTS, kht.STDERR_ERROR_LOG
                    ),
                    "w",
                    errors="ignore",
                ) as stderr_file:
                    stderr_file.write(stderr)
            except Exception as exception:
                print(
                    "Enable to write file "
                    + kht.STDERR_ERROR_LOG
                    + " in "
                    + kht.RESULTS
                    + " dir ",
                    exception,
                )
        # Cas du code retour
        if khiops_process.returncode != 0 and khiops_process.returncode != 2:
            try:
                with open(
                    os.path.join(
                        os.getcwd(), test_dir, kht.RESULTS, kht.RETURN_CODE_ERROR_LOG
                    ),
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
                    + kht.RETURN_CODE_ERROR_LOG
                    + " in "
                    + kht.RESULTS
                    + " dir ",
                    exception,
                )
        # Message de fin de test
        print(tool_dir_name + " " + suite_dir_name + " " + test_name + " test done")

        # Memorisation d'un fichier contenant le temp global
        try:
            with open(
                os.path.join(
                    os.getcwd(), os.path.join(test_dir, kht.RESULTS, kht.TIME_LOG)
                ),
                "w",
                errors="ignore",
            ) as time_file:
                time_file.write(str(overall_time_stop - overall_time_start) + "\n")
        except Exception as exception:
            print(
                "Enable to write file " + kht.TIME_LOG + " in " + kht.RESULTS + " dir ",
                exception,
            )

    # Restore initial path
    if os.name == "nt":
        os.environ["path"] = initial_path
    else:
        os.environ["LD_LIBRARY_PATH"] = initial_path

    # Comparaison des resultats
    os.chdir(suite_dir)
    test_dir = os.path.join(suite_dir, test_name)
    check.check_results(test_dir)


def evaluate_tool_on_suite_dir(tool_exe_path, suite_dir, test_name=None):
    """Evaluation d'un outil sur une suite de test et comparaison des resultats
    Parametres:
    - tool_exe_path: path de l'outil a tester, ou nul si on ne veut faire que la comparaison
    - suite_dir: repertoire de la suite de test
    - test_name: repertoire de test terminal"""

    # Echec si le nombre de processus est parametre et mpiexec n'est pas dans le path
    if (
        shutil.which(mpi_exe_name) is None
        and kht.KHIOPS_MPI_PROCESS_NUMBER in os.environ
    ):
        utils.fatal_error(
            "env var '"
            + kht.KHIOPS_MPI_PROCESS_NUMBER
            + "' set but mpiexec not found in path."
        )

    # Echec si on est en mode interactif des elements de configuration minimaux sont absents
    if not utils.get_env_var_boolean_value(kht.KHIOPS_BATCH_MODE, True):
        # Pour l'instant, verification uniquement sous Windows
        current_platform = results.get_context_platform_type()
        if current_platform == "Windows":
            # Verification de a presence de Java
            # Ne suffit pas pour verifier que jvm.dll est dans le path, mais c'est deja ca
            path_to_java = shutil.which("java.exe")
            if path_to_java is None:
                utils.fatal_error(
                    "en var '" + kht.KHIOPS_BATCH_MODE + "' set to 'true', "
                    "but Java not found in path"
                )
            # Verification de la presence de norm.jar dans le casspath
            classpath = os.getenv("classpath")
            if classpath is None or "norm.jar" not in classpath:
                utils.fatal_error(
                    "en var '" + kht.KHIOPS_BATCH_MODE + "' set to 'true', "
                    "but 'norm.jar' not found in classpath"
                )

    # Erreur si repertoire de suite absent
    if not os.path.isdir(suite_dir):
        utils.fatal_error("missing directory for test suite " + suite_dir)

    # Collecte des sous-repertoire de test
    test_list = []
    for file_name in os.listdir(suite_dir):
        if os.path.isdir(os.path.join(suite_dir, file_name)):
            test_list.append(file_name)

    # Erreur si pas de sous-repertoires
    if len(test_list) == 0:
        utils.fatal_error("no test dir is available in " + suite_dir)

    # Cas d'un repertoire de test specifique
    if test_name is not None:
        evaluate_tool_on_test_dir(tool_exe_path, suite_dir, test_name)
    # Cas de tous les sous-repertoires
    else:
        for name in test_list:
            evaluate_tool_on_test_dir(tool_exe_path, suite_dir, name)
        # Message global
        suite_dir_name = utils.dir_name(suite_dir)
        tool_dir_name = utils.parent_dir_name(suite_dir, 1)
        action_name = "TEST"
        if tool_exe_path == "nul":
            action_name = "COMPARISON"
        print(action_name + " DONE\t" + tool_dir_name + "\t" + suite_dir_name)


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


def main():
    """Fonction principale de lancement d'un test"""

    # Aide si mauvais nombre de parametres
    if len(sys.argv) != 3:
        script_name = os.path.basename(__file__)
        base_script_name = os.path.splitext(script_name)[0]
        print(
            base_script_name + " (tool binaries dir) (suite|test dir)\n"
            "  Test tool on a suite or test directory"
        )
        print("  The tool binaries dir must contain the executable of the tested tool,")
        print("   or one of the following aliases:")
        print("    d: debug binary dir in developpement environnement")
        print("    r: release binary dir in developpement environnement")
        print("    check: only compare test and reference results")
        print("  Examples")
        print(
            "   "
            + base_script_name
            + " r "
            + os.path.join(
                "<root dir>",
                kht.LEARNING_TEST,
                kht.TOOL_DIR_NAMES[kht.KHIOPS],
                "Standard",
            )
        )
        print(
            "   "
            + base_script_name
            + ' "C:\\Program Files\\khiops\\bin" '
            + os.path.join(
                "<root dir>",
                kht.LEARNING_TEST,
                kht.TOOL_DIR_NAMES[kht.KHIOPS],
                "Standard",
                "Iris",
            )
        )
        print(
            "   "
            + base_script_name
            + " check "
            + os.path.join(
                "<root dir>",
                kht.LEARNING_TEST,
                kht.TOOL_DIR_NAMES[kht.COCLUSTERING],
                "Standard",
            )
        )
        exit(1)

    # Pour flusher systematuqment sur la sortie standard
    sys.stdout = Unbuffered(sys.stdout)

    # Recherche des parametres
    tool_binaries_dir = sys.argv[1]
    suite_or_test_dir = sys.argv[2]

    # Analyse du repertoire a tester
    learning_test_depth = utils.check_learning_test_dir(suite_or_test_dir)
    tool_dir_name = ""
    suite_dir_name = ""
    test_dir_name = None
    if learning_test_depth == 3:
        tool_dir_name = utils.parent_dir_name(suite_or_test_dir, 2)
        suite_dir_name = utils.parent_dir_name(suite_or_test_dir, 1)
        test_dir_name = utils.parent_dir_name(suite_or_test_dir, 0)
    elif learning_test_depth == 2:
        tool_dir_name = utils.parent_dir_name(suite_or_test_dir, 1)
        suite_dir_name = utils.parent_dir_name(suite_or_test_dir, 0)
    else:
        utils.fatal_error(suite_or_test_dir + " should be a suite or test dir")
    assert tool_dir_name != ""
    if tool_dir_name not in kht.TOOL_DIR_NAMES.values():
        utils.fatal_error(
            tool_dir_name
            + " in "
            + os.path.realpath(suite_or_test_dir)
            + " should be a tool dir "
            + utils.list_to_label(kht.TOOL_DIR_NAMES.values())
        )
    home_dir = utils.get_home_dir(suite_or_test_dir)
    suite_dir = os.path.join(home_dir, tool_dir_name, suite_dir_name)

    # Recherche des infos sur l'outil
    tool_name = kht.TOOL_NAMES_PER_DIR_NAME[tool_dir_name]
    tool_exe_path = build_tool_exe_path(tool_binaries_dir, tool_name)

    # Evaluation
    evaluate_tool_on_suite_dir(tool_exe_path, suite_dir, test_dir_name)


if __name__ == "__main__":
    main()
