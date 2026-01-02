# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import os
import argparse

import _kht_constants as kht
import _kht_utils as utils


def help_commands():
    """Aide sur l'ensemble des commandes utilisateurs de LearningTest"""
    print("Available " + kht.LEARNING_TEST + " commands are")
    print(
        "- kht_test source binaries [options]"
        "\n    test a tool on a subset of test dirs"
    )
    print(
        "- kht_apply source instruction [options]"
        "\n    apply instruction (ex: errors) on a subset of test dirs"
    )
    print(
        "- kht_export source dest [options]"
        "\n    export a subset of a source "
        + kht.LEARNING_TEST
        + " tree to a target dir"
    )
    print(
        "- kht_collect_results source dest [options]"
        "\n    collect a subset of a source "
        + kht.LEARNING_TEST
        + " test dirs in a target dir"
    )
    print(
        "- kht_env"
        "\n    show the status of the main environment variables used by the tool binaries"
    )
    print("Type the name of a specific command with '-h' for detailed help")


def main():
    """Fonction principale de d'affichage de l'aide sur les commandes de LearningTest"""
    # Parametrage de l'analyse de la ligne de commande
    script_name = os.path.basename(__file__)
    base_script_name = os.path.splitext(script_name)[0]
    parser = argparse.ArgumentParser(
        prog=base_script_name,
        description="show help for all " + kht.LEARNING_TEST + " commands",
    )
    # Analyse de la ligne de commande et execution
    parser.parse_args()
    help_commands()


if __name__ == "__main__":
    utils.set_flushed_outputs()
    main()
