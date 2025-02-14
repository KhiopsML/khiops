// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDTextTokenSampleCollectionTask.h"

// Inclusion du header dans le source pour eviter une dependance cyclique
#include "KDTextFeatureConstruction.h"

KDTextTokenSampleCollectionTask::KDTextTokenSampleCollectionTask()
{
	ivMasterTokenNumbers = NULL;
	oaMasterCollectedTokenSamples = NULL;
	lPerformanceInitialHeapMemory = 0;

	// Variables partagees
	DeclareSharedParameter(&shared_sTextFeatures);
	DeclareSharedParameter(&shared_bIsFirstPass);
	DeclareSharedParameter(&shared_bComputeExactTokenFrequencies);
	DeclareSharedParameter(&shared_ivFirstPassTokenNumbers);
	shared_oaSecondPassSpecificTokens = new PLShared_ObjectArrayArray(new PLShared_TokenFrequency);
	DeclareSharedParameter(shared_oaSecondPassSpecificTokens);

	// Variables en sortie des esclaves
	output_oaTokens = new PLShared_ObjectArrayArray(new PLShared_TokenFrequency);
	DeclareTaskOutput(output_oaTokens);

	// Initialisation des variables partagees
	shared_sTextFeatures.SetValue("ngrams");
	shared_bIsFirstPass = true;
	shared_bComputeExactTokenFrequencies = true;
}

KDTextTokenSampleCollectionTask::~KDTextTokenSampleCollectionTask()
{
	delete shared_oaSecondPassSpecificTokens;
	delete output_oaTokens;
}

boolean KDTextTokenSampleCollectionTask::CollectTokenSamples(const KWDatabase* sourceDatabase,
							     const IntVector* ivTokenNumbers,
							     ObjectArray* oaCollectedTokenSamples)
{
	boolean bOk = true;
	boolean bDisplay = false;
	const int nMaxDisplayedTokenNumber = 10000;
	KWClass* kwcMainClass;
	int nAttribute;
	KWAttribute* attribute;
	int nTokenNumber;
	int nToken;
	ObjectArray* oaTokens;
	KWTokenFrequency* token;

	require(sourceDatabase != NULL);
	require(ivTokenNumbers != NULL);
	require(oaCollectedTokenSamples != NULL);
	require(ivTokenNumbers->GetSize() == oaCollectedTokenSamples->GetSize());

	// Recherche de la classe principale
	kwcMainClass = KWClassDomain::GetCurrentDomain()->LookupClass(sourceDatabase->GetClassName());
	check(kwcMainClass);

	// Verifications de la validite des parametres
	assert(kwcMainClass->GetUsedAttributeNumber() > 0);
	assert(kwcMainClass->GetUsedAttributeNumber() ==
	       kwcMainClass->GetUsedAttributeNumberForType(KWType::Text) +
		   kwcMainClass->GetUsedAttributeNumberForType(KWType::TextList));
	assert(kwcMainClass->GetUsedAttributeNumber() == ivTokenNumbers->GetSize());
	assert(kwcMainClass->GetUsedAttributeNumber() == oaCollectedTokenSamples->GetSize());

	// Memorisation des parametres principaux
	ivMasterTokenNumbers = ivTokenNumbers;
	oaMasterCollectedTokenSamples = oaCollectedTokenSamples;

	// On ne souhaite que les messages de fin de tache en cas d'arret
	SetDisplaySpecificTaskMessage(false);
	SetDisplayTaskTime(false);
	SetDisplayEndTaskMessage(true);

	// Implementation prototype en attendant l'implementation des methodes de la tache
	bOk = SequentialCollectTokenSamples(sourceDatabase);

	// Lancement de la tache
	// bOk = InternalCollectTokenSamples(sourceDatabase);

	// Reinitialisation des parametres principaux
	ivMasterTokenNumbers = NULL;
	oaMasterCollectedTokenSamples = NULL;

	// Affichage
	if (bDisplay)
	{
		cout << "CollectTokenSamples\t" << kwcMainClass->GetName() << "\t" << GetTextFeatures() << "\t"
		     << BooleanToString(bOk) << "\n";
		for (nAttribute = 0; nAttribute < kwcMainClass->GetUsedAttributeNumber(); nAttribute++)
		{
			attribute = kwcMainClass->GetUsedAttributeAt(nAttribute);
			assert(KWType::IsTextBased(attribute->GetType()));

			// Recherche du nombre de tokens a extraire
			nTokenNumber = ivTokenNumbers->GetAt(nAttribute);
			assert(nTokenNumber >= 0);

			// Recherche du tableau de token a alimenter
			oaTokens = cast(ObjectArray*, oaCollectedTokenSamples->GetAt(nAttribute));
			assert(oaTokens != NULL);

			// Affichage des tokens collectes par attribut de type texte
			cout << "\t" << KWType::ToString(attribute->GetType()) << "\t" << attribute->GetName() << "\t"
			     << oaTokens->GetSize() << "\t" << nTokenNumber << "\n";
			for (nToken = 0; nToken < oaTokens->GetSize(); nToken++)
			{
				token = cast(KWTokenFrequency*, oaTokens->GetAt(nToken));
				if (nToken > nMaxDisplayedTokenNumber)
				{
					cout << "\t\t...\n";
					break;
				}
				cout << "\t\t" << nToken + 1 << "\t" << token->GetFrequency() << "\t"
				     << TextService::ByteStringToWord(token->GetToken()) << "\n";
			}
			cout << flush;
		}
	}
	return bOk;
}

void KDTextTokenSampleCollectionTask::SetTextFeatures(const ALString& sValue)
{
	require(KWTextTokenizer::CheckTextFeatures(sValue));
	shared_sTextFeatures.SetValue(sValue);
}

const ALString& KDTextTokenSampleCollectionTask::GetTextFeatures() const
{
	ensure(KWTextTokenizer::CheckTextFeatures(shared_sTextFeatures.GetValue()));
	return shared_sTextFeatures.GetValue();
}

boolean KDTextTokenSampleCollectionTask::DummyCollectTokenSamples(const KWDatabase* sourceDatabase)
{
	boolean bOk = true;
	KWClass* kwcMainClass;
	int nAttribute;
	KWAttribute* attribute;
	int nTokenNumber;
	int nToken;
	ObjectArray* oaTokens;
	KWTokenFrequency* token;
	ALString sTmp;

	require(sourceDatabase != NULL);
	require(ivMasterTokenNumbers != NULL);
	require(oaMasterCollectedTokenSamples != NULL);

	// Recherche de la classe principale
	kwcMainClass = KWClassDomain::GetCurrentDomain()->LookupClass(sourceDatabase->GetClassName());
	check(kwcMainClass);

	// Collecte des tokens pour chaque chemin d'acces aux attributs de type Text
	for (nAttribute = 0; nAttribute < kwcMainClass->GetUsedAttributeNumber(); nAttribute++)
	{
		attribute = kwcMainClass->GetUsedAttributeAt(nAttribute);
		assert(KWType::IsTextBased(attribute->GetType()));

		// Recherche du nombre de tokens a extraire
		nTokenNumber = ivMasterTokenNumbers->GetAt(nAttribute);
		assert(nTokenNumber >= 0);

		// Recherche du tableau de token a alimenter
		oaTokens = cast(ObjectArray*, oaMasterCollectedTokenSamples->GetAt(nAttribute));
		assert(oaTokens != NULL);
		assert(oaTokens->GetSize() == 0);

		// Generation des tokens
		for (nToken = 0; nToken < nTokenNumber; nToken++)
		{
			token = new KWTokenFrequency;
			if (nToken < 26)
				token->SetToken(char('A' + nToken));
			else
				token->SetToken(sTmp + "W" + IntToString(nToken + 1));
			token->SetFrequency(nTokenNumber + 1 - nToken);
			oaTokens->Add(token);
		}
	}
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::SequentialCollectTokenSamples(const KWDatabase* sourceDatabase)
{
	boolean bOk = true;
	boolean bDisplayPerformanceStats = false;
	boolean bActivateStreamTokenCollection = true;
	boolean bCollectExactTokenFrequencies = true;
	ObjectArray oaTextTokenizers;
	KWTextTokenizer* textTokenizer;
	ObjectArray oaCollectedTokens;
	KWDatabase* database;
	int nAttribute;
	ObjectArray* oaTokens;
	ALString sTmp;

	require(sourceDatabase != NULL);
	require(not sourceDatabase->IsOpenedForRead());
	require(not sourceDatabase->IsOpenedForWrite());
	require(ivMasterTokenNumbers != NULL);
	require(oaMasterCollectedTokenSamples != NULL);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialisation

	// Debut de la collecte des indicateurs de performance
	StartCollectPerformanceIndicators(bDisplayPerformanceStats);

	// Initialisation des tokenizers utilises en variables de travail
	oaTextTokenizers.SetSize(oaMasterCollectedTokenSamples->GetSize());
	for (nAttribute = 0; nAttribute < oaMasterCollectedTokenSamples->GetSize(); nAttribute++)
	{
		// Creation d'un tokenizer selon le type de features de texte
		textTokenizer = KWTextTokenizer::CreateTextTokenizer(GetTextFeatures());

		// Parametrage de la collection en mode flux en specifiant nombre max de tokens a conserver pour assurer
		// la stabilite des resultats
		if (bActivateStreamTokenCollection)
			textTokenizer->SetMaxCollectedTokenNumber(
			    GetMaxStreamCollectedTokenNumber(ivMasterTokenNumbers->GetAt(nAttribute)));

		// Memorisation du tokenizer associer a l'attribut
		oaTextTokenizers.SetAt(nAttribute, textTokenizer);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Analyse de la base: premiere passe de collecte des tokens les plus frequents

	// Cast en database, ce qui ne sera pas necessaire avec la version standard de l'implementation avec les
	// methodes de la tache
	database = cast(KWDatabase*, sourceDatabase);

	// Analyse de la base pour collmecter les tokens les plus frequents
	AnalyseDatabase(database, &oaTextTokenizers);
	DisplayPerformanceIndicators("First database pass", &oaTextTokenizers);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Deuxieme passe de collecte des effectifs exact par tokens

	// On ne le fait que si c'est necessaire et demande
	if (bActivateStreamTokenCollection and bCollectExactTokenFrequencies)
	{
		// Parametrage des tokenizers pour specifier explicitement les tokens a collecter
		for (nAttribute = 0; nAttribute < oaMasterCollectedTokenSamples->GetSize(); nAttribute++)
		{
			textTokenizer = cast(KWTextTokenizer*, oaTextTokenizers.GetAt(nAttribute));

			// Export des tokens collectes
			textTokenizer->ExportTokens(&oaCollectedTokens);
			textTokenizer->CleanCollectedTokens();

			// Parametrage des tokens a extraire en prenant ceux de la premiere passe
			textTokenizer->SetMaxCollectedTokenNumber(0);
			textTokenizer->SetSpecificTokens(&oaCollectedTokens);
			oaCollectedTokens.DeleteAll();
		}

		// Analyse de la base, cette fois en ne collectant que les effectifs des tokens specifies
		AnalyseDatabase(database, &oaTextTokenizers);
		DisplayPerformanceIndicators("Second database pass", &oaTextTokenizers);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Finalisation

	// Collecte des resultats des dictionnaires de tokens dans les tableaux de tokens en sortie
	for (nAttribute = 0; nAttribute < oaMasterCollectedTokenSamples->GetSize(); nAttribute++)
	{
		textTokenizer = cast(KWTextTokenizer*, oaTextTokenizers.GetAt(nAttribute));
		oaTokens = cast(ObjectArray*, oaMasterCollectedTokenSamples->GetAt(nAttribute));
		assert(oaTokens->GetSize() == 0);

		// Export des tokens vers le tableau
		textTokenizer->ExportFrequentTokens(oaTokens, ivMasterTokenNumbers->GetAt(nAttribute));
		DisplayPerformanceIndicators(
		    sTmp + "Token extraction and sort (var " + IntToString(nAttribute + 1) + ")", &oaTextTokenizers);

		// Nettoyage
		delete textTokenizer;
		oaTextTokenizers.SetAt(nAttribute, NULL);
	}

	// Fin de la collecte des indicateurs de performance
	StopCollectPerformanceIndicators();

	return bOk;
}

boolean KDTextTokenSampleCollectionTask::AnalyseDatabase(KWDatabase* database, ObjectArray* oaTextTokenizers)
{
	boolean bOk = true;
	KWObject* kwoObject;
	longint lObjectNumber;
	longint lRecordNumber;

	require(database != NULL);
	require(not database->IsOpenedForRead());
	require(not database->IsOpenedForWrite());
	require(oaTextTokenizers != NULL);
	require(ivMasterTokenNumbers != NULL);
	require(oaMasterCollectedTokenSamples != NULL);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Analyse de la base

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Analyze database text variables " + database->GetDatabaseName());

	// Ouverture de la base en lecture
	bOk = database->OpenForRead();

	// Lecture d'objets dans la base
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		lRecordNumber = 0;
		lObjectNumber = 0;
		while (not database->IsEnd())
		{
			kwoObject = database->Read();
			lRecordNumber++;
			if (kwoObject != NULL)
			{
				lObjectNumber++;

				// Analyse de l'objet
				AnalyseDatabaseObject(kwoObject, oaTextTokenizers);

				// Nettoyage de l'objet
				delete kwoObject;
			}
			// Arret si interruption utilisateur
			else if (TaskProgression::IsInterruptionRequested())
			{
				assert(kwoObject == NULL);
				bOk = false;
				break;
			}

			// Arret si erreur
			if (database->IsError())
			{
				bOk = false;
				Object::AddError("Analyse database interrupted because of errors");
				break;
			}

			// Suivi de la tache
			if (TaskProgression::IsRefreshNecessary())
				TaskProgression::DisplayProgression((int)(100 * database->GetReadPercentage()));
		}
		Global::DesactivateErrorFlowControl();

		// Test si interruption sans qu'il y ait d'erreur
		if (not database->IsError() and TaskProgression::IsInterruptionRequested())
		{
			bOk = false;
			Object::AddWarning("Analyze database interrupted by user");
		}

		// Fermeture
		bOk = database->Close() and bOk;
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::AnalyseDatabaseObject(const KWObject* kwoObject, ObjectArray* oaTextTokenizers)
{
	const KWClass* mainClass;
	KWAttribute* attribute;
	KWTextTokenizer* textTokenizer;
	int nAttribute;

	require(kwoObject != NULL);
	require(oaTextTokenizers != NULL);

	// Acces a la classe de l'objet
	mainClass = kwoObject->GetClass();
	assert(mainClass->GetUsedAttributeNumber() == oaTextTokenizers->GetSize());

	// Anayse de l'objet par parcours de ses attributs
	for (nAttribute = 0; nAttribute < mainClass->GetUsedAttributeNumber(); nAttribute++)
	{
		attribute = mainClass->GetUsedAttributeAt(nAttribute);
		textTokenizer = cast(KWTextTokenizer*, oaTextTokenizers->GetAt(nAttribute));

		// Analyse de la valeur selon son type
		assert(KWType::IsTextBased(attribute->GetType()));
		if (attribute->GetType() == KWType::Text)
			textTokenizer->TokenizeSymbol(kwoObject->GetTextValueAt(attribute->GetLoadIndex()));
		else
			textTokenizer->TokenizeSymbolVector(kwoObject->GetTextListValueAt(attribute->GetLoadIndex()));
	}
	return true;
}

int KDTextTokenSampleCollectionTask::GetMaxStreamCollectedTokenNumber(int nRequestedTokenNumber) const
{
	int nMaxStreamCollectedTokenNumber;
	require(nRequestedTokenNumber > 0);

	// Parametrage de la collection en mode flux en specifiant nombre max de tokens a conserver, en gardant une
	// marge Les experiences montrenent qu'un facteur de 2 est largement insuffisant, et qu'un facteur de 10 semble
	// raisonnable. On ajoute egalement un nombre minimal de 1000 tokens, pour stabiliser les resultats dans le cas
	// de petits nombres de tokens. De toutes facon, cela ne coute pas tres cher ni en temps ni en memoire, car le
	// temps est domine par le parsing exhaustif de tous les tokens des textes, qui est effectue de toutes facons
	nMaxStreamCollectedTokenNumber = 1000 + nRequestedTokenNumber * 10;
	return nMaxStreamCollectedTokenNumber;
}

boolean KDTextTokenSampleCollectionTask::InternalCollectTokenSamples(const KWDatabase* sourceDatabase)
{
	boolean bOk = true;
	int nAttribute;
	ALString sTmp;

	require(sourceDatabase != NULL);
	require(not sourceDatabase->IsOpenedForRead());
	require(not sourceDatabase->IsOpenedForWrite());
	require(ivMasterTokenNumbers != NULL);
	require(oaMasterCollectedTokenSamples != NULL);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Premiere passe de collecte d'un echantillon de tokens

	// Initialisation des parametres
	shared_bIsFirstPass = true;
	shared_ivFirstPassTokenNumbers.GetIntVector()->SetSize(ivMasterTokenNumbers->GetSize());
	for (nAttribute = 0; nAttribute < ivMasterTokenNumbers->GetSize(); nAttribute++)
	{
		// Parametrage de la collection en mode flux en specifiant nombre max de tokens a conserver pour assurer
		// la stabilite des resultats
		shared_ivFirstPassTokenNumbers.SetAt(
		    nAttribute, GetMaxStreamCollectedTokenNumber(ivMasterTokenNumbers->GetAt(nAttribute)));
	}
	shared_oaSecondPassSpecificTokens->Clean();
	assert(CheckPassParameters());

	// Declenchement de tache pour la premiere passe
	bOk = RunDatabaseTask(sourceDatabase);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Deuxieme passe de collecte des effectifs exact par tokens

	// On ne le fait que si c'est necessaire et demande
	if (bOk and shared_bComputeExactTokenFrequencies)
	{
		// Initialisation des parametres, en prenant en prenant en entree les tokens produits par la tache
		shared_bIsFirstPass = false;
		shared_ivFirstPassTokenNumbers.GetIntVector()->SetSize(0);
		shared_oaSecondPassSpecificTokens->SetObjectArray(oaMasterCollectedTokenSamples);
		oaMasterCollectedTokenSamples->SetSize(0);
		assert(CheckPassParameters());

		// Declenchement de tache pour la premiere passe
		bOk = RunDatabaseTask(sourceDatabase);

		// On derefence le tableau de la variable partagee pour ne pas qu'il soit detruit avec cette variable
		shared_oaSecondPassSpecificTokens->RemoveObject();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Finalisation

	// Nettoyage si echec
	if (not bOk)
	{
		for (nAttribute = 0; nAttribute < oaMasterCollectedTokenSamples->GetSize(); nAttribute++)
		{
			cast(ObjectArray*, oaMasterCollectedTokenSamples->GetAt(nAttribute))->DeleteAll();
		}
		oaMasterCollectedTokenSamples->DeleteAll();
	}

	// Nettoyage des variables partagees
	shared_bIsFirstPass = true;
	shared_ivFirstPassTokenNumbers.GetIntVector()->SetSize(0);
	shared_oaSecondPassSpecificTokens->Clean();
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::CheckPassParameters() const
{
	boolean bOk;

	bOk = shared_bIsFirstPass or shared_oaSecondPassSpecificTokens->GetObjectArray()->GetSize() > 0;
	bOk = shared_bIsFirstPass or shared_ivFirstPassTokenNumbers.GetSize() == 0;
	bOk = not shared_bIsFirstPass or shared_ivFirstPassTokenNumbers.GetSize() > 0;
	bOk = not shared_bIsFirstPass or shared_oaSecondPassSpecificTokens->GetObjectArray()->GetSize() == 0;
	bOk = bOk and KWTextTokenizer::CheckTextFeatures(shared_sTextFeatures.GetValue());
	return bOk;
}

void KDTextTokenSampleCollectionTask::StartCollectPerformanceIndicators(boolean bDisplay)
{
	require(not performanceTimer.IsStarted());
	require(performanceTimer.GetElapsedTime() == 0);
	require(lPerformanceInitialHeapMemory == 0);

	// Une valeur strictement positive de la memoire sert d'indicateur pur l'affichage
	if (bDisplay)
	{
		performanceTimer.Start();
		lPerformanceInitialHeapMemory = MemGetHeapMemory();

		// Affichage d'une entete
		cout << "Time\tMemory\tTokens\tTokenisation step" << endl;
	}
}

void KDTextTokenSampleCollectionTask::StopCollectPerformanceIndicators()
{
	// Reinitialisation si affichage en cours
	if (lPerformanceInitialHeapMemory > 0)
	{
		performanceTimer.Stop();
		performanceTimer.Reset();
		lPerformanceInitialHeapMemory = 0;

		// Affichage d'une ligne de fin
		cout << endl;
	}
}

void KDTextTokenSampleCollectionTask::DisplayPerformanceIndicators(const ALString& sLabel,
								   const ObjectArray* oaTokenizers)
{
	KWTextTokenizer* tokenizer;
	int i;
	int nTotalTokenNumber;

	require(oaTokenizers != NULL);

	// Affichage des performance si elles sont collectees
	if (lPerformanceInitialHeapMemory > 0)
	{
		// Calcul du nombre total de token connectes
		nTotalTokenNumber = 0;
		for (i = 0; i < oaTokenizers->GetSize(); i++)
		{
			tokenizer = cast(KWTextTokenizer*, oaTokenizers->GetAt(i));
			if (tokenizer != NULL)
				nTotalTokenNumber += tokenizer->GetCollectedTokenNumber();
		}

		// Affichage des indicateurs de performance
		cout << performanceTimer.GetElapsedTime() << "\t";
		cout << LongintToHumanReadableString(MemGetHeapMemory() - lPerformanceInitialHeapMemory) << "\t";
		cout << nTotalTokenNumber << "\t";
		cout << sLabel << endl;
	}
}

const ALString KDTextTokenSampleCollectionTask::GetTaskName() const
{
	return "Text token sample collection";
}

PLParallelTask* KDTextTokenSampleCollectionTask::Create() const
{
	return new KDTextTokenSampleCollectionTask;
}

boolean KDTextTokenSampleCollectionTask::ComputeResourceRequirements()
{
	boolean bOk;

	// Appele de la methode ancetre
	bOk = KWDatabaseTask::ComputeResourceRequirements();
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::MasterInitialize()
{
	boolean bOk;

	// Appele de la methode ancetre
	bOk = KWDatabaseTask::MasterInitialize();
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;

	// Appele de la methode ancetre
	bOk = KWDatabaseTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::MasterAggregateResults()
{
	boolean bOk;
	boolean bDisplay = true;
	ObjectArray* oaCollectedTokens;
	KWClass* kwcClass;
	int nAttribute;

	// Affichage
	if (bDisplay)
	{
		cout << GetTaskLabel() << "\t";
		if (shared_bIsFirstPass)
			cout << "First pass\n";
		else
			cout << "Second pass\n";
		cout << "MasterAggregateResults\t" << GetTaskIndex() << "\n";
	}

	// Appele de la methode ancetre
	bOk = KWDatabaseTask::MasterAggregateResults();

	// Traitement des resultats
	if (bOk)
	{
		// Recherche du dictionnaire de la base
		kwcClass =
		    KWClassDomain::GetCurrentDomain()->LookupClass(shared_sourceDatabase.GetDatabase()->GetClassName());

		// Collecte des tokens en sortie de l'esclave
		assert(kwcClass->GetUsedAttributeNumber() == output_oaTokens->GetObjectArray()->GetSize());
		for (nAttribute = 0; nAttribute < output_oaTokens->GetObjectArray()->GetSize(); nAttribute++)
		{
			oaCollectedTokens = cast(ObjectArray*, output_oaTokens->GetObjectArray()->GetAt(nAttribute));

			// Prise en compte des tokens de l'esclave
			// NOT YET IMPLEMENTED

			// Affichage
			if (bDisplay)
			{
				cout << kwcClass->GetUsedAttributeAt(nAttribute)->GetName() << "\t"
				     << oaCollectedTokens->GetSize() << "\n";
				KWTextTokenizer::DisplayHeadTokenArray(oaCollectedTokens, 10, cout);
			}

			// Nettoyage
			oaCollectedTokens->DeleteAll();
		}
		output_oaTokens->GetObjectArray()->DeleteAll();
	}
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appele de la methode ancetre
	bOk = KWDatabaseTask::MasterFinalize(bProcessEndedCorrectly);
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::SlaveInitialize()
{
	boolean bOk;
	KWTextTokenizer* textTokenizer;
	KWClass* kwcClass;
	int nAttribute;

	require(CheckPassParameters());

	// Appele de la methode ancetre
	bOk = KWDatabaseTask::SlaveInitialize();

	// Initialisation des tokenizers utilises en variables de travail
	if (bOk)
	{
		// Recherche du dictionnaire de la base
		kwcClass =
		    KWClassDomain::GetCurrentDomain()->LookupClass(shared_sourceDatabase.GetDatabase()->GetClassName());

		// Initialisation des tokenisers
		oaSlaveTextTokenizers.SetSize(kwcClass->GetUsedAttributeNumber());
		for (nAttribute = 0; nAttribute < oaSlaveTextTokenizers.GetSize(); nAttribute++)
		{
			// Creation d'un tokenizer selon le type de features de texte
			textTokenizer = KWTextTokenizer::CreateTextTokenizer(GetTextFeatures());

			// Parametrage de la collection en mode flux en specifiant nombre max de tokens a conserver pour
			// assurer la stabilite des resultats
			if (shared_bIsFirstPass)
				textTokenizer->SetMaxCollectedTokenNumber(
				    shared_ivFirstPassTokenNumbers.GetAt(nAttribute));
			// Parametrage des tokens specifique dont il faut calculer l'effectif
			else
			{
				// NOT YET IMPLEMENTED
			}

			// Memorisation du tokenizer associer a l'attribut
			oaSlaveTextTokenizers.SetAt(nAttribute, textTokenizer);
		}
	}
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::SlaveProcess()
{
	boolean bOk;
	KWTextTokenizer* textTokenizer;
	ObjectArray* oaCollectedTokens;
	int nAttribute;

	// Appele de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcess();

	// Collecte des tokens a renvoyer par l'esclave
	if (bOk)
	{
		// Parcours des tokeniser pour collecter les tokens
		output_oaTokens->GetObjectArray()->SetSize(oaSlaveTextTokenizers.GetSize());
		for (nAttribute = 0; nAttribute < oaSlaveTextTokenizers.GetSize(); nAttribute++)
		{
			textTokenizer = cast(KWTextTokenizer*, oaSlaveTextTokenizers.GetAt(nAttribute));

			// Export des tokens collectes
			oaCollectedTokens = new ObjectArray;
			output_oaTokens->GetObjectArray()->SetAt(nAttribute, oaCollectedTokens);
			textTokenizer->ExportTokens(oaCollectedTokens);
			textTokenizer->CleanCollectedTokens();
		}
	}
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	const KWClass* mainClass;
	KWAttribute* attribute;
	KWTextTokenizer* textTokenizer;
	int nAttribute;

	require(kwoObject != NULL);
	require(oaSlaveTextTokenizers.GetSize() == kwoObject->GetClass()->GetUsedAttributeNumber());

	// Acces a la classe de l'objet
	mainClass = kwoObject->GetClass();

	// Anayse de l'objet par parcours de ses attributs
	for (nAttribute = 0; nAttribute < mainClass->GetUsedAttributeNumber(); nAttribute++)
	{
		attribute = mainClass->GetUsedAttributeAt(nAttribute);
		textTokenizer = cast(KWTextTokenizer*, oaSlaveTextTokenizers.GetAt(nAttribute));
		check(textTokenizer);

		// Analyse de la valeur selon son type
		assert(KWType::IsTextBased(attribute->GetType()));
		if (attribute->GetType() == KWType::Text)
			textTokenizer->TokenizeSymbol(kwoObject->GetTextValueAt(attribute->GetLoadIndex()));
		else
			textTokenizer->TokenizeSymbolVector(kwoObject->GetTextListValueAt(attribute->GetLoadIndex()));
	}
	return true;
}

boolean KDTextTokenSampleCollectionTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appele de la methode ancetre
	bOk = KWDatabaseTask::SlaveFinalize(bProcessEndedCorrectly);

	// Nettoyage
	oaSlaveTextTokenizers.DeleteAll();
	return bOk;
}
