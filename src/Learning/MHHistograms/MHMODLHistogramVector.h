// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class MHMODLHistogramVector;

#include "KWFrequencyVector.h"

///////////////////////////////////////////////////////////////////////////////////
// Table des intervalle d'un histogramme
// Sous-classe KWFrequencyTable pour les histogrammes, pour specialiser
// les classes de cout et de discretisation
class MHMODLHistogramTable : public KWFrequencyTable
{
public:
	// Constructeur
	MHMODLHistogramTable();
	~MHMODLHistogramTable();

	// Parametrage de l'exposant en puissance de 2 des central bin effectivement utilise
	void SetCentralBinExponent(int nValue);
	int GetCentralBinExponent() const;

	// Niveau de hierarchie des modeles granularises
	void SetHierarchyLevel(int nValue);
	int GetHierarchyLevel() const;

	// Longueur du plus petit bin elementaire
	// Chaque partile a une longueur multiple de cette largeur
	void SetMinBinLength(double dValue);
	double GetMinBinLength() const;

	// Bornes des intervalles extremes
	Continuous GetMinLowerBound() const;
	Continuous GetMaxUpperBound() const;

	// Affichage
	void Write(ostream& ost) const override;

	/////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Parametres pour les calculs de cout
	double dMinBinLength;
	int nCentralBinExponent;
	int nHierarchyLevel;
};

///////////////////////////////////////////////////////////////////////////////////
// Intervalle d'un histogramme
// Sous-classe KWFrequencyVector pour les histogrammes, pour specialiser
// les classes de cout et de discretisation
class MHMODLHistogramVector : public KWFrequencyVector
{
public:
	// Constructeur
	MHMODLHistogramVector();
	~MHMODLHistogramVector();

	// Createur, renvoyant une instance du meme type
	KWFrequencyVector* Create() const override;

	// Effectif de l'intervalle
	int GetFrequency() const;
	void SetFrequency(int nValue);

	// Borne inf de l'intervalle
	Continuous GetLowerBound() const;
	void SetLowerBound(Continuous cValue);

	// Borne sup de l'intervalle
	Continuous GetUpperBound() const;
	void SetUpperBound(Continuous cValue);

	// Longueur de l'intervalle
	Continuous GetLength() const;

	// Taille du vecteur d'effectif
	int GetSize() const override;

	// Copie a partir d'un vecteur source
	void CopyFrom(const KWFrequencyVector* kwfvSource) override;

	// Duplication (y compris du contenu)
	KWFrequencyVector* Clone() const override;

	// Calcul de l'effectif total
	int ComputeTotalFrequency() const override;

	// Rapport synthetique destine a rentrer dans une sous partie d'un tableau
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Effectif du bin
	int nFrequency;

	// Bornes de l'intervalle
	Continuous cLowerBound;
	Continuous cUpperBound;
};

///////////////////////
// Methodes en inline

inline int MHMODLHistogramVector::GetFrequency() const
{
	return nFrequency;
}

inline void MHMODLHistogramVector::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

inline Continuous MHMODLHistogramVector::GetLowerBound() const
{
	return cLowerBound;
}

inline void MHMODLHistogramVector::SetLowerBound(Continuous cValue)
{
	cLowerBound = cValue;
}

inline Continuous MHMODLHistogramVector::GetUpperBound() const
{
	return cUpperBound;
}

inline void MHMODLHistogramVector::SetUpperBound(Continuous cValue)
{
	cUpperBound = cValue;
}

inline Continuous MHMODLHistogramVector::GetLength() const
{
	return cUpperBound - cLowerBound;
}

inline int MHMODLHistogramVector::GetSize() const
{
	return 1;
}
