import os
import sys
import shutil
import os.path

import _learning_test_constants as lt
import _learning_test_config as learning_test_config
import _results_management as results

""" Sauvegarde du contenu de LearningTest d'un directory local, sur le reseau, pour une version donnee

Variables d'environnement necessaires:
   netroot: racine des directories utilisateur sur le reseau

La sauvergarde de LearningTest se fait sous netroot/archive/dir/version
"""


def make_learning_test_version(version: str, option: str = ""):
    """
    Sauvegarde d'une version de LearningTest
    :param version: version de Learning test
    :param option: option (default: ""),
      "scripts" pour les fichiers de script uniquement,
      "dataset" pour les jeux de donnees uniquement,
      "reference" pour les scripts plus les resultats de reference
    :return:
    """

    def is_empty_dir(source_dir, ignore_list=None):
        """
        Verification si un repertoire est vide
        :param source_dir: nom des fichiers ou repertoires a ignorer de facon siliencieuse
        :param ignore_list:
        :return:
        """
        names = os.listdir(source_dir)
        for name in names:
            if ignore_list is not None:
                if name not in names:
                    return False
                elif (
                    lt.RESULTS_REF in ignore_list
                    and results.is_candidate_results_ref_dir(name)
                ):
                    return False
            else:
                return False
        return True

    def copy_subdirs(
        source_dir, target_dir, ignore_list=None, warning_list=None, script_only=False
    ):
        """
        Copie de sous-repertoires d'un repertoire racine source vers un repertoire racine cible
        :param source_dir:
        :param target_dir:
        :param ignore_list: nom des fichiers ou repertoires a ignorer de facon siliencieuse
        :param warning_list: nom des fichiers ou repertoires a ignorer avec des warnings
        :param script_only: pour indiquer de sauver uniquement les scripts
        :return:
        """
        # Retour si le repertoire est vide
        if is_empty_dir(source_dir):
            return
        # Create target dir
        if not os.path.isdir(target_dir):
            os.mkdir(target_dir)
        # Parcours le repertoire source et propage la copie
        names = os.listdir(source_dir)
        # Test si on est dans un repertoire de test elementaire
        is_test_dir = False
        for name in names:
            if name == lt.TEST_PRM:
                is_test_dir = True
        # Copie du contenu
        for name in names:
            copy = True
            if ignore_list is not None:
                if name in ignore_list:
                    copy = False
                elif (
                    lt.RESULTS_REF in ignore_list
                    and results.is_candidate_results_ref_dir(name)
                ):
                    copy = False
            if warning_list is not None:
                if name in warning_list:
                    copy = False
                elif (
                    lt.RESULTS_REF in warning_list
                    and results.is_candidate_results_ref_dir(name)
                ):
                    copy = False
                if not copy:
                    print(
                        "warning: found "
                        + name
                        + " in directory "
                        + source_root
                        + " (ignored)"
                    )
            if is_test_dir and script_only and name != lt.TEST_PRM:
                copy = False
            if copy:
                source_path = os.path.join(source_dir, name)
                target_path = os.path.join(target_dir, name)
                if os.path.isdir(source_path):
                    try:
                        os.mkdir(target_path)
                    except (IOError, os.error) as why:
                        print(
                            "Cannot create directory %s: %s" % (target_path, str(why))
                        )
                    copy_subdirs(
                        source_path,
                        target_path,
                        ignore_list=ignore_list,
                        warning_list=warning_list,
                        script_only=(option == "scripts"),
                    )
                elif os.path.isfile(source_path):
                    try:
                        shutil.copyfile(source_path, target_path)
                    except (IOError, os.error) as why:
                        print(
                            "Cannot copy file %s to %s: %s"
                            % (source_path, target_path, str(why))
                        )

    assert (
        option == ""
        or option == "datasets"
        or option == "references"
        or option == "scripts"
    )
    # Verification des variables d'environnement
    netroot = os.getenv("NETROOT")
    if netroot is None:
        print("variable NETROOT must be defined")
        exit(0)

    # Acecs au repertoire source
    source_root = os.path.join(
        learning_test_config.learning_test_root, lt.LEARNING_TEST
    )

    # Le repertoire source sera sauvegarde sous netroot\archive\dir\version
    target_root = os.path.join(netroot, "archive", LEARNING_TEST, version)
    if option != "":
        target_root = target_root + "_" + option

    # Verification de la version a creer
    if os.path.isdir(target_root):
        option_label = "" if option == "" else "(" + option + ")"
        print(
            "Version "
            + version
            + " of LearningTest"
            + option_label
            + " already exists on "
            + os.path.join(netroot, "archive")
        )
        exit(0)

    # Creation du directory de la version a sauvegarder
    if not os.path.isdir(target_root):
        os.makedirs(target_root)

    # Parametrage des noms de fichiers ou repertoire specifiques
    dataset_dirs = ["datasets", "MTdatasets", "TextDatasets", "UnusedDatasets"]
    test_dirs = ["cmd", "doc", "TestCoclustering", "TestKhiops", "TestKNITransfer"]
    forbidden_names = ["__pycache__", "modl", lt.RESULTS, lt.COMPARISON_RESULTS_LOG]
    if option != "references":
        forbidden_names.append(lt.RESULTS_REF)

    # Cas de la copie des jeux de donnees
    if option == "datasets":
        for dataset in dataset_dirs:
            copy_subdirs(
                os.path.join(source_root, dataset),
                os.path.join(target_root, dataset),
                warning_list=forbidden_names,
            )
    # Cas de la copie des script et eventuellement des resultats de reference
    else:
        for dataset in test_dirs:
            copy_subdirs(
                os.path.join(source_root, dataset),
                os.path.join(target_root, dataset),
                ignore_list=forbidden_names,
                script_only=True,
            )


if __name__ == "__main__":
    # Verification des parametres
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print("MakeLearningTestVersion [version] [Option]")
        print(
            "Copy LearningTest from local directory in a version under network archive directory"
        )
        print(
            "  Copy most files, except from datasets, "
            + lt.RESULTS
            + ", or "
            + lt.RESULTS_REF
            + " directories, if no option is specified."
        )
        print("  Available options:")
        print("    scripts: only script files")
        print("    references: default plus reference result files")
        print("    datasets: copy only datasets")
        exit(0)
    elif len(sys.argv) == 2:
        make_learning_test_version(sys.argv[1])
    elif len(sys.argv) == 3:
        options = ["scripts", "references", "datasets"]
        if sys.argv[2] in options:
            make_learning_test_version(sys.argv[1], option=sys.argv[2])
        else:
            print("MakeLearningTestVersion: Invalid option " + sys.argv[2])
    print("DONE")
