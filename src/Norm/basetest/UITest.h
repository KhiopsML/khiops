// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "TaskProgression.h"
#include "FileService.h"

////////////////////////////////////////////////////////////////////////
// Classe StringList
// Test d'une liste contenant des chaines de caracteres
class StringList : public UIList
{
public:
	// Constructeur
	StringList();
	~StringList();
};

////////////////////////////////////////////////////////////////////////
// Classe StringCard
// Test d'une fiche contenant des chaines de caracteres
class StringCard : public UICard
{
public:
	// Constructeur
	StringCard();
	~StringCard();
};

////////////////////////////////////////////////////////////////////////////////////
// Classe TextAreaCard
// Test d'une fiche contenant des chaines de caracteres dans une boite multi-lignes

class TextAreaCard : public UICard
{
public:
	// Constructeur
	TextAreaCard();
	~TextAreaCard();

	// Test
	static void Test();
};

////////////////////////////////////////////////////////////////////////////////////
// Classe FormattedLabelCard
// Test d'une fiche contenant des chaines de caracteres sous forme d'un libelle formatte

class FormattedLabelCard : public UICard
{
public:
	// Constructeur
	FormattedLabelCard();
	~FormattedLabelCard();

	// Test
	static void Test();
};

////////////////////////////////////////////////////////////////////////////////////
// Classe LabelCard
// Test d'une fiche contenant des chaines de caracteres sous forme de libelles

class LabelCard : public UICard
{
public:
	// Constructeur
	LabelCard();
	~LabelCard();

	// Test
	static void Test();
};

////////////////////////////////////////////////////////////////////////
// Classe CharList
// Test d'une liste contenant des caracteres
class CharList : public UIList
{
public:
	// Constructeur
	CharList();
	~CharList();
};

////////////////////////////////////////////////////////////////////////
// Classe CharCard
// Test d'une fiche contenant des caracteres
class CharCard : public UICard
{
public:
	// Constructeur
	CharCard();
	~CharCard();
};

////////////////////////////////////////////////////////////////////////
// Classe BooleanList
// Test d'une liste contenant des booleens
class BooleanList : public UIList
{
public:
	// Constructeur
	BooleanList();
	~BooleanList();
};

////////////////////////////////////////////////////////////////////////
// Classe BooleanCard
// Test d'une fiche contenant des booleens
class BooleanCard : public UICard
{
public:
	// Constructeur
	BooleanCard();
	~BooleanCard();
};

////////////////////////////////////////////////////////////////////////
// Classe IntList
// Test d'une liste contenant des entiers
class IntList : public UIList
{
public:
	// Constructeur
	IntList();
	~IntList();
};

////////////////////////////////////////////////////////////////////////
// Classe IntCard
// Test d'une fiche contenant des entiers
class IntCard : public UICard
{
public:
	// Constructeur
	IntCard();
	~IntCard();
};

////////////////////////////////////////////////////////////////////////
// Classe DoubleList
// Test d'une liste contenant des reels
class DoubleList : public UIList
{
public:
	// Constructeur
	DoubleList();
	~DoubleList();
};

////////////////////////////////////////////////////////////////////////
// Classe DoubleCard
// Test d'une fiche contenant des reels
class DoubleCard : public UICard
{
public:
	// Constructeur
	DoubleCard();
	~DoubleCard();
};

////////////////////////////////////////////////////////////////////////
// Classe UITest
// Test de l'interface
class UITest : public UICard
{
public:
	// Constructeur
	UITest();
	~UITest();

	// Actions disponibles
	void DisplaySampleObject();
	void DisplaySampleObjectArray();
	void ProgressionTest();

	///// Implementation
protected:
	// Gestion d'un SampleObject et d'un tableau de SampleObject
	SampleObject aSampleObject;
	ObjectArray oaSampleObjects;
};

////////////////////////////////////////////////////////////////////////
// Classe FileReaderCard
// Test d'une fiche permettant la lecture d'un fichier
// Permet notament de tester la prise en compte des caracteres accentues
// dans les echanges entre Java et C++
class FileReaderCard : public UICard
{
public:
	// Constructeur
	FileReaderCard();
	~FileReaderCard();

	// Action
	void OpenFile();
	void CreateFiles();
	void TestFileServices();

	// Test
	static void Test();

	///// Implementation
protected:
	static const ALString sRootPath;
};
