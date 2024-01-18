import os
import platform


"""
Constantes permettant la gestion de la structure des repertoires de LearningTest
et l'analyse des resultats par repertorie de test
"""

# Repertoire racine de l'arborescence de test
LEARNING_TEST = "LearningTest"

# Repertoires des resultats de test et de reference
RESULTS = "results"
RESULTS_REF = "results.ref"

# Fichiers se trouvant d'un repertoire de test
TEST_PRM = "test.prm"
COMPARISON_RESULTS_LOG = "comparisonResults.log"

# Fichiers se trouvant d'un repertoire de resultats
ERR_TXT = "err.txt"
TIME_LOG = "time.log"
STDOUT_ERROR_LOG = "stdout_error.log"
STDERR_ERROR_LOG = "stderr_error.log"
RETURN_CODE_ERROR_LOG = "return_code_error.log"
FATAL_ERROR_FILES = [
    STDOUT_ERROR_LOG,
    STDERR_ERROR_LOG,
    RETURN_CODE_ERROR_LOG,
]

"""
Gestion de la typologie des resultats de test de reference, selon les axes suivants
- computing
  - parallel: si la variable d'environnement KhiopsMPIProcessNumber est definie
  - sequential
- platform
  - fourni par la fonction python platform.system()
  - peut etre forcee par la variable d'environnement KhiopsComparisonPlatform
  - valeurs possibles
    - Darwin (Mac)
    - Linux
    - Windows

On peut memoriser des variantes de resultats de reference selon leur type si necessaire
Dans ce cas, la typlogie est indiquee en suffix du nom de repertoire 'results.ref'
par une liste de type separes par des '-' (AND).
Si pour un type donne, plusieurs valeurs sont possibles, elle sont separees par des "_" (OR).
L'ensemble des variantes est normalise en respectant l'odre des type, et l'ordre de valeurs par type.

Pour chaque type (computing, platform), on recherche la variante la plus specialise
parmi les repertoires de reference candidats.
Chaque repertoire de reference doit avoir un nom normalise, et l'ensemble des repertoires
doit etre complet, minimal et coherent pour tous les types possibles.

Examples de noms de repertoire normalises
- results.ref-sequential
- results.ref-parallel
- results.ref-Windows
- results.ref-Darwin_Linux
- results.ref-parallel-Darwin_Linux

Examples d'ensemble correct de noms de repertoire
- [] : correspond a un seul repertoire results.ref, non cree
- [results.ref]
- [results.ref, results.ref-parallel]: repertoire de reference avec specialisation dans le cas de computing parallel
- [results.ref.parallel, results.ref-sequential]: variante au comportement identique
- [results.ref, results.ref-Darwin_Linux]: : repertoire de reference avec specialisation dans le cas de Max ou Linux
- [results.ref, results.ref-Parallel, results.ref-Parallel-Darwin_Linux]
"""

# Type de resultats de reference
COMPUTING = "computing"
PLATFORM = "platform"
RESULTS_REF_TYPES = [COMPUTING, PLATFORM]

# Valeurs par type de resultats de refences
RESULTS_REF_TYPE_VALUES = {}
RESULTS_REF_TYPE_VALUES[COMPUTING] = ["parallel", "sequential"]
RESULTS_REF_TYPE_VALUES[PLATFORM] = ["Darwin", "Linux", "Windows"]

# Caracteres separateurs utilises dans l'analyse des type de repertoire de reference
AND = "-"
OR = "_"


def get_current_results_ref_context(log_file=None, show=False):
    """Retourne le contexte courant de resultat de reference,
    sous la forme d'un vecteur avec la valeur courante par type
    Une trace est ecrite dans un fichier de log et affichees sur la console si besoin
    """
    return [
        get_context_computing_type(log_file, show),
        get_context_platform_type(log_file, show),
    ]


def get_context_computing_type(log_file=None, show=False):
    """Retourne le type de computing courant
    Base sur la variable d'environnement KhiopsMPIProcessNumber
    Une trace est ecrite dans un fichier de log et affichees sur la console si besoin
    """
    khiops_mpi_process_number = os.getenv("KhiopsMPIProcessNumber")
    if khiops_mpi_process_number is None:
        computing_type = "sequential"
    else:
        computing_type = "parallel"
    assert computing_type in RESULTS_REF_TYPE_VALUES[COMPUTING], (
        COMPUTING
        + " type ("
        + computing_type
        + ") should be in "
        + str(RESULTS_REF_TYPE_VALUES[COMPUTING])
    )
    # Affichhe d'une trace
    if log_file is not None or show:
        message = COMPUTING + " type: " + computing_type
        if khiops_mpi_process_number is not None:
            message += " (process number: " + str(khiops_mpi_process_number) + ")"
        if log_file is not None:
            log_file.write(message + "\n")
        if show:
            print(message)
    return computing_type


def get_context_platform_type(log_file=None, show=False):
    """Retourne le type de computing courant
    Base sur l'OS courant, ou force selon la variable d'environnement KhiopsComparisonPlatform
    Une trace est ecrite dans un fichier de log et affichees sur la console si besoin
    """
    platform_type = os.getenv("KhiopsComparisonPlatform")
    forced_platform_type = platform_type is not None
    if not forced_platform_type:
        platform_type = platform.system()
    assert platform_type in RESULTS_REF_TYPE_VALUES[PLATFORM], (
        PLATFORM
        + " type ("
        + platform_type
        + ") should be in "
        + str(RESULTS_REF_TYPE_VALUES[PLATFORM])
    )
    # Affichhe d'une trace
    if log_file is not None or show:
        message = PLATFORM + " type: " + platform_type
        if forced_platform_type:
            message += " (forced using 'KhiopsComparisonPlatform' env var)"
        if log_file is not None:
            log_file.write(message + "\n")
        if show:
            print(message)
    return platform_type


def get_results_ref_dir(test_dir, log_file=None, show=False):
    """Recherche du repertoire de reference correspondant au contexte courant
    Retourne:
    - le nom du repertoire, ou None en cas d'erreur
    - la liste des repertoires de references candidats, qu'il y a ait erreur ou non
    On retourne results.ref s'il n'y a aucun repertoire de reference ou si c'est le seul rerpertoire candidat
    Les erreurs sont ecrites dans un fichier de log et affichees sur la console si besoin
    """
    assert LEARNING_TEST in test_dir, (
        test_dir + " must be in a sub-directory of " + LEARNING_TEST
    )
    results_ref_context = get_current_results_ref_context()
    candidate_results_ref_dirs = get_candidate_results_ref_dirs(test_dir)
    results_ref_dir = _search_results_ref_dir(
        candidate_results_ref_dirs,
        results_ref_context,
        test_dir_name=os.path.basename(test_dir),
        log_file=log_file,
        show=show,
    )
    return results_ref_dir, candidate_results_ref_dirs


def is_candidate_results_ref_dir(test_dir):
    """Test si un nom correspond a un repertoires candidat a etre des resultats de reference"""
    basename = os.path.basename(test_dir)
    return basename == RESULTS_REF or basename.find(RESULTS_REF + AND) == 0


def get_candidate_results_ref_dirs(test_dir):
    """Recherche de la liste des nom repertoires candidats a etre des resultats de reference
    Il s'agit des repertoire de nom results.ref, avec ou sans contexte"""
    candidate_results_ref_dirs = []
    test_dir_names = os.listdir(test_dir)
    test_dir_names.sort()
    for file_name in test_dir_names:
        if is_candidate_results_ref_dir(file_name):
            file_path = os.path.join(test_dir, file_name)
            # Memorisation du repertoire par contexte si repertoire valide
            assert os.path.isdir(file_path), (
                "file name starting "
                + RESULTS_REF
                + " shoud be a directory ("
                + file_path
                + ")"
            )
            candidate_results_ref_dirs.append(file_name)
    return candidate_results_ref_dirs


def _search_results_ref_dir(
    candidate_reference_dirs,
    searched_context,
    test_dir_name=None,
    log_file=None,
    show=False,
):
    """Recherche du repertoire de reference pour un contexte donne parmi une liste
     de repertoires de reference
    Retourne le nom du repertoire, ou None en cas d'erreur
    Les erreurs sont ecrites dans un fichier de log et affichees sur la console si besoin
    en utilisant le nom du repertoire de test s'il est specifie
    """

    def add_error(candidate_dir, error_message):
        message = "error : "
        if test_dir_name is not None:
            message += test_dir_name + " : "
        if candidate_dir != "":
            message += RESULTS_REF + " dir (" + candidate_dir + "), "
        message += error_message
        if log_file is not None:
            log_file.write(message + "\n")
        if show:
            print(message)

    results_ref_dir = None
    is_valid = True
    # Le contexte est suppose etre valide
    assert len(searched_context) == len(RESULTS_REF_TYPES)
    for i in range(len(RESULTS_REF_TYPES)):
        assert searched_context[i] in RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[i]], (
            "value '"
            + searched_context[i]
            + "' should be in "
            + RESULTS_REF_TYPES[i]
            + " type values "
            + str(RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[i]])
        )
    # S'il n'y a pas de repertoire, on renvoie le repertoire de reference standard
    if len(candidate_reference_dirs) == 0:
        return RESULTS_REF
    # Erreur si repertoire invalides
    if is_valid:
        for candidate_dir in candidate_reference_dirs:
            # On doit commencer par le prefixe de base RESULTS_REF
            if candidate_dir.find(RESULTS_REF) != 0:
                is_valid = False
                add_error(candidate_dir, "must start with '" + RESULTS_REF + "'")
            # Les valeurs suivantes de la conjonction doivent etre valide
            if is_valid:
                and_values = candidate_dir.split(AND)
                # La premiere valeur est imposes
                if and_values[0] != RESULTS_REF:
                    is_valid = False
                    add_error(
                        candidate_dir,
                        "must start with '"
                        + RESULTS_REF
                        + "', with values separated by '-' or '_'",
                    )
                # Analyse des valeurs suivantes
                else:
                    current_type_index = -1
                    for i, and_value in enumerate(and_values[1:]):
                        # Les valeurs doivent etre non vide
                        if and_value == "":
                            is_valid = False
                            add_error(
                                candidate_dir,
                                "must not contain empty values separated by '"
                                + AND
                                + "'",
                            )
                        # Analyse des conjonctions de valeurs
                        else:
                            or_values = and_value.split(OR)
                            found_type = ""
                            found_type_index = -1
                            for value in or_values:
                                # Les valeurs doivent etre non vide
                                if value == "":
                                    is_valid = False
                                    add_error(
                                        candidate_dir,
                                        "must not contain empty values separated by '"
                                        + OR
                                        + "'",
                                    )
                                # Elles doivent correspond a un des types de contexte
                                elif found_type == "":
                                    for index, type in enumerate(RESULTS_REF_TYPES):
                                        if value in RESULTS_REF_TYPE_VALUES[type]:
                                            found_type = type
                                            found_type_index = index
                                            break
                                    if found_type == "":
                                        is_valid = False
                                        add_error(
                                            candidate_dir,
                                            "value (" + value + ") is not valid",
                                        )
                                # Elles doivent toute correspondre au meme type de contexte
                                if (
                                    is_valid
                                    and value not in RESULTS_REF_TYPE_VALUES[found_type]
                                ):
                                    is_valid = False
                                    add_error(
                                        candidate_dir,
                                        "values between '"
                                        + OR
                                        + "' ("
                                        + and_value
                                        + ") must correspond to the same type ("
                                        + found_type
                                        + ")",
                                    )
                                if not is_valid:
                                    break
                            # Les valeurs doivent etre triees
                            if is_valid and or_values != sorted(or_values):
                                is_valid = False
                                add_error(
                                    candidate_dir,
                                    "values between '"
                                    + OR
                                    + "' ("
                                    + and_value
                                    + ") must be sorted",
                                )
                            # Les valeurs ne doivent pas etre toute presentes simultanement
                            # Chaque ensemble de valeurs doit constituer une partition par type de contexte
                            # Dans le cas d'un nouveau type de contexte ne comportant qu'une valeur, il faudrait
                            # ajouter une valeur fictive 'other' pour constituer une partition
                            if is_valid and len(or_values) == len(
                                RESULTS_REF_TYPE_VALUES[found_type]
                            ):
                                is_valid = False
                                add_error(
                                    candidate_dir,
                                    "values ("
                                    + and_value
                                    + ") must not contain all "
                                    + found_type
                                    + " type values "
                                    + str(RESULTS_REF_TYPE_VALUES[found_type]),
                                )
                        # Le type de valeur doit etre d'index structement superieur a celui du type courant
                        if is_valid:
                            assert found_type_index >= 0
                            if (
                                current_type_index >= 0
                                and found_type_index <= current_type_index
                            ):
                                is_valid = False
                                # Message d'erreur selon l'inegalite stricte ou non
                                if found_type_index == current_type_index:
                                    add_error(
                                        candidate_dir,
                                        "values related to "
                                        + RESULTS_REF_TYPES[current_type_index]
                                        + " type should not be specified several times",
                                    )
                                else:
                                    add_error(
                                        candidate_dir,
                                        "values related to "
                                        + RESULTS_REF_TYPES[found_type_index]
                                        + " type should be specified before values related to "
                                        + RESULTS_REF_TYPES[current_type_index]
                                        + " type",
                                    )
                            current_type_index = found_type_index
                        if not is_valid:
                            break
    # Erreur si repertoires incoherents pour l'ensemble des contextes possibles
    used_candidate_dirs = {}
    if is_valid:
        assert results_ref_dir is None
        # Construction de tous les contexes possibles
        results_ref_type_number = len(RESULTS_REF_TYPES)
        all_evaluated_contexts = [[]]
        for index in range(results_ref_type_number):
            new_contexts = []
            context_type = RESULTS_REF_TYPES[index]
            for context in all_evaluated_contexts:
                for value in RESULTS_REF_TYPE_VALUES[context_type]:
                    new_context = context.copy()
                    new_context.append(value)
                    new_contexts.append(new_context)
            all_evaluated_contexts = new_contexts
        # Parcours de tous les contextes possibles pour verifier si l'ensemble des repertoire de reference est coherent
        for evaluated_context in all_evaluated_contexts:
            # Initialisation d'un liste comportant la liste des repertoire candidats par nombre de "match"
            # On va prioriser les "match" les plus specialises, apres avoire verifie qu'il n'y a pas d'ambiguite
            candidate_dirs_per_match_number = [None]
            for i in range(results_ref_type_number):
                candidate_dirs_per_match_number.append([])
            # Parcours des repertoires candidats pour verifier s'il n'y pas ambiguite
            for candidate_dir in candidate_reference_dirs:
                match_number = 0
                for value in evaluated_context:
                    if value in candidate_dir:
                        match_number += 1
                if match_number > 0:
                    candidate_dirs_per_match_number[match_number].append(candidate_dir)
            # Un seul repertoire doit etre selection pour le nombre maximal de match
            evaluated_context_results_ref_dir = ""
            match_number = results_ref_type_number
            while match_number > 0:
                selected_candidate_dir_number = len(
                    candidate_dirs_per_match_number[match_number]
                )
                if selected_candidate_dir_number >= 1:
                    if selected_candidate_dir_number == 1:
                        evaluated_context_results_ref_dir = (
                            candidate_dirs_per_match_number[match_number][0]
                        )
                        # On memorise que le repertoire a ete utilise au moins une fois
                        used_candidate_dirs[evaluated_context_results_ref_dir] = True
                    else:
                        is_valid = False
                        add_error(
                            "",
                            "context "
                            + str(evaluated_context)
                            + " matches "
                            + str(selected_candidate_dir_number)
                            + " variants of "
                            + RESULTS_REF
                            + " dirs (e.g. "
                            + str(candidate_dirs_per_match_number[match_number])
                            + ")",
                        )
                    break
                match_number -= 1
            # Si aucun repertoire specialise trouve, on se rabat sur le repertoire de base
            if is_valid and evaluated_context_results_ref_dir == "":
                if RESULTS_REF in candidate_reference_dirs:
                    evaluated_context_results_ref_dir = RESULTS_REF
                    # On memorise que le repertoire a ete utilise au moins une fois
                    used_candidate_dirs[evaluated_context_results_ref_dir] = True
                else:
                    is_valid = False
                    add_error(
                        "",
                        "no "
                        + RESULTS_REF
                        + " dir found for context "
                        + str(evaluated_context),
                    )
            # On memorise de facon opportuniste le resultat pour le contexte en entree de la methode
            if is_valid and evaluated_context == searched_context:
                results_ref_dir = evaluated_context_results_ref_dir
            if not is_valid:
                break
    # Erreur si certains repertoires ne sont jamais utilise par aucun contexte
    assert len(used_candidate_dirs) <= len(candidate_reference_dirs)
    if is_valid and len(used_candidate_dirs) < len(candidate_reference_dirs):
        unused_candidate_dirs = []
        for candidate_dir in candidate_reference_dirs:
            if candidate_dir not in used_candidate_dirs:
                unused_candidate_dirs.append(candidate_dir)
        is_valid = False
        add_error(
            "",
            "some "
            + RESULTS_REF
            + " dirs are not active for any context "
            + str(unused_candidate_dirs),
        )
    # Invalidation des resultats intermediaires en cas d'erreur
    if not is_valid:
        results_ref_dir = None
        # Message synthetique de fin
        add_error("", "invalid " + RESULTS_REF + " dir")
    return results_ref_dir


def test_results_ref_context_management():
    """Test de la gestion des contexte des repertoires de rfeference"""

    def buil_candidate_results_ref_dirs(
        list_type_values, concat_values=False, specific_dir=None
    ):
        """Construction d'une liste de repertoire de resultat de reference
        Parametres:
        - list_type_values est une liste (par type) de liste de valeurs
        - concat_values: concatenation des valeurs par contexte, plutot  que autant de contextes que de valeurs
        - specific_dir: repertoire specifique a ajouter
        """
        results_ref_dirs = []
        # Ajout du repertoire specifique
        if specific_dir is not None:
            results_ref_dirs.append(specific_dir)
        # Ajout d'un repertoire par valeur, en cumulant les types
        if not concat_values:
            base_names = [RESULTS_REF]
            for i in range(len(list_type_values)):
                new_base_names = []
                values = list_type_values[i]
                for base_name in base_names:
                    for value in values:
                        results_ref_dir = base_name + AND + value
                        results_ref_dirs.append(results_ref_dir)
                        new_base_names.append(results_ref_dir)
                base_names = new_base_names
        # Ajout d'un repertoire par liste de valeurs, en cumulant les types
        else:
            results_ref_dir = RESULTS_REF
            for i in range(len(list_type_values)):
                values = list_type_values[i]
                values_string = ""
                for value in values:
                    if values_string != "":
                        values_string += OR
                    values_string += value
                results_ref_dir += AND + values_string
                results_ref_dirs.append(results_ref_dir)
        return results_ref_dirs

    print("--- Test context management of reference results directories ---")

    # Test de gestion du contexte courant
    get_context_computing_type(show=True)
    get_context_platform_type(show=True)
    current_context = get_current_results_ref_context()
    print("Current reference context: " + str(current_context))

    # Jeux de test basique
    list_candidate_results_ref_dirs = []
    list_candidate_results_ref_dirs.append([])
    list_candidate_results_ref_dirs.append([RESULTS])
    list_candidate_results_ref_dirs.append([RESULTS_REF])
    list_candidate_results_ref_dirs.append([RESULTS_REF, RESULTS_REF + AND])
    list_candidate_results_ref_dirs.append([RESULTS_REF, RESULTS_REF + AND + OR])
    list_candidate_results_ref_dirs.append([RESULTS_REF, RESULTS_REF + "NO"])
    list_candidate_results_ref_dirs.append([RESULTS_REF, RESULTS_REF + AND + "NO"])
    # Jeu de test incomplet
    list_candidate_results_ref_dirs.append(
        [RESULTS_REF + AND + RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[0]][0]]
    )
    # Plusieurs valeurs de type different pour un meme type
    list_candidate_results_ref_dirs.append(
        [
            RESULTS_REF,
            RESULTS_REF
            + AND
            + RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[0]][0]
            + OR
            + RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[1]][0],
        ]
    )
    # Plusieurs valeurs d'un meme type dans le mauvais ordre
    list_candidate_results_ref_dirs.append(
        [
            RESULTS_REF,
            RESULTS_REF
            + AND
            + RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[1]][1]
            + OR
            + RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[1]][0],
        ]
    )
    # Spefication multiple des valeurs d'un meme type
    list_candidate_results_ref_dirs.append(
        [
            RESULTS_REF,
            RESULTS_REF
            + AND
            + RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[0]][0]
            + AND
            + RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[0]][1],
        ]
    )
    # Spefication des valeurs de different type dans le mauvais ordre des types
    list_candidate_results_ref_dirs.append(
        [
            RESULTS_REF,
            RESULTS_REF
            + AND
            + RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[1]][0]
            + AND
            + RESULTS_REF_TYPE_VALUES[RESULTS_REF_TYPES[0]][0],
        ]
    )
    # Tests systematiques avec tout ou partie des valeurs par type
    list_type_values = []
    for type in RESULTS_REF_TYPES:
        all_type_values = RESULTS_REF_TYPE_VALUES[type]
        specific_type_values = all_type_values[:-1]
        list_type_values.append(all_type_values)
        list_candidate_results_ref_dirs.append(
            buil_candidate_results_ref_dirs([all_type_values])
        )
        list_candidate_results_ref_dirs.append(
            buil_candidate_results_ref_dirs(
                [specific_type_values], specific_dir=RESULTS_REF
            )
        )
        list_candidate_results_ref_dirs.append(
            buil_candidate_results_ref_dirs([all_type_values], concat_values=True)
        )
        list_candidate_results_ref_dirs.append(
            buil_candidate_results_ref_dirs(
                [specific_type_values], concat_values=True, specific_dir=RESULTS_REF
            )
        )
    list_candidate_results_ref_dirs.append(
        buil_candidate_results_ref_dirs(list_type_values)
    )
    list_candidate_results_ref_dirs.append(
        buil_candidate_results_ref_dirs(list_type_values, specific_dir=RESULTS_REF)
    )

    # Test de recherche du repertoire de reference
    for i, candidate_results_ref_dirs in enumerate(list_candidate_results_ref_dirs):
        print(
            "Candidate reference dirs "
            + str(len(candidate_results_ref_dirs))
            + " [test "
            + str(i + 1)
            + "]:"
        )
        for candidate in candidate_results_ref_dirs:
            print("\t" + candidate)
        results_ref_dir = _search_results_ref_dir(
            candidate_results_ref_dirs, current_context, show=True
        )
        print("\t=> " + str(results_ref_dir))
