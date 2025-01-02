// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDerivationRule.h"

/** classe de stockage des probas (contribution ou renforcement) d'un attribut partitionne */
class KIPartitionedAttributeProbas : public Object
{
	KIPartitionedAttributeProbas();
	~KIPartitionedAttributeProbas();

protected:
	friend class KIDRClassifierContribution;
	friend class KIDRClassifierReinforcement;
	friend int KICompareReinforcementNewScore(const void* elem1, const void* elem2);
	friend int KICompareContributionImportanceValue(const void* elem1, const void* elem2);

	int iAttributeIndex;
	int iModalityIndex;
	Continuous cReinforcementNewScore;
	Continuous cReinforcementClassHasChanged;
	Continuous cContributionImportanceValue;
};

/** classe de stockage des probas (contribution ou renforcement) pour une cible (target value) donnee */
class KITargetValueProbas : public Object
{
	KITargetValueProbas();
	~KITargetValueProbas();

	void Write(ostream&) const;

protected:
	friend class KIDRClassifierInterpretation;
	friend class KIDRClassifierReinforcement;
	friend class KIDRClassifierContribution;

	ALString sTargetValue;

	Continuous cProbaApriori;

	/** un ContinuousVector * par variable explicative. Chaque ContinuousVector contient les logs des probas a
		posteriori de la classe, pour chaque partie de la variable explicative */
	ObjectArray* oaProbasAposteriori;
};

/** classe ancetre (abstraite) des classes de calcul des probas d'interpretation (contribution ou renforcement) */
class KIDRClassifierInterpretation : public KWDerivationRule
{
public:
	// Constructeur
	KIDRClassifierInterpretation();
	~KIDRClassifierInterpretation();

	// Creation
	virtual KWDerivationRule* Create() const override = 0; // classe abstraite

	void Compile(KWClass* kwcOwnerClass) override;

	const Symbol SHAPLEY_LABEL = "Shapley";
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
		- pour chaque poste : un pointeur sur objet de type KITargetValueProbas  	*/
	mutable ObjectArray oaModelProbabilities;

	/**
	Probas liees a l'instance en cours de traitement :
	Cle = index de l'attribut partitionne.
	Valeur = pointeur sur objet KIPartitionedAttributeProbas	*/
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

	// frequences des valeurs cibles
	IntVector ivTargetFrequencies;

	Continuous cTotalFrequency;
};

class KIDRClassifierContribution : public KIDRClassifierInterpretation
{
public:
	// Constructeur
	KIDRClassifierContribution();
	~KIDRClassifierContribution();

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
		BayesDistanceWithoutPrior,
		Shapley
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

	/// meme chose que ComputeModalityProbability, mais pour toutes les classes sauf la classe cible
	Continuous ComputeModalityProbabilityWithoutTargetClass(int nAttributeIndex, int nTargetClassIndex,
								int nModalityIndex) const;

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

	/// Calcul de la valeur Shapley
	/// nModalityIndex indique dans quel intervalle ou groupe de l'attribut designe par nAttributeIndex, cet
	/// individu appartient nTargetClassIndex est la classe cible pour le calcul de l'importance
	Continuous ComputeShapley(const int nAttributeIndex, const int nTargetClassIndex,
				  const int nModalityIndex) const;

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

class KIDRContributionValueAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRContributionValueAt();
	~KIDRContributionValueAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

class KIDRContributionNameAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRContributionNameAt();
	~KIDRContributionNameAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class KIDRContributionPartitionAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRContributionPartitionAt();
	~KIDRContributionPartitionAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class KIDRContributionClass : public KWDerivationRule
{
public:
	// Constructeur
	KIDRContributionClass();
	~KIDRContributionClass();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class KIDRContributionPriorClass : public KWDerivationRule
{
public:
	// Constructeur
	KIDRContributionPriorClass();
	~KIDRContributionPriorClass();

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

class KIDRClassifierReinforcement : public KIDRClassifierInterpretation
{
public:
	// Constructeur
	KIDRClassifierReinforcement();
	~KIDRClassifierReinforcement();

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

class KIDRReinforcementInitialScore : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementInitialScore();
	~KIDRReinforcementInitialScore();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

class KIDRReinforcementFinalScoreAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementFinalScoreAt();
	~KIDRReinforcementFinalScoreAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

class KIDRReinforcementNameAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementNameAt();
	~KIDRReinforcementNameAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class KIDRReinforcementPartitionAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementPartitionAt();
	~KIDRReinforcementPartitionAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

class KIDRReinforcementClassChangeTagAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementClassChangeTagAt();
	~KIDRReinforcementClassChangeTagAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////  Methodes en inline ///////////////////////////////

inline Continuous KIDRClassifierContribution::ExtractLogPriorProba(int nClassIndex) const
{
	require(oaModelProbabilities.GetSize() > 0);

	// Extraction du tableau des probas pour la classe cible courante
	KITargetValueProbas* targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

	return targetValueProbas->cProbaApriori;
}

inline int KICompareReinforcementNewScore(const void* elem1, const void* elem2)
{
	Continuous cCompare;

	// Comparaison sur la proba
	cCompare = cast(KIPartitionedAttributeProbas*, *(Object**)elem1)->cReinforcementNewScore -
		   cast(KIPartitionedAttributeProbas*, *(Object**)elem2)->cReinforcementNewScore;

	if (cCompare > 0)
		return -1;
	else
		return 1;
}

inline int KICompareContributionImportanceValue(const void* elem1, const void* elem2)
{
	Continuous cCompare;

	// Comparaison sur la proba
	cCompare = cast(KIPartitionedAttributeProbas*, *(Object**)elem1)->cContributionImportanceValue -
		   cast(KIPartitionedAttributeProbas*, *(Object**)elem2)->cContributionImportanceValue;

	if (cCompare > 0)
		return -1;
	else
		return 1;
}
