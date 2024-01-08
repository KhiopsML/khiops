// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"

///////////////////////////////////////////////////////////////////////
// PRIVE: utilise par les sources generes par GenereTable            //
// Utilitaires d'analyse de chaines de caracteres pour les operation //
// de chargement a partir de fichier                                 //
///////////////////////////////////////////////////////////////////////

// Comptage du nombre d'occurrences du caracteres separateur dans une chaine
int GetTokenSeparatorCount(const char* sTokens, char cSeparator);

// Parsing d'une chaine en token, separee par 1 et 1 seul caractere separateur
// appartenant a sSeperator
// Apres chaque appel, un caractere '\0' est positionne a la fin de
// sCurrentToken, ce qui permet de l'utiliser directement
// NextToken pointe au debut du token suivant s'il y en a un (sinon au meme
// endroit)
// Les blancs sont supprimes en debut et fin de sCurrentToken
// Fonction similaire a strtok
char* NextToken(char*& sCurrentToken, char cSeparator);

// Indique si au moins un token d'une chaine est non vide
boolean AreTokensEmpty(const char* sTokens, char cSeparator);

// Preprocessing de lecture des reels (transformation des ',' en '.')
// Renvoie la chaine de depart
char* PreprocessReal(char* sToken);
