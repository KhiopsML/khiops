# Copyright (c) 2023-2025 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import os.path
import re

import _kht_constants as kht
import _kht_utils as utils
import _kht_results_management as results

"""
Verification des resultats d'un repertoire de test terminal

La comparaison est effectue entre les resultats de test, et les resultats de reference
correspondant au contexte en cours (plateforme, parallel ou sequuentiel...).
Elle se fait sur tous les fichiers du repertoire de facon hierarchique
- nombre de fichiers de chaque repertoire
- noms des fichiers
- pour chaque fichier
  - nombre de lignes
  - contenu
    - comparaison des lignes
      - si necessaire, comparaison des champs des lignes, pour un separateur tabulation
        - si necessaire, comparaison des tokens du champ,
          dans le cas de la tokenisation d'un fichier json ou kdic

La comparaison se fait en etant tolerant aux variations 'normales' selon le contexte d'execution
- il peut y avoir des resultats de reference different selon le contexte
- on filtre prealablement certaines informations non presentes systematiquement
  - copyright
  - prefix de type '[0] ' lie au process, genere par mpiexec en parallele
  - statistique sur la memoire ne mode debug
  ...
- il y a une tolerance sur les valeur numeriques, ce qui entraine alors des warning et non des erreurs
- ...

En cas d'erreurs residuelles, plusieurs strategies de recouvrement des erreurs sont utilises,
a differents moments du processus de comparaison
- tolerance sur echec de scenario, si cela correspond au resultats de reference
- tolerance aux noms de fichier utilisant des caracteres accentues systeme dependant
- tolerance sur les messages d'erreurs differents en parallele et en sequentiel
- tolerance sur les message d'erreur lies au manque de ressource
...
"""

# Nom du fichier de comparaison
COMPARISON_LOG_FILE_NAME = kht.COMPARISON_RESULTS_LOG

# Constantes de la section SUMMARY des fichiers de log des resultats de comparaison
SUMMARY_TITLE = "SUMMARY"
SUMMARY_WARNING_KEY = "warning(s)"
SUMMARY_ERROR_KEY = "error(s)"
SUMMARY_FILE_TYPES_KEY = "Problem file types: "
SUMMARY_NOTE_KEY = "Note: "
SUMMARY_PORTABILITY_KEY = "Portability: "

# Constantes pour la gestion des fichiers speciaux, par priorite decroissante
SUMMARY_TIMEOUT_ERROR_KEY = "TIMEOUT ERROR"
SUMMARY_FATAL_ERROR_KEY = "FATAL ERROR"
SUMMARY_UNEXPECTED_OUTPUT_KEY = "UNEXPECTED OUTPUT"
SUMMARY_SPECIAL_FILE_KEYS = [
    SUMMARY_TIMEOUT_ERROR_KEY,
    SUMMARY_FATAL_ERROR_KEY,
    SUMMARY_UNEXPECTED_OUTPUT_KEY,
]

# Association entre type de fichier special et cle de gestion dans le resume
SUMMARY_SPECIAL_FILE_KEYS_PER_FILE = {
    kht.STDOUT_ERROR_LOG: SUMMARY_UNEXPECTED_OUTPUT_KEY,
    kht.STDERR_ERROR_LOG: SUMMARY_UNEXPECTED_OUTPUT_KEY,
    kht.PROCESS_TIMEOUT_ERROR_LOG: SUMMARY_TIMEOUT_ERROR_KEY,
    kht.RETURN_CODE_ERROR_LOG: SUMMARY_FATAL_ERROR_KEY,
}
assert len(SUMMARY_SPECIAL_FILE_KEYS_PER_FILE) == len(kht.SPECIAL_ERROR_FILES)

# Ensemble des cle pouvant se trouver dans le resume
ALL_SUMMARY_KEYS = [
    SUMMARY_WARNING_KEY,
    SUMMARY_ERROR_KEY,
    SUMMARY_FILE_TYPES_KEY,
    SUMMARY_PORTABILITY_KEY,
] + SUMMARY_SPECIAL_FILE_KEYS
assert len(set(ALL_SUMMARY_KEYS)) == len(ALL_SUMMARY_KEYS), (
    "Summary keys " + str(ALL_SUMMARY_KEYS) + " must not contain duplicates"
)


def analyse_comparison_log(test_dir):
    """
    Analyse du log de comparaison des resultats de test et de reference
    present dans un repertoire de test
    Renvoie:
    - error_number
      Le nombre d'erreurs deduit du resume
    - warning_number
      Le nombre de warnings deduit du resume
    - summary_infos:
      Un dictionnaire par avec une ligne de texte par cle de resume (ALL_SUMMARY_KEYS)
    - files_infos:
      Un dictionaire par nom de fichier contenant le resultat de la comparaison
      pour ce fichier, sous la forme d'un texte potentiellement multi-lignes
      Ce texte contient 'OK' uniquement si aucun problme n'est detecte
      Il contient des lignes de texte, dont certain sont potentiellement prefixes par 'warning: '
      ou 'error : ' sinon
    Si le log de comparaison n'est pas disponible ou exploitable, on retourne une erreur
    """

    def extract_number(message):
        assert message != ""
        fields = message.split()
        assert fields[0].isdigit()
        number = int(fields[0])
        return number

    utils.check_test_dir(test_dir)

    # Initialisation des resultats
    error_number = 0
    warning_number = 0
    summary_infos = {}
    files_infos = {}

    # Traitement des erreurs memorisee dans le log
    log_file_path = os.path.join(test_dir, kht.COMPARISON_RESULTS_LOG)
    if not os.path.isfile(log_file_path):
        # Erreur speciale si pas de fichier de comparaison
        error_number = 1
        summary_infos[SUMMARY_NOTE_KEY] = "The test has not been launched"
    else:
        try:
            with open(log_file_path, "r", errors="ignore") as log_file:
                lines = log_file.readlines()
        except Exception as exception:
            # Erreur speciale si probleme de lecture du fichier de comparaison
            lines = None
            error_number = 1
            summary_infos[SUMMARY_NOTE_KEY] = (
                "Unable to read file " + kht.COMPARISON_RESULTS_LOG + str(exception)
            )
        # Analyse du contenu du fichier
        file_pattern = "file "
        if lines is not None:
            index = 0
            while index < len(lines):
                line = lines[index]
                index += 1
                line = line.strip()

                # Analyse des lignes concernant chaque fichier avant le resume
                if line.find(file_pattern) == 0:
                    file_path = line[len(file_pattern) :]
                    file_name = os.path.basename(file_path)
                    file_info = ""
                    while index < len(lines):
                        line = lines[index]
                        index += 1
                        line = line.strip()
                        if line == "":
                            break
                        else:
                            if file_info != "":
                                file_info += "\n"
                            file_info += line
                    files_infos[file_name] = file_info
                    continue

                # Analyse du resume jsuq'u la fin du fichier si debut de resume trouve
                if line == SUMMARY_TITLE:
                    while index < len(lines):
                        line = lines[index]
                        index += 1
                        line = line.strip()
                        for key in ALL_SUMMARY_KEYS:
                            if line.find(key) >= 0:
                                summary_infos[key] = line
                                if key == SUMMARY_WARNING_KEY:
                                    warning_number = extract_number(line)
                                elif key == SUMMARY_ERROR_KEY:
                                    error_number = extract_number(line)

            # Erreur speciale si le resume n'est pas trouve
            if len(summary_infos) == 0:
                assert error_number == 0
                error_number = 1
                specific_message = (
                    "Section '"
                    + SUMMARY_TITLE
                    + "' not found in "
                    + kht.COMPARISON_RESULTS_LOG
                )
                summary_infos[SUMMARY_NOTE_KEY] = specific_message
    # Retour des resultats
    return error_number, warning_number, summary_infos, files_infos


def clean_version_from_results(results_dir):
    """
    Nettoyage de toute reference a la version des fichiers de resultats, quelque soit leur nature
    (.kdic, .xls, json...) en remplacant la version par 'VERSION', et en memorisant la version dans
    la deuxieme ligne du fichier time.log
    L'objectif est de minimiser les difference entre resultats et resultats de references, de facon
    a minimiser le volume necessaire pour memoriser tout LearningTest sur un repo git.

    :param results_dir: sous-repertoire de resultats d'un repertoire de test
    """

    def read_bytes(path):
        """Lecture du contenu d'un fichier sous formes de bytes"""
        data = []
        with open(path, "rb") as file:
            data = file.read()
        return data

    def write_bytes(path, data):
        """Ecriture du contenu d'un fichier sous formes de bytes"""
        with open(path, "wb") as file:
            file.write(data)

    def extract_data_head_as_text(data):
        """Retourne le debut d'un bloc de bytes sous forme de texte,
        en ayant remplace les fine de ligne par des blancs"""
        data_head = data[0:1000]
        data_head_text = data_head.decode("latin-1")
        return data_head_text

    def remove_version_in_data(data, pos_version, version):
        """Remplace la version a la position donnees par une valeur constante"""
        assert pos_version > 0
        # On doit avoir acces a la fois a la version byte et a la version texte
        data_head = data[0:1000]
        data_head_text = data_head.decode("latin-1")
        new_data_head_text = (
            data_head_text[:pos_version]
            + "VERSION"
            + data_head_text[pos_version + len(version) :]
        )
        new_data_dead = new_data_head_text.encode("latin-1")
        new_data = new_data_dead + data[len(data_head) :]
        return new_data

    def trace(file_name, pos_version, version, data, new_data):
        """Message de trace"""
        _, file_extension = os.path.splitext(file_name)
        print(
            "\t"
            + file_extension
            + " "
            + file_name
            + "\t"
            + str(pos_version)
            + " "
            + version
            + " "
            + str(len(data))
            + " "
            + str(len(new_data) - len(data))
        )

    # Verification que l'on est sur un jeu de test
    trace_on = False
    utils.check_test_dir(os.path.join(results_dir, ".."))

    # Nettoyage du contenu du repertoire uniquement s'il existe un fichier de temps
    time_file_path = os.path.join(results_dir, kht.TIME_LOG)
    if not os.path.isfile(time_file_path):
        return

    # Acces aux fichiers du repertoire
    if trace_on:
        print("clean_version_from_results: " + results_dir)
    version_key = "VERSION"
    found_version = ""
    file_names = os.listdir(results_dir)
    for file_name in file_names:
        _, file_extension = os.path.splitext(file_name)
        file_path = os.path.join(results_dir, file_name)
        # Cas d'un fichier de dictionnaire ou excel
        if file_extension == ".kdic" or file_extension == ".xls":
            # Recherche de la version si elle n'a pas ete trouvee
            data = read_bytes(file_path)
            if len(data) > 0:
                data_head_text = extract_data_head_as_text(data)
                pos = utils.find_pattern_in_line(data_head_text, ["#", " "])
                if pos >= 0:
                    fields = (
                        data_head_text[pos:]
                        .replace("\r", " ")
                        .replace("\n", " ")
                        .split(" ")
                    )
                    if len(fields) >= 2:
                        version = fields[1]
                        if version != "" and version != version_key:
                            if found_version == "":
                                found_version = version
                            pos_version = pos + data_head_text[pos:].find(version)
                            new_data = remove_version_in_data(
                                data, pos_version, version
                            )
                            write_bytes(file_path, new_data)
                            if trace_on:
                                trace(file_name, pos_version, version, data, new_data)
        # Cas d'un fichier json
        elif is_file_with_json_extension(file_name):
            # Recherche de la version si elle n'a pas ete trouvee
            data = read_bytes(file_path)
            if len(data) > 0:
                data_head_text = extract_data_head_as_text(data)
                pos = utils.find_pattern_in_line(data_head_text, ['"version": '])
                if pos >= 0:
                    fields = (
                        data_head_text[pos:]
                        .replace("\r", " ")
                        .replace("\n", " ")
                        .split('"')
                    )
                    if len(fields) >= 4:
                        version = fields[3]
                        if version != "" and version != version_key:
                            if found_version == "":
                                found_version = version
                            pos_version = pos + data_head_text[pos:].find(version)
                            new_data = remove_version_in_data(
                                data, pos_version, version
                            )
                            write_bytes(file_path, new_data)
                            if trace_on:
                                trace(file_name, pos_version, version, data, new_data)
    # Mise a jour du fichier de temps en ajoutant une deuxième ligne contenant la version
    if found_version != "":
        time_file = open(time_file_path, "r", errors="ignore")
        lines = time_file.readlines()
        time_file.close()
        try:
            with open(
                os.path.join(results_dir, kht.TIME_LOG),
                "w",
                errors="ignore",
            ) as time_file:
                time_file.write(lines[0])
                time_file.write("version: " + found_version + "\n")
        except Exception as exception:
            print(
                "Enable to write file " + kht.TIME_LOG + " in " + kht.RESULTS + " dir ",
                exception,
            )


def check_results(test_dir, forced_context=None):
    """
    Fonction principale de comparaison des resultats de test et de reference
     Les fichiers sont compares 2 a 2 et la synthese de la comparaison est ecrite
     dans un fichier de log, avec un resume en fin de fichier, facile a parser
    On retourne True s'il n'y a aucune erreur

    Le parametrage d'un contexte force en entree permet d'effectuer la comparaison avec
    un contexte (parallel|sequential, platform) alternatif. Dans ce cas:
    - l'objectif est essentiellement de renvoyer un indicateur global de succes de la comparaison
    - on n'ecrit pas de fichier de comparaison
    """
    utils.check_test_dir(test_dir)

    # Initialisation des stats de comparaison
    special_error_file_error_numbers = {}
    for file_name in kht.SPECIAL_ERROR_FILES:
        special_error_file_error_numbers[file_name] = 0
    error_number = 0
    warning_number = 0
    user_message_warning_number = 0
    compared_files_number = 0
    error_number_in_err_txt = 0
    error_number_per_extension = {}
    error_number_per_file = {}
    erroneous_ref_file_lines = {}
    erroneous_test_file_lines = {}
    erroneous_file_names = []
    extension_message = ""
    specific_message = ""
    portability_message = ""
    recovery_message = ""

    # Ouverture du fichier de log de comparaison, sauf si lle contexte est force
    log_file = None
    if forced_context is None:
        log_file_path = os.path.join(test_dir, COMPARISON_LOG_FILE_NAME)
        try:
            log_file = open(log_file_path, "w", errors="ignore")
        except Exception as exception:
            print("error : unable to create log file " + log_file_path, exception)
            return
        assert log_file is not None
        utils.write_message(
            utils.test_dir_name(test_dir) + " comparison", log_file=log_file
        )

    # Information sur le contexte courant de comparaison des resultats
    if forced_context is None:
        current_context = results.get_current_results_ref_context()
        utils.write_message(
            "current comparison context : " + str(current_context),
            log_file=log_file,
        )
    else:
        current_context = forced_context

    # Test de presence du repertoire de test a comparer
    results_dir = os.path.join(test_dir, kht.RESULTS)
    if not os.path.isdir(results_dir):
        utils.write_message(
            "error : no comparison, test directory not available (" + results_dir + ")",
            log_file=log_file,
            show=True,
        )
        error_number = error_number + 1

    # Recherche du repertoire courant des resultats de reference
    results_ref, candidate_dirs = results.get_results_ref_dir(
        test_dir, forced_context=forced_context, log_file=log_file, show=True
    )
    if results_ref is None:
        utils.write_message(
            "error : invalid "
            + kht.RESULTS_REF
            + " dirs "
            + utils.list_to_label(candidate_dirs),
            log_file=log_file,
            show=True,
        )
        error_number = error_number + 1
    elif len(candidate_dirs) >= 2:
        portability_message = (
            "used " + results_ref + " dir among " + utils.list_to_label(candidate_dirs)
        )
        utils.write_message(
            portability_message,
            log_file=log_file,
            show=True,
        )

    # Test de presence du repertoire de reference a comparer
    results_ref_dir = ""
    if error_number == 0:
        results_ref_dir = os.path.join(test_dir, results_ref)
        if not os.path.isdir(results_ref_dir):
            utils.write_message(
                "error : no comparison, reference directory not available ("
                + results_ref_dir
                + ")",
                log_file=log_file,
                show=True,
            )
            error_number = error_number + 1

    # Comparaison effective si possible
    if error_number == 0:
        # Acces aux fichiers des repertoires de reference et de test
        # On passe par le format bytes des noms de fichier pour avoir acces
        # aux fichier quelque soit la plateforme
        # - Windows ne supporte que l'utf8
        # - Linux stocke les noms directement sous la forme de bytes
        ref_byte_file_names = os.listdir(os.fsencode(results_ref_dir))
        test_byte_file_names = os.listdir(os.fsencode(results_dir))

        # On memorise les noms de fichiers sous forme de string pour faciliter le reporting
        # Tout en gardant l'association entre le nom python (utf8) et les noms en bytes
        #
        # Attention, la methode fsdecode utilise des 'surrogate characters' invisible
        # permettant de garder trace des bytes non utf8 pour le re-encodage par fsencode si necessaire
        # On passe par une version 'nettoyee' purement ascii de ces caracteres speciaux pour memoriser
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
            cleaned_file_name = file_name.encode("ascii", "ignore").decode("ascii")
            if cleaned_file_name != file_name:
                utils.write_message(
                    "warning : reference file name with a byte encoding ("
                    + str(byte_file_name)
                    + ") used under ascii name ("
                    + cleaned_file_name
                    + ")",
                    log_file=log_file,
                )
                warning_number += 1
                recovery = True
            ref_file_names.append(cleaned_file_name)
            dic_ref_byte_file_names[cleaned_file_name] = byte_file_name
        # Idem pour les resultat de test
        test_file_names = []
        dic_test_byte_file_names = {}
        for byte_file_name in test_byte_file_names:
            file_name = os.fsdecode(byte_file_name)
            cleaned_file_name = file_name.encode("ascii", "ignore").decode("ascii")
            if cleaned_file_name != file_name:
                utils.write_message(
                    "warning : test file name with a byte encoding ("
                    + str(byte_file_name)
                    + ") used under ascii name ("
                    + cleaned_file_name
                    + ")",
                    log_file=log_file,
                )
                warning_number += 1
                recovery = True
            test_file_names.append(cleaned_file_name)
            dic_test_byte_file_names[cleaned_file_name] = byte_file_name

        # Message de recuperation d'erreur si necessaire
        if recovery:
            utils.write_message(
                "\nRecovery from errors caused by byte encoding of file names in another platform",
                log_file=log_file,
            )
            recovery_message = utils.append_message(
                recovery_message, "Recovery of type byte encoding of file names"
            )

        # On tri par nom de fichier pour ameliorer la stabilite du reporting inter plateformes
        ref_file_names.sort()
        test_file_names.sort()

        # Comparaison des nombres de fichiers
        ref_result_file_number = len(ref_file_names)
        test_result_file_number = len(test_file_names)
        if ref_result_file_number == 0:
            utils.write_message(
                "error : no comparison, missing reference result files",
                log_file=log_file,
                show=True,
            )
            error_number = error_number + 1
        elif ref_result_file_number != test_result_file_number:
            utils.write_message(
                "\nerror : number of results files ("
                + str(test_result_file_number)
                + ") should be "
                + str(ref_result_file_number),
                log_file=log_file,
                show=True,
            )
            error_number = error_number + 1
            # Affichage des noms des fichiers supplementaires
            max_file_reported = 20
            if test_result_file_number > ref_result_file_number:
                # Message specifique en cas de fichiers en trop
                specific_message = utils.append_message(
                    specific_message, "additional result files"
                )
                utils.write_message(
                    "Additional files in " + kht.RESULTS + " dir:", log_file=log_file
                )
                file_reported = 0
                for file_name in test_file_names:
                    if file_name not in ref_file_names:
                        if file_reported < max_file_reported:
                            utils.write_message("\t" + file_name, log_file=log_file)
                        else:
                            utils.write_message("\t...", log_file=log_file)
                            break
                        file_reported += 1
            elif test_result_file_number < ref_result_file_number:
                # Message specifique en cas de fichiers manquants
                specific_message = utils.append_message(
                    specific_message, "missing result files"
                )
                utils.write_message(
                    "Missing files in " + kht.RESULTS + " dir:", log_file=log_file
                )
                file_reported = 0
                for file_name in ref_file_names:
                    if file_name not in test_file_names:
                        if file_reported < max_file_reported:
                            utils.write_message("\t" + file_name, log_file=log_file)
                        else:
                            utils.write_message("\t...", log_file=log_file)
                            break
                        file_reported += 1

        # Comparaison des fichiers 2 a 2 en memorisant les erreurs par extension
        for file_name in ref_file_names:
            compared_files_number = compared_files_number + 1

            # Path des fichiers utilises pour le reporting
            ref_file_path = os.path.join(results_ref_dir, file_name)
            test_file_path = os.path.join(results_dir, file_name)

            # En-tete de comparaison des fichiers
            utils.write_message("\nfile " + test_file_path, log_file=log_file)

            # On utilise si possible le path des fichiers en bytes pour s'adapter aux contraintes de la plateforme
            # Les erreurs seront diagnostiquees si necessaire lors de la lecture des fichiers
            used_ref_file_path = ref_file_path
            if dic_ref_byte_file_names.get(file_name) is not None:
                used_ref_file_path = os.path.join(
                    os.fsencode(results_ref_dir), dic_ref_byte_file_names.get(file_name)
                )
            used_test_file_path = test_file_path
            if dic_test_byte_file_names.get(file_name) is not None:
                used_test_file_path = os.path.join(
                    os.fsencode(results_dir), dic_test_byte_file_names.get(file_name)
                )

            # Lecture des fichiers
            ref_file_lines = utils.read_file_lines(
                used_ref_file_path, log_file=log_file
            )
            test_file_lines = utils.read_file_lines(
                used_test_file_path, log_file=log_file
            )
            if ref_file_lines is None:
                error_number = error_number + 1
            if test_file_lines is None:
                error_number = error_number + 1

            # Comparaison si ok
            if ref_file_lines is not None and test_file_lines is not None:
                # Cas des fichiers stdout et stderr, que l'on filtre du prefix de process id presnet en parallele
                if file_name in [kht.STDOUT_ERROR_LOG, kht.STDERR_ERROR_LOG]:
                    ref_file_lines = utils.filter_process_id_prefix_from_lines(
                        ref_file_lines
                    )
                    test_file_lines = utils.filter_process_id_prefix_from_lines(
                        test_file_lines
                    )

                # Mise en forme specifique des messages utilisateurs (error, warning) pour les traiter
                # de facon identique dans les cas des fichiers de log utilisateur et json
                contains_user_messages = False
                # Cas du fichier de log utilisateur
                if file_name == kht.ERR_TXT:
                    contains_user_messages = True
                    # Identification des lignes de message
                    ref_file_lines = strip_user_message_lines(ref_file_lines)
                    test_file_lines = strip_user_message_lines(test_file_lines)
                # Cas des fichiers json
                elif is_file_with_json_extension(file_name):
                    contains_user_messages = True
                    # Pretraitement des lignes de message pour les mettre dans le meme format
                    # que pour les fichier d'erreur
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
                errors, warnings, user_message_warnings = check_file_lines(
                    ref_file_path,
                    test_file_path,
                    ref_file_lines,
                    test_file_lines,
                    log_file=log_file,
                )
                error_number += errors
                warning_number += warnings
                user_message_warning_number += user_message_warnings

                # Memorisation des statistiques par extension
                if errors > 0:
                    erroneous_file_names.append(file_name)
                    error_number_per_file[file_name] = errors
                    erroneous_ref_file_lines[file_name] = ref_file_lines
                    erroneous_test_file_lines[file_name] = test_file_lines
                    if file_name == kht.ERR_TXT:
                        error_number_in_err_txt += errors
                    else:
                        _, file_extension = os.path.splitext(file_name)
                        error_number_per_extension[file_extension] = (
                            error_number_per_extension.get(file_extension, 0) + errors
                        )

        # Message synthetique de recuperation des warnng sur les message utilisateur si necessaire
        if user_message_warning_number > 0:
            recovery_message = utils.append_message(
                recovery_message, "Recovery from varying patterns in user messages"
            )

        # Recherche des erreurs fatales, avec tentative de recuperation
        # On accepte les erreurs fatales que si on ales meme en test et reference,
        # et uniquement dans le cas du pattern particulier du "Batch mode failure" qui est du
        # a des scenario n'ayant pas pu s'excuter entierement pour des raison de portabilite
        fatal_error_recovery = True
        for file_name in test_file_names:
            # Cas d'une erreur fatale
            if file_name in kht.SPECIAL_ERROR_FILES:
                special_error_file_error_numbers[file_name] = (
                    special_error_file_error_numbers[file_name] + 1
                )
                error_number += 1
                special_error = SUMMARY_SPECIAL_FILE_KEYS_PER_FILE[file_name].lower()
                utils.write_message(
                    "\n" + special_error + " : found file " + file_name,
                    log_file=log_file,
                )

                # La tentative de recuperation des erreurs fatales echoue si on ne respecte
                # pas toutes les conditions necessaires
                if file_name not in [kht.STDERR_ERROR_LOG, kht.RETURN_CODE_ERROR_LOG]:
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
                        test_file_path = os.path.join(results_dir, file_name)
                        test_file_lines = utils.read_file_lines(
                            test_file_path, log_file=log_file
                        )
                        # Pattern dans le cas de sdterr
                        fatal_error_pattern = (
                            "fatal error : Command file : Batch mode failure"
                        )
                        if file_name == kht.STDERR_ERROR_LOG:
                            if (
                                len(test_file_lines) == 0
                                or test_file_lines[0].strip() != fatal_error_pattern
                            ):
                                fatal_error_recovery = False
                        # Pattern dans le cas du code retour
                        return_code_error_pattern = "Wrong return code: 1 (should be 0)"
                        if file_name == kht.RETURN_CODE_ERROR_LOG:
                            if (
                                len(test_file_lines) == 0
                                or test_file_lines[0].strip()
                                != return_code_error_pattern
                            ):
                                fatal_error_recovery = False
        # Message de recuperation si necessaire
        if special_error_file_error_numbers[kht.RETURN_CODE_ERROR_LOG] > 0:
            # Cas de la recuperation
            if fatal_error_recovery:
                error_number -= special_error_file_error_numbers[
                    kht.RETURN_CODE_ERROR_LOG
                ]
                error_number -= special_error_file_error_numbers[kht.STDERR_ERROR_LOG]
                special_error_file_error_numbers[kht.RETURN_CODE_ERROR_LOG] = 0
                special_error_file_error_numbers[kht.STDERR_ERROR_LOG] = 0
                utils.write_message(
                    "\nRecovery from fatal errors caused solely by a 'Batch mode failure' in another platform",
                    log_file=log_file,
                )
                recovery_message = utils.append_message(
                    recovery_message, "Recovery of type 'Batch mode failure'"
                )

        # Ecriture des premieres lignes des fichiers d'erreur fatales ou de timeout si necessaire
        for file_name in test_file_names:
            if (
                file_name in kht.SPECIAL_ERROR_FILES
                and special_error_file_error_numbers[file_name] > 0
            ):
                # Lecture des lignes du fichier
                test_file_path = os.path.join(results_dir, file_name)
                test_file_lines = utils.read_file_lines(
                    test_file_path, log_file=log_file
                )
                utils.write_message(
                    "\nspecial error file " + test_file_path, log_file=log_file
                )
                max_print_lines = 10
                for i, line in enumerate(test_file_lines):
                    if i < max_print_lines:
                        utils.write_message("\t" + line.rstrip(), log_file=log_file)
                    else:
                        utils.write_message("\t...", log_file=log_file)
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
            ref_file_lines = erroneous_ref_file_lines.get(kht.ERR_TXT)
            test_file_lines = erroneous_test_file_lines.get(kht.ERR_TXT)

            # Filtrage des lignes selon le motif en nombre variable
            warning_pattern1 = "warning : Data table slice "
            warning_pattern2 = " : Read data table slice interrupted by user"
            filtered_ref_file_lines = []
            filtered_test_file_lines = []
            for line in ref_file_lines:
                if line.find(warning_pattern1) != 0 or line.find(warning_pattern2) < 0:
                    filtered_ref_file_lines.append(line)
            for line in test_file_lines:
                if line.find(warning_pattern1) != 0 or line.find(warning_pattern2) < 0:
                    filtered_test_file_lines.append(line)

            # Comparaison a nouveau des fichiers, en mode non verbeux
            errors, warnings, user_message_warnings = check_file_lines(
                kht.ERR_TXT,
                kht.ERR_TXT,
                filtered_ref_file_lines,
                filtered_test_file_lines,
            )

            # Recuperation possible si plus d'erreur apres filtrage
            varying_warning_messages_in_err_txt_recovery = errors == 0

        # Recuperation effective des erreurs si possible
        if varying_warning_messages_in_err_txt_recovery:
            # Messages sur la recuperation
            recovery_summary = (
                "Recovery from varying warning number in " + kht.ERR_TXT + " file only"
            )
            recovery_message = utils.append_message(recovery_message, recovery_summary)
            utils.write_message("\n" + recovery_summary + ":", log_file=log_file)
            utils.write_message(
                "\tall errors come from the warning in "
                + kht.ERR_TXT
                + " file only, du to varying number of active process number",
                log_file=log_file,
            )
            utils.write_message(
                "\t" + str(error_number) + " errors converted to warnings",
                log_file=log_file,
            )
            # On transforme les erreur en warning
            warning_number += error_number
            error_number = 0
            # On reinitialise egalement les stats d'erreur pour les extensions concernees
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

            def filter_record_index_from_lines(lines):
                """Filtrage avance des lignes en supprimant le debut de ligne jusqu'a l'index de record"""
                filtered_lines = []
                record_index_pattern = [
                    "warning : Data table ",
                    " : Record ",
                    " : Field ",
                ]
                for input_line in lines:
                    pos1 = utils.find_pattern_in_line(input_line, record_index_pattern)
                    if pos1 >= 0:
                        input_line = input_line[
                            input_line.find(record_index_pattern[-1]) :
                        ]
                    filtered_lines.append(input_line)
                return filtered_lines

            # Parcours des fichiers concerne pour reanalyser leur lignes specifiques aux erreurs
            user_message_error_number = 0
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
                    # La comparaison se fait de facon muette, sans passer par le fichier de log
                    errors, warnings, user_message_warnings = check_file_lines(
                        file_name,
                        file_name,
                        ref_file_lines,
                        test_file_lines,
                    )
                    user_message_error_number += errors
                    # Comparaison filtree les messages utilisateurs jusqu'aux index des records,
                    # qui peuvent varier d'une execution a l'autre, puis les avoir trier
                    test_file_lines = filter_record_index_from_lines(test_file_lines)
                    ref_file_lines = filter_record_index_from_lines(ref_file_lines)
                    test_file_lines.sort()
                    ref_file_lines.sort()
                    errors, warnings, user_message_warnings = check_file_lines(
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
            recovery_message = utils.append_message(recovery_message, recovery_summary)
            utils.write_message("\n" + recovery_summary + ":", log_file=log_file)
            utils.write_message(
                "\tall errors come from the users messages in  "
                + kht.ERR_TXT
                + " and in json reports, with a different order and possibly different record indexes",
                log_file=log_file,
            )
            utils.write_message(
                "\t" + str(error_number) + " errors converted to warnings",
                log_file=log_file,
            )
            # On transforme les erreur en warning
            warning_number += error_number
            error_number = 0
            # On reinitialise egalement les stats d'erreur pour les extensions concernees
            error_number_per_extension[".khj"] = 0
            error_number_per_extension[".khcj"] = 0
            error_number_in_err_txt = 0

    # Tentative de recuperation des erreurs si la seule difference provient de la limite des ressources
    # qui ne permet pas de calcul la courbe de ROC de facon exacte
    if error_number > 0:
        roc_curve_recovery = True

        # On verifie d'abord qu'il y a un warning correspondant dans le log utilisateur
        if roc_curve_recovery:
            # On doit potentiellement relire ce fichier, car ce type de message correspond
            # a un motif USER qui ne genere pas d'erreur
            err_file_lines = erroneous_test_file_lines.get(kht.ERR_TXT)
            if err_file_lines is None:
                err_file_path = os.path.join(results_dir, kht.ERR_TXT)
                err_file_lines = utils.read_file_lines(err_file_path)
            if err_file_lines is None:
                roc_curve_recovery = False
            else:
                searched_warning = (
                    "warning : Evaluation Selective Naive Bayes : "
                    + "Not enough memory to compute the exact AUC:"
                    + " estimation made on a sub-sample of size"
                )
                roc_curve_recovery = (
                    utils.find_pattern_in_lines(err_file_lines, [searched_warning]) >= 0
                )

        # Comptage des erreurs pour les fichier d'evaluation au format xls
        error_number_in_json_report_files = 0
        if roc_curve_recovery:
            error_number_in_evaluation_xls = 0
            for file_name in erroneous_file_names:
                _, file_extension = os.path.splitext(file_name)
                if file_extension == ".xls" and "EvaluationReport" in file_name:
                    error_number_in_evaluation_xls += error_number_per_file.get(
                        file_name
                    )
            # On teste si les nombre d'erreurs se rappartis dans le fichier de log utilisateur,
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
        roc_curve_error_number = 0
        roc_curve_warning_number = 0
        if roc_curve_recovery:
            for file_name in erroneous_file_names:
                _, file_extension = os.path.splitext(file_name)
                if file_extension == ".khj":
                    # Parcours des fichiers concerne pour reanalyser leur lignes specifiques aux erreurs
                    test_file_lines = erroneous_test_file_lines.get(file_name)
                    ref_file_lines = erroneous_ref_file_lines.get(file_name)
                    assert test_file_lines is not None
                    assert ref_file_lines is not None
                    # Extraction des champs qui correspondent au calcul de l'AUC et des courbes de ROC
                    for key in ["auc", "values"]:
                        # Selection d'un champ selon sa valeur
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
                        errors, warnings, user_message_warnings = check_file_lines(
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
            recovery_message = utils.append_message(recovery_message, recovery_summary)
            utils.write_message("\n" + recovery_summary + ":", log_file=log_file)
            utils.write_message(
                "\tall errors in json report file come from AUC rough estimate",
                log_file=log_file,
            )
            utils.write_message(
                "\t"
                + str(roc_curve_error_number)
                + " errors in json report files converted to warnings",
                log_file=log_file,
            )
            utils.write_message(
                "\t"
                + str(error_number - roc_curve_error_number)
                + " errors in evaluation xls files ignored and converted to warnings",
                log_file=log_file,
            )
            # On transforme les erreur en warning
            warning_number += error_number
            error_number = 0
            # On reinitialise egalement les stats d'erreur pour les extensions concernees
            error_number_per_extension[".khj"] = 0
            error_number_per_extension[".xls"] = 0

    # Tentative de recuperation des erreurs dans le cas tres particulier des caracteres accentues sous Windows,
    # ou on observe un comportement local a la machine de developement sous Windows different de celui
    # observe sur la machine Windows cloud, pourl aquelle certains fichiers sources avec caracteres
    # accentues n'ont pas pu etre dezippes correctement et conduisent a des erreurs de lecture
    # Dans ce cas uniquement, on tente de se comparer a une version linux de reference, pour laquelle
    # on a le meme probleme et on observe le meme comportement
    # Pas de recuperation d'erreur avancee si un contexte est force
    if error_number > 0 and forced_context is None:
        zip_encoding_recovery = True

        # On verifie d'abord que les conditions sont reunies
        linux_context = None
        if zip_encoding_recovery:
            # On doit etre sous Windows
            zip_encoding_recovery = results.get_context_platform_type() == "Windows"

            # Le fichier err.txt doit comporter une erreur de lecture
            if zip_encoding_recovery:
                read_error_pattern = ["error : File ./", " : Unable to open file ("]
                err_file_path = os.path.join(results_dir, kht.ERR_TXT)
                err_file_lines = utils.read_file_lines(err_file_path)
                zip_encoding_recovery = err_file_lines is not None
                # On doit trouver le pattern d'erreur
                if zip_encoding_recovery:
                    line_index = utils.find_pattern_in_lines(
                        err_file_lines, read_error_pattern
                    )
                    zip_encoding_recovery = line_index >= 0
                    # La ligne concernee doit avoir un probleme de caracrete accentue
                    if zip_encoding_recovery:
                        erronneous_line = err_file_lines[line_index]
                        ascii_erronneous_line = erronneous_line.encode(
                            "ascii", "ignore"
                        ).decode("ascii")
                        zip_encoding_recovery = ascii_erronneous_line != erronneous_line

            # Il doit y avoir un des resultats de references specifiques pour Linux
            if zip_encoding_recovery:
                assert forced_context is None
                windows_results_ref_dir, _ = results.get_results_ref_dir(test_dir)
                linux_context = [results.get_context_computing_type(), "Linux"]
                linux_results_ref_dir, _ = results.get_results_ref_dir(
                    test_dir, forced_context=linux_context
                )
                zip_encoding_recovery = windows_results_ref_dir != linux_results_ref_dir

        # Comparaison des resultats de test avec ceux de reference sous linux
        if zip_encoding_recovery:
            results_ref_dir = os.path.join(test_dir, linux_results_ref_dir)
            assert linux_context is not None
            # Comparaison "pragmatique" entre les fichiers des repertoires de test et de reference
            # en forcant le contexte, sans tentative de recuperation d'erreur avancee
            zip_encoding_recovery = check_results(
                test_dir, forced_context=linux_context
            )

        # Recuperation effective des erreurs si possible
        if zip_encoding_recovery:
            # Messages sur la recuperation
            recovery_summary = (
                "Recovery from poor handling of accented file names by zip"
            )
            recovery_message = utils.append_message(recovery_message, recovery_summary)
            utils.write_message("\n" + recovery_summary + ":", log_file=log_file)
            utils.write_message(
                "\tcomparison for Windows test results is performed using Linux reference results",
                log_file=log_file,
            )
            utils.write_message(
                "\t" + str(error_number) + " errors  converted to warnings",
                log_file=log_file,
            )
            # On transforme les erreur en warning
            warning_number += error_number
            error_number = 0
            # On reinitialise egalement les stats d'erreur
            for extension in error_number_per_extension:
                error_number_per_extension[extension] = 0
            for file_name in kht.SPECIAL_ERROR_FILES:
                special_error_file_error_numbers[file_name] = 0

    # Message dedies aux fichiers speciaux
    special_error_file_message = ""
    for file_name in kht.SPECIAL_ERROR_FILES:
        if special_error_file_error_numbers[file_name] > 0:
            special_error_file_message = SUMMARY_SPECIAL_FILE_KEYS_PER_FILE[file_name]
            break

    # Ecriture d'un resume synthetique
    utils.write_message("\n" + SUMMARY_TITLE, log_file=log_file)
    utils.write_message(
        str(warning_number) + " " + SUMMARY_WARNING_KEY, log_file=log_file
    )
    utils.write_message(str(error_number) + " " + SUMMARY_ERROR_KEY, log_file=log_file)
    if special_error_file_message != "":
        utils.write_message(special_error_file_message, log_file=log_file)
    if error_number > 0:
        # Tri des extensions
        file_extensions = []
        for file_extension in error_number_per_extension:
            file_extensions.append(file_extension)
        file_extensions.sort()
        # Message specifique si erreurs dans un seul type de fichier
        if error_number_in_err_txt > 0:
            extension_message = utils.append_message(extension_message, kht.ERR_TXT)
            if error_number_in_err_txt == error_number:
                specific_message = utils.append_message(
                    specific_message, "errors only in " + kht.ERR_TXT
                )
        if len(file_extensions) > 0:
            for file_extension in file_extensions:
                extension_message = utils.append_message(
                    extension_message, file_extension
                )
                if error_number_per_extension[file_extension] == error_number:
                    specific_message = utils.append_message(
                        specific_message, "errors only in " + file_extension + " files"
                    )
        # Ecriture des messages additionnels
        if extension_message != "":
            utils.write_message(
                SUMMARY_FILE_TYPES_KEY + extension_message, log_file=log_file
            )
        if specific_message != "":
            utils.write_message(SUMMARY_NOTE_KEY + specific_message, log_file=log_file)

    # Ecriture d'un message additionnel lie a la portabilite
    portability_message = utils.append_message(portability_message, recovery_message)
    if portability_message != "":
        utils.write_message(
            SUMMARY_PORTABILITY_KEY + portability_message, log_file=log_file
        )

    # Affichage d'un message de fin sur la console si le contexte n'est pas force
    if forced_context is None:
        final_message = "--Comparison done : "
        final_message += str(compared_files_number) + " files(s) compared, "
        final_message += str(error_number) + " error(s), "
        final_message += str(warning_number) + " warning(s)"
        if special_error_file_message != "":
            final_message += ", " + special_error_file_message
        if recovery_message != "":
            final_message += ", Recovery from errors"
        print(final_message)
        print("  log file: " + log_file_path + "\n")
    return error_number == 0


def is_file_with_json_extension(file_path):
    """Test si le path d'un fichier correspond a un fichier json"""
    # Recherche du fichier compare et de son extension
    file_name = os.path.basename(file_path)
    _, file_extension = os.path.splitext(file_name)

    # Extension json de base
    json_file_extensions = [".json", ".khj", ".khvj", ".khcj", ".kdicj"]
    # On rajoute les extension en les suffisant par "bad" pour permettre
    # de gerer des tests de fichier corrects avec une extension erronnee
    for extension in json_file_extensions.copy():
        json_file_extensions.append(extension + "bad")
    is_json_file = file_extension in json_file_extensions
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

    def clean_message(message):
        """Nettoyage d'une ligne de message, entre '"' et potentiellement suivi d'une ','
        Cela ne gere pas tous les cas d'encodage json, mais cela est suffisant la plupart du temps
        """
        cleaned_message = message.strip()
        # Cas d'un milieur de section, avec ',' en fin de ligne
        if cleaned_message[-1] == ",":
            cleaned_message = cleaned_message[1:-2]
        # Cas d'une fin de section
        else:
            cleaned_message = cleaned_message[1:-1]
        return cleaned_message

    # Recherche des lignes du fichier dans les sections "messages"
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
                line = clean_message(line)
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
    """Retourne la sous_liste des lignes stripees de la liste en entree"""
    striped_lines = []
    for line in lines:
        if is_line_striped(line):
            striped_lines.append(line)
    return striped_lines


def filter_sequential_messages_lines(lines, log_file=None):
    """Filtrage des errors et warning sequentiel d'un ensemble de lignes

    En sequentiel, de nouveaux messages de type 100th ou ...
    sont emis, alors qu'il sont absents en parallele
    En les filtrant, on rend les versions sequentielle et parallele comparables
    Retourne les ligne filtrees, avec un message dans le log sur le nombre de lignes filtrees
    """

    def is_specific_line_pair_sequential(line1, line2):
        """Test si une paire de lignes correspond a un pattern de message sequentiel
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
        utils.write_message(
            "Specific sequential messages (100th...): "
            + str(filtered_line_number)
            + " lines filtered",
            log_file=log_file,
        )
    return result_lines


""" Liste de motifs pour lesquels ont admet une variation normale s'il font parti de la comparaison
 dans une paire de lignes. Dans ce cas, on ignore la comparaison
"""
RESILIENCE_USER_MESSAGE_PATTERNS = [
    [
        "system resources are not sufficient to run the task (need ",
        " of additional memory)",
    ],
    [
        "error : ",
        "Database basic stats ",
        "Too much memory necessary to store the values of the target variable ",
        " (more than ",
    ],
    [
        "warning : Evaluation Selective Naive Bayes : Not enough memory to compute the exact AUC: "
        + "estimation made on a sub-sample of size "
    ],
    [
        "warning : Database ",
        ": Record ",
        " : Single instance ",
        "uses too much memory (more than ",
        " after reading ",
        " secondary records ",
    ],
    ["error : ", " : Not enough memory "],
]


def check_file_lines(
    ref_file_path: str,
    test_file_path: str,
    ref_file_lines,
    test_file_lines,
    log_file=None,
):
    """
    Comparaison d'un fichier de test et d'un fichier de reference
    Parametres:
    - ref_file_path: chemin du fichier de reference
    - test_file_path: chemin du fichier de test
    - ref_file_lines: liste des lignes du fichier de reference
    - test_file_lines: liste des lignes du fichier de test
    - log file: fichier de log ouvert dans le quel des messages sont ecrits (seulement si log_file est specifie)

    Retourne
    - errors: nombre d'erreurs
    - warnings: nombre de warnings
    - user_message_warnings: nombre de warnings lie a une tolerance sur la variation des messages utilisateurs
      (ex: "too much memory")

    Les noms des fichiers en parametre permettent de specialiser les comparaisons selon le type de fichier
    Les listes de lignes en entree permettent d'eviter de relire un fichier dont on connait le nom
    et dont on a deja lu les lignes.
    Cela permet par exemple de reutiliser les methodes de comparaison apres avoir filtre le fichier
    de sous-parties que l'on ne souhaite pas comparer.

    Compare les fichiers ligne par ligne, champ par champ (separateur '\t'), et token par token
    dans le cas des fichiers json ou dictionnaire
    On a avec des tolerances selon le type de fichier.
    Pour les valeurs numeriques, une difference relative de 0.00001 est toleree
    - ecrit les difference dans le fichier log_file et affiche le nb d'erreur dans le terminal
    - warning : 2 champs contiennent des valeurs numeriques avec une difference relative toleree
    - error : les champs sont differents
    """

    def filter_time(value):
        # Suppression d'un pattern de time d'une valeur
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
            pos = 0
            while pos < len(filename):
                c = filename[pos]
                if c != "_" and not c.isdigit():
                    filtered_filename += c
                else:
                    filtered_filename += "XXX"
                    while pos < len(filename):
                        c = filename[pos]
                        if c != "_" and not c.isdigit():
                            filtered_filename += c
                            break
                        pos += 1
                pos += 1
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
    is_time_file = file_name == kht.TIME_LOG

    # test si fichier histogramme
    is_histogram_file = "histogram" in file_name and file_extension == ".log"

    # test si fichier d'erreur
    is_error_file = file_name == kht.ERR_TXT

    # test si fichier de benchmark
    is_benchmark_file = file_name == "benchmark.xls"

    # Test si fichier json
    is_json_file = is_file_with_json_extension(file_name)

    # initialisation des nombres d'erreurs et de warning
    errors = 0
    warnings = 0
    numerical_warnings = 0  # Lie a une tolerance dee difference de valeur numerique
    user_message_warnings = (
        0  # Lie a un pattern de message avec tolerance (ex: "Not enough memory")
    )

    # Pas de controle si fichier de temps
    if is_time_file:
        utils.write_message("OK", log_file=log_file)
        return errors, warnings, user_message_warnings

    # Comparaison des nombres de lignes
    file_ref_line_number = len(ref_file_lines)
    file_test_line_number = len(test_file_lines)
    if file_test_line_number != file_ref_line_number:
        utils.write_message(
            "test file has "
            + str(file_test_line_number)
            + " lines and reference file has "
            + str(file_ref_line_number)
            + " lines",
            log_file=log_file,
        )
        errors = errors + 1

    # comparaison ligne a ligne
    max_threshold = 0
    max_print_error = 10
    max_field_length = 100
    skip_benchmark_lines = False
    line_number = min(file_ref_line_number, file_test_line_number)
    for index in range(line_number):
        line = index + 1
        line_ref = ref_file_lines[index].rstrip()
        line_test = test_file_lines[index].rstrip()

        # Cas special des fichiers de benchmark:
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

        # Cas special du fichier d'erreur: on tronque les lignes qui font du reporting de temps de calcul (" time:")
        if (
            is_error_file
            and line_ref.find(" time: ") != -1
            and line_test.find(" time: ") != -1
        ):
            line_ref = filter_time(line_ref)
            line_test = filter_time(line_test)

        # Cas special du fichier d'erreur: on tronque les lignes de stats sur les records des tables
        if is_error_file:
            record_stats_pattern = ["  Table ", " Records: "]
            if (
                utils.find_pattern_in_line(line_ref, record_stats_pattern) == 0
                and utils.find_pattern_in_line(line_test, record_stats_pattern) == 0
            ):
                line_ref = line_ref[: line_ref.find(record_stats_pattern[-1])]
                line_test = line_test[: line_test.find(record_stats_pattern[-1])]

        # Cas special du fichier d'erreur:
        # on saute les lignes qui font du reporting de temps de calcul ("interrupted ")
        if (
            is_error_file
            and line_ref.lower().find(" interrupted ") != -1
            and line_test.lower().find(" interrupted ") != -1
        ):
            continue

        # Cas special du fichier d'erreur, pour le message "(Operation canceled)" qui n'est pas case sensitive
        if is_error_file or is_json_file:
            if line_ref.find("(Operation canceled)") != -1:
                line_ref = line_ref.replace(
                    "(Operation canceled)", "(operation canceled)"
                )
            if line_test.find("(Operation canceled)") != -1:
                line_test = line_test.replace(
                    "(Operation canceled)", "(operation canceled)"
                )

        # Cas special du fichier d'erreur en coclustering:
        # on saute les lignes d'ecriture de rapport intermediaire qui different par le temps
        # ("Write intermediate coclustering report")
        if (
            is_error_file
            and line_ref.find("Write intermediate coclustering report") != -1
            and line_test.find("Write intermediate coclustering report") != -1
        ):
            continue

        # Cas special du fichier d'histogramme:
        # on tronque les lignes qui font du reporting de temps de calcul (" time\t")
        if (
            is_histogram_file
            and line_ref.find("time") != -1
            and line_test.find("time") != -1
        ):
            line_ref = line_ref[: line_ref.find("time")]
            line_test = line_test[: line_test.find("time")]
        # Cas special du fichier d'histogramme:
        # on ignore les ligne avec le numero de version
        if (
            is_histogram_file
            and line_ref.find("Version") != -1
            and line_test.find("Version") != -1
        ):
            continue

        # Cas special du caractere # en tete de premiere ligne de fichier
        # pour l'identifiant de version d'application (ex: #Khiops 10.2.0)
        tool_version_pattern = ["#", " "]
        if (
            line == 1
            and utils.find_pattern_in_line(line_ref, tool_version_pattern) == 0
            and utils.find_pattern_in_line(line_test, tool_version_pattern) == 0
        ):
            continue

        # Cas special du champ version des fichiers json (identifiant de version d'application)
        if (
            is_json_file
            and line_ref.find('"version": ') >= 0
            and line_test.find('"version": ') >= 0
        ):
            continue

        # Traitement des patterns toleres pour la comparaison
        if is_error_file or is_json_file:
            resilience_found = False
            for pattern in RESILIENCE_USER_MESSAGE_PATTERNS:
                if (
                    utils.find_pattern_in_line(line_ref, pattern) != -1
                    and utils.find_pattern_in_line(line_test, pattern) != -1
                ):
                    # On renvoie un warning, en indiquant qu'il s'agit d'un warning de resilience
                    warnings += 1
                    user_message_warnings += 1
                    # Ecriture d'un warning
                    utils.write_message(
                        "warning : line "
                        + str(line)
                        + " "
                        + line_test.strip()
                        + " -> "
                        + line_ref.strip(),
                        log_file=log_file,
                    )
                    resilience_found = True
                    break
            if resilience_found:
                continue

        # Sinon, on analyse les champs
        line_fields_ref = line_ref.split("\t")
        line_fields_test = line_test.split("\t")

        # comparaison des nombres de champs
        field_number_ref = len(line_fields_ref)
        field_number_test = len(line_fields_test)
        if field_number_ref != field_number_test:
            if errors < max_print_error:
                utils.write_message(
                    "test file (line "
                    + str(line)
                    + ") has "
                    + str(field_number_test)
                    + " columns and reference file has "
                    + str(field_number_ref)
                    + " columns",
                    log_file=log_file,
                )
            elif errors == max_print_error:
                utils.write_message("...", log_file=log_file)
            errors = errors + 1

        # comparaison des champs
        field_number_length = min(field_number_ref, field_number_test)
        for i in range(field_number_length):
            field_ref = line_fields_ref[i]
            field_test = line_fields_test[i]

            # parcours des lignes champ par champs
            # cas special du fichier d'erreur ou json: on tronque les chemins vers les repertoires temporaires de Khiops
            if (
                (is_error_file or is_json_file)
                and field_ref.find("~Khiops") != -1
                and field_test.find("~Khiops") != -1
            ):
                field_ref = filter_khiops_temp_dir(field_ref)
                field_test = filter_khiops_temp_dir(field_test)

            # cas general de comparaison de champs
            [eval_res, threshold_res] = check_field(field_ref, field_test)

            # truncature des champs affiches dans les messages d'erreur
            if len(field_test) > max_field_length:
                field_test = field_test[0:max_field_length] + "..."
            if len(field_ref) > max_field_length:
                field_ref = field_ref[0:max_field_length] + "..."
            # messages d'erreur
            if eval_res == 0:
                if errors < max_print_error or threshold_res > max_threshold:
                    utils.write_message(
                        "line "
                        + str(line)
                        + " field "
                        + str(i + 1)
                        + " "
                        + field_test
                        + " -> "
                        + field_ref,
                        log_file=log_file,
                    )
                elif errors == max_print_error:
                    utils.write_message("...", log_file=log_file)
                errors += 1
            elif eval_res == 2:
                warnings += 1
                if threshold_res > 0:
                    numerical_warnings += 1
            max_threshold = max(threshold_res, max_threshold)
    if warnings > 0:
        if numerical_warnings > 0:
            utils.write_message(
                str(numerical_warnings) + " warning(s) (epsilon difference)",
                log_file=log_file,
            )
        if user_message_warnings > 0:
            utils.write_message(
                str(user_message_warnings)
                + " warning(s) (resilience to specific user message patterns)",
                log_file=log_file,
            )
    if errors == 0:
        utils.write_message("OK", log_file=log_file)
    if errors > 0:
        message = str(errors) + " error(s)"
        if max_threshold > 0:
            message += " (max relative difference: " + str(max_threshold) + ")"
        utils.write_message(message, log_file=log_file)
    return errors, warnings, user_message_warnings


def split_field(field_value):
    """Decoupage d'un champ (champ d'une ligne avec separateur tabulation)
    en un ensemble de tokens elementaire pour le parsing d'un fichier json ou kdic
    Permet ensuite de comparer chaque valeur de token, pour avoir une tolerance par rapport aux
    mirco-variations des valeurs numeriques"""
    # Pour gerer les double-quotes a l'interieur des strings, pour les format json et kdic
    field_value = field_value.replace('\\"', "'")
    field_value = field_value.replace('""', "'")
    sub_fields = TOKEN_PARSER.findall(field_value)
    return sub_fields


def is_time(val):
    """Indique si une valeur est de type temps hh:mm:ss.ms"""
    return TIME_PARSER.match(val.strip())


def check_value(val1, val2):
    """Comparaison de deux valeurs numeriques
    Renvoie deux valeur:
    - result:
      - 1 si les valeurs sont identiques
      - 2 si les la difference relative est toleree
      - 0 si les valeurs sont differentes
    - threshold: difference relative si result = 2
    """
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


def check_field(field1, field2):
    """ " Comparaison de deux champs
    Pour les valeurs numeriques, une diffence relative de 0.00001 est toleree
    Renvoie deux valeur:
    - result:
      - 1 si les champs sont identiques
      - 2 si les la difference relative est toleree (warning)
      - 0 si les champs sont differents (error)
    - threshold: difference relative liee au cas erreur ou warning
    """
    if field1 == field2:
        return [1, 0]

    # si les deux champs sont des time, on renvoie OK pour ignorer la comparaison
    if is_time(field1) and is_time(field2):
        return [1, 0]

    # uniformisation entre windows et linux pour les chemins de fichier
    # on va remplacer les \ par des /
    string1 = field1.replace("\\", "/")
    string2 = field2.replace("\\", "/")
    # Tolerance temporaire pour le passage au format hdfs
    # hdfs_value1 = field1.replace("./", "")
    # hdfs_value1 = hdfs_value1.replace(".\\/..\\/", "")
    # hdfs_value1 = hdfs_value1.replace("..\\/", "")
    # hdfs_value1 = hdfs_value1.replace(".\\/", "")
    # hdfs_value2 = field2.replace("./", "")
    # hdfs_value2 = hdfs_value2.replace(".\\/..\\/", "")
    # hdfs_value2 = hdfs_value2.replace("..\\/", "")
    # hdfs_value2 = hdfs_value2.replace(".\\/", "")
    # if hdfs_value1 == hdfs_value2:
    #     return [1, 0]
    if string1 == string2:
        return [1, 0]

    # sinon c'est peut etre un probleme d'arrondi
    # on accepte les differences relatives faibles
    if NUMERIC_PARSER.match(field1) and NUMERIC_PARSER.match(field2):
        [eval_result, threshold_result] = check_value(field1, field2)
        return [eval_result, threshold_result]
    else:
        # on arrive pas a le convertir en float, ce n'est pas un nombre
        # on decoupe chaque champ sous la forme d'un ensemble de sous-chaines qui sont soit
        # des libelles, soit des float
        sub_fields1 = split_field(field1)
        sub_fields2 = split_field(field2)

        # nombre de sous-chaines differentes: il y a erreur
        if len(sub_fields1) != len(sub_fields2):
            return [0, 0]
        # comparaison pas a pas
        else:
            i = 0
            length = len(sub_fields1)
            warnings = 0
            errors = 0
            max_warning_threshold = 0
            max_error_threshold = 0
            while i < length:
                [eval_result, threshold_result] = check_value(
                    sub_fields1[i], sub_fields2[i]
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


def initialize_parsers():
    """Initialisation de parsers sont compile une fois pour toutes
    Retourne les parsers de token, de numeric et de time
    """
    # Delimiters pour les fichiers json et kdic
    delimiters = [
        "\\,",
        "\\{",
        "\\}",
        "\\[",
        "\\]",
        "\\:",
        "\\(",
        "\\)",
        "\\<",
        "\\>",
        "\\=",
    ]
    numeric_pattern = "-?[0-9]+\\.?[0-9]*(?:[Ee]-?[0-9]+)?"
    string_pattern = (
        '"[^"]*"'  # Sans les double-quotes dans les strings (dur a parser...)
    )
    time_pattern = "\\d{1,2}:\\d{2}:\\d{2}\\.?\\d*"
    other_tokens = "[\\w]+"
    tokens = time_pattern + "|" + numeric_pattern + "|" + string_pattern
    for delimiter in delimiters:
        tokens += "|" + delimiter
    tokens += "|" + other_tokens
    token_parser = re.compile(tokens)
    numeric_parser = re.compile(numeric_pattern)
    time_parser = re.compile(time_pattern)
    return token_parser, numeric_parser, time_parser


# Parsers en variables globales, compiles une seule fois au chargement du module
# - le parser de tokens permet d'analyser de facon detaillee le contenu d'un
#   fichier json ou dictionnaire (.kdic) en le decomposant en une suite de tokens
#   separateur, valeur numerique opu categorielle entre double-quotes.
# - le parser de numerique est specialise pour les valeurs numeriques au format scientifique
# - le parser de time est specialise pour le format time hh:mm:ss.ms
TOKEN_PARSER, NUMERIC_PARSER, TIME_PARSER = initialize_parsers()
