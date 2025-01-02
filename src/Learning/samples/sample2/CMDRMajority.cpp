// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CMDRMajority.h"

CMDRMajorityClassifier::CMDRMajorityClassifier()
{
	// Initialisation des parametres de la regle et de ses operandes
	SetName("MajorityClassifier");
	SetLabel("Majority Classifier");
}

CMDRMajorityClassifier::~CMDRMajorityClassifier() {}

KWDerivationRule* CMDRMajorityClassifier::Create() const
{
	return new CMDRMajorityClassifier;
}

Object* CMDRMajorityClassifier::ComputeStructureResult(const KWObject* kwoObject) const
{
	int nOperand;

	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	// On evalue les operandes
	for (nOperand = 0; nOperand < GetOperandNumber() - 1; nOperand++)
		GetOperandAt(nOperand)->GetStructureValue(kwoObject);

	// Calcul du vecteur de probabilites conditionnelles
	ComputeTargetProbs();

	return (Object*)this;
}

void CMDRMajorityClassifier::ComputeTargetProbs() const
{
	int nTarget;
	int nFrequencyMax;
	int nFrequencyMaxIndex;
	Symbol sMode;

	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Calcul du mode
	nFrequencyMax = 0;
	nFrequencyMaxIndex = -1;
	for (nTarget = 0; nTarget < ivFrequencies.GetSize(); nTarget++)
	{
		if (ivFrequencies.GetAt(nTarget) > nFrequencyMax)
		{
			nFrequencyMax = ivFrequencies.GetAt(nTarget);
			nFrequencyMaxIndex = nTarget;
		}
	}
	assert(nFrequencyMaxIndex > -1);
	sMode = svTargetValues.GetAt(nFrequencyMaxIndex);

	for (nTarget = 0; nTarget < svTargetValues.GetSize(); nTarget++)
	{
		if (svTargetValues.GetAt(nTarget) == sMode)
		{
			// Memorisation du resultat
			cvTargetProbs.SetAt(nTarget, (Continuous)(1.0 - (svTargetValues.GetSize() * 0.001)));
		}
		else
			cvTargetProbs.SetAt(nTarget, (Continuous)0.001);
	}
}

void CMDRMajorityClassifier::Compile(KWClass* kwcOwnerClass)
{
	const KWDRDataGrid* targetDataGrid;
	const KWDRFrequencies* targetFrequencies;
	int nTargetValueNumber;
	int nTarget;

	// Appel de la methode ancetre
	KWDRNBClassifier::Compile(kwcOwnerClass);

	// Recherche du dernier operande: distribution des valeurs cibles
	targetDataGrid =
	    cast(const KWDRDataGrid*, GetOperandAt(GetOperandNumber() - 1)->GetReferencedDerivationRule(kwcOwnerClass));
	assert(targetDataGrid->GetAttributeNumber() == 1);
	assert(targetDataGrid->GetAttributeTypeAt(0) == KWType::Symbol);
	nTargetValueNumber = targetDataGrid->GetTotalCellNumber();

	// Initialisation du vecteur des frequences cibles
	targetFrequencies =
	    cast(const KWDRFrequencies*, targetDataGrid->GetOperandAt(1)->GetReferencedDerivationRule(kwcOwnerClass));
	assert(targetFrequencies->GetFrequencyNumber() == nTargetValueNumber);
	ivFrequencies.SetSize(nTargetValueNumber);
	for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
		ivFrequencies.SetAt(nTarget, targetFrequencies->GetFrequencyAt(nTarget));
}
