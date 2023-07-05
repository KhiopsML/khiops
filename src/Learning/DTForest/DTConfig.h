// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Timer.h"

// pre-condition
#undef StartTimer
#ifndef TREE_BUILD_TIMER
#define StartTimer(ignore) ((void)0)
#else
#define StartTimer(p) ((&p) ? (void)0 : #p##".Start()")
#endif
#undef StopTimer
#ifndef TREE_BUILD_TIMER
#define StopTimer(ignore) ((void)0)
#else
#define StopTimer(p) ((&p) ? (void)0 : #p##".Stop()")
#endif

#define MAX_VARIABLE_2_BUILTREE_BORNE 100
#define MIN_VARIABLE_2_BUILTREE_BORNE 25
#define MIN_VARIABLE_2_BUILTREE 6
#define MIN_DRAWING_NULL_VARIABLE_PROBABILITY 0.0
#define MAX_DRAWING_NULL_VARIABLE_PROBABILITY 0.2

#ifdef TREE_BUILD_TIMER
// Timer pour mesurer le temps de construction des variables
extern Timer DTTimerDiscretizeGFT;
extern Timer DTTimerDiscretize;
extern Timer DTTimerTree1;
extern Timer DTTimerTree2;
extern Timer DTTimerTree3;
extern Timer DTTimerTree4;
extern Timer DTTimerBasic1;
extern Timer DTTimerBasic2;
extern Timer DTTimer_CreatePreparedAttributes;
extern Timer DTTimer_CreateDecisionTree;
extern Timer DTTimer_ComputeTree;
extern Timer DTTimer_BuildAllTree;
extern Timer DTTimer_BuildRootAttributeStats;
extern Timer DTTimer_CreateAttribute;
#endif