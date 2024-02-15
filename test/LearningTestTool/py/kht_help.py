import _kht_constants as kht


def main():
    """Aide sur l'ensemble des commandes utilisateurs de LearningTest"""
    print("Available " + kht.LEARNING_TEST + " commands are")
    print(
        "- kht_test (tool binaries dir) (suite|test dir)"
        "\n    Test tool on a specific test directory"
    )
    print(
        "- kht_test_all (tool binaries dir) (home|tool dir) [family (default: full)]"
        "\n    Test all tools on a all test directories"
    )
    print(
        "- kht_apply (instruction) (family|test dir)"
        "\n    Apply and instruction (ex: errors) on a specific test directory"
    )
    print(
        "- kht_apply_all (instruction) (home|tool dir) [family (default: full)]"
        "\n    Apply an instruction (ex: errors) on a family"
    )
    print(
        "- kht_collect_results (home dir) (target root dir) (collect option) [family (default: full)]"
        "\n    Collect test results"
    )
    print(
        "- kht_export (LearningTest home dir) (target root dir) [family (default: full)]"
        "\n    Export sub-part of LearningTest tree"
    )
    print(
        "- kht_save (home dir) (target root dir) (save option)"
        "\n    Save " + kht.LEARNING_TEST + " directory tree"
    )
    print("- kht_env" "\n    Show the status of all en vars that impact the tests")
    print("- kht_help" "\n    Show this help for all kht commands")
    print("Type the name of a specific command for detailed help")


if __name__ == "__main__":
    main()
