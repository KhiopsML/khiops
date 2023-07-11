// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRReference.h"

void KWDRRegisterReferenceRule()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRReference);
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRReference

KWDRReference::KWDRReference()
{
	SetName(KWDerivationRule::GetReferenceRuleName());
	SetLabel("Reference to an entity by its key");
	SetType(KWType::Object);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRReference::~KWDRReference() {}

KWDerivationRule* KWDRReference::Create() const
{
	return new KWDRReference;
}

KWObject* KWDRReference::ComputeObjectResult(const KWObject* kwoObject) const
{
	KWObjectKey objectKey;
	KWClass* kwcReferenceClass;
	KWObject* kwoReferenceObject;
	int n;

	require(IsCompiled());

	// Resolution de la reference si le parametrage le permet
	kwoReferenceObject = NULL;
	if (objectReferenceResolver != NULL)
	{
		// Recherche de la classe
		kwcReferenceClass = objectReferenceResolver->LookupClass(GetObjectClassName());

		// Recherche de l'objet
		if (kwcReferenceClass != NULL)
		{
			// Recherche de la cle de referencement
			objectKey.SetSize(GetOperandNumber());
			for (n = 0; n < GetOperandNumber(); n++)
				objectKey.SetAt(n, GetOperandAt(n)->GetSymbolValue(kwoObject));

			// On retourne l'objet reference de la classe correspondante
			kwoReferenceObject = objectReferenceResolver->LookupObject(kwcReferenceClass, &objectKey);
		}
	}
	return kwoReferenceObject;
}

void KWDRReference::SetObjectReferenceResolver(KWObjectReferenceResolver* resolver)
{
	objectReferenceResolver = resolver;
}

KWObjectReferenceResolver* KWDRReference::GetObjectReferenceResolver()
{
	return objectReferenceResolver;
}

KWObjectReferenceResolver* KWDRReference::objectReferenceResolver = NULL;

boolean KWDRReference::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	KWClass* kwcReferenceClass;
	ALString sTmp;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification de la compatibilite des operandes avec la cle de la classe referencee
	if (bOk)
	{
		// Recherche de la classe dans le domaine de compilation
		kwcReferenceClass = kwcOwnerClass->GetDomain()->LookupClass(GetObjectClassName());

		// La classe doit etre exister
		if (kwcReferenceClass == NULL)
		{
			AddError(sTmp + "Referenced dictionary " + GetObjectClassName() + " not found");
			bOk = false;
		}
		// La classe utilisee doit etre une classe racine
		else if (not kwcReferenceClass->GetRoot())
		{
			AddError("The referenced dictionary " + kwcReferenceClass->GetName() +
				 " should be a root dictionary");
			bOk = false;
		}
		// La classe utilisee doit avoir une cle
		else if (kwcReferenceClass->GetKeyAttributeNumber() == 0)
		{
			AddError("The referenced dictionary " + kwcReferenceClass->GetName() + " should have a key");
			bOk = false;
		}
		// Le nombre d'operandes doit etre celui de la cle de la classe referencee
		else if (GetOperandNumber() != kwcReferenceClass->GetKeyAttributeNumber())
		{
			AddError(sTmp + "The size of the key (" + IntToString(GetOperandNumber()) +
				 ") should be the same as that of the referenced dictionary " +
				 kwcReferenceClass->GetName() + " (" +
				 IntToString(kwcReferenceClass->GetKeyAttributeNumber()) + ")");
			bOk = false;
		}
	}
	return bOk;
}

void KWDRReference::WriteUsedRule(ostream& ost) const
{
	KWDerivationRuleOperand* operand;
	int i;

	// Operandes
	ost << "[";
	for (i = 0; i < GetOperandNumber(); i++)
	{
		operand = GetOperandAt(i);
		if (i > 0)
			ost << ", ";
		operand->WriteUsedOperand(ost);
	}
	ost << "]";
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWObjectReferenceResolver

KWObjectReferenceResolver::KWObjectReferenceResolver() {}

KWObjectReferenceResolver::~KWObjectReferenceResolver()
{
	RemoveAll();
}

void KWObjectReferenceResolver::RemoveAll()
{
	// Dereferencement des classes
	odAllClasses.RemoveAll();

	// Destruction des dictionnaires de KWObjects (sans detruire ces KWObjects)
	nkdAllClassesObjects.DeleteAll();
}

void KWObjectReferenceResolver::DeleteAll()
{
	int nClass;
	KWClass* kwcClass;
	ObjectArray oaClasses;
	ObjectArray oaClassObjects;

	// Destruction des objets associes a chaque classe
	ExportClasses(&oaClasses);
	for (nClass = 0; nClass < oaClasses.GetSize(); nClass++)
	{
		kwcClass = cast(KWClass*, oaClasses.GetAt(nClass));
		ExportObjects(kwcClass, &oaClassObjects);
		oaClassObjects.DeleteAll();
	}

	// Dereferencement des classes
	RemoveAll();
}

int KWObjectReferenceResolver::GetClassNumber() const
{
	return odAllClasses.GetCount();
}

void KWObjectReferenceResolver::AddClass(KWClass* kwcClass)
{
	require(kwcClass != NULL);
	require(kwcClass->GetKeyAttributeNumber() > 0);
	require(LookupClass(kwcClass->GetName()) == NULL);

	// Enregistrement de la classe
	odAllClasses.SetAt(kwcClass->GetName(), kwcClass);

	// Creation d'un dictionnaire d'objets pour la classe, selon le type de cle
	if (kwcClass->GetKeyAttributeNumber() == 1)
		nkdAllClassesObjects.SetAt(kwcClass, new NumericKeyDictionary);
	else
		nkdAllClassesObjects.SetAt(kwcClass, new ObjectDictionary);

	// Validite des tailles des containers
	ensure(nkdAllClassesObjects.GetCount() == odAllClasses.GetCount());
}

KWClass* KWObjectReferenceResolver::LookupClass(const ALString& sClassName) const
{
	return cast(KWClass*, odAllClasses.Lookup(sClassName));
}

void KWObjectReferenceResolver::ExportClasses(ObjectArray* oaClasses) const
{
	require(oaClasses != NULL);
	odAllClasses.ExportObjectArray(oaClasses);
}

int KWObjectReferenceResolver::GetObjectNumber(KWClass* kwcClass) const
{
	int nObjectNumber;
	NumericKeyDictionary* nkdClassObjects;
	ObjectDictionary* odClassObjects;

	require(kwcClass != NULL);
	require(LookupClass(kwcClass->GetName()) != NULL);
	require(LookupClass(kwcClass->GetName()) == kwcClass);

	// Recherche du dictionnaire d'objets associes a la classes, selon le type de cle
	if (kwcClass->GetKeyAttributeNumber() == 1)
	{
		nkdClassObjects = cast(NumericKeyDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		check(nkdClassObjects);
		nObjectNumber = nkdClassObjects->GetCount();
	}
	else
	{
		odClassObjects = cast(ObjectDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		check(odClassObjects);
		nObjectNumber = odClassObjects->GetCount();
	}
	return nObjectNumber;
}

void KWObjectReferenceResolver::AddObject(KWClass* kwcClass, const KWObjectKey* objectKey, KWObject* kwoObject)
{
	NumericKeyDictionary* nkdClassObjects;
	ObjectDictionary* odClassObjects;

	require(kwcClass != NULL);
	require(objectKey != NULL);
	require(objectKey->GetSize() == kwcClass->GetKeyAttributeNumber());
	require(kwoObject != NULL);
	require(LookupClass(kwcClass->GetName()) != NULL);
	require(LookupClass(kwcClass->GetName()) == kwcClass);
	require(LookupObject(kwcClass, objectKey) == NULL);

	// Recherche du dictionnaire d'objets associes a la classes, selon le type de cle
	// puis de l'objet dans ce dictionnaire
	if (kwcClass->GetKeyAttributeNumber() == 1)
	{
		nkdClassObjects = cast(NumericKeyDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		nkdClassObjects->SetAt(GetSymbolSystemKey(objectKey), kwoObject);
	}
	else
	{
		odClassObjects = cast(ObjectDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		odClassObjects->SetAt(ComputeStringSystemKey(objectKey), kwoObject);
	}
}

void KWObjectReferenceResolver::RemoveObject(KWClass* kwcClass, const KWObjectKey* objectKey)
{
	NumericKeyDictionary* nkdClassObjects;
	ObjectDictionary* odClassObjects;

	require(kwcClass != NULL);
	require(objectKey != NULL);
	require(objectKey->GetSize() == kwcClass->GetKeyAttributeNumber());
	require(LookupClass(kwcClass->GetName()) != NULL);
	require(LookupClass(kwcClass->GetName()) == kwcClass);
	require(LookupObject(kwcClass, objectKey) != NULL);

	// Recherche du dictionnaire d'objets associes a la classes, selon le type de cle
	// puis dereferencement de l'objet dans ce dictionnaire
	if (kwcClass->GetKeyAttributeNumber() == 1)
	{
		nkdClassObjects = cast(NumericKeyDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		nkdClassObjects->RemoveKey(GetSymbolSystemKey(objectKey));
	}
	else
	{
		odClassObjects = cast(ObjectDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		odClassObjects->RemoveKey(ComputeStringSystemKey(objectKey));
	}
}

KWObject* KWObjectReferenceResolver::LookupObject(KWClass* kwcClass, const KWObjectKey* objectKey) const
{
	NumericKeyDictionary* nkdClassObjects;
	ObjectDictionary* odClassObjects;
	KWObject* kwoObject;

	require(kwcClass != NULL);
	require(objectKey != NULL);
	require(objectKey->GetSize() == kwcClass->GetKeyAttributeNumber());
	require(LookupClass(kwcClass->GetName()) != NULL);
	require(LookupClass(kwcClass->GetName()) == kwcClass);

	// Recherche du dictionnaire d'objets associes a la classes, selon le type de cle
	// puis recherche de l'objet dans ce dictionnaire
	if (kwcClass->GetKeyAttributeNumber() == 1)
	{
		nkdClassObjects = cast(NumericKeyDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		kwoObject = cast(KWObject*, nkdClassObjects->Lookup(GetSymbolSystemKey(objectKey)));
	}
	else
	{
		odClassObjects = cast(ObjectDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		kwoObject = cast(KWObject*, odClassObjects->Lookup(ComputeStringSystemKey(objectKey)));
	}
	return kwoObject;
}

void KWObjectReferenceResolver::ExportObjects(KWClass* kwcClass, ObjectArray* oaClassObjects) const
{
	NumericKeyDictionary* nkdClassObjects;
	ObjectDictionary* odClassObjects;

	require(kwcClass != NULL);
	require(LookupClass(kwcClass->GetName()) != NULL);
	require(LookupClass(kwcClass->GetName()) == kwcClass);
	require(oaClassObjects != NULL);

	// Recherche du dictionnaire d'objets associes a la classes, selon le type de cle
	// puis export vers un tableau d'objets
	if (kwcClass->GetKeyAttributeNumber() == 1)
	{
		nkdClassObjects = cast(NumericKeyDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		nkdClassObjects->ExportObjectArray(oaClassObjects);
	}
	else
	{
		odClassObjects = cast(ObjectDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
		odClassObjects->ExportObjectArray(oaClassObjects);
	}
}

longint KWObjectReferenceResolver::GetUsedMemory() const
{
	longint lUsedMemory;
	ObjectArray oaClasses;
	KWClass* kwcClass;
	int nClass;
	NumericKeyDictionary* nkdClassObjects;
	ObjectDictionary* odClassObjects;
	ObjectArray oaClassObjects;
	KWObject* kwoObject;
	int nObject;

	// Tailles de dictionnaires globaux
	lUsedMemory = odAllClasses.GetUsedMemory();
	lUsedMemory += nkdAllClassesObjects.GetUsedMemory();

	// Taille des dictionnaire d'objets et des objets
	ExportClasses(&oaClasses);
	for (nClass = 0; nClass < oaClasses.GetSize(); nClass++)
	{
		kwcClass = cast(KWClass*, oaClasses.GetAt(nClass));

		// Taille du dictionnaire d'objets associes a la classes, selon le type de cle
		if (kwcClass->GetKeyAttributeNumber() == 1)
		{
			nkdClassObjects = cast(NumericKeyDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
			lUsedMemory += nkdClassObjects->GetUsedMemory();
		}
		else
		{
			odClassObjects = cast(ObjectDictionary*, nkdAllClassesObjects.Lookup(kwcClass));
			lUsedMemory += odClassObjects->GetUsedMemory();
		}

		// Taille des objets du dictionnaire
		ExportObjects(kwcClass, &oaClassObjects);
		for (nObject = 0; nObject < oaClassObjects.GetSize(); nObject++)
		{
			kwoObject = cast(KWObject*, oaClassObjects.GetAt(nObject));
			lUsedMemory += kwoObject->GetUsedMemory();
		}
	}
	return lUsedMemory;
}

NUMERIC KWObjectReferenceResolver::GetSymbolSystemKey(const KWObjectKey* objectKey) const
{
	require(objectKey != NULL);
	require(objectKey->GetSize() == 1);
	return objectKey->GetAt(0).GetNumericKey();
}

const ALString KWObjectReferenceResolver::ComputeStringSystemKey(const KWObjectKey* objectKey) const
{
	ALString sSringKey;
	int i;

	require(objectKey != NULL);
	require(objectKey->GetSize() > 1);

	// Cas general multi-champ

	// On code la cle systeme en prefixant chaque champ (sauf le dernier) par sa longueur (et d'un caractere '.')
	// De cette facon, on assure une bijection entre les vecteurs de valeurs et les cles systemes
	for (i = 0; i < objectKey->GetSize(); i++)
	{
		if (i < objectKey->GetSize() - 1)
		{
			sSringKey += IntToString(objectKey->GetAt(i).GetLength());
			sSringKey += '.';
		}
		sSringKey += objectKey->GetAt(i);
	}
	return sSringKey;
}
