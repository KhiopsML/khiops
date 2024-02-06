import os.path
import sys
import stat

import _learning_test_constants as lt
import _learning_test_utils as utils
import _check_results as check
import _results_management as results
import _one_shot_commands as one_shot_commands
import test_khiops

"""
Commandes standard pour les usages courant
Exemples: errors, logs, makeref...
Documentee et maintenues officiellement
"""

# Imports de pykhiops a effectuer au cas par cas dans chaque methode, car ralentissant trop les scripts
# import pykhiops as pk


"""
Fonctions utilitaires generales
"""


def register_command(
    available_commands: dict, command_id: str, command_function, command_label: str
):
    """Enregistrement d'une commande dans un dictionnaire"""
    assert command_id not in available_commands
    assert command_id != ""
    assert command_label != ""
    available_commands[command_id] = (command_function, command_label)


def file_compare(file_name1: str, file_name2: str, skip_patterns: list = None):
    """Comparaison du contenu de deux fichiers
    :param file_name1:
    :param file_name2:
    :param skip_patterns: ne compâre les lignes contenant une des chaines de carcateres en parametres
    :return:
    """
    compare_ok = os.path.isfile(file_name1) and os.path.isfile(file_name2)
    lines1 = []
    lines2 = []
    if compare_ok:
        file1 = open(file_name1, "r", errors="ignore")
        lines1 = file1.readlines()
        file1.close()
        file2 = open(file_name2, "r", errors="ignore")
        lines2 = file2.readlines()
        file1.close()
        compare_ok = len(lines1) == len(lines2)
        if not compare_ok:
            print(
                "\tdifferent line number ("
                + str(len(lines1))
                + " vs "
                + str(len(lines2))
                + ")"
            )
    if compare_ok:
        for i in range(len(lines1)):
            if skip_patterns is not None:
                skip = False
                for pattern in skip_patterns:
                    if pattern in lines1[i]:
                        skip = True
                        break
                if skip:
                    continue
            if lines1[i] != lines2[i]:
                compare_ok = False
                error_line = lines1[i].replace("\t", "  ")
                if len(error_line) > 70:
                    error_line = error_line[:70] + " ..."
                print("\tdifferent line (" + str(i + 1) + "): " + error_line)
                break
    return compare_ok


def analyse_tests_results(test_dir):
    """Analyse des repertoires de test pour fournir des stats sur les erreurs
    Retourne:
    - warning_number,
    - error_number,
    - special_file_message,
    - message_extension,
    - specific_message,
    - portability_message,
    """

    def extract_number(message):
        assert message != ""
        fields = message.split()
        assert fields[0].isdigit()
        number = int(fields[0])
        return number

    # Traitement des erreurs memorisee dans le log
    log_file_path = os.path.join(test_dir, lt.COMPARISON_RESULTS_LOG)
    error_number = 0
    warning_number = 0
    special_file_message = ""
    message_extension = ""
    specific_message = ""
    portability_message = ""
    if os.path.isfile(log_file_path):
        log_file = open(log_file_path, "r", errors="ignore")
        begin_summary = False
        for line in log_file:
            line = line.strip()
            # Recherche du debug de la synthese
            if line == check.SUMMARY_TITLE:
                begin_summary = True

            # Analyse de la synthese
            if begin_summary:
                if line.find(check.SUMMARY_WARNING_KEY) >= 0:
                    warning_number = extract_number(line)
                if line.find(check.SUMMARY_ERROR_KEY) >= 0:
                    error_number = extract_number(line)
                for key in check.SUMMARY_SPECIAL_FILE_KEYS:
                    if line == key:
                        special_file_message = key
                        break
                if line.find(check.SUMMARY_FILE_TYPES_KEY) == 0:
                    message_extension = line
                if line.find(check.SUMMARY_NOTE_KEY) == 0:
                    specific_message = line
                if line.find(check.SUMMARY_PORTABILITY_KEY) == 0:
                    portability_message = line
        if not begin_summary:
            assert error_number == 0
            error_number = 1
            specific_message = (
                "Section '"
                + check.SUMMARY_TITLE
                + "' not found in "
                + lt.COMPARISON_RESULTS_LOG
            )

        # Fermeture du fichier
        log_file.close()
    else:
        error_number = 1
        specific_message = "The test has not been launched"
    return (
        warning_number,
        error_number,
        special_file_message,
        message_extension,
        specific_message,
        portability_message,
    )


"""
Commandes standard
"""


def command_list(test_dir):
    """Liste des repertoires de test avec infos sur les resultats de reference"""
    results_ref_dir, candidate_dirs = results.get_results_ref_dir(test_dir, show=True)
    results_ref_info = ""
    if results_ref_dir is None:
        results_ref_info = (
            ", invalid " + lt.RESULTS_REF + " dirs " + str(candidate_dirs)
        )
    elif results_ref_dir == lt.RESULTS_REF and len(candidate_dirs) == 0:
        results_ref_info = ", missing " + lt.RESULTS_REF + " dir"
    elif len(candidate_dirs) >= 2:
        results_ref_info = (
            ", used "
            + results_ref_dir
            + " dir among "
            + utils.list_to_label(candidate_dirs)
        )
    print(test_dir + results_ref_info)
    utils.check_test_dir(test_dir)


def command_errors(test_dir):
    """Liste des repertoires de test avec des erreurs ou des warnings"""
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)
    (
        warning_number,
        error_number,
        special_file_message,
        message_extension,
        specific_message,
        portability_message,
    ) = analyse_tests_results(test_dir)
    if (
        warning_number != 0
        or error_number != 0
        or special_file_message != ""
        or portability_message != ""
    ):
        message = (
            "\t" + tool_dir_name + "\t" + suite_dir_name + "\t" + test_dir_name + "\t"
        )
        if warning_number > 0:
            message += "warnings\t" + str(warning_number) + "\t"
        else:
            message += "\t\t"
        if error_number > 0:
            message += "errors\t" + str(error_number) + "\t"
        else:
            message += "\t\t"
        if special_file_message != "":
            message += special_file_message
        message += "\t" + message_extension
        message += "\t" + specific_message
        message += "\t" + portability_message
        print(message)


def command_logs(test_dir):
    """Liste des repertoires de test avec des logs pour les cas d'erreurs ou de warnings"""
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)
    (
        warning_number,
        error_number,
        special_file_message,
        message_extension,
        specific_message,
        portability_message,
    ) = analyse_tests_results(test_dir)
    if (
        warning_number != 0
        or error_number != 0
        or special_file_message != ""
        or portability_message != ""
    ):
        log_file_path = os.path.join(test_dir, lt.COMPARISON_RESULTS_LOG)
        if os.path.isfile(log_file_path):
            print("==================================================================")
            print(tool_dir_name + " " + suite_dir_name + " " + test_dir_name)
            print("==================================================================")
            print(log_file_path)
            log_file = open(log_file_path, "r", errors="ignore")
            for s in log_file:
                s = s.replace("\n", "")
                print("  " + s)
            log_file.close()


def command_compare_times(test_dir, verbose=False):
    def print_log_message(message):
        print(
            "\t"
            + tool_dir_name
            + "\t"
            + suite_dir_name
            + "\t"
            + test_dir_name
            + "\t"
            + message
        )

    def clean_time_value(value):
        found_pos = value.find(" (")
        if found_pos >= 0:
            value = value[:found_pos]
        found_pos = value.find(")")
        if found_pos >= 0:
            value = value[:found_pos]
        return value

    def time_to_seconds(value):
        fields = value.split(":")
        computed_seconds = (
            float(fields[0]) * 3600 + float(fields[1]) * 60 + float(fields[2])
        )
        return computed_seconds

    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)

    # Recherche du repertoire des resultats de reference
    results_dir_err_file = os.path.join(test_dir, lt.RESULTS, lt.ERR_TXT)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=verbose)
    results_ref_dir_err_file = os.path.join(test_dir, results_ref_dir, lt.ERR_TXT)
    is_valid = results_ref_dir is not None
    if is_valid:
        if not os.path.isfile(results_dir_err_file):
            is_valid = False
            if verbose:
                print_log_message(
                    "\t\t\t\t\terror : missing "
                    + lt.ERR_TXT
                    + " file in "
                    + lt.RESULTS
                    + " dir"
                )
    if is_valid and not os.path.isfile(results_ref_dir_err_file):
        is_valid = False
        if verbose:
            print_log_message(
                "\t\t\t\t\terror : missing "
                + lt.ERR_TXT
                + " file in "
                + results_ref_dir
                + " dir"
            )
    if is_valid:
        with open(results_dir_err_file, "r", errors="ignore") as fErr:
            lines = fErr.readlines()
        with open(results_ref_dir_err_file, "r", errors="ignore") as f_err_ref:
            lines_ref = f_err_ref.readlines()
        if len(lines) != len(lines_ref):
            print_log_message(
                "\t\t\t\t\terror : "
                + lt.ERR_TXT
                + " files with different number of lines"
            )
        else:
            pattern = "time: "
            for i in range(len(lines)):
                line = lines[i]
                line_ref = lines_ref[i]
                if pattern in line_ref:
                    pos = line_ref.find(pattern)
                    time_label = line_ref[: pos + len(pattern)]
                    time_ref_value = line_ref[len(time_label) : -1]
                    if time_label not in line:
                        print_log_message(
                            str(i + 1)
                            + "\t"
                            + time_label[:-2]
                            + "\t"
                            + "???"
                            + "\t"
                            + clean_time_value(time_ref_value)
                            + "\tERRlt.OR no time found"
                        )
                    else:
                        time_value = line[len(time_label) : -1]
                        seconds = time_to_seconds(clean_time_value(time_value))
                        seconds_ref = time_to_seconds(clean_time_value(time_ref_value))
                        warning = ""
                        if (
                            seconds + seconds_ref > 0.3
                            and abs(seconds - seconds_ref) > 0.1
                            and abs(seconds - seconds_ref)
                            > 0.1 * (seconds + seconds_ref) / 2
                        ):
                            warning = (
                                "\tWARNING\t"
                                + str(
                                    int(
                                        (seconds - seconds_ref)
                                        * 100.0
                                        / (seconds_ref if seconds_ref > 0 else 0.01)
                                    )
                                )
                                + "%"
                            )
                        if verbose or warning != "":
                            label = time_label[: -len(pattern) - 1]
                            pos = label.find("(")
                            if pos > 0:
                                label = label[pos + 1 :]
                            print_log_message(
                                str(i + 1)
                                + "\t"
                                + label
                                + "\t"
                                + clean_time_value(time_value)
                                + "\t"
                                + clean_time_value(time_ref_value)
                                + "\t"
                                + warning
                            )


def command_compare_times_verbose(test_dir):
    command_compare_times(test_dir, verbose=True)


def command_performance(test_dir):
    """Liste des repertoires de test avec les performances du SNB sur les resultats de test"""
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)
    results_dir = os.path.join(test_dir, lt.RESULTS)
    if os.path.isdir(results_dir):
        test_pattern = "TestEvaluationReport.xls"
        for file_name in os.listdir(results_dir):
            if test_pattern in file_name:
                test_eval_file_path = os.path.join(results_dir, file_name)
                test_eval_file = open(test_eval_file_path, "r", errors="ignore")
                for s in test_eval_file:
                    if s.find("Selective Naive Bayes", 0) == 0:
                        s = s.strip()
                        print(
                            "\t"
                            + tool_dir_name
                            + "\t"
                            + suite_dir_name
                            + "\t"
                            + test_dir_name
                            + "\t"
                            + file_name[: -len(test_pattern)]
                            + "\t"
                            + s
                        )
                test_eval_file.close()


def command_performance_ref(test_dir):
    """Liste des repertoires de test avec les performances du SNB sur les resultats de reference"""
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is not None:
        results_dir = os.path.join(test_dir, results_ref_dir)
        if os.path.isdir(results_dir):
            test_pattern = "TestEvaluationReport.xls"
            for file_name in os.listdir(results_dir):
                if test_pattern in file_name:
                    test_eval_file_path = os.path.join(results_dir, file_name)
                    test_eval_file = open(test_eval_file_path, "r", errors="ignore")
                    for s in test_eval_file:
                        if s.find("Selective Naive Bayes", 0) == 0:
                            s = s.strip()
                            print(
                                tool_dir_name
                                + "\t"
                                + suite_dir_name
                                + "\t"
                                + test_dir_name
                                + "\t"
                                + file_name[: -len(test_pattern)]
                                + "\t"
                                + s
                            )
                    test_eval_file.close()


def command_clean(test_dir):
    """Nettoyage des resultats de test et du fichier de comparaison"""
    file_path = os.path.join(test_dir, check.COMPARISON_LOG_FILE_NAME)
    if os.path.isfile(file_path):
        utils.remove_file(file_path)

    # Nettoyage du repertoire des resultats de test
    results_dir = os.path.join(test_dir, lt.RESULTS)
    if os.path.isdir(results_dir):
        for file_name in os.listdir(results_dir):
            file_path = os.path.join(results_dir, file_name)
            utils.remove_file(file_path)
        utils.remove_dir(results_dir)


def command_clean_ref(test_dir):
    """Nettoyage des resultats de reference"""
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is not None and os.path.isdir(results_ref_dir):
        for file_name in os.listdir(results_ref_dir):
            file_path = os.path.join(results_ref_dir, file_name)
            utils.remove_file(file_path)


def command_delete_ref(test_dir):
    """Destruction des resultats de reference pour tous les contexte (plateformes...)"""
    current_ref_dir, all_ref_dirs = results.get_results_ref_dir(test_dir, show=True)
    if current_ref_dir is not None:
        for results_ref_dir in all_ref_dirs:
            for file_name in os.listdir(results_ref_dir):
                file_path = os.path.join(results_ref_dir, file_name)
                utils.remove_file(file_path)
            utils.remove_dir(results_ref_dir)


def command_make_ref(test_dir):
    """Copie des resultats de test vers les resultats de reference"""
    results_dir = os.path.join(test_dir, lt.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is not None:
        if not os.path.isdir(results_ref_dir):
            os.mkdir(results_ref_dir)
        if os.path.isdir(results_ref_dir):
            for file_name in os.listdir(results_ref_dir):
                file_path = os.path.join(results_ref_dir, file_name)
                utils.remove_file(file_path)
        if os.path.isdir(results_dir) and os.path.isdir(results_ref_dir):
            for file_name in os.listdir(results_dir):
                utils.copy_file(
                    os.path.join(results_dir, file_name),
                    os.path.join(results_ref_dir, file_name),
                )


def command_copy_ref(test_dir):
    """Copie des resultats de  vers les resultats de test"""
    results_dir = os.path.join(test_dir, lt.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is not None:
        if not os.path.isdir(results_dir):
            os.mkdir(results_dir)
        if os.path.isdir(results_dir):
            for file_name in os.listdir(results_dir):
                file_path = os.path.join(results_dir, file_name)
                utils.remove_file(file_path)
        if os.path.isdir(results_dir) and os.path.isdir(results_ref_dir):
            for file_name in os.listdir(results_ref_dir):
                utils.copy_file(
                    os.path.join(results_ref_dir, file_name),
                    os.path.join(results_dir, file_name),
                )


def command_check_hdfs(test_dir):
    def parameter_exist(line, searched_keyword):
        # Test s'il y a au moins un parametre dans une ligne et une valeur pour ce parametre
        fields = (
            line[line.find(searched_keyword) + len(searched_keyword) :]
            .strip()
            .split("//")
        )
        return len(fields) > 0 and len(fields[0]) > 0

    # Verification de l'adapation au systeme HDFS
    keywords = [
        "class_file_name",
        "ResultFilesDirectory",
        ".database_name",
        "DataTableName",
        ".EvaluationFileName",
        "ReportFileName",
        "InputCoclusteringFileName",
    ]
    prm_file_path = os.path.join(test_dir, lt.TEST_PRM)
    print(test_dir)
    with open(prm_file_path, "r", errors="ignore") as prm_file:
        line_index = 1
        for s in prm_file:
            # Commentaires dans le scenario
            if "//" in s:
                comment_pos = s.find("//")
                if (
                    comment_pos > 0
                    and s[comment_pos - 1] != " "
                    and s[comment_pos - 1] != "\t"
                ):
                    if s[comment_pos + 2 :].find("//") >= 0:
                        print(
                            str(line_index)
                            + ": \tWARNING: Multiple '//' in line -> "
                            + s[:-1]
                        )
                    else:
                        print(
                            str(line_index)
                            + ": \tComment without blank ' //' -> "
                            + s[:-1]
                        )
            # Test de chaque mot cle
            for keyword in keywords:
                if (
                    s.find(keyword) >= 0
                    and s.find(" ../../../datasets") <= 0
                    and s.find(" ../../../MTdatasets") <= 0
                    and s.find(" ./") <= 0
                ):
                    if parameter_exist(s, keyword):
                        print(str(line_index) + ": \t" + s[:-1])
            line_index += 1


def command_transform_hdfs(test_dir):
    def parameter_exist(line, searched_keyword):
        # Test s'il y a au moins un parametre dans une ligne et une valeur pour ce parametre
        fields = (
            line[line.find(searched_keyword) + len(searched_keyword)]
            .strip()
            .split("//")
        )
        return len(fields) > 0 and len(fields[0]) > 0

    # Creation d'un nouveau fichier test.prm.hdfs compatible avec  hdfs
    keywords = [
        "class_file_name",
        "ResultFilesDirectory",
        ".database_name",
        ".DataTableName",
        "ReportFileName",
        "InputCoclusteringFileName",
    ]
    # PostProcessedCoclusteringFileName CoclusteringDictionaryFileName supprime
    # Le nom du dictionnaire de coclustering ne devrait pas etre un chemin
    prm_file_path = os.path.join(test_dir, lt.TEST_PRM)
    prm_file = open(prm_file_path, "r", errors="ignore")
    prm_file_lines = prm_file.readlines()
    prm_file.close()
    prm_file = open(prm_file_path, "w", errors="ignore")
    for s in prm_file_lines:
        new_line = s
        # Commentaires dans le scenario
        if "//" in s:
            comment_pos = s.find("//")
            if (
                comment_pos > 0
                and s[comment_pos - 1] != " "
                and s[comment_pos - 1] != "\t"
            ):
                if s[comment_pos + 2 :].find("//") >= 0:
                    print(
                        "\tWARNING: Multiple '//' in line (NO TRANSFlt.ORM) -> "
                        + s[:-1]
                    )
                else:
                    new_line = s.replace("//", " //")
        # Test de chaque mot cle
        for keyword in keywords:
            if (
                s.find(keyword) >= 0
                and s.find(" ../../../datasets") <= 0
                and s.find(" ../../../MTdatasets") <= 0
                and s.find(" ./") <= 0
            ):
                if parameter_exist(s, keyword):
                    space_pos = s.find(" ")
                    new_line = s[: space_pos + 1] + "./" + s[space_pos + 1 :]
                    break

        # Cas special pour le mot cle "EvaluationFileName", ne doit pas etre confondu avec TestEvaluationFileName
        if (
            s.find("EvaluationFileName") == 0
            and not s.find(" ./")
            and parameter_exist(s, "EvaluationFileName")
        ):
            space_pos = s.find(" ")
            new_line = s[: space_pos + 1] + "./" + s[space_pos + 1 :]

        prm_file.write(new_line)
    prm_file.close()
    # Transformation du fichier d'erreur dans le repertoire des resultats de reference
    do_it = False
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is not None:
        results_ref_err_file_path = os.path.join(test_dir, results_ref_dir, lt.ERR_TXT)
        if do_it and os.path.isfile(results_ref_err_file_path):
            err_file = open(results_ref_err_file_path, "r", errors="ignore")
            err_file_lines = err_file.readlines()
            err_file.close()
            err_file = open(results_ref_err_file_path, "w", errors="ignore")
            for s in err_file_lines:
                new_line = s
                new_line = new_line.replace(
                    " " + lt.RESULTS + "/", " ./" + lt.RESULTS + "/"
                )
                new_line = new_line.replace(
                    " " + lt.RESULTS + "\\", " ./" + lt.RESULTS + "\\"
                )
                err_file.write(new_line)
            err_file.close()


def command_transform_hdfs_results(test_dir):
    def escape_for_json(token):
        return token.replace("/", "\\/")

    hdfs_test_dir = "hdfs:///user/bguerraz/LearningTest/TestKhiops/"
    hdfs_data_dir = "hdfs:///user/bguerraz/LearningTest/"

    std_data_dir = "../../../"
    datasets = "datasets"
    mt_datasets = "MTdatasets"

    head, sub_test_name = os.path.split(test_dir)
    _, test_name = os.path.split(head)

    hdfs_local_dir = hdfs_test_dir + test_name + "/" + sub_test_name

    results_dir = os.path.join(test_dir, lt.RESULTS)
    if os.path.isdir(results_dir):
        for file_name in os.listdir(results_dir):
            file_path = os.path.join(results_dir, file_name)

            # Lecture du fichier
            with open(file_path, "r", errors="ignore") as file:
                file_data = file.read()

                # Recherche et remplacement
                if ".khj" in file_name:
                    # Jeux de donnees
                    file_data = file_data.replace(
                        escape_for_json(hdfs_data_dir + datasets),
                        escape_for_json(std_data_dir + datasets),
                    )
                    file_data = file_data.replace(
                        escape_for_json(hdfs_data_dir + mt_datasets),
                        escape_for_json(std_data_dir + mt_datasets),
                    )

                    # Repertoire courant ./
                    file_data = file_data.replace(
                        escape_for_json(hdfs_local_dir + "/" + lt.RESULTS),
                        escape_for_json("./" + lt.RESULTS),
                    )  # ou lt.RESULTS sans "./"" ?

                    # Fichiers dans le repertoire courant

                    # file_data = file_data.replace(escape_for_json(
                    #    hdfs_local_dir+"/"), "")
                    file_data = file_data.replace(
                        escape_for_json(hdfs_local_dir + "/"), escape_for_json("./")
                    )

                else:
                    # Jeux de donnees
                    file_data = file_data.replace(
                        hdfs_data_dir + datasets, std_data_dir + datasets
                    )
                    file_data = file_data.replace(
                        hdfs_data_dir + mt_datasets, std_data_dir + mt_datasets
                    )

                    # Repertoire courant ./
                    file_data = file_data.replace(
                        hdfs_local_dir + "/" + lt.RESULTS, "./" + lt.RESULTS
                    )

                    # Fichiers dans le repertoire courant
                    file_data = file_data.replace(hdfs_local_dir + "/", "./")

                # Ecriture du fichier
                os.chmod(file_path, stat.S_IWRITE | stat.S_IREAD)
                with open(file_path, "w", errors="ignore") as output_file:
                    output_file.write(file_data)


"""
Enregistrement des commandes
"""


def register_standard_commands():
    """Enregistrement des commandes standard
    Retourne un dictionnaire des commandes
    """

    # Gestion de l'ensemble des commandes dans un dictionnaire contenant pour chaque identifiant de commande
    # une paire (comannd, libelle)
    available_commands = {}

    # Enregistrement des commandes standard
    register_command(
        available_commands,
        "list",
        command_list,
        "list of sub-directories, with results.ref info",
    )
    register_command(
        available_commands,
        "errors",
        command_errors,
        "report errors and warnings",
    )
    register_command(
        available_commands,
        "logs",
        command_logs,
        "detailed report errors and warnings",
    )
    register_command(
        available_commands,
        "compareTimes",
        command_compare_times,
        "compare time with ref time and report warnings only",
    )
    register_command(
        available_commands,
        "compareTimesVerbose",
        command_compare_times_verbose,
        "compare time with ref time and report all",
    )
    register_command(
        available_commands,
        "performance",
        command_performance,
        "report SNB test accuracy",
    )
    register_command(
        available_commands,
        "performanceRef",
        command_performance_ref,
        "report ref SNB test accuracy",
    )
    register_command(
        available_commands,
        "clean",
        command_clean,
        "delete test result files and comparison log file",
    )
    register_command(
        available_commands,
        "cleanref",
        command_clean_ref,
        "delete reference result files for current reference context",
    )
    register_command(
        available_commands,
        "deleteref",
        command_delete_ref,
        "delete reference result files and directories for all reference context",
    )
    register_command(
        available_commands,
        "makeref",
        command_make_ref,
        "copy test result files to reference dir for current reference context",
    )
    register_command(
        available_commands,
        "copyref",
        command_copy_ref,
        "copy reference result files to test dir for current reference context",
    )
    register_command(
        available_commands,
        "checkHDFS",
        command_check_hdfs,
        "check if parameter files are compliant with HDFS",
    )
    register_command(
        available_commands,
        "transformHDFS",
        command_transform_hdfs,
        "transform parameter files to be compliant with HDFS",
    )
    register_command(
        available_commands,
        "transformHDFSresults",
        command_transform_hdfs_results,
        "transform results files to be compliant with HDFS",
    )
    return available_commands
