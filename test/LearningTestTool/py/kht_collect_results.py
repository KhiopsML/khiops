import os
import sys
import os.path

import _kht_constants as kht
import _kht_utils as utils
import _kht_families as test_families
import _kht_check_results as check
import kht_save

"""
Collecte de tout ou partie des resultats de test
"""

# Liste des options de collecte
COLLECT_ALL = "all"
COLLECT_MESSAGES = "messages"
COLLECT_ERRORS = "errors"
COLLECT_OPTIONS = [COLLECT_ALL, COLLECT_MESSAGES, COLLECT_ERRORS]
assert len(set(COLLECT_OPTIONS)) == len(COLLECT_OPTIONS), (
    "Collect options " + str(COLLECT_OPTIONS) + " must not contain duplicates"
)

# Libelles des options de sauvegarde
COLLECT_OPTION_LABELS = {
    COLLECT_ALL: "collect all test dir results",
    COLLECT_MESSAGES: "collect test dir results only in case of errors or messages",
    COLLECT_ERRORS: "collect test dir results only in case or errors",
}
assert set(COLLECT_OPTION_LABELS) == set(
    COLLECT_OPTION_LABELS
), "A label must be defined for each save option"


def collect_learning_test_results(home_dir, target_root_dir, collect_option, family):
    """
    Sauvegarde de l'arborescence de LearningTest
    - home_dir: repertoire de l'aborescence source
    - target_root_dir: repertoire cible contenant l'arborescence sauvegardee
    - collect_option: option pour sauvegarder tout ou partie de l'arborescence
    - family: famille utilise pour choisir la sous-partie a exporter
    L'aborescence des resultats est sauvegardee dans <target_root_dir>/LearningTest_<family>_results
    """
    assert os.path.isdir(home_dir)
    assert os.path.isdir(target_root_dir)
    assert collect_option in COLLECT_OPTIONS
    assert family in test_families.TEST_FAMILIES

    # Nom du repertoire cible
    target_home_dir = os.path.join(
        target_root_dir, kht.LEARNING_TEST + "_results_" + collect_option + "_" + family
    )

    # Verification qu'il n'existe pas deja
    if os.path.isdir(target_home_dir):
        utils.fatal_error("target home dir already exist on " + target_home_dir)

    # Parametrage des noms de fichiers ou repertoire specifiques a ne pas prendre en compte
    forbidden_names = ["__pycache__"]

    # Creation du repertoire cible
    os.makedirs(target_home_dir)

    # Parcours des repertoires des outils pour en recuperer les resultats des suites de la famille
    collecte_test_dir_number = 0
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
                        # Analyse du log de comparaison
                        (
                            error_number,
                            warning_number,
                            summary_infos,
                            files_infos,
                        ) = check.analyse_comparison_log(source_test_dir)
                        note_message = summary_infos.get(check.SUMMARY_NOTE_KEY, "")
                        portability_message = summary_infos.get(
                            check.SUMMARY_PORTABILITY_KEY, ""
                        )
                        # On decide s'il faut collecter les results en fonction des options
                        collect_results = False
                        if collect_option == COLLECT_ALL:
                            collect_results = True
                        elif collect_option == COLLECT_MESSAGES:
                            collect_results = (
                                error_number > 0
                                or warning_number > 0
                                or note_message != ""
                                or portability_message != ""
                            )
                        elif collect_option == COLLECT_ERRORS:
                            collect_results = error_number > 0

                        # Copie du repertoire de test si necessaire
                        if collect_results:
                            target_test_dir = os.path.join(
                                target_suite_dir, test_dir_name
                            )
                            collecte_test_dir_number += 1
                            kht_save.copy_subdirs(
                                source_test_dir,
                                target_test_dir,
                                ignore_list=forbidden_names,
                            )
            else:
                print("error : suite directory not found: " + source_suite_dir)
    print("\tCollected test dir number: " + str(collecte_test_dir_number))


def main():
    """Fonction principale de collecte des resultats de LearningTest"""
    # Aide si mauvais nombre de parametres
    if len(sys.argv) != 4 and len(sys.argv) != 5:
        script_name = os.path.basename(__file__)
        base_script_name = os.path.splitext(script_name)[0]
        print(
            base_script_name
            + " (home dir) (target root dir) (collect option) [family (default: full)]\n"
            "  Collect " + kht.LEARNING_TEST + " results"
        )
        print("  Available collect options are:")
        for collect_option in COLLECT_OPTIONS:
            print(
                "    " + collect_option + ": " + COLLECT_OPTION_LABELS[collect_option]
            )
        print(
            "  The available families of test suites are "
            + utils.list_to_label(test_families.TEST_FAMILIES)
        )
        print(
            "  The home dir is saved under a directory named <target root dir>/"
            + kht.LEARNING_TEST
            + "_results_<collect option>_<family>"
        )
        exit(0)

    # Recherche des parametres
    home_dir = sys.argv[1]
    target_root_dir = sys.argv[2]
    collect_option = sys.argv[3]
    if collect_option not in COLLECT_OPTIONS:
        utils.fatal_error(
            "collect option ("
            + collect_option
            + ") must be in "
            + utils.list_to_label(COLLECT_OPTIONS)
        )
    test_family = test_families.DEFAULT_TEST_FAMILY
    if len(sys.argv) == 5:
        test_family = sys.argv[4]
        test_families.check_family(test_family)

    # Analyse de la validite des parametres
    utils.check_home_dir(home_dir)
    utils.check_candidate_root_dir(home_dir, target_root_dir)

    # Lancement de la commande
    collect_learning_test_results(
        home_dir, target_root_dir, collect_option, test_family
    )


if __name__ == "__main__":
    main()
