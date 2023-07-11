import learning_test_env
import test_khiops
import os.path
import sys

import stat


# lance les tests de khiops sur tous les repertoires contenus dans la liste "tests"


def remove_old_tests_resuls():
    """Efface tous les resultats de TOUS les tests (meme ceux qui ne sont pas specifier dans la variable tests)"""
    all_tools_test_root = os.path.join(
        learning_test_env.learning_test_root, "LearningTest"
    )
    for tool_test_dir in os.listdir(all_tools_test_root):
        tools_test_root = os.path.join(all_tools_test_root, tool_test_dir)
        if os.path.isdir(tools_test_root) and tool_test_dir.find("Test") == 0:
            for test in os.listdir(tools_test_root):
                test_path = os.path.join(tools_test_root, test)
                if os.path.isdir(test_path):
                    for sub_test in os.listdir(test_path):
                        sub_test_path = os.path.join(test_path, sub_test)
                        if os.path.isfile(
                            os.path.join(sub_test_path, "comparisonResults.log")
                        ):
                            print(sub_test_path)
                            # os.remove(os.path.join(sub_test_path, 'comparisonResults.log'))
                        result_dir = os.path.join(sub_test_path, "results")
                        if os.path.isdir(result_dir) and False:
                            for file_name in os.listdir(result_dir):
                                file_path = os.path.join(result_dir, file_name)
                                os.chmod(file_path, stat.S_IWRITE)
                                os.remove(file_path)


def test_khiops_tool(tool_name, tool_version, tool_test_dirs):
    """Run tool on test dirs"""
    # Build tool exe path name from version
    tool_exe_name, tool_test_sub_dir = test_khiops.retrieve_tool_info(tool_name)
    tool_exe_path = test_khiops.build_tool_exe_path(tool_exe_name, tool_version)
    # Run tests
    for test in tool_test_dirs:
        print("\n\n--------------------------------------------------------")
        print("\tRunning " + tool_name + " " + test + " tests")
        print("--------------------------------------------------------")
        tool_samples_path = os.path.join(
            learning_test_env.learning_test_root,
            "LearningTest",
            tool_test_sub_dir,
            test,
        )
        test_khiops.run_test(tool_exe_path, tool_samples_path, None)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("testAll [version]")
        print("  run all tests for all Khiops tools")
        print("\tversion: version of the tool")
        print("\t  d: debug version in developpement environnement")
        print("\t  r: release version in developpement environnement")
        print("\t  ver: <toolname>.<ver>.exe in directory LearningTest\\cmd\\modl")
        print("\t  nul: for comparison with the test results only")
        exit(0)

    # Info on complete tests
    if os.getenv("KhiopsCompleteTests") != "true":
        print("\n--------------------------------------------------------")
        print("Set env var KhiopsCompleteTests=true")
        print("\tto run all long, instable and unusefull tests")
        print("--------------------------------------------------------\n")
    print("\n\n--------------------------------------------------------")

    sys.stdout = test_khiops.Unbuffered(sys.stdout)

    # Remove old results (not activated)
    #  remove_old_tests_resuls()

    # Passage en mode batch
    os.environ["KhiopsBatchMode"] = "true"

    # Retrieve version
    version = sys.argv[1]
    assert version is not None

    # Khiops tool
    khiops_tests = [
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
        "Histograms",
        "HistogramsLimits",
        "TextVariables",
        "SparseData",
        "SparseModeling",
        "ParallelTask",
        "NewPriorV9",
        "DTClassification",
        "VariableConstruction",
        "NewV10",
        "KIInterpretation",
        "CrashTests",
        "SmallInstability",
    ]
    # Following tests are very long, instable and not usefull:
    if os.getenv("KhiopsCompleteTests") == "true":
        khiops_tests.append("Classification")
        khiops_tests.append("TextClassification")
        khiops_tests.append("MTClassification")
        khiops_tests.append("Regression")
        khiops_tests.append("ChallengeAutoML")
    test_khiops_tool("Khiops", version, khiops_tests)

    # Coclustering tool
    coclustering_tests = ["Standard", "Bugs", "NewPriorV9", "SmallInstability"]
    test_khiops_tool("Coclustering", version, coclustering_tests)

    # KNI tool
    KNI_tests = ["Standard", "MultiTables", "SmallInstability"]
    test_khiops_tool("KNI", version, KNI_tests)

    print("all tests are done")
