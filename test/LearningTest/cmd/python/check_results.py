import os.path
import sys
import shutil
import csv
import string
import subprocess


def print_error(log_file, error_message):
    print(error_message)
    # on encode en utf-8 en ignorant les erreurs pour eviter un erreur lors de l'encodage automatique
    log_file.write(error_message.encode("utf-8", "ignore").decode("utf-8") + "\n")


def print_detailed_error(log_file, error_message, number_print):
    number_print_max = 10
    if number_print < number_print_max:
        print(error_message)
        log_file.write(error_message + "\n")
    if number_print == number_print_max:
        print("...\n")
        log_file.write("...\n")


def check_results(test):
    # compare les fichiers 2 a 2 et ecrit les resultat dans le fichier comparisonResults.log
    print("--Comparing results...")
    ref_dir = os.path.join(os.getcwd(), test, "results.ref")
    if not os.path.isdir(ref_dir):
        print("reference directory (" + ref_dir + ") not available")
        return 0
    test_dir = os.path.join(os.getcwd(), test, "results")
    if not os.path.isdir(test_dir):
        print("test directory (" + test_dir + ") not available")
        return 0
    number_errors = 0
    number_warnings = 0
    number_files = 0
    log_file = open(os.path.join(os.getcwd(), test, "comparisonResults.log"), "w")
    log_file.write(test.upper() + " comparison\n\n")
    # test des fichiers 2 a 2
    for file_name in os.listdir(ref_dir):
        [errors, warnings] = check_file(
            log_file,
            os.path.join(ref_dir, file_name),
            os.path.join(test_dir, file_name),
        )
        number_files = number_files + 1
        number_errors = number_errors + errors
        number_warnings = number_warnings + warnings
    # comparaison du nombre de fichiers
    if len(os.listdir(ref_dir)) == 0:
        print_error(log_file, "no comparison: missing reference result files")
        number_errors = number_errors + 1
    if len(os.listdir(ref_dir)) > 0 and len(os.listdir(ref_dir)) != len(
        os.listdir(test_dir)
    ):
        print_error(
            log_file,
            "number of results files ("
            + str(len(os.listdir(test_dir)))
            + ") should be "
            + str(len(os.listdir(ref_dir))),
        )
        number_errors = number_errors + 1
    # report errors in err.txt file; if no ref file
    if len(os.listdir(ref_dir)) == 0:
        err_file_name = os.path.join(test_dir, "err.txt")
        err_file = open(err_file_name, "r", errors="ignore")
        for s in err_file:
            if s.find("error") >= 0:
                print_detailed_error(
                    log_file, err_file_name + ": " + s, number_errors + number_warnings
                )
                number_errors = number_errors + 1
            if (
                s.find("warning") >= 0
                and not s.find("converted in 0") >= 0
                and not s.find("...") >= 0
            ):
                print_detailed_error(
                    log_file, err_file_name + ": " + s, number_errors + number_warnings
                )
                number_warnings = number_warnings + 1
            if s.find("failure") >= 0:
                print_detailed_error(
                    log_file, err_file_name + ": " + s, number_errors + number_warnings
                )
                number_errors = number_errors + 1
        err_file.close()
    log_file.write("\n" + str(number_warnings) + " warning(s)\n")
    log_file.write(str(number_errors) + " error(s)")
    log_file.close()

    print(
        "--Comparison done : "
        + str(number_files)
        + " files(s) compared, "
        + str(number_errors)
        + " error(s), "
        + str(number_warnings)
        + " warning(s)"
    )
    print(
        "log writed in "
        + os.path.join(os.getcwd(), test, "comparisonResults.log")
        + "\n"
    )


def check_file(log_file, path_ref, path_test):
    def filter_time(value):
        # Supression d'un pattern de time d'une valeur
        pos_start_time = value.find(" time:")
        if pos_start_time >= 0:
            begin_value = value[:pos_start_time]
            end_value = value[pos_start_time + len(" time:") :]
            end_value = end_value.strip()
            pos_end_time = end_value.find(" ")
            if pos_end_time >= 0:
                end_value = end_value[pos_end_time:]
            else:
                end_value = ""
            filtered_value = begin_value + " time: ..." + filter_time(end_value)
        else:
            filtered_value = value
        return filtered_value

    def filter_secondary_record(value):
        # Supression d'un pattern de nombre de records secondaires
        pos_start1 = value.find(" after reading ")
        pos_start2 = value.find(" secondary records ")
        if pos_start1 >= 0 and pos_start2 >= 0:
            filtered_value = (
                value[:pos_start1] + " after reading ..." + value[pos_start2:]
            )
        else:
            filtered_value = value
        return filtered_value

    def filter_khiops_temp_dir(value):
        # Nettoyage de la partie temp directory d'une valeur
        pos_khiops_temp_dir = value.find("~Khiops")
        if pos_khiops_temp_dir >= 0:
            # Recherche du debut du path du fichier
            begin_pos = pos_khiops_temp_dir
            while begin_pos > 0 and value[begin_pos] != " ":
                begin_pos -= 1
            # Recherche de la fin du repertoire temporaire
            end_pos = pos_khiops_temp_dir
            while (
                end_pos < len(value)
                and value[end_pos] != "/"
                and value[end_pos] != "\\"
            ):
                end_pos += 1
            while end_pos < len(value) and (
                value[end_pos] == "/" or value[end_pos] == "\\"
            ):
                end_pos += 1
            # Remplacement du nom du repertoire par un nom "logique"
            begin_value = value[0:begin_pos]
            end_value = value[end_pos : len(value)]
            # Recherche du nom de fichier en debut de la end_value qui suit le nom du repertoire temporaire
            filtered_filename = ""
            end_filename_pos = end_value.find(" ")
            if end_filename_pos != -1:
                filename = end_value[0:end_filename_pos]
                end_value = end_value[end_filename_pos:]
            else:
                filename = end_value
                end_value = ""
            # Filtrage de l'eventuel nom de fichier en remplacant les chiffres  par le pattern XXX
            # pour se rendre independant des eventuels index de fichiers temporaires
            i = 0
            while i < len(filename):
                c = filename[i]
                if c != "_" and not c.isdigit():
                    filtered_filename += c
                else:
                    filtered_filename += "XXX"
                    while i < len(filename):
                        c = filename[i]
                        if c != "_" and not c.isdigit():
                            filtered_filename += c
                            break
                        i += 1
                i += 1
            filtered_value = (
                begin_value + " KHIOPS_TMP_DIR/" + filtered_filename + end_value
            )
        else:
            filtered_value = value
        return filtered_value

    # compare les fichiers ligne par ligne, cellule par cellule
    # pour les valeurs numeriques, une diffence relative de 0.00001 est toleree
    # ecrit les difference dans le fichier log_file et affiche le nb d'erreur dans le terminal
    # warning : 2 cellules contiennent des valeurs numeriques avec une difference relative toleree
    # error : les cellules sont differentes
    if not os.path.isfile(path_ref):
        print_error(log_file, "file " + path_ref + " is missing")
        return [1, 0]
    if not os.path.isfile(path_test):
        print_error(log_file, "file " + path_test + " is missing")
        return [1, 0]

    log_file.write(
        "\nfile " + path_test.encode("utf-8", "ignore").decode("utf-8") + "\n"
    )

    # test si fichier de temps
    is_time_file = os.path.basename(path_ref) == "time.log"

    # test si fichier histogramme
    is_histogram_file = "histogram" in os.path.basename(
        path_ref
    ) and ".log" in os.path.basename(path_ref)

    # test si fichier d'erreur
    is_error_file = os.path.basename(path_ref) == "err.txt"

    # test si fichier de benchmark
    is_benchmark_file = os.path.basename(path_ref) == "benchmark.xls"

    # Test si fichier json
    is_json_file = ".json" in os.path.basename(path_ref)
    is_json_file = is_json_file or ".khj" in os.path.basename(path_ref)
    is_json_file = is_json_file or ".khvj" in os.path.basename(path_ref)
    is_json_file = is_json_file or ".khcj" in os.path.basename(path_ref)
    is_json_file = is_json_file or ".kdicj" in os.path.basename(path_ref)
    # Cas particulier des fichier .bad qui sont en fait des fichiers json
    # (ex: LearningTest\TestKhiops\Advanced\AllResultsApiMode
    if ".bad" in os.path.basename(path_ref):
        if (
            os.path.isfile(path_ref.replace(".bad", ".khj"))
            or os.path.isfile(path_ref.replace(".bad", ".khj"))
            or os.path.isfile(path_ref.replace(".bad", ".kdicj"))
        ):
            is_json_file = True
    json_errors = 0
    json_derivatrion_rules_errors = 0

    # initialisation des nombres d'erreurs
    error = 0
    warning = 0

    # Pas de controle si fichier de temps
    if is_time_file:
        return [error, warning]

    # nombre de lignes de chaque fichier
    file_ref = open(path_ref, "r", errors="ignore")
    file_test = open(path_test, "r", errors="ignore")
    file_test_line_number = 0
    try:
        file_test_line_number = len(file_test.readlines())
    except BaseException as message:
        print("Error: can't compute line number (" + str(message) + ")")
    file_ref_line_number = 0
    try:
        file_ref_line_number = len(file_ref.readlines())
    except BaseException as message:
        print("Error: can't compute line number (" + str(message) + ")")
    if file_test_line_number != file_ref_line_number:
        log_file.write(
            "test file has "
            + str(file_test_line_number)
            + " lines and reference file has "
            + str(file_ref_line_number)
            + " lines\n"
        )
        error = error + 1
    file_ref.close()
    file_test.close()

    # ouverture des fichiers consideres commes des tableaux de cellules separees par des tabulation
    csv.field_size_limit(500000000)
    file_ref = open(path_ref, "r", errors="ignore")
    file_test = open(path_test, "r", errors="ignore")
    # Cas particulier du fichier d'erreur, que l'on ouvre en ignorant les tabulations
    if is_error_file:
        file_ref_csv = csv.reader(file_ref, delimiter="\n")
        file_test_csv = csv.reader(file_test, delimiter="\n")
    else:
        file_ref_csv = csv.reader(file_ref, delimiter="\t")
        file_test_csv = csv.reader(file_test, delimiter="\t")

    # comparaison ligne a ligne
    max_threshold = 0
    number_print_max = 10
    max_field_length = 100
    line = 0
    skip_benchmark_lines = 0
    try:
        for row_t in file_test_csv:
            line += 1
            if line > file_test_line_number or line > file_ref_line_number:
                break

            # parcours des fichiers ligne par ligne
            row_r = next(file_ref_csv)
            length_r = len(row_r)
            length_t = len(row_t)
            i = 0

            # comparaison des nombre de colonnes
            if length_r != length_t:
                log_file.write(
                    "test file (line "
                    + str(line)
                    + ") has "
                    + str(length_t)
                    + " columns and reference file has "
                    + str(length_r)
                    + " columns\n"
                )
                error = error + 1
                break

            # cas special du fichier d'erreur: on tronque les lignes qui font du reporting de temps de calcul (" time:")
            if (
                is_error_file
                and length_r > 0
                and row_r[i].find(" time:") != -1
                and length_t > 0
                and row_t[i].find(" time:") != -1
            ):
                row_r[i] = filter_time(row_r[i])
                row_t[i] = filter_time(row_t[i])

            # cas special du fichier d'erreur:
            # on saute les lignes qui font du reporting de temps de calcul ("interrupted after")
            if (
                is_error_file
                and length_r > 0
                and row_r[i].find(" interrupted ") != -1
                and length_t > 0
                and row_t[i].find(" interrupted ") != -1
            ):
                continue

            # cas special du fichier d'erreur, pour le message "(Operation canceled)" qui n'est pas case sensistive
            if is_error_file:
                if length_r > 0 and row_r[i].find("(Operation canceled)") != -1:
                    row_r[i] = row_r[i].replace(
                        "(Operation canceled)", "(operation canceled)"
                    )
                if length_t > 0 and row_t[i].find("(Operation canceled)") != -1:
                    row_t[i] = row_t[i].replace(
                        "(Operation canceled)", "(operation canceled)"
                    )

            # cas special du fichier d'erreur en coclustering:
            # on saute les lignes d'ecritire de rapport intermediaire qui different par le temps
            # ("Write intermediate coclustering report")
            if (
                is_error_file
                and length_r > 0
                and row_r[i].find("Write intermediate coclustering report") != -1
                and length_t > 0
                and row_t[i].find("Write intermediate coclustering report") != -1
            ):
                continue

            # cas special du fichier d'histogramme:
            # on tronque les lignes qui font du reporting de temps de calcul (" time\t")
            if (
                is_histogram_file
                and length_r > 2
                and row_r[1].find("time") != -1
                and length_t > 2
                and row_t[1].find("time") != -1
            ):
                row_r[2] = ""
                row_t[2] = ""
            # cas special du fichier d'histogramme:
            # on ignore le champ tronque les lignes qui font du reporting de temps de calcul (" time\t")
            if (
                is_histogram_file
                and length_r >= 2
                and row_r[0].find("Version") != -1
                and length_t >= 2
                and row_t[0].find("Version") != -1
            ):
                row_r[1] = ""
                row_t[1] = ""

            # cas special du caractere # en tete de premiere ligne de fichier (identifiant de version d'application)
            if (
                line == 1
                and length_r > 0
                and row_r[0].find("#") == 0
                and length_t > 0
                and row_t[0].find("#") == 0
            ):
                continue
            # idem pour des informations de licences d'un fichier d'erreur
            if (
                is_error_file
                and line == 2
                and length_r > 0
                and row_r[0].find("Khiops ") == 0
                and length_t > 0
                and row_t[0].find("Khiops ") == 0
            ):
                continue

            # cas special du champ version des fichiers json (identifiant de version d'application)
            if (
                is_json_file > 0
                and length_r == 2
                and row_r[1].find("version") == 0
                and length_t == 2
                and row_t[1].find("version") == 0
            ):
                continue

            # cas special des fichiers de benchmark:
            # on saute les blocs de ligne dont le role est le reporting de temps de calcul
            # ("Time" dans le premier champ d'entete)
            if is_benchmark_file and length_r > 0 and row_r[i].find("Time") != -1:
                skip_benchmark_lines = 1
                continue
            if is_benchmark_file and skip_benchmark_lines:
                # fin de bloc si ligne vide
                skip_benchmark_lines = 0
                j = 0
                while j < length_r:
                    if row_r[j] != "":
                        skip_benchmark_lines = 1
                    j += 1
            if skip_benchmark_lines:
                continue

            # comparaison des cellules
            while i < length_r:
                # parcours des lignes cellule par cellule
                if i < length_t:
                    # cas special du fichier d'erreur ou json: on tronque les chemins vers les repertoires temporaires de Khiops
                    if (
                        (is_error_file or is_json_file)
                        and length_r > i
                        and row_r[i].find("~Khiops") != -1
                        and length_t > i
                        and row_t[i].find("~Khiops") != -1
                    ):
                        row_r[i] = filter_khiops_temp_dir(row_r[i])
                        row_t[i] = filter_khiops_temp_dir(row_t[i])

                    # cas special du fichier d'erreur ou khj: on tronque le compte des lignes avec des warning sur le nombre de records secondaires
                    if (
                        (is_error_file or is_json_file)
                        and length_r > 0
                        and "warning" in row_r[i]
                        and " after reading " in row_r[i]
                        and " secondary records " in row_r[i]
                        and length_t > 0
                        and "warning" in row_t[i]
                        and " after reading " in row_t[i]
                        and " secondary records " in row_t[i]
                    ):
                        row_r[i] = filter_secondary_record(row_r[i])
                        row_t[i] = filter_secondary_record(row_t[i])

                    # cas general de comparaison de cellules
                    [eval_res, threshold_res] = check_cell(row_r[i], row_t[i])
                    # truncature des champs affiches dans les messages d'erreur
                    row_t_i = row_t[i]
                    if len(row_t_i) > max_field_length:
                        row_t_i = row_t_i[0:max_field_length] + "..."
                    row_r_i = row_r[i]
                    if len(row_r_i) > 5:
                        row_r_i = row_r_i[0:max_field_length] + "..."
                    # messages d'erreur
                    if eval_res == 0:
                        if error < number_print_max or threshold_res > max_threshold:
                            log_file.write(
                                "l"
                                + str(file_test_csv.line_num)
                                + " c"
                                + str(i + 1)
                                + " "
                                + row_t_i
                                + " -> "
                                + row_r_i
                                + "\n"
                            )
                        elif error == number_print_max:
                            log_file.write("...\n")
                        error = error + 1
                        # Cas particulier des erreurs dans le fichier json, si elles sont dues a la regle de derivation
                        if is_json_file:
                            json_errors += 1
                            if row_t[i].find("derivationRule:") >= 0:
                                json_derivatrion_rules_errors += 1
                    elif eval_res == 2:
                        warning = warning + 1
                    if threshold_res > max_threshold:
                        max_threshold = threshold_res
                else:
                    # apparemment il y a des cellules vides en plus dans le fichier de reference...
                    if len(row_r[i]) != 0:
                        if error < number_print_max:
                            log_file.write(
                                "l"
                                + str(file_test_csv.line_num)
                                + " c"
                                + str(i + 1)
                                + " "
                                + row_r[i]
                                + " disappeared\n"
                            )
                        elif error == number_print_max:
                            log_file.write("...\n")
                        error = error + 1
                i = i + 1
            # end while
        # end for
    except BaseException as message:
        print(
            "Error: can't compare file csv cells in "
            + os.path.basename(path_ref)
            + " ("
            + str(message)
            + ")"
        )
    file_ref.close()
    file_test.close()
    # print(str(error)+" error(s)")
    if warning > 0:
        log_file.write(str(warning) + " warning(s) (epsilon difference)\n")
    if error == 0:
        log_file.write("OK\n")
    elif max_threshold > 0:
        log_file.write("max relative difference: " + str(max_threshold) + "\n")
    if error > 0:
        log_file.write(str(error) + " error(s)")
        if is_json_file and 0 < json_errors == json_derivatrion_rules_errors:
            log_file.write(" (only in derivation rules)")
        log_file.write("\n")
    return [error, warning]


def split_cell(cell):
    # decoupe une chaine de caractere en un tableau de sous-chaines, qui sont des portions numeriques, soit non numeriques
    max_substrings = 10000
    length = len(cell)
    i = 0
    substrings = []
    substring = ""
    float_type = 1
    string_type = 2
    previous_type = 0
    while i < length:
        c = cell[i]
        if c in ".0123456789":
            cell_type = float_type
        else:
            cell_type = string_type
        if type == previous_type:
            substring = substring + c
        else:
            if previous_type != 0:
                substrings.append(substring)
            substring = "" + c
        previous_type = cell_type
        i = i + 1
        if i == length:
            substrings.append(substring)
        if len(substrings) > max_substrings:
            return substrings
    return substrings


# return true if time format


def is_time(val):
    # si format time (?h:mm:ss), on ignore en renvoyant OK
    time = val.strip()
    for c in time:
        if c not in ":0123456789":
            return False
    if 7 <= len(time) <= 8:
        if time.find(":") >= 1 and time.find(":", 3) >= 4:
            return True
    return False


def check_value(val1, val2):
    # check_cell, dans le cas de valeurs elementaires
    # Ok si valeurs egales
    if val1 == val2:
        return [1, 0]
    # Sinon, tentative de comparaison numerique
    threshold = float(0.00001)
    try:
        float1 = float(val1)
        float2 = float(val2)
        res = (
            0.5 * abs(float1 - float2) / (abs(float1) / 2 + abs(float2) / 2 + threshold)
        )
        if res <= threshold:
            return [2, res]
        return [0, res]
    # Erreur si format non numerique et difference
    except ValueError:
        return [0, 0]


def check_cell(cell1, cell2):
    # comparaison de deux cellules
    # pour les valeurs numeriques, une diffence relative de 0.00001 est toleree
    # renvoi 1 si les cellules sont identiques
    #       2 si les la difference relative est toleree
    #       0 si les cellules sont differentes

    if cell1 == cell2:
        return [1, 0]

    # si les deux cellules sont des time, on renvoie OK pour ignorer la comparaison
    if is_time(cell1) and is_time(cell2):
        return [1, 0]

    # cas special du fichier d'erreur : on saute les lignes qui indiquent les ressources manquante
    if cell1.find("system resources") != -1 and cell2.find("system resources") != -1:
        return [1, 0]

    # cas special des erreurs d'acces sur HDFS
    if (
        cell1.find("Unable to access file") != -1
        and cell2.find("Unable to open file") != -1
    ):
        return [1, 0]

    if (
        cell1.find("Unable to open file") != -1
        and cell2.find("nable to access file") != -1
    ):
        return [1, 0]

    # cas special des erreurs d'acces sur HDFS
    if (
        cell1.find("No enought memory to build trees") != -1
        and cell2.find("No enought memory to build trees") != -1
    ):
        return [1, 0]

    # cas special lorsque il y a trop de modalites cibles (?)
    if (
        cell1.find("Too much memory necessary to store the values") != -1
        and cell2.find("Too much memory necessary to store the values") != -1
    ):
        return [1, 0]

    # uniformisation entre windows et linux pour les chemins de fichier
    # on va remplacer les \ par des /
    string1 = cell1.replace("\\", "/")
    string2 = cell2.replace("\\", "/")

    # Tolerance temporaire pour le passage au format hdfs
    # hdfs_value1 = cell1.replace("./", "")
    # hdfs_value1 = hdfs_value1.replace(".\\/..\\/", "")
    # hdfs_value1 = hdfs_value1.replace("..\\/", "")
    # hdfs_value1 = hdfs_value1.replace(".\\/", "")
    # hdfs_value2 = cell2.replace("./", "")
    # hdfs_value2 = hdfs_value2.replace(".\\/..\\/", "")
    # hdfs_value2 = hdfs_value2.replace("..\\/", "")
    # hdfs_value2 = hdfs_value2.replace(".\\/", "")
    # if hdfs_value1 == hdfs_value2:
    #     return [1, 0]

    if string1 == string2:
        return [1, 0]

    # sinon c'est peut etre un pbm d'arrondi
    # on accepte les differences relatives faibles
    threshold = float(0.00001)
    try:
        float1 = float(cell1)
        float2 = float(cell2)
        res = (
            0.5 * abs(float1 - float2) / (abs(float1) / 2 + abs(float2) / 2 + threshold)
        )
        if res <= threshold:
            return [2, res]
        return [0, res]
    except ValueError:
        # on arrive pas a le convertir en float, ce n'est pas un nombre
        # on decoupe chaque cellule sous la forme d'un ensemble de sous-chaines qui sont soit
        # des libelles, soit des float
        substrings1 = split_cell(cell1)
        substrings2 = split_cell(cell2)

        # nombre de sous-chaines differentes: il y a erreur
        if len(substrings1) != len(substrings2):
            return [0, 0]
        # comparaison pas a pas
        else:
            i = 0
            length = len(substrings1)
            full_eval = 1
            full_threshold = 0
            while i < length:
                [eval_result, threshold_result] = check_value(
                    substrings1[i], substrings2[i]
                )
                if eval_result == 0:
                    return [0, 0]
                if eval_result == 2:
                    full_eval = 2
                if threshold_result > full_threshold:
                    full_threshold = threshold_result
                i = i + 1
            return [full_eval, full_threshold]
