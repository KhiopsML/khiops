// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRDateTime.h"

void KWDRRegisterDateTimeRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRYear);
	KWDerivationRule::RegisterDerivationRule(new KWDRMonth);
	KWDerivationRule::RegisterDerivationRule(new KWDRDay);
	KWDerivationRule::RegisterDerivationRule(new KWDRYearDay);
	KWDerivationRule::RegisterDerivationRule(new KWDRWeekDay);
	KWDerivationRule::RegisterDerivationRule(new KWDRDecimalYear);
	KWDerivationRule::RegisterDerivationRule(new KWDRAbsoluteDay);
	KWDerivationRule::RegisterDerivationRule(new KWDRDiffDate);
	KWDerivationRule::RegisterDerivationRule(new KWDRAddDays);
	KWDerivationRule::RegisterDerivationRule(new KWDRIsDateValid);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildDate);
	KWDerivationRule::RegisterDerivationRule(new KWDRHour);
	KWDerivationRule::RegisterDerivationRule(new KWDRMinute);
	KWDerivationRule::RegisterDerivationRule(new KWDRSecond);
	KWDerivationRule::RegisterDerivationRule(new KWDRDaySecond);
	KWDerivationRule::RegisterDerivationRule(new KWDRDecimalTime);
	KWDerivationRule::RegisterDerivationRule(new KWDRDiffTime);
	KWDerivationRule::RegisterDerivationRule(new KWDRIsTimeValid);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildTime);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetDate);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetTime);
	KWDerivationRule::RegisterDerivationRule(new KWDRDecimalYearTS);
	KWDerivationRule::RegisterDerivationRule(new KWDRAbsoluteSecond);
	KWDerivationRule::RegisterDerivationRule(new KWDRDecimalWeekDay);
	KWDerivationRule::RegisterDerivationRule(new KWDRDiffTimestamp);
	KWDerivationRule::RegisterDerivationRule(new KWDRAddSeconds);
	KWDerivationRule::RegisterDerivationRule(new KWDRIsTimestampValid);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildTimestamp);
	KWDerivationRule::RegisterDerivationRule(new KWDRUtcTimestamp);
	KWDerivationRule::RegisterDerivationRule(new KWDRLocalTimestamp);
	KWDerivationRule::RegisterDerivationRule(new KWDRSetTimeZoneMinutes);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetTimeZoneMinutes);
	KWDerivationRule::RegisterDerivationRule(new KWDRDiffTimestampTZ);
	KWDerivationRule::RegisterDerivationRule(new KWDRAddSecondsTSTZ);
	KWDerivationRule::RegisterDerivationRule(new KWDRIsTimestampTZValid);
	KWDerivationRule::RegisterDerivationRule(new KWDRBuildTimestampTZ);
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRYear::KWDRYear()
{
	SetName("Year");
	SetLabel("Year in a date");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Date);
}

KWDRYear::~KWDRYear() {}

KWDerivationRule* KWDRYear::Create() const
{
	return new KWDRYear;
}

Continuous KWDRYear::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Date dtDate;

	require(IsCompiled());

	dtDate = GetFirstOperand()->GetDateValue(kwoObject);
	if (dtDate.Check())
		return (Continuous)dtDate.GetYear();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRMonth::KWDRMonth()
{
	SetName("Month");
	SetLabel("Month in a date");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Date);
}

KWDRMonth::~KWDRMonth() {}

KWDerivationRule* KWDRMonth::Create() const
{
	return new KWDRMonth;
}

Continuous KWDRMonth::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Date dtDate;

	require(IsCompiled());

	dtDate = GetFirstOperand()->GetDateValue(kwoObject);
	if (dtDate.Check())
		return (Continuous)dtDate.GetMonth();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDay::KWDRDay()
{
	SetName("Day");
	SetLabel("Day in a date");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Date);
}

KWDRDay::~KWDRDay() {}

KWDerivationRule* KWDRDay::Create() const
{
	return new KWDRDay;
}

Continuous KWDRDay::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Date dtDate;

	require(IsCompiled());

	dtDate = GetFirstOperand()->GetDateValue(kwoObject);
	if (dtDate.Check())
		return (Continuous)dtDate.GetDay();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRYearDay::KWDRYearDay()
{
	SetName("YearDay");
	SetLabel("Day in year in a date");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Date);
}

KWDRYearDay::~KWDRYearDay() {}

KWDerivationRule* KWDRYearDay::Create() const
{
	return new KWDRYearDay;
}

Continuous KWDRYearDay::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Date dtDate;

	require(IsCompiled());

	dtDate = GetFirstOperand()->GetDateValue(kwoObject);
	if (dtDate.Check())
		return (Continuous)dtDate.GetYearDay();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRWeekDay::KWDRWeekDay()
{
	SetName("WeekDay");
	SetLabel("Day in week in a date");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Date);
}

KWDRWeekDay::~KWDRWeekDay() {}

KWDerivationRule* KWDRWeekDay::Create() const
{
	return new KWDRWeekDay;
}

Continuous KWDRWeekDay::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Date dtDate;

	require(IsCompiled());

	dtDate = GetFirstOperand()->GetDateValue(kwoObject);
	if (dtDate.Check())
		return (Continuous)dtDate.GetWeekDay();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDecimalYear::KWDRDecimalYear()
{
	SetName("DecimalYear");
	SetLabel("Year with decimal part for day in year");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Date);
}

KWDRDecimalYear::~KWDRDecimalYear() {}

KWDerivationRule* KWDRDecimalYear::Create() const
{
	return new KWDRDecimalYear;
}

Continuous KWDRDecimalYear::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Date dtDate;

	require(IsCompiled());

	dtDate = GetFirstOperand()->GetDateValue(kwoObject);
	if (dtDate.Check())
		return (Continuous)(dtDate.GetDecimalYear());
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

Date KWDRAbsoluteDay::dtRefDate;

KWDRAbsoluteDay::KWDRAbsoluteDay()
{
	SetName("AbsoluteDay");
	SetLabel("Total elapsed days since 2000-01-01");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Date);

	// Initialisation de la date de reference
	dtRefDate.Init(2000, 1, 1);
	assert(dtRefDate.Check());
}

KWDRAbsoluteDay::~KWDRAbsoluteDay() {}

KWDerivationRule* KWDRAbsoluteDay::Create() const
{
	return new KWDRAbsoluteDay;
}

Continuous KWDRAbsoluteDay::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Date dtDate;

	require(IsCompiled());

	dtDate = GetFirstOperand()->GetDateValue(kwoObject);
	if (dtDate.Check())
		return (Continuous)dtDate.Diff(dtRefDate);
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDiffDate::KWDRDiffDate()
{
	SetName("DiffDate");
	SetLabel("Difference in days between two dates");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Date);
	GetSecondOperand()->SetType(KWType::Date);
}

KWDRDiffDate::~KWDRDiffDate() {}

KWDerivationRule* KWDRDiffDate::Create() const
{
	return new KWDRDiffDate;
}

Continuous KWDRDiffDate::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Date dtDate1;
	Date dtDate2;

	require(IsCompiled());

	dtDate1 = GetFirstOperand()->GetDateValue(kwoObject);
	dtDate2 = GetSecondOperand()->GetDateValue(kwoObject);
	if (dtDate1.Check() and dtDate2.Check())
		return (Continuous)dtDate1.Diff(dtDate2);
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAddDays::KWDRAddDays()
{
	SetName("AddDays");
	SetLabel("Add a number of days to a date");
	SetType(KWType::Date);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Date);
	GetOperandAt(1)->SetType(KWType::Continuous);
}

KWDRAddDays::~KWDRAddDays() {}

KWDerivationRule* KWDRAddDays::Create() const
{
	return new KWDRAddDays;
}

Date KWDRAddDays::ComputeDateResult(const KWObject* kwoObject) const
{
	Date dtDate;
	Continuous cDays;

	require(IsCompiled());

	// Acces aux operandes
	dtDate = GetOperandAt(0)->GetDateValue(kwoObject);
	cDays = GetOperandAt(1)->GetContinuousValue(kwoObject);

	// Ajout des jours a la date, si validite des operandes
	if (dtDate.Check() and cDays != KWContinuous::GetMissingValue())
		dtDate.AddDays((int)floor(cDays + 0.5));
	// Sinon, on rend une date invalide
	else
		dtDate.Reset();
	return dtDate;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRIsDateValid::KWDRIsDateValid()
{
	SetName("IsDateValid");
	SetLabel("Check if a date is valid");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Date);
}

KWDRIsDateValid::~KWDRIsDateValid() {}

KWDerivationRule* KWDRIsDateValid::Create() const
{
	return new KWDRIsDateValid;
}

Continuous KWDRIsDateValid::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Date dtDate;

	require(IsCompiled());

	dtDate = GetFirstOperand()->GetDateValue(kwoObject);
	return (Continuous)dtDate.Check();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRBuildDate::KWDRBuildDate()
{
	SetName("BuildDate");
	SetLabel("Build a date from year, month and day");
	SetType(KWType::Date);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Continuous);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRBuildDate::~KWDRBuildDate() {}

KWDerivationRule* KWDRBuildDate::Create() const
{
	return new KWDRBuildDate;
}

Date KWDRBuildDate::ComputeDateResult(const KWObject* kwoObject) const
{
	Date dtDate;
	Continuous cYear;
	Continuous cMonth;
	Continuous cDay;

	require(IsCompiled());

	// Recherche des valeurs an, mois, jour
	cYear = GetOperandAt(0)->GetContinuousValue(kwoObject);
	cMonth = GetOperandAt(1)->GetContinuousValue(kwoObject);
	cDay = GetOperandAt(2)->GetContinuousValue(kwoObject);

	// Initialisation de la date
	dtDate.Reset();
	if (cYear != KWContinuous::GetMissingValue() and cMonth != KWContinuous::GetMissingValue() and
	    cDay != KWContinuous::GetMissingValue())
		dtDate.Init((int)floor(cYear + 0.5), (int)floor(cMonth + 0.5), (int)floor(cDay + 0.5));
	return dtDate;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRHour::KWDRHour()
{
	SetName("Hour");
	SetLabel("Hour in a time value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Time);
}

KWDRHour::~KWDRHour() {}

KWDerivationRule* KWDRHour::Create() const
{
	return new KWDRHour;
}

Continuous KWDRHour::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Time tmTime;

	require(IsCompiled());

	tmTime = GetFirstOperand()->GetTimeValue(kwoObject);
	if (tmTime.Check())
		return (Continuous)tmTime.GetHour();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRMinute::KWDRMinute()
{
	SetName("Minute");
	SetLabel("Minute in a time value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Time);
}

KWDRMinute::~KWDRMinute() {}

KWDerivationRule* KWDRMinute::Create() const
{
	return new KWDRMinute;
}

Continuous KWDRMinute::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Time tmTime;

	require(IsCompiled());

	tmTime = GetFirstOperand()->GetTimeValue(kwoObject);
	if (tmTime.Check())
		return (Continuous)tmTime.GetMinute();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSecond::KWDRSecond()
{
	SetName("Second");
	SetLabel("Second in a time value");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Time);
}

KWDRSecond::~KWDRSecond() {}

KWDerivationRule* KWDRSecond::Create() const
{
	return new KWDRSecond;
}

Continuous KWDRSecond::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Time tmTime;

	require(IsCompiled());

	tmTime = GetFirstOperand()->GetTimeValue(kwoObject);
	if (tmTime.Check())
		return (Continuous)tmTime.GetSecond();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDaySecond::KWDRDaySecond()
{
	SetName("DaySecond");
	SetLabel("Total seconds in day");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Time);
}

KWDRDaySecond::~KWDRDaySecond() {}

KWDerivationRule* KWDRDaySecond::Create() const
{
	return new KWDRDaySecond;
}

Continuous KWDRDaySecond::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Time tmTime;

	require(IsCompiled());

	tmTime = GetFirstOperand()->GetTimeValue(kwoObject);
	if (tmTime.Check())
		return (Continuous)tmTime.GetDaySecond();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDecimalTime::KWDRDecimalTime()
{
	SetName("DecimalTime");
	SetLabel("Decimal hour in day");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Time);
}

KWDRDecimalTime::~KWDRDecimalTime() {}

KWDerivationRule* KWDRDecimalTime::Create() const
{
	return new KWDRDecimalTime;
}

Continuous KWDRDecimalTime::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Time tmTime;

	require(IsCompiled());

	tmTime = GetFirstOperand()->GetTimeValue(kwoObject);
	if (tmTime.Check())
		return (Continuous)tmTime.GetDecimalTime();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDiffTime::KWDRDiffTime()
{
	SetName("DiffTime");
	SetLabel("Difference in seconds between two time values");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Time);
	GetSecondOperand()->SetType(KWType::Time);
}

KWDRDiffTime::~KWDRDiffTime() {}

KWDerivationRule* KWDRDiffTime::Create() const
{
	return new KWDRDiffTime;
}

Continuous KWDRDiffTime::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Time tmTime1;
	Time tmTime2;

	require(IsCompiled());

	tmTime1 = GetFirstOperand()->GetTimeValue(kwoObject);
	tmTime2 = GetSecondOperand()->GetTimeValue(kwoObject);
	if (tmTime1.Check() and tmTime2.Check())
		return (Continuous)tmTime1.Diff(tmTime2);
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRIsTimeValid::KWDRIsTimeValid()
{
	SetName("IsTimeValid");
	SetLabel("Check if a time is valid");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Time);
}

KWDRIsTimeValid::~KWDRIsTimeValid() {}

KWDerivationRule* KWDRIsTimeValid::Create() const
{
	return new KWDRIsTimeValid;
}

Continuous KWDRIsTimeValid::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Time tmTime;

	require(IsCompiled());

	tmTime = GetFirstOperand()->GetTimeValue(kwoObject);
	return (Continuous)tmTime.Check();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRBuildTime::KWDRBuildTime()
{
	SetName("BuildTime");
	SetLabel("Build a time from hour, minute and second");
	SetType(KWType::Time);
	SetOperandNumber(3);
	GetOperandAt(0)->SetType(KWType::Continuous);
	GetOperandAt(1)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRBuildTime::~KWDRBuildTime() {}

KWDerivationRule* KWDRBuildTime::Create() const
{
	return new KWDRBuildTime;
}

Time KWDRBuildTime::ComputeTimeResult(const KWObject* kwoObject) const
{
	Time tmTime;
	Continuous cHour;
	Continuous cMinute;
	Continuous cSecond;

	require(IsCompiled());

	// Recherche des valeurs an, mois, jour
	cHour = GetOperandAt(0)->GetContinuousValue(kwoObject);
	cMinute = GetOperandAt(1)->GetContinuousValue(kwoObject);
	cSecond = GetOperandAt(2)->GetContinuousValue(kwoObject);

	// Initialisation de la time
	tmTime.Reset();
	if (cHour != KWContinuous::GetMissingValue() and cMinute != KWContinuous::GetMissingValue() and
	    cSecond != KWContinuous::GetMissingValue())
		tmTime.Init((int)floor(cHour + 0.5), (int)floor(cMinute + 0.5), (int)floor(cSecond + 0.5));
	return tmTime;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRGetDate::KWDRGetDate()
{
	SetName("GetDate");
	SetLabel("Get date from timestamp");
	SetType(KWType::Date);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Timestamp);
}

KWDRGetDate::~KWDRGetDate() {}

KWDerivationRule* KWDRGetDate::Create() const
{
	return new KWDRGetDate;
}

Date KWDRGetDate::ComputeDateResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp;

	require(IsCompiled());

	tsTimestamp = GetFirstOperand()->GetTimestampValue(kwoObject);
	return tsTimestamp.GetDate();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRGetTime::KWDRGetTime()
{
	SetName("GetTime");
	SetLabel("Get time from timestamp");
	SetType(KWType::Time);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Timestamp);
}

KWDRGetTime::~KWDRGetTime() {}

KWDerivationRule* KWDRGetTime::Create() const
{
	return new KWDRGetTime;
}

Time KWDRGetTime::ComputeTimeResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp;

	require(IsCompiled());

	tsTimestamp = GetFirstOperand()->GetTimestampValue(kwoObject);
	return tsTimestamp.GetTime();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDecimalYearTS::KWDRDecimalYearTS()
{
	SetName("DecimalYearTS");
	SetLabel("Year with decimal part for day in year, at timestamp precision");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Timestamp);
}

KWDRDecimalYearTS::~KWDRDecimalYearTS() {}

KWDerivationRule* KWDRDecimalYearTS::Create() const
{
	return new KWDRDecimalYearTS;
}

Continuous KWDRDecimalYearTS::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp;

	require(IsCompiled());

	tsTimestamp = GetFirstOperand()->GetTimestampValue(kwoObject);
	if (tsTimestamp.Check())
		return (Continuous)(tsTimestamp.GetDecimalYear());
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

Timestamp KWDRAbsoluteSecond::tsRefTimestamp;

KWDRAbsoluteSecond::KWDRAbsoluteSecond()
{
	SetName("AbsoluteSecond");
	SetLabel("Total elapsed seconds since 2000-01-01 00:00:00");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Timestamp);

	// Initialisation de la timestamp de reference
	tsRefTimestamp.Init(2000, 1, 1, 0, 0, 0);
	assert(tsRefTimestamp.Check());
}

KWDRAbsoluteSecond::~KWDRAbsoluteSecond() {}

KWDerivationRule* KWDRAbsoluteSecond::Create() const
{
	return new KWDRAbsoluteSecond;
}

Continuous KWDRAbsoluteSecond::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp;

	require(IsCompiled());

	tsTimestamp = GetFirstOperand()->GetTimestampValue(kwoObject);
	if (tsTimestamp.Check())
		return (Continuous)tsTimestamp.Diff(tsRefTimestamp);
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDecimalWeekDay::KWDRDecimalWeekDay()
{
	SetName("DecimalWeekDay");
	SetLabel("Week day with decimal part for fraction of days");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Timestamp);
}

KWDRDecimalWeekDay::~KWDRDecimalWeekDay() {}

KWDerivationRule* KWDRDecimalWeekDay::Create() const
{
	return new KWDRDecimalWeekDay;
}

Continuous KWDRDecimalWeekDay::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp;

	require(IsCompiled());

	tsTimestamp = GetFirstOperand()->GetTimestampValue(kwoObject);
	if (tsTimestamp.Check())
		return (Continuous)tsTimestamp.GetDecimalWeekDay();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDiffTimestamp::KWDRDiffTimestamp()
{
	SetName("DiffTimestamp");
	SetLabel("Difference in seconds between two timestamps");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Timestamp);
	GetSecondOperand()->SetType(KWType::Timestamp);
}

KWDRDiffTimestamp::~KWDRDiffTimestamp() {}

KWDerivationRule* KWDRDiffTimestamp::Create() const
{
	return new KWDRDiffTimestamp;
}

Continuous KWDRDiffTimestamp::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp1;
	Timestamp tsTimestamp2;

	require(IsCompiled());

	tsTimestamp1 = GetFirstOperand()->GetTimestampValue(kwoObject);
	tsTimestamp2 = GetSecondOperand()->GetTimestampValue(kwoObject);
	if (tsTimestamp1.Check() and tsTimestamp2.Check())
		return (Continuous)tsTimestamp1.Diff(tsTimestamp2);
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAddSeconds::KWDRAddSeconds()
{
	SetName("AddSeconds");
	SetLabel("Add a number of seconds to a timestamp");
	SetType(KWType::Timestamp);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Timestamp);
	GetOperandAt(1)->SetType(KWType::Continuous);
}

KWDRAddSeconds::~KWDRAddSeconds() {}

KWDerivationRule* KWDRAddSeconds::Create() const
{
	return new KWDRAddSeconds;
}

Timestamp KWDRAddSeconds::ComputeTimestampResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp;
	Continuous cSeconds;

	require(IsCompiled());

	// Acces aux operandes
	tsTimestamp = GetOperandAt(0)->GetTimestampValue(kwoObject);
	cSeconds = GetOperandAt(1)->GetContinuousValue(kwoObject);

	// Ajout des jours a la timestamp, si validite des operandes
	if (tsTimestamp.Check() and cSeconds != KWContinuous::GetMissingValue())
		tsTimestamp.AddSeconds((int)floor(cSeconds + 0.5));
	// Sinon, on rend un timestamp invalide
	else
		tsTimestamp.Reset();

	return tsTimestamp;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRIsTimestampValid::KWDRIsTimestampValid()
{
	SetName("IsTimestampValid");
	SetLabel("Check if a timestamp is valid");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Timestamp);
}

KWDRIsTimestampValid::~KWDRIsTimestampValid() {}

KWDerivationRule* KWDRIsTimestampValid::Create() const
{
	return new KWDRIsTimestampValid;
}

Continuous KWDRIsTimestampValid::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp;

	require(IsCompiled());

	tsTimestamp = GetFirstOperand()->GetTimestampValue(kwoObject);
	return (Continuous)tsTimestamp.Check();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRBuildTimestamp::KWDRBuildTimestamp()
{
	SetName("BuildTimestamp");
	SetLabel("Build a timestamp from date and time");
	SetType(KWType::Timestamp);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Date);
	GetOperandAt(1)->SetType(KWType::Time);
}

KWDRBuildTimestamp::~KWDRBuildTimestamp() {}

KWDerivationRule* KWDRBuildTimestamp::Create() const
{
	return new KWDRBuildTimestamp;
}

Timestamp KWDRBuildTimestamp::ComputeTimestampResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp;
	Date dtDate;
	Time tmTime;

	require(IsCompiled());

	// Recherche des valeurs an, mois, jour
	dtDate = GetOperandAt(0)->GetDateValue(kwoObject);
	tmTime = GetOperandAt(1)->GetTimeValue(kwoObject);

	// Initialisation de la timestamp
	tsTimestamp.Reset();
	if (dtDate.Check() and tmTime.Check())
	{
		tsTimestamp.SetDate(dtDate);
		tsTimestamp.SetTime(tmTime);
	}
	return tsTimestamp;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRUtcTimestamp::KWDRUtcTimestamp()
{
	SetName("UtcTimestamp");
	SetLabel("Universal time zone (UTC) timestamp from a timestampTZ");
	SetType(KWType::Timestamp);
	SetOperandNumber(1);
	GetOperandAt(0)->SetType(KWType::TimestampTZ);
}

KWDRUtcTimestamp::~KWDRUtcTimestamp() {}

KWDerivationRule* KWDRUtcTimestamp::Create() const
{
	return new KWDRUtcTimestamp;
}

Timestamp KWDRUtcTimestamp::ComputeTimestampResult(const KWObject* kwoObject) const
{
	TimestampTZ tstzTimestamp;
	Timestamp tsTimestamp;

	require(IsCompiled());

	tstzTimestamp = GetFirstOperand()->GetTimestampTZValue(kwoObject);
	if (tstzTimestamp.Check())
		tsTimestamp = tstzTimestamp.GetUtcTimestamp();
	else
		tsTimestamp.Reset();
	return tsTimestamp;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRLocalTimestamp::KWDRLocalTimestamp()
{
	SetName("LocalTimestamp");
	SetLabel("Local timestamp from a timestampTZ");
	SetType(KWType::Timestamp);
	SetOperandNumber(1);
	GetOperandAt(0)->SetType(KWType::TimestampTZ);
}

KWDRLocalTimestamp::~KWDRLocalTimestamp() {}

KWDerivationRule* KWDRLocalTimestamp::Create() const
{
	return new KWDRLocalTimestamp;
}

Timestamp KWDRLocalTimestamp::ComputeTimestampResult(const KWObject* kwoObject) const
{
	TimestampTZ tstzTimestamp;
	Timestamp tsTimestamp;

	require(IsCompiled());

	tstzTimestamp = GetFirstOperand()->GetTimestampTZValue(kwoObject);
	if (tstzTimestamp.Check())
		tsTimestamp = tstzTimestamp.GetLocalTimestamp();
	else
		tsTimestamp.Reset();
	return tsTimestamp;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRSetTimeZoneMinutes::KWDRSetTimeZoneMinutes()
{
	SetName("SetTimeZoneMinutes");
	SetLabel("Set the time zone total minutes of a timestampTZ");
	SetType(KWType::TimestampTZ);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::TimestampTZ);
	GetOperandAt(1)->SetType(KWType::Continuous);
}

KWDRSetTimeZoneMinutes::~KWDRSetTimeZoneMinutes() {}

KWDerivationRule* KWDRSetTimeZoneMinutes::Create() const
{
	return new KWDRSetTimeZoneMinutes;
}

TimestampTZ KWDRSetTimeZoneMinutes::ComputeTimestampTZResult(const KWObject* kwoObject) const
{
	TimestampTZ tstzTimestamp;
	Continuous cMinutes;
	int nMinutes;
	boolean bOk;

	require(IsCompiled());

	// Modifie la time zone que si les operandes sont valides
	tstzTimestamp = GetFirstOperand()->GetTimestampTZValue(kwoObject);
	bOk = false;
	if (tstzTimestamp.Check())
	{
		cMinutes = GetSecondOperand()->GetContinuousValue(kwoObject);
		if (cMinutes != KWContinuous::GetMissingValue())
		{
			nMinutes = int(floor(cMinutes + 0.5));
			bOk = tstzTimestamp.SetTimeZoneTotalMinutes(nMinutes);
		}
	}
	if (not bOk)
		tstzTimestamp.Reset();
	return tstzTimestamp;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRGetTimeZoneMinutes::KWDRGetTimeZoneMinutes()
{
	SetName("GetTimeZoneMinutes");
	SetLabel("Get the time zone total minutes of a timestampTZ");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetOperandAt(0)->SetType(KWType::TimestampTZ);
}

KWDRGetTimeZoneMinutes::~KWDRGetTimeZoneMinutes() {}

KWDerivationRule* KWDRGetTimeZoneMinutes::Create() const
{
	return new KWDRGetTimeZoneMinutes;
}

Continuous KWDRGetTimeZoneMinutes::ComputeContinuousResult(const KWObject* kwoObject) const
{
	TimestampTZ tstzTimestamp;

	require(IsCompiled());

	// Modifie la time zone que si les operandes sont valides
	tstzTimestamp = GetFirstOperand()->GetTimestampTZValue(kwoObject);
	if (tstzTimestamp.Check())
		return (Continuous)tstzTimestamp.GetTimeZoneTotalMinutes();
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRDiffTimestampTZ::KWDRDiffTimestampTZ()
{
	SetName("DiffTimestampTZ");
	SetLabel("Difference in seconds between two timestampTZs");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::TimestampTZ);
	GetSecondOperand()->SetType(KWType::TimestampTZ);
}

KWDRDiffTimestampTZ::~KWDRDiffTimestampTZ() {}

KWDerivationRule* KWDRDiffTimestampTZ::Create() const
{
	return new KWDRDiffTimestampTZ;
}

Continuous KWDRDiffTimestampTZ::ComputeContinuousResult(const KWObject* kwoObject) const
{
	TimestampTZ tsTimestampTZ1;
	TimestampTZ tsTimestampTZ2;

	require(IsCompiled());

	tsTimestampTZ1 = GetFirstOperand()->GetTimestampTZValue(kwoObject);
	tsTimestampTZ2 = GetSecondOperand()->GetTimestampTZValue(kwoObject);
	if (tsTimestampTZ1.Check() and tsTimestampTZ2.Check())
		return (Continuous)tsTimestampTZ1.Diff(tsTimestampTZ2);
	else
		return KWContinuous::GetMissingValue();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRAddSecondsTSTZ::KWDRAddSecondsTSTZ()
{
	SetName("AddSecondsTSTZ");
	SetLabel("Add a number of seconds to a timestampTZ");
	SetType(KWType::TimestampTZ);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::TimestampTZ);
	GetOperandAt(1)->SetType(KWType::Continuous);
}

KWDRAddSecondsTSTZ::~KWDRAddSecondsTSTZ() {}

KWDerivationRule* KWDRAddSecondsTSTZ::Create() const
{
	return new KWDRAddSecondsTSTZ;
}

TimestampTZ KWDRAddSecondsTSTZ::ComputeTimestampTZResult(const KWObject* kwoObject) const
{
	TimestampTZ tsTimestampTZ;
	Continuous cSeconds;

	require(IsCompiled());

	// Acces aux operandes
	tsTimestampTZ = GetOperandAt(0)->GetTimestampTZValue(kwoObject);
	cSeconds = GetOperandAt(1)->GetContinuousValue(kwoObject);

	// Ajout des jours a la timestampTZ, si validite des operandes
	if (tsTimestampTZ.Check() and cSeconds != KWContinuous::GetMissingValue())
		tsTimestampTZ.AddSeconds((int)floor(cSeconds + 0.5));
	// Sinon, on rend un timestampTZ invalide
	else
		tsTimestampTZ.Reset();

	return tsTimestampTZ;
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRIsTimestampTZValid::KWDRIsTimestampTZValid()
{
	SetName("IsTimestampTZValid");
	SetLabel("Check if a timestampTZ is valid");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::TimestampTZ);
}

KWDRIsTimestampTZValid::~KWDRIsTimestampTZValid() {}

KWDerivationRule* KWDRIsTimestampTZValid::Create() const
{
	return new KWDRIsTimestampTZValid;
}

Continuous KWDRIsTimestampTZValid::ComputeContinuousResult(const KWObject* kwoObject) const
{
	TimestampTZ tsTimestampTZ;

	require(IsCompiled());

	tsTimestampTZ = GetFirstOperand()->GetTimestampTZValue(kwoObject);
	return (Continuous)tsTimestampTZ.Check();
}

//////////////////////////////////////////////////////////////////////////////////////

KWDRBuildTimestampTZ::KWDRBuildTimestampTZ()
{
	SetName("BuildTimestampTZ");
	SetLabel("Build a timestampTZ from a timestamp and the time zone total minutes");
	SetType(KWType::TimestampTZ);
	SetOperandNumber(2);
	GetOperandAt(0)->SetType(KWType::Timestamp);
	GetOperandAt(1)->SetType(KWType::Continuous);
}

KWDRBuildTimestampTZ::~KWDRBuildTimestampTZ() {}

KWDerivationRule* KWDRBuildTimestampTZ::Create() const
{
	return new KWDRBuildTimestampTZ;
}

TimestampTZ KWDRBuildTimestampTZ::ComputeTimestampTZResult(const KWObject* kwoObject) const
{
	Timestamp tsTimestamp;
	TimestampTZ tstzTimestamp;
	Continuous cMinutes;
	int nMinutes;
	boolean bOk;

	require(IsCompiled());

	// Modifie la time zone que si les operandes sont valides
	tsTimestamp = GetFirstOperand()->GetTimestampValue(kwoObject);
	bOk = false;
	tstzTimestamp.Reset();
	if (tsTimestamp.Check())
	{
		cMinutes = GetSecondOperand()->GetContinuousValue(kwoObject);
		if (cMinutes != KWContinuous::GetMissingValue())
		{
			nMinutes = int(floor(cMinutes + 0.5));
			bOk = tstzTimestamp.Init(tsTimestamp, nMinutes);
		}
	}
	return tstzTimestamp;
}
