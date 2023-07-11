// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWAttributeConstructionSpec.h"

// ## Custom includes

#include "KDConstructionDomainView.h"
#include "KWRecodingSpecView.h"
#include "KDDataPreparationAttributeCreationTask.h"
#include "KDDataPreparationAttributeCreationTaskView.h"
#include "KWVersion.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWAttributeConstructionSpecView
//    Feature engineering parameters
// Editeur de KWAttributeConstructionSpec
class KWAttributeConstructionSpecView : public UIObjectView
{
public:
	// Constructeur
	KWAttributeConstructionSpecView();
	~KWAttributeConstructionSpecView();

	// Acces a l'objet edite
	KWAttributeConstructionSpec* GetKWAttributeConstructionSpec();

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
	// DEPRECATED V10: actions obsoletes, conserves de facon cachee en V10 pour compatibilite ascendante des
	// scenarios
	void DEPRECATEDInspectConstructionDomain();
	void DEPRECATEDInspectAttributeCreationParameters();
	void DEPRECATEDInspectRecodingSpec();
#endif // DEPRECATED_V10

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
