import learning_test_env
import test_khiops
import test_families
import os.path
import sys
import stat
from test_dir_management import *


# lance les tests de khiops sur tous les repertoires contenus dans la liste "tool_test_dirs"
def test_khiops_tool(tool_name, tool_version, tool_test_dirs):
    """Run tool on test dirs"""
    # Build tool exe path name from version
    tool_exe_name, tool_test_sub_dir = test_khiops.retrieve_tool_info(tool_name)
    tool_exe_path = test_khiops.build_tool_exe_path(tool_exe_name, tool_version)
    # Clean results
    for test in tool_test_dirs:
        tool_samples_path = os.path.join(
            learning_test_env.learning_test_root,
            "LearningTest",
            tool_test_sub_dir,
            test,
        )
        if os.path.isdir(tool_samples_path):
            for sub_test in os.listdir(tool_samples_path):
                sub_test_path = os.path.join(tool_samples_path, sub_test)
                file_path = os.path.join(sub_test_path, COMPARISON_RESULTS_LOG)
                if os.path.isfile(file_path):
                    os.chmod(file_path, stat.S_IWRITE)
                    os.remove(file_path)
                result_dir = os.path.join(sub_test_path, "results")
                if os.path.isdir(result_dir):
                    for file_name in os.listdir(result_dir):
                        file_path = os.path.join(result_dir, file_name)
                        os.chmod(file_path, stat.S_IWRITE)
                        os.remove(file_path)
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
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        print("testAll [version] <tool>")
        print("  run all tests for all Khiops tools")
        print("\tversion: version of the tool")
        print("\t  d: debug version in developpement environnement")
        print("\t  r: release version in developpement environnement")
        print("\t  ver: <toolname>.<ver>.exe in directory LearningTest\\cmd\\modl")
        print("\t  nul: for comparison with the test results only")
        print("\t  full exe path, if <tool> parameter is used")
        print("\ttool: all tools if not specified, one specified tool otherwise")
        print("\t  Khiops")
        print("\t  Coclustering")
        print("\t  KNI")
        exit(0)

    # Info on complete tests
    if os.getenv("KhiopsCompleteTests") != "true":
        print("\n--------------------------------------------------------")
        print("Set env var KhiopsCompleteTests=true")
        print("\tto run all long, instable and unusefull tests")
        print("--------------------------------------------------------\n")
    print("\n\n--------------------------------------------------------")

    sys.stdout = test_khiops.Unbuffered(sys.stdout)

    # Passage en mode batch
    os.environ["KhiopsBatchMode"] = "true"

    # Retrieve version
    version = sys.argv[1]
    assert version is not None

    # Retrieve tool
    tool = ""
    if len(sys.argv) == 3:
        tool = sys.argv[2]

    # Khiops tool
    if tool == "" or tool == "Khiops":
        khiops_tests = test_families.get_test_family("Khiops")
        test_khiops_tool("Khiops", version, khiops_tests)

    # Coclustering tool
    if tool == "" or tool == "Coclustering":
        coclustering_tests = test_families.get_test_family("Coclustering")
        test_khiops_tool("Coclustering", version, coclustering_tests)

    # KNI tool
    if tool == "" or tool == "KNI":
        KNI_tests = test_families.get_test_family("KNI")
        test_khiops_tool("KNI", version, KNI_tests)

    print("all tests are done")
