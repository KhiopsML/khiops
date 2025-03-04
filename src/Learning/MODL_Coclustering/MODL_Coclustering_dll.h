// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

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
#ifdef KHIOPS_COCLUSTERING_EXPORTS
#define KHIOPS_COCLUSTERING_API __declspec(dllexport)
#else
#define KHIOPS_COCLUSTERING_API __declspec(dllimport)
#endif
#else
#define KHIOPS_COCLUSTERING_API __attribute__((visibility("default")))
#endif

	/*
	 * Get version of Khiops_coclustering
	 *
	 * Enable to check the major and minor version of the DLL, which is the same as that of the Khiops tool
	 * The version is given as an integer (10*major + minor) to ease comparisons
	 * Exemple:
	 *   75 for Khiops 7.5
	 *   100 for Khiops 10.0
	 *   101 for Khiops 10.1
	 *
	 * Return code:
	 *    version number, an integer constant
	 */
	KHIOPS_COCLUSTERING_API int GetVersion();

	/*
	 * Get full version of Khiops_coclustering
	 * Enable to check the full version of the DLL
	 *
	 * Return code:
	 *    full version as a sequence-based identifier (ex: "9.0.1")
	 */
	KHIOPS_COCLUSTERING_API const char* GetFullVersion();

	/*
	 * Start Khiops Coclustering with a scenario and a log file. With the standalone Khiops tool,
	 * this si similar to "khiops_coclustering -b -i  sInputScenario -e sLogFileName -t sTaskFileName"
	 * The parameter sTaskFileName can be an empty string, in this case, khiops is launch without the "-t" flag
	 *
	 * Success return code:
	 *    0
	 * Failure return code:
	 *    1
	 */
	KHIOPS_COCLUSTERING_API int StartKhiopsCoclustering(const char* sInputScenario, const char* sLogFileName,
							    const char* sTaskFileName);

	/* Use of C linkage from C++ */
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
