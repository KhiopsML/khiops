// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifdef _MSC_VER
// To disable fopen warnings (Visual C++ deprecated method)
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#include "KNIRecodeFile.h"
#include "KhiopsNativeInterface.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAXBUFFERSIZE 1000

/*
 * Read a record from a file.
 * Return NULL if error or line too long.
 */
static char* ReadRecord(char* sRecord, int nMaxLineLength, FILE* file)
{
	if (!fgets(sRecord, nMaxLineLength, file))
		return NULL;
	// Discard rest of line if line too long
	if (strchr(sRecord, '\n') == NULL && !feof(file))
	{
		int ch;
		while ((ch = fgetc(file)) != '\n' && ch != EOF)
			;
		return NULL;
	}
	return sRecord;
}

int KNIRecodeFile(const char* sDictionaryFileName, const char* sDictionaryName, const char* sInputFileName,
		  const char* sOutputFileName, const char* sErrorFileName)
{
	int nRetCode;
	int hStream;
	char* sHeaderLine;
	char* sInputRecord;
	char* sOutputRecord;
	FILE* fInputFile;
	FILE* fOutputFile;
	char* sRetCode;
	int nRecordNumber;

	assert(sDictionaryFileName != NULL);
	assert(sDictionaryName != NULL);
	assert(sInputFileName != NULL);
	assert(sOutputFileName != NULL);
	assert(sErrorFileName != NULL);

	// Allocate buffers
	sHeaderLine = (char*)malloc(MAXBUFFERSIZE * sizeof(char));
	sInputRecord = (char*)malloc(MAXBUFFERSIZE * sizeof(char));
	sOutputRecord = (char*)malloc(MAXBUFFERSIZE * sizeof(char));

	// Set error file (empty for no messages)
	KNISetLogFileName(sErrorFileName);

	// Start message
	printf("\nRecode records of %s to %s\n", sInputFileName, sOutputFileName);
	nRecordNumber = 0;

	// Open input and output files
	fInputFile = fopen(sInputFileName, "r");
	fOutputFile = fopen(sOutputFileName, "w");
	if (fInputFile == NULL)
		printf("Error : Unable to open input file %s\n", sInputFileName);
	if (fOutputFile == NULL)
		printf("Error : Unable to open output file %s\n", sOutputFileName);

	// Open KNI stream
	hStream = -1;
	if (fInputFile != NULL && fOutputFile != NULL)
	{
		// Read header line
		sRetCode = ReadRecord(sHeaderLine, MAXBUFFERSIZE, fInputFile);

		// Open stream
		if (sRetCode != NULL)
		{
			hStream = KNIOpenStream(sDictionaryFileName, sDictionaryName, sHeaderLine, '\t');
			if (hStream < 0)
				printf("Error : Open stream error: %d\n", hStream);
		}
	}

	// Recode all records of the input file
	if (hStream > 0)
	{
		// Loop on input records
		while (!feof(fInputFile))
		{
			// Read input record
			sRetCode = ReadRecord(sInputRecord, MAXBUFFERSIZE, fInputFile);

			// Recode record
			if (sRetCode != NULL)
			{
				nRetCode = KNIRecodeStreamRecord(hStream, sInputRecord, sOutputRecord, MAXBUFFERSIZE);

				// Write output record
				if (nRetCode == KNI_OK)
				{
					fprintf(fOutputFile, "%s\n", sOutputRecord);
					nRecordNumber++;
				}
			}
		}
	}

	// Close files
	if (fInputFile != NULL)
		fclose(fInputFile);
	if (fOutputFile != NULL)
		fclose(fOutputFile);

	// Close stream
	if (hStream > 0)
		nRetCode = KNICloseStream(hStream);

	// Free buffers
	free(sHeaderLine);
	free(sInputRecord);
	free(sOutputRecord);

	// End message
	printf("Recoded record number: %d\n", nRecordNumber);
	return nRecordNumber;
}

void mainKNIRecodeFile(int argc, char** argv)
{
	// Display parameters
	if (argc != 5 && argc != 6)
	{
		printf("Deploy <Dictionary file> <Dictionary> <Input File> <Output File> [Error file] \n");
	}
	// Execute command
	else
	{
		// Case without an error file
		if (argc == 5)
			KNIRecodeFile(argv[1], argv[2], argv[3], argv[4], "");
		// Case with an error file
		else
			KNIRecodeFile(argv[1], argv[2], argv[3], argv[4], argv[5]);
	}
}
