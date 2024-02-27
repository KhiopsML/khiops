// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "RMResourceManager.h"
#include "PLSharedObject.h"

class RMResourceContainer; // Classe qui stocke une valeur pour chaque type de ressource
class RMResourceGrant;     // Les ressource allouees a un processus
class RMTaskResourceGrant; // Ressources alouees pour une tache. Resultat du RMResourceManager qui contient un ensemble
			   // de RMResourceGrant

//////////////////////////////////////////////////////////////////////////
// Classe RMResourceContainer
// Contient une valeur de chaque ressource
class RMResourceContainer : public Object
{
public:
	// Constructeur
	RMResourceContainer();
	~RMResourceContainer();

	// Copie et duplication
	void CopyFrom(const RMResourceContainer* container);
	RMResourceContainer* Clone() const;

	// Acces au valeurs
	longint GetValue(int nResourceType) const;
	void SetValue(int nResourceType, longint lValue);

	// Ajout d'une valeur en gerant correctement les depassements
	void AddValue(int nResourceType, longint lValue);

	// Met a jour chacune des ressources si celles de other sont plus petites
	void UpdateMin(const RMResourceContainer* other);

	// Remise a 0
	void Initialize();

	// Affichage
	void Write(ostream& ost) const override;

protected:
	LongintVector lvResources;

	friend class PLShared_ResourceContainer;
};

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_ResourceContainer
// Serialisation de RMResourceContainer
//
class PLShared_ResourceContainer : public PLSharedObject
{
public:
	// Constructeur
	PLShared_ResourceContainer();
	~PLShared_ResourceContainer();

	// Acces au RMResourceGrant
	void SetResourceContainer(RMResourceContainer* rmResourceContainer);
	RMResourceContainer* GetResourceContainer();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

//////////////////////////////////////////////////////////////////////////
// Classe RMResourceGrant
// Cette classe represente les ressource allouees a un processus.
class RMResourceGrant : public Object
{
public:
	// Constructeur
	RMResourceGrant();
	~RMResourceGrant();

	// Memoire disponible pour ce processus
	void SetMemory(longint lMemory);
	longint GetMemory() const;

	// Espace disque disponible pour ce process
	void SetDiskSpace(longint lDisk);
	longint GetDiskSpace() const;

	// Memoire allouee aux sharedVariables
	void SetSharedMemory(longint lMemory);
	longint GetSharedMemory() const;

	// Disque alloue aux sharedVariables
	void SetSharedDisk(longint lDisk);
	longint GetSharedDisk() const;

	// Acces generique
	RMResourceContainer* GetSharedResource() const;
	RMResourceContainer* GetResource() const;

	// Verifie que la ressource a ete correctement initialisee
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	RMResourceContainer* rcResource;
	RMResourceContainer* rcSharedResource;

	friend class PLShared_ResourceGrant;
};

//////////////////////////////////////////////////////////////////////////
// Classe RMTaskResourceGrant
// Cette classe represente les ressources allouees pour une tache parallele.
// Elle est constituee d'un ensemble de RMResourceGrant. Il y en a un par processus (sauf en sequentiel ou il y en a 2)
// Un RMResourceGrant est la memoire mise a disposition pour un esclave ou un maitre de la tache, elle ne contient pas
// la memoire prevue pour les shared variables (meme si celle-ci est bien prise en compte). D'autre part la memoire
// disponible pour un esclave est la somme de l'exigence de l'esclave (GetSlaveRequirement) et de l'exigence globale
// (GetGlobalSlaveRequirement) Si il y a plus de memoire allouee que le minimum des exigences, c'est à l'utisateur de
// cette classe de repartir le surplus vers les ressources globales ou vers les exigences de l'esclave.
class RMTaskResourceGrant : public Object
{
public:
	// Constructeur
	RMTaskResourceGrant();
	~RMTaskResourceGrant();

	// Nombre de processus (maitre ou esclave)
	int GetProcessNumber() const;

	// Nombre d'esclaves (1 en sequentiel, GetProcessNumber() -1 sinon )
	int GetSlaveNumber() const;

	// Tache sequentielle (GetProcessNumber==1)
	boolean IsSequentialTask() const;

	// Renvoie true si aucune ressource n'a pu etre allouee
	// C'est la cas si il n'y a pas assez de ressources ou
	// si le nombre max de SlaveProcess est a 0
	boolean IsEmpty() const;

	// Acces aux ressources allouees
	const RMResourceGrant* GetGrantedResourceForSlave() const;
	const RMResourceGrant* GetGrantedResourceForMaster() const;

	// Ressources logiques manquantes lorqu'on ne peut pas allouer
	longint GetMissingMemory() const;
	longint GetMissingDisk() const;

	// Renvoie true si l'esclave de rang nRank peut travailler
	boolean IsResourcesForProcess(int nRank) const;

	// Message informant du manque de ressource suivant le type de ressource manquante
	ALString GetMissingResourceMessage();

	///////////////////////////////////////////////////
	// Methodes utililitaires : redirection vers GetGrantedResourceForSlave et GetGrantedResourceForMaster

	// Memoire et disque du maitre
	longint GetMasterMemory() const;
	longint GetMasterDisk() const;
	longint GetMasterResource(int nResourceType) const;

	// Memoire partagee
	longint GetSharedMemory() const;
	longint GetSharedResource(int nResourceType) const;

	// Resources alloues a chaque esclave
	longint GetSlaveMemory() const;
	longint GetSlaveDisk() const;
	longint GetSlaveResource(int nResourceType) const;

	// Affichage
	void Write(ostream& ost) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Initialisation avec les valeurs par defaut
	void Initialize();

protected:
	void SetHostCoreNumber(const ALString& sHostName, int nProcNumber);

	RMResourceContainer* rcMissingResources;

	// Vrai si il n'y a pas assez de ressources a cause des contraintes par processus
	boolean bMissingResourceForProcConstraint;

	// Vrai si il n'y a pas assez de ressource sa cause des contraintes globales sur les esclaves
	boolean bMissingResourceForGlobalConstraint;

	// Host sur lequel il manque de la memoire dans le cas de contraintes globales
	ALString sHostMissingResource;

	// Ressources allouees
	RMResourceGrant* slaveResourceGrant;
	RMResourceGrant* masterResourceGrant;

	// Processus qui ont des ressources :
	// l'index i vaut 1 si le processus de rang i peut travailler
	IntVector ivProcessWithResources;

	// Nombre de processus par host
	ObjectDictionary odHostProcNumber;

	// Nombre de processus
	int nProcessNumber;

	friend class PLShared_TaskResourceGrant;
	friend class RMParallelResourceManager;
	friend class RMParallelResourceManager;
	friend class PLKnapsackResources;
};

inline longint RMResourceContainer::GetValue(int nResourceType) const
{
	require(nResourceType < RESOURCES_NUMBER);
	return lvResources.GetAt(nResourceType);
}
