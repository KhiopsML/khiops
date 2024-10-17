// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CommandFile.h"

CommandFile::CommandFile()
{
	fInputCommands = NULL;
	fOutputCommands = NULL;
	bPrintOutputInConsole = false;
	assert(nMaxLineLength < BUFFER_LENGTH);
	assert(nMaxVariableNameLength + nMaxVariableNameLength < nMaxLineLength);
}

CommandFile::~CommandFile()
{
	require(not AreCommandFilesOpened());
}

void CommandFile::Reset()
{
	require(not AreCommandFilesOpened());

	SetInputCommandFileName("");
	SetOutputCommandFileName("");
	DeleteAllInputSearchReplaceValues();
	SetInputParameterFileName("");
}

void CommandFile::SetInputCommandFileName(const ALString& sFileName)
{
	require(not AreCommandFilesOpened());

	sInputCommandFileName = sFileName;
}

const ALString& CommandFile::GetInputCommandFileName() const
{
	return sInputCommandFileName;
}

void CommandFile::SetOutputCommandFileName(const ALString& sFileName)
{
	require(not AreCommandFilesOpened());

	sOutputCommandFileName = sFileName;

	// Si les fichier de sortie est /dev/stdout ou /dev/stderr, c'est une redirection vers la console
	bPrintOutputInConsole = false;
#ifdef __linux_or_apple__
	if (sOutputCommandFileName == "/dev/stdout" or sLocalOutputCommandsFileName == "/dev/stderr")
		bPrintOutputInConsole = true;
#endif
}

const ALString& CommandFile::GetOutputCommandFileName() const
{
	return sOutputCommandFileName;
}

boolean CommandFile::GetPrintOutputInConsole() const
{
	return bPrintOutputInConsole;
}

void CommandFile::AddInputSearchReplaceValues(const ALString& sSearchValue, const ALString& sReplaceValue)
{
	require(sSearchValue != "");

	require(not AreCommandFilesOpened());

	// Ajout des valeurs
	svInputCommandSearchValues.Add(sSearchValue);
	svInputCommandReplaceValues.Add(sReplaceValue);
}

int CommandFile::GetInputSearchReplaceValueNumber() const
{
	assert(svInputCommandSearchValues.GetSize() == svInputCommandReplaceValues.GetSize());
	return svInputCommandSearchValues.GetSize();
}

const ALString& CommandFile::GetInputSearchValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetInputSearchReplaceValueNumber());
	return svInputCommandSearchValues.GetAt(nIndex);
}

const ALString& CommandFile::GetInputReplaceValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetInputSearchReplaceValueNumber());
	return svInputCommandReplaceValues.GetAt(nIndex);
}

void CommandFile::DeleteAllInputSearchReplaceValues()
{
	svInputCommandSearchValues.SetSize(0);
	svInputCommandReplaceValues.SetSize(0);
}

void CommandFile::SetInputParameterFileName(const ALString& sFileName)
{
	require(not AreCommandFilesOpened());

	sInputParameterFileName = sFileName;
}

const ALString& CommandFile::GetInputParameterFileName() const
{
	return sInputParameterFileName;
}

boolean CommandFile::Check() const
{
	boolean bOk;

	// Le parametrage des search/replace ne peut pas etre utilise s'il n'y a pas de fichier de commande en entree
	if (GetInputSearchReplaceValueNumber() > 0 and GetInputCommandFileName() == "")
		bOk = false;

	// Le fichier de parametre json ne peut pas etre utilise s'il n'y a pas de fichier de commande en entree
	if (GetInputParameterFileName() != "" and GetInputCommandFileName() == "")
		bOk = false;

	// Le fichier de parametre json et le parametrage des search/replace sont exclusifs
	if (GetInputParameterFileName() != "" and GetInputSearchReplaceValueNumber() > 0)
		bOk = false;

	return true;
}

boolean CommandFile::OpenInputCommandFile()
{
	boolean bOk;

	require(Check());
	require(GetInputCommandFileName() != "");
	require(not IsInputCommandFileOpened());

	// Copie depuis HDFS si necessaire
	bOk = PLRemoteFileService::BuildInputWorkingFile(sInputCommandFileName, sLocalInputCommandFileName);

	// Fermeture du fichier si celui-ci est deja ouvert
	// Ce cas peut arriver si on appelle plusieurs fois ParseParameters (notamment via MODL_dll)
	if (fInputCommands != NULL)
	{
		fclose(fInputCommands);
		fInputCommands = NULL;
	}

	// Ouverture du fichier en lecture
	if (bOk)
		fInputCommands = p_fopen(sLocalInputCommandFileName, "r");
	if (fInputCommands == NULL)
	{
		AddInputCommandFileError("Unable to open file");
		bOk = false;
	}

	// Chargement des parametre json si specifie
	if (bOk and GetInputParameterFileName() != "")
	{
		bOk = LoadJsonParameters();

		// Message d'erreur synthetique
		AddInputParameterFileError("Unable to exploit json parameters");
	}
	return bOk;
}

boolean CommandFile::IsInputCommandFileOpened() const
{
	return fInputCommands != NULL;
}

boolean CommandFile::OpenOutputCommandFile()
{
	boolean bOk;

	require(Check());
	require(GetOutputCommandFileName() != "");
	require(not IsOutputCommandFileOpened());

	// Creation si necessaire des repertoires intermediaires
	PLRemoteFileService::MakeDirectories(FileService::GetPathName(sOutputCommandFileName));
	assert(FileService::GetApplicationTmpDir().IsEmpty());

	// PPreparation pour HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sOutputCommandFileName, sLocalOutputCommandFileName);

	// Fermeture du fichier si celui-ci est deja ouvert
	// Ce cas peut arriver si on appelle plusieurs fois ParseParameters (notamment via MODL_dll)
	if (fOutputCommands != NULL)
	{
		fclose(fOutputCommands);
		fOutputCommands = NULL;
	}

	// Ouverture du fichier en ecriture
	if (bOk)
		fOutputCommands = p_fopen(sLocalOutputCommandFileName, "w");
	if (fOutputCommands == NULL)
	{
		AddOutputCommandFileError("Unable to open file");
		bOk = false;
	}
	return bOk;
}

boolean CommandFile::IsOutputCommandFileOpened() const
{
	return fOutputCommands != NULL;
}

boolean CommandFile::AreCommandFilesOpened() const
{
	return IsInputCommandFileOpened() or IsOutputCommandFileOpened();
}

void CommandFile::CloseInputCommandFile()
{
	// Fermeture du fichier d'entree
	if (fInputCommands != NULL)
	{
		fclose(fInputCommands);
		fInputCommands = NULL;

		// Si le fichier est sur HDFS, on supprime la copie locale
		PLRemoteFileService::CleanInputWorkingFile(sInputCommandFileName, sLocalInputCommandFileName);
	}

	// Nettoyage des parametres json
	jsonParameters.DeleteAll();
}

void CommandFile::CloseOutputCommandFile()
{
	// Fermeture du fichier de sortie, uniquement en mode batch (sinon, on enregistre toujours le scenario)
	if (fOutputCommands != NULL)
	{
		fclose(fOutputCommands);
		fOutputCommands = NULL;

		// Copie vers HDFS si necessaire
		PLRemoteFileService::CleanOutputWorkingFile(sOutputCommandFileName, sLocalOutputCommandFileName);
	}
}

void CommandFile::CloseCommandFiles()
{
	CloseInputCommandFile();
	CloseOutputCommandFile();
}

boolean CommandFile::ReadInputCommand(StringVector* svIdentifierPath, ALString& sValue)
{
	const char cDEL = (char)127;
	boolean bContinueParsing;
	char sCharBuffer[1 + BUFFER_LENGTH];
	ALString sInputLine;
	ALString sBuffer;
	int nLineIndex;
	int nLength;
	ALString sIdentifierPath;
	int nPosition;
	int nToken;
	ALString sEndLine;
	ALString sToken;
	ALString sIdentifier;
	int i;
	ALString sTmp;

	require(svIdentifierPath != NULL);
	require(svIdentifierPath->GetSize() == 0);
	assert(GetInputSearchReplaceValueNumber() == 0 or GetInputParameterFileName() == "");

	// On arrete si pas de fichier ou si fin de fichier
	if (fInputCommands == NULL or feof(fInputCommands))
		return false;

	////////////////////////////////////////////////////////////////////////////////////////////
	// Recherche d'une ligne non vide a traiter

	// Boucle de lecture pour ignorer les lignes vides ou ne comportant que des commentaires
	bContinueParsing = true;
	nLineIndex = 0;
	while (sInputLine == "" and not feof(fInputCommands))
	{
		nLineIndex++;

		// Lecture
		StandardGetInputString(sCharBuffer, fInputCommands);
		sInputLine = sCharBuffer;

		// Si erreur ou caractere fin de fichier, on n'arret sans message d'erreur
		// Si on est pas en fin de fichier, on a forcement un caractere '\n' en fin de ligne
		if (ferror(fInputCommands) or sInputLine.GetLength() == 0)
			bContinueParsing = false;

		// Erreur si ligne trop longue
		if (bContinueParsing and sInputLine.GetLength() > nMaxLineLength)
		{
			bContinueParsing = false;
			AddInputCommandFileError(sTmp + "line " + IntToString(nLineIndex) + " too long, with length " +
						 IntToString(sInputLine.GetLength()) + " > " +
						 IntToString(nMaxLineLength));
		}

		// Suppression des blancs au debut et a la fin, donc du dernier caractere fin de ligne
		sInputLine.TrimRight();
		sInputLine.TrimLeft();

		// Ajout si necessaire d'un commentaire vide a la fin, ce qui permet de gerer
		// les search/replace sans se soucier des valeurs de remplacement comportant
		// des caracteres "//", en les isolant du commentaire de fin de ligne
		if (sInputLine.Find(sCommentPrefix) == -1)
		{
			if (sInputLine != "")
				sInputLine += ' ';
			sInputLine += sCommentPrefix;
		}

		// Dans le cas sans fichier de parametrage json, on que l'on n'utilise pas
		// des balises du langage // de pilotage par fichier de parametre
		if (bContinueParsing and sInputLine != "" and GetInputParameterFileName() == "")
		{
			// Detection du premier token de la ligne
			nToken = TokenizeInputCommand(sInputLine, sToken, sEndLine);

			// Arret si detection de token reserves au cas des fichiers de parametres json
			if (nToken == TokenLoop or nToken == TokenIf or nToken == TokenEnd or nToken == TokenVariable)
			{
				bContinueParsing = false;
				AddInputCommandFileError(sTmp + "line " + IntToString(nLineIndex) + " : use of the \"" +
							 GetPrintableValue(sToken) +
							 "\" token alllowed only with a json parameter file");
			}

			// Detection d'un second token dans la ligne
			if (bContinueParsing and nToken != TokenComment)
			{
				sBuffer = sEndLine;
				nToken = TokenizeInputCommand(sBuffer, sToken, sEndLine);

				// Arret si detection de token reserves au cas des fichier de parametres json
				if (nToken == TokenLoop or nToken == TokenIf or nToken == TokenEnd or
				    nToken == TokenVariable)
				{
					bContinueParsing = false;
					AddInputCommandFileError(sTmp + "line " + IntToString(nLineIndex) +
								 " : use of the \"" + GetPrintableValue(sToken) +
								 "\" key alllowed only with a json parameter file");
				}
			}
		}

		// Arret si necessaire
		if (not bContinueParsing)
		{
			// Nettoyage
			CloseInputCommandFile();
			return false;
		}

		// Cas du mode recherche/remplacement dans la partie valeur de la commande
		if (GetInputSearchReplaceValueNumber() > 0)
		{
			sInputLine = ProcessSearchReplaceCommand(sInputLine);
		}

		// Suppression du commentaire de fin de ligne, ce qui permet d'avoir
		// des paires (IdentifierPath, valeur) avec valeur contenant des " //"
		assert(sInputLine.Find(sCommentPrefix) >= 0);
		nLength = sInputLine.GetLength();
		for (i = nLength - sCommentPrefix.GetLength(); i >= 0; i--)
		{
			if (sInputLine.GetAt(i) == sCommentPrefix.GetAt(0) and
			    sInputLine.Right(nLength - i).Find(sCommentPrefix) == 0)
			{
				sInputLine.GetBufferSetLength(i);
				sInputLine.TrimRight();
				break;
			}
		}

		// Suppression si necessaire du dernier caractere s'il s'agit de cDEL
		// (methode FromJstring qui utilise ce cartactere special en fin de chaine pour
		// les conversions entre Java et C++)
		if (sInputLine.GetLength() > 0 and sInputLine.GetAt(sInputLine.GetLength() - 1) == cDEL)
			sInputLine.GetBufferSetLength(sInputLine.GetLength() - 1);

		// On supprime la ligne si elle commence par un commentaire
		// Cela permet de commenter y compris les lignes ayant une valeur contenant des "//"
		if (sInputLine.Find(sCommentPrefix) == 0)
			sInputLine = "";
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	// Analyse de la ligne a traiter

	// A ce stade, soit on a quite la methode, soit on a une ligne non vide a analyser
	assert(sInputLine.GetLength() > 0);

	// Recherche de l'IdentifierPath et de la valeur
	nPosition = sInputLine.Find(' ');
	if (nPosition == -1)
	{
		sIdentifierPath = sInputLine;
		sValue = "";
	}
	else
	{
		sIdentifierPath = sInputLine.Left(nPosition);
		sValue = sInputLine.Right(sInputLine.GetLength() - nPosition - 1);
		sValue.TrimRight();
		sValue.TrimLeft();
	}

	// Parsing de l'IdentifierPath: recherche des '.'
	while (sIdentifierPath != "")
	{
		// Recherche d'un '.', sinon de la fin de la ligne
		nPosition = sIdentifierPath.Find('.');
		if (nPosition == -1)
			nPosition = sIdentifierPath.GetLength();
		assert(nPosition >= 0);

		// Recherche d'un Identifier du PathIdentifier
		sToken = sIdentifierPath.Left(nPosition);
		sToken.TrimRight();
		sToken.TrimLeft();
		if (sToken != "")
		{
			sIdentifier = sToken;
			svIdentifierPath->Add(sIdentifier);
		}
		else
		{
			// Erreur dans la syntaxe de la commande
			svIdentifierPath->SetSize(0);
			break;
		}

		// Passage a la suite du buffer
		if (nPosition < sIdentifierPath.GetLength())
			sIdentifierPath = sIdentifierPath.Right(sIdentifierPath.GetLength() - nPosition - 1);
		else
			sIdentifierPath = "";
		sIdentifierPath.TrimRight();
		sIdentifierPath.TrimLeft();
	}

	// On renvoie le resultat
	if (svIdentifierPath->GetSize() != 0)
		return true;
	else
		return false;
}

void CommandFile::WriteOutputCommand(const ALString& sIdentifierPath, const ALString& sValue, const ALString& sLabel)
{
	if (fOutputCommands != NULL)
	{
		ALString sCommand;
		int nCommandLength;

		// Fabrication de la commande
		sCommand = sIdentifierPath;
		if (sValue != "")
			sCommand += " " + sValue;

		// Calcul de la position du libelle
		nCommandLength = 10 * ((9 + sCommand.GetLength()) / 10);
		if (nCommandLength < 30)
			nCommandLength = 30;

		// Impression
		if (bPrintOutputInConsole)
			// Si redirection vers la console, on ajoute un prefixe
			fprintf(fOutputCommands, "Khiops.command\t");
		if (sCommand == "" and sLabel == "")
			fprintf(fOutputCommands, "\n");
		else if (sCommand == "")
			fprintf(fOutputCommands, sCommentPrefix + " %s\n", (const char*)sLabel);
		else
			fprintf(fOutputCommands, "%-*.*s // %s\n", nCommandLength, nCommandLength,
				(const char*)sCommand, (const char*)sLabel);
		fflush(fOutputCommands);
	}
}

void CommandFile::WriteOutputCommandHeader()
{
	WriteOutputCommand("", "", "Output command file");
	WriteOutputCommand("", "", "");
	WriteOutputCommand("", "", "This file contains recorded commands, that can be replayed.");
	WriteOutputCommand("", "", "Commands are based on user interactions:");
	WriteOutputCommand("", "", "\tfield update");
	WriteOutputCommand("", "", "\tlist item selection");
	WriteOutputCommand("", "", "\tmenu action");
	WriteOutputCommand("", "", "Every command can be commented, using .");
	WriteOutputCommand("", "", "For example, commenting the last Exit command will allow other");
	WriteOutputCommand("", "", "user interactions, after the commands have been replayed.");
	WriteOutputCommand("", "", "");
}

void CommandFile::AddInputCommandFileError(const ALString& sMessage) const
{
	Global::AddError("Input command file", sInputCommandFileName, sMessage);
}

void CommandFile::AddInputParameterFileError(const ALString& sMessage) const
{
	Global::AddError("Input json parameter file", sInputParameterFileName, sMessage);
}

void CommandFile::AddOutputCommandFileError(const ALString& sMessage) const
{
	Global::AddError("Input command file", sOutputCommandFileName, sMessage);
}

const ALString CommandFile::GetPrintableValue(const ALString& sValue) const
{
	if (sValue.GetLength() <= nMaxPrintableLength)
		return sValue;
	else
		return sValue.Left(nMaxPrintableLength) + "...";
}

boolean CommandFile::LoadJsonParameters()
{
	boolean bOk = true;
	longint lInputParameterFileSize;
	JsonMember* jsonMember;
	JsonArray* jsonArray;
	JsonValue* jsonValue;
	JsonObject* firstJsonSubObject;
	JsonObject* jsonSubObject;
	JsonMember* jsonSubObjectMember;
	JsonMember* firstJsonSubObjectMember;
	ALString sMessage;
	boolean bIsFirstArrayObjectValid;
	boolean bIsArrayObjectValid;
	int i;
	int j;
	int k;
	ALString sLabelSuffix;
	ALString sTmp;

	require(GetInputParameterFileName() != "");

	// Nettoyage prealable
	jsonParameters.DeleteAll();

	// Test sur la longueur max du fichier de parametree
	lInputParameterFileSize = PLRemoteFileService::GetFileSize(GetInputParameterFileName());
	if (lInputParameterFileSize > lMaxInputParameterFileSize)
	{
		AddInputParameterFileError(
		    sTmp + "the size of the parameter file (" + LongintToHumanReadableString(lInputParameterFileSize) +
		    ") exceeds the limit of " + LongintToHumanReadableString(lMaxInputParameterFileSize));
		bOk = false;
	}

	// Chargement du fichier json
	if (bOk)
		bOk = jsonParameters.ReadFile(GetInputParameterFileName());

	// Verification de la structure, qui doit respecter l'ensemble des contraintes du parametrage json
	if (bOk)
	{
		// Analyse des membres de l'objet principal
		Global::ActivateErrorFlowControl();
		for (i = 0; i < jsonParameters.GetMemberNumber(); i++)
		{
			jsonMember = jsonParameters.GetMemberAt(i);

			// Verification de la cle
			if (not CheckVariableName(jsonMember->GetKey(), sMessage))
			{
				AddInputParameterFileError("in main json object, " + sMessage);
				bOk = false;
			}
			// Verification une seule fois de la non-collision de la cle avec sa variante de type byte
			else if (IsByteVariableName(jsonMember->GetKey()) and
				 jsonParameters.LookupMember(ToStandardVariableName(jsonMember->GetKey())) != NULL)
			{
				AddInputParameterFileError("in main json object, the \"" +
							   ToStandardVariableName(jsonMember->GetKey()) +
							   "\" key is used twice, along with its \"" +
							   jsonMember->GetKey() + "\" byte variant");
				bOk = false;
			}
			// Verification du type, au premier niveau de la structure de l'objet json
			else if (jsonMember->GetValueType() != JsonObject::StringValue and
				 jsonMember->GetValueType() != JsonObject::NumberValue and
				 jsonMember->GetValueType() != JsonObject::BooleanValue and
				 jsonMember->GetValueType() != JsonObject::ArrayValue)
			{
				AddInputParameterFileError("in main json object, the " +
							   jsonMember->GetValue()->TypeToString() +
							   " type of value at " + BuildJsonPath(jsonMember, -1, NULL) +
							   " should be string, number, boolean or array");
				bOk = false;
			}
			// Verification du type string dans le cas d'une cle avec sa variante de type byte
			else if (IsByteVariableName(jsonMember->GetKey()) and
				 jsonMember->GetValueType() != JsonObject::StringValue)
			{
				AddInputParameterFileError(
				    "in main json object, the " + jsonMember->GetValue()->TypeToString() +
				    " type of value at " + BuildJsonPath(jsonMember, -1, NULL) +
				    " should be string, as the key prefix is \"" + sByteVariablePrefix + "\"");
				bOk = false;
			}
			// Verification de de la longueur et de l'encodage base64 de la valeur string
			else if (jsonMember->GetValueType() == JsonObject::StringValue and
				 not CheckStringValue(jsonMember->GetStringValue()->GetString(),
						      IsByteVariableName(jsonMember->GetKey()), sMessage))
			{
				AddInputParameterFileError("in main json object, at " +
							   BuildJsonPath(jsonMember, -1, NULL) + ", " + sMessage);
				bOk = false;
			}

			// Cas d'une valeur tableau
			if (jsonMember->GetValueType() == JsonObject::ArrayValue)
			{
				jsonArray = jsonMember->GetArrayValue();

				// Toutes les valeurs d'un tableau doivent etre de type objet
				bIsFirstArrayObjectValid = true;
				for (j = 0; j < jsonArray->GetValueNumber(); j++)
				{
					jsonValue = jsonArray->GetValueAt(j);
					bIsArrayObjectValid = true;

					// Verification du type, au premier niveau de la structure de l'objet json
					if (jsonValue->GetType() != JsonObject::ObjectValue)
					{
						AddInputParameterFileError("in array member, type " +
									   jsonValue->TypeToString() + " of value at " +
									   BuildJsonPath(jsonMember, j, NULL) +
									   " should be object");
						bOk = false;
						bIsArrayObjectValid = false;
					}
					// Analyse de l'objet si valide
					else
					{
						jsonSubObject = jsonValue->GetObjectValue();

						// Toutes les valeurs du subobjet dovent etre de type simple
						for (k = 0; k < jsonSubObject->GetMemberNumber(); k++)
						{
							jsonSubObjectMember = jsonSubObject->GetMemberAt(k);

							// Verification de la cle
							if (not CheckVariableName(jsonSubObjectMember->GetKey(),
										  sMessage))
							{
								AddInputParameterFileError(
								    "in object value of array at " +
								    BuildJsonPath(jsonMember, j, NULL) + ", " +
								    sMessage);
								bOk = false;
								bIsArrayObjectValid = false;
							}
							// Verification de la non-collision de la cle avec sa variante de type byte
							else if (IsByteVariableName(jsonSubObjectMember->GetKey()) and
								 jsonSubObject->LookupMember(ToStandardVariableName(
								     jsonSubObjectMember->GetKey())) != NULL)
							{
								AddInputParameterFileError(
								    "in object value of array at " +
								    BuildJsonPath(jsonMember, j, NULL) + ", the \"" +
								    ToStandardVariableName(
									jsonSubObjectMember->GetKey()) +
								    "\" key is used twice,  along with its \"" +
								    jsonSubObjectMember->GetKey() + "\" byte variant");
								bOk = false;
							}
							// Verification que la cle du sous-objet n'entre pas en collision avec une cle de l'objet principal
							else if (jsonParameters.LookupMember(
								     jsonSubObjectMember->GetKey()) != NULL or
								 jsonParameters.LookupMember(ToVariantVariableName(
								     jsonSubObjectMember->GetKey())) != NULL)
							{
								// Message d'erreur avec precision dans le cas d'utilisation de variantes standard ou byte differents
								sLabelSuffix = "";
								if (jsonParameters.LookupMember(ToVariantVariableName(
									jsonSubObjectMember->GetKey())) != NULL)
									sLabelSuffix = " (in any standard or "
										       "byte variants)";
								AddInputParameterFileError(
								    "in object value of array at " +
								    BuildJsonPath(jsonMember, j, NULL) + ", the \"" +
								    jsonSubObjectMember->GetKey() +
								    "\" key is already used in main json "
								    "object" +
								    sLabelSuffix);
								bOk = false;
								bIsArrayObjectValid = false;
							}
							// Verification du type, au second niveau de la structure de l'objet json
							else if (jsonSubObjectMember->GetValueType() !=
								     JsonObject::StringValue and
								 jsonSubObjectMember->GetValueType() !=
								     JsonObject::NumberValue)
							{
								AddInputParameterFileError(
								    "in object value of array, the " +
								    jsonSubObjectMember->GetValue()->TypeToString() +
								    " type of value at " +
								    BuildJsonPath(jsonMember, j, jsonSubObjectMember) +
								    " should be string or number");
								bIsArrayObjectValid = false;
							}
							// Verification du type string dans le cas d'une cle avec sa variante de type byte
							else if (IsByteVariableName(jsonSubObjectMember->GetKey()) and
								 jsonSubObjectMember->GetValueType() !=
								     JsonObject::StringValue)
							{
								AddInputParameterFileError(
								    "in object value of array, the " +
								    jsonSubObjectMember->GetValue()->TypeToString() +
								    " type of value at " +
								    BuildJsonPath(jsonMember, j, jsonSubObjectMember) +
								    " should be string, as the key prefix is \"" +
								    sByteVariablePrefix + "\"");
								bOk = false;
							}
							// Verification de de la longueur et de l'encodage base64 de la valeur string
							else if (jsonSubObjectMember->GetValueType() ==
								     JsonObject::StringValue and
								 not CheckStringValue(
								     jsonSubObjectMember->GetStringValue()->GetString(),
								     IsByteVariableName(jsonSubObjectMember->GetKey()),
								     sMessage))
							{
								AddInputParameterFileError(
								    "in object value of array, at " +
								    BuildJsonPath(jsonMember, j, jsonSubObjectMember) +
								    ", " + sMessage);
								bOk = false;
							}
						}
					}
					if (j == 0)
						bIsFirstArrayObjectValid = bIsArrayObjectValid;

					// Verification de la coherence de structure avec le premier objet du tableau
					if (bIsArrayObjectValid and bIsFirstArrayObjectValid and j > 0)
					{
						firstJsonSubObject = jsonArray->GetValueAt(0)->GetObjectValue();
						jsonSubObject = jsonArray->GetValueAt(j)->GetObjectValue();

						// Le nombre de membre doit etre egal
						if (jsonSubObject->GetMemberNumber() !=
						    firstJsonSubObject->GetMemberNumber())
						{
							AddInputParameterFileError(
							    "number of members in object " +
							    BuildJsonPath(jsonMember, j, NULL) + " (" +
							    IntToString(jsonSubObject->GetMemberNumber()) +
							    " member) should be the same as in the the first object "
							    "in the array at " +
							    BuildJsonPath(jsonMember, 0, NULL) + " (" +
							    IntToString(firstJsonSubObject->GetMemberNumber()) +
							    " member)");
							bOk = false;
						}
						// Sinon, on compare les cle et type membre a membre
						else
						{
							for (k = 0; k < jsonSubObject->GetMemberNumber(); k++)
							{
								jsonSubObjectMember = jsonSubObject->GetMemberAt(k);
								firstJsonSubObjectMember =
								    firstJsonSubObject->GetMemberAt(k);

								// Verification de la coherence de la cle
								if (jsonSubObjectMember->GetKey() !=
									firstJsonSubObjectMember->GetKey() and
								    jsonSubObjectMember->GetKey() !=
									ToVariantVariableName(
									    firstJsonSubObjectMember->GetKey()))
								{
									AddInputParameterFileError(
									    "key \"" + jsonSubObjectMember->GetKey() +
									    "\" at " +
									    BuildJsonPath(jsonMember, j,
											  jsonSubObjectMember) +
									    " should be \"" +
									    firstJsonSubObjectMember->GetKey() +
									    "\", as in the the first object in the "
									    "array");
									bOk = false;
								}
								// Verification de la coherence de la valeur
								else if (jsonSubObjectMember->GetValueType() !=
									 firstJsonSubObjectMember->GetValueType())
								{
									AddInputParameterFileError(
									    "type " +
									    jsonSubObjectMember->GetValue()
										->TypeToString() +
									    " at " +
									    BuildJsonPath(jsonMember, j,
											  jsonSubObjectMember) +
									    " should be " +
									    firstJsonSubObjectMember->GetValue()
										->TypeToString() +
									    ", as in the the first object in the "
									    "array");
									bOk = false;
								}
							}
						}
					}
				}
			}
		}
		Global::DesactivateErrorFlowControl();
	}

	// Nettoyage en cas d'erreur
	if (not bOk)
		jsonParameters.DeleteAll();
	return bOk;
}

boolean CommandFile::CheckVariableName(const ALString& sValue, ALString& sMessage) const
{
	boolean bOk = true;
	ALString sTmp;

	// Test si non vide
	sMessage = "";
	if (bOk)
	{
		bOk = sValue.GetLength() > 0;
		if (not bOk)
			sMessage = "empty key";
	}

	// Test de la longueur max
	if (bOk)
	{
		bOk = sValue.GetLength() <= nMaxVariableNameLength;
		if (not bOk)
			sMessage = sTmp + "key \"" + GetPrintableValue(sValue) + "\" too long, with length " +
				   IntToString(sValue.GetLength()) + " > " + IntToString(nMaxVariableNameLength);
	}

	// Test du format camelCase
	if (bOk)
	{
		bOk = IsCamelCaseVariableName(sValue);
		if (not bOk)
			sMessage = "incorrect key \"" + GetPrintableValue(sValue) + "\", which should be camelCase";
	}

	ensure(not bOk or sMessage == "");
	return bOk;
}

boolean CommandFile::IsCamelCaseVariableName(const ALString& sValue) const
{
	boolean bOk;
	char c;
	int i;

	// Le nom ne doit pas etre vide
	bOk = sValue.GetLength() > 0;

	// Test des type de caracteres sans utiliser isalpha ou Il doit commencer par une lettre minuscule
	for (i = 0; i < sValue.GetLength(); i++)
	{
		c = sValue.GetAt(i);

		// On doit etre en ascii
		bOk = isascii(c);

		// Le premier caractere doit etre une lettre minuscule
		if (bOk)
		{
			if (i == 0)
				bOk = isalpha(c) and islower(c);
			// Les autre caracteres doivent etre alpha-numeriques
			else
				bOk = isalnum(c);
		}

		// Arret si necessaire
		if (not bOk)
			break;
	}
	return bOk;
}

boolean CommandFile::IsByteVariableName(const ALString& sValue) const
{
	boolean bOk;
	char c;

	require(IsCamelCaseVariableName(sValue));

	// Test si on est prefixe correctement, suivi d'un caractere alphabetique en majuscule
	bOk = sValue.GetLength() > sByteVariablePrefix.GetLength();
	if (bOk)
		bOk = sValue.Left(sByteVariablePrefix.GetLength()) == sByteVariablePrefix;
	if (bOk)
	{
		c = sValue.GetAt(sByteVariablePrefix.GetLength());
		bOk = isalpha(c) and isupper(c);
	}
	return bOk;
}

const ALString CommandFile::ToByteVariableName(const ALString& sValue) const
{
	ALString sByteVariableName;

	require(IsCamelCaseVariableName(sValue));

	if (IsByteVariableName(sValue))
		return sValue;
	else
	{
		sByteVariableName = sValue;
		sByteVariableName.SetAt(0, char(toupper(sByteVariableName.GetAt(0))));
		sByteVariableName = sByteVariablePrefix + sByteVariableName;
		return sByteVariableName;
	}
}

const ALString CommandFile::ToStandardVariableName(const ALString& sValue) const
{
	ALString sStandardVariableName;

	require(IsCamelCaseVariableName(sValue));

	if (IsByteVariableName(sValue))
	{

		sStandardVariableName = sValue.Right(sValue.GetLength() - sByteVariablePrefix.GetLength());
		sStandardVariableName.SetAt(0, char(tolower(sStandardVariableName.GetAt(0))));
		return sStandardVariableName;
	}
	else
		return sValue;
}

const ALString CommandFile::ToVariantVariableName(const ALString& sValue) const
{
	require(IsCamelCaseVariableName(sValue));

	if (IsByteVariableName(sValue))
		return ToStandardVariableName(sValue);
	else
		return ToByteVariableName(sValue);
}

boolean CommandFile::CheckStringValue(const ALString& sValue, boolean bCheckBase64Encoding, ALString& sMessage) const
{
	boolean bOk = true;
	char* sBytes;
	ALString sTmp;

	// Test de la longueur max
	sMessage = "";
	if (bOk)
	{
		bOk = sValue.GetLength() <= nMaxStringValueLength;
		if (not bOk)
			sMessage = sTmp + "string value \"" + GetPrintableValue(sValue) + "\" too long, with length " +
				   IntToString(sValue.GetLength()) + " > " + IntToString(nMaxStringValueLength);
	}

	// Test de l'encodage base64
	if (bOk and bCheckBase64Encoding)
	{
		// Reherche d'un buffer de caracteres
		sBytes = StandardGetBuffer();
		assert(nMaxStringValueLength <= BUFFER_LENGTH);

		// Test de l'encodage base64
		bOk = TextService::Base64StringToBytes(sValue, sBytes) != -1;
		if (not bOk)
			sMessage = "incorrect string value \"" + GetPrintableValue(sValue) +
				   "\", which should be encoding using base64";
	}

	ensure(not bOk or sMessage == "");
	return bOk;
}

ALString CommandFile::BuildJsonPath(JsonMember* member, int nArrayRank, JsonMember* arrayObjectmember)
{
	ALString sJsonPath;

	require(member != NULL);
	require(arrayObjectmember == NULL or nArrayRank >= 0);

	// Construction du chemin
	sJsonPath = '"';
	sJsonPath += '/';
	sJsonPath += member->GetKey();
	if (nArrayRank >= 0)
	{
		sJsonPath += '/';
		sJsonPath += IntToString(nArrayRank);
	}
	if (arrayObjectmember != NULL)
	{
		sJsonPath += '/';
		sJsonPath += arrayObjectmember->GetKey();
	}
	sJsonPath += '"';
	return sJsonPath;
}

int CommandFile::TokenizeInputCommand(const ALString& sInputCommand, ALString& sToken, ALString& sEndLine) const
{
	int nOutputToken;
	ALString sBuffer;
	int nPos;

	require(sInputCommand != "");
	require(IsValueTrimed(sInputCommand));

	// Cas d'un token commentaire
	if (sInputCommand.Find(sCommentPrefix) == 0)
	{
		nOutputToken = TokenComment;
		sToken = sInputCommand;
		sEndLine = "";
	}
	// Cas d'un token de type variable
	else if (sInputCommand.Find(sVariableDelimiter) == 0)
	{
		nOutputToken = TokenVariable;

		// Recherche de la fin de la variable
		sBuffer = sInputCommand.Right(sInputCommand.GetLength() - sVariableDelimiter.GetLength());
		nPos = sBuffer.Find(sVariableDelimiter);
		if (nPos == -1)
		{
			sToken = sInputCommand;
			sEndLine = "";
		}
		else
		{
			sToken = sInputCommand.Left(nPos + 2 * sVariableDelimiter.GetLength());
			sEndLine = sInputCommand.Right(sInputCommand.GetLength() - sToken.GetLength());
		}
	}
	// Analyse de la ligne sinon
	else
	{
		assert(sInputCommand.Find(sCommentPrefix) != 0);
		assert(sInputCommand.Find(sVariableDelimiter) != 0);

		// Recherche du premier token
		nPos = sInputCommand.Find(" ");
		if (nPos == -1)
		{
			sToken = sInputCommand;
			sEndLine = "";
		}
		else
		{
			sToken = sInputCommand.Left(nPos);
			sEndLine = sInputCommand.Right(sInputCommand.GetLength() - nPos);
			sEndLine.TrimLeft();
		}

		// Identification du type de token
		if (sToken == sTokenLoop)
			nOutputToken = TokenLoop;
		else if (sToken == sTokenIf)
			nOutputToken = TokenIf;
		else if (sToken == sTokenEnd)
			nOutputToken = TokenLoop;
		else
			nOutputToken = TokenOther;
	}

	ensure(0 <= nOutputToken and nOutputToken < None);
	ensure(IsValueTrimed(sToken));
	ensure(sInputCommand.Find(sToken) == 0);
	ensure(sEndLine == "" or sInputCommand.Find(sEndLine) >= 0);
	ensure(sToken.GetLength() + sEndLine.GetLength() <= sInputCommand.GetLength());
	return nOutputToken;
}

boolean CommandFile::CollectTokenVariables(const ALString& sInputCommand, StringVector* svVariableNames,
					   ALString& sMessage) const
{
	boolean bOk = true;
	int nToken;
	ALString sBuffer;
	ALString sToken;
	ALString sEndLine;

	require(sInputCommand != "");
	require(IsValueTrimed(sInputCommand));
	require(sInputCommand.Find(sCommentPrefix) != 0);

	// Initialisations
	svVariableNames->SetSize(0);
	sMessage = "";

	// Analyse de la ligne
	sEndLine = sInputCommand;
	while (sEndLine != "")
	{
		// Tokenisation de la fin de ligne
		sBuffer = sEndLine;
		nToken = TokenizeInputCommand(sBuffer, sToken, sEndLine);

		// Arret dans le cas d'un commentaire
		if (nToken == TokenComment)
			break;
		// Analyse du token dans le cas d'une variable
		else if (nToken == TokenVariable)
		{
			bOk = CheckTokenVariable(sToken, sMessage);

			// Prise en compte si valide
			if (bOk)
				svVariableNames->Add(sToken);
			// Arret sinon
			else
				break;
		}
	}
	ensure(bOk or sMessage.GetLength() > 0);
	return bOk;
}

boolean CommandFile::CheckTokenVariable(const ALString& sToken, ALString& sMessage) const
{
	boolean bOk;
	ALString sVariableName;
	ALString sPattern;
	int nPos;

	require(sToken != "");
	require(IsValueTrimed(sToken));
	require(sToken.Find(sVariableDelimiter) == 0);

	// Initialisation du resultat
	bOk = true;
	sMessage = "";

	// Controle des delimiteur, et suppression de ceux-ci
	sVariableName = sToken.Right(sToken.GetLength() - sVariableDelimiter.GetLength());
	if (bOk)
	{
		nPos = sVariableName.Find(sVariableDelimiter);
		if (nPos == -1)
		{
			bOk = false;
			sMessage = "missing trailing delimiter \"" + sVariableDelimiter + "\" at the end of key \"" +
				   GetPrintableValue(sToken) + "\"";
		}

		// Le token a tester doit etre extrait en s'arretant si possible au second delimiteur trouve
		if (bOk)
		{
			sVariableName = sVariableName.Left(sVariableName.GetLength() - sVariableDelimiter.GetLength());

			// On ne doit appeler la methode qu'avec un token candidat, ne au plus deux delimiteurs
			assert(sVariableName.Find(sVariableDelimiter) == -1);
		}
		assert(bOk or nPos == sToken.GetLength() - sVariableDelimiter.GetLength());
	}

	// Verification de syntaxe de la variable
	if (bOk)
	{
		bOk = CheckVariableName(sVariableName, sMessage);

		// On rajoute les delimiteur dans le nom de variable du message
		if (not bOk)
		{
			sPattern = "\"" + GetPrintableValue(sVariableName) + "\"";
			nPos = sMessage.Find(sPattern);
			if (nPos != -1)
				sMessage = sMessage.Left(nPos) + "\"" + GetPrintableValue(sToken) + "\"" +
					   sMessage.Right(sMessage.GetLength() - sPattern.GetLength());
		}
	}

	// Verification que l'on n'utilise par une variante de type byte dans un fichier de commande en entree
	if (bOk and sVariableName.Find(sByteVariablePrefix) == 0)
	{
		bOk = false;
		sMessage = "prefix \"" + sByteVariablePrefix + "\" used in key \"" + GetPrintableValue(sToken) +
			   "\" not allowed in input command file";
	}
	return bOk;
}

const ALString CommandFile::ProcessSearchReplaceCommand(const ALString& sInputCommand) const
{
	ALString sInputString;
	ALString sBeginString;
	ALString sEndString;
	ALString sOutputCommand;
	ALString sSearchValue;
	ALString sReplaceValue;
	int nSearchPosition;
	int i;

	require(GetInputSearchReplaceValueNumber() > 0);

	// Parcours de toutes les paires search/replace a appliquer
	sOutputCommand = sInputCommand;
	for (i = 0; i < GetInputSearchReplaceValueNumber(); i++)
	{
		sSearchValue = GetInputSearchValueAt(i);
		sReplaceValue = GetInputReplaceValueAt(i);

		// Remplacement iteratif des pattern trouves a partir de la chaine pretraitee precedente
		if (sSearchValue.GetLength() > 0)
		{
			sBeginString = "";
			sEndString = sOutputCommand;
			nSearchPosition = 0;
			sOutputCommand = "";
			while (nSearchPosition >= 0 and sEndString.GetLength() > 0)
			{
				nSearchPosition = sEndString.Find(sSearchValue);

				// Si non trouve, on garde la fin de la chaine en cours de traitement
				if (nSearchPosition == -1)
					sOutputCommand += sEndString;
				// Sinon, en prend le debut puis la valeur de remplacement
				else
				{
					sBeginString = sEndString.Left(nSearchPosition);
					sEndString = sEndString.Right(sEndString.GetLength() -
								      sSearchValue.GetLength() - nSearchPosition);
					sOutputCommand += sBeginString + sReplaceValue;
				}
			}
		}
	}
	return sOutputCommand;
}

boolean CommandFile::IsValueTrimed(const ALString& sValue) const
{
	if (sValue.GetLength() == 0)
		return true;
	else if (iswspace(sValue.GetAt(0)))
		return false;
	else if (iswspace(sValue.GetAt(sValue.GetLength() - 1)))
		return false;
	else
		return true;
}

const ALString CommandFile::sCommentPrefix = "//";
const ALString CommandFile::sTokenLoop = "LOOP";
const ALString CommandFile::sTokenIf = "IF";
const ALString CommandFile::sTokenEnd = "END";
const ALString CommandFile::sVariableDelimiter = "__";
const ALString CommandFile::sByteVariablePrefix = "byte";
