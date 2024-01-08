// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDateTime.h"

void DateTime::Test()
{
	DateTime dtDateTimeValue;
	DateTime dtForbiddenValue;
	DateTime dtDateValue;
	DateTime dtTimezoneValue;
	DateTime dtTimeValue;
	int i;

	// Taille de la structures
	cout << "DateTime: " << sizeof(DateTime) << "\n";
	assert(sizeof(DateTime) == sizeof(longint));

	// Nombre de digits des fractions de scondes
	cout << "Frac seconds digits: " << nFracSecondsDigitNumber << "\n";
	cout << "Frac seconds max: " << nMaxFracSeconds << "\n";
	assert(10 * (nMaxFracSeconds / 10) == nMaxFracSeconds);
	assert(int(pow(10, nFracSecondsDigitNumber)) == nMaxFracSeconds),

	    // Contenu a 0
	    dtDateTimeValue.lBytes = 0;
	cout << "\nDateTime invalid value\n", dtDateTimeValue.WriteInternalFields(cout);

	// Contenu interdit
	dtForbiddenValue.lBytes = lForbiddenValue;
	cout << "\nDateTime forbidden value\n";
	dtForbiddenValue.WriteInternalFields(cout);

	// Test de mis a jour partielle de la Date
	dtDateValue.lBytes = dtForbiddenValue.lBytes;
	dtDateValue.parts.nTime = 0;
	dtDateValue.SetTimeZone(0);
	cout << "\nDate only forbidden value\n";
	dtDateValue.WriteInternalFields(cout);

	// Test de mis a jour partielle de la Timezone
	dtTimezoneValue.lBytes = dtForbiddenValue.lBytes;
	dtTimezoneValue.parts.nDate = 0;
	dtTimezoneValue.parts.nTime = 0;
	cout << "\nTimezone only forbidden value\n";
	dtTimezoneValue.WriteInternalFields(cout);

	// Test de mis a jour partielle de la Time
	dtTimeValue.lBytes = dtForbiddenValue.lBytes;
	dtTimeValue.parts.nDate = 0;
	dtTimeValue.SetTimeZone(0);
	cout << "\nTime only forbidden value\n";
	dtTimeValue.WriteInternalFields(cout);

	// Verifications avancees
	assert((dtDateValue.lBytes | dtTimezoneValue.lBytes | dtTimeValue.lBytes) == dtForbiddenValue.lBytes);
	assert((dtDateValue.lBytes & dtTimeValue.lBytes) == 0);
	assert((dtDateValue.lBytes & dtTimezoneValue.lBytes) == 0);
	assert((dtTimezoneValue.lBytes & dtTimeValue.lBytes) == 0);

	// Reconstruction
	dtDateTimeValue.parts.nDate = dtDateValue.parts.nDate;
	dtDateTimeValue.SetTimeZone(dtTimezoneValue.GetTimeZone());
	dtDateTimeValue.parts.nTime = dtTimeValue.parts.nTime;
	cout << "\nReconstructed forbidden value\n";
	dtDateTimeValue.WriteInternalFields(cout);
	assert(dtDateTimeValue.lBytes == dtForbiddenValue.lBytes);

	// Test de la partie annees
	cout << "\nTimezone years\n";
	dtDateTimeValue.lBytes = 0;
	for (i = 0; i < nMaxYear; i++)
	{
		dtDateTimeValue.fields.nYear = i;
		if (i % 1000 == 0)
			cout << "." << dtDateTimeValue.fields.nYear;
		assert(dtDateTimeValue.fields.nYear == (unsigned int)i);
	}
	cout << "\n";

	// Test de la partie a la frontiere de minutes de timestamp
	cout << "\nTimezone minutes\n";
	dtDateTimeValue.lBytes = 0;
	for (i = 0; i < 60; i++)
	{
		dtDateTimeValue.SetTimeZoneMinute(i);
		cout << "." << dtDateTimeValue.GetTimeZoneMinute();
		assert(dtDateTimeValue.GetTimeZoneMinute() == i);
		assert(dtDateTimeValue.fields.nTimeZoneSign == 0);
		assert(dtDateTimeValue.fields.nTimeZoneHour == 0);
		assert(dtDateTimeValue.parts.nDate == 0);
		assert(dtDateTimeValue.parts.nTime == 0);
	}
	cout << "\n";

	// Test de la partie des fractions de secondes
	cout << "\nTimezone frac seconds\n";
	dtDateTimeValue.lBytes = 0;
	for (i = 0; i < nMaxFracSeconds; i++)
	{
		dtDateTimeValue.fields.nFrac = i;
		if (i % 100 == 0)
			cout << "." << dtDateTimeValue.fields.nFrac;
		assert(dtDateTimeValue.fields.nFrac == (unsigned int)i);
	}
	cout << "\n";
}

void DateTime::WriteInternalFields(ostream& ost) const
{
	cout << "dateTimeValue fields\n";
	cout << "\tlDateTime bytes: " << lBytes << "\n";
	cout << "\tDateTime parts: ((" << parts.nDate << "," << GetTimeZone() << "), " << parts.nTime << ")\n";
	cout << "\tDateTime fields: ((" << fields.nYear << "," << fields.nMonth << "," << fields.nDay << " ["
	     << fields.nTimeZoneSign << ", " << fields.nTimeZoneHour << ", " << GetTimeZoneMinute() << "]),"
	     << fields.nHour << "," << fields.nMinute << "," << fields.nSecond << "," << fields.nFrac << ")\n";
}
