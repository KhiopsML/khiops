// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDensityEstimationTest.h"

KWDensityEstimationTest::KWDensityEstimationTest() {}

KWDensityEstimationTest::~KWDensityEstimationTest() {}

void KWDensityEstimationTest::Test()
{
	KWDensityEstimationTest kwdetTest;
	const int nRunNumber = 100;
	const int nMinDatasetSize = 10;
	const int nMaxDatasetSize = 12000;
	int nDatasetSize;
	ContinuousVector cvXValues;
	ContinuousVector cvYValues;
	int nBestCellNumber;
	int nRate;
	double dNoiseRate;
	int nRun;
	DoubleVector dvAxisCellNumbers;

	// Test d'un jeu de donne
	// kwdetTest.TestOneDataset();
	// return;

	// Initialisation du vecteur de stats
	dvAxisCellNumbers.SetSize(nRunNumber);

	// Repetition de l'experience par taille croissante de jeux d'essai
	nDatasetSize = nMinDatasetSize;
	while (nDatasetSize <= nMaxDatasetSize)
	{
		// Jeu d'essai aleatoire
		/*
		for (nRun = 0; nRun < nRunNumber; nRun++)
		{
		kwdetTest.InitializeRandomDatasetValues(nRun, nDatasetSize, &cvXValues, &cvYValues);
		nBestCellNumber = kwdetTest.SearchBestInstanceGridSize(&cvXValues, &cvYValues);
		dvAxisCellNumbers.SetAt(nRun, (double)nBestCellNumber);
		}
		cout << "Random\t" << nDatasetSize << "\t" << KWStat::Mean(&dvAxisCellNumbers) << endl;
		*/

		// Jeu d'essai en dependance lineaire
		/*
		for (nRun = 0; nRun < nRunNumber; nRun++)
		{
		kwdetTest.InitializeLinearDatasetValues(nRun, nDatasetSize, &cvXValues, &cvYValues);
		nBestCellNumber = kwdetTest.SearchBestInstanceGridSize(&cvXValues, &cvYValues);
		dvAxisCellNumbers.SetAt(nRun, (double)nBestCellNumber);
		}
		cout << "Linear\t" << nDatasetSize << "\t" << KWStat::Mean(&dvAxisCellNumbers) << endl;
		*/

		// Jeu d'essai en dependance bi-lineaire
		/*
		for (nRun = 0; nRun < nRunNumber; nRun++)
		{
		kwdetTest.InitializeBiLinearDatasetValues(nRun, nDatasetSize, &cvXValues, &cvYValues);
		nBestCellNumber = kwdetTest.SearchBestInstanceGridSize(&cvXValues, &cvYValues);
		dvAxisCellNumbers.SetAt(nRun, (double)nBestCellNumber);
		}
		cout << "BiLinear\t" << nDatasetSize << "\t" << KWStat::Mean(&dvAxisCellNumbers) << endl;
		*/

		// Jeu d'essai en dependance lineaire bruite
		/*
		for (nRate = 0; nRate <= 10; nRate++)
		{
		dNoiseRate = nRate*0.1;
		for (nRun = 0; nRun < nRunNumber; nRun++)
		{
		kwdetTest.InitializeNoisyLinearDatasetValues(nRun, nDatasetSize, dNoiseRate, &cvXValues, &cvYValues);
		nBestCellNumber = kwdetTest.SearchBestInstanceGridSize(&cvXValues, &cvYValues);
		dvAxisCellNumbers.SetAt(nRun, (double)nBestCellNumber);
		}
		cout << "NoisyLinear\t" << dNoiseRate << "\t" <<
		nDatasetSize << "\t" << KWStat::Mean(&dvAxisCellNumbers) << endl;
		}
		*/

		// Jeu d'essai en dependance lineaire avec bruit gaussian
		for (nRate = 0; nRate <= 10; nRate++)
		{
			dNoiseRate = 0.001 * pow(2.0, nRate);
			for (nRun = 0; nRun < nRunNumber; nRun++)
			{
				kwdetTest.InitializeGaussianLinearDatasetValues(nRun, nDatasetSize, dNoiseRate,
										&cvXValues, &cvYValues);
				nBestCellNumber = kwdetTest.SearchBestInstanceGridSize(&cvXValues, &cvYValues);
				dvAxisCellNumbers.SetAt(nRun, (double)nBestCellNumber);
			}
			cout << "GaussianLinear\t" << dNoiseRate << "\t" << nDatasetSize << "\t"
			     << KWStat::Mean(&dvAxisCellNumbers) << endl;
		}

		// Doublement de la taille des jeux d'essai
		nDatasetSize *= 2;
	}
}

void KWDensityEstimationTest::TestOneDataset()
{
	const int nDatasetSize = 1000;
	const int nAxisCellNumber = 16;
	ContinuousVector cvXValues;
	ContinuousVector cvYValues;
	ObjectArray* oaInstanceGrid;

	// Creation d'un jeu d'essai aleatoire
	InitializeNoisyLinearDatasetValues(1, nDatasetSize, 0.70, &cvXValues, &cvYValues);

	// Affichage de ce jeu d'essai
	WriteDataset(cout, &cvXValues, &cvYValues);
	cout << endl;

	// Calcul des instances tombant dans les cellules d'une grille carree
	oaInstanceGrid =
	    ComputeInstanceGrid(&cvXValues, &cvYValues, nAxisCellNumber, 0, (Continuous)(nDatasetSize - 1));

	// Affichage de cette grille
	WriteInstanceGrid(cout, oaInstanceGrid);
	cout << endl;

	// Affichage de son cout
	cout << "Cost\t" << ComputeInstanceGridCost(oaInstanceGrid) << endl;

	// Nettoyage
	DeleteInstanceGrid(oaInstanceGrid);

	// Recherche de la meilleure taille de grille
	cout << "BestAxisCellNumber\t" << SearchBestInstanceGridSize(&cvXValues, &cvYValues) << endl;
}

int KWDensityEstimationTest::SearchBestInstanceGridSize(ContinuousVector* cvXValues, ContinuousVector* cvYValues)
{
	boolean bDisplay = false;
	int nTotalFrequency;
	int nAxisCellNumber;
	int nPreviousCellFrequency;
	int nCellFrequency;
	ObjectArray* oaInstanceGrid;
	double dCost;
	double dBestCost;
	int nBestAxisCellNumber;

	require(cvXValues != NULL);
	require(cvYValues != NULL);
	require(cvXValues->GetSize() == cvYValues->GetSize());

	// Affichage de l'entete
	if (bDisplay)
		cout << "Size\tCost\n";

	// Parcours de toutes les tailles possibles
	nTotalFrequency = cvXValues->GetSize();
	nPreviousCellFrequency = 0;
	dBestCost = DBL_MAX;
	nBestAxisCellNumber = 0;
	for (nAxisCellNumber = 1; nAxisCellNumber < nTotalFrequency; nAxisCellNumber++)
	{
		// Nombre d'instances moyen par cellule
		nCellFrequency = nTotalFrequency / nAxisCellNumber;

		// On saute l'etape s'il n'y a pas de changement d'effectif moyen
		if (nCellFrequency == nPreviousCellFrequency)
			continue;
		nPreviousCellFrequency = nCellFrequency;

		// Calcul de la grille d'instances
		oaInstanceGrid =
		    ComputeInstanceGrid(cvXValues, cvYValues, nAxisCellNumber, 0, (Continuous)(nTotalFrequency - 1));

		// Evaluation de cette grille
		dCost = ComputeInstanceGridCost(oaInstanceGrid);

		// Affichage
		if (bDisplay)
			cout << nAxisCellNumber << "\t" << dCost << endl;

		// Test si amelioration
		if (dCost < dBestCost)
		{
			dBestCost = dCost;
			nBestAxisCellNumber = nAxisCellNumber;
		}

		// Nettoyage
		DeleteInstanceGrid(oaInstanceGrid);
	}

	return nBestAxisCellNumber;
}

double KWDensityEstimationTest::ComputeInstanceGridCost(ObjectArray* oaInstanceGrid)
{
	boolean bRegression = false;
	double dValue;
	IntVector* ivInstanceVector;
	int nX;
	int nY;
	int nCell;
	int nAxisCellNumber;
	int nXIntervalFrequency;
	int nYIntervalFrequency;
	int nTotalFrequency;

	require(oaInstanceGrid != NULL);

	// Cout en cas de regression
	if (bRegression)
	{
		// Initialisation
		nAxisCellNumber = oaInstanceGrid->GetSize();
		nTotalFrequency = ComputeInstanceGridTotalFrequency(oaInstanceGrid);

		// Cout du choix des nombres d'intervalles en X et Y
		dValue = 2 * log(nAxisCellNumber * 1.0);

		// Cout du choix des intervalles en X
		dValue += KWStat::LnFactorial(nTotalFrequency + nAxisCellNumber - 1) -
			  KWStat::LnFactorial(nTotalFrequency) - KWStat::LnFactorial(nAxisCellNumber - 1);

		// Cout du choix des intervalles en Y pour chaque intervalle en X
		for (nX = 0; nX < nAxisCellNumber; nX++)
		{
			nXIntervalFrequency = ComputeXIntervalTotalFrequency(oaInstanceGrid, nX);

			// Ajout du cout pour l'intervalle
			dValue += KWStat::LnFactorial(nXIntervalFrequency + nAxisCellNumber - 1) -
				  KWStat::LnFactorial(nXIntervalFrequency) - KWStat::LnFactorial(nAxisCellNumber - 1);
		}

		// Pris en compte des exceptions
		for (nX = 0; nX < nAxisCellNumber; nX++)
		{
			nXIntervalFrequency = ComputeXIntervalTotalFrequency(oaInstanceGrid, nX);
			dValue += KWStat::LnFactorial(nXIntervalFrequency);
		}
		for (nY = 0; nY < nAxisCellNumber; nY++)
		{
			nYIntervalFrequency = ComputeYIntervalTotalFrequency(oaInstanceGrid, nY);
			dValue += KWStat::LnFactorial(nYIntervalFrequency);
		}
		for (nX = 0; nX < nAxisCellNumber; nX++)
		{
			for (nY = 0; nY < nAxisCellNumber; nY++)
			{
				ivInstanceVector = cast(IntVector*, oaInstanceGrid->GetAt(nX));
				nCell = ivInstanceVector->GetAt(nY);
				dValue -= KWStat::LnFactorial(nCell);
			}
		}
	}
	// Cout en cas d'estimation de densite
	else
	{
		// Initialisation
		nAxisCellNumber = oaInstanceGrid->GetSize();
		nTotalFrequency = ComputeInstanceGridTotalFrequency(oaInstanceGrid);

		// Cout du choix des nombres d'intervalles en X et Y
		dValue = 2 * log(nAxisCellNumber * 1.0);

		// Cout du choix des cellules en X et Y
		dValue += KWStat::LnFactorial(nTotalFrequency + nAxisCellNumber * nAxisCellNumber - 1) -
			  KWStat::LnFactorial(nTotalFrequency) -
			  KWStat::LnFactorial(nAxisCellNumber * nAxisCellNumber - 1);

		// Pris en compte des exceptions
		dValue += KWStat::LnFactorial(nTotalFrequency);
		for (nX = 0; nX < nAxisCellNumber; nX++)
		{
			nXIntervalFrequency = ComputeXIntervalTotalFrequency(oaInstanceGrid, nX);
			dValue += KWStat::LnFactorial(nXIntervalFrequency);
		}
		for (nY = 0; nY < nAxisCellNumber; nY++)
		{
			nYIntervalFrequency = ComputeYIntervalTotalFrequency(oaInstanceGrid, nY);
			dValue += KWStat::LnFactorial(nYIntervalFrequency);
		}
		for (nX = 0; nX < nAxisCellNumber; nX++)
		{
			for (nY = 0; nY < nAxisCellNumber; nY++)
			{
				ivInstanceVector = cast(IntVector*, oaInstanceGrid->GetAt(nX));
				nCell = ivInstanceVector->GetAt(nY);
				dValue -= KWStat::LnFactorial(nCell);
			}
		}
	}

	// Passage en log base 2
	dValue /= log(2.0);

	return dValue;
}

void KWDensityEstimationTest::InitializeRandomDatasetValues(int nRandomSeed, int nInstanceNumber,
							    ContinuousVector* cvXValues, ContinuousVector* cvYValues)
{
	int nInstance;

	require(nInstanceNumber >= 0);
	require(cvXValues != NULL);
	require(cvYValues != NULL);
	require(cvXValues->GetSize() == cvYValues->GetSize());

	// Initialisation des tailles des vecteurs de donnees
	cvXValues->SetSize(nInstanceNumber);
	cvYValues->SetSize(nInstanceNumber);

	// Initialisation des instances
	SetRandomSeed(nRandomSeed);
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		cvXValues->SetAt(nInstance, (Continuous)RandomInt(nInstanceNumber - 1));
		cvYValues->SetAt(nInstance, (Continuous)RandomInt(nInstanceNumber - 1));
	}
}

void KWDensityEstimationTest::InitializeLinearDatasetValues(int nRandomSeed, int nInstanceNumber,
							    ContinuousVector* cvXValues, ContinuousVector* cvYValues)
{
	int nInstance;

	require(nInstanceNumber >= 0);
	require(cvXValues != NULL);
	require(cvYValues != NULL);
	require(cvXValues->GetSize() == cvYValues->GetSize());

	// Initialisation des tailles des vecteurs de donnees
	cvXValues->SetSize(nInstanceNumber);
	cvYValues->SetSize(nInstanceNumber);

	// Initialisation des instances
	SetRandomSeed(nRandomSeed);
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		cvXValues->SetAt(nInstance, (Continuous)RandomInt(nInstanceNumber - 1));
		cvYValues->SetAt(nInstance, cvXValues->GetAt(nInstance));
	}
}

void KWDensityEstimationTest::InitializeNoisyLinearDatasetValues(int nRandomSeed, int nInstanceNumber,
								 double dNoiseRate, ContinuousVector* cvXValues,
								 ContinuousVector* cvYValues)
{
	int nInstance;

	require(nInstanceNumber >= 0);
	require(cvXValues != NULL);
	require(cvYValues != NULL);
	require(cvXValues->GetSize() == cvYValues->GetSize());
	require(0 <= dNoiseRate and dNoiseRate <= 1);

	// Initialisation des tailles des vecteurs de donnees
	cvXValues->SetSize(nInstanceNumber);
	cvYValues->SetSize(nInstanceNumber);

	// Initialisation des instances
	SetRandomSeed(nRandomSeed);
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		cvXValues->SetAt(nInstance, (Continuous)RandomInt(nInstanceNumber - 1));
		if (RandomDouble() >= dNoiseRate)
			cvYValues->SetAt(nInstance, cvXValues->GetAt(nInstance));
		else
			cvYValues->SetAt(nInstance, (Continuous)RandomInt(nInstanceNumber - 1));
	}
}

void KWDensityEstimationTest::InitializeGaussianLinearDatasetValues(int nRandomSeed, int nInstanceNumber,
								    double dNoiseRate, ContinuousVector* cvXValues,
								    ContinuousVector* cvYValues)
{
	int nInstance;
	int nY;

	require(nInstanceNumber >= 0);
	require(cvXValues != NULL);
	require(cvYValues != NULL);
	require(cvXValues->GetSize() == cvYValues->GetSize());
	require(0 <= dNoiseRate);

	// Initialisation des tailles des vecteurs de donnees
	cvXValues->SetSize(nInstanceNumber);
	cvYValues->SetSize(nInstanceNumber);

	// Initialisation des instances
	SetRandomSeed(nRandomSeed);
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		nY = -1;
		while (nY < 0 or nY >= nInstanceNumber)
		{
			cvXValues->SetAt(nInstance, (Continuous)RandomInt(nInstanceNumber - 1));
			nY = (int)KWStat::InvNormal(RandomDouble(), cvXValues->GetAt(nInstance),
						    nInstanceNumber * dNoiseRate);
		}
		cvYValues->SetAt(nInstance, (Continuous)nY);
	}
}

void KWDensityEstimationTest::InitializeBiLinearDatasetValues(int nRandomSeed, int nInstanceNumber,
							      ContinuousVector* cvXValues, ContinuousVector* cvYValues)
{
	int nInstance;

	require(nInstanceNumber >= 0);
	require(cvXValues != NULL);
	require(cvYValues != NULL);
	require(cvXValues->GetSize() == cvYValues->GetSize());

	// Initialisation des tailles des vecteurs de donnees
	cvXValues->SetSize(nInstanceNumber);
	cvYValues->SetSize(nInstanceNumber);

	// Initialisation des instances
	SetRandomSeed(nRandomSeed);
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		cvXValues->SetAt(nInstance, (Continuous)RandomInt(nInstanceNumber - 1));
		if (RandomDouble() < 0.5)
			cvYValues->SetAt(nInstance, cvXValues->GetAt(nInstance));
		else
			cvYValues->SetAt(nInstance, nInstanceNumber - 1 - cvXValues->GetAt(nInstance));
	}
}

void KWDensityEstimationTest::WriteDataset(ostream& ost, ContinuousVector* cvXValues, ContinuousVector* cvYValues)
{
	int nInstance;

	require(cvXValues != NULL);
	require(cvYValues != NULL);
	require(cvXValues->GetSize() == cvYValues->GetSize());

	// Entete
	ost << "X\tY"
	    << "\n";

	// Instances
	for (nInstance = 0; nInstance < cvXValues->GetSize(); nInstance++)
	{
		ost << cvXValues->GetAt(nInstance) << "\t" << cvYValues->GetAt(nInstance) << "\n";
	}
}

ObjectArray* KWDensityEstimationTest::ComputeInstanceGrid(ContinuousVector* cvXValues, ContinuousVector* cvYValues,
							  int nAxisCellNumber, Continuous cMinValue,
							  Continuous cMaxValue)
{
	ObjectArray* oaInstanceGrid;
	IntVector* ivInstanceVector;
	int nX;
	int nY;
	Continuous cX;
	Continuous cY;
	Continuous cIntervalRange;
	int nInstance;

	require(cvXValues != NULL);
	require(cvYValues != NULL);
	require(cvXValues->GetSize() == cvYValues->GetSize());
	require(nAxisCellNumber > 0);
	require(cMinValue < cMaxValue);

	// Creation de la grille vide
	oaInstanceGrid = new ObjectArray;
	oaInstanceGrid->SetSize(nAxisCellNumber);
	for (nX = 0; nX < nAxisCellNumber; nX++)
	{
		ivInstanceVector = new IntVector;
		ivInstanceVector->SetSize(nAxisCellNumber);
		oaInstanceGrid->SetAt(nX, ivInstanceVector);
	}

	// Rangement des instances dans la grille
	cIntervalRange = (cMaxValue - cMinValue) / nAxisCellNumber;
	for (nInstance = 0; nInstance < cvXValues->GetSize(); nInstance++)
	{
		// Valeurs X et Y de l'instance
		cX = cvXValues->GetAt(nInstance);
		cY = cvYValues->GetAt(nInstance);

		// Discretization de la valeur en X sur la grille
		nX = (int)floor((cX - cMinValue) / cIntervalRange);
		if (nX < 0)
			nX = 0;
		if (nX >= nAxisCellNumber)
			nX = nAxisCellNumber - 1;

		// Discretization de la valeur en X sur la grille
		nY = (int)floor((cY - cMinValue) / cIntervalRange);
		if (nY < 0)
			nY = 0;
		if (nY >= nAxisCellNumber)
			nY = nAxisCellNumber - 1;

		// Rangement de l'instance dans la grille
		ivInstanceVector = cast(IntVector*, oaInstanceGrid->GetAt(nX));
		ivInstanceVector->UpgradeAt(nY, 1);
	}
	assert(ComputeInstanceGridTotalFrequency(oaInstanceGrid) == cvXValues->GetSize());

	return oaInstanceGrid;
}

void KWDensityEstimationTest::DeleteInstanceGrid(ObjectArray* oaInstanceGrid)
{
	require(oaInstanceGrid != NULL);

	oaInstanceGrid->DeleteAll();
	delete oaInstanceGrid;
}

void KWDensityEstimationTest::WriteInstanceGrid(ostream& ost, ObjectArray* oaInstanceGrid)
{
	IntVector* ivInstanceVector;
	int nX;
	int nY;

	require(oaInstanceGrid != NULL);

	// Affichage de la taille de la grille
	ost << "Grid\t" << oaInstanceGrid->GetSize() << "\n";

	// Affichage des effectifs par cellule de la grille
	for (nY = 0; nY < oaInstanceGrid->GetSize(); nY++)
	{
		for (nX = 0; nX < oaInstanceGrid->GetSize(); nX++)
		{
			ivInstanceVector = cast(IntVector*, oaInstanceGrid->GetAt(nX));
			if (nX > 0)
				ost << "\t";
			ost << ivInstanceVector->GetAt(nY);
		}
		ost << "\n";
	}
}

int KWDensityEstimationTest::ComputeInstanceGridTotalFrequency(ObjectArray* oaInstanceGrid)
{
	int nTotalFrequency;
	int nX;

	require(oaInstanceGrid != NULL);

	// Calcul de la frequence totale par cumul sur les vecteurs de la grille
	nTotalFrequency = 0;
	for (nX = 0; nX < oaInstanceGrid->GetSize(); nX++)
	{
		nTotalFrequency += ComputeXIntervalTotalFrequency(oaInstanceGrid, nX);
	}
	return nTotalFrequency;
}

int KWDensityEstimationTest::ComputeXIntervalTotalFrequency(ObjectArray* oaInstanceGrid, int nX)
{
	int nTotalFrequency;
	IntVector* ivInstanceVector;
	int nY;

	require(oaInstanceGrid != NULL);
	require(0 <= nX and nX < oaInstanceGrid->GetSize());

	// Calcul de la frequence totale de l'intervalle
	ivInstanceVector = cast(IntVector*, oaInstanceGrid->GetAt(nX));
	nTotalFrequency = 0;
	for (nY = 0; nY < ivInstanceVector->GetSize(); nY++)
	{
		nTotalFrequency += ivInstanceVector->GetAt(nY);
	}
	return nTotalFrequency;
}

int KWDensityEstimationTest::ComputeYIntervalTotalFrequency(ObjectArray* oaInstanceGrid, int nY)
{
	int nTotalFrequency;
	IntVector* ivInstanceVector;
	int nX;

	require(oaInstanceGrid != NULL);
	require(0 <= nY and nY < oaInstanceGrid->GetSize());

	// Calcul de la frequence totale de l'intervalle
	nTotalFrequency = 0;
	for (nX = 0; nX < oaInstanceGrid->GetSize(); nX++)
	{
		ivInstanceVector = cast(IntVector*, oaInstanceGrid->GetAt(nX));
		nTotalFrequency += ivInstanceVector->GetAt(nY);
	}
	return nTotalFrequency;
}

//////////////////////////////////////////////////////////////////////
// Classe KWDensityEstimationStudy

KWDensityEstimationStudy::KWDensityEstimationStudy() {}

KWDensityEstimationStudy::~KWDensityEstimationStudy() {}

void KWDensityEstimationStudy::Test()
{
	KWDensityEstimationStudy study;
	int nJDResult;
	int nCDResult;
	int nI;
	int nJ;

	// Boucle de calcul de seuil d'equiprobabilite des N pour I < J
	cout << "I\tJ\tJD\tCD\n";
	for (nI = 2; nI <= 1024; nI *= 2)
	{
		for (nJ = nI; nJ <= nI * 1024; nJ += nI)
		{
			nJDResult = study.ComputeJointDensityMinFrequency(nI, nJ);
			nCDResult = study.ComputeConditionalDensityMinFrequency(nI, nJ);
			cout << nI << "\t" << nJ << "\t" << nJDResult << "\t" << nCDResult << "\n";
		}
	}
	cout << endl;

	// Boucle de calcul de seuil d'equiprobabilite des N pour I > J
	cout << "I\tJ\tJD\tCD\n";
	for (nJ = 2; nJ <= 1024; nJ *= 2)
	{
		for (nI = nJ; nI <= nJ * 1024; nI += nJ)
		{
			nJDResult = study.ComputeJointDensityMinFrequency(nI, nJ);
			nCDResult = study.ComputeConditionalDensityMinFrequency(nI, nJ);
			cout << nI << "\t" << nJ << "\t" << nJDResult << "\t" << nCDResult << "\n";
		}
	}
	cout << endl;
}

int KWDensityEstimationStudy::ComputeJointDensityMinFrequency(int nI, int nJ)
{
	boolean bDisplay = false;
	const int nNMax = 100000000;
	int nN;
	double dCost;
	int nResult;

	require(0 < nI);
	require(0 < nJ);
	require(nI == nJ * int(nI / nJ) or nJ == nI * int(nJ / nI));

	// Recherche de l'effectif minimum pour obtenir une valeur negative du critere
	nResult = -1;
	for (nN = 1; nN < nNMax; nN++)
	{
		if (nN >= nI and nN >= nJ)
		{
			dCost = JointDensityDeltaCost(nI, nJ, nN);
			if (bDisplay)
				cout << nI << "\t" << nJ << "\t" << nN << "\t" << dCost << "\n";
			if (dCost < 0 and nResult == -1)
			{
				nResult = nN;
				break;
			}
		}
	}
	return nResult;
}

double KWDensityEstimationStudy::JointDensityDeltaCost(int nI, int nJ, int nN)
{
	int nNj;
	int nLastNj;

	require(0 < nN);
	require(0 < nI and nI <= nN);
	require(0 < nJ and nJ <= nN);
	require(nI == nJ * int(nI / nJ) or nJ == nI * int(nJ / nI));

	nNj = nN / nJ;
	nLastNj = nN - nNj * nJ;
	return KWStat::LnFactorial(nN + nI * nJ - 1) - KWStat::LnFactorial(nI * nJ - 1) - 2 * KWStat::LnFactorial(nN) +
	       (nJ - 1) * KWStat::LnFactorial(nNj) + KWStat::LnFactorial(nLastNj);
}

int KWDensityEstimationStudy::ComputeConditionalDensityMinFrequency(int nI, int nJ)
{
	boolean bDisplay = false;
	const int nNMax = 100000000;
	int nN;
	double dCost;
	int nResult;

	require(0 < nI);
	require(0 < nJ);
	require(nI == nJ * int(nI / nJ) or nJ == nI * int(nJ / nI));

	// Recherche de l'effectif minimum pour obtenir une valeur negative du critere
	nResult = -1;
	for (nN = 1; nN < nNMax; nN++)
	{
		if (nN >= nI and nN >= nJ)
		{
			dCost = ConditionalDensityDeltaCost(nI, nJ, nN);
			if (bDisplay)
				cout << nI << "\t" << nJ << "\t" << nN << "\t" << dCost << "\n";
			if (dCost < 0 and nResult == -1)
			{
				nResult = nN;
				break;
			}
		}
	}
	return nResult;
}

double KWDensityEstimationStudy::ConditionalDensityDeltaCost(int nI, int nJ, int nN)
{
	int nNi;
	int nLastNi;
	int nNj;
	int nLastNj;

	require(0 < nN);
	require(0 < nI and nI <= nN);
	require(0 < nJ and nJ <= nN);
	require(nI == nJ * int(nI / nJ) or nJ == nI * int(nJ / nI));

	nNj = nN / nJ;
	nLastNj = nN - nNj * nJ;
	nNi = nN / nI;
	nLastNi = nN - nNi * nI;
	return KWStat::LnFactorial(nN + nI - 1) - KWStat::LnFactorial(nI - 1) - 2 * KWStat::LnFactorial(nN) +
	       (nI - 1) * (KWStat::LnFactorial(nNi + nJ - 1) - KWStat::LnFactorial(nJ - 1) - KWStat::LnFactorial(nNi)) +
	       KWStat::LnFactorial(nLastNi + nJ - 1) - KWStat::LnFactorial(nJ - 1) - KWStat::LnFactorial(nLastNi) +
	       (nJ - 1) * KWStat::LnFactorial(nNj) + KWStat::LnFactorial(nLastNj);
}
