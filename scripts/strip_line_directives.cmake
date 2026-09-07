# Strip absolute paths from #line directives and bison include guards in bison/flex generated files Usage: cmake
# -DFILE=<file> -DSOURCE_DIR=<dir> -P strip_line_directives.cmake
#
# Replaces absolute paths in #line directives with project-relative paths, e.g. #line 1
# "/home/user/khiops/src/Learning/KWData/KWCYac.yac" -> #line 1 "src/Learning/KWData/KWCYac.yac"
#
# Also strips the sanitized absolute source path from bison-generated header include guards, e.g.
# YY_YY_HOME_USER_KHIOPS_SRC_LEARNING_KWDATA_KWCYAC_HPP_INCLUDED -> YY_YY_SRC_LEARNING_KWDATA_KWCYAC_HPP_INCLUDED

file(READ "${FILE}" content)
string(REGEX REPLACE "#line ([0-9]+) \"${SOURCE_DIR}([^\"]+)\"" "#line \\1 \"\\2\"" content "${content}")

string(TOUPPER "${SOURCE_DIR}" upper_source_dir)
string(REGEX REPLACE "[^A-Za-z0-9]" "_" sanitized_source_dir "${upper_source_dir}")
# sanitized_source_dir is bounded by underscores (from leading/trailing '/'); collapse it to a single underscore instead
# of removing it, to keep the guard's word separator intact
string(REPLACE "${sanitized_source_dir}" "_" content "${content}")

file(WRITE "${FILE}" "${content}")
