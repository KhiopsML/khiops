// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHCommandLine.h"

MHCommandLine::MHCommandLine()
{
	InitializeParameterSettings();
}

MHCommandLine::~MHCommandLine() {}

boolean MHCommandLine::ComputeHistogram(int argc, char** argv)
{
	boolean bOk = true;
	MHTruncationDiscretizerHistogramMODL_fp histogramBuilder;
	ContinuousVector* cvLowerValues;
	ContinuousVector* cvUpperValues;
	IntVector* ivFrequencies;

	// Initialisation des parametres
	sDataFileName = InitializeParameters(argc, argv);
	bOk = sDataFileName != "";

	// Lecture des donnees
	if (bOk)
	{
		// Recopie du parametrage
		histogramBuilder.GetHistogramSpec()->CopyFrom(&histogramSpec);

		// Lecture des bins
		bOk = ReadBins(cvLowerValues, cvUpperValues, ivFrequencies);

		// Construction de l'histogramme si ok
		if (bOk)
			histogramBuilder.ComputeHistogramFromBins(cvLowerValues, cvUpperValues, ivFrequencies);

		// Nettoyage
		if (cvLowerValues != NULL)
			delete cvLowerValues;
		if (cvUpperValues != NULL)
			delete cvUpperValues;
		if (ivFrequencies != NULL)
			delete ivFrequencies;
	}

	// Nettoyage
	sDataFileName = "";
	return bOk;
}

const ALString MHCommandLine::GetClassLabel() const
{
	return "genumfp";
}

const ALString MHCommandLine::GetObjectLabel() const
{
	return sDataFileName;
}

boolean MHCommandLine::ReadBins(ContinuousVector*& cvLowerValues, ContinuousVector*& cvUpperValues,
				IntVector*& ivFrequencies)
{
	boolean bOk = true;
	int nFileFormat;
	InputBufferedFile inputFile;
	longint lFilePos;
	boolean bLineTooLong;
	longint lRecordIndex;
	longint lLineNumber;
	boolean bEndOfLine;
	int nField;
	int nFieldError;
	longint lCumulatedFrequency;
	int nFrequency;
	Continuous cLowerValue;
	Continuous cUpperValue;
	ObjectArray oaBins;
	MHBin* bin;
	int i;
	ALString sTmp;

	require(sDataFileName != "");

	// Initialisation des resultats
	nFileFormat = histogramSpec.GetFileFormat();
	lCumulatedFrequency = 0;
	cvLowerValues = NULL;
	cvUpperValues = NULL;
	ivFrequencies = NULL;
	if (nFileFormat == 1)
	{
		cvUpperValues = new ContinuousVector;
	}
	else if (nFileFormat == 2)
	{
		cvUpperValues = new ContinuousVector;
		ivFrequencies = new IntVector;
	}
	else if (nFileFormat == 3)
	{
		cvLowerValues = new ContinuousVector;
		cvUpperValues = new ContinuousVector;
		ivFrequencies = new IntVector;
	}

	// Initialisation du fichier a lire
	inputFile.SetFileName(sDataFileName);
	inputFile.SetHeaderLineUsed(false);
	inputFile.SetBufferSize(InputBufferedFile::nDefaultBufferSize);

	// Lecture de la base
	bOk = inputFile.Open();
	if (bOk)
	{
		lFilePos = 0;
		lLineNumber = 0;
		lRecordIndex = 0;
		nFrequency = 0;
		cLowerValue = KWContinuous::GetMissingValue();
		cUpperValue = KWContinuous::GetMissingValue();

		// Analyse du fichier
		while (bOk and lFilePos < inputFile.GetFileSize())
		{
			// Remplissage du buffer
			bOk = inputFile.FillOuterLines(lFilePos, bLineTooLong);
			if (not bOk)
				AddInputFileError(&inputFile, lRecordIndex, "Error while reading file");

			// Erreur si ligne trop longue
			if (bLineTooLong)
			{
				bOk = false;
				AddInputFileError(&inputFile, lRecordIndex + 1,
						  InputBufferedFile::GetLineTooLongErrorLabel());
			}

			// Analyse du buffer
			while (bOk and not inputFile.IsBufferEnd())
			{
				// Lecture des champs de la ligne
				bEndOfLine = false;
				nField = 0;
				nFieldError = inputFile.FieldNoError;
				lRecordIndex++;
				while (bOk and not bEndOfLine)
				{
					// Erreur si trop de champs
					if (nField >= nFileFormat)
					{
						AddInputFileError(&inputFile, lRecordIndex, "too many fields in line");
						bOk = false;
					}

					// Lecture du champ suivant
					if (bOk)
					{
						if (nFileFormat == 1)
						{
							bOk = ReadValue(&inputFile, lRecordIndex, cUpperValue,
									bEndOfLine);
							if (bOk)
								lCumulatedFrequency++;
						}
						else if (nFileFormat == 2)
						{
							if (nField == 0)
								bOk = ReadValue(&inputFile, lRecordIndex, cUpperValue,
										bEndOfLine);
							else if (nField == 1)
								bOk = ReadFrequency(&inputFile, lRecordIndex,
										    lCumulatedFrequency, nFrequency,
										    bEndOfLine);
						}
						else if (nFileFormat == 3)
						{
							if (nField == 0)
								bOk = ReadValue(&inputFile, lRecordIndex, cLowerValue,
										bEndOfLine);
							else if (nField == 1)
								bOk = ReadValue(&inputFile, lRecordIndex, cUpperValue,
										bEndOfLine);
							else if (nField == 2)
								bOk = ReadFrequency(&inputFile, lRecordIndex,
										    lCumulatedFrequency, nFrequency,
										    bEndOfLine);
						}
					}

					// Memorisation de la valeur si ok
					if (bOk)
					{
						// Erreur si trop de valeurs
						if (lCumulatedFrequency > INT_MAX)
						{
							AddInputFileError(&inputFile, lRecordIndex,
									  sTmp + "cumulated frequency too large (" +
									      LongintToString(lCumulatedFrequency) +
									      ")");
							bOk = false;
						}
						// Erreur si pas assez de champ en fin de ligne
						else if (bEndOfLine and nField < nFileFormat - 1)
						{
							AddInputFileError(&inputFile, lRecordIndex,
									  "not enough fields in line");
							bOk = false;
						}
						// Memorisation si fin de ligne
						else if (bEndOfLine)
						{
							// Si format 1, memorisation directe dans le vecteur des valeurs
							if (nFileFormat == 1)
								cvUpperValues->Add(cUpperValue);
							// Si format 2, memorisation d'un bin, car on devra ensuite les
							// trier
							else if (nFileFormat == 2)
							{
								bin = new MHBin;
								bin->Initialize(cUpperValue, cUpperValue, nFrequency);
								oaBins.Add(bin);
							}
							// Si format 3, les bin doivent etre tries en entree, et on peut
							// memoriser directement les valeurs
							else if (nFileFormat == 3)
							{
								// Test de validite de l'ordre des bins
								if (cLowerValue > cUpperValue)
								{
									AddInputFileError(
									    &inputFile, lRecordIndex,
									    sTmp + "lower value (" +
										KWContinuous::ContinuousToString(
										    cLowerValue) +
										") greater than upper value (" +
										KWContinuous::ContinuousToString(
										    cUpperValue) +
										")");
									bOk = false;
								}
								else if (cvUpperValues->GetSize() > 0 and
									 cLowerValue <=
									     cvUpperValues->GetAt(
										 cvUpperValues->GetSize() - 1))
								{
									AddInputFileError(
									    &inputFile, lRecordIndex,
									    sTmp + "lower value (" +
										KWContinuous::ContinuousToString(
										    cLowerValue) +
										") smaller or equal than preceding "
										"upper value (" +
										KWContinuous::ContinuousToString(
										    cvUpperValues->GetAt(
											cvUpperValues->GetSize() - 1)) +
										")");
									bOk = false;
								}

								// Memorisation si ok
								if (bOk)
								{
									cvLowerValues->Add(cLowerValue);
									cvUpperValues->Add(cUpperValue);
									ivFrequencies->Add(nFrequency);
								}
							}
						}
					}

					// Index du champs suivant
					nField++;
				}
			}

			// On se positionne sur la nouvelle position courante dans le fichier
			lFilePos = inputFile.GetPositionInFile();
		}
		inputFile.Close();
	}

	// Erreur si pas de valeur
	if (bOk and lCumulatedFrequency == 0)
	{
		AddError(sTmp + "empty dataset");
		bOk = false;
	}

	// Finalisation Ok
	if (bOk)
	{
		// Format 1: tri des valeurs
		if (nFileFormat == 1)
			cvUpperValues->Sort();
		// Format 2: tri des bins puis rangement dans les vecteurs
		else if (nFileFormat == 2)
		{
			MHBin::SortBinArray(&oaBins);
			for (i = 0; i < oaBins.GetSize(); i++)
			{
				bin = cast(MHBin*, oaBins.GetAt(i));
				cvUpperValues->Add(bin->GetUpperValue());
				ivFrequencies->Add(bin->GetFrequency());
			}
		}
		// Format 3: tout est deja ok
	}
	// Nettoyage sinon
	else
	{
		if (cvLowerValues != NULL)
			delete cvLowerValues;
		if (cvUpperValues != NULL)
			delete cvUpperValues;
		if (ivFrequencies != NULL)
			delete ivFrequencies;
		cvLowerValues = NULL;
		cvUpperValues = NULL;
		ivFrequencies = NULL;
	}
	oaBins.DeleteAll();
	ensure(not bOk or cvUpperValues->GetSize() > 0);
	return bOk;
}

boolean MHCommandLine::ReadValue(InputBufferedFile* inputFile, longint lRecordIndex, Continuous& cValue,
				 boolean& bEndOfLine)
{
	boolean bOk = true;
	char* sField;
	int nFieldLength;
	int nError;
	boolean bLineTooLong;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);

	// Lecture du champ suivant
	bEndOfLine = inputFile->GetNextField(sField, nFieldLength, nError, bLineTooLong);

	// Test si champ vide
	if (sField[0] == '\0')
	{
		AddInputFileError(inputFile, lRecordIndex, "empty field instead of value");
		bOk = false;
	}

	// Test si erreur de parsing
	if (bOk and nError != inputFile->FieldNoError)
	{
		AddInputFileError(inputFile, lRecordIndex,
				  "invalid value <" + InputBufferedFile::GetDisplayValue(sField) +
				      "> : " + inputFile->GetFieldErrorLabel(nError));
		bOk = false;
	}

	// Test si ligne trop longue
	if (bOk and bLineTooLong)
	{
		AddInputFileError(inputFile, lRecordIndex, InputBufferedFile::GetLineTooLongErrorLabel());
		bOk = false;
	}

	// Parsing si pas d'erreur
	if (bOk)
	{
		// Transformation de l'eventuel separateur decimal ',' en '.'
		KWContinuous::PreprocessString(sField);

		// Conversion en Continuous
		nError = KWContinuous::StringToContinuousError(sField, cValue);

		// Test de validite du champs
		if (bOk and nError != 0)
		{
			AddInputFileError(inputFile, lRecordIndex,
					  "invalid value <" + InputBufferedFile::GetDisplayValue(sField) + "> (" +
					      KWContinuous::ErrorLabel(nError) + ")");
			bOk = false;
		}
	}
	if (not bOk)
		cValue = KWContinuous::GetMissingValue();
	return bOk;
}

boolean MHCommandLine::ReadFrequency(InputBufferedFile* inputFile, longint lRecordIndex, longint& lCumulatedFrequency,
				     int& nFrequency, boolean& bEndOfLine)
{
	boolean bOk = true;
	char* sField;
	Continuous cFrequency;
	int nFieldLength;
	int nError;
	boolean bLineTooLong;
	ALString sTmp;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);
	require(lCumulatedFrequency >= 0);

	// Lecture du champ suivant
	bEndOfLine = inputFile->GetNextField(sField, nFieldLength, nError, bLineTooLong);

	// Test si champ vide
	if (sField[0] == '\0')
	{
		AddInputFileError(inputFile, lRecordIndex, "empty field instead of frequency");
		bOk = false;
	}

	// Test si erreur de parsing
	if (bOk and nError != inputFile->FieldNoError)
	{
		AddInputFileError(inputFile, lRecordIndex,
				  "invalid frequency <" + InputBufferedFile::GetDisplayValue(sField) +
				      "> : " + inputFile->GetFieldErrorLabel(nError));
		bOk = false;
	}

	// Test si ligne trop longue
	if (bOk and bLineTooLong)
	{
		AddInputFileError(inputFile, lRecordIndex, InputBufferedFile::GetLineTooLongErrorLabel());
		bOk = false;
	}

	// Parsing si pas d'erreur
	cFrequency = KWContinuous::GetMissingValue();
	if (bOk)
	{
		// Conversion en Continuous
		nError = KWContinuous::StringToContinuousError(sField, cFrequency);

		// Test de validite du champs
		if (bOk and nError != 0)
		{
			AddInputFileError(inputFile, lRecordIndex,
					  "invalid frequency <" + InputBufferedFile::GetDisplayValue(sField) + "> (" +
					      KWContinuous::ErrorLabel(nError) + ")");
			bOk = false;
		}
	}

	// Conversion en entier si ok
	if (bOk)
	{
		assert(cFrequency != KWContinuous::GetMissingValue());
		nFrequency = (int)cFrequency;

		// Frequence negative
		if (cFrequency < 0)
		{
			AddInputFileError(inputFile, lRecordIndex,
					  sTmp + "negative frequency (" + KWContinuous::ContinuousToString(cFrequency) +
					      ")");
			bOk = false;
		}
		// Frequence trop grande
		else if (cFrequency > INT_MAX)
		{
			AddInputFileError(inputFile, lRecordIndex,
					  sTmp + "frequency too large (" +
					      KWContinuous::ContinuousToString(cFrequency) + ")");
			bOk = false;
		}
		// Frequence non entiere
		else if (fabs(cFrequency - nFrequency) > nFrequency * 1e-12)
		{
			AddInputFileError(inputFile, lRecordIndex,
					  sTmp + "frequency should be an integer (" +
					      KWContinuous::ContinuousToString(cFrequency) + ")");
			bOk = false;
		}
		// Frequence cumulee trop large
		else if (lCumulatedFrequency + nFrequency > INT_MAX)
		{
			AddInputFileError(inputFile, lRecordIndex,
					  sTmp + "cumulated frequency too large (" +
					      LongintToReadableString(lCumulatedFrequency + nFrequency) + ")");
			bOk = false;
		}
		// Tout est ok: on calcule l'effectif cumule
		else
			lCumulatedFrequency += nFrequency;
	}
	if (not bOk)
		nFrequency = 0;
	return bOk;
}

void MHCommandLine::AddInputFileWarning(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel)
{
	ALString sTmp;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);
	if (lRecordIndex > 0)
		inputFile->AddWarning(sTmp + "line " + LongintToReadableString(lRecordIndex) + " : " + sLabel);
	else
		inputFile->AddWarning(sLabel);
}

void MHCommandLine::AddInputFileError(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel)
{
	ALString sTmp;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);
	if (lRecordIndex > 0)
		inputFile->AddError(sTmp + "line " + LongintToReadableString(lRecordIndex) + " : " + sLabel);
	else
		inputFile->AddError(sLabel);
}

const ALString MHCommandLine::InitializeParameters(int argc, char** argv)
{
	boolean bOk = true;
	int i;
	ALString sArgument;
	int nField;
	UIData* field;
	UIData* optionField;
	ALString sOption;

	// On initialise les valeurs du parametrage
	histogramSpecView.EventRefresh(&histogramSpec);

	// On recherche deja les options standard
	if (bOk)
	{
		for (i = 1; i < argc; i++)
		{
			sArgument = argv[i];

			// Version
			if (sArgument == "-v")
			{
				cout << GetClassLabel() << " " << histogramSpec.GetVersion() << "\n ";
				cout << GENUMFP_COPYRIGHT_LABEL << "\n";
				bOk = false;
			}
			// Aide
			else if (sArgument == "-h" or sArgument == "--h" or sArgument == "--help")
			{
				ShowHelp();
				bOk = false;
			}
		}
	}

	// Test du bon nombre d'options
	if (bOk)
	{
		if (argc % 2 == 1 or argc < 2)
		{
			cout << GetClassLabel() << ": invalid number of options\n ";
			cout << "Try '" << GetClassLabel() << "' -h' for more information.\n";
			bOk = false;
		}
	}

	// Traitement des options
	if (bOk)
	{
		for (i = 1; i < argc - 1; i += 2)
		{
			sArgument = argv[i];
			assert(i + 1 < argc);

			// Traitement des options
			optionField = NULL;
			for (nField = 0; nField < histogramSpecView.GetFieldNumber(); nField++)
			{
				field = histogramSpecView.GetFieldAtIndex(nField);
				if (field->GetVisible())
				{
					sOption = '-';
					sOption += field->GetShortCut();
					if (sArgument == sOption)
					{
						optionField = field;
						break;
					}
				}
			}

			// Cas d'un option inconnue
			if (optionField == NULL)
			{
				cout << GetClassLabel() << ": invalid option '" << sArgument << "'\n ";
				cout << "Try '" << GetClassLabel() << "' -h' for more information.\n";
				bOk = false;
				break;
			}
			// Aquisition du parametre
			else
			{
				bOk = SetOptionValue(cast(UIElement*, optionField), argv[i + 1]);
				if (not bOk)
				{
					cout << GetClassLabel() << ": invalid value '" << argv[i + 1]
					     << "' for option '" << sArgument << "'\n";
					cout << "Try '" << GetClassLabel() << "' -h' for more information.\n";
					break;
				}
			}
		}
	}

	// On recopie le parametrage
	if (bOk)
	{
		histogramSpecView.EventUpdate(&histogramSpec);
		sDataFileName = argv[argc - 1];
	}
	return sDataFileName;
}

void MHCommandLine::ShowHelp()
{
	int nField;
	UIData* field;

	cout << "Usage: " << GetClassLabel() << " [OPTION]...[FILE]\n ";
	cout << "Compute histogram from the data in FILE, using the G-Enum-fp method.\n";
	cout << "\n";
	cout << "Available options are\n";

	// Aide par parametre visible
	for (nField = 0; nField < histogramSpecView.GetFieldNumber(); nField++)
	{
		field = histogramSpecView.GetFieldAtIndex(nField);
		if (field->GetVisible())
			cout << field->GetHelpText();
	}

	// Options generales
	cout << "\t-h\t\tdisplay this help and exit\n";
	cout << "\t-v\t\toutput version information and exit\n";
}

void MHCommandLine::InitializeParameterSettings()
{
	int nField;
	UIData* field;
	ALString sTmp;

	// https://pubs.opengroup.org/onlinepubs/7908799/xbd/utilconv.html
	// https://pubs.opengroup.org/onlinepubs/7908799/xbd/utilconv.html#tag_009_002

	// Deplacement des commandes pour maitriser l'ordre d'affichage de l'aide
	histogramSpecView.MoveFieldBefore("FileFormat", "HistogramCriterion");
	histogramSpecView.MoveFieldBefore("ResultFilesDirectory", "HistogramCriterion");
	histogramSpecView.MoveFieldBefore("ResultFilesPrefix", "HistogramCriterion");
	histogramSpecView.MoveFieldBefore("MaxCoarsenedHistogramNumber", "HistogramCriterion");
	histogramSpecView.MoveFieldBefore("TruncationManagementHeuristic", "HistogramCriterion");
	histogramSpecView.MoveFieldBefore("SingularityRemovalHeuristic", "HistogramCriterion");
	histogramSpecView.MoveFieldBefore("MaxHierarchyLevel", "HistogramCriterion");
	histogramSpecView.MoveFieldBefore("DeltaCentralBinExponent", "HistogramCriterion");

	// Initialisation des accelerateurs des commandes
	histogramSpecView.GetFieldAt("DeltaCentralBinExponent")->SetShortCut('c');
	histogramSpecView.GetFieldAt("MaxHierarchyLevel")->SetShortCut('l');
	histogramSpecView.GetFieldAt("MaxCoarsenedHistogramNumber")->SetShortCut('n');
	histogramSpecView.GetFieldAt("TruncationManagementHeuristic")->SetShortCut('t');
	histogramSpecView.GetFieldAt("SingularityRemovalHeuristic")->SetShortCut('s');
	histogramSpecView.GetFieldAt("ResultFilesDirectory")->SetShortCut('d');
	histogramSpecView.GetFieldAt("ResultFilesPrefix")->SetShortCut('p');
	histogramSpecView.GetFieldAt("FileFormat")->SetShortCut('f');

	// Initialisation des textes d'aides specifiques a la ligne de commande
	histogramSpecView.GetFieldAt("DeltaCentralBinExponent")
	    ->SetHelpText(sTmp + "\t-" + histogramSpecView.GetFieldAt("DeltaCentralBinExponent")->GetShortCut() +
			  " int\t\tadjust central bin exponent (default: -1)\n"
			  "\t\t\t-1: the central bin is exponent is optimized automatically\n"
			  "\t\t\t0: minimum possible central bin exponent, to be floating-point oriented\n"
			  "\t\t\t1000: maximum possible central bin exponent, to be equal-width oriented\n"
			  "\t\t\totherwise used as an increment w.r.t the minimum possible central bin exponent\n");
	histogramSpecView.GetFieldAt("MaxHierarchyLevel")
	    ->SetHelpText(sTmp + "\t-" + histogramSpecView.GetFieldAt("MaxHierarchyLevel")->GetShortCut() +
			  " int\t\tmaximum evaluated hierarchy level (default: 0, means no constraint)\n");
	histogramSpecView.GetFieldAt("MaxCoarsenedHistogramNumber")
	    ->SetHelpText(sTmp + "\t-" + histogramSpecView.GetFieldAt("MaxCoarsenedHistogramNumber")->GetShortCut() +
			  " int\t\tmaximum coarsened histogram number (default: 0)\n"
			  "\t\t\tBy default, one single histogram is produced.\n"
			  "\t\t\tIf the truncation management and/or the singularity removal heuristic is triggered,\n"
			  "\t\t\tthe initial histogram, suffixed by 'raw', is also produced if it is different.\n"
			  "\t\t\tIf histogram coarsening is triggered, a number of additional simplified histograms\n"
			  "\t\t\twill be produced, suffixed by 'c1', 'c2',...\n");
	histogramSpecView.GetFieldAt("TruncationManagementHeuristic")
	    ->SetHelpText(
		sTmp + "\t-" + histogramSpecView.GetFieldAt("TruncationManagementHeuristic")->GetShortCut() +
		" [0, 1]\tindicates whether to apply the heuristic that deals with truncated values (default: 1)\n");
	histogramSpecView.GetFieldAt("SingularityRemovalHeuristic")
	    ->SetHelpText(sTmp + "\t-" + histogramSpecView.GetFieldAt("SingularityRemovalHeuristic")->GetShortCut() +
			  " [0, 1]\tindicates whether to remove to singular intervals after optimization (default: 1)\n"
			  "\t\t\tusing a lower granularity, in order to produce a smoother histogram\n");
	histogramSpecView.GetFieldAt("ResultFilesDirectory")
	    ->SetHelpText(sTmp + "\t-" + histogramSpecView.GetFieldAt("ResultFilesDirectory")->GetShortCut() +
			  " string\toutput directory\n"
			  "\t\t\tThe built histograms are output in files named <prefix>histogram<.suffix>.log.\n");
	histogramSpecView.GetFieldAt("ResultFilesPrefix")
	    ->SetHelpText(sTmp + "\t-" + histogramSpecView.GetFieldAt("ResultFilesPrefix")->GetShortCut() +
			  " string\tprefix of output histogram files\n");
	histogramSpecView.GetFieldAt("FileFormat")
	    ->SetHelpText(sTmp + "\t-" + histogramSpecView.GetFieldAt("FileFormat")->GetShortCut() +
			  " [1, 2, 3]\tdata file format (default: 1)\n"
			  "\t\t\t1: <value> per line\n"
			  "\t\t\t2: <value> tab <frequency> per line\n"
			  "\t\t\t3: <lower value> tab <upper value> tab <frequency> per line\n"
			  "\t\t\tWith the bin format (3), [lv, uv] stands for an interval containing several values.\n"
			  "\t\t\tThe bins must be exclusive and sorted by increasing value.\n");

	// On ne rend visible que les parametres ayant un short cut
	for (nField = 0; nField < histogramSpecView.GetFieldNumber(); nField++)
	{
		field = histogramSpecView.GetFieldAtIndex(nField);
		field->SetVisible(field->GetShortCut() != ' ');
	}
}

boolean MHCommandLine::SetOptionValue(UIElement* optionField, const ALString& sValue)
{
	boolean bOk = true;
	UIBooleanElement booleanFieldRef;
	UICharElement charFieldRef;
	UIIntElement intFieldRef;
	UIDoubleElement doubleFieldRef;
	UIStringElement stringFieldRef;
	UIBooleanElement* booleanField;
	UICharElement* charField;
	UIIntElement* intField;
	UIDoubleElement* doubleField;
	UIStringElement* stringField;
	Continuous cValue;
	int nValue;

	require(optionField != NULL);
	require(histogramSpecView.GetFieldAt(optionField->GetIdentifier()) == optionField);

	// On alimente la valeur selon le type du champ
	if (optionField->GetClassLabel() == booleanFieldRef.GetClassLabel())
	{
		booleanField = cast(UIBooleanElement*, optionField);

		// Parametrage de la valeur booleenne 0 ou 1
		if (sValue == "0")
			histogramSpecView.SetBooleanValueAt(optionField->GetIdentifier(), false);
		else if (sValue == "1")
			histogramSpecView.SetBooleanValueAt(optionField->GetIdentifier(), true);
		else
			bOk = false;
	}
	else if (optionField->GetClassLabel() == charFieldRef.GetClassLabel())
	{
		charField = cast(UICharElement*, optionField);

		// Parametrage du caractere
		if (sValue.GetLength() == 1)
			histogramSpecView.SetCharValueAt(optionField->GetIdentifier(), sValue.GetAt(0));
		else
			bOk = false;
	}
	else if (optionField->GetClassLabel() == intFieldRef.GetClassLabel())
	{
		intField = cast(UIIntElement*, optionField);

		// Parametrage de l'entier
		cValue = KWContinuous::StringToContinuous(sValue);
		nValue = (int)cValue;
		if (cValue == KWContinuous::GetMissingValue())
			bOk = false;
		else if (fabs(cValue - nValue) > 0)
			bOk = false;
		else if (intField->GetMinValue() <= nValue and nValue <= intField->GetMaxValue())
			histogramSpecView.SetIntValueAt(optionField->GetIdentifier(), nValue);
		else
			bOk = false;
	}
	else if (optionField->GetClassLabel() == doubleFieldRef.GetClassLabel())
	{
		doubleField = cast(UIDoubleElement*, optionField);

		// Parametrage du double
		cValue = KWContinuous::StringToContinuous(sValue);
		if (cValue == KWContinuous::GetMissingValue())
			bOk = false;
		else if (doubleField->GetMinValue() <= cValue and cValue <= doubleField->GetMaxValue())
			histogramSpecView.SetDoubleValueAt(optionField->GetIdentifier(), cValue);
		else
			bOk = false;
	}
	else if (optionField->GetClassLabel() == stringFieldRef.GetClassLabel())
	{
		stringField = cast(UIStringElement*, optionField);

		// Parametrage de la chaine de caractere
		histogramSpecView.SetStringValueAt(optionField->GetIdentifier(), sValue);
	}
	return bOk;
}
