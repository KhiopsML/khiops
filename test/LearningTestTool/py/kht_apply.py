import os.path
import sys

import _kht_constants as kht
import _kht_utils as utils
import _kht_standard_instructions as standard_instructions
import _kht_one_shot_instructions as one_shot_instructions

"""
Gestion de l'ensemble des instructions
"""


def display_instructions(
    available_instructions: dict, minimum_instruction_number, show_all=False
):
    """Affichage des instructions disponibles avec leur identifiant et libelle"""
    assert minimum_instruction_number is None or minimum_instruction_number > 0
    # Affichage de l'aide generique globale
    script_name = os.path.basename(__file__)
    base_script_name = os.path.splitext(script_name)[0]
    print(
        base_script_name + " (instruction) (suite|test dir)\n"
        "  Apply instruction (ex: errors) on a suite or test directory"
    )
    print("  Examples")
    print(
        "   "
        + base_script_name
        + " list "
        + os.path.join(
            "<root dir>",
            kht.LEARNING_TEST,
            kht.TOOL_DIR_NAMES[kht.KHIOPS],
            "Standard",
        )
    )
    print(
        "   "
        + base_script_name
        + " errors "
        + os.path.join(
            "<root dir>",
            kht.LEARNING_TEST,
            kht.TOOL_DIR_NAMES[kht.KHIOPS],
            "Standard",
            "Iris",
        )
    )
    print("  Type " + base_script_name + " all to show available one shot instructions")
    print("  Standard instructions")
    # Affichage de la liste des instructions disponibles
    for i, instruction_id in enumerate(available_instructions):
        (instruction_function, instruction_label) = available_instructions[
            instruction_id
        ]
        print("\t" + instruction_id + ": " + instruction_label)
        if i == minimum_instruction_number - 1:
            if show_all:
                print("  One shot instructions")
            else:
                break


def execute_instruction_on_suite_dir(
    available_instructions: dict, instruction_id, suite_dir, test_name=None
):
    """Usage interne
    Comme la fonction principale kht_apply du main(),
     avec un dictionnaire des instructions en premier parametre
    """
    assert instruction_id != ""
    assert suite_dir != ""
    # Verification des operandes
    if instruction_id not in available_instructions:
        utils.fatal_error("wrong instruction " + instruction_id)

    # Erreur si repertoire de suite absent
    if not os.path.isdir(suite_dir):
        utils.fatal_error("missing directory for test suite " + suite_dir)

    # Collecte des sous-repertoire de test
    test_list = []
    # Cas avec un repertoire de test specifique
    if test_name is not None:
        if os.path.isdir(os.path.join(suite_dir, test_name)):
            test_list.append(test_name)
    # Cas avec une suite de test
    else:
        for file_name in os.listdir(suite_dir):
            if os.path.isdir(os.path.join(suite_dir, file_name)):
                test_list.append(file_name)

    # Erreur si pas de sous-repertoires
    if len(test_list) == 0:
        utils.fatal_error("no test dir is available in " + suite_dir)

    # Tri pour assurer la reproductibilite inter plateforme
    test_list.sort()

    # Execution de l'instruction
    (instruction_function, instruction_label) = available_instructions[instruction_id]
    for test_dir_name in test_list:
        # lanceur d'instruction sur un directory
        test_dir = os.path.realpath(os.path.join(suite_dir, test_dir_name))
        # verification de l'existence du directory
        if not os.path.isdir(test_dir):
            utils.fatal_error("directory " + test_dir + " does not exist")
        # Lancement de l'instruction dans son repertoire de travail
        current_dir = os.getcwd()
        os.chdir(test_dir)
        instruction_function(test_dir)
        os.chdir(current_dir)
    # Message synthetique de fin
    suite_dir_name = utils.dir_name(suite_dir)
    tool_dir_name = utils.parent_dir_name(suite_dir, 1)
    if test_name is None:
        print("DONE\t" + tool_dir_name + "\t" + suite_dir_name)
    else:
        print("done\t" + tool_dir_name + "\t" + suite_dir_name + "\t" + test_name)


def register_all_instructions():
    """Enregistrement de toutes les instructions standards et a usage unique
    Retourne un dictionnaire des instructions, et le nombre d'instructions standards
    """

    instructions1 = standard_instructions.register_standard_instructions()
    instructions2 = one_shot_instructions.register_one_shot_instructions()
    all_instructions = instructions1 | instructions2
    return all_instructions, len(instructions1)


def main():
    """Fonction principale d'application systematique d'une instruction sur une suite de test"""

    (
        all_instructions,
        standard_instruction_number,
    ) = register_all_instructions()

    # Affichage des instructions si pas de parametres ou mauvais nombre de parametres
    if len(sys.argv) <= 2:
        show_one_shot_instructions = len(sys.argv) == 2 and sys.argv[1] == "all"
        display_instructions(
            all_instructions,
            standard_instruction_number,
            show_all=show_one_shot_instructions,
        )
        exit(0)

    # Recherche des parametres sur la ligne d'instruction
    instruction_name = sys.argv[1]
    suite_or_test_dir = sys.argv[2]

    # Analyse du repertoire a tester
    learning_test_depth = utils.check_learning_test_dir(suite_or_test_dir)
    tool_dir_name = ""
    suite_dir_name = ""
    test_dir_name = None
    if learning_test_depth == 3:
        tool_dir_name = utils.parent_dir_name(suite_or_test_dir, 2)
        suite_dir_name = utils.parent_dir_name(suite_or_test_dir, 1)
        test_dir_name = utils.parent_dir_name(suite_or_test_dir, 0)
    elif learning_test_depth == 2:
        tool_dir_name = utils.parent_dir_name(suite_or_test_dir, 1)
        suite_dir_name = utils.parent_dir_name(suite_or_test_dir, 0)
    else:
        utils.fatal_error(suite_or_test_dir + " should be a suite or test dir")
    assert tool_dir_name != ""
    if tool_dir_name not in kht.TOOL_DIR_NAMES.values():
        utils.fatal_error(
            tool_dir_name
            + " in "
            + os.path.realpath(suite_or_test_dir)
            + " should be a tool dir "
            + utils.list_to_label(kht.TOOL_DIR_NAMES.values())
        )
    home_dir = utils.get_home_dir(suite_or_test_dir)
    suite_dir = os.path.join(home_dir, tool_dir_name, suite_dir_name)

    # Lancement de l'instruction
    execute_instruction_on_suite_dir(
        all_instructions, instruction_name, suite_dir, test_dir_name
    )


if __name__ == "__main__":
    main()
