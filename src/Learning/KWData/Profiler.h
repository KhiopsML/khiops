// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class Profiler;

#include "Object.h"
#include "ALString.h"
#include "JSONFile.h"
#include "FileService.h"
#include "Timer.h"

///////////////////////////////////////////////
// Collecte de stats et de trace de profiling
class Profiler : public Object
{
public:
	// Constructeur
	Profiler();
	~Profiler();

	/////////////////////////////////////////////////////////////////////
	// Gestion d'une session de profiling

	// Debut du profiling, en precisant un nom de fichier ou seront ecrites
	// les statistiques sur les traces, avec les indicateurs suivants par methode:
	// nombre d'appels, temps moyen, temps total
	void Start(const ALString& sFileName);

	// Nom du fichier de profiling
	const ALString& GetFileName() const;

	// Arret du profiling
	// Declenche l'ecriture du fichier de stats resultats et reinitialise le nom du fichier
	void Stop();

	// Indique si le profiling est en cours
	boolean IsStarted();

	// Collecte des debut et fin de methode, pour actualiser les stats de profiling
	// Methode sans effet si le profiling n'est pas demarre
	void BeginMethod(const ALString& sMethodName);
	void EndMethod(const ALString& sMethodName);

	// Ajout d'une information de type cle-valeur dans la trace, si elle est active
	void WriteKeyString(const ALString& sKey, const ALString& sValue);

	// Parametrage avance pour exporter toute la trace de profiling (defaut: false)
	// Dans ce cas, un fichier de trace complet est ecrit au format json, de nom
	// egal au fichier de stats profiling, avec un suffix ".json" additionnel
	void SetTrace(boolean bTrace);
	boolean GetTrace() const;

	//////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Nom du fichier de stats de profiling
	ALString sProfilingStatsFileName;

	// Indique si le profiling est en cours
	boolean bIsStarted;

	// Dictionnaire de timers par methode
	ObjectDictionary odMethodTimers;

	// Vecteur des noms de methode utilisees
	StringVector svMethodNames;

	// Vecteur des temps passes par les methdoes au momment de leur dernier stop
	DoubleVector dvMethodLastStopElapsedTimes;

	// Fichier json des traces de profiling
	// On utilise un pointeur pour pemettre de declarer un objet Profiler en variable statique
	// sans impact sur les objet JSONFile, qui ne doivent pas etre declares en statique
	JSONFile* fJsonTraceFile;

	// Indique si l'on exporte les traces
	boolean bIsTrace;
};
