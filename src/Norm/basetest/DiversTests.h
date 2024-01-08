// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Timer.h"
#include "Standard.h"
#include "Ermgt.h"
#include "ALString.h"
#include "Object.h"
#include "SortedList.h"
#include "UITestClassSpecArrayView.h"
#include "UITest.h"
#include "MemoryManager.h"
#include "SystemResource.h"
#include "CharVector.h"
#include "InputBufferedFile.h"
#include "OutputBufferedFile.h"

// Affichage des informations d'identification de la machine
void ShowKeyInfo();

// Tests avances sur l'allocateur
void TestMemAdvanced();

// Etude sur la gestion de la memoire
void StudyMemoryManagement();

// Etude sur les encodages ansi et utf8
void StudyCharacterEncodings();
