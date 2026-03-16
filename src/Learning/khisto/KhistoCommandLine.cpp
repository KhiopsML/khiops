// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KhistoCommandLine.h"

KhistoCommandLine::KhistoCommandLine()
{
	bBinaryInput = false;
	bExploratoryAnalysis = false;
	bJsonFormat = false;
}

KhistoCommandLine::~KhistoCommandLine() {}

boolean KhistoCommandLine::ComputeHistogram(int argc, char** argv)
{
	boolean bOk = true;
	MHDiscretizerTruncationMODLHistogram histogramBuilder;
	ContinuousVector* cvLowerValues;
	ContinuousVector* cvUpperValues;
	IntVector* ivFrequencies;
	MHMODLHistogramAnalysisStats* histogramAnalysisStats;

	// Initialisation des parametres
	bOk = InitializeParameters(argc, argv);

	// Lecture des donnees
	if (bOk)
	{
		// Le parametrage des specification est cense etre standard
		assert(histogramSpec.GetHistogramCriterion() == "G-Enum-fp");
		assert(histogramSpec.GetFileFormat() == 1);
		assert(histogramSpec.GetTruncationManagementHeuristic());
		assert(histogramSpec.GetSingularityRemovalHeuristic());
		assert(histogramSpec.GetEpsilonBinWidth() == 0);
		assert(histogramSpec.GetDeltaCentralBinExponent() == -1);
		histogramBuilder.GetHistogramSpec()->CopyFrom(&histogramSpec);

		// Lecture des bins
		// On garde la possibilte de gerer les formats de fichier etendus, pour des evolutions future
		// potentielles
		if (histogramSpec.GetFileFormat() == 1)
		{
			if (bBinaryInput)
				bOk = ReadBinaryValues(cvUpperValues);
			else
				bOk = ReadValues(cvUpperValues);
			cvLowerValues = NULL;
			ivFrequencies = NULL;
		}
		else
		{
			// Option non implementee dans le cas de fichiers binaires en entree
			assert(not bBinaryInput);
			bOk = ReadBins(cvLowerValues, cvUpperValues, ivFrequencies);
		}

		// Construction de l'histogramme si ok
		if (bOk)
			histogramBuilder.ComputeHistogramFromBins(cvLowerValues, cvUpperValues, ivFrequencies);

		// Exploitation des resultats
		if (bOk)
		{
			histogramAnalysisStats =
			    cast(MHMODLHistogramAnalysisStats*, histogramBuilder.BuildMODLHistogramResults());
			bOk = histogramAnalysisStats != NULL;
			if (bOk)
			{
				if (bJsonFormat)
					ExportHistogramAnalysisStastJsonReportFile(histogramAnalysisStats,
										   sHistogramFileName);
				else
					ExportHistogramAnalysisStastCsvReportFiles(histogramAnalysisStats,
										   sHistogramFileName);
				delete histogramAnalysisStats;
			}
		}

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

const ALString KhistoCommandLine::GetClassLabel() const
{
	return "khisto";
}

const ALString KhistoCommandLine::GetObjectLabel() const
{
	return sDataFileName;
}

boolean KhistoCommandLine::ExportHistogramStatsCsvReportFile(const MHMODLHistogramStats* histogramStats,
							     const ALString& sFileName)
{
	boolean bOk = true;
	fstream fstHistogram;
	JSONFile jsonHistogram;
	int nTotalFrequency;
	int nBin;
	Continuous cLength;
	int nFrequency;
	Continuous cProbability;
	Continuous cDensity;

	require(histogramStats != NULL);
	require(histogramStats->Check());
	require(sFileName != "");
	require(not bJsonFormat);

	// Calcul de l'effectif total
	nTotalFrequency = histogramStats->ComputeTotalFrequency();

	// Ecriture au format csv
	bOk = FileService::OpenOutputFile(sFileName, fstHistogram);
	if (bOk)
	{
		// Affichage des caracteristiques des bins
		fstHistogram << "LowerBound,UpperBound,Length,Frequency,Probability,Density\n";
		for (nBin = 0; nBin < histogramStats->GetIntervalNumber(); nBin++)
		{
			cLength = histogramStats->GetIntervalUpperBoundAt(nBin) -
				  histogramStats->GetIntervalLowerBoundAt(nBin);
			nFrequency = histogramStats->GetIntervalFrequencyAt(nBin);
			cProbability = nFrequency / (double)nTotalFrequency;
			cDensity = cProbability / cLength;

			// Affichage des resultats
			fstHistogram << KWContinuous::ContinuousToString(histogramStats->GetIntervalLowerBoundAt(nBin))
				     << ",";
			fstHistogram << KWContinuous::ContinuousToString(histogramStats->GetIntervalUpperBoundAt(nBin))
				     << ",";
			fstHistogram << KWContinuous::ContinuousToString(cLength) << ",";
			fstHistogram << nFrequency << ",";
			fstHistogram << KWContinuous::ContinuousToString(cProbability) << ",";
			fstHistogram << KWContinuous::ContinuousToString(cDensity) << "\n";
		}

		// Fermeture du fichier
		FileService::CloseOutputFile(sFileName, fstHistogram);
	}
	return bOk;
}

boolean KhistoCommandLine::ExportHistogramAnalysisStastCsvReportFiles(
    const MHMODLHistogramAnalysisStats* histogramAnalysisStats, const ALString& sBaseFileName)
{
	boolean bOk = true;
	const MHMODLHistogramStats* histogramStats;
	const ALString sCoarseningPrefix = ".";
	ALString sPathName;
	ALString sFileName;
	ALString sFilePrefix;
	ALString sFileSuffix;
	ALString sCoarsenedHistogramFileName;
	ALString sHistogramSeriesFileName;
	fstream fstHistogramSeries;
	int n;
	ALString sTmp;

	require(histogramAnalysisStats != NULL);
	require(histogramAnalysisStats->Check());
	require(sBaseFileName != "");
	require(not bJsonFormat);

	// Export de l'histogramme principal
	bOk = ExportHistogramStatsCsvReportFile(histogramAnalysisStats->GetMostAccurateInterpretableHistogram(),
						sBaseFileName);

	// Export de la serie d'histogrames dans le cas de l'analyse exploratoire
	if (bOk and bExploratoryAnalysis)
	{
		// Extraction des caracteristiques du fichier en parametre
		sPathName = FileService::GetPathName(sBaseFileName);
		sFileName = FileService::GetFileName(sBaseFileName);
		sFilePrefix = FileService::GetFilePrefix(sFileName);
		sFileSuffix = FileService::GetFileSuffix(sFileName);

		// Ouverture du fichier d'indicateurs sur la serie d'histogrammes
		sHistogramSeriesFileName = FileService::BuildFilePathName(
		    sPathName, FileService::BuildFileName(sFilePrefix + sCoarseningPrefix + "series", sFileSuffix));
		bOk = FileService::OpenOutputFile(sHistogramSeriesFileName, fstHistogramSeries);

		// Export de l'ensemble des resultats
		if (bOk)
		{
			// Entete du fichier d'indicateur
			fstHistogramSeries << "FileName,";
			fstHistogramSeries
			    << "Granularity,IntervalNumber,PeakIntervalNumber,SpikeIntervalNumber,EmptyIntervalNumber,";
			fstHistogramSeries << "Level,InformationRate,";
			fstHistogramSeries << "TruncationEpsilon,RemovedSingularityNumber,Raw\n";

			// Export de chaque histogramme
			for (n = 0; n < histogramAnalysisStats->GetHistogramNumber(); n++)
			{
				histogramStats = histogramAnalysisStats->GetHistogramAt(n);

				// Export de l'histogramme
				sCoarsenedHistogramFileName = FileService::BuildFilePathName(
				    sPathName, FileService::BuildFileName(
						   sFilePrefix + sCoarseningPrefix + IntToString(n + 1), sFileSuffix));
				bOk = bOk and
				      ExportHistogramStatsCsvReportFile(histogramStats, sCoarsenedHistogramFileName);

				// Export des stats sur l'histogramme
				if (bOk)
				{
					// Indicateur de base
					fstHistogramSeries << sCoarsenedHistogramFileName << ",";
					fstHistogramSeries << histogramStats->GetGranularity() << ",";
					fstHistogramSeries << histogramStats->GetIntervalNumber() << ",";
					fstHistogramSeries << histogramStats->GetPeakIntervalNumber() << ",";
					fstHistogramSeries << histogramStats->GetSpikeIntervalNumber() << ",";
					fstHistogramSeries << histogramStats->GetEmptyIntervalNumber() << ",";
					fstHistogramSeries << histogramStats->GetNormalizedLevel() << ",";

					// Level normalise
					if (histogramAnalysisStats->GetMostAccurateInterpretableHistogram()
						->GetNormalizedLevel() != 0)
						fstHistogramSeries << histogramStats->GetNormalizedLevel() * 100 /
									  histogramAnalysisStats
									      ->GetMostAccurateInterpretableHistogram()
									      ->GetNormalizedLevel()
								   << ",";
					else
						fstHistogramSeries << "0,";

					// Indicateur potentiellement different sur le denier histogramme s'il est "raw"
					if (n < histogramAnalysisStats->GetInterpretableHistogramNumber())
					{
						fstHistogramSeries << histogramAnalysisStats->GetTruncationEpsilon()
								   << ",";
						fstHistogramSeries << "0,";
						fstHistogramSeries << "0\n";
					}
					else
					{
						fstHistogramSeries << "0,";
						fstHistogramSeries
						    << histogramAnalysisStats->GetRemovedSingularIntervalsNumber()
						    << ",";
						fstHistogramSeries << "1\n";
					}
				}

				// Arret si erreur
				if (not bOk)
					break;
			}

			// Fermeture du fichier d'indicateurs
			FileService::CloseOutputFile(sHistogramSeriesFileName, fstHistogramSeries);
		}
	}
	return bOk;
}

boolean KhistoCommandLine::ExportHistogramStatsJson(const MHMODLHistogramStats* histogramStats, const ALString& sKey,
						    JSONFile* jsonFile)
{
	boolean bOk = true;
	int nTotalFrequency;
	int nBin;
	Continuous cLength;
	int nFrequency;
	Continuous cProbability;
	Continuous cDensity;

	require(histogramStats != NULL);
	require(histogramStats->Check());
	require(jsonFile != NULL);
	require(jsonFile->IsOpened());
	require(bJsonFormat);

	// Calcul de l'effectif total
	nTotalFrequency = histogramStats->ComputeTotalFrequency();

	// Debut de l'objet
	if (sKey == "")
		jsonFile->BeginObject();
	else
		jsonFile->BeginKeyObject(sKey);

	// Bornes inf
	jsonFile->BeginKeyList("lowerBounds");
	for (nBin = 0; nBin < histogramStats->GetIntervalNumber(); nBin++)
		jsonFile->WriteContinuous(histogramStats->GetIntervalLowerBoundAt(nBin));
	jsonFile->EndList();

	// Bornes sup
	jsonFile->BeginKeyList("upperBounds");
	for (nBin = 0; nBin < histogramStats->GetIntervalNumber(); nBin++)
		jsonFile->WriteContinuous(histogramStats->GetIntervalUpperBoundAt(nBin));
	jsonFile->EndList();

	// Longueurs
	jsonFile->BeginKeyList("lengths");
	for (nBin = 0; nBin < histogramStats->GetIntervalNumber(); nBin++)
	{
		cLength = histogramStats->GetIntervalUpperBoundAt(nBin) - histogramStats->GetIntervalLowerBoundAt(nBin);
		jsonFile->WriteContinuous(cLength);
	}
	jsonFile->EndList();

	// Effectifs
	jsonFile->BeginKeyList("frequencies");
	for (nBin = 0; nBin < histogramStats->GetIntervalNumber(); nBin++)
	{
		nFrequency = histogramStats->GetIntervalFrequencyAt(nBin);
		jsonFile->WriteInt(nFrequency);
	}
	jsonFile->EndList();

	// Probabilites
	jsonFile->BeginKeyList("probabilities");
	for (nBin = 0; nBin < histogramStats->GetIntervalNumber(); nBin++)
	{
		cLength = histogramStats->GetIntervalUpperBoundAt(nBin) - histogramStats->GetIntervalLowerBoundAt(nBin);
		nFrequency = histogramStats->GetIntervalFrequencyAt(nBin);
		cProbability = nFrequency / (double)nTotalFrequency;
		jsonFile->WriteContinuous(cProbability);
	}
	jsonFile->EndList();

	// Densites
	jsonFile->BeginKeyList("densities");
	for (nBin = 0; nBin < histogramStats->GetIntervalNumber(); nBin++)
	{
		cLength = histogramStats->GetIntervalUpperBoundAt(nBin) - histogramStats->GetIntervalLowerBoundAt(nBin);
		nFrequency = histogramStats->GetIntervalFrequencyAt(nBin);
		cProbability = nFrequency / (double)nTotalFrequency;
		cDensity = cProbability / cLength;
		jsonFile->WriteContinuous(cDensity);
	}
	jsonFile->EndList();

	// Fin de l'objet
	jsonFile->EndObject();
	return bOk;
}

boolean KhistoCommandLine::ExportHistogramAnalysisStastJsonReportFile(
    const MHMODLHistogramAnalysisStats* histogramAnalysisStats, const ALString& sFileName)
{
	boolean bOk = true;
	JSONFile jsonHistogram;
	const MHMODLHistogramStats* histogramStats;
	int n;

	require(histogramAnalysisStats != NULL);
	require(histogramAnalysisStats->Check());
	require(sFileName != "");
	require(bJsonFormat);

	// Ecriture au format json
	jsonHistogram.SetFileName(sFileName);
	bOk = jsonHistogram.OpenForWrite();
	if (bOk)
	{
		// Entete du json
		jsonHistogram.WriteKeyString("tool", "Khiops Histogram");
		jsonHistogram.WriteKeyString("version", KHISTO_VERSION);

		// Export de l'histogramme principal
		ExportHistogramStatsJson(histogramAnalysisStats->GetMostAccurateInterpretableHistogram(),
					 "bestHistogram", &jsonHistogram);
	}

	// Export de la serie d'histogrames dans le cas de l'analyse exploratoire
	if (bOk and bExploratoryAnalysis)
	{
		// Debut de l'objet definissant la serie d'histogrammes
		jsonHistogram.BeginKeyObject("histogramSeries");

		// Indicateurs sur la serie
		jsonHistogram.WriteKeyInt("histogramNumber", histogramAnalysisStats->GetHistogramNumber());
		jsonHistogram.WriteKeyInt("interpretableHistogramNumber",
					  histogramAnalysisStats->GetInterpretableHistogramNumber());
		jsonHistogram.WriteKeyContinuous("truncationEpsilon", histogramAnalysisStats->GetTruncationEpsilon());
		jsonHistogram.WriteKeyInt("removedSingularIntervalNumber",
					  histogramAnalysisStats->GetRemovedSingularIntervalsNumber());

		// Granularites
		jsonHistogram.BeginKeyList("granularities");
		for (n = 0; n < histogramAnalysisStats->GetHistogramNumber(); n++)
			jsonHistogram.WriteInt(histogramAnalysisStats->GetHistogramAt(n)->GetGranularity());
		jsonHistogram.EndList();

		// Nombres d'intervalles
		jsonHistogram.BeginKeyList("intervalNumbers");
		for (n = 0; n < histogramAnalysisStats->GetHistogramNumber(); n++)
			jsonHistogram.WriteInt(histogramAnalysisStats->GetHistogramAt(n)->GetIntervalNumber());
		jsonHistogram.EndList();

		// Nombres d'intervalles peaks
		jsonHistogram.BeginKeyList("peakIntervalNumbers");
		for (n = 0; n < histogramAnalysisStats->GetHistogramNumber(); n++)
			jsonHistogram.WriteInt(histogramAnalysisStats->GetHistogramAt(n)->GetPeakIntervalNumber());
		jsonHistogram.EndList();

		// Nombres d'intervalles spikes
		jsonHistogram.BeginKeyList("spikeIntervalNumbers");
		for (n = 0; n < histogramAnalysisStats->GetHistogramNumber(); n++)
			jsonHistogram.WriteInt(histogramAnalysisStats->GetHistogramAt(n)->GetSpikeIntervalNumber());
		jsonHistogram.EndList();

		// Nombres d'intervalles vides
		jsonHistogram.BeginKeyList("emptyIntervalNumbers");
		for (n = 0; n < histogramAnalysisStats->GetHistogramNumber(); n++)
			jsonHistogram.WriteInt(histogramAnalysisStats->GetHistogramAt(n)->GetEmptyIntervalNumber());
		jsonHistogram.EndList();

		// Levels des histogrammes
		jsonHistogram.BeginKeyList("levels");
		for (n = 0; n < histogramAnalysisStats->GetHistogramNumber(); n++)
			jsonHistogram.WriteContinuous(histogramAnalysisStats->GetHistogramAt(n)->GetNormalizedLevel());
		jsonHistogram.EndList();

		// Taux d'information des histogrammes
		jsonHistogram.BeginKeyList("informationRates");
		for (n = 0; n < histogramAnalysisStats->GetHistogramNumber(); n++)
		{
			histogramStats = histogramAnalysisStats->GetHistogramAt(n);
			if (histogramAnalysisStats->GetMostAccurateInterpretableHistogram()->GetNormalizedLevel() != 0)
				jsonHistogram.WriteContinuous(
				    histogramStats->GetNormalizedLevel() * 100 /
				    histogramAnalysisStats->GetMostAccurateInterpretableHistogram()
					->GetNormalizedLevel());
			else
				jsonHistogram.WriteContinuous(0);
		}
		jsonHistogram.EndList();

		// Liste des histogrammes
		jsonHistogram.BeginKeyArray("histograms");
		for (n = 0; n < histogramAnalysisStats->GetHistogramNumber(); n++)
		{
			histogramStats = histogramAnalysisStats->GetHistogramAt(n);
			ExportHistogramStatsJson(histogramStats, "", &jsonHistogram);
		}
		jsonHistogram.EndArray();

		// Fin de l'objet
		jsonHistogram.EndObject();
	}

	// Fermeture du fichier
	if (jsonHistogram.IsOpened())
		bOk = jsonHistogram.CloseWithoutEncodingStats() and bOk;
	return bOk;
}

boolean KhistoCommandLine::ReadBins(ContinuousVector*& cvLowerValues, ContinuousVector*& cvUpperValues,
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

boolean KhistoCommandLine::ReadBinaryValues(ContinuousVector*& cvValues)
{
	boolean bOk = true;
	InputBufferedFile inputFile;
	longint lFileSize;
	int nTotalValueNumber;
	int nRequestedBufferSize;
	FILE* fChunkDataFile;
	double* buffer;
	int nDoubleBufferSize;
	int nRemainingValueNumber;
	int nToReadValueNumber;
	longint lReadValueNumber;
	int nValueIndex;
	int nBufferIndex;
	ALString sTmp;

	// Initialisation des resultats
	lFileSize = 0;
	nTotalValueNumber = 0;
	cvValues = new ContinuousVector;

	// Initialisation du fichier a lire, uniquement pour pouvoir emettre des message d'erreur
	inputFile.SetFileName(sDataFileName);

	// Test de la validite de la taille du fichier
	lFileSize = FileService::GetFileSize(sDataFileName);
	if (lFileSize == 0)
	{
		AddError("empty dataset");
		bOk = false;
	}
	else if (lFileSize % sizeof(double) != 0)
	{
		AddError(sTmp + "size of binary dataset (" + LongintToReadableString(lFileSize) +
			 ") should be a multiple of " + LongintToString(sizeof(double)));
		bOk = false;
	}
	else if (lFileSize / sizeof(double) > INT_MAX)
	{
		AddInputFileError(&inputFile, -1,
				  sTmp + "too many values (" + LongintToReadableString(lFileSize / sizeof(double)) +
				      ")");
		bOk = false;
	}

	// Initialisation du vecteur de valeur resultat
	cvValues->SetSize(0);
	if (bOk)
	{
		nTotalValueNumber = (int)(lFileSize / sizeof(double));
		assert(nTotalValueNumber > 0);

		// Initialisation du vecteur de valeur
		bOk = cvValues->SetLargeSize(nTotalValueNumber);
		if (not bOk)
			AddError(sTmp + "not enought memory to load the values (number=" +
				 LongintToReadableString(nTotalValueNumber) + ") in the binary dataset");
	}

	// Lecture du fichier si OK
	if (bOk)
	{
		bOk = FileService::OpenInputBinaryFile(sDataFileName, fChunkDataFile);
		if (not bOk)
			fChunkDataFile = NULL;

		// Lecture du contenu
		if (bOk)
		{
			// Calcul de la taille du buffer de lecture
			nRequestedBufferSize = max((int)(sizeof(double) * lMB), GetHugeBufferSize());

			// Demande d'un buffer de lecture de grande taille
			buffer = (double*)GetHugeBuffer(nRequestedBufferSize);
			nDoubleBufferSize = nRequestedBufferSize / sizeof(double);

			// Lecture depuis le fichier
			nValueIndex = 0;
			nRemainingValueNumber = nTotalValueNumber;
			while (nRemainingValueNumber > 0 and not ferror(fChunkDataFile) and not feof(fChunkDataFile))
			{
				// Cas a l'interieur de la slice : Nombre d'indexes a lire dans la limite de la taille du buffer
				if (nRemainingValueNumber >= nDoubleBufferSize)
					nToReadValueNumber = nDoubleBufferSize;
				// Cas a la fin de la slice : Nombre d'indexes restant dans la slice
				else
					nToReadValueNumber = nRemainingValueNumber;

				// Lecture depuis le fichier
				lReadValueNumber =
				    (longint)fread(buffer, sizeof(double), nToReadValueNumber, fChunkDataFile);
				assert(0 <= lReadValueNumber and lReadValueNumber <= nToReadValueNumber);
				nRemainingValueNumber -= (int)lReadValueNumber;

				// Gestion des erreurs
				if (ferror(fChunkDataFile) or lReadValueNumber < nToReadValueNumber)
				{
					AddInputFileError(&inputFile, -1, "IO error when reading binary data file");
					bOk = false;
					break;
				}
				assert(lReadValueNumber == nToReadValueNumber);

				// Alimentation du vecteur de valeur
				for (nBufferIndex = 0; nBufferIndex < lReadValueNumber; nBufferIndex++)
				{
					cvValues->SetAt(nValueIndex, buffer[nBufferIndex]);
					nValueIndex++;
				}
			}
			assert(not bOk or nValueIndex == nTotalValueNumber);

			// Destruction du buffer
			DeleteHugeBuffer();

			// Fermeture du fichier
			bOk = FileService::CloseInputBinaryFile(sDataFileName, fChunkDataFile) and bOk;
		}
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
	ensure(not bOk or cvValues->GetSize() > 0);
	return bOk;
}

boolean KhistoCommandLine::ReadValues(ContinuousVector*& cvValues)
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
									  sTmp + "too many values (" +
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

boolean KhistoCommandLine::ReadValue(InputBufferedFile* inputFile, longint lRecordIndex, Continuous& cValue,
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
	if (bOk and nError != InputBufferedFile::FieldNoError)
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

boolean KhistoCommandLine::ReadFrequency(InputBufferedFile* inputFile, longint lRecordIndex,
					 longint& lCumulatedFrequency, int& nFrequency, boolean& bEndOfLine)
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
	if (bOk and nError != InputBufferedFile::FieldNoError)
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

void KhistoCommandLine::AddInputFileError(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel)
{
	ALString sTmp;

	require(inputFile != NULL);
	require(lRecordIndex >= 0);
	if (lRecordIndex > 0)
		inputFile->AddError(sTmp + "line " + LongintToReadableString(lRecordIndex) + " : " + sLabel);
	else
		inputFile->AddError(sLabel);
}

boolean KhistoCommandLine::InitializeParameters(int argc, char** argv)
{
	boolean bOk = true;
	int i;
	ALString sArgument;
	ALString sOption;
	ALString sOptionUsedTwice;

	// Reinitialisation des options
	sDataFileName = "";
	sHistogramFileName = "";
	bBinaryInput = false;
	bExploratoryAnalysis = false;
	bJsonFormat = false;

	// On recherche deja les options standard
	if (bOk)
	{
		for (i = 1; i < argc; i++)
		{
			sArgument = argv[i];

			// Version
			if (sArgument == "-v")
			{
				cout << GetClassLabel() << " " << KHISTO_VERSION << "\n ";
				cout << KHISTO_COPYRIGHT_LABEL << "\n";
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
		if (argc < 3 or argc > 6)
		{
			cout << GetClassLabel() << ": invalid number of parameters\n";
			cout << "Try '" << GetClassLabel() << "' -h' for more information.\n";
			bOk = false;
		}
	}

	// Test des options presentes
	if (bOk and argc >= 3)
	{
		// Analyse des options
		for (i = 1; i < argc - 2; i++)
		{
			sArgument = argv[i];

			// Analyse de l'option
			if (sArgument == "-b")
			{
				if (bBinaryInput)
					sOptionUsedTwice = sArgument;
				bBinaryInput = true;
			}
			else if (sArgument == "-e")
			{
				if (bExploratoryAnalysis)
					sOptionUsedTwice = sArgument;
				bExploratoryAnalysis = true;
			}
			else if (sArgument == "-j")
			{
				if (bJsonFormat)
					sOptionUsedTwice = sArgument;
				bJsonFormat = true;
			}
			// Erreur si option inconnue
			else
			{
				cout << GetClassLabel() << ": invalid option " + sArgument + "\n ";
				cout << "Try '" << GetClassLabel() << "' -h' for more information.\n";
				bOk = false;
				break;
			}

			// Erreur si option utilisee deux fois
			if (sOptionUsedTwice != "")
			{
				cout << GetClassLabel() << ": option " + sArgument + " used twice\n ";
				cout << "Try '" << GetClassLabel() << "' -h' for more information.\n";
				bOk = false;
				break;
			}
		}
	}

	// On recopie le parametrage
	if (bOk)
	{
		assert(argc >= 3 and argc <= 6);
		sDataFileName = argv[argc - 2];
		sHistogramFileName = argv[argc - 1];
		if (sDataFileName == sHistogramFileName)
		{
			bOk = false;
			cout << "Result histogram file name must be different from input values file name ("
			     << sDataFileName << ")\n";
		}
	}
	return bOk;
}

void KhistoCommandLine::ShowHelp()
{
	cout << "Usage: " << GetClassLabel() << " [OPTIONS] INPUT_FILE HISTOGRAM\n";
	cout << "\n";
	cout << "Calculates a histogram from the data in INPUT_FILE.\n";
	cout << "The output HISTOGRAM contains bin details: lower bound, upper bound, bin width, frequency, "
		"probability, and density.\n";

	// Options specialisees
	cout << "\n";
	cout << "Options:\n";
	cout << "    -b: Read input as binary (8-byte doubles) instead of text\n";
	cout << "    -e: Generate a series of histograms with increasing accuracy for exploratory analysis\n";
	cout << "    -j: Save all outputs in a single JSON file\n";

	// Options generales
	cout << "    -h: Display help message and exit\n";
	cout << "    -v: Show version information and exit\n";

	// Aide additionnelle
	cout << "\n";
	cout << "The histogram aims to be as accurate and interpretable as possible.\n";
	cout << "Using -e, all histograms are exported in order of increasing precision.\n";
	cout << "Each histogram in the series has an index suffix (e.g., \".1\"), and a .series file provides "
		"indicators for each.\n";
	cout << "Combining -j with -e consolidates all outputs into one file.\n";
}
