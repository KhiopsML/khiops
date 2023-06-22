// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWDatabase.h"
#include "PLRemoteFileService.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Service de remplissage d'une liste d'aide pour les valeurs d'un attribut d'une database
class KWDatabaseAttributeValuesHelpList : public Object
{
public:
	// Constructeur
	KWDatabaseAttributeValuesHelpList();
	~KWDatabaseAttributeValuesHelpList();

	// Alimentation d'une liste d'aide de valeurs d'un attribut d'une classe
	// L'attribut doit etre de type simple, et peut utilise ou non
	// Le calcul n'est effectue que si necessaire, en memorisant les caracteristique de la derniere requete, et en
	// indiquant la fraicheur de derniere lecture du fichier dictionnaire
	// Le parametre bUseSelectionAttributes indique s'il faut utiliser ou non l'attribut de selection de la base
	// Les parametres Fill... permettent d'indiquer si la liste doit etre remplie ou videe, selon son type
	void FillAttributeValues(const KWDatabase* inputDatabase, const ALString& sAttributeName,
				 boolean bUseSelectionAttributes, boolean bFillIfContinuous, boolean bFillIfSymbol,
				 UIList* uilAttributeValueHelpList, const ALString& sListAttributeFieldName);

	/////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Test la fraicheur d'une liste l'aide a partir de ses sources d'alimentation
	// Permet de ne declencher son calcul que si necessaire
	boolean NeedsRefresh(const KWDatabase* inputDatabase, const ALString& sAttributeName,
			     boolean bUseSelectionAttributes) const;

	// Memorisation de la fraicheur d'une liste d'aide, en memorisant une image de ses sources d'alimentation
	// Attention: a appeler apres a'limentation (car celle ci modifie temporaire la fraicheur des sources)
	void UpdateFreshness(const KWDatabase* inputDatabase, const ALString& sAttributeName,
			     boolean bUseSelectionAttributes);

	// Memorisation des caracteristiques des listes d'aides pour
	// optimiser leur rafraichissement
	ALString sHelpListLastAttributeName;
	ObjectArray oaHelpListDatabaseUsedFileSpecs;
	KWDatabase dbHelpListLastDatabaseSpec;
	longint lHelpListLastClassHashValue;
	longint lHelpListLastClassDomainHashValue;
};