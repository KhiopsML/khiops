// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWAnalysisSpec.h"

// ## Custom includes

#include "KWModelingSpecView.h"
#include "KWRecoderSpecView.h"
#include "KWAttributeConstructionSpecView.h"
#include "KWPreprocessingSpecView.h"
#include "KWSystemParametersView.h"
#include "KWCrashTestParametersView.h"
#include "KWVersion.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWAnalysisSpecView
//    Parameters
// Editeur de KWAnalysisSpec
class KWAnalysisSpecView : public UIObjectView
{
public:
	// Constructeur
	KWAnalysisSpecView();
	~KWAnalysisSpecView();

	// Acces a l'objet edite
	KWAnalysisSpec* GetKWAnalysisSpec();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Libelles utilisateur
	const ALString GetObjectLabel() const override;

#ifdef DEPRECATED_V10
	// DEPRECATED V10
	// Utilisation d'une copie a part de KWAttributeConstructionSpec
	// pour detecter l'utilisation a tort de l'onglet obsolete
	KWAttributeConstructionSpec DEPRECATEDAttributeConstructionSpec;
	KWAttributeConstructionSpec DEPRECATEDAttributeConstructionSpecReference;

	// DEPRECATED V10
	// Utilisation d'une copie a part de KWModelingSpec
	// pour detecter l'utilisation a tort de l'onglet obsolete
	KWModelingSpec* DEPRECATEDModelingSpec;
	KWModelingSpec* DEPRECATEDModelingSpecReference;
#endif // DEPRECATED_V10

	// ##
};

// ## Custom inlines

// ##
