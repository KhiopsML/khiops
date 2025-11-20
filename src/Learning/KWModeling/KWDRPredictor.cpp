// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRPredictor.h"

void KWDRRegisterPredictorRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRClassifier);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetProb);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetProbAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRBiasedTargetValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRRankRegressor);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetRankMean);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetRankStandardDeviation);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetRankDensityAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetRankCumulativeProbAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRRegressor);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetMean);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetStandardDeviation);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetDensityAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRTargetQuantileDistribution);
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRClassifier

KWDRClassifier::KWDRClassifier()
{
	SetName("Classifier");
	SetLabel("Classifier");
	SetType(KWType::Structure);
	SetStructureName("Classifier");
}

KWDRClassifier::~KWDRClassifier() {}

KWDerivationRule* KWDRClassifier::Create() const
{
	return new KWDRClassifier;
}

Symbol KWDRClassifier::ComputeTargetValue() const
{
	return Symbol();
}

Continuous KWDRClassifier::ComputeTargetProb() const
{
	return 1;
}

Continuous KWDRClassifier::ComputeTargetProbAt(const Symbol& sValue) const
{
	if (sValue.IsEmpty())
		return 1;
	else
		return 0;
}

Symbol KWDRClassifier::ComputeBiasedTargetValue(const ContinuousVector* cvOffsets) const
{
	return Symbol();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetValue

KWDRTargetValue::KWDRTargetValue()
{
	SetName("TargetValue");
	SetLabel("Target value");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Classifier");
}

KWDRTargetValue::~KWDRTargetValue() {}

KWDerivationRule* KWDRTargetValue::Create() const
{
	return new KWDRTargetValue;
}

Symbol KWDRTargetValue::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KWDRClassifier* classifier;

	classifier = cast(KWDRClassifier*, GetFirstOperand()->GetStructureValue(kwoObject));
	return classifier->ComputeTargetValue();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetProb

KWDRTargetProb::KWDRTargetProb()
{
	SetName("TargetProb");
	SetLabel("Target prob");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Classifier");
}

KWDRTargetProb::~KWDRTargetProb() {}

KWDerivationRule* KWDRTargetProb::Create() const
{
	return new KWDRTargetProb;
}

Continuous KWDRTargetProb::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRClassifier* classifier;

	classifier = cast(KWDRClassifier*, GetFirstOperand()->GetStructureValue(kwoObject));
	return classifier->ComputeTargetProb();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetProbAt

KWDRTargetProbAt::KWDRTargetProbAt()
{
	SetName("TargetProbAt");
	SetLabel("Target prob at");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Classifier");
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRTargetProbAt::~KWDRTargetProbAt() {}

KWDerivationRule* KWDRTargetProbAt::Create() const
{
	return new KWDRTargetProbAt;
}

Continuous KWDRTargetProbAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRClassifier* classifier;

	classifier = cast(KWDRClassifier*, GetFirstOperand()->GetStructureValue(kwoObject));
	return classifier->ComputeTargetProbAt(GetSecondOperand()->GetSymbolValue(kwoObject));
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRBiasedTargetValue

KWDRBiasedTargetValue::KWDRBiasedTargetValue()
{
	SetName("BiasedTargetValue");
	SetLabel("Biased target value");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Classifier");
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("Vector");
	GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	GetSecondOperand()->SetDerivationRule(new KWDRContinuousVector);
}

KWDRBiasedTargetValue::~KWDRBiasedTargetValue() {}

KWDerivationRule* KWDRBiasedTargetValue::Create() const
{
	return new KWDRBiasedTargetValue;
}

Symbol KWDRBiasedTargetValue::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KWDRClassifier* classifier;
	KWDRContinuousVector* cvOffsets;

	classifier = cast(KWDRClassifier*, GetFirstOperand()->GetStructureValue(kwoObject));
	cvOffsets = cast(KWDRContinuousVector*, GetSecondOperand()->GetStructureValue(kwoObject));
	return classifier->ComputeBiasedTargetValue(cvOffsets->GetValues());
}

boolean KWDRBiasedTargetValue::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDRContinuousVector* biasContinuousVectorRule;

	// Appel a la method ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification du vecteur de biais
	if (bOk)
	{
		biasContinuousVectorRule =
		    cast(KWDRContinuousVector*, GetSecondOperand()->GetReferencedDerivationRule(kwcOwnerClass));

		// Le vecteur de poids doit etre constant
		if (not biasContinuousVectorRule->CheckConstantOperands(true))
		{
			bOk = false;
			AddError("Bias values in operand 2 should be constants");
		}
	}
	return bOk;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRRankRegressor

KWDRRankRegressor::KWDRRankRegressor()
{
	SetName("RankRegressor");
	SetLabel("Rank regressor");
	SetType(KWType::Structure);
	SetStructureName("RankRegressor");
}

KWDRRankRegressor::~KWDRRankRegressor() {}

KWDerivationRule* KWDRRankRegressor::Create() const
{
	return new KWDRRankRegressor;
}

Continuous KWDRRankRegressor::ComputeTargetRankMean() const
{
	return 0.5;
}

Continuous KWDRRankRegressor::ComputeTargetRankStandardDeviation() const
{
	return (Continuous)sqrt(1.0 / 12);
}

Continuous KWDRRankRegressor::ComputeTargetRankDensityAt(Continuous cRank) const
{
	return 1;
}

Continuous KWDRRankRegressor::ComputeTargetRankCumulativeProbAt(Continuous cRank) const
{
	return cRank;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetRankMean

KWDRTargetRankMean::KWDRTargetRankMean()
{
	SetName("TargetRankMean");
	SetLabel("Mean rank");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("RankRegressor");
}

KWDRTargetRankMean::~KWDRTargetRankMean() {}

KWDerivationRule* KWDRTargetRankMean::Create() const
{
	return new KWDRTargetRankMean;
}

Continuous KWDRTargetRankMean::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRRankRegressor* rankRegressor;

	rankRegressor = cast(KWDRRankRegressor*, GetFirstOperand()->GetStructureValue(kwoObject));
	return rankRegressor->ComputeTargetRankMean();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetRankStandardDeviation

KWDRTargetRankStandardDeviation::KWDRTargetRankStandardDeviation()
{
	SetName("TargetRankStandardDeviation");
	SetLabel("Rank standard deviation");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("RankRegressor");
}

KWDRTargetRankStandardDeviation::~KWDRTargetRankStandardDeviation() {}

KWDerivationRule* KWDRTargetRankStandardDeviation::Create() const
{
	return new KWDRTargetRankStandardDeviation;
}

Continuous KWDRTargetRankStandardDeviation::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRRankRegressor* rankRegressor;

	rankRegressor = cast(KWDRRankRegressor*, GetFirstOperand()->GetStructureValue(kwoObject));
	return rankRegressor->ComputeTargetRankStandardDeviation();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetRankDensityAt

KWDRTargetRankDensityAt::KWDRTargetRankDensityAt()
{
	SetName("TargetRankDensityAt");
	SetLabel("Rank density");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("RankRegressor");
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTargetRankDensityAt::~KWDRTargetRankDensityAt() {}

KWDerivationRule* KWDRTargetRankDensityAt::Create() const
{
	return new KWDRTargetRankDensityAt;
}

Continuous KWDRTargetRankDensityAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRRankRegressor* rankRegressor;

	rankRegressor = cast(KWDRRankRegressor*, GetFirstOperand()->GetStructureValue(kwoObject));
	return rankRegressor->ComputeTargetRankDensityAt(GetSecondOperand()->GetContinuousValue(kwoObject));
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetRankCumulativeProbAt

KWDRTargetRankCumulativeProbAt::KWDRTargetRankCumulativeProbAt()
{
	SetName("TargetRankCumulativeProbAt");
	SetLabel("Rank cumulative prob");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("RankRegressor");
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTargetRankCumulativeProbAt::~KWDRTargetRankCumulativeProbAt() {}

KWDerivationRule* KWDRTargetRankCumulativeProbAt::Create() const
{
	return new KWDRTargetRankCumulativeProbAt;
}

Continuous KWDRTargetRankCumulativeProbAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRRankRegressor* rankRegressor;

	rankRegressor = cast(KWDRRankRegressor*, GetFirstOperand()->GetStructureValue(kwoObject));
	return rankRegressor->ComputeTargetRankCumulativeProbAt(GetSecondOperand()->GetContinuousValue(kwoObject));
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRRegressor

KWDRRegressor::KWDRRegressor()
{
	SetName("Regressor");
	SetLabel("Regressor");
	SetType(KWType::Structure);
	SetStructureName("Regressor");
}

KWDRRegressor::~KWDRRegressor() {}

KWDerivationRule* KWDRRegressor::Create() const
{
	return new KWDRRegressor;
}

Continuous KWDRRegressor::ComputeTargetMean() const
{
	return 0;
}

Continuous KWDRRegressor::ComputeTargetStandardDeviation() const
{
	return 0;
}

Continuous KWDRRegressor::ComputeTargetDensityAt(Continuous cValue) const
{
	return 0;
}

Symbol KWDRRegressor::ComputeTargetQuantileDistribution() const
{
	return Symbol();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetMean

KWDRTargetMean::KWDRTargetMean()
{
	SetName("TargetMean");
	SetLabel("Mean");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Regressor");
}

KWDRTargetMean::~KWDRTargetMean() {}

KWDerivationRule* KWDRTargetMean::Create() const
{
	return new KWDRTargetMean;
}

Continuous KWDRTargetMean::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRRegressor* regressor;

	regressor = cast(KWDRRegressor*, GetFirstOperand()->GetStructureValue(kwoObject));
	return regressor->ComputeTargetMean();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetStandardDeviation

KWDRTargetStandardDeviation::KWDRTargetStandardDeviation()
{
	SetName("TargetStandardDeviation");
	SetLabel("Standard deviation");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Regressor");
}

KWDRTargetStandardDeviation::~KWDRTargetStandardDeviation() {}

KWDerivationRule* KWDRTargetStandardDeviation::Create() const
{
	return new KWDRTargetStandardDeviation;
}

Continuous KWDRTargetStandardDeviation::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRRegressor* regressor;

	regressor = cast(KWDRRegressor*, GetFirstOperand()->GetStructureValue(kwoObject));
	return regressor->ComputeTargetStandardDeviation();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetDensityAt

KWDRTargetDensityAt::KWDRTargetDensityAt()
{
	SetName("TargetDensityAt");
	SetLabel(" density");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Regressor");
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTargetDensityAt::~KWDRTargetDensityAt() {}

KWDerivationRule* KWDRTargetDensityAt::Create() const
{
	return new KWDRTargetDensityAt;
}

Continuous KWDRTargetDensityAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRRegressor* regressor;

	regressor = cast(KWDRRegressor*, GetFirstOperand()->GetStructureValue(kwoObject));
	return regressor->ComputeTargetDensityAt(GetSecondOperand()->GetContinuousValue(kwoObject));
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetQuantileDistribution

KWDRTargetQuantileDistribution::KWDRTargetQuantileDistribution()
{
	SetName("TargetQuantileDistribution");
	SetLabel("Quantile distribution");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Regressor");
}

KWDRTargetQuantileDistribution::~KWDRTargetQuantileDistribution() {}

KWDerivationRule* KWDRTargetQuantileDistribution::Create() const
{
	return new KWDRTargetQuantileDistribution;
}

Symbol KWDRTargetQuantileDistribution::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KWDRRegressor* regressor;

	regressor = cast(KWDRRegressor*, GetFirstOperand()->GetStructureValue(kwoObject));
	return regressor->ComputeTargetQuantileDistribution();
}
