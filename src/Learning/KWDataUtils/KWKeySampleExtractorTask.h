// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWSortedKeySample;

#include "PLParallelTask.h"
#include "KWKeyExtractor.h"
#include "SortedList.h"
#include "KWKeySizeEvaluatorTask.h"
#include "KWArtificialDataset.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWKeySampleExtractorTask
// Extraction d'une liste ordonnee de K cles a partir d'un fichier a trier,
// permettant d'assurer que ces cles representent des points de coupure du fichier
// en K+1 parties de tailles a peu pres egales.
//
//	En entree : principalement le fichier, l'index des champs de la cle a extraire,
//              et le nombre de cles a extraire
//	En sortie : un tableau ordonne de cles
//
// L'algo se base sur l'extraction d'un echantillon aleatoire de grande taille de cles,
// en les triant, puis en gardant K-1 cles coupant l'echantillon en parties de meme taille.
class KWKeySampleExtractorTask : public PLParallelTask
{
public:
	// Constructeur
	KWKeySampleExtractorTask();
	~KWKeySampleExtractorTask();

	/////////////////////////////////////////////////////
	// Specification du fichier d'entree

	// Fichier d'entree
	void SetFileURI(const ALString& sFileURI);
	const ALString& GetFileURI() const;

	// Utilisation d'une ligne d'entete: par defaut true
	void SetHeaderLineUsed(boolean bValue);
	boolean GetHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;

	/////////////////////////////////////////////////////
	// Parametres principaux de l'extraction

	// Nombre de cles Min et Max a extraire
	// elles correspondent a la taille min et max des chunks
	void SetSplitKeyNumber(int nMinValue, int nMaxValue);
	int GetSplitKeyNumberMin() const;
	int GetSplitKeyNumberMax() const;

	// Acces aux index des clefs
	IntVector* GetKeyFieldIndexes();
	const IntVector* GetConstKeyFieldIndexes() const;

	/////////////////////////////////////////////////////
	// Parametres avances

	// Nombre d'enregistrements qu'on va garder dans l'echantillon
	// Plus cet echantillon est important, plus grande est la fiabilite d'une coupure des cles
	// en partie equidistribuee
	// Attention, dans le cas ou il n'y a pas assez de memoire pour avoir un echantillon  de cette taille,
	// la taille est modifiee  automatiquement pendant l'execution et l'echantillon peut etre plus petit
	void SetSampleSize(int nLines);
	int GetSampleSize() const;

	// Nombre de lignes dans le fichier d'entree (estimation)
	void SetFileLineNumber(longint lLines);
	longint GetFileLineNumber() const;

	// Estimation de la taille en octet necessaire pour stocker une cle
	void SetKeyUsedMemory(longint lValue);
	longint GetKeyUsedMemory() const;

	/////////////////////////////////////////////////////
	// Methode principale

	// Extraction de l'ensemble des cles (KWKey)
	// Le resultat est stocke dans le tableau passe en parametre et appartient a l'appelant
	// Le nombre de cles extraites est au plus le nombre de cle demandees
	boolean ExtractSample(ObjectArray* oaKeys);

	// Methode de test
	static void Test();

	// Test avec un jeu de donnees artificiel deja cree
	// Alimente en sortie le tableau de cles
	static boolean TestWithArtificialDataset(const KWArtificialDataset* artificialDataset, longint lMeanKeySize,
						 int nSplitKeyNumberMin, int nSplitKeyNumberMax, int nSampleSize,
						 longint lFileLineNumber, ObjectArray* oaKeys);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Algorithme
	// Le maitre parcourt le fichier d'entree et chaque esclave traite sa portion du fichier
	// en extrayant un echantillon de cles, en le triant, et en le renvoyant au maitre qui
	// se contente de les archiver dans un tableau de KWSortedKeySample dans un premier temps.
	// Pour gerer finement la memoire disponible, le maitre peut etre amener a diminuer le taux
	// d'echantillonnage des cles, ce qui l'oblige alors a reechantilonner les cles qu'il a deja stockees.
	// Lors de la finalisation, le maitre tri l'ensemble de ses KWSortedKeySample au moyen d'une SortedList
	// pour produire un echantillon global de cle.
	// Il ne lui reste qu'a extraire des cles a des positions equi-reparties pour produire
	// le tableau des cles de coupures en sortie

	// Reimplementation des methodes virtuelles de tache
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Methodes du maitre

	// TODO BG renommer en ComputeNecessaryMemory
	// Renvoie la memoire globale necessaire (lecture + traitement) quand le buffer de lecture fait nBufferSize
	longint ComputeGlobalMemory(int nBufferSize) const;

	// Renvoie la taille du buffer de lecture quand la memoire allouee est de lGlobalMemory
	int ComputeBufferSize(longint lGlobalMemory) const;

	////////////////////////////////////////////////////
	// Variables du Master

	// Specifications en entree de la tache
	ALString sFileURI;
	boolean bHeaderLineUsed;
	char cFieldSeparator;
	int nSplitKeyNumberMin;
	int nSplitKeyNumberMax;
	double dSamplingRate;
	longint lInputFileSize;
	longint lFileLineNumber;
	int nSampleSize;
	longint lFilePos;

	// Borne max pour la taille de l'echantillon de cle
	// L'echantillon ne depassera pas cette taille
	// (mecanisme d'ajustement du taux et resampling)
	longint lMaxUsedMemory;
	longint lKeyUsedMemory;

	// Variables de travail du Master
	longint lCurrentUsedMemory;
	double dCummulatedBufferSize;

	// Table d'echantillons de cle tries (classe KWSortedKeySample)
	// Memorisation des resultats d'analyse des esclaves
	ObjectArray oaAllSortedKeySamples;

	// Resultats : tableau de clefs de coupure (classe KWKey)
	ObjectArray oaSplitKeys;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Index des champs de la cle
	PLShared_IntVector shared_ivKeyFieldIndexes;

	// Attributs du fichier d'entree
	PLShared_String shared_sFileName;
	PLShared_Boolean shared_bHeaderLineUsed;
	PLShared_Char shared_cFieldSeparator;

	// Taille du buffer de lecture
	PLShared_Int shared_nBufferSize;

	///////////////////////////////////////////////////////////
	// Parametres en entree et sortie des esclaves

	// Taux d'echantillonnage en entree
	PLShared_Double input_dSamplingRate;

	// Echantillon de cle retourne par l'esclave
	PLShared_ObjectArray* output_oaSampledKeys;

	// Taille totale des cle de l'echantillon
	PLShared_Longint output_lSampledKeysUsedMemory;

	// Taux d'echantillonnage en sortie
	PLShared_Double output_dSamplingRate;

	// Position dans le fichier
	PLShared_Longint input_lFilePos;

	// Taille des buffers en sortie
	PLShared_Double output_dBufferSize;

	////////////////////////////////////////////////////
	// Variables de l'esclave

	// Extracteur de cle
	KWKeyExtractor keyExtractor;

	// Fichier de travail pour l'esclave
	InputBufferedFile inputFile;
};

/////////////////////////////////////////////////////////////////////////////////
// Classe KWSortedKeySample
// Classe utilise pour le mergeSort des clefs issues des esclaves
// C'est une liste de clefs
//
// Le mergeSort est realise en utilisant une SortedList de KWSortedKeySample qui triera
// les KWSortedKeySamplesuivant leurs premiers elements.
// le Comparateur du KWSortedKeySample est base sur la premiere clef de la liste
class KWSortedKeySample : public Object
{
public:
	// Constructeur
	// Le destructeur detruits les cles
	KWSortedKeySample();
	~KWSortedKeySample();

	// Renvoie et retire la premiere clef
	KWKey* PopKey();

	// Renvoie la premiere cle sans la retirer de la liste
	KWKey* GetFirstKey();

	// Ajoute une clef a la fin
	void AddKey(KWKey* key);

	// Nombre de cles
	int GetCount();

	// Test si vide
	boolean IsEmpty() const;

	// Import des cles a partir d'un tableau de cle
	// Apres l'import, le tableau en parametre est mis a vide
	void ImportKeys(ObjectArray* oaSourceKeys);

	// Echantillonne la liste et on renvoie la memoire utilisee par les cles detruites
	longint Sample(double dRatio, int nTaskIndex);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// On passe par un ObjectArray plutot que un ObjectList, pour des raison d'efficacite memoire
	ObjectArray oaKeys;
	int nHeadKeyIndex;
	friend int KWSortedKeySampleCompareHead(const void* elem1, const void* elem2);
};

inline KWKey* KWSortedKeySample::PopKey()
{
	KWKey* key;
	require(nHeadKeyIndex < oaKeys.GetSize());
	key = cast(KWKey*, oaKeys.GetAt(nHeadKeyIndex));
	oaKeys.SetAt(nHeadKeyIndex, NULL);
	nHeadKeyIndex++;
	return key;
}

inline KWKey* KWSortedKeySample::GetFirstKey()
{
	KWKey* key;
	require(nHeadKeyIndex < oaKeys.GetSize());
	key = cast(KWKey*, oaKeys.GetAt(nHeadKeyIndex));
	return key;
}

inline int KWSortedKeySample::GetCount()
{
	return oaKeys.GetSize() - nHeadKeyIndex;
}

inline void KWSortedKeySample::AddKey(KWKey* key)
{
	require(key != NULL);
	oaKeys.Add(key);
}

inline boolean KWSortedKeySample::IsEmpty() const
{
	return oaKeys.GetSize() == nHeadKeyIndex;
}

int KWSortedKeySampleCompareHead(const void* elem1, const void* elem2);
