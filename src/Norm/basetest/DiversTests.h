// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Timer.h"
#include "Standard.h"
#include "Ermgt.h"
#include "ALString.h"
#include "Object.h"
#include "SortedList.h"
#include "LMLicenseService.h"
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

// Methode de test sur l'ecriture des gros fichier, pour etudier l'impact de la taille des buffer de fichier
// Utilisation temporaire de setvbuf dans les methodes suivantes de FileService, apres ouverture des fichier
//  . OpenInputBinaryFile, OpenOutputBinaryFile, OpenOutputBinaryFileForAppend
//  . appel de setvbuf(fFile, NULL, _IOFBF, 16 * MemSegmentByteSize);
// Resultats:
//  . par defaut, Windows C++ semble utiliser des buffer de 64 ko
//  . passer a des buffers de 1 Mo fait fagner environ 10% du temps: negligeable, sans interet
//  . passer a des buffers de 8 Mo ne fait rien gagner de plus
//  -> ce n'est pas la peine de parametrer les buffers systeme des fichiers
// Temps obtenu en local (sur c:\temp
//   . ecriture de deux fichier de 1 Go: 11s
//  . concatenation de ces deux fichiers: 48s
void TestBigFile();

// Etude sur la gestion de la memoire
void StudyMemoryManagement();

// Etude sur les encodages ansi et utf8
void StudyCharacterEncodings();
