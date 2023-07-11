// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// Thu Jul 15 10:55:39 2004
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UITestClassSpecArrayView.h"

UITestClassSpecArrayView::UITestClassSpecArrayView()
{
	SetIdentifier("Array.UITestClassSpec");
	SetLabel("Test UI (class spec)");
	AddStringField("ClassName", "Nom", "");
	AddIntField("AttributeNumber", "Attributs", 0);
	AddIntField("SymbolAttributeNumber", "Modaux", 0);
	AddIntField("ContinuousAttributeNumber", "Continus", 0);
	AddIntField("DerivedAttributeNumber", "Calcules", 0);

	// Card and help prameters
	SetItemView(new UITestClassSpecView);
	CopyCardHelpTexts();

	// ## Custom constructor

	// Le champ "ClassName" set de cle de selection
	SetKeyFieldId("ClassName");

	// Parametrage des styles
	GetFieldAt("AttributeNumber")->SetStyle("Spinner");

	// Certains champs ne sont pas visible dans la liste
	GetFieldAt("SymbolAttributeNumber")->SetVisible(false);
	GetFieldAt("ContinuousAttributeNumber")->SetVisible(false);
	GetFieldAt("DerivedAttributeNumber")->SetVisible(false);

	// Ergonomie de type inspectable
	SetErgonomy(UIList::Inspectable);

	// Ajout des actions
	AddAction("TestExtensiveListChanges", "Test de modifications de liste",
		  (ActionMethod)(&UITestClassSpecArrayView::TestExtensiveListChanges));
	AddAction("TestJavaCrash", "Test de crash Java", (ActionMethod)(&UITestClassSpecArrayView::TestJavaCrash));

	// ##
}

UITestClassSpecArrayView::~UITestClassSpecArrayView()
{
	// ## Custom destructor

	// ##
}

Object* UITestClassSpecArrayView::EventNew()
{
	return new UITestClassSpec;
}

void UITestClassSpecArrayView::EventUpdate(Object* object)
{
	UITestClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(UITestClassSpec*, object);
	editedObject->SetClassName(GetStringValueAt("ClassName"));
	editedObject->SetAttributeNumber(GetIntValueAt("AttributeNumber"));
	editedObject->SetSymbolAttributeNumber(GetIntValueAt("SymbolAttributeNumber"));
	editedObject->SetContinuousAttributeNumber(GetIntValueAt("ContinuousAttributeNumber"));
	editedObject->SetDerivedAttributeNumber(GetIntValueAt("DerivedAttributeNumber"));

	// ## Custom update

	// ##
}

void UITestClassSpecArrayView::EventRefresh(Object* object)
{
	UITestClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(UITestClassSpec*, object);
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetIntValueAt("AttributeNumber", editedObject->GetAttributeNumber());
	SetIntValueAt("SymbolAttributeNumber", editedObject->GetSymbolAttributeNumber());
	SetIntValueAt("ContinuousAttributeNumber", editedObject->GetContinuousAttributeNumber());
	SetIntValueAt("DerivedAttributeNumber", editedObject->GetDerivedAttributeNumber());

	// ## Custom refresh

	// ##
}

const ALString UITestClassSpecArrayView::GetClassLabel() const
{
	return "Test UI (class spec)s";
}

// ## Method implementation

void UITestClassSpecArrayView::TestExtensiveListChanges()
{
	const int nWaitIters = 100000000;
	const int nMaxChangeNumber = 1000;
	const int nMaxListSize = 20;
	ObjectArray* oaEditedObjects;
	int nChangeCount;
	ALString sTmp;
	ALString sUserLabel;
	int nNewListSize;
	int i;
	int nIter;
	int nBuzy;
	clock_t tStartTime;
	clock_t tTime;
	double dElapsedTime;

	// On recupere les objets edites
	oaEditedObjects = GetObjectArray();
	check(oaEditedObjects);

	// Demarage du suivi des taches
	TaskProgression::SetTitle("Tests de modification de liste");
	TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();
	TaskProgression::BeginTask();
	tStartTime = clock();

	// On recommance tant que la tache n'est pas interrompue
	nChangeCount = 0;
	while (not TaskProgression::IsInterruptionRequested())
	{
		nChangeCount++;
		sUserLabel = sTmp + "Modification " + IntToString(nChangeCount);
		TaskProgression::DisplayMainLabel(sUserLabel);

		// On recupere les objets edites
		oaEditedObjects = GetObjectArray();
		check(oaEditedObjects);

		// Choix d'une nouvelle taille de liste
		nNewListSize = RandomInt(nMaxListSize);
		tTime = clock();
		dElapsedTime = ((double)(tTime - tStartTime)) / CLOCKS_PER_SEC;
		sUserLabel = sTmp + SecondsToString(dElapsedTime);
		TaskProgression::DisplayLabel(sUserLabel);

		// Progression
		TaskProgression::DisplayProgression((nChangeCount * 100) / nMaxChangeNumber);

		// Retaillage du tableau d'objet
		oaEditedObjects->DeleteAll();
		TaskProgression::BeginTask();
		TaskProgression::DisplayMainLabel("Retaillage");
		for (i = 0; i < nNewListSize; i++)
		{
			oaEditedObjects->Add(new UITestClassSpec);
			sUserLabel = sTmp + "Ajout d'un nouvel item dans une liste: " + IntToString(i + 1);
			TaskProgression::DisplayLabel(sUserLabel);
			TaskProgression::DisplayProgression((i * 100) / nNewListSize);

			// Attente
			nBuzy = 0;
			for (nIter = 0; nIter < nWaitIters; nIter++)
				nBuzy++;

			// Arret si demande d'interruption
			if (TaskProgression::IsInterruptionRequested())
				break;
		}
		TaskProgression::EndTask();

		// On reparametre la vue avec les objets edites
		SetObjectArray(oaEditedObjects);
		check(oaEditedObjects);

		// Message utilisateur
		sUserLabel = sTmp + "Modification " + IntToString(nChangeCount) + ":\t" + IntToString(nNewListSize);
		AddSimpleMessage(sUserLabel);

		// Arret si tout a ete fait
		if (nChangeCount >= nMaxChangeNumber)
			break;
	}

	// Fin du suivi des taches
	TaskProgression::EndTask();
	TaskProgression::Stop();
}

void UITestClassSpecArrayView::TestJavaCrash()
{
	ObjectArray* oaEditedObjects;

	// On recupere les objets edites
	oaEditedObjects = GetObjectArray();
	check(oaEditedObjects);

	// Destruction d'u container d'objet, pour faire planter Java
	delete oaEditedObjects;
}

// ##
