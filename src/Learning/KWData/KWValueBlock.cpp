// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWValueBlock.h"

// Inclusion dans le source plutot que le header pour eviter les reference cycliques
#include "KWObject.h"

/////////////////////////////////////////////////////////////////////
// Classe KWValueBlock

void* KWValueBlock::GenericNewValueBlock(int nSize)
{
	KWValueBlock* newValueBlock;
	int nMemorySize;
	void* pValueBlockMemory;
	int nSegmentNumber;
	KWValueIndexPair* valueIndexPairs;
	int i;

	require(nSize >= 0);

	// Cas mono-segment: le bloc entier peut tenir dans un segment memoire
	if (nSize <= nSegmentSize)
	{
		// Calcul de la taille a allouer
		nMemorySize = sizeof(int) + nSize * sizeof(KWValueIndexPair);

		// Creation des donnees d'un bloc
		pValueBlockMemory = NewMemoryBlock(nMemorySize);

		// Initialisation avec des zero
		memset(pValueBlockMemory, 0, nMemorySize * sizeof(char));

		// On caste la memoire allouee pour pouvoir utiliser les methodes de la classe
		newValueBlock = (KWValueBlock*)pValueBlockMemory;
		assert(newValueBlock->GetValueNumber() == 0);

		// Parametrage du nombre de valeurs
		newValueBlock->nValueNumber = nSize;

		// On verifie par assertion que le packing des classe utilise est correct
		assert(sizeof(KWValueIndexPair) == sizeof(KWValue) + sizeof(int));
		assert(&(newValueBlock->cStartBlock) - (char*)newValueBlock == sizeof(int));
	}
	// Cas multi-segment: il faut plusieurs segments pour stocker le bloc
	//  On passe par un tableau de segments
	else
	{
		// Calcul du nombre de segment
		nSegmentNumber = (nSize - 1) / nSegmentSize + 1;

		// Calcul de la taille a allouer
		nMemorySize = sizeof(int) + nSegmentNumber * sizeof(KWValueIndexPair*);

		// Creation des donnees du bloc, avec un tableau de pointeurs vers des blocs
		pValueBlockMemory = NewMemoryBlock(nMemorySize);

		// On caste la memoire allouee pour pouvoir utiliser les methodes de la classe
		newValueBlock = (KWValueBlock*)pValueBlockMemory;

		// Parametrage du nombre de valeurs
		newValueBlock->nValueNumber = nSize;
		assert(newValueBlock->GetValueNumber() == nSize);

		// Creation et initialisation des segments
		for (i = nSegmentNumber - 2; i >= 0; i--)
		{
			valueIndexPairs = (KWValueIndexPair*)NewMemoryBlock(nSegmentSize * sizeof(KWValueIndexPair));
			((KWValueIndexPair**)&(newValueBlock->cStartBlock))[i] = valueIndexPairs;
			memset(valueIndexPairs, 0, nSegmentSize * sizeof(KWValueIndexPair));
		}
		if (nSize % nSegmentSize > 0)
		{
			valueIndexPairs =
			    (KWValueIndexPair*)NewMemoryBlock((nSize % nSegmentSize) * sizeof(KWValueIndexPair));
			((KWValueIndexPair**)&(newValueBlock->cStartBlock))[nSegmentNumber - 1] = valueIndexPairs;
			memset(valueIndexPairs, 0, (nSize % nSegmentSize) * sizeof(KWValueIndexPair));
		}
		else
		{
			valueIndexPairs = (KWValueIndexPair*)NewMemoryBlock(nSegmentSize * sizeof(KWValueIndexPair));
			((KWValueIndexPair**)&(newValueBlock->cStartBlock))[nSegmentNumber - 1] = valueIndexPairs;
			memset(valueIndexPairs, 0, nSegmentSize * sizeof(KWValueIndexPair));
		}
	}
	return pValueBlockMemory;
}

KWValueBlock::~KWValueBlock()
{
	int i;
	int nSegmentNumber;
	KWValueIndexPair* valueIndexPairs;

	// Desallocation des segments de paire (valeur, index) dans le cas de tableaux de grande taille
	if (nValueNumber > nSegmentSize)
	{
		nSegmentNumber = (nValueNumber - 1) / nSegmentSize + 1;
		for (i = nSegmentNumber - 1; i >= 0; i--)
		{
			valueIndexPairs = ((KWValueIndexPair**)&(cStartBlock))[i];
			DeleteMemoryBlock(valueIndexPairs);
		}
	}
}

int KWValueBlock::GetValueIndexAtAttributeSparseIndex(int nAttributeSparseIndex) const
{
	int nLeft;
	int nRight;
	int nMiddle;
	int nValue;

	// Initialisation des index de recherche dichotomique
	nLeft = 0;
	nRight = GetValueNumber() - 1;

	// Recherche dichotomique
	while (nLeft < nRight)
	{
		assert(GetAttributeSparseIndexAt(nLeft) < GetAttributeSparseIndexAt(nRight));
		nMiddle = (nLeft + nRight) / 2;
		nValue = GetAttributeSparseIndexAt(nMiddle);
		if (nValue < nAttributeSparseIndex)
			nLeft = nMiddle + 1;
		else
			nRight = nMiddle;
	}

	// On teste la cle pour l'index final
	if (nRight >= 0 and GetAttributeSparseIndexAt(nLeft) == nAttributeSparseIndex)
		return nLeft;
	else
		return -1;
}

int KWValueBlock::GetSmallestValueIndexBeyondAttributeSparseIndex(int nAttributeSparseIndex) const
{
	int nLeft;
	int nRight;
	int nMiddle;
	int nValue;

	// Initialisation des index de recherche dichotomique
	nLeft = 0;
	nRight = GetValueNumber() - 1;

	// Recherche dichotomique
	while (nLeft < nRight)
	{
		assert(GetAttributeSparseIndexAt(nLeft) < GetAttributeSparseIndexAt(nRight));
		nMiddle = (nLeft + nRight) / 2;
		nValue = GetAttributeSparseIndexAt(nMiddle);
		if (nValue < nAttributeSparseIndex)
			nLeft = nMiddle + 1;
		else
			nRight = nMiddle;
	}

	// On teste la cle pour l'index final
	if (nRight >= 0 and nAttributeSparseIndex <= GetAttributeSparseIndexAt(nLeft))
	{
		assert(nLeft == 0 or GetAttributeSparseIndexAt(nLeft - 1) < nAttributeSparseIndex);
		return nLeft;
	}
	else
		return -1;
}

boolean KWValueBlock::Check() const
{
	boolean bOk;
	ALString sMessage;
	bOk = SilentCheck(sMessage);
	if (not bOk)
		Global::AddError(GetClassLabel(), "", sMessage);
	return bOk;
}

void KWValueBlock::CheckPacking()
{
	int nPackedSize;

	nPackedSize = sizeof(KWValue) + sizeof(int);
	if (sizeof(KWValueIndexPair) != nPackedSize)
		Global::AddFatalError("", "", "Compiler packing error for structure KWValueIndexPair");
}

boolean KWValueBlock::SilentCheck(ALString& sMessage) const
{
	int i;
	ALString sTmp;

	// On verifie que les index des attributs sparse sont ordonnees et uniques
	for (i = 1; i < GetValueNumber(); i++)
	{
		if (GetAttributeSparseIndexAt(i - 1) >= GetAttributeSparseIndexAt(i))
		{
			// Cas d'egalite
			if (GetAttributeSparseIndexAt(i - 1) == GetAttributeSparseIndexAt(i))
			{
				sMessage = sTmp + "Internal sparse variable index " +
					   IntToString(GetAttributeSparseIndexAt(i)) + " is used twice";
				return false;
			}
			// Cas de mauvais ordre
			else
			{
				sMessage = sMessage + "Internal sparse variable index " +
					   IntToString(GetAttributeSparseIndexAt(i)) +
					   " should be greater than the preceding index " +
					   IntToString(GetAttributeSparseIndexAt(i - 1));
				return false;
			}
		}
	}
	return true;
}

const ALString KWValueBlock::GetClassLabel() const
{
	return "Sparse value block";
}

void KWValueBlock::Test()
{
	StringVector svInputFields;
	ALString sInputField;
	int i;
	ALString sKey;
	ALString sValue;
	Continuous cValue;
	int nOffset;
	int nFieldError;
	int nError;
	boolean bOk;

	// Liste des chaines a tester
	svInputFields.Add("toto");
	svInputFields.Add("toto:");
	svInputFields.Add("toto:1");
	svInputFields.Add("toto:1:");
	svInputFields.Add("toto:3.1416");
	svInputFields.Add("toto:1e-200");
	svInputFields.Add("toto:1E+200");
	svInputFields.Add("toto:24E");
	svInputFields.Add("toto:24A");
	svInputFields.Add("toto ");
	svInputFields.Add("'toto':1");
	svInputFields.Add("'to''to':1");
	svInputFields.Add("'to'to':1");
	svInputFields.Add("'to to':1");
	svInputFields.Add("'toto");
	svInputFields.Add("to&to:1");
	svInputFields.Add("'to&to':1");

	// Lecture d'une paire (cle:valeur numerique)
	for (i = 0; i < svInputFields.GetSize(); i++)
	{
		sInputField = svInputFields.GetAt(i);

		// Lecture de la cle
		cout << "ReadCKey(" << sInputField << ") -> ";
		nOffset = 0;
		nFieldError = FieldNoError;
		bOk = ReadCKey(sInputField, nOffset, sKey, nFieldError);
		if (not bOk)
			cout << "KO,  last read key=<" << sKey << ">\n\tbefore field offset " << nOffset << "\n\t"
			     << GetFieldErrorLabel(nFieldError);

		// Lecture de la valeur
		cValue = KWContinuous::GetMissingValue();
		if (bOk and nOffset < sInputField.GetLength())
		{
			if (sInputField[nOffset] == ':')
			{
				nOffset++;
				bOk = ReadContinuousValue(sInputField, nOffset, sValue, nFieldError);
				if (not bOk)
					cout << "KO,  last read numerical value=<" << sValue
					     << ">\n\tbefore field offset " << nOffset << "\n\t"
					     << GetFieldErrorLabel(nFieldError);
				else
				{
					nError = KWContinuous::StringToContinuousError(sValue, cValue);
					bOk = (nError == KWContinuous::NoError);
					if (not bOk)
						cout << "KO,  last read numerical value=<" << sValue
						     << ">\n\tbefore field offset " << nOffset << "\n\t"
						     << KWContinuous::ErrorLabel(nError);
				}
			}
		}

		// Affichage du resultat
		if (bOk)
			cout << "<" << sKey << ">:" << KWContinuous::ContinuousToString(cValue);
		cout << endl;
	}
}

void KWValueBlock::InitializeKeyIndexArray(const KWIndexedCKeyBlock* indexedKeyBlock,
					   const KWValueDictionary* valueDictionary, ObjectArray* oaKeyIndexValues)
{
	KWKeyIndex* keyIndexValue;
	int nKeyIndex;
	int i;
	int nFoundIndexNumber;

	require(indexedKeyBlock != NULL);
	require(valueDictionary != NULL);
	require(oaKeyIndexValues != NULL);

	// Export des objets internes du dictionnaire de paires (cle, valeur) dans un tableau
	valueDictionary->ExportKeyIndexValues(oaKeyIndexValues);

	// Parcours de toutes les paires (cle, valeur) du dictionnaire de valeurs, pour memoriser
	// l'index associe a chaque cle
	// On ne garde que les paires correspondant a une cle existante
	nFoundIndexNumber = 0;
	for (i = 0; i < oaKeyIndexValues->GetSize(); i++)
	{
		keyIndexValue = cast(KWKeyIndex*, oaKeyIndexValues->GetAt(i));

		// Recherche de l'index de cle correspondant
		nKeyIndex = indexedKeyBlock->GetKeyIndex(keyIndexValue->GetKey());

		// Memorisation uniquement si cle existante
		if (nKeyIndex >= 0)
		{
			keyIndexValue->SetIndex(nKeyIndex);

			// On range dans l'emplacement en fonction du nombre de cle trouvees
			oaKeyIndexValues->SetAt(nFoundIndexNumber, keyIndexValue);
			nFoundIndexNumber++;
		}
	}

	// Retaillage du tableau en fonction du nombre de valeurs trouvees
	oaKeyIndexValues->SetSize(nFoundIndexNumber);

	// Tri des paires (cle, valeur) par index croissant
	oaKeyIndexValues->SetCompareFunction(KWKeyIndexCompareIndex);
	oaKeyIndexValues->Sort();
	ensure(oaKeyIndexValues->GetSize() <= valueDictionary->GetCount());
}

const ALString KWValueBlock::GetFieldErrorLabel(int nFieldError)
{
	require(FieldNoError < nFieldError and nFieldError < FieldEnd);

	if (nFieldError == FieldKeyEmpty)
		return "empty key";
	else if (nFieldError == FieldIntKeyStartWithZero)
		return "integer key must not start with digit 0";
	else if (nFieldError == FieldIntKeyTooLarge)
		return "integer key must be less or equal than 1000000000";
	else if (nFieldError == FieldMiddleQuote)
		return "quote in the middle of the field should be paired";
	else if (nFieldError == FieldMissingEndQuote)
		return "missing quote at the end of the field";
	else if (nFieldError == FieldWrongIntChar)
		return "integer value containing wrong chars";
	else if (nFieldError == FieldWrongContinuousChar)
		return "numerical value containing wrong chars";
	else if (nFieldError == FieldWrongChar)
		return "field containing non alphanumeric chars should be between quotes";
	else if (nFieldError == FieldMissingBlank)
		return "blank separator is missing";
	else if (nFieldError == FieldHeadBlank)
		return "blank separator at the head of the field";
	else if (nFieldError == FieldTailBlank)
		return "blank separator at the tail of the field";
	else if (nFieldError == FieldDoubledBlank)
		return "blank separator used more than once";
	else
	{
		assert(false);
		return "";
	}
}

boolean KWValueBlock::ReadBlankSeparator(const char* sInputField, int& nCurrentOffset, int& nFieldError)
{
	require(sInputField != NULL);
	require(nCurrentOffset >= 0);
	require(sInputField[nCurrentOffset] != '\0');

	// Erreur si caractere blanc sauf au tout debut
	if (nCurrentOffset == 0)
	{
		if (sInputField[nCurrentOffset] == ' ')
		{
			nCurrentOffset++;
			nFieldError = FieldHeadBlank;
			return false;
		}
	}
	// Tentative de lecture d'un caractere blanc sauf au tout debut
	else
	{
		// Le caractere doit etre un caractere blanc
		if (sInputField[nCurrentOffset] == ' ')
			nCurrentOffset++;
		// Erreur sinon
		else
		{
			nFieldError = FieldMissingBlank;
			return false;
		}

		// Il ne doit pas y avoir plusieurs caracteres blancs de suite
		if (sInputField[nCurrentOffset] == ' ')
		{
			nCurrentOffset++;
			nFieldError = FieldDoubledBlank;
			return false;
		}
		// Il ne doit pas y avoir avoir de caracteres blancs suivi de rien
		else if (sInputField[nCurrentOffset] == '\0')
		{
			nFieldError = FieldTailBlank;
			return false;
		}
	}

	// Retour quand tout s'est bien passe
	nFieldError = FieldNoError;
	return true;
}

boolean KWValueBlock::ReadCKey(const char* sInputField, int& nCurrentOffset, ALString& sKey, int& nFieldError)
{
	boolean bOk;

	require(sInputField != NULL);
	require(nCurrentOffset >= 0);

	// On tente de lire une cle
	bOk = ReadSymbolValue(sInputField, nCurrentOffset, sKey, nFieldError);
	if (bOk and sKey.GetLength() == 0)
	{
		bOk = false;
		nFieldError = FieldKeyEmpty;
	}
	return bOk;
}

boolean KWValueBlock::ReadNKey(const char* sInputField, int& nCurrentOffset, ALString& sKey, int& nKey,
			       int& nFieldError)
{
	boolean bOk;

	require(sInputField != NULL);
	require(nCurrentOffset >= 0);

	// On tente de lire un entier
	bOk = ReadIntValue(sInputField, nCurrentOffset, sKey, nFieldError);

	// Test si non vide
	if (bOk and sKey.GetLength() == 0)
	{
		bOk = false;
		nFieldError = FieldKeyEmpty;
	}

	// Test si la cle commence par 0
	if (bOk and sKey.GetAt(0) == '0')
	{
		bOk = false;
		nFieldError = FieldIntKeyStartWithZero;
	}

	// Test si la cle est trop grande
	if (bOk and sKey.GetLength() > 10)
	{
		bOk = false;
		nFieldError = FieldIntKeyTooLarge;
	}

	// Conversion en entier
	nKey = 0;
	if (bOk)
	{
		nKey = StringToInt(sKey);
		assert(nKey >= 1);

		// Test si cle trop grande
		if (nKey > 1000000000)
		{
			bOk = false;
			nFieldError = FieldIntKeyTooLarge;
		}
	}
	return bOk;
}

boolean KWValueBlock::ReadIntValue(const char* sInputField, int& nCurrentOffset, ALString& sValue, int& nFieldError)
{
	char c;

	require(sInputField != NULL);
	require(nCurrentOffset >= 0);

	// Reinitialisation de la taille de la chaine en sortie, sans la reallouer
	sValue.GetBufferSetLength(0);

	// Lecture dans le cas avec un caractere d'echappement sur la cle
	nFieldError = FieldNoError;
	c = sInputField[nCurrentOffset];
	while (c != '\0')
	{
		// Memorisation si le caractere est un chiffre
		if (isdigit(c))
			sValue += c;
		// Test de fin de champ sinon
		else if (c == ' ' or c == ':')
			return true;
		// Erreur sinon
		else
		{
			nFieldError = FieldWrongIntChar;
			return false;
		}

		// Lecture du caractere suivant
		nCurrentOffset++;
		c = sInputField[nCurrentOffset];
	}
	return true;
}

boolean KWValueBlock::ReadContinuousValue(const char* sInputField, int& nCurrentOffset, ALString& sValue,
					  int& nFieldError)
{
	char c;

	require(sInputField != NULL);
	require(nCurrentOffset >= 0);

	// Reinitialisation de la taille de la chaine en sortie, sans la reallouer
	sValue.GetBufferSetLength(0);

	// Lecture dans le cas avec un caractere d'echappement sur la cle
	nFieldError = FieldNoError;
	c = sInputField[nCurrentOffset];
	while (c != '\0')
	{
		// Memorisation si le caractere est numerique
		if (isdigit(c) or c == '.' or c == '+' or c == '-' or c == 'e' or c == 'E')
			sValue += c;
		// Test de fin de champ sinon
		else if (c == ' ' or c == ':')
			return true;
		// Erreur sinon
		else
		{
			nFieldError = FieldWrongContinuousChar;
			return false;
		}

		// Lecture du caractere suivant
		nCurrentOffset++;
		c = sInputField[nCurrentOffset];
	}
	return true;
}

boolean KWValueBlock::ReadSymbolValue(const char* sInputField, int& nCurrentOffset, ALString& sValue, int& nFieldError)
{
	char c;

	require(sInputField != NULL);
	require(nCurrentOffset >= 0);

	// Reinitialisation de la taille de la chaine en sortie, sans la reallouer
	sValue.GetBufferSetLength(0);

	// Lecture dans le cas avec un caractere d'echappement sur la cle
	nFieldError = FieldNoError;
	c = sInputField[nCurrentOffset];
	if (c == '\'')
	{
		while (c != '\0')
		{
			// Lecture du caractere suivant
			nCurrentOffset++;
			c = sInputField[nCurrentOffset];

			// Memorisation si ce n'est pas le caractere d'echappement
			if (c != '\'')
				sValue += c;
			// Traitement du caractere d'echappement sinon
			else
			{
				// Lecture du caractere suivant
				nCurrentOffset++;
				c = sInputField[nCurrentOffset];

				// Memorisation d'un simple-quote interne s'il est double
				if (c == '\'')
					sValue += '\'';
				// Test de fin de champ sinon
				else if (c == ' ' or c == ':' or c == '\0')
				{
					// On supprime les blancs en debut et fin
					sValue.TrimLeft();
					sValue.TrimRight();
					return true;
				}
				// Erreur sinon
				else
				{
					nFieldError = FieldMiddleQuote;
					return false;
				}
			}
		}
		// Erreur parce l'on a pas rencontre de caractere d'echappement
		nFieldError = FieldMissingEndQuote;
		return false;
	}
	// Lecture dans le cas standard
	else
	{
		while (c != '\0')
		{
			// Memorisation si le caractere est alpha-numerique
			if (c >= -1 and isalnum(c)) // selon la plage de caractere exige par isalnum
				sValue += c;
			// Arret si caractere de fin (sans le caractere de fin)
			else if (c == ' ' or c == ':')
				return true;
			// Erreur sinon
			else
			{
				nFieldError = FieldWrongChar;
				return false;
			}

			// Lecture du caractere suivant
			nCurrentOffset++;
			c = sInputField[nCurrentOffset];
		}
		return true;
	}
}

const ALString KWValueBlock::BuildErrorMessage(const ALString& sError, const ALString& sCurrentKey,
					       const char* sInputField, int nCurrentOffset)
{
	ALString sErrorMessage;
	int nFirstOffset;
	int i;

	require(sError != "");
	require(sInputField != NULL);
	require(nCurrentOffset >= 0);

	// Initialisation
	sErrorMessage = sError;
	sErrorMessage += " (";

	// Precision de la cle si presente
	if (sCurrentKey != "")
	{
		sErrorMessage += "Var key=<";
		sErrorMessage += sCurrentKey;
		sErrorMessage += ">, ";
	}

	// Position dans le champ
	sErrorMessage += "field at ";
	sErrorMessage += IntToString(nCurrentOffset);

	// Valeur du champ autour de sa position
	sErrorMessage += ": \"";
	nFirstOffset = nCurrentOffset - 15;
	if (nFirstOffset <= 0)
		nFirstOffset = 0;
	else
		sErrorMessage += "...";
	for (i = nFirstOffset; i < nCurrentOffset; i++)
		sErrorMessage += sInputField[i];
	if (sInputField[nCurrentOffset] != '\0')
		sErrorMessage += sInputField[nCurrentOffset];
	sErrorMessage += "\")";
	return sErrorMessage;
}

void KWValueBlock::WriteKey(ALString& sOutputField, const KWIndexedKeyBlock* indexedKeyBlock, int nAttributeSparseIndex)
{
	if (indexedKeyBlock->GetVarKeyType() == KWType::Symbol)
		WriteSymbolValue(sOutputField,
				 cast(KWIndexedCKeyBlock*, indexedKeyBlock)->GetKeyAt(nAttributeSparseIndex));
	else
		WriteIntValue(sOutputField,
			      cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyAt(nAttributeSparseIndex));
}

void KWValueBlock::WriteIntValue(ALString& sOutputField, int nValue)
{
	sOutputField += IntToString(nValue);
}

void KWValueBlock::WriteContinuousValue(ALString& sOutputField, Continuous cValue)
{
	sOutputField += KWContinuous::ContinuousToString(cValue);
}

void KWValueBlock::WriteSymbolValue(ALString& sOutputField, const Symbol& sValue)
{
	boolean bWithQuote;
	const char* sStringValue;
	int nOffset;
	char c;

	// On determine s'il y a de caracteres non alpha-numerique dans la valeur
	// pour savoir s'il faut l'entourer de quotes
	bWithQuote = false;
	sStringValue = sValue.GetValue();
	nOffset = 0;
	c = sStringValue[0];
	while (c != '\0')
	{
		// Arret si caractere non alpha-numerique
		if (c < 0 or not isalnum(c)) // selon la plage de caractere exige par isalnum
		{
			bWithQuote = true;
			break;
		}
		nOffset++;
		c = sStringValue[nOffset];
	}

	// Ecriture dans le cas standard
	if (not bWithQuote)
		sOutputField += sValue.GetValue();
	// Ecriture dans le cas avec quote
	else
	{
		sOutputField += '\'';
		nOffset = 0;
		c = sStringValue[0];
		while (c != '\0')
		{
			sOutputField += c;

			// Doublement des quotes internes
			if (c == '\'')
				sOutputField += '\'';
			nOffset++;
			c = sStringValue[nOffset];
		}
		sOutputField += '\'';
	}
}

/////////////////////////////////////////////////////////////////////
// Classe KWContinuousValueBlock

int KWContinuousValueBlock::SearchValueIndex(Continuous cValue) const
{
	int nValueIndex;
	for (nValueIndex = 0; nValueIndex < GetValueNumber(); nValueIndex++)
	{
		if (GetValueAt(nValueIndex) == cValue)
			return nValueIndex;
	}
	return -1;
}

KWContinuousValueBlock* KWContinuousValueBlock::BuildBlockFromValueDictionary(const KWIndexedCKeyBlock* indexedKeyBlock,
									      const KWValueDictionary* valueDictionary)
{
	KWContinuousValueBlock* resultValueBlock;
	KWContinuousKeyIndex* keyIndexContinuous;
	ObjectArray oaKeyIndexValues;
	int i;

	require(indexedKeyBlock != NULL);
	require(valueDictionary != NULL);

	// Initialisation d'un tableau de KWKeyIndex<Value>
	InitializeKeyIndexArray(indexedKeyBlock, valueDictionary, &oaKeyIndexValues);

	// Rangement des index d'attribut et des valeurs selon leur ordre
	resultValueBlock = NewValueBlock(oaKeyIndexValues.GetSize());
	for (i = 0; i < oaKeyIndexValues.GetSize(); i++)
	{
		keyIndexContinuous = cast(KWContinuousKeyIndex*, oaKeyIndexValues.GetAt(i));

		// Memorisation de l'index d'attribut et de la valeur
		resultValueBlock->SetAttributeSparseIndexAt(i, keyIndexContinuous->GetIndex());
		resultValueBlock->SetValueAt(i, keyIndexContinuous->GetValue());
	}
	ensure(resultValueBlock->GetValueNumber() <= valueDictionary->GetCount());
	ensure(resultValueBlock->Check());
	return resultValueBlock;
}

KWContinuousValueBlock*
KWContinuousValueBlock::BuildBlockFromSparseValueVector(const KWContinuousValueSparseVector* valueSparseVector)
{
	KWContinuousValueBlock* resultValueBlock;
	int i;

	require(valueSparseVector != NULL);
	require(valueSparseVector->Check());

	// Rangement des index sparses et des valeurs dans leur ordre initial
	resultValueBlock = NewValueBlock(valueSparseVector->GetValueNumber());
	for (i = 0; i < valueSparseVector->GetValueNumber(); i++)
	{
		resultValueBlock->SetAttributeSparseIndexAt(i, valueSparseVector->GetSparseIndexAt(i));
		resultValueBlock->SetValueAt(i, valueSparseVector->GetValueAt(i));
	}
	ensure(resultValueBlock->GetValueNumber() == valueSparseVector->GetValueNumber());
	ensure(resultValueBlock->Check());
	return resultValueBlock;
}

KWContinuousValueBlock* KWContinuousValueBlock::ExtractBlockSubset(const KWContinuousValueBlock* sourceValueBlock,
								   const IntVector* ivNewValueIndexes)
{
	KWContinuousValueBlock* valueBlock;
	int nNewBlockSize;
	int i;
	int nNewValueIndex;

	require(sourceValueBlock != NULL);
	require(ivNewValueIndexes != NULL);
	require(sourceValueBlock->GetValueNumber() == 0 or
		sourceValueBlock->GetAttributeSparseIndexAt(sourceValueBlock->GetValueNumber() - 1) <
		    ivNewValueIndexes->GetSize());

	// Calcul de la taille du nouveau bloc en comptant le nombre d'index a transferer
	nNewBlockSize = 0;
	for (i = 0; i < sourceValueBlock->GetValueNumber(); i++)
	{
		assert(i == 0 or sourceValueBlock->GetAttributeSparseIndexAt(i) >
				     sourceValueBlock->GetAttributeSparseIndexAt(i - 1));
		if (ivNewValueIndexes->GetAt(sourceValueBlock->GetAttributeSparseIndexAt(i)) >= 0)
			nNewBlockSize++;
	}

	// Creation du nouveau bloc
	valueBlock = NewValueBlock(nNewBlockSize);

	// Recopie de son contenu
	nNewBlockSize = 0;
	for (i = 0; i < sourceValueBlock->GetValueNumber(); i++)
	{
		nNewValueIndex = ivNewValueIndexes->GetAt(sourceValueBlock->GetAttributeSparseIndexAt(i));

		// Memorisation de la paire (index, valeur) si valide
		if (nNewValueIndex >= 0)
		{
			valueBlock->SetAttributeSparseIndexAt(nNewBlockSize, nNewValueIndex);
			valueBlock->SetValueAt(nNewBlockSize, sourceValueBlock->GetValueAt(i));
			assert(nNewBlockSize == 0 or valueBlock->GetAttributeSparseIndexAt(nNewBlockSize) >
							 valueBlock->GetAttributeSparseIndexAt(nNewBlockSize - 1));
			nNewBlockSize++;
		}
	}
	return valueBlock;
}

KWContinuousValueBlock* KWContinuousValueBlock::BuildBlockFromField(const KWIndexedKeyBlock* indexedKeyBlock,
								    const char* sInputField, Continuous cDefaultValue,
								    boolean& bOk, ALString& sMessage)
{
	require(indexedKeyBlock != NULL);
	if (indexedKeyBlock->GetVarKeyType() == KWType::Symbol)
		return BuildCKeyBlockFromField(cast(const KWIndexedCKeyBlock*, indexedKeyBlock), sInputField,
					       cDefaultValue, bOk, sMessage);
	else
		return BuildNKeyBlockFromField(cast(const KWIndexedNKeyBlock*, indexedKeyBlock), sInputField,
					       cDefaultValue, bOk, sMessage);
}

void KWContinuousValueBlock::WriteField(const KWIndexedKeyBlock* indexedKeyBlock, ALString& sOutputField) const
{
	int nValue;
	int nAttributeSparseIndex;

	require(indexedKeyBlock != NULL);

	// Reinitialisation de la taille de la chaine en sortie sans la desallouer
	sOutputField.GetBufferSetLength(0);

	// Parcours des valeurs
	for (nValue = 0; nValue < GetValueNumber(); nValue++)
	{
		// Acces a l'index d'attribut sparse
		nAttributeSparseIndex = GetAttributeSparseIndexAt(nValue);

		// Memorisation de la cle et de la valeur
		if (nValue > 0)
			sOutputField += ' ';
		WriteKey(sOutputField, indexedKeyBlock, nAttributeSparseIndex);
		sOutputField += ':';
		WriteContinuousValue(sOutputField, GetValueAt(nValue));
	}
}

int KWContinuousValueBlock::WriteFieldPart(const KWIndexedKeyBlock* indexedKeyBlock, int nFirstSparseIndex,
					   int nLastSparseIndex, ALString& sOutputField) const
{
	int nWrittenValueNumber;
	int nValue;
	int nAttributeSparseIndex;
	boolean bFirstFieldWritten;

	require(indexedKeyBlock != NULL);
	require(0 <= nFirstSparseIndex);
	require(nFirstSparseIndex <= nLastSparseIndex);
	require(nLastSparseIndex < indexedKeyBlock->GetKeyNumber());

	// Reinitialisation de la taille de la chaine en sortie sans la desallouer
	sOutputField.GetBufferSetLength(0);

	// Recherche du plus petit index de valeur superieur ou egal au premier index sparse
	if (nFirstSparseIndex == 0)
		nValue = 0;
	else
		nValue = GetSmallestValueIndexBeyondAttributeSparseIndex(nFirstSparseIndex);

	// Si trouve, on ecrit les champs sparse jusqu'au dernier demande
	nWrittenValueNumber = 0;
	if (nValue >= 0)
	{
		// Parcours des valeurs
		bFirstFieldWritten = false;
		while (nValue < GetValueNumber())
		{
			// Acces a l'index d'attribut sparse
			nAttributeSparseIndex = GetAttributeSparseIndexAt(nValue);
			assert(nAttributeSparseIndex >= nFirstSparseIndex);

			// Ecriture si avant le dernier
			if (nAttributeSparseIndex <= nLastSparseIndex)
			{
				// Memorisation de la cle et de la valeur
				if (bFirstFieldWritten)
					sOutputField += ' ';
				else
					bFirstFieldWritten = true;
				WriteKey(sOutputField, indexedKeyBlock, nAttributeSparseIndex);
				sOutputField += ':';
				WriteContinuousValue(sOutputField, GetValueAt(nValue));
				nWrittenValueNumber++;

				// Valeur suivante
				nValue++;
			}
			// Arret sinon
			else
				break;
		}
	}
	return nWrittenValueNumber;
}

KWContinuousValueBlock* KWContinuousValueBlock::ConcatValueBlocks(const KWContinuousValueBlock* sourceValueBlock1,
								  const KWContinuousValueBlock* sourceValueBlock2)
{
	KWContinuousValueBlock* valueBlock;
	int i;
	int nStart;

	require(sourceValueBlock1 != NULL);
	require(sourceValueBlock2 != NULL);

	// Les deux blocs doivent necessairement contenir des plages de SparseIndex exclusives, et dans le bon ordre
	require(sourceValueBlock1->GetValueNumber() == 0 or sourceValueBlock2->GetValueNumber() == 0 or
		sourceValueBlock1->GetAttributeSparseIndexAt(sourceValueBlock1->GetValueNumber() - 1) <
		    sourceValueBlock2->GetAttributeSparseIndexAt(0));

	// Creation du bloc
	valueBlock = NewValueBlock(sourceValueBlock1->GetValueNumber() + sourceValueBlock2->GetValueNumber());

	// Ajout du premier bloc
	for (i = 0; i < sourceValueBlock1->GetValueNumber(); i++)
	{
		valueBlock->SetAttributeSparseIndexAt(i, sourceValueBlock1->GetAttributeSparseIndexAt(i));
		valueBlock->SetValueAt(i, sourceValueBlock1->GetValueAt(i));
	}

	// Ajout du second bloc
	nStart = sourceValueBlock1->GetValueNumber();
	for (i = 0; i < sourceValueBlock2->GetValueNumber(); i++)
	{
		valueBlock->SetAttributeSparseIndexAt(nStart, sourceValueBlock2->GetAttributeSparseIndexAt(i));
		valueBlock->SetValueAt(nStart, sourceValueBlock2->GetValueAt(i));
		nStart++;
	}
	return valueBlock;
}

KWContinuousValueBlock* KWContinuousValueBlock::Clone() const
{
	KWContinuousValueBlock* cloneValueBlock;
	int i;

	cloneValueBlock = NewValueBlock(GetValueNumber());
	for (i = 0; i < GetValueNumber(); i++)
	{
		cloneValueBlock->SetAttributeSparseIndexAt(i, GetAttributeSparseIndexAt(i));
		cloneValueBlock->SetValueAt(i, GetValueAt(i));
	}
	return cloneValueBlock;
}

void KWContinuousValueBlock::Write(ostream& ost) const
{
	const int nMax = 10;
	int n;

	ost << GetClassLabel() << " [" << GetValueNumber() << "]:";
	for (n = 0; n < GetValueNumber(); n++)
	{
		ost << " (" << GetAttributeSparseIndexAt(n) << ":";
		ost << " " << KWContinuous::ContinuousToString(GetValueAt(n)) << ")";
		if (n >= nMax)
		{
			ost << "...";
			break;
		}
	}
	cout << "\n";
}

void KWContinuousValueBlock::Test()
{
	const int nKeyNumber = 1000;
	const int nMaxValueNumber = 10;
	SymbolVector svKeys;
	KWIndexedCKeyBlock indexedKeyBlock;
	KWContinuousValueDictionary valueDictionary;
	KWContinuousValueBlock* continuousValueBlock;
	boolean bOk;
	ALString sExternalField;
	StringVector svExternalFields;
	ALString sMessage;
	ALString sTmp;
	int i;
	int nKey;
	Symbol sKey;
	Continuous cValue;
	Continuous cDefaultValue;
	int nFound;

	// Creation des cles
	for (i = 0; i < nKeyNumber; i++)
	{
		nKey = i + 1;
		sKey = Symbol(sTmp + "Key" + IntToString(nKey));
		svKeys.Add(sKey);
	}

	// Tri par valeur pour les inserer dans le bon ordre
	svKeys.SortValues();

	// Creation d'un bloc de cles indexe
	cDefaultValue = KWContinuous::GetMissingValue();
	for (i = 0; i < nKeyNumber; i++)
	{
		indexedKeyBlock.AddKey(svKeys.GetAt(i));
	}
	cout << indexedKeyBlock << endl;

	// Creation d'un dictionnaire de valeurs
	for (i = 0; i < nMaxValueNumber; i++)
	{
		nKey = 10 * (i + 1);
		sKey = Symbol(sTmp + "Key" + IntToString(nKey));
		valueDictionary.SetAt(sKey, nKey);
	}
	cout << valueDictionary << endl;

	// Initialisation du bloc de valeurs
	continuousValueBlock = BuildBlockFromValueDictionary(&indexedKeyBlock, &valueDictionary);
	cout << *continuousValueBlock << endl;

	// Recherche des valeurs dans le bloc
	cout << "Search values by variable index\n";
	nFound = 0;
	for (i = 0; i < nKeyNumber; i++)
	{
		cValue = continuousValueBlock->GetValueAtAttributeSparseIndex(i, cDefaultValue);
		if (cValue != cDefaultValue)
		{
			nFound++;
			if (nFound <= 10)
			{
				sKey = indexedKeyBlock.GetKeyAt(i);
				cout << "\t" << i << "\t" << sKey << "\t" << cValue << "\n";
			}
			else if (nFound == 11)
				cout << "\t...\n";
		}
	}
	assert(nFound == continuousValueBlock->GetValueNumber());

	// Conversion en chaine de caracteres
	continuousValueBlock->WriteField(&indexedKeyBlock, sExternalField);
	cout << "Write field\n";
	cout << sExternalField << endl;
	delete continuousValueBlock;
	continuousValueBlock = BuildBlockFromField(&indexedKeyBlock, sExternalField, cDefaultValue, bOk, sMessage);
	cout << "Read field\n";
	if (bOk)
		cout << *continuousValueBlock << endl;
	else
		cout << "error: " << sMessage << endl;

	// Conversion erronnees
	svExternalFields.Add("");
	svExternalFields.Add("Key10");
	svExternalFields.Add("Key10:10");
	svExternalFields.Add("'Key10':10");
	svExternalFields.Add("Key10:10a");
	svExternalFields.Add(" Key10:10");
	svExternalFields.Add("Key10:10 ");
	svExternalFields.Add("Key10");
	svExternalFields.Add("Key50:50");
	svExternalFields.Add("Key50:50 Key70:70");
	svExternalFields.Add("Key70:70 Key50:50");
	svExternalFields.Add("Key50:50 Key50:50");
	svExternalFields.Add("Key50:50  Key70:70");
	svExternalFields.Add("Key50:50:Key70:70");
	svExternalFields.Add("Key7:7");
	svExternalFields.Add("Key1111:1111");
	cout << "Read erroneous field\n";
	for (i = 0; i < svExternalFields.GetSize(); i++)
	{
		sExternalField = svExternalFields.GetAt(i);
		cout << "<" << sExternalField << ">\n";
		delete continuousValueBlock;
		continuousValueBlock =
		    BuildBlockFromField(&indexedKeyBlock, sExternalField, cDefaultValue, bOk, sMessage);
		if (bOk)
			cout << "\t" << *continuousValueBlock;
		else
			cout << "\terror: " << sMessage << endl;
	}
	delete continuousValueBlock;
}

void KWContinuousValueBlock::TestPerformance()
{
	KWContinuousValueBlock* valueBlock;
	IntVector ivSizes;
	int nTest;
	int nTotalIter;
	int nMaxSize = 100000;
	int nSize;
	int nTimes;
	int i;
	int nIndex;
	double dExpectedTotal;
	double dTotal;
	Timer timer;
	longint lEmptySize;
	longint lSize;
	ALString sTmp;

	// Nombre total d'iteration par test (plus petit en debug)
	nTotalIter = nMaxSize * 100;
	debug(nTotalIter = nMaxSize);
	cout << "Total iter\t" << nTotalIter << endl;

	// Initialisation de la famille de test
	nSize = 1;
	while (nSize <= nMaxSize)
	{
		ivSizes.Add(nSize);
		nSize *= 10;
	}

	// Parcours de tous les test
	for (nTest = 0; nTest < ivSizes.GetSize(); nTest++)
	{
		nSize = ivSizes.GetAt(nTest);
		nTimes = nTotalIter / nSize;

		// Affichage de l'entete des resultats
		if (nTest == 0)
			cout << "Size\tEmpty size\tItem size\tCreation time\tRead time\tIndex check\n";

		// Statistique preliminaires
		valueBlock = NewValueBlock(0);
		lEmptySize = valueBlock->GetUsedMemory();
		cout << nSize << "\t";
		cout << lEmptySize << "\t";

		// Creation de cles dans le bloc
		timer.Reset();
		timer.Start();
		for (i = 0; i < nTimes; i++)
		{
			delete valueBlock;
			valueBlock = NewValueBlock(nSize);
		}
		timer.Stop();
		lSize = valueBlock->GetUsedMemory();
		cout << int((lSize - lEmptySize) * 10 / nSize) / 10.0 << "\t";
		cout << (timer.GetElapsedTime() / (nTimes * nSize)) * 1000000000 << "\t";

		// Initialisation du dernier bloc en cours
		for (nIndex = 0; nIndex < nSize; nIndex++)
		{
			valueBlock->SetAttributeSparseIndexAt(nIndex, nIndex + 1);
			valueBlock->SetValueAt(nIndex, nIndex + 1);
		}

		// Creation de cles dans le bloc
		timer.Reset();
		timer.Start();
		dExpectedTotal = nTimes * (valueBlock->GetValueNumber() + 1.0) * valueBlock->GetValueNumber();
		dTotal = 0;
		for (i = 0; i < nTimes; i++)
		{
			for (nIndex = 0; nIndex < valueBlock->GetValueNumber(); nIndex++)
			{
				dTotal += valueBlock->GetAttributeSparseIndexAt(nIndex);
				dTotal += valueBlock->GetValueAt(nIndex);
			}
		}
		timer.Stop();
		cout << (timer.GetElapsedTime() / (nTimes * nSize)) * 1000000000 << "\t";
		cout << dTotal - dExpectedTotal << endl;
		delete valueBlock;
	}
}

KWContinuousValueBlock* KWContinuousValueBlock::BuildCKeyBlockFromField(const KWIndexedCKeyBlock* indexedKeyBlock,
									const char* sInputField,
									Continuous cDefaultValue, boolean& bOk,
									ALString& sMessage)
{
	KWContinuousValueBlock* resultValueBlock;
	KWContinuousValueDictionary valueDictionary;
	int nOffset;
	Symbol sKeySymbol;
	ALString sKey;
	boolean bExistingValue;
	ALString sValue;
	Continuous cValue;
	int nFieldError;
	int nError;

	require(indexedKeyBlock != NULL);
	require(sInputField != NULL);

	// Reinitialisation de la taille du message sans le desallouer
	sMessage.GetBufferSetLength(0);

	// Boucle de lecture des paires (cle, valeur)
	nOffset = 0;
	bOk = true;
	while (bOk and sInputField[nOffset] != '\0')
	{
		// Lecture d'un caractere blanc
		bOk = ReadBlankSeparator(sInputField, nOffset, nFieldError);
		if (not bOk)
			sMessage = BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField, nOffset);

		// Lecture de la cle
		if (bOk and sInputField[nOffset] != '\0')
		{
			bOk = ReadCKey(sInputField, nOffset, sKey, nFieldError);
			if (not bOk)
				sMessage =
				    BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField, nOffset);
		}

		// Lecture de la valeur si elle existe
		bExistingValue = false;
		if (bOk and sInputField[nOffset] != '\0')
		{
			if (sInputField[nOffset] == ':')
			{
				// On saute le caractere ':'
				nOffset++;

				// Lecture de la valeur continue sous forme chaine de caracteres
				bOk = ReadContinuousValue(sInputField, nOffset, sValue, nFieldError);

				// Erreur de valeur
				if (not bOk)
					sMessage = BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField,
								     nOffset);
				else
					bExistingValue = true;
			}
		}

		// Traitement de la paire cle valeur
		if (bOk)
		{
			sKeySymbol = Symbol(sKey);

			// Test si la cle est deja presente
			if (valueDictionary.IsKeyPresent(sKeySymbol))
			{
				bOk = false;
				sMessage = BuildErrorMessage("Var key used more than once", sKey, sInputField, nOffset);
			}
			// Si ok et cle a conserver, memorisation de la valeur dans le dictionnaire
			else if (indexedKeyBlock->IsKeyPresent(sKeySymbol))
			{
				// Conversion de la valeur si presente
				if (bExistingValue)
				{
					nError = KWContinuous::StringToContinuousError(sValue, cValue);
					bOk = (nError == KWContinuous::NoError);
					if (not bOk)
						sMessage = BuildErrorMessage(KWContinuous::ErrorLabel(nError), sKey,
									     sInputField, nOffset);
				}
				//  Sinon, on prend 1, comme dans SVMLight
				else
					cValue = 1;

				// Memorisation si ok et valeur utile
				if (bOk and cValue != cDefaultValue)
					valueDictionary.SetAt(sKeySymbol, cValue);
			}
		}
	}

	// Si Ok, on extrait la representation dense
	resultValueBlock = NULL;
	if (bOk)
		resultValueBlock = BuildBlockFromValueDictionary(indexedKeyBlock, &valueDictionary);

	// Sinon, on renvoie un bloc vide
	if (not bOk)
		resultValueBlock = NewValueBlock(0);
	ensure(resultValueBlock->Check());
	return resultValueBlock;
}

KWContinuousValueBlock* KWContinuousValueBlock::BuildNKeyBlockFromField(const KWIndexedNKeyBlock* indexedKeyBlock,
									const char* sInputField,
									Continuous cDefaultValue, boolean& bOk,
									ALString& sMessage)
{
	KWContinuousValueBlock* resultValueBlock;
	KWContinuousValueDictionary valueDictionary;
	KWContinuousValueSparseVector valueSparseVector;
	int nOffset;
	Symbol sKeySymbol;
	ALString sKey;
	int nLastKey;
	int nKey;
	int nSparseIndex;
	boolean bExistingValue;
	ALString sValue;
	Continuous cValue;
	int nFieldError;
	int nError;

	require(indexedKeyBlock != NULL);
	require(sInputField != NULL);

	// Reinitialisation de la taille du message sans le desallouer
	sMessage.GetBufferSetLength(0);

	// Boucle de lecture des paires (cle, valeur)
	nLastKey = 0;
	nKey = 0;
	nOffset = 0;
	bOk = true;
	while (bOk and sInputField[nOffset] != '\0')
	{
		// Lecture d'un caractere blanc
		bOk = ReadBlankSeparator(sInputField, nOffset, nFieldError);
		if (not bOk)
			sMessage = BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField, nOffset);

		// Lecture de la cle
		if (bOk and sInputField[nOffset] != '\0')
		{
			bOk = ReadNKey(sInputField, nOffset, sKey, nKey, nFieldError);
			if (not bOk)
				sMessage =
				    BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField, nOffset);
		}

		// Lecture de la valeur si elle existe
		bExistingValue = false;
		if (bOk and sInputField[nOffset] != '\0')
		{
			if (sInputField[nOffset] == ':')
			{
				// On saute le caractere ':'
				nOffset++;

				// Lecture de la valeur continue sous forme chaine de caracteres
				bOk = ReadContinuousValue(sInputField, nOffset, sValue, nFieldError);

				// Erreur de valeur
				if (not bOk)
					sMessage = BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField,
								     nOffset);
				else
					bExistingValue = true;
			}
		}

		// Test de l'ordre et de l'unicite de la cle
		if (bOk and nLastKey > 0)
		{
			if (nKey <= nLastKey)
			{
				bOk = false;

				// Cas d'egalite
				if (nKey == nLastKey)
					sMessage = sMessage + "var key " + IntToString(nKey) + " is used twice";
				// Cas de mauvais ordre
				else
					sMessage = sMessage + "var key " + IntToString(nKey) +
						   " should be greater than the preceding key " + IntToString(nLastKey);
				sMessage = BuildErrorMessage(sMessage, "", sInputField, nOffset);
			}
		}

		// Test si valeur a conserver
		if (bOk)
		{
			assert(1 <= nKey and nKey <= 1000000000);

			// Memorisation de la derniere cle traitee
			nLastKey = nKey;

			// Si cle a conserver, memorisation de la valeur dans le dictionnaire
			nSparseIndex = indexedKeyBlock->GetKeyIndex(nKey);
			if (nSparseIndex != -1)
			{
				// Conversion de la valeur si presente
				if (bExistingValue)
				{
					nError = KWContinuous::StringToContinuousError(sValue, cValue);
					bOk = (nError == KWContinuous::NoError);
					if (not bOk)
						sMessage = BuildErrorMessage(KWContinuous::ErrorLabel(nError), sKey,
									     sInputField, nOffset);
				}
				//  Sinon, on prend 1, comme dans SVMLight
				else
					cValue = 1;

				// Memorisation si ok et valeur utile
				if (bOk and cValue != cDefaultValue)
					valueSparseVector.AddValueAt(nSparseIndex, cValue);
			}
		}

		// Arret si erreur
		if (not bOk)
			break;
	}

	// Si Ok, on extrait la representation dense
	if (bOk)
	{
		// Les index sparse etant dans le bon ordre, le tri n'est pas necessaire
		resultValueBlock = BuildBlockFromSparseValueVector(&valueSparseVector);
		assert(sMessage == "");
	}
	// Sinon, on renvoie un bloc vide
	else
	{
		resultValueBlock = NewValueBlock(0);
		assert(sMessage != "");
	}
	ensure(resultValueBlock->Check());
	return resultValueBlock;
}

/////////////////////////////////////////////////////////////////////
// Classe KWSymbolValueBlock

KWSymbolValueBlock::~KWSymbolValueBlock()
{
	int i;

	// Attention: on force prealablement une initialisation afin de dereferencer les symbols
	for (i = 0; i < GetValueNumber(); i++)
		SetValueAt(i, Symbol());
}

int KWSymbolValueBlock::SearchValueIndex(const Symbol& sValue) const
{
	int nValueIndex;
	for (nValueIndex = 0; nValueIndex < GetValueNumber(); nValueIndex++)
	{
		if (GetValueAt(nValueIndex) == sValue)
			return nValueIndex;
	}
	return -1;
}

KWSymbolValueBlock* KWSymbolValueBlock::BuildBlockFromValueDictionary(const KWIndexedCKeyBlock* indexedKeyBlock,
								      const KWValueDictionary* valueDictionary)
{
	KWSymbolValueBlock* resultValueBlock;
	KWSymbolKeyIndex* keyIndexSymbol;
	ObjectArray oaKeyIndexValues;
	int i;

	require(indexedKeyBlock != NULL);
	require(valueDictionary != NULL);

	// Initialisation d'un tableau de KWKeyIndex<Value>
	InitializeKeyIndexArray(indexedKeyBlock, valueDictionary, &oaKeyIndexValues);

	// Rangement des index d'attribut et des valeurs selon leur ordre
	resultValueBlock = NewValueBlock(oaKeyIndexValues.GetSize());
	for (i = 0; i < oaKeyIndexValues.GetSize(); i++)
	{
		keyIndexSymbol = cast(KWSymbolKeyIndex*, oaKeyIndexValues.GetAt(i));

		// Memorisation de l'index d'attribut et de la valeur
		resultValueBlock->SetAttributeSparseIndexAt(i, keyIndexSymbol->GetIndex());
		resultValueBlock->SetValueAt(i, keyIndexSymbol->GetValue());
	}
	ensure(resultValueBlock->GetValueNumber() <= valueDictionary->GetCount());
	ensure(resultValueBlock->Check());
	return resultValueBlock;
}

KWSymbolValueBlock*
KWSymbolValueBlock::BuildBlockFromSparseValueVector(const KWSymbolValueSparseVector* valueSparseVector)
{
	KWSymbolValueBlock* resultValueBlock;
	int i;

	require(valueSparseVector != NULL);
	require(valueSparseVector->Check());

	// Rangement des index sparses et des valeurs dans leur ordre initial
	resultValueBlock = NewValueBlock(valueSparseVector->GetValueNumber());
	for (i = 0; i < valueSparseVector->GetValueNumber(); i++)
	{
		resultValueBlock->SetAttributeSparseIndexAt(i, valueSparseVector->GetSparseIndexAt(i));
		resultValueBlock->SetValueAt(i, valueSparseVector->GetValueAt(i));
	}
	ensure(resultValueBlock->GetValueNumber() == valueSparseVector->GetValueNumber());
	ensure(resultValueBlock->Check());
	return resultValueBlock;
}

KWSymbolValueBlock* KWSymbolValueBlock::ExtractBlockSubset(const KWSymbolValueBlock* sourceValueBlock,
							   const IntVector* ivNewValueIndexes)
{
	KWSymbolValueBlock* valueBlock;
	int nNewBlockSize;
	int i;
	int nNewValueIndex;

	require(sourceValueBlock != NULL);
	require(ivNewValueIndexes != NULL);
	require(sourceValueBlock->GetValueNumber() == 0 or
		sourceValueBlock->GetAttributeSparseIndexAt(sourceValueBlock->GetValueNumber() - 1) <
		    ivNewValueIndexes->GetSize());

	// Calcul de la taille du nouveau bloc en comptant le nombre d'index a transferer
	nNewBlockSize = 0;
	for (i = 0; i < sourceValueBlock->GetValueNumber(); i++)
	{
		assert(i == 0 or sourceValueBlock->GetAttributeSparseIndexAt(i) >
				     sourceValueBlock->GetAttributeSparseIndexAt(i - 1));
		if (ivNewValueIndexes->GetAt(sourceValueBlock->GetAttributeSparseIndexAt(i)) >= 0)
			nNewBlockSize++;
	}

	// Creation du nouveau bloc
	valueBlock = NewValueBlock(nNewBlockSize);

	// Recopie de son contenu
	nNewBlockSize = 0;
	for (i = 0; i < sourceValueBlock->GetValueNumber(); i++)
	{
		nNewValueIndex = ivNewValueIndexes->GetAt(sourceValueBlock->GetAttributeSparseIndexAt(i));

		// Memorisation de la paire (index, valeur) si valide
		if (nNewValueIndex >= 0)
		{
			valueBlock->SetAttributeSparseIndexAt(nNewBlockSize, nNewValueIndex);
			valueBlock->SetValueAt(nNewBlockSize, sourceValueBlock->GetValueAt(i));
			assert(nNewBlockSize == 0 or valueBlock->GetAttributeSparseIndexAt(nNewBlockSize) >
							 valueBlock->GetAttributeSparseIndexAt(nNewBlockSize - 1));
			nNewBlockSize++;
		}
	}
	return valueBlock;
}

// Les methodes ReadField et WriteField sont recopiees et adaptee de la classe KWContinuousValueBlock
// Il est plus simple ici de faire une maintenance par recopie plutot que par reutilisation
// (trop de specificites pour l'adaptation du type Continuous a Symbol)
KWSymbolValueBlock* KWSymbolValueBlock::BuildBlockFromField(const KWIndexedKeyBlock* indexedKeyBlock,
							    const char* sInputField, const Symbol& sDefaultValue,
							    boolean& bOk, ALString& sMessage)
{
	require(indexedKeyBlock != NULL);
	if (indexedKeyBlock->GetVarKeyType() == KWType::Symbol)
		return BuildCKeyBlockFromField(cast(const KWIndexedCKeyBlock*, indexedKeyBlock), sInputField,
					       sDefaultValue, bOk, sMessage);
	else
		return BuildNKeyBlockFromField(cast(const KWIndexedNKeyBlock*, indexedKeyBlock), sInputField,
					       sDefaultValue, bOk, sMessage);
}

void KWSymbolValueBlock::WriteField(const KWIndexedKeyBlock* indexedKeyBlock, ALString& sOutputField) const
{
	int nValue;
	int nAttributeSparseIndex;

	require(indexedKeyBlock != NULL);

	// Reinitialisation de la taille de la chaine en sortie sans la desallouer
	sOutputField.GetBufferSetLength(0);

	// Parcours des valeurs dense
	for (nValue = 0; nValue < GetValueNumber(); nValue++)
	{
		// Acces a l'index d'attribut sparse
		nAttributeSparseIndex = GetAttributeSparseIndexAt(nValue);

		// Memorisation de la cle et de la valeur
		if (nValue > 0)
			sOutputField += ' ';
		WriteKey(sOutputField, indexedKeyBlock, nAttributeSparseIndex);
		sOutputField += ':';
		WriteSymbolValue(sOutputField, GetValueAt(nValue));
	}
}

int KWSymbolValueBlock::WriteFieldPart(const KWIndexedKeyBlock* indexedKeyBlock, int nFirstSparseIndex,
				       int nLastSparseIndex, ALString& sOutputField) const
{
	int nWrittenValueNumber;
	int nValue;
	int nAttributeSparseIndex;
	boolean bFirstFieldWritten;

	require(indexedKeyBlock != NULL);
	require(0 <= nFirstSparseIndex);
	require(nFirstSparseIndex <= nLastSparseIndex);
	require(nLastSparseIndex < indexedKeyBlock->GetKeyNumber());

	// Reinitialisation de la taille de la chaine en sortie sans la desallouer
	sOutputField.GetBufferSetLength(0);

	// Recherche du plus petit index de valeur superieur ou egal au premier index sparse
	if (nFirstSparseIndex == 0)
		nValue = 0;
	else
		nValue = GetSmallestValueIndexBeyondAttributeSparseIndex(nFirstSparseIndex);

	// Si trouve, on ecrit les champs sparse jusqu'au dernier demande
	nWrittenValueNumber = 0;
	if (nValue >= 0)
	{
		// Parcours des valeurs
		bFirstFieldWritten = false;
		while (nValue < GetValueNumber())
		{
			// Acces a l'index d'attribut sparse
			nAttributeSparseIndex = GetAttributeSparseIndexAt(nValue);
			assert(nAttributeSparseIndex >= nFirstSparseIndex);

			// Ecriture si avant le dernier
			if (nAttributeSparseIndex <= nLastSparseIndex)
			{
				// Memorisation de la cle et de la valeur
				if (bFirstFieldWritten)
					sOutputField += ' ';
				else
					bFirstFieldWritten = true;
				WriteKey(sOutputField, indexedKeyBlock, nAttributeSparseIndex);
				sOutputField += ':';
				WriteSymbolValue(sOutputField, GetValueAt(nValue));
				nWrittenValueNumber++;

				// Valeur suivante
				nValue++;
			}
			// Arret sinon
			else
				break;
		}
	}
	return nWrittenValueNumber;
}

KWSymbolValueBlock* KWSymbolValueBlock::ConcatValueBlocks(const KWSymbolValueBlock* sourceValueBlock1,
							  const KWSymbolValueBlock* sourceValueBlock2)
{
	KWSymbolValueBlock* valueBlock;
	int i;
	int nStart;

	require(sourceValueBlock1 != NULL);
	require(sourceValueBlock2 != NULL);

	// Les deux blocs doivent necessairement contenir des plages de SparseIndex exclusives, et dans le bon ordre
	require(sourceValueBlock1->GetValueNumber() == 0 or sourceValueBlock2->GetValueNumber() == 0 or
		sourceValueBlock1->GetAttributeSparseIndexAt(sourceValueBlock1->GetValueNumber() - 1) <
		    sourceValueBlock2->GetAttributeSparseIndexAt(0));

	// Creation du bloc
	valueBlock = NewValueBlock(sourceValueBlock1->GetValueNumber() + sourceValueBlock2->GetValueNumber());

	// Ajout du premier bloc
	for (i = 0; i < sourceValueBlock1->GetValueNumber(); i++)
	{
		valueBlock->SetAttributeSparseIndexAt(i, sourceValueBlock1->GetAttributeSparseIndexAt(i));
		valueBlock->SetValueAt(i, sourceValueBlock1->GetValueAt(i));
	}

	// Ajout du second bloc
	nStart = sourceValueBlock1->GetValueNumber();
	for (i = 0; i < sourceValueBlock2->GetValueNumber(); i++)
	{
		valueBlock->SetAttributeSparseIndexAt(nStart, sourceValueBlock2->GetAttributeSparseIndexAt(i));
		valueBlock->SetValueAt(nStart, sourceValueBlock2->GetValueAt(i));
		nStart++;
	}
	return valueBlock;
}

KWSymbolValueBlock* KWSymbolValueBlock::Clone() const
{
	KWSymbolValueBlock* cloneValueBlock;
	int i;

	cloneValueBlock = NewValueBlock(GetValueNumber());
	for (i = 0; i < GetValueNumber(); i++)
	{
		cloneValueBlock->SetAttributeSparseIndexAt(i, GetAttributeSparseIndexAt(i));
		cloneValueBlock->SetValueAt(i, GetValueAt(i));
	}
	return cloneValueBlock;
}

void KWSymbolValueBlock::Write(ostream& ost) const
{
	const int nMax = 10;
	int n;

	ost << GetClassLabel() << " [" << GetValueNumber() << "]:";
	for (n = 0; n < GetValueNumber(); n++)
	{
		ost << " (" << GetAttributeSparseIndexAt(n) << ":";
		ost << " " << GetValueAt(n) << ")";
		if (n >= nMax)
		{
			ost << "...";
			break;
		}
	}
	cout << "\n";
}

void KWSymbolValueBlock::Test()
{
	const int nKeyNumber = 1000;
	const int nMaxValueNumber = 10;
	KWIndexedCKeyBlock indexedKeyBlock;
	SymbolVector svKeys;
	KWSymbolValueDictionary valueDictionary;
	KWSymbolValueBlock* symbolValueBlock;
	boolean bOk;
	ALString sExternalField;
	StringVector svExternalFields;
	ALString sMessage;
	ALString sTmp;
	int i;
	int nKey;
	Symbol sKey;
	Symbol sValue;
	Symbol sDefaultValue;
	int nFound;

	// Creation des cles
	for (i = 0; i < nKeyNumber; i++)
	{
		nKey = i + 1;
		sKey = Symbol(sTmp + "Key" + IntToString(nKey));
		svKeys.Add(sKey);
	}

	// Tri par valeur pour les inserer dans le bon ordre
	svKeys.SortValues();

	// Creation d'un bloc de cles indexe
	for (i = 0; i < nKeyNumber; i++)
	{
		indexedKeyBlock.AddKey(svKeys.GetAt(i));
	}
	cout << indexedKeyBlock << endl;

	// Creation d'un dictionnaire de valeurs
	for (i = 0; i < nMaxValueNumber; i++)
	{
		nKey = 10 * (i + 1);
		sKey = Symbol(sTmp + "Key" + IntToString(nKey));
		valueDictionary.SetAt(sKey, Symbol(sTmp + "v" + IntToString(nKey)));
	}
	cout << valueDictionary << endl;

	// Initialisation du bloc de valeurs
	symbolValueBlock = BuildBlockFromValueDictionary(&indexedKeyBlock, &valueDictionary);
	cout << *symbolValueBlock << endl;

	// Recherche des valeurs dans le bloc
	cout << "Search values by variable index\n";
	nFound = 0;
	for (i = 0; i < nKeyNumber; i++)
	{
		sValue = symbolValueBlock->GetValueAtAttributeSparseIndex(i, sDefaultValue);
		if (not sValue.IsEmpty())
		{
			nFound++;
			if (nFound <= 10)
			{
				sKey = indexedKeyBlock.GetKeyAt(i);
				cout << "\t" << i << "\t" << sKey << "\t" << sValue << "\n";
			}
			else if (nFound == 11)
				cout << "\t...\n";
		}
	}
	assert(nFound == symbolValueBlock->GetValueNumber());

	// Conversion en chaine de caracteres
	symbolValueBlock->WriteField(&indexedKeyBlock, sExternalField);
	cout << "Write field\n";
	cout << sExternalField << endl;
	delete symbolValueBlock;
	symbolValueBlock = BuildBlockFromField(&indexedKeyBlock, sExternalField, sDefaultValue, bOk, sMessage);
	cout << "Read field\n";
	if (bOk)
		cout << *symbolValueBlock << endl;
	else
		cout << "error: " << sMessage << endl;

	// Conversion erronnees
	svExternalFields.Add("");
	svExternalFields.Add("Key10");
	svExternalFields.Add("Key10:v10");
	svExternalFields.Add("'Key10':v10");
	svExternalFields.Add("Key10:v10a");
	svExternalFields.Add(" Key10:v10");
	svExternalFields.Add("Key10:v10 ");
	svExternalFields.Add("Key10");
	svExternalFields.Add("Key50:v50");
	svExternalFields.Add("Key50:'v50'");
	svExternalFields.Add("Key50:'''v50'");
	svExternalFields.Add("Key50:'v50'''");
	svExternalFields.Add("Key50:'v''50'");
	svExternalFields.Add("Key50:'v'''50'");
	svExternalFields.Add("Key50:'v''''50'");
	svExternalFields.Add("Key50:'v50");
	svExternalFields.Add("Key50:'v50 Key70:v70");
	svExternalFields.Add("Key50:v50 Key70:v70");
	svExternalFields.Add("Key70:v70 Key50:v50");
	svExternalFields.Add("Key50:v50 Key50:v50");
	svExternalFields.Add("Key50:v50  Key70:v70");
	svExternalFields.Add("Key50:v50:Key70:v70");
	svExternalFields.Add("Key7:v7");
	svExternalFields.Add("Key1111:v1111");
	cout << "Read erroneous field\n";
	for (i = 0; i < svExternalFields.GetSize(); i++)
	{
		sExternalField = svExternalFields.GetAt(i);
		cout << "<" << sExternalField << ">\n";
		delete symbolValueBlock;
		symbolValueBlock = BuildBlockFromField(&indexedKeyBlock, sExternalField, sDefaultValue, bOk, sMessage);
		if (bOk)
			cout << "\t" << *symbolValueBlock;
		else
			cout << "\terror: " << sMessage << endl;
	}
	delete symbolValueBlock;
}

KWSymbolValueBlock* KWSymbolValueBlock::BuildCKeyBlockFromField(const KWIndexedCKeyBlock* indexedKeyBlock,
								const char* sInputField, const Symbol& sDefaultValue,
								boolean& bOk, ALString& sMessage)
{
	KWSymbolValueBlock* resultValueBlock;
	KWSymbolValueDictionary valueDictionary;
	int nOffset;
	Symbol sKeySymbol;
	ALString sKey;
	boolean bExistingValue;
	ALString sValue;
	Symbol sValueSymbol;
	const Symbol sValueSymbol1 = Symbol("1");
	int nFieldError;

	require(indexedKeyBlock != NULL);
	require(sInputField != NULL);

	// Reinitialisation de la taille du message sans le desallouer
	sMessage.GetBufferSetLength(0);

	// Boucle de lecture des paire (cle, valeur)
	nOffset = 0;
	bOk = true;
	while (bOk and sInputField[nOffset] != '\0')
	{
		// Lecture d'un caractere blanc
		bOk = ReadBlankSeparator(sInputField, nOffset, nFieldError);
		if (not bOk)
			sMessage = BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField, nOffset);

		// Lecture de la cle
		if (bOk and sInputField[nOffset] != '\0')
		{
			bOk = ReadCKey(sInputField, nOffset, sKey, nFieldError);
			if (not bOk)
				sMessage =
				    BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField, nOffset);
		}

		// Lecture de la valeur si elle existe
		bExistingValue = false;
		if (bOk and sInputField[nOffset] != '\0')
		{
			if (sInputField[nOffset] == ':')
			{
				// On saute le caractere ':'
				nOffset++;

				// Lecture de la valeur Symbol sous forme chaine de caracteres
				bOk = ReadSymbolValue(sInputField, nOffset, sValue, nFieldError);

				// Erreur de valeur
				if (not bOk)
					sMessage = BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField,
								     nOffset);
				else
					bExistingValue = true;
			}
		}

		// Traitement de la paire cle valeur
		if (bOk)
		{
			sKeySymbol = Symbol(sKey);

			// Test si la cle est deja presente
			if (valueDictionary.IsKeyPresent(sKeySymbol))
			{
				bOk = false;
				sMessage = BuildErrorMessage("Var key used more than once", sKey, sInputField, nOffset);
			}
			// Si ok et cle a conserver, memorisation de la valeur dans le dictionnaire
			else if (indexedKeyBlock->IsKeyPresent(sKeySymbol))
			{
				// Conversion et memorisation si valeur utile ("1" si non specifiee, comme dans
				// SVMLight)
				if (bExistingValue)
					sValueSymbol = Symbol(sValue);
				else
					sValueSymbol = sValueSymbol1;
				if (sValueSymbol != sDefaultValue)
					valueDictionary.SetAt(sKeySymbol, sValueSymbol);
			}
		}
	}

	// Si Ok, on extrait la representation dense
	resultValueBlock = NULL;
	if (bOk)
		resultValueBlock = BuildBlockFromValueDictionary(indexedKeyBlock, &valueDictionary);

	// Sinon, on renvoie un bloc vide
	if (not bOk)
		resultValueBlock = NewValueBlock(0);
	ensure(resultValueBlock->Check());
	return resultValueBlock;
}

KWSymbolValueBlock* KWSymbolValueBlock::BuildNKeyBlockFromField(const KWIndexedNKeyBlock* indexedKeyBlock,
								const char* sInputField, const Symbol& sDefaultValue,
								boolean& bOk, ALString& sMessage)
{
	KWSymbolValueBlock* resultValueBlock;
	KWSymbolValueDictionary valueDictionary;
	KWSymbolValueSparseVector valueSparseVector;
	int nOffset;
	Symbol sKeySymbol;
	ALString sKey;
	int nLastKey;
	int nKey;
	int nSparseIndex;
	boolean bExistingValue;
	ALString sValue;
	Symbol sValueSymbol;
	const Symbol sValueSymbol1 = Symbol("1");
	int nFieldError;

	require(indexedKeyBlock != NULL);
	require(sInputField != NULL);

	// Reinitialisation de la taille du message sans le desallouer
	sMessage.GetBufferSetLength(0);

	// Boucle de lecture des paire (cle, valeur)
	nLastKey = 0;
	nKey = 0;
	nOffset = 0;
	bOk = true;
	while (bOk and sInputField[nOffset] != '\0')
	{
		// Lecture d'un caractere blanc
		bOk = ReadBlankSeparator(sInputField, nOffset, nFieldError);
		if (not bOk)
			sMessage = BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField, nOffset);

		// Lecture de la cle
		if (bOk and sInputField[nOffset] != '\0')
		{
			bOk = ReadNKey(sInputField, nOffset, sKey, nKey, nFieldError);
			if (not bOk)
				sMessage =
				    BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField, nOffset);
		}

		// Lecture de la valeur si elle existe
		bExistingValue = false;
		if (bOk and sInputField[nOffset] != '\0')
		{
			if (sInputField[nOffset] == ':')
			{
				// On saute le caractere ':'
				nOffset++;

				// Lecture de la valeur continue
				bOk = ReadSymbolValue(sInputField, nOffset, sValue, nFieldError);

				// Erreur de valeur
				if (not bOk)
					sMessage = BuildErrorMessage(GetFieldErrorLabel(nFieldError), sKey, sInputField,
								     nOffset);
				else
					bExistingValue = true;
			}
		}

		// Test de l'ordre et de l'unicite de la cle
		if (bOk and nLastKey > 0)
		{
			if (nKey <= nLastKey)
			{
				bOk = false;

				// Cas d'egalite
				if (nKey == nLastKey)
					sMessage = sMessage + "var key " + IntToString(nKey) + " is used twice";
				// Cas de mauvais ordre
				else
					sMessage = sMessage + "var key " + IntToString(nKey) +
						   " should be greater than the preceding key " + IntToString(nLastKey);
				sMessage = BuildErrorMessage(sMessage, "", sInputField, nOffset);
			}
		}

		// Test si valeur a conserver
		if (bOk)
		{
			assert(1 <= nKey and nKey <= 1000000000);

			// Si cle a conserver, memorisation de la valeur dans le dictionnaire
			nSparseIndex = indexedKeyBlock->GetKeyIndex(nKey);
			if (nSparseIndex != -1)
			{
				// Conversion et memorisation si valeur utile ("1" si non specifiee, comme dans
				// SVMLight)
				if (bExistingValue)
					sValueSymbol = Symbol(sValue);
				else
					sValueSymbol = sValueSymbol1;
				if (sValueSymbol != sDefaultValue)
					valueSparseVector.AddValueAt(nSparseIndex, sValueSymbol);
			}
		}

		// Arret si erreur
		if (not bOk)
			break;
	}

	// Si Ok, on extrait la representation dense
	if (bOk)
	{
		// Les index sparse etant dans le bon ordre, le tri n'est pas necessaire
		resultValueBlock = BuildBlockFromSparseValueVector(&valueSparseVector);
		assert(sMessage == "");
	}
	// Sinon, on renvoie un bloc vide
	else
	{
		resultValueBlock = NewValueBlock(0);
		assert(sMessage != "");
	}
	ensure(resultValueBlock->Check());
	return resultValueBlock;
}

/////////////////////////////////////////////////////////////////////
// Classe KWObjectArrayValueBlock

KWObjectArrayValueBlock::~KWObjectArrayValueBlock()
{
	int i;
	ObjectArray* oaValue;

	// Destruction des tableaux du bloc
	for (i = 0; i < GetValueNumber(); i++)
	{
		oaValue = GetValueAt(i);
		if (oaValue != NULL)
			delete oaValue;
	}
}

KWObjectArrayValueBlock*
KWObjectArrayValueBlock::BuildBlockFromSparseValueVector(const KWObjectArrayValueSparseVector* valueSparseVector)
{
	KWObjectArrayValueBlock* resultValueBlock;
	int i;

	require(valueSparseVector != NULL);
	require(valueSparseVector->Check());

	// Rangement des index sparses et des valeurs dans leur ordre initial
	resultValueBlock = NewValueBlock(valueSparseVector->GetValueNumber());
	for (i = 0; i < valueSparseVector->GetValueNumber(); i++)
	{
		resultValueBlock->SetAttributeSparseIndexAt(i, valueSparseVector->GetSparseIndexAt(i));
		resultValueBlock->SetValueAt(i, valueSparseVector->GetValueAt(i));
	}
	ensure(resultValueBlock->GetValueNumber() == valueSparseVector->GetValueNumber());
	ensure(resultValueBlock->Check());
	return resultValueBlock;
}

KWObjectArrayValueBlock* KWObjectArrayValueBlock::ExtractBlockSubset(KWObjectArrayValueBlock* sourceValueBlock,
								     const IntVector* ivNewValueIndexes)
{
	KWObjectArrayValueBlock* valueBlock;
	int nNewBlockSize;
	int i;
	int nNewValueIndex;

	require(sourceValueBlock != NULL);
	require(ivNewValueIndexes != NULL);
	require(sourceValueBlock->GetValueNumber() == 0 or
		sourceValueBlock->GetAttributeSparseIndexAt(sourceValueBlock->GetValueNumber() - 1) <
		    ivNewValueIndexes->GetSize());

	// Calcul de la taille du nouveau bloc en comptant le nombre d'index a transferer
	nNewBlockSize = 0;
	for (i = 0; i < sourceValueBlock->GetValueNumber(); i++)
	{
		assert(i == 0 or sourceValueBlock->GetAttributeSparseIndexAt(i) >
				     sourceValueBlock->GetAttributeSparseIndexAt(i - 1));
		if (ivNewValueIndexes->GetAt(sourceValueBlock->GetAttributeSparseIndexAt(i)) >= 0)
			nNewBlockSize++;
	}

	// Creation du nouveau bloc
	valueBlock = NewValueBlock(nNewBlockSize);

	// Recopie de son contenu
	nNewBlockSize = 0;
	for (i = 0; i < sourceValueBlock->GetValueNumber(); i++)
	{
		nNewValueIndex = ivNewValueIndexes->GetAt(sourceValueBlock->GetAttributeSparseIndexAt(i));

		// Memorisation de la paire (index, valeur) si valide
		if (nNewValueIndex >= 0)
		{
			// Memorisation du tableau dans le nouveau bloc
			valueBlock->SetAttributeSparseIndexAt(nNewBlockSize, nNewValueIndex);
			valueBlock->SetValueAt(nNewBlockSize, sourceValueBlock->GetValueAt(i));
			assert(nNewBlockSize == 0 or valueBlock->GetAttributeSparseIndexAt(nNewBlockSize) >
							 valueBlock->GetAttributeSparseIndexAt(nNewBlockSize - 1));
			nNewBlockSize++;

			// On memorise NUL sur le bloc source
			sourceValueBlock->SetValueAt(i, NULL);
		}
	}
	return valueBlock;
}

KWObjectArrayValueBlock* KWObjectArrayValueBlock::Clone() const
{
	KWObjectArrayValueBlock* cloneValueBlock;
	int i;
	ObjectArray* oaSource;

	// Duplication du bloc et des index sparse
	cloneValueBlock = NewValueBlock(GetValueNumber());

	// Duplication des tableaux
	for (i = 0; i < GetValueNumber(); i++)
	{
		cloneValueBlock->SetAttributeSparseIndexAt(i, GetAttributeSparseIndexAt(i));
		oaSource = cast(ObjectArray*, GetValueAt(i));
		if (oaSource != NULL)
			cloneValueBlock->SetValueAt(i, oaSource->Clone());
	}
	return cloneValueBlock;
}

void KWObjectArrayValueBlock::Write(ostream& ost) const
{
	const int nMax = 10;
	int n;

	ost << GetClassLabel() << " [" << GetValueNumber() << "]:";
	for (n = 0; n < GetValueNumber(); n++)
	{
		ost << " (" << GetAttributeSparseIndexAt(n) << ":";
		ost << " [" << GetValueAt(n)->GetSize() << "])";
		if (n >= nMax)
		{
			ost << "...";
			break;
		}
	}
	cout << "\n";
}

longint KWObjectArrayValueBlock::GetUsedMemory() const
{
	longint lUsedMemory;
	int n;
	ObjectArray* oaValue;

	// Appel de la methode ancetre
	lUsedMemory = KWValueBlock::GetUsedMemory();

	// On ajout la place prise par les tableaux
	for (n = 0; n < GetValueNumber(); n++)
	{
		oaValue = GetValueAt(n);
		check(oaValue);
		lUsedMemory += oaValue->GetUsedMemory();
	}
	return lUsedMemory;
}

void KWObjectArrayValueBlock::Test()
{
	const int nKeyNumber = 1000;
	const int nMaxValueNumber = 10;
	KWClass* attributeClass;
	ObjectArray oaCreatedObjects;
	KWIndexedNKeyBlock indexedKeyBlock;
	KWObjectArrayValueSparseVector valueSparseVector;
	KWObjectArrayValueBlock* objectArrayValueBlock;
	ALString sTmp;
	int i;
	int nKey;
	ObjectArray* oaValue;
	int nFound;

	// Creation d'un bloc de cles indexe
	for (i = 0; i < nKeyNumber; i++)
	{
		nKey = i + 1;
		indexedKeyBlock.AddKey(nKey);
	}
	cout << indexedKeyBlock << endl;

	// Creation d'une classe de test
	attributeClass = KWClass::CreateClass("AttributeClass", 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, true, NULL);
	KWClassDomain::GetCurrentDomain()->InsertClass(attributeClass);
	KWClassDomain::GetCurrentDomain()->Compile();

	// Creation d'objets
	for (i = 0; i < nKeyNumber; i++)
		oaCreatedObjects.Add(KWObject::CreateObject(attributeClass, i + 1));

	// Creation d'un dictionnaire de valeurs
	for (i = 0; i < nMaxValueNumber; i++)
	{
		nKey = 10 * (i + 1);
		oaValue = new ObjectArray;
		oaValue->Add(oaCreatedObjects.GetAt(i));
		valueSparseVector.AddValueAt(nKey, oaValue);
	}
	cout << valueSparseVector << endl;

	// Initialisation du bloc de valeurs
	objectArrayValueBlock = KWObjectArrayValueBlock::BuildBlockFromSparseValueVector(&valueSparseVector);
	cout << objectArrayValueBlock << endl;

	// Recherche des valeurs dans le bloc
	cout << "Search values by variable index\n";
	nFound = 0;
	for (i = 0; i < nKeyNumber; i++)
	{
		oaValue = objectArrayValueBlock->GetValueAtAttributeSparseIndex(i);
		if (oaValue != NULL)
		{
			nFound++;
			if (nFound <= 10)
			{
				nKey = i + 1;
				cout << "\t" << i << "\t" << nKey << "\t[" << oaValue->GetSize() << "]\n";
			}
			else if (nFound == 11)
				cout << "\t...\n";
		}
	}
	assert(nFound == objectArrayValueBlock->GetValueNumber());

	// Destruction des objets
	oaCreatedObjects.DeleteAll();
	delete objectArrayValueBlock;

	// Destruction de toutes les classes enregistrees
	KWClassDomain::DeleteAllDomains();
}
