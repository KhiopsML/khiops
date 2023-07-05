// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import com.sun.jna.Library;
import com.sun.jna.Memory;

public interface KNI extends Library
{

        public final static String LIBRARY_WINDOWS_NAME_64 = "KhiopsNativeInterface64.dll";

        public final static String LIBRARY_LINUX_NAME = "libKhiopsNativeInterface.so";

        /**
         * KNI functions: see KhiopsNativeInterface.h for a detailed description
         */
        int KNIGetVersion();

        int KNIOpenStream(String dictionaryFileName,
                          String dictionaryName,
                          String streamHeaderLine,
                          char fieldSeparator);

        int KNICloseStream(int hStream);

        int KNIRecodeStreamRecord(int hStream,
                                  String sStreamInputRecord,
                                  Memory sStreamOutputRecord,
                                  int nOutputMaxLength);

        /**
         * KNI functions: specific function for multi-tables schema
         */

        int KNISetSecondaryHeaderLine(int hStream, String sDataPath, String sStreamSecondaryHeaderLine);

        int KNISetExternalTable(int hStream, String sDataRoot, String sDataPath, String sDataTableFileName);

        int KNIFinishOpeningStream(int hStream);

        int KNISetSecondaryInputRecord(int hStream, String sDataPath, String sStreamSecondaryInputRecord);

        /**
         * KNI functions: advanced parameters
         */

        int KNIGetStreamMaxMemory();

        int KNISetStreamMaxMemory(int nMaxMo);

        int KNISetLogFileName(String sLogFileName);

        public final static int OK = 0;
}