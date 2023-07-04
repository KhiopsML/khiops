import os

"""
Specification of test environment.
A config file (see below) allow to personalize the test environment.

The launch of a test from LearningTest can used an executable with its full path as as parameter.
This is the standard way, used for example under linux platforms.
For a Khiops developer, it may convenient to use directly the executable obtained from compilation,
either with is release or debug version.
The choice of debug versus release is made using a parameter (r or d) instead of the full path of the executable.
To benefit from these "short cut" parameters, the config file must be correctly specified.

"""

""" name of config file """
learning_test_config_file_name = "learning_test.config"

""" List of keys in config file """
learning_test_config_keys = {
    "path": "additional path (eg: to access to java runtime)",
    "classpath": "additional classpath for java libraries",
    "learningtest_root": "alternative root dir to use where LearningTest is located",
    "learning_release_dir": "dir where the release developement binaries are located (to enable the 'r' alias')",
    "learning_debug_dir": "dir where the debug developement binaries are located (to enable the 'd' alias')",
}


def load_learning_test_config():
    """Load config file, check it
    Return config in case of dictionary, and quit the program otherwise"""
    ok = True
    config_dic = {}
    # Get the full path to the directory a Python file is contained in
    containing_dir_path = os.path.dirname(os.path.realpath(__file__))
    # Get the file name of the config file
    config_file_path = os.path.join(containing_dir_path, learning_test_config_file_name)
    # If file does not exist, use empty values for each key
    if not os.path.isfile(config_file_path):
        for key in learning_test_config_keys:
            config_dic[key] = ""
        return config_dic
    # Read file
    if ok:
        try:
            with open(config_file_path, "r") as config_file:
                lines = config_file.readlines()
        except Exception as e:
            print(
                "Error in config file "
                + learning_test_config_file_name
                + ": read error ("
                + str(e)
                + ")"
            )
            ok = False
    # Analyse key value pairs
    if ok:
        for n, line in enumerate(lines):
            line = line[:-1]
            fields = line.split("=")
            # Test syntax
            if len(fields) != 2:
                print(
                    "error in config file "
                    + learning_test_config_file_name
                    + " line "
                    + str(n + 1)
                    + ": bad field number"
                )
                ok = False
                break
            # Test validity of key
            if not fields[0] in learning_test_config_keys:
                print(
                    "error in config file "
                    + learning_test_config_file_name
                    + " line "
                    + str(n + 1)
                    + ": unknown key <"
                    + fields[0]
                    + ">"
                )
                ok = False
                break
            else:
                # Test unicity of key
                if config_dic.get(fields[0]) is not None:
                    print(
                        "error in config file "
                        + learning_test_config_file_name
                        + " line "
                        + str(n + 1)
                        + ": key <"
                        + fields[0]
                        + "> not unique"
                    )
                    ok = False
                    break
                else:
                    config_dic[fields[0]] = fields[1]
    # Test that all keys are present
    if ok:
        if len(learning_test_config_keys) != len(config_dic):
            missing_keys = ""
            for key in learning_test_config_keys:
                if not key in config_dic:
                    if missing_keys != "":
                        missing_keys += ", "
                    missing_keys += key
            print(
                "error in config file "
                + learning_test_config_file_name
                + ": missing keys ("
                + missing_keys
                + ")"
            )
            ok = False
    # Return if ok
    if ok:
        return config_dic
    else:
        # Print help message
        print("")
        print(
            "The config file "
            + learning_test_config_file_name
            + " must be in directory "
            + containing_dir_path
        )
        print(
            "It contains the following key=value pairs that allows a personnalisation of the environment:"
        )
        for key in learning_test_config_keys:
            print("\t" + key + ": " + learning_test_config_keys[key])
        quit()


""" Global config dictionary """
learning_test_config = load_learning_test_config()


def search_learning_test_root():
    """Extract root directory of LearningTest"""
    # Case where an alternative toot directory is specified in the config file
    test_root = learning_test_config["learningtest_root"]
    if test_root != "":
        # Test if valid directory
        if not os.path.isdir(test_root):
            print(
                "error in config file "
                + learning_test_config_file_name
                + ": key learningtest_root contains value <"
                + test_root
                + "> that is not a valid directory"
            )
            quit()
        if not os.path.isdir(os.path.join(test_root, "LearningTest")):
            print(
                "error in config file "
                + learning_test_config_file_name
                + ": key learningtest_root ("
                + test_root
                + ") should contain LearningTest dir"
            )
            quit()

    # Case where an alternative toot directory is specified in the config file
    else:
        # Get the full path to the directory a Python file is contained in
        containing_dir_path = os.path.dirname(os.path.realpath(__file__))
        assert "learningtest" in containing_dir_path.lower(), (
            "LearningTest dir not found in path " + containing_dir_path
        )
        # Extract start of the path before "LearningTest"
        path = containing_dir_path
        while "learningtest" in path.lower():
            path = os.path.dirname(path)
        test_root = path
    return test_root


# Specification of path environment variable
path_env = learning_test_config["path"]
if path_env != "":
    if os.environ.get("path") is None:
        os.environ["path"] = path_env
    else:
        os.environ["path"] = path_env + ";" + os.environ["path"]

# Specification of classpath environment variable
class_path_env = learning_test_config["classpath"]
if class_path_env != "":
    if os.environ.get("CLASSPATH") is None:
        os.environ["CLASSPATH"] = class_path_env
    else:
        os.environ["CLASSPATH"] = class_path_env + ";" + os.environ["CLASSPATH"]

# Root dir for LearningTest
learning_test_root = search_learning_test_root()


def build_dev_tool_exe_path(exe_name, version):
    """Build path of exe in developpement environmement for a given version (d: debug our r: release)"""
    assert version in ["d", "r"], version + " must be d or r"
    if version == "r":
        config_key = "learning_release_dir"
    else:
        config_key = "learning_debug_dir"
    learning_dev_dir = learning_test_config[config_key]
    # Check directory
    if learning_dev_dir == "":
        print(
            "error in config file "
            + learning_test_config_file_name
            + ": key "
            + config_key
            + " must be specified to use '"
            + version
            + "' alias"
        )
        quit()
    elif not os.path.isdir(learning_dev_dir):
        print(
            "error in config file "
            + learning_test_config_file_name
            + ": key "
            + config_key
            + " ("
            + learning_dev_dir
            + ") should be a valid directory"
        )
        quit()
    # Build tool path
    tool_exe_path = os.path.join(learning_dev_dir, exe_name)
    if os.name == "nt":
        tool_exe_path += ".exe"
    if not os.path.isfile(tool_exe_path):
        print("error: excutable (" + tool_exe_path + ") should be a valid file")
        quit()
    return tool_exe_path
