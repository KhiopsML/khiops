// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSharedVariable.h"

/////////////////////////////////////////////
// Implementation de la classe PLSharedVariable

PLSharedVariable::PLSharedVariable()
{
	bIsReadable = true;
	bIsWritable = true;
	bIsDeclared = false;
}

PLSharedVariable::~PLSharedVariable() {}

void PLSharedVariable::Serialize(PLSerializer* serializer)
{
	check(serializer);
	require(serializer->IsOpenForWrite());
	SerializeValue(serializer);
}

void PLSharedVariable::Deserialize(PLSerializer* serializer)
{
	check(serializer);
	require(serializer->IsOpenForRead());

	// Nettoyage prealable
	Clean();

	// Extraction de la valeur
	DeserializeValue(serializer);

	// On force le statut a registered. C'est utile
	// pour les objets contenus dans un SharedObject
	// (sharedObjectArray par exemple). Car le fait d'enregistrer
	// le sharedObjectArray n'enregistre pas le contenu
	bIsDeclared = true;
}

void PLSharedVariable::Test()
{
	cout << "-- Test de  PLSerializer" << endl;
	PLSerializer::Test();
	cout << endl << "-- Test de PLShared_Boolean" << endl;
	PLShared_Boolean::Test();
	cout << endl << "-- Test de PLShared_Int" << endl;
	PLShared_Int::Test();
	cout << endl << "-- Test de PLShared_String" << endl;
	PLShared_String::Test();
	cout << endl << "-- Test de PLShared_Double" << endl;
	PLShared_Double::Test();
	cout << endl << "-- Test de PLShared_Longint" << endl;
	PLShared_Longint::Test();
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_Boolean

PLShared_Boolean::PLShared_Boolean()
{
	bValue = true;
}

PLShared_Boolean::~PLShared_Boolean() {}

void PLShared_Boolean::SerializeValue(PLSerializer* serializer) const
{
	serializer->PutBoolean(bValue);
}

void PLShared_Boolean::DeserializeValue(PLSerializer* serializer)
{
	bValue = serializer->GetBoolean();
}

void PLShared_Boolean::Write(ostream& ost) const
{
	ost << BooleanToString(GetValue());
}

void PLShared_Boolean::Test()
{
	PLShared_Boolean shared_booleanIn;
	PLShared_Boolean shared_booleanOut;
	PLSerializer serializer;
	boolean bInitialValue;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_booleanIn.bIsDeclared = true;
	shared_booleanOut.bIsDeclared = true;

	// Quelques tests sur l'utilisation des operateurs
	cout << "Operators test" << endl;
	shared_booleanIn = true;
	shared_booleanOut = false;
	cout << shared_booleanIn << " and " << shared_booleanOut << ": " << (shared_booleanIn and shared_booleanOut)
	     << endl;
	cout << shared_booleanIn << " or " << shared_booleanOut << ": " << (shared_booleanIn or shared_booleanOut)
	     << endl;
	cout << shared_booleanIn << " == " << shared_booleanOut << ": " << (shared_booleanIn == shared_booleanOut)
	     << endl;
	cout << shared_booleanIn << " != " << shared_booleanOut << ": " << (shared_booleanIn != shared_booleanOut)
	     << endl;
	shared_booleanOut = true;
	cout << shared_booleanIn << " and " << shared_booleanOut << ": " << (shared_booleanIn and shared_booleanOut)
	     << endl;
	cout << shared_booleanIn << " or " << shared_booleanOut << ": " << (shared_booleanIn or shared_booleanOut)
	     << endl;
	cout << shared_booleanIn << " == " << shared_booleanOut << ": " << (shared_booleanIn == shared_booleanOut)
	     << endl;
	cout << shared_booleanIn << " != " << shared_booleanOut << ": " << (shared_booleanIn != shared_booleanOut)
	     << endl;

	// Initialisation d'une variable partagee en entree
	bInitialValue = true;
	shared_booleanIn = bInitialValue;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_booleanIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_booleanOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value " << bInitialValue << endl;
	cout << "Serialized value " << shared_booleanOut << endl;
	;
	cout << "Serialized form " << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_booleanOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object " << shared_booleanOut << endl;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_Double

PLShared_Double::PLShared_Double(void)
{
	dValue = 0;
}

PLShared_Double::~PLShared_Double(void) {}

void PLShared_Double::Write(ostream& ost) const
{
	ost << DoubleToString(GetValue());
}

void PLShared_Double::Test()
{
	PLShared_Double shared_valueIn;
	PLShared_Double shared_valueOut;
	PLSerializer serializer;
	double dInitialValue;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_valueIn.bIsDeclared = true;
	shared_valueOut.bIsDeclared = true;

	// Quelques tests sur l'utilisation des operateurs
	cout << "Operators test" << endl;
	shared_valueIn = 0.123450123456;
	cout << "test " << shared_valueIn << " += " << 1 << " -> ";
	shared_valueIn += 1;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 1.123450123456) << endl;
	//
	shared_valueIn = 11.0;
	cout << "test " << shared_valueIn << " ++ -> ";
	shared_valueIn++;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 12.0) << endl;
	//
	shared_valueIn = 11.0;
	cout << "test " << shared_valueIn << " -- -> ";
	shared_valueIn--;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 10.0) << endl;
	//
	shared_valueIn = 0.123450123456;
	cout << "test " << shared_valueIn << " *= " << 10 << " -> ";
	shared_valueIn *= 10;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 1.23450123456) << endl;
	cout << "\t"
	     << " /= " << 10 << " -> ";
	shared_valueIn /= 10;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 0.123450123456) << endl;
	//
	shared_valueOut = shared_valueIn * 2;
	shared_valueOut += 100;
	dInitialValue = shared_valueOut + 10;
	//
	cout << shared_valueIn << " < " << shared_valueOut << ": " << (shared_valueIn < shared_valueOut) << endl;
	cout << shared_valueIn << " < " << dInitialValue << ": " << (shared_valueIn < dInitialValue) << endl;
	cout << dInitialValue << " < " << shared_valueOut << ": " << (dInitialValue < shared_valueOut) << endl;
	//
	cout << dInitialValue << " += " << shared_valueIn << " -> ";
	dInitialValue += shared_valueIn;
	cout << dInitialValue << endl;

	// Initialisation d'une variable partagee en entree
	dInitialValue = 0.123450123456;
	shared_valueIn = dInitialValue;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_valueIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value " << dInitialValue << endl;
	cout << "Serialized value " << shared_valueOut << endl;
	;
	cout << "Serialized form " << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object " << shared_valueOut << endl;
}

void PLShared_Double::SerializeValue(PLSerializer* serializer) const
{
	serializer->PutDouble(dValue);
}

void PLShared_Double::DeserializeValue(PLSerializer* serializer)
{
	dValue = serializer->GetDouble();
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_Int

PLShared_Int::PLShared_Int(void)
{
	nValue = 0;
}

PLShared_Int::~PLShared_Int(void) {}

void PLShared_Int::Write(ostream& ost) const
{
	ost << GetValue();
}

void PLShared_Int::SerializeValue(PLSerializer* serializer) const
{
	serializer->PutInt(nValue);
}

void PLShared_Int::DeserializeValue(PLSerializer* serializer)
{
	nValue = serializer->GetInt();
}

void PLShared_Int::Test()
{
	PLShared_Int shared_valueIn;
	PLShared_Int shared_valueOut;
	PLSerializer serializer;
	int nInitialValue;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_valueIn.bIsDeclared = true;
	shared_valueOut.bIsDeclared = true;

	// Quelques tests sur l'utilisation des operateurs
	cout << "Operators test" << endl;
	shared_valueIn = 10;
	shared_valueOut = shared_valueIn * 2;
	//
	cout << "test " << shared_valueIn << " += " << 1 << " -> ";
	shared_valueIn += 1;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 11) << endl;
	//
	cout << "test " << shared_valueIn << " ++ -> ";
	shared_valueIn++;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 12) << endl;
	//
	cout << "test " << shared_valueIn << " -= " << 1 << " -> ";
	shared_valueIn -= 1;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 11) << endl;
	//
	cout << "test " << shared_valueIn << " -- -> ";
	shared_valueIn--;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 10) << endl;
	//
	cout << "test " << shared_valueIn << " *= " << 2 << " -> ";
	shared_valueIn *= 2;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 20) << endl;
	//
	cout << "test " << shared_valueIn << " /= " << 3 << " -> ";
	shared_valueIn /= 3;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == 6) << endl;
	//
	nInitialValue = abs(shared_valueOut) + 10;
	cout << shared_valueIn << " < " << shared_valueOut << ": " << (shared_valueIn < shared_valueOut) << endl;
	cout << shared_valueIn << " < " << nInitialValue << ": " << (shared_valueIn < nInitialValue) << endl;
	cout << nInitialValue << " < " << shared_valueOut << ": " << (nInitialValue < shared_valueOut) << endl;
	//
	cout << shared_valueIn << " < " << 1 << ": " << (shared_valueIn < 1) << endl;
	cout << 1 << " < " << shared_valueIn << ": " << (1 < shared_valueIn) << endl;
	cout << shared_valueIn << " > " << 1 << ": " << (shared_valueIn > 1) << endl;
	cout << 1 << " > " << shared_valueIn << ": " << (1 > shared_valueIn) << endl;
	cout << nInitialValue << " += " << shared_valueIn << " -> ";
	nInitialValue += shared_valueIn;
	cout << nInitialValue << endl;

	// Initialisation d'une variable partagee en entree
	nInitialValue = 123456789;
	shared_valueIn = nInitialValue;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_valueIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value " << nInitialValue << endl;
	cout << "Serialized value " << shared_valueOut << endl;
	;
	cout << "Serialized form " << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object " << shared_valueOut << endl;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_Longint

PLShared_Longint::PLShared_Longint(void)
{
	lValue = 0;
}

PLShared_Longint::~PLShared_Longint(void) {}

void PLShared_Longint::Write(ostream& ost) const
{
	ost << LongintToString(GetValue());
}

void PLShared_Longint::SerializeValue(PLSerializer* serializer) const
{
	serializer->PutLongint(lValue);
}

void PLShared_Longint::DeserializeValue(PLSerializer* serializer)
{
	lValue = serializer->GetLongint();
}

void PLShared_Longint::Test()
{
	PLShared_Longint shared_valueIn;
	PLShared_Longint shared_valueOut;
	PLSerializer serializer;
	longint lInitialValue;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_valueIn.bIsDeclared = true;
	shared_valueOut.bIsDeclared = true;

	// Quelques tests sur l'utilisation des operateurs
	cout << "Operators test" << endl;
	shared_valueIn = 5000000000;
	cout << "test " << shared_valueIn << " -- -> ";
	shared_valueIn--;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == (longint)4999999999) << endl;
	//
	cout << "test " << shared_valueIn << " ++ -> ";
	shared_valueIn++;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == (longint)5000000000) << endl;
	//
	cout << "test " << shared_valueIn << " += " << 10 << " -> ";
	shared_valueIn += 10;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == (longint)5000000010) << endl;
	//
	cout << "test " << shared_valueIn << " -= " << 1 << " -> ";
	shared_valueIn -= 10;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == (longint)5000000000) << endl;
	//
	cout << "test " << shared_valueIn << " *= " << 2 << " -> ";
	shared_valueIn *= 2;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == (longint)10000000000) << endl;
	//
	cout << "test " << shared_valueIn << " /= " << 10 << " -> ";
	shared_valueIn /= 10;
	cout << shared_valueIn;
	cout << ": " << (shared_valueIn == (longint)1000000000) << endl;
	//
	shared_valueIn = 5000000;
	shared_valueOut = shared_valueIn * 2;
	shared_valueOut++;
	lInitialValue = abs(shared_valueOut) + 10;
	cout << shared_valueIn << " < " << shared_valueOut << ": " << (shared_valueIn < shared_valueOut) << endl;
	cout << shared_valueIn << " < " << lInitialValue << ": " << (shared_valueIn < lInitialValue) << endl;
	cout << lInitialValue << " < " << shared_valueOut << ": " << (lInitialValue < shared_valueOut) << endl;

	// Initialisation d'une variable partagee en entree
	lInitialValue = 5000000000;
	shared_valueIn = lInitialValue;

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_valueIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value " << lInitialValue << endl;
	cout << "Serialized value " << shared_valueOut << endl;
	;
	cout << "Serialized form " << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object " << shared_valueOut << endl;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_String

PLShared_String::PLShared_String(void)
{
	sValue = "";
}

PLShared_String::~PLShared_String(void) {}

void PLShared_String::Write(ostream& ost) const
{
	ost << GetValue();
}

void PLShared_String::SerializeValue(PLSerializer* serializer) const
{
	serializer->PutString(sValue);
}

void PLShared_String::DeserializeValue(PLSerializer* serializer)
{
	sValue = serializer->GetString();
}

void PLShared_String::Test()
{
	PLShared_String shared_valueIn;
	PLShared_String shared_valueOut;
	PLSerializer serializer;
	ALString sInitialValue;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_valueIn.bIsDeclared = true;
	shared_valueOut.bIsDeclared = true;

	// Initialisation d'une variable partagee en entree
	sInitialValue = "Une chaine de caracteres pour tester la serialisation.\nUne autre avec un retour a la ligne, "
			"quelques signes de \"ponctuations\" et des '\t' tabulations.";
	shared_valueIn.SetValue(sInitialValue);

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_valueIn.Serialize(&serializer);
	serializer.Close();

	// Deserialisation vers une variable partagee en sortie
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();

	// Affichage des resultats
	cout << "Initial value : " << sInitialValue << endl;
	cout << "Serialized value : " << shared_valueOut << endl;
	;
	cout << "Serialized form : " << serializer << endl;

	// Deserialisation une deuxieme fois sur le meme objet
	serializer.OpenForRead(NULL);
	shared_valueOut.Deserialize(&serializer);
	serializer.Close();
	cout << "Deserialized value on the same object : " << shared_valueOut << endl;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_Char

PLShared_Char::PLShared_Char(void)
{
	cValue = '\0';
}

PLShared_Char::~PLShared_Char(void) {}

void PLShared_Char::Write(ostream& ost) const
{
	ost << GetValue();
}

void PLShared_Char::SerializeValue(PLSerializer* serializer) const
{
	serializer->PutChar(cValue);
}

void PLShared_Char::DeserializeValue(PLSerializer* serializer)
{
	cValue = serializer->GetChar();
}

void PLShared_Char::Test()
{
	PLShared_Char shared_valueIn;
	PLShared_Char shared_valueOut;
	PLSerializer serializer;

	// Declaration des variables partagee pour pouvoir les utiliser
	shared_valueIn.bIsDeclared = true;
	shared_valueOut.bIsDeclared = true;

	shared_valueOut.SetValue('c');

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_valueOut.Serialize(&serializer);
	serializer.Close();

	// Deserialisation
	serializer.OpenForRead(NULL);
	shared_valueIn.Deserialize(&serializer);
	serializer.Close();

	cout << "Input value : " << shared_valueOut << endl;
	cout << "Output value : " << shared_valueIn << endl;
}