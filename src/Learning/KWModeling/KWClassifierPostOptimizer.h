// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWConfusionMatrix;

#include "KWClass.h"
#include "KWPredictor.h"
#include "KWTrainedPredictor.h"
#include "KWSortableIndex.h"

///////////////////////////////////////////////////////////////////////
// Classe KWClassifierPostOptimizer
// Post-optimisation d'un classifier pour modifier les seuils de prediction
// des valeurs cibles de facon a optimiser un critere donne
// Le critere provient de de la methode GetClassifierCriterion() de KWTrainParameters
class KWClassifierPostOptimizer : public Object
{
public:
	// Constructeur
	KWClassifierPostOptimizer();
	~KWClassifierPostOptimizer();

	// Post-optimisation du classifieur
	// En sortie, on retourne true si la post-optimisation est necessaire et
	// les offsets de prediction sont alors mise a jour
	// Methode a appeler dans le InternalTrain des sous-classes de KWPredictor,
	// une fois que les attributs de probabilite conditionnelle ont ete specifies
	boolean PostOptimize(KWPredictor* predictor, KWTrainedClassifier* trainedClassifier,
			     ContinuousVector* cvBestTargetScoreOffsets);

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Index des criteres de classification pris en compte
	enum
	{
		Accuracy,
		BalancedAccuracy
	};

	// Calcul de l'index d'un critere de classification defini depuis les parametres d'apprentissage
	// (GetClassifierCriterion() de KWTrainParameters)
	// Retourne -1 si non trouve
	int ComputeClassifierCriterionIndex(KWPredictor* predictor) const;

	// Optimisation d'un vecteur d'offets a rajouter aux probabilites conditionnelles dans
	// le cas de la classification, pour optimiser le critere de classification (acc, bacc...)
	// des parametres d'apprentissage dans le cas de la classification
	// Le classifieur est utilise en entree pour relire la base et evaluer les probabilites
	// conditionnelles, ce qui permet d'optimiser les offsets pour maximiser le critere choisi.
	// En sortie, le classifier est modifie avec un attribut de prediction qui tient compte
	// de ces offsets.
	void ComputeBiasedClassificationOffsets(KWPredictor* predictor, KWTrainedClassifier* trainedClassifier,
						ContinuousVector* cvBestTargetScoreOffsets);

	// Calcul de la meilleure correction a apporter pour un vecteur d'offsets donne et une modalite cible donnee
	Continuous ComputeBiasedClassificationOffsetAt(int nClassifierCriterionIndex,
						       const ContinuousVector* cvTargetScoreOffsets, int nTargetIndex);

	// Evaluation du critere sur une matrice de confusion
	double EvaluateConfusionMatrix(int nClassifierCriterion, KWConfusionMatrix* kwctConfusionMatrix);

	// Remplissage de la matrice de confusion, pour un vecteur d'offsets donne
	void FillConfusionMatrix(const ContinuousVector* cvTargetScoreOffsets, KWConfusionMatrix* kwctConfusionMatrix);

	// Lecture de la base pour memoriser les probabilites conditionnelles et les index de valeur cible
	// Renvoie true si chargement correct, sinon renvoie false, avec donnes reinitialisees
	boolean LoadWorkingData(KWPredictor* predictor, KWTrainedClassifier* trainedClassifier);

	// Reinitialisation des variables de travail
	void CleanWorkingData();

	// Visualisation des donnees de travail
	void DisplayWorkingData() const;
	void DisplayOffsets(const ContinuousVector* cvTargetScoreOffsets) const;

	// Acces aux couples (score, index cible) par instance
	Continuous GetScoreAt(int nInstance) const;
	void SetScoreAt(int nInstance, Continuous cScore);
	int GetTargetIndexAt(int nInstance) const;
	void SetTargetIndexAt(int nInstance, int nTargetIndex);

	// Index des classes cibles
	IntVector ivTargetValueIndexes;

	// Tableau des vecteur de probabilite conditionnelles par classe cible
	ObjectArray oaTargetProbVectors;

	// Effectifs des valeurs cibles
	IntVector ivTargetFrequencies;

	// Structure pour le tri des couples (score, index cible), en se servant d'un tableau de KWSortableContinuous
	ObjectArray oaSortableScores;
};

//////////////////////////////////////////////////////////
// Methodes en inline

inline Continuous KWClassifierPostOptimizer::GetScoreAt(int nInstance) const
{
	require(0 <= nInstance and nInstance < oaSortableScores.GetSize());
	return cast(KWSortableContinuous*, oaSortableScores.GetAt(nInstance))->GetSortValue();
}

inline void KWClassifierPostOptimizer::SetScoreAt(int nInstance, Continuous cScore)
{
	require(0 <= nInstance and nInstance < oaSortableScores.GetSize());
	cast(KWSortableContinuous*, oaSortableScores.GetAt(nInstance))->SetSortValue(cScore);
}

inline int KWClassifierPostOptimizer::GetTargetIndexAt(int nInstance) const
{
	require(0 <= nInstance and nInstance < oaSortableScores.GetSize());
	return cast(KWSortableContinuous*, oaSortableScores.GetAt(nInstance))->GetIndex();
}

inline void KWClassifierPostOptimizer::SetTargetIndexAt(int nInstance, int nTargetIndex)
{
	require(0 <= nInstance and nInstance < oaSortableScores.GetSize());
	cast(KWSortableContinuous*, oaSortableScores.GetAt(nInstance))->SetIndex(nTargetIndex);
}

////////////////////////////////////////////////////////////////////////
// Matrice de confusion
// Classe interne, uniquement utilisee pour la classe KWClassifierPostOptimizer
class KWConfusionMatrix : public Object
{
public:
	// Constructeur
	KWConfusionMatrix();
	~KWConfusionMatrix();

	////////////////////////////////////////////////////////////////////
	// Gestion de base du contenu de la matrice

	// Acces a la definition des lois
	void Initialize(int nSourceValueNumber, int nTargetValueNumber);
	int GetSourceValueNumber() const;
	int GetTargetValueNumber() const;

	// Mise a jour des effectifs des cellules
	void SetFrequencyAt(int nSourceIndex, int nTargetIndex, int nNumber);
	int GetFrequencyAt(int nSourceIndex, int nTargetIndex) const;
	void UpgradeFrequencyAt(int nSourceIndex, int nTargetIndex, int nNumber);

	// Statistiques sources, cibles et globales
	int GetSourceFrequencyAt(int nSourceIndex) const;
	int GetTargetFrequencyAt(int nTargetIndex) const;
	int GetTableFrequency() const;

	////////////////////////////////////////////////////////////
	// Divers

	// Duplication
	KWConfusionMatrix* Clone() const;

	// Recopie d'une table source
	void CopyFrom(const KWConfusionMatrix* kwctSource);

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Nombre de valeurs des lois
	int nSourceValueNumberAtt;
	int nTargetValueNumberAtt;

	// Tableaux d'entier de taille (nSourceValueNumberAtt*nTargetValueNumberAtt)
	// representant le contenu de la table de contingence
	IntVector ivTableFrequencies;

	// Vecteurs pour les totaux source et cible
	IntVector ivSourceFrequencies;
	IntVector ivTargetFrequencies;

	// Effectif total
	int nTableFrequency;
};

///////////////////////
// Methodes en inline

inline int KWConfusionMatrix::GetSourceValueNumber() const
{
	return nSourceValueNumberAtt;
}

inline int KWConfusionMatrix::GetTargetValueNumber() const
{
	return nTargetValueNumberAtt;
}

inline void KWConfusionMatrix::UpgradeFrequencyAt(int nSourceIndex, int nTargetIndex, int nNumber)
{
	require(0 <= nSourceIndex and nSourceIndex < GetSourceValueNumber());
	require(0 <= nTargetIndex and nTargetIndex < GetTargetValueNumber());
	require(nNumber >= 0);

	// Incrementation du nombre de l'effectif correspondant
	ivTableFrequencies.UpgradeAt(nSourceIndex * nTargetValueNumberAtt + nTargetIndex, nNumber);
	ivSourceFrequencies.UpgradeAt(nSourceIndex, nNumber);
	ivTargetFrequencies.UpgradeAt(nTargetIndex, nNumber);
	nTableFrequency += nNumber;
}

inline void KWConfusionMatrix::SetFrequencyAt(int nSourceIndex, int nTargetIndex, int nNumber)
{
	int nCellIndex;
	int nDeltaTableFrequency;

	require(0 <= nSourceIndex and nSourceIndex < GetSourceValueNumber());
	require(0 <= nTargetIndex and nTargetIndex < GetTargetValueNumber());
	require(nNumber >= 0);

	// Recherche de la variation d'effectif
	nCellIndex = nSourceIndex * nTargetValueNumberAtt + nTargetIndex;
	nDeltaTableFrequency = nNumber - ivTableFrequencies.GetAt(nCellIndex);

	// Mise a jour de l'effectif correspondant
	ivTableFrequencies.SetAt(nCellIndex, nNumber);
	ivSourceFrequencies.UpgradeAt(nSourceIndex, nDeltaTableFrequency);
	ivTargetFrequencies.UpgradeAt(nTargetIndex, nDeltaTableFrequency);
	nTableFrequency += nDeltaTableFrequency;
}

inline int KWConfusionMatrix::GetFrequencyAt(int nSourceIndex, int nTargetIndex) const
{
	require(0 <= nSourceIndex and nSourceIndex < GetSourceValueNumber());
	require(0 <= nTargetIndex and nTargetIndex < GetTargetValueNumber());

	return ivTableFrequencies.GetAt(nSourceIndex * nTargetValueNumberAtt + nTargetIndex);
}

inline int KWConfusionMatrix::GetTableFrequency() const
{
	return nTableFrequency;
}

inline int KWConfusionMatrix::GetSourceFrequencyAt(int nSourceIndex) const
{
	require(0 <= nSourceIndex and nSourceIndex < GetSourceValueNumber());

	return ivSourceFrequencies.GetAt(nSourceIndex);
}

inline int KWConfusionMatrix::GetTargetFrequencyAt(int nTargetIndex) const
{
	require(0 <= nTargetIndex and nTargetIndex < GetTargetValueNumber());
	return ivTargetFrequencies.GetAt(nTargetIndex);
}
