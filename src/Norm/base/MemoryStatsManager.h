// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "MemoryManager.h"
#include "Longint.h"
#include "Timer.h"

class MemoryStats;

//////////////////////////////////////////////////////////
// Classe MemoryStatsManager
// Collecte de statistiques pour auditer l'utilisation de la memoire
class MemoryStatsManager : public Object
{
public:
	// Ouverture d'un fichier de log de stats memoire
	// En cas de processus parallele, le fichier du master sera nomme selon son parametrage, alors
	// que ceux des esclaves auront un suffixe lie a leur ProcessId
	//
	// Chaque log contient des statistiques sur la heap et sur les allocations plus eventuellement un libelle.
	// Un log peut etre produit explicitement par l'utilisateur ou automatiquement
	// par l'allocateur, sans libelle (via le parametrage de son handler de stats)
	//
	// Si nLogFrequency > 0, des logs sont generes automatiquement par l'allocateur selon la frequence souhaitee,
	// en plus des logs utilisateur.
	//
	// Le parametre StatsToCollect permet de specifier quellles stats sont a ecrire dans le fichier de log.
	// Voir le parametrage ci-dessous. Par exemple, pour collecter toutes les statistiques, il suffit
	// d'utiliser la constante MemoryStatsManager::AllStats.
	//
	// Note: les statistiques sur les allocations peuvent parfois sembler legerement incoherentes.
	// Cela provient du fait qu'il peut y avoir des desallocations de pointeur ayant ete
	// alloues avant la mise en place du handler.
	// Par contre, le handler utilise dans cette classe ne consomme pas de memoire
	// et sa frequence de log n'a pas d'impacts sur les stats
	static boolean OpenLogFile(const char* sFileName, longint lLogFrequency, int nStatsToCollect);

	// Ouverture d'un fichier de log a partir deparametres issus de variables d'environnement:
	//   KhiopsMemStatsLogFileName, KhiopsMemStatsLogFrequency, KhiopsMemStatsLogToCollect
	// On ne tente d'ouvrir le fichier que si ces trois variables sont presentes et valides
	// Sinon, on ne fait rien, sans message d'erreur
	// En mode verbeux, on affiche un message dans la console, uniquement si la collecte des stats est active
	//
	// Par exemple, pour avoir toutes les stats, il faut utiliser AllStats, c'est a dire
	// KhiopsMemStatsLogToCollect=16383 Si on veut avoir les stats minimalistes sans overhead, il faut se
	// restreindre au stats sur la heap avec frequence nulle de collecte,  c'est a dire KhiopsMemStatsLogFrequency=0
	// et KhiopsMemStatsLogToCollect=8207
	static boolean OpenLogFileFromEnvVars(boolean bVerbose);

	// Indique si la collecte des stats est en cours
	static boolean IsOpened();

	// Fermeture du fichier de log
	// Methode toujours appelable, mais sans effet si le fichier de log n'est pas ouvert
	// La methode est appelee systematiquement apres la fin du programme, si le fichier n'est pas deja ferme
	static boolean CloseLogFile();

	// Nom du fichier de log ouvert, vide sinon
	static const ALString& GetLogFileName();

	// Frequence de collecte pour le fichier ouvert, 0 sinon
	static longint GetAutomaticLogFrequency();

	// Flag indiquant quelles sont les stats collectee pour le fichier ouvert, NoStats sinon
	static int GetCollectedStats();

	// Ajout d'un ligne de log utilisateur
	// Methode toujours appelable, mais sans effet si le fichier de log n'est pas ouvert
	static void AddLog(const char* sLabel);

	//////////////////////////////////////////////////////////////////////////////////
	// Parametrage des statistiques a collecter
	// Il s'agit d'un ensemble de constantes, que l'on peut sommer pour avoir acces
	// a plusieurs statistiques simultanement

	// Temps en milisecondes depuis de le debut de la collecte
	static const int LogTime = 1 << 0; // temps associe au log

	// Statistiques sur la heap
	static const int HeapMemory = 1 << 1;               // taille courante de la heap
	static const int MaxHeapRequestedMemory = 1 << 2;   // taille maximale demandee pour la heap
	static const int TotalHeapRequestedMemory = 1 << 3; // taille total cumulee demande pour la heap

	// Statistiques sur les allocations
	static const int AllocNumber = 1 << 4;         // nombre courant d'allocations
	static const int MaxAllocNumber = 1 << 5;      // nombre maximale d'allocations
	static const int TotalAllocNumber = 1 << 6;    // nombre total d'allocations
	static const int TotalFreeNumber = 1 << 7;     // nombre total de liberations
	static const int GrantedSize = 1 << 8;         // taille courant alouee
	static const int MaxGrantedSize = 1 << 9;      // taille maximale alouee
	static const int TotalRequestedSize = 1 << 10; // taille totale demandee
	static const int TotalGrantedSize = 1 << 11;   // taille totale alouee
	static const int TotalFreeSize = 1 << 12;      // taille totale liberee

	// Libelle des logs utilisateur
	static const int LogLabel = 1 << 13; // libelle utilisateur

	// Aucune ou toutes les statistiques
	static const int NoStats = 0;              // aucune statistique
	static const int AllStats = (1 << 14) - 1; // toutes les statistiques

	// Quelques combinaisons portentiellement utiles
	static const int LogInfo = LogTime + LogLabel; // infos par log
	static const int HeapStats =
	    HeapMemory + MaxHeapRequestedMemory + TotalHeapRequestedMemory; // stats sur la heap
	static const int AllocStats = AllStats - LogInfo - HeapStats;       // stats sur les allocations
	static const int AllocCurrentStats = AllocNumber + GrantedSize;     // stats sur les allocations courantes
	static const int AllocMaxStats = MaxAllocNumber + MaxGrantedSize;   // stats sur les max d'allocations
	static const int AllocNumberStats =
	    AllocNumber + MaxAllocNumber + TotalAllocNumber + TotalFreeNumber; // stats sur les nombres d'allocations
	static const int AllocSizeStats = GrantedSize + MaxGrantedSize + TotalRequestedSize + TotalGrantedSize +
					  TotalFreeSize; // stats sur les tailles d'allocations

	///////////////////////////////////////////////////////////
	///// Implementation
protected:
	////////////////////////////////////////////////////////////////
	// L'overhead de l'utilisation du handler via cette classe est:
	//  . en memoire: nul, aucune allocation
	//  . en temps:
	//      . nul si frequence d'allocation a 0 et si
	//        si uniquement stats de heap collectees
	//      . negligeable si uniquement stats d'allocation collectees
	//      . negligeable a majeure si beaucoups de stats collectee et
	//        frequence de collecte elevee

	// Ecriture dans un fichier en ecrivant soit une fin de ligne ou une fin de champ en fonction de l'index du
	// champ
	static void WriteString(const char* sValue);
	static void WriteLongint(longint lValue);
	static void WriteTime(double dValue);

	// Parametrage utilisateur du handler
	static boolean bIsOpened;
	static ALString sStatsLogFileName;
	static ALString sLocalFileName;
	static longint lAutomaticLogFrequency;
	static int nCollectedStats;

	// Gestion du fichier de log
	// Utilisation de FILE plutot que de stream, pour diminuer l'empreinte memoire
	// On ne peut pas non plus utiliser un OutputBufferedFile, car cela entraine
	// un boucle infinie quand le OutputBufferedFile qui emet des logs memoires
	static Timer timer;
	static int nStatsFieldNumber;
	static int nStatsFieldIndex;
	static FILE* fMemoryStats;
};

inline boolean MemoryStatsManager::IsOpened()
{
	return bIsOpened;
}