// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTimestampTZ.h"

///////////////////////////////////////////////////////////////////////
// Classe TimestampTZ

boolean TimestampTZ::Init(Timestamp tsValue, int nTimeZoneTotalMinutes)
{
	boolean bOk;

	// Initialisation a invalide
	Reset();

	// Initialisation du timestamp
	GetInternalTimestamp() = tsValue;

	// Test si OK
	bOk = tsValue.Check();

	// Prise en compte de la time zone
	if (bOk)
		bOk = SetTimeZoneTotalMinutes(nTimeZoneTotalMinutes);

	// Re-initialisation si ko
	if (not bOk)
		Reset();
	return bOk;
}

boolean TimestampTZ::AddSeconds(double dSeconds)
{
	boolean bOk;
	Timestamp tsLocalTimestamp;

	require(Check());

	// On passe par le local timestamp pour reutiliser la methode correspondante de Timestamp
	tsLocalTimestamp = GetLocalTimestamp();
	bOk = tsLocalTimestamp.AddSeconds(dSeconds);
	SetLocalTimestamp(tsLocalTimestamp);
	return bOk;
}

const char* const TimestampTZ::ToString() const
{
	char* sTimestampTZ = StandardGetBuffer();

	if (not Check())
		sTimestampTZ[0] = '\0';
	else
		sprintf(sTimestampTZ, "%s %s%s", GetInternalDate().ToString(), GetInternalTime().ToString(),
			GetInternalDate().TimeZoneToString(true));
	return sTimestampTZ;
}

void TimestampTZ::Write(ostream& ost) const
{
	ost << ToString();
}

void TimestampTZ::Test()
{
	Timestamp tsOrigin;
	TimestampTZ tstzOrigin;
	TimestampTZ tstzCurrent;
	Timestamp tsUtcTimestamp;
	Timestamp tsLocalTimestamp;
	int i;

	cout << "sizeof(TimestampTZ): " << sizeof(TimestampTZ) << endl;
	assert(sizeof(TimestampTZ) == sizeof(Date) + sizeof(Time));
	assert(sizeof(TimestampTZ) == 2 * sizeof(int));

	// Test de quelques time zones
	tsOrigin.Init(2000, 01, 01, 13, 0, 0);
	tstzOrigin.Init(tsOrigin, 0);
	cout << "Initial\tTS hours\tWith TS\tLocal\tUTC\tDiff UTC\tDiff UTC (TZ)" << endl;
	for (i = -12; i <= 14; i++)
	{
		tstzCurrent = tstzOrigin;
		tstzCurrent.SetTimeZoneTotalMinutes(i * 60);
		cout << tstzOrigin << "\t";
		cout << i << "\t ";
		cout << tstzCurrent << "\t";
		tsLocalTimestamp = tstzCurrent.GetLocalTimestamp();
		cout << tsLocalTimestamp << "\t";
		tsUtcTimestamp = tstzCurrent.GetUtcTimestamp();
		cout << tsUtcTimestamp << "\t";
		cout << tsLocalTimestamp.Diff(tsUtcTimestamp) / 3600 << "\t";
		cout << tstzCurrent.Diff(tstzOrigin) / 3600 << endl;
	}
}

///////////////////////////////////////////////////////////////////////
// Classe KWTimestampTZFormat

KWTimestampTZFormat::KWTimestampTZFormat()
{
	Reset();
}

KWTimestampTZFormat::~KWTimestampTZFormat() {}

boolean KWTimestampTZFormat::SetFormatString(const ALString& sValue)
{
	boolean bCheck = true;
	ALString sTimestampFormat;
	int i;

	// Memorisation de la chaine de format
	sFormatString = sValue;

	// Recherche du format de time zone
	nTimeZoneFormatLength = 0;
	if (bCheck)
	{
		for (i = sValue.GetLength() - 1; i >= 0; i--)
		{
			if (sValue.GetAt(i) == 'z')
				nTimeZoneFormatLength++;
			else
				break;
		}
		nTimeZoneFormat = NoTimeZone;
		for (i = NoTimeZone + 1; i <= ExtendedTimeZone; i++)
		{
			if (nTimeZoneFormatLength == GetTimeZoneMaxLength(i))
			{
				nTimeZoneFormat = i;
				break;
			}
		}
		bCheck = nTimeZoneFormat != NoTimeZone;
	}

	// Si ok, parametrage du format Timestamp
	if (bCheck)
		bCheck = timestampFormat.SetFormatString(sValue.Left(sValue.GetLength() - nTimeZoneFormatLength));

	// Reinitialisation des caracteristiques du format si invalide
	if (not bCheck)
		Reset();
	return bCheck;
}

const ALString& KWTimestampTZFormat::GetFormatString() const
{
	return sFormatString;
}

void KWTimestampTZFormat::Reset()
{
	sFormatString = "";
	timestampFormat.Reset();
	nTimeZoneFormat = NoTimeZone;
	nTimeZoneFormatLength = 0;
}

boolean KWTimestampTZFormat::IsConsistentWith(const KWTimestampTZFormat* otherFormat) const
{
	return nTimeZoneFormat == otherFormat->nTimeZoneFormat and
	       timestampFormat.IsConsistentWith(&(otherFormat->timestampFormat));
}

TimestampTZ KWTimestampTZFormat::StringToTimestampTZ(const char* const sValue) const
{
	boolean bCheck = true;
	char sTimestampValue[100];
	TimestampTZ tstzConvertedString;
	Timestamp tsValue;
	int nLength;
	int nTimeZoneLength;
	int nTimeZoneHour;
	int nTimeZoneMinute;
	int nTimeZoneTotalMinutes;
	char cTimeZoneSign;
	char cTimeZoneH1;
	char cTimeZoneH2;
	char cTimeZoneM1;
	char cTimeZoneM2;

	require(Check());
	require(sValue != NULL);

	// Initialisations
	tsValue.Reset();
	tstzConvertedString.Reset();

	// Acces a la longueur de la valeur a convertir
	nLength = (int)strlen(sValue);

	// Recherche de la partie time zone
	assert(nTimeZoneFormat != NoTimeZone);
	nTimeZoneLength = 0;
	nTimeZoneHour = 0;
	nTimeZoneMinute = 0;
	cTimeZoneSign = '+';
	// Ok si on termine par 'Z'
	if (sValue[nLength - 1] == 'Z')
		nTimeZoneLength = 1;
	// Analyse detaillee sinon
	else
	{
		// On doit avoir au moins la taille moins, plus la place de la time zone (-1 pour le cas reduit a 'Z')
		nTimeZoneLength = nTimeZoneFormatLength;
		bCheck = nLength >= timestampFormat.GetMinCharNumber() + (nTimeZoneLength - 1);
		if (bCheck)
		{
			// Verification du signe
			cTimeZoneSign = sValue[nLength - nTimeZoneLength];
			if (cTimeZoneSign != '+' and cTimeZoneSign != '-')
				bCheck = false;
			// Verification du separateur
			else if (nTimeZoneFormat == ExtendedTimeZone and sValue[nLength - 3] != ':')
				bCheck = false;
			// Verification des heures et minutes
			{
				cTimeZoneH1 = sValue[nLength - nTimeZoneLength + 1];
				cTimeZoneH2 = sValue[nLength - nTimeZoneLength + 2];
				cTimeZoneM1 = sValue[nLength - 2];
				cTimeZoneM2 = sValue[nLength - 1];

				// Test de validite
				bCheck = isdigit(cTimeZoneH1) and isdigit(cTimeZoneH2) and isdigit(cTimeZoneM1) and
					 isdigit(cTimeZoneM2);

				// Calcul de la partie heure et minute de la time zone, que l'on ne mettra a jour
				// qu'apres la date
				if (bCheck)
				{
					nTimeZoneHour = 10 * (cTimeZoneH1 - '0') + (cTimeZoneH2 - '0');
					nTimeZoneMinute = 10 * (cTimeZoneM1 - '0') + (cTimeZoneM2 - '0');
				}
			}
		}
	}

	// Recherche de la partie Timestamp, en copiant la sous-chaine correspondante
	if (bCheck)
	{
		strncpy(sTimestampValue, sValue, nLength - nTimeZoneLength);
		sTimestampValue[nLength - nTimeZoneLength] = '\0';
		tsValue = timestampFormat.StringToTimestamp(sTimestampValue);
		bCheck = tsValue.Check();
	}

	// Mise a jour de la partie time zone
	if (bCheck)
	{
		assert(nTimeZoneFormat != NoTimeZone);
		nTimeZoneTotalMinutes = (cTimeZoneSign == '-' ? -1 : 1) * (60 * nTimeZoneHour + nTimeZoneMinute);
		bCheck = tstzConvertedString.Init(tsValue, nTimeZoneTotalMinutes);
	}
	return tstzConvertedString;
}

const char* const KWTimestampTZFormat::TimestampTZToString(TimestampTZ tstzValue) const
{
	char* sBuffer = StandardGetBuffer();

	require(Check());

	// La valeur manquante est convertie en chaine vide
	if (not tstzValue.Check())
		sBuffer[0] = '\0';
	// Traitement complet sinon
	else
	{
		assert(nTimeZoneFormat != NoTimeZone);

		// Ecriture du champ Date
		strcpy(sBuffer, timestampFormat.TimestampToString(tstzValue.GetInternalTimestamp()));

		// Ecriture du champ time zone
		strcpy(&sBuffer[strlen(sBuffer)],
		       tstzValue.GetInternalDate().TimeZoneToString(nTimeZoneFormat == ExtendedTimeZone));
		assert((int)strlen(sBuffer) >= GetMinCharNumber());
	}
	return sBuffer;
}

void KWTimestampTZFormat::Write(ostream& ost) const
{
	ost << GetClassLabel() << " " << GetObjectLabel();
}

const ALString KWTimestampTZFormat::GetClassLabel() const
{
	return "TimestampTZ format";
}

const ALString KWTimestampTZFormat::GetObjectLabel() const
{
	return sFormatString;
}

ALString KWTimestampTZFormat::GetDefaultFormatString()
{
	return "YYYY-MM-DD HH:MM:SS.zzzzzz";
}

void KWTimestampTZFormat::GetAllAvailableFormats(ObjectArray* oaAvailableFormats)
{
	ObjectArray oaAllAvailableTimestampFormats;
	KWTimestampFormat* timestampFormat;
	int nTimestamp;

	require(oaAvailableFormats != NULL);
	require(oaAvailableFormats->GetSize() == 0);

	// Recherche de tous les formats de Timestamp
	KWTimestampFormat::GetAllAvailableFormats(&oaAllAvailableTimestampFormats);

	// Prise en compte des formats avec time zone
	for (nTimestamp = 0; nTimestamp < oaAllAvailableTimestampFormats.GetSize(); nTimestamp++)
	{
		timestampFormat = cast(KWTimestampFormat*, oaAllAvailableTimestampFormats.GetAt(nTimestamp));

		// Ajout des deux formats avec time zone
		AddFormat(oaAvailableFormats, timestampFormat->GetFormatString() + "zzzzz");
		AddFormat(oaAvailableFormats, timestampFormat->GetFormatString() + "zzzzzz");
	}

	// Nettoyage
	oaAllAvailableTimestampFormats.DeleteAll();
}

void KWTimestampTZFormat::AddFormat(ObjectArray* oaFormats, const ALString& sFormatString)
{
	KWTimestampTZFormat* format;

	require(oaFormats != NULL);

	format = new KWTimestampTZFormat;
	format->SetFormatString(sFormatString);
	assert(format->Check());
	oaFormats->Add(format);
}

void KWTimestampTZFormat::UnitTest(const ALString& sInputValue, KWTimestampTZFormat* inputFormat,
				   KWTimestampTZFormat* outputFormat)
{
	boolean bOk = true;
	boolean bShow = false;
	TimestampTZ tstzInputValue;
	TimestampTZ tstzOutputValue;
	ALString sOutputValue;

	require(inputFormat != NULL);
	require(outputFormat != NULL);

	tstzInputValue.Reset();
	tstzOutputValue.Reset();
	if (inputFormat->Check())
		tstzInputValue = inputFormat->StringToTimestampTZ(sInputValue);
	if (outputFormat->Check())
		sOutputValue = outputFormat->TimestampTZToString(tstzInputValue);
	if (outputFormat->Check())
		tstzOutputValue = outputFormat->StringToTimestampTZ(sOutputValue);
	bOk = (not inputFormat->Check() or not outputFormat->Check() or
	       (not tstzOutputValue.Check() and not tstzInputValue.Check()) or tstzOutputValue == tstzInputValue);
	if (bShow or not bOk)
	{
		// Utilisation des ",\t" pour forcer Excel a ne pas transcoder les valeurs
		cout << sInputValue << " ,\t";
		cout << inputFormat->GetFormatString() << "\t";
		cout << inputFormat->TimestampTZToString(tstzInputValue) << " ,\t";
		cout << outputFormat->GetFormatString() << "\t";
		cout << outputFormat->TimestampTZToString(tstzInputValue) << " ,\t";
		cout << tstzOutputValue << " ,";
		if (not bOk)
			cout << "\tError";
		cout << endl;
	}
}

void KWTimestampTZFormat::Test()
{
	ObjectArray oaAvailableTimestampTZFormats;
	StringVector svStringTimestampTZs;
	KWTimestampTZFormat inputFormat;
	KWTimestampTZFormat* outputFormat;
	int nFormat;
	int i;

	// Recherche de tous les formats disponibles
	GetAllAvailableFormats(&oaAvailableTimestampTZFormats);

	// Creation des TimestampTZ avec timezone tester
	svStringTimestampTZs.Add("20000101130000Z");
	svStringTimestampTZs.Add("20000101130000-12:00");
	svStringTimestampTZs.Add("20000101130000+01:00");
	svStringTimestampTZs.Add("20000101130000+14:00");

	// Test des formats sur un ensemble de TimestampTZs
	inputFormat.SetFormatString("YYYYMMDDHHMMSS.zzzzzz");
	cout << "\nFormat number\t" << oaAvailableTimestampTZFormats.GetSize() << endl;
	for (nFormat = 0; nFormat < oaAvailableTimestampTZFormats.GetSize(); nFormat++)
	{
		outputFormat = cast(KWTimestampTZFormat*, oaAvailableTimestampTZFormats.GetAt(nFormat));
		cout << "\t" << *outputFormat << endl;

		// Tests de conversion
		for (i = 0; i < svStringTimestampTZs.GetSize(); i++)
		{
			// Affichage des details de test uniquement en cas d'erreur
			UnitTest(svStringTimestampTZs.GetAt(i), &inputFormat, outputFormat);
		}
	}

	// Nettoyage
	oaAvailableTimestampTZFormats.DeleteAll();
}