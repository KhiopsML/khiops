// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeDatabaseObject.h"

DTDecisionTreeDatabaseObject::DTDecisionTreeDatabaseObject(int id) : iId(id)
{
	iFrequency = 0;
	iTargetModalityIndex = -1;
	cScoreWeight = 1;
	cBoostingTreeWeight = -1;
	cAdaBoostBGErrorRate = -1;
	bIsTargetCorrectlyPredicted = false;
	trainNodeProbs = NULL;
}

DTDecisionTreeDatabaseObject::~DTDecisionTreeDatabaseObject()
{
	if (trainNodeProbs != NULL)
		delete trainNodeProbs;
}

void DTDecisionTreeDatabaseObject::WriteHeaderLine(ostream& ost)
{
	ost << "Id\tFreq\tTM idx\tScore\tIsPredicted\tBoostWeight\tBG error rate" << endl;
}

void DTDecisionTreeDatabaseObject::SetTrainNodeProbs(const ContinuousVector* cv)
{
	assert(cv != NULL);

	if (trainNodeProbs != NULL)
		delete trainNodeProbs;

	trainNodeProbs = new ContinuousVector;
	trainNodeProbs->CopyFrom(cv);
}

DTDecisionTreeDatabaseObject* DTDecisionTreeDatabaseObject::Clone() const
{
	DTDecisionTreeDatabaseObject* newDbo = new DTDecisionTreeDatabaseObject(iId);
	newDbo->CopyFrom(this);
	return newDbo;
}

void DTDecisionTreeDatabaseObject::CopyFrom(const DTDecisionTreeDatabaseObject* aSource)
{
	require(aSource != NULL);

	iFrequency = aSource->iFrequency;
	cScoreWeight = aSource->cScoreWeight;
	cBoostingTreeWeight = aSource->cBoostingTreeWeight;
	cAdaBoostBGErrorRate = aSource->cAdaBoostBGErrorRate;
	iTargetModalityIndex = aSource->iTargetModalityIndex;
	bIsTargetCorrectlyPredicted = aSource->bIsTargetCorrectlyPredicted;

	if (trainNodeProbs != NULL)
	{
		delete trainNodeProbs;
		trainNodeProbs = NULL;
	}

	if (aSource->trainNodeProbs != NULL)
	{
		trainNodeProbs = new ContinuousVector;
		trainNodeProbs->CopyFrom(aSource->trainNodeProbs);
	}
}

void DTDecisionTreeDatabaseObject::Write(ostream& ost) const
{
	ost << iId << "\t" << iFrequency << "\t" << iTargetModalityIndex << "\t" << cScoreWeight << "\t"
	    << bIsTargetCorrectlyPredicted << "\t" << cBoostingTreeWeight << "\t" << cAdaBoostBGErrorRate << endl;
}
