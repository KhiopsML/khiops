import os.path
import re


def print_message(log_file, message):
    print(message)
    write_message(log_file, message)


def write_message(log_file, message):
    # on encode en utf-8 en ignorant les erreurs pour eviter un erreur lors de l'encodage automatique
    log_file.write(message.encode("utf-8", "ignore").decode("utf-8") + "\n")


def print_detailed_message(log_file, message, number_print):
    number_print_max = 10
    if number_print < number_print_max:
        print(message)
        log_file.write(message + "\n")
    if number_print == number_print_max:
        print("...\n")
        log_file.write("...\n")


# Parsers en variables globales, compiles une seule fois
token_parser = None
time_parser = None
numeric_parser = None


def initialize_parsers():
    """Initialisation des parsers globaux"""
    global token_parser
    global time_parser
    global numeric_parser
    if token_parser is not None:
        return
    # Delimiters pour els fichiers json et kdic
    delimiters = ["\,", "\{", "\}", "\[", "\]", "\:", "\(", "\)", "\<", "\>", "\="]
    numeric_pattern = "-?[0-9]+\.?[0-9]*(?:[Ee]-?[0-9]+)?"
    string_pattern = (
        '"[^"]*"'  # Sans les double-quotes dans les strings (dur a parser...)
    )
    time_pattern = "\d{1,2}:\d{2}:\d{2}\.?\d*"
    other_tokens = "[\w]+"
    tokens = time_pattern + "|" + numeric_pattern + "|" + string_pattern
    for delimiter in delimiters:
        tokens += "|" + delimiter
    tokens += "|" + other_tokens
    token_parser = re.compile(tokens)
    numeric_parser = re.compile(numeric_pattern)
    time_parser = re.compile(time_pattern)


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
    number_fatal_errors = 0
    number_errors = 0
    number_warnings = 0
    number_files = 0
    log_file = open(os.path.join(os.getcwd(), test, "comparisonResults.log"), "w")
    write_message(log_file, test.upper() + " comparison\n")
    # Initialisation des parsers
    initialize_parsers()
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
    # recherche des erreurs fatales
    fatal_error_files = [
        "stdout_error.log",
        "stderr_error.log",
        "return_code_error.log",
    ]
    for file_name in os.listdir(test_dir):
        if file_name in fatal_error_files:
            number_fatal_errors = number_fatal_errors + 1
    # comparaison du nombre de fichiers
    if len(os.listdir(ref_dir)) == 0:
        print_message(log_file, "no comparison: missing reference result files")
        number_errors = number_errors + 1
    if len(os.listdir(ref_dir)) > 0 and len(os.listdir(ref_dir)) != len(
        os.listdir(test_dir)
    ):
        print_message(
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
                print_detailed_message(
                    log_file, err_file_name + ": " + s, number_errors + number_warnings
                )
                number_errors = number_errors + 1
            if (
                s.find("warning") >= 0
                and not s.find("converted in 0") >= 0
                and not s.find("...") >= 0
            ):
                print_detailed_message(
                    log_file, err_file_name + ": " + s, number_errors + number_warnings
                )
                number_warnings = number_warnings + 1
            if s.find("failure") >= 0:
                print_detailed_message(
                    log_file, err_file_name + ": " + s, number_errors + number_warnings
                )
                number_errors = number_errors + 1
        err_file.close()
    print_message(log_file, "\n" + str(number_warnings) + " warning(s)")
    print_message(log_file, str(number_errors) + " error(s)")
    if number_fatal_errors > 0:
        print_message(log_file, "FATAL ERROR")
    log_file.close()

    print(
        "--Comparison done : "
        + str(number_files)
        + " files(s) compared, "
        + str(number_errors)
        + " error(s), "
        + str(number_warnings)
        + " warning(s)"
        + ("\nFATAL ERROR" if number_fatal_errors > 0 else "")
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
        print_message(log_file, "file " + path_ref + " is missing")
        return [1, 0]
    if not os.path.isfile(path_test):
        print_message(log_file, "file " + path_test + " is missing")
        return [1, 0]

    # En-tete de comparaison des fichiers
    write_message(log_file, "\nfile " + path_test)

    # Recherche du fichier compare et de son extension
    file_name = os.path.basename(path_ref)
    assert file_name == os.path.basename(path_test)
    _, file_extension = os.path.splitext(file_name)

    # test si fichier de temps
    is_time_file = file_name == "time.log"

    # test si fichier histogramme
    is_histogram_file = "histogram" in file_name and file_extension == ".log"

    # test si fichier d'erreur
    is_error_file = file_name == "err.txt"

    # test si fichier de benchmark
    is_benchmark_file = file_name == "benchmark.xls"

    # test si fichier dictionnaire
    is_kdic_file = file_extension == ".kdic"

    # Test si fichier json
    is_json_file = file_extension in [".json", ".khj", ".khvj", ".khcj", ".kdicj"]
    # Cas particulier des fichier .bad qui sont en fait des fichier json (ex: LearningTest\TestKhiops\Advanced\AllResultsApiMode
    if file_extension == ".bad":
        if (
            os.path.isfile(path_ref.replace(".bad", ".khj"))
            or os.path.isfile(path_ref.replace(".bad", ".khj"))
            or os.path.isfile(path_ref.replace(".bad", ".kdicj"))
        ):
            is_json_file = True

    # initialisation des nombres d'erreurs
    error = 0
    warning = 0

    # Pas de controle si fichier de temps
    if is_time_file:
        return [error, warning]

    # lecture des lignes de chaque fichier
    try:
        with open(path_ref, "r", errors="ignore") as file_ref:
            file_ref_lines = file_ref.readlines()
    except BaseException as message:
        error += 1
        print_message(
            log_file, "Error: can't open file " + path_ref + " (" + str(message) + ")"
        )
        return [error, warning]
    assert file_ref_lines is not None
    try:
        with open(path_test, "r", errors="ignore") as file_test:
            file_test_lines = file_test.readlines()
    except BaseException as message:
        error += 1
        print_message(
            log_file, "Error: can't open file " + path_test + " (" + str(message) + ")"
        )
        return [error, warning]
    assert file_test_lines is not None

    # Comparaison des nombres de lignes
    file_ref_line_number = len(file_ref_lines)
    file_test_line_number = len(file_test_lines)
    if file_test_line_number != file_ref_line_number:
        write_message(
            log_file,
            "test file has "
            + str(file_test_line_number)
            + " lines and reference file has "
            + str(file_ref_line_number)
            + " lines",
        )
        error = error + 1

    # comparaison ligne a ligne
    max_threshold = 0
    max_print_error = 10
    max_field_length = 100
    skip_benchmark_lines = False
    line_number = min(file_ref_line_number, file_test_line_number)
    for index in range(line_number):
        line = index + 1
        line_ref = file_ref_lines[index].rstrip()
        line_test = file_test_lines[index].rstrip()

        # cas special des fichiers de benchmark:
        # on saute les blocs de ligne dont le role est le reporting de temps de calcul
        # ("Time" dans le premier champ d'entete)
        if is_benchmark_file and line_ref.find("Time") != -1:
            skip_benchmark_lines = True
            continue
        if is_benchmark_file and skip_benchmark_lines:
            # fin de bloc si ligne vide
            if line_ref.find("\t") == -1:
                skip_benchmark_lines = False
        if skip_benchmark_lines:
            continue

        # Ok si lignes egales
        if line_ref == line_test:
            continue

        # cas special du fichier d'erreur: on tronque les lignes qui font du reporting de temps de calcul (" time:")
        if (
            is_error_file
            and line_ref.find(" time:") != -1
            and line_test.find(" time:") != -1
        ):
            line_ref = filter_time(line_ref)
            line_test = filter_time(line_test)

        # cas special du fichier d'erreur:
        # on saute les lignes qui font du reporting de temps de calcul ("interrupted ")
        if (
            is_error_file
            and line_ref.lower().find(" interrupted ") != -1
            and line_test.lower().find(" interrupted ") != -1
        ):
            continue

        # cas special du fichier d'erreur, pour le message "(Operation canceled)" qui n'est pas case sensitive
        if is_error_file:
            if line_ref.find("(Operation canceled)") != -1:
                line_ref = line_ref.replace(
                    "(Operation canceled)", "(operation canceled)"
                )
            if line_test.find("(Operation canceled)") != -1:
                line_test = line_test.replace(
                    "(Operation canceled)", "(operation canceled)"
                )

        # cas special du fichier d'erreur en coclustering:
        # on saute les lignes d'ecritire de rapport intermediaire qui different par le temps
        # ("Write intermediate coclustering report")
        if (
            is_error_file
            and line_ref.find("Write intermediate coclustering report") != -1
            and line_test.find("Write intermediate coclustering report") != -1
        ):
            continue

        # cas special du fichier d'histogramme:
        # on tronque les lignes qui font du reporting de temps de calcul (" time\t")
        if (
            is_histogram_file
            and line_ref.find("time") != -1
            and line_test.find("time") != -1
        ):
            line_ref = line_ref[: line_ref.find("time")]
            line_test = line_test[: line_test.find("time")]
        # cas special du fichier d'histogramme:
        # on ignore le champ tronque les lignes qui font du reporting de temps de calcul (" time\t")
        if (
            is_histogram_file
            and line_ref.find("Version") != -1
            and line_test.find("Version") != -1
        ):
            line_ref = ""
            line_test = ""

        # cas special du caractere # en tete de premiere ligne de fichier (identifiant de version d'application)
        if line == 1 and line_ref.find("#") == 0 and line_test.find("#") == 0:
            continue

        # idem pour des informations de licences d'un fichier d'erreur
        if (
            is_error_file
            and line == 2
            and line_ref.find("Khiops ") == 0
            and line_test.find("Khiops ") == 0
        ):
            continue

        # cas special du champ version des fichiers json (identifiant de version d'application)
        if (
            is_json_file
            and line_ref.find('"version": ') >= 0
            and line_test.find('"version": ') >= 0
        ):
            continue

        # Sinon, on analyse les champs
        line_fields_ref = line_ref.split("\t")
        line_fields_test = line_test.split("\t")

        # comparaison des nombres de champs
        field_number_ref = len(line_fields_ref)
        field_number_test = len(line_fields_test)
        if field_number_ref != field_number_test:
            if error < max_print_error:
                write_message(
                    log_file,
                    "test file (line "
                    + str(line)
                    + ") has "
                    + str(field_number_test)
                    + " columns and reference file has "
                    + str(field_number_ref)
                    + " columns",
                )
            elif error == max_print_error:
                write_message(log_file, "...")
            error = error + 1

        # comparaison des champs
        field_number_length = min(field_number_ref, field_number_test)
        for i in range(field_number_length):
            field_ref = line_fields_ref[i]
            field_test = line_fields_test[i]

            # parcours des lignes cellule par cellule
            # cas special du fichier d'erreur ou json: on tronque les chemins vers les repertoires temporaires de Khiops
            if (
                (is_error_file or is_json_file)
                and field_ref.find("~Khiops") != -1
                and field_test.find("~Khiops") != -1
            ):
                field_ref = filter_khiops_temp_dir(field_ref)
                field_test = filter_khiops_temp_dir(field_test)

            # cas special du fichier d'erreur ou khj: on tronque le compte des lignes avec des warning sur le nombre de records secondaires
            if (
                (is_error_file or is_json_file)
                and "warning" in field_ref
                and " after reading " in field_ref
                and " secondary records " in field_ref
                and "warning" in field_test
                and " after reading " in field_test
                and " secondary records " in field_test
            ):
                field_ref = filter_secondary_record(field_ref)
                field_test = filter_secondary_record(field_test)

            # cas general de comparaison de cellules
            [eval_res, threshold_res] = check_cell(field_ref, field_test)
            # truncature des champs affiches dans les messages d'erreur
            if len(field_test) > max_field_length:
                field_test = field_test[0:max_field_length] + "..."
            if len(field_ref) > max_field_length:
                field_ref = field_ref[0:max_field_length] + "..."
            # messages d'erreur
            if eval_res == 0:
                if error < max_print_error or threshold_res > max_threshold:
                    write_message(
                        log_file,
                        "l"
                        + str(line)
                        + " c"
                        + str(i + 1)
                        + " "
                        + field_test
                        + " -> "
                        + field_ref,
                    )
                elif error == max_print_error:
                    write_message(log_file, "...")
                error = error + 1
            elif eval_res == 2:
                warning = warning + 1
            max_threshold = max(threshold_res, max_threshold)
    if warning > 0:
        write_message(log_file, str(warning) + " warning(s) (epsilon difference)")
    if error == 0:
        write_message(log_file, "OK")
    elif max_threshold > 0:
        write_message(log_file, "max relative difference: " + str(max_threshold))
    if error > 0:
        write_message(log_file, str(error) + " error(s)")
    return [error, warning]


def split_cell(cell):
    # Pour gerer les double-quotes a l'interieur des strings, pour les format json et kdic
    cell = cell.replace('\\"', "'")
    cell = cell.replace('""', "'")
    substrings = token_parser.findall(cell)
    return substrings


# return true if time format
def is_time(val):
    # si format time (?h:mm:ss), on ignore en renvoyant OK
    return time_parser.match(val.strip())


def check_value(val1, val2):
    # Comparaison de deux valeurs numeriques
    # renvoie deux valeur:
    # - result:
    #   - 1 si les cellules sont identiques
    #   - 2 si les la difference relative est toleree
    #   - 0 si les cellules sont differentes
    # - threshold: differe,ce relative si result = 2
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
    # renvoie deux valeur:
    # - result:
    #   - 1 si les cellules sont identiques
    #   - 2 si les la difference relative est toleree
    #   - 0 si les cellules sont differentes
    # - threshold: differe,ce relative si result = 2

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
        and cell2.find("Unable to access file") != -1
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
    if numeric_parser.match(cell1) and numeric_parser.match(cell2):
        [eval_result, threshold_result] = check_value(cell1, cell2)
        return [eval_result, threshold_result]
    else:
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
                full_threshold = max(threshold_result, full_threshold)
                i = i + 1
            return [full_eval, full_threshold]
