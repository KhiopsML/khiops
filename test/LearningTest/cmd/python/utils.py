import os
import os.path
import shutil
import stat


def copy(src, dest):
    try:
        shutil.copy(src, dest)
    except BaseException as message:
        print("can't copy " + src + " (" + str(message) + ")")


def remove_file(file_path):
    """Remove file"""
    os.chmod(file_path, stat.S_IWRITE)
    try:
        os.remove(file_path)
    except (IOError, os.error) as why:
        print("Cannot remove file %s: %s" % (file_path, str(why)))


def remove_dir(dir_path):
    """Remove empty directory"""
    try:
        os.rmdir(dir_path)
    except (IOError, os.error) as why:
        print("Cannot remove directory %s: %s" % (dir_path, str(why)))
