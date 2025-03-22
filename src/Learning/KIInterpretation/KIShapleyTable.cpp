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
	ost << "\tshapeley Value";
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
	return "Probability table";
}

void KIShapleyTable::Test() {}
