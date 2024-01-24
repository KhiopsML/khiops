import os.path
import re
from test_dir_management import *

# Nom du fichier de comparaison
COMPARISON_LOG_FILE_NAME = COMPARISON_RESULTS_LOG

# Constantes de la section SUMMARY des fichiers de log des resultats de comparaison
SUMMARY_TITLE = "SUMMARY"
SUMMARY_WARNING_KEY = "warning(s)"
SUMMARY_ERROR_KEY = "error(s)"
SUMMARY_FATAL_ERROR_KEY = "FATAL ERROR"
SUMMARY_FILE_TYPES_KEY = "Problem file types: "
SUMMARY_NOTE_KEY = "Note: "
SUMMARY_PORTABILITY_KEY = "Portability: "


def print_message(log_file, message):
    print(message)
    write_message(log_file, message)


def write_message(log_file, message):
    # on encode en utf-8 en ignorant les erreurs pour eviter un erreur lors de l'encodage automatique
    log_file.write(message.encode("utf-8", "ignore").decode("utf-8") + "\n")


def read_file_lines(log_file, file_path):
    """Chargement en memoire des lignes d'un fichier
    Retourne la liste des fichier si ok, None sinon
    Ecrit un message dans le log en cas d'erreur
    """
    # lecture des lignes du fichier de reference
    try:
        with open(file_path, "r", errors="ignore") as file:
            file_lines = file.readlines()
    except BaseException as message:
        write_message(
            log_file,
            "Error : can't open file " + file_path + " (" + str(message) + ")",
        )
        file_lines = None
    return file_lines


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
    """Compare les fichiers de resultats de test et de reference 2 a 2
    et ecrit les resultats dans le fichier de log"""

    assert os.path.isdir(test)
    test_full_path = os.path.join(os.getcwd(), test)

    # Initialisation des stats de comparaison
    number_fatal_errors = 0
    number_errors = 0
    number_warnings = 0
    number_compared_files = 0
    missing_result_files = False
    number_errors_in_err_txt = 0
    number_errors_per_extension = {}
    number_errors_per_file = {}
    erroneous_ref_file_lines = {}
    erroneous_test_file_lines = {}
    erroneous_file_names = []
    message_extension = ""
    specific_message = ""
    portability_message = ""

    # Ouverture du fichier de log de comparaison
    log_file_path = os.path.join(test_full_path, COMPARISON_LOG_FILE_NAME)
    try:
        log_file = open(log_file_path, "w")
    except:
        print("error : unable to create log file " + log_file_path)
        return
    assert log_file is not None
    write_message(log_file, test + " comparison")

    # Information sur le contexte courant de comparaison des resultats
    current_context = get_current_results_ref_context()
    write_message(
        log_file,
        "current comparison context : " + str(current_context),
    )

    # Test de presence du repertoire de test a comparer
    test_dir = os.path.join(test_full_path, RESULTS)
    if not os.path.isdir(test_dir):
        print_message(
            log_file,
            "error : no comparison, test directory not available (" + test_dir + ")",
        )
        number_errors = number_errors + 1

    # Recherche du repertoire courant des resultats de reference
    results_ref, candidate_dirs = get_results_ref_dir(
        test_full_path, log_file=log_file, show=True
    )
    if results_ref is None:
        print_message(
            log_file,
            "error : invalid " + RESULTS_REF + " dirs " + str(candidate_dirs),
        )
        number_errors = number_errors + 1
    elif len(candidate_dirs) >= 2:
        portability_message = (
            "used " + results_ref + " dir among " + str(candidate_dirs)
        )
        print_message(
            log_file,
            portability_message,
        )

    # Test de presence du repertoire de reference a comparer
    if number_errors == 0:
        ref_dir = os.path.join(test_full_path, results_ref)
        if not os.path.isdir(ref_dir):
            print_message(
                log_file,
                "error : no comparison, reference directory not available ("
                + ref_dir
                + ")",
            )
            number_errors = number_errors + 1

    # Comparaison effective si possible
    if number_errors == 0:
        # Initialisation des parsers
        initialize_parsers()

        # Acces aux fichiers des repertoire de reference et de test
        # On les tri pour ameliorer la reproductibilite inter plateformes
        ref_file_names = os.listdir(ref_dir)
        ref_file_names.sort()
        test_file_names = os.listdir(test_dir)
        test_file_names.sort()

        # Comparaison des nombres de fichiers
        ref_result_file_number = len(ref_file_names)
        test_result_file_number = len(test_file_names)
        if ref_result_file_number == 0:
            print_message(
                log_file, "error : no comparison, missing reference result files"
            )
            number_errors = number_errors + 1
        elif ref_result_file_number != test_result_file_number:
            print_message(
                log_file,
                "\nerror : number of results files ("
                + str(test_result_file_number)
                + ") should be "
                + str(ref_result_file_number),
            )
            number_errors = number_errors + 1
            missing_result_files = test_result_file_number < ref_result_file_number
            # Affichage des nom des fichier supplementaires
            max_file_reported = 20
            if test_result_file_number > ref_result_file_number:
                write_message(log_file, "Additional files in " + RESULTS + " dir:")
                file_reported = 0
                for file_name in test_file_names:
                    if file_name not in ref_file_names:
                        if file_reported < max_file_reported:
                            write_message(log_file, "\t" + file_name)
                        else:
                            write_message(log_file, "\t...")
                            break
                        file_reported += 1
            elif test_result_file_number < ref_result_file_number:
                write_message(log_file, "Missing files in " + RESULTS + " dir:")
                file_reported = 0
                for file_name in ref_file_names:
                    if file_name not in test_file_names:
                        if file_reported < max_file_reported:
                            write_message(log_file, "\t" + file_name)
                        else:
                            write_message(log_file, "\t...")
                            break
                        file_reported += 1

        # Comparaison des fichiers 2 a 2 en memorisant les erreurs par extension
        for file_name in ref_file_names:
            number_compared_files = number_compared_files + 1

            # En-tete de comparaison des fichiers
            ref_file_path = os.path.join(ref_dir, file_name)
            test_file_path = os.path.join(test_dir, file_name)
            write_message(log_file, "\nfile " + test_file_path)

            # Lecture des fichiers
            ref_file_lines = read_file_lines(log_file, ref_file_path)
            test_file_lines = read_file_lines(log_file, test_file_path)
            if ref_file_lines is None:
                number_errors = number_errors + 1
            if test_file_lines is None:
                number_errors = number_errors + 1
            # comparaison si ok
            if ref_file_lines is not None and test_file_lines is not None:
                errors, warnings = check_file(
                    log_file,
                    ref_file_path,
                    test_file_path,
                    ref_file_lines,
                    test_file_lines,
                )
                number_errors = number_errors + errors
                number_warnings = number_warnings + warnings

                # Memorisation des statistiques par extension
                if errors > 0:
                    erroneous_file_names.append(file_name)
                    number_errors_per_file[file_name] = errors
                    erroneous_ref_file_lines[file_name] = ref_file_lines
                    erroneous_test_file_lines[file_name] = test_file_lines
                    if file_name == ERR_TXT:
                        number_errors_in_err_txt += errors
                    else:
                        _, file_extension = os.path.splitext(file_name)
                        number_errors_per_extension[file_extension] = (
                            number_errors_per_extension.get(file_extension, 0) + errors
                        )
        # Recherche des erreurs fatales, avec tentative de recuperation
        # On accepte les erreurs fatales que si on les meme en test et reference,
        # et uniquement dans le cas du pattern particulier du "Batch mode failure" qui est du
        # a des scenario n'ayant pas pu s'excuter entierement pour des raison de portabilite
        fatal_error_recovery = True
        STDERR_ERROR_LOG_RECOVERY_PATTERN = (
            "fatal error : Command file : Batch mode failure"
        )
        RETURN_CODE_ERROR_LOG_RECOVERY_PATTERN = (
            "Wrong return code: 1 (should be 0 or 2)"
        )
        for file_name in test_file_names:
            if file_name in FATAL_ERROR_FILES:
                write_message(log_file, "\nfatal error : found file " + file_name)
                number_fatal_errors = number_fatal_errors + 1

                # La tentative de recuperation des erreurs fatales echoue si on ne respecte
                # pas toutes les conditions necessaires
                if file_name not in [STDERR_ERROR_LOG, RETURN_CODE_ERROR_LOG]:
                    fatal_error_recovery = False
                else:
                    # Les fichiers doivent etre les memes
                    if (
                        file_name in erroneous_file_names
                        or file_name not in ref_file_names
                    ):
                        fatal_error_recovery = False
                    # Test que le fichier est reduit au pattern accepte
                    if not fatal_error_recovery:
                        # Lecture des lignes du fichier
                        test_file_path = os.path.join(test_dir, file_name)
                        test_file_lines = read_file_lines(log_file, test_file_path)
                        # Pattern dans le cas de sdterr
                        if file_name == STDERR_ERROR_LOG:
                            if (
                                len(test_file_lines) == 0
                                or test_file_lines[0].strip()
                                != STDERR_ERROR_LOG_RECOVERY_PATTERN
                            ):
                                fatal_error_recovery = False
                        # Pattern dans le cas du code retour
                        if file_name == RETURN_CODE_ERROR_LOG:
                            if (
                                len(test_file_lines) == 0
                                or test_file_lines[0].strip()
                                != RETURN_CODE_ERROR_LOG_RECOVERY_PATTERN
                            ):
                                fatal_error_recovery = False
        # Message de recuperation si necessaire
        if number_fatal_errors > 0 and fatal_error_recovery:
            number_fatal_errors = 0
            write_message(
                log_file,
                "recovery from fatal errors caused solely by a 'Batch mode failure' in another platform",
            )
            if portability_message != "":
                portability_message += ", "
            portability_message += "recovery of type 'Batch mode failure'"
        # Ecriture des permieres lignes des fichiers d'erreur fatales si necessaire
        number_errors += number_fatal_errors
        if number_fatal_errors > 0:
            for file_name in test_file_names:
                if file_name in FATAL_ERROR_FILES:
                    # Lecture des lignes du fichier
                    test_file_path = os.path.join(test_dir, file_name)
                    test_file_lines = read_file_lines(log_file, test_file_path)
                    write_message(log_file, "fatal error file " + test_file_path)
                    max_print_lines = 10
                    for i, line in enumerate(test_file_lines):
                        if i < max_print_lines:
                            write_message(log_file, "\t" + line.rstrip())
                        else:
                            write_message(log_file, "\t...")
                            break

    # DDD
    # print(str(erroneous_file_names))
    # for file_name in erroneous_file_names:
    #     print(
    #         "errors in file "
    #         + file_name
    #         + ": "
    #         + str(len(erroneous_ref_file_lines[file_name]))
    #         + ": "
    #         + str(len(erroneous_test_file_lines[file_name]))
    #     )

    # Write summary
    write_message(log_file, "\n" + SUMMARY_TITLE)
    write_message(log_file, str(number_warnings) + " " + SUMMARY_WARNING_KEY)
    write_message(log_file, str(number_errors) + " " + SUMMARY_ERROR_KEY)
    if number_fatal_errors > 0:
        write_message(log_file, SUMMARY_FATAL_ERROR_KEY)
    if number_errors > 0:
        # Sort file extensions
        file_extensions = []
        for file_extension in number_errors_per_extension:
            file_extensions.append(file_extension)
        file_extensions.sort()
        # Build messages
        if number_errors_in_err_txt > 0:
            message_extension += ERR_TXT
            if len(file_extensions) > 0:
                message_extension += ", "
            if number_errors_in_err_txt == number_errors:
                specific_message = "errors only in " + ERR_TXT
        if len(file_extensions) > 0:
            for i, file_extension in enumerate(file_extensions):
                if i > 0:
                    message_extension += ", "
                message_extension += file_extension
                if number_errors_per_extension[file_extension] == number_errors:
                    specific_message = "errors only in " + file_extension + " files"
        # Build specific message if number or errors only in " + ERR_TXT + " report files
        if specific_message == "":
            number_errors_in_report_files = number_errors_per_extension.get(
                ".khj", 0
            ) + number_errors_per_extension.get(".khcj", 0)
            if (
                number_errors_in_err_txt == number_errors_in_report_files
                and number_errors_in_err_txt + number_errors_in_report_files
                == number_errors
            ):
                specific_message = (
                    "all errors in "
                    + ERR_TXT
                    + " and in json report files with the same number"
                )
        # Build specific message in case of missing files
        if specific_message == "" and missing_result_files:
            specific_message = "Missing result files"
        # Write additional messages
        if message_extension != "":
            write_message(log_file, SUMMARY_FILE_TYPES_KEY + message_extension)
        if specific_message != "":
            write_message(log_file, SUMMARY_NOTE_KEY + specific_message)

    # Write additional info related to portability
    if portability_message != "":
        write_message(log_file, SUMMARY_PORTABILITY_KEY + portability_message)

    # Affichage d'un message de fin sur la console
    print(
        "--Comparison done : "
        + str(number_compared_files)
        + " files(s) compared, "
        + str(number_errors)
        + " error(s), "
        + str(number_warnings)
        + " warning(s)"
        + (", FATAL ERROR" if number_fatal_errors > 0 else "")
    )
    print("log writed in " + log_file_path + "\n")


def check_file(
    log_file, ref_file_path: str, test_file_path, ref_file_lines, test_file_lines
):
    """Comparaison d'un fichier de test et d'un fihcier de reference
    Parametres:
    - log file: fichier de log ouvert dans le quel des messages sont ecrits
    - ref_file_path: chemin du fichier de refence
    - test_file_path: chemin du fichier de test
    - ref_file_lines: liste des lignes du fichier de reference
    - test_file_lines: liste des lignes du fichier de test

    Retourne
    - error: nombre d'erreurs
    - warning: nombre de warning

    Les listes de lignes en entree permettent d'eviter de relire un fichier dont on connait le nom
    et dont on a deja lu les lignes.
    Cela permet par exemple de reutiliser les methodes de comparaison apres avoir filtre le fichier
    de sous-parties que l'on ne souhaite pas comparer.

    Compare les fichiers ligne par ligne, cellule par cellule, avec des tolerances selon le type de fichier
    pour les valeurs numeriques, une diffence relative de 0.00001 est toleree
    - ecrit les difference dans le fichier log_file et affiche le nb d'erreur dans le terminal
    - warning : 2 cellules contiennent des valeurs numeriques avec une difference relative toleree
    - error : les cellules sont differentes
    """

    def is_specific_line_pair_sequential(line1, line2):
        """Test si une paire de ligne correspond a un pattern de message sequentiel
        Premiere ligne avec 100th, 1000th error ou warning
        Seconde ligne avec '...'
        """
        message_type = ""
        if line1.find("warning : ") == 0:
            message_type = "warning"
        elif line1.find("error : ") == 0:
            message_type = "error"
        is_specific = message_type != ""
        # La premiere ligne doit se terminer par un pattern de type '(100th warning)'
        if is_specific:
            line1 = line1.strip()
            expected_end_line1 = "00th " + message_type + ")"
            is_specific = (
                line1[len(line1) - len(expected_end_line1) :] == expected_end_line1
            )
        # La seconde ligne doit se terminer par ' : ...'
        if is_specific:
            is_specific = line2.find(message_type) == 0
            if is_specific:
                line2 = line2.strip()
                expected_end_line2 = " : ..."
                is_specific = (
                    line2[len(line2) - len(expected_end_line2) :] == expected_end_line2
                )
        return is_specific

    def filter_sequential_messages_in_err_file(log_file, file_lines):
        """Filtrage des errors et warning sequentiel d'un fichier de log d'erreur
        En effet, en sequentiel, de nouveaux message de type 100th ou ...
        sont emis, alors qu'il sont absent en parallele
        En les filtrant, on rend les version sequentielle et parallele comparable
        Retourne les ligne filtrees, avec un message dans le log sur le nombre de lignes filtrees
        """
        output_lines = []
        filtered_line_number = 0
        # Filtrage des lignes
        i = 0
        line_number = len(file_lines)
        while i < line_number:
            line = file_lines[i]
            if i == line_number - 1:
                output_lines.append(line)
            else:
                next_line = file_lines[i + 1]
                # On saute deux lignes si elles sont specifique a des message en sequentiel
                if is_specific_line_pair_sequential(line, next_line):
                    i += 1
                    filtered_line_number += 2
                else:
                    output_lines.append(line)
            i += 1
        # Message si lignes filtrees
        if filtered_line_number > 0:
            write_message(
                log_file,
                "Specific sequential messages (100th...): "
                + str(filtered_line_number)
                + " lines filtered",
            )
        return output_lines

    def filter_sequential_messages_in_json_file(log_file, file_lines):
        """Filtrage des errors et warning sequentiel d'un fichier json, pour les
        message emis dans la section "messages"
        En effet, en sequentiel, de nouveaux message de type 100th ou ...
        sont emis, alors qu'il sont absent en parallele
        En les filtrant, on rend les version sequentielle et parallele comparable
        Retourne les ligne filtrees, avec un message dans le log sur le nombre de lignes filtrees
        De plus, dans la section message, ont nettoie les lignes de leur '"' et potentiel ',' de fin
        pour qu'elles aient la meme forme que dans les log

        On ne recherche ici les lignes que dans les section "messages" du json
        """

        def clean_message_line(line):
            """Nettoyage d'une ligne de message, entre '"' et potentiellement suivi d'une ','
            Cela ne gere pas tous les cas d'encodage json, mais cela est suffisant la plupart du temps
            """
            cleaned_line = line.strip()
            if cleaned_line[-1] == ",":
                cleaned_line = cleaned_line[1:-2]
            else:
                cleaned_line = cleaned_line[1:-1]
            return cleaned_line

        # Recherche des ligne du fichier dans les sections "messages"
        in_message_section = False
        output_lines = []
        filtered_line_number = 0
        # Filtrage des lignes
        i = 0
        line_number = len(file_lines)
        while i < line_number:
            line = file_lines[i]
            # Test si on sort d'une section a traiter
            if in_message_section:
                in_message_section = line.strip() != "]"
            # Analyse des lignes
            if i == line_number - 1 or not in_message_section:
                output_lines.append(line)
            else:
                next_line = file_lines[i + 1]
                # Nettoyage des lignes dans la section message
                line = clean_message_line(line)
                next_line = clean_message_line(next_line)
                # On saute deux lignes si elles sont specifique a des message en sequentiel
                # Les lignes sont préalablement nettoyees de leur '"'
                if is_specific_line_pair_sequential(line, next_line):
                    i += 1
                    filtered_line_number += 2
                else:
                    # Dans la section message, on
                    output_lines.append(line)
            # Test si on est entre dans une section a traiter
            if not in_message_section:
                in_message_section = line.strip() == '"messages": ['
            i += 1
        # Message si lignes filtrees
        if filtered_line_number > 0:
            write_message(
                log_file,
                "Specific sequential messages (100th...): "
                + str(filtered_line_number)
                + " lines filtered",
            )
        return output_lines

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
        if pos_start1 >= 0 and pos_start2 > pos_start1:
            filtered_value = value[:pos_start1] + " after reading ..."
        else:
            filtered_value = value
        return filtered_value

    def filter_secondary_table_stats(value):
        # Supression d'un pattern de nombre de records secondaires
        pos_start1 = value.find("Table ")
        pos_start2 = value.find(" Records: ")
        if pos_start1 >= 0 and pos_start2 > pos_start1:
            filtered_value = value[:pos_start2] + " Records: "
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

    # Verifications
    assert ref_file_path != "", "Missing ref file path"
    assert test_file_path != "", "Missing test file path"
    assert ref_file_lines is not None, "Missing ref file lines"
    assert test_file_lines is not None, "Missing test file lines"

    # Recherche du fichier compare et de son extension
    file_name = os.path.basename(ref_file_path)
    assert file_name == os.path.basename(test_file_path)
    _, file_extension = os.path.splitext(file_name)

    # test si fichier de temps
    is_time_file = file_name == TIME_LOG

    # test si fichier histogramme
    is_histogram_file = "histogram" in file_name and file_extension == ".log"

    # test si fichier d'erreur
    is_error_file = file_name == ERR_TXT

    # test si fichier de benchmark
    is_benchmark_file = file_name == "benchmark.xls"

    # test si fichier dictionnaire
    is_kdic_file = file_extension == ".kdic"

    # Test si fichier json
    json_file_extensions = [".json", ".khj", ".khvj", ".khcj", ".kdicj"]
    is_json_file = file_extension in json_file_extensions
    # Cas particulier des fichier .bad qui sont en fait des fichiers json
    # (ex: LearningTest\TestKhiops\Advanced\AllResultsApiMode)
    if file_extension == ".bad":
        if (
            os.path.isfile(ref_file_path.replace(".bad", ".khj"))
            or os.path.isfile(ref_file_path.replace(".bad", ".khj"))
            or os.path.isfile(ref_file_path.replace(".bad", ".kdicj"))
        ):
            is_json_file = True

    # initialisation des nombres d'erreurs
    error = 0
    warning = 0

    # Pas de controle si fichier de temps
    if is_time_file:
        write_message(log_file, "OK")
        return error, warning

    # Filtrage des messages specifiq au sequentiel (100th...)
    # pour ameliorer la reproductibilite de la comparaison entre sequentiel et parallele
    if is_error_file:
        ref_file_lines = filter_sequential_messages_in_err_file(
            log_file, ref_file_lines
        )
        test_file_lines = filter_sequential_messages_in_err_file(
            log_file, test_file_lines
        )
    elif is_json_file:
        ref_file_lines = filter_sequential_messages_in_json_file(
            log_file, ref_file_lines
        )
        test_file_lines = filter_sequential_messages_in_json_file(
            log_file, test_file_lines
        )

    # Comparaison des nombres de lignes
    file_ref_line_number = len(ref_file_lines)
    file_test_line_number = len(test_file_lines)
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
    filter_secondary_record_detected = False
    line_number = min(file_ref_line_number, file_test_line_number)
    for index in range(line_number):
        line = index + 1
        line_ref = ref_file_lines[index].rstrip()
        line_test = test_file_lines[index].rstrip()

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

            # cas special du fichier d'erreur ou khj
            # on tronque le compte des lignes avec des warning sur le nombre de records secondaires
            if (
                (is_error_file or is_json_file)
                and "warning" in field_ref
                and " after reading " in field_ref
                and " secondary records " in field_ref
                and "warning" in field_test
                and " after reading " in field_test
                and " secondary records " in field_test
            ):
                filter_secondary_record_detected = True
                field_ref = filter_secondary_record(field_ref)
                field_test = filter_secondary_record(field_test)

            # Cas particulier du nombre de record secondaire draporte dans le fichier d'erreur,
            # si des enregistrement secondaire ont ete detectes
            if (
                is_error_file
                and filter_secondary_record_detected
                and "Table " in field_ref
                and " Records: " in field_ref
                and "Table " in field_test
                and " Records: " in field_test
            ):
                field_ref = filter_secondary_table_stats(field_ref)
                field_test = filter_secondary_table_stats(field_test)

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
    if error > 0:
        message = str(error) + " error(s)"
        if max_threshold > 0:
            message += " (max relative difference: " + str(max_threshold) + ")"
        write_message(log_file, message)

    return error, warning


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
    # - threshold: difference relative si result = 2
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
    #   - 2 si les la difference relative est toleree (warning)
    #   - 0 si les cellules sont differentes (error)
    # - threshold: difference relative liee au cas erreur ou warning

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
            warnings = 0
            errors = 0
            max_warning_threshold = 0
            max_error_threshold = 0
            while i < length:
                [eval_result, threshold_result] = check_value(
                    substrings1[i], substrings2[i]
                )
                # Traitement des erreurs
                if eval_result == 0:
                    errors += 1
                    max_error_threshold = max(threshold_result, max_error_threshold)
                # Traitement des warnings
                if eval_result == 2:
                    warnings += 1
                    max_warning_threshold = max(threshold_result, max_warning_threshold)
                i = i + 1
            if errors > 0:
                return [0, max_error_threshold]
            elif warnings > 0:
                return [2, max_warning_threshold]
            else:
                return [1, 0]
