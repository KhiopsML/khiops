import os.path
import sys

import _learning_test_constants as lt
import _test_families as test_families
import _learning_test_config as learning_test_config
import apply_command

if __name__ == "__main__":
    all_commands, standard_command_number = apply_command.register_all_commands()

    include_unofficial_sub_dirs = False
    ok = len(sys.argv) == 2
    if not ok:
        ok = len(sys.argv) == 3 and sys.argv[2] == "all"
        include_unofficial_sub_dirs = ok
    if not ok:
        print("applyCommandAll [command] <all>")
        print("  apply command on all test sub-directories")
        print("\tcommand: name of the command")
        print("\tall: to include 'unofficial' sub-directories, such as z_work")
        print("  Type applyCommand to see available commands")
        exit(0)

    # Acces a la commande
    main_command_name = sys.argv[1]

    # Parcours des outils pour lancer la commande sur chaque repertoire d'outil
    for tool_name in lt.TOOL_NAMES:
        tool_dir_name = lt.TOOL_DIR_NAMES.get(tool_name)
        tool_dir = os.path.join(
            learning_test_config.learning_test_root, lt.LEARNING_TEST, tool_dir_name
        )
        # Recherche des nom des suite de tests pour initialiser les drepertoire a utiuliser
        test_suite_names = test_families.get_family_suite_names(tool_name)
        suite_dir_names = test_suite_names.copy()
        # Ajout des repertoires non officiels si demande
        if include_unofficial_sub_dirs:
            # Tri des repertoire par nom pour assurer une stabiliote inter-plateforme
            tool_all_suite_dir_names = os.listdir(tool_dir)
            tool_all_suite_dir_names.sort()
            for suite_dir_name in tool_all_suite_dir_names:
                if suite_dir_name not in test_suite_names:
                    # Les repertoire non officiels commencent avec un '_' en second caractere (ex: z_work)
                    if suite_dir_name.find("_") == 1:
                        suite_dir = os.path.join(tool_dir, suite_dir_name)
                        if os.path.isdir(suite_dir):
                            suite_dir_names.append(suite_dir_name)
        # Lancement de la commande sur tous les repertoires utilises
        for suite_dir_name in suite_dir_names:
            suite_dir = os.path.join(tool_dir, suite_dir_name)
            if os.path.isdir(suite_dir):
                apply_command.execute_command(
                    all_commands, main_command_name, suite_dir
                )
            else:
                print("error : directory not found: " + suite_dir)
