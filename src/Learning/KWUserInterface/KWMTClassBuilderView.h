// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWClassDomain.h"

////////////////////////////////////////////////////////////
// Classe KWMTClassBuilderView
//    Construction d'un dictionnaire multi-table
//    En fait, d'un dictionnaire racine lie en relation 0-n avec
//     un dictionnaire (non racine et ayant une cle)
class KWMTClassBuilderView : public UIConfirmationCard
{
public:
	// Constructeur
	KWMTClassBuilderView();
	~KWMTClassBuilderView();

	// Parametrage du dictionnaire secondaire
	void SetSecondaryDictionaryName(const ALString& sValue);
	const ALString& GetSecondaryDictionaryName() const;

	// Initialisation des parametres par defaut
	void InitDefaultParameters();

	// Construction du dictionnaire multi-tables
	// Message d'erreur si echec
	boolean BuildMultiTableClass();

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Memorisation des donnees gerees par la fiche
	ALString sSecondaryDictionaryName;
	UIList* classNameList;
};
