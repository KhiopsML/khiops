import os.path
import re

from test_dir_management import *

# Nom du fichier de comparaison
COMPARISON_LOG_FILE_NAME = COMPARISON_RESULTS_LOG

# Constantes de la section SUMMARY des fichiers de log des resultats de comparaison
SUMMARY_TITLE = "SUMMARY"
SUMMARY_WARNING_KEY = "warning(s)"
SUMMARY_ERROR_KEY = "error(s)"
SUMMARY_FILE_TYPES_KEY = "Problem file types: "
SUMMARY_NOTE_KEY = "Note: "
SUMMARY_PORTABILITY_KEY = "Portability: "

# Constantes pour la gestion des fichiers speciaux, par priorite decroissante
SUMMARY_TIMOUT_ERROR_KEY = "TIMOUT ERROR"
SUMMARY_FATAL_ERROR_KEY = "FATAL ERROR"
SUMMARY_UNEXPECTED_OUTPUT_KEY = "UNEXPECTED OUTPUT"
SUMMARY_SPECIAL_FILE_KEYS = [
    SUMMARY_TIMOUT_ERROR_KEY,
    SUMMARY_FATAL_ERROR_KEY,
    SUMMARY_UNEXPECTED_OUTPUT_KEY,
]

# Association entre type de fichier special et cle de gestion dans le resume
SUMMARY_SPECIAL_FILE_KEYS_PER_FILE = {}
SUMMARY_SPECIAL_FILE_KEYS_PER_FILE[STDOUT_ERROR_LOG] = SUMMARY_UNEXPECTED_OUTPUT_KEY
SUMMARY_SPECIAL_FILE_KEYS_PER_FILE[STDERR_ERROR_LOG] = SUMMARY_UNEXPECTED_OUTPUT_KEY
SUMMARY_SPECIAL_FILE_KEYS_PER_FILE[PROCESS_TIMEOUT_ERROR_LOG] = SUMMARY_TIMOUT_ERROR_KEY
SUMMARY_SPECIAL_FILE_KEYS_PER_FILE[RETURN_CODE_ERROR_LOG] = SUMMARY_FATAL_ERROR_KEY
assert len(SUMMARY_SPECIAL_FILE_KEYS_PER_FILE) == len(SPECIAL_ERROR_FILES)


def write_message(message, log_file=None, show=False):
    """Ecriture d'un message dans un fichier de log
    Ecriture dans un fichier de log selon le le parametre log_file
    Affichage sur la console selon le parametre show
    Si ni log_file, ni show ne sont specifier, la methode est en mode silencieux
    """
    cleaned_message = message.encode("utf-8", "ignore").decode("utf-8")
    if show:
        print(cleaned_message)
    # on encode en utf-8 en ignorant les erreurs pour eviter un erreur lors de l'encodage automatique
    if log_file is not None:
        log_file.write(cleaned_message + "\n")


def read_file_lines(file_path, log_file=None, show=False):
    """Chargement en memoire des lignes d'un fichier
    Retourne la liste des fichier si ok, None sinon
    Ecrit un message dans le log en cas d'erreur
    """
    # lecture des lignes du fichier
    try:
        with open(file_path, "r", errors="ignore") as file:
            file_lines = file.readlines()
    except BaseException as exception:
        write_message(
            "Error : can't open file " + file_path + " (" + str(exception) + ")",
            log_file=log_file,
            show=show,
        )
        file_lines = None
    return file_lines


def write_file_lines(
    file_path, file_lines, striped_lines_suffix="\n", log_file=None, show=False
):
    """Ecriture d'une liste de ligne dans un fichier
    Ajoute un suffix aux lignes sans caractere fin de ligne
    Ecrit un message dans le log en cas d'erreur
    """
    # lecture des lignes du fichier
    try:
        with open(file_path, "w", errors="ignore") as file:
            for line in file_lines:
                file.write(line)
                if len(line) == 0 or line[-1] != "\n":
                    file.write(striped_lines_suffix)
    except BaseException as exception:
        write_message(
            "Error : can't open output file " + file_path + " (" + str(exception) + ")",
            log_file=log_file,
            show=show,
        )


def find_in_lines(lines, substring):
    """Recherche d'une chaine de caractere parmi un ensemble de lignes
    Renvoie l'index de la premiere ligne dans laquelle la chaine apparait, -1 sinon"""
    for i, line in enumerate(lines):
        if line.find(substring) >= 0:
            return i
    return -1


def append_message(initial_messages, message):
    """Ajout d'un message a un message existant, en ajoutant si necessaire ', '
     pour separer les messages si les deux sont non vides
    Retourne un message complete du nouveau message"""
    if message == "":
        return initial_messages
    elif initial_messages == "":
        return message
    else:
        return initial_messages + ", " + message


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
    special_error_file_error_numbers = {}
    for file_name in SPECIAL_ERROR_FILES:
        special_error_file_error_numbers[file_name] = 0
    error_number = 0
    warnings_number = 0
    compared_files_number = 0
    error_number_in_err_txt = 0
    error_number_per_extension = {}
    error_number_per_file = {}
    erroneous_ref_file_lines = {}
    erroneous_test_file_lines = {}
    erroneous_file_names = []
    extension_message = ""
    recovery_message = ""
    specific_message = ""
    portability_message = ""

    # Ouverture du fichier de log de comparaison
    log_file_path = os.path.join(test_full_path, COMPARISON_LOG_FILE_NAME)
    try:
        log_file = open(log_file_path, "w")
    except Exception as exception:
        print("error : unable to create log file " + log_file_path, exception)
        return
    assert log_file is not None
    write_message(test + " comparison", log_file=log_file)

    # Information sur le contexte courant de comparaison des resultats
    current_context = get_current_results_ref_context()
    write_message(
        "current comparison context : " + str(current_context),
        log_file=log_file,
    )

    # Test de presence du repertoire de test a comparer
    test_dir = os.path.join(test_full_path, RESULTS)
    if not os.path.isdir(test_dir):
        write_message(
            "error : no comparison, test directory not available (" + test_dir + ")",
            log_file=log_file,
            show=True,
        )
        error_number = error_number + 1

    # Recherche du repertoire courant des resultats de reference
    results_ref, candidate_dirs = get_results_ref_dir(
        test_full_path, log_file=log_file, show=True
    )
    if results_ref is None:
        write_message(
            "error : invalid " + RESULTS_REF + " dirs " + str(candidate_dirs),
            log_file=log_file,
            show=True,
        )
        error_number = error_number + 1
    elif len(candidate_dirs) >= 2:
        portability_message = (
            "used " + results_ref + " dir among " + str(candidate_dirs)
        )
        write_message(
            portability_message,
            log_file=log_file,
            show=True,
        )

    # Test de presence du repertoire de reference a comparer
    if error_number == 0:
        ref_dir = os.path.join(test_full_path, results_ref)
        if not os.path.isdir(ref_dir):
            write_message(
                "error : no comparison, reference directory not available ("
                + ref_dir
                + ")",
                log_file=log_file,
                show=True,
            )
            error_number = error_number + 1

    # Comparaison effective si possible
    if error_number == 0:
        # Initialisation des parsers
        initialize_parsers()

        # Acces aux fichiers des repertoire de reference et de test
        # On passe par le format bytes des nom de fichier pour avoir acces
        # aux fichier quelque soit la plateforme
        # - Windows ne supporte que l'utf8
        # - Linux stocke les nom directement sous la forme de bytes
        ref_byte_file_names = os.listdir(os.fsencode(ref_dir))
        test_byte_file_names = os.listdir(os.fsencode(test_dir))

        # On memorise les noms de fichiers sous forme de string pour faciliter le reporting
        # Tout en gardant l'association entre le nom python (utf8) et les nom en bytes
        #
        # Attention, la methode fsdecode utilise des 'surrogate characters' invisible
        # permettant de garder trace des bytes non utf8 pour le re-encodage par fsencode si necessaire
        # On passe par une version 'nettoyee' de ces caracteres speciaux pour memoriser
        # l'association entre un nom de fichier de type string et un nom de type bytes
        # Dans ce cas, il suffit de memoriser dans les resultats de reference la
        # version du nom de fichier sans bytes (valide quelque soit la plateforme)
        # Pour les resultats de test, le nom peut comporter des bytes, mais on tolere
        # la comparaison si sa version nettoyee est la meme que pour le fichier de reference
        ref_file_names = []
        dic_ref_byte_file_names = {}
        recovery = False
        for byte_file_name in ref_byte_file_names:
            file_name = os.fsdecode(byte_file_name)
            cleaned_file_name = file_name.encode("utf-8", "ignore").decode("utf-8")
            if cleaned_file_name != file_name:
                write_message(
                    "warning : reference file name with a byte encoding ("
                    + str(byte_file_name)
                    + ") used under utf8 name ("
                    + cleaned_file_name
                    + ")"
                )
                warnings_number += 1
                recovery = True
            ref_file_names.append(cleaned_file_name)
            dic_ref_byte_file_names[cleaned_file_name] = byte_file_name
        # Idem pour les resultat de test
        test_file_names = []
        dic_test_byte_file_names = {}
        for byte_file_name in test_byte_file_names:
            file_name = os.fsdecode(byte_file_name)
            cleaned_file_name = file_name.encode("utf-8", "ignore").decode("utf-8")
            if cleaned_file_name != file_name:
                write_message(
                    "warning : test file name with a byte encoding ("
                    + str(byte_file_name)
                    + ") used under utf8 name ("
                    + cleaned_file_name
                    + ")"
                )
                warnings_number += 1
                recovery = True
            test_file_names.append(cleaned_file_name)
            dic_test_byte_file_names[cleaned_file_name] = byte_file_name

        # Message de recuperation d'erreur si necessaire
        if recovery:
            write_message(
                "\nRecovery from errors caused byte encoding of file names in another platform",
                log_file=log_file,
            )
            portability_message = append_message(
                portability_message, "recovery of type byte enencoding of file names"
            )

        # On les tri pour ameliorer la statbilite du reporting inter plateformes
        ref_file_names.sort()
        test_file_names.sort()

        # Comparaison des nombres de fichiers
        ref_result_file_number = len(ref_file_names)
        test_result_file_number = len(test_file_names)
        if ref_result_file_number == 0:
            write_message(
                "error : no comparison, missing reference result files",
                log_file=log_file,
                show=True,
            )
            error_number = error_number + 1
        elif ref_result_file_number != test_result_file_number:
            write_message(
                "\nerror : number of results files ("
                + str(test_result_file_number)
                + ") should be "
                + str(ref_result_file_number),
                log_file=log_file,
                show=True,
            )
            error_number = error_number + 1
            # Affichage des nom des fichier supplementaires
            max_file_reported = 20
            if test_result_file_number > ref_result_file_number:
                # Message specifique en cas de fichiers en trop
                specific_message = append_message(
                    specific_message, "additional result files"
                )
                write_message(
                    "Additional files in " + RESULTS + " dir:", log_file=log_file
                )
                file_reported = 0
                for file_name in test_file_names:
                    if file_name not in ref_file_names:
                        if file_reported < max_file_reported:
                            write_message("\t" + file_name, log_file=log_file)
                        else:
                            write_message("\t...", log_file=log_file)
                            break
                        file_reported += 1
            elif test_result_file_number < ref_result_file_number:
                # Message specifique en cas de fichiers manquants
                specific_message = append_message(
                    specific_message, "missing result files"
                )
                write_message(
                    "Missing files in " + RESULTS + " dir:", log_file=log_file
                )
                file_reported = 0
                for file_name in ref_file_names:
                    if file_name not in test_file_names:
                        if file_reported < max_file_reported:
                            write_message("\t" + file_name, log_file=log_file)
                        else:
                            write_message("\t...", log_file=log_file)
                            break
                        file_reported += 1

        # Comparaison des fichiers 2 a 2 en memorisant les erreurs par extension
        for file_name in ref_file_names:
            compared_files_number = compared_files_number + 1

            # Path des fichier utilises pour le reporting
            ref_file_path = os.path.join(ref_dir, file_name)
            test_file_path = os.path.join(test_dir, file_name)

            # En-tete de comparaison des fichiers
            write_message("\nfile " + test_file_path, log_file=log_file)

            # On utilise si possible le path des fichiers en byte pour s'adapter aux contrainte de la plateforme
            # Les erreurs seront diagnostiquees si necessaire lors de la lecture des fichiers
            used_ref_file_path = ref_file_path
            if dic_ref_byte_file_names.get(file_name) is not None:
                used_ref_file_path = os.path.join(
                    os.fsencode(ref_dir), dic_ref_byte_file_names.get(file_name)
                )
            used_test_file_path = test_file_path
            if dic_test_byte_file_names.get(file_name) is not None:
                used_test_file_path = os.path.join(
                    os.fsencode(test_dir), dic_test_byte_file_names.get(file_name)
                )

            # Lecture des fichiers
            ref_file_lines = read_file_lines(used_ref_file_path, log_file=log_file)
            test_file_lines = read_file_lines(used_test_file_path, log_file=log_file)
            if ref_file_lines is None:
                error_number = error_number + 1
            if test_file_lines is None:
                error_number = error_number + 1

            # Comparaison si ok
            if ref_file_lines is not None and test_file_lines is not None:
                # Mise en forme specifique des message utilisateur (error, warning) pour les traiter des facon identique
                # dans les cas des fichiers de log utilisateur et json
                contains_user_messages = False
                # Cas du fichier de log utilisateur
                if file_name == ERR_TXT:
                    contains_user_messages = True
                    # Identification des lignes de message
                    ref_file_lines = strip_user_message_lines(ref_file_lines)
                    test_file_lines = strip_user_message_lines(test_file_lines)
                # Cas des fichier json (il faut passer le path en entier pour gerer certaines exceptions)
                elif is_file_with_json_extension(ref_file_path):
                    contains_user_messages = True
                    # Pretraitement des ligne de message pour les mettre dans le meme format que pour les fichier d'erreur
                    ref_file_lines = strip_user_message_lines_in_json_file(
                        ref_file_lines
                    )
                    test_file_lines = strip_user_message_lines_in_json_file(
                        test_file_lines
                    )

                # Filtrage des messages specifiques au sequentiel (100th...)
                if contains_user_messages:
                    ref_file_lines = filter_sequential_messages_lines(
                        ref_file_lines, log_file=log_file
                    )
                    test_file_lines = filter_sequential_messages_lines(
                        test_file_lines, log_file=log_file
                    )

                # Comparaison des fichiers pre-traites
                errors, warnings = check_file_lines(
                    ref_file_path,
                    test_file_path,
                    ref_file_lines,
                    test_file_lines,
                    log_file=log_file,
                )
                error_number += errors
                warnings_number += warnings

                # Memorisation des statistiques par extension
                if errors > 0:
                    erroneous_file_names.append(file_name)
                    error_number_per_file[file_name] = errors
                    erroneous_ref_file_lines[file_name] = ref_file_lines
                    erroneous_test_file_lines[file_name] = test_file_lines
                    if file_name == ERR_TXT:
                        error_number_in_err_txt += errors
                    else:
                        _, file_extension = os.path.splitext(file_name)
                        error_number_per_extension[file_extension] = (
                            error_number_per_extension.get(file_extension, 0) + errors
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
            # Cas d'une erreur fatale
            if file_name in SPECIAL_ERROR_FILES:
                special_error_file_error_numbers[file_name] = (
                    special_error_file_error_numbers[file_name] + 1
                )
                error_number += 1
                special_error = SUMMARY_SPECIAL_FILE_KEYS_PER_FILE[file_name].lower()
                write_message(
                    "\n" + special_error + " : found file " + file_name,
                    log_file=log_file,
                )

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
                        test_file_lines = read_file_lines(
                            test_file_path, log_file=log_file
                        )
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
        if special_error_file_error_numbers[RETURN_CODE_ERROR_LOG] > 0:
            # Cas de la recuperation
            if fatal_error_recovery:
                error_number -= special_error_file_error_numbers[RETURN_CODE_ERROR_LOG]
                error_number -= special_error_file_error_numbers[STDERR_ERROR_LOG]
                special_error_file_error_numbers[RETURN_CODE_ERROR_LOG] = 0
                special_error_file_error_numbers[STDERR_ERROR_LOG] = 0
                write_message(
                    "\nRecovery from fatal errors caused solely by a 'Batch mode failure' in another platform",
                    log_file=log_file,
                )
                portability_message = append_message(
                    portability_message, "recovery of type 'Batch mode failure'"
                )

        # Ecriture des premieres lignes des fichiers d'erreur fatales ou de timeout si necessaire
        for file_name in test_file_names:
            if (
                file_name in SPECIAL_ERROR_FILES
                and special_error_file_error_numbers[file_name] > 0
            ):
                # Lecture des lignes du fichier
                test_file_path = os.path.join(test_dir, file_name)
                test_file_lines = read_file_lines(test_file_path, log_file=log_file)
                write_message(
                    "\nspecial error file " + test_file_path, log_file=log_file
                )
                max_print_lines = 10
                for i, line in enumerate(test_file_lines):
                    if i < max_print_lines:
                        write_message("\t" + line.rstrip(), log_file=log_file)
                    else:
                        write_message("\t...", log_file=log_file)
                        break

    # Il y a plusieurs tentatives de recuperation des erreurs pour des jeux de test ou des variation normales
    # sont possibles, comme par exemple des difference sur la caucl de l'auc en cas de manque de ressource
    # Ces tentatives sont implementees de facon pragmatique (code minimaliste, facile a developper et faire evoluer)
    # pour automatiser l'analyse manuelle des resultats qui ete effectuee auparavant
    # On ne cherche pas a ere resilient a tous les cas possibles, ni a gerer la complexite des types de recuperation
    # pouvant se combiner. Ces methodes de recuperation ne servent parfois que pour un seul jeu de donnees,
    # et il ne faut pas hesiter si besoin a simplifier certains jeux de test pour eviter qu'ils combinent
    # plusieurs problemes de recuperation

    # Tentative de recuperation des erreurs si la seule difference provient du fichier de log de Khiops
    # et est du a des warning en nombre variable en mode parallele, sans ecriture de rapport
    if error_number > 0:
        varying_warning_messages_in_err_txt_recovery = True

        # Les messages doivent n'apparaitre que dans le fichier de log
        if varying_warning_messages_in_err_txt_recovery:
            varying_warning_messages_in_err_txt_recovery = (
                error_number == error_number_in_err_txt
            )

        # Filtrage d'un certain type de warning pour recommencer la comaraison
        if varying_warning_messages_in_err_txt_recovery:
            # Acces aux lignes des fichier
            ref_file_lines = erroneous_ref_file_lines.get(ERR_TXT)
            test_file_lines = erroneous_test_file_lines.get(ERR_TXT)

            # Filtrage des lignes selon le motif en nombre variable
            warning_pattern1 = "warning : Data table slice "
            warning_pattern2 = " : Read data table slice interrupted by user"
            filtered_ref_file_lines = []
            for line in ref_file_lines:
                if line.find(warning_pattern1) != 0 or line.find(warning_pattern2) < 0:
                    filtered_ref_file_lines.append(line)
                filtered_test_file_lines = []
                for line in test_file_lines:
                    if (
                        line.find(warning_pattern1) != 0
                        or line.find(warning_pattern2) < 0
                    ):
                        filtered_test_file_lines.append(line)

            # Comparaison a nouveau des fichier, en mode non verbeux
            errors, warnings = check_file_lines(
                ERR_TXT,
                ERR_TXT,
                filtered_ref_file_lines,
                filtered_test_file_lines,
            )

            # Recuperation possible si plus d'erreur apres filtrage
            varying_warning_messages_in_err_txt_recovery = errors == 0

        # Recuperation effective des erreurs si possible
        if varying_warning_messages_in_err_txt_recovery:
            # Messages sur la recuperation
            recovery_summary = (
                "Recovery from varying warning number in " + ERR_TXT + " file only"
            )
            recovery_message = recovery_summary.lower()
            write_message("\n" + recovery_summary + ":", log_file=log_file)
            write_message(
                "\tall errors come from the warning in "
                + ERR_TXT
                + " file only, du to varying number of active process number",
                log_file=log_file,
            )
            write_message(
                "\t" + str(error_number) + " errors converted to warnings",
                log_file=log_file,
            )
            # On transforme les erreur en warning
            warnings_number += error_number
            error_number = 0
            # On reinitialise egalement les stats d'erreur pour les extensuon concernees
            error_number_in_err_txt = 0

    # Tentative de recuperation des erreurs si la seule difference est une difference d'ordre
    # des messages utilisateur (error ou warning)
    if error_number > 0:
        unsorted_user_messages_recovery = True

        # Verification de la repartition des nombres d'erreur
        if unsorted_user_messages_recovery:
            # Recherche du nombre d'erreur dans les rapport json
            error_number_in_json_report_files = error_number_per_extension.get(
                ".khj", 0
            ) + error_number_per_extension.get(".khcj", 0)

            # On test si le nombre total d'erreur se rapartit entre le fichier de log utilisateur
            # et les rapports json
            unsorted_user_messages_recovery = (
                error_number_in_err_txt == error_number_in_json_report_files
                and error_number_in_err_txt + error_number_in_json_report_files
                == error_number
            )

        # Analyse specifique de la sous partie des fichiers correspondant aux messages utilisateur,
        # qui ont ete marque en stripant les lignes correspondantes
        if unsorted_user_messages_recovery:
            # Parcours des fichiers concerne pour reanalyser leur lignes specifiques aux erreurs
            user_message_error_number = 0
            user_message_warning_number = 0
            recovered_error_number = 0
            recovered_warning_number = 0
            for file_name in erroneous_file_names:
                # Recherche des lignes des fichiers erronnes
                test_file_lines = erroneous_test_file_lines.get(file_name)
                if test_file_lines is not None:
                    ref_file_lines = erroneous_ref_file_lines.get(file_name)
                    assert ref_file_lines is not None
                    # Extraction des lignes stripees, qui correspond aux messages utilisateurs
                    test_file_lines = extract_striped_lines(test_file_lines)
                    ref_file_lines = extract_striped_lines(ref_file_lines)
                    # Comparaison de la partie des fichiers pre-traites relative aux messages utilisateur
                    # La comparaison se fait de facon muette, sans passer par le ficheir de log
                    errors, warnings = check_file_lines(
                        file_name,
                        file_name,
                        ref_file_lines,
                        test_file_lines,
                    )
                    user_message_error_number += errors
                    user_message_warning_number += warnings
                    # Comparaison apres avoir triee les messages utilisateurs
                    test_file_lines.sort()
                    ref_file_lines.sort()
                    errors, warnings = check_file_lines(
                        file_name,
                        file_name,
                        ref_file_lines,
                        test_file_lines,
                    )
                    recovered_error_number += errors
                    recovered_warning_number += warnings
            # Il faut que les erreurs ne proviennent que des messages utilisateurs
            if unsorted_user_messages_recovery:
                unsorted_user_messages_recovery = (
                    user_message_error_number == error_number
                )
            # Il faut qu'il n'y ai plus d'erreur apres tri des message utilisateurs
            if unsorted_user_messages_recovery:
                unsorted_user_messages_recovery = recovered_error_number == 0

        # Recuperation effective des erreurs si possible
        if unsorted_user_messages_recovery:
            # Messages sur la recuperation
            recovery_summary = "Recovery from unsorted user messages"
            recovery_message = recovery_summary.lower()
            write_message("\n" + recovery_summary + ":", log_file=log_file)
            write_message(
                "\tall errors come from the users messages in  "
                + ERR_TXT
                + " and in json reports, with a different order",
                log_file=log_file,
            )
            write_message(
                "\t" + str(error_number) + " errors converted to warnings",
                log_file=log_file,
            )
            # On transforme les erreur en warning
            warnings_number += error_number
            error_number = 0
            # On reinitialise egalement les stats d'erreur pour les extensuon concernees
            error_number_per_extension[".khj"] = 0
            error_number_per_extension[".khcj"] = 0
            error_number_in_err_txt = 0

    # Tentative de recuperation des erreurs si la seule difference provient de la limite des ressources
    # qui ne permet pas de calcul la courbe de ROC de facon exacte
    if error_number > 0:
        roc_curve_recovery = True

        # On verifie d'abord qu'il y a un warning correspondant dans le log utiliusateur
        if roc_curve_recovery:
            # On doit potentiellement relire ce fichier, car ce type de message correspond
            # a un motif resilient qui ne genere pas d'erreur
            err_file_lines = erroneous_test_file_lines.get(ERR_TXT)
            if err_file_lines is None:
                err_file_path = os.path.join(test_dir, ERR_TXT)
                err_file_lines = read_file_lines(err_file_path)
            if err_file_lines is None:
                roc_curve_recovery = False
            else:
                searched_warning = (
                    "warning : Evaluation Selective Naive Bayes : Not enough memory to compute the exact AUC:"
                    " estimation made on a sub-sample of size"
                )
                roc_curve_recovery = (
                    find_in_lines(err_file_lines, searched_warning) >= 0
                )

        # Comptage des erreurs pour les fichier d'evaluation au format xls
        if roc_curve_recovery:
            error_number_in_evaluation_xls = 0
            for file_name in erroneous_file_names:
                _, file_extension = os.path.splitext(file_name)
                if file_extension == ".xls" and "EvaluationReport" in file_name:
                    error_number_in_evaluation_xls += error_number_per_file.get(
                        file_name
                    )
            # On test si les nombre d'erreurs se rappartis dans le fichier de log utilisateur,
            # les rapports json et les fichiers d'evalauation au format xls
            error_number_in_json_report_files = error_number_per_extension.get(
                ".khj", 0
            )
            roc_curve_recovery = (
                error_number_in_err_txt
                + error_number_in_json_report_files
                + error_number_in_evaluation_xls
                == error_number
            )

        # Analyse specifique des rapports json en excluant la partie lie a la courbe de ROC
        if roc_curve_recovery:
            for file_name in erroneous_file_names:
                _, file_extension = os.path.splitext(file_name)
                if file_extension == ".khj":
                    # Parcours des fichiers concerne pour reanalyser leur lignes specifiques aux erreurs
                    roc_curve_error_number = 0
                    roc_curve_warning_number = 0
                    test_file_lines = erroneous_test_file_lines.get(file_name)
                    ref_file_lines = erroneous_ref_file_lines.get(file_name)
                    assert test_file_lines is not None
                    assert ref_file_lines is not None
                    # Extraction des qui correspond au calcul de l'AUC et des courbes de ROC
                    for key in ["auc", "values"]:
                        selected_test_file_lines = (
                            extract_key_matching_lines_in_json_file(
                                test_file_lines, key
                            )
                        )
                        selected_ref_file_lines = (
                            extract_key_matching_lines_in_json_file(ref_file_lines, key)
                        )
                        # Comparaison de la partie des fichiers pre-traites relative aux messages utilisateur
                        # La comparaison se fait de facon muette, sans passer par le ficheir de log
                        errors, warnings = check_file_lines(
                            file_name,
                            file_name,
                            selected_test_file_lines,
                            selected_ref_file_lines,
                        )
                        roc_curve_error_number += errors
                        roc_curve_warning_number += warnings
            # Le recouvrement est possible si le nombre d'erreurs trouves specifiquement pour le calcul
            # de l'AUC et des courbes de ROC correspond au nombre d'eerur total
            assert roc_curve_error_number <= error_number_in_json_report_files
            roc_curve_recovery = (
                roc_curve_error_number == error_number_in_json_report_files
            )

        # Recuperation effective des erreurs si possible
        if roc_curve_recovery:
            # Messages sur la recuperation
            recovery_summary = "Recovery from AUC rough estimate"
            recovery_message = recovery_summary.lower()
            write_message("\n" + recovery_summary + ":", log_file=log_file)
            write_message(
                "\tall errors in json report file come from AUC rough estimate",
                log_file=log_file,
            )
            write_message(
                "\t"
                + str(roc_curve_error_number)
                + " errors in json report files converted to warnings",
                log_file=log_file,
            )
            write_message(
                "\t"
                + str(error_number - roc_curve_error_number)
                + " errors in evaluation xls files ignored and converted to warnings",
                log_file=log_file,
            )
            # On transforme les erreur en warning
            warnings_number += error_number
            error_number = 0
            # On reinitialise egalement les stats d'erreur pour les extensuon concernees
            error_number_per_extension[".khj"] = 0
            error_number_per_extension[".xls"] = 0

    # Message dedies aux fichiers speciaux
    special_error_file_message = ""
    for file_name in SPECIAL_ERROR_FILES:
        if special_error_file_error_numbers[file_name] > 0:
            special_error_file_message = SUMMARY_SPECIAL_FILE_KEYS_PER_FILE[file_name]
            break

    # Ecriture d'un resume synthetique
    write_message("\n" + SUMMARY_TITLE, log_file=log_file)
    write_message(str(warnings_number) + " " + SUMMARY_WARNING_KEY, log_file=log_file)
    write_message(str(error_number) + " " + SUMMARY_ERROR_KEY, log_file=log_file)
    if special_error_file_message != "":
        write_message(special_error_file_message, log_file=log_file)
    if error_number > 0:
        # Tri des extensions
        file_extensions = []
        for file_extension in error_number_per_extension:
            file_extensions.append(file_extension)
        file_extensions.sort()
        # Message specifique si erreurs dans un seul type de fichier
        if error_number_in_err_txt > 0:
            extension_message += ERR_TXT
            if error_number_in_err_txt == error_number:
                specific_message = append_message(
                    specific_message, "errors only in " + ERR_TXT
                )
        if len(file_extensions) > 0:
            for file_extension in file_extensions:
                extension_message += file_extension
                if error_number_per_extension[file_extension] == error_number:
                    specific_message = append_message(
                        specific_message, "errors only in " + file_extension + " files"
                    )
        # Ecriture des messages additionnels
        if extension_message != "":
            write_message(SUMMARY_FILE_TYPES_KEY + extension_message, log_file=log_file)
        if specific_message != "":
            write_message(SUMMARY_NOTE_KEY + specific_message, log_file=log_file)

    # Ecriture d'un message additionnel lie a la portabilite
    portability_message = append_message(portability_message, recovery_message)
    if portability_message != "":
        write_message(SUMMARY_PORTABILITY_KEY + portability_message, log_file=log_file)

    # Affichage d'un message de fin sur la console
    final_message = "--Comparison done : "
    final_message += str(compared_files_number) + " files(s) compared, "
    final_message += str(error_number) + " error(s), "
    final_message += str(warnings_number) + " warning(s)"
    if special_error_file_message != "":
        final_message += ", " + special_error_file_message
    if recovery_message != "":
        final_message += ", Recovery from errors"
    print(final_message)
    print("log writed in " + log_file_path + "\n")


def is_file_with_json_extension(file_path):
    """Test si le path d'un fichier correspond a un fichier json"""
    # Recherche du fichier compare et de son extension
    file_name = os.path.basename(file_path)
    _, file_extension = os.path.splitext(file_name)

    # Test si fichier json
    json_file_extensions = [".json", ".khj", ".khvj", ".khcj", ".kdicj"]
    is_json_file = file_extension in json_file_extensions
    # Cas particulier des fichier .bad qui sont en fait des fichiers json
    # On test ici l'existence d'un fichier ne differenent que par l'extenion
    # Attention: test adhoc en dur pour quelques jeu de test de LearningTesrt
    # (ex: LearningTest\TestKhiops\Advanced\AllResultsApiMode)
    if file_extension == ".bad":
        if (
            os.path.isfile(file_path.replace(".bad", ".khj"))
            or os.path.isfile(file_path.replace(".bad", ".khj"))
            or os.path.isfile(file_path.replace(".bad", ".kdicj"))
        ):
            is_json_file = True
    return is_json_file


def is_line_striped(line):
    """Test si une ligne est stripee, sans caractere fin de ligne a la fin"""
    return len(line) == 0 or line[-1] != "\n"


def strip_user_message_lines(lines):
    """Renvoie la liste des lignes en ayant stripe toutes les lignes correspondant a
    message utilisateur ('error' ou 'warning')
    Permet ensuite de reperer simplement ces lignes dans une liste
    """
    result_lines = []
    for line in lines:
        if line.find("warning : ") == 0 or line.find("error : ") == 0:
            line = line.strip()
        result_lines.append(line)
    return result_lines


def strip_user_message_lines_in_json_file(lines):
    """Analyse d'un fichier json pour identifiant les sections 'messages'
    contenant les messages utilisateur ('error' ou 'warning')
    Les ligne correspondantes sont mise sous le meme format que dans le fichier
    de log d'erreur en supprimant les caracteres '"' de debut et de fin
    Ces lignes sont egalement stripees pour pouvoir les reperer simplement dans la
    la liste de ligne retournee en sortie

    Remarque: on se base sur le formatge json en sortie des outils Khiops,
    qui permet de faire une analyse simple ligne a ligne et de garder les numeros
    de lignes corrects dans les message d'erreur, meme en cas de format json erronne
    Une alternative par chargement direct d'un fichier json ne permettrait pas
    ce type d'analyse et de diagnostic
    """

    def clean_message_line(line):
        """Nettoyage d'une ligne de message, entre '"' et potentiellement suivi d'une ','
        Cela ne gere pas tous les cas d'encodage json, mais cela est suffisant la plupart du temps
        """
        cleaned_line = line.strip()
        # Cas d'un milieur de section, avec ',' en fin de ligne
        if cleaned_line[-1] == ",":
            cleaned_line = cleaned_line[1:-2]
        # Cas d'une fin de section
        else:
            cleaned_line = cleaned_line[1:-1]
        return cleaned_line

    # Recherche des ligne du fichier dans les sections "messages"
    in_message_section = False
    result_lines = []
    # Pretraitement des lignes
    for line in lines:
        # Cas ou est dans la section des message
        if in_message_section:
            # Detection de la fin de section
            in_message_section = line.strip() != "]"
            # Nettoyage des lignes dans la section message
            if in_message_section:
                line = clean_message_line(line)
        # Cas hors de la section des message
        else:
            # Detection du debut de section
            in_message_section = line.strip() == '"messages": ['
        result_lines.append(line)
    return result_lines


def discard_key_matching_lines_in_json_file(lines, pattern):
    """Renvoie la sous-liste des lignes ne correspondant pas a la cle en parametre"""
    result_lines = []
    searched_full_pattern = '"' + pattern + '": '
    for line in lines:
        if line.strip().find(searched_full_pattern) != 0:
            result_lines.append(line)
    return result_lines


def extract_key_matching_lines_in_json_file(lines, pattern):
    """Renvoie la sous-liste des lignes correspondant a la cle en parametre"""
    result_lines = []
    searched_full_pattern = '"' + pattern + '": '
    for line in lines:
        if line.strip().find(searched_full_pattern) == 0:
            result_lines.append(line)
    return result_lines


def extract_striped_lines(lines):
    """Retourne la sous_liste des ligne stripees de la liste en entree"""
    striped_lines = []
    for line in lines:
        if is_line_striped(line):
            striped_lines.append(line)
    return striped_lines


def filter_sequential_messages_lines(lines, log_file=None):
    """Filtrage des errors et warning sequentiel d'un ensemble de lignes

    En sequentiel, de nouveaux message de type 100th ou ...
    sont emis, alors qu'il sont absents en parallele
    En les filtrant, on rend les version sequentielle et parallele comparable
    Retourne les ligne filtrees, avec un message dans le log sur le nombre de lignes filtrees
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

    result_lines = []
    filtered_line_number = 0
    # Filtrage des lignes
    i = 0
    line_number = len(lines)
    while i < line_number:
        line = lines[i]
        # On ne traite pas la derniere ligne, qui n'a pas de ligne suivante
        if i == line_number - 1:
            result_lines.append(line)
        else:
            next_line = lines[i + 1]
            # On saute deux lignes si elles sont specifique a des message en sequentiel
            if is_specific_line_pair_sequential(line, next_line):
                i += 1
                filtered_line_number += 2
            else:
                result_lines.append(line)
        i += 1
    # Message si lignes filtrees
    if filtered_line_number > 0:
        write_message(
            "Specific sequential messages (100th...): "
            + str(filtered_line_number)
            + " lines filtered",
            log_file=log_file,
        )
    return result_lines


def check_file_lines(
    ref_file_path: str, test_file_path, ref_file_lines, test_file_lines, log_file=None
):
    """Comparaison d'un fichier de test et d'un fihcier de reference
    Parametres:
    - ref_file_path: chemin du fichier de refence
    - test_file_path: chemin du fichier de test
    - ref_file_lines: liste des lignes du fichier de reference
    - test_file_lines: liste des lignes du fichier de test
    - log file: fichier de log ouvert dans le quel des messages sont ecrits (seulement si log_file est sepcifie)

    Retourne
    - error: nombre d'erreurs
    - warning: nombre de warning

    Les nom des fichiers en parametre permettent de specialiser les comparaisons selon le type de fichier
    Les listes de lignes en entree permettent d'eviter de relire un fichier dont on connait le nom
    et dont on a deja lu les lignes.
    Cela permet par exemple de reutiliser les methodes de comparaison apres avoir filtre le fichier
    de sous-parties que l'on ne souhaite pas comparer.

    Compare les fichiers ligne par ligne, cellule par cellule (separateur '\t'), et token par token
    dans le cas des fichier json ou dictionnaire
    On a avec des tolerances selon le type de fichier.
    Pour les valeurs numeriques, une difference relative de 0.00001 est toleree
    - ecrit les difference dans le fichier log_file et affiche le nb d'erreur dans le terminal
    - warning : 2 cellules contiennent des valeurs numeriques avec une difference relative toleree
    - error : les cellules sont differentes
    """

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

        pos_start1 = value.find(" uses too much memory (more than ")
        if pos_start1 == -1:
            return value
        pos_start2 = value.find(" after reading ")
        if pos_start2 == -1:
            return value
        pos_start3 = value.find(" secondary records ")
        if pos_start3 == -1:
            return value
        if pos_start1 >= 0 and pos_start2 > pos_start1 and pos_start3 > pos_start2:
            filtered_value = value[:pos_start1] + " uses too much memory..."
        else:
            filtered_value = value
        return filtered_value

    def filter_secondary_table_stats(value):
        # Supression d'un pattern de nombre de records secondaires
        pos_start1 = value.find("Table ")
        if pos_start1 == -1:
            return value
        pos_start2 = value.find(" Records: ")
        if pos_start2 == -1:
            return value
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
    is_json_file = is_file_with_json_extension(file_name)

    # initialisation des nombres d'erreurs et de warning
    error = 0
    warning = 0
    numerical_warning = 0  # Lie a une tolerance dee difference de valeur numerique
    resilience_warning = (
        0  # Lie a un pattern de message avec tolerance (ex: "Not enough memory")
    )

    # Pas de controle si fichier de temps
    if is_time_file:
        write_message("OK", log_file=log_file)
        return error, warning

    # Comparaison des nombres de lignes
    file_ref_line_number = len(ref_file_lines)
    file_test_line_number = len(test_file_lines)
    if file_test_line_number != file_ref_line_number:
        write_message(
            "test file has "
            + str(file_test_line_number)
            + " lines and reference file has "
            + str(file_ref_line_number)
            + " lines",
            log_file=log_file,
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
                    "test file (line "
                    + str(line)
                    + ") has "
                    + str(field_number_test)
                    + " columns and reference file has "
                    + str(field_number_ref)
                    + " columns",
                    log_file=log_file,
                )
            elif error == max_print_error:
                write_message("...", log_file=log_file)
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
            if is_error_file or is_json_file:
                filter_secondary_record_detected = True
                field_ref = filter_secondary_record(field_ref)
                field_test = filter_secondary_record(field_test)

            # Cas particulier du nombre de record secondaire raporte dans le fichier d'erreur,
            # si des enregistrement secondaire ont ete detectes
            if is_error_file:
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
                        "l"
                        + str(line)
                        + " c"
                        + str(i + 1)
                        + " "
                        + field_test
                        + " -> "
                        + field_ref,
                        log_file=log_file,
                    )
                elif error == max_print_error:
                    write_message("...", log_file=log_file)
                error += 1
            elif eval_res == 2:
                warning += 1
                if threshold_res == 0:
                    resilience_warning += 1
                else:
                    numerical_warning += 1
            max_threshold = max(threshold_res, max_threshold)
    if warning > 0:
        if numerical_warning > 0:
            write_message(
                str(numerical_warning) + " warning(s) (epsilon difference)",
                log_file=log_file,
            )
        if resilience_warning > 0:
            write_message(
                str(resilience_warning)
                + " warning(s) (resilience to specific message patterns)",
                log_file=log_file,
            )
    if error == 0:
        write_message("OK", log_file=log_file)
    if error > 0:
        message = str(error) + " error(s)"
        if max_threshold > 0:
            message += " (max relative difference: " + str(max_threshold) + ")"
        write_message(message, log_file=log_file)

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


# Liste de motifs pour lesquels ont admet une variation normale s'il font parti de la comparaison
# dans une paire de cellules
# Dans ce cas, on ignore la comparaison
RESILIENCE_PATTERNS = [
    "system resources",  # Gestion des ressources systemes
    "Unable to access file",  # Acces a un fichier
    "Unable to open file",  # Ouverture d'un fichier
    " : Not enough memory to ",  # Manque de memoire
    " : Not enough memory for ",  # Manque de memoire (variante)
    "Too much memory necessary to store the values",  # Manque de memoire (variante)
]


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

    # Traitement des patterns toleres pour la comparaison
    for pattern in RESILIENCE_PATTERNS:
        if cell1.find(pattern) != -1 and cell2.find(pattern) != -1:
            # On renvoie un warning, mais avec 0 indique la tolerance
            return [2, 0]

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
