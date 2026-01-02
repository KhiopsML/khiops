// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "RMResourceConstraints.h"
#include "UserInterface.h"

///////////////////////////////////////////////////////////////////////////////
// Classe RMStandardResourceDriver
// Driver d'acces au resources
// Donne les ressources de la machine courante
//
class RMStandardResourceDriver : public Object
{
public:
	// Acces au driver statique
	static RMStandardResourceDriver* GetDriver();

	// Memoire logique restante effectivement utilisable en octet sur la machine courante (en tenant compte d'un
	// overhead d'allocation, des contraintes utilisateur et de l'interface graphique) en estimant la memoire deja
	// utilisee
	virtual longint GetRemainingAvailableMemory() const;

	// Memoire logique totale effectivement utilisable en octet sur la machine courante (en tenant compte d'un
	// overhead d'allocation et des contraintes utilisateur et de l'interface graphique)
	virtual longint GetTotalAvailableMemory() const;

	// Memoire disponible dans le repertoire des fichiers temporaires
	virtual longint GetTmpDirFreeSpace() const;

	//////////////////////////////////////////////////////////////////
	///// Implementation

	// Conversion de la ressource physique vers la ressource logique (ajout d'un overhead)
	static longint PhysicalToLogical(int nResourceType, longint lPhysicalResource);

protected:
	// Constructeur
	RMStandardResourceDriver();
	~RMStandardResourceDriver();

	// Instance statique du driver, alloue a la demande dans GetDriver()
	static RMStandardResourceDriver* resourceDriver;

	// Instance singleton, dont le destructeur appele systematiquement en fin de programme,
	// se chargera de la destruction du driver courant s'il a ete alloue
	static RMStandardResourceDriver singleton;

	friend class RMResourceManager; // Acces au driver statique
};

inline RMStandardResourceDriver* RMStandardResourceDriver::GetDriver()
{
	if (resourceDriver == NULL)
		resourceDriver = new RMStandardResourceDriver;
	return resourceDriver;
}
