import os
import os.path
import shutil
import string


def copy(src, dest):
    try:
        shutil.copy(src, dest)
    except BaseException as message:
        print("can't copy " + src + " (" + str(message) + ")")


def copyfile(src, dst):
    try:
        shutil.copyfile(src, dst)
    except BaseException as message:
        print("can't copy " + src + " (" + str(message) + ")")


def copyFilesStartingWith(src, dest, start):
    lowerStart = start.lower()
    for fileName in os.listdir(src):
        lowerFileName = fileName.lower()
        if not os.path.isdir(os.path.join(src, fileName)):
            if lowerFileName.startswith(lowerStart):
                shutil.copy(os.path.join(src, fileName), dest)


def copyFilesWithExtension(src, dest, tabExt):
    for fileName in os.listdir(src):
        if not os.path.isdir(os.path.join(src, fileName)):
            (root, extension) = os.path.splitext(fileName)
            if extension in tabExt:
                copy(os.path.join(src, fileName), dest)


def copyFilesWithoutExtension(src, dest, tabExt):
    for fileName in os.listdir(src):
        if not os.path.isdir(os.path.join(src, fileName)):
            (root, extension) = os.path.splitext(fileName)
            if not extension in tabExt:
                copy(os.path.join(src, fileName), dest)


def copyAllFiles(src, dest):
    for fileName in os.listdir(src):
        if not os.path.isdir(os.path.join(src, fileName)):
            copy(os.path.join(src, fileName), dest)


def deleteAllFiles(src):
    for fileName in os.listdir(src):
        if not os.path.isdir(os.path.join(src, fileName)):
            os.remove(os.path.join(src, fileName))


# copie recursive sauf des repertoires contenus dans dirList (dirlist doit etre en minuscule)
def copyTree(src, dst, dirList):
    names = os.listdir(src)
    os.mkdir(dst)
    for name in names:
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)
        try:
            if os.path.isdir(srcname):
                if name.lower() not in dirList:
                    copyTree(srcname, dstname, dirList)
            else:
                copy(srcname, dstname)
        except (IOError, os.error) as why:
            print("Can't copy %s to %s: %s" % (str(srcname), str(dstname), str(why)))


if __name__ == "__main__":
    print("not standalone module, contains")
    print("\tcopy(src,dest)")
    print("\tcopyfile(src,dst)")
    print("\tcopyFilesStartingWith(src,dest,start)")
    print("\tcopyFilesWithExtension(src,dest,tabExt)")
    print("\tcopyFilesWithoutExtension(src,dest,tabExt)")
    print("\tcopyAllFiles(src,dest)")
    print("\tdeleteAllFiles(src)")
