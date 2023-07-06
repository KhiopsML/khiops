// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class Date;
class KWDateFormat;

#include "Object.h"
#include "Vector.h"

////////////////////////////////////////////////////////////////////////////////////
// Definition du type Date et des services attaches
// Le type Date est traite comme un type elementaire manipulable comme une valeur
// (comme un entier, une chaine de caractere).
// Il n'herite pas de la classe Object et ne peut etre utilise
// directement dans un container

///////////////////////////////////////////////////////////////////////
// Classe Date
// Prefixe pour les variables: dt
// Pas de constructeur pour pouvoir etre utilise dans l'union de KWValue
class Date : public SystemObject
{
public:
	// Remise a zero (0: date invalide)
	void Reset();

	// Initialisation
	// Renvoie true si l'initialisation est valide (entre 01/01/0001 et 31/12/4000)
	boolean Init(int nYear, int nMonth, int nDay);

	// Operateurs de comparaison
	boolean operator==(const Date& dtValue) const;
	boolean operator!=(const Date& dtValue) const;

	////////////////////////////////////////////////
	// Acces aux champs de la date
	// La Date doit etre valide

	// Test si valide
	boolean Check() const;

	// Annee
	int GetYear() const;

	// Mois
	int GetMonth() const;

	// Jour
	int GetDay() const;

	////////////////////////////////////////////////
	// Servives avances
	// La Date doit etre valide

	// Indique si l'annee est bisextile
	boolean IsBissextileYear() const;

	// Jour dans l'annee
	int GetYearDay() const;

	// Jour dans la semaine, de 1 (lundi) a 7 (dimanche)
	int GetWeekDay() const;

	// Annee avec partie decimale selon le jour dans l'annee
	double GetDecimalYear() const;

	// Index de jour absolu: nombre de jour ecoule depuis le 01/01/2000
	int GetAbsoluteDay() const;

	// Difference avec une autre date
	int Diff(const Date dtOtherDate) const;

	// Comparaison avec une autre date
	// Une Date invalide est inferieure a toutes les autres
	int Compare(const Date dtOtherDate) const;

	// Ajout d'un nombre de jour a une date
	// Renvoie true si on obtient une date valide
	boolean AddDays(int nValue);

	// Initialisation avec la date courante
	void SetCurrentDate();

	// Affichage au format par defaut YYYY-MM-DD
	const char* const ToString() const;
	void Write(ostream& ost) const;

	// Valeur interdite, ne pouvant etre prise par aucune date valide ou non
	// Utile pour la gestion avancee des KWType
	void SetForbiddenValue();
	boolean IsForbiddenValue() const;

	// Test des fonctionnalites
	static void UnitTest(int nYear, int nMonth, int nDay);
	static void Test();

	//////////////////////////////////////////////////////
	//// Implementation
protected:
	// Modification des champs de la date
	void SetYear(int nValue);
	void SetMonth(int nValue);
	void SetDay(int nValue);

	// Nombre de jours total depuis le 01/01/4713 BC
	// Utilise car il existe des formules de calcul de conversion entre
	// date Julienne et date Gregorienne (cf. http://en.wikipedia.org/wiki/Julian_day)
	int GetJulianDayNumber() const;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// La date peut stocker une partie time zone, utile quand elle est utilisee par les TimestampTZ
	// qui doivent gerer les time zones
	friend class TimestampTZ;
	friend class KWTimestampTZFormat;

	// Initialisation de la partie time zone
	// Renvoie true si l'initialisation est valide (entre -12:00 et +14:00)
	boolean InitTimeZone(int nSign, int nHour, int nMinute);

	// Initialisation de la partie time zone par son nombre total de minutes
	// Renvoie true si l'initialisation est valide (entre -12:00 et +14:00)
	boolean SetTimeZoneTotalMinutes(int nValue);
	int GetTimeZoneTotalMinutes() const;

	// Signe de la time zone
	void SetTimeZoneSign(int nValue);
	int GetTimeZoneSign() const;

	// Heure de la time zone
	void SetTimeZoneHour(int nValue);
	int GetTimeZoneHour() const;

	// Minute de la time zone
	void SetTimeZoneMinute(int nValue);
	int GetTimeZoneMinute() const;

	// Reinitialisation de la time zone
	void ResetTimeZone();

	// Test si la time zone est presente valide
	boolean CheckTimeZone() const;

	// Affichage de la partie time zone au format zzzzzz si bExtended vaut true, zzzzz sinon
	const char* const TimeZoneToString(boolean bExtended) const;

	// Stockage optimise d'une Date sous forme d'un entier
	// On utilise un champ de bits pour stocker a la fois les information de Date et de time zone, utilises
	// potentiellement dans les cas des TimestampTZ
	// Bits 1 a 12: annee (entre 0 et 4000)
	// Bits 13 a 16: mois (entre 1 et 12)
	// Bits 17 a 21: jour (entre 1 et 31, ou 0 si invalide)
	// Bit 22: signe de la timezone (0 si negatif, 1 si positif)
	// Bits 23 a 26: heure de la timezone (entre 0 et 14)
	// Bits 27 a 32: minute de la timezone (entre 0 et 59)
	// Une time est invalide si elle vaut 0 (heure a 0; validite de 1 a 24), et interdite si elle vaut 0xFFFFFFFF
	// La partie time zone est invalide si elle vaut 0 (une time zone -00:00 est codee +00:00 pour etre valide).
	union DateValue
	{
		unsigned int nDate;
		struct DateFields
		{
			unsigned int nYear : 12;
			unsigned int nMonth : 4;
			unsigned int nDay : 5;
			unsigned int nTimeZoneSign : 1;
			unsigned int nTimeZoneHour : 4;
			unsigned int nTimeZoneMinute : 6;
		} dateFields;
	} dateValue;
};

// Ecriture dans un stream
inline ostream& operator<<(ostream& ost, const Date& value)
{
	value.Write(ost);
	return ost;
}

///////////////////////////////////////////////////////////////////////
// Classe KWDateFormat
// Gestion du format d'entre sortie d'une date
// Un format de date est base sur une chaine de caracteres contenant
// les motifs suivants:
//    YYYY: annee sur 4 caracteres
//    MM: mois sur 2 caracteres
//    DD: jour sur 2 caracteres
// Les trois motifs sont obligatoires et doivent apparaitre une et une seule fois,
// le motif YYYY devant etre en debut ou en fin.
// Il peut y avoir de facon facultative un separateur de motif: '/', '.' ou '-'
// Exemple: YYYYMMDD, YYYY/MM/DD, MM-DD-YYYY
class KWDateFormat : public Object
{
public:
	// Constructeur
	KWDateFormat();
	~KWDateFormat();

	// Parametrage du format par une chaine de caracteres contenant les motifs
	// On retourne true si le parametrage est valide
	boolean SetFormatString(const ALString& sValue);

	// Acces au format sous forme chaine de caracteres
	const ALString& GetFormatString() const;

	// Reinitialisation du format
	void Reset();

	// Test si format valide
	boolean Check() const override;

	// Test si le format est compatible avec un autre format, c'est a dire si cet autre format
	// peut prendre en compte les meme valeurs sans erreur
	boolean IsConsistentWith(const KWDateFormat* otherFormat) const;

	//////////////////////////////////////////////////////////
	// Caracteristique d'un format valide

	// Nombre total de caracteres necessaire a l'encodage d'une date
	int GetTotalCharNumber() const;

	// Acces au caractere separateur ('0' si pas de separateur)
	char GetSeparatorChar() const;

	// Position des separateurs (-1 si pas de separateur)
	int GetSeparatorOffset1() const;
	int GetSeparatorOffset2() const;

	// Position dans le format de l'annee, du mois et du jour (a partir de 0)
	int GetYearOffset() const;
	int GetMonthOffset() const;
	int GetDayOffset() const;

	// Tailles minimum et maximum d'une valeur pour etre compatible avec le format
	int GetMinCharNumber() const;
	int GetMaxCharNumber() const;

	//////////////////////////////////////////////////////////
	// Methodes de conversion

	// Renvoie une date a partir d'une chaine de caracteres
	// La date est invalide en cas de probleme de conversion
	Date StringToDate(const char* const sValue) const;

	// Renvoie une chaine a partir d'une date
	// Renvoie la chaine vide si la date est invalide
	const char* const DateToString(Date dtValue) const;

	///////////////////////////////////////////////////////////
	// Services divers

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Format (chaine) par defaut
	static ALString GetDefaultFormatString();

	// Recherche de tous les formats disponibles, par ordre decroissant de priorite
	// Memoire: le contenu du tableau, rempli par l'appele, est a liberer par l'appelant
	static void GetAllAvailableFormats(ObjectArray* oaAvailableDateFormats);

	// Test des fonctionnalites
	static void UnitTest(const ALString& sInputValue, KWDateFormat* inputFormat, KWDateFormat* outputFormat);
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ajout d'un format dans un tableau
	// La methode construit et initialise un objet KWDateFormat et l'ajoute en fin de tableau
	static void AddFormat(ObjectArray* oaFormats, const ALString& sFormatString);

	// Donnees de format
	ALString sFormatString;
	int nTotalCharNumber;
	char cSeparatorChar;
	int nSeparatorOffset1;
	int nSeparatorOffset2;
	int nYearOffset;
	int nMonthOffset;
	int nDayOffset;
};

// Methodes en inline

inline void Date::Reset()
{
	dateValue.nDate = 0;
}

inline boolean Date::operator==(const Date& dtValue) const
{
	return (dateValue.nDate == dtValue.dateValue.nDate);
}

inline boolean Date::operator!=(const Date& dtValue) const
{
	return (dateValue.nDate != dtValue.dateValue.nDate);
}

inline boolean Date::Check() const
{
	require(not IsForbiddenValue());
	return dateValue.nDate != 0;
}

inline int Date::GetYear() const
{
	require(Check());
	return dateValue.dateFields.nYear;
}

inline int Date::GetMonth() const
{
	require(Check());
	return dateValue.dateFields.nMonth;
}

inline int Date::GetDay() const
{
	require(Check());
	return dateValue.dateFields.nDay;
}

inline int Date::Diff(const Date dtOtherDate) const
{
	int nCompare;
	require(Check() and dtOtherDate.Check());
	nCompare = GetJulianDayNumber() - dtOtherDate.GetJulianDayNumber();
	return nCompare;
}

inline int Date::Compare(const Date dtOtherDate) const
{
	int nCompare;

	require(not IsForbiddenValue() and not dtOtherDate.IsForbiddenValue());

	if (Check() and dtOtherDate.Check())
		nCompare = Diff(dtOtherDate);
	else if (not Check() and not dtOtherDate.Check())
		nCompare = 0;
	else if (not Check())
		nCompare = -1;
	else
		nCompare = 1;
	return nCompare;
}

inline void Date::SetYear(int nValue)
{
	require(1 <= nValue and nValue <= 4000);
	dateValue.dateFields.nYear = nValue;
}

inline void Date::SetMonth(int nValue)
{
	require(1 <= nValue and nValue <= 12);
	dateValue.dateFields.nMonth = nValue;
}

inline void Date::SetDay(int nValue)
{
	require(1 <= nValue and nValue <= 31);
	dateValue.dateFields.nDay = nValue;
}

inline void Date::SetForbiddenValue()
{
	dateValue.nDate = 0xFFFFFFFF;
}

inline boolean Date::IsForbiddenValue() const
{
	return (dateValue.nDate == 0xFFFFFFFF);
}

inline boolean Date::InitTimeZone(int nSign, int nHour, int nMinute)
{
	int nTotalMinutes;

	// Initialisation de la timezone a invalide
	ResetTimeZone();

	// Initialisation si time zone valide
	if ((nSign == -1 or nSign == 1) and (0 <= nHour and nHour <= 14) and (0 <= nMinute and nMinute <= 59))
	{
		nTotalMinutes = nSign * (nHour * 60 + nMinute);
		if (-12 * 60 <= nTotalMinutes and nTotalMinutes <= 14 * 60)
		{
			// Cas particulier de -00:00 que l'on code en +0:00, parce que -00:00 correspoind au codage
			// d'une time zone absente
			if (nHour == 0 and nMinute == 0)
				SetTimeZoneSign(1);
			else
				SetTimeZoneSign(nSign);
			SetTimeZoneHour(nHour);
			SetTimeZoneMinute(nMinute);
			return true;
		}
	}
	return false;
}

inline boolean Date::SetTimeZoneTotalMinutes(int nValue)
{
	// Initialisation de la timezone a invalide
	ResetTimeZone();

	// Initialisation si time zone valide
	if (-12 * 60 <= nValue and nValue <= 14 * 60)
	{
		if (nValue >= 0)
			SetTimeZoneSign(1);
		else
			SetTimeZoneSign(-1);
		SetTimeZoneHour(abs(nValue) / 60);
		SetTimeZoneMinute(abs(nValue) % 60);
		return true;
	}
	return false;
}

inline int Date::GetTimeZoneTotalMinutes() const
{
	require(CheckTimeZone());
	return GetTimeZoneSign() * (60 * GetTimeZoneHour() + GetTimeZoneMinute());
}

inline void Date::SetTimeZoneSign(int nValue)
{
	require(nValue == -1 or nValue == 1);
	dateValue.dateFields.nTimeZoneSign = (nValue + 1) / 2;
}

inline int Date::GetTimeZoneSign() const
{
	require(CheckTimeZone());
	return (dateValue.dateFields.nTimeZoneSign * 2) - 1;
}

inline void Date::SetTimeZoneHour(int nValue)
{
	require(0 <= nValue and nValue <= 14);
	dateValue.dateFields.nTimeZoneHour = nValue;
}

inline int Date::GetTimeZoneHour() const
{
	require(CheckTimeZone());
	return dateValue.dateFields.nTimeZoneHour;
}

inline void Date::SetTimeZoneMinute(int nValue)
{
	require(0 <= nValue and nValue <= 59);
	dateValue.dateFields.nTimeZoneMinute = nValue;
}

inline int Date::GetTimeZoneMinute() const
{
	require(CheckTimeZone());
	return dateValue.dateFields.nTimeZoneMinute;
}

inline void Date::ResetTimeZone()
{
	dateValue.dateFields.nTimeZoneSign = 0;
	dateValue.dateFields.nTimeZoneHour = 0;
	dateValue.dateFields.nTimeZoneMinute = 0;
}

inline boolean Date::CheckTimeZone() const
{
	require(not IsForbiddenValue());
	return dateValue.dateFields.nTimeZoneSign != 0 or dateValue.dateFields.nTimeZoneHour != 0 or
	       dateValue.dateFields.nTimeZoneMinute != 0;
}

// KWDateFormat

inline boolean KWDateFormat::Check() const
{
	return nTotalCharNumber > 0;
}

inline int KWDateFormat::GetTotalCharNumber() const
{
	require(Check());
	return nTotalCharNumber;
}

inline char KWDateFormat::GetSeparatorChar() const
{
	require(Check());
	return cSeparatorChar;
}

inline int KWDateFormat::GetSeparatorOffset1() const
{
	require(Check());
	return nSeparatorOffset1;
}

inline int KWDateFormat::GetSeparatorOffset2() const
{
	require(Check());
	return nSeparatorOffset2;
}

inline int KWDateFormat::GetYearOffset() const
{
	require(Check());
	return nYearOffset;
}

inline int KWDateFormat::GetMonthOffset() const
{
	require(Check());
	return nMonthOffset;
}

inline int KWDateFormat::GetDayOffset() const
{
	require(Check());
	return nDayOffset;
}

inline int KWDateFormat::GetMinCharNumber() const
{
	require(Check());
	return nTotalCharNumber;
}

inline int KWDateFormat::GetMaxCharNumber() const
{
	require(Check());
	return nTotalCharNumber;
}
