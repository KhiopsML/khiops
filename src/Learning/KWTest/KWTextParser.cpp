// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTextParser.h"

/////////////////////////////////////////////////////////////////////////////////
// Text parser

const int KWTextParser::nFileBufferMaxSize = 100000;

KWTextParser::KWTextParser()
{
	// Initialisation du buffer de fichier
	sFileBuffer = NewCharArray(nFileBufferMaxSize);
	sWordBuffer = NewCharArray(nFileBufferMaxSize);
	nLineOffset = 0;
	nTextOffset = 0;
	nMaxWordLength = 8;
	nMinWordFrequency = 3;
}

KWTextParser::~KWTextParser()
{
	odReferenceWords.DeleteAll();

	// Destruction des buffers
	DeleteCharArray(sFileBuffer);
	DeleteCharArray(sWordBuffer);
}

void KWTextParser::SetReferenceFileName(const ALString& sFileName)
{
	sReferenceFileName = sFileName;
}

const ALString& KWTextParser::GetReferenceFileName() const
{
	return sReferenceFileName;
}

void KWTextParser::SetInputFileName(const ALString& sFileName)
{
	sInputFileName = sFileName;
}

const ALString& KWTextParser::GetInputFileName() const
{
	return sInputFileName;
}

void KWTextParser::SetOutputFileName(const ALString& sFileName)
{
	sOutputFileName = sFileName;
}

const ALString& KWTextParser::GetOutputFileName() const
{
	return sOutputFileName;
}

void KWTextParser::SetMaxWordLength(int nValue)
{
	require(nMaxWordLength >= 1);
	nMaxWordLength = nValue;
}

int KWTextParser::GetMaxWordLength() const
{
	return nMaxWordLength;
}

void KWTextParser::SetMinWordFrequency(int nValue)
{
	require(nMinWordFrequency >= 1);
	nMinWordFrequency = nValue;
}

int KWTextParser::GetMinWordFrequency() const
{
	return nMinWordFrequency;
}

void KWTextParser::Transcode(boolean bBuildClassificationFile, boolean bBuildCoclusteringFile)
{
	FILE* fInput;
	FILE* fOutput;
	ObjectDictionary odInputWords;
	ObjectArray oaReferenceWords;
	int nWord;
	SampleObject* word;
	boolean bOk;

	// Construction du dictionnaire de tous les mots
	BuildReferenceWordDictionary();
	cout << "\tReference texts\t" << nTextOffset << endl;
	cout << "\tReference lines\t" << nLineOffset << endl;

	// Filtrage du dictionnaire de reference
	FilterWordDictionary(&odReferenceWords, true);

	// Affichage des mots
	cout << "\tMax word length\t" << GetMaxWordLength() << endl;
	cout << "\tMin word frequency\t" << GetMinWordFrequency() << endl;
	cout << "\tReference words\t" << odReferenceWords.GetCount() << endl;
	// WriteWords(&odReferenceWords, cout);

	// Tri du dictionnaire des mots de reference dans un tableau
	odReferenceWords.ExportObjectArray(&oaReferenceWords);
	oaReferenceWords.SetCompareFunction(SampleObjectCompareWord);
	oaReferenceWords.Sort();

	// Ouverture du fichier d'entree
	fInput = p_fopen(sInputFileName, "rb");
	sCurrentFileName = sInputFileName;
	nLineOffset = 0;
	nTextOffset = 0;

	// Ouverture du fichier de sortie
	fOutput = p_fopen(sOutputFileName, "wt");

	// Construction d'un fichier de classification
	if (bBuildClassificationFile)
	{
		// Ligne d'entete
		fputs("Text", fOutput);
		for (nWord = 0; nWord < oaReferenceWords.GetSize(); nWord++)
		{
			word = cast(SampleObject*, oaReferenceWords.GetAt(nWord));
			fputs("\t", fOutput);
			fputs(word->GetString(), fOutput);
		}
		fputs("\n", fOutput);

		// Analyse de tous les textes du fichier
		while (not feof(fInput))
		{
			// Construction d'un dictionnaire de mots local
			bOk = BuildTextWordDictionary(fInput, &odInputWords);

			// Filtrage du dictionnaire
			FilterWordDictionary(&odInputWords, false);

			// Affichage des indicateurs pour tous les mots, si pas fin de fichier
			if (bOk)
			{
				fputs(IntToString(nTextOffset), fOutput);
				for (nWord = 0; nWord < oaReferenceWords.GetSize(); nWord++)
				{
					word = cast(SampleObject*, oaReferenceWords.GetAt(nWord));
					fputs("\t", fOutput);
					if (odInputWords.Lookup(word->GetString()))
						fputs("1", fOutput);
					else
						fputs("0", fOutput);
				}
				fputs("\n", fOutput);
			}

			// Nettoyage
			odInputWords.DeleteAll();
		}
	}

	// Construction d'un fichier de coclustering
	if (bBuildCoclusteringFile)
	{
		// Ligne d'entete
		fputs("Text\tWord\n", fOutput);

		// Analyse de tous les textes du fichier
		while (not feof(fInput))
		{
			// Construction d'un dictionnaire de mots local
			bOk = BuildTextWordDictionary(fInput, &odInputWords);

			// Filtrage du dictionnaire
			FilterWordDictionary(&odInputWords, false);

			// Affichage d'une paire (Text, Word) par mot du texte, si pas fin de fichier
			if (bOk)
			{
				for (nWord = 0; nWord < oaReferenceWords.GetSize(); nWord++)
				{
					word = cast(SampleObject*, oaReferenceWords.GetAt(nWord));
					if (odInputWords.Lookup(word->GetString()))
					{
						fputs(IntToString(nTextOffset), fOutput);
						fputs("\t", fOutput);
						fputs(word->GetString(), fOutput);
						fputs("\n", fOutput);
					}
				}
			}

			// Nettoyage
			odInputWords.DeleteAll();
		}
	}

	// Statistiques
	cout << "\tInput texts\t" << nTextOffset << endl;
	cout << "\tInput lines\t" << nLineOffset << endl;

	// Fermeture
	fclose(fInput);
	fclose(fOutput);

	// Nettoyage
	odReferenceWords.DeleteAll();
	sCurrentFileName = "";
	nLineOffset = 0;
	nTextOffset = 0;
}

const ALString KWTextParser::GetClassLabel() const
{
	return "Text parser";
}

const ALString KWTextParser::GetObjectLabel() const
{
	return sCurrentFileName + " text " + IntToString(nTextOffset) + " line " + IntToString(nLineOffset);
}

void KWTextParser::TextParserTest(int argc, char** argv)
{
	KWTextParser textParserTest;

	// Aide si mauvais nombre de parametres
	if (argc != 8)
	{
		cout << "TextParserTest <BuildClassificationFile (0,1)>  <BuildCoclusteringFile (0,1)>";
		cout << " <MaxWordLength> <MinWordFrequency>";
		cout << " <reference file name> <input file name> <output file name>" << endl;
	}
	else
	// Lancement des tests sinon
	{
		// Recuperation des parametres
		textParserTest.SetMaxWordLength(StringToInt(argv[3]));
		textParserTest.SetMinWordFrequency(StringToInt(argv[4]));
		textParserTest.SetReferenceFileName(argv[5]);
		textParserTest.SetInputFileName(argv[6]);
		textParserTest.SetOutputFileName(argv[7]);

		// Lancement des traitements
		if (StringToInt(argv[1]) + StringToInt(argv[2]) == 2)
			cout << "Please, build only one output file at a time" << endl;
		else
			textParserTest.Transcode(StringToInt(argv[1]), StringToInt(argv[2]));
	}
}

boolean KWTextParser::BuildTextWordDictionary(FILE* fInput, ObjectDictionary* odWords)
{
	// return BuildTextWordDictionaryNOVA(fInput, odWords);
	return BuildTextWordDictionarySingleLine(fInput, odWords);
}

boolean KWTextParser::BuildTextWordDictionaryNOVA(FILE* fInput, ObjectDictionary* odWords)
{
	boolean bExportFormatedText = false;
	const ALString sBeginOfText = "Subject:";
	const ALString sEndOfText = "$$$$";
	ALString sTmp;
	int nLineIndex;
	static int nTextIndex = 0;

	require(odWords != NULL);

	//
	nTextIndex++;
	if (bExportFormatedText)
		cout << nTextIndex << "\t\"";

	// Lecture des lignes du fichier jusqu'a trouver une fin de texte
	nLineIndex = 0;
	nTextOffset++;
	while (not feof(fInput))
	{
		// Lecture d'une ligne
		ReadLine(sFileBuffer, nFileBufferMaxSize, fInput);
		nLineOffset++;
		nLineIndex++;

		//
		if (bExportFormatedText)
		{
			if (strncmp(sFileBuffer, sEndOfText, sBeginOfText.GetLength()) == 0)
				cout << "\"\n";
			else if (nLineIndex != 2)
				cout << sFileBuffer << "\n";
		}

		// Test de coherence si premiere ligne
		if (nLineIndex == 1)
		{
			// Cas particulier de fin de fichier
			if (strlen(sFileBuffer) == 0 and feof(fInput))
			{
				// On arrete le parsing, en decomptant le texte
				nTextOffset--;
				return false;
			}
			else if (strncmp(sFileBuffer, sBeginOfText, sBeginOfText.GetLength()) != 0)
				AddError(sTmp + "Wrong begin of text" + " (" + sFileBuffer + ")");
		}

		// Analyse de la fin de la premiere ligne
		if (nLineIndex == 1 and strncmp(sFileBuffer, sBeginOfText, sBeginOfText.GetLength()) == 0)
		{
			BuildLineWordDictionary(&(sFileBuffer[sBeginOfText.GetLength()]), odWords);
			continue;
		}

		// Saut de la deuxieme ligne
		if (nLineIndex == 2)
			continue;

		// Test si derniere ligne
		if (strncmp(sFileBuffer, sEndOfText, sBeginOfText.GetLength()) == 0)
			break;

		// Analyse des autres lignes
		BuildLineWordDictionary(sFileBuffer, odWords);
	}
	return true;
}

boolean KWTextParser::BuildTextWordDictionarySingleLine(FILE* fInput, ObjectDictionary* odWords)
{
	ALString sTmp;
	int nLineIndex;

	require(odWords != NULL);

	// Lecture des lignes du fichier jusqu'a trouver une fin de texte
	nLineIndex = 0;
	nTextOffset++;
	if (not feof(fInput))
	{
		// Lecture d'une ligne
		ReadLine(sFileBuffer, nFileBufferMaxSize, fInput);
		nLineOffset++;
		nLineIndex++;

		// Analyse de la ligne
		BuildLineWordDictionary(sFileBuffer, odWords);
	}
	return true;
}

void KWTextParser::BuildLineWordDictionary(char* sLine, ObjectDictionary* odWords)
{
	boolean bFilterAccentuation = false;
	boolean bInspectChars = false;
	boolean bInspectWords = false;
	int i;
	char cLineChar;
	int nWordChar;
	int nWordState;
	enum
	{
		StateEmpty,
		StateAlnumWord,
		StatePunctWord
	};

	require(odWords != NULL);

	// Affichage de la ligne si inspect des mots ou des caracteres
	if (bInspectChars or bInspectWords)
		cout << sLine << endl;

	// Parcours des caracteres a analyser
	i = 0;
	nWordChar = 0;
	nWordState = StateEmpty;
	if (bInspectChars)
		cout << "Offset\tChar\tisalnum\tispunct\tisspace\tisprint" << endl;
	while (sLine[i] != '\0')
	{
		cLineChar = sLine[i];
		if (bInspectChars)
		{
			cout << i << "\t" << cLineChar << "\t" << isalnum(cLineChar) << "\t" << ispunct(cLineChar)
			     << "\t" << isspace(cLineChar) << "\t" << isprint(cLineChar) << endl;
		}

		// Transformation des caracteres accentues
		if (bFilterAccentuation)
		{
			if (cLineChar == 'é')
				cLineChar = 'e';
			else if (cLineChar == 'è')
				cLineChar = 'e';
			else if (cLineChar == 'ê')
				cLineChar = 'e';
			else if (cLineChar == 'ë')
				cLineChar = 'e';
			else if (cLineChar == 'É')
				cLineChar = 'e';
			else if (cLineChar == 'à')
				cLineChar = 'a';
			else if (cLineChar == 'â')
				cLineChar = 'a';
			else if (cLineChar == 'ä')
				cLineChar = 'a';
			else if (cLineChar == 'ô')
				cLineChar = 'o';
			else if (cLineChar == 'ö')
				cLineChar = 'o';
			else if (cLineChar == 'î')
				cLineChar = 'i';
			else if (cLineChar == 'ï')
				cLineChar = 'i';
			else if (cLineChar == 'û')
				cLineChar = 'u';
			else if (cLineChar == 'ü')
				cLineChar = 'u';
			else if (cLineChar == 'ç')
				cLineChar = 'c';
		}

		// Traitement du caractere en fonction de l'etat courant
		if (nWordState == StateEmpty)
		{
			if (isalnum(cLineChar))
			{
				sWordBuffer[nWordChar] = cLineChar;
				nWordChar++;
				nWordState = StateAlnumWord;
			}
			else if (ispunct(cLineChar))
			{
				sWordBuffer[nWordChar] = cLineChar;
				nWordChar++;
				nWordState = StatePunctWord;
			}
			else
			{
				assert(nWordChar == 0);
			}
		}
		else if (nWordState == StateAlnumWord)
		{
			if (isalnum(cLineChar))
			{
				// On continue le mot alnum courant
				sWordBuffer[nWordChar] = cLineChar;
				nWordChar++;
			}
			else if (ispunct(cLineChar))
			{
				// Fin du mot alnum courant
				sWordBuffer[nWordChar] = '\0';
				if (bInspectWords)
					cout << sWordBuffer << endl;
				UpdateWordDictionary(odWords, sWordBuffer);

				// Debut d'un nouveau mot punct
				nWordChar = 0;
				sWordBuffer[nWordChar] = cLineChar;
				nWordChar++;
				nWordState = StatePunctWord;
			}
			else
			{
				// Fin du mot courant
				sWordBuffer[nWordChar] = '\0';
				if (bInspectWords)
					cout << sWordBuffer << endl;
				UpdateWordDictionary(odWords, sWordBuffer);

				// Passage a l'etat vide
				nWordChar = 0;
				nWordState = StateEmpty;
			}
		}
		else if (nWordState == StatePunctWord)
		{
			if (isalnum(cLineChar))
			{
				// Fin du mot punct courant
				sWordBuffer[nWordChar] = '\0';
				if (bInspectWords)
					cout << sWordBuffer << endl;
				UpdateWordDictionary(odWords, sWordBuffer);

				// Debut d'un nouveau mot alnum
				nWordChar = 0;
				sWordBuffer[nWordChar] = cLineChar;
				nWordChar++;
				nWordState = StateAlnumWord;
			}
			else if (ispunct(cLineChar))
			{
				// On continue le mot punct courant
				sWordBuffer[nWordChar] = cLineChar;
				nWordChar++;
			}
			else
			{
				// Fin du mot courant
				sWordBuffer[nWordChar] = '\0';
				if (bInspectWords)
					cout << sWordBuffer << endl;
				UpdateWordDictionary(odWords, sWordBuffer);

				// Passage a l'etat vide
				nWordChar = 0;
				nWordState = StateEmpty;
			}
		}

		// Caractere suivant
		i++;
	}

	// Traitement d'un eventuel dernier mot
	if (nWordState != StateEmpty and nWordChar > 0)
	{
		// Fin du mot courant
		sWordBuffer[nWordChar] = '\0';
		if (bInspectWords)
			cout << sWordBuffer << endl;
		UpdateWordDictionary(odWords, sWordBuffer);
	}
}

void KWTextParser::BuildReferenceWordDictionary()
{
	FILE* fReference;

	// Nettoyage
	odReferenceWords.DeleteAll();

	// Ouverture du fichier d'entree
	fReference = p_fopen(sReferenceFileName, "rb");
	sCurrentFileName = sReferenceFileName;
	nLineOffset = 0;
	nTextOffset = 0;

	// Analyse de tous les textes du fichier
	while (not feof(fReference))
		BuildTextWordDictionary(fReference, &odReferenceWords);

	// Fermeture
	fclose(fReference);
}

void KWTextParser::FilterWordDictionary(ObjectDictionary* odWords, boolean bReference)
{
	boolean bSpecialWords = false;
	int nMinWordLength = 2;
	ObjectArray oaWords;
	int nWord;
	SampleObject* word;
	SampleObject* searchedWord;
	ALString sNumericalRoot = "num";
	ALString sMixedRoot = "mix";
	ALString sPunctuationRoot = "pnc";
	boolean bKeepWord;

	require(odWords != NULL);

	// Transfert du dictionnaire de mots dans un tableau
	odWords->ExportObjectArray(&oaWords);
	odWords->RemoveAll();

	// On range a nouveau les mots qui repondent au critere
	for (nWord = 0; nWord < oaWords.GetSize(); nWord++)
	{
		word = cast(SampleObject*, oaWords.GetAt(nWord));

		// Test si le mot est a garder
		bKeepWord = true;
		if (word->GetInt() <= nMinWordFrequency and bReference)
			bKeepWord = false;
		if (word->GetString().GetLength() < nMinWordLength)
			bKeepWord = false;
		if (IsWordPartlyNumerical(word->GetString()))
			bKeepWord = false;
		if (IsWordPunctuation(word->GetString()))
			bKeepWord = false;

		// Mots speciaux pour synthetiser certains cas
		if (bSpecialWords)
		{
			if (IsWordNumerical(word->GetString()))
			{
				word->SetString(sNumericalRoot + IntToString(word->GetString().GetLength()));
				bKeepWord = true;
			}
			else if (IsWordPartlyNumerical(word->GetString()))
			{
				word->SetString(sMixedRoot + IntToString(word->GetString().GetLength()));
				bKeepWord = true;
			}
			else if (IsWordPunctuation(word->GetString()))
			{
				word->SetString(sPunctuationRoot + IntToString(word->GetString().GetLength()));
				bKeepWord = true;
			}
		}

		// On garde le mot si necessaire
		WordToLower(word);
		WordTruncate(word, nMaxWordLength);
		if (bKeepWord)
		{
			searchedWord = cast(SampleObject*, odWords->Lookup(word->GetString()));
			if (searchedWord == NULL)
				odWords->SetAt(word->GetString(), word);
			else
			{
				searchedWord->SetInt(searchedWord->GetInt() + word->GetInt());
				delete word;
			}
		}
		else
			delete word;
	}
}

boolean KWTextParser::IsWordNumerical(const ALString& sWord)
{
	int nChar;

	for (nChar = 0; nChar < sWord.GetLength(); nChar++)
	{
		if (not isdigit(sWord.GetAt(nChar)))
			return false;
	}
	return true;
}

boolean KWTextParser::IsWordPartlyNumerical(const ALString& sWord)
{
	int nChar;

	for (nChar = 0; nChar < sWord.GetLength(); nChar++)
	{
		if (isdigit(sWord.GetAt(nChar)))
			return true;
	}
	return false;
}

boolean KWTextParser::IsWordPunctuation(const ALString& sWord)
{
	int nChar;

	for (nChar = 0; nChar < sWord.GetLength(); nChar++)
	{
		if (not ispunct(sWord.GetAt(nChar)))
			return false;
	}
	return true;
}

void KWTextParser::WordToLower(SampleObject* word)
{
	int nChar;
	ALString sWord;

	sWord = word->GetString();
	for (nChar = 0; nChar < sWord.GetLength(); nChar++)
	{
		sWord.SetAt(nChar, (char)tolower(sWord.GetAt(nChar)));
	}
	word->SetString(sWord);
}

void KWTextParser::WordTruncate(SampleObject* word, int nMaxLength)
{
	ALString sWord;

	sWord = word->GetString();
	if (sWord.GetLength() <= nMaxLength)
		word->SetString(sWord);
	else
		word->SetString(sWord.Left(nMaxLength));
}

void KWTextParser::WriteWords(ObjectDictionary* odWords, ostream& ost) const
{
	ObjectArray oaWords;
	int nWord;
	SampleObject* word;

	// Tri du dictionnaire de mots dans un tableau
	odWords->ExportObjectArray(&oaWords);
	oaWords.SetCompareFunction(SampleObjectCompareFrequency);
	oaWords.Sort();

	// Affichage de tous les mots
	for (nWord = 0; nWord < oaWords.GetSize(); nWord++)
	{
		word = cast(SampleObject*, oaWords.GetAt(nWord));

		// Affichage
		if (nWord == 0)
			ost << "Word\tFrequency\n";
		ost << word->GetString() << "\t" << word->GetInt() << "\n";
	}
}

void KWTextParser::UpdateWordDictionary(ObjectDictionary* odWords, const ALString& sWord)
{
	SampleObject* searchedWord;

	// Recherche du mot dans le dictionnaire
	searchedWord = cast(SampleObject*, odWords->Lookup(sWord));

	// Creation si necessaire dans le dictionnaire
	if (searchedWord == NULL)
	{
		searchedWord = new SampleObject;
		searchedWord->SetString(sWord);
		odWords->SetAt(sWord, searchedWord);
	}

	// Incrementation de l'effectif
	searchedWord->SetInt(searchedWord->GetInt() + 1);
}

void KWTextParser::ReadLine(char* sBuffer, int nBufferMaxSize, FILE* fInput)
{
	int nChar;
	int nFileChar;

	require(sBuffer != NULL);
	require(nBufferMaxSize >= 0);

	// Lecture caractere a caractere
	nChar = 0;
	while (not feof(fInput))
	{
		nFileChar = fgetc(fInput);
		if (nChar >= nBufferMaxSize - 1 or nFileChar == '\n' or feof(fInput))
			break;
		sBuffer[nChar] = (char)nFileChar;
		nChar++;
	}
	sBuffer[nChar] = '\0';
}

int SampleObjectCompareWord(const void* elem1, const void* elem2)
{
	int nCompare;

	nCompare = cast(SampleObject*, *(Object**)elem1)
		       ->GetString()
		       .Compare(cast(SampleObject*, *(Object**)elem2)->GetString());
	if (nCompare == 0)
		nCompare =
		    cast(SampleObject*, *(Object**)elem1)->GetInt() - cast(SampleObject*, *(Object**)elem2)->GetInt();
	return nCompare;
}

int SampleObjectCompareFrequency(const void* elem1, const void* elem2)
{
	int nCompare;

	nCompare = cast(SampleObject*, *(Object**)elem1)->GetInt() - cast(SampleObject*, *(Object**)elem2)->GetInt();
	if (nCompare == 0)
		nCompare = cast(SampleObject*, *(Object**)elem1)
			       ->GetString()
			       .Compare(cast(SampleObject*, *(Object**)elem2)->GetString());
	return nCompare;
}
