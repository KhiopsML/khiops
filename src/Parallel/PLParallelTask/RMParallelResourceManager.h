// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "RMResourceManager.h"
#include "RMResourceSystem.h"
#include "RMResourceConstraints.h"
#include "PLParallelTask.h"
#include "SortedList.h"

///////////////////////////////////////////////////////////
// Gestion des ressources d'un systeme parallele
class RMParallelResourceManager; // Gestionnaire de ressources, qui alloue au mieux les resources en fonction des
				 // demandes
class RMTaskResourceRequirement; // Ressources demandees pour une tache
class RMTaskResourceGrant; // Ressources alouees pour une tache. Resultat du RMResourceManager qui contient un ensemble
			   // de RMResourceGrant
class RMResourceGrant;     // Les ressource allouees a un processus
class PLTestCluster;       // Classe de test de l'allocation des ressources
class RMResourceContainer; // Classe qui stocke une valeur pour chaque type de ressource

// Classes utilisees dans l'algorithme de resolution de RMParallelResourceManager
class PLHostSolution;    // Une solution possible pour une machine
class PLClusterSolution; // Une solution possible pour le cluster (contient un ensemble de PLHostSolution). Cette classe
			 // a les memes fonctions que RMTaskResourceGrant elles seront a merger (TODO)
class PLClusterResourceQuality; // Qualite q'un PLClusterSolution : permet de comparer 2 solutions

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

	// Remise a 0
	void Initialize();

	// Affichage
	void Write(ostream& ost) const override;

protected:
	LongintVector lvResources;
};

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
//			Il faudra donc veiller a initialiser le min ET le max (pour eviter que min > max ).
//			On peu utiliser la methode Set() qui affecte le min et le max en meme temps
//		-	Les exigences GetSlaveRequirement et GetGlobalSlaveRequirement sont prises en compte en meme
//			temps dans les ressources allouees, c'est a dire que la methode
//			RMTaskResourceGrant::GetMinSlaveMemory() renvoie la memoire dediee a l'esclave + la memoire
//          globale (ou repartie)

class RMTaskResourceRequirement : public Object
{
	// TODO ajouter le nombre de fichier ouverts en meme temps : openedFileNumber.
	// le gestionnaire ajustera le nombre d'esclave par machine avec le nombre de fichiers ouverts necessaires par
	// SlaveProcess
public:
	// Constructeur
	RMTaskResourceRequirement();
	~RMTaskResourceRequirement();

	// Copie et duplication
	RMTaskResourceRequirement* Clone() const;
	void CopyFrom(const RMTaskResourceRequirement* trRequirement);

	// Politique d'allocation des ressources
	enum ALLOCATION_POLICY
	{
		balanced,
		masterPreferred,
		slavePreferred,
		globalPreferred,
		unknown
	};

	// Politique de parallelisation
	enum PARALLELISATION_POLICY
	{
		horizontal,
		vertical,
		policy_number
	};

	// Politique d'allocation de la memoire restante apres avoir affecte le minimum a chaque processus.
	// Il ya 4 exigences a saturer : maitre, esclave, variables partagees, et exigence globale dur les esclaves.
	// L'algorithme reparti le surplus proportionellement aux exigences : il calcule un pourcentage p quiest le meme
	// pour chaque type d'exigence. Le surplus pour chaque exigence est p * (exigenceMax-ExigenceMin) Soit on
	// reparti proportionnelement les exigences : balanced Soit on donne la maximum au maitre et on reparti le reste
	// : masterPreferred Soit on donne le maximum aux esclaves et on reparti le reste: slavePrefered (par defaut)
	void SetMemoryAllocationPolicy(ALLOCATION_POLICY policy);
	ALLOCATION_POLICY GetMemoryAllocationPolicy() const;

	// Meme chose que pour la memoire
	void SetDiskAllocationPolicy(ALLOCATION_POLICY policy);
	ALLOCATION_POLICY GetDiskAllocationPolicy() const;

	// Politique d'allocation des machines : soit on privilegie une parallelisation horizontale, auquel cas
	// on utilisera le plus de machines possible (jusqu'a 1 processus par machine).
	// Soit on prefere la parallelisation verticale, qui concentre les processus sur un minimum de machines.
	// defaut : horizontal
	void SetParallelisationPolicy(PARALLELISATION_POLICY policy);
	PARALLELISATION_POLICY GetParallelisationPolicy() const;

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

	// Affichage detaille
	void WriteDetails(ostream& ost) const;

	// Transformation de la politique en chaine de caractere pour affichage
	static ALString ResourcePolicyToString(int policy);
	static ALString ParallelisationPolicyToString(int policy);

	// Verifie que toutes les exigences sont coherentes
	boolean Check() const override;

	// Methodes utilitaires pour faciliter l'utilisation
	longint GetSlaveMin(int nResourceType) const;
	longint GetSlaveMax(int nResourceType) const;
	longint GetMasterMin(int nResourceType) const;
	longint GetMasterMax(int nResourceType) const;
	longint GetSharedMin(int nResourceType) const;
	longint GetSharedMax(int nResourceType) const;
	longint GetSlaveGlobalMin(int nResourceType) const;
	longint GetSlaveGlobalMax(int nResourceType) const;

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Politique d'allocation pour la ressource de type nResourceType
	void SetResourceAllocationPolicy(int nResourceType, ALLOCATION_POLICY policy);
	ALLOCATION_POLICY GetResourceAllocationPolicy(int nResourceType) const;

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
	PARALLELISATION_POLICY parallelisationPolicy;

	friend class RMParallelResourceDriver;  // Acces aux GetMasterSystemRequirement et GetSlaveSystemRequirement
	friend class RMParallelResourceManager; // Acces aux GetMasterSystemRequirement et GetSlaveSystemRequirement
	friend class PLParallelTask;            // Acces aux GetMasterSystemRequirement et GetSlaveSystemRequirement
};

////////////////////////////////////////////////////////
// classe PLClusterSolution
// Une solution possible du probleme avec la valeur du critere de selection
// TODO c'est la classe RMTaskResourceGrant, il faudra merger
class PLClusterSolution : public Object
{
public:
	// Constructeur
	PLClusterSolution();
	~PLClusterSolution();

	// Copie et duplication
	void CopyFrom(const PLClusterSolution* clusterSolution);
	PLClusterSolution* Clone() const;

	// Acces a la solution de chaque host
	const ObjectDictionary* GetHostSolutions() const;

	void AddHostSolution(PLHostSolution* hostSolution);

	// Modification de la solution  par ajout d'un processus sur la machine sHostName
	// Renvoie false si l'ajout d'un processeur n'est pas possible et dans ce cas la solution n'est pas modifiee
	boolean AddProcessorAt(const ALString& sHostName);

	// Modification de la solution  par suppression d'un processus sur la machine sHostName
	// Renvoie false si l'ajout d'un processeur n'est pas possible et dans ce cas la solution n'est pas modifiee
	boolean RemoveProcessorAt(const ALString& sHostName);

	// Modification de la solution par echange de processus entre les machines sHostNameFrom et sHostNameTo
	// Renvoie false si l'ajout ou la suppression n'un processeur n'est pas possible sur au moins un des hosts
	// et dans ce cas la solution n'est pas modifiee
	boolean SwitchProcessor(const ALString& sHostNameFrom, const ALString& sHostNameTo);

	// Exigences de la taches
	void SetRequirement(const RMTaskResourceRequirement* taskRequirements);
	const RMTaskResourceRequirement* GetRequirements() const;

	// Saturation des ressources allouees, a partir
	//		- des ressources disponibles (odHostsResources)
	//		- des exigences de la tache (taskRequirements)
	//		- des contraintes utilisateurs (acces statique a RMResourceConstraints)
	void SaturateResources(boolean bIsSequential);

	PLClusterResourceQuality* GetQuality() const;

	// Nombre de host de la solution du cluster
	int GetHostSolutionNumber() const;

	// Nombre de processus de la solution du cluster
	int GetProcessSolutionNumber() const;

	// Resources utilisees sur tout le cluster
	longint GetResourceUsed(int nRT) const;

	// Ressources manquantes
	void SetMissingResource(int nRt, longint lMissingResource);
	longint GetMissingResources(int nRT) const;

	// Machine sur laquelle manque des ressources
	// en l'occurence c'est le master
	void SetMissingHostName(const ALString& sHostName);
	ALString GetHostNameMissingResources() const;

	// Resources allouees au maitre et aux esclaves
	// Utilisable uniquement si la solution est saturee
	longint GetSlaveResource(int nRT) const;
	longint GetMasterResource(int nRT) const;
	longint GetSharedResource(int nRT) const;
	longint GetGlobalResource(int nRT) const;

	// Calcule si la solution verifie les contraintes minimales en prenant en compte les contraintes globales
	// Renvoie true si la solution est valide
	// dans le cas contraire lMissingResource est mis a jour avec les ressources manquantes
	boolean FitMinimalRequirements(int nRT, longint& lMissingResource) const;

	// Calcul si la solution (non saturee) verifie les contraintes globales a partir
	// 		- des exigences de la tache
	//		- des contraintes utilisateur
	// Verifie qu'il y a au moins un processus sur la machine maitre
	// Met a jour rcMissingResource, nExtraProcNumber (le nombre de processus en trop)
	// Renvoie true si la solution est valide
	boolean FitGlobalConstraint(RMResourceContainer* rcMissingResourceForGlobalConstraint,
				    int& nExtraProcNumber) const;

	// Nettoyage
	void Clean();

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Dictionnaire de PLHostSolution, clef = hostName
	mutable ObjectDictionary odHostSolution;

	// Critere qui evalue la qualité de la solution
	PLClusterResourceQuality* quality;

	// Ressources allouees au maitre et aux esclaves
	// Ces valeurs sont calculees lors de la saturation
	RMResourceContainer rcSlaveResource;
	RMResourceContainer rcMasterResource;
	RMResourceContainer rcSharedResource;
	RMResourceContainer rcGlobalResource;

	// Resources manquantes pour avoir une solution valide
	// Calculee pendant la recherche d'une solution sequentielle
	RMResourceContainer rcMissingResource;

	// Machine sur laquelle il manque des ressources
	// C'est forcement le master host
	ALString sHostMissingResource;

	// Exigences de la tache
	const RMTaskResourceRequirement* taskRequirements;

	// Flag pour savoir si la methode FitGlobalConstraint a deja ete appelee
	mutable boolean bGlobalConstraintIsEvaluated;

	// Memorisation du calcul de FitGlobalConstraint
	mutable boolean bGlobalConstraintIsValid;

	// Nombre de processus de la solution
	int nProcessNumber;

	// Nombre de host contenant au moins 1 processus
	int nHostNumber;

	friend int CompareClusterSolution(const void* elem1, const void* elem2);
};

int CompareClusterSolution(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////
// Classe RMParallelResourceManager : resoud le probleme d'allocation des ressources sur un cluster de machines.
// En entree :
// 	- un ensemble de machine et leurs ressources disponibles (RAM, disk et CPU)
//	- Les exigences de la tache parallele
//	- Les contraintes utilisateur (celles-ci sont accessible de maniere statique dans la classe
// RMResourceConstraints)
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

	// Methode de test
	static void Test();

	////////////////////////////////////////////////////////
	//// Implementation

protected:
	static void BuildGrantedResources(const PLClusterSolution*,
					  const RMTaskResourceRequirement* resourceRequirement,
					  RMTaskResourceGrant* grantedResource);

	// Nettoyage complet pour se remettre dans l'etat initial
	void Clean();

	// Exigences de la taches
	void SetRequirement(const RMTaskResourceRequirement* taskRequirements);
	const RMTaskResourceRequirement* GetRequirements() const;

	// Ressources du cluster
	void SetResourceCluster(const RMResourceSystem* cluster);
	const RMResourceSystem* GetResourceCluster() const;

	// Resolution du probleme et renvoi de la mailleure solution
	// Renvoie systematiquement la meilleure solution, potentiellement vide si impossible
	// Memoire: l'objet en retour appartient a l'appele
	const PLClusterSolution* Solve();

	// 1ere phase de la methode Solve()
	// Algorithme glouton qui construit une premiere solution qui repond aux contraintes globales
	// (PLClusterSolution) La meilleure solution est alors initialisee
	void GreedySolve();

	// 2eme phase de la methode Solve()
	// Amelioration de la meilleure solution trouvee par GreedySolve
	void PostOptimize();

	// Construction d'une solution sequentielle, dans le cas d'un echec de l'algorithme greedy
	void SequentialSolve();

	// Verifie que les ressource sgranted remplissent les exigences de la tache et les contraintes utilisateurs
	boolean Check(RMTaskResourceGrant* taskResourceGrant) const;

	// Meilleure solution calculee
	PLClusterSolution bestSolution;

	// Resources machines
	const RMResourceSystem* clusterResources;

	// Exigences de la tache
	const RMTaskResourceRequirement* taskRequirements;

	friend class PLClusterResourceQuality;
};

//////////////////////////////////////////////////////
// Classe PLHostSolution
// Une solution possible pour une machine
class PLHostSolution : public Object
{
public:
	// Constructeur : par defaut la solution nulle : 0 procs, 0 RAM, 0 CPU
	PLHostSolution();
	~PLHostSolution();

	// Copie et duplication
	void CopyFrom(const PLHostSolution* clusterSolution);
	PLHostSolution* Clone() const;

	// Parametrage du host concerne
	void SetHost(const RMHostResource* host);
	const RMHostResource* GetHost() const;

	// Exigences de la taches
	void SetRequirement(const RMTaskResourceRequirement* taskRequirements);
	const RMTaskResourceRequirement* GetRequirements() const;

	// Ensemble des machines du systeme
	void SetClusterResources(const RMResourceSystem* resourcesCluster);
	const RMResourceSystem* GetClusterResources() const;

	// Acces au nombre de processeurs
	void SetProcNumber(int nProcNumber);
	int GetProcNumber() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////
	//// Implementation

	// Calcule le nombre de processus maximum qui peuvent s'executer (sans prendre en compte les contraintes
	// globales)
	int ComputeMaxProcessNumber(boolean bSequential) const;

	// Calcule les ressources utilisees sur une machine pour une solution aux exigences de la tache.
	//	lSlaveAllocatedResource et lMasterAllocatedResource correspondent aux ressources allouees pour le maitre
	// et chaque esclave
	//     lSlaveAllocatedResource contient SlaveMin,
	//     sharedMin et GlobalMin
	// 	Renvoie les ressources utilisees sur la machine
	// 	    lSlaveResourceUsed est mis à jour avec les ressources utilisees par chaque esclave
	// 		lMasterResourceUsed est mis a jour avec les ressources utilisees par le maitre
	longint ComputeAllocation(boolean bIsSequential, int nRT, longint lSlaveAllocatedResource,
				  longint lMasterAllocatedResource) const;

	// Methode utilitaire qui renvoie la reserve par processus (reserve pour l'allocateur + reserve de memoire +
	// java+ serialisation)
	static longint GetMasterReserve(int nResourceType);
	static longint GetSlaveReserve(int nResourceType);

	// Methode utilitaire qui renvoie les ressources cachees a l'utilisateur, elles sont allouees mais pas
	// disponibles : start et reserve
	static longint GetMasterHiddenResource(boolean bIsSequential, int nResourceType);
	static longint GetSlaveHiddenResource(boolean bIsSequential, int nResourceType);

protected:
	mutable int nProcNumber;

	// Host auquel appartient la solution
	const RMHostResource* host;

	// Exigences de la tache
	const RMTaskResourceRequirement* taskRequirements;

	// Dictionnaire des ressources machines (clef: hostName)
	const RMResourceSystem* clusterResources;
};

////////////////////////////////////////////////////////
// Classe PLClusterResourceQuality
// critere hierarchique de la qualité d'une solution valide a l'allocation de ressources sur un cluster
class PLClusterResourceQuality : public Object
{
public:
	// Constructeur
	PLClusterResourceQuality();
	~PLClusterResourceQuality();

	// Copie et duplication
	void CopyFrom(const PLClusterResourceQuality* quality);
	PLClusterResourceQuality* Clone() const;

	// Evaluation du critere de qualite pour
	//		-	une solution donnee : solution
	//		-	les exigences de la tache : solution->taskRequirements
	// 		-	les ressources du cluster : solution->odHostSolution
	//		-	les contraintes utilisateurs (acces statique a RMResourceConstraints)
	virtual void Evaluate();

	void SetSolution(PLClusterSolution*);
	PLClusterSolution* GetSolution() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Fonction de comparaison utilisee dans la methode CompareClusterSolution
	virtual int Compare(const PLClusterResourceQuality* otherResourceQuality);

	// Remise dans l'etat initial
	virtual void Clean();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs utilises lorsque la contrainte globale est valide
	int nProcNumber; // nombre de processus utilisés (maximiser)
	int nSpread; // etalement horizontal ou vertical (nombre de machines utilisees a minimiser ou maximiser suivant
		     // la politique)
	const longint lUsedResourcePrecision = 128 * lMB; // Pas d'approximation des ressources utilisees

	// Attributs lorsque la contrainte  globale n'est pas valide
	RMResourceContainer rcMissingResource; // memoire et disque manquants
	int nExtraProcessNumber;               // nombre de processus en trop

	boolean bIsEvaluated;

	// Est-ce que la contrainte est valide
	boolean bGlobalConstraintIsValid;

	PLClusterSolution* solution;

	// Methodes friend pour l'acces a bIsEvaluated
	friend boolean PLClusterSolution::AddProcessorAt(const ALString& sHostName);
	friend boolean PLClusterSolution::RemoveProcessorAt(const ALString& sHostName);
	friend boolean PLClusterSolution::SwitchProcessor(const ALString& sHostNameFrom, const ALString& sHostNameTo);
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

protected:
	// Variables qui definissent le systeme
	RMResourceSystem* cluster;
	ALString sName;
	longint lMasterSystemAtStart;
	longint lSlaveSystemAtStart;

	// Contraintes de la tache
	RMTaskResourceRequirement taskRequirement;

	// Contraintes de l'utilisateur
	longint lMemoryLimitPerHost;
	longint lDiskLimitPerHost;
	int nMaxCoreNumberPerHost;
	int nMaxCoreNumberOnSystem;
	int nMaxCoreNumber;
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

	// Memoire et disque du maitre
	longint GetMasterMemory() const;
	longint GetMasterDisk() const;
	longint GetMasterResource(int nResourceType) const;

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
	void SetMasterResource(int nRT, longint lResource);
	void SetSharedResource(int nRT, longint lResource);
	void SetGlobalResource(int nRT, longint lResource);
	void SetSlaveResource(int nRT, longint lResource);
	void SetHostCoreNumber(const ALString& sHostName, int nProcNumber);

	ObjectArray oaResourceGrant;              // Tableau de RMResourceGrant
	ObjectArray oaResourceGrantWithRankIndex; // Tableau de RMResourceGrant indexes par leur rang
	RMResourceContainer rcMissingResources;

	// Vrai si il n'y a pas assez de ressources a cause des contraintes par processus
	boolean bMissingResourceForProcConstraint;

	// Vrai si il n'y a pas assez de ressource sa cause des contraintes globales sur les esclaves
	boolean bMissingResourceForGlobalConstraint;

	// Host sur lequel il manque de la memoire dans la cas de contraintes globales
	ALString sHostMissingResource;

	// Ressources allouees
	// Redondant avec oaResourceGrant car depuis le nouvel algorithme
	// on a les memes ressources allouees sur tous les esclaves
	// TODO en attente de refactoring
	RMResourceContainer rmMasterResource;
	RMResourceContainer rmSlaveResource;
	RMResourceContainer rmSharedResource;
	RMResourceContainer rmGlobalResource;

	// Nombre de processus par host
	ObjectDictionary odHostProcNumber;

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

//////////////////////////////
// Methode en inline

inline int PLClusterSolution::GetHostSolutionNumber() const
{
	return nHostNumber;
}

inline int PLClusterSolution::GetProcessSolutionNumber() const
{
	return nProcessNumber;
}

inline longint PLClusterSolution::GetSlaveResource(int nRT) const
{
	return rcSlaveResource.GetValue(nRT);
}

inline longint PLClusterSolution::GetMasterResource(int nRT) const
{
	return rcMasterResource.GetValue(nRT);
}

inline longint PLClusterSolution::GetSharedResource(int nRT) const
{
	return rcSharedResource.GetValue(nRT);
}

inline longint PLClusterSolution::GetGlobalResource(int nRT) const
{
	return rcGlobalResource.GetValue(nRT);
}

inline void PLHostSolution::SetProcNumber(int nNumber)
{
	nProcNumber = nNumber;
}

inline int PLHostSolution::GetProcNumber() const
{
	return nProcNumber;
}

inline PLClusterResourceQuality* PLClusterSolution::GetQuality() const
{
	return quality;
}
