// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTConfig.h"

#ifdef TREE_BUILD_TIMER
// declaration de variable pour evaluer les performences de la creation des arbres
Timer DTTimerDiscretizeGFT;
Timer DTTimerDiscretize;
Timer DTTimerTree1;
Timer DTTimerTree2;
Timer DTTimerTree3;
Timer DTTimerTree4;
Timer DTTimerBasic1;
Timer DTTimerBasic2;
Timer DTTimer_CreatePreparedAttributes;
Timer DTTimer_CreateDecisionTree;
Timer DTTimer_ComputeTree;
Timer DTTimer_BuildAllTree;
Timer DTTimer_BuildRootAttributeStats;
Timer DTTimer_CreateAttribute;
#endif
