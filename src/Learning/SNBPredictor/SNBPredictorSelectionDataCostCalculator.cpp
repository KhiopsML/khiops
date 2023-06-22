// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBPredictorSelectionDataCostCalculator.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBPredictorSelectionDataCostCalculator

SNBPredictorSelectionDataCostCalculator::SNBPredictorSelectionDataCostCalculator()
{
	binarySliceSet = NULL;
	bDisplay = false;
	dInstanceNumber = 0.0;
	cMaxScore = DBL_MAX;
	dMaxExpScore = DBL_MAX;
	dLaplaceEpsilon = 0.0;
	dLaplaceDenominator = 1.0;
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

void SNBPredictorSelectionDataCostCalculator::SetDisplay(boolean bValue)
{
	bDisplay = bValue;
}

boolean SNBPredictorSelectionDataCostCalculator::GetDisplay() const
{
	return bDisplay;
}

SNBTargetPart* SNBPredictorSelectionDataCostCalculator::GetOrCreatePart()
{
	SNBTargetPart* targetPart;
	boolean bAllocOk;

	require(GetDataTableBinarySliceSet() != NULL);
	require(GetDataTableBinarySliceSet()->IsPartiallyInitialized());

	// S'il y a un objet SNBTargetPart disponible dans le cache on le retourne
	if (olDeletedPartsCache.GetCount() > 0)
		targetPart = cast(SNBTargetPart*, olDeletedPartsCache.RemoveHead());
	// Sinon initialisation du vecteur de scores de la SNBTargetPart; on renvoie NULL si l'allocation echoue
	else
	{
		targetPart = CreatePart();
		bAllocOk =
		    targetPart->GetScores()->SetLargeSize(GetDataTableBinarySliceSet()->GetActiveInstanceNumber());
		if (not bAllocOk)
		{
			delete targetPart;
			targetPart = NULL;
		}
	}
	return targetPart;
}

void SNBPredictorSelectionDataCostCalculator::ReleasePart(SNBTargetPart* targetPart)
{
	require(targetPart != NULL);
	require(GetDataTableBinarySliceSet() != NULL);
	require(GetDataTableBinarySliceSet()->IsPartiallyInitialized());
	require(targetPart->GetScores()->GetSize() == GetDataTableBinarySliceSet()->GetActiveInstanceNumber());

	// On garde dans le cache la SNBTargetPart eliminee pour une utilisation ulteriure si besoin
	olDeletedPartsCache.AddHead(targetPart);
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

int SNBPredictorSelectionDataCostCalculator::GetTargetValueNumber() const
{
	const KWDGSAttributeSymbolValues* targetValues;

	require(GetTargetValueStats()->GetAttributeNumber() == 1);
	require(GetTargetValueStats()->GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
	require(GetTargetValueStats()->GetAttributeAt(0)->ArePartsSingletons());

	targetValues = cast(const KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0));
	return targetValues->GetPartNumber();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBTargetPart

SNBTargetPart::SNBTargetPart() {}

SNBTargetPart::~SNBTargetPart() {}

ContinuousVector* SNBTargetPart::GetScores()
{
	return &cvScores;
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
	int nTarget;
	SNBTargetPart* targetPart;

	require(learningSpec->GetTargetAttributeType() == KWType::Symbol);
	require(oaTargetPartition.GetSize() == 0);

	// Creation d'autant de vecteurs de probabilites conditionnelles que de classes cibles
	bOk = oaTargetPartition.SetLargeSize(GetTargetDescriptiveStats()->GetValueNumber());
	if (bOk)
	{
		for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
		{
			// Memorisation si allocation reussie
			targetPart = GetOrCreatePart();
			if (targetPart != NULL)
				oaTargetPartition.SetAt(nTarget, targetPart);
			// Sinon nettoyage et echec
			else
			{
				oaTargetPartition.DeleteAll();
				bOk = false;
				break;
			}
		}
	}
	ensure(not bOk or Check());
	return bOk;
}

void SNBClassifierSelectionDataCostCalculator::Initialize()
{
	int nTargetValueNumber;
	int nTargetValue;
	SNBSingletonTargetPart* singletonTargetPart;
	double dTargetPartEmptyDataCost;
	Continuous cTargetPartEmptyDataCost;
	int nInstance;

	require(GetTargetAttributeType() == KWType::Symbol);
	require(oaTargetPartition.GetSize() == GetTargetDescriptiveStats()->GetValueNumber());

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

	// Calcul du cout de la selection vide pour la modalite cible courante et memorisation pour chaque instance
	// Le cout de selection vide pour la modalite j est tout simplement le a priori log P(Y_j)
	for (nTargetValue = 0; nTargetValue < nTargetValueNumber; nTargetValue++)
	{
		singletonTargetPart = cast(SNBSingletonTargetPart*, oaTargetPartition.GetAt(nTargetValue));

		singletonTargetPart->SetTargetValueIndex(nTargetValue);

		assert(GetTargetValueStats()->GetUnivariateCellFrequencyAt(nTargetValue) > 0);
		dTargetPartEmptyDataCost =
		    GetTargetValueStats()->GetUnivariateCellFrequencyAt(nTargetValue) * 1.0 / GetInstanceNumber();
		cTargetPartEmptyDataCost = (Continuous)log(dTargetPartEmptyDataCost);
		for (nInstance = 0; nInstance < GetDataTableBinarySliceSet()->GetActiveInstanceNumber(); nInstance++)
			singletonTargetPart->GetScores()->SetAt(nInstance, cTargetPartEmptyDataCost);
	}
}

void SNBClassifierSelectionDataCostCalculator::Delete()
{
	oaTargetPartition.DeleteAll();
	olDeletedPartsCache.DeleteAll();
}

double SNBClassifierSelectionDataCostCalculator::ComputeSelectionDataCost()
{
	const boolean bLocalTrace = false;
	int nTargetValueNumber;
	int nActiveInstanceNumber;
	double dDataCost;
	int nInstance;
	int nActualTargetValue;
	SNBTargetPart* actualTargetPart;
	Continuous cInstanceActualScore;
	double dInstanceInverseProb;
	int nTargetValue;
	Continuous cDeltaScore;
	SNBTargetPart* targetPart;
	double dLaplaceNumerator;

	require(Check());
	require(GetDataTableBinarySliceSet()->IsPartiallyInitialized());

	// Entete de la trace
	if (bLocalTrace)
	{
		cout << "\nIndex\tTarget\tProb\tValue\tEvaluation";
		for (nTargetValue = 0; nTargetValue < GetTargetDescriptiveStats()->GetValueNumber(); nTargetValue++)
			cout << "\tScore" << nTargetValue + 1;
		cout << "\n";
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// L'ecriture de cette methode, tres souvent utilisee, a ete particulierement optimisee :
	//  - memorisation dans des variables locales de donnees de travail
	//  - precalcul des parties constantes des couts (dans Initialize)
	//  - memorisation dans des variables locales les limites des boucles (pour eviter leur reevaluation)
	//
	// Rationale de l'algorithme pour l'estmation de couts de donnees :
	// Rappelons que le score non-normalise pour l'instance i et la modalite j est donnee par
	//
	//   score_ij = -log P(Y = C_j) + Sum_k w_k log P( X_ik | Y = C_j ).
	//
	// Notons que modalite j est associee a un objet KWSingletonPart qui contient son vecteur de scores.
	// Pour le calcul, d'abord on ecrit la "vraisemblance ponderee" (voir nota ci-dessous)
	//  de l'instance i via un softmax :
	//
	//   P( Y = C_m | X ) ~  P(Y = C_m) * Prod_k P(X_ik | Y = C_m)^w_k
	//        (regress)   ~  P(Rang(Y) = R_m) * Prod_k P(X_ik | Y = R_m)^w_k
	//                    ~  P(Y in Part(R_m)) * Prod_k P(X_ik | Y in Part(R_m))

	//        (group)     ~  P(Y = C_m | Y in G_l) P( Y in G_l ) * Prod_k P( X_ik | Y in G_l)

	//                    ~   (#C_m)/(# Inst. in G_l) * (# Inst. in G_l)/N * Prod_k( X_ik | Y in G_l)

	//                    = exp(score_im) / Sum_j exp(score_ij)

	// P(Rang(Y) = R_m) = P(Y in I_l) * P(Rang(Y) = R_m | Y in P_l)
	//                  = 1/N_l * P(Rang(Y) = R_m | Y in P_l)

	// ou l'indice m = m(i) est tel que la valeur de Y dans l'instance i est C_m, i.e y_i = C_m.
	// Maintenant on passe le numerateur au denominateur
	//
	//   P( Y = C_m | X ) = 1 / Sum_j exp(score_ij - score_im)
	//                    = 1 / (1 + Sum_{j != m} exp(score_ij - score_im))
	//
	// Et donc le cout de l'instance est donnee par
	//
	//  cout_i = -log P( Y = C_m | X ) = log (1 + Sum_{j != m} exp(score_ij - score_im))        (1)
	//
	// Pour l'estabilite numerique, avant de passer au dernier log, on regularise la quantite
	// exp(cout_i) par la methode de Laplace
	//
	//    exp(cout_i) -> (exp(cout_i) * N + epsilon) / (N + J * epsilon)                        (2)
	//
	// ou l'epsilon de Laplace a ete calcule pendant l'initialisation. On define S_i et T comme le
	// numerateur et le denominateur de formule de Laplace (2). On utilise aussi une exponentielle tronque
	// pour eviter des overflows :
	//
	//   exp_tr(x) = min(exp(x), dMaxExpScore)
	//
	// ou dMaxExpScore a ete pre-calculee dans l'initialisation.
	//
	// Pour chaque instance i, l'algorithme
	// ci-dessous calcule d'abord l'inverse exp(-cout_i) en parcourant toutes les modalites cibles.
	// Ensuite il applique l'approximation de Laplace pour obtenir S_i pour chaque instance i. Un fois
	// parcourues toutes les instances on rend
	//
	//   DataCost = - Sum_i log(S_i) + N * log(T)
	//
	// Nota : P( Y = C_m | X ) n'est pas une probabilite sauf si tous les w_k == 1

	// Calcul de la somme des numerateurs de Laplace
	nTargetValueNumber = GetTargetDescriptiveStats()->GetValueNumber();
	nActiveInstanceNumber = GetDataTableBinarySliceSet()->GetActiveInstanceNumber();
	dDataCost = 0;
	for (nInstance = 0; nInstance < nActiveInstanceNumber; nInstance++)
	{
		// Recherche du vecteur de probabilite pour la classe cible reelle
		nActualTargetValue = GetDataTableBinarySliceSet()->GetTargetValueIndexAtActiveInstance(nInstance);
		actualTargetPart = cast(SNBSingletonTargetPart*, oaTargetPartition.GetAt(nActualTargetValue));
		cInstanceActualScore = actualTargetPart->GetScores()->GetAt(nInstance);

		dInstanceInverseProb = 0.0;
		for (nTargetValue = 0; nTargetValue < nTargetValueNumber; nTargetValue++)
		{
			// Cas cible egal a la cible reelle de l'instance : contribution de 1 (pas d'appel a std::exp,
			// voir formule (1))
			if (nTargetValue == nActualTargetValue)
				dInstanceInverseProb += 1.0;
			// Cas general : Calcul complet (utilise un appel a la fonction std::exp)
			else
			{
				targetPart = cast(SNBSingletonTargetPart*, oaTargetPartition.GetAt(nTargetValue));

				// Difference de score pour la classe j
				cDeltaScore = targetPart->GetScores()->GetAt(nInstance) - cInstanceActualScore;

				// Prise en compte de l'exponentielle, en tenant compte du seuil de validite
				dInstanceInverseProb += cDeltaScore >= cMaxScore ? dMaxExpScore : exp(cDeltaScore);
			}
		}
		assert(dInstanceInverseProb >= 1);

		// Mise a jour de cette probabilite en tenant compte de l'estimateur de Laplace
		dLaplaceNumerator = dInstanceNumber / dInstanceInverseProb + dLaplaceEpsilon;
		assert(0 < dLaplaceNumerator and dLaplaceNumerator < dLaplaceDenominator);

		// Mise a jour de l'evaluation globale
		dDataCost -= log(dLaplaceNumerator);

		// Ligne de trace
		if (bLocalTrace)
		{
			cout << nInstance << "\t" << nActualTargetValue << "\t"
			     << dLaplaceNumerator / dLaplaceDenominator << "\t"
			     << -log(dLaplaceNumerator / dLaplaceDenominator) << "\t" << dDataCost;
			for (nTargetValue = 0; nTargetValue < GetTargetDescriptiveStats()->GetValueNumber();
			     nTargetValue++)
			{
				targetPart = cast(SNBTargetPart*, oaTargetPartition.GetAt(nTargetValue));
				cout << "\t" << targetPart->GetScores()->GetAt(nInstance);
			}
			cout << "\n";
		}
	}

	// Ajout des denominateurs de Laplace au cout
	dDataCost += GetDataTableBinarySliceSet()->GetActiveInstanceNumber() * log(dLaplaceDenominator);

	return dDataCost;
}

void SNBClassifierSelectionDataCostCalculator::UpdateTargetPartitionWithAddedAttribute(
    SNBDataTableBinarySliceSetAttribute* attribute)
{
	require(attribute != NULL);
	require(GetDataTableBinarySliceSet() != NULL);
	require(Check());

	// Pas d'impact sur la structure de la partition cible
}

void SNBClassifierSelectionDataCostCalculator::UpdateTargetPartitionWithRemovedAttribute(
    SNBDataTableBinarySliceSetAttribute* attribute)
{
	require(attribute != NULL);
	require(GetDataTableBinarySliceSet() != NULL);
	require(Check());

	// Pas d'impact sur la structure de la partition cible
}

boolean SNBClassifierSelectionDataCostCalculator::UpdateTargetPartScoresWithWeightedAttribute(
    SNBDataTableBinarySliceSetAttribute* attribute, Continuous cWeight)
{
	boolean bOk = true;
	int nTargetPart;
	SNBSingletonTargetPart* singletonTargetPart;

	require(attribute != NULL);
	require(GetDataTableBinarySliceSet() != NULL);
	require(GetDataTableBinarySliceSet()->IsPartiallyInitialized());
	require(Check());

	// Calcul des probabilites conditionnelles par classe cible
	for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
	{
		singletonTargetPart = cast(SNBSingletonTargetPart*, oaTargetPartition.GetAt(nTargetPart));

		// Calcul du nouveau score dans toutes les instances
		bOk = bOk and GetDataTableBinarySliceSet()->UpdateTargetValueScores(attribute, nTargetPart, cWeight,
										    singletonTargetPart->GetScores());
		if (not bOk)
			break;
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
		if (oaTargetPartition.GetSize() > 0)
		{
			AddError("Database is empty but the partition is not");
			bOk = false;
		}
	}
	// Verification de la structure sinon
	else if (bOk and GetInstanceNumber() > 0)
	{
		// Verification du nombre de parties
		if (oaTargetPartition.GetSize() != GetTargetDescriptiveStats()->GetValueNumber())
		{
			AddError(sTmp + "Number of target parts (" + IntToString(oaTargetPartition.GetSize()) +
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
	int nTarget;
	SNBTargetPart* targetPart;
	ALString sTmp;

	for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
	{
		targetPart = cast(SNBTargetPart*, oaTargetPartition.GetAt(nTarget));
		if (targetPart == NULL)
		{
			AddError(sTmp + "Part at index " + IntToString(nTarget) + " is NULL");
			bOk = false;
		}

		if (bOk and
		    targetPart->GetScores()->GetSize() != GetDataTableBinarySliceSet()->GetActiveInstanceNumber())
		{
			AddError(sTmp + "Size of target part probability vector (" +
				 IntToString(targetPart->GetScores()->GetSize()) +
				 ") is not equal to the sub-database size (" +
				 IntToString(GetDataTableBinarySliceSet()->GetActiveInstanceNumber()) + ")");
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
		targetPart = cast(SNBTargetPart*, oaTargetPartition.GetAt(nTarget));
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
	//   Max overhead des conteneurs des SNBGroupTargetPart's (oaTargetPartition & olDeletedPartsCache)
	return sizeof(SNBClassifierSelectionDataCostCalculator) +
	       nTargetPartNumber * SNBSingletonTargetPart::ComputeNecessaryMemory(nInstanceNumber) +
	       nTargetPartNumber * (oaDummy.GetUsedMemoryPerElement() + olDummy.GetUsedMemoryPerElement());
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
		bOk = oaTargetPartition.SetLargeSize(GetInstanceNumber());

	// Initialisation de la taille du vecteur d'removedAttributeIndex des partie cibles
	if (bOk)
		bOk = ivTargetPartIndexes.SetLargeSize(GetInstanceNumber());

	ensure(not bOk or Check());
	return bOk;
}

void SNBRegressorSelectionDataCostCalculator::Initialize()
{
	SNBIntervalTargetPart* intervalTargetPart;

	require(GetTargetAttributeType() == KWType::Continuous);
	require(olTargetPartition.GetCount() >= 1);
	require(oaTargetPartition.GetSize() >= 1);
	require(ivTargetPartIndexes.GetSize() == GetInstanceNumber());

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

	// Redimensionnenement eventuel de la partition jusqu'a obtenir une taille de 1
	while (olTargetPartition.GetCount() > 1)
	{
		// Supression de la derniere partie
		intervalTargetPart = cast(SNBIntervalTargetPart*, olTargetPartition.RemoveTail());
		ReleasePart(intervalTargetPart);
	}

	// Reinitialisation de l'unique partie en cours
	intervalTargetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetHead());
	intervalTargetPart->SetFrequency(GetInstanceNumber());
	intervalTargetPart->SetCumulativeFrequency(intervalTargetPart->GetFrequency());
	intervalTargetPart->SetRefCount(1);

	// Initialisation des probabilites conditionnelles a 1 (Ln(prob) a 0)
	assert(intervalTargetPart->GetScores()->GetSize() == GetDataTableBinarySliceSet()->GetActiveInstanceNumber());
	intervalTargetPart->GetScores()->Initialize();
}

void SNBRegressorSelectionDataCostCalculator::Delete()
{
	olTargetPartition.DeleteAll();
	olDeletedPartsCache.DeleteAll();
	oaTargetPartition.SetSize(0);
}

double SNBRegressorSelectionDataCostCalculator::ComputeSelectionDataCost()
{
	boolean bLocalDisplay = false;
	double dDataCost;
	Continuous cInstanceActualScore;
	int nTargetPart;
	int nActualTarget;
	int nActualTargetPart;
	SNBIntervalTargetPart* targetPart;
	SNBIntervalTargetPart* actualTargetPart;
	int nInstance;
	double dInstanceInverseProb;
	Continuous cDeltaScore;
	double dLaplaceNumerator;

	require(Check());

	// Transformation de la liste en tableau pour en accelerer l'acces dans cette methode
	olTargetPartition.ExportObjectArray(&oaTargetPartition);

	// Mise a jour de la relation [index instance -> index partie cible]
	nInstance = 0;
	for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBIntervalTargetPart*, oaTargetPartition.GetAt(nTargetPart));

		// Tous les valeurs de la cible sont associees a cette partie
		while (nInstance < targetPart->GetCumulativeFrequency())
		{
			ivTargetPartIndexes.SetAt(nInstance, nTargetPart);
			nInstance++;
		}
	}

	// Entete de l'affichage
	if (bLocalDisplay)
	{
		cout << "\nIndex\tTarget index\tInterval\tProb\tValue\tEvaluation";
		for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
			cout << "\tScore" << nTargetPart + 1;
		cout << "\n";
	}

	// Parcours synchronise de la partition cible courante et des instances pour calculer
	// les log vraissemblances negatives par instance
	dDataCost = 0;
	for (nInstance = 0; nInstance < GetDataTableBinarySliceSet()->GetActiveInstanceNumber(); nInstance++)
	{
		// Recherche de l'removedAttributeIndex de la partie cible associe a l'instance
		nActualTarget = GetDataTableBinarySliceSet()->GetTargetValueIndexAtActiveInstance(nInstance);
		nActualTargetPart = ivTargetPartIndexes.GetAt(nActualTarget);

		// Acces au numerateur pour l'instance
		actualTargetPart = cast(SNBIntervalTargetPart*, oaTargetPartition.GetAt(nActualTargetPart));
		cInstanceActualScore = actualTargetPart->GetScores()->GetAt(nInstance);

		// On utilise les formules suivantes pour les calculs de probabilites conditionnelles par rang
		//  score(n) = \sum_i{log(P(X_i|Y_j))}
		// Comme la probabilite est la meme dans chaque intervalle j, il suffit quand on normalise en
		// sommant les probabilites au denominateurs de multiplier la proba par l'effectif de l'intervalle
		//  P(Y_n|X) = 1 / sum_j'{N_j'*exp(score(j')-score(j))
		//  -log(P(Y_n|X)) = log(sum_j'{N_j'*exp(score(j')-score(j)))
		// Il suffit alors de parcourir les intervalles cibles

		// P(Y_n in P_j') = 1/N_j'

		dInstanceInverseProb = 0.0;
		for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
		{
			// Cas particulier pour eviter les calculs
			if (nTargetPart == nActualTarget)
				dInstanceInverseProb += actualTargetPart->GetFrequency();
			// Cas general, impliquant un calcul d'exponentiel
			else
			{
				targetPart = cast(SNBIntervalTargetPart*, oaTargetPartition.GetAt(nTargetPart));

				// Difference de score pour l'intervalle cible en cours
				cDeltaScore = targetPart->GetScores()->GetAt(nInstance) - cInstanceActualScore;

				// Prise en compte de l'exponentielle, en tenant compte du seuil de validite
				dInstanceInverseProb += targetPart->GetFrequency() *
							(cDeltaScore >= cMaxScore ? dMaxExpScore : exp(cDeltaScore));
			}
		}
		assert(dInstanceInverseProb >= 1);

		// Mise a jour de cette probabilite en tenant compte de l'estimateur de Laplace
		dLaplaceNumerator = dInstanceNumber / dInstanceInverseProb + dLaplaceEpsilon;
		assert(0 < dLaplaceNumerator and dLaplaceNumerator < dLaplaceDenominator);

		// Mise a jour de l'evaluation globale
		dDataCost -= log(dLaplaceNumerator);

		// Ligne de trace
		if (bLocalDisplay)
		{
			cout << nInstance << "\t" << nActualTarget << "\t" << dLaplaceNumerator / dLaplaceDenominator
			     << "\t" << -log(dLaplaceNumerator / dLaplaceDenominator) << "\t" << dDataCost;
			for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
			{
				targetPart = cast(SNBIntervalTargetPart*, oaTargetPartition.GetAt(nTargetPart));
				cout << "\t" << targetPart->GetScores()->GetAt(nInstance);
			}
			cout << "\n";
		}
	}

	// Mise a jour de l'evaluation globale (partie denominateurs)
	dDataCost += GetDataTableBinarySliceSet()->GetActiveInstanceNumber() * log(dLaplaceDenominator);

	return dDataCost;
}

void SNBRegressorSelectionDataCostCalculator::UpdateTargetPartitionWithAddedAttribute(
    SNBDataTableBinarySliceSetAttribute* attribute)
{
	POSITION position;
	POSITION targetPosition;
	IntVector ivTargetPartFrequencies;
	int nTarget;
	int nTargetFrequency;
	int nTargetCumulativeFrequency;
	SNBIntervalTargetPart* targetPart;
	SNBIntervalTargetPart* newTargetPart;

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

				// Initialisation de son vecteur de probabilites conditionnelles a partir
				// de la partie dont elle est issue
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
	ensure(Check());
}

void SNBRegressorSelectionDataCostCalculator::UpdateTargetPartitionWithRemovedAttribute(
    SNBDataTableBinarySliceSetAttribute* attribute)
{
	IntVector ivTargetPartFrequencies;
	int nTarget;
	int nTargetFrequency;
	int nTargetCumulativeFrequency;
	POSITION position;
	POSITION targetPosition;
	SNBIntervalTargetPart* targetPart;
	int nDeletedTargetPartFrequency;

	require(Check());

	// Calcul des effectifs par partie de l'attribut cible
	GetDataTableBinarySliceSet()->ExportTargetPartFrequencies(attribute, &ivTargetPartFrequencies);

	// Parcours synchronise de la partition cible courante et de la partition cible de l'attribut
	// pour determiner les parties a ajouter
	nTargetCumulativeFrequency = 0;
	nTarget = 0;
	targetPosition = NULL;
	targetPart = NULL;
	position = olTargetPartition.GetHeadPosition();
	while (nTarget < ivTargetPartFrequencies.GetSize())
	{
		// Initialisation
		if (nTargetCumulativeFrequency == 0)
		{
			// Acces a la premiere partie cible multivariee
			targetPosition = position;
			targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPosition) == targetPart);

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
			targetPosition = position;
			targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPosition) == targetPart);
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
				olTargetPartition.RemoveAt(targetPosition);
				ReleasePart(targetPart);
			}

			// Acces a la partie cible multivariee suivante
			targetPosition = position;
			targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPosition) == targetPart);

			// Passage a la partie de l'attribut suivante
			nTargetFrequency = ivTargetPartFrequencies.GetAt(nTarget);
			nTargetCumulativeFrequency += nTargetFrequency;
			nTarget++;

			// Mise a jour de la partie suivant la partie detruite
			targetPart->SetFrequency(targetPart->GetFrequency() + nDeletedTargetPartFrequency);
		}
	}
	ensure(Check());
}

boolean SNBRegressorSelectionDataCostCalculator::UpdateTargetPartScoresWithWeightedAttribute(
    SNBDataTableBinarySliceSetAttribute* attribute, Continuous cWeight)
{
	boolean bOk = true;
	IntVector ivTargetPartFrequencies;
	int nTargetValueIndex;
	int nTargetFrequency;
	int nTargetCumulativeFrequency;
	POSITION position;
	SNBIntervalTargetPart* targetPart;
	ContinuousVector* cvScores;

	require(Check());

	// Calcul des effectifs par partie de l'attribut cible
	GetDataTableBinarySliceSet()->ExportTargetPartFrequencies(attribute, &ivTargetPartFrequencies);

	// Parcours synchronise de la partition cible courante et de la partition cible de l'attribut
	// pour determiner mettre a jour les probabilites conditionnelles
	nTargetFrequency = 0;
	nTargetCumulativeFrequency = 0;
	nTargetValueIndex = 0;
	targetPart = NULL;
	position = olTargetPartition.GetHeadPosition();
	while (position != NULL)
	{
		targetPart = cast(SNBIntervalTargetPart*, olTargetPartition.GetNext(position));

		// Passage a la partie de l'attribut suivante si necessaire
		if (targetPart->GetCumulativeFrequency() > nTargetCumulativeFrequency)
		{
			nTargetFrequency = ivTargetPartFrequencies.GetAt(nTargetValueIndex);
			nTargetCumulativeFrequency += nTargetFrequency;
			nTargetValueIndex++;
		}

		// On verifie que la partie multivariee est inclue dans la partie de l'attribut
		assert(targetPart->GetCumulativeFrequency() <= nTargetCumulativeFrequency);
		assert(nTargetCumulativeFrequency - nTargetFrequency <=
		       targetPart->GetCumulativeFrequency() - targetPart->GetFrequency());

		// Calcul du nouveau score dans toutes les instances
		cvScores = targetPart->GetScores();
		bOk = bOk and GetDataTableBinarySliceSet()->UpdateTargetValueScores(attribute, nTargetValueIndex - 1,
										    cWeight, cvScores);
		if (not bOk)
			break;
	}
	assert(nTargetCumulativeFrequency == GetInstanceNumber());
	assert(nTargetValueIndex == ivTargetPartFrequencies.GetSize());
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
	bOk = bOk and oaTargetPartition.GetSize() >= 1;
	bOk = bOk and ivTargetPartIndexes.GetSize() == GetInstanceNumber();

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
					cout << "\n";
					bOk = false;
				}
				if (targetPart->GetScores()->GetSize() !=
				    GetDataTableBinarySliceSet()->GetActiveInstanceNumber())
				{
					AddError(sTmp + "Size of score vector (" +
						 IntToString(targetPart->GetScores()->GetSize()) + ") " +
						 "is inconsistent with the number active instances of the binary slice "
						 "set (" +
						 IntToString(GetDataTableBinarySliceSet()->GetActiveInstanceNumber()) +
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
	//   Max overhead des conteneurs de KWPSNewTargetIntervalScore (oaTargetPartition, olDeletedPartsCache &
	//   olTargetPartition) + Vecteur d'removedAttributeIndex des parties cible pour chaque individu (
	lEffectiveDistinctTargetValueNumber = (longint)min((double)nDistinctTargetValueNumber, sqrt(nInstanceNumber));
	return sizeof(SNBRegressorSelectionDataCostCalculator) +
	       lEffectiveDistinctTargetValueNumber * SNBIntervalTargetPart::ComputeNecessaryMemory(nInstanceNumber) +
	       lEffectiveDistinctTargetValueNumber *
		   (2 * olDummy.GetUsedMemoryPerElement() + oaDummy.GetUsedMemoryPerElement()) +
	       longint(nInstanceNumber) * sizeof(int);
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
// Classe SNBGeneralizedClassifierSelectionDataCostCalculator

SNBGeneralizedClassifierSelectionDataCostCalculator::SNBGeneralizedClassifierSelectionDataCostCalculator()
{
	signatureSchema = new SNBGroupTargetPartSignatureSchema;
}

SNBGeneralizedClassifierSelectionDataCostCalculator::~SNBGeneralizedClassifierSelectionDataCostCalculator()
{
	delete signatureSchema;
	Delete();
}

boolean SNBGeneralizedClassifierSelectionDataCostCalculator::Create()
{
	boolean bOk;
	SNBGroupTargetPart* targetPart;
	int nTarget;

	require(GetLearningSpec() != NULL);
	require(GetLearningSpec()->GetTargetAttributeType() == KWType::Symbol);
	require(oaGroupTargetPartsByTargetValueIndex.GetSize() == 0);

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
		bOk = oaTargetPartition.SetLargeSize(GetTargetValueNumber());

	// Initialisation de la taille du tableau des parties cibles indexe par les valeurs
	if (bOk)
		bOk = oaGroupTargetPartsByTargetValueIndex.SetLargeSize(GetTargetValueNumber());

	// Initialisation de ce tableau avec l'unique partie initiale
	if (bOk)
	{
		// Initialisation du tableau de parties
		oaTargetPartition.SetSize(1);
		oaTargetPartition.SetAt(0, targetPart);

		// Association entre les valeurs et la partie initiale
		for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
			oaGroupTargetPartsByTargetValueIndex.SetAt(nTarget, targetPart);
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
					// Memorisation du groupe par defaut si l'on rencontre la valeur speciale
					// StarValue
					if (nDefaultGroup == -1 and targetValueGroupsPartition->GetValueAt(
									nTargetValueIndex) == Symbol::GetStarValue())
					{
						nDefaultGroup = nGroup;

						// Pour la valeur speciale, il n'y a pas de correspondance avec une
						// valeur cible particuliere, et il faut court-circuiter la fin de la
						// boucle
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

void SNBGeneralizedClassifierSelectionDataCostCalculator::Initialize()
{
	int nTargetValueNumber;
	SNBGroupTargetPart* groupTargetPart;
	int nTargetPart;
	int nTarget;

	require(GetTargetAttributeType() == KWType::Symbol);
	require(Check());

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

	// Initialisation de le schema des signatures
	signatureSchema->Initialize();

	// Destruction de toutes les parties, sauf la premiere
	for (nTargetPart = 1; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
	{
		groupTargetPart = cast(SNBGroupTargetPart*, oaTargetPartition.GetAt(nTargetPart));
		ReleasePart(groupTargetPart);
	}
	oaTargetPartition.SetSize(1);

	// Reinitialisation de l'unique partie en cours
	groupTargetPart = cast(SNBGroupTargetPart*, oaTargetPartition.GetAt(0));
	groupTargetPart->SetFrequency(GetInstanceNumber());
	groupTargetPart->GetSignature()->SetSize(0);

	// Initialisation des probabilites conditionnelles a 1 (Ln(prob) a 0)
	assert(groupTargetPart->GetScores()->GetSize() == GetDataTableBinarySliceSet()->GetActiveInstanceNumber());
	groupTargetPart->GetScores()->Initialize();

	// Reinitialisation de l'association valeur-partie avec l'unique partie
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		oaGroupTargetPartsByTargetValueIndex.SetAt(nTarget, groupTargetPart);

	ensure(Check());
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::Delete()
{
	SNBGroupTargetPart* targetPart;
	int nTargetPart;

	// Liberation de toutes les parties
	for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBGroupTargetPart*, oaTargetPartition.GetAt(nTargetPart));
		ReleasePart(targetPart);
	}

	// Destruction de toutes les parties (elles sont toutes deja liberes)
	olDeletedPartsCache.DeleteAll();

	// Reinitialisation des tableaux
	oaGroupTargetPartsByTargetValueIndex.SetSize(0);
	oaTargetPartition.SetSize(0);

	// Nettoyage des services d'indexation de groupes cible pour les attributs
	CleanTargetValueGroupMatchings();
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::CleanTargetValueGroupMatchings()
{
	nkdTargetValueGroupMatchingsByAttribute.DeleteAll();
}

double SNBGeneralizedClassifierSelectionDataCostCalculator::ComputeSelectionDataCost()
{
	const boolean bLocalDisplay = false;
	NumericKeyDictionary nkdParts;
	double dDataCost;
	Continuous cInstanceActualScore;
	Continuous cDeltaScore;
	double dInstanceInverseProb;
	int nInstance;
	int nActualTarget;
	int nTargetPart;
	SNBGroupTargetPart* targetPart;
	SNBGroupTargetPart* actualTargetPart;
	int nActiveInstanceNumber;
	double dLaplaceNumerator;

	require(Check());

	///////////////////////////////////////////////////////////////////////////////////////////
	// L'ecriture de cette methode, tres souvent utilisee, a ete particulierement optimisee
	//  . memorisation dans des variables locales de donnees de travail
	//  . precalcul des parties constantes des couts
	//  . precalcul des borne sup des boucle (pour eviter leur reevaluation)

	// Entete de la trace
	if (bLocalDisplay)
	{
		cout << "\nIndex\tTarget\tProb\tValue\tEvaluation";
		for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
			cout << "\tScore" << nTargetPart + 1;
		cout << "\n";
	}

	// Calcul du nombre de predictions correctes
	dDataCost = 0;
	nActiveInstanceNumber = GetDataTableBinarySliceSet()->GetActiveInstanceNumber();
	for (nInstance = 0; nInstance < nActiveInstanceNumber; nInstance++)
	{
		nActualTarget = GetDataTableBinarySliceSet()->GetTargetValueIndexAtActiveInstance(nInstance);
		assert(0 <= nActualTarget and nActualTarget < GetTargetValueNumber());

		// Recherche du vecteur de probabilite pour la classe cible reelle
		actualTargetPart = cast(SNBGroupTargetPart*, oaGroupTargetPartsByTargetValueIndex.GetAt(nActualTarget));
		cInstanceActualScore = actualTargetPart->GetScores()->GetAt(nInstance);

		// On utilise les formules suivantes pour les calculs de probabilites conditionnelles par partie cible
		//  score(j) = log(P(Y_j)) + score_X(j)
		//        score_X(j) = \sum_i{log(P(X_i|Y_j))}
		// P(Y_j|X)) = P(Y_j) / (sum_j' {exp(score_X(j')-score_X(j))})
		//  -log(P(Y_j|X)) = log(sum_j'{P(Y_j') exp(score(j')-score(j))) - log(pY_j)
		// Comme la probabilite est la meme dans chaque partie cible j, il suffit quand on normalise en
		// sommant les probabilites au denominateurs de multiplier la proba par l'effectif de la partie cible p,
		// en gardant l'effectif de la valeur cible au numerateur
		//  P(Y_j|X) = N_j / sum_p{N_p*exp(score(p)-score(j))
		//  -log(P(Y_j|X)) = log(sum_p {N_p*exp(score(p)-score(j))) - log(N_j)

		// score_i = log P(Y_j) + Sum_k w_k * log P(X_ik | Y_j)
		dInstanceInverseProb = 0.0;
		for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
		{
			targetPart = cast(SNBGroupTargetPart*, oaTargetPartition.GetAt(nTargetPart));

			// Cas particulier pour eviter les calculs
			if (targetPart == actualTargetPart)
				dInstanceInverseProb += targetPart->GetFrequency();
			// Cas general, impliquant un calcul d'exponentiel
			else
			{
				// Difference de score pour le groupe cible en cours
				cDeltaScore = targetPart->GetScores()->GetAt(nInstance) - cInstanceActualScore;

				// Prise en compte de l'exponentielle, en tenant compte du seuil de validite
				dInstanceInverseProb += targetPart->GetFrequency() *
							(cDeltaScore >= cMaxScore ? dMaxExpScore : exp(cDeltaScore));
			}
		}
		assert(dInstanceInverseProb >= 1);

		// Mise a jour de l'inverse de la probabilite, avec l'effectif de la valeur cible
		dInstanceInverseProb /= GetTargetValueFrequencyAt(nActualTarget);

		// Mise a jour de cette probabilite en tenant compte de l'estimateur de Laplace
		dLaplaceNumerator = dInstanceNumber / dInstanceInverseProb + dLaplaceEpsilon;
		assert(0 < dLaplaceNumerator and dLaplaceNumerator < dLaplaceDenominator);

		// Mise a jour de l'evaluation globale
		dDataCost -= log(dLaplaceNumerator);

		// Ligne de trace
		if (bLocalDisplay)
		{
			cout << nInstance << "\t" << nActualTarget << "\t" << dLaplaceNumerator / dLaplaceDenominator
			     << "\t" << -log(dLaplaceNumerator / dLaplaceDenominator) << "\t" << dDataCost;
			for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
			{
				targetPart = cast(SNBGroupTargetPart*, oaTargetPartition.GetAt(nTargetPart));
				cout << "\t" << targetPart->GetScores()->GetAt(nInstance);
			}
			cout << "\n";
		}
	}

	// Mise a jour de l'evaluation globale (partie denominateurs)
	dDataCost += GetDataTableBinarySliceSet()->GetActiveInstanceNumber() * log(dLaplaceDenominator);
	return dDataCost;
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::UpdateTargetPartitionWithAddedAttribute(
    SNBDataTableBinarySliceSetAttribute* attribute)
{
	int nTargetValue;
	NumericKeyDictionary nkdInitialParts;
	NumericKeyDictionary* nkdSubparts;
	int nPartIndex;
	ObjectArray oaGroupTargetPartIndexes;
	KWSortableIndex* targetPartIndex;
	SNBGroupTargetPart* groupTargetPart;
	SNBGroupTargetPart* newGroupTargetPart;

	require(not signatureSchema->Contains(attribute));
	require(Check());

	// Creation de cles d'acces aux parties cibles, qui serviront de cle d'acces
	// dans des dictionnaire a cle numerique
	oaGroupTargetPartIndexes.SetSize(attribute->GetTargetPartition()->GetPartNumber());
	for (nPartIndex = 0; nPartIndex < oaGroupTargetPartIndexes.GetSize(); nPartIndex++)
	{
		targetPartIndex = new KWSortableIndex;
		targetPartIndex->SetIndex(nPartIndex);
		oaGroupTargetPartIndexes.SetAt(nPartIndex, targetPartIndex);
	}

	// Parcours des parties initiales referencees pour chaque valeur
	// Chaque partie initiale sera potentiellement splittee en sous-partie en fonction de la
	// partition issue du nouvel attribut
	for (nTargetValue = 0; nTargetValue < oaGroupTargetPartsByTargetValueIndex.GetSize(); nTargetValue++)
	{
		groupTargetPart = cast(SNBGroupTargetPart*, oaGroupTargetPartsByTargetValueIndex.GetAt(nTargetValue));

		// Recherche de la clee numerique correspondant a la partie provenant du nouvel attribut
		// nPartIndex = ivTargetValueMatching->GetAt(nTargetValue);
		nPartIndex = GetTargetValueGroupIndexAt(attribute, nTargetValue);
		targetPartIndex = cast(KWSortableIndex*, oaGroupTargetPartIndexes.GetAt(nPartIndex));
		assert(targetPartIndex->GetIndex() == nPartIndex);

		// Recherche des sous-parties liee a la partie initiale
		nkdSubparts = cast(NumericKeyDictionary*, nkdInitialParts.Lookup(groupTargetPart));

		// Creation de ce dictionnaire de sous-parties si necessaire, et dans ce cas
		// memorisation de l'ancienne partie comme sous-partie associee au groupe en cours
		newGroupTargetPart = NULL;
		if (nkdSubparts == NULL)
		{
			nkdSubparts = new NumericKeyDictionary;
			nkdInitialParts.SetAt((NUMERIC)groupTargetPart, nkdSubparts);

			// Memorisation de la premiere sous partie et de son effectif
			newGroupTargetPart = groupTargetPart;
			newGroupTargetPart->SetFrequency(GetTargetValueFrequencyAt(nTargetValue));
			nkdSubparts->SetAt((NUMERIC)targetPartIndex, newGroupTargetPart);

			// Mise a jour d'une signature par ajout d'un removedAttributeIndex de groupe
			UpdateSignatureWithAddedAttribute(attribute, nTargetValue, newGroupTargetPart);
		}
		// Sinon, on recherche la sous-partie
		else
		{
			// Recherche de la sous-partie
			newGroupTargetPart = cast(SNBGroupTargetPart*, nkdSubparts->Lookup((NUMERIC)targetPartIndex));

			// Creation si necessaire
			if (newGroupTargetPart == NULL)
			{
				newGroupTargetPart = cast(SNBGroupTargetPart*, GetOrCreatePart());

				// Gestion rudimentaire du manque de memoire
				if (newGroupTargetPart == NULL)
					AddFatalError("Not enough working memory to optimize variable selection");

				// Initialisation de son vecteur de scores a partir de la partie dont elle est issue
				newGroupTargetPart->GetScores()->CopyFrom(groupTargetPart->GetScores());

				// Memorisation de la sous partie et de son effectif
				newGroupTargetPart->SetFrequency(GetTargetValueFrequencyAt(nTargetValue));
				nkdSubparts->SetAt((NUMERIC)targetPartIndex, newGroupTargetPart);

				// Ajout de la nouvelle partie dans la partition cible
				oaTargetPartition.Add(newGroupTargetPart);

				// Mise a jour d'une signature par ajout d'un removedAttributeIndex de groupe
				// On commence par recopier l'ancienne signature, en supprimant sa derniere composant
				assert(groupTargetPart->GetSignature()->GetSize() == signatureSchema->GetSize() + 1);
				newGroupTargetPart->GetSignature()->CopyFrom(groupTargetPart->GetSignature());
				newGroupTargetPart->GetSignature()->SetSize(
				    newGroupTargetPart->GetSignature()->GetSize() - 1);
				UpdateSignatureWithAddedAttribute(attribute, nTargetValue, newGroupTargetPart);
			}
			// Mise a jour de l'effectif de la sous-partie
			else
				newGroupTargetPart->SetFrequency(newGroupTargetPart->GetFrequency() +
								 GetTargetValueFrequencyAt(nTargetValue));
		}
		assert(newGroupTargetPart != NULL);

		// Memorisation de la nouvelle partie cible associe a la valeur cible
		oaGroupTargetPartsByTargetValueIndex.SetAt(nTargetValue, newGroupTargetPart);
	}

	// Mise a jour des specifications de signature
	signatureSchema->AddAttribute(attribute);

	// Nettoyage
	nkdInitialParts.DeleteAll();
	oaGroupTargetPartIndexes.DeleteAll();

	ensure(signatureSchema->Contains(attribute));
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
    SNBDataTableBinarySliceSetAttribute* attribute)
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
	require(signatureSchema->Contains(attribute));

	// Acces a l'removedAttributeIndex de l'attribut a supprimer dans la signature cible
	nRemovedAttributeSignatureIndex = signatureSchema->GetSignatureIndexAt(attribute);

	// Retaillage initiale de la partition cible
	oaTargetPartition.SetSize(0);

	// Parcours des parties initiales referencees pour chaque valeur
	// Chaque partie initiale sera potentiellement simplifiee en fonction de la partition issue
	// du nouvel attribut, ce qui entrainera la supression des parties redondantes
	for (nTargetValue = 0; nTargetValue < oaGroupTargetPartsByTargetValueIndex.GetSize(); nTargetValue++)
	{
		groupTargetPart = cast(SNBGroupTargetPart*, oaGroupTargetPartsByTargetValueIndex.GetAt(nTargetValue));

		// Traitement de la partie, si non deja traitee (signature diminuee)
		if (groupTargetPart->GetSignature()->GetSize() == signatureSchema->GetSize())
		{
			// Mise a jour de la signature de la partie par supression d'un removedAttributeIndex de groupe
			UpgradeTargetSignatureWithRemovedAttribute(attribute, nTargetValue, groupTargetPart);
			assert(groupTargetPart->GetSignature()->GetSize() == signatureSchema->GetSize() - 1);

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
				oaTargetPartition.Add(newTargetPartScore);
			}
			// Consolidation de la partie restante sinon
			else
			{
				assert(newTargetPartScore != groupTargetPart);

				// Prise en compte de l'effectif de la partie a fusionner
				newTargetPartScore->SetFrequency(newTargetPartScore->GetFrequency() +
								 groupTargetPart->GetFrequency());

				// Memorisation de la nouvelle association
				oaGroupTargetPartsByTargetValueIndex.SetAt(nTargetValue, newTargetPartScore);

				// Enregistrement de la partie a detruire
				// (elle est encore potentiellement referencee pour d'autre valeurs cibles)
				oaPartsToDelete.Add(groupTargetPart);
			}
		}
		// Traitement de la partie, si deja traitee
		else
		{
			assert(groupTargetPart->GetSignature()->GetSize() == signatureSchema->GetSize() - 1);

			// Recherche de la partie parmi les parties restantes
			position = slRemainingParts.Find(groupTargetPart);
			assert(position != NULL);
			newTargetPartScore = cast(SNBGroupTargetPart*, slRemainingParts.GetAt(position));

			// Memorisation de la nouvelle association
			oaGroupTargetPartsByTargetValueIndex.SetAt(nTargetValue, newTargetPartScore);
		}
	}
	assert(slRemainingParts.GetCount() == oaTargetPartition.GetSize());

	// Destruction effective des parties
	for (nTargetPart = 0; nTargetPart < oaPartsToDelete.GetSize(); nTargetPart++)
	{
		groupTargetPart = cast(SNBGroupTargetPart*, oaPartsToDelete.GetAt(nTargetPart));
		ReleasePart(groupTargetPart);
	}

	// Mise a jour des specification de signature
	signatureSchema->RemoveAttribute(attribute);

	ensure(Check());
}

boolean SNBGeneralizedClassifierSelectionDataCostCalculator::UpdateTargetPartScoresWithWeightedAttribute(
    SNBDataTableBinarySliceSetAttribute* attribute, Continuous cWeight)
{
	boolean bOk = true;
	int nSignatureAttributeIndex;
	int nGroupTargetPart;
	SNBGroupTargetPart* groupTargetPart;
	int nAttributeTargetPart;

	require(Check());
	require(signatureSchema->Contains(attribute));

	// Index de l'attribut dans les specifications de la jointure
	nSignatureAttributeIndex = signatureSchema->GetSignatureIndexAt(attribute);

	// Collecte des parties par parcours des parties references pour chaque valeur
	for (nGroupTargetPart = 0; nGroupTargetPart < oaTargetPartition.GetSize(); nGroupTargetPart++)
	{
		groupTargetPart = cast(SNBGroupTargetPart*, oaTargetPartition.GetAt(nGroupTargetPart));

		// Calcul du nouveau score dans toutes les instances
		nAttributeTargetPart = groupTargetPart->GetSignature()->GetAt(nSignatureAttributeIndex);
		bOk = bOk and GetDataTableBinarySliceSet()->UpdateTargetValueScores(
				  attribute, nAttributeTargetPart, cWeight, groupTargetPart->GetScores());
		if (not bOk)
			break;
	}
	ensure(Check());
	return bOk;
}

void SNBGeneralizedClassifierSelectionDataCostCalculator::UpgradeTargetSignatureWithRemovedAttribute(
    const SNBDataTableBinarySliceSetAttribute* attribute, int nTargetValue, SNBGroupTargetPart* groupTargetPart) const
{
	int nAttributeSignature;

	require(0 <= nTargetValue and nTargetValue < GetTargetValueNumber());
	require(groupTargetPart->Check());
	require(CheckTargetSignature(groupTargetPart, nTargetValue));

	// La fin de signature remplace la place de l'attribut supprime
	nAttributeSignature = signatureSchema->GetSignatureIndexAt(attribute);
	groupTargetPart->GetSignature()->SetAt(
	    nAttributeSignature, groupTargetPart->GetGroupIndexAt(groupTargetPart->GetSignatureSize() - 1));
	groupTargetPart->GetSignature()->SetSize(groupTargetPart->GetSignatureSize() - 1);
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
	for (nTargetValue = 0; nTargetValue < oaGroupTargetPartsByTargetValueIndex.GetSize(); nTargetValue++)
	{
		groupTargetPart = cast(SNBGroupTargetPart*, oaGroupTargetPartsByTargetValueIndex.GetAt(nTargetValue));

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
		bOk = bOk and oaTargetPartition.NoNulls();
		if (oaTargetPartition.GetSize() != nkdGroupTargetPartSet.GetCount())
		{
			AddError(sTmp + "Number of different target parts (" +
				 IntToString(nkdGroupTargetPartSet.GetCount()) +
				 ") should be equal to the size of the target partition (" +
				 IntToString(oaTargetPartition.GetSize()) + ")");
			bOk = false;
		}

		// Si la base est vide, il ne doit y avoir qu'une partie cible
		if (GetInstanceNumber() == 0)
		{
			if (oaTargetPartition.GetSize() != 1)
			{
				AddError("Target partition structure should contain one single part");
				bOk = false;
			}
		}
		// Verification de la structure sinon
		else
		{
			// Verification du nombre de parties
			if (oaTargetPartition.GetSize() > GetTargetDescriptiveStats()->GetValueNumber())
			{
				AddError(sTmp + "Number of target parts (" + IntToString(oaTargetPartition.GetSize()) +
					 ") should not be greater than the number of class values (" +
					 IntToString(GetTargetDescriptiveStats()->GetValueNumber()) + ")");
				bOk = false;
			}

			// Verification des parties
			nTotalFrequency = 0;
			for (nGroupTargetPart = 0; nGroupTargetPart < oaTargetPartition.GetSize(); nGroupTargetPart++)
			{
				groupTargetPart = cast(SNBGroupTargetPart*, oaTargetPartition.GetAt(nGroupTargetPart));
				bOk = bOk and groupTargetPart != NULL;
				bOk = bOk and nkdGroupTargetPartSet.Lookup((NUMERIC)groupTargetPart) != NULL;

				// Mise a jour de l'effectif total
				bOk = bOk and 0 < groupTargetPart->GetFrequency() and
				      groupTargetPart->GetFrequency() <= GetInstanceNumber();
				nTotalFrequency += groupTargetPart->GetFrequency();

				// Verification de la partie
				if (groupTargetPart->GetScores()->GetSize() !=
				    GetDataTableBinarySliceSet()->GetActiveInstanceNumber())
				{
					AddError(sTmp + "Size of score vector (" +
						 IntToString(groupTargetPart->GetScores()->GetSize()) + ") " +
						 "is inconsistent with the number active instances of the binary slice "
						 "set (" +
						 IntToString(GetDataTableBinarySliceSet()->GetActiveInstanceNumber()) +
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

	// La taille de la signature de la partie doit etre de la meme taille que l'ensemble d'attributs qui
	// l'specifient
	bOk = bOk and groupTargetPart->GetSignatureSize() == signatureSchema->GetSize();

	// Coherence des indices des groupes pour chaque attribut dans la signature
	if (bOk)
	{
		for (nSignature = 0; nSignature < signatureSchema->GetSize(); nSignature++)
		{
			signatureAttribute = signatureSchema->GetAttributeAt(nSignature);
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
	for (nAttribute = 0; nAttribute < signatureSchema->GetSize(); nAttribute++)
	{
		attribute = signatureSchema->GetAttributeAt(nAttribute);

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
	for (nTargetPart = 0; nTargetPart < oaTargetPartition.GetSize(); nTargetPart++)
	{
		targetPart = cast(SNBTargetPart*, oaTargetPartition.GetAt(nTargetPart));
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
	//   Max overhead des conteneurs des SNBGroupTargetPart's (oaTargetPartition, olDeletedPartsCache,
	//   oaGroupTargetPartsByTargetValueIndex) Dictionnaire de groups matchings
	//   (nkdAttributeTargeValueGroupMatchings) Schema des signatures des parties cibles
	return sizeof(SNBGeneralizedClassifierSelectionDataCostCalculator) +
	       nTargetValueNumber * SNBGroupTargetPart::ComputeNecessaryMemory(nInstanceNumber, nAttributeNumber) +
	       nTargetValueNumber * (2 * oaDummy.GetUsedMemoryPerElement() + olDummy.GetUsedMemoryPerElement()) +
	       nAttributeNumber * (nkdDummy.GetUsedMemoryPerElement() + sizeof(IntVector) +
				   (longint)nTargetValueNumber * sizeof(int)) +
	       SNBGroupTargetPartSignatureSchema::ComputeNecessaryMemory(nAttributeNumber);
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

	attributeSignatureIndex = cast(KWSortableIndex*, nkdAttributeIndexes.Lookup((NUMERIC)attribute));
	if (attributeSignatureIndex == NULL)
		nSignature = -1;
	else
		nSignature = attributeSignatureIndex->GetIndex();

	return nSignature;
}

void SNBGroupTargetPartSignatureSchema::AddAttribute(SNBDataTableBinarySliceSetAttribute* attribute)
{
	KWSortableIndex* attributeSignatureIndex;

	require(not Contains(attribute));
	require(Check());

	// Ajout de l'attribut a la fin de la signature
	oaAttributes.Add(attribute);

	// Memorisation de son removedAttributeIndex dans la signature
	attributeSignatureIndex = new KWSortableIndex;
	attributeSignatureIndex->SetIndex(oaAttributes.GetSize() - 1);
	nkdAttributeIndexes.SetAt((NUMERIC)attribute, attributeSignatureIndex);

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
	removedAttributeIndex = cast(KWSortableIndex*, nkdAttributeIndexes.Lookup((NUMERIC)attribute));
	nRemovedAttribute = removedAttributeIndex->GetIndex();

	// Supression de l'indice de l'attribut dans le dictionnaire d'indexes
	nkdAttributeIndexes.RemoveKey((NUMERIC)attribute);
	delete removedAttributeIndex;

	// Deplacement du dernier attribut a la place de celui supprime (retaillage ensuite)
	if (oaAttributes.GetSize() >= 2 and nRemovedAttribute < oaAttributes.GetSize() - 1)
	{
		lastAttribute =
		    cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes.GetAt(oaAttributes.GetSize() - 1));
		oaAttributes.SetAt(nRemovedAttribute, lastAttribute);
		lastAttributeIndex = cast(KWSortableIndex*, nkdAttributeIndexes.Lookup((NUMERIC)lastAttribute));
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
			bOk = bOk and nkdAttributeIndexes.Lookup((NUMERIC)attribute) != NULL;
			attributeSignatureIndex =
			    cast(KWSortableIndex*, nkdAttributeIndexes.Lookup((NUMERIC)attribute));
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