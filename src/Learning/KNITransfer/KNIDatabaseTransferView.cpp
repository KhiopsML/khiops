// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KNIDatabaseTransferView.h"

KNIDatabaseTransferView::KNIDatabaseTransferView()
{
	// Ajout de l'action de transfert de base avec KNI
	AddAction("KNITransferDatabase", "KNI deploy model",
		  (ActionMethod)(&KNIDatabaseTransferView::KNITransferDatabase));
	GetActionAt("KNITransferDatabase")->SetStyle("Button");

	// Ajout de champs specifique au transfert KNI
	AddStringField("KNILogFileName", "KNI log file name", "");
	AddIntField("KNIStreamMaxMemory", "KNI max stream memory in MB", 0);

	// Masquage de l'action de transfert de dictionnaire
	GetActionAt("BuildTransferredClass")->SetVisible(false);
}

KNIDatabaseTransferView::~KNIDatabaseTransferView() {}

void KNIDatabaseTransferView::SetClassFileName(const ALString& sValue)
{
	sClassFileName = sValue;
}

const ALString& KNIDatabaseTransferView::GetClassFileName() const
{
	return sClassFileName;
}

void KNIDatabaseTransferView::KNITransferDatabase()
{
	KNIMTRecodingOperands recodingOperands;
	KWMTDatabaseTextFile* sourceMTDatabase;
	KWMTDatabaseTextFile* targetMTDatabase;
	KWDatabase* workingTargetDatabase;
	Timer timerTransfer;
	int nMapping;
	KWMTDatabaseMapping* mapping;
	int nRecordNumber;
	ALString sTmp;

	// Acces aux bases source et cible
	sourceMTDatabase = cast(KWMTDatabaseTextFile*, sourceDatabase);
	targetMTDatabase = cast(KWMTDatabaseTextFile*, targetDatabase);

	// On passe par une autre table en sortie, pour pouvoir specifier son chemin si elle n'en a pas
	workingTargetDatabase = targetDatabase->Clone();
	workingTargetDatabase->AddPathToUsedFiles(FileService::GetPathName(sourceDatabase->GetDatabaseName()));

	// Alimentation de la structure de definition du recodage multi-tables
	sClassName = GetStringValueAt("ClassName");
	strcpy(recodingOperands.DictionaryFileName, GetClassFileName());
	strcpy(recodingOperands.DictionaryName, sClassName);
	recodingOperands.FieldSeparator = sourceMTDatabase->GetFieldSeparator();
	recodingOperands.SecondaryFileNumber = 0;
	recodingOperands.ExternalFileNumber = 0;
	for (nMapping = 0; nMapping < sourceMTDatabase->GetMultiTableMappings()->GetSize(); nMapping++)
	{
		mapping = cast(KWMTDatabaseMapping*, sourceMTDatabase->GetMultiTableMappings()->GetAt(nMapping));

		// Recopie des caracteristiques des tables externes
		if (sourceMTDatabase->IsReferencedClassMapping(mapping))
		{
			strcpy(recodingOperands.ExternalFiles[recodingOperands.ExternalFileNumber].DataRoot,
			       mapping->GetOriginClassName());
			strcpy(recodingOperands.ExternalFiles[recodingOperands.ExternalFileNumber].DataPath,
			       mapping->GetDataPathAttributeNames());
			strcpy(recodingOperands.ExternalFiles[recodingOperands.ExternalFileNumber].FileName,
			       mapping->GetDataTableName());
			recodingOperands.ExternalFileNumber = recodingOperands.ExternalFileNumber + 1;
		}
		// Recopie des caracteristiques de la table principale ou des tables secondaires
		else
		{
			// Cas du mapping racine (premier des mapping)
			if (nMapping == 0)
			{
				strcpy(recodingOperands.InputFile.DataPath, "");
				strcpy(recodingOperands.InputFile.FileName, mapping->GetDataTableName());
				FillKeyIndexes(mapping, &(recodingOperands.InputFile));
			}
			// Cas d'une table secondaire
			else
			{
				strcpy(recodingOperands.SecondaryFiles[recodingOperands.SecondaryFileNumber].DataPath,
				       mapping->GetDataPathAttributeNames());
				strcpy(recodingOperands.SecondaryFiles[recodingOperands.SecondaryFileNumber].FileName,
				       mapping->GetDataTableName());
				FillKeyIndexes(
				    mapping, &(recodingOperands.SecondaryFiles[recodingOperands.SecondaryFileNumber]));
				recodingOperands.SecondaryFileNumber = recodingOperands.SecondaryFileNumber + 1;
			}
		}
	}
	strcpy(recodingOperands.OutputFileName, workingTargetDatabase->GetDatabaseName());
	strcpy(recodingOperands.LogFileName, GetStringValueAt("KNILogFileName"));
	recodingOperands.MaxMemory = GetIntValueAt("KNIStreamMaxMemory");

	// Message de debut
	AddSimpleMessage("Create database file " + workingTargetDatabase->GetDatabaseName() + " with KNI");

	// Recodage avec l'API KNI
	timerTransfer.Start();
	nRecordNumber = KNIRecodeMTFiles(&recodingOperands);
	timerTransfer.Stop();

	// Message sur les temps de transfert
	AddSimpleMessage(sTmp + "Deploy model " + workingTargetDatabase->GetClassName() +
			 ": Written records : " + LongintToReadableString(nRecordNumber));
	AddSimpleMessage(sTmp + "KNI deployment time: " + SecondsToString(timerTransfer.GetElapsedTime()));

	// Nettoyage
	delete workingTargetDatabase;
}

void KNIDatabaseTransferView::FillKeyIndexes(const KWMTDatabaseMapping* mapping, KNIInputFile* inputFileSpec) const
{
	boolean bOk;
	KWClass* mappingClass;
	FILE* fInputFile;
	char sInputRecord[MAXRECORDLENGTH];
	char* sRetCode;
	int nKey;
	ALString sKeyField;
	StringVector svFields;
	char* sField;
	char cSeparator;
	int nField;

	require(mapping != NULL);
	require(inputFileSpec != NULL);

	// Recherche de la class associee au mapping
	mappingClass = KWClassDomain::GetCurrentDomain()->LookupClass(mapping->GetClassName());

	// Memorisation des index des champs cles
	inputFileSpec->KeyFieldNumber = 0;
	if (mappingClass != NULL and mappingClass->GetKeyAttributeNumber() > 0 and mapping->GetDataTableName() != "")
	{
		// Lecture de la premiere ligne du fichier
		bOk = FileService::OpenInputBinaryFile(mapping->GetDataTableName(), fInputFile);
		if (bOk)
		{
			sRetCode = fgets(sInputRecord, MAXRECORDLENGTH, fInputFile);
			sInputRecord[MAXRECORDLENGTH - 1] = '\0';

			// Recherche des index des champs cles
			if (sRetCode != NULL)
			{
				// Parsing de la ligne d'entete
				cSeparator = cast(KWMTDatabaseTextFile*, sourceDatabase)->GetFieldSeparator();
				sField = sRetCode;
				while (*sRetCode != '\0')
				{
					// Ajout d'un nouveau champs si fin de champ ou de ligne
					if (*sRetCode == cSeparator or *sRetCode == '\r' or *sRetCode == '\n')
					{
						*sRetCode = '\0';
						svFields.Add(sField);
						sRetCode++;
						sField = sRetCode;
					}
					else
						sRetCode++;

					// Cas particulier d'un champ en fin de fichier
					if (*sRetCode == '\0' and sRetCode != sField)
						svFields.Add(sField);
				}

				// Recherche de la position des cles
				for (nKey = 0; nKey < mappingClass->GetKeyAttributeNumber(); nKey++)
				{
					sKeyField = mappingClass->GetKeyAttributeNameAt(nKey);

					// Recherche de l'index du champ cle
					for (nField = 0; nField < svFields.GetSize(); nField++)
					{
						if (svFields.GetAt(nField) == sKeyField)
							break;
					}
					if (nField == svFields.GetSize())
						nField = -1;
					inputFileSpec->KeyFieldIndexes[inputFileSpec->KeyFieldNumber] = nField + 1;
					inputFileSpec->KeyFieldNumber = inputFileSpec->KeyFieldNumber + 1;
				}
			}

			// Fermeture du fichier
			FileService::CloseInputBinaryFile(mapping->GetDataTableName(), fInputFile);
		}
	}
}
