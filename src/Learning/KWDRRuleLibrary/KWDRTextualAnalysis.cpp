// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTextualAnalysis.h"

void KWDRRegisterTextualAnalysisRules()
{
	if (GetLearningTextVariableMode())
	{
		KWDerivationRule::RegisterDerivationRule(new KWDRTextTokens);
		KWDerivationRule::RegisterDerivationRule(new KWDRTextListTokens);
		KWDerivationRule::RegisterDerivationRule(new KWDRTextAllNGrams);
		KWDerivationRule::RegisterDerivationRule(new KWDRTextListAllNGrams);
		KWDerivationRule::RegisterDerivationRule(new KWDRTextInit);
	}
	KWDRRegisterTextualAnalysisPROTORules();
}

////////////////////////////////////////////////////////////////////////////

KWDRTextTokens::KWDRTextTokens()
{
	SetName("TextTokens");
	SetLabel("Token counts from a text value");
	SetType(KWType::ContinuousValueBlock);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Text);
}

KWDRTextTokens::~KWDRTextTokens() {}

int KWDRTextTokens::GetVarKeyType() const
{
	return KWType::Symbol;
}

KWDerivationRule* KWDRTextTokens::Create() const
{
	return new KWDRTextTokens;
}

KWContinuousValueBlock*
KWDRTextTokens::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
						  const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	Symbol sInputValue;

	require(IsCompiled());
	require(indexedKeyBlock != NULL);

	sInputValue = GetFirstOperand()->GetTextValue(kwoObject);
	cvbTokenCounts = ComputeTokenCountsFromText(sInputValue, indexedKeyBlock);
	return cvbTokenCounts;
}

Continuous KWDRTextTokens::GetValueBlockContinuousDefaultValue() const
{
	return 0;
}

longint KWDRTextTokens::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KWDRTextTokens);
	return lUsedMemory;
}

void KWDRTextTokens::Test()
{
	KWDRTextTokens tokenCounts;
	KWIndexedCKeyBlock indexedKeyBlock;

	// Initialisation des tokens a compter, dans l'ordre alphabetique impose
	indexedKeyBlock.AddKey("bonjour");
	indexedKeyBlock.AddKey("le");
	indexedKeyBlock.AddKey("monde");
	indexedKeyBlock.AddKey("tout");
	cout << "Tokens: " << indexedKeyBlock << endl;

	// Test avec plusieurs chaines
	indexedKeyBlock.IndexKeys();
	tokenCounts.TestCount("bonjour", &indexedKeyBlock);
	tokenCounts.TestCount("le monde est grand", &indexedKeyBlock);
	tokenCounts.TestCount("coucou", &indexedKeyBlock);
	tokenCounts.TestCount("le soir et le matin", &indexedKeyBlock);
}

void KWDRTextTokens::TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock)
{
	KWContinuousValueBlock* valueBlock;

	cout << sInputValue << "\n";
	valueBlock = ComputeTokenCountsFromText(Symbol(sInputValue), indexedKeyBlock);
	cout << "\t" << *valueBlock << endl;
	delete valueBlock;
}

KWContinuousValueBlock* KWDRTextTokens::ComputeTokenCountsFromText(const Symbol& sValue,
								   const KWIndexedKeyBlock* indexedKeyBlock) const
{
	SymbolVector svValues;

	svValues.Add(sValue);
	return ComputeTokenCountsFromTextList(&svValues, indexedKeyBlock);
}

KWContinuousValueBlock* KWDRTextTokens::ComputeTokenCountsFromTextList(const SymbolVector* svValues,
								       const KWIndexedKeyBlock* indexedKeyBlock) const
{
	return AdvancedComputeTokenCountsFromTextList(svValues, indexedKeyBlock);
}

KWContinuousValueBlock*
KWDRTextTokens::BasicComputeTokenCountsFromTextList(const SymbolVector* svValues,
						    const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	KWContinuousValueDictionary cvdTokenCounts;
	int nValueNumber;
	int nValue;
	Symbol sValue;
	const char* sStringValue;
	char cChar;
	ALString sToken;
	Symbol sAttributeKey;
	boolean bEnd;
	debug(longint lCheckedTotalCharNumber = 0);
	debug(longint lCheckedTotalSpaceCharNumber = 0);
	debug(longint lCheckedTotalTokenCharNumber = 0);

	require(indexedKeyBlock != NULL);

	// Nombre de valeurs de la liste
	if (svValues == NULL)
		nValueNumber = 0;
	else
		nValueNumber = svValues->GetSize();

	// Cas d'une vecteur vide ou d'une seule chaine vide
	if (nValueNumber == 0 or (nValueNumber == 1 and svValues->GetAt(0).GetLength() == 0))
	{
		cvbTokenCounts = KWContinuousValueBlock::NewValueBlock(0);
	}
	// Cas general, avec traitement pour chaque valeur du vecteur en entree et cumul sur l'ensemble.
	else
	{
		for (nValue = 0; nValue < svValues->GetSize(); nValue++)
		{
			// Acces a la valeur
			sValue = svValues->GetAt(nValue);
			debug(lCheckedTotalCharNumber += sValue.GetLength());

			// Acces a la chaine de caractere a analyser
			sStringValue = sValue.GetValue();

			// Parcours des caracteres de la chaines pour extraire les tokens
			bEnd = false;
			while (not bEnd)
			{
				// Acces au caractere courant
				cChar = *sStringValue;

				// Preparation du caractere suivant
				bEnd = (cChar == '\0');
				sStringValue++;
				debug(lCheckedTotalSpaceCharNumber += iswspace(cChar) ? 1 : 0);

				// Ajout d'un caractere s'il n'est pas blanc
				if (not iswspace(cChar) and not bEnd)
					sToken += cChar;
				// Sinon, fin du token
				else
				{
					// On traite les token non vides
					if (sToken.GetLength() > 0)
					{
						debug(lCheckedTotalTokenCharNumber += sToken.GetLength());

						// Incrementation du compte lie au token
						sAttributeKey = Symbol(sToken);
						if (cast(KWIndexedCKeyBlock*, indexedKeyBlock)
							->IsKeyPresent(sAttributeKey))
							cvdTokenCounts.UpgradeAt(sAttributeKey, 1);
						debug(assert(GetSpaceCharNumber(sToken) == 0));

						// Reinitialisation du token, sans desallouer sa memoire
						sToken.GetBufferSetLength(0);
					}
				}
			}
		}

		// Construction du bloc de compte
		cvbTokenCounts = KWContinuousValueBlock::BuildBlockFromValueDictionary(
		    cast(KWIndexedCKeyBlock*, indexedKeyBlock), &cvdTokenCounts);
	}
	debug(assert(lCheckedTotalCharNumber == lCheckedTotalSpaceCharNumber + lCheckedTotalTokenCharNumber));
	ensure(cvbTokenCounts->SearchValueIndex(0) == -1);
	return cvbTokenCounts;
}

KWContinuousValueBlock*
KWDRTextTokens::AdvancedComputeTokenCountsFromTextList(const SymbolVector* svValues,
						       const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	KWContinuousValueDictionary cvdTokenCounts;
	int nValueNumber;
	int nValue;
	Symbol sValue;
	const char* sStringValue;
	char cChar;
	ALString sToken;
	Symbol sAttributeKey;
	boolean bIsTokenPunctuation;
	boolean bIsSpace;
	boolean bIsPunctuation;
	boolean bEndToken;
	boolean bEnd;
	debug(longint lCheckedTotalCharNumber = 0);
	debug(longint lCheckedTotalSpaceCharNumber = 0);
	debug(longint lCheckedTotalTokenCharNumber = 0);

	require(indexedKeyBlock != NULL);

	// Nombre de valeurs de la liste
	if (svValues == NULL)
		nValueNumber = 0;
	else
		nValueNumber = svValues->GetSize();

	// Cas d'une vecteur vide ou d'une seule chaine vide
	if (nValueNumber == 0 or (nValueNumber == 1 and svValues->GetAt(0).GetLength() == 0))
	{
		cvbTokenCounts = KWContinuousValueBlock::NewValueBlock(0);
	}
	// Cas general, avec traitement pour chaque valeur du vecteur en entree et cumul sur l'ensemble.
	else
	{
		sStringValue = NULL;
		for (nValue = 0; nValue < svValues->GetSize(); nValue++)
		{
			assert(sStringValue == NULL);
			assert(sToken.GetLength() == 0);

			// Acces a la valeur
			sValue = svValues->GetAt(nValue);
			debug(lCheckedTotalCharNumber += sValue.GetLength());

			// Acces a la chaine de caractere a analyser
			sStringValue = sValue.GetValue();

			// Parcours des caracteres de la chaines pour extraire les tokens
			bEnd = false;
			bIsTokenPunctuation = false;
			bEndToken = false;
			while (not bEnd)
			{
				// Acces au caractere courant
				cChar = *sStringValue;
				bIsSpace = iswspace(cChar);
				bIsPunctuation = ispunct(cChar);
				debug(lCheckedTotalSpaceCharNumber += bIsSpace ? 1 : 0);

				// Preparation du caractere suivant
				bEnd = (cChar == '\0');
				if (bEnd)
					sStringValue = NULL;
				else
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
					sToken += cChar;
				}
				// Sinon, fin du token
				else
				{
					// On traite les tokens non vides
					if (sToken.GetLength() > 0)
					{
						debug(lCheckedTotalTokenCharNumber += sToken.GetLength());

						// Incrementation du compte lie au token
						sAttributeKey = Symbol(sToken);
						if (cast(KWIndexedCKeyBlock*, indexedKeyBlock)
							->IsKeyPresent(sAttributeKey))
							cvdTokenCounts.UpgradeAt(sAttributeKey, 1);
						debug(assert(GetSpaceCharNumber(sToken) == 0));
						debug(assert(GetPunctuationCharNumber(sToken) == 0 or
							     GetPunctuationCharNumber(sToken) == sToken.GetLength()));

						// Reinitialisation du token, sans desallouer sa memoire
						sToken.GetBufferSetLength(0);

						// Initialisation du token suivant si necessaire
						if (not bIsSpace and not bEnd)
						{
							sToken += cChar;
							bIsTokenPunctuation = bIsPunctuation;
						}
					}
					bEndToken = false;
				}
			}
		}

		// Construction du bloc de compte
		cvbTokenCounts = KWContinuousValueBlock::BuildBlockFromValueDictionary(
		    cast(KWIndexedCKeyBlock*, indexedKeyBlock), &cvdTokenCounts);
	}
	debug(assert(lCheckedTotalCharNumber == lCheckedTotalSpaceCharNumber + lCheckedTotalTokenCharNumber));
	ensure(cvbTokenCounts->SearchValueIndex(0) == -1);
	return cvbTokenCounts;
}

int KWDRTextTokens::GetSpaceCharNumber(const ALString& sValue) const
{
	int nNumber;
	int i;

	nNumber = 0;
	for (i = 0; i < sValue.GetLength(); i++)
	{
		if (isspace(sValue.GetAt(i)))
			nNumber++;
	}
	return nNumber;
}

int KWDRTextTokens::GetPunctuationCharNumber(const ALString& sValue) const
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

////////////////////////////////////////////////////////////////////////////

KWDRTextListTokens::KWDRTextListTokens()
{
	// Specialisation de la classe mere
	SetName("TextListTokens");
	SetLabel("Token counts from a text list value");
	GetFirstOperand()->SetType(KWType::TextList);
}

KWDRTextListTokens::~KWDRTextListTokens() {}

KWDerivationRule* KWDRTextListTokens::Create() const
{
	return new KWDRTextListTokens;
}

KWContinuousValueBlock*
KWDRTextListTokens::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
						      const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	SymbolVector* svInputValue;

	require(IsCompiled());
	require(indexedKeyBlock != NULL);

	svInputValue = GetFirstOperand()->GetTextListValue(kwoObject);
	cvbTokenCounts = ComputeTokenCountsFromTextList(svInputValue, indexedKeyBlock);
	return cvbTokenCounts;
}

////////////////////////////////////////////////////////////////////////////

IntVector KWDRTextAllNGrams::ivNGramLengths;
IntVector KWDRTextAllNGrams::ivHashTableSizes;
IntVector KWDRTextAllNGrams::ivHashTableCumulatedSizes;
LongintVector KWDRTextAllNGrams::lvNGramMasks;

KWDRTextAllNGrams::KWDRTextAllNGrams()
{
	SetName("TextAllNGrams");
	SetLabel("All n-gram counts from a text value");
	SetType(KWType::ContinuousValueBlock);
	SetOperandNumber(1);

	// Premier operande de type texte
	GetFirstOperand()->SetType(KWType::Text);

	// Parametres d'optimisation de la regle
	nMaxVarKeyIndex = 0;
	nUsedHashTableNumber = 0;
	nDynamicCompileFreshness = 0;

	// Initialisation des variables globales
	InitializeGlobalVariables();
}

KWDRTextAllNGrams::~KWDRTextAllNGrams() {}

int KWDRTextAllNGrams::GetVarKeyType() const
{
	return KWType::Continuous;
}

KWDerivationRule* KWDRTextAllNGrams::Create() const
{
	return new KWDRTextAllNGrams;
}

KWContinuousValueBlock*
KWDRTextAllNGrams::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
						     const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	Symbol sInputValue;

	require(IsCompiled());
	require(indexedKeyBlock != NULL);

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Calcul du resultat
	sInputValue = GetFirstOperand()->GetTextValue(kwoObject);
	cvbTokenCounts = ComputeAllTablesCharNgramCountsFromText(sInputValue, indexedKeyBlock);
	return cvbTokenCounts;
}

Continuous KWDRTextAllNGrams::GetValueBlockContinuousDefaultValue() const
{
	return 0;
}

boolean KWDRTextAllNGrams::CheckBlockAttributes(const KWClass* kwcOwnerClass,
						const KWAttributeBlock* attributeBlock) const
{
	boolean bOk;
	KWAttribute* checkedAttribute;
	int nVarKey;
	ALString sTmp;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(attributeBlock != NULL);
	require(attributeBlock->GetFirstAttribute()->GetParentClass()->GetDomain() == kwcOwnerClass->GetDomain());
	require(CheckOperandsCompleteness(kwcOwnerClass));
	require(attributeBlock->GetVarKeyType() == KWType::Continuous);

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckBlockAttributes(kwcOwnerClass, attributeBlock);

	// Analyse des cles du bloc d'attribut par rapport a la partition
	if (bOk)
	{
		// Parcours des attributs du bloc pour verifier leur VarKey par rapport a la taille de la partition
		checkedAttribute = attributeBlock->GetFirstAttribute();
		while (checkedAttribute != NULL)
		{
			// VarKey de l'attribut
			nVarKey = attributeBlock->GetContinuousVarKey(checkedAttribute);
			assert(nVarKey >= 1);

			// Erreur si VarKey trop grand
			if (nVarKey > GetMaxVarKey())
			{
				attributeBlock->AddError(sTmp + "Variable " + checkedAttribute->GetName() +
							 " has its VarKey=" + IntToString(nVarKey) +
							 " greater than the maximum possible (" +
							 IntToString(GetMaxVarKey()) +
							 ") in a variable block obtained using rule " + GetName());
				bOk = false;
				break;
			}

			// Arret si derniere variable du bloc trouvee
			if (checkedAttribute == attributeBlock->GetLastAttribute())
				break;
			// Sinon, attribut suivant
			else
				checkedAttribute->GetParentClass()->GetNextAttribute(checkedAttribute);
		}
	}
	return bOk;
}

longint KWDRTextAllNGrams::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KWDRTextAllNGrams);
	lUsedMemory += lvSparseCounts.GetUsedMemory() - sizeof(LongintVector);
	return lUsedMemory;
}

int KWDRTextAllNGrams::GetMaxHashTableNumber()
{
	return ivNGramLengths.GetSize();
}

int KWDRTextAllNGrams::GetHashTableNGramLengthAt(int nIndex)
{
	return ivNGramLengths.GetAt(nIndex);
}

int KWDRTextAllNGrams::GetHashTableSizeAt(int nIndex)
{
	return ivHashTableSizes.GetAt(nIndex);
}

int KWDRTextAllNGrams::GetMaxVarKey()
{
	require(AreGlobalVariablesInitialized());
	return ivHashTableCumulatedSizes.GetAt(ivHashTableCumulatedSizes.GetSize() - 1);
}

void KWDRTextAllNGrams::Test()
{
	const int nBlockSize = 100000;
	KWDRTextAllNGrams charNgramCounts;
	KWIndexedNKeyBlock indexedKeyBlock;
	int i;

	// Initialisation des ngrammes a compter
	for (i = 1; i <= nBlockSize; i++)
		indexedKeyBlock.AddKey(i);
	cout << "NGrams: " << indexedKeyBlock << endl;

	// Test avec plusieurs chaines
	indexedKeyBlock.IndexKeys();
	charNgramCounts.TestCount("", &indexedKeyBlock);
	charNgramCounts.TestCount("le", &indexedKeyBlock);
	charNgramCounts.TestCount("bonjour", &indexedKeyBlock);
	charNgramCounts.TestCount("le monde est grand", &indexedKeyBlock);
	charNgramCounts.TestCount("coucou", &indexedKeyBlock);
	charNgramCounts.TestCount("le soir et le matin", &indexedKeyBlock);
}

void KWDRTextAllNGrams::TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock)
{
	KWContinuousValueBlock* valueBlock;

	cout << sInputValue << "\n";
	valueBlock = ComputeAllTablesCharNgramCountsFromText(Symbol(sInputValue), indexedKeyBlock);
	cout << "\t" << *valueBlock << endl;
	delete valueBlock;
}

void KWDRTextAllNGrams::DynamicCompile(const KWIndexedKeyBlock* indexedKeyBlock) const
{
	const KWIndexedNKeyBlock* indexedNKeyBlock;
	int nCumulatedSize;
	int nVarKey;
	int n;

	require(indexedKeyBlock != NULL);
	require(Check());
	require(IsCompiled());
	require(KWType::IsValueBlock(GetType()));

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness < nCompileFreshness)
	{
		// Reinitialisation des parametre a optimiser
		nMaxVarKeyIndex = 0;
		nUsedHashTableNumber = 0;

		// On prend les bons types de blocs de cles
		indexedNKeyBlock = cast(const KWIndexedNKeyBlock*, indexedKeyBlock);

		// Parcours des cles du bloc source
		nMaxVarKeyIndex = 0;
		for (n = 0; n < indexedNKeyBlock->GetKeyNumber(); n++)
		{
			nVarKey = indexedNKeyBlock->GetKeyAt(n);
			nMaxVarKeyIndex = max(nMaxVarKeyIndex, nVarKey);
		}

		// Calcul des caracteristiques specifiques a la regle
		// On pourrait optimiser en fonction de la valeur de la premiere et de la derniere cle utilisee du bloc,
		// mais c'est laborieux a mettre en place. A envisager plus tard en cas de problemes de performance
		// critiques.
		nUsedHashTableNumber = 0;
		nCumulatedSize = 0;
		while (nCumulatedSize < nMaxVarKeyIndex)
		{
			nCumulatedSize += ivHashTableSizes.GetAt(nUsedHashTableNumber);
			nUsedHashTableNumber++;
		}

		// Reinitialisation du vecteur de travail sparse des comptes
		// Ce dernier sera retaille dynamiquement lors de l'execution de la regle
		lvSparseCounts.SetSize(0);

		// Memorisation de la fraicheur
		nDynamicCompileFreshness = nCompileFreshness;
	}
}

void KWDRTextAllNGrams::InitializeGlobalVariables()
{
	boolean bDisplay = false;
	int nGramLength;
	int nHashTableSize;
	longint lNGramMask;
	int i;

	// Calcul des specifications globales communes a toutes les regles, une fois pour toutes
	if (lvNGramMasks.GetSize() == 0)
	{
		assert(ivNGramLengths.GetSize() == 0);

		///////////////////////////////////////////////////////////////////////
		// Choix d'une serie de tables de hashage de taille croissante par
		// longueur de ngrammes
		// On n'en prend que deux par longueur de n-gramme, pour eviter les collision
		// tout en evitant une redondance effective
		// Les longueurs choisies ont ete ajustee empiriquement pour avoir un bon
		// compromis entre le nombre de variables global et la performance predictive

		// Table de hashage de taille 1 pour les 1-gramme: correspond a la longueur des texte
		ivNGramLengths.Add(1);
		ivHashTableSizes.Add(1);

		// Pour les 1-grammes, il y au plus 256 1-grammes distincts
		// On commence a des tailles de 16 au minimum, par puissance de 2, jusqu'a 128
		nHashTableSize = 16;
		while (nHashTableSize <= 128)
		{
			ivNGramLengths.Add(1);
			ivHashTableSizes.Add(nHashTableSize);
			nHashTableSize *= 2;
		}
		assert(nHashTableSize == 256);

		// Pour les 2-grammes, on prend les trois tailles successives suivantes
		nHashTableSize = 256;
		while (nHashTableSize <= 1024)
		{
			ivNGramLengths.Add(2);
			ivHashTableSizes.Add(nHashTableSize);
			nHashTableSize *= 2;
		}

		// Pour les 3-grammes, on prend les trois tailles successives suivantes
		nHashTableSize = 2048;
		while (nHashTableSize <= 8192)
		{
			ivNGramLengths.Add(3);
			ivHashTableSizes.Add(nHashTableSize);
			nHashTableSize *= 2;
		}

		// Par longeur de n-gramme a partir de 4, on ne prend plus que deux tailles, toujours les memes
		for (nGramLength = 4; nGramLength <= nMaxNGramLength; nGramLength++)
		{
			ivNGramLengths.Add(nGramLength);
			ivHashTableSizes.Add(16384);
			ivNGramLengths.Add(nGramLength);
			ivHashTableSizes.Add(32768);
		}

		///////////////////////////////////////////////////////////////////////
		// Finalisation de l'initialisation

		// Calcul des tailles cumulees des tables de hashages
		ivHashTableCumulatedSizes.Add(0);
		for (i = 0; i < ivNGramLengths.GetSize(); i++)
			ivHashTableCumulatedSizes.Add(
			    ivHashTableCumulatedSizes.GetAt(ivHashTableCumulatedSizes.GetSize() - 1) +
			    ivHashTableSizes.GetAt(i));

		// Calcul des masques par nGramme: on met tous les bits a 1 partout sur la longueur des n-grammes
		lvNGramMasks.SetSize(nMaxNGramLength + 1);
		for (nGramLength = 1; nGramLength <= nMaxNGramLength; nGramLength++)
		{
			lNGramMask = 0;
			for (i = 0; i < nGramLength; i++)
			{
				lNGramMask <<= 8;
				lNGramMask += 255;
			}
			lvNGramMasks.SetAt(nGramLength, lNGramMask);
		}

		// Affichage
		if (bDisplay)
		{
			cout << "N-Gram\tHash table size\n";
			for (i = 0; i < ivNGramLengths.GetSize(); i++)
			{
				cout << ivNGramLengths.GetAt(i) << "\t" << ivHashTableSizes.GetAt(i) << "\t"
				     << ivHashTableCumulatedSizes.GetAt(i + 1) << "\n";
			}
			cout << "N-Gram\tMask\n";
			for (nGramLength = 1; nGramLength <= nMaxNGramLength; nGramLength++)
				cout << nGramLength << "\t" << lvNGramMasks.GetAt(nGramLength) << "\n";
			cout << "Max var key\t" << GetMaxVarKey() << "\n";
		}
	}
	ensure(AreGlobalVariablesInitialized());
}

boolean KWDRTextAllNGrams::AreGlobalVariablesInitialized()
{
	return lvNGramMasks.GetSize() >= 0;
}

KWContinuousValueBlock*
KWDRTextAllNGrams::ComputeAllTablesCharNgramCountsFromText(const Symbol& sValue,
							   const KWIndexedKeyBlock* indexedKeyBlock) const
{
	SymbolVector svValues;

	svValues.Add(sValue);
	return ComputeAllTablesCharNgramCountsFromTextList(&svValues, indexedKeyBlock);
}

KWContinuousValueBlock*
KWDRTextAllNGrams::ComputeAllTablesCharNgramCountsFromTextList(const SymbolVector* svValues,
							       const KWIndexedKeyBlock* indexedKeyBlock) const
{
	const boolean bDisplay = false;
	KWContinuousValueBlock* cvbTokenCounts;
	IntVector ivUsedSparseIndexes;
	int nValueNumber;
	int nValue;
	Symbol sValue;
	const char* sText;
	int nTextLength;
	int nLastNGramLength;
	int nHashTableIndex;
	int nHashTableFirstIndex;
	int nHashTableLastIndex;
	int n;
	int nNGramLength;
	longint lNGramMask;
	longint lNgramValue;
	longint lRandom;
	longint lCukooHash;
	int nNGramStartNKey;
	int nStartNKey;
	int nNKey;
	int nSparseIndex;
	char* sIndex;
	int i;

	require(AreGlobalVariablesInitialized());
	require(indexedKeyBlock != NULL);

	// Nombre de valeurs de la liste
	if (svValues == NULL)
		nValueNumber = 0;
	else
		nValueNumber = svValues->GetSize();

	// Affichage de l'entete
	if (bDisplay)
	{
		cout << GetName() << "\t" << nValueNumber << endl;
		for (i = 0; i < nValueNumber; i++)
		{
			if (i > 3)
			{
				cout << "\t...";
				break;
			}
			cout << "\t" << svValues->GetAt(i);
		}
		cout << "\tLength\tSize\tn\tngram\tindex\thash\tkey\tsparse index\tcount" << endl;
	}

	// Retaillage si necessaire du tableau sparse des parties
	if (lvSparseCounts.GetSize() < indexedKeyBlock->GetKeyNumber())
		lvSparseCounts.SetSize(indexedKeyBlock->GetKeyNumber());

	// Cas d'une vecteur vide ou d'une seule chaine vide
	if (nValueNumber == 0 or (nValueNumber == 1 and svValues->GetAt(0).GetLength() == 0))
	{
		cvbTokenCounts = KWContinuousValueBlock::NewValueBlock(0);
	}
	// Cas general, avec traitement pour chaque valeur du vecteur en entree et cumul sur l'ensemble.
	// Par valeur, on prend les caracteres un a un pour composer un index, en multipliant par 256 (<<8) apres chaque
	// caractere On aurait pu utiliser des memcpy pour recopier directement les caracteres dans l'index (longint)
	// traite comme un tableau de 8 caracteres. Le probleme est que cela depend de l'ordre des octets utilise
	// par le systeme pour stocker les entiers (endianness), ce qui rendrait la solution non portable
	// (windows et linux sont en little endian, mais ARM et android pas forcement)
	else
	{
		for (nValue = 0; nValue < svValues->GetSize(); nValue++)
		{
			// Acces a la valeur
			sValue = svValues->GetAt(nValue);
			sText = sValue.GetValue();
			nTextLength = sValue.GetLength();

			// On boucle sur les longueurs de n-grammes
			nHashTableLastIndex = -1;
			nLastNGramLength = 0;
			if (nUsedHashTableNumber > 0)
				nLastNGramLength = ivNGramLengths.GetAt(nUsedHashTableNumber - 1);
			for (nNGramLength = 1; nNGramLength <= nLastNGramLength; nNGramLength++)
			{
				// Arret si texte trop court
				if (nTextLength < nNGramLength)
					break;

				// Calcul des index de tables de hashage concernees
				nHashTableFirstIndex = nHashTableLastIndex + 1;
				nHashTableLastIndex = nHashTableFirstIndex;
				while (nHashTableLastIndex + 1 < nUsedHashTableNumber and
				       ivNGramLengths.GetAt(nHashTableLastIndex + 1) == nNGramLength)
					nHashTableLastIndex++;

				// Verifications
				assert(ivNGramLengths.GetAt(nHashTableFirstIndex) == nNGramLength);
				assert(nHashTableFirstIndex == 0 or
				       ivNGramLengths.GetAt(nHashTableFirstIndex - 1) < nNGramLength);
				assert(ivNGramLengths.GetAt(nHashTableLastIndex) == nNGramLength);

				// Cle de depart pour les tables pour cette longueur de ngramms
				nNGramStartNKey = 1 + ivHashTableCumulatedSizes.GetAt(nHashTableFirstIndex);

				// Analyse du texte pour une longueur de n-grammes donnee
				lNGramMask = lvNGramMasks.GetAt(nNGramLength);

				// Initialisation du debut premier n-gramme
				lNgramValue = 0;
				for (n = 0; n < nNGramLength - 1; n++)
				{
					// On multiplie l'index courant par 256
					lNgramValue <<= 8;

					// On rajoute la valeur du caractere courant, sauf apres la fin de chaine
					lNgramValue += (unsigned char)sText[n];
				}

				// Parcours des n-grammes jusqu'a la fin du texte
				for (n = nNGramLength - 1; n < nTextLength; n++)
				{
					// On multiplie l'index courant par 256
					lNgramValue <<= 8;

					// On rajoute la valeur du caractere courant, sauf apres la fin de chaine
					lNgramValue += (unsigned char)sText[n];

					// On tronque a la longueur du pattern en appliquant le masque
					lNgramValue &= lNGramMask;
					assert(lNgramValue >= 0);

					// On prend un nombre aleatoire a partir de la valeur de ngramme
					lRandom = IthRandomLongint(lNgramValue);

					// Alimentation des tables de hashage pour cette longueur de ngramme
					lCukooHash = lRandom;
					nStartNKey = nNGramStartNKey;
					for (nHashTableIndex = nHashTableFirstIndex;
					     nHashTableIndex <= nHashTableLastIndex; nHashTableIndex++)
					{
						// Calcul d'une cle de n-gram projetee aleatoirement sur sa table a
						// partir de nStartNKey
						nNKey = abs(lCukooHash) % ivHashTableSizes.GetAt(nHashTableIndex);
						nNKey += nStartNKey;

						// Recherche de l'index sparse correspondant
						nSparseIndex =
						    cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nNKey);

						// Test s'il faut memoriser le compte du n-gramme
						if (nSparseIndex != -1)
						{
							assert(nSparseIndex < lvSparseCounts.GetSize());

							// Memorisation si necessaire de l'index sparse utilise
							if (lvSparseCounts.GetAt(nSparseIndex) == 0)
								ivUsedSparseIndexes.Add(nSparseIndex);

							// Mise a jour du compte du ngramme
							lvSparseCounts.UpgradeAt(nSparseIndex, 1);
						}

						// Affichage des resultats intermediaires, pour la mise au point
						if (bDisplay)
						{
							cout << "\t" << nNGramLength << "\t"
							     << ivHashTableSizes.GetAt(nHashTableIndex) << "\t" << n
							     << "\t";

							// Affichage du n-gramme collecte dans la valeur lNgramValue
							sIndex = (char*)&lNgramValue;
							for (i = 7; i >= 0; i--)
							{
								if (sIndex[i] != 0)
									cout << sIndex[i];
							}

							// Affichage des variables de calcul
							cout << "\t" << lNgramValue << "\t#" << lCukooHash << "\t"
							     << nNKey << "\t" << nSparseIndex << "\t";
							if (nSparseIndex != -1)
								cout << lvSparseCounts.GetAt(nSparseIndex);
							cout << "\n";
						}

						// Position de depart pour la table suivante
						nStartNKey += ivHashTableSizes.GetAt(nHashTableIndex);

						// Variante de la valeur hashage pour la table suivante, pour eviter les
						// collisions
						lCukooHash += lNgramValue;
					}
				}
			}
		}

		// Tri des index sparses des parties utilisees
		ivUsedSparseIndexes.Sort();

		// Construction du bloc de compte de la bonne taille
		cvbTokenCounts = KWContinuousValueBlock::NewValueBlock(ivUsedSparseIndexes.GetSize());

		// Initialisation du vecteur sparse
		// On exploite ici le fait que les index sparse sont necessairement ordonnes de la
		// meme facon que les index de parties (NKey)
		for (n = 0; n < ivUsedSparseIndexes.GetSize(); n++)
		{
			nSparseIndex = ivUsedSparseIndexes.GetAt(n);

			// Memorisation de la paire (index sparse, compte)
			cvbTokenCounts->SetAttributeSparseIndexAt(n, nSparseIndex);
			cvbTokenCounts->SetValueAt(n, (Continuous)lvSparseCounts.GetAt(nSparseIndex));
			assert(n == 0 or cvbTokenCounts->GetAttributeSparseIndexAt(n) >
					     cvbTokenCounts->GetAttributeSparseIndexAt(n - 1));

			// Reinitialisation du vecteur de travail
			lvSparseCounts.SetAt(nSparseIndex, 0);
		}
	}

	// Affichage du resultat
	if (bDisplay)
	{
		cout << "Result block:\n";
		for (n = 0; n < cvbTokenCounts->GetValueNumber(); n++)
		{
			cout << "\t " << cvbTokenCounts->GetAttributeSparseIndexAt(n) << "\t";
			cout << KWContinuous::ContinuousToString(cvbTokenCounts->GetValueAt(n)) << "\n";
			if (n > 1000)
			{
				cout << "\t...\n";
				break;
			}
		}
		cout << "\n";
	}
	ensure(cvbTokenCounts->SearchValueIndex(0) == -1);
	ensure(CheckCharNgramCountsValueBlockFromTextList(svValues, indexedKeyBlock, cvbTokenCounts));
	return cvbTokenCounts;
}

boolean KWDRTextAllNGrams::CheckCharNgramCountsValueBlockFromTextList(const SymbolVector* svValues,
								      const KWIndexedKeyBlock* indexedKeyBlock,
								      const KWContinuousValueBlock* valueBlock) const
{
	boolean bOk = true;
	int nValueNumber;
	int nValue;
	Symbol sValue;
	int nTextLength;
	int nMaxTextLength;
	boolean bIsFullBlock;
	LongintVector lvCumulatedNGramCounts;
	LongintVector lvExpectedNGramCounts;
	int i;
	int nSparseIndex;
	int nNKey;
	int nHashTableIndex;
	int nNGramLength;
	int nHashTableCumulatedSize;

	require(AreGlobalVariablesInitialized());
	require(indexedKeyBlock != NULL);
	require(valueBlock != NULL);

	// Nombre de valeurs de la liste
	if (svValues == NULL)
		nValueNumber = 0;
	else
		nValueNumber = svValues->GetSize();

	// Cas d'une vecteur vide ou d'une seule chaine vide
	if (nValueNumber == 0 or (nValueNumber == 1 and svValues->GetAt(0).GetLength() == 0))
		bOk = valueBlock->GetValueNumber() == 0;
	// Cas general
	else
	{
		// Initialisation d'un vecteur de comptes de n-grammes par taille de table hashage
		lvCumulatedNGramCounts.SetSize(nUsedHashTableNumber);
		lvExpectedNGramCounts.SetSize(nUsedHashTableNumber);

		// Test si le block est plein
		assert(indexedKeyBlock->GetKeyNumber() >= nMaxVarKeyIndex);
		bIsFullBlock = (indexedKeyBlock->GetKeyNumber() == nMaxVarKeyIndex);
		assert(not bIsFullBlock or cast(KWIndexedNKeyBlock*, indexedKeyBlock)->IsKeyPresent(1));
		assert(not bIsFullBlock or cast(KWIndexedNKeyBlock*, indexedKeyBlock)->IsKeyPresent(nMaxVarKeyIndex));

		// Boucle sur les valeurs du vecteur pour determiner les comptes cumules attendus
		nMaxTextLength = 0;
		for (nValue = 0; nValue < svValues->GetSize(); nValue++)
		{
			// Acces a la valeur
			sValue = svValues->GetAt(nValue);
			nTextLength = sValue.GetLength();
			nMaxTextLength = max(nMaxTextLength, nTextLength);

			// Mise a jour des compte totaux attendus sur l'ensemble des valeurs
			for (nHashTableIndex = 0; nHashTableIndex < nUsedHashTableNumber; nHashTableIndex++)
			{
				nNGramLength = ivNGramLengths.GetAt(nHashTableIndex);

				// Cas de texte de longueur inferieure aux n-grammes
				if (nTextLength < nNGramLength)
					lvExpectedNGramCounts.UpgradeAt(nHashTableIndex, 0);
				// Cas general
				else
					lvExpectedNGramCounts.UpgradeAt(nHashTableIndex,
									nTextLength + 1 - nNGramLength);
			}
		}

		// Comptage du nombre cumule de n-grammes par taille de block
		for (i = 0; i < valueBlock->GetValueNumber(); i++)
		{
			// Acces a la cle de n-gramme
			nSparseIndex = valueBlock->GetAttributeSparseIndexAt(i);
			nNKey = cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyAt(nSparseIndex);

			// Recherche de l'index de la table de hashage correspondante
			nHashTableCumulatedSize = 0;
			nHashTableIndex = -1;
			while (nNKey > nHashTableCumulatedSize)
			{
				nHashTableIndex++;
				nHashTableCumulatedSize += ivHashTableSizes.GetAt(nHashTableIndex);
			}

			// Mise a jour du compte dans cette table de hashage
			lvCumulatedNGramCounts.UpgradeAt(nHashTableIndex, (longint)valueBlock->GetValueAt(i));
		}

		// Verification des comptes cumules de n-grammes
		for (nHashTableIndex = 0; nHashTableIndex < nUsedHashTableNumber; nHashTableIndex++)
		{
			nNGramLength = ivNGramLengths.GetAt(nHashTableIndex);

			// Cas de texte le plus grand de longueur inferieure aux n-grammes
			if (nMaxTextLength < nNGramLength)
				bOk = bOk and lvCumulatedNGramCounts.GetAt(nHashTableIndex) == 0;
			// Cas d'un comptage exhaustif si le block est plein avant la derniere table
			else if (bIsFullBlock and nHashTableIndex < nUsedHashTableNumber - 1)
				bOk = bOk and lvCumulatedNGramCounts.GetAt(nHashTableIndex) ==
						  lvExpectedNGramCounts.GetAt(nHashTableIndex);
			// Cas d'un comptage exhaustif si le block est plein pour la derniere table
			else if (bIsFullBlock and nHashTableIndex == nUsedHashTableNumber - 1 and
				 indexedKeyBlock->GetKeyNumber() ==
				     ivHashTableCumulatedSizes.GetAt(nHashTableIndex + 1))
				bOk = bOk and lvCumulatedNGramCounts.GetAt(nHashTableIndex) ==
						  lvExpectedNGramCounts.GetAt(nHashTableIndex);
			// Cas general
			else
				bOk = bOk and lvCumulatedNGramCounts.GetAt(nHashTableIndex) <=
						  lvExpectedNGramCounts.GetAt(nHashTableIndex);
			assert(bOk);
		}
	}
	return bOk;
}

////////////////////////////////////////////////////////////////////////////

KWDRTextListAllNGrams::KWDRTextListAllNGrams()
{
	// Specialisation de la classe mere
	SetName("TextListAllNGrams");
	SetLabel("All n-gram counts from a text list value");
	GetFirstOperand()->SetType(KWType::TextList);
}

KWDRTextListAllNGrams::~KWDRTextListAllNGrams() {}

KWDerivationRule* KWDRTextListAllNGrams::Create() const
{
	return new KWDRTextListAllNGrams;
}

KWContinuousValueBlock*
KWDRTextListAllNGrams::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
							 const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbTokenCounts;
	SymbolVector* svInputValue;

	require(IsCompiled());
	require(indexedKeyBlock != NULL);

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Calcul du resultat
	svInputValue = GetFirstOperand()->GetTextListValue(kwoObject);
	cvbTokenCounts = ComputeAllTablesCharNgramCountsFromTextList(svInputValue, indexedKeyBlock);
	return cvbTokenCounts;
}

////////////////////////////////////////////////////////////////////////////

KWDRTextInit::KWDRTextInit()
{
	SetName("TextInit");
	SetLabel("Initialization of a text variable");
	SetType(KWType::Text);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetSecondOperand()->SetType(KWType::Symbol);

	// On impose que les deux derniers operandes soient des constantes
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRTextInit::~KWDRTextInit() {}

KWDerivationRule* KWDRTextInit::Create() const
{
	return new KWDRTextInit;
}

Symbol KWDRTextInit::ComputeTextResult(const KWObject* kwoObject) const
{
	ALString sResult;
	int nSize;
	char cValue;

	// Construction d'un chaine de la longueur souhaitee
	nSize = int(floor(GetFirstOperand()->GetContinuousConstant() + 0.5));
	cValue = GetSecondOperand()->GetSymbolConstant().GetAt(0);
	sResult = ALString(cValue, nSize);
	return Symbol(sResult, nSize);
}

boolean KWDRTextInit::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	Continuous cValue;
	int nValue;
	Symbol sValue;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification des operandes
	if (bOk)
	{
		assert(GetOperandNumber() == 2);

		// Taille du texe a generer
		cValue = GetFirstOperand()->GetContinuousConstant();
		bOk = KWContinuous::ContinuousToInt(cValue, nValue);
		if (not bOk)
		{
			AddError(sTmp + "Size of text (" + KWContinuous::ContinuousToString(cValue) +
				 ") in first operand must be an integer");
			assert(bOk == false);
		}
		else if (nValue < 0)
		{
			AddError(sTmp + "Size of text (" + KWContinuous::ContinuousToString(cValue) +
				 ") in first operand must be positive");
			bOk = false;
		}
		else if (nValue > InputBufferedFile::nMaxFieldSize)
		{
			AddError(sTmp + "Size of text (" + KWContinuous::ContinuousToString(cValue) +
				 ") in first operand must less than " + IntToString(InputBufferedFile::nMaxFieldSize));
			bOk = false;
		}

		// Longuer du deuxieme operande, qui doit etre reduit a un seul caractere
		sValue = GetSecondOperand()->GetSymbolConstant();
		if (sValue.GetLength() != 1)
		{
			AddError(sTmp + "Second operand (" + sValue + ") must contain one single character");
			bOk = false;
		}
	}
	return bOk;
}
