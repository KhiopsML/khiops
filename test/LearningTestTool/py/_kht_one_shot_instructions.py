import os.path
import sys
import stat
import warnings

import _kht_constants as kht
import _kht_utils as utils
import _kht_check_results as check
import _kht_results_management as results
import _kht_standard_instructions as standard_instructions

"""
Instruction pour des usages uniques
Peu documente et developpe rapidment sous forme de prototype
Exemples:
- manipulation a faire une fois sur l'ensemble des repertoires de test
- modification des scenario selon evoilution de l'ergonomie de Khiops core
- evaluation de l'impcat sur les performances d'une evolution des algorithme de Khiops core
- ...
"""

# Imports de pykhiops a effectuer au cas par cas dans chaque methode, car ralentissant trop les scripts
# import khiops as pk


def instruction_make_ref_time(test_dir):
    # Copie du fichier de temps vers le repertoire des resultats de reference
    results_dir = os.path.join(test_dir, kht.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is not None:
        if not os.path.isdir(results_ref_dir):
            os.mkdir(results_ref_dir)
        if os.path.isdir(results_ref_dir):
            file_path = os.path.join(results_ref_dir, kht.TIME_LOG)
            if os.path.isfile(file_path):
                utils.remove_file(file_path)
        if os.path.isdir(results_dir) and os.path.isdir(results_ref_dir):
            utils.copy_file(
                os.path.join(results_dir, kht.TIME_LOG),
                os.path.join(results_ref_dir, kht.TIME_LOG),
            )


def instruction_make_ref_err(test_dir):
    results_dir = os.path.join(test_dir, kht.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is not None:
        if not os.path.isdir(results_ref_dir):
            os.mkdir(results_ref_dir)
        if os.path.isdir(results_ref_dir):
            file_path = os.path.join(results_ref_dir, kht.ERR_TXT)
            utils.remove_file(file_path)
        if os.path.isdir(results_dir) and os.path.isdir(results_ref_dir):
            utils.copy_file(
                os.path.join(results_dir, kht.ERR_TXT),
                os.path.join(results_ref_dir, kht.ERR_TXT),
            )


def instruction_bench(test_dir):
    # Construction de scenario de benchmark
    def extract_info(line):
        start, end = line.split(" ", 1)
        field, comment = end.split("//", 1)
        return field

    # extraction des renseignement du fichier de parametrage
    class_file_name = ""
    class_name = ""
    database_name = ""
    target_attribute_name = ""
    prm_file_path = os.path.join(test_dir, kht.TEST_PRM)
    prm_file = open(prm_file_path, "r", errors="ignore")
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


def instruction_check_fnb(test_dir):
    from khiops import core as pk

    def to_s(value):
        return str("{:.4g}".format(value))

    def print_stats(
        result_file_name, report, criterion, new_value, ref_value, maximize
    ):
        fstats.write(
            "\t" + tool_dir_name + "\t" + suite_dir_name + "\t" + test_dir_name + "\t"
        )
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
        print(
            "\t"
            + tool_dir_name
            + "\t"
            + suite_dir_name
            + "\t"
            + test_dir_name
            + "\terror\t"
            + message
        )

    results_dir = os.path.join(test_dir, kht.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is None:
        return
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)

    # Analyse du log de comparaison
    (
        error_number,
        warning_number,
        summary_infos,
        files_infos,
    ) = check.analyse_comparison_log(test_dir)

    # On verifie que les resultats hors SNB sont correct (ex: preparation)
    preparation_ok = True
    if error_number >= 0:
        for file_name in files_infos:
            if "Preparation" in file_name and ".xls" in file_name:
                preparation_ok = preparation_ok and files_infos[file_name] == "OK"

    # Creation d'un fichier de collecte des stats
    fstats = None
    home_dir = utils.get_home_dir(test_dir)
    stats_file_path = os.path.join(home_dir, tool_dir_name, "stats.FNB.log")
    if os.path.isfile(stats_file_path):
        fstats = open(stats_file_path, "a", errors="ignore")
    else:
        fstats = open(stats_file_path, "w", errors="ignore")
        fstats.write(
            "Tool\tRoot\tDir\tFile\tReport\tCriterion\tValue\tRef value\tDiff\n"
        )

    if results_ref_dir is not None and os.path.isdir(results_ref_dir):
        for file_name in os.listdir(results_ref_dir):
            ref_file_path = os.path.join(results_ref_dir, file_name)
            test_file_path = os.path.join(results_dir, file_name)

            #####
            if not os.path.isfile(test_file_path):
                print_error("Missing file " + test_file_path)
                continue

            # Comparaison du fichier d'erreur
            if file_name == kht.ERR_TXT:
                if not standard_instructions.file_compare(
                    ref_file_path, test_file_path, skip_patterns=["time"]
                ):
                    print_error(file_name + " are different")
            # Comparaison si preparation
            elif "PreparationReport" in file_name:
                if not standard_instructions.file_compare(
                    ref_file_path, test_file_path, skip_patterns=["#Khiops "]
                ):
                    print_error(file_name + " are different")
            elif ".khj" in file_name:
                # Lecture du fichier de resultats json
                try:
                    ref_report = pk.AnalysisResults()
                    ref_report.read_khiops_json_file(ref_file_path)
                    test_report = pk.AnalysisResults()
                    test_report.read_khiops_json_file(test_file_path)
                except Exception as e:
                    print_error(file_name + "\tparsing alert: " + str(e))
                    continue
                # Analyse des resultats de modelisation
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
                # Analyse des resultats d'evaluation
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


def instruction_work(test_dir):
    results_dir = os.path.join(test_dir, kht.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is None:
        return
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)

    # Transformation du fichier .prm
    transform_prm = False
    if transform_prm:
        file_path = os.path.join(test_dir, kht.TEST_PRM)
        lines = utils.read_file_lines(file_path)
        try:
            with open(file_path, "w", errors="ignore") as the_file:
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
        print("COMPARE " + test_dir)
        indicators = [
            "Null cost",
            "Reference null cost",
            "Cost",
            "Level",
            "Partition cost",
        ]
        if os.path.isdir(results_ref_dir):
            for file_name in os.listdir(results_ref_dir):
                ref_file_path = os.path.join(results_ref_dir, file_name)
                test_file_path = os.path.join(results_dir, file_name)
                if not os.path.isfile(test_file_path):
                    print("Missing ref file: " + file_name)
                elif "istogram.log" in file_name:
                    ref_lines = utils.read_file_lines(ref_file_path)
                    test_lines = utils.read_file_lines(test_file_path)
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


def instruction_compare_snb_perfs(test_dir):
    # Obtain basic information of the test
    results_dir = os.path.join(test_dir, kht.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is None:
        return
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)

    # Obtain the sub-tests names
    khj_file_names = [
        name for name in os.listdir(results_ref_dir) if name.endswith("khj")
    ]
    sub_test_names = []
    for name in khj_file_names:
        sub_test_name = name.replace(".khj", "")
        if "AnalysisResults" in name:
            sub_test_name = sub_test_name.replace("AnalysisResults", "")
            if not sub_test_name:
                sub_test_name = "None"
        if "EvaluationReport" in name:
            sub_test_name = sub_test_name.replace("EvaluationReport", "")
            if not sub_test_name:
                sub_test_name = "None"
        sub_test_names.append(sub_test_name)

    # Exctract performances measures with khiops-python
    from khiops import core as kh

    # Ignore warnings about colliding characters in reports
    warnings.filterwarnings("ignore", message=r".*colliding")

    # Shorten the test dir to show
    test_dir_show_name = f"{test_dir_name[:12]}"
    line_prefix = f"{tool_dir_name}\t{suite_dir_name}\t{test_dir_show_name}"
    for file_name, sub_test_name in zip(khj_file_names, sub_test_names):

        # Shorten sub test to show
        sub_test_show_name = f"{sub_test_name[:12]}"

        # Read the sub-test report files
        error_reason = None
        try:
            all_reports_ref = kh.read_analysis_results_file(
                os.path.join(results_ref_dir, file_name)
            )
        except FileNotFoundError:
            error_reason = "ReportRefNotFound"
        except kh.KhiopsJSONError as error:
            error_reason = str(error)
        if error_reason is None:
            try:
                all_reports = kh.read_analysis_results_file(
                    os.path.join(results_dir, file_name)
                )
            except FileNotFoundError:
                error_reason = "ReportNotFound"
            except kh.KhiopsJSONError as error:
                error_reason = str(error)
        if error_reason is None:
            if len(all_reports_ref.get_reports()) != len(all_reports.get_reports()):
                error_reason = "NumReportNonEq"

        # Print error line on failure
        if error_reason is not None:
            print(f"{line_prefix}\t{sub_test_show_name}\tERROR\t{error_reason}")
            return

        # Get the number of selected variables
        model_report_ref = all_reports_ref.modeling_report
        model_report = all_reports.modeling_report
        if (
            model_report_ref is not None
            and "Selective Naive Bayes" in model_report_ref.get_predictor_names()
        ):
            snb_var_number_ref = (
                all_reports_ref.modeling_report.get_snb_predictor().variable_number
            )
            snb_var_number = (
                all_reports.modeling_report.get_snb_predictor().variable_number
            )
        else:
            snb_var_number_ref = -1
            snb_var_number = -1

        # Print the metric, metric_ref and the relative error of metric w/r metric_ref
        for report_ref, report in zip(
            all_reports_ref.get_reports(), all_reports.get_reports()
        ):
            if not isinstance(report, kh.EvaluationReport):
                continue
            try:
                snb_perf_ref = report_ref.get_snb_performance()
            except ValueError as error:
                print(f"{line_prefix}\t{sub_test_show_name}\tERROR\tSNB: {error}")
                continue
            try:
                snb_perf = report.get_snb_performance()
            except ValueError as error:
                print(f"{line_prefix}\t{sub_test_show_name}\tERROR\tSNBRef: {error}")
                continue

            # Obtain the evaluation type (Train,Test,None)
            evaluation_type = report.evaluation_type
            if not evaluation_type:
                evaluation_type = "None"

            for metric_name in snb_perf_ref.get_metric_names():
                metric_ref = snb_perf_ref.get_metric(metric_name)
                metric = snb_perf.get_metric(metric_name)
                if metric_ref is None:
                    print(
                        f"{line_prefix}\t{sub_test_show_name}\tSKIPPED\tNullRefMetric"
                    )
                elif metric is None:
                    print(f"{line_prefix}\t{sub_test_show_name}\tERROR\tNullMetric")
                else:
                    if metric_ref == 0 and metric == 0:
                        rel_diff = 0
                    elif metric_ref == 0:
                        rel_diff = (metric - 1e-9) / (1e-9)
                    else:
                        rel_diff = (metric - metric_ref) / (metric_ref)
                    if 0 < abs(rel_diff) < 1e-9:
                        rel_diff = 0
                    print(
                        f"{line_prefix}\t{sub_test_show_name}\t{evaluation_type}\t{metric_name}\t{metric}\t{metric_ref}\t{rel_diff:.6g}\t{snb_var_number}\t{snb_var_number_ref}"
                    )


def instruction_template(test_dir):
    results_dir = os.path.join(test_dir, kht.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is None:
        return
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)


"""
Enregistrement des instructions
"""


def register_one_shot_instructions():
    """
    Enregistrement des instructions a usage unique
    Retourne un dictionnaire d'instructions
    """

    # Gestion de l'ensemble des instructions dans un dictionnaire contenant pour chaque identifiant d'instruction
    # une paire (instruction, libelle)
    available_instructions = {}

    # Enregistrement des instructions
    standard_instructions.register_instruction(
        available_instructions,
        "makereftime",
        instruction_make_ref_time,
        "copy time file to reference results dir",
    )
    standard_instructions.register_instruction(
        available_instructions,
        "makereferr",
        instruction_make_ref_err,
        "copy err file to reference results dir",
    )
    standard_instructions.register_instruction(
        available_instructions,
        "bench",
        instruction_bench,
        "build bench parameter file",
    )
    standard_instructions.register_instruction(
        available_instructions,
        "checkfnb",
        instruction_check_fnb,
        "check fnb results (deprecated)",
    )
    standard_instructions.register_instruction(
        available_instructions,
        "work",
        instruction_work,
        "last work instruction (temporary and uncommented)",
    )
    standard_instructions.register_instruction(
        available_instructions,
        "compare-snb-perfs",
        instruction_compare_snb_perfs,
        "compares the performances of all SNB predictors",
    )
    return available_instructions
