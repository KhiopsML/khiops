// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class MHKMHistogramCosts;

#include "MHMODLHistogramCost.h"
#include "MHNMLStat.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHKMHistogramCosts
// Definition de la structure des couts d'un histogram KM: Kontkanen and Myllymaki
class MHKMHistogramCosts : public MHMODLHistogramCosts
{
public:
	// Constructeur
	MHKMHistogramCosts();
	~MHKMHistogramCosts();

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type MHHistogramVector)
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartCost(const KWFrequencyVector* part) const override;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const;

	// Affichage du cout de la partition
	void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const;

	// Cout de modele par entite
	double ComputePartitionConstructionCost(int nPartNumber) const;
	double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const;
	double ComputePartModelCost(const KWFrequencyVector* part) const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};