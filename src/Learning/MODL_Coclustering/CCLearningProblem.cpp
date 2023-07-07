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
	// CH IV Begin
	// CH IV Refactoring: est-ce vraiment utile? a supprimer?
	//   a verifier et tester
	// Synchronisation du dictionnaire de la base
	database->SetClassName(classManagement->GetClassName());
	// CH IV End
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
	// CH IV Begin
	const ALString sDefaultOwnerAttributeName = "Variables";
	ALString sOwnerAttributeName;
	ObjectArray* oaAttributesAndAxes;
	KWAttributeAxisName* attributeAxisName;
	boolean bOk = true;
	// CH IV End
	ALString sReportFileName;
	Timer timer;
	boolean bWriteOk;
	// CH IV Begin
	KWClassDomain* currentDomain = NULL;
	KWClassDomain* constructedDomain = NULL;
	KWClass* currentClass;
	KWAttribute* identifierAttribute;
	// CH IV End

	require(CheckClass());
	require(CheckDatabaseName());
	require(GetDatabase()->CheckSelectionValue(GetDatabase()->GetSelectionValue()));
	require(CheckCoclusteringAttributeNames());
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

	// CH IV Begin
	// Cas d'un coclustering variable * variable : mode non expert ou type de coclustering demande a l'interface
	oaAttributesAndAxes = NULL;
	if (not GetLearningCoclusteringIVExpertMode() or not analysisSpec->GetVarPartCoclustering())
	{
		for (nAttribute = 0; nAttribute < analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize();
		     nAttribute++)
		{
			attributeName = cast(KWAttributeName*,
					     analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetAt(nAttribute));
			kwAttribute = kwcClass->LookupAttribute(attributeName->GetName());
			check(kwAttribute);
			kwAttribute->SetLoaded(true);
		}
		if (analysisSpec->GetCoclusteringSpec()->GetFrequencyAttribute() != "")
		{
			kwAttribute =
			    kwcClass->LookupAttribute(analysisSpec->GetCoclusteringSpec()->GetFrequencyAttribute());
			check(kwAttribute);
			kwAttribute->SetLoaded(true);
		}
		KWClassDomain::GetCurrentDomain()->Compile();

		// Parametrage des specifications d'apprentissage
		learningSpec.SetShortDescription(GetAnalysisResults()->GetShortDescription());
		learningSpec.SetDatabase(database);
		learningSpec.SetClass(kwcClass);

		// Parametrage du coclustering
		coclusteringBuilder.SetVarPartCoclustering(false);
		coclusteringBuilder.SetLearningSpec(&learningSpec);
		coclusteringBuilder.SetAttributeNumber(analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize());
		for (nAttribute = 0; nAttribute < analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize();
		     nAttribute++)
		{
			attributeName = cast(KWAttributeName*,
					     analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetAt(nAttribute));
			coclusteringBuilder.SetAttributeNameAt(nAttribute, attributeName->GetName());
		}
		coclusteringBuilder.SetFrequencyAttribute(analysisSpec->GetCoclusteringSpec()->GetFrequencyAttribute());
	}
	// Sinon : cas d'un coclustering instances * variables avec au moins un axe de parties de variable
	else
	{
		assert(oaAttributesAndAxes == NULL);

		// Extraction du tableau des noms de variable et de leur axe
		oaAttributesAndAxes = analysisSpec->GetVarPartCoclusteringSpec()->GetAttributesAndAxes();
		assert(oaAttributesAndAxes->GetSize() == 0);

		// Creation de l'axe avec la variable d'identifiant des instances
		// Cas ou aucun attribut identifiant n'a ete specifie
		if (analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttribute() == "")
		{
			// Creation d'une nouvelle classe enrichie d'une variable d'identifiant

			// Preparation du domaine pour la nouvelle classe
			constructedDomain = kwcClass->GetDomain()->CloneFromClass(kwcClass);
			constructedDomain->Compile();

			// Remplacement du domaine courant par le domaine de selection
			currentDomain = KWClassDomain::GetCurrentDomain();
			KWClassDomain::SetCurrentDomain(constructedDomain);

			// Remplacement de la classe courante par la classe enrichie
			currentClass = kwcClass;
			kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
			check(kwcClass);

			// Insertion d'un attribut identifiant
			identifierAttribute = InsertIdentifierAttribute(kwcClass);

			// Ajout du nom de l'attribut dans les specifications
			analysisSpec->GetVarPartCoclusteringSpec()->SetIdentifierAttribute(
			    identifierAttribute->GetName());
		}

		// Creation de la ligne de tableau avec nom de l'attribut identifiant et de son axe
		attributeAxisName = new KWAttributeAxisName;
		attributeAxisName->SetAttributeName(
		    analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttribute());
		attributeAxisName->SetOwnerAttributeName("");

		// Ajout de la ligne au tableau
		oaAttributesAndAxes->Add(attributeAxisName);

		// Recherche d'un nom d'attribut de griiole de type VarPart qui n'entre pas en collision avec un des
		// attribut du dictionnaire
		sOwnerAttributeName = kwcClass->BuildAttributeName(sDefaultOwnerAttributeName);

		// Creation de l'axe Variables avec l'ensemble des variables Used sauf l'attribut identifiant
		kwAttribute = kwcClass->GetHeadAttribute();
		while (kwAttribute != NULL)
		{
			// Ajout si attribut utilise et du bon type
			if (kwAttribute->GetUsed() and KWType::IsSimple(kwAttribute->GetType()) and
			    kwAttribute->GetName() !=
				analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttribute())
			{
				check(kwAttribute);
				kwAttribute->SetLoaded(true);

				// Ajout de l'attribut et de son axe
				attributeAxisName = new KWAttributeAxisName;
				attributeAxisName->SetAttributeName(kwAttribute->GetName());
				attributeAxisName->SetOwnerAttributeName(sOwnerAttributeName);

				// Ajout au tableau
				oaAttributesAndAxes->Add(attributeAxisName);
			}
			kwcClass->GetNextAttribute(kwAttribute);
		}

		for (nAttribute = 0;
		     nAttribute < analysisSpec->GetVarPartCoclusteringSpec()->GetAttributesAndAxes()->GetSize();
		     nAttribute++)
		{
			attributeAxisName =
			    cast(KWAttributeAxisName*,
				 analysisSpec->GetVarPartCoclusteringSpec()->GetAttributesAndAxes()->GetAt(nAttribute));
			kwAttribute = kwcClass->LookupAttribute(attributeAxisName->GetAttributeName());
			check(kwAttribute);
			kwAttribute->SetLoaded(true);
		}

		if (analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttribute() != "")
		{
			kwAttribute = kwcClass->LookupAttribute(
			    analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttribute());
			check(kwAttribute);
			kwAttribute->SetLoaded(true);
			if (kwAttribute->GetType() != KWType::Symbol)
			{
				AddError("Identifier attribute must be categoriel");
				bOk = false;
			}
		}

		KWClassDomain::GetCurrentDomain()->Compile();

		// Parametrage des specifications d'apprentissage
		learningSpec.SetShortDescription(GetAnalysisResults()->GetShortDescription());
		learningSpec.SetDatabase(database);
		learningSpec.SetClass(kwcClass);

		// Parametrage du coclustering VarPart
		coclusteringBuilder.SetVarPartCoclustering(true);
		coclusteringBuilder.SetLearningSpec(&learningSpec);
		coclusteringBuilder.SetAttributeNumber(
		    analysisSpec->GetVarPartCoclusteringSpec()->GetAttributesAndAxes()->GetSize());
		for (nAttribute = 0;
		     nAttribute < analysisSpec->GetVarPartCoclusteringSpec()->GetAttributesAndAxes()->GetSize();
		     nAttribute++)
		{
			attributeAxisName =
			    cast(KWAttributeAxisName*,
				 analysisSpec->GetVarPartCoclusteringSpec()->GetAttributesAndAxes()->GetAt(nAttribute));
			coclusteringBuilder.SetAttributeNameAt(nAttribute, attributeAxisName->GetAttributeName());

			// On rapatrie le nom de l'axe
			coclusteringBuilder.SetAxisForAttributeNameAt(nAttribute,
								      attributeAxisName->GetOwnerAttributeName());
		}
		coclusteringBuilder.SetIdentifierAttribute(
		    analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttribute());
		coclusteringBuilder.GetPreprocessingSpec()->GetDataGridOptimizerParameters()->CopyFrom(
		    analysisSpec->GetVarPartCoclusteringSpec()->GetOptimizationParameters());

		// On extrait les noms distincts des axes et des attributs internes dans chaque attribut de grille de
		// type VarPart
		bOk = bOk and coclusteringBuilder.ExtractAndCheckAxisNames();
	}
	// CH IV End

	// Parametrage du nom du rapport de coclustering
	sReportFileName = BuildOutputFilePathName(TaskBuildCoclustering);
	coclusteringBuilder.SetReportFileName(sReportFileName);
	coclusteringBuilder.SetExportAsKhc(GetAnalysisResults()->GetExportAsKhc());

	// Calcul du coclustering
	if (not TaskProgression::IsInterruptionRequested() and bOk)
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

		// CH IV Begin
		// Warning si moins d'attributs dans le coclustering que d'attributs ou d'axes specifiees
		// Cas du coclustering variable * variable
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
			    coclusteringBuilder.GetAxisNumber())
				AddWarning(
				    sTmp + "The built coclustering only exploits " +
				    IntToString(coclusteringBuilder.GetCoclusteringDataGrid()->GetAttributeNumber()) +
				    " out of the " + IntToString(coclusteringBuilder.GetAxisNumber()) +
				    " axis variables");
		}
		// CH IV End
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
		analysisSpec->GetVarPartCoclusteringSpec()->SetIdentifierAttribute("");
	}
	if (analysisSpec->GetVarPartCoclustering())
	{
		assert(oaAttributesAndAxes != NULL);
		oaAttributesAndAxes->DeleteAll();
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
	sCoclusteringDictionaryFileName = BuildOutputFilePathName(TaskPrepareDeployment);

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
	// CH IV Begin
	ALString sIdentifierAttributeName;
	// CH IV End
	ALString sTmp;

	require(CheckClass());

	// Recherche de la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

	// CH IV Begin
	// Cas d'un coclustering standard : variable * variable
	if (not analysisSpec->GetVarPartCoclustering())
	{
		// Il doit y avoir au moins deux variables dans un co-clustering
		if (analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize() < 2)
		{
			AddError("Less than two coclustering variables are specified");
			if (GetLearningCoclusteringIVExpertMode())
				AddWarning("You may be interested in Instances x Variables coclustering rather than "
					   "Variables coclustering");
			bOk = false;
		}
		// Il y a une limite au nombre de variables
		else if (analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize() >
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
			for (nVar = 0; nVar < analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize(); nVar++)
			{
				attributeName = cast(KWAttributeName*,
						     analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetAt(nVar));

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
							 "Incorrect type for coclustering variable " +
							     attributeName->GetName());
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
				Global::AddError("", "",
						 "Incorrect type for coclustering frequency variable " +
						     sFrequencyAttributeName);
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
		sIdentifierAttributeName = analysisSpec->GetVarPartCoclusteringSpec()->GetIdentifierAttribute();
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
						 "Incorrect type for coclustering identifier variable " +
						     sIdentifierAttributeName);
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

// CH IV Begin
KWAttribute* CCLearningProblem::InsertIdentifierAttribute(KWClass* kwcClass)
{
	KWDRAsSymbol* identifierRule;
	KWDRDiff* diffRule;
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

	// Cas de presence d'un header dans le fichier de donnees
	// CH IV Refactoring: supprimer ce code un peu "too much"
	// CH IV Refactoring: quel est le lien avec IsMultiTableTechnology? (uniquement pour le cast)?
	// CH IV Refactoring: la decrementation de l'index en cas de ligne d'entete est peut-etre "too much"?
	//     de toute facon, avec le sampling, on a pas tous les numeros consecutifs
	// CH IV Refactoring: par contre, en multi-table, on pourrait utiliser comme cle la concatenation
	//    des champs cles du dictionnaire? (mais pb si cle multiple...): NON
	if ((database->IsMultiTableTechnology() and cast(KWMTDatabaseTextFile*, database)->GetHeaderLineUsed()) or
	    (not database->IsMultiTableTechnology() and cast(KWSTDatabaseTextFile*, database)->GetHeaderLineUsed()))
	{
		// Creation de la regle qui decremente l'index de 1 du fait de la presence d'un header
		diffRule = new KWDRDiff;
		diffRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		diffRule->GetFirstOperand()->SetDerivationRule(indexRule);
		diffRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		diffRule->GetSecondOperand()->SetContinuousConstant(1.0);

		identifierRule->GetFirstOperand()->SetDerivationRule(diffRule);
	}
	// Sinon
	else
		identifierRule->GetFirstOperand()->SetDerivationRule(indexRule);

	// Nettoyage des meta-data des attributs de la classe
	kwcClass->RemoveAllAttributesMetaDataKey(sIdentifierMetaDataKey);

	// Creation de l'attribut et de son nom
	attribute = new KWAttribute;
	attribute->SetName(kwcClass->BuildAttributeName("Identifier"));
	attribute->GetMetaData()->SetNoValueAt("IdentifierCC");

	// Initialisation
	attribute->SetDerivationRule(identifierRule);
	attribute->CompleteTypeInfo(kwcClass);

	// Insertion dans la classe enrichie
	kwcClass->InsertAttribute(attribute);

	return attribute;
}
// CH IV End

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
