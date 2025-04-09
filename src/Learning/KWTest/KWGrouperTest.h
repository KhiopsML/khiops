// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWFrequencyVector.h"
#include "KWGrouperSpec.h"
#include "KWClassStats.h"
#include "KWSTDatabaseTextFile.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"
#include "KWStat.h"
#include "KWLoadIndex.h"

///////////////////////////////////////////////////////////////////////
// Classe KWSymbolSampleGenerator
// Generation d'exemple de test symbolique pour une classe SymbolSample(X,Class)
// Classe ancetre, permettant de ne redefinir que la generation des
// instance d'une base de donnees, et des evaluations de performances.
class KWSymbolSampleGenerator : public Object
{
public:
	// Constructeur
	KWSymbolSampleGenerator();
	~KWSymbolSampleGenerator();

	// Nom du generateur
	virtual const ALString GetName() const = 0;

	// Nombre de modalite sources (defaut: 2)
	void SetSourceModalityNumber(int nValue);
	int GetSourceModalityNumber() const;

	// Nombre de classes cibles (defaut: 2)
	void SetTargetClassNumber(int nValue);
	int GetTargetClassNumber() const;

	// Generation aleatoire des valeurs d'une instance
	void GenerateObjectValues(KWObject* kwoObject);

	// Verification de l'integrite d'un objet (par rapport a la classe)
	boolean CheckObject(KWObject* kwoObject);

	// Acces a la classe de test des examples numeriques SymbolSample(X, Class)
	KWClass* GetSymbolSampleClass();

	// Index de chargement de l'attribut Symbol X
	KWLoadIndex GetSourceAttributeLoadIndex() const;

	// Index de chargement l'attribut Symbol Class
	KWLoadIndex GetTargetAttributeLoadIndex() const;

	////////////////////////////////////////////////////////////////////////
	// Evaluation de la qualite d'une groupage par rapport a la loi reelle

	// Calcul de l'erreur de groupage en apprentissage
	double ComputeGroupingLearningError(KWAttributeStats* kwasGrouping);

	// Calcul de l'erreur de groupage en test, par rapport a la distribution
	// theorique des instances
	double ComputeGroupingTestError(KWAttributeStats* kwasGrouping);

	// Calcul de la distance entre les densites reelles des classes connues
	// a partir de la fonction generatrice des exemples et les densites des
	// classes estimees en apprentissage sur les modalites groupees
	double ComputeGroupingDistance(KWAttributeStats* kwasGrouping);

	//////////////////////
	// Methodes standard

	// Affichage des distribution de probabilites
	void Write(ostream& ost) const override;

	// Methode de verification de la coherence des parametres de generations
	boolean Check() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	///////////////////////////////////////////////////////////////////////
	// Methode a reimplementer pour specifier la distribution des modalites
	// sources et des classes cibles pour chaque modalite source

	// Probabilite d'une modalite source donnee
	virtual double GetSourceModalityProbAt(int nSourceModalityIndex) const = 0;

	// Probabilite d'une classe cible pour une probabilite source donnee
	virtual double GetTargetClassConditionalProbAt(int nSourceModalityIndex, int nTargetClassIndex) const = 0;

	////////////////////////////////////////////////////////////////////////
	// Methodes utilitaires de generation

	// Compilation du generateur
	void CompileGenerator() const;
	boolean IsGeneratorCompiled() const;

	// Acces a une modalite source
	Symbol GetSourceModalityAt(int nIndex) const;

	// Acces a une classe cible
	Symbol GetTargetClassAt(int nIndex) const;

	// Recherche de l'index d'une modalite source (-1 si non trouve)
	int GetSourceModalityIndex(const Symbol& sValue);

	// Recherche de l'index d'une classe cible (-1 si non trouve)
	int GetTargetClassIndex(const Symbol& sValue);

	// Generation d'une modalite source
	int GenerateSourceModalityIndex(double dRandom);

	// Generation d'une classe cible pour une modalite source donnee
	int GenerateTargetClassIndex(int nSourceModalityIndex, double dRandom);

	// Generateur aleatoire
	double GetRandomDouble() const;

	// Nombre de modalites et de classes
	int nSourceModalityNumber;
	int nTargetClassNumber;

	// Index de chargement des attributs source et cible
	KWLoadIndex liSourceAttributeLoadIndex;
	KWLoadIndex liTargetAttributeLoadIndex;

	// Vecteur des probabilites cumulees des modalites sources
	mutable DoubleVector dvSourceModalityCumulatedProbs;

	// Tableau des vecteur des probabilites cumulees des classes cibles
	// pour chaque modlaite source
	mutable ObjectArray oaTargetClassCumulatedConditionalProbs;

	// Memorisation des symboles des modalites sources et cibles
	mutable SymbolVector sSourceModalities;
	mutable SymbolVector sTargetClasses;

	// Flag de compilation
	mutable boolean bIsGeneratorCompiled;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSSGRandom
// Les exemples sont generes avec les modalites sources uniformement
// distribues et les classes cibles uniformement distribuees
// independamment des modalites sources
class KWSSGRandom : public KWSymbolSampleGenerator
{
public:
	// Constructeur
	KWSSGRandom();
	~KWSSGRandom();

	// Redefinition des methodes virtuelles
	const ALString GetName() const override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double GetSourceModalityProbAt(int nSourceModalityIndex) const override;
	double GetTargetClassConditionalProbAt(int nSourceModalityIndex, int nTargetClassIndex) const override;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSSGHiddenGroup
// Il y a ici:
//    . SourceModalityNumber modalites sources equidistribuee
//    . NoiseRatioNumber nombre de niveaux de bruits equidistribues sur [0, 1]
//    . TargetClassNumber classes cibles
// Chaque classe cible est associee directement a NoiseRatioNumber groupes
// de niveaux de bruits, ceux ci associes a
// SourceModalityNumber/NoiseRatioNumber*TargetClassNumber modalites sources.
// En cas de bruit, une proportion des instance est classifiee
// uniformement sur les toutes les classes cibles
class KWSSGHiddenGroup : public KWSymbolSampleGenerator
{
public:
	// Constructeur
	KWSSGHiddenGroup();
	~KWSSGHiddenGroup();

	// Ratio de frequence entre les modalites sources les plus frequentes et
	// les plus rares, celles ci suivant une progression geometrique (defaut: 1)
	void SetSourceFrequencyRatio(double dValue);
	double GetSourceFrequencyRatio() const;

	// Nombre de groupes de niveaux de bruit (defaut: 1)
	void SetNoiseRateNumber(int nValue);
	int GetNoiseRateNumber() const;

	// Redefinition des methodes virtuelles
	const ALString GetName() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Redefinition des methodes virtuelles
	double GetSourceModalityProbAt(int nSourceModalityIndex) const override;
	double GetTargetClassConditionalProbAt(int nSourceModalityIndex, int nTargetClassIndex) const override;

	// Attributs
	double dSourceFrequencyRatio;
	int nNoiseRateNumber;
};

///////////////////////////////////////////////////////////////////////
// Classe KWSymbolSampleGrouperTest
// Test d'une methode de groupage sur pour une generateur de jeu
// de donnees synthetiques
class KWSymbolSampleGrouperTest : public Object
{
public:
	// Constructeur
	KWSymbolSampleGrouperTest();
	~KWSymbolSampleGrouperTest();

	///////////////////////////////////////////////////////////////////////
	// Parametrage du generateur

	// Parametrage du generateur
	void SetGenerator(KWSymbolSampleGenerator* kwssgSampleGenerator);
	KWSymbolSampleGenerator* GetGenerator();

	// Parametrage du discretiser
	void SetGrouper(KWGrouperSpec* kwgsGrouperSpec);
	KWGrouperSpec* GetGrouper();

	// Taille des echantillons
	void SetSampleSize(int nValue);
	int GetSampleSize();

	// Nombre d'echantillons
	void SetSampleNumber(int nValue);
	int GetSampleNumber();

	// Verification de l'integrite des specifications
	boolean Check() const override;

	//////////////////////////////////////////////////////////
	// Calcul des statistiques de groupage

	// Calcul des statistiques
	boolean ComputeStats();

	// Indique si les stats sont calculees
	boolean IsStatsComputed() const;

	///////////////////////////////////////////////////////////
	// Acces aux resultats sous forme de vecteurs de resultat,
	// indexes par le nombre d'echantillons

	// Tailles des groupage
	DoubleVector* GetGroupingSizes();

	// Erreur en apprentissage
	DoubleVector* GetLearningErrors();

	// Erreur en Test
	DoubleVector* GetTestErrors();

	// Distance entre la loi estimee et la loi reelle
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
	KWSymbolSampleGenerator* sampleGenerator;
	KWGrouperSpec* grouperSpec;
	int nSampleSize;
	int nSampleNumber;
	int nFreshness;
	int nStatsFreshness;
	int nGrouperStatsFreshness;

	// Attributs de resultats
	DoubleVector dvGroupingSizes;
	DoubleVector dvLearningErrors;
	DoubleVector dvTestErrors;
	DoubleVector dvDistances;
};

///////////////////////////////////////////////////////////////////////
// Classe KWGrouperTest
// Tests de groupage
class KWGrouperTest : public Object
{
public:
	// Constructeur
	KWGrouperTest();
	~KWGrouperTest();

	/////////////////////////////////////////////////////////////////////////
	// Test effectues pour differentes tailles d'echantillon et pour
	// plusieurs methodes de discretisation

	// Test d'echantillons genere au hazard
	static void TestRandomSamples();

	// Test d'echantillons genere avec HiddenGroup
	static void TestHiddenGroupSamples();

	// Test d'echantillons genere avec HiddenGroup
	static void TestUnbalancedHiddenGroupSamples();

	// Test de la performance en temps de calcul
	static void TestCPUPerformance();

	// Tous les tests
	static void TestAll();

	/////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Creation des specifications des methodes de groupage a tester
	static void InitializeGrouperSpecArray(ObjectArray* oaGrouperSpecs);
};

// Methodes en inline

inline void KWSymbolSampleGenerator::SetSourceModalityNumber(int nValue)
{
	require(nValue >= 2);

	nSourceModalityNumber = nValue;
	bIsGeneratorCompiled = false;
}

inline int KWSymbolSampleGenerator::GetSourceModalityNumber() const
{
	return nSourceModalityNumber;
}

inline void KWSymbolSampleGenerator::SetTargetClassNumber(int nValue)
{
	require(nValue >= 2);

	nTargetClassNumber = nValue;
	bIsGeneratorCompiled = false;
}

inline int KWSymbolSampleGenerator::GetTargetClassNumber() const
{
	return nTargetClassNumber;
}

inline boolean KWSymbolSampleGenerator::IsGeneratorCompiled() const
{
	return bIsGeneratorCompiled;
}

inline Symbol KWSymbolSampleGenerator::GetSourceModalityAt(int nIndex) const
{
	require(bIsGeneratorCompiled);
	require(0 <= nIndex and nIndex < nSourceModalityNumber);

	return sSourceModalities.GetAt(nIndex);
}

inline Symbol KWSymbolSampleGenerator::GetTargetClassAt(int nIndex) const
{
	require(bIsGeneratorCompiled);
	require(0 <= nIndex and nIndex < nTargetClassNumber);

	return sTargetClasses.GetAt(nIndex);
}

inline double KWSymbolSampleGenerator::GetRandomDouble() const
{
	return RandomDouble();
}
