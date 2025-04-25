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

const ALString KIModelReinforcer::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

boolean KIModelReinforcer::Check() const
{
	boolean bOk;
	int nSelectedLeverAttributeNumber;
	boolean bTargetValueFound;
	int i;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KIModelService::Check();

	// Specialisation de la varification
	if (bOk)
	{
		nSelectedLeverAttributeNumber = ComputeSelectedLeverAttributeNumber();

		// Erreur si aucun attribut levier selectionne
		if (nSelectedLeverAttributeNumber == 0)
		{
			AddError("Number of selected lever variables should be at least 1");
			bOk = false;
		}

		// Erreur si trop d'attributs leviers
		if (nSelectedLeverAttributeNumber > GetMaxLeverAttributeNumber())
		{
			AddError(sTmp + "Number of lever variables selected (" +
				 IntToString(nSelectedLeverAttributeNumber) + ") should be less than " +
				 IntToString(GetMaxLeverAttributeNumber()));
			bOk = false;
		}

		// Erreur si pas de classe cible choisie
		bTargetValueFound = false;
		for (i = 0; i < classBuilder.GetTargetValues()->GetSize(); i++)
		{
			if (classBuilder.GetTargetValues()->GetAt(i).GetValue() == GetReinforcedTargetValue())
			{
				bTargetValueFound = true;
				break;
			}
		}
		if (not bTargetValueFound)
		{
			// Specialisation du message d'erreur si la valeur est vide
			if (GetReinforcedTargetValue() == "")
				AddError("A target value to reinforce must be selected");
			else
				AddError("An existing target value to reinforce must be selected");
			bOk = false;
		}
	}
	return bOk;
}

int KIModelReinforcer::ComputeSelectedLeverAttributeNumber() const
{
	int nSelectedLeverAttributeNumber;
	KIPredictorAttribute* leverAttribute;
	int i;

	// Comptage des attributs utilises
	nSelectedLeverAttributeNumber = 0;
	for (i = 0; i < oaLeverAttributes.GetSize(); i++)
	{
		leverAttribute = cast(KIPredictorAttribute*, oaLeverAttributes.GetAt(i));
		if (leverAttribute->GetUsed())
			nSelectedLeverAttributeNumber++;
	}
	return nSelectedLeverAttributeNumber;
}

void KIModelReinforcer::UpdateLeverAttributes()
{
	// Nettoyage initial
	oaLeverAttributes.DeleteAll();

	// Alimentation des attributs du predicteur
	if (GetClassBuilder()->IsPredictorImported())
		GetClassBuilder()->BuildPredictorAttributes(&oaLeverAttributes);
}

// ##
