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

	// Parametrage de la base associee au resultats
	analysisResults->SetTrainDatabase(database);
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
	require(database->GetClassName() == classManagement->GetClassName());
	return classManagement;
}

KWDatabase* CCLearningProblem::GetDatabase()
{
	require(database->GetClassName() == classManagement->GetClassName());
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

void CCLearningProblem::UpdateClassNameFromClassManagement()
{
	database->SetClassName(classManagement->GetClassName());
}

void CCLearningProblem::UpdateClassNameFromTrainDatabase()
{
	classManagement->SetClassName(database->GetClassName());
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
	const ALString sDefaultOwnerAttributeName = "Variables";
	ALString sOwnerAttributeName;
	KWAttribute* identifierAttribute;
	ALString sReportFileName;
	Timer timer;
	boolean bWriteOk;
	KWClassDomain* currentDomain = NULL;
	KWClassDomain* constructedDomain = NULL;

	require(CheckClass());
	require(CheckDatabaseName());
	require(GetDatabase()->CheckSelectionValue(GetDatabase()->GetSelectionValue()));
	require(CheckCoclusteringSpecifications());
	require(CheckResultFileNames(TaskBuildCoclustering));
	require(not TaskProgression::IsStarted());

	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Train coclustering " + GetClassName());
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

	// Cas d'un coclustering variable * variable : mode non expert ou type de coclustering demande a l'interface
	if (not GetLearningCoclusteringIVExpertMode() or not analysisSpec->GetVarPartCoclustering())
	{
		// Preparation des attribut a charger dans la classe
		for (nAttribute = 0; nAttribute < analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetSize();
		     nAttribute++)
		{
			attributeName =
			    cast(KWAttributeName*,
				 analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetAt(nAttribute));
			kwAttribute = kwcClass->LookupAttribute(attributeName->GetName());
			check(kwAttribute);
			kwAttribute->SetLoaded(true);
		}
		if (analysisSpec->GetCoclusteringSpec()->GetFrequencyAttributeName() != "")
		{
			kwAttribute =
			    kwcClass->LookupAttribute(analysisSpec->GetCoclusteringSpec()->GetFrequencyAttributeName());
			check(kwAttribute);
			kwAttribute->SetLoaded(true);
		}

		// Compilation du domaine
		KWClassDomain::GetCurrentDomain()->Compile();

		// Parametrage des specifications d'apprentissage
		learningSpec.SetShortDescription(GetAnalysisResults()->GetShortDescription());
		learningSpec.SetDatabase(database);
		learningSpec.SetClass(kwcClass);

		// Parametrage du coclustering
		coclusteringBuilder.SetVarPartCoclustering(false);
		coclusteringBuilder.SetLearningSpec(&learningSpec);
		coclusteringBuilder.SetAttributeNumber(
		    analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetSize());
		for (nAttribute = 0; nAttribute < analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetSize();
		     nAttribute++)
		{
			attributeName =
			    cast(KWAttributeName*,
				 analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetAt(nAttribute));
			coclusteringBuilder.SetAttributeNameAt(nAttribute, attributeName->GetName());
		}
		coclusteringBuilder.SetFrequencyAttributeName(
		    analysisSpec->GetCoclusteringSpec()->GetFrequencyAttributeName());
	}
	// Sinon : cas d'un coclustering instances * variables avec attribut de type de parties de variable
	else
	{
		// Creation de la variable d'identifiant des instances si necessaire
		// Dans ce cas, creation d'une nouvelle classe enrichie d'une variable d'identifiant
		coclusteringBuilder.SetIdentifierAttributeName(
		    analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttributeName());
		if (analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttributeName() == "")
		{
			// Preparation du domaine pour la nouvelle classe
			constructedDomain = kwcClass->GetDomain()->CloneFromClass(kwcClass);
			constructedDomain->Compile();

			// Remplacement du domaine courant par le domaine de selection
			currentDomain = KWClassDomain::GetCurrentDomain();
			KWClassDomain::SetCurrentDomain(constructedDomain);

			// Remplacement de la classe courante par la classe enrichie
			kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
			check(kwcClass);

			// Insertion d'un attribut identifiant
			identifierAttribute = InsertIdentifierAttribute(kwcClass);

			// Ajout du nom de l'attribut dans les specifications
			analysisSpec->GetVarPartCoclusteringSpec()->SetIdentifierAttributeName(
			    identifierAttribute->GetName());
			coclusteringBuilder.SetIdentifierAttributeName(
			    analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttributeName());
		}
		// Recherche de l'attribut dans la classe sinon
		else
			identifierAttribute = kwcClass->LookupAttribute(
			    analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttributeName());
		assert(identifierAttribute != NULL);
		assert(identifierAttribute->GetUsed());

		// On passe l'attribut identifiant en Loaded
		identifierAttribute->SetLoaded(true);

		// Recherche d'un nom d'attribut de grille de type VarPart qui n'entre pas en collision avec un des
		// attribut du dictionnaire
		sOwnerAttributeName = kwcClass->BuildAttributeName(sDefaultOwnerAttributeName);
		coclusteringBuilder.SetVarPartAttributeName(sOwnerAttributeName);

		// Creation des variables internes avec l'ensemble des variables Used sauf l'attribut identifiant
		coclusteringBuilder.GetInnerAttributesNames()->SetSize(0);
		kwAttribute = kwcClass->GetHeadAttribute();
		while (kwAttribute != NULL)
		{
			// Ajout si attribut utilise et du bon type
			if (kwAttribute->GetUsed() and KWType::IsSimple(kwAttribute->GetType()) and
			    kwAttribute->GetName() !=
				analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttributeName())
			{
				check(kwAttribute);
				kwAttribute->SetLoaded(true);

				// Ajout de l'attribut interne
				coclusteringBuilder.GetInnerAttributesNames()->Add(kwAttribute->GetName());
			}
			kwcClass->GetNextAttribute(kwAttribute);
		}

		// Compilation du domaine
		KWClassDomain::GetCurrentDomain()->Compile();

		// Parametrage des specifications d'apprentissage
		learningSpec.SetShortDescription(GetAnalysisResults()->GetShortDescription());
		learningSpec.SetDatabase(database);
		learningSpec.SetClass(kwcClass);

		// Parametrage du coclustering VarPart
		coclusteringBuilder.SetVarPartCoclustering(true);
		coclusteringBuilder.SetLearningSpec(&learningSpec);
	}

	// Parametre d'optimisation avances
	// Attention, on synchronise les parametre d'optimisation des grilles definies dans les
	// preprocessing spec accessibles depuis tout LearningServive
	// En effet, avec le coclustering, ces specification sont accessibles (en mode expert)
	// uniquement depuis l'onglet AnalysisSpec
	coclusteringBuilder.GetPreprocessingSpec()->GetDataGridOptimizerParameters()->CopyFrom(
	    analysisSpec->GetOptimizationParameters());

	// Verification de la validite
	assert(coclusteringBuilder.Check());
	assert(coclusteringBuilder.CheckSpecifications());

	// Parametrage du nom du rapport de coclustering
	sReportFileName = BuildOutputFilePathName(TaskBuildCoclustering);
	coclusteringBuilder.SetReportFileName(sReportFileName);
	coclusteringBuilder.SetExportAsKhc(GetAnalysisResults()->GetExportAsKhc());

	// Calcul du coclustering
	if (not TaskProgression::IsInterruptionRequested())
		coclusteringBuilder.ComputeCoclustering();

	// Message si pas de coclustering informatif trouve en depit du temp imparti
	if (coclusteringBuilder.IsCoclusteringComputed() and not coclusteringBuilder.IsCoclusteringInformative())
		AddSimpleMessage("No informative coclustering found in data");

	// Ecriture du rapport de coclustering
	if (coclusteringBuilder.IsCoclusteringInformative() and sReportFileName != "")
	{
		AddSimpleMessage("Write coclustering report " + sReportFileName);
		bWriteOk =
		    coclusteringReport.WriteJSONReport(sReportFileName, coclusteringBuilder.GetCoclusteringDataGrid());

		// Sauvegarde au format khc si necessaire
		if (bWriteOk and GetAnalysisResults()->GetExportAsKhc())
			coclusteringReport.WriteReport(BuildKhcOutputFilePathName(TaskBuildCoclustering),
						       coclusteringBuilder.GetCoclusteringDataGrid());

		// Destruction de la derniere sauvegarde de fichier temporaire
		if (bWriteOk)
			coclusteringBuilder.RemoveLastSavedReportFile();

		// Warning si moins d'attributs dans le coclustering que d'attributs ou de deimsnsions specifiees
		// Cas du coclustering de variables
		if (not GetLearningCoclusteringIVExpertMode() or not analysisSpec->GetVarPartCoclustering())
		{
			if (coclusteringBuilder.GetCoclusteringDataGrid()->GetAttributeNumber() <
			    coclusteringBuilder.GetAttributeNumber())
				AddWarning(
				    sTmp + "The built coclustering only exploits " +
				    IntToString(coclusteringBuilder.GetCoclusteringDataGrid()->GetAttributeNumber()) +
				    " out of the " + IntToString(coclusteringBuilder.GetAttributeNumber()) +
				    " input variables");
		}
		// Sinon cas du coclustering instances * variables
		else
		{
			if (coclusteringBuilder.GetCoclusteringDataGrid()->GetAttributeNumber() <
			    coclusteringBuilder.GetVarPartCoclusteringAttributeNumber())
				AddWarning(
				    sTmp + "The built instances x variables coclustering only exploits " +
				    IntToString(coclusteringBuilder.GetCoclusteringDataGrid()->GetAttributeNumber()) +
				    " out of the " +
				    IntToString(coclusteringBuilder.GetVarPartCoclusteringAttributeNumber()) +
				    " dimensions");
		}
	}

	// Cas d'un domaine construit
	if (constructedDomain != NULL)
	{
		// Restauration de l'etat initial
		assert(currentDomain != NULL);
		KWClassDomain::SetCurrentDomain(currentDomain);
		delete constructedDomain;
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
		check(kwcClass);
		analysisSpec->GetVarPartCoclusteringSpec()->SetIdentifierAttributeName("");
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
	sPostProcessedCoclusteringReportFileName = BuildOutputFilePathName(TaskPostProcessCoclustering);

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
		bOk = postProcessedcoclusteringReport.WriteJSONReport(sPostProcessedCoclusteringReportFileName,
								      &postProcessedCoclusteringDataGrid);

		// Sauvegarde au format khc si necessaire
		if (bOk and GetAnalysisResults()->GetExportAsKhc())
			postProcessedcoclusteringReport.WriteReport(
			    BuildKhcOutputFilePathName(TaskPostProcessCoclustering),
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
	sClusterTableFileName = BuildOutputFilePathName(TaskExtractClusters);

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
				WriteGroupableClusters(coclusteringAttribute, fstClusterTableFile);
			else if (coclusteringAttribute->GetAttributeType() == KWType::Continuous)
				WriteContinuousClusters(coclusteringAttribute, fstClusterTableFile);
			else if (coclusteringAttribute->GetAttributeType() == KWType::VarPart)
			{
				// Cas d'une variable de type VarPart : ajout d'un descriptif des parties de variable
				// des attributs internes
				WriteGroupableClusters(coclusteringAttribute, fstClusterTableFile);
				WriteVarPartsInnerAttributes(coclusteringAttribute);
			}

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
	sCoclusteringDictionaryFileName = BuildOutputFilePathName(TaskPrepareDeployment);

	// Recherche du nom du fichier de coclustering
	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Prepare deployment " + sCoclusteringReportFileName);
	TaskProgression::SetDisplayedLevelNumber(1);
	TaskProgression::Start();

	// Lecture du rapport de coclustering
	if (bOk)
		bOk = coclusteringReport.ReadGenericReport(sCoclusteringReportFileName, &coclusteringDataGrid);

	// CH IV Begin
	// Cas d'un rapport issu d'un coclustering instances * variables : fonctionnalite non implementee
	if (coclusteringDataGrid.IsVarPartDataGrid())
	{
		bOk = false;
		AddWarning("Deployment preparation is not yet implemented for instances * variables coclustering");
	}
	// CH IV End

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

boolean CCLearningProblem::CheckCoclusteringSpecifications() const
{
	boolean bOk = true;
	KWClass* kwcClass;
	int nVar;
	KWAttributeName* attributeName;
	KWAttribute* attribute;
	ObjectDictionary odCoclusteringAttributes;
	ALString sFrequencyAttributeName;
	ALString sIdentifierAttributeName;
	int nInternalAttributeNumber;
	ALString sTmp;

	require(CheckClass());

	// Recherche de la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

	// Cas d'un coclustering standard : coclustering de variables
	if (not analysisSpec->GetVarPartCoclustering())
	{
		// Il doit y avoir au moins deux variables dans un co-clustering
		if (analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetSize() < 2)
		{
			AddError("Less than two coclustering variables are specified");
			bOk = false;
		}
		// Il y a une limite au nombre de variables
		else if (analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetSize() >
			 CCCoclusteringSpec::GetMaxCoclusteringAttributeNumber())
		{
			AddError(sTmp + "Too many coclustering variables (> " +
				 IntToString(CCCoclusteringSpec::GetMaxCoclusteringAttributeNumber()) +
				 ") are specified");
			bOk = false;
		}
		// Sinon, verification des variables
		else
		{
			for (nVar = 0; nVar < analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetSize();
			     nVar++)
			{
				attributeName =
				    cast(KWAttributeName*,
					 analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetAt(nVar));

				// Recherche de la variable correspondante dans le dictionnaire
				attribute = kwcClass->LookupAttribute(attributeName->GetName());

				// La variable doit avoir un nom
				if (attributeName->GetName() == "")
				{
					AddError(sTmp + "Missing name for coclustering variable " +
						 IntToString(nVar + 1));
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
					Global::AddError("", "",
							 "Coclustering variable " + attributeName->GetName() +
							     " must be of Categorical or Numerical type");
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
		sFrequencyAttributeName = analysisSpec->GetCoclusteringSpec()->GetFrequencyAttributeName();
		if (bOk and sFrequencyAttributeName != "")
		{
			assert(odCoclusteringAttributes.GetCount() ==
			       analysisSpec->GetCoclusteringSpec()->GetAttributeNames()->GetSize());

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
				Global::AddError("", "",
						 "Coclustering frequency variable " + sFrequencyAttributeName +
						     " must be of Numerical type");
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
	}
	// Sinon : cas d'un coclustering instances * variables
	else
	{
		// Verification de la variable d'identifiant
		sIdentifierAttributeName = analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttributeName();
		if (bOk and sIdentifierAttributeName != "")
		{
			// Recherche de la variable correspondante dans le dictionnaire
			attribute = kwcClass->LookupAttribute(sIdentifierAttributeName);

			// La variable doit etre presente dans le dictionnaire
			if (attribute == NULL)
			{
				bOk = false;
				Global::AddError("", "",
						 "Coclustering identifier variable " + sIdentifierAttributeName +
						     " unknown in dictionary " + GetClassName());
			}
			// De type Symbol
			else if (attribute->GetType() != KWType::Symbol)
			{
				bOk = false;
				Global::AddError("", "",
						 "Coclustering identifier variable " + sIdentifierAttributeName +
						     " must be of Categorical type");
			}
			// Et utilise
			else if (not attribute->GetUsed())
			{
				bOk = false;
				Global::AddError("", "",
						 "Coclustering identifier variable " + sIdentifierAttributeName +
						     " unused in dictionary " + GetClassName());
			}
		}

		// Verification des attributs internes pour les parties de variables
		if (bOk)
		{
			// Comptage du nombre d'attribut internes
			nInternalAttributeNumber = kwcClass->GetUsedAttributeNumberForType(KWType::Continuous) +
						   kwcClass->GetUsedAttributeNumberForType(KWType::Symbol);
			if (sIdentifierAttributeName != "")
				nInternalAttributeNumber--;

			// Erreur s'il n'y en a pas
			if (nInternalAttributeNumber == 0)
			{
				bOk = false;
				AddError(
				    "No numerical or categorical variables used are available in dictionary " +
				    GetClassName() +
				    " for Instances x Variables coclustering, apart from the identifier variable.");
			}
		}
	}
	// CH IV End
	return bOk;
}

boolean CCLearningProblem::CheckResultFileNames(int nTaskId) const
{
	boolean bOk = true;
	FileSpec specInputClass;
	FileSpec specInputTrainDatabase;
	FileSpec specInputCoclustering;
	FileSpec specOutputMainFile;
	FileSpec specOutputKhcFile;
	FileSpec specOutputCoclustering;
	FileSpec specOutputClusters;
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

	// Test si fichier en sortie specifie
	specOutputMainFile.SetLabel(GetOutputFileLabel(nTaskId));
	if (GetSpecifiedOutputFileName(nTaskId) == "")
	{
		bOk = false;
		specOutputMainFile.AddError("Missing file name");
	}

	// Ajout du fichier en sortie
	if (bOk)
	{
		specOutputMainFile.SetFilePathName(BuildOutputFilePathName(nTaskId));
		oaOutputFileSpecs.Add(&specOutputMainFile);
	}

	// Ajout du fichier d'export khc si necessaire
	if (bOk and (nTaskId == TaskBuildCoclustering or nTaskId == TaskPostProcessCoclustering) and
	    analysisResults->GetExportAsKhc())
	{
		specOutputKhcFile.SetLabel("khc " + GetOutputFileLabel(nTaskId));
		specOutputKhcFile.SetFilePathName(BuildKhcOutputFilePathName(nTaskId));
		oaOutputFileSpecs.Add(&specOutputKhcFile);
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
		bOk = CheckResultDirectory(nTaskId);
	}

	// Nettoyage
	oaTrainDatabaseFileSpecs.DeleteAll();
	oaTestDatabaseFileSpecs.DeleteAll();
	return bOk;
}

boolean CCLearningProblem::CheckResultDirectory(int nTaskId) const
{
	// Verification du repertoire
	return GetResultFilePathBuilder(nTaskId)->CheckResultDirectory("");
}

const ALString CCLearningProblem::BuildOutputFilePathName(int nTaskId) const
{
	// Construction du nom du fichier en sortie
	return GetResultFilePathBuilder(nTaskId)->BuildResultFilePathName();
}

const ALString CCLearningProblem::BuildKhcOutputFilePathName(int nTaskId) const
{
	// Construction du nom du fichier en sortie
	return GetResultFilePathBuilder(nTaskId)->BuildOtherResultFilePathName("khc");
}

const ALString CCLearningProblem::GetClassLabel() const
{
	return GetLearningModuleName();
}

KWAttribute* CCLearningProblem::InsertIdentifierAttribute(KWClass* kwcClass)
{
	KWDRAsSymbol* identifierRule;
	KWDRIndex* indexRule;
	KWAttribute* attribute;
	const ALString sIdentifierMetaDataKey = "IdentifierCC";
	KWMTDatabaseTextFile refMTDatabaseTextFile;
	KWSTDatabaseTextFile refSTDatabaseTextFile;

	require(kwcClass->Check());

	// Creation de la regle de l'identifiant
	identifierRule = new KWDRAsSymbol;
	identifierRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);

	// Creation de la regle qui donne l'index de la ligne
	indexRule = new KWDRIndex;
	identifierRule->GetFirstOperand()->SetDerivationRule(indexRule);

	// Nettoyage des meta-data des attributs de la classe
	kwcClass->RemoveAllAttributesMetaDataKey(sIdentifierMetaDataKey);

	// Creation de l'attribut et de son nom
	attribute = new KWAttribute;
	attribute->SetName(kwcClass->BuildAttributeName("Identifier"));
	attribute->GetMetaData()->SetNoValueAt(sIdentifierMetaDataKey);

	// Initialisation
	attribute->SetDerivationRule(identifierRule);
	attribute->CompleteTypeInfo(kwcClass);

	// Insertion dans la classe enrichie
	kwcClass->InsertAttribute(attribute);

	return attribute;
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

void CCLearningProblem::WriteGroupableClusters(const CCHDGAttribute* groupableCoclusteringAttribute, ostream& ost)
{
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	KWDGValueSet* dgValueSet;
	KWDGValue* dgValue;

	require(groupableCoclusteringAttribute != NULL);
	require(KWType::IsCoclusteringGroupableType(groupableCoclusteringAttribute->GetAttributeType()));

	// Entete
	ost << "Cluster\tValue\tFrequency\tTypicality\n";

	// Parcours des parties
	dgPart = groupableCoclusteringAttribute->GetHeadPart();
	while (dgPart != NULL)
	{
		hdgPart = cast(CCHDGPart*, dgPart);

		// Parcours des valeurs
		dgValueSet = hdgPart->GetValueSet();
		dgValue = dgValueSet->GetHeadValue();
		while (dgValue != NULL)
		{
			// Caracteristiques des valeurs
			ost << hdgPart->GetUserLabel() << "\t" << dgValue->GetObjectLabel() << "\t"
			    << dgValue->GetValueFrequency() << "\t" << dgValue->GetTypicality() << "\n";

			// Valeur suivante
			dgValueSet->GetNextValue(dgValue);
		}

		// Partie suivante
		groupableCoclusteringAttribute->GetNextPart(dgPart);
	}
}

void CCLearningProblem::WriteVarPartsInnerAttributes(const CCHDGAttribute* varPartCoclusteringAttribute)
{
	ALString sIntervalsTableFileName;
	ALString sGroupsTableFileName;
	ALString sIntervalsTableSuffix = "intervals.txt";
	ALString sModalitiesTableSuffix = "groups.txt";
	ALString sLocalIntervalsFileName;
	ALString sLocalGroupsFileName;
	fstream fstIntervalsTableFile;
	fstream fstGroupsTableFile;
	KWDGInnerAttributes* innerAttributes;
	KWDGAttribute* innerAttribute;
	int nIndex;
	boolean bIntervalsOk = true;
	boolean bGroupsOk = true;
	boolean bFirstContinuousAttribute = true;
	boolean bFirstSymbolAttribute = true;

	// Creation des noms des fichiers dedies a la description des VarPart selon leur type continu ou categoriel
	sIntervalsTableFileName =
	    GetResultFilePathBuilder(TaskExtractClusters)->BuildOtherResultFilePathName(sIntervalsTableSuffix);
	sGroupsTableFileName =
	    GetResultFilePathBuilder(TaskExtractClusters)->BuildOtherResultFilePathName(sModalitiesTableSuffix);

	// Preparation de la copie sur HDFS si necessaire
	bIntervalsOk = PLRemoteFileService::BuildOutputWorkingFile(sIntervalsTableFileName, sLocalIntervalsFileName);
	// Ouverture du fichier de rapport en ecriture
	if (bIntervalsOk)
		bIntervalsOk = FileService::OpenOutputFile(sLocalIntervalsFileName, fstIntervalsTableFile);

	bGroupsOk = PLRemoteFileService::BuildOutputWorkingFile(sGroupsTableFileName, sLocalGroupsFileName);
	// Ouverture du fichier de rapport en ecriture
	if (bGroupsOk)
		bGroupsOk = FileService::OpenOutputFile(sLocalGroupsFileName, fstGroupsTableFile);

	// Extraction des attributs internes
	innerAttributes = varPartCoclusteringAttribute->GetDataGrid()->GetInnerAttributes();

	if (bIntervalsOk and bGroupsOk)
	{
		// Boucle sur les attributs
		for (nIndex = 0; nIndex < innerAttributes->GetInnerAttributeNumber(); nIndex++)
		{
			innerAttribute = innerAttributes->GetInnerAttributeAt(nIndex);
			// Cas d'un attribut de type Continuous
			if (innerAttribute->GetAttributeType() == KWType::Continuous)
			{
				// Premier attribut Continuous
				if (bFirstContinuousAttribute)
				{
					// Entete
					fstIntervalsTableFile << "VarPart\tLower bound\tUpper bound\n";
					WriteContinuousInnerAttribute(innerAttribute, fstIntervalsTableFile);
					bFirstContinuousAttribute = false;
				}
				// Sinon
				else
					WriteContinuousInnerAttribute(innerAttribute, fstIntervalsTableFile);
			}
			// Sinon, cas d'un attribut de type categoriel
			if (innerAttribute->GetAttributeType() == KWType::Symbol)
			{
				// Premier attribut Symbol
				if (bFirstSymbolAttribute)
				{
					// Entete
					fstGroupsTableFile << "VarPart\tModality\n";
					WriteSymbolInnerAttribute(innerAttribute, fstGroupsTableFile);
					bFirstSymbolAttribute = false;
				}
				else
					WriteSymbolInnerAttribute(innerAttribute, fstGroupsTableFile);
			}
		}
		// Ecriture du rapport
		bIntervalsOk = FileService::CloseOutputFile(sLocalIntervalsFileName, fstIntervalsTableFile);

		// Destruction du fichier si erreur
		if (not bIntervalsOk)
			FileService::RemoveFile(sLocalIntervalsFileName);

		bGroupsOk = FileService::CloseOutputFile(sLocalGroupsFileName, fstGroupsTableFile);

		// Destruction du fichier si erreur
		if (not bGroupsOk)
			FileService::RemoveFile(sLocalGroupsFileName);
	}
	if (bIntervalsOk)
	{
		// Copie vers HDFS
		PLRemoteFileService::CleanOutputWorkingFile(sIntervalsTableFileName, sLocalIntervalsFileName);
	}
	if (bGroupsOk)
	{
		// Copie vers HDFS
		PLRemoteFileService::CleanOutputWorkingFile(sGroupsTableFileName, sLocalGroupsFileName);
	}
}

void CCLearningProblem::WriteContinuousInnerAttribute(KWDGAttribute* continuousCoclusteringAttribute, ostream& ost)
{
	KWDGPart* dgPart;

	require(continuousCoclusteringAttribute != NULL);
	require(continuousCoclusteringAttribute->GetAttributeType() == KWType::Continuous);

	// Parcours des parties
	dgPart = continuousCoclusteringAttribute->GetHeadPart();
	while (dgPart != NULL)
	{
		// Caracteristiques de la partie de variable
		ost << continuousCoclusteringAttribute->GetAttributeName() + " " + dgPart->GetObjectLabel() << "\t";
		if (dgPart->GetInterval()->GetLowerBound() > KWContinuous::GetMinValue())
			ost << KWContinuous::ContinuousToString(dgPart->GetInterval()->GetLowerBound());
		else
			ost << "-inf";
		ost << "\t";
		if (dgPart->GetInterval()->GetUpperBound() < KWContinuous::GetMaxValue())
			ost << KWContinuous::ContinuousToString(dgPart->GetInterval()->GetUpperBound());
		else
			ost << "+inf";
		ost << "\n";

		// Partie suivante
		continuousCoclusteringAttribute->GetNextPart(dgPart);
	}
}

void CCLearningProblem::WriteSymbolInnerAttribute(KWDGAttribute* symbolCoclusteringAttribute, ostream& ost)
{
	KWDGPart* dgPart;
	KWDGValueSet* dgValueSet;
	KWDGValue* dgValue;

	require(symbolCoclusteringAttribute != NULL);
	require(symbolCoclusteringAttribute->GetAttributeType() == KWType::Symbol);

	// Parcours des parties
	dgPart = symbolCoclusteringAttribute->GetHeadPart();
	while (dgPart != NULL)
	{
		// Parcours des valeurs
		dgValueSet = dgPart->GetValueSet();
		dgValue = dgValueSet->GetHeadValue();
		while (dgValue != NULL)
		{
			// Caracteristiques des valeurs
			ost << symbolCoclusteringAttribute->GetAttributeName() + " " + dgPart->GetObjectLabel() << "\t"
			    << dgValue->GetSymbolValue() << "\n";

			// Valeur suivante
			dgValueSet->GetNextValue(dgValue);
		}

		// Partie suivante
		symbolCoclusteringAttribute->GetNextPart(dgPart);
	}
}

const ALString CCLearningProblem::GetSpecifiedOutputFileName(int nTaskId) const
{
	require(nTaskId == TaskBuildCoclustering or nTaskId == TaskExtractClusters or
		nTaskId == TaskPostProcessCoclustering or nTaskId == TaskPrepareDeployment);

	if (nTaskId == TaskBuildCoclustering)
		return analysisResults->GetCoclusteringFileName();
	else if (nTaskId == TaskExtractClusters)
		return analysisResults->GetClusterFileName();
	else if (nTaskId == TaskPostProcessCoclustering)
		return analysisResults->GetPostProcessedCoclusteringFileName();
	else if (nTaskId == TaskPrepareDeployment)
		return analysisResults->GetCoclusteringDictionaryFileName();
	else
		return "";
}

const ALString CCLearningProblem::GetOutputFileLabel(int nTaskId) const
{
	require(nTaskId == TaskBuildCoclustering or nTaskId == TaskExtractClusters or
		nTaskId == TaskPostProcessCoclustering or nTaskId == TaskPrepareDeployment);

	if (nTaskId == TaskBuildCoclustering)
		return "coclustering report";
	else if (nTaskId == TaskExtractClusters)
		return "cluster table file";
	else if (nTaskId == TaskPostProcessCoclustering)
		return "simplified coclustering report";
	else if (nTaskId == TaskPrepareDeployment)
		return "coclustering dictionary";
	else
		return "";
}

const KWResultFilePathBuilder* CCLearningProblem::GetResultFilePathBuilder(int nTaskId) const
{
	require(nTaskId == TaskBuildCoclustering or nTaskId == TaskExtractClusters or
		nTaskId == TaskPostProcessCoclustering or nTaskId == TaskPrepareDeployment);

	// Initialisation du createur de chemin de fichier
	if (nTaskId == TaskBuildCoclustering)
	{
		if (database != NULL)
			resultFilePathBuilder.SetInputFilePathName(database->GetDatabaseName());
	}
	else
		resultFilePathBuilder.SetInputFilePathName(analysisResults->GetInputCoclusteringFileName());
	resultFilePathBuilder.SetOutputFilePathName(GetSpecifiedOutputFileName(nTaskId));

	// Parametrage du suffixe
	if (nTaskId == TaskBuildCoclustering)
		resultFilePathBuilder.SetFileSuffix("khcj");
	else if (nTaskId == TaskExtractClusters)
		resultFilePathBuilder.SetFileSuffix("txt");
	else if (nTaskId == TaskPostProcessCoclustering)
		resultFilePathBuilder.SetFileSuffix("khcj");
	else if (nTaskId == TaskPrepareDeployment)
		resultFilePathBuilder.SetFileSuffix("kdic");
	return &resultFilePathBuilder;
}
