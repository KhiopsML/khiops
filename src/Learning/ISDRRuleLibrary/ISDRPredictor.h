// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/*
 * #%L
 * Software Name: Khiops Interpretation
 * Version : 9.0
 * %%
 * Copyright (C) 2019 Orange
 * This software is the confidential and proprietary information of Orange.
 * You shall not disclose such confidential information and shall use it only
 * in accordance with the terms of the license agreement you entered into
 * with Orange.
 * #L%
 */

#pragma once

#include "KWDerivationRule.h"

/** classe de stockage des probas (contribution ou renforcement) d'un attribut partitionne */
class ISPartitionedAttributeProbas : public Object
{

	ISPartitionedAttributeProbas();
	~ISPartitionedAttributeProbas();

protected:
	friend class ISDRClassifierContribution;
	friend class ISDRClassifierReinforcement;
	friend int ISCompareReinforcementNewScore(const void* elem1, const void* elem2);
	friend int ISCompareContributionImportanceValue(const void* elem1, const void* elem2);

	int iAttributeIndex;
	int iModalityIndex;
	Continuous cReinforcementNewScore;
	Continuous cReinforcementClassHasChanged;
	Continuous cContributionImportanceValue;
};

/** classe de stockage des probas (contribution ou renforcement) pour une cible (target value) donnee */
class ISTargetValueProbas : public Object
{

	ISTargetValueProbas();
	~ISTargetValueProbas();

protected:
	friend class ISDRClassifierInterpretation;
	friend class ISDRClassifierReinforcement;
	friend class ISDRClassifierContribution;

	ALString sTargetValue;

	Continuous cProbaApriori;

	/** un ContinuousVector * par variable explicative. Chaque ContinuousVector contient les logs des probas a
		posteriori de la classe, pour chaque partie de la variable explicative */
	ObjectArray* oaProbasAposteriori;
};

/** classe ancetre (abstraite) des classes de calcul des probas d'interpretation (contribution ou renforcement) */
class ISDRClassifierInterpretation : public KWDerivationRule
{

public:
	// Constructeur
	ISDRClassifierInterpretation();
	~ISDRClassifierInterpretation();

	// Creation
	virtual KWDerivationRule* Create() const override = 0; // classe abstraite

	void Compile(KWClass* kwcOwnerClass) override;

	const Symbol NORMALIZED_ODDS_RATIO_LABEL = "NormalizedOddsRatio";
	const Symbol MIN_PROBA_DIFF_LABEL = "MinProbaDiff";
	const Symbol WEIGHT_EVIDENCE_LABEL = "WeightEvidence";
	const Symbol INFO_DIFF_LABEL = "InfoDiff";
	const Symbol DIFF_PROBA_LABEL = "DiffProba";
	const Symbol MODALITY_PROBA_LABEL = "ModalityProba";
	const Symbol BAYES_DISTANCE_LABEL = "BayesDistance";
	const Symbol KULLBACK_LABEL = "Kullback";
	const Symbol LOG_MODALITY_PROBA_LABEL = "LogModalityProba";
	const Symbol LOG_MIN_PROBA_DIFF_LABEL = "LogMinProbaDiff";
	const Symbol BAYES_DISTANCE_WITHOUT_PRIOR_LABEL = "BayesDistanceWithoutPrior";
	const Symbol PREDICTED_CLASS_LABEL = "Predicted class";
	const Symbol CLASS_OF_HIGHEST_GAIN_LABEL = "Class of highest gain";
	const Symbol ALL_CLASSES_LABEL = "All classes";

	static const ALString LEVER_ATTRIBUTE_META_TAG;
	static const ALString INTERPRETATION_ATTRIBUTE_META_TAG;
	static const ALString NO_VALUE_LABEL;

protected:
	virtual void Clean();

	/// Extraction de la log proba de la modalite donnee d'un attribut donne
	/// conditionnellement a une classe donnee
	Continuous ExtractLogPosteriorProba(int nClassIndex, int nAttributeIndex, int nModalityIndex) const;

	///////////////// variables membres //////////////////

	/** probas du modele, par modalite cible (donc, valables quel que soit l'instance traitee) :
		- 1 poste par modalite cible
		- pour chaque poste : un pointeur sur objet de type ISTargetValueProbas  	*/
	mutable ObjectArray oaModelProbabilities;

	/**
	Probas liees a l'instance en cours de traitement :
	Cle = index de l'attribut partitionne.
	Valeur = pointeur sur objet ISPartitionedAttributeProbas	*/
	mutable ObjectArray* oaInstanceProbabilities;

	/* tableau de noms de variables partitionnees */
	mutable ObjectArray oaPartitionedPredictiveAttributeNames;

	/* tableau de noms de variables natives */
	ObjectArray oaNativePredictiveAttributeNames;

	/*	cle = modalite cible
		valeur = entier dans un StringObject *, qui renvoie a l'entree correspondante dans le tableau
	   oaModelProbabilities */
	mutable ObjectDictionary odClassNamesIndexes;

	mutable ContinuousVector cvVariableWeights;

	// Vecteurs des valeurs cibles
	SymbolVector svTargetValues;
};

class ISDRClassifierContribution : public ISDRClassifierInterpretation
{
public:
	// Constructeur
	ISDRClassifierContribution();
	~ISDRClassifierContribution();

	// Creation
	KWDerivationRule* Create() const override;

	void Compile(KWClass* kwcOwnerClass) override;

	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	/** Valeur de l'importance de la variable contributive (la numerotation du rang comemnce a 0 et non a 1,
	 * contrairement au rang figurant dans la RDD du dictionaire) */
	Continuous GetContributionValueAt(int rank) const;

	/** nom de la variable importante (la numerotation du rang commence a 0 et non a 1, contrairement au rang
	 * figurant dans la RDD du dictionaire) */
	Symbol GetContributionNameAt(int rank) const;

	/** partition ou groupement de modalites de la variable importante (la numerotation du rang comemnce a 0 et non
	 * a 1, contrairement au rang figurant dans la RDD du dictionaire) */
	Symbol GetContributionPartitionAt(int rank) const;

	Symbol GetContributionClass() const;

	enum ContributionComputingMethod
	{
		NormalizedOddsRatio,
		ImportanceValue,
		WeightOfEvidence,
		InformationDifference,
		DifferenceProbabilities,
		ModalityProbability,
		BayesDistance,
		Kullback,
		LogModalityProbability,
		LogImportanceValue,
		BayesDistanceWithoutPrior
	};

protected:
	// Calcul des donnees de contribution
	void ComputeContribution(const KWObject* kwoObject) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi) - VPD
	Continuous ComputeImportanceValue(int nAttributeIndex, int nTargetClassIndex, int nModalityIndex) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi) - LVPD
	Continuous ComputeLogImportanceValue(int nAttributeIndex, int nTargetClassIndex, int nModalityIndex) const;

	/// Calcul du max de la log proba conditionnelle sur les classes
	/// autres que la classe indiqueee en entree, our un attribut donne et une modalite donnee
	/// pour cet attribut - rien juste utile pour calculer VPD et LVPD
	Continuous ComputeMaxLogPosteriorProbaWithoutWhyClassValue(int nWhyTargetValueNumber, int nAttributeIndex,
								   int nModalityIndex) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi)
	/// selon l'indicateur Weight Of Evidence de l'article Sikonja/Kononenko - WOE
	Continuous ComputeWeightOfEvidence(int nAttributeIndex, int nTargetClassIndex, IntVector* ivModalityIndexes,
					   int nDatabaseSize, int nTargetValuesNumber) const;

	/// Calcul du Normalized Odds Ratio (NOR)
	Continuous ComputeNormalizedOddsRatio(int nAttributeIndex, int nTargetClassIndex, IntVector* ivModalityIndexes,
					      int nDatabaseSize, int nTargetValuesNumber) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi)
	/// selon l'indicateur Information Difference de l'article Sikonja/Kononenko - IDI (=LDOP)
	Continuous ComputeInformationDifference(int nAttributeIndex, int nTargetClassIndex,
						IntVector* ivModalityIndexes, int nDatabaseSize,
						int nTargetValuesNumber) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi)
	/// selon l'indicateur Difference of Probabilities de l'article Sikonja/Kononenko - DOP
	Continuous ComputeDifferenceProbabilities(
	    int nAttributeIndex, int nTargetClassIndex,
	    IntVector* ivModalityIndexes) const; //,int nDatabaseSize, int nTargetValuesNumber);

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi)
	/// selon la probabilite de la modalite de l'attribut conditionnellement a la classe : p(X_i | C) - MOP
	Continuous ComputeModalityProbability(int nAttributeIndex, int nTargetClassIndex, int nModalityIndex) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi)
	/// selon la probabilite de la modalite de l'attribut conditionnellement a la classe : log p(X_i | C) - LMOP
	Continuous ComputeLogModalityProbability(int nAttributeIndex, int nTargetClassIndex, int nModalityIndex) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi)
	/// selon : log p(X_i | C) * Weight(X_i) * P(C) ou Weight est le poids dans le SNB et valant 1 dans le cas
	/// du NB - MODL
	Continuous ComputeBayesDistance(int nAttributeIndex, int nTargetClassIndex, int nModalityIndex) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi)
	/// selon : log p(X_i | C) * Weight(X_i) ou Weight est le poids dans le SNB et valant 1 dans le cas
	/// du NB - MODL2
	Continuous ComputeBayesDistanceWithoutPrior(int nAttributeIndex, int nTargetClassIndex,
						    int nModalityIndex) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi)
	/// selon la divergence de Kullback-Leibler (non symetrisee) - KLD
	Continuous ComputeKullback(int nAttributeIndex, int nTargetClassIndex, IntVector* ivModalityIndexes,
				   int nDatabaseSize, int nTargetValuesNumber) const;

	ContinuousVector* ComputeScoreVectorLjWithoutOneVariable(IntVector* ivModalityIndexes,
								 int nVariableIndex) const;
	ContinuousVector* ComputeScoreVectorLj(IntVector* ivModalityIndexes) const;
	Continuous ComputeScoreFromScoreVector(ContinuousVector* cvScoreVector, int nReferenceClassIndex) const;

	/// Extraction de la log proba a priori de la classe donnee
	Continuous ExtractLogPriorProba(int nClassIndex) const;

	///////////////// variables membres //////////////////

	/** classe cible de la contribution : classe predite pour l'individu OU classe de gain le plus eleve pour
	 * l'individu OU une classe specifiee explicitement via l'IHM  */
	mutable Symbol sContributionClass;

	mutable boolean bSortInstanceProbas;

	ContributionComputingMethod contributionComputingMethod;
};

class ISDRContributionValueAt : public KWDerivationRule
{
public:
	// Constructeur
	ISDRContributionValueAt();
	~ISDRContributionValueAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

class ISDRContributionNameAt : public KWDerivationRule
{
public:
	// Constructeur
	ISDRContributionNameAt();
	~ISDRContributionNameAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class ISDRContributionPartitionAt : public KWDerivationRule
{
public:
	// Constructeur
	ISDRContributionPartitionAt();
	~ISDRContributionPartitionAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class ISDRContributionClass : public KWDerivationRule
{
public:
	// Constructeur
	ISDRContributionClass();
	~ISDRContributionClass();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class ISDRContributionPriorClass : public KWDerivationRule
{
public:
	// Constructeur
	ISDRContributionPriorClass();
	~ISDRContributionPriorClass();

	// Creation
	KWDerivationRule* Create() const override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	const Symbol PREDICTED_CLASS_LABEL = "Predicted class";
	const Symbol CLASS_OF_HIGHEST_GAIN_LABEL = "Class of highest gain";
	const Symbol ALL_CLASSES_LABEL = "All classes";

protected:
	SymbolVector svTargetValues;
	IntVector ivDataGridSetTargetFrequencies;
	int nTotalFrequency;
};

class ISDRClassifierReinforcement : public ISDRClassifierInterpretation
{
public:
	// Constructeur
	ISDRClassifierReinforcement();
	~ISDRClassifierReinforcement();

	// Creation
	KWDerivationRule* Create() const override;

	void Compile(KWClass* kwcOwnerClass) override;

	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	/** Valeur de la proba a priori de la classe a renforcer, pour une variable levier donnee */
	Continuous GetReinforcementInitialScore() const;

	/** Valeur de la proba a posteriori de la classe a renforcer, pour une variable levier donnee */
	Continuous GetReinforcementFinalScoreAt(int rank) const;

	/** tag indiquant si la classe apres renforcement a change ou non */
	Continuous GetReinforcementClassChangeTagAt(int rank) const;

	/** nom de la variable de renforcement (la numerotation du rang commence a 0 et non a 1, contrairement au rang
	 * figurant dans la RDD du dictionaire) */
	Symbol GetReinforcementNameAt(int rank) const;

	/** partition ou groupement de modalites de la variable de renforcement (la numerotation du rang comemnce a 0 et
	 * non a 1, contrairement au rang figurant dans la RDD du dictionaire) */
	Symbol GetReinforcementPartitionAt(int rank) const;

protected:
	// Calcul des donnees
	void ComputeReinforcement(const KWObject* kwoObject) const;

	void ComputeReinforcementProbas(IntVector* ivModalityIndexes, Symbol sPredictedClass,
					ContinuousVector* cvBestScore, const int nHowNumber) const;

	ContinuousVector* ComputeScoreVectorLjWithoutOneVariable(IntVector* ivModalityIndexes,
								 int nVariableIndex) const;

	ContinuousVector* ComputeScoreVectorLj(IntVector* ivModalityIndexes) const;

	Continuous ComputeScoreFromScoreVector(ContinuousVector* cvScoreVector, int nReferenceClassIndex) const;

	/// Calcul de la variation du vecteur de scores si l'on modifie une composante
	ContinuousVector* ComputeScoreVectorVariation(ContinuousVector* cvPreviousScoreVector, int nAttributeIndex,
						      int nPreviousModalityIndex, int nNewModalityIndex) const;

	void Initialize();

	int nTargetValuesNumberInNBScore;

	mutable Continuous cInitialScore;
};

class ISDRReinforcementInitialScore : public KWDerivationRule
{
public:
	// Constructeur
	ISDRReinforcementInitialScore();
	~ISDRReinforcementInitialScore();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

class ISDRReinforcementFinalScoreAt : public KWDerivationRule
{
public:
	// Constructeur
	ISDRReinforcementFinalScoreAt();
	~ISDRReinforcementFinalScoreAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

class ISDRReinforcementNameAt : public KWDerivationRule
{
public:
	// Constructeur
	ISDRReinforcementNameAt();
	~ISDRReinforcementNameAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class ISDRReinforcementPartitionAt : public KWDerivationRule
{
public:
	// Constructeur
	ISDRReinforcementPartitionAt();
	~ISDRReinforcementPartitionAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class ISDRReinforcementClassChangeTagAt : public KWDerivationRule
{
public:
	// Constructeur
	ISDRReinforcementClassChangeTagAt();
	~ISDRReinforcementClassChangeTagAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////  Methodes en inline ///////////////////////////////

inline Continuous ISDRClassifierInterpretation::ExtractLogPosteriorProba(int nClassIndex, int nAttributeIndex,
									 int nModalityIndex) const
{
	ContinuousVector* cvVector;

	require(oaModelProbabilities.GetSize() > 0);

	// Extraction du tableau des probas pour la classe cible courante
	ISTargetValueProbas* targetValueProbas = cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

	// Extraction du vecteur de probas pour l'attribut predictif
	cvVector = cast(ContinuousVector*, targetValueProbas->oaProbasAposteriori->GetAt(nAttributeIndex));

	// On retourne la proba associee a la modalite que prend l'individu pour cet attribut predictif
	return cvVector->GetAt(nModalityIndex);
}

inline Continuous ISDRClassifierContribution::ExtractLogPriorProba(int nClassIndex) const
{
	require(oaModelProbabilities.GetSize() > 0);

	// Extraction du tableau des probas pour la classe cible courante
	ISTargetValueProbas* targetValueProbas = cast(ISTargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

	return targetValueProbas->cProbaApriori;
}

inline int ISCompareReinforcementNewScore(const void* elem1, const void* elem2)
{
	Continuous cCompare;

	// Comparaison sur la proba
	cCompare = cast(ISPartitionedAttributeProbas*, *(Object**)elem1)->cReinforcementNewScore -
		   cast(ISPartitionedAttributeProbas*, *(Object**)elem2)->cReinforcementNewScore;

	if (cCompare > 0)
		return -1;
	else
		return 1;
}

inline int ISCompareContributionImportanceValue(const void* elem1, const void* elem2)
{
	Continuous cCompare;

	// Comparaison sur la proba
	cCompare = cast(ISPartitionedAttributeProbas*, *(Object**)elem1)->cContributionImportanceValue -
		   cast(ISPartitionedAttributeProbas*, *(Object**)elem2)->cContributionImportanceValue;

	if (cCompare > 0)
		return -1;
	else
		return 1;
}
