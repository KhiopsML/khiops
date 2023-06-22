// Copyright (c) 2023 Orange. All rights reserved.
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

	//////////////////////////////////////////////////////////////////////////////////////
	// Calculs prealables a l'utilisation de la base, avant son utilisation en
	// lecture ou en ecriture

	// Calcul d'informations necessaires a l'ouverture de la base en lecture ou ecriture
	// Cette methode est a appeler dans le maitre, essentiellement pour permettre un
	// dimensionnment fin des ressources
	// Le parametre outputDatabaseTextFile est optionnel, et n'est utilise que pour l'estimation
	// de la place disque pour une base en sortie
	//   . taille des fichiers en lecture
	//   . memoire minimum necessaire pour l'ouverture de la base
	//   . indexation des champs de la table en lecture
	// Les resultats ne sont pas transmis aux esclaves (sauf l'indexation des champs)
	boolean ComputeOpenInformation(boolean bRead, boolean bIncludingClassMemory,
				       PLMTDatabaseTextFile* outputDatabaseTextFile);
	boolean IsOpenInformationComputed() const;

	// Nettoyage des informations d'ouverture
	void CleanOpenInformation();

	// Acces aux mappings effectivement utilises
	// Les mappings (KWLoadIndexVector) sont collectes dans le meme ordre et nombre que les mappings de la base
	// s'ils sont utilises (NULL sinon)
	// Memoire: le tableau et son contenu appartiennent a l'appele
	const ObjectArray* GetUsedMappings() const;

	// Nombre de mapping effectivement utilises, c'est a dire non NULL dans le tableau precedent
	int GetUsedMappingNumber() const;

	// Acces aux tailles des fichiers pour les mapping effectivement utilises en lecture (0 sinon)
	const LongintVector* GetInputFileSizes() const;
	longint GetTotalInputFileSize() const;

	// Memoire minimum necessaire pour ouvrir la base sans tenir compte des buffers
	longint GetEmptyOpenNecessaryMemory() const;

	// Memoire minimum et maximum necessaire pour ouvrir la base, en tenant compte des buffers
	longint GetMinOpenNecessaryMemory() const;
	longint GetMaxOpenNecessaryMemory() const;

	// Memoire necessaire pour stocker le fichier d'une base en sortie a partir d'une base en lecture (0 sinon)
	longint GetOutputNecessaryDiskSpace() const;

	// Nombre maximum de taches elementaires qui devront etre traitees par les esclaves
	int GetMaxSlaveProcessNumber() const;

	// Calcul de la memoire par buffer pour une memoire allouee pour l'ouverture
	int ComputeOpenBufferSize(boolean bRead, longint lOpenGrantedMemory) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Parametrage des buffers

	// Taille du buffer du driver : a manipuler avec precaution
	// Taille du buffer lors de la prochaine ouverture
	void SetBufferSize(int nBufferSize);
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
	// Calcul de necessaire pour les buffers pour l'ouverture, en fonction d'une taille de buffer
	longint ComputeBufferNecessaryMemory(boolean bRead, int nBufferSize) const;

	// Creation de driver, a l'usage des mappings des tables principales et secondaires
	// Parametrage a la volee des headerlines des driver de type stream
	KWDataTableDriver* CreateDataTableDriver(KWMTDatabaseMapping* mapping) const override;

	// Resultat de l'appel de la methode ComputeOpenInformation
	mutable ObjectArray oaUsedMappings;
	LongintVector lvInputFileSizes;
	longint lTotalInputFileSize;
	longint lOutputNecessaryDiskSpace;
	longint lEmptyOpenNecessaryMemory;
	longint lMinOpenNecessaryMemory;
	longint lMaxOpenNecessaryMemory;

	// Memoire minimum par buffer pour l'ouverture de la base
	static const int nMinOpenBufferSize = BufferedFile::nDefaultBufferSize / 8;
	static const int nMaxOpenBufferSize = BufferedFile::nDefaultBufferSize * 8;

	// Memorisation des index des attributs pour les mappings pour la serialisation
	// Un vecteur d'index est memorise pour chaque mapping
	// ainsi qu'un vecteur des index des attributs de cle dans le cas de classes racines
	// Utile pour la serialisation des bases destinees a etre ouverte en lecture,
	// pour transferer aux esclaves les index calcules une fois pour toutes par le maitre
	ObjectArray oaIndexedMappingsDataItemLoadIndexes;
	ObjectArray oaIndexedMappingsRootKeyIndexes;
	friend class PLShared_MTDatabaseTextFile;
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
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
