// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "HanoiTower.h"

HanoiTower::HanoiTower()
{
	nHeight = 0;
	nDiskNumber = 0;
}

HanoiTower::~HanoiTower()
{
	assert(not bTrace);
}

void HanoiTower::Initialize(int nTowerHeight, int nTowerDiskNumber, const ALString& sTowerName)
{
	int i;

	require(0 <= nTowerHeight and nTowerHeight < 100);
	require(0 <= nDiskNumber and nDiskNumber <= nTowerHeight);
	require(sTowerName != "");

	// Initialisation des variables
	sName = sTowerName;
	nHeight = nTowerHeight;
	nDiskNumber = 0;

	// Remplissage de la tour
	ivDisks.SetSize(nHeight);
	ivDisks.Initialize();
	for (i = 0; i < nTowerDiskNumber; i++)
		AddDisk(nTowerDiskNumber - i);
}

const ALString& HanoiTower::GetName() const
{
	return sName;
}

int HanoiTower::GetHeight() const
{
	return nHeight;
}

int HanoiTower::GetDiskNumber() const
{
	return nDiskNumber;
}

int HanoiTower::GetDiskSizeAt(int nDiskHeight) const
{
	require(0 <= nDiskHeight and nDiskHeight < GetHeight());
	return ivDisks.GetAt(nDiskHeight);
}

void HanoiTower::AddDisk(int nDiskSize)
{
	require(Check());
	require(nDiskSize >= 1);
	require(nDiskNumber == 0 or nDiskSize < ivDisks.GetAt(nDiskNumber - 1));
	require(nDiskNumber < nHeight);

	ivDisks.SetAt(nDiskNumber, nDiskSize);
	nDiskNumber++;
	ensure(Check());
}

int HanoiTower::RemoveDisk()
{
	int nRemovedDiskSize;

	require(Check());
	require(nDiskNumber > 0);

	nDiskNumber--;
	nRemovedDiskSize = ivDisks.GetAt(nDiskNumber);
	ivDisks.SetAt(nDiskNumber, 0);
	ensure(Check());
	return nRemovedDiskSize;
}

longint HanoiTower::SolveHanoiProblem(HanoiTower* destinationTower, HanoiTower* intermediateTower,
				      const ALString& sMoveStepsTraceFileName)
{
	longint lStepNumber;

	require(intermediateTower != NULL);
	require(destinationTower != NULL);
	require(intermediateTower->GetName() != GetName());
	require(destinationTower->GetName() != GetName());
	require(destinationTower->GetName() != intermediateTower->GetName());
	require(intermediateTower->GetHeight() >= GetHeight());
	require(destinationTower->GetHeight() >= GetHeight());
	require(intermediateTower->GetDiskNumber() == 0);
	require(destinationTower->GetDiskNumber() == 0);

	// Ouverture si necessaire du fichier de trace
	assert(not bTrace);
	if (sMoveStepsTraceFileName != "")
		bTrace = FileService::OpenOutputFile(sMoveStepsTraceFileName, fstsMoveStepsTraceFileName);

	// Resolution du probleme
	lStepNumber = MoveSubTower(GetDiskNumber(), destinationTower, intermediateTower);

	// Ouverture si necessaire du fichier de trace
	if (bTrace)
		FileService::CloseInputFile(sMoveStepsTraceFileName, fstsMoveStepsTraceFileName);
	bTrace = false;
	return lStepNumber;
}

boolean HanoiTower::Check() const
{
	boolean bOk = true;
	int nDisk;
	ALString sTmp;

	// Verification de l'ordre d'empilement des disques
	for (nDisk = 1; nDisk < GetDiskNumber(); nDisk++)
	{
		if (GetDiskSizeAt(nDisk) >= GetDiskSizeAt(nDisk - 1))
		{
			AddError(sTmp + "Disk size (" + IntToString(GetDiskSizeAt(nDisk)) + ") at position " +
				 IntToString(nDisk + 1) + " larger than preceding disk size (" +
				 IntToString(GetDiskSizeAt(nDisk - 1)) + ")");
			bOk = false;
			break;
		}
	}
	return bOk;
}

void HanoiTower::Write(ostream& ost) const
{
	int nDisk;

	ost << GetClassLabel() << " " << GetObjectLabel() << ": ";
	for (nDisk = 0; nDisk < GetDiskNumber(); nDisk++)
	{
		if (nDisk > 0)
			ost << ".";
		ost << GetDiskSizeAt(nDisk);
	}
}

const ALString HanoiTower::GetClassLabel() const
{
	return "Hanoi tower";
}

const ALString HanoiTower::GetObjectLabel() const
{
	return IntToString(GetDiskNumber());
}

void HanoiTower::Test()
{
	const int nMaxHanoiTowerHeight = 30;
	const int nMaxHanoiTowerHeightWithTrace = 10;
	const int nMinIterNumber = 10;
	const longint lMinTotalStepNumber = 100000000;
	HanoiTower originTower;
	HanoiTower destinationTower;
	HanoiTower intermediateTower;
	ALString sTraceFileName;
	Timer timer;
	int nHeight;
	longint lTotalStepNumber;
	longint lStepNumber;
	int nIter;
	double dPreviousMeanElapedTime;
	double dMeanElapedTime;
	ALString sTmp;

	// Resolution complete jusqu'a une taille raisonable
	for (nHeight = 1; nHeight <= nMaxHanoiTowerHeightWithTrace; nHeight++)
	{
		// Initialisation du probleme
		originTower.Initialize(nMaxHanoiTowerHeightWithTrace, nHeight, "D");
		destinationTower.Initialize(nMaxHanoiTowerHeightWithTrace, 0, "A");
		intermediateTower.Initialize(nMaxHanoiTowerHeightWithTrace, 0, "I");

		// Resolution du probleme avec un fichier de trace
		sTraceFileName = sTmp + "HanoiTower" + IntToString(nHeight) + ".txt";
		originTower.SolveHanoiProblem(&destinationTower, &intermediateTower, sTraceFileName);
		cout << "Tour d'arrivee: " << destinationTower << endl;
		cout << "\tEtapes pour une taille de " << nHeight << " dans le fichier " << sTraceFileName << endl;
	}

	// Boucle sur les hauteurs de tours
	cout << "Test des tours de Hanoi" << endl;
	cout << "Taille\tNombre d'etapes\tRatio\tTemps\n";
	dPreviousMeanElapedTime = 0;
	for (nHeight = 1; nHeight <= nMaxHanoiTowerHeight; nHeight++)
	{
		// Initialisation
		timer.Reset();
		lTotalStepNumber = 0;
		nIter = 0;
		dMeanElapedTime = 0;

		// Boucle sur la resolution, en accumulant assez d'etapes pour fiabiliser l'estimation du temps de
		// calcul
		while (nIter < nMinIterNumber or lTotalStepNumber < lMinTotalStepNumber)
		{
			// Initialisation du probleme
			originTower.Initialize(nMaxHanoiTowerHeight, nHeight, "D");
			destinationTower.Initialize(nMaxHanoiTowerHeight, 0, "A");
			intermediateTower.Initialize(nMaxHanoiTowerHeight, 0, "I");

			// Resolution du probleme
			timer.Start();
			lStepNumber = originTower.SolveHanoiProblem(&destinationTower, &intermediateTower, "");
			timer.Stop();
			lTotalStepNumber += lStepNumber;
			nIter++;
		}

		// Affichage des resultats
		if (nIter > 0)
		{
			// Calcul du temps moyen
			dMeanElapedTime = timer.GetElapsedTime() * 1.0 / nIter;

			// Affichage
			cout << nHeight << "\t";
			cout << lTotalStepNumber / nIter << "\t";
			if (dPreviousMeanElapedTime > 0)
				cout << dMeanElapedTime / dPreviousMeanElapedTime;
			cout << "\t";
			cout << dMeanElapedTime << endl;

			// memorisation du precedent temps moyen
			dPreviousMeanElapedTime = dMeanElapedTime;
		}
	}
}

longint HanoiTower::MoveSubTower(int nSubTowerDiskNumber, HanoiTower* destinationTower, HanoiTower* intermediateTower)
{
	longint lStepNumber;

	require(nSubTowerDiskNumber <= GetDiskNumber());

	// CAs du dernier disque, que l'on deplace simplement
	if (nSubTowerDiskNumber == 1)
	{
		// Deplacement du disque du dessus vers de tout destination
		destinationTower->AddDisk(RemoveDisk());

		// Gestion de la trace
		if (bTrace)
			fstsMoveStepsTraceFileName << GetName() << "->" << destinationTower->GetName() << "\n";
		return 1;
	}
	// Cas de plusieurs disques
	else
	{
		// On deplace la sous tour composee de tous les disques du dessus sauf 1 vers la pile intermediaire
		lStepNumber = MoveSubTower(nSubTowerDiskNumber - 1, intermediateTower, destinationTower);

		// On deplace le disque restant vers la la pile destination
		lStepNumber += MoveSubTower(1, destinationTower, intermediateTower);

		// On deplace la sous tour de la pile intermediare vers la pile destination
		lStepNumber += intermediateTower->MoveSubTower(nSubTowerDiskNumber - 1, destinationTower, this);
		return lStepNumber;
	}
}

fstream HanoiTower::fstsMoveStepsTraceFileName;
boolean HanoiTower::bTrace = false;