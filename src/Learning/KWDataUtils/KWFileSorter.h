// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWFileSorter;
class KWDataTableChunk;

#include "SortedList.h"
#include "KWSortBuckets.h"
#include "RMResourceSystem.h"
#include "RMParallelResourceManager.h"
#include "PLFileConcatenater.h"

//////////////////////////////////////////////////////////////////////
// Tri d'une base fichier selon une liste d'attributs
class KWFileSorter : public Object
{
public:
	// Constructeur
	KWFileSorter();
	~KWFileSorter();

	///////////////////////////////////////////////////////////////
	// Specification de la cle et des attributs natifs

	// Parametrage des noms des attributs de la cle
	StringVector* GetKeyAttributeNames();

	// Parametrage des noms de tous les champs natifs
	StringVector* GetNativeFieldNames();

	/////////////////////////////////////////////////////////////////////
	// Parametrage de la table d'entree a trier

	// Nom du fichier en entree
	void SetInputFileName(const ALString& sValue);
	const ALString& GetInputFileName() const;

	// Utilisation d'une ligne d'entete en entree: par defaut true
	void SetInputHeaderLineUsed(boolean bValue);
	boolean GetInputHeaderLineUsed() const;

	// Separateur de champs utilise en entree (par defaut: '\t')
	void SetInputFieldSeparator(char cValue);
	char GetInputFieldSeparator() const;

	////////////////////////////////////////////////////////////////////
	// Specification du fichier en sortie, resultat du tri

	// Nom du fichier en sortie
	void SetOutputFileName(const ALString& sValue);
	const ALString& GetOutputFileName() const;

	// Utilisation d'une ligne d'entete en sortie: par defaut true
	void SetOutputHeaderLineUsed(boolean bValue);
	boolean GetOutputHeaderLineUsed() const;

	// Separateur de champs utilise en sortie (par defaut: '\t')
	void SetOutputFieldSeparator(char cValue);
	char GetOutputFieldSeparator() const;

	/////////////////////////////////////////////////////////////////////
	// Services

	// Tri de la table d'entree vers la table de sortie
	// Methode interruptible, retourne false si erreur ou interruption, true sinon
	boolean Sort(boolean bDisplayUserMessage);

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Methode de test
	static void Test();

	// Test avec un fichier artificiel en entree et en sortie
	// Le fichier specifie en entree est cree, trie selon les spec du fichier en sortie (avec suffix ".sort", puis
	// les deux fichiers sont detruits
	static boolean TestWithArtificialData(const KWArtificialDataset* inputArtificialDataset,
					      const KWArtificialDataset* outputArtificialDataset);

	/////////////////////////////////////////////////
	///// Implementation
	//
	// Notes sur la specification
	// Le tri de table est ici une fonctionnalite de bas niveau
	//  - tous les champs du fichiers sont pris en compte
	//    (on prend en compte tous les attributs natifs, Unused ou non)
	//  - pas d'attributs calcules
	//  - pas de selection ou d'echantillonnage des enregistrement
	// Ce choix est justifie par:
	//  - le tri de fichier est une fonctionnalite de data management,
	//    a priori exterieure a Khiops, mise a dispotion ici uniqmement
	//    pour une aide a l'utilisation du multi-tables
	//  - il sera possible ulterieurement de passer par une implementation
	//    externe a Khiops beaucoup plus efficace, ce qui serait plus difficile
	//    a envisager si l'on integre la gestion des attributs calcules
	//
	// Notes sur l'implementation
	// On passe ici par une reutilisation maximale des classes disponibles
	// pour la gestion des bases de donnees (KWDatabase*), pour minimiser
	// l'effort d'implementation, au detriment de performances attendues
	// au moyen d'une implemenattion adhoc
	// Pour une veritable implementation robuste, il faudra tenir compte
	// des points suivants:
	//  - bien gerer la memoire disponible
	//  - bufferiser les acces aux fichiers
	//  - quid de fichier, dont une seule ligne, voire un seul champ
	//    depasserait la memoire disponible ?
	//  - gerer des messages d'avancement
protected:
	// Estimation de la taille des chunks a trier, elle donne la taille minimale et la taille maximale.
	// La taille minimale est la taille au dessous de laquelle, il y auraity une trop grande fragmentation du disque
	// La taille maximale est la plus grande taille qui tient en memoire dans chaque esclave lors du tri
	// Si les ressources ne sont pas suffisantes, ces deux valeurs sont a 1
	void ComputeChunkSize(longint lFileSize, longint lLineNumber, longint lKeyPairSize, boolean bTrace,
			      int& nChunkSizeMin, int& nChunkSizeMax);

	// Coupure du bucket passe en parametre en plusieurs buckets dont la taille doit etre comprise entre
	// nChunkSizeMin et nChunkSizeMax Les buckets resultants sont retournes dans un KWSortBuckets
	// bIsInterruptedByUser vaut true si les taches ont ete interrompues par l'utilisateur,
	// auquel cas la methode renvoie NULL
	KWSortBuckets* SplitDatabase(int nChunkSizeMin, int nChunkSizeMax, KWSortBucket* bucket,
				     boolean bIsHeaderLineUsed, char cFieldSeparator, longint lMeanKeySize,
				     longint lLineNumber, boolean bSilentMode, longint& lEncodingErrorNumber,
				     boolean& bIsInterruptedByUser);

	// Renvoie true si il est plus efficace de trier le fichier en memoire et en sequentiel
	// C'est le cas si tout tient en memoire et que la taille du fichier ne depasse pas 100 Mo
	boolean IsInMemorySort(longint lKeySize, longint lLineNumber, longint lFileSize);

	// Fichier en entree
	ALString sInputFileName;
	boolean bInputHeaderLineUsed;
	char cInputFieldSeparator;

	// Fichier en sortie
	ALString sOutputFileName;
	boolean bOutputHeaderLineUsed;
	char cOutputFieldSeparator;

	// Nom des champs de cle et natifs, memorises au moyen KWKeyFieldsIndexer
	KWKeyFieldsIndexer keyFieldsIndexer;

	// Taille maximale d'un chunk pour qu'il tienne en memoire
	// C'est la taille des CharVector : 2 Go - 1
	static const longint lChunkSizeLimit;

	// Pour la taille de chunk qui est calculee (censee etre optimale), on essaye de construire des chunks
	// plus petits. Car avec l'alogo de DeWitt on n'est pas certain d'avoir des chunks de la bonen taille et
	// en cas de depassement, il faut redecouper les chunks ce qui est couteux, on prefere donc avoir des chunks
	// plus petits, sans appeler SplitDatabase plusieurs fois.
	const double dDeWittRatio = 0.8;
};
