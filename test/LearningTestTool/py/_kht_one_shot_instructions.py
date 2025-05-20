# Copyright (c) 2023-2025 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

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


def instruction_compare_snb_perfs(test_dir):
    """Compares the SNB performances of a test

    For each report in the results that contains a SNB predictor it compares the
    performances between the reference and the test results. The results are reported as
    a TSV table.

    The columns for each test are:
        - Family name (usually only 'TestKhiops')
        - Suite name
        - Test name
        - Subtest name: The prefix of an "*AnalysisRelsults.khj" file
        - Evaluation type: 'Test', 'Train' or 'None'
        - Metric name:
            - Classification: 'accuracy', 'auc', and 'compression'
            - Regression: 'rmse', 'mae', 'nlpd', 'rank_rmse', 'rank_mae', 'rank_nlpd'
        - Metric value
        - Reference metric value
        - Relative difference: Between 0 and 1
        - Selected variable number
        - Reference selected variable number
        - Preparation 1D flag: Empty string '' if the 1D preparations are equal,
                               'Prep1D_KO' otherwise.


    When the comparison is not feasible then the line printed will "SKIPPED" and when
    the results are not coherent with the reference these are reported as "ERROR". A
    synthetic key of the reason of failure is printed after "SKIPPED"/"ERROR".

    """
    # Import khiops-python
    try:
        from khiops import core as kh
    except ImportError:
        print("This command requires the khiops-python python package")
        exit(1)

    def _check_same_1d_preparation(prep_report_ref, prep_report):
        """Returns 'Prep1D_KO' if the 1D preparations are not equal, '' otherwise"""
        # Status values
        prep_1d_failure = "Prep1D_KO"
        prep_1d_status = ""

        # Report as Ok if both reports are not present
        if prep_report_ref is None and prep_report is None:
            pass
        # Report error if:
        # - the reference does not exist but the results do
        # - the result does not exist but the reference do
        # Similarly with the variable_statistics
        elif (
            (prep_report_ref is None and prep_report is not None)
            or (prep_report_ref is not None and prep_report is None)
            or (
                prep_report_ref.variables_statistics is None
                and prep_report.variables_statistics is not None
            )
            or (
                prep_report_ref.variables_statistics is not None
                and prep_report.variables_statistics is None
            )
        ):
            prep_1d_status = prep_1d_failure
        # Otherwise there are valid reports/var_stats in both reference and results
        else:
            # Report error if the variable statistcs differ in size
            if len(prep_report_ref.variables_statistics) != len(
                prep_report.variables_statistics
            ):
                prep_1d_status = prep_1d_failure
            # Otherwise check that each variable has the same name, level and rank
            else:
                for var_stats_ref, var_stats in zip(
                    prep_report_ref.variables_statistics,
                    prep_report.variables_statistics,
                ):
                    if (
                        var_stats_ref.name != var_stats.name
                        or var_stats_ref.rank != var_stats.rank
                        or var_stats_ref.level != var_stats.level
                    ):
                        prep_1d_status = prep_1d_failure
                        break
        return prep_1d_status

    # End of local function _check_same_1d_preparation

    # Obtain basic information of the test
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    results_dir = os.path.join(test_dir, kht.RESULTS)
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

    # Ignore warnings about colliding characters in reports
    warnings.filterwarnings("ignore", message=r".*colliding")

    # Build the beginning of this test's lines
    line_prefix = f"{tool_dir_name}\t{suite_dir_name}\t{test_dir_name}"
    for file_name, sub_test_name in zip(khj_file_names, sub_test_names):
        # Read the sub-test report files
        failure_type = None
        failure_reason = None
        try:
            all_reports_ref = kh.read_analysis_results_file(
                os.path.join(results_ref_dir, file_name)
            )
        except FileNotFoundError:
            failure_type = "SKIPPED"
            failure_reason = "ReportRefNotFound"
        except kh.KhiopsJSONError as error:
            failure_type = "SKIPPED"
            failure_reason = str(error)
        if failure_reason is None:
            try:
                all_reports = kh.read_analysis_results_file(
                    os.path.join(results_dir, file_name)
                )
            except FileNotFoundError:
                failure_type = "ERROR"
                failure_reason = "ReportNotFound"
            except kh.KhiopsJSONError as error:
                failure_type = "ERROR"
                failure_reason = str(error)
        if failure_reason is None:
            if len(all_reports_ref.get_reports()) != len(all_reports.get_reports()):
                failure_type = "ERROR"
                failure_reason = "NumReportNonEq"
        # Check the type
        if failure_reason is None:
            if (
                all_reports_ref.preparation_report is not None
                and all_reports_ref.preparation_report.learning_task
                == "Unsupervised analysis"
            ):
                failure_type = "SKIPPED"
                failure_reason = "UnsupervisedTask"

        # Print the line on failure and return
        if failure_reason is not None:
            print(f"{line_prefix}\t{sub_test_name}\t{failure_type}\t{failure_reason}")
            return

        # Check that the preparation
        prep_1d_status = _check_same_1d_preparation(
            all_reports_ref.preparation_report, all_reports.preparation_report
        )

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
            if not (
                isinstance(report, kh.EvaluationReport)
                and "Selective Naive Bayes" in report.get_predictor_names()
            ):
                continue
            try:
                snb_perf_ref = report_ref.get_snb_performance()
            except ValueError as error:
                print(f"{line_prefix}\t{sub_test_name}\tERROR\tSNB: {error}")
                continue
            try:
                snb_perf = report.get_snb_performance()
            except ValueError as error:
                print(f"{line_prefix}\t{sub_test_name}\tERROR\tSNBRef: {error}")
                continue

            # Obtain the evaluation type (Train, Test or None)
            evaluation_type = report.evaluation_type
            if not evaluation_type:
                evaluation_type = "Eval"

            for metric_name in snb_perf_ref.get_metric_names():
                metric_ref = snb_perf_ref.get_metric(metric_name)
                metric = snb_perf.get_metric(metric_name)
                if metric_ref is None:
                    print(
                        f"{line_prefix}\t{sub_test_name}\t{metric_name}\t"
                        "SKIPPED\tNullRefMetric"
                    )
                elif metric is None:
                    print(
                        f"{line_prefix}\t{sub_test_name}\t{metric_name}\t"
                        "ERROR\tNullMetric"
                    )
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
                        f"{line_prefix}\t{sub_test_name}\t{evaluation_type}\t"
                        f"{metric_name}\t{metric}\t{metric_ref}\t{rel_diff:.6g}\t"
                        f"{snb_var_number}\t{snb_var_number_ref}\t{prep_1d_status}"
                    )


def instruction_new_data_path(test_dir):
    """
    Update scenario file with new specification of data paths

    This script should work for V11 and V10 scenarios (in case of backport)
    As a "quick and dirty" script, it works well in many cases, but manual correction
    is still necessary in some cases (about ten test sets among 800 in LearningTest V11).

    The script should be applied to the whole LearningTest tree using
      "kht_apply <LarningTest dir> new-data-path -f all"
    The LearningTest directory tree should be save before using this script, in order
    to recover from some potential erroneous modifications of existing scenarios.
    """
    prm_file_path = os.path.join(test_dir, kht.TEST_PRM)
    if os.path.isfile(prm_file_path):
        # Read content of prm file
        try:
            with open(prm_file_path, "r", errors="ignore") as prm_file:
                lines = prm_file.readlines()
        except Exception as e:
            print("BUG: " + prm_file_path + " : " + str(e))
        # Update management of data path
        pattern = "DatabaseFiles.List.Key"  # Same pattern in V10 and V11
        new_lines = []
        new_scenario_necessary = False
        data_path_block_current_line = -1
        data_path_block_main_dictionary = ""
        for i, line in enumerate(lines):
            line = line.strip()
            # Case of a line containing a data path
            if pattern in line:
                new_scenario_necessary = True
                # Split line into (instruction, data path, comment)
                pos = line.find(pattern)
                #  Begin of line before data path
                instruction = line[: pos + len(pattern)]
                #  Search begin of data path
                end_line = line[pos + len(pattern) :]
                while end_line.find(" ") == 0:
                    end_line = end_line[1:]
                #  Extract datapath
                pos = end_line.find(" ")
                if pos >= 0:
                    data_path = end_line[:pos]
                    comment = end_line[pos:]
                else:
                    data_path = end_line
                    comment = ""
                if data_path.find("//") == 0:
                    comment = data_path + comment
                    data_path = ""
                debug = False
                if debug:
                    print(str(i + 1) + " :" + line)
                    print("\t" + instruction)
                    print("\t" + data_path)
                    print("\t" + comment)
                # Stop if the scenario has already been transformed
                # Allows to avoid applying the scenario several times with detrimental effects
                # But prevents the script to be applied even once in some rare cases, that
                # have to be treated manually
                if data_path == "":
                    new_scenario_necessary = False
                    break
                # Build new line
                new_line = instruction
                data_path_elements = data_path.split("`")
                assert len(data_path_elements) > 0
                # Heuristic for managing main dictionary vs external table dictionary
                if data_path_block_current_line == -1:
                    data_path_block_main_dictionary = data_path_elements[0]
                data_path_block_current_line = i
                dictionary = data_path_elements[0]
                new_line += " "
                if dictionary != data_path_block_main_dictionary:
                    new_line += "/" + dictionary
                    if len(data_path_elements) > 1:
                        new_line += "/"
                for e, element in enumerate(data_path_elements):
                    if e > 0:
                        if e > 1:
                            new_line += "/"
                        new_line += data_path_elements[e]
                new_line += comment
                new_lines.append(new_line)
                if debug:
                    print("\t" + new_line)
            # Standard case
            else:
                # Heuristic to detect a new block of data path instructions
                if i > data_path_block_current_line + 2:
                    data_path_block_current_line = -1
                    data_path_block_main_dictionary = ""
                new_lines.append(line)
        # Write content of prm file
        if new_scenario_necessary:
            try:
                with open(prm_file_path, "w", errors="ignore") as prm_file:
                    for line in new_lines:
                        prm_file.write(line + "\n")
            except Exception as e:
                print("BUG: " + prm_file_path + " : " + str(e))


def instruction_clean_version(test_dir):
    # Collecte de tous les repertoires de resultats, de reference ou non
    all_results_dir = [os.path.join(test_dir, kht.RESULTS)]
    results_ref_dir, all_results_ref_dirs = results.get_results_ref_dir(
        test_dir, show=True
    )
    for dir in all_results_ref_dirs:
        all_results_dir.append(os.path.join(test_dir, dir))
    # Nettoyage de chaque repertoire de reference
    for dir in all_results_dir:
        check.clean_version_from_results(dir)


def instruction_clean_intervals(test_dir):
    "Renommage systematique du separateur ';' en ',' dans les intervalles"
    results_dir = os.path.join(test_dir, kht.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is None:
        return
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)
    # Collecte de tous les repertoires de resultats, de reference
    all_results_dir = []
    results_ref_dir, all_results_ref_dirs = results.get_results_ref_dir(
        test_dir, show=True
    )

    # On ne traite pas les repertoires avec tests de fichers a l'ancien format
    if "SemiColumn" not in test_dir:
        # Traitement des eventuels fichiers de Coclustering utilises en entree des prm et presents a la racine
        for name in os.listdir(test_dir):
            path_name = os.path.join(test_dir, name)
            if "Coclustering" in name and ".khcj" in name:
                lines = utils.read_file_lines(path_name)
                # Transformation du fichier test.prm pour transformer les ";" en ","
                to_transform = False
                new_lines = []
                for i, line in enumerate(lines):
                    # Recherche si ";"" est utilise en tant que separateur
                    fields = line.split()
                    if ";" in line:
                        line = line.replace(";", ",")
                        to_transform = True
                    new_lines.append(line)
                if to_transform:
                    utils.write_file_lines(path_name, new_lines)
                    print(
                        tool_dir_name
                        + "/"
                        + suite_dir_name
                        + "/"
                        + test_dir_name
                        + "/"
                        + name
                    )
            if "SubDir" in name:
                for name2 in os.listdir(path_name):
                    if "Coclustering" in name2 and ".khcj" in name2:
                        path_name2 = os.path.join(path_name, name2)
                        lines = utils.read_file_lines(path_name2)
                        # Transformation du fichier test.prm pour transformer les ";" en ","
                        to_transform = False
                        new_lines = []
                        for i, line in enumerate(lines):
                            # Recherche si ";"" est utilise en tant que separateur
                            fields = line.split()
                            if ";" in line:
                                line = line.replace(";", ",")
                                to_transform = True
                            new_lines.append(line)
                        if to_transform:
                            utils.write_file_lines(path_name2, new_lines)
                            print(
                                tool_dir_name
                                + "/"
                                + suite_dir_name
                                + "/"
                                + test_dir_name
                                + "/"
                                + name2
                            )

        # Traitement du contenu des repertoires results.ref
        for dir in all_results_ref_dirs:
            all_results_dir.append(os.path.join(test_dir, dir))

            # Nettoyage de chaque repertoire de reference
            for results_ref_dir in all_results_dir:
                # Parcours des fichiers du repertoire de references
                for name in os.listdir(results_ref_dir):
                    path_name = os.path.join(test_dir, results_ref_dir, name)

                    # Noms des fichiers devant etre traites
                    bFileOk = (
                        "PreparationReport" in name
                        or "Preparation2DReport" in name
                        or "EvaluationReport" in name
                        or "ModelingReport" in name
                        or "Deploy" in name
                        or "Coclustering" in name
                    )
                    bSpecialFileOk = (
                        "Label_D_Iris" in name
                        or "D_20newsgroups" in name
                        or "R_Adult_relationship" in name
                        or "D_Adult" in name
                        or "I_Iris" in name
                        or "D_Iris" in name
                        or "R_Iris" in name
                        or "R_Isolet" in name
                        or "D_spliceJunction" in name
                        or "ClustersVariables" in name
                        or "ClustersAge" in name
                    )
                    if bFileOk or bSpecialFileOk:
                        lines = utils.read_file_lines(path_name)
                        # Transformation du fichier test.prm pour transformer les ";" en ","
                        to_transform = False
                        new_lines = []
                        for i, line in enumerate(lines):
                            # Recherche si ";" est utilise en tant que separateur dans un intervalle de valeurs
                            fields = line.split()
                            if "inf;" in line:
                                line = line.replace("inf;", "inf,")
                                to_transform = True
                            if ";+inf" in line:
                                line = line.replace(";+inf", ",+inf")
                                to_transform = True
                            for i in range(0, 10):
                                nb1 = line.count(str(i) + ";")
                                nb2 = line.count(";)")
                                nb3 = line.count("; )")
                                nb4 = line.count("[") + line.count("]")
                                if nb1 > nb2 and nb1 > nb3 and nb4 >= 2 * nb1:
                                    line = line.replace(str(i) + ";", str(i) + ",")
                                    to_transform = True
                            new_lines.append(line)
                        if to_transform:
                            utils.write_file_lines(path_name, new_lines)
                            print(
                                tool_dir_name
                                + "/"
                                + suite_dir_name
                                + "/"
                                + test_dir_name
                                + "/"
                                + name
                            )


def instruction_work(test_dir):
    results_dir = os.path.join(test_dir, kht.RESULTS)
    results_ref_dir, _ = results.get_results_ref_dir(test_dir, show=True)
    if results_ref_dir is None:
        return
    test_dir_name = utils.test_dir_name(test_dir)
    suite_dir_name = utils.suite_dir_name(test_dir)
    tool_dir_name = utils.tool_dir_name(test_dir)

    # Lecture du fichier test.prm
    prm_file_path = os.path.join(test_dir, kht.TEST_PRM)
    if os.path.isfile(prm_file_path):
        lines = utils.read_file_lines(prm_file_path)
        # Transformation du fichier test.prm pour transformer les None en none
        to_transform = False
        new_lines = []
        for i, line in enumerate(lines):
            # Recherche si None est utilise en tant que token avec des separateur
            fields = line.split()
            if "None" in fields:
                line = line.replace("None", "none")
                to_transform = True
            new_lines.append(line)
        if to_transform:
            utils.write_file_lines(prm_file_path, new_lines)
            print(tool_dir_name + "/" + suite_dir_name + "/" + test_dir_name)


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
        "compare-snb-perfs",
        instruction_compare_snb_perfs,
        "compares the performances of all SNB predictors",
    )
    standard_instructions.register_instruction(
        available_instructions,
        "new-data-path",
        instruction_new_data_path,
        "update scenarios with new data path spec",
    )
    standard_instructions.register_instruction(
        available_instructions,
        "clean-version",
        instruction_clean_version,
        "clean version info in all results dirs",
    )
    standard_instructions.register_instruction(
        available_instructions,
        "clean-intervals",
        instruction_clean_intervals,
        "rename ';' to ',' in all intervals",
    )
    standard_instructions.register_instruction(
        available_instructions,
        "work",
        instruction_work,
        "last work instruction (temporary and uncommented)",
    )
    return available_instructions
