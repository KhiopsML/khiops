import os

import _learning_test_constants as lt
import _learning_test_utils as utils


def get_family_suite_names(tool_name):
    """Retourne la liste des suites de test par outil pour une famille donnee
    Par defaut: la famille des test de non regression
    Si la variable d'environnement 'KhiopsCompleteTests'  est specifiee: la famille complete de tous les test
    """
    # Khiops tool
    family_suite_names = None
    if tool_name == lt.KHIOPS:
        family_suite_names = [
            "Standard",
            "SideEffects",
            "Rules",
            "MissingValues",
            "Advanced",
            "Bugs",
            "BugsMultiTables",
            "MultipleTargets",
            "MultiTables",
            "DeployCoclustering",
            "SparseData",
            "SparseModeling",
            "ParallelTask",
            "NewPriorV9",
            "DTClassification",
            "VariableConstruction",
            "NewV10",
            "CrashTests",
            "SmallInstability",
            "CharacterEncoding",
        ]
        # V11        "KIInterpretation",
        # V11        "Histograms",
        # V11        "HistogramsLimits",
        # V11        "TextVariables",
        # Following tests are very long, unstable and not useful:
        if utils.get_env_var_boolean_value(lt.KHIOPS_COMPLETE_TESTS, False):
            family_suite_names.append("Classification")
            family_suite_names.append("MTClassification")
            family_suite_names.append("Regression")
            family_suite_names.append("ChallengeAutoML")
            # V11 test_suites.append("TextClassification")

    # Coclustering tool
    if tool_name == lt.COCLUSTERING:
        family_suite_names = ["Standard", "Bugs", "NewPriorV9", "SmallInstability"]

    # KNI tool
    if tool_name == lt.KNI:
        family_suite_names = ["Standard", "MultiTables", "SmallInstability"]
    assert family_suite_names is not None
    return family_suite_names
