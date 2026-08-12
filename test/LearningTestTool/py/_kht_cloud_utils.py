# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import os
import stat
import subprocess
import shutil
import tempfile

import _kht_constants as kht
import _kht_results_management as results
import _kht_utils as utils
from kht_test import build_tool_exe_path

"""
Fonctions pour la cloudification et le nettoyage de l'arborescence LearningTest
"""


# Prefixes valides pour un URI de repertoire cloud
_CLOUD_URI_PREFIXES = ("gs://", "s3://", "https://")


# --- URI cloud : validation et normalisation ---
def check_cloud_dir(uri):
    """Retourne True si l'URI est un URI cloud valide (gs://, s3://, https://)"""
    return uri is not None and uri.startswith(_CLOUD_URI_PREFIXES)


def normalize_cloud_directory_uri(cloud_dir):
    """Supprime les '/' terminaux d'un URI cloud en preservant le separateur de schema (ex: gs://)"""
    normalized = cloud_dir
    while normalized.endswith("/") and not normalized.endswith("://"):
        normalized = normalized[:-1]
    return normalized


# --- Detection de cloudification dans une arborescence LearningTest ---
def is_cloudified_test_dir(test_dir):
    """Retourne True si le repertoire de test contient un scenario cloud (test-cloud.prm)"""
    return os.path.isfile(os.path.join(test_dir, kht.TEST_CLOUD_PRM))


def is_cloudified_home_dir(home_dir):
    """Retourne True si au moins un repertoire de test de l'arborescence est cloudifie"""
    for tool_dir_name in kht.TOOL_DIR_NAMES.values():
        tool_dir = os.path.join(home_dir, tool_dir_name)
        if not os.path.isdir(tool_dir):
            continue
        for suite_name in os.listdir(tool_dir):
            suite_dir = os.path.join(tool_dir, suite_name)
            if not os.path.isdir(suite_dir):
                continue
            for test_name in os.listdir(suite_dir):
                test_dir = os.path.join(suite_dir, test_name)
                if is_cloudified_test_dir(test_dir):
                    return True
    return False


# --- Helpers internes de transformation de contenu ---
def run_khiops_no_replay(khiops_params, step_label):
    """Execute khiops -O (no-replay) pour transformer le scenario; retourne le chemin du fichier .prm de sortie."""
    out_fd, out_path = tempfile.mkstemp(suffix=".prm", prefix="kht_no_replay_out_")
    os.close(out_fd)
    err_fd, err_file_path = tempfile.mkstemp(suffix=".txt", prefix="kht_no_replay_err_")
    os.close(err_fd)
    result = subprocess.run(
        khiops_params + ["-O", out_path, "-e", err_file_path],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    if result.stdout.strip():
        print("Warning: unexpected output during " + step_label + ":\n" + result.stdout)
    if result.stderr.strip():
        print(
            "Warning: unexpected error output during "
            + step_label
            + ":\n"
            + result.stderr
        )
    if os.path.isfile(err_file_path):
        with open(err_file_path, "r", errors="ignore") as err_f:
            err_content = err_f.read()
        if "error" in err_content.lower():
            print("Warning: errors in " + step_label + " error log:\n" + err_content)
        utils.remove_file(err_file_path)
    return out_path


def cloudify_file_content(content, cloud_dir, cloud_test_dir, is_json):
    """Remplace les chemins locaux par des URIs cloud dans le contenu d'un fichier.
    L'ordre est crucial : '../../../' avant './' car './' est une sous-chaine.
    """
    cloud_results_dir = cloud_test_dir + "/" + kht.RESULTS
    if is_json:
        # Dans les fichiers JSON les '/' sont echappes en '\/'
        def esc(s):
            return s.replace("/", "\\/")

        content = content.replace(esc("../../../"), esc(cloud_dir + "/"))
        content = content.replace(
            esc("./" + kht.RESULTS + "/"), esc(cloud_results_dir + "/")
        )
        content = content.replace(esc("./"), esc(cloud_test_dir + "/"))
    else:
        content = content.replace("../../../", cloud_dir + "/")
        content = content.replace("./" + kht.RESULTS + "/", cloud_results_dir + "/")
        content = content.replace("./", cloud_test_dir + "/")
    return content


def cloudify_prm_content(content, cloud_dir, cloud_test_dir):
    """Remplace les chemins locaux par des URIs cloud dans un scenario Khiops.
    L'ordre est crucial : '../../../' doit etre traite avant './' car './' est une sous-chaine.
    """
    content = content.replace("../../../", cloud_dir + "/")
    content = content.replace("./", cloud_test_dir + "/")
    return content


# --- API de cloudification pour l'export LearningTest (kht_export) ---
def generate_cloud_prm(
    source_test_dir, target_test_dir, cloud_dir, cloud_test_dir, tool_exe_path
):
    """Genere test-cloud.prm dans target_test_dir a partir de test.prm (et test.json si present).
    Etape 1 : expansion de test.json via khiops -j (si binaire fourni) ou heuristique Python.
    Etape 2 : remplacement des chemins locaux par les URIs cloud via cloudify_prm_content.
    Retourne True si le fichier a ete genere.
    """
    src_prm = os.path.join(source_test_dir, kht.TEST_PRM)
    if not os.path.isfile(src_prm):
        return False

    src_json = os.path.join(source_test_dir, kht.TEST_JSON)
    expanded_prm = None  # fichier temporaire a nettoyer si cree

    if os.path.isfile(src_json):
        # Expansion via khiops -j : khiops connait la semantique exacte des parametres JSON
        expanded_prm = run_khiops_no_replay(
            [tool_exe_path, "-b", "-i", src_prm, "-j", src_json],
            "JSON scenario expansion",
        )
        with open(expanded_prm, "r", errors="ignore") as f:
            content = f.read()
    else:
        with open(src_prm, "r", errors="ignore") as f:
            content = f.read()

    content = cloudify_prm_content(content, cloud_dir, cloud_test_dir)

    target_cloud_prm = os.path.join(target_test_dir, kht.TEST_CLOUD_PRM)
    with open(target_cloud_prm, "w", errors="ignore") as f:
        f.write(content)

    if expanded_prm is not None and os.path.isfile(expanded_prm):
        utils.remove_file(expanded_prm)
    return True


def cloudify_results_ref_dirs(target_test_dir, cloud_dir, cloud_test_dir):
    """Recrit les chemins locaux en URIs cloud dans tous les fichiers de tous les results.ref*."""
    for name in os.listdir(target_test_dir):
        ref_dir = os.path.join(target_test_dir, name)
        if not (os.path.isdir(ref_dir) and results.is_candidate_results_ref_dir(name)):
            continue
        for file_name in os.listdir(ref_dir):
            file_path = os.path.join(ref_dir, file_name)
            if not os.path.isfile(file_path):
                continue
            with open(file_path, "r", errors="ignore") as f:
                content = f.read()
            is_json = file_name.endswith(".khj")
            new_content = cloudify_file_content(
                content, cloud_dir, cloud_test_dir, is_json
            )
            if new_content != content:
                os.chmod(file_path, stat.S_IWRITE | stat.S_IREAD)
                with open(file_path, "w", errors="ignore") as f:
                    f.write(new_content)


def cloudify_learning_test_tree(home_dir, cloud_dir, tool_exe_path):
    """Ajoute les scenarios cloud et reecrit les references dans une arborescence deja exportee."""
    for tool_dir_name in kht.TOOL_DIR_NAMES.values():
        tool_dir = os.path.join(home_dir, tool_dir_name)
        if not os.path.isdir(tool_dir):
            continue
        for suite_dir_name in os.listdir(tool_dir):
            suite_dir = os.path.join(tool_dir, suite_dir_name)
            if not os.path.isdir(suite_dir):
                continue
            for test_dir_name in os.listdir(suite_dir):
                test_dir = os.path.join(suite_dir, test_dir_name)
                if not os.path.isdir(test_dir):
                    continue
                test_prm_path = os.path.join(test_dir, kht.TEST_PRM)
                if not os.path.isfile(test_prm_path):
                    continue
                cloud_test_dir = (
                    cloud_dir
                    + "/"
                    + tool_dir_name
                    + "/"
                    + suite_dir_name
                    + "/"
                    + test_dir_name
                )
                generate_cloud_prm(
                    test_dir,
                    test_dir,
                    cloud_dir,
                    cloud_test_dir,
                    tool_exe_path=tool_exe_path,
                )
                cloudify_results_ref_dirs(test_dir, cloud_dir, cloud_test_dir)


def resolve_cloudify_options(cloud_directory, home_dir):
    """
    Guard contre la double-cloudification d'une arborescence deja cloudifiee
    Renvoie un couple (cloudify, normalized_cloud_directory) ou cloudify est un booleen indiquant si on doit cloudifier
     et normalized_cloud_directory est le chemin normalise du repertoire cloudifie cible si cloudify est vrai, None sinon
    """
    cloudify = cloud_directory is not None
    if cloudify:
        normalized_cloud_directory = normalize_cloud_directory_uri(cloud_directory)
        if is_cloudified_home_dir(home_dir):
            utils.fatal_error(
                "source directory "
                + home_dir
                + " is already a cloudified LearningTest; cannot cloudify again"
            )
    return cloudify, normalized_cloud_directory if cloudify else None


# --- Integration CLI: option --cloud-directory et --binaries ---
def argument_parser_add_cloud_directory_arguments(parser):
    """Ajout des arguments --cloud-directory et --binaries"""
    parser.add_argument(
        "--cloud-directory",
        help="cloud URI of the exported LearningTest root, triggers cloudification"
        " (e.g., gs://bucket/LearningTest, s3://bucket/LearningTest)",
        type=str,
        metavar="uri",
        action="store",
    )

    # Binaire Khiops pour l'expansion JSON lors de la cloudification
    parser.add_argument(
        "--binaries",
        dest="binaries",
        help="Khiops binary directory or alias, used with --cloud-directory"
        " to expand test.json parameters via khiops -j",
        type=str,
        metavar="path",
        action="store",
    )


def argument_parser_check_cloud_directory_arguments(parser, cloud_directory, binaries):
    """Verification des arguments --cloud-directory et --binaries"""
    if cloud_directory is not None and not check_cloud_dir(cloud_directory):
        parser.error(
            "argument --cloud-directory: '"
            + cloud_directory
            + "' must be a cloud URI starting with "
            + ", ".join(_CLOUD_URI_PREFIXES)
        )

    # --binaries est obligatoire avec --cloud-directory (expansion JSON via khiops -j)
    tool_exe_path = None
    if cloud_directory is not None and binaries is None:
        parser.error("argument --binaries is required with --cloud-directory")
    if binaries is not None:
        if cloud_directory is None:
            parser.error("argument --binaries requires --cloud-directory")
        tool_exe_path, error_message = build_tool_exe_path(
            binaries, kht.KHIOPS, use_khiops_env=False
        )
        if tool_exe_path is None:
            parser.error("argument --binaries: " + error_message)


# --- Nettoyage sur le cloud avant de l'execution des tests (kht_test cloudification) : extraction et nettoyage du repertoire results ---
def get_cloud_results_dir(test_dir):
    """Retourne l'URI cloud du repertoire results cible par test-cloud.prm."""
    cloud_prm_path = os.path.join(test_dir, kht.TEST_CLOUD_PRM)
    if not os.path.isfile(cloud_prm_path):
        return None

    with open(cloud_prm_path, "r", errors="ignore") as cloud_prm_file:
        content = cloud_prm_file.read()

    cloud_results_dirs = []
    separators = " \t\r\n'\""
    for uri_prefix in ["gs://", "s3://", "https://"]:
        start = 0
        while True:
            start = content.find(uri_prefix, start)
            if start < 0:
                break
            end = start + len(uri_prefix)
            while end < len(content) and content[end] not in separators:
                end += 1
            uri = content[start:end].rstrip(",;")
            results_pattern = "/" + kht.RESULTS + "/"
            results_pos = uri.find(results_pattern)
            if results_pos >= 0:
                cloud_results_dir = uri[: results_pos + len(results_pattern) - 1]
                if cloud_results_dir not in cloud_results_dirs:
                    cloud_results_dirs.append(cloud_results_dir)
            start = end

    if len(cloud_results_dirs) == 0:
        return None
    if len(cloud_results_dirs) > 1:
        utils.fatal_error(
            "multiple cloud results dirs found in "
            + cloud_prm_path
            + ": "
            + utils.list_to_label(cloud_results_dirs)
        )
    return cloud_results_dirs[0]


def clean_cloud_results_dir(test_dir):
    """Nettoie le repertoire results cible sur le cloud avant l'execution du test."""
    cloud_results_dir = get_cloud_results_dir(test_dir)
    if cloud_results_dir is None:
        return

    if cloud_results_dir.startswith("gs://"):
        command = ["gcloud", "storage", "rm", "--recursive", cloud_results_dir]
    elif cloud_results_dir.startswith("s3://"):
        command = ["aws", "s3", "rm", cloud_results_dir, "--recursive"]
    elif cloud_results_dir.startswith("https://"):
        command = ["azcopy", "rm", cloud_results_dir, "--recursive=true"]
    else:
        utils.fatal_error("unsupported cloud results dir " + cloud_results_dir)

    if shutil.which(command[0]) is None:
        utils.fatal_error(
            "cloud cleanup command not found for "
            + cloud_results_dir
            + ": "
            + command[0]
        )

    result = subprocess.run(
        command,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    if result.returncode != 0:
        utils.write_message(message="Cloud cleanup command failed:\n" + result.stderr)
