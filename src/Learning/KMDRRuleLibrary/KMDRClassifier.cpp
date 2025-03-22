// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KMDRClassifier.h"

///////////////////////////////////////////////////////////////
// Classe KMDRClassifier

#include "KMDRClassifier.h"
KMDRClassifier::KMDRClassifier()
{
	SetName("KMDRClassifier");
	SetLabel("Clustering classifier");
	SetType(KWType::Structure);
	SetStructureName("Classifier");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);

	// Les operandes contiennent des regles de type Continuous
	nFirstDataGridOperand = 0;
	GetFirstOperand()->SetType(KWType::Continuous);

	// Gestion de l'optimisation
	cUnknownTargetProb = 0;
	nOptimizationFreshness = 0;
	idClusterAttribute = NULL;
}

KMDRClassifier::~KMDRClassifier() {}

KWDerivationRule* KMDRClassifier::Create() const
{
	return new KMDRClassifier;
}

boolean KMDRClassifier::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());

	// Verification d'un nombre d'operande minimal
	if (GetOperandNumber() < 1 + nFirstDataGridOperand)
	{
		bOk = false;
		AddError(sTmp + "The number of operands should be at least " + IntToString(1 + nFirstDataGridOperand));
	}

	return bOk;
}

boolean KMDRClassifier::CheckOperandsCompleteness(KWClass* kwcOwnerClass) const
{
	boolean bOk;

	//	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	return bOk;
}

Object* KMDRClassifier::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	if (idClusterAttribute == NULL)
	{
		// la metadata de l'id cluster n'a pas ete trouvee dans le dico ?
		Global::AddError("", "",
				 "Id cluster attribute was not found in dictionary " +
				     kwoObject->GetClass()->GetName() + ", or its meta tag was invalid.");
		return (Object*)this;
	}
	if (not idClusterAttribute->GetLoadIndex().IsValid())
	{
		Global::AddError("", "", "Id cluster attribute has no valid load index");
		return (Object*)this;
	}
	assert(idClusterAttribute->GetLoadIndex().IsValid());

	kwoObject->ComputeContinuousValueAt(idClusterAttribute->GetLoadIndex());
	Continuous idCluster = kwoObject->GetContinuousValueAt(idClusterAttribute->GetLoadIndex());
	assert(idCluster >= 0);

	const KWDRContinuousVector* cvr =
	    cast(const KWDRContinuousVector*,
		 GetOperandAt((int)idCluster)->GetReferencedDerivationRule(kwoObject->GetClass()));

	const ContinuousVector* cv = cvr->GetValues();

	assert(cv != NULL);

	for (int i = 0; i < cvTargetProbs.GetSize(); i++)
	{
		cvTargetProbs.SetAt(i, cv->GetAt(i));
	}

	return (Object*)this;
}

Symbol KMDRClassifier::ComputeTargetValue() const
{
	int nTarget;
	int nBestTarget;
	Continuous cMaxProb;

	require(IsCompiled());
	require(IsOptimized());
	// require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Recherche de l'index de la probabilite max
	cMaxProb = 0;
	nBestTarget = 0;
	for (nTarget = 0; nTarget < cvTargetProbs.GetSize(); nTarget++)
	{
		if (cvTargetProbs.GetAt(nTarget) > cMaxProb)
		{
			cMaxProb = cvTargetProbs.GetAt(nTarget);
			nBestTarget = nTarget;
		}
	}
	return svTargetValues.GetAt(nBestTarget);
}

Continuous KMDRClassifier::ComputeTargetProb() const
{
	int nTarget;
	Continuous cMaxProb;

	require(IsCompiled());
	require(IsOptimized());
	// require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Recherche de l'index de la probabilites max
	cMaxProb = 0;
	for (nTarget = 0; nTarget < cvTargetProbs.GetSize(); nTarget++)
	{
		if (cvTargetProbs.GetAt(nTarget) > cMaxProb)
			cMaxProb = cvTargetProbs.GetAt(nTarget);
	}
	return cMaxProb;
}

Continuous KMDRClassifier::ComputeTargetProbAt(const Symbol& sValue) const
{
	int nTarget;

	require(IsCompiled());
	require(IsOptimized());
	// require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Recherche de l'index de la valeur cible
	// Pour un petit nombre de valeurs cibles, une recherche indexee est suffisante en performance
	for (nTarget = 0; nTarget < cvTargetProbs.GetSize(); nTarget++)
	{
		// Si trouve, on retourne sa probabilite conditionnelles
		if (svTargetValues.GetAt(nTarget) == sValue)
			return cvTargetProbs.GetAt(nTarget);
	}

	// Si on a rien trouve (la valeur cible n'existait pas en apprentissage...)
	// on retourne la valeur speciale
	return cUnknownTargetProb;
}

Symbol KMDRClassifier::ComputeBiasedTargetValue(const ContinuousVector* cvOffsets) const
{
	int nTarget;
	int nBestTarget;
	Continuous cMaxScore;
	Continuous cOffset;

	require(IsCompiled());
	require(IsOptimized());
	// require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());
	require(cvOffsets != NULL);

	// Recherche de l'index de la probabilites max
	cMaxScore = KWContinuous::GetMinValue();
	nBestTarget = 0;
	for (nTarget = 0; nTarget < cvTargetProbs.GetSize(); nTarget++)
	{
		cOffset = 0;
		if (nTarget < cvOffsets->GetSize())
			cOffset = cvOffsets->GetAt(nTarget);
		if (cvTargetProbs.GetAt(nTarget) + cOffset > cMaxScore)
		{
			cMaxScore = cvTargetProbs.GetAt(nTarget) + cOffset;
			nBestTarget = nTarget;
		}
	}
	return svTargetValues.GetAt(nBestTarget);
}

void KMDRClassifier::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		const KWDRSymbolValueSet* targetSymbolValueSet;

		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Recherche du dernier operande: distribution des valeurs cibles
		targetSymbolValueSet =
		    cast(const KWDRSymbolValueSet*,
			 GetOperandAt(GetOperandNumber() - 1)->GetReferencedDerivationRule(kwcOwnerClass));

		assert(targetSymbolValueSet->GetValueNumber() > 0);
		const int nTargetValueNumber = targetSymbolValueSet->GetValueNumber();

		// Initialisation du vecteur des valeurs cibles
		svTargetValues.SetSize(nTargetValueNumber);

		for (int i = 0; i < nTargetValueNumber; i++)
			svTargetValues.SetAt(i, targetSymbolValueSet->GetValueAt(i));

		cvTargetProbs.SetSize(nTargetValueNumber);
		cUnknownTargetProb = 0;
	}

	idClusterAttribute = NULL;

	KWAttribute* attribute = kwcOwnerClass->GetHeadAttribute();

	while (attribute != NULL)
	{
		if (attribute->GetConstMetaData()->IsKeyPresent("ClusterIdAttribute"))
		{
			idClusterAttribute = attribute;
			break;
		}
		kwcOwnerClass->GetNextAttribute(attribute);
	}
}
