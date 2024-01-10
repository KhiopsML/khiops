// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorSelectionScore.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorSelectionScore

KWPredictorSelectionScore::KWPredictorSelectionScore()
{
	dataPreparationBase = NULL;
	dSelectionModelAllAttributCost = 0;
	bIsWorkingDataCreated = false;
	bIsWorkingDataInitialized = false;
	bTPDisplay = false;

	// Par defaut, on prend en compte les prior dans le SNB
	// Le poids de prior a 0.25 releve d'une etude empirique
	dPriorWeight = 0.25;
	bIsConstructionCost = true;
	bIsPreparationCost = true;
}

KWPredictorSelectionScore::~KWPredictorSelectionScore()
{
	olDeletedParts.DeleteAll();
}

void KWPredictorSelectionScore::SetDataPreparationBase(KWDataPreparationBase* kwdpbValue)
{
	require(kwdpbValue == NULL or GetLearningSpec() == kwdpbValue->GetLearningSpec());
	require(GetLearningSpec()->IsTargetStatsComputed());

	// Nettoyage initial
	CleanWorkingData();

	// Memorisation
	dataPreparationBase = kwdpbValue;
}

KWDataPreparationBase* KWPredictorSelectionScore::GetDataPreparationBase()
{
	ensure(dataPreparationBase == NULL or GetLearningSpec() == dataPreparationBase->GetLearningSpec());
	return dataPreparationBase;
}

boolean KWPredictorSelectionScore::CreateWorkingData()
{
	require(Check());

	// Nettoyage prealable
	CleanWorkingData();

	// Creation de la partition
	bIsWorkingDataCreated = TPCreate();

	// Nettoyage si echec de la creation
	if (not bIsWorkingDataCreated)
	{
		TPDelete();
		AddError("Unable to create working data for as a preparation step for variable selection");
	}
	return bIsWorkingDataCreated;
}

boolean KWPredictorSelectionScore::IsWorkingDataCreated() const
{
	return bIsWorkingDataCreated;
}

void KWPredictorSelectionScore::InitializeWorkingData()
{
	require(Check());
	require(bIsWorkingDataCreated);
	require(dataPreparationBase->IsPreparedDataComputed());

	// Reinitialisation de la selection d'attributs
	dSelectionModelAllAttributCost = 0;
	nkdSelectedAttributes.RemoveAll();

	// Initialisation/reinitialisation de la partition cible
	TPInitialize();
	bIsWorkingDataInitialized = true;
	ensure(bIsWorkingDataInitialized);
}

boolean KWPredictorSelectionScore::IsWorkingDataInitialized() const
{
	return bIsWorkingDataInitialized;
}

void KWPredictorSelectionScore::CleanWorkingData()
{
	dSelectionModelAllAttributCost = 0;
	nkdSelectedAttributes.RemoveAll();
	TPDelete();
	bIsWorkingDataCreated = false;
	bIsWorkingDataInitialized = false;
}

void KWPredictorSelectionScore::AddAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	require(GetDataPreparationBase() != NULL);
	require(dataPreparationAttribute != NULL);
	require(not IsAttributeSelected(dataPreparationAttribute));

	// Mise a jour de la partition cible en incorporant le nouvel attribut
	assert(TPCheck());
	TPAddAttribute(dataPreparationAttribute);
	assert(TPCheck());

	// Ajout d'un attribut dans la selection
	nkdSelectedAttributes.SetAt((NUMERIC)dataPreparationAttribute, dataPreparationAttribute);

	// Memorisation de l'ajout de son cout unitaire
	dSelectionModelAllAttributCost += ComputeSelectionModelAttributeCost(dataPreparationAttribute);

	// Mise a jour des vecteurs de probabilites conditionnelles dans la partition cible
	// La mise a jour se fait apres l'ajout, quand la partition cible est alors incluse
	// dans la partition de l'attribut
	TPUpgradeAttributeSourceConditionalProbs(dataPreparationAttribute, 1.0);

	// Affichage
	if (TPGetDisplay())
	{
		cout << "Add\t" << dataPreparationAttribute->GetPreparedAttribute()->GetName() << endl;
		TPWrite(cout);
		cout << endl;
	}

	// Verification de la partition cible
	ensure(TPCheck());
}

void KWPredictorSelectionScore::RemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	require(GetDataPreparationBase() != NULL);
	require(dataPreparationAttribute != NULL);
	require(IsAttributeSelected(dataPreparationAttribute));

	// Mise a jour des vecteurs de probabilites conditionnelles dans la partition cible
	// La mise a jour se fait avant la supression, quand la partition cible est encore incluse
	// dans la partition de l'attribut
	TPUpgradeAttributeSourceConditionalProbs(dataPreparationAttribute, -1.0);

	// Memorisation de la supression de son cout unitaire
	dSelectionModelAllAttributCost -= ComputeSelectionModelAttributeCost(dataPreparationAttribute);

	// Suppression d'un attribut dans la selection
	nkdSelectedAttributes.RemoveKey((NUMERIC)dataPreparationAttribute);

	// Mise a jour de la partition cible en supprimant l'attribut
	assert(TPCheck());
	TPRemoveAttribute(dataPreparationAttribute);
	assert(TPCheck());

	// Affichage
	if (TPGetDisplay())
	{
		cout << "Remove\t" << dataPreparationAttribute->GetPreparedAttribute()->GetName() << endl;
		TPWrite(cout);
		cout << endl;
	}

	// Verification de la partition cible
	ensure(TPCheck());
}

void KWPredictorSelectionScore::AddWeightedAttribute(KWDataPreparationAttribute* dataPreparationAttribute,
						     Continuous cWeight)
{
	require(GetDataPreparationBase() != NULL);
	require(dataPreparationAttribute != NULL);
	require(not IsAttributeSelected(dataPreparationAttribute));
	require(0 < cWeight and cWeight <= 1);

	// Mise a jour de la partition cible en incorporant le nouvel attribut
	TPAddAttribute(dataPreparationAttribute);

	// Ajout d'un attribut dans la selection
	nkdSelectedAttributes.SetAt((NUMERIC)dataPreparationAttribute, dataPreparationAttribute);

	// Memorisation de l'ajout de son cout unitaire
	dSelectionModelAllAttributCost += ComputeSelectionModelAttributeCost(dataPreparationAttribute);

	// Mise a jour des vecteurs de probabilites conditionnelles dans la partition cible
	// La mise a jour se fait apres l'ajout, quand la partition cible est alors incluse
	// dans la partition de l'attribut
	TPUpgradeAttributeSourceConditionalProbs(dataPreparationAttribute, cWeight);

	// Affichage
	if (TPGetDisplay())
	{
		cout << "Add\t" << cWeight << "\t" << dataPreparationAttribute->GetPreparedAttribute()->GetName()
		     << endl;
		TPWrite(cout);
		cout << endl;
	}

	// Verification de la partition cible
	ensure(TPCheck());
}

double KWPredictorSelectionScore::ComputeSelectionTotalCost()
{
	return ComputeSelectionModelCost() + ComputeSelectionDataCost();
}

double KWPredictorSelectionScore::ComputeSelectionModelCost()
{
	return ComputeSelectionModelAttributeNumberCost(GetSelectedAttributeNumber()) + dSelectionModelAllAttributCost;
}

double KWPredictorSelectionScore::ComputeSelectionDataCost()
{
	return 0;
}

double KWPredictorSelectionScore::ComputeSelectionModelAttributeNumberCost(int nAttributeNumber)
{
	double dModelCost;

	require(nAttributeNumber >= 0);

	// Choix entre modele nul ou modele informatif
	dModelCost = GetNullConstructionCost();

	// Si modele null, prise en compte du cout de preparation du modele null
	if (nAttributeNumber == 0)
	{
		// Cout de preparation du modele null
		if (GetPreparationCost())
			dModelCost += GetNullPreparationCost();
	}

	// Cas d'un modele informatif
	if (nAttributeNumber > 0)
	{
		// Codage du nombre d'attributs choisis
		dModelCost += KWStat::NaturalNumbersUniversalCodeLength(nAttributeNumber);

		// Codage du choix du sous-ensemble des attributs
		dModelCost -= KWStat::LnFactorial(nAttributeNumber);
	}

	// Prise en compte du poids du prior
	dModelCost *= GetPriorWeight();

	return dModelCost;
}

double
KWPredictorSelectionScore::ComputeSelectionModelAttributeCost(KWDataPreparationAttribute* dataPreparationAttribute)
{
	double dCost;
	int nInitialAttributeNumber;

	require(GetDataPreparationBase() != NULL);
	require(dataPreparationAttribute != NULL);

	// Initialisation
	dCost = 0;

	// Cout de selection et de construction (sans cout de model null)
	if (GetConstructionCost())
	{
		if (dataPreparationAttribute->GetPreparedStats()->GetConstructionCost() > 0)
			dCost += (dataPreparationAttribute->GetPreparedStats()->GetConstructionCost() -
				  dataPreparationAttribute->GetPreparedStats()->GetNullConstructionCost());
	}
	// Si pas de cout de construction, on se rabat sur un cout de selection de variables
	else
	{
		nInitialAttributeNumber =
		    dataPreparationBase->GetDataPreparationClass()->GetDataPreparationAttributes()->GetSize();
		dCost += log(nInitialAttributeNumber * 1.0);
	}

	// Cout de preparation
	if (GetPreparationCost())
		dCost += dataPreparationAttribute->GetPreparedStats()->GetPreparationCost();

	// Prise en compte du poids du prior
	dCost *= GetPriorWeight();

	return dCost;
}

int KWPredictorSelectionScore::GetSelectedAttributeNumber() const
{
	return nkdSelectedAttributes.GetCount();
}

boolean KWPredictorSelectionScore::IsAttributeSelected(KWDataPreparationAttribute* dataPreparationAttribute) const
{
	require(dataPreparationBase != NULL);
	require(dataPreparationAttribute != NULL);
	ensure(nkdSelectedAttributes.Lookup((NUMERIC)dataPreparationAttribute) == NULL or
	       nkdSelectedAttributes.Lookup((NUMERIC)dataPreparationAttribute) == dataPreparationAttribute);
	return nkdSelectedAttributes.Lookup((NUMERIC)dataPreparationAttribute) != NULL;
}

NumericKeyDictionary* KWPredictorSelectionScore::GetSelectedAttributes()
{
	return &nkdSelectedAttributes;
}

boolean KWPredictorSelectionScore::Check() const
{
	boolean bOk = true;

	// Validation des specifications
	bOk = KWLearningService::Check();

	// Verification de la compatibilite avec la base d'apprentissage
	assert(dataPreparationBase != NULL or GetLearningSpec() == dataPreparationBase->GetLearningSpec());
	assert(GetLearningSpec()->IsTargetStatsComputed());

	return bOk;
}

void KWPredictorSelectionScore::Write(ostream& ost) const
{
	require(dataPreparationBase != NULL);

	ost << "Target partition\n";
	ost << "Instances\t" << dataPreparationBase->GetInstanceNumber() << "\n";
	TPWrite(ost);
}

const ALString KWPredictorSelectionScore::GetClassLabel() const
{
	return "Selection score";
}

const ALString KWPredictorSelectionScore::GetObjectLabel() const
{
	if (GetLearningSpec() == NULL)
		return "";
	else if (GetTargetAttributeType() == KWType::Symbol)
		return "Classifier";
	else if (GetTargetAttributeType() == KWType::Continuous)
		return "Regressor";
	else
		return "";
}

boolean KWPredictorSelectionScore::TPCreate()
{
	return false;
}

void KWPredictorSelectionScore::TPInitialize() {}

void KWPredictorSelectionScore::TPDelete()
{
	olDeletedParts.DeleteAll();
}

boolean KWPredictorSelectionScore::TPCheck() const
{
	return false;
}

void KWPredictorSelectionScore::TPWrite(ostream& ost) const {}

void KWPredictorSelectionScore::TPAddAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	assert(false);
}

void KWPredictorSelectionScore::TPRemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	assert(false);
}

void KWPredictorSelectionScore::TPUpgradeAttributeSourceConditionalProbs(
    KWDataPreparationAttribute* dataPreparationAttribute, Continuous cWeight)
{
	assert(false);
}

KWPSTargetPartScore* KWPredictorSelectionScore::TPNewPart()
{
	KWPSTargetPartScore* targetPart;

	require(dataPreparationBase != NULL);

	// On retourne un element precedemment detruit s'il en existe
	if (olDeletedParts.GetCount() > 0)
	{
		targetPart = cast(KWPSTargetPartScore*, olDeletedParts.RemoveHead());
	}
	// Creation sinon
	else
	{
		// Creation avec tentative d'allocation d'un vecteur de probabilites conditionnelles,
		// potentiellement de grande taille
		targetPart = TPCreatePart();
		targetPart->GetLnSourceConditionalProbs()->SetLargeSize(GetInstanceNumber());

		// Nettoyage si echec de l'allocation
		if (targetPart->GetLnSourceConditionalProbs()->GetSize() != GetInstanceNumber())
		{
			delete targetPart;
			targetPart = NULL;
		}
	}
	return targetPart;
}

void KWPredictorSelectionScore::TPDeletePart(KWPSTargetPartScore* targetPart)
{
	require(dataPreparationBase != NULL);
	require(targetPart != NULL);
	require(targetPart->GetLnSourceConditionalProbs()->GetSize() == GetInstanceNumber());

	olDeletedParts.AddHead(targetPart);
}

KWPSTargetPartScore* KWPredictorSelectionScore::TPCreatePart()
{
	return new KWPSTargetPartScore;
}

void KWPredictorSelectionScore::TPSetDisplay(boolean bValue)
{
	bTPDisplay = bValue;
}

boolean KWPredictorSelectionScore::TPGetDisplay() const
{
	return bTPDisplay;
}

void KWPredictorSelectionScore::ExportTargetPartFrequencies(KWDataPreparationAttribute* dataPreparationAttribute,
							    IntVector* ivTargetPartFrequencies) const
{
	KWDataGridStats* preparedDataGridStats;

	require(dataPreparationAttribute != NULL);
	require(dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats() != NULL);
	require(dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats()->GetTargetAttributeNumber() ==
		1);
	require(ivTargetPartFrequencies != NULL);

	// Calcul des effectif par partie cible
	preparedDataGridStats = dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats();
	preparedDataGridStats->ExportAttributePartFrequenciesAt(preparedDataGridStats->GetFirstTargetAttributeIndex(),
								ivTargetPartFrequencies);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Classe KWPSTargetPartScore

KWPSTargetPartScore::KWPSTargetPartScore() {}

KWPSTargetPartScore::~KWPSTargetPartScore() {}

void KWPSTargetPartScore::WriteHeaderLineReport(ostream& ost) {}

void KWPSTargetPartScore::WriteLineReport(ostream& ost) {}

void KWPSTargetPartScore::Write(ostream& ost)
{
	WriteLineReport(ost);
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWClassifierSelectionScore

KWClassifierSelectionScore::KWClassifierSelectionScore() {}

KWClassifierSelectionScore::~KWClassifierSelectionScore()
{
	oaTargetPartition.DeleteAll();
}

double KWClassifierSelectionScore::ComputeSelectionDataCost()
{
	boolean bDisplay = false;
	double dDataCost;
	Continuous cInstanceActualScore;
	Continuous cDeltaScore;
	Continuous cMaxScore;
	double dMaxExpScore;
	double dInstanceInverseProb;
	int nInstance;
	int nActualTarget;
	int nTarget;
	KWPSTargetPartScore* targetPart;
	KWPSTargetPartScore* actualTargetPart;
	int nTargetNumber;
	double dLaplaceEpsilon;
	double dInstanceNumber;
	double dLaplaceNumerator;
	double dLaplaceDenominator;

	require(Check());
	require(TPCheck());
	require(dataPreparationBase->IsPreparedDataComputed());

	///////////////////////////////////////////////////////////////////////////////////////////
	// L'ecriture de cette methode, tres souvent utilisee, a ete particulierement optimisee
	//  . memorisation dans des variables locales de donnees de travail
	//  . precalcul des parties constantes des couts
	//  . precalcul des borne sup des boucle (pour eviter leur reevaluation)

	// Entete de la trace
	if (bDisplay)
	{
		cout << "\nIndex\tTarget\tProb\tValue\tEvaluation";
		for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
			cout << "\tScore" << nTarget + 1;
		cout << "\n";
	}

	// Calcul d'un epsilon de Laplace pour la gestion des petites probabilites
	// En effet, cette normalisation est faite en univarie, mais le probleme peut
	// a nouveau se poser en multivarie du fait de la multiplication de probabilites
	// En univarie, l'epsilon est tres petit (e = 1/InstanceNumber), car on considere
	// que l'estimation MODL est de bonne qualite.
	// En multivarie, on se contente du "proche du classique" e=0.5/J,
	// ce qui permet d'avoir un denominateur en N+0.5 quelque soit le nombre J de classes
	nTargetNumber = GetTargetDescriptiveStats()->GetValueNumber();
	dInstanceNumber = GetInstanceNumber();
	dLaplaceEpsilon = 0.5 / nTargetNumber;

	// Prise en compte de l'epsilon pour une probabilite p
	//   p = p*N / N
	//   p_Laplace = (p*N + e)/(N + J*e)
	// Precalcul une fois pour toutes du denominateur
	dLaplaceDenominator = dInstanceNumber + dLaplaceEpsilon * nTargetNumber;

	// Calcul des seuils de score max pour eviter les exponentielles infinies (vis a vis de la precision machine)
	dMaxExpScore = DBL_MAX / dInstanceNumber;
	cMaxScore = (Continuous)log(dMaxExpScore);

	// Calcul du nombre de predictions correctes
	dDataCost = 0;
	for (nInstance = 0; nInstance < GetInstanceNumber(); nInstance++)
	{
		nActualTarget = dataPreparationBase->GetTargetIndexes()->GetAt(nInstance);
		assert(0 <= nActualTarget and nActualTarget < oaTargetPartition.GetSize());

		// Recherche du vecteur de probabilite pour la classe cible reelle
		actualTargetPart = cast(KWPSTargetPartScore*, oaTargetPartition.GetAt(nActualTarget));
		cInstanceActualScore = actualTargetPart->GetLnSourceConditionalProbs()->GetAt(nInstance);

		// On utilise les formules suivantes pour les calculs de probabilites conditionnelles
		//  score(j) = log(P(Y_j)) + \sum_i{log(P(X_i|Y_j))}
		//  P(Y_j|X) = 1 / sum_j'{exp(score(j')-score(j))
		//  -log(P(Y_j|X)) = log(sum_j'{exp(score(j')-score(j)))
		// Il suffit alors de parcourir les classes cibles
		dInstanceInverseProb = 0.0;
		for (nTarget = 0; nTarget < nTargetNumber; nTarget++)
		{
			// Cas particulier pour eviter les calculs
			if (nTarget == nActualTarget)
				dInstanceInverseProb += 1.0;
			// Cas general, impliquant un calcul d'exponentiel
			else
			{
				targetPart = cast(KWPSTargetPartScore*, oaTargetPartition.GetAt(nTarget));

				// Difference de score pour la classe j
				cDeltaScore =
				    targetPart->GetLnSourceConditionalProbs()->GetAt(nInstance) - cInstanceActualScore;

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
		if (bDisplay)
		{
			cout << nInstance << "\t" << nActualTarget << "\t" << dLaplaceNumerator / dLaplaceDenominator
			     << "\t" << -log(dLaplaceNumerator / dLaplaceDenominator) << "\t" << dDataCost;
			for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
			{
				targetPart = cast(KWPSTargetPartScore*, oaTargetPartition.GetAt(nTarget));
				cout << "\t" << targetPart->GetLnSourceConditionalProbs()->GetAt(nInstance);
			}
			cout << "\n";
		}
	}

	// Mise a jour de l'evaluation globale (partie denominateurs)
	dDataCost += dInstanceNumber * log(dLaplaceDenominator);
	return dDataCost;
}

boolean KWClassifierSelectionScore::TPCreate()
{
	boolean bOk;
	int nTarget;
	KWPSTargetPartScore* targetPart;

	require(GetTargetAttributeType() == KWType::Symbol);
	require(Check());
	require(oaTargetPartition.GetSize() == 0);

	// Creation d'autant de vecteurs de probabilites conditionnelles que de classes cibles
	bOk = true;
	bOk = oaTargetPartition.SetLargeSize(GetTargetDescriptiveStats()->GetValueNumber());
	if (bOk)
	{
		for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
		{
			targetPart = TPNewPart();

			// Memorisation si allocation reussie
			if (targetPart != NULL)
				oaTargetPartition.SetAt(nTarget, targetPart);
			// Echec sinon
			else
			{
				bOk = false;
				break;
			}
		}
	}
	ensure(not bOk or TPCheck());
	return bOk;
}

void KWClassifierSelectionScore::TPInitialize()
{
	double dTargetScore;
	Continuous cTargetScore;
	int nTarget;
	KWPSTargetPartScore* targetPart;
	ContinuousVector* cvScore;
	int nInstance;

	require(GetTargetAttributeType() == KWType::Symbol);
	require(Check());
	require(oaTargetPartition.GetSize() == GetTargetDescriptiveStats()->GetValueNumber());

	// Calcul des scores globaux en utilisant les probabilites a priori par modalites cible
	// et initialisation de ces scores pour toutes les instances
	for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
	{
		// Calcul du score de la modalite cible
		assert(GetTargetValueStats()->GetUnivariateCellFrequencyAt(nTarget) > 0);
		dTargetScore = GetTargetValueStats()->GetUnivariateCellFrequencyAt(nTarget) * 1.0 / GetInstanceNumber();
		cTargetScore = (Continuous)log(dTargetScore);

		// Memorisation de ce score pour toutes les instances
		targetPart = cast(KWPSTargetPartScore*, oaTargetPartition.GetAt(nTarget));
		cvScore = targetPart->GetLnSourceConditionalProbs();
		check(cvScore);
		for (nInstance = 0; nInstance < cvScore->GetSize(); nInstance++)
			cvScore->SetAt(nInstance, cTargetScore);
	}
}

void KWClassifierSelectionScore::TPDelete()
{
	oaTargetPartition.DeleteAll();
	KWPredictorSelectionScore::TPDelete();
}

boolean KWClassifierSelectionScore::TPCheck() const
{
	boolean bOk = true;
	int nTarget;
	KWPSTargetPartScore* targetPart;
	ALString sTmp;

	require(GetTargetAttributeType() == KWType::Symbol);
	require(Check());

	// Si la base est vide, il ne doit rien avoir
	if (GetInstanceNumber() == 0)
	{
		if (oaTargetPartition.GetSize() != 0)
		{
			AddError("Target partition structure should be empty");
			bOk = false;
		}
	}
	else
	// Verification de la structure sinon
	{
		// Verification du nombre de parties
		if (oaTargetPartition.GetSize() != GetTargetDescriptiveStats()->GetValueNumber())
		{
			AddError(sTmp + "Number of target parts (" + IntToString(oaTargetPartition.GetSize()) +
				 ") is inconsistent with the number of class values (" +
				 IntToString(GetTargetDescriptiveStats()->GetValueNumber()) + ")");
			bOk = false;
		}

		// Verification des parties
		for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
		{
			targetPart = cast(KWPSTargetPartScore*, oaTargetPartition.GetAt(nTarget));
			check(targetPart);

			// Verification de la partie
			if (targetPart->GetLnSourceConditionalProbs()->GetSize() != GetInstanceNumber())
			{
				AddError(sTmp + "Size of target part probability vector (" +
					 IntToString(targetPart->GetLnSourceConditionalProbs()->GetSize()) +
					 ") is inconsistent with the database size (" +
					 IntToString(GetInstanceNumber()) + ")");
				bOk = false;
			}
			if (not bOk)
				break;
		}
	}

	return bOk;
}

void KWClassifierSelectionScore::TPWrite(ostream& ost) const
{
	int nTarget;
	KWPSTargetPartScore* targetPart;

	require(TPCheck());

	for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
	{
		targetPart = cast(KWPSTargetPartScore*, oaTargetPartition.GetAt(nTarget));
		if (nTarget == 0)
		{
			targetPart->WriteHeaderLineReport(ost);
			ost << "\n";
		}
		targetPart->WriteLineReport(ost);
		ost << "\n";
	}
}

void KWClassifierSelectionScore::TPAddAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	// Pas d'impact sur la structure de la partition cible
	require(TPCheck());
	require(dataPreparationAttribute != NULL);
}

void KWClassifierSelectionScore::TPRemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	// Pas d'impact sur la structure de la partition cible
	require(TPCheck());
	require(dataPreparationAttribute != NULL);
}

void KWClassifierSelectionScore::TPUpgradeAttributeSourceConditionalProbs(
    KWDataPreparationAttribute* dataPreparationAttribute, Continuous cWeight)
{
	int nTarget;
	KWPSTargetPartScore* targetPart;
	ContinuousVector* cvScore;
	KWDataGridStats* dataGridStats;
	const KWDGSAttributePartition* targetPartition;

	require(TPCheck());
	require(dataPreparationAttribute != NULL);

	// Verification de la grille de preparation
	dataGridStats = dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats();
	check(dataGridStats != NULL);
	assert(dataGridStats->GetTargetAttributeNumber() == 1);
	targetPartition = dataGridStats->GetAttributeAt(dataGridStats->GetFirstTargetAttributeIndex());
	assert(targetPartition->GetAttributeType() == KWType::Symbol);
	assert(targetPartition->GetPartNumber() == GetTargetDescriptiveStats()->GetValueNumber());
	assert(targetPartition->ArePartsSingletons());

	// Calcul des probabilites conditionnelles par classe cible
	for (nTarget = 0; nTarget < GetTargetDescriptiveStats()->GetValueNumber(); nTarget++)
	{
		targetPart = cast(KWPSTargetPartScore*, oaTargetPartition.GetAt(nTarget));

		// Calcul du nouveau score dans toutes les instances
		cvScore = targetPart->GetLnSourceConditionalProbs();
		check(cvScore);
		dataPreparationBase->UpgradeTargetConditionalLnProbsAt(dataPreparationAttribute, nTarget, cWeight,
								       cvScore, cvScore);
	}
}

KWPSTargetPartScore* KWClassifierSelectionScore::TPCreatePart()
{
	return new KWPSTargetValueScore;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Classe KWPSTargetValueScore

KWPSTargetValueScore::KWPSTargetValueScore() {}

KWPSTargetValueScore::~KWPSTargetValueScore() {}

void KWPSTargetValueScore::WriteHeaderLineReport(ostream& ost) {}

void KWPSTargetValueScore::WriteLineReport(ostream& ost) {}

//////////////////////////////////////////////////////////////////////////////
// Classe KWRegressorSelectionScore

KWRegressorSelectionScore::KWRegressorSelectionScore() {}

KWRegressorSelectionScore::~KWRegressorSelectionScore()
{
	olTargetPartition.DeleteAll();
}

double KWRegressorSelectionScore::ComputeSelectionDataCost()
{
	boolean bDisplay = false;
	POSITION position;
	double dDataCost;
	Continuous cInstanceActualScore;
	int nTargetPart;
	int nActualTarget;
	int nTarget;
	KWPSTargetIntervalScore* targetPart;
	int nInstance;
	Continuous cMaxScore;
	double dMaxExpScore;
	double dInstanceInverseProb;
	Continuous cDeltaScore;
	double dLaplaceEpsilon;
	double dInstanceNumber;
	double dLaplaceNumerator;
	double dLaplaceDenominator;

	require(Check());
	require(TPCheck());
	require(dataPreparationBase->IsPreparedDataComputed());

	// Transformation de la liste en tableau pour en accelerer l'acces
	oaTargetPartition.SetSize(olTargetPartition.GetCount());
	position = olTargetPartition.GetHeadPosition();
	nTarget = 0;
	while (position != NULL)
	{
		oaTargetPartition.SetAt(nTarget, olTargetPartition.GetNext(position));
		nTarget++;
	}

	// Calcul des index de partie cible pour chaque index de valeur cible
	nInstance = 0;
	for (nTarget = 0; nTarget < oaTargetPartition.GetSize(); nTarget++)
	{
		targetPart = cast(KWPSTargetIntervalScore*, oaTargetPartition.GetAt(nTarget));

		// Tous les index de valeurs de la partie sont associee a cette partie
		while (nInstance < targetPart->GetCumulativeFrequency())
		{
			ivTargetPartIndexes.SetAt(nInstance, nTarget);
			nInstance++;
		}
	}

	// Entete de l'affichage
	if (bDisplay)
	{
		cout << "\nIndex\tTarget index\tInterval\tProb\tValue\tEvaluation";
		for (nTarget = 0; nTarget < oaTargetPartition.GetSize(); nTarget++)
			cout << "\tScore" << nTarget + 1;
		cout << "\n";
	}

	// Calcul d'un epsilon de Laplace pour la gestion des petites probabilites
	// En effet, cette normalisation est faite en univarie, mais le probleme peut
	// a nouveau se poser en multivarie du fait de la multiplication de probabilites
	// Comme les probabilites sont donnees par rang (par defaut, la probabilite d'un rang est 1/N),
	// ON adopte un epsilon similaire au cas de la classification, en considerant que l'on est
	// dans le cas ou le nombre de classes cibles J vaut N
	//    e = 0.5/N
	dInstanceNumber = GetInstanceNumber();
	dLaplaceEpsilon = 0.5 / (dInstanceNumber + 1.0); // Pour eviter le cas InstanceNumber==0

	// Prise en compte de l'epsilon pour une probabilite p
	//   p = p*N / N
	//   p_Laplace = (p*N + e)/(N + N*e)
	// Precalcul une fois pour toutes du denominateur
	dLaplaceDenominator = dInstanceNumber + dLaplaceEpsilon * dInstanceNumber;

	// Calcul des seuils de score max pour eviter les exponetielles infinies (vis a vis de la precision machine)
	dMaxExpScore = DBL_MAX / dInstanceNumber;
	cMaxScore = (Continuous)log(dMaxExpScore);

	// Parcours synchronise de la partition cible courante et des instances pour calculer
	// les log vraissemblances negatives par instance
	dDataCost = 0;
	for (nInstance = 0; nInstance < GetInstanceNumber(); nInstance++)
	{
		// Recherche de l'index de la partie cible associe a l'instance
		nActualTarget = dataPreparationBase->GetTargetIndexes()->GetAt(nInstance);
		nTargetPart = ivTargetPartIndexes.GetAt(nActualTarget);

		// Acces au numerateur pour l'instance
		targetPart = cast(KWPSTargetIntervalScore*, oaTargetPartition.GetAt(nTargetPart));
		cInstanceActualScore = targetPart->GetLnSourceConditionalProbs()->GetAt(nInstance);

		// On utilise les formules suivantes pour les calculs de probabilites conditionnelles par rang
		//  score(n) = \sum_i{log(P(X_i|Y_j))}
		// Comme la probabilite est la meme dans chaque intervalle j, il suffit quand on normalise en
		// sommant les probabilites au denominateurs de multiplier la proba par l'effectif de l'intervalle
		//  P(Y_n|X) = 1 / sum_j'{N_j'*exp(score(j')-score(j))
		//  -log(P(Y_n|X)) = log(sum_j'{N_j'*exp(score(j')-score(j)))
		// Il suffit alors de parcourir les intervalles cibles
		dInstanceInverseProb = 0.0;
		for (nTarget = 0; nTarget < oaTargetPartition.GetSize(); nTarget++)
		{
			// Cas particulier pour eviter les calculs
			if (nTarget == nActualTarget)
				dInstanceInverseProb += targetPart->GetFrequency();
			// Cas general, impliquant un calcul d'exponentiel
			else
			{
				targetPart = cast(KWPSTargetIntervalScore*, oaTargetPartition.GetAt(nTarget));

				// Difference de score pour l'intervalle cible en cours
				cDeltaScore =
				    targetPart->GetLnSourceConditionalProbs()->GetAt(nInstance) - cInstanceActualScore;

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
		if (bDisplay)
		{
			cout << nInstance << "\t" << nActualTarget << "\t" << dLaplaceNumerator / dLaplaceDenominator
			     << "\t" << -log(dLaplaceNumerator / dLaplaceDenominator) << "\t" << dDataCost;
			for (nTarget = 0; nTarget < oaTargetPartition.GetSize(); nTarget++)
			{
				targetPart = cast(KWPSTargetIntervalScore*, oaTargetPartition.GetAt(nTarget));
				cout << "\t" << targetPart->GetLnSourceConditionalProbs()->GetAt(nInstance);
			}
			cout << "\n";
		}
	}

	// Mise a jour de l'evaluation globale (partie denominateurs)
	dDataCost += dInstanceNumber * log(dLaplaceDenominator);
	return dDataCost;
}

boolean KWRegressorSelectionScore::TPCreate()
{
	boolean bOk;
	KWPSTargetIntervalScore* targetPart;

	require(GetTargetAttributeType() == KWType::Continuous);
	require(Check());
	require(olTargetPartition.GetCount() == 0);

	// Creation d'un vecteur de probabilites conditionnelles pour un unique intervalle cible initial
	bOk = true;
	targetPart = cast(KWPSTargetIntervalScore*, TPNewPart());
	if (targetPart != NULL)
	{
		targetPart->SetFrequency(GetInstanceNumber());
		targetPart->SetCumulativeFrequency(targetPart->GetFrequency());
		targetPart->SetRefCount(1);
		olTargetPartition.AddHead(targetPart);
	}
	else
		bOk = false;

	// Initialisation de la taille max du tableau de parties cibles
	if (bOk)
	{
		bOk = oaTargetPartition.SetLargeSize(GetInstanceNumber());
	}

	// Initialisation de la taille du vecteur d'index des partie cibles
	if (bOk)
	{
		bOk = ivTargetPartIndexes.SetLargeSize(GetInstanceNumber());
	}

	ensure(not bOk or TPCheck());
	return bOk;
}

void KWRegressorSelectionScore::TPInitialize()
{
	KWPSTargetIntervalScore* targetPart;

	require(GetTargetAttributeType() == KWType::Continuous);
	require(Check());
	require(olTargetPartition.GetCount() >= 1);
	require(oaTargetPartition.GetSize() >= 1);
	require(ivTargetPartIndexes.GetSize() == GetInstanceNumber());

	// Redimensionnenement eventuel de la partition jusqu'a obtenir une taille de 1
	while (olTargetPartition.GetCount() > 1)
	{
		// Supression de la derniere partie
		targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.RemoveTail());
		TPDeletePart(targetPart);
	}

	// Reinitialisation de l'unique partie en cours
	targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetHead());
	targetPart->SetFrequency(GetInstanceNumber());
	targetPart->SetCumulativeFrequency(targetPart->GetFrequency());
	targetPart->SetRefCount(1);

	// Initialisation des probabilites conditionnelles a 1 (Ln(prob) a 0)
	assert(targetPart->GetLnSourceConditionalProbs()->GetSize() == GetInstanceNumber());
	targetPart->GetLnSourceConditionalProbs()->Initialize();
}

void KWRegressorSelectionScore::TPDelete()
{
	olTargetPartition.DeleteAll();
	oaTargetPartition.SetSize(0);
	ivTargetPartIndexes.SetSize(0);
	KWPredictorSelectionScore::TPDelete();
}

boolean KWRegressorSelectionScore::TPCheck() const
{
	boolean bOk = true;
	POSITION position;
	KWPSTargetIntervalScore* targetPart;
	int nCumulativeFrequency;
	ALString sTmp;

	require(Check());
	require(olTargetPartition.GetCount() >= 1);
	require(oaTargetPartition.GetSize() >= 1);
	require(ivTargetPartIndexes.GetSize() == GetInstanceNumber());

	// Si la base est vide, il ne doit y avoir qu'une partie cible
	if (GetInstanceNumber() == 0)
	{
		if (olTargetPartition.GetCount() != 1)
		{
			AddError("Target partition structure should contain one single part");
			bOk = false;
		}
	}
	else
	// Verification de la structure sinon
	{
		nCumulativeFrequency = 0;
		position = olTargetPartition.GetHeadPosition();
		while (position != NULL)
		{
			targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetNext(position));

			// Verification de la partie
			nCumulativeFrequency += targetPart->GetFrequency();
			if (targetPart->GetFrequency() <= 0)
			{
				AddError(sTmp + "Target part frequency (" + IntToString(targetPart->GetFrequency()) +
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
			if (targetPart->GetLnSourceConditionalProbs()->GetSize() != GetInstanceNumber())
			{
				AddError(sTmp + "Size of target part probability vector (" +
					 IntToString(targetPart->GetLnSourceConditionalProbs()->GetSize()) +
					 ") is inconsistent with the database size (" +
					 IntToString(GetInstanceNumber()) + ")");
				bOk = false;
			}
			if (not bOk)
				break;
		}

		// Verification de l'effectif cumule total
		if (bOk and nCumulativeFrequency != GetInstanceNumber())
		{
			AddError(sTmp + "Last target part cumulated frequency (" + IntToString(nCumulativeFrequency) +
				 ") is inconsistent with the database size (" + IntToString(GetInstanceNumber()) + ")");
			bOk = false;
		}
	}

	return bOk;
}

void KWRegressorSelectionScore::TPWrite(ostream& ost) const
{
	POSITION position;
	KWPSTargetIntervalScore* targetPart;
	boolean bWriteHeader;

	require(TPCheck());

	bWriteHeader = true;
	position = olTargetPartition.GetHeadPosition();
	while (position != NULL)
	{
		targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetNext(position));
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

void KWRegressorSelectionScore::TPAddAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	IntVector ivTargetPartFrequencies;
	int nTarget;
	int nTargetFrequency;
	int nTargetCumulativeFrequency;
	POSITION position;
	POSITION targetPosition;
	KWPSTargetIntervalScore* targetPart;
	KWPSTargetIntervalScore* newTargetPart;

	require(TPCheck());
	require(dataPreparationAttribute != NULL);

	// Calcul des effectifs par partie de l'attribut cible
	ExportTargetPartFrequencies(dataPreparationAttribute, &ivTargetPartFrequencies);

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
			targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPosition) == targetPart);

			// Acces la premiere partie de l'attribut
			nTargetFrequency = ivTargetPartFrequencies.GetAt(nTarget);
			nTargetCumulativeFrequency += nTargetFrequency;
			nTarget++;
		}

		// Cas ou la partie de l'attribut finit avant celle de la partition multivarie:
		//  il faut creer une nouvelle partie multivariee
		if (nTargetCumulativeFrequency < targetPart->GetCumulativeFrequency())
		{
			// Creation d'une nouvelle partie multivariee avant la partie courante
			newTargetPart = cast(KWPSTargetIntervalScore*, TPNewPart());

			// Gestion rudimentaire du manque de memoire
			if (newTargetPart == NULL)
				AddFatalError("Not enough working memory to optimize variable selection");

			// Initialisation de la partie
			newTargetPart->SetCumulativeFrequency(nTargetCumulativeFrequency);
			newTargetPart->SetFrequency(nTargetCumulativeFrequency - (targetPart->GetCumulativeFrequency() -
										  targetPart->GetFrequency()));
			newTargetPart->SetRefCount(1);

			// Initialisation de son vecteur de probabilites conditionnelles a partir
			// de la partie dont elle est issue
			newTargetPart->GetLnSourceConditionalProbs()->CopyFrom(
			    targetPart->GetLnSourceConditionalProbs());

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
		// Cas ou la partie de l'attribut finit apres de la partition multivarie:
		//  il faut passer a la partie multivariee suivante
		else if (nTargetCumulativeFrequency > targetPart->GetCumulativeFrequency())
		{
			// Acces a la partie cible multivariee suivante
			targetPosition = position;
			targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetNext(position));
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
			targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPosition) == targetPart);

			// Passage a la partie de l'attribut suivante
			nTargetFrequency = ivTargetPartFrequencies.GetAt(nTarget);
			nTargetCumulativeFrequency += nTargetFrequency;
			nTarget++;
		}
	}
	ensure(TPCheck());
}

void KWRegressorSelectionScore::TPRemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	IntVector ivTargetPartFrequencies;
	int nTarget;
	int nTargetFrequency;
	int nTargetCumulativeFrequency;
	POSITION position;
	POSITION targetPosition;
	KWPSTargetIntervalScore* targetPart;
	int nDeletedTargetPartFrequency;

	require(TPCheck());
	require(dataPreparationAttribute != NULL);

	// Calcul des effectifs par partie de l'attribut cible
	ExportTargetPartFrequencies(dataPreparationAttribute, &ivTargetPartFrequencies);

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
			targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetNext(position));
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
			targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPosition) == targetPart);
		}
		// Cas ou la partie de l'attribut coincide avec la partition multivarie:
		//  il faut decrementer le compteur d'utilisation de la partie cible
		else
		{
			assert(nTargetCumulativeFrequency == targetPart->GetCumulativeFrequency());

			// Incrementation du compteur de reference de la partie
			targetPart->SetRefCount(targetPart->GetRefCount() - 1);

			// Si le compteur passe a 0, il faut supprimer la partie de la liste
			nDeletedTargetPartFrequency = 0;
			if (targetPart->GetRefCount() == 0)
			{
				nDeletedTargetPartFrequency = targetPart->GetFrequency();
				olTargetPartition.RemoveAt(targetPosition);
				TPDeletePart(targetPart);
			}

			// Acces a la partie cible multivariee suivante
			targetPosition = position;
			targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetNext(position));
			assert(olTargetPartition.GetAt(targetPosition) == targetPart);

			// Passage a la partie de l'attribut suivante
			nTargetFrequency = ivTargetPartFrequencies.GetAt(nTarget);
			nTargetCumulativeFrequency += nTargetFrequency;
			nTarget++;

			// Mise a jour de la partie suivant la partie detruite
			targetPart->SetFrequency(targetPart->GetFrequency() + nDeletedTargetPartFrequency);
		}
	}
	ensure(TPCheck());
}

void KWRegressorSelectionScore::TPUpgradeAttributeSourceConditionalProbs(
    KWDataPreparationAttribute* dataPreparationAttribute, Continuous cWeight)
{
	IntVector ivTargetPartFrequencies;
	int nTarget;
	int nTargetFrequency;
	int nTargetCumulativeFrequency;
	POSITION position;
	KWPSTargetIntervalScore* targetPart;
	ContinuousVector* cvScore;

	require(TPCheck());
	require(dataPreparationAttribute != NULL);

	// Calcul des effectifs par partie de l'attribut cible
	ExportTargetPartFrequencies(dataPreparationAttribute, &ivTargetPartFrequencies);

	// Parcours synchronise de la partition cible courante et de la partition cible de l'attribut
	// pour determiner mettre a jour les probabilites conditionnelles
	nTargetFrequency = 0;
	nTargetCumulativeFrequency = 0;
	nTarget = 0;
	targetPart = NULL;
	position = olTargetPartition.GetHeadPosition();
	while (position != NULL)
	{
		targetPart = cast(KWPSTargetIntervalScore*, olTargetPartition.GetNext(position));

		// Passage a la partie de l'attribut suivante si necessaire
		if (targetPart->GetCumulativeFrequency() > nTargetCumulativeFrequency)
		{
			nTargetFrequency = ivTargetPartFrequencies.GetAt(nTarget);
			nTargetCumulativeFrequency += nTargetFrequency;
			nTarget++;
		}

		// On verifie que la partie multivariee est inclue dans la partie de l'attribut
		assert(targetPart->GetCumulativeFrequency() <= nTargetCumulativeFrequency);
		assert(nTargetCumulativeFrequency - nTargetFrequency <=
		       targetPart->GetCumulativeFrequency() - targetPart->GetFrequency());

		// Calcul du nouveau score dans toutes les instances
		cvScore = targetPart->GetLnSourceConditionalProbs();
		check(cvScore);
		dataPreparationBase->UpgradeTargetConditionalLnProbsAt(dataPreparationAttribute, nTarget - 1, cWeight,
								       cvScore, cvScore);
	}
	assert(nTargetCumulativeFrequency == GetInstanceNumber());
	assert(nTarget == ivTargetPartFrequencies.GetSize());
}

KWPSTargetPartScore* KWRegressorSelectionScore::TPCreatePart()
{
	return new KWPSTargetIntervalScore;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Classe KWPSTargetIntervalScore

KWPSTargetIntervalScore::KWPSTargetIntervalScore()
{
	nFrequency = 0;
	nCumulativeFrequency = 0;
	nRefCount = 0;
}

KWPSTargetIntervalScore::~KWPSTargetIntervalScore() {}

void KWPSTargetIntervalScore::WriteHeaderLineReport(ostream& ost)
{
	ost << "Frequency\t"
	    << "CumulativeFrequency\t"
	    << "RefCount";
}

void KWPSTargetIntervalScore::WriteLineReport(ostream& ost)
{
	ost << nFrequency << "\t" << nCumulativeFrequency << "\t" << nRefCount;
}

//////////////////////////////////////////////////////////////////////////////
//  classe KWGeneralizedClassifierSelectionScore

KWGeneralizedClassifierSelectionScore::KWGeneralizedClassifierSelectionScore()
{
	targetSignatureSpec = new KWTargetSignatureSpec;
}

KWGeneralizedClassifierSelectionScore::~KWGeneralizedClassifierSelectionScore()
{
	oaTargetPartition.DeleteAll();
	CleanValueTargetGroupMatchings();
	delete targetSignatureSpec;
}

double KWGeneralizedClassifierSelectionScore::ComputeSelectionDataCost()
{
	boolean bDisplay = false;
	NumericKeyDictionary nkdParts;
	double dDataCost;
	Continuous cInstanceActualScore;
	Continuous cDeltaScore;
	Continuous cMaxScore;
	double dMaxExpScore;
	double dInstanceInverseProb;
	int nInstance;
	int nActualTarget;
	int nPart;
	KWPSTargetValueSetScore* targetPart;
	KWPSTargetValueSetScore* actualTargetPart;
	int nTargetNumber;
	double dLaplaceEpsilon;
	double dInstanceNumber;
	double dLaplaceNumerator;
	double dLaplaceDenominator;

	require(Check());
	require(TPCheck());
	require(dataPreparationBase->IsPreparedDataComputed());

	///////////////////////////////////////////////////////////////////////////////////////////
	// L'ecriture de cette methode, tres souvent utilisee, a ete particulierement optimisee
	//  . memorisation dans des variables locales de donnees de travail
	//  . precalcul des parties constantes des couts
	//  . precalcul des borne sup des boucle (pour eviter leur reevaluation)

	// Entete de la trace
	if (bDisplay)
	{
		cout << "\nIndex\tTarget\tProb\tValue\tEvaluation";
		for (nPart = 0; nPart < oaTargetPartition.GetSize(); nPart++)
			cout << "\tScore" << nPart + 1;
		cout << "\n";
	}

	// Calcul d'un epsilon de Laplace pour la gestion des petites probabilites
	// En effet, cette normalisation est faite en univarie, mais le probleme peut
	// a nouveau se poser en multivarie du fait de la multiplication de probabilites
	// En univarie, l'epsilon est tres petit (e = 1/InstanceNumber), car on considere
	// que l'estimation MODL est de bonne qualite.
	// En multivarie, on se contente du "proche du classique" e=0.5/J,
	// ce qui permet d'avoir un denominateur en N+0.5 quelque soit le nombre J de classes
	nTargetNumber = GetTargetDescriptiveStats()->GetValueNumber();
	dInstanceNumber = GetInstanceNumber();
	dLaplaceEpsilon = 0.5 / nTargetNumber;

	// Prise en compte de l'epsilon pour une probabilite p
	//   p = p*N / N
	//   p_Laplace = (p*N + e)/(N + J*e)
	// Precalcul une fois pour toutes du denominateur
	dLaplaceDenominator = dInstanceNumber + dLaplaceEpsilon * nTargetNumber;

	// Calcul des seuils de score max pour eviter les exponentielles infinies (vis a vis de la precision machine)
	dMaxExpScore = DBL_MAX / dInstanceNumber;
	cMaxScore = (Continuous)log(dMaxExpScore);

	// Calcul du nombre de predictions correctes
	dDataCost = 0;
	for (nInstance = 0; nInstance < GetInstanceNumber(); nInstance++)
	{
		nActualTarget = dataPreparationBase->GetTargetIndexes()->GetAt(nInstance);
		assert(0 <= nActualTarget and nActualTarget < oaTargetValueParts.GetSize());

		// Recherche du vecteur de probabilite pour la classe cible reelle
		actualTargetPart = cast(KWPSTargetValueSetScore*, oaTargetValueParts.GetAt(nActualTarget));
		cInstanceActualScore = actualTargetPart->GetLnSourceConditionalProbs()->GetAt(nInstance);

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
		dInstanceInverseProb = 0.0;
		for (nPart = 0; nPart < oaTargetPartition.GetSize(); nPart++)
		{
			targetPart = cast(KWPSTargetValueSetScore*, oaTargetPartition.GetAt(nPart));

			// Cas particulier pour eviter les calculs
			if (targetPart == actualTargetPart)
				dInstanceInverseProb += targetPart->GetFrequency();
			// Cas general, impliquant un calcul d'exponentiel
			else
			{
				// Difference de score pour l'intervalle cible en cours
				cDeltaScore =
				    targetPart->GetLnSourceConditionalProbs()->GetAt(nInstance) - cInstanceActualScore;

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
		if (bDisplay)
		{
			cout << nInstance << "\t" << nActualTarget << "\t" << dLaplaceNumerator / dLaplaceDenominator
			     << "\t" << -log(dLaplaceNumerator / dLaplaceDenominator) << "\t" << dDataCost;
			for (nPart = 0; nPart < oaTargetPartition.GetSize(); nPart++)
			{
				targetPart = cast(KWPSTargetValueSetScore*, oaTargetPartition.GetAt(nPart));
				cout << "\t" << targetPart->GetLnSourceConditionalProbs()->GetAt(nInstance);
			}
			cout << "\n";
		}
	}

	// Mise a jour de l'evaluation globale (partie denominateurs)
	dDataCost += dInstanceNumber * log(dLaplaceDenominator);
	return dDataCost;
}

boolean KWGeneralizedClassifierSelectionScore::TPCreate()
{
	boolean bOk;
	KWPSTargetValueSetScore* targetPart;
	int nTarget;

	require(GetTargetAttributeType() == KWType::Symbol);
	require(Check());
	require(oaTargetValueParts.GetSize() == 0);

	// Initialisation des services d'indexation de groupes cible pour les attributs
	InitialiseTargetValueGroupMatchings();

	// Creation d'un vecteur de probabilites conditionnelles pour un unique intervalle cible initial
	bOk = true;
	targetPart = cast(KWPSTargetValueSetScore*, TPNewPart());
	if (targetPart != NULL)
	{
		targetPart->SetFrequency(GetInstanceNumber());
		targetPart->GetTargetSignature()->SetSize(0);
	}
	else
		bOk = false;

	// Initialisation de la taille max du tableau de parties cibles
	if (bOk)
	{
		bOk = oaTargetPartition.SetLargeSize(GetTargetValueNumber());
	}

	// Initialisation de la taille du tableau des parties cibles indexe par les valeurs
	if (bOk)
	{
		bOk = oaTargetValueParts.SetLargeSize(GetTargetValueNumber());
	}

	// Initialisation de ce tableau avec l'unique partie initiale
	if (bOk)
	{
		check(targetPart);
		assert(targetPart->GetFrequency() == GetInstanceNumber());
		assert(targetPart->GetTargetSignature()->GetSize() == 0);

		// Initialisation du tableau de parties
		oaTargetPartition.SetSize(1);
		oaTargetPartition.SetAt(0, targetPart);

		// Association entre les valeurs et la partie initiale
		for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
			oaTargetValueParts.SetAt(nTarget, targetPart);
	}

	ensure(not bOk or TPCheck());
	return bOk;
}

void KWGeneralizedClassifierSelectionScore::TPInitialize()
{
	KWPSTargetValueSetScore* targetPart;
	int nTarget;
	int nPart;

	require(GetTargetAttributeType() == KWType::Symbol);
	require(Check());
	require(TPCheck());

	// Initialisation de la specification de la signature
	targetSignatureSpec->Initialize();

	// Destruction de toutes les parties, sauf la premiere
	for (nPart = 1; nPart < oaTargetPartition.GetSize(); nPart++)
	{
		targetPart = cast(KWPSTargetValueSetScore*, oaTargetPartition.GetAt(nPart));
		TPDeletePart(targetPart);
	}
	oaTargetPartition.SetSize(1);

	// Reinitialisation de l'unique partie en cours
	targetPart = cast(KWPSTargetValueSetScore*, oaTargetPartition.GetAt(0));
	targetPart->SetFrequency(GetInstanceNumber());
	targetPart->GetTargetSignature()->SetSize(0);

	// Initialisation des probabilites conditionnelles a 1 (Ln(prob) a 0)
	assert(targetPart->GetLnSourceConditionalProbs()->GetSize() == GetInstanceNumber());
	targetPart->GetLnSourceConditionalProbs()->Initialize();

	// Reinitialisation de l'association valeur-partie avec l'unique partie
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		oaTargetValueParts.SetAt(nTarget, targetPart);

	ensure(TPCheck());
}

void KWGeneralizedClassifierSelectionScore::TPDelete()
{
	KWPSTargetValueSetScore* targetPart;
	int nPart;

	// Destruction des parties, avec la methode adequate
	for (nPart = 0; nPart < oaTargetPartition.GetSize(); nPart++)
	{
		targetPart = cast(KWPSTargetValueSetScore*, oaTargetPartition.GetAt(nPart));
		TPDeletePart(targetPart);
	}

	// Reinitialisation des tableaux
	oaTargetValueParts.SetSize(0);
	oaTargetPartition.SetSize(0);

	// Nettoyage des services d'indexation de groupes cible pour les attributs
	CleanValueTargetGroupMatchings();

	// Methode ancetre
	KWPredictorSelectionScore::TPDelete();
}

boolean KWGeneralizedClassifierSelectionScore::TPCheck() const
{
	boolean bOk = true;
	int nTarget;
	int nPart;
	KWPSTargetValueSetScore* targetPart;
	NumericKeyDictionary nkdParts;
	SortedList slCheckedTargetPartition(KWPSTargetValueSetScoreCompareTargetSignature);
	int nTotalFrequency;
	ALString sTmp;

	require(GetTargetAttributeType() == KWType::Symbol);
	require(Check());

	// Collecte des parties par parcours des parties references pour chaque valeur
	for (nTarget = 0; nTarget < oaTargetValueParts.GetSize(); nTarget++)
	{
		targetPart = cast(KWPSTargetValueSetScore*, oaTargetValueParts.GetAt(nTarget));

		// Verification des signatures des parties
		if (not CheckTargetSignature(nTarget, targetPart->GetTargetSignature()))
		{
			AddError(sTmp + "Target part (" + GetTargetValueAt(nTarget) + ") has a wrong signature " +
				 targetPart->GetObjectLabel());
			bOk = false;
			break;
		}

		// Memorisation de toute partie non deja referencee
		if (nkdParts.Lookup(targetPart) == NULL)
		{
			// Enregistrement dans un dictionnaire
			nkdParts.SetAt((NUMERIC)targetPart, targetPart);

			// Verification de l'unicite des signatures en utilisant une liste triee
			if (slCheckedTargetPartition.Find(targetPart) == NULL)
				slCheckedTargetPartition.Add(targetPart);
			else
			{
				AddError(sTmp + "Target part (" + GetTargetValueAt(nTarget) + ") has a signature " +
					 targetPart->GetObjectLabel() + " already used");
				bOk = false;
				break;
			}
		}
	}

	// Tests supplementaires
	if (bOk)
	{
		// Verification de la taille de la partition
		assert(oaTargetPartition.NoNulls());
		if (oaTargetPartition.GetSize() != nkdParts.GetCount())
		{
			AddError(sTmp + "Number of different target parts (" + IntToString(nkdParts.GetCount()) +
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
			for (nPart = 0; nPart < oaTargetPartition.GetSize(); nPart++)
			{
				targetPart = cast(KWPSTargetValueSetScore*, oaTargetPartition.GetAt(nPart));
				check(targetPart);
				assert(nkdParts.Lookup((NUMERIC)targetPart) != NULL);

				// Mise a jour de l'effectif total
				assert(0 < targetPart->GetFrequency() and
				       targetPart->GetFrequency() <= GetInstanceNumber());
				nTotalFrequency += targetPart->GetFrequency();

				// Verification de la partie
				if (targetPart->GetLnSourceConditionalProbs()->GetSize() != GetInstanceNumber())
				{
					AddError(sTmp + "Size of target part probability vector (" +
						 IntToString(targetPart->GetLnSourceConditionalProbs()->GetSize()) +
						 ") is inconsistent with the database size (" +
						 IntToString(GetInstanceNumber()) + ")");
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

void KWGeneralizedClassifierSelectionScore::TPWrite(ostream& ost) const
{
	int nPart;
	KWPSTargetPartScore* targetPart;

	require(TPCheck());

	for (nPart = 0; nPart < oaTargetPartition.GetSize(); nPart++)
	{
		targetPart = cast(KWPSTargetPartScore*, oaTargetPartition.GetAt(nPart));
		if (nPart == 0)
		{
			targetPart->WriteHeaderLineReport(ost);
			ost << "\n";
		}
		targetPart->WriteLineReport(ost);
		ost << "\n";
	}
}

void KWGeneralizedClassifierSelectionScore::TPAddAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	int nTarget;
	NumericKeyDictionary nkdInitialParts;
	NumericKeyDictionary* nkdSubparts;
	KWDataGridStats* preparedDataGridStats;
	int nPart;
	ObjectArray oaTargetPartNumericKeys;
	KWSortableIndex* targetPartNumericKey;
	KWPSTargetValueSetScore* targetPart;
	KWPSTargetValueSetScore* newTargetPartScore;
	const IntVector* ivTargetValueMatching;

	require(TPCheck());
	require(dataPreparationAttribute != NULL);
	require(targetSignatureSpec->GetAttributeIndex(dataPreparationAttribute) == -1);

	// Acces au vecteur d'index des groupes cibles d'un attribut, permettant d'associer
	// un index de groupe cible a chaque index de valeur cible
	ivTargetValueMatching = GetAttributeTargetValueGroupMatching(dataPreparationAttribute);

	// Creation de cles d'acces au partie cibles, qui serviront de cle d'acces dans des
	// dictionnaire a cle numerique
	preparedDataGridStats = dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats();
	oaTargetPartNumericKeys.SetSize(
	    preparedDataGridStats->GetAttributeAt(preparedDataGridStats->GetFirstTargetAttributeIndex())
		->GetPartNumber());
	for (nPart = 0; nPart < oaTargetPartNumericKeys.GetSize(); nPart++)
	{
		targetPartNumericKey = new KWSortableIndex;
		targetPartNumericKey->SetIndex(nPart);
		oaTargetPartNumericKeys.SetAt(nPart, targetPartNumericKey);
	}

	// Parcours des parties initiales referencees pour chaque valeur
	// Chaque partie initiale sera potentiellement splitte en sous-partie en fonction de la
	// partition issue du nouvel attribut
	for (nTarget = 0; nTarget < oaTargetValueParts.GetSize(); nTarget++)
	{
		targetPart = cast(KWPSTargetValueSetScore*, oaTargetValueParts.GetAt(nTarget));

		// Recherche de la clee numerique correspondant a la partie provenant du nouvel attribut
		nPart = ivTargetValueMatching->GetAt(nTarget);
		targetPartNumericKey = cast(KWSortableIndex*, oaTargetPartNumericKeys.GetAt(nPart));
		assert(targetPartNumericKey->GetIndex() == nPart);

		// Recherche des sous-parties liee a la partie initiale
		nkdSubparts = cast(NumericKeyDictionary*, nkdInitialParts.Lookup(targetPart));

		// Creation de ce dictionnaire de sous-partie si necessaire, et dans ce cas
		// memorisation de l'ancienne partie comme sous-partie associee au groupe en cours
		newTargetPartScore = NULL;
		if (nkdSubparts == NULL)
		{
			nkdSubparts = new NumericKeyDictionary;
			nkdInitialParts.SetAt((NUMERIC)targetPart, nkdSubparts);

			// Memorisation de la premiere sous partie et de son effectif
			newTargetPartScore = targetPart;
			newTargetPartScore->SetFrequency(GetTargetValueFrequencyAt(nTarget));
			nkdSubparts->SetAt((NUMERIC)targetPartNumericKey, newTargetPartScore);

			// Mise a jour d'une signature par ajout d'un index de groupe
			assert(newTargetPartScore->GetTargetSignature()->GetSize() ==
			       targetSignatureSpec->GetAttributeNumber());
			UpgradeTargetSignatureWithAddedAttribute(nTarget, ivTargetValueMatching,
								 newTargetPartScore->GetTargetSignature());
		}
		// Sinon, on recherche la sous-partie
		else
		{
			// Recherche de la sous-partie
			newTargetPartScore =
			    cast(KWPSTargetValueSetScore*, nkdSubparts->Lookup((NUMERIC)targetPartNumericKey));

			// Creation si necessaire
			if (newTargetPartScore == NULL)
			{
				newTargetPartScore = cast(KWPSTargetValueSetScore*, TPNewPart());

				// Gestion rudimentaire du manque de memoire
				if (newTargetPartScore == NULL)
					AddFatalError("Not enough working memory to optimize variable selection");

				// Initialisation de son vecteur de probabilites conditionnelles a partir
				// de la partie dont elle est issue
				newTargetPartScore->GetLnSourceConditionalProbs()->CopyFrom(
				    targetPart->GetLnSourceConditionalProbs());

				// Memorisation de la sous partie et de son effectif
				newTargetPartScore->SetFrequency(GetTargetValueFrequencyAt(nTarget));
				nkdSubparts->SetAt((NUMERIC)targetPartNumericKey, newTargetPartScore);

				// Ajout de la nouvelle partie dans la partition cible
				oaTargetPartition.Add(newTargetPartScore);

				// Mise a jour d'une signature par ajout d'un index de groupe
				// On commence par recopier l'ancienne signature, en supprimant sa derniere composant
				assert(targetPart->GetTargetSignature()->GetSize() ==
				       targetSignatureSpec->GetAttributeNumber() + 1);
				newTargetPartScore->GetTargetSignature()->CopyFrom(targetPart->GetTargetSignature());
				newTargetPartScore->GetTargetSignature()->SetSize(
				    newTargetPartScore->GetTargetSignature()->GetSize() - 1);
				UpgradeTargetSignatureWithAddedAttribute(nTarget, ivTargetValueMatching,
									 newTargetPartScore->GetTargetSignature());
			}
			// Mise a jour de l'effectif de la sous-partie
			else
				newTargetPartScore->SetFrequency(newTargetPartScore->GetFrequency() +
								 GetTargetValueFrequencyAt(nTarget));
		}
		check(newTargetPartScore);

		// Memorisation de la nouvelle partie cible associe a la valeur cible
		oaTargetValueParts.SetAt(nTarget, newTargetPartScore);
	}

	// Mise a jour des specifications de signature
	targetSignatureSpec->AddAttribute(dataPreparationAttribute);

	// Nettoyage
	nkdInitialParts.DeleteAll();
	oaTargetPartNumericKeys.DeleteAll();

	ensure(TPCheck());
}

void KWGeneralizedClassifierSelectionScore::TPRemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	int nTarget;
	SortedList slRemainingParts(KWPSTargetValueSetScoreCompareTargetSignature);
	POSITION position;
	KWPSTargetValueSetScore* targetPart;
	KWPSTargetValueSetScore* newTargetPartScore;
	int nRemovedSignatureAttributeIndex;
	ObjectArray oaPartsToDelete;
	int nPart;

	require(TPCheck());
	require(dataPreparationAttribute != NULL);
	require(targetSignatureSpec->GetAttributeIndex(dataPreparationAttribute) != -1);

	// Acces a l'index de l'attribut a supprimer dans la signature cible
	nRemovedSignatureAttributeIndex = targetSignatureSpec->GetAttributeIndex(dataPreparationAttribute);

	// Retaillage initiale de la partition cible
	oaTargetPartition.SetSize(0);

	// Parcours des parties initiales referencees pour chaque valeur
	// Chaque partie initiale sera potentiellement simplifiee en fonction de la partition issue
	// du nouvel attribut, ce qui entrainera la supression des parties redondantes
	for (nTarget = 0; nTarget < oaTargetValueParts.GetSize(); nTarget++)
	{
		targetPart = cast(KWPSTargetValueSetScore*, oaTargetValueParts.GetAt(nTarget));

		// Traitement de la partie, si non deja traitee (signature diminuee)
		if (targetPart->GetTargetSignature()->GetSize() == targetSignatureSpec->GetAttributeNumber())
		{
			// Mise a jour de la signature de la partie par supression d'un index de groupe
			UpgradeTargetSignatureWithRemovedAttribute(nTarget, nRemovedSignatureAttributeIndex,
								   targetPart->GetTargetSignature());
			assert(targetPart->GetTargetSignature()->GetSize() ==
			       targetSignatureSpec->GetAttributeNumber() - 1);

			// Recherche de la partie parmi les parties restantes
			position = slRemainingParts.Find(targetPart);
			newTargetPartScore = NULL;
			if (position != NULL)
				newTargetPartScore = cast(KWPSTargetValueSetScore*, slRemainingParts.GetAt(position));

			// Memorisation si partie restante
			if (newTargetPartScore == NULL)
			{
				newTargetPartScore = targetPart;
				slRemainingParts.Add(newTargetPartScore);

				// Memorisation de la partie dans la partition cible
				oaTargetPartition.Add(newTargetPartScore);
			}
			// Consolidation de la partie restante sinon
			else
			{
				assert(newTargetPartScore != targetPart);

				// Prise en compte de l'effectif de la partie a fusionner
				newTargetPartScore->SetFrequency(newTargetPartScore->GetFrequency() +
								 targetPart->GetFrequency());

				// Memorisation de la nouvelle association
				oaTargetValueParts.SetAt(nTarget, newTargetPartScore);

				// Enregistrement de la partie a detruire
				// (elle est encore potentiellement referencee pour d'autre valeurs cibles)
				oaPartsToDelete.Add(targetPart);
			}
		}
		// Traitement de la partie, si deja traitee
		else
		{
			assert(targetPart->GetTargetSignature()->GetSize() ==
			       targetSignatureSpec->GetAttributeNumber() - 1);

			// Recherche de la partie parmi les parties restantes
			position = slRemainingParts.Find(targetPart);
			assert(position != NULL);
			newTargetPartScore = cast(KWPSTargetValueSetScore*, slRemainingParts.GetAt(position));

			// Memorisation de la nouvelle association
			oaTargetValueParts.SetAt(nTarget, newTargetPartScore);
		}
	}
	assert(slRemainingParts.GetCount() == oaTargetPartition.GetSize());

	// Destruction effective des parties
	for (nPart = 0; nPart < oaPartsToDelete.GetSize(); nPart++)
	{
		targetPart = cast(KWPSTargetValueSetScore*, oaPartsToDelete.GetAt(nPart));
		TPDeletePart(targetPart);
	}

	// Mise a jour des specification de signature
	targetSignatureSpec->RemoveAttribute(dataPreparationAttribute);

	ensure(TPCheck());
}

void KWGeneralizedClassifierSelectionScore::TPUpgradeAttributeSourceConditionalProbs(
    KWDataPreparationAttribute* dataPreparationAttribute, Continuous cWeight)
{
	int nPart;
	KWPSTargetValueSetScore* targetPart;
	ContinuousVector* cvScore;
	KWDataGridStats* dataGridStats;
	const KWDGSAttributePartition* targetPartition;
	int nAttributeIndex;
	int nAttributePart;

	require(TPCheck());
	require(dataPreparationAttribute != NULL);
	require(targetSignatureSpec->GetAttributeIndex(dataPreparationAttribute) != -1);

	// Verification de la grille de preparation
	dataGridStats = dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats();
	check(dataGridStats != NULL);
	assert(dataGridStats->GetTargetAttributeNumber() == 1);
	targetPartition = dataGridStats->GetAttributeAt(dataGridStats->GetFirstTargetAttributeIndex());
	assert(targetPartition->GetAttributeType() == KWType::Symbol);

	// Index de l'attribut dans les specifications de la jointure
	nAttributeIndex = targetSignatureSpec->GetAttributeIndex(dataPreparationAttribute);

	// Collecte des parties par parcours des parties references pour chaque valeur
	for (nPart = 0; nPart < oaTargetPartition.GetSize(); nPart++)
	{
		targetPart = cast(KWPSTargetValueSetScore*, oaTargetPartition.GetAt(nPart));

		// Calcul du nouveau score dans toutes les instances
		cvScore = targetPart->GetLnSourceConditionalProbs();
		check(cvScore);
		nAttributePart = targetPart->GetTargetSignature()->GetAt(nAttributeIndex);
		dataPreparationBase->UpgradeTargetConditionalLnProbsAt(dataPreparationAttribute, nAttributePart,
								       cWeight, cvScore, cvScore);
	}

	ensure(TPCheck());
}

KWPSTargetPartScore* KWGeneralizedClassifierSelectionScore::TPCreatePart()
{
	return new KWPSTargetValueSetScore;
}

void KWGeneralizedClassifierSelectionScore::InitialiseTargetValueGroupMatchings()
{
	boolean bTrace = false;
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nAttribute;
	KWDataGridStats* dataGridStats;
	const KWDGSAttributePartition* targetPartition;
	const KWDGSAttributeGrouping* targetValueGroupsPartition;
	const KWDGSAttributeSymbolValues* targetValuesPartition;
	NumericKeyDictionary nkdTargetValueIndexes;
	KWSortableIndex* indexTargetValue;
	int nTarget;
	IntVector* ivTargetValueGroupMatching;
	int nValue;
	int nGroup;
	int nDefaultGroup;

	require(GetDataPreparationBase() != NULL);

	// Nettoyage initial
	CleanValueTargetGroupMatchings();

	// Creation d'un dictionaire de valeurs, pour memoriser leur index
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
	{
		indexTargetValue = new KWSortableIndex;
		indexTargetValue->SetIndex(nTarget);
		nkdTargetValueIndexes.SetAt(GetTargetValueAt(nTarget).GetNumericKey(), indexTargetValue);
	}

	// Parcours des attributs de preparation
	for (nAttribute = 0; nAttribute < GetDataPreparationBase()->GetDataPreparationUsedAttributes()->GetSize();
	     nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*,
			 GetDataPreparationBase()->GetDataPreparationUsedAttributes()->GetAt(nAttribute));

		// Acces a la grille de preparation
		dataGridStats = dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats();
		if (dataGridStats != NULL)
		{
			assert(dataGridStats->GetTargetAttributeNumber() == 1);
			assert(dataGridStats->GetAttributeAt(dataGridStats->GetFirstTargetAttributeIndex())
				   ->GetAttributeType() == KWType::Symbol);

			// Acces a la partition des valeurss cibles pour cette grille
			targetPartition = dataGridStats->GetAttributeAt(dataGridStats->GetFirstTargetAttributeIndex());

			// Creation et memorisation du vecteur de matching
			ivTargetValueGroupMatching = new IntVector;
			ivTargetValueGroupMatching->SetSize(GetTargetValueNumber());
			nkdAttributeTargetValueGroupMatchings.SetAt((NUMERIC)dataPreparationAttribute,
								    ivTargetValueGroupMatching);

			// Initialisation prealable avec des -1, afin de verification et de gestion de la valeur special
			// StarValue
			for (nTarget = 0; nTarget < ivTargetValueGroupMatching->GetSize(); nTarget++)
				ivTargetValueGroupMatching->SetAt(nTarget, -1);

			// Initialisation dans le cas d'une partition en valeurs elementaires
			if (targetPartition->ArePartsSingletons())
			{
				targetValuesPartition = cast(const KWDGSAttributeSymbolValues*, targetPartition);
				assert(targetValuesPartition->GetValueNumber() == GetTargetValueNumber());

				// Parcours des valeurs de la partition pour creer l'association
				for (nValue = 0; nValue < targetValuesPartition->GetValueNumber(); nValue++)
				{
					// Recherche de l'index de valeur cible correspondant
					indexTargetValue =
					    cast(KWSortableIndex*,
						 nkdTargetValueIndexes.Lookup(
						     targetValuesPartition->GetValueAt(nValue).GetNumericKey()));
					check(indexTargetValue);
					nTarget = indexTargetValue->GetIndex();

					// Memorisation de la correspondance
					assert(ivTargetValueGroupMatching->GetAt(nTarget) == -1);
					ivTargetValueGroupMatching->SetAt(nTarget, nValue);
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
					for (nValue = targetValueGroupsPartition->GetGroupFirstValueIndexAt(nGroup);
					     nValue <= targetValueGroupsPartition->GetGroupLastValueIndexAt(nGroup);
					     nValue++)
					{
						// Memorisation du groupe par defaut si l'on rencontre la valeur
						// speciale StarValue
						if (nDefaultGroup == -1 and targetValueGroupsPartition->GetValueAt(
										nValue) == Symbol::GetStarValue())
						{
							nDefaultGroup = nGroup;

							// Pour la valeur speciale, il n'y a pas de correspondance avec
							// une valeur cible particuliere, et il faut court-circuiter la
							// fin de la boucle
							continue;
						}

						// Recherche de l'index de valeur cible correspondant
						indexTargetValue = cast(
						    KWSortableIndex*, nkdTargetValueIndexes.Lookup(
									  targetValueGroupsPartition->GetValueAt(nValue)
									      .GetNumericKey()));
						check(indexTargetValue);
						nTarget = indexTargetValue->GetIndex();

						// Memorisation de la correspondance
						assert(ivTargetValueGroupMatching->GetAt(nTarget) == -1);
						ivTargetValueGroupMatching->SetAt(nTarget, nGroup);
					}
				}
				assert(nDefaultGroup != -1);

				// On associe le groupe par defaut aux valeurs cibles non traitees
				for (nTarget = 0; nTarget < ivTargetValueGroupMatching->GetSize(); nTarget++)
				{
					if (ivTargetValueGroupMatching->GetAt(nTarget) == -1)
						ivTargetValueGroupMatching->SetAt(nTarget, nDefaultGroup);
				}
			}

			// Trace
			if (bTrace)
			{
				cout << "Matching\t" << dataPreparationAttribute->GetObjectLabel() << "\n";
				for (nTarget = 0; nTarget < ivTargetValueGroupMatching->GetSize(); nTarget++)
				{
					nGroup = ivTargetValueGroupMatching->GetAt(nTarget);
					assert(targetPartition->ComputeSymbolPartIndex(GetTargetValueAt(nTarget)) ==
					       nGroup);
					cout << "\t" << GetTargetValueAt(nTarget) << "\t" << nGroup << "\n";
				}
			}
		}
	}

	// Nettoyage du dictionnaire de valeurs
	nkdTargetValueIndexes.DeleteAll();
}

void KWGeneralizedClassifierSelectionScore::CleanValueTargetGroupMatchings()
{
	nkdAttributeTargetValueGroupMatchings.DeleteAll();
}

int KWGeneralizedClassifierSelectionScore::GetTargetValueNumber() const
{
	const KWDGSAttributeSymbolValues* targetValues;

	require(Check());
	require(GetTargetValueStats()->GetAttributeNumber() == 1);
	require(GetTargetValueStats()->GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
	require(GetTargetValueStats()->GetAttributeAt(0)->ArePartsSingletons());

	targetValues = cast(const KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0));
	return targetValues->GetPartNumber();
}

Symbol& KWGeneralizedClassifierSelectionScore::GetTargetValueAt(int nIndex) const
{
	const KWDGSAttributeSymbolValues* targetValues;

	require(0 <= nIndex and nIndex < GetTargetValueNumber());

	targetValues = cast(const KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0));
	return targetValues->GetValueAt(nIndex);
}

int KWGeneralizedClassifierSelectionScore::GetTargetValueFrequencyAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetTargetValueNumber());
	return GetTargetValueStats()->GetUnivariateCellFrequencyAt(nIndex);
}

const IntVector* KWGeneralizedClassifierSelectionScore::GetAttributeTargetValueGroupMatching(
    KWDataPreparationAttribute* dataPreparationAttribute) const
{
	require(dataPreparationAttribute != NULL);
	return cast(const IntVector*, nkdAttributeTargetValueGroupMatchings.Lookup((NUMERIC)dataPreparationAttribute));
}

void KWGeneralizedClassifierSelectionScore::ComputeTargetSignature(int nTargetValueIndex,
								   IntVector* ivTargetSignature) const
{
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nAttribute;
	const IntVector* ivMatching;

	require(0 <= nTargetValueIndex and nTargetValueIndex < GetTargetValueNumber());
	require(ivTargetSignature != NULL);

	// Taillage de la signature
	ivTargetSignature->SetSize(targetSignatureSpec->GetAttributeNumber());

	// Calcul des index de groupes par attribut de la signature
	for (nAttribute = 0; nAttribute < targetSignatureSpec->GetAttributeNumber(); nAttribute++)
	{
		dataPreparationAttribute = targetSignatureSpec->GetAttributeAt(nAttribute);

		// Acces au matching
		ivMatching = GetAttributeTargetValueGroupMatching(dataPreparationAttribute);
		check(ivMatching);

		// Memorisation de l'index du groupe
		ivTargetSignature->SetAt(nAttribute, ivMatching->GetAt(nTargetValueIndex));
	}

	ensure(CheckTargetSignature(nTargetValueIndex, ivTargetSignature));
}

void KWGeneralizedClassifierSelectionScore::UpgradeTargetSignatureWithAddedAttribute(
    int nTargetValueIndex, const IntVector* ivAddedAttributeTargetValueGroupMatching,
    IntVector* ivTargetSignature) const
{
	require(0 <= nTargetValueIndex and nTargetValueIndex < GetTargetValueNumber());
	require(ivTargetSignature != NULL);
	require(CheckTargetSignature(nTargetValueIndex, ivTargetSignature));
	require(ivAddedAttributeTargetValueGroupMatching != NULL);

	// Ajout de l'index de groupe en fin de signature
	ivTargetSignature->Add(ivAddedAttributeTargetValueGroupMatching->GetAt(nTargetValueIndex));
}

void KWGeneralizedClassifierSelectionScore::UpgradeTargetSignatureWithRemovedAttribute(
    int nTargetValueIndex, int nRemovedSignatureAttributeIndex, IntVector* ivTargetSignature) const
{
	require(0 <= nTargetValueIndex and nTargetValueIndex < GetTargetValueNumber());
	require(ivTargetSignature != NULL);
	require(CheckTargetSignature(nTargetValueIndex, ivTargetSignature));
	require(0 <= nRemovedSignatureAttributeIndex and
		nRemovedSignatureAttributeIndex < targetSignatureSpec->GetAttributeNumber());

	// La fin de signature remplace la place de l'attribut supprime
	ivTargetSignature->SetAt(nRemovedSignatureAttributeIndex,
				 ivTargetSignature->GetAt(ivTargetSignature->GetSize() - 1));
	ivTargetSignature->SetSize(ivTargetSignature->GetSize() - 1);
}

boolean KWGeneralizedClassifierSelectionScore::CheckTargetSignature(int nTargetValueIndex,
								    const IntVector* ivTargetSignature) const
{
	boolean bOk = true;

	require(0 <= nTargetValueIndex and nTargetValueIndex < GetTargetValueNumber());
	require(ivTargetSignature != NULL);

	KWDataPreparationAttribute* dataPreparationAttribute;
	int nAttribute;
	const IntVector* ivMatching;

	require(0 <= nTargetValueIndex and nTargetValueIndex < GetTargetValueNumber());
	require(ivTargetSignature != NULL);

	// Taille de la signature
	if (ivTargetSignature->GetSize() != targetSignatureSpec->GetAttributeNumber())
		bOk = false;

	// Calcul des index de groupes par attribut de la signature
	if (bOk)
	{
		for (nAttribute = 0; nAttribute < targetSignatureSpec->GetAttributeNumber(); nAttribute++)
		{
			dataPreparationAttribute = targetSignatureSpec->GetAttributeAt(nAttribute);

			// Acces au matching
			ivMatching = GetAttributeTargetValueGroupMatching(dataPreparationAttribute);
			check(ivMatching);

			// Memorisation de l'index du groupe
			if (ivTargetSignature->GetAt(nAttribute) != ivMatching->GetAt(nTargetValueIndex))
			{
				bOk = false;
				break;
			}
		}
	}
	return bOk;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Class KWTargetSignatureSpec

KWTargetSignatureSpec::KWTargetSignatureSpec() {}

KWTargetSignatureSpec::~KWTargetSignatureSpec()
{
	nkdTargetSignatureAttributeIndexes.DeleteAll();
}

void KWTargetSignatureSpec::Initialize()
{
	nkdTargetSignatureAttributeIndexes.DeleteAll();
	oaTargetSignatureAttributes.SetSize(0);
}

int KWTargetSignatureSpec::GetAttributeNumber() const
{
	return oaTargetSignatureAttributes.GetSize();
}

KWDataPreparationAttribute* KWTargetSignatureSpec::GetAttributeAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetAttributeNumber());
	return cast(KWDataPreparationAttribute*, oaTargetSignatureAttributes.GetAt(nIndex));
}

int KWTargetSignatureSpec::GetAttributeIndex(KWDataPreparationAttribute* dataPreparationAttribute) const
{
	KWSortableIndex* indexDataPreparationAttribute;

	// Recherche de l'index de attribut via le dictionnaire
	indexDataPreparationAttribute =
	    cast(KWSortableIndex*, nkdTargetSignatureAttributeIndexes.Lookup((NUMERIC)dataPreparationAttribute));
	if (indexDataPreparationAttribute == NULL)
		return -1;
	else
		return indexDataPreparationAttribute->GetIndex();
}

void KWTargetSignatureSpec::AddAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	KWSortableIndex* indexDataPreparationAttribute;

	require(Check());
	require(dataPreparationAttribute != NULL);
	require(GetAttributeIndex(dataPreparationAttribute) == -1);

	// Ajout de l'attribut en fin de la signature
	oaTargetSignatureAttributes.Add(dataPreparationAttribute);

	// Memorisation de son index
	indexDataPreparationAttribute = new KWSortableIndex;
	indexDataPreparationAttribute->SetIndex(oaTargetSignatureAttributes.GetSize() - 1);
	nkdTargetSignatureAttributeIndexes.SetAt((NUMERIC)dataPreparationAttribute, indexDataPreparationAttribute);

	ensure(Check());
}

void KWTargetSignatureSpec::RemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute)
{
	KWSortableIndex* indexDataPreparationAttribute;
	int nRemovedAttributeIndex;
	KWDataPreparationAttribute* lastDataPreparationAttribute;

	require(Check());
	require(dataPreparationAttribute != NULL);
	require(GetAttributeIndex(dataPreparationAttribute) != -1);

	// Recherche de l'index de l'attribut
	indexDataPreparationAttribute =
	    cast(KWSortableIndex*, nkdTargetSignatureAttributeIndexes.Lookup((NUMERIC)dataPreparationAttribute));
	nRemovedAttributeIndex = indexDataPreparationAttribute->GetIndex();
	assert(oaTargetSignatureAttributes.GetAt(indexDataPreparationAttribute->GetIndex()) ==
	       dataPreparationAttribute);

	// Supression de l'attribut de la signature
	oaTargetSignatureAttributes.SetAt(nRemovedAttributeIndex, NULL);
	nkdTargetSignatureAttributeIndexes.RemoveKey((NUMERIC)dataPreparationAttribute);
	delete indexDataPreparationAttribute;

	// Deplacement du dernier attribut de la signature a la place de l'attribut supprime
	if (oaTargetSignatureAttributes.GetSize() >= 2)
	{
		lastDataPreparationAttribute =
		    cast(KWDataPreparationAttribute*,
			 oaTargetSignatureAttributes.GetAt(oaTargetSignatureAttributes.GetSize() - 1));
		assert(lastDataPreparationAttribute != NULL or
		       nRemovedAttributeIndex == oaTargetSignatureAttributes.GetSize() - 1);

		// Uniquement si l'element supprime n'etait pas le dernier
		if (lastDataPreparationAttribute != NULL)
		{
			indexDataPreparationAttribute =
			    cast(KWSortableIndex*,
				 nkdTargetSignatureAttributeIndexes.Lookup((NUMERIC)lastDataPreparationAttribute));
			check(indexDataPreparationAttribute);
			indexDataPreparationAttribute->SetIndex(nRemovedAttributeIndex);
			oaTargetSignatureAttributes.SetAt(nRemovedAttributeIndex, lastDataPreparationAttribute);
		}
	}

	// Retaillage du tableau d'attribut
	oaTargetSignatureAttributes.SetSize(oaTargetSignatureAttributes.GetSize() - 1);

	ensure(Check());
}

boolean KWTargetSignatureSpec::Check() const
{
	boolean bOk = true;
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nAttribute;

	assert(oaTargetSignatureAttributes.GetSize() == nkdTargetSignatureAttributeIndexes.GetCount());

	// On verifie que les index sont tous corrects
	for (nAttribute = 0; nAttribute < oaTargetSignatureAttributes.GetSize(); nAttribute++)
	{
		dataPreparationAttribute = GetAttributeAt(nAttribute);
		if (bOk)
			bOk = dataPreparationAttribute != NULL;
		if (bOk)
			bOk = GetAttributeIndex(dataPreparationAttribute) == nAttribute;
		if (not bOk)
			break;
	}
	return bOk;
}

void KWTargetSignatureSpec::Write(ostream& ost)
{
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nAttribute;

	ost << "(";
	for (nAttribute = 0; nAttribute < oaTargetSignatureAttributes.GetSize(); nAttribute++)
	{
		dataPreparationAttribute = GetAttributeAt(nAttribute);
		if (nAttribute > 0)
			ost << ", ";
		ost << dataPreparationAttribute->GetObjectLabel();
	}
	ost << ")";
}

/////////////////////////////////////////////////////////////////////////////////////////
// Class KWPSTargetValueSetScore

KWPSTargetValueSetScore::KWPSTargetValueSetScore()
{
	nFrequency = 0;
}

KWPSTargetValueSetScore::~KWPSTargetValueSetScore() {}

void KWPSTargetValueSetScore::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

int KWPSTargetValueSetScore::GetFrequency() const
{
	return nFrequency;
}

IntVector* KWPSTargetValueSetScore::GetTargetSignature()
{
	return &ivTargetSignature;
}

void KWPSTargetValueSetScore::WriteHeaderLineReport(ostream& ost)
{
	ost << "Frequency\t"
	    << "Signature";
}

void KWPSTargetValueSetScore::WriteLineReport(ostream& ost)
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

const ALString KWPSTargetValueSetScore::GetObjectLabel() const
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

int KWPSTargetValueSetScoreCompareTargetSignature(const void* elem1, const void* elem2)
{
	KWPSTargetValueSetScore* targetValueSetScore1;
	KWPSTargetValueSetScore* targetValueSetScore2;
	int nDiff;
	int nIndex;

	check(elem1);
	check(elem2);

	// Acces aux objets
	targetValueSetScore1 = cast(KWPSTargetValueSetScore*, *(Object**)elem1);
	targetValueSetScore2 = cast(KWPSTargetValueSetScore*, *(Object**)elem2);

	// Comparaison des tailles
	nDiff = targetValueSetScore1->GetTargetSignature()->GetSize() -
		targetValueSetScore2->GetTargetSignature()->GetSize();

	// Comparaison des index de groupes, attributs par attribut
	if (nDiff == 0)
	{
		for (nIndex = 0; nIndex < targetValueSetScore1->GetTargetSignature()->GetSize(); nIndex++)
		{
			nDiff = targetValueSetScore1->GetTargetSignature()->GetAt(nIndex) -
				targetValueSetScore2->GetTargetSignature()->GetAt(nIndex);
			if (nDiff != 0)
				break;
		}
	}
	return nDiff;
}
