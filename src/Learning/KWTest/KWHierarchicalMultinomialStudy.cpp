// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWHierarchicalMultinomialStudy.h"

ALString KWHierarchicalMultinomialStudy::sRootDir = ".";

KWHierarchicalMultinomialStudy::KWHierarchicalMultinomialStudy() {}

KWHierarchicalMultinomialStudy::~KWHierarchicalMultinomialStudy() {}

void KWHierarchicalMultinomialStudy::TestNewPrior()
{
	fstream fstResults;
	const int nMaxFrequency = 150000;
	int nFrequency;
	double dSigma;
	DoubleVector dvSigmas;
	int i;
	int nSeed;
	ALString sStudyFileName;
	ALString sTmp;

	// Liste des ecarts types a tester
	// dvSigmas.Add(0.0000000001);
	dvSigmas.Add(0.000001);
	dvSigmas.Add(0.00001);
	dvSigmas.Add(0.0001);
	dvSigmas.Add(0.001);
	dvSigmas.Add(0.01);
	dvSigmas.Add(0.1);
	dvSigmas.Add(1);
	// dvSigmas.Add(10);

	// Boucle d'etude sur les graines
	for (nSeed = 1; nSeed <= 10; nSeed++)
	{
		// Boucle d'etude par sigma
		for (i = 0; i < dvSigmas.GetSize(); i++)
		{
			dSigma = dvSigmas.GetAt(i);

			// Memorisation dans un fichier lie au sigma
			sStudyFileName = sTmp + sRootDir + "/RegressionStudy_" + DoubleToString(dSigma) + "_(" +
					 IntToString(nSeed) + ").txt";
			FileService::OpenOutputFile(sStudyFileName, fstResults);
			StudyWriteHeader(fstResults);

			// Boucle d'etude par effectif
			nFrequency = 2;
			while (nFrequency < nMaxFrequency)
			{
				StudyBivariateSample(nFrequency, dSigma, nSeed, fstResults);
				nFrequency *= 2;
			}
			fstResults.close();
		}
	}
}

void KWHierarchicalMultinomialStudy::TestNewPriorOne()
{
	fstream fstResults;
	int nFrequency;
	double dSigma;
	ALString sStudyFileName;
	ALString sTmp;

	// Parametres
	dSigma = 0.1;
	nFrequency = 20;

	// Memorisation dans un fichier lie au sigma
	sStudyFileName =
	    sTmp + sRootDir + "/RegressionStudy_" + IntToString(nFrequency) + "_" + DoubleToString(dSigma) + "_(1).txt";
	FileService::OpenOutputFile(sStudyFileName, fstResults);
	StudyWriteHeader(fstResults);
	StudyBivariateSample(nFrequency, dSigma, 1, fstResults);
	fstResults.close();
}

void KWHierarchicalMultinomialStudy::TestNewPriorOld()
{
	const int nMaxFrequency = 1100000;
	int nFrequencyStudy = 0;
	int i;
	int nFrequency;
	int nPrior;
	int nI;

	// Comparaison sur une taille donnee
	if (nFrequencyStudy > 1)
	{
		for (nPrior = Standard; nPrior < None; nPrior++)
			ComputeBestMultinomialModel(nPrior, nFrequencyStudy, true);
		return;
	}

	// Comparison des prior sur toutes les tailles de N
	cout << "Study of hierarchical multinomial prior" << endl;
	cout << "Prior\tN\tI\tNi\tLevel\tCost\tI prior\tMultin prior\tLikelihood\n";
	for (nPrior = Standard; nPrior < None; nPrior++)
	{
		for (i = 2; i < 100; i++)
		{
			// Recherche de la frequence suivante
			nFrequency = (int)pow(sqrt(2), i);
			if (nFrequency == (int)pow(sqrt(2), i - 1))
				continue;
			if (nFrequency > nMaxFrequency)
				break;

			// Calcul de la partition optimale
			nI = ComputeBestMultinomialModel(nPrior, nFrequency, false);
			cout << "Prior" << nPrior + 1 << "\t";
			cout << nFrequency << "\t";
			cout << nI << "\t";
			cout << nFrequency / nI << "\t";
			cout << ComputeMultinomialLevel(nPrior, nFrequency, nI) << "\t";
			if (nI == 1)
			{
				cout << ComputeNullCost(nFrequency) << "\t";
				cout << "0\t0\t";
				cout << ComputeNullCost(nFrequency) << endl;
			}
			else
			{
				cout << ComputeMultinomialCost(nPrior, nFrequency, nI) << "\t";
				cout << ComputeIPriorCost(nPrior, nFrequency, nI) << "\t";
				cout << ComputeMultinomialPriorCost(nPrior, nFrequency, nI) << "\t";
				cout << ComputeLikelihoodCost(nPrior, nFrequency, nI) << endl;
			}
		}
	}
}

void KWHierarchicalMultinomialStudy::TestDataset()
{
	ALString sRootPath;
	ALString sClassFileName;
	ALString sClassName;
	ALString sDatasetName;
	ALString sResultDirectory;

	// Parametrage de l'etude
	sRootPath = "C:/Temp/MultinomialPaper/AppliancesEnergyPrediction";
	sClassFileName = FileService::BuildFilePathName(sRootPath, "AppliancesEnergyPrediction.kdic");
	sClassName = "AppliancesEnergyPrediction";
	sDatasetName = FileService::BuildFilePathName(sRootPath, "energydata_complete.txt");
	sResultDirectory = FileService::BuildFilePathName(sRootPath, "StudyHPrior");

	// Etude
	StudyDatasetBivariate(sClassFileName, sClassName, sDatasetName, sResultDirectory);
}

void KWHierarchicalMultinomialStudy::StudyDatasetBivariate(const ALString& sClassFileName, const ALString& sClassName,
							   const ALString& sDatasetName,
							   const ALString& sResultDirectory)
{
	boolean bWriteReport = true;
	KWLearningSpec learningSpec;
	KWSTDatabaseTextFile database;
	KWClassStats classStatsS;
	KWClassStats classStatsH;
	KWAttributePairsSpec attributePairsSpec;
	KWClass* kwcClass;
	KWTupleTableLoader tupleTableLoader;
	KWTupleTable bivariateTupleTable;
	KWAttributePairStatsStudy* attributePairStatsS;
	KWAttributePairStatsStudy* attributePairStatsH;
	int nPairNumber;
	int nPair;
	int nAttribute1;
	int nAttribute2;
	KWAttribute* attribute1;
	KWAttribute* attribute2;
	ALString sTmp;

	// Chargement du fichier dictionnaire
	KWClassDomain::GetCurrentDomain()->ReadFile(sClassFileName);
	KWClassDomain::GetCurrentDomain()->Compile();
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);
	assert(kwcClass != NULL);
	cout << "Dictionary\t" << kwcClass->GetName() << endl;

	// Parametrage de la base
	database.SetClassName(sClassName);
	database.SetDatabaseName(sDatasetName);

	// Parametrage des spec d'apprentissage
	learningSpec.SetDatabase(&database);
	learningSpec.SetClass(kwcClass);
	learningSpec.SetTargetAttributeName("");

	// Parametrage avance des algorithmes
	learningSpec.GetPreprocessingSpec()->GetDataGridOptimizerParameters()->SetOptimizationLevel(6);
	learningSpec.GetPreprocessingSpec()->GetDataGridOptimizerParameters()->SetInternalParameter(
	    "LargeNeighborhoods");

	// Apprentissage de l'attribut cible si necessaire
	nPairNumber = 10000;
	classStatsS.SetLearningSpec(&learningSpec);
	classStatsH.SetLearningSpec(&learningSpec);
	attributePairsSpec.SetClassName(learningSpec.GetClass()->GetName());
	attributePairsSpec.SetMaxAttributePairNumber(nPairNumber);
	classStatsS.SetAttributePairsSpec(&attributePairsSpec);
	classStatsH.SetAttributePairsSpec(&attributePairsSpec);
	if (not learningSpec.IsTargetStatsComputed())
	{
		classStatsS.ComputeStats();
		classStatsH.ComputeStats();
	}

	// Lecture de la base
	database.ReadAll();

	// Parametrage du chargeur de table de tuples avec les objets de la base
	// Ce parametrage est valide pour toutes les lectures a venir
	tupleTableLoader.SetInputClass(kwcClass);
	tupleTableLoader.SetInputDatabaseObjects(database.GetObjects());

	// Parcours des paires d'attributs
	nPair = 0;
	for (nAttribute1 = 0; nAttribute1 < kwcClass->GetUsedAttributeNumber(); nAttribute1++)
	{
		for (nAttribute2 = nAttribute1 + 1; nAttribute2 < kwcClass->GetUsedAttributeNumber(); nAttribute2++)
		{
			// Acces aux attribute, dans l'ordre alphabetique
			attribute1 = kwcClass->GetUsedAttributeAt(nAttribute1);
			attribute2 = kwcClass->GetUsedAttributeAt(nAttribute2);
			if (attribute1->GetName() > attribute2->GetName())
			{
				attribute1 = kwcClass->GetUsedAttributeAt(nAttribute2);
				attribute2 = kwcClass->GetUsedAttributeAt(nAttribute1);
			}

			// On ne traite que les paires d'attributs Continuous
			if (attribute1->GetType() != KWType::Continuous or attribute2->GetType() != KWType::Continuous)
				continue;

			// Arret si trop de paires
			nPair++;
			if (nPair > nPairNumber)
				break;

			// Messafe d'avancement
			Global::AddSimpleMessage(sTmp + "Pair " + IntToString(nPair) + ": " + attribute1->GetName() +
						 " x " + attribute2->GetName());

			// Parametrage des stats sur la paire d'attributs, dans le cas standard
			attributePairStatsS = new KWAttributePairStatsStudy;
			attributePairStatsS->SetLearningSpec(&learningSpec);
			attributePairStatsS->SetAttributeName1(attribute1->GetName());
			attributePairStatsS->SetAttributeName2(attribute2->GetName());
			classStatsS.GetAttributePairStats()->Add(attributePairStatsS);

			// Parametrage des stats sur la paire d'attributs, dans le cas hierarchique
			attributePairStatsH = new KWAttributePairStatsStudy;
			attributePairStatsH->SetLearningSpec(&learningSpec);
			attributePairStatsH->SetAttributeName1(attribute1->GetName());
			attributePairStatsH->SetAttributeName2(attribute2->GetName());
			classStatsH.GetAttributePairStats()->Add(attributePairStatsH);

			// Chargement des tuples
			tupleTableLoader.LoadBivariate(attributePairStatsS->GetAttributeName1(),
						       attributePairStatsS->GetAttributeName2(), &bivariateTupleTable);

			// Calcul avec prior standard
			attributePairStatsS->SetHierarchicalMultinomialPrior(false);
			attributePairStatsS->ComputeStats(&bivariateTupleTable);

			/*
			dStandardNullCost = ComputeNullCost(nSize);
			dStandardLevel = attributePairStats->GetSortValue();
			dStandardCost = (1 - dStandardLevel)*dStandardNullCost;
			dStandardDKL = ComputeDKL(attributePairStats->GetPreparedDataGridStats(),
			classStats.LookupAttributeStats(attributePairStats->GetAttributeName1()),
			classStats.LookupAttributeStats(attributePairStats->GetAttributeName2()),
			dSigma);
			if (bWriteReport)
			attributePairStats->WriteReportFile(FileService::BuildFilePathName(sRootDir, sDatasetName +
			".Density.S.xls"));

			// Memorisation des resultats avec prior standard
			fstResults << nSize << "\t";
			fstResults << dSigma << "\t";
			fstResults << nSeed << "\t";
			fstResults << dStandardNullCost << "\t";
			fstResults << dStandardCost << "\t";
			fstResults << dStandardLevel << "\t";
			fstResults << dStandardDKL << "\t";
			fstResults << attributePairStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber()
			<< "\t"; fstResults <<
			attributePairStats->GetPreparedDataGridStats()->GetAttributeAt(1)->GetPartNumber() << "\t";
			*/

			// Calcul avec prior hierarchique
			attributePairStatsH->SetHierarchicalMultinomialPrior(true);
			attributePairStatsH->ComputeStats(&bivariateTupleTable);
			/*
			dHierarchicalNullCost = ComputeNullCost(nSize) + log(2.0);
			dHierarchicalLevel = attributePairStats->GetSortValue();
			dHierarchicalCost = (1 - dHierarchicalLevel)*dHierarchicalNullCost;
			dHierarchicalDKL = ComputeDKL(attributePairStats->GetPreparedDataGridStats(),
			classStats.LookupAttributeStats(attributePairStats->GetAttributeName1()),
			classStats.LookupAttributeStats(attributePairStats->GetAttributeName2()),
			dSigma);
			if (bWriteReport)
			attributePairStats->WriteReportFile(FileService::BuildFilePathName(sRootDir, sDatasetName +
			".Density.H.xls"));

			// Memorisation des resultats avec prior standard
			fstResults << (dHierarchicalLevel > dStandardLevel) << "\t";
			fstResults << dHierarchicalNullCost << "\t";
			fstResults << dHierarchicalCost << "\t";
			fstResults << dHierarchicalLevel << "\t";
			fstResults << dHierarchicalDKL << "\t";
			fstResults << attributePairStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber()
			<< "\t"; fstResults <<
			attributePairStats->GetPreparedDataGridStats()->GetAttributeAt(1)->GetPartNumber() << endl;
			*/
		}
	}

	// Ecriture du rapport
	if (bWriteReport)
	{
		classStatsS.SetWriteOptionStats1D(false);
		classStatsH.SetWriteOptionStats1D(false);
		classStatsS.WriteReportFile(
		    FileService::BuildFilePathName(sResultDirectory, "Preparation2DReport.S.xls"));
		classStatsH.WriteReportFile(
		    FileService::BuildFilePathName(sResultDirectory, "Preparation2DReport.H.xls"));
	}

	// Nettoyage
	bivariateTupleTable.CleanAll();
	database.DeleteAll();
	KWClassDomain::GetCurrentDomain()->DeleteAllClasses();
}

void KWHierarchicalMultinomialStudy::StudyBivariateSample(int nSize, double dSigma, int nSeed, fstream& fstResults)
{
	boolean bStandardTrain = false;
	boolean bAdvancedTrain = true;
	boolean bWriteReport = true;
	KWLearningProject learningProject;
	KWClass* kwcBivariateClass;
	DoubleVector dvSampleX;
	DoubleVector dvSampleY;
	ALString sDatasetName;
	ALString sDatasetFileName;
	fstream fDataset;
	KWLearningSpec learningSpec;
	KWSTDatabaseTextFile database;
	KWClassStats classStats;
	KWAttributePairsSpec attributePairsSpec;
	KWTupleTableLoader tupleTableLoader;
	KWTupleTable bivariateTupleTable;
	KWAttributePairStatsStudy attributePairStats;
	double dStandardNullCost;
	double dStandardCost;
	double dStandardLevel;
	double dStandardDKL;
	double dHierarchicalNullCost;
	double dHierarchicalCost;
	double dHierarchicalLevel;
	double dHierarchicalDKL;
	ALString sTmp;

	require(nSize >= 0);
	require(dSigma > 0);
	require(nSeed >= 1);

	// Trace dans la console
	cout << "StudyBivariateSample " << nSize << " " << dSigma << " " << nSeed << endl;

	// Initialisation de l'environnement
	learningProject.Begin();

	// Acces au dictionnaire
	kwcBivariateClass = GetBivariateClass();

	// Creation du jeu de donnees
	CreateRegressionSample(nSize, dSigma, nSeed, &dvSampleX, &dvSampleY);

	// Creation d'un fichier de donnees contenant les valeurs
	sDatasetName = sTmp + "GaussianSample_" + IntToString(nSize) + "_" + DoubleToString(dSigma) + "_(" +
		       IntToString(nSeed) + ")";
	sDatasetFileName = FileService::BuildFilePathName(FileService::GetTmpDir(), sDatasetName + ".txt");
	SaveSampleInFile(&dvSampleX, &dvSampleY, sDatasetFileName);
	if (bWriteReport)
		SaveSampleInFile(&dvSampleX, &dvSampleY,
				 FileService::BuildFilePathName(sRootDir, sDatasetName + ".txt"));

	// Parametrage de la base
	database.SetClassName(kwcBivariateClass->GetName());
	database.SetDatabaseName(sDatasetFileName);
	database.SetHeaderLineUsed(false);

	// Parametrage des spec d'apprentissage
	learningSpec.SetDatabase(&database);
	learningSpec.SetClass(kwcBivariateClass);
	learningSpec.SetTargetAttributeName("");

	// Parametrage avance des algorithmes
	learningSpec.GetPreprocessingSpec()->GetDataGridOptimizerParameters()->SetOptimizationLevel(6);
	learningSpec.GetPreprocessingSpec()->GetDataGridOptimizerParameters()->SetInternalParameter(
	    "LargeNeighborhoods");

	// Calcul des statistiques bivariees de facon standard
	if (bStandardTrain)
	{
		classStats.SetLearningSpec(&learningSpec);
		attributePairsSpec.SetClassName(learningSpec.GetClass()->GetName());
		attributePairsSpec.SetMaxAttributePairNumber(1);
		classStats.SetAttributePairsSpec(&attributePairsSpec);
		classStats.ComputeStats();
		if (bWriteReport)
			classStats.WriteReportFile(
			    FileService::BuildFilePathName(sRootDir, sDatasetName + ".Bivariate.xls"));
	}

	// Calcul des statistiques bivariees de facon avancee
	if (bAdvancedTrain)
	{
		// Apprentissage de l'attribut cible si necessaire
		if (not learningSpec.IsTargetStatsComputed())
		{
			classStats.SetLearningSpec(&learningSpec);
			classStats.ComputeStats();
		}
		if (bWriteReport)
			classStats.WriteReportFile(
			    FileService::BuildFilePathName(sRootDir, sDatasetName + ".Univariate.xls"));

		// Lecture de la base
		database.ReadAll();

		// Parametrage du chargeur de table de tuples avec les objets de la base
		// Ce parametrage est valide pour toutes les lectures a venir
		tupleTableLoader.SetInputClass(kwcBivariateClass);
		tupleTableLoader.SetInputDatabaseObjects(database.GetObjects());

		// Parametrage des stats sur la paire d'attributs
		attributePairStats.SetLearningSpec(&learningSpec);
		attributePairStats.SetAttributeName1(kwcBivariateClass->GetUsedAttributeAt(0)->GetName());
		attributePairStats.SetAttributeName2(kwcBivariateClass->GetUsedAttributeAt(1)->GetName());

		// Chargement des tuples
		tupleTableLoader.LoadBivariate(attributePairStats.GetAttributeName1(),
					       attributePairStats.GetAttributeName2(), &bivariateTupleTable);

		// Calcul avec prior standard
		attributePairStats.SetHierarchicalMultinomialPrior(false);
		attributePairStats.ComputeStats(&bivariateTupleTable);
		dStandardNullCost = ComputeNullCost(nSize);
		dStandardLevel = attributePairStats.GetSortValue();
		dStandardCost = (1 - dStandardLevel) * dStandardNullCost;
		dStandardDKL =
		    ComputeDKL(attributePairStats.GetPreparedDataGridStats(),
			       classStats.LookupAttributeStats(attributePairStats.GetAttributeName1()),
			       classStats.LookupAttributeStats(attributePairStats.GetAttributeName2()), dSigma);
		if (bWriteReport)
			attributePairStats.WriteReportFile(
			    FileService::BuildFilePathName(sRootDir, sDatasetName + ".Density.S.xls"));

		// Memorisation des resultats avec prior standard
		fstResults << nSize << "\t";
		fstResults << dSigma << "\t";
		fstResults << nSeed << "\t";
		fstResults << dStandardNullCost << "\t";
		fstResults << dStandardCost << "\t";
		fstResults << dStandardLevel << "\t";
		fstResults << dStandardDKL << "\t";
		fstResults << attributePairStats.GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber() << "\t";
		fstResults << attributePairStats.GetPreparedDataGridStats()->GetAttributeAt(1)->GetPartNumber() << "\t";

		// Calcul avec prior hierarchique
		attributePairStats.SetHierarchicalMultinomialPrior(true);
		attributePairStats.ComputeStats(&bivariateTupleTable);
		dHierarchicalNullCost = ComputeNullCost(nSize) + log(2.0);
		dHierarchicalLevel = attributePairStats.GetSortValue();
		dHierarchicalCost = (1 - dHierarchicalLevel) * dHierarchicalNullCost;
		dHierarchicalDKL =
		    ComputeDKL(attributePairStats.GetPreparedDataGridStats(),
			       classStats.LookupAttributeStats(attributePairStats.GetAttributeName1()),
			       classStats.LookupAttributeStats(attributePairStats.GetAttributeName2()), dSigma);
		if (bWriteReport)
			attributePairStats.WriteReportFile(
			    FileService::BuildFilePathName(sRootDir, sDatasetName + ".Density.H.xls"));

		// Memorisation des resultats avec prior standard
		fstResults << (dHierarchicalLevel > dStandardLevel) << "\t";
		fstResults << dHierarchicalNullCost << "\t";
		fstResults << dHierarchicalCost << "\t";
		fstResults << dHierarchicalLevel << "\t";
		fstResults << dHierarchicalDKL << "\t";
		fstResults << attributePairStats.GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber() << "\t";
		fstResults << attributePairStats.GetPreparedDataGridStats()->GetAttributeAt(1)->GetPartNumber() << endl;

		// Nettoyage
		bivariateTupleTable.CleanAll();
		database.DeleteAll();
	}

	// Nettoyage
	KWClassDomain::GetCurrentDomain()->DeleteClass(kwcBivariateClass->GetName());
	FileService::RemoveFile(sDatasetFileName);
	learningProject.End();
}

void KWHierarchicalMultinomialStudy::StudyWriteHeader(fstream& fstResults)
{
	fstResults << "Size\t";
	fstResults << "Sigma\t";
	fstResults << "Seed\t";
	fstResults << "SNullCost\t";
	fstResults << "SCost\t";
	fstResults << "SLevel\t";
	fstResults << "SDKL\t";
	fstResults << "SParts1\t";
	fstResults << "SParts2\t";
	fstResults << "HBest\t";
	fstResults << "HNullCost\t";
	fstResults << "HCost\t";
	fstResults << "HLevel\t";
	fstResults << "HDKL\t";
	fstResults << "HParts1\t";
	fstResults << "HParts2\n";
}

double KWHierarchicalMultinomialStudy::ComputeDKL(KWDataGridStats* pairDataGridStats, KWAttributeStats* attributeStats1,
						  KWAttributeStats* attributeStats2, double dSigma)
{
	boolean bDisplay = false;
	boolean bDisplayDetails = false;
	const double dPi = 3.14159265358979323846;
	double dEstimatedEntropy;
	double dTrueEntropy;
	double dDKL;
	double dMinX;
	double dMaxX;
	double dMinY;
	double dMaxY;
	KWDGSAttributeDiscretization* discretizationX;
	KWDGSAttributeDiscretization* discretizationY;
	DoubleVector dvBoundsX;
	DoubleVector dvBoundsY;
	int nTotalFrequency;
	int nCellFrequency;
	double dCellProb;
	double dCellDensity;
	double dCellDKL;
	double dX1;
	double dX2;
	double dY1;
	double dY2;
	int i;
	int j;

	require(pairDataGridStats != NULL);
	require(attributeStats1 != NULL);
	require(attributeStats2 != NULL);
	require(dSigma > 0);

	// Cas ou il n'y a qu'une seule cellule
	if (pairDataGridStats->GetAttributeNumber() < 2)
	{
		// Initialisation de la taille des vecteurs de bornes
		dvBoundsX.SetSize(2);
		dvBoundsY.SetSize(2);
	}
	// Cas ou il a une grille 2D
	else
	{
		// Recherche des bornes de la premiere variable
		discretizationX = cast(KWDGSAttributeDiscretization*, pairDataGridStats->GetAttributeAt(0));
		dvBoundsX.SetSize(discretizationX->GetIntervalBoundNumber() + 2);
		for (i = 0; i < discretizationX->GetIntervalBoundNumber(); i++)
			dvBoundsX.SetAt(i + 1, discretizationX->GetIntervalBoundAt(i));

		// Recherche des bornes de la seconde variable
		discretizationY = cast(KWDGSAttributeDiscretization*, pairDataGridStats->GetAttributeAt(1));
		dvBoundsY.SetSize(discretizationY->GetIntervalBoundNumber() + 2);
		for (j = 0; j < discretizationY->GetIntervalBoundNumber(); j++)
			dvBoundsY.SetAt(j + 1, discretizationY->GetIntervalBoundAt(j));
	}

	// Recherches des min et max de chaque attribut
	dMinX = cast(KWDescriptiveContinuousStats*, attributeStats1->GetDescriptiveStats())->GetMin();
	dMaxX = cast(KWDescriptiveContinuousStats*, attributeStats1->GetDescriptiveStats())->GetMax();
	dMinY = cast(KWDescriptiveContinuousStats*, attributeStats2->GetDescriptiveStats())->GetMin();
	dMaxY = cast(KWDescriptiveContinuousStats*, attributeStats2->GetDescriptiveStats())->GetMax();

	// Parametrage des bornes extremes
	dvBoundsX.SetAt(0, dMinX);
	dvBoundsX.SetAt(dvBoundsX.GetSize() - 1, dMaxX);
	dvBoundsY.SetAt(0, dMinY);
	dvBoundsY.SetAt(dvBoundsY.GetSize() - 1, dMaxY);

	// Parcours des cellules de la grille pour calcul la divergence de Kullback-Leibler entre
	//  . ed: densite estimee, constante par cellule
	//  . td: vraie densite, avec X uniforme sur [0; 1] et Y=Gaussian(X, sigma)
	// Cf. article sur les prior hierarchiques multinomiaux
	dDKL = 0;
	dEstimatedEntropy = 0;
	dTrueEntropy = log(dSigma * sqrt(2 * dPi * exp(1)));
	nTotalFrequency = pairDataGridStats->ComputeGridFrequency();
	for (i = 0; i < dvBoundsX.GetSize() - 1; i++)
	{
		for (j = 0; j < dvBoundsY.GetSize() - 1; j++)
		{
			// Effectif de la cellule
			if (pairDataGridStats->GetAttributeNumber() < 2)
				nCellFrequency = nTotalFrequency;
			else
				nCellFrequency = pairDataGridStats->GetBivariateCellFrequencyAt(i, j);

			// Bornes de la cellule
			dX1 = dvBoundsX.GetAt(i);
			dX2 = dvBoundsX.GetAt(i + 1);
			dY1 = dvBoundsY.GetAt(j);
			dY2 = dvBoundsY.GetAt(j + 1);

			// Proba et densite de la cellule
			dCellProb = nCellFrequency * 1.0 / nTotalFrequency;
			dCellDensity = dCellProb / ((dX2 - dX1) * (dY2 - dY1));

			// Mise a jour de l'entropie estimee
			if (dCellProb > 0)
				dEstimatedEntropy -= dCellProb * log(dCellDensity);

			// Calcul de la contribution de la cellule
			dCellDKL = 0;
			if (dCellProb > 0)
			{
				dCellDKL = dCellProb * log(dCellDensity * dSigma * sqrt(2 * dPi));
				dCellDKL += (1 / (24 * dSigma * dSigma)) * dCellDensity *
					    (pow(dY1 - dX2, 4.0) + pow(dY2 - dX1, 4.0) - pow(dY2 - dX2, 4.0) -
					     pow(dY1 - dX1, 4.0));
			}
			dDKL += dCellDKL;

			// Affichage
			if (bDisplayDetails)
			{
				// Entete
				if (i == 0 and j == 0)
					cout << "\ti\tj\tFrequency\tProb\tDensity\tDKL\tX1\tX2\tY1\tY2\n";

				// Cellule copurante
				cout << "\t" << i << "\t" << j << "\t" << nCellFrequency << "\t" << dCellProb << "\t"
				     << dCellDensity << "\t" << dCellDKL << "\t" << dX1 << "\t" << dX2 << "\t" << dY1
				     << "\t" << dY2 << endl;
			}
		}
	}

	// Affichage
	if (bDisplay)
		cout << "DKL\t" << nTotalFrequency << "\t" << dSigma << "\t" << dDKL << "\t" << dEstimatedEntropy
		     << "\t" << dTrueEntropy << "\t" << dvBoundsX.GetSize() - 1 << "\t" << dvBoundsY.GetSize() - 1
		     << endl;
	;
	return dDKL;
}

KWClass* KWHierarchicalMultinomialStudy::GetBivariateClass()
{
	const ALString sClassName = "Bivariate";
	KWClass* kwcClass;
	KWAttribute* attribute;
	ALString sAttributeName1 = "X1";
	ALString sAttributeName2 = "X2";

	// Recherche de la classe de test
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);

	// Creation et enregistrement de la classe si non trouvee
	if (kwcClass == NULL)
	{
		// Initialisation de la classe
		kwcClass = new KWClass;
		kwcClass->SetName(sClassName);

		// Initialisation des attributs
		attribute = new KWAttribute;
		attribute->SetName(sAttributeName1);
		attribute->SetType(KWType::Continuous);
		kwcClass->InsertAttribute(attribute);
		attribute = new KWAttribute;
		attribute->SetName(sAttributeName2);
		attribute->SetType(KWType::Continuous);
		kwcClass->InsertAttribute(attribute);

		// Enregistrement et compilation
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass);
		kwcClass->Compile();
	}

	// Verifications
	check(kwcClass);
	assert(kwcClass->Check());
	return kwcClass;
}

void KWHierarchicalMultinomialStudy::CreateRegressionSample(int nSize, double dSigma, int nSeed,
							    DoubleVector* dvSampleX, DoubleVector* dvSampleY)
{
	int i;
	double dX;
	double dProb;
	double dY;
	int nCurrentSeed;

	require(nSize >= 0);
	require(dSigma > 0);
	require(nSeed >= 1);
	require(dvSampleX != NULL);
	require(dvSampleY != NULL);

	// Dimensionnement des vecteurs
	dvSampleX->SetSize(nSize);
	dvSampleY->SetSize(nSize);

	// Generation des points
	nCurrentSeed = GetRandomSeed();
	SetRandomSeed(nSeed);
	for (i = 0; i < nSize; i++)
	{
		dX = RandomDouble();
		dProb = RandomDouble();
		dY = dX + KWStat::InvNormal(dProb, 0, dSigma);
		dvSampleX->SetAt(i, dX);
		dvSampleY->SetAt(i, dY);
	}
	SetRandomSeed(nCurrentSeed);
	ensure(dvSampleX->GetSize() == dvSampleY->GetSize());
}

void KWHierarchicalMultinomialStudy::SaveSampleInFile(const DoubleVector* dvSampleX, const DoubleVector* dvSampleY,
						      const ALString& sFileName)
{
	fstream fstOutput;
	boolean bOk;
	int i;

	require(dvSampleX != NULL);
	require(dvSampleY != NULL);
	require(dvSampleX->GetSize() == dvSampleY->GetSize());

	// Ouverture du fichier
	bOk = FileService::OpenOutputFile(sFileName, fstOutput);
	if (bOk)
	{
		for (i = 0; i < dvSampleX->GetSize(); i++)
		{
			fstOutput << KWContinuous::ContinuousToString(dvSampleX->GetAt(i)) << "\t";
			fstOutput << KWContinuous::ContinuousToString(dvSampleY->GetAt(i)) << "\n";
		}
		fstOutput.close();
	}
}

double KWHierarchicalMultinomialStudy::ComputeNullCost(int nFrequency)
{
	require(nFrequency > 0);
	return 2 * KWStat::NaturalNumbersUniversalCodeLength(1) + 2 * KWStat::LnFactorial(nFrequency);
}

int KWHierarchicalMultinomialStudy::ComputeBestMultinomialModel(int nPrior, int nFrequency, boolean bVerbose)
{
	int nI;
	int nBestI;
	double dCost;
	double dBestCost;

	require(0 <= nPrior and nPrior < None);
	require(nFrequency > 0);

	// Cout initial
	dBestCost = ComputeNullCost(nFrequency);
	nBestI = 1;
	if (bVerbose)
	{
		cout << "Prior\tN\tI\tNi\tLevel\tCost\tI prior\tMultin prior\tLikelihood\n";
		cout << nPrior + 1 << "\t";
		cout << nFrequency << "\t";
		cout << nFrequency << "\t";
		cout << "1\t0\t";
		cout << ComputeNullCost(nFrequency) << "\t";
		cout << "0\t0\t";
		cout << ComputeNullCost(nFrequency) << endl;
	}

	// Recherche parmi toutes les partitions en J intervalles
	for (nI = 2; nI <= nFrequency; nI++)
	{
		dCost = ComputeMultinomialCost(nPrior, nFrequency, nI);

		// Test si amelioration
		if (dCost < dBestCost - 1e-6)
		{
			nBestI = nI;
			dBestCost = dCost;
		}

		// Affichage
		if (bVerbose)
		{
			cout << nPrior + 1 << "\t";
			cout << nFrequency << "\t";
			cout << nI << "\t";
			cout << nFrequency / nI << "\t";
			cout << ComputeMultinomialLevel(nPrior, nFrequency, nI) << "\t";
			cout << ComputeMultinomialCost(nPrior, nFrequency, nI) << "\t";
			cout << ComputeIPriorCost(nPrior, nFrequency, nI) << "\t";
			cout << ComputeMultinomialPriorCost(nPrior, nFrequency, nI) << "\t";
			cout << ComputeLikelihoodCost(nPrior, nFrequency, nI) << endl;
		}

		// Arret si pas d'amelioration depuis longtemps
		if (nI > nBestI + 100 + nBestI / 10)
			break;
		if (nI > sqrt(INT_MAX - nFrequency))
		{
			nBestI = 1;
			break;
		}
	}
	return nBestI;
}

double KWHierarchicalMultinomialStudy::ComputeMultinomialCost(int nPrior, int nFrequency, int nI)
{
	double dCost;

	require(0 <= nPrior and nPrior < None);
	require(nFrequency > 0);
	require(1 <= nI and nI <= nFrequency);

	dCost = ComputeIPriorCost(nPrior, nFrequency, nI);
	dCost += ComputeMultinomialPriorCost(nPrior, nFrequency, nI);
	dCost += ComputeLikelihoodCost(nPrior, nFrequency, nI);
	return dCost;
}

double KWHierarchicalMultinomialStudy::ComputeIPriorCost(int nPrior, int nFrequency, int nI)
{
	double dCost;

	require(0 <= nPrior and nPrior < None);
	require(nFrequency > 0);
	require(1 <= nI and nI <= nFrequency);

	// Calcul du cout du choix des intervalles
	dCost = 2 * KWStat::NaturalNumbersUniversalCodeLength(nI);
	// Better below (to study)
	// dCost = 2 * KWStat::BoundedNaturalNumbersUniversalCodeLength(nI - 1, nFrequency - 1);
	return dCost;
}

double KWHierarchicalMultinomialStudy::ComputeMultinomialPriorCost(int nPrior, int nFrequency, int nI)
{
	double dCost;
	int nPartFrequency;
	int nLargePartNumber;

	require(0 <= nPrior and nPrior < None);
	require(nFrequency > 0);
	require(1 <= nI and nI <= nFrequency);

	// Calcul des effectifs de chaque partie
	nPartFrequency = nFrequency / nI;

	// Nombre de partie ayant 1 en plus dans l'effectif
	nLargePartNumber = nFrequency - nI * nPartFrequency;
	assert(0 <= nLargePartNumber and nLargePartNumber < nI);

	// Calcul du prior multinomial
	dCost = 0;
	if (nPrior == Standard)
	{
		dCost += KWStat::LnFactorial(nFrequency + nI * nI - 1) - KWStat::LnFactorial(nFrequency) -
			 KWStat::LnFactorial(nI * nI - 1);
	}
	else if (nPrior == Hierarchical1)
	{
		dCost += (nI * nI - 1) * log(2);
		dCost += log(nFrequency + 1);
		dCost += KWStat::LnFactorial(nFrequency + nI - 1) - KWStat::LnFactorial(nFrequency) -
			 KWStat::LnFactorial(nI - 1);
	}
	else if (nPrior == Hierarchical2)
	{
		dCost += log(nI * nI / 2);
		dCost += KWStat::LnFactorial(nI * nI) - KWStat::LnFactorial(nI * nI - nI) - KWStat::LnFactorial(nI);
		dCost += log(nFrequency + 1);
		dCost += KWStat::LnFactorial(nFrequency + nI - 1) - KWStat::LnFactorial(nFrequency) -
			 KWStat::LnFactorial(nI - 1);
	}
	return dCost;
}

double KWHierarchicalMultinomialStudy::ComputeLikelihoodCost(int nPrior, int nFrequency, int nI)
{
	double dCost;
	int nPartFrequency;
	int nLargePartNumber;

	require(0 <= nPrior and nPrior < None);
	require(nFrequency > 0);
	require(1 <= nI and nI <= nFrequency);

	// Calcul des effectifs de chaque partie
	nPartFrequency = nFrequency / nI;

	// Nombre de partie ayant 1 en plus dans l'effectif
	nLargePartNumber = nFrequency - nI * nPartFrequency;
	assert(0 <= nLargePartNumber and nLargePartNumber < nI);

	// Calcul de la vraissemblance
	dCost = 0;
	dCost += KWStat::LnFactorial(nFrequency);
	dCost += (nI - nLargePartNumber) * KWStat::LnFactorial(nPartFrequency) +
		 nLargePartNumber * KWStat::LnFactorial(nPartFrequency + 1);
	return dCost;
}

double KWHierarchicalMultinomialStudy::ComputeMultinomialLevel(int nPrior, int nFrequency, int nI)
{
	double dCost;
	double dNullCost;

	require(0 <= nPrior and nPrior < None);

	if (nI == 1)
		return 0;
	else
	{
		dNullCost = ComputeNullCost(nFrequency);
		dCost = ComputeMultinomialCost(nPrior, nFrequency, nI);
		if (dCost < dNullCost - 1e-6)
			return 1 - dCost / dNullCost;
		else
			return 0;
	}
}

///////////////////////////////////////////////////////
// Classe KWAttributePairStatsStudy

KWAttributePairStatsStudy::KWAttributePairStatsStudy()
{
	bHierarchicalMultinomialPrior = false;
}

KWAttributePairStatsStudy::~KWAttributePairStatsStudy() {}

void KWAttributePairStatsStudy::SetHierarchicalMultinomialPrior(boolean bValue)
{
	bHierarchicalMultinomialPrior = bValue;
}

boolean KWAttributePairStatsStudy::GetHierarchicalMultinomialPrior() const
{
	return bHierarchicalMultinomialPrior;
}

boolean KWAttributePairStatsStudy::ComputeStats(const KWTupleTable* tupleTable)
{
	boolean bPartileOptimization = false;
	KWDataGrid* dataGrid;
	KWDataGrid* optimizedDataGrid;
	KWDataGridCosts* dataGridCosts;
	KWDataGridOptimizer dataGridOptimizer;
	double dGridCost;

	require(Check());
	require(CheckSpecifications());

	// Nettoyage des donnees de preparation
	CleanDataPreparationResults();
	bIsStatsComputed = true;

	// Recopie du parametrage d'optimisation des grilles
	dataGridOptimizer.GetParameters()->CopyFrom(GetPreprocessingSpec()->GetDataGridOptimizerParameters());
	dataGridOptimizer.SetClassStats(GetClassStats());

	// Creation du DataGrid
	dataGrid = CreateDataGrid(tupleTable);

	// Arret si grille non creee
	if (dataGrid == NULL)
	{
		bIsStatsComputed = false;
		return bIsStatsComputed;
	}

	// Creation et initialisation de la structure de couts
	dataGridCosts = CreateDataGridCost();
	dataGridCosts->InitializeDefaultCosts(dataGrid);

	// Parametrage des couts de l'optimiseur de grille
	dataGridOptimizer.SetDataGridCosts(dataGridCosts);

	// Optimisation standard de la grille
	dGridCost = 0;
	if (not bPartileOptimization)
	{
		optimizedDataGrid = new KWDataGrid;
		dGridCost = dataGridOptimizer.OptimizeDataGrid(dataGrid, optimizedDataGrid);
	}
	// Optimisation par parcours des niveaux de partile
	else
	{
		boolean bDisplay = false;
		ObjectDictionary odQuantilesBuilders;
		IntVector ivMaxPartNumbers;
		int nTotalFrequency;
		int nPartileNumber;
		KWDataGrid dgGranularizedDataGrid;
		double dCurrentCost;
		int nBestPartileNumber;
		double dPreviousPartileSize;
		double dPartileSize;

		// Parametrage de quantile builder
		dataGridManager.SetSourceDataGrid(dataGrid);
		dataGridManager.InitializeQuantileBuildersBeforeGranularization(&odQuantilesBuilders,
										&ivMaxPartNumbers);

		// Effectif total
		nTotalFrequency = dataGrid->GetGridFrequency();

		// Parcours des partile possible
		dGridCost = DBL_MAX;
		nBestPartileNumber = -1;
		dPreviousPartileSize = INT_MAX;
		for (nPartileNumber = 1; nPartileNumber < nTotalFrequency; nPartileNumber++)
		{
			// On arrete si on va au dela de la limite des entiers
			if (nPartileNumber * 1.0 * nPartileNumber > INT_MAX / 2)
				break;

			// On saute si pas de changement significatif de taille de partile
			dPartileSize = nTotalFrequency * 1.0 / nPartileNumber;
			if (fabs(dPreviousPartileSize - dPartileSize) < 0.1)
				continue;
			dPreviousPartileSize = dPartileSize;

			// Export d'une grille granularisee
			dgGranularizedDataGrid.DeleteAll();
			ExportGranularizedDataGrid(&dgGranularizedDataGrid, nPartileNumber, &odQuantilesBuilders);

			// Memorisation de son cout
			dCurrentCost = dataGridCosts->ComputeDataGridTotalCost(&dgGranularizedDataGrid);

			// Affichage
			if (bDisplay)
			{
				cout << "Partiles\t" << nPartileNumber << "\t" << dCurrentCost << "\t"
				     << dgGranularizedDataGrid.GetAttributeAt(0)->GetPartNumber() << "\t"
				     << dgGranularizedDataGrid.GetAttributeAt(1)->GetPartNumber() << "\t"
				     << dgGranularizedDataGrid.GetCellNumber() << endl;
				// cout <<	dgGranularizedDataGrid << endl;
			}

			// Memorisation du meilleur niveau de partile
			if (dCurrentCost < dGridCost)
			{
				dGridCost = dCurrentCost;
				nBestPartileNumber = nPartileNumber;
			}
		}

		// On memorise la solution correspondant au meilleurs niveau de partile
		optimizedDataGrid = new KWDataGrid;
		ExportGranularizedDataGrid(optimizedDataGrid, nBestPartileNumber, &odQuantilesBuilders);

		// Nettoyage
		odQuantilesBuilders.DeleteAll();
		dataGridManager.SetSourceDataGrid(NULL);
	}

	// Calcul des donnes de preparation
	ComputeDataPreparationResults(optimizedDataGrid);

	// Memorisation des couts MODL
	SetConstructionCost(dataGridCosts->ComputeDataGridTotalConstructionCost(optimizedDataGrid));
	SetPreparationCost(dataGridCosts->ComputeDataGridTotalPreparationCost(optimizedDataGrid));
	SetDataCost(dataGridCosts->ComputeDataGridTotalDataCost(optimizedDataGrid));

	// Evaluation de la grille par son taux de compression
	// On passe par les couts de grille, qui gerent un modele nul par ensemble d'attributs,
	// ce qui est indispensable notamment dans le cas non supervise
	assert(0 <= dGridCost and dGridCost <= dataGridCosts->GetTotalDefaultCost());
	dPreparedLevel = dataGridCosts->ComputeDataGridCompressionCoefficient(optimizedDataGrid);

	// Nettoyage
	delete dataGrid;
	delete dataGridCosts;
	delete optimizedDataGrid;

	// Reinitialisation des resultats si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
	{
		CleanDataPreparationResults();
		bIsStatsComputed = false;
	}
	return bIsStatsComputed;
}

KWDataGridCosts* KWAttributePairStatsStudy::CreateDataGridCost() const
{
	KWDataGridCosts* dataGridCosts;

	require(Check());
	require(CheckSpecifications());

	// Creation et initialisation de la structure de couts
	dataGridCosts = NULL;
	if (GetHierarchicalMultinomialPrior())
		dataGridCosts = new KWDataGridClusteringCostsBivariateH;
	else
		dataGridCosts = new KWDataGridClusteringCostsBivariate;
	check(dataGridCosts);
	return dataGridCosts;
}

boolean KWAttributePairStatsStudy::CreateAttributeIntervals(const KWTupleTable* tupleTable, KWDGAttribute* dgAttribute)
{
	int nTuple;
	const KWTuple* tuple;
	KWTupleTable attributeTupleTable;
	Continuous cSourceValue;
	Continuous cSourceRef;
	KWDGPart* part;
	Continuous cIntervalBound;
	double dProgression;

	require(Check());
	require(CheckSpecifications());
	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName()) != -1);
	require(tupleTable->GetAttributeTypeAt(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName())) ==
		KWType::Continuous);
	require(dgAttribute != NULL);
	require(dgAttribute->GetPartNumber() == 0);

	// Construction d'une table de tuples univariee dediee a l'attribut
	tupleTable->BuildUnivariateTupleTable(dgAttribute->GetAttributeName(), &attributeTupleTable);
	if (TaskProgression::IsInterruptionRequested())
		return false;

	// Creation d'une premiere partie, avec sa borne inf (contenant la valeur manquante)
	part = dgAttribute->AddPart();
	part->GetInterval()->SetLowerBound(KWDGInterval::GetMinLowerBound());
	part->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());

	// Creation des parties de l'attribut pour chaque tuple
	cSourceRef = KWDGInterval::GetMinLowerBound();

	for (nTuple = 0; nTuple < attributeTupleTable.GetSize(); nTuple++)
	{
		tuple = attributeTupleTable.GetAt(nTuple);

		// Progression
		if (periodicTestDisplay.IsTestAllowed(nTuple))
		{
			// Avancement: au prorata de la base pour l'attribut en cours, en reservant 50 pour la creation
			// des cellules
			dProgression =
			    dgAttribute->GetAttributeIndex() * 50.0 / dgAttribute->GetDataGrid()->GetAttributeNumber();
			dProgression += (nTuple * 50.0 / attributeTupleTable.GetSize()) /
					dgAttribute->GetDataGrid()->GetAttributeNumber();
			TaskProgression::DisplayProgression((int)dProgression);
			if (TaskProgression::IsInterruptionRequested())
				return false;
		}

		// Valeur du tuple
		cSourceValue = tuple->GetContinuousAt(0);

		// Memorisation de la valeur de reference initiale pour le premier tuple
		if (nTuple == 0)
			cSourceRef = cSourceValue;
		// Creation d'un nouvel intervalle sinon
		else
		{
			assert(cSourceValue > cSourceRef);

			// Calcul de la borne sup de l'intervalle courant, comme moyenne de la valeur
			// des deux objets de part et d'autre de l'intervalle
			cIntervalBound = KWContinuous::GetHumanReadableLowerMeanValue(cSourceRef, cSourceValue);

			// Memorisation de la borne sup de l'intervalle en court
			part->GetInterval()->SetUpperBound(cIntervalBound);

			// Creation d'une nouvelle partie avec sa borne inf
			part = dgAttribute->AddPart();
			part->GetInterval()->SetLowerBound(cIntervalBound);
			part->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());

			// Nouvelle valeur de reference
			cSourceRef = cSourceValue;
		}
	}

	// Parametrage du nombre total de valeurs (= nombre d'instances)
	dgAttribute->SetInitialValueNumber(attributeTupleTable.GetTotalFrequency());
	dgAttribute->SetGranularizedValueNumber(attributeTupleTable.GetTotalFrequency());
	assert(dgAttribute->GetPartNumber() == attributeTupleTable.GetSize() or
	       GetPregranularizedNumericalAttributes());
	assert(dgAttribute->GetInitialValueNumber() + 1 >= dgAttribute->GetPartNumber());
	ensure(dgAttribute->Check());

	return true;
}

void KWAttributePairStatsStudy::ExportGranularizedDataGrid(KWDataGrid* targetDataGrid, int nPartileNumber,
							   ObjectDictionary* odQuantilesBuilders) const
{
	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(nPartileNumber > 0);
	require(odQuantilesBuilders->GetCount() == dataGridManager.GetSourceDataGrid()->GetAttributeNumber());

	// Export des attributs
	dataGridManager.ExportAttributes(targetDataGrid);

	// Export des parties granularisees des attributs
	ExportGranularizedParts(targetDataGrid, nPartileNumber, odQuantilesBuilders);

	// Export des cellules
	dataGridManager.ExportCells(targetDataGrid);
	ensure(dataGridManager.CheckDataGrid(targetDataGrid));

	// Memorisation de la granularite
	targetDataGrid->SetGranularity(nPartileNumber);
}

void KWAttributePairStatsStudy::ExportGranularizedParts(KWDataGrid* targetDataGrid, int nPartileNumber,
							ObjectDictionary* odQuantileBuilders) const
{
	ObjectDictionary odSourceAttributes;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWQuantileGroupBuilder* quantileGroupBuilder;
	KWQuantileIntervalBuilder* quantileIntervalBuilder;
	boolean bDisplayResults = false;

	require(Check());
	require(targetDataGrid != NULL and dataGridManager.CheckAttributes(targetDataGrid) and
		dataGridManager.CheckGranularity(targetDataGrid));
	require(0 < nPartileNumber and nPartileNumber <= dataGridManager.GetSourceDataGrid()->GetGridFrequency());
	require(odQuantileBuilders->GetCount() == dataGridManager.GetSourceDataGrid()->GetAttributeNumber());

	// Rangement des attributs sources dans un dictionnaire
	for (nAttribute = 0; nAttribute < dataGridManager.GetSourceDataGrid()->GetAttributeNumber(); nAttribute++)
	{
		sourceAttribute = dataGridManager.GetSourceDataGrid()->GetAttributeAt(nAttribute);
		odSourceAttributes.SetAt(sourceAttribute->GetAttributeName(), sourceAttribute);
	}

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche des attributs cible et source
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);
		sourceAttribute = cast(KWDGAttribute*, odSourceAttributes.Lookup(targetAttribute->GetAttributeName()));

		targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());

		// Cas d'un attribut "cible" (regression, classif avec groupage) : pas de granularisation mais poubelle
		// envisageable
		if (sourceAttribute->GetAttributeTargetFunction())
		{
			targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetTrueValueNumber());

			dataGridManager.ExportPartsForAttribute(targetDataGrid, sourceAttribute->GetAttributeName());
			if (bDisplayResults)
			{
				cout << "Attribut cible " << targetAttribute->GetAttributeName() << endl;
				cout << "Partile number = " << targetAttribute->GetGranularizedValueNumber() << endl;
			}
		}

		// Cas des attributs sources
		else
		{
			// Granularisation dans le cas continu
			if (sourceAttribute->GetAttributeType() == KWType::Continuous)
			{
				quantileIntervalBuilder =
				    cast(KWQuantileIntervalBuilder*,
					 odQuantileBuilders->Lookup(sourceAttribute->GetAttributeName()));

				ExportGranularizedPartsForContinuousAttribute(targetDataGrid, sourceAttribute,
									      targetAttribute, nPartileNumber,
									      quantileIntervalBuilder);
			}
			// Granularisation dans le cas symbolique
			else
			{
				quantileGroupBuilder =
				    cast(KWQuantileGroupBuilder*,
					 odQuantileBuilders->Lookup(sourceAttribute->GetAttributeName()));

				dataGridManager.ExportGranularizedPartsForSymbolAttribute(
				    targetDataGrid, sourceAttribute, targetAttribute, nPartileNumber,
				    quantileGroupBuilder);
			}
		}
	}

	ensure(dataGridManager.CheckParts(targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
}

void KWAttributePairStatsStudy::ExportGranularizedPartsForContinuousAttribute(
    KWDataGrid* targetDataGrid, KWDGAttribute* sourceAttribute, KWDGAttribute* targetAttribute, int nPartileNumber,
    KWQuantileIntervalBuilder* quantileBuilder) const
{
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	ObjectArray oaSourceParts;
	int nValueNumber;
	int nPartileIndex;
	int nActualPartileNumber;
	double dPartileSize;
	boolean bDisplayResults = false;

	require(quantileBuilder != NULL);

	nValueNumber = dataGridManager.GetSourceDataGrid()->GetGridFrequency();

	// Nombre potentiel de partiles associes a cette granularite
	if (nPartileNumber > nValueNumber)
		nPartileNumber = nValueNumber;

	// Effectif theorique par partile
	dPartileSize = (double)nValueNumber / (double)nPartileNumber;

	if (bDisplayResults)
	{
		cout << "Attribut " << targetAttribute->GetAttributeName() << endl;
		cout << "nPartileNumber = " << nPartileNumber << " \t dPartileSize = " << dPartileSize << endl;
	}

	// Export des parties de l'attribut source
	sourceAttribute->ExportParts(&oaSourceParts);

	// Calcul des quantiles
	quantileBuilder->ComputeQuantiles(nPartileNumber);

	// Initialisation du nombre effectif de partiles (peut etre inferieur au nombre theorique du fait de doublons)
	nActualPartileNumber = quantileBuilder->GetIntervalNumber();

	// Creation des partiles
	for (nPartileIndex = 0; nPartileIndex < nActualPartileNumber; nPartileIndex++)
	{
		targetPart = targetAttribute->AddPart();

		// Extraction du premier l'intervalle du partile
		sourcePart =
		    cast(KWDGPart*, oaSourceParts.GetAt(quantileBuilder->GetIntervalFirstValueIndexAt(nPartileIndex)));
		// Memorisation de sa borne inf
		targetPart->GetInterval()->SetLowerBound(sourcePart->GetInterval()->GetLowerBound());

		// Extraction du dernier intervalle du partile
		sourcePart =
		    cast(KWDGPart*, oaSourceParts.GetAt(quantileBuilder->GetIntervalLastValueIndexAt(nPartileIndex)));
		// Memorisation de sa borne sup
		targetPart->GetInterval()->SetUpperBound(sourcePart->GetInterval()->GetUpperBound());
	}

	// Initialisation du nombre de valeurs apres granularisation
	// Cas d'un attribut explicatif dans le cadre d'une analyse supervisee
	// Mise a jour du parametrage du nombre de partiles par le nombre effectif de partiles
	if ((targetDataGrid->GetTargetValueNumber() > 0 or
	     (targetDataGrid->GetTargetAttribute() != NULL and not sourceAttribute->GetAttributeTargetFunction())))
		targetAttribute->SetGranularizedValueNumber(nPartileNumber);
	// Sinon, la granularisation n'est qu'un procede de construction d'une grille initiale
	else
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetTrueValueNumber());
}

////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridClusteringCostsBivariate

KWDataGridClusteringCostsBivariate::KWDataGridClusteringCostsBivariate() {}

KWDataGridClusteringCostsBivariate::~KWDataGridClusteringCostsBivariate() {}

KWDataGridCosts* KWDataGridClusteringCostsBivariate::Clone() const
{
	return new KWDataGridClusteringCostsBivariate;
}

double KWDataGridClusteringCostsBivariate::ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
							       int nInformativeAttributeNumber) const
{
	double dDataGridCost;
	int nGridSize;

	require(dataGrid != NULL);
	require(dLnGridSize >= 0);
	require(nInformativeAttributeNumber >= 0);
	require(nInformativeAttributeNumber <= GetTotalAttributeNumber());
	require(GetTotalAttributeNumber() == 2);

	// Acces a la taille de la grille si elle n'est pas trop grande
	nGridSize = -1;
	if (dLnGridSize < log(INT_MAX / 2.0))
		nGridSize = int(floor(exp(dLnGridSize) + 0.5));

	// Valeur exacte si la grille n'est pas trop grande
	dDataGridCost = 0;
	if (nGridSize > 0)
	{
		// Distribution des individus sur les cellules de la grille (parametres de la multi-nomiale)
		// plus numerateur (N!) du terme de multinome de la distribution effective (terme de multinome)
		dDataGridCost += KWStat::LnFactorial(dataGrid->GetGridFrequency() + nGridSize - 1) -
				 KWStat::LnFactorial(nGridSize - 1);
	}
	// Approximation sinon
	else
	{
		dDataGridCost += dataGrid->GetGridFrequency() * dLnGridSize;
	}
	return dDataGridCost;
}

double KWDataGridClusteringCostsBivariate::ComputeAttributeCost(const KWDGAttribute* attribute,
								int nPartitionSize) const
{
	double dAttributeCost;

	require(attribute != NULL);
	require(attribute->GetTrueValueNumber() > 0);
	require(KWType::IsSimple(attribute->GetAttributeType()));
	require(attribute->GetAttributeType() == KWType::Continuous);

	// Cout de codage du nombre d'intervalles
	dAttributeCost = KWStat::NaturalNumbersUniversalCodeLength(nPartitionSize);
	return dAttributeCost;
}

double KWDataGridClusteringCostsBivariate::ComputePartCost(const KWDGPart* part) const
{
	double dPartCost;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));
	require(part->GetAttribute()->GetAttributeType() == KWType::Continuous);

	// Cout de rangement des instances dans le cas continu
	dPartCost = KWStat::LnFactorial(part->GetPartFrequency());
	return dPartCost;
}

double KWDataGridClusteringCostsBivariate::ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const
{
	double dPartUnionCost;
	int nPartFrequency;

	require(part1 != NULL);
	require(part2 != NULL);
	require(part1->GetAttribute() == part2->GetAttribute());
	require(KWType::IsSimple(part1->GetAttribute()->GetAttributeType()));
	require(part1->GetAttribute()->GetAttributeType() == KWType::Continuous);

	// Cout de rangement des instances dans le cas continu
	nPartFrequency = part1->GetPartFrequency() + part2->GetPartFrequency();
	dPartUnionCost = KWStat::LnFactorial(nPartFrequency);
	return dPartUnionCost;
}

double KWDataGridClusteringCostsBivariate::ComputeCellCost(const KWDGCell* cell) const
{
	double dCellCost;

	require(cell != NULL);

	dCellCost = -KWStat::LnFactorial(cell->GetCellFrequency());
	return dCellCost;
}

double KWDataGridClusteringCostsBivariate::ComputeValueCost(const KWDGValue* value) const
{
	double dValueCost;

	require(value != NULL);
	assert(false);
	dValueCost = -KWStat::LnFactorial(value->GetValueFrequency());
	return dValueCost;
}

const ALString KWDataGridClusteringCostsBivariate::GetClassLabel() const
{
	return "Data grid clustering bivariate costs";
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridClusteringCostsBivariateH

KWDataGridClusteringCostsBivariateH::KWDataGridClusteringCostsBivariateH() {}

KWDataGridClusteringCostsBivariateH::~KWDataGridClusteringCostsBivariateH() {}

KWDataGridCosts* KWDataGridClusteringCostsBivariateH::Clone() const
{
	return new KWDataGridClusteringCostsBivariateH;
}

double KWDataGridClusteringCostsBivariateH::ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
								int nInformativeAttributeNumber) const
{
	double dDataGridCost;
	int nGridSize;
	int nCurrentGridSize;
	IntVector ivCellFrequencies;

	require(dataGrid != NULL);
	require(dLnGridSize >= 0);
	require(nInformativeAttributeNumber >= 0);
	require(nInformativeAttributeNumber <= GetTotalAttributeNumber());
	require(GetTotalAttributeNumber() == 2);

	// Export des cellules de la grille
	ExportCellFrequencies(dataGrid, &ivCellFrequencies);

	// Acces a la taille de la grille si elle n'est pas trop grande
	nGridSize = -1;
	if (dLnGridSize < log(INT_MAX / 2.0))
		nGridSize = int(floor(exp(dLnGridSize) + 0.5));
	assert(nGridSize > 0);

	// Recherche de la taille de grille courante, qui n'est pas necessaire celle de la taille a evaluer
	nCurrentGridSize = 1;
	if (dataGrid->GetAttributeNumber() > 0)
		nCurrentGridSize *= dataGrid->GetAttributeAt(0)->GetPartNumber();
	if (dataGrid->GetAttributeNumber() > 0)
		nCurrentGridSize *= dataGrid->GetAttributeAt(1)->GetPartNumber();

	// Calcul du meilleur prior multinomial a partir du nombre d'instances et de cellules de la grille
	// et du vecteur des effectif des cellules
	dDataGridCost = ComputeBestHierarchicalMultinomialPrior(dataGrid->GetGridFrequency(), nGridSize,
								nCurrentGridSize, &ivCellFrequencies);
	return dDataGridCost;
}

const ALString KWDataGridClusteringCostsBivariateH::GetClassLabel() const
{
	return "Data grid clustering bivariate costs (hierachical)";
}

void KWDataGridClusteringCostsBivariateH::ExportCellFrequencies(const KWDataGrid* dataGrid,
								IntVector* ivCellFrequencies) const
{
	KWDGCell* cell;
	int nIndex;

	require(dataGrid != NULL);
	require(dataGrid->GetGridFrequency() > 0);
	require(dataGrid->GetCellNumber() >= 0);
	require(ivCellFrequencies != NULL);

	// Cas particulier d'une grille sans attribut
	if (dataGrid->GetCellNumber() == 0)
	{
		ivCellFrequencies->SetSize(1);
		ivCellFrequencies->SetAt(0, dataGrid->GetGridFrequency());
	}
	// Alimentation du vecteur d'effectif des cellules
	else
	{
		ivCellFrequencies->SetSize(dataGrid->GetCellNumber());
		nIndex = 0;
		cell = dataGrid->GetHeadCell();
		while (cell != NULL)
		{
			assert(cell->GetCellFrequency() > 0);
			ivCellFrequencies->SetAt(nIndex, cell->GetCellFrequency());
			dataGrid->GetNextCell(cell);
			nIndex++;
		}

		// Tri du vecteur
		ivCellFrequencies->Sort();
	}
}

double KWDataGridClusteringCostsBivariateH::ComputeBestHierarchicalMultinomialPrior(
    int nInstanceNumber, int nTotalCellNumber, int nCurrentTotalCellNumber,
    const IntVector* ivCurrentCellFrequencies) const
{
	boolean bDisplay = false;
	double dCost;
	double dStandardCost;
	double dBestHierarchicalCost;
	double dBestCost;
	double dNewStandardCost;
	double dCurrentStandardCost;
	int nBestInstanceNumberA;
	int nBestCellNumberA;
	int nInstanceNumberA;
	int nCellNumberA;
	int nInstanceNumberB;
	int nCellNumberB;
	int i;

	require(nInstanceNumber > 0);
	require(nTotalCellNumber > 0);
	require(ivCurrentCellFrequencies != NULL);
	require(ivCurrentCellFrequencies->GetSize() > 0);
	require(ivCurrentCellFrequencies->GetSize() <= nCurrentTotalCellNumber);

	// Calcul du cout standard
	// Distribution des individus sur les cellules de la grille (parametres de la multi-nomiale)
	// plus numerateur (N!) du terme de multinome de la distribution effective (terme de multinome)
	dStandardCost = 0;
	dStandardCost += KWStat::LnFactorial(nInstanceNumber + nTotalCellNumber - 1);
	dStandardCost -= KWStat::LnFactorial(nTotalCellNumber - 1);

	// Calcul du meilleurs cout dans le cas hierarchique
	dBestHierarchicalCost = DBL_MAX;
	if (ivCurrentCellFrequencies->GetSize() > 1)
	{
		nBestInstanceNumberA = 0;
		nBestCellNumberA = 0;
		nInstanceNumberA = 0;
		nCellNumberA = 0;
		nInstanceNumberB = nInstanceNumber;
		nCellNumberB = nCurrentTotalCellNumber;
		for (i = ivCurrentCellFrequencies->GetSize() - 1; i >= 0; i--)
		{
			assert(i == 0 or ivCurrentCellFrequencies->GetAt(i) >= ivCurrentCellFrequencies->GetAt(i - 1));

			// On prend en compte la prochaine cellule la plus peuplee
			nInstanceNumberA += ivCurrentCellFrequencies->GetAt(i);
			nCellNumberA++;
			nInstanceNumberB -= ivCurrentCellFrequencies->GetAt(i);
			nCellNumberB--;

			// Calcul du cout correspondant
			dCost = 0;
			dCost += log(nCurrentTotalCellNumber / 2.0);
			dCost += log(nInstanceNumber + 1.0);
			dCost += KWStat::LnFactorial(nCurrentTotalCellNumber);
			dCost -= KWStat::LnFactorial(nCellNumberA);
			dCost -= KWStat::LnFactorial(nCellNumberB);
			if (nInstanceNumberA > 0)
			{
				dCost += KWStat::LnFactorial(nInstanceNumberA + nCellNumberA - 1);
				dCost -= KWStat::LnFactorial(nInstanceNumberA);
				dCost -= KWStat::LnFactorial(nCellNumberA - 1);
			}
			if (nInstanceNumberB > 0)
			{
				dCost += KWStat::LnFactorial(nInstanceNumberB + nCellNumberB - 1);
				dCost -= KWStat::LnFactorial(nInstanceNumberB);
				dCost -= KWStat::LnFactorial(nCellNumberB - 1);
			}

			// Test si amelioration
			if (dCost < dBestHierarchicalCost)
			{
				dBestHierarchicalCost = dCost;
				nBestInstanceNumberA = nInstanceNumberA;
				nBestCellNumberA = nCellNumberA;
			}
		}

		// Affichage
		if (bDisplay)
		{
			cout << "ComputeBestHierarchicalMultinomialPrior\tBestH\t" << nBestInstanceNumberA << "\t"
			     << nBestCellNumberA << "\t" << dBestHierarchicalCost << endl;
		}

		// Correction heurististique si necessaire
		if (nCurrentTotalCellNumber != nTotalCellNumber)
		{
			// Calcul du cout standard complet
			dNewStandardCost = 0;
			dNewStandardCost += KWStat::LnFactorial(nInstanceNumber + nTotalCellNumber - 1);
			dNewStandardCost -= KWStat::LnFactorial(nTotalCellNumber - 1);
			dNewStandardCost -= KWStat::LnFactorial(nInstanceNumber);

			// Calcul du cout standard complet courant
			dCurrentStandardCost = 0;
			dCurrentStandardCost += KWStat::LnFactorial(nInstanceNumber + nCurrentTotalCellNumber - 1);
			dCurrentStandardCost -= KWStat::LnFactorial(nCurrentTotalCellNumber - 1);
			dCurrentStandardCost -= KWStat::LnFactorial(nInstanceNumber);

			// Affichage
			if (bDisplay)
			{
				cout << "ComputeBestHierarchicalMultinomialPrior\tCorrection\t" << nTotalCellNumber
				     << "\t" << nCurrentTotalCellNumber << "\t" << dNewStandardCost << "\t"
				     << dCurrentStandardCost << "\t"
				     << dBestHierarchicalCost * dNewStandardCost / dCurrentStandardCost << "\t"
				     << dBestHierarchicalCost << endl;
			}

			// Correction du cout hierarchique
			dBestHierarchicalCost *= dNewStandardCost / dCurrentStandardCost;
		}

		// Ajout du numerateur (N!) du terme de multinome de la distribution effective (terme de multinome),
		// comme pour le cas standard
		dBestHierarchicalCost += KWStat::LnFactorial(nInstanceNumber);
	}

	// Affichage
	if (bDisplay)
	{
		cout << "ComputeBestHierarchicalMultinomialPrior\tCosts\t" << nInstanceNumber << "\t"
		     << nTotalCellNumber << "\t" << nCurrentTotalCellNumber << "\t"
		     << ivCurrentCellFrequencies->GetSize() << "\t" << dStandardCost + log(2.0) << "\t"
		     << dBestHierarchicalCost + log(2.0) << endl;
	}

	// Calcul du meilleur cout
	dBestCost = log(2.0);
	if (dBestHierarchicalCost < dStandardCost)
		dBestCost += dBestHierarchicalCost;
	else
		dBestCost += dStandardCost;
	return dBestCost;
}
