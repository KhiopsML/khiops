// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "ALString.h"

// Pour une chaine vide, les donnees vont pointer ici
// Cela permet d'eviter de nombreuxtest de pointeur à NULL lors de
// l'appel des fonction de la librairie C standard
char ALSCHARNIL = '\0';

extern const ALString ALSEmptyString;

// Pour la gestion des chaines vides
const ALString ALSEmptyString;

ALString::ALString(const ALString& stringSrc)
{
	stringSrc.AllocCopy(*this, stringSrc.nDataLength, 0, 0);
}

// Portage Unix : etait en inline avant
void ALString::SafeDelete(char* pch)
{
	if (pch != (char*)&ALSCHARNIL)
		DeleteCharArray(pch);
}

// Portage unix : etait en inline avant
ALString::~ALString()
{
	SafeDelete(pchData);
}

void ALString::AllocBuffer(int nLen)
{
	require(nLen >= 0);
	require(nLen <= INT_MAX - 1);

	if (nLen == 0)
	{
		Init();
	}
	else
	{
		// On aloue toujours un caractere supplementaire pour le caractere de terminaison '\0'
		pchData = NewCharArray(nLen + 1);
		pchData[nLen] = '\0';
		nDataLength = nLen;
		nAllocLength = nLen;
	}
}

void ALString::Empty()
{
	SafeDelete(pchData);
	Init();
	assert(nDataLength == 0);
	assert(nAllocLength == 0);
}

void ALString::AllocCopy(ALString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const
{
	int nNewLen = nCopyLen + nExtraLen;

	// Duplication des donnnes en allouant des caracteres supplementaires
	// Copie d'une partie des donnees initiales au debut de la nouvelle chaine de caracteres
	if (nNewLen == 0)
	{
		dest.Init();
	}
	else
	{
		dest.AllocBuffer(nNewLen);
		memcpy(dest.pchData, &pchData[nCopyIndex], nCopyLen);
	}
}

ALString::ALString(const char* psz)
{
	int nLen;

	if ((nLen = SafeStrlen(psz)) == 0)
		Init();
	else
	{
		AllocBuffer(nLen);
		memcpy(pchData, psz, nLen);
	}
}

void ALString::AssignCopy(int nSrcLen, const char* pszSrcData)
{
	// Allocation d'un nouveau buffer si necessairte
	if (nSrcLen > nAllocLength)
	{
		Empty();
		AllocBuffer(nSrcLen);
	}
	if (nSrcLen != 0)
		memcpy(pchData, pszSrcData, nSrcLen);
	nDataLength = nSrcLen;

	// Attention au cas particulier d'une chaine vide
	if (nAllocLength != 0)
		pchData[nSrcLen] = '\0';
	else
		pchData = (char*)&ALSCHARNIL;
}

const ALString& ALString::operator=(const ALString& stringSrc)
{
	AssignCopy(stringSrc.nDataLength, stringSrc.pchData);
	return *this;
}

const ALString& ALString::operator=(const char* psz)
{
	AssignCopy(SafeStrlen(psz), psz);
	return *this;
}

void ALString::ConcatCopy(int nSrc1Len, const char* pszSrc1Data, int nSrc2Len, const char* pszSrc2Data)
{
	int nNewLen = nSrc1Len + nSrc2Len;

	// Concatenation
	AllocBuffer(nNewLen);
	memcpy(pchData, pszSrc1Data, nSrc1Len);
	memcpy(&pchData[nSrc1Len], pszSrc2Data, nSrc2Len);
}

ALString operator+(const ALString& string1, const ALString& string2)
{
	ALString s;
	s.ConcatCopy(string1.nDataLength, string1.pchData, string2.nDataLength, string2.pchData);
	return s;
}

ALString operator+(const ALString& string, const char* psz)
{
	ALString s;
	s.ConcatCopy(string.nDataLength, string.pchData, ALString::SafeStrlen(psz), psz);
	return s;
}

ALString operator+(const char* psz, const ALString& string)
{
	ALString s;
	s.ConcatCopy(ALString::SafeStrlen(psz), psz, string.nDataLength, string.pchData);
	return s;
}

char* ALString::GetBuffer(int nMinBufLength)
{
	require(nMinBufLength >= 0);

	if (nMinBufLength > nAllocLength)
	{
		// On doit agrandir le buffer
		char* pszOldData = pchData;
		int nOldLen = nDataLength;

		AllocBuffer(nMinBufLength);
		memcpy(pchData, pszOldData, nOldLen);
		nDataLength = nOldLen;
		pchData[nDataLength] = '\0';
		SafeDelete(pszOldData);
	}

	// On retourne un pointeur sur le buffer
	assert(pchData != NULL);
	return pchData;
}

void ALString::ReleaseBuffer(int nNewLength)
{
	if (nNewLength == -1)
	{
		assert(strlen(pchData) <= INT_MAX);
		nNewLength = (int)strlen(pchData); // Chaine terminee par zero
	}
	assert(nNewLength <= nAllocLength);
	nDataLength = nNewLength;
	pchData[nDataLength] = '\0';
}

char* ALString::GetBufferSetLength(int nNewLength)
{
	require(nNewLength >= 0);

	GetBuffer(nNewLength);
	nDataLength = nNewLength;
	pchData[nDataLength] = '\0';
	return pchData;
}

int ALString::Find(char ch) const
{
	char* psz;

	// Recherche le premier caractere
	psz = strchr(pchData, ch);

	// Retourne -1 si non trouve, sa position sinon
	return (psz == NULL) ? -1 : (int)(psz - pchData);
}

int ALString::FindOneOf(const char* pszCharSet) const
{
	require(pszCharSet != NULL);
	{
		char* psz = (char*)strpbrk(pchData, pszCharSet);
		return (psz == NULL) ? -1 : (int)(psz - pchData);
	}
}

void ALString::ConcatInPlace(int nSrcLen, const char* pszSrcData)
{
	//  Methode principale pour les operateurs +=

	// Allocation d'un nouveau buffer s'il est trop petit
	// La taille est doublee, ce qui optimise les boucles de concatenation par l'es operateur +=
	if (nDataLength + nSrcLen > nAllocLength)
		GetBuffer(2 * (nDataLength + nSrcLen));

	// Cocatenation rapide
	memcpy(&pchData[nDataLength], pszSrcData, nSrcLen);
	nDataLength += nSrcLen;
	assert(nDataLength <= nAllocLength);
	pchData[nDataLength] = '\0';
}

const ALString& ALString::operator+=(const char* psz)
{
	ConcatInPlace(SafeStrlen(psz), psz);
	return *this;
}

const ALString& ALString::operator+=(char ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

const ALString& ALString::operator+=(const ALString& string)
{
	ConcatInPlace(string.nDataLength, string.pchData);
	return *this;
}

ALString::ALString(char ch, int nLength)
{
	require(nLength >= 0);
	if (nLength < 1)
		Init();
	else
	{
		AllocBuffer(nLength);
		memset(pchData, ch, nLength);
	}
}

ALString::ALString(const char* pch, int nLength)
{
	if (nLength == 0)
		Init();
	else
	{
		require(pch != NULL);
		AllocBuffer(nLength);
		memcpy(pchData, pch, nLength);
	}
}

const ALString& ALString::operator=(char ch)
{
	AssignCopy(1, &ch);
	return *this;
}

ALString operator+(const ALString& string1, char ch)
{
	ALString s;
	s.ConcatCopy(string1.nDataLength, string1.pchData, 1, &ch);
	return s;
}

ALString operator+(char ch, const ALString& string)
{
	ALString s;
	s.ConcatCopy(1, &ch, string.nDataLength, string.pchData);
	return s;
}

ALString ALString::Mid(int nFirst) const
{
	require(0 <= nFirst and nFirst <= nDataLength);
	return Mid(nFirst, nDataLength - nFirst);
}

ALString ALString::Mid(int nFirst, int nCount) const
{
	ALString dest;

	require(nFirst >= 0);
	require(nCount >= 0);
	require(nFirst + nCount <= nDataLength);

	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

ALString ALString::Right(int nCount) const
{
	ALString dest;

	require(nCount >= 0);
	require(nCount <= nDataLength);

	AllocCopy(dest, nCount, nDataLength - nCount, 0);
	return dest;
}

ALString ALString::Left(int nCount) const
{
	ALString dest;

	require(nCount >= 0);
	require(nCount <= nDataLength);

	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

ALString ALString::SpanIncluding(const char* pszCharSet) const
{
	require(pszCharSet != NULL);
	assert(strspn(pchData, pszCharSet) <= INT_MAX);
	return Left((int)strspn(pchData, pszCharSet));
}

ALString ALString::SpanExcluding(const char* pszCharSet) const
{
	require(pszCharSet != NULL);
	assert(strcspn(pchData, pszCharSet) <= INT_MAX);
	return Left((int)strcspn(pchData, pszCharSet));
}

int ALString::ReverseFind(char ch) const
{
	char* psz;
	psz = (char*)strrchr(pchData, ch);

	return (psz == NULL) ? -1 : (int)(psz - pchData);
}

int ALString::Find(const char* pszSub) const
{
	require(pszSub != NULL);
	char* psz;

	psz = (char*)strstr(pchData, pszSub);

	// Retounre -1 si non trouve, la position trouvee sinon
	return (psz == NULL) ? -1 : (int)(psz - pchData);
}

void ALString::Trim()
{
	TrimLeft();
	TrimRight();
}

void ALString::TrimRight()
{
	// On part de la fin de la chaine
	char* lpszLast = pchData + nDataLength;
	assert(*lpszLast == '\0');

	// On parcours la chaine a l'envers, en la racourcissant tant que l'on est sur un caractere blanc
	while (lpszLast > pchData)
	{
		assert(nDataLength > 0);
		lpszLast--;
		if (iswspace(*lpszLast))
			*lpszLast = '\0';
		else
		{
			lpszLast++;
			break;
		}
	}

	// On met a jour la nouvelle fin de chaine
	nDataLength = (int)(lpszLast - pchData);
	assert(0 <= nDataLength and nDataLength <= nAllocLength);
}

void ALString::TrimLeft()
{
	int nLength;

	// Recherche du premier caractere non espace
	const char* lpsz = pchData;
	while (iswspace(*lpsz))
		lpsz = lpsz + 1;

	// Mise a jour des donnnes et de la longueur
	nLength = nDataLength - (int)(lpsz - pchData);
	memmove(pchData, lpsz, (nLength + 1) * sizeof(char));
	nDataLength = nLength;
}

/////////////////////////////////////////////////////////////////////////////
// Non ANSI

int ALString::CompareNoCase(const char* psz) const
{
	ALString sFirst;
	ALString sSecond;

	// Comparaison apres mise en majuscule
	sFirst = pchData;
	sFirst.MakeUpper();
	sSecond = psz;
	sSecond.MakeUpper();

	// Non ANSI: return stricmp(pchData, psz);
	return sFirst.Compare(sSecond);
}

void ALString::MakeReverse()
{
	char cBuffer;
	int i;

	assert(strlen(pchData) <= INT_MAX);
	assert(nDataLength == SafeStrlen(pchData));
	for (i = 0; i < nDataLength / 2; i++)
	{
		cBuffer = pchData[i];
		pchData[i] = pchData[nDataLength - 1 - i];
		pchData[nDataLength - 1 - i] = cBuffer;
	}
}

//////////////////////////////////////////////////////////////////////////////
// Test

void ALString::Test()
{
	ALString sTest;
	ALString sRef;
	ALString* sTestAlloc;
	ALString* sTestComp;
	int nI, nJ, nNb;
	int nStartClock;
	int nStopClock;
	int nCompare;
	longint lValue;
	double dValue;
	longint lTest;
	longint lCompareOk;

	// Conversions
	cout << "1. Chaine variable locale initiale: " << sTest << "\n";
	sTest = "Hello";
	cout << "2. Chaine initialisee a partir d'une constante: " << sTest << "\n";
	sTest += " World!";
	cout << "3. Chaine concatenee a partir d'une constante: " << sTest << "\n";
	sTest = sTest + sTest;
	cout << "4. Chaine concatenee a partir d'une chaine: " << sTest << "\n";
	sRef = sTest;
	sTest = sRef;
	cout << "5. Chaine initialisee a partir d'une chaine: " << sTest << "\n";
	sTestAlloc = new ALString;
	cout << "6. Chaine allouee par new: " << *sTestAlloc << "\n";
	*sTestAlloc = sTest;
	cout << "7. Chaine allouee initialisee par une chaine de la stack: " << *sTestAlloc << "\n";

	// Conversion vers les types numeriques
	lValue = 1;
	cout << "i\tll int\tdouble\t(ll)double\tstring(ll)\t(ll)string" << endl;
	for (nI = 0; nI < 19; nI++)
	{
		dValue = (double)lValue;
		sTest = LongintToString(lValue);
		lTest = StringToLongint(sTest);
		cout << nI << "\t" << lValue << "\t" << dValue << "\t" << (longint)dValue << "\t" << sTest << "\t"
		     << lTest << endl;
		lValue *= 10;
	}

	// Conversion de double vers string
	cout << "Test de conversion de double vers string" << endl;
	dValue = 1;
	for (nI = 0; nI < 20; nI++)
	{
		sTest = DoubleToString(dValue);
		cout << nI << "\t" << dValue << "\t" << sTest << endl;
		dValue /= 10;
	}

	// Divers
	sRef = "AbCdE";
	sTest = sRef;
	//
	sTest.MakeUpper();
	cout << "<" << sRef << "> -Upper-> <" << sTest << ">\n";
	sTest = sRef;
	//
	sTest.MakeLower();
	cout << "<" << sRef << "> -Lower-> <" << sTest << ">\n";
	//
	sTest.MakeUpper();

	// Comparaison: on normalise sur -1, 0, 1 pour ne pas dependre d'une implementation de compilateur
	nCompare = sRef.Compare(sTest);
	if (nCompare > 0)
		nCompare = 1;
	else if (nCompare < 0)
		nCompare = -1;
	cout << "<" << sRef << "> Compare <" << sTest << "> -> " << nCompare << "\n";
	nCompare = sRef.CompareNoCase(sTest);
	if (nCompare > 0)
		nCompare = 1;
	else if (nCompare < 0)
		nCompare = -1;
	cout << "<" << sRef << "> CompareNoCase <" << sTest << "> -> " << nCompare << "\n";
	cout << "Utilisation de cout avec une string: " << sRef << "!" << endl;
	//
	sRef = "  Coucou  ";
	sTest = sRef;
	sTest.TrimRight();
	sTest.TrimLeft();
	cout << "<" << sRef << "> -trim-> <" << sTest << ">\n";
	//
	sRef = "   ";
	sTest = sRef;
	sTest.TrimRight();
	sTest.TrimLeft();
	cout << "<" << sRef << "> -trim-> <" << sTest << ">\n";

	// Test de de concatenation
	cout << "\nTest de concatenation\n";
	nNb = AcquireRangedInt("Nombre de milliers d'iterations (concatenations)", 1, 1000000, 1000);
	//
	sTest = "";
	sRef = "1";
	cout << "\tStart (concatenations)\n";
	nStartClock = clock();
	for (nI = 0; nI < nNb; nI++)
	{
		sTest = "";
		for (nJ = 0; nJ < 1000; nJ++)
		{
			sTest += sRef;
			sTest += "2";
			sTest += '3';
		}
	}
	nStopClock = clock();
	cout << "\tResult length: " << sTest.GetLength();
	if (sTest.GetLength() <= 30)
		cout << " (" << sTest << ")\n";
	else
		cout << " (" << sTest.Right(30) << "...)\n";
	cout << "SYS TIME\tString concatenations\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Test de performance
	cout << "\nTest de performance\n";
	sTestComp = new ALString;
	*sTestComp = *sTestAlloc;
	nNb = AcquireRangedInt("Nombre de milliers d'iterations (comparaison de chaines)", 1, 1000000, 1000);
	//
	cout << "\tStart (comparaison de valeurs des chaines)\n";
	nStartClock = clock();
	lCompareOk = 0;
	for (nI = 0; nI < nNb; nI++)
	{
		for (nJ = 0; nJ < 1000; nJ++)
		{
			if (*sTestComp == *sTestAlloc)
				lCompareOk++;
		}
	}
	nStopClock = clock();
	cout << "SYS TIME\tString comparisons\t" << lCompareOk << "\t"
	     << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";
	//
	cout << "\tStart (comparaison des pointeurs des chaines)\n";
	nStartClock = clock();
	lCompareOk = 0;
	for (nI = 0; nI < nNb; nI++)
	{
		for (nJ = 0; nJ < 1000; nJ++)
		{
			if (sTestComp == sTestAlloc)
				lCompareOk++;
		}
	}
	nStopClock = clock();
	cout << "SYS TIME\tString pointer comparisons\t" << lCompareOk << "\t"
	     << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	delete sTestAlloc;
	delete sTestComp;
}