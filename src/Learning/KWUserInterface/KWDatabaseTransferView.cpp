// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseTransferView.h"

KWDatabaseTransferView::KWDatabaseTransferView()
{
	KWSTDatabaseTextFileView refSTDatabaseTextFileView;
	KWMTDatabaseTextFileView refMTDatabaseTextFileView;

	// Creation des bases de facon generique
	sourceDatabase = KWDatabase::CreateDefaultDatabaseTechnology();
	targetDatabase = KWDatabase::CreateDefaultDatabaseTechnology();

	// Parametrage general
	SetIdentifier("KWDatabaseTransfer");
	SetLabel("Deploy model");

	// Ajout de l'action de transfert de base
	AddAction("TransferDatabase", "Deploy model", (ActionMethod)(&KWDatabaseTransferView::TransferDatabase));
	GetActionAt("TransferDatabase")->SetStyle("Button");

	// Ajout de l'action de construction d'un dictionnaire de la base transferee
	AddAction("BuildTransferredClass", "Build deployed dictionary...",
		  (ActionMethod)(&KWDatabaseTransferView::BuildTransferredClass));
	GetActionAt("BuildTransferredClass")->SetStyle("Button");

	// Ajout d'un champ de saisie du nom du dictionnaire
	AddStringField("ClassName", "Deployment dictionary", "");

	// Ajout du parametrage de la base d'origine, creee de facon generique
	sourceDatabaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();
	sourceDatabaseView->SetObject(sourceDatabase);
	AddCardField("SourceDatabase", "Input database", sourceDatabaseView);

	// Le champ dictionnaire est rendu invisible
	sourceDatabaseView->GetFieldAt("ClassName")->SetVisible(false);

	// Specialisation du parametrage des listes d'aide de la base
	sourceDatabaseView->SetHelpListViewPath("SourceDatabase");

	// Ajout du parametrage de la base destination, creee de facon generique
	targetDatabaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();
	targetDatabaseView->ToWriteOnlyMode();
	targetDatabaseView->SetObject(targetDatabase);
	AddCardField("TargetDatabase", "Output database", targetDatabaseView);

	// Ajout d'un champ de saisie dans la fiche de la base en sortie pour choisir
	// le format tabular (dense uniquement) ou sparse (tabular etendu avec des blocs sparse)
	targetDatabaseView->AddStringField("OutputFormat", "Output format", "tabular");

	// On indique que le champ de parametrage du dictionnaire declenche une action de rafraichissement
	// de l'interface immediatement apres une mise a jour, pour pouvoir rafraichir les mapping des databases
	cast(UIElement*, GetFieldAt("ClassName"))->SetTriggerRefresh(true);

	// Parametrage de liste d'aide l'attribut de format de sortie
	targetDatabaseView->GetFieldAt("OutputFormat")->SetStyle("ComboBox");
	targetDatabaseView->GetFieldAt("OutputFormat")->SetParameters("tabular\nsparse");

	// Info-bulles
	GetFieldAt("ClassName")->SetHelpText("Dictionary used to select or derive new variables.");
	targetDatabaseView->GetFieldAt("OutputFormat")
	    ->SetHelpText("Output format :"
			  "\n - tabular: standard tabular format"
			  "\n - sparse: extended tabular format, with sparse fields in case of blocks of variables");
	GetActionAt("TransferDatabase")
	    ->SetHelpText(
		"Deploy model."
		"\n This action reads the input data, applies the deployment dictionary to select"
		"\n all or part of the variables and add derived variables, and writes the output data."
		"\n This action can be used to generate a data preparation file, containing the recoded variables."
		"\n It also can be used to deploy a scoring model, owing to the prediction variables"
		"\n contained in the predictor dictionaries.");
	GetActionAt("BuildTransferredClass")
	    ->SetHelpText(
		"Build deployed dictionary."
		"\n This action creates an output dictionary that enables to read and analyze the deployed database :"
		"\n it contains the deployed variables only, without any derivation rule in the dictionary.");

	// Short cuts
	GetActionAt("TransferDatabase")->SetShortCut('D');
	GetActionAt("BuildTransferredClass")->SetShortCut('B');
}

KWDatabaseTransferView::~KWDatabaseTransferView()
{
	// Destruction des bases
	delete sourceDatabase;
	delete targetDatabase;
}

void KWDatabaseTransferView::InitializeSourceDatabase(KWDatabase* database, const ALString& sDatabaseClassFileName)
{
	KWDatabase* defaultDatabase;
	KWDatabase* initializationDatabase;
	ALString sTargetDatabaseName;
	const ALString sPrefix = "D_";
	ALString sPathName;
	ALString sFileName;
	KWMTDatabase* targetMTDatabase;
	KWMTDatabaseMapping* mapping;
	int nMapping;

	require(database != NULL);

	// Memorisation du fichier dictionnaire
	sClassFileName = sDatabaseClassFileName;

	// Creation generique d'une base par defaut
	defaultDatabase = KWDatabase::CreateDefaultDatabaseTechnology();

	// Parametrage du nom du dictionnaire
	sClassName = database->GetClassName();
	if (KWClassDomain::GetCurrentDomain()->LookupClass(sClassName) == NULL)
		sClassName = "";

	// Choix de la base permettant l'initialisation des donnees
	if (database == NULL)
		initializationDatabase = defaultDatabase;
	else
		initializationDatabase = database;
	initializationDatabase->SetClassName(sClassName);

	// Parametrage de la base source a partir de la base d'initialisation
	sourceDatabase->CopyFrom(initializationDatabase);

	// Parametrage de la base cible a partir de la base source
	targetDatabase->CopyFrom(initializationDatabase);

	// Dans le cas multi-tables, les tables externes ne doivent pas etre specifiees
	if (targetDatabase->IsMultiTableTechnology())
	{
		// Nettoyage des mapping des tables externes
		targetMTDatabase = cast(KWMTDatabase*, targetDatabase);
		for (nMapping = 0; nMapping < targetMTDatabase->GetMultiTableMappings()->GetSize(); nMapping++)
		{
			mapping =
			    cast(KWMTDatabaseMapping*, targetMTDatabase->GetMultiTableMappings()->GetAt(nMapping));
			if (targetMTDatabase->IsReferencedClassMapping(mapping))
				mapping->SetDataTableName("");
		}
	}

	// On reinitialise le parametrage lies a l'echantillonage et a la selection
	sourceDatabase->InitializeSamplingAndSelection();
	targetDatabase->InitializeSamplingAndSelection();

	// Nom par defaut de la base de donnee cible
	targetDatabase->AddPrefixToUsedFiles(sPrefix);

	// Nettoyage
	delete defaultDatabase;
}

void KWDatabaseTransferView::Open()
{
	KWClass* kwcClass;
	ALString sClassNames;
	ALString sNonStorableClassNames;
	const int nMaxTotalShownLineNumber = 12;
	int nMaxInputNativeRelationAttributeNumber;
	int nMaxOutputNativeRelationAttributeNumber;
	int i;

	// Warning s'il n'y a pas de dictionnaire
	if (KWClassDomain::GetCurrentDomain()->GetClassNumber() == 0)
		AddWarning("No available dictionary");

	// Collecte de la liste des dictionnaires disponibles et de stats
	// sur la taille de leur mapping, selon leur caractere stockable ou non
	nMaxInputNativeRelationAttributeNumber = 0;
	nMaxOutputNativeRelationAttributeNumber = 0;
	for (i = 0; i < KWClassDomain::GetCurrentDomain()->GetClassNumber(); i++)
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->GetClassAt(i);

		// Ajout du nom de la classe dans la liste d'aide a la saisie
		if (kwcClass->IsKeyBasedStorable())
		{
			if (sClassNames.GetLength() > 0)
				sClassNames += "\n";
			sClassNames += kwcClass->GetName();
		}
		else
		{
			if (sNonStorableClassNames.GetLength() > 0)
				sNonStorableClassNames += "\n";
			sNonStorableClassNames += kwcClass->GetName();
		}

		// Mise a jour des nombre max de mappings
		if (targetDatabaseView->IsMultiTableTechnology())
		{
			nMaxInputNativeRelationAttributeNumber =
			    max(nMaxInputNativeRelationAttributeNumber,
				kwcClass->ComputeOverallNativeRelationAttributeNumber(true));
			nMaxOutputNativeRelationAttributeNumber =
			    max(nMaxOutputNativeRelationAttributeNumber,
				kwcClass->ComputeOverallNativeRelationAttributeNumber(false));
		}
	}

	// Les dictionnaires non stockables sont ranges en derniers, apres une ligne blanche
	if (sNonStorableClassNames.GetLength() > 0)
		sClassNames += "\n\n" + sNonStorableClassNames;

	// Parametrage du champ de saisie des dictionnaires en style ComboBox,
	// avec la liste des dictionnaires en cours
	SetStringValueAt("ClassName", sClassName);
	GetFieldAt("ClassName")->SetStyle("EditableComboBox");
	GetFieldAt("ClassName")->SetParameters(sClassNames);

	// Parametrage des tailles des liste de fichier des mapping en entree et en sortie dans le cas multi-table
	if (targetDatabaseView->IsMultiTableTechnology())
	{
		// Cas ou on depasse le max de ce qui est affichable
		if (nMaxInputNativeRelationAttributeNumber + nMaxOutputNativeRelationAttributeNumber >
		    nMaxTotalShownLineNumber)
		{
			nMaxOutputNativeRelationAttributeNumber =
			    min(nMaxOutputNativeRelationAttributeNumber, nMaxTotalShownLineNumber / 2);
			nMaxInputNativeRelationAttributeNumber =
			    nMaxTotalShownLineNumber - nMaxOutputNativeRelationAttributeNumber;
		}

		// On limite le nombre de ligne de mapping
		sourceDatabaseView->SetEditableTableNumber(1 + nMaxInputNativeRelationAttributeNumber);
		targetDatabaseView->SetEditableTableNumber(1 + nMaxOutputNativeRelationAttributeNumber);
	}

	// Appel de la methode ancetre pour l'ouverture
	UICard::Open();
	sClassName = GetStringValueAt("ClassName");
}

void KWDatabaseTransferView::TransferDatabase()
{
	boolean bOk = true;
	ObjectArray oaSourceDatabaseFileSpecs;
	ObjectArray oaTargetDatabaseFileSpecs;
	ALString sOutputPathName;
	int nRef;
	int nTarget;
	FileSpec* specRef;
	FileSpec* specTarget;
	KWClass* transferClass;
	ALString sTmp;
	KWDatabase* workingTargetDatabase;
	KWDatabaseTransferTask transferTask;
	longint lRecordNumber;

	// Verification du directory des fichiers temporaires
	bOk = FileService::CreateApplicationTmpDir();

	// Memorisation des donnees modifies (non geres par les View)
	sClassName = GetStringValueAt("ClassName");
	sourceDatabase->SetClassName(sClassName);
	targetDatabase->SetClassName(sClassName);

	// La classe doit etre valide
	transferClass = NULL;
	if (sClassName != "")
		transferClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);
	if (bOk and transferClass == NULL)
	{
		bOk = false;
		AddError("Deployment dictionary " + sClassName + " does not exist");
	}
	if (bOk and not transferClass->IsCompiled())
	{
		bOk = false;
		AddError("Incorrect deployment dictionary " + sClassName);
	}

	// Le nom de la base source doit etre renseigne
	if (bOk and sourceDatabase->GetDatabaseName() == "")
	{
		bOk = false;
		AddError("Missing input database name");
	}

	// Le nom de la base cible doit etre renseigne
	if (bOk and targetDatabase->GetDatabaseName() == "")
	{
		bOk = false;
		AddError("Missing output database name");
	}

	// On passe par une autre table en sortie, pour pouvoir specifier son chemin si elle n'en a pas
	workingTargetDatabase = targetDatabase->Clone();
	if (bOk)
		workingTargetDatabase->AddPathToUsedFiles(FileService::GetPathName(sourceDatabase->GetDatabaseName()));

	// Verification de la validite des specifications des bases source et cible
	bOk = bOk and sourceDatabase->Check();
	bOk = bOk and workingTargetDatabase->CheckPartially(true);
	assert(not bOk or transferClass != NULL);

	// Le ou les noms de fichier de la base cible doivent etre different de ceux de la base source
	if (bOk)
	{
		// Ajout du ou des fichiers de la base source
		sourceDatabase->ExportUsedFileSpecs(&oaSourceDatabaseFileSpecs);
		for (nRef = 0; nRef < oaSourceDatabaseFileSpecs.GetSize(); nRef++)
		{
			specRef = cast(FileSpec*, oaSourceDatabaseFileSpecs.GetAt(nRef));
			specRef->SetLabel("source " + specRef->GetLabel());
		}

		// Ajout du ou des fichiers de la base cible
		workingTargetDatabase->ExportUsedWriteFileSpecs(&oaTargetDatabaseFileSpecs);
		for (nTarget = 0; nTarget < oaTargetDatabaseFileSpecs.GetSize(); nTarget++)
		{
			specTarget = cast(FileSpec*, oaTargetDatabaseFileSpecs.GetAt(nTarget));
			specTarget->SetLabel("target " + specTarget->GetLabel());
		}

		// Chaque fichier cible doit etre different des fichiers sources
		if (bOk)
		{
			for (nTarget = 0; nTarget < oaTargetDatabaseFileSpecs.GetSize(); nTarget++)
			{
				specTarget = cast(FileSpec*, oaTargetDatabaseFileSpecs.GetAt(nTarget));

				// Parcours de fichiers d'entree
				for (nRef = 0; nRef < oaSourceDatabaseFileSpecs.GetSize(); nRef++)
				{
					specRef = cast(FileSpec*, oaSourceDatabaseFileSpecs.GetAt(nRef));

					// Test de difference
					bOk = bOk and specTarget->CheckReferenceFileSpec(specRef);
					if (not bOk)
						break;
				}
				if (not bOk)
					break;
			}
		}

		// Les fichiers cibles doivent etre differents entre eux
		if (bOk)
		{
			for (nTarget = 0; nTarget < oaTargetDatabaseFileSpecs.GetSize(); nTarget++)
			{
				specTarget = cast(FileSpec*, oaTargetDatabaseFileSpecs.GetAt(nTarget));

				// Parcours des autres fichiers de sortie
				for (nRef = nTarget + 1; nRef < oaTargetDatabaseFileSpecs.GetSize(); nRef++)
				{
					specRef = cast(FileSpec*, oaTargetDatabaseFileSpecs.GetAt(nRef));

					// Test de difference
					bOk = bOk and specTarget->CheckReferenceFileSpec(specRef);
					if (not bOk)
						break;
				}
				if (not bOk)
					break;
			}
		}

		// Creation si necessaire des repertoires des fichiers a transferer
		if (bOk)
		{
			for (nTarget = 0; nTarget < oaTargetDatabaseFileSpecs.GetSize(); nTarget++)
			{
				specTarget = cast(FileSpec*, oaTargetDatabaseFileSpecs.GetAt(nTarget));

				// Acces au repertoire du fichier a transferer
				sOutputPathName = FileService::GetPathName(specTarget->GetFilePathName());
				bOk = bOk and
				      KWResultFilePathBuilder::CheckResultDirectory(sOutputPathName, GetClassLabel());
			}
		}

		// Nettoyage
		oaSourceDatabaseFileSpecs.DeleteAll();
		oaTargetDatabaseFileSpecs.DeleteAll();
	}

	// Transformation des donnees (enfin!)
	if (bOk)
	{
		// Demarrage du suivi de la tache
		TaskProgression::SetTitle("Deploy model " + sourceDatabase->GetClassName());
		TaskProgression::Start();
		AddSimpleMessage("Deploy model " + sourceDatabase->GetClassName());

		// Preparation de la classe de transfer pour optimiser le transfer
		PrepareTransferClass(transferClass);

		// Parametrage du format de sortie de la base
		workingTargetDatabase->SetDenseOutputFormat(IsDenseOutputFormat());

		// Appel de la tache de transfer
		bOk = transferTask.Transfer(cast(KWMTDatabaseTextFile*, sourceDatabase),
					    cast(KWMTDatabaseTextFile*, workingTargetDatabase), lRecordNumber);

		// Nettoyage de la classe de transfer
		CleanTransferClass(transferClass);

		// Fin suivi de la tache
		TaskProgression::Stop();
	}

	// Ligne de separation dans le log
	AddSimpleMessage("");

	// Nettoyage
	delete workingTargetDatabase;
}

void KWDatabaseTransferView::BuildTransferredClass()
{
	boolean bOk = true;
	KWClass* transferClass;
	ALString sTmp;
	UIFileChooserCard registerCard;
	ObjectArray oaSourceDatabaseFileSpecs;
	ObjectArray oaTargetDatabaseFileSpecs;
	int nRef;
	FileSpec* specRef;
	FileSpec specTransferredDictionaryFile;
	ALString sTransferredClassFileName;
	KWClassDomain transferredClassDomain;
	KWClass* transferredClass;
	KWResultFilePathBuilder resultFilePathBuilder;

	// Memorisation des donnees modifies (non geres par les View)
	sClassName = GetStringValueAt("ClassName");

	// La classe doit etre valide
	transferClass = NULL;
	if (sClassName != "")
		transferClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);
	if (bOk and transferClass == NULL)
	{
		bOk = false;
		AddError("Deployment dictionary " + sClassName + " does not exist");
	}
	if (bOk and not transferClass->IsCompiled())
	{
		bOk = false;
		AddError("Incorrect deployment dictionary " + sClassName);
	}

	// Ouverture d'une boite de dialogue pour le nom du fichier dictionnaire
	if (bOk)
	{
		// On initialise avec le nom du dictionnaire en prenant le chemin de la base cible (ou source si vide)
		if (sourceDatabase->GetDatabaseName() != "")
		{
			// On combine les repertoire des base source et cible pour fabriquer le repertoire source de
			// reference
			resultFilePathBuilder.SetInputFilePathName(sourceDatabase->GetDatabaseName());
			resultFilePathBuilder.SetOutputFilePathName(targetDatabase->GetDatabaseName());
			resultFilePathBuilder.SetInputFilePathName(resultFilePathBuilder.BuildResultFilePathName());
		}
		else if (targetDatabase->GetDatabaseName() != "")
			resultFilePathBuilder.SetInputFilePathName(targetDatabase->GetDatabaseName());
		else if (sClassFileName != "")
			resultFilePathBuilder.SetInputFilePathName(sClassFileName);
		else
			resultFilePathBuilder.SetInputFilePathName(".");
		resultFilePathBuilder.SetOutputFilePathName("Deployed.kdic");
		resultFilePathBuilder.SetFileSuffix("kdic");
		sTransferredClassFileName = resultFilePathBuilder.BuildResultFilePathName();

		// Ouverture du FileChooser pour obtenir le nom du fichier a transfere, ou vide si annulation
		sTransferredClassFileName =
		    registerCard.ChooseFile("Save as", "Save", "FileChooser", "Dictionary\nkdic", "ClassFileName",
					    "Deployed dictionary file", sTransferredClassFileName);

		// Verification du nom du fichier de dictionnaire
		if (sTransferredClassFileName != "")
		{
			// Construction du chemin complet du dictionnaire a sauver
			resultFilePathBuilder.SetOutputFilePathName(sTransferredClassFileName);
			sTransferredClassFileName = resultFilePathBuilder.BuildResultFilePathName();

			// Specification du fichier en sortie
			specTransferredDictionaryFile.SetLabel("deployed dictionary");
			specTransferredDictionaryFile.SetFilePathName(sTransferredClassFileName);

			// Test de non collision avec des fichiers de la base source
			if (bOk)
			{
				sourceDatabase->ExportUsedFileSpecs(&oaSourceDatabaseFileSpecs);
				for (nRef = 0; nRef < oaSourceDatabaseFileSpecs.GetSize(); nRef++)
				{
					specRef = cast(FileSpec*, oaSourceDatabaseFileSpecs.GetAt(nRef));
					specRef->SetLabel("source " + specRef->GetLabel());
					bOk = bOk and specTransferredDictionaryFile.CheckReferenceFileSpec(specRef);
					if (not bOk)
						break;
				}
				oaSourceDatabaseFileSpecs.DeleteAll();
				if (not bOk)
					AddError("The deployed dictionary file name should differ from that of the "
						 "input database");
			}

			// Test de non collision avec des fichiers de la base cible
			if (bOk)
			{
				targetDatabase->ExportUsedFileSpecs(&oaTargetDatabaseFileSpecs);
				for (nRef = 0; nRef < oaTargetDatabaseFileSpecs.GetSize(); nRef++)
				{
					specRef = cast(FileSpec*, oaTargetDatabaseFileSpecs.GetAt(nRef));
					specRef->SetLabel("target " + specRef->GetLabel());
					bOk = bOk and specRef->CheckReferenceFileSpec(&specTransferredDictionaryFile);
					if (not bOk)
						break;
				}
				oaTargetDatabaseFileSpecs.DeleteAll();
				if (not bOk)
					AddError("The deployed dictionary file name should differ from that of the "
						 "output database");
			}
		}

		// Construction et sauvegarde du dictionnaire
		if (bOk and sTransferredClassFileName != "")
		{
			AddSimpleMessage("Write deployed dictionary file " + sTransferredClassFileName);
			transferredClass = InternalBuildTransferredClass(&transferredClassDomain, transferClass);

			// Ecriture du dictionnaire
			transferredClassDomain.WriteFile(sTransferredClassFileName);

			// Nettoyage
			transferredClassDomain.DeleteAllClasses();
		}
	}

	// Ligne de separation dans le log
	AddSimpleMessage("");
}

void KWDatabaseTransferView::EventUpdate()
{
	ALString sDatabaseClassName;

	// Appel de la methode ancetre
	UICard::EventUpdate();

	// Synchronisation du dictionnaire des bases
	sDatabaseClassName = GetStringValueAt("ClassName");
	sourceDatabase->SetClassName(sDatabaseClassName);
	targetDatabase->SetClassName(sDatabaseClassName);
	cast(KWDatabaseView*, GetFieldAt("SourceDatabase"))->SetStringValueAt("ClassName", sDatabaseClassName);
	cast(KWDatabaseView*, GetFieldAt("TargetDatabase"))->SetStringValueAt("ClassName", sDatabaseClassName);
}

void KWDatabaseTransferView::EventRefresh()
{
	// Appel de la methode ancetre
	UICard::EventRefresh();
}

const ALString KWDatabaseTransferView::GetClassLabel() const
{
	return "Deploy model";
}

const ALString KWDatabaseTransferView::GetObjectLabel() const
{
	return sourceDatabase->GetDatabaseName();
}

boolean KWDatabaseTransferView::IsDenseOutputFormat() const
{
	require(targetDatabaseView->GetStringValueAt("OutputFormat") == "sparse" or
		targetDatabaseView->GetStringValueAt("OutputFormat") == "tabular");
	return (targetDatabaseView->GetStringValueAt("OutputFormat") == "tabular");
}

KWClass* KWDatabaseTransferView::InternalBuildTransferredClass(KWClassDomain* transferredClassDomain,
							       const KWClass* transferClass)
{
	const ALString sPrefix = "D_";
	boolean bTransferReferenceClass;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWClass* transferredClass;
	KWAttribute* transferredAttribute;
	KWAttributeBlock* transferredAttributeBlock;
	KWAttribute* firstLoadedBlockAttribute;
	KWAttribute* lastLoadedBlockAttribute;
	KWClass* referenceClass;
	KWClass* transferredReferenceClass;
	int i;
	boolean bTransferAttribute;
	boolean bTransferClass;
	ALString sFormatMetaDataKey;
	ALString sFormat;

	require(transferredClassDomain != NULL);
	require(transferClass != NULL);
	require(transferClass == KWClassDomain::GetCurrentDomain()->LookupClass(transferClass->GetName()));
	require(transferClass->IsCompiled());
	require(transferredClassDomain->LookupClass(transferClass->GetName()) == NULL);

	// Le transfert d'une base ne concerne que les classes de la composition de la classe principale.
	// On transfere celles qui sont effectivement utilisees, ainsi que les informations de structures
	// quand elles sont toujours disponibles (presence des champs cles).
	// Par contre, les classes referencees ne sont pas transferees, pas plus que les informations
	// de structure les concernant
	bTransferReferenceClass = false;

	// Creation du dictionnaire transferee
	transferredClass = new KWClass;
	transferredClass->SetName(sPrefix + transferClass->GetName());
	transferredClass->SetLabel("Deployed dictionary");
	if (transferClass->GetLabel() != "")
		transferredClass->SetLabel("Deployed dictionary: " + transferClass->GetLabel());

	// Insertion de la classes transferee dans son domaine
	transferredClassDomain->InsertClass(transferredClass);

	// Creation de ses attributs
	for (i = 0; i < transferClass->GetLoadedAttributeNumber(); i++)
	{
		attribute = transferClass->GetLoadedAttributeAt(i);

		// On transfert les attributs data utilises, sans leur formule
		if (KWType::IsData(attribute->GetType()))
		{
			// Les attribut stockes sont toujours transferes
			bTransferAttribute = KWType::IsStored(attribute->GetType());

			// Les attributs objet sont transferes s'il sont sans regle de derivation, en lien
			// de composition ou de reference (avec regle predefinie KWDRReference) et si la classe
			// utilisee a une cle chargee en memoire
			transferredReferenceClass = NULL;
			if (sourceDatabase->IsMultiTableTechnology() and KWType::IsRelation(attribute->GetType()))
			{
				bTransferClass = false;
				// Cas d'un lien de composition
				if (not attribute->GetReference())
				{
					// Transfert si attribut utilise pour un lien de composition valide (avec cle
					// des deux cotes)
					bTransferAttribute = attribute->GetAnyDerivationRule() == NULL and
							     transferClass->IsKeyLoaded() and
							     attribute->GetClass() != NULL and
							     attribute->GetClass()->IsKeyLoaded();

					// Transfert de la classe meme si elle n'a plus sa cle
					bTransferClass =
					    attribute->GetAnyDerivationRule() == NULL and attribute->GetClass() != NULL;
				}
				else
				{
					assert(attribute->GetReference());
					// Dans la version actuelle, les classes referencees ne sont pas transferees
					// Si c'etait le cas un jour, il faudrait aussi verifier que les champs de la
					// cles utilises pour referencer l'objet cible sont egalement effectivement en
					// etat "Loaded"
					bTransferAttribute =
					    bTransferReferenceClass and attribute->GetAnyDerivationRule() != NULL and
					    attribute->GetAnyDerivationRule()->GetName() ==
						KWDerivationRule::GetReferenceRuleName() and
					    attribute->GetClass() != NULL and attribute->GetClass()->IsKeyLoaded();
				}

				// Transfer de la classe
				if (bTransferClass)
				{
					// Recherche de la classe referencee
					referenceClass = attribute->GetClass();
					check(referenceClass);

					// Creation si necessaire de la classe referencee
					transferredReferenceClass =
					    transferredClassDomain->LookupClass(sPrefix + referenceClass->GetName());
					if (transferredReferenceClass == NULL)
						transferredReferenceClass = InternalBuildTransferredClass(
						    transferredClassDomain, referenceClass);
				}
			}

			// Creation de l'attribut si attribut transfere
			if (bTransferAttribute)
			{
				// Transfert de l'attribut
				transferredAttribute = new KWAttribute;
				transferredAttribute->SetName(attribute->GetName());
				transferredAttribute->SetType(attribute->GetType());
				transferredAttribute->SetLabel(attribute->GetLabel());
				transferredAttribute->GetMetaData()->CopyFrom(attribute->GetConstMetaData());
				transferredClass->InsertAttribute(transferredAttribute);

				// Dans le cas d'un format dense, on supprime la meta-donnee VarKey des attributs des
				// blocs sparse, au meme titre que leur regle de derivation
				if (IsDenseOutputFormat())
					transferredAttribute->GetMetaData()->RemoveKey(
					    KWAttributeBlock::GetAttributeKeyMetaDataKey());

				// Complements dans le cas objet
				if (KWType::IsRelation(attribute->GetType()))
				{
					check(attribute->GetClass());
					check(transferredReferenceClass);
					assert(transferredReferenceClass ==
					       transferredClassDomain->LookupClass(sPrefix +
										   attribute->GetClass()->GetName()));

					// Chainage de cette classe
					transferredAttribute->SetClass(transferredReferenceClass);

					// Duplication de la regle de derivation dans le cas de referencement
					assert(not attribute->GetReference() or
					       (attribute->GetDerivationRule() != NULL and
						attribute->GetDerivationRule()->GetName() ==
						    KWDerivationRule::GetReferenceRuleName()));
					if (attribute->GetReference())
						transferredAttribute->SetDerivationRule(
						    attribute->GetDerivationRule()->Clone());
				}
			}
		}
	}

	// Transfer des blocs d'attribut de type simple (sans leur regle de derivation) dans le cas du format sparse
	if (not IsDenseOutputFormat())
	{
		for (i = 0; i < transferClass->GetLoadedAttributeBlockNumber(); i++)
		{
			attributeBlock = transferClass->GetLoadedAttributeBlockAt(i);
			assert(attributeBlock->GetLoadedAttributeNumber() > 0);

			// Transfer uniquement pour les type simple
			if (KWType::IsSimple(attributeBlock->GetType()))
			{
				// Recherche du premier et du dernier attribut charge du bloc
				// Attention, on ne peut pas se baser sur l'ordre des attributs depuis les methodes
				// GetLoadedAttributeAt, car l'indexation des attributs charges du bloc n'est pas
				// necessairement dans l'ordre des attributs Il faut donc explicitement rechercher ce
				// premier et dernier attribut charge
				firstLoadedBlockAttribute = NULL;
				lastLoadedBlockAttribute = NULL;
				attribute = attributeBlock->GetFirstAttribute();
				while (attribute != NULL)
				{
					// Mise a jour du premier et denrier attribut charge
					if (attribute->GetLoaded())
					{
						if (firstLoadedBlockAttribute == NULL)
							firstLoadedBlockAttribute = attribute;
						lastLoadedBlockAttribute = attribute;
					}
					// Arret si fin du bloc
					if (attribute == attributeBlock->GetLastAttribute())
						break;
					transferClass->GetNextAttribute(attribute);
				}
				assert(firstLoadedBlockAttribute != NULL);
				assert(lastLoadedBlockAttribute != NULL);

				// Creation du bloc correspondant dans la classe transferee
				// en se basant sur le premier et le dernier attribut charge du bloc
				transferredAttributeBlock = transferredClass->CreateAttributeBlock(
				    attributeBlock->GetName(),
				    transferredClass->LookupAttribute(firstLoadedBlockAttribute->GetName()),
				    transferredClass->LookupAttribute(lastLoadedBlockAttribute->GetName()));

				// On recopie les meta-data et le libelle, mais pas la regle de derivation potentielle
				// Rajout si necessaire d'une meta-data pour memoriser la valeur par defaut associee au
				// bloc
				transferredAttributeBlock->SetLabel(attributeBlock->GetLabel());
				transferredAttributeBlock->ImportMetaDataFrom(attributeBlock);
			}
		}
	}

	// Prise en compte des informations de cle, une fois les attributs transferes
	if (transferClass->IsKeyLoaded())
	{
		transferredClass->SetRoot(transferClass->GetRoot());
		transferredClass->SetKeyAttributeNumber(transferClass->GetKeyAttributeNumber());
		for (i = 0; i < transferClass->GetKeyAttributeNumber(); i++)
			transferredClass->SetKeyAttributeNameAt(i, transferClass->GetKeyAttributeNameAt(i));
	}
	ensure(transferClass->IsCompiled());
	return transferredClass;
}

void KWDatabaseTransferView::PrepareTransferClass(KWClass* transferClass)
{
	ObjectArray oaAllUsedClasses;
	KWClass* kwcUsedClass;
	int nClass;
	KWAttribute* attribute;

	// Parcours de toutes les classes utilisees
	transferClass->BuildAllUsedClasses(&oaAllUsedClasses);
	for (nClass = 0; nClass < oaAllUsedClasses.GetSize(); nClass++)
	{
		kwcUsedClass = cast(KWClass*, oaAllUsedClasses.GetAt(nClass));

		// On passe en Unload tous les attributs calcules de type Object ou ObjectArray,
		// ainsi que les attributs de type Structure
		attribute = kwcUsedClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			if (KWType::IsRelation(attribute->GetType()) and attribute->GetAnyDerivationRule() != NULL)
				attribute->SetLoaded(false);
			if (attribute->GetType() == KWType::Structure)
				attribute->SetLoaded(false);
			kwcUsedClass->GetNextAttribute(attribute);
		}
	}

	// Compilation de la classe
	transferClass->GetDomain()->Compile();
}

void KWDatabaseTransferView::CleanTransferClass(KWClass* transferClass)
{
	ObjectArray oaAllUsedClasses;
	KWClass* kwcUsedClass;
	int nClass;

	// Parcours de toutes les classes utilisees, pour tout mettre en Load
	transferClass->BuildAllUsedClasses(&oaAllUsedClasses);
	for (nClass = 0; nClass < oaAllUsedClasses.GetSize(); nClass++)
	{
		kwcUsedClass = cast(KWClass*, oaAllUsedClasses.GetAt(nClass));
		kwcUsedClass->SetAllAttributesLoaded(true);
	}

	// Compilation de la classe
	transferClass->GetDomain()->Compile();
}
