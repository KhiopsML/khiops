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
#ifdef KNI_EXPORTS
#define KNI_API __declspec(dllexport)
#else
#define KNI_API __declspec(dllimport)
#endif
#else
#define KNI_API __attribute__((visibility("default")))
#endif

	/**********************************************************************************************************
	 * The purpose of Khiops Native Interface (KNI) is to allow a deep integration of Khiops
	 * in information systems, by the mean of the C programming language, using a dynamic link library (DLL).
	 * This relates especially to the problem of model deployment, which otherwise requires the use of
	 * input and output data files when using directly the Khiops tool in batch mode.
	 * See Khiops Guide for an introduction to dictionary files, dictionaries, database files and deployment.
	 *
	 * The Khiops deployment features are thus made public through an API with a DLL.
	 * Therefore, a Khiops model can be deployed directly from any programming language, such as
	 * C, C++, Java, Python, Matlab...
	 * This enables model deployment in real time application (e.g. scoring in a marketing context,
	 * targeted advertising on the web) without the overhead of temporary data files or launching executables.
	 *
	 * All KNI functions are C functions for easier use with other programming languages.
	 * They return a positive or null value in case of success, and a negative error code in case of failure.
	 * The functions are not reentrant (thread-safe): the DLL can be used simultaneously by several executables,
	 * but not simultaneously by several threads in the same executable.
	 **********************************************************************************************************/

	/*
	 * Max number of streams
	 */
#define KNI_MaxStreamNumber 512

	/*
	 * Default max memory per stream in MB
	 */
#define KNI_DefaultMaxStreamMemory 100

	/*
	 * Max length of characters strings
	 * Each string must contain a null-termination  character ('\0')
	 * An error is raised if this null character  is not found before the max length limit
	 */
#define KNI_MaxPathNameLength 1024
#define KNI_MaxDictionaryNameLength 128
#define KNI_MaxRecordLength 1048576

	/*
	 * Possible return values for KNI functions.
	 * (strictly positive integers are stream handles)
	 */
#define KNI_OK 0
#define KNI_ErrorRunningFunction (-1)       /* Other KNI function currently running: reentrant calls not allowed */
#define KNI_ErrorDictionaryFileName (-2)    /* Bad dictionary file name */
#define KNI_ErrorDictionaryMissingFile (-3) /* Dictionary file does not exist */
#define KNI_ErrorDictionaryFileFormat (-4)  /* Bad dictionary format: syntax error in dictionary file */
#define KNI_ErrorDictionaryName (-5)        /* Bad dictionary name */
#define KNI_ErrorMissingDictionary (-6)     /* Dictionary not found in dictionary file */
#define KNI_ErrorTooManyStreams (-7)   /* Too many streams: number of simultaneously opened streams exceeds limit */
#define KNI_ErrorStreamHeaderLine (-8) /* Bad stream header line */
#define KNI_ErrorFieldSeparator (-9)   /* Bad field separator */
#define KNI_ErrorStreamHandle (-10)    /* Bad stream handle: handle does not relate to an opened stream */
#define KNI_ErrorStreamOpened (-11)    /* Stream opened */
#define KNI_ErrorStreamNotOpened (-12) /* Stream not opened */
#define KNI_ErrorStreamInputRecord                                                                                     \
	(-13) /* Bad input record: null-termination character not found before maximum string length */
#define KNI_ErrorStreamInputRead (-14) /* Problem in reading input record */
#define KNI_ErrorStreamOutputRecord                                                                                    \
	(-15) /* Bad output record: output fields require more space than maximum string length */
#define KNI_ErrorMissingSecondaryHeader (-16)   /* Missing specification of secondary table header */
#define KNI_ErrorMissingExternalTable (-17)     /* Missing specification of external table */
#define KNI_ErrorDataRoot (-18)                 /* Bad data root for an external table */
#define KNI_ErrorDataPath (-19)                 /* Bad data path */
#define KNI_ErrorDataTableFile (-20)            /* Bad data table file */
#define KNI_ErrorLoadDataTable (-21)            /* Problem in loading external data tables */
#define KNI_ErrorMemoryOverflow (-22)           /* Too much memory currently used */
#define KNI_ErrorStreamOpening (-23)            /* Stream could not be opened */
#define KNI_ErrorStreamOpeningNotFinished (-24) /* Multi-tables stream opening not finished */
#define KNI_ErrorLogFile (-25)                  /* Bad error file */

	/*
	 * Version of KNI
	 */
#define KNI_VERSION_10_1 110
#define KNI_VERSION_10_0 100
#define KNI_VERSION_9_0 90
#define KNI_VERSION_8_0 80
#define KNI_VERSION_7_5 75

	/*
	 * Get version of KNI
	 * Enable to check the major version of the DLL
	 *
	 * Return code:
	 *    version number, an integer constant
	 */
	KNI_API int KNIGetVersion();

	/*
	 * Get full version of KNI
	 * Enable to check the full version of the DLL
	 *
	 * Return code:
	 *    full version as a sequence-based identifier (ex: "9.0.1")
	 */
	KNI_API const char* KNIGetFullVersion();

	/**********************************************************************************************
	 * Management of streams
	 * A stream is equivalent of a database, where the records come from a data source on the fly.
	 * A stream must be created to declare its characteristics, then opened, then exploited for
	 * deployment to recode input stream records to output records, and finally closed.
	 * Each record is a null-terminated character string, with values separated using
	 * a field separator. Space characters are trimed at the begining and end of each field.
	 * The following functions apply in the mono-table case.
	 * Extension to the multi-table case is described further.
	 **********************************************************************************************/

	/*
	 * Open a stream
	 * Parameters:
	 *    Name of the dictionary file
	 *    Name of the dictionary
	 *    Header line of the stream, like a stream record containing all the field labels:
	 *       must be consistent with the dictionary
	 *    Field separator: any char ('\t', ';', ',', ' '...) except '\n' and '\0'
	 * In case of multi-table schema with use of secondary tables and external tables, additional
	 * specification are required after the creation of the stream, and before finishing opening it.
	 * (see below the multi-table functions)
	 * In case of failure, the stream is closed.
	 *
	 * Success return codes:
	 *    Any strictly positive code (> 0): a stream handle that will be used to reference streams
	 * Failure return codes:
	 *    KNI_ErrorRunningFunction
	 *    KNI_ErrorDictionaryFileName
	 *    KNI_ErrorDictionaryMissingFile
	 *    KNI_ErrorDictionaryFileFormat
	 *    KNI_ErrorDictionaryName
	 *    KNI_ErrorMissingDictionary
	 *    KNI_ErrorTooManyStreams
	 *    KNI_ErrorStreamHeaderLine
	 *    KNI_ErrorFieldSeparator
	 *    KNI_ErrorStreamOpening
	 *    KNI_ErrorMemoryOverflow
	 */
	KNI_API int KNIOpenStream(const char* sDictionaryFileName, const char* sDictionaryName,
				  const char* sStreamHeaderLine, char cFieldSeparator);

	/*
	 * Close a stream
	 * Parameters:
	 *    Handle of stream
	 * All the memory related to the stream (dictionary, external tables, current secondary records)
	 * is cleaned and the handle of the stream is no longer valid.
	 *
	 * Success return codes:
	 *    KNI_OK
	 * Failure return codes:
	 *    KNI_ErrorRunningFunction
	 *    KNI_ErrorStreamHandle
	 */
	KNI_API int KNICloseStream(int hStream);

	/*
	 * Recode an input stream record, using a dictionary to compute output fields
	 * The licence is checked every one or two minutes: it can be valid when
	 * the stream was opened, and become invalid after few minutes if the licence
	 * new check failed.
	 *
	 * Parameters:
	 *    Handle of stream
	 *    Input record
	 *    Output record: must be allocated by the caller, with size nOutputMaxLength chars,
	 *                   to store the recoding result (including the null-termininating character)
	 *                   Output record is empty in case of failure
	 *    Max length of output record
	 *
	 * Success return codes:
	 *    KNI_OK
	 * Failure return codes:
	 *    KNI_ErrorRunningFunction
	 *    KNI_ErrorStreamHandle
	 *    KNI_ErrorStreamNotOpened
	 *    KNI_ErrorStreamOpeningNotFinished
	 *    KNI_ErrorStreamInputRecord
	 *    KNI_ErrorStreamInputRecordFormat
	 *    KNI_ErrorStreamOutputRecord
	 */
	KNI_API int KNIRecodeStreamRecord(int hStream, const char* sStreamInputRecord, char* sStreamOutputRecord,
					  int nOutputMaxLength);

	/**********************************************************************************************
	 * Management of streams in the multi-table case
	 * The extension to the multi-table case requires two kind of specification, after the stream
	 * is created and before it is opened:
	 *  . specify the header line of each secondary table
	 *  . declare each external tables
	 * Secondary tables are identified by a data path (name of the Entity or Table variable of the
	 * stream main dictionary).
	 * External tables are identified by a data root (root dictionary of the external table) and
	 * optionally a data path for secondary external tables.
	 * Secondary records related to the main record to recode just need a data path to be identified.
	 * For multi-table schemas beyond the star schema (snowflake schema), the variables names in the
	 * data path are separated by back-quote characters.
	 * For single-table schema, none of the methods dedicated to the multi-table case can be used.
	 * See Khiops Guide and tutorial for detailed specification of data root and data path.
	 **********************************************************************************************/

	/*
	 * Set the header line of a secondary table of the stream.
	 * This function must be called for each secondary table of the multi-table schema,
	 * once the stream is successfully created and  before opening it.
	 *
	 * Success return codes:
	 *    KNI_OK
	 * Failure return codes:
	 *    KNI_ErrorRunningFunction
	 *    KNI_ErrorStreamHandle
	 *    KNI_ErrorStreamOpened
	 *    KNI_ErrorDataPath
	 *    KNI_ErrorStreamHeaderLine
	 */
	KNI_API int KNISetSecondaryHeaderLine(int hStream, const char* sDataPath,
					      const char* sStreamSecondaryHeaderLine);

	/*
	 * Set the name of a data file related to an external table.
	 * This function must be called for each external table of the multi-table schema,
	 * once the stream is successfully created and  before opening it.
	 *
	 * Success return codes:
	 *    KNI_OK
	 * Failure return codes:
	 *    KNI_ErrorRunningFunction
	 *    KNI_ErrorStreamHandle
	 *    KNI_ErrorStreamOpened
	 *    KNI_ErrorDataRoot
	 *    KNI_ErrorDataPath
	 *    KNI_ErrorDataTableFile
	 */
	KNI_API int KNISetExternalTable(int hStream, const char* sDataRoot, const char* sDataPath,
					const char* sDataTableFileName);

	/*
	 * Finish opening a stream
	 * Parameters:
	 *    Handle of stream
	 * The stream must be opened and fully specified in case of multi-table schema.
	 * In case of failure, the stream is closed.
	 *
	 * Success return codes:
	 *    KNI_OK
	 * Failure return codes:
	 *    KNI_ErrorRunningFunction
	 *    KNI_ErrorStreamHandle
	 *    KNI_ErrorStreamOpened
	 *    KNI_ErrorMissingSecondaryHeader
	 *    KNI_ErrorMissingExternalTable
	 *    KNI_ErrorLoadExternalTable
	 *    KNI_ErrorMemoryOverflow
	 *    KNI_ErrorStreamOpening
	 */
	KNI_API int KNIFinishOpeningStream(int hStream);

	/*
	 * Set a secondary input record as a preparatation to the recoding of an input record.
	 * All secondary records must be set before calling the recoding function with a primary record.
	 * They can be set in any order, provided that they are all set before recoding the primary
	 * record. Once the primary record is recoded, the memory necessary to store the secondary
	 * records is cleaned
	 * In case of error with this function, all the current secondary records are cleaned.
	 *
	 * Success return codes:
	 *    KNI_OK
	 * Failure return codes:
	 *    KNI_ErrorRunningFunction
	 *    KNI_ErrorStreamHandle
	 *    KNI_ErrorStreamOpeningNotFinished
	 *    KNI_ErrorDataPath
	 *    KNI_ErrorStreamInputRecord
	 *    KNI_ErrorMemoryOverflow
	 */
	KNI_API int KNISetSecondaryInputRecord(int hStream, const char* sDataPath,
					       const char* sStreamSecondaryInputRecord);

	/**********************************************************************************************
	 * Advanced parameters
	 **********************************************************************************************/

	/*
	 * Get the maximum amount of memory (in MB) available for the next opened stream.
	 * Default: KNI_DefaultMaxStreamMemory
	 * This accounts for the memory necessary to store the stream dictionaries, external tables
	 * as well as the secondary records under preparation for recoding.
	 */
	KNI_API int KNIGetStreamMaxMemory();

	/*
	 * Set the maximum amount of memory (in MB) available for the next opened stream.
	 * Return the accepted parameter (bounded between KNI_DefaultMaxStreamMemory and the available RAM).
	 */
	KNI_API int KNISetStreamMaxMemory(int nMaxMB);

	/*
	 * Set the name of a file to log all errors that occur during call to the KNI functions.
	 * Each time an error occur, the related call to the KNI function is logged as well as
	 * the detailed label of the error.
	 * By default, the log file name is empty.
	 * The log file name can be specified multiple times, with different or empty name,
	 * to start of stop logging the errors.
	 * Using a log file name is useful to debug an application, at the expense of extra CPU.
	 *
	 * Success return codes:
	 *    KNI_OK
	 * Failure return codes:
	 *    KNI_ErrorLogFile
	 */
	KNI_API int KNISetLogFileName(const char* sLogFileName);

	/* Use of C linkage from C++ */
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
