// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Attribute.h"
#include "ManagedObjectTable.h"
#include "QueryServices.h"

////////////////////////////////
// Classe AttributeTable
class AttributeTable : public ManagedObjectTable
{
public:
	// Constructeur
	AttributeTable();
	~AttributeTable();

	//// Requetes portant sur les attributs
	QueryServices* RankServices();

	// Methode de controle d'integrite
	boolean Check() const override;

	//////////////////////////////////////////
	// Methodes utilitaires de generation

	// Test de la presence de champs cles
	boolean NoKeyFields();

	// Nombre de champs de champs cles
	int GetKeyFieldsNumber();

	// Test s'il est necessaire de fabriquer la cle
	boolean IsKeyBuilt();

	// Nombre de champs permanents
	int GetPermanentFieldsNumber();

	// Nombre de champs visibles
	int GetVisibleFieldsNumber();

	// Nombre de champs ayant un style defini
	int GetStyleFieldsNumber();

	// Test de la presence de champs avec statistiques
	boolean NoStats();

	///// Implementation
protected:
	QueryServices qsRankServices;
	int nRankServicesFreshness;
};
