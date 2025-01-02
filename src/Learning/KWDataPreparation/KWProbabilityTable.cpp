// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWProbabilityTable.h"

KWProbabilityTable::KWProbabilityTable()
{
	nTableSourceSize = 0;
	nTableTargetSize = 0;
	bIsTargetDirection = false;
	bIsLogProbValue = false;
}

KWProbabilityTable::~KWProbabilityTable() {}

void KWProbabilityTable::Initialize(int nSourceSize, int nTargetSize, boolean bTargetDirection, boolean bLogProbValue)
{
	require(nSourceSize >= 0);
	require(nTargetSize >= 0);

	// Memorisation des catacteristiques de la table
	nTableSourceSize = nSourceSize;
	nTableTargetSize = nTargetSize;
	bIsTargetDirection = bTargetDirection;
	bIsLogProbValue = bLogProbValue;

	// Creation des valeurs (apres reinitialisation)
	cvTableValues.SetSize(0);
	cvTableValues.SetSize(nTableSourceSize * nTableTargetSize);
}

void KWProbabilityTable::ImportDataGridStats(const KWDataGridStats* dataGridStats, boolean bTargetDirection,
					     boolean bLogProbValue)
{
	int nCell;
	int nCellNumber;
	int nCellFrequency;
	int nTotalFrequency;
	IntVector ivSourceTotalFrequencies;
	IntVector ivTargetTotalFrequencies;
	IntVector ivPartIndexes;
	int nSource;
	int nTarget;
	double dLaplaceEpsilon;
	double dProb;

	require(dataGridStats != NULL);
	require(dataGridStats->Check());

	// Initialisation de la table
	Initialize(dataGridStats->ComputeSourceGridSize(), dataGridStats->ComputeTargetGridSize(), bTargetDirection,
		   bLogProbValue);

	// CH V9 Lot 8 TODO a terme recuperer et stocker nInitialValueNumber et nGranularizedValueNumber du
	// dataGridStats si besoin pour deploiement ?

	// On commence a importer les effectifs des cellules de la grille
	// et calculer les statistiques sources, cibles et globales
	nCellNumber = GetSourceSize() * GetTargetSize();
	ivPartIndexes.SetSize(dataGridStats->GetAttributeNumber());
	ivSourceTotalFrequencies.SetSize(GetSourceSize());
	ivTargetTotalFrequencies.SetSize(GetTargetSize());
	nTotalFrequency = 0;
	for (nCell = 0; nCell < nCellNumber; nCell++)
	{
		// Calcul de l'index des parties de la cellule
		dataGridStats->ComputePartIndexes(nCell, &ivPartIndexes);

		// Calcul des index sources et cibles
		nSource = dataGridStats->ComputeSourceCellIndex(&ivPartIndexes);
		nTarget = dataGridStats->ComputeTargetCellIndex(&ivPartIndexes);
		assert(0 <= nSource and nSource < GetSourceSize());
		assert(0 <= nTarget and nTarget < GetTargetSize());

		// Recherche de l'effectif de la cellule
		nCellFrequency = dataGridStats->GetCellFrequencyAt(&ivPartIndexes);

		// Mise a jour de l'effectif dans la table de probabilites conditionnelle
		assert(GetValueAt(nSource, nTarget) == 0);
		SetValueAt(nSource, nTarget, (Continuous)nCellFrequency);

		// Mise a jour des statistiques
		ivSourceTotalFrequencies.UpgradeAt(nSource, nCellFrequency);
		ivTargetTotalFrequencies.UpgradeAt(nTarget, nCellFrequency);
		nTotalFrequency += nCellFrequency;
	}

	// Calcul des probabilites conditionnelles a partir des effectifs en
	// utilisant l'estimateur de Laplace
	// Le score est la proba d'apparition de la modalite source connaissant
	// la modalite cible: on utilise l'estimateur de Laplace (avec epsilon=1/N) pour
	// les estimations de probabilite conditionnelle
	// On prend l'effectif total plus un pour eviter les effets de bord
	dLaplaceEpsilon = 1.0 / (nTotalFrequency + 1);
	for (nSource = 0; nSource < GetSourceSize(); nSource++)
	{
		for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
		{
			assert(ivSourceTotalFrequencies.GetAt(nSource) >= 0);
			assert(ivTargetTotalFrequencies.GetAt(nTarget) >= 0);

			// Cas de la probabilite conditionnelle de la cible connaissant la source
			if (IsTargetDirection())
				dProb = (GetValueAt(nSource, nTarget) + dLaplaceEpsilon) /
					(ivSourceTotalFrequencies.GetAt(nSource) + GetTargetSize() * dLaplaceEpsilon);
			// Dans le cas de la probabilite conditionnelle de la source connaissant la cible,
			else
				dProb = (GetValueAt(nSource, nTarget) + dLaplaceEpsilon) /
					(ivTargetTotalFrequencies.GetAt(nTarget) + GetSourceSize() * dLaplaceEpsilon);
			assert(0 <= dProb and dProb <= 1);
			assert((0 < dProb and dProb < 1) or GetSourceSize() == 1 or GetTargetSize() == 1);
			assert(0 < dProb);

			// Memorisation de la probabilite
			if (IsProbValue())
				SetValueAt(nSource, nTarget, (Continuous)dProb);
			// Sinon, memorisation de son logarithme
			else
				SetValueAt(nSource, nTarget, (Continuous)log(dProb));
		}
	}
	ensure(Check());
}

void KWProbabilityTable::CopyFrom(const KWProbabilityTable* kwptSource)
{
	require(kwptSource != NULL);

	nTableSourceSize = kwptSource->nTableSourceSize;
	nTableTargetSize = kwptSource->nTableTargetSize;
	bIsTargetDirection = kwptSource->bIsTargetDirection;
	bIsLogProbValue = kwptSource->bIsLogProbValue;
	cvTableValues.CopyFrom(&(kwptSource->cvTableValues));
}

KWProbabilityTable* KWProbabilityTable::Clone() const
{
	KWProbabilityTable* kwptClone;

	kwptClone = new KWProbabilityTable;
	kwptClone->CopyFrom(this);
	return kwptClone;
}

boolean KWProbabilityTable::Check() const
{
	boolean bOk = true;
	const double dRawEpsilon = 1e-5;
	double dEpsilon;
	int nSource;
	int nTarget;
	Continuous cProb;
	ALString sTmp;
	ContinuousVector cvSourceTotalProbs;
	ContinuousVector cvTargetTotalProbs;

	// Verification de coherence
	assert(nTableSourceSize >= 0);
	assert(nTableTargetSize >= 0);
	assert(cvTableValues.GetSize() == nTableSourceSize * nTableTargetSize);

	// Utilisation d'un epsilon adaptatif, en fonction du nombre de probas de la table
	dEpsilon = dRawEpsilon * sqrt(nTableSourceSize * nTableTargetSize * 1.0);

	// Verification que l'on a des proba partout
	// Calcul egalement des totaux sources et cibles
	cvSourceTotalProbs.SetSize(GetSourceSize());
	cvTargetTotalProbs.SetSize(GetTargetSize());
	for (nSource = 0; nSource < GetSourceSize(); nSource++)
	{
		for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
		{
			// Acces aux valeurs de la table par les methodes publiques,
			// ce qui permet d'en verifier les assertions
			if (IsTargetDirection())
			{
				if (IsProbValue())
					cProb = GetTargetConditionalProbAt(nSource, nTarget);
				else
					cProb = (Continuous)exp(GetTargetConditionalLogProbAt(nSource, nTarget));
			}
			else
			{
				if (IsProbValue())
					cProb = GetSourceConditionalProbAt(nSource, nTarget);
				else
					cProb = (Continuous)exp(GetSourceConditionalLogProbAt(nSource, nTarget));
			}

			// Mise a jour des totaux
			cvSourceTotalProbs.UpgradeAt(nSource, cProb);
			cvTargetTotalProbs.UpgradeAt(nTarget, cProb);

			// Test de validite de la proba
			if (cProb < 0 or cProb > 1)
			{
				AddError(sTmp + "Probability value (" + KWContinuous::ContinuousToString(cProb) +
					 ") at cell (" + IntToString(nSource + 1) + ", " + IntToString(nTarget + 1) +
					 ") should be between 0 and 1");
				bOk = false;
				break;
			}
		}
		if (not bOk)
			break;
	}

	// Verification que les totaux sont egaux a 1, selon le sens des probabilites conditionnelles
	if (IsTargetDirection())
	{
		for (nSource = 0; nSource < GetSourceSize(); nSource++)
		{
			if (fabs(cvSourceTotalProbs.GetAt(nSource) - 1.0) > dEpsilon)
			{
				AddError(sTmp + "Sum of target conditional probability values (" +
					 KWContinuous::ContinuousToString(cvSourceTotalProbs.GetAt(nSource)) +
					 ") at source index " + IntToString(nSource) + ") should be 1");
				bOk = false;
				break;
			}
		}
	}
	// Cas des probabilites conditionnelles sources
	else
	{
		for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
		{
			if (fabs(cvTargetTotalProbs.GetAt(nTarget) - 1.0) > dEpsilon)
			{
				AddError(sTmp + "Sum of source conditional probability values (" +
					 KWContinuous::ContinuousToString(cvTargetTotalProbs.GetAt(nTarget)) +
					 ") at target index " + IntToString(nTarget) + ") should be 1");
				bOk = false;
				break;
			}
		}
	}
	return bOk;
}

void KWProbabilityTable::Write(ostream& ost) const
{
	const ALString sSourcePrefix = "S";
	const ALString sTargetPrefix = "T";
	int nSource;
	int nTarget;

	// Titre
	if (IsTargetDirection())
		ost << "Target";
	else
		ost << "Source";
	ost << " conditional probability table";
	if (IsLogProbValue())
		ost << " (log)";
	ost << "\n";

	// Libelles des cibles
	for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
		ost << "\t" << sTargetPrefix << nTarget + 1;
	ost << "\n";

	// Affichage des valeurs de la table
	for (nSource = 0; nSource < GetSourceSize(); nSource++)
	{
		// Libelle de la source
		ost << sSourcePrefix << nSource + 1;

		// Valeur par cible
		for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
			ost << "\t" << GetValueAt(nSource, nTarget);
		ost << "\n";
	}
}

longint KWProbabilityTable::GetUsedMemory() const
{
	return sizeof(KWProbabilityTable) + cvTableValues.GetUsedMemory() - sizeof(ContinuousVector);
}

const ALString KWProbabilityTable::GetClassLabel() const
{
	return "Probability table";
}

void KWProbabilityTable::Test()
{
	KWDataGridStats* testDataGrid;
	KWProbabilityTable probabilityTable;

	// Non supervise
	cout << "Unsupervised (1)\n--------------------\n";
	testDataGrid = KWDataGridStats::CreateTestDataGrid(0, 1, false, 0, 2, 3);
	cout << *testDataGrid << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, true, false);
	cout << "Probability table\n" << probabilityTable << endl;
	delete testDataGrid;
	//
	cout << "Unsupervised (2)\n--------------------\n";
	testDataGrid = KWDataGridStats::CreateTestDataGrid(1, 1, false, 0, 2, 3);
	cout << *testDataGrid << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, true, false);
	cout << "Probability table\n" << probabilityTable << endl;
	delete testDataGrid;
	//
	cout << "Unsupervised simple(3)\n--------------------\n";
	testDataGrid = KWDataGridStats::CreateTestDataGrid(2, 1, true, 0, 2, 3);
	cout << *testDataGrid << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, true, false);
	cout << "Probability table\n" << probabilityTable << endl;
	delete testDataGrid;

	// Supervise
	cout << "Supervised (1, 1)\n--------------------\n";
	testDataGrid = KWDataGridStats::CreateTestDataGrid(1, 1, false, 1, 2, 3);
	cout << *testDataGrid << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, true, false);
	cout << "Probability table\n" << probabilityTable << endl;
	delete testDataGrid;
	//
	cout << "Supervised simple (2, 1)\n--------------------\n";
	testDataGrid = KWDataGridStats::CreateTestDataGrid(2, 1, true, 2, 2, 3);
	cout << *testDataGrid << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, true, false);
	cout << "Probability table\n" << probabilityTable << endl;
	delete testDataGrid;
	//
	cout << "Supervised(1, 2)\n--------------------\n";
	testDataGrid = KWDataGridStats::CreateTestDataGrid(1, 2, false, 1, 2, 3);
	cout << *testDataGrid << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, true, false);
	cout << "Probability table\n" << probabilityTable << endl;
	delete testDataGrid;
	//
	cout << "Supervised(2, 2)\n--------------------\n";
	testDataGrid = KWDataGridStats::CreateTestDataGrid(2, 2, false, 2, 2, 3);
	cout << *testDataGrid << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, true, false);
	cout << "Probability table\n" << probabilityTable << endl;
	delete testDataGrid;
	cout << "Supervised (1, 1)\n--------------------\n";
	testDataGrid = KWDataGridStats::CreateTestDataGrid(0, 2, true, 1, 3, 3);
	cout << *testDataGrid << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, true, false);
	cout << "Target probability table\n" << probabilityTable << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, true, true);
	cout << "Target probability table (log)\n" << probabilityTable << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, false, false);
	cout << "Source probability table\n" << probabilityTable << endl;
	probabilityTable.ImportDataGridStats(testDataGrid, false, true);
	cout << "Source probability table (log)\n" << probabilityTable << endl;
	delete testDataGrid;
}
