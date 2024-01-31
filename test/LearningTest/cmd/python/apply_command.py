import os.path
import sys
import stat
import utils
import test_khiops
import check_results as cr
from test_dir_management import *


# Imports de pykhiops a effectuer au cas par cas dans chaque methode, car ralentissant trop les scripts
# import pykhiops as pk


def file_read_lines(file_name):
    with open(file_name, "r") as input_file:
        lines = input_file.readlines()
    return lines


def file_write_lines(file_path, lines):
    with open(file_path, "w") as the_file:
        for line in lines:
            the_file.write(line)


def file_search(file_name, search_text):
    # search in a file
    the_file = open(file_name, "r")  # Opens the file in read-mode
    text = the_file.read()  # Reads the file and assigns the value to a variable
    the_file.close()  # Closes the file (read session)
    if text.find(search_text) >= 0:
        return True
    else:
        return False


def file_content_search(file_lines, search_text):
    # search in a file
    for line in file_lines:
        if line.find(search_text) >= 0:
            return True
    return False


def file_content_search_count(file_lines, search_text):
    # search in a file
    count = 0
    for line in file_lines:
        if line.find(search_text) >= 0:
            count += 1
    return count


def file_replace(file_name, source_text, replace_text):
    # search/replace in a file
    the_file = open(file_name, "r")  # Opens the file in read-mode
    text = the_file.read()  # Reads the file and assigns the value to a variable
    the_file.close()  # Closes the file (read session)
    # Opens the file again, this time in write-mode
    the_file = open(file_name, "w")
    # replaces all instance_number of our keyword
    the_file.write(text.replace(source_text, replace_text))
    # and writes the whole output when done, wiping over the old contents of the file
    the_file.close()  # Closes the file (write session)


def file_compare(file_name1: str, file_name2: str, skip_patterns: list = None):
    """Compare whether two file have the same content
    :param file_name1:
    :param file_name2:
    :param skip_patterns: does not compare lines containing one of the string in the list of patterns
    :return:
    """
    compare_ok = os.path.isfile(file_name1) and os.path.isfile(file_name2)
    lines1 = []
    lines2 = []
    if compare_ok:
        file1 = open(file_name1, "r")
        lines1 = file1.readlines()
        file1.close()
        file2 = open(file_name2, "r")
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


def file_compare_line_number(file_name1: str, file_name2: str):
    """Compare whether two file have the same number of lines
    :param file_name1:
    :param file_name2:
    :return:
    """
    compare_ok = os.path.isfile(file_name1) and os.path.isfile(file_name2)
    if compare_ok:
        file1 = open(file_name1, "r")
        lines1 = file1.readlines()
        file1.close()
        file2 = open(file_name2, "r")
        lines2 = file2.readlines()
        file1.close()
        compare_ok = len(lines1) == len(lines2)
    return compare_ok


def apply_command_list(work_dir):
    # list directory names, with reference results info
    results_ref_dir, candidate_dirs = get_results_ref_dir(work_dir, show=True)
    results_ref_info = ""
    if results_ref_dir is None:
        results_ref_info = ", invalid " + RESULTS_REF + " dirs " + str(candidate_dirs)
    elif results_ref_dir == RESULTS_REF and len(candidate_dirs) == 0:
        results_ref_info = ", missing " + RESULTS_REF + " dir"
    elif len(candidate_dirs) >= 2:
        results_ref_info = (
            ", used " + results_ref_dir + " dir among " + str(candidate_dirs)
        )
    print(work_dir + results_ref_info)


def analyse_tests_results(work_dir):
    """Analyse test directories for warning, errors or fatal errors
    Returns:
    - warning number
    - erreur number
    - message related to special files (optional)
    - message related to file extensions (optional)
    - specific message (optional)
    - portability message (optional)
    """

    def extract_number(message):
        assert message != ""
        fields = message.split()
        assert fields[0].isdigit()
        number = int(fields[0])
        return number

    # Traitement des erreurs memorisee dans le log
    log_file_name = os.path.join(work_dir, COMPARISON_RESULTS_LOG)
    error_number = 0
    warning_number = 0
    special_file_message = ""
    message_extension = ""
    specific_message = ""
    portability_message = ""
    if os.path.isfile(log_file_name):
        log_file = open(log_file_name, "r", errors="ignore")
        begin_summary = False
        for line in log_file:
            line = line.strip()
            # Recherche du debug de la synthese
            if line == cr.SUMMARY_TITLE:
                begin_summary = True

            # Analyse de la synthese
            if begin_summary:
                if line.find(cr.SUMMARY_WARNING_KEY) >= 0:
                    warning_number = extract_number(line)
                if line.find(cr.SUMMARY_ERROR_KEY) >= 0:
                    error_number = extract_number(line)
                for key in cr.SUMMARY_SPECIAL_FILE_KEYS:
                    if line == key:
                        special_file_message = key
                        break
                if line.find(cr.SUMMARY_FILE_TYPES_KEY) == 0:
                    message_extension = line
                if line.find(cr.SUMMARY_NOTE_KEY) == 0:
                    specific_message = line
                if line.find(cr.SUMMARY_PORTABILITY_KEY) == 0:
                    portability_message = line
        if not begin_summary:
            assert error_number == 0
            error_number = 1
            specific_message = (
                "Section '"
                + cr.SUMMARY_TITLE
                + "' not found in "
                + COMPARISON_RESULTS_LOG
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


def apply_command_errors(work_dir):
    # list test directories with errors or warnings
    # outpout in standard output stream
    test_dir_name = os.path.basename(work_dir)
    family_dir_name = os.path.basename(os.path.dirname(work_dir))
    tool_name = os.path.basename(os.path.dirname(os.path.dirname(work_dir)))
    (
        warning_number,
        error_number,
        special_file_message,
        message_extension,
        specific_message,
        portability_message,
    ) = analyse_tests_results(work_dir)
    if (
        warning_number != 0
        or error_number != 0
        or special_file_message != ""
        or portability_message != ""
    ):
        message = "\t" + tool_name + "\t"
        message += family_dir_name + "\t"
        message += test_dir_name + "\t"
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


def apply_command_logs(work_dir):
    # list test directories with logs
    # outpout in standard output stream
    dir_name = os.path.basename(work_dir)
    root_name = os.path.basename(os.path.dirname(work_dir))
    (
        warning_number,
        error_number,
        special_file_message,
        message_extension,
        specific_message,
        portability_message,
    ) = analyse_tests_results(work_dir)
    if (
        warning_number != 0
        or error_number != 0
        or special_file_message != ""
        or portability_message != ""
    ):
        log_file_name = os.path.join(work_dir, COMPARISON_RESULTS_LOG)
        if os.path.isfile(log_file_name):
            print("==================================================================")
            print(root_name + " " + dir_name)
            print("==================================================================")
            print(log_file_name)
            log_file = open(log_file_name, "r")
            for s in log_file:
                s = s.replace("\n", "")
                print("  " + s)
            log_file.close()


def apply_command_compare_times(work_dir, verbose=False):
    def print_log_message(message):
        print("\t" + tool_name + "\t" + root_name + "\t" + dir_name + "\t" + message)

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

    dir_name = os.path.basename(work_dir)
    root_name = os.path.basename(os.path.dirname(work_dir))
    tool_name = os.path.basename(os.path.dirname(os.path.dirname(work_dir)))
    # Recherche du repertoire des resultats de reference
    results_dir_err_file = os.path.join(work_dir, RESULTS, ERR_TXT)
    results_ref, _ = get_results_ref_dir(work_dir, show=verbose)
    is_valid = results_ref is not None
    if is_valid:
        results_ref_dir_err_file = os.path.join(work_dir, results_ref, ERR_TXT)
        if not os.path.isfile(results_dir_err_file):
            is_valid = False
            if verbose:
                print_log_message(
                    "\t\t\t\t\terror : missing "
                    + ERR_TXT
                    + " file in "
                    + RESULTS
                    + " dir"
                )
    if is_valid and not os.path.isfile(results_ref_dir_err_file):
        is_valid = False
        if verbose:
            print_log_message(
                "\t\t\t\t\terror : missing "
                + ERR_TXT
                + " file in "
                + results_ref
                + " dir"
            )
    if is_valid:
        with open(results_dir_err_file, "r") as fErr:
            lines = fErr.readlines()
        # with open(results_ref_dir_err_file, "r", encoding='utf-8') as fErrRef:
        with open(results_ref_dir_err_file, "r") as f_err_ref:
            lines_ref = f_err_ref.readlines()
        if len(lines) != len(lines_ref):
            print_log_message(
                "\t\t\t\t\terror : " + ERR_TXT + " files with different number of lines"
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
                            + "\tERROR no time found"
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


def apply_command_compare_times_verbose(work_dir):
    apply_command_compare_times(work_dir, verbose=True)


def apply_command_performance(work_dir):
    # list test directories with SNB performance
    dir_name = os.path.basename(work_dir)
    root_name = os.path.basename(os.path.dirname(work_dir))
    results_dir = os.path.join(work_dir, RESULTS)
    if os.path.isdir(results_dir):
        test_pattern = "TestEvaluationReport.xls"
        for file_name in os.listdir(results_dir):
            if test_pattern in file_name:
                test_eval_file_name = os.path.join(results_dir, file_name)
                test_eval_file = open(test_eval_file_name, "r")
                for s in test_eval_file:
                    if s.find("Selective Naive Bayes", 0) == 0:
                        # comma to avoid doubling "\n"
                        print(
                            root_name
                            + "\t"
                            + dir_name
                            + "\t"
                            + file_name[: -len(test_pattern)]
                            + "\t"
                            + s,
                        )
                test_eval_file.close()


def apply_command_performance_ref(work_dir):
    # list test directories with SNB ref performance
    dir_name = os.path.basename(work_dir)
    root_name = os.path.basename(os.path.dirname(work_dir))
    results_ref, _ = get_results_ref_dir(work_dir, show=True)
    if results_ref is not None:
        results_dir = os.path.join(work_dir, results_ref)
        if os.path.isdir(results_dir):
            test_pattern = "TestEvaluationReport.xls"
            for file_name in os.listdir(results_dir):
                if test_pattern in file_name:
                    test_eval_file_name = os.path.join(results_dir, file_name)
                    test_eval_file = open(test_eval_file_name, "r")
                    for s in test_eval_file:
                        if s.find("Selective Naive Bayes", 0) == 0:
                            # comma to avoid doubling "\n"
                            print(
                                root_name
                                + "\t"
                                + dir_name
                                + "\t"
                                + file_name[: -len(test_pattern)]
                                + "\t"
                                + s,
                            )
                    test_eval_file.close()


def apply_command_check_fnb(work_dir):
    from pykhiops import core as pk

    def to_s(value):
        # return str(value)
        return str("{:.4g}".format(value))

    def print_stats(
        result_file_name, report, criterion, new_value, ref_value, maximize
    ):
        fstats.write(tool_name + "\t" + root_name + "\t" + dir_name + "\t")
        fstats.write(result_file_name + "\t" + report + "\t" + criterion + "\t")
        fstats.write(to_s(new_value) + "\t" + to_s(ref_value) + "\t")
        diff = new_value - ref_value
        if maximize:
            fstats.write(to_s(diff))
            alert = diff < 0
        else:
            fstats.write(to_s(-diff))
            alert = diff > 0
        if alert and abs(diff) > 0.01 * (abs(ref_value) + abs(new_value)) / 2:
            fstats.write("\tALERT")
        fstats.write("\n")

    def print_error(message):
        print(tool_name + "\t" + root_name + "\t" + dir_name + "\terror\t" + message)

    test_dir = os.path.join(work_dir, RESULTS)
    ref_dir, _ = get_results_ref_dir(work_dir, show=True)
    if ref_dir is None:
        return
    dir_name = os.path.basename(work_dir)
    root_name = os.path.basename(os.path.dirname(work_dir))
    tool_name = os.path.basename(os.path.dirname(os.path.dirname(work_dir)))

    # Creation d'un fichier de collecte des stats
    fstats = None
    root_dir = os.path.dirname(os.path.dirname(os.path.dirname(work_dir)))
    stats_file_name = os.path.join(root_dir, "stats.FNB.log")
    if os.path.isfile(stats_file_name):
        fstats = open(stats_file_name, "a")
    else:
        fstats = open(stats_file_name, "w")
        fstats.write(
            "Tool\tRoot\tDir\tFile\tReport\tCriterion\tValue\tRef value\tDiff\n"
        )

    if ref_dir is not None and os.path.isdir(ref_dir):
        for file_name in os.listdir(ref_dir):
            ref_file_path = os.path.join(ref_dir, file_name)
            test_file_path = os.path.join(test_dir, file_name)

            #####
            if not os.path.isfile(test_file_path):
                print_error("Missing file " + test_file_path)
                continue

            # Comparaison du fichier d'erreur
            if file_name == ERR_TXT:
                if not file_compare(
                    ref_file_path, test_file_path, skip_patterns=["time"]
                ):
                    print_error(file_name + " are different")
            # Comparaison si preparation
            elif "PreparationReport" in file_name:
                if not file_compare(
                    ref_file_path, test_file_path, skip_patterns=["#Khiops "]
                ):
                    print_error(file_name + " are different")
            elif ".khj" in file_name:
                # Read json result file
                try:
                    ref_report = pk.AnalysisResults()
                    ref_report.read_khiops_json_file(ref_file_path)
                    test_report = pk.AnalysisResults()
                    test_report.read_khiops_json_file(test_file_path)
                except Exception as e:
                    print_error(file_name + "\tparsing alert: " + str(e))
                    continue
                # Analyse modeling results
                if ref_report.modeling_report is not None:
                    if test_report.modeling_report is None:
                        print_error(file_name + "\tmissing modeling report")
                    else:
                        ref_snb_predictor = ref_report.modeling_report.get_predictor(
                            "Selective Naive Bayes"
                        )
                        test_snb_predictor = test_report.modeling_report.get_predictor(
                            "Selective Naive Bayes"
                        )
                        if ref_snb_predictor is not None:
                            if test_snb_predictor is None:
                                print_error(
                                    file_name
                                    + "\tmissing SNB predictor in modeling report"
                                )
                            else:
                                print_stats(
                                    file_name,
                                    test_report.modeling_report.report_type,
                                    "Sel. vars",
                                    test_snb_predictor.variables,
                                    ref_snb_predictor.variables,
                                    False,
                                )
                # Analyse evaluation results
                ref_evaluation_reports = list()
                test_evaluation_reports = list()
                ref_evaluation_reports.append(ref_report.train_evaluation_report)
                ref_evaluation_reports.append(ref_report.test_evaluation_report)
                ref_evaluation_reports.append(ref_report.evaluation_report)
                test_evaluation_reports.append(test_report.train_evaluation_report)
                test_evaluation_reports.append(test_report.test_evaluation_report)
                test_evaluation_reports.append(test_report.evaluation_report)
                for i in range(len(ref_evaluation_reports)):
                    ref_evaluation_report = ref_evaluation_reports[i]
                    test_evaluation_report = test_evaluation_reports[i]
                    if ref_evaluation_report is not None:
                        if test_evaluation_report is None:
                            print_error(
                                file_name
                                + "\tmissing "
                                + ref_evaluation_report.report_type
                                + " "
                                + ref_evaluation_report.evaluation_type
                                + " report"
                            )
                        else:
                            ref_snb_performance = (
                                ref_evaluation_report.get_predictor_performance(
                                    "Selective Naive Bayes"
                                )
                            )
                            test_snb_performance = (
                                test_evaluation_report.get_predictor_performance(
                                    "Selective Naive Bayes"
                                )
                            )
                            if ref_snb_performance is not None:
                                if test_snb_performance is None:
                                    print_error(
                                        file_name
                                        + "\tmissing SNB performance in "
                                        + ref_evaluation_report.report_type
                                        + " "
                                        + ref_evaluation_report.evaluation_type
                                        + " report"
                                    )
                                else:
                                    if test_snb_performance.type == "Classifier":
                                        print_stats(
                                            file_name,
                                            ref_evaluation_report.report_type
                                            + " "
                                            + ref_evaluation_report.evaluation_type,
                                            "accuracy",
                                            test_snb_performance.accuracy,
                                            ref_snb_performance.accuracy,
                                            True,
                                        )
                                        print_stats(
                                            file_name,
                                            ref_evaluation_report.report_type
                                            + " "
                                            + ref_evaluation_report.evaluation_type,
                                            "compression",
                                            test_snb_performance.compression,
                                            ref_snb_performance.compression,
                                            True,
                                        )
                                        print_stats(
                                            file_name,
                                            ref_evaluation_report.report_type
                                            + " "
                                            + ref_evaluation_report.evaluation_type,
                                            "auc",
                                            test_snb_performance.auc,
                                            ref_snb_performance.auc,
                                            True,
                                        )
                                    if test_snb_performance.type == "Regressor":
                                        print_stats(
                                            file_name,
                                            ref_evaluation_report.report_type
                                            + " "
                                            + ref_evaluation_report.evaluation_type,
                                            "rmse",
                                            test_snb_performance.rmse,
                                            ref_snb_performance.rmse,
                                            False,
                                        )
                                        print_stats(
                                            file_name,
                                            ref_evaluation_report.report_type
                                            + " "
                                            + ref_evaluation_report.evaluation_type,
                                            "mae",
                                            test_snb_performance.mae,
                                            ref_snb_performance.mae,
                                            False,
                                        )
                                        print_stats(
                                            file_name,
                                            ref_evaluation_report.report_type
                                            + " "
                                            + ref_evaluation_report.evaluation_type,
                                            "nlpd",
                                            test_snb_performance.nlpd,
                                            ref_snb_performance.nlpd,
                                            False,
                                        )
    # Ecriture des stats
    fstats.close()


def apply_command_bench(work_dir):
    # build bench parameter file
    def extract_info(line):
        start, end = line.split(" ", 1)
        field, comment = end.split("//", 1)
        return field

    # extraction des renseignement du fichier de parametrage
    class_file_name = ""
    class_name = ""
    database_name = ""
    target_attribute_name = ""
    prm_file_name = os.path.join(work_dir, TEST_PRM)
    prm_file = open(prm_file_name, "r")
    for s in prm_file:
        if s.find("class_file_name") >= 0 and class_file_name == "":
            class_file_name = extract_info(s)
        if s.find("class_name") >= 0 and class_name == "":
            class_name = extract_info(s)
        if s.find("TrainDatabase.database_name") >= 0 and database_name == "":
            database_name = extract_info(s)
        if (
            s.find("AnalysisSpec.target_attribute_name") >= 0
            and target_attribute_name == ""
        ):
            target_attribute_name = extract_info(s)
    prm_file.close()
    # affichage des lignes de fichier de bencgmark correspondant
    print("")
    print("BenchmarkSpecs.InsertItemAfter      // Insert after")
    print("// -> Benchmark")
    print("class_file_name " + class_file_name + " // Dictionary file")
    print("class_name " + class_name + " // Dictionary")
    print("target_attribute_name " + target_attribute_name + " // Target variable")
    print("database_name " + database_name + " // Database file")
    print("Exit                           // OK")
    print("// <- Benchmark")


def apply_command_clean(work_dir):
    # clean comparison log file
    file_path = os.path.join(work_dir, cr.COMPARISON_LOG_FILE_NAME)
    if os.path.isfile(file_path):
        utils.remove_file(file_path)

    # clean test results directory
    test_dir = os.path.join(work_dir, RESULTS)
    if os.path.isdir(test_dir):
        for file_name in os.listdir(test_dir):
            file_path = os.path.join(test_dir, file_name)
            utils.remove_file(file_path)
        utils.remove_dir(test_dir)


def apply_command_clean_ref(work_dir):
    # clean reference results directory
    ref_dir, _ = get_results_ref_dir(work_dir, show=True)
    if ref_dir is not None and os.path.isdir(ref_dir):
        for file_name in os.listdir(ref_dir):
            file_path = os.path.join(ref_dir, file_name)
            utils.remove_file(file_path)


def apply_command_delete_ref(work_dir):
    # delete reference results files and directories for all reference contexts
    current_ref_dir, all_ref_dirs = get_results_ref_dir(work_dir, show=True)
    if current_ref_dir is not None:
        for ref_dir in all_ref_dirs:
            for file_name in os.listdir(ref_dir):
                file_path = os.path.join(ref_dir, file_name)
                utils.remove_file(file_path)
            utils.remove_dir(ref_dir)


def apply_command_make_ref(work_dir):
    # copy results files to from test to reference dir
    test_dir = os.path.join(work_dir, RESULTS)
    ref_dir, _ = get_results_ref_dir(work_dir, show=True)
    if ref_dir is not None:
        if not os.path.isdir(ref_dir):
            os.mkdir(ref_dir)
        if os.path.isdir(ref_dir):
            for file_name in os.listdir(ref_dir):
                file_path = os.path.join(ref_dir, file_name)
                utils.remove_file(file_path)
        if os.path.isdir(test_dir) and os.path.isdir(ref_dir):
            for file_name in os.listdir(test_dir):
                utils.copy(
                    os.path.join(test_dir, file_name), os.path.join(ref_dir, file_name)
                )


def apply_command_copy_ref(work_dir):
    # copy results files to from reference to test dir
    test_dir = os.path.join(work_dir, RESULTS)
    ref_dir, _ = get_results_ref_dir(work_dir, show=True)
    if ref_dir is not None:
        if not os.path.isdir(test_dir):
            os.mkdir(test_dir)
        if os.path.isdir(test_dir):
            for file_name in os.listdir(test_dir):
                file_path = os.path.join(test_dir, file_name)
                utils.remove_file(file_path)
        if os.path.isdir(test_dir) and os.path.isdir(ref_dir):
            for file_name in os.listdir(ref_dir):
                utils.copy(
                    os.path.join(ref_dir, file_name), os.path.join(test_dir, file_name)
                )


def apply_command_check_hdfs(work_dir):
    def parameter_exist(line, searched_keyword):
        # Test if there is at least one parameter in a line
        fields = (
            line[line.find(searched_keyword) + len(searched_keyword) :]
            .strip()
            .split("//")
        )
        return len(fields) > 0 and len(fields[0]) > 0

    # Check compliance to HDFS system
    keywords = [
        "class_file_name",
        "ResultFilesDirectory",
        ".database_name",
        "DataTableName",
        ".EvaluationFileName",
        "ReportFileName",
        "InputCoclusteringFileName",
    ]
    prm_file_name = os.path.join(work_dir, TEST_PRM)
    print(work_dir)
    with open(prm_file_name, "r") as prm_file:
        line_index = 1
        for s in prm_file:
            # Test comments
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
            # Test each keyword
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


def apply_command_transform_hdfs(work_dir):
    def parameter_exist(line, searched_keyword):
        # Test if there is at least one parameter in a line and a value for this parameter
        fields = (
            line[line.find(searched_keyword) + len(searched_keyword) :]
            .strip()
            .split("//")
        )
        return len(fields) > 0 and len(fields[0]) > 0

    # Create new file test.prm.hdfs compliant with hdfs
    keywords = [
        "class_file_name",
        "ResultFilesDirectory",
        ".database_name",
        ".DataTableName",
        "ReportFileName",
        "InputCoclusteringFileName",
    ]
    # PostProcessedCoclusteringFileName CoclusteringDictionaryFileName removed
    # The name of the coclustering dictionary should not be a path
    prm_file_name = os.path.join(work_dir, TEST_PRM)
    prm_file = open(prm_file_name, "r", errors="ignore")
    prm_file_lines = prm_file.readlines()
    prm_file.close()
    prm_file = open(prm_file_name, "w")
    for s in prm_file_lines:
        new_line = s
        # Test comments
        if "//" in s:
            comment_pos = s.find("//")
            if (
                comment_pos > 0
                and s[comment_pos - 1] != " "
                and s[comment_pos - 1] != "\t"
            ):
                if s[comment_pos + 2 :].find("//") >= 0:
                    print(
                        "\tWARNING: Multiple '//' in line (NO TRANSFORM) -> " + s[:-1]
                    )
                else:
                    new_line = s.replace("//", " //")
        # Test each keyword
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

        # Special case for the token "EvaluationFileName", must not be confused with TestEvaluationFileName
        if (
            s.find("EvaluationFileName") == 0
            and not s.find(" ./")
            and parameter_exist(s, "EvaluationFileName")
        ):
            space_pos = s.find(" ")
            new_line = s[: space_pos + 1] + "./" + s[space_pos + 1 :]

        prm_file.write(new_line)
    prm_file.close()
    # Transform errror file in reference results dir
    do_it = False
    results_ref, _ = get_results_ref_dir(work_dir, show=True)
    if results_ref is not None:
        err_ref_file_name = os.path.join(work_dir, results_ref, ERR_TXT)
        if do_it and os.path.isfile(err_ref_file_name):
            err_file = open(err_ref_file_name, "r")
            err_file_lines = err_file.readlines()
            err_file.close()
            err_file = open(err_ref_file_name, "w")
            for s in err_file_lines:
                new_line = s
                new_line = new_line.replace(" " + RESULTS + "/", " ./" + RESULTS + "/")
                new_line = new_line.replace(
                    " " + RESULTS + "\\", " ./" + RESULTS + "\\"
                )
                err_file.write(new_line)
            err_file.close()


def escape_for_json(token):
    return token.replace("/", "\\/")


def apply_command_transform_hdfs_results(work_dir):
    hdfs_test_dir = "hdfs:///user/bguerraz/LearningTest/TestKhiops/"
    hdfs_data_dir = "hdfs:///user/bguerraz/LearningTest/"

    std_data_dir = "../../../"
    datasets = "datasets"
    mt_datasets = "MTdatasets"

    head, sub_test_name = os.path.split(work_dir)
    _, test_name = os.path.split(head)

    hdfs_local_dir = hdfs_test_dir + test_name + "/" + sub_test_name

    test_dir = os.path.join(work_dir, RESULTS)
    if os.path.isdir(test_dir):
        for file_name in os.listdir(test_dir):
            file_path = os.path.join(test_dir, file_name)

            # Read in the file
            with open(file_path, "r", errors="ignore") as file:
                file_data = file.read()

                # search and replace
                if ".khj" in file_name:
                    # datasets
                    file_data = file_data.replace(
                        escape_for_json(hdfs_data_dir + datasets),
                        escape_for_json(std_data_dir + datasets),
                    )
                    file_data = file_data.replace(
                        escape_for_json(hdfs_data_dir + mt_datasets),
                        escape_for_json(std_data_dir + mt_datasets),
                    )

                    # current dir ./
                    file_data = file_data.replace(
                        escape_for_json(hdfs_local_dir + "/" + RESULTS),
                        escape_for_json("./" + RESULTS),
                    )  # ou RESULTS sans "./"" ?

                    # files in current dir

                    # file_data = file_data.replace(escape_for_json(
                    #    hdfs_local_dir+"/"), "")
                    file_data = file_data.replace(
                        escape_for_json(hdfs_local_dir + "/"), escape_for_json("./")
                    )

                else:
                    # datasets
                    file_data = file_data.replace(
                        hdfs_data_dir + datasets, std_data_dir + datasets
                    )
                    file_data = file_data.replace(
                        hdfs_data_dir + mt_datasets, std_data_dir + mt_datasets
                    )

                    # current dir ./
                    file_data = file_data.replace(
                        hdfs_local_dir + "/" + RESULTS, "./" + RESULTS
                    )

                    # files in current dir
                    file_data = file_data.replace(hdfs_local_dir + "/", "./")

                # Write the file in place
                os.chmod(file_path, stat.S_IWRITE | stat.S_IREAD)
                with open(file_path, "w") as output_file:
                    output_file.write(file_data)


def apply_command_make_ref_err(work_dir):
    test_dir = os.path.join(work_dir, RESULTS)
    ref_dir, _ = get_results_ref_dir(work_dir, show=True)
    if ref_dir is not None:
        if not os.path.isdir(ref_dir):
            os.mkdir(ref_dir)
        if os.path.isdir(ref_dir):
            file_path = os.path.join(ref_dir, ERR_TXT)
            utils.remove_file(file_path)
        if os.path.isdir(test_dir) and os.path.isdir(ref_dir):
            utils.copy(os.path.join(test_dir, ERR_TXT), os.path.join(ref_dir, ERR_TXT))


def apply_command_make_ref_time(work_dir):
    # copy time file to reference results dir
    test_dir = os.path.join(work_dir, RESULTS)
    ref_dir, _ = get_results_ref_dir(work_dir, show=True)
    if ref_dir is not None:
        if not os.path.isdir(ref_dir):
            os.mkdir(ref_dir)
        if os.path.isdir(ref_dir):
            file_path = os.path.join(ref_dir, TIME_LOG)
            if os.path.isfile(file_path):
                utils.remove_file(file_path)
        if os.path.isdir(test_dir) and os.path.isdir(ref_dir):
            utils.copy(
                os.path.join(test_dir, TIME_LOG),
                os.path.join(ref_dir, TIME_LOG),
            )


def apply_command_work(work_dir):
    test_dir = os.path.join(work_dir, RESULTS)
    ref_dir, _ = get_results_ref_dir(work_dir, show=True)
    if ref_dir is None:
        return
    dir_name = os.path.basename(work_dir)
    root_name = os.path.basename(os.path.dirname(work_dir))
    tool_name = os.path.basename(os.path.dirname(os.path.dirname(work_dir)))

    # Transformation du fichier .prm
    transform_prm = False
    if transform_prm:
        file_path = os.path.join(work_dir, TEST_PRM)
        try:
            lines = file_read_lines(file_path)
            with open(file_path, "w") as the_file:
                for line in lines:
                    if line.find("EpsilonBinNumber") >= 0:
                        continue
                    if line.find("OutlierManagementHeuristic") >= 0:
                        continue
                    if line.find("OptimalAlgorithm") >= 0:
                        continue
                    if line.find("EpsilonBinWidth") >= 0:
                        continue
                    if line.find("MaxIntervalNumber") >= 0:
                        continue
                    if line.find("HistogramCriterion") >= 0:
                        continue
                    if line.find("MaxHierarchyLevel") >= 0:
                        continue
                    the_file.write(line)
        except Exception as e:
            print("BUG: " + file_path + " : " + str(e))

    # Parcours du repertoire de reference
    compare_histograms = True
    if compare_histograms:
        print("COMPARE " + work_dir)
        indicators = [
            "Null cost",
            "Reference null cost",
            "Cost",
            "Level",
            "Partition cost",
        ]
        if os.path.isdir(ref_dir):
            for file_name in os.listdir(ref_dir):
                ref_file_path = os.path.join(ref_dir, file_name)
                test_file_path = os.path.join(test_dir, file_name)
                if not os.path.isfile(test_file_path):
                    print("Missing ref file: " + file_name)
                elif "istogram.log" in file_name:
                    ref_lines = file_read_lines(ref_file_path)
                    test_lines = file_read_lines(test_file_path)
                    ref_indicators = {}
                    test_indicators = {}
                    ref_histogram = []
                    test_histogram = []
                    # Analyse des resultats de references
                    for line in ref_lines:
                        # Collecte des indicateurs
                        for indicator in indicators:
                            if len(line) < 70 and indicator in line:
                                fields = line[:-1].split("\t")
                                try:
                                    ref_indicators[indicator] = float(
                                        fields[len(fields) - 1]
                                    )
                                except Exception as e:
                                    print(
                                        "  "
                                        + file_name
                                        + ": Ref conversion error: "
                                        + line[:-1]
                                        + " "
                                        + str(e)
                                    )
                        # Collectes des lignes de l'histogramme
                        if (
                            len(ref_histogram) > 0
                            or "Lower bound\tUpper bound\tFrequency" in line
                        ):
                            ref_histogram.append(line)
                    # Analyse des resultats de test
                    for line in test_lines:
                        # Collecte des indicateurs
                        for indicator in indicators:
                            if len(line) < 70 and indicator in line:
                                fields = line[:-1].split("\t")
                                try:
                                    test_indicators[indicator] = float(
                                        fields[len(fields) - 1]
                                    )
                                except Exception as e:
                                    print(
                                        "  "
                                        + file_name
                                        + ": Test conversion error: "
                                        + line[:-1]
                                        + " "
                                        + str(e)
                                    )
                        # Collectes des lignes de l'histogramme
                        if (
                            len(test_histogram) > 0
                            or "Lower bound\tUpper bound\tFrequency" in line
                        ):
                            test_histogram.append(line)
                    # Comparaison des resultats
                    for indicator in indicators:
                        ref_value = ref_indicators[indicator]
                        test_value = test_indicators[indicator]
                        if (
                            abs(ref_value - test_value)
                            > abs(ref_value + test_value) / 100000
                        ):
                            print(
                                "  "
                                + file_name
                                + ": Difference in "
                                + indicator
                                + ": "
                                + str(ref_value)
                                + " vs "
                                + str(test_value)
                            )
                    if len(ref_histogram) != len(test_histogram):
                        print(
                            "  "
                            + file_name
                            + ": Difference in interval number: "
                            + str(len(ref_histogram) - 1)
                            + " vs "
                            + str(len(test_histogram) - 1)
                        )
                    else:
                        for i in range(len(ref_histogram)):
                            ref_line = ref_histogram[i]
                            test_line = test_histogram[i]
                            ref_line_fields = ref_line.split("\t")
                            test_line_fields = test_line.split("\t")
                            # Comparaison des 9 permiers champs
                            compare_ok = True
                            for f in range(8):
                                compare_ok = (
                                    compare_ok
                                    and ref_line_fields[f] == test_line_fields[f]
                                )
                                if not compare_ok:
                                    print(
                                        "  "
                                        + file_name
                                        + ": Difference in interval "
                                        + str(i)
                                        + " field "
                                        + str(f + 1)
                                        + ": \n\t"
                                        + ref_line
                                        + "\t"
                                        + test_line
                                    )
                                    break


def apply_command_template(work_dir):
    test_dir = os.path.join(work_dir, RESULTS)
    ref_dir, _ = get_results_ref_dir(work_dir, show=True)
    if ref_dir is None:
        return
    dir_name = os.path.basename(work_dir)
    root_name = os.path.basename(os.path.dirname(work_dir))
    tool_name = os.path.basename(os.path.dirname(os.path.dirname(work_dir)))


def register_command(
    available_commands: dict, command_id: str, command_function, command_label: str
):
    """Register a command in a dictionnary of commands"""
    assert command_id not in available_commands
    assert command_id != ""
    assert command_label != ""
    available_commands[command_id] = (command_function, command_label)


def display_commands(available_commands: dict, max_number=None):
    """Display available commands, with their id and label"""
    assert max_number is None or max_number > 0
    # Print generic info
    print("apply_command [command] [root_path] ([dir_name])")
    print("  apply command on a directory structure")
    print("\tcommand: name of the command")
    print("\trootPath is the path of the root directory")
    print("\tdirName is the name of one specific sub-directory")
    print("\t         or all (default) for executing on all sub-directories")
    print("   example: applyCommand list TestKhiops\\Standard")
    print("   example: applyCommand list TestKhiops\\Standard Adult")
    print("\n List of available standard commands (* for all commands):")
    # Print list of available commands
    for i, command_id in enumerate(available_commands):
        if max_number is None or i < max_number:
            (command_function, command_label) = available_commands[command_id]
            print("\t" + command_id + ": " + command_label)


def execute_command(
    available_commands: dict, command_id, root_path, test_dir_name=None
):
    """Internal use.
    Same as apply_command, with a dictionnary of commands as first parameter
    """
    assert command_id != ""
    assert root_path != ""
    # Verification des operandes
    if command_id not in available_commands:
        print("error: wrong command " + command_id)
        exit(0)
    if not os.path.isdir(root_path):
        print("error: root directory " + root_path + " does not exist")
        exit(0)
    root_path = os.path.abspath(root_path)
    # Verification de l'utilisation d'un repertoire de test d'un des outils
    tool_ok = False
    for name in test_khiops.khiops_tool_names:
        tool_directory = test_khiops.khiops_test_sub_dirs[name]
        tool_ok = tool_ok or tool_directory in root_path
    if not tool_ok:
        print(
            "error: root directory "
            + root_path
            + " should contain a test dir for one of the tools "
            + str(test_khiops.khiops_tool_names)
            .replace("'", "")
            .replace("[", "(")
            .replace("]", ")")
        )
        exit(0)
    # Recherche des sous-repertoires a exploiter
    test_list = []
    if test_dir_name is None:
        for name in os.listdir(root_path):
            if os.path.isdir(os.path.join(root_path, name)):
                test_list.append(name)
    else:
        if os.path.isdir(os.path.join(root_path, test_dir_name)):
            test_list.append(test_dir_name)
        else:
            print(
                "error: sub-directory "
                + test_dir_name
                + " of "
                + root_path
                + " does not exist"
            )
            exit(0)
    if len(test_list) == 0:
        print("error: no sub-directory is available in " + root_path)
        exit(0)
    # Sort test list
    test_list.sort()
    # Execution de la commande
    (command_function, command_label) = available_commands[command_id]
    for name in test_list:
        # lanceur de commande sur un directory
        work_dir = os.path.join(root_path, name)
        # verification de l'existence du directory
        if not os.path.isdir(work_dir):
            print("error: directory " + work_dir + " does not exist")
            return 0
        # Lancement de la commande dans son repertoire de travail
        os.chdir(work_dir)
        command_function(work_dir)
        os.chdir(root_path)
    # Message synthetique de fin si famille de jeu de tests
    family_dir_name = os.path.basename(root_path)
    tool_test_sub_dir = os.path.basename(os.path.dirname(root_path))
    if test_dir_name is None:
        print("DONE\t" + tool_test_sub_dir + "\t" + family_dir_name)


def register_all_commands():
    """Register all available commands
    Return a dictionary of the registered commands
    """
    # Gestion de l'ensemble des commandes dans un dictionnaire contenant pour chaque identifiant de commande
    # une paire (comannd, libelle)
    all_available_commands = {}

    # Enregistrement des commandes standard
    register_command(
        all_available_commands,
        "list",
        apply_command_list,
        "list of sub-directories, with results.ref info",
    )
    register_command(
        all_available_commands,
        "errors",
        apply_command_errors,
        "report errors and warnings",
    )
    register_command(
        all_available_commands,
        "logs",
        apply_command_logs,
        "detailed report errors and warnings",
    )
    register_command(
        all_available_commands,
        "compareTimes",
        apply_command_compare_times,
        "compare time with ref time and report warnings only",
    )
    register_command(
        all_available_commands,
        "compareTimesVerbose",
        apply_command_compare_times_verbose,
        "compare time with ref time and report all",
    )
    register_command(
        all_available_commands,
        "performance",
        apply_command_performance,
        "report SNB test accuracy",
    )
    register_command(
        all_available_commands,
        "performanceRef",
        apply_command_performance_ref,
        "report ref SNB test accuracy",
    )
    register_command(
        all_available_commands,
        "clean",
        apply_command_clean,
        "delete test result files and comparison log file",
    )
    register_command(
        all_available_commands,
        "cleanref",
        apply_command_clean_ref,
        "delete reference result files for current reference context",
    )
    register_command(
        all_available_commands,
        "deleteref",
        apply_command_delete_ref,
        "delete reference result files and directories for all reference context",
    )
    register_command(
        all_available_commands,
        "makeref",
        apply_command_make_ref,
        "copy test result files to reference dir for current reference context",
    )
    register_command(
        all_available_commands,
        "copyref",
        apply_command_copy_ref,
        "copy reference result files to test dir for current reference context",
    )
    register_command(
        all_available_commands,
        "checkHDFS",
        apply_command_check_hdfs,
        "check if parameter files are compliant with HDFS",
    )
    register_command(
        all_available_commands,
        "transformHDFS",
        apply_command_transform_hdfs,
        "transform parameter files to be compliant with HDFS",
    )
    register_command(
        all_available_commands,
        "transformHDFSresults",
        apply_command_transform_hdfs_results,
        "transform results files to be compliant with HDFS",
    )
    standard_command_number = len(all_available_commands)

    # Enregistrement des commandes internes
    register_command(
        all_available_commands,
        "makereftime",
        apply_command_make_ref_time,
        "copy time file to reference results dir",
    )
    register_command(
        all_available_commands,
        "makereferr",
        apply_command_make_ref_err,
        "copy err file to reference results dir",
    )
    register_command(
        all_available_commands,
        "bench",
        apply_command_bench,
        "build bench parameter file",
    )
    register_command(
        all_available_commands,
        "checkfnb",
        apply_command_check_fnb,
        "check fnb results (deprecated)",
    )
    register_command(
        all_available_commands,
        "work",
        apply_command_work,
        "last work command (temporary and anonymous)",
    )
    return all_available_commands, standard_command_number


if __name__ == "__main__":
    all_commands, standard_command_number = register_all_commands()

    # Affichage des commandes si pas de parametres ou mauvais nombre de parametres
    if len(sys.argv) <= 2:
        display_all = len(sys.argv) == 2 and sys.argv[1] == "*"
        display_commands(
            all_commands, max_number=None if display_all else standard_command_number
        )
        exit(0)

    # Recherche des parametres sur la ligne de commande
    command_param = sys.argv[1]
    root_path_param = sys.argv[2]
    if len(sys.argv) == 4:
        test_dir_param = sys.argv[3]
    else:
        test_dir_param = None

    # Lancement de la commande
    execute_command(all_commands, command_param, root_path_param, test_dir_param)
