// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TextService.h"

const ALString TextService::ByteStringToWord(const ALString& sByteString)
{
	ALString sWord;
	int nUTF8CharLength;
	int i;
	char c;
	boolean bIsByte;
	boolean bIsPrintable;

	require(sByteString.GetLength() > 0);

	// Encodage de la chaine de bytes vers un mot UTF8
	bIsByte = false;
	i = 0;
	while (i < sByteString.GetLength())
	{
		// Recherche du nombre de caracteres UTF8 consecutifs valides
		nUTF8CharLength = GetValidUTF8CharLengthAt(sByteString, i);

		// Cas avec au plus un caractere valide
		if (nUTF8CharLength <= 1)
		{
			c = sByteString.GetAt(i);
			i++;

			// Test si caractere est valide ou non
			bIsPrintable = (nUTF8CharLength == 1) and not(iscntrl(c));

			// Gestion de la transition byte vs word
			if (sWord.GetLength() == 0)
			{
				if (not bIsPrintable)
					sWord += '{';
				bIsByte = not bIsPrintable;
			}
			else if (not bIsPrintable)
			{
				if (not bIsByte)
					sWord += '{';
				bIsByte = true;
			}
			else
			{
				if (bIsByte)
					sWord += '}';
				bIsByte = false;
			}

			// Ecriture dans le cas bytes
			if (bIsByte)
			{
				sWord += GetFirstHexChar(c);
				sWord += GetSecondHexChar(c);
			}
			// Dans le cas word sinon
			else
			{
				sWord += c;

				// Doublement dans le cas d'un '{'
				if (c == '{')
					sWord += '{';
			}
		}
		// Cas d'un catactere UTF8 multi-byte
		else
		{
			// Gestion de la transition byte vs word
			if (bIsByte)
				sWord += '}';
			bIsByte = false;

			// Ecriture du caractere utf8 multi-bytes
			if (nUTF8CharLength == 2)
			{
				sWord += sByteString.GetAt(i);
				sWord += sByteString.GetAt(i + 1);
				i += 2;
			}
			else if (nUTF8CharLength == 3)
			{
				sWord += sByteString.GetAt(i);
				sWord += sByteString.GetAt(i + 1);
				sWord += sByteString.GetAt(i + 2);
				i += 3;
			}
			else
			{
				assert(nUTF8CharLength == 4);
				sWord += sByteString.GetAt(i);
				sWord += sByteString.GetAt(i + 1);
				sWord += sByteString.GetAt(i + 2);
				sWord += sByteString.GetAt(i + 3);
				i += 4;
			}
		}
	}

	// Gestion de la la fin d'un sequence de type byte
	if (bIsByte)
		sWord += '}';
	if (WordToByteString(sWord) != sByteString)
		WordToByteString(sWord);
	assert(WordToByteString(sWord) == sByteString);
	return sWord;
}

const ALString TextService::WordToByteString(const ALString& sWord)
{
	ALString sByteString;
	int nUTF8CharLength;
	int i;
	int j;
	int iLast;
	char c;
	int nChar;
	boolean bIsByte;
	boolean bIsValid;

	require(sWord.GetLength() > 0);

	// Test de validite du premier caractere
	c = sWord.GetAt(0);
	bIsValid = sWord.GetLength() > 0;
	bIsByte = false;
	if (c == '{')
	{
		bIsValid = sWord.GetLength() > 1;
		bIsByte = sWord.GetAt(1) != '{';
	}

	// Decodage tant que la chaine est valide
	i = 0;
	while (bIsValid and i < sWord.GetLength())
	{
		// Cas de l'etat byte
		if (bIsByte)
		{
			assert(sWord.GetAt(i) == '{');
			assert(sWord.GetAt(i + 1) != '{');

			// On se decalle de 1 pour supprimer le {
			i++;

			// On regarde jusqu'ou on peut aller avant de trouver la fin de la sequence hexa
			iLast = i;
			while (iLast < sWord.GetLength())
			{
				if (sWord.GetAt(iLast) != '}')
					iLast++;
				else
					break;
			}

			// Test de validite
			bIsValid = (iLast > i) and ((iLast - i) % 2 == 0);
			bIsValid = bIsValid and (sWord.GetAt(iLast) == '}');

			// Si valide, recodage des caracteres hexa par paires en bytes
			if (bIsValid)
			{
				while (i < iLast)
				{
					nChar = 16 * GetHexCharCode(sWord.GetAt(i));
					i++;
					nChar += GetHexCharCode(sWord.GetAt(i));
					i++;
					sByteString += (char)nChar;
				}

				// On repasse en etat word
				bIsByte = false;
				i++;
			}
		}
		// Cas de l'etat word
		else
		{
			assert(i == 0 or sWord.GetAt(i - 1) == '}');

			// On regarde jusqu'ou on peut aller avant, soit la fin, soit le passage en etat byte
			iLast = i;
			while (iLast < sWord.GetLength())
			{
				if (sWord.GetAt(iLast) != '{')
					iLast++;
				// On saute les '{' doubles
				else if (iLast + 1 < sWord.GetLength() and sWord.GetAt(iLast + 1) == '{')
					iLast += 2;
				// Arret si '#' unique
				else
					break;
			}

			// Test de validite
			bIsValid = (iLast > i);
			if (bIsValid)
			{
				if (iLast < sWord.GetLength())
					bIsValid = (sWord.GetAt(iLast) == '{');
			}

			// Si valide, on recopie les caracteres UTF8 s'ils sont valides
			if (bIsValid)
			{
				assert(iLast == sWord.GetLength() or sWord.GetAt(iLast) == '{');

				// Analyse des caracteres pour verifier qu'ils sont utf8 et imprimables
				while (i < iLast)
				{
					// Recherche du nombre de caracteres UTF8 consecutifs valides
					nUTF8CharLength = GetValidUTF8CharLengthAt(sWord, i);
					c = sWord.GetAt(i);

					// Test de validite
					bIsValid = nUTF8CharLength > 0 or (nUTF8CharLength == 1 and not(iscntrl(c)));

					// Si valide, on memorise les bytes du caractere utf8 tels quels
					if (bIsValid)
					{
						// Cas mono-caracteres
						if (nUTF8CharLength == 1)
						{
							c = sWord.GetAt(i);
							i++;
							sByteString += c;

							// Cas particulier du '{' a dedoubler
							if (c == '{')
							{
								assert(sWord.GetAt(i) == '{');
								i++;
							}
						}
						// Cas general des caracteres utf8 multi-bytes
						else
						{
							for (j = 0; j < nUTF8CharLength; j++)
							{
								c = sWord.GetAt(i);
								i++;
								sByteString += c;
							}
						}
					}
					// Arret sinon
					else
						break;
				}

				// On repasse en etat byte
				bIsByte = true;
			}
		}
	}

	// Si non valide, on met le resultat a vide
	if (not bIsValid)
		sByteString = "";
	return sByteString;
}

const ALString TextService::ByteStringToHexCharString(const ALString& sByteString)
{
	ALString sHexCharString;
	int i;
	char c;

	// Recodage des bytes par paires de caracteres hexa
	for (i = 0; i < sByteString.GetLength(); i++)
	{
		c = sByteString.GetAt(i);
		sHexCharString += GetFirstHexChar(c);
		sHexCharString += GetSecondHexChar(c);
	}
	return sHexCharString;
}

const ALString TextService::HexCharStringToByteString(const ALString& sHexCharString)
{
	ALString sByteString;
	int nByteStringLength;
	int i;
	int j;
	int nChar;
	boolean bIsValid;

	// La chaine a recoder doit avoir un nombre pair de caracteres
	bIsValid = (sHexCharString.GetLength() % 2 == 0);

	// Recodage des caracteres hexa par paires en bytes
	if (bIsValid)
	{
		nByteStringLength = sHexCharString.GetLength() / 2;
		j = 0;
		for (i = 0; i < nByteStringLength; i++)
		{
			bIsValid = IsHexChar(sHexCharString.GetAt(j)) and IsHexChar(sHexCharString.GetAt(j + 1));
			if (not bIsValid)
				break;
			nChar = 16 * GetHexCharCode(sHexCharString.GetAt(j));
			j++;
			nChar += GetHexCharCode(sHexCharString.GetAt(j));
			j++;
			sByteString += (char)nChar;
		}
	}

	// Si non valide, on met le resultat a vide
	if (not bIsValid)
		sByteString = "";
	return sByteString;
}

static void HexStringToCode(const unsigned char* sHexString, unsigned int* nCode)
{
	unsigned int i;
	for (i = 0; i < 4; i++)
	{
		unsigned char c = sHexString[i];
		if (c >= 'A')
			c = (c & ~0x20) - 7;
		c -= '0';
		assert(!(c & 0xF0));
		*nCode = (*nCode << 4) | c;
	}
}

// Recode un entier representant un code UTF32 sous forme d'une sequence de 1 a 4 caracteres au format UTF8
// La chaine doit avoir au moins 5 caracteres pour stocker le resultat
// Retourne le nombre de caracteres utilises
static int Utf32toUtf8String(unsigned int nCode, char* sUtf8Chars)
{
	if (nCode < 0x80)
	{
		sUtf8Chars[0] = (char)nCode;
		sUtf8Chars[1] = 0;
		return 1;
	}
	else if (nCode < 0x0800)
	{
		sUtf8Chars[0] = (char)((nCode >> 6) | 0xC0);
		sUtf8Chars[1] = (char)((nCode & 0x3F) | 0x80);
		sUtf8Chars[2] = 0;
		return 2;
	}
	else if (nCode < 0x10000)
	{
		sUtf8Chars[0] = (char)((nCode >> 12) | 0xE0);
		sUtf8Chars[1] = (char)(((nCode >> 6) & 0x3F) | 0x80);
		sUtf8Chars[2] = (char)((nCode & 0x3F) | 0x80);
		sUtf8Chars[3] = 0;
		return 3;
	}
	else if (nCode < 0x200000)
	{
		sUtf8Chars[0] = (char)((nCode >> 18) | 0xF0);
		sUtf8Chars[1] = (char)(((nCode >> 12) & 0x3F) | 0x80);
		sUtf8Chars[2] = (char)(((nCode >> 6) & 0x3F) | 0x80);
		sUtf8Chars[3] = (char)((nCode & 0x3F) | 0x80);
		sUtf8Chars[4] = 0;
		return 4;
	}
	else
	{
		sUtf8Chars[0] = '?';
		sUtf8Chars[1] = 0;
		return 1;
	}
}

void TextService::JsonToCString(const char* sJsonString, ALString& sCString)
{
	const unsigned char* sInputString;
	unsigned int nCode;
	unsigned int nPotentialCode;
	int nBegin;
	int nEnd;
	int nLength;
	char sUtf8Chars[5];
	const char* sCharsToAdd;
	ALString sUnicodeChars;
	int nCharNumber;

	require(sJsonString != NULL);

	// On passe par un format unsigned char pour le parsing
	sInputString = (const unsigned char*)sJsonString;
	nLength = (int)strlen(sJsonString);

	// On repasse la chaine a convertir a vide, sans desallouer la memoire
	sCString.GetBufferSetLength(0);

	// Analyse de la chaine en entree
	nBegin = 0;
	nEnd = 0;
	sCharsToAdd = "?";
	while (nEnd < nLength)
	{
		if (sInputString[nEnd] == '\\')
		{
			AppendSubString(sCString, sJsonString, nBegin, nEnd - nBegin);
			nEnd++;
			assert(nEnd < nLength);
			nCharNumber = 1;
			switch (sInputString[nEnd])
			{
			case 'r':
				sCharsToAdd = "\r";
				break;
			case 'n':
				sCharsToAdd = "\n";
				break;
			case '\\':
				sCharsToAdd = "\\";
				break;
			case '/':
				sCharsToAdd = "/";
				break;
			case '"':
				sCharsToAdd = "\"";
				break;
			case 'f':
				sCharsToAdd = "\f";
				break;
			case 'b':
				sCharsToAdd = "\b";
				break;
			case 't':
				sCharsToAdd = "\t";
				break;
			case 'u':
			{
				nCode = 0;
				nEnd++;
				assert(nEnd < nLength);

				// Extraction des caracteres unicode
				if (sUnicodeChars.GetLength() != 4)
					sUnicodeChars.GetBufferSetLength(4);
				sUnicodeChars.SetAt(0, sInputString[nEnd]);
				sUnicodeChars.SetAt(1, sInputString[nEnd + 1]);
				sUnicodeChars.SetAt(2, sInputString[nEnd + 2]);
				sUnicodeChars.SetAt(3, sInputString[nEnd + 3]);

				// On tente d'abord le decodgage d'un caractere windows-1252 encode avec unicode
				nCode = UnicodeHexToWindows1252(sUnicodeChars);
				if (nCode != -1)
				{
					nEnd += 3;
					nCharNumber = 1;
					sUtf8Chars[0] = (char)nCode;
					sUtf8Chars[1] = '\0';
					sCharsToAdd = sUtf8Chars;
				}
				// Cas general sinon
				else
				{
					HexStringToCode(sInputString + nEnd, &nCode);
					nEnd += 3;
					assert(nEnd < nLength);

					// Verification du code
					if ((nCode & 0xFC00) == 0xD800)
					{
						nEnd++;
						assert(nEnd + 1 < nLength);
						if (sInputString[nEnd] == '\\' && sInputString[nEnd + 1] == 'u')
						{
							nPotentialCode = 0;
							HexStringToCode(sInputString + nEnd + 2, &nPotentialCode);
							nCode = (((nCode & 0x3F) << 10) |
								 ((((nCode >> 6) & 0xF) + 1) << 16) |
								 (nPotentialCode & 0x3FF));
							nEnd += 5;
							assert(nEnd < nLength);
						}
						else
						{
							sCharsToAdd = "?";
							break;
						}
					}

					// Conversion
					nCharNumber = Utf32toUtf8String(nCode, sUtf8Chars);
					sCharsToAdd = sUtf8Chars;

					// Cas particulier du code 0
					if (nCode == 0)
					{
						sCString += sCharsToAdd[0];
						nEnd++;
						nBegin = nEnd;
						continue;
					}
				}
				break;
			}
			default:
				// En principe, impossible avec une chaine json correctement formee
				assert(false);
			}
			if (nCharNumber == 1)
				sCString += sCharsToAdd[0];
			else
				AppendSubString(sCString, sCharsToAdd, 0, nCharNumber);
			nEnd++;
			nBegin = nEnd;
		}
		else
		{
			nEnd++;
		}
	}
	AppendSubString(sCString, sJsonString, nBegin, nEnd - nBegin);
}

void TextService::CToJsonString(const ALString& sCString, ALString& sJsonString)
{
	boolean bTrace = false;
	int nMaxValidUTF8CharLength;
	int i;
	unsigned char c;
	const char* cHexMap = "0123456789ABCDEF";
	ALString sUnicodeChars;
	int nUTF8CharLength;

	// Retaillage de la chaine json
	sJsonString.GetBufferSetLength(0);

	// Encodage de la chaine au format json
	i = 0;
	nMaxValidUTF8CharLength = 0;
	while (i < sCString.GetLength())
	{
		// Recherche du nombre de caracteres UTF8 consecutifs valides
		nUTF8CharLength = TextService::GetValidUTF8CharLengthAt(sCString, i);
		nMaxValidUTF8CharLength = max(nMaxValidUTF8CharLength, nUTF8CharLength);

		// Cas avec 0 ou un caractere valide
		if (nUTF8CharLength <= 1)
		{
			c = (unsigned char)sCString.GetAt(i);
			i++;

			// Gestion des caracteres speciaux
			switch (c)
			{
			case '"':
				sJsonString += "\\\"";
				break;
			case '\\':
				sJsonString += "\\\\";
				break;
			case '/':
				sJsonString += "\\/";
				break;
			case '\b':
				sJsonString += "\\b";
				break;
			case '\f':
				sJsonString += "\\f";
				break;
			case '\n':
				sJsonString += "\\n";
				break;
			case '\r':
				sJsonString += "\\r";
				break;
			case '\t':
				sJsonString += "\\t";
				break;
			default:
				// Caracteres de controle ansi
				if (c < 0x20)
				{
					sJsonString += "\\u00";
					sJsonString += cHexMap[c / 16];
					sJsonString += cHexMap[c % 16];
				}
				// Caracteres de ascii etendu
				else if (c >= 0x80)
				{
					Windows1252ToUnicodeHex(c, sUnicodeChars);
					sJsonString += "\\u";
					sJsonString += sUnicodeChars;
				}
				// Caractere standard
				else
					sJsonString += c;
				break;
			}
		}
		// Cas d'un catactere UTF8 multi-byte
		else if (nUTF8CharLength == 2)
		{
			sJsonString += sCString.GetAt(i);
			sJsonString += sCString.GetAt(i + 1);
			i += 2;
		}
		else if (nUTF8CharLength == 3)
		{
			sJsonString += sCString.GetAt(i);
			sJsonString += sCString.GetAt(i + 1);
			sJsonString += sCString.GetAt(i + 2);
			i += 3;
		}
		else if (nUTF8CharLength == 4)
		{
			sJsonString += sCString.GetAt(i);
			sJsonString += sCString.GetAt(i + 1);
			sJsonString += sCString.GetAt(i + 2);
			sJsonString += sCString.GetAt(i + 3);
			i += 4;
		}
	}

	// Trace
	if (bTrace)
	{
		if (nMaxValidUTF8CharLength > 1 or sCString.GetLength() != sJsonString.GetLength())
			cout << sCString.GetLength() << "\t" << sJsonString.GetLength() << "\t" << sCString << "\t"
			     << sJsonString << "\tutf8 max length:" << nMaxValidUTF8CharLength << endl;
	}
}

void TextService::CToCAnsiString(const ALString& sCString, ALString& sCAnsiString)
{
	int i;
	int j;
	int nUTF8CharLength;
	int nUtf8Code;
	int nAnsiCodeFromUtf8;

	// Retaillage de la chaine cible
	sCAnsiString.GetBufferSetLength(0);

	// Encodage de la chaine au format json
	i = 0;
	while (i < sCString.GetLength())
	{
		// Recherche du nombre de caracteres UTF8 consecutifs valides
		nUTF8CharLength = TextService::GetValidUTF8CharLengthAt(sCString, i);

		// Cas avec 0 ou un caractere valide, ascii ou ansi
		if (nUTF8CharLength <= 1)
		{
			sCAnsiString += sCString.GetAt(i);
			i++;
		}
		// Cas d'un caractere UTF8 multi-byte trop long pour la plage windows-1252
		else if (nUTF8CharLength == 4)
		{
			// On memorise les bytes du carectere utf8 tels quels
			for (j = 0; j < nUTF8CharLength; j++)
			{
				sCAnsiString += sCString.GetAt(i);
				i++;
			}
		}
		// Cas d'un caractere UTF8 multi-byte pouvant etre encode sur la plage windows-1252
		else
		{
			assert(2 <= nUTF8CharLength and nUTF8CharLength <= nWindows1252EncodingMaxByteNumber);

			// Calcul du code utf8 et deplacement dans la chaine
			nUtf8Code = 0;
			for (j = 0; j < nUTF8CharLength; j++)
			{
				nUtf8Code *= 256;
				nUtf8Code += (unsigned char)sCString.GetAt(i);
				i++;
			}

			// Recherche de l'index du caractere dans la plage windows-1252
			nAnsiCodeFromUtf8 = Windows1252Utf8CodeToWindows1252(nUtf8Code);

			// Encodage ansi si necessaire
			if (nAnsiCodeFromUtf8 != -1)
			{
				assert(128 <= nAnsiCodeFromUtf8 and nAnsiCodeFromUtf8 < 256);
				sCAnsiString += char(nAnsiCodeFromUtf8);
			}
			// Sinon, on remet les bytes de l'encodage utf8 tels quels
			else
			{
				for (j = i - nUTF8CharLength; j < i; j++)
				{
					sCAnsiString += sCString.GetAt(j);
				}
			}
		}
	}
}

void TextService::Windows1252ToUnicodeHex(int nAnsiCode, ALString& sUnicodeHexChars)
{
	require(0 <= nAnsiCode and nAnsiCode <= 255);
	require(AreEncodingStructuresInitialized());

	sUnicodeHexChars = svWindows1252UnicodeHexEncoding.GetAt(nAnsiCode);
}

int TextService::UnicodeHexToWindows1252(const ALString& sUnicodeHexChars)
{
	int nCode;
	int i;

	require(sUnicodeHexChars.GetLength() == 2 or sUnicodeHexChars.GetLength() == 4);
	require(AreEncodingStructuresInitialized());

	// Cas ou les deux premiers caracteres sont encodes avec "00"
	nCode = -1;
	if (sUnicodeHexChars.GetAt(0) == '0' and sUnicodeHexChars.GetAt(1) == '0')
	{
		// Decodage du premier caractere suivant
		nCode = TextService::GetHexCharCode(sUnicodeHexChars.GetAt(2));

		// Decodage du second caractere suivant
		nCode *= 16;
		nCode += TextService::GetHexCharCode(sUnicodeHexChars.GetAt(3));
	}

	// Sinon, on recherche dans la table des caracteres speciaux
	if (nCode == -1)
	{
		for (i = 0x80; i <= 0x9F; i++)
		{
			if (sUnicodeHexChars == svWindows1252UnicodeHexEncoding.GetAt(i))
			{
				nCode = i;
				break;
			}
		}
	}
	ensure(nCode == -1 or (0 <= nCode and nCode <= 255));
	ensure(nCode == -1 or svWindows1252UnicodeHexEncoding.GetAt(nCode) == sUnicodeHexChars);
	return nCode;
}

const ALString TextService::ToPrintable(const ALString& sBytes)
{
	ALString sPrintableBytes;
	int i;

	// Copie des caracteres en remplacant ceux non imprimable par '?'
	for (i = 0; i < sBytes.GetLength(); i++)
	{
		if (isprint(sBytes.GetAt(i)))
			sPrintableBytes += sBytes.GetAt(i);
		else
			sPrintableBytes += '?';
	}
	return sPrintableBytes;
}

int TextService::GetValidUTF8CharLengthAt(const ALString& sValue, int nStart)
{
	int nUtf8CharLength;
	int c;
	int nLength;

	require(0 <= nStart and nStart < sValue.GetLength());

	// Initialisations
	nUtf8CharLength = 0;
	nLength = sValue.GetLength();
	c = (unsigned char)sValue.GetAt(nStart);

	// Cas d'un caractere ascii 0bbbbbbb
	if (0x00 <= c and c <= 0x7f)
		nUtf8CharLength = 1;
	// Debut d'un caractere UTF8 sur deux octets 110bbbbb
	else if ((c & 0xE0) == 0xC0)
	{
		if (nStart + 1 < nLength and ((unsigned char)sValue.GetAt(nStart + 1) & 0xC0) == 0x80)
			nUtf8CharLength = 2;
		else
			nUtf8CharLength = 0;
	}
	// Debut d'un caractere UTF8 sur trois octets 1110bbbb
	else if ((c & 0xF0) == 0xE0)
	{
		if (nStart + 2 < nLength and ((unsigned char)sValue.GetAt(nStart + 1) & 0xC0) == 0x80 and
		    ((unsigned char)sValue.GetAt(nStart + 2) & 0xC0) == 0x80)
			nUtf8CharLength = 3;
		else
			nUtf8CharLength = 0;
	}
	// Debut d'un caractere UTF8 sur quatre octets 11110bbb
	else if ((c & 0xF8) == 0xF0)
	{
		if (nStart + 3 < nLength and ((unsigned char)sValue.GetAt(nStart + 1) & 0xC0) == 0x80 and
		    ((unsigned char)sValue.GetAt(nStart + 2) & 0xC0) == 0x80 and
		    ((unsigned char)sValue.GetAt(nStart + 3) & 0xC0) == 0x80)
			nUtf8CharLength = 4;
		else
			nUtf8CharLength = 0;
	}
	return nUtf8CharLength;
}

void TextService::BuildTextSample(StringVector* svTextValues)
{
	CharVector cvAscii;
	int i;
	ALString sTmp;

	require(svTextValues != NULL);

	// Creation de la table ascii
	for (i = 0; i < 256; i++)
		cvAscii.Add((char)i);

	// Initialisation de valeurs de test
	svTextValues->SetSize(0);
	svTextValues->Add("bonjour");
	svTextValues->Add("a toto");
	svTextValues->Add("bonjour#et");
	svTextValues->Add("bonjour{et}");
	svTextValues->Add("!%$)#'");
	svTextValues->Add("!%$){'");
	svTextValues->Add("###");
	svTextValues->Add("{{{");
	svTextValues->Add(" ");
	svTextValues->Add("\t");
	svTextValues->Add("#");
	svTextValues->Add("{");
	svTextValues->Add(sTmp + cvAscii.GetAt(12));
	svTextValues->Add(sTmp + cvAscii.GetAt(233));
	svTextValues->Add(sTmp + cvAscii.GetAt(232) + cvAscii.GetAt(233) + cvAscii.GetAt(234));
	svTextValues->Add(sTmp + cvAscii.GetAt(233) + "bonjour");
	svTextValues->Add(sTmp + "bonjour" + cvAscii.GetAt(232) + cvAscii.GetAt(233) + cvAscii.GetAt(234));
	svTextValues->Add(sTmp + "bon" + cvAscii.GetAt(232) + cvAscii.GetAt(233) + cvAscii.GetAt(234) + "jour");
	svTextValues->Add(sTmp + "bonne ann" + cvAscii.GetAt(233) + "e");
	svTextValues->Add(sTmp + cvAscii.GetAt(232) + cvAscii.GetAt(233) + "bon" + cvAscii.GetAt(234) + "jour" +
			  cvAscii.GetAt(233) + cvAscii.GetAt(234));
	svTextValues->Add(sTmp + cvAscii.GetAt(232) + cvAscii.GetAt(233) + "bon" + cvAscii.GetAt(234) + "jour" +
			  cvAscii.GetAt(233) + '#' + cvAscii.GetAt(234));
	svTextValues->Add(sTmp + cvAscii.GetAt(226) + cvAscii.GetAt(130) +
			  cvAscii.GetAt(172)); // Windows1252 E2 82 AC: Euro Sign
	svTextValues->Add(sTmp + cvAscii.GetAt(226) + cvAscii.GetAt(132) +
			  cvAscii.GetAt(162));                             // Windows1252 E2 84 A2: Trade Mark Sign
	svTextValues->Add(sTmp + cvAscii.GetAt(203) + cvAscii.GetAt(156)); // Windows1252 CB 9C: Small Tilde
	svTextValues->Add(sTmp + cvAscii.GetAt(240) + cvAscii.GetAt(146) + cvAscii.GetAt(128) +
			  cvAscii.GetAt(128)); // sumerian cuneiform F0928080
}

void TextService::Test()
{
	int nEncodingNumber = 10000000;
	ALString sTest;
	ALString sCString;
	StringVector svTextValues;
	ALString sValue;
	ALString sWord;
	ALString sHexaString;
	ALString sRetrievedValue;
	ALString sUnicodeChars;
	ALString sUtf8Chars;
	int nAnsiCode;
	int nUtf8Code;
	int nAnsiCodeFromUtf8;
	int i;
	int j;
	int nLength;
	Timer timer;
	ALString sTmp;

	// Test de conversion elementaire
	sTest = "aa\\u221Ebb\\u00e9cc\\/\\\\dd\\tee";
	cout << "Json string\t" << sTest << "\t";
	JsonToCString(sTest, sCString);
	cout << sCString << "\t";

	// Test des encodages unicode des caracteres ascii etendus
	cout << "Index\tChar\tUnicode\tUtf8\tAnsi code\tUtf8 code\tAnsi code from utf8\tValid\n";
	for (i = 0; i < 256; i++)
	{
		Windows1252ToUnicodeHex(i, sUnicodeChars);
		Windows1252ToUtf8Hex(i, sUtf8Chars);
		nAnsiCode = UnicodeHexToWindows1252(sUnicodeChars);
		nUtf8Code = GetHexStringCode(sUtf8Chars);
		nAnsiCodeFromUtf8 = Windows1252Utf8CodeToWindows1252(nUtf8Code);
		cout << i << "\t";
		if (0x20 <= i and i <= 0x7F and i != 34)
			cout << (char)i << "\t";
		else
			cout << "\t";
		cout << sUnicodeChars << "\t";
		cout << sUtf8Chars << "\t";
		cout << nAnsiCode << "\t";
		cout << nUtf8Code << "\t";
		cout << nAnsiCodeFromUtf8 << "\t";
		cout << ((i == nAnsiCode) and (i == nAnsiCodeFromUtf8)) << endl;
		assert((i == nAnsiCode) and (i == nAnsiCodeFromUtf8));
	}

	// Diminution des tests en mode debug
	debug(nEncodingNumber = 100000);

	// Creation de la table ascii
	// Initialisation de l'echantillon de texts
	BuildTextSample(&svTextValues);

	// Conversion des valeurs vers des mots et chaines de caracteres heaxa
	for (i = 0; i < svTextValues.GetSize(); i++)
	{
		// Encodage/decodage de la valeur en mot
		sValue = svTextValues.GetAt(i);
		sWord = ByteStringToWord(sValue);
		sRetrievedValue = WordToByteString(sWord);
		assert(sRetrievedValue == sValue);

		// Encodage/decodage de la valeur en caracteres hexa
		sValue = svTextValues.GetAt(i);
		sHexaString = ByteStringToHexCharString(sValue);
		sRetrievedValue = HexCharStringToByteString(sHexaString);
		assert(sRetrievedValue == sValue);

		// Affichage
		cout << i << "\t";
		cout << ToPrintable(sValue) << "\t";
		cout << "(" << sWord << ")\t";
		cout << sHexaString << "\n";
	}

	// Conversion de valeurs aleatoire vers des mots
	timer.Reset();
	for (i = 0; i < nEncodingNumber; i++)
	{
		// Creation d'un mot aleatoire
		sValue = "";
		nLength = 1 + RandomInt(7);
		for (j = 0; j < nLength; j++)
			sValue += (char)(1 + RandomInt(254));

		// Encodage/decodage de la valeur en mot
		timer.Start();
		sWord = ByteStringToWord(sValue);
		sRetrievedValue = WordToByteString(sWord);
		timer.Stop();
		assert(sRetrievedValue == sValue);
	}
	cout << "Word encoding/decoding time for " << nEncodingNumber << " values: " << timer.GetElapsedTime() << "\n";
}

TextService::TextService()
{
	// Initialisation des structure d'encodage
	InitializeEncodingStructures();
}

TextService::~TextService() {}

void TextService::AppendSubString(ALString& sString, const char* sAddedString, int nBegin, int nLength)
{
	int i;
	int nStringLength;

	require(sAddedString != NULL);
	require(nBegin >= 0);
	require(nLength >= 0);
	require(nBegin + nLength <= (int)strlen(sAddedString));

	// Reservation de la place necessaire
	nStringLength = sString.GetLength();
	sString.GetBufferSetLength(nStringLength + nLength);

	// Ajout des caracteres
	for (i = nBegin; i < nBegin + nLength; i++)
	{
		sString.SetAt(nStringLength, sAddedString[i]);
		nStringLength++;
	}
}

int TextService::GetHexStringCode(const ALString& sHexString)
{
	int i;
	int nCode;
	require(sHexString.GetLength() == 2 or sHexString.GetLength() == 4 or sHexString.GetLength() == 6);

	nCode = 0;
	for (i = 0; i < sHexString.GetLength(); i++)
	{
		nCode *= 16;
		nCode += TextService::GetHexCharCode(sHexString.GetAt(i));
	}
	ensure(0 <= nCode and nCode < pow(256, nWindows1252EncodingMaxByteNumber));
	return nCode;
}

void TextService::Windows1252ToUtf8Hex(int nAnsiCode, ALString& sUtf8HexChars)
{
	require(0 <= nAnsiCode and nAnsiCode <= 255);
	require(AreEncodingStructuresInitialized());

	sUtf8HexChars = svWindows1252Utf8HexEncoding.GetAt(nAnsiCode);
	ensure(sUtf8HexChars.GetLength() <= nWindows1252EncodingMaxByteNumber);
}

int TextService::Windows1252Utf8CodeToWindows1252(int nWindows1252Utf8Code)
{
	const int nLowerC2Code = 0xC2A0;
	const int nUpperC2Code = 0xC2BF;
	const int nLowerC3Code = 0xC380;
	const int nUpperC3Code = 0xC3BF;
	int nIndex;

	require(nWindows1252Utf8Code >= 0);
	require(nWindows1252Utf8Code < pow(256, nWindows1252EncodingMaxByteNumber));
	require(AreEncodingStructuresInitialized());

	// Verification
	assert(nLowerC2Code == GetHexStringCode("C2A0"));
	assert(nUpperC2Code == GetHexStringCode("C2BF"));
	assert(nLowerC3Code == GetHexStringCode("C380"));
	assert(nUpperC3Code == GetHexStringCode("C3BF"));

	// Recherche dans la plage ascii
	if (nWindows1252Utf8Code < 0x80)
		nIndex = nWindows1252Utf8Code;
	// Recherche dans la plage C3
	else if (nLowerC3Code <= nWindows1252Utf8Code and nWindows1252Utf8Code <= nUpperC3Code)
		nIndex = nWindows1252Utf8Code - nLowerC3Code + 0xC0;
	// Recherche dans la plage C2
	else if (nLowerC2Code <= nWindows1252Utf8Code and nWindows1252Utf8Code <= nUpperC2Code)
		nIndex = nWindows1252Utf8Code - nLowerC2Code + 0xA0;
	// Recherche dans la plage des caracteres de controles
	else
	{
		// Recherche en utilisant la structure de decodage, qui permet une recherche dichotomique
		nIndex = ivsWindows1252ControlCharUtf8CodeSorter.LookupInitialIndex(nWindows1252Utf8Code);

		// Si trouve, on rajoute l'index ansi de base
		if (nIndex != -1)
			nIndex += 0x80;
	}
	ensure(nIndex == -1 or GetHexStringCode(svWindows1252Utf8HexEncoding.GetAt(nIndex)) == nWindows1252Utf8Code);
	return nIndex;
}

void TextService::InitializeEncodingStructures()
{
	if (not AreEncodingStructuresInitialized())
	{
		InitializeWindows1252UnicodeHexEncoding();
		InitializeWindows1252Utf8HexEncoding();
		InitializeWindows1252Utf8DecodingStructures();
		ensure(CheckEncodingStructures());
	}
	ensure(AreEncodingStructuresInitialized());
}

boolean TextService::AreEncodingStructuresInitialized()
{
	return svWindows1252UnicodeHexEncoding.GetSize() > 0;
}

boolean TextService::CheckEncodingStructures()
{
	boolean bOk = true;
	int i;
	int nUtf8Code;
	int nAnsiCode;

	// Verification des tailles des structures
	bOk = bOk and svWindows1252UnicodeHexEncoding.GetSize() == 256;
	bOk = bOk and svWindows1252Utf8HexEncoding.GetSize() == 256;
	bOk = bOk and ivsWindows1252ControlCharUtf8CodeSorter.GetSize() == 32;
	assert(bOk);

	// Verification de l'encodage unicode
	if (bOk)
	{
		for (i = 0; i < svWindows1252UnicodeHexEncoding.GetSize(); i++)
		{
			bOk = bOk and UnicodeHexToWindows1252(svWindows1252UnicodeHexEncoding.GetAt(i)) == i;
			assert(bOk);
		}
	}

	// Verification de l'encodage utf8
	if (bOk)
	{
		for (i = 0; i < svWindows1252Utf8HexEncoding.GetSize(); i++)
		{
			nUtf8Code = GetHexStringCode(svWindows1252Utf8HexEncoding.GetAt(i));
			nAnsiCode = Windows1252Utf8CodeToWindows1252(nUtf8Code);
			bOk = bOk and nAnsiCode == i;
			assert(bOk);
		}
	}
	return bOk;
}

void TextService::InitializeWindows1252UnicodeHexEncoding()
{
	boolean bWindows1252HexEncoding = true;

	if (svWindows1252UnicodeHexEncoding.GetSize() == 0)
	{
		const char* cHexMap = "0123456789ABCDEF";
		int i;
		ALString sUnicodePrefix = "00";
		ALString sEmpty;

		// Encodage tel quel des caracteres ascii, avec 4 caracteres hexa
		for (i = 0x00; i < 0x80; i++)
			svWindows1252UnicodeHexEncoding.Add(sUnicodePrefix + cHexMap[i / 16] + cHexMap[i % 16]);

		// Encodage des caracteres de controle ascii etendu windows-1252, avec 4 caracteres hexa
		assert(svWindows1252UnicodeHexEncoding.GetSize() == 128);
		if (bWindows1252HexEncoding)
		{
			svWindows1252UnicodeHexEncoding.Add("20AC"); // Euro Sign
			svWindows1252UnicodeHexEncoding.Add("0081"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("201A"); // Single Low-9 Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("0192"); // Latin Small Letter F With Hook
			svWindows1252UnicodeHexEncoding.Add("201E"); // Double Low-9 Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("2026"); // Horizontal Ellipsis
			svWindows1252UnicodeHexEncoding.Add("2020"); // Dagger
			svWindows1252UnicodeHexEncoding.Add("2021"); // Double Dagger
			svWindows1252UnicodeHexEncoding.Add("02C6"); // Modifier Letter Circumflex Accent
			svWindows1252UnicodeHexEncoding.Add("2030"); // Per Mille Sign
			svWindows1252UnicodeHexEncoding.Add("0160"); // Latin Capital Letter S With Caron
			svWindows1252UnicodeHexEncoding.Add("2039"); // Single Left-Pointing Angle Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("0152"); // Latin Capital Ligature OE
			svWindows1252UnicodeHexEncoding.Add("008D"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("017D"); // Latin Capital Letter Z With Caron
			svWindows1252UnicodeHexEncoding.Add("008F"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("0090"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("2018"); // Left Single Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("2019"); // Right Single Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("201C"); // Left Double Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("201D"); // Right Double Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("2022"); // Bullet
			svWindows1252UnicodeHexEncoding.Add("2013"); // En Dash
			svWindows1252UnicodeHexEncoding.Add("2014"); // Em Dash
			svWindows1252UnicodeHexEncoding.Add("02DC"); // Small Tilde
			svWindows1252UnicodeHexEncoding.Add("2122"); // Trade Mark Sign
			svWindows1252UnicodeHexEncoding.Add("0161"); // Latin Small Letter S With Caron
			svWindows1252UnicodeHexEncoding.Add("203A"); // Single Right-Pointing Angle Quotation Mark
			svWindows1252UnicodeHexEncoding.Add("0153"); // Latin Small Ligature OE
			svWindows1252UnicodeHexEncoding.Add("009D"); // UNASSIGNED
			svWindows1252UnicodeHexEncoding.Add("017E"); // Latin Small Letter Z With Caron
			svWindows1252UnicodeHexEncoding.Add("0178"); // Latin Capital Letter Y With Diaeresis
		}
		// Encodage ISO-8859-1
		else
		{
			for (i = 128; i < 160; i++)
				svWindows1252UnicodeHexEncoding.Add(sUnicodePrefix + cHexMap[i / 16] + cHexMap[i % 16]);
		}
		assert(svWindows1252UnicodeHexEncoding.GetSize() == 160);

		// Encodage des caracteres ascii etendu windows-1252, latin etendu, avec 4 caracteres hexa
		for (i = 160; i < 256; i++)
			svWindows1252UnicodeHexEncoding.Add(sUnicodePrefix + cHexMap[i / 16] + cHexMap[i % 16]);
		assert(svWindows1252UnicodeHexEncoding.GetSize() == 256);
	}
}

void TextService::InitializeWindows1252Utf8HexEncoding()
{
	boolean bWindows1252HexEncoding = true;

	if (svWindows1252Utf8HexEncoding.GetSize() == 0)
	{
		const char* cHexMap = "0123456789ABCDEF";
		int i;
		ALString sUtf8PrefixC2 = "C2";
		ALString sUtf8PrefixC3 = "C3";
		ALString sEmpty;

		// Encodage tel quel des caracteres ANSI, avec un seul byte, deux caracteres hexa
		for (i = 0; i < 0x80; i++)
			svWindows1252Utf8HexEncoding.Add(sEmpty + cHexMap[i / 16] + cHexMap[i % 16]);

		// Encodage des caracteres de controle ascii etendu windows-1252, avec 4 caracteres hexa
		assert(svWindows1252Utf8HexEncoding.GetSize() == 128);
		if (bWindows1252HexEncoding)
		{
			svWindows1252Utf8HexEncoding.Add("E282AC"); // Euro Sign
			svWindows1252Utf8HexEncoding.Add("C281");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("E2809A"); // Single Low-9 Quotation Mark
			svWindows1252Utf8HexEncoding.Add("C692");   // Latin Small Letter F With Hook
			svWindows1252Utf8HexEncoding.Add("E2809E"); // Double Low-9 Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E280A6"); // Horizontal Ellipsis
			svWindows1252Utf8HexEncoding.Add("E280A0"); // Dagger
			svWindows1252Utf8HexEncoding.Add("E280A1"); // Double Dagger
			svWindows1252Utf8HexEncoding.Add("CB86");   // Modifier Letter Circumflex Accent
			svWindows1252Utf8HexEncoding.Add("E280B0"); // Per Mille Sign
			svWindows1252Utf8HexEncoding.Add("C5A0");   // Latin Capital Letter S With Caron
			svWindows1252Utf8HexEncoding.Add("E280B9"); // Single Left-Pointing Angle Quotation Mark
			svWindows1252Utf8HexEncoding.Add("C592");   // Latin Capital Ligature OE
			svWindows1252Utf8HexEncoding.Add("C28D");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("C5BD");   // Latin Capital Letter Z With Caron
			svWindows1252Utf8HexEncoding.Add("C28F");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("C290");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("E28098"); // Left Single Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E28099"); // Right Single Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E2809C"); // Left Double Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E2809D"); // Right Double Quotation Mark
			svWindows1252Utf8HexEncoding.Add("E280A2"); // Bullet
			svWindows1252Utf8HexEncoding.Add("E28093"); // En Dash
			svWindows1252Utf8HexEncoding.Add("E28094"); // Em Dash
			svWindows1252Utf8HexEncoding.Add("CB9C");   // Small Tilde
			svWindows1252Utf8HexEncoding.Add("E284A2"); // Trade Mark Sign
			svWindows1252Utf8HexEncoding.Add("C5A1");   // Latin Small Letter S With Caron
			svWindows1252Utf8HexEncoding.Add("E280BA"); // Single Right-Pointing Angle Quotation Mark
			svWindows1252Utf8HexEncoding.Add("C593");   // Latin Small Ligature OE
			svWindows1252Utf8HexEncoding.Add("C29D");   // UNASSIGNED
			svWindows1252Utf8HexEncoding.Add("C5BE");   // Latin Small Letter Z With Caron
			svWindows1252Utf8HexEncoding.Add("C5B8");   // Latin Capital Letter Y With Diaeresis
		}
		// Encodage ISO-8859-1
		else
		{
			for (i = 128; i < 160; i++)
				svWindows1252Utf8HexEncoding.Add(sUtf8PrefixC2 + cHexMap[i / 16] + cHexMap[i % 16]);
		}
		assert(svWindows1252Utf8HexEncoding.GetSize() == 160);

		// Encodage des caracteres ascii etendu windows-1252, latin etendu, sur la fin de la plage C2A0 a C2BF
		// Encodage des caracteres ascii etendu windows-1252, latin etendu, sur la fin de la plage C2A0 a C2BF
		for (i = 160; i < 192; i++)
			svWindows1252Utf8HexEncoding.Add(sUtf8PrefixC2 + cHexMap[i / 16] + cHexMap[i % 16]);

		// Encodage des caracteres ascii etendu windows-1252, latin etendu, sur la plage C380 a C2BF
		for (i = 192; i < 256; i++)
			svWindows1252Utf8HexEncoding.Add(sUtf8PrefixC3 + cHexMap[(i - 64) / 16] + cHexMap[i % 16]);
		assert(svWindows1252Utf8HexEncoding.GetSize() == 256);
	}
}

void TextService::InitializeWindows1252Utf8DecodingStructures()
{
	int i;
	int nCode;
	IntVector ivInputCodes;

	require(svWindows1252Utf8HexEncoding.GetSize() == 256);

	if (ivsWindows1252ControlCharUtf8CodeSorter.GetSize() == 0)
	{
		// Parcours des caracteres de controles de l'encodage Windows-1252 pour initialiser les codes
		// correspondant
		for (i = 0x80; i < 0xA0; i++)
		{
			nCode = GetHexStringCode(svWindows1252Utf8HexEncoding.GetAt(i));
			ivInputCodes.Add(nCode);
		}

		// Tri de ces codes pour initialiser la structure de decodage
		ivsWindows1252ControlCharUtf8CodeSorter.SortVector(&ivInputCodes);
	}
	ensure(ivsWindows1252ControlCharUtf8CodeSorter.GetSize() == 32);
}

StringVector TextService::svWindows1252UnicodeHexEncoding;
StringVector TextService::svWindows1252Utf8HexEncoding;
IntVectorSorter TextService::ivsWindows1252ControlCharUtf8CodeSorter;
TextService TextService::textServiceGlobalInitializer;
