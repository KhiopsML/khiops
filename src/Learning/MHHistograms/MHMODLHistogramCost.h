// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class MHMODLHistogramCosts;

#include "KWUnivariatePartitionCost.h"
#include "MHMODLHistogramVector.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHMODLHistogramCosts
// Definition de la structure des couts d'une discretisation non supervisee MODL
class MHMODLHistogramCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	MHMODLHistogramCosts();
	~MHMODLHistogramCosts();

	// Cout de codage de hyper-parametres
	// Ce cout constant impacte le null cost, le cost et le level, mais pas l'optimisation
	// Cout comprenant:
	//   . floating-point bin des valeurs min et max
	//   . exposant du min central bin
	//   . mantisse des valeurs min et max dans leur floating-point bin
	void SetHyperParametersCost(double dValue);
	double GetHyperParametersCost() const;

	// Min du central bin exponent
	void SetMinCentralBinExponent(int nValue);
	int GetMinCentralBinExponent() const;

	// Max du central bin exponent
	void SetMaxCentralBinExponent(int nValue);
	int GetMaxCentralBinExponent() const;

	// Central bin exponent
	void SetCentralBinExponent(int nValue);
	int GetCentralBinExponent() const;

	// Niveau de hierarchie des modeles granularises
	void SetHierarchicalLevel(int nValue);
	int GetHierarchicalLevel() const;

	// Longueur du plus petit bin elementaire
	// Chaque partile a une longueur multiple de cette largeur
	void SetMinBinLength(double dValue);
	double GetMinBinLength() const;

	// Nombre de partiles G pour l'optimisation des modeles granularises
	void SetPartileNumber(int nValue);
	int GetPartileNumber() const;

	//////////////////////////////////////////////////////////////
	// Methode calcul de cout des hyperparametres

	// Cout de codage de floating-point bin
	static double ComputeFloatingPointBinCost(boolean bIsCentralBin, int nSign, int nExponent);

	// Cout de codage de l'exposant du central bin
	static double ComputeCentralBinExponentCost(int nExponent);

	// Cout de codage des bornes du domaine de valeurs d'un jeu de donnees dans leur floating-point bin, en
	// utilisant n bits de mantisse
	static double ComputeDomainBoundsMantissaCost(int nMantissaBitNumber);

	// Cout de codage d'une valeur dans son floating-point bin, en utilisant n bits de mantisse
	static double ComputeMantissaCost(int nMantissaBitNumber);

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

	// Calcul du cout null de reference de la partition sur l'ensemble [1,2]
	double ComputeReferenceNullCost(int nFrequency, int nExtremeValueMantissaBinBitNumber);

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const override;

	// Affichage du cout de la partition
	void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const;

	// Cout de modele par entite
	double ComputePartitionConstructionCost(int nPartNumber) const;
	double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const;
	double ComputePartModelCost(const KWFrequencyVector* part) const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Parametres pour les calculs de cout
	double dHyperParametersCost;
	int nMinCentralBinExponent;
	int nMaxCentralBinExponent;
	int nCentralBinExponent;
	int nHierarchicalLevel;
	double dMinBinLength;
	int nPartileNumber;
};
