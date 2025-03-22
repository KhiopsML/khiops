
# Set java environment if Khiops GUI is installed
unset KHIOPS_JAVA_ERROR
unset KHIOPS_JAVA_PATH
unset KHIOPS_CLASSPATH
if [ -f "/usr/share/khiops/khiops.jar" ]; then
    if [[ -z $JAVA_HOME ]] && [ -f /usr/bin/java ]; then
        JAVA_HOME=$(readlink -f /usr/bin/java | sed "s:bin/java::")
    fi
    if [[ ! -d $JAVA_HOME ]]; then
        KHIOPS_JAVA_ERROR="The JAVA_HOME directory doesn't exist ($JAVA_HOME)."
        unset JAVA_HOME
    else
        JVM_PATH=$(find -L "$JAVA_HOME" -name libjvm.so 2>/dev/null)
        if [[ -z $JVM_PATH ]]; then
            KHIOPS_JAVA_ERROR="libjvm.so not found in JAVA_HOME ($JAVA_HOME)"
        else
            KHIOPS_JAVA_PATH=$(dirname "$JVM_PATH")
            KHIOPS_CLASSPATH=/usr/share/khiops/norm.jar:/usr/share/khiops/khiops.jar
        fi
    fi
fi