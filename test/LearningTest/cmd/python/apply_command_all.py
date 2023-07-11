import os.path
import sys
import learning_test_env
import apply_command
import test_khiops


if __name__ == "__main__":
    all_commands, standard_command_number = apply_command.register_all_commands()

    include_unofficial_sub_dirs = False
    ok = len(sys.argv) == 2
    if not ok:
        ok = len(sys.argv) == 3 and sys.argv[2] == "*"
        include_unofficial_sub_dirs = ok
    if not ok:
        print("applyCommandAll [command] <*>")
        print("  apply command on all test sub-directories")
        print("\tcommand: name of the command")
        print("\t*: to include 'unofficial' sub-directories, such as z_work")
        print("  Type applyCommand to see available commands")
        exit(0)

    # Acces to command
    command = sys.argv[1]

    # Browse main directories to fin test directories per tool
    for khiops_tool_name in test_khiops.khiops_tool_names:
        exe_name, test_sub_dir = test_khiops.retrieve_tool_info(khiops_tool_name)
        tool_test_path = os.path.join(
            learning_test_env.learning_test_root, "LearningTest", test_sub_dir
        )
        for dir_name in os.listdir(tool_test_path):
            root_path = os.path.join(tool_test_path, dir_name)
            if os.path.isdir(root_path):
                # Skip unofficial test directories
                if include_unofficial_sub_dirs or dir_name.find("_") != 1:
                    apply_command.execute_command(all_commands, command, root_path)
