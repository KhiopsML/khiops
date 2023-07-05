// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWContinuous.h"
#include "KWSymbol.h"
#include "KWType.h"
#include "KWClass.h"
#include "KWClassDomain.h"
#include "KWDerivationRule.h"
#include "KWObject.h"
#include "KWSTDatabaseTextFile.h"
#include "KWStat.h"
#include "KWAttributeStats.h"
#include "FileService.h"
#include "KWDRAll.h"
#include "Divers.h"
#include "KWDiscretizerTest.h"
#include "KWGrouperTest.h"
#include "KWStatisticalEvaluation.h"
#include "KWPredictorSpecView.h"
#include "KWPredictorSpecArrayView.h"
#include "KWDataGrid.h"
#include "KWSortableIndex.h"
#include "JSONFile.h"

///////////////////////////////////////////////////////////////////////////////
// Etude de la statistique de la surface d'une courbe de lift

void LiftCurveStudy();

///////////////////////////////////////////////////////////////////////////////
// Etude du critere de discretisation MODL

void MODLStudy3();
void MODLStudy4();

// Fusion de deux fichiers, par concacatenation de chacune de leurs lignes
void MergeFiles(const ALString& sInputFile1, const ALString& sInputFile2, const ALString& sOutputFile,
		const ALString& sSeparatorChar);
void mainMergeFiles(int argc, char** argv);

// Remplacements des blancs par des tabs
void ReplaceBlanksByTabs(const ALString& sInputFile, const ALString& sOutputFile);
void mainReplaceBlanksByTabs(int argc, char** argv);

// Transformation d'un fichier avec format de stockage creux en fichier colonne
// Format de stockage creux: liste ordonne des index de colonne (debut a 1) ou se trouve les 1
// de la matrice creuse.
// En sortie: un fichier au format pleain (avec des 0 et 1).
void ExpandSparseBinaryFile(const ALString& sInputFile, const ALString& sOutputFile, char cSeparatorChar,
			    int nColumnNumber);
void mainExpandSparseBinaryFile(int argc, char** argv);

// Transformation du fichier de donnees Reuters (format tres specifique...)
//   Type:
//		0: standard pour la classification de texte
//      1: pour le co-clustering, avec IdText, IdWord, WordCount, Class
//      2: pour le co-clustering, avec IdText, IdWord, Class (chaque ligne repetee WordCount fois)
void TranscodeReutersFile(const ALString& sInputFile, const ALString& sOutputFile, int nType, int nMinWordFrequency);
void mainTranscodeReutersFile(int argc, char** argv);

// Test d'ouverture de nombreux fichiers
void TestOpenManyFiles();

// Tests de performances des entrees-sorties avec fichiers texte
void TestIOPerf();

// Tests de performances CPU
void TestCPUPerf();

// Generation d'un graphe aleatoire scale-free (Barabasi et Albert)
void GenerateScaleFreeGraph(int nNodeNumber, int nDegree, int nEdgeNumber, const ALString& sOutputFile);
void mainGenerateScaleFreeGraph(int argc, char** argv);

// Generation d'une famille de courbes
void mainGenerateCurves(int argc, char** argv);

// Transcodage du jeu de donnees Digits pour l'analyse sous forme de functional data (une courbe par Digit)
void TranscodeDigitDataset();

// Generateur de nombre de Bell, pour un nombre de valeur donne
// Cree un tableau pour tous les nombres de parties possibles
void BellNumberGenerator(int argc, char** argv);

// Generateur de nombre de Bell, pour un couple (V, I) donne
void BellNumber(int argc, char** argv);

// Creation d'un fichier de donnees avec tous les caracteres ascii 1 a 255
void AnyCharFileGenerator();

// Creation d'un fichier de donnees avec tous les caracteres ascii 1 a 255 plus des caractères UTF8
void MixedAsciiUTF8CharFileGenerator();
void BuildAllCharValueFile();
void BuildAllCharValueJsonFile();

// Tests de tri
void SortStudy1();
void SortStudy3();
void SortStudy3();
void SortStudy4();

// Calcul de nombres premiers
void ComputePrimeNumbers();

// Etude des tours de Hanoi
void StudyHanoi();