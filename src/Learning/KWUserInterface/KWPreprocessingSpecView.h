// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWPreprocessingSpec.h"

// ## Custom includes

#include "KWPreprocessingAdvancedSpecView.h"
#include "MHHistogramSpecView.h"
#include "KWDataGridOptimizerParametersView.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWPreprocessingSpecView
//    Preprocessing parameters
// Editeur de KWPreprocessingSpec
class KWPreprocessingSpecView : public UIObjectView
{
public:
	// Constructeur
	KWPreprocessingSpecView();
	~KWPreprocessingSpecView();

	// Acces a l'objet edite
	KWPreprocessingSpec* GetKWPreprocessingSpec();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Action d'inspection des parametres avances
	void InspectAdvancedParameters();
	void InspectHistogramParameters();
	void InspectDataGridOptimizerParameters();

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;

	// Parametrage de l'objet de gestion de specification des histogrammes
	// Pour des raisons techniques (cycle de librairie), cet objet n'est pas un sous-objet des preprocessingSpec
	void SetHistogramSpecObject(Object* object);
	Object* GetHistogramSpecObject();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Objet de gestion de specification des histogrammes
	Object* histogramSpecObject;

	// ##
};

// ## Custom inlines

// ##
