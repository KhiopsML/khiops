// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWTypeAutomaticRecognition;
class KWTypeAvailableFormats;

#include "KWType.h"
#include "Vector.h"
#include "InputBufferedFile.h"

////////////////////////////////////////////////////////////////////
// Reconnaissance automatique de type a partir d'exemples de valeur
// sous forme chaines de caracteres
class KWTypeAutomaticRecognition : public Object
{
public:
	// Constructeur
	KWTypeAutomaticRecognition();
	~KWTypeAutomaticRecognition();

	////////////////////////////////////////////////////////////////////////
	// Reconnaissance automatique de types
	// Suite a une initialisation, on fournit des exemples de valeurs.
	// A chaque valeur fournie, des conversions vers un des types (Stored) de KWType
	// sont effectuer, afin de ne garder que les format de conversion compatibles
	// les exemples de valeurs.
	// A tout moment, on peut demander des infos sur les types et les formats de
	// conversion compatibles avec les valeurs fournies.

	// Initialisation en passant en parametres une liste de formats date, time et timestamps
	// Les tableaux en parametre appartiennent a l'appelant
	void Initialize();

	// Test si initialise
	boolean IsInitialized() const;

	// Ajout d'une valeur a analyser
	void AddStringValue(const char* const sValue);

	// Nombre de valeurs analysees
	int GetValueNumber() const;

	// Nombre de valeurs vides rencontrees
	int GetMissingValueNumber() const;

	// Nombre de valeurs "0" ou "1" rencontrees
	int GetValue0Number() const;
	int GetValue1Number() const;

	// Longueur max des valeurs analysees
	int GetMaxValueLength() const;

	// Finalisation de la reconnaissance des types
	// A appeler apres l'ajout de la derniere valeur a analyser
	void Finalize();

	// Test si finalise
	boolean IsFinalized() const;

	// Type principal compatible avec les valeurs analyses
	// Au minimum, Symbol est compatible avec n'importe quelle valeur
	// S'il n'y a que des valeurs manquantes, seul le type Symbol est retenu
	// Les Date, Type et Timestamp sont exclusifs
	// Par contre, il peut y avoir confusion entre un type complexe sans separateur
	// (par exemple: YYMMDD, HHMMSS.) et le type numerique: dans ce cas, c'est
	// le type numerique qui est prioritaire, suivi dub type complexe, et enfin symbol.
	int GetMainMatchingType() const;

	// Liste des types compatibles avec les valeurs analysees
	// par priorite decroissante
	int GetMatchingTypeNumber() const;
	int GetMatchingTypeAt(int nIndex) const;

	// Test si le type numerique est compatible
	boolean IsContinuousTypeMatching() const;

	/////////////////////////////////////////////////////////////////
	// Formats compatibles dans le cas d'un type complexe reconnu
	// En cas de plusieurs formats compatibles avec les valeurs, le premier est prefere
	//   - les formats avec et sans separateur decimaux peuvent etre
	//     simultanement compatibles et coherents
	//   - les variantes de format de Time avec ou sans premier chiffre
	//     obligatoire peuvent etre compatibles et coherents
	//   - par contre, il peut y avoir plusieurs formats Date (ou partie Date
	//     de Timestamp)  compatibles avec les valeurs, mais incoherentes:
	//     on doit tester le probleme et le signaler (ex: 20121010 est compatible
	//     avec YYYYMMDD, YYYYDDMM, DDMMYYY, mais le choix du format n'est pas indifferent
	//     pour le codage de toute dae potentielle)

	// Formats de Date compatibles avec les valeurs analysees
	int GetMatchingDateFormatNumber() const;
	const KWDateFormat* GetMatchingDateFormatAt(int nIndex) const;

	// Formats de Time compatibles avec les valeurs analysees
	int GetMatchingTimeFormatNumber() const;
	const KWTimeFormat* GetMatchingTimeFormatAt(int nIndex) const;

	// Formats de Timestamp compatibles avec les valeurs analysees
	int GetMatchingTimestampFormatNumber() const;
	const KWTimestampFormat* GetMatchingTimestampFormatAt(int nIndex) const;

	// Test si tous les formats sont coherents avec le premier de la liste
	boolean AreMatchingDateFormatConsistent() const;
	boolean AreMatchingTimeFormatConsistent() const;
	boolean AreMatchingTimestampFormatConsistent() const;

	// Affichage des resultats de reconnaissance
	void Write(ostream& ost) const override;
	void WriteHeaderLine(ostream& ost) const;
	void WriteLine(ostream& ost) const;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ajout de la premiere valeur non vide a analyser
	void AddFirstStringValue(const char* const sValue, int nValueLength);

	// Ajout d'une nouvelle valeur non vide a analyser
	void AddNewStringValue(const char* const sValue, int nValueLength);

	// Nombre de valeurs et valeurs vides analysees
	int nValueNumber;
	int nMissingValueNumber;

	// Nombre de valeurs 0 ou 1 analysees
	int nValue0Number;
	int nValue1Number;

	// Longueur de la valeur la plus longue
	int nMaxValueLength;

	// Nombre de types compatibles avec les valeurs analysees, sans compter le type Text,
	// gere au premier abord comme le type Symbol
	int nMatchingTypeNumber;

	// Valeur la plus petite du permier caractere
	// Sert a verifier si un format HHMM correspond en fait a une annee (de type 1999, 2000)
	char cMinFirstChar;

	// Indicateur de compatibilite du type Continuous
	boolean bMatchingContinuous;

	// Indicateur de compatibilite du type Text
	boolean bMatchingText;

	// Indicateur de finalisation
	boolean bIsFinalized;

	// Tableau des formats compatibles avec les valeurs analysees
	ObjectArray oaMatchingDateFormats;
	ObjectArray oaMatchingTimeFormats;
	ObjectArray oaMatchingTimestampFormats;

	// Taille minimum des champs de type Text
	// Une variable de type Symbol comportant au moins une valeur de taille superieure sera consideree comme de type
	// Text
	static const unsigned int nMinTextFieldSize = 100;

	// Taille maximum des champs de type Continuous
	// On se limite a une taille plus petit que la taille max des champs de tous types pour
	// pouvoir gerer des variables de travail dans la stack sans depassement memoire de celle-ci
	static const unsigned int nMaxContinuousFieldSize = 1000;

	// Formats de reference par type
	// Donnees partagees par l'ensemble des instances de la classe
	static const KWTypeAvailableFormats* allReferenceFormats;

	// Mise en place d'un pattern singleton pour l'ensemble des formats de reference.
	// Le compteur d'instance permet d'allouer les donnees partagees avec la creation de la
	// premiere instance, et de les desallouer avec la destruction de la derniere instance
	static int nInstanceNumber;
};

////////////////////////////////////////////////////////////////////
// Ensemble des formats disponibles par type
// Cetet classe sert a partager les informations de format
// entre tous les objets KWTypeAutomaticRecognition
class KWTypeAvailableFormats : public Object
{
public:
	// Constructeur
	KWTypeAvailableFormats();
	~KWTypeAvailableFormats();

	// Formats de Date disponibles
	int GetAvailableDateFormatNumber() const;
	const KWDateFormat* GetAvailableDateFormatAt(int nIndex) const;

	// Formats de Time disponibles
	int GetAvailableTimeFormatNumber() const;
	const KWTimeFormat* GetAvailableTimeFormatAt(int nIndex) const;

	// Formats de Timestamp disponibles
	int GetAvailableTimestampFormatNumber() const;
	const KWTimestampFormat* GetAvailableTimestampFormatAt(int nIndex) const;

	////////////////////////////////////////////////////////////////
	// Statistiques sur les formats

	// Tailles minimum et maximum d'une valeur pour etre compatible avec un des formats Date
	int GetDateMinCharNumber() const;
	int GetDateMaxCharNumber() const;

	// Tailles minimum et maximum d'une valeur pour etre compatible avec un des formats Time
	// en ignorant la partie decimale des secondes
	int GetTimeMinCharNumber() const;
	int GetTimeMaxCharNumber() const;

	// Tailles minimum et maximum d'une valeur pour etre compatible avec un des formats Timestamp
	// en ignorant la partie decimale des secondes
	int GetTimestampMinCharNumber() const;
	int GetTimestampMaxCharNumber() const;

	// Test si un charactere peut etre utilise comme separateur
	boolean IsDateSeparator(char cValue) const;
	boolean IsTimeSeparator(char cValue) const;
	boolean IsTimestampSeparator(char cValue) const;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////
	//// Implementation
protected:
	// Statistiques sur les formats
	int nDateMinCharNumber;
	int nDateMaxCharNumber;
	int nTimeMinCharNumber;
	int nTimeMaxCharNumber;
	int nTimestampMinCharNumber;
	int nTimestampMaxCharNumber;
	ALString sDateSeparators;
	ALString sTimeSeparators;
	ALString sTimestampSeparators;

	// Tableau des formats de reference
	ObjectArray oaAvailableDateFormats;
	ObjectArray oaAvailableTimeFormats;
	ObjectArray oaAvailableTimestampFormats;
};
