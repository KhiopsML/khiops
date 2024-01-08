// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorEvaluator.h"

////////////////////////////////////////////////////////////
// Classe KWPredictorEvaluatorView

KWPredictorEvaluator::KWPredictorEvaluator()
{
	kwcdInitialClassesDomain = NULL;
	kwcdInitialCurrentDomain = NULL;
	sEvaluationFileName = "EvaluationReport.xls";
	bExportJSON = true;

	// Creation d'une base dans la technologie par defaut
	evaluationDatabase = KWDatabase::CreateDefaultDatabaseTechnology();
}

KWPredictorEvaluator::~KWPredictorEvaluator()
{
	if (kwcdInitialClassesDomain != NULL)
	{
		assert(KWClassDomain::GetCurrentDomain() != kwcdInitialClassesDomain);
		assert(KWClassDomain::LookupDomain(kwcdInitialClassesDomain->GetName()) == NULL);
		delete kwcdInitialClassesDomain;
	}
	oaEvaluatedPredictorSpecs.DeleteAll();
	delete evaluationDatabase;
}

const ALString& KWPredictorEvaluator::GetEvaluationFileName() const
{
	return sEvaluationFileName;
}

void KWPredictorEvaluator::SetEvaluationFileName(const ALString& sValue)
{
	sEvaluationFileName = sValue;
}

boolean KWPredictorEvaluator::GetExportJSON() const
{
	return bExportJSON;
}

void KWPredictorEvaluator::SetExportJSON(boolean bValue)
{
	bExportJSON = bValue;
}

const ALString KWPredictorEvaluator::GetJSONReportSuffix()
{
	return "khj";
}

const ALString KWPredictorEvaluator::GetEvaluationFilePathName() const
{
	ALString sEvaluationPathName;
	ALString sEvaluationFilePathName;

	// Acces au repertoire du fichier d'evaluation
	sEvaluationPathName = FileService::GetPathName(GetEvaluationFileName());

	// On prend celui de la base si celui-ci est vide
	if (sEvaluationPathName == "")
	{
		sEvaluationPathName = FileService::GetPathName(evaluationDatabase->GetDatabaseName());

		// Calcul du chemin complet du rapport d'evaluation
		sEvaluationFilePathName = FileService::BuildFilePathName(
		    sEvaluationPathName, FileService::GetFileName(GetEvaluationFileName()));
	}
	// Sinon, on garde le nom complet initial
	else
		sEvaluationFilePathName = GetEvaluationFileName();
	return sEvaluationFilePathName;
}

const ALString KWPredictorEvaluator::GetJSONFilePathName() const
{
	ALString sJSONFilePathName;

	if (GetEvaluationFileName() != "" and GetExportJSON())
		sJSONFilePathName = FileService::SetFileSuffix(GetEvaluationFilePathName(), GetJSONReportSuffix());
	return sJSONFilePathName;
}

const ALString& KWPredictorEvaluator::GetMainTargetModality() const
{
	return sMainTargetModality;
}

void KWPredictorEvaluator::SetMainTargetModality(const ALString& sValue)
{
	sMainTargetModality = sValue;
}

KWDatabase* KWPredictorEvaluator::GetEvaluationDatabase()
{
	return evaluationDatabase;
}

ObjectArray* KWPredictorEvaluator::GetEvaluatedPredictorSpecs()
{
	return &oaEvaluatedPredictorSpecs;
}

void KWPredictorEvaluator::FillEvaluatedPredictorSpecs()
{
	int i;
	ALString sInitialClassName;
	KWClass* kwcClass;
	ObjectArray oaClasses;
	KWTrainedPredictor* trainedPredictor;
	ObjectArray oaTrainedPredictors;
	ObjectArray oaEvaluatedPredictors;
	boolean bIsPredictor;
	ALString sReferenceTargetAttribute;
	KWEvaluatedPredictorSpec* evaluatedPredictorSpec;
	KWEvaluatedPredictorSpec* previousEvaluatedPredictorSpec;
	ObjectDictionary odEvaluatedPredictorSpecs;
	ALString sTmp;

	// Nettoyage du domaine des classes initiales
	kwcdInitialCurrentDomain = KWClassDomain::GetCurrentDomain();
	if (kwcdInitialClassesDomain != NULL)
	{
		assert(KWClassDomain::GetCurrentDomain() != kwcdInitialClassesDomain);
		assert(KWClassDomain::LookupDomain(kwcdInitialClassesDomain->GetName()) == NULL);
		delete kwcdInitialClassesDomain;
	}
	kwcdInitialClassesDomain = NULL;

	// Recherche des predicteurs compatibles
	for (i = 0; i < KWClassDomain::GetCurrentDomain()->GetClassNumber(); i++)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(i);
		trainedPredictor = NULL;

		// On determine si elle correspond a un classifieur
		if (KWTrainedPredictor::GetMetaDataPredictorType(kwcClass) == KWType::Symbol)
		{
			trainedPredictor = new KWTrainedClassifier;
			bIsPredictor = trainedPredictor->ImportPredictorClass(kwcClass);
			if (not bIsPredictor)
			{
				delete trainedPredictor;
				trainedPredictor = NULL;
			}
		}
		// On determine si elle correspond a un regresseur
		else if (KWTrainedPredictor::GetMetaDataPredictorType(kwcClass) == KWType::Continuous)
		{
			trainedPredictor = new KWTrainedRegressor;
			bIsPredictor = trainedPredictor->ImportPredictorClass(kwcClass);
			if (not bIsPredictor)
			{
				delete trainedPredictor;
				trainedPredictor = NULL;
			}
		}

		// Memorisation du predicteur construit
		if (trainedPredictor != NULL)
		{
			// Construction si necessaire du domaine de classe initial, a partir de celui du premier
			// predicteur valide
			if (kwcdInitialClassesDomain == NULL)
			{
				assert(oaTrainedPredictors.GetSize() == 0);

				// Construction d'un domaine initial a partir des specification d'un predicteur
				kwcdInitialClassesDomain = BuildInitialDomainPredictor(trainedPredictor);
				assert(DomainCheckClassesInitialNames(trainedPredictor->GetPredictorDomain()));

				// Memorisation du nom de classe initial du predicteur
				sInitialClassName = KWTrainedPredictor::GetMetaDataInitialClassName(
				    trainedPredictor->GetPredictorClass());
			}

			// Memorisation si nouveau predicteur compatible avec le domaine de classe initial
			if (oaTrainedPredictors.GetSize() == 0 or
			    DomainCheckClassesInitialNames(trainedPredictor->GetPredictorDomain()))
				oaTrainedPredictors.Add(trainedPredictor);
			// Destruction sinon
			else
			{
				AddWarning("Predictor " + trainedPredictor->GetPredictorClass()->GetName() +
					   " is ignored because the native variables of its dictionary are not "
					   "consistent with the other predictors");
				delete trainedPredictor;
				trainedPredictor = NULL;
			}
		}
	}
	assert(kwcdInitialClassesDomain == NULL or oaTrainedPredictors.GetSize() > 0);
	assert(kwcdInitialClassesDomain == NULL or sInitialClassName != "");

	// Transfert des specifications precedentes dans un dictionnaire, pour memoriser leur etat de selection
	for (i = 0; i < oaEvaluatedPredictorSpecs.GetSize(); i++)
	{
		previousEvaluatedPredictorSpec = cast(KWEvaluatedPredictorSpec*, oaEvaluatedPredictorSpecs.GetAt(i));
		odEvaluatedPredictorSpecs.SetAt(previousEvaluatedPredictorSpec->GetClassName(),
						previousEvaluatedPredictorSpec);
	}
	oaEvaluatedPredictorSpecs.RemoveAll();

	// Exploitation de tous les predicteurs a evaluer
	for (i = 0; i < oaTrainedPredictors.GetSize(); i++)
	{
		trainedPredictor = cast(KWTrainedPredictor*, oaTrainedPredictors.GetAt(i));
		assert(trainedPredictor->GetTargetAttribute() != NULL);

		// Creation d'une specification d'evaluation
		evaluatedPredictorSpec = new KWEvaluatedPredictorSpec;
		evaluatedPredictorSpec->SetEvaluated(true);
		evaluatedPredictorSpec->SetPredictorType(KWType::GetPredictorLabel(trainedPredictor->GetTargetType()));
		evaluatedPredictorSpec->SetPredictorName(trainedPredictor->GetName());
		evaluatedPredictorSpec->SetClassName(trainedPredictor->GetPredictorClass()->GetName());
		evaluatedPredictorSpec->SetTargetAttributeName(trainedPredictor->GetTargetAttribute()->GetName());
		oaEvaluatedPredictorSpecs.Add(evaluatedPredictorSpec);

		// Mise a jour de la selection en fonction de la selection precedente
		previousEvaluatedPredictorSpec =
		    cast(KWEvaluatedPredictorSpec*,
			 odEvaluatedPredictorSpecs.Lookup(evaluatedPredictorSpec->GetClassName()));
		if (previousEvaluatedPredictorSpec != NULL)
			evaluatedPredictorSpec->SetEvaluated(previousEvaluatedPredictorSpec->GetEvaluated());
	}

	// Nettoyage des specifications precedentes
	odEvaluatedPredictorSpecs.DeleteAll();

	// Destruction des predicteurs (qui n'ont ici ete utiles que pour identifier leurs specifications)
	oaTrainedPredictors.DeleteAll();

	// Renommage des classes d'une base pour passer aux classes initiales
	RenameDatabaseClasses(GetEvaluationDatabase(), kwcdInitialClassesDomain);

	// Parametrage de la base par la classe initiale correspondant aux predicteurs
	GetEvaluationDatabase()->SetClassName(sInitialClassName);

	// Warning s'il n'y a pas de dictionnaire
	if (KWClassDomain::GetCurrentDomain()->GetClassNumber() == 0)
		AddWarning("No available dictionary");
	// Warning s'il n'y a pas de predicteurs parmi les dictionnaire
	else if (oaEvaluatedPredictorSpecs.GetSize() == 0)
		AddWarning("No available predictor among the dictionaries");
}

KWClassDomain* KWPredictorEvaluator::BuildInitialDomainPredictor(KWTrainedPredictor* trainedPredictor)
{
	KWClassDomain* initialPredictorDomain;
	ALString sInitialClassName;

	// Clonage du domaine du predicteur
	initialPredictorDomain = trainedPredictor->GetPredictorDomain()->Clone();
	initialPredictorDomain->SetName("Initial");

	// Memorisation du nom de classe initial du predicteur
	sInitialClassName = KWTrainedPredictor::GetMetaDataInitialClassName(trainedPredictor->GetPredictorClass());

	// Renommage des classes avec leur nom initial
	DomainRenameClassesWithInitialNames(initialPredictorDomain);

	// On passe tous les attributs de la classe principale en Used
	// (donc tous les attributs cibles potentiels)
	assert(initialPredictorDomain->LookupClass(sInitialClassName) != NULL);
	initialPredictorDomain->LookupClass(sInitialClassName)->SetAllAttributesUsed(true);
	initialPredictorDomain->LookupClass(sInitialClassName)->SetAllAttributesLoaded(true);

	// Compilation du domaine
	initialPredictorDomain->Compile();
	return initialPredictorDomain;
}

KWClassDomain* KWPredictorEvaluator::GetInitialClassesDomain()
{
	return kwcdInitialClassesDomain;
}

void KWPredictorEvaluator::EvaluatePredictorSpecs()
{
	boolean bOk = true;
	ObjectArray oaEvaluatedTrainedPredictors;
	ObjectArray oaOutputPredictorEvaluations;
	ALString sOutputPathName;

	// Recherche des predicteurs a evaluer
	BuildEvaluatedTrainedPredictors(&oaEvaluatedTrainedPredictors);

	// Test de coherence des predicteurs
	bOk = CheckEvaluatedTrainedPredictors(&oaEvaluatedTrainedPredictors);

	// On tente de cree le repertoire cible du rapport d'evaluation
	// (c'est le meme pour le rapport JSON)
	if (bOk)
	{
		sOutputPathName = FileService::GetPathName(GetEvaluationFilePathName());
		if (sOutputPathName != "" and not PLRemoteFileService::DirExists(sOutputPathName))
		{
			bOk = PLRemoteFileService::MakeDirectories(sOutputPathName);
			if (not bOk)
				AddError("Unable to create output directory (" + sOutputPathName +
					 ") for evaluation file");
		}
	}

	// Evaluation des predicteurs s'ils sont coherents
	if (bOk)
	{
		// Demarage du suivi de la tache
		TaskProgression::SetTitle("Evaluate model");
		TaskProgression::SetDisplayedLevelNumber(2);
		TaskProgression::Start();

		// Evaluation des predicteurs
		EvaluateTrainedPredictors(&oaEvaluatedTrainedPredictors, &oaOutputPredictorEvaluations);
		oaOutputPredictorEvaluations.DeleteAll();

		// Fin du suivi de la tache
		TaskProgression::Stop();
	}

	// Destruction des predicteurs (qui n'ont ici ete utiles que pour identifier leurs specifications)
	oaEvaluatedTrainedPredictors.DeleteAll();
}

boolean KWPredictorEvaluator::EvaluatePredictors(ObjectArray* oaPredictors, KWDatabase* database,
						 const ALString& sEvaluationLabel,
						 ObjectArray* oaOutputPredictorEvaluations)
{
	boolean bOk = true;
	ALString sLowerEvaluationLabel;
	KWPredictor* predictor;
	KWPredictorEvaluation* predictorEvaluation;
	int i;
	boolean bComputeEvaluation;
	ALString sTmp;

	require(oaPredictors != NULL);
	require(oaPredictors->GetSize() > 0);
	require(database != NULL);
	require(database->GetDatabaseName() != "");
	require(oaOutputPredictorEvaluations != NULL);
	require(oaOutputPredictorEvaluations->GetSize() == 0);

	// Libelle d'evaluation en minuscule
	sLowerEvaluationLabel = sEvaluationLabel;
	sLowerEvaluationLabel.MakeLower();

	// Flag d'ecriture du rapport d'evaluation
	bComputeEvaluation = true;
	if (bComputeEvaluation and not database->Check())
	{
		bComputeEvaluation = false;
		AddError("No predictor evaluation because of " + sEvaluationLabel + " database errors");
	}

	// Ecriture du rapport d'evaluation
	bOk = bComputeEvaluation and not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		// Suivi de tache
		TaskProgression::BeginTask();
		TaskProgression::SetTitle(sEvaluationLabel + " evaluation");
		TaskProgression::DisplayMainLabel(sEvaluationLabel + " evaluation on database " +
						  database->GetDatabaseName());

		// Evaluation des predicteurs
		for (i = 0; i < oaPredictors->GetSize(); i++)
		{
			predictor = cast(KWPredictor*, oaPredictors->GetAt(i));

			// Evaluation
			predictorEvaluation = NULL;
			if (predictor->IsTrained())
			{
				predictorEvaluation = EvaluatePredictor(predictor, database, sEvaluationLabel);
				if (not TaskProgression::IsInterruptionRequested() and predictorEvaluation != NULL)
					oaOutputPredictorEvaluations->Add(predictorEvaluation);
				else
				{
					bOk = false;
					if (predictorEvaluation != NULL)
						delete predictorEvaluation;
				}
			}

			// Suivi de progression
			TaskProgression::DisplayProgression(((i + 1) * 100) / oaPredictors->GetSize());
			bOk = bOk and not TaskProgression::IsInterruptionRequested();
			if (not bOk)
				break;
		}
		assert(oaPredictors->GetSize() >= oaOutputPredictorEvaluations->GetSize());

		// Suivi de tache, avec libelle de fin pertinent en cas de fichier de suivi des taches (option -t de la
		// ligne de commande)
		TaskProgression::DisplayLabel(
		    sTmp + "Evaluated predictors: " + IntToString(oaOutputPredictorEvaluations->GetSize()));
		TaskProgression::EndTask();
	}
	return bOk;
}

void KWPredictorEvaluator::WriteEvaluationReport(const ALString& sEvaluationReportName,
						 const ALString& sEvaluationLabel, ObjectArray* oaPredictorEvaluations)
{
	ALString sLowerEvaluationLabel;
	KWPredictorEvaluation* predictorEvaluation;

	require(oaPredictorEvaluations != NULL);
	require(sEvaluationReportName != "");

	// Ajout de log memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sEvaluationReportName + " Write report Begin");

	// Libelle d'evaluation en minuscule
	sLowerEvaluationLabel = sEvaluationLabel;
	sLowerEvaluationLabel.MakeLower();

	// DEPRECATED
	// Pour etre compatible avec les fichiers de rapport existant, qui sont en read-only
	if (FileService::GetURIScheme(sEvaluationReportName) == "")
	{
		if (FileService::FileExists(sEvaluationReportName))
			FileService::SetFileMode(sEvaluationReportName, false);
	}

	// Destruction du rapport d'evaluation existant
	PLRemoteFileService::RemoveFile(sEvaluationReportName);

	// Ecriture du rapport d'evaluation
	if (oaPredictorEvaluations->GetSize() == 0)
		Global::AddWarning(
		    "", "", sEvaluationLabel + " evaluation report is not written since no predictor was evaluated");
	else
	{
		predictorEvaluation = cast(KWPredictorEvaluation*, oaPredictorEvaluations->GetAt(0));
		predictorEvaluation->WriteFullReportFile(sEvaluationReportName, sEvaluationLabel,
							 oaPredictorEvaluations);
	}

	// Ajout de log memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sEvaluationReportName + " Write report End");
}

void KWPredictorEvaluator::WriteJSONReport(const ALString& sJSONReportName, const ALString& sEvaluationLabel,
					   ObjectArray* oaPredictorEvaluations)
{
	ALString sLowerEvaluationLabel;
	KWPredictorEvaluation* predictorEvaluation;
	JSONFile fJSON;

	require(oaPredictorEvaluations != NULL);
	require(sJSONReportName != "");

	// Libelle d'evaluation en minuscule
	sLowerEvaluationLabel = sEvaluationLabel;
	sLowerEvaluationLabel.MakeLower();

	// Destruction du rapport JSON existant
	PLRemoteFileService::RemoveFile(sJSONReportName);

	// Ecriture du rapport d'evaluation
	if (oaPredictorEvaluations->GetSize() == 0)
		Global::AddWarning("", "",
				   sEvaluationLabel + " JSON report is not written since no predictor was evaluated");
	else
	{
		predictorEvaluation = cast(KWPredictorEvaluation*, oaPredictorEvaluations->GetAt(0));
		predictorEvaluation->WriteFullReportFile(sJSONReportName, sEvaluationLabel, oaPredictorEvaluations);

		// Message synthetique
		AddSimpleMessage("Write " + sLowerEvaluationLabel + " evaluation report " + sJSONReportName);

		// Ouverture du fichier JSON
		fJSON.SetFileName(sJSONReportName);
		fJSON.OpenForWrite();

		// Ecriture de son contenu
		if (fJSON.IsOpened())
		{
			// Outil et version
			fJSON.WriteKeyString("tool", GetLearningApplicationName());
			if (GetLearningModuleName() != "")
				fJSON.WriteKeyString("sub_tool", GetLearningModuleName());
			fJSON.WriteKeyString("version", GetLearningVersion());

			// Liste des messages d'erreur potentiellement detectees pendant l'analyse
			KWLearningErrorManager::WriteJSONKeyReport(&fJSON);

			// Rapport d'evaluation
			fJSON.BeginKeyObject("evaluationReport");
			cast(KWPredictorEvaluation*, oaPredictorEvaluations->GetAt(0))
			    ->WriteJSONFullReportFields(&fJSON, "", oaPredictorEvaluations);
			fJSON.EndObject();

			// Fermeture du fichier
			fJSON.Close();
		}
	}
}

KWPredictorEvaluation* KWPredictorEvaluator::EvaluatePredictor(KWPredictor* predictor, KWDatabase* database,
							       const ALString& sEvaluationLabel)
{
	KWPredictorEvaluation* predictorEvaluation = NULL;

	require(predictor != NULL);
	require(database != NULL);
	require(database->Check());

	// Evaluation
	if (predictor->IsTrained() and not TaskProgression::IsInterruptionRequested())
	{
		// Libelle de tache
		TaskProgression::DisplayLabel(predictor->GetName());

		// Message
		AddSimpleMessage(sEvaluationLabel + " evaluation of " + predictor->GetObjectLabel() + " on database " +
				 database->GetDatabaseName());

		// Evaluation
		predictorEvaluation = predictor->Evaluate(database);

		// Memorisation de l'evaluation si non interrompue ou incorrecte
		if (TaskProgression::IsInterruptionRequested() or not predictorEvaluation->IsStatsComputed())
		{
			delete predictorEvaluation;
			predictorEvaluation = NULL;
		}
	}
	return predictorEvaluation;
}

const ALString KWPredictorEvaluator::GetClassLabel() const
{
	return "Evaluate model";
}

void KWPredictorEvaluator::BuildEvaluatedTrainedPredictors(ObjectArray* oaEvaluatedTrainedPredictors)
{
	int i;
	KWClass* kwcPredictorClass;
	KWTrainedPredictor* trainedPredictor;
	boolean bIsPredictor;
	KWEvaluatedPredictorSpec* evaluatedPredictorSpec;

	require(oaEvaluatedTrainedPredictors != NULL);
	require(oaEvaluatedPredictorSpecs.GetSize() == 0 or GetInitialClassesDomain() != NULL);
	require(kwcdInitialCurrentDomain != NULL);

	// Nettoyage initial du tableau resultat
	oaEvaluatedTrainedPredictors->RemoveAll();

	// Parcours des specifications de predicteurs a evaluer
	for (i = 0; i < oaEvaluatedPredictorSpecs.GetSize(); i++)
	{
		evaluatedPredictorSpec = cast(KWEvaluatedPredictorSpec*, oaEvaluatedPredictorSpecs.GetAt(i));

		// Construction du predicteur si evaluation demandee
		if (evaluatedPredictorSpec->GetEvaluated())
		{
			// Recherche de la classe correspondante
			kwcPredictorClass =
			    kwcdInitialCurrentDomain->LookupClass(evaluatedPredictorSpec->GetClassName());
			assert(kwcPredictorClass != NULL);

			// Recherche du predicteur correspondant
			trainedPredictor = NULL;
			if (evaluatedPredictorSpec->GetPredictorType() == KWType::GetPredictorLabel(KWType::Symbol))
			{
				trainedPredictor = new KWTrainedClassifier;
				bIsPredictor = trainedPredictor->ImportPredictorClass(kwcPredictorClass);
				assert(bIsPredictor);
				oaEvaluatedTrainedPredictors->Add(trainedPredictor);
			}
			else if (evaluatedPredictorSpec->GetPredictorType() ==
				 KWType::GetPredictorLabel(KWType::Continuous))
			{
				trainedPredictor = new KWTrainedRegressor;
				bIsPredictor = trainedPredictor->ImportPredictorClass(kwcPredictorClass);
				assert(bIsPredictor);
				oaEvaluatedTrainedPredictors->Add(trainedPredictor);
			}
			check(trainedPredictor);
		}
	}
}

boolean KWPredictorEvaluator::CheckEvaluatedTrainedPredictors(ObjectArray* oaEvaluatedTrainedPredictors)
{
	int i;
	KWTrainedPredictor* trainedPredictor;
	KWTrainedPredictor* referencePredictor;
	boolean bArePredictorsConsistent;

	require(oaEvaluatedTrainedPredictors != NULL);
	require(evaluationDatabase != NULL);

	// Controle de coherence des predicteurs a evaluer: meme type prediction et meme attribut cible
	bArePredictorsConsistent = true;
	if (oaEvaluatedTrainedPredictors->GetSize() > 1)
	{
		// Recherche  des informations de reference sur le premier predicteur
		referencePredictor = cast(KWTrainedPredictor*, oaEvaluatedTrainedPredictors->GetAt(0));

		// Comparaison des informations avec les autres predicteurs
		for (i = 1; i < oaEvaluatedTrainedPredictors->GetSize(); i++)
		{
			trainedPredictor = cast(KWTrainedPredictor*, oaEvaluatedTrainedPredictors->GetAt(i));

			// Comparaison
			if (not referencePredictor->IsConsistentWith(trainedPredictor))
			{
				bArePredictorsConsistent = false;
				referencePredictor->AddError(
				    "Cannot be evaluated together with an inconsistent predictor (" +
				    trainedPredictor->GetClassLabel() + " " + trainedPredictor->GetObjectLabel() + ")");
				break;
			}
		}
	}

	// Controle d'existence de l'eventuelle variable de selection pour tous els predicteurs a evaluer
	if (bArePredictorsConsistent and evaluationDatabase->GetSelectionAttribute() != "")
	{
		// Comparaison des informations avec les autres predicteurs
		for (i = 0; i < oaEvaluatedTrainedPredictors->GetSize(); i++)
		{
			trainedPredictor = cast(KWTrainedPredictor*, oaEvaluatedTrainedPredictors->GetAt(i));

			// Test d'existence de l'attribut de selection dans le predicteur
			if (trainedPredictor->GetPredictorClass()->LookupAttribute(
				evaluationDatabase->GetSelectionAttribute()) == NULL)
			{
				bArePredictorsConsistent = false;
				trainedPredictor->AddError("Cannot be evaluated with selection variable " +
							   evaluationDatabase->GetSelectionAttribute() + " (" +
							   "missing in predictor dictionary " +
							   trainedPredictor->GetPredictorClass()->GetName() + ")");
				break;
			}
		}
	}
	return bArePredictorsConsistent;
}

void KWPredictorEvaluator::EvaluateTrainedPredictors(ObjectArray* oaEvaluatedTrainedPredictors,
						     ObjectArray* oaOutputPredictorEvaluations)
{
	boolean bOk = true;
	KWLearningSpec learningSpec;
	KWClass* learningSpecClass;
	KWTrainedPredictor* trainedPredictor;
	int i;
	ObjectArray oaPredictors;
	KWPredictorExternal* predictorExternal;
	KWClassDomain* kwcdCurrentDomain;
	ObjectArray oaEvaluationDatabaseFileSpecs;
	int nRef;
	FileSpec* specRef;
	FileSpec specEvaluationReportFile;
	FileSpec specJSONReportFile;

	require(oaEvaluatedTrainedPredictors != NULL);
	require(oaEvaluatedPredictorSpecs.GetSize() == 0 or GetInitialClassesDomain() != NULL);
	require(kwcdInitialCurrentDomain != NULL);
	require(oaOutputPredictorEvaluations != NULL);
	require(oaOutputPredictorEvaluations->GetSize() == 0);

	// On positionne le domaine des classes initiales comme domaine courant
	// Cela permet ainsi le parametrage de la base d'evaluation par les
	// classes initiales des predicteurs
	kwcdCurrentDomain = KWClassDomain::GetCurrentDomain();
	if (kwcdInitialClassesDomain != NULL)
		KWClassDomain::SetCurrentDomain(kwcdInitialClassesDomain);

	// Verification de la coherence des predicteurs
	if (bOk)
		bOk = CheckEvaluatedTrainedPredictors(oaEvaluatedTrainedPredictors);

	// Le nom du rapport d'evaluation doit etre renseigne
	if (bOk and GetEvaluationFileName() == "")
	{
		bOk = false;
		AddError("Missing evaluation report name");
	}

	// Le nom de la base d'evaluation doit etre renseigne
	if (bOk and evaluationDatabase->GetDatabaseName() == "")
	{
		bOk = false;
		AddError("Missing evaluation database name");
	}

	// Verification de la validite des specifications de la base d'evaluation
	bOk = bOk and evaluationDatabase->Check();

	// Le parametrage de selection doit etre valide
	// Les messages d'erreurs sont emis par la methode appelee
	if (bOk and not evaluationDatabase->CheckSelectionValue(evaluationDatabase->GetSelectionValue()))
		bOk = false;

	// Le nom du rapport d'evaluation doit etre different du ou des fichiers de la base source
	if (bOk)
	{
		specEvaluationReportFile.SetLabel("evaluation report");
		specEvaluationReportFile.SetFilePathName(GetEvaluationFilePathName());
		evaluationDatabase->ExportUsedFileSpecs(&oaEvaluationDatabaseFileSpecs);
		for (nRef = 0; nRef < oaEvaluationDatabaseFileSpecs.GetSize(); nRef++)
		{
			specRef = cast(FileSpec*, oaEvaluationDatabaseFileSpecs.GetAt(nRef));
			specRef->SetLabel("evaluation " + specRef->GetLabel());
			bOk = bOk and specEvaluationReportFile.CheckReferenceFileSpec(specRef);
			if (not bOk)
				break;
		}
		oaEvaluationDatabaseFileSpecs.DeleteAll();
		if (not bOk)
			AddError("The evaluation report file name should differ from that of the evaluation database");

		// Le nom du rapport JSON doit etre different du ou des fichiers de la base source
		if (bOk and GetJSONFilePathName() != "")
		{
			specJSONReportFile.SetLabel("JSON report");
			specJSONReportFile.SetFilePathName(GetJSONFilePathName());
			for (nRef = 0; nRef < oaEvaluationDatabaseFileSpecs.GetSize(); nRef++)
			{
				specRef = cast(FileSpec*, oaEvaluationDatabaseFileSpecs.GetAt(nRef));
				specRef->SetLabel("evaluation " + specRef->GetLabel());
				bOk = bOk and specJSONReportFile.CheckReferenceFileSpec(specRef);
				if (not bOk)
					break;
			}
			if (not bOk)
				AddError(
				    "The JSON report file name should differ from that of the evaluation database");

			// Et il doit etre different du rapport d'evaluation
			if (bOk)
				bOk = specJSONReportFile.CheckReferenceFileSpec(&specEvaluationReportFile);
		}
	}

	// Il doit y avoir au moins un predicteur a evaluer
	if (bOk and oaEvaluatedTrainedPredictors->GetSize() == 0)
	{
		bOk = false;
		AddWarning("No requested predictor evaluation");
	}

	// Evaluation des predicteurs
	if (bOk)
	{
		// Debut de la gestion des erreurs dediees a l'apprentissage
		KWLearningErrorManager::BeginErrorCollection();
		KWLearningErrorManager::AddTask("Evaluation");

		// Destruction du domaine initial (a reconstruire), en faisant attention au domaine courant
		check(kwcdInitialClassesDomain);
		if (kwcdCurrentDomain == kwcdInitialClassesDomain)
			kwcdCurrentDomain = NULL;
		delete kwcdInitialClassesDomain;

		// On reconstruit le domaine initial a partir du premier predicteur a evaluer
		// En effet, ce domaine initial de reference peut dependre de la selection en cours des predicteurs a
		// evaluer
		trainedPredictor = cast(KWTrainedPredictor*, oaEvaluatedTrainedPredictors->GetAt(0));
		kwcdInitialClassesDomain = BuildInitialDomainPredictor(trainedPredictor);
		KWClassDomain::SetCurrentDomain(kwcdInitialClassesDomain);
		assert(DomainCheckClassesInitialNames(trainedPredictor->GetPredictorDomain()));

		// Si le domaine courant correspondait avec le domaine initial, on maintient cette correspondance
		if (kwcdCurrentDomain == NULL)
			kwcdCurrentDomain = kwcdInitialClassesDomain;

		// Parametrage des specifications d'apprentissage a partir du premier predicteur a evaluer
		trainedPredictor = cast(KWTrainedPredictor*, oaEvaluatedTrainedPredictors->GetAt(0));
		learningSpec.SetDatabase(GetEvaluationDatabase());
		learningSpec.SetTargetAttributeName(trainedPredictor->GetTargetAttribute()->GetName());
		learningSpec.SetMainTargetModality((const char*)GetMainTargetModality());
		learningSpecClass = kwcdInitialClassesDomain->LookupClass(GetEvaluationDatabase()->GetClassName());
		learningSpec.SetClass(learningSpecClass);
		assert(learningSpecClass != NULL);
		assert(learningSpecClass->GetName() ==
		       KWTrainedPredictor::GetMetaDataInitialClassName(trainedPredictor->GetPredictorClass()));
		assert(learningSpec.Check());

		// Reconstruction de predicteurs
		for (i = 0; i < oaEvaluatedTrainedPredictors->GetSize(); i++)
		{
			trainedPredictor = cast(KWTrainedPredictor*, oaEvaluatedTrainedPredictors->GetAt(i));

			// On restitue les noms initiaux des classes du predicteurs afin de pouvoiur utiliser la  base
			// d'evaluation, qui est parametree par ces classes initiales valide pour tous les predicteurs
			DomainRenameClassesWithInitialNames(trainedPredictor->GetPredictorDomain());

			// Construction du predicteur
			predictorExternal = new KWPredictorExternal;
			predictorExternal->SetLearningSpec(&learningSpec);
			predictorExternal->SetExternalTrainedPredictor(trainedPredictor);
			evaluationDatabase->SetVerboseMode(false);
			predictorExternal->Train();
			evaluationDatabase->SetVerboseMode(true);
			assert(predictorExternal->IsTrained());

			// Memorisation
			oaPredictors.Add(predictorExternal);
		}
		// Evaluation des predicteurs
		EvaluatePredictors(&oaPredictors, GetEvaluationDatabase(), "Predictor", oaOutputPredictorEvaluations);

		// Ecriture du rapport d'evaluation
		WriteEvaluationReport(GetEvaluationFilePathName(), "Predictor", oaOutputPredictorEvaluations);

		// Ecriture du rapport JSON
		if (GetJSONFilePathName() != "")
			WriteJSONReport(GetJSONFilePathName(), "Predictor", oaOutputPredictorEvaluations);

		// Nettoyage du tableau de predicteurs, en dereferencant prealablement
		// leur predicteur appris (pour eviter une double destruction)
		for (i = 0; i < oaPredictors.GetSize(); i++)
		{
			predictorExternal = cast(KWPredictorExternal*, oaPredictors.GetAt(i));
			predictorExternal->UnreferenceTrainedPredictor();
		}
		oaPredictors.DeleteAll();

		// Fin de la gestion des erreurs dediees a l'apprentissage
		KWLearningErrorManager::EndErrorCollection();
	}

	// Restitution du domaine courant
	if (kwcdInitialClassesDomain != NULL)
		KWClassDomain::SetCurrentDomain(kwcdCurrentDomain);
}

void KWPredictorEvaluator::DomainRenameClassesWithInitialNames(KWClassDomain* kwcdDomain)
{
	ObjectArray oaClasses;
	int nClass;
	KWClass* kwcClass;
	ALString sInitialClassName;

	require(kwcdDomain != NULL);

	// Export prealable des classe du domaine dans un tableau
	// (le renommage d'une classe provoque la reindexation du domaine, ce qui empeche
	// le parcours indexe des classes depuis le domaine)
	kwcdDomain->ExportClassArray(&oaClasses);

	// Renommage des classes avec les noms des classes initiales
	for (nClass = 0; nClass < oaClasses.GetSize(); nClass++)
	{
		kwcClass = cast(KWClass*, oaClasses.GetAt(nClass));

		// Acces au nom initial de la classe
		sInitialClassName = KWTrainedPredictor::GetMetaDataInitialClassName(kwcClass);
		assert(sInitialClassName != "");
		assert(kwcdDomain->LookupClass(sInitialClassName) == NULL);

		// Renommage
		kwcdDomain->RenameClass(kwcClass, sInitialClassName);
	}
}

boolean KWPredictorEvaluator::DomainCheckClassesInitialNames(KWClassDomain* kwcdDomain)
{
	boolean bOk = true;
	int nClass;
	KWClass* kwcClass;
	KWClass* kwcInitialClass;
	ALString sInitialClassName;

	require(kwcdDomain != NULL);
	require(kwcdInitialClassesDomain != NULL);

	// Verification de la compatibilite en nombre de classes
	if (kwcdDomain->GetClassNumber() != kwcdInitialClassesDomain->GetClassNumber())
		bOk = false;

	// Verification des noms des classes initiales par rapport au domaine de reference
	if (bOk)
	{
		for (nClass = 0; nClass < kwcdDomain->GetClassNumber(); nClass++)
		{
			kwcClass = kwcdDomain->GetClassAt(nClass);

			// Acces au nom initial de la classe
			sInitialClassName = KWTrainedPredictor::GetMetaDataInitialClassName(kwcClass);
			assert(sInitialClassName != "");

			// Verification de l'existence de la classe initiale et de sa composition
			// en attribut natifs
			kwcInitialClass = kwcdInitialClassesDomain->LookupClass(sInitialClassName);
			if (kwcInitialClass == NULL or not ClassCheckNativeAttributes(kwcClass, kwcInitialClass) or
			    not ClassCheckNativeAttributes(kwcInitialClass, kwcClass))
			{
				bOk = false;
				break;
			}
		}
	}

	return bOk;
}

boolean KWPredictorEvaluator::ClassCheckNativeAttributes(KWClass* sourceClass, KWClass* targetClass)
{
	boolean bOk = true;
	KWAttribute* sourceAttribute;
	KWAttribute* targetAttribute;

	require(sourceClass != NULL);
	require(targetClass != NULL);

	// Parcours des attributs natifs de la classe sosurce
	sourceAttribute = sourceClass->GetHeadAttribute();
	while (sourceAttribute != NULL)
	{
		// Test si attribut source natif
		if (KWType::IsData(sourceAttribute->GetType()) and sourceAttribute->GetAnyDerivationRule() == NULL)
		{
			// Recherche de l'attribut cible correspondant
			targetAttribute = targetClass->LookupAttribute(sourceAttribute->GetName());

			// Erreur si attribut natif correspondant non trouve
			if (targetAttribute == NULL or targetAttribute->GetType() != sourceAttribute->GetType() or
			    targetAttribute->GetAnyDerivationRule() != NULL)
			{
				bOk = false;
				break;
			}
		}

		// Attribut suivant
		sourceClass->GetNextAttribute(sourceAttribute);
	}
	return bOk;
}

void KWPredictorEvaluator::RenameDatabaseClasses(KWDatabase* database, KWClassDomain* kwcdInitialDomain)
{
	KWMTDatabase* mtDatabase;
	KWMTDatabaseMapping* mapping;
	KWClass* kwcClass;
	ALString sInitialRootClassName;
	ALString sInitialClassName;
	int i;

	require(database != NULL);

	// Cas ou il n'y a pas de classe initiale
	if (kwcdInitialDomain == NULL)
		database->SetClassName("");
	// Cas avec initialisation
	else
	{
		// Recherche de la la classe initiale correspondant a la classe en cours de la base
		sInitialRootClassName = "";
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(database->GetClassName());
		if (kwcClass != NULL)
			sInitialRootClassName = KWTrainedPredictor::GetMetaDataInitialClassName(kwcClass);

		// Memorisation de cette classe
		database->SetClassName(sInitialRootClassName);

		// Cas multi-table
		if (database->IsMultiTableTechnology())
		{
			// Parametrage des mapping (sauf la table principale, deja parametree
			mtDatabase = cast(KWMTDatabase*, database);
			for (i = 1; i < mtDatabase->GetTableNumber(); i++)
			{
				mapping = cast(KWMTDatabaseMapping*, mtDatabase->GetMultiTableMappings()->GetAt(i));

				// Recherche de la la classe initiale correspondant a la classe en cours
				sInitialClassName = "";
				kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(mapping->GetClassName());
				if (kwcClass != NULL)
					sInitialClassName = KWTrainedPredictor::GetMetaDataInitialClassName(kwcClass);

				// Recherche de la la classe initiale correspondant a la classe en cours
				sInitialRootClassName = "";
				kwcClass =
				    KWClassDomain::GetCurrentDomain()->LookupClass(mapping->GetDataPathClassName());
				if (kwcClass != NULL)
					sInitialRootClassName =
					    KWTrainedPredictor::GetMetaDataInitialClassName(kwcClass);

				// Memorisation de cette classe ainsi de que la classe principale de la base
				mapping->SetClassName(sInitialClassName);
				mapping->SetDataPathClassName(sInitialRootClassName);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorExternal

KWPredictorExternal::KWPredictorExternal()
{
	externalTrainedPredictor = NULL;
}

KWPredictorExternal::~KWPredictorExternal()
{
	if (externalTrainedPredictor != NULL)
		delete externalTrainedPredictor;
}

boolean KWPredictorExternal::IsTargetTypeManaged(int nType) const
{
	return KWType::IsSimple(nType);
}

KWPredictor* KWPredictorExternal::Create() const
{
	return new KWPredictorExternal;
}

const ALString KWPredictorExternal::GetName() const
{
	if (trainedPredictor != NULL)
		return trainedPredictor->GetName();
	else
		return "External predictor";
}

const ALString KWPredictorExternal::GetPrefix() const
{
	return "";
}

void KWPredictorExternal::SetExternalTrainedPredictor(KWTrainedPredictor* inputTrainedPredictor)
{
	if (externalTrainedPredictor != NULL)
		delete externalTrainedPredictor;
	externalTrainedPredictor = inputTrainedPredictor;
}

KWTrainedPredictor* KWPredictorExternal::GetExternalTrainedPredictor()
{
	return externalTrainedPredictor;
}

void KWPredictorExternal::UnreferenceTrainedPredictor()
{
	trainedPredictor = NULL;
}

boolean KWPredictorExternal::InternalTrain()
{
	assert(trainedPredictor != NULL);
	delete trainedPredictor;
	trainedPredictor = externalTrainedPredictor;
	externalTrainedPredictor = NULL;
	return trainedPredictor->Check();
}
