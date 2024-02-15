import os
import sys
import os.path

import _kht_constants as kht
import _kht_utils as utils
import _kht_families as test_families
import _kht_results_management as results

"""
Sauvegarde de tout ou partie d'un repertoire LearningTest
"""

# Liste des options de sauvegarde
SAVE_ALL = "all"
SAVE_DATASETS = "datasets"
SAVE_REFERENCES = "references"
SAVE_SCRIPTS = "scripts"
SAVE_OPTIONS = [SAVE_ALL, SAVE_DATASETS, SAVE_REFERENCES, SAVE_SCRIPTS]
assert len(set(SAVE_OPTIONS)) == len(SAVE_OPTIONS), (
    "Save options " + str(SAVE_OPTIONS) + " must not contain duplicates"
)

# On verifie que les options de sauvegardes et les familles sont exclusives
# En effet, elles servent toutes les deux a suffixer les repertoires cibles de sauvegarde en LearningTest_<suffix>
assert len(set(SAVE_OPTIONS + test_families.TEST_FAMILIES)) == len(
    set(SAVE_OPTIONS)
) + len(set(test_families.TEST_FAMILIES)), (
    "Save options "
    + str(SAVE_OPTIONS)
    + " and families "
    + str(test_families.TEST_FAMILIES)
    + " must not have common names"
)

# Libelles des options de sauvegarde
SAVE_OPTION_LABELS = {
    SAVE_ALL: "complete directory tree",
    SAVE_DATASETS: "only datasets",
    SAVE_REFERENCES: "only tool dirs with the scripts and references results",
    SAVE_SCRIPTS: "only tool dirs with the scripts",
}
assert set(SAVE_OPTION_LABELS) == set(
    SAVE_OPTIONS
), "A label must be defined for each save option"


def save_learning_test_tree(home_dir, target_root_dir, save_option):
    """
    Sauvegarde de l'arborescence de LearningTest
    - home_dir: repertoire de l'aborescence source
    - target_root_dir: repertoire cible contenant l'arborescence sauvegardee
    - save_option: option pour sauvegarder tout ou partie de l'arborescence
    L'aborescence est sauvegardee dans <target_root_dir>/LearningTest_<save_option>
    """
    assert os.path.isdir(home_dir)
    assert os.path.isdir(target_root_dir)
    assert save_option in SAVE_OPTIONS

    # Nom du repertoire cible
    target_home_dir = os.path.join(
        target_root_dir, kht.LEARNING_TEST + "_" + save_option
    )

    # Verification qu'il n'existe pas deja
    if os.path.isdir(target_home_dir):
        utils.fatal_error("target home dir already exist on " + target_home_dir)

    # Creation du repertoire cible
    os.makedirs(target_home_dir)

    # Parametrage des noms de fichiers ou repertoire specifiques a ne pas prendre en compte
    forbidden_names = ["__pycache__", kht.RESULTS, kht.COMPARISON_RESULTS_LOG]
    if save_option not in [SAVE_ALL, SAVE_REFERENCES]:
        # On ne sauvegarde pas les resultst de references dans tous leur contextes
        forbidden_names.append(kht.RESULTS_REF)

    # Copie des jeux de donnees
    if save_option in [SAVE_ALL, SAVE_DATASETS]:
        for dataset_collection in kht.DATASET_COLLECTION_NAMES:
            source_dir = os.path.join(home_dir, dataset_collection)
            if not os.path.isdir(source_dir):
                print("warning : missing source dataset collection dir " + source_dir)
            else:
                target_dir = os.path.join(target_home_dir, dataset_collection)
                copy_subdirs(
                    source_dir,
                    target_dir,
                    ignore_list=forbidden_names,
                )

    # Copies des scripts ou des resultats de references
    if save_option in [SAVE_ALL, SAVE_SCRIPTS, SAVE_REFERENCES]:
        for tool_name in kht.TOOL_NAMES:
            tool_dir_name = kht.TOOL_DIR_NAMES[tool_name]
            source_dir = os.path.join(home_dir, tool_dir_name)
            if not os.path.isdir(source_dir):
                print("warning : missing tool dir " + source_dir)
            else:
                target_dir = os.path.join(target_home_dir, tool_dir_name)
                copy_subdirs(
                    source_dir,
                    target_dir,
                    ignore_list=forbidden_names,
                )


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
    """Fonction principale de sauvegarde de l'arborescence LearningTest"""
    # Verification des parametres
    if len(sys.argv) != 4:
        script_name = os.path.basename(__file__)
        base_script_name = os.path.splitext(script_name)[0]
        print(
            base_script_name + " (home dir) (target root dir) (save option)\n"
            "  Save " + kht.LEARNING_TEST + " directory tree"
        )
        print(
            "  The home dir is saved under a directory named <target root dir>/"
            + kht.LEARNING_TEST
            + "_<save option>"
        )
        print("  Available save options are:")
        for save_option in SAVE_OPTIONS:
            print("    " + save_option + ": " + SAVE_OPTION_LABELS[save_option])
        exit(0)

    # Recherche des parametres
    home_dir = sys.argv[1]
    target_root_dir = sys.argv[2]
    save_option = sys.argv[3]

    # Analyse de la validite des parametres
    utils.check_home_dir(home_dir)
    utils.check_candidate_root_dir(home_dir, target_root_dir)
    if save_option not in SAVE_OPTIONS:
        utils.fatal_error(
            "save option ("
            + save_option
            + ") must be in "
            + utils.list_to_label(SAVE_OPTIONS)
        )

    # Lancement de la commande
    save_learning_test_tree(home_dir, target_root_dir, save_option)


if __name__ == "__main__":
    main()
