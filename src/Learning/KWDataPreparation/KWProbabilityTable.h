// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWContinuous.h"
#include "KWDataGridStats.h"

//////////////////////////////////////////////////////////
// Classe KWProbabilityTable
// Gestion d'une tableau de probabilites conditionnelles
// entre des valeurs sources et cibles
// Les probabilites sont disponibles selon les modes suivants:
//   . pour la source ou la cible
//   . en version directe ou logarithmique
// Les acces aux probabilites sont controles par assertions.
class KWProbabilityTable : public Object
{
public:
	// Constructeur
	KWProbabilityTable();
	~KWProbabilityTable();

	///////////////////////////////////////////////////////////////////////////
	// Initialisation

	// Initialisation pour une taille de table donnee et pour les modes choisis:
	//  - direction des probabilies conditionnelles
	//  - valeur ou log de la valeur
	// Ces choix ne peux pas etre remis en cause
	// Toutes les valeurs sont mise a 0
	void Initialize(int nSourceSize, int nTargetSize, boolean bTargetDirection, boolean bLogProbValue);

	// Taille de la table
	int GetSourceSize() const;
	int GetTargetSize() const;

	// Direction du calcul des probabilites conditionnelles: cible|source ou source|cible
	boolean IsTargetDirection() const;
	boolean IsSourceDirection() const;

	// Acces aux probabilite par leur valeur ou par le log de leur valeur
	boolean IsProbValue() const;
	boolean IsLogProbValue() const;

	////////////////////////////////////////////////////////////////////////////////
	// Acces aux valeurs de la table
	// Les valeur sont de type Continuous par coherence avec les calcul effectues
	// en deploiement dans les  regles de derivation, et par soucis d'espace memoire
	// Les methodes sont en quatre version (selon les mode), uniquement afin de
	// lisibilite et de controle de la coherence avec les modes choisis

	// Acces dans le cas de probabilites conditionnelles de la cible, par probabilite
	void SetTargetConditionalProbAt(int nSourceIndex, int nTargetIndex, Continuous cProb);
	Continuous GetTargetConditionalProbAt(int nSourceIndex, int nTargetIndex) const;

	// Acces dans le cas de probabilites conditionnelles de la cible, par log de probabilite
	void SetTargetConditionalLogProbAt(int nSourceIndex, int nTargetIndex, Continuous cLogProb);
	Continuous GetTargetConditionalLogProbAt(int nSourceIndex, int nTargetIndex) const;

	// Acces dans le cas de probabilites conditionnelles de la source, par probabilite
	void SetSourceConditionalProbAt(int nSourceIndex, int nTargetIndex, Continuous cProb);
	Continuous GetSourceConditionalProbAt(int nSourceIndex, int nTargetIndex) const;

	// Acces dans le cas de probabilites conditionnelles de la source, par log de probabilite
	void SetSourceConditionalLogProbAt(int nSourceIndex, int nTargetIndex, Continuous cLogProb);
	Continuous GetSourceConditionalLogProbAt(int nSourceIndex, int nTargetIndex) const;

	////////////////////////////////////////////////////////////////////////////////
	// Service divers

	// Import d'un objet KWDataGridStats afin d'en deduire les probabilites conditionnelles
	// La taille de la table est deduite de la taille des grilles sources et cibles
	// (produits cartesiens des attributs sources et cibles)
	// L'indexation de la table provient de l'indexation des cellules source et cible de la grille
	// Un espsilon de Laplace minimal (1/(N+1)) est utilise pour eviter les probabilites nulles
	void ImportDataGridStats(const KWDataGridStats* dataGridStats, boolean bTargetDirection, boolean bLogProbValue);

	// Copie a partir d'un table source
	void CopyFrom(const KWProbabilityTable* kwptSource);

	// Duplication
	KWProbabilityTable* Clone() const;

	// Verification de l'integrite
	// Les verifications sont effectuees selon le mode choisi
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
	//// Implementation
protected:
	// Acces generiques aux valeurs de la table
	void SetValueAt(int nSourceIndex, int nTargetIndex, Continuous cValue);
	Continuous GetValueAt(int nSourceIndex, int nTargetIndex) const;

	// Variables d'instances
	ContinuousVector cvTableValues;
	int nTableSourceSize;
	int nTableTargetSize;
	boolean bIsTargetDirection;
	boolean bIsLogProbValue;
};

// Methode en inline

inline int KWProbabilityTable::GetSourceSize() const
{
	return nTableSourceSize;
}

inline int KWProbabilityTable::GetTargetSize() const
{
	return nTableTargetSize;
}

inline boolean KWProbabilityTable::IsTargetDirection() const
{
	return bIsTargetDirection;
}

inline boolean KWProbabilityTable::IsSourceDirection() const
{
	return not bIsTargetDirection;
}

inline boolean KWProbabilityTable::IsProbValue() const
{
	return not bIsLogProbValue;
}

inline boolean KWProbabilityTable::IsLogProbValue() const
{
	return bIsLogProbValue;
}

inline void KWProbabilityTable::SetTargetConditionalProbAt(int nSourceIndex, int nTargetIndex, Continuous cProb)
{
	require(IsTargetDirection() and IsProbValue());
	require(0 <= cProb and cProb <= 1);
	SetValueAt(nSourceIndex, nTargetIndex, cProb);
}

inline Continuous KWProbabilityTable::GetTargetConditionalProbAt(int nSourceIndex, int nTargetIndex) const
{
	require(IsTargetDirection() and IsProbValue());
	return GetValueAt(nSourceIndex, nTargetIndex);
}

inline void KWProbabilityTable::SetTargetConditionalLogProbAt(int nSourceIndex, int nTargetIndex, Continuous cLogProb)
{
	require(IsTargetDirection() and IsLogProbValue());
	require(0 <= exp(cLogProb) and exp(cLogProb) <= 1);
	SetValueAt(nSourceIndex, nTargetIndex, cLogProb);
}

inline Continuous KWProbabilityTable::GetTargetConditionalLogProbAt(int nSourceIndex, int nTargetIndex) const
{
	require(IsTargetDirection() and IsLogProbValue());
	return GetValueAt(nSourceIndex, nTargetIndex);
}

inline void KWProbabilityTable::SetSourceConditionalProbAt(int nSourceIndex, int nTargetIndex, Continuous cProb)
{
	require(IsSourceDirection() and IsProbValue());
	require(0 <= cProb and cProb <= 1);
	SetValueAt(nSourceIndex, nTargetIndex, cProb);
}

inline Continuous KWProbabilityTable::GetSourceConditionalProbAt(int nSourceIndex, int nTargetIndex) const
{
	require(IsSourceDirection() and IsProbValue());
	return GetValueAt(nSourceIndex, nTargetIndex);
}

inline void KWProbabilityTable::SetSourceConditionalLogProbAt(int nSourceIndex, int nTargetIndex, Continuous cLogProb)
{
	require(IsSourceDirection() and IsLogProbValue());
	require(0 <= exp(cLogProb) and exp(cLogProb) <= 1);
	SetValueAt(nSourceIndex, nTargetIndex, cLogProb);
}

inline Continuous KWProbabilityTable::GetSourceConditionalLogProbAt(int nSourceIndex, int nTargetIndex) const
{
	require(IsSourceDirection() and IsLogProbValue());
	return GetValueAt(nSourceIndex, nTargetIndex);
}

inline void KWProbabilityTable::SetValueAt(int nSourceIndex, int nTargetIndex, Continuous cValue)
{
	require(0 <= nSourceIndex and nSourceIndex < GetSourceSize());
	require(0 <= nTargetIndex and nTargetIndex < GetTargetSize());
	cvTableValues.SetAt(nSourceIndex + nTargetIndex * nTableSourceSize, cValue);
}

inline Continuous KWProbabilityTable::GetValueAt(int nSourceIndex, int nTargetIndex) const
{
	require(0 <= nSourceIndex and nSourceIndex < GetSourceSize());
	require(0 <= nTargetIndex and nTargetIndex < GetTargetSize());
	return cvTableValues.GetAt(nSourceIndex + nTargetIndex * nTableSourceSize);
}