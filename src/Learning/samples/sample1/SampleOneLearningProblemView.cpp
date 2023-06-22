// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SampleOneLearningProblemView.h"

SampleOneLearningProblemView::SampleOneLearningProblemView()
{
	KWAnalysisSpecView* analysisSpecView;

	// Libelles
	SetIdentifier("SampleOneLearningProblem");
	SetLabel("Sample One");

	analysisSpecView = cast(KWAnalysisSpecView*, GetFieldAt("AnalysisSpec"));

	cast(KWModelingSpecView*, analysisSpecView->GetFieldAt("ModelingSpec"))->SetVisible(false);
	cast(KWAttributeConstructionSpecView*, analysisSpecView->GetFieldAt("AttributeConstructionSpec"))
	    ->SetVisible(false);
	cast(KWPreprocessingSpecView*, analysisSpecView->GetFieldAt("PreprocessingSpec"))->SetVisible(false);
	cast(KWSystemParametersView*, analysisSpecView->GetFieldAt("SystemParameters"))->SetVisible(false);

	// On rend non visible les onglets inutiles
	cast(KWDatabaseView*, GetFieldAt("TestDatabase"))->SetVisible(false);
	cast(KWAnalysisResultsView*, GetFieldAt("AnalysisResults"))->SetVisible(false);
	cast(KWLearningProblemActionView*, GetFieldAt("LearningTools"))->SetVisible(false);

	// On rend non visible les boutons inutiles
	GetActionAt("ComputeStats")->SetVisible(false);
	GetActionAt("TransferDatabase")->SetVisible(false);

	// Ajout d'actions boutons
	AddAction("ShowClass", "Show class", (ActionMethod)(&SampleOneLearningProblemView::ShowClass));
	AddAction("ShowObject", "Show object", (ActionMethod)(&SampleOneLearningProblemView::ShowObject));
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

void SampleOneLearningProblemView::CheckLearningSpec()
{
	if (not GetLearningProblem()->CheckTrainDatabaseName() or
	    not GetLearningProblem()->GetTrainDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetTrainDatabase()->GetSelectionValue()) or
	    not GetLearningProblem()->CheckClass())
		AddMessage("Specifications valides");
}

void SampleOneLearningProblemView::ShowClass()
{
	// Prerequis: validite operationnelle des specifications
	if (not GetLearningProblem()->CheckClass())
		AddError("Specifications non valides");
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
	if (not GetLearningProblem()->CheckTrainDatabaseName() or
	    not GetLearningProblem()->GetTrainDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetTrainDatabase()->GetSelectionValue()) or
	    not GetLearningProblem()->CheckClass())
		AddError("Specifications non valides");
	else
	{
		KWClass* kwcClass;
		KWDatabase* database;
		KWObject* kwoObject;
		ALString sValue;
		ALString sTmp;
		int nObjectIndex;
		int nIndex;
		int nAttribute;
		KWLoadIndex liAttributeLoadIndex;

		// Demande de l'index de l'objet a visualiser
		nObjectIndex = GetIntValue("Index de l'instance a afficher", 1);

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
				nIndex++;
				kwoObject = database->Read();

				// On detruit les objets ne correspondant pas à l'index recherche
				if (nIndex != nObjectIndex)
				{
					if (kwoObject != NULL)
					{
						delete kwoObject;
						kwoObject = NULL;
					}
				}
				// Sinon, on s'arrete
				else
					break;
			}

			// Fermeture de la base
			database->Close();
		}

		// Message d'erreur si objet non trouve
		if (kwoObject == NULL)
			AddError(sTmp + "Pas d'objet trouve a l'index " + IntToString(nObjectIndex));
		// Sinon, affichage de l'objet
		else
		{
			// Entete
			AddSimpleMessage(sTmp + "Instance " + IntToString(nObjectIndex) + " de " +
					 database->GetDatabaseName());

			// Valeurs des attributs
			assert(kwcClass->IsIndexed());
			for (nAttribute = 0; nAttribute < kwcClass->GetLoadedAttributeNumber(); nAttribute++)
			{
				// Acces a l'index de chargement de l'attribut
				liAttributeLoadIndex = kwcClass->GetLoadedAttributeAt(nAttribute)->GetLoadIndex();

				// Transcodage de la valeur en chaine de caractere selo le type d'attribut
				if (kwcClass->GetLoadedAttributeAt(nAttribute)->GetType() == KWType::Symbol)
					sValue = kwoObject->GetSymbolValueAt(liAttributeLoadIndex);
				else
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
	if (not GetLearningProblem()->CheckTrainDatabaseName() or
	    not GetLearningProblem()->GetTrainDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetTrainDatabase()->GetSelectionValue()) or
	    not GetLearningProblem()->CheckClass())
		AddError("Specifications non valides");
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
		if (classStats.GetTargetAttributeType() == KWType::Symbol or
		    classStats.GetTargetAttributeType() == KWType::None)
			classStats.SetMaxAttributePairNumber(GetLearningProblem()
								 ->GetAnalysisSpec()
								 ->GetAttributeConstructionSpec()
								 ->GetMaxAttributePairNumber());
		if (classStats.GetTargetAttributeType() == KWType::Continuous and
		    GetLearningProblem()
			    ->GetAnalysisSpec()
			    ->GetAttributeConstructionSpec()
			    ->GetMaxAttributePairNumber() > 0)
			Global::AddWarning("", "", "2D analysis: not available in regression analysis");
		if (classStats.GetTargetAttributeType() == KWType::None and GetLearningProblem()
										    ->GetAnalysisSpec()
										    ->GetAttributeConstructionSpec()
										    ->GetMaxAttributePairNumber() == 0)
			Global::AddWarning("", "",
					   "2D analysis is available in unsupervised analysis: you should try it");
		assert(classStats.Check());

		// Calcul des statistiques
		classStats.ComputeStats();

		// Demande du nom du rapport
		sReportName = GetStringValue("Nom du rapport", "Rapport.xls");

		// Ecriture d'un rapport
		classStats.WriteReportFile(sReportName);
	}
}