// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"
#include "KIModelService.h"

// ## Custom includes

#include "KIPredictorAttribute.h"
#include "SNBPredictorSelectiveNaiveBayes.h"

// ##

////////////////////////////////////////////////////////////
// Classe KIModelReinforcer
//    Reinforce model
class KIModelReinforcer : public KIModelService
{
public:
	// Constructeur
	KIModelReinforcer();
	~KIModelReinforcer();

	// Copie et duplication
	void CopyFrom(const KIModelReinforcer* aSource);
	KIModelReinforcer* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Target value to reinforce
	const ALString& GetReinforcedTargetValue() const;
	void SetReinforcedTargetValue(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Test de validite des specifications
	boolean Check() const override;

	// Tableau des attributs du predicteur pour choisir les variables leviers (classe KIPredictorAttribute)
	// Memoire: appartient a l'appele
	const ObjectArray* GeConstLeverAttributes() const;
	ObjectArray* GeLeverAttributes();

	// Calcul du nombre de variables selectionnees comme variable levier
	int ComputeSelectedLeverAttributeNumber() const;

	// Mise ajour du tableau des attributs du predicteur
	void UpdateLeverAttributes();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sReinforcedTargetValue;

	// ## Custom implementation

	// Tableau des attributs du predicteur pour le parametrage des variables leviers
	ObjectArray oaLeverAttributes;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KIModelReinforcer::GetReinforcedTargetValue() const
{
	return sReinforcedTargetValue;
}

inline void KIModelReinforcer::SetReinforcedTargetValue(const ALString& sValue)
{
	sReinforcedTargetValue = sValue;
}

// ## Custom inlines

inline const ObjectArray* KIModelReinforcer::GeConstLeverAttributes() const
{
	return &oaLeverAttributes;
}

inline ObjectArray* KIModelReinforcer::GeLeverAttributes()
{
	return &oaLeverAttributes;
}

// ##
