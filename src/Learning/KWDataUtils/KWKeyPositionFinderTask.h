// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWSortedKeySample;

#include "PLParallelTask.h"
#include "KWKeyExtractor.h"
#include "KWKeySizeEvaluatorTask.h"
#include "KWKeyPositionSampleExtractorTask.h"
#include "KWArtificialDataset.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWKeyPositionFinderTask
// Extraction d'une liste ordonnee des positions de cles en entree, a partir d'un fichier deja trie
//
//	En entree : principalement le fichier, l'index des champs de la cle a extraire,
//              et le tableau ordonne des cles dont il faut extraire la position
//	En sortie : un tableau ordonne des cles enrichi avec leur position
class KWKeyPositionFinderTask : public PLParallelTask
{
public:
	// Constructeur
	KWKeyPositionFinderTask();
	~KWKeyPositionFinderTask();

	/////////////////////////////////////////////////////
	// Specification du fichier d'entree

	// Fichier d'entree
	void SetFileName(const ALString& sFileName);
	const ALString& GetFileName() const;

	// Utilisation d'une ligne d'entete: par defaut true
	void SetHeaderLineUsed(boolean bValue);
	boolean GetHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;

	/////////////////////////////////////////////////////
	// Parametres principaux de l'extraction

	// Acces aux index des clefs
	IntVector* GetKeyFieldIndexes();
	const IntVector* GetConstKeyFieldIndexes() const;

	/////////////////////////////////////////////////////
	// Methode principale

	// Extraction de l'ensemble des positions des cles (KWKeyPosition) correspondant aux cles en entree (KWKey)
	// Le resultat est stocke dans le tableau passe en parametre et appartient a l'appelant
	// Le nombre de position extraites correspond au nombre de cles en entree
	// Attention, les cles des KWKeyPosition du tableau resultat sont les meme que les cle en entree
	// (on peut eventuellement les nettoyer pour economiser de la memoire (utiliser KWKeyPosition::CleanKeys pour
	// les reinitialiser)) En sortie:
	//  . tableau vide si erreur (par exemple: fichier non trie)
	//  . si cle en entree trouvee en sortie: plus petite position de cle strictement superieure
	//  . si cle en entree non trouvee en sortie: plus petite position pour la premiere cle en sortie plus grande si
	//  existante,
	//                                            sinon en fin de fichier (nombre de lignes et taille du fichier)
	//  . si doublons dans les cle en entree: chaque position en sortie est la meme
	boolean FindKeyPositions(const ObjectArray* oaInputKeys, ObjectArray* oaFoundKeyPositions);

	///////////////////////////////////////////////////////////////
	// Services divers

	// Verification de l'integrite d'un tableau de cles
	boolean CheckKeys(const ObjectArray* oaKeys) const;

	// Verification de l'integrite d'un tableau de positions
	boolean CheckKeyPositions(const ObjectArray* oaKeyPositions) const;

	// Verification de la coherence d'un tableau de positions avec un tableau de cles
	boolean CheckKeyPositionsConsistency(const ObjectArray* oaKeys, const ObjectArray* oaKeyPositions) const;

	// Affichage d'un tableau de cles
	void WriteKeys(const ObjectArray* oaKeys, ostream& ost) const;

	// Affichage d'un tableau de positions de cles
	void WriteKeyPositions(const ObjectArray* oaKeyPositions, ostream& ost) const;

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;

	// Methode de test
	static void Test();

	// Test en specifiant les caracteristiques d'une table principale et d'une table secondaire
	// au moyen de jeux de donnees artificiels
	// La taille de buffer n'est prise en compte que si elle est differente de 0.
	static boolean TestWithArtificialMainAndSecondaryTables(int nMainLineNumber, int nMainLineNumberPerKey,
								double dMainSamplingRate, int nSecondaryLineNumber,
								int nSecondaryLineNumberPerKey,
								double dSecondarySamplingRate, int nBufferSize);

	// Test avec un jeu de donnees artificiel deja cree
	// La taille de buffer n'est prise en compte que si elle est differente de 0.
	// Alimente en sortie le tableau de cles
	static boolean TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
						 const ObjectArray* oaInputKeys, ObjectArray* oaFoundKeyPositions,
						 int nBufferSize);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Algorithme
	// Le maitre parcourt le fichier d'entree et chaque esclave traite sa portion du fichier
	// en extrayant un echantillon de position de cles secondaires et en le renvoyant au maitre qui se contente
	// de les archiver  dans un tableau de resultats indexe par le rang des esclave.
	// Chaque esclave recherche la premiere cle en entree correspondante par dichotomie poour se caller
	// au debut de la plage de cles le corresponbdant, puis la synchronisation se fait de facon lineaire.
	// A la fin, les resultats de chaque esclave sont concatenes dans l'ordre initial (fichier deja trie)
	// pour produire le tableau de positions de cle final. Ce tableau peut contenir moins de cles que de cles
	// en entree, et il faut alors combler les trous, entre deux resultats de deux esclaves consecutifs, ou
	// avant le resultat du premier esclave ou apres celui du dernier esclave.
	// Chaque esclave ne connait les numero de ligne que localement a sa tache. Le maitre collecte donc les
	// nombre de lignes traitees par tache (dans lvLineCountPerTaskIndex), et au moment de sa finalisation,
	// il peut ainsi recalculer le numero de ligne global associe a chaque cle.

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

	////////////////////////////////////////////////////
	// Variables du Master

	// Specifications en entree de la tache
	ALString sFileName;
	boolean bHeaderLineUsed;
	char cFieldSeparator;

	// Memoire utilisee pour le stckage des cle
	longint lInputKeysUsedMemory;
	longint lOutputKeysUsedMemory;

	// Definition des exigences pour la taille du buffer
	int nReadSizeMin;
	int nReadSizeMax;
	int nReadBufferSize;

	// Table d'echantillons de cle (ObjectArray de KWKeyPosition)
	// Memorisation des resultats d'analyse des esclaves
	ObjectArray oaAllKeyPositionSubsets;

	// Tableau des premiere et derniere cles (KWKeyPosition) par esclave,
	// permettant d'effectuer des diagnostics intelligibles en cas de probleme de
	// tri du fichier, intervenant a la frontiere entre deux esclaves
	ObjectArray oaAllSlaveFirstKeyPositions;
	ObjectArray oaAllSlaveLastKeyPositions;

	// Resultats : tableau de clefs en sortie (classe KWKeyPosition)
	ObjectArray oaResultKeyPositions;

	// Taille des buffers pour effectuer des tests
	// Cette valeur n'est prise en compte que si elle est non nulle
	int nForcedBufferSize;

	// Position dans le fichier en lecture
	longint lFilePos;
	longint lInputFileSize;

	// Memorisation du nombre de lignes lues pour chaque SlaveProcess
	LongintVector lvLineCountPerTaskIndex;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Index des champs de la cle
	PLShared_IntVector shared_ivKeyFieldIndexes;

	// Tableau de cle en entree
	PLShared_ObjectArray* shared_oaInputKeys;

	// Non du fichier d'entree (parametrage des messages d'erreur)
	PLShared_String shared_sInputFileName;
	PLShared_Boolean shared_bHeaderLineUsed;
	PLShared_Char shared_cFieldSeparator;

	///////////////////////////////////////////////////////////
	// Parametres en entree et sortie des esclaves

	// Taille du buffer en entree
	PLShared_Int input_nBufferSize;

	// Position dans le fichier
	PLShared_Longint input_lFilePos;

	// Echantillon de cle retourne par l'esclave
	PLShared_ObjectArray* output_oaKeyPositionSubset;

	// Premiere et derniere cle trouvee par l'esclave
	PLShared_KeyPosition output_SlaveFirstKeyPosition;
	PLShared_KeyPosition output_SlaveLastKeyPosition;

	// Nombre de lignes lues lors de chaque traitement
	PLShared_Longint output_lLineNumber;

	////////////////////////////////////////////////////
	// Variables de l'esclave

	// Extracteur de cle
	KWKeyExtractor keyExtractor;

	// Fichier de travail pour l'esclave
	InputBufferedFile inputFile;
};
