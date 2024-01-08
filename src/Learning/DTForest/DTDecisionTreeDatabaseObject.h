// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifndef DTDATABASE_INSTANCE_H
#define DTDATABASE_INSTANCE_H

#include "Object.h"
#include "KWSymbol.h"
#include "Vector.h"
#include "KWContinuous.h"

class DTDecisionTreeDatabaseObject : public Object
{
public:
	DTDecisionTreeDatabaseObject(int id);
	~DTDecisionTreeDatabaseObject();

	int GetId() const;

	const Symbol& GetNodeIdentifier() const;
	void SetNodeIdentifier(const Symbol&);

	void IncrementFrequency();

	int GetFrequency() const;
	void SetFrequency(int);

	Continuous GetScoreWeight() const;
	void SetScoreWeight(Continuous);

	Continuous GetBoostingTreeWeight() const;
	void SetBoostingTreeWeight(Continuous);

	Continuous GetAdaBoostBGErrorRate() const;
	void SetAdaBoostBGErrorRate(Continuous);

	int GetTargetModalityIndex() const;
	void SetTargetModalityIndex(int);

	boolean IsTargetCorrectlyPredicted() const;
	void SetTargetCorrectlyPredicted(boolean);

	static void WriteHeaderLine(ostream& ost);
	void Write(ostream& ost) const;

	// probas correspondant au noeud auquel appartient l'instance
	const ContinuousVector* GetTrainNodeProbs() const;
	void SetTrainNodeProbs(const ContinuousVector*);

	DTDecisionTreeDatabaseObject* Clone() const;

	void CopyFrom(const DTDecisionTreeDatabaseObject* aSource);

protected:
	// no d'ordre de l'instance en base de donnees
	int iId;

	// identifiant du noeud de l'arbre associe a cette instance de base
	Symbol sNodeIdentifier;

	int iFrequency; // nbre de fois ou le meme KWObject est reference (si tirage avec remise)

	/** poids de l'instance dans le calcul du score */
	Continuous cScoreWeight;

	/** poids de l'instance dans le tirage boost avec remise */
	Continuous cBoostingTreeWeight;

	/** proba max trouvee, correspondant a une modalite autre que la modalite reelle de l'instance (algo AdaBoostBG)
	 */
	Continuous cAdaBoostBGErrorRate;

	int iTargetModalityIndex; // index de la modalite cible

	/* la cible a t elle ete correctement predite pour cette instance ? */
	boolean bIsTargetCorrectlyPredicted;

	ContinuousVector* trainNodeProbs;
};

inline const ContinuousVector* DTDecisionTreeDatabaseObject::GetTrainNodeProbs() const
{
	return trainNodeProbs;
}

inline int DTDecisionTreeDatabaseObject::GetId() const
{
	return iId;
}

inline void DTDecisionTreeDatabaseObject::IncrementFrequency()
{
	iFrequency++;
}

inline void DTDecisionTreeDatabaseObject::SetTargetCorrectlyPredicted(boolean b)
{
	bIsTargetCorrectlyPredicted = b;
}

inline boolean DTDecisionTreeDatabaseObject::IsTargetCorrectlyPredicted() const
{
	return bIsTargetCorrectlyPredicted;
}

inline void DTDecisionTreeDatabaseObject::SetFrequency(int i)
{
	iFrequency = i;
}

inline int DTDecisionTreeDatabaseObject::GetFrequency() const
{
	return iFrequency;
}

inline void DTDecisionTreeDatabaseObject::SetTargetModalityIndex(int i)
{
	iTargetModalityIndex = i;
}

inline int DTDecisionTreeDatabaseObject::GetTargetModalityIndex() const
{
	return iTargetModalityIndex;
}

inline Continuous DTDecisionTreeDatabaseObject::GetScoreWeight() const
{
	return cScoreWeight;
}
inline void DTDecisionTreeDatabaseObject::SetScoreWeight(Continuous c)
{
	cScoreWeight = c;
}

inline Continuous DTDecisionTreeDatabaseObject::GetBoostingTreeWeight() const
{
	return cBoostingTreeWeight;
}
inline void DTDecisionTreeDatabaseObject::SetBoostingTreeWeight(Continuous c)
{
	cBoostingTreeWeight = c;
}

inline Continuous DTDecisionTreeDatabaseObject::GetAdaBoostBGErrorRate() const
{
	return cAdaBoostBGErrorRate;
}
inline void DTDecisionTreeDatabaseObject::SetAdaBoostBGErrorRate(Continuous c)
{
	cAdaBoostBGErrorRate = c;
}

inline const Symbol& DTDecisionTreeDatabaseObject::GetNodeIdentifier() const
{
	return sNodeIdentifier;
}
inline void DTDecisionTreeDatabaseObject::SetNodeIdentifier(const Symbol& s)
{
	sNodeIdentifier = s;
}
#endif
