// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTextService.h"

const ALString KWTextService::ByteStringToWord(const ALString& sByteString)
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

const ALString KWTextService::WordToByteString(const ALString& sWord)
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

const ALString KWTextService::ByteStringToHexCharString(const ALString& sByteString)
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

const ALString KWTextService::HexCharStringToByteString(const ALString& sHexCharString)
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

const ALString KWTextService::ToPrintable(const ALString& sBytes)
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

int KWTextService::GetValidUTF8CharLengthAt(const ALString& sValue, int nStart)
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

void KWTextService::BuildTextSample(StringVector* svTextValues)
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

void KWTextService::Test()
{
	int nEncodingNumber = 10000000;
	StringVector svTextValues;
	ALString sValue;
	ALString sWord;
	ALString sHexaString;
	ALString sRetrievedValue;
	int i;
	int j;
	int nLength;
	Timer timer;
	ALString sTmp;

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