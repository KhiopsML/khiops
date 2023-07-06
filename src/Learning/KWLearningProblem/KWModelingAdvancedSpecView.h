// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWModelingSpec.h"

// ## Custom includes

#include "SNBPredictorSelectiveNaiveBayesView.h"
#include "KDConstructionDomainView.h"
#include "KDTextFeatureSpecView.h"
#include "KWAttributePairsSpecView.h"
#include "KDDataPreparationAttributeCreationTaskView.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWModelingAdvancedSpecView
//    Advanced predictor parameters
// Editeur de KWModelingSpec
class KWModelingAdvancedSpecView : public UIObjectView
{
public:
	// Constructeur
	KWModelingAdvancedSpecView();
	~KWModelingAdvancedSpecView();

	// Acces a l'objet edite
	KWModelingSpec* GetKWModelingSpec();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	///////////////////////////////////////////////////////////////
	// Parametrage avance

	// Inspection des parametres de construction de variable
	void InspectConstructionDomain();

	// Inspection des parametres de construction de variables de type texte
	void InspectTextFeaturesParameters();

	// Inspection des parametres de construction des arbres
	void InspectAttributeCreationParameters();

	// Inspection des parametres d'analyse des paires de variables
	void InspectAttributePairsParameters();

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##