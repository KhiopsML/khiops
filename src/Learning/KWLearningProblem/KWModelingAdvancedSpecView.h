// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWModelingSpecView.h"
#include "KWPredictorSelectiveNaiveBayesView.h"
#include "SNBPredictorSelectiveNaiveBayesView.h"
#include "KDConstructionDomainView.h"
#include "KDTextFeatureSpecView.h"
#include "KWAttributePairsSpecView.h"

////////////////////////////////////////////////////////////
// Classe KWModelingAdvancedSpecView
// Vue sur les parametres avances de ModelingSpec
// Cette vue specialisee n'est pas generee. Elle edite
// directement le meme objet KWModelingSpec que KWModelingSpecView
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

	// Inspection des parametres avances du predicteur Bayesien Naif Selectif
	void InspectSelectiveNaiveBayesParameters();

	// Inspection des parametres de construction de variable
	void InspectConstructionDomain();

	// Inspection des parametres de construction de variables de type texte
	void InspectTextFeaturesParameters();

	// Inspection des parametres de construction des arbres
	void InspectAttributeCreationParameters();

	// Inspection des parametres d'analyse des paires de variables
	void InspectAttributePairsParameters();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
};