// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRString.h"

KWDREncrypt::KWDREncrypt()
{
	SetName("Encrypt");
	SetLabel("Encryption of a categorical value using an encryption key");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetSecondOperand()->SetType(KWType::Symbol);
	nEncryptionHashCode = -1;
}

KWDREncrypt::~KWDREncrypt() {}

KWDerivationRule* KWDREncrypt::Create() const
{
	return new KWDREncrypt;
}

Symbol KWDREncrypt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	ALString sStringToEncrypt;
	Symbol sKey;

	require(IsCompiled());

	// Recherche des parametres
	sStringToEncrypt = GetFirstOperandGenericSymbolValue(kwoObject);
	sKey = GetSecondOperand()->GetSymbolValue(kwoObject);

	return StringToSymbol(EncryptString(sStringToEncrypt, sKey));
}

const ALString KWDREncrypt::EncryptString(ALString& sStringToEncrypt, const Symbol& sKey) const
{
	ALString sEncryptString;
	IntVector ivStringChars;
	boolean bIsDigit;
	int nBlockSize;
	int nBlock;
	int nPad;
	int i;
	int nRandomSeed;
	int nCharCode;
	int nBlockCode;
	char cLastChar;

	// Memorisation de la graine aleatoire initiale
	nRandomSeed = GetRandomSeed();

	// Recalcul si necessaire des donnees de travail d'encryptage
	InitWorkingArrays(sKey);

	// Preprocessing de la chaine a encrypter
	sStringToEncrypt = PreprocessStringForEncryption(sStringToEncrypt);

	// Test du type de chaine
	// On codera les chaines numeriques de facon specifique (plus compacte)
	bIsDigit = IsDigitString(sStringToEncrypt);

	// On met les caracteres de la chaines dans un tableau
	ivStringChars.SetSize(sStringToEncrypt.GetLength());
	for (i = 0; i < sStringToEncrypt.GetLength(); i++)
		ivStringChars.SetAt(i, sStringToEncrypt.GetAt(i));

	// Padding de ce tableau pour obtenir un multiple de 3 pour les caracteres
	// standard, de 5 dans le cas numerique
	// On pad par des '0' qui conviennent dans le cas numerique ou non
	assert(pow(1.0 * ivDigitChars.GetSize(), 5) <= pow(1.0 * ivAlphanumChars.GetSize(), 3));
	if (bIsDigit)
		nBlockSize = 5;
	else
		nBlockSize = 3;
	nPad = ivStringChars.GetSize() % nBlockSize;
	nPad = (nBlockSize - nPad) % nBlockSize;
	if (ivStringChars.GetSize() == 0)
	{
		assert(not bIsDigit);
		nPad = nBlockSize;
	}
	for (i = 0; i < nPad; i++)
		ivStringChars.Add('0');
	assert(ivStringChars.GetSize() % nBlockSize == 0);

	// Permutation aleatoire de ce tableau
	// (determinee uniquement par la cle d'encryptage, donc inversible)
	SetRandomSeed(nEncryptionHashCode);
	ivStringChars.Shuffle();

	// Initialisation de la taille de la chaine a coder (optimisation memoire pour
	// n'allouer qu'une seule fois la memoire de la chaine, plutot que de proceder
	// par concatenations successives).
	sEncryptString.GetBufferSetLength(1 + 3 * (ivStringChars.GetSize() / nBlockSize));

	// Encryptage par paquets de nBlockSize
	// Chaque paquet de taille BlockSize est remplacer par un triplet de caracteres
	// cryptes.
	// (determinee par la cle d'encryptage et la longueur de la chaine a encrypter,
	// donc inversible par analyse du dernier caractere qui code le nombre de caracteres
	// de padding, donc la longueur initiale de la chaine)
	SetRandomSeed(nEncryptionHashCode * sStringToEncrypt.GetLength());
	for (nBlock = 0; nBlock < ivStringChars.GetSize() / nBlockSize; nBlock++)
	{
		// Calcul du code du block
		nBlockCode = 0;
		for (i = 0; i < nBlockSize; i++)
		{
			// Les codes de caracteres seront d'abord permuttes aleatoirement
			// (mais de facon deterministe) dans la methode GetCharCode
			nCharCode = GetCharCode(bIsDigit, ivStringChars.GetAt(nBlock * nBlockSize + i));
			nBlockCode = nBlockCode * GetMaxCharCode(bIsDigit) + nCharCode;
		}

		// Ajout des caracteres codant le bloc
		EncryptCharTriple(sEncryptString, nBlock * 3, nBlockCode);
	}

	// Ajout d'un dernier caractere pour la gestion correcte (unicite de codage)
	// du caractere numerique ou non et du padding
	cLastChar = GetLastEncryptChar(sStringToEncrypt, bIsDigit, nPad);
	sEncryptString.SetAt(sEncryptString.GetLength() - 1, cLastChar);

	// Re-encryptage de la fin de la chaine
	SetRandomSeed(nEncryptionHashCode);
	ReEncryptEndOfString(sEncryptString);

	// Restitution de la graine aleatoire initiale
	SetRandomSeed(nRandomSeed);

	return sEncryptString;
}

ALString KWDREncrypt::PreprocessStringForEncryption(const ALString& sStringToEncrypt) const
{
	ALString sPreprocessedString;
	int i;
	unsigned char c;

	sPreprocessedString.GetBuffer(sStringToEncrypt.GetLength() * 2);
	for (i = 0; i < sStringToEncrypt.GetLength(); i++)
	{
		// Transcodage des caracteres non encryptables
		c = sStringToEncrypt.GetAt(i);
		if (ivFirstPreprocessedChars.GetAt(c) != -1)
			sPreprocessedString += (char)ivFirstPreprocessedChars.GetAt(c);
		if (ivSecondPreprocessedChars.GetAt(c) != -1)
			sPreprocessedString += (char)ivSecondPreprocessedChars.GetAt(c);
	}
	return sPreprocessedString;
}

void KWDREncrypt::EncryptCharTriple(ALString& sEncryptString, int nEncryptPos, int nBlockCode) const
{
	int nEncryptBlockCode;
	int nCode1;
	int nCode2;
	int nCode3;
	int nMaxCode;

	require(0 <= nEncryptPos and nEncryptPos + 3 <= sEncryptString.GetLength());
	require(0 <= nBlockCode and nBlockCode < pow(1.0 * ivEncryptChars.GetSize(), 3));

	// Calcul du code crypte
	nEncryptBlockCode = ivEncryptCharTripleCodes.GetAt(nBlockCode);

	// Recherche des codes de chaque caracteres
	nMaxCode = ivEncryptChars.GetSize();
	nCode1 = nEncryptBlockCode / (nMaxCode * nMaxCode);
	nCode2 = (nEncryptBlockCode - nCode1 * nMaxCode * nMaxCode) / nMaxCode;
	nCode3 = nEncryptBlockCode - nCode1 * nMaxCode * nMaxCode - nCode2 * nMaxCode;
	assert(0 <= nCode1 and nCode1 < nMaxCode);
	assert(0 <= nCode2 and nCode2 < nMaxCode);
	assert(0 <= nCode3 and nCode3 < nMaxCode);

	// Positionnement du code complet resultat
	sEncryptString.SetAt(nEncryptPos, (char)ivEncryptChars.GetAt(nCode1));
	sEncryptString.SetAt(nEncryptPos + 1, (char)ivEncryptChars.GetAt(nCode2));
	sEncryptString.SetAt(nEncryptPos + 2, (char)ivEncryptChars.GetAt(nCode3));
}

char KWDREncrypt::GetLastEncryptChar(const ALString& sValue, boolean bIsDigit, int nPadNumber) const
{
	char cResult;
	int nHash;
	int nZoneMaxIndex;
	int nZoneIndex;
	int nZoneSize;

	require(0 <= nPadNumber and nPadNumber <= 4);

	// Hashage de la chaine
	nHash = GetStringHashCode(sValue);
	if (nHash < 0)
		nHash = -nHash;

	// La table des caracteres encryptes est divisee en autant de zones que necessaires
	nZoneMaxIndex = 10; // bIsDigit (2 valeurs) * nPadNumber (5 valeurs)
	nZoneSize = ivEncryptChars.GetSize() / nZoneMaxIndex;
	assert(nZoneSize >= 1);
	nZoneIndex = 2 * nPadNumber + (bIsDigit != 0);

	// Recherche d'un caractere (pseudo aleatoire) dans la zone
	cResult = (char)ivEncryptChars.GetAt(nZoneIndex * nZoneSize + (nHash % nZoneSize));
	return cResult;
}

void KWDREncrypt::ReEncryptEndOfString(ALString& sEncryptString) const
{
	int i;
	int nLength;
	int nBlockCode;
	int nCharCode;

	require(sEncryptString.GetLength() >= 3);

	// Prepropocessing du dernier block pour remplacer les backquotes par des blancs
	nLength = sEncryptString.GetLength();
	for (i = 0; i < 3; i++)
	{
		if (sEncryptString.GetAt(nLength - 3 + i) == '`')
			sEncryptString.SetAt(nLength - 3 + i, ' ');
	}

	// Calcul du code du block
	nBlockCode = 0;
	for (i = 0; i < 3; i++)
	{
		// Les codes de caracteres seront d'abord permuttes aleatoirement
		// (mais de facon deterministe) dans la methode GetCharCode
		nCharCode = GetCharCode(false, sEncryptString.GetAt(nLength - 3 + i));
		nBlockCode = nBlockCode * GetMaxCharCode(false) + nCharCode;
	}

	// Ajout des caracteres codant le bloc
	EncryptCharTriple(sEncryptString, nLength - 3, nBlockCode);
}

boolean KWDREncrypt::IsDigitString(const ALString& sValue) const
{
	int i;

	for (i = 0; i < sValue.GetLength(); i++)
	{
		if (not isdigit(sValue.GetAt(i)))
			return false;
	}
	return sValue.GetLength() > 0;
}

int KWDREncrypt::GetCharCode(boolean bIsDigit, int nChar) const
{
	int nCode;

	// On calcule d'abord l'index du caractere dans son tableau de caracateres codables
	// (numeriques, ou imprimables)
	nCode = bIsDigit ? ivDigitCharIndexes.GetAt(nChar) : ivAlphanumCharIndexes.GetAt(nChar);

	// On le permute aleatoirement (la permutation est inversible, car l'ordre des appels
	// a GetCharCode est deterministe pour une graine aleatoire initiale donnee
	nCode = (nCode + RandomInt(GetMaxCharCode(bIsDigit))) % GetMaxCharCode(bIsDigit);
	return nCode;
}

int KWDREncrypt::GetMaxCharCode(boolean bIsDigit) const
{
	assert(ivAlphanumChars.GetSize() == ivEncryptChars.GetSize());
	return bIsDigit ? ivDigitChars.GetSize() : ivAlphanumChars.GetSize();
}

void KWDREncrypt::InitWorkingArrays(const Symbol& sKey) const
{
	IntVector ivPureAlphanumChars;
	IntVector ivPrintableNonAlphanumChars;
	IntVector ivPrintableNonAlphanumCharIndexes;
	int nRandomSeed;
	int i;
	int c;
	int cIndex;
	ALString sTmp;

	// La chaine a encrypter subit d'abord un preprocessing: les caracteres
	// non imprimables sont rempaces par blanc, puis les caracteres imprimables
	// non alphanumeriques sont codes sur deux caracteres alphanumeriques.
	// La chaine ainsi preparee devient encodable.
	// Les caracteres sont codables de deux facon differentes selon qu'il
	// sont numeriques ou alphanumeriques..
	// Les caracteres numeriques (au nombre de 10) sont ranges dans le tableau
	// ivDigitChars, permuttes aleatoirement selon une premiere cle depandant de
	// la cle de codage, et leur index dans ivDigitChars (de 0 a 9) est calcule
	// dans ivDigitCharIndexes.
	// Les caracteres alphanumeriques (isalnum et underscore) (au nombre de 63)
	// sont ranges dans le tableau ivAlphanumChars, permuttes aleatoirement
	// selon une deuxieme cle depandant de la cle de codage, et leur index dans
	// ivDigitChars (de 0 a 93) est calcule dans ivAlphanumCharIndexes.
	// Les caracteres de codage (alphabet servant de base a la codification)
	// (caracteres alphanumeriques) (au nombre de 63) sont ranges dans
	// ivEncryptChars) et permuttes aleatoirement selon une troisieme cle
	// dependant de la cle de codage.
	// Le tableau ivEncryptCharTripleCodes contient tous les triplets de codes
	// possibles (nCode1*63*63 + nCode2*63 + nCode3) permettant d'indexer les
	// caracteres de cryptage. Il est permutte aleatoirement selon une quatrieme
	// cle dependant de la cle de codage.
	// En Remarquant que 10^5 <= 63^3, on peut coder les chaines numeriques de
	// facon plus compactes, en remplacant des blocs de 5 chiffres par des
	// block de 3 caracteres codes (pourvu que l'on puisse differencier les
	// codages de chaines numeriques des autres codages).
	// Un cinquieme cle depandant de la cle de codage est enfin fabriquee. Elle
	// permettra de melanger la chaine initiale a coder avant son encryptage.

	// Si tables de travail deja initialisees pour cette cle, on ne fait rien
	// Test optimise si sKey + "Encryption" == sEncryptionKey
	assert(strlen("Encryption") == 10);
	if (sKey.GetLength() + 10 == sEncryptionKey.GetLength() and
	    strncmp(sKey, sEncryptionKey, sKey.GetLength()) == 0)
		return;

	// Memorisation de la graine aleatoire
	nRandomSeed = GetRandomSeed();

	// Reinitialisation des tableaux
	ivDigitChars.SetSize(0);
	ivAlphanumChars.SetSize(0);
	ivDigitCharIndexes.SetSize(0);
	ivAlphanumCharIndexes.SetSize(0);
	ivEncryptChars.SetSize(0);
	ivEncryptCharTripleCodes.SetSize(0);

	// Calcul du tableau des caracteres alphanumn, imprimables non alphanumeriques
	for (i = 0; i < 128; i++)
	{
		if (isalnum(i))
			ivPureAlphanumChars.Add(i);
		if (isprint(i) and not isalnum(i))
			ivPrintableNonAlphanumChars.Add(i);
	}

	// Tableau des index des caracteres imprimables non alphanumeriques
	ivPrintableNonAlphanumCharIndexes.SetSize(256);
	for (i = 0; i < ivPrintableNonAlphanumCharIndexes.GetSize(); i++)
		ivPrintableNonAlphanumCharIndexes.SetAt(i, -1);
	for (i = 0; i < ivPrintableNonAlphanumChars.GetSize(); i++)
		ivPrintableNonAlphanumCharIndexes.SetAt(ivPrintableNonAlphanumChars.GetAt(i), i);

	// Initialisation des tableaux des paires de caracteres remplacant les
	// caracteres originaux (servant d'index) pour le preprocessing (index=-1: rien)
	ivFirstPreprocessedChars.SetSize(256);
	ivSecondPreprocessedChars.SetSize(256);
	for (i = 0; i < 256; i++)
	{
		// Les caracteres alphanumeriques sont encodes tels quels
		if (i < 128 and isalnum(i))
		{
			ivFirstPreprocessedChars.SetAt(i, -1);
			ivSecondPreprocessedChars.SetAt(i, i);
		}
		else
		// Les autres caracteres imprimable sont codes sur deux caracteres:
		// un underscore (caractere d'echappement) suivi d'un caractere
		// alphanumerique
		{
			// Caractere non imprimable transforme en blanc
			c = i;
			if (c >= 128 or not isprint(c))
				c = ' ';

			// Prefixe underscore rajoute
			ivFirstPreprocessedChars.SetAt(i, '_');

			// Recherche de l'index du caractere imprimable non alphanumerique
			cIndex = ivPrintableNonAlphanumCharIndexes.GetAt(c);
			assert(cIndex != -1);
			assert(0 <= cIndex and cIndex < ivPureAlphanumChars.GetSize());

			// On met comme second caractere le caractere alphanumerique de meme index
			ivSecondPreprocessedChars.SetAt(i, ivPureAlphanumChars.GetAt(cIndex));
		}
	}

	// Initialisation du tableau des digits
	for (i = 0; i < 128; i++)
	{
		if (isdigit(i))
			ivDigitChars.Add(i);
	}

	// Initialisation du tableau des caracteres imprimables
	for (i = 0; i < 128; i++)
	{
		if (isalnum(i) or i == '_')
			ivAlphanumChars.Add(i);
	}

	// Initialisation du tableau des caracteres cryptes
	for (i = 0; i < 128; i++)
	{
		if (isalnum(i) or i == '_')
			ivEncryptChars.Add(i);
	}
	assert(ivAlphanumChars.GetSize() == ivEncryptChars.GetSize());

	// Initialisation des codes de triplets de caracteres cryptes
	ivEncryptCharTripleCodes.SetSize(ivEncryptChars.GetSize() * ivEncryptChars.GetSize() *
					 ivEncryptChars.GetSize());
	for (i = 0; i < ivEncryptCharTripleCodes.GetSize(); i++)
		ivEncryptCharTripleCodes.SetAt(i, i);

	// Permutations aleatoires des tableaux
	SetRandomSeed(GetStringHashCode(sEncryptionKey + "DigitChars"));
	ivDigitChars.Shuffle();
	SetRandomSeed(GetStringHashCode(sEncryptionKey + "AlphanumChars"));
	ivAlphanumChars.Shuffle();
	SetRandomSeed(GetStringHashCode(sEncryptionKey + "EncryptChars"));
	ivEncryptChars.Shuffle();
	SetRandomSeed(GetStringHashCode(sEncryptionKey + "EncryptCharTripleCodes"));
	ivEncryptCharTripleCodes.Shuffle();

	// Tableau des index des digits (calcule a partir du tableau des digits)
	ivDigitCharIndexes.SetSize(256);
	for (i = 0; i < ivDigitCharIndexes.GetSize(); i++)
		ivDigitCharIndexes.SetAt(i, -1);
	for (i = 0; i < ivDigitChars.GetSize(); i++)
		ivDigitCharIndexes.SetAt(ivDigitChars.GetAt(i), i);

	// Tableau des index des alphanum (calcul similaire)
	ivAlphanumCharIndexes.SetSize(256);
	for (i = 0; i < ivAlphanumCharIndexes.GetSize(); i++)
		ivAlphanumCharIndexes.SetAt(i, -1);
	for (i = 0; i < ivAlphanumChars.GetSize(); i++)
		ivAlphanumCharIndexes.SetAt(ivAlphanumChars.GetAt(i), i);

	// Memorisation de la cle d'encryptage, et du cle de hashage
	sEncryptionKey = sTmp + sKey + "Encryption";
	nEncryptionHashCode = GetStringHashCode(sEncryptionKey + "ShuffleValues");

	// Restitution de la graine aleatoire
	SetRandomSeed(nRandomSeed);
}

int KWDREncrypt::GetStringHashCode(const ALString& sValue) const
{
	int nHash;
	int i;

	// Calcul classique du fonction de hashage des chaines de caracteres
	nHash = 0;
	for (i = 0; i < sValue.GetLength(); i++)
		nHash = (nHash << 5) + nHash + sValue.GetAt(i);
	return nHash;
}