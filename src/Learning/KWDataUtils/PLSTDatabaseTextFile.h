// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class PLSTDatabaseTextFile;
class PLShared_STDatabaseTextFile;

#include "KWSTDatabaseTextFile.h"
#include "PLDataTableDriverTextFile.h"
#include "PLSharedObject.h"

///////////////////////////////////////////////////////////////////
// Classe PLSTDatabaseTextFile
// Classe "technique", dediee aux applications paralleles de la classe KWSTDatabaseTextFile
//  avec gestion des informations d'ouverture pour le dimensionnement des taches,
//  l'analyse de header de tables, et memorisation pour envoi aux esclaves via une variable partagee
// Implementation de methodes generiques definies dans PLDatabaseTextFile
//	 Le driver associe est PLDataTableDriverTextFile
//   Les buffers sont geres a l'exterieur de la classe (d'ou SetOutputBuffer et  SetInputBuffer)
//   Cette classe n'ouvre et ne ferme pas les buffers
class PLSTDatabaseTextFile : public KWSTDatabaseTextFile
{
public:
	// Conctructeur
	PLSTDatabaseTextFile();
	~PLSTDatabaseTextFile();

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
				       PLSTDatabaseTextFile* outputDatabaseTextFile);
	boolean IsOpenInformationComputed() const;

	// Nettoyage des informations d'ouverture
	void CleanOpenInformation();

	// Acces en lecture et ecriture aux index des data items
	const KWLoadIndexVector* GetConstDataItemLoadIndexes() const;
	KWLoadIndexVector* GetDataItemLoadIndexes();

	// Acces a la taille du fichier en lecture (0 sinon)
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

	// Referencement des buffers, ils sont geres en dehors de la classe
	void SetOutputBuffer(OutputBufferedFile* buffer);
	void SetInputBuffer(InputBufferedFile* buffer);
	OutputBufferedFile* GetOutputBuffer() const;
	InputBufferedFile* GetInputBuffer() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Methodes techniques

	// Acces au driver
	PLDataTableDriverTextFile* GetDriver();

	// Reimplementation de methodes pour les rendre publiques
	void PhysicalDeleteDatabase() override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Memoire minimum par buffer pour l'ouverture de la base
	static const int nMinOpenBufferSize = BufferedFile::nDefaultBufferSize / 2;
	static const int nMaxOpenBufferSize = BufferedFile::nDefaultBufferSize * 8;

	// Resultat de l'appel de la methode ComputeOpenInformation
	longint lTotalInputFileSize;
	longint lOutputNecessaryDiskSpace;
	longint lEmptyOpenNecessaryMemory;
	longint lMinOpenNecessaryMemory;
	longint lMaxOpenNecessaryMemory;
	friend class PLShared_STDatabaseTextFile;
};

///////////////////////////////////////////////////
// Classe PLShared_STDatabaseTextFile
// Serialisation de la classe KWSTDatabaseTextFile
class PLShared_STDatabaseTextFile : public PLSharedObject
{
public:
	// Constructeur
	PLShared_STDatabaseTextFile();
	~PLShared_STDatabaseTextFile();

	// Acces a la base
	void SetDatabase(PLSTDatabaseTextFile* database);
	PLSTDatabaseTextFile* GetDatabase();

	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
