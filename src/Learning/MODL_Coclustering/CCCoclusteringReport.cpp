// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCCoclusteringReport.h"

const ALString CCCoclusteringReport::sKeyWordKhiops = "#Khiops";
const ALString CCCoclusteringReport::sKeyWordShortDescription = "Short description";
const ALString CCCoclusteringReport::sKeyWordDimensions = "Dimensions";
const ALString CCCoclusteringReport::sKeyWordCoclusteringStats = "Coclustering stats";
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
const ALString CCCoclusteringReport::sKeyWordBounds = "Bounds";
const ALString CCCoclusteringReport::sKeyWordHierarchy = "Hierarchy";
const ALString CCCoclusteringReport::sKeyWordComposition = "Composition";
const ALString CCCoclusteringReport::sKeyWordAnnotation = "Annotation";
const ALString CCCoclusteringReport::sKeyWordTrue = "TRUE";
const ALString CCCoclusteringReport::sKeyWordFalse = "FALSE";

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

boolean CCCoclusteringReport::ReadGenericReport(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk;
	int nFileFormat;
	ALString sKhiopsEncoding;
	boolean bForceAnsi;

	require(sFileName != "");
	require(coclusteringDataGrid != NULL);

	// On determine le format du fichier
	nFileFormat = DetectFileFormatAndEncoding(sFileName, sKhiopsEncoding);
	bOk = nFileFormat != None;

	// Lecture selon le format du fichier
	if (nFileFormat == KHC)
		bOk = ReadReport(sFileName, coclusteringDataGrid);
	else if (nFileFormat == JSON)
	{
		// Analyse du type d'encodage pour determiner si on doit recoder les caracteres utf8 du fichier json en
		// ansi
		if (sKhiopsEncoding == "")
		{
			bForceAnsi = false;
			AddWarning("The \"khiops_encoding\" field is missing in the read coclustering file. "
				   "The coclustering file is deprecated, and may raise encoding problems "
				   "in case of mixed ansi and utf8 chars "
				   ": see the Khiops guide for more information.");
		}
		else if (sKhiopsEncoding == "ascii" or sKhiopsEncoding == "utf8")
			bForceAnsi = false;
		else if (sKhiopsEncoding == "ansi" or sKhiopsEncoding == "mixed_ansi_utf8")
			bForceAnsi = true;
		else if (sKhiopsEncoding == "colliding_ansi_utf8")
		{
			bForceAnsi = false;
			AddWarning("The \"khiops_encoding\" field is \"" + sKhiopsEncoding +
				   "\" in the read coclustering file. "
				   "This may raise encoding problems if the file has been modified outside of Khiops "
				   ": see the Khiops guide for more information.");
		}
		else
		{
			bForceAnsi = false;
			AddWarning(
			    "The value of the \"khiops_encoding\" field is \"" + sKhiopsEncoding +
			    "\" in the read coclustering file. "
			    "This encoding type is unknown and will be ignored, which may raise encoding problems "
			    "in case of mixed ansi and utf8 chars "
			    ": see the Khiops guide for more information.");
		}

		// Lecture du fichier en parametrant le json tokeniser correctement
		JSONTokenizer::SetForceAnsi(bForceAnsi);
		bOk = ReadJSONReport(sFileName, coclusteringDataGrid);
		JSONTokenizer::SetForceAnsi(false);
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadGenericReportHeader(const ALString& sFileName,
						      CCHierarchicalDataGrid* coclusteringDataGrid,
						      int& nInstanceNumber, int& nCellNumber)
{
	require(sFileName != "");
	require(coclusteringDataGrid != NULL);

	boolean bOk;
	int nFileFormat;
	ALString sKhiopsEncoding;
	boolean bForceAnsi;

	require(sFileName != "");
	require(coclusteringDataGrid != NULL);

	// On determine le format du fichier
	nFileFormat = DetectFileFormatAndEncoding(sFileName, sKhiopsEncoding);
	bOk = nFileFormat != None;

	// Lecture selon le format du fichier
	nInstanceNumber = 0;
	nCellNumber = 0;
	if (nFileFormat == KHC)
		bOk = ReadReportHeader(sFileName, coclusteringDataGrid, nInstanceNumber, nCellNumber);
	else if (nFileFormat == JSON)
	{
		// Analyse du type d'encodage pour determiner si on doit recoder les caracteres utf8 du fichier json en
		// ansi Pas de message pour cette lecture rapide
		bForceAnsi = (sKhiopsEncoding == "ansi" or sKhiopsEncoding == "mixed_ansi_utf8");

		// Lecture de l'entete du fichier en parametrant le json tokeniser correctement
		JSONTokenizer::SetForceAnsi(bForceAnsi);
		bOk = ReadJSONReportHeader(sFileName, coclusteringDataGrid, nInstanceNumber, nCellNumber);
		JSONTokenizer::SetForceAnsi(false);
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadReport(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;

	require(coclusteringDataGrid != NULL);
	require(fReport == NULL);

	// Ouverture du rapport pour initialiser l'API de lecture
	bOk = OpenInputCoclusteringReportFile(sFileName);
	if (bOk)
	{
		// Parsing
		bOk = InternalReadReport(coclusteringDataGrid, false);

		// Fermeture du fichier
		CloseCoclusteringReportFile();
	}
	ensure(fReport == NULL);
	return bOk;
}

boolean CCCoclusteringReport::ReadReportHeader(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid,
					       int& nInstanceNumber, int& nCellNumber)
{
	boolean bOk = true;

	require(coclusteringDataGrid != NULL);
	require(fReport == NULL);

	// Ouverture du rapport pour initialiser l'API de lecture
	bOk = OpenInputCoclusteringReportFile(sFileName);
	if (bOk)
	{
		// Parsing
		bOk = InternalReadReport(coclusteringDataGrid, true);
		nInstanceNumber = nHeaderInstanceNumber;
		nCellNumber = nHeaderCellNumber;

		// Fermeture du fichier
		CloseCoclusteringReportFile();
	}
	ensure(fReport == NULL);
	return bOk;
}

boolean CCCoclusteringReport::WriteReport(const ALString& sFileName, const CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	fstream fstReport;
	ALString sLocalTempFileName;

	require(coclusteringDataGrid != NULL);

	// Preparation de la copie sur HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sFileName, sLocalTempFileName);

	// Ouverture du fichier de rapport en ecriture
	if (bOk)
		bOk = FileService::OpenOutputFile(sLocalTempFileName, fstReport);
	if (bOk)
	{
		InternalWriteReport(coclusteringDataGrid, fstReport);

		// Ecriture du rapport
		bOk = FileService::CloseOutputFile(sLocalTempFileName, fstReport);

		// Destruction du fichier si erreur
		if (not bOk)
			FileService::RemoveFile(sLocalTempFileName);
	}
	if (bOk)
	{
		// Copie vers HDFS si necessaire
		PLRemoteFileService::CleanOutputWorkingFile(sFileName, sLocalTempFileName);
	}
	return bOk;
}

const ALString CCCoclusteringReport::GetJSONReportSuffix()
{
	return "khcj";
}

const ALString CCCoclusteringReport::GetKhcReportSuffix()
{
	return "khc";
}

boolean CCCoclusteringReport::ReadJSONReport(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;

	require(coclusteringDataGrid != NULL);
	require(not JSONTokenizer::IsOpened());

	// Initialisation du tokenizer pour analiser le rapport
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;
	sReportFileName = sFileName;
	bOk = JSONTokenizer::OpenForRead(GetClassLabel(), sFileName);
	if (bOk)
	{
		// Parsing
		bOk = InternalReadJSONReport(coclusteringDataGrid, false);

		// Fermeture du tokenizer
		JSONTokenizer::Close();
	}
	sReportFileName = "";
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;
	return bOk;
}

boolean CCCoclusteringReport::ReadJSONReportHeader(const ALString& sFileName,
						   CCHierarchicalDataGrid* coclusteringDataGrid, int& nInstanceNumber,
						   int& nCellNumber)
{
	boolean bOk = true;

	require(coclusteringDataGrid != NULL);
	require(not JSONTokenizer::IsOpened());

	// Initialisation du tokenizer pour analiser le rapport
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;
	sReportFileName = sFileName;
	bOk = JSONTokenizer::OpenForRead(GetClassLabel(), sFileName);
	if (bOk)
	{
		// Parsing
		bOk = InternalReadJSONReport(coclusteringDataGrid, true);
		nInstanceNumber = nHeaderInstanceNumber;
		nCellNumber = nHeaderCellNumber;

		// Fermeture du tokenizer
		JSONTokenizer::Close();
	}
	sReportFileName = "";
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;
	return bOk;
}

boolean CCCoclusteringReport::WriteJSONReport(const ALString& sJSONReportName,
					      const CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	JSONFile fJSON;
	ALString sLocalTempFileName;

	require(coclusteringDataGrid != NULL);

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
		InternalWriteJSONReport(coclusteringDataGrid, &fJSON);

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
			if (JSONTokenizer::GetTokenStringValue() == "#")
				nFileFormat = KHC;
		}

		// Message d'erreur si pas de format reconnu
		if (nFileFormat == None)
			AddError("Format of file " + sFileName + " should either " +
				 CCCoclusteringReport::GetJSONReportSuffix() + " or " +
				 CCCoclusteringReport::GetKhcReportSuffix());

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
	ObjectArray oaAttributesPartDictionaries;
	ALString sTmp;

	require(coclusteringDataGrid != NULL);
	require(fReport != NULL);

	// Affichage des informations en mode debug
	bReadDebug = false;

	// Gestion des erreurs
	Global::ActivateErrorFlowControl();

	// Reinitialisation prealable des informations
	coclusteringDataGrid->DeleteAll();

	// Premiere ligne: #Khiops...
	if (bOk)
		bOk = ReadVersion(coclusteringDataGrid);

	// Section Dimensions avec la description courte
	if (bOk)
		bOk = ReadDimensions(coclusteringDataGrid);

	// Section des statistiques de coclustering
	if (bOk)
		bOk = ReadCoclusteringStats(coclusteringDataGrid);

	// Lecture detailles
	if (not bHeaderOnly)
	{
		// Section des bornes des intervalles numeriques
		if (bOk)
			bOk = ReadBounds(coclusteringDataGrid);

		// Section des hierarchie de parties
		if (bOk)
			bOk = ReadHierarchy(coclusteringDataGrid, &oaAttributesPartDictionaries);

		// Section de specification de la composition des attributs categoriels
		if (bOk)
			bOk = ReadComposition(coclusteringDataGrid, &oaAttributesPartDictionaries);

		// Section de specification des cellules de la grille
		if (bOk)
			bOk = ReadCells(coclusteringDataGrid, &oaAttributesPartDictionaries);

		// Section de specification des annotations
		if (bOk)
			bOk = ReadAnnotation(coclusteringDataGrid, &oaAttributesPartDictionaries);

		// Ultime verification dans le cas d'une lecture complete du rapport
		if (bOk)
		{
			// Verification de l'integrite globale
			// Verification uniquement par assertion: cela devrait suffire compte-tenu des controles
			// precedents La verification exhaustive n'est pas envisageable en raison de son cout
			// algorithmique
			assert(coclusteringDataGrid->Check());

			// Verification de l'integrite de la hierarchie
			if (bOk)
				bOk = coclusteringDataGrid->CheckHierarchy();

			// On reinitialise le numero de ligne pour "nettoyer" les eventuels messages d'erreur suivants
			nLineIndex = 0;

			// Verification de la coherence des informations redondantes d'entete
			if (bOk)
			{
				if (nHeaderInstanceNumber != coclusteringDataGrid->GetGridFrequency())
				{
					bOk = false;
					AddError(sTmp + "Instance number in the report header (" +
						 IntToString(nHeaderInstanceNumber) +
						 ") inconsistent with that of the whole data grid (" +
						 IntToString(coclusteringDataGrid->GetGridFrequency()) + ")");
				}
				if (nHeaderCellNumber != coclusteringDataGrid->GetCellNumber())
				{
					bOk = false;
					AddError(sTmp + "Cell number in the report header (" +
						 IntToString(nHeaderCellNumber) +
						 ") inconsistent with that of the whole data grid (" +
						 IntToString(coclusteringDataGrid->GetCellNumber()) + ")");
				}
			}

			// Message d'erreur synthetique
			if (not bOk)
				AddError("Invalid coclustering specification");
		}

		// Nettoyage
		oaAttributesPartDictionaries.DeleteAll();
	}

	// Reinitialisation des informations si echec
	if (not bOk)
	{
		nLineIndex = 0;
		AddError("Abort read");
		coclusteringDataGrid->DeleteAll();
		nHeaderInstanceNumber = 0;
		nHeaderCellNumber = 0;
	}

	// Gestion des erreurs
	Global::DesactivateErrorFlowControl();

	return bOk;
}

boolean CCCoclusteringReport::ReadVersion(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	char* sField;
	ALString sTmp;

	require(coclusteringDataGrid != NULL);

	// Premiere ligne: #Khiops...
	sField = ReadNextField();
	if (strncmp(sKeyWordKhiops, sField, sKeyWordKhiops.GetLength()) != 0)
	{
		AddError(sTmp + "Key word " + sKeyWordKhiops + " expected but not found");
		bOk = false;
	}

	// Passage a la ligne suivante
	SkipLine();
	return bOk;
}

boolean CCCoclusteringReport::ReadDimensions(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	char* sField;
	ALString sShortDescription;
	CCHDGAttribute* dgAttribute;
	int nAttributeNumber;
	int nAttribute;
	ALString sAttributeName;
	int nAttributeType;
	int nAttributePartNumber;
	int nAttributeInitialPartNumber;
	int nAttributeValueNumber;
	double dAttributeInterest;
	ALString sAttributeDescription;
	int nPart;
	ALString sTmp;

	require(coclusteringDataGrid != NULL);

	// Lecture du premier champ, qui peut differer selon la version
	sField = ReadNextField();

	// Description courte
	if (bOk)
	{
		// Memorisation de la description courte si elle est presente
		if (strcmp(sKeyWordShortDescription, sField) == 0)
		{
			sField = ReadNextField();
			if (bOk)
				sShortDescription = sField;
			SkipLine();

			// Lecture du champ suivant pour la suite
			sField = ReadNextField();
		}
	}

	// Dimensions
	nAttributeNumber = 0;
	if (bOk)
	{
		// Recherche du mot cle, le champ ayant ete deja lu
		if (strcmp(sKeyWordDimensions, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordDimensions + " expected but not found");
			bOk = false;
		}

		// Recherche du nombre de dimensions
		if (bOk)
		{
			sField = ReadNextField();
			nAttributeNumber = StringToInt(sField);
			if (nAttributeNumber <= 0)
			{
				AddError(sTmp + "Invalid coclustering variable number");
				bOk = false;
			}
		}

		// Passage a la ligne suivante, puis saut de la ligne d'entete de la section
		SkipLine();
		SkipLine();
	}

	// Lecture des caracteristiques des variables
	if (bOk)
	{
		// Creation de la grille
		coclusteringDataGrid->Initialize(nAttributeNumber, 0);

		// Il specifier ici la description courte, qui est reinitialisee lors du Initialise
		coclusteringDataGrid->SetShortDescription(sShortDescription);

		// Boucle de lecture des caracteristiques des variables
		for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
		{
			// Initialisations
			sAttributeName = "";
			nAttributeType = 0;
			nAttributePartNumber = 0;
			nAttributeValueNumber = 0;

			// Nom
			sField = ReadNextField();
			sAttributeName = sField;
			if (bOk and sAttributeName == "")
			{
				bOk = false;
				AddError(sTmp + "Missing variable name");
			}
			if (bOk and coclusteringDataGrid->SearchAttribute(sAttributeName) != NULL)
			{
				bOk = false;
				AddError(sTmp + "Variable " + sAttributeName + " used twice");
			}

			// Type, avec gestion des noms DEPRECATED des types, pour compatibilite ascendante
			sField = ReadNextField();
			if (strcmp(sField, "Symbol") == 0)
				nAttributeType = KWType::Symbol;
			else if (strcmp(sField, "Continuous") == 0)
				nAttributeType = KWType::Continuous;
			else
				nAttributeType = KWType::ToType(sField);
			if (bOk and not KWType::IsSimple(nAttributeType))
			{
				bOk = false;
				AddError(sTmp + "Type of variable " + sAttributeName + " (" + sField +
					 ") should be Numerical or Categorical");
			}

			// Nombre de parties
			sField = ReadNextField();
			nAttributePartNumber = StringToInt(sField);
			if (bOk and nAttributePartNumber <= 0)
			{
				bOk = false;
				AddError(sTmp + "Part number of variable " + sAttributeName + " (" + sField +
					 ") is not valid");
			}

			// Nombre de parties initiales
			sField = ReadNextField();
			nAttributeInitialPartNumber = StringToInt(sField);
			if (bOk and nAttributeInitialPartNumber <= 0)
			{
				bOk = false;
				AddError(sTmp + "Initial part number of variable " + sAttributeName + " (" + sField +
					 ") is not valid");
			}

			// Nombre de valeurs
			sField = ReadNextField();
			nAttributeValueNumber = StringToInt(sField);
			if (bOk and nAttributeValueNumber <= 0)
			{
				bOk = false;
				AddError(sTmp + "Value number of variable " + sAttributeName + " (" + sField +
					 ") is not valid");
			}

			// Typicality
			sField = ReadNextField();
			dAttributeInterest = KWContinuous::StringToContinuous(sField);
			if (bOk and not(0 <= dAttributeInterest and dAttributeInterest <= 1))
			{
				bOk = false;
				AddError(sTmp + "Interest of variable " + sAttributeName + " (" + sField +
					 ") is not valid");
			}

			// Description
			sField = ReadNextField();
			sAttributeDescription = sField;

			// Message final de coherence de la variable
			if (not bOk)
			{
				AddError(sTmp + "Invalid variable specification (" + sAttributeName + ")");
				break;
			}

			// Ajout d'une caracteristique d'attribut de coclustering
			if (bOk)
			{
				// Specification de l'attribut
				dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));
				dgAttribute->SetAttributeName(sAttributeName);
				dgAttribute->SetAttributeType(nAttributeType);
				dgAttribute->SetInitialPartNumber(nAttributeInitialPartNumber);
				dgAttribute->SetInitialValueNumber(nAttributeValueNumber);
				dgAttribute->SetGranularizedValueNumber(nAttributeValueNumber);
				dgAttribute->SetInterest(dAttributeInterest);
				dgAttribute->SetDescription(sAttributeDescription);

				// Creation de parties
				for (nPart = 0; nPart < nAttributePartNumber; nPart++)
					dgAttribute->AddPart();

				// Affichage en mode debug
				if (bReadDebug)
					cout << sKeyWordDimensions << "\t" << sAttributeName << "\t"
					     << KWType::ToString(KWType::GetSimpleCoclusteringType(nAttributeType))
					     << "\t" << nAttributePartNumber << "\t" << nAttributeValueNumber << "\t"
					     << sAttributeDescription << endl;
			}

			// Ligne suivante
			SkipLine();
		}
	}

	return bOk;
}

boolean CCCoclusteringReport::ReadCoclusteringStats(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	char* sField;
	ALString sTmp;
	int nNumber;
	double dNumber;
	double dNullCost;
	double dCost;

	require(coclusteringDataGrid != NULL);

	if (bOk)
	{
		// Recherche de l'entete de la nouvelle section
		SkipLine();
		sField = ReadNextField();
		SkipLine();
		if (strcmp(sKeyWordCoclusteringStats, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordCoclusteringStats + " expected but not found");
			bOk = false;
		}
	}

	// Instances
	if (bOk)
	{
		sField = ReadNextField();
		if (strcmp(sKeyWordInstances, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordInstances + " expected but not found");
			bOk = false;
		}
		sField = ReadNextField();
		nNumber = StringToInt(sField);
		if (bOk and nNumber < 0)
		{
			AddError(sTmp + "Invalid " + sKeyWordInstances + " (" + sField + ")");
			bOk = false;
		}
		if (bOk)
			nHeaderInstanceNumber = nNumber;
		SkipLine();
		if (bReadDebug)
			cout << sKeyWordInstances << "\t" << nNumber << endl;
	}

	// Cellules
	if (bOk)
	{
		sField = ReadNextField();
		if (strcmp(sKeyWordCells, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordCells + " expected but not found");
			bOk = false;
		}
		sField = ReadNextField();
		nNumber = StringToInt(sField);
		if (bOk and nNumber < 0)
		{
			AddError(sTmp + "Invalid " + sKeyWordCells + " (" + sField + ")");
			bOk = false;
		}
		if (bOk)
			nHeaderCellNumber = nNumber;
		SkipLine();
		if (bReadDebug)
			cout << sKeyWordCells << "\t" << nNumber << endl;
	}

	// Cout null
	if (bOk)
	{
		sField = ReadNextField();
		if (strcmp(sKeyWordNullCost, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordNullCost + " expected but not found");
			bOk = false;
		}
		sField = ReadNextField();
		dNullCost = KWContinuous::StringToContinuous(sField);
		if (bOk and dNullCost < 0)
		{
			AddError(sTmp + "Invalid " + sKeyWordNullCost + " (" + sField + ")");
			bOk = false;
		}
		if (bOk)
			coclusteringDataGrid->SetNullCost(dNullCost);
		SkipLine();
		if (bReadDebug)
			cout << sKeyWordNullCost << "\t" << dNullCost << endl;
	}

	// Cout
	if (bOk)
	{
		sField = ReadNextField();
		if (strcmp(sKeyWordCost, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordCost + " expected but not found");
			bOk = false;
		}
		sField = ReadNextField();
		dCost = KWContinuous::StringToContinuous(sField);
		if (bOk and dCost < 0)
		{
			AddError(sTmp + "Invalid " + sKeyWordCost + " (" + sField + ")");
			bOk = false;
		}
		if (bOk)
			coclusteringDataGrid->SetCost(dCost);
		SkipLine();
		if (bReadDebug)
			cout << sKeyWordCost << "\t" << dCost << endl;
	}

	// On saute le Level
	if (bOk)
		SkipLine();

	// Nombre initial d'attributs
	if (bOk)
	{
		sField = ReadNextField();
		if (strcmp(sKeyWordInitialDimensions, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordInitialDimensions + " expected but not found");
			bOk = false;
		}
		sField = ReadNextField();
		nNumber = StringToInt(sField);
		if (bOk and nNumber < 0)
		{
			AddError(sTmp + "Invalid " + sKeyWordInitialDimensions + " (" + sField + ")");
			bOk = false;
		}
		if (bOk)
			coclusteringDataGrid->SetInitialAttributeNumber(nNumber);
		SkipLine();
		if (bReadDebug)
			cout << sKeyWordInitialDimensions << "\t" << nNumber << endl;
	}

	// Variable d'effectif
	if (bOk)
	{
		sField = ReadNextField();
		if (strcmp(sKeyWordFrequencyAttribute, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordFrequencyAttribute + " expected but not found");
			bOk = false;
		}
		sField = ReadNextField();
		if (bOk)
			coclusteringDataGrid->SetFrequencyAttributeName(sField);
		SkipLine();
		if (bReadDebug)
			cout << sKeyWordFrequencyAttribute << "\t" << sField << endl;
	}

	// Dictionnaire
	if (bOk)
	{
		sField = ReadNextField();
		if (strcmp(sKeyWordDictionary, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordDictionary + " expected but not found");
			bOk = false;
		}
		sField = ReadNextField();
		if (bOk)
			coclusteringDataGrid->GetDatabaseSpec()->SetClassName(sField);
		SkipLine();
		if (bReadDebug)
			cout << sKeyWordDictionary << "\t" << sField << endl;
	}

	// Base de donnees
	if (bOk)
	{
		sField = ReadNextField();
		if (strcmp(sKeyWordDatabase, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordDatabase + " expected but not found");
			bOk = false;
		}
		sField = ReadNextField();
		if (bOk)
			coclusteringDataGrid->GetDatabaseSpec()->SetDatabaseName(sField);
		SkipLine();
		if (bReadDebug)
			cout << sKeyWordDatabase << "\t" << sField << endl;
	}

	// Lecture de specifications additionnelles
	if (bOk)
	{
		// Lecture d'un premier champ pour voir si l'on a atteint la fin de de section, ou si l'in est au format
		// Khiops V10
		sField = ReadNextField();

		// Cas d'un format anterieur a Khiops V10
		if (strcmp("", sField) == 0)
		{
			// On alimente des valeurs par defaut pour les nouveaux champs
			coclusteringDataGrid->GetDatabaseSpec()->SetSampleNumberPercentage(100);
			coclusteringDataGrid->GetDatabaseSpec()->SetModeExcludeSample(false);
			coclusteringDataGrid->GetDatabaseSpec()->SetSelectionAttribute("");
			coclusteringDataGrid->GetDatabaseSpec()->SetSelectionValue("");
		}
		// Cas du format Khiops V10
		else
		{
			// Taux d'echantillonage (sans faire le ReadNextField, dela fait)
			if (bOk)
			{
				if (strcmp(sKeyWordSamplePercentage, sField) != 0)
				{
					AddError(sTmp + "Key word " + sKeyWordSamplePercentage +
						 " expected but not found");
					bOk = false;
				}
				sField = ReadNextField();
				dNumber = StringToDouble(sField);
				if (bOk and (dNumber < 0 or dNumber > 100))
				{
					AddError(sTmp + "Invalid " + sKeyWordSamplePercentage + " (" + sField + ")");
					bOk = false;
				}
				if (bOk)
					coclusteringDataGrid->GetDatabaseSpec()->SetSampleNumberPercentage(dNumber);
				SkipLine();
				if (bReadDebug)
					cout << sKeyWordSamplePercentage << "\t" << sField << endl;
			}

			// Mode d'echantillonnage
			if (bOk)
			{
				sField = ReadNextField();
				if (strcmp(sKeyWordSamplingMode, sField) != 0)
				{
					AddError(sTmp + "Key word " + sKeyWordSamplingMode + " expected but not found");
					bOk = false;
				}
				sField = ReadNextField();
				if (bOk and not KWDatabase::CheckSamplingMode(sField))
				{
					AddError(sTmp + "Invalid " + sKeyWordSamplingMode + " (" + sField + ")");
					bOk = false;
				}
				if (bOk)
					coclusteringDataGrid->GetDatabaseSpec()->SetSamplingMode(sField);
				SkipLine();
				if (bReadDebug)
					cout << sKeyWordSamplingMode << "\t" << sField << endl;
			}

			// Variable de selection
			if (bOk)
			{
				sField = ReadNextField();
				if (strcmp(sKeyWordSelectionVariable, sField) != 0)
				{
					AddError(sTmp + "Key word " + sKeyWordSelectionVariable +
						 " expected but not found");
					bOk = false;
				}
				sField = ReadNextField();
				if (bOk)
					coclusteringDataGrid->GetDatabaseSpec()->SetSelectionAttribute(sField);
				SkipLine();
				if (bReadDebug)
					cout << sKeyWordSelectionVariable << "\t" << sField << endl;
			}

			// Valeur de selection
			if (bOk)
			{
				sField = ReadNextField();
				if (strcmp(sKeyWordSelectionValue, sField) != 0)
				{
					AddError(sTmp + "Key word " + sKeyWordSelectionValue +
						 " expected but not found");
					bOk = false;
				}
				sField = ReadNextField();
				if (bOk)
					coclusteringDataGrid->GetDatabaseSpec()->SetSelectionValue(sField);
				SkipLine();
				if (bReadDebug)
					cout << sKeyWordSelectionValue << "\t" << sField << endl;
			}
		}
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadBounds(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	char* sField;
	ALString sTmp;
	CCHDGAttribute* dgAttribute;
	int nContinuousAttributeNumber;
	int nAttribute;
	ALString sAttributeName;
	Continuous cMin;
	Continuous cMax;

	require(coclusteringDataGrid != NULL);

	// Comptage du nombre d'attributs numeriques
	nContinuousAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));
		if (dgAttribute->GetAttributeType() == KWType::Continuous)
			nContinuousAttributeNumber++;
	}

	// Parsing de la section sur les bornes des intervalles numeriques si necessaire
	if (bOk and nContinuousAttributeNumber > 0)
	{
		// Recherche de l'entete de la nouvelle section
		SkipLine();
		sField = ReadNextField();
		if (strcmp(sKeyWordBounds, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordBounds + " expected but not found");
			bOk = false;
		}

		// Passage a la ligne suivante, puis saut de la ligne d'entete de la section
		SkipLine();
		SkipLine();

		// Bornes des attributs continus
		if (bOk)
		{
			for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
			{
				// Recherche de l'attribut correspondant
				dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

				// Traitement si attribut continu
				if (dgAttribute->GetAttributeType() == KWType::Continuous)
				{
					// Initialisations
					sAttributeName = "";
					cMin = 0;
					cMax = 0;

					// Nom
					sField = ReadNextField();
					sAttributeName = sField;
					if (bOk and sAttributeName == "")
					{
						bOk = false;
						AddError(sTmp + "Missing variable name");
					}
					if (bOk and dgAttribute->GetAttributeName() != sAttributeName)
					{
						bOk = false;
						AddError(sTmp + "Expected variable name is " +
							 dgAttribute->GetAttributeName() + " (not " + sAttributeName +
							 ")");
					}

					// Min
					sField = ReadNextField();
					cMin = KWContinuous::StringToContinuous(sField);

					// Max
					sField = ReadNextField();
					cMax = KWContinuous::StringToContinuous(sField);

					// Verification de coherence (non exhaustives)
					bOk = bOk and cMin <= cMax;
					if (not bOk)
					{
						AddError(sTmp + "Invalid bound specification (" + sAttributeName + ")");
						break;
					}

					// Memorisation des bornes de l'attribut
					if (bOk)
					{
						dgAttribute->SetMin(cMin);
						dgAttribute->SetMax(cMax);

						// Affichage en mode debug
						if (bReadDebug)
							cout << sKeyWordBounds << "\t" << sAttributeName << "\t" << cMin
							     << "\t" << cMax << endl;
					}

					// Ligne suivante
					SkipLine();
				}
			}
		}
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadHierarchy(CCHierarchicalDataGrid* coclusteringDataGrid,
					    ObjectArray* oaAttributesPartDictionaries)
{
	boolean bOk = true;
	char* sField;
	ALString sTmp;
	int nAttribute;
	int nPart;
	CCHDGAttribute* dgAttribute;
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	ObjectDictionary* odPartDictionary;
	ALString sAttributeName;
	ALString sPartName;
	ALString sParentPartName;
	POSITION position;
	Object* oElement;

	require(coclusteringDataGrid != NULL);
	require(oaAttributesPartDictionaries != NULL);
	require(oaAttributesPartDictionaries->GetSize() == 0);

	// Boucle de specification des partitions des attributs
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut correspondant
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Recherche de l'entete de la nouvelle section
		SkipLine();
		sField = ReadNextField();
		if (strcmp(sKeyWordHierarchy, sField) != 0)
		{
			AddError(sTmp + "Key word " + sKeyWordHierarchy + " expected but not found");
			bOk = false;
		}

		// Verification de la coherence de la section du rapport
		sField = ReadNextField();
		if (strcmp(dgAttribute->GetAttributeName(), sField) != 0)
		{
			AddError(sTmp + "Variable " + dgAttribute->GetAttributeName() +
				 " expected but not found after key word " + sKeyWordHierarchy);
			bOk = false;
		}

		// Passage a la ligne suivante, puis saut de la ligne d'entete de la section
		SkipLine();
		SkipLine();

		// Parties de l'attribut
		odPartDictionary = NULL;
		if (bOk)
		{
			// Creation d'un dictionnaire de parties
			odPartDictionary = new ObjectDictionary;
			oaAttributesPartDictionaries->Add(odPartDictionary);

			// Extraction des parties
			dgPart = dgAttribute->GetHeadPart();
			while (dgPart != NULL)
			{
				// Lecture des caracteristiques de la partie
				hdgPart = cast(CCHDGPart*, dgPart);
				bOk = bOk and
				      ReadHierarchyPart(coclusteringDataGrid, dgAttribute, hdgPart, odPartDictionary);
				if (not bOk)
					break;

				// Partie suivante
				dgAttribute->GetNextPart(dgPart);
			}

			// Saut des lignes jusqu'a la fin de la section de la hierarchie
			// Encore (PartNumber-1) ligne pour avoir l'ensemble des partie de la hierachie (ayant
			// PartNumber feuilles)
			for (nPart = 0; nPart < dgAttribute->GetPartNumber() - 1; nPart++)
			{
				// Lecture des caracteristiques de la partie
				hdgPart = NULL;
				bOk = bOk and
				      ReadHierarchyPart(coclusteringDataGrid, dgAttribute, hdgPart, odPartDictionary);
				if (not bOk)
					break;
			}
		}
		assert(not bOk or odPartDictionary != NULL);

		// Test de specification de la racine
		if (bOk and dgAttribute->GetRootPart() == NULL)
		{
			AddError("Missing root in part hierarchy for variable " + dgAttribute->GetAttributeName());
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
			if (odPartDictionary != NULL)
			{
				position = odPartDictionary->GetStartPosition();
				while (position != NULL)
				{
					odPartDictionary->GetNextAssoc(position, sPartName, oElement);
					hdgPart = cast(CCHDGPart*, oElement);

					// Supression des parties non filles
					if (not hdgPart->IsLeaf())
					{
						odPartDictionary->RemoveKey(sPartName);
						delete hdgPart;
					}
				}
			}
		}

		// Arret si erreur
		if (not bOk)
			break;
	}
	ensure(not bOk or oaAttributesPartDictionaries->GetSize() == coclusteringDataGrid->GetAttributeNumber());
	return bOk;
}

boolean CCCoclusteringReport::ReadHierarchyPart(CCHierarchicalDataGrid* coclusteringDataGrid,
						CCHDGAttribute* dgAttribute, CCHDGPart*& dgPart,
						ObjectDictionary* odPartDictionary)
{
	boolean bOk = true;
	boolean bIsLeafPart;
	char* sField;
	ALString sTmp;
	CCHDGPart* dgParentPart;
	KWDGPart* dgPreviousPart;
	KWDGInterval* dgInterval;
	ALString sPartName;
	Continuous cBound;
	int nFrequency;
	ALString sParentPartName;
	double dHierarchicalLevel;
	int nRank;
	double dInterest;
	int nHierarchicalRank;
	int nChar;

	require(coclusteringDataGrid != NULL);
	require(dgAttribute != NULL);
	require(coclusteringDataGrid->GetAttributeAt(dgAttribute->GetAttributeIndex()) == dgAttribute);
	require(dgPart == NULL or dgPart->GetAttribute() == dgAttribute);
	require(odPartDictionary != NULL);

	// Indicateur de partie fille
	bIsLeafPart = dgPart != NULL;

	// Initialisation
	sPartName = "";
	cBound = 0;

	// Nom de la partie
	sField = ReadNextField();
	sPartName = sField;

	// Extraction des bornes d'intervalle dans le cas numerique, pour les parties feuilles
	if (bOk and dgAttribute->GetAttributeType() == KWType::Continuous and bIsLeafPart)
	{
		dgInterval = dgPart->GetInterval();

		// Initialisation des bornes
		dgInterval->SetLowerBound(KWDGInterval::GetMinLowerBound());
		dgInterval->SetUpperBound(KWDGInterval::GetMaxUpperBound());

		// Recherche d'une borne inf a partir du deuxieme intervalle
		if (dgPart != dgAttribute->GetHeadPart())
		{
			// Cas particulier: valeur manquante
			if (strcmp("Missing", sField) == 0)
			{
				dgInterval->SetLowerBound(KWContinuous::GetMissingValue());
				dgInterval->SetUpperBound(KWContinuous::GetMissingValue());
			}
			// Cas standard
			else
			{
				// On remplace le separateur d'intervalle ';' par '\0', apres avoir saute le premiere
				// caractere ']'
				nChar = 1;
				while (sField[nChar] != '\0' and sField[nChar] != ';')
					nChar++;
				sField[nChar] = '\0';

				// Extraction de la borne inf
				cBound = KWContinuous::StringToContinuous(&(sField[1]));
				dgInterval->SetLowerBound(cBound);

				// On l'utilise comme borne sup de l'intervalle precedent
				dgPreviousPart = dgPart;
				dgAttribute->GetPrevPart(dgPreviousPart);
				dgPreviousPart->GetInterval()->SetUpperBound(cBound);
			}
		}
	}

	// Nom de la partie parente
	sField = ReadNextField();
	sParentPartName = sField;

	// Effectif
	sField = ReadNextField();
	nFrequency = StringToInt(sField);

	// Interet
	sField = ReadNextField();
	dInterest = KWContinuous::StringToContinuous(sField);

	// Niveau hierarchique
	sField = ReadNextField();
	dHierarchicalLevel = KWContinuous::StringToContinuous(sField);

	// Rang
	sField = ReadNextField();
	nRank = StringToInt(sField);

	// Rang hierarchique
	sField = ReadNextField();
	nHierarchicalRank = StringToInt(sField);

	//////////////////////////////////////////////////
	// Verification de coherence (non exhaustives)

	// Nom de partie
	if (bOk and sPartName == "")
	{
		AddError(sTmp + "Missing part name for variable " + dgAttribute->GetAttributeName());
		bOk = false;
	}

	// Nom de partie parente
	if (bOk and sPartName == sParentPartName)
	{
		AddError(sTmp + "Part name (" + sPartName + ") same as parent part name for variable " +
			 dgAttribute->GetAttributeName());
		bOk = false;
	}

	// Effectif
	if (bOk and nFrequency <= 0)
	{
		AddError(sTmp + "Part (" + sPartName + ") with wrong frequency for variable " +
			 dgAttribute->GetAttributeName());
		bOk = false;
	}

	// Interet
	if (bOk and not(0 <= dInterest and dInterest <= 1))
	{
		AddError(sTmp + "Part (" + sPartName + ") with wrong interest for variable " +
			 dgAttribute->GetAttributeName());
		bOk = false;
	}

	// Niveau hierarchique
	if (bOk and dHierarchicalLevel > 1)
	{
		AddError(sTmp + "Part (" + sPartName + ") with wrong hierarchical level for variable " +
			 dgAttribute->GetAttributeName());
		bOk = false;
	}

	// Rang
	if (bOk and nRank < 1)
	{
		AddError(sTmp + "Part (" + sPartName + ") with wrong rank for variable " +
			 dgAttribute->GetAttributeName());
		bOk = false;
	}

	// Rang hierarchique
	if (bOk and nHierarchicalRank < 0)
	{
		AddError(sTmp + "Part (" + sPartName + ") with wrong hierarchical rank for variable " +
			 dgAttribute->GetAttributeName());
		bOk = false;
	}
	if (bOk and nHierarchicalRank == 0)
		AddWarning(sTmp + "Part (" + sPartName + ") with missing hierarchical rank for variable " +
			   dgAttribute->GetAttributeName());

	// Test de presence dans le dictionnaire des partie
	if (bIsLeafPart)
		bOk = bOk and odPartDictionary->Lookup(sPartName) == NULL;
	else
		bOk = bOk and odPartDictionary->Lookup(sPartName) != NULL;
	if (not bOk)
	{
		AddError(sTmp + "Invalid part specification (" + sPartName + ") for variable " +
			 dgAttribute->GetAttributeName());
		bOk = false;
	}

	// Memorisation des informations sur la partie
	if (bOk)
	{
		// Si partie feuille, memorisation de l'association entre nom et partie
		if (bIsLeafPart)
			odPartDictionary->SetAt(sPartName, dgPart);
		// Sinon, recherche dans le dictionnaire
		else
			dgPart = cast(CCHDGPart*, odPartDictionary->Lookup(sPartName));
		check(dgPart);

		// Memorisation des caracteristiques de la partie
		dgPart->SetPartName(sPartName);
		dgPart->SetPartFrequency(nFrequency);
		dgPart->SetInterest(dInterest);
		dgPart->SetHierarchicalLevel(dHierarchicalLevel);
		dgPart->SetRank(nRank);
		dgPart->SetHierarchicalRank(nHierarchicalRank);

		// Gestion de la partie parente
		if (sParentPartName != "")
		{
			// Creation si necessaire de la partie parente
			dgParentPart = cast(CCHDGPart*, odPartDictionary->Lookup(sParentPartName));
			if (dgParentPart == NULL)
			{
				dgParentPart = dgAttribute->NewHierarchyPart();
				dgParentPart->SetPartName(sParentPartName);
				odPartDictionary->SetAt(sParentPartName, dgParentPart);
			}

			// Chainage de la partie dans la partie parente
			if (dgParentPart->GetChildPart1() == NULL)
				dgParentPart->SetChildPart1(dgPart);
			else if (dgParentPart->GetChildPart2() == NULL)
				dgParentPart->SetChildPart2(dgPart);
			else
			{
				AddError(sTmp + "Parent part (" + sParentPartName + ") already have two child parts");
				bOk = false;
			}

			// Fin du chainage si Ok
			if (bOk)
				dgPart->SetParentPart(dgParentPart);
		}
		// Gestion de la partie racine
		else
		{
			assert(dgPart->IsRoot());
			if (dgAttribute->GetRootPart() != NULL)
			{
				AddError(sTmp + "Root part (" + sPartName + ") already defined for variable " +
					 dgAttribute->GetAttributeName());
				bOk = false;
			}
			else
				dgAttribute->SetRootPart(dgPart);
		}

		// Affichage en mode debug
		if (bReadDebug)
		{
			cout << sKeyWordHierarchy << "\t" << dgAttribute->GetAttributeName() << "\t" << sPartName
			     << "\t" << sParentPartName << "\t" << nFrequency << "\t" << dInterest << "\t"
			     << dHierarchicalLevel << "\t" << nRank;
			if (dgAttribute->GetAttributeType() == KWType::Continuous and bIsLeafPart and
			    dgPart != dgAttribute->GetHeadPart())
				cout << "\t" << cBound;
			cout << "\n";
		}
	}

	// Ligne suivante
	SkipLine();
	return bOk;
}

boolean CCCoclusteringReport::ReadComposition(CCHierarchicalDataGrid* coclusteringDataGrid,
					      ObjectArray* oaAttributesPartDictionaries)
{
	boolean bOk = true;
	char* sField;
	ALString sTmp;
	CCHDGAttribute* dgAttribute;
	KWDGPart* dgPart;
	CCHDGSymbolValueSet* dgValueSet;
	CCHDGSymbolValue* dgValue;
	int nAttribute;
	ObjectDictionary* odPartDictionary;
	ALString sAttributeName;
	ALString sPartName;
	ALString sValueName;
	int nFrequency;
	double dTypicality;
	ALString sTrimedStarValueName;
	boolean bStarValueFound;
	Symbol sValue;
	int nTotalValueFrequency;

	require(coclusteringDataGrid != NULL);
	require(oaAttributesPartDictionaries != NULL);
	require(oaAttributesPartDictionaries->GetSize() == coclusteringDataGrid->GetAttributeNumber());

	// Valeur chaine de caractere de la star value, trimee, pour la rechercher dans le fichier
	sTrimedStarValueName = Symbol::GetStarValue().GetValue();
	sTrimedStarValueName.TrimLeft();
	sTrimedStarValueName.TrimRight();

	// Boucle de specification de la composition des attributs categoriels
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut correspondant
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));
		odPartDictionary = cast(ObjectDictionary*, oaAttributesPartDictionaries->GetAt(nAttribute));

		// Traitement si attribut categoriel
		if (dgAttribute->GetAttributeType() == KWType::Symbol)
		{
			// Recherche de l'entete de la nouvelle section
			SkipLine();
			sField = ReadNextField();
			if (strcmp(sKeyWordComposition, sField) != 0)
			{
				AddError(sTmp + "Key word " + sKeyWordComposition + " expected but not found");
				bOk = false;
			}

			// Verification de la coherence de la section du rapport
			sField = ReadNextField();
			if (strcmp(dgAttribute->GetAttributeName(), sField) != 0)
			{
				AddError(sTmp + "Variable " + dgAttribute->GetAttributeName() +
					 " expected but not found after key word " + sKeyWordComposition);
				bOk = false;
			}

			// Passage a la ligne suivante, puis saut de la ligne d'entete de la section
			SkipLine();
			SkipLine();

			// Valeurs de l'attribut
			if (bOk)
			{
				// Extraction des valeurs
				// Elle sont specifiees sequentiellement selon les parties
				dgPart = NULL;
				bStarValueFound = false;
				while (not IsEndOfFile() and bOk)
				{
					// Initialisations
					sPartName = "";
					sValueName = "";
					nFrequency = 0;

					// Nom de partie
					sField = ReadNextField();
					sPartName = sField;

					// Nom de valeur
					sField = ReadNextField();
					sValueName = sField;

					// Effectif lie a la valeur
					sField = ReadNextField();
					nFrequency = StringToInt(sField);

					// Typicalite
					sField = ReadNextField();
					dTypicality = KWContinuous::StringToContinuous(sField);

					// Arret si nom de partie vide (fin de section)
					if (sPartName == "")
					{
						// On rajoute si necessaire la StarValue au dernier groupe specifie
						if (dgPart != NULL and not bStarValueFound)
						{
							dgValueSet = cast(CCHDGSymbolValueSet*, dgPart->GetValueSet());
							dgValueSet->AddSymbolValue(Symbol::GetStarValue());
						}
						break;
					}

					// Recherche du groupe a mettre ajour
					dgPart = cast(KWDGPart*, odPartDictionary->Lookup(sPartName));

					//////////////////////////////////////////////////
					// Verification de coherence (non exhaustives)

					// Nom de partie
					if (bOk and sPartName == "")
					{
						AddError(sTmp + "Missing part name for value specification (" +
							 sValueName + ") for variable " +
							 dgAttribute->GetAttributeName());
						break;
					}

					// Existence de la partie
					if (bOk and dgPart == NULL)
					{
						AddError(sTmp + "Missing part (" + sPartName +
							 ") for value specification (" + sValueName +
							 ") for variable " + dgAttribute->GetAttributeName());
						break;
					}

					// Effectif
					if (bOk and nFrequency <= 0)
					{
						// Cas particulier de l'effectif null: seul la star value est admise
						if (nFrequency == 0 and sValueName != sTrimedStarValueName)
						{
							AddError(sTmp + "Value specification (" + sValueName +
								 ") with wrong frequency for variable " +
								 dgAttribute->GetAttributeName());
							break;
						}
					}

					// Typicalite
					if (bOk and not(0 <= dTypicality and dTypicality <= 1))
					{
						// Tolerance pour les typicalite negatives
						if (dTypicality < 0)
						{
							AddWarning(sTmp + "Value specification (" + sValueName +
								   ") with typicality less than 0 for variable " +
								   dgAttribute->GetAttributeName() +
								   " (replaced by 0)");
							dTypicality = 0;
						}
						// Erreur pour les typicalite supereures a 1
						else
						{
							AddError(sTmp + "Value specification (" + sValueName +
								 ") with typicality greater than 1 for variable " +
								 dgAttribute->GetAttributeName());
							break;
						}
					}

					// Test si on a trouve la star value
					if (nFrequency == 0 and sValueName == sTrimedStarValueName)
						bStarValueFound = true;

					// Message global
					if (not bOk)
					{
						AddError(sTmp + "Invalid value specification (" + sValueName +
							 ") for variable " + dgAttribute->GetAttributeName());
						break;
					}

					// Memorisation des caracteristiques de la valeur dans sa partie
					if (bOk)
					{
						// Transformation de la valeur en Symbol
						if (nFrequency == 0 and sValueName == sTrimedStarValueName)
							sValue = Symbol::GetStarValue();
						else
							sValue = (Symbol)sValueName;

						// Memorisation de la valeur
						dgValueSet = cast(CCHDGSymbolValueSet*, dgPart->GetValueSet());
						dgValue = cast(CCHDGSymbolValue*, dgValueSet->AddSymbolValue(sValue));
						dgValue->SetValueFrequency(nFrequency);
						dgValue->SetTypicality(dTypicality);

						if (bReadDebug)
							cout << sKeyWordComposition << "\t" << sPartName << "\t"
							     << sValueName << "\t" << nFrequency << "\t" << dTypicality
							     << "\n";
					}

					// Ligne suivante
					SkipLine();
				}
			}

			// Arret si erreur
			if (not bOk)
				break;

			// Verification rapide des parties de l'attribut
			if (bOk)
			{
				dgPart = dgAttribute->GetHeadPart();
				while (dgPart != NULL)
				{
					// Verification de la compatibilite entre l'effectif de la partie
					// et l'effectif cumule de ses valeurs
					dgValueSet = cast(CCHDGSymbolValueSet*, dgPart->GetValueSet());
					nTotalValueFrequency = dgValueSet->ComputeTotalFrequency();
					if (dgPart->GetPartFrequency() != nTotalValueFrequency)
					{
						dgAttribute->AddError(
						    sTmp + "Frequency (" + IntToString(dgPart->GetPartFrequency()) +
						    ") of cluster " + dgPart->GetObjectLabel() +
						    " different from the cumulated frequency of its values (" +
						    IntToString(nTotalValueFrequency) + ")");
						bOk = false;
						break;
					}

					// Partie suivante
					dgAttribute->GetNextPart(dgPart);
				}
			}
		}
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadCells(CCHierarchicalDataGrid* coclusteringDataGrid,
					ObjectArray* oaAttributesPartDictionaries)
{
	boolean bOk = true;
	char* sField;
	ALString sTmp;
	KWDGPart* dgPart;
	KWDGCell* dgCell;
	int nAttribute;
	int nCell;
	ObjectDictionary* odPartDictionary;
	ObjectArray oaCellParts;
	ALString sPartName;
	int nFrequency;

	require(coclusteringDataGrid != NULL);
	require(oaAttributesPartDictionaries != NULL);
	require(oaAttributesPartDictionaries->GetSize() == coclusteringDataGrid->GetAttributeNumber());
	require(coclusteringDataGrid != NULL);
	require(fReport != NULL);

	// Recherche de l'entete de la nouvelle section
	SkipLine();
	sField = ReadNextField();
	if (strcmp(sKeyWordCells, sField) != 0)
	{
		AddError(sTmp + "Key word " + sKeyWordCells + " expected but not found");
		bOk = false;
	}

	// Passage a la ligne suivante, puis saut de la ligne d'entete de la section
	SkipLine();
	SkipLine();

	// Cellules de la grille
	if (bOk)
	{
		oaCellParts.SetSize(coclusteringDataGrid->GetAttributeNumber());
		coclusteringDataGrid->SetCellUpdateMode(true);
		for (nCell = 0; nCell < nHeaderCellNumber; nCell++)
		{
			// Recherche des parties des attributs
			for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
			{
				// Nom de partie
				sField = ReadNextField();
				sPartName = sField;

				// Recherche de la partie dans son dictionnaire de partie
				odPartDictionary =
				    cast(ObjectDictionary*, oaAttributesPartDictionaries->GetAt(nAttribute));
				dgPart = cast(KWDGPart*, odPartDictionary->Lookup(sPartName));
				oaCellParts.SetAt(nAttribute, dgPart);

				// Arret si partie non trouve
				bOk = bOk and dgPart != NULL;
				if (not bOk)
				{
					AddError("Cell specification with part " + sPartName +
						 " missing for variable " +
						 coclusteringDataGrid->GetAttributeAt(nAttribute)->GetAttributeName());
					break;
				}
			}

			// Recherche de l'effectif de la cellule
			sField = ReadNextField();
			nFrequency = StringToInt(sField);

			// Arret si probleme de validite
			if (bOk and nFrequency <= 0)
			{
				AddError("Cell specification with wrong frequency");
				break;
			}
			bOk = bOk and coclusteringDataGrid->LookupCell(&oaCellParts) == NULL;
			if (not bOk)
			{
				AddError("Invalid cell specification");
				break;
			}

			// Ajout de la cellule
			if (bOk)
			{
				dgCell = coclusteringDataGrid->AddCell(&oaCellParts);
				dgCell->SetCellFrequency(nFrequency);
				if (bReadDebug)
					cout << "sKeyWordCells"
					     << "\t" << *dgCell;
			}

			// Ligne suivante
			SkipLine();
		}
		coclusteringDataGrid->SetCellUpdateMode(false);
	}

	return bOk;
}

boolean CCCoclusteringReport::ReadAnnotation(CCHierarchicalDataGrid* coclusteringDataGrid,
					     ObjectArray* oaAttributesPartDictionaries)
{
	boolean bOk = true;
	char* sField;
	ALString sTmp;
	CCHDGAttribute* dgAttribute;
	CCHDGPart* dgPart;
	int nAttribute;
	ObjectDictionary* odPartDictionary;
	ALString sPartName;
	boolean bExpand;
	boolean bSelected;
	ALString sShortDescription;
	ALString sDescription;

	require(coclusteringDataGrid != NULL);
	require(oaAttributesPartDictionaries != NULL);
	require(oaAttributesPartDictionaries->GetSize() == coclusteringDataGrid->GetAttributeNumber());

	// Boucle de specification de la composition des attributs categoriels
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut correspondant
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));
		odPartDictionary = cast(ObjectDictionary*, oaAttributesPartDictionaries->GetAt(nAttribute));

		// Recherche de l'entete de la nouvelle section
		SkipLine();
		sField = ReadNextField();

		// Soit il n'y a aucune section annotation, soit il y en a une par attribut
		if (strcmp(sKeyWordAnnotation, sField) != 0)
		{
			// Si pas de section annotation pour le premier attribut, on considere que ce n'est pas une
			// erreur
			if (nAttribute > 0)
			{
				AddError(sTmp + "Key word " + sKeyWordAnnotation + " expected but not found");
				bOk = false;
			}

			// Arret du parsing de toute facon
			break;
		}

		// Verification de la coherence de la section du rapport
		sField = ReadNextField();
		if (strcmp(dgAttribute->GetAttributeName(), sField) != 0)
		{
			AddError(sTmp + "Variable " + dgAttribute->GetAttributeName() +
				 " expected but not found after key word " + sKeyWordAnnotation);
			bOk = false;
		}

		// Passage a la ligne suivante, puis saut de la ligne d'entete de la section
		SkipLine();
		SkipLine();

		// Valeurs de l'attribut
		if (bOk)
		{
			// Extraction des valeurs
			// Elle sont specifiees sequentiellement selon les parties
			dgPart = NULL;
			while (not IsEndOfFile() and bOk)
			{
				// Initialisations
				sPartName = "";
				bExpand = false;
				bSelected = false;
				sShortDescription = "";
				sDescription = "";

				// Nom de partie
				sField = ReadNextField();
				sPartName = sField;

				// Indicateur de depliement
				sField = ReadNextField();
				if (strcmp(sKeyWordTrue, sField) == 0)
					bExpand = true;

				// Indicateur de selection
				sField = ReadNextField();
				if (strcmp(sKeyWordTrue, sField) == 0)
					bSelected = true;

				// Libelle court
				sField = ReadNextField();
				sShortDescription = sField;

				// Description longue
				sField = ReadNextField();
				sDescription = sField;

				// Arret si nom de partie vide (fin de section)
				if (sPartName == "")
					break;

				// Recherche du groupe a mettre ajour
				dgPart = cast(CCHDGPart*, odPartDictionary->Lookup(sPartName));

				// Verification de coherence (non exhaustives)
				bOk = bOk and sPartName != "";
				bOk = bOk and dgPart != NULL;
				if (not bOk)
				{
					AddError(sTmp + "Invalid part annotation (" + sPartName + ") for variable " +
						 dgAttribute->GetAttributeName());
					break;
				}

				// Memorisation des caracteristiques de la valeurs dans sa partie
				if (bOk)
				{
					dgPart->SetExpand(bExpand);
					dgPart->SetSelected(bSelected);
					dgPart->SetShortDescription(sShortDescription);
					dgPart->SetDescription(sDescription);

					if (bReadDebug)
						cout << sKeyWordAnnotation << "\t" << sPartName << "\t" << bExpand
						     << "\t" << bSelected << "\t" << sShortDescription << "\t"
						     << sDescription << "\n";
				}

				// Ligne suivante
				SkipLine();
			}
		}

		// Arret si erreur
		if (not bOk)
			break;
	}
	return bOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Ecriture des sections d'un rapport de coclustering vers un stream en sortie

void CCCoclusteringReport::InternalWriteReport(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost)
{
	require(coclusteringDataGrid != NULL);
	require(fReport == NULL);

	// Ecriture de chaque section du rapport de coclustering
	WriteVersion(coclusteringDataGrid, ost);
	WriteDimensions(coclusteringDataGrid, ost);
	WriteCoclusteringStats(coclusteringDataGrid, ost);
	WriteBounds(coclusteringDataGrid, ost);
	WriteHierarchy(coclusteringDataGrid, ost);
	WriteComposition(coclusteringDataGrid, ost);
	WriteCells(coclusteringDataGrid, ost);
	WriteAnnotation(coclusteringDataGrid, ost);
}

void CCCoclusteringReport::WriteVersion(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost)
{
	ost << GetLearningReportHeaderLine() << "\n";
}

void CCCoclusteringReport::WriteDimensions(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost)
{
	int nAttribute;
	CCHDGAttribute* dgAttribute;
	int nValueNumber;

	require(coclusteringDataGrid != NULL);

	// Entete
	ost << sKeyWordShortDescription << "\t" << coclusteringDataGrid->GetShortDescription() << "\n";
	ost << sKeyWordDimensions << "\t" << coclusteringDataGrid->GetAttributeNumber() << "\n";
	if (not KWFrequencyTable::GetWriteGranularityAndGarbage())
		ost << "Name\tType\tParts\tInitial parts\tValues\tInterest\tDescription\n";
	else
		ost << "Name\tType\tParts\tInitial parts\tValues\tInterest\tDescription\tGarbage\n";

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Nombre initial de valeurs
		nValueNumber = dgAttribute->GetInitialValueNumber();

		// Caracteristique des attributs
		// Le nombre de valeur est diminuer en externe de 1 pour tenir compte de de la StarValue en interne
		ost << dgAttribute->GetAttributeName() << "\t";
		ost << KWType::ToString(KWType::GetSimpleCoclusteringType(dgAttribute->GetAttributeType())) << "\t";
		ost << dgAttribute->GetPartNumber() << "\t";
		ost << dgAttribute->GetInitialPartNumber() << "\t";
		ost << nValueNumber << "\t";
		ost << dgAttribute->GetInterest() << "\t";
		ost << dgAttribute->GetDescription();
		if (not KWFrequencyTable::GetWriteGranularityAndGarbage())
			ost << "\n";
		else
			ost << "\t" << (dgAttribute->GetGarbageModalityNumber() > 0) << "\n";
	}
	ost << "\n";
}

void CCCoclusteringReport::WriteCoclusteringStats(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost)
{
	require(coclusteringDataGrid != NULL);

	// Entete
	ost << sKeyWordCoclusteringStats << "\n";

	// Statistiques
	ost << sKeyWordInstances << "\t" << coclusteringDataGrid->GetGridFrequency() << "\n";
	ost << sKeyWordCells << "\t" << coclusteringDataGrid->GetCellNumber() << "\n";
	ost << sKeyWordNullCost << "\t" << KWContinuous::ContinuousToString(coclusteringDataGrid->GetNullCost())
	    << "\n";
	ost << sKeyWordCost << "\t" << KWContinuous::ContinuousToString(coclusteringDataGrid->GetCost()) << "\n";
	ost << sKeyWordLevel << "\t" << KWContinuous::ContinuousToString(coclusteringDataGrid->GetLevel()) << "\n";
	ost << sKeyWordInitialDimensions << "\t" << coclusteringDataGrid->GetInitialAttributeNumber() << "\n";
	ost << sKeyWordFrequencyAttribute << "\t" << coclusteringDataGrid->GetFrequencyAttributeName() << "\n";
	ost << sKeyWordDictionary << "\t" << coclusteringDataGrid->GetConstDatabaseSpec()->GetClassName() << "\n";
	ost << sKeyWordDatabase << "\t" << coclusteringDataGrid->GetConstDatabaseSpec()->GetDatabaseName() << "\n";

	// Informations supplementaires sur la base
	ost << sKeyWordSamplePercentage << "\t"
	    << coclusteringDataGrid->GetConstDatabaseSpec()->GetSampleNumberPercentage() << "\n";
	ost << sKeyWordSamplingMode << "\t" << coclusteringDataGrid->GetConstDatabaseSpec()->GetSamplingMode() << "\n";
	ost << sKeyWordSelectionVariable << "\t"
	    << coclusteringDataGrid->GetConstDatabaseSpec()->GetSelectionAttribute() << "\n";
	ost << sKeyWordSelectionValue << "\t" << coclusteringDataGrid->GetConstDatabaseSpec()->GetSelectionValue()
	    << "\n";
	ost << "\n";
}

void CCCoclusteringReport::WriteBounds(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost)
{
	CCHDGAttribute* dgAttribute;
	int nContinuousAttributeNumber;
	int nAttribute;

	require(coclusteringDataGrid != NULL);

	// Comptage du nombre d'attributs numeriques
	nContinuousAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));
		if (dgAttribute->GetAttributeType() == KWType::Continuous)
			nContinuousAttributeNumber++;
	}

	// Ecriture des bornes des intervalles numeriques si necessaire
	if (nContinuousAttributeNumber > 0)
	{
		// Entete
		ost << sKeyWordBounds << "\n";
		ost << "Name	Min	Max\n";

		// Bornes des attributs numeriques
		for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
		{
			dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

			// Traitement si attribut continu
			if (dgAttribute->GetAttributeType() == KWType::Continuous)
			{
				ost << dgAttribute->GetAttributeName() << "\t"
				    << KWContinuous::ContinuousToString(dgAttribute->GetMin()) << "\t"
				    << KWContinuous::ContinuousToString(dgAttribute->GetMax()) << "\n";
			}
		}
		ost << "\n";
	}
}

void CCCoclusteringReport::WriteHierarchy(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost)
{
	CCHDGAttribute* dgAttribute;
	int nAttribute;
	CCHDGPart* hdgPart;
	ObjectArray oaParts;
	int nPart;
	boolean bWriteHierarchicalRank;

	require(coclusteringDataGrid != NULL);

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Exports de toutes les parties de la hierarchie en partant de la racine
		dgAttribute->ExportHierarchyParts(&oaParts);

		// Tri pour respecter l'ordre d'affichage
		oaParts.SetCompareFunction(CCHDGPartCompareLeafRank);
		oaParts.Sort();
		assert(oaParts.GetSize() > 0);

		// On determine s'il faut ecrire le rang hierarchique (absent en version 7.5)
		bWriteHierarchicalRank = false;
		if (oaParts.GetSize() > 0)
		{
			hdgPart = cast(CCHDGPart*, oaParts.GetAt(0));
			bWriteHierarchicalRank = (hdgPart->GetHierarchicalRank() > 0);
		}

		// Entete, avec rang hierarchique facultatif
		ost << sKeyWordHierarchy << "\t" << dgAttribute->GetAttributeName() << "\n";
		if (bWriteHierarchicalRank)
			ost << "Cluster	"
			       "ParentCluster\tFrequency\tInterest\tHierarchicalLevel\tRank\tHierarchicalRank\n";
		else
			ost << "Cluster	ParentCluster\tFrequency\tInterest\tHierarchicalLevel\tRank\n";

		// Affichage des parties
		for (nPart = 0; nPart < oaParts.GetSize(); nPart++)
		{
			hdgPart = cast(CCHDGPart*, oaParts.GetAt(nPart));

			// Caracteristiques de la partie
			ost << hdgPart->GetPartName();
			ost << "\t" << hdgPart->GetParentPartName();
			ost << "\t" << hdgPart->GetPartFrequency();
			ost << "\t" << hdgPart->GetInterest();
			ost << "\t" << hdgPart->GetHierarchicalLevel();
			ost << "\t" << hdgPart->GetRank();
			if (bWriteHierarchicalRank)
				ost << "\t" << hdgPart->GetHierarchicalRank();
			if (KWFrequencyTable::GetWriteGranularityAndGarbage())
			{
				if (hdgPart == dgAttribute->GetGarbagePart())
					ost << "\t"
					    << "Garbage";
			}
			ost << "\n";
		}
		oaParts.SetSize(0);
		ost << "\n";
	}
}

void CCCoclusteringReport::WriteComposition(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost)
{
	CCHDGAttribute* dgAttribute;
	int nAttribute;
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	CCHDGSymbolValueSet* hdgValueSet;
	KWDGValue* dgValue;
	CCHDGSymbolValue* hdgValue;
	// CH IV Begin
	CCHDGVarPartSet* hdgVarPartSet;
	KWDGValue* dgVarPartValue;
	CCHDGVarPartValue* hdgVarPartValue;
	// CH IV End

	require(coclusteringDataGrid != NULL);

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Traitement uniquement des attributs categoriels
		if (dgAttribute->GetAttributeType() == KWType::Symbol)
		{
			// Entete
			ost << sKeyWordComposition << "\t" << dgAttribute->GetAttributeName() << "\n";
			ost << "Cluster	Value	Frequency	Typicality\n";

			// Parcours des parties
			dgPart = dgAttribute->GetHeadPart();
			while (dgPart != NULL)
			{
				hdgPart = cast(CCHDGPart*, dgPart);

				// Parcours des valeurs
				hdgValueSet = cast(CCHDGSymbolValueSet*, hdgPart->GetValueSet());
				dgValue = hdgValueSet->GetHeadValue();
				while (dgValue != NULL)
				{
					hdgValue = cast(CCHDGSymbolValue*, dgValue);

					// Caracteristiques des valeurs
					// (y compris la valeur par defaut, pour etre coherent avec l'export JSON)
					ost << hdgPart->GetPartName() << "\t" << hdgValue->GetSymbolValue() << "\t"
					    << hdgValue->GetValueFrequency() << "\t" << hdgValue->GetTypicality()
					    << "\n";

					// Valeur suivante
					hdgValueSet->GetNextValue(dgValue);
				}

				// Partie suivante
				dgAttribute->GetNextPart(dgPart);
			}
			ost << "\n";
		}
		// CH IV Begin
		// Traitement des attributs de type VarPart
		else if (dgAttribute->GetAttributeType() == KWType::VarPart)
		{
			// Entete
			ost << sKeyWordComposition << "\t" << dgAttribute->GetAttributeName() << "\n";
			ost << "Cluster	Value	Frequency	Typicality\n";

			// Parcours des parties
			dgPart = dgAttribute->GetHeadPart();
			while (dgPart != NULL)
			{
				hdgPart = cast(CCHDGPart*, dgPart);

				// Parcours des valeurs
				hdgVarPartSet = cast(CCHDGVarPartSet*, hdgPart->GetVarPartSet());
				dgVarPartValue = hdgVarPartSet->GetHeadValue();
				while (dgVarPartValue != NULL)
				{
					hdgVarPartValue = cast(CCHDGVarPartValue*, dgVarPartValue);

					// Caracteristiques de la partie de variable
					ost << hdgPart->GetPartName() << "\t";
					ost << hdgVarPartValue->GetVarPart()->GetVarPartLabel();
					ost << "\t" << hdgVarPartValue->GetVarPart()->GetPartFrequency() << "\t"
					    << hdgVarPartValue->GetTypicality() << "\n";

					// Valeur suivante
					hdgVarPartSet->GetNextValue(dgVarPartValue);
				}

				// Partie suivante
				dgAttribute->GetNextPart(dgPart);
			}
			ost << "\n";
		}
		// CH IV End
	}
}

void CCCoclusteringReport::WriteCells(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost)
{
	int nAttribute;
	KWDGAttribute* dgAttribute;
	CCHDGPart* hdgPart;
	ObjectArray oaCells;
	KWDGCell* cell;
	int nCell;

	require(coclusteringDataGrid != NULL);

	// Entete
	ost << sKeyWordCells << "\n";
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = coclusteringDataGrid->GetAttributeAt(nAttribute);
		ost << dgAttribute->GetAttributeName() << "\t";
	}
	ost << "Frequency\n";

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

	// Affichage des cellules
	for (nCell = 0; nCell < oaCells.GetSize(); nCell++)
	{
		cell = cast(KWDGCell*, oaCells.GetAt(nCell));

		// On ignore les cellule d'effectif null (en principe, il n'y en a pas)
		if (cell->GetCellFrequency() > 0)
		{
			// Affichage des identifiants des parties de la cellule
			for (nAttribute = 0; nAttribute < cell->GetAttributeNumber(); nAttribute++)
			{
				hdgPart = cast(CCHDGPart*, cell->GetPartAt(nAttribute));
				ost << hdgPart->GetPartName() << "\t";
			}

			// Affichage des effectifs par classe cible
			ost << cell->GetCellFrequency() << "\n";
		}
	}
	ost << "\n";
}

void CCCoclusteringReport::WriteAnnotation(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost)
{
	boolean bAnnotationUsed;
	CCHDGAttribute* dgAttribute;
	int nAttribute;
	ObjectArray oaParts;
	int nPart;
	CCHDGPart* hdgPart;

	require(coclusteringDataGrid != NULL);

	// Parcours initial des attribut pour detecter l'utilisation d'annotations
	bAnnotationUsed = false;
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Exports de toutes les parties de la hierarchie en partant de la racine
		oaParts.SetSize(0);
		dgAttribute->ExportHierarchyParts(&oaParts);

		// Parcours des parties
		for (nPart = 0; nPart < oaParts.GetSize(); nPart++)
		{
			hdgPart = cast(CCHDGPart*, oaParts.GetAt(nPart));

			// Arret si detection d'annotation
			if (hdgPart->GetExpand() or hdgPart->GetSelected() or hdgPart->GetShortDescription() != "" or
			    hdgPart->GetDescription() != "")
			{
				bAnnotationUsed = true;
				break;
			}
		}
		if (bAnnotationUsed)
			break;
	}

	// Ecriture des section sur les annotation si necessaire
	if (bAnnotationUsed)
	{
		for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
		{
			dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

			// Entete
			ost << sKeyWordAnnotation << "\t" << dgAttribute->GetAttributeName() << "\n";
			ost << "Cluster	Expand	Selected	ShortDescription	Description\n";

			// Exports de toutes les parties de la hierarchie en partant de la racine
			oaParts.SetSize(0);
			dgAttribute->ExportHierarchyParts(&oaParts);

			// Tri pour respecter l'ordre d'affichage
			oaParts.SetCompareFunction(CCHDGPartCompareLeafRank);
			oaParts.Sort();

			// Affichage des parties
			for (nPart = 0; nPart < oaParts.GetSize(); nPart++)
			{
				hdgPart = cast(CCHDGPart*, oaParts.GetAt(nPart));

				// Caracteristiques de type annotation des parties
				ost << hdgPart->GetPartName() << "\t";
				if (hdgPart->GetExpand())
					ost << sKeyWordTrue << "\t";
				else
					ost << sKeyWordFalse << "\t";
				if (hdgPart->GetSelected())
					ost << sKeyWordTrue << "\t";
				else
					ost << sKeyWordFalse << "\t";
				ost << hdgPart->GetShortDescription() << "\t";
				ost << hdgPart->GetDescription() << "\n";
			}
			ost << "\n";
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

boolean CCCoclusteringReport::OpenInputCoclusteringReportFile(const ALString& sFileName)
{
	boolean bOk = true;

	require(fReport == NULL);
	require(sFileBuffer == NULL);

	// Initialisation des caracteristiques du fichier
	fReport = NULL;
	nLineIndex = 1;
	bEndOfLine = false;
	sReportFileName = sFileName;
	nHeaderInstanceNumber = 0;
	nHeaderCellNumber = 0;

	// Copie depuis HDFS si necessaire
	bOk = PLRemoteFileService::BuildInputWorkingFile(sReportFileName, sLocalFileName);

	// Tentative d'ouverture du fichier en mode binaire
	if (bOk)
		bOk = FileService::OpenInputBinaryFile(sLocalFileName, fReport);
	assert(fReport != NULL or not bOk);

	// Creation d'un buffer de lecture si necessaire
	if (bOk)
		sFileBuffer = NewCharArray(nMaxFieldSize + 1);
	else
	{
		sReportFileName = "";
		nLineIndex = 0;
	}
	return bOk;
}

void CCCoclusteringReport::CloseCoclusteringReportFile()
{
	require(fReport != NULL);

	FileService::CloseInputBinaryFile(sLocalFileName, fReport);

	// Si le fichier est sur HDFS, on supprime la copie locale
	PLRemoteFileService::CleanInputWorkingFile(sReportFileName, sLocalFileName);

	fReport = NULL;
	nLineIndex = 0;
	bEndOfLine = false;
	DeleteCharArray(sFileBuffer);
	sFileBuffer = NULL;
	sReportFileName = "";
	nHeaderCellNumber = 0;
	nHeaderInstanceNumber = 0;
}

/////////////////////////////////////////////////////////////////////////////////
// Implementation inspiree de la methode KWDataTableDriverTextFile::ReadNextField()
char* CCCoclusteringReport::ReadNextField()
{
	const char cFieldSeparator = '\t';
	char c;
	int i;
	boolean bFieldTooLong;

	require(fReport != NULL);
	require(sFileBuffer != NULL);

	// Arret immediat si fin de ligne ou de fichier
	sFileBuffer[0] = '\0';
	if (bEndOfLine or feof(fReport))
		return sFileBuffer;

	// Lecture des caracteres du token
	i = 0;
	bFieldTooLong = false;
	while (not feof(fReport))
	{
		c = (char)fgetc(fReport);

		// Test de fin de ligne
		// Attention: la fin de fichier n'est detectee qu'apres la derniere lecture de caractere
		// et dans ce cas, il n'y a pas de dernier caractere lu a memoriser
		bEndOfLine = c == '\n' or feof(fReport);

		// Test fin de champ
		if (bEndOfLine or c == cFieldSeparator)
		{
			// Fin du champ
			sFileBuffer[i] = '\0';

			// Supression des blancs en fin (TrimRight)
			while (i > 0)
			{
				i--;
				if (not iswspace(sFileBuffer[i]))
					break;
				sFileBuffer[i] = '\0';
			}
			return sFileBuffer;
		}

		// Test de depassement de longueur
		if (i == (int)nMaxFieldSize)
		{
			sFileBuffer[i] = '\0';
			bFieldTooLong = true;

			// Warning
			if (bFieldTooLong)
				AddWarning("Field too long");

			// Supression des blancs en fin (TrimRight)
			while (i > 0)
			{
				i--;
				if (not iswspace(sFileBuffer[i]))
					break;
				sFileBuffer[i] = '\0';
			}

			// On lit le fichier jusqu'a la fin du champ sans memorisation
			while (not feof(fReport))
			{
				c = (char)fgetc(fReport);

				// Test de fin de ligne ou de champ
				bEndOfLine = c == '\n' or feof(fReport);
				if (bEndOfLine or c == cFieldSeparator)
					return sFileBuffer;
			}
			assert(false);
		}

		// Mise a jour du champ sauf si caractere blanc (TrimLeft) en debut
		// et si caractere different de CR
		if (c != '\r' and (i > 0 or not iswspace(c)))
		{
			sFileBuffer[i] = c;
			i++;
		}
	}
	return sFileBuffer;
}

void CCCoclusteringReport::SkipLine()
{
	char c;

	require(fReport != NULL);

	// Saut d'une ligne
	while (not bEndOfLine)
	{
		c = (char)fgetc(fReport);
		bEndOfLine = c == '\n' or feof(fReport);
	}
	nLineIndex++;
	bEndOfLine = false;
}

boolean CCCoclusteringReport::IsEndOfLine()
{
	require(fReport != NULL);
	return bEndOfLine;
}

boolean CCCoclusteringReport::IsEndOfFile()
{
	require(fReport != NULL);
	return feof(fReport);
}

boolean CCCoclusteringReport::InternalReadJSONReport(CCHierarchicalDataGrid* coclusteringDataGrid, boolean bHeaderOnly)
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
	bOk = bOk and ReadJSONSummary(coclusteringDataGrid);
	bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
	bOk = bOk and ReadJSONDimensionSummaries(coclusteringDataGrid);

	// Lecture detailles
	if (not bHeaderOnly)
	{
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
		bOk = bOk and ReadJSONDimensionPartitions(coclusteringDataGrid);
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
		bOk = bOk and ReadJSONDimensionHierarchies(coclusteringDataGrid);
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
		bOk = bOk and ReadJSONCells(coclusteringDataGrid);
	}

	// Gestion des erreurs
	Global::DesactivateErrorFlowControl();
	return bOk;
}

boolean CCCoclusteringReport::ReadJSONSummary(CCHierarchicalDataGrid* coclusteringDataGrid)
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
	// CH IV Begin
	ALString sIdentifierVariable;
	// CH IV End

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

boolean CCCoclusteringReport::ReadJSONDimensionSummaries(CCHierarchicalDataGrid* coclusteringDataGrid)
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

		// CH IV Begin
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
		// CH IV End

		// Champs restants de l'attribut
		bOk = bOk and JSONTokenizer::ReadKeyIntValue("parts", true, nAttributePartNumber, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyIntValue("initialParts", true, nAttributeInitialPartNumber, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyIntValue("values", true, nAttributeValueNumber, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyDoubleValue("interest", true, dAttributeInterest, bIsEnd);
		bOk = bOk and JSONTokenizer::ReadKeyStringValue("description", sAttributeDescription, bIsEnd);

		// Valeur min et max dans le cas numerique
		if (bOk and nAttributeType == KWType::Continuous)
		{
			bOk = bOk and JSONTokenizer::ReadKeyContinuousValue("min", false, cMin, bIsEnd);
			bOk = bOk and JSONTokenizer::ReadKeyContinuousValue("max", false, cMax, bIsEnd);
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
				varPartAttribute = dgAttribute;

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
		bOk = bOk and JSONTokenizer::CheckObjectEnd("dimensionSummary", bIsEnd);
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
	}

	// CH IV Begin
	// Prise en compte d'un coclustering de type VarPart
	// CH IV Refactoring: potentiellement inutile apres la fin du refactoring
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
	// CH IV End

	return bOk;
}

boolean CCCoclusteringReport::ReadJSONDimensionPartitions(CCHierarchicalDataGrid* coclusteringDataGrid)
{
	boolean bOk = true;
	ObjectDictionary odCheckedParts;
	ALString sPartName;
	boolean bIsAttributeEnd;
	// CH IV Begin
	boolean bIsInnerAttributeEnd;
	// CH IV End
	boolean bIsPartEnd;
	int nAttributeIndex;
	int nPartIndex;
	CCHDGAttribute* dgAttribute;
	KWDGPart* dgPart;
	ALString sAttributeName;
	ALString sAttributeType;
	int nAttributeType;
	int nDefaultGroupIndex;
	ALString sValue;
	// CH IV Begin
	KWDGInnerAttributes* innerAttributes;
	KWDGAttribute* innerAttribute;
	int nInnerAttributeNumber;
	ObjectDictionary odInnerAttributesAllVarParts;
	ObjectDictionary odVarPartAttributeAllVarParts;
	KWDGInterval* domainBounds;
	int nInnerAttribute;
	// CH IV End
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
		if (bOk and KWType::ToString(KWType::GetSimpleCoclusteringType(dgAttribute->GetAttributeType())) !=
				sAttributeType)
		{
			JSONTokenizer::AddParseError(
			    "Read variable type " + sAttributeType + " instead of expected " +
			    KWType::ToString(KWType::GetSimpleCoclusteringType(dgAttribute->GetAttributeType())));
			bOk = false;
		}

		// Initialisation des parties
		if (bOk)
		{
			// Tableau d'intervalles ou de groupes de valeurs selon le type d'attribut
			if (dgAttribute->GetAttributeType() == KWType::Continuous)
				bOk = bOk and JSONTokenizer::ReadKeyArray("intervals");
			else if (dgAttribute->GetAttributeType() == KWType::Symbol)
				bOk = bOk and JSONTokenizer::ReadKeyArray("valueGroups");
			// CH IV Begin
			// Cas d'un attribut VarPart
			else if (dgAttribute->GetAttributeType() == KWType::VarPart)
			{
				// Tableau des variables internes dans l'attribut de type VarPart
				bOk = bOk and JSONTokenizer::ReadKeyArray("innerVariables");

				// Creation des attributs internes
				innerAttributes = new KWDGInnerAttributes;
				dgAttribute->SetInnerAttributes(innerAttributes);
				nInnerAttributeNumber = 0;

				// Boucle de lecture des attributs internes
				bIsInnerAttributeEnd = false;
				while (bOk and not bIsInnerAttributeEnd)
				{
					// Lecture de l'attribut interne
					innerAttribute = new KWDGAttribute;
					bOk = bOk and
					      ReadJSONInnerAttribute(innerAttribute, &odInnerAttributesAllVarParts);
					if (not bOk)
					{
						bOk = false;
						JSONTokenizer::AddParseError("Inner variable " +
									     innerAttribute->GetAttributeName() +
									     " not correctly specified");
					}

					// Test d'unicite de la variable interne
					if (bOk and dgAttribute->GetInnerAttributes()->LookupInnerAttribute(
							innerAttribute->GetAttributeName()) != NULL)
					{
						bOk = false;
						JSONTokenizer::AddParseError("Inner variable " +
									     innerAttribute->GetAttributeName() +
									     " used twice");
					}

					// Memorisation de l'attribut interne si ok
					if (bOk)
					{
						innerAttribute->SetOwnerAttributeName(dgAttribute->GetAttributeName());
						dgAttribute->GetInnerAttributes()->AddInnerAttribute(innerAttribute);
						nInnerAttributeNumber++;
					}
					// Destruction de l'attribut interne sinon
					else
					{
						delete innerAttribute;
						dgAttribute->SetInnerAttributes(NULL);
					}

					// Test si nouvel attribut interne
					bOk = bOk and JSONTokenizer::ReadArrayNext(bIsInnerAttributeEnd);

					// Arret si echec ou pas d'attribut suivant
					if (not bOk)
						break;
				}

				// Nettoyage des attributs internes si erreur
				if (not bOk)
				{
					dgAttribute->SetInnerAttributes(NULL);
					delete innerAttributes;
				}

				// Memorisation des informations sur les bornes des valeurs des attributs internes
				// numeriques
				if (bOk)
				{
					coclusteringDataGrid->GetInnerAttributeDomainBounds()->DeleteAll();
					for (nInnerAttribute = 0;
					     nInnerAttribute < dgAttribute->GetInnerAttributeNumber();
					     nInnerAttribute++)
					{
						innerAttribute = dgAttribute->GetInnerAttributes()->GetInnerAttributeAt(
						    nInnerAttribute);

						// Cas d'un attribut Continuous
						if (innerAttribute->GetAttributeType() == KWType::Continuous)
						{
							domainBounds = new KWDGInterval;
							domainBounds->SetLowerBound(innerAttribute->GetHeadPart()
											->GetInterval()
											->GetLowerBound());
							domainBounds->SetUpperBound(innerAttribute->GetTailPart()
											->GetInterval()
											->GetUpperBound());
							coclusteringDataGrid->GetInnerAttributeDomainBounds()->SetAt(
							    innerAttribute->GetAttributeName(), domainBounds);

							// On modifie les bornes des intervalles extremes de facon a
							// pouvoir produire les libelle avec -inf et +inf par
							// GetObjectLabel
							innerAttribute->GetHeadPart()->GetInterval()->SetLowerBound(
							    KWDGInterval::GetMinLowerBound());
							innerAttribute->GetTailPart()->GetInterval()->SetUpperBound(
							    KWDGInterval::GetMaxUpperBound());
						}
					}
				}

				// Fin du tableau des attributs internes
				bOk = bOk and JSONTokenizer::ReadExpectedToken(',');

				// Lecture des parties "valueGroups", groupes de parties de variables
				bOk = bOk and JSONTokenizer::ReadKeyArray("valueGroups");
			}
			// CH IV End

			// Lecture des parties
			dgPart = dgAttribute->GetHeadPart();
			bIsPartEnd = false;
			nPartIndex = 0;
			while (bOk and not bIsPartEnd)
			{
				// Acces a la partie si elle est disponible
				if (dgPart == NULL)
				{
					JSONTokenizer::AddParseError(
					    "Variable " + sAttributeName + " added part (index " +
					    IntToString(nPartIndex + 1) + ") beyond the expected number of parts (" +
					    IntToString(dgAttribute->GetPartNumber()) + ")");
					bOk = false;
					break;
				}
				else
					nPartIndex++;

				// Intervalles, groupes de valeurs ou groupe de parties de variables selon le type
				// d'attribut
				if (bOk)
				{
					if (dgAttribute->GetAttributeType() == KWType::Continuous)
						bOk = bOk and ReadJSONInterval(dgAttribute, dgPart);
					else if (dgAttribute->GetAttributeType() == KWType::Symbol)
						bOk = bOk and ReadJSONValueGroup(dgAttribute, dgPart);
					// CH IV Begin
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
						// Par contre, lors de la regeneration du fichier json, ces libellés
						// sont déduits des parties, et ne sont pas necessairement ceux qui
						// etaient en entree (mais la structure est preservee).
						bOk = bOk and ReadJSONVarPartAttributeValueGroup(
								  dgAttribute, dgPart, &odInnerAttributesAllVarParts,
								  &odVarPartAttributeAllVarParts);
					}
					// CH IV End
				}

				// Test si identifiant de partie unique
				if (bOk)
				{
					sPartName = cast(CCHDGPart*, dgPart)->GetPartName();
					if (odCheckedParts.Lookup(sPartName) != NULL)
					{
						JSONTokenizer::AddParseError("\"cluster\" " + sPartName +
									     " used twice");
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
				JSONTokenizer::AddParseError("Variable " + sAttributeName + " with " +
							     IntToString(nPartIndex) + " specified parts " +
							     ") below the expected number of parts (" +
							     IntToString(dgAttribute->GetPartNumber()) + ")");
				bOk = false;
				break;
			}

			// Erreur si le nombre de parties de variable collectees dans les groupes de partie de variable
			// est different du nombre de parties de variables specifie dans les variables internes
			if (bOk and dgAttribute->GetAttributeType() == KWType::VarPart and
			    odInnerAttributesAllVarParts.GetCount() != odVarPartAttributeAllVarParts.GetCount())
			{
				assert(odVarPartAttributeAllVarParts.GetCount() <
				       odInnerAttributesAllVarParts.GetCount());
				JSONTokenizer::AddParseError(
				    "VarPart variable " + sAttributeName + " contains " +
				    IntToString(odVarPartAttributeAllVarParts.GetCount()) +
				    " variable parts in its groups, below the expected number of variable parts (" +
				    IntToString(odInnerAttributesAllVarParts.GetCount()) +
				    ") specified globally in inner variables");
				bOk = false;
				break;
			}
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
					assert(dgPart->GetInterval()->GetUpperBound() ==
					       KWContinuous::GetMissingValue());

					// Modification du deuxieme intervalle
					dgAttribute->GetNextPart(dgPart);
					assert(dgPart->GetInterval()->GetLowerBound() !=
					       KWContinuous::GetMissingValue());
					dgPart->GetInterval()->SetLowerBound(KWContinuous::GetMissingValue());
				}
			}
			// Dans le cas categoriel
			else if (dgAttribute->GetAttributeType() == KWType::Symbol)
			{
				// Lecture de l'index du groupe par defaut avant la fin de l'objet
				bIsPartEnd = false;
				bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
				bOk = bOk and JSONTokenizer::ReadKeyIntValue("defaultGroupIndex", true,
									     nDefaultGroupIndex, bIsPartEnd);
				if (bOk and not bIsPartEnd)
				{
					bOk = false;
					JSONTokenizer::AddParseError(
					    "Read token " +
					    JSONTokenizer::GetTokenLabel(JSONTokenizer::GetLastToken()) +
					    " instead of expected  '}'");
					bOk = false;
				}

				// Finalisation de la partition en groupes de valeurs
				if (bOk and
				    (nDefaultGroupIndex < 0 or nDefaultGroupIndex >= dgAttribute->GetPartNumber()))
				{
					bOk = false;
					JSONTokenizer::AddParseError("Variable " + sAttributeName +
								     " with invalid default group index (" +
								     IntToString(nDefaultGroupIndex) + ")");
				}
				if (bOk)
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
					cast(CCHDGSymbolValueSet*, dgPart->GetValueSet())
					    ->AddSymbolValue(Symbol::GetStarValue());
				}
			}
			// CH IV Begin
			// Dans le cas VarPart
			else if (dgAttribute->GetAttributeType() == KWType::VarPart)
			{
				// Lecture de la fin de l'objet
				bOk = bOk and JSONTokenizer::ReadExpectedToken('}');
			}
			// CH IV End
		}

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

	// CH IV Begin
	// Nettoyage du dictionnaire temporaire des PV
	odInnerAttributesAllVarParts.RemoveAll();
	// CH IV End

	return bOk;
}

boolean CCCoclusteringReport::ReadJSONInterval(CCHDGAttribute* dgAttribute, KWDGPart* dgPart)
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
		// Lecture des bonres inf et sup, avec tableau de deux valeurs
		else if (nToken == JSONTokenizer::Number)
		{
			// Acces a la valeur de la borne inf, quyi vient d'etre lue
			cLowerBound = KWContinuous::DoubleToContinuous(JSONTokenizer::GetTokenNumberValue());

			// Lecture de la borne sup
			bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
			bOk = bOk and JSONTokenizer::ReadContinuousValue(false, cUpperBound);
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

boolean CCCoclusteringReport::ReadJSONValueGroup(CCHDGAttribute* dgAttribute, KWDGPart* dgPart)
{
	boolean bOk = true;
	ObjectDictionary odChekedValues;
	boolean bIsEnd;
	CCHDGSymbolValueSet* dgValueSet;
	CCHDGSymbolValue* dgValue;
	CCHDGPart* hdgPart;
	ALString sClusterName;
	ALString sValue;
	int nValueFrequency;
	double dValueTypicality;
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
				JSONTokenizer::AddParseError("\"values\" contains a duplicate values (" + sValue + ")");
				bOk = false;
			}
			else
			{
				odChekedValues.SetAt(sValue, &odChekedValues), svValues.Add(sValue);
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
	bOk = bOk and JSONTokenizer::ReadKeyArray("valueTypicalities");
	bIsEnd = false;
	dValueTypicality = 0;
	while (bOk and not bIsEnd)
	{
		bOk = bOk and JSONTokenizer::ReadDoubleValue(false, dValueTypicality);

		// Tolerance pour les typicalite negatives
		if (dValueTypicality < 0)
			AddWarning(sTmp + "Typicality (" + DoubleToString(dValueTypicality) +
				   ") less than 0 for variable " + dgAttribute->GetAttributeName() +
				   " in \"valueTypicalities\" line " +
				   IntToString(JSONTokenizer::GetCurrentLineIndex()));
		// Erreur pour les typicalite supereures a 1
		else if (dValueTypicality > 1)
		{
			AddError(sTmp + "Typicality (" + DoubleToString(dValueTypicality) +
				 ") greater than 1 for variable " + dgAttribute->GetAttributeName() +
				 " in \"valueTypicalities\" line " + IntToString(JSONTokenizer::GetCurrentLineIndex()));
			break;
		}
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
		if (bOk)
			dvValueTypicalities.Add(dValueTypicality);
	}
	if (bOk and svValues.GetSize() != dvValueTypicalities.GetSize())
	{
		JSONTokenizer::AddParseError(
		    "Vector \"valueTypicalities\" should be of same size as vector \"values\"");
		bOk = false;
	}

	// Fin de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('}');

	// Memorisation des informations sur le groupe de valeurs
	if (bOk)
	{
		hdgPart = cast(CCHDGPart*, dgPart);
		dgValueSet = cast(CCHDGSymbolValueSet*, dgPart->GetValueSet());
		hdgPart->SetPartName(sClusterName);
		hdgPart->SetShortDescription("");
		hdgPart->SetDescription("");

		// Memorisation des valeurs
		for (i = 0; i < svValues.GetSize(); i++)
		{
			dgValue = cast(CCHDGSymbolValue*, dgValueSet->AddSymbolValue((Symbol)svValues.GetAt(i)));
			dgValue->SetValueFrequency(ivValueFrequencies.GetAt(i));
			dgValue->SetTypicality(dvValueTypicalities.GetAt(i));
		}
	}
	return bOk;
}

// CH IV Begin

boolean CCCoclusteringReport::ReadJSONInnerAttribute(KWDGAttribute* innerAttribute,
						     ObjectDictionary* odInnerAttributesAllVarParts)
{
	boolean bOk = true;
	boolean bIsEnd;
	ALString sAttributeName;
	ALString sAttributeType;
	int nAttributeType;
	ALString sValue;
	StringVector svValues;
	DoubleVector dvValueTypicalities;
	int i;
	ALString sTmp;
	KWDGPart* dgPart;

	require(innerAttribute != NULL);
	require(odInnerAttributesAllVarParts != NULL);

	// Debut de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('{');

	// Nom de l'attribut
	bIsEnd = false;
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("variable", sAttributeName, bIsEnd);
	if (bOk and sAttributeName == "")
	{
		bOk = false;
		JSONTokenizer::AddParseError("Name of inner variable should not be empty");
	}

	// Type de l'attribut
	bOk = bOk and JSONTokenizer::ReadKeyStringValue("type", sAttributeType, bIsEnd);
	nAttributeType = KWType::ToType(sAttributeType);
	if (bOk and not(KWType::IsSimple(nAttributeType)))
	{
		bOk = false;
		JSONTokenizer::AddParseError("Type of variable " + sAttributeName + " (" + sAttributeType +
					     ") should be Numerical or Categorical");
	}

	// Initialisation, notamment du type (necessaire pour la creation des parties de l'attribut)
	if (bOk)
	{
		innerAttribute->SetAttributeName(sAttributeName);
		innerAttribute->SetAttributeType(nAttributeType);
	}

	// Partition de l'attribut
	bOk = bOk and JSONTokenizer::ReadKeyArray("partition");
	// Cas d'un attribut Continuous
	if (nAttributeType == KWType::Continuous)
	{
		bOk = bOk and ReadJSONInnerAttributeIntervals(innerAttribute);
	}
	// Cas d'un attribut Categorial
	else
	{
		bOk = bOk and ReadJSONInnerAttributeValueGroups(innerAttribute);
	}

	// Tableau des labels des parties de variables
	bOk = bOk and JSONTokenizer::ReadKeyArray("varPartIds");
	bIsEnd = false;
	while (bOk and not bIsEnd)
	{
		bOk = bOk and JSONTokenizer::ReadStringValue(sValue);
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
		if (bOk)
			svValues.Add(sValue);
	}

	if (bOk and svValues.GetSize() != innerAttribute->GetPartNumber())
	{
		JSONTokenizer::AddParseError("Vector \"varPartIds\" should be of same size as vector \"partition\"");
		bOk = false;
	}

	// Fin de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('}');

	// Memorisation des informations sur l'attribut
	if (bOk)
	{
		// Memorisation de chaque partie
		dgPart = innerAttribute->GetHeadPart();
		for (i = 0; i < svValues.GetSize(); i++)
		{
			// Test d'unicite du libelle de partie parmi l'ensembe de tous les libelles de partie
			if (odInnerAttributesAllVarParts->Lookup(svValues.GetAt(i)) != NULL)
			{
				JSONTokenizer::AddParseError("Vector \"varPartIds\" contains value \"" +
							     svValues.GetAt(i) + "\" already used previously");
				bOk = false;
				break;
			}

			// Memorisation de la PV avec son label dans un dictionnaire temporaire
			odInnerAttributesAllVarParts->SetAt(svValues.GetAt(i), dgPart);

			// Partie suivante
			innerAttribute->GetNextPart(dgPart);
		}
	}
	return bOk;
}

boolean CCCoclusteringReport::ReadJSONInnerAttributeIntervals(KWDGAttribute* innerAttribute)
{
	boolean bOk = true;
	boolean bPartitionIsEnd;
	KWDGInterval* dgInterval;
	ALString sClusterName;
	Continuous cLowerBound;
	Continuous cUpperBound;
	KWDGPart* dgPart;

	require(innerAttribute != NULL);
	require(innerAttribute->GetAttributeName() != "");
	require(innerAttribute->GetAttributeType() == KWType::Continuous);

	// Initialisations
	cLowerBound = 0;
	cUpperBound = 0;

	// Lecture des intervalles
	bPartitionIsEnd = false;
	while (bOk and not bPartitionIsEnd)
	{
		// Creation de la partie courante
		dgPart = innerAttribute->AddPart();

		// Lecture de la borne inf
		bOk = bOk and JSONTokenizer::ReadExpectedToken('[');
		bOk = bOk and JSONTokenizer::ReadContinuousValue(false, cLowerBound);

		// Lecture de la borne sup
		bOk = bOk and JSONTokenizer::ReadExpectedToken(',');
		bOk = bOk and JSONTokenizer::ReadContinuousValue(false, cUpperBound);
		bOk = bOk and JSONTokenizer::ReadExpectedToken(']');
		bOk = bOk and JSONTokenizer::ReadArrayNext(bPartitionIsEnd);

		// Memorisation des informations sur la PV intervalle
		if (bOk)
		{
			dgInterval = dgPart->GetInterval();
			dgInterval->SetLowerBound(cLowerBound);
			dgInterval->SetUpperBound(cUpperBound);
		}
	}

	// Test si section suivante dans le json
	bOk = bOk and JSONTokenizer::ReadExpectedToken(',');

	return bOk;
}

boolean CCCoclusteringReport::ReadJSONInnerAttributeValueGroups(KWDGAttribute* innerAttribute)
{
	boolean bOk = true;
	boolean bIsEnd;
	ALString sClusterName;
	ALString sValue;
	IntVector ivValueFrequencies;
	DoubleVector dvValueTypicalities;
	ALString sTmp;
	boolean bPartitionIsEnd;
	KWDGPart* dgPart;

	require(innerAttribute != NULL);
	require(innerAttribute->GetAttributeName() != "");
	require(innerAttribute->GetAttributeType() == KWType::Symbol);

	// Lecture des groupes de valeurs
	bPartitionIsEnd = false;
	while (bOk and not bPartitionIsEnd)
	{
		// Creation de la partie courante
		dgPart = innerAttribute->AddPart();

		// Lecture des valeurs
		bIsEnd = false;
		bOk = bOk and JSONTokenizer::ReadExpectedToken('[');
		while (bOk and not bIsEnd)
		{
			bOk = bOk and JSONTokenizer::ReadStringValue(sValue);
			if (bOk)
			{
				// Traitement particulier de la Star value
				if (sValue == Symbol::GetStarValue().GetValue())
					dgPart->GetSymbolValueSet()->AddSymbolValue(Symbol::GetStarValue());
				else
					dgPart->GetSymbolValueSet()->AddSymbolValue((Symbol)sValue);
			}
			bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
		}
		bOk = bOk and JSONTokenizer::ReadArrayNext(bPartitionIsEnd);
	}

	// Test si section suivante dans le json
	bOk = bOk and JSONTokenizer::ReadExpectedToken(',');

	return bOk;
}

boolean CCCoclusteringReport::ReadJSONVarPartAttributeValueGroup(CCHDGAttribute* varPartAttribute, KWDGPart* dgPart,
								 const ObjectDictionary* odInnerAttributesAllVarParts,
								 ObjectDictionary* odVarPartAttributeAllVarParts)
{
	boolean bOk = true;
	boolean bIsEnd;
	CCHDGVarPartSet* dgVarPartSet;
	CCHDGVarPartValue* dgVarPartValue;
	CCHDGPart* hdgPart;
	ALString sClusterName;
	ALString sValue;
	int nValueFrequency;
	double dValueTypicality;
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
				JSONTokenizer::AddParseError("Vector \"values\" of VarPart variable " +
							     varPartAttribute->GetAttributeName() +
							     " contains variable part \"" + sValue +
							     "\" which was not specified among all the \"varPartIds\" "
							     "vectors in the \"innerVariables\" section");
				bOk = false;
				break;
			}

			// Verification de l'unicite des partie de variables dans l'ensemble des groupes de l'attribut
			// de type VarPart
			dgCheckedVarPart = cast(KWDGPart*, odVarPartAttributeAllVarParts->Lookup(sValue));
			if (dgCheckedVarPart != NULL)
			{
				JSONTokenizer::AddParseError("Vector \"values\" of VarPart variable " +
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
	bOk = bOk and JSONTokenizer::ReadKeyArray("valueTypicalities");
	bIsEnd = false;
	dValueTypicality = 0;
	while (bOk and not bIsEnd)
	{
		bOk = bOk and JSONTokenizer::ReadDoubleValue(false, dValueTypicality);

		// Tolerance pour les typicalite negatives
		if (dValueTypicality < 0)
			AddWarning(sTmp + "Typicality (" + DoubleToString(dValueTypicality) +
				   ") less than 0 for variable " + varPartAttribute->GetAttributeName() +
				   " in \"valueTypicalities\" line " +
				   IntToString(JSONTokenizer::GetCurrentLineIndex()));
		// Erreur pour les typicalite supereures a 1
		else if (dValueTypicality > 1)
		{
			AddError(sTmp + "Typicality (" + DoubleToString(dValueTypicality) +
				 ") greater than 1 for variable " + varPartAttribute->GetAttributeName() +
				 " in \"valueTypicalities\" line " + IntToString(JSONTokenizer::GetCurrentLineIndex()));
			break;
		}
		bOk = bOk and JSONTokenizer::ReadArrayNext(bIsEnd);
		if (bOk)
			dvValueTypicalities.Add(dValueTypicality);
	}
	if (bOk and svValues.GetSize() != dvValueTypicalities.GetSize())
	{
		JSONTokenizer::AddParseError(
		    "Vector \"valueTypicalities\" should be of same size as vector \"values\"");
		bOk = false;
	}

	// Fin de l'objet
	bOk = bOk and JSONTokenizer::ReadExpectedToken('}');

	// Memorisation des informations sur le groupe de parties de variable
	if (bOk)
	{
		hdgPart = cast(CCHDGPart*, dgPart);
		dgVarPartSet = cast(CCHDGVarPartSet*, dgPart->GetVarPartSet());
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
			dgVarPartValue = cast(CCHDGVarPartValue*, dgVarPartSet->AddVarPart(dgVarPart));

			// Memorisation de l'effectif et de la typicite
			dgVarPartValue->SetValueFrequency(ivValueFrequencies.GetAt(i));
			dgVarPartValue->SetTypicality(dvValueTypicalities.GetAt(i));
		}
	}

	return bOk;
}
// CH IV End

boolean CCCoclusteringReport::ReadJSONDimensionHierarchies(CCHierarchicalDataGrid* coclusteringDataGrid)
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
		if (bOk and KWType::ToString(KWType::GetSimpleCoclusteringType(dgAttribute->GetAttributeType())) !=
				sAttributeType)
		{
			JSONTokenizer::AddParseError(
			    "Read variable type " + sAttributeType + " instead of expected " +
			    KWType::ToString(KWType::GetSimpleCoclusteringType(dgAttribute->GetAttributeType())));
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

boolean CCCoclusteringReport::ReadJSONCells(CCHierarchicalDataGrid* coclusteringDataGrid)
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

void CCCoclusteringReport::InternalWriteJSONReport(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON)
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
	WriteJSONSummary(coclusteringDataGrid, fJSON);
	WriteJSONDimensionSummaries(coclusteringDataGrid, fJSON);
	WriteJSONDimensionPartitions(coclusteringDataGrid, fJSON);
	WriteJSONDimensionHierarchies(coclusteringDataGrid, fJSON);
	WriteJSONCells(coclusteringDataGrid, fJSON);
	fJSON->EndObject();
}

void CCCoclusteringReport::WriteJSONSummary(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON)
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

void CCCoclusteringReport::WriteJSONDimensionSummaries(const CCHierarchicalDataGrid* coclusteringDataGrid,
						       JSONFile* fJSON)
{
	int nAttribute;
	CCHDGAttribute* dgAttribute;
	int nValueNumber;

	require(coclusteringDataGrid != NULL);
	require(fJSON != NULL);

	// Parcours des attributs pour la section des dimensions
	fJSON->BeginKeyArray("dimensionSummaries");
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Nombre initial de valeurs
		nValueNumber = dgAttribute->GetInitialValueNumber();

		// Caracteristique des attributs
		// Le nombre de valeur est diminuer en externe de 1 pour tenir compte de de la StarValue en interne
		fJSON->BeginObject();
		fJSON->WriteKeyString("name", dgAttribute->GetAttributeName());
		if (dgAttribute->GetAttributeType() == KWType::VarPart)
			fJSON->WriteKeyBoolean("isVarPart", true);
		fJSON->WriteKeyString(
		    "type", KWType::ToString(KWType::GetSimpleCoclusteringType(dgAttribute->GetAttributeType())));
		fJSON->WriteKeyInt("parts", dgAttribute->GetPartNumber());
		fJSON->WriteKeyInt("initialParts", dgAttribute->GetInitialPartNumber());
		fJSON->WriteKeyInt("values", nValueNumber);
		fJSON->WriteKeyDouble("interest", dgAttribute->GetInterest());
		fJSON->WriteKeyString("description", dgAttribute->GetDescription());
		if (KWFrequencyTable::GetWriteGranularityAndGarbage())
			fJSON->WriteKeyBoolean("garbage", (dgAttribute->GetGarbageModalityNumber() > 0));
		if (dgAttribute->GetAttributeType() == KWType::Continuous)
		{
			fJSON->WriteKeyDouble("min", dgAttribute->GetMin());
			fJSON->WriteKeyDouble("max", dgAttribute->GetMax());
		}
		fJSON->EndObject();
	}
	fJSON->EndArray();
}

void CCCoclusteringReport::WriteJSONDimensionPartitions(const CCHierarchicalDataGrid* coclusteringDataGrid,
							JSONFile* fJSON)
{
	CCHDGAttribute* dgAttribute;
	int nAttribute;
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	CCHDGSymbolValueSet* hdgValueSet;
	KWDGValue* dgValue;
	CCHDGSymbolValue* hdgValue;
	int nIndex;
	int nDefaultGroupIndex;
	KWDGInterval* dgInterval;
	// CH IV Begin
	CCHDGVarPartSet* hdgVarPartSet;
	KWDGValue* dgVarPartValue;
	CCHDGVarPartValue* hdgVarPartValue;
	KWDGAttribute* innerAttribute;
	// CH IV End

	require(coclusteringDataGrid != NULL);
	require(fJSON != NULL);

	// Parcours des attributs
	fJSON->BeginKeyArray("dimensionPartitions");
	for (nAttribute = 0; nAttribute < coclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = cast(CCHDGAttribute*, coclusteringDataGrid->GetAttributeAt(nAttribute));

		// Debut de l'objet
		fJSON->BeginObject();
		fJSON->WriteKeyString("name", dgAttribute->GetAttributeName());
		fJSON->WriteKeyString(
		    "type", KWType::ToString(KWType::GetSimpleCoclusteringType(dgAttribute->GetAttributeType())));

		// Traitement des attributs numeriques
		if (dgAttribute->GetAttributeType() == KWType::Continuous)
		{
			// Parcours des parties
			fJSON->BeginKeyArray("intervals");
			nIndex = 0;
			dgPart = dgAttribute->GetHeadPart();
			while (dgPart != NULL)
			{
				hdgPart = cast(CCHDGPart*, dgPart);
				dgInterval = cast(KWDGInterval*, hdgPart->GetInterval());

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
					if (dgPart == dgAttribute->GetTailPart())
						fJSON->WriteContinuous(dgAttribute->GetMax());
					else
						fJSON->WriteContinuous(dgInterval->GetUpperBound());

					// Index suivant d'intervalle non missing
					nIndex++;
				}
				fJSON->EndList();
				fJSON->EndObject();

				// Partie suivante
				dgAttribute->GetNextPart(dgPart);
			}
			fJSON->EndArray();
		}

		// Traitement des attributs categoriels
		if (dgAttribute->GetAttributeType() == KWType::Symbol)
		{
			// Parcours des parties
			fJSON->BeginKeyArray("valueGroups");
			nDefaultGroupIndex = -1;
			nIndex = 0;
			dgPart = dgAttribute->GetHeadPart();
			while (dgPart != NULL)
			{
				hdgPart = cast(CCHDGPart*, dgPart);
				hdgValueSet = cast(CCHDGSymbolValueSet*, hdgPart->GetValueSet());

				// Memorisation de l'index du groupe par defaut
				if (hdgValueSet->IsDefaultPart())
					nDefaultGroupIndex = nIndex;
				nIndex++;

				// Parcours des valeurs, sauf si effectif null (cas de la StarValue)
				fJSON->BeginObject();
				fJSON->WriteKeyString("cluster", hdgPart->GetPartName());
				fJSON->BeginKeyList("values");
				dgValue = hdgValueSet->GetHeadValue();
				while (dgValue != NULL)
				{
					hdgValue = cast(CCHDGSymbolValue*, dgValue);
					if (hdgValue->GetValueFrequency() > 0)
						fJSON->WriteString(hdgValue->GetSymbolValue().GetValue());
					hdgValueSet->GetNextValue(dgValue);
				}
				fJSON->EndList();

				// Effectifs des effectifs
				fJSON->BeginKeyList("valueFrequencies");
				dgValue = hdgValueSet->GetHeadValue();
				while (dgValue != NULL)
				{
					hdgValue = cast(CCHDGSymbolValue*, dgValue);
					if (hdgValue->GetValueFrequency() > 0)
						fJSON->WriteInt(hdgValue->GetValueFrequency());
					hdgValueSet->GetNextValue(dgValue);
				}
				fJSON->EndList();

				// Typicalite des valeurs
				fJSON->BeginKeyList("valueTypicalities");
				dgValue = hdgValueSet->GetHeadValue();
				while (dgValue != NULL)
				{
					hdgValue = cast(CCHDGSymbolValue*, dgValue);
					if (hdgValue->GetValueFrequency() > 0)
						fJSON->WriteDouble(hdgValue->GetTypicality());
					hdgValueSet->GetNextValue(dgValue);
				}
				fJSON->EndList();
				fJSON->EndObject();

				// Partie suivante
				dgAttribute->GetNextPart(dgPart);
			}
			fJSON->EndArray();

			// Index du groupe par defaut
			assert(nDefaultGroupIndex >= 0);
			fJSON->WriteKeyInt("defaultGroupIndex", nDefaultGroupIndex);
		}
		// CH IV Begin
		// Traitement des attributs de type VarPart
		if (dgAttribute->GetAttributeType() == KWType::VarPart)
		{
			// Parcours des variables de l'attribut
			fJSON->BeginKeyArray("innerVariables");
			nIndex = 0;
			for (nIndex = 0; nIndex < dgAttribute->GetInnerAttributeNumber(); nIndex++)
			{
				// Extraction de l'attribut
				innerAttribute = dgAttribute->GetInnerAttributeAt(nIndex);
				assert(innerAttribute != NULL);

				// Partition de la variable interne
				fJSON->BeginObject();
				WriteJSONInnerAttributePartition(coclusteringDataGrid, innerAttribute, fJSON);
				fJSON->EndObject();
			}
			fJSON->EndArray();

			// Parcours des groupes de parties de variable
			fJSON->BeginKeyArray("valueGroups");
			// nDefaultGroupIndex = -1;
			// nIndex = 0;
			dgPart = dgAttribute->GetHeadPart();
			while (dgPart != NULL)
			{
				hdgPart = cast(CCHDGPart*, dgPart);
				hdgVarPartSet = cast(CCHDGVarPartSet*, hdgPart->GetVarPartSet());

				// Memorisation de l'index du groupe par defaut
				// CH IV Refactoring: faut-il gere un default groupe index???
				// if (hdgVarPartSet->IsDefaultPart())
				//	nDefaultGroupIndex = nIndex;
				// nIndex++;

				// Parcours des parties de variable, sauf si effectif null
				fJSON->BeginObject();
				fJSON->WriteKeyString("cluster", hdgPart->GetPartName());
				fJSON->BeginKeyList("values");
				dgVarPartValue = hdgVarPartSet->GetHeadValue();
				while (dgVarPartValue != NULL)
				{
					hdgVarPartValue = cast(CCHDGVarPartValue*, dgVarPartValue);

					if (hdgVarPartValue->GetValueFrequency() > 0)
						fJSON->WriteString(hdgVarPartValue->GetVarPart()->GetVarPartLabel());

					hdgVarPartSet->GetNextValue(dgVarPartValue);
				}
				fJSON->EndList();

				// Effectifs des valeurs de type partie de variable
				fJSON->BeginKeyList("valueFrequencies");
				dgVarPartValue = hdgVarPartSet->GetHeadValue();
				while (dgVarPartValue != NULL)
				{
					hdgVarPartValue = cast(CCHDGVarPartValue*, dgVarPartValue);
					if (hdgVarPartValue->GetValueFrequency() > 0)
						fJSON->WriteInt(hdgVarPartValue->GetValueFrequency());
					hdgVarPartSet->GetNextValue(dgVarPartValue);
				}
				fJSON->EndList();

				// Typicalite des valeurs de type partie de variable
				fJSON->BeginKeyList("valueTypicalities");
				dgVarPartValue = hdgVarPartSet->GetHeadValue();
				while (dgVarPartValue != NULL)
				{
					hdgVarPartValue = cast(CCHDGVarPartValue*, dgVarPartValue);
					if (hdgVarPartValue->GetValueFrequency() > 0)
						fJSON->WriteDouble(hdgVarPartValue->GetTypicality());
					hdgVarPartSet->GetNextValue(dgVarPartValue);
				}
				fJSON->EndList();
				fJSON->EndObject();

				// Partie suivante
				dgAttribute->GetNextPart(dgPart);
			}
			fJSON->EndArray();

			// Index du groupe par defaut
			// assert(nDefaultGroupIndex >= 0);
			// fJSON->WriteKeyInt("defaultGroupIndex", nDefaultGroupIndex);
		}
		// CH IV End
		// Fin de l'objet
		fJSON->EndObject();
	}
	fJSON->EndArray();
}

// CH IV Begin
void CCCoclusteringReport::WriteJSONInnerAttributePartition(const CCHierarchicalDataGrid* coclusteringDataGrid,
							    KWDGAttribute* innerAttribute, JSONFile* fJSON)
{
	KWDGPart* currentPart;
	KWDGValue* currentValue;
	KWDGInterval* domainBounds;
	ObjectArray oaValues;
	ObjectArray oaAllValues;
	KWDGValue* dgValue;
	int nValue;

	require(coclusteringDataGrid != NULL);
	require(innerAttribute != NULL);
	require(KWType::IsSimple(innerAttribute->GetAttributeType()));
	require(fJSON != NULL);

	// Entete
	fJSON->WriteKeyString("variable", innerAttribute->GetAttributeName());
	fJSON->WriteKeyString("type", KWType::ToString(innerAttribute->GetAttributeType()));

	// Partition de la variable
	// Cas d'un attribut Continuous
	if (innerAttribute->GetAttributeType() == KWType::Continuous)
	{
		fJSON->BeginKeyArray("partition");

		// Acces aux bornes du domaine
		domainBounds = cast(KWDGInterval*, coclusteringDataGrid->GetInnerAttributeDomainBounds()->Lookup(
						       innerAttribute->GetAttributeName()));
		assert(domainBounds != NULL);

		// Parcours des parties de l'attribut
		currentPart = innerAttribute->GetHeadPart();
		while (currentPart != NULL)
		{
			fJSON->BeginList();

			// Borne inf, ou min pour le premier intervalle non Missing
			if (currentPart == innerAttribute->GetHeadPart())
				fJSON->WriteContinuous(domainBounds->GetLowerBound());
			else
				fJSON->WriteContinuous(currentPart->GetInterval()->GetLowerBound());

			// Cas de + inf
			if (currentPart == innerAttribute->GetTailPart())
				fJSON->WriteContinuous(domainBounds->GetUpperBound());
			else
				fJSON->WriteContinuous(currentPart->GetInterval()->GetUpperBound());
			fJSON->EndList();

			// Partie suivante
			innerAttribute->GetNextPart(currentPart);
		}
		fJSON->EndArray();
	}

	// Sinon, attribut Categoriel
	else
	{
		fJSON->BeginKeyList("partition");
		currentPart = innerAttribute->GetHeadPart();

		// Parcours des parties de l'attribut
		while (currentPart != NULL)
		{
			// Parcours des valeurs de la partie
			fJSON->BeginList();
			currentValue = currentPart->GetValueSet()->GetHeadValue();
			while (currentValue != NULL)
			{
				fJSON->WriteString(currentValue->GetSymbolValue().GetValue());

				// Valeur suivante
				currentPart->GetValueSet()->GetNextValue(currentValue);
			}
			fJSON->EndList();

			// Partie suivante
			innerAttribute->GetNextPart(currentPart);
		}
		fJSON->EndList();
	}

	// Identifiant des parties de variable utilises pour decrire les clusters de parties de variable
	fJSON->BeginKeyList("varPartIds");

	// Parcours des parties de l'attribut
	currentPart = innerAttribute->GetHeadPart();
	while (currentPart != NULL)
	{
		// Ecriture de l'identifiant de la partie de variable, sous la forme d'un libelle lisible dont l'unicite
		// est assuree par la methode GetObjectLabel des parties de variables
		fJSON->WriteString(currentPart->GetVarPartLabel());

		// Partie suivante
		innerAttribute->GetNextPart(currentPart);
	}
	fJSON->EndList();

	// Ecriture des valeurs et de leur effectif dans les cas des attribut Symbol
	// CH IV Refactoring: il faut decider d'un formatbd'export des valeurs et leur effectif,
	// soit global, soit par groupe de facon similaire a l'existant
	// En attendant, on ne fait rien
	// Il faudra egalement implementer la relecture de ces valeurs lors de la lecture d'un json
	boolean bExportValues = false;
	if (bExportValues and innerAttribute->GetAttributeType() == KWType::Symbol)
	{
		// Export de toutes les valeurs des parties de l'attribut
		currentPart = innerAttribute->GetHeadPart();
		while (currentPart != NULL)
		{
			currentPart->GetValueSet()->ExportValues(&oaValues);
			oaAllValues.InsertObjectArrayAt(oaAllValues.GetSize(), &oaValues);
			oaValues.SetSize(0);

			// Partie suivante
			innerAttribute->GetNextPart(currentPart);
		}

		// Tri des valeurs
		oaAllValues.SetCompareFunction(KWDGValueCompareDecreasingFrequency);
		oaAllValues.Sort();

		// Ecriture des valeurs
		fJSON->BeginKeyList("values");
		for (nValue = 0; nValue < oaAllValues.GetSize(); nValue++)
		{
			dgValue = cast(KWDGValue*, oaAllValues.GetAt(nValue));
			if (dgValue->GetValueFrequency() > 0)
				fJSON->WriteString(dgValue->GetSymbolValue().GetValue());
		}
		fJSON->EndList();

		// Effectifs des effectifs
		fJSON->BeginKeyList("valueFrequencies");
		for (nValue = 0; nValue < oaAllValues.GetSize(); nValue++)
		{
			dgValue = cast(KWDGValue*, oaAllValues.GetAt(nValue));
			if (dgValue->GetValueFrequency() > 0)
				fJSON->WriteInt(dgValue->GetValueFrequency());
		}
		fJSON->EndList();
	}
}
// CH IV End

void CCCoclusteringReport::WriteJSONDimensionHierarchies(const CCHierarchicalDataGrid* coclusteringDataGrid,
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
		    "type", KWType::ToString(KWType::GetSimpleCoclusteringType(dgAttribute->GetAttributeType())));

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

void CCCoclusteringReport::WriteJSONCells(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON)
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
