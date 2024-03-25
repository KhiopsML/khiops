import os
import sys
import os.path
import argparse

import _kht_constants as kht
import _kht_utils as utils
import _kht_families as test_families
import _kht_check_results as check
import kht_export

"""
Collecte de tout ou partie des resultats de test
"""

# Liste des types de collecte
COLLECT_ALL = "all"
COLLECT_WARNINGS = "warnings"
COLLECT_ERRORS = "errors"
COLLECT_TYPES = [COLLECT_ALL, COLLECT_WARNINGS, COLLECT_ERRORS]
assert len(set(COLLECT_TYPES)) == len(COLLECT_TYPES), (
    "Collect types " + str(COLLECT_TYPES) + " must not contain duplicates"
)

# Libelles des types de collecte
COLLECT_TYPE_LABELS = {
    COLLECT_ALL: "collect all test dir results",
    COLLECT_WARNINGS: "collect test dir results only in case of wanings or errors",
    COLLECT_ERRORS: "collect test dir results only in case or errors",
}
assert set(COLLECT_TYPE_LABELS) == set(
    COLLECT_TYPES
), "A label must be defined for each collect type"


def collect_learning_test_results(
    home_dir,
    input_tool_dir_name,
    input_suite_dir_name,
    input_test_dir_name,
    destination,
    family,
    collect_type,
):
    """
    Sauvegarde de l'arborescence de LearningTest
    Toute ou partie de l'arborescence source est sauvegardee selon la specification
     des operandes tool_dir_name, suite_dir_name, test_dir_name, qui peuvent etre None sinon.
    - home_dir: repertoire principal de l'aborescence source
    - tool_dir_name, suite_dir_name, test_dir_name: pour ne sauvegarder qu'une sous-partie
      de l'arborescence source si ces oprande ne sont pas None
    - destination: repertoire cible contenant l'arborescence sauvegardee
    - family: famille utilise pour choisir la sous-partie des suites a collecter
    - collect_type: pour choisir une sous-partie dees repertoires de tests a collecter
    """
    assert os.path.isdir(home_dir)
    assert family in test_families.TEST_FAMILIES
    assert collect_type in COLLECT_TYPES

    # Nom du repertoire cible
    target_home_dir = os.path.join(destination, kht.LEARNING_TEST + "_results")

    # Verification qu'il n'existe pas deja
    if os.path.isdir(target_home_dir):
        utils.fatal_error("directory " + target_home_dir + " already exists")

    # Creation du repertoire cible
    try:
        os.makedirs(target_home_dir)
    except (IOError, os.error) as why:
        utils.fatal_error(
            "unable to create target dir " + target_home_dir + " " + str(why)
        )

    # Parametrage des noms de fichiers ou repertoire specifiques a ne pas prendre en compte
    forbidden_names = ["__pycache__"]

    # Tous les outils sont a prendre en compte si on est a la racine
    if input_tool_dir_name is None:
        used_tool_names = kht.TOOL_NAMES
    # Sinon, seul l'outil correspondant au tool dir est a tester
    else:
        tool_name = kht.TOOL_NAMES_PER_DIR_NAME[input_tool_dir_name]
        used_tool_names = [tool_name]

    # Memorisation des repertoires intermediaires crees pour ne les creer que si necessaire
    created_tool_dirs = {}
    created_suite_dirs = {}

    # Parcours des repertoires des outils pour en recuperer les resultats des suites de la famille
    collected_test_dir_number = 0
    for tool_name in used_tool_names:
        tool_dir_name = kht.TOOL_DIR_NAMES[tool_name]
        # Chemin du repertoire de l'outil cible
        target_tool_dir = os.path.join(
            target_home_dir,
            tool_dir_name,
        )
        # Recherche des suites a utiliser
        if input_suite_dir_name is not None:
            assert tool_dir_name is not None
            suite_dir_names = [input_suite_dir_name]
        elif family == test_families.ALL:
            suite_dir_names = utils.sub_dirs(os.path.join(home_dir, tool_dir_name))
        else:
            suite_dir_names = test_families.FAMILY_TEST_SUITES[family, tool_name]
        # Parcours des suites de la famille
        for suite_dir_name in suite_dir_names:
            source_suite_dir = os.path.join(
                home_dir,
                tool_dir_name,
                suite_dir_name,
            )
            if os.path.isdir(source_suite_dir):
                # Chemin du repertoire de suite cible
                target_suite_dir = os.path.join(
                    target_home_dir,
                    tool_dir_name,
                    suite_dir_name,
                )
                # Repertoires de test a utiliser
                if input_test_dir_name is not None:
                    test_dir_names = [input_test_dir_name]
                else:
                    test_dir_names = os.listdir(source_suite_dir)
                # Parcours des repertoires de test de la suite
                for test_dir_name in test_dir_names:
                    source_test_dir = os.path.join(source_suite_dir, test_dir_name)
                    collect_results = False
                    if os.path.isdir(source_test_dir):
                        # On decide s'il faut collecter les results en fonction des type de collecte
                        # Cas d'une collecte systematique
                        if collect_type == COLLECT_ALL:
                            collect_results = True
                        # Cas ou cela depend des message ou erreurs du log de comparaison
                        else:
                            # Le log de comparaison doit etre disponible
                            log_file_path = os.path.join(
                                source_test_dir, kht.COMPARISON_RESULTS_LOG
                            )
                            collect_results = os.path.isfile(log_file_path)
                            # Analyse du log de comparaison s'il est disponible
                            if collect_results:
                                (
                                    error_number,
                                    warning_number,
                                    summary_infos,
                                    files_infos,
                                ) = check.analyse_comparison_log(source_test_dir)
                                # Cas de la collecte en cas de warnings
                                if collect_type == COLLECT_WARNINGS:
                                    collect_results = (
                                        error_number > 0 or warning_number > 0
                                    )
                                # Cas de la collecte d'erreurs
                                elif collect_type == COLLECT_ERRORS:
                                    collect_results = error_number > 0

                    # Copie du repertoire de test si necessaire
                    if collect_results:
                        # Creation des repertoires intermediaires uniquement si necessaire
                        if target_tool_dir not in created_tool_dirs:
                            utils.make_dir(target_tool_dir)
                            created_tool_dirs[target_tool_dir] = True
                        if target_suite_dir not in created_suite_dirs:
                            utils.make_dir(target_suite_dir)
                            created_suite_dirs[target_suite_dir] = True

                        # Copie du repertoire de test
                        target_test_dir = os.path.join(target_suite_dir, test_dir_name)
                        collected_test_dir_number += 1
                        kht_export.copy_subdirs(
                            source_test_dir,
                            target_test_dir,
                            ignore_list=forbidden_names,
                        )
            else:
                print("error : suite directory not found: " + source_suite_dir)
    print("\tCollected test dir number: " + str(collected_test_dir_number))


def main():
    def build_usage_help(
        help_command,
        help_tool_dir_name=None,
        help_suite_dir_name=None,
        help_test_dir_name=None,
        help_options=None,
    ):
        """Construction d'une lige d'aide pour un usage de la command test"""
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
        usage_help = help_command + " " + source_dir + " (target dir)"
        if help_options is not None:
            usage_help += " " + help_options
        return usage_help

    # Nom du script
    script_file_name = os.path.basename(__file__)
    script_name = os.path.splitext(script_file_name)[0]

    # Ajout d'exemples d'utilisation
    epilog = ""
    epilog += "Usage examples"
    epilog += "\n  " + build_usage_help(script_name)
    epilog += "\n  " + build_usage_help(
        script_name, help_options="-f basic --collect-type errors"
    )
    epilog += "\n  " + build_usage_help(
        script_name,
        kht.TOOL_DIR_NAMES[kht.KHIOPS],
        help_options="--collect-type errors",
    )
    epilog += "\n  " + build_usage_help(
        script_name,
        kht.TOOL_DIR_NAMES[kht.KHIOPS],
        "Standard",
        "Iris",
    )

    # Affichage de la liste des types de collecte, en la formattant au mieux
    collect_types_help = ""
    max_collect_type_len = 0
    for collect_type in COLLECT_TYPES:
        max_collect_type_len = max(max_collect_type_len, len(collect_type))
    for collect_type in COLLECT_TYPES:
        label = COLLECT_TYPE_LABELS[collect_type]
        collect_types_help += (
            "\n  " + collect_type.ljust(max_collect_type_len + 1) + label
        )
        if collect_type == COLLECT_ALL:
            collect_types_help += " (default type)"

    # Parametrage de l'analyse de la ligne de commande
    parser = argparse.ArgumentParser(
        prog=script_name,
        description="collect a subset of a source "
        + kht.LEARNING_TEST
        + " test dirs in a target dir named "
        + kht.LEARNING_TEST
        + "_results under the destination directory",
        epilog=epilog,
        formatter_class=utils.get_formatter_class(script_name),
    )

    # Arguments positionnels
    utils.argument_parser_add_source_argument(parser)
    utils.argument_parser_add_dest_argument(parser)

    # Arguments optionnels standards
    utils.argument_parser_add_family_argument(parser)

    # Argument sur le types de collecte
    parser.add_argument(
        "--collect-type",
        help="collect type" + collect_types_help,
        choices=COLLECT_TYPES,
        default=COLLECT_ALL,
        metavar="type",
        action="store",
    )

    # Analyse de la ligne de commande
    args = parser.parse_args()

    # Verification de l'argument source
    (
        home_dir,
        tool_dir_name,
        suite_dir_name,
        test_dir_name,
    ) = utils.argument_parser_check_source_argument(parser, args.source)

    # Verification de l'argument destination
    utils.argument_parser_check_destination_dir(parser, home_dir, args.dest)

    # Lancement de la commande
    collect_learning_test_results(
        home_dir,
        tool_dir_name,
        suite_dir_name,
        test_dir_name,
        args.dest,
        args.family,
        args.collect_type,
    )


if __name__ == "__main__":
    utils.set_flushed_outputs()
    main()
