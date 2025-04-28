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

void KIShapleyTable::InitializeFromDataGridStats(const SymbolVector* svTargetValues,
						 const IntVector* ivTargetValueFrequencies,
						 const KWDataGridStats* dataGridStats, double dAttributeWeight)
{
	const KWDGSAttributePartition* targetPartition;
	IntVector ivSourcePartFrequencies;
	IntVector ivTargetPartFrequencies;
	int nTotalFrequency;
	int nSourcePartNumber;
	int nTargetPartNumber;
	int nSourcePart;
	int nTargetPart;
	int nTarget;
	double dLaplaceEpsilon;
	double dConditionalOneFrequency;
	double dConditionalOneLaplaceEpsilon;
	double dConditionalAllFrequency;
	double dConditionalAllLaplaceEpsilon;
	double dProbOne;
	double dProbAll;
	double dTerm;
	double dExpectedTerm;
	double dShapleyValue;

	require(svTargetValues != NULL);
	require(svTargetValues->GetSize() > 1);
	require(ivTargetValueFrequencies != NULL);
	require(ivTargetValueFrequencies->GetSize() == svTargetValues->GetSize());
	require(dataGridStats != NULL);
	require(dataGridStats->GetAttributeNumber() == 2);
	require(dataGridStats->GetAttributeAt(1)->GetAttributeType() == KWType::Symbol);
	require(dataGridStats->GetAttributeAt(0)->GetPartNumber() > 1);
	require(dataGridStats->GetAttributeAt(1)->GetPartNumber() > 1);
	require(dataGridStats->GetAttributeAt(1)->GetPartNumber() <= svTargetValues->GetSize());
	require(0 < dAttributeWeight);

	// Acces a la partition cible
	targetPartition = dataGridStats->GetAttributeAt(1);

	// Calcul des effectifs par partie pour l'attribut source et cible de la grille
	dataGridStats->ExportAttributePartFrequenciesAt(0, &ivSourcePartFrequencies);
	dataGridStats->ExportAttributePartFrequenciesAt(1, &ivTargetPartFrequencies);
	nSourcePartNumber = ivSourcePartFrequencies.GetSize();
	nTargetPartNumber = ivTargetPartFrequencies.GetSize();
	nTotalFrequency = dataGridStats->ComputeGridFrequency();

	// Initialisation de la taille avec le nombre de parties sources et le nombre de valeurs cibles
	// Attention, le cacul des valeurs de Shapley se fait par valeur cible, et non par partie cible,
	// notamment pour le cas avec groupement des valeurs cibles
	// Dans le cas avec groupement des valeurs cibles, on exploite l'hypothese de densite uniforme
	// par morceaux pour faire le calcul des valeurs de Shapley en repartissant les effectifs de chaque
	// groupe de valeurs cibles au prorata des effectifs des valeurs du groupe
	Initialize(dataGridStats->GetAttributeAt(0)->GetPartNumber(), svTargetValues->GetSize());

	// On calcul d'abord les log(p(x_i|yOne)/p(x_i|yAll)) pour chaque x_i et chaque y_j
	// en mode One versus All pour chaque classe y_j
	//
	// Utilisation d'un epsilon de Laplace pour eviter les probabilite nulles
	// On prend l'effectif total plus un pour eviter les effets de bord (cf. classe KWProbabilityTable)
	// Pour etre au plus proche des calculs de probabilites utilises dans le SNB, on considere toujours
	// le meme nombre de valeurs cibles dans l'utilisation de l'epsilon de Laplace, en utilisant 1 espilon pour
	// la probabilite de la partie cible d'interet et (nTargetNumber-1) epsilon pour l'ensemble des autres parties cibles
	dLaplaceEpsilon = 1.0 / (nTotalFrequency + 1);
	for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
	{
		// Index de la partie cible correspondant a la valeur cible
		nTargetPart = targetPartition->ComputeSymbolPartIndex(svTargetValues->GetAt(nTarget));

		// Epsilon de Laplace associe a la valeur cible, reparti au prorata de l'effectif
		// de la valeur cible dans son eventuelle partie cible, dans le cas One
		dConditionalOneLaplaceEpsilon = dLaplaceEpsilon;
		if (not targetPartition->ArePartsSingletons())
			dConditionalOneLaplaceEpsilon =
			    (dConditionalOneLaplaceEpsilon * ivTargetValueFrequencies->GetAt(nTarget)) /
			    ivTargetPartFrequencies.GetAt(nTargetPart);

		// Idem pour la cas All
		dConditionalAllLaplaceEpsilon = nTargetPartNumber * dLaplaceEpsilon - dConditionalOneLaplaceEpsilon;

		// Premiere passe pour calcul le terme de valeur et d'esperance
		dExpectedTerm = 0;
		for (nSourcePart = 0; nSourcePart < GetSourceSize(); nSourcePart++)
		{
			// Effectif de la cellule (partie source, valeur cible) pour la valeur dans le cas One
			// - effectif exact dans le cas de valeurs cible singleton
			// - effectif au prorata des valeurs de la partie dans le cas avec groupement de valeur cibles
			dConditionalOneFrequency = dataGridStats->GetBivariateCellFrequencyAt(nSourcePart, nTargetPart);
			if (not targetPartition->ArePartsSingletons())
				dConditionalOneFrequency =
				    (dConditionalOneFrequency * ivTargetValueFrequencies->GetAt(nTarget)) /
				    ivTargetPartFrequencies.GetAt(nTargetPart);

			// Effectif de la cellule et epsilon de Laplace dans le cas All
			dConditionalAllFrequency =
			    ivSourcePartFrequencies.GetAt(nSourcePart) - dConditionalOneFrequency;

			// Probabilite conditionnelle pour la valeur source sachant la valeur cible
			dProbOne = (dConditionalOneFrequency + dConditionalOneLaplaceEpsilon) /
				   (ivTargetValueFrequencies->GetAt(nTarget) +
				    nSourcePartNumber * dConditionalOneLaplaceEpsilon);

			// Probabilite conditionnelle pour la valeur source sachant toutes les autres valeurs cibles
			dProbAll = (dConditionalAllFrequency + dConditionalAllLaplaceEpsilon) /
				   (nTotalFrequency - ivTargetValueFrequencies->GetAt(nTarget) +
				    nSourcePartNumber * dConditionalAllLaplaceEpsilon);

			// Memorisation du resultats intermediaire
			dTerm = log(dProbOne / dProbAll);
			SetShapleyValueAt(nSourcePart, nTarget, dTerm);

			// Mise a jour du calcul d'esperance de la valeur
			dExpectedTerm += (ivSourcePartFrequencies.GetAt(nSourcePart) / (double)nTotalFrequency) * dTerm;
		}

		// Deuxieme passe pour soustraire l'esperance et obtenir la valeur de Shapley
		for (nSourcePart = 0; nSourcePart < GetSourceSize(); nSourcePart++)
		{
			dTerm = GetShapleyValueAt(nSourcePart, nTarget);
			dShapleyValue = dAttributeWeight * (dTerm - dExpectedTerm);
			SetShapleyValueAt(nSourcePart, nTarget, dShapleyValue);
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
