// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MHFloatingPointFrequencyTableBuilder.h"

//////////////////////////////////////////////////////////////////////////////////////
// Sous-classe de MHFloatingPointFrequencyTableBuilder specialisee pour la gestion
// des donnees tronquees
class MHTruncationFloatingPointFrequencyTableBuilder : public MHFloatingPointFrequencyTableBuilder
{
public:
	// Constructeur
	MHTruncationFloatingPointFrequencyTableBuilder();
	~MHTruncationFloatingPointFrequencyTableBuilder();

	// Parametrage du epsilon de troncature (defaut: 0)
	// La classe n'est utilisable que pour un epsilon de troncature strictement positif
	void SetTruncationEpsilon(double dValue);
	double GetTruncationEpsilon() const;

	// Epsilon de troncature binaire, plus petit superieur ou egal au epsilon de troncature
	double GetTruncationBinaryEpsilon() const;

	// Exposant de l'epsilon de troncature binaire
	int GetTruncationBinaryEpsilonExponent() const;

	// Redefinition des methodes virtuelles
	void InitializeBins(const ContinuousVector* cvSourceBinLowerValues,
			    const ContinuousVector* cvSourceBinUpperValues,
			    const IntVector* ivSourceBinFrequencies) override;
	void Clean() override;
	void SetCentralBinExponent(int nValue) override;
	int GetMinOptimizedCentralBinExponent() const override;
	int GetMaxOptimizedCentralBinExponent() const override;
	int GetTotalBinNumberAt(int nHierarchyLevel) const override;
	void ExtractFloatingPointBinBounds(Continuous cValue, int nHierarchyBitNumber, Continuous& cLowerBound,
					   Continuous& cUpperBound) const override;

	// Acces au table builder portant sur les bins initiaux avant troncature
	const MHFloatingPointFrequencyTableBuilder* GetInitialBinsTableBuilder() const;

	// Transformation de valeur pour un epsilon de troncature
	// La valeur est multipliee par dBinaryTruncationEpsilon/dTruncationEpsilon, puis projetee
	// sur la valeur la plus proche au epsilon binaire de troncature pres
	Continuous TransformValue(Continuous cValue) const;

	// Transformation inverse d'une valeur
	Continuous InverseTransformValue(Continuous cTransformedValue) const;

	/////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Transformation d'un vecteur de valeurs
	void TransformValues(const ContinuousVector* cvSourceValues, ContinuousVector* cvOutputTransformedValues) const;

	// Redefinition des metrhodes virtuelles dans la cas de la troncature
	Continuous GetSystemMinValue() const override;
	Continuous GetSystemMaxValue() const override;
	void UpdateMaxSafeHierarchyLevel() override;
	void InitializeDomainBounds() override;

	// Gestion de l'epsilon de troncature
	double dTruncationEpsilon;
	double dTruncationBinaryEpsilon;
	int nTruncationBinaryEpsilonExponent;
	MHFloatingPointFrequencyTableBuilder* initialBinsTableBuilder;
};
