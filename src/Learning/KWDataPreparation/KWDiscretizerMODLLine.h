// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWMODLLineMerge;
class KWMODLLineSplit;
class KWMODLLine;
class KWMODLLineOptimization;
class KWMODLLineDeepOptimization;
class KWMODLLineOptimalDiscretization;

#include "KWFrequencyVector.h"

////////////////////////////////////////////////////////////////////////////
// Classe de travail pour l'algorithme de discretisation MODL,
// permettant de representer un intervalle (ligne de tableau de contingence)
// et memoriser les informations de fusion de deux intervalles (Merge) ou
// de coupure d'un intervalle en deux (Split)
// La classe KWMODLLineOptimization est une classe legere ne gerant qu'un
// seul type d'optimisation (Merge).
// La classe KWMODLLineDeepOptimization est une classe plus complete (et
// egalement plus couteuse en memoire) permettant de gerer trois types
// d'optimisation par intervalles: Split, MergeSplit et MergeMergeSplit.
// L'ensemble des classes de travail est parametre dans son constructeur par
// un objet generique de creation de vecteur d'effectif

/////////////////////////////////////////////////////////////////////
// Evaluation de la fusion de deux ligne d'un tableau de contingence
// pour l'algorithme MODL
class KWMODLLineMerge : public Object
{
public:
	// Constructeur
	KWMODLLineMerge(const KWFrequencyVector* kwfvFrequencyVectorCreator);
	~KWMODLLineMerge();

	// Vecteur de comptage des effectifs de la ligne fusionnee
	KWFrequencyVector* GetFrequencyVector();

	// Cout d'une ligne suite a une fusion
	void SetCost(double dValue);
	double GetCost() const;

	// Variation de cout suite a une fusion
	void SetDeltaCost(double dValue);
	double GetDeltaCost() const;

	// Position dans une liste triee
	void SetPosition(POSITION pos);
	POSITION GetPosition() const;

	// Recopie
	void CopyFrom(KWMODLLineMerge* lineMerge);

	// Rapport synthetique destine a rentrer dans un tableau (sans retour a la ligne)
	void WriteHeaderLineReport(ostream& ost) const;
	void WriteLineReport(ostream& ost) const;

	// Display
	void Write(ostream& ost) const override;

	///////////////////// Implementation ///////////////////////////
protected:
	KWFrequencyVector* kwfvFrequencyVector;
	double dCost;
	double dDeltaCost;
	POSITION position;
};

/////////////////////////////////////////////////////////////////////
// Evaluation de la coupure d'une ligne d'un tableau de contingence
// pour l'algorithme MODL
// Une ligne de tableau de contingence est decrite par son vecteur de comptage
// d'effectifs et l'index de sa derniere ligne dans le tableau de contingence initial
// Pour decrire la coupure, il suffit de decrire la premiere des lignes, la seconde
// s'en deduisant simplement.
class KWMODLLineSplit : public Object
{
public:
	// Constructeur
	KWMODLLineSplit(const KWFrequencyVector* kwfvFrequencyVectorCreator);
	~KWMODLLineSplit();

	// Vecteur de comptage des effectifs de la premiere sous-partie de la ligne coupee
	KWFrequencyVector* GetFirstSubLineFrequencyVector();

	// Index de la derniere ligne du tableau de contingence initial
	// constituant la premiere sous-partie de la ligne coupee. Cela correspond
	// a l'index de coupure.
	void SetFirstSubLineIndex(int nValue);
	int GetFirstSubLineIndex() const;

	// Cout de la premiere sous-ligne
	void SetFirstSubLineCost(double dValue);
	double GetFirstSubLineCost() const;

	// Cout de la seconde sous-ligne
	void SetSecondSubLineCost(double dValue);
	double GetSecondSubLineCost() const;

	// Variation de cout suite a une coupure
	void SetDeltaCost(double dValue);
	double GetDeltaCost() const;

	// Position dans une liste triee
	void SetPosition(POSITION pos);
	POSITION GetPosition() const;

	// Recopie
	void CopyFrom(KWMODLLineSplit* lineSplit);

	// Rapport synthetique destine a rentrer dans un tableau (sans retour a la ligne)
	void WriteHeaderLineReport(ostream& ost) const;
	void WriteLineReport(ostream& ost) const;

	// Display
	void Write(ostream& ost) const override;

	///////////////////// Implementation ///////////////////////////
protected:
	KWFrequencyVector* kwfvFirstSubLineFrequencyVector;
	int nFirstSubLineIndex;
	double dFirstSubLineCost;
	double dSecondSubLineCost;
	double dDeltaCost;
	POSITION position;
};

/////////////////////////////////////////////////////////////////////
// Ligne de tableau de contingence pour l'algorithme MODL
class KWMODLLine : public Object
{
public:
	// Constructeur
	KWMODLLine(const KWFrequencyVector* kwfvFrequencyVectorCreator);
	~KWMODLLine();

	// Creator, renvoyant une instance du meme type
	virtual KWMODLLine* Create() const;

	// Recopie
	void CopyFrom(KWMODLLine* line);

	/////////////////////////////////////////////////////////////////////
	// Donnees standard d'une ligne de contingence

	// Vecteur de comptage des effectifs
	KWFrequencyVector* GetFrequencyVector();

	// Index de la derniere ligne du tableau de contingence initial
	// constituant la ligne courante
	// L'index de la premiere ligne du tableau de contingence initial est
	// deduit de la ligne precedente (index 0 si pas de ligne precedente)
	void SetIndex(int nValue);
	int GetIndex() const;

	// Acces a la ligne de contingence precedente
	KWMODLLine* GetPrev() const;
	void SetPrev(KWMODLLine* line);

	// Acces a la ligne de contingence suivante
	KWMODLLine* GetNext() const;
	void SetNext(KWMODLLine* line);

	// Cout de la ligne
	double GetCost() const;
	void SetCost(double dValue);

	// Nombre de modalites representees par une ligne
	void SetModalityNumber(int nValue);
	int GetModalityNumber() const;

	// Position dans une liste triee des lignes par nombre de modalites
	void SetPosition(POSITION pos);
	POSITION GetPosition() const;

	// Rapport synthetique destine a rentrer dans un tableau (sans retour a la ligne)
	virtual void WriteHeaderLineReport(ostream& ost) const;
	virtual void WriteLineReport(ostream& ost) const;

	// Display
	void Write(ostream& ost) const override;

	///////////////////// Implementation ///////////////////////////
protected:
	KWFrequencyVector* kwfvFrequencyVector;
	int nIndex;
	KWMODLLine* prevLine;
	KWMODLLine* nextLine;
	double dCost;
	POSITION position;
};

/////////////////////////////////////////////////////////////////////
// Ligne de tableau de contingence pour l'algorithme MODL,
// pour une optimisation basee sur une fusion ascendante des ligne
class KWMODLLineOptimization : public KWMODLLine
{
public:
	// Constructeur
	KWMODLLineOptimization(const KWFrequencyVector* kwfvFrequencyVectorCreator);
	~KWMODLLineOptimization();

	// Creation
	KWMODLLine* Create() const;

	// Evaluation d'une fusion entre une ligne et sa ligne prededente
	KWMODLLineMerge* GetMerge();

	// Rapport synthetique destine a rentrer dans un tableau (sans retour a la ligne)
	void WriteHeaderLineReport(ostream& ost) const;
	void WriteLineReport(ostream& ost) const;

	///////////////////// Implementation ///////////////////////////
protected:
	KWMODLLineMerge merge;
};

/////////////////////////////////////////////////////////////////////
// Ligne de tableau de contingence pour l'algorithme MODL,
// pour une post-optimisation poussee basee sur des Split, MergeSplit
// et MergeMergeSplit
class KWMODLLineDeepOptimization : public KWMODLLine
{
public:
	// Constructeur
	KWMODLLineDeepOptimization(const KWFrequencyVector* kwfvFrequencyVectorCreator);
	~KWMODLLineDeepOptimization();

	// Creation
	KWMODLLine* Create() const;

	// Evaluation d'une coupure d'un intervalle en deux
	KWMODLLineSplit* GetSplit();

	// Evaluation d'un changement de borne entre un intervalle et son predecesseur
	// (Merge des deux intervalles, suivi d'un Split pour trouver le point de
	// coupure optimal)
	KWMODLLineSplit* GetMergeSplit();

	// Evaluation d'une fusion de trois intervalles (intervalle courant et ses deux
	// predecesseurs) suivi d'un Split
	KWMODLLineSplit* GetMergeMergeSplit();

	// Rapport synthetique destine a rentrer dans un tableau (sans retour a la ligne)
	void WriteHeaderLineReport(ostream& ost) const;
	void WriteLineReport(ostream& ost) const;

	///////////////////// Implementation ///////////////////////////
protected:
	KWMODLLineSplit mergeSplit;
	KWMODLLineSplit split;
	KWMODLLineSplit mergeMergeSplit;
};

/////////////////////////////////////////////////////////////////////
// Discretisation optimale de la fin des lignes d'une table de contingence
// pour l'algorithme MODL, pour accueillir une discretisation optimale
// de la fin de la table en K intervalles. Ceete classe est utilisee
// dans l'algorithme de recherche de discretisation optimal MODL, base
// sur la programmation dynamique
// On herite des de KWMODLLine pour beneficier de ses service (bien qu'ici,
// une structure en tableau aurait ete aussi bien adaptee qu'une structure
// en liste chainee)
class KWMODLLineOptimalDiscretization : public KWMODLLine
{
public:
	// Constructeur
	KWMODLLineOptimalDiscretization(const KWFrequencyVector* kwfvFrequencyVectorCreator);
	~KWMODLLineOptimalDiscretization();

	// Creation
	KWMODLLine* Create() const;

	// Nombre d'intervalles pris en compte
	int GetIntervalNumber() const;
	void SetIntervalNumber(int nValue);

	// Nombre  max d'intervalles pris en compte
	// Cet attribut permet d'optimiser la gestion de la memorisation des index
	// des intervalles, d'une part en memoire en allouant que ce qui est necessaire
	// (une fois pour toutes au depart), d'autre part en temps de calcul en
	// evitant l'evaluation des solutions ayant trop d'intervalles
	int GetMaxIntervalNumber() const;
	void SetMaxIntervalNumber(int nValue);

	// Cout des lignes de la discretisation (somme des cout des intervalles,
	// sans le cout global du modele)
	double GetDiscretizationCost() const;
	void SetDiscretizationCost(double dValue);

	// Index des dernieres lignes des intervalles
	int GetLastLineIndexAt(int nIntervalIndex) const;
	void SetLastLineIndexAt(int nIntervalIndex, int nLineIndex);

	// Rapport synthetique destine a rentrer dans un tableau (sans retour a la ligne)
	void WriteHeaderLineReport(ostream& ost) const;
	void WriteLineReport(ostream& ost) const;

	///////////////////// Implementation ///////////////////////////
protected:
	int nIntervalNumber;
	double dDiscretizationCost;
	IntVector ivLastLineIndexes;
};

///////////////////////////////////////////////////////////////////////
// Methodes en inline

// Classe KWMODLLineMerge

inline KWMODLLineMerge::KWMODLLineMerge(const KWFrequencyVector* kwfvFrequencyVectorCreator)
{
	require(kwfvFrequencyVectorCreator != NULL);
	dCost = 0;
	dDeltaCost = 0;
	position = NULL;
	kwfvFrequencyVector = kwfvFrequencyVectorCreator->Create();
}

inline KWMODLLineMerge::~KWMODLLineMerge()
{
	delete kwfvFrequencyVector;
}

inline KWFrequencyVector* KWMODLLineMerge::GetFrequencyVector()
{
	return kwfvFrequencyVector;
}

inline void KWMODLLineMerge::SetCost(double dValue)
{
	dCost = dValue;
}

inline double KWMODLLineMerge::GetCost() const
{
	return dCost;
}

inline void KWMODLLineMerge::SetDeltaCost(double dValue)
{
	dDeltaCost = dValue;
}

inline double KWMODLLineMerge::GetDeltaCost() const
{
	return dDeltaCost;
}

inline void KWMODLLineMerge::SetPosition(POSITION pos)
{
	position = pos;
}

inline POSITION KWMODLLineMerge::GetPosition() const
{
	return position;
}

/////////////////////////////////////////////////////////////////////
// Classe KWMODLLineSplit

inline KWMODLLineSplit::KWMODLLineSplit(const KWFrequencyVector* kwfvFrequencyVectorCreator)
{
	require(kwfvFrequencyVectorCreator != NULL);
	nFirstSubLineIndex = 0;
	dFirstSubLineCost = 0;
	dSecondSubLineCost = 0;
	dDeltaCost = 0;
	position = 0;
	kwfvFirstSubLineFrequencyVector = kwfvFrequencyVectorCreator->Create();
}

inline KWMODLLineSplit::~KWMODLLineSplit()
{
	delete kwfvFirstSubLineFrequencyVector;
}

inline KWFrequencyVector* KWMODLLineSplit::GetFirstSubLineFrequencyVector()
{
	return kwfvFirstSubLineFrequencyVector;
}

inline void KWMODLLineSplit::SetFirstSubLineIndex(int nValue)
{
	require(nValue >= 0);
	nFirstSubLineIndex = nValue;
}

inline int KWMODLLineSplit::GetFirstSubLineIndex() const
{
	return nFirstSubLineIndex;
}

inline void KWMODLLineSplit::SetFirstSubLineCost(double dValue)
{
	dFirstSubLineCost = dValue;
}

inline double KWMODLLineSplit::GetFirstSubLineCost() const
{
	return dFirstSubLineCost;
}

inline void KWMODLLineSplit::SetSecondSubLineCost(double dValue)
{
	dSecondSubLineCost = dValue;
}

inline double KWMODLLineSplit::GetSecondSubLineCost() const
{
	return dSecondSubLineCost;
}

inline void KWMODLLineSplit::SetDeltaCost(double dValue)
{
	dDeltaCost = dValue;
}

inline double KWMODLLineSplit::GetDeltaCost() const
{
	return dDeltaCost;
}

inline void KWMODLLineSplit::SetPosition(POSITION pos)
{
	position = pos;
}

inline POSITION KWMODLLineSplit::GetPosition() const
{
	return position;
}

/////////////////////////////////////////////////////////////////////
// Classe KWMODLLine

inline KWMODLLine::KWMODLLine(const KWFrequencyVector* kwfvFrequencyVectorCreator)
{
	require(kwfvFrequencyVectorCreator != NULL);
	nIndex = 0;
	prevLine = NULL;
	nextLine = NULL;
	dCost = 0;
	kwfvFrequencyVector = kwfvFrequencyVectorCreator->Create();
	position = NULL;
}

inline KWMODLLine::~KWMODLLine()
{
	delete kwfvFrequencyVector;
}

inline KWMODLLine* KWMODLLine::Create() const
{
	return new KWMODLLine(kwfvFrequencyVector);
}

inline KWFrequencyVector* KWMODLLine::GetFrequencyVector()
{
	return kwfvFrequencyVector;
}

inline void KWMODLLine::SetIndex(int nValue)
{
	require(nIndex >= 0);
	nIndex = nValue;
}

inline int KWMODLLine::GetIndex() const
{
	return nIndex;
}

inline KWMODLLine* KWMODLLine::GetPrev() const
{
	return prevLine;
}

inline void KWMODLLine::SetPrev(KWMODLLine* line)
{
	prevLine = line;
}

inline KWMODLLine* KWMODLLine::GetNext() const
{
	return nextLine;
}

inline void KWMODLLine::SetNext(KWMODLLine* line)
{
	nextLine = line;
}

inline double KWMODLLine::GetCost() const
{
	return dCost;
}

inline void KWMODLLine::SetCost(double dValue)
{
	dCost = dValue;
}

inline void KWMODLLine::SetModalityNumber(int nModality)
{
	require(nModality >= 1);
	kwfvFrequencyVector->SetModalityNumber(nModality);
}

inline int KWMODLLine::GetModalityNumber() const
{
	return kwfvFrequencyVector->GetModalityNumber();
}

inline void KWMODLLine::SetPosition(POSITION pos)
{
	position = pos;
}
inline POSITION KWMODLLine::GetPosition() const
{
	return position;
}

// Classe KWMODLLineOptimization

inline KWMODLLineOptimization::KWMODLLineOptimization(const KWFrequencyVector* kwfvFrequencyVectorCreator)
    : KWMODLLine(kwfvFrequencyVectorCreator), merge(kwfvFrequencyVectorCreator)
{
}

inline KWMODLLineOptimization::~KWMODLLineOptimization() {}

inline KWMODLLine* KWMODLLineOptimization::Create() const
{
	return new KWMODLLineOptimization(kwfvFrequencyVector);
}

inline KWMODLLineMerge* KWMODLLineOptimization::GetMerge()
{
	return &merge;
}

// Classe KWMODLLineDeepOptimization

inline KWMODLLineDeepOptimization::KWMODLLineDeepOptimization(const KWFrequencyVector* kwfvFrequencyVectorCreator)
    : KWMODLLine(kwfvFrequencyVectorCreator), mergeSplit(kwfvFrequencyVectorCreator), split(kwfvFrequencyVectorCreator),
      mergeMergeSplit(kwfvFrequencyVectorCreator)
{
}

inline KWMODLLineDeepOptimization::~KWMODLLineDeepOptimization() {}

inline KWMODLLine* KWMODLLineDeepOptimization::Create() const
{
	return new KWMODLLineDeepOptimization(kwfvFrequencyVector);
}

inline KWMODLLineSplit* KWMODLLineDeepOptimization::GetSplit()
{
	return &split;
}

inline KWMODLLineSplit* KWMODLLineDeepOptimization::GetMergeSplit()
{
	return &mergeSplit;
}

inline KWMODLLineSplit* KWMODLLineDeepOptimization::GetMergeMergeSplit()
{
	return &mergeMergeSplit;
}
