// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWTextService;

#include "Object.h"
#include "ALString.h"
#include "Vector.h"
#include "CharVector.h"
#include "Timer.h"
#include "KWTokenFrequency.h"

//////////////////////////////////////////
// Service de gestion du type Text
//  . nommage de variable compatible utf8
//  . decodage utf8
//  ...
class KWTextService : public Object
{
public:
	//////////////////////////////////////////////////////////////////////////////////////////
	// Conversion d'une chaine de caractere en mot utf8 imprimable
	// Une chaine de caractere est en effet une sequence de bytes sans
	// aucune contrainte d'encodage, et peut ne pas correspondre a un mot lisible utf8.
	//   - si ascii et imprimable ou utf8, on garde les bytes tels quels
	//   - sinon, on recode les bytes par le code hexa, entre des caracteres { et }
	// Le caractere { est lui double s'il est dans une sequence imprimable
	//
	// Exemples:
	//   - cas utf8
	//     bonjour
	//     a toto
	//     bonjour{{et}
	//     !%$){{}'
	//   - cas non utf8
	//     {09}
	//     {B7}
	//     {A205CF}
	//   - cas mixte
	//     {A3}bonjour
	//     bon{B578}
	//     ann{E9}e

	// Conversion d'une chaine de caracteres en un mot
	static const ALString ByteStringToWord(const ALString& sByteString);

	// Conversion d'un mot en chaine de caracteres
	// Renvoie "" si le mot ne correspond pas a un codage valide
	static const ALString WordToByteString(const ALString& sWord);

	//////////////////////////////////////////////////////////////////////////////////////
	// Conversion entre caracteres hexa et bytes

	// Test si un caractere est un caractere hexa (0 a 9 ou A a F)
	static boolean IsHexChar(char c);

	// Code entre 0 et 15 associe a un caractere hexa
	static int GetHexCharCode(char c);

	// Caractere hexa pour un code entre 0 et 15
	static char GetHexChar(int nHexCharCode);

	// Caracteres hexa correspondant a un caractere
	static char GetFirstHexChar(char c);
	static char GetSecondHexChar(char c);

	// Transformation d'une chaine de bytes en chaine de caracteres hexa
	static const ALString ByteStringToHexCharString(const ALString& sByteString);

	// Transformation d'une chaine de caracteres hexa en chaines de bytes
	// Renvoie "" si le mot ne correspond pas a un codage valide
	static const ALString HexCharStringToByteString(const ALString& sHexCharString);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Services divers

	// Conversion d'une chaine de bytes vers un format imprimable, en remplacant tous les caractres non imprimables
	// par '?'
	static const ALString ToPrintable(const ALString& sBytes);

	// Longueur en bytes d'un caractere UTF8 valide a partir d'une position donnee
	// Retourne 1 a 4 dans le cas d'un caractere valide, 0 sinon pour un caractere ANSI non encodable directement
	static int GetValidUTF8CharLengthAt(const ALString& sValue, int nStart);

	// Construction d'un echantillon de textes basiques pour des tests
	static void BuildTextSample(StringVector* svTextValues);

	// Test de la classe
	static void Test();
};

////////////////////////////////////
// Methode en inline

inline boolean KWTextService::IsHexChar(char c)
{
	return ('0' <= c and c <= '9') or ('A' <= c and c <= 'F');
}

inline int KWTextService::GetHexCharCode(char c)
{
	require(IsHexChar(c));
	if (c >= 'A')
		return c - 'A' + 10;
	else
		return c - '0';
}

inline char KWTextService::GetHexChar(int nHexCharCode)
{
	require(0 <= nHexCharCode and nHexCharCode < 16);
	if (nHexCharCode >= 10)
		return 'A' + (char)nHexCharCode - 10;
	else
		return '0' + (char)nHexCharCode;
}

inline char KWTextService::GetFirstHexChar(char c)
{
	return GetHexChar(((unsigned char)c) / 16);
}

inline char KWTextService::GetSecondHexChar(char c)
{
	return GetHexChar(((unsigned char)c) % 16);
}
