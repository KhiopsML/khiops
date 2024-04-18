import os.path
import sys
import argparse

import _kht_constants as kht
import _kht_utils as utils
import _kht_families as test_families
import _kht_results_management as results
import _kht_standard_instructions as standard_instructions
import _kht_one_shot_instructions as one_shot_instructions

"""
Gestion de l'ensemble des instructions
"""


def apply_instruction_on_suite_dir(
    instruction_function,
    suite_dir,
    input_test_dir_name,
    min_test_time=None,
    max_test_time=None,
):
    """Application d'une instruction sur une suite ou sur un repertoire de test specifique"""
    assert suite_dir != ""

    # Erreur si repertoire de suite absent
    if not os.path.isdir(suite_dir):
        utils.fatal_error("missing directory for test suite " + suite_dir)

    # Collecte des sous-repertoires de test
    test_list = []
    # Cas avec un repertoire de test specifique
    if input_test_dir_name is not None:
        if os.path.isdir(os.path.join(suite_dir, input_test_dir_name)):
            test_list.append(input_test_dir_name)
    # Cas avec une suite de test
    else:
        for name in os.listdir(suite_dir):
            if os.path.isdir(os.path.join(suite_dir, name)):
                test_list.append(name)

    # Tri pour assurer la reproductibilite inter plateforme
    test_list.sort()

    # Execution de l'instruction
    for test_dir_name in test_list:
        # lanceur d'instruction sur un directory
        test_dir = os.path.realpath(os.path.join(suite_dir, test_dir_name))
        # verification de l'existence du directory
        if not os.path.isdir(test_dir):
            utils.fatal_error("directory " + test_dir + " does not exist")
        # On ne prend en compte que les tests compatibles avedc les contraintes de temps
        if results.is_results_ref_dir_time_selected(
            test_dir, min_test_time, max_test_time
        ):
            # Application de l'instruction
            current_dir = os.getcwd()
            os.chdir(test_dir)
            instruction_function(test_dir)
            os.chdir(current_dir)
    # Message synthetique de fin
    suite_dir_name = utils.dir_name(suite_dir)
    tool_dir_name = utils.parent_dir_name(suite_dir, 1)
    if input_test_dir_name is None:
        print("DONE\t" + tool_dir_name + "\t" + suite_dir_name)
    else:
        print(
            "done\t"
            + tool_dir_name
            + "\t"
            + suite_dir_name
            + "\t"
            + input_test_dir_name
        )


def apply_instruction_on_learning_test_tree(
    home_dir,
    input_tool_dir_name,
    input_suite_dir_name,
    input_test_dir_name,
    instruction_function,
    family,
    **kwargs
):
    """Applique une instruction un ensemble de suites de tests
    Toute ou partie de l'arborescence est prise en compte selon la specification
     des operandes tool_dir_name, suite_dir_name, test_dir_name, qui peuvent etre None sinon.
    - home_dir: repertoire principal de l'aborescence source
    - tool_dir_name, suite_dir_name, test_dir_name: pour ne prendre en compte qu'une sous-partie
      de l'arborescence source si ces oprande ne sont pas None
    - instruction_function: instruction a appliquee
    - family: famille utilise pour choisir la sous-partie des suites a exporter
    - kwargs: argument optionnels de la ligne de commande
    """

    # Tous les outils sont a prendre en compte si on est a la racine
    if input_tool_dir_name is None:
        used_tool_names = kht.TOOL_NAMES
    # Sinon, seul l'outil correspondant au tool dir est a tester
    else:
        tool_name = kht.TOOL_NAMES_PER_DIR_NAME[input_tool_dir_name]
        used_tool_names = [tool_name]

    # Cas d'un seul outil avec un repertoire de suite au de test specifique
    # Dans ce cas, on ignore la famille
    if input_suite_dir_name is not None:
        suite_dir = os.path.join(home_dir, input_tool_dir_name, input_suite_dir_name)
        apply_instruction_on_suite_dir(
            instruction_function, suite_dir, input_test_dir_name, **kwargs
        )
    # Cas d'un ou plusieurs outils, ou il faut utiliser les suites de la famille specifiee
    else:
        assert len(used_tool_names) >= 1
        for tool_name in used_tool_names:
            tool_dir_name = kht.TOOL_DIR_NAMES[tool_name]
            if family == test_families.ALL:
                test_suites = utils.sub_dirs(os.path.join(home_dir, tool_dir_name))
            else:
                test_suites = test_families.FAMILY_TEST_SUITES[family, tool_name]
            # Parcours de toutes les suites
            for name in test_suites:
                suite_dir = os.path.join(home_dir, tool_dir_name, name)
                if os.path.isdir(suite_dir):
                    apply_instruction_on_suite_dir(
                        instruction_function, suite_dir, None, **kwargs
                    )
                else:
                    print("error : suite directory not found: " + suite_dir)


def register_all_instructions():
    """Enregistrement de toutes les instructions standards et a usage unique
    Retourne un dictionnaire des instructions, et le nombre d'instructions standards
    """

    instructions1 = standard_instructions.register_standard_instructions()
    instructions2 = one_shot_instructions.register_one_shot_instructions()
    # L'operateur d'union entre dictionnaires '|' n'est supporte que depuis python 3.9
    all_instructions = instructions1
    all_instructions.update(instructions2)
    return all_instructions, len(instructions1)


def main():
    """Fonction principale d'application systematique d'une instruction sur une suite de test"""

    def build_usage_help(
        help_command,
        help_instruction,
        help_tool_dir_name=None,
        help_suite_dir_name=None,
        help_test_dir_name=None,
        help_options=None,
    ):
        """Construction d'une ligne d'aide pour un usage de la commande test"""
        source_dir = os.path.join(".", kht.LEARNING_TEST)
        if help_test_dir_name is not None:
            source_dir = os.path.join(
                source_dir, help_tool_dir_name, help_suite_dir_name, help_test_dir_name
            )
        elif help_suite_dir_name is not None:
            source_dir = os.path.join(
                source_dir, help_tool_dir_name, help_suite_dir_name
            )
        elif help_tool_dir_name is not None:
            source_dir = os.path.join(source_dir, help_tool_dir_name)
        usage_help = help_command + " " + source_dir + " " + help_instruction
        if help_options is not None:
            usage_help += " " + help_options
        return usage_help

    # Enregistrement de toutes les instructions
    (
        all_instructions,
        standard_instruction_number,
    ) = register_all_instructions()

    # Nom du script
    script_file_name = os.path.basename(__file__)
    script_name = os.path.splitext(script_file_name)[0]

    # Ajout d'exemples d'utilisation
    epilog = ""
    epilog += "Usage examples"
    epilog += "\n  " + build_usage_help(script_name, "errors")
    epilog += "\n  " + build_usage_help(
        script_name,
        "logs",
        kht.TOOL_DIR_NAMES[kht.KHIOPS],
        "Standard",
        "Iris",
    )
    epilog += "\n  " + build_usage_help(
        script_name,
        "errors",
        kht.TOOL_DIR_NAMES[kht.COCLUSTERING],
        help_options="-f basic",
    )

    # Affichage de la liste des instructions disponibles, en la formattant au mieux
    instructions_help = ""
    max_id_len = 0
    for instruction_id in all_instructions:
        max_id_len = max(max_id_len, len(instruction_id))
    for index, instruction_id in enumerate(all_instructions):
        (instruction_function, instruction_label) = all_instructions[instruction_id]
        if index == standard_instruction_number:
            instructions_help += "\none-shot instructions"
        instructions_help += (
            "\n  " + instruction_id.ljust(max_id_len + 1) + instruction_label
        )

    # Parametrage de l'analyse de la ligne de commande
    parser = argparse.ArgumentParser(
        prog=script_name,
        description="apply instruction (ex: errors) on a subset of test dirs",
        epilog=epilog,
        formatter_class=utils.get_formatter_class(script_name),
    )

    # Arguments positionnels
    utils.argument_parser_add_source_argument(parser)
    parser.add_argument(
        "instruction",
        help="instruction to apply" + instructions_help,
    )

    # Arguments optionnels standards
    utils.argument_parser_add_family_argument(parser)
    utils.argument_parser_add_processes_argument(parser)
    utils.argument_parser_add_forced_platform_argument(parser)
    utils.argument_parser_add_limit_test_time_arguments(parser)

    # Analyse de la ligne de commande
    args = parser.parse_args()

    # Verification de l'argument source
    (
        home_dir,
        tool_dir_name,
        suite_dir_name,
        test_dir_name,
    ) = utils.argument_parser_check_source_argument(parser, args.source)

    # Verification de l'argument instruction
    # On n'utilise pas le parametre 'choices' de add_argument pour eviter
    # d'avoir des messages d'erreur tres long comportant toutes les valeurs possibles
    if args.instruction not in all_instructions:
        parser.error(
            "argument instruction: unknown instruction '" + args.instruction + "'"
        )

    # Verification des arguments optionnels
    utils.argument_parser_check_processes_argument(parser, args.n)
    utils.argument_parser_check_limit_test_time_arguments(
        parser, args.min_test_time, args.max_test_time
    )

    # Memorisation des variables globales de gestion du contexte des resultats de reference
    results.process_number = args.n
    results.forced_platform = args.forced_platform

    # Acces a l'instruction a executer
    (instruction_function, instruction_label) = all_instructions[args.instruction]

    # Lancement de la commande
    apply_instruction_on_learning_test_tree(
        home_dir,
        tool_dir_name,
        suite_dir_name,
        test_dir_name,
        instruction_function,
        args.family,
        min_test_time=args.min_test_time,
        max_test_time=args.max_test_time,
    )


if __name__ == "__main__":
    utils.set_flushed_outputs()
    main()
