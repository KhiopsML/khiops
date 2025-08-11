// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRDataGrid.h"

void KWDRRegisterDataGridRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRDataGrid);
	KWDerivationRule::RegisterDerivationRule(new KWDRFrequencies);
	KWDerivationRule::RegisterDerivationRule(new KWDRCellIndex);
	KWDerivationRule::RegisterDerivationRule(new KWDRCellIndexWithMissing);
	KWDerivationRule::RegisterDerivationRule(new KWDRCellLabel);
	KWDerivationRule::RegisterDerivationRule(new KWDRCellId);
	KWDerivationRule::RegisterDerivationRule(new KWDRValueIndex);
	KWDerivationRule::RegisterDerivationRule(new KWDRValueRank);
	KWDerivationRule::RegisterDerivationRule(new KWDRValueRankSelfDistance);
	KWDerivationRule::RegisterDerivationRule(new KWDRInverseValueRank);
	KWDerivationRule::RegisterDerivationRule(new KWDRDataGridStats);
	KWDerivationRule::RegisterDerivationRule(new KWDRSourceConditionalInfo);
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGrid

KWDRDataGrid::KWDRDataGrid()
{
	SetName("DataGrid");
	SetLabel("Data grid");
	SetType(KWType::Structure);
	SetStructureName("DataGrid");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);

	// Les operandes contiennent des regles de type Structure
	GetFirstOperand()->SetType(KWType::Unknown);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);

	// Parametre de la compilation optimisee
	nOptimizationFreshness = 0;
	nDataGridAttributeNumber = 0;
}

KWDRDataGrid::~KWDRDataGrid() {}

void KWDRDataGrid::ImportDataGridStats(const KWDataGridStats* dataGridStats, boolean bImportGranularityFeatures)
{
	KWDerivationRuleOperand* operand;
	KWDRFrequencies* frequenciesRule;
	int nAttribute;
	const KWDGSAttributePartition* attributePartition;
	IntVector ivPartIndexes;
	int nCellNumber;
	int nCell;
	int nCellFrequency;

	require(dataGridStats != NULL);
	require(dataGridStats->Check());

	/////////////////////////////////////////////////////////////////////////////
	// Choix d'implementation
	// Les methodes ImportDataGridStats et ExportDataGridStats sont implementees
	// entierement dans la classe KWDRDataGrid, au lieu detre dispatchees dans
	// l'ensemble des classes des composants concernees (partition univariees, effectifs...)
	// Cela permet d'avoir une seule unite de maintenance, et reste coherent avec
	// l'aspect fonctionnel (des methodes Import... locales sont sans interet).

	// Reinitialisation
	DeleteAllOperands();

	// Creation des operandes de la regle
	for (nAttribute = 0; nAttribute < dataGridStats->GetAttributeNumber(); nAttribute++)
	{
		attributePartition = dataGridStats->GetAttributeAt(nAttribute);

		// Creation d'un operande d'indexation univarie
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetType(KWType::Structure);
		AddOperand(operand);

		// Initialisation des parties des attributs en fonction du type d'attribut
		// Cas d'une discretisation
		if (attributePartition->GetAttributeType() == KWType::Continuous and
		    not attributePartition->ArePartsSingletons())
		{
			KWDRIntervalBounds* intervalBoundsRule;

			// Creation d'une regle de type intervalles
			intervalBoundsRule = new KWDRIntervalBounds;
			operand->SetDerivationRule(intervalBoundsRule);
			operand->SetStructureName(intervalBoundsRule->GetStructureName());

			// Parametrage des intervalles
			intervalBoundsRule->ImportAttributeDiscretization(
			    cast(const KWDGSAttributeDiscretization*, attributePartition));
		}
		// Cas d'un ensemble de valeurs continues
		else if (attributePartition->GetAttributeType() == KWType::Continuous and
			 attributePartition->ArePartsSingletons())
		{
			KWDRContinuousValueSet* continuousValueSetRule;

			// Creation d'une regle de type ensemble de valeurs
			continuousValueSetRule = new KWDRContinuousValueSet;
			operand->SetDerivationRule(continuousValueSetRule);
			operand->SetStructureName(continuousValueSetRule->GetStructureName());

			// Parametrage des valeurs
			continuousValueSetRule->ImportAttributeContinuousValues(
			    cast(const KWDGSAttributeContinuousValues*, attributePartition));
		}
		// Cas d'un groupement de valeurs
		else if (attributePartition->GetAttributeType() == KWType::Symbol and
			 not attributePartition->ArePartsSingletons())
		{
			KWDRValueGroups* valueGroupsRule;

			// Creation d'une regle de type groupes
			valueGroupsRule = new KWDRValueGroups;
			operand->SetDerivationRule(valueGroupsRule);
			operand->SetStructureName(valueGroupsRule->GetStructureName());

			// Parametrage des groupes de valeurs
			valueGroupsRule->ImportAttributeGrouping(
			    cast(const KWDGSAttributeGrouping*, attributePartition));
		}
		// Cas d'un ensemble de valeurs symboliques
		else if (attributePartition->GetAttributeType() == KWType::Symbol and
			 attributePartition->ArePartsSingletons())
		{
			KWDRSymbolValueSet* symbolValueSetRule;

			// Creation d'une regle de type ensemble de valeurs
			symbolValueSetRule = new KWDRSymbolValueSet;
			operand->SetDerivationRule(symbolValueSetRule);
			operand->SetStructureName(symbolValueSetRule->GetStructureName());

			// Parametrage des valeurs
			symbolValueSetRule->ImportAttributeSymbolValues(
			    cast(const KWDGSAttributeSymbolValues*, attributePartition));
		}
		// Cas d'un ensemble de valeurs de type VarPart
		else if (attributePartition->GetAttributeType() == KWType::VarPart)
		{
			KWDRValueGroups* valueGroupsRule;

			// Creation d'une regle de type groupes
			valueGroupsRule = new KWDRValueGroups;
			operand->SetDerivationRule(valueGroupsRule);
			operand->SetStructureName(valueGroupsRule->GetStructureName());

			// Parametrage des groupes de valeurs
			valueGroupsRule->ImportAttributeGrouping(
			    cast(const KWDGSAttributeGrouping*, attributePartition));
		}

		assert(operand == GetOperandAt(nAttribute));
		assert(operand->GetDerivationRule() != NULL);
	}

	// Ajout d'un dernier operande pour les effectifs des cellules
	operand = new KWDerivationRuleOperand;
	operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
	operand->SetType(KWType::Structure);
	AddOperand(operand);

	// Creation d'une regle de type effectifs
	frequenciesRule = new KWDRFrequencies;
	operand->SetDerivationRule(frequenciesRule);
	operand->SetStructureName(frequenciesRule->GetStructureName());

	// Nettoyage prealable des operandes existants
	frequenciesRule->DeleteAllOperands();

	// Initialisation des effectifs par cellule
	nCellNumber = dataGridStats->ComputeTotalGridSize();
	ivPartIndexes.SetSize(dataGridStats->GetAttributeNumber());
	frequenciesRule->SetFrequencyNumber(nCellNumber);
	for (nCell = 0; nCell < nCellNumber; nCell++)
	{
		// Recherche des index des parties composant la cellule
		dataGridStats->ComputePartIndexes(nCell, &ivPartIndexes);

		// Specification de l'effectif de la cellule
		nCellFrequency = dataGridStats->GetCellFrequencyAt(&ivPartIndexes);
		frequenciesRule->SetFrequencyAt(nCell, nCellFrequency);
	}

	// Import des informations de granularite et d'effectifs initiaux et granularises par attribut de la grille
	if (bImportGranularityFeatures)
	{
		// Import de la granularite
		operand = new KWDerivationRuleOperand;
		operand->SetType(KWType::Continuous);
		operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		operand->SetContinuousConstant(dataGridStats->GetGranularity());
		AddOperand(operand);

		// Import du vecteur d'effectifs des valeurs initiales
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetType(KWType::Structure);
		operand->SetStructureName(frequenciesRule->GetStructureName());
		frequenciesRule = new KWDRFrequencies;
		operand->SetDerivationRule(frequenciesRule);
		AddOperand(operand);

		// Initialisation des effectifs initiaux par attributs
		frequenciesRule->DeleteAllOperands();
		frequenciesRule->SetFrequencyNumber(dataGridStats->GetAttributeNumber());
		for (nAttribute = 0; nAttribute < dataGridStats->GetAttributeNumber(); nAttribute++)
		{
			attributePartition = dataGridStats->GetAttributeAt(nAttribute);
			frequenciesRule->SetFrequencyAt(nAttribute, attributePartition->GetInitialValueNumber());
		}

		// Import du vecteur d'effectifs des valeurs granularisees
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetType(KWType::Structure);
		operand->SetStructureName(frequenciesRule->GetStructureName());
		frequenciesRule = new KWDRFrequencies;
		operand->SetDerivationRule(frequenciesRule);
		AddOperand(operand);

		// Initialisation des effectifs granularises par attributs
		frequenciesRule->DeleteAllOperands();
		frequenciesRule->SetFrequencyNumber(dataGridStats->GetAttributeNumber());
		for (nAttribute = 0; nAttribute < dataGridStats->GetAttributeNumber(); nAttribute++)
		{
			attributePartition = dataGridStats->GetAttributeAt(nAttribute);
			frequenciesRule->SetFrequencyAt(nAttribute, attributePartition->GetGranularizedValueNumber());
		}
	}
}

void KWDRDataGrid::ExportDataGridStats(KWDataGridStats* dataGridStats) const
{
	const KWDRIntervalBounds intervalBoundsRefRule;
	const KWDRValueGroups valueGroupsRefRule;
	const KWDRContinuousValueSet continuousValueSetRefRule;
	const KWDRSymbolValueSet symbolValueSetRefRule;
	const ALString sAttributePrefix = "Var";
	KWDerivationRuleOperand* operand;
	KWDRFrequencies* frequenciesRule;
	boolean bExportGranularityFeatures;
	int nGranularity;
	KWDRFrequencies* initialValueFrequenciesRule;
	KWDRFrequencies* granularizedValueFrequenciesRule;
	int nAttribute;
	KWDGSAttributePartition* attributePartition;
	IntVector ivPartIndexes;
	int nCellNumber;
	int nCell;
	int nCellFrequency;
	int nTotalFrequency;

	require(IsCompiled());
	require(dataGridStats != NULL);
	assert(GetOperandNumber() == GetAttributeNumber() + 1 or GetOperandNumber() == GetAttributeNumber() + 4);

	// Reinitialisation de la grille
	dataGridStats->DeleteAll();

	// Recherche des informations de granularite et d'effectifs initiaux et granularises par attribut de la grille
	bExportGranularityFeatures = false;
	nGranularity = 0;
	initialValueFrequenciesRule = NULL;
	granularizedValueFrequenciesRule = NULL;
	if (GetOperandNumber() == GetAttributeNumber() + 4)
	{
		bExportGranularityFeatures = true;

		// Export de la granularite
		operand = GetOperandAt(GetAttributeNumber() + 1);
		assert(operand->GetType() == KWType::Continuous);
		assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginConstant);
		nGranularity = int(floor(operand->GetContinuousConstant() + 0.5));

		// Recherche du vecteur d'effectifs des valeurs initiales
		operand = GetOperandAt(GetAttributeNumber() + 2);
		assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginRule);
		assert(operand->GetType() == KWType::Structure);
		assert(operand->GetDerivationRule() != NULL);
		initialValueFrequenciesRule = cast(KWDRFrequencies*, operand->GetDerivationRule());
		assert(initialValueFrequenciesRule->GetFrequencyNumber() == GetAttributeNumber());

		// Recherche du vecteur d'effectifs des valeurs granularisees
		operand = GetOperandAt(GetAttributeNumber() + 3);
		assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginRule);
		assert(operand->GetType() == KWType::Structure);
		assert(operand->GetDerivationRule() != NULL);
		granularizedValueFrequenciesRule = cast(KWDRFrequencies*, operand->GetDerivationRule());
		assert(granularizedValueFrequenciesRule->GetFrequencyNumber() == GetAttributeNumber());
	}

	// Acces a l'operande contenant les effectifs des cellules
	operand = GetOperandAt(GetAttributeNumber());
	assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginRule);
	assert(operand->GetType() == KWType::Structure);
	assert(operand->GetDerivationRule() != NULL);
	frequenciesRule = cast(KWDRFrequencies*, operand->GetDerivationRule());

	// Comptabilisation des effectifs totaux, ce qui sera potentiellement necessaire pour parametrer
	// les informations de granularite
	nTotalFrequency = 0;
	for (nCell = 0; nCell < frequenciesRule->GetFrequencyNumber(); nCell++)
	{
		// Specification de l'effectif de la cellule
		nCellFrequency = frequenciesRule->GetFrequencyAt(nCell);
		nTotalFrequency += nCellFrequency;
		assert(nCellFrequency >= 0);
		assert(nTotalFrequency >= 0);
	}

	// Specification par defaut de la granularite de la grille
	dataGridStats->SetGranularity(0);

	// Specification de la granularite de la grille
	if (bExportGranularityFeatures)
		dataGridStats->SetGranularity(nGranularity);

	// Creation des attributs de la grille a partir des operandes de la regle
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		operand = GetOperandAt(nAttribute);
		assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginRule);
		assert(operand->GetType() == KWType::Structure);
		assert(operand->GetDerivationRule() != NULL);

		// Creation d'un attribut de grille en fonction de type de l'operande
		// Cas d'une discretisation
		attributePartition = NULL;
		if (operand->GetStructureName() == intervalBoundsRefRule.GetStructureName())
		{
			const KWDRIntervalBounds* intervalBoundsRule;
			KWDGSAttributeDiscretization* attributeDiscretization;

			// Creation d'une partition de type intervalles
			attributeDiscretization = new KWDGSAttributeDiscretization;
			attributePartition = attributeDiscretization;

			// Parametrage des intervalles
			intervalBoundsRule = cast(const KWDRIntervalBounds*, operand->GetDerivationRule());
			intervalBoundsRule->ExportAttributeDiscretization(attributeDiscretization);

			// Specification par defaut des informations sur les nombre de valeurs
			attributePartition->SetInitialValueNumber(nTotalFrequency);
			attributePartition->SetGranularizedValueNumber(nTotalFrequency);
		}
		// Cas d'un ensemble de valeurs continues
		else if (operand->GetStructureName() == continuousValueSetRefRule.GetStructureName())
		{
			const KWDRContinuousValueSet* continuousValueSetRule;
			KWDGSAttributeContinuousValues* attributeContinuousValues;

			// Creation d'une partition de type ensemble de valeurs continues
			attributeContinuousValues = new KWDGSAttributeContinuousValues;
			attributePartition = attributeContinuousValues;

			// Parametrage des valeurs
			continuousValueSetRule = cast(const KWDRContinuousValueSet*, operand->GetDerivationRule());
			continuousValueSetRule->ExportAttributeContinuousValues(attributeContinuousValues);

			// Specification par defaut des informations sur les nombre de valeurs
			attributePartition->SetInitialValueNumber(nTotalFrequency);
			attributePartition->SetGranularizedValueNumber(nTotalFrequency);
		}
		// Cas d'un groupement de valeurs : Sympol ou VarPart
		else if (operand->GetStructureName() == valueGroupsRefRule.GetStructureName())
		{
			KWDRValueGroups* valueGroupsRule;
			KWDGSAttributeGrouping* attributeGrouping;

			// Creation d'une partition de type groupement de valeurs
			attributeGrouping = new KWDGSAttributeGrouping;
			attributePartition = attributeGrouping;

			// Parametrage des groupes et des valeurs
			valueGroupsRule = cast(KWDRValueGroups*, operand->GetDerivationRule());
			valueGroupsRule->ExportAttributeGrouping(attributeGrouping);

			// Specification par defaut des informations sur les nombre de valeurs
			attributePartition->SetInitialValueNumber(valueGroupsRule->GetTotalPartValueNumber());
			attributePartition->SetGranularizedValueNumber(valueGroupsRule->GetTotalPartValueNumber());
		}
		// Cas d'un ensemble de valeurs symboliques
		else if (operand->GetStructureName() == symbolValueSetRefRule.GetStructureName())
		{
			const KWDRSymbolValueSet* symbolValueSetRule;
			KWDGSAttributeSymbolValues* attributeSymbolValues;

			// Creation d'une partition de type ensemble de valeurs continues
			attributeSymbolValues = new KWDGSAttributeSymbolValues;
			attributePartition = attributeSymbolValues;

			// Parametrage des valeurs
			symbolValueSetRule = cast(const KWDRSymbolValueSet*, operand->GetDerivationRule());
			symbolValueSetRule->ExportAttributeSymbolValues(attributeSymbolValues);

			// Specification par defaut des informations sur les nombre de valeurs
			attributePartition->SetInitialValueNumber(symbolValueSetRule->GetValueNumber());
			attributePartition->SetGranularizedValueNumber(symbolValueSetRule->GetValueNumber());
		}
		assert(attributePartition != NULL);

		// Mise a jour specifiques du nombre de valeurs initiales et granularisees de l'attribut
		if (bExportGranularityFeatures)
		{
			attributePartition->SetInitialValueNumber(
			    initialValueFrequenciesRule->GetFrequencyAt(nAttribute));
			attributePartition->SetGranularizedValueNumber(
			    granularizedValueFrequenciesRule->GetFrequencyAt(nAttribute));
		}

		// Ajout de la partition dans la grille
		attributePartition->SetAttributeName(sAttributePrefix + IntToString(nAttribute + 1));
		dataGridStats->AddAttribute(attributePartition);
	}
	assert(dataGridStats->GetAttributeNumber() == GetAttributeNumber());

	// Creation des effectifs de la grille
	dataGridStats->CreateAllCells();

	// Initialisation des effectifs par cellule
	assert(frequenciesRule != NULL);
	nCellNumber = dataGridStats->ComputeTotalGridSize();
	assert(dataGridStats->ComputeTotalGridSize() == frequenciesRule->GetFrequencyNumber());
	ivPartIndexes.SetSize(dataGridStats->GetAttributeNumber());
	for (nCell = 0; nCell < nCellNumber; nCell++)
	{
		// Recherche des index des parties composant la cellule
		dataGridStats->ComputePartIndexes(nCell, &ivPartIndexes);

		// Specification de l'effectif de la cellule
		nCellFrequency = frequenciesRule->GetFrequencyAt(nCell);
		dataGridStats->SetCellFrequencyAt(&ivPartIndexes, nCellFrequency);
	}
	ensure(dataGridStats->Check());
}

int KWDRDataGrid::GetUncheckedAttributeNumber() const
{
	int nUncheckedAttributeNumber;
	KWDRFrequencies frequenciesRule;
	int nOperand;
	KWDerivationRuleOperand* operand;

	// Verification du type de structure de toutes les operandes
	nUncheckedAttributeNumber = 0;
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		operand = GetOperandAt(nOperand);

		// On arrete quand on a trouve le premier operande de type Frequencies
		if (operand->GetType() == KWType::Structure and
		    operand->GetStructureName() == frequenciesRule.GetStructureName())
		{
			nUncheckedAttributeNumber = nOperand;
			break;
		}
	}
	return nUncheckedAttributeNumber;
}

int KWDRDataGrid::ComputeUncheckedTotalFrequency() const
{
	KWDerivationRule* dataGridFrequenciesGenericRule;
	KWDRFrequencies* dataGridFrequenciesRule;

	require(GetOperandNumber() > 1);

	// Erreur si pas de regle de derivation dans l'operande destinee aux frequences (le dernier)
	dataGridFrequenciesGenericRule = GetOperandAt(GetOperandNumber() - 1)->GetDerivationRule();
	if (dataGridFrequenciesGenericRule == NULL)
		return -1;

	// Calcul de l'effectif total de la grille de reference
	dataGridFrequenciesRule = cast(KWDRFrequencies*, dataGridFrequenciesGenericRule);
	return dataGridFrequenciesRule->ComputeTotalFrequency();
}

KWDerivationRule* KWDRDataGrid::Create() const
{
	return new KWDRDataGrid;
}

void KWDRDataGrid::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Optimisation
		Optimize(kwcOwnerClass);
	}
}

Object* KWDRDataGrid::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	return (Object*)this;
}

boolean KWDRDataGrid::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	KWDRIntervalBounds intervalBoundsRule;
	KWDRContinuousValueSet continuousValueSetRule;
	KWDRValueGroups valueGroupsRule;
	KWDRSymbolValueSet symbolValueSetRule;
	KWDRFrequencies frequenciesRule;
	int nAttributeNumber;
	int nAttribute;
	int nOperand;
	KWDerivationRuleOperand* operand;
	KWDRUnivariatePartition* univariatePartition;
	KWDRFrequencies* frequencies;
	int nPartNumber;
	int nTotalCellNumber;
	double dTotalCellNumber;
	double dTotalFrequency;
	int nTotalFrequency;
	int nCell;
	int nGranularity;
	int nFrequency;
	int nValueNumber;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());

	// Verification d'un nombre d'operande minimal
	if (GetOperandNumber() < 2)
	{
		bOk = false;
		AddError("The number of operands should be at least 2");
	}

	// Calcul du nombre d'attribut de la grille
	nAttributeNumber = GetUncheckedAttributeNumber();

	// Verification du type de structure de toutes les operandes definissant la grille
	for (nOperand = 0; nOperand < nAttributeNumber + 1; nOperand++)
	{
		operand = GetOperandAt(nOperand);

		// Verification du type Structure
		if (operand->GetType() != KWType::Structure)
		{
			bOk = false;
			AddError(sTmp + "Operand " + IntToString(nOperand + 1) + " must be of type " +
				 KWType::ToString(KWType::Structure));
			break;
		}
		// Verification de la presence d'une regle de derivation
		else if (operand->GetDerivationRule() == NULL)
		{
			bOk = false;
			AddError(sTmp + "Operand " + IntToString(nOperand + 1) +
				 " must be produced by a derivation rule");
			break;
		}
	}

	// Verification du type de structure des operandes de partitionnement univarie
	if (bOk)
	{
		for (nOperand = 0; nOperand < nAttributeNumber; nOperand++)
		{
			operand = GetOperandAt(nOperand);
			assert(operand->GetDerivationRule() != NULL);

			// Verification d'une structure de partition univariee
			if (operand->GetStructureName() != intervalBoundsRule.GetName() and
			    operand->GetStructureName() != valueGroupsRule.GetName() and
			    operand->GetStructureName() != continuousValueSetRule.GetName() and
			    operand->GetStructureName() != symbolValueSetRule.GetName())
			{
				bOk = false;
				AddError(sTmp + "Incorrect structure(" + operand->GetStructureName() +
					 ") for operand " + IntToString(nOperand + 1) + " (must be " +
					 intervalBoundsRule.GetName() + " or " + valueGroupsRule.GetName() + " or " +
					 continuousValueSetRule.GetName() + " or " + symbolValueSetRule.GetName() +
					 ")");
				break;
			}
		}
	}

	// Verification de l'operande devant porter les effectifs de la grille
	dTotalFrequency = 0;
	nTotalFrequency = 0;
	if (bOk and GetOperandNumber() > nAttributeNumber)
	{
		operand = GetOperandAt(nAttributeNumber);
		assert(operand->GetDerivationRule() != NULL);

		// Verification d'une structure de type effectifs
		if (operand->GetStructureName() != frequenciesRule.GetName())
		{
			bOk = false;
			AddError(sTmp + "Incorrect structure(" + operand->GetStructureName() + ") for operand " +
				 IntToString(nAttributeNumber + 1) + "(must be " + frequenciesRule.GetName() + ")");
		}
		else
		{
			frequencies = cast(KWDRFrequencies*, operand->GetDerivationRule());

			// Calcul du nombre total de cellules a partir des partitions univariees
			nTotalCellNumber = 1;
			dTotalCellNumber = 1;
			for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
			{
				nPartNumber =
				    cast(KWDRUnivariatePartition*, GetOperandAt(nAttribute)->GetDerivationRule())
					->GetPartNumber();
				nTotalCellNumber *= nPartNumber;
				dTotalCellNumber *= nPartNumber;
			}

			// Test si on ne depasse pas la capacite des entiers
			if (dTotalCellNumber >= INT_MAX)
			{
				bOk = false;
				AddError(sTmp + "Total cell number too large (" + DoubleToString(dTotalCellNumber) +
					 ")");
			}

			// Test de coherence avec le nombre d'effectifs specifies
			if (bOk and nTotalCellNumber != frequencies->GetFrequencyNumber())
			{
				bOk = false;
				AddError(sTmp + "Size of the vector of frequencies (" +
					 IntToString(frequencies->GetFrequencyNumber()) +
					 ") does not match the size of the data grid (" +
					 IntToString(nTotalCellNumber) + ")");
			}

			// Comptage de l'effectif total
			if (bOk)
			{
				for (nCell = 0; nCell < frequencies->GetFrequencyNumber(); nCell++)
				{
					nTotalFrequency += frequencies->GetFrequencyAt(nCell);
					dTotalFrequency += frequencies->GetFrequencyAt(nCell);
				}

				// Erreur si depassement de capacite
				if (dTotalFrequency >= INT_MAX)
				{
					bOk = false;
					AddError(sTmp + "Total data grid frequency too large (" +
						 DoubleToString(dTotalFrequency) + ")");
				}
			}
		}
	}

	// Verification des operandes optionnels portant sur le cout de la grille
	if (bOk and GetOperandNumber() > nAttributeNumber + 1)
	{
		// Il doit y avoir exactement trois operandes en plus
		if (GetOperandNumber() != nAttributeNumber + 4)
		{
			bOk = false;
			AddError(sTmp + "Incorrect operand number (" + IntToString(GetOperandNumber()) + "): must be " +
				 IntToString(nAttributeNumber + 1) + " or " + IntToString(nAttributeNumber + 4));
		}
	}

	// Verification de l'operande de granularite
	if (bOk and GetOperandNumber() == nAttributeNumber + 4)
	{
		// Le premier operande doit etre la granularite de la grille
		operand = GetOperandAt(nAttributeNumber + 1);

		// L'operande doit etre de type Continuous
		if (operand->GetType() != KWType::Continuous)
		{
			bOk = false;
			AddError(sTmp + "Operand " + IntToString(nAttributeNumber + 2) + " must be of type " +
				 KWType::ToString(KWType::Continuous));
		}
		// L'operande doit etre constant
		else if (operand->GetOrigin() != KWDerivationRuleOperand::OriginConstant)
		{
			bOk = false;
			AddError(sTmp + "Operand " + IntToString(nAttributeNumber + 2) + " must be a constant");
		}
		// Verification de la granularite
		else
		{
			nGranularity = (int)floor(operand->GetContinuousConstant() + 0.5);
			if (nGranularity < 0)
			{
				bOk = false;
				AddError(sTmp + "Operand " + IntToString(nAttributeNumber + 2) +
					 " must be an integer greater than 0");
			}
			else if (nGranularity > (int)ceil(log(nTotalFrequency * 1.0) / log(2.0)))
			{
				bOk = false;
				AddError(sTmp + "Operand " + IntToString(nAttributeNumber + 2) +
					 " must be an integer lower than ceil(log2(N))");
			}
		}
	}

	// Verification des operandes de nombres de valeurs initiales et granularises
	if (bOk and GetOperandNumber() == nAttributeNumber + 4)
	{
		// Les deux derniers operandes correspondant aux nombres de valeurs initiales et granularisees
		for (nOperand = nAttributeNumber + 2; nOperand < nAttributeNumber + 4; nOperand++)
		{
			operand = GetOperandAt(nOperand);

			// Verification du type Structure
			if (operand->GetType() != KWType::Structure)
			{
				bOk = false;
				AddError(sTmp + "Operand " + IntToString(nOperand + 1) + " must be of type " +
					 KWType::ToString(KWType::Structure));
				break;
			}
			// Verification de la presence d'une regle de derivation
			else if (operand->GetDerivationRule() == NULL)
			{
				bOk = false;
				AddError(sTmp + "Operand " + IntToString(nOperand + 1) +
					 " must be produced by a derivation rule");
				break;
			}
			// Verification d'une structure de type effectifs
			else if (operand->GetStructureName() != frequenciesRule.GetName())
			{
				bOk = false;
				AddError(sTmp + "Incorrect structure(" + operand->GetStructureName() +
					 ") for operand " + IntToString(nOperand + 1) + "(must be " +
					 frequenciesRule.GetName() + ")");
			}
			// Verification de la taille du vecteur d'effectif
			else
			{
				frequencies = cast(KWDRFrequencies*, operand->GetDerivationRule());

				// Test de coherence avec le nombre d'attributs de la grille
				if (bOk and nAttributeNumber != frequencies->GetFrequencyNumber())
				{
					bOk = false;
					AddError(sTmp + "Size of the vector of frequencies (" +
						 IntToString(frequencies->GetFrequencyNumber()) + ") in operand " +
						 IntToString(nOperand + 1) +
						 " does not match the number of variables of the data grid(" +
						 IntToString(nAttributeNumber) + ")");
				}

				if (bOk)
				{
					// Parcours des effectif par partie
					for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
					{
						nFrequency = frequencies->GetFrequencyAt(nAttribute);

						// Acces a la partition univariee de l'attribut
						univariatePartition =
						    cast(KWDRUnivariatePartition*,
							 GetOperandAt(nAttribute)->GetDerivationRule());

						// Traitement de chaque type de partition
						nValueNumber = 0;
						if (univariatePartition->GetStructureName() !=
						    intervalBoundsRule.GetName())
							nValueNumber = nTotalFrequency;
						else if (univariatePartition->GetStructureName() !=
							 continuousValueSetRule.GetName())
							nValueNumber = nTotalFrequency;
						else if (univariatePartition->GetStructureName() !=
							 valueGroupsRule.GetName())
							nValueNumber = cast(KWDRValueGroups*, univariatePartition)
									   ->ComputeTotalPartValueNumber();
						else if (univariatePartition->GetStructureName() !=
							 symbolValueSetRule.GetName())
							nValueNumber = cast(KWDRSymbolValueSet*, univariatePartition)
									   ->GetValueNumber();

						// Verification de la coherence entre le nombre de valeurs initiales ou
						// granularisees specifie (nFrequency) et le nombre de valeurs provenant
						// de l'attribut (nValueNumber)
						if (nFrequency <= 0)
						{
							bOk = false;
							AddError(sTmp + "Frequency (" + IntToString(nFrequency) +
								 ") specified in the frequency vector of operand " +
								 IntToString(nOperand + 1) + " for variable " +
								 IntToString(nAttribute + 1) +
								 " of the data grid  must be greater than 0");
						}
						else if (nFrequency > nValueNumber)
						{
							bOk = false;
							AddError(sTmp + "Frequency (" + IntToString(nFrequency) +
								 ") specified in the frequency vector of operand " +
								 IntToString(nOperand + 1) + " for variable " +
								 IntToString(nAttribute + 1) +
								 " of the data grid  must be less than " +
								 IntToString(nValueNumber));
						}
						else if (nFrequency > nTotalFrequency)
						{
							bOk = false;
							AddError(sTmp + "Frequency (" + IntToString(nFrequency) +
								 ") specified in the frequency vector of operand " +
								 IntToString(nOperand + 1) + " for variable " +
								 IntToString(nAttribute + 1) +
								 " of the data grid  must be less than " +
								 IntToString(nTotalFrequency));
						}
					}
				}
			}
		}
	}
	return bOk;
}

boolean KWDRDataGrid::SilentCheckUnivariatePartitionOperand(const KWDerivationRuleOperand* operand) const
{
	boolean bOk;
	KWDRIntervalBounds intervalBoundsRule;
	KWDRContinuousValueSet continuousValueSetRule;
	KWDRValueGroups valueGroupsRule;
	KWDRSymbolValueSet symbolValueSetRule;

	require(operand != NULL);

	bOk = operand->GetType() == KWType::Structure and operand->GetDerivationRule() != NULL and
	      (operand->GetStructureName() != valueGroupsRule.GetName() or
	       operand->GetStructureName() != continuousValueSetRule.GetName() or
	       operand->GetStructureName() != symbolValueSetRule.GetName() or
	       operand->GetStructureName() != symbolValueSetRule.GetName());
	return bOk;
}

void KWDRDataGrid::Optimize(KWClass* kwcOwnerClass)
{
	require(IsCompiled());

	// Calcul du nombre d'attributs de la grille
	nDataGridAttributeNumber = GetUncheckedAttributeNumber();
	ensure(nDataGridAttributeNumber > 0);
}

boolean KWDRDataGrid::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

///////////////////////////////////////////////////////////////
// Classe KWDRFrequencies

KWDRFrequencies::KWDRFrequencies()
{
	SetName("Frequencies");
	SetLabel("Frequencies");
	SetStructureName("Frequencies");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRFrequencies::~KWDRFrequencies() {}

void KWDRFrequencies::SetFrequencyNumber(int nFrequency)
{
	require(nFrequency >= 0);

	ivFrequencies.SetSize(nFrequency);
	bStructureInterface = true;
	nFreshness++;
}

int KWDRFrequencies::GetFrequencyNumber() const
{
	if (bStructureInterface)
		return ivFrequencies.GetSize();
	else
		return GetOperandNumber();
}

void KWDRFrequencies::SetFrequencyAt(int nIndex, int nFrequency)
{
	require(0 <= nIndex and nIndex < ivFrequencies.GetSize());
	require(nFrequency >= 0);
	require(bStructureInterface);

	ivFrequencies.SetAt(nIndex, nFrequency);
	nFreshness++;
}

int KWDRFrequencies::GetFrequencyAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < ivFrequencies.GetSize());
	require(bStructureInterface);

	return ivFrequencies.GetAt(nIndex);
}

int KWDRFrequencies::ComputeTotalFrequency() const
{
	int nTotalFrequency = 0;
	int nIndex;

	require(CheckOperandsDefinition());

	// Calcul de l'effectif total en interface de structure ou non
	if (bStructureInterface)
	{
		for (nIndex = 0; nIndex < ivFrequencies.GetSize(); nIndex++)
			nTotalFrequency += ivFrequencies.GetAt(nIndex);
	}
	else
	{
		for (nIndex = 0; nIndex < GetOperandNumber(); nIndex++)
			nTotalFrequency += (int)floor(GetOperandAt(nIndex)->GetContinuousConstant() + 0.5);
	}

	return nTotalFrequency;
}

KWDerivationRule* KWDRFrequencies::Create() const
{
	return new KWDRFrequencies;
}

void KWDRFrequencies::CleanCompiledBaseInterface()
{
	DeleteAllOperands();
}

boolean KWDRFrequencies::CheckOperandsDefinition() const
{
	boolean bOk;
	int nOperand;
	KWDerivationRuleOperand* operand;
	Continuous cValue;
	int nValue;
	ALString sTmp;

	// Verification de base
	bOk = KWDerivationRule::CheckOperandsDefinition();

	// Verification de la valeur entiere positive des operandes
	if (bOk)
	{
		// Parcours des index
		for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		{
			operand = GetOperandAt(nOperand);
			assert(operand->GetOrigin() == KWDerivationRuleOperand::OriginConstant);

			// Verification de la valeur entiere de l'operande
			cValue = operand->GetContinuousConstant();
			bOk = KWContinuous::ContinuousToInt(cValue, nValue);
			if (not bOk)
			{
				assert(bOk == false);
				AddError(sTmp + "Operand " + IntToString(nOperand + 1) + " (" +
					 KWContinuous::ContinuousToString(cValue) + ")" +
					 ": operand must be an integer");
			}
			else if (nValue < 0)
			{
				bOk = false;
				AddError(sTmp + "Operand " + IntToString(nOperand + 1) + " (" +
					 KWContinuous::ContinuousToString(cValue) + ")" +
					 ": operand must be non negative");
			}

			// Arret si erreur
			if (not bOk)
				break;
		}
	}
	return bOk;
}

void KWDRFrequencies::CopyStructureFrom(const KWDerivationRule* kwdrSource)
{
	const KWDRFrequencies* kwdrfSource = cast(KWDRFrequencies*, kwdrSource);

	// Copie de la version optimisee du parametrage des valeurs
	ivFrequencies.CopyFrom(&kwdrfSource->ivFrequencies);
}

void KWDRFrequencies::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	int i;
	KWDerivationRuleOperand* operand;
	Continuous cValue;
	int nFrequency;

	require(kwdrSource->KWDerivationRule::CheckDefinition());

	// Recopie des operandes
	ivFrequencies.SetSize(kwdrSource->GetOperandNumber());
	for (i = 0; i < kwdrSource->GetOperandNumber(); i++)
	{
		operand = kwdrSource->GetOperandAt(i);
		cValue = operand->GetContinuousConstant();

		// On se recale sur l'entier le plus proche
		nFrequency = (int)floor(cValue + 0.5);
		ivFrequencies.SetAt(i, nFrequency);
	}
	bStructureInterface = true;
}

void KWDRFrequencies::WriteStructureUsedRule(ostream& ost) const
{
	int i;

	// Nom de la regle utilisee
	ost << KWClass::GetExternalName(GetName());

	// Operandes
	ost << "(";
	for (i = 0; i < ivFrequencies.GetSize(); i++)
	{
		if (i > 0)
			ost << ", ";
		ost << ivFrequencies.GetAt(i);
	}
	ost << ")";
}

int KWDRFrequencies::FullCompare(const KWDerivationRule* rule) const
{
	int nDiff;
	KWDRFrequencies* ruleFrequencies;
	int i;

	require(rule != NULL);

	// Comparaison sur la classe sur laquelle la regle est applicable
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison sur le nom de la regle
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// En cas d'egalite, comparaison en utilisant l'interface de structure
	if (nDiff == 0)
	{
		ruleFrequencies = cast(KWDRFrequencies*, rule);
		assert(GetStructureInterface());
		assert(ruleFrequencies->GetStructureInterface());

		// Taille du vecteur
		nDiff = ivFrequencies.GetSize() - ruleFrequencies->ivFrequencies.GetSize();

		// Si egalite, comparaison sur les valeurs
		if (nDiff == 0)
		{
			for (i = 0; i < ivFrequencies.GetSize(); i++)
			{
				nDiff = ivFrequencies.GetAt(i) - ruleFrequencies->ivFrequencies.GetAt(i);
				if (nDiff != 0)
					break;
			}
		}
	}

	return nDiff;
}

longint KWDRFrequencies::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRStructureRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRFrequencies) - sizeof(KWDRStructureRule);
	lUsedMemory += ivFrequencies.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridRule

KWDRDataGridRule::KWDRDataGridRule()
{
	SetName("DataGridRule");
	SetLabel("DataGrid rule");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("DataGrid");

	// A partir du deuxieme operande, le type est guide par les attributs de la grille
	// On doit neanmoins preciser un type valide pour la regle generique
	GetSecondOperand()->SetType(KWType::Continuous);

	// Index de la partie source, calcule pour le dernier objet
	nCellIndex = -1;

	// Indication de valeur manquante pour le dernier objet
	bIsMissingValue = false;

	// Gestion de la compilation dynamique
	dataGridRule = NULL;
	nOptimizationFreshness = 0;
}

KWDRDataGridRule::~KWDRDataGridRule() {}

void KWDRDataGridRule::ComputeCellIndex(const KWObject* kwoObject) const
{
	int nOperand;
	int nAttributeIndex;
	int nPartIndex;
	int nFactor;
	Continuous cValue;
	Symbol sValue;

	require(Check());
	require(IsOptimized());

	// Calcul de l'index de la cellule
	nCellIndex = 0;
	bIsMissingValue = false;
	nFactor = 1;
	for (nOperand = 1; nOperand < GetOperandNumber(); nOperand++)
	{
		nAttributeIndex = nOperand - 1;

		// Recherche de l'index de la partie selon son type, et mise a jour de l'indication de valeur manquante
		if (dataGridRule->GetAttributeTypeAt(nAttributeIndex) == KWType::Continuous)
		{
			assert(GetOperandAt(nOperand)->GetType() == KWType::Continuous);
			cValue = GetOperandAt(nOperand)->GetContinuousValue(kwoObject);
			bIsMissingValue = bIsMissingValue or cValue == KWContinuous::GetMissingValue();
			nPartIndex = dataGridRule->GetContinuousAttributePartIndexAt(nAttributeIndex, cValue);
		}
		else
		{
			assert(dataGridRule->GetAttributeTypeAt(nAttributeIndex) == KWType::Symbol);
			assert(GetOperandAt(nOperand)->GetType() == KWType::Symbol);
			sValue = GetOperandAt(nOperand)->GetSymbolValue(kwoObject);
			bIsMissingValue = bIsMissingValue or sValue.IsEmpty();
			nPartIndex = dataGridRule->GetSymbolAttributePartIndexAt(nAttributeIndex, sValue);
		}

		// Calcul de l'index de cellule
		nCellIndex += nFactor * nPartIndex;
		nFactor *= dataGridRule->GetAttributePartNumberAt(nAttributeIndex);
	}
}

const ALString KWDRDataGridRule::ComputeCellLabel(const KWObject* kwoObject) const
{
	ALString sCellLabel;
	int nOperand;
	int nAttributeIndex;
	int nPartIndex;
	int nFactor;
	Continuous cValue;
	Symbol sValue;

	require(Check());
	require(IsOptimized());

	// Calcul de l'index de la cellule
	nCellIndex = 0;
	bIsMissingValue = false;
	nFactor = 1;
	for (nOperand = 1; nOperand < GetOperandNumber(); nOperand++)
	{
		nAttributeIndex = nOperand - 1;

		// Recherche de l'index de la partie selon son type
		if (dataGridRule->GetAttributeTypeAt(nAttributeIndex) == KWType::Continuous)
		{
			assert(GetOperandAt(nOperand)->GetType() == KWType::Continuous);
			cValue = GetOperandAt(nOperand)->GetContinuousValue(kwoObject);
			bIsMissingValue = bIsMissingValue or cValue == KWContinuous::GetMissingValue();
			nPartIndex = dataGridRule->GetContinuousAttributePartIndexAt(nAttributeIndex, cValue);
		}
		else
		{
			assert(dataGridRule->GetAttributeTypeAt(nAttributeIndex) == KWType::Symbol);
			assert(GetOperandAt(nOperand)->GetType() == KWType::Symbol);
			sValue = GetOperandAt(nOperand)->GetSymbolValue(kwoObject);
			bIsMissingValue = bIsMissingValue or sValue.IsEmpty();
			nPartIndex = dataGridRule->GetSymbolAttributePartIndexAt(nAttributeIndex, sValue);
		}

		// Prise en compte du livelle de la partie
		if (nOperand > 1)
			sCellLabel += " x ";
		sCellLabel +=
		    cast(KWDRUnivariatePartition*, dataGridRule->GetOperandAt(nAttributeIndex)->GetDerivationRule())
			->GetPartLabelAt(nPartIndex);
	}
	return sCellLabel;
}

boolean KWDRDataGridRule::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());
	require(ruleFamily->GetOperandNumber() > 0);
	require(ruleFamily->GetFirstOperand()->GetType() == KWType::Structure);
	require(ruleFamily->GetFirstOperand()->GetStructureName() == "DataGrid");

	// Il faut au moins un operande
	if (GetOperandNumber() == 0)
	{
		AddError("Missing operand");
		bOk = false;
	}
	// Verification du premier operande: grille de donnees
	else
	{
		operand = GetOperandAt(0);
		if (not operand->CheckFamily(ruleFamily->GetOperandAt(0)))
		{
			AddError(sTmp + "Type of first operand" +
				 " inconsistent with that of the registered rule (Structure(DataGrid))");
			bOk = false;
		}
	}

	// Le premier operande est deja verifie en precondition de la methode
	// Les autres operandes doivent etre de type Continuous ou Symbol
	if (bOk)
	{
		for (i = 1; i < GetOperandNumber(); i++)
		{
			operand = GetOperandAt(i);

			// Test de type simple
			if (not KWType::IsSimple(operand->GetType()))
			{
				AddError(sTmp + "Type of operand " + IntToString(i + 1) +
					 " inconsistent with that of the registered rule");
				bOk = false;
				break;
			}
		}
	}
	return bOk;
}

boolean KWDRDataGridRule::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	int nDataGridAttributeNumber;
	const KWDRDataGrid refDataGrid;
	KWDRDataGrid* referencedDataGrid;
	KWDRUnivariatePartition* univariatePartitionRule;
	int nDataGridAttributeType;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	require(GetOperandNumber() > 0);
	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification que le premier operande reference une regle de type grille
	if (bOk)
		bOk = CheckReferencedDerivationRuleAt(0, kwcOwnerClass, refDataGrid.GetName());
	if (bOk)
	{
		referencedDataGrid = cast(KWDRDataGrid*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));

		// Calcul du nombre d'attributs de la grille
		// La methode GetAttributeNumber n'est pas encore disponible (il faut une compilation)
		nDataGridAttributeNumber = referencedDataGrid->GetOperandNumber() - 1;

		// Les autres operandes doivent etre compatibles avec la grille
		if (GetOperandNumber() - 1 > nDataGridAttributeNumber)
		{
			AddError(sTmp +
				 "Too many operands with respect to the number of variables of the data grid of the "
				 "first operand (" +
				 IntToString(nDataGridAttributeNumber) + " variables)");
			bOk = false;
		}
		else
		{
			// Verification du type des operandes
			for (i = 1; i < GetOperandNumber(); i++)
			{
				operand = GetOperandAt(i);

				// Recherche du type de l'attribut de la grille
				// La methode GetAttributeType n'est pas encore disponible (il faut une compilation)
				assert(referencedDataGrid->CheckReferencedDerivationRuleAt(i - 1, kwcOwnerClass, ""));
				univariatePartitionRule =
				    cast(KWDRUnivariatePartition*,
					 referencedDataGrid->GetOperandAt(i - 1)->GetReferencedDerivationRule(
					     kwcOwnerClass));
				check(univariatePartitionRule);
				nDataGridAttributeType = univariatePartitionRule->GetAttributeType();

				// Test de validite du type
				if (operand->GetType() != nDataGridAttributeType)
				{
					AddError(sTmp + "Type " + KWType::ToString(operand->GetType()) +
						 " of operand " + IntToString(i + 1) + " inconsistent with type " +
						 KWType::ToString(nDataGridAttributeType) + " of dimension " +
						 IntToString(i) + " in the related data grid");
					bOk = false;
					break;
				}
			}
		}
	}
	return bOk;
}

boolean KWDRDataGridRule::CheckPredictorCompleteness(int nPredictorType, const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	int nDataGridAttributeNumber;
	KWDRDataGrid refDataGrid;
	KWDRDataGrid* referencedDataGrid;
	int nDataGridLastAttributeType;
	ALString sTmp;

	require(KWType::IsSimple(nPredictorType));
	require(kwcOwnerClass != NULL);

	// Test si la premiere regle existe et est du bon type, arret si erreur
	bOk = CheckReferencedDerivationRuleAt(0, kwcOwnerClass, refDataGrid.GetName());
	if (not bOk)
		return bOk;

	// Acces a la regle de type grille
	referencedDataGrid = cast(KWDRDataGrid*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
	check(referencedDataGrid);

	// Calcul du nombre d'attribut de la grille
	// La methode GetAttributeNumber n'est pas encore disponible (il faut une compilation)
	nDataGridAttributeNumber = referencedDataGrid->GetOperandNumber() - 1;

	// Il doit y avoir au moins un attribut
	if (nDataGridAttributeNumber == 0)
	{
		AddError(sTmp + "The data grid specified in the first operand has no variable");
		bOk = false;
	}

	// Le nombre d'attributs en operandes doit etre egale a la dimension (moins un) de la grille
	if (GetOperandNumber() - 1 != nDataGridAttributeNumber - 1)
	{
		AddError(sTmp + "The number of input variables (" + IntToString(GetOperandNumber() - 1) +
			 ") does not match the input size of the data grid (" +
			 IntToString(nDataGridAttributeNumber - 1) + ")");
		bOk = false;
	}

	// Verification du type du dernier operande
	if (bOk)
	{
		// Recherche du type du dernier attribut de la grille
		nDataGridLastAttributeType =
		    referencedDataGrid->GetUncheckedAttributeTypeAt(nDataGridAttributeNumber - 1);

		// Test de consistence de ce type
		if (nDataGridLastAttributeType != nPredictorType)
		{
			if (nPredictorType == KWType::Symbol)
				AddError(
				    sTmp +
				    "Wrong type of the last variable of the referenced data grid for classification");
			else
				AddError(sTmp +
					 "Wrong type of the last variable of the referenced data grid for regression");
			bOk = false;
		}
	}
	return bOk;
}

boolean KWDRDataGridRule::CheckTargetAttributeCompletness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	int nDataGridAttributeNumber;
	KWDRDataGrid* referencedDataGrid;
	KWDRDataGrid refDataGrid;
	ALString sTmp;

	require(kwcOwnerClass != NULL);

	// Test si la premiere regle existe et est du bon type, arret si erreur
	bOk = CheckReferencedDerivationRuleAt(0, kwcOwnerClass, refDataGrid.GetName());
	if (not bOk)
		return bOk;

	// Acces a la regle de type grille
	referencedDataGrid = cast(KWDRDataGrid*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
	check(referencedDataGrid);

	// Calcul du nombre d'attribut de la grille
	// La methode GetAttributeNumber n'est pas encore disponible (il faut une compilation)
	nDataGridAttributeNumber = referencedDataGrid->GetOperandNumber() - 1;

	// Il doit y avoir exactement un attribut
	if (nDataGridAttributeNumber != 1)
	{
		AddError(sTmp + "The data grid specified in the first operand should be univariate");
		bOk = false;
	}

	// Le nombre d'attributs en operandes doit etre egale a 1
	if (GetOperandNumber() - 1 != 1)
	{
		AddError(sTmp + "The number of input variables (" + IntToString(GetOperandNumber() - 1) +
			 ") should be equal to 1");
		bOk = false;
	}
	return bOk;
}

void KWDRDataGridRule::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation si necessaire, en comparant a la fraicheur de la classe entiere
	if (nOptimizationFreshness < kwcOwnerClass->GetCompileFreshness())
	{
		// Memorisation de la fraicheur
		nOptimizationFreshness = kwcOwnerClass->GetCompileFreshness();

		// Optimisation
		Optimize(kwcOwnerClass);
	}
}

longint KWDRDataGridRule::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRDataGridRule) - sizeof(KWDerivationRule);
	return lUsedMemory;
}

void KWDRDataGridRule::Optimize(KWClass* kwcOwnerClass)
{
	require(IsCompiled());
	dataGridRule = cast(KWDRDataGrid*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
	check(dataGridRule);
}

boolean KWDRDataGridRule::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

///////////////////////////////////////////////////////////////
// Classe KWDRCellIndex

KWDRCellIndex::KWDRCellIndex()
{
	SetName("CellIndex");
	SetLabel("Cell index of an instance");
	SetType(KWType::Continuous);
}

KWDRCellIndex::~KWDRCellIndex() {}

KWDerivationRule* KWDRCellIndex::Create() const
{
	return new KWDRCellIndex;
}

Continuous KWDRCellIndex::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsOptimized());

	// Calcul et memorisation de l'index de cellule
	ComputeCellIndex(kwoObject);

	// On renvoie l'index de la cellule
	return (Continuous)(nCellIndex + 1);
}

///////////////////////////////////////////////////////////////
// Classe KWDRCellIndexWithMissing

KWDRCellIndexWithMissing::KWDRCellIndexWithMissing()
{
	SetName("CellIndexWithMissing");
	SetLabel("Cell index of an instance or missing value index");
	SetType(KWType::Continuous);
}

KWDRCellIndexWithMissing::~KWDRCellIndexWithMissing() {}

KWDerivationRule* KWDRCellIndexWithMissing::Create() const
{
	return new KWDRCellIndexWithMissing;
}

Continuous KWDRCellIndexWithMissing::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsOptimized());

	// Calcul et memorisation de l'index de cellule
	ComputeCellIndex(kwoObject);

	// On renvoie -1 en cas de valeur manquante ou l'index au cas echeant
	if (bIsMissingValue)
		return -1;
	else
		return (Continuous)(nCellIndex + 1);
}

///////////////////////////////////////////////////////////////
// Classe KWDRCellLabel

KWDRCellLabel::KWDRCellLabel()
{
	SetName("CellLabel");
	SetLabel("Cell label of an instance");
	SetType(KWType::Symbol);
}

KWDRCellLabel::~KWDRCellLabel() {}

KWDerivationRule* KWDRCellLabel::Create() const
{
	return new KWDRCellLabel;
}

Symbol KWDRCellLabel::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ALString sCellLabel;

	require(Check());
	require(IsOptimized());

	// Calcul du libelle de de cellule
	sCellLabel = ComputeCellLabel(kwoObject);
	return (Symbol)sCellLabel;
}

///////////////////////////////////////////////////////////////
// Classe KWDRCellId

KWDRCellId::KWDRCellId()
{
	SetName("CellId");
	SetLabel("Cell Id of an instance");
	SetType(KWType::Symbol);
}

KWDRCellId::~KWDRCellId() {}

KWDerivationRule* KWDRCellId::Create() const
{
	return new KWDRCellId;
}

Symbol KWDRCellId::ComputeSymbolResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsOptimized());

	// Calcul et memorisation de l'index de cellule
	ComputeCellIndex(kwoObject);

	// On renvoie l'identifiant de la cellule
	return svCellIds.GetAt(nCellIndex);
}

longint KWDRCellId::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRDataGridRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRCellId) - sizeof(KWDRDataGridRule);
	lUsedMemory += svCellIds.GetUsedMemory();
	return lUsedMemory;
}

void KWDRCellId::Optimize(KWClass* kwcOwnerClass)
{
	int nIndex;

	require(IsCompiled());

	// Appel de la methode ancetre
	KWDRDataGridRule::Optimize(kwcOwnerClass);

	// Calcul des Ids des cellules
	svCellIds.SetSize(dataGridRule->GetTotalCellNumber());
	for (nIndex = 0; nIndex < svCellIds.GetSize(); nIndex++)
		svCellIds.SetAt(nIndex, KWDRUnivariatePartition::ComputeId(nIndex));
}

///////////////////////////////////////////////////////////////
// Classe KWDRValueIndex

KWDRValueIndex::KWDRValueIndex()
{
	SetName("ValueIndexDG");
	SetLabel("Value index of an instance");
	SetType(KWType::Continuous);
}

KWDRValueIndex::~KWDRValueIndex() {}

KWDerivationRule* KWDRValueIndex::Create() const
{
	return new KWDRValueIndex;
}

boolean KWDRValueIndex::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification que l'on a une regle de type statistique univariee
	bOk = bOk and CheckTargetAttributeCompletness(kwcOwnerClass);
	return bOk;
}

Continuous KWDRValueIndex::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	// Calcul et memorisation de l'index de cellule
	ComputeCellIndex(kwoObject);

	// On renvoie le rang de la valeur
	return cvValueIndexes.GetAt(nCellIndex);
}

longint KWDRValueIndex::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRDataGridRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRValueIndex) - sizeof(KWDRDataGridRule);
	lUsedMemory += cvValueIndexes.GetUsedMemory();
	return lUsedMemory;
}

void KWDRValueIndex::Optimize(KWClass* kwcOwnerClass)
{
	KWDRFrequencies* frequencyRule;
	int nAttributeType;
	int nPartNumber;
	int nIndex;
	int nFrequency;
	int nTotalFrequency;

	require(IsCompiled());

	// Appel de la methode ancetre
	KWDRDataGridRule::Optimize(kwcOwnerClass);
	assert(dataGridRule == cast(KWDRDataGrid*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass)));
	assert(dataGridRule->GetOperandNumber() == 2);

	// Recherche du type de l'attribut de la grille
	nAttributeType = dataGridRule->GetUncheckedAttributeTypeAt(0);
	assert(KWType::IsSimple(nAttributeType));

	// Acces aux effectif par valeur
	frequencyRule =
	    cast(KWDRFrequencies*, dataGridRule->GetOperandAt(1)->GetReferencedDerivationRule(kwcOwnerClass));
	check(frequencyRule);

	// Initialisation de la taille vecteur de rang
	nPartNumber = dataGridRule->GetUncheckedAttributePartNumberAt(0);
	assert(frequencyRule->GetFrequencyNumber() == nPartNumber);
	cvValueIndexes.SetSize(nPartNumber);

	// Dans le cas symbolique, le rang est egal a l'index "externe" de la valeur
	if (nAttributeType == KWType::Symbol)
	{
		for (nIndex = 0; nIndex < cvValueIndexes.GetSize(); nIndex++)
			cvValueIndexes.SetAt(nIndex, (Continuous)(nIndex + 1));
	}
	// Dans le cas continuous, le rang est egal a l'index moyen de la valeur
	else if (nAttributeType == KWType::Continuous)
	{
		nTotalFrequency = 0;
		for (nIndex = 0; nIndex < cvValueIndexes.GetSize(); nIndex++)
		{
			nFrequency = frequencyRule->GetFrequencyAt(nIndex);
			cvValueIndexes.SetAt(nIndex, (Continuous)(nTotalFrequency + nFrequency / 2 + 1));
			nTotalFrequency += nFrequency;
		}
	}
}

///////////////////////////////////////////////////////////////
// Classe KWDRValueRank

KWDRValueRank::KWDRValueRank()
{
	SetName("ValueRank");
	SetLabel("Value rank of an instance");
	SetType(KWType::Continuous);
}

KWDRValueRank::~KWDRValueRank() {}

KWDerivationRule* KWDRValueRank::Create() const
{
	return new KWDRValueRank;
}

boolean KWDRValueRank::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification que l'on a une regle de type statistique univariee
	bOk = bOk and CheckTargetAttributeCompletness(kwcOwnerClass);

	// Verification que le deuxieme operande est de type Continuous
	if (bOk)
	{
		assert(GetOperandNumber() == 2);
		if (GetSecondOperand()->GetType() != KWType::Continuous)
		{
			AddError("Type of second operand should be Numerical");
			bOk = false;
		}
	}
	return bOk;
}

Continuous KWDRValueRank::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	// Calcul et memorisation de l'index de cellule
	ComputeCellIndex(kwoObject);

	// On renvoie le rang de la valeur
	return cvValueRanks.GetAt(nCellIndex);
}

longint KWDRValueRank::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRDataGridRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRValueRank) - sizeof(KWDRDataGridRule);
	lUsedMemory += cvValueRanks.GetUsedMemory();
	return lUsedMemory;
}

void KWDRValueRank::Optimize(KWClass* kwcOwnerClass)
{
	KWDRFrequencies* frequencyRule;
	int nIndex;
	int nFrequency;
	int nCumulatedFrequency;
	Continuous cTotalFrequency;
	KWDRUnivariatePartition* univariatePartitionRule;

	require(IsCompiled());

	// Appel de la methode ancetre
	KWDRDataGridRule::Optimize(kwcOwnerClass);
	assert(dataGridRule == cast(KWDRDataGrid*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass)));
	assert(dataGridRule->GetOperandNumber() == 2);

	// Memorisation de la partition univariee associee a la grille
	univariatePartitionRule =
	    cast(KWDRUnivariatePartition*, dataGridRule->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
	check(univariatePartitionRule);

	// Acces aux effectif par valeur
	frequencyRule =
	    cast(KWDRFrequencies*, dataGridRule->GetOperandAt(1)->GetReferencedDerivationRule(kwcOwnerClass));
	check(frequencyRule);

	// Initialisation de la taille vecteur de rang
	assert(frequencyRule->GetFrequencyNumber() == univariatePartitionRule->GetPartNumber());
	cvValueRanks.SetSize(univariatePartitionRule->GetPartNumber());

	// Calcul de l'effectif total
	cTotalFrequency = (Continuous)frequencyRule->ComputeTotalFrequency();
	if (cTotalFrequency == 0)
		cTotalFrequency = 1;

	// Le rang est egal a l'index moyen de la valeur, normalise par l'effectif total
	nCumulatedFrequency = 0;
	for (nIndex = 0; nIndex < cvValueRanks.GetSize(); nIndex++)
	{
		nFrequency = frequencyRule->GetFrequencyAt(nIndex);
		cvValueRanks.SetAt(nIndex, (Continuous)((nCumulatedFrequency + nFrequency / 2.0) / cTotalFrequency));
		nCumulatedFrequency += nFrequency;
	}
}

///////////////////////////////////////////////////////////////
// Classe KWDRValueRankSelfDistance

KWDRValueRankSelfDistance::KWDRValueRankSelfDistance()
{
	SetName("ValueRankSelfDistance");
	SetLabel("Self-distance of an instance");
	SetType(KWType::Continuous);
}

KWDRValueRankSelfDistance::~KWDRValueRankSelfDistance() {}

KWDerivationRule* KWDRValueRankSelfDistance::Create() const
{
	return new KWDRValueRankSelfDistance;
}

Continuous KWDRValueRankSelfDistance::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	// Calcul et memorisation de l'index de cellule
	ComputeCellIndex(kwoObject);

	// On renvoie le rang de la valeur
	return cvValueRankSelfDistances.GetAt(nCellIndex);
}

longint KWDRValueRankSelfDistance::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRValueRank::GetUsedMemory();
	lUsedMemory += sizeof(KWDRValueRankSelfDistance) - sizeof(KWDRValueRank);
	lUsedMemory += cvValueRankSelfDistances.GetUsedMemory();
	return lUsedMemory;
}

void KWDRValueRankSelfDistance::Optimize(KWClass* kwcOwnerClass)
{
	KWDRFrequencies* frequencyRule;
	int nIndex;
	int nFrequency;
	Continuous cTotalFrequency;
	KWDRUnivariatePartition* univariatePartitionRule;

	require(IsCompiled());

	// Appel de la methode ancetre
	KWDRDataGridRule::Optimize(kwcOwnerClass);
	assert(dataGridRule == cast(KWDRDataGrid*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass)));
	assert(dataGridRule->GetOperandNumber() == 2);

	// Memorisation de la partition univariee associee a la grille
	univariatePartitionRule =
	    cast(KWDRUnivariatePartition*, dataGridRule->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
	check(univariatePartitionRule);

	// Acces aux effectif par valeur
	frequencyRule =
	    cast(KWDRFrequencies*, dataGridRule->GetOperandAt(1)->GetReferencedDerivationRule(kwcOwnerClass));
	check(frequencyRule);

	// Initialisation de la taille vecteur de rang
	assert(frequencyRule->GetFrequencyNumber() == univariatePartitionRule->GetPartNumber());
	cvValueRankSelfDistances.SetSize(univariatePartitionRule->GetPartNumber());

	// Calcul de l'effectif total
	cTotalFrequency = (Continuous)frequencyRule->ComputeTotalFrequency();
	if (cTotalFrequency == 0)
		cTotalFrequency = 1;

	// Le rang est egal a l'index moyen de la valeur, normalise par l'effectif total
	for (nIndex = 0; nIndex < cvValueRankSelfDistances.GetSize(); nIndex++)
	{
		nFrequency = frequencyRule->GetFrequencyAt(nIndex);
		cvValueRankSelfDistances.SetAt(nIndex, (Continuous)(nFrequency / cTotalFrequency) / 3.0);
	}
}

///////////////////////////////////////////////////////////////
// Classe KWDRInverseValueRank

KWDRInverseValueRank::KWDRInverseValueRank()
{
	SetName("InverseValueRank");
	SetLabel("Inverse value rank of an instance");
	SetType(KWType::Continuous);
	continuousValueSetRule = NULL;
}

KWDRInverseValueRank::~KWDRInverseValueRank() {}

KWDerivationRule* KWDRInverseValueRank::Create() const
{
	return new KWDRInverseValueRank;
}

boolean KWDRInverseValueRank::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	KWDRContinuousValueSet continuousValueSet;
	KWDRDataGrid* checkedDataGridRule;
	KWDRUnivariatePartition* univariatePartitionRule;

	// Methode ancetre
	bOk = KWDRValueRank::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification que l'on a une partition simple de type continu
	if (bOk)
	{
		// Acces a la regle de grille du premier operande
		checkedDataGridRule =
		    cast(KWDRDataGrid*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		check(checkedDataGridRule);
		assert(checkedDataGridRule->GetOperandNumber() == 2);

		// Recherche du type de l'attribut de la grille
		// La methode GetAttributeType n'est pas encore disponible (il faut une compilation)
		univariatePartitionRule =
		    cast(KWDRUnivariatePartition*,
			 checkedDataGridRule->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
		check(univariatePartitionRule);
		assert(univariatePartitionRule->GetAttributeType() == KWType::Continuous);

		// Test du type de partition univariee
		if (univariatePartitionRule->GetName() != continuousValueSet.GetName())
		{
			AddError("The univariate partition of the data grid in first operand should be " +
				 continuousValueSet.GetName());
			bOk = false;
		}
	}
	return bOk;
}

Continuous KWDRInverseValueRank::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cRank;
	int nValueIndex;
	Continuous cValue;

	require(Check());
	require(IsCompiled());
	require(IsOptimized());

	// Acces au rank normalise
	cRank = GetSecondOperand()->GetContinuousValue(kwoObject);
	if (cRank < 0)
		cRank = 0;
	else if (cRank > 1)
		cRank = 1;

	// Calcul de l'index de valeur correspondant a un rang normalise
	nValueIndex = ComputeNormalizedRankIndex(cRank);

	// Recherche de la valeur correspondante
	cValue = continuousValueSetRule->GetValueAt(nValueIndex);
	return cValue;
}

void KWDRInverseValueRank::Optimize(KWClass* kwcOwnerClass)
{
	require(IsCompiled());

	// Appel de la methode ancetre
	KWDRValueRank::Optimize(kwcOwnerClass);
	assert(dataGridRule == cast(KWDRDataGrid*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass)));
	assert(dataGridRule->GetOperandNumber() == 2);

	// Memorisation de la partition univariee associee a la grille
	continuousValueSetRule =
	    cast(KWDRContinuousValueSet*, dataGridRule->GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));
}

int KWDRInverseValueRank::ComputeNormalizedRankIndex(Continuous cRank) const
{
	int nIndex;

	require(Check());
	require(IsCompiled());
	require(IsOptimized());
	require(0 <= cRank and cRank <= 1);

	// Recherche sequentielle s'il y a peu de valeurs
	if (cvValueRanks.GetSize() <= 10)
	{
		for (nIndex = 0; nIndex < cvValueRanks.GetSize() - 1; nIndex++)
		{
			if (cRank <= (cvValueRanks.GetAt(nIndex) + cvValueRanks.GetAt(nIndex + 1)) / 2)
				return nIndex;
		}
		assert(nIndex == cvValueRanks.GetSize() - 1);
		return nIndex;
	}
	// Recherche dichotomique sinon
	else
	{
		int nLowerIndex;
		int nUpperIndex;

		// Initialisation des index extremites
		nLowerIndex = 0;
		nUpperIndex = cvValueRanks.GetSize() - 2;

		// Recherche dichotomique de l'index de la valeur
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			if (cRank <= (cvValueRanks.GetAt(nIndex) + cvValueRanks.GetAt(nIndex + 1)) / 2)
				nUpperIndex = nIndex;
			else
				nLowerIndex = nIndex;

			// Modification du prochain intervalle teste
			nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// On compare par rapport aux deux index restant
		if (cRank <= (cvValueRanks.GetAt(nLowerIndex) + cvValueRanks.GetAt(nLowerIndex + 1)) / 2)
			nIndex = nLowerIndex;
		else if (cRank > (cvValueRanks.GetAt(nUpperIndex) + cvValueRanks.GetAt(nUpperIndex + 1)) / 2)
			nIndex = nUpperIndex + 1;
		else
			nIndex = nUpperIndex;

		// On retourne le resultat
		assert(nIndex == cvValueRanks.GetSize() - 1 or
		       cRank <= (cvValueRanks.GetAt(nIndex) + cvValueRanks.GetAt(nIndex + 1)) / 2);
		assert(nIndex == 0 or cRank > (cvValueRanks.GetAt(nIndex - 1) + cvValueRanks.GetAt(nIndex)) / 2);
		return nIndex;
	}
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridStats

KWDRDataGridStats::KWDRDataGridStats()
{
	SetName("DataGridStats");
	SetLabel("Data grid stats");
	SetType(KWType::Structure);
	SetStructureName("DataGridStats");

	// Gestion de l'optimisation
	nPredictionType = KWType::Unknown;
}

KWDRDataGridStats::~KWDRDataGridStats() {}

KWDerivationRule* KWDRDataGridStats::Create() const
{
	return new KWDRDataGridStats;
}

Object* KWDRDataGridStats::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsOptimized());

	// Calcul de l'index de cellule, pour avoir acces a l'index et a l'indication de valeur manquante
	ComputeCellIndex(kwoObject);

	// On retourne l'objet
	return (Object*)this;
}

longint KWDRDataGridStats::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRDataGridRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRDataGridStats) - sizeof(KWDRDataGridRule);
	lUsedMemory += cvTargetCumulativeFrequencies.GetUsedMemory();
	lUsedMemory += ptSourceConditionalLogProbs.GetUsedMemory();
	return lUsedMemory;
}

void KWDRDataGridStats::Optimize(KWClass* kwcOwnerClass)
{
	KWDataGridStats dataGridStats;
	KWDRFrequencies* frequenciesRule;
	int nSourceCellNumber;
	int nTargetCellNumber;
	int nSource;
	int nTarget;
	int nCell;
	int nCellFrequency;
	Continuous cCumulativeFrequency;

	require(IsCompiled());

	// Appel de la methode ancetre
	KWDRDataGridRule::Optimize(kwcOwnerClass);

	// Calcul de la grille de statistiques (objet temporaire) a partir de la regle,
	// ce qui permettra de construire une table de probabilites conditionnelles
	dataGridRule->ExportDataGridStats(&dataGridStats);

	// Presence d'attributs sources si moins de de parametres attributs que de dimensions dans la grille
	if (GetOperandNumber() - 1 < dataGridRule->GetAttributeNumber())
		dataGridStats.SetSourceAttributeNumber(GetOperandNumber() - 1);

	// Recherche du vecteur d'effectifs
	frequenciesRule = cast(KWDRFrequencies*,
			       dataGridRule->GetOperandAt(dataGridRule->GetOperandNumber() - 1)->GetDerivationRule());

	// Type de prediction: supervise si un seul attribut cible
	nPredictionType = KWType::Unknown;
	if (dataGridRule->GetAttributeNumber() == GetOperandNumber())
		nPredictionType = dataGridRule->GetAttributeTypeAt(dataGridRule->GetAttributeNumber() - 1);
	assert(nPredictionType != KWType::Unknown or dataGridStats.GetSourceAttributeNumber() == 0);

	// Import d'un objet de la grille afin d'en deduire les probabilites conditionnelles
	// Calcul y compris dans le cas non supervise, pour eviter les effets de bord
	ptSourceConditionalLogProbs.ImportDataGridStats(&dataGridStats, false, true);

	// Calcul des effectifs cumule dans le cas d'une cible continue
	nCell = 0;
	cCumulativeFrequency = 0;
	cvTargetCumulativeFrequencies.SetSize(0);
	if (nPredictionType == KWType::Continuous)
	{
		nSourceCellNumber = ptSourceConditionalLogProbs.GetSourceSize();
		nTargetCellNumber = ptSourceConditionalLogProbs.GetTargetSize();
		cvTargetCumulativeFrequencies.SetSize(nTargetCellNumber);
		for (nTarget = 0; nTarget < nTargetCellNumber; nTarget++)
		{
			for (nSource = 0; nSource < nSourceCellNumber; nSource++)
			{
				nCellFrequency = frequenciesRule->GetFrequencyAt(nCell);
				cCumulativeFrequency += nCellFrequency;

				// Cellule suivante
				nCell++;
			}

			// Mise a jour de l'effectif cumule par cellule cible
			cvTargetCumulativeFrequencies.SetAt(nTarget, cCumulativeFrequency);
		}
	}
}

///////////////////////////////////////////////////////////////
// Classe KWDRSourceConditionalInfo

KWDRSourceConditionalInfo::KWDRSourceConditionalInfo()
{
	SetName("SourceConditionalInfo");
	SetLabel("Source conditional info");
	SetType(KWType::Continuous);
	SetOperandNumber(2);

	// Le premier operandes est une statistique de grille
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("DataGridStats");

	// Le second operande est un index de partie cible
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRSourceConditionalInfo::~KWDRSourceConditionalInfo() {}

KWDerivationRule* KWDRSourceConditionalInfo::Create() const
{
	return new KWDRSourceConditionalInfo;
}

Continuous KWDRSourceConditionalInfo::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRDataGridStats* dataGridStats;
	int nTargetIndex;
	Continuous cSourceConditionalInfo;

	require(Check());
	require(IsCompiled());

	// Acces aux operandes
	dataGridStats = cast(KWDRDataGridStats*, GetFirstOperand()->GetStructureValue(kwoObject));
	nTargetIndex = (int)floor(GetSecondOperand()->GetContinuousValue(kwoObject) + 0.5);

	// On passe de l'index "externe" (entre 1 et J) a un index "interne" (entre 0 et J-1)
	nTargetIndex = nTargetIndex - 1;

	// Calcul de l'information conditionnelle s'il y a au moins une cellule source
	// Sinon, la grille ne contient que des attributs cibles, et l'information
	// conditionnelle de la source vaut 0
	cSourceConditionalInfo = 0;
	if (dataGridStats->GetDataGridSourceCellNumber() > 1 and 0 <= nTargetIndex and
	    nTargetIndex < dataGridStats->GetDataGridTargetCellNumber())
	{
		assert(0 <= dataGridStats->GetCellIndex() and
		       dataGridStats->GetCellIndex() < dataGridStats->GetDataGridSourceCellNumber());
		cSourceConditionalInfo =
		    -dataGridStats->GetDataGridSourceConditionalLogProbAt(dataGridStats->GetCellIndex(), nTargetIndex);
	}
	return cSourceConditionalInfo;
}
