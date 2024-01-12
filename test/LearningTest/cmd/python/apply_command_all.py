import os.path
import sys
import learning_test_env
import apply_command
import test_khiops
import test_families

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
        # Get standard families to initialize directories to use
        test_family = test_families.get_test_family(khiops_tool_name)
        used_dir_names = test_family.copy()
        # Add unofficial test directories if requested
        if include_unofficial_sub_dirs:
            # Sort all actual directories by name to ensure stability accross platforms
            all_dir_names = os.listdir(tool_test_path)
            all_dir_names.sort()
            for dir_name in all_dir_names:
                if not dir_name in test_family:
                    # Unofficial directopries are with an '_' in second char (e.g. z_work)
                    if dir_name.find("_") == 1:
                        root_path = os.path.join(tool_test_path, dir_name)
                        if os.path.isdir(root_path):
                            used_dir_names.append(dir_name)
        # Execute command on all used directories
        for dir_name in used_dir_names:
            root_path = os.path.join(tool_test_path, dir_name)
            if os.path.isdir(root_path):
                apply_command.execute_command(all_commands, command, root_path)
            else:
                print("BUG directory not found: " + root_path)
