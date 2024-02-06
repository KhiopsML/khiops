import os
import os.path
import shutil
import stat

import _learning_test_constants as lt


"""
Fonction utilitaires, notamment pour la gestion des fichiers et des messages
"""


"""
Verification de la typologie des repertoires dans LearningTest
- test dir: repertoire d'un test elementaire (ex: `IrisLight`)
- suite dir: repertoire d'une famille de test, contenant un sous-repertoire par test (ex: Standard)
- tool dir: repertoire pour un outil, contenant un sous-repertoire par suite de test: (ex: TestKhiops)
- home dir: repertoire LearningTest, contenant les tool dirs
- root dir: repertoire contenant le home dir

Les methodes suivante verifie qu'un path, relatif ou absolu, se termine par un repertoire de la typologie.
En cas d'erreur, un message est affiche est on sort du programme
"""


def check_test_dir(checked_dir):
    """Test si un chemin est celui d'un repertoire de test"""
    home_dir = parent_dir_name(checked_dir, 3)
    tool_dir = parent_dir_name(checked_dir, 2)
    if home_dir != lt.LEARNING_TEST or tool_dir not in lt.TOOL_DIR_NAMES.values():
        fatal_error(checked_dir + " should be a test directory of " + lt.LEARNING_TEST)


def check_suite_dir(checked_dir):
    """Test si un chemin est celui d'un repertoire de suite"""
    home_dir = parent_dir_name(checked_dir, 2)
    tool_dir = parent_dir_name(checked_dir, 1)
    if home_dir != lt.LEARNING_TEST or tool_dir not in lt.TOOL_DIR_NAMES.values():
        fatal_error(checked_dir + " should be a suite directory of " + lt.LEARNING_TEST)


def check_tool_dir(checked_dir):
    """Test si un chemin est celui d'un repertoire d'outil"""
    home_dir = parent_dir_name(checked_dir, 1)
    tool_dir = parent_dir_name(checked_dir, 0)
    if home_dir != lt.LEARNING_TEST or tool_dir not in lt.TOOL_DIR_NAMES.values():
        fatal_error(checked_dir + " should be a tool directory of " + lt.LEARNING_TEST)


def check_home_dir(checked_dir):
    """Test si un chemin est celui du repertoire de base"""
    home_dir = parent_dir_name(checked_dir, 0)
    if home_dir != lt.LEARNING_TEST or tool_dir not in lt.TOOL_DIR_NAMES.values():
        fatal_error(
            checked_dir + " should be the home directory of " + lt.LEARNING_TEST
        )


def check_candidate_root_dir(source_home_dir, candidate_target_root_dir):
    """Test si un chemin peut servir a about a un repertoire racine
    Il ne doit pas etre un sous-repertoire du repertoire de base"""
    check_home_dir(source_home_dir)
    if not os.path.isdir(candidate_target_root_dir):
        fatal_error(candidate_target_root_dir + " should be a valid directory")
    home_dir = os.path.realpath(source_home_dir)
    root_dir = os.path.realpath(candidate_target_root_dir)
    if root_dir.find(home_dir):
        fatal_error(
            candidate_target_root_dir
            + " must not be a sub_directory of "
            + source_home_dir
        )


def get_home_dir(learning_test_dir):
    """Retourne le repertoire de base LearningTest a partir d'un sous-repertoire de profondeur quelconque"""
    assert lt.LEARNING_TEST in learning_test_dir
    # On remonte dans le chemin (reel) jusqu'a trouver le repertoire racine
    home_dir = os.path.realpath(learning_test_dir)
    while os.path.basename(home_dir) != lt.LEARNING_TEST:
        home_dir = os.path.dirname(home_dir)
    return home_dir


def get_root_dir(learning_test_dir):
    """Retourne le repertoire contenant LearningTest a partir d'un sous-repertoire de profondeur quelconque"""
    assert lt.LEARNING_TEST in learning_test_dir
    home_dir = get_home_dir(learning_test_dir)
    root_dir = os.path.dirname(home_dir)
    return root_dir


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
    if not os.path.isdir(dir_path):
        fatal_error(dir_path + " should be a valid directory")
    # Recherche du parent avec un chemin relatif
    relative_parent_path = dir_path
    for i in range(depth):
        relative_parent_path += "/.."
    # Nom reel du chemin
    real_parent_path = os.path.realpath(relative_parent_path)
    # On extrait le nom du repertoire
    parent_dir = os.path.basename(real_parent_path)
    return parent_dir


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
Gestion de la valeur des variables d'environnement
"""


def get_env_var_positive_value(env_var_name, is_int=False):
    """Retourne la valeur numerique d'une variable d'environnement representant une valeur positive
    Renvoie None si la variable n'est pas definie
    Sort du programme avec une erreur si elle ne correspond pas a une valeur numerique positive
    """
    value = os.getenv(env_var_name)
    if value is not None:
        try:
            if is_int:
                value = int(value)
            else:
                value = float(value)
            if value < 0:
                raise ValueError("should be positive")
        except ValueError as valueException:
            value = None
            fatal_error(
                "env var "
                + env_var_name
                + " ("
                + str(os.getenv(env_var_name))
                + ") : "
                + str(valueException),
            )
    return value


def get_env_var_boolean_value(env_var_name, default_value):
    """Retourne la valeur booleenn d'une variable d'environnement
    Renvoie:
    - True si la valeur est "true"
    - False si la valeur est "false"
    - la valeur par defaut sinon
    """
    value = os.getenv(env_var_name)
    if value is not None:
        if value.lower() == "true":
            return True
        elif value.lower() == "false":
            return False
        else:
            fatal_error(
                "env var "
                + env_var_name
                + " ("
                + str(os.getenv(env_var_name))
                + ") : should be 'false' or 'true'",
            )
    return default_value


"""
Gestion du contenu d'un fichier
"""


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


"""
Gestion de patterns dans un fichier
Un patterns est une liste de sous-chaines devant se trouver en sequance dans une ligne
Exemples:
- la ligne "warning : unable to open file" contient le pattern elementaire ["warning"]
  et le pattern complexe ["warning", "file"]
"""


def find_pattern_in_line(line, pattern):
    """Renvoie la position de la premier sous-chaine d'un pattern si une ligne contient un pattern
    Retourne -1 sinon"""
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
    for i, line in enumerate(lines):
        if find_pattern_in_line(line, pattern):
            return i
    return -1


def filter_lines_with_pattern(lines, pattern):
    """Retourne les lignes sans celles contenant le pattern en parametre"""
    output_lines = []
    for line in lines:
        if not find_pattern_in_line(line, pattern):
            output_lines.append(line)
    return output_lines


def filter_copyright_lines(lines):
    """Retourne les lignes sans les lignes de copyright, presentes en mode UI"""
    output_lines = lines
    is_copyright = False
    if len(lines) >= 2:
        is_copyright = find_pattern_in_lines(
            lines[1], ["(c)", "Orange - All rights reserved."]
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
Gestion des fichier et repertoires
"""


def copy_file(src_file_path, dest_file_path):
    """Copie d'un fichier, avec message d'eerur"""
    try:
        shutil.copy(src_file_path, dest_file_path)
    except BaseException as message:
        print("can't copy " + src_file_path + " (" + str(message) + ")")


def remove_file(file_path):
    """Supression d'un fichier, avec message d'erreur"""
    try:
        os.chmod(file_path, stat.S_IWRITE)
        os.remove(file_path)
    except (IOError, os.error) as why:
        print("Cannot remove file %s: %s" % (file_path, str(why)))


def remove_dir(dir_to_remove):
    """Supression d'un repertoire cense etre vide, avec message d'erreur"""
    try:
        os.rmdir(dir_to_remove)
    except (IOError, os.error) as why:
        print("Cannot remove directory %s: %s" % (dir_to_remove, str(why)))
