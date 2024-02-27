// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CMDRMajority.h"

CMDRMajorityClassifier::CMDRMajorityClassifier()
{
	// Initialisation des parametres de la regle et de ses operandes
	SetName("MajorityClassifier");
	SetLabel("Majority Classifier");
	nTotalFrequency = 0;
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

	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Calcul des proba a partir des effectifs en apprentissage
	for (nTarget = 0; nTarget < svTargetValues.GetSize(); nTarget++)
	{
		cvTargetProbs.SetAt(nTarget, ivFrequencies.GetAt(nTarget) / (double)nTotalFrequency);
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
	nTotalFrequency = 0;
	targetFrequencies =
	    cast(const KWDRFrequencies*, targetDataGrid->GetOperandAt(1)->GetReferencedDerivationRule(kwcOwnerClass));
	assert(targetFrequencies->GetFrequencyNumber() == nTargetValueNumber);
	ivFrequencies.SetSize(nTargetValueNumber);
	for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
	{
		ivFrequencies.SetAt(nTarget, targetFrequencies->GetFrequencyAt(nTarget));
		nTotalFrequency += targetFrequencies->GetFrequencyAt(nTarget);
	}
}
