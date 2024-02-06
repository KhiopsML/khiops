import os

import _learning_test_constants as lt
import _learning_test_utils as utils


"""
Specification de l'environnement de test.
Un fichier de configuration (voir ci-dessous) permet de personnaliser l'environnement de test.

Le lancement d'un test à partir de LearningTest peut utiliser un executable avec son chemin complet en parametre.
C'est la methode standard, utilisee par exemple pour les plateformes linux.
Pour un developpeur Khiops, il peut etre pratique d'utiliser directement l'executable obtenu lors de
la compilation, soit avec sa version release, soit avec sa version debug.
Le choix de la version debug ou release se fait en utilisant un parametre (r ou d)
au lieu du chemin complet de l'executable.
Pour beneficier de ces parametres "raccourcis", le fichier de configuration doit etre correctement specifie.
"""

""" Nom du fichier de config """
learning_test_config_file_name = "learning_test.config"

""" Liste des cles du fichier de config """
learning_test_config_keys = {
    "path": "additional path (eg: to access to java runtime)",
    "classpath": "additional classpath for java libraries",
    "learningtest_root": "alternative root dir to use where LearningTest is located",
    "learning_release_dir": "dir where the release developement binaries are located (to enable the 'r' alias')",
    "learning_debug_dir": "dir where the debug developement binaries are located (to enable the 'd' alias')",
}


def load_learning_test_config():
    """Lecture et verification du fichier de config
    Retourne les element de coinfiguration soius forme d'un dictionnaire si ok
    Quitte le programme sinon"""
    ok = True
    config_dic = {}
    # Recherche du chemin complet du script python en cours
    containing_dir_path = os.path.dirname(os.path.realpath(__file__))
    # Chemin du fichier de config
    config_file_path = os.path.join(containing_dir_path, learning_test_config_file_name)
    # Si fichier absent, on utilise des valeurs vides pour chaque cle de config
    if not os.path.isfile(config_file_path):
        for key in learning_test_config_keys:
            config_dic[key] = ""
        return config_dic
    # Lecture du fichier
    if ok:
        try:
            with open(config_file_path, "r") as config_file:
                lines = config_file.readlines()
        except Exception as e:
            print(
                "Error in config file "
                + learning_test_config_file_name
                + ": read error ("
                + str(e)
                + ")"
            )
            ok = False
    # Analyse des paires cle valeur
    if ok:
        for n, line in enumerate(lines):
            line = line.strip()
            # On saute les lignes de commentaires et les lignes vides
            if len(line) == 0 or line.find("#") == 0:
                continue
            # Analyse de la paire (key=value)
            fields = line.split("=")
            # Cas d'une erreur de syntaxe
            if len(fields) != 2:
                print(
                    "error in config file "
                    + learning_test_config_file_name
                    + " line "
                    + str(n + 1)
                    + ": bad field number"
                )
                ok = False
                break
            # Test de validite de la cle
            if not fields[0] in learning_test_config_keys:
                print(
                    "error in config file "
                    + learning_test_config_file_name
                    + " line "
                    + str(n + 1)
                    + ": unknown key <"
                    + fields[0]
                    + ">"
                )
                ok = False
                break
            else:
                # Test de l'unicite de la cle
                if config_dic.get(fields[0]) is not None:
                    print(
                        "error in config file "
                        + learning_test_config_file_name
                        + " line "
                        + str(n + 1)
                        + ": key <"
                        + fields[0]
                        + "> not unique"
                    )
                    ok = False
                    break
                else:
                    config_dic[fields[0]] = fields[1]
    # Initialisation des cle manquantes avec des valeurs vides
    if ok:
        if len(learning_test_config_keys) != len(config_dic):
            for key in learning_test_config_keys:
                if key not in config_dic:
                    config_dic[key] = ""
    # Retour si ok
    if ok:
        return config_dic
    else:
        # Affichage d'une message d'aide
        print("")
        print(
            "The config file "
            + learning_test_config_file_name
            + " must be in directory LearningTest\\cmd\\python"
        )
        print("It is optional, in which case all keys are set to empty")
        print(
            "It contains the following key=value pairs that allows a personnalisation of the environment:"
        )
        for key in learning_test_config_keys:
            print("\t" + key + ": " + learning_test_config_keys[key])
        quit()


""" Global config dictionary """
learning_test_config = load_learning_test_config()


def search_learning_test_root():
    """Extraction du repertoire racine de  LearningTest"""
    # Cas ou un repertoire racine alternatif est specifie dans le fichier de config
    root_dir = learning_test_config["learningtest_root"]
    if root_dir != "":
        # Test si le repertoire est valdie
        if not os.path.isdir(root_dir):
            print(
                "error in config file "
                + learning_test_config_file_name
                + ": key learningtest_root contains value <"
                + root_dir
                + "> that is not a valid directory"
            )
            quit()
        if not os.path.isdir(os.path.join(root_dir, lt.LEARNING_TEST)):
            print(
                "error in config file "
                + learning_test_config_file_name
                + ": key learningtest_root ("
                + root_dir
                + ") should contain LearningTest dir"
            )
            quit()

    # Cas ou on prend le repertoire racine par defaut
    else:
        # Recherche du chemin complet du script python en cours
        containing_dir_path = os.path.dirname(os.path.realpath(__file__))
        assert lt.LEARNING_TEST in containing_dir_path, (
            lt.LEARNING_TEST + " dir not found in path " + containing_dir_path
        )
        # Recherche du repertoire racine
        root_dir = utils.get_root_dir(containing_dir_path)
    return root_dir


# Specification de la variable d'environement 'path'
path_env = learning_test_config["path"]
if path_env != "":
    if os.environ.get("path") is None:
        os.environ["path"] = path_env
    else:
        os.environ["path"] = path_env + ";" + os.environ["path"]

# Specification de la variable d'environement 'classpath'
class_path_env = learning_test_config["classpath"]
if class_path_env != "":
    if os.environ.get("CLASSPATH") is None:
        os.environ["CLASSPATH"] = class_path_env
    else:
        os.environ["CLASSPATH"] = class_path_env + ";" + os.environ["CLASSPATH"]

# Root dir de LearningTest
learning_test_root = search_learning_test_root()


def build_dev_tool_exe_path(exe_name, version):
    """Construction du path de l'exe de l'environnement de developpement
    pour une version donnee (d: debug our r: release)"""
    assert version in ["d", "r"], version + " must be d or r"
    if version == "r":
        config_key = "learning_release_dir"
    else:
        config_key = "learning_debug_dir"
    learning_dev_dir = learning_test_config[config_key]
    # Verification du repertoire
    if learning_dev_dir == "":
        print(
            "error in config file "
            + learning_test_config_file_name
            + ": key "
            + config_key
            + " must be specified to use '"
            + version
            + "' alias"
        )
        quit()
    elif not os.path.isdir(learning_dev_dir):
        print(
            "error in config file "
            + learning_test_config_file_name
            + ": key "
            + config_key
            + " ("
            + learning_dev_dir
            + ") should be a valid directory"
        )
        quit()
    # Construction du path de l'outil
    tool_exe_path = os.path.join(learning_dev_dir, exe_name)
    if os.name == "nt":
        tool_exe_path += ".exe"
    if not os.path.isfile(tool_exe_path):
        utils.fatal_error("executable (" + tool_exe_path + ") should be a valid file")
    return tool_exe_path
