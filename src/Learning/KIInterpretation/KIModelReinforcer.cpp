// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelReinforcer.h"

KIModelReinforcer::KIModelReinforcer()
{
	// ## Custom constructor

	// ##
}

KIModelReinforcer::~KIModelReinforcer()
{
	// ## Custom destructor

	oaLeverAttributes.DeleteAll();

	// ##
}

void KIModelReinforcer::CopyFrom(const KIModelReinforcer* aSource)
{
	require(aSource != NULL);

	KIModelService::CopyFrom(aSource);

	sReinforcedTargetValue = aSource->sReinforcedTargetValue;

	// ## Custom copyfrom

	// Variable locales
	KIPredictorAttribute* leverAttribute;
	int i;

	// Duplication des attributs du predicteur
	oaLeverAttributes.DeleteAll();
	for (i = 0; i < aSource->oaLeverAttributes.GetSize(); i++)
	{
		leverAttribute = cast(KIPredictorAttribute*, aSource->oaLeverAttributes.GetAt(i));
		oaLeverAttributes.Add(leverAttribute->Clone());
	}

	// ##
}

KIModelReinforcer* KIModelReinforcer::Clone() const
{
	KIModelReinforcer* aClone;

	aClone = new KIModelReinforcer;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KIModelReinforcer::Write(ostream& ost) const
{
	KIModelService::Write(ost);
	ost << "Target value to reinforce\t" << GetReinforcedTargetValue() << "\n";
}

const ALString KIModelReinforcer::GetClassLabel() const
{
	return "Reinforce model";
}

// ## Method implementation

void KIModelReinforcer::UpdateLeverAttributes()
{
	KIPredictorAttribute* leverAttribute;
	KWAttribute* attribute;
	double dLevel;
	double dWeight;
	double dImportance;
	int i;

	require((GetClassBuilder()->IsPredictorImported() and
		 GetPredictorClassName() == GetClassBuilder()->GetPredictorClass()->GetName()) or
		(not GetClassBuilder()->IsPredictorImported() and GetPredictorClassName() == ""));

	// Nettoyage initial
	oaLeverAttributes.DeleteAll();

	// Alimentation des attributs du predicteur
	if (GetClassBuilder()->IsPredictorImported())
	{
		// Alimentation a partir des specification disponible dans le ClassBuilder
		for (i = 0; i < GetClassBuilder()->GetPredictorAttributeNumber(); i++)
		{
			// Ajout d'une variable au tableau
			leverAttribute = new KIPredictorAttribute;
			oaLeverAttributes.Add(leverAttribute);

			// Specification de la variable
			attribute = GetClassBuilder()->GetPredictorClass()->LookupAttribute(
			    GetClassBuilder()->GetPredictorAttributeNames()->GetAt(i));
			assert(attribute != NULL);
			leverAttribute->SetType(KWType::ToString(attribute->GetType()));
			leverAttribute->SetName(attribute->GetName());

			// Attention, pour l'importance, on reproduit le calcul effectue dans la methode KWSelectedAttributeReport::GetImportance
			// Il n'y a pas de meta-data disponible pour le critere d'importance
			// Ce code est pour l'instant "adhoc", et on se premunit contre les meta-data erronnees
			dLevel = attribute->GetConstMetaData()->GetDoubleValueAt("Level");
			dWeight = attribute->GetConstMetaData()->GetDoubleValueAt("Weight");
			dLevel = max(dLevel, (double)0);
			dLevel = min(dLevel, (double)1);
			dWeight = max(dLevel, (double)0);
			dWeight = min(dWeight, (double)1);
			dImportance = sqrt(dLevel * dWeight);
			leverAttribute->SetImportance(KWContinuous::DoubleToContinuous(dImportance));
		}

		// Tri par importance decroissante
		oaLeverAttributes.SetCompareFunction(KIPredictorAttributeCompareImportance);
		oaLeverAttributes.Sort();
	}
}

const ALString KIModelReinforcer::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
