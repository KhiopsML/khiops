// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWSortableIndex;
class KWSortableValue;
class KWSortableValueSymbol;
class KWSortableContinuous;
class KWSortableContinuousSymbol;
class KWSortableSymbol;
class KWSortableFrequencySymbol;
class KWSortableObject;

#include "KWContinuous.h"
#include "KWSymbol.h"
#include "Object.h"

////////////////////////////////////////////////////////////////////////
// Bibliotheque de classes dediees au tri
// La plupart des classes sont des objet caracterises par une valeur
// (a trier) et un index (permettant de gerer la correspondance entre
// une position avant et apres le tri)
// Les objet une fois instancies peuvent etre ranges dans un tableau
// a parametrer par un fonction de comparaison

////////////////////////////////////////////////////////////////////////
// Classe ancetre des objet de tri d'un index
// On donne ici la possibilite de trier directement les index
class KWSortableIndex : public Object
{
public:
	// Constructeur
	KWSortableIndex();
	~KWSortableIndex();

	// Index, permettant d'etablir la correspondance entre les
	// positions avant et apres le tri
	void SetIndex(int nValue);
	int GetIndex() const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	int nIndex;
};

// Comparaison de deux objets KWSortableIndex
int KWSortableIndexCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////
// Objet a trier par valeur relle
class KWSortableValue : public KWSortableIndex
{
public:
	// Constructeur
	KWSortableValue();
	~KWSortableValue();

	// Critere de tri
	void SetSortValue(double dValue);
	double GetSortValue() const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	double dSortValue;
};

// Comparaison de deux objets KWSortableValue
int KWSortableValueCompare(const void* elem1, const void* elem2);
int KWSortableValueCompareDecreasing(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////
// Classe permettant le tri d'un ensemble d'objets pour une valeur de tri
// donnees. En cas d'egalite, la valeur (contenu) du symbole est utilisee.
class KWSortableValueSymbol : public KWSortableValue
{
public:
	// Constructeur
	KWSortableValueSymbol();
	~KWSortableValueSymbol();

	// Symbole
	void SetSymbol(const Symbol& sValue);
	Symbol& GetSymbol() const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	mutable Symbol sSymbol;
};

// Comparaison de deux objets KWSortableValueSymbol
int KWSortableValueSymbolCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////
// Objet a trier par valeur continue
class KWSortableContinuous : public KWSortableIndex
{
public:
	// Constructeur
	KWSortableContinuous();
	~KWSortableContinuous();

	// Critere de tri
	void SetSortValue(Continuous cValue);
	Continuous GetSortValue() const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Continuous cSortValue;
};

// Comparaison de deux objets KWSortableContinuous
int KWSortableContinuousCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////
// Objet a trier par valeur continue
// En cas d'egalite, la valeur (contenu) du symbole est utilisee.
class KWSortableContinuousSymbol : public KWSortableContinuous
{
public:
	// Constructeur
	KWSortableContinuousSymbol();
	~KWSortableContinuousSymbol();

	// Symbole
	void SetSymbol(const Symbol& sValue);
	Symbol& GetSymbol() const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	mutable Symbol sSymbol;
};

// Comparaison de deux objets KWSortableContinuousSymbol
int KWSortableContinuousSymbolCompare(const void* elem1, const void* elem2);

// Comparaison de deux objets KWSortableContinuousSymbol par valeur de symbol, puis valeur de tri
int KWSortableContinuousSymbolCompareSymbolValue(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////
// Objet a trier par Symbol
class KWSortableSymbol : public KWSortableIndex
{
public:
	// Constructeur
	KWSortableSymbol();
	~KWSortableSymbol();

	// Critere de tri
	void SetSortValue(const Symbol& sValue);
	Symbol& GetSortValue() const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	mutable Symbol sSortValue;
};

// Comparaison de deux objets KWSortableSymbol par reference
int KWSortableSymbolCompare(const void* elem1, const void* elem2);

// Comparaison de deux objets KWSortableSymbol par valeur, puis par index croissant
int KWSortableSymbolCompareValue(const void* elem1, const void* elem2);

// Comparaison de deux objets KWSortableSymbol par index decroissant, puis par valeur
int KWSortableSymbolCompareDecreasingIndexValue(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////
// Objet a trier par effective decroissant, pour par Symbol selon
// l'ordre lexicographique
class KWSortableFrequencySymbol : public KWSortableSymbol
{
public:
	// Constructeur
	KWSortableFrequencySymbol();
	~KWSortableFrequencySymbol();

	// Effectif
	void SetFrequency(int nValue);
	int GetFrequency() const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	int nFrequency;
};

// Comparaison de deux objets KWSortableFrequencySymbol par effectif decroissant, puis par valeur
int KWSortableFrequencySymbolCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////
// Objet a trier par adresse d'un objet
class KWSortableObject : public KWSortableIndex
{
public:
	// Constructeur
	KWSortableObject();
	~KWSortableObject();

	// Critere de tri
	void SetSortValue(Object* oValue);
	Object* GetSortValue() const;

	// Affichage
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* oSortValue;
};

// Comparaison de deux objets KWSortableObject
int KWSortableObjectCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////////
// Methodes en inline

inline KWSortableIndex::KWSortableIndex()
{
	nIndex = 0;
}

inline KWSortableIndex::~KWSortableIndex() {}

inline void KWSortableIndex::SetIndex(int nValue)
{
	nIndex = nValue;
}

inline int KWSortableIndex::GetIndex() const
{
	return nIndex;
}

/////

inline KWSortableValue::KWSortableValue()
{
	dSortValue = 0;
}

inline KWSortableValue::~KWSortableValue() {}

inline void KWSortableValue::SetSortValue(double dValue)
{
	dSortValue = dValue;
}

inline double KWSortableValue::GetSortValue() const
{
	return dSortValue;
}

/////

inline KWSortableValueSymbol::KWSortableValueSymbol() {}

inline KWSortableValueSymbol::~KWSortableValueSymbol() {}

inline void KWSortableValueSymbol::SetSymbol(const Symbol& sValue)
{
	sSymbol = sValue;
}

inline Symbol& KWSortableValueSymbol::GetSymbol() const
{
	return sSymbol;
}

/////

inline KWSortableContinuous::KWSortableContinuous()
{
	cSortValue = 0;
}

inline KWSortableContinuous::~KWSortableContinuous() {}

inline void KWSortableContinuous::SetSortValue(Continuous cValue)
{
	cSortValue = cValue;
}

inline Continuous KWSortableContinuous::GetSortValue() const
{
	return cSortValue;
}

/////

inline KWSortableContinuousSymbol::KWSortableContinuousSymbol() {}

inline KWSortableContinuousSymbol::~KWSortableContinuousSymbol() {}

inline void KWSortableContinuousSymbol::SetSymbol(const Symbol& sValue)
{
	sSymbol = sValue;
}

inline Symbol& KWSortableContinuousSymbol::GetSymbol() const
{
	return sSymbol;
}

/////

inline KWSortableSymbol::KWSortableSymbol() {}

inline KWSortableSymbol::~KWSortableSymbol() {}

inline void KWSortableSymbol::SetSortValue(const Symbol& sValue)
{
	sSortValue = sValue;
}

inline Symbol& KWSortableSymbol::GetSortValue() const
{
	return sSortValue;
}

/////

inline KWSortableFrequencySymbol::KWSortableFrequencySymbol()
{
	nFrequency = 0;
}

inline KWSortableFrequencySymbol::~KWSortableFrequencySymbol() {}

inline void KWSortableFrequencySymbol::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

inline int KWSortableFrequencySymbol::GetFrequency() const
{
	return nFrequency;
}

/////

inline KWSortableObject::KWSortableObject()
{
	oSortValue = NULL;
}

inline KWSortableObject::~KWSortableObject() {}

inline void KWSortableObject::SetSortValue(Object* oValue)
{
	oSortValue = oValue;
}

inline Object* KWSortableObject::GetSortValue() const
{
	return oSortValue;
}
