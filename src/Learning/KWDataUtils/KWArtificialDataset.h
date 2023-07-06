// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"
#include "FileService.h"
#include "TaskProgression.h"
#include "InputBufferedFile.h"
#include "OutputBufferedFile.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWArtificialDataset
// Specification et generation d'un jeu de donnees artificiel
class KWArtificialDataset : public Object
{
public:
	// Constructeur
	KWArtificialDataset();
	~KWArtificialDataset();

	/////////////////////////////////////////////////////
	// Specification du fichier

	// Nom du fichier
	void SetFileName(const ALString& sValue);
	const ALString& GetFileName() const;

	// Utilisation d'une ligne d'entete: par defaut true
	// Non utilise directement: memorise pour ceux qui en ont besoin
	void SetHeaderLineUsed(boolean bValue);
	boolean GetHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: ';', compact pour les affichages)
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;

	// Nombre de lignes du fichier (en plus de la ligne d'entete): par defaut: 1000
	void SetLineNumber(int nValue);
	int GetLineNumber() const;

	// Nombre de champs par ligne: par defaut: 10
	void SetFieldNumber(int nValue);
	int GetFieldNumber() const;

	// Acces aux index des champs de la cle  (par defaut: un champ cle)
	IntVector* GetKeyFieldIndexes();
	const IntVector* GetConstKeyFieldIndexes() const;

	// Sens du tri (par defaut: true)
	void SetAscendingSort(boolean bValue);
	boolean GetAscendingSort() const;

	// Nombre max de ligne par cle (par defaut: 1)
	void SetMaxLineNumberPerKey(int nValue);
	int GetMaxLineNumberPerKey() const;

	// Taux d'echantillonnage aleatoire (par defaut: 1)
	void SetSamplingRate(double dValue);
	double GetSamplingRate() const;

	/////////////////////////////////////////////////////
	// Services

	// Duplication et copie
	KWArtificialDataset* Clone() const;
	void CopyFrom(const KWArtificialDataset* sourceDataset);

	// Construction d'un identifiant a partir des caracteristique du fichier
	const ALString BuildIdentifier() const;

	// Construction d'un nom complet de fichier a partir du repertorie temporaire et des caracteristiques du fichier
	const ALString BuildFileName() const;

	// Specification d'un jeu de donnee artificiel a trier (pas de creation du fichier)
	void SpecifySortDataset();

	// Specification d'un jeu de donnee artificiel trie, avec cle unique (pas de creation du fichier)
	void SpecifySortedDataset();

	// Creation du fichier
	void CreateDataset() const;

	// Export des noms des champs
	void ExportNativeFieldNames(StringVector* svNativeFieldNames) const;

	// Export des noms des attributs cles dans l'ordre des cles
	void ExportKeyAttributeNames(StringVector* svKeyAttributedNames) const;

	// Destruction du fichier
	void DeleteDataset() const;

	// Affichage des premieres lignes
	void DisplayFirstLines(int nDisplayLineNumber) const;

	// Affichage des premieres lignes d'un fichier quelconque
	static void DisplayFileFirstLines(const ALString& sFilePathName, int nDisplayLineNumber);

	// Ajoute la ligne sLine dans le fichier sFileName pour produire le fichier sDestFileName.
	// La ligne est inseree aux emplacements ivLineIndexes
	static boolean AddLinesInFile(const ALString& sFileName, const ALString& sDestFileName, const ALString& sLine,
				      IntVector* ivLineIndexes);

	// Methode de test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Specifications en entree de la tache
	ALString sFileName;
	boolean bHeaderLineUsed;
	char cFieldSeparator;
	int nLineNumber;
	int nFieldNumber;
	IntVector ivKeyFieldIndexes;
	boolean bAscendingSort;
	int nMaxLineNumberPerKey;
	double dSamplingRate;
};