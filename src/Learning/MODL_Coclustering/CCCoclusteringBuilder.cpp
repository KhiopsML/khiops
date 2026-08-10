// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCCoclusteringBuilder.h"

////////////////////////////////////////////////////////////////////
// Classe CCCoclusteringBuilder

CCCoclusteringBuilder::CCCoclusteringBuilder()
{
	coclusteringDataGrid = NULL;
	initialDataGrid = NULL;
	coclusteringDataGridCosts = NULL;
	nAnyTimeOptimizationIndex = 0;
	dAnyTimeBestCost = 0;
	bVarPartCoclustering = false;
}

CCCoclusteringBuilder::~CCCoclusteringBuilder()
{
	CleanCoclusteringResults();
}

boolean CCCoclusteringBuilder::GetVarPartCoclustering() const
{
	return bVarPartCoclustering;
}

void CCCoclusteringBuilder::SetVarPartCoclustering(boolean bValue)
{
	bVarPartCoclustering = bValue;
}

const ALString& CCCoclusteringBuilder::GetFrequencyAttributeName() const
{
	return sFrequencyAttributeName;
}

void CCCoclusteringBuilder::SetFrequencyAttributeName(const ALString& sValue)
{
	sFrequencyAttributeName = sValue;
}

const ALString& CCCoclusteringBuilder::GetIdentifierAttributeName() const
{
	return sIdentifierAttributeName;
}

void CCCoclusteringBuilder::SetIdentifierAttributeName(const ALString& sValue)
{
	sIdentifierAttributeName = sValue;
}

const ALString& CCCoclusteringBuilder::GetVarPartAttributeName() const
{
	return sVarPartAttributeName;
}

void CCCoclusteringBuilder::SetVarPartAttributeName(const ALString& sValue)
{
	sVarPartAttributeName = sValue;
}

int CCCoclusteringBuilder::GetVarPartCoclusteringAttributeNumber() const
{
	int nNumber;
	require(GetVarPartCoclustering());

	nNumber = 0;
	if (GetIdentifierAttributeName() != "")
		nNumber++;
	if (GetVarPartAttributeName() != "")
		nNumber++;
	return nNumber;
}

StringVector* CCCoclusteringBuilder::GetInnerAttributesNames()
{
	return &svInnerAttributeNames;
}

boolean CCCoclusteringBuilder::CheckSpecifications() const
{
	if (not GetVarPartCoclustering())
		return CheckStandardSpecifications();
	else
		return CheckVarPartSpecifications();
}

boolean CCCoclusteringBuilder::CheckStandardSpecifications() const
{
	boolean bOk;
	KWAttribute* attribute;
	ObjectDictionary odCoclusteringAttributes;
	ObjectDictionary odInnerAttributes;
	ALString sAttributeName;
	int i;

	require(not GetVarPartCoclustering());
	require(GetIdentifierAttributeName() == "");
	require(GetVarPartAttributeName() == "");
	require(svInnerAttributeNames.GetSize() == 0);

	// Verification standard
	bOk = KWAttributeSubsetStats::CheckSpecifications();

	// Verification du cas non supervise
	if (bOk and GetTargetAttributeName() != "")
	{
		bOk = false;
		AddError("Coclustering not available in the supervised case");
	}

	// Verification du nombre d'attribut dans le cas du coclustering de variables
	if (bOk and GetAttributeNumber() < 2)
	{
		bOk = false;
		AddError("Coclustering available for at least two variables");
	}

	// Verification du type des attributs
	for (i = 0; i < GetAttributeNumber(); i++)
	{
		sAttributeName = GetAttributeNameAt(i);

		// Recherche de l'attribut
		attribute = GetClass()->LookupAttribute(sAttributeName);

		// Test du type de l'attribut
		if (attribute != NULL and not KWType::IsSimple(attribute->GetType()))
		{
			bOk = false;
			AddError("Coclustering variable " + sAttributeName +
				 " must be of Categorical or Numerical type");
		}

		// Test si variable deja utilisee
		if (bOk and odCoclusteringAttributes.Lookup(sAttributeName) != NULL)
		{
			bOk = false;
			AddError("Coclustering variable " + sAttributeName + " used twice");
		}

		// Memorisation de la variable
		if (bOk)
			odCoclusteringAttributes.SetAt(sAttributeName, attribute);

		// Arret si erreurs
		if (not bOk)
			break;
	}

	// Verification de l'attribut d'effectif
	if (bOk and GetFrequencyAttributeName() != "")
	{
		// Recherche de la variable correspondante dans le dictionnaire
		attribute = GetClass()->LookupAttribute(sFrequencyAttributeName);

		// La variable doit etre presente dans le dictionnaire
		if (attribute == NULL)
		{
			bOk = false;
			Global::AddError("", "",
					 "Coclustering frequency variable " + sFrequencyAttributeName +
					     " unknown in dictionary " + GetClass()->GetName());
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
					     " unused in dictionary " + GetClass()->GetName());
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

boolean CCCoclusteringBuilder::CheckVarPartSpecifications() const
{
	boolean bOk;
	KWAttribute* attribute;
	ObjectDictionary odCoclusteringAttributes;
	ObjectDictionary odInnerAttributes;
	ALString sAttributeName;
	int i;

	require(GetVarPartCoclustering());
	require(GetAttributeNumber() == 0);
	require(GetFrequencyAttributeName() == "");

	// Verification standard
	bOk = KWAttributeSubsetStats::CheckSpecifications();

	// Verification du cas non supervise
	if (bOk and GetTargetAttributeName() != "")
	{
		bOk = false;
		AddError("Coclustering not available in the supervised case");
	}

	// La variable identifiant doit etre specifiee
	if (bOk and GetIdentifierAttributeName() == "")
	{
		bOk = false;
		Global::AddError("", "", "Coclustering identifier variable not specified");
	}

	// La variable identifiant doit etre de type categoriel
	if (bOk)
	{
		attribute = GetClass()->LookupAttribute(sIdentifierAttributeName);
		check(attribute);
		if (attribute->GetType() != KWType::Symbol)
		{
			Global::AddError("", "",
					 "Coclustering identifier variable " + sIdentifierAttributeName +
					     " must be of categorical type");
			bOk = false;
		}

		// Test d'utilisation de l'attribut
		if (bOk and not attribute->GetUsed())
		{
			bOk = false;
			AddError("Identifier variable " + sIdentifierAttributeName + " is not used");
		}

		// Test de chargement en memoire de l'attribut
		if (bOk and not attribute->GetLoaded())
		{
			bOk = false;
			AddError("Identifier variable " + sIdentifierAttributeName + " is not loaded");
		}
	}

	// La variable VarPart doit etre specifiee
	if (bOk and GetVarPartAttributeName() == "")
	{
		bOk = false;
		Global::AddError("", "", "Coclustering VarPart variable not specified");
	}

	// La variable VarPart ne doit pas faire partie du dictionnaire
	if (bOk)
	{
		if (GetClass()->LookupAttribute(GetVarPartAttributeName()) != NULL)
		{
			bOk = false;
			Global::AddError("", "",
					 "Coclustering VarPart variable " + sVarPartAttributeName +
					     " must not belong to dictionary " + GetClass()->GetName());
		}
	}

	// Verification des attributs internes
	if (bOk)
	{
		// Il doit y en avoir au moins deux
		if (svInnerAttributeNames.GetSize() == 0)
		{
			bOk = false;
			AddError("No numerical or categorical variables used are available in dictionary " +
				 GetClass()->GetName() +
				 " for Instances x Variables coclustering, beyong the identifier variable, apart from "
				 "the identifier variable.");
		}
		// Elles doivent etre utilisables dans le dictionnaire
		else
		{
			// Parcours des variables internes
			for (i = 0; i < svInnerAttributeNames.GetSize(); i++)
			{
				sAttributeName = svInnerAttributeNames.GetAt(i);

				// Recherche de l'attribut
				attribute = GetClass()->LookupAttribute(sAttributeName);

				// Test d'existence de l'attribut
				if (attribute == NULL)
				{
					bOk = false;
					AddError("Inner variable " + sAttributeName + " does not exist");
				}

				// Test du type de l'attribut
				if (attribute != NULL and not KWType::IsSimple(attribute->GetType()))
				{
					bOk = false;
					AddError("Inner variable " + sAttributeName +
						 " is not of numerical or categorical type");
				}

				// Test d'utilisation de l'attribut
				if (attribute != NULL and not attribute->GetUsed())
				{
					bOk = false;
					AddError("Inner variable " + sAttributeName + " is not used");
				}

				// Test de chargement en memoire de l'attribut
				if (attribute != NULL and not attribute->GetLoaded())
				{
					bOk = false;
					AddError("Inner variable " + sAttributeName + " is not loaded");
				}

				// Test si variable deja utilisee
				if (bOk and odInnerAttributes.Lookup(sAttributeName) != NULL)
				{
					bOk = false;
					AddError("Inner variable " + sAttributeName + " used twice");
				}

				// Memorisation de la variable
				if (bOk)
					odInnerAttributes.SetAt(sAttributeName, attribute);

				// Arret si erreurs
				if (not bOk)
					break;
			}
		}
	}
	return bOk;
}

boolean CCCoclusteringBuilder::ComputeCoclustering()
{
	boolean bOk = true;
	KWTupleTable tupleTable;
	KWDataGrid optimizedDataGrid;
	KWDataGridManager dataGridManager;
	ALString sProfileFileName;
	ALString sTmp;

	require(Check());
	require(CheckSpecifications());
	require(initialDataGrid == NULL);
	require(odDescriptiveStats.GetCount() == 0);

	// Debut du pilotage anytime
	AnyTimeStart();

	// Nettoyage des resultats de coclustering
	CleanCoclusteringResults();

	///////////////////////////////////////////////////////////////////////////////////
	// Calcul d'une grille initiale

	// Craetion d'une grille initial plus plus fine possible a partir de la base de donnees
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		// Cas du coclustering standard, VxV
		if (not GetVarPartCoclustering())
			bOk = CreateStandardInitialDataGrid();
		// Sinon, cas de coclustering VarPart, IxV
		else
			bOk = CreateVarPartInitialDataGrid();
		assert(not bOk or initialDataGrid != NULL);
	}

	// On verifie une derniere fois qu'il n'y a pas eu d'interruption
	if (bOk)
		bOk = not TaskProgression::IsInterruptionRequested();

	// Verification de la memoire necessaire pour optimiser le coclustering
	if (bOk)
		bOk = CheckMemoryForDataGridOptimization(initialDataGrid);

	////////////////////////////////////////////////////////////////////////////////////
	// Optimisation de la grille

	// Lancement du profiler pour l'optimisation des grilles
	// Choix du fichier de trace a parametrer
	if (GetPreprocessingSpec()->GetDataGridOptimizerParameters()->GetOptimizationProfiling())
	{
		// Nom du fichier de profiling, avec le nombre d'individus et de variables
		sProfileFileName = "DataGridOptimizationProfiling_";
		sProfileFileName += GetDatabase()->GetClassName();
		if (initialDataGrid->IsVarPartDataGrid())
		{
			sProfileFileName += "_I";
			sProfileFileName += IntToString(initialDataGrid->GetAttributeAt(0)->GetInitialValueNumber());
			sProfileFileName += "_V";
			sProfileFileName +=
			    IntToString(initialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber());
		}
		else
		{
			sProfileFileName += "_F";
			sProfileFileName += IntToString(initialDataGrid->GetGridFrequency());
			sProfileFileName += "_D";
			sProfileFileName += IntToString(initialDataGrid->GetAttributeNumber());
		}
		sProfileFileName += ".txt";

		// On utilise le repertoire du rapport d'apprentissage pour stocker le profilage de l'optimisation de la grille
		sProfileFileName =
		    FileService::BuildFilePathName(FileService::GetPathName(GetReportFileName()), sProfileFileName);

		// Lancement du profiling
		KWDataGridOptimizer::GetProfiler()->SetTrace(true);
		KWDataGridOptimizer::GetProfiler()->SetTraceTime(false);
		KWDataGridOptimizer::GetProfiler()->Start(sProfileFileName);
	}

	// Optimisation de la grille
	if (bOk and not TaskProgression::IsInterruptionRequested())
		OptimizeDataGrid(initialDataGrid, &optimizedDataGrid);

	// Arret du profiler
	if (GetPreprocessingSpec()->GetDataGridOptimizerParameters()->GetOptimizationProfiling())
		KWDataGridOptimizer::GetProfiler()->Stop();

	// Message si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
	{
		bOk = false;
		AddWarning("Coclustering optimization interrupted by user");
	}

	// La solution est sauvegardee periodiquement grace au mode anytime
	// Nettoyage si aucune solution n'a encore ete trouvee
	if (coclusteringDataGrid == NULL)
		CleanCoclusteringResults();

	// Nettoyage de la grille initiale (si non deja nettoyee), dont on a plus besoin desormais
	if (initialDataGrid != NULL)
	{
		delete initialDataGrid;
		initialDataGrid = NULL;
	}
	odDescriptiveStats.DeleteAll();

	// Fin du pilotage anytime
	AnyTimeStop();
	bIsStatsComputed = bOk;
	ensure(Check());
	return bIsStatsComputed;
}

boolean CCCoclusteringBuilder::IsCoclusteringComputed() const
{
	return IsStatsComputed();
}

boolean CCCoclusteringBuilder::IsCoclusteringInformative() const
{
	boolean bInformative;
	bInformative = IsCoclusteringComputed() and coclusteringDataGrid != NULL and
		       (coclusteringDataGrid->GetInformativeAttributeNumber() >= 2);
	return bInformative;
}

KWDataGridCosts* CCCoclusteringBuilder::CreateDataGridCost() const
{
	KWDataGridCosts* dataGridCosts;

	require(Check());
	require(CheckSpecifications());

	// Cas d'un coclustering standard
	if (not GetVarPartCoclustering())
		dataGridCosts = new KWDataGridClusteringCosts;
	// Cas d'un coclustering VarPart
	else
		dataGridCosts = new KWVarPartDataGridClusteringCosts;
	check(dataGridCosts);
	return dataGridCosts;
}

const CCHierarchicalDataGrid* CCCoclusteringBuilder::GetCoclusteringDataGrid() const
{
	require(IsCoclusteringComputed());
	return coclusteringDataGrid;
}

const KWDataGridCosts* CCCoclusteringBuilder::GetCoclusteringDataGridCosts() const
{
	require(IsCoclusteringComputed());
	return coclusteringDataGridCosts;
}

void CCCoclusteringBuilder::SetReportFileName(const ALString& sFileName)
{
	sAnyTimeReportFileName = sFileName;
}

const ALString& CCCoclusteringBuilder::GetReportFileName() const
{
	return sAnyTimeReportFileName;
}

void CCCoclusteringBuilder::RemoveLastSavedReportFile() const
{
	if (sLastActualAnyTimeReportFileName != "")
		PLRemoteFileService::RemoveFile(sLastActualAnyTimeReportFileName);
	sLastActualAnyTimeReportFileName = "";
}

void CCCoclusteringBuilder::HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
						   const KWDataGrid* initialGranularizedDataGrid) const
{
	boolean bKeepIntermediateReports = false;
	boolean bWriteOk;
	const boolean bTrace = false;
	boolean bTraceGranularity;
	const int nMaxVarPartsDisplayed = 10;
	const double dEpsilon = 1e-6;
	double dCost;
	double dLevel;
	double dOptimizationTime;
	ALString sReportFileName;
	KWDataGridManager dataGridManager;
	CCCoclusteringReport coclusteringReport;
	ALString sCoclusteringSizeInfo;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	int nGranularityMax;
	ALString sMessage;
	ALString sTmp;

	require(optimizedDataGrid != NULL);
	require(nAnyTimeOptimizationIndex >= 0);
	require(dAnyTimeBestCost > 0);

	// Cout de la nouvelle solution
	dCost = coclusteringDataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);

	// Test si amelioration
	// Le model nul est sauvegarde une fois si necessaire
	// Les grilles avec un seul attribut informatif ne sont pas sauvegardees
	if (dCost < dAnyTimeBestCost - dEpsilon and (optimizedDataGrid->GetInformativeAttributeNumber() == 0 or
						     optimizedDataGrid->GetInformativeAttributeNumber() >= 2))
	{
		// Ajout de trace lie au profiling
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Save best solution");

		// Memorisation du meilleur cout
		dAnyTimeBestCost = dCost;

		// Sauvegarde de la grille
		if (coclusteringDataGrid != NULL)
			delete coclusteringDataGrid;
		coclusteringDataGrid = new CCHierarchicalDataGrid;

		// Copie de la grille pour calculer les infos du rapport en sortie
		// Dans le cas VarPart, creation d'innerAttributes propre a la grille hierarchique
		// afin qu'ils soient du type CCHDGAttribute comme les attributs de grille
		dataGridManager.CopyDataGridWithInnerAttributesCloned(optimizedDataGrid, coclusteringDataGrid);

		// Memorisation de la description courte
		coclusteringDataGrid->SetShortDescription(GetShortDescription());

		// Memorisation variable identifiant dans le cas d'un coclustering instances * variables
		if (optimizedDataGrid->IsVarPartDataGrid())
			coclusteringDataGrid->SetIdentifierAttributeName(GetIdentifierAttributeName());

		// Calcul de ses infos de hierarchie
		// Cas sans granularisation, ou de type VarPart
		if (initialGranularizedDataGrid == NULL or initialDataGrid->IsVarPartDataGrid())
			ComputeHierarchicalInfo(initialDataGrid, coclusteringDataGridCosts, coclusteringDataGrid);
		// Avec granularisation : la grille granularisee initiale est la reference pour le calcul des infos
		else
			ComputeHierarchicalInfo(initialGranularizedDataGrid, coclusteringDataGridCosts,
						coclusteringDataGrid);

		// Calcul du temps d'optimisation
		tAnyTimeTimer.Stop();
		dOptimizationTime = tAnyTimeTimer.GetElapsedTime(), tAnyTimeTimer.Start();

		// Calcul du Level
		dLevel = 1 - dAnyTimeBestCost / coclusteringDataGridCosts->GetTotalDefaultCost();

		// Calcul d'un libelle sur la taille de la grille (nombre de parties par dimension)
		for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
		{
			dgAttribute = coclusteringDataGrid->GetAttributeAt(nAttribute);
			if (nAttribute > 0)
				sCoclusteringSizeInfo += "*";
			sCoclusteringSizeInfo += IntToString(dgAttribute->GetPartNumber());
		}

		// Cas d'une grille individu * variable; ajout d'infos la partition des attributs
		if (GetVarPartCoclustering())
		{
			// Calcul d'un libelle sur la taille des partitions des variables internes
			sCoclusteringSizeInfo += "\tVarParts: ";
			sCoclusteringSizeInfo += IntToString(
			    coclusteringDataGrid->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts());
			sCoclusteringSizeInfo += "=";
			for (nAttribute = 0;
			     nAttribute < coclusteringDataGrid->GetInnerAttributes()->GetInnerAttributeNumber();
			     nAttribute++)
			{
				dgAttribute =
				    coclusteringDataGrid->GetInnerAttributes()->GetInnerAttributeAt(nAttribute);
				if (nAttribute >= nMaxVarPartsDisplayed)
				{
					sCoclusteringSizeInfo += "+...";
					break;
				}
				if (nAttribute > 0)
					sCoclusteringSizeInfo += "+";
				sCoclusteringSizeInfo += IntToString(dgAttribute->GetPartNumber());
			}
		}

		// Test si on doit afficher les info de granularite
		// On doit etre en mode granularite avec une granularite inferieure a la granularite maximale
		nGranularityMax = (int)ceil(log(initialDataGrid->GetGridFrequency() * 1.0) / log(2.0));
		bTraceGranularity = initialGranularizedDataGrid->GetGranularity() > 0 and
				    initialGranularizedDataGrid->GetGranularity() < nGranularityMax and
				    initialGranularizedDataGrid->GetLnGridSize() < initialDataGrid->GetLnGridSize();

		// Nom du fichier temporaire
		nAnyTimeOptimizationIndex++;
		sReportFileName = AnyTimeBuildTemporaryReportFileName(nAnyTimeOptimizationIndex);

		// Message utilisateur
		sMessage = sTmp + "  " + SecondsToString((int)dOptimizationTime);
		sMessage += sTmp + "\tWrite intermediate report " + FileService::GetFileName(sReportFileName);
		sMessage += sTmp + "\tLevel: " + DoubleToString(dLevel);
		sMessage += sTmp + "\tSize: " + sCoclusteringSizeInfo;
		if (bTraceGranularity)
		{
			sMessage +=
			    sTmp + "\tGranularity: " + IntToString(initialGranularizedDataGrid->GetGranularity());
		}
		AddSimpleMessage(sMessage);

		// Sauvegarde dans un fichier temporaire
		// On supprime le mode verbeux pour les sauvegardes intermediaires
		assert(JSONFile::GetVerboseMode() == true);
		JSONFile::SetVerboseMode(false);
		bWriteOk = coclusteringReport.WriteReport(sReportFileName, coclusteringDataGrid);
		JSONFile::SetVerboseMode(true);

		// Destruction de la precedente sauvegarde
		if (not bKeepIntermediateReports and bWriteOk)
			RemoveLastSavedReportFile();

		// Memorisation du nouveau nom du dernier fichier sauvegarde
		if (bWriteOk)
			sLastActualAnyTimeReportFileName = sReportFileName;

		// Ajout de trace lie au profiling
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", optimizedDataGrid->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dCost);
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("New best level", dLevel);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Save best solution");
	}
	else
	{
		if (bTrace)
			cout << "HandleOptimizationStep :: Grille non sauvegardee car n'apporte pas "
				"d'amelioration"
			     << endl;
	}
}

const ALString CCCoclusteringBuilder::GetClassLabel() const
{
	return GetLearningModuleName();
}

const ALString CCCoclusteringBuilder::GetObjectLabel() const
{
	// Cas d'un coclustering instances * variables: on renvoie le nom du dictionnaire
	if (GetVarPartCoclustering())
	{
		if (GetClass() == NULL)
			return "";
		else
			return GetClass()->GetName();
	}
	// Cas d'un coclustering de variable: on renvoie le nom des variables
	else
		return KWAttributeSubsetStats::GetObjectLabel();
}

void CCCoclusteringBuilder::OptimizeDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid)
{
	KWDataGridOptimizerVxV dataGridOptimizerVxV;
	KWDataGridOptimizerIxV dataGridOptimizerIxV;
	KWDataGridOptimizer* dataGridOptimizer;

	require(inputInitialDataGrid != NULL);
	require(coclusteringDataGridCosts == NULL);

	// Parametrage de l'optimiser selon le type de grille
	if (not initialDataGrid->IsVarPartDataGrid())
		dataGridOptimizer = &dataGridOptimizerVxV;
	else
		dataGridOptimizer = &dataGridOptimizerIxV;

	// Optimisation de la grille
	InitializeDataGridOptimizer(inputInitialDataGrid, dataGridOptimizer);

	// Trace de debut d'optimisation, avec informations sur la grille initiale
	KWDataGridOptimizer::GetProfiler()->BeginMethod("Compute coclustering");
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Dictionary", GetDatabase()->GetClassName());
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Database", GetDatabase()->GetDatabaseName());
	KWDataGridOptimizer::GetProfiler()->WriteKeyInt("Instances", initialDataGrid->GetGridFrequency());
	KWDataGridOptimizer::GetProfiler()->WriteKeyInt("Variables", initialDataGrid->GetAttributeNumber());
	KWDataGridOptimizer::GetProfiler()->WriteKeyBoolean("Is VarPart", initialDataGrid->IsVarPartDataGrid());
	if (initialDataGrid->IsVarPartDataGrid())
	{
		KWDataGridOptimizer::GetProfiler()->WriteKeyInt(
		    initialDataGrid->GetAttributeAt(0)->GetAttributeName() + " values",
		    initialDataGrid->GetAttributeAt(0)->GetInitialValueNumber());
		KWDataGridOptimizer::GetProfiler()->WriteKeyInt(
		    initialDataGrid->GetAttributeAt(1)->GetAttributeName() + " values",
		    initialDataGrid->GetAttributeAt(1)->GetInitialValueNumber());
		KWDataGridOptimizer::GetProfiler()->WriteKeyInt(
		    "Inner variables", initialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber());
	}

	// Optimisation de la grille
	dataGridOptimizer->OptimizeDataGrid(initialDataGrid, optimizedDataGrid);
	KWDataGridOptimizer::GetProfiler()->EndMethod("Compute coclustering");
}

void CCCoclusteringBuilder::InitializeDataGridOptimizer(const KWDataGrid* inputInitialDataGrid,
							KWDataGridOptimizer* dataGridOptimizer)
{
	require(inputInitialDataGrid != NULL);
	require(dataGridOptimizer != NULL);

	// Reinitialisation de l'optimiseur
	dataGridOptimizer->Reset();

	// Creation et initialisation de la structure de couts
	coclusteringDataGridCosts = CreateDataGridCost();

	// Parametrage des couts de l'optimiseur de grille
	dataGridOptimizer->SetDataGridCosts(coclusteringDataGridCosts);

	// Parametrage pour l'optimisation anytime: avoir acces aux ameliorations a chaque etape de l'optimisation
	dataGridOptimizer->SetOptimizationHandler(this);

	// Recopie du parametrage d'optimisation des grilles
	dataGridOptimizer->GetParameters()->CopyFrom(GetPreprocessingSpec()->GetDataGridOptimizerParameters());
	dataGridOptimizer->GetParameters()->SetOptimizationTime(RMResourceConstraints::GetOptimizationTime());

	// Initialisation des couts par defaut
	coclusteringDataGridCosts->InitializeDefaultCosts(inputInitialDataGrid);
	dAnyTimeBestCost = DBL_MAX;
}

boolean CCCoclusteringBuilder::CreateStandardInitialDataGrid()
{
	boolean bOk = true;
	KWTupleTable tupleTable;

	require(not GetVarPartCoclustering());
	require(CheckStandardSpecifications());
	require(initialDataGrid == NULL);
	require(GetDatabase()->GetObjects()->GetSize() == 0);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Create coclustering optimization data");

	// Lecture des enregistrements de la base
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Read database");
		TaskProgression::DisplayProgression(20);
		bOk = ReadDatabaseAndCheckRemainingMemory(GetDatabase());
	}

	// Alimentation d'une base de tuple a partir de la base, en tenant compte de l'attribut d'effectif
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Extract database tuples");
		TaskProgression::DisplayProgression(30);
		bOk = FillStandardTupleTableFromDatabase(GetDatabase(), &tupleTable);
	}

	// Calcul de statistiques descriptives globales et par attribut
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Compute univariate statistics");
		TaskProgression::DisplayProgression(40);
		bOk = GetLearningSpec()->ComputeTargetStats(&tupleTable);
		if (bOk and not TaskProgression::IsInterruptionRequested())
			bOk = ComputeDescriptiveAttributeStats(&tupleTable, &odDescriptiveStats);
	}

	// Verification de la memoire necessaire pour construire une grille initiale a partir des tuples
	if (bOk and not TaskProgression::IsInterruptionRequested())
		bOk = CheckMemoryForStandardDataGridInitialization(&tupleTable);

	// Creation du DataGrid
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Build initial finest coclustering");
		TaskProgression::DisplayProgression(100);
		initialDataGrid = CreateDataGrid(&tupleTable);
		bOk = initialDataGrid != NULL;
		assert(initialDataGrid == NULL or initialDataGrid->GetCellNumber() == tupleTable.GetSize());
		assert(initialDataGrid == NULL or GetDatabase()->GetObjects()->GetSize() == 0);
	}

	// Supression des tuples, desormais transferes dans la grille
	tupleTable.CleanAll();

	// Nettoyage de la base en cas d'echec
	if (not bOk)
		GetDatabase()->DeleteAll();

	// Fin de suivi de tache
	TaskProgression::EndTask();

	ensure(GetDatabase()->GetObjects()->GetSize() == 0);
	ensure(bOk or initialDataGrid == NULL);
	return bOk;
}

boolean CCCoclusteringBuilder::FillStandardTupleTableFromDatabase(KWDatabase* database, KWTupleTable* tupleTable)
{
	boolean bOk = true;
	longint lAvailableMemory;
	KWTuple* inputTuple;
	int nAttribute;
	KWAttribute* attribute;
	KWAttribute* frequencyAttribute;
	KWLoadIndexVector livLoadIndexes;
	KWObject* kwoObject;
	int nObject;
	int nObjectFrequency;
	longint lTotalFrequency;
	int nNewTupleNumberToBuild;
	longint lEmptyTupleTableUsedMemory;
	longint lMinTupleNecessaryMemory;
	longint lAvailableRemainingMemory;
	longint lRemainingTupleNecessaryMemory;
	ALString sTmp;

	require(not GetVarPartCoclustering());
	require(CheckStandardSpecifications());
	require(database != NULL);
	require(database->GetObjects()->GetSize() > 0);
	require(tupleTable != NULL);
	require(not tupleTable->GetUpdateMode());
	require(tupleTable->GetSize() == 0);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Extract database tuples " + database->GetDatabaseName());
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Recherche de l'index de l'attribut d'effectif
	frequencyAttribute = NULL;
	if (GetFrequencyAttributeName() != "")
	{
		frequencyAttribute = GetClass()->LookupAttribute(GetFrequencyAttributeName());
		check(frequencyAttribute);
	}

	// Initialisation des attributs de la table de tuples
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetClass()->LookupAttribute(GetAttributeNameAt(nAttribute));
		tupleTable->AddAttribute(attribute->GetName(), attribute->GetType());

		// Memorisation du LoadIndex de l'attribut
		livLoadIndexes.Add(attribute->GetLoadIndex());
	}

	// Memoire necessaire pour construire les prochain tuples, verifiee periodiquement
	nNewTupleNumberToBuild = TaskProgression::GetRefreshFrequency();
	lMinTupleNecessaryMemory = KWTupleTable::ComputeNecessaryMemory(nNewTupleNumberToBuild, GetAttributeNumber()) +
				   KWTupleTable::ComputeNecessaryBuildingMemory(nNewTupleNumberToBuild) + 16 * lMB;

	// Passage de la table de tuples en mode edition
	tupleTable->SetUpdateMode(true);
	inputTuple = tupleTable->GetInputTuple();
	lEmptyTupleTableUsedMemory = tupleTable->GetUsedMemory();

	// Parcours des objets de la base
	lTotalFrequency = 0;
	Global::ActivateErrorFlowControl();
	for (nObject = 0; nObject < database->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, database->GetObjects()->GetAt(nObject));
		assert(kwoObject != NULL);

		// Suivi de la tache au debut pour gerer la memoire de facon preventive
		if (TaskProgression::IsRefreshNecessary(nObject))
		{
			// Arret si pas assez de memoire restante
			lAvailableRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();
			if (lAvailableRemainingMemory < lMinTupleNecessaryMemory)
			{
				bOk = false;

				// Estimation de la memoire necessaire pour finaliser le remplissage de la table de tuple
				lRemainingTupleNecessaryMemory =
				    (longint)((tupleTable->GetUsedMemory() - lEmptyTupleTableUsedMemory) *
					      (double)(database->GetObjects()->GetSize() - nObject - 1) /
					      (nObject + 1));
				lRemainingTupleNecessaryMemory =
				    max(lRemainingTupleNecessaryMemory, lMinTupleNecessaryMemory);

				// Message d'erreur
				Object::AddError(
				    sTmp + "Not enough memory after extracting " +
				    LongintToReadableString((longint)nObject + 1) + " database tuples out of " +
				    LongintToReadableString(database->GetObjects()->GetSize()) +
				    RMResourceManager::BuildMissingMemoryMessage(lRemainingTupleNecessaryMemory));
				AddSimpleMessage(RMResourceManager::BuildMemoryLimitMessage());
				break;
			}

			// Arret si interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
				break;

			// Affichage de la progression
			TaskProgression::DisplayProgression(
			    (int)(100.0 * (nObject + 1) / (double)database->GetObjects()->GetSize()));
			TaskProgression::DisplayLabel(sTmp + IntToString(nObject) + " tuples");
		}

		// Acces a l'effectif, avec warning eventuel
		nObjectFrequency = GetDatabaseObjectFrequency(kwoObject, frequencyAttribute);
		assert(nObjectFrequency >= 0);
		lTotalFrequency += nObjectFrequency;

		// Erreur si effectif total trop important
		if (lTotalFrequency > INT_MAX)
		{
			Object::AddError(sTmp + "Database tuple extraction interrupted after record " +
					 LongintToString(kwoObject->GetCreationIndex()) +
					 " because total frequency is too large (" +
					 LongintToReadableString(lTotalFrequency) + ")");
			bOk = false;
			break;
		}

		// Prise en compte si effectif non null
		if (nObjectFrequency > 0)
		{
			// Parametrage du tuple d'entree de la table a cree
			for (nAttribute = 0; nAttribute < livLoadIndexes.GetSize(); nAttribute++)
			{
				if (tupleTable->GetAttributeTypeAt(nAttribute) == KWType::Symbol)
					inputTuple->SetSymbolAt(
					    nAttribute, kwoObject->GetSymbolValueAt(livLoadIndexes.GetAt(nAttribute)));
				else
					inputTuple->SetContinuousAt(nAttribute, kwoObject->GetContinuousValueAt(
										    livLoadIndexes.GetAt(nAttribute)));
			}

			// Ajout d'un nouveau tuple apres avoir specifie son effectif
			assert(nObjectFrequency <= INT_MAX - tupleTable->GetTotalFrequency());
			inputTuple->SetFrequency(nObjectFrequency);
			tupleTable->UpdateWithInputTuple();
		}

		// Liberation de l'objet une fois traite
		delete kwoObject;
		database->GetObjects()->SetAt(nObject, NULL);
	}
	Global::DesactivateErrorFlowControl();

	// Test final si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Message sur l'effectif total
	if (bOk and GetFrequencyAttributeName() != "" and not TaskProgression::IsInterruptionRequested())
		database->AddMessage(sTmp + "Total frequency: " + LongintToReadableString(lTotalFrequency));

	// Finalisation en repassant la table de tuples en mode consultation
	tupleTable->SetUpdateMode(false);
	if (not bOk)
		tupleTable->DeleteAll();

	// Erreur si aucun enregistrement lu
	if (bOk and tupleTable->GetSize() == 0)
	{
		AddError("No record in database");
		bOk = false;
	}

	// Nettoyage de la base pour liberer les objets non necessairement traites
	database->DeleteAll();

	// Fin de suivi de tache
	TaskProgression::EndTask();

	ensure(GetDatabase()->GetObjects()->GetSize() == 0);
	ensure(not tupleTable->GetUpdateMode());
	ensure(bOk or tupleTable->GetSize() == 0);
	return bOk;
}

int CCCoclusteringBuilder::GetDatabaseObjectFrequency(const KWObject* kwoObject, const KWAttribute* frequencyAttribute)
{
	int nObjectFrequency;
	Continuous cObjectFrequency;
	ALString sTmp;

	require(kwoObject != NULL);
	require(frequencyAttribute == NULL or frequencyAttribute->GetName() == GetFrequencyAttributeName());

	// Recherche de l'effectif de la cellule, en fonction de l'eventuelle variable d'effectif
	nObjectFrequency = 1;
	if (frequencyAttribute != NULL)
	{
		// Recherche de l'effectif
		cObjectFrequency = kwoObject->GetContinuousValueAt(frequencyAttribute->GetLoadIndex());
		nObjectFrequency = (int)floor(cObjectFrequency + 0.5);
		if (nObjectFrequency < 0)
			nObjectFrequency = 0;

		// Enregistrement ignore si effectif trop grand
		if (cObjectFrequency > INT_MAX)
		{
			GetDatabase()->AddWarning(
			    sTmp + "Ignored record " + LongintToString(kwoObject->GetCreationIndex()) +
			    ", frequency variable (" + GetFrequencyAttributeName() + ") with value too large (" +
			    KWContinuous::ContinuousToString(cObjectFrequency) + ")");

			// On met l'effectif a 0 pour ignorer l'enregistrement
			nObjectFrequency = 0;
		}
		// Enregistrement ignore si effectif negatif ou nul
		else if (cObjectFrequency <= 0)
		{
			GetDatabase()->AddWarning(
			    sTmp + "Ignored record " + LongintToString(kwoObject->GetCreationIndex()) +
			    ", frequency variable (" + GetFrequencyAttributeName() + ") with non positive value (" +
			    KWContinuous::ContinuousToString(cObjectFrequency) + ")");
		}
		// Warning si erreur d'arrondi
		else if (fabs(cObjectFrequency - nObjectFrequency) > 0.05)
		{
			if (nObjectFrequency > 0)
			{
				GetDatabase()->AddWarning(
				    sTmp + "Record " + LongintToString(kwoObject->GetCreationIndex()) +
				    ", frequency variable (" + GetFrequencyAttributeName() +
				    ") with non integer value (" + KWContinuous::ContinuousToString(cObjectFrequency) +
				    " -> " + IntToString(nObjectFrequency) + ")");
			}
			else
			{
				GetDatabase()->AddWarning(sTmp + "Ignored record " +
							  LongintToString(kwoObject->GetCreationIndex()) +
							  ", frequency variable (" + GetFrequencyAttributeName() +
							  ") with null rounded value (" +
							  KWContinuous::ContinuousToString(cObjectFrequency) + ")");
			}
		}
	}
	return nObjectFrequency;
}

boolean CCCoclusteringBuilder::CreateVarPartInitialDataGrid()
{
	boolean bOk = true;
	KWTupleTable tupleTable;

	require(GetVarPartCoclustering());
	require(CheckVarPartSpecifications());
	require(initialDataGrid == NULL);
	require(GetDatabase()->GetObjects()->GetSize() == 0);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Create coclustering optimization data");

	// Creation d'un grille vide de type IxV
	initialDataGrid = CreateVarPartEmptyDataGrid();

	// Lecture des enregistrements de la base
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Read database");
		TaskProgression::DisplayProgression(20);
		bOk = ReadDatabaseAndCheckRemainingMemory(GetDatabase());
	}

	// Creation des parties de l'attribut identifiant,
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Build initial instances");
		TaskProgression::DisplayProgression(30);
		bOk = InitializeIdentifierAttributeParts(GetDatabase(), initialDataGrid->GetAttributeAt(0),
							 &odDescriptiveStats);
	}

	// Creation des attributs internes et des parties elementaires de l'attribut de type VarPart,
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Build initial inner variables");
		TaskProgression::DisplayProgression(40);
		bOk = InitializeVarPartAttributeParts(GetDatabase(), initialDataGrid->GetAttributeAt(1),
						      &odDescriptiveStats);
	}
	assert(not bOk or odDescriptiveStats.GetCount() == GetInnerAttributesNames()->GetSize() + 1);

	// Creation des cellules de la grille initiale
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Build initial finest coclustering");
		TaskProgression::DisplayProgression(100);
		bOk = InitializeVarPartCells(GetDatabase(), initialDataGrid);
	}

	// Nettoyage si necessaire
	if (not bOk)
	{
		// Destruction de la grille
		if (initialDataGrid != NULL)
		{
			delete initialDataGrid;
			initialDataGrid = NULL;
		}

		// Nettoyage de la base sinon
		GetDatabase()->DeleteAll();
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();

	ensure(bOk or initialDataGrid == NULL);
	return initialDataGrid;
}

KWDataGrid* CCCoclusteringBuilder::CreateVarPartEmptyDataGrid()
{
	KWDataGrid* emptyDataGrid;
	KWDGAttribute* identifierAttribute;
	KWDGAttribute* varPartAttribute;

	// Creation du DataGrid initial avec un attribut identifiant et un attribut de type VarPart
	emptyDataGrid = new KWDataGrid;
	emptyDataGrid->Initialize(2, 0);

	// Parametrage de l'attribut Identifiant
	identifierAttribute = emptyDataGrid->GetAttributeAt(0);
	identifierAttribute->SetAttributeName(GetIdentifierAttributeName());
	identifierAttribute->SetAttributeType(KWType::Symbol);

	// Parametrage de l'attribut du DataGrid de type partie de variables
	// On specifie des attributs internes vides par defaut pour avoir une coherence initiale minimale
	varPartAttribute = emptyDataGrid->GetAttributeAt(1);
	varPartAttribute->SetAttributeName(GetVarPartAttributeName());
	varPartAttribute->SetAttributeType(KWType::VarPart);
	varPartAttribute->SetInnerAttributes(new KWDGInnerAttributes);
	return emptyDataGrid;
}

boolean CCCoclusteringBuilder::InitializeIdentifierAttributeParts(KWDatabase* database,
								  KWDGAttribute* identifierAttribute,
								  ObjectDictionary* odOutputDescriptiveStats)
{
	boolean bOk = true;
	boolean bTrace = false;
	KWTupleTableLoader tupleTableLoader;
	KWTupleTable identifierTupleTable;
	KWDescriptiveSymbolStats* descriptiveStats;
	int nTuple;
	const KWTuple* tuple;
	KWDGPart* part;
	KWDGValue* value;

	require(database != NULL);
	require(database->GetObjects()->GetSize() > 0);
	require(identifierAttribute != NULL);
	require(identifierAttribute->GetAttributeName() == GetIdentifierAttributeName());
	require(identifierAttribute->GetAttributeType() == KWType::Symbol);
	require(identifierAttribute->GetPartNumber() == 0);
	require(odOutputDescriptiveStats != NULL);
	require(odOutputDescriptiveStats->Lookup(identifierAttribute->GetAttributeName()) == NULL);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Extract instances from database");

	// Parametrage du chargeur de tuple
	tupleTableLoader.SetInputClass(GetClass());
	tupleTableLoader.SetInputDatabaseObjects(database->GetObjects());

	// Creation de la table de tuple de l'attribut identifiant
	if (bOk and not TaskProgression::IsInterruptionRequested())
		tupleTableLoader.LoadUnivariate(identifierAttribute->GetAttributeName(), &identifierTupleTable);

	// Initialisation des stat desciptives
	descriptiveStats = NULL;
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		descriptiveStats = new KWDescriptiveSymbolStats;
		descriptiveStats->SetLearningSpec(GetLearningSpec());
		descriptiveStats->SetAttributeName(identifierAttribute->GetAttributeName());
		descriptiveStats->ComputeStats(&identifierTupleTable);
		odOutputDescriptiveStats->SetAt(identifierAttribute->GetAttributeName(), descriptiveStats);
	}

	// Tri des identifiants, pour ameliorer la reproductibilite
	if (bOk and not TaskProgression::IsInterruptionRequested())
		identifierTupleTable.SortByValues();

	// Creation d'une partie par valeur
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		for (nTuple = 0; nTuple < identifierTupleTable.GetSize(); nTuple++)
		{
			tuple = identifierTupleTable.GetAt(nTuple);

			// Progression
			if (TaskProgression::IsRefreshNecessary(nTuple))
			{
				TaskProgression::DisplayProgression(
				    (int)(100.0 * nTuple / identifierTupleTable.GetSize()));
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}
			}

			// Creation d'une nouvelle partie mono-valeur
			part = identifierAttribute->AddPart();
			value = part->GetSymbolValueSet()->AddSymbolValue(tuple->GetSymbolAt(0));
		}

		// Initialisation de la partie par defaut, contenant la modalite speciale
		if (bOk)
			identifierAttribute->GetTailPart()->GetSymbolValueSet()->AddSymbolValue(Symbol::GetStarValue());

		// Memorisation du nombre de valeurs initial
		identifierAttribute->SetInitialValueNumber(identifierTupleTable.GetSize());
		identifierAttribute->SetGranularizedValueNumber(identifierAttribute->GetInitialValueNumber());
	}

	// Test final si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Nettoyage si echec
	if (not bOk)
	{
		descriptiveStats = cast(KWDescriptiveSymbolStats*,
					odOutputDescriptiveStats->Lookup(identifierAttribute->GetAttributeName()));
		if (descriptiveStats != NULL)
		{
			odOutputDescriptiveStats->RemoveKey(identifierAttribute->GetAttributeName());
			delete descriptiveStats;
			descriptiveStats = NULL;
		}
		identifierAttribute->DeleteAllParts();
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();

	// Trace
	if (bTrace)
	{
		cout << "InitializeIdentifierAttributeParts\n";
		cout << "Identifier values\n" << identifierTupleTable << endl;
		if (descriptiveStats != NULL)
		{
			cout << "Identifier stats\n";
			descriptiveStats->WriteReport(cout);
		}
		cout << "Identifier attribute\n";
		identifierAttribute->WriteParts(cout);
	}
	ensure(not bOk or identifierAttribute->Check());
	return bOk;
}

boolean CCCoclusteringBuilder::InitializeVarPartAttributeParts(KWDatabase* database, KWDGAttribute* varPartAttribute,
							       ObjectDictionary* odOutputDescriptiveStats)
{
	boolean bOk = true;
	int nAttribute;
	KWAttribute* attribute;
	int n;
	KWAttributeBlock* attributeBlock;
	ObjectArray oaBlockAttributes;
	ObjectDictionary odBlockAttributes;
	ObjectDictionary odBlockTupleTables;
	KWTupleTable univariateTupleTable;
	KWTupleTable* tupleTable;
	KWTupleTable targetTupleTable;
	KWDGAttribute* innerAttribute;
	KWTupleTableLoader tupleTableLoader;
	ObjectDictionary odInnerAttributes;
	KWDGInnerAttributes* initialInnerAttributes;
	int nInitializedAttributeNumber;
	int nEmptyInnerAttributeNumber;
	longint lAvailableMemory;
	longint lNecessaryMemory;
	longint lBlockValueNumber;
	ALString sTmp;

	require(GetTargetAttributeName() == "");
	require(database != NULL);
	require(database->GetObjects()->GetSize() > 0);
	require(GetClass()->GetLoadedAttributeNumber() == 1 + GetInnerAttributesNames()->GetSize());
	require(varPartAttribute != NULL);
	require(varPartAttribute->GetAttributeName() == GetVarPartAttributeName());
	require(varPartAttribute->GetAttributeType() == KWType::VarPart);
	require(varPartAttribute->GetPartNumber() == 0);
	require(odOutputDescriptiveStats != NULL);
	require(odOutputDescriptiveStats->Lookup(varPartAttribute->GetAttributeName()) == NULL);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Extract inner variables from database");

	// Parametrage du chargeur de tuple
	tupleTableLoader.SetInputClass(GetClass());
	tupleTableLoader.SetInputDatabaseObjects(database->GetObjects());

	// On parcours les attributs de la classe selon l'ordre dans la classe pour traiter
	// les attributs par bloc de variables si possible pour les trater efficacement de facon sparse.
	// Attention, l'ordre des attributs internes n'est pas necessairement celui des blocs de variable,
	// et il faudra enuite reordonner les attributs internes dans l'ordre alphabetique
	nInitializedAttributeNumber = 0;
	nEmptyInnerAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < GetClass()->GetLoadedAttributeNumber(); nAttribute++)
	{
		attribute = GetClass()->GetLoadedAttributeAt(nAttribute);

		// Cas d'un attribut dense
		// Cas ou l'attribut n'est pas dans un bloc
		if (not attribute->IsInBlock())
		{
			// On saute l'attribut identifiant
			if (attribute->GetName() == GetIdentifierAttributeName())
				continue;

			// Gestion de la progression
			TaskProgression::DisplayLabel(attribute->GetName());
			TaskProgression::DisplayProgression((100 * (nInitializedAttributeNumber + 1)) /
							    GetInnerAttributesNames()->GetSize());
			if (TaskProgression::IsInterruptionRequested())
			{
				bOk = false;
				break;
			}

			// Test de depassement memoire, avec estimation de la place prise par la table des tuples
			// dans le pire des cas avec des valeurs toutes distinctes
			lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();
			lNecessaryMemory =
			    KWTupleTable::ComputeNecessaryMemory(database->GetObjects()->GetSize(), 1) +
			    KWTupleTable::ComputeNecessaryBuildingMemory(database->GetObjects()->GetSize());
			lNecessaryMemory +=
			    database->GetObjects()->GetSize() * (sizeof(KWDGPart) + sizeof(KWDGInterval));
			if (lAvailableMemory < lNecessaryMemory)
			{
				Object::AddError(sTmp + "Not enough memory to initialize the inner variable " +
						 IntToString(nInitializedAttributeNumber + 1) + " among " +
						 IntToString(GetInnerAttributesNames()->GetSize()) +
						 RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
				AddSimpleMessage(RMResourceManager::BuildMemoryLimitMessage());
				bOk = false;
				break;
			}

			// Chargement de la table de tuple pour l'attribut
			tupleTableLoader.LoadUnivariate(attribute->GetName(), &univariateTupleTable);

			// Creation de l'attribut interne correspondant et mise a jour des containers associes
			innerAttribute = CreateInnerAttribute(attribute, varPartAttribute, &univariateTupleTable,
							      &odInnerAttributes, odOutputDescriptiveStats);
			nInitializedAttributeNumber++;
			if (innerAttribute == NULL)
				nEmptyInnerAttributeNumber++;
		}
		// Cas ou l'attribut est dans un block
		else
		{
			attributeBlock = attribute->GetAttributeBlock();

			// Test de depassement memoire, avec estimation de la place prise par l'ensemble des table des tuples du bloc
			// dans le pire des cas avec des valeurs toutes distinctes
			lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();
			lBlockValueNumber = ComputeDatabaseBlockValueNumber(database, attributeBlock);
			lNecessaryMemory = KWTupleTable::ComputeNecessaryMemory(lBlockValueNumber, 1) +
					   KWTupleTable::ComputeNecessaryBuildingMemory(lBlockValueNumber);
			lNecessaryMemory +=
			    attributeBlock->GetLoadedAttributeNumber() * (sizeof(KWTupleTable) + sizeof(KWTupleTable*));
			lNecessaryMemory +=
			    database->GetObjects()->GetSize() * (sizeof(KWDGPart) + sizeof(KWDGInterval));
			if (lAvailableMemory < lNecessaryMemory)
			{
				Object::AddError(sTmp +
						 "Not enough memory to initialize the block of inner variables from " +
						 IntToString(nInitializedAttributeNumber + 1) + " to " +
						 IntToString(nInitializedAttributeNumber +
							     attributeBlock->GetLoadedAttributeNumber()) +
						 " among " + IntToString(GetInnerAttributesNames()->GetSize()) +
						 RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
				AddSimpleMessage(RMResourceManager::BuildMemoryLimitMessage());
				bOk = false;
				break;
			}

			// Collecte de tous les attributs consecutifs du meme bloc
			assert(oaBlockAttributes.GetSize() == 0);
			assert(odBlockAttributes.GetCount() == 0);
			for (n = 0; n < attributeBlock->GetLoadedAttributeNumber(); n++)
			{
				attribute = attributeBlock->GetLoadedAttributeAt(n);
				oaBlockAttributes.Add(attribute);
				odBlockAttributes.SetAt(attribute->GetName(), attribute);

				// L'attribut identifiant ne peut pas etre sparse
				assert(attribute->GetName() != GetIdentifierAttributeName());
			}
			nAttribute += attributeBlock->GetLoadedAttributeNumber() - 1;

			// Chargement de la table de tuple pour tous les attributs du bloc
			tupleTableLoader.BlockLoadUnivariateInitialize(attributeBlock->GetName(), &odBlockAttributes,
								       &odBlockTupleTables);

			// Parcours des attributs du bloc
			for (n = 0; n < oaBlockAttributes.GetSize(); n++)
			{
				// Gestion de la progression en se basant sur le nombre de statistique deja calculees
				TaskProgression::DisplayLabel(attribute->GetName());
				TaskProgression::DisplayProgression((100 * (nInitializedAttributeNumber + 1)) /
								    GetInnerAttributesNames()->GetSize());

				// Arret si erreur ou interruption
				if (TaskProgression::IsInterruptionRequested())
				{
					odBlockTupleTables.DeleteAll();
					bOk = false;
					break;
				}

				// Acces a l'attribut correspondant aux stats
				attribute = cast(KWAttribute*, oaBlockAttributes.GetAt(n));
				assert(attribute->GetLoaded());
				assert(KWType::IsSimple(attribute->GetType()));
				assert(attribute->GetName() != GetIdentifierAttributeName());

				// Acces la table de tuple courante
				tupleTable = cast(KWTupleTable*, odBlockTupleTables.Lookup(attribute->GetName()));
				assert(tupleTable != NULL);
				assert(tupleTable->GetAttributeNameAt(0) == attribute->GetName());

				// Finalisation de l'alimentation de la table, en prenant en compte les valeurs manquantes
				tupleTableLoader.BlockLoadUnivariateFinalize(attributeBlock->GetName(), tupleTable);

				// Creation de l'attribut interne correspondant et mise a jour des containers associes
				innerAttribute = CreateInnerAttribute(attribute, varPartAttribute, tupleTable,
								      &odInnerAttributes, odOutputDescriptiveStats);
				nInitializedAttributeNumber++;
				if (innerAttribute == NULL)
					nEmptyInnerAttributeNumber++;

				// On detruit la table de tuples et on la supprime du dictionnaire
				// qui pourrait etre detruit exhaustivement en cas d'interruption utilisateur
				odBlockTupleTables.RemoveKey(attribute->GetName());
				delete tupleTable;
			}
			assert(odBlockTupleTables.GetCount() == 0);

			// Nettoyage
			oaBlockAttributes.RemoveAll();
			odBlockAttributes.RemoveAll();
		}
	}

	// Test final si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Erreur si aucun attribut interne ayant des valeurs
	if (bOk and odInnerAttributes.GetCount() == 0)
	{
		AddError("All inner variables contain only missing values");
		bOk = false;
	}

	// Nettoyage si erreur
	if (not bOk)
		odInnerAttributes.DeleteAll();
	// Creation des attributs internes sinon
	// Pas de suivi de tache ici: cela doit se faire tres rapidement
	else
	{
		assert(odInnerAttributes.GetCount() + nEmptyInnerAttributeNumber ==
		       GetInnerAttributesNames()->GetSize());

		// Ajout des attributs internes, apres les avoir trie par nom pour etre en phase avec
		// les attributs internes de la grille, tries egalement par nom
		GetInnerAttributesNames()->Sort();

		// Creation de la description initiale des attributs internes
		initialInnerAttributes = new KWDGInnerAttributes;

		// Ajout des attribut interne dans l'ordre specifie
		for (nAttribute = 0; nAttribute < GetInnerAttributesNames()->GetSize(); nAttribute++)
		{
			innerAttribute = cast(KWDGAttribute*,
					      odInnerAttributes.Lookup(GetInnerAttributesNames()->GetAt(nAttribute)));
			assert(innerAttribute != NULL or nEmptyInnerAttributeNumber > 0);

			// Prise en compte des attributs internes non vide
			if (innerAttribute != NULL)
				initialInnerAttributes->AddInnerAttribute(innerAttribute);
		}

		// Parametrage des attribut interne
		varPartAttribute->SetInnerAttributes(initialInnerAttributes);

		// Creation des parties de l'attribut de grille de type de type VarPart
		// A la creation, une partie est un cluster de parties de variables qui ne contient qu'une
		// partie de variable
		if (initialInnerAttributes->GetInnerAttributeNumber() > 0)
			varPartAttribute->CreateVarPartsSet();
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();

	ensure(not bOk or varPartAttribute->GetInnerAttributeNumber() + nEmptyInnerAttributeNumber ==
			      GetInnerAttributesNames()->GetSize());
	ensure(not bOk or varPartAttribute->GetPartNumber() == varPartAttribute->GetInitialValueNumber());
	ensure(not bOk or varPartAttribute->Check());
	return bOk;
}

KWDGAttribute* CCCoclusteringBuilder::CreateInnerAttribute(const KWAttribute* attribute,
							   KWDGAttribute* varPartAttribute, KWTupleTable* tupleTable,
							   ObjectDictionary* odInnerAttributes,
							   ObjectDictionary* odOutputDescriptiveStats)
{
	KWDGAttribute* innerAttribute;
	KWDescriptiveStats* descriptiveStats;

	require(attribute != NULL);
	require(KWType::IsSimple(attribute->GetType()));
	require(attribute->GetLoaded());
	require(varPartAttribute != NULL);
	require(varPartAttribute->GetAttributeType() == KWType::VarPart);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNumber() == 1);
	require(tupleTable->GetAttributeNameAt(0) == attribute->GetName());
	require(tupleTable->GetAttributeTypeAt(0) == attribute->GetType());
	require(odInnerAttributes != NULL);
	require(odInnerAttributes->Lookup(attribute->GetName()) == NULL);
	require(odOutputDescriptiveStats != NULL);
	require(odOutputDescriptiveStats->Lookup(attribute->GetName()) == NULL);

	// Supression des eventuelles valeurs manquantes
	// Dans le cas des blocs des valeur, supression egalement de la valeur par defaut du bloc un traitement sparse efficace
	//
	// Il est a noter que ce "nettoyage" pourrait etre effectue plus efficacement s'il etait realise directement
	// lors du chargement des tables de tuples par le TupleLoader. Mais cela demanderait des methodes specifiques,
	// avec une complexite accrue. La solution actuelle est nettement plus modulaire et simple, et son cout algorithmique
	// a un overhead negligeable (O(n) tres basique) par rapport a celui du TupleLoader (O(n log n))
	if (attribute->GetType() == KWType::Continuous)
	{
		RemoveTupleWithMissingContinuousValue(tupleTable, KWContinuous::GetMissingValue());
		if (attribute->IsInBlock() and
		    attribute->GetAttributeBlock()->GetContinuousDefaultValue() != KWContinuous::GetMissingValue())
			RemoveTupleWithMissingContinuousValue(
			    tupleTable, attribute->GetAttributeBlock()->GetContinuousDefaultValue());
	}
	else
	{
		RemoveTupleWithMissingSymbolValue(tupleTable, Symbol());
		if (attribute->IsInBlock() and attribute->GetAttributeBlock()->GetSymbolDefaultValue() != Symbol())
			RemoveTupleWithMissingSymbolValue(tupleTable,
							  attribute->GetAttributeBlock()->GetSymbolDefaultValue());
	}

	// Creation et initialisation d'un objet de stats pour l'attribut
	// Creation d'un objet de stats pour l'attribut selon son type
	if (attribute->GetType() == KWType::Continuous)
		descriptiveStats = new KWDescriptiveContinuousStats;
	else
		descriptiveStats = new KWDescriptiveSymbolStats;

	// Initialisation
	descriptiveStats->SetLearningSpec(GetLearningSpec());
	descriptiveStats->SetAttributeName(attribute->GetName());

	// Calcul des stats
	descriptiveStats->ComputeStats(tupleTable);

	// Memorisation des stats
	odOutputDescriptiveStats->SetAt(descriptiveStats->GetAttributeName(), descriptiveStats);

	// Creation de l'attribut interne s'il contient au moins une instance
	innerAttribute = NULL;
	if (tupleTable->GetSize() > 0)
	{
		// Creation de l'attribut interne correspondant
		innerAttribute = new KWDGAttribute;

		// Parametrage de l'attribut interne
		innerAttribute->SetOwnerAttributeName(varPartAttribute->GetAttributeName());

		// Parametrage de l'attribut du DataGrid
		innerAttribute->SetAttributeName(attribute->GetName());
		innerAttribute->SetAttributeType(attribute->GetType());

		// Recuperation du cout de selection/construction de l'attribut hors attribut cible
		attribute = GetClass()->LookupAttribute(innerAttribute->GetAttributeName());
		check(attribute);
		innerAttribute->SetCost(attribute->GetCost());

		// Ajout de l'attribut interne dans un tableau temporaire pour reordonner ensuite les attributs internes par nom
		odInnerAttributes->SetAt(innerAttribute->GetAttributeName(), innerAttribute);

		// Tri des tuples par effectif decroissant, puis valeurs croissantes dans le cas Symbol
		if (attribute->GetType() == KWType::Symbol)
			tupleTable->SortByDecreasingFrequencies();

		// Creation des parties de l'attribut interne, apres avoir supprime les eventuelles valeurs manquantes
		if (innerAttribute->GetAttributeType() == KWType::Continuous)
			CreateAttributeIntervals(tupleTable, innerAttribute);
		else
			CreateAttributeValueSets(tupleTable, innerAttribute);

		// Nettoyage des tuples
		tupleTable->CleanAll();
	}
	ensure(odOutputDescriptiveStats->Lookup(attribute->GetName()) != NULL);
	return innerAttribute;
}

boolean CCCoclusteringBuilder::InitializeVarPartCells(KWDatabase* database, KWDataGrid* dataGrid)
{
	boolean bOk = true;
	int nObject;
	KWDGAttribute* dgIdentifierAttribute;
	KWDGAttribute* dgVarPartAttribute;
	const KWDGInnerAttributes* innerAttributes;
	KWDGAttribute* dgInnerAttribute;
	KWAttribute* identifierAttribute;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWLoadIndex liIdentifier;
	KWObject* kwoObject;
	ObjectArray oaParts;
	KWDGPart* identifierPart;
	Symbol sIdentifierValue;
	KWDGValue* identifierValue;
	Symbol sValue;
	Continuous cValue;
	KWDGPart* valuePart;
	KWDGPart* varPartAttributePart;
	KWDGCell* cell;
	KWContinuousValueBlock* cvbContinuousValues;
	KWSymbolValueBlock* svbSymbolValues;
	int n;
	int nValue;
	int nSparseIndex;
	int nRefreshFrequency;
	longint lNecessaryMemoryToBuildIndexingStructure;
	longint lAvailableRemainingMemory;
	longint lCellSize;
	longint lMinCellNecessaryMemory;
	longint lRemainingCellNecessaryMemory;
	longint lTotalRemainingValueNumber;
	ALString sTmp;

	require(database != NULL);
	require(database->GetObjects()->GetSize() > 0);
	require(database->GetClassName() == GetClass()->GetName());
	require(GetClass()->GetLoadedAttributeNumber() == 1 + GetInnerAttributesNames()->GetSize());
	require(dataGrid != NULL);
	require(dataGrid->GetAttributeNumber() == 2);
	require(dataGrid->GetAttributeAt(0)->GetAttributeName() == GetIdentifierAttributeName());
	require(dataGrid->GetAttributeAt(1)->GetAttributeName() == GetVarPartAttributeName());
	require(dataGrid->GetAttributeAt(0)->GetPartNumber() > 0);
	require(dataGrid->GetAttributeAt(1)->GetPartNumber() > 0);
	require(dataGrid->GetCellNumber() == 0);
	require(dataGrid->GetInnerAttributes() != NULL);
	require(not dataGrid->GetCellUpdateMode());

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Extract all database values");

	// Rerchercher des attribut de grille pour les deux dimensions
	dgIdentifierAttribute = dataGrid->GetAttributeAt(0);
	dgVarPartAttribute = dataGrid->GetAttributeAt(1);
	innerAttributes = dgVarPartAttribute->GetInnerAttributes();

	// Recherche de l'attribut identifier dans la classe
	identifierAttribute = GetClass()->LookupAttribute(GetIdentifierAttributeName());
	check(identifierAttribute);
	liIdentifier = identifierAttribute->GetLoadIndex();
	assert(liIdentifier.IsValid());

	// Estimation de la memoire necessaire pour construire la structure d'indexation
	lAvailableRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();
	lNecessaryMemoryToBuildIndexingStructure = dataGrid->ComputeNecessaryMemoryForIndexingStructure();
	if (lAvailableRemainingMemory < lNecessaryMemoryToBuildIndexingStructure)
	{
		bOk = false;
		Object::AddError(
		    sTmp + "Not enough remaining memory to start creating coclustering cells" +
		    RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemoryToBuildIndexingStructure));
		AddSimpleMessage(RMResourceManager::BuildMemoryLimitMessage());
	}

	// Passage en mode update
	if (bOk)
	{
		dataGrid->BuildIndexingStructure();
		dataGrid->SetCellUpdateMode(true);
	}

	// Initialisation du tableau de partie par cellule
	oaParts.SetSize(dataGrid->GetAttributeNumber());

	// Calcul de la frequence de rafraichissement de la barre de progression
	nRefreshFrequency = 1 + 8192 / (1 + dgVarPartAttribute->GetInnerAttributeNumber());
	nRefreshFrequency = min(nRefreshFrequency, TaskProgression::GetRefreshFrequency());

	// Calcul de la taille necessaire pour creer les prochaines cellules entre deux rafraichissement
	lCellSize = sizeof(KWDGCell) + (2 + initialDataGrid->GetAttributeNumber()) * sizeof(void*);
	lMinCellNecessaryMemory =
	    (longint)lCellSize * dgVarPartAttribute->GetInnerAttributeNumber() * nRefreshFrequency;

	// Parcours des objet de la base
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		for (nObject = 0; nObject < database->GetObjects()->GetSize(); nObject++)
		{
			kwoObject = cast(KWObject*, database->GetObjects()->GetAt(nObject));
			assert(kwoObject != NULL);

			// Suivi de tache
			if (nObject % nRefreshFrequency == 0)
			{
				// Gestion de la progression
				TaskProgression::DisplayProgression(
				    (int)(100.0 * nObject / database->GetObjects()->GetSize()));
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}

				// Gestion de la memoire
				lAvailableRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();
				if (lAvailableRemainingMemory < lMinCellNecessaryMemory)
				{
					// Calcul du nombre total de valeurs restant dans la base de la base, qui est un borne sup de nombre de cellules
					lTotalRemainingValueNumber =
					    ComputeDatabaseTotalRemainingValueNumber(database, nObject);

					// Message unbiquement si pas d'interruption utilisateur (sinon, lTotalValueNimber n'est pas estime)
					if (not TaskProgression::IsInterruptionRequested())
					{
						// Estimation de la memoire necessaire pour creer toutes les cellules
						lRemainingCellNecessaryMemory = lTotalRemainingValueNumber * lCellSize;
						lRemainingCellNecessaryMemory =
						    max(lRemainingCellNecessaryMemory, lMinCellNecessaryMemory);

						// Message d'erreur
						Object::AddError(sTmp +
								 "Not enough memory to create coclustering cells after "
								 "processing " +
								 IntToString((int)(100.0 * nObject /
										   database->GetObjects()->GetSize())) +
								 "% of the data base" +
								 RMResourceManager::BuildMissingMemoryMessage(
								     lRemainingCellNecessaryMemory));
						AddSimpleMessage(RMResourceManager::BuildMemoryLimitMessage());
					}
					bOk = false;
					break;
				}
			}

			// Valeur pour l'attribut identifiant
			sIdentifierValue = kwoObject->GetSymbolValueAt(liIdentifier);

			// Rechercher de la partie correspondant a l'identifiant
			identifierPart = dgIdentifierAttribute->LookupSymbolPart(sIdentifierValue);
			assert(identifierPart != NULL);
			assert(identifierPart->GetValueSet()->GetValueNumber() == 1);
			assert(identifierPart->GetPartFrequency() == 0);

			// Recherche de la valeur de poartie correspondant a l'identifier
			identifierValue = identifierPart->GetValueSet()->GetHeadValue();
			assert(identifierValue->GetSymbolValue() == sIdentifierValue);

			// Initialisation de la partie corresponbdant a l'identifiant
			oaParts.SetAt(0, identifierPart);

			// Parcours des attributs dense l'objet pour creer une cellule par valeur non manquante
			for (n = 0; n < GetClass()->GetLoadedDenseAttributeNumber(); n++)
			{
				attribute = GetClass()->GetLoadedDenseAttributeAt(n);

				// On ignore l'attribut identifiant
				if (attribute == identifierAttribute)
					continue;

				// Recherche de l'attribut interne correspondant
				dgInnerAttribute = innerAttributes->LookupInnerAttribute(attribute->GetName());

				// Valeur de l'attribut interne dans le cas Continuous
				valuePart = NULL;
				if (attribute->GetType() == KWType::Continuous)
				{
					cValue = kwoObject->GetContinuousValueAt(attribute->GetLoadIndex());

					// Si non missing, recherche de la partie correspondant de l'attribut interne
					if (cValue != KWContinuous::GetMissingValue())
						valuePart = dgInnerAttribute->LookupContinuousPart(cValue);
				}
				// Valeur de l'attribut interne dans le cas Symbol
				else
				{
					sValue = kwoObject->GetSymbolValueAt(attribute->GetLoadIndex());

					// Si non missing, recherche de la partie correspondant de l'attribut interne
					if (not sValue.IsEmpty())
					{
						valuePart = dgInnerAttribute->LookupSymbolPart(sValue);
						assert(valuePart->GetValueSet()->GetValueNumber() == 1);
						assert(valuePart->GetValueSet()->GetHeadValue()->GetValueFrequency() >
						       0);
					}
				}

				// Traitement de la valeur si elle est presente
				if (valuePart != NULL)
				{
					// Mise a jour de l'effectif de la valeur de l'identifiant
					identifierValue->SetValueFrequency(identifierValue->GetValueFrequency() + 1);

					// Mise a jour de l'effectif de la partie de variable interne
					valuePart->SetPartFrequency(valuePart->GetPartFrequency() + 1);

					// Recherche du cluster contenant cette partie de variable
					varPartAttributePart = dgVarPartAttribute->LookupVarPart(valuePart);

					// Parametrage de la dimension VarPart de la cellule
					oaParts.SetAt(1, varPartAttributePart);

					// Recherche de la cellule, et creation si necessaire
					cell = dataGrid->LookupCell(&oaParts);
					if (cell == NULL)
						cell = dataGrid->AddCell(&oaParts);
					assert(cell != NULL);

					// Mise a jour de l'effectif de la cellule
					cell->SetCellFrequency(cell->GetCellFrequency() + 1);
				}
			}

			// Parcours des blocs d'attribut sparse de l'objet pour creer une cellule par valeur present de chaque bloc
			for (n = 0; n < GetClass()->GetLoadedAttributeBlockNumber(); n++)
			{
				attributeBlock = GetClass()->GetLoadedAttributeBlockAt(n);

				// Cas d'un bloc Continuous
				if (attributeBlock->GetType() == KWType::Continuous)
				{
					cvbContinuousValues =
					    kwoObject->GetContinuousValueBlockAt(attributeBlock->GetLoadIndex());

					// Mise a jour de l'effectif de la valeur de l'identifiant pour toutes les valeurs du bloc
					identifierValue->SetValueFrequency(identifierValue->GetValueFrequency() +
									   cvbContinuousValues->GetValueNumber());

					// Parcours des valeurs du bloc
					for (nValue = 0; nValue < cvbContinuousValues->GetValueNumber(); nValue++)
					{
						// Acces a l'index de l'attribut et a sa valeur
						nSparseIndex = cvbContinuousValues->GetAttributeSparseIndexAt(nValue);
						cValue = cvbContinuousValues->GetValueAt(nValue);

						// Recherche de l'attribut interne correspondant
						attribute = attributeBlock->GetLoadedAttributeAt(nSparseIndex);
						dgInnerAttribute =
						    innerAttributes->LookupInnerAttribute(attribute->GetName());

						// Recherche de la partie correspondant de l'attribut interne
						valuePart = dgInnerAttribute->LookupContinuousPart(cValue);

						// Mise a jour de l'effectif de la partie de variable interne
						valuePart->SetPartFrequency(valuePart->GetPartFrequency() + 1);

						// Recherche du cluster contenant cette partie de variable
						varPartAttributePart = dgVarPartAttribute->LookupVarPart(valuePart);

						// Parametrage de la dimension VarPart de la cellule
						oaParts.SetAt(1, varPartAttributePart);

						// Recherche de la cellule, et creation si necessaire
						cell = dataGrid->LookupCell(&oaParts);
						if (cell == NULL)
							cell = dataGrid->AddCell(&oaParts);
						assert(cell != NULL);

						// Mise a jour de l'effectif de la cellule
						cell->SetCellFrequency(cell->GetCellFrequency() + 1);
					}
				}
				// Cas d'un bloc Symbol
				else
				{
					assert(attributeBlock->GetType() == KWType::Symbol);
					svbSymbolValues =
					    kwoObject->GetSymbolValueBlockAt(attributeBlock->GetLoadIndex());

					// Mise a jour de l'effectif de la valeur de l'identifiant pour toutes les valeurs du bloc
					identifierValue->SetValueFrequency(identifierValue->GetValueFrequency() +
									   svbSymbolValues->GetValueNumber());

					// Parcours des valeurs du bloc
					for (nValue = 0; nValue < svbSymbolValues->GetValueNumber(); nValue++)
					{
						// Acces a l'index de l'attribut et a sa valeur
						nSparseIndex = svbSymbolValues->GetAttributeSparseIndexAt(nValue);
						sValue = svbSymbolValues->GetValueAt(nValue);

						// Recherche de l'attribut interne correspondant
						attribute = attributeBlock->GetLoadedAttributeAt(nSparseIndex);
						dgInnerAttribute =
						    innerAttributes->LookupInnerAttribute(attribute->GetName());

						// Recherche de la partie correspondant de l'attribut interne
						valuePart = dgInnerAttribute->LookupSymbolPart(sValue);

						// Mise a jour de l'effectif de la partie de variable interne
						valuePart->SetPartFrequency(valuePart->GetPartFrequency() + 1);

						// Recherche du cluster contenant cette partie de variable
						varPartAttributePart = dgVarPartAttribute->LookupVarPart(valuePart);

						// Parametrage de la dimension VarPart de la cellule
						oaParts.SetAt(1, varPartAttributePart);

						// Recherche de la cellule, et creation si necessaire
						cell = dataGrid->LookupCell(&oaParts);
						if (cell == NULL)
							cell = dataGrid->AddCell(&oaParts);
						assert(cell != NULL);

						// Mise a jour de l'effectif de la cellule
						cell->SetCellFrequency(cell->GetCellFrequency() + 1);
					}
				}
			}

			// Nettoyage de l'objet
			delete kwoObject;
			database->GetObjects()->SetAt(nObject, NULL);
		}
		Global::DesactivateErrorFlowControl();
	}

	// Mise a jour des statistiques globale de la grille
	// Permet entre autre de calculer les effectifs par partie pour les attributs identifiant et VarPart
	if (bOk)
		dataGrid->UpdateAllStatistics();

	// Fin du mode update
	if (dataGrid->GetCellUpdateMode())
	{
		dataGrid->SetCellUpdateMode(false);
		dataGrid->DeleteIndexingStructure();
	}

	// Test final si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Nettoyage de la base pour detruire les eventuels objets non traites
	database->DeleteAll();

	// Fin de suivi de tache
	TaskProgression::EndTask();

	ensure(database->GetObjects()->GetSize() == 0);
	ensure(not bOk or dataGrid->Check());
	return bOk;
}

void CCCoclusteringBuilder::RemoveTupleWithMissingContinuousValue(KWTupleTable* tupleTable, Continuous cValue) const
{
	int i;
	int nFoundIndex;

	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNumber() == 1);
	require(tupleTable->GetAttributeTypeAt(0) == KWType::Continuous);

	// Recherche de l'index du tuple comportant la valeur donnee
	nFoundIndex = -1;
	for (i = 0; i < tupleTable->GetSize(); i++)
	{
		if (tupleTable->GetAt(i)->GetContinuousAt(0) == cValue)
		{
			nFoundIndex = i;
			break;
		}
	}

	// Destruction du tupe si trouve
	if (nFoundIndex != -1)
		tupleTable->DeleteAt(nFoundIndex);
}

void CCCoclusteringBuilder::RemoveTupleWithMissingSymbolValue(KWTupleTable* tupleTable, Symbol sValue) const
{
	int i;
	int nFoundIndex;

	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNumber() == 1);
	require(tupleTable->GetAttributeTypeAt(0) == KWType::Symbol);

	// Recherche de l'index du tuple comportant la valeur donnee
	nFoundIndex = -1;
	for (i = 0; i < tupleTable->GetSize(); i++)
	{
		if (tupleTable->GetAt(i)->GetSymbolAt(0) == sValue)
		{
			nFoundIndex = i;
			break;
		}
	}

	// Destruction du tupe si trouve
	if (nFoundIndex != -1)
		tupleTable->DeleteAt(nFoundIndex);
}

boolean CCCoclusteringBuilder::ReadDatabaseAndCheckRemainingMemory(KWDatabase* database) const
{
	boolean bOk;
	longint lInitialAvailableRemainingMemory;
	longint lAvailableRemainingMemory;
	longint lDatabaseObjectsUsedMemory;
	longint lDataGridInitializationNecessaryMemory;

	require(database != NULL);
	require(database->GetObjects()->GetSize() == 0);

	// Memoire disponible initiale, avant le debut de la lecture
	lInitialAvailableRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Lecture de la base
	bOk = database->ReadAll();

	// Estimation de la memoire necessaire pour construire la grille initiale
	if (bOk)
	{
		// Memoire utilisee pour les objet lus
		lAvailableRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();
		lDatabaseObjectsUsedMemory = lInitialAvailableRemainingMemory - lAvailableRemainingMemory;

		// On estime de facon heuristique qu'il faut au moins la meme quantite de memoire pour constuire la grille initiale
		// Estimation tres heuristique, mais raisonnable en pratique
		lDataGridInitializationNecessaryMemory = lDatabaseObjectsUsedMemory;

		// Erreur si pas assez de memoire
		if (lAvailableRemainingMemory < lDataGridInitializationNecessaryMemory)
		{
			bOk = false;
			AddError("Not enough memory to initialize coclustering optimization data" +
				 RMResourceManager::BuildMissingMemoryMessage(lDataGridInitializationNecessaryMemory));
			AddSimpleMessage(RMResourceManager::BuildMemoryLimitMessage());
		}
	}
	ensure(bOk or database->GetObjects()->GetSize() == 0);
	return bOk;
}

longint CCCoclusteringBuilder::ComputeDatabaseTotalRemainingValueNumber(KWDatabase* database,
									int nStartObjectIndex) const
{
	longint lTotalValueNumber;
	int nObject;
	KWObject* kwoObject;
	KWAttributeBlock* attributeBlock;
	KWContinuousValueBlock* cvbContinuousValues;
	KWSymbolValueBlock* svbSymbolValues;
	int n;

	require(database != NULL);
	require(database->GetObjects()->GetSize() > 0);
	require(0 <= nStartObjectIndex and nStartObjectIndex < database->GetObjects()->GetSize());
	require(database->GetClassName() == GetClass()->GetName());

	// Initialisation avec les valeurs dense
	lTotalValueNumber = (longint)(database->GetObjects()->GetSize() - nStartObjectIndex) *
			    GetClass()->GetLoadedDenseAttributeNumber();

	// Parcours des objet de la base
	for (nObject = nStartObjectIndex; nObject < database->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, database->GetObjects()->GetAt(nObject));
		assert(kwoObject != NULL);

		// Suivi des interruptions utilisateurs
		if (TaskProgression::IsRefreshNecessary(nObject))
		{
			if (TaskProgression::IsInterruptionRequested())
				break;
		}

		// Parcours des blocs d'attribut sparse de l'objet pour creer une cellule par valeur present de chaque bloc
		for (n = 0; n < GetClass()->GetLoadedAttributeBlockNumber(); n++)
		{
			attributeBlock = GetClass()->GetLoadedAttributeBlockAt(n);

			// Cas d'un bloc Continuous
			if (attributeBlock->GetType() == KWType::Continuous)
			{
				cvbContinuousValues =
				    kwoObject->GetContinuousValueBlockAt(attributeBlock->GetLoadIndex());
				lTotalValueNumber += cvbContinuousValues->GetValueNumber();
			}
			// Cas d'un bloc Symbol
			else
			{
				assert(attributeBlock->GetType() == KWType::Symbol);
				svbSymbolValues = kwoObject->GetSymbolValueBlockAt(attributeBlock->GetLoadIndex());
				lTotalValueNumber += svbSymbolValues->GetValueNumber();
			}
		}
	}

	// Test final si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		lTotalValueNumber = 0;
	return lTotalValueNumber;
}

longint CCCoclusteringBuilder::ComputeDatabaseBlockValueNumber(KWDatabase* database,
							       KWAttributeBlock* attributeBlock) const
{
	longint lBlockValueNumber;
	int nObject;
	KWObject* kwoObject;
	KWContinuousValueBlock* cvbContinuousValues;
	KWSymbolValueBlock* svbSymbolValues;

	require(database != NULL);
	require(database->GetObjects()->GetSize() > 0);
	require(database->GetClassName() == GetClass()->GetName());
	require(attributeBlock != NULL);
	require(GetClass()->LookupAttributeBlock(attributeBlock->GetName()) == attributeBlock);

	// Parcours des objet de la base
	lBlockValueNumber = 0;
	for (nObject = 0; nObject < database->GetObjects()->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, database->GetObjects()->GetAt(nObject));
		assert(kwoObject != NULL);

		// Suivi des interruptions utilisateurs
		if (TaskProgression::IsRefreshNecessary(nObject))
		{
			if (TaskProgression::IsInterruptionRequested())
				break;
		}

		// Cas d'un bloc Continuous
		if (attributeBlock->GetType() == KWType::Continuous)
		{
			cvbContinuousValues = kwoObject->GetContinuousValueBlockAt(attributeBlock->GetLoadIndex());
			lBlockValueNumber += cvbContinuousValues->GetValueNumber();
		}
		// Cas d'un bloc Symbol
		else
		{
			assert(attributeBlock->GetType() == KWType::Symbol);
			svbSymbolValues = kwoObject->GetSymbolValueBlockAt(attributeBlock->GetLoadIndex());
			lBlockValueNumber += svbSymbolValues->GetValueNumber();
		}
	}

	// Test final si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		lBlockValueNumber = 0;
	return lBlockValueNumber;
}

boolean CCCoclusteringBuilder::CheckMemoryForStandardDataGridInitialization(const KWTupleTable* tupleTable) const
{
	boolean bOk = true;
	const boolean bTrace = false;
	int nTotalFrequency;
	int nCellNumber;
	longint lAvailableMemory;
	longint lNecessaryMemory;
	ALString sAttributeName;
	int nAttributeType;
	longint lInitialDataGridSize;
	longint lDataGridIndexingSize;
	longint lDataGridSize;
	longint lAttributeSize;
	longint lPartSize;
	longint lValueSize;
	longint lCellSize;
	int nValueNumber;
	int nExpectedMaxPartNumber;
	int nAttribute;
	KWDescriptiveStats* descriptiveStats;

	require(not GetVarPartCoclustering());
	require(tupleTable != NULL);
	require(tupleTable->GetTotalFrequency() > 0);
	require(not tupleTable->GetUpdateMode());
	require(odDescriptiveStats.GetCount() == tupleTable->GetAttributeNumber());
	require(odDescriptiveStats.GetCount() ==
		GetClass()->GetLoadedAttributeNumber() - (GetFrequencyAttributeName() == "" ? 0 : 1));

	// Calcul des caracteristiques memoire disponibles (le fichier est lu a ce moment)
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Extraction des informations d ela ytables de tuple
	nTotalFrequency = tupleTable->GetTotalFrequency();
	nCellNumber = tupleTable->GetSize();

	// Initialisation avec la taille de la grille a vide
	lDataGridSize = sizeof(KWDataGrid) + sizeof(void*);
	lInitialDataGridSize = lDataGridSize;

	// Parcours des attributs et de leurs valeurs pour initialiser le plus finement possible chacuqne des grille
	lDataGridIndexingSize = 0;
	for (nAttribute = 0; nAttribute < tupleTable->GetAttributeNumber(); nAttribute++)
	{
		sAttributeName = tupleTable->GetAttributeNameAt(nAttribute);
		nAttributeType = tupleTable->GetAttributeTypeAt(nAttribute);

		// Prise en compte de la taille de stockage de l'attribut
		lAttributeSize = sizeof(KWDGAttribute) + sizeof(void*) + sAttributeName.GetLength();
		lInitialDataGridSize += lAttributeSize;

		// Nombre de valeur de l'attribut si ses statistiques descriptives sont disponible
		descriptiveStats = cast(KWDescriptiveStats*, odDescriptiveStats.Lookup(sAttributeName));
		assert(descriptiveStats != NULL);
		nValueNumber = descriptiveStats->GetValueNumber();

		// Nombre de parties max attendu dans un modele optimise
		nExpectedMaxPartNumber = (int)pow(nTotalFrequency, 1.0 / tupleTable->GetAttributeNumber());
		nExpectedMaxPartNumber = min(nExpectedMaxPartNumber, nValueNumber);

		// Prise en compte de la taille de stockage des parties
		if (nAttributeType == KWType::Continuous)
			lPartSize = sizeof(KWDGMPart) + sizeof(KWDGInterval) + sizeof(void*);
		else
			lPartSize = sizeof(KWDGMPart) + sizeof(KWDGValueSet) + sizeof(void*);
		lInitialDataGridSize += nValueNumber * lPartSize;

		// Prise en compte de la taille de stockage des valeurs
		if (nAttributeType == KWType::Continuous)
			lValueSize = 0;
		else
			lValueSize = sizeof(KWDGValue) + sizeof(void*);
		lInitialDataGridSize += nValueNumber * lValueSize;

		// Prise en compte de la structure d'indexatuion
		lDataGridIndexingSize += nValueNumber * KWDGAttribute::GetUsedMemoryPerIndexingElement(nAttributeType);
	}

	// Prise en compte des cellules
	lCellSize = sizeof(KWDGMCell) + ((longint)2 + GetClass()->GetLoadedAttributeNumber()) * sizeof(void*);
	lInitialDataGridSize += nCellNumber * lCellSize;

	// Memoire necessaire totale
	lNecessaryMemory = lInitialDataGridSize + lDataGridIndexingSize;

	// Affichage de stats memoire
	if (bTrace)
	{
		cout << "CheckMemoryForStandardDataGridInitialization\n";
		cout << "\tStats\n";
		cout << "\t\tTuples\t" << IntToString(tupleTable->GetSize());
		cout << "\t\tAttributes\t" << IntToString(tupleTable->GetAttributeNumber());
		cout << "\t\tTotal frequency\t" << IntToString(tupleTable->GetTotalFrequency());
		cout << "\tElement sizes\n";
		cout << "\t\tData grid\t" << sizeof(KWDataGrid) << "\n";
		cout << "\t\tAttribute\t" << sizeof(KWDGAttribute) << "\n";
		cout << "\t\tPart\t" << sizeof(KWDGPart) << "\n";
		cout << "\t\tInterval\t" << sizeof(KWDGInterval) << "\n";
		cout << "\t\tValue set\t" << sizeof(KWDGValueSet) << "\n";
		cout << "\t\tSymbol value\t" << sizeof(KWDGSymbolValue) << "\n";
		cout << "\t\tCell\t" << sizeof(KWDGCell) << "\t" << sizeof(KWDGMCell) << "\n";
		cout << "Detail requirements\n";
		cout << "\t\tInitial data grid\t" << LongintToHumanReadableString(lInitialDataGridSize) << "\n";
		cout << "\t\tInitial data grid indexing\t" << LongintToHumanReadableString(lDataGridIndexingSize)
		     << "\n";
		cout << "Synthesis with logical memory\n";
		cout << "\t\tNecessary\t" << LongintToHumanReadableString(lNecessaryMemory) << "\n";
		cout << "\t\tAvailable\t" << LongintToHumanReadableString(lAvailableMemory) << "\n";
		cout << "\t\tOK\t" << BooleanToString(lNecessaryMemory <= lAvailableMemory) << "\n";
		cout << "Synthesis with physical memory\n";
		cout << "\t\tUsed\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\n";
		cout << "\t\tNecessary\t" << RMResourceManager::ActualMemoryToString(lNecessaryMemory) << "\n";
		cout << "\t\tAvailable\t" << RMResourceManager::ActualMemoryToString(lAvailableMemory) << "\n";
	}

	// Test si memoire suffisante
	if (lNecessaryMemory > lAvailableMemory)
	{
		AddError("Not enough memory to generate coclustering optimization data" +
			 RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
		AddSimpleMessage(RMResourceManager::BuildMemoryLimitMessage());
		if (RMResourceConstraints::GetIgnoreMemoryLimit())
			RMResourceManager::DisplayIgnoreMemoryLimitMessage();
		else
			bOk = false;
	}
	return bOk;
}

boolean CCCoclusteringBuilder::CheckMemoryForDataGridOptimization(KWDataGrid* inputInitialDataGrid) const
{
	boolean bOk = true;
	const boolean bTrace = false;
	int nTotalFrequency;
	int nCellNumber;
	longint lAvailableMemory;
	longint lNecessaryMemory;
	ALString sAttributeName;
	int nAttributeType;
	int nInstanceNumber;
	longint lNullDataGridSize;
	longint lWorkingDataGridSize;
	longint lOptimizedDataGridSize;
	longint lDataGridSize;
	longint lAttributeSize;
	longint lPartSize;
	longint lValueSize;
	longint lCellSize;
	longint lDataGridIndexingSize;
	longint lDataGridPostOptimizationSize;
	int nValueNumber;
	int nInitialMaxPartNumber;
	int nExpectedMaxPartNumber;
	int nMergeNumber;
	int nAttribute;
	KWDescriptiveStats* descriptiveStats;
	KWDGAttribute* dgAttribute;

	require(inputInitialDataGrid != NULL);
	require(inputInitialDataGrid->Check());
	require(GetDatabase()->GetObjects()->GetSize() == 0);
	require(odDescriptiveStats.GetCount() ==
		GetClass()->GetLoadedAttributeNumber() - (GetFrequencyAttributeName() == "" ? 0 : 1));

	// Calcul des caracteristiques memoire disponibles (le fichier est lu a ce moment)
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Extraction des informations d ela ytables de tuple
	nTotalFrequency = inputInitialDataGrid->GetGridFrequency();
	nCellNumber = inputInitialDataGrid->GetCellNumber();

	// Initialisation avec la taille de la grille a vide
	lDataGridSize = sizeof(KWDataGrid) + sizeof(void*);
	lNullDataGridSize = lDataGridSize;
	lWorkingDataGridSize = lDataGridSize + sizeof(KWDataGridMerger) - sizeof(KWDataGrid);
	lOptimizedDataGridSize = lDataGridSize;

	// Parcours des attributs et de leurs valeurs pour initialiser le plus finement possible chacuqne des grille
	nInitialMaxPartNumber = 1;
	for (nAttribute = 0; nAttribute < inputInitialDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = inputInitialDataGrid->GetAttributeAt(nAttribute);
		sAttributeName = dgAttribute->GetAttributeName();
		nAttributeType = dgAttribute->GetAttributeType();

		// Prise en compte de la taille de stockage de l'attribut
		lAttributeSize = sizeof(KWDGAttribute) + sizeof(void*) + sAttributeName.GetLength();
		lNullDataGridSize += lAttributeSize;
		lWorkingDataGridSize += lAttributeSize + sizeof(KWDGMAttribute) - sizeof(KWDGAttribute);
		lOptimizedDataGridSize += lAttributeSize;

		// Cas d'un attribut de type partie de variable
		// C'est un cas particulier, car le nombre de parties et de valeur depend
		// de la partition des variables internes
		if (nAttributeType == KWType::VarPart)
		{
			// Nombre d'instances, issue de la premiere variable
			assert(inputInitialDataGrid->GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
			descriptiveStats = cast(
			    KWDescriptiveStats*,
			    odDescriptiveStats.Lookup(inputInitialDataGrid->GetAttributeAt(0)->GetAttributeName()));
			assert(descriptiveStats != NULL);
			nInstanceNumber = descriptiveStats->GetValueNumber();

			// Nombre de parties max attendu dans un modele optimise pour la variable
			// On a au max le nombre d'individu issu du premier attribut de la grille
			nExpectedMaxPartNumber =
			    (int)pow(nTotalFrequency, 1.0 / inputInitialDataGrid->GetAttributeNumber());
			nExpectedMaxPartNumber = min(nExpectedMaxPartNumber, nInstanceNumber);
			nExpectedMaxPartNumber = min(nExpectedMaxPartNumber, dgAttribute->GetInitialValueNumber());

			// Nombre max de parties initiales sur l'ensemble des variables
			// On se base sur un nombre de VarPart attendu maximum
			nInitialMaxPartNumber = max(nInitialMaxPartNumber, nExpectedMaxPartNumber);

			// Prise en compte de la taille de stockage des parties
			lPartSize = sizeof(KWDGMPart) + sizeof(KWDGVarPartSet) + sizeof(void*);
			lNullDataGridSize += lPartSize;
			lWorkingDataGridSize +=
			    nExpectedMaxPartNumber * (lPartSize + sizeof(KWDGMPart) - sizeof(KWDGPart));
			lOptimizedDataGridSize += nExpectedMaxPartNumber * lPartSize;

			// Prise en compte de la taille de stockage des valeurs
			// Le nombre de valeur est celui du nombre d'attributs internes dans le cas du modele nul,
			// et celui du nombre max de VarPart attendu pour un modele de travail
			lValueSize = sizeof(KWDGVarPartValue) + sizeof(void*);
			lNullDataGridSize +=
			    inputInitialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber() * lValueSize;
			lWorkingDataGridSize += nExpectedMaxPartNumber * lValueSize;
			lOptimizedDataGridSize += nExpectedMaxPartNumber * lValueSize;
		}
		// Cas d'un attribut standard
		else
		{
			// Nombre de valeurs de l'attribut
			descriptiveStats = cast(KWDescriptiveStats*, odDescriptiveStats.Lookup(sAttributeName));
			assert(descriptiveStats != NULL);
			nValueNumber = descriptiveStats->GetValueNumber();

			// Nombre de parties max attendu dans un modele optimise pour la variable
			nExpectedMaxPartNumber =
			    (int)pow(nTotalFrequency, 1.0 / inputInitialDataGrid->GetAttributeNumber());
			nExpectedMaxPartNumber = min(nExpectedMaxPartNumber, nValueNumber);

			// Nombre max de parties initiales sur l'ensemble des variables
			nInitialMaxPartNumber = max(nInitialMaxPartNumber,
						    inputInitialDataGrid->GetAttributeAt(nAttribute)->GetPartNumber());

			// Prise en compte de la taille de stockage des parties
			if (nAttributeType == KWType::Continuous)
				lPartSize = sizeof(KWDGMPart) + sizeof(KWDGInterval) + sizeof(void*);
			else
				lPartSize = sizeof(KWDGMPart) + sizeof(KWDGSymbolValueSet) + sizeof(void*);
			lNullDataGridSize += lPartSize;
			lWorkingDataGridSize +=
			    nExpectedMaxPartNumber * (lPartSize + sizeof(KWDGMPart) - sizeof(KWDGPart));
			lOptimizedDataGridSize += nExpectedMaxPartNumber * lPartSize;

			// Prise en compte de la taille de stockage des valeurs
			if (nAttributeType == KWType::Continuous)
				lValueSize = 0;
			else
				lValueSize = sizeof(KWDGSymbolValue) + sizeof(void*);
			lNullDataGridSize += nValueNumber * lValueSize;
			lWorkingDataGridSize += nValueNumber * lValueSize;
			lOptimizedDataGridSize += nValueNumber * lValueSize;
		}

		// Prise en compte du nombre de merges pour la grille de travail
		if (nAttributeType == KWType::Continuous)
			nMergeNumber = nExpectedMaxPartNumber - 1;
		else
			nMergeNumber = nExpectedMaxPartNumber * (nExpectedMaxPartNumber - 1) / 2;
		lWorkingDataGridSize += nMergeNumber * sizeof(KWDGMPartMerge);
	}

	// Pris en compte des attribut internes dans la cas instance x variables
	if (initialDataGrid->IsVarPartDataGrid())
	{
		// Parcours des attributs internes pour prendre en compte la taille des attribut et de leurs valeurs
		for (nAttribute = 0; nAttribute < inputInitialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber();
		     nAttribute++)
		{
			dgAttribute = inputInitialDataGrid->GetInnerAttributes()->GetInnerAttributeAt(nAttribute);
			sAttributeName = dgAttribute->GetAttributeName();
			nAttributeType = dgAttribute->GetAttributeType();
			assert(KWType::IsSimple(nAttributeType));

			// Pris en compte de l'attribut
			lAttributeSize = sizeof(KWDGAttribute) + sizeof(void*) + sAttributeName.GetLength();
			lNullDataGridSize += lAttributeSize;
			lWorkingDataGridSize += lAttributeSize + sizeof(KWDGMAttribute) - sizeof(KWDGAttribute);
			lOptimizedDataGridSize += lAttributeSize;

			// Nombre de valeurs de l'attribut
			descriptiveStats = cast(KWDescriptiveStats*, odDescriptiveStats.Lookup(sAttributeName));
			assert(descriptiveStats != NULL);
			nValueNumber = descriptiveStats->GetValueNumber();

			// Prise en compte de la taille de stockage des valeurs
			if (nAttributeType == KWType::Continuous)
				lValueSize = 0;
			else
				lValueSize = sizeof(KWDGSymbolValue) + sizeof(void*);
			lNullDataGridSize += nValueNumber * lValueSize;
			lWorkingDataGridSize += nValueNumber * lValueSize;
			lOptimizedDataGridSize += nValueNumber * lValueSize;
		}

		// Nombre d'instances, issue de la premiere variable
		assert(inputInitialDataGrid->GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
		descriptiveStats =
		    cast(KWDescriptiveStats*,
			 odDescriptiveStats.Lookup(inputInitialDataGrid->GetAttributeAt(0)->GetAttributeName()));
		assert(descriptiveStats != NULL);
		nInstanceNumber = descriptiveStats->GetValueNumber();

		// Nombre de parties max attendu dans un modele optimise pour la variable
		// On a au max le nombre d'individu issu du premier attribut de la grille
		nExpectedMaxPartNumber = (int)pow(nTotalFrequency, 1.0 / inputInitialDataGrid->GetAttributeNumber());
		nExpectedMaxPartNumber = min(nExpectedMaxPartNumber, nInstanceNumber);
		nExpectedMaxPartNumber =
		    min(nExpectedMaxPartNumber, inputInitialDataGrid->GetVarPartAttribute()->GetInitialValueNumber());

		// Prise en compte de la taille de stockage des parties numerique ou categorielles
		lPartSize = sizeof(KWDGMPart) + max(sizeof(KWDGInterval), sizeof(KWDGSymbolValueSet)) + sizeof(void*);
		lNullDataGridSize += inputInitialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber() * lPartSize;
		lWorkingDataGridSize += nExpectedMaxPartNumber * lPartSize;
		lOptimizedDataGridSize += nExpectedMaxPartNumber * lPartSize;
	}

	// Prise en compte des cellules
	lCellSize = sizeof(KWDGCell) + (2 + initialDataGrid->GetAttributeNumber()) * sizeof(void*);
	lNullDataGridSize += lCellSize;
	lWorkingDataGridSize += nCellNumber * (lCellSize + sizeof(KWDGMCell) - sizeof(KWDGCell));
	lOptimizedDataGridSize += nCellNumber * lCellSize;

	// Memoire necessaire totale pour l'ensemble des grilles
	lNecessaryMemory = lNullDataGridSize + lWorkingDataGridSize + lOptimizedDataGridSize;

	// Prise en compte de la memoire de travail pour l'indexation de la grille
	lDataGridIndexingSize = inputInitialDataGrid->ComputeNecessaryMemoryForIndexingStructure();
	lNecessaryMemory += lDataGridIndexingSize;

	// Prise en compte de la memoire de travail pour post-optimisation
	lDataGridPostOptimizationSize =
	    inputInitialDataGrid->GetCellNumber() *
	    (sizeof(KWDGMCell) + (2 + inputInitialDataGrid->GetAttributeNumber()) * sizeof(void*));
	lDataGridPostOptimizationSize += inputInitialDataGrid->GetCellNumber() * sizeof(KWDGPOCellFrequencyVector);
	lDataGridPostOptimizationSize += nInitialMaxPartNumber * (sizeof(KWMODLLineDeepOptimization) + 2 * sizeof(int) +
								  sizeof(KWDGPOPartFrequencyVector));
	lNecessaryMemory += lDataGridPostOptimizationSize;

	// Affichage de stats memoire
	if (bTrace)
	{
		cout << "CheckMemoryForDataGridOptimization\t" << initialDataGrid->GetObjectLabel() << "\n";
		cout << "\tCell number\t" << initialDataGrid->GetCellNumber() << "\n";
		cout << "\tElement sizes\n";
		cout << "\t\tData grid\t" << sizeof(KWDataGrid) << "\t" << sizeof(KWDataGridMerger) << "\n";
		cout << "\t\tAttribute\t" << sizeof(KWDGAttribute) << "\t" << sizeof(KWDGMAttribute) << "\n";
		cout << "\t\tPart\t" << sizeof(KWDGPart) << "\t" << sizeof(KWDGMPart) << "\t" << sizeof(KWDGMPartMerge)
		     << "\n";
		cout << "\t\tInterval\t" << sizeof(KWDGInterval) << "\n";
		cout << "\t\tValue set\t" << sizeof(KWDGValueSet) << "\n";
		cout << "\t\tVarPart set\t" << sizeof(KWDGVarPartSet) << "\n";
		cout << "\t\tSymbol value\t" << sizeof(KWDGSymbolValue) << "\n";
		cout << "\t\tVarPart value\t" << sizeof(KWDGVarPartValue) << "\n";
		cout << "\t\tCell\t" << sizeof(KWDGCell) << "\t" << sizeof(KWDGMCell) << "\n";
		cout << "Detail requirements\n";
		cout << "\t\tNull data grid\t" << LongintToHumanReadableString(lNullDataGridSize) << "\n";
		cout << "\t\tWorking data grid\t" << LongintToHumanReadableString(lWorkingDataGridSize) << "\n";
		cout << "\t\tOptimized data grid\t" << LongintToHumanReadableString(lOptimizedDataGridSize) << "\n";
		cout << "\t\tData grid indexing\t" << LongintToHumanReadableString(lDataGridIndexingSize) << "\n";
		cout << "\t\tData grid post-optimization\t"
		     << LongintToHumanReadableString(lDataGridPostOptimizationSize) << "\n";
		cout << "Synthesis with logical memory\n";
		cout << "\t\tNecessary\t" << LongintToHumanReadableString(lNecessaryMemory) << "\n";
		cout << "\t\tAvailable\t" << LongintToHumanReadableString(lAvailableMemory) << "\n";
		cout << "\t\tOK\t" << BooleanToString(lNecessaryMemory <= lAvailableMemory) << "\n";
		cout << "Synthesis with physical memory\n";
		cout << "\t\tUsed\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\n";
		cout << "\t\tNecessary\t" << RMResourceManager::ActualMemoryToString(lNecessaryMemory) << "\n";
		cout << "\t\tAvailable\t" << RMResourceManager::ActualMemoryToString(lAvailableMemory) << "\n";
	}

	// Test si memoire suffisante
	if (lNecessaryMemory > lAvailableMemory)
	{
		AddError("Not enough memory to generate coclustering optimization data" +
			 RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
		AddSimpleMessage(RMResourceManager::BuildMemoryLimitMessage());
		if (RMResourceConstraints::GetIgnoreMemoryLimit())
			RMResourceManager::DisplayIgnoreMemoryLimitMessage();
		else
			bOk = false;
	}
	return bOk;
}

void CCCoclusteringBuilder::AnyTimeStart() const
{
	// Initialisations
	nAnyTimeOptimizationIndex = 0;
	dAnyTimeBestCost = 0;
	tAnyTimeTimer.Reset();
	tAnyTimeTimer.Start();
	sLastActualAnyTimeReportFileName = "";
}

void CCCoclusteringBuilder::AnyTimeStop() const
{
	ALString sReportFileName;

	// Initialisations (sauf nom du dernier fichier temporaire, potentiellement a detruire)
	nAnyTimeOptimizationIndex = 0;
	dAnyTimeBestCost = 0;
	tAnyTimeTimer.Stop();
}

const ALString CCCoclusteringBuilder::AnyTimeBuildTemporaryReportFileName(int nIndex) const
{
	ALString sTemporaryReportFileName;
	ALString sPathName;
	ALString sFilePrefix;
	ALString sFileSuffix;

	require(GetReportFileName() != "");
	require(nIndex >= 1);

	// Extraction des partie du nom du fichier
	sPathName = FileService::GetPathName(GetReportFileName());
	sFilePrefix = FileService::GetFilePrefix(GetReportFileName());
	sFileSuffix = FileService::GetFileSuffix(GetReportFileName());

	// Construction d'un nom de fichier en le suffixant par l'index
	sTemporaryReportFileName = FileService::BuildFilePathName(
	    sPathName, FileService::BuildFileName(sFilePrefix + "(" + IntToString(nIndex) + ")", sFileSuffix));
	return sTemporaryReportFileName;
}

void CCCoclusteringBuilder::CleanCoclusteringResults()
{
	// Nettoyage de la grille et de sa structure de cout
	if (coclusteringDataGrid != NULL)
		delete coclusteringDataGrid;
	if (initialDataGrid != NULL)
		delete initialDataGrid;
	if (coclusteringDataGridCosts != NULL)
		delete coclusteringDataGridCosts;
	coclusteringDataGrid = NULL;
	initialDataGrid = NULL;
	coclusteringDataGridCosts = NULL;
	odDescriptiveStats.DeleteAll();
}

boolean CCCoclusteringBuilder::ComputeDescriptiveAttributeStats(const KWTupleTable* tupleTable,
								ObjectDictionary* odOutputDescriptiveStats) const
{
	boolean bOk = true;
	int nAttribute;
	KWAttribute* attribute;
	KWDescriptiveStats* descriptiveStats;
	KWTupleTable univariateTupleTable;

	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(odOutputDescriptiveStats != NULL);
	require(odOutputDescriptiveStats->GetCount() == 0);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Compute univariate descriptive stats");

	// Calcul des stats descriptives par attribut
	for (nAttribute = 0; nAttribute < GetClass()->GetLoadedAttributeNumber(); nAttribute++)
	{
		attribute = GetClass()->GetLoadedAttributeAt(nAttribute);

		// Suivi de tache
		TaskProgression::DisplayProgression((nAttribute + 1) * 100 / GetClass()->GetLoadedAttributeNumber());
		TaskProgression::DisplayLabel(attribute->GetName());
		if (TaskProgression::IsInterruptionRequested())
		{
			bOk = false;
			break;
		}

		// Cas des attributs simples, hors attribut d'effectif
		if (attribute->GetName() != GetFrequencyAttributeName() and KWType::IsSimple(attribute->GetType()) and
		    not TaskProgression::IsInterruptionRequested())
		{
			// Creation d'un objet de stats pour l'attribut selon son type
			if (attribute->GetType() == KWType::Continuous)
				descriptiveStats = new KWDescriptiveContinuousStats;
			else
				descriptiveStats = new KWDescriptiveSymbolStats;

			// Initialisation
			descriptiveStats->SetLearningSpec(GetLearningSpec());
			descriptiveStats->SetAttributeName(attribute->GetName());

			// Creation d'une table de tuples univariee a partir de la table de tuples globale
			tupleTable->BuildUnivariateTupleTable(attribute->GetName(), &univariateTupleTable);

			// Calcul des stats
			descriptiveStats->ComputeStats(&univariateTupleTable);

			// Memorisation
			odOutputDescriptiveStats->SetAt(descriptiveStats->GetAttributeName(), descriptiveStats);
		}
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();

	// Test final si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	// Nettoyage si erreur
	if (not bOk)
		odOutputDescriptiveStats->DeleteAll();
	return bOk;
}

void CCCoclusteringBuilder::ComputeHierarchicalInfo(const KWDataGrid* inputInitialDataGrid,
						    const KWDataGridCosts* dataGridCosts,
						    CCHierarchicalDataGrid* optimizedDataGrid) const
{
	KWDataGridManager dataGridManager;
	KWDataGridMerger dataGridMerger;
	double dBestDataGridTotalCost;

	require(optimizedDataGrid != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGrid != NULL);

	// Memorisation des bornes des attributs Continuous
	ComputeContinuousAttributeBounds(optimizedDataGrid);

	// Calcul de la typicalite des attributs
	ComputeAttributeTypicalities(optimizedDataGrid);

	// Calcul de la typicalite des valeurs des attributs categoriels
	ComputeValueTypicalities(inputInitialDataGrid, dataGridCosts, optimizedDataGrid);

	////////////////////////////////////////////////////////////////////////////
	// Calcul de caracteristiques des parties du coclustering
	// en evaluant l'impact des fusions de groupes

	// Creation d'un KWDataGridMerger pour l'evaluation des fusions entre groupes du coclustering
	dataGridManager.ExportDataGrid(optimizedDataGrid, &dataGridMerger);

	// Initialisation de la structure de couts
	dataGridMerger.SetDataGridCosts(dataGridCosts);

	// Initialisation des couts des entites du DataGridMerger
	dataGridMerger.InitializeAllCosts();
	dBestDataGridTotalCost = dataGridMerger.GetDataGridCosts()->ComputeDataGridMergerTotalCost(&dataGridMerger);

	// Memorisation des couts
	optimizedDataGrid->SetCost(dBestDataGridTotalCost);
	optimizedDataGrid->SetNullCost(dataGridMerger.GetDataGridCosts()->GetTotalDefaultCost());

	// Memorisation du contexte d'apprentissage
	optimizedDataGrid->SetInitialAttributeNumber(inputInitialDataGrid->GetAttributeNumber());
	optimizedDataGrid->SetFrequencyAttributeName(GetFrequencyAttributeName());
	optimizedDataGrid->GetDatabaseSpec()->CopyFrom(GetDatabase());

	// Initialisation de la table de hash des cellules
	dataGridMerger.CellDictionaryInit();

	// Initialisation de toutes les fusions
	dataGridMerger.InitializeAllPartMerges();
	assert(dataGridMerger.CheckAllPartMerges());

	// Initialisation de la liste des parties triees par nombre de modalites
	dataGridMerger.InitializeAllPartLists();

	// Calcul de l'interet des parties
	ComputePartInterests(&dataGridMerger, dataGridCosts, optimizedDataGrid);

	// Calcul des hierarchies des parties, en creant de nouvelles parties pour les
	// coder les hierarchies
	ComputePartHierarchies(&dataGridMerger, dataGridCosts, optimizedDataGrid);

	// Calcul des rangs des parties
	ComputePartRanks(&dataGridMerger, dataGridCosts, optimizedDataGrid);

	// Tri des valeurs par typicalite decorissante pour les attributs categoriels
	SortAttributePartsAndValues(optimizedDataGrid);
}

void CCCoclusteringBuilder::ComputeAttributeTypicalities(CCHierarchicalDataGrid* optimizedDataGrid) const
{
	int nAttribute;
	CCHDGAttribute* hdgAttribute;
	int nInnerAttribute;
	CCHDGAttribute* innerAttribute;

	require(optimizedDataGrid != NULL);

	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		hdgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Memorisation du nombre de parties initiales, eagl ici au nombre de parties
		hdgAttribute->SetInitialPartNumber(hdgAttribute->GetPartNumber());

		// Pour l'instant, on met toutes les typicalites a 1
		hdgAttribute->SetInterest(1);

		// Cas d'un attribut VarPart : traitement de ses innerAttributes
		if (hdgAttribute->GetAttributeType() == KWType::VarPart and hdgAttribute->GetInnerAttributeNumber() > 0)
		{
			for (nInnerAttribute = 0; nInnerAttribute < hdgAttribute->GetInnerAttributeNumber();
			     nInnerAttribute++)
			{
				innerAttribute =
				    cast(CCHDGAttribute*, hdgAttribute->GetInnerAttributeAt(nInnerAttribute));

				// Memorisation du nombre de parties initiales, eagl ici au nombre de parties
				innerAttribute->SetInitialPartNumber(innerAttribute->GetPartNumber());

				// Pour l'instant, on met toutes les typicalites a 1
				innerAttribute->SetInterest(1);
			}
		}
	}
}

void CCCoclusteringBuilder::ComputeContinuousAttributeBounds(CCHierarchicalDataGrid* optimizedDataGrid) const
{
	int nAttribute;
	CCHDGAttribute* hdgAttribute;
	KWDescriptiveContinuousStats* descriptiveContinuousStats;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;

	require(optimizedDataGrid != NULL);

	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		hdgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Recherche des caracteristiques de l'attribut si numerique
		if (hdgAttribute->GetAttributeType() == KWType::Continuous)
		{
			descriptiveContinuousStats = cast(KWDescriptiveContinuousStats*,
							  odDescriptiveStats.Lookup(hdgAttribute->GetAttributeName()));

			// Memorisation de ses bornes
			if (descriptiveContinuousStats != NULL)
			{
				hdgAttribute->SetMin(descriptiveContinuousStats->GetMin());
				hdgAttribute->SetMax(descriptiveContinuousStats->GetMax());
			}
		}

		// Memorisation des informations sur les bornes des valeurs des attributs internes numeriques
		if (hdgAttribute->GetAttributeType() == KWType::VarPart and hdgAttribute->GetInnerAttributeNumber() > 0)
		{
			for (nInnerAttribute = 0; nInnerAttribute < hdgAttribute->GetInnerAttributeNumber();
			     nInnerAttribute++)
			{
				innerAttribute = hdgAttribute->GetInnerAttributeAt(nInnerAttribute);

				// Cas d'un attribut Continuous
				if (innerAttribute->GetAttributeType() == KWType::Continuous)
				{
					descriptiveContinuousStats =
					    cast(KWDescriptiveContinuousStats*,
						 odDescriptiveStats.Lookup(innerAttribute->GetAttributeName()));

					// Memorisation de ses bornes
					if (descriptiveContinuousStats != NULL)
					{
						cast(CCHDGAttribute*, innerAttribute)
						    ->SetMin(descriptiveContinuousStats->GetMin());
						cast(CCHDGAttribute*, innerAttribute)
						    ->SetMax(descriptiveContinuousStats->GetMax());
					}
				}
			}
		}
	}
}

void CCCoclusteringBuilder::ComputeValueTypicalities(const KWDataGrid* inputInitialDataGrid,
						     const KWDataGridCosts* dataGridCosts,
						     CCHierarchicalDataGrid* optimizedDataGrid) const
{
	int nAttribute;

	for (nAttribute = 0; nAttribute < optimizedDataGrid->GetAttributeNumber(); nAttribute++)
	{
		if (optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeType() == KWType::Symbol)
			ComputeValueTypicalitiesAt(inputInitialDataGrid, dataGridCosts, optimizedDataGrid, nAttribute);
	}
}

void CCCoclusteringBuilder::ComputeValueTypicalitiesAt(const KWDataGrid* inputInitialDataGrid,
						       const KWDataGridCosts* dataGridCosts,
						       CCHierarchicalDataGrid* optimizedDataGrid, int nAttribute) const
{
	boolean bTrace = false;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	KWDGAttribute* initialAttribute;
	KWDataGrid* univariateInitialDataGrid;
	KWDGPOGrouper dataGridUnivariateGrouper;
	KWDataGridUnivariateCosts* dataGridUnivariateCosts;
	KWDataGridManager dataGridManager;
	KWFrequencyTable initialFrequencyTable;
	IntVector ivGroups;
	int nGroupNumber;
	int nValueNumber;
	KWFrequencyTable groupedFrequencyTable;
	DoubleVector dvGroupCosts;
	int nValue;
	int nIntraCatchAllValue;
	int nGroup;
	double dOutDeltaCost;
	double dInDeltaCost;
	double dDeltaCost;
	int nOutGroup;
	double dTypicality;
	ObjectArray oaValueParts;
	ObjectArray oaGroupParts;
	KWDGPart* dgValuePart;
	KWDGValue* dgValue;
	NumericKeyDictionary nkdOptimizedAttributeValues;
	KWDGValue* dgOptimizedValue;
	DoubleVector dvTypicalities;
	DoubleVector dvMaxTypicalities;
	int nGarbageGroupIndex;
	int nGarbageModalityNumber;
	IntVector ivGroupModalityNumber;
	int nNewGarbageModalityNumber;
	int nNewGroupNumber;
	int nValueModalityNumber;
	double dElementaryTypicality;

	require(optimizedDataGrid != NULL);
	require(inputInitialDataGrid != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGrid->Check());
	require(inputInitialDataGrid->Check());
	require(inputInitialDataGrid->GetAttributeNumber() >= optimizedDataGrid->GetAttributeNumber());
	require(0 <= nAttribute and nAttribute < optimizedDataGrid->GetAttributeNumber());
	require(inputInitialDataGrid->SearchAttribute(
		    optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName()) != NULL);
	require(optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeType() == KWType::Symbol);

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Parametrage: on transforme les grilles en tableaux (KWFrequencyTable) pour l'attribut etudie
	// Chaque partie de l'attribut est ordonne de la meme facon dans la grille et le tableau
	// Methode fortement inspiree de KWDGPOGrouper::PostOptimizeDataGrid

	// Collecte des parties contenant les valeurs et les groupes
	initialAttribute =
	    inputInitialDataGrid->SearchAttribute(optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());
	initialAttribute->ExportParts(&oaValueParts);
	optimizedDataGrid->GetAttributeAt(nAttribute)->ExportParts(&oaGroupParts);

	// Construction d'une grille initiale pour l'optimisation univariee
	univariateInitialDataGrid = dataGridPostOptimizer.BuildUnivariateInitialDataGrid(
	    optimizedDataGrid, inputInitialDataGrid, optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());

	// Verification de la compatibilite entre grille optimisee et grille initiale
	assert(dataGridManager.CheckDataGrid(univariateInitialDataGrid, optimizedDataGrid));

	// Parametrage des couts d'optimisation univarie de la grille
	dataGridUnivariateGrouper.SetPostOptimizationAttributeName(
	    optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());
	dataGridUnivariateCosts = cast(KWDataGridUnivariateCosts*, dataGridUnivariateGrouper.GetGroupingCosts());
	dataGridUnivariateCosts->SetPostOptimizationAttributeName(
	    optimizedDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());
	dataGridUnivariateCosts->SetDataGridCosts(dataGridCosts);
	dataGridUnivariateCosts->InitializeUnivariateCostParameters(optimizedDataGrid);

	// Construction d'une table d'effectif selon l'attribut a post-optimiser, pour la grille initiale
	nValueNumber = univariateInitialDataGrid->GetAttributeAt(nAttribute)->GetPartNumber();
	dataGridUnivariateGrouper.InitializeFrequencyTableFromDataGrid(&initialFrequencyTable,
								       univariateInitialDataGrid);
	assert(initialFrequencyTable.GetFrequencyVectorNumber() == nValueNumber);

	// Initialisation des index de groupes et de l'index du groupe poubelle si present
	nGarbageGroupIndex = dataGridUnivariateGrouper.InitializeGroupIndexesAndGarbageIndex(
	    &ivGroups, univariateInitialDataGrid, optimizedDataGrid);
	nGarbageModalityNumber = 0;

	// Initialisation d'un tableau d'effectif groupe a partir d'une grille initiale et des index des groupes
	nGroupNumber = optimizedDataGrid->GetAttributeAt(nAttribute)->GetPartNumber();
	dataGridUnivariateGrouper.InitializeGroupedFrequencyTableFromDataGrid(
	    &groupedFrequencyTable, &initialFrequencyTable, &ivGroups, nGroupNumber);
	assert(groupedFrequencyTable.GetFrequencyVectorNumber() == nGroupNumber);

	// Memorisation des valeurs de l'attributs optimise dans un dictionnaire
	assert(nGroupNumber == oaGroupParts.GetSize());
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
	{
		dgValuePart = cast(KWDGPart*, oaGroupParts.GetAt(nGroup));
		assert(dgValuePart->GetAttribute() == optimizedDataGrid->GetAttributeAt(nAttribute));

		// Memorisation des valeurs
		dgValue = dgValuePart->GetValueSet()->GetHeadValue();
		while (dgValue != NULL)
		{
			nkdOptimizedAttributeValues.SetAt((dgValue->GetNumericKeyValue()), dgValue);
			dgValuePart->GetValueSet()->GetNextValue(dgValue);
		}

		// Memorisation du nombre de modalites du groupe
		ivGroupModalityNumber.Add(dgValuePart->GetValueSet()->GetValueNumber());

		// Cas du groupe poubelle : memorisation du nombre de modalites
		if (nGroup == nGarbageGroupIndex)
		{
			nGarbageModalityNumber = dgValuePart->GetValueSet()->GetValueNumber();
			// CH AB AF adaptation eventuelle VarPart
		}
	}

	// Tri du vecteur de nombre de modalites (tri croissant)
	ivGroupModalityNumber.Sort();

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Calcul des variations de cout lors de deplacement de valeur d'un groupe vers un autre
	// Methode fortement inspiree de KWGrouperMODL::FastPostOptimizeGroups

	// Initialisation des valeurs de groupes
	dvGroupCosts.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
	{
		dvGroupCosts.SetAt(nGroup, dataGridUnivariateGrouper.ComputeGroupCost(
					       groupedFrequencyTable.GetFrequencyVectorAt(nGroup)));
	}

	// Affichage de resultats: entete
	if (bTrace)
		cout << "\nValue\tOutGroup\tGroup\tOutDCost\tInDCost\tDCost\n";

	// Initialisation des typicites max
	dvMaxTypicalities.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		dvMaxTypicalities.SetAt(nGroup, -DBL_MAX);

	// Parcours de toutes les modalites
	// Il s'agit du parcours des modalites de la table initiale
	// Si la table initiale est issue d'une granularisation, il s'agit des modalites elementaires ou du fourre-tout
	// (super modalite) Dans le cas du fourre-tout, on calcule ici la typicite globale du fourre-tout en envisageant
	// son deplacement (deplacement de toutes les modalites du fourre-tout)
	dvTypicalities.SetSize(nValueNumber);
	for (nValue = 0; nValue < nValueNumber; nValue++)
	{
		// Recherche du groupe de rattachement de la modalite
		nOutGroup = ivGroups.GetAt(nValue);

		// Nombre de modalites associe : peut etre superieur a 1 dans le cas de la super modalite (fourre-tout)
		// Il doit alors s'agir de la derniere modalite
		nValueModalityNumber = initialFrequencyTable.GetFrequencyVectorAt(nValue)->GetModalityNumber();
		assert(nValueModalityNumber == 1 or nValue == nValueNumber - 1);

		// Calcul du cout du groupe apres le depart de la modalite,
		// en se basant sur les nouveaux effectifs du groupe
		dOutDeltaCost = dataGridUnivariateGrouper.ComputeGroupDiffCost(
		    groupedFrequencyTable.GetFrequencyVectorAt(nOutGroup),
		    initialFrequencyTable.GetFrequencyVectorAt(nValue));
		dOutDeltaCost -= dvGroupCosts.GetAt(nOutGroup);

		// Parcours des groupes cible potentiels
		dTypicality = 0;

		nNewGroupNumber = nGroupNumber;
		// Cas ou la modalite etait la seule de son groupe : la taille de la nouvelle partition est decrementee
		// de 1 : attention si nouveau nombre de groupes = 2, il ne peut pas y avoir de poubelle
		if (groupedFrequencyTable.GetFrequencyVectorAt(nOutGroup)->ComputeTotalFrequency() ==
		    initialFrequencyTable.GetFrequencyVectorAt(nValue)->ComputeTotalFrequency())
			nNewGroupNumber = nGroupNumber - 1;

		for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		{
			// On n'evalue que les nouveaux groupes potentiels
			if (nGroup != nOutGroup)
			{
				// Calcul du cout du groupe apres l'arrivee de la modalite,
				// en se basant sur les nouveaux effectifs du groupe
				dInDeltaCost = dataGridUnivariateGrouper.ComputeGroupUnionCost(
				    groupedFrequencyTable.GetFrequencyVectorAt(nGroup),
				    initialFrequencyTable.GetFrequencyVectorAt(nValue));
				dInDeltaCost -= dvGroupCosts.GetAt(nGroup);

				// Evaluation de la variation de cout globale
				dDeltaCost = dOutDeltaCost + dInDeltaCost;

				// Cas d'une nouvelle partition en deux groupes : partition obligatoirement sans groupe
				// poubelle
				if (nNewGroupNumber == 2)
					dDeltaCost +=
					    dataGridUnivariateGrouper.ComputePartitionCost(nNewGroupNumber, 0) -
					    dataGridUnivariateGrouper.ComputePartitionCost(nGroupNumber,
											   nGarbageModalityNumber);

				// Sinon, cas d'une partition avec groupe poubelle
				else if (nGarbageModalityNumber > 0)
				{
					nNewGarbageModalityNumber = nGarbageModalityNumber;
					// Cas ou la modalite part du groupe poubelle
					if (nOutGroup == nGarbageGroupIndex)
					{
						// Taille du groupe apres le depart de la modalite
						nNewGarbageModalityNumber =
						    nGarbageModalityNumber - nValueModalityNumber;
						// Est ce que le 2nd plus gros groupe devient le groupe poubelle ?
						if (nNewGarbageModalityNumber <
						    ivGroupModalityNumber.GetAt(ivGroupModalityNumber.GetSize() - 2))
							nNewGarbageModalityNumber = ivGroupModalityNumber.GetAt(
							    ivGroupModalityNumber.GetSize() - 2);
					}
					// Comparaison avec le nombre de modalites du groupe d'accueil
					if (nNewGarbageModalityNumber <
					    groupedFrequencyTable.GetFrequencyVectorAt(nGroup)->GetModalityNumber() +
						nValueModalityNumber)
						nNewGarbageModalityNumber =
						    groupedFrequencyTable.GetFrequencyVectorAt(nGroup)
							->GetModalityNumber() +
						    nValueModalityNumber;

					// Variation du cout de partition avec groupe poubelle
					dDeltaCost += dataGridUnivariateGrouper.ComputePartitionCost(
							  nNewGroupNumber, nNewGarbageModalityNumber) -
						      dataGridUnivariateGrouper.ComputePartitionCost(
							  nGroupNumber, nGarbageModalityNumber);
				}

				// CH IV Refactoring: nettoyer lignes suivantes?
				// CH IV Refactoring: maintenu pour l'instant afin d'identifier en Release les bases pour lesquelles on obtient un dDeltaCost < 0
				// Affichage en cas d'anomalie de dDeltaCost negatif
				if (bTrace and dDeltaCost < 0)
					cout << "ComputeValueTypicalities :: dDeltaCost < 0\t" << dDeltaCost << endl;

				// assert(dDeltaCost >= 0);

				// Cumul de la typicite
				dTypicality += dDeltaCost;

				// Affichage de resultats: ligne de detail
				if (bTrace)
					cout << oaValueParts.GetAt(nValue)->GetObjectLabel() << "\t"
					     << oaGroupParts.GetAt(nOutGroup)->GetObjectLabel() << "\t"
					     << oaGroupParts.GetAt(nGroup)->GetObjectLabel() << "\t" << dOutDeltaCost
					     << "\t" << dInDeltaCost << "\t" << dDeltaCost << endl;
			}
		}

		// Calcul de la typicite: variation de cout moyenne lorsque la modalite change de groupe
		if (nGroupNumber > 1)
			dTypicality /= nGroupNumber - 1;

		// Cas d'une modalite elementaire
		if (nValueModalityNumber == 1)
		{
			// Memorisation de la typicalite
			dvTypicalities.SetAt(nValue, dTypicality);

			// Mise a jour typicite max
			if (dTypicality > dvMaxTypicalities.GetAt(nOutGroup))
				dvMaxTypicalities.SetAt(nOutGroup, dTypicality);
		}
		// Sinon : cas du fourre-tout
		else
		{
			// Retaillage du vecteur des typicites qui devient vecteur des typicites par modalite
			// elementaire
			dvTypicalities.SetSize(dvTypicalities.GetSize() + nValueModalityNumber - 1);

			// Acces a la valeur
			dgValuePart = cast(KWDGPart*, oaValueParts.GetAt(nValue));

			dgValue = dgValuePart->GetValueSet()->GetHeadValue();

			// Repartition de la typicite du fourre-tout entre ses modalites elementaires
			for (nIntraCatchAllValue = 0; nIntraCatchAllValue < nValueModalityNumber; nIntraCatchAllValue++)
			{
				// Recherche de la valeur correspondante pour l'attribut optimise
				dgOptimizedValue = cast(
				    KWDGValue*, nkdOptimizedAttributeValues.Lookup((dgValue->GetNumericKeyValue())));

				// Calcul de la typicite elementaire = la typicite du fourre-tout * effectif  de la
				// modalite elementaire / effectif total du fourre-tout
				dElementaryTypicality = (dTypicality * dgOptimizedValue->GetValueFrequency()) /
							(1.0 * dgValuePart->GetPartFrequency());
				dvTypicalities.SetAt(nValue + nIntraCatchAllValue, dElementaryTypicality);

				// Mise a jour typicite max du groupe
				if (dElementaryTypicality > dvMaxTypicalities.GetAt(nOutGroup))
					dvMaxTypicalities.SetAt(nOutGroup, dElementaryTypicality);

				// Modalite suivante
				dgValuePart->GetValueSet()->GetNextValue(dgValue);
			}
		}
	}

	// Normalisation de typicite dans chaque groupe par la typicite max du groupe
	for (nValue = 0; nValue < nValueNumber; nValue++)
	{
		// Acces a la valeur
		dgValuePart = cast(KWDGPart*, oaValueParts.GetAt(nValue));
		assert(dgValuePart->GetValueSet()->GetValueNumber() == 1 or
		       // n'est plus garanti car la grille initiale contient un fourre-tout
		       //(dgValuePart->GetValueSet()->GetValueNumber() == 2 and
		       dgValuePart->GetValueSet()->GetTailValue()->GetSymbolValue() == Symbol::GetStarValue());

		// Acces au nombre de modalites de la valeur
		nValueModalityNumber = initialFrequencyTable.GetFrequencyVectorAt(nValue)->GetModalityNumber();

		// Recherche du groupe de rattachement de la modalite
		nOutGroup = ivGroups.GetAt(nValue);

		// Cas d'une modalite non fourre-tout
		if (dgValuePart->GetValueSet()->GetValueNumber() == 1)
		{
			dgValue = dgValuePart->GetValueSet()->GetHeadValue();

			// Recherche de la valeur correspondante pour l'attribut optimise
			dgOptimizedValue =
			    cast(KWDGValue*, nkdOptimizedAttributeValues.Lookup((dgValue->GetNumericKeyValue())));

			// Memorisation de la typicalite normalisee
			// Il se peut qu'une modalite ait une typicalite negative, si la post-optimisation des grille
			// n'a pas pu aller jusqu'a la convergence. Dans ce cas, on met une typicalite a 0, ce qui
			// indique que la modalite n'est pas a sap lace (de justesse) dans son cluster
			if (dvTypicalities.GetAt(nValue) <= 0)
				dgOptimizedValue->SetTypicality(0);
			// Cas general ou on normalise la typicalite par sa valeur max
			else
			{
				assert(dvMaxTypicalities.GetAt(nOutGroup) > 0);
				dgOptimizedValue->SetTypicality(dvTypicalities.GetAt(nValue) /
								dvMaxTypicalities.GetAt(nOutGroup));
			}
		}
		// Sinon : cas du fourre-tout
		else
		{
			dgValue = dgValuePart->GetValueSet()->GetHeadValue();

			for (nIntraCatchAllValue = 0; nIntraCatchAllValue < nValueModalityNumber; nIntraCatchAllValue++)
			{
				// Recherche de la valeur correspondante pour l'attribut optimise
				dgOptimizedValue = cast(
				    KWDGValue*, nkdOptimizedAttributeValues.Lookup((dgValue->GetNumericKeyValue())));

				// Memorisation de la typicalite normalisee dans le cas du fourre-tout
				if (dvTypicalities.GetAt(nValue + nIntraCatchAllValue) <= 0)
					dgOptimizedValue->SetTypicality(0);
				// Cas general ou on normalise la typicalite par sa valeur max
				else
				{
					assert(dvMaxTypicalities.GetAt(nOutGroup) > 0);
					dgOptimizedValue->SetTypicality(
					    dvTypicalities.GetAt(nValue + nIntraCatchAllValue) /
					    dvMaxTypicalities.GetAt(nOutGroup));
				}

				// Modalite suivante
				dgValuePart->GetValueSet()->GetNextValue(dgValue);
			}
		}
	}

	// Nettoyage
	delete univariateInitialDataGrid;
}

void CCCoclusteringBuilder::ComputePartInterests(const KWDataGridMerger* optimizedDataGridMerger,
						 const KWDataGridCosts* dataGridCosts,
						 CCHierarchicalDataGrid* optimizedDataGrid) const
{
	const boolean bTrace = false;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	CCHDGAttribute* hdgAttribute;
	int nPart;
	KWDGPart* part1;
	KWDGMPart* partM1;
	KWDGPart* part2;
	KWDGMPart* partM2;
	KWDGMPartMerge* partMerge;
	CCHDGPart* hdgPart;
	double dTotalDefaultCost;
	double dBestDataGridTotalCost;
	double dDeltaCost;
	double dInterest;
	double dTotalInterest;
	double dMaxInterest;
	DoubleVector dvInterests;

	require(optimizedDataGridMerger != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGridMerger->GetDataGridCosts() == dataGridCosts);
	require(optimizedDataGridMerger->CheckAllPartMerges());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGridMerger->GetAttributeNumber() == optimizedDataGrid->GetAttributeNumber());

	// Cout par defaut et meilleur cout
	dTotalDefaultCost = dataGridCosts->GetTotalDefaultCost();
	dBestDataGridTotalCost =
	    optimizedDataGridMerger->GetDataGridCosts()->ComputeDataGridMergerTotalCost(optimizedDataGridMerger);

	// Ecriture des distances inter-cluster pour chaque attribut
	for (nAttribute = 0; nAttribute < optimizedDataGridMerger->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = optimizedDataGridMerger->GetAttributeAt(nAttribute);

		// Recherche de l'attribut de coclustering correspondant
		hdgAttribute = cast(CCHDGAttribute*, optimizedDataGrid->GetAttributeAt(nAttribute));
		assert(hdgAttribute->GetAttributeName() == dgAttribute->GetAttributeName());
		assert(hdgAttribute->GetPartNumber() == dgAttribute->GetPartNumber());

		// Affichage des resultats: entete
		if (bTrace)
			cout << "Attribute\tPart1\tPart2\tDefault\tBest\tMerge\tCost\tInfo\n";

		// Parcours de toutes les parties de l'attribut
		dvInterests.SetSize(dgAttribute->GetPartNumber());
		dMaxInterest = -DBL_MAX;
		part1 = dgAttribute->GetHeadPart();
		nPart = 0;
		while (part1 != NULL)
		{
			partM1 = cast(KWDGMPart*, part1);

			// Parcours de toutes les autres parties de l'attribut pour evaluer leur fusion
			dTotalInterest = 0;
			part2 = dgAttribute->GetHeadPart();
			while (part2 != NULL)
			{
				partM2 = cast(KWDGMPart*, part2);

				// Recherche d'une fusion entre les parties
				partMerge = partM1->LookupPartMerge(partM2);

				// Calcul de la typicite
				dDeltaCost = 0;
				if (partMerge != NULL)
					dDeltaCost = partMerge->GetMergeCost();
				// Ce MergeCost ne tient pas compte des fusions de PV au sein du nouveau cluster
				dInterest = dDeltaCost / (dTotalDefaultCost - dBestDataGridTotalCost);
				dTotalInterest += dInterest;

				// Affichage des resultats: details
				if (bTrace)
					cout << dgAttribute->GetAttributeName() << "\t" << part1->GetObjectLabel()
					     << "\t" << part2->GetObjectLabel() << "\t" << dTotalDefaultCost << "\t"
					     << dBestDataGridTotalCost << "\t" << dDeltaCost << "\t"
					     << dBestDataGridTotalCost + dDeltaCost << "\t" << dInterest << "\n";

				// Partie suivante
				dgAttribute->GetNextPart(part2);
			}

			// Memorisation de la typicite moyenne
			if (dgAttribute->GetPartNumber() > 1)
				dInterest = dTotalInterest / (dgAttribute->GetPartNumber() - 1);
			else
				dInterest = 1;
			// coclusteringPart->SetInterest(dInterest);
			dvInterests.SetAt(nPart, dInterest);

			// Mise a jour du max
			if (dInterest > dMaxInterest)
				dMaxInterest = dInterest;

			// Partie suivante
			dgAttribute->GetNextPart(part1);
			nPart++;
		}

		// Normalisation des typicites par leur max
		part1 = hdgAttribute->GetHeadPart();
		debug(part2 = dgAttribute->GetHeadPart());
		nPart = 0;
		while (part1 != NULL)
		{
			hdgPart = cast(CCHDGPart*, part1);
			debug(assert(part1->GetObjectLabel() == part2->GetObjectLabel()));

			// Initialisation du niveau hierarchique
			hdgPart->SetHierarchicalLevel(1);

			// Mise a jour de la typicalite
			assert(0 <= dvInterests.GetAt(nPart) and dvInterests.GetAt(nPart) <= dMaxInterest);
			if (dMaxInterest > 0)
				hdgPart->SetInterest(dvInterests.GetAt(nPart) / dMaxInterest);
			else
				hdgPart->SetInterest(1);

			// Initialisation du ranh hierarchique avec le nombre total de parties
			hdgPart->SetHierarchicalRank(optimizedDataGrid->GetTotalPartNumber());

			// Partie suivante
			hdgAttribute->GetNextPart(part1);
			debug(dgAttribute->GetNextPart(part2));
			nPart++;
		}
	}
}

void CCCoclusteringBuilder::ComputePartHierarchies(KWDataGridMerger* optimizedDataGridMerger,
						   const KWDataGridCosts* dataGridCosts,
						   CCHierarchicalDataGrid* optimizedDataGrid) const
{
	const boolean bTrace = false;
	double dDataGridTotalCost;
	double dBestDataGridTotalCost;
	double dBestDeltaCost;
	double dTotalDefaultCost;
	double dHierarchicalLevel;
	double dInterest;
	KWDGMPartMerge* bestPartMerge;
	boolean bContinue;
	int nCount;
	NumericKeyDictionary nkdHierarchicalParts;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	CCHDGAttribute* hdgAttribute;
	int nPart;
	KWDGPart* dgPart;
	KWDGMPart* dgmMergedPart;
	KWDGPart* dgPart2;
	CCHDGPart* hdgPart;
	CCHDGPart* hdgParentPart;
	double dEpsilon = 1e-10;

	require(optimizedDataGridMerger != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGridMerger->GetDataGridCosts() == dataGridCosts);
	require(optimizedDataGridMerger->CheckAllPartMerges());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGridMerger->GetAttributeNumber() == optimizedDataGrid->GetAttributeNumber());

	// Memorisation dans un dictionnaire des parties de coclustering associee aux parties
	// initiales du merger de grille (qui doivent etre de structure correspondante (mais non egale))
	for (nAttribute = 0; nAttribute < optimizedDataGridMerger->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = optimizedDataGridMerger->GetAttributeAt(nAttribute);

		// Recherche de l'attribut de coclustering correspondant
		hdgAttribute = cast(CCHDGAttribute*, optimizedDataGrid->GetAttributeAt(nAttribute));
		assert(hdgAttribute->GetAttributeName() == dgAttribute->GetAttributeName());
		assert(hdgAttribute->GetPartNumber() == dgAttribute->GetPartNumber());

		// Parcours synchronise des parties de l'attribut
		nPart = 0;
		dgPart = dgAttribute->GetHeadPart();
		dgPart2 = hdgAttribute->GetHeadPart();
		while (dgPart != NULL)
		{
			hdgPart = cast(CCHDGPart*, dgPart2);
			assert(hdgPart->GetObjectLabel() == dgPart->GetObjectLabel());

			// Memorisation de l'association dans le dictionnaire
			nkdHierarchicalParts.SetAt(dgPart, hdgPart);

			// Partie suivante
			dgAttribute->GetNextPart(dgPart);
			hdgAttribute->GetNextPart(dgPart2);
			nPart++;
		}

		// Memorisation de la racine de la hierarchie dans le cas particulier d'une seule partie
		if (hdgAttribute->GetPartNumber() == 1)
			hdgAttribute->SetRootPart(cast(CCHDGPart*, hdgAttribute->GetHeadPart()));
	}
	assert(nkdHierarchicalParts.GetCount() == optimizedDataGridMerger->GetTotalPartNumber());

	// Cout par defaut et meilleur cout
	dTotalDefaultCost = dataGridCosts->GetTotalDefaultCost();
	dDataGridTotalCost =
	    optimizedDataGridMerger->GetDataGridCosts()->ComputeDataGridMergerTotalCost(optimizedDataGridMerger);
	dBestDataGridTotalCost = dDataGridTotalCost;

	// Affichage: entete
	if (bTrace)
		cout << "\nCount\tAttribute\tPart1\tPart2\tBest\tMerge\tCost\tDefault cost\tHierachicalLevel\n";

	// Boucle de recherche d'ameliorations
	nCount = 0;
	bContinue = true;
	while (bContinue)
	{
		nCount++;

		// Recherche de la meilleure amelioration
		dBestDeltaCost = optimizedDataGridMerger->SearchBestPartMergeWithGarbageSearch(bestPartMerge);

		// Ce cout n'est pas le vrai cout car la fusion n'est pas suivie issue de la fusion des PV adjacent
		bContinue = (bestPartMerge != NULL);
		assert(bContinue or dBestDeltaCost == DBL_MAX);

		// Impact de la meilleure amelioration
		if (bContinue)
		{
			// Affichage des details de la fusion
			dHierarchicalLevel = (dTotalDefaultCost - (dDataGridTotalCost + dBestDeltaCost));
			if (dTotalDefaultCost - dBestDataGridTotalCost > 0)
				dHierarchicalLevel /= dTotalDefaultCost - dBestDataGridTotalCost;
			// En presence d'un groupe poubelle la repartition des couts ne garantit plus dHierarchicalLevel
			// <=1 assert(dHierarchicalLevel <= 1 + dEpsilon);

			if (dHierarchicalLevel > 1 - dEpsilon)
				dHierarchicalLevel = 1;

			// Attention, le hierarchical level peut etre negatif: ici, on arrondi uniquement les
			// presque-zero
			if (fabs(dHierarchicalLevel) < dEpsilon)
				dHierarchicalLevel = 0;

			// Affichage: detail
			if (bTrace)
				cout << nCount << "\t" << bestPartMerge->GetPart1()->GetAttribute()->GetObjectLabel()
				     << "\t" << bestPartMerge->GetPart1()->GetObjectLabel() << "\t"
				     << bestPartMerge->GetPart2()->GetObjectLabel() << "\t" << dBestDataGridTotalCost
				     << "\t" << dBestDeltaCost << "\t" << dDataGridTotalCost + dBestDeltaCost << "\t"
				     << dTotalDefaultCost << "\t" << dHierarchicalLevel << "\n";

			// Recherche de l'attribut correspondant a la fusion de partie
			nAttribute = bestPartMerge->GetPart1()->GetAttributeIndex();
			dgAttribute = optimizedDataGridMerger->GetAttributeAt(nAttribute);
			hdgAttribute = cast(CCHDGAttribute*, optimizedDataGrid->GetAttributeAt(nAttribute));

			// Creation d'une nouvelle partie de coclustering
			hdgParentPart = hdgAttribute->NewHierarchyPart();
			hdgParentPart->SetHierarchicalLevel(dHierarchicalLevel);

			// Identifiant base sur la distance a la racine
			hdgParentPart->SetPartName(
			    BuildHierachicalPartName(hdgAttribute, optimizedDataGridMerger->GetTotalPartNumber() -
								       optimizedDataGridMerger->GetAttributeNumber()));

			// Rang hierarchique, basee sur le nombre de partie restante (apres la fusion)
			hdgParentPart->SetHierarchicalRank(optimizedDataGridMerger->GetTotalPartNumber() - 1);

			// Lien avec ses partie filles
			hdgPart = cast(CCHDGPart*, nkdHierarchicalParts.Lookup(bestPartMerge->GetPart1()));
			hdgPart->SetParentPart(hdgParentPart);
			hdgParentPart->SetChildPart1(hdgPart);
			hdgPart = cast(CCHDGPart*, nkdHierarchicalParts.Lookup(bestPartMerge->GetPart2()));
			hdgPart->SetParentPart(hdgParentPart);
			hdgParentPart->SetChildPart2(hdgPart);

			// Effectif
			hdgParentPart->SetPartFrequency(hdgParentPart->GetChildPart1()->GetPartFrequency() +
							hdgParentPart->GetChildPart2()->GetPartFrequency());

			// Typicite par moyenne ponderee des typicites des parties filles
			dInterest = (hdgParentPart->GetChildPart1()->GetPartFrequency() *
					 hdgParentPart->GetChildPart1()->GetInterest() +
				     hdgParentPart->GetChildPart2()->GetPartFrequency() *
					 hdgParentPart->GetChildPart2()->GetInterest());
			if (hdgParentPart->GetPartFrequency() > 0)
				dInterest /= hdgParentPart->GetPartFrequency();
			hdgParentPart->SetInterest(dInterest);

			// Realisation de la fusion
			dgmMergedPart = optimizedDataGridMerger->PerformPartMerge(bestPartMerge);
			dDataGridTotalCost += dBestDeltaCost;
			assert(fabs(optimizedDataGridMerger->GetDataGridCosts()->ComputeDataGridMergerTotalCost(
					optimizedDataGridMerger) -
				    dDataGridTotalCost) < dEpsilon * dDataGridTotalCost);

			// Pour les attributs Continuous, on utilise le nom d'intervalle comme identifiant de cluster
			// On le fait apres la fusion, pour beneficier de la methode GetObjectLabel des intervalles
			if (dgmMergedPart->GetAttribute()->GetAttributeType() == KWType::Continuous)
			{
				hdgParentPart->SetPartName(dgmMergedPart->GetObjectLabel());

				// Test si la partie fusionne de droite impliquait la valeur manquante
				// Dans ce cas, on rajoute * en tete de l'identifiant de la partie, pour distinguer le
				// cas ]-inf,ub] de *]-inf, ub]
				if (hdgParentPart->GetChildPart1()->GetInterval()->GetUpperBound() ==
					KWContinuous::GetMissingValue() or
				    (hdgParentPart->GetChildPart1()->IsParent() and
				     hdgParentPart->GetChildPart1()->GetPartName().GetAt(0) == '*'))
					hdgParentPart->SetPartName("*" + hdgParentPart->GetPartName());
			}

			// Si derniere partie de l'attribut, on memorise la racine de la hierarchie
			if (dgmMergedPart->GetAttribute()->GetPartNumber() == 1)
				hdgAttribute->SetRootPart(hdgParentPart);

			// Memorisation de la nouvelle partie
			nkdHierarchicalParts.SetAt(dgmMergedPart, hdgParentPart);
		}
	}
}

const ALString CCCoclusteringBuilder::BuildHierachicalPartName(const CCHDGAttribute* hdgAttribute,
							       int nHierarchicalIndex) const
{
	const char cIdentifierPrefix = 'A';
	ALString sPartName;

	require(hdgAttribute != NULL);
	require(nHierarchicalIndex > 0);

	// Construction du nom de la partie
	sPartName = cIdentifierPrefix;
	sPartName.SetAt(0, cIdentifierPrefix + char(hdgAttribute->GetAttributeIndex()));
	sPartName += IntToString(nHierarchicalIndex);
	return sPartName;
}

void CCCoclusteringBuilder::ComputePartRanks(const KWDataGridMerger* optimizedDataGridMerger,
					     const KWDataGridCosts* dataGridCosts,
					     CCHierarchicalDataGrid* optimizedDataGrid) const
{
	ObjectArray oaAttributeParts;
	int nAttribute;
	CCHDGAttribute* hdgAttribute;
	CCHDGPart* hdgRootgPart;
	CCHDGPart* hdgPart;
	CCHDGPart* hdgChildPart1;
	CCHDGPart* hdgChildPart2;
	int nPart;

	require(optimizedDataGridMerger != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGridMerger->GetDataGridCosts() == dataGridCosts);
	require(optimizedDataGridMerger->CheckAllPartMerges());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGridMerger->GetAttributeNumber() == optimizedDataGrid->GetAttributeNumber());

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < optimizedDataGrid->GetAttributeNumber(); nAttribute++)
	{
		hdgAttribute = cast(CCHDGAttribute*, optimizedDataGrid->GetAttributeAt(nAttribute));

		// Export des parties
		oaAttributeParts.SetSize(0);
		hdgAttribute->ExportHierarchyParts(&oaAttributeParts);

		// Parcours de toutes les parties de l'attribut et de sa structure hierarchique
		// pour reordonner les sous-branches de chaque noeud intermediaire
		for (nPart = 0; nPart < oaAttributeParts.GetSize(); nPart++)
		{
			hdgPart = cast(CCHDGPart*, oaAttributeParts.GetAt(nPart));
			assert(not hdgPart->IsLeaf() or hdgPart->Check());

			// Reordonnancement des sous parties par maximum d'interet pour les parties intermediaires de la
			// hierarchie Uniquement dans le cas Symbol
			if (hdgAttribute->GetAttributeType() == KWType::Symbol)
			{
				hdgChildPart1 = hdgPart->GetChildPart1();
				hdgChildPart2 = hdgPart->GetChildPart2();
				if (hdgChildPart1 != NULL and hdgChildPart2 != NULL)
				{
					if (hdgChildPart1->GetInterest() < hdgChildPart2->GetInterest())
					{
						hdgPart->SetChildPart1(hdgChildPart2);
						hdgPart->SetChildPart2(hdgChildPart1);
					}
					// En cas d'egalite des interets des parties, comparaison de leur effectif
					else if (hdgChildPart1->GetInterest() == hdgChildPart2->GetInterest())
					{
						if (hdgChildPart1->GetPartFrequency() <
						    hdgChildPart2->GetPartFrequency())
						{
							hdgPart->SetChildPart1(hdgChildPart2);
							hdgPart->SetChildPart2(hdgChildPart1);
						}
						// En cas d'egalite d'effectif, comparaison lexicographique des noms des
						// parties
						else if (hdgChildPart1->GetPartFrequency() ==
							 hdgChildPart2->GetPartFrequency())
						{
							if (hdgChildPart1->GetPartName() > hdgChildPart2->GetPartName())
							{
								hdgPart->SetChildPart1(hdgChildPart2);
								hdgPart->SetChildPart2(hdgChildPart1);
							}
						}
					}
				}
			}
		}

		// Numerotation (Ranks) des noeuds d'un arbre de partie de coclustering par parcours infixe
		hdgRootgPart = hdgAttribute->GetRootPart();
		check(hdgRootgPart);
		ComputePartInfixRanks(hdgRootgPart);
	}
}

void CCCoclusteringBuilder::ComputePartInfixRanks(CCHDGPart* hdgRootgPart) const
{
	CCHDGPart* hdgPreviousPart;
	CCHDGPart* hdgActualPart;
	CCHDGPart* hdgNextPart;
	int nRank;

	require(hdgRootgPart != NULL);
	require(cast(CCHDGAttribute*, hdgRootgPart->GetAttribute())->GetRootPart() == hdgRootgPart);

	// Parcours de l'arbre en infixe a partir de la racine, pour numerotation des noeuds
	// Emprunte a wikipedia
	// VisiterInfixeIteratif(racine)
	// precedent    := null
	// actuel	:= racine
	// suivant	:= null
	//
	// Tant que (actuel != null) Faire
	//     Si (precedent == pere(actuel)) Alors
	//	  precedent := actuel
	//	  suivant   := gauche(actuel)
	//     FinSi
	//     Si (suivant == null OU precedent == gauche(actuel)) Alors
	//	  Visiter(actuel)
	//	  precedent := actuel
	//	  suivant   := droite(actuel)
	//     FinSi
	//     Si (suivant == null OU precedent == droite(actuel)) Alors
	//	  precedent := actuel
	//	  suivant   := pere(actuel)
	//     FinSi
	//     actuel := suivant
	// FinTantQue
	nRank = 1;
	hdgPreviousPart = NULL;
	hdgActualPart = hdgRootgPart;
	hdgNextPart = NULL;
	while (hdgActualPart != NULL)
	{
		if (hdgPreviousPart == hdgActualPart->GetParentPart())
		{
			hdgPreviousPart = hdgActualPart;
			hdgNextPart = hdgActualPart->GetChildPart1();
		}
		if (hdgNextPart == NULL or hdgPreviousPart == hdgActualPart->GetChildPart1())
		{
			hdgPreviousPart = hdgActualPart;
			hdgNextPart = hdgActualPart->GetChildPart2();

			// Numerotation du noeud
			hdgActualPart->SetRank(nRank);
			nRank++;
		}
		if (hdgNextPart == NULL or hdgPreviousPart == hdgActualPart->GetChildPart2())
		{
			hdgPreviousPart = hdgActualPart;
			hdgNextPart = hdgActualPart->GetParentPart();
		}
		hdgActualPart = hdgNextPart;
	}
}

void CCCoclusteringBuilder::SortAttributePartsAndValues(CCHierarchicalDataGrid* optimizedDataGrid) const
{
	int nAttribute;
	KWDGAttribute* dgAttribute;
	KWDGPart* dgPart;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	KWDGPart* innerPart;

	require(optimizedDataGrid != NULL);

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < optimizedDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = optimizedDataGrid->GetAttributeAt(nAttribute);

		// Tri des parties
		cast(CCHDGAttribute*, dgAttribute)->SortPartsByRank();

		// Nommage des parties terminales et tri des valeurs pour les attributs categoriels
		dgPart = dgAttribute->GetHeadPart();
		while (dgPart != NULL)
		{
			// Tri des valeurs de la partie si attribut categoriel
			if (dgAttribute->GetAttributeType() == KWType::Symbol)
				dgPart->GetValueSet()->SortValuesByDecreasingTypicalities();

			// Initialisation du nom de la partie
			cast(CCHDGPart*, dgPart)->SetPartName(dgPart->GetObjectLabel());

			// Partie suivante
			dgAttribute->GetNextPart(dgPart);
		}

		// Traitement des innerAttributes dans le cas d'un attribut VarPart
		if (dgAttribute->GetAttributeType() == KWType::VarPart)
		{
			// Parcours des inner attributes
			for (nInnerAttribute = 0; nInnerAttribute < dgAttribute->GetInnerAttributeNumber();
			     nInnerAttribute++)
			{
				innerAttribute = dgAttribute->GetInnerAttributeAt(nInnerAttribute);

				// Nommage des parties terminales et tri des valeurs pour les attributs categoriels
				innerPart = innerAttribute->GetHeadPart();
				while (innerPart != NULL)
				{

					// Tri des valeurs de la partie si attribut interne categoriel
					if (innerAttribute->GetAttributeType() == KWType::Symbol)
						innerPart->GetSymbolValueSet()->SortValuesByDecreasingTypicalities();

					// Initialisation du nom de la partie
					cast(CCHDGPart*, innerPart)->SetPartName(innerPart->GetVarPartLabel());

					// Partie suivante
					dgAttribute->GetNextPart(innerPart);
				}
			}
		}
	}
}
