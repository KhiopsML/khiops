// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWLoadIndex;
class KWLoadIndexVector;
class PLShared_LoadIndex;
class PLShared_LoadIndexVector;

#include "KWSymbol.h"
#include "Object.h"
#include "Vector.h"
#include "Timer.h"
#include "PLSharedObject.h"

//////////////////////////////////////////////////////////////////
// Classe de gestion des index de chargement, avec version dense et sparse
// Classe interne, geree comme un type simple depuis l'exterieur,
// avec des services avances uniquement pour les classes devant gerer
// l'acces indexe a des valeurs denses ou sparse
class KWLoadIndex : public SystemObject
{
public:
	// Creation et reinitialisation, dans un etat invalide
	KWLoadIndex();
	void Reset();

	// Test si l'index correspond a une valeur valide
	boolean IsValid() const;

	// Operateurs de comparaison
	boolean operator==(const KWLoadIndex& liLoadIndex) const;
	boolean operator!=(const KWLoadIndex& liLoadIndex) const;

	// Affichage dans un stream
	friend ostream& operator<<(ostream& ost, const KWLoadIndex& value);

	// Methode de test
	static void Test();

	////////////////////////////////////////////////////////////////////
	///// Implementation
private:
	// Acces a l'index de donnee dense
	void SetDenseIndex(int nDenseIndex);
	int GetDenseIndex() const;

	// Acces a l'index de donnee sparse
	void SetSparseIndex(int nSparseIndex);
	int GetSparseIndex() const;

	// Test si l'index correspond a une valeur dense ou sparse
	boolean IsDense() const;
	boolean IsSparse() const;

	// Classes amies, ayant acces aux services internes
	friend class KWLoadIndexVector;
	friend class KWClass;
	friend class KWAttribute;
	friend class KWAttributeBlock;
	friend class KWDerivationRuleOperand;
	friend class KWObject;
	friend class KWDataTableSliceSet;
	friend class PLShared_LoadIndex;

	// Donnees de gestion optimise d'un index
	// L'index complet est compose d'un index dense (-1) si invalide, permettant
	// d'acceder a une donnee dense (type de donnees dense, ou bloc de donnees sparse).
	// Si la donnees est sparse, l'index sparse permet d'acceder a la donnee sparse
	// au sein de son bloc
	// L'index complet (lFull) permet de traiter les index dense et sparse en une
	// seule operation
	union
	{
		longint lFullIndex;
		struct
		{
			int nDenseIndex;
			int nSparseIndex;
		} IndexParts;
	};
};

//////////////////////////////////////////////////////////////////
// Vecteur d'index de chargement
// Classe interne, geree comme un type simple depuis l'exterieur,
// avec des services avances uniquement pour les classe devant gerer
// l'acces indexe a des valeurs denses ou sparse
class KWLoadIndexVector : public Object
{
public:
	KWLoadIndexVector();
	~KWLoadIndexVector();

	// Copie et duplication
	KWLoadIndexVector* Clone() const;
	void CopyFrom(const KWLoadIndexVector* livSource);

	// Taille du vecteur
	void SetSize(int nValue);
	int GetSize() const;

	// Acces aux elements
	void SetAt(int nIndex, KWLoadIndex liValue);
	KWLoadIndex GetAt(int nIndex) const;

	// Ajout d'un element en fin et retaillage du vecteur
	void Add(KWLoadIndex liValue);

	// Reinitialisation d'un element a une valeur invalide
	void ResetAt(int nIndex);

	// Memoire utilisee
	longint GetUsedMemory() const;

	////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	LongintVector lvLoadIndexes;
	friend class PLShared_LoadIndexVector;
};

// Ecriture dans un stream
inline ostream& operator<<(ostream& ost, const KWLoadIndex& value)
{
	ost << "(" << value.GetDenseIndex() << ", " << value.GetSparseIndex() << ")";
	return ost;
}

//////////////////////////////////////////////////////////
// Classe PLShared_LoadIndex
// Serialisation de la classe KWLoadIndex
class PLShared_LoadIndex : public PLSharedVariable
{
public:
	// Constructeur
	PLShared_LoadIndex();
	~PLShared_LoadIndex();

	// Acces a la valeur
	KWLoadIndex GetValue() const;
	void SetValue(KWLoadIndex liValue);

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void SerializeValue(PLSerializer*) const override;
	void DeserializeValue(PLSerializer*) override;

	// Valeur
	KWLoadIndex liValue;
};

//////////////////////////////////////////////////////////
// Classe PLShared_LoadIndexVector
// Serialisation de la classe KWLoadIndexVector
class PLShared_LoadIndexVector : public PLSharedObject
{
public:
	// Constructeur
	PLShared_LoadIndexVector();
	~PLShared_LoadIndexVector();

	// Acces aux valeurs
	KWLoadIndex GetAt(int nIndex) const;
	void SetAt(int nIndex, KWLoadIndex liValue);
	void ResetAt(int nIndex);

	// Taille du vecteur
	void SetSize(int nValue);
	int GetSize() const;

	// Acces au vecteur
	void SetLoadIndexVector(KWLoadIndexVector* liv);
	const KWLoadIndexVector* GetConstLoadIndexVector() const;
	KWLoadIndexVector* GetLoadIndexVector();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

// Methodes en inline de KWLoadIndex

inline KWLoadIndex::KWLoadIndex()
{
	lFullIndex = -1;
	ensure(not IsValid() and not IsSparse());
}

inline void KWLoadIndex::Reset()
{
	lFullIndex = -1;
	ensure(not IsValid() and not IsSparse());
}

inline boolean KWLoadIndex::operator==(const KWLoadIndex& liLoadIndex) const
{
	return (lFullIndex == liLoadIndex.lFullIndex);
}

inline boolean KWLoadIndex::operator!=(const KWLoadIndex& liLoadIndex) const
{
	return (lFullIndex != liLoadIndex.lFullIndex);
}

inline void KWLoadIndex::SetDenseIndex(int nDenseIndex)
{
	require(nDenseIndex >= -1);
	IndexParts.nDenseIndex = nDenseIndex;
}

inline int KWLoadIndex::GetDenseIndex() const
{
	return IndexParts.nDenseIndex;
}

inline void KWLoadIndex::SetSparseIndex(int nSparseIndex)
{
	require(nSparseIndex >= -1);
	IndexParts.nSparseIndex = nSparseIndex;
}

inline int KWLoadIndex::GetSparseIndex() const
{
	return IndexParts.nSparseIndex;
}

inline boolean KWLoadIndex::IsValid() const
{
	return IndexParts.nDenseIndex != -1;
}

inline boolean KWLoadIndex::IsDense() const
{
	return IndexParts.nSparseIndex == -1;
}

inline boolean KWLoadIndex::IsSparse() const
{
	return IndexParts.nSparseIndex != -1;
}

// Methodes en inline de KWLoadIndexVector

inline KWLoadIndexVector::KWLoadIndexVector() {}

inline KWLoadIndexVector::~KWLoadIndexVector() {}

inline void KWLoadIndexVector::CopyFrom(const KWLoadIndexVector* livSource)
{
	require(livSource != NULL);
	lvLoadIndexes.CopyFrom(&livSource->lvLoadIndexes);
}

inline KWLoadIndexVector* KWLoadIndexVector::Clone() const
{
	KWLoadIndexVector* livClone;
	livClone = new KWLoadIndexVector;
	livClone->CopyFrom(this);
	return livClone;
}

inline void KWLoadIndexVector::SetSize(int nValue)
{
	lvLoadIndexes.SetSize(nValue);
}

inline int KWLoadIndexVector::GetSize() const
{
	return lvLoadIndexes.GetSize();
}

inline void KWLoadIndexVector::SetAt(int nIndex, KWLoadIndex liValue)
{
	lvLoadIndexes.SetAt(nIndex, liValue.lFullIndex);
}

inline KWLoadIndex KWLoadIndexVector::GetAt(int nIndex) const
{
	KWLoadIndex liValue;
	liValue.lFullIndex = lvLoadIndexes.GetAt(nIndex);
	return liValue;
}

inline void KWLoadIndexVector::Add(KWLoadIndex liValue)
{
	lvLoadIndexes.Add(liValue.lFullIndex);
}

inline void KWLoadIndexVector::ResetAt(int nIndex)
{
	KWLoadIndex liValue;
	lvLoadIndexes.SetAt(nIndex, liValue.lFullIndex);
	ensure(not GetAt(nIndex).IsValid());
}

inline longint KWLoadIndexVector::GetUsedMemory() const
{
	return lvLoadIndexes.GetUsedMemory();
}

// Methodes en inline de PLShared_LoadIndexVector

inline KWLoadIndex PLShared_LoadIndexVector::GetAt(int nIndex) const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstLoadIndexVector()->GetAt(nIndex);
}

inline void PLShared_LoadIndexVector::SetAt(int nIndex, KWLoadIndex liValue)
{
	require(bIsWritable);
	GetLoadIndexVector()->SetAt(nIndex, liValue);
}

inline void PLShared_LoadIndexVector::ResetAt(int nIndex)
{
	require(bIsWritable);
	GetLoadIndexVector()->ResetAt(nIndex);
}

inline void PLShared_LoadIndexVector::SetSize(int nValue)
{
	return GetLoadIndexVector()->SetSize(nValue);
}

inline int PLShared_LoadIndexVector::GetSize() const
{
	return GetConstLoadIndexVector()->GetSize();
}

inline void PLShared_LoadIndexVector::SetLoadIndexVector(KWLoadIndexVector* liv)
{
	SetObject(liv);
}

inline const KWLoadIndexVector* PLShared_LoadIndexVector::GetConstLoadIndexVector() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(KWLoadIndexVector*, GetObject());
}

inline KWLoadIndexVector* PLShared_LoadIndexVector::GetLoadIndexVector()
{
	require(bIsWritable);
	require(bIsDeclared);

	return cast(KWLoadIndexVector*, GetObject());
}

// Methodes en inline de PLShared_LoadIndex

inline void PLShared_LoadIndex::Clean()
{
	liValue.Reset();
}

inline KWLoadIndex PLShared_LoadIndex::GetValue() const
{
	require(bIsReadable);
	require(bIsDeclared);
	return liValue;
}

inline void PLShared_LoadIndex::SetValue(KWLoadIndex li)
{
	require(bIsWritable);
	require(bIsDeclared);
	this->liValue = li;
}
