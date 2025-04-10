// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIDRReinforcement.h"

void KIDRRegisterReinforcementRules()
{
	KWDerivationRule::RegisterDerivationRule(new KIDRClassifierReinforcer);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementInitialScoreAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementAttributeAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementPartAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementFinalScoreAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementClassChangeTagAt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierReinforcer

KIDRClassifierReinforcer::KIDRClassifierReinforcer()
{
	SetName("ClassifierReinforcer");
	SetLabel("Classifier reinforcer");
	SetStructureName("ClassifierReinforcer");

	// Ajout d'un deuxieme operande en plus du classifier, pour les attributs de renforcement
	assert(GetOperandNumber() == 1);
	SetOperandNumber(2);
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("VectorC");
}

KIDRClassifierReinforcer::~KIDRClassifierReinforcer() {}

KWDerivationRule* KIDRClassifierReinforcer::Create() const
{
	return new KIDRClassifierReinforcer;
}

void KIDRClassifierReinforcer::Compile(KWClass* kwcOwnerClass)
{
	const boolean bTrace = false;

	// Appel de la methode ancetre
	KIDRClassifierService::Compile(kwcOwnerClass);
	assert(classifierRule != NULL);

	// Initialisation du vecteur de score
	assert(cvInitialScores.GetSize() == 0);
	cvInitialScores.SetSize(GetTargetValueNumber());

	// Trace
	if (bTrace)
		WriteDetails(cout);
}

Object* KIDRClassifierReinforcer::ComputeStructureResult(const KWObject* kwoObject) const
{
	int nTarget;

	// Appel de la methode ancetre
	KIDRClassifierService::ComputeStructureResult(kwoObject);

	// On indique que les renforecment par classe cible sont a recalculer si necessaire,
	// en forcant les scores a 0 (une probabilite du predicteur ne peut etre nulle)
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		cvInitialScores.SetAt(nTarget, 0);
	return (Object*)this;
}

Continuous KIDRClassifierReinforcer::GetReinforcementInitialScoreAt(Symbol sTargetValue) const
{
	int nTargetValueRank;

	require(IsCompiled());

	// Recherche du rang de la valeur cible
	nTargetValueRank = GetTargetValueRank(sTargetValue);

	// On ne renvoie rien si la valeur cible est incorrecte
	if (nTargetValueRank == -1)
		return 0;
	// Sinon, on renvoie le nom de l'attribut correspondant
	else
	{
		// Calcul des informations de renforcement
		if (cvInitialScores.GetAt(nTargetValueRank) == 0)
			ComputeRankedReinforcementAt(nTargetValueRank);

		// Retourne le initial
		return cvInitialScores.GetAt(nTargetValueRank);
	}
}

Symbol KIDRClassifierReinforcer::GetRankedReinforcementAttributeAt(Symbol sTargetValue, int nAttributeRank) const
{
	return Symbol();
}

Symbol KIDRClassifierReinforcer::GetRankedReinforcementPartAt(Symbol sTargetValue, int nAttributeRank) const
{
	return Symbol();
}

Continuous KIDRClassifierReinforcer::GetRankedReinforcementFinalScoreAt(Symbol sTargetValue, int nAttributeRank) const
{
	return 0;
}

Continuous KIDRClassifierReinforcer::GetRankedReinforcementClassChangeTagAt(Symbol sTargetValue,
									    int nAttributeRank) const
{
	return 0;
}

longint KIDRClassifierReinforcer::GetUsedMemory() const
{
	return 0;
}

void KIDRClassifierReinforcer::Clean()
{
	// Methode ancetre
	KIDRClassifierService::Clean();

	// Nettoyage des structures specifiques
	cvInitialScores.SetSize(0);
}

void KIDRClassifierReinforcer::ComputeRankedReinforcementAt(int nTarget) const
{
	require(IsCompiled());
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	require(ivDataGridSourceIndexes.GetSize() == GetPredictorAttributeNumber());
	require(classifierRule != NULL);
	require(cvInitialScores.GetAt(nTarget) == 0);

	// On confie au classifieur le calcul du score
	cvInitialScores.SetAt(nTarget, classifierRule->ComputeTargetProbAt(GetTargetValueAt(nTarget)));

	ensure(cvInitialScores.GetAt(nTarget) > 0);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementInitialScoreAt

KIDRReinforcementInitialScoreAt::KIDRReinforcementInitialScoreAt()
{
	SetName("ReinforcementInitialScoreAt");
	SetLabel("Reinforcement initial score");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
}

KIDRReinforcementInitialScoreAt::~KIDRReinforcementInitialScoreAt() {}

KWDerivationRule* KIDRReinforcementInitialScoreAt::Create() const
{
	return new KIDRReinforcementInitialScoreAt;
}

Continuous KIDRReinforcementInitialScoreAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;

	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));
	return classifierReinforcer->GetReinforcementInitialScoreAt(GetOperandAt(1)->GetSymbolValue(kwoObject));
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementAttributeAt

KIDRReinforcementAttributeAt::KIDRReinforcementAttributeAt()
{
	SetName("ReinforcementVariableAt");
	SetLabel("Reinforcement variable name");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRReinforcementAttributeAt::~KIDRReinforcementAttributeAt() {}

KWDerivationRule* KIDRReinforcementAttributeAt::Create() const
{
	return new KIDRReinforcementAttributeAt;
}

Symbol KIDRReinforcementAttributeAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;
	int nAttributeRank;

	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));
	nAttributeRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);
	return classifierReinforcer->GetRankedReinforcementAttributeAt(GetOperandAt(1)->GetSymbolValue(kwoObject),
								       nAttributeRank);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementPartAt

KIDRReinforcementPartAt::KIDRReinforcementPartAt()
{
	SetName("ReinforcementPartAt");
	SetLabel("Reinforcement variable part");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRReinforcementPartAt::~KIDRReinforcementPartAt() {}

KWDerivationRule* KIDRReinforcementPartAt::Create() const
{
	return new KIDRReinforcementPartAt;
}

Symbol KIDRReinforcementPartAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;
	int nAttributeRank;

	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));
	nAttributeRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);
	return classifierReinforcer->GetRankedReinforcementPartAt(GetOperandAt(1)->GetSymbolValue(kwoObject),
								  nAttributeRank);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementFinalScoreAt

KIDRReinforcementFinalScoreAt::KIDRReinforcementFinalScoreAt()
{
	SetName("ReinforcementFinalScoreAt");
	SetLabel("Reinforcement final score");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRReinforcementFinalScoreAt::~KIDRReinforcementFinalScoreAt() {}

KWDerivationRule* KIDRReinforcementFinalScoreAt::Create() const
{
	return new KIDRReinforcementFinalScoreAt;
}

Continuous KIDRReinforcementFinalScoreAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;
	int nAttributeRank;

	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));
	nAttributeRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);
	return classifierReinforcer->GetRankedReinforcementFinalScoreAt(GetOperandAt(1)->GetSymbolValue(kwoObject),
									nAttributeRank);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementClassChangeTagAt

KIDRReinforcementClassChangeTagAt::KIDRReinforcementClassChangeTagAt()
{
	SetName("ReinforcementClassChangeTagAt");
	SetLabel("Variable reinforcement class change tag");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRReinforcementClassChangeTagAt::~KIDRReinforcementClassChangeTagAt() {}

KWDerivationRule* KIDRReinforcementClassChangeTagAt::Create() const
{
	return new KIDRReinforcementClassChangeTagAt;
}

Continuous KIDRReinforcementClassChangeTagAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;
	int nAttributeRank;

	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));
	nAttributeRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);
	return classifierReinforcer->GetRankedReinforcementClassChangeTagAt(GetOperandAt(1)->GetSymbolValue(kwoObject),
									    nAttributeRank);
}

////////////////////////////////////////////////////////////
// Classe KIAttributeReinforcement

KIAttributeReinforcement::KIAttributeReinforcement()
{
	nAttributeIndex = 0;
	nReinforcementModalityIndex = 0;
	cReinforcementFinalScore = 0;
	nReinforcementClassChangeTag = 0;
	svAttributeNames = NULL;
}

KIAttributeReinforcement::~KIAttributeReinforcement() {}

void KIAttributeReinforcement::SetAttributeNames(const StringVector* svNames)
{
	svAttributeNames = svNames;
}

const StringVector* KIAttributeReinforcement::GetAttributeNames() const
{
	return svAttributeNames;
}

int KIAttributeReinforcementCompare(const void* elem1, const void* elem2)
{
	int nCompare;
	KIAttributeReinforcement* attributeReinforcement1;
	KIAttributeReinforcement* attributeReinforcement2;

	// Acces aux objets
	attributeReinforcement1 = cast(KIAttributeReinforcement*, *(Object**)elem1);
	attributeReinforcement2 = cast(KIAttributeReinforcement*, *(Object**)elem2);

	// Comparaison selon la precision du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(attributeReinforcement1->GetReinforcementFinalScore(),
							attributeReinforcement2->GetReinforcementFinalScore());

	// Comparaison sur le nom de l'attribut en cas d'egalite
	if (nCompare == 0)
		nCompare =
		    attributeReinforcement1->GetAttributeName().Compare(attributeReinforcement2->GetAttributeName());
	return nCompare;
}
