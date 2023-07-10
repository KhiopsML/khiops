// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDMultinomialSampleGenerator;
class KDIndexedFrequency;

#include "Vector.h"
#include "KWStat.h"
#include "KWSortableIndex.h"
#include "SortedList.h"
#include "Timer.h"

///////////////////////////////////////////////////////////////////////////////
// Classe KDMultinomialSampleGenerator
// Generation d'echantillons MAP selon une loi multonomiale
// Les effectifs sont codes avec des double plutot que des int pour pouvoir
// gerer des effectifs tres importants
class KDMultinomialSampleGenerator : public Object
{
public:
	// Constructeur
	KDMultinomialSampleGenerator();
	~KDMultinomialSampleGenerator();

	/////////////////////////////////////////////////////////////////////////////////
	// Generation du meilleur echantillon pour des lois basees sur la multinomiale
	// Pour un effectif N donne et un vecteur de proba dvProbs donne (potentiellement partiel),
	// il s'agit de tirer l'echantillon (ivFrequencies) le plus probable

	// Meilleur echantillon pour un effectif donne et une distribution multinomiale donnee
	// On choisit l'echantillon le plus probable (calcul "presque" exact)
	void ComputeBestSample(double dTotalFrequency, const DoubleVector* dvProbs, DoubleVector* dvFrequencies) const;

	// Meilleur echantillon pour une distribution multinomiale equidistribuee
	void ComputeBestEquidistributedSample(double dTotalFrequency, int nValueNumber,
					      DoubleVector* dvFrequencies) const;

	// Meilleur echantillon pour une distribution multinomiale equidistribuee a deux niveau,
	// le deuxieme niveau n'etant atteint que pour une derniere categorie du niveau 1
	// Au niveau 1: nTotalFrequency pour nValueNumber categories plus une categorie "SecondNiveau" si necessaire
	// (nSubValueNumber>0),
	//              acceuillant nSubTotalFrequency individus
	// Au niveau 2: nSubValueNumber pour nSubValueNumber categories
	void ComputeBestHierarchicalSamples(double dTotalFrequency, int nValueNumber, int nSubValueNumber,
					    DoubleVector* dvFrequencies, DoubleVector* dvSubFrequencies) const;

	// Meilleur echantillon et vecteur de probabilite base sur la serie de Basel (sum(1/k^2)=pi^2/6, p(k)=6/(pi^2
	// k^2)) jusqu'a un index maximal
	void ComputeBestBaselSample(double dTotalFrequency, int nMaxIndex, DoubleVector* dvFrequencies) const;
	void ComputeBaselProbs(int nMaxIndex, DoubleVector* dvProbs) const;
	double ComputeBaselProbAt(int nIndex) const;
	double ComputeBaselCodingLengthAt(int nIndex) const;

	// Meilleur echantillon et vecteur de probabilite base sur le codage naturel des nombre entiers (Rissanen)
	// jusqu'a un index maximal
	void ComputeBestNaturalNumbersUniversalPriorSample(double dTotalFrequency, int nMaxIndex,
							   DoubleVector* dvFrequencies) const;
	void ComputeNaturalNumbersUniversalPriorProbs(int nMaxIndex, DoubleVector* dvProbs) const;

	// Meilleur echantillon pour un produit de deux distributions multinomiales
	// Le resultat est un tableau d'effectif indexes par les index de chaque vecteur de proba initial,
	// ne contenant que les effectifs non nuls
	void ComputeBestProductSample(double dTotalFrequency, const DoubleVector* dvProbs1,
				      const DoubleVector* dvProbs2, ObjectArray* oaIndexedFrequencies) const;

	// Meilleur echantillon pour un produit de K distributions multinomiales
	// Le resultat est un tableau d'effectif indexes par les index de chaque vecteur de proba initial,
	// ne contenant que les effectifs non nuls
	void ComputeBestMultipleProductSample(double dTotalFrequency, const ObjectArray* oaProbVectors,
					      ObjectArray* oaIndexedFrequencies) const;

	// Meilleur echantillon pour une selection de K valeurs distinctes pour un vecteur de probabilite donne de
	// taille V >= K La loi est celle d'un tirage multinomial de selection de valeur selon p(Sel)=K!p1 p2 ... PK Le
	// resultats est un tableau d'effectif indexes par les index de chaque vecteur de proba initial, ne contenant
	// que les effectifs non nuls. Les index sont ordonnes selon le vecteur de probabilites initial
	void ComputeBestSelectionSample(double dTotalFrequency, int nSelectionSize, const DoubleVector* dvProbs,
					ObjectArray* oaIndexedFrequencies) const;

	/////////////////////////////////////////////////////////////////////////////////
	// Information sur les vecteur de probas et d'effectif

	// Proba (et info=-log(prob)) d'un vecteur d'effectif
	double ComputeFrequencyVectorProb(const DoubleVector* dvProbs, DoubleVector* dvFrequencies) const;
	double ComputeFrequencyVectorInfo(const DoubleVector* dvProbs, DoubleVector* dvFrequencies) const;

	// Verification d'un vecteur d'effectifs par rapport a effectif global et un vecteur de proba (sa taille)
	boolean CheckFrequencies(double dTotalFrequency, const DoubleVector* dvProbs,
				 const DoubleVector* dvFrequencies) const;

	// Verification d'un vecteur d'effectifs par rapport a effectif global
	boolean CheckFrequencyVector(double dTotalFrequency, const DoubleVector* dvFrequencies) const;

	// Verification d'un tableau d'effectifs indexes
	boolean CheckIndexedFrequencies(double dTotalFrequency, const ObjectArray* oaProbVectors,
					ObjectArray* oaIndexedFrequencies) const;

	// Verification d'un vecteur de proba
	boolean CheckProbVector(const DoubleVector* dvProbs) const;

	// Verification d'un vecteur de proba partiel (la somme des proba peut etre plus petite que 1)
	boolean CheckPartialProbVector(const DoubleVector* dvProbs) const;

	// Test si un effectif est tres important (au dela de la limite de la precision numerique)
	static boolean IsVeryLargeFrequency(double dFrequency);

	// Affichage d'un vecteur de probabilites
	void WriteProbVector(const DoubleVector* dvProbs, ostream& ost) const;

	// Affichage d'un vecteur d'effectif
	void WriteFrequencyVector(const DoubleVector* dvFrequencies, ostream& ost) const;

	// Affichage d'un tableau d'effectifs indexes
	void WriteIndexedFrequencyArray(const ObjectArray* oaIndexedFrequencies, ostream& ost) const;

	// Test de la classe
	static void Test();
	static void TestMultinomial(int nMaxTotalFrequency, const DoubleVector* dvProbs);
	static void TestEquidistributed(int nMaxTotalFrequency, int nValueNumber);
	static void TestHierarchical(int nMaxTotalFrequency, int nValueNumber, int nSubValueNumber);
	static void TestProduct(int nMaxTotalFrequency, const DoubleVector* dvProbs1, const DoubleVector* dvProbs2);
	static void TestProducts(int nMaxTotalFrequency, int nDimNumber, const DoubleVector* dvProbs);
	static void TestSelection(int nMaxTotalFrequency, int nSelectionSize, const DoubleVector* dvProbs);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Estimation heuristique de l'echantillon plus probable pour un effectif donne
	// sur la base des parties entieres superieures des effectifs estimes par valeur
	void ComputeBestCeilSample(double dTotalFrequency, const DoubleVector* dvProbs,
				   DoubleVector* dvFrequencies) const;

	// Post-optimisation d'un echantillon, en deplacant un effectif frequent vers une autre valeur
	void PostOptimizeSample(double dTotalFrequency, const DoubleVector* dvProbs, DoubleVector* dvFrequencies) const;

	// Recherche recursive des probas a retenir pour le calcul du meilleur echantillon de la loi produit
	// Le tableau oaSortedProbVectors contient pour chaque dimension les probas de la dimension triees
	// de facon decroissantes, et memorisee sous la forme de tableaux de KWSortableValue(Index, Prob)
	// Les resultats sont retournes sous la forme d'effectifs indexes, avec les index et les probas
	// correctement initialises, mais les effectifs restant a 0
	void ComputeBestMultipleProductProbsAt(double dTotalFrequency, const ObjectArray* oaSortedProbVectors,
					       int nCurrentDim, double dLargestProb,
					       KDIndexedFrequency* currentIndexedFrequency,
					       SortedList* slIndexedFrequencies) const;

	// Recherche recursive des probas a retenir pour le calcul du meilleur echantillon de la loi de selection
	// Similaire a la methode precedente, en prenant la meme loi de probabilite pour toutes les
	// dimensions et en evitant les permutation d'index entre dimensions
	// Les vecteur d'index en sortie sont ordonnes selon les valeur d'index, pour avoir un representant unique
	void ComputeBestSelectionProbsAt(double dTotalFrequency, int nSelectionSize, ObjectArray* oaSortedProbs,
					 int nCurrentDim, int nStartIndex, double dLargestProb,
					 KDIndexedFrequency* currentIndexedFrequency,
					 SortedList* slIndexedFrequencies) const;

	// Verification que tous les index d'un effectif indexe sont differents
	boolean CheckDifferentIndexes(const KDIndexedFrequency* indexedFrequency) const;

	// Perturbation aleatoire infinetimale d'une valeur reelle
	double EpsilonPerturbation(double dProb) const;

	// Perturbation aleatoire infinetimale d'une probabilite, avec respect des bornes sur [0; 1]
	double EpsilonProbPerturbation(double dProb) const;

	// Verification d'un vecteur de proba
	// Si bComplete=false, la somme des proba doit seulement etre plus petite que 1
	boolean InternalCheckProbVector(const DoubleVector* dvProbs, boolean bComplete) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDIndexedFrequency
// Effectif d'un tirage avec indexation d'une multi-valeurs pour un produit de distribution multinomiales
class KDIndexedFrequency : public Object
{
public:
	// Constructeur
	KDIndexedFrequency();
	~KDIndexedFrequency();

	// Taille de l'index
	void SetIndexSize(int nValue);
	int GetIndexSize() const;

	// Valeur des index
	void SetIndexAt(int nDimension, int nIndex);
	int GetIndexAt(int nDimension) const;

	// Tri des index des dimensions, pour avoir une unicite du vecteur de dimensions;
	// independament de toute permutation
	void SortIndexes();

	// Probabilite pour cette multi-valeur
	void SetProb(double dValue);
	double GetProb() const;

	// Effectif
	void SetFrequency(double dValue);
	double GetFrequency() const;

	// Copie et duplication
	void CopyFrom(const KDIndexedFrequency* sourceIndexedFrequency);
	KDIndexedFrequency* Clone() const;

	// Verification de l'integrite
	boolean Check() const override;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	IntVector ivIndexes;
	double dProb;
	double dFrequency;
};

// Tri par proba decroissante
int KDIndexedFrequencyCompareProb(const void* elem1, const void* elem2);

////////////////////////////////////////////////
// Methodes en inline

inline boolean KDMultinomialSampleGenerator::IsVeryLargeFrequency(double dFrequency)
{
	require(dFrequency >= 0);
	return (dFrequency * DBL_EPSILON > 0.5);
}

inline KDIndexedFrequency::KDIndexedFrequency()
{
	dProb = 0;
	dFrequency = 0;
}

inline KDIndexedFrequency::~KDIndexedFrequency() {}

inline void KDIndexedFrequency::SetIndexSize(int nValue)
{
	require(nValue >= 0);
	ivIndexes.SetSize(nValue);
}

inline int KDIndexedFrequency::GetIndexSize() const
{
	return ivIndexes.GetSize();
}

inline void KDIndexedFrequency::SetIndexAt(int nDimension, int nIndex)
{
	require(0 <= nDimension and nDimension < GetIndexSize());
	require(nIndex >= 0);
	ivIndexes.SetAt(nDimension, nIndex);
}

inline int KDIndexedFrequency::GetIndexAt(int nDimension) const
{
	require(0 <= nDimension and nDimension < GetIndexSize());
	return ivIndexes.GetAt(nDimension);
}

inline void KDIndexedFrequency::SortIndexes()
{
	ivIndexes.Sort();
}

inline void KDIndexedFrequency::SetProb(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dProb = dValue;
}

inline double KDIndexedFrequency::GetProb() const
{
	return dProb;
}

inline void KDIndexedFrequency::SetFrequency(double dValue)
{
	require(dValue >= 0);
	require(dValue * DBL_EPSILON > 0.5 or fabs(dValue - floor(0.5 + dValue)) < 1e-5);
	dFrequency = dValue;
}

inline double KDIndexedFrequency::GetFrequency() const
{
	return dFrequency;
}

inline void KDIndexedFrequency::CopyFrom(const KDIndexedFrequency* sourceIndexedFrequency)
{
	require(sourceIndexedFrequency != NULL);

	ivIndexes.CopyFrom(&(sourceIndexedFrequency->ivIndexes));
	dProb = sourceIndexedFrequency->dProb;
	dFrequency = sourceIndexedFrequency->dFrequency;
}

inline KDIndexedFrequency* KDIndexedFrequency::Clone() const
{
	KDIndexedFrequency* cloneIndexedFrequency;

	cloneIndexedFrequency = new KDIndexedFrequency;
	cloneIndexedFrequency->CopyFrom(this);
	return cloneIndexedFrequency;
}
