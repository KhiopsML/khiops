// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRNBPredictor.h"

void KWDRRegisterNBPredictorRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRNBClassifier);
	KWDerivationRule::RegisterDerivationRule(new KWDRSNBClassifier);
	KWDerivationRule::RegisterDerivationRule(new KWDRNBRankRegressor);
	KWDerivationRule::RegisterDerivationRule(new KWDRSNBRankRegressor);
	KWDerivationRule::RegisterDerivationRule(new KWDRNBRegressor);
	KWDerivationRule::RegisterDerivationRule(new KWDRSNBRegressor);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWNaiveBayesPredictorRuleHelper
KWNaiveBayesPredictorRuleHelper::KWNaiveBayesPredictorRuleHelper() {}

KWNaiveBayesPredictorRuleHelper::~KWNaiveBayesPredictorRuleHelper() {}

boolean KWNaiveBayesPredictorRuleHelper::RuleHasDataGridStatsAtOperand(const KWDerivationRule* predictorRule,
								       int nOperand) const
{
	require(0 <= nOperand and nOperand <= predictorRule->GetOperandNumber());
	return predictorRule->GetOperandAt(nOperand)->GetStructureName() == refDataGridStatsRule.GetName();
}

boolean KWNaiveBayesPredictorRuleHelper::RuleHasDataGridStatsBlockAtOperand(const KWDerivationRule* predictorRule,
									    int nOperand) const
{
	require(0 <= nOperand and nOperand <= predictorRule->GetOperandNumber());
	return predictorRule->GetOperandAt(nOperand)->GetStructureName() == refDataGridStatsBlockRule.GetName();
}

const ALString& KWNaiveBayesPredictorRuleHelper::GetDataGridStatsRuleName() const
{
	return refDataGridStatsRule.GetName();
}

const ALString& KWNaiveBayesPredictorRuleHelper::GetDataGridStatsBlockRuleName() const
{
	return refDataGridStatsBlockRule.GetName();
}

boolean KWNaiveBayesPredictorRuleHelper::CheckDataGridStatsOperandsType(const KWDerivationRule* predictorRule,
									int nFirstDataGridOperand,
									int nLastDataGridOperand) const
{
	boolean bOk = true;
	int nOperand;
	ALString sTmp;

	require(0 <= nFirstDataGridOperand and nFirstDataGridOperand <= nLastDataGridOperand and
		nLastDataGridOperand < predictorRule->GetOperandNumber());

	// Verification du nombre d'operands
	if (predictorRule->GetOperandNumber() < 1 + nFirstDataGridOperand)
	{
		bOk = false;
		predictorRule->AddError(sTmp + "Operand number must be at least " +
					IntToString(1 + nFirstDataGridOperand) + " (it is " +
					IntToString(predictorRule->GetOperandNumber()) + ")");
	}

	// Verification du type de structure de l'eventuel vecteur de poids en premier operande
	if (nFirstDataGridOperand == 1)
	{
		if (predictorRule->GetFirstOperand()->GetStructureName() != refContinuousVectorRule.GetName())
		{
			bOk = false;
			predictorRule->AddError(
			    sTmp + "Incorrect Structure(" + predictorRule->GetFirstOperand()->GetStructureName() +
			    ") " + "for first operand (must be " + refContinuousVectorRule.GetName() + ")");
		}
	}

	// Verification du type de structure des operandes de statistiques par grilles
	for (nOperand = nFirstDataGridOperand; nOperand <= nLastDataGridOperand; nOperand++)
	{
		if (not(RuleHasDataGridStatsAtOperand(predictorRule, nOperand) or
			RuleHasDataGridStatsBlockAtOperand(predictorRule, nOperand)))
		{
			bOk = false;
			predictorRule->AddError(
			    sTmp + "Incorrect Structure(" + predictorRule->GetOperandAt(nOperand)->GetStructureName() +
			    ") " + "for operand " + IntToString(nOperand + 1) + " (must be " +
			    refDataGridStatsRule.GetName() + " or " + refDataGridStatsBlockRule.GetName() + ")");
			break;
		}
	}

	return bOk;
}

boolean KWNaiveBayesPredictorRuleHelper::CheckDataGridStatsOperandsFrequencyAndTargetType(
    const KWClass* kwcOwnerClass, const KWDerivationRule* predictorRule, int nFirstDataGridOperand,
    int nLastDataGridOperand, const KWDRDataGrid* referenceDataGridRule) const
{
	boolean bOk = true;
	KWDRFrequencies* frequenciesRule;
	int nReferenceTotalFrequency;
	int nPredictorType;
	int nOperand;
	KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	KWDRDataGridStats* dataGridStatsRule;
	KWDRDataGridBlock* dataGridBlockRule;
	KWDRDataGrid* dataGridRule;
	int nDataGridTotalFrequency;
	ALString sTmp;

	// Obtention du type de la cible
	if (predictorRule->GetStructureName().Right(10) == "Classifier")
		nPredictorType = KWType::Symbol;
	else
	{
		assert(predictorRule->GetStructureName().Right(9) == "Regressor");
		nPredictorType = KWType::Continuous;
	}

	// Calcul de la frequence total de la regle data grid de reference
	frequenciesRule =
	    cast(KWDRFrequencies*, referenceDataGridRule->GetOperandAt(referenceDataGridRule->GetOperandNumber() - 1)
				       ->GetReferencedDerivationRule(kwcOwnerClass));
	nReferenceTotalFrequency = frequenciesRule->ComputeTotalFrequency();

	// Parcours des operandes de type stats par grille
	for (nOperand = nFirstDataGridOperand; nOperand <= nLastDataGridOperand; nOperand++)
	{
		// Verification basique de la regle referencee
		bOk = predictorRule->CheckReferencedDerivationRuleAt(nOperand, kwcOwnerClass, "");
		if (not bOk)
			break;

		// Initialisation des variables d'iteration
		dataGridStatsBlockRule = NULL;
		dataGridStatsRule = NULL;

		// Verification du type du dernier attribut de la grille utilise pour les stats
		// Cas grille simple
		if (RuleHasDataGridStatsAtOperand(predictorRule, nOperand))
		{
			dataGridStatsRule =
			    cast(KWDRDataGridStats*,
				 predictorRule->GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass));
			bOk = dataGridStatsRule->CheckPredictorCompleteness(nPredictorType, kwcOwnerClass);
		}
		// Cas block de grilles
		else
		{
			assert(RuleHasDataGridStatsBlockAtOperand(predictorRule, nOperand));
			dataGridStatsBlockRule =
			    cast(KWDRDataGridStatsBlock*,
				 predictorRule->GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass));
			bOk = dataGridStatsBlockRule->CheckPredictorCompleteness(nPredictorType, kwcOwnerClass);
		}
		if (not bOk)
			break;

		// Une seule regle doit etre valable
		assert(dataGridStatsRule != NULL or dataGridStatsBlockRule != NULL);
		assert(not(dataGridStatsRule != NULL and dataGridStatsBlockRule != NULL));

		// Verification que l'effectif total de la grille est coherent avec celui de la reference
		// Cas grille simple
		if (dataGridStatsRule != NULL)
		{
			dataGridRule =
			    cast(KWDRDataGrid*,
				 dataGridStatsRule->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
			nDataGridTotalFrequency = dataGridRule->ComputeUncheckedTotalFrequency();
			if (nDataGridTotalFrequency != nReferenceTotalFrequency)
			{
				AddError(sTmp + "Total frequency of the data grid operand " +
					 IntToString(nOperand - nFirstDataGridOperand + 1) + " (" +
					 IntToString(nDataGridTotalFrequency) + ") " +
					 "does not match that of the reference data grid operand (" +
					 IntToString(nReferenceTotalFrequency) + "). " +
					 "The reference data grid is the last operand in classification and the first "
					 "in regression");
				bOk = false;
				break;
			}
		}
		// Cas bloc de grilles
		else
		{
			dataGridBlockRule =
			    cast(KWDRDataGridBlock*,
				 dataGridStatsBlockRule->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
			nDataGridTotalFrequency = dataGridBlockRule->ComputeUncheckedTotalFrequency(kwcOwnerClass);
			if (nDataGridTotalFrequency != nReferenceTotalFrequency)
			{
				AddError(sTmp + "Total frequency of data grid block operand " +
					 IntToString(nOperand - nFirstDataGridOperand + 1) + " (" +
					 IntToString(nDataGridTotalFrequency) + ") " +
					 "does not match that of the reference data grid operand (" +
					 IntToString(nReferenceTotalFrequency) + "). " +
					 "The reference data grid is the last operand in classification and the first "
					 "in regression");
				bOk = false;
				break;
			}
		}
	}
	return bOk;
}

void KWNaiveBayesPredictorRuleHelper::CompileInitializeDataGridAndWeightRules(
    const KWClass* kwcOwnerClass, const KWDerivationRule* predictorRule, int nFirstDataGridOperand,
    int nLastDataGridOperand, ObjectArray* oaDataGridStatsAndBlockRules, ContinuousVector* cvWeights,
    IntVector* ivIsDataGridStatsRule) const
{
	int nDataGridStatsNumber;
	int nOperand;
	const KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	int nDataGridRule;
	const KWDRContinuousVector* continuousVectorRule;

	// Memorisation :
	// - des regles data grid stats et data grid stats bloc
	// - des indexes ou il y a des KWDRDataGridStatsBlock
	nDataGridStatsNumber = 0;
	oaDataGridStatsAndBlockRules->SetSize(0);
	ivIsDataGridStatsRule->SetSize(0);
	for (nOperand = nFirstDataGridOperand; nOperand <= nLastDataGridOperand; nOperand++)
	{
		assert(RuleHasDataGridStatsAtOperand(predictorRule, nOperand) or
		       RuleHasDataGridStatsBlockAtOperand(predictorRule, nOperand));
		oaDataGridStatsAndBlockRules->Add(
		    predictorRule->GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass));
		if (RuleHasDataGridStatsAtOperand(predictorRule, nOperand))
		{
			nDataGridStatsNumber += 1;
			ivIsDataGridStatsRule->Add(true);
		}
		else
		{
			assert(RuleHasDataGridStatsBlockAtOperand(predictorRule, nOperand));
			ivIsDataGridStatsRule->Add(false);
			dataGridStatsBlockRule =
			    cast(const KWDRDataGridStatsBlock*,
				 predictorRule->GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass));
			nDataGridStatsNumber +=
			    dataGridStatsBlockRule->GetDataGridBlock()->GetUncheckedDataGridNumber();
		}
	}

	// Initialisation des poids pour le calcul
	// Cas sans vecteur de poids : Mise a 1 de tous les poids
	cvWeights->SetSize(nDataGridStatsNumber);
	if (nFirstDataGridOperand == 0)
	{
		for (nDataGridRule = 0; nDataGridRule < nDataGridStatsNumber; nDataGridRule++)
			cvWeights->SetAt(nDataGridRule, 1);
	}
	// Cas avec vecteur de poids : Memorisation des poids depuis le premier operande
	else
	{
		continuousVectorRule = cast(const KWDRContinuousVector*,
					    predictorRule->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
		assert(continuousVectorRule->GetValueNumber() == nDataGridStatsNumber);
		for (nDataGridRule = 0; nDataGridRule < nDataGridStatsNumber; nDataGridRule++)
			cvWeights->SetAt(nDataGridRule, continuousVectorRule->GetValueAt(nDataGridRule));
	}
	ensure(nDataGridStatsNumber == cvWeights->GetSize());
	ensure(nDataGridStatsNumber >= oaDataGridStatsAndBlockRules->GetSize());
	ensure(oaDataGridStatsAndBlockRules->GetSize() == ivIsDataGridStatsRule->GetSize());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRNBClassifier

KWDRNBClassifier::KWDRNBClassifier()
{
	SetName("NBClassifier");
	SetLabel("Naive Bayes classifier");
	SetType(KWType::Structure);
	SetStructureName("Classifier");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);

	// Les operandes contiennent des regles de type Structure
	nFirstDataGridOperand = 0;
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("DataGridStats");

	// Gestion de l'optimisation
	cUnknownTargetProb = 0;
	nOptimizationFreshness = 0;
}

KWDRNBClassifier::~KWDRNBClassifier() {}

KWDerivationRule* KWDRNBClassifier::Create() const
{
	return new KWDRNBClassifier;
}

boolean KWDRNBClassifier::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());

	// Verification en tant que predicteur SNB
	if (GetName() == "SNBClassifier" or GetName() == "SNBRegressor")
	{
		assert(GetOperandNumber() >= 3);
		bOk = bOk and naiveBayesPredictorRuleHelper.CheckDataGridStatsOperandsType(this, nFirstDataGridOperand,
											   GetOperandNumber() - 2);
	}

	// Verification que le dernier operande est une structure de type data grid univarie
	if (bOk and GetOperandNumber() > 0)
	{
		if (naiveBayesPredictorRuleHelper.RuleHasDataGridStatsAtOperand(this, GetOperandNumber() - 1))
		{
			bOk = false;
			AddError(sTmp + "Incorrect Structure(" +
				 GetOperandAt(GetOperandNumber() - 1)->GetStructureName() + ") " +
				 "for last operand (must be " +
				 naiveBayesPredictorRuleHelper.GetDataGridStatsRuleName() + ")");
		}
	}
	return bOk;
}

boolean KWDRNBClassifier::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDRDataGrid refDataGridRule;
	KWDRDataGrid* targetDataGridRule;
	KWDRUnivariatePartition* univariatePartitionRule;
	KWDRSymbolValueSet symbolValueSetRefRule;
	int nOperand;
	KWDRDataGridStats* dataGridStatsRule;
	KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	KWDRDataGrid* dataGridRule;
	KWDRSymbolValueSet* symbolValueSetRule;
	KWDRSymbolValueSet refCheckedSymbolValueSetRule;
	KWDRDataGridBlock* dataGridBlockRule;
	int nDataGrid;
	ALString sStringVarKey;
	ALString sTmp;

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Analyse du dernier operande qui fournit la distribution des valeurs cibles
	if (bOk)
		bOk = CheckReferencedDerivationRuleAt(GetOperandNumber() - 1, kwcOwnerClass, refDataGridRule.GetName());
	targetDataGridRule = NULL;
	if (bOk)
	{
		targetDataGridRule = cast(
		    KWDRDataGrid*, GetOperandAt(GetOperandNumber() - 1)->GetReferencedDerivationRule(kwcOwnerClass));
		if (targetDataGridRule->GetOperandNumber() != 2)
		{
			AddError(sTmp + "Last operand rule must have only two operands");
			bOk = false;
		}
		else if (targetDataGridRule->GetUncheckedAttributeTypeAt(0) != KWType::Symbol)
		{
			AddError(sTmp + "Last operand must be a valid univariate categorical data grid");
			bOk = false;
		}
		else if (targetDataGridRule->GetUncheckedAttributeTypeAt(0))
		{
			// L'erreur sur la grille doit etre diagniostique par ailleurs
			bOk = false;
		}
		else
		{
			univariatePartitionRule =
			    cast(KWDRUnivariatePartition*,
				 targetDataGridRule->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));

			// Verification du type de la partition cible
			if (univariatePartitionRule->GetStructureName() != symbolValueSetRefRule.GetStructureName())
			{
				AddError(sTmp + "Type of target partition (" +
					 univariatePartitionRule->GetStructureName() + ") should be " +
					 symbolValueSetRefRule.GetStructureName());
				bOk = false;
			}
		}
	}

	// Verification de la compatibilite des arguments avec la classification
	if (bOk)
	{
		bOk = naiveBayesPredictorRuleHelper.CheckDataGridStatsOperandsFrequencyAndTargetType(
		    kwcOwnerClass, this, nFirstDataGridOperand, GetOperandNumber() - 2, targetDataGridRule);
	}

	// Memorisation des valeurs cibles, en transferant les valeurs vers la representation structuree
	// La representation de la regle a verifier est structuree ou non selon qu'elle a a ete fabriquee
	// par programme (pour un classifieur) ou qu'elle est issue d'une lecture de fichier dictionnaire
	// (avant compilation). On passe alors par une regle temporaire (refCheckedSymbolValueSetRule)
	//  permettant d'avoir un acces systematique a la represnetation structuree, ce qui permet d'effectuer
	// des controles sans modifier (par compilation ou autre) la regle a verifier
	if (bOk)
	{
		symbolValueSetRule =
		    cast(KWDRSymbolValueSet*,
			 targetDataGridRule->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));

		// La methode suivante force la representation structuree
		refCheckedSymbolValueSetRule.SetValueNumber(symbolValueSetRule->GetValueNumber());

		// Transfert vers une representation structuree
		if (not symbolValueSetRule->GetStructureInterface())
			refCheckedSymbolValueSetRule.BuildStructureFromBase(symbolValueSetRule);
		else
			refCheckedSymbolValueSetRule.CopyStructureFrom(symbolValueSetRule);
	}

	// Verification que les valeurs cibles sont les memes pour chaque grille our bloc de grilles
	if (bOk)
	{
		for (nOperand = nFirstDataGridOperand; nOperand < GetOperandNumber() - 1; nOperand++)
		{
			// Cas grille simple
			if (naiveBayesPredictorRuleHelper.RuleHasDataGridStatsAtOperand(this, nOperand))
			{
				// Access a la regle data grid reference
				dataGridStatsRule =
				    cast(KWDRDataGridStats*,
					 GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass));
				dataGridRule = cast(
				    KWDRDataGrid*,
				    dataGridStatsRule->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));

				// Verification des valeurs cibles du data grid par rapport a la reference
				sStringVarKey = "";
				CheckOperandsCompletenessForTargetValues(kwcOwnerClass, dataGridRule,
									 nOperand - nFirstDataGridOperand + 1,
									 sStringVarKey, &refCheckedSymbolValueSetRule);
			}
			// Cas bloc des grilles
			else
			{
				assert(
				    naiveBayesPredictorRuleHelper.RuleHasDataGridStatsBlockAtOperand(this, nOperand));

				// Access au data grid bloc reference
				dataGridStatsBlockRule =
				    cast(KWDRDataGridStatsBlock*,
					 GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass));
				dataGridBlockRule =
				    cast(KWDRDataGridBlock*,
					 dataGridStatsBlockRule->GetFirstOperand()->GetReferencedDerivationRule(
					     kwcOwnerClass));
				// Verification des valeurs cibles des regles des grilles du bloc
				for (nDataGrid = 1; nDataGrid < dataGridBlockRule->GetOperandNumber(); nDataGrid++)
				{
					dataGridRule = cast(
					    KWDRDataGrid*,
					    dataGridBlockRule->GetOperandAt(nDataGrid)->GetReferencedDerivationRule(
						kwcOwnerClass));

					// Memorisation de la varkey comme string
					if (dataGridBlockRule->GetUncheckedDataGridVarKeyType() == KWType::Continuous)
					{
						sStringVarKey =
						    sTmp + KWContinuous::ContinuousToString(
							       dataGridBlockRule->GetUncheckedContinuousVarKeyAt(
								   nDataGrid - 1));
					}
					else
						sStringVarKey =
						    sTmp + dataGridBlockRule->GetUncheckedSymbolVarKeyAt(nDataGrid - 1)
							       .GetValue();

					// Verification des valeurs cibles du data grid par rapport a la reference
					CheckOperandsCompletenessForTargetValues(
					    kwcOwnerClass, dataGridRule, nOperand - nFirstDataGridOperand + 1,
					    sStringVarKey, &refCheckedSymbolValueSetRule);
				}
			}
		}
	}
	return bOk;
}

boolean
KWDRNBClassifier::CheckOperandsCompletenessForTargetValues(const KWClass* kwcOwnerClass, KWDRDataGrid* dataGridRule,
							   int nDataGridStatsOperandIndex, ALString& sStringVarKey,
							   const KWDRSymbolValueSet* refCheckedSymbolValueSetRule) const
{
	boolean bOk = true;
	ALString sDataGridIdentifierMessage;
	KWDRUnivariatePartition* univariatePartitionRule;
	KWDRSymbolValueSet* symbolValueSetRule;
	KWDRValueGroups* symbolValueGroupsRule;
	KWDRSymbolValueSet refSymbolValueSetRule;
	KWDRValueGroups refSymbolValueGroupsRule;
	KWDRSymbolValueSet checkedSymbolValueSetRule;
	int nValue;
	Symbol sValue;
	NumericKeyDictionary nkdCheckedValues;
	int nGroup;
	KWDRValueGroup* symbolValueGroupRule;
	KWDRValueGroup checkedSymbolValueGroupRule;
	ALString sTmp;

	// Creation du message d'operand conditionel a son appartenance a un bloc
	if (sStringVarKey == "")
		sDataGridIdentifierMessage = sTmp + "data grid operand " + IntToString(nDataGridStatsOperandIndex);
	else
		sDataGridIdentifierMessage = sTmp + "data grid with VarKey=" + sStringVarKey +
					     " at data grid block operand " + IntToString(nDataGridStatsOperandIndex);

	// Recherche des valeurs cibles et de leur effectif
	univariatePartitionRule =
	    cast(KWDRUnivariatePartition*, dataGridRule->GetOperandAt(dataGridRule->GetOperandNumber() - 2)
					       ->GetReferencedDerivationRule(kwcOwnerClass));
	assert(univariatePartitionRule->GetAttributeType() == KWType::Symbol);

	// Test du type de partition cible: soit ensemble de valeurs, soit ensemble de groupes de valeurs
	symbolValueSetRule = NULL;
	symbolValueGroupsRule = NULL;
	if (univariatePartitionRule->GetStructureName() == refSymbolValueSetRule.GetStructureName())
		symbolValueSetRule = cast(KWDRSymbolValueSet*, univariatePartitionRule);
	else if (univariatePartitionRule->GetStructureName() == refSymbolValueGroupsRule.GetStructureName())
		symbolValueGroupsRule = cast(KWDRValueGroups*, univariatePartitionRule);
	else
	{
		AddError(sTmp + "Type of target partition for " + sDataGridIdentifierMessage + " (" +
			 univariatePartitionRule->GetStructureName() + ") " + "should be " +
			 refSymbolValueSetRule.GetStructureName() + " or " +
			 refSymbolValueGroupsRule.GetStructureName());
		bOk = false;
	}
	assert((symbolValueSetRule == NULL and symbolValueGroupsRule != NULL) or
	       (symbolValueSetRule != NULL and symbolValueGroupsRule == NULL));

	// Cas d'une grille definie par un ensemble de valeurs
	if (symbolValueSetRule != NULL)
	{
		// Memorisation des valeurs cibles, en les transferant vers une representation structuree
		checkedSymbolValueSetRule.SetValueNumber(symbolValueSetRule->GetValueNumber());
		if (not symbolValueSetRule->GetStructureInterface())
			checkedSymbolValueSetRule.BuildStructureFromBase(symbolValueSetRule);
		else
			checkedSymbolValueSetRule.CopyStructureFrom(symbolValueSetRule);

		// Verification du nombre de valeurs cibles
		if (checkedSymbolValueSetRule.GetValueNumber() != refCheckedSymbolValueSetRule->GetValueNumber())
		{
			AddError(sTmp + "Size of target partition of " + sDataGridIdentifierMessage + " (" +
				 IntToString(checkedSymbolValueSetRule.GetValueNumber()) + ") " +
				 "should be equal to " + IntToString(refCheckedSymbolValueSetRule->GetValueNumber()));
			bOk = false;
		}
		// Verification de chaque valeur cibles
		else
		{
			for (nValue = 0; nValue < checkedSymbolValueSetRule.GetValueNumber(); nValue++)
			{
				if (checkedSymbolValueSetRule.GetValueAt(nValue) !=
				    refCheckedSymbolValueSetRule->GetValueAt(nValue))
				{
					AddError(sTmp + "Unexpected target value (" +
						 checkedSymbolValueSetRule.GetValueAt(nValue) + ") " + "at " +
						 sDataGridIdentifierMessage);
					bOk = false;
					break;
				}
			}
		}
	}
	// Cas d'une grille definie par un ensemble de groupes de valeurs
	else if (symbolValueGroupsRule != NULL)
	{
		// Rangement des valeurs cibles dans un dictionnaire
		for (nValue = 0; nValue < refCheckedSymbolValueSetRule->GetValueNumber(); nValue++)
		{
			sValue = refCheckedSymbolValueSetRule->GetValueAt(nValue);
			nkdCheckedValues.SetAt(sValue.GetNumericKey(), &nkdCheckedValues);
		}

		// Verification des groupes
		for (nGroup = 0; nGroup < symbolValueGroupsRule->GetValueGroupNumber(); nGroup++)
		{
			symbolValueGroupRule = symbolValueGroupsRule->GetValueGroupAt(nGroup);

			// Memorisation des valeurs du groupe, en transferant les valeurs vers la representation structuree
			checkedSymbolValueGroupRule.SetValueNumber(symbolValueGroupRule->GetValueNumber());
			if (not symbolValueGroupRule->GetStructureInterface())
				checkedSymbolValueGroupRule.BuildStructureFromBase(symbolValueGroupRule);
			else
				checkedSymbolValueGroupRule.CopyStructureFrom(symbolValueGroupRule);

			// Verification des valeurs du groupes (sauf pour la valeur speciale)
			for (nValue = 0; nValue < checkedSymbolValueGroupRule.GetValueNumber(); nValue++)
			{
				sValue = checkedSymbolValueGroupRule.GetValueAt(nValue);
				if (sValue != Symbol::GetStarValue() and
				    nkdCheckedValues.Lookup(sValue.GetNumericKey()) == NULL)
				{
					AddError(sTmp + "Unexpected target value (" +
						 checkedSymbolValueGroupRule.GetValueAt(nValue) + ") " +
						 "in a group of values of " + sDataGridIdentifierMessage);
					bOk = false;
					break;
				}
			}
		}
	}
	return bOk;
}

Object* KWDRNBClassifier::ComputeStructureResult(const KWObject* kwoObject) const
{
	int nOperand;

	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	// On evalue les operandes
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		GetOperandAt(nOperand)->GetStructureValue(kwoObject);

	// Calcul du vecteur de probabilites conditionnelles
	ComputeTargetProbs();

	return (Object*)this;
}

Symbol KWDRNBClassifier::ComputeTargetValue() const
{
	int nTarget;
	int nBestTarget;
	Continuous cMaxProb;

	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Recherche de l'index de la probabilite maximale
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

Continuous KWDRNBClassifier::ComputeTargetProb() const
{
	int nTarget;
	Continuous cMaxProb;

	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Recherche de l'index de la probabilites max
	cMaxProb = 0;
	for (nTarget = 0; nTarget < cvTargetProbs.GetSize(); nTarget++)
	{
		if (cvTargetProbs.GetAt(nTarget) > cMaxProb)
			cMaxProb = cvTargetProbs.GetAt(nTarget);
	}
	return cMaxProb;
}

Continuous KWDRNBClassifier::ComputeTargetProbAt(const Symbol& sValue) const
{
	int nTarget;

	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

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

Symbol KWDRNBClassifier::ComputeBiasedTargetValue(const ContinuousVector* cvOffsets) const
{
	int nTarget;
	int nBestTarget;
	Continuous cMaxScore;
	Continuous cOffset;

	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());
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

void KWDRNBClassifier::Compile(KWClass* kwcOwnerClass)
{
	boolean bLocalTrace = false;
	int nDataGridRuleNumber;
	const KWDRDataGrid* targetDataGridRule;
	int nTargetValueNumber;
	const KWDRSymbolValueSet* targetSymbolValueSetRule;
	int nTargetValue;
	int nDataGridRule;
	int nDataGridOrBlockRule;
	int nDataGridOrBlockOperand;
	const KWDRUnivariatePartition* univariatePartitionRule;
	int nOperandDataGridRule;
	int nOperand;
	KWDRDataGridStats* dataGridStatsRule;
	const KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	ObjectArray oaOperandDataGridRules;
	KWDRDataGridBlock* dataGridBlockRule;
	KWDRDataGrid* dataGridRule;
	const KWDRDataGridStats* constDataGridStatsRule;
	ObjectArray oaOperandDataGridStatsRules;
	int nMissingValueIndex;

	// Appel de la methode ancetre (compile les operands)
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Initialisation des data grid et poids
		naiveBayesPredictorRuleHelper.CompileInitializeDataGridAndWeightRules(
		    kwcOwnerClass, this, nFirstDataGridOperand, GetOperandNumber() - 2, &oaDataGridStatsAndBlockRules,
		    &cvWeights, &ivIsDataGridStatsRule);
		nDataGridRuleNumber = cvWeights.GetSize();

		// Recherche du dernier operande: distribution des valeurs cibles
		targetDataGridRule =
		    cast(const KWDRDataGrid*,
			 GetOperandAt(GetOperandNumber() - 1)->GetReferencedDerivationRule(kwcOwnerClass));
		nTargetValueNumber = targetDataGridRule->GetTotalCellNumber();

		// Initialisation du vecteur des valeurs cibles
		targetSymbolValueSetRule =
		    cast(const KWDRSymbolValueSet*,
			 targetDataGridRule->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
		assert(targetSymbolValueSetRule->GetValueNumber() == nTargetValueNumber);
		svTargetValues.SetSize(nTargetValueNumber);
		for (nTargetValue = 0; nTargetValue < nTargetValueNumber; nTargetValue++)
			svTargetValues.SetAt(nTargetValue, targetSymbolValueSetRule->GetValueAt(nTargetValue));

		// Initialisation des probabilites cibles
		cvTargetProbs.SetSize(nTargetValueNumber);
		cUnknownTargetProb = 0;

		// Memorisation des effectifs par partie cible
		ivDataGridSetTargetFrequencies.SetSize(nTargetValueNumber);
		for (nTargetValue = 0; nTargetValue < nTargetValueNumber; nTargetValue++)
			ivDataGridSetTargetFrequencies.SetAt(nTargetValue,
							     targetDataGridRule->GetCellFrequencyAt(nTargetValue));

		// Calcul de la correspondance entre les index cibles de l'ensemble des grille et les index cibles de chaque grille
		ivDataGridTargetIndexes.SetSize(ivDataGridSetTargetFrequencies.GetSize() * nDataGridRuleNumber);
		dvMissingLogProbas.SetSize(ivDataGridSetTargetFrequencies.GetSize() * nDataGridRuleNumber);
		dvMissingLogProbas.Initialize();
		nDataGridRule = 0;
		for (nDataGridOrBlockRule = 0; nDataGridOrBlockRule < oaDataGridStatsAndBlockRules.GetSize();
		     nDataGridOrBlockRule++)
		{
			nDataGridOrBlockOperand = nFirstDataGridOperand + nDataGridOrBlockRule;
			assert(naiveBayesPredictorRuleHelper.RuleHasDataGridStatsAtOperand(this,
											   nDataGridOrBlockOperand) or
			       naiveBayesPredictorRuleHelper.RuleHasDataGridStatsBlockAtOperand(
				   this, nDataGridOrBlockOperand));

			// Memorisation dans un tableau la seule grille de la regle ou les grilles du bloc de l'operand
			oaOperandDataGridRules.SetSize(0);
			oaOperandDataGridStatsRules.SetSize(0);
			if (naiveBayesPredictorRuleHelper.RuleHasDataGridStatsAtOperand(this, nDataGridOrBlockOperand))
			{
				dataGridStatsRule =
				    cast(KWDRDataGridStats*, oaDataGridStatsAndBlockRules.GetAt(nDataGridOrBlockRule));
				dataGridRule = cast(
				    KWDRDataGrid*,
				    dataGridStatsRule->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
				oaOperandDataGridRules.Add(dataGridRule);
				oaOperandDataGridStatsRules.Add(dataGridStatsRule);
			}
			else
			{
				dataGridStatsBlockRule = cast(KWDRDataGridStatsBlock*,
							      oaDataGridStatsAndBlockRules.GetAt(nDataGridOrBlockRule));
				dataGridBlockRule =
				    cast(KWDRDataGridBlock*,
					 dataGridStatsBlockRule->GetFirstOperand()->GetReferencedDerivationRule(
					     kwcOwnerClass));
				for (nOperand = 1; nOperand < dataGridBlockRule->GetOperandNumber(); nOperand++)
				{
					dataGridRule =
					    cast(KWDRDataGrid*,
						 dataGridBlockRule->GetOperandAt(nOperand)->GetReferencedDerivationRule(
						     kwcOwnerClass));
					oaOperandDataGridRules.Add(dataGridRule);
				}
				for (nOperand = 0; nOperand < dataGridStatsBlockRule->GetDataGridStatsNumber();
				     nOperand++)
				{
					constDataGridStatsRule =
					    dataGridStatsBlockRule->GetDataGridStatsAtBlockIndex(nOperand);
					oaOperandDataGridStatsRules.Add(
					    const_cast<KWDRDataGridStats*>(constDataGridStatsRule));
				}
			}

			// Memorisation des indexes des cibles pour chacune des grilles de l'operand
			for (nOperandDataGridRule = 0; nOperandDataGridRule < oaOperandDataGridRules.GetSize();
			     nOperandDataGridRule++)
			{
				dataGridRule = cast(KWDRDataGrid*, oaOperandDataGridRules.GetAt(nOperandDataGridRule));
				dataGridStatsRule =
				    cast(KWDRDataGridStats*, oaOperandDataGridStatsRules.GetAt(nOperandDataGridRule));

				// Recherche de la partition cible de la grille
				univariatePartitionRule =
				    cast(const KWDRUnivariatePartition*,
					 dataGridRule->GetOperandAt(dataGridRule->GetOperandNumber() - 2)
					     ->GetReferencedDerivationRule(kwcOwnerClass));
				assert(univariatePartitionRule->GetAttributeType() == KWType::Symbol);
				assert(univariatePartitionRule->IsCompiled());

				// Calcul de l'index de la grille ou tombent les valeurs manquantes
				nMissingValueIndex = -1;
				if (dataGridRule->GetAttributeTypeAt(0) == KWType::Continuous)
					nMissingValueIndex = dataGridRule->GetContinuousAttributePartIndexAt(
					    0, KWContinuous::GetMissingValue());
				else
					nMissingValueIndex = dataGridRule->GetSymbolAttributePartIndexAt(0, Symbol(""));

				// Calcul de la correspondance entre index cible et index des partie cible de la grille
				// Calcul de la log-vraisemblance dans le cas des valeurs manquantes seulement pour les DataGridStatsBlock
				for (nTargetValue = 0; nTargetValue < nTargetValueNumber; nTargetValue++)
				{
					ivDataGridTargetIndexes.SetAt(nDataGridRule * nTargetValueNumber + nTargetValue,
								      univariatePartitionRule->GetSymbolPartIndex(
									  svTargetValues.GetAt(nTargetValue)));
					if (not naiveBayesPredictorRuleHelper.RuleHasDataGridStatsAtOperand(
						this, nDataGridOrBlockOperand))
					{
						dvMissingLogProbas.SetAt(
						    nDataGridRule * nTargetValueNumber + nTargetValue,
						    dataGridStatsRule->GetDataGridSourceConditionalLogProbAt(
							nMissingValueIndex, nTargetValue));
					}
				}
				nDataGridRule++;
			}
		}
		assert(nDataGridRule == nDataGridRuleNumber);

		// Calcul de la valeur de reference de la log-vraisemblance pour chaque cible et grille
		dvMissingScores.SetSize(nTargetValueNumber);
		dvMissingScores.Initialize();
		for (nTargetValue = 0; nTargetValue < nTargetValueNumber; nTargetValue++)
		{
			for (nDataGridRule = 0; nDataGridRule < nDataGridRuleNumber; nDataGridRule++)
			{
				dvMissingScores.UpgradeAt(
				    nTargetValue,
				    cvWeights.GetAt(nDataGridRule) *
					dvMissingLogProbas.GetAt(nDataGridRule * nTargetValueNumber + nTargetValue));
			}
		}
		assert(ivDataGridTargetIndexes.GetSize() == nDataGridRuleNumber * nTargetValueNumber);
		assert(dvMissingLogProbas.GetSize() == nDataGridRuleNumber * nTargetValueNumber);

		// Trace de debogage
		if (bLocalTrace)
		{
			cout << "DataGridSet\n";
			cout << "DataGrid and DataGridBlock rule number\t" << oaDataGridStatsAndBlockRules.GetSize()
			     << endl;
			cout << "DataGrid total number\t" << nDataGridRuleNumber << endl;

			// Affichage des effectifs  de la cible
			cout << "Target frequencies\n";
			cout << "DataGridSet";
			for (nDataGridRule = 0; nDataGridRule < nDataGridRuleNumber; nDataGridRule++)
				cout << "\tDataGrid " << nDataGridRule;
			cout << endl;
			for (nTargetValue = 0; nTargetValue < nTargetValueNumber; nTargetValue++)
			{
				cout << ivDataGridSetTargetFrequencies.GetAt(nTargetValue);
				for (nDataGridRule = 0; nDataGridRule < nDataGridRuleNumber; nDataGridRule++)
					cout << "\t"
					     << ivDataGridTargetIndexes.GetAt(nDataGridRule * nTargetValueNumber +
									      nTargetValue);
				cout << endl;
			}
			cout << "MissingLogProbas";
			for (nDataGridRule = 0; nDataGridRule < nDataGridRuleNumber; nDataGridRule++)
				cout << "\tDataGrid " << nDataGridRule;
			cout << endl;
			for (nTargetValue = 0; nTargetValue < nTargetValueNumber; nTargetValue++)
			{
				for (nDataGridRule = 0; nDataGridRule < nDataGridRuleNumber; nDataGridRule++)
					cout << "\t"
					     << dvMissingLogProbas.GetAt(nDataGridRule * nTargetValueNumber +
									 nTargetValue);
				cout << endl;
			}
			cout << "MissingScores" << endl;
			for (nTargetValue = 0; nTargetValue < nTargetValueNumber; nTargetValue++)
				cout << dvMissingScores.GetAt(nTargetValue) << endl;
		}
		assert(nDataGridRuleNumber >= oaDataGridStatsAndBlockRules.GetSize());
	}
}

int KWDRNBClassifier::GetDataGridStatsOrBlockNumber() const
{
	require(IsOptimized());
	return oaDataGridStatsAndBlockRules.GetSize();
}

int KWDRNBClassifier::GetDataGridStatsNumber() const
{
	require(IsOptimized());
	return cvWeights.GetSize();
}

boolean KWDRNBClassifier::IsDataGridStatsAt(int nDataGridStatsOrBlock) const
{
	require(IsOptimized());
	require(0 <= nDataGridStatsOrBlock and nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize());

	return ivIsDataGridStatsRule.GetAt(nDataGridStatsOrBlock);
}

const KWDRDataGridStats* KWDRNBClassifier::GetDataGridStatsAt(int nDataGridStatsOrBlock) const
{
	require(IsOptimized());
	require(0 <= nDataGridStatsOrBlock and nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize());
	require(IsDataGridStatsAt(nDataGridStatsOrBlock));
	return cast(KWDRDataGridStats*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));
}

const KWDRDataGridStatsBlock* KWDRNBClassifier::GetDataGridStatsBlockAt(int nDataGridStatsOrBlock) const
{
	require(IsOptimized());
	require(0 <= nDataGridStatsOrBlock and nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize());
	require(not IsDataGridStatsAt(nDataGridStatsOrBlock));
	return cast(KWDRDataGridStatsBlock*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));
}

Continuous KWDRNBClassifier::GetDataGridWeightAt(int nDataGrid) const
{
	require(IsOptimized());
	require(0 <= nDataGrid and nDataGrid < cvWeights.GetSize());
	return cvWeights.GetAt(nDataGrid);
}

int KWDRNBClassifier::GetDataGridSetTargetPartNumber() const
{
	require(IsOptimized());
	return ivDataGridSetTargetFrequencies.GetSize();
}

int KWDRNBClassifier::GetDataGridSetTargetFrequencyAt(int nTarget) const
{
	require(IsOptimized());
	require(0 <= nTarget and nTarget < ivDataGridSetTargetFrequencies.GetSize());
	return ivDataGridSetTargetFrequencies.GetAt(nTarget);
}

int KWDRNBClassifier::GetDataGridSetTargetCellIndexAt(int nDataGrid, int nTarget) const
{
	require(IsOptimized());
	require(0 <= nDataGrid and nDataGrid < cvWeights.GetSize());
	require(0 <= nTarget and nTarget < ivDataGridTargetIndexes.GetSize());
	return ivDataGridTargetIndexes.GetAt(nDataGrid * ivDataGridSetTargetFrequencies.GetSize() + nTarget);
}

double KWDRNBClassifier::GetMissingLogProbaAt(int nDataGrid, int nTarget) const
{
	require(IsOptimized());
	require(0 <= nDataGrid and nDataGrid < cvWeights.GetSize());
	require(0 <= nTarget and nTarget < ivDataGridTargetIndexes.GetSize());
	return dvMissingLogProbas.GetAt(nDataGrid * ivDataGridSetTargetFrequencies.GetSize() + nTarget);
}

double KWDRNBClassifier::GetMissingScoreAt(int nTarget) const
{
	require(IsOptimized());
	require(0 <= nTarget and nTarget < dvMissingScores.GetSize());
	return dvMissingScores.GetAt(nTarget);
}

boolean KWDRNBClassifier::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

longint KWDRNBClassifier::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRClassifier::GetUsedMemory();
	lUsedMemory += sizeof(KWDRNBClassifier) - sizeof(KWDRClassifier);
	lUsedMemory += oaDataGridStatsAndBlockRules.GetUsedMemory();
	lUsedMemory += cvWeights.GetUsedMemory();
	lUsedMemory += ivDataGridSetTargetFrequencies.GetUsedMemory();
	lUsedMemory += ivDataGridTargetIndexes.GetUsedMemory();
	lUsedMemory += svTargetValues.GetUsedMemory();
	lUsedMemory += cvTargetProbs.GetUsedMemory();
	return lUsedMemory;
}

void KWDRNBClassifier::ComputeTargetProbs() const
{
	const KWDRDataGridStats* dataGridStatsRule;
	const KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	int nDataGridStatsOrBlock;
	int nSourceCellIndex;
	int nTargetCellIndex;
	int nTarget;
	int nTargetFrequency;
	int nTargetTotalFrequency;
	double dTargetLogProb;
	double dMaxTargetLogProb;
	double dProb;
	double dTotalProb;
	double dLaplaceEpsilon;
	double dLaplaceDenominator;
	int nValue;
	int nDataGrid;
	int nDataGridIndexWithinBlock;

	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Calcul de l'effectif global
	nTargetTotalFrequency = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
	{
		nTargetFrequency = GetDataGridSetTargetFrequencyAt(nTarget);
		assert(nTargetFrequency > 0);
		nTargetTotalFrequency += nTargetFrequency;
	}

	// Calcul des logarithmes de probabilites des valeurs cibles
	dMaxTargetLogProb = KWContinuous::GetMinValue();
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
	{
		// Initialisation avec le prior
		assert(GetDataGridSetTargetFrequencyAt(nTarget) > 0);
		dTargetLogProb = log(GetDataGridSetTargetFrequencyAt(nTarget) * 1.0 / nTargetTotalFrequency);
		dTargetLogProb += GetMissingScoreAt(nTarget);

		// Ajout des probabilites conditionnelles par grille
		nDataGrid = 0;
		for (nDataGridStatsOrBlock = 0; nDataGridStatsOrBlock < GetDataGridStatsOrBlockNumber();
		     nDataGridStatsOrBlock++)
		{
			if (IsDataGridStatsAt(nDataGridStatsOrBlock))
			{
				dataGridStatsRule = GetDataGridStatsAt(nDataGridStatsOrBlock);

				// Acces aux indexes de la source et la cible
				nSourceCellIndex = dataGridStatsRule->GetCellIndex();
				nTargetCellIndex = GetDataGridSetTargetCellIndexAt(nDataGrid, nTarget);

				// Si la valeur n'est pas manquante: Mise a jour du terme de proba pondere par son poids
				if (not dataGridStatsRule->IsMissingValue())
				{
					dTargetLogProb += GetDataGridWeightAt(nDataGrid) *
							  dataGridStatsRule->GetDataGridSourceConditionalLogProbAt(
							      nSourceCellIndex, nTargetCellIndex);
				}
				nDataGrid++;
			}
			else
			{
				dataGridStatsBlockRule = GetDataGridStatsBlockAt(nDataGridStatsOrBlock);
				for (nValue = 0; nValue < dataGridStatsBlockRule->GetValueNumber(); nValue++)
				{
					// Acces aux indexes de la source et la cible
					// La source doit etre ajuste a zero par des raisons techiques des DataGridBlocks
					nSourceCellIndex = dataGridStatsBlockRule->GetCellIndexAt(nValue) - 1;
					nDataGridIndexWithinBlock = dataGridStatsBlockRule->GetDataGridIndexAt(nValue);
					nTargetCellIndex = GetDataGridSetTargetCellIndexAt(
					    nDataGrid + nDataGridIndexWithinBlock, nTarget);

					// Mise a jour du terme de proba pondere par son poids
					dataGridStatsRule = dataGridStatsBlockRule->GetDataGridStatsAt(nValue);
					dTargetLogProb +=
					    GetDataGridWeightAt(nDataGrid + nDataGridIndexWithinBlock) *
					    (dataGridStatsRule->GetDataGridSourceConditionalLogProbAt(
						 nSourceCellIndex, nTargetCellIndex) -
					     GetMissingLogProbaAt(nDataGrid + nDataGridIndexWithinBlock, nTarget));
				}
				nDataGrid += dataGridStatsBlockRule->GetDataGridBlock()->GetDataGridNumber();
			}
		}

		// Memorisation du resultat
		cvTargetProbs.SetAt(nTarget, dTargetLogProb);

		// Memorisation du max
		if (dTargetLogProb > dMaxTargetLogProb)
			dMaxTargetLogProb = dTargetLogProb;
	}
	assert(dMaxTargetLogProb > KWContinuous::GetMinValue());

	// Calcul des probabilites des valeurs cibles, en normalisant par le dMaxTargetLogProb
	// pour eviter les valeurs extremes
	dTotalProb = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
	{
		dProb = exp(cvTargetProbs.GetAt(nTarget) - dMaxTargetLogProb);
		cvTargetProbs.SetAt(nTarget, (Continuous)dProb);
		dTotalProb += dProb;
	}
	assert(dTotalProb >= 1 - 1e-5);

	// Normalisation pour obtenir des probas
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
		cvTargetProbs.SetAt(nTarget, (Continuous)(cvTargetProbs.GetAt(nTarget) / dTotalProb));

	// Prise en compte d'un epsilon de Laplace (comme dans KWClassifierSelectionScore)
	// en considerant qu'on ne peut pas avoir de precision meilleure que 1/N
	//   p = p*N / N
	//   p_Laplace = (p*N + 0.5/J)/(N + 0.5)
	//   p_Laplace = (p + 0.5/JN)/(1 + 0.5/N)
	// (on se base sur N+1 pour eviter le cas N=0)
	dLaplaceEpsilon = 0.5 / (GetDataGridSetTargetPartNumber() * (nTargetTotalFrequency + 1));
	dLaplaceDenominator = (1.0 + 0.5 / (nTargetTotalFrequency + 1));
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
		cvTargetProbs.SetAt(nTarget, (cvTargetProbs.GetAt(nTarget) + dLaplaceEpsilon) / dLaplaceDenominator);

	// Calcul d'une probabilite par defaut pour les classes inconnues
	cUnknownTargetProb = dLaplaceEpsilon / dLaplaceDenominator;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRSNBClassifier

KWDRSNBClassifier::KWDRSNBClassifier()
{
	SetName("SNBClassifier");
	SetLabel("Selective Naive Bayes classifier");
	SetType(KWType::Structure);
	SetStructureName("Classifier");
	SetOperandNumber(2);
	SetVariableOperandNumber(true);

	// Le premier operande est un vecteurs de poids
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Vector");

	// Les operandes principaux contiennent des regles de type Structure
	nFirstDataGridOperand = 1;
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("DataGridStats");
}

KWDRSNBClassifier::~KWDRSNBClassifier() {}

KWDerivationRule* KWDRSNBClassifier::Create() const
{
	return new KWDRSNBClassifier;
}

boolean KWDRSNBClassifier::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	int nOperand;
	KWDerivationRule* dataGridStatsOrBlockRule;
	KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	KWDRDataGridBlock* dataGridBlockRule;
	int nTotalDataGridNumber;
	KWDRContinuousVector* weightsContinuousVectorRule;
	ALString sTmp;

	// Appel a la method ancetre
	bOk = KWDRNBClassifier::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification que le nombre des poids est egal au nombre total de data grid stats (DGS)
	// Plus precisement:
	//    #(total DGS) = #(total operandes du type DGS) + #(total DGS references dans les operandes du type bloc)
	if (bOk)
	{
		nTotalDataGridNumber = 0;
		for (nOperand = nFirstDataGridOperand; nOperand < GetOperandNumber() - 1; nOperand++)
		{
			dataGridStatsOrBlockRule = GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass);

			// Cas grille simple: On compte une grille
			if (naiveBayesPredictorRuleHelper.RuleHasDataGridStatsAtOperand(this, nOperand))
				nTotalDataGridNumber++;
			// Cas bloc des grilles: On compte toutes les grilles du block
			else
			{
				assert(
				    naiveBayesPredictorRuleHelper.RuleHasDataGridStatsBlockAtOperand(this, nOperand));
				dataGridStatsBlockRule = cast(KWDRDataGridStatsBlock*, dataGridStatsOrBlockRule);
				dataGridBlockRule =
				    cast(KWDRDataGridBlock*,
					 dataGridStatsBlockRule->GetFirstOperand()->GetReferencedDerivationRule(
					     kwcOwnerClass));
				nTotalDataGridNumber += dataGridBlockRule->GetUncheckedDataGridNumber();
			}
		}

		// Verfication que le nombre total de grilles est coherent avec le vecteur de poids
		weightsContinuousVectorRule =
		    cast(KWDRContinuousVector*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		if (nTotalDataGridNumber != weightsContinuousVectorRule->GetValueNumber())
		{
			AddError(sTmp + "Total number of data grids (" + IntToString(nTotalDataGridNumber) + ") " +
				 "incoherent with the number of weights (" +
				 IntToString(weightsContinuousVectorRule->GetValueNumber()) + ")");
			bOk = false;
		}
	}
	return bOk;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRNBRankRegressor

KWDRNBRankRegressor::KWDRNBRankRegressor()
{
	SetName("NBRankRegressor");
	SetLabel("Naive Bayes rank regressor");
	SetType(KWType::Structure);
	SetStructureName("RankRegressor");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);

	// Les operandes contiennent des regles de type Structure
	nFirstDataGridOperand = 0;
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("DataGridStats");

	// Gestion de l'optimisation
	nOptimizationFreshness = 0;
}

KWDRNBRankRegressor::~KWDRNBRankRegressor() {}

KWDerivationRule* KWDRNBRankRegressor::Create() const
{
	return new KWDRNBRankRegressor;
}

boolean KWDRNBRankRegressor::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	return naiveBayesPredictorRuleHelper.CheckDataGridStatsOperandsType(this, nFirstDataGridOperand,
									    GetOperandNumber() - 1);
}

boolean KWDRNBRankRegressor::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDRDataGridBlock* dataGridBlockRule;
	KWDRDataGrid* firstDataGridRule;
	ALString sTmp;

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de la compatibilite des arguments avec la regression
	if (bOk)
	{
		// Memorisation de l'effectif total du premier operand (DataGridStats ou DataGridStatsBlock)
		if (naiveBayesPredictorRuleHelper.RuleHasDataGridStatsBlockAtOperand(this, nFirstDataGridOperand))
		{
			dataGridBlockRule = cast(KWDRDataGridBlock*, GetOperandAt(nFirstDataGridOperand)
									 ->GetReferencedDerivationRule(kwcOwnerClass)
									 ->GetFirstOperand()
									 ->GetReferencedDerivationRule(kwcOwnerClass));
			firstDataGridRule =
			    cast(KWDRDataGrid*,
				 dataGridBlockRule->GetSecondOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		}
		else
		{
			assert(
			    naiveBayesPredictorRuleHelper.RuleHasDataGridStatsAtOperand(this, nFirstDataGridOperand));
			firstDataGridRule = cast(KWDRDataGrid*, GetOperandAt(nFirstDataGridOperand)
								    ->GetReferencedDerivationRule(kwcOwnerClass)
								    ->GetFirstOperand()
								    ->GetReferencedDerivationRule(kwcOwnerClass));
		}

		// Verification the la coherence des efectifs totaux et de la cible des operands data grid ou bloc de data gird
		bOk = naiveBayesPredictorRuleHelper.CheckDataGridStatsOperandsFrequencyAndTargetType(
		    kwcOwnerClass, this, nFirstDataGridOperand, GetOperandNumber() - 1, firstDataGridRule);
	}
	return bOk;
}

Object* KWDRNBRankRegressor::ComputeStructureResult(const KWObject* kwoObject) const
{
	int nOperand;

	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	// On evalue les operandes
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		GetOperandAt(nOperand)->GetStructureValue(kwoObject);

	// Calcul du vecteur de probabilites conditionnelles
	ComputeTargetProbs();

	return (Object*)this;
}

Continuous KWDRNBRankRegressor::ComputeTargetRankMean() const
{
	require(IsOptimized());

	return ComputeExpectation(&cvTargetCumulativeRanks);
}

Continuous KWDRNBRankRegressor::ComputeTargetRankStandardDeviation() const
{
	Continuous cMeanRank;
	Continuous cMeanSquareRank;
	Continuous cVarianceRanks;

	require(IsOptimized());

	// Calcul de l'esperance des rangs et de leur carre
	cMeanRank = ComputeExpectation(&cvTargetCumulativeRanks);
	cMeanSquareRank = ComputeExpectation(&cvTargetCumulativeSquareRanks);
	cVarianceRanks = cMeanSquareRank - cMeanRank * cMeanRank;
	assert(cVarianceRanks > -1e-5);
	if (cVarianceRanks < 0)
		cVarianceRanks = 0;

	return (Continuous)sqrt(cVarianceRanks);
}

Continuous KWDRNBRankRegressor::ComputeTargetRankDensityAt(Continuous cRank) const
{
	Continuous cNormalizedRank;
	Continuous cDensityRank;

	require(IsOptimized());

	// Recherche du rang normalise (projection sur [0,1] si necessaire)
	cNormalizedRank = cRank;
	if (cNormalizedRank < 0)
		cNormalizedRank = 0;
	else if (cNormalizedRank > 1)
		cNormalizedRank = 1;

	// Calcul de la densite
	cDensityRank = ComputeRankDensity(cNormalizedRank);
	return cDensityRank;
}

Continuous KWDRNBRankRegressor::ComputeTargetRankCumulativeProbAt(Continuous cRank) const
{
	Continuous cNormalizedRank;
	Continuous cCumulativeProbRank;

	require(IsOptimized());

	// Recherche du rang normalise (projection sur [0,1] si necessaire)
	cNormalizedRank = cRank;
	if (cNormalizedRank < 0)
		cNormalizedRank = 0;
	else if (cNormalizedRank > 1)
		cNormalizedRank = 1;

	// Calcul de la valeur de la fonction de repartition
	cCumulativeProbRank = ComputeRankCumulativeProb(cNormalizedRank);
	return cCumulativeProbRank;
}

void KWDRNBRankRegressor::Compile(KWClass* kwcOwnerClass)
{

	// Appel de la methode ancetre (compile tous les operands)
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Initialisation des data grid et poids
		naiveBayesPredictorRuleHelper.CompileInitializeDataGridAndWeightRules(
		    kwcOwnerClass, this, nFirstDataGridOperand, GetOperandNumber() - 1, &oaDataGridStatsAndBlockRules,
		    &cvWeights, &ivIsDataGridStatsRule);

		// Initialisation des indexation des frequences et de indexes de cellules des data grids
		CompileInitializeFrequencyAndCellIndexes();

		// Taillage du vecteur des probabilites cible
		assert(cvDataGridSetTargetCumulativeFrequencies.GetSize() >= 1);
		cvTargetProbs.SetSize(cvDataGridSetTargetCumulativeFrequencies.GetSize());

		// Calcul des rangs et de leur carre cumules par partie cible
		ComputeCumulativeRanks(&cvTargetCumulativeRanks);
		ComputeCumulativeSquareRanks(&cvTargetCumulativeSquareRanks);
	}
}

void KWDRNBRankRegressor::CompileInitializeFrequencyAndCellIndexes()
{
	boolean bLocalTrace = false;
	int nDataGridStatsOrBlock;
	const KWDRDataGridStats* constDataGridStatsRule;
	KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	int nTargetValue;
	ContinuousVector cvAllCumulativeFrequencies;
	int nBlockDataGridStats;
	int nFrequency;
	Continuous cCurrentFrequency;
	Continuous cLastFrequency;
	int nDataGridStatsNumber;
	int nDataGridStats;
	KWDRDataGridStats* dataGridStatsRule;

	require(cvWeights.GetSize() > 0);
	require(oaDataGridStatsAndBlockRules.GetSize() > 0);

	// Union et tri de toutes les effectifs cumules pour toutes les grilles
	for (nDataGridStatsOrBlock = 0; nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize();
	     nDataGridStatsOrBlock++)
	{
		if (ivIsDataGridStatsRule.GetAt(nDataGridStatsOrBlock))
		{
			constDataGridStatsRule =
			    cast(const KWDRDataGridStats*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));
			for (nTargetValue = 0; nTargetValue < constDataGridStatsRule->GetDataGridTargetCellNumber();
			     nTargetValue++)
				cvAllCumulativeFrequencies.Add(
				    constDataGridStatsRule->GetDataGridTargetCumulativeFrequencyAt(nTargetValue));
		}
		else
		{
			dataGridStatsBlockRule =
			    cast(KWDRDataGridStatsBlock*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));
			for (nBlockDataGridStats = 0;
			     nBlockDataGridStats < dataGridStatsBlockRule->GetDataGridStatsNumber();
			     nBlockDataGridStats++)
			{
				constDataGridStatsRule =
				    dataGridStatsBlockRule->GetDataGridStatsAtBlockIndex(nBlockDataGridStats);
				for (nTargetValue = 0;
				     nTargetValue < constDataGridStatsRule->GetDataGridTargetCellNumber();
				     nTargetValue++)
					cvAllCumulativeFrequencies.Add(
					    constDataGridStatsRule->GetDataGridTargetCumulativeFrequencyAt(
						nTargetValue));
			}
		}
	}
	cvAllCumulativeFrequencies.Sort();

	// Memorisation des frequences uniques a epsilon pres
	assert(KWContinuous::IsInt(cvAllCumulativeFrequencies.GetAt(0)));
	cvDataGridSetTargetCumulativeFrequencies.SetSize(0);
	cvDataGridSetTargetCumulativeFrequencies.Add(cvAllCumulativeFrequencies.GetAt(0));
	for (nFrequency = 1; nFrequency < cvAllCumulativeFrequencies.GetSize(); nFrequency++)
	{
		assert(KWContinuous::IsInt(cvAllCumulativeFrequencies.GetAt(nFrequency)));
		cCurrentFrequency = cvAllCumulativeFrequencies.GetAt(nFrequency);
		cLastFrequency = cvAllCumulativeFrequencies.GetAt(nFrequency - 1);
		if (cCurrentFrequency > cLastFrequency + 0.5)
			cvDataGridSetTargetCumulativeFrequencies.Add(cCurrentFrequency);
	}

	// Calcul de la correspondance entre les index cibles de l'ensemble des grilles et les index cibles de chaque grille
	nDataGridStatsNumber = cvWeights.GetSize();
	ivDataGridTargetIndexes.SetSize(cvDataGridSetTargetCumulativeFrequencies.GetSize() * nDataGridStatsNumber);
	nDataGridStats = 0;
	for (nDataGridStatsOrBlock = 0; nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize();
	     nDataGridStatsOrBlock++)
	{
		if (ivIsDataGridStatsRule.GetAt(nDataGridStatsOrBlock))
		{
			dataGridStatsRule =
			    cast(KWDRDataGridStats*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));

			// Parcours des index cibles de l'ensemble des grilles
			nTargetValue = 0;
			for (nFrequency = 0; nFrequency < cvDataGridSetTargetCumulativeFrequencies.GetSize();
			     nFrequency++)
			{
				// Comparaison de l'effectif cumule pour la partition sur l'ensemble des grille
				// a l'effectif cumule pour la grille en cours
				if (cvDataGridSetTargetCumulativeFrequencies.GetAt(nFrequency) >
				    dataGridStatsRule->GetDataGridTargetCumulativeFrequencyAt(nTargetValue) + 0.5)
					nTargetValue++;

				// Mise a jour de la correspondance
				ivDataGridTargetIndexes.SetAt(
				    nDataGridStats * cvDataGridSetTargetCumulativeFrequencies.GetSize() + nFrequency,
				    nTargetValue);
			}
			// Mise a jour du compte de DGS
			nDataGridStats++;
		}
		else
		{
			dataGridStatsBlockRule =
			    cast(KWDRDataGridStatsBlock*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));
			for (nBlockDataGridStats = 0;
			     nBlockDataGridStats < dataGridStatsBlockRule->GetDataGridStatsNumber();
			     nBlockDataGridStats++)
			{
				constDataGridStatsRule =
				    dataGridStatsBlockRule->GetDataGridStatsAtBlockIndex(nBlockDataGridStats);
				nTargetValue = 0;
				for (nFrequency = 0; nFrequency < cvDataGridSetTargetCumulativeFrequencies.GetSize();
				     nFrequency++)
				{
					// Comparaison de l'effectif cumule pour la partition sur l'ensemble des grille
					// a l'effectif cumule pour la grille en cours
					if (cvDataGridSetTargetCumulativeFrequencies.GetAt(nFrequency) >
					    constDataGridStatsRule->GetDataGridTargetCumulativeFrequencyAt(
						nTargetValue) +
						0.5)
						nTargetValue++;

					// Mise a jour de la correspondance
					ivDataGridTargetIndexes.SetAt(
					    nDataGridStats * cvDataGridSetTargetCumulativeFrequencies.GetSize() +
						nFrequency,
					    nTargetValue);
				}
				// Mise a jour du compte de DGS
				nDataGridStats++;
			}
		}
	}
	assert(nDataGridStatsNumber == nDataGridStats);

	// Affichage des resultats de compilation
	if (bLocalTrace)
	{
		cout << "DataGridSet\n";
		cout << "DataGrid and DataGridBlock rule number\t" << oaDataGridStatsAndBlockRules.GetSize() << endl;
		cout << "DataGrid total number\t" << nDataGridStatsNumber << endl;

		// Affichage des effectifs  de la cible
		cout << "Target frequencies\n";
		cout << "DataGridSet";
		for (nDataGridStats = 0; nDataGridStats < nDataGridStatsNumber; nDataGridStats++)
			cout << "\tDataGrid " << nDataGridStats;
		cout << endl;
		for (nFrequency = 0; nFrequency < cvDataGridSetTargetCumulativeFrequencies.GetSize(); nFrequency++)
		{
			cout << cvDataGridSetTargetCumulativeFrequencies.GetAt(nFrequency);
			for (nDataGridStats = 0; nDataGridStats < nDataGridStatsNumber; nDataGridStats++)
				cout << "\t"
				     << ivDataGridTargetIndexes.GetAt(
					    nDataGridStats * cvDataGridSetTargetCumulativeFrequencies.GetSize() +
					    nFrequency);
			cout << endl;
		}
	}
}

longint KWDRNBRankRegressor::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRRankRegressor::GetUsedMemory();
	lUsedMemory += sizeof(KWDRNBRankRegressor) - sizeof(KWDRRankRegressor);
	lUsedMemory += oaDataGridStatsAndBlockRules.GetUsedMemory();
	lUsedMemory += cvWeights.GetUsedMemory();
	lUsedMemory += cvDataGridSetTargetCumulativeFrequencies.GetUsedMemory();
	lUsedMemory += ivDataGridTargetIndexes.GetUsedMemory();
	lUsedMemory += cvTargetCumulativeSquareRanks.GetUsedMemory();
	lUsedMemory += cvTargetProbs.GetUsedMemory();
	return lUsedMemory;
}

void KWDRNBRankRegressor::ComputeTargetProbs() const
{
	Continuous dMaxTargetLogProb;
	int nTargetValue;
	int nDataGrid;
	double dTargetLogProb;
	int nDataGridStatsOrBlock;
	const KWDRDataGridStats* dataGridStatsRule;
	int nSourceCellIndex;
	int nTargetCellIndex;
	const KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	int nBlockValue;
	int nDataGridIndexWithinBlock;
	Continuous dTargetFrequency;
	Continuous cTargetTotalFrequency;
	Continuous dProb;
	double dTotalProb;
	Continuous dLaplaceEpsilon;
	Continuous dLaplaceDenominator;
	Continuous dRankNumber;
	Continuous dTargetCumulativeFrequency;

	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetValueNumber());

	// Calcul des logarithmes des probabilites des valeurs cibles
	dMaxTargetLogProb = KWContinuous::GetMinValue();
	for (nTargetValue = 0; nTargetValue < GetDataGridSetTargetValueNumber(); nTargetValue++)
	{
		nDataGrid = 0;
		dTargetLogProb = 0;
		for (nDataGridStatsOrBlock = 0; nDataGridStatsOrBlock < GetDataGridStatsOrBlockNumber();
		     nDataGridStatsOrBlock++)
		{
			// Cas d'une grille simple
			if (IsDataGridStatsAt(nDataGridStatsOrBlock))
			{
				dataGridStatsRule = GetDataGridStatsAt(nDataGridStatsOrBlock);

				// Acces aux indexes de la source et la cible
				nSourceCellIndex = dataGridStatsRule->GetCellIndex();
				nTargetCellIndex = GetDataGridSetTargetIndexAt(nDataGrid, nTargetValue);

				// Mise a jour du terme de proba, en prenant en compte le poids de la grille
				dTargetLogProb += GetDataGridWeightAt(nDataGrid) *
						  dataGridStatsRule->GetDataGridSourceConditionalLogProbAt(
						      nSourceCellIndex, nTargetCellIndex);

				// Mise-a-jour du compteur de grilles
				nDataGrid++;
			}
			// Cas d'un bloc de grilles
			else
			{
				dataGridStatsBlockRule = GetDataGridStatsBlockAt(nDataGridStatsOrBlock);

				for (nBlockValue = 0; nBlockValue < dataGridStatsBlockRule->GetValueNumber();
				     nBlockValue++)
				{
					// Acces aux indexes de la source et la cible
					// La source doit etre ajuste a zero par des raisons techiques de l'implementation de la regle DataGridBlock
					nSourceCellIndex = dataGridStatsBlockRule->GetCellIndexAt(nBlockValue) - 1;
					nDataGridIndexWithinBlock =
					    dataGridStatsBlockRule->GetDataGridIndexAt(nBlockValue);
					nTargetCellIndex = GetDataGridSetTargetCellIndexAt(
					    nDataGrid + nDataGridIndexWithinBlock, nTargetValue);

					// Mise a jour du terme de proba, en prenant en compte le poids de la grille
					dataGridStatsRule = dataGridStatsBlockRule->GetDataGridStatsAt(nBlockValue);
					dTargetLogProb += GetDataGridWeightAt(nDataGrid + nDataGridIndexWithinBlock) *
							  dataGridStatsRule->GetDataGridSourceConditionalLogProbAt(
							      nSourceCellIndex, nTargetCellIndex);
				}

				// Mise-a-jour du compteur de grilles
				nDataGrid += dataGridStatsBlockRule->GetDataGridBlock()->GetDataGridNumber();
			}
		}
		// Memorisation de la probabilite pour la valeur cible courante
		cvTargetProbs.SetAt(nTargetValue, (Continuous)dTargetLogProb);

		// Memorisation de la probabilite maximale
		if (dTargetLogProb > dMaxTargetLogProb)
			dMaxTargetLogProb = dTargetLogProb;
	}
	assert(dMaxTargetLogProb > KWContinuous::GetMinValue());

	// Calcul des probabilites des valeurs cibles
	// On normalise par dMaxTargetLogProb pour eviter les valeurs extremes
	dTotalProb = 0;
	dTargetCumulativeFrequency = 0;
	for (nTargetValue = 0; nTargetValue < GetDataGridSetTargetValueNumber(); nTargetValue++)
	{
		// Recherche de l'effectif de la partie cible courante
		dTargetFrequency = GetDataGridSetTargetCumulativeFrequencyAt(nTargetValue) - dTargetCumulativeFrequency;
		dTargetCumulativeFrequency = GetDataGridSetTargetCumulativeFrequencyAt(nTargetValue);

		// Normalisation de la probabilite
		dProb = exp(cvTargetProbs.GetAt(nTargetValue) - dMaxTargetLogProb);
		cvTargetProbs.SetAt(nTargetValue, dProb);

		// Calcul de la probabilite total au denominateur, en multipliant la probabilite (uniforme par rang)
		// par le nombre de rangs contenus dans la partie
		dTotalProb += dProb * dTargetFrequency;
	}
	assert(dTotalProb >= 1);

	// Normalisation pour obtenir des probas
	for (nTargetValue = 0; nTargetValue < GetDataGridSetTargetValueNumber(); nTargetValue++)
		cvTargetProbs.SetAt(nTargetValue, cvTargetProbs.GetAt(nTargetValue) / dTotalProb);

	// Prise en compte d'un epsilon de Laplace (comme dans KWClassifierSelectionScore)
	// en considerant qu'on ne peut pas avoir de precision meilleure que 1/N
	//   p = p*N / N
	//   p_Laplace = (p*N + 0.5/J)/(N + 0.5)
	//   p_Laplace = (p + 0.5/JN)/(1 + 0.5/N)
	// (on se base sur N+1 pour eviter le cas N=0, et J=nRankNumber)
	cTargetTotalFrequency = GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetValueNumber() - 1);
	dRankNumber = cTargetTotalFrequency;
	if (dRankNumber == 0)
		dRankNumber = 1;
	dLaplaceEpsilon = 0.5 / (dRankNumber * (cTargetTotalFrequency + 1));
	dLaplaceDenominator = (1.0 + 0.5 / (cTargetTotalFrequency + 1));
	for (nTargetValue = 0; nTargetValue < GetDataGridSetTargetValueNumber(); nTargetValue++)
		cvTargetProbs.SetAt(nTargetValue,
				    (cvTargetProbs.GetAt(nTargetValue) + dLaplaceEpsilon) / dLaplaceDenominator);
}

Continuous KWDRNBRankRegressor::ComputeExpectation(const ContinuousVector* cvTargetCumulativeValues) const
{
	double dMeanValue;
	Continuous cMeanValue;
	int nTarget;
	Continuous cTargetCumulativeValue;
	Continuous cPreviousTargetCumulativeValue;

	require(IsOptimized());
	require(cvTargetCumulativeValues != NULL);
	require(cvTargetCumulativeValues->GetSize() == GetDataGridSetTargetValueNumber());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetValueNumber());

	// Calcul de l'esperance des valeurs
	cPreviousTargetCumulativeValue = 0;
	dMeanValue = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetValueNumber(); nTarget++)
	{
		// Mise a jour de l'esperance de la valeur
		cTargetCumulativeValue = cvTargetCumulativeValues->GetAt(nTarget);
		dMeanValue += cvTargetProbs.GetAt(nTarget) * (cTargetCumulativeValue - cPreviousTargetCumulativeValue);
		cPreviousTargetCumulativeValue = cTargetCumulativeValue;
	}
	cMeanValue = (Continuous)dMeanValue;

	return cMeanValue;
}

Continuous KWDRNBRankRegressor::ComputeRankCumulativeProb(Continuous cNormalizedRank) const
{
	ContinuousVector cvCumulativeCrenel;
	Continuous cRankCumulativeProb;

	require(IsOptimized());
	require(0 <= cNormalizedRank and cNormalizedRank <= 1);

	// Calcul de la valeur d'un creneau cumule par partie cible
	ComputeCumulativeCrenel(&cvCumulativeCrenel, cNormalizedRank);
	assert(cvCumulativeCrenel.GetSize() == GetDataGridSetTargetValueNumber());

	// Calcul de la valeur de la fonction de repartition par integration de la fonction creneau
	cRankCumulativeProb = ComputeExpectation(&cvCumulativeCrenel);
	ensure(-1e-5 <= cRankCumulativeProb and cRankCumulativeProb <= 1 + 1e-5);
	if (cRankCumulativeProb < 0)
		cRankCumulativeProb = 0;
	if (cRankCumulativeProb > 1)
		cRankCumulativeProb = 1;
	return cRankCumulativeProb;
}

Continuous KWDRNBRankRegressor::ComputeRankDensity(Continuous cNormalizedRank) const
{
	Continuous cDensityRank;
	Continuous cRank;
	Continuous cTargetTotalFrequency;
	int nTarget;

	require(IsOptimized());
	require(0 <= cNormalizedRank and cNormalizedRank <= 1);
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetValueNumber());

	// Recherche de l'effectif total
	cTargetTotalFrequency = 0;
	if (GetDataGridSetTargetValueNumber() > 0)
		cTargetTotalFrequency =
		    GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetValueNumber() - 1);

	// Calcul du rang parmi l'effectif total en apprentissage
	cRank = cNormalizedRank * cTargetTotalFrequency;

	// Recherche du partile correspondant au rang
	cDensityRank = 1;
	for (nTarget = 0; nTarget < GetDataGridSetTargetValueNumber(); nTarget++)
	{
		// Mise a jour de l'esperance de la valeur
		if (cRank <= GetDataGridSetTargetCumulativeFrequencyAt(nTarget))
		{
			cDensityRank = cvTargetProbs.GetAt(nTarget) * cTargetTotalFrequency;
			break;
		}
	}

	return cDensityRank;
}

void KWDRNBRankRegressor::ComputeCumulativeCrenel(ContinuousVector* cvTargetCumulativeValues,
						  Continuous cNormalizedRank) const
{
	int nTarget;
	Continuous cTargetCumulativeFrequency;
	Continuous cCumulativeValue;
	Continuous cTotalFrequency;
	Continuous cInputFrequency;

	require(IsOptimized());
	require(cvTargetCumulativeValues != NULL);
	require(0 <= cNormalizedRank and cNormalizedRank <= 1);

	// Calcul de l'effectif total
	cTotalFrequency = 0;
	if (GetDataGridSetTargetValueNumber() > 0)
		cTotalFrequency = GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetValueNumber() - 1);

	// Calcul de l'effectif equivalent au rang normalise donne en parametre
	cInputFrequency = cNormalizedRank * cTotalFrequency;

	// Calcul du vecteur des valeurs cumulees
	cvTargetCumulativeValues->SetSize(GetDataGridSetTargetValueNumber());
	cCumulativeValue = 0;
	cTargetCumulativeFrequency = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetValueNumber(); nTarget++)
	{
		// Recherche de l'effectif de la partie cible courante
		cTargetCumulativeFrequency = GetDataGridSetTargetCumulativeFrequencyAt(nTarget);

		// Mise a jour de la valeur cumulee
		if (cTargetCumulativeFrequency <= cInputFrequency)
			cCumulativeValue = cTargetCumulativeFrequency;
		else
			cCumulativeValue = cInputFrequency;

		// Memorisation de la valeur cumulee
		cvTargetCumulativeValues->SetAt(nTarget, cCumulativeValue);
	}
}

void KWDRNBRankRegressor::ComputeCumulativeRanks(ContinuousVector* cvTargetCumulativeValues) const
{
	int nTarget;
	Continuous cTargetCumulativeFrequency;
	Continuous cTargetFrequency;
	Continuous cCumulativeValue;
	Continuous cTotalFrequency;
	Continuous cMeanRank;

	require(IsOptimized());
	require(cvTargetCumulativeValues != NULL);

	// Calcul de l'effectif total
	cTotalFrequency = 0;
	if (GetDataGridSetTargetValueNumber() > 0)
		cTotalFrequency = GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetValueNumber() - 1);

	// Calcul du vecteur des valeurs cumulees
	cvTargetCumulativeValues->SetSize(GetDataGridSetTargetValueNumber());
	cCumulativeValue = 0;
	cTargetCumulativeFrequency = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetValueNumber(); nTarget++)
	{
		// Recherche de l'effectif de la partie cible courante
		cTargetFrequency = GetDataGridSetTargetCumulativeFrequencyAt(nTarget) - cTargetCumulativeFrequency;
		cTargetCumulativeFrequency = GetDataGridSetTargetCumulativeFrequencyAt(nTarget);

		// Mise a jour de la valeur cumulee
		cMeanRank = (cTargetCumulativeFrequency - cTargetFrequency / 2) / cTotalFrequency;
		cCumulativeValue += cTargetFrequency * cMeanRank;

		// Memorisation de la valeur cumulee
		cvTargetCumulativeValues->SetAt(nTarget, cCumulativeValue);
	}
}

void KWDRNBRankRegressor::ComputeCumulativeSquareRanks(ContinuousVector* cvTargetCumulativeValues) const
{
	int nTarget;
	Continuous cTargetCumulativeFrequency;
	Continuous cTargetFrequency;

	Continuous cCumulativeValue;
	Continuous cTotalFrequency;
	Continuous cMeanRank;

	require(IsOptimized());
	require(cvTargetCumulativeValues != NULL);

	// Calcul de l'effectif total
	cTotalFrequency = 0;
	if (GetDataGridSetTargetValueNumber() > 0)
		cTotalFrequency = GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetValueNumber() - 1);

	// Calcul du vecteur des valeurs cumulees
	cvTargetCumulativeValues->SetSize(GetDataGridSetTargetValueNumber());
	cCumulativeValue = 0;
	cTargetCumulativeFrequency = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetValueNumber(); nTarget++)
	{
		// Recherche de l'effectif de la partie cible courante
		cTargetFrequency = GetDataGridSetTargetCumulativeFrequencyAt(nTarget) - cTargetCumulativeFrequency;
		cTargetCumulativeFrequency = GetDataGridSetTargetCumulativeFrequencyAt(nTarget);

		// Mise a jour de la valeur cumulee
		cMeanRank = (cTargetCumulativeFrequency - cTargetFrequency / 2) / cTotalFrequency;
		cCumulativeValue +=
		    cTargetFrequency * (cMeanRank * cMeanRank + 1 / (12 * cTotalFrequency * cTotalFrequency));

		// Memorisation de la valeur cumulee
		cvTargetCumulativeValues->SetAt(nTarget, cCumulativeValue);
	}
}

boolean KWDRNBRankRegressor::IsDataGridStatsAt(int nDataGridStatsOrBlock) const
{
	require(IsOptimized());
	require(0 <= nDataGridStatsOrBlock and nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize());

	return ivIsDataGridStatsRule.GetAt(nDataGridStatsOrBlock);
}

const KWDRDataGridStatsBlock* KWDRNBRankRegressor::GetDataGridStatsBlockAt(int nDataGridStatsOrBlock) const
{
	require(IsOptimized());
	require(0 <= nDataGridStatsOrBlock and nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize());
	require(not IsDataGridStatsAt(nDataGridStatsOrBlock));
	return cast(KWDRDataGridStatsBlock*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));
}

int KWDRNBRankRegressor::GetDataGridStatsOrBlockNumber() const
{
	require(IsOptimized());
	return oaDataGridStatsAndBlockRules.GetSize();
}

int KWDRNBRankRegressor::GetDataGridSetTargetCellIndexAt(int nDataGrid, int nTargetValue) const
{
	require(IsOptimized());
	require(0 <= nDataGrid and nDataGrid < cvWeights.GetSize());
	require(0 <= nTargetValue and nTargetValue < ivDataGridTargetIndexes.GetSize());
	return ivDataGridTargetIndexes.GetAt(nDataGrid * cvDataGridSetTargetCumulativeFrequencies.GetSize() +
					     nTargetValue);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRSNBRankRegressor

KWDRSNBRankRegressor::KWDRSNBRankRegressor()
{
	SetName("SNBRankRegressor");
	SetLabel("Selective Naive Bayes rank regressor");
	SetType(KWType::Structure);
	SetStructureName("RankRegressor");
	SetOperandNumber(2);
	SetVariableOperandNumber(true);

	// Le premier operande est un vecteurs de poids
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Vector");

	// Les operandes principaux contiennent des regles de type Structure
	nFirstDataGridOperand = 1;
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("DataGridStats");
}

KWDRSNBRankRegressor::~KWDRSNBRankRegressor() {}

KWDerivationRule* KWDRSNBRankRegressor::Create() const
{
	return new KWDRSNBRankRegressor;
}
boolean KWDRSNBRankRegressor::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	int nTotalDataGridNumber;
	int nOperand;
	KWDerivationRule* dataGridStatsOrBlockRule;
	KWDRDataGridStats refDataGridStatsRule;
	KWDRDataGridStatsBlock refDataGridStatsBlockRule;
	KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	KWDRDataGridBlock* dataGridBlockRule;
	KWDRContinuousVector* weightsContinuousVectorRule;
	ALString sTmp;

	// Appel a la method ancetre
	bOk = KWDRNBRankRegressor::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification que le nombre des poids est egal au nombre total de data grid stats (DGS)
	// Plus precisement:
	//    #(total DGS) = #(total operandes du type DGS) + #(total DGS references dans les operandes du type bloc)
	if (bOk)
	{
		nTotalDataGridNumber = 0;
		for (nOperand = nFirstDataGridOperand; nOperand < GetOperandNumber(); nOperand++)
		{
			dataGridStatsOrBlockRule = GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass);

			// Cas grille simple: On compte une grille
			if (dataGridStatsOrBlockRule->GetName() == refDataGridStatsRule.GetName())
				nTotalDataGridNumber++;
			// Cas bloc des grilles: On compte toutes les grilles du block
			else
			{
				assert(dataGridStatsOrBlockRule->GetName() == refDataGridStatsBlockRule.GetName());
				dataGridStatsBlockRule = cast(KWDRDataGridStatsBlock*, dataGridStatsOrBlockRule);
				dataGridBlockRule =
				    cast(KWDRDataGridBlock*,
					 dataGridStatsBlockRule->GetFirstOperand()->GetReferencedDerivationRule(
					     kwcOwnerClass));
				nTotalDataGridNumber += dataGridBlockRule->GetUncheckedDataGridNumber();
			}
		}

		// Verfication que le nombre total de grilles est coherent avec le vecteur de poids
		weightsContinuousVectorRule =
		    cast(KWDRContinuousVector*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		if (nTotalDataGridNumber != weightsContinuousVectorRule->GetValueNumber())
		{
			AddError(sTmp + "Total number of data grids (" + IntToString(nTotalDataGridNumber) + ") " +
				 "incoherent with the number of weights (" +
				 IntToString(weightsContinuousVectorRule->GetValueNumber()) + ")");
			bOk = false;
		}
	}
	return bOk;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRNBRegressor

KWDRNBRegressor::KWDRNBRegressor()
{
	SetName("NBRegressor");
	SetLabel("Naive Bayes regressor");
	SetType(KWType::Structure);
	SetStructureName("Regressor");
	SetOperandNumber(2);

	// Le premier operande est un regresseur de rangs
	// (c'est egalement un Naive Bayes, a verifier dans les methodes Check...)
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("RankRegressor");

	// Le second operande comprend les valeurs
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("DataGrid");

	// Donnees d'optimisation
	cTargetValueRange = 0;
	cMeanTargetValueRange = 0;
	nMissingValueNumber = 0;
	rankRegressorRule = NULL;
	targetDataGridRule = NULL;
	targetValuesRules = NULL;
	targetFrequenciesRule = NULL;
	nOptimizationFreshness = 0;
}

KWDRNBRegressor::~KWDRNBRegressor() {}

KWDerivationRule* KWDRNBRegressor::Create() const
{
	return new KWDRNBRegressor;
}

boolean KWDRNBRegressor::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDerivationRule* firstRule;
	ALString sRegressor = "Regressor";
	ALString sRankRegressor = "RankRegressor";
	ALString sExpectedRuleName;

	// Methode ancetre
	bOk = KWDRRegressor::CheckOperandsCompleteness(kwcOwnerClass);

	// Verifications supplementaires
	if (bOk)
	{
		// Calcul du nom attendu pour la regle du premiere operande
		// en remplacant le nom courant, base sur un regresseur, par le nom base sur regresseur de rang
		assert(GetName().Right(sRegressor.GetLength()) == sRegressor);
		sExpectedRuleName = GetName().Left(GetName().GetLength() - sRegressor.GetLength()) + sRankRegressor;

		// Verification du type de la regle du premier operande
		firstRule = GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass);
		if (firstRule->GetName() != sExpectedRuleName)
		{
			bOk = false;
			AddError("The source rule of the first operand (" + firstRule->GetName() + ") should be " +
				 sExpectedRuleName);
		}

		// Verification de l'ordre des valeurs pour la regle
		// TODO MB
	}
	return bOk;
}

void KWDRNBRegressor::Compile(KWClass* ownerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(ownerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < ownerClass->GetCompileFreshness())
	{
		// Mise-a-jour de la fraicheur
		nOptimizationFreshness = ownerClass->GetCompileFreshness();

		// Memorisation des operandes
		rankRegressorRule =
		    cast(KWDRNBRankRegressor*, GetFirstOperand()->GetReferencedDerivationRule(ownerClass));
		targetDataGridRule = cast(KWDRDataGrid*, GetSecondOperand()->GetReferencedDerivationRule(ownerClass));
		targetValuesRules =
		    cast(KWDRContinuousValueSet*,
			 targetDataGridRule->GetFirstOperand()->GetReferencedDerivationRule(ownerClass));
		targetFrequenciesRule = cast(
		    KWDRFrequencies*, targetDataGridRule->GetSecondOperand()->GetReferencedDerivationRule(ownerClass));

		// Calcul des effectifs cumules des valeurs par index de valeur
		ComputeSingleTargetValueCumulativeInstanceNumbers(&ivSingleTargetValueCumulativeInstanceNumbers);

		// Calcul de l'ecart moyen inter-valeurs (Max-Min)/N, ou 1/N si Min=Max
		cTargetValueRange = ComputeTargetValueRange();
		cMeanTargetValueRange = ComputeMeanTargetValueRange();

		// Calcul du nombre de valeurs manquantes
		nMissingValueNumber = ComputeMissingValueNumber();

		// Calcul des valeurs et de leur carre cumules par partie cible
		ComputeCumulativeTargetValues(&cvCumulativeTargetValues);
		ComputeCumulativeSquareTargetValues(&cvCumulativeSquareTargetValues);
	}
}

Object* KWDRNBRegressor::ComputeStructureResult(const KWObject* kwoObject) const
{
	int nOperand;

	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	// On evalue les operandes
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		GetOperandAt(nOperand)->GetStructureValue(kwoObject);

	return (Object*)this;
}

Continuous KWDRNBRegressor::ComputeTargetMean() const
{
	require(IsOptimized());

	// En cas de valeurs manquantes, on renvoie valeur manquante
	if (nMissingValueNumber > 0)
		return KWContinuous::GetMissingValue();

	// Calcul de l'esperance des valeurs
	return rankRegressorRule->ComputeExpectation(&cvCumulativeTargetValues);
}

Continuous KWDRNBRegressor::ComputeTargetStandardDeviation() const
{
	Continuous cMeanValue;
	Continuous cMeanSquareValue;
	Continuous cVarianceValues;

	require(IsOptimized());

	// En cas de valeurs manquantes, on renvoie valeur manquante
	if (nMissingValueNumber > 0)
		return KWContinuous::GetMissingValue();

	// Calcul de l'esperance des valeurs et de leur carre
	cMeanValue = rankRegressorRule->ComputeExpectation(&cvCumulativeTargetValues);
	cMeanSquareValue = rankRegressorRule->ComputeExpectation(&cvCumulativeSquareTargetValues);
	cVarianceValues = cMeanSquareValue - cMeanValue * cMeanValue;
	assert(cVarianceValues > 1e-5);
	if (cVarianceValues < 0)
		cVarianceValues = 0;

	return (Continuous)sqrt(cVarianceValues);
}

Continuous KWDRNBRegressor::ComputeTargetDensityAt(Continuous cValue) const
{
	int nValueIndex;
	Continuous cNormalizedRank;
	Continuous cRankDensity;
	Continuous cIntervalProb;
	Continuous cIntervalLowerBound;
	Continuous cIntervalUpperBound;
	Continuous cTotalFrequency;
	Continuous cClosestValue;
	Continuous cValueFrequency;
	Continuous cDensity;

	require(IsOptimized());

	// En cas de valeurs manquantes en apprentissage, on renvoie valeur manquante
	if (nMissingValueNumber > 0)
		return KWContinuous::GetMissingValue();

	// Si la valeur est manquante, on renvoie valeur manquante
	if (cValue == KWContinuous::GetMissingValue())
		return KWContinuous::GetMissingValue();

	// Calcul de l'effectif total
	cTotalFrequency = 0;
	if (GetSingleTargetValueNumber() > 0)
		cTotalFrequency =
		    (Continuous)GetSingleTargetValueCumulativeFrequencyAt(GetSingleTargetValueNumber() - 1);

	// Calcul de l'index de la valeur la plus proche
	nValueIndex = GetTargetValueIndex(cValue);

	// Calcul du rang normalise
	cNormalizedRank = 0.5;
	if (cTotalFrequency > 0)
	{
		cNormalizedRank = (Continuous)(GetSingleTargetValueCumulativeFrequencyAt(nValueIndex) -
					       GetSingleTargetValueFrequencyAt(nValueIndex) / 2.0);
		cNormalizedRank /= cTotalFrequency;
	}

	// Calcul de la densite sur les rangs
	cRankDensity = rankRegressorRule->ComputeRankDensity(cNormalizedRank);

	// On passe de la densite sur les rangs a celle sur les valeurs en recherchant
	// l'amplitude de l'intervalle de valeurs correspondant au rang
	// Cas ou il n'y a pas de valeur: on prend l'exponentielle decroissant comme densite
	cDensity = 0;
	if (GetSingleTargetValueNumber() == 0)
	{
		if (cValue <= -cMeanTargetValueRange / 2)
			cDensity = exp(-fabs(cValue + cMeanTargetValueRange / 2) / cTargetValueRange) /
				   (4 * cTargetValueRange);
		else if (cValue <= cMeanTargetValueRange / 2)
			cDensity = 1 / (2 * cMeanTargetValueRange);
		else
			cDensity = exp(-fabs(cValue - cMeanTargetValueRange / 2) / cTargetValueRange) /
				   (4 * cTargetValueRange);
	}
	// Cas ou la valeur est inferieure a la valeur min
	else if (cValue <= GetSingleTargetValueAt(0))
	{
		assert(nValueIndex == 0);
		assert(cTotalFrequency > 0);

		// Calcul de la probabilite d'etre dans le demi-intervalle contenant la valeur
		cValueFrequency = (Continuous)GetSingleTargetValueFrequencyAt(0);
		cIntervalProb = cRankDensity * cValueFrequency / (2 * cTotalFrequency);

		// Le premier demi intervalle est decoupe pour moitie en exponentielle decroissante,
		// pour moitie en densite constante
		cClosestValue = GetSingleTargetValueAt(0) - cMeanTargetValueRange / 2;
		if (cValue <= cClosestValue)
			cDensity = cIntervalProb * exp(-fabs(cValue - cClosestValue) / cTargetValueRange) /
				   (2 * cTargetValueRange);
		else
			cDensity = cIntervalProb / cMeanTargetValueRange;
	}
	// Cas ou la valeur est superieure a la valeur max
	else if (cValue > GetSingleTargetValueAt(GetSingleTargetValueNumber() - 1))
	{
		assert(nValueIndex == GetSingleTargetValueNumber() - 1);
		assert(cTotalFrequency > 0);

		// Calcul de la probabilite d'etre dans le demi-intervalle contenant la valeur
		cValueFrequency = (Continuous)GetSingleTargetValueFrequencyAt(GetSingleTargetValueNumber() - 1);
		cIntervalProb = cRankDensity * cValueFrequency / (2 * cTotalFrequency);

		// Le dernier demi intervalle est decoupe pour moitie en en densite constante
		// pour moitie en exponentielle decroissante
		cClosestValue = GetSingleTargetValueAt(0) + cMeanTargetValueRange / 2;
		if (cValue <= cClosestValue)
			cDensity = cIntervalProb / cMeanTargetValueRange;
		else
			cDensity = cIntervalProb * exp(-fabs(cValue - cClosestValue) / cTargetValueRange) /
				   (2 * cTargetValueRange);
	}
	// Cas general :
	// On utilise la methode GetLowerMeanValue (plutot que GetHumanReadableLowerMeanValue), car les
	// valeurs des bornes ne sont pas montrees aux utilisateurs, et que l'on ne s'en sert que pour les
	// calculs dans une regle de derivation (exigence de rapidite).
	else
	{
		assert(0 <= nValueIndex and nValueIndex <= GetSingleTargetValueNumber());
		assert(nValueIndex == 0 or
		       (GetSingleTargetValueAt(nValueIndex - 1) + GetSingleTargetValueAt(nValueIndex)) / 2 <= cValue);
		assert(nValueIndex == GetSingleTargetValueNumber() - 1 or
		       cValue <= KWContinuous::GetLowerMeanValue(GetSingleTargetValueAt(nValueIndex),
								 GetSingleTargetValueAt(nValueIndex + 1)));
		assert(cTotalFrequency > 0);

		// Calcul de la probabilite d'etre dans le demi-intervalle contenant la valeur
		cValueFrequency = (Continuous)GetSingleTargetValueFrequencyAt(nValueIndex);
		cIntervalProb = cRankDensity * cValueFrequency / (2 * cTotalFrequency);

		// On determine de quelle valeur on est le plus proche
		if (cValue <= GetSingleTargetValueAt(nValueIndex))
		{
			// On calcul la densite moyenne entre la borne inf et sup de l'intervalle
			cIntervalLowerBound = KWContinuous::GetLowerMeanValue(GetSingleTargetValueAt(nValueIndex - 1),
									      GetSingleTargetValueAt(nValueIndex));
			cIntervalUpperBound = GetSingleTargetValueAt(nValueIndex);
			assert(cIntervalUpperBound >= cIntervalLowerBound);

			// Tolerance si egalite des bornes, possible dans les limites de precision numerique
			if (cIntervalUpperBound > cIntervalLowerBound)
				cDensity = cIntervalProb / (cIntervalUpperBound - cIntervalLowerBound);
			else
				cDensity = KWContinuous::GetMissingValue();
		}
		else
		{
			// On calcul la densite moyenne entre la borne inf et sup de l'intervalle
			cIntervalLowerBound = GetSingleTargetValueAt(nValueIndex);
			cIntervalUpperBound = KWContinuous::GetLowerMeanValue(GetSingleTargetValueAt(nValueIndex),
									      GetSingleTargetValueAt(nValueIndex + 1));
			assert(cIntervalUpperBound >= cIntervalLowerBound);

			// Tolerance si egalite des bornes, possible dans les limites de precision numerique
			if (cIntervalUpperBound > cIntervalLowerBound)
				cDensity = cIntervalProb / (cIntervalUpperBound - cIntervalLowerBound);
			else
				cDensity = KWContinuous::GetMissingValue();
		}
	}
	ensure(cDensity > 0);
	return cDensity;
}

Symbol KWDRNBRegressor::ComputeTargetQuantileDistribution() const
{
	ALString sQuantileDistribution;
	ContinuousVector cvTargetRankIntervalProbs;
	Continuous cRankIntervalProb;
	Continuous cValueIntervalProb;
	Continuous cValueCumulativeProb;
	Continuous cBound;
	int nTarget;
	Continuous cTargetCumulativeFrequency;
	Continuous cTargetFrequency;
	int nCumulativeFrequency;
	int nValue;
	int nInstance;
	int nValueFrequency;

	require(IsOptimized());
	require(rankRegressorRule->cvTargetProbs.GetSize() == rankRegressorRule->GetDataGridSetTargetValueNumber());

	// En cas de valeurs manquantes, on ne renvoie rien
	if (nMissingValueNumber > 0)
		return Symbol();

	// Parcours des parties cible, synchronisee avec le parcours des valeurs
	// On ne gere pas explicitement le cas d'une base vide
	nValue = 0;
	nInstance = 0;
	cTargetCumulativeFrequency = 0;
	cValueCumulativeProb = 0;
	for (nTarget = 0; nTarget < rankRegressorRule->GetDataGridSetTargetValueNumber(); nTarget++)
	{
		// Recherche de l'effectif de la partie cible courante
		cTargetFrequency =
		    rankRegressorRule->GetDataGridSetTargetCumulativeFrequencyAt(nTarget) - cTargetCumulativeFrequency;
		cTargetCumulativeFrequency = rankRegressorRule->GetDataGridSetTargetCumulativeFrequencyAt(nTarget);
		nCumulativeFrequency = (int)floor(0.5 + cTargetCumulativeFrequency);

		// Calcul de la probabilite la partie cible
		cRankIntervalProb = rankRegressorRule->cvTargetProbs.GetAt(nTarget) * cTargetFrequency;

		// Parcours des valeurs de la partie cible de la partie cible
		while (nInstance < nCumulativeFrequency)
		{
			// Calcul de la frequence et de la frequence cumulee des valeurs
			nValueFrequency = GetSingleTargetValueFrequencyAt(nValue);
			nInstance = GetSingleTargetValueCumulativeFrequencyAt(nValue);
			assert(nInstance <= nCumulativeFrequency);

			// Calcul de la probabilite de l'intervalle de valeur
			cValueIntervalProb = cRankIntervalProb * nValueFrequency / cTargetFrequency;

			// Gestion du demi interval precedant la valeur
			// Cas particulier de la premiere valeur
			if (nValue == 0)
			{
				// Le premier demi intervalle est decoupe pour moitie en exponentielle decroissante,
				// pour moitie en densite constante (entre v0-cMeanTargetValueRange/2 et v0)
				// On decoupe la queue exponentielle en deux partie d'eqale probabilite
				cValueCumulativeProb += cValueIntervalProb / 8;
				cBound = GetSingleTargetValueAt(nValue) - cMeanTargetValueRange / 2 -
					 cTargetValueRange * (Continuous)log(2.0);
				sQuantileDistribution += KWContinuous::ContinuousToString(cValueCumulativeProb);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cBound);

				// Deuxieme partie de la queue de distribution exponentielle
				cValueCumulativeProb += cValueIntervalProb / 8;
				cBound = GetSingleTargetValueAt(nValue) - cMeanTargetValueRange / 2;
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cValueCumulativeProb);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cBound);

				// Partie constante precedant la premiere valeur
				cValueCumulativeProb += cValueIntervalProb / 4;
				cBound = GetSingleTargetValueAt(nValue);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cValueCumulativeProb);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cBound);
			}
			// Cas general
			else
			{
				cValueCumulativeProb += cValueIntervalProb / 2;
				cBound = GetSingleTargetValueAt(nValue);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cValueCumulativeProb);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cBound);
			}

			// Gestion du demi interval suivant la valeur
			// Cas particulier de la derniere valeur
			if (nValue == GetSingleTargetValueNumber() - 1)
			{
				// Le dernier demi intervalle est decoupe pour moitie en densite constante,
				// pour moitie en exponentielle decroissante
				// Partie constante suivant la derniere la premiere valeur
				cValueCumulativeProb += cValueIntervalProb / 4;
				cBound = GetSingleTargetValueAt(nValue) + cMeanTargetValueRange / 2;
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cValueCumulativeProb);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cBound);

				// On decoupe la queue exponentielle en deux partie d'eqale probabilite
				cValueCumulativeProb += cValueIntervalProb / 8;
				cBound = GetSingleTargetValueAt(nValue) + cMeanTargetValueRange / 2 +
					 cTargetValueRange * (Continuous)log(2.0);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cValueCumulativeProb);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cBound);

				// Deuxieme partie de la queue de distribution exponentielle: on doit arrive
				// a une probabilite cumulee egale a 1
				assert(fabs(cValueCumulativeProb + cValueIntervalProb / 8 - 1) < 1e-5);
			}
			// Cas general
			else
			{
				cValueCumulativeProb += cValueIntervalProb / 2;
				cBound = (GetSingleTargetValueAt(nValue) + GetSingleTargetValueAt(nValue + 1)) / 2;
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cValueCumulativeProb);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cBound);
			}

			// Passage a la valeur suivante
			nValue++;
		}
	}
	assert(nValue == GetSingleTargetValueNumber());

	return Symbol(sQuantileDistribution);
}

longint KWDRNBRegressor::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRRegressor::GetUsedMemory();
	lUsedMemory += sizeof(KWDRNBRegressor) - sizeof(KWDRRegressor);
	lUsedMemory += ivSingleTargetValueCumulativeInstanceNumbers.GetUsedMemory();
	lUsedMemory += cvCumulativeTargetValues.GetUsedMemory();
	lUsedMemory += cvCumulativeSquareTargetValues.GetUsedMemory();
	return lUsedMemory;
}

void KWDRNBRegressor::ComputeSingleTargetValueCumulativeInstanceNumbers(IntVector* ivResultVector) const
{
	int nIndex;
	int nCumulatedFrequency;

	require(IsCompiled());
	require(ivResultVector != NULL);

	// Calcul du vecteur des effectif cumules
	nCumulatedFrequency = 0;
	ivResultVector->SetSize(GetSingleTargetValueNumber());
	for (nIndex = 0; nIndex < ivResultVector->GetSize(); nIndex++)
	{
		nCumulatedFrequency += GetSingleTargetValueFrequencyAt(nIndex);
		ivResultVector->SetAt(nIndex, nCumulatedFrequency);
	}
}

void KWDRNBRegressor::ComputeCumulativeTargetValues(ContinuousVector* cvResultVector) const
{
	boolean bKernelCorrection = true;
	int nTarget;
	int nCumulativeFrequency;
	int nValue;
	int nInstance;
	Continuous cCumulativeValue;
	int nValueFrequency;
	Continuous cFirstValue;
	Continuous cLastValue;

	require(IsCompiled());
	require(cvResultVector != NULL);
	require(cTargetValueRange == ComputeTargetValueRange());
	require(cMeanTargetValueRange == ComputeMeanTargetValueRange());

	// Calcul des valeurs extremes
	cFirstValue = -cMeanTargetValueRange / 4;
	cLastValue = cMeanTargetValueRange / 4;
	if (GetSingleTargetValueNumber() > 0)
	{
		cFirstValue += 3 * GetSingleTargetValueAt(0) / 4;
		cLastValue += 3 * GetSingleTargetValueAt(GetSingleTargetValueNumber() - 1) / 4;
	}

	// Calcul du vecteur des valeurs cumulees
	cvResultVector->SetSize(rankRegressorRule->GetDataGridSetTargetValueNumber());
	nValue = 0;
	nInstance = 0;
	cCumulativeValue = 0;
	for (nTarget = 0; nTarget < rankRegressorRule->GetDataGridSetTargetValueNumber(); nTarget++)
	{
		nCumulativeFrequency =
		    (int)floor(0.5 + rankRegressorRule->GetDataGridSetTargetCumulativeFrequencyAt(nTarget));

		// Calcul du cumul des valeurs en prenant en compte les instance de la partie cible
		while (nInstance < nCumulativeFrequency)
		{
			// Calcul de la frequence et de la frequence cumulee des valeurs
			nValueFrequency = GetSingleTargetValueFrequencyAt(nValue);
			nInstance = GetSingleTargetValueCumulativeFrequencyAt(nValue);
			assert(nInstance <= nCumulativeFrequency);

			// Mise a jour de la valeur cumulee
			cCumulativeValue += nValueFrequency * GetSingleTargetValueAt(nValue);

			// Correction pour la premiere et derniere instance de chaque valeur
			if (bKernelCorrection)
			{
				cCumulativeValue -= GetSingleTargetValueAt(nValue);
				if (nValue > 0)
					cCumulativeValue +=
					    (GetSingleTargetValueAt(nValue - 1) + GetSingleTargetValueAt(nValue) * 3) /
					    8;
				else
					cCumulativeValue += cFirstValue;
				if (nValue < GetSingleTargetValueNumber() - 1)
					cCumulativeValue +=
					    (GetSingleTargetValueAt(nValue) * 3 + GetSingleTargetValueAt(nValue + 1)) /
					    8;
				else
					cCumulativeValue += cLastValue;
			}

			// Passage a la valeur suivante
			nValue++;
		}

		// Memorisation de la valeur cumulee
		cvResultVector->SetAt(nTarget, cCumulativeValue);
	}
	assert(nValue == GetSingleTargetValueNumber());
}

void KWDRNBRegressor::ComputeCumulativeSquareTargetValues(ContinuousVector* cvResultVector) const
{
	boolean bKernelCorrection = true;
	int nTarget;
	int nCumulativeFrequency;
	int nValue;
	int nInstance;
	Continuous cCumulativeValue;
	int nValueFrequency;
	Continuous cFirstValue;
	Continuous cLastValue;

	require(IsCompiled());
	require(cvResultVector != NULL);
	require(cTargetValueRange == ComputeTargetValueRange());
	require(cMeanTargetValueRange == ComputeMeanTargetValueRange());

	// Calcul des valeurs extremes
	if (GetSingleTargetValueNumber() == 0)
	{
		cFirstValue = 0;
		cLastValue = cFirstValue;
	}
	else
	{
		cFirstValue = GetSingleTargetValueAt(0);
		cLastValue = GetSingleTargetValueAt(GetSingleTargetValueNumber() - 1);
	}
	cFirstValue = (cFirstValue * cFirstValue + cFirstValue * (cFirstValue - cMeanTargetValueRange / 2) +
		       (cFirstValue - cMeanTargetValueRange / 2) * (cFirstValue - cMeanTargetValueRange / 2)) /
			  24 +
		      ((cFirstValue - cTargetValueRange) * (cFirstValue - cTargetValueRange) +
		       cTargetValueRange * cTargetValueRange) /
			  4;
	cLastValue = (cLastValue * cLastValue + cLastValue * (cLastValue + cMeanTargetValueRange / 2) +
		      (cLastValue + cMeanTargetValueRange / 2) * (cLastValue + cMeanTargetValueRange / 2)) /
			 24 +
		     ((cLastValue + cTargetValueRange) * (cLastValue + cTargetValueRange) +
		      cTargetValueRange * cTargetValueRange) /
			 4;

	// Calcul du vecteur des valeurs cumulees
	cvResultVector->SetSize(rankRegressorRule->GetDataGridSetTargetValueNumber());
	nValue = 0;
	nInstance = 0;
	cCumulativeValue = 0;
	for (nTarget = 0; nTarget < rankRegressorRule->GetDataGridSetTargetValueNumber(); nTarget++)
	{
		nCumulativeFrequency =
		    (int)floor(0.5 + rankRegressorRule->GetDataGridSetTargetCumulativeFrequencyAt(nTarget));

		// Calcul du cumul des valeurs en prenant en compte les instance de la partie cible
		while (nInstance < nCumulativeFrequency)
		{
			// Calcul de la frequence et de la frequence cumulee des valeurs
			nValueFrequency = GetSingleTargetValueFrequencyAt(nValue);
			nInstance = GetSingleTargetValueCumulativeFrequencyAt(nValue);
			assert(nInstance <= nCumulativeFrequency);

			// Mise a jour de la valeur cumulee
			cCumulativeValue +=
			    nValueFrequency * GetSingleTargetValueAt(nValue) * GetSingleTargetValueAt(nValue);

			// Correction pour la premiere et derniere instance de chaque valeur
			if (bKernelCorrection)
			{
				cCumulativeValue -= GetSingleTargetValueAt(nValue) * GetSingleTargetValueAt(nValue);
				if (nValue > 0)
					cCumulativeValue +=
					    (GetSingleTargetValueAt(nValue - 1) * GetSingleTargetValueAt(nValue - 1) +
					     GetSingleTargetValueAt(nValue - 1) * GetSingleTargetValueAt(nValue) +
					     GetSingleTargetValueAt(nValue) * GetSingleTargetValueAt(nValue)) /
					    6;
				else
					cCumulativeValue += cFirstValue;
				if (nValue < GetSingleTargetValueNumber() - 1)
					cCumulativeValue +=
					    (GetSingleTargetValueAt(nValue) * GetSingleTargetValueAt(nValue) +
					     GetSingleTargetValueAt(nValue) * GetSingleTargetValueAt(nValue + 1) +
					     GetSingleTargetValueAt(nValue + 1) * GetSingleTargetValueAt(nValue + 1)) /
					    6;
				else
					cCumulativeValue += cLastValue;
			}

			// Passage a la valeur suivante
			nValue++;
		}

		// Memorisation de la valeur cumulee
		cvResultVector->SetAt(nTarget, cCumulativeValue);
	}
	assert(nValue == GetSingleTargetValueNumber());
}

Continuous KWDRNBRegressor::ComputeTargetValueRange() const
{
	Continuous cTargetValueRange;

	require(IsCompiled());

	if (GetSingleTargetValueNumber() > 1)
		cTargetValueRange =
		    GetSingleTargetValueAt(GetSingleTargetValueNumber() - 1) - GetSingleTargetValueAt(0);
	else
		cTargetValueRange = 1.0;

	ensure(cTargetValueRange > 0);
	return cTargetValueRange;
}

Continuous KWDRNBRegressor::ComputeMeanTargetValueRange() const
{
	Continuous cMeanTargetValueRange;

	require(IsCompiled());

	if (GetSingleTargetValueNumber() > 1)
		cMeanTargetValueRange =
		    GetSingleTargetValueAt(GetSingleTargetValueNumber() - 1) - GetSingleTargetValueAt(0);
	else
		cMeanTargetValueRange = 1.0;
	if (GetSingleTargetValueNumber() >= 0)
		cMeanTargetValueRange /= GetSingleTargetValueCumulativeFrequencyAt(GetSingleTargetValueNumber() - 1);

	ensure(cMeanTargetValueRange > 0);
	return cMeanTargetValueRange;
}

int KWDRNBRegressor::ComputeMissingValueNumber() const
{
	int nResult;
	int nTargetValue;

	require(IsCompiled());

	nResult = 0;
	for (nTargetValue = 0; nTargetValue < GetSingleTargetValueNumber(); nTargetValue++)
	{
		if (GetSingleTargetValueAt(nTargetValue) == KWContinuous::GetMissingValue())
			nResult++;
	}
	return nResult;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRSNBRegressor

KWDRSNBRegressor::KWDRSNBRegressor()
{
	SetName("SNBRegressor");
	SetLabel("Selective Naive Bayes regressor");
}

KWDRSNBRegressor::~KWDRSNBRegressor() {}

KWDerivationRule* KWDRSNBRegressor::Create() const
{
	return new KWDRSNBRegressor;
}
