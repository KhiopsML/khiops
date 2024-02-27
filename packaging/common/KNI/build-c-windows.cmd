cl cpp/KNIRecodeFile.c %KNI_HOME%\lib\KhiopsNativeInterface.lib ^
    -I %KNI_HOME%\include /link "/LIBPATH:%KNI_HOME%\bin"
cl cpp/KNIRecodeMTFiles.c %KNI_HOME%\lib\KhiopsNativeInterface.lib ^
    -I %KNI_HOME%\include /link "/LIBPATH:%KNI_HOME%\bin"