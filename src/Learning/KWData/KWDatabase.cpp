// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabase.h"

KWDatabase::KWDatabase()
{
	kwcClass = NULL;
	kwcPhysicalClass = NULL;
	nClassFreshness = 0;
	dSampleNumberPercentage = 100;
	bModeExcludeSample = false;
	nSelectionAttributeType = KWType::Unknown;
	cSelectionContinuous = 0;
	bVerboseMode = true;
	bSilentMode = false;
	bIsError = false;
	bOpenedForRead = false;
	bOpenedForWrite = false;

	// Initialisation des formats par defaut des types speciaux
	dateDefaultConverter.SetFormatString(KWDateFormat::GetDefaultFormatString());
	timeDefaultConverter.SetFormatString(KWTimeFormat::GetDefaultFormatString());
	timestampDefaultConverter.SetFormatString(KWTimestampFormat::GetDefaultFormatString());
	timestampTZDefaultConverter.SetFormatString(KWTimestampTZFormat::GetDefaultFormatString());
}

KWDatabase::~KWDatabase()
{
	assert(not IsOpenedForRead() and not IsOpenedForWrite());
	assert(kwcPhysicalClass == NULL);
	oaAllObjects.DeleteAll();
}

KWDatabase* KWDatabase::Clone() const
{
	KWDatabase* kwdClone;
	kwdClone = Create();
	kwdClone->CopyFrom(this);
	return kwdClone;
}

KWDatabase* KWDatabase::Create() const
{
	return new KWDatabase;
}

void KWDatabase::CopyFrom(const KWDatabase* kwdSource)
{
	require(kwdSource != NULL);
	require(kwdSource != this);
	require(not IsOpenedForRead() and not IsOpenedForWrite());
	require(kwcPhysicalClass == NULL);

	// Reinitialisation prealable de toutes les variables
	kwcClass = NULL;
	kwcPhysicalClass = NULL;
	nClassFreshness = 0;
	dSampleNumberPercentage = 100;
	bModeExcludeSample = false;
	liSelectionAttributeLoadIndex.Reset();
	nSelectionAttributeType = KWType::Unknown;
	cSelectionContinuous = 0;
	bVerboseMode = true;
	bSilentMode = false;
	bIsError = false;
	bOpenedForRead = false;
	bOpenedForWrite = false;
	sClassName = "";
	sDatabaseName = "";
	sSelectionAttribute = "";
	sSelectionValue = "";
	sSelectionSymbol = "";
	ivMarkedInstances.SetSize(0);

	// Recopie des parametres de specification de la base
	SetDatabaseName(kwdSource->GetDatabaseName());
	SetClassName(kwdSource->GetClassName());
	SetSampleNumberPercentage(kwdSource->GetSampleNumberPercentage());
	SetModeExcludeSample(kwdSource->GetModeExcludeSample());
	SetSelectionAttribute(kwdSource->GetSelectionAttribute());
	SetSelectionValue(kwdSource->GetSelectionValue());
	GetMarkedInstances()->CopyFrom(&kwdSource->ivMarkedInstances);
	SetVerboseMode(kwdSource->GetVerboseMode());
	SetSilentMode(kwdSource->GetSilentMode());
}

void KWDatabase::InitializeSamplingAndSelection()
{
	require(not IsOpenedForRead() and not IsOpenedForWrite());
	require(kwcPhysicalClass == NULL);

	// Initialisation des parametres d'echantillonnage et de selection
	SetSampleNumberPercentage(100);
	SetModeExcludeSample(false);
	SetSelectionAttribute("");
	SetSelectionValue("");
}

void KWDatabase::CopySamplingAndSelectionFrom(const KWDatabase* kwdSource)
{
	require(kwdSource != NULL);
	require(kwdSource != this);
	require(not IsOpenedForRead() and not IsOpenedForWrite());
	require(kwcPhysicalClass == NULL);

	// Recopie des parametres d'echantillonnage et de selection
	SetSampleNumberPercentage(kwdSource->GetSampleNumberPercentage());
	SetModeExcludeSample(kwdSource->GetModeExcludeSample());
	SetSelectionAttribute(kwdSource->GetSelectionAttribute());
	SetSelectionValue(kwdSource->GetSelectionValue());
}

int KWDatabase::Compare(const KWDatabase* kwdSource) const
{
	int nCompare = 0;

	// Comparaison terme a terme de elements de specification
	if (nCompare == 0)
		nCompare = GetDatabaseName().Compare(kwdSource->GetDatabaseName());
	if (nCompare == 0)
		nCompare = GetClassName().Compare(kwdSource->GetClassName());
	if (nCompare == 0)
		nCompare = CompareDouble(GetSampleNumberPercentage(), kwdSource->GetSampleNumberPercentage());
	if (nCompare == 0)
		nCompare = CompareBoolean(GetModeExcludeSample(), kwdSource->GetModeExcludeSample());
	if (nCompare == 0)
		nCompare = GetSelectionAttribute().Compare(kwdSource->GetSelectionAttribute());
	if (nCompare == 0)
		nCompare = GetSelectionValue().Compare(kwdSource->GetSelectionValue());
	return nCompare;
}

void KWDatabase::SetDatabaseName(const ALString& sValue)
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	sDatabaseName = sValue;
}

const ALString& KWDatabase::GetDatabaseName() const
{
	return sDatabaseName;
}

void KWDatabase::SetClassName(const ALString& sValue)
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	sClassName = sValue;
}

const ALString& KWDatabase::GetClassName() const
{
	return sClassName;
}

void KWDatabase::AddPrefixToUsedFiles(const ALString& sPrefix)
{
	ALString sFilePrefix;
	ALString sNewDatabaseName;

	// Renommage si nom existant
	if (GetDatabaseName() != "")
	{
		// Extraction du prefixe du fichier
		sFilePrefix = FileService::GetFilePrefix(GetDatabaseName());

		// On construit le nom complet du fichier
		sNewDatabaseName = FileService::SetFilePrefix(GetDatabaseName(), sPrefix + sFilePrefix);
		SetDatabaseName(sNewDatabaseName);
	}
}

void KWDatabase::AddSuffixToUsedFiles(const ALString& sSuffix)
{
	ALString sFilePrefix;
	ALString sNewDatabaseName;

	// Renommage si nom existant
	if (GetDatabaseName() != "")
	{
		// Extraction du prefixe du fichier
		sFilePrefix = FileService::GetFilePrefix(GetDatabaseName());

		// On construit le nom complet du fichier
		sNewDatabaseName = FileService::SetFilePrefix(GetDatabaseName(), sFilePrefix + sSuffix);
		SetDatabaseName(sNewDatabaseName);
	}
}

void KWDatabase::AddPathToUsedFiles(const ALString& sPathName)
{
	ALString sNewDatabaseName;
	KWResultFilePathBuilder resultFilePathBuilder;

	if (GetDatabaseName() != "" and sPathName != "")
	{
		// On passe par le service de construction des chemins de fichier en sortie
		// avec un fichier bidon en entree pour qu'il extrait correctement la path en entree
		resultFilePathBuilder.SetInputFilePathName(FileService::BuildFilePathName(sPathName, "dummy.txt"));
		resultFilePathBuilder.SetOutputFilePathName(GetDatabaseName());
		sNewDatabaseName = resultFilePathBuilder.BuildResultFilePathName();
		SetDatabaseName(sNewDatabaseName);
	}
}

int KWDatabase::GetTableNumber() const
{
	return 1;
}

void KWDatabase::ExportUsedFileSpecs(ObjectArray* oaUsedFileSpecs) const
{
	FileSpec* fsDatabaseName;

	require(oaUsedFileSpecs != NULL);

	// Nettoyage prealable
	oaUsedFileSpecs->RemoveAll();

	// Ajout d'une specification de fichier
	fsDatabaseName = new FileSpec;
	oaUsedFileSpecs->Add(fsDatabaseName);

	// Parametrage du nom de base
	fsDatabaseName->SetLabel("database");
	fsDatabaseName->SetFilePathName(GetDatabaseName());
}

void KWDatabase::ExportUsedWriteFileSpecs(ObjectArray* oaUsedFileSpecs) const
{
	ExportUsedFileSpecs(oaUsedFileSpecs);
}

ALString KWDatabase::GetTechnologyName() const
{
	return "";
}

boolean KWDatabase::IsMultiTableTechnology() const
{
	return false;
}

KWDataTableDriver* KWDatabase::GetDataTableDriver()
{
	assert(false);
	return NULL;
}

KWClass* KWDatabase::ComputeClass()
{
	boolean bBuildClassOk;
	boolean bRefVerboseMode;
	KWClass* kwcComputedClass;
	const double dMaxTime = 20.0;
	const int nMinObjectNumber = 100;
	Timer timerComputeClass;
	KWObject* kwoObject;
	int nAttribute;
	KWLoadIndex liAttributeLoadIndex;
	Symbol sValue;
	KWAttribute* attribute;
	ObjectArray oaTypeAutomaticRecognitions;
	KWTypeAutomaticRecognition* typeAutomaticRecognition;
	IntVector ivTypeAttributeNumbers;
	int nType;
	longint lRecordNumber;
	longint lObjectNumber;
	ALString sTmp;

	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	require(kwcClass == NULL);
	require(KWClass::CheckName(GetClassName(), KWClass::Class, this));
	require(KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()) == NULL);

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Build dictionary for database " + GetDatabaseName());

	// Creation d'une classe vide
	kwcComputedClass = new KWClass;
	kwcComputedClass->SetName(GetClassName());

	// Initialisation a partir du schema de la base, en ne montrant que les erreurs
	bRefVerboseMode = GetVerboseMode();
	SetVerboseMode(false);
	bBuildClassOk = BuildDatabaseClass(kwcComputedClass);
	SetVerboseMode(bRefVerboseMode);

	// Compilation si classe correcte
	if (bBuildClassOk)
	{
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcComputedClass);
		assert(kwcComputedClass->Check());
		kwcComputedClass->Compile();
		assert(kwcComputedClass->GetAttributeNumber() == kwcComputedClass->GetLoadedAttributeNumber());
	}
	// Destruction sinon
	else
	{
		delete kwcComputedClass;
		kwcComputedClass = NULL;
	}

	// Calcul des types si necessaire
	if (bBuildClassOk and not IsTypeInitializationManaged() and kwcComputedClass->GetAttributeNumber() > 0)
	{
		// Ici, on suppose que tous les champs sont dense (pas de bloc sparse)
		assert(kwcComputedClass->GetUsedAttributeNumberForType(KWType::Symbol) ==
		       kwcComputedClass->GetAttributeNumber());

		// Ouverture de la base en lecture
		OpenForRead();

		// On n'est pas forcement dans le cas ou les classes physiques et logiques coincident:
		// une autre classe du domaine peut avoir necessite une classe physique. Mais si ce n'est pas
		// le cas, on a au moins coincidence du nombre d'attributs
		assert(kwcComputedClass == kwcPhysicalClass or
		       kwcComputedClass->GetAttributeNumber() == kwcPhysicalClass->GetAttributeNumber());

		// Creation des objets de reconnaissance de type pour chaque variable
		oaTypeAutomaticRecognitions.SetSize(kwcComputedClass->GetLoadedAttributeNumber());
		for (nAttribute = 0; nAttribute < kwcComputedClass->GetLoadedAttributeNumber(); nAttribute++)
			oaTypeAutomaticRecognitions.SetAt(nAttribute, new KWTypeAutomaticRecognition);

		// Lecture d'objets dans la base
		lRecordNumber = 0;
		lObjectNumber = 0;
		if (IsOpenedForRead())
		{
			Global::ActivateErrorFlowControl();
			timerComputeClass.Start();
			while (not IsEnd() and not IsError())
			{
				// On passe ici par la lecture physique, qui coincide avec la lecture
				// logique, mais evite les operations de selection et d'echantillonnage
				kwoObject = PhysicalRead();
				lRecordNumber++;
				if (kwoObject != NULL)
				{
					lObjectNumber++;

					// Analyse des valeurs des champs de l'objet
					for (nAttribute = 0; nAttribute < kwcComputedClass->GetLoadedAttributeNumber();
					     nAttribute++)
					{
						liAttributeLoadIndex =
						    kwcComputedClass->GetLoadedAttributeAt(nAttribute)->GetLoadIndex();
						sValue = kwoObject->GetSymbolValueAt(liAttributeLoadIndex);

						// Analyse de la valeur de l'objet
						typeAutomaticRecognition =
						    cast(KWTypeAutomaticRecognition*,
							 oaTypeAutomaticRecognitions.GetAt(nAttribute));
						typeAutomaticRecognition->AddStringValue(sValue);
					}

					// On n'a plus besoin de l'objet
					delete kwoObject;
				}

				// Suivi de la tache
				if (TaskProgression::IsRefreshNecessary())
				{
					TaskProgression::DisplayProgression((int)(100 * GetReadPercentage()));
					if (TaskProgression::IsInterruptionRequested())
						break;
				}

				// Arret si assez d'objet et si temps minimum ecoule
				if (lObjectNumber >= nMinObjectNumber and
				    timerComputeClass.GetElapsedTime() >= dMaxTime)
					break;
			}
			timerComputeClass.Stop();
			Global::DesactivateErrorFlowControl();

			// Fermeture
			Close();
		}

		// Initialisation du vecteur de nombre d'attributs par type
		ivTypeAttributeNumbers.SetSize(KWType::Unknown);

		// Determination des types des champs et mise a jour du type des champs de la classe
		// On ne reconnait que les type d'attributs denses; les bloc de variable serons reconnus comme des
		// attributs Symbol (lecture par liste car les mises a jour empechent l'acces indexe)
		attribute = kwcComputedClass->GetHeadAttribute();
		for (nAttribute = 0; nAttribute < oaTypeAutomaticRecognitions.GetSize(); nAttribute++)
		{
			check(attribute);
			assert(attribute->GetType() == KWType::Symbol);

			// Acces a la reconnaissance du type
			typeAutomaticRecognition =
			    cast(KWTypeAutomaticRecognition*, oaTypeAutomaticRecognitions.GetAt(nAttribute));

			// Finalisation de la reconnaissance de type
			typeAutomaticRecognition->Finalize();

			// Parametrage du type de l'attribut si Continu
			if (typeAutomaticRecognition->GetMainMatchingType() == KWType::Continuous)
			{
				ivTypeAttributeNumbers.UpgradeAt(KWType::Continuous, 1);
				attribute->SetType(KWType::Continuous);
				AddFormatMetaData(attribute, typeAutomaticRecognition);
			}
			// Cas du type Date
			else if (typeAutomaticRecognition->GetMainMatchingType() == KWType::Date)
			{
				ivTypeAttributeNumbers.UpgradeAt(KWType::Date, 1);
				attribute->SetType(KWType::Date);
				AddFormatMetaData(attribute, typeAutomaticRecognition);
			}
			// Cas du type Time
			else if (typeAutomaticRecognition->GetMainMatchingType() == KWType::Time)
			{
				ivTypeAttributeNumbers.UpgradeAt(KWType::Time, 1);
				attribute->SetType(KWType::Time);
				AddFormatMetaData(attribute, typeAutomaticRecognition);
			}
			// Cas du type Timestamp
			else if (typeAutomaticRecognition->GetMainMatchingType() == KWType::Timestamp)
			{
				ivTypeAttributeNumbers.UpgradeAt(KWType::Timestamp, 1);
				attribute->SetType(KWType::Timestamp);
				AddFormatMetaData(attribute, typeAutomaticRecognition);
			}
			// Cas du type TimestampTZ
			else if (typeAutomaticRecognition->GetMainMatchingType() == KWType::TimestampTZ)
			{
				ivTypeAttributeNumbers.UpgradeAt(KWType::TimestampTZ, 1);
				attribute->SetType(KWType::TimestampTZ);
				AddFormatMetaData(attribute, typeAutomaticRecognition);
			}
			// Cas du type Text
			else if (typeAutomaticRecognition->GetMainMatchingType() == KWType::Text)
			{
				ivTypeAttributeNumbers.UpgradeAt(KWType::Text, 1);
				attribute->SetType(KWType::Text);
			}
			// Par defaut: type Symbol: on se contente de memoriser le nombre d'attribut pour ce type
			else
				ivTypeAttributeNumbers.UpgradeAt(KWType::Symbol, 1);

			// Attribut suivant
			kwcComputedClass->GetNextAttribute(attribute);
		}
		kwcComputedClass->CompleteTypeInfo();
		kwcComputedClass->Compile();

		// Message synthetique sur l'ensemble des types construits
		if (kwcComputedClass->GetAttributeNumber() > 0)
		{
			boolean bFirstType = true;
			sTmp = "Recognized variable types: ";
			for (nType = 0; nType < ivTypeAttributeNumbers.GetSize(); nType++)
			{
				if (ivTypeAttributeNumbers.GetAt(nType) > 0)
				{
					if (not bFirstType)
						sTmp += ", ";
					bFirstType = false;
					sTmp += IntToString(ivTypeAttributeNumbers.GetAt(nType));
					sTmp += " " + KWType::ToString(nType);
				}
			}
			AddMessage(sTmp);
		}
	}

	// Nettoyage
	oaTypeAutomaticRecognitions.DeleteAll();

	// Fin de suivi de tache
	TaskProgression::EndTask();

	// Retour
	ensure(kwcClass == NULL);
	ensure(kwcComputedClass == NULL or kwcComputedClass->IsCompiled());
	return kwcComputedClass;
}

boolean KWDatabase::OpenForRead()
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	require(GetClassName() != "");
	require(kwcClass == NULL);
	require(kwcPhysicalClass == NULL);
	require(Check());

	// Recherche de la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	require(kwcClass != NULL);
	require(kwcClass->Check());
	nClassFreshness = kwcClass->GetFreshness();

	// Construction du dictionnaire physique
	BuildPhysicalClass();

	// Compilation des informations de selection
	CompilePhysicalSelection();

	// Ouverture physique de la base
	bIsError = false;
	assert(not bOpenedForRead);
	bOpenedForRead = PhysicalOpenForRead();

	// Installation du handler specifique pour ignorer le flow des erreur dans le cas du memory guard
	KWDatabaseMemoryGuard::InstallMemoryGuardErrorFlowIgnoreFunction();

	// Fermeture si echec
	if (not bOpenedForRead)
	{
		// On force le flag d'ouverture a true pour passer l'assertion du Close()
		bOpenedForRead = true;
		Close();
		assert(bOpenedForRead == false);
	}
	return bOpenedForRead;
}

boolean KWDatabase::OpenForWrite()
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	require(GetClassName() != "");
	require(kwcClass == NULL);
	require(kwcPhysicalClass == NULL);
	require(CheckPartially(true));

	// Recherche de la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	require(kwcClass != NULL);
	require(kwcClass->Check());
	nClassFreshness = kwcClass->GetFreshness();

	// Ouverture physique de la base
	bIsError = false;
	assert(not bOpenedForRead);
	bOpenedForWrite = PhysicalOpenForWrite();

	// Fermeture si echec
	if (not bOpenedForWrite)
	{
		// On force le flag d'ouverture a true pour passer l'assertion du Close()
		bOpenedForWrite = true;
		Close();
		assert(bOpenedForWrite == false);

		// Destruction eventuelle de ce qui a ete cree en partie pendant la tentative d'ouverture
		PhysicalDeleteDatabase();

		// Message synthetique
		AddError("Unable to open output database due to previous errors");
	}
	return bOpenedForWrite;
}

KWObject* KWDatabase::Read()
{
	KWObject* kwoObject = NULL;
	boolean bKeepRecord;
	longint lPhysicalRecordIndex;
	double dRandom;

	require(GetClassName() != "");
	require(kwcClass != NULL);
	require(kwcClass->GetFreshness() == nClassFreshness);
	require(IsOpenedForRead());
	require(kwcPhysicalClass != NULL);
	require(kwcClass->GetName() == kwcPhysicalClass->GetName());

	// Initialisation du memory guard
	memoryGuard.Init();

	// Lecture d'un objet physique valide
	if (not IsPhysicalEnd())
	{
		// Tirage d'un pourcentage aleatoire, en faisant attention a ne pas
		// interferer avec le generateur aleatoire et en utilisant un generateur
		// aleatoire indexe par l'instance courante
		// Permet de paralleliser la lecture des bases tout en preservant
		// les echantillons tires
		lPhysicalRecordIndex = GetPhysicalRecordIndex();

		// Effet de bord ou on selectionne 0%
		if (GetSampleNumberPercentage() == 0)
			bKeepRecord = GetModeExcludeSample();
		// Cas general
		else
		{
			dRandom = IthRandomDouble(lPhysicalRecordIndex) * 100;
			bKeepRecord = (dRandom <= GetSampleNumberPercentage() and not GetModeExcludeSample()) or
				      (dRandom > GetSampleNumberPercentage() and GetModeExcludeSample());
		}

		// On teste si on garde ou non l'exemple selon les
		// parametres d'echantillonnage
		if (bKeepRecord)
			kwoObject = PhysicalRead();
		// Si on ne garde pas l'exemple, on saute l'enregistrement
		else
			PhysicalSkip();

		// Test si objet selectionne
		if (kwoObject != NULL)
		{
			if (not IsPhysicalObjectSelected(kwoObject))
			{
				delete kwoObject;
				kwoObject = NULL;
			}
		}
	}
	assert(kwoObject == NULL or kwoObject->GetClass() == kwcPhysicalClass);

	// On teste en plus si l'instance est "marquee" (pour les benchmark en k folds)
	if (ivMarkedInstances.GetSize() > 0 and kwoObject != NULL)
	{
		lPhysicalRecordIndex = GetLastReadMarkIndex();
		if (lPhysicalRecordIndex < ivMarkedInstances.GetSize() and
		    not ivMarkedInstances.GetAt((int)lPhysicalRecordIndex))
		{
			delete kwoObject;
			kwoObject = NULL;
		}
	}

	// Mutation en objet logique
	if (kwoObject != NULL)
	{
		// Parametrage de la gestion des erreurs dans les regles de conversion
		KWDRConversionRule::SetErrorSender(this);

		// Mutation de l'objet
		MutatePhysicalObject(kwoObject);

		// Reinitialisation du parametrage de la gestion des erreurs
		KWDRConversionRule::SetErrorSender(NULL);
	}
	assert(kwoObject == NULL or kwoObject->GetClass() == kwcClass);

	// Warning selon l'etat du memory guard
	// On emet que des warning, car on est toujours capable de "rattaper" l'erreur
	// Par controle, on passe par handler des gestion des flow d'erreur pour emmetre ces warning meme
	// en cas de depassement du flow. Ce handler est parametre lors des ouverture-fermeture de base
	if (kwoObject != NULL)
	{
		// Cas du depassement memoire, avec fonctionnement degrade (variables calculees mises a missing)
		if (memoryGuard.IsSingleInstanceMemoryLimitReached())
			AddWarning(memoryGuard.GetSingleInstanceMemoryLimitLabel());
		// Cas  du seuil de nombre d'enregistrements secondaires depasse, ou d'un temps de calcul
		// potentiellement allonge
		else if (memoryGuard.IsSingleInstanceVeryLarge())
			AddWarning(memoryGuard.GetSingleInstanceVeryLargeLabel());
	}
	return kwoObject;
}

void KWDatabase::Skip()
{
	require(GetClassName() != "");
	require(kwcClass != NULL);
	require(kwcClass->GetFreshness() == nClassFreshness);
	require(IsOpenedForRead());
	require(kwcPhysicalClass != NULL);
	require(kwcClass->GetName() == kwcPhysicalClass->GetName());

	// Saut d'un enregistrement
	if (not IsPhysicalEnd())
		PhysicalSkip();
}

boolean KWDatabase::Close()
{
	boolean bOk;

	require(IsOpenedForRead() or IsOpenedForWrite());
	require(GetClassName() != "");
	require(kwcClass != NULL);
	require(kwcClass->GetFreshness() == nClassFreshness);

	// Desinstallation du handler specifique pour ignorer le flow des erreur dans le cas du memory guard
	// Uniquement dans le cas des bases ouvertes en lecture
	if (IsOpenedForRead())
		KWDatabaseMemoryGuard::UninstallMemoryGuardErrorFlowIgnoreFunction();

	// Fermeture physique
	bOk = PhysicalClose();
	bOpenedForRead = false;
	bOpenedForWrite = false;
	bIsError = false;
	nClassFreshness = 0;

	// Destruction de la classe physique
	DeletePhysicalClass();
	kwcClass = NULL;
	return bOk;
}

boolean KWDatabase::ReadAll()
{
	boolean bOk = true;
	KWObject* kwoObject;
	ObjectArray oaPhysicalMessages;
	longint lObjectNumber;
	longint lRecordNumber;
	ALString sTmp;

	require(GetClassName() != "");
	require(kwcClass == NULL);
	require(kwcPhysicalClass == NULL);
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Read database " + GetDatabaseName());

	// Destruction des objets en cours
	DeleteAll();

	// Ouverture de la base en lecture
	bOk = OpenForRead();

	// Lecture d'objets dans la base
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		lRecordNumber = 0;
		lObjectNumber = 0;
		while (not IsEnd())
		{
			kwoObject = Read();
			lRecordNumber++;
			if (kwoObject != NULL)
			{
				lObjectNumber++;

				// Ajout de l'objet au tableau
				oaAllObjects.Add(kwoObject);

				// Test de depassement de capacite du tableau
				if (oaAllObjects.GetSize() == INT_MAX)
				{
					bOk = false;
					Object::AddError(
					    sTmp +
					    "Read database interrupted because maximum number of instances reached (" +
					    IntToString(INT_MAX) + ")");
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

			// Arret si erreur
			if (IsError())
			{
				bOk = false;
				Object::AddError("Read database interrupted because of errors");
				break;
			}

			// Suivi de la tache
			if (TaskProgression::IsRefreshNecessary())
			{
				TaskProgression::DisplayProgression((int)(100 * GetReadPercentage()));
				DisplayReadTaskProgressionLabel(lRecordNumber, lObjectNumber);
			}
		}
		Global::DesactivateErrorFlowControl();

		// Test si interruption sans qu'il y ait d'erreur
		if (not IsError() and TaskProgression::IsInterruptionRequested())
		{
			bOk = false;
			Object::AddWarning("Read database interrupted by user");
		}

		// Fermeture
		CollectPhysicalStatsMessages(&oaPhysicalMessages);
		bOk = Close() and bOk;

		// Message global de compte-rendu
		if (bOk)
		{
			if (lRecordNumber == lObjectNumber)
				AddMessage(sTmp + "Read records: " + LongintToReadableString(lObjectNumber));
			else
				AddMessage(sTmp + "Read records: " + LongintToReadableString(lRecordNumber) +
					   "\tSelected records: " + LongintToReadableString(lObjectNumber));
			DisplayPhysicalMessages(&oaPhysicalMessages);
		}
		oaPhysicalMessages.DeleteAll();
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();

	// Nettoyage si necessaire
	if (not bOk)
		DeleteAll();
	return bOk;
}

void KWDatabase::RemoveAll()
{
	oaAllObjects.RemoveAll();
}

void KWDatabase::DeleteAll()
{
	boolean bDisplay = false;
	Timer timer;
	boolean bIsInTask;
	int nObject;

	// On teste si on en en cours de suivi de tache
	// La destruction d'une base de grande taille pouvant etre tres longue donne
	// lieu a un suivi
	bIsInTask = TaskProgression::IsInTask();

	// Destruction des objet sans gestion de l'anvancement
	if (not bIsInTask)
		oaAllObjects.DeleteAll();
	// Destruction des objet avec gestion de l'anvancement
	else
	{
		// Debut de suivi de tache
		TaskProgression::BeginTask();
		TaskProgression::DisplayLabel("Clean memory for database " + GetDatabaseName());
		timer.Start();

		// Destruction des objets
		for (nObject = 0; nObject < oaAllObjects.GetSize(); nObject++)
		{
			// Destruction de l'objet courant
			delete oaAllObjects.GetAt(nObject);

			// Suivi de la tache (sans test d'interruption: il faut detruire tous les objets)
			if (TaskProgression::IsRefreshNecessary())
				TaskProgression::DisplayProgression((int)(nObject * 100.0 / oaAllObjects.GetSize()));
			if (bDisplay)
			{
				if (nObject == 0)
					cout << "Database " << GetDatabaseName() << " DeleteAll" << endl;
				if (nObject == oaAllObjects.GetSize() - 1 or nObject % 100000 == 0)
					cout << timer.GetElapsedTime() << "\t" << nObject << "\t"
					     << Symbol::GetSymbolNumber() << "\t" << MemGetHeapMemory() << "\t"
					     << MemGetTotalHeapRequestedMemory() << endl;
			}
		}
		oaAllObjects.SetSize(0);

		// Fin de suivi de tache
		TaskProgression::DisplayLabel("");
		TaskProgression::DisplayProgression(0);
		TaskProgression::EndTask();
	}
}

boolean KWDatabase::WriteAll(KWDatabase* sourceObjects)
{
	boolean bOk = true;
	KWObject* kwoObject;
	int nObject;
	ObjectArray oaPhysicalMessages;
	longint lObjectNumber;
	ALString sTmp;

	require(sourceObjects != NULL);
	require(GetClassName() != "");
	require(kwcClass == NULL);
	require(kwcPhysicalClass == NULL);
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Write database " + GetDatabaseName());

	// Ouverture de la base en ecriture
	OpenForWrite();

	// Ecriture d'objets dans la base
	if (IsOpenedForWrite())
	{
		// Parcours des objets sources
		Global::ActivateErrorFlowControl();
		lObjectNumber = 0;
		for (nObject = 0; nObject < sourceObjects->GetObjects()->GetSize(); nObject++)
		{
			kwoObject = cast(KWObject*, sourceObjects->GetObjects()->GetAt(nObject));
			check(kwoObject);
			assert(kwoObject->GetClass() == kwcClass);
			Write(kwoObject);
			lObjectNumber++;

			// Arret si erreur
			if (IsError())
			{
				Object::AddError("Write database interrupted because of errors");
				bOk = false;
				break;
			}

			// Suivi de la tache
			if (TaskProgression::IsRefreshNecessary())
			{
				TaskProgression::DisplayProgression(
				    (int)(nObject * 100.0 / sourceObjects->GetObjects()->GetSize()));
				if (TaskProgression::IsInterruptionRequested())
					break;
			}
		}
		Global::DesactivateErrorFlowControl();

		// Test si interruption sans qu'il y ait d'erreur
		if (not IsError() and TaskProgression::IsInterruptionRequested())
		{
			// Pas de mise en erreur: le fichier est juste ecrit partiellement
			Object::AddWarning("Write database interrupted by user");
		}

		// Fermeture
		CollectPhysicalStatsMessages(&oaPhysicalMessages);
		bOk = Close() and bOk;

		// Message global de compte-rendu si Ok
		if (bOk)
		{
			AddMessage(sTmp + "Written records: " + LongintToReadableString(lObjectNumber));
			DisplayPhysicalMessages(&oaPhysicalMessages);
		}
		// Destruction de la base sinon
		else
			PhysicalDeleteDatabase();
		oaPhysicalMessages.DeleteAll();
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();
	return bOk;
}

longint KWDatabase::GetExactObjectNumber()
{
	longint lNumber;

	require(GetClassName() != "");
	require(kwcClass == NULL);
	require(kwcPhysicalClass == NULL);
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());

	// Ouverture de la base en lecture
	OpenForRead();

	// Lecture d'objets dans la base
	lNumber = 0;
	if (IsOpenedForRead())
	{
		Global::ActivateErrorFlowControl();
		while (not IsEnd() and not IsError())
		{
			PhysicalSkip();
			lNumber++;
		}
		Global::DesactivateErrorFlowControl();

		// Fermeture
		Close();
	}
	return lNumber;
}

void KWDatabase::AddFormatMetaData(KWAttribute* sourceAttribute,
				   const KWTypeAutomaticRecognition* typeAutomaticRecognition) const
{
	ALString sFormatMetaDataKey;
	ALString sIncompatibleFormat;
	int i;

	require(sourceAttribute != NULL);
	require(KWType::IsComplex(sourceAttribute->GetType()) or sourceAttribute->GetType() == KWType::Continuous);
	require(typeAutomaticRecognition != NULL);
	require(typeAutomaticRecognition->GetMainMatchingType() == sourceAttribute->GetType());

	// Acces a la meta-donne de format dans le cas d'un type complexe
	if (KWType::IsComplex(sourceAttribute->GetType()))
	{
		assert(not typeAutomaticRecognition->IsContinuousTypeMatching());
		sFormatMetaDataKey = sourceAttribute->GetFormatMetaDataKey(sourceAttribute->GetType());
	}

	// Cas d'un attribut Date
	if (typeAutomaticRecognition->GetMainMatchingType() == KWType::Date)
	{
		// Ajout de meta-donnee de format si necessaire
		if (not typeAutomaticRecognition->GetMatchingDateFormatAt(0)->IsConsistentWith(&dateDefaultConverter))
			sourceAttribute->GetMetaData()->SetStringValueAt(
			    sFormatMetaDataKey,
			    typeAutomaticRecognition->GetMatchingDateFormatAt(0)->GetFormatString());

		// Emission d'un warning si type incoherents
		if (not typeAutomaticRecognition->AreMatchingDateFormatConsistent())
		{
			sIncompatibleFormat = "";
			for (i = 1; i < typeAutomaticRecognition->GetMatchingDateFormatNumber(); i++)
			{
				if (not typeAutomaticRecognition->GetMatchingDateFormatAt(0)->IsConsistentWith(
					typeAutomaticRecognition->GetMatchingDateFormatAt(i)))
				{
					sIncompatibleFormat =
					    typeAutomaticRecognition->GetMatchingDateFormatAt(i)->GetFormatString();
					break;
				}
			}
			AddWarning("Variable " + sourceAttribute->GetName() + " recognized as type " +
				   KWType::ToString(KWType::Date) + " with format " +
				   typeAutomaticRecognition->GetMatchingDateFormatAt(0)->GetFormatString() +
				   " but other incompatible formats are possible (e.g.: " + sIncompatibleFormat + ")");
		}
	}
	// Cas d'un attribut Time
	else if (typeAutomaticRecognition->GetMainMatchingType() == KWType::Time)
	{
		// Ajout de meta-donnee de format si necessaire
		if (not typeAutomaticRecognition->GetMatchingTimeFormatAt(0)->IsConsistentWith(&timeDefaultConverter))
			sourceAttribute->GetMetaData()->SetStringValueAt(
			    sFormatMetaDataKey,
			    typeAutomaticRecognition->GetMatchingTimeFormatAt(0)->GetFormatString());

		// Emission d'un warning si type incoherents
		if (not typeAutomaticRecognition->AreMatchingTimeFormatConsistent())
		{
			sIncompatibleFormat = "";
			for (i = 1; i < typeAutomaticRecognition->GetMatchingTimeFormatNumber(); i++)
			{
				if (not typeAutomaticRecognition->GetMatchingTimeFormatAt(0)->IsConsistentWith(
					typeAutomaticRecognition->GetMatchingTimeFormatAt(i)))
				{
					sIncompatibleFormat =
					    typeAutomaticRecognition->GetMatchingTimeFormatAt(i)->GetFormatString();
					break;
				}
			}
			AddWarning("Variable " + sourceAttribute->GetName() + " recognized as type " +
				   KWType::ToString(KWType::Time) + " with format " +
				   typeAutomaticRecognition->GetMatchingTimeFormatAt(0)->GetFormatString() +
				   " but other incompatible formats are possible (e.g.: " + sIncompatibleFormat + ")");
		}
	}
	// Cas d'un attribut Timestamp
	else if (typeAutomaticRecognition->GetMainMatchingType() == KWType::Timestamp)
	{
		// Ajout de meta-donnee de format si necessaire
		if (not typeAutomaticRecognition->GetMatchingTimestampFormatAt(0)->IsConsistentWith(
			&timestampDefaultConverter))
			sourceAttribute->GetMetaData()->SetStringValueAt(
			    sFormatMetaDataKey,
			    typeAutomaticRecognition->GetMatchingTimestampFormatAt(0)->GetFormatString());

		// Emission d'un warning si type incoherents
		if (not typeAutomaticRecognition->AreMatchingTimestampFormatConsistent())
		{
			sIncompatibleFormat = "";
			for (i = 1; i < typeAutomaticRecognition->GetMatchingTimestampFormatNumber(); i++)
			{
				if (not typeAutomaticRecognition->GetMatchingTimestampFormatAt(0)->IsConsistentWith(
					typeAutomaticRecognition->GetMatchingTimestampFormatAt(i)))
				{
					sIncompatibleFormat = typeAutomaticRecognition->GetMatchingTimestampFormatAt(i)
								  ->GetFormatString();
					break;
				}
			}
			AddWarning("Variable " + sourceAttribute->GetName() + " recognized as type " +
				   KWType::ToString(KWType::Timestamp) + " with format " +
				   typeAutomaticRecognition->GetMatchingTimestampFormatAt(0)->GetFormatString() +
				   " but other incompatible formats are possible (e.g.: " + sIncompatibleFormat + ")");
		}
	}
	// Cas d'un attribut TimestampTZ
	else if (typeAutomaticRecognition->GetMainMatchingType() == KWType::TimestampTZ)
	{
		// Ajout de meta-donnee de format si necessaire
		if (not typeAutomaticRecognition->GetMatchingTimestampTZFormatAt(0)->IsConsistentWith(
			&timestampTZDefaultConverter))
			sourceAttribute->GetMetaData()->SetStringValueAt(
			    sFormatMetaDataKey,
			    typeAutomaticRecognition->GetMatchingTimestampTZFormatAt(0)->GetFormatString());

		// Emission d'un warning si type incoherents
		if (not typeAutomaticRecognition->AreMatchingTimestampTZFormatConsistent())
		{
			sIncompatibleFormat = "";
			for (i = 1; i < typeAutomaticRecognition->GetMatchingTimestampTZFormatNumber(); i++)
			{
				if (not typeAutomaticRecognition->GetMatchingTimestampTZFormatAt(0)->IsConsistentWith(
					typeAutomaticRecognition->GetMatchingTimestampTZFormatAt(i)))
				{
					sIncompatibleFormat =
					    typeAutomaticRecognition->GetMatchingTimestampTZFormatAt(i)
						->GetFormatString();
					break;
				}
			}
			AddWarning("Variable " + sourceAttribute->GetName() + " recognized as type " +
				   KWType::ToString(KWType::TimestampTZ) + " with format " +
				   typeAutomaticRecognition->GetMatchingTimestampTZFormatAt(0)->GetFormatString() +
				   " but other incompatible formats are possible (e.g.: " + sIncompatibleFormat + ")");
		}
	}
	else
	{
		assert(typeAutomaticRecognition->GetMainMatchingType() == KWType::Continuous);

		// Warning si type complexe disponible (en plus des type Continuous et Symbol)
		if (typeAutomaticRecognition->GetMatchingTypeNumber() >= 3)
		{
			// Warning si type Date possible egalement
			if (typeAutomaticRecognition->GetMatchingDateFormatNumber() > 0)
			{
				AddWarning("Variable " + sourceAttribute->GetName() + " recognized as type " +
					   KWType::ToString(KWType::Continuous) + " but " +
					   KWType::ToString(KWType::Date) + " format is also possible (e.g.: " +
					   typeAutomaticRecognition->GetMatchingDateFormatAt(0)->GetFormatString() +
					   ")");
			}
			// Warning si type Time possible egalement
			else if (typeAutomaticRecognition->GetMatchingTimeFormatNumber() > 0)
			{
				AddWarning("Variable " + sourceAttribute->GetName() + " recognized as type " +
					   KWType::ToString(KWType::Continuous) + " but " +
					   KWType::ToString(KWType::Time) + " format is also possible (e.g.: " +
					   typeAutomaticRecognition->GetMatchingTimeFormatAt(0)->GetFormatString() +
					   ")");
			}
			// Warning si type Timestamp possible egalement
			else if (typeAutomaticRecognition->GetMatchingTimestampFormatNumber() > 0)
			{
				AddWarning(
				    "Variable " + sourceAttribute->GetName() + " recognized as type " +
				    KWType::ToString(KWType::Continuous) + " but " +
				    KWType::ToString(KWType::Timestamp) + " format is also possible (e.g.: " +
				    typeAutomaticRecognition->GetMatchingTimestampFormatAt(0)->GetFormatString() + ")");
			}
		}
	}
}

void KWDatabase::AddSimpleMessage(const ALString& sLabel) const
{
	if (not bSilentMode and bVerboseMode)
		Object::AddSimpleMessage(sLabel);
}

void KWDatabase::AddMessage(const ALString& sLabel) const
{
	if (not bSilentMode and bVerboseMode)
		Object::AddMessage(sLabel);
}

void KWDatabase::AddWarning(const ALString& sLabel) const
{
	if (not bSilentMode and bVerboseMode)
		Object::AddWarning(sLabel);
}

void KWDatabase::AddError(const ALString& sLabel) const
{
	if (not bSilentMode)
		Object::AddError(sLabel);
}

void KWDatabase::DisplayReadTaskProgressionLabel(longint lRecordNumber, longint lObjectNumber)

{
	ALString sTmp;

	require(0 <= lObjectNumber);
	require(lObjectNumber <= lRecordNumber);
	require(TaskProgression::IsInTask());

	// Libelle
	if (lRecordNumber == lObjectNumber)
		TaskProgression::DisplayLabel(sTmp + LongintToReadableString(lObjectNumber) + " instances");
	else
		TaskProgression::DisplayLabel(sTmp + LongintToReadableString(lObjectNumber) + " instances  (" +
					      LongintToReadableString(lRecordNumber) + " records)");
}

longint KWDatabase::GetUsedMemory() const
{
	longint lUsedMemory;
	KWObject* kwoObject;
	int nObject;

	// Memoire de base
	lUsedMemory = sizeof(KWDatabase);

	// Memoire par attribut non elementaire
	lUsedMemory += nkdUnusedNativeAttributesToKeep.GetUsedMemory();
	lUsedMemory += oaAllObjects.GetUsedMemory();
	lUsedMemory += sClassName.GetLength();
	lUsedMemory += sDatabaseName.GetLength();
	lUsedMemory += sSelectionAttribute.GetLength();
	lUsedMemory += sSelectionValue.GetLength();
	lUsedMemory += ivMarkedInstances.GetUsedMemory();

	// Memoire de la classe physique si presente
	if (kwcPhysicalClass != NULL)
		lUsedMemory += kwcPhysicalClass->GetDomain()->GetUsedMemory();

	// Objets charges en memoire
	for (nObject = 0; nObject < oaAllObjects.GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, oaAllObjects.GetAt(nObject));
		lUsedMemory += kwoObject->GetUsedMemory();
	}
	return lUsedMemory;
}

longint KWDatabase::ComputeOpenNecessaryMemory(boolean bRead, boolean bIncludingClassMemory)
{
	longint lNecessaryMemory;
	longint lClassNecessaryMemory;
	longint lPhysicalClassNecessaryMemory;

	require(not IsOpenedForRead() and not IsOpenedForWrite());
	require(kwcClass != NULL);

	// Prise en compte de la classe courante
	lNecessaryMemory = 0;
	lClassNecessaryMemory = 0;
	if (bIncludingClassMemory)
	{
		lClassNecessaryMemory = KWClassDomain::GetCurrentDomain()->GetClassDependanceUsedMemory(kwcClass);
		lNecessaryMemory += lClassNecessaryMemory;
	}

	// En lecture, on prend en compte le dictionnaire physique
	if (bRead)
	{
		// On utilise la classe physique si elle est disponible
		if (kwcPhysicalClass != NULL)
		{
			if (kwcPhysicalClass != kwcClass)
				lPhysicalClassNecessaryMemory =
				    kwcPhysicalClass->GetDomain()->GetClassDependanceUsedMemory(kwcPhysicalClass);
			else
				lPhysicalClassNecessaryMemory = 0;
		}
		// Sinon, on prend la classe courante pour avoir une estimation de type pire cas
		else
		{
			// On ne recalcul la memoire la memoire ncessaire que si elle n'est pas deja calculee
			if (lClassNecessaryMemory == 0)
				lPhysicalClassNecessaryMemory =
				    KWClassDomain::GetCurrentDomain()->GetClassDependanceUsedMemory(kwcClass);
			else
				lPhysicalClassNecessaryMemory = lClassNecessaryMemory;
		}
		lNecessaryMemory += lPhysicalClassNecessaryMemory;
	}
	ensure(lNecessaryMemory >= 0);
	return lNecessaryMemory;
}

void KWDatabase::WriteJSONFields(JSONFile* fJSON)
{
	fJSON->WriteKeyString("database", GetDatabaseName());

	// Taux d'echantillonnage
	fJSON->WriteKeyDouble("samplePercentage", GetSampleNumberPercentage());
	fJSON->WriteKeyString("samplingMode", GetSamplingMode());

	// Variable de selection
	fJSON->WriteKeyString("selectionVariable", GetSelectionAttribute());
	fJSON->WriteKeyString("selectionValue", GetSelectionValue());
}

const ALString KWDatabase::GetClassLabel() const
{
	return "Database";
}

const ALString KWDatabase::GetObjectLabel() const
{
	ALString sLabel;
	longint lPhysicalRecordIndex;

	// On ne met l'index de record que si la base est ouverte
	sLabel = GetDatabaseName();
	lPhysicalRecordIndex = GetPhysicalRecordIndex();
	if (lPhysicalRecordIndex > 0 and (IsOpenedForRead() or IsOpenedForWrite()))
	{
		sLabel += " : Record ";
		sLabel += LongintToString(lPhysicalRecordIndex);
	}
	return sLabel;
}

void KWDatabase::TestCreateObjects(int nNumber)
{
	KWClass* kwcCreationClass;
	int i;

	require(kwcClass == NULL);
	require(nNumber >= 0);

	// Creation d'un nom de classe si necessaire
	if (GetClassName() == "")
		SetClassName("TestClass");

	// Creation d'une classe si necessaire
	kwcCreationClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	if (kwcCreationClass == NULL)
	{
		cout << "Create dictionary " << GetClassName() << endl;
		kwcCreationClass = KWClass::CreateClass(GetClassName(), 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, true, NULL);
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcCreationClass);
		kwcCreationClass->Compile();
	}

	// Creation d'objets
	DeleteAll();
	for (i = 0; i < nNumber; i++)
		GetObjects()->Add(KWObject::CreateObject(kwcCreationClass, (longint)i + 1));
}

void KWDatabase::TestRead()
{
	KWClass* kwcTestClass;
	boolean bOk;
	KWObject* kwoObject;
	int nNumber;

	require(GetClassName() != "");
	require(kwcClass == NULL);

	// Construction si necessaire de la classe a partir de la base
	kwcTestClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	if (kwcTestClass == NULL or kwcTestClass->GetAttributeNumber() == 0)
	{
		cout << "Build dictionary " << GetClassName() << " from database " << GetDatabaseName() << endl;
		ComputeClass();
	}

	// Evaluation du nombre d'instances
	cout << "Evaluation of the number of instances" << endl;
	cout << "\t" << GetEstimatedObjectNumber() << " instances" << endl;

	// Lecture instance par instance
	cout << "Read instance by instance" << endl;
	bOk = OpenForRead();
	nNumber = 0;
	if (bOk)
	{
		while (not IsEnd())
		{
			kwoObject = Read();
			if (kwoObject != NULL)
			{
				nNumber++;
				delete kwoObject;
			}
		}
		Close();
	}
	cout << "\t" << nNumber << " instances" << endl;

	// Lecture globale
	cout << "Read globally" << endl;
	ReadAll();
	cout << "\t" << GetObjects()->GetSize() << " instances" << endl;
}

//////////////////////////////////////////////////////////////////////////////////

void KWDatabase::BuildPhysicalClass()
{
	const boolean bTrace = false;
	boolean bDebug = false;
	KWClassDomain* kwcdPhysicalDomain;
	boolean bPhysicalDomainNeeded;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWDataItem* dataItem;
	KWDerivationRule* currentDerivationRule;
	int i;
	KWAttribute* keyAttribute;
	int nAttribute;
	int nAttributeBlock;
	NumericKeyDictionary nkdClassNeededAttributes;
	ObjectArray oaClassNeededAttributes;
	NumericKeyDictionary nkdAllNeededAttributes;
	NumericKeyDictionary nkdLoadedAttributeBlocks;
	NumericKeyDictionary nkdAllNeededClasses;
	NumericKeyDictionary nkdLogicalyNeededClasses;
	ObjectArray oaAllNeededClasses;
	ObjectArray oaAttributesToDelete;
	ObjectArray oaNeededAttributes;
	ObjectArray oaNeededAttributeBlocks;
	int nClass;
	KWClass* kwcRefClass;
	KWClass* kwcCurrentPhysicalClass;
	KWClass* kwcInitialClass;
	KWAttribute* initialAttribute;
	KWAttributeBlock* initialAttributeBlock;
	KWAttribute* attributeToDelete;

	require(kwcClass != NULL);
	require(kwcClass->GetName() == GetClassName());
	require(kwcClass->IsCompiled());
	require(kwcPhysicalClass == NULL);

	// Affichage du debut de methode
	if (bTrace)
	{
		cout << "\nBuildPhysicalClass(" << kwcClass->GetName() << ") (process " << GetProcessId() << ")\n";

		// Caracteristique du domaine
		cout << " Domain " << kwcClass->GetDomain()->GetName() << endl;
		for (i = 0; i < kwcClass->GetDomain()->GetClassNumber(); i++)
		{
			kwcInitialClass = kwcClass->GetDomain()->GetClassAt(i);
			cout << "\t" << kwcInitialClass->GetName()
			     << " (Used: " << kwcInitialClass->GetUsedAttributeNumber()
			     << ", Loaded: " << kwcInitialClass->GetLoadedAttributeNumber()
			     << ", Dense: " << kwcInitialClass->GetLoadedDenseAttributeNumber()
			     << ", Block: " << kwcInitialClass->GetLoadedAttributeBlockNumber()
			     << ", Relation: " << kwcInitialClass->GetLoadedRelationAttributeNumber() << ")" << endl;
		}

		// Caracteristique de la classe logique
		cout << "Logical class\t" << kwcClass->GetName() << " (Used: " << kwcClass->GetUsedAttributeNumber()
		     << ", Loaded: " << kwcClass->GetLoadedAttributeNumber()
		     << ", Dense: " << kwcClass->GetLoadedDenseAttributeNumber()
		     << ", Block: " << kwcClass->GetLoadedAttributeBlockNumber()
		     << ", Relation: " << kwcClass->GetLoadedRelationAttributeNumber() << ")" << endl;
		attribute = kwcClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			cout << "\t";
			if (attribute->GetUsed())
				cout << "U";
			if (attribute->GetLoaded())
				cout << "L";
			if (attribute->IsInBlock())
				cout << "B";
			cout << "\t" << KWType::ToString(attribute->GetType());
			cout << "\t" << attribute->GetName();
			cout << "\t" << attribute->GetLoadIndex();
			cout << "\n";
			kwcClass->GetNextAttribute(attribute);
		}
	}

	// Creation d'un nouveau domaine
	kwcdPhysicalDomain = kwcClass->GetDomain()->CloneFromClass(kwcClass);
	kwcdPhysicalDomain->SetName("Physical");
	kwcdPhysicalDomain->Compile();

	// Recherche de la classe physique correspondante
	kwcPhysicalClass = kwcdPhysicalDomain->LookupClass(GetClassName());
	assert(kwcClass->GetLoadedAttributeNumber() == kwcPhysicalClass->GetLoadedAttributeNumber());

	// Initialisation de l'ensemble des classes physiques necessaires avec la classe physique "racine"
	// Les classes physiques correspondent a tout ce qui doit etre lu physiquement directement ou indirectement
	// pour le calcul des regles de derivation
	nkdAllNeededClasses.SetAt(kwcPhysicalClass, kwcPhysicalClass);
	oaAllNeededClasses.Add(kwcPhysicalClass);

	// Initialisation des classes necessaires au niveau logique
	// Les classes logiques sont celles dont l'utilisateur demande explicitement (Loaded) le chargement en memoire
	nkdLogicalyNeededClasses.SetAt(kwcPhysicalClass, kwcPhysicalClass);

	// Calcul de tous les operandes terminaux intervenant dans une derivation d'un
	// attribut Loaded de la classe physique (ou d'une autre classe du domaine physique,
	// ou de l'attribut de selection)
	for (nClass = 0; nClass < oaAllNeededClasses.GetSize(); nClass++)
	{
		assert(nkdAllNeededClasses.GetCount() == oaAllNeededClasses.GetSize());
		kwcCurrentPhysicalClass = cast(KWClass*, oaAllNeededClasses.GetAt(nClass));

		// Recherche des attributs necessaires pour cette classe
		currentDerivationRule = NULL;
		assert(nkdClassNeededAttributes.GetCount() == 0);
		for (nAttribute = 0; nAttribute < kwcCurrentPhysicalClass->GetLoadedAttributeNumber(); nAttribute++)
		{
			attribute = kwcCurrentPhysicalClass->GetLoadedAttributeAt(nAttribute);

			// Analyse de la regle de derivation
			// Dans le cas d'un bloc, il faut en effet la reanalyser pour chaque attribut du bloc
			// pour detecter les attributs utilises des blocs potentiellement en operande
			currentDerivationRule = attribute->GetAnyDerivationRule();
			if (currentDerivationRule != NULL)
				currentDerivationRule->BuildAllUsedAttributes(attribute, &nkdClassNeededAttributes);

			// Ajout eventuelle de la classe necessaire
			if (KWType::IsRelation(attribute->GetType()))
			{
				kwcRefClass = attribute->GetClass();

				// Ajout au niveau physique
				if (nkdAllNeededClasses.Lookup(kwcRefClass) == NULL)
				{
					nkdAllNeededClasses.SetAt(kwcRefClass, kwcRefClass);
					oaAllNeededClasses.Add(kwcRefClass);
				}

				// Ajout au niveau logique
				if (nkdLogicalyNeededClasses.Lookup(kwcRefClass) == NULL)
					nkdLogicalyNeededClasses.SetAt(kwcRefClass, kwcRefClass);
			}
		}

		// Ajout des operandes utilise pour le calcul de l'attribut de selection pour la classe racine
		if (kwcCurrentPhysicalClass == kwcPhysicalClass)
		{
			attribute = kwcPhysicalClass->LookupAttribute(GetSelectionAttribute());
			assert(attribute == NULL or KWType::IsSimple(attribute->GetType()));
			if (attribute != NULL and attribute->GetAnyDerivationRule() != NULL)
				attribute->GetAnyDerivationRule()->BuildAllUsedAttributes(attribute,
											  &nkdClassNeededAttributes);
		}

		// Ajout de l'attribut de selection pour la classe racine
		if (kwcCurrentPhysicalClass == kwcPhysicalClass)
		{
			attribute = kwcPhysicalClass->LookupAttribute(GetSelectionAttribute());
			if (attribute != NULL and not attribute->GetLoaded())
				nkdAllNeededAttributes.SetAt(attribute, attribute);
		}

		// Ajout de la cle pour toute classe chargeable depuis la classe racine
		for (i = 0; i < kwcCurrentPhysicalClass->GetKeyAttributeNumber(); i++)
		{
			keyAttribute = kwcCurrentPhysicalClass->GetKeyAttributeAt(i);
			check(keyAttribute);
			assert(keyAttribute->GetAttributeBlock() == NULL);
			nkdAllNeededAttributes.SetAt(keyAttribute, keyAttribute);
		}

		// Recherche parmi les operandes de tous les attributs (et classes) necessaires
		nkdClassNeededAttributes.ExportObjectArray(&oaClassNeededAttributes);
		for (i = 0; i < oaClassNeededAttributes.GetSize(); i++)
		{
			attribute = cast(KWAttribute*, oaClassNeededAttributes.GetAt(i));

			// Ajout de l'attribut si necessaire
			if (nkdAllNeededAttributes.Lookup(attribute) == NULL)
			{
				nkdAllNeededAttributes.SetAt(attribute, attribute);

				// Dans le cas d'un attribut de table secondaire, ajout si necessaire de l'attribut cle
				// de la classe origine et destination
				if (KWType::IsRelation(attribute->GetType()))
				{
					// Ajout de la classe destination au niveau physique
					if (nkdAllNeededClasses.Lookup(attribute->GetClass()) == NULL)
					{
						nkdAllNeededClasses.SetAt(attribute->GetClass(), attribute->GetClass());
						oaAllNeededClasses.Add(attribute->GetClass());
					}
				}
			}

			// Ajout eventuel de la classe necessaire
			if (attribute != NULL and attribute->GetLoaded() and KWType::IsRelation(attribute->GetType()))
			{
				kwcRefClass = attribute->GetClass();
				if (nkdAllNeededClasses.Lookup(kwcRefClass) == NULL)
				{
					nkdAllNeededClasses.SetAt(kwcRefClass, kwcRefClass);
					oaAllNeededClasses.Add(kwcRefClass);
				}
			}
		}
		nkdClassNeededAttributes.RemoveAll();
		oaClassNeededAttributes.RemoveAll();
	}

	// Affichage des classes et attributs necessaires
	if (bTrace)
	{
		// Affichages des classes necessaires
		cout << "Needed classes\n";
		for (nClass = 0; nClass < oaAllNeededClasses.GetSize(); nClass++)
		{
			kwcCurrentPhysicalClass = cast(KWClass*, oaAllNeededClasses.GetAt(nClass));
			cout << "\t" << kwcCurrentPhysicalClass->GetName();
			if (nkdLogicalyNeededClasses.Lookup(kwcCurrentPhysicalClass))
				cout << "\tLogical";
			else
				cout << "\tPhysical";
			cout << endl;
		}

		// Affichage des attributs necessaires
		cout << "Needed attributes\t" << nkdAllNeededAttributes.GetCount() << endl;
		nkdAllNeededAttributes.ExportObjectArray(&oaNeededAttributes);
		oaNeededAttributes.SetCompareFunction(KWAttributeCompareName);
		oaNeededAttributes.Sort();
		for (nAttribute = 0; nAttribute < oaNeededAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, oaNeededAttributes.GetAt(nAttribute));
			cout << "\t" << attribute->GetParentClass()->GetName() << "\t" << attribute->GetName() << endl;
		}
		oaNeededAttributes.RemoveAll();
	}

	// On determine si le domaine physique est necessaire, c'est a dire s'il existe au moins
	// un attribut non loaded (ou d'une classe non necessaire logiquement) qui soit necessaire
	// pour le calcul des regles de derivations
	bPhysicalDomainNeeded = (kwcdPhysicalDomain->GetClassNumber() != nkdAllNeededClasses.GetCount());
	nkdAllNeededAttributes.ExportObjectArray(&oaNeededAttributes);
	for (nAttribute = 0; nAttribute < oaNeededAttributes.GetSize(); nAttribute++)
	{
		attribute = cast(KWAttribute*, oaNeededAttributes.GetAt(nAttribute));
		if (not attribute->GetLoaded() or nkdLogicalyNeededClasses.Lookup(attribute->GetParentClass()) == NULL)
		{
			bPhysicalDomainNeeded = true;
			break;
		}
	}
	oaNeededAttributes.RemoveAll();
	if (bTrace)
		cout << "Need physical domain\t" << bPhysicalDomainNeeded << endl;

	// Si domaine physique non necessaire, destruction et remplacement par
	// le domain initial
	if (not bPhysicalDomainNeeded)
	{
		DeletePhysicalClass();
		kwcPhysicalClass = kwcClass;
		kwcdPhysicalDomain = kwcClass->GetDomain();
	}
	// Sinon, construction des classes physiques necessaires
	else
	{
		// Parametrage des classes necessaires
		for (nClass = 0; nClass < oaAllNeededClasses.GetSize(); nClass++)
		{
			kwcCurrentPhysicalClass = cast(KWClass*, oaAllNeededClasses.GetAt(nClass));

			// Identification des blocs d'attributs charges en memoire
			// Ces blocs initialement charges en memoire ne sont ni a supprimer, ni a deplacer en fin de
			// classe
			assert(kwcCurrentPhysicalClass->IsIndexed());
			nkdLoadedAttributeBlocks.RemoveAll();
			for (nAttributeBlock = 0;
			     nAttributeBlock < kwcCurrentPhysicalClass->GetLoadedAttributeBlockNumber();
			     nAttributeBlock++)
			{
				attributeBlock = kwcCurrentPhysicalClass->GetLoadedAttributeBlockAt(nAttributeBlock);
				nkdLoadedAttributeBlocks.SetAt(attributeBlock, attributeBlock);
			}

			// Si la classe n'est pas necessaire au niveau logique,
			// ses attributs de base ne sont pas a charger
			if (nkdLogicalyNeededClasses.Lookup(kwcCurrentPhysicalClass) == NULL)
				kwcCurrentPhysicalClass->SetAllAttributesLoaded(false);

			////////////////////////////////////////////////////////////////////////
			// Reamenagement de la classe: on va garder les attributs Loaded en tete,
			// puis ajouter les autres attributs necessaires aux derivations.
			// Tous ces attributs seront marques comme Used et Loaded, les autres
			// seront detruits
			// Les blocs ayant des attributs Loaded sont garde en tete, les blocs
			// sans attributs Loaded mais avec des attributs utiles aux derivation
			// sont gardes en fin de classe, les blocs sont attribut Loaded ou
			// utiles sont detruits avec leurs attributs

			// Recherche des attributs et blocs d'attribut a charger en memoire pour cette classe
			oaAttributesToDelete.SetSize(0);
			oaNeededAttributes.SetSize(0);
			oaNeededAttributeBlocks.SetSize(0);
			attributeBlock = NULL;
			attribute = kwcCurrentPhysicalClass->GetHeadAttribute();
			while (attribute != NULL)
			{
				attributeBlock = attribute->GetAttributeBlock();

				// Si attribut Loaded, on le laisse, sinon, on le retranche
				if (not attribute->GetLoaded())
				{
					// Cas d'un attribut hors bloc
					if (not attribute->IsInBlock())
					{
						// Memorisation si attribut necessaire pour le reintegrer en fin de
						// classe
						if (nkdAllNeededAttributes.Lookup(attribute) != NULL)
							oaNeededAttributes.Add(attribute);
						// Sinon, on enregistre l'attribut a detruire
						else
							oaAttributesToDelete.Add(attribute);
					}
					// Cas d'un attribut d'un bloc
					else
					{
						assert(attributeBlock != NULL);

						// Memorisation si attribut necessaire pour le reintegrer en fin de bloc
						if (nkdAllNeededAttributes.Lookup(attribute) != NULL)
						{
							oaNeededAttributes.Add(attribute);

							// Memorisation si bloc necessaire pour le reintegrer en fin de
							// classe
							if (oaNeededAttributeBlocks.GetSize() == 0 or
							    oaNeededAttributeBlocks.GetAt(
								oaNeededAttributeBlocks.GetSize() - 1) !=
								attributeBlock)
							{
								// Si le bloc n'est pas charge en memoire initialement,
								// on le memorise comme etant a deplacer
								if (nkdLoadedAttributeBlocks.Lookup(attributeBlock) ==
								    NULL)
									oaNeededAttributeBlocks.Add(attributeBlock);
							}
						}
						// Sinon, on enregistre l'attribut a detruire
						else
							oaAttributesToDelete.Add(attribute);
					}
				}

				// Passage a l'attribut suivant
				kwcCurrentPhysicalClass->GetNextAttribute(attribute);
			}
			assert(nkdLogicalyNeededClasses.Lookup(kwcCurrentPhysicalClass) != NULL or
			       kwcCurrentPhysicalClass->GetAttributeNumber() >=
				   oaAttributesToDelete.GetSize() + oaNeededAttributes.GetSize());

			// Destruction des attributs a detruire
			for (nAttribute = 0; nAttribute < oaAttributesToDelete.GetSize(); nAttribute++)
			{
				attribute = cast(KWAttribute*, oaAttributesToDelete.GetAt(nAttribute));

				// La destruction d'un attribut peut entrainer la destruction de son bloc englobant s'il
				// en est le dernier
				kwcCurrentPhysicalClass->DeleteAttribute(attribute->GetName());
			}

			// Deplacement des attributs necessaires en fin de classe pour le calcul des attributs charges
			// en memoire Dans les blocs, ce n'est pas necessaire, car ceux-ci sont base sur une indexation
			// base sur l'ordre des VarKey, et non pas des attributs dans le bloc
			for (nAttribute = 0; nAttribute < oaNeededAttributes.GetSize(); nAttribute++)
			{
				attribute = cast(KWAttribute*, oaNeededAttributes.GetAt(nAttribute));

				// On passe l'attribut en Used et Loaded
				attribute->SetUsed(true);
				attribute->SetLoaded(true);

				// Deplacement en fin de classe
				// Dans le cas des blocs, le calcul des reindexation des variables du bloc se fera a la
				// fin de la methode
				if (not attribute->IsInBlock())
					kwcCurrentPhysicalClass->MoveAttributeToClassTail(attribute);
			}

			// Deplacement des blocs d'attributs necessaires en fin de classe pour le calcul des attributs
			// charges en memoire
			for (nAttributeBlock = 0; nAttributeBlock < oaNeededAttributeBlocks.GetSize();
			     nAttributeBlock++)
			{
				attributeBlock =
				    cast(KWAttributeBlock*, oaNeededAttributeBlocks.GetAt(nAttributeBlock));

				// Deplacement du bloc en fin de classe
				kwcCurrentPhysicalClass->MoveAttributeBlockToClassTail(attributeBlock);
			}

			// Indexation de la classe
			kwcCurrentPhysicalClass->IndexClass();
			assert(nkdLogicalyNeededClasses.Lookup(kwcCurrentPhysicalClass) == NULL or
			       kwcCurrentPhysicalClass->GetAttributeNumber() ==
				   KWClassDomain::GetCurrentDomain()
					   ->LookupClass(kwcCurrentPhysicalClass->GetName())
					   ->GetLoadedAttributeNumber() +
				       oaNeededAttributes.GetSize());

			// Affichage de la classe
			if (bTrace)
			{
				cout << "Physical class\t" << kwcCurrentPhysicalClass->GetName()
				     << " (Used: " << kwcCurrentPhysicalClass->GetUsedAttributeNumber()
				     << ", Loaded: " << kwcCurrentPhysicalClass->GetLoadedAttributeNumber()
				     << ", Dense: " << kwcCurrentPhysicalClass->GetLoadedDenseAttributeNumber()
				     << ", Block: " << kwcCurrentPhysicalClass->GetLoadedAttributeBlockNumber()
				     << ", Relation: " << kwcCurrentPhysicalClass->GetLoadedRelationAttributeNumber()
				     << ")" << endl;

				// Recherche de la classe initiale correspondante
				kwcInitialClass =
				    KWClassDomain::GetCurrentDomain()->LookupClass(kwcCurrentPhysicalClass->GetName());

				// Affichage des caracteristiques de la classe physique
				attribute = kwcCurrentPhysicalClass->GetHeadAttribute();
				while (attribute != NULL)
				{
					initialAttribute = kwcInitialClass->LookupAttribute(attribute->GetName());
					check(initialAttribute);
					cout << "\t";
					if (attribute->GetUsed())
						cout << "U";
					if (attribute->GetLoaded())
						cout << "L";
					if (attribute->IsInBlock())
						cout << "B";
					if (not initialAttribute->GetLoaded())
						cout << "*";
					cout << "\t" << KWType::ToString(attribute->GetType());
					cout << "\t" << attribute->GetName();
					cout << "\t" << attribute->GetLoadIndex();
					cout << "\n";
					kwcCurrentPhysicalClass->GetNextAttribute(attribute);
				}
			}

			// Verification
			debug(bDebug = true);
			if (bDebug)
			{
				// Recherche de la classe initiale correspondante
				kwcInitialClass =
				    KWClassDomain::GetCurrentDomain()->LookupClass(kwcCurrentPhysicalClass->GetName());

				// Verification de la coherence entre classes initiales et classes physiques
				if (nkdLogicalyNeededClasses.Lookup(kwcCurrentPhysicalClass) != NULL)
				{
					// Les attributs initiaux Loaded doivent avoir meme index pour les classes
					// necessaires logiquement saif s'il sont dans des blocs
					for (nAttribute = 0; nAttribute < kwcInitialClass->GetLoadedAttributeNumber();
					     nAttribute++)
					{
						initialAttribute = kwcInitialClass->GetLoadedAttributeAt(nAttribute);
						attribute = kwcCurrentPhysicalClass->LookupAttribute(
						    initialAttribute->GetName());
						assert(attribute != NULL);
						assert(attribute->GetName() == initialAttribute->GetName());
						assert(attribute->GetLoadIndex().IsValid());
						assert(initialAttribute->GetLoadIndex().IsValid());
						assert(attribute->IsInBlock() or
						       attribute->GetLoadIndex() == initialAttribute->GetLoadIndex());
					}

					// Les attributs physique supplementaire ne doivent pas etre charges dans la
					// classe initiale
					assert(kwcInitialClass->GetLoadedAttributeNumber() <=
					       kwcCurrentPhysicalClass->GetLoadedAttributeNumber());
					assert(kwcInitialClass->GetAttributeNumber() >=
					       kwcCurrentPhysicalClass->GetAttributeNumber());
					assert(kwcCurrentPhysicalClass->GetAttributeNumber() ==
					       kwcCurrentPhysicalClass->GetLoadedAttributeNumber());
				}
			}
		}

		// On nettoie les classes non necessaires en supprimant leur attributs calcules
		// Cela permet d'eviter de compiler des classes ayant des attributs referencant
		// potentiellement des attributs detruits dans les clases necessaires
		// On ne les detruits pas pour qu'il reste autant de classes physiques que de classes logiques
		// ce qui simplifie la gestion de la coherence entre domaines logiques et physiques
		for (nClass = 0; nClass < kwcdPhysicalDomain->GetClassNumber(); nClass++)
		{
			kwcCurrentPhysicalClass = kwcdPhysicalDomain->GetClassAt(nClass);

			// Identification de la classe comme non necessaire
			if (nkdAllNeededClasses.Lookup(kwcCurrentPhysicalClass) == NULL)
			{
				// On supprime tous les attributs non natifs, pour couper les problemes de dependances
				// potentiels en indiquant que les donnees ne sont pas a charger
				attribute = kwcCurrentPhysicalClass->GetHeadAttribute();
				while (attribute != NULL)
				{
					// Memorisation de l'attribut comme attribut a detruire, si regle de derivation
					attributeToDelete = NULL;
					if (attribute->GetAnyDerivationRule() != NULL)
						attributeToDelete = attribute;

					// Passage a l'attribut suivant
					kwcCurrentPhysicalClass->GetNextAttribute(attribute);

					// Destruction si necessaire de l'attribut
					if (attributeToDelete != NULL)
						kwcCurrentPhysicalClass->DeleteAttribute(attributeToDelete->GetName());
				}

				// On passe egalement tous les attributs en Unused, ce qui a un impact notamment pour
				// les champs cle
				kwcCurrentPhysicalClass->SetAllAttributesUsed(false);
			}
		}

		// Affichage des domaines logique et physique
		if (bTrace)
		{
			cout << "\n=== Logical domain === \n" << *kwcClass->GetDomain() << endl;
			cout << "=== Physical domain === \n" << *kwcdPhysicalDomain << endl;
		}

		// Compilation du domaine
		kwcdPhysicalDomain->Compile();

		// Parcours des classes du domaine courant pour identifier les blocs d'attribut a reindexer
		if (bTrace)
			cout << "Compute loaded block attribute mutation indexes" << endl;
		for (nClass = 0; nClass < kwcdPhysicalDomain->GetClassNumber(); nClass++)
		{
			kwcCurrentPhysicalClass = kwcdPhysicalDomain->GetClassAt(nClass);

			// Recherche de la classe initiale correspondante
			kwcInitialClass = kwcClass->GetDomain()->LookupClass(kwcCurrentPhysicalClass->GetName());
			assert(kwcInitialClass != NULL);

			// Parcours des blocs d'attribut de la classe courante
			for (nAttributeBlock = 0;
			     nAttributeBlock < kwcCurrentPhysicalClass->GetLoadedAttributeBlockNumber();
			     nAttributeBlock++)
			{
				attributeBlock = kwcCurrentPhysicalClass->GetLoadedAttributeBlockAt(nAttributeBlock);
				initialAttributeBlock =
				    kwcInitialClass->LookupAttributeBlock(attributeBlock->GetName());

				// Verification de coherence
				// Le nombre d'attributs charges du bloc physique peut etre plus grand que celui du bloc
				// initial, s'il faut lire des attribut physiques non utilise, mais necessaire pour
				// effectuer des calculs Attention, le nombre d'attributs charges du bloc physique peut
				// aussi plus petit que celui du bloc initial, s'il n'est pas necessaire de lire un
				// attribut physique, mais que l'attribut initial correspondant est utilise dans une
				// sous classe non utilisee
				assert(initialAttributeBlock != NULL);
				assert(initialAttributeBlock->GetAttributeNumber() >=
				       attributeBlock->GetLoadedAttributeNumber());

				// Calcul des index de mutation du bloc
				attributeBlock->ComputeLoadedAttributeMutationIndexes(initialAttributeBlock);

				// Affichage des index de mutation
				if (bTrace)
				{
					if (attributeBlock->GetLoadedAttributeMutationIndexes() != NULL)
					{
						for (nAttribute = 0;
						     nAttribute < attributeBlock->GetLoadedAttributeNumber();
						     nAttribute++)
						{
							attribute = attributeBlock->GetLoadedAttributeAt(nAttribute);
							cout << kwcCurrentPhysicalClass->GetName() << "\t";
							cout << attributeBlock->GetName() << "\t";
							cout << attribute->GetName() << "\t";
							cout << nAttribute << "\t";
							cout << attributeBlock->GetLoadedAttributeMutationIndexes()
								    ->GetAt(nAttribute)
							     << endl;
						}
					}
				}
			}
		}
	}

	// Parametrage des classes pour gerer les elements de donnees a calculer
	assert(not bPhysicalDomainNeeded or kwcdPhysicalDomain != kwcClass->GetDomain());
	for (nClass = 0; nClass < kwcdPhysicalDomain->GetClassNumber(); nClass++)
	{
		kwcCurrentPhysicalClass = kwcdPhysicalDomain->GetClassAt(nClass);

		// Reinitialisation du parametrage
		kwcCurrentPhysicalClass->GetDatabaseDataItemsToCompute()->SetSize(0);
		kwcCurrentPhysicalClass->GetDatabaseTemporayDataItemsToComputeAndClean()->SetSize(0);

		// Recherche de la classe initiale correspondante
		kwcInitialClass = kwcClass->GetDomain()->LookupClass(kwcCurrentPhysicalClass->GetName());
		assert(kwcInitialClass != NULL);

		// Affichage
		if (bTrace)
			cout << "Specify data items to compute\t" << kwcCurrentPhysicalClass->GetName() << "\n";

		// Parcours des elements de donnees de la classe physique pour determiner ceux qui sont a calculer
		for (nAttribute = 0; nAttribute < kwcCurrentPhysicalClass->GetLoadedDataItemNumber(); nAttribute++)
		{
			dataItem = kwcCurrentPhysicalClass->GetLoadedDataItemAt(nAttribute);
			if (dataItem->IsAttribute())
			{
				attribute = cast(KWAttribute*, dataItem);

				// Prise en compte des attributs a calculer
				if (attribute->GetDerivationRule() != NULL)
				{
					// Ajout dans les attributs a calculer
					kwcCurrentPhysicalClass->GetDatabaseDataItemsToCompute()->Add(dataItem);

					// Ajout dans les attributs temporaires nettoyable si absent de la classe
					// logique et dont le nettoyage peut liberer significativement de la memoire
					initialAttribute = kwcInitialClass->LookupAttribute(attribute->GetName());
					check(initialAttribute);
					if (not initialAttribute->GetLoaded() and
					    (attribute->GetType() == KWType::Symbol or
					     attribute->GetType() == KWType::Text or
					     attribute->GetType() == KWType::TextList or
					     attribute->GetType() == KWType::ObjectArray))
						kwcCurrentPhysicalClass->GetDatabaseTemporayDataItemsToComputeAndClean()
						    ->Add(dataItem);

					// Affichage
					if (bTrace)
					{
						cout << "\t", cout << KWType::ToString(attribute->GetType()) << "\t";
						cout << attribute->GetName() << "\t";
						cout << initialAttribute->GetLoaded() << "\n";
					}
				}
				// Prise en compte des attribut de type relation natif, pour propagation des calcul
				else if (KWType::IsRelation(attribute->GetType()))
				{
					// Ajout dans les attributs a calculer
					kwcCurrentPhysicalClass->GetDatabaseDataItemsToCompute()->Add(dataItem);
				}
			}
			else
			{
				attributeBlock = cast(KWAttributeBlock*, dataItem);

				// Prise en compte des blocs d'attributs a calculer
				if (attributeBlock->GetDerivationRule() != NULL)
				{
					// Ajout dans les blocs d'attribut a calculer
					kwcCurrentPhysicalClass->GetDatabaseDataItemsToCompute()->Add(dataItem);

					// Ajout dans les attributs temporaires nettoyable si absent de la classe
					// logique et dont le nettoyage peut liberer significativement de la memoire
					initialAttributeBlock =
					    kwcInitialClass->LookupAttributeBlock(attributeBlock->GetName());
					check(initialAttributeBlock);
					if (not initialAttributeBlock->GetLoaded())
						kwcCurrentPhysicalClass->GetDatabaseTemporayDataItemsToComputeAndClean()
						    ->Add(dataItem);

					// Affichage
					if (bTrace)
					{
						cout << "\t",
						    cout << KWType::ToString(attributeBlock->GetType()) << "\t";
						cout << attributeBlock->GetName() << "\t";
						cout << initialAttributeBlock->GetLoaded() << "\n";
					}
				}
			}
		}
	}

	// Calcul des attributs natifs non utilises a garder pour gerer correctement la mutation des objets
	ComputeUnusedNativeAttributesToKeep(&nkdUnusedNativeAttributesToKeep);

	ensure(kwcPhysicalClass != NULL);
	ensure(kwcPhysicalClass->GetName() == GetClassName());
	ensure(kwcPhysicalClass->Check());
	ensure(kwcPhysicalClass->GetLoadedAttributeNumber() >= kwcClass->GetLoadedAttributeNumber());
}

void KWDatabase::ComputeUnusedNativeAttributesToKeep(NumericKeyDictionary* nkdAttributes)
{
	const boolean bTrace = false;
	ObjectArray oaImpactedClasses;
	NumericKeyDictionary nkdImpactedClasses;
	KWClass* kwcCurrentClass;
	int nClass;
	KWAttribute* attribute;
	int nAttribute;
	ObjectArray oaAttributesToKeep;
	NumericKeyDictionary nkdAnalysedRules;

	require(kwcClass != NULL);
	require(nkdAttributes != NULL);
	require(nkdAttributes->GetCount() == 0);

	// Initialisation des classes a traiter avec la classe principale
	oaImpactedClasses.Add(kwcClass);
	nkdImpactedClasses.SetAt(kwcClass, kwcClass);

	// Analyse des classes impactees, en ajoutant au fur et a mesure de nouvelles classes
	// pour la composition des attributs natifs, et en analysant les regles de derivation
	// pour determiner les attributs natifs referencees
	// Ainsi, toute les classes utilisees recursivement sont ici prises en compte
	for (nClass = 0; nClass < oaImpactedClasses.GetSize(); nClass++)
	{
		kwcCurrentClass = cast(KWClass*, oaImpactedClasses.GetAt(nClass));
		assert(kwcCurrentClass->IsCompiled());

		// Parcours des attributs utilises
		for (nAttribute = 0; nAttribute < kwcCurrentClass->GetLoadedAttributeNumber(); nAttribute++)
		{
			attribute = kwcCurrentClass->GetLoadedAttributeAt(nAttribute);

			// Traitement si attribut de type Object (dans un bloc ou non)
			if (KWType::IsRelation(attribute->GetType()))
			{
				// Analyse si retourne par une regle de derivation
				// On identifie ainsi les attributs Object natifs potentiellement references
				if (attribute->GetAnyDerivationRule() != NULL)
					ComputeUnusedNativeAttributesToKeepForRule(nkdAttributes, &nkdAnalysedRules,
										   attribute->GetAnyDerivationRule());

				// Ajout de la classe parmi les classes a analyser
				if (nkdImpactedClasses.Lookup(attribute->GetClass()) == NULL)
				{
					oaImpactedClasses.Add(attribute->GetClass());
					nkdImpactedClasses.SetAt(attribute->GetClass(), attribute->GetClass());
				}
			}
		}
	}

	// Affichage des attributs a garder
	if (bTrace)
	{
		// Export des attrribut a garder
		nkdAttributes->ExportObjectArray(&oaAttributesToKeep);
		oaAttributesToKeep.SetCompareFunction(KWAttributeCompareName);
		oaAttributesToKeep.Sort();

		// Affichage
		cout << "Unused native attributes to keep\n";
		for (nAttribute = 0; nAttribute < oaAttributesToKeep.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, oaAttributesToKeep.GetAt(nAttribute));
			cout << "\t" << attribute->GetClass()->GetName() << "\t" << attribute->GetName() << "\n";
		}
	}
}

void KWDatabase::ComputeUnusedNativeAttributesToKeepForRule(NumericKeyDictionary* nkdAttributes,
							    NumericKeyDictionary* nkdAnalysedRules,
							    KWDerivationRule* rule)
{
	KWDerivationRuleOperand* operand;
	int nOperand;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;

	require(kwcClass != NULL);
	require(nkdAttributes != NULL);
	require(nkdAnalysedRules != NULL);
	require(rule != NULL);
	require(KWType::IsGeneralRelation(rule->GetType()));

	// Arret si regle deja analysee
	if (nkdAnalysedRules->Lookup(rule) != NULL)
		return;
	// Sinon, on commence par memoriser la regle analysee
	else
		nkdAnalysedRules->SetAt(rule, rule);

	// Parcours des operandes de la regle
	for (nOperand = 0; nOperand < rule->GetOperandNumber(); nOperand++)
	{
		operand = rule->GetOperandAt(nOperand);

		// Cas d'un type non bloc
		if (not KWType::IsValueBlock(operand->GetType()))
		{
			// On considere l'operande s'il s'agit d'un objet du meme type que celui de la regle
			// Il n'est pas utile de poursuivre l'analyse recursive des operandes si l'on a pas le bon type
			// de relation
			if (KWType::IsRelation(operand->GetType()) and
			    operand->GetObjectClassName() == rule->GetObjectClassName())
			{
				// Propagation si regle
				if (operand->GetDerivationRule() != NULL)
				{
					assert(not KWType::IsValueBlock(operand->GetType()));
					ComputeUnusedNativeAttributesToKeepForRule(nkdAttributes, nkdAnalysedRules,
										   operand->GetDerivationRule());
				}
				// Traitement de l'attribut sinon
				else
				{
					attribute = operand->GetOriginAttribute();
					assert(attribute->GetParentClass()->GetDomain() == kwcClass->GetDomain());

					// Propagation si regle
					if (attribute->GetAnyDerivationRule() != NULL)
						ComputeUnusedNativeAttributesToKeepForRule(
						    nkdAttributes, nkdAnalysedRules, attribute->GetAnyDerivationRule());

					// Memorisation de l'attribut natif ou cree sinon, s'il n'est pas utilise
					if (not attribute->IsInBlock() and not attribute->GetReference() and
					    not attribute->GetUsed())
						nkdAttributes->SetAt(attribute, attribute);
				}
			}
		}
		// Cas d'un type bloc
		else
		{
			// On verifie qu'un operande de type block ne peut etre renvoye par une regle
			assert(not(operand->GetOrigin() == KWDerivationRuleOperand::OriginRule));

			// On considere l'operande s'il s'agit d'un bloc d'objets du meme type que celui de la regle
			if (operand->GetType() == KWType::ObjectArrayValueBlock and
			    operand->GetObjectClassName() == rule->GetObjectClassName())
			{
				// Traitement du bloc d'attributs
				attributeBlock = operand->GetOriginAttributeBlock();
				assert(attributeBlock->GetParentClass()->GetDomain() == kwcClass->GetDomain());

				// Propagation a la regle: un bloc d'attributs de type relation ne peut qu'etre calcule
				assert(attributeBlock->GetDerivationRule() != NULL);
				ComputeUnusedNativeAttributesToKeepForRule(nkdAttributes, nkdAnalysedRules,
									   attributeBlock->GetDerivationRule());
			}
		}
	}
}

void KWDatabase::DeletePhysicalClass()
{
	int nClass;
	KWClass* kwcCurrentClass;

	// Destruction de la classe physique et de son domaine si necessaire
	if (kwcPhysicalClass != NULL and kwcPhysicalClass != kwcClass)
		delete kwcPhysicalClass->GetDomain();
	kwcPhysicalClass = NULL;

	// Nettoyage des specifications d'attributs a calculer
	for (nClass = 0; nClass < kwcClass->GetDomain()->GetClassNumber(); nClass++)
	{
		kwcCurrentClass = kwcClass->GetDomain()->GetClassAt(nClass);
		kwcCurrentClass->GetDatabaseDataItemsToCompute()->SetSize(0);
		kwcCurrentClass->GetDatabaseTemporayDataItemsToComputeAndClean()->SetSize(0);
	}

	// Nettoyage du dictionnaire des attribut natif non utilises a detruire
	nkdUnusedNativeAttributesToKeep.RemoveAll();
}

void KWDatabase::MutatePhysicalObject(KWObject* kwoPhysicalObject) const
{
	KWObjectKey objectKey;

	require(kwcPhysicalClass != NULL);
	require(kwcPhysicalClass->GetName() == GetClassName());
	require(kwcPhysicalClass->IsCompiled());
	require(kwoPhysicalObject != NULL);
	require(kwoPhysicalObject->GetClass() == kwcPhysicalClass);
	require(kwcPhysicalClass->GetLoadedAttributeNumber() >= kwcClass->GetLoadedAttributeNumber());

	// Calcul de toutes les valeurs a transferer
	kwoPhysicalObject->ComputeAllValues(&memoryGuard);

	// Memorisation de la cle de l'objet en cas de depassement memoire
	// Il faut le faire sur l'objet physique avant sa mutation, car il contient
	// necessairement les champs de la cle en multi-table, alors qu'ils sont potentiellement
	// absent de l'objet logique si les champs de la cle sont en unused
	if (memoryGuard.IsSingleInstanceMemoryLimitReached() or memoryGuard.IsMaxSecondaryRecordNumberReached())
	{
		// On se base sur la cle de l'objet s'il y en a une
		if (kwoPhysicalObject->GetClass()->IsKeyLoaded())
		{
			objectKey.InitializeFromObject(kwoPhysicalObject);
			memoryGuard.SetMainObjectKey(objectKey.GetObjectLabel());
		}

		kwoPhysicalObject->GetObjectLabel();
	}

	// Mutation si necessaire
	if (kwcPhysicalClass != kwcClass)
		kwoPhysicalObject->Mutate(kwcClass, &nkdUnusedNativeAttributesToKeep);
	ensure(kwoPhysicalObject->GetClass() == kwcClass);
	ensure(kwoPhysicalObject->Check());
}

boolean KWDatabase::CheckSelectionValue(const ALString& sValue) const
{
	boolean bOk = true;
	KWClass* kwcSelectionClass;
	KWAttribute* selectionAttribute;
	int nConversionError;
	Continuous cValue;

	// Si pas d'attribut de selection specifie: OK
	if (GetSelectionAttribute() == "")
		return true;

	// Recherche de la classe
	if (GetClassName() == "")
	{
		bOk = false;
		AddError("Dictionary not specified");
	}
	kwcSelectionClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	if (GetClassName() != "" and kwcSelectionClass == NULL)
	{
		bOk = false;
		AddError("Unknown dictionary " + GetClassName());
	}

	// Recherche de l'attribut de selection
	if (bOk)
	{
		selectionAttribute = kwcSelectionClass->LookupAttribute(GetSelectionAttribute());

		// Test d'existence de l'attribut
		if (selectionAttribute == NULL)
		{
			bOk = false;
			AddError("Unknown selection variable " + GetSelectionAttribute());
		}
		// Test du type de l'attribut
		else if (not KWType::IsSimple(selectionAttribute->GetType()))
		{
			bOk = false;
			AddError("Selection variable " + GetSelectionAttribute() +
				 " must be of type numerical or categorical");
		}

		// Test de validite du type de la valeur
		if (bOk and selectionAttribute->GetType() == KWType::Continuous)
		{
			nConversionError = KWContinuous::StringToContinuousError(sValue, cValue);
			if (nConversionError != KWContinuous::NoError)
			{
				bOk = false;
				AddError("Incorrect selection value \"" + sValue +
					 "\": " + KWContinuous::ErrorLabel(nConversionError));
			}
		}
	}
	return bOk;
}

void KWDatabase::ResetMemoryGuard()
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	memoryGuard.Reset();
}

void KWDatabase::SetMemoryGuardMaxSecondaryRecordNumber(longint lValue)
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	memoryGuard.SetMaxSecondaryRecordNumber(lValue);
}

longint KWDatabase::GetMemoryGuardMaxSecondaryRecordNumber() const
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	return memoryGuard.GetMaxSecondaryRecordNumber();
}

void KWDatabase::SetMemoryGuardSingleInstanceMemoryLimit(longint lValue)
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	memoryGuard.SetSingleInstanceMemoryLimit(lValue);
}

longint KWDatabase::GetMemoryGuardSingleInstanceMemoryLimit() const
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	return memoryGuard.GetSingleInstanceMemoryLimit();
}

boolean KWDatabase::Check() const
{
	return CheckPartially(false);
}

boolean KWDatabase::CheckPartially(boolean bWriteOnly) const
{
	boolean bOk = true;
	KWClass* kwcSelectionClass;

	// Recherche de la classe
	if (GetClassName() == "")
	{
		bOk = false;
		AddError("Dictionary not specified");
	}
	kwcSelectionClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	if (GetClassName() != "" and kwcSelectionClass == NULL)
	{
		bOk = false;
		AddError("Unknown dictionary " + GetClassName());
	}

	// La classe doit etre valide
	if (bOk and not kwcSelectionClass->IsCompiled())
	{
		bOk = false;
		AddError("Incorrect dictionary " + GetClassName());
	}

	// Verification de la valeur de selection
	if (not CheckSelectionValue(GetSelectionValue()))
		bOk = false;

	// Verification du format
	if (not CheckFormat())
		bOk = false;

	return bOk;
}

boolean KWDatabase::CheckFormat() const
{
	return true;
}

void KWDatabase::CompilePhysicalSelection()
{
	KWAttribute* selectionAttribute;

	require(kwcPhysicalClass != NULL);
	require(kwcPhysicalClass->GetName() == GetClassName());
	require(kwcPhysicalClass->IsCompiled());

	// Reinitialisation
	liSelectionAttributeLoadIndex.Reset();
	nSelectionAttributeType = KWType::Unknown;
	sSelectionSymbol = "";
	cSelectionContinuous = 0;

	// Recherche de l'attribut de selection
	selectionAttribute = kwcPhysicalClass->LookupAttribute(GetSelectionAttribute());

	// Compilation des infos de selection
	if (selectionAttribute != NULL)
	{
		assert(selectionAttribute->GetUsed());
		assert(selectionAttribute->GetLoaded());
		liSelectionAttributeLoadIndex = selectionAttribute->GetLoadIndex();
		nSelectionAttributeType = selectionAttribute->GetType();
		if (nSelectionAttributeType == KWType::Symbol)
			sSelectionSymbol = GetSelectionValue();
		else if (nSelectionAttributeType == KWType::Continuous)
			cSelectionContinuous = KWContinuous::StringToContinuous(GetSelectionValue());
	}
}

boolean KWDatabase::IsPhysicalObjectSelected(KWObject* kwoPhysicalObject)
{
	require(kwcPhysicalClass != NULL);
	require(kwcPhysicalClass->GetName() == GetClassName());
	require(kwcPhysicalClass->IsCompiled());
	require(kwoPhysicalObject != NULL);
	require(kwoPhysicalObject->GetClass() == kwcPhysicalClass);

	if (not liSelectionAttributeLoadIndex.IsValid())
		return true;
	else if (nSelectionAttributeType == KWType::Symbol)
		return kwoPhysicalObject->ComputeSymbolValueAt(liSelectionAttributeLoadIndex) == sSelectionSymbol;
	else if (nSelectionAttributeType == KWType::Continuous)
		return kwoPhysicalObject->ComputeContinuousValueAt(liSelectionAttributeLoadIndex) ==
		       cSelectionContinuous;
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////

boolean KWDatabase::BuildDatabaseClass(KWClass* kwcDatabaseClass)
{
	require(kwcDatabaseClass != NULL);
	return false;
}

boolean KWDatabase::IsTypeInitializationManaged() const
{
	return false;
}

boolean KWDatabase::PhysicalOpenForRead()
{
	return true;
}

boolean KWDatabase::PhysicalOpenForWrite()
{
	return true;
}

boolean KWDatabase::IsPhysicalEnd() const
{
	return true;
}

KWObject* KWDatabase::PhysicalRead()
{
	return NULL;
}

void KWDatabase::PhysicalSkip() {}

void KWDatabase::PhysicalWrite(const KWObject* kwoObject)
{
	require(kwoObject != NULL);
}

boolean KWDatabase::PhysicalClose()
{
	return true;
}

void KWDatabase::PhysicalDeleteDatabase() {}

longint KWDatabase::GetPhysicalEstimatedObjectNumber()
{
	return 0;
}

double KWDatabase::GetPhysicalReadPercentage()
{
	return 0;
}

longint KWDatabase::GetPhysicalRecordIndex() const
{
	return 0;
}

void KWDatabase::CollectPhysicalStatsMessages(ObjectArray* oaPhysicalMessages) {}

void KWDatabase::DisplayPhysicalMessages(ObjectArray* oaPhysicalMessages)
{
	int i;
	Error* errorMessage;

	require(oaPhysicalMessages != NULL);

	for (i = 0; i < oaPhysicalMessages->GetSize(); i++)
	{
		errorMessage = cast(Error*, oaPhysicalMessages->GetAt(i));
		AddSimpleMessage(errorMessage->GetLabel());
	}
}

///////////////////////////////////////////////
// Administration des objets KWDatabase

void KWDatabase::SetDefaultTechnologyName(const ALString& sTechnologyName)
{
	if (sDefaultTechnologyName == NULL)
		sDefaultTechnologyName = new ALString;
	*sDefaultTechnologyName = sTechnologyName;
}

const ALString KWDatabase::GetDefaultTechnologyName()
{
	if (sDefaultTechnologyName == NULL)
		return "";
	else
		return *sDefaultTechnologyName;
}

KWDatabase* KWDatabase::CreateDefaultDatabaseTechnology()
{
	require(GetDefaultTechnologyName() != "");
	require(LookupDatabaseTechnology(GetDefaultTechnologyName()) != NULL);

	return CloneDatabaseTechnology(GetDefaultTechnologyName());
}

void KWDatabase::RegisterDatabaseTechnology(KWDatabase* database)
{
	require(database != NULL);
	require(database->GetTechnologyName() != "");
	require(odDatabaseTechnologies == NULL or
		odDatabaseTechnologies->Lookup(database->GetTechnologyName()) == NULL);

	// Creation si necessaire du dictionnaire de bases
	if (odDatabaseTechnologies == NULL)
		odDatabaseTechnologies = new ObjectDictionary;

	// Remplacement par la nouvelle base
	odDatabaseTechnologies->SetAt(database->GetTechnologyName(), database);
}

KWDatabase* KWDatabase::LookupDatabaseTechnology(const ALString& sTechnologyName)
{
	require(sTechnologyName != "");

	// Creation si necessaire du dictionnaire de bases
	if (odDatabaseTechnologies == NULL)
		odDatabaseTechnologies = new ObjectDictionary;

	return cast(KWDatabase*, odDatabaseTechnologies->Lookup(sTechnologyName));
}

KWDatabase* KWDatabase::CloneDatabaseTechnology(const ALString& sTechnologyName)
{
	KWDatabase* referenceDatabase;
	require(sTechnologyName != "");

	// Creation si necessaire du dictionnaire de bases
	if (odDatabaseTechnologies == NULL)
		odDatabaseTechnologies = new ObjectDictionary;

	// Recherche d'une base de meme nom
	referenceDatabase = cast(KWDatabase*, odDatabaseTechnologies->Lookup(sTechnologyName));

	// Retour de son Clone si possible
	if (referenceDatabase == NULL)
		return NULL;
	else
		return referenceDatabase->Clone();
}

void KWDatabase::ExportAllDatabaseTechnologies(ObjectArray* oaDatabases)
{
	require(oaDatabases != NULL);

	// Creation si necessaire du dictionnaire de bases
	if (odDatabaseTechnologies == NULL)
		odDatabaseTechnologies = new ObjectDictionary;

	odDatabaseTechnologies->ExportObjectArray(oaDatabases);
}

void KWDatabase::DeleteAllDatabaseTechnologies()
{
	if (odDatabaseTechnologies != NULL)
	{
		odDatabaseTechnologies->DeleteAll();
		delete odDatabaseTechnologies;
		odDatabaseTechnologies = NULL;
	}
	if (sDefaultTechnologyName != NULL)
	{
		delete sDefaultTechnologyName;
		sDefaultTechnologyName = NULL;
	}
	ensure(odDatabaseTechnologies == NULL);
	ensure(sDefaultTechnologyName == NULL);
}

ALString* KWDatabase::sDefaultTechnologyName = NULL;
ObjectDictionary* KWDatabase::odDatabaseTechnologies = NULL;
