// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated  with GenereTable

#include "Object.h"

// ## Custom includes

#include "KWAttributeName.h"
#include "KWVersion.h"

// DDD
#include "KWDataGridOptimizerParameters.h"

// ##

////////////////////////////////////////////////////////////
// Classe CCInstancesVariablesCoclusteringSpec
//    Coclustering parameters
class CCInstancesVariablesCoclusteringSpec : public Object
{
public:
	// Constructeur
	CCInstancesVariablesCoclusteringSpec();
	~CCInstancesVariablesCoclusteringSpec();

	// Copie et duplication
	void CopyFrom(const CCInstancesVariablesCoclusteringSpec* aSource);
	CCInstancesVariablesCoclusteringSpec* Clone() const;

	////////////////////////////////////////////////////////
	// Acces aux attributs

	// Identifier variable
	const ALString& GetIdentifierAttribute() const;
	void SetIdentifierAttribute(const ALString& sValue);

	// CH AD DDD
	// Optimization  paramters
	KWDataGridOptimizerParameters* GetOptimizationParameters();
	// void SetOptimizationParameters(KWDataGridOptimizerParameters* parameters);

	////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Tableau des attributes (KWAttributeName), parametres d'un coclustering
	ObjectArray* GetAttributes();

	// Nombre max d'attributs pour le coclustering
	static int GetMaxCoclusteringAttributeNumber();

	// Tableau des noms des variables et de leur axe (KWAttributeAxisName), parametres d'un coclustering generique
	ObjectArray* GetAttributesAndAxes();

	// Nombre max d'axes pour le coclustering generique
	static int GetMaxCoclusteringAxisNumber();

	// ##

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	ALString sIdentifierAttribute;

	// ## Custom implementation

	// Tableau des variables
	ObjectArray oaAttributes;

	// Tableau des variables avec leur axe
	ObjectArray oaAttributesAxes;

	// CH AB DDD
	KWDataGridOptimizerParameters optimizationParameters;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& CCInstancesVariablesCoclusteringSpec::GetIdentifierAttribute() const
{
	return sIdentifierAttribute;
}

inline void CCInstancesVariablesCoclusteringSpec::SetIdentifierAttribute(const ALString& sValue)
{
	sIdentifierAttribute = sValue;
}

// ## Custom inlines

// ##
