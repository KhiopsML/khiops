// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTimestamp.h"

///////////////////////////////////////////////////////////////////////
// Classe Timestamp

boolean Timestamp::Init(int nYear, int nMonth, int nDay, int nHour, int nMinute, double dSecond)
{
	boolean bOk;
	Date dtValue;
	Time tmValue;

	// Initialisation a invalide
	Reset();

	// Initialisation d'un Date et d'une Time
	dtValue.Init(nYear, nMonth, nDay);
	tmValue.Init(nHour, nMinute, dSecond);

	// Test si OK
	bOk = dtValue.Check() and tmValue.Check();

	// Initialisation si Ok
	if (bOk)
	{
		SetDate(dtValue);
		SetTime(tmValue);
	}
	return bOk;
}

double Timestamp::GetDecimalYear() const
{
	int nYear;
	int nDayNumber;
	double dDecimalYear;

	nYear = GetDate().GetYear();
	nDayNumber = 365;
	if (nYear % 4 == 0 and (nYear % 100 != 0 or nYear % 400 == 0))
		nDayNumber += 1;
	dDecimalYear = nYear + (GetDate().GetYearDay() - 1.0 + GetTime().GetDecimalTime() / 24) / nDayNumber;
	return dDecimalYear;
}

boolean Timestamp::AddSeconds(double dSeconds)
{
	boolean bOk = true;
	const double dEpsilon = 1e-5;
	const double dDaySecondNumber = 86400.0;
	int nDays;
	double dDaySeconds;
	double dTotalSeconds;
	int nHour;
	int nMinute;

	require(Check());

	// Nombre de secondes du jour en cours
	dDaySeconds = GetTime().GetDaySecond();

	// Nombre total de seconde ajoute depuis 00:00:00
	dTotalSeconds = dDaySeconds + dSeconds;

	// Calcul du nombre de jours a ajouter ou retrancher
	if (dTotalSeconds >= 0)
		nDays = (int)floor(dTotalSeconds / dDaySecondNumber);
	else
		nDays = -(int)ceil(-dTotalSeconds / dDaySecondNumber);

	// Ajout des jours a la date
	GetInternalDate().AddDays(nDays);

	// Si OK, mise a jour de la Time
	if (GetDate().Check())
	{
		// Calcul du nombre de secondes dans la journee
		dDaySeconds = dTotalSeconds - dDaySecondNumber * nDays;
		assert(-dEpsilon < dDaySeconds and dDaySeconds < dDaySecondNumber + dEpsilon);
		if (dDaySeconds < 0)
			dDaySeconds = 0;
		if (dDaySeconds >= dDaySecondNumber)
			dDaySeconds = dDaySecondNumber - dEpsilon;

		// Calcul des champs de la Time
		nHour = (int)floor(dDaySeconds / 3600.0);
		assert(0 <= nHour and nHour <= 23);
		dDaySeconds -= 3600.0 * nHour;
		nMinute = (int)floor(dDaySeconds / 60.0);
		assert(0 <= nMinute and nMinute <= 59);
		dDaySeconds -= 60.0 * nMinute;
		assert(0 <= dDaySeconds and dDaySeconds < 60);

		// Mise a jour
		GetInternalTime().Init(nHour, nMinute, dDaySeconds);
	}

	// Reinitialistaion si non valide
	if (not bOk)
	{
		Reset();
		bOk = false;
	}
	return bOk;
}

void Timestamp::SetCurrentTimestamp()
{
	GetInternalDate().SetCurrentDate();
	GetInternalTime().SetCurrentTime();
}

const char* const Timestamp::ToString() const
{
	char* sTimestamp = StandardGetBuffer();

	if (not Check())
		sTimestamp[0] = '\0';
	else
		sprintf(sTimestamp, "%s %s", GetDate().ToString(), GetTime().ToString());
	return sTimestamp;
}

void Timestamp::Write(ostream& ost) const
{
	ost << ToString();
}

void Timestamp::UnitTest(int nYear, int nMonth, int nDay, int nHour, int nMinute, double dSecond)
{
	Timestamp tsValue;
	Timestamp tsCopy;

	tsValue.Init(nYear, nMonth, nDay, nHour, nMinute, dSecond);
	tsCopy = tsValue;
	cout << "(" << nYear << ", " << nMonth << ", " << nDay << ", " << nHour << ", " << nMinute << ", " << dSecond
	     << ")"
	     << "\t";
	cout << tsValue << " ,\t";
	cout << tsCopy << " ,\t";
	cout << tsValue.GetDate() << " ,\t";
	cout << tsValue.GetTime() << " ,\t";
	if (tsValue.Check())
	{
		cout << tsValue.GetAbsoluteSecond() << "\t";
		tsCopy = tsValue;
		tsCopy.AddSeconds(0);
		cout << tsCopy << " ,\t" << flush;
		cout << tsValue.Diff(tsCopy) << "\t";
		tsCopy = tsValue;
		tsCopy.AddSeconds(0.1);
		cout << tsCopy << " ,\t";
		cout << tsValue.Diff(tsCopy) << "\t";
		tsCopy = tsValue;
		tsCopy.AddSeconds(1);
		cout << tsCopy << " ,\t";
		cout << tsValue.Diff(tsCopy) << "\t";
		tsCopy = tsValue;
		tsCopy.AddSeconds(60);
		cout << tsCopy << " ,\t";
		cout << tsValue.Diff(tsCopy) << "\t";
		tsCopy = tsValue;
		tsCopy.AddSeconds(3600);
		cout << tsCopy << " ,\t";
		cout << tsValue.Diff(tsCopy) << "\t";
		tsCopy = tsValue;
		tsCopy.AddSeconds(86400);
		cout << tsCopy << " ,\t";
		cout << tsValue.Diff(tsCopy) << "\t";
	}
	cout << endl;
}

void Timestamp::Test()
{
	Timestamp tsOrigin;
	Timestamp tsCurrent;
	Timestamp tsPreviousTimestamp;
	Timestamp tsTimestamp;
	Time tmTime;
	int nOriginAbsoluteDay;
	int nCurrentAbsoluteDay;
	int i;
	int nTotalYearNumber;
	int nTotalMonthNumber;
	int nTotalDayNumber;
	int nTotalLeapDayNumber;
	int nTotalSecondNumber;

	cout << "sizeof(Timestamp): " << sizeof(Timestamp) << endl;
	assert(sizeof(Timestamp) == sizeof(Date) + sizeof(Time));
	assert(sizeof(Timestamp) == 2 * sizeof(int));
	tsCurrent.SetCurrentTimestamp();
	cout << "SYSTEM\tCurrent Timestamp\t" << tsCurrent << "\t" << tsCurrent.GetDate().GetWeekDay() << endl;

	// Parcours de toutes les Timestamps depuis la Timestamp origine jusqu'a la Timestamp actuelle
	// et verifications de coherence
	tsOrigin.Init(1, 1, 1, 0, 0, 0);
	tsCurrent.Init(2013, 04, 15, 12, 0, 0);
	nOriginAbsoluteDay = tsOrigin.GetDate().GetAbsoluteDay();
	nCurrentAbsoluteDay = tsCurrent.GetDate().GetAbsoluteDay();
	cout << "Start Timestamp\t" << tsOrigin << endl;
	cout << "End Timestamp\t" << tsCurrent << endl;
	nOriginAbsoluteDay = tsOrigin.GetDate().GetAbsoluteDay();
	nCurrentAbsoluteDay = tsCurrent.GetDate().GetAbsoluteDay();
	tsPreviousTimestamp = tsOrigin;
	nTotalYearNumber = 1;
	nTotalMonthNumber = 1;
	nTotalDayNumber = 1;
	nTotalLeapDayNumber = 0;
	for (i = nOriginAbsoluteDay + 1; i <= nCurrentAbsoluteDay; i++)
	{
		tsTimestamp = tsOrigin;
		tsTimestamp.AddSeconds(86400.0 * (i - nOriginAbsoluteDay));
		assert(tsTimestamp.GetDate().GetAbsoluteDay() == i);
		if (tsTimestamp.GetDate().GetYear() != tsPreviousTimestamp.GetDate().GetYear())
			nTotalYearNumber++;
		if (tsTimestamp.GetDate().GetMonth() != tsPreviousTimestamp.GetDate().GetMonth())
			nTotalMonthNumber++;
		if (tsTimestamp.GetDate().GetDay() != tsPreviousTimestamp.GetDate().GetDay())
			nTotalDayNumber++;
		if (tsTimestamp.GetDate().GetMonth() == 2 and tsTimestamp.GetDate().GetDay() == 29)
			nTotalLeapDayNumber++;
		tsPreviousTimestamp = tsTimestamp;
	}
	cout << "\tYears\t2013\t" << nTotalYearNumber << endl;
	cout << "\tMonths\t24148\t" << nTotalMonthNumber << endl;
	cout << "\tDays\t734973\t" << nTotalDayNumber << endl;
	cout << "\tLeap days\t488\t" << nTotalLeapDayNumber << endl;
	cout << endl;

	// Parcours de tous les 1/20000 de secondes pendant 5 secondes et verifications de coherence
	tsOrigin.Init(2013, 04, 15, 12, 0, 0);
	tmTime = tsOrigin.GetTime();
	tmTime.Init(23, 59, 57);
	tsOrigin.SetTime(tmTime);
	tsCurrent = tsOrigin;
	tsCurrent.AddSeconds(5);
	cout << "Start Timestamp\t" << tsOrigin << " ," << endl;
	cout << "Current Timestamp\t" << tsCurrent << " ," << endl;
	nTotalDayNumber = 1;
	nTotalSecondNumber = 1;
	tsPreviousTimestamp = tsOrigin;
	for (i = 0; i < 100000; i++)
	{
		tsTimestamp = tsOrigin;
		tsTimestamp.AddSeconds(i / 20000.0);
		if (tsTimestamp.GetDate().GetDay() != tsPreviousTimestamp.GetDate().GetDay())
			nTotalDayNumber++;
		if ((int)floor(tsTimestamp.GetTime().GetSecond()) !=
		    (int)floor(tsPreviousTimestamp.GetTime().GetSecond()))
			nTotalSecondNumber++;
		tsPreviousTimestamp = tsTimestamp;
	}
	cout << "End Timestamp\t" << tsTimestamp << " ," << endl;
	cout << "\tDays\t2\t" << nTotalDayNumber << endl;
	cout << "\tSeconds\t5\t" << nTotalSecondNumber << endl;
	cout << endl;

	// Test de quelques Timestamps
	cout << "Initial\tTimestamp\tCopy\tDate\tTime\tAbsSecond\t+0s\tComp\t+0.1s\tComp\t+1s\tComp\t+60s\tComp\t+"
		"3600s\tComp\t+86400s\tComp"
	     << endl;
	UnitTest(0, 0, 0, 0, 0, 0);
	UnitTest(0, 1, 1, 0, 0, 0);
	UnitTest(1, 1, 1, 0, 0, 0);
	UnitTest(1900, 1, 1, 0, 0, 0);
	UnitTest(1999, 1, 1, 0, 0, 0);
	UnitTest(2000, 1, 1, 0, 0, 0);
	UnitTest(2001, 1, 1, 0, 0, 0);
	UnitTest(2002, 1, 1, 0, 0, 0);
	UnitTest(2003, 1, 1, 0, 0, 0);
	UnitTest(2004, 1, 1, 0, 0, 0);
	UnitTest(2013, 4, 5, 0, 0, 0);
	UnitTest(1, 12, 31, 0, 0, 0);
	UnitTest(1900, 12, 31, 0, 0, 0);
	UnitTest(1999, 12, 31, 0, 0, 0);
	UnitTest(2000, 12, 31, 0, 0, 0);
	UnitTest(2001, 12, 31, 0, 0, 0);
	UnitTest(2002, 12, 31, 0, 0, 0);
	UnitTest(2003, 12, 31, 0, 0, 0);
	UnitTest(2004, 12, 31, 0, 0, 0);
	UnitTest(2013, 4, 5, 0, 0, 0);
	UnitTest(2013, 4, 5, 23, 59, 0);
	UnitTest(2013, 4, 5, 23, 59, 59);
	UnitTest(2013, 4, 5, 23, 59, 59.9);
	UnitTest(2013, 4, 5, 23, 59, 59.99);
	UnitTest(2013, 4, 5, 23, 59, 59.999);
	UnitTest(2013, 4, 5, 23, 59, 59.9999);
	UnitTest(2013, 4, 5, 23, 59, 59.99999);
	UnitTest(2013, 4, 5, 23, 59, 60);
	UnitTest(4000, 1, 1, 0, 0, 0);
	UnitTest(9999, 1, 1, 0, 0, 0);
	UnitTest(9999, 12, 30, 23, 59, 59.9999);
}

///////////////////////////////////////////////////////////////////////
// Classe KWTimestampFormat

KWTimestampFormat::KWTimestampFormat()
{
	Reset();
}

KWTimestampFormat::~KWTimestampFormat() {}

boolean KWTimestampFormat::SetFormatString(const ALString& sValue)
{
	boolean bCheck = true;
	int nSubFormatLength;

	// Memorisation de la chaine de format
	sFormatString = sValue;

	// Taille de la chaine de format
	nTotalCharNumber = sValue.GetLength();

	// Test de la longueur
	if (bCheck)
	{
		// Longueur minimale
		bCheck = (nTotalCharNumber >= 12);
	}

	// Recherche des motifs dans la chaine
	if (bCheck)
	{
		nTimeOffset = sValue.Find("HH");
		if (nTimeOffset == -1)
			nTimeOffset = sValue.Find("(H)H");

		// Test de validite
		bCheck = (8 <= nTimeOffset and nTimeOffset <= 11);
	}

	// Recherche des separateurs
	if (bCheck)
	{
		// Cas ou il n'y a pas de separateur
		if (nTimeOffset == 8 or nTimeOffset == 10)
		{
			cSeparatorChar = '\0';
			nSeparatorOffset = -1;
		}
		// Cas avec separateur
		else
		{
			assert(nTimeOffset == 9 or nTimeOffset == 11);

			// Le separateur est un caractere avant l'heure
			nSeparatorOffset = nTimeOffset - 1;
			cSeparatorChar = sValue.GetAt(nSeparatorOffset);

			// Test de validite
			bCheck = (cSeparatorChar == ' ' or cSeparatorChar == '-' or cSeparatorChar == '_' or
				  cSeparatorChar == 'T');
		}
	}

	// Analyse du format Date
	if (bCheck)
	{
		nSubFormatLength = nTimeOffset;
		if (cSeparatorChar != '\0')
			nSubFormatLength = nSeparatorOffset;
		else
			nSubFormatLength = nTimeOffset;
		bCheck = dateFormat.SetFormatString(sValue.Left(nSubFormatLength));
	}

	// Analyse du format Time
	if (bCheck)
	{
		nSubFormatLength = nTotalCharNumber - nTimeOffset;
		bCheck = timeFormat.SetFormatString(sValue.Right(nSubFormatLength));
	}

	// Il doit y avoir un separateur si la Date ou Time ont un separateur
	if (bCheck)
	{
		if (dateFormat.GetSeparatorChar() != '\0' or timeFormat.GetSeparatorChar() != '\0')
			bCheck = (cSeparatorChar != '\0');
	}

	// Calcul des nombres min et max de caracteres
	if (bCheck)
	{
		nMinCharNumber = GetDateFormat()->GetMinCharNumber() + GetTimeFormat()->GetMinCharNumber();
		if (nSeparatorOffset != -1)
			nMinCharNumber++;
		nMaxCharNumber = GetDateFormat()->GetMaxCharNumber() + GetTimeFormat()->GetMaxCharNumber();
		if (nSeparatorOffset != -1)
			nMaxCharNumber++;
	}

	// Reinitialisation des caracteristiques du format si invalide
	if (not bCheck)
		Reset();
	return bCheck;
}

const ALString& KWTimestampFormat::GetFormatString() const
{
	return sFormatString;
}

void KWTimestampFormat::Reset()
{
	sFormatString = "";
	dateFormat.Reset();
	timeFormat.Reset();
	nTotalCharNumber = 0;
	cSeparatorChar = '\0';
	nSeparatorOffset = 0;
	nTimeOffset = 0;
	nMinCharNumber = 0;
	nMaxCharNumber = 0;
}

boolean KWTimestampFormat::IsConsistentWith(const KWTimestampFormat* otherFormat) const
{
	return GetSeparatorChar() == otherFormat->GetSeparatorChar() and
	       dateFormat.IsConsistentWith(&(otherFormat->dateFormat)) and
	       timeFormat.IsConsistentWith(&(otherFormat->timeFormat));
}

Timestamp KWTimestampFormat::StringToTimestamp(const char* const sValue) const
{
	boolean bCheck = true;
	char sDateValue[12];
	int nTotalMainCharNumber;
	Timestamp tsConvertedString;
	Date dtValue;
	Time tmValue;
	int nLength;

	require(Check());
	require(sValue != NULL);

	// Initialisations
	dtValue.Reset();
	tmValue.Reset();
	tsConvertedString.Reset();

	// Longueur du format sans la partie decimale optionnelle des secondes
	nTotalMainCharNumber = nTotalCharNumber;
	if (timeFormat.GetDecimalPointOffset() != -1)
		nTotalMainCharNumber--;

	// Acces a la longueur de la valeur a convertir
	nLength = (int)strlen(sValue);

	// Test si longueur compatible
	if (bCheck)
	{
		bCheck = (nLength >= nMinCharNumber);
	}

	// Test du caractere separateur
	if (bCheck)
	{
		if (cSeparatorChar != '\0')
			bCheck = sValue[nSeparatorOffset] == cSeparatorChar;
	}

	// Recherche de la Date et la Time
	if (bCheck)
	{
		// Recherche de la Time d'abord
		tmValue = timeFormat.StringToTime(&sValue[nTimeOffset]);
		bCheck = tmValue.Check();

		// Recherche de la Date, en copiant la sous-chaine correspondante
		if (bCheck)
		{
			strncpy(sDateValue, sValue, dateFormat.GetTotalCharNumber());
			sDateValue[dateFormat.GetTotalCharNumber()] = '\0';
			dtValue = dateFormat.StringToDate(sDateValue);
			bCheck = dtValue.Check();
		}
	}

	// Conversion vers la Timestamp
	if (bCheck)
	{
		tsConvertedString.SetDate(dtValue);
		tsConvertedString.SetTime(tmValue);
	}

	return tsConvertedString;
}

const char* const KWTimestampFormat::TimestampToString(Timestamp tsValue) const
{
	char* sBuffer = StandardGetBuffer();

	require(Check());

	// La valeur manquante est convertie en chaine vide
	if (not tsValue.Check())
		sBuffer[0] = '\0';
	else
	{
		// Ecriture du champ Date
		strcpy(sBuffer, dateFormat.DateToString(tsValue.GetDate()));

		// Ecriture du separateur optionnel
		if (nSeparatorOffset != -1)
			sBuffer[nSeparatorOffset] = cSeparatorChar;

		// Ecriture du champ Time
		strcpy(&sBuffer[nTimeOffset], timeFormat.TimeToString(tsValue.GetTime()));
		assert((int)strlen(sBuffer) >= dateFormat.GetMinCharNumber() + timeFormat.GetMinCharNumber() +
						   ((nSeparatorOffset != -1) ? 1 : 0));
	}
	return sBuffer;
}

void KWTimestampFormat::Write(ostream& ost) const
{
	ost << GetClassLabel() << " " << GetObjectLabel();
}

const ALString KWTimestampFormat::GetClassLabel() const
{
	return "Timestamp format";
}

const ALString KWTimestampFormat::GetObjectLabel() const
{
	return sFormatString;
}

ALString KWTimestampFormat::GetDefaultFormatString()
{
	return "YYYY-MM-DD HH:MM:SS.";
}

void KWTimestampFormat::GetAllAvailableFormats(ObjectArray* oaAvailableFormats)
{
	ObjectArray oaAllAvailableDateFormats;
	ObjectArray oaAllAvailableTimeFormats;
	KWDateFormat* dateFormat;
	KWTimeFormat* timeFormat;
	int nDate;
	int nTime;

	require(oaAvailableFormats != NULL);
	require(oaAvailableFormats->GetSize() == 0);

	// Recherche de tous les formats de Date et Time
	KWDateFormat::GetAllAvailableFormats(&oaAllAvailableDateFormats);
	KWTimeFormat::GetAllAvailableFormats(&oaAllAvailableTimeFormats);

	// Creation de toutes les combinaisons de formats
	for (nDate = 0; nDate < oaAllAvailableDateFormats.GetSize(); nDate++)
	{
		dateFormat = cast(KWDateFormat*, oaAllAvailableDateFormats.GetAt(nDate));
		for (nTime = 0; nTime < oaAllAvailableTimeFormats.GetSize(); nTime++)
		{
			timeFormat = cast(KWTimeFormat*, oaAllAvailableTimeFormats.GetAt(nTime));

			// Creation de la combinaison de format pour chaque type de separateur
			if (dateFormat->GetSeparatorChar() == '\0' and timeFormat->GetSeparatorChar() == '\0')
				AddFormat(oaAvailableFormats,
					  dateFormat->GetFormatString() + timeFormat->GetFormatString());
			AddFormat(oaAvailableFormats,
				  dateFormat->GetFormatString() + ' ' + timeFormat->GetFormatString());
			AddFormat(oaAvailableFormats,
				  dateFormat->GetFormatString() + '-' + timeFormat->GetFormatString());
			AddFormat(oaAvailableFormats,
				  dateFormat->GetFormatString() + '_' + timeFormat->GetFormatString());
			AddFormat(oaAvailableFormats,
				  dateFormat->GetFormatString() + 'T' + timeFormat->GetFormatString());
		}
	}

	// Nettoyage
	oaAllAvailableDateFormats.DeleteAll();
	oaAllAvailableTimeFormats.DeleteAll();
}

void KWTimestampFormat::AddFormat(ObjectArray* oaFormats, const ALString& sFormatString)
{
	KWTimestampFormat* format;

	require(oaFormats != NULL);

	format = new KWTimestampFormat;
	format->SetFormatString(sFormatString);
	assert(format->Check());
	oaFormats->Add(format);
}

void KWTimestampFormat::UnitTest(const ALString& sInputValue, KWTimestampFormat* inputFormat,
				 KWTimestampFormat* outputFormat)
{
	boolean bOk = true;
	boolean bShow = false;
	Timestamp tsInputValue;
	Timestamp tsOutputValue;
	ALString sOutputValue;

	require(inputFormat != NULL);
	require(outputFormat != NULL);

	tsInputValue.Reset();
	tsOutputValue.Reset();
	if (inputFormat->Check())
		tsInputValue = inputFormat->StringToTimestamp(sInputValue);
	if (outputFormat->Check())
		sOutputValue = outputFormat->TimestampToString(tsInputValue);
	if (outputFormat->Check())
		tsOutputValue = outputFormat->StringToTimestamp(sOutputValue);
	bOk = (not inputFormat->Check() or not outputFormat->Check() or
	       (not tsOutputValue.Check() and not tsInputValue.Check()) or
	       (tsOutputValue.GetDate() == tsInputValue.GetDate() and
		tsOutputValue.GetTime().GetHour() == tsInputValue.GetTime().GetHour() and
		tsOutputValue.GetTime().GetMinute() == tsInputValue.GetTime().GetMinute()));
	if (bShow or not bOk)
	{
		// Utilisation des ",\t" pour forcer Excel a ne pas transcoder les valeurs
		cout << sInputValue << " ,\t";
		cout << inputFormat->GetFormatString() << "\t";
		cout << inputFormat->TimestampToString(tsInputValue) << " ,\t";
		cout << outputFormat->GetFormatString() << "\t";
		cout << outputFormat->TimestampToString(tsInputValue) << " ,\t";
		cout << tsOutputValue << " ,";
		if (not bOk)
			cout << "\tError";
		cout << endl;
	}
}

void KWTimestampFormat::Test()
{
	ObjectArray oaAvailableTimestampFormats;
	StringVector svStringTimestamps;
	KWTimestampFormat inputFormat;
	KWTimestampFormat* outputFormat;
	int nFormat;
	int i;

	// Recherche de tous les formats disponibles
	GetAllAvailableFormats(&oaAvailableTimestampFormats);

	// Creation des timestamps a tester
	svStringTimestamps.Add("1012031");
	svStringTimestamps.Add("00000000235959.9");
	svStringTimestamps.Add("00000101235959.9");
	svStringTimestamps.Add("00010101010101");
	svStringTimestamps.Add("00010101010101.1");
	svStringTimestamps.Add("00010101235959.9");
	svStringTimestamps.Add("19000101235959.9");
	svStringTimestamps.Add("19990101235959.9");
	svStringTimestamps.Add("20000101235959.9");
	svStringTimestamps.Add("20010101235959.9");
	svStringTimestamps.Add("20020101235959.9");
	svStringTimestamps.Add("20030101235959.9");
	svStringTimestamps.Add("20040101235959.9");
	svStringTimestamps.Add("20130405235959.9");
	svStringTimestamps.Add("19001231235959.9");
	svStringTimestamps.Add("19991231235959.9");
	svStringTimestamps.Add("20001231235959.9");
	svStringTimestamps.Add("20011231235959.9");
	svStringTimestamps.Add("20021231235959.9");
	svStringTimestamps.Add("20031231235959.9");
	svStringTimestamps.Add("20041231235959.9");
	svStringTimestamps.Add("20130405235959.9");
	svStringTimestamps.Add("40000101235959.9");
	svStringTimestamps.Add("99990101235959.9");

	// Affichages des timestamps testees
	cout << "Timestamps used for conversion tests" << endl;
	for (i = 0; i < svStringTimestamps.GetSize(); i++)
		cout << "\t" << svStringTimestamps.GetAt(i) << endl;

	// Test des formats sur un ensemble de Timestamps
	inputFormat.SetFormatString("YYYYMMDDHHMMSS.");
	cout << "\nFormat number\t" << oaAvailableTimestampFormats.GetSize() << endl;
	for (nFormat = 0; nFormat < oaAvailableTimestampFormats.GetSize(); nFormat++)
	{
		outputFormat = cast(KWTimestampFormat*, oaAvailableTimestampFormats.GetAt(nFormat));
		cout << "\t" << *outputFormat << endl;

		// Tests de conversion
		for (i = 0; i < svStringTimestamps.GetSize(); i++)
		{
			// Affichage des details de test uniquement en cas d'erreur
			UnitTest(svStringTimestamps.GetAt(i), &inputFormat, outputFormat);
		}
	}

	// Nettoyage
	oaAvailableTimestampFormats.DeleteAll();
}
