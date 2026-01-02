// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "FileService.h"
#include "KWContinuous.h"
#include "Timer.h"

//////////////////////////////////////////////////////////////////////////
// Test de performance des routines d'entree-sortie
// Comparaison entre les API C, C++, binaire et texte.
//
// Resultats des tests: API la plus performante
//		- API C fopen, fread, fwrite, fclose
//		- ouverture des fichier en binaire
//		- utilisation d'un buffer de taille fixe egale a 16384 octets
//
// Gestion des retours a la ligne en DOS et UNIX
//		- en DOS: CR LF (\r\n)
//			le CR est vue comme un ^M en fin de ligne sous Unix
//		- en Unix: LF (\n)
//			sous DOS, tout apparait sur une seule ligne
// Solution:
//		- en lecture: ignorer \r et detecter les nouvelle ligne par \n
//		- en ecriture: ecrire \r\n en DOS, et \n en Unix
//
// Interet du mode binaire
//		- permet de gerer des fichier de taille superieure a 2 Go (dont
//		  la gestion par l'API texte semble incorrecte sous Windows)

// Routine principale
void StreamTest(int argc, char** argv);

// Test de comptage des lignes d'un fichier
void TestStreamLineCount1(const ALString& sFileName);
void TestStreamLineCount2(const ALString& sFileName);
void TestStreamLineCount3(const ALString& sFileName);
void TestStreamLineCount4(const ALString& sFileName);
void TestStreamLineCount5(const ALString& sFileName);
void TestStreamLineCount6(const ALString& sFileName);
void TestStreamLineCount7(const ALString& sFileName);
void TestStreamLineCount8(const ALString& sFileName);

// Test de gestion des find de lignes (CR et LF)
void TestStreamEndLine();
