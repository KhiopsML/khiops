// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataTableDriverTextFile.h"

//////////////////////////////////////////////////////////////////////////
// Test de performance des routines d'entree-sortie
// Comparaison entre les API C, C++, binaire et texte.
//
// Resultats des tests: API la plus performante
//		- API C fopen, fread, fwrite, fclose
//		- ouverture des fichier en binaire
//		- utilisation d'un buffer de taille fixe egale a 16384 octets
//
// Gestion des retours a la ligne en DOS et UNIX
//		- en DOS: CR LF (\r\n)
//			le CR est vue comme un ^M en fin de ligne sous Unix
//		- en Unix: LF (\n)
//			sous DOS, tout apparait sur une seule ligne
// Solution:
//		- en lecture: ignorer \r et detecter les nouvelles lignes par \n
//		- en ecriture: ecrire \r\n en DOS, et \n en Unix
//
// Interet du mode binaire
//		- permet de gerer des fichier de taille superieure a 2 Go (dont
//		  la gestion par l'API texte semble incorrecte sous Windows)

KWDataTableDriverTextFile::KWDataTableDriverTextFile()
{
	bHeaderLineUsed = true;
	cFieldSeparator = '\t';

	// Initialisation du fichier
	bWriteMode = false;
	inputBuffer = NULL;
	outputBuffer = NULL;

	// Taille du buffer
	nBufferSize = GetDefaultBufferSize();
}

KWDataTableDriverTextFile::~KWDataTableDriverTextFile()
{
	if (inputBuffer != NULL)
		delete inputBuffer;
	if (outputBuffer != NULL)
		delete outputBuffer;
}

KWDataTableDriver* KWDataTableDriverTextFile::Create() const
{
	return new KWDataTableDriverTextFile;
}

void KWDataTableDriverTextFile::CopyFrom(const KWDataTableDriver* kwdtdSource)
{
	const KWDataTableDriverTextFile* kwdttfSource = cast(KWDataTableDriverTextFile*, kwdtdSource);

	require(inputBuffer == NULL and outputBuffer == NULL);

	// Copie standard
	KWDataTableDriver::CopyFrom(kwdtdSource);

	// Reinitialisation des attributs supplementaires
	inputBuffer = NULL;
	outputBuffer = NULL;
	bWriteMode = false;
	livDataItemLoadIndexes.SetSize(0);

	// Recopie des attribut supplementaires
	SetHeaderLineUsed(kwdttfSource->GetHeaderLineUsed());
	SetFieldSeparator(kwdttfSource->GetFieldSeparator());

	// Recopie des tailles de buffer
	SetBufferSize(kwdttfSource->GetBufferSize());
}

int KWDataTableDriverTextFile::Compare(const KWDataTableDriver* kwdtdSource) const
{
	int nCompare;
	const KWDataTableDriverTextFile* kwdttfSource = cast(KWDataTableDriverTextFile*, kwdtdSource);

	// Comparaison de base
	nCompare = KWDataTableDriver::Compare(kwdtdSource);

	// Comparaison specifique
	if (nCompare == 0)
		nCompare = CompareBoolean(GetHeaderLineUsed(), kwdttfSource->GetHeaderLineUsed());
	if (nCompare == 0)
		nCompare = GetFieldSeparator() - kwdttfSource->GetFieldSeparator();
	return nCompare;
}

boolean KWDataTableDriverTextFile::BuildDataTableClass(KWClass* kwcDataTableClass)
{
	boolean bOk;
	const ALString sLabelPrefix = "Var";
	StringVector svFirstLineFields;
	const int nFieldBigNumber = 100000;
	int nField;
	ALString sField;
	KWAttribute* attribute;
	ALString sAttributeName;
	boolean bAttributeOk;
	ALString sTmp;

	require(inputBuffer == NULL);

	require(kwcDataTableClass != NULL);
	require(kwcDataTableClass->GetDomain() == NULL);
	require(kwcDataTableClass->GetAttributeNumber() == 0);

	// Lecture des champs de la premiere ligne du fichier
	bOk = ReadHeaderLineFields(&svFirstLineFields);

	// Lecture de la premiere ligne du fichier pour construire les libelles
	// des champs
	if (bOk)
	{
		// Creation des noms des champs a partir de la premiere ligne
		nField = 0;
		Global::ActivateErrorFlowControl();
		while (nField < svFirstLineFields.GetSize())
		{
			sField = svFirstLineFields.GetAt(nField);
			nField++;

			// Cas ou la premiere ligne contient les libelles des champs
			if (GetHeaderLineUsed())
				sAttributeName = sField;
			// Sinon, on creer le nom des champs
			else
				sAttributeName = sLabelPrefix + IntToString(nField);

			// Test si nom d'attribut non vide
			if (sAttributeName == "")
			{
				// Erreur si au moins deux champs
				if (svFirstLineFields.GetSize() > 1)
				{
					bOk = false;
					AddError(sTmp + "The header line field " + IntToString(nField) + " is empty");
				}
				// Sinon, on sort pour ne pas comptabiliser le champs
				else
					break;
			}
			// Test de validite du nom de l'attribut
			else if (not KWClass::CheckName(sAttributeName, this))
			{
				bOk = false;
				AddError(sTmp + "The header line field " + IntToString(nField) + " is not valid");
			}
			// Test d'existence de l'attribut
			else if (kwcDataTableClass->LookupAttribute(sAttributeName) != NULL)
			{
				bOk = false;
				AddError(sTmp + "The header line field " + IntToString(nField) + " named " +
					 sAttributeName + " is already used");
			}
			// Creation de l'attribut sinon
			else
			{
				attribute = new KWAttribute;
				attribute->SetType(KWType::Symbol);
				attribute->SetName(sAttributeName);
				bAttributeOk = kwcDataTableClass->InsertAttribute(attribute);
				assert(bAttributeOk);
			}

			// Message si beaucoup de champs
			if ((nField % nFieldBigNumber == 0 and nField < svFirstLineFields.GetSize()) or
			    (nField >= nFieldBigNumber and nField == svFirstLineFields.GetSize()))
			{
				Global::DesactivateErrorFlowControl();
				if (nField < svFirstLineFields.GetSize())
					AddMessage(sTmp + "Read header line: " + IntToString(nField) + " fields read");
				else
					AddMessage(sTmp + "Read header line: total " + IntToString(nField) +
						   " fields read");
				Global::ActivateErrorFlowControl();
			}
		}
		Global::DesactivateErrorFlowControl();
	}

	// Si echec: on nettoie la classe
	if (not bOk)
	{
		kwcDataTableClass->SetName("");
		kwcDataTableClass->DeleteAllAttributes();
	}
	// Sinon, on l'indexe
	else
		kwcDataTableClass->IndexClass();

	// Retour
	ensure(not bOk or kwcDataTableClass->GetUsedAttributeNumber() == kwcDataTableClass->GetAttributeNumber());
	return bOk;
}

boolean KWDataTableDriverTextFile::OpenForRead(const KWClass* kwcLogicalClass)
{
	boolean bOk = true;
	KWClass kwcHeaderLineClass;
	KWClass* kwcUsedHeaderLineClass;

	// Reinitialisation du fichier de la base de donnees
	ResetDatabaseFile();

	// Creation d'une classe fictive basee sur l'analyse de la premiere ligne du fichier, dans le cas d'une ligne
	// d'entete pour obtenir le nombre de champs et leur eventuels nom et position (sans se soucier de leur type)
	kwcUsedHeaderLineClass = NULL;
	if (GetHeaderLineUsed())
	{
		kwcHeaderLineClass.SetName(kwcClass->GetDomain()->BuildClassName(GetClassName() + "HeaderLine"));
		bOk = BuildDataTableClass(&kwcHeaderLineClass);
		kwcUsedHeaderLineClass = &kwcHeaderLineClass;
	}

	// Calcul des index des champs de la classe physique
	if (bOk)
		bOk = ComputeDataItemLoadIndexes(kwcLogicalClass, kwcUsedHeaderLineClass);

	// Ouverture du fichier si OK
	if (bOk)
	{
		// Ouverture du fichier
		bOk = OpenInputDatabaseFile();

		// Lecture d'un premier buffer
		if (bOk)
		{
			bOk = UpdateInputBuffer();
			if (not bOk)
				inputBuffer->Close();
		}

		// Analyse du premier buffer pour detecter des eventuels caracteres NULL avant la fin du fichier
		if (bOk)
			bOk = CheckInputBuffer();

		// On saute la ligne d'entete si necessaire
		if (bOk and GetHeaderLineUsed())
			Skip();
	}

	// Nettoyage si erreur
	if (not bOk and inputBuffer != NULL)
	{
		delete inputBuffer;
		inputBuffer = NULL;
	}
	assert(bOk or not IsOpenedForRead());
	return bOk;
}

boolean KWDataTableDriverTextFile::OpenForWrite()
{
	boolean bOk;

	require(kwcClass != NULL);
	require(kwcClass->IsCompiled());
	require(outputBuffer == NULL);

	// Reinitialisation du fichier de la base de donnees
	ResetDatabaseFile();

	// Ouverture en ecriture
	bOk = OpenOutputDatabaseFile();

	// Ecriture de la ligne d'entete si necessaire
	if (bOk and GetHeaderLineUsed())
		WriteHeaderLine();
	assert(bOk or not IsOpenedForWrite());
	return bOk;
}

boolean KWDataTableDriverTextFile::IsOpenedForRead() const
{
	return inputBuffer != NULL and not bWriteMode;
}

boolean KWDataTableDriverTextFile::IsOpenedForWrite() const
{
	return outputBuffer != NULL and bWriteMode;
}

KWObject* KWDataTableDriverTextFile::Read()
{
	char* sField;
	int nFieldLength;
	int nFieldError;
	boolean bEndOfLine;
	boolean bLineTooLong;
	ALString sTmp;
	int nField;
	int nError;
	Continuous cValue;
	Date dtValue;
	Time tmValue;
	Timestamp tsValue;
	TimestampTZ tstzValue;
	KWObject* kwoObject;
	KWLoadIndex liLoadIndex;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWContinuousValueBlock* cvbValue;
	KWSymbolValueBlock* svbValue;
	ALString sMessage;
	boolean bValueBlockOk;

	require(not bWriteMode);
	require(inputBuffer != NULL);
	require(not IsEnd());
	require(not inputBuffer->IsBufferEnd());

	// On retourne NULL, sans message, si interruption utilisateur
	if (periodicTestInterruption.IsTestAllowed(lRecordIndex))
	{
		if (TaskProgression::IsInterruptionRequested())
			return NULL;
	}

	// Reinitialisation des champ de la derniere cle lue si necessaire
	assert(kwcClass->GetRoot() == (ivRootKeyIndexes.GetSize() > 0));
	if (lastReadRootKey.GetSize() > 0)
	{
		assert(livDataItemLoadIndexes.GetSize() == ivRootKeyIndexes.GetSize());
		lastReadRootKey.Initialize();
	}

	// Lecture des champs de la ligne
	bEndOfLine = false;
	bLineTooLong = false;
	nField = 0;
	sField = NULL;
	nFieldLength = 0;
	nFieldError = inputBuffer->FieldNoError;
	lRecordIndex++;
	kwoObject = new KWObject(kwcClass, lRecordIndex);
	while (not bEndOfLine)
	{
		// Analyse du champ si son index ne depasse pas le nombre de colonnes de l'entete et est utilise
		liLoadIndex.Reset();
		if (nField < livDataItemLoadIndexes.GetSize())
			liLoadIndex = livDataItemLoadIndexes.GetAt(nField);

		// On ne retient que les attributs ou blocs reconnus et non calcules
		// On lit toujours le premier champ pour detecter les lignes vides
		if (liLoadIndex.IsValid() or nField == 0)
			bEndOfLine = inputBuffer->GetNextField(sField, nFieldLength, nFieldError, bLineTooLong);
		else
			bEndOfLine = inputBuffer->SkipField(bLineTooLong);

		// Cas particulier: ligne trop longue
		if (bLineTooLong)
		{
			assert(bEndOfLine);

			// Warning
			AddWarning(sTmp + "Ignored record, " + InputBufferedFile::GetLineTooLongErrorLabel());

			// Destruction de l'objet en cours et on sort
			delete kwoObject;
			kwoObject = NULL;
			break;
		}

		// Cas particulier: ligne vide
		if (nField == 0 and bEndOfLine and (sField == NULL or sField[0] == '\0'))
		{
			// Destruction de l'objet uniquement si plusieurs champs attendus dans la classe ou dans le
			// fichier
			if (kwcClass->GetNativeDataItemNumber() > 1 or livDataItemLoadIndexes.GetSize() > 1)
			{
				// Warning si on n'est pas en fin de fichier
				if (not IsEnd())
				{
					AddWarning("Empty line");
				}

				// Destruction de l'objet en cours et on sort
				delete kwoObject;
				kwoObject = NULL;
				break;
			}
			// Sinon, on sort pour ne pas comptabiliser le champs,
			// uniquement si on est dans le cas d'un dictionnaire sans attribut natif
			else if (kwcClass->GetNativeDataItemNumber() == 0)
				break;
		}

		// Alimentation des champs de la derniere cle lue si necessaire
		// On le fait avant l'analyse des champs, car on doit collecter les champs de la cle
		// independament des erreurs
		if (lastReadRootKey.GetSize() > 0)
		{
			if (nField < livDataItemLoadIndexes.GetSize() and ivRootKeyIndexes.GetAt(nField) >= 0)
			{
				// Les champs cles sont necessairement lu dans le cas d'un driver physique de classe
				assert(liLoadIndex.IsValid());
				lastReadRootKey.SetAt(ivRootKeyIndexes.GetAt(nField), Symbol(sField, nFieldLength));
			}
		}

		// Analyse des attributs reconnus et non calcules
		if (liLoadIndex.IsValid())
		{
			// Acces au dataItem correspondant a l'index de chargement
			dataItem = kwcClass->GetDataItemAtLoadIndex(liLoadIndex);

			// Message si erreur sur le champ
			if (nFieldError != inputBuffer->FieldNoError)
				AddWarning(sTmp + dataItem->GetClassLabel() + " " + dataItem->GetObjectLabel() +
					   " with value <" + InputBufferedFile::GetDisplayValue(sField) +
					   "> : " + inputBuffer->GetFieldErrorLabel(nFieldError));

			// Cas d'un attribut
			if (dataItem->IsAttribute())
			{
				// Acces aux caracteristiques de l'attribut
				attribute = cast(KWAttribute*, dataItem);
				assert(attribute->GetDerivationRule() == NULL);

				// Cas attribut Symbol
				if (attribute->GetType() == KWType::Symbol)
				{
					// Troncature si necessaire des champs trop longs
					if (nFieldLength > KWValue::nMaxSymbolFieldSize)
					{
						if (bOverlengthyFieldsVerboseMode)
						{
							AddWarning(sTmp + dataItem->GetClassLabel() + " " +
								   dataItem->GetObjectLabel() + " with value <" +
								   InputBufferedFile::GetDisplayValue(sField) +
								   "> : " + "categorical field too long (" +
								   IntToString(nFieldLength) + "), truncated to " +
								   IntToString(KWValue::nMaxSymbolFieldSize) +
								   " characters");
						}
						nFieldLength = KWValue::nMaxSymbolFieldSize;
						sField[nFieldLength] = '\0';
					}
					kwoObject->SetSymbolValueAt(liLoadIndex, Symbol(sField, nFieldLength));
				}
				// Cas attribut Continuous
				else if (attribute->GetType() == KWType::Continuous)
				{
					// Transformation de l'eventuel separateur decimal ',' en '.'
					KWContinuous::PreprocessString(sField);

					// Conversion en Continuous
					nError = KWContinuous::StringToContinuousError(sField, cValue);
					kwoObject->SetContinuousValueAt(liLoadIndex, cValue);

					// Test de validite du champs
					if (nError != 0)
						AddWarning(sTmp + "Numerical variable " + attribute->GetName() +
							   ": value <" + InputBufferedFile::GetDisplayValue(sField) +
							   "> converted to <" +
							   KWContinuous::ContinuousToString(cValue) + "> (" +
							   KWContinuous::ErrorLabel(nError) + ")");
				}
				// Cas attribut Date
				else if (attribute->GetType() == KWType::Date)
				{
					// Conversion en Date
					dtValue = attribute->GetDateFormat()->StringToDate(sField);
					kwoObject->SetDateValueAt(liLoadIndex, dtValue);

					// Test de validite du champs
					if (sField[0] != '\0' and not dtValue.Check())
						AddWarning(sTmp + "Date variable " + attribute->GetName() +
							   ": value <" + InputBufferedFile::GetDisplayValue(sField) +
							   "> converted to <> (invalid date " +
							   attribute->GetDateFormat()->GetFormatString() + ")");
				}
				// Cas attribut Time
				else if (attribute->GetType() == KWType::Time)
				{
					// Conversion en Time
					tmValue = attribute->GetTimeFormat()->StringToTime(sField);
					kwoObject->SetTimeValueAt(liLoadIndex, tmValue);

					// Test de validite du champs
					if (sField[0] != '\0' and not tmValue.Check())
						AddWarning(sTmp + "Time variable " + attribute->GetName() +
							   ": value <" + InputBufferedFile::GetDisplayValue(sField) +
							   "> converted to <> (invalid time " +
							   attribute->GetTimeFormat()->GetFormatString() + ")");
				}
				// Cas attribut Timestamp
				else if (attribute->GetType() == KWType::Timestamp)
				{
					// Conversion en date
					tsValue = attribute->GetTimestampFormat()->StringToTimestamp(sField);
					kwoObject->SetTimestampValueAt(liLoadIndex, tsValue);

					// Test de validite du champs
					if (sField[0] != '\0' and not tsValue.Check())
						AddWarning(sTmp + "Timestamp variable " + attribute->GetName() +
							   ": value <" + InputBufferedFile::GetDisplayValue(sField) +
							   "> converted to <> (invalid timestamp " +
							   attribute->GetTimestampFormat()->GetFormatString() + ")");
				}
				// Cas attribut TimestampTZ
				else if (attribute->GetType() == KWType::TimestampTZ)
				{
					// Conversion en date
					tstzValue = attribute->GetTimestampTZFormat()->StringToTimestampTZ(sField);
					kwoObject->SetTimestampTZValueAt(liLoadIndex, tstzValue);

					// Test de validite du champs
					if (sField[0] != '\0' and not tstzValue.Check())
						AddWarning(sTmp + "TimestampTZ variable " + attribute->GetName() +
							   ": value <" + InputBufferedFile::GetDisplayValue(sField) +
							   "> converted to <> (invalid timestampTZ " +
							   attribute->GetTimestampTZFormat()->GetFormatString() + ")");
				}
				// Cas attribut Text
				else if (attribute->GetType() == KWType::Text)
				{
					// Les champs trop long sont tronques directement par le inputBuffer (cf.
					// nMaxFieldSize)
					kwoObject->SetTextValueAt(liLoadIndex, Symbol(sField, nFieldLength));
				}
			}
			// Cas d'un bloc d'attributs
			else
			{
				// Acces aux caracteristiques du bloc d'attributs
				attributeBlock = cast(KWAttributeBlock*, dataItem);
				assert(attributeBlock->GetDerivationRule() == NULL);

				// Cas des blocs d'attributs Symbol
				bValueBlockOk = true;
				if (attributeBlock->GetType() == KWType::Symbol)
				{
					// Lecture et alimentation d'un bloc de valeurs
					svbValue = KWSymbolValueBlock::BuildBlockFromField(
					    attributeBlock->GetLoadedAttributesIndexedKeyBlock(), sField,
					    attributeBlock->GetSymbolDefaultValue(), bValueBlockOk, sMessage);
					kwoObject->SetSymbolValueBlockAt(liLoadIndex, svbValue);
				}
				// Cas des blocs d'attributs Continuous
				else if (attributeBlock->GetType() == KWType::Continuous)
				{
					// Lecture et alimentation d'un bloc de valeurs
					cvbValue = KWContinuousValueBlock::BuildBlockFromField(
					    attributeBlock->GetLoadedAttributesIndexedKeyBlock(), sField,
					    attributeBlock->GetContinuousDefaultValue(), bValueBlockOk, sMessage);
					kwoObject->SetContinuousValueBlockAt(liLoadIndex, cvbValue);
				}

				// Warning si erreur de parsing des valeurs du bloc de variables
				if (not bValueBlockOk)
					AddWarning(sTmp + "Sparse variable block " + attributeBlock->GetName() +
						   ": value <" + InputBufferedFile::GetDisplayValue(sField) +
						   "> ignored because of parsing error (" + sMessage + ")");
			}
		}

		// Index du champs suivant
		nField++;
	}

	// Verification du nombre de champs
	if (kwoObject != NULL and nField != livDataItemLoadIndexes.GetSize())
	{
		// Emission d'un warning
		AddWarning(sTmp + "Ignored record, bad field number (" + IntToString(nField) + " read fields for " +
			   IntToString(livDataItemLoadIndexes.GetSize()) + " expected fields)");

		// Destruction de l'objet en cours et retour
		// Remarque: on ne modifie pas le nRecordNumber, qui permet d'identifier la ligne fautive
		delete kwoObject;
		kwoObject = NULL;
	}

	// Remplissage du buffer si necessaire
	UpdateInputBuffer();

	return kwoObject;
}

void KWDataTableDriverTextFile::Skip()
{
	boolean bLineTooLong;

	require(not bWriteMode);
	require(inputBuffer != NULL);

	// Cas d'une classe racine
	assert(kwcClass->GetRoot() == (ivRootKeyIndexes.GetSize() > 0));
	if (ivRootKeyIndexes.GetSize() > 0)
		SkipRootRecord();
	// Cas standard
	else
	{
		// Saut d'une ligne
		if (not IsEnd())
			lRecordIndex++;
		inputBuffer->SkipLine(bLineTooLong);

		// Remplissage du buffer si necessaire
		UpdateInputBuffer();
	}
}

void KWDataTableDriverTextFile::Write(const KWObject* kwoObject)
{
	int i;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWLoadIndex liLoadIndex;
	const char* sValue;
	Symbol sSymbolValue;
	KWContinuousValueBlock* continuousValueBlock;
	KWSymbolValueBlock* symbolValueBlock;
	ALString sValueBlockField;
	int nFieldIndex;
	ALString sTimeZoneAwareLabel;

	require(inputBuffer == NULL);
	require(outputBuffer != NULL);
	require(bWriteMode);

	// Parcours de tous les dataItems Loaded
	lRecordIndex++;
	nFieldIndex = 0;
	for (i = 0; i < kwcClass->GetLoadedDataItemNumber(); i++)
	{
		dataItem = kwcClass->GetLoadedDataItemAt(i);

		// Cas d'un attribut dense
		if (dataItem->IsAttribute())
		{
			attribute = cast(KWAttribute*, dataItem);

			// Ecriture de la valeur du champs
			// Contrairement a la lecture, ou ne limite pas la taille des champs ecrit pour les cas Symbol
			// et Text Cela donne de la souplesse pour des usages de type data management purs, et n'est pas
			// genant pour les analyses, qui reliront les donnees en les tronquant si necessaiore
			if (KWType::IsStored(attribute->GetType()))
			{
				if (nFieldIndex > 0)
					outputBuffer->Write(cFieldSeparator);
				sValue = kwoObject->ValueToString(attribute);
				outputBuffer->WriteField(sValue);
				nFieldIndex++;
			}
		}
		// Cas d'un bloc d'attributs
		else
		{
			attributeBlock = cast(KWAttributeBlock*, dataItem);

			// Acces au bloc de valeur sparse, selon son type
			if (KWType::IsStored(attributeBlock->GetType()))
			{
				liLoadIndex = attributeBlock->GetLoadIndex();
				if (nFieldIndex > 0)
					outputBuffer->Write(cFieldSeparator);
				if (attributeBlock->GetType() == KWType::Continuous)
				{
					continuousValueBlock = kwoObject->GetContinuousValueBlockAt(liLoadIndex);
					continuousValueBlock->WriteField(
					    attributeBlock->GetLoadedAttributesIndexedKeyBlock(), sValueBlockField);
					outputBuffer->WriteField(sValueBlockField);
				}
				else if (attributeBlock->GetType() == KWType::Symbol)
				{
					symbolValueBlock = kwoObject->GetSymbolValueBlockAt(liLoadIndex);
					symbolValueBlock->WriteField(
					    attributeBlock->GetLoadedAttributesIndexedKeyBlock(), sValueBlockField);
					outputBuffer->WriteField(sValueBlockField);
				}
				nFieldIndex++;
			}
		}
	}
	outputBuffer->WriteEOL();
}

boolean KWDataTableDriverTextFile::Close()
{
	boolean bOk;
	require(inputBuffer != NULL or outputBuffer != NULL);
	bOk = CloseDatabaseFile();
	return bOk;
}

boolean KWDataTableDriverTextFile::IsClosed() const
{
	return (outputBuffer == NULL or not outputBuffer->IsOpened()) and
	       (inputBuffer == NULL or not inputBuffer->IsOpened());
}

void KWDataTableDriverTextFile::DeleteDataTable()
{
	if (GetDataTableName() != "")
		FileService::RemoveFile(GetDataTableName());
}

longint KWDataTableDriverTextFile::GetEstimatedObjectNumber()
{
	longint lEstimatedObjectNumber = 0;
	InputBufferedFile temporaryFile;
	boolean bOk;
	longint lFileTotalSize;
	boolean bLineTooLong;
	longint lLineNumber;
	longint lTotalLineSize;
	double dAverageLineSize;
	longint lHeaderLigneSize;
	longint lBeginPos;
	Timer tReadTime;

	require(inputBuffer == NULL);
	require(outputBuffer == NULL);

	// On passe par un fichier temporaire pour ne pas interagir avec la gestion de la classe
	temporaryFile.SetFileName(GetDataTableName());

	// Ouverture du fichier
	// On prend la taille par defaut qui correspond a une taille "raisonnable" pour une analyse rapide et assez
	// significative du fichier On ne prend pas ici la PreferredBufferSize liee au fichier, qui risque d'eêtre soit
	// trop petite, soit trop grande
	temporaryFile.SetBufferSize(InputBufferedFile::nDefaultBufferSize);
	temporaryFile.SetSilentMode(true);
	bOk = temporaryFile.Open();

	// Analyse du fichier
	if (bOk)
	{
		// Calcul de la taille du fichier
		lFileTotalSize = temporaryFile.GetFileSize();

		// Remplissage avec un buffer
		lBeginPos = 0;
		bOk = temporaryFile.FillInnerLines(lBeginPos);

		// Analyse du buffer
		if (bOk and temporaryFile.GetCurrentBufferSize() > 0)
		{
			// On saute la premiere ligne, donc la taille n'est peut etre pas representative
			temporaryFile.SkipLine(bLineTooLong);
			lHeaderLigneSize = temporaryFile.GetPositionInFile();

			// Calcul du nombre de lignes hors ligne d'entete
			lLineNumber = temporaryFile.GetBufferLineNumber() - 1;

			// Calcul de la taille correspondante occupee dans le fichier
			lTotalLineSize = temporaryFile.GetCurrentBufferSize() - lHeaderLigneSize;
			assert(lTotalLineSize >= lLineNumber);

			// Calcul du nombre d'objets du fichier
			if (lLineNumber > 0 and lTotalLineSize > 0)
			{
				dAverageLineSize = (lTotalLineSize * 1.0) / lLineNumber;
				lEstimatedObjectNumber =
				    (longint)ceil((lFileTotalSize - lHeaderLigneSize) / dAverageLineSize);
			}
		}

		// Fermeture du fichier
		temporaryFile.Close();
	}
	return lEstimatedObjectNumber;
}

longint KWDataTableDriverTextFile::ComputeOpenNecessaryMemory(boolean bRead)
{
	longint lNecessaryMemory;
	longint lFileSize;

	require(inputBuffer == NULL and outputBuffer == NULL);

	// Initialisation avec la memoire utilisee
	lNecessaryMemory = GetUsedMemory();

	// Acces a la taille du buffer et du fichier si lecture
	lFileSize = 0;
	if (bRead and GetBufferSize() > 0)
		lFileSize = PLRemoteFileService::GetFileSize(GetDataTableName());

	// Prise en compte de la taille necessaire pour le buffer
	lNecessaryMemory += ComputeBufferNecessaryMemory(bRead, GetBufferSize(), lFileSize);
	ensure(lNecessaryMemory >= 0);
	return lNecessaryMemory;
}

longint KWDataTableDriverTextFile::ComputeNecessaryMemoryForFullExternalRead(const KWClass* kwcLogicalClass)
{
	boolean bDisplay = false;
	longint lNecessaryMemory;
	longint lFileTotalSize;
	int nDenseNativeValueNumber;
	int nSparseNativeValueNumber;
	int nDenseNativeLoadedSymbolValueNumber;
	int nSparseNativeLoadedSymbolValueNumber;
	int nPhysicalDenseLoadedValueNumber;
	int nPhysicalSparseLoadedValueNumber;
	int nPhysicalSparseLoadedValueBlockNumber;
	int nEstimatedValueNumber;
	longint lEstimatedRecordNumber;
	longint lObjectSize;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	ObjectDictionary odEmpty;

	require(GetClass() != NULL);
	require(kwcLogicalClass != NULL);

	// Initialisation des statistiques par type d'attribut
	nDenseNativeValueNumber = 0;
	nSparseNativeValueNumber = 0;
	nDenseNativeLoadedSymbolValueNumber = 0;
	nSparseNativeLoadedSymbolValueNumber = 0;
	nPhysicalDenseLoadedValueNumber = 0;
	nPhysicalSparseLoadedValueNumber = 0;
	nPhysicalSparseLoadedValueBlockNumber = 0;

	// Calcul des nombres d'attributs natifs dense et sparse dans la classe logique,
	// ce qui permet d'analyser le contenu du fichier et de l'exhaustivite de ses champs stockes
	// Dans le cas des blocs, on fait une estimation
	attribute = kwcLogicalClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Cas des attributs denses
		if (not attribute->IsInBlock())
		{
			if (attribute->IsNative())
			{
				nDenseNativeValueNumber++;
				if (attribute->GetType() == KWType::Symbol and attribute->GetLoaded())
					nDenseNativeLoadedSymbolValueNumber++;
			}
		}
		// Cas des attributs dans les blocs
		else
		{
			// Dans ce cas, on se base sur une estimation heuristique du nombre de valeurs presentes
			// en fonction de la taille globale du bloc (au plus une fois par bloc)
			if (attribute->IsFirstInBlock())
			{
				attributeBlock = attribute->GetAttributeBlock();
				if (attributeBlock->IsNative())
				{
					nSparseNativeValueNumber += KWAttributeBlock::GetEstimatedMeanValueNumber(
					    attributeBlock->GetAttributeNumber());
					if (attributeBlock->GetType() == KWType::Symbol and attributeBlock->GetLoaded())
						nSparseNativeLoadedSymbolValueNumber++;
				}
			}
		}
		kwcLogicalClass->GetNextAttribute(attribute);
	}

	// Calcul des nombres d'attributs charges en memoire dans la classe physique
	// qui ne memorise que les attributs charges en memoire (natifs ou calcules)
	// Dans le cas des blocs, on fait une estimation
	attribute = GetClass()->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Cas des attributs denses
		if (not attribute->IsInBlock())
		{
			if (attribute->GetLoaded())
				nPhysicalDenseLoadedValueNumber++;
		}
		// Cas des attributs dans les blocs
		else
		{
			// Dans ce cas, on se base sur une estimation heuristique du nombre de valeurs presentes
			// en fonction de la taille globale du bloc (au plus une fois par bloc)
			if (attribute->IsFirstInBlock())
			{
				attributeBlock = attribute->GetAttributeBlock();
				if (attributeBlock->GetLoaded())
				{
					nEstimatedValueNumber =
					    (int)ceil(KWAttributeBlock::GetEstimatedMeanValueNumber(
							  attributeBlock->GetAttributeNumber()) *
						      1.0 * attributeBlock->GetLoadedAttributeNumber() /
						      attributeBlock->GetAttributeNumber());
					nPhysicalSparseLoadedValueNumber += nEstimatedValueNumber;
					nPhysicalSparseLoadedValueBlockNumber++;
				}
			}
		}
		GetClass()->GetNextAttribute(attribute);
	}

	// Taille necessaire uniquement si au moins un attribut a charger
	lNecessaryMemory = 0;
	if (nPhysicalDenseLoadedValueNumber + nPhysicalSparseLoadedValueNumber > 0)
	{
		// Calcul de la taille du fichier
		lFileTotalSize = PLRemoteFileService::GetFileSize(GetDataTableName());

		// Estimation heuristique du nombre d'enregistrements du fichier
		lEstimatedRecordNumber = GetInMemoryEstimatedObjectNumber(lFileTotalSize);

		// Estimation de la memoire necessaire pour stocker un objet
		lObjectSize = sizeof(KWObject) + 2 * sizeof(void*);               // KWObject a vide
		lObjectSize += nPhysicalDenseLoadedValueNumber * sizeof(KWValue); // Valeurs dense de l'objet
		lObjectSize +=
		    nPhysicalSparseLoadedValueNumber * (sizeof(int) + sizeof(KWValue)); // Valeurs sparse de l'objet
		lObjectSize += nPhysicalSparseLoadedValueBlockNumber * sizeof(KWSymbolValueBlock);
		lObjectSize += GetClass()->GetKeyAttributeNumber() *
			       Symbol::GetUsedMemoryPerSymbol(); // Taille des Symbol de la cle (sans leur contenu)
		lObjectSize +=
		    odEmpty
			.GetUsedMemoryPerElement(); // Taille utilise dans le dictionnaire du KWObjectReferenceResolver

		// On calcul la memoire necessaire pour ce nombre d'objets avec le nombre de champs a charger
		lNecessaryMemory = lEstimatedRecordNumber * lObjectSize;

		// On rajoute par regle de 3 la partie du volume du fichier a prendre en compte
		// en estimant que chaque attribut calcule ou non est de taille similaire a ce qu'il y a dans le fichier
		// Cela permet de prendre en compte en partie la taille du contenu des attributs Symbol
		if (nDenseNativeValueNumber + nSparseNativeValueNumber > 0)
			lNecessaryMemory += (longint)ceil(
			    lFileTotalSize * 1.0 *
			    (1 + nDenseNativeLoadedSymbolValueNumber * 2 + nSparseNativeLoadedSymbolValueNumber * 5) /
			    (1 + nDenseNativeValueNumber * 2 + nSparseNativeValueNumber * 5));

		// Affichage
		if (bDisplay)
		{
			cout << "ComputeNecessaryMemoryForFullExternalRead " << GetDataTableName() << endl;
			cout << "\tNecessaryMemory\t" << lNecessaryMemory << endl;
			cout << "\tDictionary\t" << GetClass()->GetName() << endl;
			cout << "\tDenseNativeValueNumber\t" << nDenseNativeValueNumber << endl;
			cout << "\tPhysicalDenseLoadedValueNumber\t" << nPhysicalDenseLoadedValueNumber << endl;
			cout << "\tSparseNativeValueNumber\t" << nSparseNativeValueNumber << endl;
			cout << "\tPhysicalSparseLoadedValueNumber\t" << nPhysicalSparseLoadedValueNumber << endl;
			cout << "\tPhysicalSparseLoadedValueBlockNumber\t" << nPhysicalSparseLoadedValueBlockNumber
			     << endl;
			cout << "\tFileTotalSize\t" << lFileTotalSize << endl;
			cout << "\tEstimatedRecordNumber\t" << lEstimatedRecordNumber << endl;
			cout << "\tObjectSize\t" << lObjectSize << endl;
		}
	}
	return lNecessaryMemory;
}

longint KWDataTableDriverTextFile::ComputeNecessaryDiskSpaceForFullWrite(const KWClass* kwcLogicalClass,
									 longint lInputFileSize)
{
	boolean bDisplay = false;
	longint lNecessaryDiskSpace;
	longint lEstimatedRecordNumber;
	longint lWrittenObjectSize;

	require(GetClass() != NULL);
	require(kwcLogicalClass != NULL);
	require(lInputFileSize >= 0);

	// Taille necessaire uniquement si au moins un attribut a charger
	lNecessaryDiskSpace = 0;
	if (kwcLogicalClass->GetUsedAttributeNumber() > 0)
	{
		// Estimation du nombre d'enregistrements du fichier
		// L'estimation est fait en memoire avec le meme type d'heuristique que pour les donnees a ecrire
		lEstimatedRecordNumber = GetInMemoryEstimatedObjectNumber(lInputFileSize);

		// Estimation de la memoire necessaire pour stocker un objet a ecrire
		// (generalisation de l'objet natif)
		lWrittenObjectSize = GetEstimatedUsedOutputDiskSpacePerObject(kwcLogicalClass);

		// On calcul la memoire necessaire pour ce nombre d'ojet avec le nombre de champs a charger
		lNecessaryDiskSpace = lEstimatedRecordNumber * lWrittenObjectSize;

		// Affichage
		if (bDisplay)
		{
			cout << "ComputeNecessaryDiskSpaceForFullWrite " << GetDataTableName() << endl;
			cout << "\tNecessaryDiskSpace\t" << lNecessaryDiskSpace << endl;
			cout << "\tDictionary\t" << GetClass()->GetName() << endl;
			cout << "\tInputFileSize\t" << lInputFileSize << endl;
			cout << "\tEstimatedRecordNumber\t" << lEstimatedRecordNumber << endl;
			cout << "\tEstimatedWrittenObjectSize\t" << lWrittenObjectSize << endl;
		}
	}
	return lNecessaryDiskSpace;
}

double KWDataTableDriverTextFile::GetReadPercentage()
{
	double dPercentage = 0;

	// Calcul du pourcentage d'avancement en se basant sur la position courante dans le fichier
	if (inputBuffer != NULL and inputBuffer->IsOpened())
	{
		assert(inputBuffer->GetFileSize() >= 0);
		if (inputBuffer->GetFileSize() == 0)
			dPercentage = 0;
		else if (inputBuffer->GetPositionInFile() >= inputBuffer->GetFileSize())
			dPercentage = 1;
		else
			dPercentage = inputBuffer->GetPositionInFile() * 1.0 / inputBuffer->GetFileSize();
	}
	return dPercentage;
}

longint KWDataTableDriverTextFile::GetUsedMemory() const
{
	longint lUsedMemory;

	// Classe ancetre
	lUsedMemory = KWDataTableDriver::GetUsedMemory();

	// Specialisation
	lUsedMemory += sizeof(KWDataTableDriverTextFile) - sizeof(KWDataTableDriver);
	lUsedMemory += livDataItemLoadIndexes.GetUsedMemory();
	if (inputBuffer != NULL)
		lUsedMemory += inputBuffer->GetUsedMemory();
	if (outputBuffer != NULL)
		lUsedMemory += outputBuffer->GetUsedMemory();
	return lUsedMemory;
}

longint KWDataTableDriverTextFile::GetInMemoryEstimatedObjectNumber(longint lInputFileSize)
{
	boolean bDisplay = false;
	longint lEstimatedObjectNumber;
	longint lNativeObjectDiskSpace;

	require(GetClass() != NULL);
	require(GetDataTableName() != "");
	require(lInputFileSize >= 0);

	// Estimation de la taille d'un objet natif
	lNativeObjectDiskSpace = GetEstimatedUsedInputDiskSpacePerObject();

	// Estimation d'une borne sup du nombre d'enregistrement du fichier
	// Tres rustique, mais tres rapide (pas de scan du fichier)
	lEstimatedObjectNumber = 1 + lInputFileSize / lNativeObjectDiskSpace;

	// Affichage
	if (bDisplay)
	{
		cout << "GetEstimatedObjectNumber " << GetDataTableName() << endl;
		cout << "\tDictionary\t" << GetClass()->GetName() << endl;
		cout << "\tInputFileSize\t" << lInputFileSize << endl;
		cout << "\tEstimatedRecordNumber\t" << lEstimatedObjectNumber << endl;
		cout << "\tEstimatedNativeObjectDiskSpace\t" << lNativeObjectDiskSpace << endl;
	}
	return lEstimatedObjectNumber;
}

longint KWDataTableDriverTextFile::GetEstimatedUsedInputDiskSpacePerObject() const
{
	boolean bDisplay = false;
	int nDenseNativeValueNumber;
	int nTextNativeValueNumber;
	int nSparseNativeValueNumber;
	longint lNativeObjectSize;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;

	require(GetClass() != NULL);

	//////////////////////////////////////////////////////////////////////////////////////////
	// L'estimation est heuristique, sans lecture du fichier, en se basant uniquement sur une
	// estimation heuristique la la place occupee par chaque champs natif du dictionnaire

	// Initialisation des statistiques par type d'attribut
	nDenseNativeValueNumber = 0;
	nTextNativeValueNumber = 0;
	nSparseNativeValueNumber = 0;

	// Calcul des nombres d'attributs natifs dense et sparse dans la classe
	// Dans le cas des blocs, on fait une estimation
	attribute = GetClass()->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Cas des attributs denses
		if (not attribute->IsInBlock())
		{
			if (attribute->IsNative())
			{
				if (attribute->GetType() == KWType::Text)
					nTextNativeValueNumber++;
				else
					nDenseNativeValueNumber++;
			}
		}
		// Cas des attributs dans les blocs
		else
		{
			// Dans ce cas, on se base sur une estimation heuristique du nombre de valeurs presentes
			// en fonction de la taille globale du bloc (au plus une fois par bloc)
			if (attribute->IsFirstInBlock())
			{
				attributeBlock = attribute->GetAttributeBlock();
				if (attributeBlock->IsNative())
				{
					nSparseNativeValueNumber += KWAttributeBlock::GetEstimatedMeanValueNumber(
					    attributeBlock->GetAttributeNumber());
				}
			}
		}
		GetClass()->GetNextAttribute(attribute);
	}

	// Estimation de la taille d'un objet natif
	lNativeObjectSize = nMinRecordSize; // Fin de ligne plus un minimum
	lNativeObjectSize +=
	    nDenseNativeValueNumber * nDenseValueSize; // Valeurs dense de l'objet (valeur + separateur)
	lNativeObjectSize +=
	    nTextNativeValueNumber *
	    nTextValueSize; // Longueur moyenne d'un champ texte (entre un ancien et un nouveau tweet...)
	lNativeObjectSize += nSparseNativeValueNumber *
			     nSparseValueSize; // Valeurs sparse de l'objet (cle + ':' + valeur + blanc + separateur)
	lNativeObjectSize +=
	    GetClass()->GetKeyAttributeNumber() * nKeyFieldSize; // Taille des champs de la cle (heuristique)

	// Affichage
	if (bDisplay)
	{
		cout << "\tDictionary\t" << GetClass()->GetName() << endl;
		cout << "\tDenseNativeValueNumber\t" << nDenseNativeValueNumber << endl;
		cout << "\tTextNativeValueNumber\t" << nTextNativeValueNumber << endl;
		cout << "\tSparseNativeValueNumber\t" << nSparseNativeValueNumber << endl;
		cout << "\tEstimatedNativeObjectSize\t" << lNativeObjectSize << endl;
	}
	return lNativeObjectSize;
}

longint KWDataTableDriverTextFile::GetEstimatedUsedMemoryPerObject() const
{
	boolean bDisplay = false;
	int nPhysicalTextLoadedValueNumber;
	int nPhysicalDenseLoadedValueNumber;
	int nPhysicalSparseLoadedValueNumber;
	int nPhysicalSparseLoadedValueBlockNumber;
	int nEstimatedValueNumber;
	longint lObjectSize;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	ObjectDictionary odEmpty;

	require(GetClass() != NULL);

	// Initialisation des statistiques par type d'attribut
	nPhysicalTextLoadedValueNumber = 0;
	nPhysicalDenseLoadedValueNumber = 0;
	nPhysicalSparseLoadedValueNumber = 0;
	nPhysicalSparseLoadedValueBlockNumber = 0;

	// Calcul des nombres d'attributs charges en memoire dans la classe physique
	// qui ne memorise que les attributs charges en memoire (natifs ou calcules)
	// Dans le cas des blocs, on fait une estimation
	attribute = GetClass()->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Cas des attributs denses
		if (not attribute->IsInBlock())
		{
			if (attribute->GetLoaded())
			{
				if (attribute->GetType() == KWType::Text)
					nPhysicalTextLoadedValueNumber++;
				nPhysicalDenseLoadedValueNumber++;
			}
		}
		// Cas des attributs dans les blocs
		else
		{
			// Dans ce cas, on se base sur une estimation heuristique du nombre de valeurs presentes
			// en fonction de la taille globale du bloc (au plus une fois par bloc)
			if (attribute->IsFirstInBlock())
			{
				attributeBlock = attribute->GetAttributeBlock();
				if (attributeBlock->GetLoaded())
				{
					nEstimatedValueNumber =
					    (int)ceil(KWAttributeBlock::GetEstimatedMeanValueNumber(
							  attributeBlock->GetAttributeNumber()) *
						      1.0 * attributeBlock->GetLoadedAttributeNumber() /
						      attributeBlock->GetAttributeNumber());
					nPhysicalSparseLoadedValueNumber += nEstimatedValueNumber;
					nPhysicalSparseLoadedValueBlockNumber++;
				}
			}
		}
		GetClass()->GetNextAttribute(attribute);
	}

	// Estimation de la memoire necessaire pour stocker un objet
	lObjectSize = sizeof(KWObject) + 2 * sizeof(void*); // KWObject a vide
	lObjectSize +=
	    nPhysicalDenseLoadedValueNumber * (sizeof(KWValue) + nDenseValueSize);         // Valeurs dense de l'objet
	lObjectSize += nPhysicalTextLoadedValueNumber * nTextValueSize;                    // Valeurs Text de l'objet
	lObjectSize += nPhysicalSparseLoadedValueNumber * (sizeof(int) + sizeof(KWValue)); // Valeurs sparse de l'objet
	lObjectSize += nPhysicalSparseLoadedValueBlockNumber * sizeof(KWSymbolValueBlock);
	lObjectSize +=
	    GetClass()->GetKeyAttributeNumber() *
	    (Symbol::GetUsedMemoryPerSymbol() + nKeyFieldSize); // Taille des Symbol de la cle (sans leur contenu)

	// Affichage
	if (bDisplay)
	{
		cout << "ComputeNecessaryMemoryForFullExternalRead " << GetDataTableName() << endl;
		cout << "\tDictionary\t" << GetClass()->GetName() << endl;
		cout << "\tPhysicalTextLoadedValueNumber\t" << nPhysicalTextLoadedValueNumber << endl;
		cout << "\tPhysicalDenseLoadedValueNumber\t" << nPhysicalDenseLoadedValueNumber << endl;
		cout << "\tPhysicalSparseLoadedValueNumber\t" << nPhysicalSparseLoadedValueNumber << endl;
		cout << "\tPhysicalSparseLoadedValueBlockNumber\t" << nPhysicalSparseLoadedValueBlockNumber << endl;
		cout << "\tObjectSize\t" << lObjectSize << endl;
	}
	return lObjectSize;
}

longint KWDataTableDriverTextFile::GetEstimatedUsedOutputDiskSpacePerObject(const KWClass* kwcLogicalClass) const
{
	boolean bDisplay = false;
	int nDenseLoadedValueNumber;
	int nTextLoadedValueNumber;
	int nSparseLoadedValueNumber;
	int nEstimatedValueNumber;
	longint lWrittenObjectSize;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;

	require(kwcLogicalClass != NULL);

	// Initialisation des statistiques par type d'attribut
	nDenseLoadedValueNumber = 0;
	nTextLoadedValueNumber = 0;
	nSparseLoadedValueNumber = 0;

	// Calcul des nombres d'attributs natifs dense et sparse dans la classe logique,
	// ce qui permet d'analyser le contenu du fichier
	// Dans le cas des blocs, on fait une estimation
	attribute = kwcLogicalClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Cas des attributs denses
		if (not attribute->IsInBlock())
		{
			if (attribute->GetLoaded())
			{
				if (attribute->GetType() == KWType::Text)
					nTextLoadedValueNumber++;
				else
					nDenseLoadedValueNumber++;
			}
		}
		// Cas des attributs dans les blocs
		else
		{
			// Dans ce cas, on se base sur une estimation heuristique du nombre de valeurs presentes
			// en fonction de la taille globale du bloc (au plus une fois par bloc)
			if (attribute->IsFirstInBlock())
			{
				attributeBlock = attribute->GetAttributeBlock();
				if (attributeBlock->GetLoaded())
				{
					nEstimatedValueNumber =
					    (int)ceil(KWAttributeBlock::GetEstimatedMeanValueNumber(
							  attributeBlock->GetAttributeNumber()) *
						      1.0 * attributeBlock->GetLoadedAttributeNumber() /
						      attributeBlock->GetAttributeNumber());
					nSparseLoadedValueNumber += nEstimatedValueNumber;
				}
			}
		}
		kwcLogicalClass->GetNextAttribute(attribute);
	}

	// Estimation de la memoire necessaire pour stocker un objet a ecrire
	// (generalisation de l'objet natif)
	lWrittenObjectSize = nMinRecordSize;
	lWrittenObjectSize += nDenseLoadedValueNumber * nDenseValueSize;
	lWrittenObjectSize += nTextLoadedValueNumber * nTextValueSize;
	lWrittenObjectSize += nSparseLoadedValueNumber * nSparseValueSize;
	lWrittenObjectSize += kwcLogicalClass->GetKeyAttributeNumber() * nKeyFieldSize;

	// Affichage
	if (bDisplay)
	{
		cout << "ComputeNecessaryDiskSpaceForFullWrite " << GetDataTableName() << endl;
		cout << "\tDictionary\t" << GetClass()->GetName() << endl;
		cout << "\tDenseLoadedValueNumber\t" << nDenseLoadedValueNumber << endl;
		cout << "\tTextLoadedValueNumber\t" << nTextLoadedValueNumber << endl;
		cout << "\tSparseLoadedValueNumber\t" << nSparseLoadedValueNumber << endl;
		cout << "\tEstimatedWrittenObjectSize\t" << lWrittenObjectSize << endl;
	}
	return lWrittenObjectSize;
}

boolean KWDataTableDriverTextFile::ReadHeaderLineFields(StringVector* svFirstLineFields)
{
	boolean bOk;

	require(inputBuffer == NULL);
	require(svFirstLineFields != NULL);

	// Lecture des champs de la premiere ligne du fichier
	bOk = InputBufferedFile::GetFirstLineFields(GetDataTableName(), GetFieldSeparator(), GetSilentMode(),
						    GetVerboseMode(), svFirstLineFields);
	return bOk;
}

void KWDataTableDriverTextFile::WriteHeaderLine()
{
	int i;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	int nFieldIndex = 0;

	require(GetHeaderLineUsed());
	require(kwcClass != NULL);
	require(kwcClass->IsCompiled());
	require(bWriteMode);
	assert(outputBuffer != NULL);

	// Ecriture des nom des dataItems Loaded
	for (i = 0; i < kwcClass->GetLoadedDataItemNumber(); i++)
	{
		dataItem = kwcClass->GetLoadedDataItemAt(i);

		// Ecriture du nom de l'attribut
		if (dataItem->IsAttribute())
		{
			attribute = cast(KWAttribute*, dataItem);

			if (KWType::IsStored(attribute->GetType()))
			{
				if (nFieldIndex > 0)
					outputBuffer->Write(cFieldSeparator);
				outputBuffer->WriteField(attribute->GetName());
				nFieldIndex++;
			}
		}
		// Ecriture du nom du bloc d'attributs
		else
		{
			attributeBlock = cast(KWAttributeBlock*, dataItem);
			if (nFieldIndex > 0)
				outputBuffer->Write(cFieldSeparator);
			outputBuffer->WriteField(attributeBlock->GetName());
			nFieldIndex++;
		}
	}
	outputBuffer->WriteEOL();
}

boolean KWDataTableDriverTextFile::CheckFormat() const
{
	boolean bOk;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDataTableDriver::CheckFormat();

	// Verification du separateur de champ
	if (bOk)
	{
		if (isalnum(cFieldSeparator))
		{
			bOk = false;
			AddError(sTmp + "Field separator '" + CharToString(cFieldSeparator) +
				 "' must not be alphanumeric");
		}
		else if (cFieldSeparator == '"')
		{
			bOk = false;
			AddError(sTmp + "Field separator '\"' must not be double-quote");
		}
		else if (cFieldSeparator == '\r' or cFieldSeparator == '\n')
		{
			bOk = false;
			AddError(sTmp + "Field separator '" + CharToString(cFieldSeparator) +
				 "' must not be end of line");
		}
		else if (cFieldSeparator == '\0')
		{
			bOk = false;
			AddError(sTmp + "Field separator '" + CharToString(cFieldSeparator) + "' must not be null");
		}
	}
	return bOk;
}

void KWDataTableDriverTextFile::SetBufferSize(int nSize)
{
	require(nSize >= 0);
	nBufferSize = nSize;
}

int KWDataTableDriverTextFile::GetBufferSize() const
{
	return nBufferSize;
}

int KWDataTableDriverTextFile::ComputeBufferNecessaryMemory(boolean bRead, int nBufferSize, longint lFileSize)
{
	int nNecessaryMemory;

	require(nBufferSize >= 0);
	require(lFileSize >= 0);

	// Calcul de la taille du buffer, en tenant compte de la taille du fichier
	nNecessaryMemory = nBufferSize;
	if (bRead and nNecessaryMemory > 0)
	{
		if (lFileSize < nNecessaryMemory)
			nNecessaryMemory = (int)lFileSize;
	}

	// Ajout d'un overhead pour la taille des objets en memoire, essentiellement en lecture
	if (bRead)
		nNecessaryMemory += nNecessaryMemory / 4;
	else
		nNecessaryMemory += nNecessaryMemory / 8;
	return nNecessaryMemory;
}

boolean KWDataTableDriverTextFile::bOverlengthyFieldsVerboseMode = true;

void KWDataTableDriverTextFile::SetOverlengthyFieldsVerboseMode(boolean bValue)
{
	bOverlengthyFieldsVerboseMode = bValue;
}

boolean KWDataTableDriverTextFile::GetOverlengthyFieldsVerboseMode()
{
	return bOverlengthyFieldsVerboseMode;
}

void KWDataTableDriverTextFile::SetSilentMode(boolean bValue)
{
	// Appel de la methode ancetre
	KWDataTableDriver::SetSilentMode(bValue);

	// Application au buffer en cours
	if (inputBuffer != NULL)
		inputBuffer->SetSilentMode(GetSilentMode());
	if (outputBuffer != NULL)
		outputBuffer->SetSilentMode(GetSilentMode());
}

void KWDataTableDriverTextFile::SkipRootRecord()
{
	char* sField;
	int nRootKeyIndex;
	int nKeyFieldNumber;
	boolean bEndOfLine;
	boolean bLineTooLong;
	int nFieldLength;
	int nFieldError;
	int nField;

	require(not bWriteMode);
	require(inputBuffer != NULL);
	require(not IsEnd());
	require(not inputBuffer->IsBufferEnd());
	assert(kwcClass->GetRoot() == (ivRootKeyIndexes.GetSize() > 0));

	// Reinitialisation des champ de la derniere cle lue si necessaire
	assert(livDataItemLoadIndexes.GetSize() == ivRootKeyIndexes.GetSize());
	lastReadRootKey.Initialize();

	// Saut d'une ligne
	if (not IsEnd() and not IsError())
	{
		lRecordIndex++;

		// Lecture des champs de la ligne
		bEndOfLine = false;
		nField = 0;
		sField = NULL;
		nFieldLength = 0;
		nKeyFieldNumber = 0;
		while (not bEndOfLine)
		{
			// Analyse du champ si son index ne depasse pas le nombre de colonnes de l'entete et est utilise
			nRootKeyIndex = -1;
			if (nField < ivRootKeyIndexes.GetSize())
				nRootKeyIndex = ivRootKeyIndexes.GetAt(nField);

			// On ne retient que les attributs ou blocs reconnus et non calcules
			// On lit toujours le premier champ pour detecter les lignes vides
			if (nRootKeyIndex >= 0 or nField == 0)
				bEndOfLine = inputBuffer->GetNextField(sField, nFieldLength, nFieldError, bLineTooLong);
			else
				bEndOfLine = inputBuffer->SkipField(bLineTooLong);

			// Cas particulier: ligne vide
			if (nField == 0 and bEndOfLine and (sField == NULL or sField[0] == '\0'))
				break;

			// Alimentation des champs de la derniere cle lue si necessaire
			if (nRootKeyIndex >= 0)
			{
				lastReadRootKey.SetAt(ivRootKeyIndexes.GetAt(nField), Symbol(sField, nFieldLength));
				nKeyFieldNumber++;

				// Arret si on a lu tous les champs de la cle
				if (nKeyFieldNumber == kwcClass->GetKeyAttributeNumber())
				{
					if (not bEndOfLine)
						inputBuffer->SkipLine(bLineTooLong);
					break;
				}
			}

			// On ignore les lignes trop longues
			if (bLineTooLong)
			{
				lastReadRootKey.Initialize();
				AddWarning("Ignored record, " + InputBufferedFile::GetLineTooLongErrorLabel());
			}

			// Index du champs suivant
			nField++;
		}
	}

	// Remplissage du buffer si necessaire
	UpdateInputBuffer();
}

boolean KWDataTableDriverTextFile::FillInputBufferWithFullLines(longint lBeginPos, longint lMaxEndPos)
{
	boolean bOk;
	boolean bLineTooLong;
	longint lNewBeginPos;
	ALString sTmp;

	require(not bWriteMode);
	require(inputBuffer != NULL);
	require(0 <= lBeginPos and lBeginPos <= inputBuffer->GetFileSize());
	require(lBeginPos <= lMaxEndPos and lMaxEndPos <= inputBuffer->GetFileSize());
	require(inputBuffer->GetBufferSize() > 0 or inputBuffer->GetFileSize() == 0 or lBeginPos == lMaxEndPos);

	// Lecture de ligne entiere a partir de la position courante
	bOk = inputBuffer->FillOuterLinesUntil(lBeginPos, lMaxEndPos, bLineTooLong);

	// Traitement des lignes trop longues
	while (bOk and bLineTooLong)
	{
		assert(inputBuffer->GetPositionInFile() - lBeginPos > InputBufferedFile::GetMaxLineLength());

		// Warning sur la ligne trop longue
		AddWarning(sTmp + "Line too long (" +
			   LongintToHumanReadableString(inputBuffer->GetPositionInFile() - lBeginPos) + ")");

		// Erreur si on depasse lMaxEndPos, le fichier etant indexe prealablement et se terminant sur une fin de
		// ligne en lMaxEndPos
		if (inputBuffer->GetPositionInFile() > lMaxEndPos)
		{
			AddWarning("The line ends are not consistent with those calculated during the preliminary "
				   "indexing of the file");
			break;
		}

		// Incrementation du numero de ligne pour tenir compte de la ligne sautee
		lRecordIndex++;

		// Lecture de la suite a partir de la fin de la ligne trop longue
		lNewBeginPos = inputBuffer->GetPositionInFile();
		bOk = inputBuffer->FillOuterLinesUntil(lNewBeginPos, lMaxEndPos, bLineTooLong);
	}
	ensure(inputBuffer->IsFileEnd() or not bOk or
	       (inputBuffer->GetPositionInFile() + inputBuffer->GetCurrentBufferSize()) <= lMaxEndPos);
	return bOk;
}

boolean KWDataTableDriverTextFile::CheckInputBuffer()
{
	boolean bOk = true;

	require(inputBuffer != NULL);

	// Erreur si probleme d'encodage
	bOk = inputBuffer->CheckEncoding();

	// Nettoyage du buffer si erreur
	if (not bOk)
	{
		delete inputBuffer;
		inputBuffer = NULL;
	}
	return bOk;
}

boolean KWDataTableDriverTextFile::ComputeDataItemLoadIndexes(const KWClass* kwcLogicalClass,
							      const KWClass* kwcHeaderLineClass)
{
	boolean bOk = true;
	boolean bDisplay = false;
	int i;
	ObjectDictionary odKeyFieldIndexes;
	IntObject* keyFieldIndex;
	ObjectArray oaNativeLogicalDataItems;
	KWAttribute* headerLineAttribute;
	KWDataItem* logicalDataItem;
	KWAttribute* logicalAttribute;
	KWAttributeBlock* logicalAttributeBlock;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	ALString sAttributeName;
	ALString sTmp;

	require(kwcLogicalClass != NULL);
	require(kwcHeaderLineClass != NULL or not GetHeaderLineUsed());
	require(kwcHeaderLineClass == NULL or GetHeaderLineUsed());

	// Initialisation pour le calcul des index des attributs cles que dans le cas d'une classe racine
	ivRootKeyIndexes.SetSize(0);
	lastReadRootKey.SetSize(0);
	if (kwcLogicalClass->GetRoot())
	{
		// Creation d'un objet pour accuillir les champ de la cle
		lastReadRootKey.SetSize(kwcLogicalClass->GetKeyAttributeNumber());

		// Creation d'un dictionnaire qui a chaque attribut de la cle associe son index
		for (i = 0; i < kwcLogicalClass->GetKeyAttributeNumber(); i++)
		{
			keyFieldIndex = new IntObject;
			keyFieldIndex->SetInt(i);
			odKeyFieldIndexes.SetAt(kwcLogicalClass->GetKeyAttributeNameAt(i), keyFieldIndex);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// Si noms extraits de la ligne d'entete, recherche des correspondances
	Global::ActivateErrorFlowControl();
	if (GetHeaderLineUsed())
	{
		assert(kwcHeaderLineClass != NULL);

		// Parcours des champs de l'entete
		livDataItemLoadIndexes.SetSize(kwcHeaderLineClass->GetUsedAttributeNumber());
		for (i = 0; i < kwcHeaderLineClass->GetUsedAttributeNumber(); i++)
		{
			headerLineAttribute = kwcHeaderLineClass->GetUsedAttributeAt(i);

			// Recherche de l'attribut ou du bloc d'attribut correspondant au nom du champ
			dataItem = kwcClass->LookupDataItem(headerLineAttribute->GetName());

			// Indexation eventuelle dans le cas d'un attribut ou d'un bloc d'attribut
			livDataItemLoadIndexes.ResetAt(i);
			if (dataItem != NULL)
			{
				// Cas d'un attribut
				if (dataItem->IsAttribute())
				{
					attribute = cast(KWAttribute*, dataItem);

					// Erreur si champ derive
					if (attribute->GetDerivationRule() != NULL)
					{
						bOk = false;
						AddError("The field " + headerLineAttribute->GetName() +
							 " corresponds to a derived variable in the dictionary " +
							 GetClassName());
					}
					// Erreur si champ dans un bloc
					else if (attribute->GetAttributeBlock() != NULL)
					{
						bOk = false;
						AddError("The field " + headerLineAttribute->GetName() +
							 " corresponds to a variable in a the sparse variable block " +
							 attribute->GetAttributeBlock()->GetName() +
							 " in the dictionary " + GetClassName());
					}
					// Erreur si champ non stocke
					else if (not KWType::IsStored(attribute->GetType()))
					{
						bOk = false;
						AddError("The field " + headerLineAttribute->GetName() +
							 " corresponds to a variable of type " +
							 KWType::ToString(attribute->GetType()) +
							 " in the dictionary " + GetClassName());
					}
					// Sinon, indexation
					else
						livDataItemLoadIndexes.SetAt(i, attribute->GetLoadIndex());
				}
				// Cas d'un bloc d'attributs
				else
				{
					attributeBlock = cast(KWAttributeBlock*, dataItem);

					// Warning si champ derive
					if (attributeBlock->GetDerivationRule() != NULL)
						AddWarning("The field " + headerLineAttribute->GetName() +
							   " corresponds to a derived sparse variable block in the "
							   "dictionary " +
							   GetClassName() + " and is ignored during read");
					// Sinon, indexation
					else
						livDataItemLoadIndexes.SetAt(i, attributeBlock->GetLoadIndex());
				}
			}
			// Warning si champ inconnu dans la classe logique
			else if (kwcLogicalClass->LookupDataItem(headerLineAttribute->GetName()) == NULL)
				AddWarning("The field " + headerLineAttribute->GetName() +
					   " is unknown in the dictionary " + GetClassName() +
					   " and is ignored during read");
		}

		// Recherche des attributs non derives absents du fichier par parcours exhaustif de la
		// classe d'origine (la classe "physique" a supprime les references a tous
		// les attributs non utilises par les attribut derives, y compris les
		// attribut inexistants (sauf si classe physique = classe logique))
		logicalAttribute = kwcLogicalClass->GetHeadAttribute();
		while (logicalAttribute != NULL)
		{
			// Recherche du champ correspondant si attribut natif hors block
			if (not logicalAttribute->IsInBlock())
			{
				if (logicalAttribute->IsNative() and
				    kwcHeaderLineClass->LookupAttribute(logicalAttribute->GetName()) == NULL)
				{
					// Erreur si necessaire dans la classe physique
					attribute = kwcClass->LookupAttribute(logicalAttribute->GetName());
					if (attribute != NULL and attribute->GetLoaded())
					{
						bOk = false;
						AddError("The variable " + logicalAttribute->GetName() +
							 " in the dictionary " + GetClassName() +
							 " is missing in the database file");
					}
					// Warning sinon
					else
						AddWarning("The variable " + logicalAttribute->GetName() +
							   " in the dictionary " + GetClassName() +
							   " is missing in the database file (variable not required)");
				}
			}
			// Test pour les blocs d'attribut natifs, uniquement pour le premier attribut de chaque bloc
			else if (logicalAttribute->IsFirstInBlock())
			{
				logicalAttributeBlock = logicalAttribute->GetAttributeBlock();

				// Recherche du champ correspondant si bloc d'attribut natif
				if (logicalAttributeBlock->IsNative() and
				    kwcHeaderLineClass->LookupAttribute(logicalAttributeBlock->GetName()) == NULL)
				{
					// Erreur si necessaire dans la classe physique
					attributeBlock =
					    kwcClass->LookupAttributeBlock(logicalAttributeBlock->GetName());
					if (attributeBlock != NULL and attributeBlock->GetLoaded())
					{
						bOk = false;
						AddError("The sparse variable block " +
							 logicalAttributeBlock->GetName() + " in the dictionary " +
							 GetClassName() + " is missing in the database file");
					}
					// Warning sinon
					else
						AddWarning("The sparse variable block " +
							   logicalAttributeBlock->GetName() + " in the dictionary " +
							   GetClassName() +
							   " is missing in the database file (sparse variable block "
							   "not required)");
				}
			}

			// Attribut suivant
			kwcLogicalClass->GetNextAttribute(logicalAttribute);
		}

		// Calcul des index des attribut de la cle
		if (bOk and kwcLogicalClass->GetRoot())
		{
			// Initialisation
			ivRootKeyIndexes.SetSize(kwcHeaderLineClass->GetUsedAttributeNumber());

			// Parcours des champs de l'entete
			for (i = 0; i < kwcHeaderLineClass->GetUsedAttributeNumber(); i++)
			{
				headerLineAttribute = kwcHeaderLineClass->GetUsedAttributeAt(i);

				// Recherche s'il s'agit d'un attribut de la cle
				keyFieldIndex =
				    cast(IntObject*, odKeyFieldIndexes.Lookup(headerLineAttribute->GetName()));

				// Indexation eventuelle dans le cas d'un attribut de la cle
				ivRootKeyIndexes.SetAt(i, -1);
				if (keyFieldIndex != NULL)
					ivRootKeyIndexes.SetAt(i, keyFieldIndex->GetInt());
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////
	// Si pas de ligne d'entete, on prend les champs dans l'ordre des attributs natifs
	// de la classe logique, puis on cherche la correspondance dans la classe physique
	else
	{
		assert(bOk);
		assert(kwcHeaderLineClass == NULL);

		// Collecte des champs natifs de la classe logique
		logicalAttribute = kwcLogicalClass->GetHeadAttribute();
		while (logicalAttribute != NULL)
		{
			// Memorisation si attribut natif
			if (not logicalAttribute->IsInBlock() and logicalAttribute->IsNative())
				oaNativeLogicalDataItems.Add(logicalAttribute);
			// Test pour les blocs d'attribut, uniquement pour le premier attribut de chaque bloc
			else if (logicalAttribute->IsFirstInBlock())
			{
				logicalAttributeBlock = logicalAttribute->GetAttributeBlock();

				// Memorisation si bloc d'attribut natif
				if (logicalAttributeBlock->IsNative())
					oaNativeLogicalDataItems.Add(logicalAttributeBlock);
			}

			// Attribut suivant
			kwcLogicalClass->GetNextAttribute(logicalAttribute);
		}

		// Parcours des champs logiques
		livDataItemLoadIndexes.SetSize(oaNativeLogicalDataItems.GetSize());
		for (i = 0; i < oaNativeLogicalDataItems.GetSize(); i++)
		{
			// Recherche de l'element de donnee logique correspondant
			logicalDataItem = cast(KWDataItem*, oaNativeLogicalDataItems.GetAt(i));

			// Cas d'un attribut
			if (logicalDataItem->IsAttribute())
			{
				// Recherche de l'attribut logique correspondant
				logicalAttribute = cast(KWAttribute*, logicalDataItem);

				// Recherche de l'attribut physique correspondant
				attribute = kwcClass->LookupAttribute(logicalAttribute->GetName());

				// Indexation eventuelle de l'attribut
				livDataItemLoadIndexes.ResetAt(i);
				if (attribute != NULL)
				{
					assert(attribute->IsNative());
					livDataItemLoadIndexes.SetAt(i, attribute->GetLoadIndex());
				}
			}
			// Cas d'un bloc d'attributs
			else
			{
				// Recherche du bloc d'attributs logiques correspondant
				logicalAttributeBlock = cast(KWAttributeBlock*, logicalDataItem);

				// Recherche du bloc d'attributs physiques correspondant
				attributeBlock = kwcClass->LookupAttributeBlock(logicalAttributeBlock->GetName());

				// Indexation eventuelle du bloc d'attributs
				livDataItemLoadIndexes.ResetAt(i);
				if (attributeBlock != NULL)
				{
					assert(attributeBlock->GetDerivationRule() == NULL);
					livDataItemLoadIndexes.SetAt(i, attributeBlock->GetLoadIndex());
				}
			}
		}

		// Calcul des index des attribut de la cle
		if (bOk and kwcLogicalClass->GetRoot())
		{
			// Initialisation
			ivRootKeyIndexes.SetSize(oaNativeLogicalDataItems.GetSize());

			// Parcours des champs logiques
			livDataItemLoadIndexes.SetSize(oaNativeLogicalDataItems.GetSize());
			for (i = 0; i < oaNativeLogicalDataItems.GetSize(); i++)
			{
				// Recherche de l'element de donnee logique correspondant
				logicalDataItem = cast(KWDataItem*, oaNativeLogicalDataItems.GetAt(i));

				// Recherche s'il s'agit d'un attribut de la cle
				keyFieldIndex = cast(IntObject*, odKeyFieldIndexes.Lookup(logicalDataItem->GetName()));

				// Indexation eventuelle dans le cas d'un attribut de la cle
				ivRootKeyIndexes.SetAt(i, -1);
				if (keyFieldIndex != NULL)
					ivRootKeyIndexes.SetAt(i, keyFieldIndex->GetInt());
			}
		}
	}
	Global::DesactivateErrorFlowControl();

	// Nettoyage
	odKeyFieldIndexes.DeleteAll();

	// Affichage du resultat d'indexation
	if (bDisplay)
	{
		cout << "Compute data item indexes of dictionary " << kwcClass->GetName() << endl;

		// Cas avec classe de ligne d'entete
		if (kwcHeaderLineClass != NULL)
		{
			for (i = 0; i < kwcHeaderLineClass->GetUsedAttributeNumber(); i++)
			{
				headerLineAttribute = kwcHeaderLineClass->GetUsedAttributeAt(i);
				dataItem = kwcClass->LookupDataItem(headerLineAttribute->GetName());
				cout << "\t" << i;
				cout << "\t" << headerLineAttribute->GetName();
				cout << "\t" << livDataItemLoadIndexes.GetAt(i);
				if (dataItem != NULL)
				{
					if (dataItem->IsAttribute())
						cout << "\t" << cast(KWAttribute*, dataItem)->GetName();
					else
						cout << "\t" << cast(KWAttributeBlock*, dataItem)->GetName();
				}
				cout << endl;
			}
		}
		// Cas sans ligne d'entete
		else
		{
			cout << "\tNo header line dictionary" << endl;
			cout << "\tIndexed data items: " << livDataItemLoadIndexes.GetSize() << endl;
		}

		// Affichage des correspondances entre champs de la cle et leur index dans le fichier
		if (kwcClass->GetRoot())
		{
			cout << "Compute root key indexes of dictionary " << kwcClass->GetName() << endl;
			for (i = 0; i < ivRootKeyIndexes.GetSize(); i++)
			{
				if (ivRootKeyIndexes.GetAt(i) != -1)
				{
					cout << "\t"
					     << "Key" << ivRootKeyIndexes.GetAt(i) + 1 << "\t"
					     << kwcLogicalClass->GetKeyAttributeNameAt(ivRootKeyIndexes.GetAt(i))
					     << "\t" << i << endl;
				}
			}
		}
	}
	assert(not bOk or lastReadRootKey.GetSize() > 0 or not kwcClass->GetRoot());
	return bOk;
}

boolean KWDataTableDriverTextFile::OpenInputDatabaseFile()
{
	boolean bOk;
	int nPreferredBufferSize;

	require(inputBuffer == NULL);
	require(outputBuffer == NULL);

	// Initialisation des caracteristiques du fichier
	bWriteMode = false;

	// Initialisation du buffer
	inputBuffer = new InputBufferedFile;
	inputBuffer->SetFieldSeparator(cFieldSeparator);
	inputBuffer->SetHeaderLineUsed(bHeaderLineUsed);
	inputBuffer->SetFileName(GetDataTableName());
	inputBuffer->SetSilentMode(GetSilentMode());

	// Ouverture du fichier
	bOk = inputBuffer->Open();

	// Taille de buffer reduite si fichier de petite taille
	// Le fichier doit avoir ete ouvert pour acceder a sa taille
	if (bOk)
	{
		// Ajustement de la taille du buffer en fonction de la taille du fichier
		if (inputBuffer->GetFileSize() < nBufferSize)
		{
			inputBuffer->SetBufferSize((int)inputBuffer->GetFileSize());
			inputBuffer->SetCacheOn(false);
		}
		// Prise en compte du cache selon lma taille disponible pour le buffer
		else
		{
			nPreferredBufferSize = PLRemoteFileService::GetPreferredBufferSize(GetDataTableName());
			if (nBufferSize >= nPreferredBufferSize + SystemFile::nMinPreferredBufferSize)
			{
				inputBuffer->SetBufferSize(nBufferSize - nPreferredBufferSize);
				inputBuffer->SetCacheOn(true);
			}
			else
			{
				inputBuffer->SetBufferSize(nBufferSize);
				inputBuffer->SetCacheOn(false);
			}
		}
	}

	// Nettoyage si erreur
	if (not bOk)
	{
		delete inputBuffer;
		inputBuffer = NULL;
	}
	return bOk;
}

boolean KWDataTableDriverTextFile::OpenOutputDatabaseFile()
{
	boolean bOk;

	require(inputBuffer == NULL);
	require(outputBuffer == NULL);

	// Initialisation des caracteristiques du fichier
	bWriteMode = false;

	// Initialisation du buffer
	outputBuffer = new OutputBufferedFile;
	outputBuffer->SetFieldSeparator(cFieldSeparator);
	outputBuffer->SetHeaderLineUsed(bHeaderLineUsed);
	outputBuffer->SetFileName(GetDataTableName());
	outputBuffer->SetBufferSize(nBufferSize);
	outputBuffer->SetSilentMode(GetSilentMode());

	// Ouverture du fichier
	bOk = outputBuffer->Open();
	if (bOk)
		bWriteMode = true;

	// Nettoyage si erreur
	if (not bOk)
	{
		delete outputBuffer;
		outputBuffer = NULL;
	}
	return bOk;
}

boolean KWDataTableDriverTextFile::CloseDatabaseFile()
{
	boolean bOk;

	require((bWriteMode and outputBuffer != NULL) or (not bWriteMode and inputBuffer != NULL));

	// Cas ecriture
	if (bWriteMode)
	{
		bOk = outputBuffer->Close();
		delete outputBuffer;
		outputBuffer = NULL;
	}
	// Cas lecture
	else
	{
		bOk = inputBuffer->Close();
		delete inputBuffer;
		inputBuffer = NULL;
	}
	return bOk;
}

void KWDataTableDriverTextFile::ResetDatabaseFile()
{
	lRecordIndex = 0;
	lUsedRecordNumber = 0;
}
