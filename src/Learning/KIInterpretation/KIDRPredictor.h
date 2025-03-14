// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDerivationRule.h"
#include "KWDRNBPredictor.h"
#include "KWDRDataGrid.h"
#include "KIShapleyTable.h"
#include "KIInterpretationDictionary.h"

////////////////////////////////////////////////////////////
// Classe KIPartitionedAttributeProbas
// Classe de stockage des contribution ou renforcement d'un attribut partitionne
// pemet de stocker pour une variable et une modalite la contribution et le reinforcement
class KIPartitionedAttributeProbas : public Object
{
	// Constructeur
	KIPartitionedAttributeProbas();
	~KIPartitionedAttributeProbas();

protected:
	friend class KIDRClassifierContribution;
	friend class KIDRClassifierReinforcement;
	friend int KICompareReinforcementNewScore(const void* elem1, const void* elem2);
	friend int KICompareContributionImportanceValue(const void* elem1, const void* elem2);

	int nAttributeIndex;
	int nModalityIndex;
	Continuous dReinforcementNewScore;
	Continuous dReinforcementClassHasChanged;
	Continuous dContributionImportanceValue;
};

////////////////////////////////////////////////////////////
// Classe KITargetValueProbas
// Classe de stockage des probas du modele pour une cible (target value) donnee
// permet de stocker les probas a posteriori de la classe pour chaque partie
// de la variable explicative
class KITargetValueProbas : public Object
{
	// Constructeur
	KITargetValueProbas();
	~KITargetValueProbas();

	// Ecriture
	void Write(ostream&) const;

	// Memoire utilisee par KITargetValueProbas
	longint GetUsedMemory() const override;

protected:
	friend class KIDRClassifierInterpretation;
	friend class KIDRClassifierReinforcement;
	friend class KIDRClassifierContribution;

	// nom de la valeur cible
	ALString sTargetValue;
	// proba de la valeur cible
	Continuous dProbaApriori;

	// un ContinuousVector * par variable explicative. Chaque ContinuousVector contient les logs des probas a
	// 	posteriori de la classe, pour chaque partie de la variable explicative
	ObjectArray* oaProbasAposteriori;
};

////////////////////////////////////////////////////////////
// Classe ancetre (abstraite) des classes de calcul des probas d'interpretation
// (contribution ou renforcement)
// classe abstraite
class KIDRClassifierInterpretation : public KWDerivationRule
{
public:
	// Constructeur
	KIDRClassifierInterpretation();
	~KIDRClassifierInterpretation();

	// Creation
	virtual KWDerivationRule* Create() const override = 0;

	// compile la regle de derivation
	void Compile(KWClass* kwcOwnerClass) override;

	// liste des labels utlises pour differencier les methodes de calcul
	// de score d'interpretation et leur derivationrule
	static const ALString SHAPLEY_LABEL;
	static const ALString PREDICTED_CLASS_LABEL;
	static const ALString CLASS_OF_HIGHEST_GAIN_LABEL;
	static const ALString ALL_CLASSES_LABEL;
	static const ALString LEVER_ATTRIBUTE_META_TAG;
	static const ALString INTERPRETATION_ATTRIBUTE_META_TAG;
	static const ALString NO_VALUE_LABEL;

	// Memoire utilisee par KIDRClassifierInterpretation
	longint GetUsedMemory() const override;

protected:
	// netoye la regle
	virtual void Clean();

	// Extraction de la log proba de la modalite donnee d'un attribut donne
	// conditionnellement a une classe donnee
	Continuous ExtractLogPosteriorProba(int nClassIndex, int nAttributeIndex, int nModalityIndex) const;

	// Test que kwcOwnerClass est bien un NB ou un SNB avec les bons orepands
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const;

	//////////////// variables membres //////////////////

	// probas du modele, par modalite cible (donc, valables quel que soit l'instance traitee) :
	// 	- 1 poste par modalite cible
	// 	- pour chaque poste : un pointeur sur objet de type KITargetValueProbas
	mutable ObjectArray oaModelProbabilities;

	// Probas liees a l'instance en cours de traitement :
	// Cle = index de l'attribut partitionne.
	// Valeur = pointeur sur objet KIPartitionedAttributeProbas
	mutable ObjectArray* oaInstanceProbabilities;

	// Noms des variables partitionnees
	mutable StringVector svPartitionedPredictiveAttributeNames;

	// Noms des variables natives
	StringVector svNativePredictiveAttributeNames;

	// 	cle = modalite cible
	// 	valeur = entier dans un StringObject *, qui renvoie a l'entree correspondante dans le tableau oaModelProbabilities
	mutable ObjectDictionary odClassNamesIndexes;

	// vecteur de poids des variables utilisees par le SNB ou NB
	mutable ContinuousVector cvVariableWeights;

	// Vecteurs des valeurs cibles
	SymbolVector svTargetValues;

	// frequences des valeurs cibles
	IntVector ivTargetFrequencies;

	// frequence total
	Continuous dTotalFrequency;
};

////////////////////////////////////////////////////////////
// Classe herite de KIDRClassifierInterpretation des classes
// de calcul des probas d'interpretation pour un classifieur
// de type KWDRNBClassifier
class KIDRClassifierContribution : public KIDRClassifierInterpretation
{
public:
	// Constructeur
	KIDRClassifierContribution();
	~KIDRClassifierContribution();

	// Creation
	KWDerivationRule* Create() const override;

	//compile la regle
	void Compile(KWClass* kwcOwnerClass) override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Valeur de l'importance de la variable contributive (la numerotation du rang comemnce a 0 et non a 1, contrairement au rang figurant dans la RDD du dictionaire)
	Continuous GetContributionValueAt(int rank) const;

	// nom de la variable importante (la numerotation du rang commence a 0 et non a 1, contrairement au rang figurant dans la RDD du dictionaire)
	Symbol GetContributionNameAt(int rank) const;

	// partition ou groupement de modalites de la variable importante (la numerotation du rang comemnce a 0 et non a 1, contrairement au rang figurant dans la RDD du dictionaire)
	Symbol GetContributionPartitionAt(int rank) const;

	// Donne le nom du type de caontribution
	// Enables to choose the reference class for the contribution
	// Analysis among the following choices:
	// - Predicted class,
	// - One of the target values;
	// - Class of highest gain: class, for which the ratio between
	// the probability of the class knowing the instance and the
	// prior probability of the class is the highest,
	// - All classes: all the classes successively. (default)
	Symbol GetContributionClass() const;

	// Memoire utilisee par KIDRClassifierContribution
	longint GetUsedMemory() const override;

protected:
	// Calcul des donnees de contribution
	void ComputeContribution(const KWObject* kwoObject) const;

	/// Calcul de la valeur d'importance pour un attribut et une partie de cet attribut (pour le pourquoi)
	/// selon la probabilite de la modalite de l'attribut conditionnellement a la classe : p(X_i | C) - MOP
	Continuous ComputeModalityProbability(int nAttributeIndex, int nTargetClassIndex, int nModalityIndex) const;

	// Calcul du max de la log proba conditionnelle sur les classes
	// autres que la classe indiqueee en entree, our un attribut donne et une modalite donnee
	// pour cet attribut - rien juste utile pour calculer VPD et LVPD
	Continuous ComputeMaxLogPosteriorProbaWithoutWhyClassValue(int nWhyTargetValueNumber, int nAttributeIndex,
								   int nModalityIndex) const;

	// meme chose que ComputeModalityProbability, mais pour toutes les classes sauf la classe cible
	Continuous ComputeModalityProbabilityWithoutTargetClass(int nAttributeIndex, int nTargetClassIndex,
								int nModalityIndex) const;

	// Calcul de la valeur Shapley
	// nModalityIndex indique dans quel intervalle ou groupe de l'attribut designe par nAttributeIndex, cet individu appartient
	// nTargetClassIndex est la classe cible pour le calcul de l'importance
	Continuous ComputeShapley(const int nAttributeIndex, const int nTargetClassIndex,
				  const int nModalityIndex) const;

	//  Precalul les valeurs de Shapley
	void InitializeShapleyTables();

	ContinuousVector* ComputeScoreVectorLjWithoutOneVariable(IntVector* ivModalityIndexes,
								 int nVariableIndex) const;
	ContinuousVector* ComputeScoreVectorLj(IntVector* ivModalityIndexes) const;
	Continuous ComputeScoreFromScoreVector(ContinuousVector* cvScoreVector, int nReferenceClassIndex) const;

	// Extraction de la log proba a priori de la classe donnee
	Continuous ExtractLogPriorProba(int nClassIndex) const;

	//////////////// variables membres //////////////////

	// classe cible de la contribution : classe predite pour l'individu OU classe de gain le plus eleve pour l'individu OU une classe specifiee explicitement via l'IHM
	mutable Symbol sContributionClass;

	mutable boolean bSortInstanceProbas;

	//structure pour sauvegarder les precalculs des valeurs de Shapley
	ObjectArray oaShapleyTables;
};

////////////////////////////////////////////////////////////
// Classe KIDRContributionValueAt
// Donne la valeur de la contribution a partir de KIDRClassifierContribution
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

////////////////////////////////////////////////////////////
// Classe KIDRContributionValueAt
// Donne la nom de la variable de contribution a partir de KIDRClassifierContribution
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

////////////////////////////////////////////////////////////
// Classe KIDRContributionValueAt
// Donne la parti de la variable de la contribution a partir de KIDRClassifierContribution
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

////////////////////////////////////////////////////////////
// Classe KIDRContributionValueAt
// Donne la valeur de la cible a partir de KIDRClassifierContribution
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

////////////////////////////////////////////////////////////
// Classe KIDRContributionPriorClass
// Donne la valeur du prior de la classe a partir de KIDRClassifierContribution
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

////////////////////////////////////////////////////////////
// Classe herite de KIDRClassifierInterpretation des classes
// de calcul des valeur de reinforcement pour un classifieur
// de type KWDRNBClassifier
class KIDRClassifierReinforcement : public KIDRClassifierInterpretation
{
public:
	// Constructeur
	KIDRClassifierReinforcement();
	~KIDRClassifierReinforcement();

	// Creation
	KWDerivationRule* Create() const override;

	// Compile la regle
	void Compile(KWClass* kwcOwnerClass) override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Valeur de la proba a priori de la classe a renforcer, pour une variable levier donnee
	Continuous GetReinforcementInitialScore() const;

	// Valeur de la proba a posteriori de la classe a renforcer, pour une variable levier donnee
	Continuous GetReinforcementFinalScoreAt(int rank) const;

	// Tag indiquant si la classe apres renforcement a change ou non
	// 1  : la classe a changer cer la classe voulue
	// 0  : pas de changement possible
	// -1 : changement possible vers une autre classe
	Continuous GetReinforcementClassChangeTagAt(int rank) const;

	// nom de la variable de renforcement (la numerotation du rang commence a 0 et non a 1, contrairement au rang figurant dans la RDD du dictionaire)
	Symbol GetReinforcementNameAt(int rank) const;

	// partition ou groupement de modalites de la variable de renforcement (la numerotation du rang comemnce a 0 et non a 1, contrairement au rang figurant dans la RDD du dictionaire)
	Symbol GetReinforcementPartitionAt(int rank) const;

protected:
	// Calcul des differents valeurs
	void ComputeReinforcement(const KWObject* kwoObject) const;
	void ComputeReinforcementProbas(IntVector* ivModalityIndexes, Symbol sPredictedClass,
					ContinuousVector* cvBestScore, const int nHowNumber) const;
	ContinuousVector* ComputeScoreVectorLjWithoutOneVariable(IntVector* ivModalityIndexes,
								 int nVariableIndex) const;
	ContinuousVector* ComputeScoreVectorLj(IntVector* ivModalityIndexes) const;
	Continuous ComputeScoreFromScoreVector(ContinuousVector* cvScoreVector, int nReferenceClassIndex) const;

	// Calcul de la variation du vecteur de scores si l'on modifie une composante
	ContinuousVector* ComputeScoreVectorVariation(ContinuousVector* cvPreviousScoreVector, int nAttributeIndex,
						      int nPreviousModalityIndex, int nNewModalityIndex) const;

	// nombre de valuer de la classe
	int nTargetValuesNumberInNBScore;

	// score initiale
	mutable Continuous cInitialScore;
};

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementInitialScore
// Donne la valeur de reinforcement initiale
// a partir de KIDRClassifierReinforcement
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

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementInitialScore
// Donne la valeur de reinforcement finale apres reinforcement
// a partir de KIDRClassifierReinforcement
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

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementInitialScore
// Donne la valeur de la variable reenforcee
// a partir de KIDRClassifierReinforcement
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

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementInitialScore
// Donne la valeur de la partition de la variable reenforcee
// a partir de KIDRClassifierReinforcement
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

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementInitialScore
// Donne le tag de reenforcement de la variable reenforcee
// a partir de KIDRClassifierReinforcement
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

//////////////////////////////  Methodes en inline ///////////////////////////////

inline Continuous KIDRClassifierContribution::ExtractLogPriorProba(int nClassIndex) const
{
	require(oaModelProbabilities.GetSize() > 0);

	// Extraction du tableau des probas pour la classe cible courante
	KITargetValueProbas* targetValueProbas = cast(KITargetValueProbas*, oaModelProbabilities.GetAt(nClassIndex));

	return targetValueProbas->dProbaApriori;
}

inline int KICompareReinforcementNewScore(const void* elem1, const void* elem2)
{
	int nCompare;
	KIPartitionedAttributeProbas* dataAttribute1;
	KIPartitionedAttributeProbas* dataAttribute2;

	// Acces aux objets
	dataAttribute1 = cast(KIPartitionedAttributeProbas*, *(Object**)elem1);
	dataAttribute2 = cast(KIPartitionedAttributeProbas*, *(Object**)elem2);
	assert(dataAttribute1->Check());
	assert(dataAttribute2->Check());

	// Comparaison selon la precision du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(dataAttribute1->dReinforcementNewScore,
							dataAttribute2->dReinforcementNewScore);

	// Comparaison sur le nom en cas d'egalite du level (sort value)
	if (nCompare == 0)
		nCompare = dataAttribute1->nAttributeIndex - dataAttribute2->nAttributeIndex;
	return nCompare;
}

inline int KICompareContributionImportanceValue(const void* elem1, const void* elem2)
{
	int nCompare;
	KIPartitionedAttributeProbas* dataAttribute1;
	KIPartitionedAttributeProbas* dataAttribute2;

	// Acces aux objets
	dataAttribute1 = cast(KIPartitionedAttributeProbas*, *(Object**)elem1);
	dataAttribute2 = cast(KIPartitionedAttributeProbas*, *(Object**)elem2);
	assert(dataAttribute1->Check());
	assert(dataAttribute2->Check());

	// Comparaison selon la precision du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(dataAttribute1->dContributionImportanceValue,
							dataAttribute2->dContributionImportanceValue);

	// Comparaison sur le nom en cas d'egalite du level (sort value)
	if (nCompare == 0)
		nCompare = dataAttribute1->nAttributeIndex - dataAttribute2->nAttributeIndex;
	return nCompare;
}
