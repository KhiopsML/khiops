// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KMDRLocalModelChooser.h"

///////////////////////////////////////////////////////////////
// Classe KMDRLocalModelChooser

#include "KMDRLocalModelChooser.h"
KMDRLocalModelChooser::KMDRLocalModelChooser()
{
	SetName("LocalModelChooser");
	SetLabel("Clustering local model chooser");
	SetType(KWType::Structure);
	SetStructureName("Classifier");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);

	nFirstDataGridOperand = 0;
	GetFirstOperand()->SetType(KWType::Continuous);

	// Gestion de l'optimisation
	nOptimizationFreshness = 0;

	idClusterAttribute = NULL;
}

KMDRLocalModelChooser::~KMDRLocalModelChooser() {}

KWDerivationRule* KMDRLocalModelChooser::Create() const
{
	return new KMDRLocalModelChooser;
}

boolean KMDRLocalModelChooser::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
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

boolean KMDRLocalModelChooser::CheckOperandsCompleteness(KWClass* kwcOwnerClass) const
{
	boolean bOk;

	//	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	return bOk;
}

Object* KMDRLocalModelChooser::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	// la methode renvoie le classifieur correspondant a l'id de cluster pour cette instance

	if (idClusterAttribute == NULL)
	{
		// la metadata de l'id cluster n'a pas ete trouvee dans le dico ?
		Global::AddError("", "",
				 "Id cluster attribute was not found in dictionary " +
				     kwoObject->GetClass()->GetName() + ", or its meta tag was invalid.");
		return (Object*)this;
	}

	kwoObject->ComputeContinuousValueAt(idClusterAttribute->GetLoadIndex());
	const Continuous idCluster = kwoObject->GetContinuousValueAt(idClusterAttribute->GetLoadIndex());
	assert(idCluster >= 0);

	const KWDRClassifier* classifier = cast(
	    const KWDRClassifier*, GetOperandAt((int)idCluster)->GetReferencedDerivationRule(kwoObject->GetClass()));

	return classifier->ComputeStructureResult(kwoObject);
}

void KMDRLocalModelChooser::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();
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