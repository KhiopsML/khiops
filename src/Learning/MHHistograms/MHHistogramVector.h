// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class MHHistogramVector;

#include "KWFrequencyVector.h"

///////////////////////////////////////////////////////////////////////////////////
// Intervalle d'un histogramme
// Sous-classe KWFrequencyVector pour les histogrammes, pour specialiser
// les classes de cout et de discretisation
class MHHistogramVector : public KWFrequencyVector
{
public:
	// Constructeur
	MHHistogramVector();
	~MHHistogramVector();

	// Createur, renvoyant une instance du meme type
	KWFrequencyVector* Create() const override;

	// Effectif de l'intervalle
	int GetFrequency() const;
	void SetFrequency(int nValue);

	// Longueur de l'intervalle, en nombre de bins elementaires
	int GetLength() const;
	void SetLength(int nValue);

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

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Effectif du bin
	int nFrequency;

	// Largeur du bin
	int nLength;
};

///////////////////////
// Methodes en inline

inline int MHHistogramVector::GetFrequency() const
{
	return nFrequency;
}

inline void MHHistogramVector::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

inline int MHHistogramVector::GetLength() const
{
	return nLength;
}

inline void MHHistogramVector::SetLength(int nValue)
{
	require(nValue >= 0);
	nLength = nValue;
}

inline int MHHistogramVector::GetSize() const
{
	return 1;
}
