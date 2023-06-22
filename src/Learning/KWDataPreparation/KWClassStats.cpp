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
	attributeConstructionReport = NULL;
	bWriteOptionStats1D = true;
	bWriteOptionStatsCreated = true;
	bWriteOptionStats2D = true;
	bWriteOptionDetailedStats = true;
	attributeCreationTask = NULL;
	dataTableSliceSet = NULL;
	svSymbolTargetValues = NULL;
	cvContinuousTargetValues = NULL;
}

KWClassStats::~KWClassStats()
{
	if (attributeConstructionReport != NULL)
		delete attributeConstructionReport;
	CleanWorkingData();
}

void KWClassStats::SetAttributePairsSpec(KWAttributePairsSpec* spec)
{
	attributePairSpec = spec;
}

KWAttributePairsSpec* KWClassStats::GetAttributePairsSpec()
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
	require(attributePairSpec != NULL);
	require(attributePairSpec->GetClassName() == GetClass()->GetName());

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
		bOk = databaseSlicerTask.SliceDatabase(GetDatabase(), GetTargetAttributeName(), nDatabaseObjectNumber,
						       nMaxLoadableAttributeNumber, dataTableSliceSet);
	}

	// Preparation univariee des donnees en parallele
	bOk = bOk and not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		// La tache gere sa propre progression
		bOk = univariateDataPreparationTask.CollectPreparationStats(GetLearningSpec(), &tupleTableLoader,
									    dataTableSliceSet, &odAttributeStats);

		// On range les statistiques par attributs dans le meme ordre que dans la classe initiale
		if (bOk)
		{
			for (i = 0; i < GetLearningSpec()->GetClass()->GetUsedAttributeNumber(); i++)
			{
				attribute = GetLearningSpec()->GetClass()->GetUsedAttributeAt(i);
				attributeStats = cast(KWAttributeStats*, odAttributeStats.Lookup(attribute->GetName()));
				if (attributeStats != NULL)
					oaAttributeStats.Add(attributeStats);
			}
			oaAllPreparedStats.CopyFrom(&oaAttributeStats);
		}
	}

	// Prise en compte de couts de selection basiques pour les attributs utilisable pour les paires ou les arbres
	UseUnivariateBasicSelectionCosts();

	// Calcul des stats pour les paires d'attributs (non disponible en regression)
	if (bOk and attributePairSpec->GetMaxAttributePairNumber() > 0 and nDatabaseObjectNumber > 0 and
	    oaAttributeStats.GetSize() > 1 and
	    (GetTargetAttributeType() == KWType::Symbol or GetTargetAttributeType() == KWType::None) and
	    not TaskProgression::IsInterruptionRequested())
	{
		AddSimpleMessage("Evaluation of variable pairs");

		// La tache gere sa propre progression
		bIsStatsComputed = true;
		bOk = bivariateDataPreparationTask.CollectPreparationPairStats(
		    GetLearningSpec(), this, &tupleTableLoader, dataTableSliceSet, &oaAttributePairStats);

		// Collecte des resultats dans le tableau de toutes les stats de preparation
		oaAllPreparedStats.InsertObjectArrayAt(oaAllPreparedStats.GetSize(), &oaAttributePairStats);
		assert(oaAllPreparedStats.GetSize() == oaAttributeStats.GetSize() + oaAttributePairStats.GetSize());
	}

	// Creation d'attributs
	bOk = bOk and not TaskProgression::IsInterruptionRequested();
	if (bOk and KDDataPreparationAttributeCreationTask::GetGlobalCreationTask() != NULL and
	    nDatabaseObjectNumber > 0 and odAttributeStats.GetCount() > 0 and
	    KDDataPreparationAttributeCreationTask::GetGlobalCreationTask()->GetMaxCreatedAttributeNumber() > 0)
	{
		ObjectArray oaOutputAttributeStats;

		// Recherche de la tache de creation d'attribut
		assert(attributeCreationTask == NULL);
		attributeCreationTask = KDDataPreparationAttributeCreationTask::CloneGlobalCreationTask();

		// La tache gere sa propre progression
		MemoryStatsManager::AddLog(attributeCreationTask->GetClassLabel() + " CreatePreparedAttributes Begin");
		bOk = attributeCreationTask->CreatePreparedAttributes(GetLearningSpec(), &tupleTableLoader,
								      dataTableSliceSet, &odAttributeStats,
								      &oaOutputAttributeStats);
		MemoryStatsManager::AddLog(attributeCreationTask->GetClassLabel() + " CreatePreparedAttributes End");

		// Enregistrement des preparations d'attributs si ok
		if (bOk)
		{
			// Memorisation des preparations, sans les memoriser avec les preparations univaries
			// Cela a l'avantage de garder un rapport de preparation univarie plus propre
			oaCreatedAttributeStats.CopyFrom(&oaOutputAttributeStats);
			oaAllPreparedStats.InsertObjectArrayAt(oaAllPreparedStats.GetSize(), &oaOutputAttributeStats);
		}
	}

	// Restoration des cout de selection standards
	RestoreUnivariateSelectionCosts();

	// Temps final
	timerTotal.Stop();

	// Messsage de fin
	if (bOk and not TaskProgression::IsInterruptionRequested())
		AddSimpleMessage(sTmp + "Data preparation time: " + SecondsToString(timerTotal.GetElapsedTime()));
	else
	{
		// Message differents selon le tye d'erreur
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

ObjectArray* KWClassStats::GetCreatedAttributeStats()
{
	require(IsStatsComputed());

	return &oaCreatedAttributeStats;
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
		if (attributeStats->GetLevel() > 0)
			nInformativeAttributeNumber++;
	}

	return nInformativeAttributeNumber;
}

int KWClassStats::GetInformativeCreatedAttributeNumber() const
{
	KWAttributeStats* attributeStats;
	int nInformativeAttributeNumber;
	int nAttribute;

	require(IsStatsComputed());

	// Comptage des attributs informatifs
	nInformativeAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < oaCreatedAttributeStats.GetSize(); nAttribute++)
	{
		attributeStats = cast(KWAttributeStats*, oaCreatedAttributeStats.GetAt(nAttribute));
		if (attributeStats->GetLevel() > 0)
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
		if (GetTargetAttributeName() != "")
		{
			if (attributePairStats->GetDeltaLevel() > 0)
				nInformativeAttributePairNumber++;
		}
		else
		{
			if (attributePairStats->GetLevel() > 0)
				nInformativeAttributePairNumber++;
		}
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

const KDDataPreparationAttributeCreationTask* KWClassStats::GetAttributeCreationTask() const
{
	return attributeCreationTask;
}

void KWClassStats::RemoveAttributeCreationTask()
{
	attributeCreationTask = NULL;
}

void KWClassStats::DeleteAttributeCreationTask()
{
	if (attributeCreationTask != NULL)
		delete attributeCreationTask;
	attributeCreationTask = NULL;
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

void KWClassStats::AddAttributePairStats(KWAttributePairStats* attributePairStats)
{
	require(attributePairStats != NULL);
	oaAttributePairStats.Add(attributePairStats);
	oaAllPreparedStats.Add(attributePairStats);
	bIsStatsComputed = true;
}

void KWClassStats::AddCreatedAttributeStats(KWAttributeStats* createdAttributeStats)
{
	require(createdAttributeStats != NULL);
	oaCreatedAttributeStats.Add(createdAttributeStats);
	oaAllPreparedStats.Add(createdAttributeStats);
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
	oaCreatedAttributeStats.RemoveAll();
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
	// dans le cas des variables construites
	if (GetWriteOptionStatsCreated())
	{
		ost << "\n";
		ost << "Evaluated variables"
		    << "\t" << GetCreatedAttributeStats()->GetSize() << "\n";
		if (GetConstructedAttributeNumber() > 0)
		{
			ost << "Native variables"
			    << "\t0\n";
			ost << "Constructed variables"
			    << "\t" << GetCreatedAttributeStats()->GetSize() << "\n";
		}
		if (GetTargetAttributeName() != "")
			ost << "Informative variables"
			    << "\t" << GetInformativeCreatedAttributeNumber() << "\n";
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
	if (GetWriteOptionStats1D() or GetWriteOptionStatsCreated())
	{
		ost << "\n";

		// Parametres de construction de variable
		if (GetWriteOptionStats1D() and GetAttributeConstructionReport() != NULL)
			GetAttributeConstructionReport()->WriteReport(ost);

		// Parametrage des partraitements
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
	if ((GetWriteOptionStats1D() or GetWriteOptionStatsCreated()) and GetTargetAttributeName() != "")
	{
		ost << "\nNull model\n";
		ost << "\tConstr. cost\t" << GetNullConstructionCost() << "\n";
		ost << "\tPrep. cost\t" << GetNullPreparationCost() << "\n";
		ost << "\tData cost\t" << GetNullDataCost() << "\n";
	}

	// Calcul des identifiants des rapports bases sur leur rang
	if (GetWriteOptionStats1D())
		ComputeRankIdentifiers(&oaAttributeStats);
	if (GetWriteOptionStatsCreated())
		ComputeRankIdentifiers(&oaCreatedAttributeStats);
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

	// On dispatche egalement les statistiques univariee par type d'attribut dans le cas d'attributs cree
	if (GetWriteOptionStatsCreated())
	{
		for (i = 0; i < oaCreatedAttributeStats.GetSize(); i++)
		{
			attributeStats = cast(KWAttributeStats*, oaCreatedAttributeStats.GetAt(i));
			if (attributeStats->GetAttributeType() == KWType::Symbol)
				oaSymbolAttributeStats.Add(attributeStats);
			else if (attributeStats->GetAttributeType() == KWType::Continuous)
				oaContinuousAttributeStats.Add(attributeStats);
		}
	}

	// Rapports synthetiques
	if (GetWriteOptionStats1D() or GetWriteOptionStatsCreated())
	{
		WriteArrayLineReport(ost, "Categorical variables statistics", &oaSymbolAttributeStats);
		WriteArrayLineReport(ost, "Numerical variables statistics", &oaContinuousAttributeStats);
	}
	if (GetWriteOptionStats2D())
		WriteArrayLineReport(ost, "Variables pairs statistics", GetAttributePairStats());

	// Rapports detailles
	if (GetWriteOptionStats1D())
		WriteArrayReport(ost, "Variables detailed statistics", &oaAttributeStats);
	if (GetWriteOptionStatsCreated())
		WriteArrayReport(ost, "Variables detailed statistics", &oaCreatedAttributeStats);
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

boolean KWClassStats::IsCreationRequired() const
{
	return (KDDataPreparationAttributeCreationTask::GetGlobalCreationTask() != NULL and
		KDDataPreparationAttributeCreationTask::GetGlobalCreationTask()->GetMaxCreatedAttributeNumber() > 0 and
		attributeCreationTask != NULL);
}

void KWClassStats::SetAttributeConstructionReport(KWLearningReport* report)
{
	if (attributeConstructionReport != NULL)
		delete attributeConstructionReport;
	attributeConstructionReport = report;
}

KWLearningReport* KWClassStats::GetAttributeConstructionReport() const
{
	return attributeConstructionReport;
}

void KWClassStats::SetWriteOptionStatsCreated(boolean bValue)
{
	bWriteOptionStatsCreated = bValue;
}

boolean KWClassStats::GetWriteOptionStatsCreated() const
{
	return bWriteOptionStatsCreated;
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
	bWriteOptionStatsCreated = bValue;
	bWriteOptionStats2D = bValue;
	bWriteOptionDetailedStats = bValue;
}

void KWClassStats::WriteJSONFields(JSONFile* fJSON)
{
	int nType;
	int nAttributeNumber;
	KWLearningReport* attributeCreationReport;

	require(not GetWriteOptionStats1D() or not GetWriteOptionStats2D());
	require(not GetWriteOptionStats1D() or not GetWriteOptionStatsCreated());
	require(not GetWriteOptionStatsCreated() or not GetWriteOptionStats2D());
	require(Check());
	require(IsStatsComputed());
	require(GetClass()->GetUsedAttributeNumber() == GetClass()->GetLoadedAttributeNumber());

	// Type de rapport
	if (GetWriteOptionStats1D())
		fJSON->WriteKeyString("reportType", "Preparation");
	else if (GetWriteOptionStatsCreated())
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
	// pour les variables construites
	if (GetWriteOptionStatsCreated())
	{
		fJSON->WriteKeyInt("evaluatedVariables", oaCreatedAttributeStats.GetSize());
		if (GetConstructedAttributeNumber() > 0)
		{
			fJSON->WriteKeyInt("nativeVariables", 0);
			fJSON->WriteKeyInt("constructedVariables", oaCreatedAttributeStats.GetSize());
		}
		if (GetTargetAttributeName() != "")
			fJSON->WriteKeyInt("informativeVariables", GetInformativeCreatedAttributeNumber());
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
		// Parametres de construction de variable
		if (GetAttributeConstructionReport() != NULL)
			GetAttributeConstructionReport()->WriteJSONReport(fJSON);

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
	if (GetWriteOptionStatsCreated())
		ComputeRankIdentifiers(&oaCreatedAttributeStats);
	if (GetWriteOptionStats2D())
		ComputeRankIdentifiers(GetAttributePairStats());

	// Rapports synthetiques
	if (GetWriteOptionStats1D())
		WriteJSONArrayReport(fJSON, "variablesStatistics", &oaAttributeStats, true);
	if (GetWriteOptionStatsCreated())
		WriteJSONArrayReport(fJSON, "variablesStatistics", &oaCreatedAttributeStats, true);
	if (GetWriteOptionStats2D())
		WriteJSONArrayReport(fJSON, "variablesPairsStatistics", GetAttributePairStats(), true);

	// Rapports detailles
	if (GetWriteOptionStats1D())
		WriteJSONDictionaryReport(fJSON, "variablesDetailedStatistics", &oaAttributeStats, false);
	if (GetWriteOptionStatsCreated())
		WriteJSONDictionaryReport(fJSON, "variablesDetailedStatistics", &oaCreatedAttributeStats, false);
	if (GetWriteOptionStats2D())
		WriteJSONDictionaryReport(fJSON, "variablesPairsDetailedStatistics", GetAttributePairStats(), false);

	// // Rapport detaille additionnel dans le cas de la creation d'attributs
	if (GetWriteOptionStatsCreated() and attributeCreationTask != NULL)
	{
		attributeCreationReport = attributeCreationTask->GetCreationReport();
		if (attributeCreationReport != NULL)
			attributeCreationReport->WriteJSONKeyReport(fJSON, attributeCreationTask->GetReportPrefix() +
									       "Details");
	}
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

void KWClassStats::UseUnivariateBasicSelectionCosts()
{
	InternalUpdateUnivariateSelectionCosts(GetLearningSpec()->GetSelectionCost(),
					       GetLearningSpec()->GetBasicSelectionCost());
}

void KWClassStats::RestoreUnivariateSelectionCosts()
{
	InternalUpdateUnivariateSelectionCosts(GetLearningSpec()->GetBasicSelectionCost(),
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
	oaCreatedAttributeStats.DeleteAll();
	DeleteAttributeCreationTask();
	DeleteDataTableSliceSet();
	DeleteTargetValues();
	timerTotal.Reset();
	bIsStatsComputed = false;
}

KWAttributeStats* KWClassStats::CreateAttributeStats() const
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