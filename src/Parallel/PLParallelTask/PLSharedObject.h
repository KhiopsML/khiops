// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLSharedVariable.h"

class PLSharedObject;
class PLShared_ObjectArray;
class PLShared_ObjectDictionary;
class PLShared_ObjectList;
class PLShared_StringObject;
class PLShared_DoubleObject;
class PLShared_IntObject;
class PLShared_LongintObject;

////////////////////////////////////////////////////////////////////////////
// Classe PLSharedObject.
// Classe virtuelle pure qui offre un cadre pour implementer la serialisation d'objet structures.
// Un exemple est donne dans la classe PLShared_SampleObject
//
// Les conteneurs partages utilisent les PLSharedObject (PLShared_ObjectArray par exemple).
class PLSharedObject : public PLSharedVariable
{
public:
	// Constructeur
	PLSharedObject();
	~PLSharedObject();

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles (rediriges sur l'objet partage)
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Ecrase l'objet avec la valeur NULL, sans le detruire prealablement : dereferencement
	// A utiliser avec precaution
	void RemoveObject();

	// Methodes de serialisation / deserialisation d'un objet non null
	virtual void SerializeObject(PLSerializer* serializer, const Object* o) const = 0;
	virtual void DeserializeObject(PLSerializer* serializer, Object* o) const = 0;

	// Ajout de null dans le serializer, si l'objet passe en parametre est null
	void AddNull(PLSerializer* serializer, const Object*) const;
	boolean GetNull(PLSerializer* serializer) const;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Creation d'un objet (type d'objet a serialiser)
	virtual Object* Create() const = 0;

	// Serialisation / Deserialization de l'objet, potentiellement null
	void SerializeValue(PLSerializer*) const final;
	void DeserializeValue(PLSerializer*) final;

	// L'objet appartient a l'appele (construit a la premiere utilisation, detruit si necessaire au destructeur,
	// remplace au set)
	void SetObject(Object*);
	Object* GetObject() const;

private:
	// Objet a serialiser (mutable pour que GetObject soit const)
	mutable Object* oValue;

	// Indicateur d'initialisation
	// (creation d'un objet a la volee si non initialisee; peut etre mis a null apres initialisation)
	// (mutable pour que GetObject soit const)
	mutable boolean bIsInit;

	// Classes friend
	friend class PLShared_ObjectArray;
	friend class PLShared_ObjectList;
	friend class PLShared_ObjectDictionary;
	friend class PLShared_StringVector;
	friend class PLShared_IntVector;
	friend class PLShared_DoubleVector;
	friend class PLShared_CharVector;
	friend class PLShared_LongintVector;
};

////////////////////////////////////////////////////////////////////////////
// Classe PLShared_ObjectArray.
// Classe qui permet d'echanger des tableaux d'objets.
//
// L'objet contenu doit avoir un type serialise associe (qui efectue la serialisation / deserialisation)
// Par exemple pour serialiser un tableau de SampleObject, il faut avoir construit le type PLShared_SampleObject.
// Une instance de cet objet est passee au constructeur.
// Un PLShared_ObjectArray donne un pointeur vers un ObjectArray qui peut etre manipuler comme d'habitude.
// Cet ObjectArray appartient au PLShared_ObjectArray (ainsi que tous les elements qu'il contient)
//
// Un exemple d'utilisation est donne dans la methode de test
//
// Note : Les fonctions de comparaison ne sont pas serialisees. Il est recommande d'associer la fonction de comparaison
// dans le constructeur de la tache.
class PLShared_ObjectArray : public PLSharedObject
{
public:
	// Constructeur avec declaration du "createur" d'objet partage utilise pour les elements du tableau partage
	// Servira a recreer les elements du tableau apres deserialisation
	// Memoire: l'objet en parametre appartient a l'appele
	PLShared_ObjectArray(const PLSharedObject* object);
	~PLShared_ObjectArray();

	// Acces au tableau
	void SetObjectArray(ObjectArray* oa);
	ObjectArray* GetObjectArray();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	void Clean() override;

	// Methodes de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	Object* Create() const override;

	// Variables de gestion du tableau partage
	const PLSharedObject* elementCreator;

	// Fonction de comparaison
	CompareFunction compareFunction;
};

////////////////////////////////////////////////////////////////////////////
// Classe PLShared_ObjectList.
// Classe qui permet d'echanger des listes d'objets.
//
// L'objet contenu doit avoir un type serialise associe (qui efectue la serialisation / deserialisation)
// Par exemple pour serialiser une liste de SampleObject, il faut avoir construit le type PLShared_SampleObject.
// Une instance de cet objet est passe au constructeur.
// Un PLShared_ObjectList donne un pointeur vers un ObjectList qui peut etre manipule comme d'habitude.
// Cet ObjectList appartient au PLShared_ObjectList (ainsi que tous les elements qu'il contient)
//
class PLShared_ObjectList : public PLSharedObject
{
public:
	PLShared_ObjectList(const PLSharedObject* object);
	~PLShared_ObjectList();

	// Acces a la liste
	void SetObjectList(ObjectList* ol);
	ObjectList* GetObjectList();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	void Clean() override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles

	Object* Create() const override;

	// Variables de gestion du tableau partage
	const PLSharedObject* elementCreator;
};

////////////////////////////////////////////////////////////////////////////
// Classe PLShared_ObjectDictionary.
// Classe qui permet d'echanger des dictionnaires d'objets.
//
// L'objet contenu doit avoir un type serialise associe (qui efectue la serialisation / deserialisation)
// Par exemple pour serialiser un dictionnaire de SampleObject, il faut avoir construit le type PLShared_SampleObject.
// Une instance de cet objet est passee au constructeur.
// Un PLShared_ObjectDictionary donne un pointeur vers un ObjectDictionary qui peut etre manipuler comme d'habitude.
// Cet ObjectDictionary appartient au PLShared_ObjectDictionary (ainsi que tous les elements qu'il contient)
//
class PLShared_ObjectDictionary : public PLSharedObject
{
public:
	PLShared_ObjectDictionary(const PLSharedObject* object);
	~PLShared_ObjectDictionary();

	// Acces au dictionnaire
	void SetObjectDictionary(ObjectDictionary* od);
	ObjectDictionary* GetObjectDictionary();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	void Clean() override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	Object* Create() const override;

	// Variables de gestion du dictionnaire partage
	const PLSharedObject* elementCreator;
};

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_StringObject
// Serialisation de StringObject
//
class PLShared_StringObject : public PLSharedObject
{
public:
	// Constructeur
	PLShared_StringObject();
	~PLShared_StringObject();

	// Acces au DoubleObject
	void SetStringObject(StringObject* so);
	StringObject* GetStringObject();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_DoubleObject
// Serialisation de DoubleObject
//
class PLShared_DoubleObject : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DoubleObject();
	~PLShared_DoubleObject();

	// Acces au DoubleObject
	void SetDoubleObject(DoubleObject* o);
	DoubleObject* GetDoubleObject();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_IntObject
// Serialisation de IntObject
//
class PLShared_IntObject : public PLSharedObject
{
public:
	// Constructeur
	PLShared_IntObject();
	~PLShared_IntObject();

	// Acces au IntObject
	void SetIntObject(IntObject* io);
	IntObject* GetIntObject();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

/////////////////////////////////////////////////////////////////////////////
// Classe PLShared_LongintObject
// Serialisation de LongintObject
//
class PLShared_LongintObject : public PLSharedObject
{
public:
	// Constructeur
	PLShared_LongintObject();
	~PLShared_LongintObject();

	// Acces au LongintObject
	void SetLongintObject(LongintObject* hr);
	LongintObject* GetLongintObject();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline void PLSharedObject::Clean()
{
	boolean bStatus;
	bStatus = bIsWritable;
	bIsWritable = true;
	SetObject(NULL);
	bIsInit = false;
	bIsWritable = bStatus;
}

inline void PLSharedObject::Write(ostream& ost) const
{
	if (oValue == NULL)
		ost << "NULL" << endl;
	else
		oValue->Write(ost);
}

inline void PLSharedObject::SetObject(Object* o)
{
	require(bIsWritable);
	bIsInit = true;
	if (oValue != NULL)
		delete oValue;
	oValue = o;
}

inline Object* PLSharedObject::GetObject() const
{
	require(bIsDeclared);

	// Si l'objet n'a pas ete initialise, on le cree
	if (not bIsInit)
	{
		bIsInit = true;
		oValue = Create();
	}
	return oValue;
}

inline void PLShared_ObjectArray::SetObjectArray(ObjectArray* oa)
{
	SetObject(oa);
}

inline ObjectArray* PLShared_ObjectArray::GetObjectArray()
{
	return cast(ObjectArray*, GetObject());
}

inline Object* PLShared_ObjectArray::Create() const
{
	return new ObjectArray;
}

inline void PLShared_ObjectArray::Clean()
{
	if (oValue != NULL)
	{
		cast(ObjectArray*, oValue)->DeleteAll();

		// Sauvegarde de la fonction de comparaison
		if (cast(ObjectArray*, oValue)->IsSortable())
		{
			compareFunction = cast(ObjectArray*, oValue)->GetCompareFunction();
		}
	}
	PLSharedObject::Clean();
}

inline void PLShared_ObjectList::SetObjectList(ObjectList* ol)
{
	require(ol != NULL);
	SetObject(ol);
}

inline ObjectList* PLShared_ObjectList::GetObjectList()
{
	return cast(ObjectList*, GetObject());
}

inline Object* PLShared_ObjectList::Create() const
{
	return new ObjectList;
}

inline void PLShared_ObjectList::Clean()
{
	if (oValue != NULL)
		cast(ObjectList*, oValue)->DeleteAll();
	PLSharedObject::Clean();
}

inline void PLShared_ObjectDictionary::SetObjectDictionary(ObjectDictionary* oa)
{
	SetObject(oa);
}

inline ObjectDictionary* PLShared_ObjectDictionary::GetObjectDictionary()
{
	return cast(ObjectDictionary*, GetObject());
}

inline Object* PLShared_ObjectDictionary::Create() const
{
	return new ObjectDictionary;
}

inline void PLShared_ObjectDictionary::Clean()
{
	if (oValue != NULL)
		cast(ObjectDictionary*, oValue)->DeleteAll();
	PLSharedObject::Clean();
}

inline void PLShared_StringObject::SetStringObject(StringObject* so)
{
	SetObject(so);
}

inline StringObject* PLShared_StringObject::GetStringObject()
{
	return cast(StringObject*, GetObject());
}

inline Object* PLShared_StringObject::Create() const
{
	return new StringObject;
}

inline void PLShared_DoubleObject::SetDoubleObject(DoubleObject* o)
{
	SetObject(o);
}

inline DoubleObject* PLShared_DoubleObject::GetDoubleObject()
{
	return cast(DoubleObject*, GetObject());
}

inline Object* PLShared_DoubleObject::Create() const
{
	return new DoubleObject;
}

inline void PLShared_IntObject::SetIntObject(IntObject* io)
{
	SetObject(io);
}

inline IntObject* PLShared_IntObject::GetIntObject()
{
	return cast(IntObject*, GetObject());
}

inline Object* PLShared_IntObject::Create() const
{
	return new IntObject;
}

inline void PLShared_LongintObject::SetLongintObject(LongintObject* lo)
{
	SetObject(lo);
}

inline LongintObject* PLShared_LongintObject::GetLongintObject()
{
	return cast(LongintObject*, GetObject());
}

inline Object* PLShared_LongintObject::Create() const
{
	return new LongintObject;
}
