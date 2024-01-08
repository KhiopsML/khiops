// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "CCAnalysisResults.h"

// ## Custom includes

#include "CCCoclusteringReport.h"

// ##

////////////////////////////////////////////////////////////
// Classe CCAnalysisResultsView
//    Results
// Editeur de CCAnalysisResults
class CCAnalysisResultsView : public UIObjectView
{
public:
	// Constructeur
	CCAnalysisResultsView();
	~CCAnalysisResultsView();

	// Acces a l'objet edite
	CCAnalysisResults* GetCCAnalysisResults();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Parametrage de la visibilite des fichiers resultats
	void SetResultFieldsVisible(boolean bValue);

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
