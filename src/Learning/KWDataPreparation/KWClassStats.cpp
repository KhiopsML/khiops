// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClassStats.h"

// Includes uniquement dans l'implementation, pour eviter les cycle de dependance dans les headers
#include "KWDatabaseBasicStatsTask.h"
#include "KWDatabaseSlicerTask.h"
#include "KWDataPreparationUnivariateTask.h"
#include "KWDataPreparationBivariateTask.h"
#include "KDDataPreparationAttributeCreationTask.h"

KWClassStats::KWClassStats()
{
	attributePairSpec = NULL;
	attributeTreeConstructionReport = NULL;
	bWriteOptionStats1D = true;
	bWriteOptionStatsText = true;
	bWriteOptionStatsTrees = true;
	bWriteOptionStats2D = true;
	bWriteOptionDetailedStats = true;
	attributeTreeConstructionTask = NULL;
	dataTableSliceSet = NULL;
	svSymbolTargetValues = NULL;
	cvContinuousTargetValues = NULL;
}

KWClassStats::~KWClassStats()
{
	if (attributeTreeConstructionReport != NULL)
		delete attributeTreeConstructionReport;
	CleanWorkingData();
}

ObjectDictionary* KWClassStats::GetMultiTableConstructionSpec()
{
	return &odMultiTableConstructedAttributes;
}

ObjectDictionary* KWClassStats::GetTextConstructionSpec()
{
	return &odTextConstructedAttributes;
}

void KWClassStats::SetAttributePairsSpec(const KWAttributePairsSpec* spec)
{
	attributePairSpec = spec;
}

const KWAttributePairsSpec* KWClassStats::GetAttributePairsSpec() const
{
	return attributePairSpec;
}

boolean KWClassStats::ComputeStats()
{
	boolean bOk = true;
	ALString sMessage;
	KWDatabaseBasicStatsTask databaseBasicClassStatsTask;
	int nDatabaseObjectNumber;
	KWDatabaseSlicerTask databaseSlicerTask;
	KWDataPreparationUnivariateTask univariateDataPreparationTask;
	KWDataPreparationBivariateTask bivariateDataPreparationTask;
	ObjectDictionary odUnivariateAttributeStats;
	ObjectDictionary* odTreeInputAttributeStats;
	longint lRecordNumber;
	longint lCollectedObjectNumber;
	KWTupleTableLoader tupleTableLoader;
	KWTupleTable targetTupleTable;
	int nUsedAttributeNumber;
	int nMaxLoadableAttributeNumber;
	int i;
	KWAttribute* attribute;
	KWAttributeStats* attributeStats;
	ALString sTmp;

	require(Check());

	// Message d'info sur le type de tache d'apprentissage effectue
	if (GetTargetAttributeName() == "")
		sMessage += "Train unsupervised model";
	else
	{
		sMessage += "Train supervised model";
		if (GetTargetAttributeType() == KWType::Continuous)
			sMessage += " for regression of target variable " + GetTargetAttributeName();
		else if (GetTargetAttributeType() == KWType::Symbol)
			sMessage += " for classification of target variable " + GetTargetAttributeName();
	}
	AddSimpleMessage(sMessage);

	// Nettoyage initial
	CleanWorkingData();

	// Demarrage du timer
	assert(timerTotal.GetElapsedTime() == 0);
	timerTotal.Start();

	// Collecte du nombre d'enregistrements et des valeurs cibles
	nDatabaseObjectNumber = 0;
	lRecordNumber = 0;
	lCollectedObjectNumber = 0;

	// Initialization des vecteurs des valeurs de la cible
	svSymbolTargetValues = new SymbolVector;
	cvContinuousTargetValues = new ContinuousVector;

	// Collecte des stats de base, avec message d'erreur en cas d'echec
	// On se place en mode non verbeux, car les erreurs seront de toutes facon detectees lors des passes suivantes
	if (bOk)
	{
		databaseBasicClassStatsTask.SetDisplayAllTaskMessages(false);
		GetDatabase()->SetVerboseMode(false);
		databaseBasicClassStatsTask.SetReusableDatabaseIndexer(GetLearningSpec()->GetDatabaseIndexer());
		bOk = databaseBasicClassStatsTask.CollectBasicStats(GetDatabase(), GetTargetAttributeName(),
								    lRecordNumber, lCollectedObjectNumber,
								    svSymbolTargetValues, cvContinuousTargetValues);
		GetDatabase()->SetVerboseMode(true);
	}

	// Erreur si trop d'instances
	bOk = bOk and not TaskProgression::IsInterruptionRequested();
	if (bOk and lCollectedObjectNumber >= INT_MAX)
	{
		AddError(sTmp + "Too many instances in the train dataset (" +
			 LongintToReadableString(lCollectedObjectNumber) + ")");
		bOk = false;
	}

	// Calcul des statistiques de l'attribut cible (ou du nombre d'instances a traiter en non supervise)
	bOk = bOk and not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		// Memorisation du nombre d'objets
		nDatabaseObjectNumber = (int)lCollectedObjectNumber;

		// Parametrage du chargeur de table de tuples avec les objets de la base
		// Ce parametrage est valide pour toutes les lectures a venir
		tupleTableLoader.SetInputClass(GetClass());
		tupleTableLoader.SetInputDatabaseObjects(GetDatabase()->GetObjects());

		// Parametrage de l'attribut cible
		tupleTableLoader.SetInputExtraAttributeName(GetTargetAttributeName());
		tupleTableLoader.SetInputExtraAttributeType(GetTargetAttributeType());

		// Calcul de la table de tuple de l'attribut cible
		if (GetTargetAttributeType() == KWType::None)
		{
			tupleTableLoader.LoadTupleTableFromFrequency(nDatabaseObjectNumber, &targetTupleTable);
		}
		else if (GetTargetAttributeType() == KWType::Continuous)
		{
			assert(cvContinuousTargetValues->GetSize() == nDatabaseObjectNumber);
			tupleTableLoader.SetInputExtraAttributeContinuousValues(cvContinuousTargetValues);
			tupleTableLoader.LoadTupleTableFromContinuousValues(
			    GetTargetAttributeName(), cvContinuousTargetValues, &targetTupleTable);
		}
		else if (GetTargetAttributeType() == KWType::Symbol)
		{
			assert(svSymbolTargetValues->GetSize() == nDatabaseObjectNumber);
			tupleTableLoader.SetInputExtraAttributeSymbolValues(svSymbolTargetValues);
			tupleTableLoader.LoadTupleTableFromSymbolValues(GetTargetAttributeName(), svSymbolTargetValues,
									&targetTupleTable);
		}
		tupleTableLoader.SetInputExtraAttributeTupleTable(&targetTupleTable);

		// Calcul des valeurs cibles
		bOk = GetLearningSpec()->ComputeTargetStats(&targetTupleTable);
		assert(not bOk or GetInstanceNumber() == lCollectedObjectNumber);

		// Controles additionnels
		if (bOk)
		{
			// Warning si valeur cible specififie inexistante le cas categoriel
			if (GetTargetAttributeType() == KWType::Symbol and GetMainTargetModality() != Symbol() and
			    GetMainTargetModalityIndex() == -1)
			{
				AddWarning(sTmp + "The main target value '" + GetMainTargetModality() +
					   "' is missing in the train database");
			}

			// Warning si beaucoup de valeurs dans le cas categoriel
			if (GetTargetAttributeType() == KWType::Symbol and
			    GetTargetDescriptiveStats()->GetValueNumber() >
				GetTargetValueLargeNumber(nDatabaseObjectNumber))
			{
				AddWarning("The target variable " + GetTargetAttributeName() +
					   " contains many values (" +
					   IntToString(GetTargetDescriptiveStats()->GetValueNumber()) + ")");
			}
			// Warning si une seule valeur dans le cas supervise
			else if (GetTargetAttributeType() != KWType::None and
				 GetTargetDescriptiveStats()->GetValueNumber() == 1)
			{
				if (GetTargetAttributeType() == KWType::Continuous and
				    cast(KWDescriptiveContinuousStats*, GetTargetDescriptiveStats())
					    ->GetMissingValueNumber() > 0)
					AddWarning("The target variable " + GetTargetAttributeName() +
						   " has only missing values");
				else
					AddWarning("The target variable " + GetTargetAttributeName() +
						   " contains only one value");
			}
			// Erreur si regression et valeur manquante dans la cible
			else if (GetTargetAttributeType() == KWType::Continuous and
				 cast(KWDescriptiveContinuousStats*, GetTargetDescriptiveStats())
					 ->GetMissingValueNumber() > 0)
			{
				// Cas particulier: on emet ici un warning au lieu d'une erreur, car une autre tentative
				// peut avoir lieu apres filtrage des valeur manquantes cibles
				AddWarning("The target variable " + GetTargetAttributeName() + " contains " +
					   IntToString(cast(KWDescriptiveContinuousStats*, GetTargetDescriptiveStats())
							   ->GetMissingValueNumber()) +
					   " missing values");
				bOk = false;
			}
		}
	}

	// Calcul du nombre d'attributs a analyser, hors attribut cible
	nUsedAttributeNumber = GetClass()->GetUsedAttributeNumberForType(KWType::Continuous) +
			       GetClass()->GetUsedAttributeNumberForType(KWType::Symbol);
	if (GetTargetAttributeName() != "")
		nUsedAttributeNumber--;

	// Warning si aucun attribut a analyser
	if (bOk and nUsedAttributeNumber == 0)
		AddWarning("No input variable in analysis dictionary " + GetClass()->GetName());

	// Dimensionnement des taches de preparation univariee
	nMaxLoadableAttributeNumber = 0;
	bOk = bOk and not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		nMaxLoadableAttributeNumber = univariateDataPreparationTask.ComputeMaxLoadableAttributeNumber(
		    GetLearningSpec(), &targetTupleTable,
		    attributePairSpec->GetMaxRequestedAttributePairNumber(GetTargetAttributeName()));
		if (nMaxLoadableAttributeNumber <= 0 and nUsedAttributeNumber > 0)
			bOk = false;
	}

	// Decoupage des donnees analyses en tranches de variables
	bOk = bOk and not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		// Creation de la base preparee
		assert(dataTableSliceSet == NULL);
		dataTableSliceSet = new KWDataTableSliceSet;

		// On n'affiche que les messages sur les statistiques par table traitees
		databaseSlicerTask.SetDisplayEndTaskMessage(false);
		databaseSlicerTask.SetDisplayTaskTime(false);
		databaseSlicerTask.SetReusableDatabaseIndexer(GetLearningSpec()->GetDatabaseIndexer());
		bOk = databaseSlicerTask.SliceDatabase(GetDatabase(), GetTargetAttributeName(), nDatabaseObjectNumber,
						       nMaxLoadableAttributeNumber, dataTableSliceSet);
	}

	// Preparation univariee des donnees en parallele
	bOk = bOk and not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		// La tache gere sa propre progression
		bOk = univariateDataPreparationTask.CollectPreparationStats(
		    GetLearningSpec(), &tupleTableLoader, dataTableSliceSet, &odUnivariateAttributeStats);

		// On range les statistiques par attributs dans le meme ordre que dans la classe initiale
		if (bOk)
		{
			for (i = 0; i < GetLearningSpec()->GetClass()->GetUsedAttributeNumber(); i++)
			{
				attribute = GetLearningSpec()->GetClass()->GetUsedAttributeAt(i);
				attributeStats =
				    cast(KWAttributeStats*, odUnivariateAttributeStats.Lookup(attribute->GetName()));

				// Memorisation du resultats de preparation
				if (attributeStats != NULL)
				{
					// Rangement dans le tableau dedies pour les variables de type texte
					if (GetTextConstructionSpec()->Lookup(attribute->GetName()) != NULL)
					{
						assert(GetTextConstructionSpec()->Lookup(attribute->GetName()) ==
						       attribute);
						oaTextAttributeStats.Add(attributeStats);
					}
					// Rangement dans le tableau et le dictionnaire des attribut standard (attribut
					// initial ou multi-table) sinon
					else
					{
						oaAttributeStats.Add(attributeStats);
						odAttributeStats.SetAt(attribute->GetName(), attributeStats);
					}
				}
			}

			// Memorisation dans le tableau global
			assert(oaAllPreparedStats.GetSize() == 0);
			oaAllPreparedStats.CopyFrom(&oaAttributeStats);
			oaAllPreparedStats.InsertObjectArrayAt(oaAllPreparedStats.GetSize(), &oaTextAttributeStats);
			assert(oaAllPreparedStats.GetSize() ==
			       oaAttributeStats.GetSize() + oaTextAttributeStats.GetSize());
		}
	}

	// Calcul des stats pour les paires d'attributs (non disponible en regression)
	if (bOk and attributePairSpec->GetMaxAttributePairNumber() > 0 and nDatabaseObjectNumber > 0 and
	    oaAttributeStats.GetSize() > 1 and
	    (GetTargetAttributeType() == KWType::Symbol or GetTargetAttributeType() == KWType::None) and
	    not TaskProgression::IsInterruptionRequested())
	{
		AddSimpleMessage("Evaluation of variable pairs");

		// Prise en compte de couts de selection basiques pour les attributs utilisable pour les paires,
		// qui peuvent utiliser les variables de type texte
		UseUnivariateBasicSelectionCosts(true);

		// La tache gere sa propre progression
		bIsStatsComputed = true;
		bOk = bivariateDataPreparationTask.CollectPreparationPairStats(
		    GetLearningSpec(), this, &tupleTableLoader, dataTableSliceSet, &oaAttributePairStats);

		// Collecte des resultats dans le tableau de toutes les stats de preparation
		oaAllPreparedStats.InsertObjectArrayAt(oaAllPreparedStats.GetSize(), &oaAttributePairStats);
		assert(oaAllPreparedStats.GetSize() ==
		       oaAttributeStats.GetSize() + +oaTextAttributeStats.GetSize() + oaAttributePairStats.GetSize());

		// Restoration des couts de selection standards
		RestoreUnivariateSelectionCosts(true);
	}

	// Dictionnaire des attributs a utiliser pour les arbres, avec ou sans les variables de type texte
	if (GetLearningSpec()->GetTextConstructionUsedByTrees())
		odTreeInputAttributeStats = &odUnivariateAttributeStats;
	else
		odTreeInputAttributeStats = &odAttributeStats;

	// Creation d'attributs de type arbre
	bOk = bOk and not TaskProgression::IsInterruptionRequested();
	if (bOk and KDDataPreparationAttributeCreationTask::GetGlobalCreationTask() != NULL and
	    nDatabaseObjectNumber > 0 and odTreeInputAttributeStats->GetCount() > 0 and
	    KDDataPreparationAttributeCreationTask::GetGlobalCreationTask()->GetMaxCreatedAttributeNumber() > 0)
	{
		ObjectArray oaOutputAttributeStats;

		// Prise en compte de couts de selection basiques pour les attributs utilisable pour les arbres,
		// selon ce qui est specifie dans les learningSpec
		UseUnivariateBasicSelectionCosts(GetLearningSpec()->GetTextConstructionUsedByTrees());

		// Recherche de la tache de creation d'attribut, specifiee dans la classe
		// KDDataPreparationAttributeCreationTask Le passage par cette classe a permis de modulariser le
		// developpement de la librairie DTForest
		assert(attributeTreeConstructionTask == NULL);
		attributeTreeConstructionTask = KDDataPreparationAttributeCreationTask::CloneGlobalCreationTask();

		// La tache gere sa propre progression
		// Meme en cas d'echec de la preparation des arbres, on continue les traitements (pas de mise a jour de
		// bOk)
		MemoryStatsManager::AddLog(attributeTreeConstructionTask->GetClassLabel() +
					   " CreatePreparedAttributes Begin");
		bOk = attributeTreeConstructionTask->CreatePreparedAttributes(
		    GetLearningSpec(), &tupleTableLoader, dataTableSliceSet, odTreeInputAttributeStats,
		    &oaOutputAttributeStats);
		MemoryStatsManager::AddLog(attributeTreeConstructionTask->GetClassLabel() +
					   " CreatePreparedAttributes End");

		// Enregistrement des preparations d'attributs si ok
		if (bOk)
		{
			// Memorisation des preparations, sans les memoriser avec les preparations univaries
			// Cela a l'avantage de garder un rapport de preparation univarie plus propre
			oaTreeAttributeStats.CopyFrom(&oaOutputAttributeStats);
			oaAllPreparedStats.InsertObjectArrayAt(oaAllPreparedStats.GetSize(), &oaOutputAttributeStats);
			assert(oaAllPreparedStats.GetSize() ==
			       oaAttributeStats.GetSize() + +oaTextAttributeStats.GetSize() +
				   oaTreeAttributeStats.GetSize() + oaAttributePairStats.GetSize());
		}

		// Restoration des couts de selection standards
		RestoreUnivariateSelectionCosts(GetLearningSpec()->GetTextConstructionUsedByTrees());
	}

	// Temps final
	timerTotal.Stop();

	// Messsage de fin
	if (bOk and not TaskProgression::IsInterruptionRequested())
		AddSimpleMessage(sTmp + "Data preparation time: " + SecondsToString(timerTotal.GetElapsedTime()));
	else
	{
		// Message differents selon le type d'erreur
		if (TaskProgression::IsInterruptionRequested())
			AddSimpleMessage(sTmp + "Data preparation interrupted by user after " +
					 SecondsToString(timerTotal.GetElapsedTime()));
		else
		{
			// Message d'erreur synthetique, sauf dans le cas particulier de la regression s'il s'agit
			// d'un probleme de valeurs cibles manquante
			if (not(GetTargetAttributeType() == KWType::Continuous and
				GetLearningSpec()->IsTargetStatsComputed() and
				cast(KWDescriptiveContinuousStats*, GetTargetDescriptiveStats())
					->GetMissingValueNumber() > 0))
				AddSimpleMessage("Data preparation interrupted because of errors");
		}
	}

	// Nettoyage si probleme (il y a remise a zero des timers: doit etre appele a la fin)
	bOk = bOk and not TaskProgression::IsInterruptionRequested();
	bIsStatsComputed = bOk;
	if (not bIsStatsComputed)
		CleanWorkingData();
	return bIsStatsComputed;
}

ObjectArray* KWClassStats::GetAllPreparedStats()
{
	require(IsStatsComputed());

	return &oaAllPreparedStats;
}

ObjectArray* KWClassStats::GetAttributeStats()
{
	require(IsStatsComputed());

	return &oaAttributeStats;
}

KWAttributeStats* KWClassStats::LookupAttributeStats(const ALString& sAttributeName)
{
	require(IsStatsComputed());
	require(sAttributeName != "");
	return cast(KWAttributeStats*, odAttributeStats.Lookup(sAttributeName));
}

ObjectArray* KWClassStats::GetAttributePairStats()
{
	require(IsStatsComputed());

	return &oaAttributePairStats;
}

ObjectArray* KWClassStats::GetTextAttributeStats()
{
	require(IsStatsComputed());

	return &oaTextAttributeStats;
}

ObjectArray* KWClassStats::GetTreeAttributeStats()
{
	require(IsStatsComputed());

	return &oaTreeAttributeStats;
}

int KWClassStats::GetEvaluatedAttributeNumber() const
{
	require(IsStatsComputed());
	return oaAttributeStats.GetSize();
}

int KWClassStats::GetNativeAttributeNumber() const
{
	return GetEvaluatedAttributeNumber() - GetConstructedAttributeNumber();
}

int KWClassStats::GetConstructedAttributeNumber() const
{
	KWAttributeStats* attributeStats;
	KWAttribute* attribute;
	int nConstructedAttributeNumber;
	int nAttribute;

	require(IsStatsComputed());

	// Comptage des attributs construit
	nConstructedAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < oaAttributeStats.GetSize(); nAttribute++)
	{
		attributeStats = cast(KWAttributeStats*, oaAttributeStats.GetAt(nAttribute));
		assert(attributeStats->GetAttributeNumber() == 1);
		assert(KWType::IsSimple(attributeStats->GetAttributeType()));

		// Attribut construit s'il possede une regle de derivation avec cout non nul
		attribute = GetClass()->LookupAttribute(attributeStats->GetAttributeName());
		if (attribute != NULL and attribute->GetAnyDerivationRule() != NULL)
		{
			// Tous les attributs construits (regle utilisateur ou construction automatique)
			// sont pris en compte: plus facile a interpreter pour l'utilisateur final
			nConstructedAttributeNumber++;
		}
	}
	return nConstructedAttributeNumber;
}

int KWClassStats::GetTotalInformativeAttributeNumber() const
{
	KWDataPreparationStats* dataPreparationStats;
	int nInformativeAttributeNumber;
	int nAttribute;

	require(IsStatsComputed());

	// Comptage des attributs informatifs
	nInformativeAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < oaAllPreparedStats.GetSize(); nAttribute++)
	{
		dataPreparationStats = cast(KWDataPreparationStats*, oaAllPreparedStats.GetAt(nAttribute));
		if (dataPreparationStats->IsInformative())
			nInformativeAttributeNumber++;
	}
	assert(nInformativeAttributeNumber == GetInformativeAttributeNumber() + GetInformativeTextAttributeNumber() +
						  GetInformativeTreeAttributeNumber() +
						  GetInformativeAttributePairNumber());
	return nInformativeAttributeNumber;
}

int KWClassStats::GetInformativeAttributeNumber() const
{
	KWAttributeStats* attributeStats;
	int nInformativeAttributeNumber;
	int nAttribute;

	require(IsStatsComputed());

	// Comptage des attributs informatifs
	nInformativeAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < oaAttributeStats.GetSize(); nAttribute++)
	{
		attributeStats = cast(KWAttributeStats*, oaAttributeStats.GetAt(nAttribute));
		if (attributeStats->IsInformative())
			nInformativeAttributeNumber++;
	}

	return nInformativeAttributeNumber;
}

int KWClassStats::GetInformativeTextAttributeNumber() const
{
	KWAttributeStats* attributeStats;
	int nInformativeAttributeNumber;
	int nAttribute;

	require(IsStatsComputed());

	// Comptage des attributs informatifs
	nInformativeAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < oaTextAttributeStats.GetSize(); nAttribute++)
	{
		attributeStats = cast(KWAttributeStats*, oaTextAttributeStats.GetAt(nAttribute));
		if (attributeStats->IsInformative())
			nInformativeAttributeNumber++;
	}

	return nInformativeAttributeNumber;
}

int KWClassStats::GetInformativeTreeAttributeNumber() const
{
	KWAttributeStats* attributeStats;
	int nInformativeAttributeNumber;
	int nAttribute;

	require(IsStatsComputed());

	// Comptage des attributs informatifs
	nInformativeAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < oaTreeAttributeStats.GetSize(); nAttribute++)
	{
		attributeStats = cast(KWAttributeStats*, oaTreeAttributeStats.GetAt(nAttribute));
		if (attributeStats->IsInformative())
			nInformativeAttributeNumber++;
	}

	return nInformativeAttributeNumber;
}

int KWClassStats::GetInformativeAttributePairNumber() const
{
	KWAttributePairStats* attributePairStats;
	int nInformativeAttributePairNumber;
	int nAttribute;

	require(IsStatsComputed());

	// Comptage des attributs informatifs
	nInformativeAttributePairNumber = 0;
	for (nAttribute = 0; nAttribute < oaAttributePairStats.GetSize(); nAttribute++)
	{
		attributePairStats = cast(KWAttributePairStats*, oaAttributePairStats.GetAt(nAttribute));
		if (attributePairStats->IsInformative())
			nInformativeAttributePairNumber++;
	}
	return nInformativeAttributePairNumber;
}

int KWClassStats::GetUsedAttributeNumberForType(int nType) const
{
	int nNumber;
	KWAttributeStats* attributeStats;
	int nAttribute;

	require(0 <= nType and nType < KWType::None);
	require(IsStatsComputed());

	// Cas d'un attribut de type simple: on se base sur les attributs evalues
	// On s'adapte ainsi a la variante de creation des arbres, qui sont soit
	// traites a part (mode standard), soit integres au rapport de preparation
	// (mode  experet des forets d'arbre)
	if (KWType::IsSimple(nType))
	{
		// Parcours des attributs evalues
		nNumber = 0;
		for (nAttribute = 0; nAttribute < oaAttributeStats.GetSize(); nAttribute++)
		{
			attributeStats = cast(KWAttributeStats*, oaAttributeStats.GetAt(nAttribute));
			if (attributeStats->GetAttributeType() == nType)
				nNumber++;
		}

		// Ajout eventuel de l'attribut cible
		if (GetTargetAttributeType() == nType)
			nNumber++;
	}
	// Sinon, on va chercher dans la classe
	else
		nNumber = GetClass()->GetUsedAttributeNumberForType(nType);
	return nNumber;
}

double KWClassStats::GetTotalComputeTime() const
{
	require(IsStatsComputed());

	return timerTotal.GetElapsedTime();
}

const KDDataPreparationAttributeCreationTask* KWClassStats::GetAttributeTreeConstructionTask() const
{
	return attributeTreeConstructionTask;
}

void KWClassStats::RemoveAttributeTreeConstructionTask()
{
	attributeTreeConstructionTask = NULL;
}

void KWClassStats::DeleteAttributeTreeConstructionTask()
{
	if (attributeTreeConstructionTask != NULL)
		delete attributeTreeConstructionTask;
	attributeTreeConstructionTask = NULL;
}

void KWClassStats::AddAttributeStats(KWAttributeStats* attributeStats)
{
	require(attributeStats != NULL);
	require(odAttributeStats.Lookup(attributeStats->GetAttributeName()) == NULL);
	oaAttributeStats.Add(attributeStats);
	odAttributeStats.SetAt(attributeStats->GetAttributeName(), attributeStats);
	oaAllPreparedStats.Add(attributeStats);
	bIsStatsComputed = true;
}

KWDataTableSliceSet* KWClassStats::GetDataTableSliceSet() const
{
	require(IsStatsComputed());

	return dataTableSliceSet;
}

void KWClassStats::SetDataTableSliceSet(KWDataTableSliceSet* sliceSet)
{
	require(dataTableSliceSet == NULL);
	dataTableSliceSet = sliceSet;
}

void KWClassStats::RemoveDataTableSliceSet()
{
	dataTableSliceSet = NULL;
}

void KWClassStats::DeleteDataTableSliceSet()
{
	if (dataTableSliceSet != NULL)
	{
		delete dataTableSliceSet;
		dataTableSliceSet = NULL;
	}
}

SymbolVector* KWClassStats::GetSymbolTargetValues() const
{
	require(IsStatsComputed());

	return svSymbolTargetValues;
}

ContinuousVector* KWClassStats::GetContinuousTargetValues() const
{
	require(IsStatsComputed());

	return cvContinuousTargetValues;
}

void KWClassStats::SetSymbolTargetValues(SymbolVector* svValues)
{
	require(svSymbolTargetValues == NULL);
	svSymbolTargetValues = svValues;
}

void KWClassStats::SetContinuousTargetValues(ContinuousVector* cvValues)
{
	require(cvContinuousTargetValues == NULL);
	cvContinuousTargetValues = cvValues;
}

void KWClassStats::RemoveTargetValues()
{
	svSymbolTargetValues = NULL;
	cvContinuousTargetValues = NULL;
}

void KWClassStats::DeleteTargetValues()
{
	if (svSymbolTargetValues != NULL)
	{
		delete svSymbolTargetValues;
		svSymbolTargetValues = NULL;
	}
	if (cvContinuousTargetValues != NULL)
	{
		delete cvContinuousTargetValues;
		cvContinuousTargetValues = NULL;
	}
}

void KWClassStats::RemoveAll()
{
	oaAttributeStats.RemoveAll();
	oaAttributePairStats.RemoveAll();
	oaTextAttributeStats.RemoveAll();
	oaTreeAttributeStats.RemoveAll();
	CleanWorkingData();
}

void KWClassStats::DeleteAll()
{
	CleanWorkingData();
}

void KWClassStats::WriteReport(ostream& ost)
{
	ObjectArray oaSymbolAttributeStats;
	ObjectArray oaContinuousAttributeStats;
	KWAttributeStats* attributeStats;
	int i;
	int nType;
	int nAttributeNumber;
	int nTotalAttributeNumber;

	require(Check());
	require(IsStatsComputed());
	require(GetClass()->GetUsedAttributeNumber() == GetClass()->GetLoadedAttributeNumber());

	// Titre
	ost << "Descriptive statistics"
	    << "\n";
	ost << "\n\n";

	// Description du probleme d'apprentissage
	ost << "Problem description"
	    << "\n";

	// Description courte
	ost << "Short description"
	    << "\t" << GetShortDescription() << "\n";

	// Disctionnaire
	ost << "\n";
	ost << "Dictionary"
	    << "\t" << GetClass()->GetName() << "\n";

	// Nombres d'attributs par type
	ost << "Variables"
	    << "\n";
	nTotalAttributeNumber = 0;
	for (nType = 0; nType < KWType::None; nType++)
	{
		if (KWType::IsData(nType))
		{
			nAttributeNumber = GetUsedAttributeNumberForType(nType);
			nTotalAttributeNumber += nAttributeNumber;
			if (nAttributeNumber > 0)
				ost << "\t" << KWType::ToString(nType) << "\t" << nAttributeNumber << "\n";
		}
	}
	ost << "\t"
	    << "Total"
	    << "\t" << nTotalAttributeNumber << "\n";
	ost << "\n";

	// Base de donnees
	ost << "Database\t" << GetDatabase()->GetDatabaseName() << "\n";

	// Taux d'echantillonnage
	ost << "Sample percentage\t" << GetDatabase()->GetSampleNumberPercentage() << "\n";
	ost << "Sampling mode\t" << GetDatabase()->GetSamplingMode() << "\n";

	// Variable de selection
	ost << "Selection variable\t" << GetDatabase()->GetSelectionAttribute() << "\n";
	ost << "Selection value\t" << GetDatabase()->GetSelectionValue() << "\n";

	// Nombre d'instances
	ost << "Instances\t" << GetInstanceNumber() << "\n";

	// Type de tache d'apprentissage effectue
	ost << "\nLearning task";

	// Cas ou l'attribut cible n'est pas renseigne
	if (GetTargetAttributeType() == KWType::None)
	{
		ost << "\tUnsupervised analysis"
		    << "\n";
	}
	// Autres cas
	else
	{
		// Cas ou l'attribut cible est continu
		if (GetTargetAttributeType() == KWType::Continuous)
			ost << "\tRegression analysis"
			    << "\n";

		// Cas ou l'attribut cible est categoriel
		else if (GetTargetAttributeType() == KWType::Symbol)
			ost << "\tClassification analysis"
			    << "\n";
	}

	// Parametrage eventuel de l'apprentissage supervise
	if (GetTargetAttributeName() != "")
	{
		// Attribut cible
		// On demande un affichage complet (source et cible) pour forcer
		// l'utilisation explicite du libelle "Target"
		assert(GetTargetValueStats()->GetSourceAttributeNumber() == 0);
		ost << "\n";
		GetTargetValueStats()->WriteAttributeArrayLineReports(ost, true, true);

		// Statistiques descriptives
		if (GetTargetAttributeType() == KWType::Continuous or
		    (GetTargetAttributeType() == KWType::Symbol and
		     GetTargetDescriptiveStats()->GetValueNumber() > GetTargetValueLargeNumber(GetInstanceNumber())))
		{
			ost << "\n";
			GetTargetDescriptiveStats()->WriteReport(ost);
		}

		// Detail par valeur dans le cas symbol
		if (GetTargetAttributeType() == KWType::Symbol)
		{
			ost << "\n";
			GetTargetValueStats()->WriteAttributePartArrayLineReports(ost, true, true);
		}
	}

	// Arret si base vide
	if (GetInstanceNumber() == 0)
		return;

	// Statistiques sur les nombres de variables evaluees, natives, construites, informatives
	if (GetWriteOptionStats1D())
	{
		ost << "\n";
		ost << "Evaluated variables"
		    << "\t" << GetEvaluatedAttributeNumber() << "\n";
		if (GetConstructedAttributeNumber() > 0)
		{
			ost << "Native variables"
			    << "\t" << GetNativeAttributeNumber() << "\n";
			ost << "Constructed variables"
			    << "\t" << GetConstructedAttributeNumber() << "\n";
		}
		if (GetTargetAttributeName() != "")
			ost << "Informative variables"
			    << "\t" << GetInformativeAttributeNumber() << "\n";
	}

	// Statistiques sur les nombres de variables evaluees, natives, construites, informatives
	// dans le cas des variables de type text
	if (GetWriteOptionStatsText())
	{
		ost << "\n";
		ost << "Evaluated variables"
		    << "\t" << GetTextAttributeStats()->GetSize() << "\n";
		if (GetConstructedAttributeNumber() > 0)
		{
			ost << "Native variables"
			    << "\t0\n";
			ost << "Constructed variables"
			    << "\t" << GetTextAttributeStats()->GetSize() << "\n";
		}
		if (GetTargetAttributeName() != "")
			ost << "Informative variables"
			    << "\t" << GetInformativeTextAttributeNumber() << "\n";
	}

	// Statistiques sur les nombres de variables evaluees, natives, construites, informatives
	// dans le cas des variables de type arbre
	if (GetWriteOptionStatsTrees())
	{
		ost << "\n";
		ost << "Evaluated variables"
		    << "\t" << GetTreeAttributeStats()->GetSize() << "\n";
		if (GetConstructedAttributeNumber() > 0)
		{
			ost << "Native variables"
			    << "\t0\n";
			ost << "Constructed variables"
			    << "\t" << GetTreeAttributeStats()->GetSize() << "\n";
		}
		if (GetTargetAttributeName() != "")
			ost << "Informative variables"
			    << "\t" << GetInformativeTreeAttributeNumber() << "\n";
	}

	// Statistiques sur les nombres de variables evaluees, informatives
	// dans le cas des paires de variables
	if (GetWriteOptionStats2D())
	{
		ost << "\n";
		ost << "Evaluated variable pairs"
		    << "\t" << GetAttributePairStats()->GetSize() << "\n";
		ost << "Informative variable pairs"
		    << "\t" << GetInformativeAttributePairNumber() << "\n";
	}

	// Algorithme utilises
	if (GetWriteOptionStats1D() or GetWriteOptionStatsText() or GetWriteOptionStatsTrees())
	{
		ost << "\n";

		// Parametres de construction de variables de type arbre
		if (GetWriteOptionStats1D() and GetAttributeTreeConstructionReport() != NULL)
			GetAttributeTreeConstructionReport()->WriteReport(ost);

		// Parametrage des pretraitements
		ost << GetPreprocessingSpec()->GetDiscretizerSpec()->GetClassLabel() << "\t";
		if (IsTargetGrouped())
			ost << "MODL"
			    << "\n";
		else
			ost << GetPreprocessingSpec()->GetDiscretizerSpec()->GetMethodLabel(GetTargetAttributeType())
			    << "\n";
		ost << GetPreprocessingSpec()->GetGrouperSpec()->GetClassLabel() << "\t";
		if (IsTargetGrouped())
			ost << "MODL"
			    << "\n";
		else
			ost << GetPreprocessingSpec()->GetGrouperSpec()->GetMethodLabel(GetTargetAttributeType())
			    << "\n";
	}

	// Cout du model null
	if ((GetWriteOptionStats1D() or GetWriteOptionStatsText() or GetWriteOptionStatsTrees()) and
	    GetTargetAttributeName() != "")
	{
		ost << "\nNull model\n";
		ost << "\tConstr. cost\t" << GetNullConstructionCost() << "\n";
		ost << "\tPrep. cost\t" << GetNullPreparationCost() << "\n";
		ost << "\tData cost\t" << GetNullDataCost() << "\n";
	}

	// Calcul des identifiants des rapports bases sur leur rang
	if (GetWriteOptionStats1D())
		ComputeRankIdentifiers(&oaAttributeStats);
	if (GetWriteOptionStatsText())
		ComputeRankIdentifiers(&oaTextAttributeStats);
	if (GetWriteOptionStatsTrees())
		ComputeRankIdentifiers(&oaTreeAttributeStats);
	if (GetWriteOptionStats2D())
		ComputeRankIdentifiers(GetAttributePairStats());

	// On dispatche les statistiques univariee par type d'attribut dans le cas univarie
	if (GetWriteOptionStats1D())
	{
		for (i = 0; i < oaAttributeStats.GetSize(); i++)
		{
			attributeStats = cast(KWAttributeStats*, oaAttributeStats.GetAt(i));
			if (attributeStats->GetAttributeType() == KWType::Symbol)
				oaSymbolAttributeStats.Add(attributeStats);
			else if (attributeStats->GetAttributeType() == KWType::Continuous)
				oaContinuousAttributeStats.Add(attributeStats);
		}
	}

	// On dispatche les statistiques univariee par type d'attribut dans le cas d'attributs de type texte
	if (GetWriteOptionStatsText())
	{
		for (i = 0; i < oaTextAttributeStats.GetSize(); i++)
		{
			attributeStats = cast(KWAttributeStats*, oaTextAttributeStats.GetAt(i));
			if (attributeStats->GetAttributeType() == KWType::Symbol)
				oaSymbolAttributeStats.Add(attributeStats);
			else if (attributeStats->GetAttributeType() == KWType::Continuous)
				oaContinuousAttributeStats.Add(attributeStats);
		}
	}

	// On dispatche les statistiques univariee par type d'attribut dans le cas d'attributs de type arbre
	if (GetWriteOptionStatsTrees())
	{
		for (i = 0; i < oaTreeAttributeStats.GetSize(); i++)
		{
			attributeStats = cast(KWAttributeStats*, oaTreeAttributeStats.GetAt(i));
			if (attributeStats->GetAttributeType() == KWType::Symbol)
				oaSymbolAttributeStats.Add(attributeStats);
			else if (attributeStats->GetAttributeType() == KWType::Continuous)
				oaContinuousAttributeStats.Add(attributeStats);
		}
	}

	// Rapports synthetiques
	if (GetWriteOptionStats1D() or GetWriteOptionStatsText() or GetWriteOptionStatsTrees())
	{
		WriteArrayLineReport(ost, "Categorical variables statistics", &oaSymbolAttributeStats);
		WriteArrayLineReport(ost, "Numerical variables statistics", &oaContinuousAttributeStats);
	}
	if (GetWriteOptionStats2D())
		WriteArrayLineReport(ost, "Variables pairs statistics", GetAttributePairStats());

	// Rapports detailles
	if (GetWriteOptionStats1D())
		WriteArrayReport(ost, "Variables detailed statistics", &oaAttributeStats);
	if (GetWriteOptionStatsText())
		WriteArrayReport(ost, "Variables detailed statistics", &oaTextAttributeStats);
	if (GetWriteOptionStatsTrees())
		WriteArrayReport(ost, "Variables detailed statistics", &oaTreeAttributeStats);
	if (GetWriteOptionStats2D())
		WriteArrayReport(ost,
				 "Variables pairs detailed statistics\n(Pairs with two jointly informative variables)",
				 GetAttributePairStats());
}

void KWClassStats::SetWriteOptionStats1D(boolean bValue)
{
	bWriteOptionStats1D = bValue;
}

boolean KWClassStats::GetWriteOptionStats1D() const
{
	return bWriteOptionStats1D;
}

void KWClassStats::SetWriteOptionStats2D(boolean bValue)
{
	bWriteOptionStats2D = bValue;
}

boolean KWClassStats::GetWriteOptionStats2D() const
{
	return bWriteOptionStats2D;
}

void KWClassStats::SetAttributeTreeConstructionReport(KWLearningReport* report)
{
	if (attributeTreeConstructionReport != NULL)
		delete attributeTreeConstructionReport;
	attributeTreeConstructionReport = report;
}

KWLearningReport* KWClassStats::GetAttributeTreeConstructionReport() const
{
	return attributeTreeConstructionReport;
}

boolean KWClassStats::IsTreeConstructionRequired() const
{
	return (KDDataPreparationAttributeCreationTask::GetGlobalCreationTask() != NULL and
		KDDataPreparationAttributeCreationTask::GetGlobalCreationTask()->GetMaxCreatedAttributeNumber() > 0 and
		attributeTreeConstructionTask != NULL);
}

void KWClassStats::SetWriteOptionStatsText(boolean bValue)
{
	bWriteOptionStatsText = bValue;
}

boolean KWClassStats::GetWriteOptionStatsText() const
{
	return bWriteOptionStatsText;
}

void KWClassStats::SetWriteOptionStatsTrees(boolean bValue)
{
	bWriteOptionStatsTrees = bValue;
}

boolean KWClassStats::GetWriteOptionStatsTrees() const
{
	return bWriteOptionStatsTrees;
}

void KWClassStats::SetWriteOptionDetailedStats(boolean bValue)
{
	bWriteOptionDetailedStats = bValue;
}

boolean KWClassStats::GetWriteOptionDetailedStats() const
{
	return bWriteOptionDetailedStats;
}

void KWClassStats::SetAllWriteOptions(boolean bValue)
{
	bWriteOptionStats1D = bValue;
	bWriteOptionStatsText = bValue;
	bWriteOptionStatsTrees = bValue;
	bWriteOptionStats2D = bValue;
	bWriteOptionDetailedStats = bValue;
}

void KWClassStats::WriteJSONFields(JSONFile* fJSON)
{
	int nType;
	int nAttributeNumber;
	KWLearningReport* attributeCreationReport;

	require(not GetWriteOptionStats1D() or not GetWriteOptionStats2D());
	require(not GetWriteOptionStats1D() or not GetWriteOptionStatsText());
	require(not GetWriteOptionStats1D() or not GetWriteOptionStatsTrees());
	require(not GetWriteOptionStatsText() or not GetWriteOptionStatsTrees());
	require(not GetWriteOptionStatsText() or not GetWriteOptionStats2D());
	require(not GetWriteOptionStatsTrees() or not GetWriteOptionStats2D());
	require(Check());
	require(IsStatsComputed());
	require(GetClass()->GetUsedAttributeNumber() == GetClass()->GetLoadedAttributeNumber());

	// Type de rapport
	if (GetWriteOptionStats1D())
		fJSON->WriteKeyString("reportType", "Preparation");
	else if (GetWriteOptionStatsText())
		fJSON->WriteKeyString("reportType", "Preparation");
	else if (GetWriteOptionStatsTrees())
		fJSON->WriteKeyString("reportType", "Preparation");
	else if (GetWriteOptionStats2D())
		fJSON->WriteKeyString("reportType", "BivariatePreparation");

	// Description du probleme d'apprentissage
	fJSON->BeginKeyObject("summary");
	fJSON->WriteKeyString("dictionary", GetClass()->GetName());

	// Nombres d'attributs par type
	fJSON->BeginKeyObject("variables");
	fJSON->BeginKeyArray("types");
	for (nType = 0; nType < KWType::None; nType++)
	{
		if (KWType::IsData(nType))
		{
			nAttributeNumber = GetUsedAttributeNumberForType(nType);
			if (nAttributeNumber > 0)
				fJSON->WriteString(KWType::ToString(nType));
		}
	}
	fJSON->EndArray();
	fJSON->BeginKeyArray("numbers");
	for (nType = 0; nType < KWType::None; nType++)
	{
		if (KWType::IsData(nType))
		{
			nAttributeNumber = GetUsedAttributeNumberForType(nType);
			if (nAttributeNumber > 0)
				fJSON->WriteInt(nAttributeNumber);
		}
	}
	fJSON->EndArray();
	fJSON->EndObject();

	// Base de donnees
	GetDatabase()->WriteJSONFields(fJSON);
	fJSON->WriteKeyLongint("instances", GetInstanceNumber());

	// Cas ou l'attribut cible n'est pas renseigne
	if (GetTargetAttributeType() == KWType::None)
	{
		fJSON->WriteKeyString("learningTask", "Unsupervised analysis");
	}
	// Autres cas
	else
	{
		// Cas ou l'attribut cible est continu
		if (GetTargetAttributeType() == KWType::Continuous)
			fJSON->WriteKeyString("learningTask", "Regression analysis");

		// Cas ou l'attribut cible est categoriel
		else if (GetTargetAttributeType() == KWType::Symbol)
			fJSON->WriteKeyString("learningTask", "Classification analysis");
	}

	// Parametrage eventuel de l'apprentissage supervise
	if (GetTargetAttributeName() != "")
	{
		// Attribut cible
		// On demande un affichage complet (source et cible) pour forcer
		// l'utilisation explicite du libelle "Target"
		assert(GetTargetValueStats()->GetSourceAttributeNumber() == 0);
		fJSON->WriteKeyString("targetVariable", GetTargetAttributeName());

		// Modalite cible principale
		if (GetTargetAttributeType() == KWType::Symbol and GetMainTargetModalityIndex() != -1)
			fJSON->WriteKeyString("mainTargetValue", GetMainTargetModality().GetValue());

		// Statistiques descriptives
		GetTargetDescriptiveStats()->WriteJSONKeyReport(fJSON, "targetDescriptiveStats");

		// Detail par valeur dans le cas symbol
		if (GetTargetAttributeType() == KWType::Symbol)
		{
			GetTargetValueStats()->WriteJSONKeyValueFrequencies(fJSON, "targetValues");
		}
	}

	// Arret si base vide
	if (GetInstanceNumber() == 0)
	{
		fJSON->EndObject();
		return;
	}

	// Statistiques sur les nombres de variables evaluees, natives, construites, informatives
	if (GetWriteOptionStats1D())
	{
		fJSON->WriteKeyInt("evaluatedVariables", GetEvaluatedAttributeNumber());
		if (GetConstructedAttributeNumber() > 0)
		{
			fJSON->WriteKeyInt("nativeVariables", GetNativeAttributeNumber());
			fJSON->WriteKeyInt("constructedVariables", GetConstructedAttributeNumber());
		}
		if (GetTargetAttributeName() != "")
			fJSON->WriteKeyInt("informativeVariables", GetInformativeAttributeNumber());
	}

	// Statistiques sur les nombres de variables evaluees, natives, construites, informatives
	// pour les variables de type texte
	if (GetWriteOptionStatsText())
	{
		fJSON->WriteKeyInt("evaluatedVariables", oaTextAttributeStats.GetSize());
		if (GetConstructedAttributeNumber() > 0)
		{
			fJSON->WriteKeyInt("nativeVariables", 0);
			fJSON->WriteKeyInt("constructedVariables", oaTextAttributeStats.GetSize());
		}
		if (GetTargetAttributeName() != "")
			fJSON->WriteKeyInt("informativeVariables", GetInformativeTextAttributeNumber());
	}

	// Statistiques sur les nombres de variables evaluees, natives, construites, informatives
	// pour les variables de type arbre
	if (GetWriteOptionStatsTrees())
	{
		fJSON->WriteKeyInt("evaluatedVariables", oaTreeAttributeStats.GetSize());
		if (GetConstructedAttributeNumber() > 0)
		{
			fJSON->WriteKeyInt("nativeVariables", 0);
			fJSON->WriteKeyInt("constructedVariables", oaTreeAttributeStats.GetSize());
		}
		if (GetTargetAttributeName() != "")
			fJSON->WriteKeyInt("informativeVariables", GetInformativeTreeAttributeNumber());
	}

	// Statistiques sur les nombres de variables evaluees, informatives pour les paires de variables
	if (GetWriteOptionStats2D())
	{
		fJSON->WriteKeyInt("evaluatedVariablePairs", oaAttributePairStats.GetSize());
		fJSON->WriteKeyInt("informativeVariablePairs", GetInformativeAttributePairNumber());
	}

	// Algorithme utilises
	if (GetWriteOptionStats1D())
	{
		// Parametres de construction de variables de type arbre
		if (GetAttributeTreeConstructionReport() != NULL)
			GetAttributeTreeConstructionReport()->WriteJSONReport(fJSON);

		// Discretisation
		if (IsTargetGrouped())
			fJSON->WriteKeyString("discretization", "MODL");
		else
			fJSON->WriteKeyString(
			    "discretization",
			    GetPreprocessingSpec()->GetDiscretizerSpec()->GetMethodLabel(GetTargetAttributeType()));

		// Groupement de valeur
		if (IsTargetGrouped())
			fJSON->WriteKeyString("valueGrouping", "MODL");
		else
			fJSON->WriteKeyString("valueGrouping", GetPreprocessingSpec()->GetGrouperSpec()->GetMethodLabel(
								   GetTargetAttributeType()));
	}

	// Cout du model null
	if (GetWriteOptionStats1D() and GetTargetAttributeName() != "")
	{
		fJSON->BeginKeyObject("nullModel");
		fJSON->WriteKeyDouble("constructionCost", GetNullConstructionCost());
		fJSON->WriteKeyDouble("preparationCost", GetNullPreparationCost());
		fJSON->WriteKeyDouble("dataCost", GetNullDataCost());
		fJSON->EndObject();
	}

	// Fin de description du probleme d'apprentissage
	fJSON->EndObject();

	// Calcul des identifiants des rapports bases sur leur rang
	if (GetWriteOptionStats1D())
		ComputeRankIdentifiers(&oaAttributeStats);
	if (GetWriteOptionStatsText())
		ComputeRankIdentifiers(&oaTextAttributeStats);
	if (GetWriteOptionStatsTrees())
		ComputeRankIdentifiers(&oaTreeAttributeStats);
	if (GetWriteOptionStats2D())
		ComputeRankIdentifiers(GetAttributePairStats());

	// Rapports synthetiques
	if (GetWriteOptionStats1D())
		WriteJSONArrayReport(fJSON, "variablesStatistics", &oaAttributeStats, true);
	if (GetWriteOptionStatsText())
		WriteJSONArrayReport(fJSON, "variablesStatistics", &oaTextAttributeStats, true);
	if (GetWriteOptionStatsTrees())
		WriteJSONArrayReport(fJSON, "variablesStatistics", &oaTreeAttributeStats, true);
	if (GetWriteOptionStats2D())
		WriteJSONArrayReport(fJSON, "variablesPairsStatistics", GetAttributePairStats(), true);

	// Rapports detailles
	if (GetWriteOptionStats1D())
		WriteJSONDictionaryReport(fJSON, "variablesDetailedStatistics", &oaAttributeStats, false);
	if (GetWriteOptionStatsText())
		WriteJSONDictionaryReport(fJSON, "variablesDetailedStatistics", &oaTextAttributeStats, false);
	if (GetWriteOptionStatsTrees())
		WriteJSONDictionaryReport(fJSON, "variablesDetailedStatistics", &oaTreeAttributeStats, false);
	if (GetWriteOptionStats2D())
		WriteJSONDictionaryReport(fJSON, "variablesPairsDetailedStatistics", GetAttributePairStats(), false);

	// // Rapport detaille additionnel dans le cas des arbres
	if (GetWriteOptionStatsTrees() and attributeTreeConstructionTask != NULL)
	{
		attributeCreationReport = attributeTreeConstructionTask->GetCreationReport();
		if (attributeCreationReport != NULL)
			attributeCreationReport->WriteJSONKeyReport(
			    fJSON, attributeTreeConstructionTask->GetReportPrefix() + "Details");
	}
}

boolean KWClassStats::Check() const
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWLearningReport::Check();

	// Verification de la specification des paires de variables
	// On passe par learningSpec->GetClass() pour eviter un appel recursif au Check de la classe ancetre
	if (bOk)
	{
		if (attributePairSpec != NULL and
		    attributePairSpec->GetClassName() != learningSpec->GetClass()->GetName())
		{
			bOk = false;
			AddError("Wrong specification for bivariate variable construction (dictionary " +
				 attributePairSpec->GetClassName() + " instead of " +
				 learningSpec->GetClass()->GetName() + ")");
		}
	}

	// Verification de la specification de construction multi-tables
	if (bOk and not CheckConstructionAttributes(&odMultiTableConstructedAttributes))
	{
		bOk = false;
		AddError("Wrong specification for multi-table variable construction");
	}

	// Verification de la specification de construction de type texte
	if (bOk and not CheckConstructionAttributes(&odTextConstructedAttributes))
	{
		bOk = false;
		AddError("Wrong specification for text variable construction");
	}
	return bOk;
}

const ALString KWClassStats::GetClassLabel() const
{
	return "Data preparation";
}

const ALString KWClassStats::GetObjectLabel() const
{
	if (GetLearningSpec() == NULL)
		return "";
	else
		return GetLearningSpec()->GetObjectLabel();
}

void KWClassStats::UseUnivariateBasicSelectionCosts(boolean bUsingTextConstruction)
{
	InternalUpdateUnivariateSelectionCosts(GetLearningSpec()->GetSelectionCost(),
					       GetLearningSpec()->GetBasicSelectionCost(bUsingTextConstruction));
}

void KWClassStats::RestoreUnivariateSelectionCosts(boolean bUsingTextConstruction)
{
	InternalUpdateUnivariateSelectionCosts(GetLearningSpec()->GetBasicSelectionCost(bUsingTextConstruction),
					       GetLearningSpec()->GetSelectionCost());
}

void KWClassStats::InternalUpdateUnivariateSelectionCosts(double dPreviousSelectionCost, double dNewSelectionCost)
{
	int i;
	KWClass* kwcCurrentClass;
	KWAttributeStats* attributeStats;
	KWAttribute* attribute;
	int nSlice;
	const double dEpsilon = 1e-10;
	double dDeltaSelectionCost;

	require(GetClass() != NULL);
	require(dPreviousSelectionCost >= 0);
	require(dNewSelectionCost >= 0);

	// Mise a jour si necessaire
	if (dNewSelectionCost != dPreviousSelectionCost)
	{
		dDeltaSelectionCost = dNewSelectionCost - dPreviousSelectionCost;

		// Parcours des statistiques univariees pour la classes des LearningSpec
		kwcCurrentClass = GetClass();
		for (i = 0; i < oaAttributeStats.GetSize(); i++)
		{
			attributeStats = cast(KWAttributeStats*, oaAttributeStats.GetAt(i));

			// Recherche de l'attribut
			attribute = kwcCurrentClass->LookupAttribute(attributeStats->GetAttributeName());
			check(attribute);

			// Mise a jour du cout de construction de l'attribut
			assert(attribute->GetCost() >= dPreviousSelectionCost - dEpsilon);
			assert(fabs(attribute->GetCost() - dPreviousSelectionCost) < dEpsilon or
			       attribute->GetAnyDerivationRule() != NULL);
			attribute->SetCost(attribute->GetCost() + dDeltaSelectionCost);

			// Correction epsilonesque si besoin
			if (fabs(attribute->GetCost() - dNewSelectionCost) < dEpsilon)
				attribute->SetCost(dNewSelectionCost);

			// Mise a jour des meta-donnees
			attribute->SetMetaDataCost(attribute->GetCost());
		}

		// Mise a jour egalement des cout dans toutes les classes du SliceSets
		if (dataTableSliceSet != NULL)
		{
			// Parcours des tranches
			for (nSlice = 0; nSlice < dataTableSliceSet->GetSliceNumber(); nSlice++)
			{
				kwcCurrentClass = dataTableSliceSet->GetSliceAt(nSlice)->GetClass();

				// // Mise ajour des attributs de la classe de la tranche
				attribute = kwcCurrentClass->GetHeadAttribute();
				while (attribute != NULL)
				{
					// Mise a jour du cout de construction de l'attribut
					assert(attribute->GetCost() >= dPreviousSelectionCost - dEpsilon);
					attribute->SetCost(attribute->GetCost() + dDeltaSelectionCost);

					// Correction epsilonesque si besoin
					if (fabs(attribute->GetCost() - dNewSelectionCost) < dEpsilon)
						attribute->SetCost(dNewSelectionCost);

					// Mise a jour des meta-donnees
					attribute->SetMetaDataCost(attribute->GetCost());

					// Attribute suivant
					kwcCurrentClass->GetNextAttribute(attribute);
				}
			}
		}
	}
}

void KWClassStats::CleanWorkingData()
{
	oaAllPreparedStats.RemoveAll();
	odAttributeStats.RemoveAll();
	oaAttributeStats.DeleteAll();
	oaAttributePairStats.DeleteAll();
	oaTextAttributeStats.DeleteAll();
	oaTreeAttributeStats.DeleteAll();
	DeleteAttributeTreeConstructionTask();
	DeleteDataTableSliceSet();
	DeleteTargetValues();
	timerTotal.Reset();
	bIsStatsComputed = false;
}

KWAttributeStats* KWClassStats::TreeAttributeStats() const
{
	return new KWAttributeStats;
}

KWAttributePairStats* KWClassStats::CreateAttributePairStats() const
{
	return new KWAttributePairStats;
}

int KWClassStats::GetTargetValueLargeNumber(int nDatabaseSize)
{
	require(nDatabaseSize >= 0);
	return 10 + (int)sqrt(1.0 * nDatabaseSize);
}

boolean KWClassStats::CheckConstructionAttributes(const ObjectDictionary* odConstructedAttributes) const
{
	boolean bOk = true;
	POSITION position;
	ALString sName;
	Object* oElement;
	KWAttribute* attribute;
	KWAttribute* foundAttribute;

	require(GetLearningSpec() != NULL);
	require(GetLearningSpec()->GetClass() != NULL);
	require(odConstructedAttributes != NULL);

	// Verification des attributs construits du dictionnaire
	position = odConstructedAttributes->GetStartPosition();
	while (position != NULL)
	{
		// Acces a chaque attribut et a son nom
		odConstructedAttributes->GetNextAssoc(position, sName, oElement);
		attribute = cast(KWAttribute*, oElement);
		assert(attribute != NULL);
		assert(attribute->GetName() == sName);

		// Verification de l'utilisation et du type d'attribut
		bOk = bOk and attribute->GetUsed();
		bOk = bOk and KWType::IsSimple(attribute->GetType());

		// Verification de l'existence de l'attribut
		foundAttribute = GetLearningSpec()->GetClass()->LookupAttribute(sName);
		bOk = bOk and foundAttribute == attribute;

		// Arret si erreur
		if (not bOk)
			break;
	}
	return bOk;
}
