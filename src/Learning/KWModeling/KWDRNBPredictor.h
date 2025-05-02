// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour les predicteurs de la famille Naive Bayes a base de DataGrid's

class KWNaiveBayesPredictorRuleHelper;
class KWDRNBClassifier;
class KWDRSNBClassifier;
class KWDRNBRankRegressor;
class KWDRSNBRankRegressor;
class KWDRNBRegressor;
class KWDRSNBRegressor;

#include "KWDerivationRule.h"
#include "KWDRPreprocessing.h"
#include "KWDRPredictor.h"
#include "KWDataGrid.h"
#include "KWDataGridStats.h"
#include "KWContinuous.h"
#include "KWSymbol.h"
#include "KWDRDataGrid.h"
#include "KWDRDataGridBlock.h"

// Enregistrement de ces regles
void KWDRRegisterNBPredictorRules();

///////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWNaiveBayesPredictorRuleHelper
//
// Classe auxiliaire contenant des fonctions pour la verification et compilation
// des regles des predicteurs de la famille Naive Bayes.

class KWNaiveBayesPredictorRuleHelper : public Object
{
public:
	// Constructeur
	KWNaiveBayesPredictorRuleHelper();
	~KWNaiveBayesPredictorRuleHelper();

	// True si l'operand dans la position specifiee est une regle DataGridStatsBlock
	boolean RuleHasDataGridStatsBlockAtOperand(const KWDerivationRule* predictorRule, int nOperand) const;

	// True si l'operand dans la position specifiee est une regle DataGridStats
	boolean RuleHasDataGridStatsAtOperand(const KWDerivationRule* predictorRule, int nOperand) const;

	// Nom du type DataGridStats
	const ALString& GetDataGridStatsRuleName() const;

	// Nom du type DataGridStatsBlock
	const ALString& GetDataGridStatsBlockRuleName() const;

	// Verification que les operands sont des regles DataGridStats ou DataGridStatsBlock
	boolean CheckDataGridStatsOperandsType(const KWDerivationRule* predictorRule, int nFirstDataGridOperand,
					       int nLastDataGridOperand) const;

	// Verification de coherence des effectifs des operateurs DataGridStats ou DataGridStatsBlock
	boolean CheckDataGridStatsOperandsFrequencyAndTargetType(const KWClass* kwcOwnerClass,
								 const KWDerivationRule* predictorRule,
								 int nFirstDataGridOperand, int nLastDataGridOperand,
								 const KWDRDataGrid* referenceDataGridRule) const;

	// Initialisation des objets data grid et poids pour la compilation
	void CompileInitializeDataGridAndWeightRules(const KWClass* kwcOwnerClass,
						     const KWDerivationRule* predictorRule, int nFirstDataGridOperand,
						     int nLastDataGridOperand,
						     ObjectArray* oaDataGridStatsAndBlockRules,
						     ContinuousVector* cvWeights,
						     IntVector* ivIsDataGridStatsRule) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Regles de reference pour les test des operands des regles data grid et data grid bloc
	const KWDRContinuousVector refContinuousVectorRule;
	const KWDRDataGridStats refDataGridStatsRule;
	const KWDRDataGridStatsBlock refDataGridStatsBlockRule;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRNBClassifier
// Classifier base sur une ou plusieurs grilles
// Premiers operandes de type KWDRDataGridStats
// Dernier operande: distribution des valeurs de l'attribut cible (KWDRDataGrid univarie)
class KWDRNBClassifier : public KWDRClassifier
{
public:
	// Constructeur
	KWDRNBClassifier();
	~KWDRNBClassifier();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a une objet, et services associes

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Reimplementation des services de classification, ici precalcules
	Symbol ComputeTargetValue() const override;
	Continuous ComputeTargetProb() const override;
	Continuous ComputeTargetProbAt(const Symbol& sValue) const override;
	Symbol ComputeBiasedTargetValue(const ContinuousVector* cvOffsets) const override;

	////////////////////////////////////////////////////////////////////
	// Methodes avancees pour acceder au calcul des probabilites du predicteur SNB

	// Acces au vecteur des probabilites par valeur cibles
	const ContinuousVector* GetTargetProbs() const;

	// Probabilite associee a uneclasse inconnue en apprentissage (presque nulle, mais non nulle)
	double GetUnknownTargetProb() const;

	// Acces aux termes du numerateur des logs de probabilites, avant normalisation
	// Ce terme est la somme additives des logs des probabilites impliquees pour un predicteur SNB
	const ContinuousVector* GetTargetLogProbNumeratorTerms() const;

	// Recalcul des probabilites a partir d'un vecteur des termes de numerateur
	void ComputeTargetProbsFromNumeratorTerms(const ContinuousVector* cvNumeratorTerms,
						  ContinuousVector* cvProbs) const;

	////////////////////////////////////////////////////////////////////
	// Compilation de la regle et services associes

	// Compilation redefinie pour l'optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Acces aux valeurs cible
	int GetTargetValueNumber() const;
	Symbol GetTargetValueAt(int nTarget) const;

	// Rang d'une valeur cible (-1 si non trouve)
	int GetTargetValueRank(const Symbol& sValue) const;

	// Acces aux statistiques de grilles en operande
	int GetDataGridStatsNumber() const;
	const KWDRDataGridStats* GetDataGridStatsAt(int nDataGridStatsOrBlock) const;
	int GetDataGridStatsOrBlockNumber() const;
	boolean IsDataGridStatsAt(int nDataGridStatsOrBlock) const;
	const KWDRDataGridStatsBlock* GetDataGridStatsBlockAt(int nDataGridStatsOrBlock) const;
	Continuous GetDataGridWeightAt(int nDataGrid) const;

	// Acces aux statistiques sur l'ensemble des grilles
	int GetDataGridSetTargetPartNumber() const;
	int GetDataGridSetTargetFrequencyAt(int nTarget) const;
	int GetDataGridSetTargetCellIndexAt(int nDataGrid, int nTarget) const;
	double GetMissingLogProbaAt(int nDataGrid, int nTarget) const;
	double GetMissingScoreAt(int nTarget) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Export des noms des variables du classifieur, ainsi que les nom des attribut des DataGrid correspondantes
	// et des regles de preparation dans le cas dense
	// La methode renvoie les informations exploitables au mieux selon la validite de la classe en cours
	// Dans le cas particulier des paires de variables utilises par le predicteur, le nom de la variable
	// associe a la paire est vide, puisqu'il n'y a pas de variable construire associee.
	// Par contre, la variable associee a la grille de preparation est systematiquement presente.
	// Le regle de preparation n'est elle disponible que dans le cas dense
	void ExportAttributeNames(const KWClass* kwcOwnerClass, StringVector* svPredictorAttributeNames,
				  StringVector* svPredictorAttributeDataGridNames,
				  ObjectArray* oaPredictorDenseAttributeDataGridStatsRules) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Acces a la classe avec les operations commons des predicteurs
	friend class KWNaiveBayesPredictorRuleHelper;

	// Calcul du vecteur de probabilites conditionnelles
	void ComputeTargetProbs() const;

	// Verification de la coherence des valeurs cibles d'un DataGrid vis-a-vis a ceux du predicteur
	boolean CheckOperandsCompletenessForTargetValues(const KWClass* kwcOwnerClass, KWDRDataGrid* dataGridRule,
							 int nDataGridStatsOperand, ALString& sVarKey,
							 const KWDRSymbolValueSet* refSymbolValueSet) const;

	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Test si optimisee
	boolean IsOptimized() const;

	// Index du premier operande potentiel de type grille
	//  0 dans cette classe, 1 si un premier operande sert a memoriser les poids
	// (il peut n'y avoir aucun operande de type grille dans le cas ou aucun attribut n'est predictif)
	int nFirstDataGridOperand;

	// Tableau des regles des grilles et blocs de grilles
	ObjectArray oaDataGridStatsAndBlockRules;

	// Index i est true si oaDataGridStatsAndBlockRules[i] est un objet KWDRDataGridStats
	IntVector ivIsDataGridStatsRule;

	// Vecteur des poids pour chaque grille (par defaut: tous les poids a 1)
	ContinuousVector cvWeights;

	// Vecteurs des effectifs par partie cible
	IntVector ivDataGridSetTargetFrequencies;

	// Log-vraisemblances des valeurs manquantes pour chaque cible et grille (variable)
	// Les grilles denses sont mis a zero
	DoubleVector dvMissingLogProbas;

	// Valeurs de reference de la log-likelihood pour chaque cible:
	// Ce sont ceux ou tous les valeurs sparse sont missing
	DoubleVector dvMissingScores;

	// Index des parties cibles de grille pour chaque partie cible de l'ensemble
	// Vecteur de taille nTargetPartNumber * DataGridRuleNumber
	// Dans le cas de plusieurs grilles, les parties resultent de l'union des parties
	// cibles sur l'ensemble des grilles
	IntVector ivDataGridTargetIndexes;

	// Vecteurs des valeurs cibles
	SymbolVector svTargetValues;

	// Dictionnaire des rangs des valeurs cibles, en memorisant le rang+1
	LongintNumericKeyDictionary lnkdTargetValueRanks;

	// Probabilite conditionnelle par defaut; attribue aux classes inconnues
	Continuous cUnknownTargetProb;

	// Effectif total des valeur cibles
	int nTargetTotalFrequency;

	// Gestion des epsilon de Laplace
	double dLaplaceEpsilon;
	double dLaplaceDenominator;

	// Vecteur des numerateurs des log de probabilites conditionnelles, avant normalisation pour obtenir des probabilites
	mutable ContinuousVector cvTargetLogProbNumeratorTerms;

	// Vecteur des probabilites conditionnelles
	mutable ContinuousVector cvTargetProbs;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;

	// Objet du support pour les verifications
	KWNaiveBayesPredictorRuleHelper naiveBayesPredictorRuleHelper;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSNBClassifier
//   Selective Naive Bayes
// Le premier operande est un vecteur de poids des variables
// (KWDRContinuousVector); les autres operandes sont les
// memes que pour un Naive Bayes
class KWDRSNBClassifier : public KWDRNBClassifier
{
public:
	// Constructeur
	KWDRSNBClassifier();
	~KWDRSNBClassifier();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRNBRankRegressor
// Regresseur de rang base sur une ou plusieurs grilles
// (operandes de type KWDRDataGridStats)
class KWDRNBRankRegressor : public KWDRRankRegressor
{
public:
	// Constructeur
	KWDRNBRankRegressor();
	~KWDRNBRankRegressor();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a une objet, et services associes

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Reimplementation de l'interface regresseur de rang
	Continuous ComputeTargetRankMean() const override;
	Continuous ComputeTargetRankStandardDeviation() const override;
	Continuous ComputeTargetRankDensityAt(Continuous cRank) const override;
	Continuous ComputeTargetRankCumulativeProbAt(Continuous cRank) const override;

	////////////////////////////////////////////////////////////////////
	// Compilation de la regle et services associes

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Acces aux statistiques de grilles en operande
	int GetDataGridStatsOrBlockNumber() const;
	int GetDataGridStatsNumber() const;
	boolean IsDataGridStatsAt(int nDataGridStatsOrBlock) const;
	const KWDRDataGridStats* GetDataGridStatsAt(int nDataGridStatsOrBlock) const;
	const KWDRDataGridStatsBlock* GetDataGridStatsBlockAt(int nDataGridStatsOrBlock) const;
	Continuous GetDataGridWeightAt(int nDataGridStatsOrBlock) const;

	// Acces aux statistiques sur l'ensemble des grilles
	int GetDataGridSetTargetValueNumber() const;
	Continuous GetDataGridSetTargetCumulativeFrequencyAt(int nTarget) const;
	int GetDataGridSetTargetIndexAt(int nDataGridStats, int nTarget) const;
	int GetDataGridSetTargetCellIndexAt(int nDataGrid, int nTarget) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	friend class KWDRNBRegressor;

	// Calcul du vecteur de probabilites conditionnelles par intervalle cible
	void ComputeTargetProbs() const;

	/////////////////////////////////////////////////////////////////
	// Methodes de calcul pour les indicateurs de regression de rang

	// Calcul de l'esperance d'une variable, dont les valeurs sont fournies sous la forme
	// d'un vecteur de valeurs cumulees par partie cible
	Continuous ComputeExpectation(const ContinuousVector* cvTargetCumulativeValues) const;

	// Calcul de la fonction de repartition des rangs normalises
	Continuous ComputeRankCumulativeProb(Continuous cNormalizedRank) const;

	// Calcul de la densite sur les rangs, pour un rang normalise (entre 0 et 1)
	Continuous ComputeRankDensity(Continuous cNormalizedRank) const;

	// Calcul de la valeur d'un creneau cumule par partie cible
	// Creneau: 1 jusqu'a un seuil, 0 au dela
	// Cela permet de calculer la fonction de repartition par calcul d'esperance
	void ComputeCumulativeCrenel(ContinuousVector* cvTargetCumulativeValues, Continuous cNormalizedRank) const;

	// Calcul des rangs cumules par partie cible
	void ComputeCumulativeRanks(ContinuousVector* cvTargetCumulativeValues) const;

	// Calcul des carres des rangs cumules par partie cible
	void ComputeCumulativeSquareRanks(ContinuousVector* cvTargetCumulativeValues) const;

	////////////////////////////////////////////
	// Compilation

	// Bufferisation
	void CompileInitializeFrequencyAndCellIndexes();

	// Test si la compilation est optimisee
	boolean IsOptimized() const;

	// Index du premier operande de type grille
	// (0 dans cette classe, 1 si un premier operande sert a memoriser les poids)
	int nFirstDataGridOperand;

	// Tableau des statistiques par grilles
	ObjectArray oaDataGridStatsAndBlockRules;

	// Index i est true si oaDataGridStatsAndBlockRules[i] est un objet KWDRDataGridStatsBlock
	IntVector ivIsDataGridStatsRule;

	// Vecteur des probabilites conditionnelles
	mutable ContinuousVector cvTargetProbs;

	// Vecteur des poids de grille (par defaut: tous les poids a 1)
	ContinuousVector cvWeights;

	// Vecteurs des effectifs cumules par partie cible (S(j) = N.0 + N.1 + ... + N.j)
	ContinuousVector cvDataGridSetTargetCumulativeFrequencies;

	// Index des parties cibles de grille pour chaque partie cible de l'ensemble
	// Vecteur de taille nTargetPartNumber*DataGridRuleNumber
	// Dans le cas de plusieurs grilles, les parties resultent de l'union des parties
	// cibles sur l'ensemble des grilles
	IntVector ivDataGridTargetIndexes;

	// Vecteurs des rangs et de leur carre cumules par partie cible
	ContinuousVector cvTargetCumulativeRanks;
	ContinuousVector cvTargetCumulativeSquareRanks;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;

	// Objet du support pour les verifications
	KWNaiveBayesPredictorRuleHelper naiveBayesPredictorRuleHelper;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSNBRankRegressor
//   Selective Naive Bayes
// Le premier operande est un vecteur de poids des variables
// (KWDRContinuousVector); les autres operandes sont les
// memes que pour un Naive Bayes
class KWDRSNBRankRegressor : public KWDRNBRankRegressor
{
public:
	// Constructeur
	KWDRSNBRankRegressor();
	~KWDRSNBRankRegressor();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRNBRegressor
//   Selective Naive Bayes
// Premier operande: Structure(RankRegressor) depuis un KWDRNBRankRegressor
// Deuxieme operande: distribution des valeurs de l'attribut cible (KWDRDataGrid univarie)
class KWDRNBRegressor : public KWDRRegressor
{
public:
	// Constructeur
	KWDRNBRegressor();
	~KWDRNBRegressor();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	////////////////////////////////////////////////////////////////////
	// Application de la regle a une objet, et services associes

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Reimplementation des services de regression
	Continuous ComputeTargetMean() const override;
	Continuous ComputeTargetStandardDeviation() const override;
	Continuous ComputeTargetDensityAt(Continuous cValue) const override;
	Symbol ComputeTargetQuantileDistribution() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	////////////////////////////////////////////////////////////////////////////////////
	// Services specifiques a la gestion des valeurs cible

	// Calcul de l'index de la valeur la plus proche de la valeur en parametre
	int GetTargetValueIndex(Continuous cValue) const;

	// Acces aux statistiques sur les valeurs (pas de doublons), issues de la compilation
	int GetSingleTargetValueNumber() const;
	Continuous GetSingleTargetValueAt(int nIndex) const;
	int GetSingleTargetValueFrequencyAt(int nIndex) const;
	int GetSingleTargetValueCumulativeFrequencyAt(int nIndex) const;

	/////////////////////////////////////////////////////////////////////////////////////////
	// Methodes de calcul pour les indicateurs de regression de rang

	// Calcul des effectifs cumules des valeurs par index de valeur
	void ComputeSingleTargetValueCumulativeInstanceNumbers(IntVector* ivResultVector) const;

	// Calcul des delta de valeurs pour la gestion des valeurs extremes
	Continuous ComputeTargetValueRange() const;
	Continuous ComputeMeanTargetValueRange() const;

	// Calcul du nombre de valeurs manquantes
	int ComputeMissingValueNumber() const;

	// Calcul des valeurs cumulees par partie cible
	void ComputeCumulativeTargetValues(ContinuousVector* cvResultVector) const;

	// Calcul des carres des rangs cumules par partie cible
	void ComputeCumulativeSquareTargetValues(ContinuousVector* cvResultVector) const;

	// True si la compilation est optimisee
	boolean IsOptimized() const;

	//////////////////////////////////////////////////
	// Objets de travail

	// Effectifs cumules des valeurs par index de valeur
	IntVector ivSingleTargetValueCumulativeInstanceNumbers;

	// Vecteurs des valeurs et de leur carre cumules par partie cible
	ContinuousVector cvCumulativeTargetValues;
	ContinuousVector cvCumulativeSquareTargetValues;

	// Delta de valeur pour la gestion des valeurs extremes
	Continuous cTargetValueRange;
	Continuous cMeanTargetValueRange;

	// Nombre de valeurs manquantes
	// En cas de valeurs manquantes en apprentissage, le regresseur ne predit que des valeurs manquantes
	int nMissingValueNumber;

	// Regresseur de rang
	const KWDRNBRankRegressor* rankRegressorRule;

	// Liste des valeurs en apprentissage, sous forme d'une grille univariee
	const KWDRDataGrid* targetDataGridRule;
	const KWDRContinuousValueSet* targetValuesRules;
	const KWDRFrequencies* targetFrequenciesRule;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRSNBRegressor
//   Selective Naive Bayes
// Premier operande: Structure(RankRegressor) depuis un KWDRSNBRankRegressor
// Deuxieme operande: distribution des valeurs de l'attribut cible (KWDataGrid univariee Continuous)
class KWDRSNBRegressor : public KWDRNBRegressor
{
public:
	// Constructeur
	KWDRSNBRegressor();
	~KWDRSNBRegressor();

	// Creation
	KWDerivationRule* Create() const override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Methodes en inline

inline const ContinuousVector* KWDRNBClassifier::GetTargetProbs() const
{
	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetProbs.GetSize() == GetDataGridSetTargetPartNumber());

	return &cvTargetProbs;
}

inline double KWDRNBClassifier::GetUnknownTargetProb() const
{
	require(IsCompiled());
	require(IsOptimized());
	cUnknownTargetProb;
}

inline const ContinuousVector* KWDRNBClassifier::GetTargetLogProbNumeratorTerms() const
{
	require(IsCompiled());
	require(IsOptimized());
	require(cvTargetLogProbNumeratorTerms.GetSize() == GetDataGridSetTargetPartNumber());

	return &cvTargetLogProbNumeratorTerms;
}

inline int KWDRNBClassifier::GetTargetValueNumber() const
{
	require(IsOptimized());
	return svTargetValues.GetSize();
}

inline Symbol KWDRNBClassifier::GetTargetValueAt(int nTarget) const
{
	require(IsOptimized());
	require(0 <= nTarget and nTarget < svTargetValues.GetSize());
	return svTargetValues.GetAt(nTarget);
}

inline int KWDRNBClassifier::GetTargetValueRank(const Symbol& sValue) const
{
	int nRank;
	require(IsCompiled());
	nRank = (int)lnkdTargetValueRanks.Lookup(sValue.GetNumericKey()) - 1;
	ensure(nRank == -1 or GetTargetValueAt(nRank) == sValue);
	return nRank;
}

inline int KWDRNBClassifier::GetDataGridStatsOrBlockNumber() const
{
	require(IsOptimized());
	return oaDataGridStatsAndBlockRules.GetSize();
}

inline int KWDRNBClassifier::GetDataGridStatsNumber() const
{
	require(IsOptimized());
	return cvWeights.GetSize();
}

inline boolean KWDRNBClassifier::IsDataGridStatsAt(int nDataGridStatsOrBlock) const
{
	require(IsOptimized());
	require(0 <= nDataGridStatsOrBlock and nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize());

	return ivIsDataGridStatsRule.GetAt(nDataGridStatsOrBlock);
}

inline const KWDRDataGridStats* KWDRNBClassifier::GetDataGridStatsAt(int nDataGridStatsOrBlock) const
{
	require(IsOptimized());
	require(0 <= nDataGridStatsOrBlock and nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize());
	require(IsDataGridStatsAt(nDataGridStatsOrBlock));
	return cast(KWDRDataGridStats*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));
}

inline const KWDRDataGridStatsBlock* KWDRNBClassifier::GetDataGridStatsBlockAt(int nDataGridStatsOrBlock) const
{
	require(IsOptimized());
	require(0 <= nDataGridStatsOrBlock and nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize());
	require(not IsDataGridStatsAt(nDataGridStatsOrBlock));
	return cast(KWDRDataGridStatsBlock*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));
}

inline Continuous KWDRNBClassifier::GetDataGridWeightAt(int nDataGrid) const
{
	require(IsOptimized());
	require(0 <= nDataGrid and nDataGrid < cvWeights.GetSize());
	return cvWeights.GetAt(nDataGrid);
}

inline int KWDRNBClassifier::GetDataGridSetTargetPartNumber() const
{
	require(IsOptimized());
	return ivDataGridSetTargetFrequencies.GetSize();
}

inline int KWDRNBClassifier::GetDataGridSetTargetFrequencyAt(int nTarget) const
{
	require(IsOptimized());
	require(0 <= nTarget and nTarget < ivDataGridSetTargetFrequencies.GetSize());
	return ivDataGridSetTargetFrequencies.GetAt(nTarget);
}

inline int KWDRNBClassifier::GetDataGridSetTargetCellIndexAt(int nDataGrid, int nTarget) const
{
	require(IsOptimized());
	require(0 <= nDataGrid and nDataGrid < cvWeights.GetSize());
	require(0 <= nTarget and nTarget < ivDataGridTargetIndexes.GetSize());
	return ivDataGridTargetIndexes.GetAt(nDataGrid * ivDataGridSetTargetFrequencies.GetSize() + nTarget);
}

inline double KWDRNBClassifier::GetMissingLogProbaAt(int nDataGrid, int nTarget) const
{
	require(IsOptimized());
	require(0 <= nDataGrid and nDataGrid < cvWeights.GetSize());
	require(0 <= nTarget and nTarget < ivDataGridTargetIndexes.GetSize());
	return dvMissingLogProbas.GetAt(nDataGrid * ivDataGridSetTargetFrequencies.GetSize() + nTarget);
}

inline double KWDRNBClassifier::GetMissingScoreAt(int nTarget) const
{
	require(IsOptimized());
	require(0 <= nTarget and nTarget < dvMissingScores.GetSize());
	return dvMissingScores.GetAt(nTarget);
}

inline boolean KWDRNBClassifier::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

inline int KWDRNBRegressor::GetTargetValueIndex(Continuous cValue) const
{
	return targetValuesRules->GetContinuousPartIndex(cValue);
}

inline int KWDRNBRegressor::GetSingleTargetValueNumber() const
{
	return targetValuesRules->GetValueNumber();
}

inline Continuous KWDRNBRegressor::GetSingleTargetValueAt(int nIndex) const
{
	return targetValuesRules->GetValueAt(nIndex);
}

inline int KWDRNBRegressor::GetSingleTargetValueFrequencyAt(int nIndex) const
{
	return targetFrequenciesRule->GetFrequencyAt(nIndex);
}

inline int KWDRNBRegressor::GetSingleTargetValueCumulativeFrequencyAt(int nIndex) const
{
	return ivSingleTargetValueCumulativeInstanceNumbers.GetAt(nIndex);
}

inline boolean KWDRNBRegressor::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}

inline int KWDRNBRankRegressor::GetDataGridStatsNumber() const
{
	require(IsOptimized());
	return cvWeights.GetSize();
}

inline const KWDRDataGridStats* KWDRNBRankRegressor::GetDataGridStatsAt(int nDataGridStatsOrBlock) const
{
	require(IsOptimized());
	require(0 <= nDataGridStatsOrBlock and nDataGridStatsOrBlock < oaDataGridStatsAndBlockRules.GetSize());
	require(IsDataGridStatsAt(nDataGridStatsOrBlock));
	return cast(KWDRDataGridStats*, oaDataGridStatsAndBlockRules.GetAt(nDataGridStatsOrBlock));
}

inline Continuous KWDRNBRankRegressor::GetDataGridWeightAt(int nDataGrid) const
{
	require(IsOptimized());
	require(0 <= nDataGrid and nDataGrid < cvWeights.GetSize());
	return cvWeights.GetAt(nDataGrid);
}

inline int KWDRNBRankRegressor::GetDataGridSetTargetValueNumber() const
{
	require(IsOptimized());
	return cvDataGridSetTargetCumulativeFrequencies.GetSize();
}

inline Continuous KWDRNBRankRegressor::GetDataGridSetTargetCumulativeFrequencyAt(int nTarget) const
{
	require(IsOptimized());
	require(0 <= nTarget and nTarget < ivDataGridTargetIndexes.GetSize());
	return cvDataGridSetTargetCumulativeFrequencies.GetAt(nTarget);
}

inline int KWDRNBRankRegressor::GetDataGridSetTargetIndexAt(int nDataGridStats, int nTarget) const
{
	require(IsOptimized());
	require(0 <= nDataGridStats and nDataGridStats < GetDataGridStatsNumber());
	require(0 <= nTarget and nTarget < ivDataGridTargetIndexes.GetSize());
	return ivDataGridTargetIndexes.GetAt(nDataGridStats * cvDataGridSetTargetCumulativeFrequencies.GetSize() +
					     nTarget);
}

inline boolean KWDRNBRankRegressor::IsOptimized() const
{
	return IsCompiled() and nOptimizationFreshness == GetOwnerClass()->GetCompileFreshness();
}
