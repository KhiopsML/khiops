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

	// Creation des structures des gestion des renforcement pour les acces par rang
	/*DDD
	CreateRankedContributionStructures(GetTargetValueNumber(), svPredictorAttributeNames.GetSize(),
					   &svPredictorAttributeNames);
					   */

	// Trace
	if (bTrace)
		WriteDetails(cout);
}

Object* KIDRClassifierReinforcer::ComputeStructureResult(const KWObject* kwoObject) const
{
	return (Object*)this;
}

Continuous KIDRClassifierReinforcer::GetReinforcementInitialScoreAt(Symbol sTargetValue) const
{
	return 0;
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

void KIDRClassifierReinforcer::Clean() {}

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
