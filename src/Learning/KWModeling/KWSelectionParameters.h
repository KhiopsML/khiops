// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KWSelectionParameters
//    Selection parameters
class KWSelectionParameters : public Object
{
public:
	// Constructeur
	KWSelectionParameters();
	~KWSelectionParameters();

	// Copie et duplication
	void CopyFrom(const KWSelectionParameters* aSource);
	KWSelectionParameters* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Max number of selected variables
	int GetMaxSelectedAttributeNumber() const;
	void SetMaxSelectedAttributeNumber(int nValue);

	// Selection criterion
	const ALString& GetSelectionCriterion() const;
	void SetSelectionCriterion(const ALString& sValue);

	// Optimization algorithm
	const ALString& GetOptimizationAlgorithm() const;
	void SetOptimizationAlgorithm(const ALString& sValue);

	// Optimization level
	int GetOptimizationLevel() const;
	void SetOptimizationLevel(int nValue);

	// Prior weight (expert)
	double GetPriorWeight() const;
	void SetPriorWeight(double dValue);

	// Prior exponent (expert)
	double GetPriorExponent() const;
	void SetPriorExponent(double dValue);

	// Construction cost (expert)
	boolean GetConstructionCost() const;
	void SetConstructionCost(boolean bValue);

	// Preparation cost (expert)
	boolean GetPreparationCost() const;
	void SetPreparationCost(boolean bValue);

	// Trace level
	int GetTraceLevel() const;
	void SetTraceLevel(int nValue);

	// Trace selected variables
	boolean GetTraceSelectedAttributes() const;
	void SetTraceSelectedAttributes(boolean bValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	//////////////////////////////////////////////////////////////////////////////
	// Description fine des parametres
	//
	// MaxSelectedAttributeNumber
	//  Apres optimisation, la liste des attributs selectionnees est tronquees en supprimant
	//  les moins importants (selon le critere multivarie, puis sur le critere univarie)
	//  Par defaut: 0 (signifie pas de maximum)
	//
	// SelectionCriterion
	//  Les regularisations se font avec une penalisation sur le choix des attributs
	//  Valeurs possibles:
	//   CMA: integration de la regularisation MAP par une esperance (utilisant les infos des modeles)
	//	 MA: integration de la regularisation MAP par une esperance (utilisant les proba des modeles)
	//	 MAP: regularisation basee sur l'estimation des probabilites conditionnelles du SNB
	//
	// OptimizationAlgorithm
	//  Par defaut: MS_FFWBW
	//  Valeurs possible:
	//   MS_FFWBW: Multi-Start de gloutons de Fast Backward Forward
	//   OPT: Algorithme optimale de recherche exhaustive
	//	 FW: Forward
	//	 FWBW: Forward Backward
	//	 FFW: Fast Forward
	//	 FFWBW: Fast Backward Forward
	//
	// OptimizationLevel
	//	MS_FFWBW: nombre de gloutons total (y compris le glouton initial)
	//			  si niveau 0: on prend log2(N)
	//	Autres algorithmes: sans effet
	//
	// TraceLevel
	//  0: pas de trace
	//  1: seul les optimaux locaux
	//  2: toutes les ameliorations
	//  3: toutes les evaluations
	//
	// TraceSelectedAttributes
	//  Affichage de la selection des attributs dans la trace (uniquement si TraceLevel > 0)

	// Verification des parametres
	boolean CheckMaxSelectedAttributeNumber(int nValue) const;
	boolean CheckSelectionCriterion(const ALString& sValue) const;
	boolean CheckOptimizationAlgorithm(const ALString& sValue) const;
	boolean CheckOptimizationLevel(int nValue) const;
	boolean CheckTraceLevel(int nValue) const;

	// Verification globale
	boolean Check() const override;

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	int nMaxSelectedAttributeNumber;
	ALString sSelectionCriterion;
	ALString sOptimizationAlgorithm;
	int nOptimizationLevel;
	double dPriorWeight;
	double dPriorExponent;
	boolean bConstructionCost;
	boolean bPreparationCost;
	int nTraceLevel;
	boolean bTraceSelectedAttributes;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline int KWSelectionParameters::GetMaxSelectedAttributeNumber() const
{
	return nMaxSelectedAttributeNumber;
}

inline void KWSelectionParameters::SetMaxSelectedAttributeNumber(int nValue)
{
	nMaxSelectedAttributeNumber = nValue;
}

inline const ALString& KWSelectionParameters::GetSelectionCriterion() const
{
	return sSelectionCriterion;
}

inline void KWSelectionParameters::SetSelectionCriterion(const ALString& sValue)
{
	sSelectionCriterion = sValue;
}

inline const ALString& KWSelectionParameters::GetOptimizationAlgorithm() const
{
	return sOptimizationAlgorithm;
}

inline void KWSelectionParameters::SetOptimizationAlgorithm(const ALString& sValue)
{
	sOptimizationAlgorithm = sValue;
}

inline int KWSelectionParameters::GetOptimizationLevel() const
{
	return nOptimizationLevel;
}

inline void KWSelectionParameters::SetOptimizationLevel(int nValue)
{
	nOptimizationLevel = nValue;
}

inline double KWSelectionParameters::GetPriorWeight() const
{
	return dPriorWeight;
}

inline void KWSelectionParameters::SetPriorWeight(double dValue)
{
	dPriorWeight = dValue;
}

inline double KWSelectionParameters::GetPriorExponent() const
{
	return dPriorExponent;
}

inline void KWSelectionParameters::SetPriorExponent(double dValue)
{
	dPriorExponent = dValue;
}

inline boolean KWSelectionParameters::GetConstructionCost() const
{
	return bConstructionCost;
}

inline void KWSelectionParameters::SetConstructionCost(boolean bValue)
{
	bConstructionCost = bValue;
}

inline boolean KWSelectionParameters::GetPreparationCost() const
{
	return bPreparationCost;
}

inline void KWSelectionParameters::SetPreparationCost(boolean bValue)
{
	bPreparationCost = bValue;
}

inline int KWSelectionParameters::GetTraceLevel() const
{
	return nTraceLevel;
}

inline void KWSelectionParameters::SetTraceLevel(int nValue)
{
	nTraceLevel = nValue;
}

inline boolean KWSelectionParameters::GetTraceSelectedAttributes() const
{
	return bTraceSelectedAttributes;
}

inline void KWSelectionParameters::SetTraceSelectedAttributes(boolean bValue)
{
	bTraceSelectedAttributes = bValue;
}

// ## Custom inlines

// ##
