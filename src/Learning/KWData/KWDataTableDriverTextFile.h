// Copyright (c) 2024 Orange. All rights reserved.
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
	longint ComputeNecessaryDiskSpaceForFullWrite(const KWClass* kwcLogicalClass, longint lInputFileSize) override;
	double GetReadPercentage() override;
	longint GetUsedMemory() const override;

	// Variante de l'estimation du nombre d'objets dans la base, en memoire et sans acces disque,
	// en analysant la structure du dictionnaire avec dimensionnement heuristique
	longint GetInMemoryEstimatedObjectNumber(longint lInputFileSize);

	// Estimation heuristique de la place disque par record d'un fichier a lire en se basant sur les variable native
	// du dictionnaire
	longint GetEstimatedUsedInputDiskSpacePerObject() const;

	// Estimation heuristique de la memoire utilise par KWObject en se basant sur les variables utilisee du
	// dictionnaire, natives ou calculees
	longint GetEstimatedUsedMemoryPerObject() const;

	// Estimation heuristique de la place disque par record d'un fichier a ecrire en se basant sur les variables
	// utilisees du dictionnaire logique
	longint GetEstimatedUsedOutputDiskSpacePerObject(const KWClass* kwcLogicalClass) const;

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
	void SetBufferSize(int nSize);
	int GetBufferSize() const;

	// Taille par defaut des buffers
	static int GetDefaultBufferSize();

	// Calcul de la taille memoire necessaire pour la gestion des buffer en tenant compte d'une taille de fichier en
	// lecture
	static int ComputeBufferNecessaryMemory(boolean bRead, int nBufferSize, longint lFileSize);

	// Mode verbeux pour la detection des champs trop long (defaut: true)
	static void SetOverlengthyFieldsVerboseMode(boolean bValue);
	static boolean GetOverlengthyFieldsVerboseMode();

	// Redefinition du parametrage du mode silencieux, pour le synchroniser avec celui du buffer
	void SetSilentMode(boolean bValue) override;

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
	// des lignes entieres ou jusqu'a la position de fin max en parametre
	// Dans le cas d'une ligne trop longue en fin de buffer, on emet un warning, et on
	// continue a lire jusqu'a ce que le buffer soit vide ou contienne le debut de la ligne suivante
	virtual boolean FillInputBufferWithFullLines(longint lBeginPos, longint lMaxEndPos);

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
	int nBufferSize;

	// Mode verbeux pour la detection des champs trop long
	static boolean bOverlengthyFieldsVerboseMode;

	// Constantes pour l'estimation heuristique conservatrice de la taille des champs sur fichier
	static const int nMinRecordSize = 5;
	static const int nDenseValueSize = 2;
	static const int nSparseValueSize = 7;
	static const int nTextValueSize = 200;
	static const int nKeyFieldSize = 5;
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
		return FillInputBufferWithFullLines(inputBuffer->GetPositionInFile(), inputBuffer->GetFileSize());
	else
		return true;
}

inline int KWDataTableDriverTextFile::GetDefaultBufferSize()
{
	return InputBufferedFile::nDefaultBufferSize;
}
