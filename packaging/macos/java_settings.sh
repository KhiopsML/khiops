# shellcheck shell=bash
# shellcheck disable=SC2034
unset KHIOPS_JAVA_ERROR
unset KHIOPS_JAVA_PATH
unset KHIOPS_CLASSPATH
_KHIOPS_APP_BIN_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
_JRE_DIR="$_KHIOPS_APP_BIN_DIR/../jre"
# libjvm.dylib is loaded via dlopen, not exec'd: only readability is required, not +x
if [[ -f "$_JRE_DIR/lib/server/libjvm.dylib" ]]; then
    KHIOPS_JAVA_PATH="$_JRE_DIR/lib/server"
    KHIOPS_CLASSPATH="$_KHIOPS_APP_BIN_DIR/../jars/norm.jar:$_KHIOPS_APP_BIN_DIR/../jars/khiops.jar"
else
    KHIOPS_JAVA_ERROR="Bundled JRE not found ($_JRE_DIR)"
fi
unset _KHIOPS_APP_BIN_DIR