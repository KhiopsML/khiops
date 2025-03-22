// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRDataGridDeployment.h"

void KWDRRegisterDataGridDeploymentRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRPartIndexAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRPartIdAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRDataGridDeployment);
	KWDerivationRule::RegisterDerivationRule(new KWDRPredictedPartIndex);
	KWDerivationRule::RegisterDerivationRule(new KWDRPredictedPartId);
	KWDerivationRule::RegisterDerivationRule(new KWDRPredictedPartDistances);
	KWDerivationRule::RegisterDerivationRule(new KWDRPredictedPartFrequenciesAt);
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridAtRule

KWDRDataGridAtRule::KWDRDataGridAtRule()
{
	SetName("DataGridDeploymentRule");
	SetLabel("Datagrid deployment rule");
	SetType(KWType::Continuous);
	SetOperandNumber(2);

	// Le premier operande est une grille
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("DataGrid");

	// Le second operande est un index constant
	GetSecondOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);

	// Gestion de la compilation dynamique
	dataGridRule = NULL;
	nDeploymentAttributeIndex = -1;
	nOptimizationFreshness = 0;
}

KWDRDataGridAtRule::~KWDRDataGridAtRule() {}

boolean KWDRDataGridAtRule::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());
	require(ruleFamily->GetOperandNumber() == 3);
	require(ruleFamily->GetFirstOperand()->GetType() == KWType::Structure);
	require(ruleFamily->GetFirstOperand()->GetStructureName() == "DataGrid");

	// Verification du nombre d'operandes: au moins un operande en plus de la grille et de l'index
	if (GetOperandNumber() < 3)
	{
		AddError("Wrong operand number");
		bOk = false;
	}
	// Verification des premiers operandes
	// Les derniers operandes, dont le type depend de la grille, ne sont pas verifie a ce niveau
	else
	{
		for (i = 0; i < 2; i++)
		{
			operand = cast(KWDerivationRuleOperand*, oaOperands.GetAt(i));

			if (not operand->CheckFamily(ruleFamily->GetOperandAt(i)))
			{
				AddError(sTmp + "Operand " + IntToString(i + 1) +
					 " inconsistent with that of the registered rule");
				bOk = false;
			}
		}
	}

	return bOk;
}

boolean KWDRDataGridAtRule::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	const KWDRDataGrid refDataGrid;
	const KWDRSymbolVector refSymbolVector;
	const KWDRContinuousVector refContinuousVector;
	int nDataGridAttributeNumber;
	KWDRDataGrid* referencedDataGrid;
	Continuous cDeploymentAttributeIndex;
	int nAttributeIndex;
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

		// Calcul du nombre d'attribut de la grille
		// La methode GetAttributeNumber n'est pas encore disponible (il faut une compilation)
		nDataGridAttributeNumber = referencedDataGrid->GetUncheckedAttributeNumber();
		assert(nDataGridAttributeNumber >= 1);

		// Test de validite de l'index
		cDeploymentAttributeIndex = GetOperandAt(1)->GetContinuousConstant();
		bOk = KWContinuous::ContinuousToInt(cDeploymentAttributeIndex, nAttributeIndex);
		if (not bOk)
		{
			AddError(sTmp + "The data grid variable index (" +
				 KWContinuous::ContinuousToString(cDeploymentAttributeIndex) +
				 ") in the second operand should be an integer value");
			assert(bOk == false);
		}
		else if (nAttributeIndex < 1 or nAttributeIndex > nDataGridAttributeNumber)
		{
			AddError(sTmp + "The data grid variable index (" + IntToString(nAttributeIndex) +
				 ") in the second operand should be between 1 and " +
				 IntToString(nDataGridAttributeNumber));
			bOk = false;
		}
	}
	return bOk;
}

void KWDRDataGridAtRule::Compile(KWClass* kwcOwnerClass)
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

longint KWDRDataGridAtRule::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRDataGridAtRule) - sizeof(KWDerivationRule);
	return lUsedMemory;
}

void KWDRDataGridAtRule::Optimize(KWClass* kwcOwnerClass)
{
	require(IsCompiled());
	dataGridRule = cast(KWDRDataGrid*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
	nDeploymentAttributeIndex = (int)floor(GetSecondOperand()->GetContinuousConstant() - 1 + 0.5);
	assert(0 <= nDeploymentAttributeIndex and
	       nDeploymentAttributeIndex < dataGridRule->GetUncheckedAttributeNumber());
}

boolean KWDRDataGridAtRule::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridDeployment

KWDRDataGridDeployment::KWDRDataGridDeployment()
{
	KWDRSymbolVector refSymbolVector;

	SetName("DataGridDeployment");
	SetLabel("Datagrid deployment rule");

	// Type structure pour cette regle
	SetType(KWType::Structure);
	SetStructureName("DataGridDeployment");

	// On cree un type coherent pour le troisieme operande
	// En pratique, ce type sera un vecteur de Continuous ou de Symbol selon
	// le type de la distribution a deployer
	// On doit neanmoins preciser un type valide pour la regle generique
	assert(GetOperandNumber() == 2);
	AddOperand(new KWDerivationRuleOperand);
	GetOperandAt(2)->SetType(KWType::Structure);
	GetOperandAt(2)->SetStructureName(refSymbolVector.GetStructureName());
	SetVariableOperandNumber(true);
	nTotalDataGridFrequency = 0;
	operandeFrequencyVector = NULL;
}

KWDRDataGridDeployment::~KWDRDataGridDeployment() {}

KWDerivationRule* KWDRDataGridDeployment::Create() const
{
	return new KWDRDataGridDeployment;
}

Object* KWDRDataGridDeployment::ComputeStructureResult(const KWObject* kwoObject) const
{
	int nOperand;

	require(Check());
	require(IsCompiled());

	// On evalue les operandes selon leur type
	GetOperandAt(0)->GetStructureValue(kwoObject);
	GetOperandAt(1)->GetContinuousValue(kwoObject);
	for (nOperand = 2; nOperand < GetOperandNumber(); nOperand++)
		GetOperandAt(nOperand)->GetStructureValue(kwoObject);

	// Calcul des statistiques de deploiement
	ComputeDeploymentStats(kwoObject);

	return (Object*)this;
}

const KWDataGridDeployment* KWDRDataGridDeployment::GetDeploymentResults() const
{
	require(dataGridDeployment.IsDeploymentStatsComputed());
	return &dataGridDeployment;
}

boolean KWDRDataGridDeployment::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	const KWDRDataGrid refDataGrid;
	const KWDRSymbolVector refSymbolVector;
	const KWDRContinuousVector refContinuousVector;
	const KWDerivationRule* refVectorRule;
	int nDataGridAttributeNumber;
	KWDRDataGrid* referencedDataGrid;
	int nDataGridAttributeType;
	KWDerivationRuleOperand* operand;
	Continuous cDeploymentAttributeIndex;
	int nAttributeIndex;
	int nDistributionOperandIndex;
	int i;
	ALString sTmp;

	require(GetOperandNumber() > 0);
	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDRDataGridAtRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification du type des derniers operandes
	if (bOk)
	{
		referencedDataGrid = cast(KWDRDataGrid*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));

		// Calcul du nombre d'attribut de la grille
		// La methode GetAttributeNumber n'est pas encore disponible (il faut une compilation)
		nDataGridAttributeNumber = referencedDataGrid->GetUncheckedAttributeNumber();
		assert(nDataGridAttributeNumber >= 1);

		// Contrainte specifique sur la taille de la grille dans le cas de deploiement de distribution de
		// distribution La grille doit avoir au moins deux attributs
		if (nDataGridAttributeNumber < 2)
		{
			AddError(sTmp + "The data grid in the first operand must contain at least two variables");
			bOk = false;
		}

		// Acces a l'index, en passant a un index "interne" entre 0 et K-1
		cDeploymentAttributeIndex = GetOperandAt(1)->GetContinuousConstant();
		nAttributeIndex = (int)floor(cDeploymentAttributeIndex + 0.5);
		assert(1 <= nAttributeIndex and nAttributeIndex <= nDataGridAttributeNumber);
		nAttributeIndex--;

		// Tests sur les derniers operandes, qui doivent etre des vecteurs de valeurs
		// pour les dimension de la grille complementaire a la dimension de deploiement
		if (bOk)
		{
			assert(nDataGridAttributeNumber >= 2);
			assert(0 <= nAttributeIndex and nAttributeIndex < nDataGridAttributeNumber);

			// Le nombre d'operandes supplementaire doit etre egal a la dimension de la grille diminuee de
			// un, plus optionnelement un si un vecteur de frequence est utilise Soit 1 + 1 +
			// nDataGridAttributeNumber-1 (+1)
			if (GetOperandNumber() < nDataGridAttributeNumber + 1)
			{
				AddError(sTmp + "The number of operands (" + IntToString(GetOperandNumber()) +
					 ") should be at least one plus the number of variables (" +
					 IntToString(nDataGridAttributeNumber) +
					 ") of the data grid in the first operand");
				bOk = false;
			}
			// Cas avec nombre d'operandes plus grande que le max, en tenant compte du vecteur de frequences
			// optionnels
			else if (GetOperandNumber() > nDataGridAttributeNumber + 2)
			{
				AddError(sTmp + "The number of operands (" + IntToString(GetOperandNumber()) +
					 ") should be at most two plus the number of variables (" +
					 IntToString(nDataGridAttributeNumber) +
					 ") of the data grid in the first operand");
				bOk = false;
			}

			// Verification du type des operandes
			if (bOk)
			{
				// Parcours des attributs de la grille
				nDistributionOperandIndex = 2;
				for (i = 0; i < nDataGridAttributeNumber; i++)
				{
					// On saute l'attribut de deploiement
					if (i == nAttributeIndex)
						continue;

					// Recherche du type de l'attribut de l'attribut indexe de la grille,
					// qui correspond a l'attribut portant sur la distribution de valeurs
					assert(referencedDataGrid->CheckReferencedDerivationRuleAt(1 - nAttributeIndex,
												   kwcOwnerClass, ""));
					nDataGridAttributeType = referencedDataGrid->GetUncheckedAttributeTypeAt(i);
					if (not KWType::IsSimple(nDataGridAttributeType))
					{
						// L'erreur sur la grille doit etre diagniostique par ailleurs
						bOk = false;
						break;
					}

					// Test de compatibilite du type de l'operande avec celui de l'attribut sur la
					// distribution de valeur
					operand = GetOperandAt(nDistributionOperandIndex);
					if (nDataGridAttributeType == KWType::Symbol)
						refVectorRule = &refSymbolVector;
					else
						refVectorRule = &refContinuousVector;
					if (operand->GetType() != KWType::Structure or
					    operand->GetStructureName() != refVectorRule->GetStructureName())
					{
						AddError(sTmp + "Type of operand (" +
							 IntToString(nDistributionOperandIndex + 1) + ") should be " +
							 KWType::ToString(KWType::Structure) + "(" +
							 refVectorRule->GetStructureName() + ")");
						bOk = false;
						break;
					}

					// Incrementation de l'index d'operande
					nDistributionOperandIndex++;
				}

				// Test du vecteur de frequence, dernier operande optionnel
				if (bOk and GetOperandNumber() == nDataGridAttributeNumber + 2)
				{
					// Test de compatibilite du type de l'operande avec un vecteur de frequences
					// (valeurs numeriques)
					operand = GetOperandAt(GetOperandNumber() - 1);
					if (operand->GetType() != KWType::Structure or
					    operand->GetStructureName() != refContinuousVector.GetStructureName())
					{
						AddError(sTmp + "Type of last operand should be " +
							 KWType::ToString(KWType::Structure) + "(" +
							 refContinuousVector.GetStructureName() + ")");
						bOk = false;
					}
				}
			}
		}
	}
	return bOk;
}

longint KWDRDataGridDeployment::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRDataGridAtRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRDataGridDeployment) - sizeof(KWDRDataGridAtRule);
	lUsedMemory += dataGridDeployment.GetUsedMemory();
	return lUsedMemory;
}

void KWDRDataGridDeployment::Optimize(KWClass* kwcOwnerClass)
{
	KWDataGridStats dataGridStats;

	require(IsCompiled());

	// Optimisation ancetre
	KWDRDataGridAtRule::Optimize(kwcOwnerClass);

	// Export d'un objet DataGridStats a partir de la regle de type grille (du premier operande)
	dataGridRule->ExportDataGridStats(&dataGridStats);

	// Nettoyage initial de la preparation precedente
	dataGridDeployment.Clean();
	dataGridDeployment.Initialize(0, 0);
	operandeFrequencyVector = NULL;

	// Initialisation du service de deploiement de grille de donnees
	dataGridDeployment.ImportDataGridStats(&dataGridStats);
	dataGridDeployment.SetDeploymentAttributeIndex(nDeploymentAttributeIndex);
	dataGridDeployment.PrepareForDeployment();

	// Memorisation de l'effectif total de la grille
	nTotalDataGridFrequency = dataGridStats.ComputeGridFrequency();

	// Memorisation de l'operande portant sur le vecteur de frequence
	if (GetOperandNumber() == dataGridStats.GetAttributeNumber() + 2)
		operandeFrequencyVector = GetOperandAt(GetOperandNumber() - 1);
}

void KWDRDataGridDeployment::ComputeDeploymentStats(const KWObject* kwoObject) const
{
	ObjectArray oaDistributionValueVectors;
	IntVector ivFrequencyVector;
	IntVector* ivOptionalFrequencyVector;
	int nAttribute;
	int nDistributionAttributeIndex;
	const SymbolVector* svValueDistribution;
	const ContinuousVector* cvValueDistribution;
	const ContinuousVector* cvFrequencyVector;
	SymbolVector svValueNullDistribution;
	ContinuousVector cvValueNullDistribution;
	int nDistributionSize;
	int i;
	Continuous cFrequency;
	Continuous cTotalFrequency;
	boolean bOk;

	require(Check());
	require(IsOptimized());
	require(dataGridDeployment.GetAttributeNumber() >= 2);
	require(dataGridDeployment.GetDeploymentAttributeIndex() == nDeploymentAttributeIndex);

	// Parametrage des vecteurs de valeurs par attribut de distribution
	oaDistributionValueVectors.SetSize(dataGridDeployment.GetAttributeNumber());
	nDistributionAttributeIndex = 0;
	nDistributionSize = -1;
	bOk = true;
	for (nAttribute = 0; nAttribute < dataGridDeployment.GetAttributeNumber(); nAttribute++)
	{
		if (nAttribute != nDeploymentAttributeIndex)
		{
			if (dataGridDeployment.GetAttributeAt(nAttribute)->GetAttributeType() == KWType::Symbol)
			{
				svValueDistribution =
				    cast(KWDRSymbolVector*,
					 GetOperandAt(2 + nDistributionAttributeIndex)->GetStructureValue(kwoObject))
					->GetValues();
				oaDistributionValueVectors.SetAt(nAttribute, (Object*)svValueDistribution);

				// On teste si toutes les distribution ont la meme taille
				if (nDistributionSize == -1)
					nDistributionSize = svValueDistribution->GetSize();
				else
					bOk = bOk and nDistributionSize == svValueDistribution->GetSize();
			}
			else
			{
				assert(dataGridDeployment.GetAttributeAt(nAttribute)->GetAttributeType() ==
				       KWType::Continuous);
				cvValueDistribution =
				    cast(KWDRContinuousVector*,
					 GetOperandAt(2 + nDistributionAttributeIndex)->GetStructureValue(kwoObject))
					->GetValues();
				oaDistributionValueVectors.SetAt(nAttribute, (Object*)cvValueDistribution);

				// On teste si toutes les distribution ont la meme taille
				if (nDistributionSize == -1)
					nDistributionSize = cvValueDistribution->GetSize();
				else
					bOk = bOk and nDistributionSize == cvValueDistribution->GetSize();
			}
			nDistributionAttributeIndex++;
		}
	}
	assert(nDistributionSize >= 0);

	// Calcul du vecteur de frequence optionnel
	ivOptionalFrequencyVector = NULL;
	if (bOk and operandeFrequencyVector != NULL)
	{
		cvFrequencyVector =
		    cast(KWDRContinuousVector*, operandeFrequencyVector->GetStructureValue(kwoObject))->GetValues();
		bOk = bOk and nDistributionSize == cvFrequencyVector->GetSize();

		// Initialisation d'un vecteur de frequence
		if (bOk)
		{
			cTotalFrequency = 0, ivFrequencyVector.SetSize(nDistributionSize);
			for (i = 0; i < nDistributionSize; i++)
			{
				cFrequency = cvFrequencyVector->GetAt(i);

				// Memorisation de la frequence si valide, erreur sinon
				if (cFrequency >= 0 and cFrequency < INT_MAX)
				{
					ivFrequencyVector.SetAt(i, (int)floor(cFrequency + 0.5));
					cTotalFrequency += cFrequency;
				}
				else
				{
					bOk = false;
					break;
				}
			}

			// On verifie egalement que la frequence cumulee est valdie
			if (bOk)
				bOk = (cTotalFrequency < INT_MAX);

			// On verifie egalement que la frequence cumulee plus celle de la grille est valdie
			if (bOk)
				bOk = (cTotalFrequency + nTotalDataGridFrequency < INT_MAX);
		}

		// Si OK, on memorise ce parametre
		if (bOk)
			ivOptionalFrequencyVector = &ivFrequencyVector;
	}

	// On remplace par des vecteurs de taille nulle en cas d'incoherence des tailles de distribution
	if (not bOk)
	{
		for (nAttribute = 0; nAttribute < dataGridDeployment.GetAttributeNumber(); nAttribute++)
		{
			if (nAttribute != nDeploymentAttributeIndex)
			{
				if (dataGridDeployment.GetAttributeAt(nAttribute)->GetAttributeType() == KWType::Symbol)
					oaDistributionValueVectors.SetAt(nAttribute, &svValueNullDistribution);
				else
					oaDistributionValueVectors.SetAt(nAttribute, &cvValueNullDistribution);
			}
		}
	}

	// Calcul des statistiques de deploiement
	dataGridDeployment.ComputeDeploymentStats(&oaDistributionValueVectors, ivOptionalFrequencyVector);
}

///////////////////////////////////////////////////////////////
// Classe KWDRPredictedPartIndex

KWDRPredictedPartIndex::KWDRPredictedPartIndex()
{
	const KWDRDataGridDeployment refDataGridDeploymentRule;

	SetName("PredictedPartIndex");
	SetLabel("Part index of a distribution for a given variable of a data grid");
	SetType(KWType::Continuous);

	// Le premier et seul operande est une regle de deploiement de grille
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(refDataGridDeploymentRule.GetStructureName());
}

KWDRPredictedPartIndex::~KWDRPredictedPartIndex() {}

KWDerivationRule* KWDRPredictedPartIndex::Create() const
{
	return new KWDRPredictedPartIndex;
}

Continuous KWDRPredictedPartIndex::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRDataGridDeployment* dataGridDeploymentRule;

	require(IsCompiled());

	// Acces au premier operande
	dataGridDeploymentRule = cast(KWDRDataGridDeployment*, GetFirstOperand()->GetStructureValue(kwoObject));

	// On rend la partie predite
	return (Continuous)(dataGridDeploymentRule->GetDeploymentResults()->GetDeploymentIndex() + 1);
}

///////////////////////////////////////////////////////////////
// Classe KWDRPredictedPartId

KWDRPredictedPartId::KWDRPredictedPartId()
{
	SetName("PredictedPartId");
	SetLabel("Part identifier of a distribution for a given variable of a data grid");
	SetType(KWType::Symbol);
	nDynamicCompileFreshness = 0;
}

KWDRPredictedPartId::~KWDRPredictedPartId() {}

KWDerivationRule* KWDRPredictedPartId::Create() const
{
	return new KWDRPredictedPartId;
}

Symbol KWDRPredictedPartId::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KWDRDataGridDeployment* dataGridDeploymentRule;
	int nBestDeploymentPart;

	require(Check());
	require(IsCompiled());
	require(KWType::IsSimple(GetSecondOperand()->GetType()));

	// Acces au premier operande
	dataGridDeploymentRule = cast(KWDRDataGridDeployment*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness < nCompileFreshness)
	{
		KWDRDataGrid* dataGridRule;
		int nDeploymentAttributeIndex;
		int nIndex;

		// Acces aux parametres necessaires
		dataGridRule =
		    cast(KWDRDataGrid*, dataGridDeploymentRule->GetFirstOperand()->GetStructureValue(kwoObject));
		nDeploymentAttributeIndex =
		    (int)floor(dataGridDeploymentRule->GetSecondOperand()->GetContinuousValue(kwoObject) + 0.5);

		// Calcul des Ids de la partition
		svPartIds.SetSize(dataGridRule->GetAttributePartNumberAt(nDeploymentAttributeIndex));
		for (nIndex = 0; nIndex < svPartIds.GetSize(); nIndex++)
			svPartIds.SetAt(nIndex, KWDRUnivariatePartition::ComputeId(nIndex));

		// Memorisation de la fraicheur
		nDynamicCompileFreshness = nCompileFreshness;
	}

	// On rend l'identifiant de la partie predite
	nBestDeploymentPart = dataGridDeploymentRule->GetDeploymentResults()->GetDeploymentIndex();
	assert(0 <= nBestDeploymentPart and nBestDeploymentPart < svPartIds.GetSize());
	return svPartIds.GetAt(nBestDeploymentPart);
}

longint KWDRPredictedPartId::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRPredictedPartIndex::GetUsedMemory();
	lUsedMemory += sizeof(KWDRPredictedPartId) - sizeof(KWDRPredictedPartIndex);
	lUsedMemory += svPartIds.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRPredictedPartDistances

KWDRPredictedPartDistances::KWDRPredictedPartDistances()
{
	const KWDRDataGridDeployment refDataGridDeploymentRule;

	SetName("PredictedPartDistances");
	SetLabel("Part distances of a distribution for a given variable of a data grid");

	// Le code retour est un vecteur de distances
	SetType(KWType::Structure);
	SetStructureName("Vector");

	// Le premier et seul operande est une regle de deploiement de grille
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(refDataGridDeploymentRule.GetStructureName());
}

KWDRPredictedPartDistances::~KWDRPredictedPartDistances() {}

KWDerivationRule* KWDRPredictedPartDistances::Create() const
{
	return new KWDRPredictedPartDistances;
}

Object* KWDRPredictedPartDistances::ComputeStructureResult(const KWObject* kwoObject) const
{
	boolean bNormalize = true;
	KWDRDataGridDeployment* dataGridDeploymentRule;
	const DoubleVector* dvDeploymentDistances;
	int nPart;
	int nDistributionTotalFrequency;

	require(IsCompiled());

	// Acces au premier operande
	dataGridDeploymentRule = cast(KWDRDataGridDeployment*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Acces au vecteur de distances, et reformatage
	dvDeploymentDistances = dataGridDeploymentRule->GetDeploymentResults()->GetDeploymentDistances();
	nDistributionTotalFrequency = dataGridDeploymentRule->GetDeploymentResults()->GetDistributionTotalFrequency();
	kwdrcvDistances.SetValueNumber(dvDeploymentDistances->GetSize());
	for (nPart = 0; nPart < dvDeploymentDistances->GetSize(); nPart++)
	{
		if (nDistributionTotalFrequency > 0)
		{
			if (bNormalize)
				kwdrcvDistances.SetValueAt(nPart, (Continuous)(dvDeploymentDistances->GetAt(nPart) /
									       nDistributionTotalFrequency));
			else
				kwdrcvDistances.SetValueAt(nPart, (Continuous)(dvDeploymentDistances->GetAt(nPart)));
		}
		else
			kwdrcvDistances.SetValueAt(nPart, 0);
	}

	// On rend le vecteur des distances
	return &kwdrcvDistances;
}

longint KWDRPredictedPartDistances::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRPredictedPartDistances) - sizeof(KWDerivationRule);
	lUsedMemory += kwdrcvDistances.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRPredictedPartFrequenciesAt

KWDRPredictedPartFrequenciesAt::KWDRPredictedPartFrequenciesAt()
{
	const KWDRDataGridDeployment refDataGridDeploymentRule;

	SetName("PredictedPartFrequenciesAt");
	SetLabel("Part frequencies of a distribution for a given variable of a data grid");

	// Le code retour est un vecteur d'effectifs
	SetType(KWType::Structure);
	SetStructureName("Vector");

	// Le premier operande est une regle de deploiement de grille
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName(refDataGridDeploymentRule.GetStructureName());

	// Le second operand est un index d'attribut de la grille, hors attribut de deploiement
	GetSecondOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRPredictedPartFrequenciesAt::~KWDRPredictedPartFrequenciesAt() {}

KWDerivationRule* KWDRPredictedPartFrequenciesAt::Create() const
{
	return new KWDRPredictedPartFrequenciesAt;
}

Object* KWDRPredictedPartFrequenciesAt::ComputeStructureResult(const KWObject* kwoObject) const
{
	boolean bNormalize = false;
	KWDRDataGridDeployment* dataGridDeploymentRule;
	int nDistributionAttributeIndex;
	const IntVector* ivDistributionDistances;
	int nPart;
	int nDistributionTotalFrequency;

	require(IsCompiled());

	// Acces au premier operande
	dataGridDeploymentRule = cast(KWDRDataGridDeployment*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Acces a l'index de distribution
	nDistributionAttributeIndex = (int)floor(GetOperandAt(1)->GetContinuousConstant() + 0.5);
	nDistributionAttributeIndex--;

	// Acces au vecteur de distances, et reformatage
	ivDistributionDistances =
	    dataGridDeploymentRule->GetDeploymentResults()->GetDistributionFrequenciesAt(nDistributionAttributeIndex);
	nDistributionTotalFrequency = dataGridDeploymentRule->GetDeploymentResults()->GetDistributionTotalFrequency();
	kwdrcvFrequencies.SetValueNumber(ivDistributionDistances->GetSize());
	for (nPart = 0; nPart < ivDistributionDistances->GetSize(); nPart++)
	{
		if (nDistributionTotalFrequency > 0)
		{
			if (bNormalize)
				kwdrcvFrequencies.SetValueAt(nPart, (Continuous)(ivDistributionDistances->GetAt(nPart) *
										 1.0 / nDistributionTotalFrequency));
			else
				kwdrcvFrequencies.SetValueAt(nPart,
							     (Continuous)(ivDistributionDistances->GetAt(nPart)));
		}
		else
			kwdrcvFrequencies.SetValueAt(nPart, 0);
	}

	// On rend le vecteur d'effectifs
	return &kwdrcvFrequencies;
}

boolean KWDRPredictedPartFrequenciesAt::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	KWDRDataGridDeployment* dataGridDeploymentRule;
	int nDataGridAttributeNumber;
	KWDRDataGrid* referencedDataGrid;
	Continuous cDeploymentAttributeIndex;
	int nDeploymentAttributeIndex;
	Continuous cDistributionAttributeIndex;
	int nDistributionAttributeIndex;
	ALString sTmp;

	require(GetOperandNumber() == 2);
	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification que le dernier operande est un index d'attribut de la grille coherent avec l'attribut de
	// deploiement
	if (bOk)
	{
		// Acces au premier operande
		dataGridDeploymentRule =
		    cast(KWDRDataGridDeployment*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));

		// Acces a la grille
		referencedDataGrid =
		    cast(KWDRDataGrid*,
			 dataGridDeploymentRule->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));

		// Calcul du nombre d'attribut de la grille
		// La methode GetAttributeNumber n'est pas encore disponible (il faut une compilation)
		nDataGridAttributeNumber = referencedDataGrid->GetUncheckedAttributeNumber();
		assert(nDataGridAttributeNumber >= 1);

		// Acces a l'index de deploiement, en passant a un index "interne" entre 0 et K-1 (deja verifie dans la
		// classe ancetre)
		cDeploymentAttributeIndex = dataGridDeploymentRule->GetOperandAt(1)->GetContinuousConstant();
		bOk = KWContinuous::ContinuousToInt(cDeploymentAttributeIndex, nDeploymentAttributeIndex);
		assert(bOk);
		assert(1 <= nDeploymentAttributeIndex and nDeploymentAttributeIndex <= nDataGridAttributeNumber);

		// Acces a l'index de distribution, en passant a un index "interne" entre 0 et K-1
		cDistributionAttributeIndex = GetOperandAt(1)->GetContinuousConstant();
		bOk = KWContinuous::ContinuousToInt(cDistributionAttributeIndex, nDistributionAttributeIndex);

		// Test de validite de l'index de distribution
		if (not bOk)
		{
			AddError(sTmp + "The data grid variable index (" + IntToString(nDistributionAttributeIndex) +
				 ") in the second operand should be an integer value");
			assert(bOk == false);
		}
		else if (nDistributionAttributeIndex < 1 or nDistributionAttributeIndex > nDataGridAttributeNumber)
		{
			AddError(
			    sTmp + "The data grid variable index (" + IntToString(nDistributionAttributeIndex + 1) +
			    ") in the second operand should be between 1 and " + IntToString(nDataGridAttributeNumber));
			bOk = false;
		}
		else if (nDistributionAttributeIndex == nDeploymentAttributeIndex)
		{
			AddError(sTmp + "The data grid variable index (" +
				 IntToString(nDistributionAttributeIndex + 1) +
				 ") in the second operand should be different from the deployment index " +
				 IntToString(nDeploymentAttributeIndex));
			bOk = false;
		}
	}
	return bOk;
}

longint KWDRPredictedPartFrequenciesAt::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRPredictedPartFrequenciesAt) - sizeof(KWDerivationRule);
	lUsedMemory += kwdrcvFrequencies.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridDeploymentDistributionRule

KWDRDataGridDeploymentDistributionRule::KWDRDataGridDeploymentDistributionRule()
{
	KWDRSymbolVector refSymbolVector;

	SetName("DataGridDeploymentDistributionRule");
	SetLabel("Datagrid deployment distribution rule");

	// On cree un type coherent pour le troisieme operande
	// En pratique, ce type sera un vecteur de Continuous ou de Symbol selon
	// le type de la distribution a deployer
	// On doit neanmoins preciser un type valide pour la regle generique
	assert(GetOperandNumber() == 2);
	AddOperand(new KWDerivationRuleOperand);
	GetOperandAt(2)->SetType(KWType::Structure);
	GetOperandAt(2)->SetStructureName(refSymbolVector.GetStructureName());
	SetVariableOperandNumber(true);
}

KWDRDataGridDeploymentDistributionRule::~KWDRDataGridDeploymentDistributionRule() {}

boolean KWDRDataGridDeploymentDistributionRule::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	const KWDRDataGrid refDataGrid;
	const KWDRSymbolVector refSymbolVector;
	const KWDRContinuousVector refContinuousVector;
	const KWDerivationRule* refVectorRule;
	int nDataGridAttributeNumber;
	KWDRDataGrid* referencedDataGrid;
	int nDataGridAttributeType;
	KWDerivationRuleOperand* operand;
	Continuous cDeploymentAttributeIndex;
	int nAttributeIndex;
	int nDistributionOperandIndex;
	int i;
	ALString sTmp;

	require(GetOperandNumber() > 0);
	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDRDataGridAtRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification du type des derniers operandes
	if (bOk)
	{
		referencedDataGrid = cast(KWDRDataGrid*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));

		// Calcul du nombre d'attribut de la grille
		// La methode GetAttributeNumber n'est pas encore disponible (il faut une compilation)
		nDataGridAttributeNumber = referencedDataGrid->GetUncheckedAttributeNumber();
		assert(nDataGridAttributeNumber >= 1);

		// Contrainte specifique sur la taille de la grille dans le cas de deploiement de distribution de
		// distribution La grille doit avoir au moins deux attributs
		if (nDataGridAttributeNumber < 2)
		{
			AddError(sTmp + "The data grid in the first operand must contain at least two variables");
			bOk = false;
		}

		// Acces a l'index, en passant a un index "interne" entre 0 et K-1
		cDeploymentAttributeIndex = GetOperandAt(1)->GetContinuousConstant();
		nAttributeIndex = (int)floor(cDeploymentAttributeIndex + 0.5);
		assert(1 <= nAttributeIndex and nAttributeIndex <= nDataGridAttributeNumber);
		nAttributeIndex--;

		// Tests sur les derniers operandes, qui doivent etre des vecteurs de valeurs
		// pour les dimension de la grille complementaire a la dimension de deploiement
		if (bOk)
		{
			assert(nDataGridAttributeNumber >= 2);
			assert(0 <= nAttributeIndex and nAttributeIndex < nDataGridAttributeNumber);

			// Le nombre d'operandes supplementaire doit etre egal a la dimension de la grille diminuee de
			// un Soit 1 + 1 + nDataGridAttributeNumber-1
			if (GetOperandNumber() != nDataGridAttributeNumber + 1)
			{
				AddError(sTmp + "The number of operands (" + IntToString(GetOperandNumber()) +
					 ") should be equal to one plus the number of variables (" +
					 IntToString(nDataGridAttributeNumber) +
					 ") of the data grid in the first operand");
				bOk = false;
			}

			// Verification du type des operandes
			if (bOk)
			{
				// Parcours des attributs de la grille
				nDistributionOperandIndex = 2;
				for (i = 0; i < nDataGridAttributeNumber; i++)
				{
					// On saute l'attribut de deploiement
					if (i == nAttributeIndex)
						continue;

					// Recherche du type de l'attribut de l'attribut indexe de la grille,
					// qui correspond a l'attribut portant sur la distribution de valeurs
					// La methode GetAttributeType n'est pas encore disponible (il faut une
					// compilation)
					assert(referencedDataGrid->CheckReferencedDerivationRuleAt(1 - nAttributeIndex,
												   kwcOwnerClass, ""));
					nDataGridAttributeType = referencedDataGrid->GetUncheckedAttributeTypeAt(i);
					if (not KWType::IsSimple(nDataGridAttributeType))
					{
						// L'erreur sur la grille doit etre diagniostique par ailleurs
						bOk = false;
						break;
					}

					// Test de compatibilite du type de l'operande avec celui de l'attribut sur la
					// distribution de valeur
					operand = GetOperandAt(nDistributionOperandIndex);
					if (nDataGridAttributeType == KWType::Symbol)
						refVectorRule = &refSymbolVector;
					else
						refVectorRule = &refContinuousVector;
					if (operand->GetType() != KWType::Structure or
					    operand->GetStructureName() != refVectorRule->GetStructureName())
					{
						AddError(sTmp + "Type of operand (" +
							 IntToString(nDistributionOperandIndex + 1) + ") should be " +
							 KWType::ToString(KWType::Structure) + "(" +
							 refVectorRule->GetStructureName() + ")");
						bOk = false;
					}

					// Incrementation de l'index d'operande
					nDistributionOperandIndex++;
				}
			}
		}
	}
	return bOk;
}

void KWDRDataGridDeploymentDistributionRule::Compile(KWClass* kwcOwnerClass)
{
	// Methode ancetre
	KWDRDataGridAtRule::Compile(kwcOwnerClass);
}

void KWDRDataGridDeploymentDistributionRule::Optimize(KWClass* kwcOwnerClass)
{
	// Methode ancetre
	KWDRDataGridAtRule::Optimize(kwcOwnerClass);
}

///////////////////////////////////////////////////////////////
// Classe KWDRPartIndexAt

KWDRPartIndexAt::KWDRPartIndexAt()
{
	SetName("PartIndexAt");
	SetLabel("Part index of a value for a given variable of a data grid");

	// Le type du troisieme operande est guide par les attributs de la grille
	// On doit neanmoins preciser un type valide pour la regle generique
	assert(GetOperandNumber() == 2);
	AddOperand(new KWDerivationRuleOperand);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRPartIndexAt::~KWDRPartIndexAt() {}

KWDerivationRule* KWDRPartIndexAt::Create() const
{
	return new KWDRPartIndexAt;
}

Continuous KWDRPartIndexAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	int nPartIndex;

	require(Check());
	require(IsOptimized());

	// On passe aux index externes, commencant a 1
	nPartIndex = ComputeValueDeploymentPartIndex(kwoObject);
	nPartIndex++;
	return (Continuous)nPartIndex;
}

boolean KWDRPartIndexAt::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	const KWDRDataGrid refDataGrid;
	const KWDRSymbolVector refSymbolVector;
	const KWDRContinuousVector refContinuousVector;
	int nDataGridAttributeNumber;
	KWDRDataGrid* referencedDataGrid;
	int nDataGridAttributeType;
	KWDerivationRuleOperand* operand;
	Continuous cDeploymentAttributeIndex;
	int nAttributeIndex;
	ALString sTmp;

	require(GetOperandNumber() == 3);
	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDRDataGridAtRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Le nombre d'operandes doit etre egal exactement a 3
	if (bOk and GetOperandNumber() != 3)
	{
		AddError(sTmp + "The number of operands should be 3");
		bOk = false;
	}

	// Verification que le dernier operande est une valeur du meme type que l'attribut de deploiement
	if (bOk)
	{
		referencedDataGrid = cast(KWDRDataGrid*, GetOperandAt(0)->GetReferencedDerivationRule(kwcOwnerClass));

		// Calcul du nombre d'attribut de la grille
		// La methode GetAttributeNumber n'est pas encore disponible (il faut une compilation)
		nDataGridAttributeNumber = referencedDataGrid->GetUncheckedAttributeNumber();
		assert(nDataGridAttributeNumber >= 1);

		// Acces a l'index, en passant a un index "interne" entre 0 et K-1
		cDeploymentAttributeIndex = GetOperandAt(1)->GetContinuousConstant();
		nAttributeIndex = (int)floor(cDeploymentAttributeIndex + 0.5);
		assert(1 <= nAttributeIndex and nAttributeIndex <= nDataGridAttributeNumber);
		nAttributeIndex--;

		// Tests sur le troisieme operande, specifiques au deploiement de valeurs
		// Recherche du type de l'attribut indexe de la grille
		// La methode GetAttributeType n'est pas encore disponible (il faut une compilation)
		assert(referencedDataGrid->CheckReferencedDerivationRuleAt(nAttributeIndex, kwcOwnerClass, ""));
		nDataGridAttributeType = referencedDataGrid->GetUncheckedAttributeTypeAt(nAttributeIndex);
		if (not KWType::IsSimple(nDataGridAttributeType))
		{
			// L'erreur sur la grille doit etre diagniostique par ailleurs
			bOk = false;
		}

		// Test de compatibilite du type du troisieme operande avec celui de l'attribut sur la valeur
		// a deployer
		operand = GetOperandAt(2);
		if (bOk and operand->GetType() != nDataGridAttributeType)
		{
			AddError(sTmp +
				 "Type of the last operand inconsistent with that of deployment variable in the data "
				 "grid (" +
				 KWType::ToString(nDataGridAttributeType) + ")");
			bOk = false;
		}
	}
	return bOk;
}

int KWDRPartIndexAt::ComputeValueDeploymentPartIndex(const KWObject* kwoObject) const
{
	Continuous cValue;
	Symbol sValue;
	int nPartIndex;

	require(Check());
	require(IsOptimized());

	// Recherche de l'index de la partie dans le cas Continuous
	if (dataGridRule->GetAttributeTypeAt(nDeploymentAttributeIndex) == KWType::Continuous)
	{
		assert(GetOperandAt(2)->GetType() == KWType::Continuous);
		cValue = GetOperandAt(2)->GetContinuousValue(kwoObject);
		nPartIndex = dataGridRule->GetContinuousAttributePartIndexAt(nDeploymentAttributeIndex, cValue);
	}
	// Recherche de l'index de la partie dans le cas Symbol
	else
	{
		assert(dataGridRule->GetAttributeTypeAt(nDeploymentAttributeIndex) == KWType::Symbol);
		assert(GetOperandAt(2)->GetType() == KWType::Symbol);
		sValue = GetOperandAt(2)->GetSymbolValue(kwoObject);
		nPartIndex = dataGridRule->GetSymbolAttributePartIndexAt(nDeploymentAttributeIndex, sValue);
	}
	return nPartIndex;
}

///////////////////////////////////////////////////////////////
// Classe KWDRPartIdAt

KWDRPartIdAt::KWDRPartIdAt()
{
	SetName("PartIdAt");
	SetLabel("Part identifier of a value for a given variable of a data grid");
	SetType(KWType::Symbol);
}

KWDRPartIdAt::~KWDRPartIdAt() {}

KWDerivationRule* KWDRPartIdAt::Create() const
{
	return new KWDRPartIdAt;
}

Symbol KWDRPartIdAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	int nIndex;

	require(Check());
	require(IsCompiled());
	require(KWType::IsSimple(GetSecondOperand()->GetType()));

	// Calcul de l'index de partie
	nIndex = ComputeValueDeploymentPartIndex(kwoObject);
	assert(0 <= nIndex and nIndex < svPartIds.GetSize());
	return svPartIds.GetAt(nIndex);
}

longint KWDRPartIdAt::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRPartIndexAt::GetUsedMemory();
	lUsedMemory += sizeof(KWDRPartIdAt) - sizeof(KWDRPartIndexAt);
	lUsedMemory += svPartIds.GetUsedMemory();
	return lUsedMemory;
}

void KWDRPartIdAt::Optimize(KWClass* kwcOwnerClass)
{
	int nIndex;

	require(IsCompiled());

	// Optimisation ancetre
	KWDRPartIndexAt::Optimize(kwcOwnerClass);

	// Calcul des Ids de la partition
	svPartIds.SetSize(dataGridRule->GetAttributePartNumberAt(nDeploymentAttributeIndex));
	for (nIndex = 0; nIndex < svPartIds.GetSize(); nIndex++)
		svPartIds.SetAt(nIndex, KWDRUnivariatePartition::ComputeId(nIndex));
}
