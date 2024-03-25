#!/bin/bash 

# Absolute path to this script
SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

# Name of current script, without extension
SCRIPTFULLNAME=$(basename "$SCRIPT")
SCRIPTNAME="${SCRIPTFULLNAME%.*}"

# Call python script
python3 $SCRIPTPATH/../py/$SCRIPTNAME.py "$@"

