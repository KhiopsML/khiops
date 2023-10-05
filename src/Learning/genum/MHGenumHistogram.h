// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class MHGenumHistogram;
class MHGenumHistogramInterval;

#include "MHHistogram.h"

//////////////////////////////////////////////////////////
// Classe de description d'un histogramme Genum resultat
class MHGenumHistogram : public MHHistogram
{
public:
	// Constructeur
	MHGenumHistogram();
	~MHGenumHistogram();

	////////////////////////////////////////////////////////
	// Specification des intervalles

	// Cout du modele null granularise
	double GetGranularizedNullCost() const;

	// Cout du modele granularise
	double GetGranularizedCost() const;

	// Indicateur Level granularise
	double GetGranularizedLevel() const;

	// Calcul du nombre total de bins
	int ComputeTotalBinLength() const;

	// Nombre d'intervalles PICH d'un histogramme
	int ComputePICHIntervalNumber() const;

	// Calcul du nombre total de bins dans le cas d'histogrammes construits selon l'heuristique OMH
	// L'agregation de sous-histogramme peut en effet declencher le depassement de la limite des entiers
	// On utilise egalement cette methode dans les cas de la representation a virgule flotante
	double ComputeLargeScaleTotalBinLength() const;

	////////////////////////////////////////////////////////
	// Specification des informations de modele et de cout

	//////////////////////////////////////////////////////////////////////////////
	// Parametres de modelisation dans le cas des histogrammes a virgule flotante

	/////////////////////////////////////////////////////////////////////////////////////
	// Parametres de modelisation dans le cas des histogrammes avec gestion des outliers

	// Nombre de sous ensemble de donnees impliques dans l'heuristique des gestion des outliers
	int GetOutlierDataSubsetNumber() const;

	// Nombre de sous ensemble de donnees impliques dans l'heuristique des getsion des outliers
	int GetOutlierBoundaryIntervalNumber() const;

	////////////////////////////////////////////////////////
	// Divers

	// Nettoyage
	void Clean() override;

	// Creation virtuelle d'un histogramme dans le bon type
	MHHistogram* Create() const override;

	// Copie
	void CopyFrom(const MHHistogram* sourceHistogram) override;

	// Ecriture sous forme d'un tableau de bins avec leurs caracteristiques
	void WriteSummary(ostream& ost) const override;

	// Verification qu'un intervalle est du bon type
	boolean CheckIntervalType(const MHHistogramInterval* interval) const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
};

//////////////////////////////////////////////////////////
// Classe de description d'un interval d'histogramme Genum
class MHGenumHistogramInterval : public MHHistogramInterval
{
public:
	// Constructeur
	MHGenumHistogramInterval();
	~MHGenumHistogramInterval();

	////////////////////////////////////////////////////////////////
	// Definition de l'intervalle

	//////////////////////////////////////////////////////////////////
	// Informations sur les valeurs ayant ete utilisees pour definir
	// les intervalles (les valeurs sont potentiellement hors intervalle)

	//////////////////////////////////////////////////////////////////
	// Informations avancees sur les intervalles, issues des algorithmes

	// Longueur en nombre de epsilon bins
	void SetBinLength(int nValue);
	int GetBinLength() const;

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

	//////////////////////////////////////////////////////////////////
	// Information calculees sur les intervalles

	////////////////////////////////////////////////////////
	// Divers

	// Creation virtuelle d'un intervalle dans le bon type
	virtual MHHistogramInterval* Create() const override;

	// Copie
	void CopyFrom(const MHHistogramInterval* sourceInterval) override;

	// Ecriture sous fomre d'un tableau de bins avec leurs caracteristiques
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(int nTotalFrequency, ostream& ost) const override;

	// Ecriture, sans la probabilite et la densite
	void Write(ostream& ost) const override;

	// Verification de l'integrite
	boolean Check() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs specifiques
	int nBinLength;
	int nDataSubsetIndex;
	boolean bPreviousDataSubsetBoundary;
	boolean bPICH;
	int nPICHSplitIndex;
};
