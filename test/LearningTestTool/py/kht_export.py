import os
import os.path
import argparse

import _kht_constants as kht
import _kht_utils as utils
import _kht_families as test_families
import _kht_results_management as results

"""
Export de la sous-partie d'un repertoire LearningTest pour la famille de test specifiee
"""

# Liste des types d'export
EXPORT_ALL = "all"
EXPORT_REFERENCES = "references"
EXPORT_SCRIPTS = "scripts"
EXPORT_DATASETS = "datasets"
EXPORT_TYPES = [EXPORT_ALL, EXPORT_REFERENCES, EXPORT_SCRIPTS, EXPORT_DATASETS]
assert len(set(EXPORT_TYPES)) == len(EXPORT_TYPES), (
    "Export types " + str(EXPORT_TYPES) + " must not contain duplicates"
)

# Libelles des types d'exports
EXPORT_TYPE_LABELS = {
    EXPORT_ALL: "complete directory tree",
    EXPORT_REFERENCES: "only test dirs with the scripts and references results",
    EXPORT_SCRIPTS: "only test dirs with the scripts",
    EXPORT_DATASETS: "only datasets",
}
assert set(EXPORT_TYPE_LABELS) == set(
    EXPORT_TYPES
), "A label must be defined for each export type"


def export_learning_test_tree(
    home_dir,
    input_tool_dir_name,
    input_suite_dir_name,
    input_test_dir_name,
    destination,
    family,
    export_type,
):
    """
    Sauvegarde de l'arborescence de LearningTest
    Toute ou partie de l'arborescence source est sauvegardee selon la specification
     des operandes tool_dir_name, suite_dir_name, test_dir_name, qui peuvent etre None sinon.
    - home_dir: repertoire principal de l'aborescence source
    - tool_dir_name, suite_dir_name, test_dir_name: pour ne sauvegarder qu'une sous-partie
      de l'arborescence source si ces oprande ne sont pas None
    - destination: repertoire cible contenant l'arborescence sauvegardee
    - family: famille utilise pour choisir la sous-partie des suites a exporter
    - export_type: pour choisir une sous-partie de l'arborescence a exporter
    L'aborescence est sauvegardee dans <destination>/LearningTest avec _<export_type>
     en suffixe si lle type d'export est different de 'all'
    """
    assert os.path.isdir(home_dir)
    assert family in test_families.TEST_FAMILIES
    assert export_type in EXPORT_TYPES

    # Nom du repertoire cible
    target_home_dir = os.path.join(destination, kht.LEARNING_TEST)
    if export_type != EXPORT_ALL:
        target_home_dir += "_" + export_type

    # Verification qu'il n'existe pas deja
    if os.path.isdir(target_home_dir):
        utils.fatal_error("directory " + target_home_dir + " already exists")

    # Creation du repertoire cible
    try:
        os.makedirs(target_home_dir)
    except (IOError, os.error) as why:
        utils.fatal_error(
            "Unable to create target dir " + target_home_dir + " " + str(why)
        )

    # Parametrage des noms de fichiers ou repertoire specifiques a ne pas prendre en compte
    export_test_dirs = export_type in [EXPORT_ALL, EXPORT_SCRIPTS, EXPORT_REFERENCES]
    forbidden_names = ["__pycache__", kht.RESULTS, kht.COMPARISON_RESULTS_LOG]
    if export_type not in [EXPORT_ALL, EXPORT_REFERENCES]:
        # On ne sauvegarde pas les resultats de references dans tous leur contextes
        forbidden_names.append(kht.RESULTS_REF)

    # Initialisation des ensembles de dataset utilises par repertoire de jeux de donnees
    used_dataset_collections = {}
    for name in kht.DATASET_COLLECTION_NAMES:
        used_dataset_collections[name] = {}

    # Tous les outils sont a prendre en compte si on est a la racine
    if input_tool_dir_name is None:
        used_tool_names = kht.TOOL_NAMES
    # Sinon, seul l'outil correspondant au tool dir est a tester
    else:
        tool_name = kht.TOOL_NAMES_PER_DIR_NAME[input_tool_dir_name]
        used_tool_names = [tool_name]

    # Parcours des repertoires des outils pour en recopier les suites par famille
    exported_test_dir_number = 0
    for tool_name in used_tool_names:
        tool_dir_name = kht.TOOL_DIR_NAMES[tool_name]
        # Recherche des suites a utiliser
        if input_suite_dir_name is not None:
            assert tool_dir_name is not None
            suite_dir_names = [input_suite_dir_name]
        elif family == test_families.ALL:
            suite_dir_names = utils.sub_dirs(os.path.join(home_dir, tool_dir_name))
        else:
            suite_dir_names = test_families.FAMILY_TEST_SUITES[family, tool_name]
        # Chemin du repertoire de l'outil
        target_tool_dir = os.path.join(
            target_home_dir,
            tool_dir_name,
        )
        target_tool_dir_created = False
        # Parcours des suites de la famille
        for suite_dir_name in suite_dir_names:
            source_suite_dir = os.path.join(
                home_dir,
                tool_dir_name,
                suite_dir_name,
            )
            if os.path.isdir(source_suite_dir):
                # Chemin du repertoire de suite
                target_suite_dir = os.path.join(
                    target_home_dir,
                    tool_dir_name,
                    suite_dir_name,
                )
                target_suite_dir_created = False
                # Repertoires de test a utiliser
                if input_test_dir_name is not None:
                    test_dir_names = [input_test_dir_name]
                else:
                    test_dir_names = os.listdir(source_suite_dir)
                # Parcours des repertoires de test de la suite
                for test_dir_name in test_dir_names:
                    source_test_dir = os.path.join(source_suite_dir, test_dir_name)
                    # Gestion des fichiers de la suite (ex: readme), a conserver
                    if os.path.isfile(source_test_dir):
                        if export_test_dirs:
                            source_file_name = source_test_dir
                            target_file_name = os.path.join(
                                target_suite_dir, test_dir_name
                            )
                            utils.copy_file(source_file_name, target_file_name)
                    # Gestion des repertoires de la suite: a exporter ou analyser
                    if os.path.isdir(source_test_dir):
                        # Export du repertoire de test si necessaire
                        if export_test_dirs:
                            # Creation du repertoire de l'outil uniquement si necessaire
                            if not target_tool_dir_created:
                                utils.make_dir(target_tool_dir)
                                target_tool_dir_created = os.path.isdir(target_tool_dir)
                            # Creation du repertoire de suite uniquement si necessaire
                            if not target_suite_dir_created:
                                utils.make_dir(target_suite_dir)
                                target_suite_dir_created = os.path.isdir(
                                    target_suite_dir
                                )
                            # Copie du repertoire de test
                            target_test_dir = os.path.join(
                                target_suite_dir, test_dir_name
                            )
                            exported_test_dir_number += 1
                            copy_subdirs(
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

    # Recopie des jeux de donnees selon les types d'export utilises si on exporte tout
    if export_type in [EXPORT_ALL, EXPORT_DATASETS]:
        # Recopie exhaustive des jeux de donnees utilises si on est a la racine avec toutes les suites
        if input_tool_dir_name is None and family == test_families.ALL:
            for dataset_collection in kht.DATASET_COLLECTION_NAMES:
                source_dataset_collection_dir = os.path.join(
                    home_dir, dataset_collection
                )
                target_dataset_collection_dir = os.path.join(
                    target_home_dir, dataset_collection
                )
                if os.path.isdir(source_dataset_collection_dir):
                    copy_subdirs(
                        source_dataset_collection_dir,
                        target_dataset_collection_dir,
                        ignore_list=forbidden_names,
                    )
        # Recopie des jeux de donnees utilises, sans selection sinon
        else:
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
                        copy_subdirs(
                            source_dataset_dir,
                            target_dataset_dir,
                            ignore_list=forbidden_names,
                        )
    print("\tExported test dir number: " + str(exported_test_dir_number))


def copy_subdirs(source_dir, target_dir, ignore_list=None):
    """
    Copie recursive d'un repertoires source vers un repertoire cible
    :param source_dir:
    :param target_dir:
    :param ignore_list: nom des fichiers ou repertoires a ignorer de facon siliencieuse
    :return:
    """

    def is_dir_ignored(input_dir):
        """
        Verification si un repertoire peut etre ignorer, s'il est vide ou si son contenu peut eTre ignorer
        """
        input_names = os.listdir(input_dir)
        # Ignorer si le repertoire est vide
        if len(input_names) == 0:
            return True
        # Ne pas ignorer si non vide at iren a ignorer
        elif ignore_list is None and len(input_names) > 0:
            return False
        # Sinon, on parcours le repertoire pour voir s'il a un contenu a ignorer
        else:
            ignore_results = kht.RESULTS_REF in ignore_list
            for input_name in input_names:
                # Ne pas ignorer car le contenu n'est pas a ignorer
                if input_name not in ignore_list:
                    return False
                # Idem pour le cas particulier des resultst de reference
                elif ignore_results and not results.is_candidate_results_ref_dir(
                    input_name
                ):
                    return False
            # Peut etre ignorer si on a rien detecte
            return True

    # Retour si on peut ignorer le repertoire
    if is_dir_ignored(source_dir):
        return
    # Creation du repertoire cible
    if not os.path.isdir(target_dir):
        utils.make_dir(target_dir)
    # Parcours du repertoire source et propagation de la copie
    names = os.listdir(source_dir)
    # Copie du contenu
    ignore_reference_results = kht.RESULTS_REF in ignore_list
    for name in names:
        copy = True
        if ignore_list is not None:
            if name in ignore_list:
                copy = False
            elif ignore_reference_results and results.is_candidate_results_ref_dir(
                name
            ):
                copy = False
        if copy:
            source_path = os.path.join(source_dir, name)
            target_path = os.path.join(target_dir, name)
            if os.path.isdir(source_path):
                copy_subdirs(
                    source_path,
                    target_path,
                    ignore_list=ignore_list,
                )
            elif os.path.isfile(source_path):
                utils.copy_file(source_path, target_path)


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
    epilog += "\n  " + build_usage_help(script_name, help_options="-f all")
    epilog += "\n  " + build_usage_help(
        script_name, help_options="-f all --export-type references"
    )
    epilog += "\n  " + build_usage_help(
        script_name,
        kht.TOOL_DIR_NAMES[kht.KHIOPS],
        help_options="--export-type datasets",
    )
    epilog += "\n  " + build_usage_help(
        script_name,
        kht.TOOL_DIR_NAMES[kht.KHIOPS],
        "Standard",
        "Iris",
    )

    # Affichage de la liste des types d'exports, en la formattant au mieux
    export_types_help = ""
    max_export_type_len = 0
    for export_type in EXPORT_TYPES:
        max_export_type_len = max(max_export_type_len, len(export_type))
    for export_type in EXPORT_TYPES:
        label = EXPORT_TYPE_LABELS[export_type]
        export_types_help += "\n  " + export_type.ljust(max_export_type_len + 1) + label
        if export_type == EXPORT_ALL:
            export_types_help += " (default type)"

    # Parametrage de l'analyse de la ligne de commande
    parser = argparse.ArgumentParser(
        prog=script_name,
        description="export a subset of a source "
        + kht.LEARNING_TEST
        + " tree to a target dir named "
        + kht.LEARNING_TEST
        + " under the destination directory,"
        "\n the target directory is suffixed by _<type> if the export type is not 'all'",
        epilog=epilog,
        formatter_class=utils.get_formatter_class(script_name),
    )

    # Arguments positionnels
    utils.argument_parser_add_source_argument(parser)
    utils.argument_parser_add_dest_argument(parser)

    # Arguments optionnels standards
    utils.argument_parser_add_family_argument(parser)

    # Argument sur le types d'export
    parser.add_argument(
        "--export-type",
        help="export type" + export_types_help,
        choices=EXPORT_TYPES,
        default=EXPORT_ALL,
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
    export_learning_test_tree(
        home_dir,
        tool_dir_name,
        suite_dir_name,
        test_dir_name,
        args.dest,
        args.family,
        args.export_type,
    )


if __name__ == "__main__":
    main()
