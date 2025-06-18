// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRText.h"

void KWDRRegisterTextRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRTextLoadFile);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextLength);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextLeft);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRight);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextMiddle);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTokenLength);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTokenLeft);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTokenRight);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTokenMiddle);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextTranslate);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextSearch);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextReplace);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextReplaceAll);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRegexMatch);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRegexSearch);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRegexReplace);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextRegexReplaceAll);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextToUpper);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextToLower);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextConcat);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextHash);
	KWDerivationRule::RegisterDerivationRule(new KWDRTextEncrypt);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTextLoadFile::KWDRTextLoadFile()
{
	SetName("TextLoadFile");
	SetLabel("Load the content of a file into a text value");
	SetType(KWType::Text);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRTextLoadFile::~KWDRTextLoadFile() {}

KWDerivationRule* KWDRTextLoadFile::Create() const
{
	return new KWDRTextLoadFile;
}

Symbol KWDRTextLoadFile::ComputeTextResult(const KWObject* kwoObject) const
{
	Symbol sLoadedText;
	ALString sPathFileName;
	InputBufferedFile inputFile;
	boolean bOk;
	longint lBeginPos;
	const CharVector* cvFileCache;
	char* sHugeBuffer;
	int nTextLength;
	int nDoubleQuoteNumber;
	int nStart;
	int nStop;
	char c;
	int i;
	ALString sTmp;

	require(IsCompiled());

	// Lecture du fichier si un path est specifie
	sPathFileName = GetFirstOperand()->GetSymbolValue(kwoObject);
	if (sPathFileName != "")
	{
		// On traite teporairement les error en warning, pour prevenir l'utilisateur d'un probleme
		// sans que cela ne soit bloquant
		Global::SetErrorAsWarningMode(true);

		// Parametrage du fichier, avec la taille maximum possible pour un champ de type Text
		inputFile.SetFileName(sPathFileName);
		inputFile.SetUTF8BomManagement(false);
		inputFile.SetBufferSize(InputBufferedFile::nMaxFieldSize);

		// Lecture d'un fichier
		bOk = inputFile.Open();
		if (bOk)
		{
			// Lecture
			lBeginPos = 0;
			bOk = inputFile.FillBytes(lBeginPos);

			// Remplissage d'un buffer de grande taille
			// Permet de stocker tout le contenu du fichier en un seul blob memoire alloue efficacement
			// afin de se preparer en le stocker sous forme de Symbol
			if (bOk)
			{
				// Recherche d'un gros blob memoire en le terminaux par une fin de chaine de caracteres
				nTextLength = inputFile.GetCurrentBufferSize();
				sHugeBuffer = GetHugeBuffer(nTextLength + 1);
				sHugeBuffer[nTextLength] = '\0';

				// Acces aux methodes de bas niveau du fichier pour transferer directement le contenu
				// du cache du fichier vers le buffer
				cvFileCache = inputFile.GetCache();
				assert(inputFile.GetBufferStartInCache() == 0);
				nDoubleQuoteNumber = 0;
				for (i = 0; i < nTextLength; i++)
				{
					c = cvFileCache->GetAt(i);

					// Comptage des double quotes de la chaine pour ne pas depasser la taille limite des Text
					if (sHugeBuffer[i] == '"')
						nDoubleQuoteNumber++;

					// On remplace par des blancs les caracteres posant des problemes a l'ecriture d'un text
					// sous la forme d'un champ d'une base de donnees
					if (c == '\0' or c == '\n' or c == '\r')
						c = ' ';
					sHugeBuffer[i] = c;
				}

				// On supprime les blancs en debut et fin de la chaine pour que le champ Text
				// soit exactement le meme que celui qui serait relu depuis un fichier tabulaire,
				// dont les champs sont "trimes" (cf. InputBufferedFile::GetNextField)

				// Supression des blancs en fin
				nStart = 0;
				while (nStart < nTextLength)
				{
					if (iswspace(sHugeBuffer[nStart]))
						nStart++;
					else
						break;
				}
				nTextLength -= nStart;

				// S'il y a risque de depassement de la taille limites des Text en doublant les doubles quotes
				// et en inserant la valeur entre 2 doubles-quote (cas ou la valeur contient le separateur
				// de champ des fichiers tabulaires), on diminue la taille du texte en proportion
				if (nTextLength + 2 + nDoubleQuoteNumber > InputBufferedFile::nMaxFieldSize)
					nTextLength = InputBufferedFile::nMaxFieldSize - (2 + nDoubleQuoteNumber);

				// Supression des blancs en fin
				nStop = nStart + nTextLength - 1;
				while (nStop >= nStart)
				{
					if (iswspace(sHugeBuffer[nStop]))
						nStop--;
					else
						break;
				}
				nStop++;
				sHugeBuffer[nStop] = '\0';
				nTextLength = nStop - nStart;

				// Transformation du buffer en Symbol
				sLoadedText = Symbol(&sHugeBuffer[nStart], nTextLength);
			}

			// Fermeture du fichier
			inputFile.Close();
		}

		// Si erreur, emission d'un warning permettant de localiser l'enreigistrement
		if (not bOk)
			AddWarning(sTmp + "Enable to load text file in record " +
				   LongintToReadableString(kwoObject->GetCreationIndex()));

		// Restittion du mode standard de traitement des erreurs
		Global::SetErrorAsWarningMode(false);
	}
	return sLoadedText;
}

KWDRTextLength::KWDRTextLength()
{
	TransformSymbolToTextRule();
}

KWDRTextLength::~KWDRTextLength() {}

KWDerivationRule* KWDRTextLength::Create() const
{
	return new KWDRTextLength;
}

KWDRTextLeft::KWDRTextLeft()
{
	TransformSymbolToTextRule();
}

KWDRTextLeft::~KWDRTextLeft() {}

KWDerivationRule* KWDRTextLeft::Create() const
{
	return new KWDRTextLeft;
}

KWDRTextRight::KWDRTextRight()
{
	TransformSymbolToTextRule();
}

KWDRTextRight::~KWDRTextRight() {}

KWDerivationRule* KWDRTextRight::Create() const
{
	return new KWDRTextRight;
}

KWDRTextMiddle::KWDRTextMiddle()
{
	TransformSymbolToTextRule();
}

KWDRTextMiddle::~KWDRTextMiddle() {}

KWDerivationRule* KWDRTextMiddle::Create() const
{
	return new KWDRTextMiddle;
}

KWDRTextTokenLength::KWDRTextTokenLength()
{
	TransformSymbolToTextRule();
}

KWDRTextTokenLength::~KWDRTextTokenLength() {}

KWDerivationRule* KWDRTextTokenLength::Create() const
{
	return new KWDRTextTokenLength;
}

KWDRTextTokenLeft::KWDRTextTokenLeft()
{
	TransformSymbolToTextRule();
}

KWDRTextTokenLeft::~KWDRTextTokenLeft() {}

KWDerivationRule* KWDRTextTokenLeft::Create() const
{
	return new KWDRTextTokenLeft;
}

KWDRTextTokenRight::KWDRTextTokenRight()
{
	TransformSymbolToTextRule();
}

KWDRTextTokenRight::~KWDRTextTokenRight() {}

KWDerivationRule* KWDRTextTokenRight::Create() const
{
	return new KWDRTextTokenRight;
}

KWDRTextTokenMiddle::KWDRTextTokenMiddle()
{
	TransformSymbolToTextRule();
}

KWDRTextTokenMiddle::~KWDRTextTokenMiddle() {}

KWDerivationRule* KWDRTextTokenMiddle::Create() const
{
	return new KWDRTextTokenMiddle;
}

KWDRTextTranslate::KWDRTextTranslate()
{
	TransformSymbolToTextRule();
}

KWDRTextTranslate::~KWDRTextTranslate() {}

KWDerivationRule* KWDRTextTranslate::Create() const
{
	return new KWDRTextTranslate;
}

KWDRTextSearch::KWDRTextSearch()
{
	TransformSymbolToTextRule();
}

KWDRTextSearch::~KWDRTextSearch() {}

KWDerivationRule* KWDRTextSearch::Create() const
{
	return new KWDRTextSearch;
}

KWDRTextReplace::KWDRTextReplace()
{
	TransformSymbolToTextRule();
}

KWDRTextReplace::~KWDRTextReplace() {}

KWDerivationRule* KWDRTextReplace::Create() const
{
	return new KWDRTextReplace;
}

KWDRTextReplaceAll::KWDRTextReplaceAll()
{
	TransformSymbolToTextRule();
}

KWDRTextReplaceAll::~KWDRTextReplaceAll() {}

KWDerivationRule* KWDRTextReplaceAll::Create() const
{
	return new KWDRTextReplaceAll;
}

KWDRTextRegexMatch::KWDRTextRegexMatch()
{
	TransformSymbolToTextRule();
}

KWDRTextRegexMatch::~KWDRTextRegexMatch() {}

KWDerivationRule* KWDRTextRegexMatch::Create() const
{
	return new KWDRTextRegexMatch;
}

KWDRTextRegexSearch::KWDRTextRegexSearch()
{
	TransformSymbolToTextRule();
}

KWDRTextRegexSearch::~KWDRTextRegexSearch() {}

KWDerivationRule* KWDRTextRegexSearch::Create() const
{
	return new KWDRTextRegexSearch;
}

KWDRTextRegexReplace::KWDRTextRegexReplace()
{
	TransformSymbolToTextRule();
}

KWDRTextRegexReplace::~KWDRTextRegexReplace() {}

KWDerivationRule* KWDRTextRegexReplace::Create() const
{
	return new KWDRTextRegexReplace;
}

KWDRTextRegexReplaceAll::KWDRTextRegexReplaceAll()
{
	TransformSymbolToTextRule();
}

KWDRTextRegexReplaceAll::~KWDRTextRegexReplaceAll() {}

KWDerivationRule* KWDRTextRegexReplaceAll::Create() const
{
	return new KWDRTextRegexReplaceAll;
}

KWDRTextToUpper::KWDRTextToUpper()
{
	TransformSymbolToTextRule();
}

KWDRTextToUpper::~KWDRTextToUpper() {}

KWDerivationRule* KWDRTextToUpper::Create() const
{
	return new KWDRTextToUpper;
}

KWDRTextToLower::KWDRTextToLower()
{
	TransformSymbolToTextRule();
}

KWDRTextToLower::~KWDRTextToLower() {}

KWDerivationRule* KWDRTextToLower::Create() const
{
	return new KWDRTextToLower;
}

KWDRTextHash::KWDRTextHash()
{
	TransformSymbolToTextRule();
}

KWDRTextHash::~KWDRTextHash() {}

KWDerivationRule* KWDRTextHash::Create() const
{
	return new KWDRTextHash;
}

KWDRTextConcat::KWDRTextConcat()
{
	TransformSymbolToTextRule();
}

KWDRTextConcat::~KWDRTextConcat() {}

KWDerivationRule* KWDRTextConcat::Create() const
{
	return new KWDRTextConcat;
}

KWDRTextEncrypt::KWDRTextEncrypt()
{
	TransformSymbolToTextRule();
}

KWDRTextEncrypt::~KWDRTextEncrypt() {}

KWDerivationRule* KWDRTextEncrypt::Create() const
{
	return new KWDRTextEncrypt;
}
