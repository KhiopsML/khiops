// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "FileService.h"
#include "ALString.h"
#include "Timer.h"

// Etude de creation de fichier selon different mode
//   <RootDir> <FileSize>
void StudyCreateLargeFiles(int argc, char** argv);

// Test de creation de fichier de tres grande taille
//   <RootDir> <MaxSize> <IterNumber>
void TestCreateLargeFiles(int argc, char** argv);