// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWClassDomain.h"
#include "KWDatabase.h"
#include "KWDatabaseView.h"
#include "KWSTDatabaseTextFileView.h"
#include "KWMTDatabaseTextFileView.h"
#include "KWDatabaseTransferTask.h"
#include "KWClassAttributeHelpList.h"
#include "KWDatabaseAttributeValuesHelpList.h"
#include "LMLicenseManager.h"

#include "KIWhyParameterView.h"
#include "KIHowParameterView.h"

////////////////////////////////////////////////////////////
// Classe KIPredictorInterpretationView

class KIPredictorInterpretationView : public UICard
{
public:
	// Constructeur
	KIPredictorInterpretationView();
	~KIPredictorInterpretationView();

	// Initialisation des specifications du fichier d'origine
	// a presenter a l'utilisateur
	// Si NULL, reinitialisation de toutes les valeurs a vide
	// La classe assure sinon la permanence des dernieres saisies
	// utilisateurs entre deux ouvertures successives
	void InitializeSourceDatabase(KWDatabase* database);

	// Reimplementation de la methode Open
	void Open() override;

	// Action de construction d'un dictionnaire d'interpretation
	void BuildInterpretationClass();

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KIWhyParameterView* whyParameterView;
	KIHowParameterView* howParameterView;

	// Memorisation des donnees gerees par la fiche
	ALString sClassName;

	// Parameters for interpretation dictionary
	KIInterpretationSpec* interpretationSpec;

	KWDatabase* sourceDatabase;
	KWClassDomain* originDomain;
};