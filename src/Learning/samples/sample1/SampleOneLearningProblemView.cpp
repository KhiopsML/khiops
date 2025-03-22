// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SampleOneLearningProblemView.h"

SampleOneLearningProblemView::SampleOneLearningProblemView()
{
	KWAnalysisSpecView* analysisSpecView;
	int i;

	// Libelles
	SetIdentifier("SampleOneLearningProblem");
	SetLabel("Sample One");

	// Dans l'onglet des parametres d'analyse, on met tout en invisible, sauf le choix de la variable cible
	analysisSpecView = cast(KWAnalysisSpecView*, GetFieldAt("AnalysisSpec"));
	for (i = 0; i < analysisSpecView->GetFieldNumber(); i++)
		analysisSpecView->GetFieldAtIndex(i)->SetVisible(false);
	analysisSpecView->GetFieldAt("TargetAttributeName")->SetVisible(true);

	// On rend non visible les onglets inutiles
	cast(KWAnalysisResultsView*, GetFieldAt("AnalysisResults"))->SetVisible(false);
	cast(KWLearningProblemActionView*, GetFieldAt("LearningTools"))->SetVisible(false);

	// On rend non visible les boutons inutiles
	GetActionAt("ComputeStats")->SetVisible(false);
	GetActionAt("TransferDatabase")->SetVisible(false);

	// Ajout d'actions boutons
	AddAction("ShowClass", "Show dictionary", (ActionMethod)(&SampleOneLearningProblemView::ShowClass));
	AddAction("ShowObject", "Show instance", (ActionMethod)(&SampleOneLearningProblemView::ShowObject));
	AddAction("ExportStats", "Export stats", (ActionMethod)(&SampleOneLearningProblemView::ExportStats));
	GetActionAt("ShowClass")->SetStyle("Button");
	GetActionAt("ShowObject")->SetStyle("Button");
	GetActionAt("ExportStats")->SetStyle("Button");
}

SampleOneLearningProblemView::~SampleOneLearningProblemView() {}

void SampleOneLearningProblemView::SetObject(Object* object)
{
	SampleOneLearningProblem* learningProblem;

	require(object != NULL);

	// Appel de la methode ancetre
	KWLearningProblemView::SetObject(object);

	// Acces a l'objet edite
	learningProblem = cast(SampleOneLearningProblem*, object);
}

SampleOneLearningProblem* SampleOneLearningProblemView::GetSampleOneLearningProblem()
{
	return cast(SampleOneLearningProblem*, objValue);
}

boolean SampleOneLearningProblemView::CheckLearningSpec()
{
	return GetLearningProblem()->CheckClass() and GetLearningProblem()->CheckTrainDatabaseName() and
	       GetLearningProblem()->GetTrainDatabase()->Check();
}

void SampleOneLearningProblemView::ShowClass()
{
	// Prerequis: validite operationnelle des specifications
	if (not CheckLearningSpec())
		AddError("Invalid specifications");
	else
	{
		KWClass* kwClass;
		KWAttribute* kwAttribute;

		// Extraction du dictionnaire
		kwClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetLearningProblem()->GetClassName());

		// Affichage des attributs (utilises) du dictionnaire
		AddSimpleMessage("Dictionnaire " + kwClass->GetName());
		kwAttribute = kwClass->GetHeadAttribute();
		while (kwAttribute != NULL)
		{
			if (kwAttribute->GetUsed())
				AddSimpleMessage("\t" + KWType::ToString(kwAttribute->GetType()) + " " +
						 kwAttribute->GetName());
			kwClass->GetNextAttribute(kwAttribute);
		}
	}
}

void SampleOneLearningProblemView::ShowObject()
{
	// Prerequis: validite operationnelle des specifications
	if (not CheckLearningSpec())
		AddError("Invalid specifications");
	else
	{
		KWClass* kwcClass;
		KWAttribute* attribute;
		KWDatabase* database;
		KWObject* kwoObject;
		ALString sValue;
		ALString sTmp;
		int nObjectIndex;
		int nIndex;
		int nAttribute;
		KWLoadIndex liAttributeLoadIndex;

		// Demande de l'index de l'objet a visualiser
		nObjectIndex = GetIntValue("Index of the instance to show", 1);

		// Acces a la base de donnees et au dictionnaire
		database = GetLearningProblem()->GetTrainDatabase();
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetLearningProblem()->GetClassName());

		// Recherche de l'objet pour l'index demande
		kwoObject = NULL;
		if (database->OpenForRead())
		{
			// Lecture des objets un a un
			nIndex = 0;
			while (not database->IsEnd())
			{
				kwoObject = database->Read();

				// Traitement des objet lus
				if (kwoObject != NULL)
				{
					nIndex++;

					// Arret si objet trouve
					if (nIndex == nObjectIndex)
						break;
					// Destruction de l'objet sinon
					else
					{
						delete kwoObject;
						kwoObject = NULL;
					}
				}
			}

			// Fermeture de la base
			database->Close();
		}

		// Message d'erreur si objet non trouve
		if (kwoObject == NULL)
			AddError(sTmp + "Instance not found for index " + IntToString(nObjectIndex));
		// Sinon, affichage de l'objet
		else
		{
			// Entete, en precisant l'id de creation de l'instance (numero de ligne dans le fichier)
			AddSimpleMessage(sTmp + "Instance " + IntToString(nObjectIndex) + " of " +
					 database->GetDatabaseName() +
					 " (id=" + LongintToString(kwoObject->GetCreationIndex()) + ")");

			// Valeurs des attributs
			assert(kwcClass->IsIndexed());
			for (nAttribute = 0; nAttribute < kwcClass->GetLoadedAttributeNumber(); nAttribute++)
			{
				attribute = kwcClass->GetLoadedAttributeAt(nAttribute);

				// Acces a l'index de chargement de l'attribut
				liAttributeLoadIndex = attribute->GetLoadIndex();

				// Transcodage de la valeur en chaine de caractere selon le type d'attribut
				if (attribute->GetType() == KWType::Symbol)
					sValue = kwoObject->GetSymbolValueAt(liAttributeLoadIndex);
				else if (attribute->GetType() == KWType::Continuous)
					sValue = KWContinuous::ContinuousToString(
					    kwoObject->GetContinuousValueAt(liAttributeLoadIndex));

				// Affichage du nom de l'attribut et de sa valeur
				AddSimpleMessage(sTmp + "\t" + kwcClass->GetLoadedAttributeAt(nAttribute)->GetName() +
						 ": " + sValue);
			}

			// Destruction de l'objet
			delete kwoObject;
		}
	}
}

void SampleOneLearningProblemView::ExportStats()
{
	// Prerequis: validite operationnelle des specifications
	if (not CheckLearningSpec())
		AddError("Invalid specifications");
	else
	{
		KWLearningSpec learningSpec;
		KWClassStats classStats;
		KWClass* kwClass;
		ALString sReportName;

		// Parametrage pour une discretisation EqualFrequency en 10 intervalles, en supervise et non supervise
		GetLearningProblem()->GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName(
		    "EqualFrequency");
		GetLearningProblem()->GetPreprocessingSpec()->GetDiscretizerSpec()->SetUnsupervisedMethodName(
		    "EqualFrequency");
		GetLearningProblem()->GetPreprocessingSpec()->GetDiscretizerSpec()->SetParam(0);
		GetLearningProblem()->GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(10);

		// Acces au dictionnaire
		kwClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetLearningProblem()->GetClassName());

		// Initialisation des objets de calculs des statistiques
		// InitializeLearningSpec
		learningSpec.SetDatabase(GetLearningProblem()->GetTrainDatabase());
		learningSpec.SetClass(kwClass);
		learningSpec.SetTargetAttributeName(GetLearningProblem()->GetTargetAttributeName());
		learningSpec.SetMainTargetModality((const char*)GetLearningProblem()->GetMainTargetModality());
		learningSpec.GetPreprocessingSpec()->CopyFrom(GetLearningProblem()->GetPreprocessingSpec());

		// InitializeClassStats
		// Parametrage de classStats
		classStats.SetLearningSpec(&learningSpec);

		// Parametrage pour le nombre de paires d'attributs a evaluer
		classStats.SetAttributePairsSpec(GetLearningProblem()
						     ->GetAnalysisSpec()
						     ->GetModelingSpec()
						     ->GetAttributeConstructionSpec()
						     ->GetAttributePairsSpec());
		assert(classStats.Check());

		// Calcul des statistiques
		classStats.ComputeStats();

		// Demande du nom du rapport
		sReportName = GetStringValue("Report name", "Report.xls");

		// Parametrage des options du rapport
		classStats.SetAllWriteOptions(false);
		classStats.SetWriteOptionStats1D(true);

		// Ecriture d'un rapport
		AddSimpleMessage("Write " + sReportName);
		classStats.WriteReportFile(sReportName);
	}
}
