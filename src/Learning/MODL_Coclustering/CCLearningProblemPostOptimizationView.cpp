// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemPostOptimizationView.h"

CCLearningProblemPostOptimizationView::CCLearningProblemPostOptimizationView()
{
	CCPostProcessingSpecView* postProcessingSpecView;
	CCAnalysisResultsView* analysisResultsView;
	UICard* postOptimizationSpecView;
	int i;

	// Libelles
	SetIdentifier("CCLearningProblemPostProcessing");
	SetLabel("Coclustering post-optimization");

	// Creation des sous fiches (creation generique pour les vues sur bases de donnees)
	postProcessingSpecView = new CCPostProcessingSpecView;
	analysisResultsView = new CCAnalysisResultsView;

	// Parametrage de la fiche de pretraitement, en ne rendant visible que les clusters
	for (i = 0; i < postProcessingSpecView->GetFieldNumber(); i++)
		postProcessingSpecView->GetFieldAtIndex(i)->SetVisible(false);
	postProcessingSpecView->GetFieldAt("FrequencyAttribute")->SetVisible(true);
	postProcessingSpecView->GetFieldAt("PostProcessedAttributes")->SetVisible(true);
	cast(CCPostProcessedAttributeArrayView*, postProcessingSpecView->GetFieldAt("PostProcessedAttributes"))
	    ->GetFieldAt("MaxPartNumber")
	    ->SetVisible(false);

	// Creation d'une sous-fiche "en dur" pour le nom de la variable de coclustering dont il faut extraire les
	// cluster Ce n'est pas la peine de creer une structure pour memoriser ce seul parametre
	postOptimizationSpecView = new UICard;
	postOptimizationSpecView->AddBooleanField("PreOptimize", "Pre-optimize (fast value move)", true);
	postOptimizationSpecView->AddBooleanField("Optimize", "Optimize (merge clusters)", true);
	postOptimizationSpecView->AddBooleanField("PostOptimize", "Post-optimize (deep value move)", true);

	// Ajout des sous-fiches
	AddCardField("PostProcessingSpec", "Input clusters", postProcessingSpecView);
	AddCardField("PostOptimizationSpec", "Post-optimization parameters", postOptimizationSpecView);
	AddCardField("AnalysisResults", "Coclustering results", analysisResultsView);
	analysisResultsView->SetResultFieldsVisible(false);
	analysisResultsView->GetFieldAt("PostProcessedCoclusteringFileName")->SetVisible(true);
	analysisResultsView->GetFieldAt("PostProcessedCoclusteringFileName")
	    ->SetLabel("Post-optimized coclustering report");
	analysisResultsView->GetFieldAt("PostProcessedCoclusteringFileName")
	    ->SetHelpText("Name of the post-optimized coclustering report.");

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Redefinition de l'action de chargement du coclustering en entree, pour ne lire que la definition des clusters
	GetActionAt("SelectInputCoclustering")
	    ->SetActionMethod((ActionMethod)(&CCLearningProblemPostOptimizationView::SelectInputCoclustering));

	// Ajout d'actions sous formes de boutons
	AddAction("PostOptimize", "Post-optimize coclustering",
		  (ActionMethod)(&CCLearningProblemPostOptimizationView::PostOptimize));
	GetActionAt("PostOptimize")->SetStyle("Button");

	// Info-bulles
	postOptimizationSpecView->GetFieldAt("PreOptimize")
	    ->SetHelpText("Pre-optimize the clusters by a fast move of values between clusters.");
	postOptimizationSpecView->GetFieldAt("Optimize")->SetHelpText("Optimize the clusters by merging them.");
	postOptimizationSpecView->GetFieldAt("PostOptimize")
	    ->SetHelpText("Post-optimize the clusters by a deep move of values between clusters.");
	GetActionAt("PostOptimize")
	    ->SetHelpText(
		"Post-optimize the input coclustering."
		"\n The clusters are extracted from the input coclustering and the structure is filled from the input "
		"database."
		"\n It is then post-optimized according to the post-optimization parameters."
		"\n"
		"\n Prototype implemente rapidement dans le cadre du post-doc de Marirus Bartcus,"
		"\n disponible dans le menu Tools de Khiops coclustering:"
		"\n   . sous le nom \"Post-optimize coclustering (expert mode)...\""
		"\n"
		"\n L'objectif est de permettre d'initialiser une solution de coclustering a partir"
		"\n d'une definitions de clusters existants, en offrant des fonctionnalites de:"
		"\n   . pre-optimisation rapide par deplacement de valeurs"
		"\n   . optimisation par fusions de clusters"
		"\n   . post-optimisation intense par deplacement de valeurs"
		"\n Chaque traitement peut etre effectue independamment."
		"\n Les clusters sont specifie au moyen d'un fichier au format des fichier de coclustering .khc,"
		"\n ne contenant que:"
		"\n   . la ligne de version de Khiops"
		"\n   . la section d'entete \"Dimensions\""
		"\n   . les sections \"Composition\" par variable categorielle"
		"\n     . les clusters doivent etre definis sur des ligne successives"
		"\n     . les valeurs par cluster doivent etre complete, et leur Frequency correcte"
		"\n     . les typicalites doivent juste etre entre 0 et 1 (OK si toutes a 1)");

	// Short cuts
	GetActionAt("SelectInputCoclustering")->SetShortCut('S');
	GetActionAt("PostOptimize")->SetShortCut('P');
}

CCLearningProblemPostOptimizationView::~CCLearningProblemPostOptimizationView() {}

//////////////////////////////////////////////////////////////////////////

void CCLearningProblemPostOptimizationView::SelectInputCoclustering()
{
	UIFileChooserCard openCard;
	ALString sCoclusteringReportFileName;
	CCCoclusteringReport coclusteringReport;
	CCHierarchicalDataGrid coclusteringDataGrid;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	CCPostProcessedAttribute* postProcessedAttribute;
	boolean bOk;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Ouverture du FileChooser
	sCoclusteringReportFileName =
	    openCard.ChooseFile("Select input coclustering", "Open", "FileChooser", "Coclustering\nkhc\nkhcj\njson",
				"InputCoclusteringFileName", "Input coclustering file",
				GetLearningProblem()->GetAnalysisResults()->GetInputCoclusteringFileName());

	// Parametrage des specifications de coclustering a partir du rapport de coclustering
	if (sCoclusteringReportFileName != "")
	{
		GetLearningProblem()->GetAnalysisResults()->SetInputCoclusteringFileName(sCoclusteringReportFileName);

		// Lecture de l'entete du rapport de coclustering
		bOk = ReadReportClusters(sCoclusteringReportFileName, &coclusteringDataGrid);

		// Parametrage des champs de la vue
		GetLearningProblem()->GetPostProcessingSpec()->SetFrequencyAttribute("");
		GetLearningProblem()->GetPostProcessingSpec()->GetPostProcessedAttributes()->DeleteAll();
		if (bOk)
		{
			// Variable de frequence
			GetLearningProblem()->GetPostProcessingSpec()->SetFrequencyAttribute(
			    coclusteringDataGrid.GetFrequencyAttributeName());

			// Information sur les attributs de coclustering
			for (nAttribute = 0; nAttribute < coclusteringDataGrid.GetAttributeNumber(); nAttribute++)
			{
				dgAttribute = coclusteringDataGrid.GetAttributeAt(nAttribute);

				// Ajout d'une caracteristique d'attribut de coclustering
				postProcessedAttribute = new CCPostProcessedAttribute;
				postProcessedAttribute->SetName(dgAttribute->GetAttributeName());
				postProcessedAttribute->SetType(KWType::ToString(dgAttribute->GetAttributeType()));
				postProcessedAttribute->SetPartNumber(dgAttribute->GetPartNumber());
				GetLearningProblem()->GetPostProcessingSpec()->GetPostProcessedAttributes()->Add(
				    postProcessedAttribute);
			}
		}
	}
}

void CCLearningProblemPostOptimizationView::PostOptimize()
{
	boolean bOk = true;
	KWClass* kwcClass;
	KWLearningSpec learningSpec;
	int nAttribute;
	KWAttribute* kwAttribute;
	KWAttributeName* attributeName;
	CCAnalysisSpec* analysisSpec;
	CCCoclusteringPostOptimizer coclusteringPostOptimizer;
	Timer timer;
	CCCoclusteringReport coclusteringReport;
	ALString sCoclusteringReportFileName;
	ALString sPostOptimizedCoclusteringReportFileName;
	CCHierarchicalDataGrid clustersSpecDataGrid;
	CCCoclusteringReport postOptimizedcoclusteringReport;
	boolean bWriteOk;
	ALString sTmp;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Arret si fichiers non corrects
	if (not GetLearningProblem()->CheckResultFileNames(CCLearningProblem::TaskBuildCoclustering) or
	    not GetLearningProblem()->CheckResultFileNames(CCLearningProblem::TaskPostProcessCoclustering))
		return;

	// Recherche du nom du fichier de coclustering
	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Post-optimize coclustering " + sCoclusteringReportFileName);
	TaskProgression::SetDisplayedLevelNumber(1);
	TaskProgression::Start();

	///////////////////////////////////////////////////////
	// Parametrage des specification du coclustering

	// Initialisations
	kwcClass = NULL;

	// Demarage du timer
	timer.Start();

	// Recherche de la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetLearningProblem()->GetClassName());
	check(kwcClass);

	// Parametrage des attributs de la base a lire
	analysisSpec = GetLearningProblem()->GetAnalysisSpec();
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
	learningSpec.SetDatabase(GetLearningProblem()->GetDatabase());
	learningSpec.SetClass(kwcClass);

	// Parametrage du coclustering
	coclusteringPostOptimizer.SetLearningSpec(&learningSpec);
	coclusteringPostOptimizer.SetAttributeNumber(analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize());
	for (nAttribute = 0; nAttribute < analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetSize(); nAttribute++)
	{
		attributeName =
		    cast(KWAttributeName*, analysisSpec->GetCoclusteringSpec()->GetAttributes()->GetAt(nAttribute));
		coclusteringPostOptimizer.SetAttributeNameAt(nAttribute, attributeName->GetName());
	}
	coclusteringPostOptimizer.SetFrequencyAttribute(analysisSpec->GetCoclusteringSpec()->GetFrequencyAttribute());

	// Lecture des clusters existants
	sCoclusteringReportFileName = GetLearningProblem()->GetAnalysisResults()->GetInputCoclusteringFileName();
	if (bOk)
		bOk = ReadReportClusters(sCoclusteringReportFileName, &clustersSpecDataGrid);

	// Parametrage du nom du rapport Khiphren
	sPostOptimizedCoclusteringReportFileName = GetLearningProblem()->BuildOutputFilePathName(
	    GetLearningProblem()->GetAnalysisResults()->GetPostProcessedCoclusteringFileName(), true);
	coclusteringPostOptimizer.SetReportFileName(sPostOptimizedCoclusteringReportFileName);
	coclusteringPostOptimizer.SetExportJSON(GetLearningProblem()->GetAnalysisResults()->GetExportJSON());

	// Post-optimization
	if (bOk)
	{
		// Parametrage de l'optimiseur
		coclusteringPostOptimizer.SetClusterDataGrid(&clustersSpecDataGrid);
		coclusteringPostOptimizer.SetPreOptimize(
		    cast(UICard*, GetFieldAt("PostOptimizationSpec"))->GetBooleanValueAt("PreOptimize"));
		coclusteringPostOptimizer.SetOptimize(
		    cast(UICard*, GetFieldAt("PostOptimizationSpec"))->GetBooleanValueAt("Optimize"));
		coclusteringPostOptimizer.SetPostOptimize(
		    cast(UICard*, GetFieldAt("PostOptimizationSpec"))->GetBooleanValueAt("PostOptimize"));

		// Optimisation
		coclusteringPostOptimizer.ComputeCoclustering();
		bOk = coclusteringPostOptimizer.IsCoclusteringComputed();
	}

	// Message si pas de coclustering informatif trouve en depit du temp imparti
	if (not coclusteringPostOptimizer.IsCoclusteringInformative() and
	    not TaskProgression::IsInterruptionRequested())
		AddSimpleMessage("No informative coclustering found in data");

	// Ecriture du rapport Khiphren, meme si pas de coclustering informatif
	if (sPostOptimizedCoclusteringReportFileName != "")
	{
		AddSimpleMessage("Write coclustering report " + sPostOptimizedCoclusteringReportFileName);
		bWriteOk = coclusteringReport.WriteReport(sPostOptimizedCoclusteringReportFileName,
							  coclusteringPostOptimizer.GetCoclusteringDataGrid());

		// Sauvegarde au format JSON si necessaire
		if (bWriteOk and GetLearningProblem()->GetAnalysisResults()->GetExportJSON())
		{
			coclusteringReport.WriteJSONReport(
			    FileService::SetFileSuffix(sPostOptimizedCoclusteringReportFileName,
						       CCCoclusteringReport::GetJSONReportSuffix()),
			    coclusteringPostOptimizer.GetCoclusteringDataGrid());
		}

		// Destruction de la derniere sauvegarde de fichier temporaire
		if (bWriteOk)
			coclusteringPostOptimizer.RemoveLastSavedReportFile();

		// Warning si moins d'attributs dans le coclustering que d'attributs specifiees
		if (coclusteringPostOptimizer.GetCoclusteringDataGrid()->GetAttributeNumber() <
		    coclusteringPostOptimizer.GetAttributeNumber())
			AddWarning(
			    sTmp + "The built coclustering only exploits " +
			    IntToString(coclusteringPostOptimizer.GetCoclusteringDataGrid()->GetAttributeNumber()) +
			    " out of the " + IntToString(coclusteringPostOptimizer.GetAttributeNumber()) +
			    " input variables");
	}

	// Nettoyage
	kwcClass->SetAllAttributesLoaded(true);
	KWClassDomain::GetCurrentDomain()->Compile();

	// Temps final
	timer.Stop();

	// Messsage de fin
	if (not TaskProgression::IsInterruptionRequested())
		AddSimpleMessage(sTmp +
				 "Coclustering post-optimization time: " + SecondsToString(timer.GetElapsedTime()));
	else
		AddSimpleMessage(sTmp + "Coclustering post-optimization interrupted after " +
				 SecondsToString(timer.GetElapsedTime()));

	// Fin du suivi de la tache
	TaskProgression::Stop();

	ensure(not TaskProgression::IsStarted());
}

boolean CCLearningProblemPostOptimizationView::ReadReportClusters(const ALString& sFileName,
								  CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	CCCoclusteringReport coclusteringReport;

	require(coclusteringDataGrid != NULL);

	// Ouverture du rapport pour initialiser l'API de lecture
	bOk = coclusteringReport.OpenInputCoclusteringReportFile(sFileName);
	if (bOk)
	{
		// Gestion des erreurs
		Global::ActivateErrorFlowControl();

		// Reinitialisation prealable des informations
		coclusteringDataGrid->DeleteAll();

		// Premiere ligne: #Khiops...
		if (bOk)
			bOk = coclusteringReport.ReadVersion(coclusteringDataGrid);

		// Section Dimensions
		if (bOk)
			bOk = coclusteringReport.ReadDimensions(coclusteringDataGrid);

		// Section des bornes des intervalles numeriques
		if (bOk)
			bOk = coclusteringReport.ReadBounds(coclusteringDataGrid);

		// Section de specification de la composition des attributs categoriels
		if (bOk)
			bOk = ReadClusterComposition(&coclusteringReport, coclusteringDataGrid);

		// Mise a jour des statistiques de la grille
		if (bOk)
			coclusteringDataGrid->UpdateAllStatistics();

		// Gestion des erreurs
		Global::DesactivateErrorFlowControl();

		// Fermeture du fichier
		coclusteringReport.CloseCoclusteringReportFile();
	}
	return bOk;
}

// Implementation presque identique a CCCoclusteringReport::ReadComposition
// Difference: les parties sont  creee au fur et a mesure
boolean CCLearningProblemPostOptimizationView::ReadClusterComposition(CCCoclusteringReport* coclusteringReport,
								      CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	char* sField;
	ALString sTmp;
	CCHDGAttribute* dgAttribute;
	KWDGPart* dgPart;
	CCHDGValueSet* dgValueSet;
	CCHDGValue* dgValue;
	int nAttribute;
	ALString sAttributeName;
	ALString sPreviousPartName;
	ALString sPartName;
	ALString sValueName;
	int nFrequency;
	double dTypicality;

	require(coclusteringReport != NULL);
	require(coclusteringDataGrid != NULL);

	// Boucle de specification de la composition des attributs categoriels
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut correspondant
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Traitement si attribut categoriel
		if (dgAttribute->GetAttributeType() == KWType::Symbol)
		{
			// Recherche de l'entete de la nouvelle section
			coclusteringReport->SkipLine();
			sField = coclusteringReport->ReadNextField();
			if (strcmp(coclusteringReport->sKeyWordComposition, sField) != 0)
			{
				AddError(sTmp + "Key word " + coclusteringReport->sKeyWordComposition +
					 " expected but not found");
				bOk = false;
			}

			// Verification de la coherence de la section du rapport
			sField = coclusteringReport->ReadNextField();
			if (strcmp(dgAttribute->GetAttributeName(), sField) != 0)
			{
				AddError(sTmp + "Variable " + dgAttribute->GetAttributeName() +
					 " expected but not found after key word " +
					 coclusteringReport->sKeyWordComposition);
				bOk = false;
			}

			// Passage a la ligne suivante, puis saut de la ligne d'entete de la section
			coclusteringReport->SkipLine();
			coclusteringReport->SkipLine();

			// Valeurs de l'attribut
			if (bOk)
			{
				// Extraction des valeurs
				// Elle sont specifiees sequentiellement selon les parties
				sPreviousPartName = "";
				dgPart = NULL;
				while (not coclusteringReport->IsEndOfFile() and bOk)
				{
					// Initialisations
					sPartName = "";
					sValueName = "";
					nFrequency = 0;

					// Nom de partie
					sField = coclusteringReport->ReadNextField();
					sPartName = sField;

					// Nom de valeur
					sField = coclusteringReport->ReadNextField();
					sValueName = sField;

					// Effectif lie a la valeur
					sField = coclusteringReport->ReadNextField();
					nFrequency = StringToInt(sField);

					// Typicalite
					sField = coclusteringReport->ReadNextField();
					dTypicality = KWContinuous::StringToContinuous(sField);

					// Arret si nom de partie vide (fin de section)
					if (sPartName == "")
					{
						// On rajoute prelablement la StarValue au dernier groupe specifie
						if (dgPart != NULL)
						{
							dgValueSet = cast(CCHDGValueSet*, dgPart->GetValueSet());
							dgValueSet->AddValue(Symbol::GetStarValue());
						}
						break;
					}

					// Modification principale de l'implementation par rapport a la methode de
					// reference On change de groupe si necessaire
					if (sPartName != sPreviousPartName)
					{
						if (sPreviousPartName == "")
							dgPart = dgAttribute->GetHeadPart();
						else
							dgAttribute->GetNextPart(dgPart);
						sPreviousPartName = sPartName;

						// Arret si erreur
						if (dgPart == NULL)
						{
							AddError(sTmp + "More parts than expected (part " + sPartName +
								 ") for variable " + dgAttribute->GetAttributeName());
							break;
						}
					}

					//////////////////////////////////////////////////
					// Verification de coherence (non exhaustives)

					// Nom de partie
					if (bOk and sPartName == "")
					{
						AddError(sTmp + "Missing part name for value specification (" +
							 sValueName + ") for variable " +
							 dgAttribute->GetAttributeName());
						break;
					}

					// Existence de la partie
					if (bOk and dgPart == NULL)
					{
						AddError(sTmp + "Missing part (" + sPartName +
							 ") for value specification (" + sValueName +
							 ") for variable " + dgAttribute->GetAttributeName());
						break;
					}

					// Effectif
					if (bOk and nFrequency <= 0)
					{
						AddError(sTmp + "Value specification (" + sValueName +
							 ") with wrong frequency for variable " +
							 dgAttribute->GetAttributeName());
						break;
					}

					// Typicalite
					if (bOk and not(0 <= dTypicality and dTypicality <= 1))
					{
						// Tolerance pour les typicalite negatives
						if (dTypicality < 0)
							AddWarning(sTmp + "Value specification (" + sValueName +
								   ") with typicality less than 0 for variable " +
								   dgAttribute->GetAttributeName());
						// Erreur pour les typicalite superieures a 1
						else
						{
							AddError(sTmp + "Value specification (" + sValueName +
								 ") with typicality greater than 1 for variable " +
								 dgAttribute->GetAttributeName());
							break;
						}
					}

					// Message global
					if (not bOk)
					{
						AddError(sTmp + "Invalid value specification (" + sValueName +
							 ") for variable " + dgAttribute->GetAttributeName());
						break;
					}

					// Memorisation des caracteristiques de la valeur dans sa partie
					if (bOk)
					{
						dgValueSet = cast(CCHDGValueSet*, dgPart->GetValueSet());
						dgValue = cast(CCHDGValue*, dgValueSet->AddValue((Symbol)sValueName));
						dgValue->SetValueFrequency(nFrequency);
						dgValue->SetTypicality(dTypicality);

						if (coclusteringReport->bReadDebug)
							cout << coclusteringReport->sKeyWordComposition << "\t"
							     << sPartName << "\t" << sValueName << "\t" << nFrequency
							     << "\t" << dTypicality << "\n";
					}

					// Ligne suivante
					coclusteringReport->SkipLine();
				}
			}

			// Arret si erreur
			if (not bOk)
				break;
		}
	}
	return bOk;
}

void CCLearningProblemPostOptimizationView::SetObject(Object* object)
{
	CCLearningProblem* learningProblem;

	require(object != NULL);

	// Acces a l'objet edite
	learningProblem = cast(CCLearningProblem*, object);

	// Parametrage des sous-fiches
	cast(CCPostProcessingSpecView*, GetFieldAt("PostProcessingSpec"))
	    ->SetObject(learningProblem->GetPostProcessingSpec());
	cast(CCAnalysisResultsView*, GetFieldAt("AnalysisResults"))->SetObject(learningProblem->GetAnalysisResults());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

////////////////////////////////////////////////////////////
// Classe CCCoclusteringPostOptimizer

CCCoclusteringPostOptimizer::CCCoclusteringPostOptimizer()
{
	clusterDataGrid = NULL;
	bPreOptimize = false;
	bOptimize = false;
	bPostOptimize = false;
}

CCCoclusteringPostOptimizer::~CCCoclusteringPostOptimizer() {}

void CCCoclusteringPostOptimizer::SetClusterDataGrid(const KWDataGrid* dataGrid)
{
	clusterDataGrid = dataGrid;
}

const KWDataGrid* CCCoclusteringPostOptimizer::GetClusterDataGrid() const
{
	return clusterDataGrid;
}

boolean CCCoclusteringPostOptimizer::GetPreOptimize() const
{
	return bPreOptimize;
}

void CCCoclusteringPostOptimizer::SetPreOptimize(boolean bValue)
{
	bPreOptimize = bValue;
}

boolean CCCoclusteringPostOptimizer::GetOptimize() const
{
	return bOptimize;
}

void CCCoclusteringPostOptimizer::SetOptimize(boolean bValue)
{
	bOptimize = bValue;
}

boolean CCCoclusteringPostOptimizer::GetPostOptimize() const
{
	return bPostOptimize;
}

void CCCoclusteringPostOptimizer::SetPostOptimize(boolean bValue)
{
	bPostOptimize = bValue;
}

void CCCoclusteringPostOptimizer::OptimizeDataGrid(const KWDataGrid* inputInitialDataGrid,
						   KWDataGrid* optimizedDataGrid)
{
	CCCoclusteringOptimizer dataGridOptimizer;
	KWDataGridManager dataGridManager;
	double dBestCost;

	require(clusterDataGrid != NULL);

	// Creation et initialisation de la structure de couts
	coclusteringDataGridCosts = CreateDataGridCost();
	coclusteringDataGridCosts->InitializeDefaultCosts(inputInitialDataGrid);

	// Parametrage des couts de l'optimiseur de grille
	dataGridOptimizer.SetDataGridCosts(coclusteringDataGridCosts);

	// Parametrage pour l'optimisation anytime: avoir acces aux ameliorations a cjhaque etape de l'optimisation
	dataGridOptimizer.SetCoclusteringBuilder(this);

	// Recopie du parametrage d'optimisation des grilles
	dataGridOptimizer.GetParameters()->CopyFrom(GetPreprocessingSpec()->GetDataGridOptimizerParameters());
	dataGridOptimizer.GetParameters()->SetOptimizationTime(RMResourceConstraints::GetOptimizationTime());

	// Parametrage d'un niveau d'optimisation "illimite" si une limite de temps est indiquee
	debug(dataGridOptimizer.GetParameters()->SetOptimizationLevel(0));
	if (dataGridOptimizer.GetParameters()->GetOptimizationTime() > 0)
		dataGridOptimizer.GetParameters()->SetOptimizationLevel(20);

	// Optimisation de la grille
	dBestCost = GreedyOptimize(inputInitialDataGrid, optimizedDataGrid);

	// Sauvegarde de la grille, systematique meme si l'on a un modele moins bon que le modele nul
	if (coclusteringDataGrid == NULL)
		coclusteringDataGrid = new CCHierarchicalDataGrid;
	if (dBestCost >= 0)
	{
		dataGridManager.CopyDataGrid(optimizedDataGrid, coclusteringDataGrid);

		// Calcul de ses infos de hierarchie
		ComputeHierarchicalInfo(inputInitialDataGrid, coclusteringDataGridCosts, coclusteringDataGrid);
	}
}

double CCCoclusteringPostOptimizer::GreedyOptimize(const KWDataGrid* inputInitialDataGrid,
						   KWDataGrid* optimizedDataGrid) const
{
	KWDataGridManager dataGridManager;
	KWDataGridMerger dataGridMerger;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	double dBestCost;
	double dCost;
	Timer timer;
	ALString sTmp;

	require(clusterDataGrid != NULL);

	// Export des clusters existants vers un merger de grille
	timer.Start();
	dataGridManager.SetSourceDataGrid(clusterDataGrid);
	dataGridManager.ExportAttributes(&dataGridMerger);
	dataGridManager.ExportParts(&dataGridMerger);

	// Export de toutes les cellules
	dataGridManager.SetSourceDataGrid(inputInitialDataGrid);
	dataGridManager.ExportCells(&dataGridMerger);

	// Mise a jour des statistiques de la grille
	dataGridMerger.UpdateAllStatistics();
	if (not dataGridMerger.Check())
	{
		AddError("Value frequencies in cluster file should be correctly specified");
		return -1;
	}

	// Memorisation de la solution initiale
	dataGridManager.CopyDataGrid(&dataGridMerger, optimizedDataGrid);

	// Initialisations des couts
	dBestCost = coclusteringDataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	dCost = dBestCost;
	dataGridMerger.SetDataGridCosts(coclusteringDataGridCosts);
	dataGridPostOptimizer.SetDataGridCosts(coclusteringDataGridCosts);
	AddMessage(sTmp + "Initialisation cost: " + DoubleToString(dBestCost));
	AddMessage(sTmp + "Initialisation time: " + DoubleToString(timer.GetElapsedTime()));

	// Pre-optimisation de la grille
	if (bPreOptimize)
	{
		timer.Reset();
		timer.Start();
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(inputInitialDataGrid, &dataGridMerger, false);
		AddMessage(sTmp + "Pre-optimization time: " + DoubleToString(timer.GetElapsedTime()));
	}

	// Optimisation par fusion des groupes
	if (bOptimize)
	{
		timer.Reset();
		timer.Start();
		dCost = dataGridMerger.Merge();
		AddMessage(sTmp + "Optimization time: " + DoubleToString(timer.GetElapsedTime()));
	}

	// Post-optimisation de la grille
	if (bPostOptimize)
	{
		timer.Reset();
		timer.Start();
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(inputInitialDataGrid, &dataGridMerger, true);
		AddMessage(sTmp + "Post-optimization time: " + DoubleToString(timer.GetElapsedTime()));
	}

	// Memorisation de la meilleure solution
	AddMessage(sTmp + "Final cost: " + DoubleToString(dCost));
	if (dCost < dBestCost - 1e-5)
	{
		dBestCost = dCost;
		dataGridManager.CopyDataGrid(&dataGridMerger, optimizedDataGrid);
	}

	// Retour du cout
	ensure(fabs(coclusteringDataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < 1e-5);
	return dBestCost;
}
