// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "MDKhiopsModelingSpec.h"
#include "KWModelingSpecView.h"

// ## Custom includes

#include "KWPredictorSelectiveNaiveBayesView.h"
#include "SNBPredictorSelectiveNaiveBayesView.h"
#include "KWPredictorDataGridView.h"
#include "KWVersion.h"

// ##

////////////////////////////////////////////////////////////
// Classe MDKhiopsModelingSpecView
//    Modeling parameters
// Editeur de MDKhiopsModelingSpec
class MDKhiopsModelingSpecView : public KWModelingSpecView
{
public:
	// Constructeur
	MDKhiopsModelingSpecView();
	~MDKhiopsModelingSpecView();

	// Acces a l'objet edite
	MDKhiopsModelingSpec* GetMDKhiopsModelingSpec();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

#ifdef DEPRECATED_V10
	// DEPRECATED V10: fonctionnalite obsolete, conservee de facon cachee en V10 pour compatibilite ascendante des
	// scenarios Cette inspection se fait maintenant depuis une autre vue Inspection des parametres avances du
	// predicteur Bayesien Naif Selectif
	void DEPRECATEDInspectSelectiveNaiveBayesParameters();
#endif // DEPRECATED_V10

	// Inspection des parametres avances du predicteur Data Grid
	void InspectDataGridParameters();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
