// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDTextTokenSampleCollectionTask.h"

KDTextTokenSampleCollectionTask::KDTextTokenSampleCollectionTask()
{
	ivMasterTokenNumbers = NULL;
	oaMasterCollectedTokenSamples = NULL;
}

KDTextTokenSampleCollectionTask::~KDTextTokenSampleCollectionTask() {}

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
	KDTokenFrequency* token;

	require(sourceDatabase != NULL);
	require(ivTokenNumbers != NULL);
	require(oaCollectedTokenSamples != NULL);

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
	// bOk = PROTOVirtualCollectTokenSamples(sourceDatabase);
	bOk = PROTOCollectTokenSamples(sourceDatabase);

	// Lancement de la tache
	// bOk = RunDatabaseTask(sourceDatabase);

	// Reinitialisation des parametres principaux
	ivMasterTokenNumbers = NULL;
	oaMasterCollectedTokenSamples = NULL;

	// Affichage
	if (bDisplay)
	{
		cout << "CollectTokenSamples\t" << kwcMainClass->GetName() << "\t" << BooleanToString(bOk) << "\n";
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
				token = cast(KDTokenFrequency*, oaTokens->GetAt(nToken));
				if (nToken > nMaxDisplayedTokenNumber)
				{
					cout << "\t\t...\n";
					break;
				}
				cout << "\t\t" << nToken + 1 << "\t" << token->GetFrequency() << "\t"
				     << token->GetToken() << "\n";
			}
			cout << flush;
		}
	}
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::PROTOVirtualCollectTokenSamples(const KWDatabase* sourceDatabase)
{
	boolean bOk = true;
	KWClass* kwcMainClass;
	int nAttribute;
	KWAttribute* attribute;
	int nTokenNumber;
	int nToken;
	ObjectArray* oaTokens;
	KDTokenFrequency* token;
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
			token = new KDTokenFrequency;
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

boolean KDTextTokenSampleCollectionTask::PROTOCollectTokenSamples(const KWDatabase* sourceDatabase)
{
	boolean bOk = true;
	boolean bDisplay = false;
	KWDatabase* database;
	PeriodicTest periodicTestInterruption;
	int nAttribute;
	KWObject* kwoObject;
	ObjectArray oaPhysicalMessages;
	longint lObjectNumber;
	longint lRecordNumber;
	ObjectArray oaTokenDictionaries;
	int nToken;
	ObjectDictionary* odTokens;
	ObjectArray* oaTokens;
	ALString sTmp;

	require(sourceDatabase != NULL);
	require(not sourceDatabase->IsOpenedForRead());
	require(not sourceDatabase->IsOpenedForWrite());
	require(ivMasterTokenNumbers != NULL);
	require(oaMasterCollectedTokenSamples != NULL);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialisation

	// Initialisation des dictionnaires de tokesn utilises en variables de travail
	oaTokenDictionaries.SetSize(oaMasterCollectedTokenSamples->GetSize());
	for (nAttribute = 0; nAttribute < oaMasterCollectedTokenSamples->GetSize(); nAttribute++)
		oaTokenDictionaries.SetAt(nAttribute, new ObjectDictionary);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Analyse de la base

	// Cast en database, ce qui ne sera pas necessaire avec la version standard de l'implementation avec les
	// methdoes de la tache
	database = cast(KWDatabase*, sourceDatabase);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Analyze database text variables " + sourceDatabase->GetDatabaseName());

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
				PROTOAanalyseDatabaseObject(kwoObject, &oaTokenDictionaries);

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
			if (periodicTestInterruption.IsTestAllowed(lRecordNumber))
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

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Finalisation

	// Collecte des resultats des dictionnaires de tokens dans les tableaux de tokens en sortie
	for (nAttribute = 0; nAttribute < oaMasterCollectedTokenSamples->GetSize(); nAttribute++)
	{
		odTokens = cast(ObjectDictionary*, oaTokenDictionaries.GetAt(nAttribute));
		oaTokens = cast(ObjectArray*, oaMasterCollectedTokenSamples->GetAt(nAttribute));

		// Affichage
		if (bDisplay)
		{
			if (nAttribute == 0)
				cout << "PROTOCollectTokenSamples\t" << database->GetDatabaseName() << "\n";
			cout << "\tVariable" << nAttribute << "\t" << ivMasterTokenNumbers->GetAt(nAttribute) << "\t"
			     << odTokens->GetCount() << "\n";
		}

		// Export des tokens vers le tableau
		odTokens->ExportObjectArray(oaTokens);

		// Tri des tokens par effectif decroissant
		oaTokens->SetCompareFunction(KDTokenFrequencyCompareFrequency);
		oaTokens->Sort();

		// Suppression des eventuels tokens surnumeraires
		if (oaTokens->GetSize() > ivMasterTokenNumbers->GetAt(nAttribute))
		{
			for (nToken = ivMasterTokenNumbers->GetAt(nAttribute); nToken < oaTokens->GetSize(); nToken++)
				delete oaTokens->GetAt(nToken);
			oaTokens->SetSize(ivMasterTokenNumbers->GetAt(nAttribute));
		}

		// Destruction du dictionnaire de travail
		delete odTokens;
	}
	return bOk;
}

boolean KDTextTokenSampleCollectionTask::PROTOAanalyseDatabaseObject(const KWObject* kwoObject,
								     ObjectArray* oaTokenDictionaries)
{
	const KWClass* mainClass;
	KWAttribute* attribute;
	int nAttribute;

	require(kwoObject != NULL);
	require(oaTokenDictionaries != NULL);

	// Acces a la classe de l'objet
	mainClass = kwoObject->GetClass();
	assert(mainClass->GetUsedAttributeNumber() == oaTokenDictionaries->GetSize());

	// Anayse de l'objet par parcours de ses attributs
	for (nAttribute = 0; nAttribute < mainClass->GetUsedAttributeNumber(); nAttribute++)
	{
		attribute = mainClass->GetUsedAttributeAt(nAttribute),

		// Analyse de la valeur selon son type
		    assert(KWType::IsTextBased(attribute->GetType()));
		if (attribute->GetType() == KWType::Text)
			PROTOAnalyseTextValue(kwoObject->GetTextValueAt(attribute->GetLoadIndex()),
					      cast(ObjectDictionary*, oaTokenDictionaries->GetAt(nAttribute)));
		else
			PROTOAanalyseTextListValue(kwoObject->GetTextListValueAt(attribute->GetLoadIndex()),
						   cast(ObjectDictionary*, oaTokenDictionaries->GetAt(nAttribute)));
	}
	return true;
}

void KDTextTokenSampleCollectionTask::PROTOAnalyseTextValue(const Symbol& sTextValue,
							    ObjectDictionary* odTokenDictionary)
{
	return PROTOAnalyseTextValueUsingPunctuation(sTextValue, odTokenDictionary);
}

void KDTextTokenSampleCollectionTask::PROTOAnalyseTextValueBasic(const Symbol& sTextValue,
								 ObjectDictionary* odTokenDictionary)
{
	boolean bDisplay = false;
	const int nMaxTokenLength = 20;
	const char* sStringValue;
	char cChar;
	ALString sToken;
	KDTokenFrequency* token;
	boolean bEnd;
	debug(int nCheckedTotalSpaceCharNumber = 0);
	debug(int nCheckedTotalTokenCharNumber = 0);

	require(odTokenDictionary != NULL);

	// Acces a la chaine de caractere a analyser
	sStringValue = sTextValue.GetValue();
	if (bDisplay)
		cout << sStringValue << "\n";

	// Parcours des caracteres de la chaines pour extraire les tokens
	bEnd = false;
	while (not bEnd)
	{
		// Acces au caractere courant
		cChar = *sStringValue;

		// Preparation du caractere suivant
		bEnd = (cChar == '\0');
		sStringValue++;
		debug(nCheckedTotalSpaceCharNumber += iswspace(cChar) ? 1 : 0);

		// Ajout d'un caractere s'il n'est pas blanc
		if (not iswspace(cChar) and not bEnd)
			sToken += cChar;
		// Sinon, fin du token
		else
		{
			// On traite les token non vides
			if (sToken.GetLength() > 0)
			{
				debug(nCheckedTotalTokenCharNumber += sToken.GetLength());

				// On ignore les tokens trop longs, peu interpretable, couteux a stocker, et
				// probablement rares de toutes facon
				if (sToken.GetLength() <= nMaxTokenLength)
				{
					// Creation si necessaire du token dans le dictionnaire
					token = cast(KDTokenFrequency*, odTokenDictionary->Lookup(sToken));
					if (token == NULL)
					{
						token = new KDTokenFrequency;
						token->SetToken(sToken);
						odTokenDictionary->SetAt(sToken, token);
					}
					debug(assert(token->GetSpaceCharNumber() == 0));

					// Incrementation de son effectif
					token->IncrementFrequency();
					if (bDisplay)
						cout << "\t" << token->GetFrequency() << "\t" << token->GetToken()
						     << "\n";
				}

				// Reinitialisation du token, sans desallouer sa memoire
				sToken.GetBufferSetLength(0);
			}
		}
	}
	debug(assert(sTextValue.GetLength() == nCheckedTotalSpaceCharNumber + nCheckedTotalTokenCharNumber));
}

void KDTextTokenSampleCollectionTask::PROTOAnalyseTextValueUsingPunctuation(const Symbol& sTextValue,
									    ObjectDictionary* odTokenDictionary)
{
	boolean bDisplay = false;
	const int nMaxTokenLength = 20;
	const char* sStringValue;
	char cChar;
	boolean bIsTokenPunctuation;
	boolean bIsSpace;
	boolean bIsPunctuation;
	boolean bEndToken;
	boolean bEnd;
	ALString sToken;
	KDTokenFrequency* token;
	debug(int nCheckedTotalSpaceCharNumber = 0);
	debug(int nCheckedTotalTokenCharNumber = 0);

	require(odTokenDictionary != NULL);

	// Acces a la chaine de caractere a analyser
	sStringValue = sTextValue.GetValue();
	if (bDisplay)
		cout << sStringValue << "\n";

	// Parcours des caracteres de la chaines pour extraire les tokens
	bEnd = false;
	bIsTokenPunctuation = false;
	bEndToken = false;
	while (not bEnd)
	{
		// Acces au caractere courant
		cChar = *sStringValue;
		bIsSpace = iswspace(cChar);
		bIsPunctuation = ispunct(cChar);
		debug(nCheckedTotalSpaceCharNumber += bIsSpace ? 1 : 0);

		// Preparation du caractere suivant
		bEnd = (cChar == '\0');
		sStringValue++;

		// Test si fin de token
		if (bEnd or bIsSpace)
			bEndToken = true;
		else if (sToken.GetLength() > 0 and bIsTokenPunctuation != bIsPunctuation)
			bEndToken = true;

		// Ajout d'un caractere si on est pas en fin de token
		if (not bEndToken)
		{
			if (sToken.GetLength() == 0)
				bIsTokenPunctuation = bIsPunctuation;
			sToken += cChar;
		}
		// Sinon, fin du token
		else
		{
			// On traite les tokens non vides
			if (sToken.GetLength() > 0)
			{
				debug(nCheckedTotalTokenCharNumber += sToken.GetLength());

				// On ignore les tokens trop longs, peu interpretables, couteux a stocker, et
				// probablement rares de toute facon
				if (sToken.GetLength() <= nMaxTokenLength)
				{
					// Creation si necessaire du token dans le dictionnaire
					token = cast(KDTokenFrequency*, odTokenDictionary->Lookup(sToken));
					if (token == NULL)
					{
						token = new KDTokenFrequency;
						token->SetToken(sToken);
						odTokenDictionary->SetAt(sToken, token);
					}
					debug(assert(token->GetSpaceCharNumber() == 0));
					debug(
					    assert(token->GetPunctuationCharNumber() == 0 or
						   token->GetPunctuationCharNumber() == token->GetToken().GetLength()));

					// Incremantation de son effectif
					token->IncrementFrequency();
					if (bDisplay)
						cout << "\t" << token->GetFrequency() << "\t" << token->GetToken()
						     << "\n";
				}

				// Reinitialisation du token, sans desallouer sa memoire
				sToken.GetBufferSetLength(0);

				// Initialisation du token suivant si necessaire
				if (not bIsSpace)
				{
					sToken += cChar;
					bIsTokenPunctuation = bIsPunctuation;
				}
			}
			bEndToken = false;
		}
	}
	debug(assert(sTextValue.GetLength() == nCheckedTotalSpaceCharNumber + nCheckedTotalTokenCharNumber));
}

void KDTextTokenSampleCollectionTask::PROTOAanalyseTextListValue(const SymbolVector* svTextListValue,
								 ObjectDictionary* odTokenDictionary)
{
	int nText;

	require(odTokenDictionary != NULL);

	// Traitement des texte du vecteur s'il est non null
	if (svTextListValue)
	{
		for (nText = 0; nText < svTextListValue->GetSize(); nText++)
			PROTOAnalyseTextValue(svTextListValue->GetAt(nText), odTokenDictionary);
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
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::MasterInitialize()
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::MasterAggregateResults()
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::SlaveInitializePrepareDatabase()
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::SlaveInitializeOpenDatabase()
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::SlaveProcessStartDatabase()
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::SlaveProcessExploitDatabase()
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::SlaveProcessStopDatabase(boolean bProcessEndedCorrectly)
{
	return boolean();
}

boolean KDTextTokenSampleCollectionTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	return boolean();
}