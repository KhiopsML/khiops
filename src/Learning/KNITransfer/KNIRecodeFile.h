// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
/******************************************************************************
 * Khiops Native Interface (KNI)
 * Copyright (c) 2022 Orange Labs. All rights reserved.
 *****************************************************************************/

/*
 * Recode an input file to an output file, using a Khiops dictionary from a dictionary file
 * The input file must have a header line, describing the structure of all its instances
 * The input and output files have a tabular format
 * The error file may be useful for debuging purpose. It is optional and may be empty.
 *
 * Parameters:
 *    Name of the dictionary file
 *    Name of the dictionary
 *    Name of the input file
 *    Name of the output file
 *    Name of the error file
 * Returns recoded record number
 */
int KNIRecodeFile(const char* sDictionaryFileName, const char* sDictionaryName, const char* sInputFileName,
		  const char* sOutputFileName, const char* sErrorFileName);

// Call recoding function from a main-like function
void mainKNIRecodeFile(int argc, char** argv);
