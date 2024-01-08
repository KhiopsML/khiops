// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class MHHistogram;
class MHHistogramInterval;

#include "Object.h"
#include "KWContinuous.h"
#include "FileService.h"
#include "KWStat.h"

//////////////////////////////////////////////////////////
// Classe de description d'un histogramme resultat
// de la meyhode de discretisation des donnees en histogrammes
class MHHistogram : public Object
{
public:
	// Constructeur
	MHHistogram();
	~MHHistogram();

	////////////////////////////////////////////////////////
	// Specification des intervalles

	// Nombre d'intervalles
	int GetIntervalNumber() const;

	// Acces a un intervalle
	MHHistogramInterval* GetIntervalAt(int nIndex);
	const MHHistogramInterval* GetConstIntervalAt(int nIndex) const;

	// Acces direct aux intervalles, pour edition et consultation
	// Memoire: les intervalles appartiennet a l'appele
	ObjectArray* GetIntervals();

	// Nombre d'intervalles vides d'un histogramme
	int ComputeEmptyIntervalNumber() const;

	// Nombre d'intervalles avec une seule instance d'un histogramme
	int ComputeSingletonIntervalNumber() const;

	// Nombre d'intervalles avec une seule valeur d'un histogramme
	int ComputeSingleValueIntervalNumber() const;

	// Nombre d'intervalles PICH d'un histogramme
	int ComputePICHIntervalNumber() const;

	// Nombre d'intervalles singuliers d'un histogramme
	int ComputeSingularIntervalNumber() const;

	// Test si un intervalle d'un histogramme est singulier: unique valeur, entoure de deux intervalles vides
	boolean IsSingularIntervalAt(int nIntervalIndex) const;

	// Nombre d'intervalles de type spike: unique valeur, entoure de deux intervalles moins denses
	int ComputeSpikeIntervalNumber() const;

	// Test si un intervalle est de type spike
	boolean IsSpikeIntervalAt(int nIntervalIndex) const;

	// Nombre d'intervalles de type peak: intervalle non vide, entoure de deux intervalles moins denses
	int ComputePeakIntervalNumber() const;

	// Test si un intervalle est de type spike
	boolean IsPeakIntervalAt(int nIntervalIndex) const;

	// Calcul de l'effectif total
	int ComputeTotalFrequency() const;

	// Calcul du nombre total de bins
	int ComputeTotalBinLength() const;

	// Calcul du nombre total de bins dans le cas d'histogrammes construits selon l'heuristique OMH
	// L'agregation de sous-histogramme peut en effet declencher le depassement de la limite des entiers
	// On utilise egalement cette methode dans les cas de la representation a virgule flotante
	double ComputeLargeScaleTotalBinLength() const;

	// Nombre de valeurs distinctes
	void SetDistinctValueNumber(int nValue);
	int GetDistinctValueNumber() const;

	// Valeur min et max
	Continuous GetMinValue() const;
	Continuous GetMaxValue() const;

	////////////////////////////////////////////////////////
	// Specification des informations de modele et de cout

	// Critere de construction des histogramme
	void SetHistogramCriterion(const ALString& sValue);
	const ALString& GetHistogramCriterion() const;

	// Index de simplification de l'histogramme en sortie
	void SetCoarseningIndex(int nValue);
	int GetCoarseningIndex() const;

	// Epsilon de troncature en sortie
	void SetTruncationEpsilon(double dValue);
	double GetTruncationEpsilon() const;

	// Nombre d'intervalles singuliers en moins par rapport a l'histogramme le plus fin
	void SetRemovedSingularIntervalsNumber(int nValue);
	int GetRemovedSingularIntervalsNumber() const;

	// Granularite: nombre d'intervalles elementairtes a la granularite G
	void SetGranularity(int nValue);
	int GetGranularity() const;

	// Cout du model null
	void SetNullCost(double dValue);
	double GetNullCost() const;

	// Cout de reference du model null dans [1,2], dans les cas des histogrammes a virgule flotante
	void SetReferenceNullCost(double dValue);
	double GetReferenceNullCost() const;

	// Cout du modele
	void SetCost(double dValue);
	double GetCost() const;

	// Cout de partition dans le cout du modele
	void SetPartitionCost(double dValue);
	double GetPartitionCost() const;

	// Cout du modele null granularise
	double GetGranularizedNullCost() const;

	// Cout du modele granularise
	double GetGranularizedCost() const;

	// Indicateur Level
	double GetLevel() const;

	// Indicateur Level granularise
	double GetGranularizedLevel() const;

	// Temp de calcul
	void SetComputationTime(double dValue);
	double GetComputationTime() const;

	//////////////////////////////////////////////////////////////////////////////
	// Parametres de modelisation dans le cas des histogrammes a virgule flotante

	// Borne inf du domaine numerique, provenant des hyper-parametres
	void SetDomainLowerBound(Continuous cValue);
	Continuous GetDomainLowerBound() const;

	// Borne inf du domaine numerique, provenant des hyper-parametres
	void SetDomainUpperBound(Continuous cValue);
	Continuous GetDomainUpperBound() const;

	// Nombre de bits de mantisse utilises pour coder les mantissa bins des boresndu domaine
	void SetDomainBoundsMantissaBitNumber(int nValue);
	int GetDomainBoundsMantissaBitNumber() const;

	// Niveau de hierarchie, exposant de la puissance de 2
	void SetHierarchyLevel(int nValue);
	int GetHierarchyLevel() const;

	// Niveau de reference de la racine de la hierarchie par rapport au niveau des main bins, en nombre de bins
	void SetMainBinHierarchyRootLevel(int nValue);
	int GetMainBinHierarchyRootLevel() const;

	// Niveau max de hierarchie
	void SetMaxHierarchyLevel(int nValue);
	int GetMaxHierarchyLevel() const;

	// Niveau max de la hierarchie plus sur
	void SetMaxSafeHierarchyLevel(int nValue);
	int GetMaxSafeHierarchyLevel() const;

	// Exposant en puissance de 2 des central bins, ne pouvant contenir que la valeur 0 (0 si pas de central bin)
	void SetCentralBinExponent(int nValue);
	int GetCentralBinExponent() const;

	// Exposant minimum des central bins
	void SetMinCentralBinExponent(int nValue);
	int GetMinCentralBinExponent() const;

	// Exposant maximum des central bins
	void SetMaxCentralBinExponent(int nValue);
	int GetMaxCentralBinExponent() const;

	// Nombre de main bins, exponent ou central bins, necessaires pour couvrir toutes les valeurs
	void SetMainBinNumber(int nValue);
	int GetMainBinNumber() const;

	// Specification de la longuer du plus petit bin dans le cas des histogrammes a virgule flottante
	void SetMinBinLength(double dValue);
	double GetMinBinLength() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Parametres de modelisation dans le cas des histogrammes avec gestion des outliers

	// Nombre de sous ensemble de donnees impliques dans l'heuristique des gestion des outliers
	int GetOutlierDataSubsetNumber() const;

	// Nombre de sous ensemble de donnees impliques dans l'heuristique des getsion des outliers
	int GetOutlierBoundaryIntervalNumber() const;

	////////////////////////////////////////////////////////
	// Divers

	// Nettoyage
	void Clean();

	// Copie
	void CopyFrom(const MHHistogram* sourceHistogram);

	// Duplication
	MHHistogram* Clone() const;

	// Ecriture sous forme d'un tableau de bins avec leurs caracteristiques
	void Write(ostream& ost) const override;
	void WriteFile(const ALString& sFileName) const;

	// Verification de l'integrite
	boolean Check() const;

	// Verification de l'integrite des valeurs des intervalles (lower et upper value)
	boolean CheckValues(const ContinuousVector* cvValues) const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	ObjectArray oaIntervals;
	int nDistinctValueNumber;

	// Parametres de modelisation generique
	double dTruncationEpsilon;
	int nRemovedSingularIntervalsNumber;
	int nGranularity;
	double dNullCost;
	double dReferenceNullCost;
	double dCost;
	double dPartitionCost;
	double dComputationTime;

	// Parametres de modelisation pour les histogrammes a virgule flotante
	ALString sHistogramCriterion;
	Continuous cDomainLowerBound;
	Continuous cDomainUpperBound;
	int nDomainBoundsMantissaBitNumber;
	int nCoarseningIndex;
	int nHierarchyLevel;
	int nMainBinHierarchyRootLevel;
	int nMaxHierarchyLevel;
	int nMaxSafeHierarchyLevel;
	int nCentralBinExponent;
	int nMinCentralBinExponent;
	int nMaxCentralBinExponent;
	int nMainBinNumber;
	double dMinBinLength;
};

//////////////////////////////////////////////////////////
// Classe de description d'un interval d'histogramme
class MHHistogramInterval : public Object
{
public:
	// Constructeur
	MHHistogramInterval();
	~MHHistogramInterval();

	////////////////////////////////////////////////////////////////
	// Definition de l'intervalle

	// Borne inferieure
	void SetLowerBound(Continuous cValue);
	Continuous GetLowerBound() const;

	// Borne superieure
	void SetUpperBound(Continuous cValue);
	Continuous GetUpperBound() const;

	// Effectif
	void SetFrequency(int nValue);
	int GetFrequency() const;

	//////////////////////////////////////////////////////////////////
	// Informations sur les valeurs ayant ete utilisees pour definir
	// les intervalles (les valeurs sont potentiellement hors intervalle)

	// Valeur inferieure
	void SetLowerValue(Continuous cValue);
	Continuous GetLowerValue() const;

	// Valeur superieure
	void SetUpperValue(Continuous cValue);
	Continuous GetUpperValue() const;

	// Test s'il y a une seule valeur
	boolean IsSingleValue() const;

	// Longueur en nombre de epsilon bins
	void SetBinLength(int nValue);
	int GetBinLength() const;

	//////////////////////////////////////////////////////////////////
	// Information avancee sur les intervalles, issues des algorithmes

	// Index du sous-ensemble utilise pour construire l'histogramme
	// dans le cas de l'heuristique de gestion des outliers
	// On utilise 0 si un seul ensemble a ete utilise globalement,
	// sinon un index superieur ou egal a 1
	void SetDataSubsetIndex(int nValue);
	int GetDataSubsetIndex() const;

	// Indique qu'il s'agit d'un intervalle frontiere avec le sous-ensemble precedent
	void SetPreviousDataSubsetBoundary(boolean bValue);
	boolean GetPreviousDataSubsetBoundary() const;

	// Indique qu'il s'agit d'un intervalle provenant d'un sous-ensemble PICH
	void SetPICH(boolean bValue);
	boolean GetPICH() const;

	// Index de decoupage de l'ensemble PICH a l'origine de l'intervalle
	void SetPICHSplitIndex(int nValue);
	int GetPICHSplitIndex() const;

	// Cout de l'intervalle localement a son sous-ensemble
	void SetCost(double dValue);
	double GetCost() const;

	//////////////////////////////////////////////////////////////////
	// Information calculees sur les intervalles

	// Longeur de l'intervalle, basee sur ses bornes
	Continuous GetLength() const;

	// Probabilite
	double GetProbability(int nTotalFrequency) const;

	// Densite
	double GetDensity(int nTotalFrequency) const;

	////////////////////////////////////////////////////////
	// Divers

	// Comparaison des intervalle sur leur densite en prenant en compte les valeurs et non les bornes
	int CompareDensity(const MHHistogramInterval* otherInterval) const;

	// Copie
	void CopyFrom(const MHHistogramInterval* sourceInterval);

	// Duplication
	MHHistogramInterval* Clone() const;

	// Ecriture sous fomre d'un tableau de bins avec leurs caracteristiques
	void WriteHeaderLineReport(ostream& ost) const;
	void WriteLineReport(int nTotalFrequency, ostream& ost) const;

	// Ecriture, sans la probabilite et la densite
	void Write(ostream& ost) const override;

	// Verification de l'integrite des bornes uniquement
	boolean CheckBounds() const;

	// Verification de l'integrite
	boolean Check() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	Continuous cLowerBound;
	Continuous cUpperBound;
	int nFrequency;
	int nBinLength;
	Continuous cLowerValue;
	Continuous cUpperValue;
	int nDataSubsetIndex;
	boolean bPreviousDataSubsetBoundary;
	boolean bPICH;
	int nPICHSplitIndex;
	double dCost;
};
