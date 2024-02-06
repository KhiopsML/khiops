import os.path
import sys
import stat

import _learning_test_utils as utils
import _standard_commands as standard_commands
import _one_shot_commands as one_shot_commands

"""
Gestion de l'ensemble des commandes
"""


def display_commands(available_commands: dict, minimum_command_number, show_all=False):
    """Affichage des commandes disponible avec leur identifiant et libelle"""
    assert minimum_command_number is None or minimum_command_number > 0
    # Affichage de l'aide generique globale
    print("apply_command [command] [suite_path] ([test_dir])")
    print("  apply command on a directory structure")
    print("\tcommand: name of the command")
    print("\tsuite_path is the path of the suite directory")
    print("\ttest_dir is the name of one specific test directory")
    print("\t         or all (default) for executing on all tests of the suite")
    print("    examples:")
    print("      applyCommand list TestKhiops\\Standard")
    print("      applyCommand list TestKhiops\\Standard Adult")
    print("  apply_command all: to include one shot commands")
    print("\n Standard commands")
    # Affichage de la liste des commandes disponibles
    for i, command_id in enumerate(available_commands):
        (command_function, command_label) = available_commands[command_id]
        print("\t" + command_id + ": " + command_label)
        if i == minimum_command_number - 1:
            if show_all:
                print("\n One shot commands")
            else:
                break


def execute_command(
    available_commands: dict, command_id, suite_dir, specific_test_dir_name=None
):
    """Usage interne
    Comme la fonction principale apply_command du main(),
     avec un dictionnaire des commandes en premier parametre
    """
    assert command_id != ""
    assert suite_dir != ""
    # Verification des operandes
    if command_id not in available_commands:
        utils.fatal_error("wrong command " + command_id)
    utils.check_suite_dir(suite_dir)
    # Recherche des sous-repertoires a exploiter
    test_dir_name_list = []
    if specific_test_dir_name is None:
        for name in os.listdir(suite_dir):
            if os.path.isdir(os.path.join(suite_dir, name)):
                test_dir_name_list.append(name)
    else:
        if os.path.isdir(os.path.join(suite_dir, specific_test_dir_name)):
            test_dir_name_list.append(specific_test_dir_name)
        else:
            utils.fatal_error(
                "test directory "
                + specific_test_dir_name
                + " of "
                + suite_dir
                + " does not exist"
            )
    if len(test_dir_name_list) == 0:
        utils.fatal_error("no test directory is available in " + suite_dir)

    # Tri pour assurer la reproductibilite inter plateforme
    test_dir_name_list.sort()

    # Execution de la commande
    (command_function, command_label) = available_commands[command_id]
    for test_dir_name in test_dir_name_list:
        # lanceur de commande sur un directory
        test_dir = os.path.realpath(os.path.join(suite_dir, test_dir_name))
        # verification de l'existence du directory
        if not os.path.isdir(test_dir):
            utils.fatal_error("directory " + test_dir + " does not exist")
        # Lancement de la commande dans son repertoire de travail
        current_dir = os.getcwd()
        os.chdir(test_dir)
        command_function(test_dir)
        os.chdir(current_dir)
    # Message synthetique de fin si famille de jeu de tests
    suite_dir_name = utils.dir_name(suite_dir)
    tool_dir_name = utils.parent_dir_name(suite_dir, 1)
    if specific_test_dir_name is None:
        print("DONE\t" + tool_dir_name + "\t" + suite_dir_name)


def register_all_commands():
    """Enregistrement de toutes les commandes standard et a usage unique
    Retourne un dictionnaire des commandes, et le nombre de commandes standards
    """

    commands1 = standard_commands.register_standard_commands()
    commands2 = one_shot_commands.register_one_shot_commands()
    commands = commands1 | commands2
    return commands, len(commands1)


if __name__ == "__main__":
    all_commands, standard_command_number = register_all_commands()

    # Affichage des commandes si pas de parametres ou mauvais nombre de parametres
    if len(sys.argv) <= 2:
        show_one_shot_commands = len(sys.argv) == 2 and sys.argv[1] == "all"
        display_commands(
            all_commands, standard_command_number, show_all=show_one_shot_commands
        )
        exit(0)

    # Recherche des parametres sur la ligne de commande
    main_command_name = sys.argv[1]
    main_suite_dir = sys.argv[2]
    if len(sys.argv) == 4:
        main_test_dir_name = sys.argv[3]
    else:
        main_test_dir_name = None

    # Lancement de la commande
    execute_command(all_commands, main_command_name, main_suite_dir, main_test_dir_name)
