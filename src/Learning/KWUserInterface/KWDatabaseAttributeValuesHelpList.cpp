// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseAttributeValuesHelpList.h"

KWDatabaseAttributeValuesHelpList::KWDatabaseAttributeValuesHelpList()
{
	lHelpListLastClassHashValue = 0;
	lHelpListLastClassDomainHashValue = 0;
}

KWDatabaseAttributeValuesHelpList::~KWDatabaseAttributeValuesHelpList()
{
	oaHelpListDatabaseUsedFileSpecs.DeleteAll();
}

void KWDatabaseAttributeValuesHelpList::FillAttributeValues(const KWDatabase* inputDatabase,
							    const ALString& sAttributeName,
							    boolean bUseSelectionAttributes, boolean bFillIfContinuous,
							    boolean bFillIfSymbol, UIList* uilAttributeValueHelpList,
							    const ALString& sListAttributeFieldName)
{
	boolean bOk = true;
	KWClass* kwcClass;
	const int nMaxTargetModalityNumber = 100;
	const double dMaxComputeTime = 0.75;
	boolean bInitialTaskProgressionSilentMode;
	double dInitialTaskProgressionMaxTaskTime;
	KWAttribute* helpAttribute;
	KWLoadIndex liHelpAttributeLoadIndex;
	boolean bIsHelpAttributeUsed;
	boolean bIsHelpAttributeContinuous;
	KWObject* kwoObject;
	longint lRecordNumber;
	Symbol sHelpValue;
	Continuous cHelpValue;
	boolean bUnfinishedAnalysis;
	NumericKeyDictionary nkdHelpValues;
	SymbolVector svHelpValues;
	ContinuousVector cvHelpValues;
	int nValue;
	KWDatabase* helpDatabase;
	boolean bGlobalSilentMode;

	require(inputDatabase != NULL);
	require(uilAttributeValueHelpList != NULL);
	require(uilAttributeValueHelpList->GetFieldIndex(sListAttributeFieldName) != -1);

	// En principe, on doit etre hors gestion des taches
	assert(not TaskProgression::IsStarted());
	if (TaskProgression::IsStarted())
		return;

	// Arret s'il n'est pas necessaire de rafraichir la liste d'aide
	if (not NeedsRefresh(inputDatabase, sAttributeName, bUseSelectionAttributes))
		return;

	// On va utiliser la gestion des taches en mode silencieux pour
	// limiter le temps de calcul (en supplement de ce qui est fait localement)
	// On doit ne peut pas se contner d'un timer, car la lecture d'une base est un processus
	// complexe, avec des tables externes, des objets complexes et longs a lire et traiter.
	// Seule la gestion des tache permet a ce processu d'etre interrompu a tous moments,
	// ce qui est necessaire pour limiter le temps de gestion de la liste d'aide
	assert(not TaskProgression::IsStarted());
	bInitialTaskProgressionSilentMode = TaskProgression::GetSilentMode();
	dInitialTaskProgressionMaxTaskTime = TaskProgression::GetMaxTaskTime();
	TaskProgression::SetSilentMode(true);
	TaskProgression::SetMaxTaskTime(dMaxComputeTime);
	TaskProgression::Start();
	TaskProgression::BeginTask();

	// On commence par la vider
	uilAttributeValueHelpList->RemoveAllItems();

	// Test de validite de la classe
	kwcClass = NULL;
	if (bOk)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(inputDatabase->GetClassName());
		bOk = kwcClass != NULL and kwcClass->IsCompiled();
		assert(not bOk or kwcClass->GetUsedAttributeNumber() == kwcClass->GetLoadedAttributeNumber());
	}

	// On teste la validite de l'attribut
	helpAttribute = NULL;
	if (bOk)
	{
		helpAttribute = kwcClass->LookupAttribute(sAttributeName);
		bOk = helpAttribute != NULL and KWType::IsSimple(helpAttribute->GetType());
	}

	// Test de validite de la base de d'apprentissage (au moins pour la table principale
	if (bOk)
	{
		bOk = inputDatabase->GetDatabaseName() != "" and
		      PLRemoteFileService::FileExists(inputDatabase->GetDatabaseName());
	}

	// Si tout est OK, on va faire une passe de lecture sur la base pour
	// alimenter la liste des valeurs cibles si le type est adequat
	bUnfinishedAnalysis = false;
	if (bOk and ((helpAttribute->GetType() == KWType::Continuous and bFillIfContinuous) or
		     (helpAttribute->GetType() == KWType::Symbol and bFillIfSymbol)))
	{
		assert(kwcClass != NULL);
		assert(helpAttribute != NULL);

		// Type de l'attribut
		assert(KWType::IsSimple(helpAttribute->GetType()));
		bIsHelpAttributeContinuous = helpAttribute->GetType() == KWType::Continuous;

		// Preparation de la classe: tous les attributs passent en "not Loaded",
		// sauf l'attribut cible
		assert(kwcClass->GetUsedAttributeNumber() == kwcClass->GetLoadedAttributeNumber());
		bIsHelpAttributeUsed = helpAttribute->GetUsed();
		kwcClass->SetAllAttributesLoaded(false);
		helpAttribute->SetUsed(true);
		helpAttribute->SetLoaded(true);

		// On ne recompile que la classe (pas d'impact sur le reste du domaine)
		kwcClass->Compile();
		assert(kwcClass->GetLoadedAttributeNumber() == 1);

		// Acces a l'index de chargement de l'attribut d'aide
		liHelpAttributeLoadIndex = helpAttribute->GetLoadIndex();
		assert(liHelpAttributeLoadIndex.IsValid());

		// Creation d'une base pour la lecture des valeurs
		helpDatabase = inputDatabase->Clone();

		// Utilisation ou non de l'attribut de selection
		if (not bUseSelectionAttributes)
		{
			helpDatabase->SetSelectionAttribute("");
			helpDatabase->SetSelectionValue("");
		}

		// Passage en mode silencieux
		bGlobalSilentMode = Global::GetSilentMode();
		Global::SetSilentMode(true);
		helpDatabase->SetSilentMode(true);

		// Test si base ouvrable
		bOk = kwcClass->IsCompiled() and helpDatabase->CheckPartially(false);

		// Arret si trop de temps passe (en compilation de la classe notamment)
		if (TaskProgression::IsInterruptionRequested())
		{
			bUnfinishedAnalysis = true;
			bOk = false;
		}

		// Ouverture de la base si OK
		if (bOk)
			bOk = helpDatabase->OpenForRead();

		// Parcours de la base en sequentiel pour optimiser la memoire
		if (bOk)
		{
			// Boucle d'analyse des enregistrements
			lRecordNumber = 0;
			while (not helpDatabase->IsEnd() and not helpDatabase->IsError())
			{
				// Arret si trop de temps passe
				// Tests frequents pour s'arreter des que le temps imparti est depasse
				if (lRecordNumber % 100 == 0 and TaskProgression::IsInterruptionRequested())
				{
					bUnfinishedAnalysis = not helpDatabase->IsEnd();
					break;
				}

				// Lecture d'un enregistrement
				kwoObject = helpDatabase->Read();
				lRecordNumber++;

				// Analyse de l'enregistrement
				if (kwoObject != NULL)
				{
					// Traitement de la valeur dans le cas continu
					if (bIsHelpAttributeContinuous)
					{
						// Lecture de la valeur
						cHelpValue = kwoObject->GetContinuousValueAt(liHelpAttributeLoadIndex);

						// Transformation en symbol
						sHelpValue = Symbol(KWContinuous::ContinuousToString(cHelpValue));

						// Ajout de la modalite cible un dictionnaire pour en controler
						// l'unicite
						if (nkdHelpValues.Lookup(sHelpValue.GetNumericKey()) == NULL)
						{
							nkdHelpValues.SetAt(sHelpValue.GetNumericKey(), &nkdHelpValues);

							// Memorisation dans un vecteur de valeurs symbol pour les
							// detruire ensuite
							svHelpValues.Add(sHelpValue);

							// Memorisation dans un vecteur de valeurs pour les trier
							// ensuite
							cvHelpValues.Add(cHelpValue);
						}
					}
					// Traitement de la valeur dans le cas symbolique
					else
					{
						// Lecture de la valeur
						sHelpValue = kwoObject->GetSymbolValueAt(liHelpAttributeLoadIndex);

						// Ajout de la modalite cible un dictionnaire pour en controler
						// l'unicite
						if (nkdHelpValues.Lookup(sHelpValue.GetNumericKey()) == NULL)
						{
							nkdHelpValues.SetAt(sHelpValue.GetNumericKey(), &nkdHelpValues);

							// Memorisation dans un vecteur de valeurs pour les trier
							// ensuite
							svHelpValues.Add(sHelpValue);
						}
					}

					// Destruction de l'objet
					delete kwoObject;
				}

				// Arret si trop de modalites
				if (nkdHelpValues.GetCount() >= nMaxTargetModalityNumber)
				{
					bUnfinishedAnalysis = not helpDatabase->IsEnd();
					break;
				}
			}

			// Fermeture de la base
			helpDatabase->Close();
		}
		helpDatabase->SetSilentMode(false);
		Global::SetSilentMode(bGlobalSilentMode);

		// Destruction de la base, desormais inutile
		delete helpDatabase;

		// Tri par valeur
		if (bIsHelpAttributeContinuous)
			cvHelpValues.Sort();
		else
			svHelpValues.SortValues();

		// Ajout des modalites dans la liste d'aide, apres une premiere modalite vide
		uilAttributeValueHelpList->AddItem();
		uilAttributeValueHelpList->SetStringValueAt(sListAttributeFieldName, "");
		if (bIsHelpAttributeContinuous)
		{
			for (nValue = 0; nValue < cvHelpValues.GetSize(); nValue++)
			{
				// Ajout d'un item dans la liste d'aide
				cHelpValue = cvHelpValues.GetAt(nValue);
				if (cHelpValue != KWContinuous::GetMissingValue())
				{
					uilAttributeValueHelpList->AddItem();
					uilAttributeValueHelpList->SetStringValueAt(
					    sListAttributeFieldName, KWContinuous::ContinuousToString(cHelpValue));
				}
			}
		}
		// Cas symbolique
		else
		{
			for (nValue = 0; nValue < svHelpValues.GetSize(); nValue++)
			{
				// Ajout d'un item dans la liste d'aide
				sHelpValue = svHelpValues.GetAt(nValue);
				if (not sHelpValue.IsEmpty())
				{
					uilAttributeValueHelpList->AddItem();
					uilAttributeValueHelpList->SetStringValueAt(sListAttributeFieldName,
										    sHelpValue.GetValue());
				}
			}
		}

		// Ajout si necessaire d'une modalite "..." si tout n'a pas ete analyse
		if (bUnfinishedAnalysis)
		{
			uilAttributeValueHelpList->AddItem();
			uilAttributeValueHelpList->SetStringValueAt(sListAttributeFieldName, "...");
		}

		// On restitue la classe dans son etat initial,
		// avec tous les attributs utilises en Loaded
		helpAttribute->SetUsed(bIsHelpAttributeUsed);
		kwcClass->SetAllAttributesLoaded(true);
		kwcClass->Compile();
		assert(kwcClass->GetUsedAttributeNumber() == kwcClass->GetLoadedAttributeNumber());
	}

	// On restitue l'etat initial du TaskManager
	TaskProgression::EndTask();
	TaskProgression::Stop();
	TaskProgression::SetSilentMode(bInitialTaskProgressionSilentMode);
	TaskProgression::SetMaxTaskTime(dInitialTaskProgressionMaxTaskTime);

	// Memorisation de la fraicheur de la liste d'aide
	UpdateFreshness(inputDatabase, sAttributeName, bUseSelectionAttributes);
}

boolean KWDatabaseAttributeValuesHelpList::NeedsRefresh(const KWDatabase* inputDatabase, const ALString& sAttributeName,
							boolean bUseSelectionAttributes) const
{
	boolean bNeedsRefresh;
	KWClass* kwcClass;
	ObjectArray oaInputDatabaseUsedFileSpecs;
	int nFile;
	FileSpec* inputFileSpec;
	FileSpec* refFileSpec;
	longint lClassDomainHashValue;
	longint lClassHashValue;

	require(inputDatabase != NULL);

	// En mode batch, les liste d'aide sont inutiles
	if (UIObject::IsBatchMode())
		return false;

	// Test sur l'attribut d'aide
	bNeedsRefresh = false;
	if (sHelpListLastAttributeName != sAttributeName)
		bNeedsRefresh = true;

	// Test sur les caracteristiques communes a toute la base
	if (not bNeedsRefresh)
	{
		if (dbHelpListLastDatabaseSpec.GetClassName() != inputDatabase->GetClassName() or
		    dbHelpListLastDatabaseSpec.GetDatabaseName() != inputDatabase->GetDatabaseName() or
		    dbHelpListLastDatabaseSpec.GetSampleNumberPercentage() !=
			inputDatabase->GetSampleNumberPercentage() or
		    dbHelpListLastDatabaseSpec.GetModeExcludeSample() != inputDatabase->GetModeExcludeSample() or
		    dbHelpListLastDatabaseSpec.GetSelectionAttribute() != inputDatabase->GetSelectionAttribute())
			bNeedsRefresh = true;
	}

	// Prise en compte de l'attribut de selection uniquement si necessaire
	if (not bNeedsRefresh)
	{
		if (bUseSelectionAttributes and
		    dbHelpListLastDatabaseSpec.GetSelectionValue() != inputDatabase->GetSelectionValue())
			bNeedsRefresh = true;
	}

	// Tests specifique au multi-tables
	if (not bNeedsRefresh and oaHelpListDatabaseUsedFileSpecs.GetSize() != inputDatabase->GetTableNumber())
		bNeedsRefresh = true;
	if (not bNeedsRefresh)
	{
		// Comparaison de la liste des fichier de composition
		inputDatabase->ExportUsedFileSpecs(&oaInputDatabaseUsedFileSpecs);
		for (nFile = 0; nFile < oaInputDatabaseUsedFileSpecs.GetSize(); nFile++)
		{
			inputFileSpec = cast(FileSpec*, oaInputDatabaseUsedFileSpecs.GetAt(nFile));
			refFileSpec = cast(FileSpec*, oaHelpListDatabaseUsedFileSpecs.GetAt(nFile));
			if (inputFileSpec->GetFilePathName() != refFileSpec->GetFilePathName())
			{
				bNeedsRefresh = true;
				break;
			}
		}
		oaInputDatabaseUsedFileSpecs.DeleteAll();
	}

	// Comparaison de la valeur de hash de la classe
	if (not bNeedsRefresh)
	{
		// Calcul de la valeur de hashage de la classe
		lClassHashValue = 0;
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(inputDatabase->GetClassName());
		if (kwcClass != NULL)
			lClassHashValue = kwcClass->ComputeHashValue();

		// Recalcul necessaire si la fraicheur a change
		if (lClassHashValue != lHelpListLastClassHashValue)
			bNeedsRefresh = true;
	}

	// Comparaison de la valeur de hash du domaine
	if (not bNeedsRefresh)
	{
		lClassDomainHashValue = KWClassDomain::GetCurrentDomain()->ComputeHashValue();
		if (lClassDomainHashValue != lHelpListLastClassDomainHashValue)
			bNeedsRefresh = true;
	}
	return bNeedsRefresh;
}

void KWDatabaseAttributeValuesHelpList::UpdateFreshness(const KWDatabase* inputDatabase, const ALString& sAttributeName,
							boolean bUseSelectionAttributes)
{
	KWClass* kwcClass;
	longint lClassDomainHashValue;
	longint lClassHashValue;

	// Memorisation des caracteristiques conduisant a un nouveau calcul
	sHelpListLastAttributeName = sAttributeName;
	dbHelpListLastDatabaseSpec.SetClassName(inputDatabase->GetClassName());
	dbHelpListLastDatabaseSpec.SetDatabaseName(inputDatabase->GetDatabaseName());
	dbHelpListLastDatabaseSpec.SetSampleNumberPercentage(inputDatabase->GetSampleNumberPercentage());
	dbHelpListLastDatabaseSpec.SetModeExcludeSample(inputDatabase->GetModeExcludeSample());
	dbHelpListLastDatabaseSpec.SetSelectionAttribute(inputDatabase->GetSelectionAttribute());

	// Prise en compte de l'attribut de selection uniquement si necessaire
	if (bUseSelectionAttributes)
		dbHelpListLastDatabaseSpec.SetSelectionValue(inputDatabase->GetSelectionValue());

	// Memorisation des caracteristiques multi-tables
	oaHelpListDatabaseUsedFileSpecs.DeleteAll();
	inputDatabase->ExportUsedFileSpecs(&oaHelpListDatabaseUsedFileSpecs);

	// Memorisation de la valeur de hashage de la classe
	lClassHashValue = 0;
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(inputDatabase->GetClassName());
	if (kwcClass != NULL)
		lClassHashValue = kwcClass->ComputeHashValue();
	lHelpListLastClassHashValue = lClassHashValue;

	// Memorisation de la valeur de hashage du domaine
	lClassDomainHashValue = KWClassDomain::GetCurrentDomain()->ComputeHashValue();
	lHelpListLastClassDomainHashValue = lClassDomainHashValue;
	ensure(not NeedsRefresh(inputDatabase, sAttributeName, bUseSelectionAttributes));
}
