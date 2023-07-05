// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class DTMODLDiscretizationCosts;
class DTMODLGroupingCosts;

#include "KWStat.h"
#include "KWFrequencyVector.h"
#include "KWVersion.h"
#include "KWUnivariatePartitionCost.h"

////////////////////////////////////////////////////////////////////////////
// Classe DTMODLDiscretizationCosts
// Definition de la structure des couts d'une discretisation MODL pour un arbre binaire
class DTMODLDiscretizationCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	DTMODLDiscretizationCosts();
	~DTMODLDiscretizationCosts();

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type KWDenseFrequencyVector)
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
};

////////////////////////////////////////////////////////////////////////////
// Classe DTMODLGroupingCosts
// Definition de la structure des couts d'une discretisation MODL pour un arbre binaire
class DTMODLGroupingCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	DTMODLGroupingCosts();
	~DTMODLGroupingCosts();

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type KWDenseFrequencyVector)
	// Le parametre nPartNumber designe le nombre total de parties
	// Une partition avec groupe poubelle non vide contient donc une partition en nPartNumber-1 groupes + 1 groupe
	// poubelle
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const override;
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
};