// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////////////////////////////////
// Type longint, pour les tailles des ressources systemes (RAM, taille de fichier...)

typedef long long int longint;

// Conversion en chaine de caracteres lisible par un humain pour une quantite de bytes
const char* const LongintToHumanReadableString(longint lValue);

// Conversion en chaine de caracteres lisible, avec separateurs de milliers a partir de 10,000
const char* const LongintToReadableString(longint lValue);

// Contantes utiles pour les unite de tailles de ressources systemes
const longint lB = 1ll;
const longint lKB = 1024ll;
const longint lMB = 1048576ll;
const longint lGB = 1073741824ll;
const longint lTB = 1099511627776ll;
const longint lPB = 1125899906842624ll;
const longint lEB = 1152921504606846976ll;