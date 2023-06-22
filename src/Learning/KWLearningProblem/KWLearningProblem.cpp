// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningProblem.h"

KWLearningProblem::KWLearningProblem()
{
	// Creation explicite des sous-objets, ce qui permet de creer des sous-objets specifiques dans des sous-classes
	classManagement = new KWClassManagement;
	trainDatabase = KWDatabase::CreateDefaultDatabaseTechnology();
	testDatabase = KWDatabase::CreateDefaultDatabaseTechnology();
	analysisSpec = new KWAnalysisSpec;
	analysisResults = new KWAnalysisResults;
	predictorEvaluator = new KWPredictorEvaluator;

	// Valeurs par defaut
	trainDatabase->SetSampleNumberPercentage(70);
	analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxConstructedAttributeNumber(100);
	analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxTextFeatureNumber(10000);
	analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxTreeNumber(10);
	if (not GetLearningTextVariableMode())
		analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxTextFeatureNumber(0);
}

KWLearningProblem::~KWLearningProblem()
{
	delete classManagement;
	delete trainDatabase;
	delete testDatabase;
	delete analysisSpec;
	delete analysisResults;
	delete predictorEvaluator;
}

/////////////////////////////////////////////////////////////////////////////

KWClassManagement* KWLearningProblem::GetClassManagement()
{
	return classManagement;
}

KWDatabase* KWLearningProblem::GetTrainDatabase()
{
	// Synchronisation du dictionnaire de la base
	trainDatabase->SetClassName(classManagement->GetClassName());
	return trainDatabase;
}

KWDatabase* KWLearningProblem::GetTestDatabase()
{
	// Synchronisation du dictionnaire de la base
	testDatabase->SetClassName(classManagement->GetClassName());
	return testDatabase;
}

KWAnalysisSpec* KWLearningProblem::GetAnalysisSpec()
{
	return analysisSpec;
}

KWAnalysisResults* KWLearningProblem::GetAnalysisResults()
{
	return analysisResults;
}

/////////////////////////////////////////////////////////////////////////////

ALString KWLearningProblem::GetClassFileName() const
{
	return classManagement->GetClassFileName();
}

ALString KWLearningProblem::GetClassName() const
{
	return classManagement->GetClassName();
}

int KWLearningProblem::GetAttributeNumber() const
{
	KWClass* kwcClass;

	// Recherche de la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

	// Si trouve: on retourne son nombre de champs
	if (kwcClass != NULL)
		return kwcClass->GetUsedAttributeNumber();
	else
		return 0;
}

ALString KWLearningProblem::GetDatabaseName() const
{
	return trainDatabase->GetDatabaseName();
}

ALString KWLearningProblem::GetTargetAttributeName() const
{
	return analysisSpec->GetTargetAttributeName();
}

ALString KWLearningProblem::GetMainTargetModality() const
{
	return analysisSpec->GetMainTargetModality();
}

KWPreprocessingSpec* KWLearningProblem::GetPreprocessingSpec()
{
	return analysisSpec->GetPreprocessingSpec();
}

KWPredictorEvaluator* KWLearningProblem::GetPredictorEvaluator()
{
	return predictorEvaluator;
}

/////////////////////////////////////////////////////////////////////////////

void KWLearningProblem::CheckData()
{
	KWDatabaseCheckTask databaseCheckTask;

	require(CheckClass());
	require(CheckTrainDatabaseName());
	require(GetTrainDatabase()->CheckSelectionValue(GetTrainDatabase()->GetSelectionValue()));

	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Check database " + GetTrainDatabase()->GetDatabaseName());
	TaskProgression::Start();

	// Verification de la base d'apprentissage
	AddSimpleMessage("Check database " + GetTrainDatabase()->GetDatabaseName());
	databaseCheckTask.CheckDatabase(GetTrainDatabase());

	// Fin du suivi de la tache
	TaskProgression::Stop();
}

boolean KWLearningProblem::CheckClass() const
{
	KWClass* kwcClass;

	// Erreur si classe non specifiee
	kwcClass = NULL;
	if (GetClassName() == "")
		Global::AddError("Database management", trainDatabase->GetDatabaseName(), "Missing dictionary name");
	// Sinon, recherche de la classe
	else
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

		// Si non trouve, erreur
		if (kwcClass == NULL)
		{
			Global::AddError("Database management", trainDatabase->GetDatabaseName(),
					 "Dictionary " + GetClassName() + " must be loaded");
		}
	}

	// Erreur si classe incorrecte
	if (kwcClass != NULL)
	{
		// Verification de la classe
		if (not kwcClass->Check())
			kwcClass = NULL;
		// Verification de l'absence de cycle
		else if (not kwcClass->IsCompiled())
		{
			// Redeclenchement de la compilation du domaine, seule apte a detecter les
			// cycles, pour provoquer l'emission de messages d'erreur
			kwcClass->GetDomain()->Compile();
			if (not kwcClass->IsCompiled())
				kwcClass = NULL;
		}
	}

	return kwcClass != NULL;
}

boolean KWLearningProblem::CheckTargetAttribute() const
{
	boolean bOk = true;
	KWClass* kwcClass;
	KWAttribute* attribute;

	require(CheckClass());

	if (GetTargetAttributeName() != "")
	{
		// Recherche de la classe
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
		check(kwcClass);

		// Recherche de l'attribut cible
		attribute = kwcClass->LookupAttribute(GetTargetAttributeName());
		if (attribute == NULL)
		{
			bOk = false;
			Global::AddError("", "",
					 "Target variable " + GetTargetAttributeName() + " unknown in dictionary " +
					     GetClassName());
		}
		else if (not KWType::IsSimple(attribute->GetType()))
		{
			bOk = false;
			Global::AddError("", "", "Incorrect type for target variable " + GetTargetAttributeName());
		}
		else if (not attribute->GetUsed())
		{
			bOk = false;
			Global::AddError("", "",
					 "Target variable " + GetTargetAttributeName() + " unused in dictionary " +
					     GetClassName());
		}
	}
	return bOk;
}

#ifdef DEPRECATED_V10
boolean KWLearningProblem::CheckMandatoryAttributeInPairs() const
{
	boolean bOk = true;
	ALString sMandatoryAttributeInPairs;
	KWClass* kwcClass;
	KWAttribute* attribute;

	require(CheckClass());

	// Acces au nom de l'attribut obligatoire dans les paires
	sMandatoryAttributeInPairs = analysisSpec->GetModelingSpec()
					 ->GetAttributeConstructionSpec()
					 ->GetAttributePairsSpec()
					 ->GetMandatoryAttributeInPairs();

	// Verification
	if (sMandatoryAttributeInPairs != "")
	{
		// Recherche de la classe
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
		check(kwcClass);

		// Recherche de l'attribut cible
		attribute = kwcClass->LookupAttribute(sMandatoryAttributeInPairs);
		if (attribute == NULL)
		{
			bOk = false;
			Global::AddError("", "",
					 "Mandatory variable in pair analysis " + sMandatoryAttributeInPairs +
					     " unknown in dictionary " + GetClassName());
		}
		else if (not KWType::IsSimple(attribute->GetType()))
		{
			bOk = false;
			Global::AddError("", "",
					 "Incorrect type for mandatory variable in pair analysis " +
					     sMandatoryAttributeInPairs);
		}
		else if (not attribute->GetUsed())
		{
			bOk = false;
			Global::AddError("", "",
					 "Mandatory variable in pair analysis " + sMandatoryAttributeInPairs +
					     " unused in dictionary " + GetClassName());
		}
		else if (GetTargetAttributeName() != "" and sMandatoryAttributeInPairs == GetTargetAttributeName())
		{
			bOk = false;
			Global::AddError("", "",
					 "Mandatory variable in pair analysis " + sMandatoryAttributeInPairs +
					     " should be different from target variable");
		}
	}
	return bOk;
}
#endif // DEPRECATED_V10

boolean KWLearningProblem::CheckTrainDatabaseName() const
{
	if (GetDatabaseName() == "")
	{
		Global::AddError("", "", "Missing train database name");
		return false;
	}
	else
		return true;
}

boolean KWLearningProblem::CheckPreprocessingSpecs() const
{
	KWAnalysisSpec* analysisSpecRef;
	KWPreprocessingSpec* preprocessingSpecRef;
	ALString sTmp;
	boolean bOk = true;

	require(CheckClass());
	require(CheckTargetAttribute());

	// On caste l'objet analysisSpec pour contourner les controles lies a la methode const
	analysisSpecRef = cast(KWAnalysisSpec*, analysisSpec);
	assert(analysisSpecRef->Check());
	preprocessingSpecRef = analysisSpecRef->GetPreprocessingSpec();

	// Test selon le type de la cible
	if (GetTargetAttributeName() == "")
		bOk = preprocessingSpecRef->CheckForTargetType(KWType::None);
	else
		bOk = preprocessingSpecRef->CheckForTargetType(KWType::Symbol);
	return bOk;
}

boolean KWLearningProblem::CheckRecodingSpecs() const
{
	KWAnalysisSpec* analysisSpecRef;
	KWRecodingSpec* recodingSpecRef;
	ALString sTmp;
	boolean bOk = true;

	require(CheckClass());
	require(CheckTargetAttribute());

	// On caste l'objet analysisSpec pour contourner les controles lies a la methode const
	analysisSpecRef = cast(KWAnalysisSpec*, analysisSpec);
	assert(analysisSpecRef->Check());
	recodingSpecRef = analysisSpecRef->GetRecoderSpec()->GetRecodingSpec();
	assert(recodingSpecRef->Check());

	// Verification uniquement si recodage demande
	if (analysisSpecRef->GetRecoderSpec()->GetRecoder())
	{
		// Cas non supervise
		if (GetTargetAttributeName() == "")
		{
			// Emission d'un message dans le cas de l'utilisation de la methode "conditional info"
			if (recodingSpecRef->GetRecodeSymbolAttributes() == "conditional info")
			{
				Global::AddWarning("", "",
						   sTmp + "In unsupervised analysis, categorical recoding method "
							  "(conditional info) always outputs 0");
			}
			if (recodingSpecRef->GetRecodeContinuousAttributes() == "conditional info")
			{
				Global::AddWarning("", "",
						   sTmp + "In unsupervised analysis, numerical recoding method "
							  "(conditional info) always outputs 0");
			}
		}
	}
	return bOk;
}

boolean KWLearningProblem::CheckResultFileNames() const
{
	boolean bOk = true;
	FileSpec specInputClass;
	FileSpec specInputTrainDatabase;
	FileSpec specInputTestDatabase;
	FileSpec specOutputPreparation;
	FileSpec specOutputTextPreparation;
	FileSpec specOutputTreePreparation;
	FileSpec specOutputPreparation2D;
	FileSpec specOutputModelingDictionary;
	FileSpec specOutputModeling;
	FileSpec specOutputTrainEvaluation;
	FileSpec specOutputTestEvaluation;
	FileSpec specOutputJSON;
	ObjectArray oaTrainDatabaseFileSpecs;
	ObjectArray oaTestDatabaseFileSpecs;
	ObjectArray oaInputFileSpecs;
	ObjectArray oaOutputFileSpecs;
	int nOutput;
	int nRef;
	int n;
	FileSpec* specCurrent;
	FileSpec* specOutput;
	FileSpec* specRef;
	ALString sOutputPathName;

	// Initialisation des fichiers d'entree concernes
	specInputClass.SetLabel("input dictionary");
	specInputClass.SetFilePathName(classManagement->GetClassFileName());
	oaInputFileSpecs.Add(&specInputClass);

	// Ajout du ou des fichiers de la base d'apprentissage
	trainDatabase->ExportUsedFileSpecs(&oaTrainDatabaseFileSpecs);
	for (n = 0; n < oaTrainDatabaseFileSpecs.GetSize(); n++)
	{
		specCurrent = cast(FileSpec*, oaTrainDatabaseFileSpecs.GetAt(n));
		specCurrent->SetLabel("train " + specCurrent->GetLabel());
	}
	oaInputFileSpecs.InsertObjectArrayAt(oaInputFileSpecs.GetSize(), &oaTrainDatabaseFileSpecs);

	// Ajout du ou des fichiers de la base de test
	testDatabase->ExportUsedFileSpecs(&oaTestDatabaseFileSpecs);
	for (n = 0; n < oaTestDatabaseFileSpecs.GetSize(); n++)
	{
		specCurrent = cast(FileSpec*, oaTestDatabaseFileSpecs.GetAt(n));
		specCurrent->SetLabel("test " + specCurrent->GetLabel());
	}
	oaInputFileSpecs.InsertObjectArrayAt(oaInputFileSpecs.GetSize(), &oaTestDatabaseFileSpecs);

	// Specification des fichiers de sortie concernes
	specOutputPreparation.SetLabel("preparation report");
	specOutputPreparation.SetFilePathName(analysisResults->GetPreparationFileName());
	oaOutputFileSpecs.Add(&specOutputPreparation);
	specOutputTextPreparation.SetLabel("text preparation report");
	specOutputTextPreparation.SetFilePathName(analysisResults->GetTextPreparationFileName());
	oaOutputFileSpecs.Add(&specOutputTextPreparation);
	specOutputTreePreparation.SetLabel("tree preparation report");
	specOutputTreePreparation.SetFilePathName(analysisResults->GetTreePreparationFileName());
	oaOutputFileSpecs.Add(&specOutputTreePreparation);
	specOutputPreparation2D.SetLabel("preparation report (2D)");
	specOutputPreparation2D.SetFilePathName(analysisResults->GetPreparation2DFileName());
	oaOutputFileSpecs.Add(&specOutputPreparation2D);
	specOutputModelingDictionary.SetLabel("modeling dictionary");
	specOutputModelingDictionary.SetFilePathName(analysisResults->GetModelingDictionaryFileName());
	oaOutputFileSpecs.Add(&specOutputModelingDictionary);
	specOutputModeling.SetLabel("modeling report");
	specOutputModeling.SetFilePathName(analysisResults->GetModelingFileName());
	oaOutputFileSpecs.Add(&specOutputModeling);
	specOutputTrainEvaluation.SetLabel("train evaluation report");
	specOutputTrainEvaluation.SetFilePathName(analysisResults->GetTrainEvaluationFileName());
	oaOutputFileSpecs.Add(&specOutputTrainEvaluation);
	specOutputTestEvaluation.SetLabel("test evaluation report");
	specOutputTestEvaluation.SetFilePathName(analysisResults->GetTestEvaluationFileName());
	oaOutputFileSpecs.Add(&specOutputTestEvaluation);
	specOutputJSON.SetLabel("JSON report");
	specOutputJSON.SetFilePathName(analysisResults->GetJSONFileName());
	oaOutputFileSpecs.Add(&specOutputJSON);

	// Verification que les fichiers en sortie sont des fichiers simples, sans chemin
	for (nOutput = 0; nOutput < oaOutputFileSpecs.GetSize(); nOutput++)
	{
		specOutput = cast(FileSpec*, oaOutputFileSpecs.GetAt(nOutput));
		bOk = bOk and specOutput->CheckSimplePath();
		if (not bOk)
			break;
	}

	// Calcul des chemins complets des fichiers de sortie concernes
	if (bOk)
	{
		for (nOutput = 0; nOutput < oaOutputFileSpecs.GetSize(); nOutput++)
		{
			specOutput = cast(FileSpec*, oaOutputFileSpecs.GetAt(nOutput));
			specOutput->SetFilePathName(BuildOutputFilePathName(specOutput->GetFilePathName()));
		}
	}

	// Chaque fichier de sortie doit etre different des fichiers d'entree
	if (bOk)
	{
		for (nOutput = 0; nOutput < oaOutputFileSpecs.GetSize(); nOutput++)
		{
			specOutput = cast(FileSpec*, oaOutputFileSpecs.GetAt(nOutput));

			// Parcours de fichiers d'entree
			for (nRef = 0; nRef < oaInputFileSpecs.GetSize(); nRef++)
			{
				specRef = cast(FileSpec*, oaInputFileSpecs.GetAt(nRef));

				// Test de difference
				bOk = bOk and specOutput->CheckReferenceFileSpec(specRef);
				if (not bOk)
					break;
			}
			if (not bOk)
				break;
		}
	}

	// Les fichiers de sortie doivent etre differents entre eux
	if (bOk)
	{
		for (nOutput = 0; nOutput < oaOutputFileSpecs.GetSize(); nOutput++)
		{
			specOutput = cast(FileSpec*, oaOutputFileSpecs.GetAt(nOutput));

			// Parcours des autres fichiers de sortie
			for (nRef = nOutput + 1; nRef < oaOutputFileSpecs.GetSize(); nRef++)
			{
				specRef = cast(FileSpec*, oaOutputFileSpecs.GetAt(nRef));

				// Test de difference
				bOk = bOk and specOutput->CheckReferenceFileSpec(specRef);
				if (not bOk)
					break;
			}
			if (not bOk)
				break;
		}
	}

	// Creation du repertoire des fichiers de sortie
	if (bOk)
	{
		sOutputPathName = BuildOutputPathName();
		if (sOutputPathName != "" and not PLRemoteFileService::DirExists(sOutputPathName))
		{
			bOk = PLRemoteFileService::MakeDirectories(sOutputPathName);
			if (not bOk)
				Global::AddError("", "",
						 "Unable to create results directory (" + sOutputPathName + ")");
		}
	}

	// Nettoyage
	oaTrainDatabaseFileSpecs.DeleteAll();
	oaTestDatabaseFileSpecs.DeleteAll();
	return bOk;
}

const ALString KWLearningProblem::BuildOutputFilePathName(const ALString& sFileName) const
{
	ALString sOutputPathName;
	ALString sOutputFilePathName;

	require(sFileName == "" or not FileService::IsPathInFilePath(sFileName));

	// Calcul du repertoire effectif des resultats
	sOutputPathName = BuildOutputPathName();

	// On construit le nom complet du fichier, s'il est specifie
	if (sFileName != "")
	{
		sOutputFilePathName = FileService::BuildFilePathName(
		    sOutputPathName, analysisResults->GetResultFilesPrefix() + sFileName);
	}
	return sOutputFilePathName;
}

const ALString KWLearningProblem::BuildOutputPathName() const
{
	ALString sResultPathName;
	ALString sDatabasePathName;
	ALString sOutputPathName;

	// Si le repertoire des resultats n'est pas specifie, on utilise celui de la base d'apprentissage
	sDatabasePathName = FileService::GetPathName(GetDatabaseName());
	sResultPathName = analysisResults->GetResultFilesDirectory();
	if (sResultPathName == "")
		sOutputPathName = sDatabasePathName;
	// S'il est absolu ou si c'est une URI, on le prend tel quel
	else if (FileService::IsAbsoluteFilePathName(sResultPathName) or
		 (FileService::GetURIScheme(sResultPathName) != ""))
		sOutputPathName = sResultPathName;
	// S'il commence par "./" ou ".\", on le traite comme un chemin absolu
	else if (sResultPathName.GetLength() > 2 and sResultPathName.GetAt(0) == '.' and
		 (sResultPathName.GetAt(1) == '/' or sResultPathName.GetAt(1) == '\\'))
		sOutputPathName = sResultPathName;
	// s'il est relatif, on le concatene a celui de la base d'apprentissage
	else
		sOutputPathName = FileService::BuildFilePathName(sDatabasePathName, sResultPathName);
	return sOutputPathName;
}

const ALString KWLearningProblem::GetClassLabel() const
{
	return GetLearningModuleName();
}

/////////////////////////////////////////////////////////////////////////////

void KWLearningProblem::InitializeLearningSpec(KWLearningSpec* learningSpec, KWClass* kwcClass)
{
	require(learningSpec != NULL);
	require(kwcClass != NULL);

	learningSpec->SetShortDescription(analysisResults->GetShortDescription());
	learningSpec->SetDatabase(GetTrainDatabase());
	learningSpec->SetClass(kwcClass);
	learningSpec->SetTargetAttributeName(GetTargetAttributeName());
	learningSpec->SetMainTargetModality((const char*)GetMainTargetModality());
	learningSpec->GetPreprocessingSpec()->CopyFrom(GetPreprocessingSpec());
	learningSpec->SetInitialAttributeNumber(
	    kwcClass->ComputeInitialAttributeNumber(GetTargetAttributeName() != ""));
	ensure(learningSpec->Check());
}

boolean KWLearningProblem::BuildConstructedClass(KWLearningSpec* learningSpec, KWClass*& constructedClass,
						 ObjectDictionary* odMultiTableConstructedAttributes,
						 ObjectDictionary* odTextConstructedAttributes)
{
	boolean bOk = true;
	KWClass* kwcClass;
	KDMultiTableFeatureConstruction multiTableFeatureConstruction;
	KDTextFeatureConstruction textFeatureConstruction;
	boolean bIsTextConstructionPossible;
	KWClassDomain* constructedDomain;

	require(learningSpec != NULL);
	require(learningSpec->Check());
	require(odMultiTableConstructedAttributes != NULL);
	require(odMultiTableConstructedAttributes->GetCount() == 0);
	require(odTextConstructedAttributes != NULL);
	require(odTextConstructedAttributes->GetCount() == 0);

	// Recherche de la classe initiale
	constructedClass = NULL;
	kwcClass = learningSpec->GetClass();
	check(kwcClass);
	assert(kwcClass == KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()));

	// Initialisation de la construction de variables multi-tables
	multiTableFeatureConstruction.SetLearningSpec(learningSpec);
	multiTableFeatureConstruction.SetConstructionDomain(
	    GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetConstructionDomain());
	multiTableFeatureConstruction.SetMaxRuleNumber(
	    GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxConstructedAttributeNumber());

	// Initialisation de la construction de variables a base de textes
	textFeatureConstruction.SetLearningSpec(learningSpec);
	textFeatureConstruction.SetConstructionDomain(
	    GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetConstructionDomain());
	textFeatureConstruction.SetInterpretableMode(
	    GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetTextFeatureSpec()->IsToken());

	// Detection si des variable de type texte peuvent etre construites
	bIsTextConstructionPossible = textFeatureConstruction.ContainsTextAttributes(kwcClass);

	// Parametrage des familles de construction de variables
	// En non supervise, on ne compte pas les paires de variables comme une famille
	GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->SpecifyLearningSpecConstructionFamilies(
	    learningSpec, true, bIsTextConstructionPossible);

	// Calcul de la nouvelle classe si demande
	constructedDomain = NULL;
	if (GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxConstructedAttributeNumber() >
	    0)
	{
		// Construction de variable
		multiTableFeatureConstruction.ComputeStats();
		bOk = multiTableFeatureConstruction.IsStatsComputed();
		if (bOk)
		{
			// Acces a la classe construite si la construction est effective
			if (multiTableFeatureConstruction.IsClassConstructed())
			{
				constructedClass = multiTableFeatureConstruction.GetConstructedClass();
				constructedDomain = constructedClass->GetDomain();
				multiTableFeatureConstruction.RemoveConstructedClass();

				// Il peut y avoir des cas ou aucune variable n'a ete effectivement construite, en
				// raison par exemple de variables existantes avec meme regle de derivation Dans ce cas,
				// ou restitue les couts initiaux
				assert(constructedClass->GetAttributeNumber() >= kwcClass->GetAttributeNumber());
				if (constructedClass->GetAttributeNumber() == kwcClass->GetAttributeNumber())
				{
					GetAnalysisSpec()
					    ->GetModelingSpec()
					    ->GetAttributeConstructionSpec()
					    ->SpecifyLearningSpecConstructionFamilies(learningSpec, false,
										      bIsTextConstructionPossible);
					multiTableFeatureConstruction.ComputeInitialAttributeCosts(constructedClass);
				}
			}
			// Mise a jour des familles de construction de variable sinon
			else
				GetAnalysisSpec()
				    ->GetModelingSpec()
				    ->GetAttributeConstructionSpec()
				    ->SpecifyLearningSpecConstructionFamilies(learningSpec, false,
									      bIsTextConstructionPossible);
		}
	}

	// Duplication de la classe initiale si pas de construction demandee ou pas de classe constructible
	if (bOk and constructedDomain == NULL)
	{
		assert(constructedDomain == NULL);
		constructedDomain = kwcClass->GetDomain()->CloneFromClass(kwcClass);
		constructedDomain->Compile();
		constructedClass = constructedDomain->LookupClass(GetClassName());

		// Calcul des couts de variable de facon standard
		multiTableFeatureConstruction.ComputeInitialAttributeCosts(constructedClass);
	}

	// Collecte des attributs construits en multi-tables
	if (bOk)
	{
		assert(constructedClass != NULL);
		multiTableFeatureConstruction.CollectConstructedAttributes(learningSpec->GetClass(), constructedClass,
									   odMultiTableConstructedAttributes);
	}

	// Construction de variable en mode texte si necessaire
	if (bOk)
	{
		assert(constructedClass != NULL);
		if (GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxTextFeatureNumber() >
			0 and
		    bIsTextConstructionPossible)
			bOk = textFeatureConstruction.ConstructTextFeatures(constructedClass,
									    GetAnalysisSpec()
										->GetModelingSpec()
										->GetAttributeConstructionSpec()
										->GetMaxTextFeatureNumber(),
									    odTextConstructedAttributes);
	}

	// Modification de la classe des learning spec si ok et classe construite
	if (bOk)
	{
		assert(constructedClass != NULL);
		assert(learningSpec->GetClass()->GetUsedAttributeNumber() +
			   odMultiTableConstructedAttributes->GetCount() + odTextConstructedAttributes->GetCount() ==
		       constructedClass->GetUsedAttributeNumber());
		learningSpec->SetClass(constructedClass);
	}

	// Nettoyage si necessaire
	if (not bOk and constructedDomain != NULL)
	{
		delete constructedDomain;
		constructedClass = NULL;
	}
	ensure(learningSpec->Check());
	return bOk;
}

boolean KWLearningProblem::ImportAttributeMetaDataCosts(KWLearningSpec* learningSpec, KWClass*& constructedClass)
{
	boolean bOk = true;
	KWClass* kwcClass;
	KDFeatureConstruction featureConstruction;
	KWClassDomain* constructedDomain;

	require(learningSpec != NULL);
	require(learningSpec->Check());

	// Initialisations
	constructedClass = NULL;
	constructedDomain = NULL;

	// Recherche de la classe initiale
	kwcClass = learningSpec->GetClass();
	check(kwcClass);
	assert(kwcClass == KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()));

	// Initialisation de la construction de variable
	featureConstruction.SetLearningSpec(learningSpec);
	featureConstruction.SetConstructionDomain(
	    GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetConstructionDomain());

	// On ne peut importer des couts que si aucune construction n'est demandee
	if (GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxConstructedAttributeNumber() >
	    0)
	{
		Global::AddError("", "", "Variable construction is not compatible with the import of variable costs");
		bOk = false;
	}
	else if (GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxTreeNumber() > 0)
	{
		Global::AddError("", "", "Tree construction is not compatible with the import of variable costs");
		bOk = false;
	}
	else if (GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxAttributePairNumber() > 0)
	{
		Global::AddError("", "",
				 "Variable pair construction is not compatible with the import of variable costs");
		bOk = false;
	}
	// Tentative d'import des couts sinon
	else
	{
		// On duplique la classe
		constructedDomain = kwcClass->GetDomain()->CloneFromClass(kwcClass);
		constructedDomain->Compile();
		constructedClass = constructedDomain->LookupClass(GetClassName());

		// Tentative d'imort des couts
		bOk = featureConstruction.ImportAttributeMetaDataCosts(constructedClass);
	}

	// Modification de la classe des learning spec si ok et classe construite
	if (bOk)
	{
		assert(constructedClass != NULL);
		learningSpec->SetClass(constructedClass);
	}

	// Nettoyage si necessaire
	if (not bOk and constructedDomain != NULL)
	{
		delete constructedDomain;
		constructedClass = NULL;
	}
	ensure(learningSpec->Check());
	return bOk;
}

void KWLearningProblem::InitializeClassStats(KWClassStats* classStats, KWLearningSpec* learningSpec)
{
	KWAttributeConstructionReport* attributeTreeConstructionReport;

	require(classStats != NULL);
	require(learningSpec != NULL);

	// Parametrage de classStats
	classStats->SetLearningSpec(learningSpec);

	// Parametrage d'un rapport dedie au parametrage de la construction d'attribut
	attributeTreeConstructionReport = new KWAttributeConstructionReport;
	attributeTreeConstructionReport->SetAttributeConstructionSpec(
	    GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec());
	classStats->SetAttributeTreeConstructionReport(attributeTreeConstructionReport);

	// Parametrage des arbres a construire
	if (KDDataPreparationAttributeCreationTask::GetGlobalCreationTask())
	{
		// On recopie les specifications de creation d'attributs
		KDDataPreparationAttributeCreationTask::GetGlobalCreationTask()->CopyAttributeCreationSpecFrom(
		    analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->GetAttributeCreationParameters());

		// On recopie le nombre d'attributs a construire, qui est specifie dans au niveau au dessus
		KDDataPreparationAttributeCreationTask::GetGlobalCreationTask()->SetMaxCreatedAttributeNumber(
		    analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxTreeNumber());
	}

	// Parametrage des paires d'attributs a evaluer
	classStats->SetAttributePairsSpec(
	    analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->GetAttributePairsSpec());

	assert(classStats->Check());
}

boolean KWLearningProblem::IsSpecificRegressionLearningSpecNecessary(const KWLearningSpec* learningSpec) const
{
	boolean bNecessary;

	require(learningSpec != NULL);

	// Test du cas particulier de la regression, si la classe cible contient des valeurs Missing
	// Dans ce cas, la variable cible est disponible, mais on a pas pu calcule les stats
	bNecessary = learningSpec->GetTargetAttributeType() == KWType::Continuous;
	bNecessary = bNecessary and learningSpec->IsTargetStatsComputed();
	bNecessary =
	    bNecessary and
	    cast(KWDescriptiveContinuousStats*, learningSpec->GetTargetDescriptiveStats())->GetMissingValueNumber() > 0;
	return bNecessary;
}

void KWLearningProblem::PrepareLearningSpecForRegression(KWLearningSpec* learningSpec) const
{
	KWClass* kwcClass;
	KWAttribute* selectionAttribute;
	KWAttribute* filterAttribute;
	KWDRNEQ* notMissingRule;
	KWDerivationRule* selectionRule;
	KWDerivationRule* filterRule;

	require(learningSpec != NULL);
	require(IsSpecificRegressionLearningSpecNecessary(learningSpec));

	// Recherche de la la classe en cours
	kwcClass = learningSpec->GetClass();

	// Creation d'une regle de filtrage des valeurs cibles manquantes
	notMissingRule = new KWDRNEQ;
	notMissingRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	notMissingRule->GetFirstOperand()->SetAttributeName(learningSpec->GetTargetAttributeName());
	notMissingRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	notMissingRule->GetSecondOperand()->SetContinuousConstant(KWContinuous::GetMissingValue());
	filterRule = notMissingRule;

	// Combinaison avec l'attribut de selection en cours si necessaire
	if (learningSpec->GetDatabase()->GetSelectionAttribute() != "")
	{
		// Recherche de l'attribut de selection
		selectionAttribute = kwcClass->LookupAttribute(learningSpec->GetDatabase()->GetSelectionAttribute());
		assert(selectionAttribute != NULL);

		// Creation d'une regle reconstituant la selection
		if (selectionAttribute->GetType() == KWType::Continuous)
		{
			selectionRule = new KWDREQ;
			selectionRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			selectionRule->GetFirstOperand()->SetAttributeName(selectionAttribute->GetName());
			selectionRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
			selectionRule->GetSecondOperand()->SetContinuousConstant(
			    KWContinuous::StringToContinuous(learningSpec->GetDatabase()->GetSelectionValue()));
		}
		else
		{
			assert(selectionAttribute->GetType() == KWType::Symbol);
			selectionRule = new KWDRSymbolEQ;
			selectionRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			selectionRule->GetFirstOperand()->SetAttributeName(selectionAttribute->GetName());
			selectionRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
			selectionRule->GetSecondOperand()->SetSymbolConstant(
			    Symbol(learningSpec->GetDatabase()->GetSelectionValue()));
		}

		// Combinaison de la selection existante et de la regle de filtrage
		filterRule = new KWDRAnd;
		filterRule->AddOperand(filterRule->GetFirstOperand()->Clone());
		filterRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		filterRule->GetFirstOperand()->SetDerivationRule(notMissingRule);
		filterRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		filterRule->GetSecondOperand()->SetDerivationRule(selectionRule);
	}

	// Creation d'un attribut pour filtrer les valeurs cibles manquantes
	filterAttribute = new KWAttribute;
	filterAttribute->SetName(kwcClass->BuildAttributeName("FilterTargetMissingValues"));
	filterAttribute->SetDerivationRule(filterRule);
	filterAttribute->SetUsed(false);
	filterAttribute->CompleteTypeInfo(kwcClass);
	kwcClass->InsertAttribute(filterAttribute);

	// Compilation de la classe
	kwcClass->GetDomain()->Compile();

	// Parametrage de la base par l'attribut de filtrage
	learningSpec->GetDatabase()->SetSelectionAttribute(filterAttribute->GetName());
	learningSpec->GetDatabase()->SetSelectionValue("1");
	ensure(learningSpec->Check());
}

void KWLearningProblem::RestoreInitialLearningSpec(KWLearningSpec* learningSpec,
						   const KWDatabase* initialDatabase) const
{
	KWClass* kwcClass;
	KWAttribute* filterAttribute;

	require(learningSpec != NULL);
	require(learningSpec->GetTargetAttributeType() == KWType::Continuous);
	require(learningSpec->IsTargetStatsComputed());
	require(
	    cast(KWDescriptiveContinuousStats*, learningSpec->GetTargetDescriptiveStats())->GetMissingValueNumber() ==
	    0);
	require(learningSpec->GetDatabase()->GetSelectionAttribute() != "");
	require(initialDatabase != NULL);

	// Recherche de la la classe en cours
	kwcClass = learningSpec->GetClass();

	// Recherche de l'attribut de selection utilise pour le filtrage de la base
	filterAttribute = kwcClass->LookupAttribute(learningSpec->GetDatabase()->GetSelectionAttribute());
	assert(filterAttribute != NULL);
	assert(filterAttribute->GetName().Find("FilterTargetMissingValues") == 0);

	// Supression de l'attribut de selection de la classe
	kwcClass->DeleteAttribute(filterAttribute->GetName());

	// Compilation de la classe
	kwcClass->GetDomain()->Compile();

	// Restitution du parametrage initial de la base
	learningSpec->GetDatabase()->CopySamplingAndSelectionFrom(initialDatabase);
	ensure(learningSpec->Check());
}

void KWLearningProblem::DeleteAllOutputFiles()
{
	ALString sOutputFileName;

	// Destruction de tous les fichiers de sortie potentiels
	DeleteOutputFile(BuildOutputFilePathName(analysisResults->GetJSONFileName()));
	DeleteOutputFile(BuildOutputFilePathName(analysisResults->GetPreparationFileName()));
	DeleteOutputFile(BuildOutputFilePathName(analysisResults->GetPreparation2DFileName()));
	DeleteOutputFile(BuildOutputFilePathName(analysisResults->GetModelingFileName()));
	DeleteOutputFile(BuildOutputFilePathName(analysisResults->GetModelingDictionaryFileName()));
	DeleteOutputFile(BuildOutputFilePathName(analysisResults->GetTrainEvaluationFileName()));
	DeleteOutputFile(BuildOutputFilePathName(analysisResults->GetTestEvaluationFileName()));
}

void KWLearningProblem::DeleteOutputFile(const ALString& sOutputFilePathName)
{
	if (sOutputFilePathName != "")
		PLRemoteFileService::RemoveFile(sOutputFilePathName);
}

void KWLearningProblem::WritePreparationReports(KWClassStats* classStats)
{
	ALString sReportName;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());

	// Ecriture d'un rapport de stats univarie complet
	sReportName = BuildOutputFilePathName(analysisResults->GetPreparationFileName());
	if (sReportName != "")
	{
		classStats->SetAllWriteOptions(false);
		classStats->SetWriteOptionStats1D(true);
		classStats->SetWriteOptionDetailedStats(true);
		classStats->WriteReportFile(sReportName);
		classStats->SetAllWriteOptions(true);
	}

	// Ecriture d'un rapport de stats univarie dans le cas des textes
	sReportName = BuildOutputFilePathName(analysisResults->GetTextPreparationFileName());
	if (sReportName != "" and
	    analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxTextFeatureNumber() > 0 and
	    classStats->GetTextAttributeStats()->GetSize() > 0 and GetLearningTextVariableMode())
	{
		classStats->SetAllWriteOptions(false);
		classStats->SetWriteOptionStatsText(true);
		classStats->SetWriteOptionDetailedStats(true);
		classStats->WriteReportFile(sReportName);
		classStats->SetAllWriteOptions(true);
	}

	// Ecriture d'un rapport de stats univarie dans le cas des arbres
	sReportName = BuildOutputFilePathName(analysisResults->GetTreePreparationFileName());
	if (sReportName != "" and
	    analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxTreeNumber() > 0 and
	    classStats->GetTreeAttributeStats()->GetSize() > 0 and GetForestExpertMode())
	{
		classStats->SetAllWriteOptions(false);
		classStats->SetWriteOptionStatsTrees(true);
		classStats->SetWriteOptionDetailedStats(true);
		classStats->WriteReportFile(sReportName);
		classStats->SetAllWriteOptions(true);
	}

	// Ecriture d'un rapport de stats bivarie complet
	sReportName = BuildOutputFilePathName(analysisResults->GetPreparation2DFileName());
	if (sReportName != "" and
	    analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxAttributePairNumber() > 0 and
	    classStats->GetAttributePairStats()->GetSize() > 0 and
	    classStats->GetTargetAttributeType() != KWType::Continuous)
	{
		classStats->SetAllWriteOptions(false);
		classStats->SetWriteOptionStats2D(true);
		classStats->SetWriteOptionDetailedStats(true);
		classStats->WriteReportFile(sReportName);
		classStats->SetAllWriteOptions(true);
	}
}

void KWLearningProblem::WriteJSONAnalysisReport(KWClassStats* classStats, ObjectArray* oaTrainedPredictorReports,
						ObjectArray* oaTrainPredictorEvaluations,
						ObjectArray* oaTestPredictorEvaluations)
{
	ALString sJSONReportName;
	JSONFile fJSON;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(oaTrainedPredictorReports != NULL);
	require(oaTrainPredictorEvaluations != NULL);
	require(oaTestPredictorEvaluations != NULL);

	// Ecriture d'un rapport de stats univarie complet
	sJSONReportName = BuildOutputFilePathName(analysisResults->GetJSONFileName());
	if (sJSONReportName != "")
	{
		// Message synthetique signifiant que tout s'est bien passe et indiquant
		// ou se trouve le fichier de resultat
		AddSimpleMessage("Write report " + sJSONReportName);

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

			// Description courte
			fJSON.WriteKeyString("shortDescription", analysisResults->GetShortDescription());

			// Liste des messages d'erreur potentiellement detectees pendant l'analyse
			KWLearningErrorManager::WriteJSONKeyReport(&fJSON);

			// Rapport de modelisation
			if (analysisResults->GetModelingFileName() != "" and oaTrainedPredictorReports->GetSize() > 0)
			{
				fJSON.BeginKeyObject("modelingReport");
				cast(KWPredictorReport*, oaTrainedPredictorReports->GetAt(0))
				    ->WriteJSONFullReportFields(&fJSON, oaTrainedPredictorReports);
				fJSON.EndObject();
			}

			// Rapport d'evaluation en train
			if (analysisResults->GetTrainEvaluationFileName() != "" and
			    oaTrainPredictorEvaluations->GetSize() > 0)
			{
				fJSON.BeginKeyObject("trainEvaluationReport");
				cast(KWPredictorEvaluation*, oaTrainPredictorEvaluations->GetAt(0))
				    ->WriteJSONFullReportFields(&fJSON, "Train", oaTrainPredictorEvaluations);
				fJSON.EndObject();
			}

			// Rapport d'evaluation en test
			if (analysisResults->GetTestEvaluationFileName() != "" and
			    oaTestPredictorEvaluations->GetSize() > 0)
			{
				fJSON.BeginKeyObject("testEvaluationReport");
				cast(KWPredictorEvaluation*, oaTestPredictorEvaluations->GetAt(0))
				    ->WriteJSONFullReportFields(&fJSON, "Test", oaTestPredictorEvaluations);
				fJSON.EndObject();
			}

			// Rapport de preparation
			if (analysisResults->GetPreparationFileName() != "")
			{
				classStats->SetAllWriteOptions(false);
				classStats->SetWriteOptionStats1D(true);
				classStats->SetWriteOptionDetailedStats(true);
				classStats->WriteJSONKeyReport(&fJSON, "preparationReport");
				classStats->SetAllWriteOptions(true);
			}

			// Rapport de preparation pour les variables de type texte
			if (analysisResults->GetPreparationFileName() != "" and
			    classStats->GetTextAttributeStats()->GetSize() > 0)
			{
				classStats->SetAllWriteOptions(false);
				classStats->SetWriteOptionStatsText(true);
				classStats->SetWriteOptionDetailedStats(true);
				classStats->WriteJSONKeyReport(&fJSON, "textPreparationReport");
				classStats->SetAllWriteOptions(true);
			}

			// Rapport de preparation pour les variables de type arbre
			if (analysisResults->GetPreparationFileName() != "" and
			    classStats->IsTreeConstructionRequired() and
			    classStats->GetTreeAttributeStats()->GetSize() > 0)
			{
				classStats->SetAllWriteOptions(false);
				classStats->SetWriteOptionStatsTrees(true);
				classStats->SetWriteOptionDetailedStats(true);
				classStats->WriteJSONKeyReport(
				    &fJSON, classStats->GetAttributeTreeConstructionTask()->GetReportPrefix() +
						"PreparationReport");
				classStats->SetAllWriteOptions(true);
			}

			// On a plus besoin du rapport de preparation des arbres une fois exploite pour le rapport
			classStats->DeleteAttributeTreeConstructionTask();

			// Rapport de preparation bivarie
			if (analysisResults->GetPreparation2DFileName() != "" and
			    analysisSpec->GetModelingSpec()
				    ->GetAttributeConstructionSpec()
				    ->GetMaxAttributePairNumber() > 0 and
			    classStats->GetAttributePairStats()->GetSize() > 0 and
			    classStats->GetTargetAttributeType() != KWType::Continuous)
			{
				classStats->SetAllWriteOptions(false);
				classStats->SetWriteOptionStats2D(true);
				classStats->SetWriteOptionDetailedStats(true);
				classStats->WriteJSONKeyReport(&fJSON, "bivariatePreparationReport");
				classStats->SetAllWriteOptions(true);
			}

			// Fermeture du fichier
			fJSON.Close();
		}
	}
}

void KWLearningProblem::BuildRecodingClass(const KWClassDomain* initialDomain, KWClassStats* classStats,
					   KWClassDomain* trainedClassDomain)
{
	const ALString sRecodingPrefix = "R_";
	KWClass* kwcClass;
	KWClass* preparedClass;
	KWAttribute* attribute;
	KWClassDomain* preparedDomain;
	KWRecodingSpec* recodingSpec;
	KWDataPreparationClass dataPreparationClass;
	KWDataPreparationAttribute* dataPreparationAttribute;
	ObjectArray oaSelectedDataPreparationAttributes;
	NumericKeyDictionary nkdSelectedDataPreparationAttributes;
	int nAttribute;
	boolean bFilterAttribute;
	int nNative;
	ObjectArray oaAddedAttributes;

	require(initialDomain != NULL);
	require(initialDomain->LookupClass(GetClassName()) != NULL);
	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(GetAnalysisSpec()->GetRecoderSpec()->GetRecodingSpec()->Check());
	require(trainedClassDomain != NULL);

	// Message utilisateur
	kwcClass = classStats->GetLearningSpec()->GetClass();
	AddSimpleMessage("Build data preparation dictionary " + sRecodingPrefix + kwcClass->GetName());

	// Creation de la classe de recodage
	dataPreparationClass.SetLearningSpec(classStats->GetLearningSpec());
	dataPreparationClass.ComputeDataPreparationFromClassStats(classStats);
	preparedClass = dataPreparationClass.GetDataPreparationClass();
	preparedDomain = dataPreparationClass.GetDataPreparationDomain();

	// Libelle de la classe
	preparedClass->SetLabel("Recoding dictionary");
	if (kwcClass->GetLabel() != "")
		preparedClass->SetLabel("Recoding dictionary: " + kwcClass->GetLabel());

	// Acces aux parametres de recodage
	recodingSpec = GetAnalysisSpec()->GetRecoderSpec()->GetRecodingSpec();

	// Memorisation des attributs informatifs
	for (nAttribute = 0; nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize(); nAttribute++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*,
						dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

		// S'il y a filtrage, on ne garde que ceux de valeur (Level, DeltaLevel...) strictement positive en
		// supervise,
		if (GetTargetAttributeName() != "")
			bFilterAttribute = recodingSpec->GetFilterAttributes() and
					   dataPreparationAttribute->GetPreparedStats()->GetSortValue() <= 0;
		else
		// et ceux ayant au moins deux valeurs en non supervise
		{
			if (dataPreparationAttribute->GetNativeAttributeNumber() == 1)
				bFilterAttribute = recodingSpec->GetFilterAttributes() and
						   cast(KWAttributeStats*, dataPreparationAttribute->GetPreparedStats())
							   ->GetDescriptiveStats()
							   ->GetValueNumber() <= 1;
			else
				bFilterAttribute = recodingSpec->GetFilterAttributes() and
						   dataPreparationAttribute->GetPreparedStats()->GetSortValue() <= 0;
		}
		if (not bFilterAttribute)
			oaSelectedDataPreparationAttributes.Add(dataPreparationAttribute);
	}

	// Calcul si necessaire des attributs a traiter
	// On les memorise dans un tableau temporaire trie par valeur predictive decroissante, puis
	// dans un dictionnaire permettant de tester s'il faut les traiter
	// Cela permet ensuite de parcourir les attributs dans leur ordre initial
	oaSelectedDataPreparationAttributes.SetCompareFunction(KWDataPreparationAttributeCompareSortValue);
	oaSelectedDataPreparationAttributes.Sort();
	for (nAttribute = 0; nAttribute < oaSelectedDataPreparationAttributes.GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaSelectedDataPreparationAttributes.GetAt(nAttribute));

		// On selectionne l'attribut selon le nombre max demande
		if (recodingSpec->GetMaxFilteredAttributeNumber() == 0 or
		    (recodingSpec->GetMaxFilteredAttributeNumber() > 0 and
		     nkdSelectedDataPreparationAttributes.GetCount() < recodingSpec->GetMaxFilteredAttributeNumber()))
			nkdSelectedDataPreparationAttributes.SetAt((NUMERIC)dataPreparationAttribute,
								   dataPreparationAttribute);
	}
	oaSelectedDataPreparationAttributes.SetSize(0);
	assert(recodingSpec->GetMaxFilteredAttributeNumber() == 0 or
	       (recodingSpec->GetMaxFilteredAttributeNumber() > 0 and
		nkdSelectedDataPreparationAttributes.GetCount() <= recodingSpec->GetMaxFilteredAttributeNumber()));

	// Parcours des attributs de preparation pour mettre tous les attributs natifs en Unused par defaut
	for (nAttribute = 0; nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize(); nAttribute++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*,
						dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

		// Parametrage des variables natives en Unused
		for (nNative = 0; nNative < dataPreparationAttribute->GetNativeAttributeNumber(); nNative++)
			dataPreparationAttribute->GetNativeAttributeAt(nNative)->SetUsed(false);
	}

	// Parcours des attributs de preparation, dans le meme ordre que celui des attributs prepares
	for (nAttribute = 0; nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize(); nAttribute++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*,
						dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

		// Filtrage de l'attribut s'il n'est pas informatif
		bFilterAttribute =
		    (nkdSelectedDataPreparationAttributes.Lookup((NUMERIC)dataPreparationAttribute) == NULL);

		// Creation des variables recodees
		dataPreparationAttribute->GetPreparedAttribute()->SetUsed(false);
		if (not bFilterAttribute)
		{
			// Recodage selon la distance probabiliste (mode expert uniquement)
			// Chaque variable servant a mesurer la distance entre deux individus est suivi
			// d'une variable d'auto-distance, a utiliser uniquement en cas d'egalite de distance
			if (recodingSpec->GetRecodeProbabilisticDistance())
			{
				assert(GetDistanceStudyMode());
				dataPreparationAttribute->AddPreparedDistanceStudyAttributes(&oaAddedAttributes);
			}
			// Cas des variables Continuous
			else if (dataPreparationAttribute->GetNativeAttributeNumber() == 1 and
				 dataPreparationAttribute->GetNativeAttribute()->GetType() == KWType::Continuous)
			{
				// Recodage par identifiant de partie
				if (recodingSpec->GetRecodeContinuousAttributes() == "part Id")
					dataPreparationAttribute->AddPreparedIdAttribute();
				// Recodage par libelle de partie
				else if (recodingSpec->GetRecodeContinuousAttributes() == "part label")
					dataPreparationAttribute->AddPreparedLabelAttribute();
				// Recodage disjonctif complet de l'identifiant de partie
				else if (recodingSpec->GetRecodeContinuousAttributes() == "0-1 binarization")
					dataPreparationAttribute->AddPreparedBinarizationAttributes(&oaAddedAttributes);
				// Recodage par les informations conditionnelles de la source sachant la cible
				else if (recodingSpec->GetRecodeContinuousAttributes() == "conditional info")
					dataPreparationAttribute->AddPreparedSourceConditionalInfoAttributes(
					    &oaAddedAttributes);
				// Normalisation par centrage-reduction
				else if (recodingSpec->GetRecodeContinuousAttributes() == "center-reduction")
					dataPreparationAttribute->AddPreparedCenterReducedAttribute();
				// Normalisation 0-1
				else if (recodingSpec->GetRecodeContinuousAttributes() == "0-1 normalization")
					dataPreparationAttribute->AddPreparedNormalizedAttribute();
				// Normalisation par le rang
				else if (recodingSpec->GetRecodeContinuousAttributes() == "rank normalization")
					dataPreparationAttribute->AddPreparedRankNormalizedAttribute();
			}
			// Cas des variables Symbol
			else if (dataPreparationAttribute->GetNativeAttributeNumber() == 1 and
				 dataPreparationAttribute->GetNativeAttribute()->GetType() == KWType::Symbol)
			{
				// Recodage par identifiant de partie
				if (recodingSpec->GetRecodeSymbolAttributes() == "part Id")
					dataPreparationAttribute->AddPreparedIdAttribute();
				// Recodage par libelle de partie
				else if (recodingSpec->GetRecodeSymbolAttributes() == "part label")
					dataPreparationAttribute->AddPreparedLabelAttribute();
				// Recodage disjonctif complet de l'identifiant de partie
				else if (recodingSpec->GetRecodeSymbolAttributes() == "0-1 binarization")
					dataPreparationAttribute->AddPreparedBinarizationAttributes(&oaAddedAttributes);
				// Recodage par les informations conditionnelles de la source sachant la cible
				else if (recodingSpec->GetRecodeSymbolAttributes() == "conditional info")
					dataPreparationAttribute->AddPreparedSourceConditionalInfoAttributes(
					    &oaAddedAttributes);
			}
			// Cas multivarie
			else
			{
				assert(dataPreparationAttribute->GetNativeAttributeNumber() > 1);

				// Recodage par identifiant de partie
				if (recodingSpec->GetRecodeBivariateAttributes() == "part Id")
					dataPreparationAttribute->AddPreparedIdAttribute();
				// Recodage par libelle de partie
				else if (recodingSpec->GetRecodeBivariateAttributes() == "part label")
					dataPreparationAttribute->AddPreparedLabelAttribute();
				// Recodage disjonctif complet de l'identifiant de partie
				else if (recodingSpec->GetRecodeBivariateAttributes() == "0-1 binarization")
					dataPreparationAttribute->AddPreparedBinarizationAttributes(&oaAddedAttributes);
				// Recodage par les informations conditionnelles de la source sachant la cible
				else if (recodingSpec->GetRecodeBivariateAttributes() == "conditional info")
					dataPreparationAttribute->AddPreparedSourceConditionalInfoAttributes(
					    &oaAddedAttributes);
			}
		}

		// Transfer des variables natives, si elle doivent etre utilise au moins une fois
		// (une variable native peut intervenir dans plusieurs attributs prepares (e.g: bivariate))
		for (nNative = 0; nNative < dataPreparationAttribute->GetNativeAttributeNumber(); nNative++)
		{
			if (dataPreparationAttribute->GetNativeAttributeAt(nNative)->GetType() == KWType::Continuous)
			{
				dataPreparationAttribute->GetNativeAttributeAt(nNative)->SetUsed(
				    dataPreparationAttribute->GetNativeAttributeAt(nNative)->GetUsed() or
				    (not bFilterAttribute and recodingSpec->GetKeepInitialContinuousAttributes()));
			}
			else if (dataPreparationAttribute->GetNativeAttributeAt(nNative)->GetType() == KWType::Symbol)
			{
				dataPreparationAttribute->GetNativeAttributeAt(nNative)->SetUsed(
				    dataPreparationAttribute->GetNativeAttributeAt(nNative)->GetUsed() or
				    (not bFilterAttribute and recodingSpec->GetKeepInitialSymbolAttributes()));
			}
			dataPreparationAttribute->GetNativeAttributeAt(nNative)->SetLoaded(
			    dataPreparationAttribute->GetNativeAttributeAt(nNative)->GetUsed());
		}
	}

	// On passe tous les attributs non simple en Unused
	attribute = preparedClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		if (not KWType::IsSimple(attribute->GetType()))
			attribute->SetUsed(false);
		preparedClass->GetNextAttribute(attribute);
	}

	// Supression des attribut inutiles (necessite une classe compilee)
	preparedDomain->Compile();
	preparedClass->DeleteUnusedDerivedAttributes(initialDomain);

	// Transfert du domaine de preparation dans el domaine cible
	trainedClassDomain->ImportDomain(preparedDomain, sRecodingPrefix, "");
	delete preparedDomain;
	dataPreparationClass.RemoveDataPreparation();
	ensure(trainedClassDomain->Check());
}