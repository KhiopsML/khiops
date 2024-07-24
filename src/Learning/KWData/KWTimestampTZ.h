// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class TimestampTZ;
class KWTimestampTZFormat;

#include "Object.h"
#include "Standard.h"
#include "KWTimestamp.h"

////////////////////////////////////////////////////////////////////////////////////
// Definition du type TimestampTZ et des services attaches
// Le type TimestampTZ est traite comme un type elementaire manipulable comme une valeur
// (comme un entier, une chaine de caractere).
// Il n'herite pas de la classe Object et ne peut etre utilise
// directement dans un container

///////////////////////////////////////////////////////////////////////
// Classe TimestampTZ
// Prefixe pour les variables: tstz
// Pas de constructeur pour pouvoir etre utilise dans l'union de KWValue
//
// Les TimestampTZ peuvent comporter ou non information de type time zone (time zone aware ou unaware)
class TimestampTZ : public SystemObject
{
public:
	// Remise a zero (0: TimestampTZ invalide)
	void Reset();

	// Initialisation
	// Renvoie true si l'initialisation est valide
	boolean Init(Timestamp tsValue, int nTimeZoneTotalMinutes);

	// Operateurs de comparaison
	boolean operator==(const TimestampTZ& tstzValue) const;
	boolean operator!=(const TimestampTZ& tstzValue) const;

	// Test si valide
	boolean Check() const;

	//////////////////////////////////////////////////////
	// Gestion des informations de time zone
	// Un TimestampTZ contient des informations de time zone, entre -12:00 et +14:00.
	// - Les valeurs et les formats de TimestampTZ doivent etre coherentes avec les informations relatives la time
	// zone
	//   lors des operations de lecture et d'ecriture dans la base de donnees.
	//   Par exemple, dans le cas d'un format de TimestampTZ tenant compte de la time zone (ex :
	//   "YYYY-MM-DDTHH:MM:SSzzzzz"), les valeurs de TimestampTZ sans information sur le fuseau horaire seront
	//   traitees comme des valeurs manquantes.
	// - Les valeurs de TimestampTZ avec time zone peuvent etre transformees en valeurs de Timestamp sans time zone,
	//    en utilisant les regles UtcTimestamp ou LocalTimestamp.
	// - Les valeurs de Timestamp sans time zone peuvent etre transformees en valeurs de TimestampTZ avec time zone
	//   a l'aide de la regle SetTimeZoneMinutes.
	// - Les valeurs de TimestampTZ avec time zone doivent d'abord etre transformees en un Timestamp local ou UTC,
	//   pour extraire des informations avec les regles GetDate, GetTime, DecimalYearTS et AbsoluteSecond.
	// - La regle DiffTimestampTZ doit etre utilisee avec deux valeurs de TimestampTZ, toutes deux avec zone

	// Reinitialisation de la time zone
	void ResetTimeZone();

	// Initialisation de la partie time zone d'un TimestampTZ valide par son nombre total de minutes
	// Nombre total de minutes de la time zone (+- (hh*60 + mm)
	// Renvoie true si l'initialisation est valide (entre -12:00 et +14:00)
	boolean SetTimeZoneTotalMinutes(int nValue);
	int GetTimeZoneTotalMinutes() const;

	// Test si la time zone est valide
	boolean CheckTimeZone() const;

	// Modification du timestamp local, sans changer la time zone
	void SetLocalTimestamp(const Timestamp timestamp);

	// Conversion vers un Timestamp local, en utilisant la time zone locale et en supprimant les informations de
	// time zone
	Timestamp GetLocalTimestamp() const;

	// Conversion vers un Timestamp UTC, en utilisant la time zone UTC et en supprimant les informations de time
	// zone
	Timestamp GetUtcTimestamp() const;

	////////////////////////////////////////////////
	// Servives avances
	// Le TimestampTZ doit etre valide

	// Difference avec une autre TimestampTZ en secondes
	// Les TimestampTZs doivent conjointement etre soit aware, soit unware
	double Diff(const TimestampTZ tstzOtherTimestampTZ) const;

	// Comparaison avec un autre TimestampTZ
	// Un TimestampTZ invalide est inferieur a tous les autres
	// Les TimestampTZs doivent conjointement etre soit aware, soit unware
	int Compare(const TimestampTZ tstzOtherTimestampTZ) const;

	// Ajout d'un nombre de secondes a un TimestampTZ
	// Renvoie true si on obtient une TimestampTZ valide
	boolean AddSeconds(double dSeconds);

	// Affichage au format par defaut YYYY-MM-DD HH:MM:SS.
	const char* const ToString() const;
	void Write(ostream& ost) const;

	// Valeur interdite, ne pouvant etre prise par aucune TimestampTZ valide ou non
	// Utile pour la gestion avancee des KWType
	void SetForbiddenValue();
	boolean IsForbiddenValue() const;

	// Test des fonctionnalites
	static void Test();

	//////////////////////////////////////////////////////
	//// Implementation
protected:
	// Class de format en friend pour acceder aux informations interne de Date et Time,
	// meme en presence de timezone
	friend class KWTimestampTZFormat;

	// Acces aux attributs en mode mise a jour
	// L'acces par reference permet de les utiliser en affectation
	// A noter que le format Date est utiliser pour gerer les information de time zone si elles sont presentes
	// Les type Date et Time sont uilises au maximum pour stocker les information de date et time zone (Date)
	// et de time, potentiellement jusqu'au 1/10000 de seconde (Time)
	//
	// Exemple: GetInternalTimestamp() = myTimestamp, GetInternalDate() = myDate, ou GetInternalDate().Reset()
	Timestamp& GetInternalTimestamp() const;
	Date& GetInternalDate() const;
	Time& GetInternalTime() const;

	// Utilisation d'une union DateTime pour acceder au champs Timestamp, dans une structure commune aux type temporels
	union DateTime timestampTZValue;
};

// Ecriture dans un stream
inline ostream& operator<<(ostream& ost, const TimestampTZ& value)
{
	value.Write(ost);
	return ost;
}

///////////////////////////////////////////////////////////////////////
// Classe KWTimestampTZFormat
// Gestion du format d'entre sortie d'une TimestampTZ
// Un format de TimestampTZ est base sur un format de Timestamp suivi d'une partie de type time zone,
// en utilisant un format ISO 8601:
//    <Timestamp format>zzzzz: basic time zone format (Z or +hhmm or -hhmm)
//    <Timestamp format>zzzzzz: extended time zone format (Z or +hh:mm or -hh:mm)
class KWTimestampTZFormat : public Object
{
public:
	// Constructeur
	KWTimestampTZFormat();
	~KWTimestampTZFormat();

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
	boolean IsConsistentWith(const KWTimestampTZFormat* otherFormat) const;

	//////////////////////////////////////////////////////////
	// Caracteristique d'un format valide

	// Types de format de type zone
	enum
	{
		NoTimeZone,      // Pas de time zone valide
		BasicTimeZone,   // Time zone au format basic xxxxx (Z ou +hhmm ou -hhmm)
		ExtendedTimeZone // Time zone au format basic xxxxxx (Z ou +hh:mm ou -hh:mm)
	};

	// Services de gestion de la taille de la partie dediee au time zone
	static int GetTimeZoneMaxLength(int nTimeZoneFormat);
	static const int nBasicTimeZoneMaxLength = 5;
	static const int nExtendedTimeZoneMaxLength = 6;

	// Format de la partie Timestamp du TimestampTZ
	const KWTimestampFormat* GetTimestampFormat() const;

	// Format de la partie time zone du TimestampTZ
	int GetTimeZoneFormat() const;

	// Nombre total de caracteres necessaire a l'encodage d'un TimestampTZ
	int GetTotalCharNumber() const;

	// Tailles minimum et maximum d'une valeur pour etre compatible avec le format
	// en ignorant la partie decimale des secondes
	int GetMinCharNumber() const;
	int GetMaxCharNumber() const;

	//////////////////////////////////////////////////////////
	// Methodes de conversion

	// Renvoie une TimestampTZ a partir d'une chaine de caracteres
	// La TimestampTZ est invalide en cas de probleme de conversion
	TimestampTZ StringToTimestampTZ(const char* const sValue) const;

	// Renvoie une chaine a partir d'une TimestampTZ
	// Renvoie la chaine vide si la TimestampTZ est invalide
	const char* const TimestampTZToString(TimestampTZ tstzValue) const;

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
	static void GetAllAvailableFormats(ObjectArray* oaAvailableTimestampTZFormats);

	// Test des fonctionnalites
	static void UnitTest(const ALString& sInputValue, KWTimestampTZFormat* inputFormat,
			     KWTimestampTZFormat* outputFormat);
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ajout d'un format dans un tableau
	// La methode construit et initialise un objet KWTimestampTZFormat et l'ajoute en fin de tableau
	static void AddFormat(ObjectArray* oaFormats, const ALString& sFormatString);

	// Donnees de format
	ALString sFormatString;
	KWTimestampFormat timestampFormat;
	int nTimeZoneFormat;
	int nTimeZoneFormatLength;
};

// Methodes en inline

inline void TimestampTZ::Reset()
{
	timestampTZValue.lBytes = 0;
}

inline boolean TimestampTZ::operator==(const TimestampTZ& tstzValue) const
{
	return (timestampTZValue.lBytes == tstzValue.timestampTZValue.lBytes);
}

inline boolean TimestampTZ::operator!=(const TimestampTZ& tstzValue) const
{
	return (timestampTZValue.lBytes != tstzValue.timestampTZValue.lBytes);
}

inline boolean TimestampTZ::Check() const
{
	require(not IsForbiddenValue());
	return timestampTZValue.lBytes != 0;
}

inline void TimestampTZ::ResetTimeZone()
{
	GetInternalDate().ResetTimeZone();
}

inline boolean TimestampTZ::SetTimeZoneTotalMinutes(int nValue)
{
	require(Check());
	return GetInternalDate().SetTimeZoneTotalMinutes(nValue);
}

inline int TimestampTZ::GetTimeZoneTotalMinutes() const
{
	return GetInternalDate().GetTimeZoneTotalMinutes();
}

inline boolean TimestampTZ::CheckTimeZone() const
{
	return GetInternalDate().CheckTimeZone();
}

inline void TimestampTZ::SetLocalTimestamp(const Timestamp timestamp)
{
	int nTimeZoneTotalMinutes;

	// On recupere la time zone en cours
	if (CheckTimeZone())
		nTimeZoneTotalMinutes = GetTimeZoneTotalMinutes();
	else
		nTimeZoneTotalMinutes = 0;

	// On modifie le timestamp
	GetInternalTimestamp() = timestamp;

	// On remet la time zone en cours
	if (nTimeZoneTotalMinutes != 0 and timestamp.Check())
		SetTimeZoneTotalMinutes(nTimeZoneTotalMinutes);
}

inline Timestamp TimestampTZ::GetLocalTimestamp() const
{
	Timestamp tsResult;

	require(Check());
	require(CheckTimeZone());

	// Suppression de la time zone la la partie date
	tsResult = GetInternalTimestamp();
	tsResult.GetInternalDate().ResetTimeZone();
	return tsResult;
}

inline Timestamp TimestampTZ::GetUtcTimestamp() const
{
	Timestamp tsResult;

	require(Check());
	require(CheckTimeZone());

	// Suppression de la time zone la la partie date
	tsResult = GetInternalTimestamp();
	tsResult.GetInternalDate().ResetTimeZone();

	// Supression du decallage lie a la time zone
	tsResult.AddSeconds(-60 * GetTimeZoneTotalMinutes());
	return tsResult;
}

inline double TimestampTZ::Diff(const TimestampTZ tstzOtherTimestampTZ) const
{
	double dDiff;
	require(Check() and tstzOtherTimestampTZ.Check());

	// Difference des Timestamps
	dDiff = GetInternalTimestamp().Diff(tstzOtherTimestampTZ.GetInternalTimestamp());

	// Ajout des differences de time zone
	dDiff += (GetTimeZoneTotalMinutes() - tstzOtherTimestampTZ.GetTimeZoneTotalMinutes()) * 60;
	return dDiff;
}

inline int TimestampTZ::Compare(const TimestampTZ tsOtherTimestampTZ) const
{
	double dCompare;
	int nCompare;

	require(not IsForbiddenValue() and not tsOtherTimestampTZ.IsForbiddenValue());

	if (Check() and tsOtherTimestampTZ.Check())
	{
		dCompare = Diff(tsOtherTimestampTZ);
		if (dCompare == 0)
			nCompare = 0;
		else if (dCompare > 0)
			nCompare = 1;
		else
			nCompare = -1;
	}
	else if (not Check() and not tsOtherTimestampTZ.Check())
		nCompare = 0;
	else if (not Check())
		nCompare = -1;
	else
		nCompare = 1;
	return nCompare;
}

inline void TimestampTZ::SetForbiddenValue()
{
	timestampTZValue.lBytes = DateTime::lForbiddenValue;
}

inline boolean TimestampTZ::IsForbiddenValue() const
{
	return (timestampTZValue.lBytes == DateTime::lForbiddenValue);
}

inline Timestamp& TimestampTZ::GetInternalTimestamp() const
{
	return (Timestamp&)timestampTZValue;
}

inline Date& TimestampTZ::GetInternalDate() const
{
	return ((Timestamp&)timestampTZValue).GetInternalDate();
}

inline Time& TimestampTZ::GetInternalTime() const
{
	return ((Timestamp&)timestampTZValue).GetInternalTime();
}

// KWTimestampTZFormat

inline boolean KWTimestampTZFormat::Check() const
{
	return nTimeZoneFormatLength > 0;
}

inline int KWTimestampTZFormat::GetTimeZoneMaxLength(int nTimeZoneFormat)
{
	require(NoTimeZone <= nTimeZoneFormat and nTimeZoneFormat <= ExtendedTimeZone);
	if (nTimeZoneFormat == NoTimeZone)
		return 0;
	else if (nTimeZoneFormat == BasicTimeZone)
		return nBasicTimeZoneMaxLength;
	else
		return nExtendedTimeZoneMaxLength;
}

inline const KWTimestampFormat* KWTimestampTZFormat::GetTimestampFormat() const
{
	return &timestampFormat;
}

inline int KWTimestampTZFormat::GetTimeZoneFormat() const
{
	return nTimeZoneFormat;
}

inline int KWTimestampTZFormat::GetTotalCharNumber() const
{
	require(Check());
	return timestampFormat.GetTotalCharNumber() + nTimeZoneFormatLength;
}

inline int KWTimestampTZFormat::GetMinCharNumber() const
{
	require(Check());
	return timestampFormat.GetMinCharNumber() + 1;
}

inline int KWTimestampTZFormat::GetMaxCharNumber() const
{
	require(Check());
	return timestampFormat.GetMaxCharNumber() + nTimeZoneFormatLength;
}
