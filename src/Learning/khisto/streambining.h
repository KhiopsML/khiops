// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "MHStreamBiningCommandLine.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Methode de type main pour un outil de stream bining
//
// Le code de stream bining provient de l'ancien executable stream bining du projet HistogramsMODL.
// Le code est ici dans le meme repertoire que celui de l'executable khisto, pour garder les sources
// de l'algorithme, meme s'il n'est pas actuellement actif.
// Ce code, independant de khisto, est gere dans les fichiers suivants:
//  MHBinMerge
//  MHStreamBining
//  MHStreamBiningMixed
//  MHStreamBiningCommandLine
//  streambining
// Aterme, il pourrait soit faire partie d'un executable a part, soit etre integre dans l'executable
// khisto pour une option partiellement stream, permetant de controler la memoire totale necessaire
// au calcul d'histogramme en limitant le nombre de micro-bins a garder en stream, automatiquement
// (par exemple: pas plus de un million) ou selon une option utilisateur
int mainStreamBining(int argc, char** argv);