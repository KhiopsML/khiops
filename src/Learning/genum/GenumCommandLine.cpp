// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "GenumCommandLine.h"

GenumCommandLine::GenumCommandLine()
{
	InitializeParameterSettings();
}

GenumCommandLine::~GenumCommandLine() {}

boolean GenumCommandLine::ComputeHistogram(int argc, char** argv)
{
	boolean bOk = true;
	MHDiscretizerGenumHistogram histogramBuilder;
	ContinuousVector* cvValues;
	MHHistogram* optimizedHistogram;
	const MHHistogramInterval* histogramInterval;
	fstream fstHistogram;
	int nTotalFrequency;
	int nBin;

	// Initialisation des parametres
	bOk = InitializeParameters(argc, argv);

	// Lecture des donnees
	if (bOk)
	{
		// Recopie du parametrage
		histogramBuilder.GetHistogramSpec()->CopyFrom(&histogramSpec);

		// Lecture des bins
		bOk = ReadValues(cvValues);

		// Construction de l'histograme si ok
		optimizedHistogram = NULL;
		if (bOk)
		{
			histogramBuilder.ComputeHistogramFromBins(cvValues, optimizedHistogram);

			// Exploitation des resultats
			bOk = FileService::OpenOutputFile(sHistogramFileName, fstHistogram);
			if (bOk)
			{
				nTotalFrequency = optimizedHistogram->ComputeTotalFrequency();

				// Affichage des caracteristiques des bins
				fstHistogram << "LowerBound,UpperBound,Length,Frequency,Probability,Density\n";
				for (nBin = 0; nBin < optimizedHistogram->GetIntervalNumber(); nBin++)
				{
					histogramInterval = optimizedHistogram->GetConstIntervalAt(nBin);
					fstHistogram
					    << KWContinuous::ContinuousToString(histogramInterval->GetLowerBound())
					    << ",";
					fstHistogram
					    << KWContinuous::ContinuousToString(histogramInterval->GetUpperBound())
					    << ",";
					fstHistogram << KWContinuous::ContinuousToString(histogramInterval->GetLength())
						     << ",";
					fstHistogram << histogramInterval->GetFrequency() << ",";
					fstHistogram << KWContinuous::ContinuousToString(
							    histogramInterval->GetProbability(nTotalFrequency))
						     << ",";
					fstHistogram << KWContinuous::ContinuousToString(
							    histogramInterval->GetDensity(nTotalFrequency))
						     << "\n";
				}
				FileService::CloseOutputFile(sHistogramFileName, fstHistogram);
			}
		}

		// Nettoyage
		if (cvValues != NULL)
			delete cvValues;
		if (optimizedHistogram != NULL)
			delete optimizedHistogram;
	}

	// Nettoyage
	sDataFileName = "";
	return bOk;
}

const ALString GenumCommandLine::GetClassLabel() const
{
	return "genum";
}

const ALString GenumCommandLine::GetObjectLabel() const
{
	return sDataFileName;
}

boolean GenumCommandLine::ReadValues(ContinuousVector*& cvValues)
{
	boolean bOk = true;
	InputBufferedFile inputFile;
	longint lFilePos;
	boolean bLineTooLong;
	longint lRecordIndex;
	longint lLineNumber;
	boolean bEndOfLine;
	int nField;
	int nFieldError;
	longint lCumulatedFrequency;
	Continuous cValue;
	ObjectArray oaBins;
	ALString sTmp;

	require(sDataFileName != "");
	require(histogramSpec.GetFileFormat() == 1);

	// Initialisation des resultats
	lCumulatedFrequency = 0;
	cvValues = new ContinuousVector;

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
		cValue = KWContinuous::GetMissingValue();

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
					if (nField >= 1)
					{
						AddInputFileError(&inputFile, lRecordIndex, "too many fields in line");
						bOk = false;
					}

					// Lecture du champ suivant
					if (bOk)
					{
						bOk = ReadValue(&inputFile, lRecordIndex, cValue, bEndOfLine);
						if (bOk)
							lCumulatedFrequency++;
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
						// Memorisation si fin de ligne
						else if (bEndOfLine)
						{
							// Memorisation directe dans le vecteur des valeurs
							cvValues->Add(cValue);
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
		// Tri des valeurs
		cvValues->Sort();
	}
	// Nettoyage sinon
	else
	{
		if (cvValues != NULL)
			delete cvValues;
		cvValues = NULL;
	}
	oaBins.DeleteAll();
	ensure(not bOk or cvValues->GetSize() > 0);
	return bOk;
}

boolean GenumCommandLine::ReadValue(InputBufferedFile* inputFile, longint lRecordIndex, Continuous& cValue,
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

void GenumCommandLine::AddInputFileWarning(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel)
{
	ALString sTmp;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);
	if (lRecordIndex > 0)
		inputFile->AddWarning(sTmp + "line " + LongintToReadableString(lRecordIndex) + " : " + sLabel);
	else
		inputFile->AddWarning(sLabel);
}

void GenumCommandLine::AddInputFileError(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel)
{
	ALString sTmp;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);
	if (lRecordIndex > 0)
		inputFile->AddError(sTmp + "line " + LongintToReadableString(lRecordIndex) + " : " + sLabel);
	else
		inputFile->AddError(sLabel);
}

boolean GenumCommandLine::InitializeParameters(int argc, char** argv)
{
	boolean bOk = true;
	int i;
	ALString sArgument;
	ALString sOption;

	// On recherche deja les options standard
	if (bOk)
	{
		for (i = 1; i < argc; i++)
		{
			sArgument = argv[i];

			// Version
			if (sArgument == "-v")
			{
				cout << GetClassLabel() << " " << GENUM_VERSION << "\n ";
				cout << "Copyright (C) 2023 Orange labs\n";
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
		if (argc != 3)
		{
			cout << GetClassLabel() << ": invalid number of parameters\n";
			cout << "Try '" << GetClassLabel() << "' -h' for more information.\n";
			bOk = false;
		}
	}

	// On recopie le parametrage
	if (bOk)
	{
		assert(argc == 3);
		sDataFileName = argv[1];
		sHistogramFileName = argv[2];
		if (sDataFileName == sHistogramFileName)
		{
			bOk = false;
			cout << "Result histogram file name must be different from input values file name ("
			     << sDataFileName << ")\n";
		}
	}
	return bOk;
}

void GenumCommandLine::ShowHelp()
{
	// Aide principale
	cout << "Usage: " << GetClassLabel() << " [VALUES] [HISTOGRAM]\n ";
	cout << "Compute histogram from the values in VALUES file, using the G-Enum method.\n";
	cout << " The resulting histogram is output in HISTOGRAM file, with the lower bound, upper bound,\n";
	cout << "  length, frequency, probability and density per bin.\n ";

	// Options generales
	cout << "\t-h\tdisplay this help and exit\n";
	cout << "\t-v\tdisplay version information and exit\n";
}

void GenumCommandLine::InitializeParameterSettings()
{
	// Parametrage pour obtenir un histogramme G-enum conforme a l'article de Valentina
	histogramSpec.SetHistogramCriterion("G-Enum");
	histogramSpec.SetTruncationManagementHeuristic(false);
	histogramSpec.SetSingularityRemovalHeuristic(false);
	histogramSpec.SetOutlierManagementHeuristic(false);
}
