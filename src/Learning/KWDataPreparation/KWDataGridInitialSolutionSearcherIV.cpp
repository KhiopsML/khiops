// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridInitialSolutionSearcherIV.h"

KWDataGridInitialSolutionSearcherIV::KWDataGridInitialSolutionSearcherIV()
{
	learningSpec = NULL;
}

KWDataGridInitialSolutionSearcherIV::~KWDataGridInitialSolutionSearcherIV() {}

void KWDataGridInitialSolutionSearcherIV::SetLearningSpec(KWLearningSpec* specification)
{
	learningSpec = specification;
}

KWLearningSpec* KWDataGridInitialSolutionSearcherIV::GetLearningSpec() const
{
	return learningSpec;
}

boolean KWDataGridInitialSolutionSearcherIV::SearchInitialSolution(const KWDataGrid* initialDataGrid,
								   KWDataGrid* initialDataGridSolution) const
{
	boolean bOk;
	const boolean bTrace = false;
	const boolean bTraceDataGrid = false;
	ObjectArray oaSelectedAttributePairStats;
	KWDataGridManager dataGridManager;
	int nLowerPairNumber;
	int nUpperPairNumber;
	int nPairNumber;
	boolean bIsOptimizable;

	require(learningSpec != NULL);
	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(initialDataGridSolution != NULL);
	require(initialDataGridSolution->GetCellNumber() == 0);

	// Parametrage d'un libelle si une tache en cours
	if ((TaskProgression::IsInTask()))
		TaskProgression::DisplayLabel("Search initial solution using bivariate analysis");

	// Calcul des paires de variables
	bOk = ComputeInternalAttributesBivariateStats(initialDataGrid);

	// On remet le libelle a vide
	if ((TaskProgression::IsInTask()))
		TaskProgression::DisplayLabel("");

	// Selection des paires informatives retenues
	if (bOk)
		SelectAttributePairStats(GetInternalAttributesBivariateStats(), &oaSelectedAttributePairStats);

	// On verifie qu'au moins une pair est informative
	if (bOk)
		bOk = oaSelectedAttributePairStats.GetSize() > 0;

	// Recherche de la solution optimisable exploitant le plus de paires possibles
	if (bOk)
	{
		// La solution avec une paire est necessaire optimisable
		nLowerPairNumber = 1;
		bIsOptimizable = true;
		if (bTrace)
			cout << "Pairs\tOptimisable\n1\t\ttrue\n";

		// Evaluation de la solution comprenant toutes les paires
		nUpperPairNumber = oaSelectedAttributePairStats.GetSize();
		if (nUpperPairNumber > 1)
		{
			// Construction de la solution avec toutes les paires
			BuildInitialSolutionFromBestPairs(initialDataGrid, &oaSelectedAttributePairStats,
							  nUpperPairNumber, initialDataGridSolution);
			bIsOptimizable = IsInitialSolutionOptimizable(initialDataGrid, initialDataGridSolution);
			if (bTrace)
				cout << nUpperPairNumber << "\t" << initialDataGridSolution->GetObjectLabel() << "\t"
				     << BooleanToString(bIsOptimizable) << "\n";
		}

		// Arret de la recherche si la solution avec toutes les paires est informatives, ou s'il y a moins de deux paires
		if (bIsOptimizable or oaSelectedAttributePairStats.GetSize() <= 2)
		{
			// On doit calculer la solution s'il n'y a qu'une seule paire, ou si la solution a deux paire n'est pas optimisable
			if (nUpperPairNumber == 1 or not bIsOptimizable)
			{
				assert(oaSelectedAttributePairStats.GetSize() <= 2);
				BuildInitialSolutionFromBestPairs(initialDataGrid, &oaSelectedAttributePairStats,
								  nUpperPairNumber, initialDataGridSolution);
				if (bTrace)
					cout << nUpperPairNumber << "\t" << initialDataGridSolution->GetObjectLabel()
					     << "\t" << BooleanToString(bIsOptimizable) << "\n";
			}
		}
		// Sinon, recherche par parcours des nombres de paires possibles
		else
		{
			assert(oaSelectedAttributePairStats.GetSize() > 2);

			// Recherche dichotomique du nombre max de paires permettant d'obtenir une solution initiale
			// de taille compatible avec les contraintes d'optimisation
			nPairNumber = -1;
			while (nLowerPairNumber + 1 < nUpperPairNumber)
			{
				// Modification du prochain nombre de pair a tester
				nPairNumber = (nLowerPairNumber + nUpperPairNumber + 1) / 2;

				// Construction de la solution avec le nombre de paires demandees
				BuildInitialSolutionFromBestPairs(initialDataGrid, &oaSelectedAttributePairStats,
								  nPairNumber, initialDataGridSolution);
				bIsOptimizable = IsInitialSolutionOptimizable(initialDataGrid, initialDataGridSolution);
				if (bTrace)
					cout << nPairNumber << "\t" << initialDataGridSolution->GetObjectLabel() << "\t"
					     << BooleanToString(bIsOptimizable) << "\n";

				// Deplacement des bornes de recherche en fonction
				// de la comparaison avec la borne courante
				if (bIsOptimizable)
					nLowerPairNumber = nPairNumber;
				else
					nUpperPairNumber = nPairNumber;
			}
			assert(nPairNumber == nLowerPairNumber or nPairNumber == nUpperPairNumber);
			assert(nLowerPairNumber <= nUpperPairNumber);
			assert(nUpperPairNumber <= nLowerPairNumber + 1);

			// Si la derniere solution calculee n'est pas optimisable, ou il faut recalculer la solution avec une paire en moins
			if (not bIsOptimizable)
			{
				assert(nPairNumber == nUpperPairNumber);
				BuildInitialSolutionFromBestPairs(initialDataGrid, &oaSelectedAttributePairStats,
								  nUpperPairNumber - 1, initialDataGridSolution);
				bIsOptimizable = IsInitialSolutionOptimizable(initialDataGrid, initialDataGridSolution);
				assert(bIsOptimizable);
				if (bTrace)
					cout << nUpperPairNumber - 1 << "\t"
					     << initialDataGridSolution->GetObjectLabel() << "\t"
					     << BooleanToString(bIsOptimizable) << "\n";
			}
		}
	}

	// Trace
	if (bTrace)
	{
		if (bOk)
		{
			cout << "Variable pairs\t" << oaSelectedAttributePairStats.GetSize() << "\n";
			cout << "Initial solution\t" << initialDataGridSolution->GetObjectLabel() << "\n";

			// Affichage de la la grille solution
			if (bTraceDataGrid)
				cout << "Initial data grid solution based on bivariate analysis\n"
				     << *initialDataGridSolution << "\n";
		}
		else
			cout << "Variable partitions not computed\n";
	}

	// Nettoyage
	if (not bOk)
		initialDataGridSolution->DeleteAll();
	CleanInternalAttributesBivariateStats();
	ensure(not bOk or initialDataGridSolution->Check());
	ensure(not bOk or initialDataGridSolution->IsVarPartDataGrid());
	ensure(not bOk or initialDataGrid->GetInnerAttributes()->ContainsSubVarParts(
			      initialDataGridSolution->GetInnerAttributes()));
	ensure(not bOk or initialDataGridSolution->GetGridFrequency() == initialDataGrid->GetGridFrequency());
	ensure(not bOk or IsInitialSolutionOptimizable(initialDataGrid, initialDataGridSolution));
	return bOk;
}

void KWDataGridInitialSolutionSearcherIV::BuildInitialSolutionFromBestPairs(
    const KWDataGrid* initialDataGrid, const ObjectArray* oaInformativeAttributePairStats, int nPairNumber,
    KWDataGrid* initialDataGridSolution) const
{
	const boolean bTrace = false;
	const boolean bTraceDataGrid = false;
	const KWDGAttribute* innerAttribute;
	KWAttributeStats* attributeStats;
	const KWAttributePairStats* attributePairStats;
	const KWDataGridStats* pairStats;
	const KWDGSAttributePartition* attributePartition;
	ObjectArray oaAllAttributesPartitions;
	ObjectDictionary odAllAttributesPartitions;
	ObjectArray* oaAttributePartitions;
	ObjectDictionary odInnerAttributePartitions;
	KWDGSAttributeDiscretization* attributeResultDiscretization;
	KWDGSAttributeGrouping* attributeResultGrouping;
	const KWDGSAttributePartition* attributeResultPartition;
	int n;
	int nAttribute;
	KWDataGridManager dataGridManager;

	require(learningSpec != NULL);
	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(oaInformativeAttributePairStats != NULL);
	require(oaInformativeAttributePairStats->GetSize() > 0);
	require(1 <= nPairNumber and nPairNumber <= oaInformativeAttributePairStats->GetSize());
	require(initialDataGridSolution != NULL);

	// Nettoyage de la solution
	initialDataGridSolution->DeleteAll();

	// Parcours des paires a analyser
	for (n = 0; n < nPairNumber; n++)
	{
		attributePairStats = cast(const KWAttributePairStats*, oaInformativeAttributePairStats->GetAt(n));
		pairStats = attributePairStats->GetPreparedDataGridStats();
		assert(pairStats->ComputeInformativeAttributeNumber() > 0);

		// Analyse de chaque attribut des paires
		for (nAttribute = 0; nAttribute < pairStats->GetAttributeNumber(); nAttribute++)
		{
			attributePartition = pairStats->GetAttributeAt(nAttribute);

			// Recherche du tableau des partition pour cet attribut
			oaAttributePartitions = cast(
			    ObjectArray*, odAllAttributesPartitions.Lookup(attributePartition->GetAttributeName()));

			// Creation si necessaire
			if (oaAttributePartitions == NULL)
			{
				oaAttributePartitions = new ObjectArray;
				odAllAttributesPartitions.SetAt(attributePartition->GetAttributeName(),
								oaAttributePartitions);
				oaAllAttributesPartitions.Add(oaAttributePartitions);
			}

			// Memorisation de la partition
			oaAttributePartitions->Add(cast(Object*, attributePartition));
		}
	}
	assert(oaAllAttributesPartitions.GetSize() > 0);

	// Extraction des partitions les plus fines pour chaque attribut, par intersection de ses partitions
	for (nAttribute = 0; nAttribute < oaAllAttributesPartitions.GetSize(); nAttribute++)
	{
		oaAttributePartitions = cast(ObjectArray*, oaAllAttributesPartitions.GetAt(nAttribute));

		// Acces a la premiere partition pour avoir le type de l'attribut
		attributePartition = cast(const KWDGSAttributePartition*, oaAttributePartitions->GetAt(0));

		// Recherche de la stats univariee de l'attribut correspondant
		attributeStats =
		    GetInternalAttributesBivariateStats()->LookupAttributeStats(attributePartition->GetAttributeName());
		assert(attributeStats != NULL);

		// Recherche de l'attribut interne correspondant
		innerAttribute =
		    initialDataGrid->GetInnerAttributes()->LookupInnerAttribute(attributeStats->GetSortName());
		assert(innerAttribute != NULL);

		///////////////////////////////////////////////////////////////////////////////////////////////////////
		// Creation et memorisation d'une partition univariee par attribut interne implique dans une
		// grille bivariee informative
		//
		// Une grille bivariee peut etre informative en tenant compte des valeurs manquantes, mais la grille
		// univariee resultant peut ne contenir qu'une seule partie apres filtrage des valeurs ou bornes manquantes,
		// qui sont absentes des attributs internes.
		// C'est le cas par exemple pour les mots d'un texte, ou l'analyse bivariee peut detecter une correlation
		// de presence simultanee, et la partition univariee peut etre reduite a une seule partie
		// L'attribut interne reste neanmoins informatif, et il faut l'isoler dans un cluster singleton de parties de
		// variable, distinct du cluster regroupant tous les attributs internes non partitionnes.

		// Cas d'un attribut numerique
		if (attributePartition->GetAttributeType() == KWType::Continuous)
		{
			attributeResultDiscretization = new KWDGSAttributeDiscretization;
			odInnerAttributePartitions.SetAt(attributePartition->GetAttributeName(),
							 attributeResultDiscretization);
			ComputeIntersectionDiscretizations(innerAttribute, attributeStats, oaAttributePartitions,
							   attributeResultDiscretization);
		}
		// Cas d'un attribut categoriel
		else
		{
			attributeResultGrouping = new KWDGSAttributeGrouping;
			odInnerAttributePartitions.SetAt(attributePartition->GetAttributeName(),
							 attributeResultGrouping);
			ComputeIntersectionGroupings(innerAttribute, attributeStats, oaAttributePartitions,
						     attributeResultGrouping);
		}
	}

	// Creation d'une grille exploitant la version partitionnee des attributs internes
	dataGridManager.ExportDataGridWithPartitionnedInnerAttributes(initialDataGrid, &odInnerAttributePartitions,
								      initialDataGridSolution);

	// Trace
	if (bTrace)
	{
		cout << "Variable partitions\t" << oaAllAttributesPartitions.GetSize() << "\n";
		for (nAttribute = 0; nAttribute < oaAllAttributesPartitions.GetSize(); nAttribute++)
		{
			// Acces aux info de l'attribut
			oaAttributePartitions = cast(ObjectArray*, oaAllAttributesPartitions.GetAt(nAttribute));
			attributePartition = cast(const KWDGSAttributePartition*, oaAttributePartitions->GetAt(0));
			attributeResultPartition =
			    cast(const KWDGSAttributePartition*,
				 odInnerAttributePartitions.Lookup(attributePartition->GetAttributeName()));

			// Affichage des ces infos
			cout << "  " << attributePartition->GetAttributeName() << "\t";
			cout << KWType::ToString(attributePartition->GetAttributeType()) << "\t";
			cout << oaAttributePartitions->GetSize() << "\n";
			cout << "  " << *attributeResultPartition << "\n";
		}

		// Affichage de la la grille solution
		if (bTraceDataGrid)
			cout << "Initial data grid solution based on bivariate analysis\n"
			     << *initialDataGridSolution << "\n";
	}

	// Nettoyage
	oaAllAttributesPartitions.DeleteAll();
	odInnerAttributePartitions.DeleteAll();
	ensure(initialDataGridSolution->Check());
	ensure(initialDataGridSolution->IsVarPartDataGrid());
	ensure(
	    initialDataGrid->GetInnerAttributes()->ContainsSubVarParts(initialDataGridSolution->GetInnerAttributes()));
	ensure(initialDataGridSolution->GetGridFrequency() == initialDataGrid->GetGridFrequency());
}

boolean
KWDataGridInitialSolutionSearcherIV::ComputeInternalAttributesBivariateStats(const KWDataGrid* initialDataGrid) const
{
	boolean bOk;
	const boolean bTrace = false;
	int nCurrentSeed;
	KWAttributePairsSpec bivariatePairSpec;
	ALString sBivariateReportPath;
	KWAttributePairName* pairName;
	ObjectArray oaFilteredInnerAttributes;
	int nTotalPairNumber;
	int nMaxIndex;
	int nMinIndex;

	require(GetLearningSpec() != NULL);
	require(initialDataGrid != NULL);

	// Parametrage des variables de travail pour  calculer les statistiques bivariees
	bivariateLearningSpec.CopyFrom(GetLearningSpec());
	bivariateLearningSpec.GetPreprocessingSpec()->SetDiscretizerUnsupervisedMethodName("none");
	bivariateLearningSpec.GetPreprocessingSpec()->SetGrouperUnsupervisedMethodName("none");
	bivariatePairSpec.SetClassName(bivariateLearningSpec.GetClass()->GetName());
	bivariateClassStats.SetLearningSpec(&bivariateLearningSpec);

	// Filtrage des attributs internes utilisables pour l'analyse bivariee
	FilterInnerAttributes(initialDataGrid, &oaFilteredInnerAttributes);

	// Tri des attribut par complexite decroissante
	SortInnerAttributesByIncreasingComplexity(initialDataGrid, &oaFilteredInnerAttributes);

	// Nombre total de paires a prendre en compte
	// On ne peut pas prendre en compte toutes les paires dans la cas general en raison de leur nombre
	// qui augmente comme le carre du nombre d'attributs
	// On limite le nombre de paires de facon a ce que leur cout total d'optimisation soit inferieur
	// a celui du coclusering IxV
	// - N: nombre d'instances
	// - K: nombre de variables internes
	// - complexite pour une paire: O(N sqrt(N) log(N))
	// - complexite pour le coclustering IxV: O(KN sqrt(KN) log(KN)
	// On peut donc traiter O(K sqrt(K)) paires, mais on va se limiter ici a O(K log(K)) paires
	// // pour limiter cette partie du temps de calcul
	nTotalPairNumber = (int)(ceil(oaFilteredInnerAttributes.GetSize()) *
				 log(oaFilteredInnerAttributes.GetSize() + 1.0) / log(2.0));

	// Parametrage des paires a analyser avec en priorite les pair dont l'index maximal est le plus faible,
	// pour traiter les paires en priorite si les deux variables sont les plus simples
	// Prise en compte des paires par complexite d'optimisation croissante, jusqu' a ce la complexite cumulee atteigne
	// celle de l'optimisation de la grille IxV tout entiere, selon le ratio de temps de calcul desire
	bivariatePairSpec.GetSpecificAttributePairs()->DeleteAll();
	for (nMaxIndex = 1; nMaxIndex < oaFilteredInnerAttributes.GetSize(); nMaxIndex++)
	{
		for (nMinIndex = 0; nMinIndex < nMaxIndex; nMinIndex++)
		{
			// Ajout de la paire
			pairName = new KWAttributePairName;
			pairName->SetFirstName(
			    cast(KWDGAttribute*, oaFilteredInnerAttributes.GetAt(nMinIndex))->GetAttributeName());
			pairName->SetSecondName(
			    cast(KWDGAttribute*, oaFilteredInnerAttributes.GetAt(nMaxIndex))->GetAttributeName());
			bivariatePairSpec.GetSpecificAttributePairs()->Add(pairName);

			// Trace
			if (bTrace)
			{
				cout << "Pair\t" << bivariatePairSpec.GetSpecificAttributePairs()->GetSize() << "\t";
				cout << pairName->GetFirstName() << "\t";
				cout << pairName->GetSecondName() << "\n";
			}

			// Arret si on a atteint la limite
			if (bivariatePairSpec.GetSpecificAttributePairs()->GetSize() > nTotalPairNumber)
				break;
		}
		// Arret si on a atteint la limite
		if (bivariatePairSpec.GetSpecificAttributePairs()->GetSize() > nTotalPairNumber)
			break;
	}
	bivariateClassStats.SetAttributePairsSpec(&bivariatePairSpec);
	bivariatePairSpec.SetMaxAttributePairNumber(bivariatePairSpec.GetSpecificAttributePairs()->GetSize());

	// On parametre les stats pour ne pas avoir les message principaux sur la preparation des donnees
	// et eviter les warnings deja emis lors de la phase initiale de lecture de la base
	bivariateClassStats.SetMainMessageVerboseMode(false);
	GetLearningSpec()->GetDatabase()->SetVerboseMode(false);

	// Calcul des statistiques sur les paires de variables
	// On restitue la seed pour que l'etat final ne dependent pas de l'execution
	// sequentielle ou parallele de l'analyse bivariee
	nCurrentSeed = GetRandomSeed();
	bOk = bivariateClassStats.ComputeStats();
	SetRandomSeed(nCurrentSeed);
	bivariateClassStats.GetLearningSpec()->GetDatabase()->SetVerboseMode(true);

	// Suppression du parametrage des paires, qui est local a la methode
	bivariateClassStats.SetAttributePairsSpec(NULL);
	bivariatePairSpec.SetMaxAttributePairNumber(0);

	// Nettoyage
	bivariatePairSpec.GetSpecificAttributePairs()->DeleteAll();

	// Trace
	if (bTrace)
	{
		sBivariateReportPath =
		    FileService::BuildFilePathName(FileService::GetTmpDir(), "CoclusteringBivariate.khj");
		cout << GetLearningSpec()->GetClass()->GetName()
		     << " coclustering bivariate report: " << sBivariateReportPath << "\n";
		if (bOk)
			WriteJSONAnalysisReport(&bivariateClassStats, sBivariateReportPath);
		else
			cout << "Bivariate stats not computed\n";
	}
	return bOk;
}

boolean KWDataGridInitialSolutionSearcherIV::IsInitialSolutionOptimizable(const KWDataGrid* initialDataGrid,
									  KWDataGrid* initialDataGridSolution) const
{
	boolean bOk;
	int nTotalFrequency;
	int nMaxPartNumber;
	int nAttribute;
	KWDGAttribute* innerAttribute;
	int nPartitionnedInnerAttributeNumber;
	int nPartitionnedInnerAttributeTotalPartNumber;

	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(initialDataGridSolution != NULL);
	require(initialDataGridSolution->IsVarPartDataGrid());

	// Calcul des contraintes d'optimisation sur la grille initiale
	nTotalFrequency = initialDataGrid->GetGridFrequency();
	nMaxPartNumber = (int)ceil(sqrt(nTotalFrequency));

	// Verification si les contraintes sont respectees
	bOk = true;
	for (nAttribute = 0; nAttribute < initialDataGridSolution->GetAttributeNumber(); nAttribute++)
		bOk = bOk and initialDataGridSolution->GetAttributeAt(nAttribute)->GetPartNumber() <= nMaxPartNumber;

	// On tolere que les contraintes ne soient pas respectees dans le cas ou il n'y a que deux attributs internes partitionnes
	if (not bOk)
	{
		// Collecte de stats de partitionnement des attributs internes
		nPartitionnedInnerAttributeNumber = 0;
		nPartitionnedInnerAttributeTotalPartNumber = 0;
		for (nAttribute = 0;
		     nAttribute < initialDataGridSolution->GetInnerAttributes()->GetInnerAttributeNumber();
		     nAttribute++)
		{
			innerAttribute = initialDataGridSolution->GetInnerAttributes()->GetInnerAttributeAt(nAttribute);
			if (innerAttribute->GetPartNumber() > 1)
			{
				nPartitionnedInnerAttributeNumber++;
				nPartitionnedInnerAttributeTotalPartNumber += innerAttribute->GetPartNumber();
			}
			if (nPartitionnedInnerAttributeNumber > 2)
				break;
		}

		// On accepte qu'une seule paire soit partitionnee, quelque soit le nombre de parties
		if (nPartitionnedInnerAttributeNumber == 2)
		{
			bOk = true;
			assert(initialDataGridSolution->GetAttributeAt(1)->GetPartNumber() ==
			       nPartitionnedInnerAttributeTotalPartNumber);
		}
	}
	return bOk;
}

const KWClassStats* KWDataGridInitialSolutionSearcherIV::GetInternalAttributesBivariateStats() const
{
	require(bivariateClassStats.IsStatsComputed());
	return &bivariateClassStats;
}

void KWDataGridInitialSolutionSearcherIV::CleanInternalAttributesBivariateStats() const
{
	bivariateClassStats.DeleteAll();
}

void KWDataGridInitialSolutionSearcherIV::FilterInnerAttributes(const KWDataGrid* initialDataGrid,
								ObjectArray* oaFilteredInnerAttributes) const
{
	int n;
	int nIdentifierNumber;
	KWDGAttribute* dgAttribute;
	KWDGPart* dgSingletonPart;
	boolean bKeepAttribute;

	require(GetLearningSpec() != NULL);
	require(initialDataGrid != NULL);
	require(oaFilteredInnerAttributes != NULL);
	require(oaFilteredInnerAttributes->GetSize() == 0);

	// Nombre d'identifiants des instances
	nIdentifierNumber = initialDataGrid->GetAttributeAt(0)->GetInitialValueNumber();

	// Parcours des attributs internes
	for (n = 0; n < initialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber(); n++)
	{
		dgAttribute = initialDataGrid->GetInnerAttributes()->GetInnerAttributeAt(n);
		bKeepAttribute = false;

		// On garde l'attribut s'il comporte plus d'une valeur
		if (dgAttribute->GetPartNumber() > 1)
			bKeepAttribute = true;
		// Analyse de l'attribut sinon pour detecter le cas des valeurs mannqauntes
		// En effet, les valeurs manquantes ne font pas parties des obervation de la grille IxV, mais
		// elle sont exploitee pour l'analyse bivariee et peuvent pemettre de detecter des correlation
		else
		{
			assert(dgAttribute->GetPartNumber() == 1);

			// Acces a l'unique partie
			dgSingletonPart = dgAttribute->GetHeadPart();

			// On garde l'attribut si l'effectif de la partie est different du nombre d'individus
			if (dgSingletonPart->GetPartFrequency() != nIdentifierNumber)
				bKeepAttribute = true;
		}

		// On garde l'attribut si possible
		if (bKeepAttribute)
			oaFilteredInnerAttributes->Add(dgAttribute);
	}
}

void KWDataGridInitialSolutionSearcherIV::SortInnerAttributesByIncreasingComplexity(
    const KWDataGrid* initialDataGrid, ObjectArray* oaInnerAttributes) const
{
	const boolean bTrace = false;
	KWDGAttribute* attribute;
	int n;

	require(initialDataGrid != NULL);
	require(oaInnerAttributes != NULL);

	// Parametrage  de la fonction de tri, puis tri
	oaInnerAttributes->SetCompareFunction(CompareAttributeOptimizationComplexity);
	oaInnerAttributes->Sort();

	// Trace
	if (bTrace)
	{
		cout << "Inner variables sorted by decreasing complexity\n";
		cout << "Index\tType\tName\tParts\tValues\tComplexity\n";
		for (n = 0; n < oaInnerAttributes->GetSize(); n++)
		{
			attribute = cast(KWDGAttribute*, oaInnerAttributes->GetAt(n));
			cout << n + 1 << "\t";
			cout << KWType::ToString(attribute->GetAttributeType()) << "\t";
			cout << attribute->GetAttributeName() << "\t";
			cout << attribute->GetPartNumber() << "\t";
			cout << attribute->GetInitialValueNumber() << "\t";
			cout << ComputeAttributeOptimizationComplexity(attribute) << "\n";
		}
	}
}

void KWDataGridInitialSolutionSearcherIV::SelectAttributePairStats(const KWClassStats* classStats,
								   ObjectArray* oaSelectedAttributePairStats) const
{
	const boolean bTrace = false;
	const ObjectArray* oaAttributePairStats;
	KWAttributePairStats* attributePairStats;
	KWDataGridStats* pairStats;
	int n;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(oaSelectedAttributePairStats != NULL);
	require(oaSelectedAttributePairStats->GetSize() == 0);

	// Selection des paires informatives
	oaAttributePairStats = classStats->GetAttributePairStats();
	for (n = 0; n < oaAttributePairStats->GetSize(); n++)
	{
		attributePairStats = cast(KWAttributePairStats*, oaAttributePairStats->GetAt(n));
		pairStats = attributePairStats->GetPreparedDataGridStats();

		// Analyse de chaque attribut des paires non nulles
		if (pairStats != NULL)
		{
			assert(pairStats->ComputeInformativeAttributeNumber() > 0);
			oaSelectedAttributePairStats->Add(attributePairStats);
		}
	}

	// Tri par Level decroissante
	oaSelectedAttributePairStats->SetCompareFunction(KWLearningReportCompareSortValue);
	oaSelectedAttributePairStats->Sort();

	// Trace
	if (bTrace)
	{
		cout << "SelectAttributePairStats\t" << oaSelectedAttributePairStats->GetSize() << "\n";
		for (n = 0; n < oaSelectedAttributePairStats->GetSize(); n++)
		{
			attributePairStats = cast(KWAttributePairStats*, oaSelectedAttributePairStats->GetAt(n));
			cout << attributePairStats->GetAttributeName1() << "\t";
			cout << attributePairStats->GetAttributeName2() << "\t";
			cout << attributePairStats->GetLevel() << "\n";
		}
	}
}

void KWDataGridInitialSolutionSearcherIV::ComputeIntersectionDiscretizations(
    const KWDGAttribute* innerAttribute, const KWAttributeStats* attributeStats,
    const ObjectArray* oaAttributeDiscretizations, KWDGSAttributeDiscretization* resultDiscretization) const
{
	const boolean bTrace = false;
	const KWDGSAttributeDiscretization* attributeDiscretization;
	ContinuousVector cvAllBounds;
	ContinuousVector cvResultBounds;
	ContinuousVector cvResultFilteredBounds;
	Continuous cBound;
	int n;
	int nBound;
	KWDGPart* innerAttributePart;

	require(innerAttribute != NULL);
	require(innerAttribute->GetAttributeType() == KWType::Continuous);
	require(attributeStats != NULL);
	require(attributeStats->GetAttributeType() == KWType::Continuous);
	require(attributeStats->GetAttributeName() == innerAttribute->GetAttributeName());
	require(oaAttributeDiscretizations != NULL);
	require(oaAttributeDiscretizations->GetSize() > 0);
	require(resultDiscretization != NULL);

	// Initialisation de la discretisation
	resultDiscretization->SetAttributeName(attributeStats->GetAttributeName());
	resultDiscretization->SetInitialValueNumber(attributeStats->GetDescriptiveStats()->GetValueNumber());
	resultDiscretization->SetGranularizedValueNumber(attributeStats->GetDescriptiveStats()->GetValueNumber());

	// Collecte de l'ensemble de toutes les bornes pour toutes les discretisations
	for (n = 0; n < oaAttributeDiscretizations->GetSize(); n++)
	{
		attributeDiscretization =
		    cast(const KWDGSAttributeDiscretization*, oaAttributeDiscretizations->GetAt(n));
		assert(attributeDiscretization->GetAttributeName() ==
		       cast(const KWDGSAttributeDiscretization*, oaAttributeDiscretizations->GetAt(0))
			   ->GetAttributeName());

		// Memorisation des bornes, sauf si elles correspondent a la valeur manquante, qui peut exister dans
		// l'analyse bivariee, mais est ignoree pour les attributs internes d'un coclustering instances x variables
		for (nBound = 0; nBound < attributeDiscretization->GetIntervalBoundNumber(); nBound++)
		{
			cBound = attributeDiscretization->GetIntervalBoundAt(nBound);
			if (cBound != KWContinuous::GetMissingValue())
				cvAllBounds.Add(cBound);
		}
	}

	// Tri de toutes les bornes
	cvAllBounds.Sort();

	// On garde les bornes uniques
	for (nBound = 0; nBound < cvAllBounds.GetSize(); nBound++)
	{
		if (nBound == 0 or cvAllBounds.GetAt(nBound) > cvAllBounds.GetAt(nBound - 1))
			cvResultBounds.Add(cvAllBounds.GetAt(nBound));
	}

	// On garde les bornes communes avec celle de l'attribut interne
	// En effet, ce dernier n'exploite que les valeurs sparse, hors Missing et potentiellement
	// hors valeur par defaut de blocs sparse, comme 0 par exemple (cf. GetValueBlockContinuousDefaultValue)
	nBound = 0;
	innerAttributePart = innerAttribute->GetHeadPart();
	while (innerAttributePart != innerAttribute->GetTailPart())
	{
		// Arret si on a traite toutes les bornes des intervalles
		if (nBound >= cvResultBounds.GetSize())
			break;

		// Test si la borne de l'attribut interne est compatible avec une borne d'intervalle
		if (innerAttributePart->GetInterval()->GetLowerBound() >= cvResultBounds.GetAt(nBound))
		{
			// On garde la borne si elle existe dans l'attribut interne
			if (innerAttributePart->GetInterval()->GetLowerBound() == cvResultBounds.GetAt(nBound))
				cvResultFilteredBounds.Add(cvResultBounds.GetAt(nBound));

			// On passe a la borne suivante
			nBound++;
		}

		// Partie suivante
		innerAttribute->GetNextPart(innerAttributePart);
	}

	// Memorisation des bones des intervales
	resultDiscretization->SetPartNumber(cvResultFilteredBounds.GetSize() + 1);
	for (n = 0; n < cvResultFilteredBounds.GetSize(); n++)
		resultDiscretization->SetIntervalBoundAt(n, cvResultFilteredBounds.GetAt(n));

	// Trace
	if (bTrace)
	{
		attributeDiscretization =
		    cast(const KWDGSAttributeDiscretization*, oaAttributeDiscretizations->GetAt(0));
		cout << "ComputeIntersectionDiscretizations " << attributeDiscretization->GetAttributeName() << "\n";
		cout << *resultDiscretization << "\n";
	}

	ensure(resultDiscretization->GetAttributeName() == attributeStats->GetAttributeName());
	ensure(resultDiscretization->Check());
}

void KWDataGridInitialSolutionSearcherIV::ComputeIntersectionGroupings(const KWDGAttribute* innerAttribute,
								       const KWAttributeStats* attributeStats,
								       const ObjectArray* oaAttributeGroupings,
								       KWDGSAttributeGrouping* resultGrouping) const
{
	const boolean bTrace = false;
	const KWDGSAttributeGrouping* attributeGrouping;
	NumericKeyDictionary nkdValueSignatures;
	ObjectArray oaValueSignatures;
	KWValueSignature* valueSignature;
	KWValueSignature* defaultValueSignature;
	SymbolVector svResultValues;
	IntVector ivResultGroupFirstValueIndexes;
	int n;
	int nGroup;
	int nValue;
	int nDefaultGroupIndex;
	Symbol sValue;
	KWDGPart* innerAttributeDefaultPart;
	Symbol sInnerAttributeDefaultPartValue;
	KWDGPart* innerAttributePart;

	require(innerAttribute != NULL);
	require(innerAttribute->GetAttributeType() == KWType::Symbol);
	require(attributeStats != NULL);
	require(attributeStats->GetAttributeType() == KWType::Symbol);
	require(attributeStats->GetAttributeName() == innerAttribute->GetAttributeName());
	require(oaAttributeGroupings != NULL);
	require(oaAttributeGroupings->GetSize() > 0);
	require(resultGrouping != NULL);

	// Initialisation du groupement de valeurs
	resultGrouping->SetAttributeName(attributeStats->GetAttributeName());
	resultGrouping->SetInitialValueNumber(attributeStats->GetDescriptiveStats()->GetValueNumber());
	resultGrouping->SetGranularizedValueNumber(attributeStats->GetDescriptiveStats()->GetValueNumber());

	// Premiere passe de collecte de toutes les valeurs pour creer les signatures
	// En effet chaque partition peut concerner des valeurs distinctes, selon la taille du groupe par defaut
	// qui ne reference pas toutes ses valeurs
	for (n = 0; n < oaAttributeGroupings->GetSize(); n++)
	{
		attributeGrouping = cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(n));
		assert(attributeGrouping->GetAttributeName() ==
		       cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(0))->GetAttributeName());

		// Analyse de la partition pour creer puis completer la signature de chaque valeur
		for (nGroup = 0; nGroup < attributeGrouping->GetGroupNumber(); nGroup++)
		{
			for (nValue = attributeGrouping->GetGroupFirstValueIndexAt(nGroup);
			     nValue <= attributeGrouping->GetGroupLastValueIndexAt(nGroup); nValue++)
			{
				sValue = attributeGrouping->GetValueAt(nValue);

				// On ne traite que les valeurs presentes des attributs internes pour le coclustering instances x variables
				if (sValue != Symbol())
				{
					// Creation de la signature si necessaire
					valueSignature =
					    cast(KWValueSignature*, nkdValueSignatures.Lookup(sValue.GetNumericKey()));
					if (valueSignature == NULL)
					{
						valueSignature = new KWValueSignature;
						valueSignature->SetValue(sValue);

						// Enregistrement
						nkdValueSignatures.SetAt(sValue.GetNumericKey(), valueSignature);
						oaValueSignatures.Add(valueSignature);
					}
				}
			}
		}
	}

	// Seconde passe de specification des signatures de toutes les valeurs pour l'ensemble des groupements de valeurs
	// La signature est le vecteur des index de groupes sur l'ensemble des partitions
	for (n = 0; n < oaAttributeGroupings->GetSize(); n++)
	{
		attributeGrouping = cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(n));
		assert(attributeGrouping->GetAttributeName() ==
		       cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(0))->GetAttributeName());

		// Analyse de la partition pour creer puis completer la signature de chaque valeur
		nDefaultGroupIndex = -1;
		for (nGroup = 0; nGroup < attributeGrouping->GetGroupNumber(); nGroup++)
		{
			// Parcours des valeurs decrites dans la partition
			for (nValue = attributeGrouping->GetGroupFirstValueIndexAt(nGroup);
			     nValue <= attributeGrouping->GetGroupLastValueIndexAt(nGroup); nValue++)
			{
				sValue = attributeGrouping->GetValueAt(nValue);

				// Memorisation de l'index du groupe par defaut
				if (sValue == Symbol::GetStarValue())
					nDefaultGroupIndex = nGroup;

				// On ne traite que les valeurs presentes des attributs internes
				if (sValue != Symbol())
				{
					valueSignature =
					    cast(KWValueSignature*, nkdValueSignatures.Lookup(sValue.GetNumericKey()));
					assert(valueSignature != NULL);
					assert(valueSignature->GetSignature()->GetSize() == n);

					// Ajout de l'index de la partie en fin de signature
					valueSignature->GetSignature()->Add(nGroup);
				}
			}
		}
		assert(nDefaultGroupIndex != -1);

		// On parcours maintenant toutes les valeurs pour completer la signature de celle du groupe poubelle
		// qui n'ont pas ete decrites explicitement dans la partition
		for (nValue = 0; nValue < oaValueSignatures.GetSize(); nValue++)
		{
			valueSignature = cast(KWValueSignature*, oaValueSignatures.GetAt(nValue));

			// Mise a jour de la signature avec l'index du groupe poubelle si necessaire
			if (valueSignature->GetSignature()->GetSize() == n)
				valueSignature->GetSignature()->Add(nDefaultGroupIndex);
		}
	}

	// Recherche de la partie par defaut de l'attribut interne, qui contient une valeur en plus de la StarValue
	innerAttributeDefaultPart = innerAttribute->GetTailPart();
	assert(innerAttributeDefaultPart->GetSymbolValueSet()->GetTailValue()->GetSymbolValue() ==
	       Symbol::GetStarValue());
	assert(innerAttributeDefaultPart->GetSymbolValueSet()->GetHeadValue() !=
	       innerAttributeDefaultPart->GetSymbolValueSet()->GetTailValue());

	// Recherche de la valeur associee a la partie par defaut
	sInnerAttributeDefaultPartValue =
	    innerAttributeDefaultPart->GetSymbolValueSet()->GetHeadValue()->GetSymbolValue();
	assert(sInnerAttributeDefaultPartValue != Symbol::GetStarValue());

	// On remplace la signature de la valeur par defaut pour qu'elle soit dans le meme groupe que la
	// valeur avec laquelle elle cohabite, sauf cette valeur ne fait pas partie des specification de groupement
	defaultValueSignature =
	    cast(KWValueSignature*, nkdValueSignatures.Lookup(Symbol::GetStarValue().GetNumericKey()));
	valueSignature =
	    cast(KWValueSignature*, nkdValueSignatures.Lookup(sInnerAttributeDefaultPartValue.GetNumericKey()));
	assert(defaultValueSignature != NULL);
	if (valueSignature != NULL)
		defaultValueSignature->GetSignature()->CopyFrom(valueSignature->GetSignature());

	// Parcours de toutes les valeurs initiales de l'attribut interne pour identifier celles qui ne sont
	// specifiees dans aucun groupe et on les associe au groupe par defaut, via la meme signature
	innerAttributePart = innerAttribute->GetHeadPart();
	while (innerAttributePart != NULL)
	{
		assert(innerAttributePart->GetSymbolValueSet()->GetValueNumber() == 1);

		// Recherche de la valeur de la partie singleton
		sValue = innerAttributePart->GetSymbolValueSet()->GetHeadValue()->GetSymbolValue();

		// Creation de la signature si necessaire, en prenant la meme signature que celle du groupe par defaut
		valueSignature = cast(KWValueSignature*, nkdValueSignatures.Lookup(sValue.GetNumericKey()));
		if (valueSignature == NULL)
		{
			valueSignature = new KWValueSignature;
			valueSignature->SetValue(sValue);
			valueSignature->GetSignature()->CopyFrom(defaultValueSignature->GetSignature());

			// Enregistrement
			nkdValueSignatures.SetAt(sValue.GetNumericKey(), valueSignature);
			oaValueSignatures.Add(valueSignature);
		}

		// Partie suivante
		innerAttribute->GetNextPart(innerAttributePart);
	}

	// Tri des signatures pour identifier les groupes uniques
	oaValueSignatures.SetCompareFunction(KWValueSignatureCompare);
	oaValueSignatures.Sort();

	// Parcours des signatures triee pour identifier les groupes de valeurs de l'intersection des partitions
	for (nValue = 0; nValue < oaValueSignatures.GetSize(); nValue++)
	{
		valueSignature = cast(KWValueSignature*, oaValueSignatures.GetAt(nValue));

		// Memorisation de la valeur
		svResultValues.Add(valueSignature->GetValue());

		// Memorisation de la premiere valeur d'un nouveau groupe si changement de signature
		if (nValue == 0 or
		    valueSignature->CompareSignature(cast(KWValueSignature*, oaValueSignatures.GetAt(nValue - 1))) > 0)
			ivResultGroupFirstValueIndexes.Add(nValue);
	}

	// Memorisation des specification du groupement de valeurs
	resultGrouping->SetKeptValueNumber(svResultValues.GetSize());
	resultGrouping->SetPartNumber(ivResultGroupFirstValueIndexes.GetSize());
	for (n = 0; n < svResultValues.GetSize(); n++)
		resultGrouping->SetValueAt(n, svResultValues.GetAt(n));
	for (n = 0; n < ivResultGroupFirstValueIndexes.GetSize(); n++)
		resultGrouping->SetGroupFirstValueIndexAt(n, ivResultGroupFirstValueIndexes.GetAt(n));

	// Trace
	if (bTrace)
	{
		attributeGrouping = cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(0));
		cout << "ComputeIntersectionGroupings " << attributeGrouping->GetAttributeName() << "\n";
		for (nValue = 0; nValue < oaValueSignatures.GetSize(); nValue++)
		{
			valueSignature = cast(KWValueSignature*, oaValueSignatures.GetAt(nValue));
			cout << "\t" << *valueSignature << "\n";
		}
		cout << "Grouping\n" << *resultGrouping << "\n";
	}

	// Nettoyage
	oaValueSignatures.DeleteAll();
	ensure(resultGrouping->GetAttributeName() == attributeStats->GetAttributeName());
	ensure(resultGrouping->Check());
}

void KWDataGridInitialSolutionSearcherIV::WriteJSONAnalysisReport(KWClassStats* classStats,
								  const ALString& sReportFileName) const
{
	JSONFile fJSON;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(sReportFileName != "");

	// Ouverture du rapport
	fJSON.SetFileName(sReportFileName);
	fJSON.OpenForWrite();

	// Ecriture
	if (fJSON.IsOpened())
	{

		// Outil et version
		fJSON.WriteKeyString("tool", GetLearningApplicationName());
		if (GetLearningModuleName() != "")
			fJSON.WriteKeyString("sub_tool", GetLearningModuleName());
		fJSON.WriteKeyString("version", GetLearningVersion());

		// Description courte
		fJSON.WriteKeyString("shortDescription", "Bivariate analysis for initialization a cocluystereing IxV");

		// Rapport de preparation complet
		classStats->SetWriteOptionStatsNativeOrConstructed(true);
		classStats->WriteJSONKeyReport(&fJSON, "preparationReport");
		classStats->SetWriteOptionStatsNativeOrConstructed(false);

		// Rapport de preparation bivarie
		if (classStats->GetAttributePairStats()->GetSize() > 0)
		{
			classStats->SetWriteOptionStats2D(true);
			classStats->WriteJSONKeyReport(&fJSON, "bivariatePreparationReport");
			classStats->SetWriteOptionStats2D(false);
		}

		// Fermeture du fichier
		fJSON.Close();
	}
}

int KWDataGridInitialSolutionSearcherIV::ComputeAttributeOptimizationComplexity(const KWDGAttribute* attribute)
{
	int nComplexity;
	int nValueNumber;

	require(attribute != NULL);
	require(attribute->IsInnerAttribute());
	require(attribute->GetPartNumber() > 0);
	require(attribute->GetAttributeType() == KWType::Continuous or
		attribute->GetInitialValueNumber() == attribute->GetPartNumber());

	// Nombre de valeurs, incremente pour tenir compte des eventuelle valeurs manquantes, absente des attributs internes
	// d'un coclustering IxV, mais traitee en analyse bivariee et permettant de detecter des correlations meme
	// avec des atrtributs ayant une seule valeur presente
	nValueNumber = attribute->GetPartNumber() + 1;

	// Complexite algorithmique dans le cas numerique
	if (attribute->GetAttributeType() == KWType::Continuous)
		nComplexity = (int)ceil(nValueNumber * log(nValueNumber));
	// Et dans le cas categoriel
	else
		nComplexity = (int)ceil(nValueNumber * sqrt(nValueNumber) * log(nValueNumber));
	return nComplexity;
}

int KWDataGridInitialSolutionSearcherIV::CompareAttributeOptimizationComplexity(const void* elem1, const void* elem2)
{
	int nCompare;
	KWDGAttribute* attribute1;
	KWDGAttribute* attribute2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la grille
	attribute1 = cast(KWDGAttribute*, *(Object**)elem1);
	attribute2 = cast(KWDGAttribute*, *(Object**)elem2);

	// Comparaison sur la valeur de tri
	nCompare =
	    ComputeAttributeOptimizationComplexity(attribute1) - ComputeAttributeOptimizationComplexity(attribute2);

	// En cas d'egalite, on se base sur la comparaison des nom des attributs
	if (nCompare == 0)
		nCompare = attribute1->GetAttributeName().Compare(attribute2->GetAttributeName());
	return nCompare;
}

/////////////////////////////////////////////////////////////////////////////////////
// Classe KWValueSignature

int KWValueSignature::Compare(const KWValueSignature* aSource) const
{
	int nCompare;

	require(aSource != NULL);
	require(ivSignature.GetSize() == aSource->ivSignature.GetSize());

	// Comparaison des signatures
	nCompare = CompareSignature(aSource);

	// Comparaison de la valeur, en prenant la StarValue en dernier
	if (nCompare == 0 and sValue != aSource->sValue)
	{
		if (sValue == Symbol::GetStarValue())
			nCompare = 1;
		else if (aSource->sValue == Symbol::GetStarValue())
			nCompare = -1;
		else
			nCompare = sValue.CompareValue(aSource->sValue);
	}
	return nCompare;
}

int KWValueSignature::CompareSignature(const KWValueSignature* aSource) const
{
	int i;
	int nCompare;

	require(aSource != NULL);

	// Comparaison de la taille des signatures
	nCompare = ivSignature.GetSize() - aSource->ivSignature.GetSize();

	// Comparaison terme a terme des vecteurs de signature
	if (nCompare == 0)
	{
		for (i = 0; i < ivSignature.GetSize(); i++)
		{
			nCompare = ivSignature.GetAt(i) - aSource->ivSignature.GetAt(i);
			if (nCompare != 0)
				break;
		}
	}
	return nCompare;
}

void KWValueSignature::Write(ostream& ost) const
{
	int i;

	ost << sValue << " [";
	for (i = 0; i < ivSignature.GetSize(); i++)
	{
		if (i > 0)
			ost << '.';
		ost << ivSignature.GetAt(i);
	}
	ost << ']';
}

int KWValueSignatureCompare(const void* elem1, const void* elem2)
{

	KWValueSignature* value1;
	KWValueSignature* value2;
	int nCompare;

	// Acces aux signatures
	value1 = (KWValueSignature*)*(Object**)elem1;
	value2 = (KWValueSignature*)*(Object**)elem2;
	assert(value1->GetSignature()->GetSize() == value2->GetSignature()->GetSize());

	// Comparaison
	nCompare = value1->Compare(value2);
	return nCompare;
}
