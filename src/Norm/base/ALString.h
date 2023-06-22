// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"

/////////////////////////////////////////////////////////////////////////////
// Classe de gestion des chaines de caracteres                             //
// Fonctionnalites de concatenation, comparaison, affectation              //
// Interfacage efficace automatique avec les routines C attendant un char* //
/////////////////////////////////////////////////////////////////////////////
class ALString : public SystemObject
{
public:
	// Constructeurs
	ALString();
	ALString(const ALString& stringSrc);
	ALString(char ch, int nRepeat = 1);
	ALString(const char* psz);
	ALString(const char* pch, int nLength);
	~ALString();

	/////////////////////////////////
	// Attributs et operations

	// Chaine comme tableau de caracteres
	int GetLength() const;
	int IsEmpty() const;
	void Empty();                      // Nettoyage
	char GetAt(int nIndex) const;      // Commence a 0
	char operator[](int nIndex) const; // Comme GetAt
	void SetAt(int nIndex, char ch);
	operator const char*() const; // C string
	operator char*() const;       // C string

	// Operateur d'affectation
	const ALString& operator=(const ALString& stringSrc);
	const ALString& operator=(char ch);
	const ALString& operator=(const char* psz);

	// Concatenation
	const ALString& operator+=(const ALString& string);
	const ALString& operator+=(char ch);
	const ALString& operator+=(const char* psz);
	friend ALString operator+(const ALString& string1, const ALString& string2);
	friend ALString operator+(const ALString& string, char ch);
	friend ALString operator+(char ch, const ALString& string);
	friend ALString operator+(const ALString& string, const char* psz);
	friend ALString operator+(const char* psz, const ALString& string);

	// Comparaison
	int Compare(const char* psz) const;
	int CompareNoCase(const char* psz) const;

	// Extraction de sous-chaine
	ALString Mid(int nFirst, int nCount) const;
	ALString Mid(int nFirst) const;
	ALString Left(int nCount) const;
	ALString Right(int nCount) const;

	// Sous-chaine du debut incluant/excluant les caracteres d'un ensemble de caracteres
	ALString SpanIncluding(const char* pszCharSet) const;
	ALString SpanExcluding(const char* pszCharSet) const;

	// Mise en majuscule/minuscile et inversion
	void MakeUpper();
	void MakeLower();
	void MakeReverse();

	// Nettoyage des blancs
	void Trim();
	void TrimRight();
	void TrimLeft();

	// Recherche d'un caracteres (retourne l'index de debut, ou -1 si non trouve)
	// look for a single character match
	int Find(char ch) const; // cf. "C" strchr
	int ReverseFind(char ch) const;
	int FindOneOf(const char* pszCharSet) const;

	// Recherche d'une sous-chaine
	int Find(const char* pszSub) const; // cf. "C" strstr

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const;

	// Access a l'implementation (buffer) sous forme d'une tableau "C" de caracteres
	// Si nNewLength vaut -1, on positionne la taille sur celle de la string (terminee par '\0'=
	char* GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	char* GetBufferSetLength(int nNewLength);

	// Methode de test globale
	static void Test();

	////////////////////////////////////////////////////////////////////
	// Implementation
public:
	int GetAllocLength() const;

	// Constructeur et affectation de deplacement, pour l'optimisation de la
	// gestion des variables temporaires par le compilateur
#if not defined __UNIX__ or defined __C11__
	ALString(ALString&& stringSrc) noexcept;
	ALString& operator=(ALString&& stringSrc) noexcept;
#endif // not defined __UNIX__ or defined __C11__

protected:
	// Longueur/tailles en caracteres (note: un caractere supplementaire est toujours alloue)
	// En principe, la longueur d'un string est de type size_t (impact notamment en 64 bits)
	// On impose neanmoins qu'elle soit toujours inferieure a INT_MAX, ce qui d'une part permet
	// des chaines de tres grandes taille, d'autre part permet d'utiliser le type int de facon
	// generique dans toute la bibliotheque
	char* pchData;    // chaines de caracteres (zero terminated)
	int nDataLength;  // ne contient pas le 0 de terminaison
	int nAllocLength; // ne contient pas le 0 de terminaison

	// Methodes internes
	void Init();
	void AllocCopy(ALString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, const char* pszSrcData);
	void ConcatCopy(int nSrc1Len, const char* pszSrc1Data, int nSrc2Len, const char* pszSrc2Data);
	void ConcatInPlace(int nSrcLen, const char* pszSrcData);
	static void SafeDelete(char* pch);
	static int SafeStrlen(const char* psz);
};

// Fonctions de comparaison
int operator==(const ALString& s1, const ALString& s2);
int operator==(const ALString& s1, const char* s2);
int operator==(const char* s1, const ALString& s2);
int operator!=(const ALString& s1, const ALString& s2);
int operator!=(const ALString& s1, const char* s2);
int operator!=(const char* s1, const ALString& s2);
int operator<(const ALString& s1, const ALString& s2);
int operator<(const ALString& s1, const char* s2);
int operator<(const char* s1, const ALString& s2);
int operator>(const ALString& s1, const ALString& s2);
int operator>(const ALString& s1, const char* s2);
int operator>(const char* s1, const ALString& s2);
int operator<=(const ALString& s1, const ALString& s2);
int operator<=(const ALString& s1, const char* s2);
int operator<=(const char* s1, const ALString& s2);
int operator>=(const ALString& s1, const ALString& s2);
int operator>=(const ALString& s1, const char* s2);
int operator>=(const char* s1, const ALString& s2);

// Implementations en inline

inline int ALString::GetLength() const
{
	assert(nDataLength <= INT_MAX);
	return nDataLength;
}
inline int ALString::GetAllocLength() const
{
	assert(nAllocLength <= INT_MAX);
	return nAllocLength;
}
inline int ALString::IsEmpty() const
{
	return nDataLength == 0;
}
inline ALString::operator const char*() const
{
	return (const char*)pchData;
}
inline ALString::operator char*() const
{
	return (char*)pchData;
}
inline int ALString::SafeStrlen(const char* psz)
{
	if (psz == NULL)
		return 0;
	else
	{
		assert(strlen(psz) <= INT_MAX);
		// Le type size_t retournee par strlen est plus grand que int
		// mais on impose et on s'assure que la taille du char* ne depasse pas INT_MAX
		return (int)strlen(psz);
	}
}
inline int ALString::Compare(const char* psz) const
{
	return strcmp(pchData, psz);
}
inline void ALString::MakeUpper()
{
	assert(nDataLength == SafeStrlen(pchData));
	for (int i = 0; i < nDataLength; i++)
		pchData[i] = (char)toupper(pchData[i]);
}
inline void ALString::MakeLower()
{
	assert(nDataLength == SafeStrlen(pchData));
	for (int i = 0; i < nDataLength; i++)
		pchData[i] = (char)tolower(pchData[i]);
}
inline char ALString::GetAt(int nIndex) const
{
	require(nIndex >= 0);
	require(nIndex < nDataLength);

	return pchData[nIndex];
}
inline char ALString::operator[](int nIndex) const
{
	require(nIndex >= 0);
	require(nIndex < nDataLength);

	return pchData[nIndex];
}
inline void ALString::SetAt(int nIndex, char ch)
{
	require(nIndex >= 0);
	require(nIndex < nDataLength);
	require(ch != 0);

	pchData[nIndex] = ch;
}
inline int operator==(const ALString& s1, const ALString& s2)
{
	return s1.Compare(s2) == 0;
}
inline int operator==(const ALString& s1, const char* s2)
{
	return s1.Compare(s2) == 0;
}
inline int operator==(const char* s1, const ALString& s2)
{
	return s2.Compare(s1) == 0;
}
inline int operator!=(const ALString& s1, const ALString& s2)
{
	return s1.Compare(s2) != 0;
}
inline int operator!=(const ALString& s1, const char* s2)
{
	return s1.Compare(s2) != 0;
}
inline int operator!=(const char* s1, const ALString& s2)
{
	return s2.Compare(s1) != 0;
}
inline int operator<(const ALString& s1, const ALString& s2)
{
	return s1.Compare(s2) < 0;
}
inline int operator<(const ALString& s1, const char* s2)
{
	return s1.Compare(s2) < 0;
}
inline int operator<(const char* s1, const ALString& s2)
{
	return s2.Compare(s1) > 0;
}
inline int operator>(const ALString& s1, const ALString& s2)
{
	return s1.Compare(s2) > 0;
}
inline int operator>(const ALString& s1, const char* s2)
{
	return s1.Compare(s2) > 0;
}
inline int operator>(const char* s1, const ALString& s2)
{
	return s2.Compare(s1) < 0;
}
inline int operator<=(const ALString& s1, const ALString& s2)
{
	return s1.Compare(s2) <= 0;
}
inline int operator<=(const ALString& s1, const char* s2)
{
	return s1.Compare(s2) <= 0;
}
inline int operator<=(const char* s1, const ALString& s2)
{
	return s2.Compare(s1) >= 0;
}
inline int operator>=(const ALString& s1, const ALString& s2)
{
	return s1.Compare(s2) >= 0;
}
inline int operator>=(const ALString& s1, const char* s2)
{
	return s1.Compare(s2) >= 0;
}
inline int operator>=(const char* s1, const ALString& s2)
{
	return s2.Compare(s1) <= 0;
}
extern char ALSCHARNIL;
inline void ALString::Init()
{
	nDataLength = nAllocLength = 0;
	pchData = (char*)&ALSCHARNIL;
}
inline ALString::ALString()
{
	Init();
}
#if not defined __UNIX__ or defined __C11__
inline ALString::ALString(ALString&& stringSrc) noexcept
{
	// On transfere le contenu de la chaine
	pchData = stringSrc.pchData;
	nDataLength = stringSrc.nDataLength;
	nAllocLength = stringSrc.nAllocLength;
	// On positionne a NULL les donnes de la chaine temporaire, qui ne sera pas detruite
	stringSrc.pchData = (char*)&ALSCHARNIL;
}
inline ALString& ALString::operator=(ALString&& stringSrc) noexcept
{
	if (this != &stringSrc)
	{
		SafeDelete(pchData);
		pchData = stringSrc.pchData;
		nDataLength = stringSrc.nDataLength;
		nAllocLength = stringSrc.nAllocLength;
		stringSrc.pchData = (char*)&ALSCHARNIL;
	}
	return (*this);
}
#endif // not defined __UNIX__ or defined __C11__
inline longint ALString::GetUsedMemory() const
{
	return sizeof(ALString) + nAllocLength;
}

// Redefinition de l'operateur << de ostream
inline ostream& operator<<(ostream& ost, const ALString& s1)
{
	ost << (const char*)s1;
	return ost;
}
