// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "RMResourceManager.h"
#include "RMResourceSystem.h"
#include "RMResourceConstraints.h"
#include "PLParallelTask.h"
#include "PLKnapsackProblem.h"

///////////////////////////////////////////////////////////
// Gestion des ressources d'un systeme parallele
class RMParallelResourceManager; // Gestionnaire de ressources, qui alloue au mieux les resources en fonction des
				 // demandes
class RMTaskResourceRequirement; // Ressources demandees pour une tache
class RMTaskResourceGrant; // Ressources alouees pour une tache. Resultat du RMResourceManager qui contient un ensemble
			   // de RMResourceGrant
class PLTestCluster;       // Classe de test de l'allocation des ressources
class RMResourceContainer; // Classe qui stocke une valeur pour chaque type de ressource
class RMResourceSolutions; // Classe technique de stockage des solutions de l'optimisation des ressources

//////////////////////////////////////////////////////////////////////////
// Classe RMTaskResourceRequirement
// Exigences liees a une tache donnee.
// Les exigences d'une tache sont definies par :
//	- les exigences de chaque esclave
//	- les exigences du maitre
//  - les exigences partagees par le maitre et l'esclave
//	- les exigences partagees par tous les esclaves
//	- une politique d'allocation de l'espace disque
//	- une politique d'allocation de la memoire
//  - un nombre maximum de taches elementaires (SlaveProcess)
//
// Par defaut les exigences sont :
//		- 0 comme minimum,
//		- +inf comme maximum (sauf pour GlobalSlaveRequirement et SharedRequirement)
//		- comme politique, une preference pour les esclaves
//
// Notes sur les exigences Globales et Shared:
// 		- 	Ces exigences sont condiderees comme optionnelles c'est pourquoi le max est initialise a 0
//			Il faudra donc veiller a initialiser le min ET le max (pour eviter que min > max ). On peu
// utiliser
// la methode Set() qui affecte le min et le max en meme temps 		-	Les exigences GetSlaveRequirement et
// GetGlobalSlaveRequirement sont prises en compte en meme temps dans les ressources allouees, c'est a dire que la
// methode 			RMTaskResourceGrant::GetMinSlaveMemory() renvoie la memoire dediee a l'esclave + la
// memoire globale (ou repartie)
//
class RMTaskResourceRequirement : public Object
{
	// TODO ajouter le nombre de fichier ouverts en meme temps : openedFileNumber.
public:
	// Constructeur
	RMTaskResourceRequirement();
	~RMTaskResourceRequirement();

	// Copie et duplication
	RMTaskResourceRequirement* Clone() const;
	void CopyFrom(const RMTaskResourceRequirement* trRequirement);

	// Politique d'allocation des ressources
	enum POLICY
	{
		masterPreferred,
		slavePreferred,
		unknown
	};

	// Politique d'allocation de la memoire restante apres avoir affecte le minimum a chaque processus.
	// Soit on donne la maximum au maitre et le reste aux esclaves: masterPreferred
	// Soit on donne le maximum aux esclaves et le reste au maitre: slavePrefered (politique par defaut)
	void SetMemoryAllocationPolicy(POLICY policy);
	POLICY GetMemoryAllocationPolicy() const;

	// Meme chose que pour la memoire
	void SetDiskAllocationPolicy(POLICY policy);
	POLICY GetDiskAllocationPolicy() const;

	// Nombre maximum de taches elementaires qui devront etre traitees par les esclaves (nombre de SlaveProcess)
	// Le temps de traitement sera optimal si il y a autant d'esclaves que de SlaveProcess
	// En revanche, il est inutile d'avoir plus d'esclaves que de SlaveProcess a executer
	// par defaut, a l'infini
	// Si le nombre de process est 0, la tache n'est pas lancee
	void SetMaxSlaveProcessNumber(int nSlaveProcessNumber);
	int GetMaxSlaveProcessNumber() const;

	// Acces aux exigences de la tache pour le processus maitre
	RMResourceRequirement* GetMasterRequirement() const;

	// Acces aux exigences de la tache pour chaque processus esclave
	RMResourceRequirement* GetSlaveRequirement() const;

	// Acces aux exigences de la tache partagees par l'ensemble des processus esclaves. Permet d'exiger une
	// ressource reparties sur tous les esclaves. Si un fichier doit etre reparti sur tous les esclaves, ne
	// connaissant pas le nombre d'esclaves, on ne peut pas specifier les exigences pour chaque esclave (taille du
	// fichier /  nbSlaves). On peut par contre specifier les exigences pour l'ensemble des esclaves : pour
	// l'ensemble des esclaves, on a besoin de la taille du fichier, c'est le GlobalSlaveRequirement.
	RMResourceRequirement* GetGlobalSlaveRequirement() const;

	// Acces aux exigences de la tache pour les variables partagees (shared, input et output)
	// En sequentiel, ces exigences portent sur un seul processus.
	// En parallele, elles imposent un minumum pour chaque processus (maitre ou esclave), elles sont toutes a un
	// moment donne en meme temps chez le maitre et chez l'esclave.
	RMResourceRequirement* GetSharedRequirement() const;

	// Max des resources utilisees par les esclaves au lancement
	static RMResourceRequirement* GetSlaveSystemAtStart();

	// Resources utilisees par le maitre au lancement
	static RMResourceRequirement* GetMasterSystemAtStart();

	// Affichage
	void Write(ostream& ost) const override;

	// Transformation de la politique en chaine de caractere pour affichage
	static ALString PolicyToString(POLICY policy);

	// Verifie que toutes les exigences sont coherentes
	boolean Check() const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Politique d'allocation pour la ressource de type nResourceType
	void SetResourceAllocationPolicy(int nResourceType, POLICY policy);
	POLICY GetResourceAllocationPolicy(int nResourceType) const;

protected:
	// Exigences du maitre
	RMResourceRequirement* masterRequirement;

	// Exigences de chaque esclave
	RMResourceRequirement* slaveRequirement;

	// Exigences sur tous les esclaves
	RMResourceRequirement* globalSlaveRequirement;

	// Exigences liees aux variables partagees
	RMResourceRequirement* sharedRequirement;

	// Nombre de SlaveProcess maximum
	int nSlaveProcessNumber;

	// Exigences systeme
	static RMResourceRequirement masterSystemAtStart;
	static RMResourceRequirement slaveSystemAtStart;
	static longint lMinProcessDisk;
	static longint lMinProcessMem;

	// Politique
	IntVector ivResourcesPolicy;

	friend class RMParallelResourceDriver;  // Acces aux GetMasterSystemRequirement et GetSlaveSystemRequirement
	friend class RMParallelResourceManager; // Acces aux GetMasterSystemRequirement et GetSlaveSystemRequirement
	friend class PLParallelTask;            // Acces aux GetMasterSystemRequirement et GetSlaveSystemRequirement
};

//////////////////////////////////////////////////////////////////////////
// Classe RMParallelResourceManager
// Gestionnaire des ressources d'un ensemble de processus MPI qui peut s'executer sur une simple machine
// ou sur un cluster. Les ressources sont exposees via la classe RMResourceSystem. Le gestionnaire de ressources
// optimise au mieux le nombre de processus, la RAM et l'espace disque en fonction des ressources disponibles
// (RMResourceSystem) et des contraintes de l'utilisateur (RMResourceRequirement). Le resultat est un
// RMTaskResourceGrant i.e. une memoire et un espace disque alloues a chaque processus du systeme. Les processus n'ont
// pas necessairement tous un RMResourceGrant associe, en effet il peut y avoir plus de processus MPI que de processus
// effectivement alloues par le gestionnaire.
//
class RMParallelResourceManager : public Object
{
public:
	// Constructeur
	RMParallelResourceManager();
	~RMParallelResourceManager();

	// Calcule et renvoie les ressources allouees pour le system passe en parametre
	// Doit etre appelee avec PLTaskDriver::GetResourceSystem() pour le systeme courant
	// Il peut y avoir moins de ressources que de processus MPI
	// Si les ressource sont suffisantes pour un seul processus (sequentiel), grantedResource contient 2
	// RMResourceGrant, Un pour le maitre (index 0) et un pour l'esclave (index 1). Ces 2 RMResourceGrant ont la
	// meme valeur pour les sharedVariable. Les 2 processus partagent cette valeur (ils ont tous les 2 acces a la
	// valeur mais cette memoire ne peut etre allouee qu'uen seule fois) Si aucune ressource n'est allouee, les
	// ressources manquantes sont accessibles via la methodes GetMissingMemory et GetMissingDisk de
	// RMTaskResourceGrant Dans ce cas un message destine a l'utilisateur est genere par la methode
	// GetMissingResourceMessage La methode retourne true si des ressources peuvent etre allouees Notes :
	//   - grantedResource est reinitialise au debut de la methode, cela permet des appels successifs sur la meme
	//   instance de grantedResource
	//   - les ressources systeme sont mises a jour automatiquement au debut de la methode
	static boolean ComputeGrantedResources(const RMTaskResourceRequirement* resourceRequirement,
					       RMTaskResourceGrant* grantedResource);

	// Methode de test
	static void Test();

protected:
	static boolean ComputeGrantedResourcesForSystem(const RMResourceSystem* resourceSystem,
							const RMTaskResourceRequirement* resourceRequirement,
							RMTaskResourceGrant* grantedResource);

	// Affectation des exigences et du systeme
	void SetRequirement(const RMTaskResourceRequirement* resourceRequirement);
	void SetSystem(const RMResourceSystem* resourceSystem);

	// Calcul de l'allocation des ressources comme un probmleme de sac a dos, les parametres sont les memes que pour
	// la methode ComputeGrantedResourcesForSystem sauf bSequential. Quand celui-ci est true, on, cherche une
	// solution dans le cas sequentiel seulement : on ne prend qu'un seule machine (celle qui contient le rang 0) et
	// l'allocation locale des ressources n'est pas calculee de la meme maniere. Dans ce cas il y a 2 processus, un
	// maitre et un esclave. grantedResource est reinitialise au debut de la methode pour autoriser les appels
	// successifs sur la meme instance
	void KnapsackSolve(boolean bSequential, RMTaskResourceGrant* grantedResource) const;

	// Construction des ressources allouees (grantedResources) a partir des classes du sac a dos.
	// Ajoute des ressources restante au minum necessaire.
	void TransformClassesToResources(boolean bSequential, const ObjectArray* oaClasses, const ObjectArray* oaHosts,
					 RMTaskResourceGrant* grantedResources) const;

	// Verifie que les ressource sgranted remplissent les exigences de la tache et les contraintes utilisateurs
	boolean Check(RMTaskResourceGrant* taskResourceGrant) const;

	// Calcul le surplus de ressource allouable au maitre et a chaque esclave, en plus des exigences minimum, tout
	// en respectant les contraintes. Tout est rajoute dans la memoire principale (pas dans les shared) en
	// privilegiant soit le maitre soit les esclaves (en fonction des preferences de resourceRequirement)
	void ComputeExtraResource(int nResourceType, boolean bSequential, int nProcNumberOnHost,
				  int nProcNumberOnSystem, const RMHostResource* hostResource, longint lSlaveResource,
				  longint lMasterResource, longint& lExtraSlaveResource,
				  longint& lExtraMasterResource) const;

	// Diminue la ressource allouee a chaque esclave (en entree lRemainingResource) de facon a ce qu'elle verifie
	// les exigences et les contrainte
	void ShrinkForSlaveUnderConstraints(boolean bIsSequential, int nResourceType, int nSlaveNumberOnHost,
					    int nSlaveNumberOnSystem, int nMasterNumber,
					    longint& lRemainingResource) const;

	// Diminue la ressource allouee au maitre (en entree lRemainingResource) de facon a ce qu'elle verifie les
	// exigences et les contrainte
	void ShrinkForMasterUnderConstraints(boolean bIsSequential, int nResourceType, int nSlaveNumber,
					     int nMasterNumber, longint& lRemainingResource) const;

	// Alloue les ressources au maitre et a chaque esclave tout en verifiant les exigences minimales.
	// Renvoie les ressources consommes sur la machine ainsi que les resources min sur le maitre et chaque esclave
	longint MinimumAllocation(boolean bIsSequential, int nResourceType, int nProcNumber,
				  const RMHostResource* hostResource, longint& lSlaveResource,
				  longint& lMasterResource) const;

	// Calcule le nombre de processus maximum qui peuvent s'executer
	// La variable lMissingResource est mise a jour avec les ressources manquantes dans le cas ou le nombre de proc
	// vaut 0
	int ComputeProcessNumber(boolean bSequential, int nRT, longint lLogicalHostResource, boolean bIsMasterHost,
				 longint& lMissingResource) const;

	// Calcule l'utilisation des ressources pour nProcNumber process, en utilisant lMasterUse pour le maitre,
	// lSlaveUse pour les esclaves
	longint GetUsedResource(int nProcNumber, boolean bIsMasterHost, longint lMasterUse, longint lSlaveUse) const;

	// Methode utilitaire qui renvoie la reserve par processus (reserve pour l'allocateur + reserve de memoire +
	// java+ serialisation)
	static longint GetMasterReserve(int nResourceType);
	static longint GetSlaveReserve(int nResourceType);

	// Methode utilitaire qui renvoie les ressources cachees a l'utilisateur, elles sont allouees mais pas
	// disponibles : shared, start et reserve
	longint GetMasterHiddenResource(boolean bIsSequential, int nResourceType) const;
	longint GetSlaveHiddenResource(boolean bIsSequential, int nResourceType) const;

	// Methodes utilitaires pour acceder au exigences
	longint GetSlaveMin(int nResourceType) const;
	longint GetSlaveMax(int nResourceType) const;
	longint GetMasterMin(int nResourceType) const;
	longint GetMasterMax(int nResourceType) const;
	longint GetSharedMin(int nResourceType) const;
	longint GetSlaveGlobalMin(int nResourceType) const;
	longint GetSlaveGlobalMax(int nResourceType) const;

	// Exigences et systeme, en consultation
	const RMResourceSystem* resourceSystem;
	const RMTaskResourceRequirement* resourceRequirement;

	friend class PLTestCluster;
	friend class PLShared_TaskResourceGrant; // Pour la methode de test
	friend class PLParallelTask;             // Pour detruire ivFileServerRanks
};

//////////////////////////////////////////////////////////////////////////
// Classe RMResourceContainer
// Contient une valeur de chaque ressource
class RMResourceContainer : public Object
{
public:
	// Constructeur
	RMResourceContainer();
	~RMResourceContainer();

	// Copie
	RMResourceContainer* Clone() const;

	// Acces au valeurs
	longint GetValue(int nResourceType) const;
	void SetValue(int nResourceType, longint lValue);

protected:
	LongintVector lvResources;
};

//////////////////////////////////////////////////////////////////////////
// Classe PLTestCluster
// Classe de Test de RMParallelResourceManager
// Cree un cluster synthetique et des contraintes (utilisateur, application)
// et alloue les ressources a partir de ces contraintes en utilisant la classe RMParallelResourceManager
class PLTestCluster : public Object
{
public:
	// Constructeur
	PLTestCluster();
	~PLTestCluster();

	////////////////////////////////
	// Configuration du systeme

	// Intitule du test pour affichage
	void SetTestLabel(const ALString& sName);

	// Nombre de machines
	void SetHostNumber(int nHostNumber);

	// Nombre de coeurs maximum par Host
	void SetHostCores(int nCoresNumber);

	// Memoire disponible sur chaque host
	void SetHostMemory(longint lMemory);

	// Disque disponible sur chaque host
	void SetHostDisk(longint lDisk);

	// Memoire au debut du run (par defaut a 8Mo)
	void SetMasterSystemAtStart(longint lMemory);
	void SetSlaveSystemAtStart(longint lMemory);

	// Taille des machines :
	// - 0 les machines sont identiques (par defaut),
	// - 1 elles sont de taille croissante (1 coeur pour le maitre),
	// - 2 elles sont de taille decroissante (nCoresNumber pour le maitre)
	void SetSystemConfig(int nConfig);

	// Met la configuration dan l'etat initial
	void Reset();

	//////////////////////////////////////////
	// Configuration des contraintes utilisateurs
	void SetConstraintMasterMemory(longint lMin, longint lMax);
	void SetConstraintSlaveMemory(longint lMin, longint lMax);
	void SetConstraintMasterDisk(longint lMin, longint lMax);
	void SetConstraintSlaveDisk(longint lMin, longint lMax);
	void SetConstraintSharedMemory(longint lMin, longint lMax);
	void SetSlaveProcessNumber(int nProcess);
	void SetSlaveGlobalMemoryMin(longint lMin);
	void SetSlaveGlobalMemoryMax(longint lMax);
	void SetSlaveGlobalDiskMin(longint lMin);

	void SetMemoryLimitPerHost(longint lMemory);
	void SetDiskLimitPerHost(longint lDisk);

	void SetMemoryLimitPerProc(longint lMax);
	void SetDiskLimitPerProc(longint lMax);
	void SetMaxCoreNumberPerHost(int nMax);
	void SetMaxCoreNumberOnSystem(int nMax);

	//////////////////////////////////////////
	// Resolution
	void Solve();

protected:
	// Variables qui definissent le systeme
	int nHostNumber;
	int nCoresNumberPerHost;
	longint lMemoryPerHost;
	longint lDiskPerHost;
	int nSystemConfig;
	ALString sName;
	longint lMasterSystemAtStart;
	longint lSlaveSystemAtStart;

	// Variables qui definissent les contraintes

	// Contrainte sdu programme
	longint lMinMasterMemory;
	longint lMaxMasterMemory;
	longint lMinSlaveMemory;
	longint lMaxSlaveMemory;
	longint lMinMasterDisk;
	longint lMaxMasterDisk;
	longint lMinSlaveDisk;
	longint lMaxSlaveDisk;
	longint lMinSharedMemory;
	longint lMaxSharedMemory;
	int nProcessNumber;
	longint lMinSlaveGlobalMemory;
	longint lMaxSlaveGlobalMemory;
	longint lMinGlobalConstraintDisk;

	// Contraintes de l'utilisateur
	longint lMemoryLimitPerHost;
	longint lDiskLimitPerHost;
	longint lMemoryLimitPerProc;
	longint lDiskLimitPerProc;
	int nMaxCoreNumberPerHost;
	int nMaxCoreNumberOnSystem;
	int nMaxCoreNumber;
};

//////////////////////////////////////////////////////////////////////////
// Classe RMTaskResourceGrant
// Cette classe represente les ressources allouees pour une tache parallele.
// Elle est constituee d'un ensemble de RMResourceGrant. Il y en a un par processus (sauf en sequentiel ou il y en a 2)
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

	// Memoire et disque du maitre
	longint GetMasterMemory() const;
	longint GetMasterDisk() const;

	// Memoire et disque de l'esclave de rang i (i>0)
	longint GetSlaveMemoryOf(int i) const;
	longint GetSlaveDiskOf(int i) const;

	// Memoire paratgee
	longint GetSharedMemory() const;

	// Minimum alloues sur tous les esclaves
	longint GetMinSlaveMemory() const;
	longint GetMinSlaveDisk() const;
	longint GetMinSlaveResource(int nResourceType) const;

	// Ressources logiques manquantes lorqu'on ne peut pas allouer
	longint GetMissingMemory();
	longint GetMissingDisk();

	// Message informant du manque de ressource suivant le type de ressource manquante
	ALString GetMissingResourceMessage();

	// Affichage
	void Write(ostream& ost) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Acces a la ressource du processus de rang nRank
	// Renvoi NULL si aucubne ressources n'est allouee pour ce processus
	RMResourceGrant* GetResourceAtRank(int nRank) const;

	// Ajout d'une resource
	void AddResource(RMResourceGrant* resource);

	// Reindexation des ressources (mise a jour de oaResourceGrantWithRankIndex)
	// a utilise lorsque le rang des resources a change
	void ReindexRank();

	// Initiamisation avec les valeurs par defaut
	void Initialize();

protected:
	ObjectArray oaResourceGrant;              // Tableau de RMResourceGrant
	ObjectArray oaResourceGrantWithRankIndex; // Tableau de RMResourceGrant indexes par leur rang
	LongintVector lvMissingResources;

	// Vrai si il n'y a pas assez de ressources a cause des contraintes par processus
	boolean bMissingResourceForProcConstraint;

	// Vrai si il n'y a pas assez de ressource sa cause des contraintes globales sur les esclaves
	boolean bMissingResourceForGlobalConstraint;

	// Host sur lequel il manque de la memoire dans la cas de contraintes globales
	ALString sHostMissingResource;

	friend class PLShared_TaskResourceGrant;
	friend class RMParallelResourceManager;
	friend class PLKnapsackResources;
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

	// Rang du processus MPI
	void SetRank(int nRank);
	int GetRank() const;

	// Memoire allouee aux sharedVariables
	void SetSharedMemory(longint lMemory);
	longint GetSharedMemory() const;

	// Disque alloue aux sharedVariables
	void SetSharedDisk(longint lDisk);
	longint GetSharedDisk() const;

	// Acces generique
	void SetSharedResource(int nResource, longint lValue);
	longint GetSharedResource(int nResource) const;
	void SetResource(int nResource, longint lValue);
	longint GetResource(int nResource) const;

	// Verifie que la ressource a ete correctement initialisee
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	int nRank;
	LongintVector lvResource;
	LongintVector lvSharedResource;

	friend class PLShared_ResourceGrant;
};

//////////////////////////////////////////////////////////////////////////
// Classe PLKnapsackResources
// Cette classe est une specialisation de PLKnapsackProblem
// Elle prend en compte les contraintes globales en ressource sur les esclaves
class PLKnapsackResources : public PLKnapsackProblem
{
public:
	PLKnapsackResources();
	~PLKnapsackResources();

	void SetRequirement(const RMTaskResourceRequirement* rq);
	void SetGrantedResource(RMTaskResourceGrant* grantedResource);

	void SetHostResources(const ObjectArray* oaHosts);

protected:
	// Renvoie true si les contraintes globales sur les esclaves sont verifiees
	boolean DoesSolutionFitGlobalConstraint() override;

	const RMTaskResourceRequirement* resourceRequirement;
	RMTaskResourceGrant* grantedResource;
	const ObjectArray* oaHostResources;
};

inline void RMTaskResourceRequirement::SetResourceAllocationPolicy(int nResourceType, POLICY policy)
{
	require(nResourceType < UNKNOWN);
	ivResourcesPolicy.SetAt(nResourceType, policy);
}

inline RMTaskResourceRequirement::POLICY RMTaskResourceRequirement::GetResourceAllocationPolicy(int nResourceType) const
{
	require(nResourceType < UNKNOWN);
	return (POLICY)ivResourcesPolicy.GetAt(nResourceType);
}

inline void RMTaskResourceRequirement::SetMemoryAllocationPolicy(POLICY policy)
{
	SetResourceAllocationPolicy(MEMORY, policy);
}

inline RMTaskResourceRequirement::POLICY RMTaskResourceRequirement::GetMemoryAllocationPolicy() const
{
	return GetResourceAllocationPolicy(MEMORY);
}

inline void RMTaskResourceRequirement::SetDiskAllocationPolicy(POLICY policy)
{
	SetResourceAllocationPolicy(DISK, policy);
}

inline RMTaskResourceRequirement::POLICY RMTaskResourceRequirement::GetDiskAllocationPolicy() const
{
	return GetResourceAllocationPolicy(DISK);
}

inline RMResourceRequirement* RMTaskResourceRequirement::GetMasterRequirement() const
{
	return masterRequirement;
}

inline RMResourceRequirement* RMTaskResourceRequirement::GetSlaveRequirement() const
{
	return slaveRequirement;
}

inline RMResourceRequirement* RMTaskResourceRequirement::GetGlobalSlaveRequirement() const
{
	return globalSlaveRequirement;
}

inline RMResourceRequirement* RMTaskResourceRequirement::GetSharedRequirement() const
{
	return sharedRequirement;
}

inline RMResourceRequirement* RMTaskResourceRequirement::GetMasterSystemAtStart()
{
	return &masterSystemAtStart;
}

inline RMResourceRequirement* RMTaskResourceRequirement::GetSlaveSystemAtStart()
{
	return &slaveSystemAtStart;
}

inline void RMTaskResourceRequirement::SetMaxSlaveProcessNumber(int nValue)
{
	require(nValue >= 0);
	nSlaveProcessNumber = nValue;
}

inline int RMTaskResourceRequirement::GetMaxSlaveProcessNumber() const
{
	return nSlaveProcessNumber;
}