// Copyright (c) 2023 Orange. All rights reserved.
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
	dAnyTimeDefaultCost = 0;
	dAnyTimeBestCost = 0;
	bIsDefaultCostComputed = false;
	bExportJSON = true;
}

CCCoclusteringBuilder::~CCCoclusteringBuilder()
{
	CleanCoclusteringResults();
}

const ALString& CCCoclusteringBuilder::GetFrequencyAttribute() const
{
	return sFrequencyAttribute;
}

void CCCoclusteringBuilder::SetFrequencyAttribute(const ALString& sValue)
{
	sFrequencyAttribute = sValue;
}

boolean CCCoclusteringBuilder::CheckSpecifications() const
{
	boolean bOk;
	KWAttribute* attribute;
	int i;

	// Verification standard
	bOk = KWAttributeSubsetStats::CheckSpecifications();

	// Verification du cas non supervise
	if (bOk and GetTargetAttributeName() != "")
	{
		bOk = false;
		AddError("Coclustering not available in the supervised case");
	}

	// Verification du nombre d'attribut
	if (bOk and GetAttributeNumber() < 2)
	{
		bOk = false;
		AddError("Coclustering available for at least two variables");
	}

	// Verification du type des attributs
	for (i = 0; i < GetAttributeNumber(); i++)
	{
		// Recherche de l'attribut
		attribute = GetClass()->LookupAttribute(GetAttributeNameAt(i));

		// Test du type de l'attribut
		if (attribute != NULL and not KWType::IsSimple(attribute->GetType()))
		{
			bOk = false;
			AddError("Variable " + GetAttributeNameAt(i) + " is not of numerical or categorical type");
		}

		// Arret si erreurs
		if (not bOk)
			break;
	}

	// Verification de l'attribut d'effectif
	if (GetFrequencyAttribute() != "")
		if (bOk and sFrequencyAttribute != "")
		{
			// Recherche de la variable correspondante dans le dictionnaire
			attribute = GetClass()->LookupAttribute(sFrequencyAttribute);

			// La variable doit etre presente dans le dictionnaire
			if (attribute == NULL)
			{
				bOk = false;
				Global::AddError("", "",
						 "Coclustering frequency variable " + sFrequencyAttribute +
						     " unknown in dictionary " + GetClass()->GetName());
			}
			// De type Continuous
			else if (attribute->GetType() != KWType::Continuous)
			{
				bOk = false;
				Global::AddError("", "",
						 "Incorrect type for coclustering frequency variable " +
						     sFrequencyAttribute);
			}
			// Et utilise
			else if (not attribute->GetUsed())
			{
				bOk = false;
				Global::AddError("", "",
						 "Coclustering frequency variable " + sFrequencyAttribute +
						     " unused in dictionary " + GetClass()->GetName());
			}
			// et differente des attributs de coclustering
			else
			{
				// Parcours des attributs de coclustering
				for (i = 0; i < GetAttributeNumber(); i++)
				{
					if (sFrequencyAttribute == GetAttributeNameAt(i))
					{
						bOk = false;
						Global::AddError(
						    "", "",
						    "Coclustering frequency variable " + sFrequencyAttribute +
							" is already used among the coclustering variables");
						break;
					}
				}
			}
		}

	return bOk;
}

boolean CCCoclusteringBuilder::ComputeCoclustering()
{
	boolean bOk = true;
	KWTupleTableLoader tupleTableLoader;
	StringVector svTupleAttributeNames;
	KWTupleTable tupleTable;
	KWTupleTable tupleFrequencyTable;
	CCCoclusteringOptimizer dataGridOptimizer;
	KWDataGrid optimizedDataGrid;
	KWDataGridManager dataGridManager;
	ALString sTmp;

	require(Check());
	require(CheckSpecifications());

	// Debut de la gestion des erreurs dediees a l'apprentissage
	KWLearningErrorManager::BeginErrorCollection();
	KWLearningErrorManager::AddTask("Coclustering");

	// Debut du pilotage anytime
	AnyTimeStart();

	// Nettoyage des resultats de coclustering
	CleanCoclusteringResults();

	///////////////////////////////////////////////////////////////////////////////////
	// Calcul d'une grille initiale

	// Verification de la memoire necessaire pour charger la base
	if (bOk)
		bOk = CheckMemoryForDatabaseRead(GetDatabase());

	// Alimentation d'une table de tuples comportant les attribut a analyser a partir de la base
	if (bOk and not TaskProgression::IsInterruptionRequested())
		bOk = FillTupleTableFromDatabase(GetDatabase(), &tupleTable);

	// Calcul de statistiques descriptives globales et par attribut
	if (bOk and not TaskProgression::IsInterruptionRequested())
		bOk = GetLearningSpec()->ComputeTargetStats(&tupleTable);
	if (bOk and not TaskProgression::IsInterruptionRequested())
		ComputeDescriptiveAttributeStats(&tupleTable, &odDescriptiveStats);

	// Verification de la memoire necessaire pour construire une grille initiale a partir des tuples
	nMaxCellNumberConstraint = 0;
	if (bOk and not TaskProgression::IsInterruptionRequested())
		bOk =
		    CheckMemoryForDataGridInitialization(GetDatabase(), tupleTable.GetSize(), nMaxCellNumberConstraint);

	// Creation du DataGrid
	if (bOk and not TaskProgression::IsInterruptionRequested())
	{
		initialDataGrid = CreateDataGrid(&tupleTable);
		bOk = initialDataGrid != NULL;
	}

	// Supression des tuples, desormais transferes dans la grille
	tupleTable.CleanAll();

	// On verifie une derniere fois qu'il n'y a pas eu d'interruption
	if (bOk)
		bOk = not TaskProgression::IsInterruptionRequested();

	// Verification de la memoire necessaire pour optimiser le coclustering
	if (bOk)
		bOk = CheckMemoryForDataGridOptimization(initialDataGrid);

	// Arret si grille non creee, avec nettoyage
	if (not bOk)
	{
		if (initialDataGrid != NULL)
		{
			delete initialDataGrid;
			initialDataGrid = NULL;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Optimisation et post-optimisation de la grille

	// Optimisation de la grille, par appel d'une methode virtuelle
	if (bOk and not TaskProgression::IsInterruptionRequested())
		OptimizeDataGrid(initialDataGrid, &optimizedDataGrid);

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
	nMaxCellNumberConstraint = 0;

	// Fin du pilotage anytime
	AnyTimeStop();

	// Fin de la gestion des erreurs dediees a l'apprentissage
	KWLearningErrorManager::EndErrorCollection();

	// Nettoyage
	bIsStatsComputed = bOk;
	ensure(Check());
	return bIsStatsComputed;
}

void CCCoclusteringBuilder::OptimizeDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid)
{
	CCCoclusteringOptimizer dataGridOptimizer;

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
	debug(cout << "BEWARE: Optimization level set to 0 in debug mode only!!!" << endl);
	if (dataGridOptimizer.GetParameters()->GetOptimizationTime() > 0)
		dataGridOptimizer.GetParameters()->SetOptimizationLevel(20);

	// Optimisation de la grille
	dataGridOptimizer.OptimizeDataGrid(inputInitialDataGrid, optimizedDataGrid);
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

boolean CCCoclusteringBuilder::GetExportJSON() const
{
	return bExportJSON;
}

void CCCoclusteringBuilder::SetExportJSON(boolean bValue)
{
	bExportJSON = bValue;
}

void CCCoclusteringBuilder::RemoveLastSavedReportFile() const
{
	if (sLastActualAnyTimeReportFileName != "")
	{
		PLRemoteFileService::RemoveFile(sLastActualAnyTimeReportFileName);

		// Destruction eventuelle du rapport au format JSON
		if (GetExportJSON())
			PLRemoteFileService::RemoveFile(FileService::SetFileSuffix(
			    sLastActualAnyTimeReportFileName, CCCoclusteringReport::GetJSONReportSuffix()));
	}
	sLastActualAnyTimeReportFileName = "";
}

void CCCoclusteringBuilder::HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
						   const KWDataGrid* initialGranularizedDataGrid,
						   boolean bIsLastSaving) const
{
	boolean bKeepIntermediateReports = false;
	boolean bWriteOk;
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
	ALString sTmp;
	int nGranularityMax;

	require(optimizedDataGrid != NULL);
	require(nAnyTimeOptimizationIndex >= 0);

	nGranularityMax = (int)ceil(log(initialDataGrid->GetGridFrequency() * 1.0) / log(2.0));

	// Memorisation du cout par defaut la premiere fois

	if (not bIsDefaultCostComputed)
	{
		assert(nAnyTimeOptimizationIndex == 0);
		assert(dAnyTimeDefaultCost == 0);
		dAnyTimeDefaultCost = coclusteringDataGridCosts->GetTotalDefaultCost();
		dAnyTimeBestCost = dAnyTimeDefaultCost;
		bIsDefaultCostComputed = true;
	}
	assert(dAnyTimeDefaultCost > 0);

	// Cout de la nouvelle solution
	dCost = coclusteringDataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);

	// Test si amelioration
	//  ou si la mise a jour est commandee par l'atteinte de la granularite maximale
	// Les grilles avec un seul attribut informatif ne sont pas sauvegardes
	// Cela signifie qu'une grille legerement plus chere avec deux attributs informatifs rencontree au cours
	// de l'optimisation mais non sauvegardee car non optimale du point de vue du cout peut exister mais
	// n'aura pas ete sauvegardee (cf resultats sur AdultSmall1var cout de la grille initiale granularisee)
	if (optimizedDataGrid->GetInformativeAttributeNumber() >= 2 and
	    (dCost < dAnyTimeBestCost - dEpsilon or
	     (bIsLastSaving and ((coclusteringDataGrid == NULL) or (coclusteringDataGrid->GetGranularity() <
								    initialGranularizedDataGrid->GetGranularity())))))
	{
		dAnyTimeBestCost = dCost;

		// Sauvegarde de la grille
		if (coclusteringDataGrid != NULL)
			delete coclusteringDataGrid;
		coclusteringDataGrid = new CCHierarchicalDataGrid;
		dataGridManager.CopyDataGrid(optimizedDataGrid, coclusteringDataGrid);

		// Memorisation de la description courte
		coclusteringDataGrid->SetShortDescription(GetShortDescription());

		// Calcul de ses infos de hierarchie
		// Cas sans granularisation
		if (initialGranularizedDataGrid == NULL)
			ComputeHierarchicalInfo(initialDataGrid, coclusteringDataGridCosts, coclusteringDataGrid);
		// Avec granularisation : la grille granularisee initiale est la reference pour le calcul des infos
		else
			ComputeHierarchicalInfo(initialGranularizedDataGrid, coclusteringDataGridCosts,
						coclusteringDataGrid);

		// Calcul du temps d'optimisation
		tAnyTimeTimer.Stop();
		dOptimizationTime = tAnyTimeTimer.GetElapsedTime(), tAnyTimeTimer.Start();

		// Calcul du Level
		dLevel = 1 - dAnyTimeBestCost / dAnyTimeDefaultCost;

		// Calcul d'un libelle sur la taille de la grille (nombre de parties par dimension)
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			dgAttribute = coclusteringDataGrid->SearchAttribute(GetAttributeNameAt(nAttribute));
			if (nAttribute > 0)
				sCoclusteringSizeInfo += "*";
			if (dgAttribute == NULL)
				sCoclusteringSizeInfo += "1";
			else
				sCoclusteringSizeInfo += IntToString(dgAttribute->GetPartNumber());
		}

		// Sauvegarde dans un fichier temporaire
		nAnyTimeOptimizationIndex++;
		sReportFileName = AnyTimeBuildTemporaryReportFileName(nAnyTimeOptimizationIndex);
		// Cas de non affichage d'info de granularite : pas en mode granularite ou granularite maximale
		if (initialGranularizedDataGrid->GetGranularity() == 0 or
		    (initialGranularizedDataGrid->GetGranularity() == nGranularityMax or
		     initialGranularizedDataGrid->GetLnGridSize() == initialDataGrid->GetLnGridSize()))
			AddSimpleMessage(sTmp + "  " + SecondsToString((int)dOptimizationTime) + "\t" +
					 "Write intermediate coclustering report " +
					 FileService::GetFileName(sReportFileName) +
					 "\tLevel: " + DoubleToString(dLevel) + "\tSize: " + sCoclusteringSizeInfo);
		else
			AddSimpleMessage(
			    sTmp + "  " + SecondsToString((int)dOptimizationTime) + "\t" +
			    "Write intermediate coclustering report " + FileService::GetFileName(sReportFileName) +
			    "\tLevel: " + DoubleToString(dLevel) + "\tSize: " + sCoclusteringSizeInfo +
			    "\tGranularity: " + IntToString(initialGranularizedDataGrid->GetGranularity()));
		bWriteOk = coclusteringReport.WriteReport(sReportFileName, coclusteringDataGrid);

		// Sauvegarde au format JSON si necessaire
		if (bWriteOk and GetExportJSON())
			coclusteringReport.WriteJSONReport(
			    FileService::SetFileSuffix(sReportFileName, CCCoclusteringReport::GetJSONReportSuffix()),
			    coclusteringDataGrid);

		// Destruction de la precedente sauvegarde
		if (not bKeepIntermediateReports and bWriteOk)
			RemoveLastSavedReportFile();

		// Memorisation du nouveau nom du dernier fichier sauvegarde
		if (bWriteOk)
			sLastActualAnyTimeReportFileName = sReportFileName;
	}
}

const ALString CCCoclusteringBuilder::GetClassLabel() const
{
	return GetLearningModuleName();
}

boolean CCCoclusteringBuilder::CheckMemoryForDatabaseRead(KWDatabase* database) const
{
	boolean bOk = true;
	boolean bDisplayMemoryStats = false;
	PLDatabaseTextFile plDatabaseTextFile;
	longint lAvailableMemory;
	longint lSourceFileSize;
	longint lRecordSize;
	int nAttributeNumber;
	longint lEstimatedRecordNumber;
	longint lNecessaryMemory;
	longint lFileMemory;
	longint lInitialDataGridSize;
	longint lWorkingDataGridSize;
	longint lSizeOfCell;
	int nDatabasePercentage;

	require(database != NULL);
	require(database->GetObjects()->GetSize() == 0);

	// Memoire disponible
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Memoire de base pour ouvrir la base
	// On passe par un objet plDatabaseTextFile, qui dispose de fonctionnalite de dimensionnement des ressources
	lNecessaryMemory = 0;
	plDatabaseTextFile.InitializeFrom(GetDatabase());
	bOk = plDatabaseTextFile.ComputeOpenInformation(true, false, NULL);
	if (bOk)
		lNecessaryMemory = plDatabaseTextFile.GetMinOpenNecessaryMemory();

	// On ne fait une estimation fiable de la memoire necessaire que s'il
	// n'y a pas d'attribut de selection (qui ne permet pas d'estimer le nombre d'enregistrements)
	if (bOk and GetDatabase()->GetSelectionAttribute() == "")
	{
		// Elements de dimensionnement a partir des caracteristiques du fichier et du dictionnaire
		lSourceFileSize = FileService::GetFileSize(database->GetDatabaseName());
		nAttributeNumber = GetClass()->GetLoadedAttributeNumber();
		lRecordSize =
		    sizeof(KWObject) + sizeof(KWObject*) + 2 * sizeof(void*) + sizeof(KWValue) * nAttributeNumber;

		// Pourcentage de la base traite
		if (database->GetModeExcludeSample())
			nDatabasePercentage = 100 - database->GetSampleNumberPercentage();
		else
			nDatabasePercentage = database->GetSampleNumberPercentage();

		// Calcul des caracteristiques memoires disponibles et necessaires pour les enregistrements du fichier
		lEstimatedRecordNumber = database->GetEstimatedObjectNumber() * nDatabasePercentage / 100;
		lFileMemory = lEstimatedRecordNumber * lRecordSize;
		lFileMemory += lSourceFileSize *
			       (GetClass()->GetUsedAttributeNumberForType(KWType::Symbol) /
				GetClass()->GetNativeDataItemNumber()) *
			       nDatabasePercentage / 100;
		lNecessaryMemory += lFileMemory;

		// Prise en compte d'une grille initiale "minimale" estimee de facon heuristique
		lSizeOfCell = sizeof(KWDGMCell) + (2 + GetClass()->GetLoadedAttributeNumber()) * sizeof(void*);
		lInitialDataGridSize = sizeof(KWDataGrid) + nAttributeNumber * sizeof(KWDGAttribute) +
				       (longint)(ceil(sqrt(lEstimatedRecordNumber * 1.0)) * nAttributeNumber *
						 (lSizeOfCell + sizeof(KWDGMPart) + sizeof(KWDGMPartMerge) +
						  sizeof(KWDGInterval) + sizeof(KWDGValueSet) + sizeof(KWDGValue)));
		lNecessaryMemory += lInitialDataGridSize;

		// Plus une grille de travail, et une pour la meilleure solution (de meme taille que la grille initiale)
		lWorkingDataGridSize = 2 * lInitialDataGridSize;
		lNecessaryMemory += lWorkingDataGridSize;

		// Affichage de stats memoire
		if (bDisplayMemoryStats)
		{
			cout << "CheckMemoryForDatabaseRead" << endl;
			cout << "\tEstimated Record number: " << lEstimatedRecordNumber << endl;
			cout << "\tSource file fize: " << LongintToHumanReadableString(lSourceFileSize) << endl;
			cout << "\tFile memory: " << LongintToHumanReadableString(lFileMemory) << endl;
			cout << "\tInitial data grid: " << LongintToHumanReadableString(lInitialDataGridSize) << endl;
			cout << "\tWorking data grid: " << LongintToHumanReadableString(lWorkingDataGridSize) << endl;
			cout << "\t  Necessary: " << LongintToHumanReadableString(lNecessaryMemory) << endl;
			cout << "\t  Available: " << LongintToHumanReadableString(lAvailableMemory) << endl;
			cout << "\t  OK: " << (lNecessaryMemory <= lAvailableMemory) << endl;
		}
	}

	// Test si memoire suffisante
	if (bOk and lNecessaryMemory > lAvailableMemory)
	{
		AddError("Not enough memory to load database " +
			 RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
		AddMessage(RMResourceManager::BuildMemoryLimitMessage());
		if (RMResourceConstraints::GetIgnoreMemoryLimit())
			RMResourceManager::DisplayIgnoreMemoryLimitMessage();
		else
			bOk = false;
	}
	return bOk;
}

boolean CCCoclusteringBuilder::FillTupleTableFromDatabase(KWDatabase* database, KWTupleTable* tupleTable)
{
	boolean bOk;
	KWTupleTableLoader tupleTableLoader;
	KWTuple* inputTuple;
	StringVector svTupleAttributeNames;
	int nAttribute;
	KWAttribute* attribute;
	KWLoadIndex liFrequencyAttributeLoadIndex;
	KWLoadIndexVector livLoadIndexes;
	KWObject* kwoObject;
	longint lObjectNumber;
	longint lRecordNumber;
	int nObjectFrequency;
	longint lTotalFrequency;
	PeriodicTest periodicTestInterruption;
	ALString sTmp;

	require(database != NULL);
	require(database->GetObjects()->GetSize() == 0);
	require(tupleTable != NULL);
	require(not tupleTable->GetUpdateMode());
	require(tupleTable->GetSize() == 0);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Read database " + database->GetDatabaseName());

	// Recherche de l'index de l'attribut d'effectif
	if (GetFrequencyAttribute() != "")
		liFrequencyAttributeLoadIndex = GetClass()->LookupAttribute(GetFrequencyAttribute())->GetLoadIndex();

	// Parametrage du chargeur de tuples
	tupleTableLoader.SetInputClass(GetClass());

	// Initialisation des attributs de la table de tuples
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetClass()->LookupAttribute(GetAttributeNameAt(nAttribute));
		tupleTable->AddAttribute(attribute->GetName(), attribute->GetType());

		// Memorisation du LoadIndex de l'attribut
		livLoadIndexes.Add(attribute->GetLoadIndex());
	}

	// Passage de la table de tuples en mode edition
	tupleTable->SetUpdateMode(true);
	inputTuple = tupleTable->GetInputTuple();

	// Ouverture de la base en lecture
	bOk = database->OpenForRead();

	// Lecture d'objets dans la base
	lTotalFrequency = 0;
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		lRecordNumber = 0;
		lObjectNumber = 0;
		while (not database->IsEnd())
		{
			kwoObject = database->Read();
			lRecordNumber++;

			// Acces a l'effectif de l'objet
			nObjectFrequency = 0;
			if (kwoObject != NULL)
			{
				// Acces a l'effectif, avec warning eventuel
				nObjectFrequency =
				    GetDatabaseObjectFrequency(kwoObject, liFrequencyAttributeLoadIndex, lRecordNumber);
				lTotalFrequency += nObjectFrequency;

				// Destruction de l'objet si effectif null
				if (nObjectFrequency <= 0)
				{
					delete kwoObject;
					kwoObject = NULL;
				}
				// Erreur si effectif total trop important
				else if (lTotalFrequency > INT_MAX)
				{
					Object::AddError(sTmp + "Read database interrupted after record " +
							 LongintToString(lRecordNumber) +
							 " because total frequency is too large (" +
							 LongintToReadableString(lTotalFrequency) + ")");
					delete kwoObject;
					kwoObject = NULL;
					bOk = false;
					break;
				}
			}

			// Gestion de l'objet
			if (kwoObject != NULL)
			{
				lObjectNumber++;

				// Parametrage du tuple d'entree de la table a cree
				for (nAttribute = 0; nAttribute < livLoadIndexes.GetSize(); nAttribute++)
				{
					if (tupleTable->GetAttributeTypeAt(nAttribute) == KWType::Symbol)
						inputTuple->SetSymbolAt(
						    nAttribute,
						    kwoObject->GetSymbolValueAt(livLoadIndexes.GetAt(nAttribute)));
					else
						inputTuple->SetContinuousAt(
						    nAttribute,
						    kwoObject->GetContinuousValueAt(livLoadIndexes.GetAt(nAttribute)));
				}

				// Ajout d'un nouveau tuple apres avoir specifie son effectif
				assert(nObjectFrequency <= INT_MAX - tupleTable->GetTotalFrequency());
				inputTuple->SetFrequency(nObjectFrequency);
				tupleTable->UpdateWithInputTuple();

				// Destruction de l'objet
				delete kwoObject;
				kwoObject = NULL;

				// Test regulierement si il y a assez de memoire
				if (tupleTable->GetSize() % 65536 == 0)
				{
					bOk = CheckMemoryForDataGridInitialization(GetDatabase(), tupleTable->GetSize(),
										   nMaxCellNumberConstraint);
					if (not bOk)
						break;
				}
			}
			// Arret si interruption utilisateur
			else if (TaskProgression::IsInterruptionRequested())
			{
				assert(kwoObject == NULL);
				bOk = false;
				break;
			}
			assert(kwoObject == NULL);

			// Arret si erreur
			if (database->IsError())
			{
				bOk = false;
				Object::AddError("Read database interrupted because of errors");
				break;
			}

			// Suivi de la tache
			if (periodicTestInterruption.IsTestAllowed(lRecordNumber))
			{
				TaskProgression::DisplayProgression((int)(100 * database->GetReadPercentage()));
				database->DisplayReadTaskProgressionLabel(lRecordNumber, lObjectNumber);
			}
		}
		Global::DesactivateErrorFlowControl();

		// Test si interruption sans qu'il y ait d'erreur
		if (not database->IsError() and TaskProgression::IsInterruptionRequested())
		{
			bOk = false;
			Object::AddWarning("Read database interrupted by user");
		}

		// Fermeture
		bOk = database->Close() and bOk;

		// Message global de compte-rendu
		if (bOk)
		{
			if (lRecordNumber == lObjectNumber)
				database->AddMessage(sTmp + "Read records: " + LongintToReadableString(lObjectNumber));
			else
				database->AddMessage(sTmp + "Read records: " + LongintToReadableString(lRecordNumber) +
						     "\tSelected records: " + LongintToReadableString(lObjectNumber));
			if (GetFrequencyAttribute() != "")
				GetDatabase()->AddMessage(
				    sTmp + "Total frequency: " + LongintToReadableString(lTotalFrequency));
		}
	}

	// Finalisation en repassant la table de tuples en mode consultation
	tupleTable->SetUpdateMode(false);
	if (not bOk)
		tupleTable->DeleteAll();

	// Erreur si aucun enregistrement lu
	if (bOk and tupleTable->GetSize() == 0)
	{
		AddError("No record read from database");
		bOk = false;
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();
	ensure(not tupleTable->GetUpdateMode());
	ensure(bOk or tupleTable->GetSize() == 0);
	return bOk;
}

int CCCoclusteringBuilder::GetDatabaseObjectFrequency(KWObject* kwoObject, KWLoadIndex liFrequencyAttributeLoadIndex,
						      longint lRecordIndex)
{
	int nObjectFrequency;
	Continuous cObjectFrequency;
	ALString sTmp;

	require(kwoObject != NULL);
	require(lRecordIndex >= 1);

	// Recherche de l'effectif de la cellule, en fonction de l'eventuelle variable d'effectif
	nObjectFrequency = 1;
	if (liFrequencyAttributeLoadIndex.IsValid())
	{
		// Recherche de l'effectif
		cObjectFrequency = kwoObject->GetContinuousValueAt(liFrequencyAttributeLoadIndex);
		nObjectFrequency = (int)floor(cObjectFrequency + 0.5);
		if (nObjectFrequency < 0)
			nObjectFrequency = 0;

		// Enregistrement ignore si effectif trop grand
		if (cObjectFrequency > INT_MAX)
		{
			GetDatabase()->AddWarning(sTmp + "Ignored record " + LongintToString(lRecordIndex) +
						  ", frequency variable (" + GetFrequencyAttribute() +
						  ") with value too large (" +
						  KWContinuous::ContinuousToString(cObjectFrequency) + ")");

			// On met l'effectif a 0 pour ignorer l'enregistrement
			nObjectFrequency = 0;
		}
		// Enregistrement ignore si effectif negatif ou nul
		else if (cObjectFrequency <= 0)
		{
			GetDatabase()->AddWarning(sTmp + "Ignored record " + LongintToString(lRecordIndex) +
						  ", frequency variable (" + GetFrequencyAttribute() +
						  ") with non positive value (" +
						  KWContinuous::ContinuousToString(cObjectFrequency) + ")");
		}
		// Warning si erreur d'arrondi
		else if (fabs(cObjectFrequency - nObjectFrequency) > 0.05)
		{
			if (nObjectFrequency > 0)
			{
				GetDatabase()->AddWarning(sTmp + "Record " + LongintToString(lRecordIndex) +
							  ", frequency variable (" + GetFrequencyAttribute() +
							  ") with non integer value (" +
							  KWContinuous::ContinuousToString(cObjectFrequency) + " -> " +
							  IntToString(nObjectFrequency) + ")");
			}
			else
			{
				GetDatabase()->AddWarning(sTmp + "Ignored record " + LongintToString(lRecordIndex) +
							  ", frequency variable (" + GetFrequencyAttribute() +
							  ") with null rounded value (" +
							  KWContinuous::ContinuousToString(cObjectFrequency) + ")");
			}
		}
	}
	return nObjectFrequency;
}

boolean CCCoclusteringBuilder::CheckMemoryForDataGridInitialization(KWDatabase* database, int nTupleNumber,
								    int& nMaxCellNumber) const
{
	boolean bOk = true;
	boolean bDisplayMemoryStats = false;
	int nAttributeNumber;
	longint lAvailableMemory;
	longint lNecessaryMemory;
	longint lInitialDataGridSize;
	longint lWorkingDataGridSize;
	longint lSizeOfCell;
	double dMaxCellNumber;
	int nValueNumber;
	int nAttribute;
	KWAttribute* attribute;
	KWDescriptiveStats* descriptiveStats;
	ALString sMessage;
	ALString sTmp;

	require(database != NULL);
	require(database->IsOpenedForRead() or odDescriptiveStats.GetCount() > 0);
	require(nTupleNumber > 0);
	require(odDescriptiveStats.GetCount() == 0 or
		odDescriptiveStats.GetCount() ==
		    GetClass()->GetLoadedAttributeNumber() - (GetFrequencyAttribute() == "" ? 0 : 1));

	// Calcul des caracteristiques memoire disponibles (le fichier est lu a ce moment)
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Stats de bases sur les variables et instances
	nAttributeNumber = GetClass()->GetLoadedAttributeNumber();

	// Prise en compte d'une grille initiale "minimale" estimee de facon fine en prenant en compte les nombres de
	// valeurs par variable
	lNecessaryMemory = 0;
	lInitialDataGridSize = sizeof(KWDataGrid) + sizeof(void*);
	dMaxCellNumber = 1;
	for (nAttribute = 0; nAttribute < GetClass()->GetLoadedAttributeNumber(); nAttribute++)
	{
		attribute = GetClass()->GetLoadedAttributeAt(nAttribute);

		// Recherche des stats descriptives de l'attribut, sauf si attribut d'effectif
		if (attribute->GetName() != GetFrequencyAttribute())
		{
			// Nombre de valeur de l'attribut si ses statistiques descriptives sont disponible
			descriptiveStats = cast(KWDescriptiveStats*, odDescriptiveStats.Lookup(attribute->GetName()));
			if (descriptiveStats != NULL)
				nValueNumber = descriptiveStats->GetValueNumber();
			// Estimation sinon
			else
				nValueNumber = (int)sqrt(nTupleNumber);

			// Prise en compte de la taille de stockage de l'attribut, de ses parties et valeurs
			dMaxCellNumber *= nValueNumber;
			lInitialDataGridSize += sizeof(KWDGAttribute) + sizeof(void*);
			if (attribute->GetType() == KWType::Continuous)
				lInitialDataGridSize +=
				    nValueNumber * (sizeof(KWDGMPart) + sizeof(KWDGInterval) + sizeof(void*));
			else if (attribute->GetType() == KWType::Symbol)
				lInitialDataGridSize += nValueNumber * (sizeof(KWDGMPart) + sizeof(KWDGValueSet) +
									sizeof(KWDGValue) + 2 * sizeof(void*));
		}
	}
	if (dMaxCellNumber > nTupleNumber)
		dMaxCellNumber = nTupleNumber;
	lSizeOfCell = sizeof(KWDGMCell) + (2 + GetClass()->GetLoadedAttributeNumber()) * sizeof(void*);
	lInitialDataGridSize += (int)ceil(sqrt(dMaxCellNumber)) * nAttributeNumber * lSizeOfCell;
	lNecessaryMemory += lInitialDataGridSize;

	// Plus une grille de travail, et une pour la meilleure solution (de taille estimee minimale)
	lWorkingDataGridSize = sizeof(KWDataGridMerger) + nAttributeNumber * sizeof(KWDGMAttribute) +
			       (int)ceil(sqrt(dMaxCellNumber)) * nAttributeNumber *
				   (lSizeOfCell + sizeof(KWDGMPart) + sizeof(KWDGMPartMerge) + sizeof(KWDGInterval) +
				    sizeof(KWDGValueSet) + sizeof(KWDGValue));
	lWorkingDataGridSize *= 2;
	lNecessaryMemory += lWorkingDataGridSize;

	// Estimation du nombre max de cellules que l'on peut charger en memoire, pour les grilles initiales, de
	// travail, et finales
	nMaxCellNumber = 0;
	if (lNecessaryMemory < lAvailableMemory)
		nMaxCellNumber = (int)((lAvailableMemory - lNecessaryMemory) / (3 * lSizeOfCell));

	// Affichage de stats memoire
	if (bDisplayMemoryStats)
	{
		cout << "CheckMemoryForDataGridInitialization" << endl;
		cout << "\tInitial data grid: " << lInitialDataGridSize << endl;
		cout << "\tWorking data grid: " << lWorkingDataGridSize << endl;
		cout << "\tMax cell number: " << nMaxCellNumber << endl;
		cout << "\t  Necessary: " << lNecessaryMemory << endl;
		cout << "\t  Available: " << lAvailableMemory << endl;
		cout << "\t  OK: " << (lNecessaryMemory <= lAvailableMemory) << endl;
	}

	// Test si memoire suffisante
	if (lNecessaryMemory > lAvailableMemory)
	{
		sMessage = "Not enough memory to create initial data grid ";
		if (database->IsOpenedForRead() and database->GetReadPercentage() < 0.99)
			sMessage += sTmp + "after reading " + IntToString((int)(100 * database->GetReadPercentage())) +
				    "% of the database";
		else
			sMessage += RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory);
		AddError(sMessage);
		AddMessage(RMResourceManager::BuildMemoryLimitMessage());
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
	boolean bDisplayMemoryStats = false;
	longint lAvailableMemory;
	longint lNecessaryMemory;
	longint lDataGridPostOptimizationSize;
	longint lWorkingDataGridSize;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	longint lTotalValueNumber;
	longint lInitialMaxPartNumber;
	longint lMaxPartNumber;
	longint lPartNumber;
	longint lTotalPartMergeNumber;
	longint lSizeOfCell;

	require(inputInitialDataGrid != NULL);
	require(inputInitialDataGrid->GetAttributeNumber() > 0);

	// Calcul des caracteristiques memoires disponibles, la grille initiale etant deja chargee en memoire
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Calcul de stats sur la grille initiale
	lTotalValueNumber = 0;
	lInitialMaxPartNumber = 1;
	for (nAttribute = 0; nAttribute < inputInitialDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = inputInitialDataGrid->GetAttributeAt(nAttribute);

		// Nombre de partie maximale
		lPartNumber = dgAttribute->GetPartNumber();
		if (lPartNumber > lInitialMaxPartNumber)
			lInitialMaxPartNumber = lPartNumber;

		// Nombre total de valeurs
		if (dgAttribute->GetAttributeType() == KWType::Symbol)
			lTotalValueNumber += dgAttribute->GetStoredValueNumber();
	}

	// Prise en compte d'une grille de travail et d'une grille pour la meilleure solution
	lNecessaryMemory = 0;
	lMaxPartNumber = 1 + (int)ceil(pow(inputInitialDataGrid->GetGridFrequency(),
					   1.0 / inputInitialDataGrid->GetAttributeNumber()));
	lWorkingDataGridSize = sizeof(KWDataGridMerger) + sizeof(void*);
	lWorkingDataGridSize += sizeof(KWDataGrid) + sizeof(void*);
	lTotalPartMergeNumber = 0;
	for (nAttribute = 0; nAttribute < inputInitialDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = inputInitialDataGrid->GetAttributeAt(nAttribute);

		// Prise en compte de la taille de stockage de l'attribut, de ses parties et valeurs
		lWorkingDataGridSize += 2 * (sizeof(KWDGMAttribute) + sizeof(void*));
		lPartNumber = dgAttribute->GetPartNumber();
		if (lPartNumber > lMaxPartNumber)
			lPartNumber = lMaxPartNumber;
		lTotalPartMergeNumber += (lPartNumber * (lPartNumber - 1)) / 2;
		if (dgAttribute->GetAttributeType() == KWType::Continuous)
			lWorkingDataGridSize +=
			    2 * lPartNumber * (sizeof(KWDGMPart) + sizeof(KWDGInterval) + sizeof(void*));
		else if (dgAttribute->GetAttributeType() == KWType::Symbol)
			lWorkingDataGridSize += 2 * dgAttribute->GetPartNumber() *
						(sizeof(KWDGMPart) + sizeof(KWDGValueSet) + 2 * sizeof(void*));
	}
	lSizeOfCell = sizeof(KWDGMCell) + (2 + GetClass()->GetLoadedAttributeNumber()) * sizeof(void*);
	lWorkingDataGridSize += inputInitialDataGrid->GetCellNumber() * lSizeOfCell +
				lTotalValueNumber * (sizeof(KWDGValue) + sizeof(void*));
	lWorkingDataGridSize += inputInitialDataGrid->GetCellNumber() * lSizeOfCell +
				lTotalValueNumber * (sizeof(KWDGValue) + sizeof(void*));
	lWorkingDataGridSize += lTotalPartMergeNumber * (sizeof(KWDGMPartMerge) + 2 * sizeof(void*));
	lNecessaryMemory += lWorkingDataGridSize;

	// Prise en compte de la memoire de travail pour post-optimisation
	lDataGridPostOptimizationSize =
	    inputInitialDataGrid->GetCellNumber() *
	    (sizeof(KWDGMCell) + (2 + GetClass()->GetLoadedAttributeNumber()) * sizeof(void*));
	lDataGridPostOptimizationSize += inputInitialDataGrid->GetCellNumber() * sizeof(KWDGPOCellFrequencyVector);
	lDataGridPostOptimizationSize += lInitialMaxPartNumber * (sizeof(KWMODLLineDeepOptimization) + 2 * sizeof(int) +
								  sizeof(KWDGPOPartFrequencyVector));
	lNecessaryMemory += lDataGridPostOptimizationSize;

	// Affichage de stats memoire
	if (bDisplayMemoryStats)
	{
		cout << "CheckMemoryForDataGridOptimization" << endl;
		cout << "\tWorking data grid: " << lWorkingDataGridSize << endl;
		cout << "\tData grid post-optimization: " << lDataGridPostOptimizationSize << endl;
		cout << "\t  Necessary: " << lNecessaryMemory << endl;
		cout << "\t  Available: " << lAvailableMemory << endl;
		cout << "\t  OK: " << (lNecessaryMemory <= lAvailableMemory) << endl;
	}

	// Test si memoire suffisante
	if (lNecessaryMemory > lAvailableMemory)
	{
		AddError("Not enough memory to optimize data grid " +
			 RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
		AddMessage(RMResourceManager::BuildMemoryLimitMessage());
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
	dAnyTimeDefaultCost = 0;
	dAnyTimeBestCost = 0;
	tAnyTimeTimer.Reset();
	tAnyTimeTimer.Start();
	sLastActualAnyTimeReportFileName = "";
	bIsDefaultCostComputed = false;
}

void CCCoclusteringBuilder::AnyTimeStop() const
{
	ALString sReportFileName;

	// Initialisations (sauf nom du dernier fichier temporaire, potentiellement a detruire)
	nAnyTimeOptimizationIndex = 0;
	dAnyTimeDefaultCost = 0;
	dAnyTimeBestCost = 0;
	tAnyTimeTimer.Stop();
	bIsDefaultCostComputed = false;
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

void CCCoclusteringBuilder::ComputeDescriptiveAttributeStats(const KWTupleTable* tupleTable,
							     ObjectDictionary* odOutputDescriptiveStats) const
{
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

		// Cas des attributs simples, hors attribut d'effectif
		if (attribute->GetName() != GetFrequencyAttribute() and KWType::IsSimple(attribute->GetType()) and
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

	// Calcul de la typicalite des valeurs des attribut categoriels
	ComputeValueTypicalities(inputInitialDataGrid, dataGridCosts, optimizedDataGrid);

	////////////////////////////////////////////////////////////////////////////
	// Calcul de caracteristiques des parties du coclustering
	// en evaluant l'impact des fusions de groupes

	// Creation d'un KWDataGridMerger pour l'evaluation des fusions entre groupes du coclustering
	dataGridManager.SetSourceDataGrid(optimizedDataGrid);
	dataGridManager.ExportDataGrid(&dataGridMerger);

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
	optimizedDataGrid->SetFrequencyAttributeName(GetFrequencyAttribute());
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

	require(optimizedDataGrid != NULL);

	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		hdgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Memorisation du nombre de parties initiales, eagl ici au nombre de parties
		hdgAttribute->SetInitialPartNumber(hdgAttribute->GetPartNumber());

		// Pour l'instant, on met toutes les typicalites a 1
		hdgAttribute->SetInterest(1);
	}
}

void CCCoclusteringBuilder::ComputeContinuousAttributeBounds(CCHierarchicalDataGrid* optimizedDataGrid) const
{
	int nAttribute;
	CCHDGAttribute* hdgAttribute;
	KWDescriptiveContinuousStats* descriptiveContinuousStats;

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
	boolean bDisplay = false;
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
	CCHDGValue* hdgValue;
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
	dataGridManager.SetSourceDataGrid(univariateInitialDataGrid);
	assert(dataGridManager.CheckDataGrid(optimizedDataGrid));

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
			nkdOptimizedAttributeValues.SetAt((NUMERIC)(dgValue->GetValue().GetNumericKey()), dgValue);
			dgValuePart->GetValueSet()->GetNextValue(dgValue);
		}

		// Memorisation du nombre de modalites du groupe
		ivGroupModalityNumber.Add(dgValuePart->GetValueSet()->GetTrueValueNumber());

		// Cas du groupe poubelle : memorisation du nombre de modalites
		if (nGroup == nGarbageGroupIndex)
		{
			nGarbageModalityNumber = dgValuePart->GetValueSet()->GetTrueValueNumber();
			// CH AB AF adaptation eventuelle VarParts
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
	if (bDisplay)
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

				assert(dDeltaCost >= 0);

				// Cumul de la typicite
				dTypicality += dDeltaCost;

				// Affichage de resultats: ligne de detail
				if (bDisplay)
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
				hdgValue = cast(CCHDGValue*, nkdOptimizedAttributeValues.Lookup(
								 (NUMERIC)(dgValue->GetValue().GetNumericKey())));

				// Calcul de la typicite elementaire = la typicite du fourre-tout * effectif  de la
				// modalite elementaire / effectif total du fourre-tout
				dElementaryTypicality = (dTypicality * hdgValue->GetValueFrequency()) /
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
		       dgValuePart->GetValueSet()->GetTailValue()->GetValue() == Symbol::GetStarValue());

		// Acces au nombre de modalites de la valeur
		nValueModalityNumber = initialFrequencyTable.GetFrequencyVectorAt(nValue)->GetModalityNumber();

		// Recherche du groupe de rattachement de la modalite
		nOutGroup = ivGroups.GetAt(nValue);

		// Cas d'une modalite non fourre-tout
		if (dgValuePart->GetValueSet()->GetTrueValueNumber() == 1)
		{
			dgValue = dgValuePart->GetValueSet()->GetHeadValue();

			// Recherche de la valeur correspondante pour l'attribut optimise
			hdgValue =
			    cast(CCHDGValue*,
				 nkdOptimizedAttributeValues.Lookup((NUMERIC)(dgValue->GetValue().GetNumericKey())));

			// Memorisation de la typicalite normalisee
			assert(0 <= dvTypicalities.GetAt(nValue) and
			       dvTypicalities.GetAt(nValue) <= dvMaxTypicalities.GetAt(nOutGroup));

			if (dvMaxTypicalities.GetAt(nOutGroup) > 0)
				hdgValue->SetTypicality(dvTypicalities.GetAt(nValue) /
							dvMaxTypicalities.GetAt(nOutGroup));
			else
				hdgValue->SetTypicality(1);
		}
		// Sinon : cas du fourre-tout
		else
		{
			dgValue = dgValuePart->GetValueSet()->GetHeadValue();

			for (nIntraCatchAllValue = 0; nIntraCatchAllValue < nValueModalityNumber; nIntraCatchAllValue++)
			{
				// Recherche de la valeur correspondante pour l'attribut optimise
				hdgValue = cast(CCHDGValue*, nkdOptimizedAttributeValues.Lookup(
								 (NUMERIC)(dgValue->GetValue().GetNumericKey())));

				if (dvMaxTypicalities.GetAt(nOutGroup) > 0)
					hdgValue->SetTypicality(dvTypicalities.GetAt(nValue + nIntraCatchAllValue) /
								dvMaxTypicalities.GetAt(nOutGroup));
				else
					hdgValue->SetTypicality(1);

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
	boolean bDisplay = false;
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
		if (bDisplay)
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
				dInterest = dDeltaCost / (dTotalDefaultCost - dBestDataGridTotalCost);
				dTotalInterest += dInterest;

				// Affichage des resultats: details
				if (bDisplay)
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
		debug(part2 = dgAttribute->GetHeadPart();) nPart = 0;
		while (part1 != NULL)
		{
			hdgPart = cast(CCHDGPart*, part1);
			debug(assert(part1->GetObjectLabel() == part2->GetObjectLabel());)

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
			debug(dgAttribute->GetNextPart(part2);) nPart++;
		}
	}
}

void CCCoclusteringBuilder::ComputePartHierarchies(KWDataGridMerger* optimizedDataGridMerger,
						   const KWDataGridCosts* dataGridCosts,
						   CCHierarchicalDataGrid* optimizedDataGrid) const
{
	boolean bDisplay = false;
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
			nkdHierarchicalParts.SetAt((NUMERIC)dgPart, hdgPart);

			// Partie suivante
			dgAttribute->GetNextPart(dgPart);
			hdgAttribute->GetNextPart(dgPart2);
			nPart++;
		}
	}
	assert(nkdHierarchicalParts.GetCount() == optimizedDataGridMerger->GetTotalPartNumber());

	// Cout par defaut et meilleur cout
	dTotalDefaultCost = dataGridCosts->GetTotalDefaultCost();
	dDataGridTotalCost =
	    optimizedDataGridMerger->GetDataGridCosts()->ComputeDataGridMergerTotalCost(optimizedDataGridMerger);
	dBestDataGridTotalCost = dDataGridTotalCost;

	// Affichage: entete
	if (bDisplay)
		cout << "\nCount\tAttribute\tPart1\tPart2\tBest\tMerge\tCost\tDefault cost\tHierachicalLevel\n";

	// Boucle de recherche d'ameliorations
	nCount = 0;
	bContinue = true;
	while (bContinue)
	{
		nCount++;

		// Recherche de la meilleure amelioration
		dBestDeltaCost = optimizedDataGridMerger->SearchBestPartMergeWithGarbageSearch(bestPartMerge);
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
			if (bDisplay)
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
			hdgPart = cast(CCHDGPart*, nkdHierarchicalParts.Lookup((NUMERIC)bestPartMerge->GetPart1()));
			hdgPart->SetParentPart(hdgParentPart);
			hdgParentPart->SetChildPart1(hdgPart);
			hdgPart = cast(CCHDGPart*, nkdHierarchicalParts.Lookup((NUMERIC)bestPartMerge->GetPart2()));
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
			nkdHierarchicalParts.SetAt((NUMERIC)dgmMergedPart, hdgParentPart);
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
				cast(CCHDGValueSet*, dgPart->GetValueSet())->SortValuesByTypicality();

			// Initialisation du nom de la partie
			cast(CCHDGPart*, dgPart)->SetPartName(dgPart->GetObjectLabel());

			// Partie suivante
			dgAttribute->GetNextPart(dgPart);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
// Classe CCCoclusteringOptimizer

CCCoclusteringOptimizer::CCCoclusteringOptimizer()
{
	coclusteringBuilder = NULL;
}

CCCoclusteringOptimizer::~CCCoclusteringOptimizer() {}

void CCCoclusteringOptimizer::SetCoclusteringBuilder(const CCCoclusteringBuilder* builder)
{
	coclusteringBuilder = builder;
}

const CCCoclusteringBuilder* CCCoclusteringOptimizer::GetCoclusteringBuilder()
{
	return coclusteringBuilder;
}

void CCCoclusteringOptimizer::HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
						     const KWDataGrid* initialGranularizedDataGrid,
						     boolean bIsLastSaving) const
{
	// Integration de la granularite
	if (coclusteringBuilder != NULL)
		coclusteringBuilder->HandleOptimizationStep(optimizedDataGrid, initialGranularizedDataGrid,
							    bIsLastSaving);
}