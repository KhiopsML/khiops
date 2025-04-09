// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWClassStats.h"
#include "KWSTDatabaseTextFile.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"
#include "KWStat.h"
#include "KWAttributeSubsetStats.h"
#include "KWProbabilityTable.h"
#include "KWDRDataGrid.h"
#include "KWLoadIndex.h"

///////////////////////////////////////////////////////////////////////
// Classe KWSampleGenerator
// Generation d'exemple pour une classe Sample(X1, X2,...,Class)
// Classe ancetre, permettant de ne redefinir que la generation des
// instance d'une base de donnees, et des evaluations de performances.
class KWSampleGenerator : public Object
{
public:
	// Constructeur
	KWSampleGenerator();
	~KWSampleGenerator();

	// Nom du generateur
	virtual const ALString GetName() const = 0;

	// Nom du generateur, avec types des variables
	virtual ALString GetFullName() const;

	// Types des attributs descriptifs (Symbol ou Continuous)
	const IntVector* GetAttributeTypes() const;

	// Nombre de valeurs des attributs symboliques en entree
	void SetModalityNumber(int nValue);
	int GetModalityNumber() const;

	// Taux de bruit: pourcentage des exemples avec affectation aleatoire de la classe
	void SetNoiseRate(double dValue);
	double GetNoiseRate() const;

	// Bruitage aleatoire de la classe d'un objet selon le taux de bruit
	void NoisifyObjectClass(KWObject* kwoObject);

	// Generation aleatoire des valeurs d'une instance
	// A reimplementer (sans le bruitage de la classe, qui est appele par ailleur)
	virtual void GenerateObjectValues(KWObject* kwoObject) = 0;

	// Generation aleatoire des valeurs d'une instance, avec bruitage de la classe
	void GenerateObjectValuesAndNoisyClass(KWObject* kwoObject);

	// Verification de l'integrite d'un objet (par rapport a la classe)
	boolean CheckObject(KWObject* kwoObject) const;

	// Acces a la classe de test des examples numeriques Sample(X1, X2,..., Class)
	// Index de l'attribut Continuous X1: 0
	// Index de l'attribut Symbol Class: K-1
	KWClass* GetSampleClass() const;

	////////////////////////////////////////////////////////////////////////
	// Evaluation de la qualite d'une grille de donnees par rapport a la loi reelle
	// L'utilisation d'une grille permet d'associer a chaque objet une prediction de la
	// classe C_p et de sa probabilite conditionnelle Prob_p(C_p), a comparer
	// avec la classe majoritaire reelle C et sa proba Prob(C)
	// On doit prealablement initialiser la classe a partir d'un objet KWAttributeSubsetStats

	// Verification de l'integrite d'une grille de donnees (par rapport a la classe)
	boolean CheckAttributeSubsetStats(KWAttributeSubsetStats* attributeSubsetStats) const;

	// Initialisation a partir des statistiques de preparation multivariees
	void InitFromAttributeSubsetStats(KWAttributeSubsetStats* attributeSubsetStats);
	boolean IsAttributeSubsetStatsInitialized() const;

	// Evaluation de l'erreur de prediction sur un objet
	// Error = 0 si classe predite correcte, 0 sinon
	double ComputeObjectError(KWObject* kwoObject);

	// Evaluation de la divergence de Kulback-Leibler sur un objet
	// DKL = sum_j{log(Prob(j)/Prob_p(j))}
	double ComputeObjectDKL(KWObject* kwoObject);

	// Evaluation de la Mean Square Error entre les proba estimees et vraies sur un objet
	// MSE = (Prob(C)-Prob_p(C))^2
	double ComputeObjectMSE(KWObject* kwoObject);

	// Moyenne des evaluations sur toute la base
	double ComputeDatabaseError(KWDatabase* testDatabase);
	double ComputeDatabaseDKL(KWDatabase* testDatabase);
	double ComputeDatabaseMSE(KWDatabase* testDatabase);

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Index de l'attribut de classe
	KWLoadIndex GetClassLoadIndex();

	// Calcul de la classe predite et des probabilites de prediction par classe par une grille de donnees
	Symbol ComputePredictedClass(KWObject* kwoObject);
	double ComputePredictedProb(KWObject* kwoObject, const Symbol& sClass);

	// Calcul de la classe reelle, et de sa probabilite de prediction par classe, en tenant compte taux de bruit
	Symbol ComputeNoisyTrueMajorityClass(KWObject* kwoObject);
	double ComputeNoisyTrueProb(KWObject* kwoObject, const Symbol& sClass);

	// Calcul des probabilites de prediction par classe (a reimplementer, sans bruitage de classe)
	virtual double ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass) = 0;

	// Generateur aleatoire
	double GetRandomDouble();

	// Acces aux valeurs des classes cibles
	static Symbol GetPlusClass();
	static Symbol GetMinusClass();

	// Acces aux modalites sources
	Symbol GetModalityAt(int nIndex);

	// Type des attributs (a initialiser dans le constructeur)
	IntVector ivAttributeTypes;

	// Index de chargement des des attributs
	mutable KWLoadIndexVector livAttributeLoadIndexes;
	mutable KWLoadIndex liTargetAttributeLoadIndex;

	// Nombre de modalites sources
	int nModalityNumber;

	// Taux de bruit
	double dNoiseRate;

	// Donnees de travail pour le calcul des indicateurs de performance
	KWDataGridStats* preparedDataGridStats;
	KWProbabilityTable* probabilityTable;
	KWDRCellIndex* cellIndexRule;
	double dGlobalPlusProb;
	double dGlobalMinusProb;
	int nMinusIndex;
	int nPlusIndex;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSGRandomContinuous
// Les exemples sont generes avec X uniformement sur [0, 1] et Class
// independant de X, equistribue sur {'-', '+'}
class KWSGRandomContinuous : public KWSampleGenerator
{
public:
	// Constructeur
	KWSGRandomContinuous();
	~KWSGRandomContinuous();

	// Redefinition des methodes virtuelles
	const ALString GetName() const override;
	void GenerateObjectValues(KWObject* kwoObject) override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass) override;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSGRandomSymbol
// Les exemples sont generes avec X uniformement sur les valeurs et Class
// independant de X, equistribue sur {'-', '+'}
class KWSGRandomSymbol : public KWSampleGenerator
{
public:
	// Constructeur
	KWSGRandomSymbol();
	~KWSGRandomSymbol();

	// Redefinition des methodes virtuelles
	const ALString GetName() const override;
	void GenerateObjectValues(KWObject* kwoObject) override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass) override;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSGChessBoardSymbol
// Les exemples sont generes avec X uniformement sur les valeurs et Class
// dependant de X sur un damier (Class '-', '+' dependant de la parite
// de la somme des indices de modalites)
class KWSGChessBoardSymbol : public KWSampleGenerator
{
public:
	// Constructeur
	KWSGChessBoardSymbol();
	~KWSGChessBoardSymbol();

	// Redefinition des methodes virtuelles
	const ALString GetName() const override;
	void GenerateObjectValues(KWObject* kwoObject) override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass) override;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSGChessBoardContinuous
// Les exemples sont generes avec X uniformement sur les valeurs et Class
// dependant de X sur un damier (Class '-', '+' dependant de la parite
// de la somme des indices des intervalles)
// Le nombre d'intervalles (equidistribues) est fixe en utilisant ModalityNumber
class KWSGChessBoardContinuous : public KWSampleGenerator
{
public:
	// Constructeur
	KWSGChessBoardContinuous();
	~KWSGChessBoardContinuous();

	// Redefinition des methodes virtuelles
	const ALString GetName() const override;
	void GenerateObjectValues(KWObject* kwoObject) override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass) override;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSGMultivariateXORContinuous
// Les exemples sont generes avec X uniformement sur les valeurs et Class
// dependant de X sur un XOR (Class '-', '+' dependant de la parite
// de la somme des indices des intervalles) pour une sous partie des variables
class KWSGMultivariateXORContinuous : public KWSampleGenerator
{
public:
	// Constructeur
	KWSGMultivariateXORContinuous();
	~KWSGMultivariateXORContinuous();

	// Randomisation des variables du XOR
	void RandomizeXORVariables();

	// Nombre total de variables en entree
	void SetInputVariableNumber(int nValue);
	int GetInputVariableNumber() const;

	// Nombre de variables en entree interveant dans le XOR
	void SetXORVariableNumber(int nValue);
	int GetXORVariableNumber() const;

	// Redefinition des methodes virtuelles
	const ALString GetName() const override;
	ALString GetFullName() const override;
	void GenerateObjectValues(KWObject* kwoObject) override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass) override;

	// Parametres de generation
	int nInputVariableNumber;
	int nXORVariableNumber;

	// Variables intervenant dans le XOR
	IntVector ivXORVariableIndexes;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSGGaussianMixture
// Les exemples sont generes selon une Gaussienne pour chaque classe
class KWSGGaussianMixture : public KWSampleGenerator
{
public:
	// Constructeur
	KWSGGaussianMixture();
	~KWSGGaussianMixture();

	// Redefinition des methodes virtuelles
	const ALString GetName() const override;
	void GenerateObjectValues(KWObject* kwoObject) override;

	// Parametres de la Gaussienne pour la classe '-' (par defaut: moyenne = (-1 -1) et ecart type = (1 1))
	void SetMinusClassXMean(double dValue);
	double GetMinusClassXMean() const;
	void SetMinusClassYMean(double dValue);
	double GetMinusClassYMean() const;
	void SetMinusClassXStandardDeviation(double dValue);
	double GetMinusClassXStandardDeviation() const;
	void SetMinusClassYStandardDeviation(double dValue);
	double GetMinusClassYStandardDeviation() const;

	// Parametres de la Gaussienne pour la classe '+' (par defaut: moyenne = (-1 -1) et ecart type = (1 1))
	void SetPlusClassXMean(double dValue);
	double GetPlusClassXMean() const;
	void SetPlusClassYMean(double dValue);
	double GetPlusClassYMean() const;
	void SetPlusClassXStandardDeviation(double dValue);
	double GetPlusClassXStandardDeviation() const;
	void SetPlusClassYStandardDeviation(double dValue);
	double GetPlusClassYStandardDeviation() const;

	//////////////////////////////////////////////////////////////////////
	// Evaluation de la qualite d'un estimateur parametrique
	// Methodes identiques a celle de KWSampleGenerator en remplacant
	// l'estimateur non parametrique par grille de donnes par
	// un estimateur parametrique par melange de Gaussiennes (GM)

	// Apprentissage des parametres des Gaussiennes
	void ComputeGaussianParameters(KWDatabase* trainDatabase);

	// Evaluation de l'erreur de prediction sur un objet
	// Error = 0 si classe predite correcte, 0 sinon
	double ComputeObjectGMError(KWSGGaussianMixture* gaussianMixture, KWObject* kwoObject);

	// Evaluation de la divergence de Kulback-Leibler sur un objet
	// DKL = sum_j{log(Prob(j)/Prob_p(j))}
	double ComputeObjectGMDKL(KWSGGaussianMixture* gaussianMixture, KWObject* kwoObject);

	// Evaluation de la Mean Square Error entre les proba estimees et vraies sur un objet
	// MSE = (Prob(C)-Prob_p(C))^2
	double ComputeObjectGMMSE(KWSGGaussianMixture* gaussianMixture, KWObject* kwoObject);

	// Moyenne des evaluations sur toute la base
	double ComputeDatabaseGMError(KWSGGaussianMixture* gaussianMixture, KWDatabase* testDatabase);
	double ComputeDatabaseGMDKL(KWSGGaussianMixture* gaussianMixture, KWDatabase* testDatabase);
	double ComputeDatabaseGMMSE(KWSGGaussianMixture* gaussianMixture, KWDatabase* testDatabase);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double ComputeTrueProb(KWObject* kwoObject, const Symbol& sClass) override;

	// Parametres des gaussiennes
	double dMinusClassXMean;
	double dMinusClassYMean;
	double dMinusClassXStandardDeviation;
	double dMinusClassYStandardDeviation;
	double dPlusClassXMean;
	double dPlusClassYMean;
	double dPlusClassXStandardDeviation;
	double dPlusClassYStandardDeviation;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSampleDataGridTest
// Test d'une methode de grille de donnees sur pour une generateur de jeu
// de donnees synthetiques
class KWSampleDataGridTest : public Object
{
public:
	// Constructeur
	KWSampleDataGridTest();
	~KWSampleDataGridTest();

	///////////////////////////////////////////////////////////////////////
	// Parametrage du generateur

	// Parametrage du generateur
	void SetGenerator(KWSampleGenerator* kwcsgSampleGenerator);
	KWSampleGenerator* GetGenerator();

	// Parametrage d'optimisation de la grille de donnees
	void SetDataGridOptimizerParameters(KWDataGridOptimizerParameters* optimizationParameters);
	KWDataGridOptimizerParameters* GetDataGridOptimizerParameters();

	// Taille des echantillons
	void SetSampleSize(int nValue);
	int GetSampleSize() const;

	// Nombre d'echantillons
	void SetSampleNumber(int nValue);
	int GetSampleNumber() const;

	// Taille de la base de test
	void SetTestDatabaseSize(int nValue);
	int GetTestDatabaseSize() const;

	// Index d'un exemple a exporter (par defaut: -1 pour ne rien exporter)
	void SetSampleExportNumber(int nValue);
	int GetSampleExportNumber() const;

	// Verification de l'integrite des specifications
	boolean Check() const override;

	//////////////////////////////////////////////////////////
	// Calcul des statistiques de discretisation

	// Calcul des statistiques
	boolean ComputeStats();

	// Indique si les stats sont calculees
	boolean IsStatsComputed() const;

	///////////////////////////////////////////////////////////
	// Acces aux resultats sous forme de vecteurs de resultat,
	// indexes par le nombre d'echantillons

	// Erreur en Test
	DoubleVector* GetErrors();
	DoubleVector* GetGMErrors();

	// Divergence de Kullback-Leibler entre la loi relle et la loi predite
	DoubleVector* GetDKLs();
	DoubleVector* GetGMDKLs();

	// Mean square error entre la loi relle et la loi predite
	DoubleVector* GetMSEs();
	DoubleVector* GetGMMSEs();

	// Cout de grille
	DoubleVector* GetCosts();

	// Temps de calcul
	DoubleVector* GetTimes();

	// Nombre de cellules des grilles de donnees
	DoubleVector* GetCellNumbers();

	// Nombre d'attributs actifs des grilles de donnes (actif: ayant au moins deux parties)
	DoubleVector* GetNoInformativeAttributeNumbers();
	DoubleVector* GetInformativeAttributeNumbers();

	// Nombre de parties des partition univariees
	DoubleVector* GetPartNumbersAt(int nAttribute);

	////////////////////////////////////////////////////////
	// Ecriture de rapport

	// Ecriture d'un rapport detaille
	void WriteReportFile(const ALString& sFileName);
	void WriteReport(ostream& ost);

	// Rapport synthetique destine a rentrer dans un tableau
	void WriteHeaderLineReport(ostream& ost);
	void WriteLineReport(ostream& ost);

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Generation d'un echantillon
	// La base passee en parametre doit etre correctement initialisee.
	// Ses objets charges en memoire sont potentiellement reutilises pour
	// chaque nouvelle echantilon
	void GenerateSample(KWDatabase* database);

	// Attributs de specification
	KWSampleGenerator* sampleGenerator;
	KWDataGridOptimizerParameters* dataGridOptimizationParameters;
	int nSampleSize;
	int nSampleNumber;
	int nTestDatabaseSize;
	int nFreshness;
	int nStatsFreshness;
	int nOptimizationParametersFreshness;
	int nSampleExportNumber;

	// Attributs de resultats
	DoubleVector dvErrors;
	DoubleVector dvDKLs;
	DoubleVector dvMSEs;
	DoubleVector dvCosts;
	DoubleVector dvTimes;
	DoubleVector dvCellNumbers;
	DoubleVector dvNoInformativeAttributeNumbers;
	DoubleVector dvInformativeAttributeNumbers;
	DoubleVector dvGMErrors;
	DoubleVector dvGMDKLs;
	DoubleVector dvGMMSEs;
	ObjectArray oaUnivariatePartitionSizes;
};

///////////////////////////////////////////////////////////////////////
// Classe KWDataGridTest
// Tests de discretisation
class KWDataGridTest : public Object
{
public:
	// Constructeur
	KWDataGridTest();
	~KWDataGridTest();

	/////////////////////////////////////////////////////////////////////////
	// Test effectues pour differentes tailles d'echantillon et pour
	// plusieurs methodes de discretisation

	// Test d'echantillons pour un generateur donne
	void TestSamples(const ALString& sSampleGeneratorName, int nUnivariatePartNumber, double dNoiseRate,
			 int nMinSampleSize, int nMaxSampleSize, int nSampleNumber, int nTestDatabaseSize,
			 int nSampleExportNumber);

	// Lancement de tests depuis la ligne de commande
	static void DataGridTest(int argc, char** argv);

	// Etude dans le cas non asymptotique
	static void TestNonAsymptotic();

	// Tous les tests
	static void Test();

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Creation des specifications des parametrages d'optimisation
	void InitializeOptimizationParametersArray();

	// Recherche d'un generateur par son nom
	// Renvoie NULL si non trouve
	// Memoire: a liberer par l'appelant
	KWSampleGenerator* LookupSampleGenerator(const ALString& sName);

	// Tailles des echantillons
	IntVector ivSampleSizes;

	// Parametrages d'optimisation
	ObjectArray oaOptimizationParameters;
};

///// Methodes en inline

inline Symbol KWSampleGenerator::GetPlusClass()
{
	static Symbol sPlusClass = Symbol("+");
	return sPlusClass;
}

inline Symbol KWSampleGenerator::GetMinusClass()
{
	static Symbol sMinusClass = Symbol("-");
	return sMinusClass;
}

inline Symbol KWSampleGenerator::GetModalityAt(int nIndex)
{
	static ALString sModalityPrefix = "v";
	require(0 <= nIndex and nIndex < nModalityNumber);
	return Symbol(sModalityPrefix + IntToString(nIndex));
}

inline KWLoadIndex KWSampleGenerator::GetClassLoadIndex()
{
	return liTargetAttributeLoadIndex;
}
