import os.path
import sys

import _kht_constants as kht
import _kht_utils as utils
import _kht_families as test_families
import kht_apply


def main():
    """Fonction principale d'application systematique d'une instruction sur une famille de tests"""

    (
        all_instructions,
        standard_instruction_number,
    ) = kht_apply.register_all_instructions()

    # Aide si mauvais nombre de parametres
    if len(sys.argv) != 3 and len(sys.argv) != 4:
        script_name = os.path.basename(__file__)
        base_script_name = os.path.splitext(script_name)[0]
        print(
            base_script_name
            + " (instruction) (home|tool dir) [family (default: full)]\n"
            "  Apply an instruction (ex: errors) on a family"
        )
        print("  Type kht_apply to show all available instructions")
        print(
            "  The available families of test suites are "
            + utils.list_to_label(test_families.TEST_FAMILIES + ["all"])
        )
        print(
            "  Family 'all' stands for all suite directories of each tool dir,\n"
            "   including 'unofficial' test suites, such as z_work"
        )
        print("  Examples")
        print(
            "   "
            + base_script_name
            + " list "
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
            + " errors "
            + os.path.join(
                "<root dir>",
                kht.LEARNING_TEST,
                kht.TOOL_DIR_NAMES[kht.KHIOPS],
            )
        )

        exit(0)

    # Recherche des parametres
    instruction_name = sys.argv[1]
    home_or_tool_dir = sys.argv[2]
    test_family = test_families.DEFAULT_TEST_FAMILY
    include_unofficial_sub_dirs = False
    if len(sys.argv) == 4:
        test_family = sys.argv[3]
        if test_family == "all":
            include_unofficial_sub_dirs = True
        else:
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

    # Recherche des outils concerness
    if tool_name is None:
        tested_tools_names = kht.TOOL_NAMES
    else:
        tested_tools_names = [tool_name]

    # Lancement de la commande sur tous les repertoires concernes
    for tool_name in tested_tools_names:
        tool_dir_name = kht.TOOL_DIR_NAMES.get(tool_name)
        tool_dir = os.path.join(home_dir, tool_dir_name)
        # Cas d'une famille de suites
        if not include_unofficial_sub_dirs:
            test_suites = test_families.FAMILY_TEST_SUITES[test_family, tool_name]
        # Cas de toutes les suites
        else:
            # Tri des repertoire par nom pour assurer une stabiliote inter-plateforme
            dir_names = os.listdir(tool_dir)
            dir_names.sort()
            test_suites = []
            for name in dir_names:
                dir = os.path.join(tool_dir, name)
                if os.path.isdir(dir):
                    test_suites.append(name)

        # Lancement de l'instruction sur tous les repertoires utilises
        for name in test_suites:
            suite_dir = os.path.join(tool_dir, name)
            if os.path.isdir(suite_dir):
                # Lancement de l'instruction
                kht_apply.execute_instruction_on_suite_dir(
                    all_instructions, instruction_name, suite_dir
                )
            else:
                print("error : suite directory not found: " + suite_dir)


if __name__ == "__main__":
    main()
