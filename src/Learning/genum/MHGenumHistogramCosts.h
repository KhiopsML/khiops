// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class MHGenumHistogramCosts;

#include "KWUnivariatePartitionCost.h"
#include "MHGenumHistogramVector.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHGenumHistogramCosts
// Definition de la structure des couts d'une discretisation non supervisee MODL
class MHGenumHistogramCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	MHGenumHistogramCosts();
	~MHGenumHistogramCosts();

	// Nombre total de bin E, correspondant a la precision des longueurs des intervalles (defaut: 0; a parametrer)
	void SetTotalBinNumber(int nValue);
	int GetTotalBinNumber() const;

	// Nombre de partiles G pour l'optimisation des modeles granularises (defaut: 0, signifie ignore)
	void SetPartileNumber(int nValue);
	int GetPartileNumber() const;

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type MHGenumHistogramVector)
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartCost(const KWFrequencyVector* part) const override;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const override;

	// Affichage du cout de la partition
	void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const override;

	// Cout de modele par entite
	double ComputePartitionConstructionCost(int nPartNumber) const override;
	double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartModelCost(const KWFrequencyVector* part) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Parametres pour les calculs de cout
	int nTotalBinNumber;
	int nPartileNumber;
};
