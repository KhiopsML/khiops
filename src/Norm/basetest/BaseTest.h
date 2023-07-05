// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Timer.h"
#include "Standard.h"
#include "Ermgt.h"
#include "ALString.h"
#include "Object.h"
#include "SortedList.h"
#include "UITestClassSpecArrayView.h"
#include "UITest.h"
#include "MemoryManager.h"
#include "MemoryStatsManager.h"
#include "SystemResource.h"
#include "CharVector.h"
#include "InputBufferedFile.h"
#include "OutputBufferedFile.h"
#include "Regexp.h"
#include "DiversTests.h"
#include "UITestObject.h"
#include "UITestObjectView.h"

// Classe de test des composants de base
class TestBaseComponents : public UICard
{
public:
	// Constructeur
	TestBaseComponents();
	~TestBaseComponents();

	// Listes des actions
	void SystemResourceTest();
	void MemTest();
	void RandomTest();
	void ErrorTest();
	void FileServiceTest();
	void ALStringTest();
	void ALRegexTest();
	void TimerTest();
	void ObjectArrayTest();
	void ObjectListTest();
	void SortedListTest();
	void ObjectDictionaryTest();
	void NumericKeyDictionaryTest();
	void DoubleVectorTest();
	void IntVectorTest();
	void LongintVectorTest();
	void CharVectorTest();
	void StringVectorTest();
	void UserInterfaceTest();
	void UserInterfaceTestView();
	void UserInterfaceTestFileChooser();
	void FileReaderCardTest();

	// Test en mode interactif des composants
	static void InteractiveTest();

	// Test en mode batch des composants (hors interface graphique);
	static void BatchTest();

	///////////////////////////////////////////////
	///// Implementation
protected:
	// Tableau d'objets edites
	ObjectArray oaUITestObjects;
};