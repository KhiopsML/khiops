// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDiscretizerSpec.h"
#include "KWClassStats.h"
#include "KWSTDatabaseTextFile.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"
#include "KWStat.h"
#include "KWLoadIndex.h"

///////////////////////////////////////////////////////////////////////
// Classe KWContinuousSampleGenerator
// Generation d'exemple de test numerique pour une classe ContinuousSample(X,Class)
// Classe ancetre, permettant de ne redefinir que la generation des
// instance d'une base de donnees, et des evaluations de performances.
class KWContinuousSampleGenerator : public Object
{
public:
	// Constructeur
	KWContinuousSampleGenerator();
	~KWContinuousSampleGenerator();

	// Nom du generateur
	virtual const ALString GetName() = 0;

	// Generation aleatoire des valeurs d'une instance
	virtual void GenerateObjectValues(KWObject* kwoObject) = 0;

	// Verification de l'integrite d'un objet (par rapport a la classe
	boolean CheckObject(KWObject* kwoObject);

	// Acces a la classe de test des examples numeriques ContinuousSample(X, Class)
	KWClass* GetContinuousSampleClass();

	// Index de chargement de l'attribut Continuous X
	KWLoadIndex GetSourceAttributeLoadIndex() const;

	// Index de chargement l'attribut Symbol Class
	KWLoadIndex GetTargetAttributeLoadIndex() const;

	////////////////////////////////////////////////////////////////////////
	// Evaluation de la qualite d'une discretisation par rapport a la loi reelle

	// Calcul de l'erreur de discretisation en apprentissage
	double ComputeDiscretizationLearningError(KWAttributeStats* kwasDiscretization);

	// Calcul de l'erreur de discretisation en test, par rapport a la distribution
	// theorique des instances
	double ComputeDiscretizationTestError(KWAttributeStats* kwasDiscretization);

	// Calcul de la distance entre la densite de la classe '+' estimee sur les
	// intervalles discretises, et la densite reelle de la classe plus connue
	// a partir de la fonction generatrice des exemples
	double ComputeDiscretizationDistance(KWAttributeStats* kwasDiscretization);

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Calcul de la part du taux d'erreur en test sur un intervalle, defini
	// par ses bornes et sa classe majoritaire
	// La methode a redefinir doit calculer le taux d'erreur theorique, a partir
	// de la fonction generatrice des exemple qui est connue de facon exacte
	virtual double ComputeIntervalTestError(double dLowerBound, double dUpperBound,
						boolean bIsClassPlusMajoritary) = 0;

	// Calcul de la part de la distance a la densite de la classe '+' sur un
	// intervalle, defini par ses bornes et sa probabilite estimee de '+'
	// La methode a redefinir doit calculer la distance theorique, a partir
	// de la fonction generatrice des exemple qui est connue de facon exacte
	virtual double ComputeIntervalDistance(double dLowerBound, double dUpperBound, double dClassPlusProb) = 0;

	// Generateur aleatoire
	double GetRandomDouble();

	// Index de chargement des attributs source et cible
	KWLoadIndex liSourceAttributeLoadIndex;
	KWLoadIndex liTargetAttributeLoadIndex;

	// Memorisation des symboles des classes cibles
	Symbol sClassPlus;
	Symbol sClassMinus;
};

///////////////////////////////////////////////////////////////////////
// Classe KWCSGRandom
// Les exemples sont generes avec X au hazard  sur [0, 1] et Class
// independant de X, equistribue sur {'-', '+'}
class KWCSGRandom : public KWContinuousSampleGenerator
{
public:
	// Constructeur
	KWCSGRandom();
	~KWCSGRandom();

	// Redefinition des methodes virtuelles
	const ALString GetName() override;
	void GenerateObjectValues(KWObject* kwoObject) override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double ComputeIntervalTestError(double dLowerBound, double dUpperBound,
					boolean bIsClassPlusMajoritary) override;
	double ComputeIntervalDistance(double dLowerBound, double dUpperBound, double dClassPlusProb) override;
};

///////////////////////////////////////////////////////////////////////
// Classe KWCSGSinusSign
// Les exemples sont generes avec X au hazard  sur [0, 1] et Class
// equistribue sur {'-', '+'}, de valeur Class = Sign(Sin(k.2.pi.X)
// Un bruit uniforme peut egalement etre ajoute
class KWCSGSinusSign : public KWContinuousSampleGenerator
{
public:
	// Constructeur
	KWCSGSinusSign();
	~KWCSGSinusSign();

	// Frequence des sinusoides: par defaut: 1
	void SetSinusoidFrequency(int nValue);
	int GetSinusoidFrequency() const;

	// Probabilite de bruit: par defaut: 0
	// (proportion des exemples dont la Class est generee au hasard)
	void SetNoiseProb(double dValue);
	double GetNoiseProb() const;

	// Redefinition des methodes virtuelles
	const ALString GetName() override;
	void GenerateObjectValues(KWObject* kwoObject) override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double ComputeIntervalTestError(double dLowerBound, double dUpperBound,
					boolean bIsClassPlusMajoritary) override;
	double ComputeIntervalDistance(double dLowerBound, double dUpperBound, double dClassPlusProb) override;

	// Projection d'une valeur sur un intervalle
	// La valeur est projetee sur la borne de l'intervalle la plus proche si
	// elle est exterieure a l'intervalle, sinon est reste inchangee
	double GetIntervalProjection(double dLowerBound, double dUpperBound, double dValue);

	// Parametre de la fonction generatrice des exemples
	int nSinusoidFrequency;
	double dNoiseProb;
};

///////////////////////////////////////////////////////////////////////
// Classe KWCSGLinearProb
// Les exemples sont generes avec X au hazard  sur [0, 1] et Class
// equistribue (en moyenne) sur {'-', '+'}, de valeur Class tel que
// Prob(Class="+")=X
class KWCSGLinearProb : public KWContinuousSampleGenerator
{
public:
	// Constructeur
	KWCSGLinearProb();
	~KWCSGLinearProb();

	// Redefinition des methodes virtuelles
	const ALString GetName() override;
	void GenerateObjectValues(KWObject* kwoObject) override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double ComputeIntervalTestError(double dLowerBound, double dUpperBound,
					boolean bIsClassPlusMajoritary) override;
	double ComputeIntervalDistance(double dLowerBound, double dUpperBound, double dClassPlusProb) override;
};

///////////////////////////////////////////////////////////////////////
// Classe KWContinuousSampleDiscretizerTest
// Test d'une methode de discretisation sur pour une generateur de jeu
// de donnees synthetiques
class KWContinuousSampleDiscretizerTest : public Object
{
public:
	// Constructeur
	KWContinuousSampleDiscretizerTest();
	~KWContinuousSampleDiscretizerTest();

	///////////////////////////////////////////////////////////////////////
	// Parametrage du generateur

	// Parametrage du generateur
	void SetGenerator(KWContinuousSampleGenerator* kwcsgSampleGenerator);
	KWContinuousSampleGenerator* GetGenerator();

	// Parametrage du discretiser
	void SetDiscretizer(KWDiscretizerSpec* kwdsDiscretizerSpec);
	KWDiscretizerSpec* GetDiscretizer();

	// Taille des echantillons
	void SetSampleSize(int nValue);
	int GetSampleSize();

	// Nombre d'echantillons
	void SetSampleNumber(int nValue);
	int GetSampleNumber();

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

	// Tailles des discretisation
	DoubleVector* GetNoInformativeAttributeNumbers();
	DoubleVector* GetDiscretizationSizes();

	// Erreur en apprentissage
	DoubleVector* GetLearningErrors();

	// Erreur en Test
	DoubleVector* GetTestErrors();

	// Distance entre la loi de la calsse '+' estimee et la loi reelle
	DoubleVector* GetDistances();

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
	KWContinuousSampleGenerator* sampleGenerator;
	KWDiscretizerSpec* discretizerSpec;
	int nSampleSize;
	int nSampleNumber;
	int nFreshness;
	int nStatsFreshness;
	int nDiscretizerStatsFreshness;

	// Attrubuts de resultats
	DoubleVector dvNoInformativeAttributeNumbers;
	DoubleVector dvDiscretizationSizes;
	DoubleVector dvLearningErrors;
	DoubleVector dvTestErrors;
	DoubleVector dvDistances;
};

///////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerTest
// Tests de discretisation
class KWDiscretizerTest : public Object
{
public:
	// Constructeur
	KWDiscretizerTest();
	~KWDiscretizerTest();

	/////////////////////////////////////////////////////////////////////////
	// Test effectues pour differentes tailles d'echantillon et pour
	// plusieurs methodes de discretisation

	// Test du seuil de detection d'une sequence pure en tete d'un echantillon
	// pour deux classes cibles equidistribuees et en faisant varier la
	// frequence de la premiere classe
	static void TestHeadPureInterval();
	static void TestUnbalancedHeadPureInterval();

	// Test du seuil de detection d'une sequence pure au milieu d'un echantillon
	// pour deux classes cibles equidistribuees et en faisant varier la
	// frequence de la premiere classe
	static void TestCenterPureInterval();
	static void TestUnbalancedCenterPureInterval();

	// Test du seuil de detection d'une paire alternee sequence pure au milieu d'un echantillon
	// pour deux classes cibles equidistribuees
	static void TestCenterTwoPureIntervals();

	// Test d'echantillons genere au hazard
	static void TestRandomSamples();

	// CH V9
	// static void TestRandomSamplesForPriorV9();
	// Fin CH V9

	// Test d'echantillons genere avec le generateur SinusSign
	static void TestSinusSignSamples();

	// CH V9
	// Comparaison du discretiseur MODL avant refonte, apres refonte sans granularite, apres refonte
	// Test d'echantillons genere avec le generateur SinusSign
	// static void TestSinusSignSamplesForNewPriorV9();
	// Fin CH V9

	// Test d'echantillons genere avec le generateur LinearProb
	static void TestLinearProbSamples();

	// CH V9
	// Test d'echantillons avec cible binaire et une variable explicative numerique.
	// Toutes les configurations possibles de 0 et de 1 sont generees exhaustivement
	// On recense le nombre de configurations retenues comme informatives selon le modele de cout utilise
	// avant la refonte des prior, apres la refonte sans la prise en compte de la granularite, avec la granularite
	// static void TestNewPriorThreshold();

	// Comparaison du discretiseur MODL avant refonte, apres refonte sans granularite, apres refonte
	// Test d'echantillons genere avec le generateur LinearProb
	static void TestLinearProbSamplesForNewPriorV9();
	// Fin CH V9

	// Tous les tests
	static void TestAll();

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Initialisation d'un table de contingence, pour chaque type de test
	static void InitializeHeadPureIntervalTable(KWFrequencyTable* table, int nSampleSize, int nFirstClassFrequency,
						    int nIntervalSize);
	static void InitializeCenterPureIntervalTable(KWFrequencyTable* table, int nSampleSize,
						      int nFirstClassFrequency, int nIntervalSize);
	static void InitializeCenterTwoPureIntervalsTable(KWFrequencyTable* table, int nSampleSize,
							  int nFirstClassFrequency, int nIntervalSize);

	// CH V9
	static void InitializeBinaryIntegerTable(KWFrequencyTable* table, int nInteger, int nRepresentationSize);
	// Fin CH V9

	// Creation des specifications des methodes de discretisation a tester
	static void InitializeDiscretizerSpecArray(ObjectArray* oaDiscretizerSpecs);
};
