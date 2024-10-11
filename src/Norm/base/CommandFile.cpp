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
	require(not IsCommandFileOpened());

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
			sMessage = sTmp + "overlengthy key \"" + GetPrintableValue(sValue) + "\", with length " +
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
			sMessage = sTmp + "overlengthy string value \"" + GetPrintableValue(sValue) +
				   "\", with length " + IntToString(sValue.GetLength()) + " > " +
				   IntToString(nMaxStringValueLength);
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

const ALString CommandFile::GetPrintableValue(const ALString& sValue) const
{
	if (sValue.GetLength() <= nMaxPrintableLength)
		return sValue;
	else
		return sValue.Left(nMaxPrintableLength) + "...";
}

void CommandFile::AddInputCommandFileError(const ALString& sMessage) const
{
	Global::AddError("Input command file", sInputCommandFileName, sMessage);
}

void CommandFile::AddInputParameterFileError(const ALString& sMessage) const
{
	Global::AddError("Input parameter file", sInputParameterFileName, sMessage);
}

void CommandFile::AddOutputCommandFileError(const ALString& sMessage) const
{
	Global::AddError("Input command file", sOutputCommandFileName, sMessage);
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

const ALString CommandFile::sByteVariablePrefix = "byte";
