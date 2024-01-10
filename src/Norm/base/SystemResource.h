// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Longint.h"

// Force le processus courrant a etre inactif pendant une duree en secondes
void SystemSleep(double dSeconds);

// Memoire disque utilisable sur un repertoire en octets
// Renvoie 0 si repertoire inexistant
longint DiskGetFreeSpace(const char* sPathName);

// Nombre de processeurs disponibles
int SystemGetProcessorNumber();

// Timestamp courant au format chaine de caracteres YYYY-MM-DD HH:MM:SS.mmm
// avec ou sans les millisecondes
const char* CurrentPreciseTimestamp();
const char* CurrentTimestamp();

//////////////////////////////////////////////////////////////////////////////
// Acces aux statistiques sur la memoire physique (RAM) utilisable, en octets

// Memoire physique theorique adressable, avec legere marge de securite en 32 bits
longint MemGetAdressablePhysicalMemory();

// Memoire physique totale
longint MemGetAvailablePhysicalMemory();

// Memoire physique reservee, non utilisable pour les applications
longint MemGetPhysicalMemoryReserve();

// Memoire physique disponible (en tenant compte de l'occupation memoire des autres applications)
longint MemGetFreePhysicalMemory();

// Memoire virtuelle utilisee par le process courant
longint MemGetCurrentProcessVirtualMemory();

//////////////////////////////////////////////////////////////////////////////
// Divers

// Acces a l'adresse MAC au format XX-XX-XX-XX-XX-XX
// Renvoie toujours une chaine de caractere valide (pas NULL)
// eventuellement initialise avec 00-00-00-00-00-00 si non trouvee
const char* GetMACAddress();

// Acces au numero de serie
// Utile notament sur un telephone portable, sans effet sur un PC ("")
// Renvoie toujours une chaine de caractere valide (pas NULL)
// eventuellement vide si non trouve
const char* GetSerialNumber();

// Acces au nom de la machine
const char* GetLocalHostName();

// Identifiant unique de la machine
const char* GetMachineGUID();

// Nombre maximum de fichier ouverts en meme temps par un processus
int GetMaxOpenedFileNumber();
