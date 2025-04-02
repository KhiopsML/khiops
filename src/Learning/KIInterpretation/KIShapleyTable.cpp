// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIShapleyTable.h"

KIShapleyTable::KIShapleyTable()
{
	nTableSourceSize = 0;
	nTableTargetSize = 0;
}

KIShapleyTable::~KIShapleyTable() {}

void KIShapleyTable::InitializeFromDataGridStats(const KWDataGridStats* dataGridStats, double dAttributeWeight)
{
	IntVector ivSourceFrequencies;
	IntVector ivTargetFrequencies;
	int nTotalFrequency;
	int nTargetNumber;
	int nSource;
	int nTarget;
	double dLaplaceEpsilon;
	double dProb_i1;
	double dProb_i0;
	double dTerm;
	double dExpectedTerm;
	double dShapleyValue;

	require(dataGridStats != NULL);
	require(dataGridStats->GetAttributeNumber() == 2);
	require(dataGridStats->GetAttributeAt(0)->GetPartNumber() > 1);
	require(dataGridStats->GetAttributeAt(1)->GetPartNumber() > 1);
	require(0 < dAttributeWeight);

	// Initialisation de la taille
	Initialize(dataGridStats->GetAttributeAt(0)->GetPartNumber(),
		   dataGridStats->GetAttributeAt(1)->GetPartNumber());

	// Calcul des effectifs par partie pour l'attribut source et cible de la grille
	dataGridStats->ExportAttributePartFrequenciesAt(0, &ivSourceFrequencies);
	dataGridStats->ExportAttributePartFrequenciesAt(1, &ivTargetFrequencies);
	nTargetNumber = ivTargetFrequencies.GetSize();
	nTotalFrequency = dataGridStats->ComputeGridFrequency();

	// On calcul d'abord les log(p(x_i|y1)/p(x_i|y0)) pour chaque x_i et chaque y_j
	// en mode une classe contre tous pour chaque y_j
	//
	// Utilisation d'un epsilon de Laplace pour eviter les probabilite nulles
	// On prend l'effectif total plus un pour eviter les effets de bord (cf. classe KWProbabilityTable)
	// Pour etre au plus proche des calcul de probabilites utilises dans le SNB, on considere toujours
	// le meme nombre de valeurs cibles dans l'utilisation de l'epsilon de Laplace, en utilisant 1 espilon
	// pour la probabilite de la cible d'interet et (nTargetNumber-1) pour l'ensemble des autre valeurs cibles
	dLaplaceEpsilon = 1.0 / (nTotalFrequency + 1);
	for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
	{
		// Premiere passe pour calcul le terme de valeurn et d'esperance
		dExpectedTerm = 0;
		for (nSource = 0; nSource < GetSourceSize(); nSource++)
		{
			// Probabilite conditionnelle pour la valeur source sachant la valeur cible
			dProb_i1 = (dataGridStats->GetBivariateCellFrequencyAt(nSource, nTarget) + dLaplaceEpsilon) /
				   (ivTargetFrequencies.GetAt(nTarget) + nTargetNumber * dLaplaceEpsilon);

			// Probabilite conditionnelle pour la valeur source sachant toutes les autres valeurs cibles
			dProb_i0 =
			    (ivSourceFrequencies.GetAt(nSource) -
			     dataGridStats->GetBivariateCellFrequencyAt(nSource, nTarget) +
			     (nTargetNumber - 1) * dLaplaceEpsilon) /
			    (nTotalFrequency - ivTargetFrequencies.GetAt(nTarget) + nTargetNumber * dLaplaceEpsilon);

			// Memorisation du resultats intermediaire
			dTerm = log(dProb_i1 / dProb_i0);
			SetShapleyValueAt(nSource, nTarget, dTerm);

			// Mise a jour du calcul d'esperance de la valeur
			dExpectedTerm += (ivSourceFrequencies.GetAt(nSource) / (double)nTotalFrequency) * dTerm;
		}

		// Deuiemme passe pour soustraire l'esperance et obtenir la valeur de Shapley
		for (nSource = 0; nSource < GetSourceSize(); nSource++)
		{
			dTerm = GetShapleyValueAt(nSource, nTarget);
			dShapleyValue = dAttributeWeight * (dTerm - dExpectedTerm);
			SetShapleyValueAt(nSource, nTarget, dShapleyValue);
		}
	}
}

void KIShapleyTable::Initialize(int nSourceSize, int nTargetSize)
{
	require(nSourceSize >= 0);
	require(nTargetSize >= 0);

	// Memorisation des catacteristiques de la table
	nTableSourceSize = nSourceSize;
	nTableTargetSize = nTargetSize;

	// Creation des valeurs (apres reinitialisation)
	cvTableValues.SetSize(0);
	cvTableValues.SetSize(nTableSourceSize * nTableTargetSize);
}

void KIShapleyTable::CopyFrom(const KIShapleyTable* kwptSource)
{
	require(kwptSource != NULL);

	nTableSourceSize = kwptSource->nTableSourceSize;
	nTableTargetSize = kwptSource->nTableTargetSize;
	cvTableValues.CopyFrom(&(kwptSource->cvTableValues));
}

KIShapleyTable* KIShapleyTable::Clone() const
{
	KIShapleyTable* kwptClone;

	kwptClone = new KIShapleyTable;
	kwptClone->CopyFrom(this);
	return kwptClone;
}

boolean KIShapleyTable::Check() const
{
	if (cvTableValues.GetSize() == nTableSourceSize * nTableTargetSize)
		return true;

	return false;
}

void KIShapleyTable::Write(ostream& ost) const
{
	const ALString sSourcePrefix = "S";
	const ALString sTargetPrefix = "T";
	int nSource;
	int nTarget;

	// Titre
	ost << "Target";
	ost << "\tShapley Value";
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
			ost << "\t" << GetShapleyValueAt(nSource, nTarget);
		ost << "\n";
	}
}

longint KIShapleyTable::GetUsedMemory() const
{
	return sizeof(KIShapleyTable) + cvTableValues.GetUsedMemory() - sizeof(ContinuousVector);
}

const ALString KIShapleyTable::GetClassLabel() const
{
	return "Shapley table";
}

void KIShapleyTable::Test() {}
