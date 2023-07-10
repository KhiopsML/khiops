// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifndef DTGlobalTag_H
#define DTGlobalTag_H

#include "ALString.h"
#include "Object.h"

class DTGlobalTag : public Object
{
public:
	static const ALString SCHRODER_LABEL;
	static const ALString REC_LABEL;
	static const ALString REC_RISSANEN_LABEL;
	static const ALString REC_RISSANEN_V1_LABEL;
	static const ALString REC_RISSANEN_V2_LABEL;
	static const ALString REC_N2_LABEL;
	static const ALString REC_BIN_LABEL;
	static const ALString REC_NEW_BIN_LABEL;
	static const ALString REC_TRI_LABEL;
	static const ALString CATALAN_LABEL;
	static const ALString IDENTICAL_LABEL;
	static const ALString BEST_TREE_COST_LABEL;
	static const ALString RANDOM_UNIFORM_LABEL;
	static const ALString RANDOM_LEVEL_LABEL;
	static const ALString MODL_LABEL;
	static const ALString MODL_EQUAL_FREQUENCY_LABEL;
	static const ALString BASIC_GROUPING_LABEL;
	static const ALString PRE_PRUNING_LABEL;
	static const ALString POST_PRUNING_LABEL;
	static const ALString NO_PRUNING_LABEL;
	static const ALString UNIFORM_SAMPLING_WITH_REPLACEMENT_LABEL;
	static const ALString LEVEL_SAMPLING_WITH_REPLACEMENT_LABEL;
	static const ALString RANK_WITH_REPLACEMENT_LABEL;
	static const ALString NODE_UNIFORM_SAMPLING_WITH_REPLACEMENT_LABEL;
	static const ALString NODE_LEVEL_SAMPLING_WITH_REPLACEMENT_LABEL;
	static const ALString RF_OPTIMIZATION_RANDOM_VARIABLES;
	static const ALString RF_OPTIMIZATION_ALL_TREES;
	static const ALString RF_OPTIMIZATION_BEST_TREE;
	static const ALString RF_OPTIMIZATION_MULTISTART;
};

#endif
