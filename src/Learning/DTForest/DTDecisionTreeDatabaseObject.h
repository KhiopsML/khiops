// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifndef DTDATABASE_INSTANCE_H
#define DTDATABASE_INSTANCE_H

#include "Object.h"
#include "KWSymbol.h"
#include "Vector.h"
#include "KWContinuous.h"

// Instance de base de donnees associee a un noeud d'arbre de decision.
// Encapsule un objet KWObject pour le calcul des effectifs par noeud, le suivi
// de tirage avec remise (AdaBoost, Out-of-Bag) et la prediction de la cible.
class DTDecisionTreeDatabaseObject : public Object
{
public:
	DTDecisionTreeDatabaseObject(int id);
	~DTDecisionTreeDatabaseObject();

	// Identifiant numerique unique de l'instance dans la base
	int GetId() const;

	// Identifiant du noeud feuille auquel l'instance est actuellement affectee
	const Symbol& GetNodeIdentifier() const;
	void SetNodeIdentifier(const Symbol&);

	// Incrementation du compteur de passages de cet objet (tirage avec remise)
	void IncrementFrequency();

	// Nombre de fois ou cet objet a ete tire (1 en tirage sans remise)
	int GetFrequency() const;
	void SetFrequency(int);

	// Poids de l'instance pour le calcul du score de prediction
	Continuous GetScoreWeight() const;
	void SetScoreWeight(Continuous);

	// Poids de l'instance pour le tirage pondere (algorithme de boosting)
	Continuous GetBoostingTreeWeight() const;
	void SetBoostingTreeWeight(Continuous);

	// Taux d'erreur de l'instance pour l'algorithme AdaBoostBG
	Continuous GetAdaBoostBGErrorRate() const;
	void SetAdaBoostBGErrorRate(Continuous);

	// Index de la modalite cible associee a cette instance
	int GetTargetModalityIndex() const;
	void SetTargetModalityIndex(int);

	// Indique si la cible a ete correctement predite pour cette instance
	boolean IsTargetCorrectlyPredicted() const;
	void SetTargetCorrectlyPredicted(boolean);

	static void WriteHeaderLine(ostream& ost);
	void Write(ostream& ost) const override;

	// probas correspondant au noeud auquel appartient l'instance
	const ContinuousVector* GetTrainNodeProbs() const;
	void SetTrainNodeProbs(const ContinuousVector*);

	DTDecisionTreeDatabaseObject* Clone() const;

	void CopyFrom(const DTDecisionTreeDatabaseObject* aSource);

protected:
	// no d'ordre de l'instance en base de donnees
	int nId;

	// identifiant du noeud de l'arbre associe a cette instance de base
	Symbol sNodeIdentifier;

	// nbre de fois ou le meme KWObject est reference (si tirage avec remise)
	int nFrequency;

	// poids de l'instance dans le calcul du score
	Continuous cScoreWeight;

	// poids de l'instance dans le tirage boost avec remise
	Continuous cBoostingTreeWeight;

	// proba max trouvee, correspondant a une modalite autre que la modalite reelle de l'instance (algo AdaBoostBG)
	Continuous cAdaBoostBGErrorRate;

	// index de la modalite cible
	int nTargetModalityIndex;

	// la cible a t elle ete correctement predite pour cette instance ?
	boolean bIsTargetCorrectlyPredicted;

	ContinuousVector* trainNodeProbs;
};

inline const ContinuousVector* DTDecisionTreeDatabaseObject::GetTrainNodeProbs() const
{
	return trainNodeProbs;
}

inline int DTDecisionTreeDatabaseObject::GetId() const
{
	return nId;
}

inline void DTDecisionTreeDatabaseObject::IncrementFrequency()
{
	nFrequency++;
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
	nFrequency = i;
}

inline int DTDecisionTreeDatabaseObject::GetFrequency() const
{
	return nFrequency;
}

inline void DTDecisionTreeDatabaseObject::SetTargetModalityIndex(int i)
{
	nTargetModalityIndex = i;
}

inline int DTDecisionTreeDatabaseObject::GetTargetModalityIndex() const
{
	return nTargetModalityIndex;
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
