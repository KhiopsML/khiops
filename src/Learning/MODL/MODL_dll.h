// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#ifdef __ANDROID__

/* Use of C linkage from C++ */
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

	/* The following ifdef block is the standard way of creating macros
	 * which make exporting from a DLL simpler, in Visual C++.
	 * All files within this DLL are compiled with the KNI_EXPORTS symbol defined.
	 * This symbol should not be defined on any project that uses this DLL.
	 * This way any other project whose source files include this file see
	 * KNI_API functions as being imported from a DLL, whereas this DLL
	 * sees symbols defined with this macro as being exported.
	 */
#ifdef _MSC_VER
	/* Windows Visual C++ only */
#ifdef KHIOPS_EXPORTS
#define KHIOPS_API __declspec(dllexport)
#else
#define KHIOPS_API __declspec(dllimport)
#endif
#else
#define KHIOPS_API __attribute__((visibility("default")))
#endif

	/*
	 * Version of KHIOPS_API
	 */
#define KHIOPS_API_VERSION_10_0 100

	/*
	 * Get version of Khiops
	 * Enable to check the version of the DLL
	 *
	 * Success return code:
	 *    0
	 * Failure return code:
	 *    1
	 */
	KHIOPS_API int GetVersion();

	/*
	 * Start Khiops with a scenario, a log file and a task file. With the standalone Khiops tool,
	 * this si similar to "khiops -b -i  sInputScenario -e sLogFileName -t sTaskFileName"
	 * The parameter sTaskFileName can be an empty string, in this case, khiops is launch without the "-t" flag
	 *
	 * Success return code:
	 *    0
	 * Failure return code:
	 *    1
	 */
	KHIOPS_API int StartKhiops(const char* sInputScenario, const char* sLogFileName, const char* sTaskFileName);

	/* Use of C linkage from C++ */
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif // __ANDROID__
