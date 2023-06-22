// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "KhiopsNativeInterface.h"
#include "KNIRecodeFile.h"
#include "KNIRecodeMTFiles.h"

// Tests complet de l'API de deploiement, avec des utilisation correcte ou erronnes des fonctions KNI
void KNITest(const char* sDictionaryFileName, const char* sDictionaryName, const char* sInputFileName,
	     const char* sOutputFileName, const char* sErrorFileName);

// Appel de la methode de test depuis un main
void mainKNITest(int argc, char** argv);
