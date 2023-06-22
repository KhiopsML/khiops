// Copyright (c) 2023 Orange. All rights reserved.
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

///////////////////////////////////////////////////////////////
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
	KWDRContinuousVector continuousVectorRule;
	KWDRDataGridStats dataGridStatsRule;
	KWDRDataGrid dataGridRule;
	int nOperand;
	KWDerivationRuleOperand* operand;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());

	// Verification d'un nombre d'operande minimal
	if (GetOperandNumber() < 1 + nFirstDataGridOperand)
	{
		bOk = false;
		AddError(sTmp + "The number of operands should be at least " + IntToString(1 + nFirstDataGridOperand));
	}

	// Verification du type de structure de l'eventuel vecteur de poids en premier operande
	if (nFirstDataGridOperand == 1)
	{
		operand = GetOperandAt(0);
		assert(operand->GetDerivationRule() != NULL);
		if (operand->GetStructureName() != continuousVectorRule.GetName())
		{
			bOk = false;
			AddError(sTmp + "Incorrect structure(" + operand->GetStructureName() + ") for first operand " +
				 " (must be " + continuousVectorRule.GetName() + ")");
		}
	}

	// Verification du type de structure des operandes de statistiques par grilles
	for (nOperand = nFirstDataGridOperand; nOperand < GetOperandNumber() - 1; nOperand++)
	{
		operand = GetOperandAt(nOperand);

		// Verification d'une structure de partition univariee
		if (operand->GetStructureName() != dataGridStatsRule.GetName())
		{
			bOk = false;
			AddError(sTmp + "Incorrect structure(" + operand->GetStructureName() + ") for operand " +
				 IntToString(nOperand + 1) + " (must be " + dataGridStatsRule.GetName() + ")");
			break;
		}
	}

	// Verification du dernier operande
	if (bOk and GetOperandNumber() > 0)
	{
		operand = GetOperandAt(GetOperandNumber() - 1);

		// Verification d'une structure de type data grid univarie
		if (operand->GetStructureName() != dataGridRule.GetName())
		{
			bOk = false;
			AddError(sTmp + "Incorrect structure(" + operand->GetStructureName() +
				 ") for last operand (must be " + dataGridRule.GetName() + ")");
		}
	}
	return bOk;
}

boolean KWDRNBClassifier::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	int nOperand;
	KWDRDataGridStats* dataGridStats;
	KWDRDataGridStats refDataGridStats;
	KWDRDataGrid* dataGrid;
	KWDRDataGrid refDataGrid;
	KWDRFrequencies* frequencies;
	int nTotalFrequency;
	int nReferenceTotalFrequency;
	KWDRUnivariatePartition* univariatePartitionRule;
	KWDRSymbolValueSet symbolValueSetRefRule;
	KWDRValueGroups symbolValueGroupsRefRule;
	KWDRSymbolValueSet* symbolValueSetRule;
	KWDRValueGroups* symbolValueGroupsRule;
	KWDRValueGroup* symbolValueGroupRule;
	KWDRSymbolValueSet checkedSymbolValueSetRule;
	KWDRValueGroup checkedSymbolValueGroupRule;
	KWDRSymbolValueSet checkedReferenceSymbolValueSetRule;
	int nValue;
	int nGroup;
	Symbol sValue;
	ALString sTmp;
	NumericKeyDictionary nkdCheckedValues;

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Analyse du dernier operande qui fournit la distribution des valeurs cibles
	if (bOk)
		bOk = CheckReferencedDerivationRuleAt(GetOperandNumber() - 1, kwcOwnerClass, refDataGrid.GetName());
	dataGrid = NULL;
	if (bOk)
	{
		nReferenceTotalFrequency = 0;
		dataGrid = cast(KWDRDataGrid*,
				GetOperandAt(GetOperandNumber() - 1)->GetReferencedDerivationRule(kwcOwnerClass));
		if (dataGrid->GetOperandNumber() - 1 != 1)
		{
			AddError(sTmp + "Last operand must be a univariate data grid");
			bOk = false;
		}
		else if (dataGrid->GetUncheckedAttributeTypeAt(0) != KWType::Symbol)
		{
			AddError(sTmp + "Last operand must be a valid univariate categorial data grid");
			bOk = false;
		}
		else if (dataGrid->GetUncheckedAttributeTypeAt(dataGrid->GetOperandNumber() - 2))
		{
			// L'erreur sur la grille doit etre diagniostique par ailleurs
			bOk = false;
		}
		else
		{
			univariatePartitionRule =
			    cast(KWDRUnivariatePartition*, dataGrid->GetOperandAt(dataGrid->GetOperandNumber() - 2)
							       ->GetReferencedDerivationRule(kwcOwnerClass));

			// Test du type de partition cible
			if (univariatePartitionRule->GetStructureName() != symbolValueSetRefRule.GetStructureName())
			{
				AddError(sTmp + "Type of target partition (" +
					 univariatePartitionRule->GetStructureName() + ") should be " +
					 symbolValueSetRefRule.GetStructureName());
				bOk = false;
			}
		}
	}

	// Memorisation de l'effectif total
	nReferenceTotalFrequency = 0;
	if (bOk)
	{
		check(dataGrid);
		frequencies = cast(KWDRFrequencies*, dataGrid->GetOperandAt(dataGrid->GetOperandNumber() - 1)
							 ->GetReferencedDerivationRule(kwcOwnerClass));
		nReferenceTotalFrequency = frequencies->ComputeTotalFrequency();
	}

	// Memorisation des valeurs cibles, en transferant les valeurs vers la representation structuree
	// La representation de la regle a verifier est structuree ou non selon qu'elle a a ete fabriquee
	// par programme (pour un classifieur) ou qu'elle est issue d'une lecture de fichier dictionnaire
	// (avant compilation). On passe alors par une regle temporaire (checkedReferenceSymbolValueSetRule)
	//  permettant d'avoir un acces systematique a la represnetation structuree, ce qui permet d'effectuer
	// des controles sans modifier (par compilation ou autre) la regle a verifier
	if (bOk)
	{
		symbolValueSetRule = cast(KWDRSymbolValueSet*, dataGrid->GetOperandAt(dataGrid->GetOperandNumber() - 2)
								   ->GetReferencedDerivationRule(kwcOwnerClass));

		// La methode suivante force la representation structuree
		checkedReferenceSymbolValueSetRule.SetValueNumber(symbolValueSetRule->GetValueNumber());

		// Transfert vers une representation structuree
		if (not symbolValueSetRule->GetStructureInterface())
			checkedReferenceSymbolValueSetRule.BuildStructureFromBase(symbolValueSetRule);
		else
			checkedReferenceSymbolValueSetRule.CopyStructureFrom(symbolValueSetRule);
	}

	// Verification de la compatibilite des arguments avec la classification
	if (bOk)
	{
		assert(GetOperandNumber() >= 1 + nFirstDataGridOperand);

		// Parcours des operandes de type stats par grille
		for (nOperand = nFirstDataGridOperand; nOperand < GetOperandNumber() - 1; nOperand++)
		{
			bOk = CheckReferencedDerivationRuleAt(nOperand, kwcOwnerClass, refDataGridStats.GetName());
			dataGridStats = NULL;
			if (bOk)
				dataGridStats =
				    cast(KWDRDataGridStats*,
					 GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass));

			// Verification du type du dernier attribut de la grille utilise pour les stats
			if (bOk)
				bOk = dataGridStats->CheckPredictorCompletness(KWType::Symbol, kwcOwnerClass);
			if (bOk)
			{
				// Calcul de l'effectif total de la grille referencee
				dataGrid =
				    cast(KWDRDataGrid*,
					 dataGridStats->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
				frequencies =
				    cast(KWDRFrequencies*, dataGrid->GetOperandAt(dataGrid->GetOperandNumber() - 1)
							       ->GetReferencedDerivationRule(kwcOwnerClass));
				nTotalFrequency = frequencies->ComputeTotalFrequency();

				// Verification de la compatibilite des effectifs totaux des grilles
				if (nTotalFrequency != nReferenceTotalFrequency)
				{
					AddError(sTmp + "Total frequency of the data grid operand " +
						 IntToString(nOperand - nFirstDataGridOperand + 1) + " (" +
						 IntToString(nTotalFrequency) +
						 ") does not match that of the first data grid operand (" +
						 IntToString(nReferenceTotalFrequency) + ")");
					bOk = false;
					break;
				}
			}
		}
	}

	// Verification du fait que les valeurs cibles sont les memes pour chaque grille
	if (bOk)
	{
		// Parcours des operandes de type stats par grille
		for (nOperand = nFirstDataGridOperand; nOperand < GetOperandNumber() - 1; nOperand++)
		{
			assert(CheckReferencedDerivationRuleAt(nOperand, kwcOwnerClass, refDataGridStats.GetName()));
			dataGridStats = cast(KWDRDataGridStats*,
					     GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass));

			// Recherche des valeurs cibles et de leur effectif
			dataGrid = cast(KWDRDataGrid*,
					dataGridStats->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
			univariatePartitionRule =
			    cast(KWDRUnivariatePartition*, dataGrid->GetOperandAt(dataGrid->GetOperandNumber() - 2)
							       ->GetReferencedDerivationRule(kwcOwnerClass));
			assert(univariatePartitionRule->GetAttributeType() == KWType::Symbol);

			// Test du type de partition cible: soit ensemble de valeurs, soit ensemble de groupes de
			// valeurs
			symbolValueSetRule = NULL;
			symbolValueGroupsRule = NULL;
			if (univariatePartitionRule->GetStructureName() == symbolValueSetRefRule.GetStructureName())
				symbolValueSetRule = cast(KWDRSymbolValueSet*, univariatePartitionRule);
			else if (univariatePartitionRule->GetStructureName() ==
				 symbolValueGroupsRefRule.GetStructureName())
				symbolValueGroupsRule = cast(KWDRValueGroups*, univariatePartitionRule);
			else
			{
				AddError(sTmp + "Type of target partition of the data grid operand " +
					 IntToString(nOperand - nFirstDataGridOperand + 1) + " (" +
					 univariatePartitionRule->GetStructureName() + ") should be " +
					 symbolValueSetRefRule.GetStructureName() + " or " +
					 symbolValueGroupsRefRule.GetStructureName());
				bOk = false;
				break;
			}
			assert((symbolValueSetRule == NULL and symbolValueGroupsRule != NULL) or
			       (symbolValueSetRule != NULL and symbolValueGroupsRule == NULL));
			assert(bOk);

			// Cas d'une grille definie par un ensemble de valeurs
			if (symbolValueSetRule != NULL)
			{
				// Memorisation des valeurs cibles, en transferant les valeurs vers la representation
				// structuree
				checkedSymbolValueSetRule.SetValueNumber(symbolValueSetRule->GetValueNumber());
				if (not symbolValueSetRule->GetStructureInterface())
					checkedSymbolValueSetRule.BuildStructureFromBase(symbolValueSetRule);
				else
					checkedSymbolValueSetRule.CopyStructureFrom(symbolValueSetRule);

				// Verification du nombre de valeurs
				if (checkedSymbolValueSetRule.GetValueNumber() !=
				    checkedReferenceSymbolValueSetRule.GetValueNumber())
				{
					AddError(sTmp + "Size of target partition of the data grid operand " +
						 IntToString(nOperand - nFirstDataGridOperand + 1) + " (" +
						 IntToString(checkedSymbolValueSetRule.GetValueNumber()) +
						 ") should be equal to " +
						 IntToString(checkedReferenceSymbolValueSetRule.GetValueNumber()));
					bOk = false;
					break;
				}
				// Verification de chaque valeur
				else
				{
					for (nValue = 0; nValue < checkedSymbolValueSetRule.GetValueNumber(); nValue++)
					{
						if (checkedSymbolValueSetRule.GetValueAt(nValue) !=
						    checkedReferenceSymbolValueSetRule.GetValueAt(nValue))
						{
							AddError(sTmp + "Unexpected target value (" +
								 checkedSymbolValueSetRule.GetValueAt(nValue) +
								 ") in the data grid operand " +
								 IntToString(nOperand - nFirstDataGridOperand + 1));
							bOk = false;
							break;
						}
					}
					if (not bOk)
						break;
				}
			}
			// Cas d'une grille definie par un ensemble de groupes de valeurs
			else if (symbolValueGroupsRule != NULL)
			{
				// Rangement des valeurs cibles dans un dictionnaire
				for (nValue = 0; nValue < checkedReferenceSymbolValueSetRule.GetValueNumber(); nValue++)
				{
					sValue = checkedReferenceSymbolValueSetRule.GetValueAt(nValue);
					nkdCheckedValues.SetAt(sValue.GetNumericKey(), &nkdCheckedValues);
				}

				// Parcours des groupes de valeurs a verifier
				for (nGroup = 0; nGroup < symbolValueGroupsRule->GetValueGroupNumber(); nGroup++)
				{
					symbolValueGroupRule = symbolValueGroupsRule->GetValueGroupAt(nGroup);

					// Memorisation des valeurs du groupe, en transferant les valeurs vers la
					// representation structuree
					checkedSymbolValueGroupRule.SetValueNumber(
					    symbolValueGroupRule->GetValueNumber());
					if (not symbolValueGroupRule->GetStructureInterface())
						checkedSymbolValueGroupRule.BuildStructureFromBase(
						    symbolValueGroupRule);
					else
						checkedSymbolValueGroupRule.CopyStructureFrom(symbolValueGroupRule);

					// Verification des valeurs du groupes
					for (nValue = 0; nValue < checkedSymbolValueGroupRule.GetValueNumber();
					     nValue++)
					{
						// Verification, sauf pour la valeur speciale
						sValue = checkedSymbolValueGroupRule.GetValueAt(nValue);
						if (sValue != Symbol::GetStarValue() and
						    nkdCheckedValues.Lookup(sValue.GetNumericKey()) == NULL)
						{
							AddError(sTmp + "Unexpected target value (" +
								 checkedSymbolValueGroupRule.GetValueAt(nValue) +
								 ") in a group of values of the data grid operand " +
								 IntToString(nOperand - nFirstDataGridOperand + 1));
							bOk = false;
							break;
						}
					}
					if (not bOk)
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

	// Recherche de l'index de la probabilites max
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
	boolean bDisplayDetails = false;
	int nDataGridIndex;
	const KWDRDataGridStats* dataGridStats;
	const KWDRDataGrid* targetDataGrid;
	const KWDRSymbolValueSet* targetSymbolValueSet;
	const KWDRContinuousVector* continuousVector;
	const KWDRDataGrid* dataGrid;
	const KWDRUnivariatePartition* univariatePartitionRule;
	int nTarget;
	int nTargetValueNumber;
	ContinuousVector cvAllFrequencies;
	int i;

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Memorisation des statistiques des grilles
		// (le dernier operandes ne doit pas etre pris: il s'agit de la distribution des valeurs cibles)
		oaDataGridStats.SetSize(GetOperandNumber() - 1 - nFirstDataGridOperand);
		for (i = 0; i < oaDataGridStats.GetSize(); i++)
			oaDataGridStats.SetAt(
			    i, GetOperandAt(nFirstDataGridOperand + i)->GetReferencedDerivationRule(kwcOwnerClass));

		// Si le vecteur de poids n'est pas precise, on met tous les poids a 1,
		// Sinon, on va les chercher dans le premier operande
		cvWeights.SetSize(oaDataGridStats.GetSize());
		if (nFirstDataGridOperand == 0)
		{
			for (i = 0; i < cvWeights.GetSize(); i++)
				cvWeights.SetAt(i, 1);
		}
		else
		{
			continuousVector =
			    cast(KWDRContinuousVector*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
			assert(continuousVector->GetValueNumber() == oaDataGridStats.GetSize());
			for (i = 0; i < cvWeights.GetSize(); i++)
				cvWeights.SetAt(i, continuousVector->GetValueAt(i));
		}

		// Recherche du dernier operande: distribution des valeurs cibles
		targetDataGrid = cast(const KWDRDataGrid*,
				      GetOperandAt(GetOperandNumber() - 1)->GetReferencedDerivationRule(kwcOwnerClass));
		assert(targetDataGrid->GetAttributeNumber() == 1);
		assert(targetDataGrid->GetAttributeTypeAt(0) == KWType::Symbol);
		nTargetValueNumber = targetDataGrid->GetTotalCellNumber();

		// Initialisation du vecteur des valeurs cibles
		targetSymbolValueSet =
		    cast(const KWDRSymbolValueSet*,
			 targetDataGrid->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
		assert(targetSymbolValueSet->GetValueNumber() == nTargetValueNumber);
		svTargetValues.SetSize(nTargetValueNumber);
		for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
			svTargetValues.SetAt(nTarget, targetSymbolValueSet->GetValueAt(nTarget));

		// Taillage du vecteur des probabilites cible
		cvTargetProbs.SetSize(nTargetValueNumber);
		cUnknownTargetProb = 0;

		// Memorisation des effectifs par partie cible
		ivDataGridSetTargetFrequencies.SetSize(nTargetValueNumber);
		for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
			ivDataGridSetTargetFrequencies.SetAt(nTarget, targetDataGrid->GetCellFrequencyAt(nTarget));

		// Calcul de la correspondance entre les index cibles de l'ensemble des grille et
		// les index cibles de chaque grille
		ivDataGridTargetIndexes.SetSize(ivDataGridSetTargetFrequencies.GetSize() * oaDataGridStats.GetSize());
		for (nDataGridIndex = 0; nDataGridIndex < oaDataGridStats.GetSize(); nDataGridIndex++)
		{
			dataGridStats = cast(const KWDRDataGridStats*, oaDataGridStats.GetAt(nDataGridIndex));

			// Recherche de la partition cible de la grille
			dataGrid = cast(const KWDRDataGrid*,
					dataGridStats->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
			univariatePartitionRule = cast(const KWDRUnivariatePartition*,
						       dataGrid->GetOperandAt(dataGrid->GetOperandNumber() - 2)
							   ->GetReferencedDerivationRule(kwcOwnerClass));
			assert(univariatePartitionRule->GetAttributeType() == KWType::Symbol);
			assert(univariatePartitionRule->IsCompiled());

			// Calcul de la correspondance entre index cible et index des partie cible de la grille
			for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
			{
				ivDataGridTargetIndexes.SetAt(
				    nDataGridIndex * nTargetValueNumber + nTarget,
				    univariatePartitionRule->GetSymbolPartIndex(svTargetValues.GetAt(nTarget)));
			}
		}

		// Affichage des resultats de compilation
		if (bDisplayDetails)
		{
			cout << "DataGridSet\n";
			cout << "DataGrid number\t" << oaDataGridStats.GetSize() << endl;

			// Affichage des effectifs  de la cible
			cout << "Target frequencies\n";
			cout << "DataGridSet";
			for (nDataGridIndex = 0; nDataGridIndex < oaDataGridStats.GetSize(); nDataGridIndex++)
				cout << "\tDataGrid " << nDataGridIndex;
			cout << endl;
			for (i = 0; i < ivDataGridSetTargetFrequencies.GetSize(); i++)
			{
				cout << ivDataGridSetTargetFrequencies.GetAt(i);
				for (nDataGridIndex = 0; nDataGridIndex < oaDataGridStats.GetSize(); nDataGridIndex++)
					cout << "\t"
					     << ivDataGridTargetIndexes.GetAt(
						    nDataGridIndex * ivDataGridSetTargetFrequencies.GetSize() + i);
				cout << endl;
			}
		}
	}
}

longint KWDRNBClassifier::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRClassifier::GetUsedMemory();
	lUsedMemory += sizeof(KWDRNBClassifier) - sizeof(KWDRClassifier);
	lUsedMemory += oaDataGridStats.GetUsedMemory();
	lUsedMemory += cvWeights.GetUsedMemory();
	lUsedMemory += ivDataGridSetTargetFrequencies.GetUsedMemory();
	lUsedMemory += ivDataGridTargetIndexes.GetUsedMemory();
	lUsedMemory += svTargetValues.GetUsedMemory();
	lUsedMemory += cvTargetProbs.GetUsedMemory();
	return lUsedMemory;
}

void KWDRNBClassifier::ComputeTargetProbs() const
{
	const KWDRDataGridStats* dataGridStats;
	int nDataGridIndex;
	int nSourceIndex;
	int nTargetIndex;
	int nTarget;
	int nTargetFrequency;
	int nTargetTotalFrequency;
	double dTargetLogProb;
	double dMaxTargetLogProb;
	double dProb;
	double dTotalProb;
	double dLaplaceEpsilon;
	double dLaplaceDenominator;

	require(IsCompiled());
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
	assert(nTargetTotalFrequency > 0);

	// Calcul des logarithmes de probabilites des valeurs cibles
	dMaxTargetLogProb = KWContinuous::GetMinValue();
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
	{
		// Initialisation avec le prior
		assert(GetDataGridSetTargetFrequencyAt(nTarget) > 0);
		dTargetLogProb = log(GetDataGridSetTargetFrequencyAt(nTarget) * 1.0 / nTargetTotalFrequency);

		// Ajout des probabilites conditionnelles par grille
		for (nDataGridIndex = 0; nDataGridIndex < GetDataGridStatsNumber(); nDataGridIndex++)
		{
			// Recherche de la grille
			dataGridStats = GetDataGridStatsAt(nDataGridIndex);

			// Recherche de l'index de la partie source
			nSourceIndex = dataGridStats->GetCellIndex();

			// Recherche de l'index de la partie cible de la grille
			nTargetIndex = GetDataGridSetTargetIndexAt(nDataGridIndex, nTarget);

			// Mise a jour du terme de proba, en prenant en compte le poids de la grille
			dTargetLogProb +=
			    GetDataGridWeightAt(nDataGridIndex) *
			    dataGridStats->GetDataGridSourceConditionalLogProbAt(nSourceIndex, nTargetIndex);
		}

		// Memorisation du resultat
		cvTargetProbs.SetAt(nTarget, (Continuous)dTargetLogProb);

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
	dLaplaceEpsilon = (Continuous)0.5 / (GetDataGridSetTargetPartNumber() * (nTargetTotalFrequency + 1));
	dLaplaceDenominator = (Continuous)(1.0 + 0.5 / (nTargetTotalFrequency + 1));
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
		cvTargetProbs.SetAt(
		    nTarget, (Continuous)((cvTargetProbs.GetAt(nTarget) + dLaplaceEpsilon) / dLaplaceDenominator));

	// Calcul d'une probabilite par defaut pour les classes inconnues
	cUnknownTargetProb = (Continuous)(dLaplaceEpsilon / dLaplaceDenominator);
}

///////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////
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

boolean KWDRNBRankRegressor::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	int nOperand;
	KWDRDataGridStats* dataGridStats;
	KWDRDataGridStats refDataGridStats;
	KWDRDataGrid* dataGrid;
	KWDRFrequencies* frequencies;
	int nTotalFrequency;
	int nReferenceTotalFrequency;
	ALString sTmp;

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de la compatibilite des arguments avec la regression
	if (bOk)
	{
		// Parcours des operandes de type stats par grille
		nReferenceTotalFrequency = 0;
		for (nOperand = nFirstDataGridOperand; nOperand < GetOperandNumber(); nOperand++)
		{
			bOk = CheckReferencedDerivationRuleAt(nOperand, kwcOwnerClass, refDataGridStats.GetName());
			dataGridStats = NULL;
			if (bOk)
				dataGridStats =
				    cast(KWDRDataGridStats*,
					 GetOperandAt(nOperand)->GetReferencedDerivationRule(kwcOwnerClass));

			// Verification du type du dernier attribut de la grille utilise pour les stats
			if (bOk)
				bOk = dataGridStats->CheckPredictorCompletness(KWType::Continuous, kwcOwnerClass);
			if (bOk)
			{
				// Calcul de l'effectif total de la grille referencee
				dataGrid =
				    cast(KWDRDataGrid*,
					 dataGridStats->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
				frequencies =
				    cast(KWDRFrequencies*, dataGrid->GetOperandAt(dataGrid->GetOperandNumber() - 1)
							       ->GetReferencedDerivationRule(kwcOwnerClass));
				nTotalFrequency = frequencies->ComputeTotalFrequency();

				// Verification de la compatibilite des effectifs totaux des grilles
				if (nOperand == nFirstDataGridOperand)
					nReferenceTotalFrequency = nTotalFrequency;
				else if (nTotalFrequency != nReferenceTotalFrequency)
				{
					AddError(sTmp + "Total frequency of the data grid operand " +
						 IntToString(nOperand - nFirstDataGridOperand + 1) + " (" +
						 IntToString(nTotalFrequency) +
						 ") does not match that of the first data grid operand (" +
						 IntToString(nReferenceTotalFrequency) + ")");
					bOk = false;
					break;
				}
			}
		}
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
	boolean bDisplayDetails = false;
	int nDataGridIndex;
	const KWDRDataGridStats* dataGridStats;
	const KWDRContinuousVector* continuousVector;
	int nTarget;
	ContinuousVector cvAllCumulativeFrequencies;
	int i;

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Memorisation des statistiques des grilles
		oaDataGridStats.SetSize(GetOperandNumber() - nFirstDataGridOperand);
		for (i = 0; i < oaDataGridStats.GetSize(); i++)
			oaDataGridStats.SetAt(
			    i, GetOperandAt(nFirstDataGridOperand + i)->GetReferencedDerivationRule(kwcOwnerClass));

		// Si le vecteur de poids n'est pas precise, on met tous les poids a 1,
		// Sinon, on va les chercher dans le premier operande
		cvWeights.SetSize(oaDataGridStats.GetSize());
		if (nFirstDataGridOperand == 0)
		{
			for (i = 0; i < cvWeights.GetSize(); i++)
				cvWeights.SetAt(i, 1);
		}
		else
		{
			continuousVector =
			    cast(KWDRContinuousVector*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
			assert(continuousVector->GetValueNumber() == oaDataGridStats.GetSize());
			for (i = 0; i < cvWeights.GetSize(); i++)
				cvWeights.SetAt(i, continuousVector->GetValueAt(i));
		}

		// Recherche de l'union de toutes les effectifs cumules pour toutes les grilles
		for (nDataGridIndex = 0; nDataGridIndex < oaDataGridStats.GetSize(); nDataGridIndex++)
		{
			dataGridStats = cast(const KWDRDataGridStats*, oaDataGridStats.GetAt(nDataGridIndex));

			// Ajout dans l'union des effectifs cumules de la grille
			for (nTarget = 0; nTarget < dataGridStats->GetDataGridTargetCellNumber(); nTarget++)
			{
				cvAllCumulativeFrequencies.Add(
				    dataGridStats->GetDataGridTargetCumulativeFrequencyAt(nTarget));
			}
		}

		// Tri de ces valeurs, supression des doublons et memorisation
		cvAllCumulativeFrequencies.Sort();
		cvDataGridSetTargetCumulativeFrequencies.SetSize(0);
		nTarget = 0;
		for (i = 0; i < cvAllCumulativeFrequencies.GetSize(); i++)
		{
			assert(KWContinuous::IsInt(cvAllCumulativeFrequencies.GetAt(i)));
			if (i == 0 or
			    cvAllCumulativeFrequencies.GetAt(i) > cvAllCumulativeFrequencies.GetAt(i - 1) + 0.5)
				cvDataGridSetTargetCumulativeFrequencies.Add(cvAllCumulativeFrequencies.GetAt(i));
		}

		// Calcul de la correspondance entre les index cibles de l'ensemble des grille et
		// les index cibles de chaque grille
		ivDataGridTargetIndexes.SetSize(cvDataGridSetTargetCumulativeFrequencies.GetSize() *
						oaDataGridStats.GetSize());
		for (nDataGridIndex = 0; nDataGridIndex < oaDataGridStats.GetSize(); nDataGridIndex++)
		{
			dataGridStats = cast(const KWDRDataGridStats*, oaDataGridStats.GetAt(nDataGridIndex));

			// Parcours des index cibles de l'ensemble des grilles
			nTarget = 0;
			for (i = 0; i < cvDataGridSetTargetCumulativeFrequencies.GetSize(); i++)
			{
				// Comparaison de l'effectif cumule pour la partition sur l'ensemble des grille
				// a l'effectif cumule pour la grille en cours
				if (cvDataGridSetTargetCumulativeFrequencies.GetAt(i) >
				    dataGridStats->GetDataGridTargetCumulativeFrequencyAt(nTarget) + 0.5)
					nTarget++;

				// Mise a jour de la correspondance
				ivDataGridTargetIndexes.SetAt(
				    nDataGridIndex * cvDataGridSetTargetCumulativeFrequencies.GetSize() + i, nTarget);
			}
		}

		// Taillage du vecteur des probabilites cible
		assert(cvDataGridSetTargetCumulativeFrequencies.GetSize() >= 1);
		cvTargetProbs.SetSize(cvDataGridSetTargetCumulativeFrequencies.GetSize());

		// Calcul des rangs et de leur carre cumules par partie cible
		ComputeCumulativeRanks(&cvTargetCumulativeRanks);
		ComputeCumulativeSquareRanks(&cvTargetCumulativeSquareRanks);

		// Affichage des resultats de compilation
		if (bDisplayDetails)
		{
			cout << "DataGridSet\n";
			cout << "DataGrid number\t" << oaDataGridStats.GetSize() << endl;

			// Affichage des effectifs cumules de la cible
			cout << "Cumulative frequencies\n";
			cout << "DataGridSet";
			for (nDataGridIndex = 0; nDataGridIndex < oaDataGridStats.GetSize(); nDataGridIndex++)
				cout << "\tDataGrid " << nDataGridIndex;
			cout << endl;
			for (i = 0; i < cvDataGridSetTargetCumulativeFrequencies.GetSize(); i++)
			{
				cout << cvDataGridSetTargetCumulativeFrequencies.GetAt(i);
				for (nDataGridIndex = 0; nDataGridIndex < oaDataGridStats.GetSize(); nDataGridIndex++)
					cout
					    << "\t"
					    << ivDataGridTargetIndexes.GetAt(
						   nDataGridIndex * cvDataGridSetTargetCumulativeFrequencies.GetSize() +
						   i);
				cout << endl;
			}
		}
	}
}

longint KWDRNBRankRegressor::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRRankRegressor::GetUsedMemory();
	lUsedMemory += sizeof(KWDRNBRankRegressor) - sizeof(KWDRRankRegressor);
	lUsedMemory += oaDataGridStats.GetUsedMemory();
	lUsedMemory += cvWeights.GetUsedMemory();
	lUsedMemory += cvDataGridSetTargetCumulativeFrequencies.GetUsedMemory();
	lUsedMemory += ivDataGridTargetIndexes.GetUsedMemory();
	lUsedMemory += cvTargetCumulativeSquareRanks.GetUsedMemory();
	lUsedMemory += cvTargetProbs.GetUsedMemory();
	return lUsedMemory;
}

void KWDRNBRankRegressor::ComputeTargetProbs() const
{
	const KWDRDataGridStats* dataGridStats;
	int nDataGridIndex;
	int nSourceIndex;
	int nTargetIndex;
	int nTarget;
	Continuous cTargetFrequency;
	Continuous cTargetTotalFrequency;
	Continuous cTargetLogProb;
	Continuous cMaxTargetLogProb;
	Continuous cProb;
	Continuous cTotalProb;
	Continuous cLaplaceEpsilon;
	Continuous cLaplaceDenominator;
	Continuous cRankNumber;
	Continuous cTargetCumulativeFrequency;

	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Calcul de l'esperance des valeurs
	cMaxTargetLogProb = KWContinuous::GetMinValue();
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
	{
		// Calcul du terme portant sur les probas
		cTargetLogProb = 0;
		for (nDataGridIndex = 0; nDataGridIndex < GetDataGridStatsNumber(); nDataGridIndex++)
		{
			// Recherche de la grille
			dataGridStats = GetDataGridStatsAt(nDataGridIndex);

			// Recherche de l'index de la partie source
			nSourceIndex = dataGridStats->GetCellIndex();

			// Recherche de l'index de la partie cible de la grille
			nTargetIndex = GetDataGridSetTargetIndexAt(nDataGridIndex, nTarget);

			// Mise a jour du terme de proba, en prenant en compte le poids de la grille
			cTargetLogProb +=
			    GetDataGridWeightAt(nDataGridIndex) *
			    dataGridStats->GetDataGridSourceConditionalLogProbAt(nSourceIndex, nTargetIndex);
		}

		// Memorisation du resultat
		cvTargetProbs.SetAt(nTarget, cTargetLogProb);

		// Memorisation du max
		if (cTargetLogProb > cMaxTargetLogProb)
			cMaxTargetLogProb = cTargetLogProb;
	}
	assert(cMaxTargetLogProb > KWContinuous::GetMinValue());

	// Calcul des probabilites des valeurs cibles, en normalisant par le dMaxTargetLogProb
	// pour eviter les valeurs extremes
	cTotalProb = 0;
	cTargetCumulativeFrequency = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
	{
		// Recherche de l'effectif de la partie cible courante
		cTargetFrequency = GetDataGridSetTargetCumulativeFrequencyAt(nTarget) - cTargetCumulativeFrequency;
		cTargetCumulativeFrequency = GetDataGridSetTargetCumulativeFrequencyAt(nTarget);

		// Normalisation de la probabilite
		cProb = (Continuous)exp(cvTargetProbs.GetAt(nTarget) - cMaxTargetLogProb);
		cvTargetProbs.SetAt(nTarget, cProb);

		// Calcul de la probabilite total au denominateur, en multipliant la probabilite (uniforme par rang)
		// par le nombre de rangs contenus dans la partie
		cTotalProb += cProb * cTargetFrequency;
	}
	assert(cTotalProb >= 1);

	// Normalisation pour obtenir des probas
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
		cvTargetProbs.SetAt(nTarget, cvTargetProbs.GetAt(nTarget) / cTotalProb);

	// Prise en compte d'un epsilon de Laplace (comme dans KWClassifierSelectionScore)
	// en considerant qu'on ne peut pas avoir de precision meilleure que 1/N
	//   p = p*N / N
	//   p_Laplace = (p*N + 0.5/J)/(N + 0.5)
	//   p_Laplace = (p + 0.5/JN)/(1 + 0.5/N)
	// (on se base sur N+1 pour eviter le cas N=0, et J=nRankNumber)
	cTargetTotalFrequency = GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetPartNumber() - 1);
	cRankNumber = cTargetTotalFrequency;
	if (cRankNumber == 0)
		cRankNumber = 1;
	cLaplaceEpsilon = (Continuous)0.5 / (cRankNumber * (cTargetTotalFrequency + 1));
	cLaplaceDenominator = (Continuous)(1.0 + 0.5 / (cTargetTotalFrequency + 1));
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
		cvTargetProbs.SetAt(nTarget, (cvTargetProbs.GetAt(nTarget) + cLaplaceEpsilon) / cLaplaceDenominator);
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
	require(cvTargetCumulativeValues->GetSize() == GetDataGridSetTargetPartNumber());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Calcul de l'esperance des valeurs
	cPreviousTargetCumulativeValue = 0;
	dMeanValue = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
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
	assert(cvCumulativeCrenel.GetSize() == GetDataGridSetTargetPartNumber());

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
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	// Recherche de l'effectif total
	cTargetTotalFrequency = 0;
	if (GetDataGridSetTargetPartNumber() > 0)
		cTargetTotalFrequency = GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetPartNumber() - 1);

	// Calcul du rang parmi l'effectif total en apprentissage
	cRank = cNormalizedRank * cTargetTotalFrequency;

	// Recherche du partile correspondant au rang
	cDensityRank = 1;
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
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
	if (GetDataGridSetTargetPartNumber() > 0)
		cTotalFrequency = GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetPartNumber() - 1);

	// Calcul de l'effectif equivalent au rang normalise donne en parametre
	cInputFrequency = cNormalizedRank * cTotalFrequency;

	// Calcul du vecteur des valeurs cumulees
	cvTargetCumulativeValues->SetSize(GetDataGridSetTargetPartNumber());
	cCumulativeValue = 0;
	cTargetCumulativeFrequency = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
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
	if (GetDataGridSetTargetPartNumber() > 0)
		cTotalFrequency = GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetPartNumber() - 1);

	// Calcul du vecteur des valeurs cumulees
	cvTargetCumulativeValues->SetSize(GetDataGridSetTargetPartNumber());
	cCumulativeValue = 0;
	cTargetCumulativeFrequency = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
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
	if (GetDataGridSetTargetPartNumber() > 0)
		cTotalFrequency = GetDataGridSetTargetCumulativeFrequencyAt(GetDataGridSetTargetPartNumber() - 1);

	// Calcul du vecteur des valeurs cumulees
	cvTargetCumulativeValues->SetSize(GetDataGridSetTargetPartNumber());
	cCumulativeValue = 0;
	cTargetCumulativeFrequency = 0;
	for (nTarget = 0; nTarget < GetDataGridSetTargetPartNumber(); nTarget++)
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

///////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////
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
	cTotalDeltaTargetValue = 0;
	cMeanDeltaTargetValue = 0;
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

void KWDRNBRegressor::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Memorisation des operandes
		rankRegressorRule =
		    cast(KWDRNBRankRegressor*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		targetDataGridRule =
		    cast(KWDRDataGrid*, GetSecondOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		targetValuesRules =
		    cast(KWDRContinuousValueSet*,
			 targetDataGridRule->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		targetFrequenciesRule =
		    cast(KWDRFrequencies*,
			 targetDataGridRule->GetSecondOperand()->GetReferencedDerivationRule(kwcOwnerClass));

		// Calcul des effectifs cumules des valeurs par index de valeur
		ComputeSingleTargetValueCumulativeInstanceNumbers(&ivSingleTargetValueCumulativeInstanceNumbers);

		// Calcul de l'ecart moyen inter-valeurs (Max-Min)/N, ou 1/N si Min=Max
		cTotalDeltaTargetValue = ComputeTotalDeltaTargetValue();
		cMeanDeltaTargetValue = ComputeMeanDeltaTargetValue();
		assert(cMeanDeltaTargetValue > 0);

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
		if (cValue <= -cMeanDeltaTargetValue / 2)
			cDensity = exp(-fabs(cValue + cMeanDeltaTargetValue / 2) / cTotalDeltaTargetValue) /
				   (4 * cTotalDeltaTargetValue);
		else if (cValue <= cMeanDeltaTargetValue / 2)
			cDensity = 1 / (2 * cMeanDeltaTargetValue);
		else
			cDensity = exp(-fabs(cValue - cMeanDeltaTargetValue / 2) / cTotalDeltaTargetValue) /
				   (4 * cTotalDeltaTargetValue);
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
		cClosestValue = GetSingleTargetValueAt(0) - cMeanDeltaTargetValue / 2;
		if (cValue <= cClosestValue)
			cDensity = cIntervalProb * exp(-fabs(cValue - cClosestValue) / cTotalDeltaTargetValue) /
				   (2 * cTotalDeltaTargetValue);
		else
			cDensity = cIntervalProb / cMeanDeltaTargetValue;
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
		cClosestValue = GetSingleTargetValueAt(0) + cMeanDeltaTargetValue / 2;
		if (cValue <= cClosestValue)
			cDensity = cIntervalProb / cMeanDeltaTargetValue;
		else
			cDensity = cIntervalProb * exp(-fabs(cValue - cClosestValue) / cTotalDeltaTargetValue) /
				   (2 * cTotalDeltaTargetValue);
	}
	// Cas general
	else
	{
		// On utilise ici la methode GetLowerMeanValue (plutot que GetHumanReadableLowerMeanValue), car les
		// valeurs des bornes ne sont pas montrees aux utilisateurs, et que l'on ne s'en sert que pour les
		// calculs dans une regle de derivation (exigence de rapidite)
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
	require(rankRegressorRule->cvTargetProbs.GetSize() == rankRegressorRule->GetDataGridSetTargetPartNumber());

	// En cas de valeurs manquantes, on ne renvoie rien
	if (nMissingValueNumber > 0)
		return Symbol();

	// Parcours des parties cible, synchronisee avec le parcours des valeurs
	// On ne gere pas explicitement le cas d'une base vide
	nValue = 0;
	nInstance = 0;
	cTargetCumulativeFrequency = 0;
	cValueCumulativeProb = 0;
	for (nTarget = 0; nTarget < rankRegressorRule->GetDataGridSetTargetPartNumber(); nTarget++)
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
				// pour moitie en densite constante (entre v0-cMeanDeltaTargetValue/2 et v0)
				// On decoupe la queue exponentielle en deux partie d'eqale probabilite
				cValueCumulativeProb += cValueIntervalProb / 8;
				cBound = GetSingleTargetValueAt(nValue) - cMeanDeltaTargetValue / 2 -
					 cTotalDeltaTargetValue * (Continuous)log(2.0);
				sQuantileDistribution += KWContinuous::ContinuousToString(cValueCumulativeProb);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cBound);

				// Deuxieme partie de la queue de distribution exponentielle
				cValueCumulativeProb += cValueIntervalProb / 8;
				cBound = GetSingleTargetValueAt(nValue) - cMeanDeltaTargetValue / 2;
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
				cBound = GetSingleTargetValueAt(nValue) + cMeanDeltaTargetValue / 2;
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cValueCumulativeProb);
				sQuantileDistribution += ' ';
				sQuantileDistribution += KWContinuous::ContinuousToString(cBound);

				// On decoupe la queue exponentielle en deux partie d'eqale probabilite
				cValueCumulativeProb += cValueIntervalProb / 8;
				cBound = GetSingleTargetValueAt(nValue) + cMeanDeltaTargetValue / 2 +
					 cTotalDeltaTargetValue * (Continuous)log(2.0);
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
	require(cTotalDeltaTargetValue == ComputeTotalDeltaTargetValue());
	require(cMeanDeltaTargetValue == ComputeMeanDeltaTargetValue());

	// Calcul des valeurs extremes
	cFirstValue = -cMeanDeltaTargetValue / 4;
	cLastValue = cMeanDeltaTargetValue / 4;
	if (GetSingleTargetValueNumber() > 0)
	{
		cFirstValue += 3 * GetSingleTargetValueAt(0) / 4;
		cLastValue += 3 * GetSingleTargetValueAt(GetSingleTargetValueNumber() - 1) / 4;
	}

	// Calcul du vecteur des valeurs cumulees
	cvResultVector->SetSize(rankRegressorRule->GetDataGridSetTargetPartNumber());
	nValue = 0;
	nInstance = 0;
	cCumulativeValue = 0;
	for (nTarget = 0; nTarget < rankRegressorRule->GetDataGridSetTargetPartNumber(); nTarget++)
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
	require(cTotalDeltaTargetValue == ComputeTotalDeltaTargetValue());
	require(cMeanDeltaTargetValue == ComputeMeanDeltaTargetValue());

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
	cFirstValue = (cFirstValue * cFirstValue + cFirstValue * (cFirstValue - cMeanDeltaTargetValue / 2) +
		       (cFirstValue - cMeanDeltaTargetValue / 2) * (cFirstValue - cMeanDeltaTargetValue / 2)) /
			  24 +
		      ((cFirstValue - cTotalDeltaTargetValue) * (cFirstValue - cTotalDeltaTargetValue) +
		       cTotalDeltaTargetValue * cTotalDeltaTargetValue) /
			  4;
	cLastValue = (cLastValue * cLastValue + cLastValue * (cLastValue + cMeanDeltaTargetValue / 2) +
		      (cLastValue + cMeanDeltaTargetValue / 2) * (cLastValue + cMeanDeltaTargetValue / 2)) /
			 24 +
		     ((cLastValue + cTotalDeltaTargetValue) * (cLastValue + cTotalDeltaTargetValue) +
		      cTotalDeltaTargetValue * cTotalDeltaTargetValue) /
			 4;

	// Calcul du vecteur des valeurs cumulees
	cvResultVector->SetSize(rankRegressorRule->GetDataGridSetTargetPartNumber());
	nValue = 0;
	nInstance = 0;
	cCumulativeValue = 0;
	for (nTarget = 0; nTarget < rankRegressorRule->GetDataGridSetTargetPartNumber(); nTarget++)
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

Continuous KWDRNBRegressor::ComputeTotalDeltaTargetValue() const
{
	Continuous cTotalDeltaValue;

	require(IsCompiled());

	if (GetSingleTargetValueNumber() <= 1)
		cTotalDeltaValue = 1.0;
	else
		cTotalDeltaValue = GetSingleTargetValueAt(GetSingleTargetValueNumber() - 1) - GetSingleTargetValueAt(0);
	return cTotalDeltaValue;
}

Continuous KWDRNBRegressor::ComputeMeanDeltaTargetValue() const
{
	Continuous cMeanDeltaValue;

	require(IsCompiled());

	if (GetSingleTargetValueNumber() <= 1)
		cMeanDeltaValue = 1.0;
	else
		cMeanDeltaValue = GetSingleTargetValueAt(GetSingleTargetValueNumber() - 1) - GetSingleTargetValueAt(0);
	if (GetSingleTargetValueNumber() >= 0)
		cMeanDeltaValue /= GetSingleTargetValueCumulativeFrequencyAt(GetSingleTargetValueNumber() - 1);
	return cMeanDeltaValue;
}

int KWDRNBRegressor::ComputeMissingValueNumber() const
{
	int nResult;
	int i;

	require(IsCompiled());

	nResult = 0;
	for (i = 0; i < GetSingleTargetValueNumber(); i++)
	{
		if (GetSingleTargetValueAt(i) == KWContinuous::GetMissingValue())
			nResult++;
	}
	return nResult;
}

///////////////////////////////////////////////////////////////
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