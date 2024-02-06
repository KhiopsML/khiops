import sys
import stat
import os.path

import _learning_test_constants as lt
import _learning_test_utils as utils
import _test_families as test_families
import _learning_test_config as learning_test_config
import test_khiops


def test_tool(tool_name, tool_version, suite_dir_names):
    """Lance les tests de khiops sur tous les repertoires contenus dans la liste suite_dir_names"""

    # Constuction du chemin complet de l'exe de l'outil
    assert tool_name in lt.TOOL_NAMES
    tool_exe_name = lt.TOOL_EXE_NAMES[tool_name]
    tool_dir_name = lt.TOOL_DIR_NAMES[tool_name]
    tool_exe_path = test_khiops.build_tool_exe_path(tool_exe_name, tool_version)

    # Nettoyage des resultats
    if tool_version != "nul":
        for suite_dir_name in suite_dir_names:
            suite_dir = os.path.join(
                learning_test_config.learning_test_root,
                lt.LEARNING_TEST,
                tool_dir_name,
                suite_dir_name,
            )
            if os.path.isdir(suite_dir):
                for test_dir_name in os.listdir(suite_dir):
                    test_dir = os.path.join(suite_dir, test_dir_name)
                    file_path = os.path.join(test_dir, lt.COMPARISON_RESULTS_LOG)
                    if os.path.isfile(file_path):
                        utils.remove_file(file_path)
                    results_dir = os.path.join(test_dir, lt.RESULTS)
                    if os.path.isdir(results_dir):
                        for file_name in os.listdir(results_dir):
                            file_path = os.path.join(results_dir, file_name)
                            utils.remove_file(file_path)

    # Lancement des tests
    for suite_dir_name in suite_dir_names:
        print("\n\n--------------------------------------------------------")
        print("\tRunning " + tool_name + " " + suite_dir_name + " tests")
        print("--------------------------------------------------------")
        suite_dir = os.path.join(
            learning_test_config.learning_test_root,
            lt.LEARNING_TEST,
            tool_dir_name,
            suite_dir_name,
        )
        if os.path.isdir(suite_dir):
            test_khiops.evaluate_tool_on_suite(tool_exe_path, suite_dir, None)
        else:
            # Affichage d'une erreur, mais sans quitter le programme, pour continuer les tests
            print("error : missing directory for test suite " + suite_dir)


if __name__ == "__main__":
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        print("testAll [version] <tool>")
        print("  run all tests for all Khiops tools")
        print("\tversion: version of the tool")
        print("\t  d: debug version in developpement environnement")
        print("\t  r: release version in developpement environnement")
        print("\t  ver: <toolname>.<ver>.exe in directory LearningTest\\cmd\\modl")
        print("\t  nul: for comparison with the test results only")
        print("\t  full exe path, if <tool> parameter is used")
        print("\ttool: all tools if not specified, one specified tool otherwise")
        print("\t  Khiops")
        print("\t  Coclustering")
        print("\t  KNI")
        exit(0)

    # Info sur les tests complets
    if os.getenv(lt.KHIOPS_COMPLETE_TESTS) != "true":
        print("\n--------------------------------------------------------")
        print("Set env var " + lt.KHIOPS_COMPLETE_TESTS + "=true")
        print("\tto run all long, instable and unusefull tests")
        print("--------------------------------------------------------\n")
    print("\n\n--------------------------------------------------------")

    sys.stdout = test_khiops.Unbuffered(sys.stdout)

    # Passage en mode batch
    os.environ[lt.KHIOPS_BATCH_MODE] = "true"

    # Recherche de la version
    main_version = sys.argv[1]
    assert main_version is not None

    # Recherche de l'outil
    main_tool_name = ""
    if len(sys.argv) == 3:
        main_tool_name = sys.argv[2]
        if main_tool_name not in lt.TOOL_NAMES:
            utils.fatal_error(
                "tool "
                + main_tool_name
                + " should be in "
                + utils.list_to_label(lt.TOOL_NAMES)
            )

    # Khiops tool
    if main_tool_name == "" or main_tool_name == lt.KHIOPS:
        khiops_suite_dir_names = test_families.get_family_suite_names(lt.KHIOPS)
        test_tool(lt.KHIOPS, main_version, khiops_suite_dir_names)

    # Coclustering tool
    if main_tool_name == "" or main_tool_name == lt.COCLUSTERING:
        coclustering_suite_dir_names = test_families.get_family_suite_names(
            lt.COCLUSTERING
        )
        test_tool(lt.COCLUSTERING, main_version, coclustering_suite_dir_names)

    # KNI tool
    if main_tool_name == "" or main_tool_name == lt.KNI:
        KNI_suite_dir_names = test_families.get_family_suite_names(lt.KNI)
        test_tool(lt.KNI, main_version, KNI_suite_dir_names)

    print("all tests are done")
