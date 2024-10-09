// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CommandFile.h"

CommandFile::CommandFile()
{
	fInputCommands = NULL;
	fOutputCommands = NULL;
	bPrintOutputInConsole = false;
}

CommandFile::~CommandFile()
{
	require(not IsCommandFileOpened());
}

void CommandFile::Reset()
{
	require(not IsCommandFileOpened());

	SetInputCommandFileName("");
	SetOutputCommandFileName("");
	DeleteAllInputSearchReplaceValues();
	SetInputParameterFileName("");
}

void CommandFile::SetInputCommandFileName(const ALString& sFileName)
{
	require(not IsCommandFileOpened());

	sInputCommandFileName = sFileName;
}

const ALString& CommandFile::GetInputCommandFileName() const
{
	return sInputCommandFileName;
}

void CommandFile::SetOutputCommandFileName(const ALString& sFileName)
{
	require(not IsCommandFileOpened());

	sOutputCommandFileName = sFileName;

	// Si les fichier de sortie est /dev/stdout ou /dev/stderr, c'est un eredirection vers la console
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

	require(not IsCommandFileOpened());

	// Ajout des valeurs
	svInputCommandSearchValues.Add(sSearchValue);
	svInputCommandReplaceValues.Add(sReplaceValue);
}

int CommandFile::GetInputSearchReplaceValueNumber()
{
	assert(svInputCommandSearchValues.GetSize() == svInputCommandReplaceValues.GetSize());
	return svInputCommandSearchValues.GetSize();
}

const ALString& CommandFile::GetInputSearchValueAt(int nIndex)
{
	require(0 <= nIndex and nIndex < GetInputSearchReplaceValueNumber());
	return svInputCommandSearchValues.GetAt(nIndex);
}

const ALString& CommandFile::GetInputReplaceValueAt(int nIndex)
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
	require(not IsCommandFileOpened());

	sInputParameterFileName = sFileName;
}

const ALString& CommandFile::GetInputParameterFileName() const
{
	return sInputParameterFileName;
}

boolean CommandFile::Check() const
{
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
		Global::AddError("Input command file", sInputCommandFileName, "Unable to open file");
		bOk = false;
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
		Global::AddError("Output command file", sOutputCommandFileName, "Unable to open file");
		bOk = false;
	}
	return bOk;
}

boolean CommandFile::IsOutputCommandFileOpened() const
{
	return fOutputCommands != NULL;
}

boolean CommandFile::IsCommandFileOpened() const
{
	return IsInputCommandFileOpened() or IsOutputCommandFileOpened();
}

void CommandFile::CloseCommandFiles()
{
	// Fermeture du fichier d'entree
	if (fInputCommands != NULL)
	{
		fclose(fInputCommands);
		fInputCommands = NULL;

		// Si le fichier est sur HDFS, on supprime la copie locale
		PLRemoteFileService::CleanInputWorkingFile(sInputCommandFileName, sLocalInputCommandFileName);
	}

	// Fermeture du fichier de sortie, uniquement en mode batch (sinon, on enregistre toujours le scenario)
	if (fOutputCommands != NULL)
	{
		fclose(fOutputCommands);
		fOutputCommands = NULL;

		// Copie vers HDFS si necessaire
		PLRemoteFileService::CleanOutputWorkingFile(sOutputCommandFileName, sLocalOutputCommandFileName);
	}
}

boolean CommandFile::ReadInputCommand(StringVector* svIdentifierPath, ALString& sValue)
{
	const char cDEL = (char)127;
	int i;
	char sCharBuffer[1 + BUFFER_LENGTH];
	ALString sStringBuffer;
	int nLength;
	ALString sIdentifierPath;
	int nPosition;
	ALString sToken;
	ALString sIdentifier;

	require(svIdentifierPath != NULL);
	require(svIdentifierPath->GetSize() == 0);

	// On arrete si pas de fichier ou si fin de fichier
	if (fInputCommands == NULL or feof(fInputCommands))
		return false;

	// Boucle de lecture pour ignorer les lignes vides
	// ou ne comportant que des commentaires
	while (sStringBuffer == "" and not feof(fInputCommands))
	{
		// Lecture
		StandardGetInputString(sCharBuffer, fInputCommands);

		// Si erreur ou caractere fin de fichier, on ne renvoie rien
		if (ferror(fInputCommands) or sCharBuffer[0] == '\0')
		{
			// Nettoyage
			fclose(fInputCommands);
			fInputCommands = NULL;
			return false;
		}

		// Suppression du premier '\n'
		for (i = 0; i < BUFFER_LENGTH; i++)
		{
			if (sCharBuffer[i] == '\n')
				sCharBuffer[i] = '\0';
		}

		// Recherche /remplacement dans la partie valeur de la commande
		sStringBuffer = ProcessSearchReplaceCommand(sCharBuffer);

		// Suppression des commentaires en partant de la fin
		// Ainsi on ne supprime que le dernier commentaire, ce qui permet d'avoir
		// des paires (IdentifierPath, valeur) avec valeur contenant des " //"
		// si on a un commentaire est en fin de ligne
		nLength = sStringBuffer.GetLength();
		for (i = nLength - 1; i >= 2; i--)
		{
			if (sStringBuffer.GetAt(i) == '/' and sStringBuffer.GetAt(i - 1) == '/' and
			    iswspace(sStringBuffer.GetAt(i - 2)))
			{
				sStringBuffer = sStringBuffer.Left(i - 1);
				break;
			}
		}

		// Suppression des blancs au debut et a la fin
		sStringBuffer.TrimRight();
		sStringBuffer.TrimLeft();

		// Suppression si necessaire du dernier caractere s'il s'agit de cDEL
		// (methode FromJstring qui utilise ce cartactere special en fin de chaine pour
		// les conversions entre Java et C++)
		if (sStringBuffer.GetLength() > 0 and sStringBuffer.GetAt(sStringBuffer.GetLength() - 1) == cDEL)
			sStringBuffer.GetBufferSetLength(sStringBuffer.GetLength() - 1);

		// On supprime la ligne si elle commence par un commentaire
		// Cela permet de commenter y compris les lignes ayant une valeur contenant des "//"
		if (sStringBuffer.GetLength() >= 2 and sStringBuffer.GetAt(0) == '/' and sStringBuffer.GetAt(1) == '/')
			sStringBuffer = "";
	}

	// Recherche de l'IdentifierPath et de la valeur
	nPosition = sStringBuffer.Find(' ');
	if (nPosition == -1)
	{
		sIdentifierPath = sStringBuffer;
		sValue = "";
	}
	else
	{
		sIdentifierPath = sStringBuffer.Left(nPosition);
		sValue = sStringBuffer.Right(sStringBuffer.GetLength() - nPosition - 1);
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
			fprintf(fOutputCommands, "// %s\n", (const char*)sLabel);
		else
			fprintf(fOutputCommands, "%-*.*s // %s\n", nCommandLength, nCommandLength,
				(const char*)sCommand, (const char*)sLabel);
		fflush(fOutputCommands);
	}
}

const ALString CommandFile::ProcessSearchReplaceCommand(const ALString& sInputCommand)
{
	ALString sInputString;
	ALString sBeginString;
	ALString sEndString;
	ALString sOutputCommand;
	ALString sSearchValue;
	ALString sReplaceValue;
	int nSearchPosition;
	int i;

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
