// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBPredictorSelectionDataCostCalculator.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBTargetPart

SNBTargetPart::SNBTargetPart() {}

SNBTargetPart::~SNBTargetPart() {}

ContinuousVector* SNBTargetPart::GetScores()
{
	return &cvScores;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBPredictorSelectionDataCostCalculator

SNBPredictorSelectionDataCostCalculator::SNBPredictorSelectionDataCostCalculator()
{
	binarySliceSet = NULL;
	dInstanceNumber = 0.0;
	cMaxScore = DBL_MAX;
	dMaxExpScore = DBL_MAX;
	dLaplaceEpsilon = 0.0;
	dLaplaceDenominator = 1.0;
	dSelectionDataCost = DBL_MAX;
	lastModificationAttribute = NULL;
	bLastModificationWasIncrease = false;
	dLastModificationSelectionDataCost = 0.0;
	cLastModificationDeltaWeight = 0.0;
	bLastModificationUpdatedTargetPartition = false;
}

SNBPredictorSelectionDataCostCalculator::~SNBPredictorSelectionDataCostCalculator() {}

void SNBPredictorSelectionDataCostCalculator::SetDataTableBinarySliceSet(
    SNBDataTableBinarySliceSet* dataTableBinarySliceSet)
{
	require(dataTableBinarySliceSet != NULL);
	binarySliceSet = dataTableBinarySliceSet;
}

SNBDataTableBinarySliceSet* SNBPredictorSelectionDataCostCalculator::GetDataTableBinarySliceSet() const
{
	return binarySliceSet;
}

boolean SNBPredictorSelectionDataCostCalculator::Create()
{
	boolean bOk = true;

	require(GetDataTableBinarySliceSet() != NULL);
	require(GetDataTableBinarySliceSet()->IsInitialized());
	require(GetDataTableBinarySliceSet()->Check());

	bOk = bOk and dvInstanceNonNormalizedDataCosts.SetLargeSize(
			  GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber());

	return bOk;
}

void SNBPredictorSelectionDataCostCalculator::Initialize()
{
	require(GetDataTableBinarySliceSet() != NULL);
	require(GetDataTableBinarySliceSet()->Check());
	require(GetDataTableBinarySliceSet()->IsInitialized());

	// Initialisation des estructures qui gerent la partition de la variable cible
	InitializeTargetPartition();

	// Initialisation des variables necessaire pour le calcul du cout de selection
	InitializeDataCostState();

	ensure(Check());
}

double SNBPredictorSelectionDataCostCalculator::GetSelectionDataCost() const
{
	return dSelectionDataCost;
}

boolean SNBPredictorSelectionDataCostCalculator::IncreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute,
									 Continuous cDeltaWeight,
									 boolean bUpdateTargetPartition)
{
	boolean bOk = true;

	require(attribute != NULL);
	require(cDeltaWeight >= 0);
	require(GetDataTableBinarySliceSet() != NULL);
	require(GetDataTableBinarySliceSet()->IsInitialized());
	require(GetDataTableBinarySliceSet()->ContainsAttribute(attribute));
	require(Check());

	// Memorisation de l'etat pour le undo
	lastModificationAttribute = attribute;
	bLastModificationWasIncrease = true;
	cLastModificationDeltaWeight = cDeltaWeight;
	dLastModificationSelectionDataCost = dSelectionDataCost;
	bLastModificationUpdatedTargetPartition = bUpdateTargetPartition;
	dvLastModificationInstanceNonNormalizedDataCosts.CopyFrom(&dvInstanceNonNormalizedDataCosts);

	// Mise-a-jour de l'etat de la calculatrice
	if (bUpdateTargetPartition)
		UpdateTargetPartitionWithAddedAttribute(attribute);
	bOk = bOk and UpdateTargetPartScoresWithWeightedAttribute(attribute, cDeltaWeight);
	if (attribute->IsSparse())
		bOk = bOk and UpdateDataCostWithSparseAttribute(attribute);
	else
		bOk = bOk and UpdateDataCost();

	// Annule le undo si echec
	if (not bOk)
		lastModificationAttribute = NULL;

	ensure(not bOk or IsUndoAllowed());
	return bOk;
}

boolean SNBPredictorSelectionDataCostCalculator::DecreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute,
									 Continuous cDeltaWeight,
									 boolean bUpdateTargetPartition)
{
	boolean bOk = true;

	require(attribute != NULL);
	require(cDeltaWeight >= 0);
	require(GetDataTableBinarySliceSet() != NULL);
	require(GetDataTableBinarySliceSet()->IsInitialized());
	require(GetDataTableBinarySliceSet()->ContainsAttribute(attribute));
	require(Check());

	// Memorisation de l'etat pour le undo
	lastModificationAttribute = attribute;
	bLastModificationWasIncrease = false;
	cLastModificationDeltaWeight = cDeltaWeight;
	dLastModificationSelectionDataCost = dSelectionDataCost;
	bLastModificationUpdatedTargetPartition = bUpdateTargetPartition;
	dvLastModificationInstanceNonNormalizedDataCosts.CopyFrom(&dvInstanceNonNormalizedDataCosts);

	// Mise-a-jour de l'etat de la calculatrice
	bOk = bOk and UpdateTargetPartScoresWithWeightedAttribute(attribute, -cDeltaWeight);
	if (bUpdateTargetPartition)
		UpdateTargetPartitionWithRemovedAttribute(attribute);
	if (attribute->IsSparse())
		bOk = bOk and UpdateDataCostWithSparseAttribute(attribute);
	else
		bOk = bOk and UpdateDataCost();

	// Annule le undo si echec
	if (not bOk)
		lastModificationAttribute = NULL;

	ensure(not bOk or IsUndoAllowed());
	return bOk;
}

boolean SNBPredictorSelectionDataCostCalculator::UndoLastModification()
{
	boolean bOk = true;

	require(IsUndoAllowed());
	require(cLastModificationDeltaWeight >= 0.0);

	// Remise a l'etat d'avant de la derniere modification
	dSelectionDataCost = dLastModificationSelectionDataCost;
	dvInstanceNonNormalizedDataCosts.CopyFrom(&dvLastModificationInstanceNonNormalizedDataCosts);
	if (bLastModificationWasIncrease)
	{
		bOk = bOk and UpdateTargetPartScoresWithWeightedAttribute(lastModificationAttribute,
									  -cLastModificationDeltaWeight);
		if (bLastModificationUpdatedTargetPartition)
			UpdateTargetPartitionWithRemovedAttribute(lastModificationAttribute);
	}
	else
	{
		if (bLastModificationUpdatedTargetPartition)
			UpdateTargetPartitionWithAddedAttribute(lastModificationAttribute);
		bOk = bOk and UpdateTargetPartScoresWithWeightedAttribute(lastModificationAttribute,
									  cLastModificationDeltaWeight);
	}

	// Remise a zero des variables pour defaire la derniere modification
	lastModificationAttribute = NULL;
	cLastModificationDeltaWeight = 0.0;
	dLastModificationSelectionDataCost = 0.0;
	bLastModificationUpdatedTargetPartition = false;
	dvLastModificationInstanceNonNormalizedDataCosts.Initialize();

	ensure(not IsUndoAllowed());
	return bOk;
}

SNBTargetPart* SNBPredictorSelectionDataCostCalculator::GetOrCreatePart()
{
	SNBTargetPart* targetPart;
	boolean bAllocOk;

	require(GetDataTableBinarySliceSet() != NULL);
	require(GetDataTableBinarySliceSet()->Check());
	require(GetDataTableBinarySliceSet()->IsInitialized());

	// S'il y a un objet SNBTargetPart disponible dans le cache on le retourne
	if (olReleasedPartsCache.GetCount() > 0)
		targetPart = cast(SNBTargetPart*, olReleasedPartsCache.RemoveHead());
	// Sinon initialisation du vecteur de scores de la SNBTargetPart; on renvoie NULL si l'allocation echoue
	else
	{
		targetPart = CreatePart();
		bAllocOk = targetPart->GetScores()->SetLargeSize(
		    GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber());
		if (not bAllocOk)
		{
			delete targetPart;
			targetPart = NULL;
		}
	}
	ensure(targetPart->GetScores()->GetSize() == GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber());
	return targetPart;
}

void SNBPredictorSelectionDataCostCalculator::ReleasePart(SNBTargetPart* targetPart)
{
	require(targetPart != NULL);

	// On garde dans le cache la SNBTargetPart eliminee pour une utilisation ulteriure si besoin
	olReleasedPartsCache.AddHead(targetPart);
}

int SNBPredictorSelectionDataCostCalculator::GetTargetValueNumber() const
{
	const KWDGSAttributeSymbolValues* targetValues;

	require(GetTargetValueStats()->GetAttributeNumber() == 1);
	require(GetTargetValueStats()->GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
	require(GetTargetValueStats()->GetAttributeAt(0)->ArePartsSingletons());

	targetValues = cast(const KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0));
	return targetValues->GetPartNumber();
}

Symbol& SNBPredictorSelectionDataCostCalculator::GetTargetValueAt(int nTargetValue) const
{
	const KWDGSAttributeSymbolValues* targetValues;

	require(0 <= nTargetValue and nTargetValue < GetTargetValueNumber());

	targetValues = cast(const KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0));

	return targetValues->GetValueAt(nTargetValue);
}

int SNBPredictorSelectionDataCostCalculator::GetTargetValueFrequencyAt(int nTargetValue) const
{
	require(0 <= nTargetValue and nTargetValue < GetTargetValueNumber());
	return GetTargetValueStats()->GetUnivariateCellFrequencyAt(nTargetValue);
}

boolean SNBPredictorSelectionDataCostCalculator::IsUndoAllowed()
{
	return lastModificationAttribute != NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBSingletonTargetPart

SNBSingletonTargetPart::SNBSingletonTargetPart()
{
	nTargetValue = -1;
}

SNBSingletonTargetPart::~SNBSingletonTargetPart() {}

void SNBSingletonTargetPart::SetTargetValueIndex(int nIndex)
{
	nTargetValue = nIndex;
}

int SNBSingletonTargetPart::GetTargetValueIndex() const
{
	return nTargetValue;
}

void SNBSingletonTargetPart::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Singleton part target value indexes\n";
}

void SNBSingletonTargetPart::WriteLineReport(ostream& ost) const
{
	ost << nTargetValue << "\n";
}

longint SNBSingletonTargetPart::ComputeNecessaryMemory(int nInstanceNumber)
{
	return sizeof(SNBSingletonTargetPart) + (longint)nInstanceNumber * sizeof(Continuous);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBClassifierSelectionDataCostCalculator

SNBClassifierSelectionDataCostCalculator::SNBClassifierSelectionDataCostCalculator() {}

SNBClassifierSelectionDataCostCalculator::~SNBClassifierSelectionDataCostCalculator()
{
	Delete();
}

boolean SNBClassifierSelectionDataCostCalculator::Create()
{
	boolean bOk = true;
	int nTargetPart;
	SNBTargetPart* targetPart;

	require(learningSpec->GetTargetAttributeType() == KWType::Symbol);
	require(oaTargetParts.GetSize() == 0);

	// Appel a la methode ancetre
	bOk = bOk and SNBPredictorSelectionDataCostCalculator::Create();

	// Creation d'autant de vecteurs de probabilites conditionnelles que de classes cibles
	bOk = oaTargetParts.SetLargeSize(GetTargetDescriptiveStats()->GetValueNumber());
	if (bOk)
	{
		for (nTargetPart = 0; nTargetPart < GetTargetDescriptiveStats()->GetValueNumber(); nTargetPart++)
		{
			// Memorisation si allocation reussie
			targetPart = GetOrCreatePart();
			if (targetPart != NULL)
				oaTargetParts.SetAt(nTargetPart, targetPart);
			// Sinon nettoyage et echec
			else
			{
				oaTargetParts.DeleteAll();
				bOk = false;
				break;
			}
		}
	}
	ensure(not bOk or Check());
	return bOk;
}

void SNBClassifierSelectionDataCostCalculator::Delete()
{
	oaTargetParts.DeleteAll();
	olReleasedPartsCache.DeleteAll();
}

void SNBClassifierSelectionDataCostCalculator::InitializeTargetPartition()
{
	const boolean bLocalTrace = false;
	int nTargetValue;
	SNBSingletonTargetPart* targetPart;
	double dTargetAbsoluteProb;
	Continuous cTargetPartEmptyDataCost;
	int nChunkInstance;

	require(GetTargetAttributeType() == KWType::Symbol);
	require(oaTargetParts.GetSize() == GetTargetDescriptiveStats()->GetValueNumber());

	// Calcul du cout de la selection vide pour la modalite cible courante et memorisation pour chaque instance
	// Le cout de selection vide pour la modalite j est tout simplement le a priori log P(Y_j)
	for (nTargetValue = 0; nTargetValue < GetTargetDescriptiveStats()->GetValueNumber(); nTargetValue++)
	{
		assert(GetTargetValueStats()->GetUnivariateCellFrequencyAt(nTargetValue) > 0);

		targetPart = cast(SNBSingletonTargetPart*, oaTargetParts.GetAt(nTargetValue));
		targetPart->SetTargetValueIndex(nTargetValue);
		dTargetAbsoluteProb =
		    GetTargetValueStats()->GetUnivariateCellFrequencyAt(nTargetValue) * 1.0 / GetInstanceNumber();
		cTargetPartEmptyDataCost = log(dTargetAbsoluteProb);
		for (nChunkInstance = 0;
		     nChunkInstance < GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber();
		     nChunkInstance++)
			targetPart->GetScores()->SetAt(nChunkInstance, cTargetPartEmptyDataCost);
	}
}

void SNBClassifierSelectionDataCostCalculator::InitializeDataCostState()
{
	int nTargetValueNumber;
	int nChunkInstance;
	double dInstanceNonNormalizedDataCost;

	// Calcul d'un epsilon de Laplace pour la gestion des petites probabilites
	// En effet, cette normalisation est faite en univarie, mais le probleme peut
	// a nouveau se poser en multivarie du fait de la multiplication de probabilites
	// En univarie, l'epsilon est tres petit (e = 1/InstanceNumber), car on considere
	// que l'estimation MODL est de bonne qualite.
	// En multivarie, on se contente du "proche du classique" e = 0.5/J,
	// ce qui permet d'avoir un denominateur en N+0.5 quelque soit le nombre J de classes
	nTargetValueNumber = GetTargetDescriptiveStats()->GetValueNumber();
	dInstanceNumber = (double)GetInstanceNumber();
	dLaplaceEpsilon = 0.5 / nTargetValueNumber;

	// Prise en compte de l'epsilon pour une probabilite p
	//   p = p*N / N
	//   p_Laplace = (p*N + e)/(N + J*e)
	// Precalcul une fois pour toutes du denominateur
	dLaplaceDenominator = dInstanceNumber + dLaplaceEpsilon * nTargetValueNumber;

	// Calcul des seuils de score max pour eviter les exponentielles infinies (vis a vis de la precision machine)
	dMaxExpScore = DBL_MAX / dInstanceNumber;
	cMaxScore = (Continuous)log(dMaxExpScore);

	// Calcul du couts de la selection vide
	dSelectionDataCost = 0.0;
	for (nChunkInstance = 0; nChunkInstance < GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber();
	     nChunkInstance++)
	{
		dInstanceNonNormalizedDataCost = ComputeInstanceNonNormalizedDataCost(nChunkInstance);
		dvInstanceNonNormalizedDataCosts.SetAt(nChunkInstance, dInstanceNonNormalizedDataCost);
		dSelectionDataCost += dInstanceNonNormalizedDataCost;
	}
	dSelectionDataCost +=
	    GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber() * log(dLaplaceDenominator);

	// Initialisation des variables pour defaire une modification
	lastModificationAttribute = NULL;
	bLastModificationWasIncrease = false;
	cLastModificationDeltaWeight = 0.0;
	dLastModificationSelectionDataCost = 0.0;
	dvLastModificationInstanceNonNormalizedDataCosts.CopyFrom(&dvInstanceNonNormalizedDataCosts);
}

void SNBClassifierSelectionDataCostCalculator::UpdateTargetPartitionWithAddedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute)
{
	require(attribute != NULL);
	require(GetDataTableBinarySliceSet() != NULL);
	require(Check());

	// Pas d'impact sur la structure de la partition cible pour un classifieur normal
}

void SNBClassifierSelectionDataCostCalculator::UpdateTargetPartitionWithRemovedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute)
{
	require(attribute != NULL);
	require(GetDataTableBinarySliceSet() != NULL);
	require(Check());

	// Pas d'impact sur la structure de la partition cible pour un classifieur normal
}

boolean SNBClassifierSelectionDataCostCalculator::UpdateTargetPartScoresWithWeightedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute, Continuous cDeltaWeight)
{
	boolean bOk = true;
	SNBDataTableBinarySliceSetColumn* chunkColumn;
	int nTargetPart;
	SNBTargetPart* targetPart;
	int nColumnValueNumber;
	int nColumnValueIndex;
	int nChunkInstance;
	int nChunkInstanceValue;
	Continuous cLogProb;

	// Acces a la colonne de donnees de l'attribut
	chunkColumn = NULL;
	bOk = bOk and GetDataTableBinarySliceSet()->GetAttributeColumnView(attribute, chunkColumn);

	// Mise a jour des scores des parties cibles
	if (bOk)
	{
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
		{
			targetPart = cast(SNBTargetPart*, oaTargetParts.GetAt(nTargetPart));

			// Mise a jour des scores de la partie cible courante seulement pour les valeurs presentes de la colonne sparse
			if (chunkColumn->IsSparse())
			{
				nColumnValueNumber = chunkColumn->GetValueNumber();
				for (nColumnValueIndex = 0; nColumnValueIndex < nColumnValueNumber; nColumnValueIndex++)
				{
					nChunkInstance = chunkColumn->GetSparseValueInstanceIndexAt(nColumnValueIndex);
					nChunkInstanceValue = chunkColumn->GetSparseValueAt(nColumnValueIndex);
					cLogProb =
					    attribute->GetLnSourceConditionalProb(nChunkInstanceValue, nTargetPart);
					targetPart->GetScores()->UpgradeAt(nChunkInstance, cDeltaWeight * cLogProb);
				}
			}
			// Mise a jour des scores de la partie cible courante seulement pour les valeurs non manquantes (index > -1)
			else
			{
				nColumnValueNumber = chunkColumn->GetValueNumber();
				for (nChunkInstance = 0; nChunkInstance < nColumnValueNumber; nChunkInstance++)
				{
					nChunkInstanceValue = chunkColumn->GetDenseValueAt(nChunkInstance);
					cLogProb =
					    attribute->GetLnSourceConditionalProb(nChunkInstanceValue, nTargetPart);
					targetPart->GetScores()->UpgradeAt(nChunkInstance, cDeltaWeight * cLogProb);
				}
			}
		}
	}
	return bOk;
}

boolean SNBClassifierSelectionDataCostCalculator::UpdateDataCost()
{
	const boolean bLocalTrace = false;
	boolean bOk = true;
	int nTargetPart;
	int nChunkInstanceNumber;
	int nChunkInstance;
	double dInstanceNonNormalizedDataCost;

	// Entete de trace de debbogage
	if (bLocalTrace)
	{
		cout << "\t[D]Instance\tTarget\tTargetPart\tProb\tCost";
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
			cout << "\tScore" << nTargetPart + 1;
		cout << "\n";
	}

	// Mise-a-jour du score en recalculant tous les couts par instance
	dSelectionDataCost = 0.0;
	nChunkInstanceNumber = GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber();
	for (nChunkInstance = 0; nChunkInstance < nChunkInstanceNumber; nChunkInstance++)
	{
		dInstanceNonNormalizedDataCost = ComputeInstanceNonNormalizedDataCost(nChunkInstance);
		dSelectionDataCost += dInstanceNonNormalizedDataCost;
		dvInstanceNonNormalizedDataCosts.SetAt(nChunkInstance, dInstanceNonNormalizedDataCost);
	}
	dSelectionDataCost += nChunkInstanceNumber * log(dLaplaceDenominator);

	return bOk;
}

boolean SNBClassifierSelectionDataCostCalculator::UpdateDataCostWithSparseAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute)
{
	const boolean bLocalTrace = false;
	boolean bOk = true;
	int nTargetPart;
	int nChunkColumnValueNumber;
	int nChunkColumnValue;
	int nChunkInstance;
	SNBDataTableBinarySliceSetColumn* chunkColumn;
	double dOldInstanceNonNormalizedDataCost;
	double dInstanceNonNormalizedDataCost;

	require(attribute != NULL);
	require(attribute->IsSparse());
	require(GetDataTableBinarySliceSet()->ContainsAttribute(attribute));

	// Entete de trace de debbogage
	if (bLocalTrace)
	{
		cout << attribute->GetNativeAttributeName() << "\t[S]Instance\tTarget\tTargetPart\tProb\tCost";
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
			cout << "\tScore" << nTargetPart + 1;
		cout << "\n";
	}

	// Mise-a-jour du score en calculant seulement le delta dans les instances non-manquantes
	// Nota: Pas necessaire de normaliser avec le denominateur de Laplace car on ne recommence pas de
	// zero comme dans le cas dense
	chunkColumn = NULL;
	bOk = bOk and GetDataTableBinarySliceSet()->GetAttributeColumnView(attribute, chunkColumn);
	if (bOk)
	{
		nChunkColumnValueNumber = chunkColumn->GetValueNumber();
		for (nChunkColumnValue = 0; nChunkColumnValue < nChunkColumnValueNumber; nChunkColumnValue++)
		{
			nChunkInstance = chunkColumn->GetSparseValueInstanceIndexAt(nChunkColumnValue);
			dOldInstanceNonNormalizedDataCost = dvInstanceNonNormalizedDataCosts.GetAt(nChunkInstance);
			dInstanceNonNormalizedDataCost = ComputeInstanceNonNormalizedDataCost(nChunkInstance);
			dSelectionDataCost += dInstanceNonNormalizedDataCost - dOldInstanceNonNormalizedDataCost;
			dvInstanceNonNormalizedDataCosts.SetAt(nChunkInstance, dInstanceNonNormalizedDataCost);
		}
	}
	return bOk;
}

SNBTargetPart* SNBClassifierSelectionDataCostCalculator::CreatePart()
{
	return new SNBSingletonTargetPart;
}

boolean SNBClassifierSelectionDataCostCalculator::Check() const
{
	boolean bOk = true;
	ALString sTmp;

	if (GetLearningSpec() == NULL)
	{
		AddError("Learning specification is NULL");
		bOk = false;
	}

	if (bOk and GetLearningSpec()->GetTargetAttributeType() != KWType::Symbol)
	{
		AddError("Type of the targe variable is not 'Categorical'");
		bOk = false;
	}

	// Si la base est vide, il ne doit rien avoir
	if (bOk and GetInstanceNumber() == 0)
	{
		if (oaTargetParts.GetSize() > 0)
		{
			AddError("Database is empty but the partition is not");
			bOk = false;
		}
	}
	// Verification de la structure sinon
	else if (bOk and GetInstanceNumber() > 0)
	{
		// Verification du nombre de parties
		if (oaTargetParts.GetSize() != GetTargetDescriptiveStats()->GetValueNumber())
		{
			AddError(sTmp + "Number of target parts (" + IntToString(oaTargetParts.GetSize()) +
				 ") is not equal to the number of class values (" +
				 IntToString(GetTargetDescriptiveStats()->GetValueNumber()) + ")");
			bOk = false;
		}
		else
			bOk = CheckParts();
	}
	return bOk;
}

boolean SNBClassifierSelectionDataCostCalculator::CheckParts() const
{
	boolean bOk = true;
	ALString sTmp = "";
	int nTarget;
	SNBTargetPart* targetPart;

	for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
	{
		targetPart = cast(SNBTargetPart*, oaTargetParts.GetAt(nTarget));
		if (targetPart == NULL)
		{
			AddError(sTmp + "Part at index " + IntToString(nTarget) + " is NULL");
			bOk = false;
		}

		if (bOk and targetPart->GetScores()->GetSize() !=
				GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber())
		{
			AddError(sTmp + "Size of target part probability vector (" +
				 IntToString(targetPart->GetScores()->GetSize()) +
				 ") is not equal to the sub-database size (" +
				 IntToString(GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber()) + ")");
			bOk = false;
		}
	}
	return bOk;
}

void SNBClassifierSelectionDataCostCalculator::Write(ostream& ost) const
{
	int nTarget;
	SNBTargetPart* targetPart;

	require(Check());

	for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
	{
		targetPart = cast(SNBTargetPart*, oaTargetParts.GetAt(nTarget));
		if (nTarget == 0)
		{
			targetPart->WriteHeaderLineReport(ost);
			ost << "\n";
		}
		targetPart->WriteLineReport(ost);
		ost << "\n";
	}
}

longint SNBClassifierSelectionDataCostCalculator::ComputeNecessaryMemory(int nInstanceNumber, int nTargetPartNumber)
{
	ObjectArray oaDummy;
	ObjectList olDummy;

	// Formule de l'estimation :
	//   Objets de l'instance +
	//   Max d'objects SNBGroupTargetPart's +
	//   Max overhead des conteneurs des SNBGroupTargetPart's (oaTargetParts & olReleasedPartsCache)
	return sizeof(SNBClassifierSelectionDataCostCalculator) +
	       nTargetPartNumber * SNBSingletonTargetPart::ComputeNecessaryMemory(nInstanceNumber) +
	       nTargetPartNumber * (oaDummy.GetUsedMemoryPerElement() + olDummy.GetUsedMemoryPerElement());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBIntervalTargetPart

SNBIntervalTargetPart::SNBIntervalTargetPart()
{
	nFrequency = 0;
	nCumulativeFrequency = 0;
	nRefCount = 0;
}

SNBIntervalTargetPart::~SNBIntervalTargetPart() {}

void SNBIntervalTargetPart::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

int SNBIntervalTargetPart::GetFrequency() const
{
	return nFrequency;
}

void SNBIntervalTargetPart::SetCumulativeFrequency(int nValue)
{
	require(nValue >= 0);
	nCumulativeFrequency = nValue;
}

int SNBIntervalTargetPart::GetCumulativeFrequency() const
{
	return nCumulativeFrequency;
}

void SNBIntervalTargetPart::SetRefCount(int nValue)
{
	require(nValue >= 0);
	nRefCount = nValue;
}

inline int SNBIntervalTargetPart::GetRefCount() const
{
	return nRefCount;
}

void SNBIntervalTargetPart::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Frequency\t"
	    << "CumulativeFrequency\t"
	    << "RefCount";
}

void SNBIntervalTargetPart::WriteLineReport(ostream& ost) const
{
	ost << nFrequency << "\t" << nCumulativeFrequency << "\t" << nRefCount;
}

longint SNBIntervalTargetPart::ComputeNecessaryMemory(int nInstanceNumber)
{
	return sizeof(SNBIntervalTargetPart) + (longint)nInstanceNumber * sizeof(Continuous);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBRegressorSelectionDataCostCalculator

SNBRegressorSelectionDataCostCalculator::SNBRegressorSelectionDataCostCalculator() {}

SNBRegressorSelectionDataCostCalculator::~SNBRegressorSelectionDataCostCalculator()
{
	Delete();
}

boolean SNBRegressorSelectionDataCostCalculator::Create()
{
	boolean bOk;
	SNBIntervalTargetPart* targetPart;

	require(GetLearningSpec() != NULL);
	require(GetLearningSpec()->GetTargetAttributeType() == KWType::Continuous);
	require(olTargetPartition.GetCount() == 0);

	// Appel a la methode ancetre
	bOk = SNBPredictorSelectionDataCostCalculator::Create();

	// Creation d'un vecteur de probabilites conditionnelles pour un unique intervalle cible initial
	bOk = true;
	targetPart = cast(SNBIntervalTargetPart*, GetOrCreatePart());
	if (targetPart != NULL)
	{
		targetPart->SetFrequency(GetInstanceNumber());
		targetPart->SetCumulativeFrequency(targetPart->GetFrequency());
		targetPart->SetRefCount(1);
		olTargetPartition.AddHead(targetPart);
	}
	else
	{
		binarySliceSet = NULL;
		bOk = false;
	}

	// Initialisation de la taille max du tableau de parties cibles
	if (bOk)
	{
		oaTargetParts.SetSize(1);
		oaTargetParts.SetAt(0, targetPart);
	}

	// Initialisation de la taille du vecteur d'index des partie cibles
	if (bOk)
		bOk = ivTargetPartIndexesByRank.SetLargeSize(GetInstanceNumber());

	ensure(not bOk or Check());
	return bOk;
}

void SNBRegressorSelectionDataCostCalculator::InitializeTargetPartition()
{
	SNBIntervalTargetPart* targetPart;

	require(GetTargetAttributeType() == KWType::Continuous);
	require(olTargetPartition.GetCount() >= 1);
	require(oaTargetParts.GetSize() >= 1);
	require(ivTargetPartIndexesByRank.GetSize() == GetInstanceNumber());

	// Redimensionnenement eventuel de la partition jusqu'a obtenir une taille de 1
	while (olTargetPartition.GetCount() > 1)
	{
		// Supression de la derniere partie
		targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.RemoveTail());
		ReleasePart(targetPart);
	}

	// Reinitialisation de l'unique partie en cours
	targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetHead());
	targetPart->SetFrequency(GetInstanceNumber());
	targetPart->SetCumulativeFrequency(targetPart->GetFrequency());
	targetPart->SetRefCount(1);
	oaTargetParts.SetAt(0, targetPart);

	// Initialisation des probabilites conditionnelles a 1 (Ln(prob) a 0)
	targetPart->GetScores()->Initialize();

	ensure(targetPart->GetScores()->GetSize() == GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber());
	ensure(olTargetPartition.GetCount() == oaTargetParts.GetSize());
}

void SNBRegressorSelectionDataCostCalculator::InitializeDataCostState()
{
	int nChunkInstance;
	double dInstanceNonNormalizedDataCost;

	// Calcul d'un epsilon de Laplace pour la gestion des petites probabilites
	// En effet, cette normalisation est faite en univarie, mais le probleme peut
	// a nouveau se poser en multivarie du fait de la multiplication de probabilites
	// Comme les probabilites sont donnees par rang (par defaut, la probabilite d'un rang est 1/N),
	// ON adopte un epsilon similaire au cas de la classification, en considerant que l'on est
	// dans le cas ou le nombre de classes cibles J vaut N
	//    e = 0.5/N
	dInstanceNumber = GetInstanceNumber();

	// Pour eviter le cas InstanceNumber == 0
	dLaplaceEpsilon = 0.5 / (dInstanceNumber + 1.0);

	// Prise en compte de l'epsilon pour une probabilite p
	//   p = p*N / N
	//   p_Laplace = (p*N + e)/(N + N*e)
	// Precalcul une fois pour toutes du denominateur
	dLaplaceDenominator = dInstanceNumber + dLaplaceEpsilon * dInstanceNumber;

	// Calcul des seuils de score max pour eviter les exponentielles infinies (vis a vis de la precision machine)
	dMaxExpScore = DBL_MAX / dInstanceNumber;
	cMaxScore = (Continuous)log(dMaxExpScore);

	// Calcul du couts de la selection vide
	dSelectionDataCost = 0.0;
	for (nChunkInstance = 0; nChunkInstance < GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber();
	     nChunkInstance++)
	{
		dInstanceNonNormalizedDataCost = ComputeInstanceNonNormalizedDataCost(nChunkInstance);
		dvInstanceNonNormalizedDataCosts.SetAt(nChunkInstance, dInstanceNonNormalizedDataCost);
		dSelectionDataCost += dInstanceNonNormalizedDataCost;
	}
	dSelectionDataCost +=
	    GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber() * log(dLaplaceDenominator);

	// Initialisation des variables pour defaire une modification
	lastModificationAttribute = NULL;
	bLastModificationWasIncrease = false;
	cLastModificationDeltaWeight = 0.0;
	dLastModificationSelectionDataCost = 0.0;
	dvLastModificationInstanceNonNormalizedDataCosts.CopyFrom(&dvInstanceNonNormalizedDataCosts);
}

void SNBRegressorSelectionDataCostCalculator::Delete()
{
	olTargetPartition.DeleteAll();
	olReleasedPartsCache.DeleteAll();
	oaTargetParts.SetSize(0);
}

void SNBRegressorSelectionDataCostCalculator::UpdateTargetPartitionWithAddedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute)
{
	POSITION position;
	POSITION targetPosition;
	IntVector ivTargetPartFrequencies;
	int nTarget;
	int nTargetFrequency;
	int nTargetCumulativeFrequency;
	SNBIntervalTargetPart* targetPart;
	SNBIntervalTargetPart* newTargetPart;
	int nRank;
	int nTargetPart;

	require(Check());

	// Calcul des effectifs par partie de l'attribut cible
	GetDataTableBinarySliceSet()->ExportTargetPartFrequencies(attribute, &ivTargetPartFrequencies);

	// Acces la premiere partie de l'attribut
	position = olTargetPartition.GetHeadPosition();
	targetPosition = position;
	targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));
	nTargetFrequency = ivTargetPartFrequencies.GetAt(0);
	assert(olTargetPartition.GetAt(targetPosition) == targetPart);

	// Parcours synchronise de la partition cible courante et de la partition cible de l'attribut
	// pour determiner les parties a ajouter
	nTarget = 1;
	nTargetCumulativeFrequency = nTargetFrequency;
	while (nTarget < ivTargetPartFrequencies.GetSize())
	{
		// Cas ou la partie de l'attribut finit avant celle de la partition multivarie:
		//  il faut creer une nouvelle partie multivariee
		if (nTargetCumulativeFrequency < targetPart->GetCumulativeFrequency())
		{
			// Creation d'une nouvelle partie multivariee avant la partie courante
			newTargetPart = cast(SNBIntervalTargetPart*, GetOrCreatePart());

			// Gestion rudimentaire du manque de memoire
			if (newTargetPart == NULL)
				AddFatalError("Not enough working memory to optimize variable selection");
			else
			{
				// Initialisation de la partie
				newTargetPart->SetCumulativeFrequency(nTargetCumulativeFrequency);
				newTargetPart->SetFrequency(
				    nTargetCumulativeFrequency -
				    (targetPart->GetCumulativeFrequency() - targetPart->GetFrequency()));
				newTargetPart->SetRefCount(1);

				// Initialisation de ses probas conditionnelles a partir de la partie dont elle est issue
				newTargetPart->GetScores()->CopyFrom(targetPart->GetScores());

				// Mise a jour de la partie courante
				targetPart->SetFrequency(targetPart->GetFrequency() - newTargetPart->GetFrequency());

				// Chainage de la nouvelle partie avant la partie courante
				olTargetPartition.InsertBefore(targetPosition, newTargetPart);
				assert(olTargetPartition.GetAt(targetPosition) == targetPart);

				// Passage a la partie de l'attribut suivante
				nTargetFrequency = ivTargetPartFrequencies.GetAt(nTarget);
				nTargetCumulativeFrequency += nTargetFrequency;
				nTarget++;
			}
		}
		// Cas ou la partie de l'attribut finit apres de la partition multivarie:
		//  il faut passer a la partie multivariee suivante
		else if (nTargetCumulativeFrequency > targetPart->GetCumulativeFrequency())
		{
			// Acces a la partie cible multivariee suivante
			targetPosition = position;
			targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPosition) == targetPart);
		}
		// Cas ou la partie de l'attribut coincide avec la partition multivarie:
		//  il faut incrementer le compteur d'utilisation de la partie cible
		else
		{
			assert(nTargetCumulativeFrequency == targetPart->GetCumulativeFrequency());

			// Incrementation du compteur de reference de la partie
			targetPart->SetRefCount(targetPart->GetRefCount() + 1);

			// Acces a la partie cible multivariee suivante
			targetPosition = position;
			targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPosition) == targetPart);

			// Passage a la partie de l'attribut suivante
			nTargetFrequency = ivTargetPartFrequencies.GetAt(nTarget);
			nTargetCumulativeFrequency += nTargetFrequency;
			nTarget++;
		}
	}

	// Transformation de la liste de parties en tableau indexe par rang pour en accelerer l'acces dans GetSelectionDataCost
	olTargetPartition.ExportObjectArray(&oaTargetParts);
	nRank = 0;
	for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBIntervalTargetPart*, oaTargetParts.GetAt(nTargetPart));

		// Tous les valeurs de la cible sont associees a cette partie
		while (nRank < targetPart->GetCumulativeFrequency())
		{
			ivTargetPartIndexesByRank.SetAt(nRank, nTargetPart);
			nRank++;
		}
	}

	ensure(Check());
}

void SNBRegressorSelectionDataCostCalculator::UpdateTargetPartitionWithRemovedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute)
{
	IntVector ivTargetPartFrequencies;
	int nTarget;
	int nTargetFrequency;
	int nTargetCumulativeFrequency;
	POSITION position;
	POSITION targetPartPosition;
	SNBIntervalTargetPart* targetPart;
	int nDeletedTargetPartFrequency;
	int nRank;
	int nTargetPart;

	require(Check());

	// Calcul des effectifs par partie de l'attribut cible
	GetDataTableBinarySliceSet()->ExportTargetPartFrequencies(attribute, &ivTargetPartFrequencies);

	// Parcours synchronise de la partition cible courante et de la partition cible de l'attribut
	// pour determiner les parties a ajouter
	nTargetCumulativeFrequency = 0;
	nTarget = 0;
	targetPartPosition = NULL;
	targetPart = NULL;
	position = olTargetPartition.GetHeadPosition();
	while (nTarget < ivTargetPartFrequencies.GetSize())
	{
		// Initialisation
		if (nTargetCumulativeFrequency == 0)
		{
			// Acces a la premiere partie cible multivariee
			targetPartPosition = position;
			targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPartPosition) == targetPart);

			// Acces la premiere partie de l'attribut
			nTargetFrequency = ivTargetPartFrequencies.GetAt(nTarget);
			nTargetCumulativeFrequency += nTargetFrequency;
			nTarget++;
		}

		// La partie de l'attribut finit necessairement apres celle de la partition multivarie:
		assert(nTargetCumulativeFrequency >= targetPart->GetCumulativeFrequency());

		// Cas ou la partie de l'attribut finit apres de la partition multivarie:
		//  il faut passer a la partie multivariee suivante
		if (nTargetCumulativeFrequency > targetPart->GetCumulativeFrequency())
		{
			// Acces a la partie cible multivariee suivante
			targetPartPosition = position;
			targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));
		}
		// Cas ou la partie de l'attribut coincide avec la partition multivarie:
		//  il faut decrementer le compteur d'utilisation de la partie cible
		else
		{
			assert(nTargetCumulativeFrequency == targetPart->GetCumulativeFrequency());

			// Increment du compteur de reference de la partie
			targetPart->SetRefCount(targetPart->GetRefCount() - 1);

			// Si le compteur passe a 0, il faut supprimer la partie de la liste
			nDeletedTargetPartFrequency = 0;
			if (targetPart->GetRefCount() == 0)
			{
				nDeletedTargetPartFrequency = targetPart->GetFrequency();
				olTargetPartition.RemoveAt(targetPartPosition);
				ReleasePart(targetPart);
			}

			// Acces a la partie cible multivariee suivante
			targetPartPosition = position;
			targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));

			// Passage a la partie de l'attribut suivante
			nTargetFrequency = ivTargetPartFrequencies.GetAt(nTarget);
			nTargetCumulativeFrequency += nTargetFrequency;
			nTarget++;

			// Mise a jour de la partie suivant la partie detruite
			targetPart->SetFrequency(targetPart->GetFrequency() + nDeletedTargetPartFrequency);
		}
	}

	// Transformation de la liste de parties en tableau indexe par rang pour en accelerer l'acces dans GetSelectionDataCost
	olTargetPartition.ExportObjectArray(&oaTargetParts);
	nRank = 0;
	for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBIntervalTargetPart*, oaTargetParts.GetAt(nTargetPart));

		// Tous les valeurs de la cible sont associees a cette partie
		while (nRank < targetPart->GetCumulativeFrequency())
		{
			ivTargetPartIndexesByRank.SetAt(nRank, nTargetPart);
			nRank++;
		}
	}
	ensure(Check());
}

boolean SNBRegressorSelectionDataCostCalculator::UpdateTargetPartScoresWithWeightedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute, Continuous cDeltaWeight)
{
	boolean bOk = true;
	int nTargetPart;
	SNBIntervalTargetPart* targetPart;
	IntVector ivAttributeTargetPartFrequencies;
	int nAttributeTargetCumulativeFrequency;
	int nAttributeTargetPartFrequency;
	int nAttributeTargetPart;
	SNBDataTableBinarySliceSetColumn* chunkColumn;
	int nColumnValueNumber;
	int nColumnValueIndex;
	int nChunkInstance;
	int nChunkInstanceValue;
	Continuous cLogProb;

	// Memorisation des indexes de parties cibles multivaries pour chaque une des parties cibles de l'attribut
	GetDataTableBinarySliceSet()->ExportTargetPartFrequencies(attribute, &ivAttributeTargetPartFrequencies);
	ivAttributeTargetPartIndexByTargetPartIndex.SetSize(oaTargetParts.GetSize());
	nAttributeTargetCumulativeFrequency = 0;
	nAttributeTargetPart = 0;
	for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBIntervalTargetPart*, oaTargetParts.GetAt(nTargetPart));

		if (targetPart->GetCumulativeFrequency() > nAttributeTargetCumulativeFrequency)
		{
			nAttributeTargetPartFrequency = ivAttributeTargetPartFrequencies.GetAt(nAttributeTargetPart);
			nAttributeTargetCumulativeFrequency += nAttributeTargetPartFrequency;
			nAttributeTargetPart++;
		}
		ivAttributeTargetPartIndexByTargetPartIndex.SetAt(nTargetPart, nAttributeTargetPart - 1);
	}

	// Acces a la colonne de donnees de l'attribut
	chunkColumn = NULL;
	bOk = bOk and GetDataTableBinarySliceSet()->GetAttributeColumnView(attribute, chunkColumn);

	// Mise a jour des scores des parties cibles
	if (bOk)
	{
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
		{
			targetPart = cast(SNBIntervalTargetPart*, oaTargetParts.GetAt(nTargetPart));

			// Mise a jour des scores de la partie cible courante seulement pour les valeurs presentes de la colonne sparse
			if (chunkColumn->IsSparse())
			{
				nColumnValueNumber = chunkColumn->GetValueNumber();
				for (nColumnValueIndex = 0; nColumnValueIndex < nColumnValueNumber; nColumnValueIndex++)
				{
					nChunkInstance = chunkColumn->GetSparseValueInstanceIndexAt(nColumnValueIndex);
					nChunkInstanceValue = chunkColumn->GetSparseValueAt(nColumnValueIndex);
					cLogProb = attribute->GetLnSourceConditionalProb(
					    nChunkInstanceValue,
					    ivAttributeTargetPartIndexByTargetPartIndex.GetAt(nTargetPart));
					targetPart->GetScores()->UpgradeAt(nChunkInstance, cDeltaWeight * cLogProb);
				}
			}
			// Mise a jour des scores de la partie cible courante pour les valeurs denses
			else
			{
				nColumnValueNumber = chunkColumn->GetValueNumber();
				for (nChunkInstance = 0; nChunkInstance < nColumnValueNumber; nChunkInstance++)
				{
					nChunkInstanceValue = chunkColumn->GetDenseValueAt(nChunkInstance);
					cLogProb = attribute->GetLnSourceConditionalProb(
					    nChunkInstanceValue,
					    ivAttributeTargetPartIndexByTargetPartIndex.GetAt(nTargetPart));
					targetPart->GetScores()->UpgradeAt(nChunkInstance, cDeltaWeight * cLogProb);
				}
			}
		}
	}
	return bOk;
}

boolean SNBRegressorSelectionDataCostCalculator::UpdateDataCost()
{
	const boolean bLocalTrace = false;
	boolean bOk = true;
	int nTargetPart;
	int nChunkInstanceNumber;
	int nChunkInstance;
	double dInstanceNonNormalizedDataCost;

	// Entete de trace de debbogage
	if (bLocalTrace)
	{
		cout << "\tInstance\tTarget\tTargetPart\tProb\tCost";
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
			cout << "\tScore" << nTargetPart + 1;
		cout << "\n";
	}

	// Mise-a-jour du score en recalculant tous les couts par instance
	dSelectionDataCost = 0.0;
	nChunkInstanceNumber = GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber();
	for (nChunkInstance = 0; nChunkInstance < nChunkInstanceNumber; nChunkInstance++)
	{
		dInstanceNonNormalizedDataCost = ComputeInstanceNonNormalizedDataCost(nChunkInstance);
		dSelectionDataCost += dInstanceNonNormalizedDataCost;
		dvInstanceNonNormalizedDataCosts.SetAt(nChunkInstance, dInstanceNonNormalizedDataCost);
	}
	dSelectionDataCost += nChunkInstanceNumber * log(dLaplaceDenominator);

	return bOk;
}

boolean SNBRegressorSelectionDataCostCalculator::UpdateDataCostWithSparseAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute)
{
	const boolean bLocalTrace = false;
	boolean bOk = true;
	int nTargetPart;
	int nChunkColumnValueNumber;
	int nChunkColumnValue;
	int nChunkInstance;
	SNBDataTableBinarySliceSetColumn* chunkColumn;
	double dOldInstanceNonNormalizedDataCost;
	double dInstanceNonNormalizedDataCost;

	require(attribute != NULL);
	require(attribute->IsSparse());
	require(GetDataTableBinarySliceSet()->ContainsAttribute(attribute));

	// Entete de trace de debbogage
	if (bLocalTrace)
	{
		cout << "\tInstance\tTarget\tTargetPart\tProb\tCost";
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
			cout << "\tScore" << nTargetPart + 1;
		cout << "\n";
	}

	// Mise-a-jour du score en calculant seulement le delta dans les instances non-manquantes
	// Nota: Pas necessaire de normaliser avec le denominateur de Laplace car on ne recommence pas de
	// zero comme dans le cas dense
	chunkColumn = NULL;
	bOk = bOk and GetDataTableBinarySliceSet()->GetAttributeColumnView(attribute, chunkColumn);
	if (bOk)
	{
		nChunkColumnValueNumber = chunkColumn->GetValueNumber();
		for (nChunkColumnValue = 0; nChunkColumnValue < nChunkColumnValueNumber; nChunkColumnValue++)
		{
			nChunkInstance = chunkColumn->GetSparseValueInstanceIndexAt(nChunkColumnValue);
			dOldInstanceNonNormalizedDataCost = dvInstanceNonNormalizedDataCosts.GetAt(nChunkInstance);
			dInstanceNonNormalizedDataCost = ComputeInstanceNonNormalizedDataCost(nChunkInstance);
			dSelectionDataCost += dInstanceNonNormalizedDataCost - dOldInstanceNonNormalizedDataCost;
			dvInstanceNonNormalizedDataCosts.SetAt(nChunkInstance, dInstanceNonNormalizedDataCost);
		}
	}

	return bOk;
}

SNBTargetPart* SNBRegressorSelectionDataCostCalculator::CreatePart()
{
	return new SNBIntervalTargetPart;
}

boolean SNBRegressorSelectionDataCostCalculator::Check() const
{
	boolean bOk = true;
	int nCumulativeFrequency;
	POSITION position;
	SNBIntervalTargetPart* targetPart;
	ALString sTmp;

	bOk = bOk and olTargetPartition.GetCount() >= 1;
	bOk = bOk and oaTargetParts.GetSize() >= 1;
	bOk = bOk and ivTargetPartIndexesByRank.GetSize() == GetInstanceNumber();

	if (bOk)
	{
		// Si la base est vide, il ne doit y avoir qu'une partie cible
		if (GetInstanceNumber() == 0)
		{
			if (olTargetPartition.GetCount() != 1)
			{
				AddError("Target partition structure should contain one single part");
				bOk = false;
			}
		}
		// Verification de la structure sinon
		else
		{
			nCumulativeFrequency = 0;
			position = olTargetPartition.GetHeadPosition();
			while (position != NULL)
			{
				targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));

				// Verification de la partie
				nCumulativeFrequency += targetPart->GetFrequency();
				if (targetPart->GetFrequency() <= 0)
				{
					AddError(sTmp + "Target part frequency (" +
						 IntToString(targetPart->GetFrequency()) +
						 ") should be strictly positive");
					bOk = false;
				}
				if (targetPart->GetCumulativeFrequency() != nCumulativeFrequency)
				{
					AddError(sTmp + "Target part cumulated frequency (" +
						 IntToString(targetPart->GetCumulativeFrequency()) +
						 ") is inconsistent with the computed cumulated frequency (" +
						 IntToString(nCumulativeFrequency) + ")");
					bOk = false;
				}
				if (targetPart->GetRefCount() < 1)
				{
					AddError(sTmp + "Target part reference counter (" +
						 IntToString(targetPart->GetRefCount()) + ") should be at least 1");
					bOk = false;
				}
				if (targetPart->GetScores()->GetSize() !=
				    GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber())
				{
					AddError(
					    sTmp + "Size of score vector (" +
					    IntToString(targetPart->GetScores()->GetSize()) + ") " +
					    "is inconsistent with the number active instances of the binary slice set "
					    "(" +
					    IntToString(
						GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber()) +
					    ")");
					bOk = false;
				}
				if (not bOk)
					break;
			}

			// Verification de l'effectif cumule total
			if (bOk and nCumulativeFrequency != GetInstanceNumber())
			{
				AddError(sTmp + "Last target part cumulated frequency (" +
					 IntToString(nCumulativeFrequency) +
					 ") is inconsistent with the database size (" +
					 IntToString(GetInstanceNumber()) + ")");
				bOk = false;
			}
		}
	}
	return bOk;
}

void SNBRegressorSelectionDataCostCalculator::Write(ostream& ost) const
{
	POSITION position;
	SNBIntervalTargetPart* targetPart;
	boolean bWriteHeader;

	require(Check());

	bWriteHeader = true;
	position = olTargetPartition.GetHeadPosition();
	while (position != NULL)
	{
		targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));
		if (bWriteHeader)
		{
			targetPart->WriteHeaderLineReport(ost);
			ost << "\n";
			bWriteHeader = false;
		}
		targetPart->WriteLineReport(ost);
		ost << "\n";
	}
}

longint SNBRegressorSelectionDataCostCalculator::ComputeNecessaryMemory(int nInstanceNumber,
									int nDistinctTargetValueNumber)
{
	ObjectList olDummy;
	ObjectArray oaDummy;
	longint lEffectiveDistinctTargetValueNumber;

	require(nInstanceNumber > 0);

	// Formule de l'instance (nombre max. de parties estimes a ~sqrt(nInstanceNumber)):
	//   Objets de l'instance +
	//   Max d'objects KWPSNewTargetIntervalScore +
	//   Max overhead des conteneurs de KWPSNewTargetIntervalScore (oaTargetParts, olReleasedPartsCache & olTargetPartition) +
	//   Vecteur d'removedAttributeIndex des parties cible pour chaque individu (
	lEffectiveDistinctTargetValueNumber = (longint)min((double)nDistinctTargetValueNumber, sqrt(nInstanceNumber));
	return sizeof(SNBRegressorSelectionDataCostCalculator) +
	       lEffectiveDistinctTargetValueNumber * SNBIntervalTargetPart::ComputeNecessaryMemory(nInstanceNumber) +
	       lEffectiveDistinctTargetValueNumber *
		   (2 * olDummy.GetUsedMemoryPerElement() + oaDummy.GetUsedMemoryPerElement()) +
	       longint(nInstanceNumber) * sizeof(int);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBGroupTargetPart

SNBGroupTargetPart::SNBGroupTargetPart()
{
	nFrequency = 0;
}

SNBGroupTargetPart::~SNBGroupTargetPart() {}

void SNBGroupTargetPart::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

int SNBGroupTargetPart::GetFrequency() const
{
	return nFrequency;
}

IntVector* SNBGroupTargetPart::GetSignature()
{
	return &ivTargetSignature;
}

int SNBGroupTargetPart::GetSignatureSize() const
{
	return ivTargetSignature.GetSize();
}

int SNBGroupTargetPart::GetGroupIndexAt(int nSignature) const
{
	require(0 <= nSignature && nSignature < GetSignatureSize());
	return ivTargetSignature.GetAt(nSignature);
}

void SNBGroupTargetPart::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Frequency\tSignature [signature id -> attribute group id]";
}

void SNBGroupTargetPart::WriteLineReport(ostream& ost) const
{
	int nAttribute;

	ost << GetFrequency() << "\t";
	ost << "(";
	for (nAttribute = 0; nAttribute < ivTargetSignature.GetSize(); nAttribute++)
	{
		if (nAttribute > 0)
			ost << ", ";
		ost << ivTargetSignature.GetAt(nAttribute);
	}
	ost << ")";
}

const ALString SNBGroupTargetPart::GetObjectLabel() const
{
	ALString sLabel;
	int nAttribute;

	sLabel = "Part(";
	for (nAttribute = 0; nAttribute < ivTargetSignature.GetSize(); nAttribute++)
	{
		if (nAttribute > 0)
			sLabel += ", ";
		sLabel += IntToString(ivTargetSignature.GetAt(nAttribute));
	}
	sLabel += ")";
	sLabel += "{";
	sLabel += IntToString(nFrequency);
	sLabel += "}";

	return sLabel;
}

longint SNBGroupTargetPart::ComputeNecessaryMemory(int nInstanceNumber, int nAttributeNumber)
{
	// Formule de l'estimation
	//   Objets de l'instance +
	//   Contenus du vecteur de scores (cvScores) +
	//   Contenus du vecteur de la signature (ivTargetSignature)
	return sizeof(SNBGroupTargetPart) + (longint)nInstanceNumber * sizeof(Continuous) +
	       (longint)nAttributeNumber * sizeof(int);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBGroupTargetPartSignatureSchema

SNBGroupTargetPartSignatureSchema::SNBGroupTargetPartSignatureSchema() {}

SNBGroupTargetPartSignatureSchema::~SNBGroupTargetPartSignatureSchema()
{
	nkdAttributeIndexes.DeleteAll();
}

void SNBGroupTargetPartSignatureSchema::Initialize()
{
	nkdAttributeIndexes.DeleteAll();
	oaAttributes.RemoveAll();
}

int SNBGroupTargetPartSignatureSchema::GetSize() const
{
	return oaAttributes.GetSize();
}

SNBDataTableBinarySliceSetAttribute* SNBGroupTargetPartSignatureSchema::GetAttributeAt(int nSignature) const
{
	require(0 <= nSignature && nSignature < oaAttributes.GetSize());
	return cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes.GetAt(nSignature));
}

int SNBGroupTargetPartSignatureSchema::GetSignatureIndexAt(const SNBDataTableBinarySliceSetAttribute* attribute) const
{
	KWSortableIndex* attributeSignatureIndex;
	int nSignature;

	attributeSignatureIndex = cast(KWSortableIndex*, nkdAttributeIndexes.Lookup(attribute));
	if (attributeSignatureIndex == NULL)
		nSignature = -1;
	else
		nSignature = attributeSignatureIndex->GetIndex();

	return nSignature;
}

void SNBGroupTargetPartSignatureSchema::AddAttribute(const SNBDataTableBinarySliceSetAttribute* attribute)
{
	KWSortableIndex* attributeSignatureIndex;

	require(not Contains(attribute));
	require(Check());

	// Ajout de l'attribut a la fin de la signature
	oaAttributes.Add(const_cast<SNBDataTableBinarySliceSetAttribute*>(attribute));

	// Memorisation de son removedAttributeIndex dans la signature
	attributeSignatureIndex = new KWSortableIndex;
	attributeSignatureIndex->SetIndex(oaAttributes.GetSize() - 1);
	nkdAttributeIndexes.SetAt(attribute, attributeSignatureIndex);

	ensure(Contains(attribute));
	ensure(Check());
}

void SNBGroupTargetPartSignatureSchema::RemoveAttribute(const SNBDataTableBinarySliceSetAttribute* attribute)
{
	KWSortableIndex* removedAttributeIndex;
	int nRemovedAttribute;
	KWSortableIndex* lastAttributeIndex;
	SNBDataTableBinarySliceSetAttribute* lastAttribute;

	require(Contains(attribute));
	require(Check());

	// Recherche de l'indice de l'attribut a supprimer
	removedAttributeIndex = cast(KWSortableIndex*, nkdAttributeIndexes.Lookup(attribute));
	nRemovedAttribute = removedAttributeIndex->GetIndex();

	// Supression de l'indice de l'attribut dans le dictionnaire d'indexes
	nkdAttributeIndexes.RemoveKey(attribute);
	delete removedAttributeIndex;

	// Deplacement du dernier attribut a la place de celui supprime (retaillage ensuite)
	if (oaAttributes.GetSize() >= 2 and nRemovedAttribute < oaAttributes.GetSize() - 1)
	{
		lastAttribute =
		    cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes.GetAt(oaAttributes.GetSize() - 1));
		oaAttributes.SetAt(nRemovedAttribute, lastAttribute);
		lastAttributeIndex = cast(KWSortableIndex*, nkdAttributeIndexes.Lookup(lastAttribute));
		lastAttributeIndex->SetIndex(nRemovedAttribute);
	}
	oaAttributes.SetSize(oaAttributes.GetSize() - 1);

	ensure(not Contains(attribute));
	ensure(Check());
}

boolean SNBGroupTargetPartSignatureSchema::Contains(const SNBDataTableBinarySliceSetAttribute* attribute) const
{
	return GetSignatureIndexAt(attribute) != -1;
}

boolean SNBGroupTargetPartSignatureSchema::Check() const
{
	boolean bOk = true;
	int nSignature;
	SNBDataTableBinarySliceSetAttribute* attribute;
	KWSortableIndex* attributeSignatureIndex;

	// Verification de la coherence de la taille conteneurs des attributs et ses indexes
	bOk = bOk and oaAttributes.GetSize() == nkdAttributeIndexes.GetCount();

	// Verification de la coherence de la relation [attribut -> index de l'attribut dans la signature]
	if (bOk)
	{
		for (nSignature = 0; nSignature < oaAttributes.GetSize(); nSignature++)
		{
			attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes.GetAt(nSignature));
			bOk = bOk and attribute != NULL;
			bOk = bOk and nkdAttributeIndexes.Lookup(attribute) != NULL;
			attributeSignatureIndex = cast(KWSortableIndex*, nkdAttributeIndexes.Lookup(attribute));
			bOk = bOk and attributeSignatureIndex->GetIndex() == nSignature;
			if (not bOk)
				break;
		}
	}
	return bOk;
}

void SNBGroupTargetPartSignatureSchema::Write(ostream& ost)
{
	int nSignature;

	ost << "(";
	for (nSignature = 0; nSignature < oaAttributes.GetSize(); nSignature++)
	{
		if (nSignature > 0)
			ost << ", ";
		ost << GetAttributeAt(nSignature)->GetNativeAttributeName();
	}
	ost << ")";
}

longint SNBGroupTargetPartSignatureSchema::ComputeNecessaryMemory(int nAttributes)
{
	ObjectArray oaDummy;
	NumericKeyDictionary nkdDummy;

	require(nAttributes >= 0);

	// Formule pour l'estimation :
	//   Objets de l'instance +
	//   Contenus du tableau d'attributs de la signature (oaAttributes, seulement pointeurs) +
	//   Contenus du dictionnaire [attribut -> index de signature] (nkdAttributeIndexes)
	return sizeof(SNBGroupTargetPartSignatureSchema) + nAttributes * oaDummy.GetUsedMemoryPerElement() +
	       nAttributes * (nkdDummy.GetUsedMemoryPerElement() + sizeof(KWSortableIndex));
}

int KWGroupTargetPartCompareTargetSignature(const void* elem1, const void* elem2)
{
	SNBGroupTargetPart* groupTargetPart1;
	SNBGroupTargetPart* groupTargetPart2;
	IntVector* signature1;
	IntVector* signature2;
	int nSignatureDiff;
	int nSignatureIndex;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux objets
	groupTargetPart1 = cast(SNBGroupTargetPart*, *(Object**)elem1);
	groupTargetPart2 = cast(SNBGroupTargetPart*, *(Object**)elem2);
	signature1 = groupTargetPart1->GetSignature();
	signature2 = groupTargetPart2->GetSignature();

	// Comparaison des tailles
	nSignatureDiff = signature1->GetSize() - signature2->GetSize();

	// Comparaison attribut par attribut des removedAttributeIndex de groupes
	if (nSignatureDiff == 0)
	{
		for (nSignatureIndex = 0; nSignatureIndex < signature1->GetSize(); nSignatureIndex++)
		{
			nSignatureDiff = signature1->GetAt(nSignatureIndex) - signature2->GetAt(nSignatureIndex);
			if (nSignatureDiff != 0)
				break;
		}
	}
	return nSignatureDiff;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBGeneralizedClassifierSelectionDataCostCalculator

SNBGeneralizedClassifierSelectionDataCostCalculator::SNBGeneralizedClassifierSelectionDataCostCalculator() {}

SNBGeneralizedClassifierSelectionDataCostCalculator::~SNBGeneralizedClassifierSelectionDataCostCalculator()
{
	Delete();
}

boolean SNBGeneralizedClassifierSelectionDataCostCalculator::Create()
{
	boolean bOk = true;
	SNBGroupTargetPart* targetPart;
	int nTarget;

	require(GetLearningSpec() != NULL);
	require(GetLearningSpec()->GetTargetAttributeType() == KWType::Symbol);
	require(oaTargetPartsByTargetValueIndex.GetSize() == 0);

	// Appel a la methode ancetre
	bOk = bOk and SNBPredictorSelectionDataCostCalculator::Create();

	// Initialisation des services d'indexation de groupes cible pour les attributs
	InitializeTargetValueGroupMatchings();

	// Creation d'un vecteur de probabilites conditionnelles pour un unique groupe cible initial
	bOk = true;
	targetPart = cast(SNBGroupTargetPart*, GetOrCreatePart());
	if (targetPart != NULL)
	{
		targetPart->SetFrequency(GetInstanceNumber());
		targetPart->GetSignature()->SetSize(0);
	}
	else
		bOk = false;

	// Initialisation de la taille max du tableau de parties cibles
	if (bOk)
		bOk = oaTargetParts.SetLargeSize(GetTargetValueNumber());

	// Initialisation de la taille du tableau des parties cibles indexe par les valeurs
	if (bOk)
		bOk = oaTargetPartsByTargetValueIndex.SetLargeSize(GetTargetValueNumber());

	// Initialisation de ce tableau avec l'unique partie initiale
	if (bOk)
	{
		// Initialisation du tableau de parties
		oaTargetParts.SetSize(1);
		oaTargetParts.SetAt(0, targetPart);

		// Association entre les valeurs et la partie initiale
		for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
			oaTargetPartsByTargetValueIndex.SetAt(nTarget, targetPart);
	}

	ensure(not bOk or Check());
	return bOk;
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::InitializeTargetValueGroupMatchings()
{
	boolean bTrace = false;
	int nAttribute;
	const KWDGSAttributePartition* targetPartition;
	const KWDGSAttributeGrouping* targetValueGroupsPartition;
	const KWDGSAttributeSymbolValues* targetValuesPartition;
	NumericKeyDictionary nkdTargetValueIndexes;
	KWSortableIndex* targetValueIndex;
	int nTargetValue;
	IntVector* ivTargetValueGroupMatching;
	int nTargetValueIndex;
	NUMERIC targetValueKey;
	int nGroup;
	int nDefaultGroup;
	SNBDataTableBinarySliceSetAttribute* attribute;

	require(GetDataTableBinarySliceSet() != NULL);

	// Nettoyage initial
	CleanTargetValueGroupMatchings();

	// Creation d'un dictionnaire de valeurs, pour memoriser leur removedAttributeIndex
	for (nTargetValue = 0; nTargetValue < GetTargetValueNumber(); nTargetValue++)
	{
		targetValueIndex = new KWSortableIndex;
		targetValueIndex->SetIndex(nTargetValue);
		targetValueKey = GetTargetValueAt(nTargetValue).GetNumericKey();
		nkdTargetValueIndexes.SetAt(targetValueKey, targetValueIndex);
	}

	// Parcours des attributs de la database d'apprentissage
	for (nAttribute = 0; nAttribute < GetDataTableBinarySliceSet()->GetAttributeNumber(); nAttribute++)
	{
		attribute = GetDataTableBinarySliceSet()->GetAttributeAt(nAttribute);

		// Creation du vecteur de matching (avec des -1 pour la gestion de la StarValue) et memorisation
		ivTargetValueGroupMatching = new IntVector;
		ivTargetValueGroupMatching->SetSize(GetTargetValueNumber());
		for (nTargetValue = 0; nTargetValue < GetTargetValueNumber(); nTargetValue++)
			ivTargetValueGroupMatching->SetAt(nTargetValue, -1);
		nkdTargetValueGroupMatchingsByAttribute.SetAt((NUMERIC)attribute, ivTargetValueGroupMatching);

		// Acces a la partition des valeurs cibles de l'attribut
		targetPartition = attribute->GetTargetPartition();

		// Initialisation dans le cas d'une partition de singletons
		if (targetPartition->ArePartsSingletons())
		{
			targetValuesPartition = cast(const KWDGSAttributeSymbolValues*, targetPartition);
			assert(targetValuesPartition->GetValueNumber() == GetTargetValueNumber());

			// Parcours des valeurs de la partition pour creer l'association
			for (nTargetValueIndex = 0; nTargetValueIndex < GetTargetValueNumber(); nTargetValueIndex++)
			{
				// Recherche de l'removedAttributeIndex de valeur cible correspondant
				targetValueKey = targetValuesPartition->GetValueAt(nTargetValueIndex).GetNumericKey();
				targetValueIndex = cast(KWSortableIndex*, nkdTargetValueIndexes.Lookup(targetValueKey));
				nTargetValue = targetValueIndex->GetIndex();

				// Memorisation de la correspondance
				assert(ivTargetValueGroupMatching->GetAt(nTargetValue) == -1);
				ivTargetValueGroupMatching->SetAt(nTargetValue, nTargetValueIndex);
			}
		}
		// Initialisation dans le cas d'une partition en groupes de valeurs
		else
		{
			targetValueGroupsPartition = cast(const KWDGSAttributeGrouping*, targetPartition);

			// Parcours des parties
			nDefaultGroup = -1;
			for (nGroup = 0; nGroup < targetValueGroupsPartition->GetGroupNumber(); nGroup++)
			{
				// Parcours des valeurs du groupe pour creer l'association
				for (nTargetValueIndex = targetValueGroupsPartition->GetGroupFirstValueIndexAt(nGroup);
				     nTargetValueIndex <= targetValueGroupsPartition->GetGroupLastValueIndexAt(nGroup);
				     nTargetValueIndex++)
				{
					// Memorisation du groupe par defaut si l'on rencontre la valeur speciale StarValue
					if (nDefaultGroup == -1 and targetValueGroupsPartition->GetValueAt(
									nTargetValueIndex) == Symbol::GetStarValue())
					{
						nDefaultGroup = nGroup;

						// Pour la valeur speciale, il n'y a pas de correspondance avec une valeur cible
						// particuliere, et il faut court-circuiter la fin de la boucle
						continue;
					}

					// Recherche de l'removedAttributeIndex de valeur cible correspondant
					targetValueKey =
					    targetValueGroupsPartition->GetValueAt(nTargetValueIndex).GetNumericKey();
					targetValueIndex =
					    cast(KWSortableIndex*, nkdTargetValueIndexes.Lookup(targetValueKey));
					nTargetValue = targetValueIndex->GetIndex();

					// Memorisation de la correspondance
					assert(ivTargetValueGroupMatching->GetAt(nTargetValue) == -1);
					ivTargetValueGroupMatching->SetAt(nTargetValue, nGroup);
				}
			}
			assert(nDefaultGroup != -1);

			// On associe le groupe par defaut aux valeurs cibles non traitees
			for (nTargetValue = 0; nTargetValue < ivTargetValueGroupMatching->GetSize(); nTargetValue++)
			{
				if (ivTargetValueGroupMatching->GetAt(nTargetValue) == -1)
					ivTargetValueGroupMatching->SetAt(nTargetValue, nDefaultGroup);
			}
		}

		// Trace
		if (bTrace)
		{
			cout << "Matching\t" << GetDataTableBinarySliceSet()->GetAttributeNameAt(nAttribute) << "\n";
			for (nTargetValue = 0; nTargetValue < ivTargetValueGroupMatching->GetSize(); nTargetValue++)
			{
				nGroup = ivTargetValueGroupMatching->GetAt(nTargetValue);
				assert(targetPartition->ComputeSymbolPartIndex(GetTargetValueAt(nTargetValue)) ==
				       nGroup);
				cout << "\t" << GetTargetValueAt(nTargetValue) << "\t" << nGroup << "\n";
			}
		}
	}

	// Nettoyage du dictionnaire de valeurs
	nkdTargetValueIndexes.DeleteAll();
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::InitializeDataCostState()
{
	int nTargetValueNumber;
	int nChunkInstance;
	double dInstanceNonNormalizedDataCost;

	require(GetDataTableBinarySliceSet() != NULL);
	require(GetDataTableBinarySliceSet()->IsInitialized());

	// Calcul d'un epsilon de Laplace pour la gestion des petites probabilites
	// En effet, cette normalisation est faite en univarie, mais le probleme peut
	// a nouveau se poser en multivarie du fait de la multiplication de probabilites
	// En univarie, l'epsilon est tres petit (e = 1/InstanceNumber), car on considere
	// que l'estimation MODL est de bonne qualite.
	// En multivarie, on se contente du "proche du classique" e = 0.5/J,
	// ce qui permet d'avoir un denominateur en N+0.5 quelque soit le nombre J de classes
	nTargetValueNumber = GetTargetDescriptiveStats()->GetValueNumber();
	dInstanceNumber = double(GetInstanceNumber());
	dLaplaceEpsilon = 0.5 / nTargetValueNumber;

	// Prise en compte de l'epsilon pour une probabilite p
	//   p = p*N / N
	//   p_Laplace = (p*N + e)/(N + J*e)
	// Precalcul une fois pour toutes du denominateur
	dLaplaceDenominator = dInstanceNumber + dLaplaceEpsilon * nTargetValueNumber;

	// Calcul des seuils de score max pour eviter les exponentielles infinies (vis a vis de la precision machine)
	dMaxExpScore = DBL_MAX / dInstanceNumber;
	cMaxScore = (Continuous)log(dMaxExpScore);

	// Calcul du couts de la selection vide
	dSelectionDataCost = 0.0;
	for (nChunkInstance = 0; nChunkInstance < GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber();
	     nChunkInstance++)
	{
		dInstanceNonNormalizedDataCost = ComputeInstanceNonNormalizedDataCost(nChunkInstance);
		dvInstanceNonNormalizedDataCosts.SetAt(nChunkInstance, dInstanceNonNormalizedDataCost);
		dSelectionDataCost += dInstanceNonNormalizedDataCost;
	}
	dSelectionDataCost +=
	    GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber() * log(dLaplaceDenominator);

	// Initialisation des variables pour defaire une modification
	lastModificationAttribute = NULL;
	bLastModificationWasIncrease = false;
	cLastModificationDeltaWeight = 0.0;
	dLastModificationSelectionDataCost = 0.0;
	dvLastModificationInstanceNonNormalizedDataCosts.CopyFrom(&dvInstanceNonNormalizedDataCosts);
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::InitializeTargetPartition()
{
	SNBGroupTargetPart* targetPart;
	int nTargetPart;
	int nTargetValue;

	require(GetTargetAttributeType() == KWType::Symbol);

	// Initialisation de le schema des signatures
	signatureSchema.Initialize();

	// Destruction de toutes les parties, sauf la premiere
	for (nTargetPart = 1; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBGroupTargetPart*, oaTargetParts.GetAt(nTargetPart));
		ReleasePart(targetPart);
	}
	oaTargetParts.SetSize(1);

	// Reinitialisation de l'unique partie en cours
	targetPart = cast(SNBGroupTargetPart*, oaTargetParts.GetAt(0));
	targetPart->SetFrequency(GetInstanceNumber());
	targetPart->GetSignature()->SetSize(0);

	// Initialisation des probabilites conditionnelles a 1 (Ln(prob) a 0)
	assert(targetPart->GetScores()->GetSize() == GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber());
	targetPart->GetScores()->Initialize();

	// Reinitialisation de l'association valeur-partie avec l'unique partie
	for (nTargetValue = 0; nTargetValue < GetTargetValueNumber(); nTargetValue++)
		oaTargetPartsByTargetValueIndex.SetAt(nTargetValue, targetPart);
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::Delete()
{
	SNBGroupTargetPart* targetPart;
	int nTargetPart;

	// Liberation de toutes les parties
	for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBGroupTargetPart*, oaTargetParts.GetAt(nTargetPart));
		ReleasePart(targetPart);
	}

	// Destruction de toutes les parties (elles sont toutes deja liberes)
	olReleasedPartsCache.DeleteAll();

	// Reinitialisation des tableaux
	oaTargetPartsByTargetValueIndex.SetSize(0);
	oaTargetParts.SetSize(0);

	// Nettoyage des services d'indexation de groupes cible pour les attributs
	CleanTargetValueGroupMatchings();
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::CleanTargetValueGroupMatchings()
{
	nkdTargetValueGroupMatchingsByAttribute.DeleteAll();
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::UpdateTargetPartitionWithAddedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute)
{
	int nTargetValue;
	NumericKeyDictionary nkdInitialParts;
	NumericKeyDictionary* nkdSubparts;
	int nPartIndex;
	ObjectArray oaTargetPartIndexes;
	KWSortableIndex* targetPartIndex;
	SNBGroupTargetPart* targetPart;
	SNBGroupTargetPart* newTargetPart;

	require(not signatureSchema.Contains(attribute));
	require(Check());

	// Creation de cles d'acces aux parties cibles, qui serviront de cle d'acces
	// dans des dictionnaire a cle numerique
	oaTargetPartIndexes.SetSize(attribute->GetTargetPartition()->GetPartNumber());
	for (nPartIndex = 0; nPartIndex < oaTargetPartIndexes.GetSize(); nPartIndex++)
	{
		targetPartIndex = new KWSortableIndex;
		targetPartIndex->SetIndex(nPartIndex);
		oaTargetPartIndexes.SetAt(nPartIndex, targetPartIndex);
	}

	// Parcours des parties initiales referencees pour chaque valeur
	// Chaque partie initiale sera potentiellement splittee en sous-partie en fonction de la
	// partition issue du nouvel attribut
	for (nTargetValue = 0; nTargetValue < oaTargetPartsByTargetValueIndex.GetSize(); nTargetValue++)
	{
		targetPart = cast(SNBGroupTargetPart*, oaTargetPartsByTargetValueIndex.GetAt(nTargetValue));

		// Recherche de la clee numerique correspondant a la partie provenant du nouvel attribut
		nPartIndex = GetTargetValueGroupIndexAt(attribute, nTargetValue);
		targetPartIndex = cast(KWSortableIndex*, oaTargetPartIndexes.GetAt(nPartIndex));
		assert(targetPartIndex->GetIndex() == nPartIndex);

		// Recherche des sous-parties liee a la partie initiale
		nkdSubparts = cast(NumericKeyDictionary*, nkdInitialParts.Lookup(targetPart));

		// Creation de ce dictionnaire de sous-parties si necessaire, et dans ce cas
		// memorisation de l'ancienne partie comme sous-partie associee au groupe en cours
		newTargetPart = NULL;
		if (nkdSubparts == NULL)
		{
			nkdSubparts = new NumericKeyDictionary;
			nkdInitialParts.SetAt((NUMERIC)targetPart, nkdSubparts);

			// Memorisation de la premiere sous partie et de son effectif
			newTargetPart = targetPart;
			newTargetPart->SetFrequency(GetTargetValueFrequencyAt(nTargetValue));
			nkdSubparts->SetAt((NUMERIC)targetPartIndex, newTargetPart);

			// Mise a jour d'une signature par ajout d'un removedAttributeIndex de groupe
			UpdateSignatureWithAddedAttribute(attribute, nTargetValue, newTargetPart);
		}
		// Sinon, on recherche la sous-partie
		else
		{
			// Recherche de la sous-partie
			newTargetPart = cast(SNBGroupTargetPart*, nkdSubparts->Lookup((NUMERIC)targetPartIndex));

			// Creation si necessaire
			if (newTargetPart == NULL)
			{
				newTargetPart = cast(SNBGroupTargetPart*, GetOrCreatePart());

				// Gestion rudimentaire du manque de memoire
				if (newTargetPart == NULL)
					AddFatalError("Not enough working memory to optimize variable selection");

				// Initialisation de son vecteur de scores a partir de la partie dont elle est issue
				newTargetPart->GetScores()->CopyFrom(targetPart->GetScores());

				// Memorisation de la sous partie et de son effectif
				newTargetPart->SetFrequency(GetTargetValueFrequencyAt(nTargetValue));
				nkdSubparts->SetAt((NUMERIC)targetPartIndex, newTargetPart);

				// Ajout de la nouvelle partie dans la partition cible
				oaTargetParts.Add(newTargetPart);

				// Mise a jour d'une signature par ajout d'un removedAttributeIndex de groupe
				// On commence par recopier l'ancienne signature, en supprimant sa derniere composant
				assert(targetPart->GetSignature()->GetSize() == signatureSchema.GetSize() + 1);
				newTargetPart->GetSignature()->CopyFrom(targetPart->GetSignature());
				newTargetPart->GetSignature()->SetSize(newTargetPart->GetSignature()->GetSize() - 1);
				UpdateSignatureWithAddedAttribute(attribute, nTargetValue, newTargetPart);
			}
			// Mise a jour de l'effectif de la sous-partie
			else
				newTargetPart->SetFrequency(newTargetPart->GetFrequency() +
							    GetTargetValueFrequencyAt(nTargetValue));
		}
		assert(newTargetPart != NULL);

		// Memorisation de la nouvelle partie cible associe a la valeur cible
		oaTargetPartsByTargetValueIndex.SetAt(nTargetValue, newTargetPart);
	}

	// Mise a jour des specifications de signature
	signatureSchema.AddAttribute(attribute);

	// Nettoyage
	nkdInitialParts.DeleteAll();
	oaTargetPartIndexes.DeleteAll();

	ensure(signatureSchema.Contains(attribute));
	ensure(Check());
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::UpdateSignatureWithAddedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute, int nTargetValue, SNBGroupTargetPart* groupTargetPart) const
{
	require(0 <= nTargetValue and nTargetValue < GetTargetValueNumber());
	require(groupTargetPart->Check());
	require(CheckTargetSignature(groupTargetPart, nTargetValue));

	// Ajout de l'removedAttributeIndex de groupe en fin de signature
	groupTargetPart->GetSignature()->Add(GetTargetValueGroupIndexAt(attribute, nTargetValue));
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::UpdateTargetPartitionWithRemovedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute)
{
	int nTargetValue;
	SortedList slRemainingParts(KWGroupTargetPartCompareTargetSignature);
	POSITION position;
	SNBGroupTargetPart* groupTargetPart;
	SNBGroupTargetPart* newTargetPartScore;
	int nRemovedAttributeSignatureIndex;
	ObjectArray oaPartsToDelete;
	int nTargetPart;

	require(Check());
	require(signatureSchema.Contains(attribute));

	// Acces a l'removedAttributeIndex de l'attribut a supprimer dans la signature cible
	nRemovedAttributeSignatureIndex = signatureSchema.GetSignatureIndexAt(attribute);

	// Retaillage initiale de la partition cible
	oaTargetParts.SetSize(0);

	// Parcours des parties initiales referencees pour chaque valeur
	// Chaque partie initiale sera potentiellement simplifiee en fonction de la partition issue
	// du nouvel attribut, ce qui entrainera la supression des parties redondantes
	for (nTargetValue = 0; nTargetValue < oaTargetPartsByTargetValueIndex.GetSize(); nTargetValue++)
	{
		groupTargetPart = cast(SNBGroupTargetPart*, oaTargetPartsByTargetValueIndex.GetAt(nTargetValue));

		// Traitement de la partie, si non deja traitee (signature diminuee)
		if (groupTargetPart->GetSignature()->GetSize() == signatureSchema.GetSize())
		{
			// Mise a jour de la signature de la partie par supression d'un removedAttributeIndex de groupe
			UpgradeTargetSignatureWithRemovedAttribute(attribute, nTargetValue, groupTargetPart);
			assert(groupTargetPart->GetSignature()->GetSize() == signatureSchema.GetSize() - 1);

			// Recherche de la partie parmi les parties restantes
			position = slRemainingParts.Find(groupTargetPart);
			newTargetPartScore = NULL;
			if (position != NULL)
				newTargetPartScore = cast(SNBGroupTargetPart*, slRemainingParts.GetAt(position));

			// Memorisation si partie restante
			if (newTargetPartScore == NULL)
			{
				newTargetPartScore = groupTargetPart;
				slRemainingParts.Add(newTargetPartScore);

				// Memorisation de la partie dans la partition cible
				oaTargetParts.Add(newTargetPartScore);
			}
			// Consolidation de la partie restante sinon
			else
			{
				assert(newTargetPartScore != groupTargetPart);

				// Prise en compte de l'effectif de la partie a fusionner
				newTargetPartScore->SetFrequency(newTargetPartScore->GetFrequency() +
								 groupTargetPart->GetFrequency());

				// Memorisation de la nouvelle association
				oaTargetPartsByTargetValueIndex.SetAt(nTargetValue, newTargetPartScore);

				// Enregistrement de la partie a detruire
				// (elle est encore potentiellement referencee pour d'autre valeurs cibles)
				oaPartsToDelete.Add(groupTargetPart);
			}
		}
		// Traitement de la partie, si deja traitee
		else
		{
			assert(groupTargetPart->GetSignature()->GetSize() == signatureSchema.GetSize() - 1);

			// Recherche de la partie parmi les parties restantes
			position = slRemainingParts.Find(groupTargetPart);
			assert(position != NULL);
			newTargetPartScore = cast(SNBGroupTargetPart*, slRemainingParts.GetAt(position));

			// Memorisation de la nouvelle association
			oaTargetPartsByTargetValueIndex.SetAt(nTargetValue, newTargetPartScore);
		}
	}
	assert(slRemainingParts.GetCount() == oaTargetParts.GetSize());

	// Destruction effective des parties
	for (nTargetPart = 0; nTargetPart < oaPartsToDelete.GetSize(); nTargetPart++)
	{
		groupTargetPart = cast(SNBGroupTargetPart*, oaPartsToDelete.GetAt(nTargetPart));
		ReleasePart(groupTargetPart);
	}

	// Mise a jour des specification de signature
	signatureSchema.RemoveAttribute(attribute);

	ensure(Check());
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::UpgradeTargetSignatureWithRemovedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute, int nTargetValue, SNBGroupTargetPart* groupTargetPart) const
{
	int nAttributeSignature;

	require(0 <= nTargetValue and nTargetValue < GetTargetValueNumber());
	require(groupTargetPart->Check());
	require(CheckTargetSignature(groupTargetPart, nTargetValue));

	// La fin de signature remplace la place de l'attribut supprime
	nAttributeSignature = signatureSchema.GetSignatureIndexAt(attribute);
	groupTargetPart->GetSignature()->SetAt(
	    nAttributeSignature, groupTargetPart->GetGroupIndexAt(groupTargetPart->GetSignatureSize() - 1));
	groupTargetPart->GetSignature()->SetSize(groupTargetPart->GetSignatureSize() - 1);
}

boolean SNBGeneralizedClassifierSelectionDataCostCalculator::UpdateTargetPartScoresWithWeightedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute, Continuous cDeltaWeight)
{
	boolean bOk = true;
	SNBDataTableBinarySliceSetColumn* chunkColumn;
	int nSignatureAttributeIndex;
	int nTargetPart;
	SNBGroupTargetPart* targetPart;
	int nAttributeTargetPart;
	int nColumnValueNumber;
	int nColumnValueIndex;
	int nChunkInstance;
	int nChunkInstanceValue;
	Continuous cLogProb;

	// Acces a la colonne de donnees de l'attribut
	chunkColumn = NULL;
	bOk = bOk and GetDataTableBinarySliceSet()->GetAttributeColumnView(attribute, chunkColumn);

	// Index de l'attribut dans les specifications de la jointure
	nSignatureAttributeIndex = signatureSchema.GetSignatureIndexAt(attribute);

	// Mise a jour des scores des parties cibles
	if (bOk)
	{
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
		{
			targetPart = cast(SNBGroupTargetPart*, oaTargetParts.GetAt(nTargetPart));
			nAttributeTargetPart = targetPart->GetSignature()->GetAt(nSignatureAttributeIndex);

			// Mise a jour des scores de la partie cible courante seulement pour les valeurs presentes de la colonne sparse
			if (chunkColumn->IsSparse())
			{
				nColumnValueNumber = chunkColumn->GetValueNumber();
				for (nColumnValueIndex = 0; nColumnValueIndex < nColumnValueNumber; nColumnValueIndex++)
				{
					nChunkInstance = chunkColumn->GetSparseValueInstanceIndexAt(nColumnValueIndex);
					nChunkInstanceValue = chunkColumn->GetSparseValueAt(nColumnValueIndex);
					cLogProb = attribute->GetLnSourceConditionalProb(nChunkInstanceValue,
											 nAttributeTargetPart);
					targetPart->GetScores()->UpgradeAt(nChunkInstance, cDeltaWeight * cLogProb);
				}
			}
			// Mise a jour des scores de la partie cible pour une colonne dense
			else
			{
				nColumnValueNumber = chunkColumn->GetValueNumber();
				for (nChunkInstance = 0; nChunkInstance < nColumnValueNumber; nChunkInstance++)
				{
					nChunkInstanceValue = chunkColumn->GetDenseValueAt(nChunkInstance);
					cLogProb = attribute->GetLnSourceConditionalProb(nChunkInstanceValue,
											 nAttributeTargetPart);
					targetPart->GetScores()->UpgradeAt(nChunkInstance, cDeltaWeight * cLogProb);
				}
			}
		}
	}
	return bOk;
}

boolean SNBGeneralizedClassifierSelectionDataCostCalculator::UpdateDataCost()
{
	const boolean bLocalTrace = false;
	boolean bOk = true;
	int nTargetPart;
	int nChunkInstanceNumber;
	int nChunkInstance;
	double dInstanceNonNormalizedDataCost;

	// Entete de trace de debbogage
	if (bLocalTrace)
	{
		cout << "\tInstance\tTarget\tTargetPart\tProb\tCost";
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
			cout << "\tScore" << nTargetPart + 1;
		cout << "\n";
	}

	// Mise-a-jour du score en recalculant tous les couts par instance
	dSelectionDataCost = 0.0;
	nChunkInstanceNumber = GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber();
	for (nChunkInstance = 0; nChunkInstance < nChunkInstanceNumber; nChunkInstance++)
	{
		dInstanceNonNormalizedDataCost = ComputeInstanceNonNormalizedDataCost(nChunkInstance);
		dSelectionDataCost += dInstanceNonNormalizedDataCost;
		dvInstanceNonNormalizedDataCosts.SetAt(nChunkInstance, dInstanceNonNormalizedDataCost);
	}
	dSelectionDataCost += nChunkInstanceNumber * log(dLaplaceDenominator);

	return bOk;
}

boolean SNBGeneralizedClassifierSelectionDataCostCalculator::UpdateDataCostWithSparseAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute)
{
	const boolean bLocalTrace = false;
	boolean bOk = true;
	int nTargetPart;
	int nChunkColumnValueNumber;
	int nChunkColumnValue;
	int nChunkInstance;
	SNBDataTableBinarySliceSetColumn* chunkColumn;
	double dOldInstanceNonNormalizedDataCost;
	double dInstanceNonNormalizedDataCost;

	require(attribute != NULL);
	require(attribute->IsSparse());
	require(GetDataTableBinarySliceSet()->ContainsAttribute(attribute));

	// Entete de trace de debbogage
	if (bLocalTrace)
	{
		cout << "\tInstance\tTarget\tTargetPart\tProb\tCost";
		for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
			cout << "\tScore" << nTargetPart + 1;
		cout << "\n";
	}

	// Mise-a-jour du score en calculant seulement le delta dans les instances non-manquantes
	// Nota: Pas necessaire de normaliser avec le denominateur de Laplace car on ne recommence pas de
	// zero comme dans le cas dense
	chunkColumn = NULL;
	bOk = bOk and GetDataTableBinarySliceSet()->GetAttributeColumnView(attribute, chunkColumn);
	if (bOk)
	{
		nChunkColumnValueNumber = chunkColumn->GetValueNumber();
		for (nChunkColumnValue = 0; nChunkColumnValue < nChunkColumnValueNumber; nChunkColumnValue++)
		{
			nChunkInstance = chunkColumn->GetSparseValueInstanceIndexAt(nChunkColumnValue);
			dOldInstanceNonNormalizedDataCost = dvInstanceNonNormalizedDataCosts.GetAt(nChunkInstance);
			dInstanceNonNormalizedDataCost = ComputeInstanceNonNormalizedDataCost(nChunkInstance);
			dSelectionDataCost += dInstanceNonNormalizedDataCost - dOldInstanceNonNormalizedDataCost;
			dvInstanceNonNormalizedDataCosts.SetAt(nChunkInstance, dInstanceNonNormalizedDataCost);
		}
	}
	return bOk;
}

SNBTargetPart* SNBGeneralizedClassifierSelectionDataCostCalculator::CreatePart()
{
	return new SNBGroupTargetPart;
}

boolean SNBGeneralizedClassifierSelectionDataCostCalculator::Check() const
{
	boolean bOk = true;
	int nTargetValue;
	SNBGroupTargetPart* groupTargetPart;
	int nGroupTargetPart;
	NumericKeyDictionary nkdGroupTargetPartSet;
	SortedList slCheckedTargetPartition(KWGroupTargetPartCompareTargetSignature);
	int nTotalFrequency;
	ALString sTmp;

	// Verification que on est bien dans le cas discret
	bOk = bOk and learningSpec->GetTargetAttributeType() == KWType::Symbol;

	// Collecte des parties par parcours des parties references pour chaque valeur
	for (nTargetValue = 0; nTargetValue < oaTargetPartsByTargetValueIndex.GetSize(); nTargetValue++)
	{
		groupTargetPart = cast(SNBGroupTargetPart*, oaTargetPartsByTargetValueIndex.GetAt(nTargetValue));

		// Verification des signatures des parties
		if (not CheckTargetSignature(groupTargetPart, nTargetValue))
		{
			AddError(sTmp + "Target part (" + GetTargetValueAt(nTargetValue) + ") has a wrong signature " +
				 groupTargetPart->GetObjectLabel());
			bOk = false;
			break;
		}

		// Memorisation de toute partie non deja referencee
		if (nkdGroupTargetPartSet.Lookup(groupTargetPart) == NULL)
		{
			// Enregistrement dans un dictionnaire
			nkdGroupTargetPartSet.SetAt((NUMERIC)groupTargetPart, groupTargetPart);

			// Verification de l'unicite des signatures en utilisant une liste triee
			if (slCheckedTargetPartition.Find(groupTargetPart) == NULL)
				slCheckedTargetPartition.Add(groupTargetPart);
			else
			{
				AddError(sTmp + "Target part (" + GetTargetValueAt(nTargetValue) +
					 ") has a signature " + groupTargetPart->GetObjectLabel() + " already used");
				bOk = false;
				break;
			}
		}
	}

	// Tests supplementaires
	if (bOk)
	{
		// Verification de la taille de la partition
		bOk = bOk and oaTargetParts.NoNulls();
		if (oaTargetParts.GetSize() != nkdGroupTargetPartSet.GetCount())
		{
			AddError(sTmp + "Number of different target parts (" +
				 IntToString(nkdGroupTargetPartSet.GetCount()) +
				 ") should be equal to the size of the target partition (" +
				 IntToString(oaTargetParts.GetSize()) + ")");
			bOk = false;
		}

		// Si la base est vide, il ne doit y avoir qu'une partie cible
		if (GetInstanceNumber() == 0)
		{
			if (oaTargetParts.GetSize() != 1)
			{
				AddError("Target partition structure should contain one single part");
				bOk = false;
			}
		}
		// Verification de la structure sinon
		else
		{
			// Verification du nombre de parties
			if (oaTargetParts.GetSize() > GetTargetDescriptiveStats()->GetValueNumber())
			{
				AddError(sTmp + "Number of target parts (" + IntToString(oaTargetParts.GetSize()) +
					 ") should not be greater than the number of class values (" +
					 IntToString(GetTargetDescriptiveStats()->GetValueNumber()) + ")");
				bOk = false;
			}

			// Verification des parties
			nTotalFrequency = 0;
			for (nGroupTargetPart = 0; nGroupTargetPart < oaTargetParts.GetSize(); nGroupTargetPart++)
			{
				groupTargetPart = cast(SNBGroupTargetPart*, oaTargetParts.GetAt(nGroupTargetPart));
				bOk = bOk and groupTargetPart != NULL;
				bOk = bOk and nkdGroupTargetPartSet.Lookup((NUMERIC)groupTargetPart) != NULL;

				// Mise a jour de l'effectif total
				bOk = bOk and 0 < groupTargetPart->GetFrequency() and
				      groupTargetPart->GetFrequency() <= GetInstanceNumber();
				nTotalFrequency += groupTargetPart->GetFrequency();

				// Verification de la partie
				if (groupTargetPart->GetScores()->GetSize() !=
				    GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber())
				{
					AddError(
					    sTmp + "Size of score vector (" +
					    IntToString(groupTargetPart->GetScores()->GetSize()) + ") " +
					    "is inconsistent with the number active instances of the binary slice set "
					    "(" +
					    IntToString(
						GetDataTableBinarySliceSet()->GetInitializedChunkInstanceNumber()) +
					    ")");
					bOk = false;
				}
				if (not bOk)
					break;
			}

			// Verification de l'effectif total
			if (bOk and nTotalFrequency != GetInstanceNumber())
			{
				AddError(sTmp + "Sum of the sizes of the parts (" + IntToString(nTotalFrequency) +
					 ") is inconsistent with the database size (" +
					 IntToString(GetInstanceNumber()) + ")");
				bOk = false;
			}
		}
	}

	return bOk;
}

boolean SNBGeneralizedClassifierSelectionDataCostCalculator::CheckTargetSignature(SNBGroupTargetPart* groupTargetPart,
										  int nTargetValue) const
{
	boolean bOk = true;
	int nSignature;
	SNBDataTableBinarySliceSetAttribute* signatureAttribute;

	require(0 <= nTargetValue and nTargetValue < GetTargetValueNumber());

	// La taille de la signature de la partie doit etre de la meme taille que l'ensemble d'attributs qui l'specifient
	bOk = bOk and groupTargetPart->GetSignatureSize() == signatureSchema.GetSize();

	// Coherence des indices des groupes pour chaque attribut dans la signature
	if (bOk)
	{
		for (nSignature = 0; nSignature < signatureSchema.GetSize(); nSignature++)
		{
			signatureAttribute = signatureSchema.GetAttributeAt(nSignature);
			bOk = bOk and groupTargetPart->GetGroupIndexAt(nSignature) ==
					  GetTargetValueGroupIndexAt(signatureAttribute, nTargetValue);
		}
	}
	return bOk;
}

int SNBGeneralizedClassifierSelectionDataCostCalculator::GetTargetValueGroupIndexAt(
    const SNBDataTableBinarySliceSetAttribute* attribute, int nTargetValue) const
{
	const IntVector* ivAttributeTargetValueGroupMatching;

	require(GetDataTableBinarySliceSet()->ContainsAttribute(attribute));
	require(0 <= nTargetValue && nTargetValue < GetTargetValueNumber());

	ivAttributeTargetValueGroupMatching =
	    cast(const IntVector*, nkdTargetValueGroupMatchingsByAttribute.Lookup((NUMERIC)attribute));
	return ivAttributeTargetValueGroupMatching->GetAt(nTargetValue);
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::Write(ostream& ost) const
{
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;
	int nTargetValue;
	int nTargetPart;
	SNBTargetPart* targetPart;

	require(Check());

	ost << GetClassLabel() << "\n";
	ost << "Signature attribute matchings\n";
	ost << "Attribute\t[target value id -> attribute group id]\n";
	for (nAttribute = 0; nAttribute < signatureSchema.GetSize(); nAttribute++)
	{
		attribute = signatureSchema.GetAttributeAt(nAttribute);

		ost << attribute->GetNativeAttributeName() << "\t";
		ost << "(";
		for (nTargetValue = 0; nTargetValue < GetTargetValueNumber(); nTargetValue++)
		{
			ost << GetTargetValueGroupIndexAt(attribute, nTargetValue);
			if (nTargetValue < GetTargetValueNumber() - 1)
				ost << ", ";
		}
		ost << ")\n";
	}

	ost << "\nGroup Target Parts\n";
	for (nTargetPart = 0; nTargetPart < oaTargetParts.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBTargetPart*, oaTargetParts.GetAt(nTargetPart));
		if (nTargetPart == 0)
		{
			targetPart->WriteHeaderLineReport(ost);
			ost << "\n";
		}
		targetPart->WriteLineReport(ost);
		ost << "\n";
	}
}

longint SNBGeneralizedClassifierSelectionDataCostCalculator::ComputeNecessaryMemory(int nInstanceNumber,
										    int nAttributeNumber,
										    int nTargetValueNumber)
{
	ObjectArray oaDummy;
	ObjectList olDummy;
	NumericKeyDictionary nkdDummy;

	// Formule pour l'estimation :
	//   Objets de l'instance
	//   Max d'objets SNBGroupTargetPart
	//   Max overhead des conteneurs des SNBGroupTargetPart's (oaTargetParts, olReleasedPartsCache, oaTargetPartsByTargetValueIndex)
	//   Dictionnaire de groups matchings (nkdAttributeTargeValueGroupMatchings)
	//   Schema des signatures des parties cibles
	return sizeof(SNBGeneralizedClassifierSelectionDataCostCalculator) +
	       nTargetValueNumber * SNBGroupTargetPart::ComputeNecessaryMemory(nInstanceNumber, nAttributeNumber) +
	       nTargetValueNumber * (2 * oaDummy.GetUsedMemoryPerElement() + olDummy.GetUsedMemoryPerElement()) +
	       nAttributeNumber * (nkdDummy.GetUsedMemoryPerElement() + sizeof(IntVector) +
				   (longint)nTargetValueNumber * sizeof(int)) +
	       SNBGroupTargetPartSignatureSchema::ComputeNecessaryMemory(nAttributeNumber);
}
