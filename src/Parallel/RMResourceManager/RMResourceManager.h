// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"
#include "UserInterface.h"
#include "RMResourceConstraints.h"
#include "RMResourceSystem.h"
#include "RMStandardResourceDriver.h"

class RMResourceManager;   // Acces aux ressources disponibles et calcul des ressources en fonction des exigences
class RMResourceGrant;     // Ressources allouees a un processus
class RMTaskResourceGrant; // Ressources alouees pour une tache. Resultat du RMResourceManager qui contient un ensemble
			   // de RMResourceGrant
class RMResourceRequirement; // Exigences sur le disque et la memoire
class RMPhysicalResource;    // Contrainte sur l'utilisation d'une ressource generique (memoire ou disque)

//////////////////////////////////////////////////////////////////////////
// Classe RMResourceManager
// Cette classe a deux roles :
//	- Acces aux ressources disponibles sur la machine courante (memoire et disque)
//	- Gestionnaire des ressources d'un systeme mono-machine avec la methode ComputeGrantedResources
class RMResourceManager : public Object
{
public:
	// Constructeur
	RMResourceManager();
	~RMResourceManager();

	// Acces aux ressources du systeme
	static RMResourceSystem* GetResourceSystem();

	// Nombre de coeurs du systeme
	static int GetPhysicalCoreNumber();

	// Nombre de processus logiques (MPI) lances sur le systeme
	static int GetLogicalProcessNumber();

	// Memoire disponible dans le repertoire des fichiers temporaires
	static longint GetTmpDirFreeSpace();

	///////////////////////////////////////////
	// Il y a d'une part la memoire physique, correspondant a ce qui est disponible sur les machines,
	// qui concerne l'utilisateur de facon externe (RAM installee, limite RAM utulisateur, RAM consommee...).
	// D'autre part, la memoire "logique" correspond a ce qui est theoriquement necessaire pour les algorithmes
	// (a base de sizeof, de taille de tableaux, structures...), et qui permet de dimnessionner les besoins en
	// ressources pour le developpeur (Ressource requirements et Grants...).
	// La memoire physique est genarelement geree dans la bibliotheque Norm, qui se preoccupe des ressources
	// systemes, alors que la memoire logique est l'objet de la gestion des ressources pour les taches. Un overhead
	// de gestion memoire est utilise pour passer de la memoire theorique a la memoire physique.

	// Memoire logique restante effectivement utilisable en octet sur la machine courante (memoire disponible -
	// memoire de la Heap) Note : en parallele c'est la memoire utilisable pour le processus courant
	static longint GetRemainingAvailableMemory();

	// Memoire logique totale effectivement utilisable en octet sur la machine courante (en tenant compte d'un
	// overhead d'allocation et des contraintes utilisateur) Note : en parallele c'est la memoire totale allouee au
	// processus
	static longint GetTotalAvailableMemory();

	// Memoire logique utilisee en octet (en tenant compte d'un overhead d'allocation)
	// Note : en parallele c'est la memoire utilisee par le processus courant
	static longint GetHeapLogicalMemory();

	// Memoire logique minimum necessaire au lancement d'une application (IHM + reserve allocateur)
	// A prendre en compte dans la calcul de la memoire necessaire a l'execution d'une methode
	// (On doit avoir NecessaryMemory = <method>Requirement + GetSystemMemoryRequirement <
	// GetRemainingAvailableMemory)
	static longint GetSystemMemoryReserve();

	// Message de warning a appeler pour prevenir du depassement delibere (et risque) des ressources memoire
	static void DisplayIgnoreMemoryLimitMessage();

	// Message utilisateur pour indiquer qu'il manque de la memoire (message a concatener en fin du message d'erreur
	// d'origine)
	//  entree: memoire logique necessaire
	//  sortie: message avec memoire physique manquante en MB, en tenant compte de l'overhead de l'allocateur, de la
	//  memoire utilise et
	//          de la memoire utilisateur disponible
	static ALString BuildMissingMemoryMessage(longint lNecessaryMemory);

	// Conversion d'une quantite de memoire logique en chaine de caractere, arrondi, avec l'unite la plus adaptee
	// (MB ou GB), en tenant compte de l'overhead de l'allocateur, pour avoir un message utilisateur portant sur la
	// memoire physique
	static ALString ActualMemoryToString(longint lMemory);

	// Message utilisateur pour suggerer de changer le parametre de limite memoire physique
	static ALString BuildMemoryLimitMessage();

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Modification du driver de ressources (avec destruction du driver actuel)
	static void SetResourceDriver(RMStandardResourceDriver* driver);

	// Acces au driver de ressources
	static RMStandardResourceDriver* GetResourceDriver();

protected:
	// Ressources du systeme, initialisees dans la methode PLTaskDriver::InitializeSystemResource
	static RMResourceSystem resourceSystem;

	// Driver de gestion des ressource (sequentiel ou parallele)
	static RMStandardResourceDriver* resourceDriver;

	friend class PLParallelTask;
};

//////////////////////////////////////////////////////////////////////////
// Classe RMResourceRequirement
// Cette classe represente les exigences utilisateurs sur les ressources systeme.
// L'utilisateur peut fixer la memoire requise minimum et l'espace disque minimum.
// Avec la semantique que si ces contraintes min ne sont pas respectees, la tache ne
// peut pas etre executer. L'utilisateur peut egalement fixer des bornes max. Dans ce cas
// la semantique est moins forte : il n'est pas utile d'utiliser plus de resources que le max
// mais la tache peut s'executer.
// Par defaut
//			- la memoire min est fixee a 0
//			- l'espace disque min est fixe a 0
//			- la memoire max est fixee a INF
//			- l'espace disque max est fixe a INF
class RMResourceRequirement : public Object
{
public:
	// Constructeur
	RMResourceRequirement();
	~RMResourceRequirement();

	// Copie et duplication
	RMResourceRequirement* Clone() const;
	void CopyFrom(const RMResourceRequirement* requirement);

	// Acces aux exigences sur la memoire
	RMPhysicalResource* GetMemory() const;

	// Acces aux exigences sur le disque
	RMPhysicalResource* GetDisk() const;

	// Acces generique
	RMPhysicalResource* GetResource(int nResourceIndex) const;

	// Verifie que les exigences sont coherentes
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteDetails(ostream& ost) const;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	ObjectArray oaResources; // Tableau de RMPhysicalResource*
	friend class PLShared_ResourceRequirement;
};

//////////////////////////////////////////////////////////////////////////
// Classe RMPhysicalResource
// Specification des limites d'utilisation d'une ressource generique
class RMPhysicalResource : public Object
{
public:
	// Constructeur, initialisation a 0 et +inf
	RMPhysicalResource();
	~RMPhysicalResource();

	// Copie et duplication
	RMPhysicalResource* Clone() const;
	void CopyFrom(const RMPhysicalResource* resource);

	// Ressource maximum
	void SetMax(longint lMax);
	longint GetMax() const;

	// Ressource minimum
	void SetMin(longint lMin);
	longint GetMin() const;

	// Ajoute un delta au max (resp min) a la valeur courante
	// tout en gerant le depassement au dessus de LLONG_MAX
	void UpgradeMin(longint lDeltaMin);
	void UpgradeMax(longint lDeltaMax);

	// Ressource minimum = ressource maximum
	void Set(longint lMinMax);

	// Affichage
	void Write(ostream& ost) const override;
	void WriteDetails(ostream& ost) const;

	// Verifie que le min est plus petit que le max
	boolean Check() const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	longint lMin;
	longint lMax;
};
