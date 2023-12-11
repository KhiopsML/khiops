// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRString.h"

void KWDRRegisterStringRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRLength);
	KWDerivationRule::RegisterDerivationRule(new KWDRLeft);
	KWDerivationRule::RegisterDerivationRule(new KWDRRight);
	KWDerivationRule::RegisterDerivationRule(new KWDRMiddle);
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenLength);
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenLeft);
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenRight);
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenMiddle);
	KWDerivationRule::RegisterDerivationRule(new KWDRTranslate);
	KWDerivationRule::RegisterDerivationRule(new KWDRSearch);
	KWDerivationRule::RegisterDerivationRule(new KWDRReplace);
	KWDerivationRule::RegisterDerivationRule(new KWDRReplaceAll);
	KWDerivationRule::RegisterDerivationRule(new KWDRRegexMatch);
	KWDerivationRule::RegisterDerivationRule(new KWDRRegexSearch);
	KWDerivationRule::RegisterDerivationRule(new KWDRRegexReplace);
	KWDerivationRule::RegisterDerivationRule(new KWDRRegexReplaceAll);
	KWDerivationRule::RegisterDerivationRule(new KWDRToUpper);
	KWDerivationRule::RegisterDerivationRule(new KWDRToLower);
	KWDerivationRule::RegisterDerivationRule(new KWDRConcat);
	KWDerivationRule::RegisterDerivationRule(new KWDRHash);
	KWDerivationRule::RegisterDerivationRule(new KWDREncrypt);
}

//////////////////////////////////////////////////////////////////////////////////////

Symbol KWDRStringRule::ComputeTextResult(const KWObject* kwoObject) const
{
	return ComputeSymbolResult(kwoObject);
}

void KWDRStringRule::TransformSymbolToTextRule()
{
	const ALString sTextLabel = "Text";
	ALString sLowerSymbolLabel;
	ALString sLowerTextLabel;
	int nSymbolLabelPos;

	require(GetOperandNumber() >= 1);
	require(GetFirstOperand()->GetType() == KWType::Symbol);
	require(GetName().Find(sTextLabel) == -1);

	// Changement du nom de la methode
	SetName(sTextLabel + GetName());

	// Changement du libelle de la methode
	sLowerSymbolLabel = KWType::ToString(KWType::Symbol);
	sLowerSymbolLabel.MakeLower();
	nSymbolLabelPos = GetLabel().Find(sLowerSymbolLabel);
	if (nSymbolLabelPos != -1)
	{
		sLowerTextLabel = KWType::ToString(KWType::Text);
		sLowerTextLabel.MakeLower();
		SetLabel(GetLabel().Left(nSymbolLabelPos) + sLowerTextLabel +
			 GetLabel().Right(GetLabel().GetLength() - nSymbolLabelPos - sLowerSymbolLabel.GetLength()));
	}

	// Changement en Text du type du premier operande
	GetFirstOperand()->SetType(KWType::Text);

	// Changement en Text du type retour si necessaire
	if (GetType() == KWType::Symbol)
		SetType(KWType::Text);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRLength::KWDRLength()
{
	SetName("Length");
	SetLabel("Length in chars of a categorical value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRLength::~KWDRLength() {}

KWDerivationRule* KWDRLength::Create() const
{
	return new KWDRLength;
}

Continuous KWDRLength::ComputeContinuousResult(const KWObject* kwoObject) const
{
	require(IsCompiled());
	return (Continuous)GetFirstOperandGenericSymbolValue(kwoObject).GetLength();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRLeft::KWDRLeft()
{
	SetName("Left");
	SetLabel("Extraction of the left characters of a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRLeft::~KWDRLeft() {}

KWDerivationRule* KWDRLeft::Create() const
{
	return new KWDRLeft;
}

Symbol KWDRLeft::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nCharNumber;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nCharNumber = (int)floor(GetSecondOperand()->GetContinuousValue(kwoObject) + 0.5);

	// Extraction de la sous-chaine
	if (nCharNumber <= 0)
		return "";
	else if (nCharNumber >= sValue.GetLength())
		return sValue;
	else
	{
		const KWSymbolAsString sTmpValue(sValue);
		return StringToSymbol(sTmpValue.Left(nCharNumber));
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRRight::KWDRRight()
{
	SetName("Right");
	SetLabel("Extraction of the right characters of a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRRight::~KWDRRight() {}

KWDerivationRule* KWDRRight::Create() const
{
	return new KWDRRight;
}

Symbol KWDRRight::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nCharNumber;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nCharNumber = (int)floor(GetSecondOperand()->GetContinuousValue(kwoObject) + 0.5);

	// Extraction de la sous-chaine
	if (nCharNumber <= 0)
		return "";
	else if (nCharNumber >= sValue.GetLength())
		return sValue;
	else
	{
		const KWSymbolAsString sTmpValue(sValue);
		return StringToSymbol(sTmpValue.Right(nCharNumber));
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRMiddle::KWDRMiddle()
{
	SetName("Middle");
	SetLabel("Extraction of the middle characters of a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRMiddle::~KWDRMiddle() {}

KWDerivationRule* KWDRMiddle::Create() const
{
	return new KWDRMiddle;
}

Symbol KWDRMiddle::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nBegin;
	int nCharNumber;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nBegin = (int)floor(GetOperandAt(1)->GetContinuousValue(kwoObject) + 0.5);
	nCharNumber = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) + 0.5);

	// Extraction de la sous-chaine
	if (nBegin < 1 or nBegin > sValue.GetLength())
		return "";
	else if (nCharNumber <= 0)
		return "";
	else if (nBegin + nCharNumber > sValue.GetLength())
	{
		const KWSymbolAsString sTmpValue(sValue);
		return StringToSymbol(sTmpValue.Right(sTmpValue.GetLength() - nBegin + 1));
	}
	else
	{
		const KWSymbolAsString sTmpValue(sValue);
		return Symbol(sTmpValue.Mid(nBegin - 1, nCharNumber));
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTokenLength::KWDRTokenLength()
{
	SetName("TokenLength");
	SetLabel("Length in tokens of a categorical value");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRTokenLength::~KWDRTokenLength() {}

KWDerivationRule* KWDRTokenLength::Create() const
{
	return new KWDRTokenLength;
}

Continuous KWDRTokenLength::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	Symbol sDelimiters;
	int nTokenLength;
	int nLength;
	int i;
	boolean bInToken;
	boolean bNewCharInToken;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nLength = sValue.GetLength();
	sDelimiters = GetSecondOperand()->GetSymbolValue(kwoObject);

	// Si chaine vide: pas de token
	if (nLength == 0)
		return 0;
	// Si pas de delimiteurs: un token (chaine non vide)
	else if (sDelimiters.GetLength() == 0)
		return 1;
	else
	{
		const KWSymbolAsString sTmpDelimiters(sDelimiters);

		// Recherche des separateurs en tenant compte des caracteres speciaux
		nTokenLength = 0;
		bInToken = false;
		for (i = 0; i < nLength; i++)
		{
			// Si caractere delimiteur, on est dans un delimiteur, sinon dans un token
			bNewCharInToken = (sTmpDelimiters.Find(sValue.GetAt(i)) == -1);

			// Un token supplementaire si on rentre dans un token
			if (not bInToken and bNewCharInToken)
				nTokenLength++;
			bInToken = bNewCharInToken;
		}

		// Nettoyage
		return (Continuous)nTokenLength;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTokenLeft::KWDRTokenLeft()
{
	SetName("TokenLeft");
	SetLabel("Extraction of the left tokens in a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRTokenLeft::~KWDRTokenLeft() {}

KWDerivationRule* KWDRTokenLeft::Create() const
{
	return new KWDRTokenLeft;
}

Symbol KWDRTokenLeft::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	Symbol sDelimiters;
	int nTokenIndex;
	int nTokenNumber;
	int nLength;
	int i;
	boolean bInToken;
	boolean bNewCharInToken;
	int nFirstChar;
	int nLastChar;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nLength = sValue.GetLength();
	sDelimiters = GetOperandAt(1)->GetSymbolValue(kwoObject);
	nTokenNumber = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) + 0.5);

	// Si chaine vide: pas de token
	if (nLength == 0)
		return "";
	// Si nombre de token incorecte: pas de token
	else if (nTokenNumber <= 0)
		return "";
	// Si pas de delimiteurs: un token (chaine non vide)
	else if (sDelimiters.GetLength() == 0)
		return sValue;
	else
	{
		KWSymbolAsString sTmpValue(sValue);
		KWSymbolAsString sTmpDelimiters(sDelimiters);

		// Recherche des separateurs en tenant compte des caracteres speciaux
		nTokenIndex = 0;
		bInToken = false;
		nFirstChar = -1;
		nLastChar = -1;
		for (i = 0; i < nLength; i++)
		{
			// Si caractere delimiteur, on est dans un delimiteur, sinon dans un token
			bNewCharInToken = (sTmpDelimiters.Find(sValue.GetAt(i)) == -1);

			// Un token supplementaire si on rentre dans un token
			if (not bInToken and bNewCharInToken)
			{
				nTokenIndex++;

				// On memorise la position du premier caractere du premier token
				if (nTokenIndex == 1)
					nFirstChar = i;
			}

			// Detection de la fin d'un token pour memoriser la position du
			// dernier caractere du token si necessaire
			if (bInToken and not bNewCharInToken)
			{
				if (nTokenIndex <= nTokenNumber)
					nLastChar = i - 1;
			}
			// Idem en fin de chaine (effet de bord)
			else if (bNewCharInToken and i == nLength - 1)
			{
				if (nTokenIndex <= nTokenNumber)
					nLastChar = i;
			}

			// Arret si nombre de token trouve (ou fin de chaine): on retourne la
			// sous-chaine contenant les tokens
			if (nFirstChar >= 0 and (nLastChar == i or i == nLength - 1) and
			    (nTokenIndex == nTokenNumber or i == nLength - 1))
				return StringToSymbol(sTmpValue.Mid(nFirstChar, nLastChar + 1 - nFirstChar));

			// Memorisation de l'etat suivant
			bInToken = bNewCharInToken;
		}

		// On n'a trouve aucun token
		assert(nFirstChar == -1 and nLastChar == -1);
		return "";
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTokenRight::KWDRTokenRight()
{
	SetName("TokenRight");
	SetLabel("Extraction of the right tokens in a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRTokenRight::~KWDRTokenRight() {}

KWDerivationRule* KWDRTokenRight::Create() const
{
	return new KWDRTokenRight;
}

Symbol KWDRTokenRight::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	Symbol sDelimiters;
	int nTokenIndex;
	int nTokenNumber;
	int nLength;
	int i;
	boolean bInToken;
	boolean bNewCharInToken;
	int nFirstChar;
	int nLastChar;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nLength = sValue.GetLength();
	sDelimiters = GetOperandAt(1)->GetSymbolValue(kwoObject);
	nTokenNumber = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) + 0.5);

	// Si chaine vide: pas de token
	if (nLength == 0)
		return "";
	// Si nombre de token incorecte: pas de token
	else if (nTokenNumber <= 0)
		return "";
	// Si pas de delimiteurs: un token (chaine non vide)
	else if (sDelimiters.GetLength() == 0)
		return sValue;
	else
	{
		KWSymbolAsString sTmpValue(sValue);
		KWSymbolAsString sTmpDelimiters(sDelimiters);

		// Recherche des separateurs en tenant compte des caracteres speciaux
		// On parcours la chaine a l'envers pour extraire les tokens a droite
		nTokenIndex = 0;
		bInToken = false;
		nFirstChar = -1;
		nLastChar = -1;
		for (i = nLength - 1; i >= 0; i--)
		{
			// Si caractere delimiteur, on est dans un delimiteur, sinon dans un token
			bNewCharInToken = (sTmpDelimiters.Find(sValue.GetAt(i)) == -1);

			// Un token supplementaire si on rentre dans un token
			if (not bInToken and bNewCharInToken)
			{
				nTokenIndex++;

				// On memorise la position du premier caractere du premier token
				if (nTokenIndex == 1)
					nLastChar = i;
			}

			// Detection de la fin d'un token pour memoriser la position du
			// dernier caractere du token si necessaire
			if (bInToken and not bNewCharInToken)
			{
				if (nTokenIndex <= nTokenNumber)
					nFirstChar = i + 1;
			}
			// Idem en debut de chaine (effet de bord)
			else if (bNewCharInToken and i == 0)
			{
				if (nTokenIndex <= nTokenNumber)
					nFirstChar = i;
			}

			// Arret si nombre de token trouve (ou debut de chaine): on retourne
			// la sous-chaine contenant les tokens
			if (nLastChar >= 0 and (nFirstChar == i + 1 or i == 0) and
			    (nTokenIndex == nTokenNumber or i == 0))
				return StringToSymbol(sTmpValue.Mid(nFirstChar, nLastChar + 1 - nFirstChar));

			// Memorisation de l'etat suivant
			bInToken = bNewCharInToken;
		}

		// On n'a trouve aucun token
		assert(nFirstChar == -1 and nLastChar == -1);
		return "";
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTokenMiddle::KWDRTokenMiddle()
{
	SetName("TokenMiddle");
	SetLabel("Extraction of the middle tokens in a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(4);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
	GetOperandAt(3)->SetType(KWType::Continuous);
}

KWDRTokenMiddle::~KWDRTokenMiddle() {}

KWDerivationRule* KWDRTokenMiddle::Create() const
{
	return new KWDRTokenMiddle;
}

Symbol KWDRTokenMiddle::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	Symbol sDelimiters;
	int nTokenIndex;
	int nTokenBegin;
	int nTokenNumber;
	int nLength;
	int i;
	boolean bInToken;
	boolean bNewCharInToken;
	int nFirstChar;
	int nLastChar;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nLength = sValue.GetLength();
	sDelimiters = GetOperandAt(1)->GetSymbolValue(kwoObject);
	nTokenBegin = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) + 0.5);
	nTokenNumber = (int)floor(GetOperandAt(3)->GetContinuousValue(kwoObject) + 0.5);

	// Si chaine vide: pas de token
	if (nLength == 0)
		return "";
	// Si position de depart incorrecte: pas de token
	else if (nTokenBegin < 1)
		return "";
	// Si nombre de token incorecte: pas de token
	else if (nTokenNumber <= 0)
		return "";
	// Si pas de delimiteurs: un token (chaine non vide)
	else if (sDelimiters.GetLength() == 0)
		return sValue;
	else
	{
		KWSymbolAsString sTmpValue(sValue);
		KWSymbolAsString sTmpDelimiters(sDelimiters);

		// Recherche des separateurs en tenant compte des caracteres speciaux
		nTokenIndex = 0;
		bInToken = false;
		nFirstChar = -1;
		nLastChar = -1;
		for (i = 0; i < nLength; i++)
		{
			// Si caractere delimiteur, on est dans un delimiteur, sinon dans un token
			bNewCharInToken = (sTmpDelimiters.Find(sValue.GetAt(i)) == -1);

			// Un token supplementaire si on rentre dans un token
			if (not bInToken and bNewCharInToken)
			{
				nTokenIndex++;

				// On memorise la position du premier caractere du premier token
				// a partir de la position demandee
				if (nTokenIndex == nTokenBegin)
					nFirstChar = i;
			}

			// Detection de la fin d'un token pour memoriser la position du
			// dernier caractere du token si necessaire
			if (bInToken and not bNewCharInToken)
			{
				if (nTokenBegin <= nTokenIndex and nTokenIndex + 1 - nTokenBegin <= nTokenNumber)
					nLastChar = i - 1;
			}
			// Idem en fin de chaine (effet de bord)
			else if (bNewCharInToken and i == nLength - 1)
			{
				if (nTokenBegin <= nTokenIndex and nTokenIndex + 1 - nTokenBegin <= nTokenNumber)
					nLastChar = i;
			}

			// Arret si nombre de token trouve (ou fin de chaine): on retourne
			// la sous-chaine contenant les tokens
			if (nFirstChar >= 0 and (nLastChar == i or i == nLength - 1) and
			    (nTokenIndex + 1 - nTokenBegin == nTokenNumber or i == nLength - 1))
				return StringToSymbol(sTmpValue.Mid(nFirstChar, nLastChar + 1 - nFirstChar));

			// Memorisation de l'etat suivant
			bInToken = bNewCharInToken;
		}

		// On n'a trouve aucun token
		assert(nFirstChar == -1 and nLastChar == -1);
		return "";
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRTranslate::KWDRTranslate()
{
	SetName("Translate");
	SetLabel("Replace substrings in sequence, with search/replace values in two input lists");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Structure);
	GetOperandAt(1)->SetStructureName("VectorC");
	GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginRule);
	GetOperandAt(1)->SetDerivationRule(new KWDRSymbolVector);
	GetOperandAt(2)->SetType(KWType::Structure);
	GetOperandAt(2)->SetStructureName("VectorC");
	GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginRule);
	GetOperandAt(2)->SetDerivationRule(new KWDRSymbolVector);
}

KWDRTranslate::~KWDRTranslate() {}

KWDerivationRule* KWDRTranslate::Create() const
{
	return new KWDRTranslate;
}

Symbol KWDRTranslate::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	ALString sStringValue;
	int i;
	KWDRSymbolVector* searchStrings;
	KWDRSymbolVector* replaceStrings;
	Symbol sSearchString;
	Symbol sReplaceString;
	int nReplacePos;
	ALString sBeginString;
	ALString sEndString;
	ALString sOutputString;

	require(IsCompiled());

	// On ne fait rien si la chaine est vide
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	if (sValue.IsEmpty())
		return sValue;
	// Remplacement en sequence des valeurs sinon
	else
	{
		// Recherche des parametres
		sValue = GetFirstOperandGenericSymbolValue(kwoObject);
		searchStrings = cast(KWDRSymbolVector*, GetOperandAt(1)->GetDerivationRule());
		replaceStrings = cast(KWDRSymbolVector*, GetOperandAt(2)->GetDerivationRule());

		// Parcours des valeurs a remplacer
		sStringValue = sValue;
		for (i = 0; i < searchStrings->GetValueNumber(); i++)
		{
			sSearchString = searchStrings->GetValueAt(i);
			sReplaceString = replaceStrings->GetValueAt(i);

			// On ignore les chaines a rechercher vides, qui provoqueraient une boucle infinie
			if (sSearchString.IsEmpty())
				continue;

			// Remplacement iteratif des pattern trouves a partir de la chaine pretraitee precedente
			nReplacePos = 0;
			sEndString = sStringValue;
			sOutputString = "";
			while (nReplacePos >= 0 and sEndString.GetLength() > 0)
			{
				nReplacePos = sEndString.Find(sSearchString);

				// Si non trouve, on garde la fin de la chaine en cours de traitement
				if (nReplacePos == -1)
					sOutputString += sEndString;
				// Sinon, en prend le debut puis la valeur de remplacement
				else
				{
					sBeginString = sEndString.Left(nReplacePos);
					sEndString = sEndString.Right(sEndString.GetLength() -
								      sSearchString.GetLength() - nReplacePos);
					sOutputString += sBeginString;
					sOutputString += sReplaceString;
				}
			}

			// On memorise le resultat, et on prepare les remplacements suivants
			sStringValue = sOutputString;
		}
		return StringToSymbol(sStringValue);
	}
}
boolean KWDRTranslate::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	ALString sTmp;
	int nSearchValueNumber;
	int nReplaceValueNumber;

	// Methode ancetre
	bOk = KWDerivationRule::CheckCompleteness(kwcOwnerClass);

	// Verification de la longueur des listes
	if (bOk)
	{
		nSearchValueNumber = cast(KWDRSymbolVector*, GetOperandAt(1)->GetDerivationRule())->GetValueNumber();
		nReplaceValueNumber = cast(KWDRSymbolVector*, GetOperandAt(2)->GetDerivationRule())->GetValueNumber();
		if (nSearchValueNumber != nReplaceValueNumber)
		{
			bOk = false;
			AddError(sTmp + " number of search values (" + IntToString(nSearchValueNumber) +
				 ") should be equal to number of replace values (" + IntToString(nReplaceValueNumber) +
				 ")");
		}
	}
	return bOk;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSearch::KWDRSearch()
{
	SetName("Search");
	SetLabel("Search the position of a substring in a categorical value");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Symbol);
}

KWDRSearch::~KWDRSearch() {}

KWDerivationRule* KWDRSearch::Create() const
{
	return new KWDRSearch;
}

Continuous KWDRSearch::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nBegin;
	Symbol sSearchString;
	int nFoundPos;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nBegin = (int)floor(GetOperandAt(1)->GetContinuousValue(kwoObject) + 0.5);
	sSearchString = GetOperandAt(2)->GetSymbolValue(kwoObject);

	// Recherche de la sous-chaine
	if (nBegin < 1 or nBegin > sValue.GetLength())
		return -1;
	else if (nBegin == 1)
	{
		const KWSymbolAsString sTmpValue(sValue);

		nFoundPos = sTmpValue.Find(sSearchString);
		if (nFoundPos == -1)
			return -1;
		else
			return (Continuous)(1 + nFoundPos);
	}
	else
	{
		const KWSymbolAsString sTmpValue(sValue);

		nFoundPos = sTmpValue.Right(sTmpValue.GetLength() + 1 - nBegin).Find(sSearchString);
		if (nFoundPos == -1)
			return -1;
		else
			return (Continuous)(nBegin + nFoundPos);
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRReplace::KWDRReplace()
{
	SetName("Replace");
	SetLabel("Replace a substring in a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(4);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Symbol);
	GetOperandAt(3)->SetType(KWType::Symbol);
}

KWDRReplace::~KWDRReplace() {}

KWDerivationRule* KWDRReplace::Create() const
{
	return new KWDRReplace;
}

Symbol KWDRReplace::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nBegin;
	Symbol sSearchString;
	Symbol sReplaceString;
	int nReplacePos;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nBegin = (int)floor(GetOperandAt(1)->GetContinuousValue(kwoObject) + 0.5);
	sSearchString = GetOperandAt(2)->GetSymbolValue(kwoObject);
	sReplaceString = GetOperandAt(3)->GetSymbolValue(kwoObject);

	// Recherche de la sous-chaine
	if (nBegin < 1 or nBegin > sValue.GetLength() or sSearchString.IsEmpty())
		return sValue;
	else if (nBegin == 1)
	{
		const KWSymbolAsString sTmpValue(sValue);

		// Recherche a partir du debut de la chaine
		nReplacePos = sTmpValue.Find(sSearchString);
		if (nReplacePos == -1)
			return sValue;
		else
			return StringToSymbol(
			    sTmpValue.Left(nReplacePos) + sReplaceString +
			    sTmpValue.Right(sTmpValue.GetLength() - nReplacePos - sSearchString.GetLength()));
	}
	else
	{
		const KWSymbolAsString sTmpValue(sValue);

		// Recherche a partir d'une position de depart
		nReplacePos = sTmpValue.Right(sTmpValue.GetLength() + 1 - nBegin).Find(sSearchString);
		if (nReplacePos == -1)
			return sValue;
		else
		{
			nReplacePos += nBegin - 1;
			return StringToSymbol(
			    sTmpValue.Left(nReplacePos) + sReplaceString +
			    sTmpValue.Right(sTmpValue.GetLength() - nReplacePos - sSearchString.GetLength()));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRReplaceAll::KWDRReplaceAll()
{
	SetName("ReplaceAll");
	SetLabel("Replace all substrings in a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(4);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Symbol);
	GetOperandAt(3)->SetType(KWType::Symbol);
}

KWDRReplaceAll::~KWDRReplaceAll() {}

KWDerivationRule* KWDRReplaceAll::Create() const
{
	return new KWDRReplaceAll;
}

Symbol KWDRReplaceAll::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nBegin;
	Symbol sSearchString;
	Symbol sReplaceString;
	int nReplacePos;
	ALString sBeginString;
	ALString sEndString;
	ALString sOutputString;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nBegin = (int)floor(GetOperandAt(1)->GetContinuousValue(kwoObject) + 0.5);
	sSearchString = GetOperandAt(2)->GetSymbolValue(kwoObject);
	sReplaceString = GetOperandAt(3)->GetSymbolValue(kwoObject);

	// Recherche de la sous-chaine
	if (nBegin < 1 or nBegin > sValue.GetLength() or sSearchString.IsEmpty())
		return sValue;
	else
	{
		const KWSymbolAsString sTmpValue(sValue);

		// Remplacement iteratif des pattern trouves a partir de la chaine pretraitee precedente
		nReplacePos = nBegin - 1;
		sBeginString = sTmpValue.Left(nReplacePos);
		sEndString = sTmpValue.Right(sTmpValue.GetLength() - nReplacePos);
		sOutputString = sBeginString;
		while (nReplacePos >= 0 and sEndString.GetLength() > 0)
		{
			nReplacePos = sEndString.Find(sSearchString);

			// Si non trouve, on garde la fin de la chaine en cours de traitement
			if (nReplacePos == -1)
				sOutputString += sEndString;
			// Sinon, en prend le debut puis la valeur de remplacement
			else
			{
				sBeginString = sEndString.Left(nReplacePos);
				sEndString =
				    sEndString.Right(sEndString.GetLength() - sSearchString.GetLength() - nReplacePos);
				sOutputString += sBeginString;
				sOutputString += sReplaceString;
			}
		}
		return StringToSymbol(sOutputString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRRegex::KWDRRegex() {}

KWDRRegex::~KWDRRegex() {}

boolean KWDRRegex::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	int nRegexOperandIndex;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckCompleteness(kwcOwnerClass);

	// Verification si necessaire de la regex
	if (bOk)
	{
		// Acces a l'index de l'operande de regex: 2, sauf pour pour la regle Match
		nRegexOperandIndex = 2;
		if (GetOperandNumber() <= nRegexOperandIndex)
			nRegexOperandIndex = GetOperandNumber() - 1;

		// On ne verifie pas l'expression si elle a deja ete validee
		if (not regEx.IsValid() or
		    regEx.GetRegex() != GetOperandAt(nRegexOperandIndex)->GetSymbolConstant().GetValue())
			bOk = regEx.Initialize(GetOperandAt(nRegexOperandIndex)->GetSymbolConstant().GetValue(), this);
	}
	return bOk;
}

void KWDRRegex::Compile(KWClass* kwcOwnerClass)
{
	int nRegexOperandIndex;

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Acces a l'index de l'operande de regex: 2, sauf pour pour la regle Match
	nRegexOperandIndex = 2;
	if (GetOperandNumber() <= nRegexOperandIndex)
		nRegexOperandIndex = GetOperandNumber() - 1;

	// On ne verifie pas l'expression si elle a deja ete validee
	if (not regEx.IsValid() or regEx.GetRegex() != GetOperandAt(nRegexOperandIndex)->GetSymbolConstant().GetValue())
		regEx.Initialize(GetOperandAt(nRegexOperandIndex)->GetSymbolConstant().GetValue(), this);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRRegexMatch::KWDRRegexMatch()
{
	SetName("RegexMatch");
	SetLabel("Return 1 if a categorical value completely matches a regex, 0 otherwise");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);  // Texte sur lequel s'appliquera l'expression reguliere
	GetSecondOperand()->SetType(KWType::Symbol); // Expression reguliere
	GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRRegexMatch::~KWDRRegexMatch() {}

KWDerivationRule* KWDRRegexMatch::Create() const
{
	return new KWDRRegexMatch;
}

Continuous KWDRRegexMatch::ComputeContinuousResult(const KWObject* kwoObject) const
{
	boolean bResult;
	Symbol sValue;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);

	// Test si on a un match
	bResult = regEx.Match(sValue);
	if (bResult)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRRegexSearch::KWDRRegexSearch()
{
	SetName("RegexSearch");
	SetLabel("Search the position of a regular expression in a categorical value");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Symbol);      // Texte sur lequel s'appliquera l'expression reguliere
	GetSecondOperand()->SetType(KWType::Continuous); // Position de depart pour la recherche
	GetOperandAt(2)->SetType(KWType::Symbol);        // Expression reguliere
	GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRRegexSearch::~KWDRRegexSearch() {}

KWDerivationRule* KWDRRegexSearch::Create() const
{
	return new KWDRRegexSearch;
}

Continuous KWDRRegexSearch::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nBegin;
	int nFoundPos;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nBegin = (int)floor(GetOperandAt(1)->GetContinuousValue(kwoObject) + 0.5);

	// Recherche a partir du debut dans la cas standard
	if (nBegin == 1)
	{
		nFoundPos = regEx.Find(sValue);
		if (nFoundPos == -1)
			return -1;
		else
			return (Continuous)(1 + nFoundPos);
	}
	// Recherche a partir de la position de depart
	else
	{
		// Cas ou la position de depart est invalide
		if (nBegin < 1 or nBegin > sValue.GetLength())
			return -1;
		// Cas avec position de depart valide
		else
		{
			nFoundPos = regEx.Find(sValue + nBegin - 1);
			if (nFoundPos == -1)
				return -1;
			else
				return (Continuous)(nBegin + nFoundPos);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRRegexReplace::KWDRRegexReplace()
{
	SetName("RegexReplace");
	SetLabel("Replace a regular expression in a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(4);
	GetFirstOperand()->SetType(KWType::Symbol);      // Texte sur lequel s'appliquera l'expression reguliere
	GetSecondOperand()->SetType(KWType::Continuous); // Position de depart pour la recherche
	GetOperandAt(2)->SetType(KWType::Symbol);        // Expression reguliere
	GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	GetOperandAt(3)->SetType(KWType::Symbol); // Chaine de remplacement pour la premiere occurence trouvee
}

KWDRRegexReplace::~KWDRRegexReplace() {}

KWDerivationRule* KWDRRegexReplace::Create() const
{
	return new KWDRRegexReplace;
}

Symbol KWDRRegexReplace::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nBegin;
	Symbol sReplaceString;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nBegin = (int)floor(GetOperandAt(1)->GetContinuousValue(kwoObject) + 0.5);
	sReplaceString = GetOperandAt(3)->GetSymbolValue(kwoObject);

	// Recherche a partir du debut dans la cas standard
	if (nBegin == 1)
	{
		return StringToSymbol(regEx.Replace(sValue, sReplaceString));
	}
	// Recherche a partir de la position de depart
	else
	{
		// Cas ou la position de depart est invalide
		if (nBegin < 1 or nBegin > sValue.GetLength())
			return sValue;
		// Cas avec position de depart valide
		else
		{
			const KWSymbolAsString sTmpValue(sValue);

			return StringToSymbol(sTmpValue.Left(nBegin - 1) +
					      regEx.Replace(sValue + nBegin - 1, sReplaceString));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRRegexReplaceAll::KWDRRegexReplaceAll()
{
	SetName("RegexReplaceAll");
	SetLabel("Replace all regular expressions in a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(4);
	GetFirstOperand()->SetType(KWType::Symbol);      // Texte sur lequel s'appliquera l'expression reguliere
	GetSecondOperand()->SetType(KWType::Continuous); // Position de depart pour la recherche
	GetOperandAt(2)->SetType(KWType::Symbol);        // Expression reguliere
	GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	GetOperandAt(3)->SetType(KWType::Symbol); // Chaine de remplacement pour la premiere occurence trouvee
}

KWDRRegexReplaceAll::~KWDRRegexReplaceAll() {}

KWDerivationRule* KWDRRegexReplaceAll::Create() const
{
	return new KWDRRegexReplaceAll;
}

Symbol KWDRRegexReplaceAll::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nBegin;
	Symbol sReplaceString;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nBegin = (int)floor(GetOperandAt(1)->GetContinuousValue(kwoObject) + 0.5);
	sReplaceString = GetOperandAt(3)->GetSymbolValue(kwoObject);

	// Recherche a partir du debut dans la cas standard
	if (nBegin == 1)
	{
		return StringToSymbol(regEx.ReplaceAll(sValue, sReplaceString));
	}
	// Recherche a partir de la position de depart
	else
	{
		// Cas ou la position de depart est invalide
		if (nBegin < 1 or nBegin > sValue.GetLength())
			return sValue;
		// Cas avec position de depart valide
		else
		{
			const KWSymbolAsString sTmpValue(sValue);

			return StringToSymbol(sTmpValue.Left(nBegin - 1) +
					      regEx.ReplaceAll(sValue + nBegin - 1, sReplaceString));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRToUpper::KWDRToUpper()
{
	SetName("ToUpper");
	SetLabel("Conversion to upper case of a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRToUpper::~KWDRToUpper() {}

KWDerivationRule* KWDRToUpper::Create() const
{
	return new KWDRToUpper;
}

Symbol KWDRToUpper::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ALString sResult;

	require(IsCompiled());

	sResult = GetFirstOperandGenericSymbolValue(kwoObject);
	sResult.MakeUpper();
	return StringToSymbol(sResult);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRToLower::KWDRToLower()
{
	SetName("ToLower");
	SetLabel("Conversion to lower case of a categorical value");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRToLower::~KWDRToLower() {}

KWDerivationRule* KWDRToLower::Create() const
{
	return new KWDRToLower;
}

Symbol KWDRToLower::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ALString sResult;

	require(IsCompiled());

	sResult = GetFirstOperandGenericSymbolValue(kwoObject);
	sResult.MakeLower();
	return StringToSymbol(sResult);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRConcat::KWDRConcat()
{
	SetName("Concat");
	SetLabel("Concatenation of categorical values");
	SetType(KWType::Symbol);
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Symbol);
}

KWDRConcat::~KWDRConcat() {}

KWDerivationRule* KWDRConcat::Create() const
{
	return new KWDRConcat;
}

Symbol KWDRConcat::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ALString sResult;
	int i;

	require(IsCompiled());

	// Calcul de la concatenation
	for (i = 0; i < GetOperandNumber(); i++)
		sResult += GetGenericSymbolValue(GetOperandAt(i), kwoObject);
	return StringToSymbol(sResult);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRHash::KWDRHash()
{
	SetName("Hash");
	SetLabel("Hash a categorical value");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Symbol);
	GetOperandAt(1)->SetType(KWType::Continuous);
}

KWDRHash::~KWDRHash() {}

KWDerivationRule* KWDRHash::Create() const
{
	return new KWDRHash;
}

Continuous KWDRHash::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	unsigned int nSize;
	unsigned int nHash;

	require(IsCompiled());

	// Recherche des parametres
	sValue = GetFirstOperandGenericSymbolValue(kwoObject);
	nSize = (int)floor(GetOperandAt(1)->GetContinuousValue(kwoObject) + 0.5);

	// Calcul de la cle de hashage
	nHash = HashValue(sValue) % nSize;
	return (Continuous)nHash;
}
