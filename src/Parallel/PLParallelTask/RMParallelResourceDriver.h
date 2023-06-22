// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "RMStandardResourceDriver.h"
#include "RMParallelResourceManager.h"

///////////////////////////////////////////////////////////////////////////////
// Classe RMParallelResourceDriver
// Driver d'acces au resources a l'interieur d'une tache parallele
// Donne les ressources du processus courant (maitre, esclave ou sequentiel)
// en prennant en compte les ressources allouees a la tache parallele
//
// Note technique :
// La memoire disponible pour un processus GetRemainingAvailableMemory() est bornee par
//	-Ms: la memoire disponible sur le systeme
//	-Ma: la memoire adressable (MemGetAdressablePhysicalMemory())
//	-Mu: la limite utilisateur (RMResourceConstraints::GetResourceLimit())
// On garde une reserve de securite R qui est soustraite a la limite utilisateur (MemGetPhysicalMemoryReserve()+
// MemGetAllocatorReserve()+ GetUserInterfaceMemoryReserve()) D'autre part on prend egalement en compte la heap utilisee
// au lancement de l'application, notee Mc. En effet si l'utilisateur veut utiliser 2Go, il faut que l'application
// puisse allouer 2Go + Mc. La memoire allouee a un processus (resultat de la methode ComputeGrantedResources)  notee Mg
// tient compte de ces bornes, de la reserve et de la memoire courante (Mg + Mc < min(Mu,Ma,Ms)-R) Dans une tache
// parallele la memoire disponible est simplement obtenue par Mg-HeapCourante (Mg tient compte de la memoire au
// demarrage Mc et de la reserve)
//
// En dehors des taches paralleles, la memoire disponible est obtenue par min(Ms,Mu) - R - HeapCourante
//
class RMParallelResourceDriver : public RMStandardResourceDriver
{
public:
	// Acces au driver statique
	static RMParallelResourceDriver* GetDriver();

	// Memoire restante effectivement utilisable en octet par le processus courant (en tenant compte d'un overhead
	// d'allocation et des contraintes utilisateur) en estimant la memoire deja utilisee
	longint GetRemainingAvailableMemory() const override;

	// Memoire totale effectivement utilisable en octet par le processus courant (en tenant compte d'un overhead
	// d'allocation et des contraintes utilisateur)
	longint GetTotalAvailableMemory() const override;

	// Memoire disponible dans le repertoire des fichiers temporaires pour le processus courant
	longint GetTmpDirFreeSpace() const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Renvoi la ressource logique disponible en partant de la resource logique courante lCurrentResource et en ne
	// depassant pas les contraintes de l'application
	longint GetRemainingAvailableResource(Resource resource, longint lCurrentResource) const;

	// Constructeur
	RMParallelResourceDriver();
	~RMParallelResourceDriver();

	// Ressources allouees pour une tache parallele
	static RMTaskResourceGrant* grantedResources;

	// Instanc statique du driver parallele
	static RMParallelResourceDriver* parallelResourceDriver;

	// Instance singleton, dont le destructeur appele systematiquement en fin de programme,
	// se chargera de la destruction du driver courant s'il a ete alloue
	static RMParallelResourceDriver singleton;

	friend class PLMPIMaster;    // Pour serialisation
	friend class PLMPISlave;     // Pour de-serialisation
	friend class PLParallelTask; // Pour acces direct a grantedResources
};

inline RMParallelResourceDriver* RMParallelResourceDriver::GetDriver()
{
	if (parallelResourceDriver == NULL)
		parallelResourceDriver = new RMParallelResourceDriver;
	return parallelResourceDriver;
}