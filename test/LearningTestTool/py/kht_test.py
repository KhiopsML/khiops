import os.path
import sys
import platform
import shutil
import subprocess
import time
import argparse

import _kht_constants as kht
import _kht_utils as utils
import _kht_families as test_families
import _kht_results_management as results
import _kht_check_results as check

# mpiexec sous Windows
if os.name == "nt":
    mpi_exe_name = "mpiexec.exe"
# mpiexec sous Linux
else:
    mpi_exe_name = "mpirun"


def build_tool_exe_path(tool_binaries_dir, tool_name):
    """Construction du chemin de l'executable d'un outil a partir du repertoire des binaire
    Le premier parametre peut contenir plusieurs types de valeurs
    - un repertoire devant contenir les binaire de l'outil a tester
    - 'r' ou 'd', alias pour le repertoire des binaires en release ou debug de l'envbironnement de developpement
    - 'check': pour effectuer seulment une comparaison entre resultats de test et de reference
    On renvoie:
    - le path complet d'un binaire d'un outil si un repertoire est specifie, 'check' sinon, None si erreur
    - le message d'erreur en cas d'error
    """
    assert tool_name in kht.TOOL_NAMES
    tool_exe_path = None
    error_message = ""
    # Cas particulier de la comparaison seulement
    if tool_binaries_dir == "check":
        return "check", error_message

    # Recherche du repertoire des binaires de l'environnement de developpement
    alias_info = ""
    current_platform = results.get_context_platform_type()
    assert current_platform in kht.RESULTS_REF_TYPE_VALUES[kht.PLATFORM]
    # Cas d'un alias pour rechercher le repertoire des binaires dans l'environnement de developpement
    actual_tool_binaries_dir = ""
    if tool_binaries_dir in [kht.ALIAS_D, kht.ALIAS_R]:
        script_path = __file__
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
        searched_suffix = ""
        if tool_binaries_dir == kht.ALIAS_D:
            searched_suffix = "-debug"
        elif tool_binaries_dir == kht.ALIAS_R:
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
            error_message = (
                "Tool binaries dir for alias '"
                + tool_binaries_dir
                + "' not found in current khiops repo under the bin dir "
                + build_dir
            )
        # Erreur si plusieurs repertoires des binaires non trouves
        elif len(candidate_binaries_dirs) > 1:
            error_message = (
                "Multiple tool binaries dir found for alias '"
                + tool_binaries_dir
                + "' in current khiops repo under the bin dir :"
                + utils.list_to_label(candidate_binaries_dirs)
            )
        # On a trouve un repertoire des binaires
        else:
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
    assert actual_tool_binaries_dir != "" or error_message != ""

    # Test qu'il s'agit bien d'un repertoire
    if error_message == "" and not os.path.isdir(actual_tool_binaries_dir):
        error_message = (
            tool_name
            + " binary "
            + actual_tool_binaries_dir
            + " dir"
            + alias_info
            + " is not a valid directory"
        )

    # Construction du path du binaire de l'outil
    if error_message == "":
        tool_exe_name = kht.TOOL_EXE_NAMES[tool_name]
        if current_platform == "Windows":
            tool_exe_name += ".exe"
        tool_exe_path = os.path.join(actual_tool_binaries_dir, tool_exe_name)
        if not os.path.isfile(tool_exe_path):
            tool_exe_path = None
            # si le binaire n'existe pas, c'est peut-etre un binaire parallele qui a un suffixe
            if tool_name in kht.PARALLEL_TOOL_NAMES:
                tool_with_suffixes = []
                tested_binaries_name = []
                # construction de la liste des binaires avec suffixe qui sont presents dans le repertoire bin
                for suffix in kht.TOOL_MPI_SUFFIXES:
                    tool_exe_name = kht.TOOL_EXE_NAMES[tool_name] + suffix
                    if platform == "Windows":
                        tool_exe_name += ".exe"
                    tested_binaries_name.append(tool_exe_name)
                    tool_exe_path = os.path.join(
                        actual_tool_binaries_dir, tool_exe_name
                    )
                    if os.path.isfile(tool_exe_path):
                        tool_with_suffixes.append(tool_exe_path)
                # Si il y en a plusieurs ou aucun, il y a une erreur
                if len(tool_with_suffixes) == 0:
                    tool_exe_path = None
                    tool_full_name = ""
                    for name in tested_binaries_name:
                        tool_full_name += name + " "
                    tool_full_name += kht.TOOL_EXE_NAMES[tool_name]
                    error_message = (
                        "no binaries found for "
                        + tool_name
                        + " ("
                        + tool_full_name.rstrip()
                        + ") in "
                        + actual_tool_binaries_dir
                        + alias_info
                    )
                elif len(tool_with_suffixes) > 1:
                    tool_exe_path = None
                    conflict_names = ""
                    for name in tool_with_suffixes:
                        conflict_names += os.path.basename(name) + " "
                    error_message = (
                        "multiple binaries found for "
                        + tool_name
                        + " ("
                        + conflict_names.rstrip()
                        + ") in "
                        + actual_tool_binaries_dir
                        + alias_info
                    )
                else:
                    tool_exe_path = tool_with_suffixes[0]
            # Message d'erreur par defaut
            if tool_exe_path == None and error_message == "":
                error_message = (
                    tool_name
                    + " binary ("
                    + tool_exe_name
                    + ") not found in tool binaries dir "
                    + actual_tool_binaries_dir
                    + alias_info
                )
    return tool_exe_path, error_message


def evaluate_tool_on_test_dir(
    tool_exe_path,
    suite_dir,
    test_dir_name,
    min_test_time=None,
    max_test_time=None,
    test_timeout_limit=None,
    task_file=False,
    output_scenario=False,
    user_interface=False,
):
    """Evaluation d'un outil sur un repertoire de test terminal et comparaison des resultats
    Parametres:
    - tool_exe_path: path de l'outil a tester, ou nul si on ne veut faire que la comparaison
    - suite_dir: repertoire racine du repertoire de test
    - test_dir_name: repertoire de test terminal"""

    # Verification du chemin de l'exe
    if tool_exe_path != kht.ALIAS_CHECK:
        if not os.path.isfile(tool_exe_path):
            utils.fatal_error("tool path : " + tool_exe_path + " is not correct")

    # Verification de l'integrite du repertoire de test
    test_dir = os.path.join(suite_dir, test_dir_name)
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
    tool_process_number = results.process_number
    if tool_name not in kht.PARALLEL_TOOL_NAMES:
        tool_process_number = 1

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
        + test_dir_name
        + " (processes: "
        + str(tool_process_number)
        + ", platform: "
        + results.get_context_platform_type()
        + ")"
        + exe_path_info
    )

    # Lancement des tests
    if tool_exe_path != kht.ALIAS_CHECK:
        # Recherche du nom du l'executable Khiops (sans l'extension)
        tool_exe_full_name, _ = os.path.splitext(os.path.basename(tool_exe_path))

        # ... et sans le suffixe mpi
        tool_exe_name = utils.extract_tool_exe_name(tool_exe_full_name)

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

        # Recherche du temps des resultats de reference dans le fichier de temps
        results_ref_test_time = results.get_results_ref_dir_time(test_dir)

        # Arret si test trop long ou trop court
        if not results.is_results_ref_dir_time_selected(
            test_dir, min_test_time, max_test_time
        ):
            print(
                test_dir_name
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

        # khiops en mode HardMemoryLimit via une variable d'environnement pour provoquer
        # un plantage physique de l'allocateur en cas de depassement des contraintes memoires des scenarios
        os.putenv(kht.KHIOPS_HARD_MEMORY_LIMIT_MODE, "true")

        # khiops en mode crash test via une variable d'environnement
        os.putenv(kht.KHIOPS_CRASH_TEST_MODE, "true")

        # Construction des parametres
        khiops_params = []
        if tool_process_number > 1:
            khiops_params.append(mpi_exe_name)
            # Option -l, specifique a mpich, valide au moins pour Windows:
            #    "Label standard out and standard error (stdout and stderr) with the rank of the process"
            if platform.system() == "Windows":
                khiops_params.append("-l")
            if platform.system() == "Darwin":
                khiops_params.append("-host")
                khiops_params.append("localhost")
            # Options specifiques a Open MPI
            if platform.system() == "Linux":
                # permet de lancer plus de processus qu'il n'y a de coeurs
                khiops_params.append("--oversubscribe")
                # permet de lancer en tant que root
                khiops_params.append("--allow-run-as-root")
                # Supprime les traces en cas d'erreur fatale de khiops
                khiops_params.append("--quiet")
            khiops_params.append("-n")
            khiops_params.append(str(tool_process_number))
        khiops_params.append(tool_exe_path)
        if not user_interface:
            khiops_params.append("-b")
        khiops_params.append("-i")
        khiops_params.append(os.path.join(os.getcwd(), kht.TEST_PRM))
        khiops_params.append("-e")
        khiops_params.append(
            os.path.join(os.getcwd(), test_dir, kht.RESULTS, kht.ERR_TXT)
        )
        if output_scenario:
            khiops_params.append("-o")
            khiops_params.append(
                os.path.join(os.getcwd(), test_dir, kht.RESULTS, "output_test.prm")
            )
        if task_file:
            khiops_params.append("-p")
            khiops_params.append(
                os.path.join(os.getcwd(), test_dir, kht.RESULTS, "task_progression.log")
            )

        # Calcul d'un time_out en fonction du temps de reference, uniquement si celui est disponible
        timeout = None
        if results_ref_test_time is not None:
            if test_timeout_limit is None:
                test_timeout_limit = kht.MIN_TIMEOUT
            timeout = test_timeout_limit + kht.TIMEOUT_RATIO * results_ref_test_time

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
            )  # Suppression de l'eventuel prefix de type '[0] '
            lines = utils.filter_copyright_lines(
                lines
            )  # Suppression eventuelle des lignes de copyright
            lines = utils.filter_empty_lines(lines)  # Suppression des lignes vides

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
        if khiops_process.returncode != 0:
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
                        + " (should be 0)"
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
        print(tool_dir_name + " " + suite_dir_name + " " + test_dir_name + " test done")

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
    test_dir = os.path.join(suite_dir, test_dir_name)
    check.check_results(test_dir)


def evaluate_tool_on_suite_dir(tool_exe_path, suite_dir, test_dir_name=None, **kwargs):
    """Evaluation d'un outil sur une suite de test et comparaison des resultats
    Parametres:
    - tool_exe_path: path de l'outil a tester, ou nul si on ne veut faire que la comparaison
    - suite_dir: repertoire de la suite de test
    - test_dir_name: repertoire de test terminal"""

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
    if test_dir_name is not None:
        evaluate_tool_on_test_dir(tool_exe_path, suite_dir, test_dir_name, **kwargs)

    # Cas de tous les sous-repertoires
    else:
        for name in test_list:
            evaluate_tool_on_test_dir(tool_exe_path, suite_dir, name, **kwargs)
        # Message global
        suite_dir_name = utils.dir_name(suite_dir)
        tool_dir_name = utils.parent_dir_name(suite_dir, 1)
        action_name = "TEST"
        if tool_exe_path == "nul":
            action_name = "COMPARISON"
        print(action_name + " DONE\t" + tool_dir_name + "\t" + suite_dir_name)


def evaluate_tool(tool_name, tool_exe_path, home_dir, test_suites, **kwargs):
    """Lance les tests d'un outil sur un ensemble de suites de tests"""
    assert tool_name in kht.TOOL_NAMES
    assert utils.check_home_dir(home_dir)
    # Recherche du repertoire lie a l'outil
    tool_dir_name = kht.TOOL_DIR_NAMES[tool_name]
    # Lancement des tests sur les repertoires valides
    for suite_dir_name in test_suites:
        suite_dir = os.path.join(
            home_dir,
            tool_dir_name,
            suite_dir_name,
        )
        if os.path.isdir(suite_dir):
            print("\n\n--------------------------------------------------------")
            print("\tRunning " + tool_name + " " + suite_dir_name + " tests")
            print("--------------------------------------------------------")
            suite_dir = os.path.join(
                home_dir,
                tool_dir_name,
                suite_dir_name,
            )
            evaluate_tool_on_suite_dir(tool_exe_path, suite_dir, **kwargs)


def evaluate_all_tools_on_learning_test_tree(
    home_dir,
    input_tool_dir_name,
    input_suite_dir_name,
    input_test_dir_name,
    binaries_dir,
    family,
    **kwargs
):
    """Lance les tests des outils un ensemble de suites de tests
    Tout ou partie de l'arborescence est prise en compte selon la specification
     des operandes tool_dir_name, suite_dir_name, test_dir_name, qui peuvent etre None sinon.
    - home_dir: repertoire principal de l'aborescence source
    - tool_dir_name, suite_dir_name, test_dir_name: pour ne prendre en compte qu'une sous-partie
      de l'arborescence source si ces oprande ne sont pas None
    - binaries_dir: repertorie des executables des outils
    - family: famille utilise pour choisir la sous-partie des suites a exporter
    - kwargs: argument optionnels de la ligne de commande
    """
    # Tous les outils sont a prendre en compte si on est a la racine
    if input_tool_dir_name is None:
        used_tool_names = kht.TOOL_NAMES
    # Sinon, seul l'outil correspondant au tool dir est a tester
    else:
        tool_name = kht.TOOL_NAMES_PER_DIR_NAME[input_tool_dir_name]
        used_tool_names = [tool_name]

    # Parcours des repertoires des outils verifier les repertoires de suite et nettoyer les resultats
    suite_errors = False
    for tool_name in used_tool_names:
        tool_dir_name = kht.TOOL_DIR_NAMES[tool_name]
        tool_exe_path, error_message = build_tool_exe_path(binaries_dir, tool_name)
        # Recherche des suites a utiliser
        if input_suite_dir_name is not None:
            assert tool_dir_name is not None
            suite_dir_names = [input_suite_dir_name]
        elif family == test_families.ALL:
            suite_dir_names = utils.sub_dirs(os.path.join(home_dir, tool_dir_name))
        else:
            suite_dir_names = test_families.FAMILY_TEST_SUITES[family, tool_name]
        # Parcours des suites de la famille
        for suite_dir_name in suite_dir_names:
            suite_dir = os.path.join(
                home_dir,
                tool_dir_name,
                suite_dir_name,
            )
            if os.path.isdir(suite_dir):
                # Repertoires de test a utiliser
                if input_test_dir_name is not None:
                    test_dir_names = [input_test_dir_name]
                else:
                    test_dir_names = os.listdir(suite_dir)
                # Parcours des repertoires de test de la suite
                for test_dir_name in test_dir_names:
                    # Nettoyage sauf si seulement comparaisons des resultats
                    if tool_exe_path != kht.ALIAS_CHECK:
                        test_dir = os.path.join(suite_dir, test_dir_name)
                        if os.path.isdir(test_dir):
                            # Nettoyage uniquement si test compatible avec les contraIntes de temps
                            if results.is_results_ref_dir_time_selected(
                                test_dir,
                                kwargs["min_test_time"],
                                kwargs["max_test_time"],
                            ):
                                file_path = os.path.join(
                                    test_dir, kht.COMPARISON_RESULTS_LOG
                                )
                                if os.path.isfile(file_path):
                                    utils.remove_file(file_path)
                                results_dir = os.path.join(test_dir, kht.RESULTS)
                                if os.path.isdir(results_dir):
                                    for file_name in os.listdir(results_dir):
                                        file_path = os.path.join(results_dir, file_name)
                                        utils.remove_file(file_path)
                                    utils.remove_dir(results_dir)
            # Message d'erreur si suite inexistante
            else:
                if not suite_errors:
                    print("")
                    suite_errors = True
                print("error : suite directory not found: " + suite_dir)

    # Cas d'un seul outil avec un repertoire de suite au de test specifique
    # Dans ce cas, on ignore la famille
    if input_suite_dir_name is not None:
        assert input_tool_dir_name is not None
        tool_name = kht.TOOL_NAMES_PER_DIR_NAME[input_tool_dir_name]
        tool_exe_path, error_message = build_tool_exe_path(binaries_dir, tool_name)
        if tool_exe_path is None:
            utils.fatal_error(error_message)
        suite_dir = os.path.join(home_dir, input_tool_dir_name, input_suite_dir_name)
        evaluate_tool_on_suite_dir(
            tool_exe_path,
            suite_dir,
            input_test_dir_name,
            **kwargs,
        )
    # Cas d'un ou plusieurs outils, ou il faut utiliser les suites de la famille specifiee
    else:
        # Tous les outils sont a prendre en compte si on est a la racine
        if input_tool_dir_name is None:
            used_tool_names = kht.TOOL_NAMES
        # Sinon, seul l'outil correspondant au tool dir est a tester
        else:
            tool_name = kht.TOOL_NAMES_PER_DIR_NAME[input_tool_dir_name]
            used_tool_names = [tool_name]

        # Parcours des outils a evaluer
        for tool_name in used_tool_names:
            tool_dir_name = kht.TOOL_DIR_NAMES[tool_name]
            if family == test_families.ALL:
                test_suites = utils.sub_dirs(os.path.join(home_dir, tool_dir_name))
            else:
                test_suites = test_families.FAMILY_TEST_SUITES[family, tool_name]
            # On ne teste la validite de l'exe que s'il y au moins une suite dans la famille
            if len(test_suites) > 0:
                # On sort avec un message d'erreur si l'exe n'esiste pas
                # Cela n'est fait a priori sur tous les outils
                # Cela permet de lancer un test complet sur une famille, meme si l'exe de KNI
                # (exploite en dernier) n'est pas disponible
                tool_exe_path, error_message = build_tool_exe_path(
                    binaries_dir, tool_name
                )
                if tool_exe_path is None:
                    utils.fatal_error(error_message)
                evaluate_tool(
                    tool_name,
                    tool_exe_path,
                    home_dir,
                    test_suites,
                    **kwargs,
                )


def main():
    """Fonction principale de lancement d'un test"""

    def build_usage_help(
        help_command,
        help_binary_dir,
        help_tool_dir_name=None,
        help_suite_dir_name=None,
        help_test_dir_name=None,
        help_options=None,
    ):
        """Construction d'une ligne d'aide pour un usage de la commande test"""
        source_dir = os.path.join(".", kht.LEARNING_TEST)
        if help_test_dir_name is not None:
            source_dir = os.path.join(
                source_dir, help_tool_dir_name, help_suite_dir_name, help_test_dir_name
            )
        elif help_suite_dir_name is not None:
            source_dir = os.path.join(
                source_dir, help_tool_dir_name, help_suite_dir_name
            )
        elif help_tool_dir_name is not None:
            source_dir = os.path.join(source_dir, help_tool_dir_name)
        usage_help = help_command + " " + source_dir + " " + help_binary_dir
        if help_options is not None:
            usage_help += " " + help_options
        return usage_help

    # Nom du script
    script_file_name = os.path.basename(__file__)
    script_name = os.path.splitext(script_file_name)[0]

    # Ajout d'exemples d'utilisation
    epilog = ""
    epilog += "Usage examples"
    epilog += "\n  " + build_usage_help(script_name, "r", help_options="-p 4")
    epilog += "\n  " + build_usage_help(
        script_name,
        '"C:\\Program Files\\khiops\\bin"',
        kht.TOOL_DIR_NAMES[kht.KHIOPS],
        "Standard",
        "Iris",
    )
    epilog += "\n  " + build_usage_help(
        script_name,
        "d",
        kht.TOOL_DIR_NAMES[kht.COCLUSTERING],
        help_options="--max-test-time 5 --test-timeout-limit 1000",
    )
    epilog += "\n  " + build_usage_help(script_name, "check", help_options="-f basic")

    # Parametrage de l'analyse de la ligne de commande
    parser = argparse.ArgumentParser(
        prog=script_name,
        description="test a tool on a subset of test dirs",
        epilog=epilog,
        formatter_class=utils.get_formatter_class(script_name),
    )

    # Arguments positionnels
    utils.argument_parser_add_source_argument(parser)
    parser.add_argument(
        "binaries",
        help="tool binaries dir,"
        " or one of the following aliases:\n"
        "  r, d: release or debug binary dir in developpement environnement\n"
        "  check: for comparison of test and reference results only\n",
    )

    # Arguments optionnels standards
    utils.argument_parser_add_family_argument(parser)
    utils.argument_parser_add_processes_argument(parser)
    utils.argument_parser_add_forced_platform_argument(parser)
    utils.argument_parser_add_limit_test_time_arguments(parser)

    # Temps de gestion d'un timeout
    parser.add_argument(
        "--test-timeout-limit",
        help="kill overlengthy process exeeding timeout limit",
        type=float,
        metavar="t",
        action="store",
    )

    # Mode avec fichier de tache
    parser.add_argument(
        "--task-file",
        help="create a task progression file task_progression.log in results dir",
        action="store_true",
    )

    # Mode avec scenario en sortie
    parser.add_argument(
        "--output-scenario",
        help="create an output scenario output_test.prm in results dir",
        action="store_true",
    )

    # Mode interface utilisateur
    parser.add_argument(
        "--user-interface",
        help="run in user interface mode"
        " (path to java and classpath with norm.jar must be defined)",
        action="store_true",
    )

    # Analyse de la ligne de commande
    args = parser.parse_args()

    # Verifications supplementaires des arguments
    # On nomme les arguments concerne de la meme facon que pour le comportement par defaut
    # des controles automatiques du parser
    # Le rappel des noms des arguments est redondant avec la definition des arguments ajoutes,
    # mais ce n'est pas trop lourd a maintenir
    # (il n'y a pas d'api officielle d'introspection de la classe argparse)

    # Verification de l'argument source
    (
        home_dir,
        tool_dir_name,
        suite_dir_name,
        test_dir_name,
    ) = utils.argument_parser_check_source_argument(parser, args.source)

    # Verification des arguments optionnels
    utils.argument_parser_check_processes_argument(parser, args.n)
    utils.argument_parser_check_limit_test_time_arguments(
        parser, args.min_test_time, args.max_test_time
    )
    if args.test_timeout_limit is not None and args.test_timeout_limit < 0:
        parser.error("argument --test-timeout-limit must be positive")

    # Echec si le nombre de processus est parametre et mpiexec n'est pas dans le path
    if args.n > 1 and shutil.which(mpi_exe_name) is None:
        parser.error(
            "argument -p/--processes: process number "
            + str(args.n)
            + " is greater than 1 but mpiexec not found in path."
        )

    # Echec si on est en mode interactif des elements de configuration minimaux sont absents
    if args.user_interface:
        # Pour l'instant, verification uniquement sous Windows
        current_platform = results.get_context_platform_type()
        if current_platform == "Windows":
            # Verification de a presence de Java
            # Ne suffit pas pour verifier que jvm.dll est dans le path, mais c'est deja ca
            path_to_java = shutil.which("java.exe")
            if path_to_java is None:
                parser.error(
                    "argument --user-interface is set but Java  not found in path"
                )
            # Verification de la presence de norm.jar dans le classpath
            classpath = os.getenv("classpath")
            if classpath is None or "norm.jar" not in classpath:
                parser.error(
                    "argument --user-interface is set but 'norm.jar' not found in classpath"
                )

    # Memorisation des variables globales de gestion du contexte des resultats de reference
    results.process_number = args.n
    results.forced_platform = args.forced_platform

    # Lancement de la commande
    evaluate_all_tools_on_learning_test_tree(
        home_dir,
        tool_dir_name,
        suite_dir_name,
        test_dir_name,
        args.binaries,
        args.family,
        min_test_time=args.min_test_time,
        max_test_time=args.max_test_time,
        test_timeout_limit=args.test_timeout_limit,
        task_file=args.task_file,
        output_scenario=args.output_scenario,
        user_interface=args.user_interface,
    )


if __name__ == "__main__":
    utils.set_flushed_outputs()
    main()
