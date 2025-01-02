// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDatabaseTransferTask.h"
#include "KWArtificialDataset.h"
#include "KWDRMultiTable.h"

/////////////////////////////////////////////////
// Test de transfer de base de donnees
class KWTestDatabaseTransfer : public Object
{
public:
	// Creation de la classe associee a une table artificielle
	static KWClass* ComputeArtificialClass(KWArtificialDataset* artificialDataset);

	// Methode de test globale
	static void Test();

	//////////////////////////////////////////////////////
	// Methodes de test dans le cas de bases mono-tables

	// Methode principale
	static void STTest();

	// Test de transfert avec fichier genere en faisant varier la taille des buffers et la taille des fichiers: les
	// fichiers de sortie doivent etre les memes
	static boolean STTestTransfer();

	// Test de transfert de fichier
	//   Si WriteFileName est absent: pas d'ecriture
	//   Si ClassFileName est absent: la classe est lue d'apres le fichier
	static void STMainTestReadWrite(int argc, char** argv);
	static void STTestReadWrite(const ALString& sClassFileName, const ALString& sClassName,
				    const ALString& sReadFileName, const ALString& sWriteFileName);

	//////////////////////////////////////////////////////
	// Methodes de test dans le cas de bases multi-tables

	// Methode principale
	static void MTTest();

	// Test en specifiant les caracteristiques d'une base multi-tables au moyen de jeux de donnees artificiels
	// TableNumber:
	//   . 0: cas mono-table, sans identifiants
	//   . 1: une table racine avec identifiants
	//   . 2: une table racine avec une table secondaire en relation 0-n
	//   . 3: une table racine avec une table secondaire en relation 0-1 (identique a table principale), et
	//        une autre table secondaire en relation 0-n avec la seconde table
	//   . 4: comme la cas (3), avec en plus une table externe (identique a la table principale)
	// UsedTableNumber: pour mettre en Unused tout ou partie des tables et des regles de calcul
	// UseBuildRules: utilisation des regles de calcul liant les tables
	// RootLineNumber, RootLineNumberPerKey, RootSamplingRate: caracteristique de la table principale
	//    (et de la table en relation 0-1, et de la table externe)
	// SecondaryLineNumber, SecondaryLineNumberPerKey, SecondarySamplingRate: caracteristique de la table en
	// relation 0-n La taille de buffer ou des fichier par process n'est prise en compte que si elle est differente
	// de 0.
	static void MTTestWithArtificialDatabase(int nTableNumber, int nUsedTableNumber, boolean bUseBuildRules,
						 int nRootLineNumber, int nRootLineNumberPerKey,
						 double dRootSamplingRate, int nSecondaryLineNumber,
						 int nSecondaryLineNumberPerKey, double dSecondarySamplingRate,
						 int nBufferSize, int lMaxFileSizePerProcess,
						 const ALString& sTestLabel);

	// Test de transfert de fichier
	//   Si RootWriteFileName est absent: pas d'ecriture
	static void MTMainTestReadWrite(int argc, char** argv);
	static void MTTestReadWrite(const ALString& sClassFileName, const ALString& sClassName,
				    const ALString& sRootReadFileName, const ALString& sSecondaryReadFileName,
				    const ALString& sRootWriteFileName, const ALString& sSecondaryWriteFileName);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation

	// Test le transfert (appele dans TestTransfer)
	// Transfert la database avec son dictionnaire
	// Compare le resultat du transfert avec le fichier initial de reference.
	// Renvoie true si le transfert correspond bien au fichier
	static boolean STIsTransferOk(KWSTDatabaseTextFile* databaseSource, const ALString& sReferenceFilePathName);
};
