// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class MHFloatingPointFrequencyTableBuilder;

#include "KWContinuous.h"
#include "MHContinuousLimits.h"
#include "Vector.h"
#include "KWStat.h"
#include "MHMODLHistogramVector.h"
#include "MHMODLHistogramCost.h"
#include "MHMODLHistogramVector.h"

//////////////////////////////////////////////////////////////////////////////
// Service  de calcul d'intervalles en representation virgule flottantes
// Les intervalles en virgule flottantes se divisent en:
//   . exponent bins: une plage de puissance de 2,  ]-2^(i+1, -2^i] ou ]2^i, 2^(i+1)]
//   . central bins: de 0 a une puissance de 2, ]-2^i, 0] ou ]0, 2^i]
//   . mantissa bin: sous intervalle d'un exponent bin ou central bin, de largeur relative 2^(-i)
class MHFloatingPointFrequencyTableBuilder : public Object
{
public:
	// Constructeur
	MHFloatingPointFrequencyTableBuilder();
	~MHFloatingPointFrequencyTableBuilder();

	//////////////////////////////////////////////////////////////////////////
	// Initialisation et calcul de statistiques sur les valeurs a traiter

	// Calcul a partir d'une liste de bins elementaires tries ([lower value, upper value], frequency)
	// On a trois vecteurs en entree:
	//  . cvSourceBinLowerValues
	//  . cvSourceBinUpperValues
	//  . ivSourceBinFrequencies
	// Si cvSourceBinLowerValues=NULL, on est dans le cas d'une initialisation par valeur et on
	// considere les valeurs inf egales aux valeurs sup
	// Si ivSourceBinFrequencies=NULL, on utilie un effectif de 1 partout
	//
	// Permet d'acceder a tous les services
	// Memoire: les valeurs appartiennent a l'appelant
	virtual void InitializeBins(const ContinuousVector* cvSourceBinLowerValues,
				    const ContinuousVector* cvSourceBinUpperValues,
				    const IntVector* ivSourceBinFrequencies);

	// Test si initialise
	boolean IsInitialized() const;

	// Nombre de bins elementaires en entree
	// Les bins elementaires peuvent etre des intervalles, ou etre reduits a une seule valeur
	int GetBinNumber() const;

	// Acces aux bornes d'un bin elementaire ]lower bound, upper bound]
	// Dans le cas d'une inialialisation par valeur, les deux methodes renvoient la meme valeur
	Continuous GetBinLowerValueAt(int nBinIndex) const;
	Continuous GetBinUpperValueAt(int nBinIndex) const;

	// Effectif d'une valeur ou d'un intervalle
	int GetBinFrequencyAt(int nBinIndex) const;

	// Valeur pour un index d'effectif donne, entre 0 et TotalFrequency-1
	Continuous GetBinLowerValueAtFrequency(int nFrequencyIndex) const;
	Continuous GetBinUpperValueAtFrequency(int nFrequencyIndex) const;

	// Nettoyage
	virtual void Clean();

	//////////////////////////////////////////////////////////////////////////
	// Acces aux statistiques sur les valeurs a traiter, apres initialisation

	// Effectif total
	int GetTotalFrequency() const;

	// Nombre de valeurs distinctes
	// Il s'agit d'un minorant dans le cas d'un initialisation par bins de longueur strictement positive
	int GetDistinctValueNumber() const;

	// Valeurs extremes
	Continuous GetMinValue() const;
	Continuous GetMaxValue() const;

	// Valeurs negatives et positives les plus proches de 0 non nulle la plus petite (0 si aucune valeur negative ou
	// positive)
	Continuous GetMaxNegativeValue() const;
	Continuous GetMinPositiveValue() const;

	// Exposant en puissance de 2 associe aux valeurs extremes (0 si valeur extreme nulle)
	int GetMinValueExponent() const;
	int GetMaxValueExponent() const;

	// Minimum de l'exposant en puissance de 2 des central bin, ne pouvant contenir que la valeur 0
	// Il s'agit du plus petit exposant utilisable, compatible avec les donnees, permettant
	// d'obtenir une representation dediee virgule flottante
	int GetMinCentralBinExponent() const;

	// Maximum de l'exposant en puissance de 2 des central bin, ne pouvant contenir que la valeur 0
	// Il s'agit du plus grand exposant utilisable, compatible avec les donnees, permettant
	// d'obtenir une representation dediee equal-width
	int GetMaxCentralBinExponent() const;

	// Valeur min et max des central bin exponent a utiliser pour l'optimisation
	virtual int GetMinOptimizedCentralBinExponent() const;
	virtual int GetMaxOptimizedCentralBinExponent() const;

	////////////////////////////////////////////////////////////////////////////////////////////
	// Acces au choix de la representation, compromis entre la representation virgule flottante
	// avec un maximum d'exponent bins et la representation equal-width avec uniquement
	// des central bins
	// Ce choix est optimise lors de l'apprentissage

	// Parametrage de l'exposant en puissance de 2 des central bin effectivement utilise
	// Cet exposant doit etre compris entre MinCentralBinExponent pour une representation accee sur les bins virgule
	// flottante et max(MinValueExponent, MaxValueExponent) pour une representation accee sur les bins equal-width
	virtual void SetCentralBinExponent(int nValue);
	int GetCentralBinExponent() const;

	// Nombre de main bins, exponent ou central bins, necessaires pour couvrir toutes les valeurs
	int GetMainBinNumber() const;

	// Index du main bin contenant 0 (-1 si aucun)
	int GetZeroMainBinIndex() const;

	// Informations sur chaque main bin
	void GetMainBinSpecAt(int nIndex, int& nSign, int& nExponent, boolean& bIsCentralBin) const;

	// Bornes d'un main bin
	void GetMainBinBoundsAt(int nIndex, Continuous& cLowerBound, Continuous& cUpperBound) const;

	// Niveau de reference de la racine de la hierarchie par rapport au niveau des main bins, en nombre de bins
	// Un niveau 0 correspond a un seul main bin pleinement utilise
	// Un niveau positif signifie que 2^h main bin sont necessaires pour couvrir le domaine numerique
	// Un niveau negatif signifie qu'une fraction 2^h de l'unique main bin est suffisante pour couvrir le domaine
	// numerique au moyen de mantissa bins
	int GetMainBinHierarchyRootLevel() const;

	// Nombre total de bin selon le niveau de la hierarchie, entre 1 a la racine et au plus 2^HierarchyLevel
	// Le nombre de bins effectif peut etre plus petit que 2^HierarchyLevel, si le nombre de main bin
	// n'est pas une puissance de deux ou si les valeur min et max ne sont pas sur des frontieres de main bins
	// Les bins ayant une frontiere avec la singuarite ]-EpsilonValue,EpsilonValue[ autour de 0 sont traites
	// de facon speciale
	virtual int GetTotalBinNumberAt(int nHierarchyLevel) const;

	// Bornes inf et sup des premiers intervalles de l'histogramme, calculees lors de l'optimisation des
	// hyper-parametres
	Continuous GetDomainLowerBound() const;
	Continuous GetDomainUpperBound() const;

	// Nombre de bits de mantisse utilises pour coder les mantissa bins des bornes du domaine
	int GetDomainBoundsMantissaBitNumber() const;

	// Longueur du plus petit bin
	// Il s'agit de la longueur du mantissa bin au niveau maximum de la hierarchie, dans le main bin d'exposant le
	// plus petit
	double GetMinBinLength() const;

	// Niveau max de hierarchie, qui peut etre inferieur au nombre max total si l'on est sur une plage de valeur
	// trop petite par rapport a la precision de la mantisse
	int GetMaxHierarchyLevel() const;

	// Niveau max de la hierarchie plus sur, si l'on veut eviter d'etre trop pret du seuil
	// ou les nombre a virgule flotante sont de nature discrete
	int GetMaxSafeHierarchyLevel() const;

	// Nombre minimum de valeurs distinctes encodable par bin elementaire au niveau le plus fin (environ sqrt(n)
	// ln(n)) Cela contribue a limiter le nombre de bits de mantisse utilises, pour eviter d'etre trop pret du seuil
	// ou les nombre a virgule flotante sont de nature discrete
	int GetMinDistinctValueNumberPerBin() const;

	// Nombre max de niveau de hierarchie
	static int GetDefaultTotalHierarchyLevel();

	//////////////////////////////////////////////////////////////////////////
	// Calcul d'une table d'intervalles en representation virgule flottante

	// Calcul d'une table d'intervalles en representation virgule flottante
	// Le niveau de hierarchie est une puissance de 2 entre 0 et GetMaxHierarchyLevel()
	// Le central bin exponent utilise est celui courant accessible par GetCentralBinExponent()
	// La singularite autour de 0 (dans ]-EpsilonValue, EpsilonValue[) est traitee en fusionnant
	// intervalles successif dont la frontiere est une frontiere de la singularite en 0
	void BuildFrequencyTable(int nHierarchyLevel, KWFrequencyTable*& histogramFrequencyTable) const;

	// Calcul d'une table d'intervalles en representation virgule flottante pour le modele nul
	void BuildNulFrequencyTable(KWFrequencyTable*& histogramFrequencyTable) const;

	// Calcul des bornes d'un intervalle de type floating-point bin autour d'une valeur
	// pour un niveau de hierarchie donne, exprime en nombre de bits
	// Au niveau 0, on englobe tout le jeu de donnee, puis on exploite une
	// hierarchie binaire des floating-point bins tant qu'on est au dessus
	// du niveau des exponent bins ou central bins. Et enfin, on redecoupe
	// chaque exponent bin par puissance de 2 sur la base de mantissa bins.
	// Quand les bornes d'un floating-point bin traversent la singularité autour de zero,
	// elles sont ajustees de facon a etre soit 0, soit une frontiere de la singularite en 0
	virtual void ExtractFloatingPointBinBounds(Continuous cValue, int nHierarchyBitNumber, Continuous& cLowerBound,
						   Continuous& cUpperBound) const;

	// Acces a l'index de main bin pour une valeur donnees
	int GetMainBinIndex(Continuous cValue) const;

	///////////////////////////////////////////////////////////////////////////
	// Services globaux de gestion des parties mantisse et exposant de la representation virgule flottante
	// et de gestion des floatting-point bins:
	//   . central bins: ]-2^e, 0] ou ]0, 2^e]
	//   . exponent bins: ]-2^(e+1), -2^e] ou ]2^e, 2^(e+1)]
	//   . mantissa bins: sous-bin d'un central bin ou exponent bin de largeur relative 2^(-m)
	// Les mantissa bins doivent contenir toutes les valeurs (sauf 0), et les central bins ne peuvent contenir que
	// la valeur 0. Attention, les representations usuelles sont symetriques autour de zero:
	//   . la representation de la norme IEEE suppose que l'on a des mantisses dans ]-2, -1] ou [1, 2[
	//   . la representation donnee par la fonction ANSI C frexp fournit des mantisses normalisees dans ]-1, -0.5]
	//   ou [0.5, 1[
	// Ici, on souhaite constituer des intervalles tous de la meme forme, ouverts a gauche et fermes a droite.
	// On fournit des mantisses dans ]-2, -1] ou ]1, 2], en ajustant l'exposant correspondant
	// Par exemple, la valeur 1 correspond a une mantisse de 2 pour une exposant de -1.

	// Extraction de la representation virgule flotante: renvoie la mantisse et initialise l'exposant
	static Continuous ExtractMantissa(Continuous cValue, int& nExponent);

	// Extraction de l'index du mantissa bin d'un main bin contenant une valeur, pour un nombre de bit de mantisse
	// donne L'exposant du central bin est necessaire
	static longint ExtractMantissaBinInMainBinIndex(Continuous cValue, int nMantissaBitNumber,
							int nCentralExponent);

	// Extraction de l'index du mantissa bin d'un exponent bin contenant une valeur, pour un nombre de bit de
	// mantisse donne Pour la valeur 0, on renvoie 0
	static longint ExtractMantissaBinInExponentBinIndex(Continuous cValue, int nMantissaBitNumber);

	// Extraction de l'index du mantissa bin d'un central bin contenant une valeur, pour un nombre de bit de
	// mantisse donne L'exposant du central bin est necessaire
	static longint ExtractMantissaBinInCentralBinIndex(Continuous cValue, int nMantissaBitNumber,
							   int nCentralExponent);

	// Calcul la valeur a partir de la representation virgule flottante
	static Continuous BuildValue(Continuous cMantissa, int nExponent);

	// Calcul des bornes d'un mantissa bin autour d'une valeur d'un main bin,
	// en precisant le nombre de bits de precision de la mantisse et
	// l'exposant du central bin est celui courant specifie
	// Si ce nombre de digit vaut 0, on retrouve un main bin
	static void ExtractMantissaMainBinBounds(Continuous cValue, int nMantissaBitNumber, int nCentralExponent,
						 int& nExponent, Continuous& cLowerBound, Continuous& cUpperBound);

	// Calcul des bornes d'un mantissa bin autour d'une valeur d'un exponent bin,
	// en precisant le nombre de bits de precision de la mantisse
	// Si ce nombre de digit vaut 0, on retrouve un exponent bin
	// Si la valeur vaut 0, on renvoie ]-1, 1]
	static void ExtractMantissaExponentBinBounds(Continuous cValue, int nMantissaBitNumber, int& nExponent,
						     Continuous& cLowerBound, Continuous& cUpperBound);

	// Calcul des bornes d'un mantissa bin autour d'une valeur appartenant a un central bin ]-2^e, 0] ou ]0, 2^e],
	// en precisant le nombre de bits de precision de la mantisse
	// Si ce nombre de digit vaut 0, on retrouve un central bin
	static void ExtractMantissaCentralBinBounds(Continuous cValue, int nMantissaBitNumber, int nCentralExponent,
						    Continuous& cLowerBound, Continuous& cUpperBound);

	// Calcul des bornes d'un exponent bin
	static void BuildExponentBinBounds(int nSign, int nExponent, Continuous& cLowerBound, Continuous& cUpperBound);

	// Calcul des bornes d'un central bin
	static void BuildCentralBinBounds(int nSign, int nExponent, Continuous& cLowerBound, Continuous& cUpperBound);

	// Nombre max de bits de precision d'un mantissa bin
	static int GetMaxMantissaBinBitNumber();

	// Exposants minimum et maximum des exponent bins et central bins
	static int GetMinBinExponent();
	static int GetMaxBinExponent();

	///////////////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage
	void Write(ostream& ost) const;

	// Ecriture des bornes des bins principaux
	void WriteMainBins(ostream& ost) const;

	// Ecriture des nombre totaux de bins par niveau de hierarchie
	void WriteHierarchyTotalBinNumbers(ostream& ost) const;

	// Ecriture d'une frequency table
	void WriteFrequencyTable(KWFrequencyTable* histogramFrequencyTable, ostream& ost) const;

	// Test d'integrite
	boolean Check() const override;

	// Comptage du nombre de valeurs distinctes d'un vectur de valeur trie
	static int ComputeDistinctValueNumber(const ContinuousVector* cvValues);

	// Methode de test
	static void Test();

	// Test pour un vecteur de valeurs
	static void TestWithValues(const ALString& sLabel, const ContinuousVector* cvValues);

	// Etude de l'optimisation des bornes du domaine de valeurs
	static void StudyDomainBoundsOptimization(ContinuousVector* cvValues);

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Acces en friend pour la sous-classe dediee troncature
	friend class MHDiscretizerTruncationMODLHistogram;

	// Recherche de l'index d'un bin pour un effctif cumule donne
	int SearchBinIndex(int nSearchedCumulativeFrequency) const;

	/////////////////////////////////////////////////////////////////////////////
	// Sous-partie de la methode InitializeValues definie en methodes virtuelles
	// pour pouvoir etre specialisees

	// Valeurs extremes systemes, utilises pour les assertions
	virtual Continuous GetSystemMinValue() const;
	virtual Continuous GetSystemMaxValue() const;

	// Initialisation des bornes du domaine, appele en fin de la methode InitializeValues
	virtual void InitializeDomainBounds();

	// Indique si les bornes ont ete initialisee
	boolean AreDomainBoundsInitialized() const;

	// Mise a jour du niveau de hierarchie max "sur", en tenant compte des limites de la precision numerique,
	// appele a la fin de SetCentralBinExponent
	virtual void UpdateMaxSafeHierarchyLevel();

	//////////////////////////////////////////////////////////////////////////
	// Methodes de test

	// Initialisation d'un vecteur de valeurs de test
	static void InitializeTestValues(ContinuousVector* cvValues);

	// Initialisation d'un vecteur de valeurs uniformes, entre une borne min, max, plus un epsilon autour de 0 si
	// min < 0 < max et l'indication de generet ou non la valeur 0
	static void InitializeUniformValues(ContinuousVector* cvValues, Continuous cMin, Continuous cMax,
					    Continuous cEpsilon, boolean bZero, int nValueNumber);

	// Initialisation d'un vecteur de valeurs gaussiennes (m=0, v=1)
	static void InitializeGaussianValues(ContinuousVector* cvValues, int nValueNumber);

	// Etude de la representation virgule flottante
	static void StudyFloatingPointRepresentation(ContinuousVector* cvValues);

	//////////////////////////////////////////////////////////////////////////
	// Attributs de la classe

	// Description generiques des bins en entrees, de type (]lower bound, upper bound], frequency)
	// Dans le cas de valeurs plutot que d'intervalles, on passe utilise lower bound= upper bound
	ContinuousVector cvBinLowerValues;
	ContinuousVector cvBinUpperValues;
	IntVector ivBinFrequencies;

	// Vecteur des effectifs cumules
	IntVector ivBinCumulatedFrequencies;

	// Statistiques sur les valeurs en entrees
	int nDistinctValueNumber;
	Continuous cMinValue;
	Continuous cMaxValue;
	Continuous cMaxNegativeValue;
	Continuous cMinPositiveValue;

	// Parametres des floating-point bins
	Continuous cDomainLowerBound;
	Continuous cDomainUpperBound;
	int nMinValueExponent;
	int nMaxValueExponent;
	int nMinCentralBinExponent;
	int nCentralBinExponent;
	int nMainBinNumber;
	int nZeroMainBinIndex;
	int nMainBinHierarchyRootLevel;
	int nMaxHierarchyLevel;
	int nMaxSafeHierarchyLevel;
	int nDomainBoundsMantissaBitNumber;
	double dMinBinLength;
};