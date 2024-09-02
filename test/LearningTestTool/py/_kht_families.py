# Copyright (c) 2024 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import os

import _kht_constants as kht
import _kht_utils as utils

"""
Definition des familles de test
Chaque famille de test est une liste de suite de test par outil
"""


def check_family(family):
    """Test d'existence d'une famille, avec sortie en erreur fatale si non trouvee"""
    if family not in TEST_FAMILIES:
        utils.fatal_error(
            "Family "
            + family
            + " should be in available families "
            + utils.list_to_label(TEST_FAMILIES)
        )


# Liste des familles, de la plus simple a la plus complete
BASIC = "basic"  # Famille elementaire, tres rapide a tester
FULL = "full"  # Famille correspondant a tous les tests de non regression (environ une heure a tester)
FULL_NO_KNI = "full-no-kni"  # Idem, sans KNI
COMPLETE = "complete"  # Famille complete, tres lourde a tester (environ une journee)
ALL = "all"  # Tous les repertoire de suite exhaustivement: ne pas utiliser pour les test, mais pour leur gestion
TEST_FAMILIES = [BASIC, FULL, FULL_NO_KNI, COMPLETE, ALL]
assert len(set(TEST_FAMILIES)) == len(TEST_FAMILIES), (
    "Families " + str(TEST_FAMILIES) + " must not contain duplicates"
)

# Fammile par defaut
DEFAULT_TEST_FAMILY = FULL
assert DEFAULT_TEST_FAMILY in TEST_FAMILIES

# Cas de la version V11, qui inclus des suites de test additionnelles
KHIOPS_V11 = True

# Ensuite des suites de test par famille et par outils
# Pour le cas particlier le la famille ALL, les suite ne sont pas specifiee:
# il fautparcours les sous repertoire des tool dir exhaustivement
FAMILY_TEST_SUITES = {}

# Famille basique
FAMILY_TEST_SUITES[BASIC, kht.KHIOPS] = ["Standard"]
FAMILY_TEST_SUITES[BASIC, kht.COCLUSTERING] = ["Standard"]
FAMILY_TEST_SUITES[BASIC, kht.KNI] = ["Standard"]

# Famille full
FAMILY_TEST_SUITES[FULL, kht.KHIOPS] = [
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
FAMILY_TEST_SUITES[FULL, kht.COCLUSTERING] = [
    "Standard",
    "Bugs",
    "NewPriorV9",
    "SmallInstability",
]
FAMILY_TEST_SUITES[FULL, kht.KNI] = ["Standard", "MultiTables", "SmallInstability"]

# Nouvelle suites specifique a la version 11
if KHIOPS_V11:
    FAMILY_TEST_SUITES[FULL, kht.KHIOPS] = FAMILY_TEST_SUITES[FULL, kht.KHIOPS] + [
        "KIInterpretation",
        "Histograms",
        "HistogramsLimits",
        "TextVariables",
    ]

# Famille full sans KNI
FAMILY_TEST_SUITES[FULL_NO_KNI, kht.KHIOPS] = FAMILY_TEST_SUITES[
    FULL, kht.KHIOPS
].copy()
FAMILY_TEST_SUITES[FULL_NO_KNI, kht.COCLUSTERING] = FAMILY_TEST_SUITES[
    FULL, kht.COCLUSTERING
].copy()
FAMILY_TEST_SUITES[FULL_NO_KNI, kht.KNI] = []

# Famille Complete
FAMILY_TEST_SUITES[COMPLETE, kht.KHIOPS] = FAMILY_TEST_SUITES[
    FULL, kht.KHIOPS
].copy() + [
    "Classification",
    "MTClassification",
    "Regression",
    "ChallengeAutoML",
]
FAMILY_TEST_SUITES[COMPLETE, kht.COCLUSTERING] = FAMILY_TEST_SUITES[
    FULL, kht.COCLUSTERING
].copy()
FAMILY_TEST_SUITES[COMPLETE, kht.KNI] = FAMILY_TEST_SUITES[FULL, kht.KNI].copy()
if KHIOPS_V11:
    FAMILY_TEST_SUITES[COMPLETE, kht.KHIOPS] = FAMILY_TEST_SUITES[
        COMPLETE, kht.KHIOPS
    ] + ["TextClassification"]
