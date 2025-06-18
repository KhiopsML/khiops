// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class TextService;

#include "Object.h"
#include "ALString.h"
#include "Vector.h"
#include "CharVector.h"
#include "Timer.h"
#include "IntVectorSorter.h"

//////////////////////////////////////////
// Service de gestion du type Text
//  . nommage de variable compatible utf8
//  . decodage utf8
//  ...
class TextService : public Object
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

	///////////////////////////////////////////////////////////////////////////////
	// Gestion de l'encodage des chaines ansi en utf8, le format supporte par json
	//
	// Cas avance, celui de l'encodage utuilise par Khiops pour gerer les caracteres ansi:
	// - les caracteres ansi 128 a 255 sont encodes avec iso-8859-1/windows-1252
	// - l'encodage dans le json se fait avec le caractere d'echapement pour
	//   un encodage unicode \uHHHH, ce qui permet de le distinguer des memes
	//   caracteres encodes en Utf8
	// Exemple: cas du carectere e accent aigu
	// - encodage ansi: E9
	// - encodage Utf8: C3A9
	// - encodage Unicode: U+00E0
	// Le parser de json utilise JsonToCString pour transformer les chaines de caracteres json
	// en chaine de caracteres C, par defaut avec un encodage Utf8
	// Avec le mode ForceUnicodeToAnsi, on peut retrouver l'encodage ansi dans le cas unicode

	// En mode ForceUnicodeToAnsi (defaut: false), les caracteres de la plage de windows1252/iso8859-1 encodes
	// via la syntaxe \uxxxx sont transformes en caracteres de l'ansi etendu, non Utf8
	static void SetForceUnicodeToAnsi(boolean bValue);
	static boolean GetForceUnicodeToAnsi();

	// Conversion d'une chaine Json valide vers une chaine C
	// On renvoie false en cas d'erreur d'encodage de la chaine a convertir,
	// tout en encodant au mieux avec des caractere speciaux ou les caracteres tels quels
	// En mode ForceUnicodeAnsi, on renvoie true si la chaine Json est valide,
	// bien que la chaine C ne soit pas avec un encodage UTF8
	static boolean JsonToCString(const char* sJsonString, ALString& sCString);

	// Encodage d'un chaine de caracteres C au format json, sans les double quotes de debut et fin
	static void CToJsonString(const ALString& sCString, ALString& sJsonString);

	// Encodage d'une chaine de caracteres C au format ansi en re-encodant les caracteres utf8
	// endodes avec iso-8859-1/windows-1252 vers ansi
	static void CToCAnsiString(const ALString& sCString, ALString& sCAnsiString);

	// Conversion d'un caractere ansi windows-1252 vers un caractere unicode au format hexa
	static void Windows1252ToUnicodeHex(int nAnsiCode, ALString& sUnicodeHexChars);

	// Conversion d'un caractere unicode au format hexa vers le code ansi windows-1252
	// Renvoie un code ansi entre 0 et 255 si ok, -1 sinon
	static int UnicodeHexToWindows1252(const ALString& sUnicodeHexChars);

	///////////////////////////////////////////////////////////////////////////////
	// Gestion de l'encodage des donnees binaires au format base64

	// Conversion d'une chaine base64 vers une chaine C
	// Le tableau de byte en sortie doit etre de taille au moins 3 * inputLength/4,
	// conformement aux besoins de l'encodage base64
	// On renvoie false en cas d'erreur d'encodage
	static boolean Base64StringToBytes(const ALString& sBase64String, char* sBytes);

	// Encodage d'un tableau de bytes vers le format base64, sans les double quotes de debut et fin
	// Le teableau en entree peut contenir n'importe quel byte, y comrpis des '\0'
	static void BytesToBase64String(const char* sBytes, int nByteNumber, ALString& sBase64String);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Services divers

	// Conversion d'une chaine de bytes vers un format imprimable, en remplacant tous les caractres non imprimables
	// par '?'
	static const ALString ToPrintable(const ALString& sBytes);

	// Longueur en bytes d'un caractere UTF8 valide a partir d'une position donnee
	// Retourne 1 a 4 dans le cas d'un caractere valide, 0 sinon pour un caractere ANSI non encodable directement
	static int GetValidUTF8CharLengthAt(const char* sValue, int nStart);

	// Longueur en bytes de la sous-partie d'une chaine encodee avec des caracteres UTF8 valide
	static int GetValidUTF8SubStringLength(const char* sValue);

	// Construction d'un echantillon de textes basiques pour des tests
	static void BuildTextSample(StringVector* svTextValues);

	// Test de la classe
	static void Test();

	//////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constructeur, permettant d'initialiser l'instance statique textServiceGlobalInitializer
	TextService();
	~TextService();

	// Ajout d'une sous partie d'une chaine
	static void AppendSubString(ALString& sString, const char* sAddedString, int nBegin, int nLength);

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des encodages windows-1252 vers unicode et UFT8

	// Code d'une chaine de caracteres hexa encodant un caractere unicode ou utf8,
	// pour une longueur d'au plus trois byte correspondant aux besoins d'encodage de la page de code Windows-1252
	static int GetHexStringCode(const ALString& sHexString);

	// Conversion d'un caractere ansi windows-1252 vers un caractere utf8 au format hexa
	static void Windows1252ToUtf8Hex(int nAnsiCode, ALString& sUtf8HexChars);

	// Recherche du code ansi d'un code de caracteres utf8 dans la page de code Windows-1252
	// Renvoie un code ansi entre 0 et 255 si ok, -1 sinon
	static int Windows1252Utf8CodeToWindows1252(int nWindows1252Utf8Code);

	// Initialisation de l'ensemble des structures d'encodages
	static void InitializeEncodingStructures();

	// Indique que l'initialisation est effectuee
	static boolean AreEncodingStructuresInitialized();

	// Verification de l'ensemble des structures d'encodages
	static boolean CheckEncodingStructures();

	// Initialisation de la table de transcodage entre les caracteres asci Windows-1252 et les caracteres unicode ou
	// utf8 Les caracteres speciaux 0x00 a 0x1F sont encodes en 0x00HH (4 caractere hexa en unicode) Les caracteres
	// ansi 0x20 a 0x7F sont encodes tels quels avec un seul caractere Les caracteres speciaux 0x80 a 0x9F sont
	// encodes de facon speciale pour avoir le meme caractere imprimable qu'avec l'encodage windows-1252 (4
	// caractere hexa en unicode). Cf. https://www.i18nqa.com/debug/table-iso8859-1-vs-windows-1252.html
	// 	   https://www.charset.org/utf-8
	// Les caracteres latin etendus speciaux 0xA0 a 0xEF sont encodes en 0x00HH (4 caractere hexa)
	static void InitializeWindows1252UnicodeHexEncoding();
	static void InitializeWindows1252Utf8HexEncoding();

	// Initialisation des structure de decodage des caracteres Windows-1252, pour retrouver leur index en fonction
	// de leur code utf8
	static void InitializeWindows1252Utf8DecodingStructures();

	// Nombre max de bytes de l'encodage windows-1252
	const static int nWindows1252EncodingMaxByteNumber = 3;

	///////////////////////////////////////////////////////////////////////////////////
	// Gestion des encodage windows-1252 vers l'unicode et l'UTF8, en caracteres hexa

	// Table de transcodage de taille 256 entre les caracteres asci windows-1252 et les caracteres unicode, ou utf8,
	// sous forme de chaines de caracteres en hexa (0-9 et A-F)
	static StringVector svWindows1252UnicodeHexEncoding;
	static StringVector svWindows1252Utf8HexEncoding;

	// Vecteur des codes UTF8 pour les 32 caracteres de controles Windows-1252 (128 a 159), et leur index
	static IntVectorSorter ivsWindows1252ControlCharUtf8CodeSorter;

	// Instance statique de KWTextService, permettant de forcer l'initialisation des structure d'encodage une fois
	// pour toute lors de l'appel du constructeur de cette instance
	// Ne pas declarer d'autres instances statiques de JSONFile, par exemple via d'autre classes, sinon cela pose
	// des problemes de memoire non liberee non diagnostiques par les outils de getsion de la memoire
	static TextService textServiceGlobalInitializer;

	// Parametrage de la conversion dees chaines json vers des chaines C
	static boolean bForceUnicodeToAnsi;
};

////////////////////////////////////
// Methode en inline

inline boolean TextService::IsHexChar(char c)
{
	return ('0' <= c and c <= '9') or ('A' <= c and c <= 'F') or ('a' <= c and c <= 'f');
}

inline int TextService::GetHexCharCode(char c)
{
	require(IsHexChar(c));
	if (c >= 'a')
		return c - 'a' + 10;
	else if (c >= 'A')
		return c - 'A' + 10;
	else
		return c - '0';
}

inline char TextService::GetHexChar(int nHexCharCode)
{
	require(0 <= nHexCharCode and nHexCharCode < 16);
	if (nHexCharCode >= 10)
		return 'A' + (char)nHexCharCode - 10;
	else
		return '0' + (char)nHexCharCode;
}

inline char TextService::GetFirstHexChar(char c)
{
	return GetHexChar(((unsigned char)c) / 16);
}

inline char TextService::GetSecondHexChar(char c)
{
	return GetHexChar(((unsigned char)c) % 16);
}
