// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWValueDictionary.h"

// Inclusion dans le source plutot que le header pour eviter les reference cycliques
#include "KWObject.h"

//////////////////////////////////////////////////////////////////
// Classe KWValueDictionary

void KWValueDictionary::CopyFrom(const KWValueDictionary* sourceValueDictionary)
{
	POSITION position;
	Symbol sKey;
	KWKeyIndex* keyIndexValue;

	require(sourceValueDictionary != NULL);
	require(sourceValueDictionary->GetType() == GetType());

	// On ne copie que s'il s'agit d'un objet different
	if (sourceValueDictionary != this)
	{
		// Nettoyage prealable
		nkdKeyValues.DeleteAll();

		// Parcours des valeurs pour dupliquer le contenu
		position = sourceValueDictionary->GetStartPosition();
		while (position != NULL)
		{
			sourceValueDictionary->GetNextAssoc(position, sKey, keyIndexValue);
			nkdKeyValues.SetAt(sKey.GetNumericKey(), keyIndexValue->Clone());
		}
	}
}

void KWValueDictionary::ExportKeyIndexValues(ObjectArray* oaKeyIndexValues) const
{
	require(oaKeyIndexValues != NULL);
	nkdKeyValues.ExportObjectArray(oaKeyIndexValues);
}

void KWValueDictionary::Write(ostream& ost) const
{
	ObjectArray oaKeyIndexValues;
	KWKeyIndex* keyIndexValue;
	const int nMax = 10;
	int n;

	// Export des paire (cle,valeur) dans un tableau
	ExportKeyIndexValues(&oaKeyIndexValues);

	// Tri des paires (cle, valeur) par cle
	oaKeyIndexValues.SetCompareFunction(KWKeyIndexCompareKeyValue);
	oaKeyIndexValues.Sort();

	// Affichage du contenu
	ost << GetClassLabel() << " [" << GetCount() << "]:";
	for (n = 0; n < oaKeyIndexValues.GetSize(); n++)
	{
		keyIndexValue = cast(KWKeyIndex*, oaKeyIndexValues.GetAt(n));
		assert(keyIndexValue->GetType() == GetType());
		ost << ' ' << *keyIndexValue;
		if (n >= nMax)
		{
			ost << "...";
			break;
		}
	}
	cout << "\n";
}

longint KWValueDictionary::GetUsedMemory() const
{
	longint lUsedMemory;
	POSITION position;
	Symbol sKey;
	KWKeyIndex* keyIndexValue;

	// Initialisation de la memoire utilisee
	lUsedMemory = sizeof(KWValueDictionary) - sizeof(NumericKeyDictionary);
	lUsedMemory += nkdKeyValues.GetUsedMemory();

	// Ajout de la memoire utilise par les paires, en extrapolant a partir de la premiere
	position = GetStartPosition();
	if (position != NULL)
	{
		GetNextAssoc(position, sKey, keyIndexValue);
		lUsedMemory += GetCount() * keyIndexValue->GetUsedMemory();
	}
	return lUsedMemory;
}

const ALString KWValueDictionary::GetClassLabel() const
{
	return "Dictionary of " + KWType::ToString(GetType()) + " values";
}

POSITION KWValueDictionary::GetStartPosition() const
{
	return nkdKeyValues.GetStartPosition();
}

void KWValueDictionary::GetNextAssoc(POSITION& rNextPosition, Symbol& sKey, KWKeyIndex*& keyIndexValue) const
{
	Object* rValue;
	NUMERIC rKey;

	require(rNextPosition != NULL);

	// Acces aux donnees de la position courante
	rKey = sKey.GetNumericKey();
	nkdKeyValues.GetNextAssoc(rNextPosition, rKey, rValue);
	keyIndexValue = cast(KWKeyIndex*, rValue);
	check(keyIndexValue);
	assert(keyIndexValue->GetType() == GetType());

	// Acces a la cle et a la valeur
	sKey = keyIndexValue->GetKey();
}

//////////////////////////////////////////////////////////////////
// Classe KWContinuousValueDictionary

int KWContinuousValueDictionary::GetType() const
{
	return KWType::Continuous;
}

void KWContinuousValueDictionary::SetAt(const Symbol& sKey, Continuous cNewValue)
{
	KWContinuousKeyIndex* keyIndexContinuous;

	// Recherche d'une valeur existante
	keyIndexContinuous = cast(KWContinuousKeyIndex*, nkdKeyValues.Lookup(sKey.GetNumericKey()));

	// Creation d'une nouvelle valeur si ncessaire
	if (keyIndexContinuous == NULL)
	{
		keyIndexContinuous = new KWContinuousKeyIndex;
		keyIndexContinuous->SetKey(sKey);
		nkdKeyValues.SetAt(sKey.GetNumericKey(), keyIndexContinuous);
	}
	keyIndexContinuous->SetValue(cNewValue);
}

void KWContinuousValueDictionary::UpgradeAt(const Symbol& sKey, Continuous cDeltaValue)
{
	KWContinuousKeyIndex* keyIndexContinuous;

	// Recherche d'une valeur existante
	keyIndexContinuous = cast(KWContinuousKeyIndex*, nkdKeyValues.Lookup(sKey.GetNumericKey()));

	// Modification si valeur existante
	if (keyIndexContinuous != NULL)
		keyIndexContinuous->SetValue(keyIndexContinuous->GetValue() + cDeltaValue);
	// Creation d'une nouvelle valeur sinon
	else
	{
		keyIndexContinuous = new KWContinuousKeyIndex;
		keyIndexContinuous->SetKey(sKey);
		keyIndexContinuous->SetValue(cDeltaValue);
		nkdKeyValues.SetAt(sKey.GetNumericKey(), keyIndexContinuous);
	}
}

KWContinuousValueDictionary* KWContinuousValueDictionary::Clone() const
{
	KWContinuousValueDictionary* cloneValueDictionary;

	cloneValueDictionary = new KWContinuousValueDictionary;
	cloneValueDictionary->CopyFrom(this);
	return cloneValueDictionary;
}

void KWContinuousValueDictionary::Test()
{
	const int nValueNumber = 1000;
	KWContinuousValueDictionary vdTest;
	KWContinuousValueDictionary vdCopy;
	int i;
	ALString sTmp;

	// Alimentation
	for (i = 0; i < nValueNumber; i++)
		vdTest.SetAt(Symbol(sTmp + "Key" + IntToString(i + 1)), i + 1);
	cout << "Init " << vdTest << endl;

	// Copie
	vdCopy.CopyFrom(&vdTest);
	cout << "Copy " << vdCopy << endl;

	// Replace values
	for (i = 0; i < nValueNumber; i++)
		vdTest.SetAt(Symbol(sTmp + "Key" + IntToString(i + 1)), i + 1 + nValueNumber);
	cout << "Replace " << vdTest << endl;

	// Remove values
	vdTest.RemoveAll();
	cout << "Remove " << vdTest << endl;
}

//////////////////////////////////////////////////////////////////
// Classe KWSymbolValueDictionary

int KWSymbolValueDictionary::GetType() const
{
	return KWType::Symbol;
}

void KWSymbolValueDictionary::SetAt(const Symbol& sKey, const Symbol& sNewValue)
{
	KWSymbolKeyIndex* keyIndexSymbol;

	// Recherche d'une valeur existante
	keyIndexSymbol = cast(KWSymbolKeyIndex*, nkdKeyValues.Lookup(sKey.GetNumericKey()));

	// Creation d'une nouvelle valeur si ncessaire
	if (keyIndexSymbol == NULL)
	{
		keyIndexSymbol = new KWSymbolKeyIndex;
		keyIndexSymbol->SetKey(sKey);
		nkdKeyValues.SetAt(sKey.GetNumericKey(), keyIndexSymbol);
	}
	keyIndexSymbol->SetValue(sNewValue);
}

KWSymbolValueDictionary* KWSymbolValueDictionary::Clone() const
{
	KWSymbolValueDictionary* cloneValueDictionary;

	cloneValueDictionary = new KWSymbolValueDictionary;
	cloneValueDictionary->CopyFrom(this);
	return cloneValueDictionary;
}

void KWSymbolValueDictionary::Test()
{
	const int nValueNumber = 1000;
	KWSymbolValueDictionary vdTest;
	KWSymbolValueDictionary vdCopy;
	int i;
	ALString sTmp;

	// Alimentation
	for (i = 0; i < nValueNumber; i++)
		vdTest.SetAt(Symbol(sTmp + "Key" + IntToString(i + 1)), Symbol(sTmp + "v" + IntToString(i + 1)));
	cout << "Init " << vdTest << endl;

	// Copie
	vdCopy.CopyFrom(&vdTest);
	cout << "Copy " << vdCopy << endl;

	// Replace values
	for (i = 0; i < nValueNumber; i++)
		vdTest.SetAt(Symbol(sTmp + "Key" + IntToString(i + 1)),
			     Symbol(sTmp + "v" + IntToString(i + 1 + nValueNumber)));
	cout << "Replace " << vdTest << endl;

	// Remove values
	vdTest.RemoveAll();
	cout << "Remove " << vdTest << endl;
}

//////////////////////////////////////////////////////////////////
// Classe KWObjectArrayValueDictionary

int KWObjectArrayValueDictionary::GetType() const
{
	return KWType::ObjectArray;
}

void KWObjectArrayValueDictionary::AddObjectAt(const Symbol& sKey, KWObject* kwoObject)
{
	KWObjectArrayKeyIndex* keyIndexObjectArray;

	require(kwoObject != NULL);

	// Recherche d'un tableau existant
	keyIndexObjectArray = cast(KWObjectArrayKeyIndex*, nkdKeyValues.Lookup(sKey.GetNumericKey()));

	// Creation d'une nouvelle valeur si ncessaire
	if (keyIndexObjectArray == NULL)
	{
		keyIndexObjectArray = new KWObjectArrayKeyIndex;
		keyIndexObjectArray->SetKey(sKey);
		nkdKeyValues.SetAt(sKey.GetNumericKey(), keyIndexObjectArray);
	}

	// Ajout de l'objet dans le tableau
	keyIndexObjectArray->GetValue()->Add(kwoObject);
}

KWObjectArrayValueDictionary* KWObjectArrayValueDictionary::Clone() const
{
	KWObjectArrayValueDictionary* cloneValueDictionary;

	cloneValueDictionary = new KWObjectArrayValueDictionary;
	cloneValueDictionary->CopyFrom(this);
	return cloneValueDictionary;
}

void KWObjectArrayValueDictionary::Test()
{
	const int nValueNumber = 1000;
	KWClass* attributeClass;
	ObjectArray oaCreatedObjects;
	ObjectArray oaNewCreatedObjects;
	KWObjectArrayValueDictionary vdTest;
	KWObjectArrayValueDictionary vdCopy;
	int i;
	ALString sTmp;

	// Creation d'une classe de test
	attributeClass = KWClass::CreateClass("AttributeClass", 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, true, NULL);
	KWClassDomain::GetCurrentDomain()->InsertClass(attributeClass);
	KWClassDomain::GetCurrentDomain()->Compile();

	// Creation d'objets
	for (i = 0; i < nValueNumber; i++)
	{
		oaCreatedObjects.Add(KWObject::CreateObject(attributeClass, i + 1));
		oaNewCreatedObjects.Add(KWObject::CreateObject(attributeClass, nValueNumber + i + 1));
	}

	// Alimentation avec un objet par cle
	for (i = 0; i < nValueNumber; i++)
		vdTest.AddObjectAt(Symbol(sTmp + "Key" + IntToString(i + 1)),
				   cast(KWObject*, oaCreatedObjects.GetAt(i)));
	cout << "Init " << vdTest << endl;

	// Copie
	vdCopy.CopyFrom(&vdTest);
	cout << "Copy " << vdCopy << endl;

	// Ajout d'autres objets
	for (i = 0; i < nValueNumber; i++)
		vdTest.AddObjectAt(Symbol(sTmp + "Key" + IntToString(i + 1)),
				   cast(KWObject*, oaNewCreatedObjects.GetAt(i)));
	cout << "With new objets " << vdTest << endl;

	// Remove values
	vdTest.RemoveAll();
	cout << "Remove " << vdTest << endl;

	// Destruction des objets
	oaCreatedObjects.DeleteAll();
	oaNewCreatedObjects.DeleteAll();

	// Destruction de toutes les classes enregistrees
	KWClassDomain::DeleteAllDomains();
}
