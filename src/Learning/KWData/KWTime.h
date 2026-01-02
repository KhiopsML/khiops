// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class Time;
class KWTimeFormat;

#include "Object.h"
#include "KWDateTime.h"
#include "Vector.h"

////////////////////////////////////////////////////////////////////////////////////
// Definition du type Time et des services attaches
// Le type Time est traite comme un type elementaire manipulable comme une valeur
// (comme un entier, une chaine de caractere).
// Il n'herite pas de la classe Object et ne peut etre utilise
// directement dans un container

///////////////////////////////////////////////////////////////////////
// Classe Time
// Prefixe pour les variables: tm
// Pas de constructeur pour pouvoir etre utilise dans l'union de KWValue
class Time : public SystemObject
{
public:
	// Remise a zero (0: Time invalide)
	void Reset();

	// Initialisation
	// Renvoie true si l'initialisation est valide
	// Les heures sont entre 0 et 23, les minutes et secondes entre 0 et 60 exclu,
	// les secondes peuvent etre decimales
	boolean Init(int nHour, int nMinute, double dSecond);

	// Operateurs de comparaison
	boolean operator==(const Time& tmValue) const;
	boolean operator!=(const Time& tmValue) const;

	////////////////////////////////////////////////
	// Acces aux champs de la Time
	// La Time doit etre valide

	// Test si valide
	boolean Check() const;

	// Heure
	int GetHour() const;

	// Minute
	int GetMinute() const;

	// Seconde
	double GetSecond() const;

	////////////////////////////////////////////////
	// Servives avances
	// La Time doit etre valide

	// Nombre total  de secondes de la journee; avec une precision potentielle jusqu'au 1/1000 de seconde
	double GetDaySecond() const;

	// Heure decimale, entre 0 et 23.99...
	double GetDecimalTime() const;

	// Difference avec une autre Time en secondes
	double Diff(const Time tmOtherTime) const;

	// Comparaison avec une autre Time
	// Une Time invalide est inferieure a toutes les autres
	int Compare(const Time tmOtherTime) const;

	// Initialisation avec la Time courante
	void SetCurrentTime();

	// Affichage au format par defaut HH:MM:SS.
	const char* const ToString() const;
	void Write(ostream& ost) const;

	// Valeur interdite, ne pouvant etre prise par aucune Time valide ou non
	// Utile pour la gestion avancee des KWType
	void SetForbiddenValue();
	boolean IsForbiddenValue() const;

	// Test des fonctionnalites
	static void UnitTest(int nHour, int nMinute, double dSecond);
	static void Test();

	//////////////////////////////////////////////////////
	//// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////////////////////
	// La time peut stocker une partie timestamp
	friend class Timestamp;

	// Modification des champs de la Time
	void SetHour(int nValue);
	void SetMinute(int nValue);
	void SetSecond(double dValue);

	// Utilisation d'une union DateTime pour acceder au champs Time, dans une structure commune aux type temporels
	union DateTime timeValue;
};

// Ecriture dans un stream
inline ostream& operator<<(ostream& ost, const Time& value)
{
	value.Write(ost);
	return ost;
}

///////////////////////////////////////////////////////////////////////
// Classe KWTimeFormat
// Gestion du format d'entre sortie d'une Time
// Un format de Time est base sur une chaine de caracteres contenant
// les motifs suivants:
//    HH ou (H)H: heure sur 2 caracteres, ou avec le premier chiffre facultatif s'il s'agit d'un '0'
//    MM ou (M): minute sur 2 caracteres, ou avec le premier chiffre facultatif s'il s'agit d'un '0'
//    SS ou (S): second sur 2 caracteres, ou avec le premier chiffre facultatif s'il s'agit d'un '0'
// Les deux premier motifs sont obligatoires et doivent apparaitre une et une seule fois,
// le motif des secondes est facultatif.
// Les trois motifs doivent apparaitre dans l'ordre.
// Le cas des premiers chiffres facultatifs (H), (M) ou (S) doit etre coherent sur l'ensemble des motifs,
// et utilise conjointement avec caractere separateur non vide.
// Il peut y avoir de facon facultative un separateur de motif: ' ', '.' ou ':'
// Un point '.' apres le motif des secondes indique la presence optionnelle d'une partie decimale
// Exemple: HHMMSS, HH:MM:SS, HH:MM, HH.MM.SS.
class KWTimeFormat : public Object
{
public:
	// Constructeur
	KWTimeFormat();
	~KWTimeFormat();

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
	// peut prendre en compte les meme valeurs sans erreur
	// Par exemple: HH:MM:SS est compatible avec HH:MM:SS., our avec (H)H:(M)M:(S)S
	boolean IsConsistentWith(const KWTimeFormat* otherFormat) const;

	//////////////////////////////////////////////////////////
	// Caracteristique d'un format valide

	// Utilisation de premiers chiffres obligatoire pour les heures, minutes et secondes
	boolean GetMandatoryFirstDigit() const;

	// Nombre total de caracteres necessaire a l'encodage d'une Time
	int GetTotalCharNumber() const;

	// Acces au caractere separateur ('0' si pas de separateur)
	char GetSeparatorChar() const;

	// Position des separateurs (-1 si pas de separateur)
	int GetSeparatorOffset1() const;
	int GetSeparatorOffset2() const;

	// Position dans le format des heures, minutes et secondes (a partir de 0)
	int GetHourOffset() const;
	int GetMinuteOffset() const;
	int GetSecondOffset() const;

	// Position du separateur decimal pour les secondes (-1 si pas de partie decimale)
	int GetDecimalPointOffset() const;

	// Tailles minimum et maximum d'une valeur pour etre compatible avec le format
	// en ignorant la partie decimale des secondes
	int GetMinCharNumber() const;
	int GetMaxCharNumber() const;

	//////////////////////////////////////////////////////////
	// Methodes de conversion

	// Renvoie une Time a partir d'une chaine de caracteres
	// La Time est invalide en cas de probleme de conversion
	Time StringToTime(const char* const sValue) const;

	// Renvoie une chaine a partir d'une Time
	// Renvoie la chaine vide si la Time est invalide
	const char* const TimeToString(Time tmValue) const;

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
	static void UnitTest(const ALString& sInputValue, KWTimeFormat* inputFormat, KWTimeFormat* outputFormat);
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ajout d'un format dans un tableau
	// La methode construit et initialise un objet KWDateFormat et l'ajoute en fin de tableau
	static void AddFormat(ObjectArray* oaDateFormats, const ALString& sFormatString);

	// Donnees de format
	ALString sFormatString;
	boolean bMandatoryFirstDigit;
	int nTotalCharNumber;
	char cSeparatorChar;
	int nSeparatorOffset1;
	int nSeparatorOffset2;
	int nHourOffset;
	int nMinuteOffset;
	int nSecondOffset;
	int nDecimalPointOffset;
	int nMinCharNumber;
	int nMaxCharNumber;
};

// Methodes en inline

inline void Time::Reset()
{
	timeValue.lBytes = 0;
}

inline boolean Time::operator==(const Time& tmValue) const
{
	return (timeValue.lBytes == tmValue.timeValue.lBytes);
}

inline boolean Time::operator!=(const Time& tmValue) const
{
	return (timeValue.lBytes != tmValue.timeValue.lBytes);
}

inline boolean Time::Check() const
{
	require(not IsForbiddenValue());
	return timeValue.lBytes != 0;
}

inline int Time::GetHour() const
{
	require(Check());
	return timeValue.fields.nHour - 1;
}

inline int Time::GetMinute() const
{
	require(Check());
	return timeValue.fields.nMinute;
}

inline double Time::GetSecond() const
{
	require(Check());
	return timeValue.fields.nSecond + timeValue.fields.nFrac / (double)DateTime::nMaxFracSeconds;
}

inline double Time::GetDaySecond() const
{
	double dTotalSeconds = 0;

	require(Check());
	dTotalSeconds = 3600 * GetHour() + 60 * GetMinute() + GetSecond();
	return dTotalSeconds;
}

inline double Time::GetDecimalTime() const
{
	double dDecimalTime = 0;

	require(Check());
	dDecimalTime = GetHour() + (GetMinute() * 60 + GetSecond()) / 3600;
	return dDecimalTime;
}

inline double Time::Diff(const Time tmOtherTime) const
{
	double dDiff;
	require(Check() and tmOtherTime.Check());
	dDiff = GetDaySecond() - tmOtherTime.GetDaySecond();
	return dDiff;
}

inline int Time::Compare(const Time tmOtherTime) const
{
	double dCompare;
	int nCompare;

	require(not IsForbiddenValue() and not tmOtherTime.IsForbiddenValue());

	if (Check() and tmOtherTime.Check())
	{
		dCompare = Diff(tmOtherTime);
		if (dCompare == 0)
			nCompare = 0;
		else if (dCompare > 0)
			nCompare = 1;
		else
			nCompare = -1;
	}
	else if (not Check() and not tmOtherTime.Check())
		nCompare = 0;
	else if (not Check())
		nCompare = -1;
	else
		nCompare = 1;
	return nCompare;
}

inline void Time::SetHour(int nValue)
{
	require(0 <= nValue and nValue < 24);
	timeValue.fields.nHour = nValue + 1;
}

inline void Time::SetMinute(int nValue)
{
	require(0 <= nValue and nValue < 60);
	timeValue.fields.nMinute = nValue;
}

inline void Time::SetSecond(double dValue)
{
	const double dEpsilon = 0.5 / DateTime::nMaxFracSeconds;
	int nSecond;
	int nSecondFrac;
	require(0 <= dValue and dValue < 60);
	nSecond = (int)floor(dValue + dEpsilon);
	if (nSecond == 60)
		nSecond--;
	nSecondFrac = (int)floor((dValue - nSecond + dEpsilon) * DateTime::nMaxFracSeconds);
	if (nSecondFrac == DateTime::nMaxFracSeconds)
		nSecondFrac--;
	timeValue.fields.nSecond = nSecond;
	timeValue.fields.nFrac = nSecondFrac;
}

inline void Time::SetForbiddenValue()
{
	timeValue.lBytes = DateTime::lForbiddenValue;
}

inline boolean Time::IsForbiddenValue() const
{
	return (timeValue.lBytes == DateTime::lForbiddenValue);
}

// KWTimeFormat

inline boolean KWTimeFormat::Check() const
{
	return nTotalCharNumber > 0;
}

inline boolean KWTimeFormat::GetMandatoryFirstDigit() const
{
	require(Check());
	return bMandatoryFirstDigit;
}

inline int KWTimeFormat::GetTotalCharNumber() const
{
	require(Check());
	return nTotalCharNumber;
}

inline char KWTimeFormat::GetSeparatorChar() const
{
	require(Check());
	return cSeparatorChar;
}

inline int KWTimeFormat::GetSeparatorOffset1() const
{
	require(Check());
	return nSeparatorOffset1;
}

inline int KWTimeFormat::GetSeparatorOffset2() const
{
	require(Check());
	return nSeparatorOffset2;
}

inline int KWTimeFormat::GetHourOffset() const
{
	require(Check());
	return nHourOffset;
}

inline int KWTimeFormat::GetMinuteOffset() const
{
	require(Check());
	return nMinuteOffset;
}

inline int KWTimeFormat::GetSecondOffset() const
{
	require(Check());
	return nSecondOffset;
}

inline int KWTimeFormat::GetDecimalPointOffset() const
{
	require(Check());
	return nDecimalPointOffset;
}

inline int KWTimeFormat::GetMinCharNumber() const
{
	require(Check());
	return nMinCharNumber;
}

inline int KWTimeFormat::GetMaxCharNumber() const
{
	require(Check());
	return nMaxCharNumber;
}
