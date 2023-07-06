// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation de traitement des dates et heures
// Les dates sont codees au format natif YYYY-MM-DD
// Les heures sont codees au format natif HH:MM:SS
// Les timestamps sont codees au format natif YYYY-MM-DD HH:MM:SS.

// Date
class KWDRYear;
class KWDRMonth;
class KWDRDay;
class KWDRYearDay;
class KWDRWeekDay;
class KWDRDecimalYear;
class KWDRAbsoluteDay;
class KWDRDiffDate;
class KWDRAddDays;
class KWDRIsDateValid;
class KWDRBuildDate;

// Time
class KWDRHour;
class KWDRMinute;
class KWDRSecond;
class KWDRDaySecond;
class KWDRDecimalTime;
class KWDRDiffTime;
class KWDRIsTimeValid;
class KWDRBuildTime;

// Timestamp
class KWDRGetDate;
class KWDRGetTime;
class KWDRDecimalYearTS;
class KWDRAbsoluteSecond;
class KWDRDecimalWeekDay;
class KWDRDiffTimestamp;
class KWDRAddSeconds;
class KWDRIsTimestampValid;
class KWDRBuildTimestamp;

// TimestampTZ
class KWDRUtcTimestamp;
class KWDRLocalTimestamp;
class KWDRSetTimeZoneMinutes;
class KWDRGetTimeZoneMinutes;
class KWDRDiffTimestampTZ;
class KWDRAddSecondsTZ;
class KWDRIsTimestampValidTZ;
class KWDRBuildTimestampTZ;

#include "KWDerivationRule.h"
#include "KWDate.h"
#include "KWTime.h"
#include "KWTimestamp.h"

// Enregistrement de ces regles
void KWDRRegisterDateTimeRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRYear
// Annee d'une date
// Renvoie MissingValue si date non valide
class KWDRYear : public KWDerivationRule
{
public:
	// Constructeur
	KWDRYear();
	~KWDRYear();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRMonth
// Mois d'une date
// Renvoie MissingValue si date non valide
class KWDRMonth : public KWDerivationRule
{
public:
	// Constructeur
	KWDRMonth();
	~KWDRMonth();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDay
// Jour d'une date
// Renvoie MissingValue si date non valide
class KWDRDay : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDay();
	~KWDRDay();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRYearDay
// Jour dans l'annee d'une date
// Renvoie MissingValue si date non valide
class KWDRYearDay : public KWDerivationRule
{
public:
	// Constructeur
	KWDRYearDay();
	~KWDRYearDay();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRWeekDay
// Jour dans la semaine d'une date
// Renvoie entre 1 (lundi) et 7 (dimanche)
// Renvoie MissingValue si date non valide
class KWDRWeekDay : public KWDerivationRule
{
public:
	// Constructeur
	KWDRWeekDay();
	~KWDRWeekDay();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDecimalYear
// Annee avec partie decimal pour les fractions d'annees
// Renvoie MissingValue si date non valide
class KWDRDecimalYear : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDecimalYear();
	~KWDRDecimalYear();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAbsoluteDay
// Nombre de jours ecoules depuis 2000-01-01
// Renvoie MissingValue si date non valide
class KWDRAbsoluteDay : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAbsoluteDay();
	~KWDRAbsoluteDay();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
public:
	static Date dtRefDate;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDiffDate
// Difference entre deux dates en nombre de jours
// Renvoie MissingValue si une des dates n'est pas valide
class KWDRDiffDate : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDiffDate();
	~KWDRDiffDate();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAddDays
// Ajout de jours a une date
// Renvoie une date invalide si la date est invalide ou si les jours sont missing
class KWDRAddDays : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAddDays();
	~KWDRAddDays();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Date ComputeDateResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRIsDateValid
// Renvoie entre 1 si date valide, 0 sinon
class KWDRIsDateValid : public KWDerivationRule
{
public:
	// Constructeur
	KWDRIsDateValid();
	~KWDRIsDateValid();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildDate
// Construction d'un date a partir de annee, mois, jour
// La date resultante est potentiellement non valide
class KWDRBuildDate : public KWDerivationRule
{
public:
	// Constructeur
	KWDRBuildDate();
	~KWDRBuildDate();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Date ComputeDateResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRHour
// Heure d'un attribut heure
// Renvoie MissingValue si heure non valide
class KWDRHour : public KWDerivationRule
{
public:
	// Constructeur
	KWDRHour();
	~KWDRHour();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRMinute
// Minute d'un attribut heure
// Renvoie MissingValue si heure non valide
class KWDRMinute : public KWDerivationRule
{
public:
	// Constructeur
	KWDRMinute();
	~KWDRMinute();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSecond
// Seconde d'un attribut heure
// Renvoie MissingValue si heure non valide
class KWDRSecond : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSecond();
	~KWDRSecond();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDaySecond
// Nombre de secondes ecoules dans la journee
// Renvoie MissingValue si time non valide
class KWDRDaySecond : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDaySecond();
	~KWDRDaySecond();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDecimalTime
// Heure decimale, entre 0 et 23.99...
// Renvoie MissingValue si time non valide
class KWDRDecimalTime : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDecimalTime();
	~KWDRDecimalTime();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDiffTime
// Difference de deux time en nombre de secondes
// Renvoie MissingValue si une des heures n'est pas valide
class KWDRDiffTime : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDiffTime();
	~KWDRDiffTime();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRIsTimeValid
// Renvoie entre 1 si time valide, 0 sinon
class KWDRIsTimeValid : public KWDerivationRule
{
public:
	// Constructeur
	KWDRIsTimeValid();
	~KWDRIsTimeValid();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTime
// Construction d'une time a partir de heure, minute, seconde
// La time resultante est potentiellement non valide
class KWDRBuildTime : public KWDerivationRule
{
public:
	// Constructeur
	KWDRBuildTime();
	~KWDRBuildTime();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Time ComputeTimeResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetDate
// Jour dans l'annee d'un timestamp
// Renvoie MissingValue si timestamp non valide
class KWDRGetDate : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGetDate();
	~KWDRGetDate();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Date ComputeDateResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetTime
// Jour dans la semaine d'un timestamp
// Renvoie entre 1 (lundi) et 7 (dimanche)
// Renvoie MissingValue si timestamp non valide
class KWDRGetTime : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGetTime();
	~KWDRGetTime();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Time ComputeTimeResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDecimalYearTS
// Annee avec partie decimal pour les fractions d'annees
// Renvoie MissingValue si date non valide
class KWDRDecimalYearTS : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDecimalYearTS();
	~KWDRDecimalYearTS();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAbsoluteSecond
// Nombre de secondes ecoules depuis 2000-01-01 00:00:00
// Renvoie MissingValue si timestamp non valide
class KWDRAbsoluteSecond : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAbsoluteSecond();
	~KWDRAbsoluteSecond();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
public:
	static Timestamp tsRefTimestamp;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDecimalWeekDay
// Jour decimal de la semaine depuis lundi 00:00:00, entre 0 et 7, en tenant compte
// des heures converties en fraction de jour
// Renvoie MissingValue si timestamp non valide
class KWDRDecimalWeekDay : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDecimalWeekDay();
	~KWDRDecimalWeekDay();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDiffTimestamp
// Difference entre deux timestamps en nombre de secondes
// Renvoie MissingValue si une des timestamps n'est pas valide
class KWDRDiffTimestamp : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDiffTimestamp();
	~KWDRDiffTimestamp();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAddSeconds
// Ajout de secondes a un timestamp
// Renvoie un timestamp invalide si le timestamps est invalide ou si les secondes sont missing
class KWDRAddSeconds : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAddSeconds();
	~KWDRAddSeconds();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Timestamp ComputeTimestampResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRIsTimestampValid
// Renvoie entre 1 si timestamp valide, 0 sinon
class KWDRIsTimestampValid : public KWDerivationRule
{
public:
	// Constructeur
	KWDRIsTimestampValid();
	~KWDRIsTimestampValid();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTimestamp
// Construction d'un timestamp a partir de date et time
// Le timestamp resultant est potentiellement non valide
class KWDRBuildTimestamp : public KWDerivationRule
{
public:
	// Constructeur
	KWDRBuildTimestamp();
	~KWDRBuildTimestamp();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Timestamp ComputeTimestampResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRUtcTimestamp
// Passage a la time zone UTC, en supprimant les informations de time zone
// Le timestamp resultane est potentiellement non valide
class KWDRUtcTimestamp : public KWDerivationRule
{
public:
	// Constructeur
	KWDRUtcTimestamp();
	~KWDRUtcTimestamp();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Timestamp ComputeTimestampResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRLocalTimestamp
// Passage a la time zone locale, en supprimant les informations de time zone
// Le timestamp resultant est potentiellement non valide
class KWDRLocalTimestamp : public KWDerivationRule
{
public:
	// Constructeur
	KWDRLocalTimestamp();
	~KWDRLocalTimestamp();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Timestamp ComputeTimestampResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSetTimeZoneMinutes
// Initialisation de la partie time zone d'un timestamp valide par son nombre total de minutes
// Nombre total de minutes de la time zone (+- (hh*60 + mm)
// Le timestamp resultant est potentiellement non valide
class KWDRSetTimeZoneMinutes : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSetTimeZoneMinutes();
	~KWDRSetTimeZoneMinutes();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	TimestampTZ ComputeTimestampTZResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetTimeZoneMinutes
// Renvoie le nombre de minutes de la time zone d'un timestamp
// Le resultat est potentiellement Missing
class KWDRGetTimeZoneMinutes : public KWDerivationRule
{
public:
	// Constructeur
	KWDRGetTimeZoneMinutes();
	~KWDRGetTimeZoneMinutes();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDiffTimestampTZ
// Difference entre deux timestampTZ en nombre de secondes
// Renvoie MissingValue si une des timestampTZ n'est pas valide
class KWDRDiffTimestampTZ : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDiffTimestampTZ();
	~KWDRDiffTimestampTZ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAddSeconds
// Ajout de secondes a un timestampTZ
// Renvoie un timestampTZ invalide si le timestampTZs est invalide ou si les secondes sont missing
class KWDRAddSecondsTSTZ : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAddSecondsTSTZ();
	~KWDRAddSecondsTSTZ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	TimestampTZ ComputeTimestampTZResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRIsTimestampTZValid
// Renvoie entre 1 si timestampTZ valide, 0 sinon
class KWDRIsTimestampTZValid : public KWDerivationRule
{
public:
	// Constructeur
	KWDRIsTimestampTZValid();
	~KWDRIsTimestampTZValid();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTimestampTZ
// Construction d'un timestampTZ a partir d'un time stamp et d'un nombretotal de minutes de time zone
// Le timestampTZ resultant est potentiellement non valide
class KWDRBuildTimestampTZ : public KWDerivationRule
{
public:
	// Constructeur
	KWDRBuildTimestampTZ();
	~KWDRBuildTimestampTZ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	TimestampTZ ComputeTimestampTZResult(const KWObject* kwoObject) const override;
};