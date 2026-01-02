# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import os
import argparse

import _kht_constants as kht
import _kht_utils as utils


def print_env_var_help(env_var, help_text):
    """Affichage de l'aide sur une variable d'environnement"""
    print(env_var + ": " + str(os.getenv(env_var)) + "\n\t" + help_text)


def get_env_var_help_label(env_var, help_text):
    """Renvoie un libelle d'aide sur une variable d'environnement"""
    return env_var + ": " + str(os.getenv(env_var)) + ", " + help_text


def help_env_vars():
    # Aide sur les variables d'environnement influant le comportement des outils Khiops
    print("")
    print_env_var_help(
        kht.KHIOPS_PREPARATION_TRACE_MODE,
        "trace for dimensionnining of preparation tasks (default: false)",
    )
    print_env_var_help(kht.KHIOPS_PARALLEL_TRACE, "trace for parallel tasks (0 to 3)")
    print_env_var_help(
        kht.KHIOPS_FILE_SERVER_ACTIVATED, "activate MPI file server (false,true)"
    )

    # Aide particulier sur le pilotage des traces memoire
    print(
        "Analysis of memory stats"
        + "\n\t"
        + get_env_var_help_label(
            kht.KHIOPS_MEM_STATS_LOG_FILE_NAME, "memory stats log file name"
        )
        + "\n\t"
        + get_env_var_help_label(
            kht.KHIOPS_MEM_STATS_LOG_FREQUENCY,
            "frequency of allocator stats collection (0, 100000, 1000000,...)",
        )
        + "\n\t"
        + get_env_var_help_label(
            kht.KHIOPS_MEM_STATS_LOG_TO_COLLECT,
            "stats to collect (8193: only time and labels, 16383: all,...)",
        )
        + "\n\t"
        + get_env_var_help_label(
            kht.KHIOPS_IO_TRACE_MODE, "to collect IO trace (false, true)"
        )
    )


def main():
    """Fonction principale d'affichage de l'aide sur les variables d'environnement"""
    # Parametrage de l'analyse de la ligne de commande
    script_name = os.path.basename(__file__)
    base_script_name = os.path.splitext(script_name)[0]
    parser = argparse.ArgumentParser(
        prog=base_script_name,
        description="show the status of the main environment variables used by the tool binaries",
    )
    # Analyse de la ligne de commande et execution
    parser.parse_args()
    help_env_vars()


if __name__ == "__main__":
    utils.set_flushed_outputs()
    main()
