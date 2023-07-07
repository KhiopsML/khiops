// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWAttributeName.h"
#include "KWVersion.h"
#include "KWDataGridOptimizerParameters.h"

// ##

////////////////////////////////////////////////////////////
// Classe CCVarPartCoclusteringSpec
//    Instances Variables coclustering parameters
class CCVarPartCoclusteringSpec : public Object
{
public:
	// Constructeur
	CCVarPartCoclusteringSpec();
	~CCVarPartCoclusteringSpec();

	// Copie et duplication
	void CopyFrom(const CCVarPartCoclusteringSpec* aSource);
	CCVarPartCoclusteringSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Identifier variable
	const ALString& GetIdentifierAttribute() const;
	void SetIdentifierAttribute(const ALString& sValue);

	///////////////////////////////////////////////////////////
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

	// Tableau des noms des variables et de leur axe (KWAttributeAxisName), parametres d'un coclustering VarPart
	ObjectArray* GetAttributesAndAxes();

	// Nombre max d'axes pour le coclustering VarPart
	static int GetMaxCoclusteringAxisNumber();

	// CH AD DDD
	// Optimization  paramters
	KWDataGridOptimizerParameters* GetOptimizationParameters();
	// void SetOptimizationParameters(KWDataGridOptimizerParameters* parameters);

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
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

inline const ALString& CCVarPartCoclusteringSpec::GetIdentifierAttribute() const
{
	return sIdentifierAttribute;
}

inline void CCVarPartCoclusteringSpec::SetIdentifierAttribute(const ALString& sValue)
{
	sIdentifierAttribute = sValue;
}

// ## Custom inlines

// ##
