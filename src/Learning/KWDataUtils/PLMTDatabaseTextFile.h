// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class PLMTDatabaseTextFile;
class PLShared_MTDatabaseTextFile;

#include "KWMTDatabaseTextFile.h"
#include "PLDataTableDriverTextFile.h"
#include "PLParallelTask.h"
#include "PLSharedObject.h"
#include "PLSharedVector.h"

///////////////////////////////////////////////////////////////////
// Classe PLMTDatabaseTextFile
// Classe "technique", dediee aux applications paralleles de la classe KWSTDatabaseTextFile
//  avec gestion des informations d'ouverture pour le dimensionnement des taches,
//  l'analyse de header de tables, et memorisation pour envoi aux esclaves via une variable partagee
// Implementation de methodes generiques definies dans PLDatabaseTextFile
// 	 Le driver associe est PLDataTableDriverTextFile
//   Les buffers sont geres a l'exterieur de la classe (d'ou SetOutputBuffer et  SetInputBuffer)
//   Cette classe n'ouvre et ne ferme pas les buffers
class PLMTDatabaseTextFile : public KWMTDatabaseTextFile
{
public:
	// Conctructeur
	PLMTDatabaseTextFile();
	~PLMTDatabaseTextFile();

	// Remise dans l'etat non initialise
	void Reset();

	//////////////////////////////////////////////////////////////////////////////////////
	// Calculs prealables a l'utilisation de la base, avant son utilisation en
	// lecture ou en ecriture

	// Calcul des informations necessaires a l'ouverture de la base en lecture ou ecriture
	// Cette methode est a appeler dans le maitre, essentiellement pour permettre un
	// dimensionnment fin des ressources
	// Le parametre outputDatabaseTextFile est optionnel, et n'est utilise que pour l'estimation
	// de la place disque pour une base en sortie
	//   . taille des fichiers en lecture
	//   . memoire minimum necessaire pour l'ouverture de la base
	//   . indexation des champs de la table en lecture
	boolean ComputeOpenInformation(boolean bRead, boolean bIncludingClassMemory,
				       PLMTDatabaseTextFile* outputDatabaseTextFile);
	boolean IsOpenInformationComputed() const;

	// Nettoyage des informations d'ouverture
	void CleanOpenInformation();

	// Acces aux mappings effectivement utilises, avec un index de table parmi toute les tables possibles
	// Les mappings renvoyes sont a NULL s'il ne sont pas utilises
	KWMTDatabaseMapping* GetUsedMappingAt(int nTableIndex) const;

	// Nombre de mapping effectivement utilises, c'est a dire non NULL
	int ComputeUsedMappingNumber() const;

	// Acces aux classes de gestion des ligne d'entete par table
	// Renvoie NULL pour les entete pour des tables non encore utiliseees
	const KWClass* GetUsedMappingHeaderLineClassAt(int nIndex) const;

	// Acces aux tailles des fichiers
	// Les tailles des fichiers pour les tables non encore utilisees sont a 0
	const LongintVector* GetFileSizes() const;

	// Taille cumulee de tous les fichiers en lecture (0 sinon)
	longint GetTotalFileSize() const;

	// Taille cumulee des fichiers pour les mapping effectivement utilises en lecture
	longint GetTotalUsedFileSize() const;

	// Taille de buffer preferee pour l'ensemble de la base
	int GetDatabasePreferredBuferSize() const;

	// Nombre d'objets par fichier, estime de facon heuristique
	// Renvoie 0 pour les fichiers des tables non encore utilisees
	const LongintVector* GetInMemoryEstimatedFileObjectNumbers() const;

	// Memoire utilisee par KWObject physique pour chaque fichier
	// Renvoie 0 pour les fichiers des tables non concernees
	const LongintVector* GetEstimatedUsedMemoryPerObject() const;

	// Nombre de tables principales, hors tables externes, avec des fichiers locaux (sans URI)
	int GetMainLocalTableNumber() const;

	// Memoire minimum necessaire pour ouvrir la base sans tenir compte des buffers
	longint GetEmptyOpenNecessaryMemory() const;

	// Memoire minimum et maximum necessaire pour ouvrir la base, en tenant compte des buffers
	// et de la la memoire dediee au DatabaseMemoryGuard pour gerer les instances "elephants"
	longint GetMinOpenNecessaryMemory() const;
	longint GetMaxOpenNecessaryMemory() const;

	// Memoire necessaire pour stocker le fichier d'une base en sortie a partir d'une base en lecture (0 sinon)
	longint GetOutputNecessaryDiskSpace() const;

	// Nombre max de records secondaires pour la parametrage du DatabaseMemoryGuard
	// On renvoie 0 s'il n'est pas necessaire d'activer le DatabaseMemoryGuard, meme si
	// on reserve de la memoire pour sa gestion
	longint GetEstimatedMaxSecondaryRecordNumber() const;

	// Memoire minimum et maximum necessaire pour le parametrage du DatabaseMemoryGuard
	longint GetEstimatedMinSingleInstanceMemoryLimit() const;
	longint GetEstimatedMaxSingleInstanceMemoryLimit() const;

	// Nombre maximum de taches elementaires qui devront etre traitees par les esclaves
	int GetMaxSlaveProcessNumber() const;

	// Calcul de la memoire par buffer pour une memoire allouee pour l'ouverture et un nombre de process
	int ComputeOpenBufferSize(boolean bRead, longint lOpenGrantedMemory, int nProcessNumber) const;

	// Calcul de la memoire a reserver pour le DatabaseMemoryGuard
	longint ComputeEstimatedSingleInstanceMemoryLimit(longint lOpenGrantedMemory) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Parametrage des buffers

	// Ouverture a la demande (defaut: false)
	void SetOpenOnDemandMode(boolean bValue);
	boolean GetOpenOnDemandMode() const;

	// Taille du buffer du driver : a manipuler avec precaution
	// Taille du buffer lors de la prochaine ouverture
	void SetBufferSize(int nSize);
	int GetBufferSize() const;

	// Parametrage des buffers d'entree par mapping
	void SetInputBufferedFileAt(KWMTDatabaseMapping* mapping, InputBufferedFile* buffer);
	InputBufferedFile* GetInputBufferedFileAt(KWMTDatabaseMapping* mapping);

	// Parametrage des buffers de sortie par mapping
	void SetOutputBufferedFileAt(KWMTDatabaseMapping* mapping, OutputBufferedFile* buffer);
	OutputBufferedFile* GetOutputBufferedFileAt(KWMTDatabaseMapping* mapping);

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des buffer en lecture ou en ecriture

	// Creation, ouverture, fermeture et destruction des buffers de fichier pour la base d'entree
	// Les noms de fichier sont unique tout le long de la tache
	boolean CreateInputBuffers();
	boolean OpenInputBuffers();
	boolean CloseInputBuffers();
	boolean DeleteInputBuffers();

	// Creation, ouverture, fermeture et destruction des buffers de fichier pour la base de sortie
	// Les noms de fichier cree par OpenOutputBuffers sont locaux a chaque SlaveProcess
	// Le vecteur de noms de fichier  en sortie est de meme taille que les tables de la base
	// (non vide uniquement pour les fichier crees)
	boolean CreateOutputBuffers();
	boolean OpenOutputBuffers(const PLParallelTask* task, int nTaskIndex, StringVector* svOutputBufferFileNames);
	boolean CloseOutputBuffers();
	boolean DeleteOutputBuffers();

	//////////////////////////////////////////////////////////////////////////////////////
	// Methodes techniques

	// Test si un mapping est initialise (avec un driver de table)
	boolean IsMappingInitialized(KWMTDatabaseMapping* mapping);

	// Parametrage de la derniere cle lue dans la table principale
	void SetLastReadRootKey(const KWObjectKey* objectKey);

	// Nettoyage d'un mapping de ses informations de contexte (last key ou last object)
	void CleanMapping(KWMTDatabaseMapping* mapping);

	// Acces au driver associe au mapping
	PLDataTableDriverTextFile* GetDriverAt(KWMTDatabaseMapping* mapping);

	// Reimplementation de methodes pour les rendre publiques
	void PhysicalDeleteDatabase() override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	friend class PLShared_MTDatabaseTextFile;

	// Calcul des informations necessaires pour la DatabaseMemoryGuard lors de l'ouverture de la base en lecture
	void ComputeMemoryGuardOpenInformation();

	// Calcul de necessaire pour les buffers pour l'ouverture, en fonction d'une taille de buffer
	longint ComputeBufferNecessaryMemory(boolean bRead, int nBufferSize) const;

	// Creation de driver, a l'usage des mappings des tables principales et secondaires
	// Parametrage a la volee des headerlines des driver de type stream
	KWDataTableDriver* CreateDataTableDriver(KWMTDatabaseMapping* mapping) const override;

	// Calcul du nombre de tables principales, hors tables externes, avec des fichiers locaux (sans URI)
	int ComputeMainLocalTableNumber();

	// Resultat de l'appel de la methode ComputeOpenInformation
	mutable IntVector ivUsedMappingFlags;
	LongintVector lvFileSizes;
	longint lTotalFileSize;
	longint lTotalUsedFileSize;
	int nDatabasePreferredBufferSize;
	LongintVector lvInMemoryEstimatedFileObjectNumbers;
	LongintVector lvEstimatedUsedMemoryPerObject;
	int nMainLocalTableNumber;
	longint lOutputNecessaryDiskSpace;
	longint lEmptyOpenNecessaryMemory;
	longint lMinOpenNecessaryMemory;
	longint lMaxOpenNecessaryMemory;
	longint lEstimatedMaxSecondaryRecordNumber;
	longint lEstimatedMinSingleInstanceMemoryLimit;
	longint lEstimatedMaxSingleInstanceMemoryLimit;

	// Definition des exigences pour la taille du buffer
	// La taille de buffer est porte par le driver
	int nReadSizeMin;
	int nReadSizeMax;

	// Ouverture des fichiers a la demande
	boolean bOpenOnDemandMode;

	// Memorisation des classes de lignes d'entete par fichier du mapping
	ObjectArray oaUsedMappingHeaderLineClasses;

	// Memorisation des index des attributs pour les mappings pour la serialisation
	// Un vecteur d'index est memorise pour chaque mapping
	// ainsi qu'un vecteur des index des attributs de cle dans le cas de classes racines
	// Utile pour la serialisation des bases destinees a etre ouverte en lecture,
	// pour transferer aux esclaves les index calcules une fois pour toutes par le maitre
	ObjectArray oaIndexedMappingsDataItemLoadIndexes;
	ObjectArray oaIndexedMappingsRootKeyIndexes;
};

///////////////////////////////////////////////////
// Classe PLShared_MTDatabaseTextFile
// Serialisation de la classe KWSTDatabaseTextFile
class PLShared_MTDatabaseTextFile : public PLSharedObject
{
public:
	// Constructeur
	PLShared_MTDatabaseTextFile();
	~PLShared_MTDatabaseTextFile();

	// Acces a la base
	void SetDatabase(PLMTDatabaseTextFile* database);
	PLMTDatabaseTextFile* GetDatabase();

	// Reimplementation des methodes virtuelles, avec transfer des specification de la base ainsi que des index
	// d'attribut par mapping
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
