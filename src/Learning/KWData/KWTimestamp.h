// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class Timestamp;
class KWTimestampFormat;

#include "Object.h"
#include "KWDate.h"
#include "KWTime.h"

////////////////////////////////////////////////////////////////////////////////////
// Definition du type Timestamp et des services attaches
// Le type Timestamp est traite comme un type elementaire manipulable comme une valeur
// (comme un entier, une chaine de caractere).
// Il n'herite pas de la classe Object et ne peut etre utilise
// directement dans un container

///////////////////////////////////////////////////////////////////////
// Classe Timestamp
// Prefixe pour les variables: ts
// Pas de constructeur pour pouvoir etre utilise dans l'union de KWValue
class Timestamp : public SystemObject
{
public:
	// Remise a zero (0: Timestamp invalide)
	void Reset();

	// Initialisation
	// Renvoie true si l'initialisation est valide
	boolean Init(int nYear, int nMonth, int nDay, int nHour, int nMinute, double dSecond);

	// Operateurs de comparaison
	boolean operator==(const Timestamp& tsValue) const;
	boolean operator!=(const Timestamp& tsValue) const;

	// Test si valide
	boolean Check() const;

	////////////////////////////////////////////////
	// Acces aux champs de la Timestamp par valeur

	// Date
	const Date GetDate() const;
	void SetDate(const Date dtValue);

	// Time
	const Time GetTime() const;
	void SetTime(const Time tmValue);

	////////////////////////////////////////////////
	// Servives avances
	// Le TimeStamp doit etre valide

	// Annee avec partie decimale selon le jour dans l'annee
	double GetDecimalYear() const;

	// Nombre total  de secondes ecoule depuis le 01/01/2000
	double GetAbsoluteSecond() const;

	// Jour decimal de la semaine depuis lundi 00:00:00, entre 1.0 et 7.99..., en tenant compte
	// des heures converties en fraction de jour
	double GetDecimalWeekDay() const;

	// Difference avec une autre Time en secondes
	double Diff(const Timestamp tsOtherTimestamp) const;

	// Comparaison avec un autre Timestamp
	// Un Timestamp invalide est inferieur a tous les autres
	int Compare(const Timestamp tsOtherTimestamp) const;

	// Ajout d'un nombre de secondes a un Timestamp
	// Renvoie true si on obtient une Timestamp valide
	boolean AddSeconds(double dSeconds);

	// Initialisation avec la Timestamp courante
	void SetCurrentTimestamp();

	// Affichage au format par defaut YYYY-MM-DD HH:MM:SS.
	const char* const ToString() const;
	void Write(ostream& ost) const;

	// Valeur interdite, ne pouvant etre prise par aucune Timestamp valide ou non
	// Utile pour la gestion avancee des KWType
	void SetForbiddenValue();
	boolean IsForbiddenValue() const;

	// Test des fonctionnalites
	static void UnitTest(int nYear, int nMonth, int nDay, int nHour, int nMinute, double dSecond);
	static void Test();

	//////////////////////////////////////////////////////
	//// Implementation
protected:
	// Stockage d'un Timestamp sous forme d'une Date et Time
	// On n'utilise pas directement les type Date et Time car sous linux il y a un probleme de packing
	// et le sizeof(Timestamp) fait plus de 8 octets si on utilise les type Date et Time
	// (ce probleme apparait meme si on utilise les primitives specifiques a Linux pour desactiver le packing.
	// on n'a pas ce probleme sous windows)
	unsigned int nDate;
	unsigned int nTime;

	// Classes en friend pour acceder aux informations interne de Date et Time
	friend class KWTimestampFormat;
	friend class TimestampTZ;

	// Acces aux attributs en mode mise a jour
	// L'acces par reference permet de les utilisater en affectation
	// Exemple: GetInternalDate() = myDate, ou GetInternalDate().Reset()
	Date& GetInternalDate() const;
	Time& GetInternalTime() const;
};

// Ecriture dans un stream
inline ostream& operator<<(ostream& ost, const Timestamp& value)
{
	value.Write(ost);
	return ost;
}

///////////////////////////////////////////////////////////////////////
// Classe KWTimestampFormat
// Gestion du format d'entre sortie d'une Timestamp
// Un format de Timestamp est base sur une chaine de caracteres contenant
// un motif de date (KWDateFormat) et un motif de Time (KWTimeFormat)
// Il doit y avoir un separateur de motif: ' ', '-' ou '_', sauf dans le cas
// ou ni la Date ni la Time n'ont de separateur
// Exemple: YYYYMMDDHHMMSS, YYYY/MM/DD-HH:MM:SS.
class KWTimestampFormat : public Object
{
public:
	// Constructeur
	KWTimestampFormat();
	~KWTimestampFormat();

	// Parametrage du format par une chaine de caracteres contenant les motifs
	// On retourne true si le parametrage est valide
	boolean SetFormatString(const ALString& sValue);

	// Acces au format sous forme chaine de caracteres
	const ALString& GetFormatString() const;

	// Test si format valide
	boolean Check() const override;

	// Reinitialisation du format
	void Reset();

	// Test si le format est compatible avec un autre format, c'est a dire si cet autre format
	// peut prendre en compte les memes valeurs sans erreur
	boolean IsConsistentWith(const KWTimestampFormat* otherFormat) const;

	//////////////////////////////////////////////////////////
	// Caracteristique d'un format valide

	// Format de la partie Date du Timestamp
	const KWDateFormat* GetDateFormat() const;

	// Format de la partie Time du Timestamp
	const KWTimeFormat* GetTimeFormat() const;

	// Nombre total de caracteres necessaire a l'encodage d'une Timestamp
	int GetTotalCharNumber() const;

	// Acces au caractere separateur ('0' si pas de separateur)
	char GetSeparatorChar() const;

	// Position du separateur (-1 si pas de separateur)
	int GetSeparatorOffset() const;

	// Position du format time
	int GetTimeOffset() const;

	// Tailles minimum et maximum d'une valeur pour etre compatible avec le format
	// en ignorant la partie decimale des secondes
	int GetMinCharNumber() const;
	int GetMaxCharNumber() const;

	//////////////////////////////////////////////////////////
	// Methodes de conversion

	// Renvoie une Timestamp a partir d'une chaine de caracteres
	// La Timestamp est invalide en cas de probleme de conversion
	Timestamp StringToTimestamp(const char* const sValue) const;

	// Renvoie une chaine a partir d'une Timestamp
	// Renvoie la chaine vide si la Timestamp est invalide
	const char* const TimestampToString(Timestamp tsValue) const;

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
	static void GetAllAvailableFormats(ObjectArray* oaAvailableTimestampFormats);

	// Test des fonctionnalites
	static void UnitTest(const ALString& sInputValue, KWTimestampFormat* inputFormat,
			     KWTimestampFormat* outputFormat);
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ajout d'un format dans un tableau
	// La methode construit et initialise un objet KWTimestampFormat et l'ajoute en fin de tableau
	static void AddFormat(ObjectArray* oaFormats, const ALString& sFormatString);

	// Donnees de format
	ALString sFormatString;
	KWDateFormat dateFormat;
	KWTimeFormat timeFormat;
	int nTotalCharNumber;
	char cSeparatorChar;
	int nSeparatorOffset;
	int nTimeOffset;
	int nMinCharNumber;
	int nMaxCharNumber;
};

// Methodes en inline

inline void Timestamp::Reset()
{
	GetInternalDate().Reset();
	GetInternalTime().Reset();
}

inline boolean Timestamp::operator==(const Timestamp& tsValue) const
{
	return (GetDate() == tsValue.GetDate() and GetTime() == tsValue.GetTime());
}

inline boolean Timestamp::operator!=(const Timestamp& tsValue) const
{
	return (GetDate() != tsValue.GetDate() or GetTime() != tsValue.GetTime());
}

inline boolean Timestamp::Check() const
{
	require(not IsForbiddenValue());
	return GetDate().Check() and GetTime().Check();
}

inline const Date Timestamp::GetDate() const
{
	return (const Date)GetInternalDate();
}

inline void Timestamp::SetDate(const Date dtValue)
{
	GetInternalDate() = dtValue;
}

inline const Time Timestamp::GetTime() const
{
	return (const Time)GetInternalTime();
}

inline void Timestamp::SetTime(const Time tmValue)
{
	GetInternalTime() = tmValue;
}

inline double Timestamp::GetAbsoluteSecond() const
{
	double dAbsoluteSecond = 0;

	require(Check());
	dAbsoluteSecond = GetDate().GetAbsoluteDay() * 86400 + GetTime().GetDaySecond();
	return dAbsoluteSecond;
}

inline double Timestamp::GetDecimalWeekDay() const
{
	double dDecimalWeekDay = 0;

	require(Check());
	dDecimalWeekDay = GetDate().GetWeekDay() + GetTime().GetDecimalTime() / 24.0;
	return dDecimalWeekDay;
}

inline double Timestamp::Diff(const Timestamp tsOtherTimestamp) const
{
	double dDiff;
	require(Check() and tsOtherTimestamp.Check());
	dDiff = GetDate().Diff(tsOtherTimestamp.GetDate()) * 86400.0 + GetTime().Diff(tsOtherTimestamp.GetTime());
	return dDiff;
}

inline int Timestamp::Compare(const Timestamp tsOtherTimestamp) const
{
	double dCompare;
	int nCompare;

	require(not IsForbiddenValue() and not tsOtherTimestamp.IsForbiddenValue());

	if (Check() and tsOtherTimestamp.Check())
	{
		dCompare = Diff(tsOtherTimestamp);
		if (dCompare == 0)
			nCompare = 0;
		else if (dCompare > 0)
			nCompare = 1;
		else
			nCompare = -1;
	}
	else if (not Check() and not tsOtherTimestamp.Check())
		nCompare = 0;
	else if (not Check())
		nCompare = -1;
	else
		nCompare = 1;
	return nCompare;
}

inline void Timestamp::SetForbiddenValue()
{
	GetInternalDate().SetForbiddenValue();
	GetInternalTime().SetForbiddenValue();
}

inline boolean Timestamp::IsForbiddenValue() const
{
	return GetDate().IsForbiddenValue() or GetTime().IsForbiddenValue();
}

inline Date& Timestamp::GetInternalDate() const
{
	return (Date&)nDate;
}

inline Time& Timestamp::GetInternalTime() const
{
	return (Time&)nTime;
}

// KWTimestampFormat

inline boolean KWTimestampFormat::Check() const
{
	return nTotalCharNumber > 0;
}

inline const KWDateFormat* KWTimestampFormat::GetDateFormat() const
{
	return &dateFormat;
}

inline const KWTimeFormat* KWTimestampFormat::GetTimeFormat() const
{
	return &timeFormat;
}

inline int KWTimestampFormat::GetTotalCharNumber() const
{
	require(Check());
	return nTotalCharNumber;
}

inline char KWTimestampFormat::GetSeparatorChar() const
{
	require(Check());
	return cSeparatorChar;
}

inline int KWTimestampFormat::GetSeparatorOffset() const
{
	require(Check());
	return nSeparatorOffset;
}

inline int KWTimestampFormat::GetTimeOffset() const
{
	require(Check());
	return nTimeOffset;
}

inline int KWTimestampFormat::GetMinCharNumber() const
{
	require(Check());
	return nMinCharNumber;
}

inline int KWTimestampFormat::GetMaxCharNumber() const
{
	require(Check());
	return nMaxCharNumber;
}
