# Strip absolute paths from #line directives in bison/flex generated files Usage: cmake -DFILE=<file> -DSOURCE_DIR=<dir>
# -P strip_line_directives.cmake
#
# Replaces absolute paths in #line directives with project-relative paths, e.g. #line 1
# "/home/user/khiops/src/Learning/KWData/KWCYac.yac" -> #line 1 "src/Learning/KWData/KWCYac.yac"

file(READ "${FILE}" content)
string(REGEX REPLACE "#line ([0-9]+) \"${SOURCE_DIR}([^\"]+)\"" "#line \\1 \"\\2\"" content "${content}")
file(WRITE "${FILE}" "${content}")
