// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIDRInterpretation.h"

void KIDRRegisterInterpretationRules()
{
	KWDerivationRule::RegisterDerivationRule(new KIDRClassifierInterpreter);
	KWDerivationRule::RegisterDerivationRule(new KIDRContributionAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRContributionAttributeAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRContributionValueAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRContributionPartAt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierService

KIDRClassifierService::KIDRClassifierService()
{
	SetType(KWType::Structure);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("Classifier");

	// Initialisation des resultats de compilation
	classifierRule = NULL;
}

KIDRClassifierService::~KIDRClassifierService()
{
	Clean();
}

KWDerivationRule* KIDRClassifierService::Create() const
{
	return new KIDRClassifierService;
}

boolean KIDRClassifierService::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	const KWDRNBClassifier referenceNBRule;
	const KWDRSNBClassifier referenceSNBRule;
	KWDerivationRule* checkedClassifierRule;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// On doit se baser sur un classifieur de type naive bayes
	if (bOk)
	{
		checkedClassifierRule = GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass);
		if (checkedClassifierRule->GetName() != referenceNBRule.GetName() and
		    checkedClassifierRule->GetName() != referenceSNBRule.GetName())
		{
			AddError("First rule operand must be a classifier of type " + referenceNBRule.GetName() +
				 " or " + referenceSNBRule.GetName());
			bOk = false;
		}
	}
	return bOk;
}

void KIDRClassifierService::Compile(KWClass* kwcOwnerClass)
{
	const KWDRDataGridStats* dataGridStatsRule;
	const KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	const KWDRDataGrid* dataGridRule;
	const KWDRDataGridBlock* dataGridBlockRule;
	const KWAttributeBlock* attributeBlock;
	const KWDRUnivariatePartition* univariatePartitionRule;
	KWAttribute* attribute;
	ALString sAttributeName;
	int nVarKey;
	Symbol sVarKey;
	int nDataGridStatsOrBlock;
	int nDataGrid;
	int nAttribute;

	// Nettoyage prealable
	Clean();

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du classifier du premier operande
	classifierRule = cast(const KWDRNBClassifier*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
	assert(classifierRule->IsCompiled());

	// Initialisation des vecteur et tableau de resultats a la bonne taille
	ivDataGridSourceIndexes.SetSize(classifierRule->GetDataGridStatsNumber());
	ivDataGridSourceDefaultIndexes.SetSize(classifierRule->GetDataGridStatsNumber());
	svPredictorAttributeNames.SetSize(classifierRule->GetDataGridStatsNumber());
	oaPredictorAttributeDataGridRules.SetSize(classifierRule->GetDataGridStatsNumber());
	oaPredictorAttributeDataGridStatsRules.SetSize(classifierRule->GetDataGridStatsNumber());

	// Collecte des variables du predicteur et calcul des tables de Shapley
	nAttribute = 0;
	for (nDataGridStatsOrBlock = 0; nDataGridStatsOrBlock < classifierRule->GetDataGridStatsOrBlockNumber();
	     nDataGridStatsOrBlock++)
	{
		// Cas d'une preparation d'une variable seule
		if (classifierRule->IsDataGridStatsAt(nDataGridStatsOrBlock))
		{
			dataGridStatsRule = classifierRule->GetDataGridStatsAt(nDataGridStatsOrBlock);
			assert(dataGridStatsRule->GetFirstOperand()->GetOrigin() ==
			       KWDerivationRuleOperand::OriginAttribute);

			// Extraction du nom et de la grille de preparation
			sAttributeName = dataGridStatsRule->GetSecondOperand()->GetAttributeName();
			dataGridRule =
			    cast(const KWDRDataGrid*,
				 dataGridStatsRule->GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));

			// Memorisation du nom de l'attribut sous forme de Symbol
			svPredictorAttributeNames.SetAt(nAttribute, (Symbol)sAttributeName);

			// Memorisation de la grille de preparation et de ses stats
			oaPredictorAttributeDataGridRules.SetAt(nAttribute, cast(KWDRDataGrid*, dataGridRule));
			oaPredictorAttributeDataGridStatsRules.SetAt(nAttribute,
								     cast(KWDRDataGridStats*, dataGridStatsRule));

			// Incrementation de l'attribut
			nAttribute++;
		}
		// Cas d'une preparation d'un bloc de variable
		else
		{
			dataGridStatsBlockRule = classifierRule->GetDataGridStatsBlockAt(nDataGridStatsOrBlock);

			// Acces au bloc de grilles et au bloc de donnees
			dataGridBlockRule = dataGridStatsBlockRule->GetDataGridBlock();
			attributeBlock = dataGridStatsBlockRule->GetSecondOperand()->GetOriginAttributeBlock();

			// Parcours des grilles du bloc
			for (nDataGrid = 0; nDataGrid < dataGridBlockRule->GetDataGridNumber(); nDataGrid++)
			{
				assert(dataGridBlockRule->GetOperandAt(1 + nDataGrid)->GetOrigin() ==
				       KWDerivationRuleOperand::OriginAttribute);
				dataGridRule = dataGridBlockRule->GetDataGridAt(nDataGrid);
				dataGridStatsRule = dataGridStatsBlockRule->GetDataGridStatsAtBlockIndex(nDataGrid);

				// Recherche de l'attribut correspondant
				if (dataGridBlockRule->GetDataGridVarKeyType() == KWType::Continuous)
				{
					nVarKey = int(floor(dataGridBlockRule->GetContinuousVarKeyAt(nDataGrid) + 0.5));
					attribute = attributeBlock->LookupAttributeByContinuousVarKey(nVarKey);
				}
				else
				{
					sVarKey = dataGridBlockRule->GetSymbolVarKeyAt(nDataGrid);
					attribute = attributeBlock->LookupAttributeBySymbolVarKey(sVarKey);
				}

				// Memorisation du nom de l'attribut sous forme de Symbol
				svPredictorAttributeNames.SetAt(nAttribute, (Symbol)attribute->GetName());

				// Memorisation de la grille de preparation et de ses stats
				oaPredictorAttributeDataGridRules.SetAt(nAttribute, cast(KWDRDataGrid*, dataGridRule));
				oaPredictorAttributeDataGridStatsRules.SetAt(
				    nAttribute, cast(KWDRDataGridStats*, dataGridStatsRule));

				// Acces a la partition univariee de l'attribut source de la grille
				assert(dataGridRule->GetAttributeNumber() == 2);
				univariatePartitionRule = cast(const KWDRUnivariatePartition*,
							       dataGridRule->GetOperandAt(0)->GetDerivationRule());

				// Recherche de l'index source correspondant a la valeur par defaut du bloc
				if (attributeBlock->GetType() == KWType::Continuous)
				{
					ivDataGridSourceDefaultIndexes.SetAt(
					    nAttribute, univariatePartitionRule->GetContinuousPartIndex(
							    attributeBlock->GetContinuousDefaultValue()));
				}
				else
				{
					ivDataGridSourceDefaultIndexes.SetAt(
					    nAttribute, univariatePartitionRule->GetSymbolPartIndex(
							    attributeBlock->GetSymbolDefaultValue()));
				}

				// Incrementation de l'attribut
				nAttribute++;
			}
		}
	}

	// Memorisation des rangs de variables
	// On memorise le rang+1, car 0 correspond a la valeur retournee en cas de cle inexistante
	for (nAttribute = 0; nAttribute < svPredictorAttributeNames.GetSize(); nAttribute++)
		lnkdPredictorAttributeRanks.SetAt(svPredictorAttributeNames.GetAt(nAttribute).GetNumericKey(),
						  (longint)nAttribute + 1);
	ensure(svPredictorAttributeNames.GetSize() == GetPredictorAttributeNumber());
	ensure(oaPredictorAttributeDataGridRules.GetSize() == GetPredictorAttributeNumber());
	ensure(oaPredictorAttributeDataGridStatsRules.GetSize() == GetPredictorAttributeNumber());
}

Object* KIDRClassifierService::ComputeStructureResult(const KWObject* kwoObject) const
{
	// On evalue l'operande correspondant au classifieur
	GetFirstOperand()->GetStructureValue(kwoObject);

	// Calcul du vecteur de index source de grille
	ComputeDataGridSourcesIndexes();
	return (Object*)this;
}

void KIDRClassifierService::WriteDetails(ostream& ost) const
{
	int nTarget;
	int nAttribute;

	require(IsCompiled());

	// Titre
	ost << "# " << GetName() << "\n";

	// Valeur cibles
	ost << " ## Target values\t" << GetTargetValueNumber() << "\n";
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		ost << "\t" << GetTargetValueAt(nTarget) << endl;

	// Variables du predicteur
	ost << " ## Predictor variables\t" << GetPredictorAttributeNumber() << "\n";
	for (nAttribute = 0; nAttribute < GetPredictorAttributeNumber(); nAttribute++)
		WriteAttributeDetails(ost, nAttribute);
}

void KIDRClassifierService::WriteAttributeDetails(ostream& ost, int nAttribute) const
{
	const KWDRDataGrid* dataGridRule;
	KWDataGridStats dataGridStats;
	int i;

	require(IsCompiled());
	require(0 <= nAttribute and nAttribute < GetPredictorAttributeNumber());

	ost << "  ### " << nAttribute + 1 << "\t" << GetPredictorAttributeWeightAt(nAttribute) << "\t"
	    << GetPredictorAttributeNameAt(nAttribute) << "\n";

	// Acces a la regle de grille pour pouvoir visualiser la grille
	dataGridRule = cast(const KWDRDataGrid*, oaPredictorAttributeDataGridRules.GetAt(nAttribute));
	dataGridRule->ExportDataGridStats(&dataGridStats);

	// Affichage de la grille, avec ses dimensions
	ost << "    ####  Data grid\t";
	for (i = 0; i < dataGridStats.GetAttributeNumber(); i++)
	{
		if (i > 0)
			ost << "x";
		ost << dataGridStats.GetAttributeAt(i)->GetPartNumber();
	}
	ost << "\n";
	dataGridStats.WriteFrequencyCrossTable(ost);
}

longint KIDRClassifierService::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KIDRClassifierService) - sizeof(KWDerivationRule);
	lUsedMemory += svPredictorAttributeNames.GetUsedMemory() - sizeof(StringVector);
	lUsedMemory += lnkdPredictorAttributeRanks.GetUsedMemory() - sizeof(LongintNumericKeyDictionary);
	lUsedMemory += oaPredictorAttributeDataGridRules.GetUsedMemory() - sizeof(ObjectArray);
	lUsedMemory += oaPredictorAttributeDataGridStatsRules.GetUsedMemory() - sizeof(ObjectArray);
	lUsedMemory += ivDataGridSourceIndexes.GetUsedMemory() - sizeof(IntVector);
	lUsedMemory += ivDataGridSourceDefaultIndexes.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

void KIDRClassifierService::Clean()
{
	// Nettoyage des structures issuee de la compilation
	classifierRule = NULL;
	svPredictorAttributeNames.SetSize(0);
	lnkdPredictorAttributeRanks.RemoveAll();
	oaPredictorAttributeDataGridRules.RemoveAll();
	oaPredictorAttributeDataGridStatsRules.RemoveAll();
	ivDataGridSourceIndexes.SetSize(0);
	ivDataGridSourceDefaultIndexes.SetSize(0);
}

void KIDRClassifierService::ComputeDataGridSourcesIndexes() const
{
	const KWDRDataGridStats* dataGridStatsRule;
	const KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	int nDataGridStatsOrBlock;
	int nSourceCellIndex;
	int nValue;
	int nDataGrid;
	int nDataGridIndexWithinBlock;

	require(IsCompiled());
	require(ivDataGridSourceIndexes.GetSize() == GetPredictorAttributeNumber());
	require(classifierRule != NULL);

	// On commence a recopier les index par defaut, ce qui permet une prealimentation dans les cas des blocs sparses
	ivDataGridSourceIndexes.CopyFrom(&ivDataGridSourceDefaultIndexes);

	// Calcul des index source par grille
	nDataGrid = 0;
	for (nDataGridStatsOrBlock = 0; nDataGridStatsOrBlock < classifierRule->GetDataGridStatsOrBlockNumber();
	     nDataGridStatsOrBlock++)
	{
		if (classifierRule->IsDataGridStatsAt(nDataGridStatsOrBlock))
		{
			dataGridStatsRule = classifierRule->GetDataGridStatsAt(nDataGridStatsOrBlock);

			// Acces a l'index de la source
			nSourceCellIndex = dataGridStatsRule->GetCellIndex();
			ivDataGridSourceIndexes.SetAt(nDataGrid, nSourceCellIndex);
			nDataGrid++;
		}
		else
		{
			dataGridStatsBlockRule = classifierRule->GetDataGridStatsBlockAt(nDataGridStatsOrBlock);
			for (nValue = 0; nValue < dataGridStatsBlockRule->GetValueNumber(); nValue++)
			{
				// Acces a l'index de la source
				// La source doit etre ajuste a zero par des raisons techiques des DataGridBlocks
				nSourceCellIndex = dataGridStatsBlockRule->GetCellIndexAt(nValue) - 1;
				nDataGridIndexWithinBlock = dataGridStatsBlockRule->GetDataGridIndexAt(nValue);
				ivDataGridSourceIndexes.SetAt(nDataGrid + nDataGridIndexWithinBlock, nSourceCellIndex);
			}
			nDataGrid += dataGridStatsBlockRule->GetDataGridBlock()->GetDataGridNumber();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierInterpreter

KIDRClassifierInterpreter::KIDRClassifierInterpreter()
{
	SetName("ClassifierInterpreter");
	SetLabel("Classifier interpreter");
	SetStructureName("ClassifierInterpreter");

	// Initialisation des resultats de compilation
	bIsRankedContributionComputed = false;
}

KIDRClassifierInterpreter::~KIDRClassifierInterpreter()
{
	// Il faut appeler explicitement la methode dans le destructeur
	// Car meme si la methode est virtuelle, le destructeur ancetre
	// appelle la version de sa propre classe
	Clean();
}

KWDerivationRule* KIDRClassifierInterpreter::Create() const
{
	return new KIDRClassifierInterpreter;
}

void KIDRClassifierInterpreter::Compile(KWClass* kwcOwnerClass)
{
	const boolean bTrace = false;
	SymbolVector svTargetValues;
	IntVector ivTargetValueFrequencies;
	const KWDRDataGrid* dataGridRule;
	KWDataGridStats dataGridStats;
	KIShapleyTable* stShapleyTable;
	int nAttribute;
	int nTarget;

	// Appel de la methode ancetre
	KIDRClassifierService::Compile(kwcOwnerClass);
	assert(classifierRule != NULL);

	// Collecte des valeurs cibles et de leur effectif
	svTargetValues.SetSize(classifierRule->GetTargetValueNumber());
	ivTargetValueFrequencies.SetSize(classifierRule->GetTargetValueNumber());
	for (nTarget = 0; nTarget < svTargetValues.GetSize(); nTarget++)
	{
		svTargetValues.SetAt(nTarget, classifierRule->GetTargetValueAt(nTarget));
		ivTargetValueFrequencies.SetAt(nTarget, classifierRule->GetDataGridSetTargetFrequencyAt(nTarget));
	}

	// Initialisation des vecteur et tableau de resultats a la bonne taille
	oaPredictorAttributeShapleyTables.SetSize(classifierRule->GetDataGridStatsNumber());

	// Calcul des tables de Shapley par variable du predicteur
	for (nAttribute = 0; nAttribute < oaPredictorAttributeDataGridRules.GetSize(); nAttribute++)
	{
		// Grille de preparation
		dataGridRule = cast(const KWDRDataGrid*, oaPredictorAttributeDataGridRules.GetAt(nAttribute));

		// Export de la grille pour pouvoir calculer les valeur de Shapley
		dataGridRule->ExportDataGridStats(&dataGridStats);

		// Calcul de la table de Shapley
		stShapleyTable = new KIShapleyTable;
		oaPredictorAttributeShapleyTables.SetAt(nAttribute, stShapleyTable);
		stShapleyTable->InitializeFromDataGridStats(&svTargetValues, &ivTargetValueFrequencies, &dataGridStats,
							    GetPredictorAttributeWeightAt(nAttribute));
	}

	// Creation des structures de gestion des contributions pour les acces par rang
	CreateRankedContributionStructures(GetTargetValueNumber(), svPredictorAttributeNames.GetSize(),
					   &svPredictorAttributeNames);

	// Trace
	if (bTrace)
		WriteDetails(cout);
	ensure(oaPredictorAttributeShapleyTables.GetSize() == GetPredictorAttributeNumber());
}

Object* KIDRClassifierInterpreter::ComputeStructureResult(const KWObject* kwoObject) const
{
	// Appel de la methode ancetre
	KIDRClassifierService::ComputeStructureResult(kwoObject);

	// On indique que les contributions sont a recalculer si necessaire
	bIsRankedContributionComputed = false;
	return (Object*)this;
}

Continuous KIDRClassifierInterpreter::GetContributionAt(int nTargetValueRank, int nPredictorAttributeRank) const
{
	int nSourceCellIndex;
	Continuous cShapleyValue;

	require(IsCompiled());
	require(0 <= nTargetValueRank and nTargetValueRank < GetTargetValueNumber());
	require(0 <= nPredictorAttributeRank and nPredictorAttributeRank < GetPredictorAttributeNumber());

	// Recheche des index source et cible dans la grille correspondante
	nSourceCellIndex = ivDataGridSourceIndexes.GetAt(nPredictorAttributeRank);

	// Recherche de la valeur de Shapley
	cShapleyValue = GetPredictorAttributeShapleyTableAt(nPredictorAttributeRank)
			    ->GetShapleyValueAt(nSourceCellIndex, nTargetValueRank);
	return cShapleyValue;
}

Symbol KIDRClassifierInterpreter::GetRankedContributionAttributeAt(int nTargetValueRank, int nContributionRank) const
{
	require(IsCompiled());
	require(0 <= nTargetValueRank and nTargetValueRank < GetTargetValueNumber());
	require(0 <= nContributionRank and nContributionRank < GetPredictorAttributeNumber());

	// Calcul des contributions pour des acces par rang
	if (not bIsRankedContributionComputed)
		ComputeRankedContributions();

	// On renvoie le nom de l'attribut correspondant
	return GetRankedContributionAt(nTargetValueRank, nContributionRank)->GetAttributeName();
}

Symbol KIDRClassifierInterpreter::GetRankedContributionPartAt(int nTargetValueRank, int nContributionRank) const
{
	int nAttributeIndex;
	const KWDRDataGrid* dataGridRule;
	int nDataGridSourceIndex;
	KWDataGridStats dataGridStats;
	ALString sCellLabel;

	require(IsCompiled());
	require(0 <= nTargetValueRank and nTargetValueRank < GetTargetValueNumber());
	require(0 <= nContributionRank and nContributionRank < GetPredictorAttributeNumber());

	// Calcul des contributions pour des acces par rang
	if (not bIsRankedContributionComputed)
		ComputeRankedContributions();

	// Acces a l'attribut, la grille corespondant, et l'index source dans la grille
	nAttributeIndex = GetRankedContributionAt(nTargetValueRank, nContributionRank)->GetAttributeIndex();
	dataGridRule = cast(const KWDRDataGrid*, oaPredictorAttributeDataGridRules.GetAt(nAttributeIndex));
	nDataGridSourceIndex = ivDataGridSourceIndexes.GetAt(nAttributeIndex);

	// Acces a la partition univariee de l'attribut pour obtenir le libelle de la partie
	assert(dataGridRule->GetAttributeNumber() == 2);
	sCellLabel = cast(KWDRUnivariatePartition*, dataGridRule->GetOperandAt(0)->GetDerivationRule())
			 ->GetPartLabelAt(nDataGridSourceIndex);
	return (Symbol)sCellLabel;
}

Continuous KIDRClassifierInterpreter::GetRankedContributionValueAt(int nTargetValueRank, int nContributionRank) const
{
	require(IsCompiled());
	require(0 <= nTargetValueRank and nTargetValueRank < GetTargetValueNumber());
	require(0 <= nContributionRank and nContributionRank < GetPredictorAttributeNumber());

	// Calcul des contributions pour des acces par rang
	if (not bIsRankedContributionComputed)
		ComputeRankedContributions();

	// On renvoie la valeur de contribution correspondante
	return GetRankedContributionAt(nTargetValueRank, nContributionRank)->GetContribution();
}

void KIDRClassifierInterpreter::WriteAttributeDetails(ostream& ost, int nAttribute) const
{
	const KIShapleyTable* stShapleyTable;

	require(IsCompiled());

	// Appel de la methode ancetre
	KIDRClassifierService::WriteAttributeDetails(ost, nAttribute);

	// Table de Shapley
	stShapleyTable = GetPredictorAttributeShapleyTableAt(nAttribute);
	ost << "    ####  Shapley values\n";
	ost << *stShapleyTable << "\n";
}

longint KIDRClassifierInterpreter::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KIDRClassifierService::GetUsedMemory();
	lUsedMemory += sizeof(KIDRClassifierInterpreter) - sizeof(KIDRClassifierService);
	lUsedMemory += oaPredictorAttributeShapleyTables.GetOverallUsedMemory() - sizeof(ObjectArray);
	lUsedMemory += oaTargetValueRankedAttributeContributions.GetSize() *
		       (sizeof(ObjectArray) + svPredictorAttributeNames.GetSize() *
						  (sizeof(KIAttributeContribution*) + sizeof(KIAttributeContribution)));
	return lUsedMemory;
}

void KIDRClassifierInterpreter::Clean()
{
	ObjectArray* oaRankedAttributeContributions;
	int nTarget;

	// Methode ancetre
	KIDRClassifierService::Clean();

	// Nettoyage des structures specifiques
	oaPredictorAttributeShapleyTables.DeleteAll();
	for (nTarget = 0; nTarget < oaTargetValueRankedAttributeContributions.GetSize(); nTarget++)
	{
		oaRankedAttributeContributions =
		    cast(ObjectArray*, oaTargetValueRankedAttributeContributions.GetAt(nTarget));
		oaRankedAttributeContributions->DeleteAll();
	}
	oaTargetValueRankedAttributeContributions.DeleteAll();
	bIsRankedContributionComputed = false;
}

void KIDRClassifierInterpreter::CreateRankedContributionStructures(int nTargetValueNumber, int nAttributeNumber,
								   const SymbolVector* svAttributeNames)
{
	ObjectArray* oaRankedAttributeContributions;
	KIAttributeContribution* attributeContribution;
	int nTarget;
	int nAttribute;

	require(nTargetValueNumber > 0);
	require(nAttributeNumber > 0);
	require(svAttributeNames != NULL);
	require(svAttributeNames->GetSize() == nAttributeNumber);
	require(oaTargetValueRankedAttributeContributions.GetSize() == 0);

	// Creation des tableaux par valeur cible
	oaTargetValueRankedAttributeContributions.SetSize(nTargetValueNumber);
	for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
	{
		// Creation du tableau pour la valeur cible
		oaRankedAttributeContributions = new ObjectArray;
		oaTargetValueRankedAttributeContributions.SetAt(nTarget, oaRankedAttributeContributions);

		// Parametrage du tri du tableau
		oaRankedAttributeContributions->SetCompareFunction(KIAttributeContributionCompare);

		// Creation d'une contribution par variable
		oaRankedAttributeContributions->SetSize(nAttributeNumber);
		for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
		{
			attributeContribution = new KIAttributeContribution;
			attributeContribution->SetAttributeNames(svAttributeNames);
			oaRankedAttributeContributions->SetAt(nAttribute, attributeContribution);
		}
	}
}

void KIDRClassifierInterpreter::ComputeRankedContributions() const
{
	const boolean bTrace = false;
	ObjectArray* oaRankedAttributeContributions;
	KIAttributeContribution* attributeContribution;
	int nTarget;
	int nAttribute;
	int nSourceCellIndex;
	Continuous cShapleyValue;

	require(IsCompiled());
	require(ivDataGridSourceIndexes.GetSize() == GetPredictorAttributeNumber());
	require(classifierRule != NULL);
	require(oaTargetValueRankedAttributeContributions.GetSize() == GetTargetValueNumber());
	require(not bIsRankedContributionComputed);

	// Calcul des contributions par valeur cible
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
	{
		oaRankedAttributeContributions =
		    cast(ObjectArray*, oaTargetValueRankedAttributeContributions.GetAt(nTarget));
		assert(oaRankedAttributeContributions->GetSize() == GetPredictorAttributeNumber());

		// Calcul des contribution par attributs
		for (nAttribute = 0; nAttribute < GetPredictorAttributeNumber(); nAttribute++)
		{
			attributeContribution =
			    cast(KIAttributeContribution*, oaRankedAttributeContributions->GetAt(nAttribute));

			// Recheche des index source et cible dans la grille correspondante
			nSourceCellIndex = ivDataGridSourceIndexes.GetAt(nAttribute);

			// Recherche de la valeur de Shapley
			cShapleyValue = GetPredictorAttributeShapleyTableAt(nAttribute)
					    ->GetShapleyValueAt(nSourceCellIndex, nTarget);
			assert(cShapleyValue == GetContributionAt(nTarget, nAttribute));

			// Memorisation de l'index de l'attribut et de sa contribution
			attributeContribution->SetAttributeIndex(nAttribute);
			attributeContribution->SetContribution(cShapleyValue);
		}

		// Tri des contribution
		oaRankedAttributeContributions->Sort();
	}

	// On memorise que la calcul est effectue
	bIsRankedContributionComputed = true;

	// Trace des resultats
	if (bTrace)
	{
		// Affichage par valeur cible
		for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		{
			cout << "Contributions for " << GetTargetValueAt(nTarget) << "\n";

			// Affichage par attributs
			oaRankedAttributeContributions =
			    cast(ObjectArray*, oaTargetValueRankedAttributeContributions.GetAt(nTarget));
			for (nAttribute = 0; nAttribute < oaRankedAttributeContributions->GetSize(); nAttribute++)
			{
				attributeContribution =
				    cast(KIAttributeContribution*, oaRankedAttributeContributions->GetAt(nAttribute));
				cout << "\t" << attributeContribution->GetContribution() << "\t"
				     << attributeContribution->GetAttributeName() << "\n";
			}
		}
	}
}

////////////////////////////////////////////////////////////
// Classe KIDRInterpretationRule

KIDRInterpretationRule::KIDRInterpretationRule()
{
	nConstantTargetValueRank = -1;
	nConstantPredictorAttributeRank = -1;
	nConstantContributionRank = -1;
}

KIDRInterpretationRule::~KIDRInterpretationRule() {}

void KIDRInterpretationRule::Compile(KWClass* kwcOwnerClass)
{
	KIDRClassifierInterpreter* classifierInterpreter;
	Symbol sTargetValue;
	Symbol sPredictorAttributeName;

	require(GetOperandNumber() == 3);
	require(GetOperandAt(1)->GetType() == KWType::Symbol);
	require(GetOperandAt(2)->GetType() == KWType::Symbol or GetOperandAt(2)->GetType() == KWType::Continuous);

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Initialisation des rangs optimises
	nConstantTargetValueRank = -1;
	nConstantPredictorAttributeRank = -1;
	nConstantContributionRank = -1;

	// Acces a l'interpreteur du premier operande
	classifierInterpreter =
	    cast(KIDRClassifierInterpreter*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));

	// Calcul du rang de la valeur cible si le deuxieme operande est constant
	if (GetOperandAt(1)->GetOrigin() == KWDerivationRuleOperand::OriginConstant)
	{
		sTargetValue = GetOperandAt(1)->GetSymbolConstant();
		nConstantTargetValueRank = classifierInterpreter->GetTargetValueRank(sTargetValue);
	}

	// Calcul du rang du second parametre si le troisieme operande est constant
	if (GetOperandAt(2)->GetOrigin() == KWDerivationRuleOperand::OriginConstant)
	{
		// Rang d'un attribut du predicteur
		if (GetOperandAt(2)->GetType() == KWType::Symbol)
		{
			sPredictorAttributeName = GetOperandAt(2)->GetSymbolConstant();
			nConstantPredictorAttributeRank =
			    classifierInterpreter->GetPredictorAttributeRank(sPredictorAttributeName);
		}
		// Rang de la variable de contribution
		else
		{
			assert(GetOperandAt(2)->GetType() == KWType::Continuous);
			nConstantContributionRank = (int)floor(GetOperandAt(2)->GetContinuousConstant() - 0.5);
		}
	}
}

////////////////////////////////////////////////////////////
// Classe KIDRContributionAt

KIDRContributionAt::KIDRContributionAt()
{
	SetName("ContributionAt");
	SetLabel("Variable contribution");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierInterpreter");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Symbol);
}

KIDRContributionAt::~KIDRContributionAt() {}

KWDerivationRule* KIDRContributionAt::Create() const
{
	return new KIDRContributionAt;
}

Continuous KIDRContributionAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierInterpreter* classifierInterpreter;
	int nTargetValueRank;
	int nPredictorAttributeRank;

	// Acces a l'interpreteur
	classifierInterpreter = cast(KIDRClassifierInterpreter*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Calcul des parametres, uniquement si necessaire
	nTargetValueRank = nConstantTargetValueRank;
	if (nTargetValueRank == -1)
		nTargetValueRank =
		    classifierInterpreter->GetTargetValueRank(GetOperandAt(1)->GetSymbolValue(kwoObject));
	nPredictorAttributeRank = nConstantPredictorAttributeRank;
	if (nPredictorAttributeRank == -1)
		nPredictorAttributeRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);

	// Contribution si valide
	if (nTargetValueRank >= 0 and nPredictorAttributeRank >= 0)
		return classifierInterpreter->GetContributionAt(nTargetValueRank, nPredictorAttributeRank);
	// Valeur manquante sinon
	else
		return KWContinuous::GetMissingValue();
}

////////////////////////////////////////////////////////////
// Classe KIDRContributionAttributeAt

KIDRContributionAttributeAt::KIDRContributionAttributeAt()
{
	SetName("ContributionVariableAt");
	SetLabel("Contribution variable name at");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierInterpreter");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRContributionAttributeAt::~KIDRContributionAttributeAt() {}

KWDerivationRule* KIDRContributionAttributeAt::Create() const
{
	return new KIDRContributionAttributeAt;
}

Symbol KIDRContributionAttributeAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierInterpreter* classifierInterpreter;
	int nTargetValueRank;
	int nContributionRank;

	// Acces a l'interpreteur
	classifierInterpreter = cast(KIDRClassifierInterpreter*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Calcul des parametres, uniquement si necessaire
	nTargetValueRank = nConstantTargetValueRank;
	if (nTargetValueRank == -1)
		nTargetValueRank =
		    classifierInterpreter->GetTargetValueRank(GetOperandAt(1)->GetSymbolValue(kwoObject));
	nContributionRank = nConstantContributionRank;
	if (nContributionRank == -1)
		nContributionRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);

	// Attribut de contribution si valide
	if (nTargetValueRank >= 0 and nContributionRank >= 0)
		return classifierInterpreter->GetRankedContributionAttributeAt(nTargetValueRank, nContributionRank);
	// Valeur manquante sinon
	else
		return Symbol();
}

////////////////////////////////////////////////////////////
// Classe KIDRContributionPartAt

KIDRContributionPartAt::KIDRContributionPartAt()
{
	SetName("ContributionPartAt");
	SetLabel("Contribution variable part at");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierInterpreter");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRContributionPartAt::~KIDRContributionPartAt() {}

KWDerivationRule* KIDRContributionPartAt::Create() const
{
	return new KIDRContributionPartAt;
}

Symbol KIDRContributionPartAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierInterpreter* classifierInterpreter;
	int nTargetValueRank;
	int nContributionRank;

	// Acces a l'interpreteur
	classifierInterpreter = cast(KIDRClassifierInterpreter*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Calcul des parametres, uniquement si necessaire
	nTargetValueRank = nConstantTargetValueRank;
	if (nTargetValueRank == -1)
		nTargetValueRank =
		    classifierInterpreter->GetTargetValueRank(GetOperandAt(1)->GetSymbolValue(kwoObject));
	nContributionRank = nConstantContributionRank;
	if (nContributionRank == -1)
		nContributionRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);

	// Partie de l'attribut de contribution si valide
	if (nTargetValueRank >= 0 and nContributionRank >= 0)
		return classifierInterpreter->GetRankedContributionPartAt(nTargetValueRank, nContributionRank);
	// Valeur manquante sinon
	else
		return Symbol();
}

////////////////////////////////////////////////////////////
// Classe KIDRContributionValueAt

KIDRContributionValueAt::KIDRContributionValueAt()
{
	SetName("ContributionValueAt");
	SetLabel("Contribution value at");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierInterpreter");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRContributionValueAt::~KIDRContributionValueAt() {}

KWDerivationRule* KIDRContributionValueAt::Create() const
{
	return new KIDRContributionValueAt;
}

Continuous KIDRContributionValueAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierInterpreter* classifierInterpreter;
	int nTargetValueRank;
	int nContributionRank;

	// Acces a l'interpreteur
	classifierInterpreter = cast(KIDRClassifierInterpreter*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Calcul des parametres, uniquement si necessaire
	nTargetValueRank = nConstantTargetValueRank;
	if (nTargetValueRank == -1)
		nTargetValueRank =
		    classifierInterpreter->GetTargetValueRank(GetOperandAt(1)->GetSymbolValue(kwoObject));
	nContributionRank = nConstantContributionRank;
	if (nContributionRank == -1)
		nContributionRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);

	// Valeur de contribution si valide
	if (nTargetValueRank >= 0 and nContributionRank >= 0)
		return classifierInterpreter->GetRankedContributionValueAt(nTargetValueRank, nContributionRank);
	// Valeur manquante sinon
	else
		return KWContinuous::GetMissingValue();
}

////////////////////////////////////////////////////////////
// Classe KIAttributeContribution

KIAttributeContribution::KIAttributeContribution()
{
	nAttributeIndex = 0;
	cContribution = 0;
	svAttributeNames = NULL;
}

KIAttributeContribution::~KIAttributeContribution() {}

void KIAttributeContribution::SetAttributeNames(const SymbolVector* svNames)
{
	svAttributeNames = svNames;
}

const SymbolVector* KIAttributeContribution::GetAttributeNames() const
{
	return svAttributeNames;
}

int KIAttributeContributionCompare(const void* elem1, const void* elem2)
{
	int nCompare;
	KIAttributeContribution* attributeContribution1;
	KIAttributeContribution* attributeContribution2;

	// Acces aux objets
	attributeContribution1 = cast(KIAttributeContribution*, *(Object**)elem1);
	attributeContribution2 = cast(KIAttributeContribution*, *(Object**)elem2);

	// Comparaison selon la precision du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(attributeContribution1->GetContribution(),
							attributeContribution2->GetContribution());

	// Comparaison sur le nom de l'attribut en cas d'egalite
	// Attention a prendre la valeur du Symbol contenant le nom
	if (nCompare == 0)
		nCompare =
		    attributeContribution1->GetAttributeName().CompareValue(attributeContribution2->GetAttributeName());
	return nCompare;
}
