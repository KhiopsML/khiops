// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseBasicStatsTask.h"

KWDatabaseBasicStatsTask::KWDatabaseBasicStatsTask()
{
	slaveTargetAttribute = NULL;
	masterTargetAttribute = NULL;
	bMasterCollectValues = false;
	lMasterAllValuesGrantedMemory = 0;
	lMasterAllValuesUsedMemory = 0;
	lSlaveValuesUsedMemory = 0;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_sTargetAttributeName);
	DeclareSharedParameter(&shared_bCollectValues);
	DeclareSharedParameter(&shared_lSlaveValuesMaxMemory);

	// Variables en entree et sortie des esclaves
	DeclareTaskOutput(&output_svReadValues);
	DeclareTaskOutput(&output_cvReadValues);
}

KWDatabaseBasicStatsTask::~KWDatabaseBasicStatsTask()
{
	assert(oaAllSlaveResults.GetSize() == 0);
}

boolean KWDatabaseBasicStatsTask::CollectBasicStats(const KWDatabase* sourceDatabase,
						    const ALString& sTargetAttributeName, longint& lRecordNumber,
						    longint& lCollectedObjectNumber, SymbolVector* svCollectedValues,
						    ContinuousVector* cvCollectedValues)
{
	boolean bOk = true;
	KWClass* kwcClass;
	KWAttribute* targetAttribute;

	require(sourceDatabase != NULL);
	require(svCollectedValues != NULL);
	require(cvCollectedValues != NULL);

	// Recherche du dictionnaire de la base
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sourceDatabase->GetClassName());
	check(kwcClass);

	// Recherche de l'attribut cible
	targetAttribute = NULL;
	if (sTargetAttributeName != "")
	{
		// Recherche de l'attribut cible
		targetAttribute = kwcClass->LookupAttribute(sTargetAttributeName);
		check(targetAttribute);
		assert(KWType::IsSimple(targetAttribute->GetType()));
	}

	// On passe tous les attribut en Unload, sauf l'attribut cible
	kwcClass->SetAllAttributesLoaded(false);
	if (targetAttribute != NULL)
		targetAttribute->SetLoaded(true);
	kwcClass->Compile();

	// Lancement de la tache
	shared_sTargetAttributeName.SetValue(sTargetAttributeName);
	bOk = RunDatabaseTask(sourceDatabase);

	// Collecte des resultats
	lRecordNumber = 0;
	lCollectedObjectNumber = 0;
	svCollectedValues->SetSize(0);
	cvCollectedValues->SetSize(0);
	if (bOk)
	{
		lRecordNumber = GetReadRecords();
		lCollectedObjectNumber = GetReadObjects();
		if (targetAttribute != NULL and lCollectedObjectNumber > 0)
		{
			svCollectedValues->CopyFrom(&svReadValues);
			cvCollectedValues->CopyFrom(&cvReadValues);
			assert(svCollectedValues->GetSize() + cvCollectedValues->GetSize() == lCollectedObjectNumber);
			assert(svCollectedValues->GetSize() == 0 or cvCollectedValues->GetSize() == 0);
		}
	}
	// On passe tous les attribut en Load
	kwcClass->SetAllAttributesLoaded(true);
	kwcClass->Compile();

	// Nettoyage
	shared_sTargetAttributeName.SetValue("");
	svReadValues.SetSize(0);
	cvReadValues.SetSize(0);
	return bOk;
}

const ALString KWDatabaseBasicStatsTask::GetTaskName() const
{
	return "Database basic stats";
}

PLParallelTask* KWDatabaseBasicStatsTask::Create() const
{
	return new KWDatabaseBasicStatsTask;
}

boolean KWDatabaseBasicStatsTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	PLDatabaseTextFile* sourceDatabase;
	longint lSlaveValuesMinRequiredMemory;
	longint lSlaveValuesMaxRequiredMemory;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::ComputeResourceRequirements();

	// Memorisation des ressources initiales demandee pour le maitre et l'esclave
	databaseTaskMasterMemoryRequirement.CopyFrom(GetResourceRequirements()->GetMasterRequirement()->GetMemory());
	databaseTaskSlaveMemoryRequirement.CopyFrom(GetResourceRequirements()->GetSlaveRequirement()->GetMemory());
	assert(databaseTaskMasterMemoryRequirement.GetMax() < LLONG_MAX);

	// Acces a la base source
	sourceDatabase = shared_sourceDatabase.GetPLDatabase();

	// On ajoute la taille d'un buffer pour le stockage des valeurs cibles par esclave, qui dans le pire des cas ne
	// contiendra que les cles On rajoute une taille de buffer standard pour prendre en compte la taille des
	// structures
	lSlaveValuesMinRequiredMemory = sourceDatabase->GetMinOpenNecessaryMemory() -
					sourceDatabase->GetEmptyOpenNecessaryMemory() +
					InputBufferedFile::nDefaultBufferSize;
	lSlaveValuesMaxRequiredMemory = sourceDatabase->GetMaxOpenNecessaryMemory() -
					sourceDatabase->GetEmptyOpenNecessaryMemory() +
					InputBufferedFile::nDefaultBufferSize;
	if (bOk and shared_sTargetAttributeName.GetValue() != "")
	{
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(
		    lSlaveValuesMinRequiredMemory);
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(
		    lSlaveValuesMaxRequiredMemory);
	}

	// On donne pour le maitre une capacite de stockage importante pour les valeurs cibles
	if (bOk and shared_sTargetAttributeName.GetValue() != "")
	{
		// On demande au minimum la meme chose que pour un esclave
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMin(
		    lSlaveValuesMinRequiredMemory);
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMax(
		    lSlaveValuesMinRequiredMemory);

		// Et au maximum la moitie de ce qui est disponible
		// Pas plus, car il faudra de toute facon faire bien d'autres traitements
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMax(
		    max(GetResourceRequirements()->GetMasterRequirement()->GetMemory()->GetMax(),
			RMResourceManager::GetRemainingAvailableMemory() / 2));
	}

	// On chosi une politique d'allocation des ressources equilibre entre le maitre et les esclaves, ce qui permet a
	// chacun de beneficier de davantage de ressources. A noter que l'on a quand meme demande un max potentiel tres
	// important pour le maitre, ce qui tend a le privilegier.
	GetResourceRequirements()->SetMemoryAllocationPolicy(RMTaskResourceRequirement::balanced);
	return bOk;
}

boolean KWDatabaseBasicStatsTask::MasterInitialize()
{
	boolean bOk = true;
	KWClass* kwcClass;
	longint lMasterGrantedMemory;
	longint lSlaveGrantedMemory;
	ALString sTmp;

	require(masterTargetAttribute == NULL);
	require(oaAllSlaveResults.GetSize() == 0);

	// Appel de la methode ancetre
	if (bOk)
		bOk = KWDatabaseTask::MasterInitialize();

	// Recherche du dictionnaire de la base
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(shared_sourceDatabase.GetDatabase()->GetClassName());
	check(kwcClass);

	// Recherche de l'attribut cible
	masterTargetAttribute = NULL;
	if (shared_sTargetAttributeName.GetValue() != "")
	{
		// Recherche de l'attribut cible
		masterTargetAttribute = kwcClass->LookupAttribute(shared_sTargetAttributeName.GetValue());
		check(masterTargetAttribute);
		assert(KWType::IsSimple(masterTargetAttribute->GetType()));
	}

	// Initialisation des resultats
	svReadValues.SetSize(0);
	cvReadValues.SetSize(0);
	nkdAllSymbolValues.RemoveAll();
	bMasterCollectValues = false;
	lMasterAllValuesGrantedMemory = 0;
	lMasterAllValuesUsedMemory = 0;

	// Initialisation des variables partagee
	shared_bCollectValues = false;
	shared_lSlaveValuesMaxMemory = 0;

	// Parametrage specifique dans le cas de la collecte des valeurs
	if (masterTargetAttribute != NULL)
	{
		bMasterCollectValues = true;
		shared_bCollectValues = true;

		// Parametrage de la memoire disponible pour la collecte des valeurs par le maitre
		lMasterGrantedMemory = GetMasterResourceGrant()->GetMemory();
		lMasterAllValuesGrantedMemory = ComputeTaskSelfMemory(
		    lMasterGrantedMemory, GetResourceRequirements()->GetMasterRequirement()->GetMemory(),
		    &databaseTaskMasterMemoryRequirement);

		// Parametrage de la memoire disponible pour la collecte des valeurs par chaque esclave, dans le cas
		// parallel
		if (GetTaskResourceGrant()->GetSlaveNumber() > 1)
		{
			lSlaveGrantedMemory = GetTaskResourceGrant()->GetSlaveMemory();
			shared_lSlaveValuesMaxMemory = ComputeTaskSelfMemory(
			    lSlaveGrantedMemory, GetResourceRequirements()->GetSlaveRequirement()->GetMemory(),
			    &databaseTaskSlaveMemoryRequirement);
		}
		// Dans le cas sequentiel, on donne a l'esclave toute la memoire reservee au maitre
		// En effet, dans ce cas, il n'y a pas d'indexation prealabe de la base et l'unique esclave doit traiter
		// l'integralite des valeurs de la base avant qu'elles soit recopiee dans le maitre
		else
			shared_lSlaveValuesMaxMemory = lMasterAllValuesGrantedMemory;
	}
	return bOk;
}

boolean KWDatabaseBasicStatsTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);

	// Cas de la collecte des valeurs cibles
	if (bOk and not bIsTaskFinished and bMasterCollectValues)
	{
		// On alloue le vecteur de resultat de l'esclave, a une position selon son rang
		if (masterTargetAttribute->GetType() == KWType::Symbol)
			oaAllSlaveResults.Add(new SymbolVector);
		else if (masterTargetAttribute->GetType() == KWType::Continuous)
			oaAllSlaveResults.Add(new ContinuousVector);
	}
	return bOk;
}

boolean KWDatabaseBasicStatsTask::MasterAggregateResults()
{
	boolean bOk;
	SymbolVector* svSlaveValues;
	ContinuousVector* cvSlaveValues;
	int nDistinctValueNumber;
	int nValue;
	longint lParsedObjects;
	longint lInputFileSize;
	LongintVector lvChunkEndPos;
	double dOveralTaskPercent;
	ALString sTmp;

	require(shared_sTargetAttributeName.GetValue() == "" or masterTargetAttribute != NULL);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterAggregateResults();

	// Erreur si trop d'enregistrements
	if (bOk and lReadObjects > INT_MAX)
	{
		// Taille du fichier a traiter
		lInputFileSize = databaseChunkBuilder.GetDatabaseIndexer()->GetPLDatabase()->GetFileSizeAt(0);

		// Calcul du pourcentage du fichier traite, dans le cas multi-table
		if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
		{
			databaseChunkBuilder.GetChunkEndPositionsAt(GetTaskIndex(), &lvChunkEndPos);
			dOveralTaskPercent = (lvChunkEndPos.GetAt(0) * 1.0) / lInputFileSize;
		}
		// et dans le cas mono-table
		else
		{
			dOveralTaskPercent =
			    (databaseChunkBuilder.GetChunkEndPositionAt(GetTaskIndex()) * 1.0) / lInputFileSize;
		}

		// Message d'erreur
		AddSimpleMessage(sTmp + "Input database " + shared_sourceDatabase.GetDatabase()->GetDatabaseName() +
				 ": " + LongintToHumanReadableString(lInputFileSize));
		AddError(sTmp + "Too many selected records after reading " +
			 DoubleToString(int(1000 * dOveralTaskPercent) / 10) +
			 "% of the input file (beyond the maximum two billions records that could fit in memory)");
		bOk = false;
	}

	// Collecte eventuelle des valeurs
	if (bOk and masterTargetAttribute != NULL and bMasterCollectValues)
	{
		// Cas categoriel
		if (masterTargetAttribute->GetType() == KWType::Symbol)
		{
			// Recherche des resultats selon le rang de l'esclave
			svSlaveValues = cast(SymbolVector*, oaAllSlaveResults.GetAt(GetTaskIndex()));
			assert(svSlaveValues->GetSize() == 0);

			// Prise en compte de la memoire de stockage du vecteur de valeurs
			svSlaveValues->SetSize(output_svReadValues.GetSize());

			// Erreur si depassement de la memoire disponible
			lMasterAllValuesUsedMemory += sizeof(SymbolVector*) + svSlaveValues->GetUsedMemory();
			if (lMasterAllValuesUsedMemory > lMasterAllValuesGrantedMemory)
			{
				AddError("Too much memory necessary to store the values of the target variable " +
					 masterTargetAttribute->GetName() + " (more than " +
					 RMResourceManager::ActualMemoryToString(lMasterAllValuesUsedMemory) +
					 " used after reading " + LongintToReadableString(lReadObjects) + " records)");
				bMasterCollectValues = false;
				bOk = false;
			}

			// Collecte des valeurs
			if (bOk)
			{
				for (nValue = 0; nValue < output_svReadValues.GetSize(); nValue++)
				{
					svSlaveValues->SetAt(nValue, (Symbol)output_svReadValues.GetAt(nValue));

					// Ajout de la valeur dans le dictionnaire pour les comptabiliser
					nDistinctValueNumber = nkdAllSymbolValues.GetCount();
					nkdAllSymbolValues.SetAt(svSlaveValues->GetAt(nValue).GetNumericKey(),
								 svSlaveValues);

					// Test si depassement de memoire disponible en cas de nouveau symbol
					if (nkdAllSymbolValues.GetCount() > nDistinctValueNumber)
					{
						nDistinctValueNumber++;
						lMasterAllValuesUsedMemory +=
						    svSlaveValues->GetAt(nValue).GetUsedMemory();
						lMasterAllValuesUsedMemory +=
						    nkdAllSymbolValues.GetUsedMemoryPerElement();
						if (lMasterAllValuesUsedMemory > lMasterAllValuesGrantedMemory)
						{
							lParsedObjects =
							    lReadObjects - output_svReadValues.GetSize() + nValue + 1;
							AddError("Too much memory necessary to store the values of the "
								 "target variable " +
								 masterTargetAttribute->GetName() + " (more than " +
								 RMResourceManager::ActualMemoryToString(
								     lMasterAllValuesUsedMemory) +
								 " used for " + IntToString(nDistinctValueNumber) +
								 " distinct values after parsing " +
								 LongintToReadableString(lParsedObjects) + " records)");
							bMasterCollectValues = false;
							bOk = false;
							break;
						}
					}

					// Test si depassement du nombre max de valeur distincte
					if (nkdAllSymbolValues.GetCount() > nMaxSymbolValueNumber)
					{
						lParsedObjects =
						    lReadObjects - output_svReadValues.GetSize() + nValue + 1;
						AddError("Too many distinct values collected for target variable " +
							 masterTargetAttribute->GetName() + " (more than " +
							 IntToString(nMaxSymbolValueNumber) + " values after parsing " +
							 LongintToReadableString(lParsedObjects) + " records)");
						bOk = false;
						bMasterCollectValues = false;
						break;
					}
				}
			}
		}
		// Cas numerique
		else if (masterTargetAttribute->GetType() == KWType::Continuous)
		{
			// Recherche des resultats selon le rang de l'esclave
			cvSlaveValues = cast(ContinuousVector*, oaAllSlaveResults.GetAt(GetTaskIndex()));
			assert(cvSlaveValues->GetSize() == 0);

			// Prise en compte de la memoire de stockage du vecteur de valeurs
			cvSlaveValues->SetSize(output_cvReadValues.GetSize());

			// Erreur si depassement de la memoire disponible
			lMasterAllValuesUsedMemory += sizeof(ContinuousVector*) + cvSlaveValues->GetUsedMemory();
			if (lMasterAllValuesUsedMemory > lMasterAllValuesGrantedMemory)
			{
				AddError("Too much memory necessary to store the values of the target variable " +
					 masterTargetAttribute->GetName() + " (more than " +
					 RMResourceManager::ActualMemoryToString(lMasterAllValuesUsedMemory) +
					 " used after reading " + LongintToReadableString(lReadObjects) + " records)");
				bMasterCollectValues = false;
				bOk = false;
			}

			// Collecte des valeurs
			if (bOk)
			{
				for (nValue = 0; nValue < output_cvReadValues.GetSize(); nValue++)
				{
					cvSlaveValues->SetAt(nValue, (Continuous)output_cvReadValues.GetAt(nValue));
				}
			}
		}
	}

	// Reinitialisation des vecteurs de resultats en cas de probleme
	if (not bOk)
	{
		bMasterCollectValues = false;
		nkdAllSymbolValues.RemoveAll();
		oaAllSlaveResults.DeleteAll();
	}

	return bOk;
}

boolean KWDatabaseBasicStatsTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;
	int nSlaveRank;
	SymbolVector* svSlaveValues;
	ContinuousVector* cvSlaveValues;
	int nObject;
	int nValue;

	require(svReadValues.GetSize() == 0);
	require(cvReadValues.GetSize() == 0);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterFinalize(bProcessEndedCorrectly);

	// Collecte des valeurs de l'ensemble de tous les esclaves
	if (bOk and masterTargetAttribute != NULL)
	{
		assert(lReadObjects <= INT_MAX);
		assert(lMasterAllValuesUsedMemory <= lMasterAllValuesGrantedMemory);

		// Cas categoriel
		nkdAllSymbolValues.RemoveAll();
		if (masterTargetAttribute->GetType() == KWType::Symbol)
		{
			nObject = 0;
			for (nSlaveRank = 0; nSlaveRank < oaAllSlaveResults.GetSize(); nSlaveRank++)
			{
				svSlaveValues = cast(SymbolVector*, oaAllSlaveResults.GetAt(nSlaveRank));

				// Collecte des valeurs
				svReadValues.SetSize(svReadValues.GetSize() + svSlaveValues->GetSize());
				for (nValue = 0; nValue < svSlaveValues->GetSize(); nValue++)
				{
					svReadValues.SetAt(nObject, svSlaveValues->GetAt(nValue));
					nObject++;
				}
			}
		}
		// Cas numerique
		if (masterTargetAttribute->GetType() == KWType::Continuous)
		{
			nObject = 0;
			for (nSlaveRank = 0; nSlaveRank < oaAllSlaveResults.GetSize(); nSlaveRank++)
			{
				cvSlaveValues = cast(ContinuousVector*, oaAllSlaveResults.GetAt(nSlaveRank));

				// Collecte des valeurs
				cvReadValues.SetSize(cvReadValues.GetSize() + cvSlaveValues->GetSize());
				for (nValue = 0; nValue < cvSlaveValues->GetSize(); nValue++)
				{
					cvReadValues.SetAt(nObject, cvSlaveValues->GetAt(nValue));
					nObject++;
				}
				cvSlaveValues->SetSize(0);
			}
		}
	}

	// Nettoyage
	nkdAllSymbolValues.RemoveAll();
	oaAllSlaveResults.DeleteAll();
	masterTargetAttribute = NULL;
	bMasterCollectValues = false;
	lMasterAllValuesGrantedMemory = 0;
	lMasterAllValuesUsedMemory = 0;
	return bOk;
}

boolean KWDatabaseBasicStatsTask::SlaveInitialize()
{
	boolean bOk;
	KWClass* kwcClass;

	require(slaveTargetAttribute == NULL);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveInitialize();

	// Recherche du dictionnaire de la base
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(shared_sourceDatabase.GetDatabase()->GetClassName());
	check(kwcClass);

	// Recherche de l'attribut cible
	slaveTargetAttribute = NULL;
	if (shared_sTargetAttributeName.GetValue() != "")
	{
		// Recherche de l'attribut cible
		slaveTargetAttribute = kwcClass->LookupAttribute(shared_sTargetAttributeName.GetValue());
		check(slaveTargetAttribute);
		assert(KWType::IsSimple(slaveTargetAttribute->GetType()));
	}
	return bOk;
}

boolean KWDatabaseBasicStatsTask::SlaveProcess()
{
	boolean bOk;

	// Initialisation de la memoire dediie a la collecte des valeursd
	lSlaveValuesUsedMemory = 0;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcess();
	return bOk;
}

boolean KWDatabaseBasicStatsTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	boolean bOk = true;
	Symbol sValue;
	longint lValueMemory;
	longint lValueNumber;

	// Appel de la methode ancetre
	if (slaveTargetAttribute != NULL and shared_bCollectValues == true)
	{
		lValueMemory = 0;
		lValueNumber = 0;
		if (slaveTargetAttribute->GetType() == KWType::Symbol)
		{
			sValue = kwoObject->GetSymbolValueAt(slaveTargetAttribute->GetLoadIndex()).GetValue();
			lValueMemory = sizeof(ALString*) + sizeof(ALString) + sValue.GetLength();

			// Prise en compte de la valeur si pas de depassement memoire
			bOk = lSlaveValuesUsedMemory + lValueMemory <= shared_lSlaveValuesMaxMemory;
			if (bOk)
				output_svReadValues.Add(sValue.GetValue());
			lValueNumber = output_svReadValues.GetSize();
		}
		else if (slaveTargetAttribute->GetType() == KWType::Continuous)
		{
			lValueMemory = sizeof(Continuous);

			// Prise en compte de la valeur si pas de depassement memoire
			bOk = lSlaveValuesUsedMemory + lValueMemory <= shared_lSlaveValuesMaxMemory;
			if (bOk)
				output_cvReadValues.Add(
				    (Continuous)kwoObject->GetContinuousValueAt(slaveTargetAttribute->GetLoadIndex()));
			lValueNumber = output_cvReadValues.GetSize();
		}

		// Incrementation de la memoire utilisee si ok
		if (bOk)
			lSlaveValuesUsedMemory += lValueMemory;
		// Message d'erreur sinon
		else
		{
			AddError("Too much memory necessary to store the values of the target variable " +
				 slaveTargetAttribute->GetName() + " (more than " +
				 RMResourceManager::ActualMemoryToString(lSlaveValuesUsedMemory) + " after parsing " +
				 LongintToReadableString(lValueNumber) + " records in slave process " +
				 IntToString(GetTaskIndex() + 1) + ")");
		}
	}
	return bOk;
}

boolean KWDatabaseBasicStatsTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveFinalize(bProcessEndedCorrectly);

	// Nettoyage
	slaveTargetAttribute = NULL;
	return bOk;
}

longint KWDatabaseBasicStatsTask::ComputeTaskSelfMemory(longint lTaskGrantedMemory,
							RMPhysicalResource* taskMemoryRequirement,
							RMPhysicalResource* parentTaskMemoryRequirement) const
{
	longint lDeltaMinMemory;
	longint lDeltaMaxMemory;
	longint lTaskMemoryRequirementRange;
	longint lTaskSelfMemory;
	double dMemoryRatio;

	require(taskMemoryRequirement != NULL);
	require(taskMemoryRequirement->GetMin() <= taskMemoryRequirement->GetMax());
	require(taskMemoryRequirement->GetMin() <= lTaskGrantedMemory and
		lTaskGrantedMemory <= taskMemoryRequirement->GetMax());
	require(parentTaskMemoryRequirement != NULL);
	require(parentTaskMemoryRequirement->GetMin() <= parentTaskMemoryRequirement->GetMax());

	// Calcul "delta" exigences de cette classe par rapport aux exigences de la classe ancetre
	lDeltaMinMemory = taskMemoryRequirement->GetMin() - parentTaskMemoryRequirement->GetMin();
	lDeltaMaxMemory = taskMemoryRequirement->GetMax() - parentTaskMemoryRequirement->GetMax();
	assert(0 < lDeltaMinMemory and lDeltaMinMemory <= lDeltaMaxMemory);

	// Si l'on a attribue le min a la tache alors on rends le delta min avec la classe ancetre
	if (lTaskGrantedMemory == taskMemoryRequirement->GetMin())
		lTaskSelfMemory = lDeltaMinMemory;
	// Si l'on a attribue le max a la tache alors on rends le delta max avec la classe ancetre
	else if (lTaskGrantedMemory == taskMemoryRequirement->GetMax())
		lTaskSelfMemory = lDeltaMaxMemory;
	// Sinon on rends au prorata du rang la memoire propre
	else
	{
		lTaskMemoryRequirementRange = taskMemoryRequirement->GetMax() - taskMemoryRequirement->GetMin();
		if (lTaskMemoryRequirementRange > 0)
		{
			dMemoryRatio =
			    double(lTaskGrantedMemory - taskMemoryRequirement->GetMin()) / lTaskMemoryRequirementRange;
			lTaskSelfMemory =
			    (longint)(lDeltaMinMemory + dMemoryRatio * (lDeltaMaxMemory - lDeltaMinMemory));
		}
		else
			lTaskSelfMemory = lDeltaMinMemory;
	}
	ensure(lTaskSelfMemory > 0);
	return lTaskSelfMemory;
}
