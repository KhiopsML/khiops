// Copyright (c) 2023 Orange. All rights reserved.
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
// de la methode de discretisation des donnees en histogrammes
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
	// Memoire: les intervalles appartiennent a l'appele
	ObjectArray* GetIntervals();

	// Nombre d'intervalles vides d'un histogramme
	int ComputeEmptyIntervalNumber() const;

	// Nombre d'intervalles avec une seule instance d'un histogramme
	int ComputeSingletonIntervalNumber() const;

	// Nombre d'intervalles avec une seule valeur d'un histogramme
	int ComputeSingleValueIntervalNumber() const;

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

	// Test si un intervalle est de type peak
	boolean IsPeakIntervalAt(int nIntervalIndex) const;

	// Calcul de l'effectif total
	int ComputeTotalFrequency() const;

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

	// Indique si l'histograme est basique, avant l'application des heuristiques troncature ou de simplification
	void SetRaw(boolean bValue);
	boolean GetRaw() const;

	// Epsilon de troncature en sortie
	void SetTruncationEpsilon(double dValue);
	double GetTruncationEpsilon() const;

	// Nombre d'intervalles singuliers en moins par rapport a l'histogramme le plus fin
	void SetRemovedSingularIntervalsNumber(int nValue);
	int GetRemovedSingularIntervalsNumber() const;

	// Granularite: nombre d'intervalles elementaires a la granularite G
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

	// Indicateur Level
	double GetLevel() const;

	// Indicateur Level normalise
	double GetNormalizedLevel() const;

	// Temp de calcul
	void SetComputationTime(double dValue);
	double GetComputationTime() const;

	//////////////////////////////////////////////////////////////////////////////
	// Parametres de modelisation dans le cas des histogrammes a virgule flotante

	// Borne inf du domaine numerique, provenant des hyper-parametres
	void SetDomainLowerBound(Continuous cValue);
	Continuous GetDomainLowerBound() const;

	// Borne sup du domaine numerique, provenant des hyper-parametres
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

	////////////////////////////////////////////////////////
	// Divers

	// Nettoyage
	virtual void Clean();

	// Creation virtuelle d'un histogramme dans le bon type
	virtual MHHistogram* Create() const;

	// Copie
	virtual void CopyFrom(const MHHistogram* sourceHistogram);

	// Duplication
	virtual MHHistogram* Clone() const;

	// Ecriture sous forme d'un tableau de bins avec leurs caracteristiques
	virtual void WriteSummary(ostream& ost) const;
	void Write(ostream& ost) const override;
	void WriteFile(const ALString& sFileName) const;

	// Verification de l'integrite
	boolean Check() const override;

	// Verification qu'un intervalle est du bon type
	virtual boolean CheckIntervalType(const MHHistogramInterval* interval) const;

	// Verification de l'integrite des valeurs des intervalles (lower et upper value)
	boolean CheckValues(const ContinuousVector* cvValues) const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	///////////////////////////////////////////////////////
	// Notes sur l'implementation
	// La classe est specialisee pour les histogrammes MODL virgule flotante.
	// Mais elle comporte quelques methodes generiques virtuelles, permettant
	// de la specialiser si necessaire pour des methodes alternatives

	// Tableau des intervalles
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
	boolean bRaw;
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

	//////////////////////////////////////////////////////////////////
	// Informations avancees sur les intervalles, issues des algorithmes

	// Cout de l'intervalle localement a son sous-ensemble
	void SetCost(double dValue);
	double GetCost() const;

	//////////////////////////////////////////////////////////////////
	// Informations calculees sur les intervalles

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

	// Creation virtuelle d'un intervalle dans le bon type
	virtual MHHistogramInterval* Create() const;

	// Copie
	virtual void CopyFrom(const MHHistogramInterval* sourceInterval);

	// Duplication
	MHHistogramInterval* Clone() const;

	// Ecriture sous fomre d'un tableau de bins avec leurs caracteristiques
	virtual void WriteHeaderLineReport(ostream& ost) const;
	virtual void WriteLineReport(int nTotalFrequency, ostream& ost) const;

	// Ecriture, sans la probabilite et la densite
	void Write(ostream& ost) const override;

	// Verification de l'integrite des bornes uniquement
	boolean CheckBounds() const;

	// Verification de l'integrite
	boolean Check() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	Continuous cLowerBound;
	Continuous cUpperBound;
	int nFrequency;
	Continuous cLowerValue;
	Continuous cUpperValue;
	double dCost;
};
