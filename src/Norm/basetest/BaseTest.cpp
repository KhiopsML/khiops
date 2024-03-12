// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "BaseTest.h"

///////////////////////////////////////////////////////
// Classe TestBaseComponents

TestBaseComponents::TestBaseComponents()
{
	SetIdentifier("TestBaseComponents");
	SetLabel("Test des composants de base");

	// Declaration des actions
	AddAction("SystemResourceTest", "Test System Resource", (ActionMethod)&TestBaseComponents::SystemResourceTest);
	AddAction("MemTest", "Test de la gestion de la memoire", (ActionMethod)&TestBaseComponents::MemTest);
	AddAction("RandomTest", "Test du generateur de nombre aleatoire",
		  (ActionMethod)&TestBaseComponents::RandomTest);
	AddAction("ErrorTest", "Test de la gestion d'erreur", (ActionMethod)&TestBaseComponents::ErrorTest);
	AddAction("FileServiceTest", "Test des utilitaires de gestion des fichiers",
		  (ActionMethod)&TestBaseComponents::FileServiceTest);
	AddAction("ALStringTest", "Test de manipulation des chaines de caracteres",
		  (ActionMethod)&TestBaseComponents::ALStringTest);
	AddAction("ALRegexTest", "Test des regex", (ActionMethod)&TestBaseComponents::ALRegexTest);
	AddAction("TimerTest", "Test de timer", (ActionMethod)&TestBaseComponents::TimerTest);
	AddAction("ObjectArrayTest", "Test des tableaux d'objects", (ActionMethod)&TestBaseComponents::ObjectArrayTest);
	AddAction("ObjectListTest", "Test des listes d'objects", (ActionMethod)&TestBaseComponents::ObjectListTest);
	AddAction("SortedListTest", "Test des listes triees d'objects",
		  (ActionMethod)&TestBaseComponents::SortedListTest);
	AddAction("ObjectDictionaryTest", "Test des dictionaires d'objects",
		  (ActionMethod)&TestBaseComponents::ObjectDictionaryTest);
	AddAction("NumericKeyDictionaryTest", "Test des dictionaires d'objects a cle numerique",
		  (ActionMethod)&TestBaseComponents::NumericKeyDictionaryTest);
	AddAction("LongintDictionaryTest", "Test des dictionaires de longint",
		  (ActionMethod)&TestBaseComponents::LongintDictionaryTest);
	AddAction("LongintNumericKeyDictionaryTest", "Test des dictionaires de longint a cle numerique",
		  (ActionMethod)&TestBaseComponents::LongintNumericKeyDictionaryTest);
	AddAction("DoubleVectorTest", "Test des vecteurs de doubles",
		  (ActionMethod)&TestBaseComponents::DoubleVectorTest);
	AddAction("IntVectorTest", "Test des vecteurs d'entiers", (ActionMethod)&TestBaseComponents::IntVectorTest);
	AddAction("LongintVectorTest", "Test des vecteurs d'entiers longs",
		  (ActionMethod)&TestBaseComponents::LongintVectorTest);
	AddAction("CharVectorTest", "Test des vecteurs de caracteres",
		  (ActionMethod)&TestBaseComponents::CharVectorTest);
	AddAction("StringVectorTest", "Test des vecteurs de chaines de caracteres",
		  (ActionMethod)&TestBaseComponents::StringVectorTest);
	AddAction("UserInterfaceTest", "Test de l'interface utilisateur",
		  (ActionMethod)&TestBaseComponents::UserInterfaceTest);
	AddAction("UserInterfaceTestView", "Test UI avec des View",
		  (ActionMethod)&TestBaseComponents::UserInterfaceTestView);
	AddAction("UserInterfaceTestFileChooser", "Test UI avec action de menu file chooser",
		  (ActionMethod)&TestBaseComponents::UserInterfaceTestFileChooser);
	AddAction("FileReaderCardTest", "Test UI avec fiche de lecture de fichier",
		  (ActionMethod)&TestBaseComponents::FileReaderCardTest);
}

TestBaseComponents::~TestBaseComponents()
{
	oaUITestObjects.DeleteAll();
}

void TestBaseComponents::SystemResourceTest()
{
	TestSystemResource();
}

void TestBaseComponents::MemTest()
{
	TestMemory();
}

//  Test du generateur aleatoire
void TestBaseComponents::RandomTest()
{
	TestRandom();
}

void TestBaseComponents::ErrorTest()
{
	Error::Test();
}

void TestBaseComponents::FileServiceTest()
{
	FileService::Test();
}

void TestBaseComponents::ALStringTest()
{
	ALString::Test();
}

void TestBaseComponents::ALRegexTest()
{
	Regex::Test();
}

void TestBaseComponents::TimerTest()
{
	Timer::Test();
}

void TestBaseComponents::ObjectArrayTest()
{
	ObjectArray::Test();
}

void TestBaseComponents::ObjectListTest()
{
	ObjectList::Test();
}

void TestBaseComponents::SortedListTest()
{
	SortedList::Test();
}

void TestBaseComponents::ObjectDictionaryTest()
{
	ObjectDictionary::Test();
}

void TestBaseComponents::NumericKeyDictionaryTest()
{
	NumericKeyDictionary::Test();
}

void TestBaseComponents::LongintDictionaryTest()
{
	LongintDictionary::Test();
}

void TestBaseComponents::LongintNumericKeyDictionaryTest()
{
	LongintNumericKeyDictionary::Test();
}

void TestBaseComponents::DoubleVectorTest()
{
	DoubleVector::Test();
}

void TestBaseComponents::IntVectorTest()
{
	IntVector::Test();
}

void TestBaseComponents::LongintVectorTest()
{
	LongintVector::Test();
}

void TestBaseComponents::CharVectorTest()
{
	CharVector::Test();
}

void TestBaseComponents::StringVectorTest()
{
	StringVector::Test();
}

void TestBaseComponents::UserInterfaceTest()
{
	UITest test;
	UIConfirmationCard confirmationCard;
	boolean bContinue;

	// On reste dans l'interface tant que l'on ne confirme pas la sortie
	UIObject::SetUIMode(UIObject::Graphic);
	bContinue = true;
	while (bContinue)
	{
		test.Open();
		bContinue = not confirmationCard.OpenAndConfirm();
	}
	UIObject::SetUIMode(UIObject::Textual);
}

void TestBaseComponents::UserInterfaceTestView()
{
	UITestClassSpecArrayView testArrayView;

	UIObject::SetUIMode(UIObject::Graphic);
	testArrayView.SetObjectArray(&oaUITestObjects);
	testArrayView.SetLineNumber(7);
	testArrayView.Open();
	UIObject::SetUIMode(UIObject::Textual);
}

void TestBaseComponents::UserInterfaceTestFileChooser()
{
	boolean bReOpen = false;
	UITestObject testObject;
	UITestObjectView testObjectView;

	// Parametrage de a fiche d'interface
	UIObject::SetUIMode(UIObject::Graphic);
	testObjectView.SetObject(&testObject);
	testObjectView.Check();

	// Ouverture en testant la synchornisation avec Java
	cout << "TestObjectView Start" << endl;
	cout << "Input object\n" << testObject << *testObject.GetSubObject() << *testObject.GetActionSubObject();
	testObjectView.Open();
	cout << "Output object\n" << testObject << *testObject.GetSubObject() << *testObject.GetActionSubObject();
	cout << "TestObjectView Stop" << endl;

	// Re-ouverture en testant la synchronisation avec Java
	if (bReOpen)
	{
		cout << "=================================================" << endl;
		cout << "TestObjectView Start2" << endl;
		testObjectView.Open();
		cout << "TestObjectView Stop2" << endl;
	}
	UIObject::SetUIMode(UIObject::Textual);
}

void TestBaseComponents::FileReaderCardTest()
{
	FileReaderCard fileReaderCard;

	UIObject::SetUIMode(UIObject::Graphic);
	fileReaderCard.Open();
	UIObject::SetUIMode(UIObject::Textual);
}

void TestBaseComponents::InteractiveTest()
{
	TestBaseComponents tests;

	UIObject::SetUIMode(UIObject::Textual);
	UIObject::SetTextualInteractiveModeAllowed(true);
	Error::SetDisplayErrorFunction(Error::GetDefaultDisplayErrorFunction());
	tests.Open();
}

void TestBaseComponents::BatchTest()
{
	TestBaseComponents tests;
	const ALString sHeader = "=========================================================\n";
	UIAction* action;
	int i;

	// On passe en mode textuel
	UIObject::SetUIMode(UIObject::Textual);

	// On passe en mode batch pour avoir des parametres par defaut, sans interaction utilisateur
	SetAcquireBatchMode(true);

	// Lancement de tous les tests
	for (i = 0; i < tests.GetActionNumber(); i++)
	{
		action = tests.GetActionAtIndex(i);

		// On saute les action predefinies
		if (action->GetIdentifier() == "Exit" or action->GetIdentifier() == "Refresh")
			continue;

		// Arret si premiere action avec interface
		if (action->GetIdentifier().Find("View") >= 0 or action->GetIdentifier().Find("UserInterface") >= 0)
			break;

		// Execution de l'action
		cout << sHeader << action->GetLabel() << endl;
		MemoryStatsManager::AddLog("Start " + action->GetLabel());
		(tests.*action->GetActionMethod())();
		MemoryStatsManager::AddLog("Stop " + action->GetLabel());
	}

	// Restitution des modes standard
	UIObject::SetUIMode(UIObject::Graphic);
	SetAcquireBatchMode(false);
}
