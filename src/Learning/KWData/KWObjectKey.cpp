// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWObjectKey.h"

KWObjectKey::~KWObjectKey()
{
	if (sKeyFields != NULL)
		DeleteKeyFields();
}

void KWObjectKey::SetSize(int nValue)
{
	int i;

	require(nValue >= 0);
	require((unsigned)nValue <= INT_MAX / sizeof(Symbol));

	// Changement de taille seulement si necessaire
	if (nValue != nSize)
	{
		// Desallocation si nouvelle taille est 0
		if (nValue == 0)
		{
			if (sKeyFields != NULL)
				DeleteKeyFields();
		}
		// Allocation si taille actuelle a 0
		else if (nSize == 0)
		{
			NewKeyFields(nValue);
		}
		// Reallocation sinon
		else
		{
			Symbol* sNewKeyFields;

			// Creation d'un nouveau vecteur de symbol
			sNewKeyFields = (Symbol*)NewMemoryBlock(nValue * sizeof(Symbol));

			// Suppression du warning class-memaccess
			DISABLE_WARNING_PUSH
			DISABLE_WARNING_CLASS_MEMACCESS

			// Une recopie directe des valeurs ne pose pas de probleme, puisque le compteur de reference
			// des symbol est inchange entre avant et apres le retaillage
			if (nValue < nSize)
				memcpy(sNewKeyFields, sKeyFields, nValue * sizeof(Symbol));
			else
			{
				memcpy(sNewKeyFields, sKeyFields, nSize * sizeof(Symbol));
				memset(&sNewKeyFields[nSize], 0, (nValue - nSize) * sizeof(Symbol));
			}
			DISABLE_WARNING_POP

			// Destruction de l'ancien vecteur
			// Attention: on force le deferencement des valeurs Symbol en trop
			if (nValue < nSize)
			{
				for (i = nValue; i < nSize; i++)
					sKeyFields[i].Reset();
			}
			DeleteMemoryBlock(sKeyFields);

			// Memorisation du nouveau vecteur
			sKeyFields = sNewKeyFields;
			nSize = nValue;
		}
	}
	assert(nSize == nValue);
	assert(sKeyFields != NULL or nSize == 0);
}

void KWObjectKey::Initialize()
{
	int i;
	for (i = 0; i < nSize; i++)
		sKeyFields[i].Reset();
}

void KWObjectKey::InitializeFromObject(const KWObject* kwoObject)
{
	const KWClass* kwcObjectClass;
	int nKeySize;
	int i;

	require(kwoObject != NULL);
	require(kwoObject->GetClass()->IsKeyLoaded());

	// Parcours des champs de la cle pour l'alimenter a partir de l'objet
	kwcObjectClass = kwoObject->GetClass();
	nKeySize = kwcObjectClass->GetKeyAttributeNumber();
	SetSize(nKeySize);
	for (i = 0; i < nKeySize; i++)
	{
		assert(kwcObjectClass->GetKeyAttributeAt(i)->GetLoadIndex() ==
		       kwcObjectClass->GetKeyAttributeLoadIndexAt(i));
		sKeyFields[i] = kwoObject->GetSymbolValueAt(kwcObjectClass->GetKeyAttributeLoadIndexAt(i));
	}
}

void KWObjectKey::CopyFrom(const KWObjectKey* kwokSource)
{
	int i;

	require(kwokSource != NULL);

	// Cas particulier ou source egale cible
	if (kwokSource == this)
		return;

	// Retaillage
	if (kwokSource->GetSize() != nSize)
		SetSize(kwokSource->GetSize());

	// Recopie des valeurs sources
	for (i = 0; i < nSize; i++)
		sKeyFields[i] = kwokSource->sKeyFields[i];
}

KWObjectKey* KWObjectKey::Clone() const
{
	KWObjectKey* svClone;

	svClone = new KWObjectKey;
	svClone->CopyFrom(this);
	return svClone;
}

int KWObjectKey::Compare(const KWObjectKey* kwokKey) const
{
	int nMinSize;
	int nCompare;
	int i;

	require(kwokKey != NULL);

	// Calcul du nombre de champs communs
	nMinSize = GetSize();
	if (kwokKey->GetSize() < nMinSize)
		nMinSize = kwokKey->GetSize();

	// Comparaison champ a champ sur les champs communs
	for (i = 0; i < nMinSize; i++)
	{
		nCompare = GetAt(i).CompareValue(kwokKey->GetAt(i));
		if (nCompare != 0)
			return nCompare;
	}

	// Comparaison en fonction de la longueur, puisqu'il y a egalite sur las champs en commun
	return GetSize() - kwokKey->GetSize();
}

int KWObjectKey::StrictCompare(const KWObjectKey* kwokKey) const
{
	int nCompare;
	int i;

	require(kwokKey != NULL);
	require(GetSize() == kwokKey->GetSize());

	// Comparaison champ a champ
	for (i = 0; i < GetSize(); i++)
	{
		nCompare = GetAt(i).CompareValue(kwokKey->GetAt(i));
		if (nCompare != 0)
			return nCompare;
	}
	return 0;
}

int KWObjectKey::SubCompare(const KWObjectKey* kwokKey) const
{
	int nCompare;
	int i;

	require(kwokKey != NULL);
	require(GetSize() <= kwokKey->GetSize());

	// Comparaison champ a champ
	for (i = 0; i < GetSize(); i++)
	{
		nCompare = GetAt(i).CompareValue(kwokKey->GetAt(i));
		if (nCompare != 0)
			return nCompare;
	}
	return 0;
}

void KWObjectKey::Write(ostream& ost) const
{
	int i;

	ost << "[";
	for (i = 0; i < GetSize(); i++)
	{
		if (i > 0)
			ost << ", ";
		ost << GetAt(i);
	}
	ost << "]";
}

const ALString KWObjectKey::GetClassLabel() const
{
	return "Key";
}

const ALString KWObjectKey::GetObjectLabel() const
{
	ALString sObjectLabel;
	int i;

	sObjectLabel = "[";
	for (i = 0; i < GetSize(); i++)
	{
		if (i > 0)
			sObjectLabel += ", ";
		sObjectLabel += GetAt(i);
	}
	sObjectLabel += "]";
	return sObjectLabel;
}

void KWObjectKey::Test()
{
	KWObjectKey key1;
	KWObjectKey key2;
	KWObjectKey key3;
	KWObjectKey key4;
	KWObjectKey key5;
	ObjectArray oaKeys;
	int i;
	int j;
	KWObjectKey* key;
	KWObjectKey* keyCompared;

	// Initialisations
	key1.SetSize(1);
	key1.SetAt(0, "A1");
	key2.SetSize(2);
	key2.SetAt(0, "A2");
	key2.SetAt(1, "a2");
	key3.CopyFrom(&key2);
	key3.SetAt(1, "b2");
	key4.CopyFrom(&key3);
	key4.SetSize(3);
	key4.SetAt(2, "last");
	key5.CopyFrom(&key4);
	key5.SetSize(4);
	key5.SetAt(3, "END");

	// Rangement dans un tableau
	oaKeys.Add(&key1);
	oaKeys.Add(&key2);
	oaKeys.Add(&key3);
	oaKeys.Add(&key4);
	oaKeys.Add(&key5);

	// Affichages
	for (i = 0; i < oaKeys.GetSize(); i++)
	{
		key = cast(KWObjectKey*, oaKeys.GetAt(i));
		cout << "key" << i + 1 << ": " << *key << endl;
	}

	// Comparaisons
	cout << "Key comparirons" << endl;
	cout << "First key\tSecond key\tComp\tSubComp\tStrictComp" << endl;
	for (i = 0; i < oaKeys.GetSize(); i++)
	{
		key = cast(KWObjectKey*, oaKeys.GetAt(i));
		for (j = 0; j < oaKeys.GetSize(); j++)
		{
			keyCompared = cast(KWObjectKey*, oaKeys.GetAt(j));
			cout << key->GetObjectLabel() << "\t" << keyCompared->GetObjectLabel() << "\t"
			     << key->Compare(keyCompared) << "\t";
			if (key->GetSize() <= keyCompared->GetSize())
				cout << key->SubCompare(keyCompared);
			cout << "\t";
			if (key->GetSize() == keyCompared->GetSize())
				cout << " " << key->StrictCompare(keyCompared);
			cout << endl;
		}
	}
}

void KWObjectKey::NewKeyFields(int nInitialisationSize)
{
	require(sKeyFields == NULL);
	require(nInitialisationSize > 0);

	sKeyFields = (Symbol*)NewMemoryBlock(nInitialisationSize * sizeof(Symbol));

	// Suppression du warning class-memaccess
	DISABLE_WARNING_PUSH
	DISABLE_WARNING_CLASS_MEMACCESS
	memset(sKeyFields, 0, nInitialisationSize * sizeof(Symbol));
	DISABLE_WARNING_POP
	nSize = nInitialisationSize;
}

void KWObjectKey::DeleteKeyFields()
{
	require(sKeyFields != NULL);
	require(nSize > 0);

	// Destruction apres appel a initialisation pour gerer dereferencer les Symbol
	Initialize();
	DeleteMemoryBlock(sKeyFields);
	nSize = 0;
	sKeyFields = NULL;
}

int KWObjectCompareKey(const void* elem1, const void* elem2)
{
	static KWObjectKey key1;
	static KWObjectKey key2;
	KWObject* kwoObject1;
	KWObject* kwoObject2;
	int nCompare;

	// Acces aux objets
	kwoObject1 = cast(KWObject*, *(Object**)elem1);
	kwoObject2 = cast(KWObject*, *(Object**)elem2);

	// Initialisation des cles de comparaisons
	key1.InitializeFromObject(kwoObject1);
	key2.InitializeFromObject(kwoObject2);

	// Comparaison
	nCompare = key1.StrictCompare(&key2);
	return nCompare;
}
