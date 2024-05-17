import os
import os.path
import sys
import shutil
import stat
import argparse

import _kht_constants as kht
import _kht_families as test_families


"""
Fonction utilitaires, notamment pour la gestion des fichiers et des messages
"""

"""
Verification de la typologie des repertoires dans LearningTest
- test dir: repertoire d'un test elementaire (ex: `IrisLight`)
- suite dir: repertoire d'une famille de test, contenant un sous-repertoire par test (ex: Standard)
- tool dir: repertoire pour un outil, contenant un sous-repertoire par suite de test: (ex: TestKhiops)
- home dir: repertoire LearningTest, contenant les tool dirs, designe en externe par 'LearningTest dir'

Les methodes suivante verifie qu'un path, relatif ou absolu, se termine par un repertoire de la typologie.
En cas d'erreur, un message est affiche est on sort du programme
"""


def check_test_dir(checked_dir):
    """Test si un chemin est celui d'un repertoire de test"""
    checked_home_dir_path = parent_dir_path(checked_dir, 3)
    checked_tool_dir_name = parent_dir_name(checked_dir, 2)
    if (
        not check_home_dir(checked_home_dir_path)
        or checked_tool_dir_name not in kht.TOOL_DIR_NAMES.values()
    ):
        fatal_error(checked_dir + " should be a test directory of " + kht.LEARNING_TEST)
    return True


def check_suite_dir(checked_dir):
    """Test si un chemin est celui d'un repertoire de suite"""
    checked_home_dir_path = parent_dir_path(checked_dir, 3)
    checked_tool_dir_name = parent_dir_name(checked_dir, 1)
    if (
        not check_home_dir(checked_home_dir_path)
        or checked_tool_dir_name not in kht.TOOL_DIR_NAMES.values()
    ):
        fatal_error(
            checked_dir + " should be a suite directory of " + kht.LEARNING_TEST
        )
    return True


def check_tool_dir(checked_dir):
    """Test si un chemin est celui d'un repertoire d'outil"""
    checked_home_dir_path = parent_dir_path(checked_dir, 3)
    checked_tool_dir_name = parent_dir_name(checked_dir, 0)
    if (
        not check_home_dir(checked_home_dir_path)
        or checked_tool_dir_name not in kht.TOOL_DIR_NAMES.values()
    ):
        fatal_error(checked_dir + " should be a tool directory of " + kht.LEARNING_TEST)
    return True


def check_home_dir(checked_dir, fatal_error_if_false=True):
    """Test si un chemin est celui du repertoire LearningTest"""
    checked_home_dir_path = parent_dir_path(checked_dir, 0)
    # On n'impose pas que le repertoire racine ait le nom predefini kht.LEARNING_TEST
    # On verifie juste que le repertoire contient au moins un des repertoires d'outil
    for name in kht.TOOL_DIR_NAMES.values():
        checked_tool_dir_name = os.path.join(checked_home_dir_path, name)
        if os.path.isdir(checked_tool_dir_name):
            return True
    # Echec si aucun repertoire d'outil trouve
    if fatal_error_if_false:
        fatal_error(
            checked_dir
            + " should be a valid '"
            + kht.LEARNING_TEST
            + "' home dir, containing at least one the tools directory "
            + list_to_label(kht.TOOL_DIR_NAMES.values())
        )
    return False


def get_learning_test_sub_dir_depth(checked_dir):
    """Test si un chemin est correspond a un sous-repertoire de LearningTest
    Renvoie la profondeur a laquelle se trouver LearningTest
    - 0: home dir
    - 1: tool dir
    - 2: suite dir
    - 3: test dir
    """
    if not os.path.isdir(checked_dir):
        fatal_error(checked_dir + " should be a directory")
    checked_home_dir_path = os.path.realpath(checked_dir)
    depth = 0
    while depth < 4:
        if check_home_dir(checked_home_dir_path, fatal_error_if_false=False):
            return depth
        checked_home_dir_path = os.path.dirname(checked_home_dir_path)
        depth += 1
    fatal_error(
        checked_dir
        + " must be in a directory tree located a maximum of three levels above a valid '"
        + kht.LEARNING_TEST
        + "' home dir, containing at least one the tools directory "
        + list_to_label(kht.TOOL_DIR_NAMES.values())
    )


def get_home_dir(home_dir):
    """Retourne le repertoire de base LearningTest a partir d'un sous-repertoire de profondeur quelconque"""
    # On remonte dans le chemin (reel) jusqu'a trouver le repertoire racine
    checked_home_dir_path = os.path.realpath(home_dir)
    depth = 0
    while depth < 4:
        if check_home_dir(checked_home_dir_path, fatal_error_if_false=False):
            return checked_home_dir_path
        checked_home_dir_path = os.path.dirname(checked_home_dir_path)
        depth += 1
    assert False, (
        "No valid '" + kht.LEARNING_TEST + "' home dir found in path " + home_dir
    )


def test_dir_name(test_dir):
    """Nom du repertoire de test a partir du chemin repertoire de test"""
    return parent_dir_name(test_dir, 0)


def suite_dir_name(test_dir):
    """Nom du repertoire de suite a partir du chemin repertoire de test"""
    return parent_dir_name(test_dir, 1)


def tool_dir_name(test_dir):
    """Nom du repertoire d'outil a partir du chemin repertoire de test"""
    return parent_dir_name(test_dir, 2)


def dir_name(dir_path):
    """Renvoie le nom reel du repertoire de base d'un chemin, meme si le chemin est relatif"""
    return parent_dir_name(dir_path, 0)


def parent_dir_path(dir_path, depth):
    """Renvoie le chemin d'un repertoire parent a une profondeur donnee
    Le nom est le nom reel absolu, meme si le parametre en entree est un chemin relatif
    Ainsi, utiliser depth=0 permet d'avoir le nom reel du repertoire de base dans tous les cas

    Example: pour un test path dir_path=<root_path>/LearningTest/TestKhiops/Standard/Iris/.
    - test dir: parent_dir_name(dir_path, 0) -> Iris
    - suite dir: parent_dir_name(dir_path, 1) -> Standard
    - tool dir: parent_dir_name(dir_path, 2) -> TestKhiops
    - home dir: parent_dir_name(dir_path, 3) -> LearningTest
    """
    if not os.path.isdir(dir_path):
        fatal_error(dir_path + " should be a valid directory")
    # Recherche du parent avec un chemin relatif
    relative_parent_path = dir_path
    for i in range(depth):
        relative_parent_path += "/.."
    # Nom reel du chemin
    real_parent_path = os.path.realpath(relative_parent_path)
    return real_parent_path


def parent_dir_name(dir_path, depth):
    """Renvoie le nom d'un repertoire parent a une profondeur donnee
    Le nom est le nom reel absolu, meme si le parametre en entree est un chemin relatif
    Ainsi, utiliser depth=0 permet d'avoir le nom reel du repertoire de base dans tous les cas

    Example: pour un test path dir_path=<root_path>/LearningTest/TestKhiops/Standard/Iris/.
    - test dir: parent_dir_name(dir_path, 0) -> Iris
    - suite dir: parent_dir_name(dir_path, 1) -> Standard
    - tool dir: parent_dir_name(dir_path, 2) -> TestKhiops
    - home dir: parent_dir_name(dir_path, 3) -> LearningTest
    """
    parent_path = parent_dir_path(dir_path, depth)
    # On extrait le nom du repertoire
    result_name = os.path.basename(parent_path)
    return result_name


"""
Gestion des noms des binaires
"""


def is_valid_tool_full_exe_name(tool_exe_name):
    """Indique si le nom du binaire fait partie des noms valides, avec prise
    en compte des suffixes mpi pour les exe paralellisable"""

    return (
        tool_exe_name in kht.TOOL_EXE_NAMES.values()
        or extract_tool_exe_name(tool_exe_name) in kht.PARALLEL_TOOL_NAMES
    )


def extract_tool_exe_name(tool_full_exe_name):
    """Extrait le nom du binaire a partir d'un nom ayant potentiellement un suffixe mpi"""

    if tool_full_exe_name in kht.TOOL_EXE_NAMES.values():
        return tool_full_exe_name
    for suffix in kht.TOOL_MPI_SUFFIXES:
        if tool_full_exe_name.endswith(suffix):
            return tool_full_exe_name.removesuffix(suffix)


"""
Gestion des messages utilisateurs
"""


def fatal_error(message):
    """Affichage du message lie a l'erreur puis quit"""
    print("error : " + message)
    exit(1)


def write_message(message, log_file=None, show=False):
    """Ecriture d'un message dans un fichier de log
    Ecriture dans un fichier de log selon le le parametre log_file
    Affichage sur la console selon le parametre show
    Si ni log_file, ni show ne sont specifier, la methode est en mode silencieux
    """
    cleaned_message = message.encode(encoding="utf-8", errors="ignore").decode(
        encoding="utf-8"
    )
    if show:
        print(cleaned_message)
    # on encode en utf-8 en ignorant les erreurs pour eviter un erreur lors de l'encodage automatique
    if log_file is not None:
        log_file.write(cleaned_message + "\n")


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


def list_to_label(value_list):
    """Creation d'un libelle a partir de l'ensemble des valeurs d'une liste
    sous la forme '(value1, value2..., valuek)'"""
    label = ""
    for value in value_list:
        if label != "":
            label += ", "
        label += str(value)
    label = "(" + label + ")"
    return label


"""
Gestion des arguments commun de la ligne de commande, pour la librairie argparse
"""


def get_formatter_class(script_name):
    """Variante de la classe de formatage a utiliser pour la creation des ArgumentParser
    - pour montrer une option par ligne, en indentant legerement l'espace des noms des options
    - pour permettre des aide multi-lignes, notamment dans l'epilogue
    (je ne sais pas pourquoi il faut pas passer par un lambda expression: pas grave)
    """
    argument_parser_formatter_class = lambda prog: argparse.RawTextHelpFormatter(
        script_name, max_help_position=26
    )
    return argument_parser_formatter_class


def argument_parser_add_source_argument(parser):
    """Ajout de l'argument source, sous repertoire dans l'arborescence LearningTest"""
    parser.add_argument(
        "source",
        help="source directory, sub-dir in a " + kht.LEARNING_TEST + " tree",
    )


def argument_parser_add_dest_argument(parser):
    """Ajout de l'argument dest, repertoire devant contenir une sous-partie d'une arborescence LearningTest"""
    parser.add_argument(
        "dest",
        help="destination directory that contains the output "
        + kht.LEARNING_TEST
        + " tree",
    )


def argument_parser_add_family_argument(parser):
    """Ajout de l'argument de famile de suites"""
    parser.add_argument(
        "-f",
        "--family",
        help="family of test suites among "
        + ", ".join(test_families.TEST_FAMILIES)
        + " (default: "
        + test_families.FULL
        + ")",
        choices=test_families.TEST_FAMILIES,
        default=test_families.FULL,
        metavar="name",
        action="store",
    )


def argument_parser_add_processes_argument(parser):
    """Ajout de l'argument du nombre de process"""
    parser.add_argument(
        "-p",
        "--processes",
        help="number of processes (default: 1)",
        dest="n",
        type=int,
        default=1,
        metavar="n",
        action="store",
    )


def argument_parser_add_forced_platform_argument(parser):
    """Ajout de l'argument de plateforme forcee en remplacement de la plateforme courante"""
    parser.add_argument(
        "--forced-platform",
        help="platform "
        + list_to_label(kht.RESULTS_REF_TYPE_VALUES[kht.PLATFORM])
        + " used to compare results (default: current platform)",
        choices=kht.RESULTS_REF_TYPE_VALUES[kht.PLATFORM],
        metavar="plt",
        action="store",
    )


def argument_parser_add_limit_test_time_arguments(parser):
    """Ajout des argument de limite des temps de test"""
    parser.add_argument(
        "--min-test-time",
        help="only for test dirs where reference test time (in file "
        + kht.TIME_LOG
        + ") is beyond a threshold",
        type=float,
        metavar="t",
        action="store",
    )
    parser.add_argument(
        "--max-test-time",
        help="only for test dirs where reference test time (in file "
        + kht.TIME_LOG
        + ") is below a threshold",
        type=float,
        metavar="t",
        action="store",
    )


def argument_parser_check_source_argument(parser, source):
    """Verification de l'argument source, a appeler apres les verification standard de parse_args()
    On renvoie la decomposition du repertoire source sous la forme des champs:
    home_dir, tool_dir_name, suite_dir_name, test_dir_name
    """
    # Verification du repertoire a tester
    learning_test_depth = get_learning_test_sub_dir_depth(source)
    source_tool_dir_name = None
    source_suite_dir_name = None
    source_test_dir_name = None
    if learning_test_depth == 3:
        source_tool_dir_name = parent_dir_name(source, 2)
        source_suite_dir_name = parent_dir_name(source, 1)
        source_test_dir_name = parent_dir_name(source, 0)
    elif learning_test_depth == 2:
        source_tool_dir_name = parent_dir_name(source, 1)
        source_suite_dir_name = parent_dir_name(source, 0)
    elif learning_test_depth == 1:
        source_tool_dir_name = parent_dir_name(source, 0)
    if (
        source_tool_dir_name is not None
        and source_tool_dir_name not in kht.TOOL_DIR_NAMES.values()
    ):
        parser.error(
            "argument source: "
            + source_tool_dir_name
            + " in "
            + os.path.realpath(source)
            + " should be a tool dir "
            + list_to_label(kht.TOOL_DIR_NAMES.values())
        )
    source_home_dir = get_home_dir(source)
    return (
        source_home_dir,
        source_tool_dir_name,
        source_suite_dir_name,
        source_test_dir_name,
    )


def argument_parser_check_destination_dir(parser, source_home_dir, destination_dir):
    """Test si un chemin peut servir a about a un repertoire racine
    Il ne doit pas etre un sous-repertoire du repertoire de base"""
    check_home_dir(source_home_dir)
    home_dir = os.path.realpath(source_home_dir)
    target_dir = os.path.realpath(destination_dir)
    if target_dir.find(home_dir) >= 0:
        parser.error(
            "argument dest: "
            "destination dir "
            + destination_dir
            + " must not in the directory tree "
            + home_dir
        )


def argument_parser_check_processes_argument(parser, processes):
    """Verification de l'argument processes, a appeler apres les verification standard de parse_args()"""
    max_process_number = 128
    if processes < 1:
        parser.error("argument -p/--processes: min value is 1")
    elif processes > max_process_number:
        parser.error("argument -p/--processes: max value is " + str(max_process_number))


def argument_parser_check_limit_test_time_arguments(
    parser, min_test_time, max_test_time
):
    """Verification des arguments de limites des temps de test,
    a appeler apres les verification standard de parse_args()"""
    if min_test_time is not None and min_test_time < 0:
        parser.error("argument --min-test-time must be positive")
    if max_test_time is not None and max_test_time < 0:
        parser.error("argument --max-test-time must be positive")


"""
Gestion du contenu d'un fichier
"""


def read_file_lines(file_path, log_file=None, show=False):
    """Chargement en memoire des lignes d'un fichier
    Retourne la liste des fichiers si ok, None sinon
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


"""
Gestion de patterns dans un fichier
Un patterns est une liste de sous-chaines devant se trouver en sequance dans une ligne
Exemples:
- la ligne "warning : unable to open file" contient le pattern elementaire ["warning"]
  et le pattern complexe ["warning", "file"]
"""


def find_pattern_in_line(line, pattern):
    """Renvoie la position de la premiere sous-chaine d'un pattern si une ligne contient un pattern
    Retourne -1 sinon"""
    assert isinstance(pattern, list)
    pos = 0
    first_pos = None
    for sub_pattern in pattern:
        pos = line[pos:].find(sub_pattern)
        if first_pos is None:
            first_pos = pos
        if pos == -1:
            return -1
    if first_pos is None:
        return -1
    else:
        return first_pos


def find_pattern_in_lines(lines, pattern):
    """Recherche d'un pattern un ensemble de lignes
    Renvoie l'index de la premiere ligne contenant le pattern, -1 sinon"""
    assert isinstance(lines, list)
    assert isinstance(pattern, list)
    for i, line in enumerate(lines):
        if find_pattern_in_line(line, pattern) != -1:
            return i
    return -1


def filter_lines_with_pattern(lines, pattern):
    """Retourne les lignes sans celles contenant le pattern en parametre"""
    assert isinstance(lines, list)
    assert isinstance(pattern, list)
    output_lines = []
    for line in lines:
        if find_pattern_in_line(line, pattern) == -1:
            output_lines.append(line)
    return output_lines


def filter_copyright_lines(lines):
    """Retourne les lignes sans les lignes de copyright, presentes en mode UI"""
    assert isinstance(lines, list)
    output_lines = lines
    is_copyright = False
    if len(lines) >= 2:
        is_copyright = (
            find_pattern_in_line(lines[1], ["(c)", "Orange - All rights reserved."])
            != -1
        )
    if is_copyright:
        output_lines = lines[2:]
    return output_lines


def filter_process_id_prefix_from_lines(lines):
    """Retourne les lignes sans l'eventuel prefixe de process id, du type '[0] '
    qui est emis par mpiexce dans les sorties standard"""
    output_lines = []
    for line in lines:
        # En parallelle, une ligne vide peut contenir le numero du process entre crochets
        pos_end = -1
        is_process_id = len(line) > 0 and line[0] == "["
        if is_process_id:
            pos_end = line.find("]")
            is_process_id = pos_end > 0 and line[1:pos_end].isdigit()
        if is_process_id:
            line = line[pos_end + 1 :].lstrip()
        output_lines.append(line)
    return output_lines


def filter_empty_lines(lines):
    """Retourne les lignes sans les lignes vides"""
    output_lines = []
    for line in lines:
        if line.strip() != "":
            output_lines.append(line)
    return output_lines


"""
Gestion des fichiers et repertoires
"""


def copy_file(src_file_path, dest_file_path):
    """Copie d'un fichier, avec message d'erreur"""
    try:
        shutil.copy(src_file_path, dest_file_path)
    except BaseException as message:
        print("can't copy " + src_file_path + " (" + str(message) + ")")


def remove_file(file_path):
    """Suppression d'un fichier, avec message d'erreur"""
    try:
        os.chmod(file_path, stat.S_IWRITE)
        os.remove(file_path)
    except (IOError, os.error) as why:
        print("Cannot remove file %s: %s" % (file_path, str(why)))


def make_dir(dest_dir):
    """Creation d'un repertoire, avec message d'erreur"""
    try:
        os.mkdir(dest_dir)
    except (IOError, os.error) as why:
        print("Cannot create directory %s: %s" % (dest_dir, str(why)))


def remove_dir(dir_to_remove):
    """Suppression d'un repertoire cense etre vide, avec message d'erreur"""
    try:
        os.rmdir(dir_to_remove)
    except (IOError, os.error) as why:
        print("Cannot remove directory %s: %s" % (dir_to_remove, str(why)))


def sub_dirs(source_dir):
    """Renvoie la liste des sous-repertoire d'un repertoire, sans message d'erreur"""
    result_sub_dirs = []
    if os.path.isdir(source_dir):
        try:
            list_dir = os.listdir(source_dir)
            for name in list_dir:
                if os.path.isdir(os.path.join(source_dir, name)):
                    result_sub_dirs.append(name)
        except (IOError, os.error):
            pass
    return result_sub_dirs


def set_flushed_outputs():
    """Flush systematique des sorties standard et d'erreur"""
    sys.stdout = Unbuffered(sys.stdout)
    sys.stderr = Unbuffered(sys.stderr)


class Unbuffered:
    """Pour ouvrir un fichier avec un flush systematique
    usage: par exemple, appeler sys.stdout = Unbuffered(sys.stdout) pour que toutes les sorties standard
     soit immediatement affichees dans le shell, sans bufferisation
    """

    def __init__(self, stream):
        self.stream = stream

    def write(self, data):
        # on encode en utf-8 en ignorant les erreurs pour eviter un erreur lors de l'encodage automatique
        self.stream.write(data.encode("utf-8", "ignore").decode("utf-8"))
        self.stream.flush()

    def writelines(self, datas):
        # on encode en utf-8 en ignorant les erreurs pour eviter un erreur lors de l'encodage automatique
        self.stream.writelines(
            [data.encode("utf-8", "ignore").decode("utf-8") for data in datas]
        )
        self.stream.flush()

    def __getattr__(self, attr):
        return getattr(self.stream, attr)
