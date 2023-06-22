// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "QueryServicesView.h"

//////////////////////////////////////////////////
// Implementation de la classe QueryServicesView

QueryServicesView::QueryServicesView(QueryServices* qsValue, UIObjectArrayView* objectsView)
{
	require(qsValue != NULL);
	require(objectsView != NULL);

	qsQueryServices = qsValue;
	oaCurrentQuery = NULL;

	// Titre
	SetLabel("Queries on " + qsQueryServices->GetClassName() + " by " + qsQueryServices->GetAttributeName());

	// Valeur pour la recherche des statistiques
	AddStringField("Value", "Value", "");

	// Nombre d'items trouves
	AddIntField("ItemNumber", "Item number", 0);
	GetFieldAt("ItemNumber")->SetEditable(false);

	// Liste des objets trouves
	queryResultField = objectsView;
	AddListField("QueryResult." + queryResultField->GetIdentifier(), "Query result", queryResultField);
	queryResultField->SetEditable(false);

	// Ajout des actions
	AddAction("QueryObjectsMatching", "Extract items matching value",
		  (ActionMethod)&QueryServicesView::QueryObjectsMatching);
	AddAction("QueryAllObjects", "Extract all items", (ActionMethod)&QueryServicesView::QueryAllObjects);
	AddAction("QueryStatistics", "Show statistics...", (ActionMethod)&QueryServicesView::QueryStatistics);

	// Declenchement d'une action initiale: extractions de tous les items
	QueryAllObjects();
}

QueryServicesView::~QueryServicesView()
{
	if (oaCurrentQuery != NULL)
		delete oaCurrentQuery;
}

void QueryServicesView::QueryAllObjects()
{
	// Requete
	if (oaCurrentQuery != NULL)
		delete oaCurrentQuery;
	oaCurrentQuery = qsQueryServices->QueryAllObjects();

	// Mise a jour de l'interface
	SetStringValueAt("Value", "");
	SetIntValueAt("ItemNumber", oaCurrentQuery->GetSize());
	queryResultField->SetObjectArray(oaCurrentQuery);
}

void QueryServicesView::QueryObjectsMatching()
{
	ALString sKey;
	Object* keyObject = NULL;
	Object* object;
	GetterFunction fGetter;

	// Recherche d'un objet avec cette cle
	sKey = GetStringValueAt("Value");
	fGetter = qsQueryServices->GetGetterFunction();
	for (int i = 0; i < qsQueryServices->GetSize(); i++)
	{
		object = qsQueryServices->GetAt(i);
		if (sKey == fGetter(object))
		{
			keyObject = object;
			break;
		}
	}

	// Requete
	if (oaCurrentQuery != NULL)
		delete oaCurrentQuery;
	if (keyObject == NULL)
		oaCurrentQuery = new ObjectArray;
	else
		oaCurrentQuery = qsQueryServices->QueryObjectsMatching(keyObject);

	// Mise a jour de l'interface
	SetIntValueAt("ItemNumber", oaCurrentQuery->GetSize());
	queryResultField->SetObjectArray(oaCurrentQuery);
}

void QueryServicesView::QueryStatistics()
{
	UIList statList;
	ObjectArray* oaStats;
	int nCurrent;
	StatObject* stat;

	// Construction d'une unite d'interface de visualisation des statistiques
	statList.SetLabel("Statistics on " + qsQueryServices->GetClassName() + " by " +
			  qsQueryServices->GetAttributeName());
	statList.AddStringField("AttributeName", qsQueryServices->GetAttributeName(), "");
	statList.AddIntField("ItemNumber", "Item number", 0);
	statList.SetEditable(false);

	// Calcul des statistiques
	oaStats = qsQueryServices->QueryStatistics();

	// Initialisation des valeurs de l'unite d'interface
	for (nCurrent = 0; nCurrent < oaStats->GetSize(); nCurrent++)
	{
		stat = cast(StatObject*, oaStats->GetAt(nCurrent));
		statList.AddItem();
		statList.SetStringValueAt("AttributeName", stat->GetAttributeValue());
		statList.SetIntValueAt("ItemNumber", stat->GetItemNumber());
	}

	// Liberations
	oaStats->DeleteAll();
	delete oaStats;

	// Affichage
	statList.Open();
}

const ALString QueryServicesView::GetClassLabel() const
{
	return "Query services view";
}

void QueryServicesView::Test(ostream& ost)
{
	ObjectArray oaTest;
	SampleObject so1(10, "a");
	SampleObject so2(3, "a");
	SampleObject so3(3, "a");
	SampleObject so4(5, "a");
	SampleObject so5(1, "a");
	SampleObject soTest;
	QueryServices qsTest;
	QueryServicesView* qsvTest;

	// Initialisation du tableau de test, et des services de query
	oaTest.Add(&so1);
	oaTest.Add(&so2);
	oaTest.Add(&so3);
	oaTest.Add(&so4);
	oaTest.Add(&so5);
	qsTest.Init(SampleObjectCompare, SampleObjectGetInt, "SampleObject", "Int");
	qsTest.LoadObjectArray(&oaTest);

	// Visualisation des services de requetes
	qsvTest = new QueryServicesView(&qsTest, new SampleObjectArrayView);
	qsvTest->Open();
	delete qsvTest;
}