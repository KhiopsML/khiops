// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour les predicteurs naive Bayes a base de grilles

class KWDRNBClassifier;
class KWDRSNBClassifier;
class KWDRNBRankRegressor;
class KWDRSNBRankRegressor;
class KWDRNBRegressor;
class KWDRSNBRegressor;

#include "KWDerivationRule.h"
#include "KWDRPreprocessing.h"
#include "KWDRPredictor.h"
#include "KWDataGrid.h"
#include "KWDataGridStats.h"
#include "KWContinuous.h"
#include "KWSymbol.h"
#include "KWDRDataGrid.h"

// Enregistrement de ces regles
void KWDRRegisterNBPredictorRules();

///////////////////////////////////////////////////////////////
// Classe KWDRNBClassifier
// Classifier base sur une ou plusieurs grilles
// Premiers operandes de type KWDRDataGridStats
// Dernier operande: distribution des valeurs de l'attribut cible (KWDRDataGrid univarie)
class KWDRNBClassifier : public KWDRClassifier
{
public:
	// Constructeur
	KWDRNBClassifier();
	~KWDRNBClassifier();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a une objet, et services associes

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Reimplementation des services de classification
	Symbol ComputeTargetValue() const override;
	Continuous ComputeTargetProb() const override;
	Continuous ComputeTargetProbAt(const Symbol& sValue) const override;
	Symbol ComputeBiasedTargetValue(const ContinuousVector* cvOffsets) const override;

	////////////////////////////////////////////////////////////////////
	// Compilation de la regle et services associes

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Acces aux statistiques de grilles en operande
	int GetDataGridStatsNumber() const;
	const KWDRDataGridStats* GetDataGridStatsAt(int nIndex) const;
	Continuous GetDataGridWeightAt(int nIndex) const;

	// Acces aux statistiques sur l'ensemble des grilles
	int GetDataGridSetTargetPartNumber() const;
	int GetDataGridSetTargetFrequencyAt(int nTarget) const;
	int GetDataGridSetTargetIndexAt(int nDataGridIndex, int nTarget) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul du vecteur de probabilites conditionnelles
	void ComputeTargetProbs() const;

	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Test si optimisee
	boolean IsOptimized() const;

	// Index du premier operande potentiel de type grille
	//  0 dans cette classe, 1 si un premier operande sert a memoriser les poids
	// (il peut n'y avoir aucun operande de type grille dans le cas ou aucun attribut n'est predictif)
	int nFirstDataGridOperand;

	// Tableau des statistiques par grilles
	ObjectArray oaDataGridStats;

	// Vecteur des poids de grille (par defaut: tous les poids a 1)
	ContinuousVector cvWeights;

	// Vecteurs des effectifs par partie cible
	IntVector ivDataGridSetTargetFrequencies;

	// Index des parties cibles de grille pour chaque partie cible de l'ensemble
	// Vecteur de taille nTargetPartNumber*DataGridRuleNumber
	// Dans le cas de plusieurs grilles, les parties resultent de l'union des parties
	// cibles sur l'ensemble des grilles
	IntVector ivDataGridTargetIndexes;

	// Vecteurs des valeurs cibles
	SymbolVector svTargetValues;

	// Vecteur des probabilites conditionnelles
	mutable ContinuousVector cvTargetProbs;

	// Probabilite conditionnelle par defaut, pour les classes inconnues
	mutable Continuous cUnknownTargetProb;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSNBClassifier
//   Selective Naive Bayes
// Le premier operande est un vecteur de poids des variables
// (KWDRContinuousVector); les autres operandes sont les
// memes que pour un Naive Bayes
class KWDRSNBClassifier : public KWDRNBClassifier
{
public:
	// Constructeur
	KWDRSNBClassifier();
	~KWDRSNBClassifier();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRNBRankRegressor
// Regresseur de rang base sur une ou plusieurs grilles
// (operandes de type KWDRDataGridStats)
class KWDRNBRankRegressor : public KWDRRankRegressor
{
public:
	// Constructeur
	KWDRNBRankRegressor();
	~KWDRNBRankRegressor();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a une objet, et services associes

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Reimplementation des services de regression de rang
	Continuous ComputeTargetRankMean() const override;
	Continuous ComputeTargetRankStandardDeviation() const override;
	Continuous ComputeTargetRankDensityAt(Continuous cRank) const override;
	Continuous ComputeTargetRankCumulativeProbAt(Continuous cRank) const override;

	////////////////////////////////////////////////////////////////////
	// Compilation de la regle et services associes

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Acces aux statistiques de grilles en operande
	int GetDataGridStatsNumber() const;
	const KWDRDataGridStats* GetDataGridStatsAt(int nIndex) const;
	Continuous GetDataGridWeightAt(int nIndex) const;

	// Acces aux statistiques sur l'ensemble des grilles
	int GetDataGridSetTargetPartNumber() const;
	Continuous GetDataGridSetTargetCumulativeFrequencyAt(int nTarget) const;
	int GetDataGridSetTargetIndexAt(int nDataGridIndex, int nTarget) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	friend class KWDRNBRegressor;

	// Calcul du vecteur de probabilites conditionnelles par intervalle cible
	void ComputeTargetProbs() const;

	/////////////////////////////////////////////////////////////////
	// Methodes de calcul pour les indicateurs de regression de rang

	// Calcul de l'esperance d'une variable, dont les valeurs sont fournies sous la forme
	// d'un vecteur de valeurs cumulees par partie cible
	Continuous ComputeExpectation(const ContinuousVector* cvTargetCumulativeValues) const;

	// Calcul de la fonction de repartition des rangs normalises
	Continuous ComputeRankCumulativeProb(Continuous cNormalizedRank) const;

	// Calcul de la densite sur les rangs, pour un rang normalise (entre 0 et 1)
	Continuous ComputeRankDensity(Continuous cNormalizedRank) const;

	// Calcul de la valeur d'un creneau cumule par partie cible
	// Creneau: 1 jusqu'a un seuil, 0 au dela
	// Cela permet de calculer la fonction de repartition par calcul d'esperance
	void ComputeCumulativeCrenel(ContinuousVector* cvTargetCumulativeValues, Continuous cNormalizedRank) const;

	// Calcul des rangs cumules par partie cible
	void ComputeCumulativeRanks(ContinuousVector* cvTargetCumulativeValues) const;

	// Calcul des carres des rangs cumules par partie cible
	void ComputeCumulativeSquareRanks(ContinuousVector* cvTargetCumulativeValues) const;

	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Test si optimisee
	boolean IsOptimized() const;

	// Index du premier operande de type grille
	// (0 dans cette classe, 1 si un premier operande sert a memoriser les poids)
	int nFirstDataGridOperand;

	// Tableau des statistiques par grilles
	ObjectArray oaDataGridStats;

	// Vecteur des probabilites conditionnelles
	mutable ContinuousVector cvTargetProbs;

	// Vecteur des poids de grille (par defaut: tous les poids a 1)
	ContinuousVector cvWeights;

	// Vecteurs des effectifs cumules par partie cible (S(j) = N.0 + N.1 + ... + N.j)
	ContinuousVector cvDataGridSetTargetCumulativeFrequencies;

	// Index des parties cibles de grille pour chaque partie cible de l'ensemble
	// Vecteur de taille nTargetPartNumber*DataGridRuleNumber
	// Dans le cas de plusieurs grilles, les parties resultent de l'union des parties
	// cibles sur l'ensemble des grilles
	IntVector ivDataGridTargetIndexes;

	// Vecteurs des rangs et de leur carre cumules par partie cible
	ContinuousVector cvTargetCumulativeRanks;
	ContinuousVector cvTargetCumulativeSquareRanks;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSNBRankRegressor
//   Selective Naive Bayes
// Le premier operande est un vecteur de poids des variables
// (KWDRContinuousVector); les autres operandes sont les
// memes que pour un Naive Bayes
class KWDRSNBRankRegressor : public KWDRNBRankRegressor
{
public:
	// Constructeur
	KWDRSNBRankRegressor();
	~KWDRSNBRankRegressor();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRNBRegressor
//   Selective Naive Bayes
// Premier operande: Structure(RankRegressor) depuis un KWDRNBRankRegressor
// Deuxieme operande: distribution des valeurs de l'attribut cible (KWDRDataGrid univarie)
class KWDRNBRegressor : public KWDRRegressor
{
public:
	// Constructeur
	KWDRNBRegressor();
	~KWDRNBRegressor();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	////////////////////////////////////////////////////////////////////
	// Application de la regle a une objet, et services associes

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Reimplementation des services de regression
	Continuous ComputeTargetMean() const override;
	Continuous ComputeTargetStandardDeviation() const override;
	Continuous ComputeTargetDensityAt(Continuous cValue) const override;
	Symbol ComputeTargetQuantileDistribution() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	/////////////////////////////////////////////////////
	// Services specifiques a la gestion des valeurs cible

	// Calcul de l'index de la valeur la plus proche de la valeur en parametre
	int GetTargetValueIndex(Continuous cValue) const;

	// Acces aux statistiques sur les valeurs (pas de doublons), issues de la compilation
	int GetSingleTargetValueNumber() const;
	Continuous GetSingleTargetValueAt(int nIndex) const;
	int GetSingleTargetValueFrequencyAt(int nIndex) const;
	int GetSingleTargetValueCumulativeFrequencyAt(int nIndex) const;

	/////////////////////////////////////////////////////////////////
	// Methodes de calcul pour les indicateurs de regression de rang

	// Calcul des effectifs cumules des valeurs par index de valeur
	void ComputeSingleTargetValueCumulativeInstanceNumbers(IntVector* ivResultVector) const;

	// Calcul des delta de valeurs pour la gestion des valeurs extremes
	Continuous ComputeTotalDeltaTargetValue() const;
	Continuous ComputeMeanDeltaTargetValue() const;

	// Calcul du nombre de valeurs manquantes
	int ComputeMissingValueNumber() const;

	// Calcul des valeurs cumulees par partie cible
	void ComputeCumulativeTargetValues(ContinuousVector* cvResultVector) const;

	// Calcul des carres des rangs cumules par partie cible
	void ComputeCumulativeSquareTargetValues(ContinuousVector* cvResultVector) const;

	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Test si optimisee
	boolean IsOptimized() const;

	// Effectifs cumules des valeurs par index de valeur
	IntVector ivSingleTargetValueCumulativeInstanceNumbers;

	// Vecteurs des valeurs et de leur carre cumules par partie cible
	ContinuousVector cvCumulativeTargetValues;
	ContinuousVector cvCumulativeSquareTargetValues;

	// Delta de valeur pour la gestion des valeurs extremes
	Continuous cTotalDeltaTargetValue;
	Continuous cMeanDeltaTargetValue;

	// Nombre de valeurs manquantes
	// En cas de valeurs manquantes en apprentissage, le regresseur ne predit que des valeurs manquantes
	int nMissingValueNumber;

	// Regresseur de rang
	const KWDRNBRankRegressor* rankRegressorRule;

	// Liste des valeurs en apprentissage, sous forme d'une grille univariee
	const KWDRDataGrid* targetDataGridRule;
	const KWDRContinuousValueSet* targetValuesRules;
	const KWDRFrequencies* targetFrequenciesRule;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSNBRegressor
//   Selective Naive Bayes
// Premier operande: Structure(RankRegressor) depuis un KWDRSNBRankRegressor
// Deuxieme operande: distribution des valeurs de l'attribut cible (KWDataGrid univariee Continuous)
class KWDRSNBRegressor : public KWDRNBRegressor
{
public:
	// Constructeur
	KWDRSNBRegressor();
	~KWDRSNBRegressor();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////
// Methode en inline

inline int KWDRNBClassifier::GetDataGridStatsNumber() const
{
	require(IsOptimized());
	return oaDataGridStats.GetSize();
}

inline const KWDRDataGridStats* KWDRNBClassifier::GetDataGridStatsAt(int nIndex) const
{
	require(IsOptimized());
	require(0 <= nIndex and nIndex < oaDataGridStats.GetSize());
	return cast(KWDRDataGridStats*, oaDataGridStats.GetAt(nIndex));
}

inline Continuous KWDRNBClassifier::GetDataGridWeightAt(int nIndex) const
{
	require(IsOptimized());
	require(0 <= nIndex and nIndex < oaDataGridStats.GetSize());
	return cvWeights.GetAt(nIndex);
}

inline int KWDRNBClassifier::GetDataGridSetTargetPartNumber() const
{
	require(IsOptimized());
	return ivDataGridSetTargetFrequencies.GetSize();
}

inline int KWDRNBClassifier::GetDataGridSetTargetFrequencyAt(int nTarget) const
{
	require(IsOptimized());
	require(0 <= nTarget and nTarget < ivDataGridSetTargetFrequencies.GetSize());
	return ivDataGridSetTargetFrequencies.GetAt(nTarget);
}

inline int KWDRNBClassifier::GetDataGridSetTargetIndexAt(int nDataGridIndex, int nTarget) const
{
	require(IsOptimized());
	require(0 <= nDataGridIndex and nDataGridIndex < oaDataGridStats.GetSize());
	require(0 <= nTarget and nTarget < ivDataGridTargetIndexes.GetSize());
	return ivDataGridTargetIndexes.GetAt(nDataGridIndex * ivDataGridSetTargetFrequencies.GetSize() + nTarget);
}

inline boolean KWDRNBClassifier::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

inline int KWDRNBRegressor::GetTargetValueIndex(Continuous cValue) const
{
	return targetValuesRules->GetContinuousPartIndex(cValue);
}

inline int KWDRNBRegressor::GetSingleTargetValueNumber() const
{
	return targetValuesRules->GetValueNumber();
}

inline Continuous KWDRNBRegressor::GetSingleTargetValueAt(int nIndex) const
{
	return targetValuesRules->GetValueAt(nIndex);
}

inline int KWDRNBRegressor::GetSingleTargetValueFrequencyAt(int nIndex) const
{
	return targetFrequenciesRule->GetFrequencyAt(nIndex);
}

inline int KWDRNBRegressor::GetSingleTargetValueCumulativeFrequencyAt(int nIndex) const
{
	return ivSingleTargetValueCumulativeInstanceNumbers.GetAt(nIndex);
}

inline boolean KWDRNBRegressor::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

inline int KWDRNBRankRegressor::GetDataGridStatsNumber() const
{
	require(IsOptimized());
	return oaDataGridStats.GetSize();
}

inline const KWDRDataGridStats* KWDRNBRankRegressor::GetDataGridStatsAt(int nIndex) const
{
	require(IsOptimized());
	require(0 <= nIndex and nIndex < oaDataGridStats.GetSize());
	return cast(KWDRDataGridStats*, oaDataGridStats.GetAt(nIndex));
}

inline Continuous KWDRNBRankRegressor::GetDataGridWeightAt(int nIndex) const
{
	require(IsOptimized());
	require(0 <= nIndex and nIndex < oaDataGridStats.GetSize());
	return cvWeights.GetAt(nIndex);
}

inline int KWDRNBRankRegressor::GetDataGridSetTargetPartNumber() const
{
	require(IsOptimized());
	return cvDataGridSetTargetCumulativeFrequencies.GetSize();
}

inline Continuous KWDRNBRankRegressor::GetDataGridSetTargetCumulativeFrequencyAt(int nTarget) const
{
	require(IsOptimized());
	require(0 <= nTarget and nTarget < ivDataGridTargetIndexes.GetSize());
	return cvDataGridSetTargetCumulativeFrequencies.GetAt(nTarget);
}

inline int KWDRNBRankRegressor::GetDataGridSetTargetIndexAt(int nDataGridIndex, int nTarget) const
{
	require(IsOptimized());
	require(0 <= nDataGridIndex and nDataGridIndex < oaDataGridStats.GetSize());
	require(0 <= nTarget and nTarget < ivDataGridTargetIndexes.GetSize());
	return ivDataGridTargetIndexes.GetAt(nDataGridIndex * cvDataGridSetTargetCumulativeFrequencies.GetSize() +
					     nTarget);
}

inline boolean KWDRNBRankRegressor::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}