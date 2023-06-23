// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblem.h"

CCLearningProblem::CCLearningProblem()
{
	// Creation explicite des sous-objets, ce qui permet de creer des sous-objets specifiques dans des sous-classes
	classManagement = new KWClassManagement;
	database = KWDatabase::CreateDefaultDatabaseTechnology();
	analysisSpec = new CCAnalysisSpec;
	postProcessingSpec = new CCPostProcessingSpec;
	deploymentSpec = new CCDeploymentSpec;
	analysisResults = new CCAnalysisResults;
}

CCLearningProblem::~CCLearningProblem()
{
	delete classManagement;
	delete database;
	delete analysisResults;
	delete postProcessingSpec;
	delete deploymentSpec;
	delete analysisSpec;
}

/////////////////////////////////////////////////////////////////////////////

KWClassManagement* CCLearningProblem::GetClassManagement()
{
	return classManagement;
}

KWDatabase* CCLearningProblem::GetDatabase()
{
	// Synchronisation du dictionnaire de la base
	database->SetClassName(classManagement->GetClassName());
	return database;
}

CCAnalysisSpec* CCLearningProblem::GetAnalysisSpec()
{
	return analysisSpec;
}

CCPostProcessingSpec* CCLearningProblem::GetPostProcessingSpec()
{
	return postProcessingSpec;
}

CCDeploymentSpec* CCLearningProblem::GetDeploymentSpec()
{
	return deploymentSpec;
}

CCAnalysisResults* CCLearningProblem::GetAnalysisResults()
{
	return analysisResults;
}

/////////////////////////////////////////////////////////////////////////////

ALString CCLearningProblem::GetClassFileName() const
{
	return classManagement->GetClassFileName();
}

ALString CCLearningProblem::GetClassName() const
{
	return classManagement->GetClassName();
}

int CCLearningProblem::GetAttributeNumber() const
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

ALString CCLearningProblem::GetDatabaseName() const
{
	return database->GetDatabaseName();
}

/////////////////////////////////////////////////////////////////////////////

void CCLearningProblem::BuildCoclustering()
{
	KWClass* kwcClass;
	KWLearningSpec learningSpec;
	CCCoclusteringBuilder coclusteringBuilder;
	CCCoclusteringReport coclusteringReport;
	fstream ost;
	ALString sTmp;
	int nAttribute;
	KWAttribute* kwAttribute;
	KWAttributeName* attributeName;
	ALString sReportFileName;
	Timer timer;
	boolean bWriteOk;

	require(CheckClass());
	require(CheckDatabaseName());
	require(GetDatabase()->CheckSelectionValue(GetDatabase()->GetSelectionValue()));
	require(CheckCoclusteringAttributeNames());
	require(CheckResultFileNames(TaskBuildCoclustering));
	require(not TaskProgression::IsStarted());

	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Train model " + GetClassName());
	TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();

	// Initialisations
	kwcClass = NULL;

	// Demarage du timer
	timer.Start();

	// Recherche de la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	check(kwcClass);

	// Parametrage des attributs de la base a lire
	kwcClass->SetAllAttributesLoaded(false);
	for (nAttribute = 0; nAttribute < analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize(); nAttribute++)
	{
		attributeName =
		    cast(KWAttributeName*, analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetAt(nAttribute));
		kwAttribute = kwcClass->LookupAttribute(attributeName->GetName());
		check(kwAttribute);
		kwAttribute->SetLoaded(true);
	}
	if (analysisSpec->GetCoclusteringSpec()->GetFrequencyAttribute() != "")
	{
		kwAttribute = kwcClass->LookupAttribute(analysisSpec->GetCoclusteringSpec()->GetFrequencyAttribute());
		check(kwAttribute);
		kwAttribute->SetLoaded(true);
	}
	KWClassDomain::GetCurrentDomain()->Compile();

	// Parametrage des specifications d'apprentissage
	learningSpec.SetShortDescription(GetAnalysisResults()->GetShortDescription());
	learningSpec.SetDatabase(database);
	learningSpec.SetClass(kwcClass);

	// Parametrage du coclustering
	coclusteringBuilder.SetLearningSpec(&learningSpec);
	coclusteringBuilder.SetAttributeNumber(analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize());
	for (nAttribute = 0; nAttribute < analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize(); nAttribute++)
	{
		attributeName =
		    cast(KWAttributeName*, analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetAt(nAttribute));
		coclusteringBuilder.SetAttributeNameAt(nAttribute, attributeName->GetName());
	}
	coclusteringBuilder.SetFrequencyAttribute(analysisSpec->GetCoclusteringSpec()->GetFrequencyAttribute());

	// Parametrage du nom du rapport Khiphren
	sReportFileName = BuildOutputFilePathName(GetAnalysisResults()->GetCoclusteringFileName(), false);
	coclusteringBuilder.SetReportFileName(sReportFileName);
	coclusteringBuilder.SetExportJSON(GetAnalysisResults()->GetExportJSON());

	// Calcul du coclustering
	if (not TaskProgression::IsInterruptionRequested())
		coclusteringBuilder.ComputeCoclustering();

	// Message si pas de coclustering informatif trouve en depit du temp imparti
	if (coclusteringBuilder.IsCoclusteringComputed() and not coclusteringBuilder.IsCoclusteringInformative())
		AddSimpleMessage("No informative coclustering found in data");

	// Ecriture du rapport Khiphren
	if (coclusteringBuilder.IsCoclusteringInformative() and sReportFileName != "")
	{
		AddSimpleMessage("Write coclustering report " + sReportFileName);
		bWriteOk =
		    coclusteringReport.WriteReport(sReportFileName, coclusteringBuilder.GetCoclusteringDataGrid());

		// Sauvegarde au format JSON si necessaire
		if (bWriteOk and GetAnalysisResults()->GetExportJSON())
			coclusteringReport.WriteJSONReport(
			    FileService::SetFileSuffix(sReportFileName, CCCoclusteringReport::GetJSONReportSuffix()),
			    coclusteringBuilder.GetCoclusteringDataGrid());

		// Destruction de la derniere sauvegarde de fichier temporaire
		if (bWriteOk)
			coclusteringBuilder.RemoveLastSavedReportFile();

		// Warning si moins d'attributs dans le coclustering que d'attributs specifiees
		if (coclusteringBuilder.GetCoclusteringDataGrid()->GetAttributeNumber() <
		    coclusteringBuilder.GetAttributeNumber())
			AddWarning(sTmp + "The built coclustering only exploits " +
				   IntToString(coclusteringBuilder.GetCoclusteringDataGrid()->GetAttributeNumber()) +
				   " out of the " + IntToString(coclusteringBuilder.GetAttributeNumber()) +
				   " input variables");
	}

	// Nettoyage
	kwcClass->SetAllAttributesLoaded(true);
	KWClassDomain::GetCurrentDomain()->Compile();

	// Temps final
	timer.Stop();

	// Messsage de fin
	if (not TaskProgression::IsInterruptionRequested())
		AddSimpleMessage(sTmp + "Coclustering analysis time: " + SecondsToString(timer.GetElapsedTime()));
	else
		AddSimpleMessage(sTmp + "Coclustering analysis interrupted after " +
				 SecondsToString(timer.GetElapsedTime()));

	// Fin du suivi de la tache
	TaskProgression::Stop();

	ensure(not TaskProgression::IsStarted());
}

void CCLearningProblem::PostProcessCoclustering()
{
	boolean bOk = true;
	CCCoclusteringReport coclusteringReport;
	ALString sCoclusteringReportFileName;
	ALString sPostProcessedCoclusteringReportFileName;
	CCHierarchicalDataGrid postProcessedCoclusteringDataGrid;
	CCCoclusteringReport postProcessedcoclusteringReport;

	require(CheckResultFileNames(TaskPostProcessCoclustering));

	// Parametrage des specifications de coclustering a partir du rapport de coclustering
	sCoclusteringReportFileName = GetAnalysisResults()->GetInputCoclusteringFileName();
	sPostProcessedCoclusteringReportFileName =
	    BuildOutputFilePathName(GetAnalysisResults()->GetPostProcessedCoclusteringFileName(), true);

	// Recherche du nom du fichier de coclustering
	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Simplify coclustering " + sCoclusteringReportFileName);
	TaskProgression::SetDisplayedLevelNumber(1);
	TaskProgression::Start();

	// Lecture du rapport de coclustering
	if (bOk)
		bOk = coclusteringReport.ReadGenericReport(sCoclusteringReportFileName,
							   &postProcessedCoclusteringDataGrid);

	// Post-traitement
	if (bOk)
		bOk = GetPostProcessingSpec()->PostProcessCoclustering(&postProcessedCoclusteringDataGrid);

	// Ecriture du rapport si OK
	if (bOk and sPostProcessedCoclusteringReportFileName != "")
	{
		AddSimpleMessage("Write simplified report " + sPostProcessedCoclusteringReportFileName);
		bOk = postProcessedcoclusteringReport.WriteReport(sPostProcessedCoclusteringReportFileName,
								  &postProcessedCoclusteringDataGrid);

		// Sauvegarde au format JSON si necessaire
		if (bOk and GetAnalysisResults()->GetExportJSON())
			postProcessedcoclusteringReport.WriteJSONReport(
			    FileService::SetFileSuffix(sPostProcessedCoclusteringReportFileName,
						       CCCoclusteringReport::GetJSONReportSuffix()),
			    &postProcessedCoclusteringDataGrid);
	}

	// Fin du suivi de la tache
	TaskProgression::Stop();

	ensure(not TaskProgression::IsStarted());
}

void CCLearningProblem::ExtractClusters(const ALString& sCoclusteringAttributeName)
{
	boolean bOk = true;
	CCCoclusteringReport coclusteringReport;
	ALString sCoclusteringReportFileName;
	fstream fstClusterTableFile;
	ALString sClusterTableFileName;
	ALString sLocalFileName;
	CCHierarchicalDataGrid coclusteringDataGrid;
	CCCoclusteringReport postProcessedcoclusteringReport;
	CCHDGAttribute* coclusteringAttribute;

	require(CheckResultFileNames(TaskExtractClusters));
	require(sCoclusteringAttributeName != "");

	// Parametrage des specifications de coclustering a partir du rapport de coclustering
	sCoclusteringReportFileName = GetAnalysisResults()->GetInputCoclusteringFileName();
	sClusterTableFileName = BuildOutputFilePathName(GetAnalysisResults()->GetClusterFileName(), true);

	// Recherche du nom du fichier de coclustering
	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Extract clusters from variable " + sCoclusteringAttributeName);
	TaskProgression::SetDisplayedLevelNumber(1);
	TaskProgression::Start();

	// Lecture du rapport de coclustering
	if (bOk)
		bOk = coclusteringReport.ReadGenericReport(sCoclusteringReportFileName, &coclusteringDataGrid);

	// Post-traitement
	if (bOk)
		bOk = GetPostProcessingSpec()->PostProcessCoclustering(&coclusteringDataGrid);

	// Test si attribut present dans le coclustering
	coclusteringAttribute = NULL;
	if (bOk)
	{
		coclusteringAttribute =
		    cast(CCHDGAttribute*, coclusteringDataGrid.SearchAttribute(sCoclusteringAttributeName));
		if (coclusteringAttribute == NULL)
		{
			bOk = false;
			Global::AddError("Cluster extraction", "",
					 "Variable " + sCoclusteringAttributeName + " not found in input coclustering");
		}
	}

	// Ecriture des clusters si OK
	if (bOk and sClusterTableFileName != "")
	{
		AddSimpleMessage("Write cluster table file " + sClusterTableFileName);

		// Preparation de la copie sur HDFS si necessaire
		bOk = PLRemoteFileService::BuildOutputWorkingFile(sClusterTableFileName, sLocalFileName);

		// Ouverture du fichier de rapport en ecriture
		if (bOk)
			bOk = FileService::OpenOutputFile(sLocalFileName, fstClusterTableFile);
		if (bOk)
		{
			// Ecriture des clusters selon le type de la variable
			if (coclusteringAttribute->GetAttributeType() == KWType::Symbol)
				WriteSymbolClusters(coclusteringAttribute, fstClusterTableFile);
			else if (coclusteringAttribute->GetAttributeType() == KWType::Continuous)
				WriteContinuousClusters(coclusteringAttribute, fstClusterTableFile);

			// Ecriture du rapport
			bOk = FileService::CloseOutputFile(sLocalFileName, fstClusterTableFile);

			// Destruction du fichier si erreur
			if (not bOk)
				FileService::RemoveFile(sLocalFileName);
		}

		if (bOk)
		{
			// Copie vers HDFS
			PLRemoteFileService::CleanOutputWorkingFile(sClusterTableFileName, sLocalFileName);
		}
	}

	// Fin du suivi de la tache
	TaskProgression::Stop();

	ensure(not TaskProgression::IsStarted());
}

void CCLearningProblem::PrepareDeployment()
{
	boolean bOk = true;
	CCCoclusteringReport coclusteringReport;
	ALString sCoclusteringReportFileName;
	ALString sCoclusteringDictionaryFileName;
	CCHierarchicalDataGrid coclusteringDataGrid;
	KWClassDomain* deploymentDomain;

	require(CheckResultFileNames(TaskPrepareDeployment));

	// Parametrage des specifications de coclustering a partir du rapport de coclustering
	sCoclusteringReportFileName = GetAnalysisResults()->GetInputCoclusteringFileName();
	sCoclusteringDictionaryFileName =
	    BuildOutputFilePathName(GetAnalysisResults()->GetCoclusteringDictionaryFileName(), true);

	// Recherche du nom du fichier de coclustering
	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Prepare deployment " + sCoclusteringReportFileName);
	TaskProgression::SetDisplayedLevelNumber(1);
	TaskProgression::Start();

	// Lecture du rapport de coclustering
	if (bOk)
		bOk = coclusteringReport.ReadGenericReport(sCoclusteringReportFileName, &coclusteringDataGrid);

	// Post-traitement
	if (bOk)
		bOk = GetPostProcessingSpec()->PostProcessCoclustering(&coclusteringDataGrid);

	// Preparation d'un dictionnaire de deploiement
	deploymentDomain = NULL;
	if (bOk)
		bOk = GetDeploymentSpec()->PrepareCoclusteringDeployment(&coclusteringDataGrid, deploymentDomain);

	// Ecriture du fichier de dictionnaire de deploiement
	if (bOk)
	{
		AddSimpleMessage("Write deployment dictionary file " + sCoclusteringDictionaryFileName);
		deploymentDomain->WriteFile(sCoclusteringDictionaryFileName);
	}

	// Nettoyage
	if (bOk)
		delete deploymentDomain;

	// Fin du suivi de la tache
	TaskProgression::Stop();

	ensure(not TaskProgression::IsStarted());
}

boolean CCLearningProblem::CheckClass() const
{
	KWClass* kwcClass;

	// Erreur si classe non specifiee
	kwcClass = NULL;
	if (GetClassName() == "")
		Global::AddError("Database management", database->GetDatabaseName(), "Missing dictionary name");
	// Sinon, recherche de la classe
	else
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

		// Si non trouve, erreur
		if (kwcClass == NULL)
		{
			Global::AddError("Database management", database->GetDatabaseName(),
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

boolean CCLearningProblem::CheckDatabaseName() const
{
	if (GetDatabaseName() == "")
	{
		Global::AddError("", "", "Missing database name");
		return false;
	}
	else
		return true;
}

boolean CCLearningProblem::CheckCoclusteringAttributeNames() const
{
	boolean bOk = true;
	KWClass* kwcClass;
	int nVar;
	KWAttributeName* attributeName;
	KWAttribute* attribute;
	ObjectDictionary odCoclusteringAttributes;
	ALString sFrequencyAttributeName;
	ALString sTmp;

	require(CheckClass());

	// Recherche de la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

	// Il doit y avoir au moins deux variables
	if (analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize() < 2)
	{
		AddError("Less than two coclustering variables are specified");
		bOk = false;
	}
	// Il y a une limite au nombre de variables
	else if (analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize() >
		 CCCoclusteringSpec::GetMaxCoclusteringAttributeNumber())
	{
		AddError(sTmp + "Two many coclustering variables (> " +
			 IntToString(CCCoclusteringSpec::GetMaxCoclusteringAttributeNumber()) + ") are specified");
		bOk = false;
	}
	// Sinon, verification des variables
	else
	{
		for (nVar = 0; nVar < analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize(); nVar++)
		{
			attributeName =
			    cast(KWAttributeName*, analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetAt(nVar));

			// Recherche de la variable correspondante dans le dictionnaire
			attribute = kwcClass->LookupAttribute(attributeName->GetName());

			// La variable doit avoir un nom
			if (attributeName->GetName() == "")
			{
				AddError(sTmp + "Missing name for coclustering variable " + IntToString(nVar + 1));
				bOk = false;
			}
			// Elle doit etre presente dans le dictionnaire
			else if (attribute == NULL)
			{
				bOk = false;
				Global::AddError("", "",
						 "Coclustering variable " + attributeName->GetName() +
						     " unknown in dictionary " + GetClassName());
			}
			// De type simple
			else if (not KWType::IsSimple(attribute->GetType()))
			{
				bOk = false;
				Global::AddError(
				    "", "", "Incorrect type for coclustering variable " + attributeName->GetName());
			}
			// Et utilise
			else if (not attribute->GetUsed())
			{
				bOk = false;
				Global::AddError("", "",
						 "Coclustering variable " + attributeName->GetName() +
						     " unused in dictionary " + GetClassName());
			}

			// Si ok, verification de l'absence de doublon
			if (bOk)
			{
				if (odCoclusteringAttributes.Lookup(attributeName->GetName()) != NULL)
				{
					bOk = false;
					Global::AddError("", "",
							 "Coclustering variable " + attributeName->GetName() +
							     " is specified twice");
				}
				else
					odCoclusteringAttributes.SetAt(attributeName->GetName(), attributeName);
			}
		}
	}

	// Verification de la variable de frequence
	sFrequencyAttributeName = analysisSpec->GetCoclusteringSpec()->GetFrequencyAttribute();
	if (bOk and sFrequencyAttributeName != "")
	{
		assert(odCoclusteringAttributes.GetCount() ==
		       analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize());

		// Recherche de la variable correspondante dans le dictionnaire
		attribute = kwcClass->LookupAttribute(sFrequencyAttributeName);

		// La variable doit etre presente dans le dictionnaire
		if (attribute == NULL)
		{
			bOk = false;
			Global::AddError("", "",
					 "Coclustering frequency variable " + sFrequencyAttributeName +
					     " unknown in dictionary " + GetClassName());
		}
		// De type Continuous
		else if (attribute->GetType() != KWType::Continuous)
		{
			bOk = false;
			Global::AddError(
			    "", "", "Incorrect type for coclustering frequency variable " + sFrequencyAttributeName);
		}
		// Et utilise
		else if (not attribute->GetUsed())
		{
			bOk = false;
			Global::AddError("", "",
					 "Coclustering frequency variable " + sFrequencyAttributeName +
					     " unused in dictionary " + GetClassName());
		}
		// et differente des attributs de coclustering
		else if (odCoclusteringAttributes.Lookup(sFrequencyAttributeName) != NULL)
		{
			bOk = false;
			Global::AddError("", "",
					 "Coclustering frequency variable " + sFrequencyAttributeName +
					     " is already used among the coclustering variables");
		}
	}

	return bOk;
}

boolean CCLearningProblem::CheckResultFileNames(int nTaskId) const
{
	boolean bOk = true;
	FileSpec specInputClass;
	FileSpec specInputTrainDatabase;
	FileSpec specInputCoclustering;
	FileSpec specOutputCoclustering;
	FileSpec specOutputCoclusteringDictionary;
	FileSpec specOutputPostProcessedCoclustering;
	FileSpec specOutputTrainEvaluation;
	FileSpec specOutputTestEvaluation;
	FileSpec specOutputProject;
	FileSpec specOutputJSONCoclustering;
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

	require(nTaskId == TaskBuildCoclustering or nTaskId == TaskExtractClusters or
		nTaskId == TaskPostProcessCoclustering or nTaskId == TaskPrepareDeployment);

	// Initialisation des fichiers d'entree concernes
	specInputClass.SetLabel("input dictionary");
	specInputClass.SetFilePathName(classManagement->GetClassFileName());
	oaInputFileSpecs.Add(&specInputClass);

	// Verification du fichier de coclustering en entree des post-traitements
	if (nTaskId == TaskExtractClusters or nTaskId == TaskPostProcessCoclustering or
	    nTaskId == TaskPrepareDeployment)
	{
		specInputCoclustering.SetLabel("input coclustering");
		specInputCoclustering.SetFilePathName(analysisResults->GetInputCoclusteringFileName());
		oaInputFileSpecs.Add(&specInputCoclustering);

		// Test si fichier specifie
		if (analysisResults->GetInputCoclusteringFileName() == "")
		{
			bOk = false;
			specInputCoclustering.AddError("Missing file name");
		}
		// Test d'existence du fichier
		else if (not PLRemoteFileService::FileExists(analysisResults->GetInputCoclusteringFileName()))
		{
			bOk = false;
			specInputCoclustering.AddError("File does not exist");
		}
	}

	// Ajout du ou des fichiers de la base d'apprentissage
	database->ExportUsedFileSpecs(&oaTrainDatabaseFileSpecs);
	for (n = 0; n < oaTrainDatabaseFileSpecs.GetSize(); n++)
	{
		specCurrent = cast(FileSpec*, oaTrainDatabaseFileSpecs.GetAt(n));
		specCurrent->SetLabel("train " + specCurrent->GetLabel());
	}
	oaInputFileSpecs.InsertObjectArrayAt(oaInputFileSpecs.GetSize(), &oaTrainDatabaseFileSpecs);

	// Specification des fichiers de sortie concernes
	if (nTaskId == TaskBuildCoclustering)
	{
		specOutputCoclustering.SetLabel("covisualization report");
		specOutputCoclustering.SetFilePathName(analysisResults->GetCoclusteringFileName());
		oaOutputFileSpecs.Add(&specOutputCoclustering);

		// Ajout du fichier d'export JSON si necessaire
		if (analysisResults->GetExportJSON())
		{
			specOutputJSONCoclustering.SetLabel("JSON covisualization report");
			specOutputJSONCoclustering.SetFilePathName(FileService::SetFileSuffix(
			    analysisResults->GetCoclusteringFileName(), CCCoclusteringReport::GetJSONReportSuffix()));
			oaOutputFileSpecs.Add(&specOutputJSONCoclustering);
		}
	}
	else if (nTaskId == TaskExtractClusters)
	{
		specOutputCoclustering.SetLabel("cluster table file");
		specOutputCoclustering.SetFilePathName(analysisResults->GetCoclusteringFileName());
		oaOutputFileSpecs.Add(&specOutputCoclustering);
	}
	else if (nTaskId == TaskPostProcessCoclustering)
	{
		specOutputPostProcessedCoclustering.SetLabel("simplified covisualization report");
		specOutputPostProcessedCoclustering.SetFilePathName(
		    analysisResults->GetPostProcessedCoclusteringFileName());
		oaOutputFileSpecs.Add(&specOutputPostProcessedCoclustering);

		// Ajout du fichier d'export JSON si necessaire
		if (analysisResults->GetExportJSON())
		{
			specOutputJSONCoclustering.SetLabel("simplified JSON covisualization report");
			specOutputJSONCoclustering.SetFilePathName(
			    FileService::SetFileSuffix(analysisResults->GetPostProcessedCoclusteringFileName(),
						       CCCoclusteringReport::GetJSONReportSuffix()));
			oaOutputFileSpecs.Add(&specOutputJSONCoclustering);
		}
	}
	else if (nTaskId == TaskPrepareDeployment)
	{
		specOutputCoclusteringDictionary.SetLabel("coclustering dictionary");
		specOutputCoclusteringDictionary.SetFilePathName(analysisResults->GetCoclusteringDictionaryFileName());
		oaOutputFileSpecs.Add(&specOutputCoclusteringDictionary);
	}

	// Verification que les fichiers en sortie sont des fichiers simple, sans chemin
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
			specOutput->SetFilePathName(
			    BuildOutputFilePathName(specOutput->GetFilePathName(), (nTaskId != TaskBuildCoclustering)));
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
		sOutputPathName = BuildOutputPathName((nTaskId != TaskBuildCoclustering));
		if (sOutputPathName != "" and not FileService::DirExists(sOutputPathName))
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

const ALString CCLearningProblem::BuildOutputFilePathName(const ALString& sFileName, boolean bInputCoclustering) const
{
	ALString sOutputPathName;
	ALString sOutputFilePathName;

	require(sFileName == "" or not FileService::IsPathInFilePath(sFileName));

	// Calcul du repertoire effectif des resultats
	sOutputPathName = BuildOutputPathName(bInputCoclustering);

	// On construit le nom complet du fichier, s'il est specifie
	if (sFileName != "")
	{
		sOutputFilePathName = FileService::BuildFilePathName(
		    sOutputPathName, analysisResults->GetResultFilesPrefix() + sFileName);
	}
	return sOutputFilePathName;
}

const ALString CCLearningProblem::BuildOutputPathName(boolean bInputCoclustering) const
{
	ALString sResultPathName;
	ALString sBasePathName;
	ALString sOutputPathName;

	// Recherche du chemin de base
	if (bInputCoclustering)
	{
		sBasePathName = FileService::GetPathName(analysisResults->GetInputCoclusteringFileName());
		sResultPathName = analysisResults->GetResultFilesDirectory();
	}
	else
	{
		sBasePathName = FileService::GetPathName(GetDatabaseName());
		sResultPathName = analysisResults->GetResultFilesDirectory();
	}

	// Si le repertoire des resultats n'est pas specifie, on utilise celui de la base d'apprentissage
	if (sResultPathName == "")
		sOutputPathName = sBasePathName;
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
		sOutputPathName = FileService::BuildFilePathName(sBasePathName, sResultPathName);
	return sOutputPathName;
}

const ALString CCLearningProblem::GetClassLabel() const
{
	return GetLearningModuleName();
}

void CCLearningProblem::WriteSymbolClusters(const CCHDGAttribute* symbolCoclusteringAttribute, ostream& ost)
{
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	CCHDGValueSet* hdgValueSet;
	KWDGValue* dgValue;
	CCHDGValue* hdgValue;

	require(symbolCoclusteringAttribute != NULL);
	require(symbolCoclusteringAttribute->GetAttributeType() == KWType::Symbol);

	// Entete
	ost << "Cluster\tValue\tFrequency\tTypicality\n";

	// Parcours des parties
	dgPart = symbolCoclusteringAttribute->GetHeadPart();
	while (dgPart != NULL)
	{
		hdgPart = cast(CCHDGPart*, dgPart);

		// Parcours des valeurs
		hdgValueSet = cast(CCHDGValueSet*, hdgPart->GetValueSet());
		dgValue = hdgValueSet->GetHeadValue();
		while (dgValue != NULL)
		{
			hdgValue = cast(CCHDGValue*, dgValue);

			// Caracteristiques des valeurs
			ost << hdgPart->GetUserLabel() << "\t" << hdgValue->GetValue() << "\t"
			    << hdgValue->GetValueFrequency() << "\t" << hdgValue->GetTypicality() << "\n";

			// Valeur suivante
			hdgValueSet->GetNextValue(dgValue);
		}

		// Partie suivante
		symbolCoclusteringAttribute->GetNextPart(dgPart);
	}
}

void CCLearningProblem::WriteContinuousClusters(const CCHDGAttribute* continuousCoclusteringAttribute, ostream& ost)
{
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;

	require(continuousCoclusteringAttribute != NULL);
	require(continuousCoclusteringAttribute->GetAttributeType() == KWType::Continuous);

	// Entete
	ost << "Cluster\tLower bound\tUpper bound\n";

	// Parcours des parties
	dgPart = continuousCoclusteringAttribute->GetHeadPart();
	while (dgPart != NULL)
	{
		hdgPart = cast(CCHDGPart*, dgPart);

		// Caracteristiques du cluster
		ost << hdgPart->GetUserLabel() << "\t";
		if (hdgPart->GetInterval()->GetLowerBound() > KWContinuous::GetMinValue())
			ost << KWContinuous::ContinuousToString(hdgPart->GetInterval()->GetLowerBound());
		ost << "\t";
		if (hdgPart->GetInterval()->GetUpperBound() < KWContinuous::GetMaxValue())
			ost << KWContinuous::ContinuousToString(hdgPart->GetInterval()->GetUpperBound());
		ost << "\n";

		// Partie suivante
		continuousCoclusteringAttribute->GetNextPart(dgPart);
	}
}