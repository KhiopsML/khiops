// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCCoclusteringReport.h"

const ALString CCCoclusteringReport::sKeyWordInstances = "Instances";
const ALString CCCoclusteringReport::sKeyWordCells = "Cells";
const ALString CCCoclusteringReport::sKeyWordNullCost = "Null cost";
const ALString CCCoclusteringReport::sKeyWordCost = "Cost";
const ALString CCCoclusteringReport::sKeyWordLevel = "Level";
const ALString CCCoclusteringReport::sKeyWordInitialDimensions = "Initial dimensions";
const ALString CCCoclusteringReport::sKeyWordFrequencyAttribute = "Frequency variable";
const ALString CCCoclusteringReport::sKeyWordDictionary = "Dictionary";
const ALString CCCoclusteringReport::sKeyWordDatabase = "Database";
const ALString CCCoclusteringReport::sKeyWordSamplePercentage = "Sample percentage";
const ALString CCCoclusteringReport::sKeyWordSamplingMode = "Sampling mode";
const ALString CCCoclusteringReport::sKeyWordSelectionVariable = "Selection variable";
const ALString CCCoclusteringReport::sKeyWordSelectionValue = "Selection value";

CCCoclusteringReport::CCCoclusteringReport()
{
	fReport = NULL;
	nLineIndex = 0;
	bEndOfLine = false;
	sFileBuffer = NULL;
	bReadDebug = false;
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;
}

CCCoclusteringReport::~CCCoclusteringReport()
{
	assert(fReport == NULL);
	assert(sFileBuffer == NULL);
	assert(nLineIndex == 0);
}

boolean CCCoclusteringReport::ReadReport(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk;
	int nFileFormat;
	ALString sKhiopsEncoding;
	boolean bForceUnicodeToAnsi;

	require(sFileName != "");
	require(coclusteringDataGrid != NULL);
	require(not JSONTokenizer::IsOpened());

	// On determine le format du fichier
	nFileFormat = DetectFileFormatAndEncoding(sFileName, sKhiopsEncoding);
	bOk = nFileFormat == JSON;

	// On arrete l'analyse si le format est incorrect
	if (not bOk)
		return bOk;

	// Analyse du type d'encodage pour determiner si on doit recoder les caracteres utf8 du fichier json en
	// ansi
	if (sKhiopsEncoding == "")
	{
		bForceUnicodeToAnsi = false;
		AddWarning("The \"khiops_encoding\" field is missing in the read coclustering file. "
			   "The coclustering file is deprecated, and may raise encoding problems "
			   "in case of mixed ansi and utf8 chars "
			   ": see the Khiops guide for more information.");
	}
	else if (sKhiopsEncoding == "ascii" or sKhiopsEncoding == "utf8")
		bForceUnicodeToAnsi = false;
	else if (sKhiopsEncoding == "ansi" or sKhiopsEncoding == "mixed_ansi_utf8")
		bForceUnicodeToAnsi = true;
	else if (sKhiopsEncoding == "colliding_ansi_utf8")
	{
		bForceUnicodeToAnsi = true;
		AddWarning("The \"khiops_encoding\" field is \"" + sKhiopsEncoding +
			   "\" in the read coclustering file. "
			   "This may raise encoding problems if the file has been modified outside of Khiops "
			   ": see the Khiops guide for more information.");
	}
	else
	{
		bForceUnicodeToAnsi = false;
		AddWarning("The value of the \"khiops_encoding\" field is \"" + sKhiopsEncoding +
			   "\" in the read coclustering file. "
			   "This encoding type is unknown and will be ignored, which may raise encoding problems "
			   "in case of mixed ansi and utf8 chars "
			   ": see the Khiops guide for more information.");
	}

	// Lecture du fichier en parametrant le json tokeniser correctement
	JSONTokenizer::SetForceUnicodeToAnsi(bForceUnicodeToAnsi);

	// Initialisation du tokenizer pour analiser le rapport
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;
	sReportFileName = sFileName;
	bOk = JSONTokenizer::OpenForRead(GetClassLabel(), sFileName);
	if (bOk)
	{
		// Parsing
		bOk = InternalReadReport(coclusteringDataGrid, false);

		// Fermeture du tokenizer
		JSONTokenizer::Close();
	}
	sReportFileName = "";
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;

	JSONTokenizer::SetForceUnicodeToAnsi(false);

	return bOk;
}

boolean CCCoclusteringReport::ReadReportHeader(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid,
					       int& nInstanceNumber, int& nCellNumber)
{
	boolean bOk;
	int nFileFormat;
	ALString sKhiopsEncoding;
	boolean bForceUnicodeToAnsi;

	require(sFileName != "");
	require(coclusteringDataGrid != NULL);
	require(not JSONTokenizer::IsOpened());

	// On determine le format du fichier
	nFileFormat = DetectFileFormatAndEncoding(sFileName, sKhiopsEncoding);
	bOk = nFileFormat == JSON;

	// On arrete l'analyse si le format est incorrect
	if (not bOk)
		return bOk;

	// Lecture selon le format JSON
	nInstanceNumber = 0;
	nCellNumber = 0;

	// Analyse du type d'encodage pour determiner si on doit recoder les caracteres utf8 du fichier json en
	// ansi Pas de message pour cette lecture rapide
	bForceUnicodeToAnsi = (sKhiopsEncoding == "ansi" or sKhiopsEncoding == "mixed_ansi_utf8" or
			       sKhiopsEncoding == "colliding_ansi_utf8");

	// Lecture de l'entete du fichier en parametrant le json tokeniser correctement
	JSONTokenizer::SetForceUnicodeToAnsi(bForceUnicodeToAnsi);

	// Initialisation du tokenizer pour analiser le rapport
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;
	sReportFileName = sFileName;
	bOk = JSONTokenizer::OpenForRead(GetClassLabel(), sFileName);
	if (bOk)
	{
		// Parsing
		bOk = InternalReadReport(coclusteringDataGrid, true);
		nInstanceNumber = nHeaderInstanceNumber;
		nCellNumber = nHeaderCellNumber;

		// Fermeture du tokenizer
		JSONTokenizer::Close();
	}
	sReportFileName = "";
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;

	JSONTokenizer::SetForceUnicodeToAnsi(false);

	return bOk;
}

const ALString CCCoclusteringReport::GetReportSuffix()
{
	return "khcj";
}

boolean CCCoclusteringReport::WriteReport(const ALString& sJSONReportName,
					  const CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	JSONFile fJSON;
	ALString sLocalTempFileName;

	require(coclusteringDataGrid != NULL);
	require(coclusteringDataGrid->Check());

	// Preparation de la copie sur HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sJSONReportName, sLocalTempFileName);

	// Ouverture du fichier de rapport JSON en ecriture
	if (bOk)
	{
		fJSON.SetFileName(sLocalTempFileName);
		bOk = fJSON.OpenForWrite();
	}

	// Ecriture de son contenu
	if (bOk)
	{
		InternalWriteReport(coclusteringDataGrid, &fJSON);

		// Fermeture du fichier
		fJSON.Close();
	}

	// Copie vers HDFS si necessaire
	if (bOk)
		PLRemoteFileService::CleanOutputWorkingFile(sJSONReportName, sLocalTempFileName);

	return bOk;
}

const ALString CCCoclusteringReport::GetClassLabel() const
{
	return "Coclustering report";
}

const ALString CCCoclusteringReport::GetObjectLabel() const
{
	ALString sLabel;

	if (fReport == NULL or nLineIndex == 0)
		sLabel = sReportFileName;
	else
		sLabel = sReportFileName + " (line " + IntToString(nLineIndex) + ")";
	return sLabel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Lecture des sections d'un rapport de coclustering

int CCCoclusteringReport::DetectFileFormatAndEncoding(const ALString& sFileName, ALString& sKhiopsEncoding) const
{
	boolean bOk = true;
	int nFileFormat;
	int nTokenType;
	ALString sTokenValue;

	require(not JSONTokenizer::IsOpened());

	// Initialisation du tokenizer pour analiser le rapport
	nFileFormat = None;
	bOk = JSONTokenizer::OpenForRead(GetClassLabel(), sFileName);
	if (bOk)
	{
		// Detection du format en lisant le premier token
		nTokenType = JSONTokenizer::ReadNextToken();

		// Choix du format en fonction du de ce caractere
		if (nTokenType == '{')
			nFileFormat = JSON;
		else if (nTokenType == JSONTokenizer::Error)
		{
			// On conserve temporairement la reconnaissance du format KHC afin de pouvoir informer l'utilisateur du fait que ce format n'est plus supporte
			if (JSONTokenizer::GetTokenStringValue() == "#")
				nFileFormat = KHC;
		}

		// Message d'erreur si pas de format reconnu
		if (nFileFormat == None)
			AddError("Format of file " + sFileName + " should be " +
				 CCCoclusteringReport::GetReportSuffix());

		// Message d'information si ancien format KHC
		if (nFileFormat == KHC)
			AddError("Khc format is no longer supported. Use json format file or use a Khiops version "
				 "prior to V11.");

		// Recherche de l'encodage dans le cas json
		if (nFileFormat == JSON)
		{
			assert(nTokenType == '{');
			while (nTokenType != 0)
			{
				nTokenType = JSONTokenizer::ReadNextToken();
				if (nTokenType == JSONTokenizer::String and
				    JSONTokenizer::GetTokenStringValue() == "khiops_encoding")
				{
					// Les deux tokens suivant doivent etre ':', puis la chaine contenant le type
					// d'encodage
					nTokenType = JSONTokenizer::ReadNextToken();
					if (nTokenType == ':')
					{
						nTokenType = JSONTokenizer::ReadNextToken();
						if (nTokenType == JSONTokenizer::String)
						{
							sKhiopsEncoding = JSONTokenizer::GetTokenStringValue();

							// On arrete car on a trouve la syntaxe correcte:
							// "khiops_encoding" : <string>
							break;
						}
					}
				}
			}
		}

		// Fermeture du tokenizer
		JSONTokenizer::Close();
	}
	return nFileFormat;
}

boolean CCCoclusteringReport::InternalReadReport(CCHierarchicalDataGrid* coclusteringDataGrid, boolean bHeaderOnly)
{
	boolean bOk = true;
	boolean bIsEnd = false;
	ALString sKey;
	ALString sValue;

	require(JSONTokenizer::IsOpened());
	require(coclusteringDataGrid != NULL);

	// Gestion des erreurs
	Global::ActivateErrorFlowControl();

	// Debut de fichier
	bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

	// Outil
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("tool", sValue, bIsEnd);
	if (bOk)
	{
		if (sValue != "Khiops Coclustering")
		{
			JSONTokenizer::AddParseError("Read tool \"" + sValue +
						     "\" instead of expected \"Khiops Coclustering\"");
			bOk = false;
		}
	}

	// Version
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("version", sValue, bIsEnd);

	// On ne lit d'abord que la cle, pour savoir si l'on part sur l'extraction de la description courte,
	// optionnelle, ou directement sur les information suvante
	bOk = bOk and JSONTokenizer::ReadStringValue(sKey);

	// Lecture de la description, depuis Khiops V10
	if (sKey == "shortDescription")
	{
		// Lecture de la valeur, et de l'indicateur de fin
		bOk = bOk and JSONTokenizer::ReadExpectedToken(':');
		bOk = bOk and JSONTokenizer::ReadStringValue(sValue);
		bOk = bOk and JSONTokenizer::ReadObjectNext(bIsEnd);

		// Memorisation
		if (bOk)
			coclusteringDataGrid->SetShortDescription(sValue);

		// Lecture de la cle suivante, pour la suite
		bOk = bOk and JSONTokenizer::ReadStringValue(sKey);
	}

	// Object principal
	if (bOk)
	{
		// Fn du parsing si ok
		if (sKey == "coclusteringReport")
		{
			bOk = bOk and JSONTokenizer::ReadExpectedToken(':');
			bOk = bOk and JSONTokenizer::ReadExpectedToken('{');
		}
		// Message d'erreur sinon
		else
		{
			JSONTokenizer::AddParseError("Read key \"coclusteringReport\" instead of expected \"" + sKey +
						     "\"");
			bOk = false;
		}
	}

	// Lecture chaque section du rapport JSON de coclustering
	bOk = bOk and ReadSummary(coclusteringDataGrid);
	bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
	bOk = bOk and ReadDimensionSummaries(coclusteringDataGrid);

	// Lecture detailles
	if (not bHeaderOnly)
	{
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
		bOk = bOk and ReadDimensionPartitions(coclusteringDataGrid);
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
		bOk = bOk and ReadDimensionHierarchies(coclusteringDataGrid);
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
		bOk = bOk and ReadCells(coclusteringDataGrid);
	}

	// Gestion des erreurs
	Global::DesactivateErrorFlowControl();
	return bOk;
}

boolean CCCoclusteringReport::ReadSummary(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	boolean bIsEnd = false;
	ALString sValue;
	double dCost;
	double dNullCost;
	double dLevel;
	int nInitialAttributeNumber;
	ALString sFrequencyVariable;
	ALString sDictionary;
	ALString sDatabase;
	double dSamplePercentage;
	ALString sSamplingMode;
	ALString sSelectionAttribute;
	ALString sSelectionValue;
	ALString sTmp;
	ALString sIdentifierVariable;

	require(coclusteringDataGrid != NULL);
	require(coclusteringDataGrid->GetAttributeNumber() == 0);

	// Object principal
	bOk = bOk and JSONTokenizer::ReadKeyObject("summary");

	// Initialisations
	dCost = 0;
	dNullCost = 0;
	dLevel = 0;
	nInitialAttributeNumber = 0;
	dSamplePercentage = 100;
	coclusteringDataGrid->GetDatabaseSpec()->SetModeExcludeSample(false);
	sSamplingMode = coclusteringDataGrid->GetDatabaseSpec()->GetSamplingMode();

	// Lecture des champs
	bOk = bOk and JSONTokenizer::ReadKeyIntValue("instances", true, nHeaderInstanceNumber, bIsEnd);
	bOk = bOk and JSONTokenizer::ReadKeyIntValue("cells", true, nHeaderCellNumber, bIsEnd);
	bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("nullCost", true, dNullCost, bIsEnd);
	bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("cost", true, dCost, bIsEnd);
	bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("level", true, dLevel, bIsEnd);
	bOk = bOk and JSONTokenizer::ReadKeyIntValue("initialDimensions", true, nInitialAttributeNumber, bIsEnd);
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("frequencyVariable", sFrequencyVariable, bIsEnd);
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("dictionary", sDictionary, bIsEnd);
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("database", sDatabase, bIsEnd);

	// Lecture des champs supplementaires sur la base, depuis Khiops V10
	if (not bIsEnd)
	{
		// Lecture des champs
		bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("samplePercentage", true, dSamplePercentage, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("samplingMode", sSamplingMode, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("selectionVariable", sSelectionAttribute, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("selectionValue", sSelectionValue, bIsEnd);

		// Verification de l'integrite
		if (bOk and dSamplePercentage > 100)
		{
			assert(dSamplePercentage >= 0);
			AddError(sTmp + "Invalid value of JSON key 'samplePercentage' (" +
				 DoubleToString(dSamplePercentage) + ")");
			bOk = false;
		}
		if (bOk and not KWDatabase::CheckSamplingMode(sSamplingMode))
		{
			AddError(sTmp + "Invalid value of JSON key 'samplingMode' (" + sSamplingMode + ")");
			bOk = false;
		}
	}

	// Initialisation des informations sur la grille
	if (bOk)
	{
		coclusteringDataGrid->SetCost(dCost);
		coclusteringDataGrid->SetNullCost(dNullCost);
		coclusteringDataGrid->SetInitialAttributeNumber(nInitialAttributeNumber);
		coclusteringDataGrid->SetFrequencyAttributeName(sFrequencyVariable);
		coclusteringDataGrid->GetDatabaseSpec()->SetClassName(sDictionary);
		coclusteringDataGrid->GetDatabaseSpec()->SetDatabaseName(sDatabase);
		coclusteringDataGrid->GetDatabaseSpec()->SetSampleNumberPercentage(dSamplePercentage);
		coclusteringDataGrid->GetDatabaseSpec()->SetSamplingMode(sSamplingMode);
		coclusteringDataGrid->GetDatabaseSpec()->SetSelectionAttribute(sSelectionAttribute);
		coclusteringDataGrid->GetDatabaseSpec()->SetSelectionValue(sSelectionValue);
	}

	// Fin de l'objet
	bOk = bOk and JSONTokenizer::CheckObjectEnd("summary", bIsEnd);

	// Reinitialisation des informations si echec
	if (not bOk)
	{
		AddError("Abort read");
		coclusteringDataGrid->DeleteAll();
		nHeaderInstanceNumber = 0;
		nHeaderCellNumber = 0;
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadDimensionSummaries(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	boolean bIsEnd = false;
	CCHDGAttribute* dgAttribute;
	ALString sAttributeName;
	ALString sAttributeType;
	int nAttributeType;
	int nAttributePartNumber;
	int nAttributeInitialPartNumber;
	int nAttributeValueNumber;
	double dAttributeInterest;
	ALString sAttributeDescription;
	Continuous cMin;
	Continuous cMax;
	int nPart;
	ALString sKey;
	boolean bIsVarPart;
	ALString sValue;
	int nAttribute;
	CCHDGAttribute* varPartAttribute;
	ALString sTmp;

	require(coclusteringDataGrid != NULL);
	require(coclusteringDataGrid->GetAttributeNumber() == 0);

	// Tableau principal
	bOk = bOk and JSONTokenizer::ReadKeyArray("dimensionSummaries");

	// Lecture des elements du tableau
	varPartAttribute = NULL;
	while (bOk and not bIsEnd)
	{
		// Initialisations
		sAttributeName = "";
		nAttributeType = 0;
		nAttributePartNumber = 0;
		nAttributeInitialPartNumber = 0;
		nAttributeValueNumber = 0;
		dAttributeInterest = 0;
		cMin = 0;
		cMax = 0;

		// Debut de l'objet
		bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

		// Nom de l'attribut
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("name", sAttributeName, bIsEnd);
		if (bOk and sAttributeName == "")
		{
			bOk = false;
			AddError("Missing variable name");
		}
		if (bOk and coclusteringDataGrid->SearchAttribute(sAttributeName) != NULL)
		{
			bOk = false;
			AddError("Variable " + sAttributeName + " used twice");
		}

		// Lecture de la prochaine cle explicitement, pour traiter la cle optionnelle "isVarPart" precedent la
		// cle "type", depuis la prise en compte du coclustering instances x variables
		bOk = bOk and JSONTokenizer::ReadStringValue(sKey);

		// Lecture du champ optionnel "isVarPart"
		bIsVarPart = false;
		if (bOk and sKey == "isVarPart")
		{
			bOk = bOk and JSONTokenizer::ReadExpectedToken(':');
			bOk = bOk and JSONTokenizer::ReadBooleanValue(bIsVarPart);
			bOk = bOk and JSONTokenizer::ReadObjectNext(bIsEnd);

			// Lecture ensuite du champ "type"
			bOk = bOk and JSONTokenizer::ReadKeyStringValue("type", sAttributeType, bIsEnd);
		}
		// Lecture directe du champ "type"
		else if (bOk and sKey == "type")
		{
			bOk = bOk and JSONTokenizer::ReadExpectedToken(':');
			bOk = bOk and JSONTokenizer::ReadStringValue(sAttributeType);
			bOk = bOk and JSONTokenizer::ReadObjectNext(bIsEnd);
		}
		// Erreur sinon
		else if (bOk and sKey != "isVarPart" and sKey != "type")
		{
			bOk = false;
			JSONTokenizer::AddParseError("Read key \"" + sKey +
						     "\" instead of expected of \"isVarPart\" or \"type\"");
		}

		// Verification du type de l'attribut
		nAttributeType = KWType::ToType(sAttributeType);
		if (bOk and not KWType::IsSimple(nAttributeType))
		{
			bOk = false;
			JSONTokenizer::AddParseError("Type of variable " + sAttributeName + " (" + sAttributeType +
						     ") should be Numerical or Categorical");
		}
		else if (bOk and bIsVarPart and nAttributeType != KWType::Symbol)
		{
			bOk = false;
			JSONTokenizer::AddParseError("Type of VarPart variable " + sAttributeName + " (" +
						     sAttributeType + ") should be Categorical");
		}

		// Changement du type de l'attribut en VarPart si specifie
		if (bOk and bIsVarPart)
			nAttributeType = KWType::VarPart;

		// Champs restants de l'attribut
		bOk = bOk and JSONTokenizer::ReadKeyIntValue("parts", true, nAttributePartNumber, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyIntValue("initialParts", true, nAttributeInitialPartNumber, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyIntValue("values", true, nAttributeValueNumber, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("interest", true, dAttributeInterest, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("description", sAttributeDescription, bIsEnd);

		// Valeur min et max dans le cas numerique
		if (bOk and nAttributeType == KWType::Continuous)
		{
			bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("min", false, cMin, bIsEnd);
			bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("max", false, cMax, bIsEnd);
		}

		// Initialisation de la grille ou ajout d'un attribut
		if (bOk)
		{
			assert(coclusteringDataGrid->GetTargetValueNumber() == 0);
			coclusteringDataGrid->AddAttribute();

			// Tests d'integrite par rapport au nombre de dimensions initial
			if (bOk and coclusteringDataGrid->GetAttributeNumber() >
					coclusteringDataGrid->GetInitialAttributeNumber())
			{
				bOk = false;
				JSONTokenizer::AddParseError(
				    "Variable " + sAttributeName + " added (index " +
				    IntToString(coclusteringDataGrid->GetAttributeNumber()) +
				    ") beyond the number of initial variables (" +
				    IntToString(coclusteringDataGrid->GetInitialAttributeNumber()) + ")");
			}
		}

		// Erreur si tentative d'ajout d'un second attribut de type VarPart
		if (bOk and varPartAttribute != NULL and nAttributeType == KWType::VarPart)
		{
			bOk = false;
			AddError("Incorrect use of two variables of type VarPart: " +
				 varPartAttribute->GetAttributeName() + " and " + sAttributeName);
		}

		// Specification de l'attribut
		if (bOk)
		{
			dgAttribute =
			    cast(CCHDGAttribute*,
				 coclusteringDataGrid->GetAttributeAt(coclusteringDataGrid->GetAttributeNumber() - 1));
			dgAttribute->SetAttributeName(sAttributeName);
			dgAttribute->SetAttributeType(nAttributeType);
			dgAttribute->SetInitialPartNumber(nAttributeInitialPartNumber);
			dgAttribute->SetInitialValueNumber(nAttributeValueNumber);
			dgAttribute->SetGranularizedValueNumber(nAttributeValueNumber);
			dgAttribute->SetInterest(dAttributeInterest);
			dgAttribute->SetDescription(sAttributeDescription);

			// Memorisation dans le cas d'un attribut de type VarPart
			if (nAttributeType == KWType::VarPart)
			{
				varPartAttribute = dgAttribute;

				// Creation des attributs internes
				dgAttribute->SetInnerAttributes(new KWDGInnerAttributes);
			}

			// Valeur min et max dans le cas numerique
			if (nAttributeType == KWType::Continuous)
			{
				dgAttribute->SetMin(cMin);
				dgAttribute->SetMax(cMax);
			}

			// Creation de parties
			for (nPart = 0; nPart < nAttributePartNumber; nPart++)
				dgAttribute->AddPart();
		}

		// Fin de l'objet et test si nouvel objet
		bOk = bOk and JSONTokenizer::CheckObjectEnd("dimensionSummaries", bIsEnd);
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
	}

	// Prise en compte d'un coclustering de type VarPart
	// CH IV Refactoring: potentiellement inutile apres la fin du refactoring
	// CH IV Refactoring: on le conserve pour l'instant dans l'hypothese ou l'on etende le coclustering IV a plusieurs variables hors variable varPart
	if (bOk and varPartAttribute != NULL)
	{
		// Parametrage de l'attribut identifiant, en prenant le premier attribut catgoriel trouve
		for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
		{
			dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));
			if (dgAttribute->GetAttributeType() == KWType::Symbol)
				coclusteringDataGrid->SetIdentifierAttributeName(dgAttribute->GetAttributeName());
		}
	}

	// Tests d'integrite dans le cas d'un coclustering de type instances * variables
	// CH IV Refactoring: potentiellement inutile apres la fin du refactoring
	// CH IV Refactoring: a voir ulterieurement en fonction de l'extension a plus que deux variables
	if (bOk and coclusteringDataGrid->IsVarPartDataGrid())
	{
		assert(varPartAttribute == coclusteringDataGrid->GetVarPartAttribute());

		// Verification du nombre d'attributs
		if (coclusteringDataGrid->GetAttributeNumber() != 2)
		{
			AddError(
			    sTmp +
			    "Number of variables for instance * variables coclustering should be 2 and is equal to (" +
			    IntToString(coclusteringDataGrid->GetAttributeNumber()) + ")");
			bOk = false;
		}
	}
	if (bOk and coclusteringDataGrid->IsVarPartDataGrid())
	{
		// Verification du type des attributs : categoriel (ou numerique) pour le 1er, VarPart pour le second
		if (not KWType::IsSimple(coclusteringDataGrid->GetAttributeAt(0)->GetAttributeType()) or
		    coclusteringDataGrid->GetAttributeAt(1)->GetAttributeType() != KWType::VarPart)
		{
			bOk = false;
			AddError("Incorrect type of variables for instance * variables coclustering ");
		}
	}

	return bOk;
}

boolean CCCoclusteringReport::ReadInnerAttributesDimensionSummaries(KWDGAttribute* dgAttribute)
{
	boolean bOk = true;
	boolean bIsEnd = false;
	CCHDGAttribute* innerAttribute;
	ALString sAttributeName;
	ALString sAttributeType;
	int nAttributeType;
	int nAttributePartNumber;
	int nAttributeInitialPartNumber;
	int nAttributeValueNumber;
	double dAttributeInterest;
	ALString sAttributeDescription;
	Continuous cMin;
	Continuous cMax;
	int nPart;
	ALString sKey;
	ALString sValue;
	ALString sTmp;
	KWDGInnerAttributes* innerAttributes;

	require(dgAttribute != NULL);

	// Creation des attributs internes
	innerAttributes = new KWDGInnerAttributes();
	dgAttribute->SetInnerAttributes(innerAttributes);

	// Tableau principal
	bOk = bOk and JSONTokenizer::ReadKeyArray("dimensionSummaries");

	// Lecture des elements du tableau
	while (bOk and not bIsEnd)
	{
		// Initialisations
		sAttributeName = "";
		nAttributeType = 0;
		nAttributePartNumber = 0;
		nAttributeInitialPartNumber = 0;
		nAttributeValueNumber = 0;
		dAttributeInterest = 0;
		cMin = 0;
		cMax = 0;

		// Debut de l'objet
		bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

		// Nom de l'attribut
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("name", sAttributeName, bIsEnd);
		if (bOk and sAttributeName == "")
		{
			bOk = false;
			AddError("Missing variable name");
		}

		if (bOk and innerAttributes->LookupInnerAttribute(sAttributeName) != NULL)
		{
			bOk = false;
			AddError("Inner variable " + sAttributeName + " used twice");
		}

		// Lecture du champ "type"
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("type", sAttributeType, bIsEnd);

		// Verification du type de l'attribut
		nAttributeType = KWType::ToType(sAttributeType);
		if (bOk and not KWType::IsSimple(nAttributeType))
		{
			bOk = false;
			JSONTokenizer::AddParseError("Type of inner variable " + sAttributeName + " (" +
						     sAttributeType + ") should be Numerical or Categorical");
		}

		// Champs restants de l'attribut
		bOk = bOk and JSONTokenizer::ReadKeyIntValue("parts", true, nAttributePartNumber, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyIntValue("initialParts", true, nAttributeInitialPartNumber, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyIntValue("values", true, nAttributeValueNumber, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("interest", true, dAttributeInterest, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("description", sAttributeDescription, bIsEnd);

		// Valeur min et max dans le cas numerique
		if (bOk and nAttributeType == KWType::Continuous)
		{
			bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("min", false, cMin, bIsEnd);
			bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("max", false, cMax, bIsEnd);
		}

		// Creation et specification de l'attribut interne
		if (bOk)
		{
			innerAttribute = new CCHDGAttribute;
			innerAttribute->SetAttributeName(sAttributeName);
			innerAttribute->SetAttributeType(nAttributeType);
			innerAttribute->SetInitialPartNumber(nAttributeInitialPartNumber);
			innerAttribute->SetInitialValueNumber(nAttributeValueNumber);
			innerAttribute->SetGranularizedValueNumber(nAttributeValueNumber);
			innerAttribute->SetInterest(dAttributeInterest);
			innerAttribute->SetDescription(sAttributeDescription);
			innerAttribute->SetOwnerAttributeName(dgAttribute->GetAttributeName());

			// Valeur min et max dans le cas numerique
			if (nAttributeType == KWType::Continuous)
			{
				innerAttribute->SetMin(cMin);
				innerAttribute->SetMax(cMax);
			}

			// Creation de parties
			for (nPart = 0; nPart < nAttributePartNumber; nPart++)
				innerAttribute->AddPart();

			innerAttributes->AddInnerAttribute(innerAttribute);
		}

		// Fin de l'objet et test si nouvel objet
		bOk = bOk and JSONTokenizer::CheckObjectEnd("dimensionSummaries", bIsEnd);
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
	}

	return bOk;
}

boolean CCCoclusteringReport::ReadDimensionPartitions(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	ObjectDictionary odCheckedParts;
	ALString sPartName;
	boolean bIsAttributeEnd;
	int nAttributeIndex;
	CCHDGAttribute* dgAttribute;
	ALString sAttributeName;
	ALString sAttributeType;
	int nAttributeType;
	int nDefaultGroupIndex;
	ALString sValue;
	ALString sTmp;

	require(coclusteringDataGrid != NULL);

	// Tableau principal
	bOk = bOk and JSONTokenizer::ReadKeyArray("dimensionPartitions");

	// Lecture des elements du tableau
	nAttributeIndex = 0;
	bIsAttributeEnd = false;
	while (bOk and not bIsAttributeEnd)
	{
		// Recherche de l'attribut courant
		if (nAttributeIndex < coclusteringDataGrid->GetAttributeNumber())
		{
			dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttributeIndex));
			nAttributeIndex++;
		}
		// Erreur si trop de variables
		else
		{
			JSONTokenizer::AddParseError(
			    sTmp + "Too many variables in section \"dimensionPartitions\" (expected number: " +
			    IntToString(coclusteringDataGrid->GetAttributeNumber()) + ")");
			bOk = false;
			break;
		}

		// Initialisations
		sAttributeName = "";
		nAttributeType = 0;
		nDefaultGroupIndex = 0;

		// Debut de l'objet
		bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

		// Nom de l'attribut
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("name", sAttributeName, bIsAttributeEnd);
		if (bOk and dgAttribute->GetAttributeName() != sAttributeName)
		{
			JSONTokenizer::AddParseError("Read variable \"" + sAttributeName + "\" instead of expected \"" +
						     dgAttribute->GetAttributeName() + "\"");
			bOk = false;
		}

		// Type de l'attribut
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("type", sAttributeType, bIsAttributeEnd);
		if (bOk and KWType::ToString(KWType::GetCoclusteringSimpleType(dgAttribute->GetAttributeType())) !=
				sAttributeType)
		{
			JSONTokenizer::AddParseError(
			    "Read variable type " + sAttributeType + " instead of expected " +
			    KWType::ToString(KWType::GetCoclusteringSimpleType(dgAttribute->GetAttributeType())));
			bOk = false;
		}

		bOk = bOk and ReadAttributePartition(dgAttribute, coclusteringDataGrid);

		// Test si nouvel objet
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsAttributeEnd);
	}

	// Erreur si pas assez de variables
	if (bOk and nAttributeIndex <= coclusteringDataGrid->GetAttributeNumber() - 1)
	{
		JSONTokenizer::AddParseError(sTmp +
					     "Too few variables in section \"dimensionPartitions\" (expected number: " +
					     IntToString(coclusteringDataGrid->GetAttributeNumber()) + ")");
		bOk = false;
	}

	return bOk;
}

boolean CCCoclusteringReport::ReadAttributePartition(KWDGAttribute* dgAttribute,
						     CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	boolean bIsPartEnd;
	const KWDGInnerAttributes* innerAttributes;
	CCHDGAttribute* innerAttribute;
	KWDGPart* dgPart;
	int nPartIndex;
	boolean bIsInnerAttributeEnd;
	ObjectDictionary odInnerAttributesAllVarParts;
	ObjectDictionary odVarPartAttributeAllVarParts;
	ObjectDictionary odCheckedParts;
	ALString sPartName;
	ALString sAttributeName;
	int nInnerAttributeIndex;
	ALString sTmp;
	int nInnerAttributeType;
	int nDefaultGroupIndex;
	ALString sInnerAttributeType;

	// Tableau d'intervalles ou de groupes de valeurs selon le type d'attribut
	if (dgAttribute->GetAttributeType() == KWType::Continuous)
		bOk = bOk and JSONTokenizer::ReadKeyArray("intervals");
	else if (dgAttribute->GetAttributeType() == KWType::Symbol)
		bOk = bOk and JSONTokenizer::ReadKeyArray("valueGroups");
	// Cas d'un attribut VarPart
	else if (dgAttribute->GetAttributeType() == KWType::VarPart)
	{
		// Creation des attributs internes
		innerAttributes = dgAttribute->GetInnerAttributes();

		// Tableau principal
		bOk = bOk and JSONTokenizer::ReadKeyObject("innerVariables");

		// Descriptif des variables internes dans l'attribut de type VarPart
		bOk = bOk and ReadInnerAttributesDimensionSummaries(dgAttribute);
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');

		// Tableau des variables internes dans l'attribut de type VarPart
		bOk = bOk and JSONTokenizer::ReadKeyArray("dimensionPartitions");

		// Lecture des elements du tableau
		nInnerAttributeIndex = 0;
		bIsInnerAttributeEnd = false;
		while (bOk and not bIsInnerAttributeEnd)
		{
			// Recherche de l'attribut courant
			if (nInnerAttributeIndex < dgAttribute->GetInnerAttributeNumber())
			{
				// Extraction de l'attribut interne
				innerAttribute =
				    cast(CCHDGAttribute*, dgAttribute->GetInnerAttributeAt(nInnerAttributeIndex));
				nInnerAttributeIndex++;
			}
			// Erreur si trop de variables
			else
			{
				JSONTokenizer::AddParseError(
				    sTmp + "Too many inner variables in section \"innerVariables\" (expected number: " +
				    IntToString(dgAttribute->GetInnerAttributeNumber()) + ")");
				bOk = false;
				break;
			}

			// Initialisations
			sAttributeName = "";
			nInnerAttributeType = 0;
			nDefaultGroupIndex = 0;

			// Debut de l'objet
			bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

			// Nom de l'attribut
			bOk = bOk and JSONTokenizer::ReadKeyStringValue("name", sAttributeName, bIsInnerAttributeEnd);
			if (bOk and innerAttribute->GetAttributeName() != sAttributeName)
			{
				JSONTokenizer::AddParseError("Read variable \"" + sAttributeName +
							     "\" instead of expected \"" +
							     innerAttribute->GetAttributeName() + "\"");
				bOk = false;
			}

			// Type de l'attribut
			bOk = bOk and
			      JSONTokenizer::ReadKeyStringValue("type", sInnerAttributeType, bIsInnerAttributeEnd);
			if (bOk and KWType::ToString(KWType::GetCoclusteringSimpleType(
					innerAttribute->GetAttributeType())) != sInnerAttributeType)
			{
				JSONTokenizer::AddParseError("Read variable type " + sInnerAttributeType +
							     " instead of expected " +
							     KWType::ToString(KWType::GetCoclusteringSimpleType(
								 innerAttribute->GetAttributeType())));
				bOk = false;
			}

			bOk = bOk and ReadAttributePartition(innerAttribute, coclusteringDataGrid);

			// Memorisation des informations sur les parties de l'attribut
			if (bOk)
			{
				// Memorisation de chaque partie
				dgPart = innerAttribute->GetHeadPart();
				while (dgPart != NULL)
				{
					sPartName = cast(CCHDGPart*, dgPart)->GetPartName();

					// Test d'unicite du libelle de partie parmi l'ensemble de tous les libelles de partie
					if (odInnerAttributesAllVarParts.Lookup(sPartName) != NULL)
					{
						JSONTokenizer::AddParseError("Attribute contains value \"" + sPartName +
									     "\" already used previously");
						bOk = false;
						break;
					}

					// Memorisation de la PV avec son label dans un dictionnaire temporaire
					odInnerAttributesAllVarParts.SetAt(sPartName, dgPart);

					// Partie suivante
					innerAttribute->GetNextPart(dgPart);
				}
			}

			// Test si nouvel objet
			bOk = bOk and JSONTokenizer::ReadArrayNext(bIsInnerAttributeEnd);
		}

		// Nettoyage des attributs internes si erreur
		if (not bOk)
		{
			// Cette methode va entrainer la destruction des attributs internes existants.
			// puisqu'il sont utilises uniquement a cet endroit
			dgAttribute->SetInnerAttributes(NULL);
		}

		// Fin de l'objet
		bOk = bOk and JSONTokenizer::ReadExpectedToken('}');
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');

		// Lecture des parties "valueGroups", groupes de parties de variables
		bOk = bOk and JSONTokenizer::ReadKeyArray("valueGroups");
	}

	// Lecture des parties
	dgPart = dgAttribute->GetHeadPart();
	bIsPartEnd = false;
	nPartIndex = 0;
	while (bOk and not bIsPartEnd)
	{
		// Acces a la partie si elle est disponible
		if (dgPart == NULL)
		{
			// Cas d'un attribut de la grille
			if (not dgAttribute->IsInnerAttribute())
			{

				JSONTokenizer::AddParseError("Variable " + sAttributeName + " added part (index " +
							     IntToString(nPartIndex + 1) +
							     ") beyond the expected number of parts (" +
							     IntToString(dgAttribute->GetPartNumber()) + ")");
				bOk = false;
				break;
			}
			// Cas d'un innerAttribute
			// Dans ce cas, les parties ne sont pas encore crees, on ne connait pas leur nombre
			// L'initialisation des dgAttribute effectuee dans ReadJSONDimensionSummaries n'est pas effectuee pour ces attributes
			else
			{
				dgAttribute->AddPart();
			}
		}
		else
			nPartIndex++;

		// Intervalles, groupes de valeurs ou groupe de parties de variables
		// selon le type d'attribut
		if (bOk)
		{
			if (dgAttribute->GetAttributeType() == KWType::Continuous)
				bOk = bOk and ReadInterval(dgAttribute, dgPart);
			else if (dgAttribute->GetAttributeType() == KWType::Symbol)
				bOk = bOk and ReadValueGroup(dgAttribute, dgPart);
			else if (dgAttribute->GetAttributeType() == KWType::VarPart)
			{
				// Dans le cas instances x variables, on procede a des tests pousses sur
				// la coherences des varPartIds
				//  - tous les varPartIds declare dans les variables internes doivent
				//  etre uniques
				//  - tous les varPartIds utilises dans les grpoupes de parties de
				//  variable de l'attribut VarPart doivent
				//    etre uniques, et correspondre a des varPartIds declares dans les
				//    attributs internes
				//  - les deux ensembles de varPartIds doivent etre exactement les meme
				//  (tous ceux declares sont utilises)
				// Par contre, lors de la regeneration du fichier json, ces libelles
				// sont deduits des parties, et ne sont pas necessairement ceux qui
				// etaient en entree (mais la structure est preservee).
				bOk = bOk and
				      ReadVarPartAttributeValueGroup(dgAttribute, dgPart, &odInnerAttributesAllVarParts,
								     &odVarPartAttributeAllVarParts);
			}
		}

		// Test si identifiant de partie unique
		if (bOk)
		{
			sPartName = cast(CCHDGPart*, dgPart)->GetPartName();
			if (odCheckedParts.Lookup(sPartName) != NULL)
			{
				JSONTokenizer::AddParseError("\"cluster\" " + sPartName + " used twice");
				bOk = false;
			}
			else
				odCheckedParts.SetAt(sPartName, dgPart);
		}

		// Test si nouvelle partie
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsPartEnd);

		// Partie suivante
		if (bOk)
			dgAttribute->GetNextPart(dgPart);
		else
			break;
	}

	// Erreur s'il manque des parties
	if (bOk and nPartIndex < dgAttribute->GetPartNumber())
	{
		JSONTokenizer::AddParseError("Variable " + sAttributeName + " with " + IntToString(nPartIndex) +
					     " specified parts " + ") below the expected number of parts (" +
					     IntToString(dgAttribute->GetPartNumber()) + ")");
		bOk = false;
	}

	// Erreur si le nombre de parties de variable collectees dans les groupes de partie de variable
	// est different du nombre de parties de variables specifie dans les variables internes
	if (bOk and dgAttribute->GetAttributeType() == KWType::VarPart and
	    odInnerAttributesAllVarParts.GetCount() != odVarPartAttributeAllVarParts.GetCount())
	{
		assert(odVarPartAttributeAllVarParts.GetCount() < odInnerAttributesAllVarParts.GetCount());
		JSONTokenizer::AddParseError(
		    "VarPart variable " + sAttributeName + " contains " +
		    IntToString(odVarPartAttributeAllVarParts.GetCount()) +
		    " variable parts in its groups, below the expected number of variable parts (" +
		    IntToString(odInnerAttributesAllVarParts.GetCount()) + ") specified globally in inner variables");
		bOk = false;
	}

	// Fin de l'objet
	if (bOk)
	{
		// Dans le cas numerique
		if (dgAttribute->GetAttributeType() == KWType::Continuous)
		{
			// Lecture de la fin de l'objet
			bOk = bOk and JSONTokenizer::ReadExpectedToken('}');

			// Cas particulier ou le premier intervalle est Missing: il faut modifier la borne inf
			// du second pour la mettre a missing
			if (bOk and dgAttribute->GetPartNumber() >= 2 and
			    dgAttribute->GetHeadPart()->GetInterval()->GetLowerBound() ==
				KWContinuous::GetMissingValue())
			{
				// Acces au premier intervalle
				dgPart = dgAttribute->GetHeadPart();
				assert(dgPart->GetInterval()->GetUpperBound() == KWContinuous::GetMissingValue());

				// Modification du deuxieme intervalle
				dgAttribute->GetNextPart(dgPart);
				assert(dgPart->GetInterval()->GetLowerBound() != KWContinuous::GetMissingValue());
				dgPart->GetInterval()->SetLowerBound(KWContinuous::GetMissingValue());
			}

			// On modifie les bornes des intervalles extremes de facon a
			// pouvoir produire les libelle avec -inf et +inf par GetObjectLabel
			dgAttribute->GetHeadPart()->GetInterval()->SetLowerBound(KWDGInterval::GetMinLowerBound());
			dgAttribute->GetTailPart()->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());
		}
		// Dans le cas categoriel ou VarPart
		else
		{
			assert(dgAttribute->GetAttributeType() == KWType::Symbol or
			       dgAttribute->GetAttributeType() == KWType::VarPart);
			nDefaultGroupIndex = 0;

			// Lecture de l'index du groupe par defaut avant la fin de l'objet
			bIsPartEnd = false;
			bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
			bOk = bOk and
			      JSONTokenizer::ReadKeyIntValue("defaultGroupIndex", true, nDefaultGroupIndex, bIsPartEnd);
			if (bOk and not bIsPartEnd)
			{
				bOk = false;
				JSONTokenizer::AddParseError(
				    "Read token " + JSONTokenizer::GetTokenLabel(JSONTokenizer::GetLastToken()) +
				    " instead of expected  '}'");
				bOk = false;
			}

			// Finalisation de la partition en groupes de valeurs
			if (bOk and (nDefaultGroupIndex < 0 or nDefaultGroupIndex >= dgAttribute->GetPartNumber()))
			{
				bOk = false;
				JSONTokenizer::AddParseError("Variable " + sAttributeName +
							     " with invalid default group index (" +
							     IntToString(nDefaultGroupIndex) + ")");
			}
			// Gestion de la valeur par defaut uniquement dans le cas standard
			// Dans le cas instances x variables, le groupe par defaut n'est pas utilise,
			// mais on a garde "defaultGroupIndex" pour avoir des rapports generiques
			if (bOk and dgAttribute->GetAttributeType() == KWType::Symbol)
			{
				// Recherche du groupe par defaut
				nPartIndex = 0;
				dgPart = dgAttribute->GetHeadPart();
				while (nPartIndex < nDefaultGroupIndex)
				{
					nPartIndex++;
					dgAttribute->GetNextPart(dgPart);
				}

				// Ajout de la valeur par defaut dans ce groupe
				assert(dgPart != NULL);
				cast(KWDGSymbolValueSet*, dgPart->GetValueSet())
				    ->AddSymbolValue(Symbol::GetStarValue());
			}
		}
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadInterval(KWDGAttribute* dgAttribute, KWDGPart* dgPart)
{
	boolean bOk = true;
	boolean bIsEnd;
	int nToken;
	CCHDGPart* hdgPart;
	KWDGInterval* dgInterval;
	ALString sClusterName;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(dgAttribute != NULL);
	require(dgPart != NULL);

	// Debut de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

	// Initialisations
	cLowerBound = 0;
	cUpperBound = 0;

	// Lecture des champs
	bIsEnd = false;
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("cluster", sClusterName, bIsEnd);
	bOk = bOk and JSONTokenizer::ReadKeyArray("bounds");

	// Lecture des bornes
	if (bOk)
	{
		nToken = JSONTokenizer::ReadNextToken();

		// Cas missing, avec un tableau vide
		if (nToken == ']')
		{
			cLowerBound = KWContinuous::GetMissingValue();
			cUpperBound = KWContinuous::GetMissingValue();
		}
		// Lecture des bornes inf et sup, avec tableau de deux valeurs
		else if (nToken == JSONTokenizer::Number)
		{
			// Acces a la valeur de la borne inf, qui vient d'etre lue
			cLowerBound = JSONTokenizer::GetTokenNumberValue();

			// Lecture de la borne sup
			bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
			bOk = bOk and JSONTokenizer::ReadDoubleValue(false, cUpperBound);
			bOk = bOk and JSONTokenizer::ReadExpectedToken(']');
		}
	}

	// Fin de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('}');

	// Memorisation des informations sur l'intervalle
	if (bOk)
	{
		hdgPart = cast(CCHDGPart*, dgPart);
		dgInterval = dgPart->GetInterval();
		hdgPart->SetPartName(sClusterName);
		dgInterval->SetLowerBound(cLowerBound);
		dgInterval->SetUpperBound(cUpperBound);
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadValueGroup(KWDGAttribute* dgAttribute, KWDGPart* dgPart)
{
	boolean bOk = true;
	ObjectDictionary odChekedValues;
	boolean bIsEnd;
	KWDGSymbolValueSet* dgValueSet;
	KWDGValue* dgValue;
	CCHDGPart* hdgPart;
	ALString sClusterName;
	ALString sValue;
	int nValueFrequency;
	StringVector svValues;
	IntVector ivValueFrequencies;
	DoubleVector dvValueTypicalities;
	int i;
	ALString sTmp;

	require(dgAttribute != NULL);
	require(dgPart != NULL);

	// Debut de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

	// Nom du cluster
	bIsEnd = false;
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("cluster", sClusterName, bIsEnd);
	if (bOk and sClusterName == "")
	{
		JSONTokenizer::AddParseError("\"cluster\" should have a non empty value");
		bOk = false;
	}

	// Tableau des valeurs
	bOk = bOk and JSONTokenizer::ReadKeyArray("values");
	bIsEnd = false;
	while (bOk and not bIsEnd)
	{
		bOk = bOk and JSONTokenizer::ReadStringValue(sValue);
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
		if (bOk)
		{
			if (odChekedValues.Lookup(sValue) != NULL)
			{
				JSONTokenizer::AddParseError("\"values\" contains a duplicate value (" + sValue + ")");
				bOk = false;
			}
			else
			{
				odChekedValues.SetAt(sValue, &odChekedValues);
				svValues.Add(sValue);
			}
		}
	}
	bOk = bOk and JSONTokenizer::ReadExpectedToken(',');

	// Tableau des effectifs
	bOk = bOk and JSONTokenizer::ReadKeyArray("valueFrequencies");
	bIsEnd = false;
	nValueFrequency = 0;
	while (bOk and not bIsEnd)
	{
		bOk = bOk and JSONTokenizer::ReadIntValue(true, nValueFrequency);
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
		if (bOk)
			ivValueFrequencies.Add(nValueFrequency);
	}
	if (bOk and svValues.GetSize() != ivValueFrequencies.GetSize())
	{
		JSONTokenizer::AddParseError("Vector \"valueFrequencies\" should be of same size as vector \"values\"");
		bOk = false;
	}

	// Typicalite, sauf si la variable est interne
	if (not dgAttribute->IsInnerAttribute())
	{
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');

		// Tableau des typicalites
		bOk = bOk and ReadTypicalities(dgAttribute, svValues.GetSize(), &dvValueTypicalities);
	}

	// Fin de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('}');

	// Memorisation des informations sur le groupe de valeurs
	if (bOk)
	{
		hdgPart = cast(CCHDGPart*, dgPart);
		dgValueSet = cast(KWDGSymbolValueSet*, dgPart->GetValueSet());
		hdgPart->SetPartName(sClusterName);
		hdgPart->SetShortDescription("");
		hdgPart->SetDescription("");

		// Memorisation des valeurs
		for (i = 0; i < svValues.GetSize(); i++)
		{
			dgValue = dgValueSet->AddSymbolValue((Symbol)svValues.GetAt(i));
			dgValue->SetValueFrequency(ivValueFrequencies.GetAt(i));
			if (not dgAttribute->IsInnerAttribute())
				dgValue->SetTypicality(dvValueTypicalities.GetAt(i));
		}
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadVarPartAttributeValueGroup(KWDGAttribute* varPartAttribute, KWDGPart* dgPart,
							     const ObjectDictionary* odInnerAttributesAllVarParts,
							     ObjectDictionary* odVarPartAttributeAllVarParts)
{
	boolean bOk = true;
	boolean bIsEnd;
	KWDGVarPartSet* dgVarPartSet;
	KWDGValue* dgValue;
	CCHDGPart* hdgPart;
	ALString sClusterName;
	ALString sValue;
	int nValueFrequency;
	StringVector svValues;
	IntVector ivValueFrequencies;
	DoubleVector dvValueTypicalities;
	int i;
	KWDGPart* dgVarPart;
	KWDGPart* dgCheckedVarPart;
	ALString sTmp;

	require(varPartAttribute != NULL);
	require(varPartAttribute->GetAttributeType() == KWType::VarPart);
	require(dgPart != NULL);
	require(odInnerAttributesAllVarParts != NULL);
	require(odVarPartAttributeAllVarParts != NULL);

	// Debut de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

	// Nom du cluster
	bIsEnd = false;
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("cluster", sClusterName, bIsEnd);
	if (bOk and sClusterName == "")
	{
		JSONTokenizer::AddParseError("\"cluster\" should have a non empty value");
		bOk = false;
	}

	// Tableau des libelles des parties de variable, analogue des valeurs d'un variable categorielle
	bOk = bOk and JSONTokenizer::ReadKeyArray("values");
	bIsEnd = false;
	while (bOk and not bIsEnd)
	{
		bOk = bOk and JSONTokenizer::ReadStringValue(sValue);
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
		if (bOk)
		{
			// Verification de l'existence de la partie de variable
			dgVarPart = cast(KWDGPart*, odInnerAttributesAllVarParts->Lookup(sValue));
			if (dgVarPart == NULL)
			{
				JSONTokenizer::AddParseError("\"values\" of VarPart variable " +
							     varPartAttribute->GetAttributeName() +
							     " contains variable part \"" + sValue +
							     "\" which was not specified among all the \"cluster\" "
							     "vectors in the \"innerVariables\" section");
				bOk = false;
				break;
			}

			// Verification de l'unicite des partie de variables dans l'ensemble des groupes de l'attribut
			// de type VarPart
			dgCheckedVarPart = cast(KWDGPart*, odVarPartAttributeAllVarParts->Lookup(sValue));
			if (dgCheckedVarPart != NULL)
			{
				JSONTokenizer::AddParseError("\"values\" of VarPart variable " +
							     varPartAttribute->GetAttributeName() +
							     " contains a duplicate variable part (" + sValue + ")");
				bOk = false;
			}

			// Prise en compte si ok
			if (bOk)
			{
				assert(dgVarPart != NULL);
				assert(dgCheckedVarPart == NULL);
				svValues.Add(sValue);

				// Memorisation dans le dictionnaire des controle d'unicite
				odVarPartAttributeAllVarParts->SetAt(sValue, dgVarPart);
			}
		}
	}
	bOk = bOk and JSONTokenizer::ReadExpectedToken(',');

	// Tableau des effectifs
	bOk = bOk and JSONTokenizer::ReadKeyArray("valueFrequencies");
	bIsEnd = false;
	nValueFrequency = 0;
	while (bOk and not bIsEnd)
	{
		bOk = bOk and JSONTokenizer::ReadIntValue(true, nValueFrequency);
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
		if (bOk)
			ivValueFrequencies.Add(nValueFrequency);
	}
	if (bOk and svValues.GetSize() != ivValueFrequencies.GetSize())
	{
		JSONTokenizer::AddParseError("Vector \"valueFrequencies\" should be of same size as vector \"values\"");
		bOk = false;
	}
	bOk = bOk and JSONTokenizer::ReadExpectedToken(',');

	// Tableau des typicalites
	bOk = bOk and ReadTypicalities(varPartAttribute, svValues.GetSize(), &dvValueTypicalities);

	// Fin de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('}');

	// Memorisation des informations sur le groupe de parties de variable
	if (bOk)
	{
		hdgPart = cast(CCHDGPart*, dgPart);
		dgVarPartSet = dgPart->GetVarPartSet();
		hdgPart->SetPartName(sClusterName);
		hdgPart->SetShortDescription("");
		hdgPart->SetDescription("");

		// Memorisation des parties de variable identifiees par leur label
		for (i = 0; i < svValues.GetSize(); i++)
		{
			// Recherche de la partie de variable d'apres son libelle
			dgVarPart = cast(KWDGPart*, odInnerAttributesAllVarParts->Lookup((Symbol)svValues.GetAt(i)));
			assert(dgVarPart != NULL);

			// Ajout de la PV identifiee par son Label au cluster de PV
			dgValue = dgVarPartSet->AddVarPart(dgVarPart);

			// Memorisation de l'effectif et de la typicite
			dgValue->SetValueFrequency(ivValueFrequencies.GetAt(i));
			dgValue->SetTypicality(dvValueTypicalities.GetAt(i));
		}
	}

	return bOk;
}

boolean CCCoclusteringReport::ReadTypicalities(KWDGAttribute* dgAttribute, int nValueNumber,
					       DoubleVector* dvValueTypicalities)
{
	boolean bOk = true;
	boolean bIsEnd;
	double dValueTypicality;
	ALString sTmp;

	require(dgAttribute != NULL);
	require(nValueNumber >= 0);
	require(dvValueTypicalities != NULL);
	require(dvValueTypicalities->GetSize() == 0);

	// Tableau des typicalites
	bOk = bOk and JSONTokenizer::ReadKeyArray("valueTypicalities");
	bIsEnd = false;
	dValueTypicality = 0;
	while (bOk and not bIsEnd)
	{
		bOk = bOk and JSONTokenizer::ReadDoubleValue(false, dValueTypicality);

		// Tolerance pour les typicalite negatives
		if (dValueTypicality < 0)
		{
			AddWarning(sTmp + "Typicality (" + DoubleToString(dValueTypicality) +
				   ") less than 0 for variable " + dgAttribute->GetAttributeName() +
				   " in \"valueTypicalities\" line " +
				   IntToString(JSONTokenizer::GetCurrentLineIndex()) + " (replaced by 0)");
			dValueTypicality = 0;
		}
		// Erreur pour les typicalite supereures a 1
		else if (dValueTypicality > 1)
		{
			AddError(sTmp + "Typicality (" + DoubleToString(dValueTypicality) +
				 ") greater than 1 for variable " + dgAttribute->GetAttributeName() +
				 " in \"valueTypicalities\" line " + IntToString(JSONTokenizer::GetCurrentLineIndex()));
			bOk = false;
			break;
		}
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
		if (bOk)
			dvValueTypicalities->Add(dValueTypicality);
	}
	if (bOk and nValueNumber != dvValueTypicalities->GetSize())
	{
		JSONTokenizer::AddParseError(
		    "Vector \"valueTypicalities\" should be of same size as vector \"values\"");
		bOk = false;
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadDimensionHierarchies(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	boolean bIsAttributeEnd;
	boolean bIsPartEnd;
	int nAttributeIndex;
	int nPartIndex;
	CCHDGAttribute* dgAttribute;
	ObjectDictionary odPartDictionary;
	KWDGPart* dgPart;
	CCHDGPart* hdgParentPart;
	CCHDGPart* hdgPart;
	ALString sAttributeName;
	ALString sAttributeType;
	ALString sCluster;
	ALString sParentCluster;
	int nFrequency;
	double dInterest;
	double dHierarchicalLevel;
	int nRank;
	int nHierarchicalRank;
	boolean bIsLeaf;
	ALString sKey;
	ALString sShortDescription;
	ALString sDescription;
	POSITION position;
	Object* oElement;
	ALString sTmp;

	require(coclusteringDataGrid != NULL);

	// Initialisations
	nFrequency = 0;
	dInterest = 0;
	dHierarchicalLevel = 0;
	nRank = 0;
	nHierarchicalRank = 0;
	bIsLeaf = 0;

	// Tableau principal
	bOk = bOk and JSONTokenizer::ReadKeyArray("dimensionHierarchies");

	// Lecture des elements du tableau
	nAttributeIndex = 0;
	bIsAttributeEnd = false;
	while (bOk and not bIsAttributeEnd)
	{
		// Recherche de l'attribut courant
		if (nAttributeIndex < coclusteringDataGrid->GetAttributeNumber())
		{
			dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttributeIndex));
			nAttributeIndex++;
		}
		// Erreur si trop de variables
		else
		{
			JSONTokenizer::AddParseError(
			    sTmp + "Too many variables in section \"dimensionHierarchies\" (expected number: " +
			    IntToString(coclusteringDataGrid->GetAttributeNumber()) + ")");
			bOk = false;
			break;
		}

		// Debut de l'objet
		bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

		// Nom de l'attribut
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("name", sAttributeName, bIsAttributeEnd);
		if (bOk and dgAttribute->GetAttributeName() != sAttributeName)
		{
			JSONTokenizer::AddParseError("Read variable \"" + sAttributeName + "\" instead of expected \"" +
						     dgAttribute->GetAttributeName() + "\"");
			bOk = false;
		}

		// Type de l'attribut
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("type", sAttributeType, bIsAttributeEnd);
		if (bOk and KWType::ToString(KWType::GetCoclusteringSimpleType(dgAttribute->GetAttributeType())) !=
				sAttributeType)
		{
			JSONTokenizer::AddParseError(
			    "Read variable type " + sAttributeType + " instead of expected " +
			    KWType::ToString(KWType::GetCoclusteringSimpleType(dgAttribute->GetAttributeType())));
			bOk = false;
		}

		// Initialisation de la hierarchie des parties
		if (bOk)
		{
			// Tableau des parties de la hierarchie
			bOk = bOk and JSONTokenizer::ReadKeyArray("clusters");

			// Initialisation d'un dictionnaire de parties avec la partition de l'attribut
			odPartDictionary.RemoveAll();
			dgPart = dgAttribute->GetHeadPart();
			while (dgPart != NULL)
			{
				hdgPart = cast(CCHDGPart*, dgPart);
				odPartDictionary.SetAt(hdgPart->GetPartName(), hdgPart);
				dgAttribute->GetNextPart(dgPart);
			}

			// Cas d'un attribut numerique
			dgPart = dgAttribute->GetHeadPart();
			bIsPartEnd = false;
			nPartIndex = 0;
			while (bOk and not bIsPartEnd)
			{
				// Verification du nombre de parties dans la hierarchie
				if (nPartIndex >= 2 * dgAttribute->GetPartNumber() - 1)
				{
					JSONTokenizer::AddParseError(
					    "Variable " + sAttributeName + " added part (index " +
					    IntToString(nPartIndex + 1) +
					    ") beyond the expected number of parts in the hierarchy (" +
					    IntToString(2 * dgAttribute->GetPartNumber() - 1) + ")");
					bOk = false;
					break;
				}
				nPartIndex++;

				// Lectures des caracteristiques de la partie
				if (bOk)
				{
					bOk = bOk and JSONTokenizer::ReadExpectedToken('{');
					bOk =
					    bOk and JSONTokenizer::ReadKeyStringValue("cluster", sCluster, bIsPartEnd);
					bOk = bOk and JSONTokenizer::ReadKeyStringValue("parentCluster", sParentCluster,
											bIsPartEnd);
					bOk = bOk and
					      JSONTokenizer::ReadKeyIntValue("frequency", true, nFrequency, bIsPartEnd);
					bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("interest", true, dInterest,
											bIsPartEnd);
					bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("hierarchicalLevel", false,
											dHierarchicalLevel, bIsPartEnd);
					bOk = bOk and JSONTokenizer::ReadKeyIntValue("rank", true, nRank, bIsPartEnd);
					bOk = bOk and JSONTokenizer::ReadKeyIntValue("hierarchicalRank", true,
										     nHierarchicalRank, bIsPartEnd);
					bOk = bOk and JSONTokenizer::ReadKeyBooleanValue("isLeaf", bIsLeaf, bIsPartEnd);

					// Lecture des descriptions provenant potentiellement de l'outil de
					// visualisation
					sShortDescription = "";
					sDescription = "";
					while (bOk and not bIsPartEnd)
					{
						// Lecture d'une cle
						bOk = bOk and JSONTokenizer::ReadStringValue(sKey);

						// Test si cle valide
						if (bOk and sKey != "shortDescription" and sKey != "description")
						{
							JSONTokenizer::AddParseError(
							    "Key should be \"shortDescription\" or \"description\"");
							bOk = false;
						}

						// Lecture de la valeur
						bOk = bOk and JSONTokenizer::ReadExpectedToken(':');
						if (bOk)
						{
							if (sKey == "shortDescription")
								bOk = bOk and
								      JSONTokenizer::ReadStringValue(sShortDescription);
							else if (sKey == "description")
								bOk = bOk and
								      JSONTokenizer::ReadStringValue(sDescription);
						}

						// Lecture de la fin de l'objet
						bOk = bOk and JSONTokenizer::ReadObjectNext(bIsPartEnd);
					}
				}

				// Memorisation des informations sur la partie
				if (bOk)
				{
					// Recherche dans le dictionnaire
					hdgPart = cast(CCHDGPart*, odPartDictionary.Lookup(sCluster));
					if (hdgPart == NULL)
					{
						JSONTokenizer::AddParseError("Part " + sCluster + " of variable " +
									     sAttributeName +
									     " not declared previously");
						bOk = false;
					}

					// Memorisation des caracteristiques de la partie
					if (bOk)
					{
						hdgPart->SetPartFrequency(nFrequency);
						hdgPart->SetInterest(dInterest);
						hdgPart->SetHierarchicalLevel(dHierarchicalLevel);
						hdgPart->SetRank(nRank);
						hdgPart->SetHierarchicalRank(nHierarchicalRank);
						hdgPart->SetShortDescription(sShortDescription);
						hdgPart->SetDescription(sDescription);
					}

					// Gestion de la partie parente
					if (bOk and sParentCluster != "")
					{
						// Creation si necessaire de la partie parente
						hdgParentPart =
						    cast(CCHDGPart*, odPartDictionary.Lookup(sParentCluster));
						if (hdgParentPart == NULL)
						{
							hdgParentPart = dgAttribute->NewHierarchyPart();
							hdgParentPart->SetPartName(sParentCluster);
							odPartDictionary.SetAt(sParentCluster, hdgParentPart);
						}

						// Chainage de la partie dans la partie parente
						if (hdgParentPart->GetChildPart1() == NULL)
							hdgParentPart->SetChildPart1(hdgPart);
						else if (hdgParentPart->GetChildPart2() == NULL)
							hdgParentPart->SetChildPart2(hdgPart);
						else
						{
							AddError(sTmp + "Parent part (" + sParentCluster +
								 ") already have two child parts");
							bOk = false;
						}

						// Fin du chainage si Ok
						if (bOk)
							hdgPart->SetParentPart(hdgParentPart);
					}
					// Gestion de la partie racine
					else
					{
						assert(hdgPart->IsRoot());
						if (dgAttribute->GetRootPart() != NULL)
						{
							AddError(sTmp + "Root part (" + sCluster +
								 ") already defined for variable " +
								 dgAttribute->GetAttributeName());
							bOk = false;
						}
						else
							dgAttribute->SetRootPart(hdgPart);
					}
				}

				// Test si nouvelle partie
				bOk = bOk and JSONTokenizer::ReadArrayNext(bIsPartEnd);
			}

			// Erreur s'il manque des parties dans la hierarchie
			if (bOk and nPartIndex < 2 * dgAttribute->GetPartNumber() - 1)
			{
				JSONTokenizer::AddParseError("Variable " + sAttributeName + " with " +
							     IntToString(nPartIndex) + " specified parts " +
							     ") below the expected number of parts in the hierarchy (" +
							     IntToString(2 * dgAttribute->GetPartNumber() - 1) + ")");
				bOk = false;
				break;
			}

			// Test de specification de la racine
			if (bOk and dgAttribute->GetRootPart() == NULL)
			{
				AddError("Missing root in part hierarchy for variable " +
					 dgAttribute->GetAttributeName());
				bOk = false;
			}

			// Si erreur, nettoyage des parties de hierarchie en cours de construction
			if (not bOk)
			{
				// Nettoyage des chainages vers des parties de la hierarchie
				dgPart = dgAttribute->GetHeadPart();
				while (dgPart != NULL)
				{
					// Lecture des caracteristiques de la partie
					hdgPart = cast(CCHDGPart*, dgPart);
					hdgPart->SetParentPart(NULL);
					assert(hdgPart->IsLeaf());

					// Partie suivante
					dgAttribute->GetNextPart(dgPart);
				}

				// Dereferencement de la partie racine
				dgAttribute->SetRootPart(NULL);

				// Supression des parties non feuilles, memorisee dans le dictionnaires de parties
				position = odPartDictionary.GetStartPosition();
				while (position != NULL)
				{
					odPartDictionary.GetNextAssoc(position, sCluster, oElement);
					hdgPart = cast(CCHDGPart*, oElement);

					// Supression des parties non filles
					if (not hdgPart->IsLeaf())
					{
						odPartDictionary.RemoveKey(sCluster);
						delete hdgPart;
					}
				}
			}
		}

		// Fin de l'objet
		bOk = bOk and JSONTokenizer::ReadExpectedToken('}');

		// Test si nouvel objet
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsAttributeEnd);
	}

	// Erreur si pas assez de variables
	if (bOk and nAttributeIndex <= coclusteringDataGrid->GetAttributeNumber() - 1)
	{
		JSONTokenizer::AddParseError(sTmp +
					     "Too few variables in section \"dimensionPartitions\" (expected number: " +
					     IntToString(coclusteringDataGrid->GetAttributeNumber()) + ")");
		bOk = false;
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadCells(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	boolean bIsEnd;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	KWDGPart* dgPart;
	KWDGCell* dgCell;
	ObjectArray oaAttributePartArrays;
	ObjectArray* oaAttributeParts;
	int nPartIndex;
	ObjectArray oaCellParts;
	ObjectArray oaCells;
	int nCell;
	int nFrequency;
	ALString sTmp;

	require(coclusteringDataGrid != NULL);

	// Initialisation des tableau de parties par partition d'attribut
	oaAttributePartArrays.SetSize(coclusteringDataGrid->GetAttributeNumber());
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = coclusteringDataGrid->GetAttributeAt(nAttribute);

		// Creation d'un tableau de partie
		oaAttributeParts = new ObjectArray;
		oaAttributePartArrays.SetAt(nAttribute, oaAttributeParts);
		dgPart = dgAttribute->GetHeadPart();
		while (dgPart != NULL)
		{
			oaAttributeParts->Add(dgPart);
			dgAttribute->GetNextPart(dgPart);
		}
	}

	// Passage en mode mise a jour des cellules
	coclusteringDataGrid->SetCellUpdateMode(true);
	oaCellParts.SetSize(coclusteringDataGrid->GetAttributeNumber());

	// Lecture du tableau de cellules
	bIsEnd = false;
	bOk = bOk and JSONTokenizer::ReadKeyArray("cellPartIndexes");
	nPartIndex = 0;
	while (bOk and not bIsEnd)
	{
		// Lecture du debut du vecteur d'index
		bOk = bOk and JSONTokenizer::ReadExpectedToken('[');

		// Recherche des parties des attributs
		for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
		{
			// Index de la partie
			bOk = bOk and JSONTokenizer::ReadIntValue(true, nPartIndex);

			// Verification de l'index de la partie
			oaAttributeParts = cast(ObjectArray*, oaAttributePartArrays.GetAt(nAttribute));
			if (bOk)
			{
				// Erreur si index trop grand
				assert(nPartIndex >= 0);
				if (nPartIndex >= oaAttributeParts->GetSize())
				{
					JSONTokenizer::AddParseError(
					    sTmp + "Cell with part index (" + IntToString(nPartIndex) +
					    ") greater than the number of parts (" +
					    IntToString(oaAttributeParts->GetSize()) + ") of variable " +
					    coclusteringDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());
					bOk = false;
				}
			}

			// Memorisation de la partie
			if (bOk)
				oaCellParts.SetAt(nAttribute, oaAttributeParts->GetAt(nPartIndex));

			// Lecture de la suite du tableau d'index
			if (bOk)
			{
				if (nAttribute == coclusteringDataGrid->GetAttributeNumber() - 1)
					bOk = bOk and JSONTokenizer::ReadExpectedToken(']');
				else
					bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
			}
		}

		// Test si cellule existe deja
		if (bOk and coclusteringDataGrid->LookupCell(&oaCellParts) != NULL)
		{
			JSONTokenizer::AddParseError(sTmp + "Cell already specified previously");
			bOk = false;
		}

		// Ajout de la cellule
		if (bOk)
		{
			dgCell = coclusteringDataGrid->AddCell(&oaCellParts);
			oaCells.Add(dgCell);
		}

		// Cellule suivante
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);

		// Arret si erreur
		if (not bOk)
			break;
	}

	// Verification du nombre de cellules
	if (bOk and coclusteringDataGrid->GetCellNumber() != nHeaderCellNumber)
	{
		JSONTokenizer::AddParseError(sTmp + "Number of cells read in section \"cellPartIndexes\" (" +
					     IntToString(coclusteringDataGrid->GetCellNumber()) +
					     ") different from that in section \"summary\" (" +
					     IntToString(nHeaderCellNumber) + ")");
		bOk = false;
	}

	// Lecture du tableau des effectifs de cellules
	bIsEnd = false;
	bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
	bOk = bOk and JSONTokenizer::ReadKeyArray("cellFrequencies");
	nCell = 0;
	nFrequency = 0;
	while (bOk and not bIsEnd)
	{
		// Effectif de la cellule
		bOk = bOk and JSONTokenizer::ReadIntValue(true, nFrequency);

		// Recherche de la cellule
		if (bOk)
		{
			dgCell = cast(KWDGCell*, oaCells.GetAt(nCell));
			dgCell->SetCellFrequency(nFrequency);
		}

		// Cellule suivante
		nCell++;
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);

		// Test de conformite avec le nombre de cellules
		if (bOk)
		{
			if (nCell == nHeaderCellNumber and not bIsEnd)
			{
				JSONTokenizer::AddParseError(
				    sTmp + "Number of cells frequencies read in section \"cellFrequencies\"" +
				    " is larger than that in section \"summary\" (" + IntToString(nHeaderCellNumber) +
				    ")");
				bOk = false;
			}
			else if (nCell < nHeaderCellNumber and bIsEnd)
			{
				JSONTokenizer::AddParseError(
				    sTmp + "Number of cells frequencies read in section \"cellFrequencies\" (" +
				    IntToString(nCell) + ") is smaller than that in section \"summary\" (" +
				    IntToString(nHeaderCellNumber) + ")");
				bOk = false;
			}
		}

		// Arret si erreur
		if (not bOk)
			break;
	}

	// Fin des mises a jour de cellules
	coclusteringDataGrid->SetCellUpdateMode(false);

	// Nettoyage
	oaAttributePartArrays.DeleteAll();
	return bOk;
}

void CCCoclusteringReport::InternalWriteReport(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON)
{
	require(coclusteringDataGrid != NULL);
	require(fReport == NULL);
	require(fJSON != NULL);
	require(fJSON->IsOpened());

	// Outil et version
	fJSON->WriteKeyString("tool", GetLearningApplicationName() + " Coclustering");
	fJSON->WriteKeyString("version", GetLearningVersion());

	// Description courte
	fJSON->WriteKeyString("shortDescription", coclusteringDataGrid->GetShortDescription());

	// Liste des messages d'erreur potentiellement detectees pendant l'analyse
	KWLearningErrorManager::WriteJSONKeyReport(fJSON);

	// Ecriture de chaque section du rapport JSON de coclustering
	fJSON->BeginKeyObject("coclusteringReport");
	WriteSummary(coclusteringDataGrid, fJSON);
	WriteDimensionSummaries(coclusteringDataGrid, fJSON);
	WriteDimensionPartitions(coclusteringDataGrid, fJSON);
	WriteDimensionHierarchies(coclusteringDataGrid, fJSON);
	WriteCells(coclusteringDataGrid, fJSON);
	fJSON->EndObject();
}

void CCCoclusteringReport::WriteSummary(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON)
{
	require(coclusteringDataGrid != NULL);
	require(fJSON != NULL);

	// On passe en mode camel case, pour reutiliser les memes identifieurs que dans les rapports de coclustering
	fJSON->SetCamelCaseKeys(true);

	// Statistiques (cf. WriteCoclusteringStats)
	fJSON->BeginKeyObject("summary");
	fJSON->WriteKeyInt(sKeyWordInstances, coclusteringDataGrid->GetGridFrequency());
	fJSON->WriteKeyInt(sKeyWordCells, coclusteringDataGrid->GetCellNumber());
	fJSON->WriteKeyContinuous(sKeyWordNullCost, coclusteringDataGrid->GetNullCost());
	fJSON->WriteKeyContinuous(sKeyWordCost, coclusteringDataGrid->GetCost());
	fJSON->WriteKeyContinuous(sKeyWordLevel, coclusteringDataGrid->GetLevel());
	fJSON->WriteKeyInt(sKeyWordInitialDimensions, coclusteringDataGrid->GetInitialAttributeNumber());
	fJSON->WriteKeyString(sKeyWordFrequencyAttribute, coclusteringDataGrid->GetFrequencyAttributeName());
	fJSON->WriteKeyString(sKeyWordDictionary, coclusteringDataGrid->GetConstDatabaseSpec()->GetClassName());
	fJSON->WriteKeyString(sKeyWordDatabase, coclusteringDataGrid->GetConstDatabaseSpec()->GetDatabaseName());

	// Specification completes de la base
	fJSON->WriteKeyDouble(sKeyWordSamplePercentage,
			      coclusteringDataGrid->GetConstDatabaseSpec()->GetSampleNumberPercentage());
	fJSON->WriteKeyString(sKeyWordSamplingMode, coclusteringDataGrid->GetConstDatabaseSpec()->GetSamplingMode());
	fJSON->WriteKeyString(sKeyWordSelectionVariable,
			      coclusteringDataGrid->GetConstDatabaseSpec()->GetSelectionAttribute());
	fJSON->WriteKeyString(sKeyWordSelectionValue,
			      coclusteringDataGrid->GetConstDatabaseSpec()->GetSelectionValue());
	fJSON->EndObject();

	// On restitue le mode par defaut
	fJSON->SetCamelCaseKeys(false);
}

void CCCoclusteringReport::WriteDimensionSummaries(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON)
{
	int nAttribute;
	CCHDGAttribute* dgAttribute;

	require(coclusteringDataGrid != NULL);
	require(fJSON != NULL);

	// Parcours des attributs pour la section des dimensions
	fJSON->BeginKeyArray("dimensionSummaries");
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		WriteDimensionSummary(dgAttribute, fJSON);
	}
	fJSON->EndArray();
}

void CCCoclusteringReport::WriteDimensionSummary(CCHDGAttribute* attribute, JSONFile* fJSON)
{
	int nValueNumber;

	// Nombre initial de valeurs
	nValueNumber = attribute->GetInitialValueNumber();

	// Caracteristique des attributs
	// Le nombre de valeur est diminuer en externe de 1 pour tenir compte de de la StarValue en interne
	fJSON->BeginObject();
	fJSON->WriteKeyString("name", attribute->GetAttributeName());
	if (attribute->GetAttributeType() == KWType::VarPart)
		fJSON->WriteKeyBoolean("isVarPart", true);
	fJSON->WriteKeyString("type",
			      KWType::ToString(KWType::GetCoclusteringSimpleType(attribute->GetAttributeType())));
	fJSON->WriteKeyInt("parts", attribute->GetPartNumber());
	fJSON->WriteKeyInt("initialParts", attribute->GetInitialPartNumber());
	fJSON->WriteKeyInt("values", nValueNumber);
	fJSON->WriteKeyDouble("interest", attribute->GetInterest());
	fJSON->WriteKeyString("description", attribute->GetDescription());
	if (KWFrequencyTable::GetWriteGranularityAndGarbage())
		fJSON->WriteKeyBoolean("garbage", (attribute->GetGarbageModalityNumber() > 0));
	if (attribute->GetAttributeType() == KWType::Continuous)
	{
		fJSON->WriteKeyContinuous("min", attribute->GetMin());
		fJSON->WriteKeyContinuous("max", attribute->GetMax());
	}
	fJSON->EndObject();
}

void CCCoclusteringReport::WriteDimensionPartitions(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON)
{
	int nAttribute;
	KWDGAttribute* attribute;

	require(coclusteringDataGrid != NULL);
	require(fJSON != NULL);

	// Parcours des attributs
	fJSON->BeginKeyArray("dimensionPartitions");
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = coclusteringDataGrid->GetAttributeAt(nAttribute);

		// Ecriture de la partition de l'attribut
		// Dans le cas d'un attribut de type VarPart, declenche l'ecriture de ses innerAttributes
		WriteAttributePartition(attribute, fJSON);
	}
	fJSON->EndArray();
}

void CCCoclusteringReport::WriteAttributePartition(KWDGAttribute* attribute, JSONFile* fJSON)
{
	CCHDGAttribute* dgAttribute;
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	KWDGInterval* dgInterval;
	KWDGValue* dgValue;
	KWDGValueSet* dgValueSet;
	int nIndex;
	int nDefaultGroupIndex;

	require(attribute != NULL);

	// Debut de l'objet
	fJSON->BeginObject();
	fJSON->WriteKeyString("name", attribute->GetAttributeName());
	fJSON->WriteKeyString("type",
			      KWType::ToString(KWType::GetCoclusteringSimpleType(attribute->GetAttributeType())));

	// Traitement des attributs numeriques
	if (attribute->GetAttributeType() == KWType::Continuous)
	{
		dgAttribute = cast(CCHDGAttribute*, attribute);

		// Parcours des parties
		fJSON->BeginKeyArray("intervals");
		nIndex = 0;
		dgPart = attribute->GetHeadPart();
		while (dgPart != NULL)
		{
			hdgPart = cast(CCHDGPart*, dgPart);
			dgInterval = dgPart->GetInterval();

			// Ecritures des bornes
			fJSON->BeginObject();
			fJSON->WriteKeyString("cluster", hdgPart->GetPartName());
			fJSON->BeginKeyList("bounds");
			if (dgInterval->GetUpperBound() != KWContinuous::GetMissingValue())
			{
				// Borne inf, ou min pour le premier intervalle non Missing
				if (nIndex == 0)
					fJSON->WriteContinuous(dgAttribute->GetMin());
				else
					fJSON->WriteContinuous(dgInterval->GetLowerBound());

				// Borne sup, ou max pour le premier intervalle non Missing
				if (dgPart == attribute->GetTailPart())
					fJSON->WriteContinuous(dgAttribute->GetMax());
				else
					fJSON->WriteContinuous(dgInterval->GetUpperBound());

				// Index suivant d'intervalle non missing
				nIndex++;
			}
			fJSON->EndList();
			fJSON->EndObject();

			// Partie suivante
			attribute->GetNextPart(dgPart);
		}
		fJSON->EndArray();
	}
	// Traitement des attributs groupables
	else
	{
		assert(KWType::IsCoclusteringGroupableType(attribute->GetAttributeType()));

		// Ecriture des attributs internes dans le cas d'un attribut VarPart
		if (attribute->GetAttributeType() == KWType::VarPart)
			WriteInnerAttributes(attribute->GetInnerAttributes(), fJSON);

		// Parcours des groupes de parties de variable
		fJSON->BeginKeyArray("valueGroups");
		nDefaultGroupIndex = -1;
		nIndex = 0;
		dgPart = attribute->GetHeadPart();
		while (dgPart != NULL)
		{
			dgValueSet = dgPart->GetValueSet();
			hdgPart = cast(CCHDGPart*, dgPart);

			// Memorisation de l'index du groupe par defaut
			if (dgValueSet->IsDefaultPart())
				nDefaultGroupIndex = nIndex;
			nIndex++;

			// Parcours des valeurs, sauf si effectif nul (cas de la valeur par defaut)
			fJSON->BeginObject();
			fJSON->WriteKeyString("cluster", hdgPart->GetPartName());
			fJSON->BeginKeyList("values");
			dgValue = dgValueSet->GetHeadValue();
			while (dgValue != NULL)
			{
				if (dgValue->GetValueFrequency() > 0)
					fJSON->WriteString(dgValue->GetObjectLabel());
				dgValueSet->GetNextValue(dgValue);
			}
			fJSON->EndList();

			// Effectifs des valeurs
			fJSON->BeginKeyList("valueFrequencies");
			dgValue = dgValueSet->GetHeadValue();
			while (dgValue != NULL)
			{
				if (dgValue->GetValueFrequency() > 0)
					fJSON->WriteInt(dgValue->GetValueFrequency());
				dgValueSet->GetNextValue(dgValue);
			}
			fJSON->EndList();

			// Typicalite des valeurs, sauf pour un attribut interne
			if (not attribute->IsInnerAttribute())
			{
				fJSON->BeginKeyList("valueTypicalities");
				dgValue = dgValueSet->GetHeadValue();
				while (dgValue != NULL)
				{
					if (dgValue->GetValueFrequency() > 0)
						fJSON->WriteDouble(dgValue->GetTypicality());
					dgValueSet->GetNextValue(dgValue);
				}
				fJSON->EndList();
			}

			// Fin de l'objet
			fJSON->EndObject();

			// Partie suivante
			attribute->GetNextPart(dgPart);
		}
		fJSON->EndArray();

		// Index du groupe par defaut
		assert(nDefaultGroupIndex >= 0 or attribute->GetAttributeType() != KWType::Symbol);
		if (nDefaultGroupIndex >= 0)
			fJSON->WriteKeyInt("defaultGroupIndex", nDefaultGroupIndex);
		else
		{
			// Dans le cas VarPart, on ecrit defaultGroupIndex meme s'il est inutile dans ce cas
			// Cela permet d'avoir une structure de rapport generique
			assert(attribute->GetAttributeType() == KWType::VarPart);
			fJSON->WriteKeyInt("defaultGroupIndex", 0);
		}
	}

	// Fin de l'objet
	fJSON->EndObject();
}

void CCCoclusteringReport::WriteInnerAttributes(const KWDGInnerAttributes* innerAttributes, JSONFile* fJSON)
{
	CCHDGAttribute* innerAttribute;
	int nAttribute;

	require(innerAttributes != NULL);
	require(fJSON != NULL);

	// Section des attributs internes
	fJSON->BeginKeyObject("innerVariables");

	// Parcours des innerAttributes pour la section des dimensions
	fJSON->BeginKeyArray("dimensionSummaries");
	for (nAttribute = 0; nAttribute < innerAttributes->GetInnerAttributeNumber(); nAttribute++)
	{
		innerAttribute = cast(CCHDGAttribute*, innerAttributes->GetInnerAttributeAt(nAttribute));
		WriteDimensionSummary(innerAttribute, fJSON);
	}
	fJSON->EndArray();

	// Parcours des innerAttributes pour la section des partitions
	fJSON->BeginKeyArray("dimensionPartitions");
	for (nAttribute = 0; nAttribute < innerAttributes->GetInnerAttributeNumber(); nAttribute++)
	{
		innerAttribute = cast(CCHDGAttribute*, innerAttributes->GetInnerAttributeAt(nAttribute));
		WriteAttributePartition(innerAttribute, fJSON);
	}
	fJSON->EndArray();

	// Fin de la section
	fJSON->EndObject();
}

void CCCoclusteringReport::WriteDimensionHierarchies(const CCHierarchicalDataGrid* coclusteringDataGrid,
						     JSONFile* fJSON)
{
	CCHDGAttribute* dgAttribute;
	int nAttribute;
	CCHDGPart* hdgPart;
	ObjectArray oaParts;
	int nPart;

	require(coclusteringDataGrid != NULL);
	require(fJSON != NULL);

	// Parcours des attributs
	fJSON->BeginKeyArray("dimensionHierarchies");
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Debut de l'objet
		fJSON->BeginObject();
		fJSON->WriteKeyString("name", dgAttribute->GetAttributeName());
		fJSON->WriteKeyString(
		    "type", KWType::ToString(KWType::GetCoclusteringSimpleType(dgAttribute->GetAttributeType())));

		// Exports de toutes les parties de la hierarchie en partant de la racine
		oaParts.SetSize(0);
		dgAttribute->ExportHierarchyParts(&oaParts);

		// Tri pour respecter l'ordre d'affichage
		oaParts.SetCompareFunction(CCHDGPartCompareLeafRank);
		oaParts.Sort();
		assert(oaParts.GetSize() > 0);

		// Affichage des parties
		fJSON->BeginKeyArray("clusters");
		for (nPart = 0; nPart < oaParts.GetSize(); nPart++)
		{
			hdgPart = cast(CCHDGPart*, oaParts.GetAt(nPart));

			// Caracteristiques de la partie
			fJSON->BeginObject();
			fJSON->WriteKeyString("cluster", hdgPart->GetPartName());
			fJSON->WriteKeyString("parentCluster", hdgPart->GetParentPartName());
			fJSON->WriteKeyInt("frequency", hdgPart->GetPartFrequency());
			fJSON->WriteKeyDouble("interest", hdgPart->GetInterest());
			fJSON->WriteKeyDouble("hierarchicalLevel", hdgPart->GetHierarchicalLevel());
			fJSON->WriteKeyInt("rank", hdgPart->GetRank());
			fJSON->WriteKeyInt("hierarchicalRank", hdgPart->GetHierarchicalRank());
			if (KWFrequencyTable::GetWriteGranularityAndGarbage())
				fJSON->WriteKeyBoolean("garbage", ((hdgPart == dgAttribute->GetGarbagePart())));
			fJSON->WriteKeyBoolean("isLeaf", hdgPart->IsLeaf());
			if (hdgPart->GetShortDescription() != "")
				fJSON->WriteKeyString("shortDescription", hdgPart->GetShortDescription());
			if (hdgPart->GetDescription() != "")
				fJSON->WriteKeyString("description", hdgPart->GetDescription());
			fJSON->EndObject();
		}
		fJSON->EndArray();

		// Fin de l'objet
		fJSON->EndObject();
	}
	fJSON->EndArray();
}

void CCCoclusteringReport::WriteCells(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON)
{
	int nAttribute;
	KWDGAttribute* dgAttribute;
	KWDGPart* dgPart;
	ObjectArray oaAttributePartIndexes;
	NumericKeyDictionary* nkdPartIndexes;
	KWSortableIndex* siPartIndex;
	int nIndex;
	ObjectArray oaCells;
	KWDGCell* cell;
	int nCell;

	require(coclusteringDataGrid != NULL);
	require(fJSON != NULL);

	// Tri des cellules par valeurs des parties d'attribut (et non par adresse)
	// en les rentrant prealablement dans un tableau
	oaCells.SetSize(coclusteringDataGrid->GetCellNumber());
	cell = coclusteringDataGrid->GetHeadCell();
	nCell = 0;
	while (cell != NULL)
	{
		oaCells.SetAt(nCell, cell);
		coclusteringDataGrid->GetNextCell(cell);
		nCell++;
	}
	oaCells.SetCompareFunction(KWDGCellCompareDecreasingFrequency);
	oaCells.Sort();

	// Creation des dictionnaires d'index de parties, pour chaque attribut
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Creation d'un dictionnaire d'index de parties
		nkdPartIndexes = new NumericKeyDictionary;
		oaAttributePartIndexes.Add(nkdPartIndexes);

		// Parcours des partie pour memoriser leur index
		nIndex = 0;
		dgPart = dgAttribute->GetHeadPart();
		while (dgPart != NULL)
		{
			// Creation d'un index de partie
			siPartIndex = new KWSortableIndex;
			siPartIndex->SetIndex(nIndex);
			nIndex++;

			// Memorisation de l'index associe a la partoe
			nkdPartIndexes->SetAt(dgPart, siPartIndex);

			// Partie suivante
			dgAttribute->GetNextPart(dgPart);
		}
	}

	// Index des paties des cellules
	fJSON->BeginKeyArray("cellPartIndexes");
	for (nCell = 0; nCell < oaCells.GetSize(); nCell++)
	{
		cell = cast(KWDGCell*, oaCells.GetAt(nCell));

		// On ignore les cellule d'effectif null (en principe, il n'y en a pas)
		if (cell->GetCellFrequency() > 0)
		{
			// Affichage des identifiants des parties de la cellule
			fJSON->BeginList();
			for (nAttribute = 0; nAttribute < cell->GetAttributeNumber(); nAttribute++)
			{
				dgPart = cell->GetPartAt(nAttribute);

				// Recherche de l'index associe a la partie
				nkdPartIndexes = cast(NumericKeyDictionary*, oaAttributePartIndexes.GetAt(nAttribute));
				siPartIndex = cast(KWSortableIndex*, nkdPartIndexes->Lookup(dgPart));

				// Ecriture de l'index
				fJSON->WriteInt(siPartIndex->GetIndex());
			}
			fJSON->EndList();
		}
	}
	fJSON->EndArray();

	// Effectif des cellules
	fJSON->BeginKeyList("cellFrequencies");
	for (nCell = 0; nCell < oaCells.GetSize(); nCell++)
	{
		cell = cast(KWDGCell*, oaCells.GetAt(nCell));

		// On ignore les cellule d'effectif null (en principe, il n'y en a pas)
		if (cell->GetCellFrequency() > 0)
		{
			// Effectif de la cellule
			fJSON->WriteInt(cell->GetCellFrequency());
		}
	}
	fJSON->EndList();

	// Nettoyage des dictionnaires d'index de parties
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Nettoyage du dictionnaire d'index de parties
		nkdPartIndexes = cast(NumericKeyDictionary*, oaAttributePartIndexes.GetAt(nAttribute));
		nkdPartIndexes->DeleteAll();
		delete nkdPartIndexes;
	}
}
