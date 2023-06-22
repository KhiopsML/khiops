// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:58
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWBenchmarkSpecView.h"

KWBenchmarkSpecView::KWBenchmarkSpecView()
{
	SetIdentifier("KWBenchmarkSpec");
	SetLabel("Benchmark");
	AddStringField("ClassName", "Dictionary", "");
	AddStringField("TargetAttributeName", "Target variable", "");
	AddStringField("DatabaseName", "Database file", "");

	// ## Custom constructor

	// Initialisation des domaines de classe
	currentClassDomain = NULL;
	temporaryClassDomain = NULL;

	// Ajout d'une fiche d'edition des specification du dictionnaire du benchmark
	KWBenchmarkClassSpecView* benchmarkClassSpecView;
	benchmarkClassSpecView = new KWBenchmarkClassSpecView;
	AddCardField("BenchmarkClassSpec", "Benchmark dictionary", benchmarkClassSpecView);

	// Ajout d'une fiche d'edition de la base
	KWDatabaseView* databaseView;
	databaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();
	AddCardField("Database", "Database", databaseView);

	// Personnalisation de l'action Exit
	GetActionAt("Exit")->SetActionMethod((ActionMethod)(&KWBenchmarkSpecView::ActionExit));

	// Parametrage des champs visibles pour les specifications de benchmark
	GetFieldAt("ClassName")->SetVisible(false);
	GetFieldAt("TargetAttributeName")->SetVisible(false);
	GetFieldAt("DatabaseName")->SetVisible(false);

	// Parametrage des champs visibles pour la base
	databaseView->GetFieldAt("ClassName")->SetVisible(false);
	databaseView->GetFieldAt("SampleNumberPercentage")->SetVisible(false);
	databaseView->GetFieldAt("SamplingMode")->SetVisible(false);
	databaseView->GetFieldAt("SelectionAttribute")->SetVisible(false);
	databaseView->GetFieldAt("SelectionValue")->SetVisible(false);

	// ##
}

KWBenchmarkSpecView::~KWBenchmarkSpecView()
{
	// ## Custom destructor

	// ##
}

KWBenchmarkSpec* KWBenchmarkSpecView::GetKWBenchmarkSpec()
{
	require(objValue != NULL);
	return cast(KWBenchmarkSpec*, objValue);
}

void KWBenchmarkSpecView::EventUpdate(Object* object)
{
	KWBenchmarkSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWBenchmarkSpec*, object);

	// ## Custom update

	// Lecture des classes du benchmark si la fenetre est ouverte
	// Une mise a jour du fichier des classes (depuis KWBenchmarkClassSpec) rend en effet
	// eventuellement necessaire une relecture des classes, notamment pour l'edition d'un mapping dans le cas
	// multi-tables
	if (IsOpened())
		editedObject->GetBenchmarkClassSpec()->ReadClasses();

	// Synchronisation des specifications des bases d'apprentissage et de test avec le dictionnaire en cours
	editedObject->GetBenchmarkDatabase()->SetClassName(editedObject->GetBenchmarkClassSpec()->GetClassName());

	// Synchronisation egalement des interfaces pour forcer la coherence entre interface et objet edite
	cast(KWDatabaseView*, GetFieldAt("Database"))
	    ->SetStringValueAt("ClassName", editedObject->GetBenchmarkDatabase()->GetClassName());

	// ##
}

void KWBenchmarkSpecView::EventRefresh(Object* object)
{
	KWBenchmarkSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWBenchmarkSpec*, object);
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetStringValueAt("TargetAttributeName", editedObject->GetTargetAttributeName());
	SetStringValueAt("DatabaseName", editedObject->GetDatabaseName());

	// ## Custom refresh

	// ##
}

const ALString KWBenchmarkSpecView::GetClassLabel() const
{
	return "Benchmark";
}

// ## Method implementation

void KWBenchmarkSpecView::SetObject(Object* object)
{
	KWBenchmarkSpec* benchmarkSpec;

	require(object != NULL);

	// Acces a l'objet edite
	benchmarkSpec = cast(KWBenchmarkSpec*, object);

	// Lecture des classes du benchmark, devant etre presente lors de l'ouverture de la fenetre
	// Utile notamment pour l'edition d'un mapping dans le cas multi-tables
	// Doit etre fait avant le parametrage de la base
	benchmarkSpec->GetBenchmarkClassSpec()->ReadClasses();

	// Parametrage des sous-fiches
	cast(KWBenchmarkClassSpecView*, GetFieldAt("BenchmarkClassSpec"))
	    ->SetObject(benchmarkSpec->GetBenchmarkClassSpec());
	cast(KWDatabaseView*, GetFieldAt("Database"))->SetObject(benchmarkSpec->GetBenchmarkDatabase());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

Object* KWBenchmarkSpecView::GetObject()
{
	KWBenchmarkSpec* benchmarkSpec;

	benchmarkSpec = cast(KWBenchmarkSpec*, UIObjectView::GetObject());

	// Supression des classes du benchmark
	benchmarkSpec->GetBenchmarkClassSpec()->DropClasses();

	return benchmarkSpec;
}

KWBenchmarkSpec* KWBenchmarkSpecView::GetBenchmarkSpec()
{
	return cast(KWBenchmarkSpec*, GetObject());
}

// ##
