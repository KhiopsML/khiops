// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"
#include "RMResourceConstraints.h" // Uniquement pour l'enum resources

class RMHostResource;
class RMResourceSystem;

/////////////////////////////////////////////////////////////////////////////
// Classe RMResourceSystem
// Recense les ressources pour chaque machine du systeme en terme de
// nombre de coeurs disponibles, memoire vive, taille du disque.
// Les accesseurs ne peuvent etre utilises que si l'objet a ete initialise.
// (mecanisme d'assertion si SetInitialized() n'a pas ete appele)
// Par defaut les ressource sont initialisees a partir de la machine locale.
class RMResourceSystem : public Object
{
public:
	/////////////////////////////////////////////////////////////////////////////
	// Accces aux ressources en consultation, pour le maitre uniquement

	// Nombre de coeurs du systeme
	int GetPhysicalCoreNumber() const;

	// Nombre de processus logiques (MPI) lances sur le systeme
	int GetLogicalProcessNumber() const;

	// Memoire physique du systeme
	longint GetPhysicalMemory() const;

	// Espace disque du systeme
	longint GetDiskFreeSpace() const;

	// Nombre de hosts
	int GetHostNumber() const;

	// Acces aux ressources physiques du master host
	const RMHostResource* GetMasterHostResource() const;

	// Acces au ith eme host
	RMHostResource* GetHostResourceAt(int ith) const;

	// Renvoie l'index de l'host sur lequel se trouve le process processId
	// Renvoie -1 si le processId n'est sur aucun host
	// Utile pour l'affichage
	int GetHostIndex(int processId) const;

	/////////////////////////////////////////////////////////////////////////////
	// Accces aux ressources en modification
	// Par defaut les ressources sont initialisees a partir des ressources locales
	// L'initialisation sur des machines paralleles (MPI et/ou cluster) est effectuee
	// dans la classe PLMPITaskDriver (MPI)

	// Constructeur
	RMResourceSystem();
	~RMResourceSystem();

	// Initialise avec la machine locale
	void InitializeFromLocaleHost();

	// Ajoute un host
	void AddHostResource(RMHostResource* resource);

	// Remet les ressources dans l'etat initial (non initialise)
	void Reset();

	// Modification du nom du ieme host
	void SetHostNameAt(int ithHost, const ALString& sHostName);
	ALString GetHostNameAt(int ith) const;

	// Modification de l'espace disque du ieme host
	void SetHostDiskAt(int ithHost, longint lDiskSpace);
	longint GetHostDiskAt(int ith) const;

	// Supprime les process dont le rang est passe en parametre
	void RemoveProcesses(const IntVector* processToReomve);

	/////////////////////////////////////////////////////////////////////////////
	// Services divers

	// Duplication
	RMResourceSystem* Clone() const;

	// Affichage des infos sur le cluster
	void Write(ostream& ost) const override;

	// Cree un ensemble artificiel de resources. Pour les tests.
	// nSystemConfig permet de faire varier le nombre de coeurs des machines :
	// - 0 les machines sont identiques (nProcNumber pour tout le monde),
	// - 1 elles sont de taille croissante (1 coeur pour le maitre),
	// - 2 elles sont de taille decroissante (nCoresNumber pour le maitre)
	static RMResourceSystem* CreateSyntheticCluster(int nHostNumber, int nProcNumber, longint lPhysicalMemory,
							longint lDiskFreeSpace, int nSystemConfig);

	// Cree un ensemble artificiel de resources pour les tests.
	// Les machines ont toute sles memes ressources, pour creer des classe differentes on fait varier le nombre de
	// processeurs.
	static RMResourceSystem* CreateSyntheticClusterWithClasses(int nHostNumber, int nProcNumber,
								   longint lPhysicalMemory, longint lDiskFreeSpace,
								   int bClassNumber);

	static RMResourceSystem* CreateAdhocCluster();

	// Creation d'un cluster de 2 machines, chacune ayant 8 coeurs
	// La premiere a 8 Go de memoire la seconde 100 Go
	// Le master est sur la premiere machine
	static RMResourceSystem* CreateUnbalancedCluster();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Est-ce que les ressources ont ete initialisees
	// Les accesseurs sont utilisables seulement si les ressource sont initialisees
	boolean IsInitialized() const;
	void SetInitialized();

	// Tableau des hostRessource
	ObjectArray oaHostResources;
	boolean bIsInitialize;

	friend class PLMPITaskDriver;
	friend void RemoveProcessOnEachHost(RMResourceSystem* resourceSystem);
};

/////////////////////////////////////////////////////////////////////////////
// Classe RMHostResource
// Recense les ressources systeme d'une  machine en terme de
// nombre de coeurs, memoire vive, taille du disque.
class RMHostResource : public Object
{
public:
	/////////////////////////////////////////////////////////////////////////////
	// Accces aux ressources en consultation

	// Nom du host
	const ALString& GetHostName() const;

	// Memoire disponible sur ce host (en octets)
	longint GetPhysicalMemory() const;

	// Espace disque disponible sur ce host  (pour le repertoire temporaire, en octets)
	longint GetDiskFreeSpace() const;

	// Renvoie le nombre de processus MPI de ce host
	int GetLogicalProcessNumber() const;

	// Renvoie le nombre de coeurs de la machine
	int GetPhysicalCoreNumber() const;

	// Retourne la liste des rangs des processus MPI qui s'execute sur ce host
	const IntVector* GetRanks() const;

	// Renvoie true si le noeud contient le maitre (Rank = 0)
	boolean IsMasterHost() const;

	/////////////////////////////////////////////////////////////////////////////
	// Accces aux ressources en modification

	// Constructeurs
	RMHostResource();
	~RMHostResource();

	// Nom du host
	void SetHostName(const ALString& sHostName);

	// Memoire disponible sur ce host en octets
	void SetPhysicalMemory(longint);

	// Espace disque disponible sur ce host en octets (pour le repertoire temporaire)
	void SetDiskFreeSpace(longint);

	// Ajoute un rang
	void AddProcessusRank(int nRank);

	// Nombre de coeurs
	void SetPhysicalCoresNumber(int nCoresNumber);

	// Acces aux ressources de maniere generique
	void SetResourceFree(int nResourceType, longint nValue);
	longint GetResourceFree(int nResourceType) const;

	/////////////////////////////////////////////////////////////////////////////
	// Services divers

	// Duplication
	RMHostResource* Clone() const;

	// Affichage des infos sur la machine
	// entete  "hostname" << "\t" << "MPI ranks" << "\t" << "logical memory (MB)" << "\t" << "disk (GB)";
	void Write(ostream& ost) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	ALString sHostName;
	LongintVector lvResources; // TODO remplacer par RMResourecContainer
	IntVector ivRanks;
	int nPhysicalCoresNumber;

	friend class PLShared_HostResource;
	friend class RMResourceSystem; // Pour RemoveOneSlavePerHost
};
