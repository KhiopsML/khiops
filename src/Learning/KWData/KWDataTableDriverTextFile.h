// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDataTableDriver.h"
#include "FileService.h"
#include "KWObject.h"
#include "InputBufferedFile.h"
#include "OutputBufferedFile.h"
#include "KWLoadIndex.h"
#include "PLRemoteFileService.h"

/////////////////////////////////////////////////////
// Table de donnees au format fichier texte
//
// Un fichier de donnees texte possede la structure
// suivante:
//		- une ligne d'entete (facultative) contenant les libelles des champs
//		- une ligne par instance
//		- un caractere (',', ' ', TAB...) comme separateur de champs
//
// Si la classe existe de facon prealable, les champs inconnus
// seront ignores, ainsi que les champs calcules (qui sont
// recalcules)
// Si la ligne d'en-tete n'est pas utilise en lecture, seuls
// les premiers champs de la classe (correspondant aux champs
// presents dans le fichier) seront initialises.
class KWDataTableDriverTextFile : public KWDataTableDriver
{
public:
	// Constructeur
	KWDataTableDriverTextFile();
	~KWDataTableDriverTextFile();

	// Creation dynamique
	KWDataTableDriver* Create() const override;

	// Recopie des attributs de definition
	void CopyFrom(const KWDataTableDriver* kwdtdSource) override;

	// Comparaison des attributs de definition avec une autre table du meme type
	int Compare(const KWDataTableDriver* kwdtdSource) const override;

	///////////////////////////////////////////////////////////
	// Parametrage de la structure du fichier

	// Utilisation d'une ligne d'entete: par defaut true
	void SetHeaderLineUsed(boolean bValue);
	boolean GetHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;

	// Reimplementation des methodes virtuelles de KWDataTableDriver
	boolean BuildDataTableClass(KWClass* kwcDataTableClass) override;
	boolean OpenForRead(const KWClass* kwcLogicalClass) override;
	boolean OpenForWrite() override;
	boolean IsOpenedForRead() const override;
	boolean IsOpenedForWrite() const override;
	boolean IsEnd() const override;
	KWObject* Read() override;
	boolean IsError() const override;
	void Skip() override;
	void Write(const KWObject* kwoObject) override;
	boolean Close() override;
	boolean IsClosed() const override;
	void DeleteDataTable() override;
	longint GetEstimatedObjectNumber() override;
	longint ComputeOpenNecessaryMemory(boolean bRead) override;
	longint ComputeNecessaryMemoryForFullExternalRead(const KWClass* kwcLogicalClass) override;
	longint ComputeNecessaryDiskSpaceForFullWrite(const KWClass* kwcLogicalClass) override;
	double GetReadPercentage() override;
	longint GetUsedMemory() const override;

	// Lecture des champs de la ligne d'entete
	virtual boolean ReadHeaderLineFields(StringVector* svFirstLineFields);

	// Ecriture de la ligne d'entete
	virtual void WriteHeaderLine();

	// Verification du format, notament le separateur de champ
	boolean CheckFormat() const override;

	///////////////////////////////////////////////////////////
	// Methodes avancees

	// Taille du buffer lors de la prochaine ouverture
	// Initialisement a la valeur par defaut
	// N'a aucun effet dans la classe fille PLDataTableDriverTextFileParallel
	void SetBufferSize(int nSize);
	int GetBufferSize() const;

	// Taille par defaut des buffers
	static int GetDefaultBufferSize();

	// Calcul de la taille memoire necessaire pour la gestion des buffer en tenant compte d'une taille de fichier en
	// lecture
	static int ComputeBufferNecessaryMemory(boolean bRead, int nBufferSize, longint lFileSize);

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Implementation specifique du saut de ligne dans le cas d'une classe racine
	// En effet, dans ce cas, on analyse partiellement la ligne pour en extraire la derniere cle
	void SkipRootRecord();

	// Remplissage du buffer si necessaire (fin de buffer et pas fin de fichier)
	virtual boolean UpdateInputBuffer();

	// Remplissage du buffer a partir de la position courante dans le fichier jusqu'a
	// remplir completement le buffer avec des lignes entieres.
	// En cas de lignes trop longues, le remplissage du buffer continue jusqu'a obtenir
	// des lignes entieres ou jusqu'a la fin du fichier
	// Si une ligne ne tient pas dans la taille max elle n'est pas prise en compte, le
	// buffer commence au debut de la suivante
	virtual boolean FillInputBufferWithFullLines();

	// Verification du buffer pour tester s'il y a des caracteres null
	// Si echec, renvoie, false, emet des messages d'erreur, et met le buffer a NULL
	virtual boolean CheckInputBuffer();

	// Calcul des indexes des data items (attributs ou blocs d'attributs) de la classe logique associee
	// a chaque champ du fichier en comparant la classe logique comportant les champs necessaires
	// et une classe representant le header du fichier a analyser (optionnelle si pas de ligne d'entete)
	// On calcule egalement les index des attributs de la cle dans le cas d'un classe racine
	virtual boolean ComputeDataItemLoadIndexes(const KWClass* kwcLogicalClass, const KWClass* kwcHeaderLineClass);

	// Ouverture du fichier en lecture ou ecriture: retourne true si OK
	virtual boolean OpenInputDatabaseFile();
	virtual boolean OpenOutputDatabaseFile();

	// Fermeture du fichier
	virtual boolean CloseDatabaseFile();

	// Reinitialisation du fichier sur la base de donnee, a utiliser avant
	// chaque ouverture du fichier, de facon a forcer une initialisation
	// coherente des flags internes de gestion du fichier
	virtual void ResetDatabaseFile();

	// Memorisation de l'etat de suivi des taches
	PeriodicTest periodicTestInterruption;

	// Utilisation d'une ligne d'en-tete
	boolean bHeaderLineUsed;

	// Separateur de champ
	char cFieldSeparator;

	// Index des dataItems, attributs ou blocs, de la classe physique pour chaque champ du fichier
	// Index invalide si champ du fichier inutilise ou inexistant dans la classe
	KWLoadIndexVector livDataItemLoadIndexes;

	// Index des champs de la cle dans le cas d'une classe racine
	// A chaque index de champ de fichier, on associe soit -1 si le champ ne fait pas partie de la cle,
	// soit l'index du champs de la cle
	// On a en effet besoin de memoriser les champs de la cle dans ce cas, que ce soit lors des
	// lecture par Read (que l'enregistrement soit errone ou non) ou lors des sauts de ligne
	// C'est necessaire pour faire le controle des enregistrements dupliques, et de ne garder
	// que le premier (si valide), et ignorant tous les suivants consideres comme dupliques
	IntVector ivRootKeyIndexes;

	// Fichier utilise pour la gestion de la base
	InputBufferedFile* inputBuffer;
	OutputBufferedFile* outputBuffer;
	boolean bWriteMode;

	// Taille des buffers (taille allouee a la prochaine ouverture)
	int nBufferedFileSize;

	// Taille par defaut des buffers
	static const int nDefaultBufferSize = InputBufferedFile::nDefaultBufferSize;
};

////////////////////////////////////////
// Methode en inline

inline void KWDataTableDriverTextFile::SetHeaderLineUsed(boolean bValue)
{
	bHeaderLineUsed = bValue;
}

inline boolean KWDataTableDriverTextFile::GetHeaderLineUsed() const
{
	return bHeaderLineUsed;
}

inline void KWDataTableDriverTextFile::SetFieldSeparator(char cValue)
{
	cFieldSeparator = cValue;
}

inline char KWDataTableDriverTextFile::GetFieldSeparator() const
{
	return cFieldSeparator;
}

inline boolean KWDataTableDriverTextFile::IsError() const
{
	assert(inputBuffer == NULL or outputBuffer == NULL);
	if (inputBuffer != NULL)
		return inputBuffer->IsError();
	else if (outputBuffer != NULL)
		return outputBuffer->IsError();
	else
		return false;
}

inline boolean KWDataTableDriverTextFile::IsEnd() const
{
	return inputBuffer->IsFileEnd();
}

inline boolean KWDataTableDriverTextFile::UpdateInputBuffer()
{
	if (inputBuffer->IsBufferEnd() and not inputBuffer->IsFileEnd())
		return FillInputBufferWithFullLines();
	else
		return true;
}

inline int KWDataTableDriverTextFile::GetDefaultBufferSize()
{
	return nDefaultBufferSize;
}
