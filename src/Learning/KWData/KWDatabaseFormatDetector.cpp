// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseFormatDetector.h"

KWDatabaseFormatDetector::KWDatabaseFormatDetector()
{
	analysedDatabase = NULL;
	bUsingClass = true;
	InitializeInvalidSeparators();
	InitCurrentSolution('\0');
	InitBestSolution();
	bShowDetails = false;
}

KWDatabaseFormatDetector::~KWDatabaseFormatDetector() {}

void KWDatabaseFormatDetector::SetDatabase(KWDatabase* database)
{
	analysedDatabase = database;
}

KWDatabase* KWDatabaseFormatDetector::GetDatabase() const
{
	return analysedDatabase;
}

void KWDatabaseFormatDetector::SetUsingClass(boolean bValue)
{
	bUsingClass = bValue;
}

boolean KWDatabaseFormatDetector::GetUsingClass()
{
	return bUsingClass;
}

boolean KWDatabaseFormatDetector::DetectFileFormat()
{
	boolean bOk = true;
	KWClass* kwcDatabaseClass;
	RewindableInputBufferedFile inputFile;
	CharVector cvLine;
	longint lBeginPos;
	ALString sTmp;

	require(analysedDatabase != NULL);

	////////////////////////////////////////////////////////////////
	// Article sur la detection de format
	// "Wrangling Messy CSV Files by Detecting Row and TypePatterns"
	// https://arxiv.org/pdf/1811.11242.pdf
	// Objectif dans un cadre beaucoup plus general:
	//   . cell separator,
	//   . quote character,
	//   . whether or not headers exist,
	//   . how many lines to skip before the data table starts.
	// Quelques exemple de cas limites sont fournis, bases sur un mesure de consistence qui est optimisee.
	// Potentiellement une bonne source d'inspiration, mais en fait, dans le cas de Khiops, on des contextes
	// d'usage beaucoup plus spoecifique, ce qui permet d'exploiter des informations pour mettre au point
	// des heuristiques efficaces et adaptees au besoins.
	//
	// Dans le cas de Khiops, deux cas se presentent:
	//   1) disponibilite d'un dictionnaire de donnees: partout, sauf pour "Build dictionary from data table"
	//     . on peut l'exploiter pour identifier facilement s'il y a une ligne d'entete
	//     . si ligne d'entete:
	//         . attention, on en peut se base que sur les champs dont la presence est obligatoire
	//         . on obtient egalement le separator
	//         . on peut verifier l'unicite des nom des champs pour confirmation
	//         . on peut faire un parsing pour confirmation, ou pour choisir en cas d'egalite
	//     . si pas de ligne d'entete, on le dictionnaire nous fournit le nombre de champ
	//       exact et leur type
	//         . on peut determiner facilement la liste des separators candidats
	//         . ensuite, en cas d'egalite, on simule un parsing et on retient le separator
	//           entrainant le moins d'erreur
	//    2) pas de dictionnaire de donnee: cas du "Build dictionary from data table"
	//     . collecte des separators candidats (attention au caractere d'echappement '"")
	//     . tentative de parsing avec toutes les combinaisons (HeaderLineUsed, Separator)
	//        . coherence: si header line, les nom des champs doivent etre non vide et uniques
	//        . sinon, on fait un pattern de types numeriques ou categoriel par ligne
	//            . ex: NNCNC pour 5 champs, dont 3 numeriques et 3 categoriels
	//        . les patterns doivent etre coherents:
	//             . meme nombre de champs: indispensable, sinon erreur
	//             . score de similarite
	//             . on prend le separator maximisant la similarite
	//             . warning si egalite

	// Verification de la presence d'un nom de base
	if (bOk and analysedDatabase->GetDatabaseName() == "")
	{
		AddError("Missing file name");
		bOk = false;
	}

	// Verification si utilisation d'un dictionnaire
	kwcDatabaseClass = NULL;
	if (GetUsingClass() and analysedDatabase->GetClassName() != "")
	{
		// Recherche du dictionnaire associee a la base
		if (bOk)
		{
			kwcDatabaseClass =
			    KWClassDomain::GetCurrentDomain()->LookupClass(analysedDatabase->GetClassName());
			if (kwcDatabaseClass == NULL)
			{
				AddError("Missing dictionary " + analysedDatabase->GetClassName());
				bOk = false;
			}
		}

		// Test s'il y a au moins un attribut natif
		if (bOk)
		{
			if (kwcDatabaseClass->GetNativeDataItemNumber() == 0)
			{
				AddError("No native variable in dictionary " + analysedDatabase->GetClassName());
				bOk = false;
			}
		}
	}

	// Ouverture du fichier
	if (bOk)
	{
		// Ouverture
		inputFile.SetFileName(analysedDatabase->GetDatabaseName());
		bOk = inputFile.Open();
		if (not bOk)
			AddError("No possible format detection");
	}

	// On verifie que le fichier est non vide
	if (bOk)
	{
		bOk = inputFile.GetFileSize() > 0;
		if (not bOk)
			AddError("Empty file");
	}

	// Lecture du premier buffer du fichier
	inputFile.SetBufferSize(InputBufferedFile::GetMaxLineLength());
	if (bOk)
	{
		lBeginPos = 0;
		inputFile.FillInnerLines(lBeginPos);
	}

	// Erreur si premiere ligne trop longue
	if (bOk and inputFile.GetCurrentBufferSize() == 0)
	{
		AddError(sTmp + "First line too long (beyond " +
			 LongintToHumanReadableString(InputBufferedFile::GetMaxLineLength()) + ")");
		bOk = false;
	}

	// Detection de problemes d'encodage potentiels
	if (bOk)
		bOk = inputFile.CheckEncoding();

	// Detection de format
	if (bOk)
	{
		if (GetUsingClass() and analysedDatabase->GetClassName() != "")
			bOk = DetectFileFormatUsingClass(kwcDatabaseClass, &inputFile);
		else
			bOk = DetectFileFormatWithoutClass(&inputFile);
	}

	// Fermeture du fichier
	if (inputFile.IsOpened())
		inputFile.Close();
	return bOk;
}

void KWDatabaseFormatDetector::ShowFirstLines(int nMaxLineNumber)
{
	InputBufferedFile inputFile;
	boolean bOk;
	longint lBeginPos;
	ALString sTmp;

	require(analysedDatabase != NULL);
	require(nMaxLineNumber > 0);

	// Visualisation des premiere ligne si fichier specifie
	if (GetDatabase()->GetDatabaseName() != "")
	{
		// Ouverture du fichier
		inputFile.SetFileName(analysedDatabase->GetDatabaseName());
		bOk = inputFile.Open();

		// Lecture des premieres lignes
		if (bOk)
		{
			// Cas d'un fichier vide
			if (inputFile.GetFileSize() == 0)
			{
				AddSimpleMessage("File " + GetDatabase()->GetDatabaseName() + " is empty");
				AddSimpleMessage("");
			}
			// Sinon, affichage du debut du fichier
			else
			{
				// Lecture du premier buffer du fichier
				inputFile.SetBufferSize(InputBufferedFile::GetMaxLineLength());
				if (bOk)
				{
					lBeginPos = 0;
					bOk = inputFile.FillInnerLines(lBeginPos);
				}

				// Erreur si premiere ligne trop longue
				if (bOk and inputFile.GetCurrentBufferSize() == 0)
				{
					AddError(sTmp + "First line too long (beyond " +
						 LongintToHumanReadableString(InputBufferedFile::GetMaxLineLength()) +
						 ")");
					AddSimpleMessage("");
					bOk = false;
				}

				// Affichage des premiere lignes
				if (bOk)
				{
					// Detection d'erreurs de format potentielles
					bOk = inputFile.CheckEncoding();

					// Affichage des premieres lignes, meme en cas d'erreur
					ShowCurrentLines(&inputFile,
							 "First lines of " + GetDatabase()->GetDatabaseName(), false,
							 true, nMaxLineNumber);
				}
			}

			// Fermeture du fichier
			inputFile.Close();
		}
	}
}

const ALString KWDatabaseFormatDetector::GetClassLabel() const
{
	return "Database format detector";
}

const ALString KWDatabaseFormatDetector::GetObjectLabel() const
{
	if (analysedDatabase == NULL)
		return "";
	else
		return analysedDatabase->GetDatabaseName();
}

void KWDatabaseFormatDetector::UpdateDatabaseFormat(boolean bHeaderLineUsed, char cFieldSeparator)
{
	KWDataTableDriverTextFile* analysedDatabaseDriverTextFile;
	ALString sMessage;

	require(analysedDatabase != NULL);

	// Acces au driver de fichier de la base analysee pour le parametrer
	analysedDatabaseDriverTextFile = cast(KWDataTableDriverTextFile*, analysedDatabase->GetDataTableDriver());
	analysedDatabaseDriverTextFile->SetHeaderLineUsed(bHeaderLineUsed);
	analysedDatabaseDriverTextFile->SetFieldSeparator(cFieldSeparator);

	// Message utilisateur
	sMessage = "File format detected:";
	if (bHeaderLineUsed)
		sMessage += " header line and field separator ";
	else
		sMessage += " no header line and field separator ";
	if (cFieldSeparator == '\t')
		sMessage += "tabulation";
	else
	{
		sMessage += '\'';
		sMessage += cFieldSeparator;
		sMessage += '\'';
	}
	AddSimpleMessage(sMessage);
	if (bShowDetails)
		cout << "=> Update file format: header=" << bHeaderLineUsed << ", separator='"
		     << CharToString(cFieldSeparator) << "\n";
}

boolean KWDatabaseFormatDetector::DetectFileFormatUsingClass(const KWClass* kwcClass,
							     RewindableInputBufferedFile* inputFile)
{
	boolean bIsHeaderLine;
	boolean bIsNoHeaderLine;
	ALString sDisplayedSeparators;
	ALString sMessage;
	int nTotalAnalysedLineNumber;
	int nNumber;

	require(kwcClass != NULL);
	require(kwcClass->GetNativeDataItemNumber() > 0);
	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(inputFile->IsFirstPositionInFile());

	// Tentative d'initialisation avec la ligne d'entete
	bIsHeaderLine = DetectFileFormatUsingClassWithHeaderLine(kwcClass, inputFile);

	// Tentative d'initialisation sans la ligne d'entete
	bIsNoHeaderLine = false;
	if (not bIsHeaderLine)
		bIsNoHeaderLine = DetectFileFormatUsingClassWithoutHeaderLine(kwcClass, inputFile);
	assert(not(bIsHeaderLine and bIsNoHeaderLine));

	// Memorisation des resultats si on a trouve un separator correct
	if (bIsHeaderLine or bIsNoHeaderLine)
	{
		assert(cBestSeparator != '\0');
		assert(cvBestSeparatorList.GetSize() >= 1);
		assert(nBestCorrectLineNumber <= nBestAnalysedLineNumber);

		// Mise a jour du format de la base
		UpdateDatabaseFormat(bIsHeaderLine, cBestSeparator);

		// Emission de warning si necessaire
		if (cvBestSeparatorList.GetSize() > 1 or
		    nBestMatchedFieldNumber < kwcClass->GetNativeDataItemNumber() or nBestUnknownFieldNumber > 0 or
		    nBestCorrectLineNumber < nBestAnalysedLineNumber)
		{
			// Calcul d'un libelle dedies aux separators alternatifs possibles
			if (cvBestSeparatorList.GetSize() > 1)
			{
				// Tri des separators par priorite decroissante
				SortSeparators(&cvBestSeparatorList);
				sDisplayedSeparators = BuildSeparatorsLabel(&cvBestSeparatorList, cBestSeparator);
			}

			// Cas ou d'autre separateurs sont possible
			if (cvBestSeparatorList.GetSize() > 1)
			{
				// Particularisation du message dans le cas d'un seul champ
				if (nBestFieldNumber == 1)
				{
					sMessage += "one single field has been detected in the file and a default "
						    "field separator has been choosen";
				}
				// Message dans le cas general
				else
				{
					nNumber = cvBestSeparatorList.GetSize() - 1;
					sMessage += IntToString(nNumber);
					sMessage += " other field separator" + Plural("", nNumber) + " ";
					sMessage += Plural("is", nNumber) + " possible: " + sDisplayedSeparators;
				}
			}

			// Cas il manque des champs
			if (nBestMatchedFieldNumber < kwcClass->GetNativeDataItemNumber())
			{
				assert(bIsHeaderLine);
				nNumber = kwcClass->GetNativeDataItemNumber() - nBestMatchedFieldNumber;
				if (sMessage != "")
					sMessage += ", ";
				sMessage += IntToString(nNumber);
				sMessage +=
				    " variable" + Plural("", nNumber) + " in dictionary " + kwcClass->GetName() + " ";
				sMessage += Plural("is", nNumber) + " missing in the header line";
			}

			// Cas ou il y a des champs en trop
			if (nBestUnknownFieldNumber > 0)
			{
				assert(bIsHeaderLine);
				nNumber = nBestUnknownFieldNumber;
				if (sMessage != "")
					sMessage += ", ";
				sMessage += IntToString(nNumber);
				sMessage += " field name" + Plural("", nNumber) + " in the header line ";
				sMessage += Plural("is", nNumber) + " missing in dictionary " + kwcClass->GetName();
			}

			// Cas avec des lignes erronees
			if (nBestCorrectLineNumber < nBestAnalysedLineNumber)
			{
				// La premiere ligne doit etre prise en compte dans le total des lignes analysees
				nTotalAnalysedLineNumber = nBestAnalysedLineNumber + 1;
				nNumber = nBestAnalysedLineNumber - nBestCorrectLineNumber;
				if (sMessage != "")
					sMessage += ", ";
				sMessage += IntToString(nNumber);
				sMessage += " record" + Plural("", nNumber) + " ";
				sMessage += Plural("is", nNumber) + " not correctly parsed in the first ";
				sMessage += IntToString(nTotalAnalysedLineNumber);
				sMessage += " record" + Plural("", nTotalAnalysedLineNumber) + " of the file";
			}

			// Emission du message
			AddWarning(sMessage);

			// Affichage des premieres lignes sauf sans le cas des lignes erronnees,
			// sauf si elles sont toutes incorrectes
			if (cvBestSeparatorList.GetSize() > 1 or
			    nBestMatchedFieldNumber < kwcClass->GetNativeDataItemNumber() or
			    nBestUnknownFieldNumber > 0 or nBestCorrectLineNumber <= nMinCorrectLineNumber)
				ShowHeadLines(inputFile);
		}
	}
	// Message d'erreur sinon
	else
	{
		AddError("No detected file format compatible with the dictionary " + GetDatabase()->GetClassName());
		ShowHeadLines(inputFile);
	}
	return (bIsHeaderLine or bIsNoHeaderLine);
}

boolean KWDatabaseFormatDetector::DetectFileFormatUsingClassWithHeaderLine(const KWClass* kwcClass,
									   RewindableInputBufferedFile* inputFile)
{
	boolean bIsHeaderLine;
	StringVector svMandatoryDataItemNames;
	StringVector svNativeAttributeNames;
	CharVector cvLine;
	boolean bLineTooLong;
	KWCharFrequencyVector cfvAttributeNames;
	KWCharFrequencyVector cfvFirstLine;
	KWCharFrequencyVector cfvCandidateSeparators;
	KWHeaderLineAnalyser headerLineAnaliser;
	IntVector ivLineFieldNumbers;
	int i;
	char cSeparator;

	require(kwcClass != NULL);
	require(kwcClass->GetNativeDataItemNumber() > 0);
	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(inputFile->GetFileSize() > 0);
	require(inputFile->GetCurrentBufferSize() > 0);

	// Export des noms des attributs obligatoires du dictionnaire
	// On ne se concentre que sur les attributs obligatoires, car la lectre des fichiers est tolerante
	// aux attributs absents du fichier s'ils ne sont pas utilises
	BuildMandatoryDataItemNames(kwcClass, &svMandatoryDataItemNames);

	// Export des noms des attributs natifs du dictionnaire
	kwcClass->ExportNativeFieldNames(&svNativeAttributeNames);
	assert(svNativeAttributeNames.GetSize() >= svMandatoryDataItemNames.GetSize());

	// Dans le cas particulier ou aucun attribut n'est obligatoire, on les prend tous pour detecter le format
	// Sinon, on est incapable d'identifier un caractere separateur de champs
	if (svMandatoryDataItemNames.GetSize() == 0)
		svMandatoryDataItemNames.CopyFrom(&svNativeAttributeNames);

	// Calcul des effectifs des caracteres utilises dans les nom des variables
	cfvAttributeNames.InitializeFromStringVector(&svMandatoryDataItemNames);
	if (bShowDetails)
		cout << "Variables names: " << cfvAttributeNames << "\n";

	// Lecture de la premiere ligne du fichier
	inputFile->RewindBuffer();
	inputFile->GetNextLine(&cvLine, bLineTooLong);

	// Erreur et retour immediat dans le cas d'une ligne trop longue
	if (bLineTooLong)
	{
		AddError("First line, " + InputBufferedFile::GetLineTooLongErrorLabel());
		return false;
	}

	// Calcul des effectifs des caracteres utilises dans la premiere ligne du fichier
	cfvFirstLine.InitializeFromBuffer(&cvLine);
	if (bShowDetails)
		cout << "First line: " << cfvFirstLine << "\n";

	// Initialisation des separateurs candidats avec les caracteres de la premiere ligne
	cfvCandidateSeparators.CopyFrom(&cfvFirstLine);
	bIsHeaderLine = not cfvCandidateSeparators.IsZero();

	// On soustrait les caracteres des champs obligatoire
	if (bIsHeaderLine)
	{
		bIsHeaderLine = cfvCandidateSeparators.IsGreaterOrEqual(&cfvAttributeNames);
		if (bIsHeaderLine)
			cfvCandidateSeparators.Substract(&cfvAttributeNames);
		bIsHeaderLine = not cfvCandidateSeparators.IsZero();
	}

	// On filtre les caracteres invalides
	if (bIsHeaderLine)
	{
		cfvCandidateSeparators.FilterChars(&cfvInvalidSeparators);
		bIsHeaderLine = not cfvCandidateSeparators.IsZero();
		if (bShowDetails)
			cout << "Candidate separators: " << cfvCandidateSeparators << "\n";
	}

	// Parcours des caractere separateurs candidats avec choix du meilleur critere selon un critere hierarchique
	InitBestSolution();
	if (bIsHeaderLine)
	{
		// Boucle d'analyse sur les separateurs candidats
		for (i = 0; i < cfvCandidateSeparators.GetSize(); i++)
		{
			cSeparator = cfvCandidateSeparators.GetCharAtIndex(i);

			// On doit avoir assez de separateurs pour les champs obligatoires
			if (cfvCandidateSeparators.GetFrequencyAt(cSeparator) >=
				svMandatoryDataItemNames.GetSize() - 1 and
			    cfvInvalidSeparators.GetFrequencyAt(cSeparator) == 0)
			{
				if (bShowDetails)
					cout << "Test header line separator '" << CharToString(cSeparator) << "'\n";

				// Initialisation de la solution courante
				InitCurrentSolution(cSeparator);

				// On se remet au debut du buffer de lecture en parametrant le separator
				inputFile->RewindBuffer();
				inputFile->SetFieldSeparator(cCurrentSeparator);
				bIsHeaderLine = true;
				require(inputFile->IsFirstPositionInFile());

				// Test si une header line est possible
				if (bIsHeaderLine)
				{
					bIsHeaderLine = headerLineAnaliser.FillFromFile(inputFile);
					nCurrentFieldNumber = headerLineAnaliser.GetSize();
					if (bShowDetails)
						cout << "\tHeader line (" << headerLineAnaliser.GetSize()
						     << "): " << headerLineAnaliser << "\n";
				}

				// Test si elle contient tous les champs obligatoires
				if (bIsHeaderLine)
				{
					nCurrentMatchedFieldNumber =
					    headerLineAnaliser.ComputePresentNameNumber(&svMandatoryDataItemNames);
					nCurrentUnknownFieldNumber =
					    headerLineAnaliser.GetSize() - nCurrentMatchedFieldNumber;
					bIsHeaderLine =
					    nCurrentMatchedFieldNumber == svMandatoryDataItemNames.GetSize();
					if (bShowDetails)
						cout << "\tMantatory fields (" << svMandatoryDataItemNames.GetSize()
						     << "): " << bIsHeaderLine << "\n";
				}

				// Recherche du nombre de champ natifs reconnus
				if (bIsHeaderLine and
				    svNativeAttributeNames.GetSize() > svMandatoryDataItemNames.GetSize())
				{
					assert(svNativeAttributeNames.GetSize() >= 1);
					nCurrentMatchedFieldNumber =
					    headerLineAnaliser.ComputePresentNameNumber(&svNativeAttributeNames);
					nCurrentUnknownFieldNumber =
					    headerLineAnaliser.GetSize() - nCurrentMatchedFieldNumber;
					assert(nCurrentMatchedFieldNumber >= svMandatoryDataItemNames.GetSize());
					if (bShowDetails)
						cout << "\tNative fields (" << svNativeAttributeNames.GetSize()
						     << "): " << bIsHeaderLine << "\n";
				}

				// Test si les premieres lignes du fichier sont conformes au format
				if (bIsHeaderLine)
				{
					CollectLineFieldNumbers(inputFile, nMaxAnalysedLineNumber, &ivLineFieldNumbers);
					assert(nCurrentAnalysedLineNumber == -1 or
					       nCurrentAnalysedLineNumber == ivLineFieldNumbers.GetSize());
					nCurrentAnalysedLineNumber = ivLineFieldNumbers.GetSize();
					nCurrentCorrectLineNumber =
					    ComputeCorrectLineNumber(&ivLineFieldNumbers, headerLineAnaliser.GetSize());
					if (bShowDetails)
						cout << "\tCorrect line number: " << nCurrentCorrectLineNumber << "/"
						     << nCurrentAnalysedLineNumber << "\n";
				}

				// Recherche de la priorite du separator
				if (bIsHeaderLine)
				{
					nCurrentSeparatorPriority = ComputeSeparatorPriority(cCurrentSeparator);
				}

				// Memorisation du separateur
				if (bIsHeaderLine)
					UpdateBestSolution();
			}
		}
	}
	return cBestSeparator != '\0';
}

boolean KWDatabaseFormatDetector::DetectFileFormatUsingClassWithoutHeaderLine(const KWClass* kwcClass,
									      RewindableInputBufferedFile* inputFile)
{
	boolean bIsFirstLineValid;
	CharVector cvLine;
	boolean bLineTooLong;
	KWCharFrequencyVector cfvFirstLine;
	KWCharFrequencyVector cfvCandidateSeparators;
	IntVector ivLineFieldNumbers;
	int i;
	char cSeparator;

	require(kwcClass != NULL);
	require(kwcClass->GetNativeDataItemNumber() > 0);
	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(inputFile->GetFileSize() > 0);
	require(inputFile->GetCurrentBufferSize() > 0);

	// Initialisation a nouveau d'une meilleure solution
	InitBestSolution();

	// Lecture de la premiere ligne du fichier
	inputFile->RewindBuffer();
	inputFile->GetNextLine(&cvLine, bLineTooLong);

	// Erreur et retour immediat dans le cas d'une ligne trop longue
	if (bLineTooLong)
	{
		AddError("First line, " + InputBufferedFile::GetLineTooLongErrorLabel());
		return false;
	}

	// Calcul des effectifs des caracteres utilises dans la premiere ligne du fichier
	cfvFirstLine.InitializeFromBuffer(&cvLine);
	if (bShowDetails)
		cout << "First line: " << cfvFirstLine << "\n";

	// Initialisation des separateurs candidats avec les caracteres de la premiere ligne
	cfvCandidateSeparators.CopyFrom(&cfvFirstLine);
	if (bShowDetails)
		cout << "First line candidate separators: " << cfvCandidateSeparators << "\n";

	// Test si la premiere ligne peut contenir assez de separateurs
	bIsFirstLineValid = true;
	if (bIsFirstLineValid)
	{
		assert(kwcClass->GetNativeDataItemNumber() > 0);

		// Cas particulier d'un seul champ natif
		if (kwcClass->GetNativeDataItemNumber() == 1)
			cfvCandidateSeparators.InitializeFrequencies(1);
		// Cas general avec au moins deux champs
		else
			cfvCandidateSeparators.FilterBelowFrequency(kwcClass->GetNativeDataItemNumber() - 1);
		bIsFirstLineValid = not cfvCandidateSeparators.IsZero();
		if (bShowDetails)
			cout << "First line frequent candidate separators: " << cfvCandidateSeparators << "\n";
	}

	// On filtre les caracteres invalides
	if (bIsFirstLineValid)
	{
		cfvCandidateSeparators.FilterChars(&cfvInvalidSeparators);
		bIsFirstLineValid = not cfvCandidateSeparators.IsZero();
		if (bShowDetails)
			cout << "First line filtered candidate separators: " << cfvCandidateSeparators << "\n";
	}

	// Boucle d'analyse sur les separateurs candidats
	if (bIsFirstLineValid)
	{
		for (i = 0; i < cfvCandidateSeparators.GetSize(); i++)
		{
			cSeparator = cfvCandidateSeparators.GetCharAtIndex(i);

			// On doit avoir assez de separateurs pour tous les champs du fichier
			if (cfvCandidateSeparators.GetFrequencyAt(cSeparator) >=
				kwcClass->GetNativeDataItemNumber() - 1 and
			    cfvInvalidSeparators.GetFrequencyAt(cSeparator) == 0)
			{
				if (bShowDetails)
					cout << "Test no header line separator '" << CharToString(cSeparator) << "'\n";

				// Initialisation de la solution courante
				InitCurrentSolution(cSeparator);

				// On se remet au debut du buffer de lecture en parametrant le separator
				inputFile->RewindBuffer();
				inputFile->SetFieldSeparator(cCurrentSeparator);
				bIsFirstLineValid = true;
				assert(inputFile->IsFirstPositionInFile());

				// Test si la premiere ligne est valide
				if (bIsFirstLineValid)
				{
					nCurrentFieldNumber = ComputeLineFieldNumber(inputFile);
					bIsFirstLineValid = nCurrentFieldNumber == kwcClass->GetNativeDataItemNumber();
					if (bShowDetails)
						cout << "\tFirst line field number: " << nCurrentFieldNumber << " ("
						     << bIsFirstLineValid << ")\n";
				}

				// On continue l'analyse si au moins la premiere ligne est valide
				if (bIsFirstLineValid)
				{
					// On memorise que l'on a trouve tous les champs
					nCurrentMatchedFieldNumber = nCurrentFieldNumber;
					nCurrentUnknownFieldNumber = 0;

					// Test si les premieres lignes du fichier sont conformes au format
					if (bIsFirstLineValid)
					{
						CollectLineFieldNumbers(inputFile, nMaxAnalysedLineNumber,
									&ivLineFieldNumbers);
						assert(nCurrentAnalysedLineNumber == -1 or
						       nCurrentAnalysedLineNumber == ivLineFieldNumbers.GetSize());
						nCurrentAnalysedLineNumber = ivLineFieldNumbers.GetSize();
						nCurrentCorrectLineNumber =
						    ComputeCorrectLineNumber(&ivLineFieldNumbers, nCurrentFieldNumber);
						if (nCurrentAnalysedLineNumber > 0 and nCurrentCorrectLineNumber == 0)
							bIsFirstLineValid = false;
						if (bShowDetails)
							cout << "\tCorrect line number: " << nCurrentCorrectLineNumber
							     << "/" << nCurrentAnalysedLineNumber << "\n";
					}

					// Recherche de la priorite du separator
					if (bIsFirstLineValid)
					{
						nCurrentSeparatorPriority = ComputeSeparatorPriority(cCurrentSeparator);
					}

					// Memorisation du separateur
					if (bIsFirstLineValid)
						UpdateBestSolution();
				}
			}
		}
	}
	return cBestSeparator != '\0';
}

boolean KWDatabaseFormatDetector::DetectFileFormatWithoutClass(RewindableInputBufferedFile* inputFile)
{
	boolean bErrorDisplayed = false;
	boolean bIsValid = true;
	boolean bIsHeaderLine;
	KWCharFrequencyVector cfvCandidateSeparators;
	KWCharFrequencyVector cfvLineMinCharFrequencies;
	KWCharFrequencyVector cfvLineMaxCharFrequencies;
	KWHeaderLineAnalyser headerLineAnaliser;
	ALString sMandatoryChars;
	IntVector ivLineFieldNumbers;
	int nAnalysedLineNumber;
	CharVector cvLine;
	boolean bLineTooLong;
	int i;
	char cSeparator;
	ALString sDisplayedSeparators;
	ALString sMessage;
	int nTotalAnalysedLineNumber;
	int nNumber;

	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(inputFile->GetFileSize() > 0);
	require(inputFile->GetCurrentBufferSize() > 0);

	// Initialisation d'une meilleure solution
	InitBestSolution();

	// Lecture de la premiere ligne du fichier
	inputFile->RewindBuffer();
	inputFile->GetNextLine(&cvLine, bLineTooLong);

	// Erreur et retour immediat dans le cas d'une ligne trop longue
	if (bLineTooLong)
	{
		AddError("First line, " + InputBufferedFile::GetLineTooLongErrorLabel());
		return false;
	}

	// Test s'il y a plus d'une ligne
	if (bIsValid)
	{
		if (inputFile->IsFileEnd())
		{
			bIsValid = false;
			if (not bErrorDisplayed)
			{
				AddError("File contains one single line");
				bErrorDisplayed = true;
			}
		}
		if (bShowDetails)
			cout << "First line read: " << bIsValid << "\n";
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Analyse des premieres lignes du fichier pour collecter les stats d'effectifs des caracteres par ligne
	if (bIsValid)
	{
		inputFile->RewindBuffer();
		nAnalysedLineNumber = CollectCharFrequenciesLineStats(
		    inputFile, nMaxAnalysedLineNumber, &cfvLineMinCharFrequencies, &cfvLineMaxCharFrequencies);

		// Erreur si au plus une ligne non vide
		if (nAnalysedLineNumber <= 1)
		{
			bIsValid = false;
			if (nAnalysedLineNumber == 0)
				sMessage = "Cannot detect non-empty lines after scanning ";
			else
				sMessage = "Cannot detect more than one non-empty line after scanning ";
			if (inputFile->IsFileEnd())
				sMessage += "the file";
			else
			{
				sMessage += "the first ";
				sMessage += LongintToHumanReadableString(inputFile->GetCurrentBufferSize());
				sMessage += " bytes of the file ";
			}
			if (not bErrorDisplayed)
			{
				AddError(sMessage);
				bErrorDisplayed = true;
			}
		}
		if (bShowDetails)
			cout << "First lines scanned (" << nAnalysedLineNumber << "): " << bIsValid << "\n";
	}

	// Initialisation des separateurs candidats avec les caracteres apparaissant dans toutes les lignes
	if (bIsValid)
	{
		cfvCandidateSeparators.CopyFrom(&cfvLineMinCharFrequencies);
		bIsValid = not cfvCandidateSeparators.IsZero();
		if (bShowDetails)
			cout << "Initial candidate separators: " << cfvCandidateSeparators << "\n";
	}

	// On filtre les caracteres invalides
	if (bIsValid)
	{
		cfvCandidateSeparators.FilterChars(&cfvInvalidSeparators);
		bIsValid = not cfvCandidateSeparators.IsZero();
		if (bShowDetails)
			cout << "Filtered candidate separators: " << cfvCandidateSeparators << "\n";

		// Cas particulier ou il n'y plus de caracteres candidats: cela correspond a un seul champ par ligne
		// Dans ce cas, on va prend tous les separateurs comme candidats, en excluant les caracteres
		// rencontres dans le fichier
		if (not bIsValid)
		{
			bIsValid = true;

			// Initialisation avec tous les separateurs possibles
			cfvCandidateSeparators.InitializeFrequencies(1);

			// Supression des caracteres utilises puis filtrage
			cfvCandidateSeparators.Not(&cfvLineMaxCharFrequencies);
			cfvCandidateSeparators.FilterChars(&cfvInvalidSeparators);
			if (bShowDetails)
				cout << "Filtered candidate separators for one single field: " << cfvCandidateSeparators
				     << "\n";
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Recherche de separateurs candidats compatibles avec le nombre de champ de la premiere ligne
	if (bIsValid)
	{
		// Initialisation d'une meilleure solution
		InitBestSolution();

		// Boucle de recherche des meilleurs separateurs
		for (i = 0; i < cfvCandidateSeparators.GetSize(); i++)
		{
			cSeparator = cfvCandidateSeparators.GetCharAtIndex(i);

			// On n'exploite que les separateurs arrivant au moins une fois
			if (cfvCandidateSeparators.GetFrequencyAt(cSeparator) > 0)
			{
				if (bShowDetails)
					cout << "Test line separator for field number consistency: '"
					     << CharToString(cSeparator) << "'\n";

				// Initialisation de la solution courante
				bIsValid = true;
				InitCurrentSolution(cSeparator);

				// On se remet au debut du buffer de lecture en parametrant le separator
				inputFile->RewindBuffer();
				inputFile->SetFieldSeparator(cCurrentSeparator);

				// Collecte des nombres de champ par ligne
				CollectLineFieldNumbers(inputFile, nMaxAnalysedLineNumber, &ivLineFieldNumbers);

				// Collecte des indicateurs de performance utilises
				assert(nCurrentAnalysedLineNumber == -1 or
				       nCurrentAnalysedLineNumber == ivLineFieldNumbers.GetSize());
				nCurrentAnalysedLineNumber = ivLineFieldNumbers.GetSize();
				nCurrentFieldNumber = ivLineFieldNumbers.GetAt(0);
				nCurrentCorrectLineNumber =
				    ComputeCorrectLineNumber(&ivLineFieldNumbers, nCurrentFieldNumber);

				// Initialisation des autres indicateurs a des valeurs par defaut
				nCurrentMatchedFieldNumber = nCurrentFieldNumber;
				nCurrentUnknownFieldNumber = 0;

				// Recherche de la priorite du separator
				nCurrentSeparatorPriority = ComputeSeparatorPriority(cCurrentSeparator);

				// Validite si au moins la moitie des lignes sont correctes
				bIsValid = nCurrentCorrectLineNumber > nCurrentAnalysedLineNumber / 2;

				// Memorisation du separateur
				if (bIsValid)
					UpdateBestSolution();
			}
		}

		// Memorisation des separateurs candidats restant
		cfvCandidateSeparators.Initialize();
		if (cvBestSeparatorList.GetSize() > 0)
		{
			for (i = 0; i < cvBestSeparatorList.GetSize(); i++)
			{
				cSeparator = cvBestSeparatorList.GetAt(i);
				cfvCandidateSeparators.SetFrequencyAt(cSeparator, 1);
			}
		}
		bIsValid = cfvCandidateSeparators.GetTotalFrequency() > 0;
		if (bShowDetails)
			cout << "Candidate separators after field number detection: " << cfvCandidateSeparators << "\n";
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Analyse des separateurs candidats restants en recherchant ceux conduisant au typage le plus homogene des
	// champs
	if (bIsValid)
	{
		// Initialisation d'une meilleure solution
		InitBestSolution();

		// Boucle de recherche des meilleurs separateurs
		for (i = 0; i < cfvCandidateSeparators.GetSize(); i++)
		{
			cSeparator = cfvCandidateSeparators.GetCharAtIndex(i);

			// On n'exploite que les separateurs arrivant au moins une fois
			if (cfvCandidateSeparators.GetFrequencyAt(cSeparator) > 0)
			{
				if (bShowDetails)
					cout << "Test line separator for type consistency '" << CharToString(cSeparator)
					     << "'\n";

				// Initialisation de la solution courante
				bIsValid = true;
				InitCurrentSolution(cSeparator);

				// On se remet au debut du buffer de lecture en parametrant le separator
				inputFile->RewindBuffer();
				inputFile->SetFieldSeparator(cCurrentSeparator);

				// Recherche du nombre de champs de la premiere ligne
				nCurrentFieldNumber = ComputeLineFieldNumber(inputFile);

				// Calcul d'un indicateur de coherence des types
				nCurrentTypeConsistency =
				    ComputeTypeConsistency(inputFile, nMaxAnalysedLineNumber, nCurrentFieldNumber,
							   nCurrentAnalysedLineNumber, nCurrentCorrectLineNumber);
				assert(nCurrentTypeConsistency >= nCurrentFieldNumber);

				// Initialisation des autres indicateurs a des valeurs par defaut
				nCurrentMatchedFieldNumber = nCurrentFieldNumber;
				nCurrentUnknownFieldNumber = 0;

				// Recherche de la priorite du separator
				nCurrentSeparatorPriority = ComputeSeparatorPriority(cCurrentSeparator);

				// Memorisation du separateur
				if (bIsValid)
					UpdateBestSolution();
			}
		}

		// Memorisation des separateurs candidats restant
		cfvCandidateSeparators.Initialize();
		if (cvBestSeparatorList.GetSize() > 0)
		{
			for (i = 0; i < cvBestSeparatorList.GetSize(); i++)
			{
				cSeparator = cvBestSeparatorList.GetAt(i);
				cfvCandidateSeparators.SetFrequencyAt(cSeparator, 1);
			}
		}
		bIsValid = cfvCandidateSeparators.GetTotalFrequency() > 0;
		if (bShowDetails)
			cout << "Candidate separators after type consistency evaluation: " << cfvCandidateSeparators
			     << "\n";
	}

	// Recherche s'il y a une ligne d'entete
	bIsHeaderLine = false;
	if (bIsValid)
	{
		assert(cBestSeparator != '\0');

		// On se remet au debut du buffer de lecture en parametrant le separator
		inputFile->RewindBuffer();
		inputFile->SetFieldSeparator(cBestSeparator);

		// Test si une header line est possible
		bIsHeaderLine = headerLineAnaliser.FillFromFile(inputFile);
		assert(not bIsHeaderLine or headerLineAnaliser.GetSize() == nBestFieldNumber);
		if (bShowDetails)
			cout << "\tHeader line (" << headerLineAnaliser.GetSize() << "): " << headerLineAnaliser
			     << "\n";

		// On impose egalement qu'elle doivent comporter au moins un caractere obligatoire par nom de champ
		if (bIsHeaderLine)
		{
			sMandatoryChars = GetHeaderLineMandatoryChars();
			bIsHeaderLine = (headerLineAnaliser.ComputeValidNameNumber(sMandatoryChars) ==
					 headerLineAnaliser.GetSize());
			if (bShowDetails)
				cout << "\tHeader line contains mandatory chars:" << bIsHeaderLine << "\n";
		}

		// On verifie en fin que l'on est compatible avec les contraintes des dictionnaires
		if (bIsHeaderLine)
		{
			for (i = 0; i < headerLineAnaliser.GetSize(); i++)
			{
				bIsHeaderLine =
				    KWClass::CheckName(headerLineAnaliser.GetAt(i), KWClass::Attribute, NULL);
				if (not bIsHeaderLine)
					break;
			}
			if (bShowDetails)
				cout << "\tHeader line with valid identifiers:" << bIsHeaderLine << "\n";
		}
	}

	// Memorisation des resultats si on a trouve un separator correct
	if (bIsValid)
	{
		assert(cBestSeparator != '\0');
		assert(cvBestSeparatorList.GetSize() >= 1);
		assert(nBestCorrectLineNumber <= nBestAnalysedLineNumber);
		assert(nBestTypeConsistency >= nBestFieldNumber);

		// Mise a jour du format de la base
		UpdateDatabaseFormat(bIsHeaderLine, cBestSeparator);

		// Emission de warning si necessaire
		if (cvBestSeparatorList.GetSize() > 1 or nBestCorrectLineNumber < nBestAnalysedLineNumber)
		{
			// Calcul d'un libelle dedies aux separators alternatifs possibles
			if (cvBestSeparatorList.GetSize() > 1)
			{
				// Tri des separators par priorite decroissante
				SortSeparators(&cvBestSeparatorList);
				sDisplayedSeparators = BuildSeparatorsLabel(&cvBestSeparatorList, cBestSeparator);
			}

			// Cas ou d'autre separateurs sont possible
			if (cvBestSeparatorList.GetSize() > 1)
			{
				// Particularisation du message dans le cas d'un seul champ
				if (nBestFieldNumber == 1)
				{
					sMessage += "one single field has been detected in the file and a default "
						    "field separator has been choosen";
				}
				// Message dans le cas general
				else
				{
					nNumber = cvBestSeparatorList.GetSize() - 1;
					sMessage += IntToString(nNumber);
					sMessage += " other field separator" + Plural("", nNumber) + " ";
					sMessage += Plural("is", nNumber) + " possible: " + sDisplayedSeparators;
				}
			}

			// Cas avec des lignes erronees
			if (nBestCorrectLineNumber < nBestAnalysedLineNumber)
			{
				// La premiere ligne doit etre prise en compte dans le total des lignes analysees
				nTotalAnalysedLineNumber = nBestAnalysedLineNumber + 1;
				nNumber = nBestAnalysedLineNumber - nBestCorrectLineNumber;
				if (sMessage != "")
					sMessage += ", ";
				sMessage += IntToString(nNumber);
				sMessage += " record" + Plural("", nNumber) + " ";
				sMessage += Plural("is", nNumber) + " not correctly parsed in the first ";
				sMessage += IntToString(nTotalAnalysedLineNumber);
				sMessage += " record" + Plural("", nTotalAnalysedLineNumber) + " of the file";
			}

			// Emission du message
			AddWarning(sMessage);

			// Affichage des premieres lignes sauf sans le cas des lignes erronnees,
			// sauf si elles sont toutes incorrectes
			if (cvBestSeparatorList.GetSize() > 1 or nBestCorrectLineNumber <= nMinCorrectLineNumber)
				ShowHeadLines(inputFile);
		}
	}
	// Message d'erreur sinon
	else
	{
		if (not bErrorDisplayed)
		{
			AddError("No consistent file format detected");
			ShowHeadLines(inputFile);
		}
	}
	return cBestSeparator != '\0';
}

void KWDatabaseFormatDetector::ShowHeadLines(RewindableInputBufferedFile* inputFile)
{
	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(nMaxShownLineNumber > 0);
	require(nMaxShownLineLength > 0);
	require(nMinCorrectLineNumber < nMaxShownLineNumber);

	// On revient au debut du fichier
	inputFile->RewindBuffer();

	// Affichage des premieres lignes
	ShowCurrentLines(inputFile, "First lines of file " + inputFile->GetFileName(), true, true, nMaxShownLineNumber);
}

void KWDatabaseFormatDetector::ShowCurrentLines(InputBufferedFile* inputFile, const ALString& sTitle,
						boolean bEmptyLineBefore, boolean bEmptyLineAfter, int nMaxLineNumber)
{
	int nLine;
	int i;
	char c;
	CharVector cvLine;
	boolean bLineTooLong;
	ALString sLine;

	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(nMaxLineNumber > 0);
	require(nMaxShownLineLength > 0);

	// Affichage de chaque ligne
	nLine = 0;
	while (not inputFile->IsBufferEnd())
	{
		inputFile->GetNextLine(&cvLine, bLineTooLong);

		// Ajout des caracteres a afficher
		sLine = "";
		for (i = 0; i < cvLine.GetSize(); i++)
		{
			c = cvLine.GetAt(i);
			if (c == '\r' or c == '\n')
				break;
			else
			{
				if (sLine.GetLength() == nMaxShownLineLength)
				{
					sLine += "...";
					break;
				}
				else
					sLine += c;
			}
		}

		// Affichage d'un message de debut si demande
		if (nLine == 0)
		{
			if (bEmptyLineBefore)
				AddSimpleMessage("");
			if (sTitle != "")
				AddSimpleMessage(sTitle);
		}

		// Warning si ligne trop longue
		if (bLineTooLong)
			AddWarning("Next line, " + InputBufferedFile::GetLineTooLongErrorLabel());

		// Affichage de la ligne
		AddSimpleMessage(sLine);

		// Test du nombre max de lignes
		nLine++;
		if (nLine == nMaxLineNumber or inputFile->IsBufferEnd())
		{
			if (not inputFile->IsFileEnd())
				AddSimpleMessage("...");

			// Ajout d'un message de fin si demande
			if (bEmptyLineAfter)
				AddSimpleMessage("");
			break;
		}
	}
}

const ALString KWDatabaseFormatDetector::Plural(const ALString& sSource, int nNumber) const
{
	if (nNumber > 1)
	{
		if (sSource == "")
			return "s";
		else if (sSource == "is")
			return "are";
		else if (sSource == "has")
			return "have";
		else
			return sSource;
	}
	else
		return sSource;
}

int KWDatabaseFormatDetector::ComputeSeparatorPriority(char cSeparator) const
{
	const ALString sPreferredSeparators = "\t;,| ";
	int nPriority;

	// Recherche de la position dans les separateurs preferes
	nPriority = sPreferredSeparators.Find(cSeparator);

	// Si non trouve, on prend le le caractere lui meme d'abord dans sa plage ascii, puis dans la plage ascii etendue
	if (nPriority == -1)
	{
		if (p_isprint(cSeparator))
		{
			if (cSeparator >= 0)
				nPriority = 1000 + cSeparator;
			else
				nPriority = 2000 + cSeparator;
		}
		else
		{
			if (cSeparator >= 0)
				nPriority = 3000 + cSeparator;
			else
				nPriority = 4000 + cSeparator;
		}
	}
	ensure(nPriority >= 0);
	return nPriority;
}

void KWDatabaseFormatDetector::SortSeparators(CharVector* cvSeparators) const
{
	IntVectorSorter sorter;
	IntVector ivSeparatorPriorities;
	int i;
	char cSeparator;
	int nSeparatorPriority;
	int nInitialIndex;
	CharVector cvSeparatorsCopy;

	require(cvSeparators != NULL);

	// Collecte des priorite de chaque separator
	for (i = 0; i < cvSeparators->GetSize(); i++)
	{
		cSeparator = cvSeparators->GetAt(i);
		nSeparatorPriority = ComputeSeparatorPriority(cSeparator);
		ivSeparatorPriorities.Add(nSeparatorPriority);
	}

	// Utilisation d'un trieur de vecteur d'entiers pour trier les separator par priorite
	sorter.SortVector(&ivSeparatorPriorities);
	cvSeparatorsCopy.CopyFrom(cvSeparators);
	for (i = 0; i < cvSeparators->GetSize(); i++)
	{
		nInitialIndex = sorter.GetInitialIndexAt(i);

		// On remet le separator dans l'ordre
		cvSeparators->SetAt(i, cvSeparatorsCopy.GetAt(nInitialIndex));
	}
}

const ALString KWDatabaseFormatDetector::BuildSeparatorsLabel(CharVector* cvSeparators, char cSeparator) const
{
	ALString sSeparatorsLabel;
	const int nMaxDisplayedCharNumber = 5;
	int i;
	int nDisplayedCharNumber;
	char cValue;

	require(cvSeparators != NULL);

	// Affichage du contenu dans la limite d'un nombre max de caracteres
	nDisplayedCharNumber = 0;
	for (i = 0; i < cvSeparators->GetSize(); i++)
	{
		cValue = cvSeparators->GetAt(i);

		// On ne traite pas le meilleur separateur
		if (cValue != cSeparator)
		{
			// Insertion d'un seperateur de valeur
			if (nDisplayedCharNumber > 0)
				sSeparatorsLabel += "  ";

			// Cas ou la limite est atteinte
			if (nDisplayedCharNumber == nMaxDisplayedCharNumber)
			{
				sSeparatorsLabel += " ...";
				break;
			}
			else
			{
				sSeparatorsLabel += '\'';
				sSeparatorsLabel += CharToString(cValue);
				sSeparatorsLabel += '\'';
			}
			nDisplayedCharNumber++;
		}
	}
	return sSeparatorsLabel;
}

boolean KWDatabaseFormatDetector::IsInvalidSeparator(char c) const
{
	return c == '\0' or c == '\n' or c == '\r' or c == '"' or isalnum(c);
}

void KWDatabaseFormatDetector::InitializeInvalidSeparators()
{
	int i;
	char cValue;

	// Initialisation avec des effectifs a 1 pour les separateurs valides, a 0 sinon
	if (cfvInvalidSeparators.GetTotalFrequency() == 0)
	{
		cfvInvalidSeparators.Initialize();
		for (i = 0; i < cfvInvalidSeparators.GetSize(); i++)
		{
			cValue = cfvInvalidSeparators.GetCharAtIndex(i);
			if (IsInvalidSeparator(cValue))
				cfvInvalidSeparators.SetFrequencyAt(cValue, 1);
		}
	}
}

void KWDatabaseFormatDetector::BuildMandatoryDataItemNames(const KWClass* kwcClass,
							   StringVector* svMandatoryDataItemNames) const
{
	NumericKeyDictionary nkdAllUsedAttributes;
	NumericKeyDictionary nkdAllUsedClasses;
	NumericKeyDictionary nkdAllLoadedClasses;
	NumericKeyDictionary nkdAllNativeClasses;
	ObjectArray oaAllUsedAttributes;
	NumericKeyDictionary nkdAllUsedAttributeBlocks;
	ObjectArray oaAllUsedAttributeBlocks;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWDerivationRule* derivationRule;
	int nAttribute;
	int nAttributeBlock;

	require(kwcClass != NULL);
	require(svMandatoryDataItemNames != NULL);
	require(svMandatoryDataItemNames->GetSize() == 0);

	// Recherche des attributs necessaires pour cette classe
	// On a ici une tolerance en n'imposant pas que tous les attributs natifs soient presents dans le header
	// On n'exige que la presence des attributs utilises (Used et Loaded), ainsi que les attributs necessaires
	// au calcul des attributs derives utilises
	for (nAttribute = 0; nAttribute < kwcClass->GetLoadedAttributeNumber(); nAttribute++)
	{
		attribute = kwcClass->GetLoadedAttributeAt(nAttribute);

		// Memorisation si l'attribut est natif
		if (attribute->IsNative())
			nkdAllUsedAttributes.SetAt(attribute, cast(Object*, attribute));

		// Analyse de la regle de derivation
		// Dans le cas d'un bloc, il faut en effet la reanalyser pour chaque attribut du bloc
		// pour detecter les attributs utilises des blocs potentiellement en operande
		derivationRule = attribute->GetAnyDerivationRule();
		if (derivationRule != NULL)
			derivationRule->BuildAllUsedAttributes(attribute, &nkdAllUsedAttributes);
	}

	// Collecte des attributs utilises recursivement dans les operandes des regles ou via les
	// attribut cles necessaires dans le cas multi-table
	kwcClass->BuildAllUsedAttributes(&nkdAllUsedAttributes, &nkdAllUsedClasses, &nkdAllLoadedClasses,
					 &nkdAllNativeClasses);

	// Export des attributs utilises
	nkdAllUsedAttributes.ExportObjectArray(&oaAllUsedAttributes);

	// On garde les attribut natifs de la classe en cours
	for (nAttribute = 0; nAttribute < oaAllUsedAttributes.GetSize(); nAttribute++)
	{
		attribute = cast(KWAttribute*, oaAllUsedAttributes.GetAt(nAttribute));
		if (attribute->IsNative() and attribute->GetParentClass() == kwcClass)
		{
			// Memorisation si attribut
			if (not attribute->IsInBlock())
				svMandatoryDataItemNames->Add(attribute->GetName());
			// Enregistrement du bloc sinon
			else
				nkdAllUsedAttributeBlocks.SetAt(attribute->GetAttributeBlock(),
								attribute->GetAttributeBlock());
		}
	}

	// Export des blocs utilises
	nkdAllUsedAttributeBlocks.ExportObjectArray(&oaAllUsedAttributeBlocks);

	// On garde les attribut natifs de la classe en cours
	for (nAttributeBlock = 0; nAttributeBlock < oaAllUsedAttributeBlocks.GetSize(); nAttributeBlock++)
	{
		attributeBlock = cast(KWAttributeBlock*, oaAllUsedAttributeBlocks.GetAt(nAttributeBlock));
		svMandatoryDataItemNames->Add(attributeBlock->GetName());
	}
}

int KWDatabaseFormatDetector::ComputeCorrectLineNumber(IntVector* ivLineFieldNumbers, int nExpectedFieldNumber) const
{
	int nResult;
	int i;
	require(ivLineFieldNumbers != NULL);

	nResult = 0;
	if (nExpectedFieldNumber != -1)
	{
		for (i = 0; i < ivLineFieldNumbers->GetSize(); i++)
		{
			if (ivLineFieldNumbers->GetAt(i) == nExpectedFieldNumber)
				nResult++;
		}
	}
	return nResult;
}

void KWDatabaseFormatDetector::CollectLineFieldNumbers(RewindableInputBufferedFile* inputFile, int nMaxLineNumber,
						       IntVector* ivLineFieldNumbers) const
{
	int nLineFieldNumber;

	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(not inputFile->IsBufferEnd());
	require(nMaxLineNumber >= 0);
	require(ivLineFieldNumbers != NULL);

	// Boucle de lecture sur le fichier, au plus jusqu'a la fin du buffer
	ivLineFieldNumbers->SetSize(0);
	while (not inputFile->IsBufferEnd() and ivLineFieldNumbers->GetSize() < nMaxLineNumber)
	{
		nLineFieldNumber = ComputeLineFieldNumber(inputFile);

		// On prend en compte les lignes non vide uniquement
		// Les ligne erronnees (retour -1) ou non vides sont prise en compte
		if (nLineFieldNumber != 0)
			ivLineFieldNumbers->Add(nLineFieldNumber);
	}
}

int KWDatabaseFormatDetector::ComputeLineFieldNumber(RewindableInputBufferedFile* inputFile) const
{
	int nFieldNumber;
	boolean bEndOfLine;
	boolean bLineTooLong;
	char* sField;
	int nFieldLength;
	int nFieldError;
	boolean bIsError;

	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(not inputFile->IsBufferEnd());

	// Lecture des champs jusqu'a la fin de la ligne
	bEndOfLine = false;
	nFieldNumber = 0;
	bIsError = false;
	while (not bEndOfLine)
	{
		bEndOfLine = inputFile->GetNextField(sField, nFieldLength, nFieldError, bLineTooLong);

		// Memorisation des erreurs
		if (bLineTooLong or (nFieldError != InputBufferedFile::FieldNoError))
		{
			bIsError = true;

			// On saute les champ jusqu'a la fin de ligne
			if (not bEndOfLine)
			{
				inputFile->SkipLastFields(bLineTooLong);
				bEndOfLine = true;
			}
			break;
		}

		// On compte 0 champ dans le cas particulier d'une ligne vide
		if (bEndOfLine and nFieldNumber == 0 and sField[0] == '\0' and
		    nFieldError == InputBufferedFile::FieldNoError)
			break;

		// Incrementation du nombre de champs trouves
		nFieldNumber++;
	}
	assert(bEndOfLine);

	// On renvoie -1 si une erreur a ete detectee
	if (bIsError)
		nFieldNumber = -1;
	return nFieldNumber;
}

const ALString KWDatabaseFormatDetector::GetHeaderLineMandatoryChars() const
{
	ALString sMandatoryChars;
	int i;
	char c;

	// On prend tous les carateres alphabetiques, plus le underscore
	sMandatoryChars = "_";
	for (i = 0; i < 256; i++)
	{
		c = (char)i;
		if (isalpha(c))
			sMandatoryChars += c;
	}
	return sMandatoryChars;
}

int KWDatabaseFormatDetector::ComputeTypeConsistency(RewindableInputBufferedFile* inputFile, int nMaxLineNumber,
						     int nExpectedFieldNumber, int& nAnalysedLineNumber,
						     int& nCorrectLineNumber) const
{
	int nTypeConsistencyCriterion;
	StringVector svFields;
	ObjectArray oaLineFieldTypes;
	CharVector* cvFieldTypes;
	CharVector cvFieldMostFrequentTypes;
	IntVector ivTypeFrequencies;
	int nLine;
	int nField;
	char cType;
	int nBestFrequency;
	boolean bOk;

	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(not inputFile->IsBufferEnd());
	require(nMaxLineNumber >= 0);
	require(nExpectedFieldNumber >= 0);
	require(nExpectedFieldNumber == 0 or nMaxLineNumber <= INT_MAX / nExpectedFieldNumber);

	// Boucle de lecture sur le fichier, au plus jusqu'a la fin du buffer
	nLine = 0;
	nCorrectLineNumber = 0;
	nAnalysedLineNumber = 0;
	while (not inputFile->IsBufferEnd() and nLine < nMaxLineNumber)
	{
		// Lecture des champs d'un ligne
		bOk = ReadLineFields(inputFile, nExpectedFieldNumber, &svFields);
		nAnalysedLineNumber++;

		// Analyse de la ligne
		if (bOk)
		{
			nCorrectLineNumber++;

			// Creation d'un nouveau vecteur de types
			cvFieldTypes = new CharVector;
			oaLineFieldTypes.Add(cvFieldTypes);

			// Alimentation des types champs de la ligne
			ComputeFieldTypes(&svFields, cvFieldTypes);
		}

		// Ligne suivante
		nLine++;
	}

	// Calcul des types les plus frequents par champ
	cvFieldMostFrequentTypes.SetSize(nExpectedFieldNumber);
	ivTypeFrequencies.SetSize(KWType::Unknown);
	for (nField = 0; nField < nExpectedFieldNumber; nField++)
	{
		// Reinitialisation des effectifs par type
		ivTypeFrequencies.Initialize();

		// Alimentation des effectif par type
		for (nLine = 0; nLine < oaLineFieldTypes.GetSize(); nLine++)
		{
			cvFieldTypes = cast(CharVector*, oaLineFieldTypes.GetAt(nLine));
			cType = cvFieldTypes->GetAt(nField);
			assert(KWType::Check(cType));
			ivTypeFrequencies.UpgradeAt(cType, 1);
		}

		// Memorisation du type le plus frequent
		nBestFrequency = -1;
		for (cType = 0; cType < ivTypeFrequencies.GetSize(); cType++)
		{
			if (ivTypeFrequencies.GetAt(cType) >= nBestFrequency)
			{
				cvFieldMostFrequentTypes.SetAt(nField, cType);
				nBestFrequency = ivTypeFrequencies.GetAt(cType);
			}
		}
	}

	// Calcul du critere en comptant le nombre total de champ ayant le type le plus frequent
	nTypeConsistencyCriterion = 0;
	for (nLine = 0; nLine < oaLineFieldTypes.GetSize(); nLine++)
	{
		cvFieldTypes = cast(CharVector*, oaLineFieldTypes.GetAt(nLine));

		for (nField = 0; nField < nExpectedFieldNumber; nField++)
		{
			if (cvFieldTypes->GetAt(nField) == cvFieldMostFrequentTypes.GetAt(nField))
				nTypeConsistencyCriterion++;
		}
	}

	// Nettoyage
	oaLineFieldTypes.DeleteAll();
	return nTypeConsistencyCriterion;
}

void KWDatabaseFormatDetector::ComputeFieldTypes(const StringVector* svFields, CharVector* cvFieldTypes) const
{
	int i;
	Continuous cValue;
	int nError;

	require(svFields != NULL);
	require(cvFieldTypes != NULL);

	// Boucle sur les champs pour identifier leur type
	cvFieldTypes->SetSize(svFields->GetSize());
	for (i = 0; i < svFields->GetSize(); i++)
	{
		// On met un type Symbol par defaut
		cvFieldTypes->SetAt(i, KWType::Symbol);

		// On passe au type Continuous si la conversion est possible
		nError = KWContinuous::StringToContinuousError(svFields->GetAt(i), cValue);
		if (nError == KWContinuous::NoError)
			cvFieldTypes->SetAt(i, KWType::Continuous);
	}
}

boolean KWDatabaseFormatDetector::ReadLineFields(RewindableInputBufferedFile* inputFile, int nExpectedFieldNumber,
						 StringVector* svFields) const
{
	boolean bOk;
	int nFieldNumber;
	boolean bEndOfLine;
	boolean bLineTooLong;
	char* sField;
	int nFieldLength;
	int nFieldError;

	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(not inputFile->IsBufferEnd());
	require(nExpectedFieldNumber >= 0);
	require(svFields != NULL);

	// Taillage du vecteur de sortie
	svFields->SetSize(nExpectedFieldNumber);

	// Lecture des champs jusqu'a la fin de la ligne
	bEndOfLine = false;
	nFieldNumber = 0;
	bOk = true;
	while (not bEndOfLine)
	{
		bEndOfLine = inputFile->GetNextField(sField, nFieldLength, nFieldError, bLineTooLong);

		// Memorisation des erreurs
		if (bLineTooLong or (nFieldError != InputBufferedFile::FieldNoError))
		{
			bOk = false;
			break;
		}

		// Erreur si in depasse le nombre de champ attendus
		if (nFieldNumber >= nExpectedFieldNumber)
		{
			bOk = false;
			break;
		}

		// Memorisation de la valeur si pas d'erreur
		if (bOk)
			svFields->SetAt(nFieldNumber, sField);
		nFieldNumber++;
	}

	// Erreur si on a pas le bon nombre de champs
	if (nFieldNumber != nExpectedFieldNumber)
		bOk = false;
	return bOk;
}

int KWDatabaseFormatDetector::CollectCharFrequenciesLineStats(RewindableInputBufferedFile* inputFile,
							      int nMaxLineNumber,
							      KWCharFrequencyVector* cfvLineMinCharFrequencies,
							      KWCharFrequencyVector* cfvLineMaxCharFrequencies) const
{
	int nAnalysedLineNumber;
	boolean bIsEmptyLine;
	CharVector cvLine;
	boolean bLineTooLong;
	KWCharFrequencyVector cfvLineCharFrequencies;

	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(not inputFile->IsBufferEnd());
	require(nMaxLineNumber >= 0);
	require(cfvLineMinCharFrequencies != NULL);
	require(cfvLineMaxCharFrequencies != NULL);

	// Boucle de lecture sur le fichier, au plus jusqu'a la fin du buffer
	nAnalysedLineNumber = 0;
	cfvLineMinCharFrequencies->Initialize();
	cfvLineMaxCharFrequencies->Initialize();
	while (not inputFile->IsBufferEnd() and nAnalysedLineNumber < nMaxLineNumber)
	{
		inputFile->GetNextLine(&cvLine, bLineTooLong);

		// Detection de ligne vide
		bIsEmptyLine = false;
		if (cvLine.GetSize() <= 2)
		{
			if (cvLine.GetSize() == 0)
				bIsEmptyLine = true;
			else if (cvLine.GetSize() == 1)
				bIsEmptyLine = cvLine.GetAt(0) == '\n';
			else
				bIsEmptyLine = (cvLine.GetAt(0) == '\r' and cvLine.GetAt(1) == '\n');
		}

		// Analyse uniquement des lignes non vides et pas trop longues
		if (not bIsEmptyLine and not bLineTooLong)
		{
			nAnalysedLineNumber++;

			// Collecte des stats d'utilisation de caractere de la ligne
			cfvLineCharFrequencies.InitializeFromBuffer(&cvLine);

			// On prend le minimum par rapport aux stats precedentes, sauf pour la premiere fois
			if (nAnalysedLineNumber == 1)
				cfvLineMinCharFrequencies->CopyFrom(&cfvLineCharFrequencies);
			else
				cfvLineMinCharFrequencies->Min(&cfvLineCharFrequencies);

			// On prend le maximum directement
			cfvLineMaxCharFrequencies->Max(&cfvLineCharFrequencies);
		}
	}
	return nAnalysedLineNumber;
}

void KWDatabaseFormatDetector::InitCurrentSolution(char cSeparator)
{
	cCurrentSeparator = cSeparator;
	nCurrentMatchedFieldNumber = 0;
	nCurrentUnknownFieldNumber = INT_MAX;
	nCurrentCorrectLineNumber = 0;
	nCurrentTypeConsistency = 0;
	nCurrentFieldNumber = INT_MAX;
	nCurrentSeparatorPriority = INT_MAX;
	nCurrentAnalysedLineNumber = -1;
}

void KWDatabaseFormatDetector::InitBestSolution()
{
	cvBestSeparatorList.SetSize(0);
	cBestSeparator = '\0';
	nBestMatchedFieldNumber = 0;
	nBestUnknownFieldNumber = INT_MAX;
	nBestCorrectLineNumber = 0;
	nBestTypeConsistency = 0;
	nBestFieldNumber = INT_MAX;
	nBestSeparatorPriority = INT_MAX;
	nBestAnalysedLineNumber = -1;
}

boolean KWDatabaseFormatDetector::UpdateBestSolution()
{
	int nDiffSeparator;

	require(cCurrentSeparator != '\0');
	require(nCurrentCorrectLineNumber <= nCurrentAnalysedLineNumber);
	require(nCurrentFieldNumber != INT_MAX);
	require(nCurrentSeparatorPriority != INT_MAX);
	require(nCurrentTypeConsistency >= 0);
	require(nCurrentAnalysedLineNumber != -1);

	// On memorise le separateur de toutes facon
	cvBestSeparatorList.Add(cCurrentSeparator);
	if (bShowDetails)
		cout << "-> possible separator: " << CharToString(cCurrentSeparator) << "\n";

	// On regarde si on a trouve un meilleur separateur
	// On evalue si on a une evaluation stricte ou non
	nDiffSeparator = 1;
	if (cvBestSeparatorList.GetSize() > 1)
	{
		nDiffSeparator = 0;

		// Criteres dans le cas de l'utilisation d'une classe
		if (GetUsingClass())
		{
			if (nDiffSeparator == 0)
				nDiffSeparator = nCurrentMatchedFieldNumber - nBestMatchedFieldNumber;
			if (nDiffSeparator == 0)
				nDiffSeparator = -(nCurrentUnknownFieldNumber - nBestUnknownFieldNumber);
		}

		// Autre criteres d'amelioration
		if (nDiffSeparator == 0)
			nDiffSeparator = nCurrentCorrectLineNumber - nBestCorrectLineNumber;
		if (nDiffSeparator == 0)
			nDiffSeparator = nCurrentTypeConsistency - nBestTypeConsistency;
		if (nDiffSeparator == 0)
			nDiffSeparator = -(nCurrentFieldNumber - nBestFieldNumber);
		if (nDiffSeparator == 0)
			nDiffSeparator = -(nCurrentSeparatorPriority - nBestSeparatorPriority);
	}

	// Memorisation du meilleur separateur
	if (nDiffSeparator > 0)
	{
		cBestSeparator = cCurrentSeparator;
		nBestMatchedFieldNumber = nCurrentMatchedFieldNumber;
		nBestUnknownFieldNumber = nCurrentUnknownFieldNumber;
		nBestCorrectLineNumber = nCurrentCorrectLineNumber;
		nBestTypeConsistency = nCurrentTypeConsistency;
		nBestFieldNumber = nCurrentFieldNumber;
		nBestSeparatorPriority = nCurrentSeparatorPriority;
		nBestAnalysedLineNumber = nCurrentAnalysedLineNumber;
		if (bShowDetails)
		{
			cout << "=> best separator: " << CharToString(cBestSeparator);
			cout << "( " << nBestMatchedFieldNumber;
			cout << ", " << nBestUnknownFieldNumber;
			cout << ", " << nBestCorrectLineNumber;
			cout << ", " << nBestTypeConsistency;
			cout << ", " << nBestFieldNumber;
			cout << ", " << nBestSeparatorPriority;
			cout << ", " << nBestAnalysedLineNumber;
			cout << ")\n";
		}
	}
	return (nDiffSeparator > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe RewindableInputBufferedFile

KWHeaderLineAnalyser::KWHeaderLineAnalyser() {}

KWHeaderLineAnalyser::~KWHeaderLineAnalyser() {}

void KWHeaderLineAnalyser::Initialize()
{
	svFieldNames.SetSize(0);
	odFieldNames.RemoveAll();
}

boolean KWHeaderLineAnalyser::FillFromFile(InputBufferedFile* inputFile)
{
	boolean bOk = true;
	boolean bEndOfLine;
	boolean bLineTooLong;
	char* sField;
	int nFieldLength;
	int nFieldError;
	ALString sFieldName;

	require(inputFile != NULL);
	require(inputFile->IsOpened());
	require(inputFile->IsFirstPositionInFile());
	require(not inputFile->IsBufferEnd());

	// Initialiszation
	Initialize();

	// Lecture des champs jusqu'a la fin de la ligne
	bEndOfLine = false;
	while (not bEndOfLine)
	{
		bEndOfLine = inputFile->GetNextField(sField, nFieldLength, nFieldError, bLineTooLong);

		// Arret si erreur
		if (bLineTooLong or (nFieldError != InputBufferedFile::FieldNoError))
		{
			bOk = false;
			break;
		}
		// Arret si champ vide
		else if (sField[0] == '\0')
		{
			bOk = false;
			break;
		}
		// Prise en compte du champ sinon
		else
		{
			// Arret si champs deja existant
			if (odFieldNames.Lookup(sField) != NULL)
			{
				bOk = false;
				break;
			}
			// Memorisation du champ sinon
			else
			{
				odFieldNames.SetAt(sField, &odFieldNames);
				svFieldNames.Add(sField);
			}
		}
	}
	// Nettoyage en cas d'erreur
	if (not bOk)
		Initialize();
	return bOk;
}

int KWHeaderLineAnalyser::GetSize() const
{
	return svFieldNames.GetSize();
}

const ALString& KWHeaderLineAnalyser::GetAt(int i) const
{
	return svFieldNames.GetAt(i);
}

boolean KWHeaderLineAnalyser::IsPresent(const ALString& sValue) const
{
	return odFieldNames.Lookup(sValue) != NULL;
}

int KWHeaderLineAnalyser::ComputePresentNameNumber(StringVector* svNames) const
{
	int nPresentNameNumber;
	int i;

	require(svNames != NULL);

	// Comptage des noms presents dans la ligne d'entete
	nPresentNameNumber = 0;
	for (i = 0; i < svNames->GetSize(); i++)
	{
		if (IsPresent(svNames->GetAt(i)))
			nPresentNameNumber++;
	}
	return nPresentNameNumber++;
}

int KWHeaderLineAnalyser::ComputeValidNameNumber(const ALString& sMandatoryChars) const
{
	int nValidNameNumber;
	ALString sFieldName;
	int n;

	// Comptage des noms de la ligne d'entet comportant au moins un caractere obligatoire
	nValidNameNumber = 0;
	for (n = 0; n < svFieldNames.GetSize(); n++)
	{
		sFieldName = svFieldNames.GetAt(n);
		if (sFieldName.FindOneOf(sMandatoryChars) != -1)
			nValidNameNumber++;
	}
	return nValidNameNumber++;
}

void KWHeaderLineAnalyser::Write(ostream& ost) const
{
	const int nMaxDisplayedNameNumber = 3;
	int i;

	// Affichage des premeirs champs sous forme d'un libelle
	ost << "\"";
	for (i = 0; i < svFieldNames.GetSize(); i++)
	{
		if (i > 0)
			ost << "; ";
		if (i >= nMaxDisplayedNameNumber)
		{
			ost << "...";
			break;
		}
		else
			ost << svFieldNames.GetAt(i);
	}
	ost << "\"";
}

void KWHeaderLineAnalyser::FullWrite(ostream& ost) const
{
	int i;

	// Affichage des permeirs champs sous forme d'un libelle
	for (i = 0; i < svFieldNames.GetSize(); i++)
	{
		if (i > 0)
			ost << "\t";
		ost << svFieldNames.GetAt(i);
	}
	ost << "\n";
}

longint KWHeaderLineAnalyser::GetUsedMemory() const
{
	return sizeof(KWHeaderLineAnalyser) + odFieldNames.GetUsedMemory() + svFieldNames.GetUsedMemory();
	;
}

const ALString KWHeaderLineAnalyser::GetClassLabel() const
{
	return "Header line analyser";
}

const ALString KWHeaderLineAnalyser::GetObjectLabel() const
{
	if (svFieldNames.GetSize() == 0)
		return "";
	else
		return "\"" + svFieldNames.GetAt(0) + "..\"";
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe RewindableInputBufferedFile

RewindableInputBufferedFile::RewindableInputBufferedFile() {}

RewindableInputBufferedFile::~RewindableInputBufferedFile() {}

void RewindableInputBufferedFile::RewindBuffer()
{
	require(IsOpened());

	// Il faut tenir compte du fait qu'un BOM a peut-etre ete rencontre
	assert(nBufferStartInCache == nUTF8BomSkippedCharNumber);
	nPositionInCache = nBufferStartInCache;
	nLastBolPositionInCache = nBufferStartInCache;
	nCurrentLineIndex = 1;
	bLastFieldReachEol = false;
}
