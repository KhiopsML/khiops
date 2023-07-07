// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Timer.h"

// CH IV Refactoring: nettoyer lignes suivantes? (les define ne semble plus utilises)
// CH IV Refactoring: voir supprimer les fichiers CCConfig? (qui semblent ne plus etre utiles)
#define CC_BUILD_TIMER
// pre-condition
#undef StartCCTimer
#ifndef CC_BUILD_TIMER
#define StartCCTimer(ignore) ((void)0)
#else
#define StartCCTimer(p) ((&p) ? (void)0 : #p##".Start()")
#endif
#undef StopCCTimer
#ifndef CC_BUILD_TIMER
#define StopCCTimer(ignore) ((void)0)
#else
#define StopCCTimer(p) ((&p) ? (void)0 : #p##".Stop()")
#endif

// #define MAX_VARIABLE_2_BUILTREE_BORNE 100

#ifdef CC_BUILD_TIMER
// Timer pour mesurer le temps d'optimisation d'une grille individus * variables
extern Timer CCTimerMainOptimize;
extern Timer CCTimerPostOptimize;
extern Timer CCTimerIVPosteOptimize;

#endif
