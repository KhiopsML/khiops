// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Standard.h"
#include "RMResourceManager.h"

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
// globale (ou repartie)

class RMTaskResourceRequirement : public Object
{
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
	// reparti proportionnellement les exigences : balanced Soit on donne la maximum au maitre et on reparti le
	// reste : masterPreferred Soit on donne le maximum aux esclaves et on reparti le reste: slavePrefered (par
	// defaut)
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

inline longint RMTaskResourceRequirement::GetSlaveMin(int nResourceType) const
{
	return slaveRequirement->GetResource(nResourceType)->GetMin();
}
inline longint RMTaskResourceRequirement::GetSlaveMax(int nResourceType) const
{
	return slaveRequirement->GetResource(nResourceType)->GetMax();
}
inline longint RMTaskResourceRequirement::GetMasterMin(int nResourceType) const
{
	return masterRequirement->GetResource(nResourceType)->GetMin();
}
inline longint RMTaskResourceRequirement::GetMasterMax(int nResourceType) const
{
	return masterRequirement->GetResource(nResourceType)->GetMax();
}
inline longint RMTaskResourceRequirement::GetSharedMin(int nResourceType) const
{
	return sharedRequirement->GetResource(nResourceType)->GetMin();
}
inline longint RMTaskResourceRequirement::GetSharedMax(int nResourceType) const
{
	return sharedRequirement->GetResource(nResourceType)->GetMax();
}
inline longint RMTaskResourceRequirement::GetSlaveGlobalMin(int nResourceType) const
{
	return globalSlaveRequirement->GetResource(nResourceType)->GetMin();
}
inline longint RMTaskResourceRequirement::GetSlaveGlobalMax(int nResourceType) const
{
	return globalSlaveRequirement->GetResource(nResourceType)->GetMax();
}

inline RMResourceRequirement* RMTaskResourceRequirement::GetMasterSystemAtStart()
{
	return &masterSystemAtStart;
}

inline RMResourceRequirement* RMTaskResourceRequirement::GetSlaveSystemAtStart()
{
	return &slaveSystemAtStart;
}
