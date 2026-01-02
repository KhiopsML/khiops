// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTextTokenizer.h"

KWTextTokenizer::KWTextTokenizer()
{
	gdSpecificTokens = NULL;
	nMaxCollectedTokenNumber = 0;
	gdCollectedTokens = new LongintDictionary;
	bIsDeploymentMode = false;
}

KWTextTokenizer::~KWTextTokenizer()
{
	require(gdCollectedTokens != NULL);
	Reset();
	delete gdCollectedTokens;
}

void KWTextTokenizer::Reset()
{
	require(gdCollectedTokens != NULL);
	if (gdSpecificTokens != NULL)
		delete gdSpecificTokens;
	gdSpecificTokens = NULL;
	nMaxCollectedTokenNumber = 0;
	bIsDeploymentMode = false;
	CleanCollectedTokens();
}

void KWTextTokenizer::SetSpecificTokens(const ObjectArray* oaTokens)
{
	LongintDictionary* ldSpecificTokens;
	const KWTokenFrequency* token;
	int nToken;

	require(GetCollectedTokenNumber() == 0);
	require(gdCollectedTokens->IsStringKey());

	// Nettoyage prealable
	if (gdSpecificTokens != NULL)
		delete gdSpecificTokens;
	gdSpecificTokens = NULL;
	lvSpecificTokenFrequencies.SetSize(0);
	bIsDeploymentMode = false;

	// Prise en compte de la specification
	if (oaTokens != NULL)
	{
		// Choix du type de dictionnaire selon le type de cle, par defaut cle chaine de caracteres
		ldSpecificTokens = new LongintDictionary;
		gdSpecificTokens = ldSpecificTokens;

		// Memorisation des tokens dans le dictionnaire
		for (nToken = 0; nToken < oaTokens->GetSize(); nToken++)
		{
			token = cast(const KWTokenFrequency*, oaTokens->GetAt(nToken));

			// Memorisation de l'index du token en demarant a 1 pour indiquer sa presence
			assert(ldSpecificTokens->Lookup(token->GetToken()) == 0);
			ldSpecificTokens->SetAt(token->GetToken(), nToken + 1);
		}
		assert(ldSpecificTokens->GetCount() == oaTokens->GetSize());

		// Dimensionnement du vecteur d'effectifs
		lvSpecificTokenFrequencies.SetSize(oaTokens->GetSize() + 1);
	}
}

int KWTextTokenizer::GetSpecificTokenNumber() const
{
	if (gdSpecificTokens == NULL)
		return 0;
	else
		return gdSpecificTokens->GetCount();
}

boolean KWTextTokenizer::CheckParameters() const
{
	boolean bOk;

	bOk = GetSpecificTokenNumber() == 0 or GetMaxCollectedTokenNumber() == 0;
	bOk = bOk and
	      (GetSpecificTokenNumber() == 0 or lvSpecificTokenFrequencies.GetSize() == GetSpecificTokenNumber() + 1);
	bOk = bOk and ivUsedSpecificTokenIndexes.GetSize() <= GetSpecificTokenNumber();
	return bOk;
}

void KWTextTokenizer::TokenizeStringVector(const StringVector* svValues)
{
	int i;

	require(not IsDeploymentMode());
	require(CheckParameters());
	require(svValues != NULL);

	for (i = 0; i < svValues->GetSize(); i++)
		TokenizeString(svValues->GetAt(i));
}

void KWTextTokenizer::TokenizeSymbolVector(const SymbolVector* svValues)
{
	int i;

	require(not IsDeploymentMode());
	require(CheckParameters());
	require(svValues != NULL);

	for (i = 0; i < svValues->GetSize(); i++)
		TokenizeSymbol(svValues->GetAt(i));
}

void KWTextTokenizer::CumulateTokenFrequencies(const ObjectArray* oaTokens)
{
	int nToken;
	const KWTokenFrequency* token;

	require(not IsDeploymentMode());
	require(CheckParameters());
	require(oaTokens != NULL);

	// Prise en compte des effectifs des tokens
	for (nToken = 0; nToken < oaTokens->GetSize(); nToken++)
	{
		token = cast(const KWTokenFrequency*, oaTokens->GetAt(nToken));

		// Ajout de l'effectif du token
		UpgradeTokenFrequency(token->GetToken(), token->GetFrequency());
	}
}

void KWTextTokenizer::ExportTokens(ObjectArray* oaTokenFrequencies) const
{
	require(not IsDeploymentMode());
	require(oaTokenFrequencies != NULL);
	require(oaTokenFrequencies->GetSize() == 0);

	ExportFrequentTokens(oaTokenFrequencies, GetCollectedTokenNumber());
}

void KWTextTokenizer::ExportFrequentTokens(ObjectArray* oaTokenFrequencies, int nMaxTokenNumber) const
{
	const int nMaxCollectedFrequency = 1000;
	const LongintDictionary* ldCollectedTokens;
	const LongintDictionary* ldSpecificTokens;
	POSITION position;
	ALString sKey;
	longint lValue;
	IntVector ivNumbersOfTokensPerFrequency;
	int nMinFrequency;
	int nCollectedTokenNumber;
	int nSpecificTokenIndex;
	KWTokenFrequency* token;
	int nIndex;

	require(not IsDeploymentMode());
	require(gdCollectedTokens != NULL);
	require(gdCollectedTokens->IsStringKey());
	require(not gdCollectedTokens->IsObjectValue());
	require(nMaxTokenNumber >= 0);
	require(oaTokenFrequencies != NULL);
	require(oaTokenFrequencies->GetSize() == 0);

	// Choix du type de dictionnaire selon le type de cle
	ldCollectedTokens = cast(const LongintDictionary*, gdCollectedTokens);
	ldSpecificTokens = cast(const LongintDictionary*, gdSpecificTokens);

	// Cas sans tokens specifiques
	if (ldSpecificTokens == NULL)
	{
		// Initialisation du nombre de tokens a collecter sans contrainte d'effectif minimum
		nCollectedTokenNumber = gdCollectedTokens->GetCount();
		nMinFrequency = 0;

		// Si necessaire, passe prealabale de collecte des stats sur les nombres de tokens par effectif
		if (gdCollectedTokens->GetCount() > 2 * nMaxTokenNumber)
		{
			// Calcul des stats par effectif de token
			ComputeTokenDictionaryFrequencyStats(gdCollectedTokens, nMaxCollectedFrequency,
							     &ivNumbersOfTokensPerFrequency);

			// On determine l'effectif minimum suffisant pour collecter les tokens les plus frequents
			nMinFrequency = 1;
			while (nCollectedTokenNumber > nMaxTokenNumber and nMinFrequency < nMaxCollectedFrequency)
			{
				nCollectedTokenNumber -= ivNumbersOfTokensPerFrequency.GetAt(nMinFrequency);
				nMinFrequency++;
			}
			assert(nMinFrequency == nMaxCollectedFrequency or nCollectedTokenNumber <= nMaxTokenNumber);
			assert(nMinFrequency == nMaxCollectedFrequency or
			       nCollectedTokenNumber + ivNumbersOfTokensPerFrequency.GetAt(nMinFrequency - 1) >
				   nMaxTokenNumber);
		}

		// Parcours des cles pour creer l'ensemble des tokens, en ne gardant que ceux ayant un effectif minimum
		// suffisant
		assert(nMinFrequency > 0 or nCollectedTokenNumber == gdCollectedTokens->GetCount());
		oaTokenFrequencies->SetSize(nCollectedTokenNumber);
		nIndex = 0;
		position = ldCollectedTokens->GetStartPosition();
		while (position != NULL)
		{
			ldCollectedTokens->GetNextAssoc(position, sKey, lValue);

			// Creation et memorisation du token selon la contrainte d'effectif minimum
			if (lValue >= nMinFrequency)
			{
				token = new KWTokenFrequency;
				token->SetToken(sKey);
				token->SetFrequency(lValue);
				oaTokenFrequencies->SetAt(nIndex, token);
				nIndex++;
			}
		}
		assert(nIndex == nCollectedTokenNumber);
	}
	// Cas avec tokens specifiques
	else
	{
		assert(gdCollectedTokens->GetCount() == 0);

		// Initialisation du nombre de tokens a collecter sans contrainte d'effectif minimum
		nCollectedTokenNumber = ivUsedSpecificTokenIndexes.GetSize();

		// Parcours des tokens specifiques pour retrouver leur index et donc leur effectif
		oaTokenFrequencies->SetSize(nCollectedTokenNumber);
		nIndex = 0;
		position = ldSpecificTokens->GetStartPosition();
		while (position != NULL)
		{
			ldSpecificTokens->GetNextAssoc(position, sKey, lValue);
			nSpecificTokenIndex = (int)lValue;

			// Creation et memorisation du token selon la contrainte d'effectif minimum
			if (lvSpecificTokenFrequencies.GetAt(nSpecificTokenIndex) > 0)
			{
				token = new KWTokenFrequency;
				token->SetToken(sKey);
				token->SetFrequency(lvSpecificTokenFrequencies.GetAt(nSpecificTokenIndex));
				oaTokenFrequencies->SetAt(nIndex, token);
				nIndex++;
			}
		}
	}
	// Filtrage de la fin du tableau apres tri
	SortTokenArray(oaTokenFrequencies);
	FilterTokenArray(oaTokenFrequencies, nMaxTokenNumber);
}

void KWTextTokenizer::CleanCollectedTokens()
{
	require(not IsDeploymentMode());
	require(gdCollectedTokens != NULL);

	gdCollectedTokens->RemoveAll();
	ivUsedSpecificTokenIndexes.SetSize(0);
	ensure(GetCollectedTokenNumber() == 0);
}

void KWTextTokenizer::DisplayTokens(ostream& ost) const
{
	require(not IsDeploymentMode());

	DisplayFrequentTokens(GetCollectedTokenNumber(), ost);
}

void KWTextTokenizer::DisplayFrequentTokens(int nMaxTokenNumber, ostream& ost) const
{
	ObjectArray oaTokenFrequencies;

	require(not IsDeploymentMode());

	ExportFrequentTokens(&oaTokenFrequencies, nMaxTokenNumber);
	DisplayTokenArray(&oaTokenFrequencies, ost);
	oaTokenFrequencies.DeleteAll();
}

void KWTextTokenizer::SetDeploymentTokens(const StringVector* svDeploymentTokens)
{
	LongintDictionary* ldSpecificTokens;
	int i;
	int nTokenIndex;

	require(GetCollectedTokenNumber() == 0);
	require(ivUsedSpecificTokenIndexes.GetSize() == 0);
	require(gdCollectedTokens->IsStringKey());

	// Nettoyage prealable
	if (gdSpecificTokens != NULL)
		delete gdSpecificTokens;
	gdSpecificTokens = NULL;
	lvSpecificTokenFrequencies.SetSize(0);
	bIsDeploymentMode = false;

	// Prise en compte de la specification
	if (svDeploymentTokens != NULL)
	{
		bIsDeploymentMode = true;

		// Choix du type de dictionnaire selon le type de cle
		ldSpecificTokens = new LongintDictionary;
		gdSpecificTokens = ldSpecificTokens;

		// Memorisation des tokens dans le dictionnaire
		for (i = 0; i < svDeploymentTokens->GetSize(); i++)
		{
			// On utilise le sparse index plus 1 comme token index
			// (la valeur 0 signifie l'absence dans un dictionnaire de longint)
			nTokenIndex = i + 1;

			// Memorisation de l'index du token en demarant a 1 pour indiquer sa presence
			assert(ldSpecificTokens->Lookup(svDeploymentTokens->GetAt(i)) == 0);
			ldSpecificTokens->SetAt(svDeploymentTokens->GetAt(i), nTokenIndex);
		}
		assert(gdSpecificTokens->GetCount() == svDeploymentTokens->GetSize());

		// Dimensionnement du vecteur d'effectifs
		lvSpecificTokenFrequencies.SetSize(svDeploymentTokens->GetSize() + 1);
	}
}

KWContinuousValueBlock* KWTextTokenizer::BuildBlockFromString(const ALString& sValue)
{
	KWContinuousValueBlock* resultValueBlock;

	require(IsDeploymentMode());
	require(ivUsedSpecificTokenIndexes.GetSize() == 0);
	require(CheckParameters());

	// Tokenisation
	TokenizeText(sValue, sValue.GetLength());

	// Fabrication du bloc de valeurs
	resultValueBlock = BuildBlockFromDeployedTokens();
	CleanDeployedTokens();
	ensure(ivUsedSpecificTokenIndexes.GetSize() == 0);
	return resultValueBlock;
}

KWContinuousValueBlock* KWTextTokenizer::BuildBlockFromSymbol(const Symbol& sValue)
{
	KWContinuousValueBlock* resultValueBlock;

	require(IsDeploymentMode());
	require(ivUsedSpecificTokenIndexes.GetSize() == 0);
	require(CheckParameters());

	// Tokenisation
	TokenizeText(sValue, sValue.GetLength());

	// Fabrication du bloc de valeurs
	resultValueBlock = BuildBlockFromDeployedTokens();
	CleanDeployedTokens();
	ensure(ivUsedSpecificTokenIndexes.GetSize() == 0);
	return resultValueBlock;
}

KWContinuousValueBlock* KWTextTokenizer::BuildBlockFromStringVector(const StringVector* svValues)
{
	KWContinuousValueBlock* resultValueBlock;
	int i;

	require(IsDeploymentMode());
	require(ivUsedSpecificTokenIndexes.GetSize() == 0);
	require(CheckParameters());
	require(svValues != NULL);

	// Tokenisation
	for (i = 0; i < svValues->GetSize(); i++)
		TokenizeText(svValues->GetAt(i), svValues->GetAt(i).GetLength());

	// Fabrication du bloc de valeurs
	resultValueBlock = BuildBlockFromDeployedTokens();
	CleanDeployedTokens();
	ensure(ivUsedSpecificTokenIndexes.GetSize() == 0);
	return resultValueBlock;
}

KWContinuousValueBlock* KWTextTokenizer::BuildBlockFromSymbolVector(const SymbolVector* svValues)
{
	KWContinuousValueBlock* resultValueBlock;
	int i;

	require(IsDeploymentMode());
	require(ivUsedSpecificTokenIndexes.GetSize() == 0);
	require(CheckParameters());
	require(svValues != NULL);

	// Tokenisation
	for (i = 0; i < svValues->GetSize(); i++)
		TokenizeText(svValues->GetAt(i), svValues->GetAt(i).GetLength());

	// Fabrication du bloc de valeurs
	resultValueBlock = BuildBlockFromDeployedTokens();
	CleanDeployedTokens();
	ensure(ivUsedSpecificTokenIndexes.GetSize() == 0);
	return resultValueBlock;
}

void KWTextTokenizer::SortTokenArray(ObjectArray* oaTokens)
{
	require(oaTokens != NULL);

	oaTokens->SetCompareFunction(KWTokenFrequencyCompareFrequency);
	oaTokens->Sort();
}

void KWTextTokenizer::FilterTokenArray(ObjectArray* oaTokens, int nMaxTokenNumber)
{
	int nToken;

	require(oaTokens != NULL);
	require(nMaxTokenNumber >= 0);

	// Suppression des eventuels tokens surnumeraires
	if (oaTokens->GetSize() > nMaxTokenNumber)
	{
		for (nToken = nMaxTokenNumber; nToken < oaTokens->GetSize(); nToken++)
			delete oaTokens->GetAt(nToken);
		oaTokens->SetSize(nMaxTokenNumber);
	}
}

void KWTextTokenizer::DisplayTokenArray(const ObjectArray* oaTokens, ostream& ost)
{
	DisplayHeadTokenArray(oaTokens, oaTokens->GetSize(), ost);
}

void KWTextTokenizer::DisplayHeadTokenArray(const ObjectArray* oaTokens, int nMaxTokenNumber, ostream& ost)
{
	KWTokenFrequency* token;
	int i;

	require(oaTokens != NULL);

	// Entete
	cout << "Rank\tToken\tFrequency\n";

	// Parcours de tokens a afficher
	for (i = 0; i < oaTokens->GetSize(); i++)
	{
		if (i >= nMaxTokenNumber)
			break;
		token = cast(KWTokenFrequency*, oaTokens->GetAt(i));
		ost << i + 1 << "\t" << ToPrintable(token->GetToken()) << "\t" << token->GetFrequency() << "\n";
	}
}

boolean KWTextTokenizer::CheckTextFeatures(const ALString& sValue)
{
	return sValue == "ngrams" or sValue == "words" or sValue == "tokens";
}

KWTextTokenizer* KWTextTokenizer::CreateTextTokenizer(const ALString& sTextFeatures)
{
	KWTextTokenizer* textTokenizer;

	require(CheckTextFeatures(sTextFeatures));

	if (sTextFeatures == "ngrams")
		textTokenizer = new KWTextNgramTokenizer;
	else if (sTextFeatures == "words")
		textTokenizer = new KWTextWordTokenizer;
	else
		textTokenizer = new KWTextTokenizer;
	return textTokenizer;
}

int KWTextTokenizer::GetSpaceCharNumber(const ALString& sValue)
{
	int nNumber;
	int i;

	nNumber = 0;
	for (i = 0; i < sValue.GetLength(); i++)
	{
		if (iswspace(sValue.GetAt(i)))
			nNumber++;
	}
	return nNumber;
}

int KWTextTokenizer::GetPunctuationCharNumber(const ALString& sValue)
{
	int nNumber;
	int i;

	nNumber = 0;
	for (i = 0; i < sValue.GetLength(); i++)
	{
		if (ispunct(sValue.GetAt(i)))
			nNumber++;
	}
	return nNumber;
}

longint KWTextTokenizer::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWTextTokenizer);
	if (gdSpecificTokens != NULL)
		lUsedMemory += gdSpecificTokens->GetUsedMemory();
	lUsedMemory += gdCollectedTokens->GetUsedMemory();
	lUsedMemory += lvSpecificTokenFrequencies.GetUsedMemory() - sizeof(LongintVector);
	lUsedMemory += ivUsedSpecificTokenIndexes.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

void KWTextTokenizer::Write(ostream& ost) const
{
	ost << GetClassLabel() << " " << GetObjectLabel() << "\n";
	ost << "Collected tokens: " << GetCollectedTokenNumber() << "\n";
	DisplayFrequentTokens(10, ost);
}

const ALString KWTextTokenizer::GetClassLabel() const
{
	return "Text tokenizer";
}

const ALString KWTextTokenizer::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = "(Specific tokens: ";
	sLabel += IntToString(GetSpecificTokenNumber());
	sLabel += ", Max:";
	sLabel += IntToString(GetMaxCollectedTokenNumber());
	sLabel += ")";
	return sLabel;
}

void KWTextTokenizer::Test()
{
	KWTextTokenizer tokenizer;

	tokenizer.TokenizeString("Bonjour, tout le monde!!!");
	tokenizer.DisplayTokens(cout);
}

void KWTextTokenizer::TokenizeText(const char* sText, int nTextLength)
{
	debug(boolean bDisplay = false);
	LongintDictionary* ldCollectedTokens;
	LongintDictionary* ldSpecificTokens;
	const char* sStringValue;
	unsigned char cChar;
	boolean bIsSpace;
	boolean bEndToken;
	boolean bEnd;
	ALString sToken;
	debug(int nCheckedTotalSpaceCharNumber = 0);
	debug(int nCheckedTotalTokenCharNumber = 0);
	debug(int nCheckedTotalIgnoredCharNumber = 0);

	// Affichage de la chaine de caractere a analyser
	debug(if (bDisplay) cout << sText << "\n");

	// Acces aux tokens du bon type
	ldCollectedTokens = cast(LongintDictionary*, gdCollectedTokens);
	ldSpecificTokens = cast(LongintDictionary*, gdSpecificTokens);

	// Parcours des caracteres de la chaines pour extraire les tokens
	sStringValue = sText;
	bEnd = false;
	bEndToken = false;
	while (not bEnd)
	{
		// Acces au caractere courant
		cChar = *sStringValue;
		bIsSpace = cChar == ' ';
		debug(nCheckedTotalSpaceCharNumber += (bIsSpace and cChar != '\0') ? 1 : 0);

		// Preparation du caractere suivant
		bEnd = (cChar == '\0');
		sStringValue++;

		// Test si fin de token
		if (bEnd or bIsSpace)
			bEndToken = true;

		// Ajout d'un caractere si on est pas en fin de token
		if (not bEndToken)
		{
			// Prise en compte du caractere si token pas trop long
			if (sToken.GetLength() < GetMaxWordLength())
				sToken += cChar;
			else
			// Sinon, on ignore la fin du token
			{
				// On aspire la fin du token en ignorant ses caracteres
				debug(nCheckedTotalIgnoredCharNumber += 1);
				while (not bEnd)
				{
					// Acces au caractere courant
					cChar = *sStringValue;
					bIsSpace = cChar == ' ';
					debug(nCheckedTotalSpaceCharNumber += (bIsSpace and cChar != '\0') ? 1 : 0);

					// Preparation du caractere suivant
					bEnd = (cChar == '\0');
					sStringValue++;

					// Arret si fin de token
					if (bIsSpace)
						break;
					debug(nCheckedTotalIgnoredCharNumber += (cChar != '\0') ? 1 : 0);
				}
				bEndToken = true;

				// On rajoute "..." en fin de token pour signifier qu'il a ete tronque
				sToken += "...";
				debug(nCheckedTotalIgnoredCharNumber -= 3);
			}
		}

		// Memorisation si fin de token
		if (bEndToken)
		{
			// On traite les tokens non vides
			if (sToken.GetLength() > 0)
			{
				assert(sToken.GetLength() <= GetMaxWordLength() or
				       sToken.GetLength() == GetMaxWordLength() + 3);
				debug(nCheckedTotalTokenCharNumber += sToken.GetLength());

				// Incrementation de son effectif
				UpgradeTokenFrequency(sToken, 1);
				debug(assert(sToken.Find(' ') == -1));
				assert(bEnd or (sStringValue - (const char*)sText - (bIsSpace ? 0 : 1) ==
						nCheckedTotalSpaceCharNumber + nCheckedTotalTokenCharNumber +
						    nCheckedTotalIgnoredCharNumber));

				// Affichage
				debug(if (bDisplay) cout << "\t" << sToken.GetLength() << "\t"
							 << ldCollectedTokens->Lookup(sToken) << "\t("
							 << ByteStringToWord(sToken) << ")\n");

				// Reinitialisation du token, sans desallouer sa memoire
				sToken.GetBufferSetLength(0);
			}
			bEndToken = false;
		}
	}
	debug(assert(nTextLength ==
		     nCheckedTotalSpaceCharNumber + nCheckedTotalTokenCharNumber + nCheckedTotalIgnoredCharNumber));
}

void KWTextTokenizer::UpgradeTokenFrequency(const ALString& sToken, longint lFrequency)
{
	int nSpecificTokenIndex;

	require(gdCollectedTokens->IsStringKey());
	require(lFrequency >= 0);

	// Incrementation de son effectif dans le cas sans token specifique
	if (gdSpecificTokens == NULL)
		cast(LongintDictionary*, gdCollectedTokens)->UpgradeAt(sToken, 1);
	// Et dans le cas avec token specifique
	else
	{
		// Recherche de l'index du token specifique
		nSpecificTokenIndex = (int)cast(LongintDictionary*, gdSpecificTokens)->Lookup(sToken);

		// Prise en compte si necessaire
		if (nSpecificTokenIndex > 0)
		{
			// Memorisation la premiere fois
			if (lvSpecificTokenFrequencies.GetAt(nSpecificTokenIndex) == 0)
				ivUsedSpecificTokenIndexes.Add(nSpecificTokenIndex);

			// Incrementation de l'effectif
			lvSpecificTokenFrequencies.UpgradeAt(nSpecificTokenIndex, 1);
		}
	}
}

void KWTextTokenizer::StreamCleanTokenDictionary(GenericDictionary* gdTokenDictionary, int nMaxTokenNumber)
{
	const int nMaxCollectedFrequency = 100;
	IntVector ivNumbersOfTokensPerFrequency;
	int nMinFrequency;
	int nTargetTokenNumber;

	require(not IsDeploymentMode());
	require(gdTokenDictionary != NULL);
	require(not gdTokenDictionary->IsObjectValue());
	require(nMaxTokenNumber >= 0);

	// On effectue la decrementation en boucle, car contrairement a l'algorithme de Karp et al, la decrementation
	// n'est pas effectue apres chaque mise a jour d'un compte, mais apres la mise a jour de tous les comptes d'un
	// texte
	if (nMaxTokenNumber > 0)
	{
		// On commence par une decrementation simple et rapide, car c'est le cas le plus frequent de loin
		if (gdTokenDictionary->GetCount() > nMaxTokenNumber)
		{
			// Decrementation de tous les compte de tokens
			gdTokenDictionary->UpgradeAll(-1);

			// Suppression de cle pour le compte a 0
			gdTokenDictionary->RemoveAllNullValues();
		}

		// Si cela ne suffit pas, on continue avec une boucle en calculant un effectif important a decrementer
		// par passe Cela equivaut a effectuer jusqu'a nMaxCollectedFrequency passes en une seule fois Mais
		// c'est un peu plus couteux que la passe simple prealable
		while (gdTokenDictionary->GetCount() > nMaxTokenNumber)
		{
			// Calcul des stats par effectif de token, pour evitre de decrementer les effectifs 1 par 1
			// comme dans l'algorithme de Karp et al
			ComputeTokenDictionaryFrequencyStats(gdCollectedTokens, nMaxCollectedFrequency,
							     &ivNumbersOfTokensPerFrequency);

			// On determine l'effectif minimum suffisant pour collecter les tokens les plus frequents
			nTargetTokenNumber = gdTokenDictionary->GetCount();
			nMinFrequency = 1;
			while (nTargetTokenNumber > nMaxTokenNumber and nMinFrequency < nMaxCollectedFrequency)
			{
				nTargetTokenNumber -= ivNumbersOfTokensPerFrequency.GetAt(nMinFrequency);
				nMinFrequency++;
			}
			assert(nMinFrequency > 1);
			assert(nMinFrequency == nMaxCollectedFrequency or nTargetTokenNumber <= nMaxTokenNumber);
			assert(nMinFrequency == nMaxCollectedFrequency or
			       nTargetTokenNumber + ivNumbersOfTokensPerFrequency.GetAt(nMinFrequency - 1) >
				   nMaxTokenNumber);

			// Decrementation de tous les comptes de tokens de l'effectif minimum -1, et troncature a zero
			gdTokenDictionary->BoundedUpgradeAll(-(nMinFrequency - 1), 0, LLONG_MAX);

			// Suppression de cle pour le compte a 0
			gdTokenDictionary->RemoveAllNullValues();
			assert(gdTokenDictionary->GetCount() == nTargetTokenNumber);
		}
	}
}

void KWTextTokenizer::ComputeTokenDictionaryFrequencyStats(const GenericDictionary* gdTokenDictionary,
							   int nMaxFrequency,
							   IntVector* ivNumbersOfTokensPerFrequency) const
{
	const LongintDictionary* ldTokenDictionary;
	const LongintNumericKeyDictionary* lnkdTokenDictionary;
	POSITION position;
	ALString sKey;
	NUMERIC numericKey;
	longint lFrequency;

	require(gdTokenDictionary != NULL);
	require(not gdTokenDictionary->IsObjectValue());
	require(ivNumbersOfTokensPerFrequency != NULL);

	// Initialisation du vecteur de stats
	ivNumbersOfTokensPerFrequency->SetSize(nMaxFrequency + 1);
	ivNumbersOfTokensPerFrequency->Initialize();

	// Collecte des stats selon le type de dictionnaire
	if (gdTokenDictionary->IsStringKey())
	{
		ldTokenDictionary = cast(const LongintDictionary*, gdTokenDictionary);
		position = ldTokenDictionary->GetStartPosition();
		while (position != NULL)
		{
			ldTokenDictionary->GetNextAssoc(position, sKey, lFrequency);

			// Incrementation du nombre de tokens pour l'effectif
			if (lFrequency <= nMaxFrequency)
				ivNumbersOfTokensPerFrequency->UpgradeAt((int)lFrequency, 1);
		}
	}
	else
	{
		lnkdTokenDictionary = cast(const LongintNumericKeyDictionary*, gdTokenDictionary);
		position = lnkdTokenDictionary->GetStartPosition();
		while (position != NULL)
		{
			lnkdTokenDictionary->GetNextAssoc(position, numericKey, lFrequency);

			// Incrementation du nombre de tokens pour l'effectif
			if (lFrequency <= nMaxFrequency)
				ivNumbersOfTokensPerFrequency->UpgradeAt((int)lFrequency, 1);
		}
	}
}

KWContinuousValueBlock* KWTextTokenizer::BuildBlockFromDeployedTokens()
{
	KWContinuousValueBlock* resultValueBlock;
	int i;
	int nTokenIndex;

	// Tri des index de tokens pour les avoir dans le bon ordre dans le bloc de valeurs sparse
	ivUsedSpecificTokenIndexes.Sort();

	// Creation du bloc de valeur sparse a la bonne taille
	resultValueBlock = KWContinuousValueBlock::NewValueBlock(ivUsedSpecificTokenIndexes.GetSize());

	// Rangement des index d'attribut et des valeurs selon leur ordre
	for (i = 0; i < ivUsedSpecificTokenIndexes.GetSize(); i++)
	{
		nTokenIndex = ivUsedSpecificTokenIndexes.GetAt(i);
		assert(nTokenIndex >= 1);

		// Memorisation de l'index d'attribut (debut a 0) et de la valeur
		resultValueBlock->SetAttributeSparseIndexAt(i, nTokenIndex - 1);
		resultValueBlock->SetValueAt(i, (Continuous)lvSpecificTokenFrequencies.GetAt(nTokenIndex));
	}
	return resultValueBlock;
}

void KWTextTokenizer::CleanDeployedTokens()
{
	int i;
	int nTokenIndex;

	require(IsDeploymentMode());

	// Remise a zero des effectifs des tokens deployes en preparation du prochain deploiement
	for (i = 0; i < ivUsedSpecificTokenIndexes.GetSize(); i++)
	{
		nTokenIndex = ivUsedSpecificTokenIndexes.GetAt(i);
		assert(nTokenIndex >= 1);
		lvSpecificTokenFrequencies.SetAt(nTokenIndex, 0);
	}
	ivUsedSpecificTokenIndexes.SetSize(0);
}

longint KWTextTokenizer::ComputeTotalTokenFrequency() const
{
	longint lTotalTokenFrequency;
	int i;
	int nTokenIndex;

	// On prend en compte les token collecte via le dictionnaire de tous les tokens
	lTotalTokenFrequency = gdCollectedTokens->ComputeTotalValue();

	// Ainsi que ceux via les tokens specifiques
	for (i = 0; i < ivUsedSpecificTokenIndexes.GetSize(); i++)
	{
		nTokenIndex = ivUsedSpecificTokenIndexes.GetAt(i);
		lTotalTokenFrequency += lvSpecificTokenFrequencies.GetAt(nTokenIndex);
	}
	return lTotalTokenFrequency;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Classe KWTextNgramTokenizer

KWTextNgramTokenizer::KWTextNgramTokenizer()
{
	assert(gdCollectedTokens != NULL);
	delete gdCollectedTokens;
	gdCollectedTokens = new LongintNumericKeyDictionary;
}

KWTextNgramTokenizer::~KWTextNgramTokenizer() {}

void KWTextNgramTokenizer::SetSpecificTokens(const ObjectArray* oaTokens)
{
	LongintNumericKeyDictionary* lnkdSpecificTokens;
	const KWTokenFrequency* token;
	longint lEncodedNgram;
	int nToken;

	require(GetCollectedTokenNumber() == 0);
	require(not gdCollectedTokens->IsStringKey());
	require(lvSpecificTokenFrequencies.GetSize() == 0);
	require(ivUsedSpecificTokenIndexes.GetSize() == 0);

	// Nettoyage prealable
	if (gdSpecificTokens != NULL)
		delete gdSpecificTokens;
	gdSpecificTokens = NULL;

	// Prise en compte de la specification
	if (oaTokens != NULL)
	{
		// Choix du type de dictionnaire selon le type de cle, ici des cles numeriques
		lnkdSpecificTokens = new LongintNumericKeyDictionary;
		gdSpecificTokens = lnkdSpecificTokens;

		// Memorisation des tokens dans le dictionnaire
		for (nToken = 0; nToken < oaTokens->GetSize(); nToken++)
		{
			token = cast(const KWTokenFrequency*, oaTokens->GetAt(nToken));

			// Memorisation de l'index du token en demarant a 1 pour indiquer sa presence
			lEncodedNgram = NgramToLongint(token->GetToken());
			assert(lnkdSpecificTokens->Lookup(lEncodedNgram) == 0);
			lnkdSpecificTokens->SetAt(lEncodedNgram, nToken + 1);

			// Dimensionnement du vecteur d'effectifs
			lvSpecificTokenFrequencies.SetSize(oaTokens->GetSize() + 1);
		}
		assert(gdSpecificTokens->GetCount() == oaTokens->GetSize());
	}
}

void KWTextNgramTokenizer::ExportFrequentTokens(ObjectArray* oaTokenFrequencies, int nMaxTokenNumber) const
{
	const int nMaxCollectedFrequency = 1000;
	const LongintNumericKeyDictionary* lnkdCollectedTokens;
	const LongintNumericKeyDictionary* lnkdSpecificTokens;
	POSITION position;
	NUMERIC numericKey;
	longint lValue;
	IntVector ivNumbersOfTokensPerFrequency;
	int nMinFrequency;
	int nCollectedTokenNumber;
	int nSpecificTokenIndex;
	KWTokenFrequency* token;
	int nIndex;

	require(gdCollectedTokens != NULL);
	require(not gdCollectedTokens->IsStringKey());
	require(not gdCollectedTokens->IsObjectValue());
	require(nMaxTokenNumber >= 0);
	require(oaTokenFrequencies != NULL);
	require(oaTokenFrequencies->GetSize() == 0);

	// Choix du type de dictionnaire selon le type de cle
	lnkdCollectedTokens = cast(const LongintNumericKeyDictionary*, gdCollectedTokens);
	lnkdSpecificTokens = cast(const LongintNumericKeyDictionary*, gdSpecificTokens);

	// Cas sans tokens specifiques
	if (lnkdSpecificTokens == NULL)
	{
		// Initialisation du nombre de tokens a collecter sans contrainte d'effectif minimum
		nCollectedTokenNumber = gdCollectedTokens->GetCount();
		nMinFrequency = 0;

		// Si necessaire, passe prealabale de collecte des stats sur les nombres de tokens par effectif
		if (gdCollectedTokens->GetCount() > 2 * nMaxTokenNumber)
		{
			// Calcul des stats par effectif de token
			ComputeTokenDictionaryFrequencyStats(gdCollectedTokens, nMaxCollectedFrequency,
							     &ivNumbersOfTokensPerFrequency);

			// On determine l'effectif minimum suffisant pour collecter les tokens les plus frequents
			for (nMinFrequency = 1; nMinFrequency <= nMaxCollectedFrequency; nMinFrequency++)
			{
				if (nCollectedTokenNumber - ivNumbersOfTokensPerFrequency.GetAt(nMinFrequency) >=
				    nMaxTokenNumber)
					nCollectedTokenNumber -= ivNumbersOfTokensPerFrequency.GetAt(nMinFrequency);
				else
					break;
			}
		}

		// Parcours des cles pour creer l'ensemble des tokens, en ne gardant que ceux ayant un effectif minimum
		// suffisant
		assert(nMinFrequency > 0 or nCollectedTokenNumber == gdCollectedTokens->GetCount());
		oaTokenFrequencies->SetSize(nCollectedTokenNumber);
		nIndex = 0;
		position = lnkdCollectedTokens->GetStartPosition();
		while (position != NULL)
		{
			lnkdCollectedTokens->GetNextAssoc(position, numericKey, lValue);

			// Creation et memorisation du token de type ngrams selon la contrainte d'effectif minimum
			if (lValue >= nMinFrequency)
			{
				token = new KWTokenFrequency;
				token->SetToken(LongintToNgram(numericKey.ToLongint()));
				token->SetFrequency(lValue);
				oaTokenFrequencies->SetAt(nIndex, token);
				nIndex++;
			}
		}
	}
	// Cas avec tokens specifiques
	else
	{
		assert(gdCollectedTokens->GetCount() == 0);

		// Initialisation du nombre de tokens a collecter sans contrainte d'effectif minimum
		nCollectedTokenNumber = ivUsedSpecificTokenIndexes.GetSize();

		// Parcours des tokens specifiques pour retrouver leur index et donc leur effectif
		oaTokenFrequencies->SetSize(nCollectedTokenNumber);
		nIndex = 0;
		position = lnkdSpecificTokens->GetStartPosition();
		while (position != NULL)
		{
			lnkdSpecificTokens->GetNextAssoc(position, numericKey, lValue);
			nSpecificTokenIndex = (int)lValue;

			// Creation et memorisation du token de type ngrams selon la contrainte d'effectif minimum
			if (lvSpecificTokenFrequencies.GetAt(nSpecificTokenIndex) > 0)
			{
				token = new KWTokenFrequency;
				token->SetToken(LongintToNgram(numericKey.ToLongint()));
				token->SetFrequency(lvSpecificTokenFrequencies.GetAt(nSpecificTokenIndex));
				oaTokenFrequencies->SetAt(nIndex, token);
				nIndex++;
			}
		}
	}

	// Filtrage de la fin du tableau apres tri
	SortTokenArray(oaTokenFrequencies);
	FilterTokenArray(oaTokenFrequencies, nMaxTokenNumber);
}

void KWTextNgramTokenizer::CumulateTokenFrequencies(const ObjectArray* oaTokens)
{
	int nToken;
	const KWTokenFrequency* token;
	longint lEncodedNgram;

	require(CheckParameters());
	require(oaTokens != NULL);

	// Prise en compte des effectifs des tokens
	for (nToken = 0; nToken < oaTokens->GetSize(); nToken++)
	{
		token = cast(const KWTokenFrequency*, oaTokens->GetAt(nToken));

		// Ajout de l'effectif du token
		lEncodedNgram = NgramToLongint(token->GetToken());
		UpgradeNgramTokenFrequency(lEncodedNgram, token->GetFrequency());
	}
}

void KWTextNgramTokenizer::SetDeploymentTokens(const StringVector* svDeploymentTokens)
{
	LongintNumericKeyDictionary* lnkdSpecificTokens;
	Symbol sKey;
	longint lEncodedNgram;
	int i;
	int nTokenIndex;

	require(GetCollectedTokenNumber() == 0);
	require(ivUsedSpecificTokenIndexes.GetSize() == 0);
	require(not gdCollectedTokens->IsStringKey());

	// Nettoyage prealable
	if (gdSpecificTokens != NULL)
		delete gdSpecificTokens;
	gdSpecificTokens = NULL;
	lvSpecificTokenFrequencies.SetSize(0);
	bIsDeploymentMode = false;

	// Prise en compte de la specification
	if (svDeploymentTokens != NULL)
	{
		bIsDeploymentMode = true;

		// Choix du type de dictionnaire selon le type de cle
		lnkdSpecificTokens = new LongintNumericKeyDictionary;
		gdSpecificTokens = lnkdSpecificTokens;

		// Memorisation des tokens dans le dictionnaire
		for (i = 0; i < svDeploymentTokens->GetSize(); i++)
		{
			// On deduit une valeur de token de la cle
			lEncodedNgram = NgramToLongint(svDeploymentTokens->GetAt(i));

			// On utilise le sparse index plus 1 comme token index
			// (la valeur 0 signifie l'absence dans un dictionnaire de longint)
			nTokenIndex = i + 1;

			// Memorisation de l'index du token en demarant a 1 pour indiquer sa presence
			assert(lnkdSpecificTokens->Lookup(lEncodedNgram) == 0);
			lnkdSpecificTokens->SetAt(lEncodedNgram, nTokenIndex);
		}
		assert(gdSpecificTokens->GetCount() == svDeploymentTokens->GetSize());

		// Dimensionnement du vecteur d'effectifs
		lvSpecificTokenFrequencies.SetSize(svDeploymentTokens->GetSize() + 1);
	}
}

const ALString KWTextNgramTokenizer::GetClassLabel() const
{
	return "Text ngrams tokenizer";
}

void KWTextNgramTokenizer::Test()
{
	KWTextNgramTokenizer tokenizer;
	int nEncodingNumber = 10000000;
	StringVector svTextValues;
	ALString sValue;
	ALString sWord;
	ALString sRetrievedValue;
	int i;
	int j;
	int nLength;
	longint lEncodedNgram;
	LongintNumericKeyDictionary lnkdOutputNgramsFrequencies;
	LongintDictionary ldOutputWordFrequencies;
	LongintDictionary ldOutputTokenFrequencies;
	ObjectArray oaTokens;
	Timer timer;
	ALString sTmp;

	// Diminution des tests en mode debug
	debug(nEncodingNumber = 100000);

	// Initialisation de l'echantillon de texts
	BuildTextSample(&svTextValues);

	// Conversion des valeurs de type ngrams vers des longint
	for (i = 0; i < svTextValues.GetSize(); i++)
	{
		// Encodage/decodage de la valeur en longint
		sValue = svTextValues.GetAt(i);
		if (sValue.GetLength() <= GetMaxNgramLength())
		{
			lEncodedNgram = NgramToLongint(sValue);
			sRetrievedValue = LongintToNgram(lEncodedNgram);
			assert(sRetrievedValue == sValue);

			// Affichage
			cout << i << "\t";
			cout << ToPrintable(sValue) << "\t";
			cout << lEncodedNgram << "\t";
			cout << ToPrintable(sRetrievedValue) << "\n";
		}
	}

	// Conversion de ngrams aleatoires vers des longint
	timer.Reset();
	for (i = 0; i < nEncodingNumber; i++)
	{
		// Creation d'un mot aleatoire
		sValue = "";
		nLength = 1 + RandomInt(GetMaxNgramLength() - 1);
		for (j = 0; j < nLength; j++)
			sValue += (char)(1 + RandomInt(254));

		// Encodage/decodage de la valeur en mot
		timer.Start();
		lEncodedNgram = NgramToLongint(sValue);
		sRetrievedValue = LongintToNgram(lEncodedNgram);
		timer.Stop();
		assert(lEncodedNgram == SubNgramToLongint(sValue, 0, sValue.GetLength()));
		assert(sRetrievedValue == sValue);
	}
	cout << "Longint encoding/decoding time for " << nEncodingNumber << " ngrams: " << timer.GetElapsedTime()
	     << "\n";

	// Test de tokenization en ngrams
	tokenizer.TokenizeString("Bonjour, tout le monde!!!");
	tokenizer.DisplayTokens(cout);
}

void KWTextNgramTokenizer::TokenizeText(const char* sText, int nTextLength)
{
	debug(boolean bDisplay = false);
	const char* sCurrentStringValue;
	int nMaxUsableNgramLength;
	longint lEncodedNgram;
	char* sEncodedNgramBytes;
	int n;
	int i;
	debug(static int nMethodCallIndex = 0);
	debug(int nNgramLength);
	debug(LongintNumericKeyDictionary * lnkdCollectedTokens);
	debug(LongintNumericKeyDictionary * lnkdSpecificTokens);
	debug(longint lInitialCumulatedTokenCount);
	debug(longint lExpectedCumulatedTokenCount);
	debug(longint lFinalCumulatedTokenCount);

	require(sText != NULL);
	require((int)strlen(sText) == nTextLength);

	// Affichage de la chaine de caractere a analyser
	debug(if (bDisplay) cout << sText << "\n");

	// Effectif cumule initial des tokens
	// On ne le fait que de temps en temps, car c'est tres couteux
	debug(nMethodCallIndex++);
	debug(if (gdSpecificTokens != NULL) nMethodCallIndex = 0);
	debug(lInitialCumulatedTokenCount = 0);
	debug(if (nMethodCallIndex % 1000 == 1) lInitialCumulatedTokenCount = ComputeTotalTokenFrequency());
	debug(lnkdCollectedTokens = cast(LongintNumericKeyDictionary*, gdCollectedTokens));
	debug(lnkdSpecificTokens = cast(LongintNumericKeyDictionary*, gdSpecificTokens));

	// Acces a la memoire du longint lEncodedNgram sous forme d'un tableau de bytes
	sEncodedNgramBytes = (char*)&lEncodedNgram;

	// Analyse de la chaine pour en extraire tous les ngrams
	sCurrentStringValue = sText;
	for (n = 0; n < nTextLength; n++)
	{
		assert(sCurrentStringValue[0] == sText[n]);

		// Remise a zero de tous les bytes du ngram encode en longint
		lEncodedNgram = 0;

		// Analyse par longueur de ngram
		nMaxUsableNgramLength = min(GetMaxNgramLength(), nTextLength - n);
		for (i = 0; i < nMaxUsableNgramLength; i++)
		{
			assert(sEncodedNgramBytes[i] == '\0');

			// Recopie directe du byte additionnel du ngram a sa position dans la longint, traite comme un
			// tableau de bytes
			sEncodedNgramBytes[i] = sCurrentStringValue[i];
			debug(nNgramLength = i + 1);
			assert(lEncodedNgram == SubNgramToLongint(sText, n, nNgramLength));

			// Incrementation de son effectif
			UpgradeNgramTokenFrequency(lEncodedNgram, 1);

			// Affichage
			debug(if (bDisplay) cout
			      << "\t" << n << "\t" << nNgramLength << "\t"
			      << (lnkdSpecificTokens != NULL ? lnkdSpecificTokens->Lookup(lEncodedNgram)
							     : lnkdCollectedTokens->Lookup(lEncodedNgram))
			      << "\t(" << ByteStringToWord(LongintToNgram(lEncodedNgram)) << ")\n");
		}

		// Decallage de la position courante
		sCurrentStringValue++;
	}

	// Effectif cumule attendu des tokens suite a l'extraction des ngrams
	debug(lExpectedCumulatedTokenCount = 0);
	debug(for (nNgramLength = 1; nNgramLength <= GetMaxNgramLength(); nNgramLength++)
		  lExpectedCumulatedTokenCount += max(nTextLength - nNgramLength + 1, 0));
	debug(lFinalCumulatedTokenCount = 0);
	debug(if (nMethodCallIndex % 1000 == 1) lFinalCumulatedTokenCount = ComputeTotalTokenFrequency());
	debug(ensure(nMethodCallIndex % 1000 != 1 or
		     lInitialCumulatedTokenCount + lExpectedCumulatedTokenCount == lFinalCumulatedTokenCount));
}

void KWTextNgramTokenizer::UpgradeNgramTokenFrequency(longint lEncodedNgram, longint lFrequency)
{
	int nSpecificTokenIndex;

	require(not gdCollectedTokens->IsStringKey());
	require(lFrequency >= 0);

	// Incrementation de son effectif dans le cas sans token specifique
	if (gdSpecificTokens == NULL)
		cast(LongintNumericKeyDictionary*, gdCollectedTokens)->UpgradeAt(lEncodedNgram, 1);
	// Et dans le cas avec token specifique
	else
	{
		// Recherche de l'index du token specifique
		nSpecificTokenIndex = (int)cast(LongintNumericKeyDictionary*, gdSpecificTokens)->Lookup(lEncodedNgram);

		// Prise en compte si necessaire
		if (nSpecificTokenIndex > 0)
		{
			// Memorisation la premiere fois
			if (lvSpecificTokenFrequencies.GetAt(nSpecificTokenIndex) == 0)
				ivUsedSpecificTokenIndexes.Add(nSpecificTokenIndex);

			// Incrementation de l'effectif
			lvSpecificTokenFrequencies.UpgradeAt(nSpecificTokenIndex, 1);
		}
	}
}

longint KWTextNgramTokenizer::NgramToLongint(const ALString& sNgramBytes)
{
	longint lEncodedNgram;

	require(sNgramBytes.GetLength() > 0);
	require(sNgramBytes.GetLength() <= GetMaxNgramLength());

	// Initialisation a 0 du longint, donc de tous ses bytes
	lEncodedNgram = 0;

	// Recopie directes des bytes du nGram au debut du longint, traite comme un tableau de bytes
	memcpy((void*)&lEncodedNgram, (const char*)sNgramBytes, sNgramBytes.GetLength());
	return lEncodedNgram;
}

longint KWTextNgramTokenizer::SubNgramToLongint(const ALString& sNgramBytes, int nStart, int nGramLength)
{
	longint lEncodedNgram;

	require(sNgramBytes.GetLength() > 0);
	require(nStart >= 0);
	require(1 <= nGramLength and nGramLength <= GetMaxNgramLength());
	require(nStart + nGramLength <= sNgramBytes.GetLength());

	// Initialisation a 0 du longint, donc de tous ses bytes
	lEncodedNgram = 0;

	// Recopie directes des bytes du nGram au debut du longint, traite comme un tableau de bytes
	memcpy((void*)&lEncodedNgram, &((const char*)sNgramBytes)[nStart], nGramLength);
	return lEncodedNgram;
}

const ALString KWTextNgramTokenizer::LongintToNgram(longint lValue)
{
	ALString sNgramBytes;
	char* sBytes;

	// On alloue au moins la place necessaire pour tous les bytes d'un ngram de longueur max
	sBytes = sNgramBytes.GetBuffer(GetMaxNgramLength());

	// On recopie directement les bytes du longint
	*(longint*)sBytes = lValue;

	// On ajuste la longueur de la chaine a son contenu
	sNgramBytes.ReleaseBuffer();
	return sNgramBytes;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Classe KWTextWordTokenizer

KWTextWordTokenizer::KWTextWordTokenizer()
{
	assert(gdCollectedTokens != NULL);
}

KWTextWordTokenizer::~KWTextWordTokenizer() {}

const ALString KWTextWordTokenizer::GetClassLabel() const
{
	return "Text word tokenizer";
}

void KWTextWordTokenizer::Test()
{
	KWTextWordTokenizer tokenizer;

	tokenizer.TokenizeString("Bonjour, tout le monde!!!");
	tokenizer.DisplayTokens(cout);
}

void KWTextWordTokenizer::TokenizeText(const char* sText, int nTextLength)
{
	debug(boolean bDisplay = false);
	LongintDictionary* ldCollectedTokens;
	LongintDictionary* ldSpecificTokens;
	const char* sStringValue;
	char cChar;
	boolean bIsTokenPunctuation;
	boolean bIsSpace;
	boolean bIsPunctuation;
	boolean bEndToken;
	boolean bEnd;
	ALString sToken;
	debug(int nCheckedTotalSpaceCharNumber = 0);
	debug(int nCheckedTotalTokenCharNumber = 0);
	debug(int nCheckedTotalIgnoredCharNumber = 0);

	// Affichage de la chaine de caractere a analyser
	debug(if (bDisplay) cout << sText << "\n");

	// Acces aux tokens du bon type
	ldCollectedTokens = cast(LongintDictionary*, gdCollectedTokens);
	ldSpecificTokens = cast(LongintDictionary*, gdSpecificTokens);

	// Parcours des caracteres de la chaines pour extraire les tokens
	sStringValue = sText;
	bEnd = false;
	bIsTokenPunctuation = false;
	bEndToken = false;
	while (not bEnd)
	{
		// Acces au caractere courant
		cChar = *sStringValue;
		bIsSpace = iswspace(cChar) or iscntrl(cChar);
		bIsPunctuation = ispunct(cChar);
		debug(nCheckedTotalSpaceCharNumber += (bIsSpace and cChar != '\0') ? 1 : 0);

		// Preparation du caractere suivant
		bEnd = (cChar == '\0');
		sStringValue++;

		// Test si fin de token
		if (bEnd or bIsSpace)
			bEndToken = true;
		else if (sToken.GetLength() > 0 and bIsTokenPunctuation != bIsPunctuation)
			bEndToken = true;

		// Ajout d'un caractere si on est pas en fin de token
		if (not bEndToken)
		{
			if (sToken.GetLength() == 0)
				bIsTokenPunctuation = bIsPunctuation;

			// Prise en compte du caractere si token pas trop long
			if (sToken.GetLength() < GetMaxWordLength())
				sToken += cChar;
			// Sinon, on ignore la fin du token
			else
			{
				// On aspire la fin du token en ignorant ses caracteres
				debug(nCheckedTotalIgnoredCharNumber += 1);
				while (not bEnd)
				{
					// Acces au caractere courant
					cChar = *sStringValue;
					bIsSpace = iswspace(cChar) or iscntrl(cChar);
					bIsPunctuation = ispunct(cChar);
					debug(nCheckedTotalSpaceCharNumber += (bIsSpace and cChar != '\0') ? 1 : 0);

					// Preparation du caractere suivant
					bEnd = (cChar == '\0');
					sStringValue++;

					// Arret si fin de token
					if (bIsSpace or bIsTokenPunctuation != bIsPunctuation)
						break;
					debug(nCheckedTotalIgnoredCharNumber += 1);
				}
				bEndToken = true;

				// On rajoute "..." en fin de token pour signifier qu'il a ete tronque
				sToken += "...";
				debug(nCheckedTotalIgnoredCharNumber -= 3);
			}
		}

		// Memorisation si fin de token
		if (bEndToken)
		{
			// On traite les tokens non vides
			if (sToken.GetLength() > 0)
			{
				assert(sToken.GetLength() <= GetMaxWordLength() or
				       sToken.GetLength() == GetMaxWordLength() + 3);
				debug(nCheckedTotalTokenCharNumber += sToken.GetLength());

				// Incrementation de son effectif
				UpgradeTokenFrequency(sToken, 1);
				debug(assert(GetSpaceCharNumber(sToken) == 0));
				debug(assert(GetPunctuationCharNumber(sToken) == 0 or
					     GetPunctuationCharNumber(sToken) == sToken.GetLength() or
					     sToken.GetLength() > GetMaxWordLength()));
				assert(bEnd or (sStringValue - (const char*)sText - (bIsSpace ? 0 : 1) ==
						nCheckedTotalSpaceCharNumber + nCheckedTotalTokenCharNumber +
						    nCheckedTotalIgnoredCharNumber));

				// Affichage
				debug(if (bDisplay) cout << "\t" << sToken.GetLength() << "\t"
							 << ldCollectedTokens->Lookup(sToken) << "\t("
							 << ByteStringToWord(sToken) << ")\n");

				// Reinitialisation du token, sans desallouer sa memoire
				sToken.GetBufferSetLength(0);

				// Initialisation du token suivant si necessaire
				if (not bIsSpace)
				{
					sToken += cChar;
					bIsTokenPunctuation = bIsPunctuation;
				}
			}
			bEndToken = false;
		}
	}
	debug(assert(nTextLength ==
		     nCheckedTotalSpaceCharNumber + nCheckedTotalTokenCharNumber + nCheckedTotalIgnoredCharNumber));
}
