// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CommandFile.h"

CommandFile::CommandFile()
{
	bBatchMode = false;
	bPrintOutputInConsole = false;
	fInputCommands = NULL;
	fOutputCommands = NULL;
	ResetParser();
	assert(nMaxLineLength < BUFFER_LENGTH);
	assert(nMaxJsonKeyLength + nMaxStringValueLength < nMaxLineLength);
}

CommandFile::~CommandFile()
{
	require(not AreCommandFilesOpened());
	require(nParserState == TokenOther);
}

void CommandFile::Reset()
{
	require(not AreCommandFilesOpened());

	bWarningIfMissingJsonKey = false;
	SetInputCommandFileName("");
	SetInputParameterFileName("");
	SetOutputCommandFileName("");
	DeleteAllInputSearchReplaceValues();
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
	if (sOutputCommandFileName == "/dev/stdout" or sOutputCommandFileName == "/dev/stderr")
		bPrintOutputInConsole = true;
#endif
}

void CommandFile::SetBatchMode(boolean bValue)
{
	bBatchMode = bValue;
}

boolean CommandFile::GetBatchMode() const
{
	return bBatchMode;
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
	require(nParserLineIndex == 0);
	require(nParserState == TokenOther);

	// Copie depuis HDFS si necessaire
	bOk = PLRemoteFileService::BuildInputWorkingFile(sInputCommandFileName, sLocalInputCommandFileName);

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
		if (not bOk)
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
	static boolean bPendingFatalError = false;
	boolean bOk;
	boolean bIsParserOkBeforeEnd;
	boolean bIsParserOkAfterEnd;
	StringVector svIdentifierPath;
	ALString sValue;

	// Arret immediat en cas d'erreur en cours: cf. fin de la methode
	if (bPendingFatalError)
	{
		assert(fInputCommands == NULL);
		return;
	}

	// Lecture si necessaire de la fin du fichier pour avoir des diagnostics d'erreur complets
	// - bloc if non termine
	// - diagnostic sur les cles du parametrage json non utilisees dans les commandes
	bIsParserOkBeforeEnd = bParserOk;
	if (GetInputParameterFileName() != "" and IsInputCommandFileOpened() and not IsInputCommandEnd() and
	    nParserLineIndex > 0 and bParserOk)
	{
		// Analyse des ligne jusqu'a la fin du fichier ou d'une erreur, qui mettra a jour bParserOk
		bOk = true;
		while (bOk)
		{
			bOk = ReadInputCommand(&svIdentifierPath, sValue);
			svIdentifierPath.SetSize(0);
		}
	}
	bIsParserOkAfterEnd = bParserOk;

	// Detection des membres non utilises du parametrage json, s'il n'y a pas d'autres erreurs
	if (bIsParserOkAfterEnd and not Global::IsAtLeastOneError())
		bIsParserOkAfterEnd = DetectedUnusedJsonParameterMembers();

	// Fermeture du fichier d'entree
	if (fInputCommands != NULL)
	{
		fclose(fInputCommands);
		fInputCommands = NULL;

		// Si le fichier est sur HDFS, on supprime la copie locale
		PLRemoteFileService::CleanInputWorkingFile(sInputCommandFileName, sLocalInputCommandFileName);
	}

	// Nettoyage du parser et des parametre json (et donc  de bParserOk)
	ResetParser();
	jsonParameters.DeleteAll();

	// Erreur fatale standard si erreur avant la fin de fichier
	// On empeche une boucle infinie en cas d'erreur fatale, car la destruction des objets statiques
	// et la bPendingFatalError des fichiers est appellee apres la fin du main en cas d'erreur fatale
	assert(not bPendingFatalError);
	if (not bIsParserOkBeforeEnd)
	{
		AddInputCommandFileError("Analysis of input commands interrupted because of errors");

		// Erreur fatale uniquement en mode batch
		if (GetBatchMode())
		{
			bPendingFatalError = true;
			Global::AddFatalError("Command file", "", "Batch mode failure");
		}
	}
	// Erreur fatale en cas d'erreurs detectees apres la fermeture du fichier
	else if (not bIsParserOkAfterEnd)
	{
		// Erreur fatale uniquement en mode batch
		if (GetBatchMode())
		{
			bPendingFatalError = true;
			Global::AddFatalError("Command file", "",
					      "Batch mode failure detected when closing input command file");
		}
	}
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
	// On ferme d'abord le fichier en sortie
	CloseOutputCommandFile();

	// Puis le fichier en entree, ce qui peut declencher potentiellement une erreur fatale
	CloseInputCommandFile();
}

boolean CommandFile::ReadInputCommand(StringVector* svIdentifierPath, ALString& sValue)
{
	const char cDEL = (char)127;
	boolean bOk;
	char sCharBuffer[1 + BUFFER_LENGTH];
	ALString sInputLine;
	boolean bContinueParsing;
	IntVector ivTokenTypes;
	StringVector svTokenValues;
	ALString sMessage;
	int nLength;
	int nPosition;
	int nToken;
	ALString sEndLine;
	ALString sToken;
	ALString sInterToken;
	ALString sIdentifierPath;
	ALString sIdentifier;
	int i;
	ALString sTmp;

	require(svIdentifierPath != NULL);
	require(svIdentifierPath->GetSize() == 0);
	require(GetInputSearchReplaceValueNumber() == 0 or GetInputParameterFileName() == "");
	require(nParserState == TokenOther or nParserState == TokenIf or nParserState == TokenLoop);

	// On arrete si pas de fichier ou si fin de fichier
	if (fInputCommands == NULL or feof(fInputCommands))
		return false;

	////////////////////////////////////////////////////////////////////////////////////////////
	// Recherche d'une ligne non vide a traiter

	// On effectue le parsing uniquement si necessaire
	// Dans le cas d'un bloc de type loop en cours de traitement, ce n'est pas necessaire
	bOk = true;
	if (oaParserLoopLinesTokenTypes.GetSize() == 0)
	{
		// Boucle de lecture pour ignorer les lignes vides ou ne comportant que des commentaires
		while (sInputLine == "" and not feof(fInputCommands))
		{
			nParserLineIndex++;

			// Lecture
			StandardGetInputString(sCharBuffer, fInputCommands);
			sInputLine = sCharBuffer;

			// Si erreur ou caractere fin de fichier, on arrete sans message d'erreur
			// Si on est pas en fin de fichier, on a forcement un caractere '\n' en fin de ligne
			if (ferror(fInputCommands) or sInputLine.GetLength() == 0)
			{
				bOk = false;
				break;
			}

			// Erreur si ligne trop longue
			if (bOk and sInputLine.GetLength() > nMaxLineLength)
			{
				bOk = false;
				AddInputCommandFileError(sTmp + "line too long, with length " +
							 IntToString(sInputLine.GetLength()) + " > " +
							 IntToString(nMaxLineLength));
				break;
			}

			// Suppression des blancs au debut et a la fin, donc du dernier caractere fin de ligne
			sInputLine.TrimRight();
			sInputLine.TrimLeft();

			// Supression de la ligne entiere si elle est entierement commentee
			if (sInputLine.Find(sCommentPrefix) == 0)
				sInputLine = "";
			// Sinon, suppression du commentaire de fin de ligne precede d'un espace, ce qui permet d'avoir
			// des paires (IdentifierPath, valeur) avec valeur contenant des " //"
			else
			{
				nLength = sInputLine.GetLength();
				for (i = nLength - sCommentPrefix.GetLength() - 1; i >= 0; i--)
				{
					if (iswspace(sInputLine.GetAt(i)) and
					    sInputLine.GetAt(i + 1) == sCommentPrefix.GetAt(0) and
					    sInputLine.Right(nLength - i - 1).Find(sCommentPrefix) == 0)
					{
						sInputLine.GetBufferSetLength(i);
						sInputLine.TrimRight();
						break;
					}
				}
			}

			// Suppression si necessaire du dernier caractere s'il s'agit de cDEL
			// (methode FromJstring qui utilise ce cartactere special en fin de chaine pour
			// les conversions entre Java et C++ et l'ajoute dans les fichiers de commande en sortie)
			if (sInputLine.GetLength() > 0 and sInputLine.GetAt(sInputLine.GetLength() - 1) == cDEL)
				sInputLine.GetBufferSetLength(sInputLine.GetLength() - 1);

			// Tokenisation de la ligne en cours dans le cas d'un fichier de parametrage
			// On continue tant que necessaire, c'est a dire jusqu'a ce qu'une commande parsee soit
			// directement disponible, ou que l'on est prepare toutes les commandes d'un bloc loop
			assert(bOk);
			if (sInputLine != "" and GetInputParameterFileName() != "")
			{
				// Tokenisation de la ligne
				bOk = ParseInputCommand(sInputLine, bContinueParsing);

				// Traitement pour ignorer des lignes en fonction l'etat du parser
				// On force la poursuite du parsing si necessaire
				if (bContinueParsing)
					sInputLine = "";
			}
		}
		assert(sInputLine != "" or not bOk or feof(fInputCommands));

		// Cas particulier de fin de fichier sans fin de bloc
		// C'est le seul cas qui ne pas etre traite par le parsing en flux des lignes du fichier de commande
		if (bOk and feof(fInputCommands) and GetInputParameterFileName() != "" and nParserState != TokenOther)
		{
			bOk = false;
			AddInputCommandFileError("No " + sTokenEnd + " " + GetBlockType(nParserState) +
						 " found until the end of the file in current " +
						 GetBlockType(nParserState) + " " + sParserBlockKey + " block");
		}

		// Arret si necessaire
		if (not bOk or feof(fInputCommands))
			return false;
		assert(sInputLine != "");
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	// Pretraitement de la ligne

	// Cas sans fichier de parametrage json: on verifie que l'on n'utilise pas
	// des balises du langage de pilotage par fichier de parametre
	if (bOk and GetInputParameterFileName() == "")
	{
		// Detection du premier token de la ligne
		nToken = GetFirstInputToken(sInputLine, sToken, sInterToken, sEndLine);

		// Arret si detection de token reserves au cas des fichiers de parametres json
		// On ne peut pas aller plus loin dans l'analyse des tokens suivants, qui pourait
		// correspondre a des valeurs dans les donnees (par exemple, un nom de variable "__improbable__")
		if (nToken == TokenLoop or nToken == TokenIf or nToken == TokenEnd or nToken == TokenKey)
		{
			bOk = false;
			AddInputCommandFileError(sTmp + "use of the \"" + GetPrintableValue(sToken) +
						 "\" value allowed only with a json parameter file");
		}
	}

	// Cas avec fichier de parametrage json: analyse des lignes
	if (bOk and GetInputParameterFileName() != "")
		sInputLine = RecodeCurrentLineUsingJsonParameters(bOk);
	// Cas du mode recherche/remplacement dans la partie valeur de la commande
	else if (bOk and GetInputSearchReplaceValueNumber() > 0)
		sInputLine = ProcessSearchReplaceCommand(sInputLine);

	// Arret si necessaire
	if (not bOk)
		return false;

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

boolean CommandFile::IsInputCommandEnd() const
{
	require(GetInputCommandFileName() != "");
	require(IsInputCommandFileOpened());

	// On teste s'il reste des ligne a lire dans le fichier en entree et des commande de bloc loop en cours
	return feof(fInputCommands) and oaParserLoopLinesTokenTypes.GetSize() == 0;
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

		// Si redirection vers la console, on ajoute un prefixe
		if (bPrintOutputInConsole)
			fprintf(fOutputCommands, "Khiops.command\t");

		// Impression
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

boolean CommandFile::ReadWriteCommandFiles()
{
	boolean bOk = true;
	StringVector svIdentifierPath;
	ALString sValue;
	ALString sIdentifierPath;
	int i;

	require(Check());
	require(not AreCommandFilesOpened());
	require(GetInputCommandFileName() != "");
	require(GetOutputCommandFileName() != "");

	// Activation des warnings en cas de cle manquante dans le fichier json
	bWarningIfMissingJsonKey = true;

	// Ouverture des fichier de commandes
	if (bOk)
		bOk = OpenInputCommandFile();
	if (bOk)
		bOk = OpenOutputCommandFile();

	// Lecture/ecriture des lignes de commandes
	while (bOk and not IsInputCommandEnd())
	{
		bOk = ReadInputCommand(&svIdentifierPath, sValue);

		// Ecriture de la commande si ok
		if (bOk)
		{
			// Construction de l'identifier de commande a partir de ses composants
			sIdentifierPath = "";
			for (i = 0; i < svIdentifierPath.GetSize(); i++)
			{
				if (i > 0)
					sIdentifierPath += '.';
				sIdentifierPath += svIdentifierPath.GetAt(i);
			}
			svIdentifierPath.SetSize(0);

			// Ecriture
			WriteOutputCommand(sIdentifierPath, sValue, "");
		}
	}

	// Fermeture des fichiers de commandes, declenchant potentiellement des erreur
	CloseCommandFiles();

	// Desactivation des warnings
	bWarningIfMissingJsonKey = false;

	return bOk;
}

void CommandFile::AddInputCommandFileWarning(const ALString& sMessage) const
{
	ALString sLineLocalisation;
	ALString sTmp;

	// On precise le numero de ligne si disponible
	if (nParserLineIndex > 0)
		sLineLocalisation = sTmp + ", line " + IntToString(nParserLineIndex);
	Global::AddWarning("Input command file", sInputCommandFileName + sLineLocalisation, sMessage);
	bParserOk = false;
}

void CommandFile::AddInputCommandFileError(const ALString& sMessage) const
{
	ALString sLineLocalisation;
	ALString sTmp;

	// On precise le numero de ligne si disponible, pour un fichier ouvert et en cours de lecture
	if (nParserLineIndex > 0)
		sLineLocalisation = sTmp + ", line " + IntToString(nParserLineIndex);
	Global::AddError("Input command file", sInputCommandFileName + sLineLocalisation, sMessage);
	bParserOk = false;
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
	JSONMember* jsonMember;
	JSONArray* jsonArray;
	JSONValue* jsonValue;
	JSONObject* firstJsonSubObject;
	JSONObject* jsonSubObject;
	JSONMember* jsonSubObjectMember;
	JSONMember* firstJsonSubObjectMember;
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
			if (not CheckJsonKey(jsonMember->GetKey(), sMessage))
			{
				AddInputParameterFileError("in main json object, " + sMessage);
				bOk = false;
			}
			// Verification une seule fois de la non-collision de la cle avec sa variante de type byte
			else if (IsByteJsonKey(jsonMember->GetKey()) and
				 jsonParameters.LookupMember(ToStandardJsonKey(jsonMember->GetKey())) != NULL)
			{
				AddInputParameterFileError("in main json object, the \"" +
							   ToStandardJsonKey(jsonMember->GetKey()) +
							   "\" key is used twice, along with its \"" +
							   jsonMember->GetKey() + "\" byte variant");
				bOk = false;
			}
			// Verification du type, au premier niveau de la structure de l'objet json
			else if (jsonMember->GetValueType() != JSONObject::StringValue and
				 jsonMember->GetValueType() != JSONObject::NumberValue and
				 jsonMember->GetValueType() != JSONObject::BooleanValue and
				 jsonMember->GetValueType() != JSONObject::ArrayValue)
			{
				// Tolerance aux valeurs null, sans le mentionner dans le message d'erreur
				if (not(bAcceptMissingOrNullKeys and
					jsonMember->GetValueType() == JSONObject::NullValue))
				{
					AddInputParameterFileError(
					    "in main json object, the " + jsonMember->GetValue()->TypeToString() +
					    " type of value at " + BuildJsonPath(jsonMember, -1, NULL) +
					    " should be string, number, boolean or array");
					bOk = false;
				}
			}
			// Verification du type string dans le cas d'une cle avec sa variante de type byte
			else if (IsByteJsonKey(jsonMember->GetKey()) and
				 jsonMember->GetValueType() != JSONObject::StringValue)
			{
				AddInputParameterFileError(
				    "in main json object, the " + jsonMember->GetValue()->TypeToString() +
				    " type of value at " + BuildJsonPath(jsonMember, -1, NULL) +
				    " should be string, as the key prefix is \"" + sByteJsonKeyPrefix + "\"");
				bOk = false;
			}
			// Verification de de la longueur et de l'encodage base64 de la valeur string
			else if (jsonMember->GetValueType() == JSONObject::StringValue and
				 not CheckStringValue(jsonMember->GetStringValue()->GetString(),
						      IsByteJsonKey(jsonMember->GetKey()), sMessage))
			{
				AddInputParameterFileError("in main json object, at " +
							   BuildJsonPath(jsonMember, -1, NULL) + ", " + sMessage);
				bOk = false;
			}

			// Cas d'une valeur tableau
			if (jsonMember->GetValueType() == JSONObject::ArrayValue)
			{
				jsonArray = jsonMember->GetArrayValue();

				// Toutes les valeurs d'un tableau doivent etre de type objet
				bIsFirstArrayObjectValid = true;
				for (j = 0; j < jsonArray->GetValueNumber(); j++)
				{
					jsonValue = jsonArray->GetValueAt(j);
					bIsArrayObjectValid = true;

					// Verification du type, au premier niveau de la structure de l'objet json
					if (jsonValue->GetType() != JSONObject::ObjectValue)
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
							if (not CheckJsonKey(jsonSubObjectMember->GetKey(), sMessage))
							{
								AddInputParameterFileError(
								    "in object value of array at " +
								    BuildJsonPath(jsonMember, j, NULL) + ", " +
								    sMessage);
								bOk = false;
								bIsArrayObjectValid = false;
							}
							// Verification de la non-collision de la cle avec sa variante de type byte
							else if (IsByteJsonKey(jsonSubObjectMember->GetKey()) and
								 jsonSubObject->LookupMember(ToStandardJsonKey(
								     jsonSubObjectMember->GetKey())) != NULL)
							{
								AddInputParameterFileError(
								    "in object value of array at " +
								    BuildJsonPath(jsonMember, j, NULL) + ", the \"" +
								    ToStandardJsonKey(jsonSubObjectMember->GetKey()) +
								    "\" key is used twice,  along with its \"" +
								    jsonSubObjectMember->GetKey() + "\" byte variant");
								bOk = false;
							}
							// Verification que la cle du sous-objet n'entre pas en collision avec une cle de l'objet principal
							else if (jsonParameters.LookupMember(
								     jsonSubObjectMember->GetKey()) != NULL or
								 jsonParameters.LookupMember(ToVariantJsonKey(
								     jsonSubObjectMember->GetKey())) != NULL)
							{
								// Message d'erreur avec precision dans le cas d'utilisation de variantes standard ou byte differents
								sLabelSuffix = "";
								if (jsonParameters.LookupMember(ToVariantJsonKey(
									jsonSubObjectMember->GetKey())) != NULL)
									sLabelSuffix =
									    " (in the standard or byte variants)";
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
								     JSONObject::StringValue and
								 jsonSubObjectMember->GetValueType() !=
								     JSONObject::NumberValue)
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
							else if (IsByteJsonKey(jsonSubObjectMember->GetKey()) and
								 jsonSubObjectMember->GetValueType() !=
								     JSONObject::StringValue)
							{
								AddInputParameterFileError(
								    "in object value of array, the " +
								    jsonSubObjectMember->GetValue()->TypeToString() +
								    " type of value at " +
								    BuildJsonPath(jsonMember, j, jsonSubObjectMember) +
								    " should be string, as the key prefix is \"" +
								    sByteJsonKeyPrefix + "\"");
								bOk = false;
							}
							// Verification de de la longueur et de l'encodage base64 de la valeur string
							else if (jsonSubObjectMember->GetValueType() ==
								     JSONObject::StringValue and
								 not CheckStringValue(
								     jsonSubObjectMember->GetStringValue()->GetString(),
								     IsByteJsonKey(jsonSubObjectMember->GetKey()),
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
							    " member) should be the same as in the first object "
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
									ToVariantJsonKey(
									    firstJsonSubObjectMember->GetKey()))
								{
									AddInputParameterFileError(
									    "key \"" + jsonSubObjectMember->GetKey() +
									    "\" at " +
									    BuildJsonPath(jsonMember, j,
											  jsonSubObjectMember) +
									    " should be \"" +
									    firstJsonSubObjectMember->GetKey() +
									    "\", as in the first object in the "
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
									    ", as in the first object in the "
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

boolean CommandFile::CheckJsonKey(const ALString& sValue, ALString& sMessage) const
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
		bOk = sValue.GetLength() <= nMaxJsonKeyLength;
		if (not bOk)
			sMessage = sTmp + "key \"" + GetPrintableValue(sValue) + "\" too long, with length " +
				   IntToString(sValue.GetLength()) + " > " + IntToString(nMaxJsonKeyLength);
	}

	// Test du format camelCase
	if (bOk)
	{
		bOk = IsCamelCaseJsonKey(sValue);
		if (not bOk)
			sMessage = "key \"" + GetPrintableValue(sValue) + "\" should be camelCase";
	}

	ensure(not bOk or sMessage == "");
	return bOk;
}

boolean CommandFile::IsCamelCaseJsonKey(const ALString& sValue) const
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

boolean CommandFile::IsByteJsonKey(const ALString& sValue) const
{
	boolean bOk;
	char c;

	require(IsCamelCaseJsonKey(sValue));

	// Test si on est prefixe correctement, suivi d'un caractere alphabetique en majuscule
	bOk = sValue.GetLength() > sByteJsonKeyPrefix.GetLength();
	if (bOk)
		bOk = sValue.Left(sByteJsonKeyPrefix.GetLength()) == sByteJsonKeyPrefix;
	if (bOk)
	{
		c = sValue.GetAt(sByteJsonKeyPrefix.GetLength());
		bOk = isalpha(c) and isupper(c);
	}
	return bOk;
}

const ALString CommandFile::ToByteJsonKey(const ALString& sValue) const
{
	ALString sByteJsonKey;

	require(IsCamelCaseJsonKey(sValue));

	if (IsByteJsonKey(sValue))
		return sValue;
	else
	{
		sByteJsonKey = sValue;
		sByteJsonKey.SetAt(0, char(toupper(sByteJsonKey.GetAt(0))));
		sByteJsonKey = sByteJsonKeyPrefix + sByteJsonKey;
		return sByteJsonKey;
	}
}

const ALString CommandFile::ToStandardJsonKey(const ALString& sValue) const
{
	ALString sStandardJsonKey;

	require(IsCamelCaseJsonKey(sValue));

	if (IsByteJsonKey(sValue))
	{

		sStandardJsonKey = sValue.Right(sValue.GetLength() - sByteJsonKeyPrefix.GetLength());
		sStandardJsonKey.SetAt(0, char(tolower(sStandardJsonKey.GetAt(0))));
		return sStandardJsonKey;
	}
	else
		return sValue;
}

const ALString CommandFile::ToVariantJsonKey(const ALString& sValue) const
{
	require(IsCamelCaseJsonKey(sValue));

	if (IsByteJsonKey(sValue))
		return ToStandardJsonKey(sValue);
	else
		return ToByteJsonKey(sValue);
}

boolean CommandFile::CheckStringValue(const ALString& sValue, boolean bCheckBase64Encoding, ALString& sMessage) const
{
	boolean bOk = true;
	char* sBytes;
	int nPos;
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

	// Test si champ multi-ligne
	if (bOk)
	{
		nPos = sValue.Find('\n');
		bOk = nPos == -1;
		if (not bOk)
			sMessage = sTmp + "forbidden multi-line string value \"" +
				   GetPrintableValue(sValue.Left(nPos) + "...") + "\" ";
	}

	// Test de l'encodage base64
	if (bOk and bCheckBase64Encoding)
	{
		// Reherche d'un buffer de caracteres
		sBytes = StandardGetBuffer();
		assert(nMaxStringValueLength <= BUFFER_LENGTH);

		// Test de l'encodage base64
		bOk = TextService::Base64StringToBytes(sValue, sBytes);
		if (not bOk)
			sMessage = "incorrect string value \"" + GetPrintableValue(sValue) +
				   "\", which should be encoding using base64";
	}

	ensure(not bOk or sMessage == "");
	return bOk;
}

ALString CommandFile::BuildJsonPath(JSONMember* member, int nArrayRank, JSONMember* arrayObjectmember) const
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

const ALString& CommandFile::GetBlockType(int nToken) const
{
	require(nToken == TokenIf or nToken == TokenLoop);
	if (nToken == TokenIf)
		return sTokenIf;
	else
		return sTokenLoop;
}

void CommandFile::ResetParser()
{
	nParserLineIndex = 0;
	nParserState = TokenOther;
	nParserCurrentLineState = None;
	sParserBlockKey = "";
	ivParserTokenTypes.SetSize(0);
	svParserTokenValues.SetSize(0);
	bParserIgnoreBlockState = false;
	bParserIfState = false;
	parserLoopJSONArray = NULL;
	oaParserLoopLinesTokenTypes.DeleteAll();
	oaParserLoopLinesTokenValues.DeleteAll();
	nParserLoopLineIndex = -1;
	nParserLoopObjectIndex = -1;
	bParserOk = true;
	nkdParserUsedJsonParameterMembers.RemoveAll();
}

const ALString CommandFile::RecodeCurrentLineUsingJsonParameters(boolean& bOk)
{
	boolean bCleanLoopWorkingData;
	ALString sRecodedLine;
	JSONValue* jsonValue;
	ALString sJsonKey;
	boolean bIsByteJsonKey;
	IntVector* ivCurrentTokenTypes;
	StringVector* svCurrentTokenValues;
	JSONObject* currentJSONObject;
	char sCharBuffer[1 + BUFFER_LENGTH];
	int i;
	int nToken;

	require(GetInputParameterFileName() != "");
	require(IsInputCommandFileOpened());
	require(not bParserIgnoreBlockState);
	require(nParserState == TokenOther or nParserState == TokenIf);

	// Initialisation
	bOk = true;

	// Recherche du vecteur de token courant dans la boucle si elle est en cours de traitement
	bCleanLoopWorkingData = false;
	if (oaParserLoopLinesTokenTypes.GetSize() > 0)
	{
		// Dans un bloc loop, on est revenu en etat standard une fois la fin de la boucle detectee
		// et on doit alors traiter toutes les instruction memorisee autant de fois qu'il y d'objets json
		assert(nParserState == TokenOther);
		assert(parserLoopJSONArray != NULL);
		assert(nParserLoopLineIndex >= 0);
		assert(nParserLoopObjectIndex >= 0);
		assert(nParserLoopLineIndex < oaParserLoopLinesTokenTypes.GetSize() or
		       nParserLoopObjectIndex < parserLoopJSONArray->GetValueNumber());

		// Analyse de la ligne courante avec l'objet courant
		ivCurrentTokenTypes = cast(IntVector*, oaParserLoopLinesTokenTypes.GetAt(nParserLoopLineIndex));
		svCurrentTokenValues = cast(StringVector*, oaParserLoopLinesTokenValues.GetAt(nParserLoopLineIndex));
		currentJSONObject = parserLoopJSONArray->GetValueAt(nParserLoopObjectIndex)->GetObjectValue();

		// On passe a la suite
		nParserLoopLineIndex++;
		if (nParserLoopLineIndex == oaParserLoopLinesTokenTypes.GetSize())
		{
			nParserLoopLineIndex = 0;
			nParserLoopObjectIndex++;
		}

		// On memorise la necessite de nettoyage si necessaire des donnees de travail du bloc loop
		// Le nettoyage sera effectue plus tard, car il faut encore traiter la derniere instruction en cours
		if (nParserLoopObjectIndex == parserLoopJSONArray->GetValueNumber())
			bCleanLoopWorkingData = true;
	}
	// Sinon, on est sur une instruction standard a executer
	else
	{
		assert(nParserState == TokenOther or nParserState == TokenIf);
		assert(nParserState != TokenIf or bParserIfState);
		ivCurrentTokenTypes = &ivParserTokenTypes;
		svCurrentTokenValues = &svParserTokenValues;
		currentJSONObject = NULL;
	}

	// Recodage de la ligne en cours
	for (i = 0; i < ivCurrentTokenTypes->GetSize(); i++)
	{
		nToken = ivCurrentTokenTypes->GetAt(i);
		assert(nToken == TokenOther or nToken == TokenKey);
		assert(nToken == TokenOther or i > 0);

		// On a necessairement un blanc avant le deuxieme token
		if (i == 1)
			sRecodedLine += ' ';

		// Prise en compte du token tel quel dans le cas standard
		if (nToken == TokenOther)
			sRecodedLine += svCurrentTokenValues->GetAt(i);
		// Remplacement de la valeur dans le cas d'une cle json
		else
		{
			sJsonKey = ExtractJsonKey(svCurrentTokenValues->GetAt(i));
			bIsByteJsonKey = false;

			// Recherche d'un valeur d'abord dans l'objet courant
			jsonValue = NULL;
			if (currentJSONObject != NULL)
			{
				// Recherche d'abord en mode standard
				jsonValue = LookupJSONValue(currentJSONObject, sJsonKey);

				// Recherche sinon en mode byte
				if (jsonValue == NULL)
				{
					jsonValue = LookupJSONValue(currentJSONObject, ToByteJsonKey(sJsonKey));
					if (jsonValue != NULL)
						bIsByteJsonKey = true;
				}
				assert(jsonValue == NULL or
				       (LookupJSONValue(&jsonParameters, sJsonKey) == NULL and
					LookupJSONValue(&jsonParameters, ToByteJsonKey(sJsonKey)) == NULL));
			}

			// Recherche dans l'objet principal si non trouve
			if (jsonValue == NULL)
			{
				// Recherche d'abord en mode standard
				jsonValue = LookupJSONValue(&jsonParameters, sJsonKey);

				// Recherche sinon en mode byte
				if (jsonValue == NULL)
				{
					jsonValue = LookupJSONValue(&jsonParameters, ToByteJsonKey(sJsonKey));
					if (jsonValue != NULL)
						bIsByteJsonKey = true;
				}
			}

			// Erreur si non trouve
			if (jsonValue == NULL)
			{
				bOk = false;
				AddInputCommandFileError("Value not found for key \"" + sJsonKey +
							 "\" in json parameters");
			}
			// Erreur si type de valeur invalide
			else if (jsonValue->GetType() != JSONValue::NumberValue and
				 jsonValue->GetType() != JSONValue::StringValue and
				 jsonValue->GetType() != JSONValue::BooleanValue)
			{
				bOk = false;
				AddInputCommandFileError(
				    "Value found for key \"" + sJsonKey +
				    "\" in json parameters should be of type number or string, instead of " +
				    jsonValue->TypeToString());
			}
			// Sinon, on ajoute la valeur recodee
			else
			{
				assert(jsonValue->GetType() == JSONValue::NumberValue or
				       jsonValue->GetType() == JSONValue::StringValue or
				       jsonValue->GetType() == JSONValue::BooleanValue);

				// Cas numerique
				if (jsonValue->GetType() == JSONValue::NumberValue)
					sRecodedLine += jsonValue->GetNumberValue()->WriteString();
				// Cas booleen
				else if (jsonValue->GetType() == JSONValue::BooleanValue)
				{
					sRecodedLine += jsonValue->GetBooleanValue()->WriteString();
				}
				// Cas chaine de caracteres
				else
				{
					assert(jsonValue->GetType() == JSONValue::StringValue);

					// Cas avec recodage base64
					if (bIsByteJsonKey)
					{
						assert(jsonValue->GetStringValue()->GetString().GetLength() <=
						       nMaxStringValueLength);
						assert(nMaxStringValueLength < BUFFER_LENGTH);
						TextService::Base64StringToBytes(
						    jsonValue->GetStringValue()->GetString(), sCharBuffer);
						sRecodedLine += sCharBuffer;
					}
					// Cas standard
					else
						sRecodedLine += jsonValue->GetStringValue()->GetString();
				}
			}

			// Arret si erreur
			if (not bOk)
				break;
		}
	}

	// Nettoyage si necessaire des donnees de travail du bloc loop
	if (bCleanLoopWorkingData)
	{
		parserLoopJSONArray = NULL;
		oaParserLoopLinesTokenTypes.DeleteAll();
		oaParserLoopLinesTokenValues.DeleteAll();
		nParserLoopLineIndex = 0;
		nParserLoopObjectIndex = 0;
	}

	// Nettoyage si erreur
	if (not bOk)
		sRecodedLine = "";
	return sRecodedLine;
}

boolean CommandFile::ParseInputCommand(const ALString& sInputCommand, boolean& bContinueParsing)
{
	boolean bOk = true;
	int nNextToken;
	JSONValue* jsonValue;

	require(GetInputParameterFileName() != "");
	require(sInputCommand != "");
	require(IsValueTrimed(sInputCommand));
	require(sInputCommand.Find(sCommentPrefix) != 0);
	require(nParserState == TokenOther or nParserState == TokenIf or nParserState == TokenLoop);
	require(nParserState == TokenOther or sParserBlockKey != "");
	require(nParserState == TokenLoop or oaParserLoopLinesTokenTypes.GetSize() == 0);
	require(oaParserLoopLinesTokenValues.GetSize() == oaParserLoopLinesTokenTypes.GetSize());

	// Tokenisation de la ligne
	bContinueParsing = false;
	bOk = TokenizeInputCommand(sInputCommand, &ivParserTokenTypes, &svParserTokenValues);

	// Analyse des tokens
	if (bOk)
	{
		nParserCurrentLineState = ivParserTokenTypes.GetAt(0);
		nNextToken = None;
		if (ivParserTokenTypes.GetSize() > 1)
			nNextToken = ivParserTokenTypes.GetAt(1);

		/////////////////////////////////////////////////////////////////
		// Cas d'un debut de bloc IF ou LOOP
		if (nParserCurrentLineState == TokenIf or nParserCurrentLineState == TokenLoop)
		{
			assert(nNextToken == TokenKey);

			// Cas valide uniquement si l'etat du parser est instruction
			if (nParserState == TokenOther)
			{
				nParserState = nParserCurrentLineState;
				sParserBlockKey = svParserTokenValues.GetAt(1);
				bParserIfState = false;
				parserLoopJSONArray = NULL;

				// Recherche de la valeur json associee au bloc
				jsonValue = LookupJSONValue(&jsonParameters, ExtractJsonKey(sParserBlockKey));

				// Traitement d'une cle manquante ou de valeur null
				if (jsonValue == NULL or jsonValue->GetType() == JSONValue::NullValue)
				{
					// Si tolerance aux cle manquantes, on ignore le bloc
					if (bAcceptMissingOrNullKeys)
					{
						bParserIgnoreBlockState = true;
						bContinueParsing = true;

						// Warning si la valeur json est null
						if (bWarningIfMissingJsonKey and jsonValue == NULL)
							AddInputCommandFileWarning(
							    "For beginning of " + svParserTokenValues.GetAt(0) +
							    " block, key " + svParserTokenValues.GetAt(1) +
							    " not found in json parameter file");
					}
					// Erreur sinon
					else
					{
						bOk = false;
						AddInputCommandFileError("For beginning of " +
									 svParserTokenValues.GetAt(0) + " block, key " +
									 svParserTokenValues.GetAt(1) +
									 " not found in json parameter file");
					}
				}
				// Cas du if
				else if (nParserCurrentLineState == TokenIf)
				{
					// Erreur si type incorrect
					if (jsonValue->GetType() != JSONObject::BooleanValue)
					{
						bOk = false;
						AddInputCommandFileError("For beginning of " +
									 svParserTokenValues.GetAt(0) + " block, key " +
									 svParserTokenValues.GetAt(1) +
									 " found in json parameter file should be of "
									 "type boolean instead of " +
									 jsonValue->TypeToString());
					}
					// Sinon, on memorise la valeur du booleen
					else
					{
						bParserIfState = jsonValue->GetBooleanValue()->GetBoolean();

						// Comme on est sur une structure de controle, il faut continuer le parsing
						bContinueParsing = true;
					}
				}
				// Cas du loop
				else
				{
					assert(nParserCurrentLineState == TokenLoop);

					// Erreur si type incorrect
					if (jsonValue->GetType() != JSONObject::ArrayValue)
					{
						bOk = false;
						AddInputCommandFileError("For beginning of " +
									 svParserTokenValues.GetAt(0) + " block, key " +
									 svParserTokenValues.GetAt(1) +
									 " found in json parameter file should be of "
									 "type array instead of " +
									 jsonValue->TypeToString());
					}
					// Sinon, on memorise la valeur du tableau
					else
					{
						parserLoopJSONArray = jsonValue->GetArrayValue();

						// Reinitialisation du tableau des vecteurs de tokens du bloc loop
						oaParserLoopLinesTokenTypes.DeleteAll();
						oaParserLoopLinesTokenValues.DeleteAll();
						nParserLoopLineIndex = 0;
						nParserLoopObjectIndex = 0;

						// Comme on est sur une structure de controle, il faut continuer le parsing
						bContinueParsing = true;
					}
				}
			}
			// Commande invalide sinon
			else
			{
				bOk = false;
				AddInputCommandFileError("Unexpected beginning of a " + svParserTokenValues.GetAt(0) +
							 " block inside the current " + GetBlockType(nParserState) +
							 " " + sParserBlockKey + " block");
			}
		}
		/////////////////////////////////////////////////////////////////
		// Cas d'une fin de bloc END
		else if (nParserCurrentLineState == TokenEnd)
		{
			assert(nNextToken == TokenIf or nNextToken == TokenLoop);

			// On quite necessairement l'etat ou on ignore les lignes d'un bloc
			bParserIgnoreBlockState = false;

			// Cas valide uniquement si l'etat du parser est le bloc de bon type en cours
			if ((nParserState == TokenIf or nParserState == TokenLoop) and nParserState == nNextToken)
			{
				nParserState = TokenOther;
				sParserBlockKey = "";

				// Il faut continuer le parsing dans le cas d'un bloc if ou d'un bloc loop vide
				if (nParserState == TokenIf)
					bContinueParsing = true;
				else if (oaParserLoopLinesTokenTypes.GetSize() == 0)
					bContinueParsing = true;
				else if (parserLoopJSONArray->GetValueNumber() == 0)
				{
					bContinueParsing = true;
					oaParserLoopLinesTokenTypes.DeleteAll();
					oaParserLoopLinesTokenValues.DeleteAll();
				}
				else
					bContinueParsing = false;
			}
			// Commande invalide sinon
			else
			{
				bOk = false;
				if (nParserState == TokenOther)
					AddInputCommandFileError("Unexpected " + svParserTokenValues.GetAt(0) + " " +
								 svParserTokenValues.GetAt(1) +
								 " without beginning of the corresponding block");
				else
					AddInputCommandFileError("Unexpected " + svParserTokenValues.GetAt(0) + " " +
								 svParserTokenValues.GetAt(1) + " inside the current " +
								 GetBlockType(nParserState) + " " + sParserBlockKey +
								 " block");
			}
		}
		/////////////////////////////////////////////////////////////////
		// Cas standard valide d'une instruction
		else
		{
			// Memorisation dans le cas d'un bloc loop
			if (nParserState == TokenLoop)
			{
				// Erreur si trop de ligne dans la boucle
				if (oaParserLoopLinesTokenTypes.GetSize() > nLoopMaxLineNumber)
				{
					bOk = false;
					AddInputCommandFileError("Too many lines in current " + sTokenLoop + " " +
								 sParserBlockKey + " block, with more than " +
								 IntToString(nLoopMaxLineNumber) +
								 " non-empty command lines");
				}
				// Ajout des vecteurs de tokens sinon
				else
				{
					// On ignore le contenu de la boucle selon l'etat du parser et la specification
					if (not(bAcceptMissingOrNullKeys and bParserIgnoreBlockState))
					{
						oaParserLoopLinesTokenTypes.Add(ivParserTokenTypes.Clone());
						oaParserLoopLinesTokenValues.Add(svParserTokenValues.Clone());
					}
				}

				// Il faut continuer le parsing tant que le bloc loop n'est pas fini, sans erreur
				bContinueParsing = bOk;
			}
			// On ignore les ligne si necessaire dans le cas d'un bloc if
			else if (nParserState == TokenIf)
				bContinueParsing = not bParserIfState;
			// Sinon, la ligne est prete a l'emploi
			else
			{
				assert(nParserState == TokenOther);
				bContinueParsing = false;

				// Si la ligne contient des cles absentes, on l'ignore
				if (bAcceptMissingOrNullKeys)
				{
					if (ContainsMissingOrNullJSONValue(&jsonParameters, &ivParserTokenTypes,
									   &svParserTokenValues))
						bContinueParsing = true;
				}
			}

			// On doit continuer le parsing si on ignore un bloc
			if (bAcceptMissingOrNullKeys)
			{
				if (bParserIgnoreBlockState)
					bContinueParsing = true;
			}
		}
	}
	ensure(nParserState == TokenOther or nParserState == TokenIf or nParserState == TokenLoop);
	ensure(oaParserLoopLinesTokenValues.GetSize() == oaParserLoopLinesTokenTypes.GetSize());
	return bOk;
}

boolean CommandFile::TokenizeInputCommand(const ALString& sInputCommand, IntVector* ivTokenTypes,
					  StringVector* svTokenValues) const
{
	boolean bOk = true;
	int nToken;
	ALString sToken;
	ALString sBuffer;
	ALString sEndLine;
	ALString sInterToken;

	require(sInputCommand != "");
	require(IsValueTrimed(sInputCommand));
	require(sInputCommand.Find(sCommentPrefix) != 0);
	require(ivTokenTypes != NULL);
	require(svTokenValues != NULL);

	// Initialisations
	ivTokenTypes->SetSize(0);
	svTokenValues->SetSize(0);

	// Recherche du premier Token
	nToken = GetFirstInputToken(sInputCommand, sToken, sInterToken, sEndLine);
	ivTokenTypes->Add(nToken);
	svTokenValues->Add(sToken);

	// Cas IF <key> ou LOOP <key>
	if (nToken == TokenIf or nToken == TokenLoop)
	{
		// Recherche du token suivant attendu
		nToken = None;
		if (sEndLine != "")
		{
			sBuffer = sEndLine;
			nToken = GetFirstInputToken(sBuffer, sToken, sInterToken, sEndLine);
			ivTokenTypes->Add(nToken);
			svTokenValues->Add(sToken);
		}

		// Erreur pas de token suivant
		if (nToken == None)
		{
			bOk = false;
			AddInputCommandFileError("the " + svTokenValues->GetAt(0) +
						 " command must be followed by a __key__");
		}
		// Le token doit etre de type cle
		else if (nToken != TokenKey)
		{
			bOk = false;
			AddInputCommandFileError("the " + svTokenValues->GetAt(0) +
						 " command must be followed by a __key__ (instead of \"" +
						 GetPrintableValue(sToken) + "\")");
		}
		// La cle doit etre valide
		else
		{
			assert(nToken == TokenKey);
			bOk = CheckTokenKey(sToken);
		}

		// Il ne doit pas y en avoir d'autres tokens
		if (bOk and sEndLine != "")
		{
			bOk = false;
			AddInputCommandFileError("the " + svTokenValues->GetAt(0) +
						 " __key__ command is terminated (unexpected \"" +
						 GetPrintableValue(sEndLine) + "\" found afterwards)");
		}
	}
	// Cas END LOOP ou END IF
	else if (nToken == TokenEnd)
	{
		// Recherche du token suivant attendu
		nToken = None;
		if (sEndLine != "")
		{
			sBuffer = sEndLine;
			nToken = GetFirstInputToken(sBuffer, sToken, sInterToken, sEndLine);
			ivTokenTypes->Add(nToken);
			svTokenValues->Add(sToken);
		}

		// Erreur pas de token suivant
		if (nToken == None)
		{
			bOk = false;
			AddInputCommandFileError("the " + svTokenValues->GetAt(0) + " command must be followed by " +
						 sTokenIf + " or " + sTokenLoop);
		}
		// Le token doit LOOP ou IF
		else if (nToken != TokenIf and nToken != TokenLoop)
		{
			bOk = false;
			AddInputCommandFileError("the " + sTokenEnd + " command must be followed by " + sTokenIf +
						 " or " + sTokenLoop + " (instead of \"" + GetPrintableValue(sToken) +
						 "\")");
		}

		// Il ne doit pas y en avoir d'autres tokens
		if (bOk and sEndLine != "")
		{
			bOk = false;
			AddInputCommandFileError("the " + svTokenValues->GetAt(0) + " " + sToken +
						 " command is terminated (unexpected \"" + GetPrintableValue(sEndLine) +
						 "\" found afterwards)");
		}
	}
	// Cas  <command> (<key>|<other>)*
	else if (nToken == TokenOther)
	{
		// Analyse de la fin de la ligne pour en extraire les variables
		while (sEndLine != "")
		{
			// Tokenisation de la fin de ligne
			sBuffer = sEndLine;
			nToken = GetFirstInputToken(sBuffer, sToken, sInterToken, sEndLine);

			// Analyse du token dans le cas d'une cle
			if (nToken == TokenKey)
			{
				// Erreur si token cle en deuxieme position sans separateur
				if (ivTokenTypes->GetSize() == 1)
				{
					if (svTokenValues->GetAt(0).Find(' ') == -1 and
					    sInputCommand.Find(sToken) == svTokenValues->GetAt(0).GetLength())
					{
						bOk = false;
						AddInputCommandFileError("incorrect use of key \"" +
									 GetPrintableValue(sToken) +
									 "\" after the command \"" +
									 GetPrintableValue(svTokenValues->GetAt(0)) +
									 "\" without a preceding space character");
					}
				}

				// Verification du token
				if (bOk)
					bOk = CheckTokenKey(sToken);

				// Prise en compte si valide
				if (bOk)
				{
					ivTokenTypes->Add(nToken);
					svTokenValues->Add(sToken);
				}
				// Arret sinon
				else
					break;

				// A jout d'un token de type valeur si necessaire
				if (sInterToken != "")
				{
					assert(sEndLine.GetLength() > 0);
					ivTokenTypes->Add(TokenOther);
					svTokenValues->Add(sInterToken);
				}
			}
			// Sinon, ajout si necessaire du token en tant que TokenOther
			else
			{
				// Ajout de la valeur inter-token
				sToken += sInterToken;

				// Ajout du token si seul le token de commande est enregistre
				if (ivTokenTypes->GetSize() == 1)
				{
					ivTokenTypes->Add(TokenOther);
					svTokenValues->Add(sToken);
				}
				// Sinon, ajout du token si le precedent est de type cle
				else if (ivTokenTypes->GetAt(ivTokenTypes->GetSize() - 1) == TokenKey)
				{
					ivTokenTypes->Add(TokenOther);
					svTokenValues->Add(sToken);
				}
				// Completion du token existant sinon
				else
				{
					assert(ivTokenTypes->GetSize() >= 2);
					assert(ivTokenTypes->GetAt(ivTokenTypes->GetSize() - 1) == TokenOther);
					sToken = svTokenValues->GetAt(ivTokenTypes->GetSize() - 1) + sToken;
					svTokenValues->SetAt(ivTokenTypes->GetSize() - 1, sToken);
				}
			}
		}
	}
	// Cas KEY ...
	else
	{
		assert(nToken == TokenKey);

		// Erreur
		bOk = false;
		AddInputCommandFileError("incorrect use of key \"" + GetPrintableValue(sToken) +
					 "\" at the beginning of a command");
	}

	// Nettoyage si erreur
	if (not bOk)
	{
		ivTokenTypes->SetSize(0);
		svTokenValues->SetSize(0);
	}
	ensure(ivTokenTypes->GetSize() == svTokenValues->GetSize());
	ensure(not bOk or ivTokenTypes->GetSize() > 0);
	return bOk;
}

int CommandFile::GetFirstInputToken(const ALString& sInputCommand, ALString& sToken, ALString& sInterToken,
				    ALString& sEndLine) const
{
	int nOutputToken;
	ALString sBuffer;
	int nPosDelimiter;
	int nPosSpace;
	int nPosNextToken;

	require(sInputCommand != "");
	require(IsValueTrimed(sInputCommand));

	// Cas d'un token de type cle json
	nPosDelimiter = sInputCommand.Find(sJsonKeyDelimiter);
	if (nPosDelimiter == 0)
	{
		nOutputToken = TokenKey;

		// Recherche de la fin de la cle json
		sBuffer = sInputCommand.Right(sInputCommand.GetLength() - sJsonKeyDelimiter.GetLength());
		nPosDelimiter = sBuffer.Find(sJsonKeyDelimiter);
		if (nPosDelimiter == -1)
		{
			sToken = sInputCommand;
			sEndLine = "";
		}
		else
		{
			sToken = sInputCommand.Left(nPosDelimiter + 2 * sJsonKeyDelimiter.GetLength());
			sEndLine = sInputCommand.Right(sInputCommand.GetLength() - sToken.GetLength());
			sEndLine.TrimLeft();
		}
	}
	// Analyse de la ligne sinon
	else
	{
		assert(sInputCommand.Find(sJsonKeyDelimiter) != 0);

		// Recherche du premier token
		nPosSpace = sInputCommand.Find(" ");
		if (nPosSpace >= 0 and nPosDelimiter >= 0)
			nPosNextToken = min(nPosSpace, nPosDelimiter);
		else if (nPosSpace == -1)
			nPosNextToken = nPosDelimiter;
		else
			nPosNextToken = nPosSpace;

		// Extraction du premier token
		if (nPosNextToken == -1)
		{
			sToken = sInputCommand;
			sEndLine = "";
		}
		else
		{
			assert(nPosNextToken > 0);
			sToken = sInputCommand.Left(nPosNextToken);
			sEndLine = sInputCommand.Right(sInputCommand.GetLength() - nPosNextToken);
			sEndLine.TrimLeft();
		}

		// Identification du type de token
		if (sToken == sTokenLoop)
			nOutputToken = TokenLoop;
		else if (sToken == sTokenIf)
			nOutputToken = TokenIf;
		else if (sToken == sTokenEnd)
			nOutputToken = TokenEnd;
		else
			nOutputToken = TokenOther;
	}

	// Calcul de la valeur inter-token
	// C'est necessaire pour ne pas pas perdre les caractere blancs dans les valeurs
	sInterToken = sInputCommand.Mid(sToken.GetLength(),
					sInputCommand.GetLength() - sToken.GetLength() - sEndLine.GetLength());

	ensure(0 <= nOutputToken and nOutputToken < None);
	ensure(IsValueTrimed(sToken));
	ensure(IsValueTrimed(sEndLine));
	ensure(sInputCommand.Find(sToken) == 0);
	ensure(sToken.GetLength() + sInterToken.GetLength() + sEndLine.GetLength() == sInputCommand.GetLength());
	return nOutputToken;
}

void CommandFile::WriteInputCommandTokens(ostream& ost, const IntVector* ivTokenTypes,
					  const StringVector* svTokenValues) const
{
	int i;
	int nToken;

	require(ivTokenTypes != NULL);
	require(svTokenValues != NULL);
	ensure(ivTokenTypes->GetSize() == svTokenValues->GetSize());

	// Affichage avec mise en forme de la liste des tokens
	for (i = 0; i < ivTokenTypes->GetSize(); i++)
	{
		nToken = ivTokenTypes->GetAt(i);
		assert(0 <= nToken and nToken < None);

		// On a necessairement un blanc avant le deuxieme token
		if (i == 1)
			ost << ' ';

		// Affichage du token
		ost << svTokenValues->GetAt(i);
	}
	ost << '\n';
}

boolean CommandFile::CheckTokenKey(const ALString& sToken) const
{
	boolean bOk;
	ALString sJsonKey;
	int nPos;
	ALString sMessage;

	require(sToken != "");
	require(IsValueTrimed(sToken));
	require(sToken.Find(sJsonKeyDelimiter) == 0);

	// Initialisation du resultat
	bOk = true;

	// Controle des delimiteurs, et suppression de ceux-ci
	sJsonKey = sToken.Right(sToken.GetLength() - sJsonKeyDelimiter.GetLength());
	if (bOk)
	{
		nPos = sJsonKey.Find(sJsonKeyDelimiter);
		if (nPos == -1)
		{
			bOk = false;
			AddInputCommandFileError("missing trailing key delimiter \"" + sJsonKeyDelimiter +
						 "\" at the end of \"" + GetPrintableValue(sToken) + "\"");
		}

		// Le token a tester doit etre extrait en s'arretant si possible au second delimiteur trouve
		if (bOk)
		{
			sJsonKey = sJsonKey.Left(sJsonKey.GetLength() - sJsonKeyDelimiter.GetLength());

			// On ne doit appeler la methode qu'avec un token candidat, ne au plus deux delimiteurs
			assert(sJsonKey.Find(sJsonKeyDelimiter) == -1);
		}
		assert(not bOk or sJsonKey.GetLength() == sToken.GetLength() - 2 * sJsonKeyDelimiter.GetLength());
	}

	// Verification de syntaxe de la variable
	if (bOk)
	{
		bOk = CheckJsonKey(sJsonKey, sMessage);
		if (not bOk)
			AddInputCommandFileError("incorrect __key__, " + sMessage);
	}

	// Verification que l'on n'utilise par une variante de type byte dans un fichier de commande en entree
	if (bOk and sJsonKey.Find(sByteJsonKeyPrefix) == 0)
	{
		bOk = false;
		AddInputCommandFileError("prefix \"" + sByteJsonKeyPrefix + "\" used in \"" +
					 GetPrintableValue(sToken) + "\" not allowed in input command file");
	}
	return bOk;
}

const ALString CommandFile::ExtractJsonKey(const ALString& sTokenKey) const
{
	require(CheckTokenKey(sTokenKey));
	return sTokenKey.Mid(sJsonKeyDelimiter.GetLength(), sTokenKey.GetLength() - 2 * sJsonKeyDelimiter.GetLength());
}

JSONValue* CommandFile::LookupJSONValue(JSONObject* jsonObject, const ALString& sKey) const
{
	JSONValue* jsonValue;
	JSONMember* jsonMember;

	debug(ALString sMessage);

	require(jsonObject != NULL);
	debug(require(CheckJsonKey(sKey, sMessage)));

	// Recherche du membre
	jsonMember = jsonObject->LookupMember(sKey);

	// Mise a jour de l'indicateur d'utilisation du membre
	if (jsonMember != NULL)
		nkdParserUsedJsonParameterMembers.SetAt(jsonMember, jsonMember);

	// Recherche de la valeur
	jsonValue = NULL;
	if (jsonMember != NULL)
		jsonValue = jsonMember->GetValue();
	return jsonValue;
}

boolean CommandFile::ContainsMissingOrNullJSONValue(JSONObject* jsonObject, const IntVector* ivTokenTypes,
						    const StringVector* svTokenValues) const
{
	boolean bIsMissing;
	boolean bIsNullValue;
	int i;
	int nToken;
	ALString sJsonKey;
	JSONValue* jsonValue;

	require(bAcceptMissingOrNullKeys);
	require(jsonObject != NULL);
	require(ivTokenTypes != NULL);
	require(svTokenValues != NULL);
	ensure(ivTokenTypes->GetSize() == svTokenValues->GetSize());

	// Affichage avec mise en forme de la liste des tokens
	bIsMissing = false;
	bIsNullValue = false;
	for (i = 0; i < ivTokenTypes->GetSize(); i++)
	{
		nToken = ivTokenTypes->GetAt(i);
		assert(0 <= nToken and nToken < None);

		// Traitement des tokens de type cle
		// Attention: une valeur peut etre disponible soit sous sa forme standard, soit sous sa forme byte
		// Elle est consideree comme manquante si elle n'est disponible sous aucune des deux formes
		if (nToken == TokenKey)
		{
			// Recherche d'une valeur sous sa forme standard
			sJsonKey = ExtractJsonKey(svTokenValues->GetAt(i));
			jsonValue = LookupJSONValue(jsonObject, sJsonKey);

			// Recherche d'une valeur sous sa forme byte si non trouve
			if (jsonValue == NULL)
				jsonValue = LookupJSONValue(jsonObject, ToByteJsonKey(sJsonKey));

			// Memorisation du resultat
			bIsMissing = (jsonValue == NULL);
			bIsNullValue = false;
			if (not bIsMissing)
				bIsNullValue = (jsonValue->GetType() == JSONValue::NullValue);

			// Arret si valeur absente du json ou de valeur null
			if (bIsMissing or bIsNullValue)
			{
				// Warning sur l'absence de cle
				if (bWarningIfMissingJsonKey and bIsMissing)
					AddInputCommandFileWarning("Key " + svTokenValues->GetAt(1) +
								   " not found in json parameter file");
				break;
			}
		}
	}
	return bIsMissing or bIsNullValue;
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

boolean CommandFile::DetectedUnusedJsonParameterMembers() const
{
	boolean bOk = true;
	int nMember;
	int nObject;
	int nSubMember;
	JSONMember* jsonMember;
	JSONArray* jsonArray;
	JSONObject* jsonObject;
	JSONMember* jsonSubMember;
	JSONMember* usedMember;

	// Parcours des membres de l'objet principal de parametrage json
	Global::ActivateErrorFlowControl();
	for (nMember = 0; nMember < jsonParameters.GetMemberNumber(); nMember++)
	{
		jsonMember = jsonParameters.GetMemberAt(nMember);

		// Recherche si le membre est utilise
		usedMember = cast(JSONMember*, nkdParserUsedJsonParameterMembers.Lookup(jsonMember));
		assert(usedMember == NULL or usedMember == jsonMember);

		// Erreur si le membre n'est pas utlise
		if (usedMember == NULL)
		{
			bOk = false;
			AddInputParameterFileError("value at " + BuildJsonPath(jsonMember, -1, NULL) +
						   " not used in input command file");
		}

		// Cas d'un tableau
		if (jsonMember->GetValueType() == JSONValue::ArrayValue)
		{
			jsonArray = jsonMember->GetArrayValue();

			// Parcours des objets du tableau
			for (nObject = 0; nObject < jsonArray->GetValueNumber(); nObject++)
			{
				jsonObject = jsonArray->GetValueAt(nObject)->GetObjectValue();

				// Parcours des membres du sous-objets
				for (nSubMember = 0; nSubMember < jsonObject->GetMemberNumber(); nSubMember++)
				{
					jsonSubMember = jsonObject->GetMemberAt(nSubMember);

					// Recherche si le membre est utilise
					usedMember =
					    cast(JSONMember*, nkdParserUsedJsonParameterMembers.Lookup(jsonSubMember));
					assert(usedMember == NULL or usedMember == jsonSubMember);

					// Erreur si le membre n'est pas utlise
					if (usedMember == NULL)
					{
						bOk = false;
						AddInputParameterFileError(
						    "value at " + BuildJsonPath(jsonMember, nObject, jsonSubMember) +
						    " not used in input command file");
					}
				}
			}
		}
	}
	Global::DesactivateErrorFlowControl();
	return bOk;
}

const ALString CommandFile::sCommentPrefix = "//";
const ALString CommandFile::sTokenLoop = "LOOP";
const ALString CommandFile::sTokenIf = "IF";
const ALString CommandFile::sTokenEnd = "END";
const ALString CommandFile::sJsonKeyDelimiter = "__";
const ALString CommandFile::sByteJsonKeyPrefix = "byte";
