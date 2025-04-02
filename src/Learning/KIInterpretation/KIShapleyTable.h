// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWContinuous.h"
#include "KWDataGridStats.h"

//////////////////////////////////////////////////////////
// Classe KIShapleyTable
// Gestion d'une table de valeurs de Shapley pour une variable source donnee
class KIShapleyTable : public Object
{
public:
	// Constructeur
	KIShapleyTable();
	~KIShapleyTable();

	// Initialisation complete a partir d'une grille dont le dernier attribut contient l'attribut cible
	// et d'un poids de variable
	// Un epsilon de Laplace minimal (1/(N+1)) est utilise pour eviter les probabilites nulles
	void InitializeFromDataGridStats(const KWDataGridStats* dataGridStats, double dAttributeWeight);

	///////////////////////////////////////////////////////////////////////////
	// Initialisation

	// Initialisation pour une taille de table donnee:
	// Toutes les valeurs sont mise a 0
	void Initialize(int nSourceSize, int nTargetSize);

	// Taille de la table
	int GetSourceSize() const;
	int GetTargetSize() const;

	////////////////////////////////////////////////////////////////////////////////
	// Acces aux valeurs de la table des valeurs de Shapley
	// Les valeur sont de type Continuous par coherence avec les calcul effectues
	// en deploiement dans les  regles de derivation, et par soucis d'espace memoire
	// Reference:
	// V. Lemaire, F. Clerot, M. Boulle. An Efficient Shapley Value Computation for the Naive Bayes Classifier.
	// European Conference on Machine Learning (ECML PKDD) - Workshop AIMLAI, 2023

	// Acces aux valeurs de Shapley conditionnellement a la cible
	void SetShapleyValueAt(int nSourceIndex, int nTargetIndex, Continuous cProb);
	Continuous GetShapleyValueAt(int nSourceIndex, int nTargetIndex) const;

	////////////////////////////////////////////////////////////////////////////////
	// Service divers

	// Copie a partir d'un table source
	void CopyFrom(const KIShapleyTable* kwptSource);

	// Duplication
	KIShapleyTable* Clone() const;

	// Verification de l'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test
	static void Test();

	/////////////////////////////////////////////////////////
	// Implementation
protected:
	// Variables d'instances
	ContinuousVector cvTableValues;
	int nTableSourceSize;
	int nTableTargetSize;
};

// Methode en inline

inline int KIShapleyTable::GetSourceSize() const
{
	return nTableSourceSize;
}

inline int KIShapleyTable::GetTargetSize() const
{
	return nTableTargetSize;
}

inline void KIShapleyTable::SetShapleyValueAt(int nSourceIndex, int nTargetIndex, Continuous cProb)
{
	require(0 <= nSourceIndex and nSourceIndex < GetSourceSize());
	require(0 <= nTargetIndex and nTargetIndex < GetTargetSize());
	cvTableValues.SetAt(nSourceIndex + nTargetIndex * nTableSourceSize, cProb);
}

inline Continuous KIShapleyTable::GetShapleyValueAt(int nSourceIndex, int nTargetIndex) const
{
	require(0 <= nSourceIndex and nSourceIndex < GetSourceSize());
	require(0 <= nTargetIndex and nTargetIndex < GetTargetSize());
	return cvTableValues.GetAt(nSourceIndex + nTargetIndex * nTableSourceSize);
}
