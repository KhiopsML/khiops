// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDSelectionOperandSamplingTask.h"

KDSelectionOperandSamplingTask::KDSelectionOperandSamplingTask()
{
	bTrace = false;
	bLastSlaveProcessDone = false;
	masterSelectionOperandDataSampler = NULL;
	slaveSelectionOperandDataSampler = NULL;

	// Variables partagees
	DeclareSharedParameter(&shared_selectionOperandDataSamplerSpec);
	DeclareTaskInput(&input_dvObjectSelectionProbThresholds);
	DeclareTaskInput(&input_bLastRound);
	DeclareTaskOutput(&output_bLastRound);
	DeclareTaskOutput(&output_selectionOperandDataSampler);
}

KDSelectionOperandSamplingTask::~KDSelectionOperandSamplingTask()
{
	assert(masterSelectionOperandDataSampler == NULL);
	assert(slaveSelectionOperandDataSampler == NULL);
}

boolean KDSelectionOperandSamplingTask::CollectSelectionOperandSamples(
    KDSelectionOperandDataSampler* selectionOperandDataSampler, const KWDatabase* sourceDatabase,
    int& nReadObjectNumber)
{
	boolean bOk;
	boolean bTestMode = false;

	require(selectionOperandDataSampler != NULL);
	require(selectionOperandDataSampler->Check());
	require(sourceDatabase != NULL);
	require(sourceDatabase->IsMultiTableTechnology());

	require(selectionOperandDataSampler->Check());
	require(selectionOperandDataSampler->GetClassSelectionData()->GetSize() > 0);
	require(selectionOperandDataSampler->GetTotalSelectionOperandNumber() > 0);

	// Calcul de la taille max des echantillons
	selectionOperandDataSampler->SetMaxSampleSize(selectionOperandDataSampler->ComputeMaxSampleSize());
	assert(selectionOperandDataSampler->GetMaxSampleSize() > 0);

	// Mode test
	if (bTestMode)
	{
		// Pour forcer le traitement du fichier avec de nombreux esclaves
		lForcedMaxFileSizePerProcess = lMB;

		// Pour forcer un taille d'echantillon
		selectionOperandDataSampler->SetMaxSampleSize(1000);
	}

	// Parametrage des variables partagees par une copie des spec des echantillons a collecter
	shared_selectionOperandDataSamplerSpec.SetSelectionOperandDataSampler(selectionOperandDataSampler->CloneSpec());

	// Memorisation de l'echantillonneur de donnees pour le maitre
	masterSelectionOperandDataSampler = selectionOperandDataSampler;

	// On ne souhaite que les messages de fin de tache en cas d'arret
	SetDisplaySpecificTaskMessage(false);
	SetDisplayTaskTime(false);
	SetDisplayEndTaskMessage(true);

	// Lancement de la tache
	bOk = RunDatabaseTask(sourceDatabase);
	masterSelectionOperandDataSampler = NULL;

	// Prise en compte des donnees collectees par un echantilonneur de donnees
	if (bOk)
	{
		nReadObjectNumber = (int)GetReadObjects();
	}
	// Nettoyage sinon
	else
	{
		nReadObjectNumber = 0;
		selectionOperandDataSampler->CleanData();
	}
	return bOk;
}

const ALString KDSelectionOperandSamplingTask::GetTaskName() const
{
	return "Selection operand extraction";
}

PLParallelTask* KDSelectionOperandSamplingTask::Create() const
{
	return new KDSelectionOperandSamplingTask;
}

boolean KDSelectionOperandSamplingTask::ComputeResourceRequirements()
{
	boolean bOk;
	boolean bDisplay = false;
	const SortedList slSample;
	longint lSelectionOperandDataSamplerSpecUsedMemory;
	longint lAllSamplesNecessaryMemory;
	longint lMaxSampleSize;
	longint lTotalValueNumber;
	PLDatabaseTextFile* sourcePLDatabase;
	PLMTDatabaseTextFile* sourceMTDatabase;
	const KWClass* kwcClass;
	int nClass;
	KDClassSelectionData* classSelectionData;
	int nMapping;
	KWMTDatabaseMapping* mapping;
	longint lClassEstimatedObjectNumber;
	longint lMaxClassEstimatedObjectNumber;

	require(masterSelectionOperandDataSampler != NULL);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::ComputeResourceRequirements();

	// Information de dimensionnement specifiques
	if (bOk)
	{
		// Taille des specifications de l'echantillonneur
		lSelectionOperandDataSamplerSpecUsedMemory = masterSelectionOperandDataSampler->GetUsedMemory();

		// Estimation heuristique du nombre d'objets du fichier principal
		sourcePLDatabase = shared_sourceDatabase.GetPLDatabase();
		lClassEstimatedObjectNumber = sourcePLDatabase->GetInMemoryEstimatedFileObjectNumber();

		// On prend en compte le nombre d'objet potentiellement cree par des regles de creation d'Entity
		// Cela donne un majorant pour le fichier principal du nombre total d'objet du fichier ou crees en memoire
		// Pour le cas de regles de creation de Table, on suppose que ces objet seront crees soit a partir
		// des Entity creees, soit a partir d'autre tables provenant de fichier des tables secondaire du schema
		// multi-tabbles, qui seront elles meme prise en compte
		kwcClass =
		    KWClassDomain::GetCurrentDomain()->LookupClass(sourcePLDatabase->GetDatabase()->GetClassName());
		lClassEstimatedObjectNumber *= max(1, ComputeCreatedEntityNumber(kwcClass));

		// Estimation heuristique du max des nombres totaux d'objets dans les fichiers,
		// sur l'ensemble des classes de selection, en partant des objets du fichier principal
		lMaxClassEstimatedObjectNumber = lClassEstimatedObjectNumber;
		for (nClass = 0; nClass < masterSelectionOperandDataSampler->GetClassSelectionData()->GetSize();
		     nClass++)
		{
			classSelectionData =
			    cast(KDClassSelectionData*,
				 masterSelectionOperandDataSampler->GetClassSelectionData()->GetAt(nClass));

			// Estimation heuristique du nombre total d'objet dans les fichiers pour une classe de selection
			if (classSelectionData->GetClassSelectionOperandData()->GetSize() > 0)
			{
				// Analyse des mappings utilises de la bonne classe dans le cas multi-tables
				lClassEstimatedObjectNumber = 0;
				if (sourcePLDatabase->IsMultiTableTechnology())
				{
					sourceMTDatabase = shared_sourceDatabase.GetMTDatabase();

					// Parcours des mapping pour identifier ceux qui concernes
					for (nMapping = 0; nMapping < sourceMTDatabase->GetTableNumber(); nMapping++)
					{
						mapping = sourceMTDatabase->GetUsedMappingAt(nMapping);
						if (mapping != NULL and
						    mapping->GetClassName() == classSelectionData->GetClassName())
							lClassEstimatedObjectNumber +=
							    sourceMTDatabase->GetInMemoryEstimatedFileObjectNumbers()
								->GetAt(nMapping);
					}

					// Prise en compte des Entity creees, comme pour le fichier principal
					kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(
					    classSelectionData->GetClassName());
					lClassEstimatedObjectNumber *= max(1, ComputeCreatedEntityNumber(kwcClass));

					// Mise a jour du nombre maximum d'objet sur l'ensemble des classes
					lMaxClassEstimatedObjectNumber =
					    max(lMaxClassEstimatedObjectNumber, lClassEstimatedObjectNumber);
				}
			}
		}

		// Acces a la taille des echantillons et au nombre total de valeurs a collecter.
		// On utilise le min de la taille d'echantillon souhaitee et du nombre total d'objets
		// effectifs dans les fichier de donnees.
		// Cela permet de limiter si necessaire la memoire necessaire en se basant sur les donnees disponibles,
		// notamment dans les cas ou on genere beaucoup de variables a partir de petites bases de donnees.
		// Notons que c'est une estimation heuristique macroscopique, qui pourrait etre affinee dans le cas de
		// schemas multi-tables complexes comprenant plusieurs type de tables avec des variations d'une part sur le
		// nombre de variables a generer par type de dictionnaire et le nombre d'objets dans les fichiers de donnees.
		// Cet affinage est plus complexe a implementer, probablement utile uniquement dans certains cas "extremes":
		// il n'est pas prevu de le prendre en compte, puisque l'enjeu est ici uniquement de diminuer potentiellement
		// les exigences memoires dans le cas de petites bases: une solution heuristique approximative et suffisante.
		lMaxSampleSize = masterSelectionOperandDataSampler->GetMaxSampleSize();
		lTotalValueNumber = min(lMaxSampleSize, lMaxClassEstimatedObjectNumber) *
				    masterSelectionOperandDataSampler->GetTotalSelectionOperandNumber();

		// Taille memoire necessaire pour collecter l'ensemble des references aux objet, plus les echantillons
		// de valeurs On ignore le probleme des variables Symbol qui potentiellement exigent de la memoire
		// supplementaire
		lAllSamplesNecessaryMemory = masterSelectionOperandDataSampler->GetClassSelectionData()->GetSize() *
					     lMaxSampleSize *
					     (slSample.GetUsedMemoryPerElement() + sizeof(KDClassSelectionObjectRef));
		lAllSamplesNecessaryMemory += lTotalValueNumber * sizeof(KWValue);

		// En variable partagee: specification de l'echantillonneur
		GetResourceRequirements()->GetSharedRequirement()->GetMemory()->UpgradeMin(
		    lSelectionOperandDataSamplerSpecUsedMemory);
		GetResourceRequirements()->GetSharedRequirement()->GetMemory()->UpgradeMax(
		    lSelectionOperandDataSamplerSpecUsedMemory);

		// En variable par esclave: specification de l'echantillonneur, plus un echantillon
		// On ignore la place prise pour referencer les objets des tables externes dans des dictionnaires, on
		// considerant que c'est petit devant les objets externes eux meme ainsi que leur references par cle
		// memorise par le ObjectReferenceSolver de la base multi-tables
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(
		    lSelectionOperandDataSamplerSpecUsedMemory + lAllSamplesNecessaryMemory);
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(
		    lSelectionOperandDataSamplerSpecUsedMemory + lAllSamplesNecessaryMemory);

		// En variable du maitre: deux fois l'echantillon, celui du maitre est celui local de l'esclave en cours
		// d'agregation Apres la fin de la tache, il n'y en aura plus qu'un, mais on estime qu'il y a aura assez
		// de memoire pour construire les partiles a partir des echantillons, puis les variables construites
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMin(
		    lSelectionOperandDataSamplerSpecUsedMemory + lAllSamplesNecessaryMemory);
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMax(
		    lSelectionOperandDataSamplerSpecUsedMemory + lAllSamplesNecessaryMemory);

		// Affichage des ressources
		if (bDisplay)
		{
			cout << "KDSelectionOperandSamplingTask::ComputeResourceRequirements\n";
			cout << "\tSelectionOperandDataSamplerSpecUsedMemory\t"
			     << lSelectionOperandDataSamplerSpecUsedMemory << "\n";
			cout << "\tMaxClassEstimatedObjectNumber\t" << lMaxClassEstimatedObjectNumber << "\n";
			cout << "\tMaxSampleSize\t" << lMaxSampleSize << "\n";
			cout << "\tTotalValueNumber\t" << lTotalValueNumber << "\n";
			cout << "\tClassSelectionDataNumber\t"
			     << masterSelectionOperandDataSampler->GetClassSelectionData()->GetSize() << "\n";
			cout << "\tTotalSelectionOperandNumber\t"
			     << masterSelectionOperandDataSampler->GetTotalSelectionOperandNumber() << "\n";
			cout << "\tMemoryPerElement\t"
			     << (slSample.GetUsedMemoryPerElement() + sizeof(KDClassSelectionObjectRef)) << "\n";
			cout << "\tAllSamplesNecessaryMemory\t" << lAllSamplesNecessaryMemory << "\n";
			cout << *GetResourceRequirements() << "\n";
		}
	}

	return bOk;
}

boolean KDSelectionOperandSamplingTask::MasterInitialize()
{
	boolean bOk;
	longint lDatabaseFileSize;

	require(masterSelectionOperandDataSampler != NULL);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterInitialize();

	// Parametrage de la taille totale des fichiers de la base
	// On attend d'etre dans le MasterInitialize pour acceder a cette information issue
	// de l'indexation des fichier de la base en entree
	lDatabaseFileSize = shared_sourceDatabase.GetPLDatabase()->GetFileSizeAt(0);
	shared_selectionOperandDataSamplerSpec.GetSelectionOperandDataSampler()->SetDatabaseFileSize(lDatabaseFileSize);
	masterSelectionOperandDataSampler->SetDatabaseFileSize(lDatabaseFileSize);

	// Finalisation de la variable partagee
	if (bOk)
		shared_selectionOperandDataSamplerSpec.FinalizeSpecification();

	// Trace
	if (bTrace)
	{
		cout << GetTaskName() << " MasterInitialize " << GetTaskIndex() << " [" << GetProcessId() << "]\n"
		     << *shared_selectionOperandDataSamplerSpec.GetSelectionOperandDataSampler() << flush;
	}

	// Initialisation du flag de gestion de la derniere passe d'analyse en cas de tables externes
	bLastSlaveProcessDone = false;
	return bOk;
}

boolean KDSelectionOperandSamplingTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;
	int i;
	KDClassSelectionData* classSelectionData;

	// Est-ce qu'il y a encore du travail ?
	if (bLastSlaveProcessDone)
	{
		// Tous les esclaves ont effectue la derniere passe de SlaveProcess
		bIsTaskFinished = true;
		return true;
	}

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);
	input_bLastRound = false;

	// Si fin de traitement des instances de la base, les esclaves effectuent la derniere passe de SlaveProcess
	// pour traiter les tables externes
	if (bOk and bIsTaskFinished)
	{
		// Necessite d'une dernier passe en cas de tables externes
		if (shared_sourceDatabase.GetPLDatabase()->IsMultiTableTechnology() and
		    shared_sourceDatabase.GetMTDatabase()->GetReferencedTableNumber() > 0)
		{
			input_bLastRound = true;
			bIsTaskFinished = false;
			SetSlaveAtRestAfterProcess();
		}
		// Sinon, pas besoin de passe supplementaire
		else
			bLastSlaveProcessDone = true;
	}

	// Parametrage du seuil d'extraction par classe
	assert(input_dvObjectSelectionProbThresholds.GetSize() == 0);
	for (i = 0; i < masterSelectionOperandDataSampler->GetClassSelectionData()->GetSize(); i++)
	{
		classSelectionData =
		    cast(KDClassSelectionData*, masterSelectionOperandDataSampler->GetClassSelectionData()->GetAt(i));
		assert(classSelectionData->GetSampleSize() <= masterSelectionOperandDataSampler->GetMaxSampleSize());

		// Seuil de probabilite pour les objets a garder si l'echantillon est plein
		if (classSelectionData->GetSampleSize() == masterSelectionOperandDataSampler->GetMaxSampleSize())
			input_dvObjectSelectionProbThresholds.Add(classSelectionData->GetSampleMinSelectionProb());
		// Pas de seuil sinon
		else
			input_dvObjectSelectionProbThresholds.Add(0);
	}
	assert(input_dvObjectSelectionProbThresholds.GetSize() ==
	       masterSelectionOperandDataSampler->GetClassSelectionData()->GetSize());
	return bOk;
}

boolean KDSelectionOperandSamplingTask::MasterAggregateResults()
{
	boolean bOk = true;

	require(masterSelectionOperandDataSampler != NULL);

	// Si tous les esclaves sont en sommeil, c'est qu'ils ont tous execute le dernier SlaveProcess
	// Il faut les reveiller pour recevoir l'ordre d'arret
	if (not bLastSlaveProcessDone and GetRestingSlaveNumber() == GetProcessNumber())
	{
		SetAllSlavesAtWork();
		bLastSlaveProcessDone = true;
	}

	// Appel de la methode ancetre dans le cas standard
	if (not output_bLastRound)
		bOk = KWDatabaseTask::MasterAggregateResults();

	// Finalisation de la variable revue de l'esclave
	output_selectionOperandDataSampler.FinalizeSpecification();
	assert(output_selectionOperandDataSampler.GetSelectionOperandDataSampler()->Check());

	// Trace des donnees recues
	if (bTrace)
	{
		cout << GetTaskName() << " MasterAggregate " << GetTaskIndex() << " [" << GetProcessId() << "] ("
		     << output_bLastRound << ")\n"
		     << *output_selectionOperandDataSampler.GetSelectionOperandDataSampler() << flush;
	}

	// Prise en compte des donnees collectees par l'esclave
	masterSelectionOperandDataSampler->AggregateClassSelectionData(
	    output_selectionOperandDataSampler.GetSelectionOperandDataSampler());

	// Trace des donnees agregees
	if (bTrace)
	{
		cout << "=>\n" << *masterSelectionOperandDataSampler << flush;
	}
	return bOk;
}

boolean KDSelectionOperandSamplingTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterFinalize(bProcessEndedCorrectly);
	return bOk;
}

boolean KDSelectionOperandSamplingTask::SlaveInitializePrepareDatabase()
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveInitializePrepareDatabase();

	// Finalisation de la variable partagee
	if (bOk)
		shared_selectionOperandDataSamplerSpec.FinalizeSpecification();

	// Trace
	if (bTrace)
	{
		cout << GetTaskName() << " SlaveInitialize " << GetTaskIndex() << " [" << GetProcessId() << "]\n"
		     << *shared_selectionOperandDataSamplerSpec.GetSelectionOperandDataSampler() << flush;
	}
	return bOk;
}

boolean KDSelectionOperandSamplingTask::SlaveInitializeOpenDatabase()
{
	boolean bOk;

	require(slaveSelectionOperandDataSampler == NULL);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveInitializeOpenDatabase();

	// Finalisation de la variable partagee
	if (bOk)
		shared_selectionOperandDataSamplerSpec.FinalizeSpecification();

	// Creation de l'echantillonneur de donnees de l'esclave en fin d'initilisation de l'esclave
	if (bOk)
		slaveSelectionOperandDataSampler =
		    shared_selectionOperandDataSamplerSpec.GetSelectionOperandDataSampler()->CloneSpec();

	// Memorisation des objets references globaux apres ouverture de la base
	if (bOk)
	{
		if (shared_sourceDatabase.GetPLDatabase()->IsMultiTableTechnology())
			slaveSelectionOperandDataSampler->RegisterAllReferencedObjects(
			    shared_sourceDatabase.GetMTDatabase());
	}
	return bOk;
}

boolean KDSelectionOperandSamplingTask::SlaveProcessStartDatabase()
{
	boolean bOk = true;

	// Appel de la methode ancetre dans le cas standard
	if (not input_bLastRound)
		bOk = KWDatabaseTask::SlaveProcessStartDatabase();
	return bOk;
}

boolean KDSelectionOperandSamplingTask::SlaveProcessExploitDatabase()
{
	boolean bOk = true;
	int i;
	KDClassSelectionData* classSelectionData;

	require(slaveSelectionOperandDataSampler != NULL);
	require(slaveSelectionOperandDataSampler->GetClassSelectionData()->GetSize() ==
		input_dvObjectSelectionProbThresholds.GetSize());

	// Parametrage du seuil d'extraction par classe
	for (i = 0; i < slaveSelectionOperandDataSampler->GetClassSelectionData()->GetSize(); i++)
	{
		classSelectionData =
		    cast(KDClassSelectionData*, slaveSelectionOperandDataSampler->GetClassSelectionData()->GetAt(i));
		assert(classSelectionData->GetSampleSize() == 0);
		classSelectionData->SetObjectSelectionProbThreshold(input_dvObjectSelectionProbThresholds.GetAt(i));
	}

	// Appel de la methode ancetre dans le cas standard
	if (not input_bLastRound)
		bOk = KWDatabaseTask::SlaveProcessExploitDatabase();
	// Sinon, extraction des valeurs de selection de tous les objets references globaux
	// ayant ete utilises au moins une fois
	else
	{
		if (shared_sourceDatabase.GetPLDatabase()->IsMultiTableTechnology())
			slaveSelectionOperandDataSampler->ExtractAllSelectionReferencedObjects(
			    shared_sourceDatabase.GetMTDatabase());
	}
	return bOk;
}

boolean KDSelectionOperandSamplingTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	boolean bOk = true;
	NumericKeyDictionary nkdAllSubObjectsTask;
	NumericKeyDictionary nkdAllSubObjects;
	longint lSubObjectIndex = 0;

	require(slaveSelectionOperandDataSampler != NULL);

	// Analyse de l'objet pour enregistrer les objets des classes de selection
	slaveSelectionOperandDataSampler->ExtractSelectionObjects(kwoObject, kwoObject->GetCreationIndex(),
								  lSubObjectIndex, &nkdAllSubObjects);

	// Nettoyage
	nkdAllSubObjects.RemoveAll();
	return bOk;
}

boolean KDSelectionOperandSamplingTask::SlaveProcessStopDatabase(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	KDSelectionOperandDataSampler* resultSelectionOperandDataSampler;

	require(slaveSelectionOperandDataSampler != NULL);

	// Appel de la methode ancetre dans le cas standard
	if (not input_bLastRound)
		bOk = KWDatabaseTask::SlaveProcessStopDatabase(bProcessEndedCorrectly);

	// Initialisation des donnees en sortie de l'esclave a partir des specification partagees
	resultSelectionOperandDataSampler =
	    shared_selectionOperandDataSamplerSpec.GetSelectionOperandDataSampler()->CloneSpec();

	// On prend les donnees collectees par l'achantilonneur local a l'esclace par echange de son contenu
	// La variable de travail de l'esclave continue a avoir le contexte de travail avec les references aux objets
	// des tables externes
	resultSelectionOperandDataSampler->SwapClassSelectionData(slaveSelectionOperandDataSampler);
	assert(resultSelectionOperandDataSampler->Check());

	// Parametrage de la variable en sortie de l'esclave
	output_selectionOperandDataSampler.SetSelectionOperandDataSampler(resultSelectionOperandDataSampler);

	// On memorise le flag en sortie
	output_bLastRound = input_bLastRound;

	// Trace
	if (bTrace)
	{
		cout << GetTaskName() << " SlaveProcess " << GetTaskIndex() << " [" << GetProcessId() << "] ("
		     << output_bLastRound << ")\n"
		     << *resultSelectionOperandDataSampler << flush;
	}
	return bOk;
}

boolean KDSelectionOperandSamplingTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Nettoyage des objets references globaux avant la fermeture des bases
	if (slaveSelectionOperandDataSampler != NULL)
	{
		slaveSelectionOperandDataSampler->CleanAllReferencedObjects();
		delete slaveSelectionOperandDataSampler;
		slaveSelectionOperandDataSampler = NULL;
	}

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveFinalize(bProcessEndedCorrectly);
	ensure(slaveSelectionOperandDataSampler == NULL);
	return bOk;
}

int KDSelectionOperandSamplingTask::ComputeCreatedEntityNumber(const KWClass* kwcClass) const
{
	int nCreatedEntityNumber;
	KWAttribute* attribute;

	require(kwcClass != NULL);

	// Comptage des attributs de type Entity cree par des regles
	nCreatedEntityNumber = 0;
	attribute = kwcClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		if (attribute->GetType() == KWType::Object and attribute->GetDerivationRule() != NULL and
		    not attribute->GetDerivationRule()->GetReference())
			nCreatedEntityNumber++;
		kwcClass->GetNextAttribute(attribute);
	}
	return nCreatedEntityNumber;
}
