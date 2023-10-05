// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTime.h"

///////////////////////////////////////////////////////////////////////
// Classe Time

boolean Time::Init(int nHour, int nMinute, double dSecond)
{
	boolean bOk = true;

	// Initialisation a invalide
	Reset();

	// Verification de l'heure
	if (nHour < 0 or nHour >= 24)
		bOk = false;
	// Verification de la minute
	else if (nMinute < 0 or nMinute >= 60)
		bOk = false;
	// Verification de la seconde
	else if (dSecond < 0 or dSecond >= 60)
		bOk = false;

	// Initialisation si Ok
	if (bOk)
	{
		SetHour(nHour);
		SetMinute(nMinute);
		SetSecond(dSecond);
	}
	return bOk;
}

void Time::SetCurrentTime()
{
	time_t lCurrentTime;
	struct tm* TimeCurrent;

	// Recherche de la Time courante
	time(&lCurrentTime);
	TimeCurrent = p_localtime(&lCurrentTime);

	// Memorisation de la Time
	Init(TimeCurrent->tm_hour, TimeCurrent->tm_min, TimeCurrent->tm_sec);
}

const char* const Time::ToString() const
{
	char* sTime = StandardGetBuffer();
	int nSecondFrac;
	int i;
	int nLength;

	if (not Check())
		sTime[0] = '\0';
	else
	{
		nSecondFrac = timeValue.timeFields.nFrac;

		// Cas ou il n'y a pas de fraction de secondes
		if (nSecondFrac == 0)
			snprintf(sTime, BUFFER_LENGTH, "%02d:%02d:%02d", GetHour(), GetMinute(),
				 (int)timeValue.timeFields.nSecond);
		// Cas avec fraction de secondes
		else
		{
			nLength = snprintf(sTime, BUFFER_LENGTH, "%02d:%02d:%02d.%04d", GetHour(), GetMinute(),
					   (int)timeValue.timeFields.nSecond, nSecondFrac);

			// Supression des zero en fin pour ne garder que la partie utile des decimales de secondes
			for (i = 0; i < 4; i++)
			{
				if (sTime[nLength - 1 - i] == '0')
					sTime[nLength - 1 - i] = '\0';
				else
					break;
			}
		}
	}
	return sTime;
}

void Time::Write(ostream& ost) const
{
	ost << ToString();
}

void Time::UnitTest(int nHour, int nMinute, double dSecond)
{
	Time tmValue;
	Time tmCopy;

	tmValue.Init(nHour, nMinute, dSecond);
	tmCopy = tmValue;
	cout << "(" << nHour << ", " << nMinute << ", " << dSecond << ")"
	     << "\t";
	cout << tmValue << "\t";
	cout << tmCopy << "\t";
	if (tmValue.Check())
		cout << tmValue.GetDaySecond();
	cout << "\t";
	if (tmValue.Check())
		cout << 86400 - tmValue.GetDaySecond();
	cout << "\t";
	cout << tmValue.Compare(tmCopy) << "\t";
	tmCopy.Init(12, 0, 0);
	cout << tmValue.Compare(tmCopy) << "\t";
	cout << endl;
}

void Time::Test()
{
	Time tmOrigin;
	Time tmCurrent;
	int nHour;
	int nMinute;
	int nSecond;
	int nTotalTimeNumber;

	cout << "sizeof(Time): " << sizeof(Time) << endl;
	tmCurrent.SetCurrentTime();
	cout << "SYSTEM\tCurrent Time\t" << tmCurrent << "\t" << tmCurrent.GetDaySecond() << endl;

	// Parcours de toutes les Times d'une journee
	tmOrigin.Init(0, 0, 0);
	nTotalTimeNumber = 0;
	for (nHour = 0; nHour < 24; nHour++)
	{
		for (nMinute = 0; nMinute < 60; nMinute++)
		{
			for (nSecond = 0; nSecond < 60; nSecond++)
			{
				tmCurrent.Init(nHour, nMinute, nSecond);
				if (tmCurrent != tmOrigin)
					nTotalTimeNumber++;
				assert(fabs(nTotalTimeNumber - tmCurrent.GetDaySecond()) < 1e-5);
			}
		}
	}
	cout << "Times in a day\t" << nTotalTimeNumber + 1 << endl;
	cout << endl;

	// Test de quelques Times
	cout << "Initial\tTime\tCopy\tT. seconds\tRest\tComp\tComp(12:00:00)" << endl;
	UnitTest(0, 0, 0);
	UnitTest(0, 1, 1);
	UnitTest(1, 1, 1);
	UnitTest(11, 0, 0);
	UnitTest(12, 0, 0);
	UnitTest(13, 0, 0);
	UnitTest(23, 59, 59);
	UnitTest(23, 59, 60);
	UnitTest(23, 59, 59.9);
	UnitTest(23, 59, 59.99);
	UnitTest(23, 59, 59.999);
	UnitTest(23, 59, 59.9999);
	UnitTest(23, 59, 59.99999);
	UnitTest(23, 59, 59.09999);
	UnitTest(23, 59, 59.00999);
	UnitTest(23, 59, 59.00099);
	UnitTest(23, 59, 59.00009);
	UnitTest(23, 59, 59.000009);
	UnitTest(24, 0, 0);
}

///////////////////////////////////////////////////////////////////////
// Classe KWTimeFormat

KWTimeFormat::KWTimeFormat()
{
	Reset();
}

KWTimeFormat::~KWTimeFormat() {}

boolean KWTimeFormat::SetFormatString(const ALString& sValue)
{
	boolean bCheck = true;
	int nTotalMainCharNumber;

	// Memorisation de la chaine de format
	sFormatString = sValue;

	// Taille de la chaine de format
	nTotalCharNumber = sValue.GetLength();
	nTotalMainCharNumber = nTotalCharNumber;

	// Test de la longueur
	if (bCheck)
	{
		// Longueur peut aller de 4 (HHMM) a 15 ((H)H:(M)M:(S)S.) en passant par tous
		// les intermediaires
		bCheck = (4 <= nTotalCharNumber and nTotalCharNumber <= 15);
	}

	// Test du separateur decimal, optionnel en fin de chaine de format
	if (bCheck)
	{
		// Le dernier caractere peut etre un '.' (partie decimale des secondes)
		nDecimalPointOffset = -1;
		if (sValue.GetAt(nTotalCharNumber - 1) == '.')
		{
			nDecimalPointOffset = nTotalCharNumber - 1;
			nTotalMainCharNumber--;
		}
	}

	// Recherche des motifs dans la chaine
	if (bCheck)
	{
		// Cas avec premier chiffre facultatif
		if (sValue.GetAt(0) == '(')
		{
			bMandatoryFirstDigit = false;
			nHourOffset = sValue.Find("(H)H");
			nMinuteOffset = sValue.Find("(M)M");
			nSecondOffset = sValue.Find("(S)S");
		}
		// Cas premier chiffre obligatoire
		else
		{
			bMandatoryFirstDigit = true;
			nHourOffset = sValue.Find("HH");
			nMinuteOffset = sValue.Find("MM");
			nSecondOffset = sValue.Find("SS");
		}

		// Test de validite: HH et MM sont obligatoires, l'ordre est impose
		bCheck = nHourOffset == 0 and nMinuteOffset > nHourOffset and
			 (nSecondOffset == -1 or nSecondOffset > nMinuteOffset);

		// Il ne peut y avoir de separateur des secondes que s'il y a des secondes
		if (bCheck)
			bCheck = (nDecimalPointOffset == -1 or nSecondOffset != -1);
	}

	// Recherche des separateurs
	if (bCheck)
	{
		// Cas avec premier chiffre facultatif
		if (not bMandatoryFirstDigit)
		{
			// Il doit y avoir un separateur
			bCheck = (nTotalMainCharNumber == 9 or nTotalMainCharNumber == 14);

			// Verification des separateurs
			if (bCheck)
			{
				// Le separateur doit apparaitre en positions 4 et 9
				nSeparatorOffset1 = 4;
				nSeparatorOffset2 = 9;
				cSeparatorChar = sValue.GetAt(nSeparatorOffset1);

				// Pas de deuxieme separateur si pas de secondes
				if (nTotalMainCharNumber == 9)
					nSeparatorOffset2 = -1;

				// Test de validite
				bCheck = (cSeparatorChar == ':' or cSeparatorChar == '.');
				if (nSeparatorOffset2 != -1)
					bCheck = bCheck and sValue.GetAt(nSeparatorOffset2) == cSeparatorChar;
			}
		}
		// Cas premier chiffre obligatoire
		else
		{
			// Cas sans separateur
			if (nTotalMainCharNumber == 4 or nTotalMainCharNumber == 6)
			{
				cSeparatorChar = '\0';
				nSeparatorOffset1 = -1;
				nSeparatorOffset2 = -1;
				assert(nMinuteOffset == 2);
				assert(nTotalMainCharNumber == 4 or nSecondOffset == 4);
				assert(nTotalMainCharNumber == 6 or nSecondOffset == -1);
			}
			// Cas avec separateur
			else if (nTotalMainCharNumber == 5 or nTotalMainCharNumber == 8)
			{
				// Le separateur doit apparaitre en positions 2 et 5
				nSeparatorOffset1 = 2;
				nSeparatorOffset2 = 5;
				cSeparatorChar = sValue.GetAt(nSeparatorOffset1);

				// Pas de deuxieme separateur si pas de secondes
				if (nTotalMainCharNumber == 5)
					nSeparatorOffset2 = -1;

				// Test de validite
				bCheck = (cSeparatorChar == ':' or cSeparatorChar == '.');
				if (nSeparatorOffset2 != -1)
					bCheck = bCheck and sValue.GetAt(nSeparatorOffset2) == cSeparatorChar;
			}
			// Erreur sinon
			else
				bCheck = false;
		}
	}

	// Calcul des nombre de caractere min et max
	if (bCheck)
	{
		// Calcul du nombre de caracteres min
		// Cas avec premier chiffre obligatoire
		if (bMandatoryFirstDigit)
		{
			nMinCharNumber = 4;
			if (nSeparatorOffset1 != -1)
				nMinCharNumber++;
			if (nSecondOffset != -1)
			{
				nMinCharNumber += 2;
				if (nSeparatorOffset2 != -1)
					nMinCharNumber++;
			}
		}
		// Cas sans premier chiffre obligatoire
		else
		{
			nMinCharNumber = 3;
			if (nSecondOffset != -1)
				nMinCharNumber += 2;
		}

		// Calcul du nombre de caracteres min
		// Cas avec premier chiffre obligatoire
		if (bMandatoryFirstDigit)
			nMaxCharNumber = nMinCharNumber;
		// Cas sans premier chiffre obligatoire
		else
		{
			nMaxCharNumber = 5;
			if (nSecondOffset != -1)
				nMaxCharNumber += 3;
		}
	}

	// Reinitialisation des caracteristiques du format si invalide
	if (not bCheck)
		Reset();
	return bCheck;
}

void KWTimeFormat::Reset()
{
	sFormatString = "";
	bMandatoryFirstDigit = true;
	nTotalCharNumber = 0;
	cSeparatorChar = '\0';
	nSeparatorOffset1 = 0;
	nSeparatorOffset2 = 0;
	nHourOffset = 0;
	nMinuteOffset = 0;
	nSecondOffset = 0;
	nDecimalPointOffset = 0;
	nMinCharNumber = 0;
	nMaxCharNumber = 0;
}

boolean KWTimeFormat::IsConsistentWith(const KWTimeFormat* otherFormat) const
{
	boolean bOk = true;

	require(Check() and otherFormat->Check());

	bOk = bOk and GetSeparatorChar() == otherFormat->GetSeparatorChar();
	if (not GetMandatoryFirstDigit())
		bOk = bOk and not otherFormat->GetMandatoryFirstDigit();
	if (GetSecondOffset() == -1)
		bOk = bOk and otherFormat->GetSecondOffset() == -1;
	else
		bOk = bOk and otherFormat->GetSecondOffset() != -1;
	if (GetDecimalPointOffset() != -1)
		bOk = bOk and otherFormat->GetDecimalPointOffset() != -1;
	return bOk;
}

const ALString& KWTimeFormat::GetFormatString() const
{
	return sFormatString;
}

Time KWTimeFormat::StringToTime(const char* const sValue) const
{
	boolean bCheck = true;
	Time tmConvertedString;
	int nLength;
	int nHour;
	int nMinute;
	int nSecond;
	int nSecondFrac;
	char cChar0;
	char cChar1;
	int i;
	int nUnit;
	int nOffset;
	char cChar;

	require(Check());
	require(sValue != NULL);

	// Initialisation
	tmConvertedString.Reset();
	nHour = 0;
	nMinute = 0;
	nSecond = 0;
	nSecondFrac = 0;

	// Acces a la longueur de la valeur a convertir
	nLength = (int)strlen(sValue);

	//////////////////////////////////////////////
	// Cas avec premier chiffre facultatif
	if (not bMandatoryFirstDigit)
	{
		// Test si longueur compatible
		if (bCheck)
			bCheck = (nLength >= nMinCharNumber);

		// Recherche de l'heure
		nOffset = 0;
		if (bCheck)
		{
			// Premier digit des heures
			cChar = sValue[nOffset];
			nOffset++;
			if (not isdigit(cChar))
				bCheck = false;
			else
			{
				nHour = cChar - '0';

				// Deuxieme digit: unite des heures, ou separateur
				cChar = sValue[nOffset];
				nOffset++;
				if (isdigit(cChar))
				{
					nHour = nHour * 10 + cChar - '0';

					// Lecture du caractere suivant
					assert(nOffset < nLength);
					cChar = sValue[nOffset];
					nOffset++;
				}
				if (cChar != cSeparatorChar)
					bCheck = false;
			}
		}

		// Recherche de la minute
		bCheck = bCheck and nOffset < nLength;
		if (bCheck)
		{
			// Premier digit des minutes
			cChar = sValue[nOffset];
			nOffset++;
			if (not isdigit(cChar))
				bCheck = false;
			else
			{
				nMinute = cChar - '0';

				// Deuxieme digit: unite des minutes, ou separateur s'il y a des secondes
				if (nOffset < nLength)
				{
					cChar = sValue[nOffset];
					nOffset++;
					if (isdigit(cChar))
					{
						nMinute = nMinute * 10 + cChar - '0';

						// Lecture du caractere suivant (potentiellement '\0')
						cChar = sValue[nOffset];
						nOffset++;
					}
					if (nSecondOffset == -1)
						bCheck = (cChar == '\0');
					else
						bCheck = (cChar == cSeparatorChar);
				}
			}
		}

		// Recherche de la seconde
		bCheck = bCheck and (nSecondOffset == -1 or nOffset < nLength);
		if (bCheck and nSecondOffset != -1)
		{
			// Premier digit des secondes
			cChar = sValue[nOffset];
			nOffset++;
			if (not isdigit(cChar))
				bCheck = false;
			else
			{
				nSecond = cChar - '0';

				// Deuxieme digit: unite des secondes, ou separateur s'il y a des fractions de secondes
				if (nOffset < nLength)
				{
					cChar = sValue[nOffset];
					nOffset++;
					if (isdigit(cChar))
					{
						nSecond = nSecond * 10 + cChar - '0';

						// Lecture du caractere suivant (potentiellement '\0')
						cChar = sValue[nOffset];
						nOffset++;
					}
					if (nDecimalPointOffset == -1)
						bCheck = (cChar == '\0');
					else
						bCheck = (cChar == '.' or cChar == '\0');
				}
			}
		}

		// Recherche de la fraction de seconde
		if (bCheck and nDecimalPointOffset != -1 and nOffset < nLength)
		{
			assert(sValue[nOffset - 1] == '.');

			// Parcours de la fin de chaine a partir du sepateur decimal des secondes
			nUnit = 1000;
			for (i = nOffset; i < nLength; i++)
			{
				cChar0 = sValue[i];
				bCheck = isdigit(cChar0);
				if (bCheck)
				{
					// On ne prend en compte que les decimales utiles au 1/10000 ieme
					if (nUnit > 0)
					{
						nSecondFrac += nUnit * (cChar0 - '0');
						nUnit /= 10;
					}
				}
				else
					break;
			}
		}
	}
	//////////////////////////////////////////////
	// Cas avec premier chiffre facultatif
	else
	{
		// Test si longueur compatible
		if (bCheck)
		{
			if (nDecimalPointOffset == -1)
				bCheck = (nLength == nMinCharNumber);
			else
				bCheck = (nLength >= nMinCharNumber);
		}

		// Test du separateur decimal, optionnel en fin de chaine de format
		if (Check())
		{
			if (nDecimalPointOffset != -1 and nLength > nMinCharNumber)
				bCheck = sValue[nDecimalPointOffset] == '.';
		}

		// Test des caracteres separateurs
		if (bCheck)
		{
			if (cSeparatorChar != '\0')
			{
				bCheck = sValue[nSeparatorOffset1] == cSeparatorChar;
				if (nSeparatorOffset2 != -1)
					bCheck = bCheck and sValue[nSeparatorOffset2] == cSeparatorChar;
			}
		}

		// Recherche de l'heure
		if (bCheck)
		{
			cChar0 = sValue[nHourOffset];
			cChar1 = sValue[nHourOffset + 1];

			// Test de validite
			bCheck = isdigit(cChar0) and isdigit(cChar1);
			if (bCheck)
				nHour = 10 * (cChar0 - '0') + (cChar1 - '0');
		}

		// Recherche de la minute
		if (bCheck)
		{
			cChar0 = sValue[nMinuteOffset];
			cChar1 = sValue[nMinuteOffset + 1];

			// Test de validite
			bCheck = isdigit(cChar0) and isdigit(cChar1);
			if (bCheck)
				nMinute = 10 * (cChar0 - '0') + (cChar1 - '0');
		}

		// Recherche de la seconde
		if (bCheck and nSecondOffset != -1)
		{
			cChar0 = sValue[nSecondOffset];
			cChar1 = sValue[nSecondOffset + 1];

			// Test de validite
			bCheck = isdigit(cChar0) and isdigit(cChar1);
			if (bCheck)
				nSecond = 10 * (cChar0 - '0') + (cChar1 - '0');

			// Recherche de la fraction de seconde
			if (bCheck and nDecimalPointOffset != -1)
			{
				// Parcours de la fin de chaine a partir du sepateur decimal des secondes
				nUnit = 1000;
				for (i = nDecimalPointOffset + 1; i < nLength; i++)
				{
					cChar0 = sValue[i];
					bCheck = isdigit(cChar0);
					if (bCheck)
					{
						// On ne prend en compte que les decimales utiles au 1/10000 ieme
						if (nUnit > 0)
						{
							nSecondFrac += nUnit * (cChar0 - '0');
							nUnit /= 10;
						}
					}
					else
						break;
				}
			}
		}
	}

	// Conversion vers la Time
	if (bCheck)
		tmConvertedString.Init(nHour, nMinute, nSecond + nSecondFrac / 10000.0);

	return tmConvertedString;
}

const char* const KWTimeFormat::TimeToString(Time tmValue) const
{
	char* sBuffer = StandardGetBuffer();
	int nOffset;
	const double dEpsilon = 1e-5;
	int nSecond;
	int nSecondFrac;
	int i;

	require(Check());

	// La valeur manquante est convertie en chaine vide
	if (not tmValue.Check())
		sBuffer[0] = '\0';
	else
	{
		// Ecriture du champ heure
		nOffset = 0;
		assert(nOffset == nHourOffset);
		if (bMandatoryFirstDigit)
			nOffset += snprintf(sBuffer, BUFFER_LENGTH, "%02d", tmValue.GetHour());
		else
			nOffset += snprintf(sBuffer, BUFFER_LENGTH, "%d", tmValue.GetHour());

		// Ecriture du premier separateur optionnel
		if (nSeparatorOffset1 != -1)
		{
			sBuffer[nOffset] = cSeparatorChar;
			nOffset++;
		}

		// Ecriture du champ minute
		if (bMandatoryFirstDigit)
			nOffset += snprintf(sBuffer + nOffset, BUFFER_LENGTH - nOffset, "%02d", tmValue.GetMinute());
		else
			nOffset += snprintf(sBuffer + nOffset, BUFFER_LENGTH - nOffset, "%d", tmValue.GetMinute());

		// Ecriture du deuxieme separateur optionnel
		if (nSeparatorOffset2 != -1)
		{
			sBuffer[nOffset] = cSeparatorChar;
			nOffset++;
		}

		// Calcul des secondes
		nSecond = (int)floor(tmValue.GetSecond() + dEpsilon);
		if (nSecond == 60)
			nSecond--;

		// Ecriture du champ secondes
		if (nSecondOffset != -1)
		{
			if (bMandatoryFirstDigit)
				nOffset += snprintf(sBuffer + nOffset, BUFFER_LENGTH - nOffset, "%02d",
						    (int)floor(tmValue.GetSecond()));
			else
				nOffset += snprintf(sBuffer + nOffset, BUFFER_LENGTH - nOffset, "%d",
						    (int)floor(tmValue.GetSecond()));
		}

		// Ecriture de la partie decimale des secondes
		if (nDecimalPointOffset != -1)
		{
			// Calcul des fractions de secondes
			nSecondFrac = (int)floor((tmValue.GetSecond() - nSecond + dEpsilon) * 10000);
			if (nSecondFrac == 10000)
				nSecondFrac--;

			// Ecriture des fractions de secondes
			if (nSecondFrac > 0)
			{
				nOffset += snprintf(sBuffer + nOffset, BUFFER_LENGTH - nOffset, ".%04d", nSecondFrac);

				// Supression des zero en fin pour ne garder que la partie utile des decimales de
				// secondes
				for (i = 0; i < 4; i++)
				{
					if (sBuffer[nOffset - 1 - i] == '0')
						sBuffer[nOffset - 1 - i] = '\0';
					else
						break;
				}
			}
		}
	}
	return sBuffer;
}

void KWTimeFormat::Write(ostream& ost) const
{
	ost << GetClassLabel() << " " << GetObjectLabel();
}

const ALString KWTimeFormat::GetClassLabel() const
{
	return "Time format";
}

const ALString KWTimeFormat::GetObjectLabel() const
{
	return sFormatString;
}

ALString KWTimeFormat::GetDefaultFormatString()
{
	return "HH:MM:SS.";
}

void KWTimeFormat::GetAllAvailableFormats(ObjectArray* oaAvailableFormats)
{
	require(oaAvailableFormats != NULL);
	require(oaAvailableFormats->GetSize() == 0);

	// Ajout de tous les formats de date par ordre de priorite
	AddFormat(oaAvailableFormats, "HH:MM:SS");
	AddFormat(oaAvailableFormats, "HH.MM.SS");
	AddFormat(oaAvailableFormats, "(H)H:(M)M:(S)S");
	AddFormat(oaAvailableFormats, "(H)H.(M)M.(S)S");
	AddFormat(oaAvailableFormats, "HHMMSS");
	AddFormat(oaAvailableFormats, "HH:MM:SS.");
	AddFormat(oaAvailableFormats, "HH.MM.SS.");
	AddFormat(oaAvailableFormats, "(H)H:(M)M:(S)S.");
	AddFormat(oaAvailableFormats, "(H)H.(M)M.(S)S.");
	AddFormat(oaAvailableFormats, "HHMMSS.");
	AddFormat(oaAvailableFormats, "HH:MM");
	AddFormat(oaAvailableFormats, "HH.MM");
	AddFormat(oaAvailableFormats, "(H)H:(M)M");
	AddFormat(oaAvailableFormats, "(H)H.(M)M");
	AddFormat(oaAvailableFormats, "HHMM");
}

void KWTimeFormat::AddFormat(ObjectArray* oaFormats, const ALString& sFormatString)
{
	KWTimeFormat* format;

	require(oaFormats != NULL);

	format = new KWTimeFormat;
	format->SetFormatString(sFormatString);
	assert(format->Check());
	oaFormats->Add(format);
}

void KWTimeFormat::UnitTest(const ALString& sInputValue, KWTimeFormat* inputFormat, KWTimeFormat* outputFormat)
{
	boolean bOk = true;
	boolean bShow = false;
	Time tmInputValue;
	Time tmOutputValue;
	ALString sOutputValue;

	require(inputFormat != NULL);
	require(outputFormat != NULL);

	tmInputValue.Reset();
	tmOutputValue.Reset();
	if (inputFormat->Check())
		tmInputValue = inputFormat->StringToTime(sInputValue);
	if (outputFormat->Check())
		sOutputValue = outputFormat->TimeToString(tmInputValue);
	if (outputFormat->Check())
		tmOutputValue = outputFormat->StringToTime(sOutputValue);
	bOk = (not inputFormat->Check() or not outputFormat->Check() or
	       (not tmInputValue.Check() and not tmOutputValue.Check()) or
	       (tmOutputValue.GetHour() == tmInputValue.GetHour() and
		tmOutputValue.GetMinute() == tmInputValue.GetMinute()));
	if (bShow or not bOk)
	{
		cout << sInputValue << "\t";
		cout << inputFormat->GetFormatString() << "\t";
		cout << inputFormat->TimeToString(tmInputValue) << "\t";
		cout << outputFormat->GetFormatString() << "\t";
		cout << outputFormat->TimeToString(tmInputValue) << "\t";
		cout << tmOutputValue;
		if (not bOk)
			cout << "\tError";
		cout << endl;
	}
}

void KWTimeFormat::Test()
{
	ObjectArray oaAvailableTimeFormats;
	StringVector svStringTimes;
	KWTimeFormat inputFormat;
	KWTimeFormat* outputFormat;
	int nFormat;
	int i;

	// Recherche de tous les formats disponibles
	GetAllAvailableFormats(&oaAvailableTimeFormats);

	// Creation des times a tester
	svStringTimes.Add("000000");
	svStringTimes.Add("000101");
	svStringTimes.Add("010101");
	svStringTimes.Add("110000");
	svStringTimes.Add("120000");
	svStringTimes.Add("130000");
	svStringTimes.Add("235959");
	svStringTimes.Add("235959.");
	svStringTimes.Add("235959.9");
	svStringTimes.Add("235959.99");
	svStringTimes.Add("235959.999");
	svStringTimes.Add("235959.9999");
	svStringTimes.Add("235959.99999");
	svStringTimes.Add("235959.09999");
	svStringTimes.Add("235959.00999");
	svStringTimes.Add("235959.00099");
	svStringTimes.Add("235959.00009");
	svStringTimes.Add("235959.000009");
	svStringTimes.Add("000000.1");
	svStringTimes.Add("000001.1");
	svStringTimes.Add("000010.1");
	svStringTimes.Add("000100.1");
	svStringTimes.Add("001000.1");
	svStringTimes.Add("010000.1");
	svStringTimes.Add("100000.1");
	svStringTimes.Add("00000");
	svStringTimes.Add("235960");
	svStringTimes.Add("240000");

	// Affichages des times testees
	cout << "Times used for conversion tests" << endl;
	for (i = 0; i < svStringTimes.GetSize(); i++)
		cout << "\t" << svStringTimes.GetAt(i) << endl;

	// Test des formats sur un ensemble de times
	inputFormat.SetFormatString("HHMMSS.");
	cout << "\nFormat number\t" << oaAvailableTimeFormats.GetSize() << endl;
	for (nFormat = 0; nFormat < oaAvailableTimeFormats.GetSize(); nFormat++)
	{
		outputFormat = cast(KWTimeFormat*, oaAvailableTimeFormats.GetAt(nFormat));
		cout << "\t" << *outputFormat << endl;

		// Tests de conversion
		for (i = 0; i < svStringTimes.GetSize(); i++)
		{
			// Affichage des details de test uniquement en cas d'erreur
			UnitTest(svStringTimes.GetAt(i), &inputFormat, outputFormat);
		}
	}

	// Nettoyage
	oaAvailableTimeFormats.DeleteAll();
}
