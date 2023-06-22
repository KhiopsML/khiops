// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDate.h"

///////////////////////////////////////////////////////////////////////
// Classe Date

boolean Date::Init(int nYear, int nMonth, int nDay)
{
	boolean bOk = true;

	// Initialisation a invalide
	Reset();

	// L'annee doit etre comprise entre 1 et 9999
	if (nYear < 1 or nYear > 9999)
		bOk = false;
	// Le mois doit etre compris entre 1 et 12
	else if (nMonth < 1 or nMonth > 12)
		bOk = false;
	// Le jour doit etre compris entre 1 et 31
	else if (nDay < 1 or nDay > 31)
		bOk = false;
	// Mois de 30 jours
	else if (nMonth == 4 or nMonth == 6 or nMonth == 9 or nMonth == 11)
		bOk = (nDay <= 30);
	// Cas particulier du mois de fevrier
	else if (nMonth == 2)
	{
		// Calcul des annees bissextiles
		if (nYear % 4 == 0 and (nYear % 100 != 0 or nYear % 400 == 0))
			bOk = (nDay <= 29);
		else
			bOk = (nDay <= 28);
	}

	// Initialisation si Ok
	if (bOk)
	{
		SetYear(nYear);
		SetMonth(nMonth);
		SetDay(nDay);
	}
	return bOk;
}

boolean Date::IsBissextileYear() const
{
	int nYear;

	require(Check());

	nYear = GetYear();
	return (nYear % 4 == 0 and (nYear % 100 != 0 or nYear % 400 == 0));
}

int Date::GetYearDay() const
{
	int nYearDay = 0;
	int nYear;
	int nMonth;
	int nDay;

	require(Check());

	// Recherche de l'an, du mois et du jour
	nYear = GetYear();
	nMonth = GetMonth();
	nDay = GetDay();

	// Calcul du jour dans l'annee (offset pour debut de mois)
	switch (nMonth)
	{
	case 1:
		nYearDay = 0;
		break;
	case 2:
		nYearDay = 31;
		break;
	case 3:
		nYearDay = 59;
		break;
	case 4:
		nYearDay = 90;
		break;
	case 5:
		nYearDay = 120;
		break;
	case 6:
		nYearDay = 151;
		break;
	case 7:
		nYearDay = 181;
		break;
	case 8:
		nYearDay = 212;
		break;
	case 9:
		nYearDay = 243;
		break;
	case 10:
		nYearDay = 273;
		break;
	case 11:
		nYearDay = 304;
		break;
	default:
		nYearDay = 334;
		break;
	}

	// Ajout des jours du mois
	nYearDay += nDay;

	// Prise en compte des annees bissextiles
	if (nMonth > 2 and nYear % 4 == 0 and (nYear % 100 != 0 or nYear % 400 == 0))
		nYearDay += 1;
	return nYearDay;
}

int Date::GetWeekDay() const
{
	int nWeekDay = 0;
	debug(Date dtReference);

	require(Check());

	// On verifie que le 1 janvier 1900 est bien un lundi
	debug(dtReference.Init(1900, 1, 1));
	debug(assert(1 + (dtReference.GetJulianDayNumber() % 7) == 1));
	nWeekDay = 1 + (GetJulianDayNumber() % 7);
	assert(nWeekDay >= 1);
	return nWeekDay;
}

double Date::GetDecimalYear() const
{
	int nYear;
	int nDayNumber;
	double dDecimalYear;

	nYear = GetYear();
	nDayNumber = 365;
	if (nYear % 4 == 0 and (nYear % 100 != 0 or nYear % 400 == 0))
		nDayNumber += 1;
	dDecimalYear = nYear + (GetYearDay() - 1.0) / nDayNumber;
	return dDecimalYear;
}

int Date::GetAbsoluteDay() const
{
	int nAbsoluteDay = 0;
	debug(Date dtReference);
	const int nReferenceJulianDayNumber = 2451545;

	require(Check());

	// On se base sur la difference du nombres de jours avec le 1 janvier 2000
	debug(dtReference.Init(2000, 1, 1));
	debug(assert(dtReference.GetJulianDayNumber() == nReferenceJulianDayNumber));
	nAbsoluteDay = GetJulianDayNumber() - nReferenceJulianDayNumber;
	return nAbsoluteDay;
}

boolean Date::AddDays(int nValue)
{
	boolean bOk = true;
	int nJulianDayNumber;
	int nYear;
	int nMonth;
	int nDay;
	int f, e, g, h;

	require(Check());

	// Recherche du nombre de jours total
	nJulianDayNumber = GetJulianDayNumber();

	// Ajout du nombre de jours
	nJulianDayNumber += nValue;

	// Conversion du jour Julien en jour Gregorien (cf. http://en.wikipedia.org/wiki/Julian_day)
	f = nJulianDayNumber + 1401;
	f = f + (((4 * nJulianDayNumber + 274277) / 146097) * 3) / 4 - 38;
	e = 4 * f + 3;
	g = (e % 1461) / 4;
	h = 5 * g + 2;
	nDay = (h % 153) / 5 + 1;
	nMonth = (h / 153 + 2) % 12 + 1;
	nYear = e / 1461 - 4716 + (14 - nMonth) / 12;

	// Initialisation si annee valide
	if (1 <= nYear and nYear <= 9999)
	{
		SetYear(nYear);
		SetMonth(nMonth);
		SetDay(nDay);
	}
	// Sinon, reinitialistaion
	else
	{
		Reset();
		bOk = false;
	}
	return bOk;
}

void Date::SetCurrentDate()
{
	time_t lCurrentTime;
	struct tm* dateCurrent;

	// Recherche de la date courante
	time(&lCurrentTime);
	dateCurrent = p_localtime(&lCurrentTime);

	// Memorisation de la date
	Init(dateCurrent->tm_year + 1900, dateCurrent->tm_mon + 1, dateCurrent->tm_mday);
}

const char* const Date::ToString() const
{
	char* sDate = StandardGetBuffer();

	if (not Check())
		sDate[0] = '\0';
	else
		sprintf(sDate, "%04d-%02d-%02d", GetYear(), GetMonth(), GetDay());
	return sDate;
}

void Date::Write(ostream& ost) const
{
	ost << ToString();
}

int Date::GetJulianDayNumber() const
{
	int nJulianDayNumber = -1;
	int nYear;
	int nMonth;
	int nDay;
	int a, y, m;

	require(Check());

	// Recherche de l'an, du mois et du jour
	nYear = GetYear();
	nMonth = GetMonth();
	nDay = GetDay();

	// On utilise le jour Julien comme date absolue
	// cf. http://en.wikipedia.org/wiki/Julian_day
	a = (14 - nMonth) / 12;
	y = nYear + 4800 - a;
	m = nMonth + 12 * a - 3;
	nJulianDayNumber = nDay + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
	return nJulianDayNumber;
}

void Date::UnitTest(int nYear, int nMonth, int nDay)
{
	Date dtValue;
	Date dtCopy;

	dtValue.Init(nYear, nMonth, nDay);
	dtCopy = dtValue;
	cout << "(" << nYear << ", " << nMonth << ", " << nDay << ")"
	     << "\t";
	cout << dtValue << "\t";
	cout << dtCopy << "\t";
	if (dtValue.Check())
		cout << dtValue.GetYearDay();
	cout << "\t";
	if (dtValue.Check())
		cout << dtValue.GetWeekDay();
	cout << "\t";
	if (dtValue.Check())
		cout << dtValue.GetAbsoluteDay();
	cout << "\t";
	if (dtCopy.Check())
		dtCopy.AddDays(0);
	cout << dtCopy << "\t";
	cout << dtValue.Compare(dtCopy) << "\t";
	if (dtCopy.Check())
		dtCopy.AddDays(364);
	cout << dtCopy << "\t";
	cout << dtValue.Compare(dtCopy) << "\t";
	if (dtCopy.Check())
		dtCopy.AddDays(1);
	cout << dtCopy << "\t";
	cout << dtValue.Compare(dtCopy) << "\t";
	if (dtCopy.Check())
		dtCopy.AddDays(1);
	cout << dtCopy << "\t";
	cout << dtValue.Compare(dtCopy) << "\t";
	cout << endl;
}

void Date::Test()
{
	Date dtOrigin;
	Date dtCurrent;
	Date dtPreviousDate;
	Date dtDate;
	int nOriginAbsoluteDay;
	int nCurrentAbsoluteDay;
	int i;
	int nTotalYearNumber;
	int nTotalMonthNumber;
	int nTotalDayNumber;
	int nTotalLeapDayNumber;

	cout << "sizeof(Date): " << sizeof(Date) << endl;
	dtCurrent.SetCurrentDate();
	cout << "SYSTEM\tCurrent date\t" << dtCurrent << "\t" << dtCurrent.GetWeekDay() << endl;

	// Parcours de toutes les dates depuis la date origine jusqu'a la date actuelle
	// et verifications de coherence
	dtOrigin.Init(1, 1, 1);
	dtCurrent.Init(2013, 04, 15);
	nOriginAbsoluteDay = dtOrigin.GetAbsoluteDay();
	nCurrentAbsoluteDay = dtCurrent.GetAbsoluteDay();
	cout << "Start date\t" << dtOrigin << endl;
	cout << "End date\t" << dtCurrent << endl;
	nOriginAbsoluteDay = dtOrigin.GetAbsoluteDay();
	nCurrentAbsoluteDay = dtCurrent.GetAbsoluteDay();
	dtPreviousDate = dtOrigin;
	nTotalYearNumber = 1;
	nTotalMonthNumber = 1;
	nTotalDayNumber = 1;
	nTotalLeapDayNumber = 0;
	for (i = nOriginAbsoluteDay + 1; i <= nCurrentAbsoluteDay; i++)
	{
		dtDate = dtOrigin;
		dtDate.AddDays(i - nOriginAbsoluteDay);
		assert(dtDate.GetAbsoluteDay() == i);
		if (dtDate.GetYear() != dtPreviousDate.GetYear())
			nTotalYearNumber++;
		if (dtDate.GetMonth() != dtPreviousDate.GetMonth())
			nTotalMonthNumber++;
		if (dtDate.GetDay() != dtPreviousDate.GetDay())
			nTotalDayNumber++;
		if (dtDate.GetMonth() == 2 and dtDate.GetDay() == 29)
			nTotalLeapDayNumber++;
		dtPreviousDate = dtDate;
	}
	cout << "\tUnit\tRef\tResult" << endl;
	cout << "\tYears\t2013\t" << nTotalYearNumber << endl;
	cout << "\tMonths\t24148\t" << nTotalMonthNumber << endl;
	cout << "\tDays\t734973\t" << nTotalDayNumber << endl;
	cout << "\tLeap days\t488\t" << nTotalLeapDayNumber << endl;
	cout << endl;

	// Test de quelques dates
	cout << "Initial\tDate\tCopy\tYDay\tWDay\tAbsDay\t+0j\tComp\t+364j\tComp\t+365j\tComp\t+366j\tComp" << endl;
	UnitTest(0, 0, 0);
	UnitTest(0, 1, 1);
	UnitTest(1, 1, 1);
	UnitTest(1900, 1, 1);
	UnitTest(1999, 1, 1);
	UnitTest(2000, 1, 1);
	UnitTest(2001, 1, 1);
	UnitTest(2002, 1, 1);
	UnitTest(2003, 1, 1);
	UnitTest(2004, 1, 1);
	UnitTest(2013, 4, 5);
	UnitTest(1, 12, 31);
	UnitTest(1900, 12, 31);
	UnitTest(1999, 12, 31);
	UnitTest(2000, 12, 31);
	UnitTest(2001, 12, 31);
	UnitTest(2002, 12, 31);
	UnitTest(2003, 12, 31);
	UnitTest(2004, 12, 31);
	UnitTest(2013, 4, 5);
	UnitTest(9999, 1, 1);
}

///////////////////////////////////////////////////////////////////////
// Classe KWDateFormat

KWDateFormat::KWDateFormat()
{
	nTotalCharNumber = 0;
	cSeparatorChar = '\0';
	nSeparatorOffset1 = 0;
	nSeparatorOffset2 = 0;
	nYearOffset = 0;
	nMonthOffset = 0;
	nDayOffset = 0;
}

KWDateFormat::~KWDateFormat() {}

boolean KWDateFormat::SetFormatString(const ALString& sValue)
{
	boolean bCheck = true;

	// Memorisation de la chaine de format
	sFormatString = sValue;

	// Taille de la chaine de format
	nTotalCharNumber = sValue.GetLength();

	// Test de la longueur
	if (bCheck)
	{
		bCheck = (nTotalCharNumber == 8 or nTotalCharNumber == 10);
	}

	// Recherche des motifs dans la chaine
	if (bCheck)
	{
		nYearOffset = sValue.Find("YYYY");
		nMonthOffset = sValue.Find("MM");
		nDayOffset = sValue.Find("DD");

		// Test de validite
		bCheck = nYearOffset >= 0 and nMonthOffset >= 0 and nDayOffset >= 0 and
			 (nYearOffset == 0 or nYearOffset == nTotalCharNumber - 4);
	}

	// Recherche des separateurs
	if (bCheck)
	{
		// Cas ou il n'y a pas de separateur
		if (nTotalCharNumber == 8)
		{
			cSeparatorChar = '\0';
			nSeparatorOffset1 = -1;
			nSeparatorOffset2 = -1;
			assert((nYearOffset + nMonthOffset + nDayOffset) == 6 or
			       (nYearOffset + nMonthOffset + nDayOffset) == 10);
		}
		// Cas avec separateur
		else
		{
			assert(nTotalCharNumber == 10);

			// Cas ou l'annee est en tete
			if (nYearOffset == 0)
			{
				// Le separateur doit apparaitre en positions 4 et 7
				nSeparatorOffset1 = 4;
				nSeparatorOffset2 = 7;
				cSeparatorChar = sValue.GetAt(nSeparatorOffset1);
			}
			// Sinon, l'annee est en fin
			else
			{
				assert(nYearOffset == nTotalCharNumber - 4);

				// Le separateur doit apparaitre en positions 2 et 5
				nSeparatorOffset1 = 2;
				nSeparatorOffset2 = 5;
				cSeparatorChar = sValue.GetAt(nSeparatorOffset1);
			}

			// Test de validite
			bCheck = sValue.GetAt(nSeparatorOffset2) == cSeparatorChar and
				 (cSeparatorChar == '/' or cSeparatorChar == '.' or cSeparatorChar == '-');
		}
	}

	// Reinitialisation des caracteristiques du format si invalide
	if (not bCheck)
	{
		nTotalCharNumber = 0;
		cSeparatorChar = '\0';
		nSeparatorOffset1 = 0;
		nSeparatorOffset2 = 0;
		nYearOffset = 0;
		nMonthOffset = 0;
		nDayOffset = 0;
	}
	return bCheck;
}

const ALString& KWDateFormat::GetFormatString() const
{
	return sFormatString;
}

boolean KWDateFormat::IsConsistentWith(const KWDateFormat* otherFormat) const
{
	require(Check() and otherFormat->Check());

	// Tous les formats Date sont incompatibles entre eux
	return GetFormatString() == otherFormat->GetFormatString();
}

Date KWDateFormat::StringToDate(const char* const sValue) const
{
	boolean bCheck = true;
	Date dtConvertedString;
	int nLength;
	int nYear;
	int nMonth;
	int nDay;
	char cChar0;
	char cChar1;
	char cChar2;
	char cChar3;

	require(Check());
	require(sValue != NULL);

	// Initialisation
	dtConvertedString.Reset();
	nYear = 0;
	nMonth = 0;
	nDay = 0;

	// Acces a la longueur de la valeur a convertir
	nLength = (int)strlen(sValue);

	// Test si longueur compatible
	if (bCheck)
		bCheck = (nLength == nTotalCharNumber);

	// Test des caracteres separateurs
	if (bCheck)
	{
		if (cSeparatorChar != '\0')
			bCheck =
			    sValue[nSeparatorOffset1] == cSeparatorChar and sValue[nSeparatorOffset2] == cSeparatorChar;
	}

	// Recherche de l'annee
	if (bCheck)
	{
		cChar0 = sValue[nYearOffset];
		cChar1 = sValue[nYearOffset + 1];
		cChar2 = sValue[nYearOffset + 2];
		cChar3 = sValue[nYearOffset + 3];

		// Test de validite
		bCheck = isdigit(cChar0) and isdigit(cChar1) and isdigit(cChar2) and isdigit(cChar3);
		if (bCheck)
			nYear = 1000 * (cChar0 - '0') + 100 * (cChar1 - '0') + 10 * (cChar2 - '0') + (cChar3 - '0');
	}

	// Recherche du mois
	if (bCheck)
	{
		cChar0 = sValue[nMonthOffset];
		cChar1 = sValue[nMonthOffset + 1];

		// Test de validite
		bCheck = isdigit(cChar0) and isdigit(cChar1);
		if (bCheck)
			nMonth = 10 * (cChar0 - '0') + (cChar1 - '0');
	}

	// Recherche du jour
	if (bCheck)
	{
		cChar0 = sValue[nDayOffset];
		cChar1 = sValue[nDayOffset + 1];

		// Test de validite
		bCheck = isdigit(cChar0) and isdigit(cChar1);
		if (bCheck)
			nDay = 10 * (cChar0 - '0') + (cChar1 - '0');
	}

	// Conversion vers la date
	if (bCheck)
		dtConvertedString.Init(nYear, nMonth, nDay);

	return dtConvertedString;
}

const char* const KWDateFormat::DateToString(Date dtValue) const
{
	char* sBuffer = StandardGetBuffer();
	int nOffset;

	require(Check());

	// La valeur manquante est convertie en chaine vide
	if (not dtValue.Check())
		sBuffer[0] = '\0';
	else
	{
		// Ecriture du premier champ
		nOffset = 0;
		assert(nOffset == nYearOffset or nOffset == nMonthOffset or nOffset == nDayOffset);
		if (nYearOffset == 0)
			nOffset += sprintf(sBuffer, "%04d", dtValue.GetYear());
		else if (nMonthOffset == 0)
			nOffset += sprintf(sBuffer, "%02d", dtValue.GetMonth());
		else
			nOffset += sprintf(sBuffer, "%02d", dtValue.GetDay());

		// Ecriture du premier separateur optionnel
		if (nSeparatorOffset1 == nOffset)
		{
			sBuffer[nOffset] = cSeparatorChar;
			nOffset++;
		}

		// Ecriture du deuxieme champ
		assert(nOffset == nYearOffset or nOffset == nMonthOffset or nOffset == nDayOffset);
		if (nYearOffset == nOffset)
			nOffset += sprintf(sBuffer + nOffset, "%04d", dtValue.GetYear());
		else if (nMonthOffset == nOffset)
			nOffset += sprintf(sBuffer + nOffset, "%02d", dtValue.GetMonth());
		else
			nOffset += sprintf(sBuffer + nOffset, "%02d", dtValue.GetDay());

		// Ecriture du deuxiemme separateur optionnel
		if (nSeparatorOffset2 == nOffset)
		{
			sBuffer[nOffset] = cSeparatorChar;
			nOffset++;
		}

		// Ecriture du troisieme champ
		assert(nOffset == nYearOffset or nOffset == nMonthOffset or nOffset == nDayOffset);
		if (nYearOffset == nOffset)
			nOffset += sprintf(sBuffer + nOffset, "%04d", dtValue.GetYear());
		else if (nMonthOffset == nOffset)
			nOffset += sprintf(sBuffer + nOffset, "%02d", dtValue.GetMonth());
		else
			nOffset += sprintf(sBuffer + nOffset, "%02d", dtValue.GetDay());
	}
	return sBuffer;
}

void KWDateFormat::Write(ostream& ost) const
{
	ost << GetClassLabel() << " " << GetObjectLabel();
}

const ALString KWDateFormat::GetClassLabel() const
{
	return "Date format";
}

const ALString KWDateFormat::GetObjectLabel() const
{
	return sFormatString;
}

ALString KWDateFormat::GetDefaultFormatString()
{
	return "YYYY-MM-DD";
}

void KWDateFormat::GetAllAvailableFormats(ObjectArray* oaAvailableFormats)
{
	require(oaAvailableFormats != NULL);
	require(oaAvailableFormats->GetSize() == 0);

	// Ajout de tous les formats de date par ordre de priorite
	AddFormat(oaAvailableFormats, "YYYY-MM-DD");
	AddFormat(oaAvailableFormats, "DD-MM-YYYY");
	AddFormat(oaAvailableFormats, "YYYY-DD-MM");
	AddFormat(oaAvailableFormats, "MM-DD-YYYY");
	AddFormat(oaAvailableFormats, "YYYY/MM/DD");
	AddFormat(oaAvailableFormats, "DD/MM/YYYY");
	AddFormat(oaAvailableFormats, "YYYY/DD/MM");
	AddFormat(oaAvailableFormats, "MM/DD/YYYY");
	AddFormat(oaAvailableFormats, "YYYY.MM.DD");
	AddFormat(oaAvailableFormats, "DD.MM.YYYY");
	AddFormat(oaAvailableFormats, "YYYY.DD.MM");
	AddFormat(oaAvailableFormats, "MM.DD.YYYY");
	AddFormat(oaAvailableFormats, "YYYYMMDD");
	AddFormat(oaAvailableFormats, "DDMMYYYY");
	AddFormat(oaAvailableFormats, "YYYYDDMM");
	AddFormat(oaAvailableFormats, "MMDDYYYY");
}

void KWDateFormat::AddFormat(ObjectArray* oaFormats, const ALString& sFormatString)
{
	KWDateFormat* format;

	require(oaFormats != NULL);

	format = new KWDateFormat;
	format->SetFormatString(sFormatString);
	assert(format->Check());
	oaFormats->Add(format);
}

void KWDateFormat::UnitTest(const ALString& sInputValue, KWDateFormat* inputFormat, KWDateFormat* outputFormat)
{
	boolean bOk = true;
	boolean bShow = false;
	Date dtInputValue;
	Date dtOutputValue;
	ALString sOutputValue;

	require(inputFormat != NULL);
	require(outputFormat != NULL);

	dtInputValue.Reset();
	if (inputFormat->Check())
		dtInputValue = inputFormat->StringToDate(sInputValue);
	if (outputFormat->Check())
		sOutputValue = outputFormat->DateToString(dtInputValue);
	if (outputFormat->Check())
		dtOutputValue = outputFormat->StringToDate(sOutputValue);
	bOk = (not inputFormat->Check() or not outputFormat->Check() or dtOutputValue == dtInputValue);
	if (bShow or not bOk)
	{
		cout << sInputValue << "\t";
		cout << inputFormat->GetFormatString() << "\t";
		cout << inputFormat->DateToString(dtInputValue) << "\t";
		cout << outputFormat->GetFormatString() << "\t";
		cout << outputFormat->DateToString(dtInputValue) << "\t";
		cout << dtOutputValue;
		if (not bOk)
			cout << "\tError";
		cout << endl;
	}
}

void KWDateFormat::Test()
{
	ObjectArray oaAvailableDateFormats;
	StringVector svStringDates;
	KWDateFormat inputFormat;
	KWDateFormat* outputFormat;
	int nFormat;
	int i;

	// Recherche de tous les formats disponibles
	GetAllAvailableFormats(&oaAvailableDateFormats);

	// Creation des dates a tester
	svStringDates.Add("1012031");
	svStringDates.Add("00000000");
	svStringDates.Add("00000101");
	svStringDates.Add("00010101");
	svStringDates.Add("19000101");
	svStringDates.Add("19990101");
	svStringDates.Add("20000101");
	svStringDates.Add("20010101");
	svStringDates.Add("20020101");
	svStringDates.Add("20030101");
	svStringDates.Add("20040101");
	svStringDates.Add("20130405");
	svStringDates.Add("19001231");
	svStringDates.Add("19991231");
	svStringDates.Add("20001231");
	svStringDates.Add("20011231");
	svStringDates.Add("20021231");
	svStringDates.Add("20031231");
	svStringDates.Add("20041231");
	svStringDates.Add("20130405");
	svStringDates.Add("99990101");

	// Affichages des dates testees
	cout << "Dates used for conversion tests" << endl;
	for (i = 0; i < svStringDates.GetSize(); i++)
		cout << "\t" << svStringDates.GetAt(i) << endl;

	// Test des formats sur un ensemble de dates
	inputFormat.SetFormatString("YYYYMMDD");
	cout << "\nFormat number\t" << oaAvailableDateFormats.GetSize() << endl;
	for (nFormat = 0; nFormat < oaAvailableDateFormats.GetSize(); nFormat++)
	{
		outputFormat = cast(KWDateFormat*, oaAvailableDateFormats.GetAt(nFormat));
		cout << "\t" << *outputFormat << endl;

		// Tests de conversion
		for (i = 0; i < svStringDates.GetSize(); i++)
		{
			// Affichage des details de test uniquement en cas d'erreur
			UnitTest(svStringDates.GetAt(i), &inputFormat, outputFormat);
		}
	}

	// Nettoyage
	oaAvailableDateFormats.DeleteAll();
}