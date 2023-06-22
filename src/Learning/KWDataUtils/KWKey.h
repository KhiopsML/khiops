// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWKey;
class KWKeyPosition;
class PLShared_Key;
class PLShared_KeyPosition;

#include "Object.h"
#include "PLSerializer.h"
#include "PLSharedObject.h"

////////////////////////////////////////////////////////////
// Classe KWKey
//	 Clef a champs multiples
class KWKey : public Object
{
public:
	// Constructeur
	KWKey();
	~KWKey();

	// Taille de la cle
	void SetSize(int);
	int GetSize() const;

	// Acces aux champs de la cle
	void SetAt(int nIndex, const ALString& sValue);
	const ALString& GetAt(int) const;

	// Copie a partir d'une cle source
	// (retaillage si necessaire)
	void CopyFrom(const KWKey* kwkSource);

	// Duplication
	KWKey* Clone() const;

	// Comparaison avec une autre cle
	int Compare(const KWKey*) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	StringVector svFields;
	friend class PLShared_Key;
};

// Fonction de comparaison de deux clefs
int KWKeyCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////
// Classe PLShared_Key
//	 Serialisation de la classe Key
class PLShared_Key : public PLSharedObject
{
public:
	// Constructeur
	PLShared_Key();
	~PLShared_Key();

	// Acces a la cle
	void SetKey(KWKey* key);
	KWKey* GetKey();

	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Classe KWKeyPosition
//	 Informations sur la position d'une cle dans un fichier,
//   permettant le decoupage en troncons terminant par une
//   position de cle et le debut du troncon suivant
class KWKeyPosition : public Object
{
public:
	// Constructeur
	KWKeyPosition();
	~KWKeyPosition();

	// Memoire: appartient a l'appele
	KWKey* GetKey();

	// Index de la ligne contenant la cle dans un fichier (commence a 1)
	void SetLineIndex(longint lValue);
	longint GetLineIndex() const;

	// Position du debut de la ligne suivante contenant la cle dans un fichier (commence a 0)
	void SetLinePosition(longint lValue);
	longint GetLinePosition() const;

	// Copie a partir d'une position de cle source
	void CopyFrom(const KWKeyPosition* kwkSource);

	// Duplication
	KWKeyPosition* Clone() const;

	// Comparaison avec une autre position de cle, selon la cle
	int Compare(const KWKeyPosition*) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Collecte des cles (KWKey) d'un tableau de KWKeyPosition
	// Memoire: les cles collectees appartiennent toujours a leur position appelante
	static void CollectKeys(const ObjectArray* oaKeyPositions, ObjectArray* oaKeys);

	// Reinitialisation des cle d'un tableau de KWKeyPosition
	// Permet ainsi d'economiser de la memoire en ne gardant que les positions
	static void CleanKeys(const ObjectArray* oaKeyPositions);

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWKey key;
	longint lLinePosition;
	longint lLineIndex;
	friend class PLShared_KeyPosition;
};

// Fonction de comparaison de deux position de clefs, selon la cle
int KWKeyPositionCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////
// Classe PLShared_KeyPosition
//	 Serialisation de la classe KeyPosition
class PLShared_KeyPosition : public PLSharedObject
{
public:
	// Constructeur
	PLShared_KeyPosition();
	~PLShared_KeyPosition();

	// Acces a la cle
	void SetKeyPosition(KWKeyPosition* keyPosition);
	KWKeyPosition* GetKeyPosition();

	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////
// Implementation en inline

inline void KWKey::SetSize(int nValue)
{
	svFields.SetSize(nValue);
}

inline int KWKey::GetSize() const
{
	return svFields.GetSize();
}

inline void KWKey::SetAt(int nIndex, const ALString& sValue)
{
	require(0 <= nIndex and nIndex < svFields.GetSize());
	svFields.SetAt(nIndex, sValue);
}

inline const ALString& KWKey::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < svFields.GetSize());
	return svFields.GetAt(nIndex);
}

inline int KWKey::Compare(const KWKey* key) const
{
	int i;
	int nCompare;
	ALString value1;
	ALString value2;

	require(GetSize() == key->GetSize());

	for (i = 0; i < key->GetSize(); i++)
	{
		value1 = this->GetAt(i);
		value2 = key->GetAt(i);
		nCompare = value1.Compare(value2);
		if (nCompare != 0)
			return nCompare;
	}
	return 0;
}

inline longint KWKey::GetUsedMemory() const
{
	return sizeof(KWKey) + svFields.GetUsedMemory() - sizeof(StringVector);
}

inline void PLShared_Key::SetKey(KWKey* key)
{
	require(key != NULL);
	SetObject(key);
}

inline KWKey* PLShared_Key::GetKey()
{
	return cast(KWKey*, GetObject());
}

inline Object* PLShared_Key::Create() const
{
	return new KWKey;
}

inline KWKey* KWKeyPosition::GetKey()
{
	return &key;
}

inline void KWKeyPosition::SetLineIndex(longint lValue)
{
	require(lValue >= 0);
	lLineIndex = lValue;
}

inline longint KWKeyPosition::GetLineIndex() const
{
	return lLineIndex;
}

inline void KWKeyPosition::SetLinePosition(longint lValue)
{
	require(lValue >= 0);
	lLinePosition = lValue;
}

inline longint KWKeyPosition::GetLinePosition() const
{
	return lLinePosition;
}

inline int KWKeyPosition::Compare(const KWKeyPosition* keyPosition) const
{
	require(keyPosition != NULL);
	return key.Compare(&keyPosition->key);
}

inline longint KWKeyPosition::GetUsedMemory() const
{
	longint lUsedMemory;

	// On ne prend pas en compte la taille de la cle, uniquement referencee
	lUsedMemory = sizeof(KWKeyPosition) - sizeof(KWKey) + key.GetUsedMemory();
	return lUsedMemory;
}

inline void PLShared_KeyPosition::SetKeyPosition(KWKeyPosition* keyPosition)
{
	require(keyPosition != NULL);
	SetObject(keyPosition);
}

inline KWKeyPosition* PLShared_KeyPosition::GetKeyPosition()
{
	return cast(KWKeyPosition*, GetObject());
}

inline Object* PLShared_KeyPosition::Create() const
{
	return new KWKeyPosition;
}
