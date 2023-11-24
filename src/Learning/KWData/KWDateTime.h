// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"

///////////////////////////////////////////////////////////////////////
// Type DateTime
// Type interne, uniquement pour l'implementation des classes
// Date, Time, Timestamp et TimestampTZ
// Pas de constructeur pour pouvoir etre utilise dans l'union de KWValue
union DateTime
{
public:
	// Test des fonctionnalites
	static void Test();

	//////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Classes ayant besoin de connaitre l'implementation en friend
	friend class Date;
	friend class Time;
	friend class Timestamp;
	friend class TimestampTZ;
	friend class KWTimeFormat;

	// Affichage detaille de tous les champs
	void WriteInternalFields(ostream& ost) const;

	//////////////////////////////////////////////////////////////////////////
	// Acces aux champs
	// Acces direct depuis les champs de bit fields, sauf pour la timezone
	// qui est a cheval sur les deux int occupes par la structure (taille de KWValue)
	// Des accesseur dedies sont prevus pour gerer certains champ de bit fields qui sont
	// a cheval sur les deux entier

	// Acces a la timezone
	// (coupe en deux dans les bit fields pour des raison d'alignement avec des int)
	void SetTimeZone(int nValue);
	int GetTimeZone() const;

	// Acces aux minutes de la timezone
	// (coupe en deux dans les bit fields pour des raison d'alignement avec des int)
	void SetTimeZoneMinute(int nValue);
	int GetTimeZoneMinute() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Stockage optimise d'une (Date, Time) sous forme d'un entier long
	// On utilise un champ de bits pour stocker a la fois les information de Date et de time zone, utilises
	// potentiellement dans les cas des TimestampTZ
	// Utilisation systematique de 0 pour gerer la notion de valeur invalide
	// - Date invalide si 0 (par exemple, le mois ne peut pas etre 0)
	//   - Date de base: idem
	//   - timezone si 0 (une time zone -00:00 est codee +00:00 pour etre valide).
	// - Time invalide si 0 (heure a 0; validite de 1 a 24), et interdite si elle vaut 0xFFFFFFFF
	//
	// Attention, il est critique que la taile complete de la structure soit 8 bytes, comme pour les KWValue
	// Le packing des structures de type bit field pour certains compilateurs ne fonctionne correctement
	// que pour des sous parties qui sont des multiples de la taille d'un int de 32 bits (cas de Visual C++)
	// Cf. C99 standard 6.7.2.1/10 (c'est meme peu portable: l'ordre des bits peut dependre de l'endianess...)
	// C'est pourquoi la timezone etant a cheval sur les deux int, les bits de la partie minute
	// de la timezone sont coupes en deux

	// Pour manipuler tous les champs en une seule operation
	longint lBytes;

	// Pour manipuler les champs de detail individuellement
	struct DateTimeFields
	{
		unsigned int nYear : 14;           // annee (entre 0 et 9999)
		unsigned int nMonth : 4;           // mois (entre 1 et 12)
		unsigned int nDay : 5;             // jour (entre 1 et 31, ou 0 si invalide)
		unsigned int nTimeZoneSign : 1;    // signe de la timezone (0 si negatif, 1 si positif)
		unsigned int nTimeZoneHour : 4;    // heure de la timezone (entre 0 et 14)
		unsigned int nTimeZoneMinute1 : 4; // minute de la timezone (entre 0 et 59), en deux parties
		unsigned int nTimeZoneMinute2 : 2; //
		unsigned int nHour : 5;            // heure de 0 a 23, plus 1
		unsigned int nMinute : 6;          // minutes de 0 a 59
		unsigned int nSecond : 6;          // secondes de 0 a 59
		unsigned int nFrac : 13;           // partie decimale des secondes; de 0 a 999 (en 1/1000 de seconde)
	} fields;

	// Pour manipuler d'un coup tous les champs de Date ou de Time, et de Timezone le plus simplement
	struct DateTimeParts
	{
		unsigned int nDate : 23;     // sous-partie des champs Date de base
		unsigned int nTimezone1 : 9; // sous-partie des champs Date de timezone, en deux parties
		unsigned int nTimezone2 : 2; //
		unsigned int nTime : 30;     // sous-partie des champs Time
	} parts;

	// Valeur max des annees
	static const int nMaxYear = 9999;

	// Nombre de digits utilises pour les fraction de secondes
	static const int nFracSecondsDigitNumber = 3;

	// Valeur max des fractions de secondes, en puissance de 10
	static const int nMaxFracSeconds = 1000;

	// Constante utilisee pour gerer la valeur interdite de tous les types date time
	static const longint lForbiddenValue = 0xFFFFFFFFFFFFFFFF;
};

////////////////////////////////////////////////

inline void DateTime::SetTimeZone(int nValue)
{
	require(0 <= nValue);
	parts.nTimezone1 = nValue / 4;
	parts.nTimezone2 = nValue % 4;
}

inline int DateTime::GetTimeZone() const
{
	return 4 * parts.nTimezone1 + parts.nTimezone2;
}

inline void DateTime::SetTimeZoneMinute(int nValue)
{
	require(0 <= nValue);
	fields.nTimeZoneMinute1 = nValue / 4;
	fields.nTimeZoneMinute2 = nValue % 4;
}

inline int DateTime::GetTimeZoneMinute() const
{
	return 4 * fields.nTimeZoneMinute1 + fields.nTimeZoneMinute2;
}
