import sys
import os.path

import _kht_constants as kht
import _kht_utils as utils
import _kht_families as test_families
import kht_test


def evaluate_tool(tool_name, tool_exe_path, home_dir, test_suites):
    """Lance les tests d'un outil sur une ensemble de suites de tests"""
    assert tool_name in kht.TOOL_NAMES
    utils.check_home_dir(home_dir)

    # Recherche du repertoire lie a l'outil
    tool_dir_name = kht.TOOL_DIR_NAMES[tool_name]

    # Verification des repertoires des suites et nettoyage des resultats
    for suite_dir_name in test_suites:
        suite_dir = os.path.join(
            home_dir,
            tool_dir_name,
            suite_dir_name,
        )
        if os.path.isdir(suite_dir):
            # Nettoyage sauf si seulement comparains des resultats
            if tool_exe_path != kht.ALIAS_CHECK:
                for test_dir_name in os.listdir(suite_dir):
                    test_dir = os.path.join(suite_dir, test_dir_name)
                    file_path = os.path.join(test_dir, kht.COMPARISON_RESULTS_LOG)
                    if os.path.isfile(file_path):
                        utils.remove_file(file_path)
                    results_dir = os.path.join(test_dir, kht.RESULTS)
                    if os.path.isdir(results_dir):
                        for file_name in os.listdir(results_dir):
                            file_path = os.path.join(results_dir, file_name)
                            utils.remove_file(file_path)
        else:
            print("error : suite directory not found: " + suite_dir)

    # Lancement des tests sur les repertoires valides
    for suite_dir_name in test_suites:
        suite_dir = os.path.join(
            home_dir,
            tool_dir_name,
            suite_dir_name,
        )
        if os.path.isdir(suite_dir):
            print("\n\n--------------------------------------------------------")
            print("\tRunning " + tool_name + " " + suite_dir_name + " tests")
            print("--------------------------------------------------------")
            suite_dir = os.path.join(
                home_dir,
                tool_dir_name,
                suite_dir_name,
            )
            kht_test.evaluate_tool_on_suite_dir(tool_exe_path, suite_dir)


def main():
    """Fonction principale de lancement d'un test"""

    # Aide si mauvais nombre de parametres
    if len(sys.argv) != 3 and len(sys.argv) != 4:
        script_name = os.path.basename(__file__)
        base_script_name = os.path.splitext(script_name)[0]
        print(
            base_script_name
            + " (tool binaries dir) (home|tool dir) [family (default: full)]\n"
            "  Test tools on a family of test suites"
        )
        print(
            "  The tool binaries dir must contain the executable of the tested tools,"
        )
        print("   or one of the following aliases:")
        print("    d: debug binary dir in developpement environnement")
        print("    r: release binary dir in developpement environnement")
        print("    check: only compare test and reference results")
        print(
            "  The available families of test suites are "
            + utils.list_to_label(test_families.TEST_FAMILIES)
        )
        print("  Examples")
        print(
            "   "
            + base_script_name
            + " r "
            + os.path.join(
                "<root dir>",
                kht.LEARNING_TEST,
            )
            + " "
            + test_families.BASIC
        )
        print(
            "   "
            + base_script_name
            + ' "C:\\Program Files\\khiops\\bin" '
            + os.path.join(
                "<root dir>",
                kht.LEARNING_TEST,
                kht.TOOL_DIR_NAMES[kht.KHIOPS],
            )
        )
        print(
            "   "
            + base_script_name
            + " check "
            + os.path.join(
                "<root dir>",
                kht.LEARNING_TEST,
            )
        )
        exit(1)

    # Pour flusher systematiqment sur la sortie standard
    sys.stdout = kht_test.Unbuffered(sys.stdout)

    # Passage en mode batch
    os.environ[kht.KHIOPS_BATCH_MODE] = "true"

    # Recherche des parametres
    tool_binaries_dir = sys.argv[1]
    home_or_tool_dir = sys.argv[2]
    test_family = test_families.DEFAULT_TEST_FAMILY
    if len(sys.argv) == 4:
        test_family = sys.argv[3]
        test_families.check_family(test_family)

    # Analyse du repertoire a tester
    learning_test_depth = utils.check_learning_test_dir(home_or_tool_dir)
    tool_dir_name = None
    tool_name = None
    if learning_test_depth == 1:
        tool_dir_name = utils.parent_dir_name(home_or_tool_dir, 0)
        if tool_dir_name not in kht.TOOL_DIR_NAMES.values():
            utils.fatal_error(
                tool_dir_name
                + " in "
                + os.path.realpath(home_or_tool_dir)
                + " should be a tool dir "
                + utils.list_to_label(kht.TOOL_DIR_NAMES.values())
            )
        tool_name = kht.TOOL_NAMES_PER_DIR_NAME[tool_dir_name]
    elif learning_test_depth != 0:
        utils.fatal_error(home_or_tool_dir + " should be the home dir or a tool dir")
    home_dir = utils.get_home_dir(home_or_tool_dir)

    # Recherche des outils a tester
    if tool_name is None:
        tested_tools_names = kht.TOOL_NAMES
    else:
        tested_tools_names = [tool_name]

    # Lancement de tous les tests
    for tool_name in tested_tools_names:
        test_suites = test_families.FAMILY_TEST_SUITES[test_family, tool_name]
        # On ne teste la validite de l'exe que s'il y au moins une suite dans la famille
        if len(test_suites) > 0:
            # On sort en erreur fatal si l'exe n'esiste pas
            # Cela n'est fait a priori sur tous les outils
            # Cela permet de lancer un test complet sur une famille, meme si l'exe de KNI
            # (exploite en dernier) n'est pas disponible
            tool_exe_path = kht_test.build_tool_exe_path(tool_binaries_dir, tool_name)
            evaluate_tool(tool_name, tool_exe_path, home_dir, test_suites)

    print("all tests are done")


if __name__ == "__main__":
    main()
