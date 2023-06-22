/******************************************************************************
 * Khiops Native Interface (KNI)
 * Copyright (c) 2019 Orange Labs. All rights reserved.
 *****************************************************************************/

#ifdef _MSC_VER
// To disable fopen warnings (Visual C++ deprecated method)
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#include "KNIRecodeMTFiles.h"
#include "KhiopsNativeInterface.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/*
 * Initialize recoding operands
 */
void KNIInitializeRecodingOperands(KNIMTRecodingOperands* recodingOperands)
{
	strcpy(recodingOperands->DictionaryFileName, "");
	strcpy(recodingOperands->DictionaryName, "");
	recodingOperands->FieldSeparator = '\t';
	strcpy(recodingOperands->InputFile.DataPath, "");
	strcpy(recodingOperands->InputFile.FileName, "");
	recodingOperands->InputFile.KeyFieldNumber = 0;
	recodingOperands->SecondaryFileNumber = 0;
	recodingOperands->ExternalFileNumber = 0;
	strcpy(recodingOperands->OutputFileName, "");
	strcpy(recodingOperands->LogFileName, "");
	recodingOperands->MaxMemory = 0;
}

/*
 * Set recoding operands from command line parameters
 * Return 1 if OK, 0 otherwise
 */
int KNIInitializeRecodingOperandsFromCommandLine(KNIMTRecodingOperands* recodingOperands, int argc, char** argv)
{
	int i;
	char* option;
	int nFileIndex;
	int nKeyIndex;
	int nIntegerValue;

	// Initialize recoding operands
	KNIInitializeRecodingOperands(recodingOperands);

	// Analyse command line parameters
	i = 1;
	while (i < argc)
	{
		option = argv[i];

		// Analyze dictionary operand
		if (strcmp(option, "-d") == 0)
		{
			if (argc - i <= 2)
			{
				printf("Error : Two operands are required for -d option\n");
				return EXIT_FAILURE;
			}
			else
			{
				strcpy(recodingOperands->DictionaryFileName, argv[i + 1]);
				strcpy(recodingOperands->DictionaryName, argv[i + 2]);
				i += 3;
			}
		}
		// Analyze field separator operand
		else if (strcmp(option, "-f") == 0)
		{
			if (argc - i <= 1)
			{
				printf("Error : One operand is required for -f option\n");
				return EXIT_FAILURE;
			}
			else
			{
				recodingOperands->FieldSeparator = argv[i + 1][0];
				i += 2;
			}
		}
		// Analyze input file operand
		else if (strcmp(option, "-i") == 0)
		{
			if (argc - i <= 2)
			{
				printf("Error : At least one operands are required for -i option\n");
				return EXIT_FAILURE;
			}
			else
			{
				strcpy(recodingOperands->InputFile.DataPath, "");
				strcpy(recodingOperands->InputFile.FileName, argv[i + 1]);
				i += 2;

				// Analyse key field indexes
				nKeyIndex = 0;
				while (i < argc && argv[i][0] != '-')
				{
					if (nKeyIndex == MAXKEYFIELDNUMBER - 1)
					{
						printf("Error : Too many key fields (max %d) for input file %s\n",
						       MAXKEYFIELDNUMBER, recodingOperands->InputFile.FileName);
						return EXIT_FAILURE;
					}
					nIntegerValue = atoi(argv[i]);
					if (nIntegerValue <= 0)
					{
						printf("Error : Key field index (%s) invalid for input file %s\n",
						       argv[i], recodingOperands->InputFile.FileName);
						return EXIT_FAILURE;
					}
					recodingOperands->InputFile.KeyFieldIndexes[nKeyIndex] = nIntegerValue;
					nKeyIndex++;
					i++;
				}
				recodingOperands->InputFile.KeyFieldNumber = nKeyIndex;

				// Check that key fields are specified
				if (recodingOperands->InputFile.KeyFieldNumber == 0)
				{
					printf("Error : key fields indexes are not specified for input file %s\n",
					       recodingOperands->InputFile.FileName);
					return EXIT_FAILURE;
				}
			}
		}
		// Analyze secondary file operand
		else if (strcmp(option, "-s") == 0)
		{
			if (argc - i <= 2)
			{
				printf("Error : At least two operands are required for -s option\n");
				return EXIT_FAILURE;
			}
			else
			{
				nFileIndex = recodingOperands->SecondaryFileNumber;
				if (nFileIndex == MAXFILENUMBER - 1)
				{
					printf("Error : Too many secondary files (max %d) with -s option\n",
					       MAXFILENUMBER);
					return EXIT_FAILURE;
				}
				strcpy(recodingOperands->SecondaryFiles[nFileIndex].DataPath, argv[i + 1]);
				strcpy(recodingOperands->SecondaryFiles[nFileIndex].FileName, argv[i + 2]);
				recodingOperands->SecondaryFileNumber = nFileIndex + 1;
				i += 3;

				// Analyse key field indexes
				nKeyIndex = 0;
				while (i < argc && argv[i][0] != '-')
				{
					if (nKeyIndex == MAXKEYFIELDNUMBER - 1)
					{
						printf("Error : Too many key fields (max %d) for secondary file %s\n",
						       MAXKEYFIELDNUMBER,
						       recodingOperands->SecondaryFiles[nFileIndex].FileName);
						return EXIT_FAILURE;
					}
					nIntegerValue = atoi(argv[i]);
					if (nIntegerValue <= 0)
					{
						printf("Error : Key field index (%s) invalid for secondary file %s\n",
						       argv[i], recodingOperands->SecondaryFiles[nFileIndex].FileName);
						return EXIT_FAILURE;
					}
					recodingOperands->SecondaryFiles[nFileIndex].KeyFieldIndexes[nKeyIndex] =
					    nIntegerValue;
					nKeyIndex++;
					i++;
				}
				recodingOperands->SecondaryFiles[nFileIndex].KeyFieldNumber = nKeyIndex;

				// Check that key fields are specified
				if (recodingOperands->SecondaryFiles[nFileIndex].KeyFieldNumber == 0)
				{
					printf("Error : key fields indexes are not specified for secondary file %s\n",
					       recodingOperands->SecondaryFiles[nFileIndex].FileName);
					return EXIT_FAILURE;
				}
			}
		}
		// Analyze external file operand
		else if (strcmp(option, "-x") == 0)
		{
			if (argc - i <= 3)
			{
				printf("Error : Three operands are required for -x option\n");
				return EXIT_FAILURE;
			}
			else
			{
				nFileIndex = recodingOperands->ExternalFileNumber;
				if (nFileIndex == MAXFILENUMBER - 1)
				{
					printf("Error : Too many input files (max %d) with -x option\n", MAXFILENUMBER);
					return EXIT_FAILURE;
				}
				strcpy(recodingOperands->ExternalFiles[nFileIndex].DataRoot, argv[i + 1]);
				strcpy(recodingOperands->ExternalFiles[nFileIndex].DataPath, argv[i + 2]);
				strcpy(recodingOperands->ExternalFiles[nFileIndex].FileName, argv[i + 3]);
				recodingOperands->ExternalFileNumber = nFileIndex + 1;
				i += 4;
			}
		}
		// Analyze output file operand
		else if (strcmp(option, "-o") == 0)
		{
			if (argc - i <= 1)
			{
				printf("Error : One operand is required for -o option\n");
				return EXIT_FAILURE;
			}
			else
			{
				strcpy(recodingOperands->OutputFileName, argv[i + 1]);
				i += 2;
			}
		}
		// Analyze log file operand
		else if (strcmp(option, "-e") == 0)
		{
			if (argc - i <= 1)
			{
				printf("Error : One operand is required for -e option\n");
				return EXIT_FAILURE;
			}
			else
			{
				strcpy(recodingOperands->LogFileName, argv[i + 1]);
				i += 2;
			}
		}
		// Analyze memory operand
		else if (strcmp(option, "-m") == 0)
		{
			if (argc - i <= 1)
			{
				printf("Error : One operand is required for -e option\n");
				return EXIT_FAILURE;
			}
			else
			{
				nIntegerValue = atoi(argv[i + 1]);
				if (nIntegerValue <= 0)
				{
					printf("Error : Max memory (%s) invalid\n", argv[i + 1]);
					return EXIT_FAILURE;
				}
				recodingOperands->MaxMemory = nIntegerValue;
				i += 2;
			}
		}
		// Unknown operand
		else
		{
			printf("Error : Wrong operand %d (%s)\n", i, argv[i]);
			return EXIT_FAILURE;
		}
	}

	// Check that at least the input file is initialized
	if (strcmp(recodingOperands->InputFile.FileName, "") == 0)
	{
		printf("Error : input file is not specified\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/*
 * Compare two records of input files according to their key fields
 * (up to the number of fields in the main input file)
 */
int KNICompareInputRecords(char cFieldSeparator, KNIInputFile* mainFile, char* sMainRecord, KNIInputFile* secondaryFile,
			   char* sSecondaryRecord)
{
	int nCompare;
	int i;
	int nKeyIndex1;
	int nKeyIndex2;
	char* sMainField;
	char* sSecondaryField;
	char sKey1[MAXRECORDLENGTH];
	char sKey2[MAXRECORDLENGTH];
	char* sKey;
	int nIndex;

	// Loop on key fields
	nCompare = 0;
	for (i = 0; i < mainFile->KeyFieldNumber; i++)
	{
		nKeyIndex1 = mainFile->KeyFieldIndexes[i];
		nKeyIndex2 = secondaryFile->KeyFieldIndexes[i];

		// Search start key field of main record
		nIndex = 1;
		sMainField = sMainRecord;
		while (sMainField[0] != '\0')
		{
			if (nIndex == nKeyIndex1)
				break;
			sMainField++;
			if (sMainField[0] == cFieldSeparator)
			{
				nIndex++;
				sMainField++;
			}
		}

		// Search start key field of secondary record
		nIndex = 1;
		sSecondaryField = sSecondaryRecord;
		while (sSecondaryField[0] != '\0')
		{
			if (nIndex == nKeyIndex2)
				break;
			sSecondaryField++;
			if (sSecondaryField[0] == cFieldSeparator)
			{
				nIndex++;
				sSecondaryField++;
			}
		}

		// Copy main key field
		sKey = sKey1;
		while (sMainField[0] != '\0' && sMainField[0] != cFieldSeparator && sMainField[0] != '\r' &&
		       sMainField[0] != '\n')
		{
			sKey[0] = sMainField[0];
			sKey++;
			sMainField++;
		}
		sKey[0] = '\0';

		// Copy secondary key field
		sKey = sKey2;
		while (sSecondaryField[0] != '\0' && sSecondaryField[0] != cFieldSeparator &&
		       sSecondaryField[0] != '\r' && sSecondaryField[0] != '\n')
		{
			sKey[0] = sSecondaryField[0];
			sKey++;
			sSecondaryField++;
		}
		sKey[0] = '\0';

		// Compare fields
		nCompare = strcmp(sKey1, sKey2);

		// Do not analyze remaining key fields if unncessary
		if (nCompare != 0)
			break;
	}
	return nCompare;
}

/*
 * Clean a record by supressing the potential last end of line char.
 * This is useful for display purpose, and has no impact on recoding records
 * (input records may be indifferently terminated by '\0' or '\n' then '\0').
 */
char* CleanRecord(char* sRecord)
{
	int nLength;
	nLength = (int)strlen(sRecord);
	if (nLength > 0 && sRecord[nLength - 1] == '\n')
		sRecord[nLength - 1] = '\0';
	return sRecord;
}

/*
 * Recode a set of input files and external files to an output file, using a Khiops dictionary from a dictionary file
 * The recoding parameters stores all required parameters and must be correctly initialized
 * Returns recoded record number
 */
int KNIRecodeMTFiles(KNIMTRecodingOperands* recodingOperands)
{
	int nVerbose = 0; // 0: minimal message; 1: display input parameters; 2: display all file input and output lines
	int nFileIndex;
	int nKeyIndex;
	int nRetCode;
	int hStream;
	char sInputRecord[MAXRECORDLENGTH];
	char sSecondaryRecords[MAXFILENUMBER][MAXRECORDLENGTH];
	char sOutputRecord[MAXRECORDLENGTH];
	FILE* fInputFile;
	FILE* fSecondaryFiles[MAXFILENUMBER];
	FILE* fSecondaryFile;
	FILE* fOutputFile;
	char* sRetCode;
	int nErrorNumber;
	int nRecordNumber;
	int nCompare;

	// Show recoding parameters
	if (nVerbose >= 1)
	{
		printf("Multi-table recoding\n");
		printf("  Dictionary file name: %s\n", recodingOperands->DictionaryFileName);
		printf("  Dictionary name: %s\n", recodingOperands->DictionaryName);
		printf("  Field separator: %c\n", recodingOperands->FieldSeparator);
		printf("  Input file name: %s\n", recodingOperands->InputFile.FileName);
		if (recodingOperands->InputFile.KeyFieldNumber)
		{
			printf("    Key field indexes: ");
			for (nKeyIndex = 0; nKeyIndex < recodingOperands->InputFile.KeyFieldNumber; nKeyIndex++)
			{
				if (nKeyIndex > 0)
					printf(", ");
				printf("%d", recodingOperands->InputFile.KeyFieldIndexes[nKeyIndex]);
			}
			printf("\n");
		}
		for (nFileIndex = 0; nFileIndex < recodingOperands->SecondaryFileNumber; nFileIndex++)
		{
			printf("  Secondary file %d\n", nFileIndex + 1);
			printf("    Data path: %s\n", recodingOperands->SecondaryFiles[nFileIndex].DataPath);
			printf("    File name: %s\n", recodingOperands->SecondaryFiles[nFileIndex].FileName);
			if (recodingOperands->SecondaryFiles[nFileIndex].KeyFieldNumber)
			{
				printf("    Key field indexes: ");
				for (nKeyIndex = 0;
				     nKeyIndex < recodingOperands->SecondaryFiles[nFileIndex].KeyFieldNumber;
				     nKeyIndex++)
				{
					if (nKeyIndex > 0)
						printf(", ");
					printf("%d",
					       recodingOperands->SecondaryFiles[nFileIndex].KeyFieldIndexes[nKeyIndex]);
				}
				printf("\n");
			}
		}
		for (nFileIndex = 0; nFileIndex < recodingOperands->ExternalFileNumber; nFileIndex++)
		{
			printf("  External file %d\n", nFileIndex + 1);
			printf("    Data root: %s\n", recodingOperands->ExternalFiles[nFileIndex].DataRoot);
			printf("    Data path: %s\n", recodingOperands->ExternalFiles[nFileIndex].DataPath);
			printf("    File name: %s\n", recodingOperands->ExternalFiles[nFileIndex].FileName);
		}
		printf("  Output file name: %s\n", recodingOperands->OutputFileName);
		printf("  Log file name: %s\n", recodingOperands->LogFileName);
		printf("  Max memory (MB): %d\n", recodingOperands->MaxMemory);
	}

	// Set log file
	if (strcmp(recodingOperands->LogFileName, "") != 0)
		KNISetLogFileName(recodingOperands->LogFileName);

	// Set max memory
	if (recodingOperands->MaxMemory > 0)
	{
		KNISetStreamMaxMemory(recodingOperands->MaxMemory);
		if (nVerbose >= 1)
			printf("Actual max memory (MB): %d\n", KNIGetStreamMaxMemory());
	}

	// Open input and output files
	nErrorNumber = 0;
	fInputFile = fopen(recodingOperands->InputFile.FileName, "r");
	if (fInputFile == NULL)
	{
		printf("Error : Unable to open input file %s\n", recodingOperands->InputFile.FileName);
		nErrorNumber++;
	}
	for (nFileIndex = 0; nFileIndex < recodingOperands->SecondaryFileNumber; nFileIndex++)
	{
		fSecondaryFile = fopen(recodingOperands->SecondaryFiles[nFileIndex].FileName, "r");
		fSecondaryFiles[nFileIndex] = fSecondaryFile;
		if (fSecondaryFile == NULL)
		{
			printf("Error : Unable to open secondary file %s\n",
			       recodingOperands->SecondaryFiles[nFileIndex].FileName);
			nErrorNumber++;
		}

		// Check that number of key fields is greater than 0 in case of multi-tables schema
		if (recodingOperands->SecondaryFileNumber >= 2 &&
		    recodingOperands->SecondaryFiles[nFileIndex].KeyFieldNumber == 0)
		{
			printf("Error : Secondary file %s should be specified with key field indexes (multi-tables "
			       "schema)\n",
			       recodingOperands->SecondaryFiles[nFileIndex].FileName);
			nErrorNumber++;
		}

		// Check that number of key fields in secondary input file is greater or equal than that of input file
		if (recodingOperands->SecondaryFiles[nFileIndex].KeyFieldNumber <
		    recodingOperands->InputFile.KeyFieldNumber)
		{
			printf("Error : Number of key fields (%d) in secondary file %s should greater or equal than "
			       "that of input file (%d)\n",
			       recodingOperands->SecondaryFiles[nFileIndex].KeyFieldNumber,
			       recodingOperands->SecondaryFiles[nFileIndex].FileName,
			       recodingOperands->InputFile.KeyFieldNumber);
			nErrorNumber++;
		}
	}
	fOutputFile = fopen(recodingOperands->OutputFileName, "w");
	if (fOutputFile == NULL)
	{
		printf("Error : Unable to open output file %s\n", recodingOperands->OutputFileName);
		nErrorNumber++;
	}

	// Open KNI stream
	hStream = -1;
	nRecordNumber = 0;
	if (nErrorNumber == 0)
	{
		// Read header line of input file
		sRetCode = fgets(sInputRecord, MAXRECORDLENGTH, fInputFile);

		// Open stream
		if (sRetCode != NULL)
		{
			hStream = KNIOpenStream(recodingOperands->DictionaryFileName, recodingOperands->DictionaryName,
						sInputRecord, recodingOperands->FieldSeparator);
			if (hStream < 0)
			{
				printf("Error : Open stream error: %d\n", hStream);
				nErrorNumber++;
			}
		}
	}

	// Set specific multi-table parameters
	if (nErrorNumber == 0 &&
	    (recodingOperands->SecondaryFileNumber > 0 || recodingOperands->ExternalFileNumber > 0))
	{
		// Read header line of secondary files
		for (nFileIndex = 0; nFileIndex < recodingOperands->SecondaryFileNumber; nFileIndex++)
		{
			fSecondaryFile = fSecondaryFiles[nFileIndex];
			sRetCode = fgets(sInputRecord, MAXRECORDLENGTH, fSecondaryFile);
			nRetCode = KNISetSecondaryHeaderLine(
			    hStream, recodingOperands->SecondaryFiles[nFileIndex].DataPath, sInputRecord);
			if (nRetCode != KNI_OK)
				nErrorNumber++;
		}

		// Set external tables
		for (nFileIndex = 0; nFileIndex < recodingOperands->ExternalFileNumber; nFileIndex++)
		{
			nRetCode = KNISetExternalTable(hStream, recodingOperands->ExternalFiles[nFileIndex].DataRoot,
						       recodingOperands->ExternalFiles[nFileIndex].DataPath,
						       recodingOperands->ExternalFiles[nFileIndex].FileName);
			if (nRetCode != KNI_OK)
				nErrorNumber++;
		}

		// Finish opening the stream
		nRetCode = KNIFinishOpeningStream(hStream);
		if (nRetCode < 0)
		{
			printf("Error : Finish opening stream error: %d\n", nRetCode);
			nErrorNumber++;
		}
	}

	// Recode all records of the input file
	if (nErrorNumber == 0)
	{
		// Initialize empty records for all secondary files
		for (nFileIndex = 0; nFileIndex < recodingOperands->SecondaryFileNumber; nFileIndex++)
			sSecondaryRecords[nFileIndex][0] = '\0';

		// Loop on input records
		sInputRecord[0] = '\0';
		while (!feof(fInputFile))
		{
			// Read input record
			sRetCode = fgets(sInputRecord, MAXRECORDLENGTH, fInputFile);

			// Recode record
			if (sRetCode != NULL)
			{
				if (nVerbose >= 2)
					printf("Main: %.20s\n", CleanRecord(sInputRecord));

				// Read lines of secondary input files with same key fields
				for (nFileIndex = 0; nFileIndex < recodingOperands->SecondaryFileNumber; nFileIndex++)
				{
					fSecondaryFile = fSecondaryFiles[nFileIndex];

					// Exploit last stored record (empty record the first time)
					nCompare = KNICompareInputRecords(recodingOperands->FieldSeparator,
									  &recodingOperands->InputFile, sInputRecord,
									  &recodingOperands->SecondaryFiles[nFileIndex],
									  sSecondaryRecords[nFileIndex]);
					if (nCompare == 0)
						KNISetSecondaryInputRecord(
						    hStream, recodingOperands->SecondaryFiles[nFileIndex].DataPath,
						    sSecondaryRecords[nFileIndex]);

					// Read new records if main key greater than secondary key
					if (nCompare >= 0)
					{
						while (!feof(fSecondaryFile))
						{
							sRetCode = fgets(sSecondaryRecords[nFileIndex], MAXRECORDLENGTH,
									 fSecondaryFile);
							if (sRetCode == NULL)
								break;

							// Compare key with main record
							nCompare = KNICompareInputRecords(
							    recodingOperands->FieldSeparator,
							    &recodingOperands->InputFile, sInputRecord,
							    &recodingOperands->SecondaryFiles[nFileIndex],
							    sSecondaryRecords[nFileIndex]);
							if (nVerbose >= 2)
							{
								char* sSigns = "<=>";
								printf("  %c %.20s: %.20s\n", sSigns[nCompare + 1],
								       recodingOperands->SecondaryFiles[nFileIndex]
									   .DataPath,
								       CleanRecord(sSecondaryRecords[nFileIndex]));
							}

							// Secondary record is related to the main record (same key)
							if (nCompare == 0)
							{
								KNISetSecondaryInputRecord(
								    hStream,
								    recodingOperands->SecondaryFiles[nFileIndex]
									.DataPath,
								    sSecondaryRecords[nFileIndex]);
							}
							// Stop reading if secondary record greater than main record
							// (record stored for next main record)
							else if (nCompare < 0)
								break;
							// Otherwise, continue reading if secondary record smaller than
							// main record (orphan secondary record)
						}
					}
				}

				// Recode main record, once all related secondary records have been set
				nRetCode = KNIRecodeStreamRecord(hStream, sInputRecord, sOutputRecord, MAXRECORDLENGTH);
				if (nRetCode != KNI_OK)
				{
					printf("Error : Recode failure (%d) in record %.20s\n", nRetCode,
					       CleanRecord(sInputRecord));
					nErrorNumber++;
				}

				// Write output record
				if (nRetCode == KNI_OK)
				{
					fprintf(fOutputFile, "%s\n", sOutputRecord);
					nRecordNumber++;
					if (nVerbose >= 2)
						printf(" -> %.20s\n", sOutputRecord);
				}
			}
		}
	}

	// Close files
	if (fInputFile != NULL)
		fclose(fInputFile);
	for (nFileIndex = 0; nFileIndex < recodingOperands->SecondaryFileNumber; nFileIndex++)
	{
		fSecondaryFile = fSecondaryFiles[nFileIndex];
		if (fSecondaryFile != NULL)
		{
			if (nVerbose >= 2 && !feof(fSecondaryFile))
				printf("Error : Remaining records in file %s\n",
				       recodingOperands->SecondaryFiles[nFileIndex].FileName);
			fclose(fSecondaryFile);
		}
	}
	if (fOutputFile != NULL)
		fclose(fOutputFile);

	// Close stream
	if (hStream >= 0)
		nRetCode = KNICloseStream(hStream);

	// End message
	printf("Recoded record number: %d\n", nRecordNumber);
	return nRecordNumber;
}

/*
 * Recode a set of multi-tables input files to an output file.
 *   -d: <input dictionary file> <input dictionary>
 *   [-f: <field separator>]
 *   -i: <input file name> [<key index>...]
 *   -s: <secondary data path> <secondary file name> <key index>...
 *   -x: <external data root> <external data path> <external file name>
 *   -o: <output file name>
 *   [-e: <error file name>]
 *   [-m: <max memory (MB)>]
 * Data files must have a header line.
 * The field separator is tabulation by default.
 * One file must be specified per input or external file (see Khiops documentation),
 * with the first input file corresponding to the input dictionary.
 * In case of multi-tables schema, each input file must be specified and the index of
 * their key fields (starting at 1) must be specified.
 * These key fields index do not belong to the KNI API, but they are necessary in this
 * sample code to read all the input files synchronously.
 */
void mainKNIRecodeMTFiles(int argc, char** argv)
{
	KNIMTRecodingOperands recodingOperands;
	int retCode;

	// Analyse command line and stores the parameters into a KNIMTRecodingOperands structure.
	retCode = KNIInitializeRecodingOperandsFromCommandLine(&recodingOperands, argc, argv);

	// Print help in case of error
	if (retCode == 1)
	{
		printf("\nRecode a set of multi-tables input files to an output file.\n");
		printf("  -d: <input dictionary file> <input dictionary>\n");
		printf("  [-f: <field separator>]\n");
		printf("  -i: <input file name> [<key index>...]\n");
		printf("  -s: <secondary data path> <secondary file name> <key index>...\n");
		printf("  -x: <external data root> <external data path> <external file name>\n");
		printf("  -o: <output file name>\n");
		printf("  [-e: <error file name>]\n");
		printf("  [-m: <max memory (MB)>]\n");
		printf("Data file must have a header line. \n");
		printf("The field separator is tabulation by default.\n");
		printf("The main input file must be the first one.\n");
		printf("For input files in multi-tables schema, the index of the key fields (starting at 1) must be "
		       "specified.\n");
	}
	// Recode files
	else
		KNIRecodeMTFiles(&recodingOperands);
}