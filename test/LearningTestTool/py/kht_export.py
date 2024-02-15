import os
import sys
import os.path

import _kht_constants as kht
import _kht_utils as utils
import _kht_families as test_families
import kht_save

"""
Export de la sous-partie d'un repertoire LearningTest pour la famille de test specifiee
"""


def export_learning_test_tree(home_dir, target_root_dir, family):
    """
    Sauvegarde de l'arborescence de LearningTest
    - home_dir: repertoire de l'aborescence source
    - target_root_dir: repertoire cible contenant l'arborescence sauvegardee
    - family: famille utilise pour choisir la sous-partie a exporter
    L'aborescence est sauvegardee dans <target_root_dir>/LearningTest_<family>
    """
    assert os.path.isdir(home_dir)
    assert os.path.isdir(target_root_dir)
    assert family in test_families.TEST_FAMILIES

    # Nom du repertoire cible
    target_home_dir = os.path.join(target_root_dir, kht.LEARNING_TEST + "_" + family)

    # Verification qu'il n'existe pas deja
    if os.path.isdir(target_home_dir):
        utils.fatal_error("target home dir already exist on " + target_home_dir)

    # Parametrage des noms de fichiers ou repertoire specifiques a ne pas prendre en compte
    forbidden_names = ["__pycache__", kht.RESULTS, kht.COMPARISON_RESULTS_LOG]

    # Initialisation des ensembles de dataset utilises par repertoire de jeux de donnees
    used_dataset_collections = {}
    for name in kht.DATASET_COLLECTION_NAMES:
        used_dataset_collections[name] = {}

    # Creation du repertoire cible
    os.makedirs(target_home_dir)

    # Parcours des repertoires des outils pour en recopier les suites par famille
    for tool_name in kht.TOOL_NAMES:
        tool_dir_name = kht.TOOL_DIR_NAMES[tool_name]
        test_suites = test_families.FAMILY_TEST_SUITES[family, tool_name]
        # Creation du repertoire de l'outil
        target_tool_dir = os.path.join(
            target_home_dir,
            tool_dir_name,
        )
        if len(test_suites) > 0:
            utils.make_dir(target_tool_dir)
        # Parcours des suites de la famille
        for suite_dir_name in test_suites:
            source_suite_dir = os.path.join(
                home_dir,
                tool_dir_name,
                suite_dir_name,
            )
            if os.path.isdir(source_suite_dir):
                # Creation du repertoire de suite
                target_suite_dir = os.path.join(
                    target_home_dir,
                    tool_dir_name,
                    suite_dir_name,
                )
                utils.make_dir(target_suite_dir)
                # Parcours des repertoires de test de la suite
                for test_dir_name in os.listdir(source_suite_dir):
                    source_test_dir = os.path.join(source_suite_dir, test_dir_name)
                    if os.path.isdir(source_test_dir):
                        # Copie du repertoire de test
                        target_test_dir = os.path.join(target_suite_dir, test_dir_name)
                        kht_save.copy_subdirs(
                            source_test_dir,
                            target_test_dir,
                            ignore_list=forbidden_names,
                        )
                        # Analyse du scenario pour detecter les jeux de donnees utilises
                        test_prm_path = os.path.join(source_test_dir, kht.TEST_PRM)
                        if os.path.isfile(test_prm_path):
                            with open(
                                test_prm_path, "r", errors="ignore"
                            ) as test_prm_file:
                                lines = test_prm_file.readlines()
                                for line in lines:
                                    # Nettoyage de la ligne
                                    line = line.strip()
                                    pos_comment = line.find("//")
                                    if pos_comment >= 0:
                                        line = line[:pos_comment]
                                        line = line.strip()
                                    # Parsing de la ligne pour recherche les nom de dataset en derniere position
                                    # En effet, les patterns de type "ClassFileName ../../../datasets/Adult/Adult.kdic"
                                    # peuvent etre separes par un ou plusieurs caracteres blancs ou tabulation
                                    line = line.replace("\t", " ")
                                    fields = line.split(" ")
                                    if len(fields) >= 2:
                                        candidate_file_path = fields[-1]
                                        if candidate_file_path[0] == ".":
                                            # Le separateur est normalise dans les fichier scenario
                                            path_components = candidate_file_path.split(
                                                "/"
                                            )
                                            # les datasets se trouvent toujours au meme niveau relatif de l'arborescence
                                            if (
                                                len(path_components) > 5
                                                and path_components[0] == ".."
                                                and path_components[1] == ".."
                                                and path_components[2] == ".."
                                            ):
                                                dataset_collection = path_components[3]
                                                if (
                                                    dataset_collection
                                                    in kht.DATASET_COLLECTION_NAMES
                                                ):
                                                    dataset_name = path_components[4]
                                                    used_dataset_collections[
                                                        dataset_collection
                                                    ][dataset_name] = True
            else:
                print("error : suite directory not found: " + source_suite_dir)

    # Recopie des jeux de donnees utilises
    for dataset_collection in used_dataset_collections:
        used_dataset_names = used_dataset_collections[dataset_collection]
        if len(used_dataset_names):
            # Creation du repertoire global de jeu de donnees
            target_dataset_collection_dir = os.path.join(
                target_home_dir,
                dataset_collection,
            )
            utils.make_dir(target_dataset_collection_dir)
            # Creation des repertoire elementaire de jeux de donnees
            for dataset_name in used_dataset_names:
                source_dataset_dir = os.path.join(
                    home_dir, dataset_collection, dataset_name
                )
                target_dataset_dir = os.path.join(
                    target_home_dir, dataset_collection, dataset_name
                )
                kht_save.copy_subdirs(
                    source_dataset_dir,
                    target_dataset_dir,
                    ignore_list=forbidden_names,
                )


def main():
    """Fonction principale d'export de l'arborescence LearningTest"""
    # Aide si mauvais nombre de parametres
    if len(sys.argv) != 3 and len(sys.argv) != 4:
        script_name = os.path.basename(__file__)
        base_script_name = os.path.splitext(script_name)[0]
        print(
            base_script_name
            + " (home dir) (target root dir) [family (default: full)]\n"
            "  Export " + kht.LEARNING_TEST + " directory tree"
        )
        print(
            "  The available families of test suites are "
            + utils.list_to_label(test_families.TEST_FAMILIES)
        )
        print(
            "  The home dir is saved under a directory named <target root dir>/"
            + kht.LEARNING_TEST
            + "_<family>"
        )
        exit(0)

    # Recherche des parametres
    home_dir = sys.argv[1]
    target_root_dir = sys.argv[2]
    test_family = test_families.DEFAULT_TEST_FAMILY
    if len(sys.argv) == 4:
        test_family = sys.argv[3]
        test_families.check_family(test_family)

    # Analyse de la validite des parametres
    utils.check_home_dir(home_dir)
    utils.check_candidate_root_dir(home_dir, target_root_dir)

    # Lancement de la commande
    export_learning_test_tree(home_dir, target_root_dir, test_family)


if __name__ == "__main__":
    main()
