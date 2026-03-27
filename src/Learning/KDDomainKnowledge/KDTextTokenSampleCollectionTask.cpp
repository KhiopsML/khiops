// Copyright (c) 2023-2026 Orange. All rights reserved.
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
	DeclareSharedParameter(&shared_ivFirstPassTokenNumbers);
	shared_oaSecondPassSpecificTokens = new PLShared_ObjectArrayArray(new PLShared_TokenFrequency);
	DeclareSharedParameter(shared_oaSecondPassSpecificTokens);

	// Variables en sortie des esclaves
	output_oaTokens = new PLShared_ObjectArrayArray(new PLShared_TokenFrequency);
	DeclareTaskOutput(output_oaTokens);

	// Initialisation des variables partagees
	shared_sTextFeatures.SetValue("ngrams");
	shared_bIsFirstPass = true;
}

KDTextTokenSampleCollectionTask::~KDTextTokenSampleCollectionTask()
{
	assert(oaMasterTextTokenizers.GetSize() == 0);
	assert(oaSlaveTextTokenizers.GetSize() == 0);

	delete shared_oaSecondPassSpecificTokens;
	delete output_oaTokens;
}

boolean KDTextTokenSampleCollectionTask::CollectTokenSamples(const KWDatabase* sourceDatabase,
							     const IntVector* ivTokenNumbers,
							     ObjectArray* oaCollectedTokenSamples)
{
	boolean bOk = true;
	const boolean bTrace = false;
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
	require(ivTokenNumbers->GetSize() > 0);
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

	// Lancement de la tache
	bOk = InternalCollectTokenSamples(sourceDatabase);

	// Reinitialisation des parametres principaux
	ivMasterTokenNumbers = NULL;
	oaMasterCollectedTokenSamples = NULL;

	// Affichage
	if (bTrace)
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
			token->SetFrequency((longint)nTokenNumber + 1 - nToken);
			oaTokens->Add(token);
		}
	}
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::SequentialCollectTokenSamples(const KWDatabase* sourceDatabase)
{
	boolean bOk = true;
	const boolean bTracePerformanceStats = false;
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
	StartCollectPerformanceIndicators(bTracePerformanceStats);

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

		// Memorisation du tokenizer associe a l'attribut
		oaTextTokenizers.SetAt(nAttribute, textTokenizer);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Analyse de la base: premiere passe de collecte des tokens les plus frequents

	// Cast en database, ce qui ne sera pas necessaire avec la version standard de l'implementation avec les
	// methodes de la tache
	database = cast(KWDatabase*, sourceDatabase);

	// Analyse de la base pour collecter les tokens les plus frequents
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
			if (TaskProgression::IsRefreshNecessary(lRecordNumber))
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
	KWTextTokenizer* textTokenizer;
	int nAttribute;
	ALString sTmp;
	ObjectArray* oaCollectedTokens;
	ObjectArray* oaTokens;

	require(sourceDatabase != NULL);
	require(not sourceDatabase->IsOpenedForRead());
	require(not sourceDatabase->IsOpenedForWrite());
	require(ivMasterTokenNumbers != NULL);
	require(ivMasterTokenNumbers->GetSize() > 0);
	require(oaMasterCollectedTokenSamples != NULL);
	require(oaMasterCollectedTokenSamples->GetSize() == ivMasterTokenNumbers->GetSize());

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Premiere passe de collecte d'un echantillon de tokens

	// Initialisation des parametres
	shared_bIsFirstPass = true;
	shared_ivFirstPassTokenNumbers.GetIntVector()->SetSize(ivMasterTokenNumbers->GetSize());
	oaMasterTextTokenizers.SetSize(oaMasterCollectedTokenSamples->GetSize());
	for (nAttribute = 0; nAttribute < ivMasterTokenNumbers->GetSize(); nAttribute++)
	{
		// Parametrage de la collection en mode flux en specifiant nombre max de tokens a conserver pour assurer
		// la stabilite des resultats
		shared_ivFirstPassTokenNumbers.SetAt(
		    nAttribute, GetMaxStreamCollectedTokenNumber(ivMasterTokenNumbers->GetAt(nAttribute)));

		// Creation d'un tokenizer selon le type de features de texte
		textTokenizer = KWTextTokenizer::CreateTextTokenizer(GetTextFeatures());
		textTokenizer->SetMaxCollectedTokenNumber(shared_ivFirstPassTokenNumbers.GetAt(nAttribute));

		// Memorisation du tokenizer associe a l'attribut
		oaMasterTextTokenizers.SetAt(nAttribute, textTokenizer);
	}
	shared_oaSecondPassSpecificTokens->Clean();
	assert(CheckPassParameters());

	// Declenchement de tache pour la premiere passe
	bOk = RunDatabaseTask(sourceDatabase);

	// Parametrage des tokens specifiques pour les tokenizers des esclaves, avec les tokens collectes lors de la premiere passe
	if (bOk)
	{
		shared_oaSecondPassSpecificTokens->GetObjectArray()->SetSize(oaMasterTextTokenizers.GetSize());
		for (nAttribute = 0; nAttribute < oaMasterTextTokenizers.GetSize(); nAttribute++)
		{
			textTokenizer = cast(KWTextTokenizer*, oaMasterTextTokenizers.GetAt(nAttribute));

			// Export des tokens collectes lors de la premiere passe
			oaCollectedTokens = new ObjectArray;
			textTokenizer->ExportTokens(oaCollectedTokens);

			// Parametrage des tokens specifiques pour les tokenizers des esclaves
			shared_oaSecondPassSpecificTokens->GetObjectArray()->SetAt(nAttribute, oaCollectedTokens);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Deuxieme passe de collecte des effectifs exact par tokens

	// Collecte uniquement si necessaire
	if (bOk)
	{
		// Initialisation des parametres, en prenant en entree les tokens produits par la tache
		assert(shared_oaSecondPassSpecificTokens->GetObjectArray()->GetSize() > 0);
		shared_bIsFirstPass = false;
		shared_ivFirstPassTokenNumbers.GetIntVector()->SetSize(0);
		assert(CheckPassParameters());

		// Nettoyage des tokens collectes lors de la premiere passe, afin de ne
		// pas compter les memes tokens deux fois
		// Parametrage des tokens specifiques selon les tokens collectes lors de
		// la premiere passe
		for (nAttribute = 0; nAttribute < oaMasterTextTokenizers.GetSize(); nAttribute++)
		{
			textTokenizer = cast(KWTextTokenizer*, oaMasterTextTokenizers.GetAt(nAttribute));
			textTokenizer->CleanCollectedTokens();
			textTokenizer->SetMaxCollectedTokenNumber(0);
			textTokenizer->SetSpecificTokens(
			    cast(ObjectArray*, shared_oaSecondPassSpecificTokens->GetObjectArray()->GetAt(nAttribute)));
		}

		// Declenchement de tache pour la deuxieme passe
		bOk = RunDatabaseTask(sourceDatabase);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Finalisation

	// Collecte des resultats des dictionnaires de tokens dans les tableaux de tokens en sortie
	if (bOk)
	{
		for (nAttribute = 0; nAttribute < oaMasterCollectedTokenSamples->GetSize(); nAttribute++)
		{
			textTokenizer = cast(KWTextTokenizer*, oaMasterTextTokenizers.GetAt(nAttribute));
			oaTokens = cast(ObjectArray*, oaMasterCollectedTokenSamples->GetAt(nAttribute));
			assert(oaTokens->GetSize() == 0);

			// Export des tokens vers le tableau
			textTokenizer->ExportFrequentTokens(oaTokens, ivMasterTokenNumbers->GetAt(nAttribute));

			// Nettoyage
			delete textTokenizer;
			oaMasterTextTokenizers.SetAt(nAttribute, NULL);
		}
	}

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

void KDTextTokenSampleCollectionTask::StartCollectPerformanceIndicators(const boolean bTrace)
{
	require(not performanceTimer.IsStarted());
	require(performanceTimer.GetElapsedTime() == 0);
	require(lPerformanceInitialHeapMemory == 0);

	// Une valeur strictement positive de la memoire sert d'indicateur pur l'affichage
	if (bTrace)
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

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::ComputeResourceRequirements();
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::MasterInitialize()
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterInitialize();
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::MasterAggregateResults()
{
	boolean bOk;
	const boolean bTrace = false;
	KWTextTokenizer* textTokenizer;
	ObjectArray* oaCollectedTokens;
	KWClass* kwcClass;
	int nAttribute;

	// Affichage
	if (bTrace)
	{
		cout << GetTaskLabel() << "\t";
		if (shared_bIsFirstPass)
			cout << "First pass\n";
		else
			cout << "Second pass\n";
		cout << "MasterAggregateResults\t" << GetTaskIndex() << "\n";
	}

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterAggregateResults();

	// Traitement des resultats
	if (bOk)
	{
		assert(output_oaTokens->GetObjectArray()->GetSize() == oaMasterTextTokenizers.GetSize());

		// Recherche du dictionnaire de la base
		kwcClass =
		    KWClassDomain::GetCurrentDomain()->LookupClass(shared_sourceDatabase.GetDatabase()->GetClassName());

		// Agregation des tokens collectes dans l'esclave dans les container de token globaux du maitre
		assert(kwcClass->GetUsedAttributeNumber() == output_oaTokens->GetObjectArray()->GetSize());

		for (nAttribute = 0; nAttribute < kwcClass->GetUsedAttributeNumber(); nAttribute++)
		{
			// Cumul des frequences des tokens collectes par les esclaves
			oaCollectedTokens = cast(ObjectArray*, output_oaTokens->GetObjectArray()->GetAt(nAttribute));
			textTokenizer = cast(KWTextTokenizer*, oaMasterTextTokenizers.GetAt(nAttribute));
			textTokenizer->CumulateTokenFrequencies(oaCollectedTokens);

			// Affichage
			if (bTrace)
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

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterFinalize(bProcessEndedCorrectly);
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::SlaveInitialize()
{
	boolean bOk;
	KWTextTokenizer* textTokenizer;
	KWClass* kwcClass;
	int nAttribute;
	ObjectArray* oaSpecificTokens;

	require(CheckPassParameters());

	// Appel de la methode ancetre
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

			// Positionnement des tokens specifiques
			else
			{
				oaSpecificTokens =
				    cast(ObjectArray*,
					 shared_oaSecondPassSpecificTokens->GetObjectArray()->GetAt(nAttribute));
				textTokenizer->SetMaxCollectedTokenNumber(0);
				textTokenizer->SetSpecificTokens(oaSpecificTokens);
			}

			// Memorisation du tokenizer associe a l'attribut
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

	// Appel de la methode ancetre
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
			textTokenizer->ExportTokens(oaCollectedTokens);
			output_oaTokens->GetObjectArray()->SetAt(nAttribute, oaCollectedTokens);

			// Nettoyage du tokenizer : si l'esclave est appele plus d'une fois, alors
			// il ne cumulera pas les tokens des appels precedents
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

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveFinalize(bProcessEndedCorrectly);

	// Nettoyage
	oaSlaveTextTokenizers.DeleteAll();
	return bOk;
}
