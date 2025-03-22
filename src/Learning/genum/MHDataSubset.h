// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWContinuous.h"
#include "MHGenumHistogramVector.h"

//////////////////////////////////////////////////////////
// Classe de gestion d'un sous-ensemble de donnees
// Classe utilitaire pour l'algorithme de gestion des
// outlier dans la construction des histogrammes
class MHDataSubset : public Object
{
public:
	// Constructeur
	MHDataSubset();
	~MHDataSubset();

	// Valeurs de l'ensemble de de donnees dont on est un sous-ensemble
	// Memoire: appartient a l'appelant
	void SetDatasetValues(const ContinuousVector* cvValues);
	const ContinuousVector* GetDatasetValues() const;

	//////////////////////////////////////////////////////////////////
	// Specification de la sous-partie des valeurs du sus-ensemble

	// Index de la premiere valeur du sous-ensemble, valeur comprise
	void SetFirstValueIndex(int nValue);
	int GetFirstValueIndex() const;

	// Index de la dernier valeur du sous-ensemble, valeur non comprise
	void SetLastValueIndex(int nValue);
	int GetLastValueIndex() const;

	//////////////////////////////////////////////////////////////////
	// Acces a quelque caracteristiques de base du sous-ensemble

	// Taille du sous ensemble
	int GetSize() const;

	// Premiere et derniere valeur du sous ensemble valeurs comprises
	Continuous GetFirstValue() const;
	Continuous GetLastValue() const;

	// Test si l'ensemble est un singleton ne contenant qu'une seule valeur
	boolean IsSingleton() const;

	//////////////////////////////////////////////////////////////////
	// Acces aux caracteristiques de l'heuristique de gestion des outliers

	// Memorisation de l'indicateur PWCH du sous-ensemble
	void SetPWCH(boolean bValue);
	boolean GetPWCH() const;

	// Memorisation de l'index de decoupage dans le cas d'un sous-ensemble PICH (0 sinon)
	void SetPICHSplitIndex(int nValue);
	int GetPICHSplitIndex() const;

	// Memorisation du status terminal precedent du sous-ensemble,
	// s'il ne peut pas etre fusionne avec le sous-ensemble precedent
	// sans perte de precision (par exemple: PWCH+PWCH->PICH)
	void SetTerminalPrev(boolean bValue);
	boolean GetTerminalPrev() const;

	// Memorisation du status terminal suivant du sous-ensemble,
	// s'il ne peut pas etre fusionne avec le sous-ensemble suivany
	// sans perte de precision (par exemple: PWCH+PWCH->PICH)
	void SetTerminalNext(boolean bValue);
	boolean GetTerminalNext() const;

	// Status terminal si le sous ensemble est terminal precdent et suivant
	boolean GetTerminal() const;

	//////////////////////////////////////////////////////////////////
	// Acces aux histogrammes construits pour la gestion des outliers

	// Histogramme pour le sous-ensemble
	// Memoire: appartient a l'appele
	void SetHistogram(const KWFrequencyTable* histogramFrequencyTable);
	const KWFrequencyTable* GetHistogram() const;

	// Histogramme pour le sous-ensemble correspondant a l'union du dernier intervalle
	// de l'histogramme courant et du premier intervalle de l'histogramme suivant
	// Memoire: appartient a l'appele
	void SetPrevBoundaryHistogram(const KWFrequencyTable* histogramFrequencyTable);
	const KWFrequencyTable* GetPrevBoundaryHistogram() const;

	// Supression ou destruction des histogrammes
	void RemoveHistograms();
	void DeleteHistograms();

	////////////////////////////////////////////////////////
	// Divers

	// Copie et duplication
	void CopyFrom(const MHDataSubset* aSource);
	MHDataSubset* Clone() const;

	// Ecriture
	void WriteHeaderLine(ostream& ost) const;
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	const ContinuousVector* cvDatasetValues;
	int nFirstValueIndex;
	int nLastValueIndex;
	boolean bPWCH;
	int nPICHSplitIndex;
	boolean bTerminalPrev;
	boolean bTerminalNext;
	const KWFrequencyTable* histogram;
	const KWFrequencyTable* prevBoundaryHistogram;
};
