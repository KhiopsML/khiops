// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "RMTaskResourceRequirement.h"
#include "RMTaskResourceGrant.h"
#include "PLParallelTask.h"
#include "SystemFileDriverCreator.h"

class RMParallelResourceManager;
class PLHostClassDefinition;
class PLHostClass;
class PLHostClassSolution;
class PLSolution;
class PLSolutionResources;

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
	// valeur mais cette memoire ne peut etre allouee qu'uen seule fois) Dans le mode Parallele Simule, les
	// ressources allouees sont celle d'un maitre et d'un esclave. Si aucune ressource n'est allouee, les ressources
	// manquantes sont accessibles via la methodes GetMissingMemory et GetMissingDisk de RMTaskResourceGrant Dans ce
	// cas un message destine a l'utilisateur est genere par la methode GetMissingResourceMessage La methode
	// retourne true si des ressources peuvent etre allouees Notes :
	//   - grantedResource est reinitialise au debut de la methode, cela permet des appels successifs sur la meme
	//   instance de grantedResource
	//   - les ressources systeme sont mises a jour automatiquement au debut de la methode
	static boolean ComputeGrantedResources(const RMTaskResourceRequirement* resourceRequirement,
					       RMTaskResourceGrant* grantedResource);

	static boolean ComputeGrantedResourcesForCluster(const RMResourceSystem* cluster,
							 const RMTaskResourceRequirement* resourceRequirement,
							 RMTaskResourceGrant* grantedResource);

	const ALString GetClassLabel() const override;

	// Methode de test
	static void Test();

	// Test les performances (temps d'execution)
	static void TestPerformances();

protected:
	// Nettoyage complet pour se remettre dans l'etat initial
	void Clean();

	// Exigences de la taches
	void SetRequirement(const RMTaskResourceRequirement* taskRequirements);
	const RMTaskResourceRequirement* GetRequirements() const;

	// Ressources du cluster
	void SetResourceCluster(const RMResourceSystem* cluster);
	const RMResourceSystem* GetResourceCluster() const;

	// Resolution du probleme et renvoi de la meilleure solution
	// Renvoie systematiquement la meilleure solution, potentiellement vide si impossible
	PLSolution* Solve();

	// 1ere phase de la methode Solve()
	// Algorithme glouton qui construit une solution qui repond aux contraintes globales (PLClusterSolution) a
	// partir de la solution passee en parametre La solution passee en parametre est modifiee avce la solution
	// trouvee
	void GreedySolve(PLSolution* solution);

	// 2eme phase de la methode Solve()
	// Amelioration de la meilleure solution trouvee par GreedySolve
	void PostOptimize(PLSolution* solution);

	// Construction de la solution sequentielle : seulement la machine maitre
	PLSolution* BuildSequentialSolution() const;

	// Construction de la solution initiale, c'est ici que sont definies les classes de machines
	PLSolution* BuildInitialSolution() const;

	// Transformation de la solution passee en parametre an RMTaskResourceGrant
	void BuildGrantedResources(const PLSolution*, const RMResourceSystem* resourceSystem,
				   RMTaskResourceGrant* grantedResource) const;

	// Verifie que les ressource sgranted remplissent les exigences de la tache et les contraintes utilisateurs
	boolean Check(const RMTaskResourceGrant* taskResourceGrant) const;

	// Methode utilitaire qui renvoie la reserve par processus (reserve pour l'allocateur + reserve de memoire +
	// java+ serialisation)
	static longint GetMasterReserve(int nResourceType);
	static longint GetSlaveReserve(int nResourceType);

	// Methode utilitaire qui renvoie les ressources cachees a l'utilisateur, elles sont allouees mais pas
	// disponibles : start et reserve
	static longint GetMasterHiddenResource(boolean bIsSequential, int nResourceType);
	static longint GetSlaveHiddenResource(boolean bIsSequential, int nResourceType);

	// Resources machines
	const RMResourceSystem* clusterResources;

	// Exigences de la tache
	const RMTaskResourceRequirement* taskRequirements;

	friend class PLHostClass; // Acces a GetMasterReserve, GetSlaveReserve, GetMasterHiddenResource et
				  // GetSlaveHiddenResource
	friend class PLSolution;
};

class PLHostClassDefinition : public Object
{
public:
	// Constructeur
	PLHostClassDefinition();
	~PLHostClassDefinition();

	// Copie
	void CopyFrom(const PLHostClassDefinition* source);

	void SetMasterClass(boolean bIsMaster);
	boolean GetMasterClass() const;

	// Definition d'une classe de machines
	// Nombre de processeurs utilises
	void SetProcNumber(int nProc);
	int GetProcNumber() const;

	// Memoire Min
	void SetMemoryMin(longint lMemory);
	longint GetMemoryMin() const;

	// Memoire max
	void SetMemoryMax(longint lMemory);
	longint GetMemoryMax() const;

	// Disque Min
	void SetDiskMin(longint lDisk);
	longint GetDiskMin() const;

	// Disque Max
	void SetDiskMax(longint lDisk);
	longint GetDiskMax() const;

	// Ressource Min
	RMResourceContainer* GetResourceMin() const;

	// Ressource Max
	RMResourceContainer* GetResourceMax() const;

	ALString GetSignature() const;

	// Construction d'une nouvelle classe de machine a laquelle appartient le machine passee en parametre
	static PLHostClassDefinition* BuildClassDefinitionForHost(const RMTaskResourceRequirement* requirements,
								  const RMHostResource* host,
								  const LongintVector* lvMemoryBounds,
								  const LongintVector* lvDiskBounds);

	void Write(ostream& ost) const override;
	boolean Check() const override;

protected:
	// Definition de la classe
	RMResourceContainer* resourceMin;
	RMResourceContainer* resourceMax;
	int nProcNumber;
	boolean bIsMasterClass;
};

////////////////////////////////////////////////////////
// classe PLHostClass
// Regroupement de machines dans une classe. Une classe de machines est definie
// par un min et max pour chaque ressources et un nombre de processeurs
class PLHostClass : public Object
{
public:
	// Constructeur
	PLHostClass();
	~PLHostClass();

	// Copie
	PLHostClass* Clone() const;
	void CopyFrom(const PLHostClass* hostClassSource);

	// Initialisation de la classe a partir de sa definition
	// Memoire : appartient a l'appele
	void SetDefinition(PLHostClassDefinition* definition);
	PLHostClassDefinition* GetDefinition() const;

	// Modifie la definition pour qu'elle contiennent toute sles machines de la classe tout en etant la plus petite
	// possible
	void FitDefinition();

	// Nombre de machines appartenant a la classe
	int GetHostNumber() const;

	// Ajout d'une machine dans la classe
	// Memoire *host appartient a l'appelant
	void AddHost(RMHostResource* host);

	// Acces aux hosts
	const ObjectArray* GetHosts() const;

	// Renvoi true si la machine qui heberge le maitre est dans la classe
	boolean IsMasterClass() const;

	// Saturation des ressources de cette classe de machine
	// Renvoie les ressources saturees
	PLSolutionResources* SaturateResource(const RMTaskResourceRequirement* taskRequirements, int nProcNumberOnHost,
					      int nProcNumberOnCluster, boolean bIsSequential) const;

	// Resource logique disponible sur une machine
	longint GetAvaiblableResource(int nRT) const;

	// Tri decroissant des hosts de la classes en regardant la memoire puis le disque
	void SortHosts();

	void Write(ostream& ost) const override;
	boolean Check() const override;

protected:
	// Definition de la classe
	PLHostClassDefinition* definition;

	// Machines qui appartiennent a la classe
	ObjectArray oaHosts;

	boolean bContainsMaster;
};

////////////////////////////////////////////////////////
// classe PLHostClassSolution
// Solution pour une classe de machine
class PLHostClassSolution : public Object
{
public:
	// Constructeur
	PLHostClassSolution();
	~PLHostClassSolution();

	// Copie
	PLHostClassSolution* Clone() const;
	void CopyFrom(const PLHostClassSolution* solutionSource);

	// Classe de machines
	// Memoire : recopie
	void SetHostClass(const PLHostClass* hostClass);
	const PLHostClass* GetHostClass() const;

	// Acces a la solution
	void SetHostCountPerProcNumber(const IntVector* ivHostCountPerProcNumber);
	const IntVector* GetHostCountPerProcNumber() const;

	// Est-ce qu'on peut ajouter un processus sur une des machines qui a nProcHost processus
	boolean AllowsOnMoreProc(int nProcHost) const;

	// Tri decroissant des hosts de la classe sur les ressources (aavec un arrondi a 100 MB pres)
	void SortHosts();

	void Write(ostream& ost) const override;
	boolean Check() const override;

	// Affichage de la solution (nombre de machines pour chaque nombre de processus)
	// Pour le debogage
	void PrintHostCountPerProcNumber() const;

protected:
	PLHostClass* hostClass;

	// Comptage du nombre de machines suivant leur nombre de processus
	// Taille du vecteur hostClass->GetProcNumber() + 1
	// Si tout le contenu est a 0, toutes les machines ont 0 processus
	// Si ivHostCountPerProcNumber.Get(1)==2, 2 machines ont 1 processus
	// Si ivHostCountPerProcNumber.Get(2)==4, 4 machines ont 2 processus
	IntVector ivHostCountPerProcNumber;

	friend class PLSolution; // pour AddProc
};

////////////////////////////////////////////////////////
// Classe PLSolution
// Solution de la gestion des ressources
class PLSolution : public Object
{
public:
	// Constructeur
	PLSolution();
	~PLSolution();

	int CompareTo(const PLSolution*) const;

	PLSolution* Clone() const;
	void CopyFrom(const PLSolution* solutionSource);

	// Acces aux solutions par classe de machine
	// Memoire : appartient a l'appele
	void AddHostSolution(PLHostClassSolution*);
	PLHostClassSolution* GetHostSolutionAt(int i) const;

	// Nombre de classes de machines
	int GetHostClassSolutionNumber() const;

	// Acces au exigences de la tache
	void SetTaskRequirements(const RMTaskResourceRequirement* taskRequirements);
	const RMTaskResourceRequirement* GetTaskRequirements() const;

	// Evaluation de la solution
	// Met a jour tous les attributs qui concernent les ressources utilisees ou manquantes
	void Evaluate(boolean bIsSequential);

	// Ressources utilisees par cette solution
	// Accessibles seulement si la ressource a ete evaluee
	const RMResourceContainer* GetMasterResource() const;
	const RMResourceContainer* GetSlaveResource() const;
	const RMResourceContainer* GetSharedResource() const;
	const RMResourceContainer* GetGlobalResource() const;

	// Nombre de machine utilisees dans cette solution
	// Accessible seulement si la ressource a ete evaluee
	int GetUsedHostNumber() const;

	// Renvoie le nombre de processus utilises dans la solution
	int GetUsedProcessNumber() const;

	// Resources manquantes pour que cette solution soit valide
	// Accessibles seulement si la ressource a ete evaluee
	const RMResourceContainer* GetMissingResources() const;

	// Creation d'une nouvelle solution equivalente a la solution courante ou les classes sont decoupees pour
	// n'avoir que des singletons Memoire : La nouvelle solution appartient a l'appelant
	PLSolution* Expand() const;

	// Renvoi true sil la solution est valide
	boolean IsValid() const;

	void Write(ostream& ost) const override;
	boolean Check() const override;

	// Tri des PLHostClassSolution
	void SortHostClasses();

protected:
	// Calcule si la solution verifie les contraintes minimales en prenant en compte les contraintes globales
	// Renvoie true si la solution est valide
	// dans le cas contraire lMissingResource est mis a jour avec les ressources manquantes
	boolean FitMinimalRequirements(int nRT, longint& lMissingResource, boolean bIsSequential) const;

	// Calcul si la solution (non saturee) verifie les contraintes globales a partir
	// 		- des exigences de la tache
	//		- des contraintes utilisateur
	// Met a jour rcMissingResource, nExtraProcNumber (le nombre de processus en trop)
	// Renvoie true si la solution est valide
	boolean FitResources(boolean bIsSequential);

	// Verifie qu'il n'y a pas trop de processus
	boolean FitProcessNumber() const;

	// Saturation des ressources de la solution
	void SaturateResource(boolean bIsSequential);

	// Ajoute un processus sur une la classe nHostClassIndex, sur une machine qui a nProcNumber processus
	void AddProcessusOnHost(int nHostClassIndex, int nProcNumber);

	// Supprime un processus sur une la classe nHostClassIndex, sur une machine qui a nProcNumber processus*
	// ne peut etre utilisee qu'apres un ajout
	void RemoveProcessusOnHost(int nHostClassIndex, int nProcNumber);

	// Classes de solution qui appartienent a la solution
	ObjectArray oaHostClassSolution;

	// Exigences de la tache
	const RMTaskResourceRequirement* taskRequirements;

	// Est-ce que la solution est valide
	boolean bGlobalConstraintIsValid;

	// Memorisation de bGlobalConstraintIsValid precedent l'appel a AddProcessusOnHost
	boolean bOldGlobalConstraintIsValid;

	// Utiliser pour dans les assertion spour s'assurer que RemoveProcessusOnHost a etet appelee
	// apres AddProcessusOnHost
	boolean bLastIsAdd;

	// Nombre de machines utilisees
	int nUsedHost;

	// Nombre de processus instancies
	int nUsedProcs;

	// Est-ce que la solution a ete evaluee
	boolean bIsEvaluated;

	// Pas d'approximation des ressources utilisees
	const longint lUsedResourcePrecision = 128 * lMB;

	// Ressources allouees au maitre et aux esclaves
	// Ces valeurs sont calculees lors de la saturation (dans la methode evaluate)
	PLSolutionResources* resources;

	// Memorisation de l'attribut resources precedent l'appel a AddProcessusOnHost
	PLSolutionResources* oldResources;

	// Resources manquantes si la solution n'est pas valide
	RMResourceContainer* rcMissingResources;

	// Nombre de master sur la solution
	int nMasterNumber;

	friend class RMParallelResourceManager;
};

////////////////////////////////////////////////////////
// Classe PLSolutionResources
// Resources (maitre, esclave, shared et global) allouees a une solution
class PLSolutionResources : public Object
{
public:
	// Constructeur
	PLSolutionResources();
	~PLSolutionResources();

	PLSolutionResources* Clone() const;
	void CopyFrom(const PLSolutionResources* source);

	// Acces aux ressources de chaque esclave
	void SetSlaveResource(int nRT, longint lValue);
	RMResourceContainer* GetSlaveResource() const;

	// Acces aux ressources de chaque esclave
	void SetMasterResource(int nRT, longint lValue);
	RMResourceContainer* GetMasterResource() const;

	// Acces aux ressources de chaque esclave
	void SetSharedResource(int nRT, longint lValue);
	RMResourceContainer* GetSharedResource() const;

	// Acces aux ressources de chaque esclave
	void SetGlobalResource(int nRT, longint lValue);
	RMResourceContainer* GetGlobalResource() const;

	void Write(ostream& ost) const override;

protected:
	RMResourceContainer* rcSlaveResource;
	RMResourceContainer* rcMasterResource;
	RMResourceContainer* rcSharedResource;
	RMResourceContainer* rcGlobalResource;
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

	// Definition du cluster
	// Appartient à l'appeleant
	void SetCluster(RMResourceSystem* cluster);
	RMResourceSystem* GetCluster() const;

	// Intitule du test pour affichage
	void SetTestLabel(const ALString& sName);

	// Mode verbeux (par defaut : true)
	void SetVerbose(boolean bIsVerbose);
	boolean GetVerbose() const;

	// Memoire au debut du run (par defaut a 8Mo)
	void SetMasterSystemAtStart(longint lMemory);
	void SetSlaveSystemAtStart(longint lMemory);

	// Met la configuration dan l'etat initial
	void Reset();

	//////////////////////////////////////////
	// Configuration des contraintes utilisateurs

	// Acces aux exigences de la tache
	RMTaskResourceRequirement* GetTaskRequirement();

	void SetMemoryLimitPerHost(longint lMemory);
	void SetDiskLimitPerHost(longint lDisk);
	void SetMaxCoreNumberPerHost(int nMax);
	void SetMaxCoreNumberOnSystem(int nMax);

	//////////////////////////////////////////
	// Resolution
	void Solve();

	// Temps, de la resolution
	double GetElapsedTime() const;

protected:
	// Variables qui definissent le systeme
	RMResourceSystem* cluster;
	ALString sName;
	longint lMasterSystemAtStart;
	longint lSlaveSystemAtStart;

	// Contraintes de la tache
	RMTaskResourceRequirement* taskRequirement;

	// Contraintes de l'utilisateur
	longint lMemoryLimitPerHost;
	longint lDiskLimitPerHost;
	int nMaxCoreNumberPerHost;
	int nMaxCoreNumberOnSystem;
	int nMaxCoreNumber;

	boolean bVerbose;
	Timer timer;
};

inline RMResourceContainer* PLSolutionResources::GetMasterResource() const
{
	return rcMasterResource;
}

inline RMResourceContainer* PLSolutionResources::GetSlaveResource() const
{
	return rcSlaveResource;
}

inline RMResourceContainer* PLSolutionResources::GetSharedResource() const
{
	return rcSharedResource;
}

inline RMResourceContainer* PLSolutionResources::GetGlobalResource() const
{
	return rcGlobalResource;
}

inline const RMResourceContainer* PLSolution::GetMasterResource() const
{
	require(bIsEvaluated);
	return resources->GetMasterResource();
}
inline const RMResourceContainer* PLSolution::GetSlaveResource() const
{
	require(bIsEvaluated);
	return resources->GetSlaveResource();
}
inline const RMResourceContainer* PLSolution::GetSharedResource() const
{
	require(bIsEvaluated);
	return resources->GetSharedResource();
}
inline const RMResourceContainer* PLSolution::GetGlobalResource() const
{
	require(bIsEvaluated);
	return resources->GetGlobalResource();
}

inline int PLSolution::GetUsedProcessNumber() const
{
	return nUsedProcs;
}

inline int PLSolution::GetHostClassSolutionNumber() const
{
	return oaHostClassSolution.GetSize();
}

inline PLHostClassSolution* PLSolution::GetHostSolutionAt(int i) const
{
	return cast(PLHostClassSolution*, oaHostClassSolution.GetAt(i));
}

inline int PLHostClass::GetHostNumber() const
{
	return oaHosts.GetSize();
}

inline boolean PLHostClass::IsMasterClass() const
{
	return bContainsMaster;
}

inline const IntVector* PLHostClassSolution::GetHostCountPerProcNumber() const
{
	return &ivHostCountPerProcNumber;
}

inline const PLHostClass* PLHostClassSolution::GetHostClass() const
{
	return hostClass;
}