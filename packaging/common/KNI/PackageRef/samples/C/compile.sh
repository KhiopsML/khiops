#!/bin/bash

gcc -o KNIRecodeFile KNIRecodeFile.c -lKhiopsNativeInterface -ldl
gcc -o KNIRecodeMTFiles KNIRecodeMTFiles.c -lKhiopsNativeInterface -ldl
