// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWDatabase.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Service de remplissage d'une liste d'aide pour les attributs d'une classe
class KWClassAttributeHelpList : public Object
{
public:
	// Constructeur
	KWClassAttributeHelpList();
	~KWClassAttributeHelpList();

	// Service d'alimentation d'une liste d'aide de nom d'attribut d'une classe
	// On peut preciser les types d'attribut a filtrer
	void FillAttributeNames(const ALString& sClassName, boolean bContinuousAttributes, boolean bSymbolAttributes,
				boolean bNativeAttributesOnly, boolean bUsedAttributesOnly,
				UIList* uilAttributeNameHelpList, const ALString& sListAttributeFieldName);

	/////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Test la fraicheur d'une liste l'aide a partir de ses sources d'alimentation
	// Permet de ne declencher son calcul que si necessaire
	boolean NeedsRefresh(const ALString& sClassName) const;

	// Memorisation de la fraicheur d'une liste d'aide, en memorisant une image de ses sources d'alimentation
	// Attention: a appeler apres a'limentation (car celle ci modifie temporaire la fraicheur des sources)
	void UpdateFreshness(const ALString& sClassName);

	// Memorisation des caracteristiques des listes d'aides pour
	// optimiser leur rafraichissement
	ALString sHelpListLastClassName;
	longint lHelpListLastClassHashValue;
	longint lHelpListLastClassDomainHashValue;
};
