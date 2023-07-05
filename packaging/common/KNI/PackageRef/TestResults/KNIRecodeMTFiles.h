// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

/*
 * Recode a set of multi-tables input files to an output file.
 * See main function mainKNIRecodeMTFiles(argc, argv).
 *
 * The purpose of this sample code is to illustrate the use of the
 * Khiops Native Interface (KNI) API, with a use of each KNI method.
 * It is not intended to be a complete and safe implementation for any
 * use of file-based multi-tables recoding.
 */

// Constants for input structure specifications
#define MAXNAMESIZE 256
#define MAXKEYFIELDNUMBER 10
#define MAXFILENUMBER 10

// Specification of an input file in a multi-tables schema
// The main input file (not secondary) has an empty data path
// Key field indexes are necessary in case of multi-table schema
// to read several files simultaneously and synchronize
// them according the record keys
typedef struct KNIInputFile
{
	char DataPath[MAXNAMESIZE];
	char FileName[MAXNAMESIZE];
	int KeyFieldNumber;
	int KeyFieldIndexes[MAXKEYFIELDNUMBER];
} KNIInputFile;

// Specification of an external file in a multi-tables schema
typedef struct KNIExternalFile
{
	char DataRoot[MAXNAMESIZE];
	char DataPath[MAXNAMESIZE];
	char FileName[MAXNAMESIZE];
} KNIExternalFile;

// Overal specification to store all operands of a multi-tables recoding
typedef struct KNIMTRecodingOperands
{
	char DictionaryFileName[MAXNAMESIZE];
	char DictionaryName[MAXNAMESIZE];
	char FieldSeparator;
	KNIInputFile InputFile;
	int SecondaryFileNumber;
	KNIInputFile SecondaryFiles[MAXFILENUMBER];
	int ExternalFileNumber;
	KNIExternalFile ExternalFiles[MAXFILENUMBER];
	char OutputFileName[MAXNAMESIZE];
	char LogFileName[MAXNAMESIZE];
	int MaxMemory;
} KNIMTRecodingOperands;

// Maximum size of records
#define MAXRECORDLENGTH 10000

/*
 * Recode a set of input files and external files to an output file, using a Khiops dictionary from a dictionary file
 * The recoding parameters stores all required parameters and must be correctly initialized
 * Returns recoded record number
 */
int KNIRecodeMTFiles(KNIMTRecodingOperands* recodingOperands);

// Recode a set of multi-tables input files to an output file.
//   -d: <input dictionary file> <input dictionary>
//   [-f: <field separator>]
//   -i: <input file name> [<key index>...]
//   -s: <secondary data path> <secondary file name> <key index>...
//   -x: <external data root> <external data path> <external file name>
//   -o: <output file name>
//   [-e: <error file name>]
//   [-m: <max memory (MB)>]
void mainKNIRecodeMTFiles(int argc, char** argv);