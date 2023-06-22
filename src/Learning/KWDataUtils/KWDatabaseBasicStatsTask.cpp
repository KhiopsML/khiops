// Copyright (c) 2023 Orange. All rights reserved.
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

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_sTargetAttributeName);

	// Variables en entree et sortie des esclaves
	DeclareTaskInput(&input_bCollectValues);
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

	// Messages de fin de tache
	DisplayTaskMessage();

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
	longint lMinMasterAdditionalMemory;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::ComputeResourceRequirements();

	// On ajoute la taille de deux buffers pour le stockage des valeurs cibles par esclave
	if (bOk and shared_sTargetAttributeName.GetValue() != "")
	{
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(
		    BufferedFile::nDefaultBufferSize * 2);
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(
		    BufferedFile::nDefaultBufferSize * 2);
	}

	// On donne pour le maitre une capacite de stockage importante pour les valeurs cibles
	if (bOk and shared_sTargetAttributeName.GetValue() != "")
	{
		// Calcul d'une memoire minimale additionnelle reservee au master
		lMinMasterAdditionalMemory = RMResourceManager::GetRemainingAvailableMemory() -
					     GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->GetMin() -
					     GetResourceRequirements()->GetMasterRequirement()->GetMemory()->GetMin();
		if (lMinMasterAdditionalMemory < 0)
			lMinMasterAdditionalMemory = 0;
		lMinMasterAdditionalMemory /= 10;

		// Parametrage de la memoire du master
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMin(lMinMasterAdditionalMemory);
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMax(
		    RMResourceManager::GetRemainingAvailableMemory() / 2);
	}
	return bOk;
}

boolean KWDatabaseBasicStatsTask::MasterInitialize()
{
	boolean bOk = true;
	KWClass* kwcClass;
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

	// On memorise les informations sur la collecte des valeurs
	if (masterTargetAttribute != NULL)
	{
		bMasterCollectValues = true;
		lMasterAllValuesGrantedMemory = GetMasterResourceGrant()->GetMemory();
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

	// On indique s'il faut collecter les stats
	input_bCollectValues = bMasterCollectValues;
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
	double dOveralTaskPercent;
	ALString sTmp;

	require(shared_sTargetAttributeName.GetValue() == "" or masterTargetAttribute != NULL);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterAggregateResults();

	// Erreur si trop d'enregistrements
	if (bOk and lReadObjects > INT_MAX)
	{
		// Calcul du pourcentage du fichier traite, dans le cas multi-table
		if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
		{
			lInputFileSize =
			    mtDatabaseIndexer.GetChunkEndPositionsAt(mtDatabaseIndexer.GetChunkNumber() - 1)->GetAt(0);
			dOveralTaskPercent =
			    (mtDatabaseIndexer.GetChunkEndPositionsAt(GetTaskIndex())->GetAt(0) * 1.0) / lInputFileSize;
		}
		// et dans le cas mono-table
		else
		{
			lInputFileSize = lvFileBeginPositions.GetAt(lvFileBeginPositions.GetSize() - 1);
			dOveralTaskPercent = (lvFileBeginPositions.GetAt(GetTaskIndex() + 1) * 1.0) / lInputFileSize;
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

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcess();
	return bOk;
}

boolean KWDatabaseBasicStatsTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	// Appel de la methode ancetre
	if (slaveTargetAttribute != NULL and input_bCollectValues == true)
	{
		if (slaveTargetAttribute->GetType() == KWType::Symbol)
			output_svReadValues.Add(
			    kwoObject->GetSymbolValueAt(slaveTargetAttribute->GetLoadIndex()).GetValue());
		else if (slaveTargetAttribute->GetType() == KWType::Continuous)
			output_cvReadValues.Add(
			    (Continuous)kwoObject->GetContinuousValueAt(slaveTargetAttribute->GetLoadIndex()));
	}
	return true;
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