// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"
#include "KWStat.h"

////////////////////////////////////////////////////////////////////////
// Classe KWStatisticalEvaluation
// Classe permettant la collecte, la synthese et la mise en forme de rapport
// pour un critere applique pour l'evaluation d'une methode
// Les resultats sont collecte pour un nombre d'experiences, chacune repetees
// un certain nombre de fois pour en accroitre la fiabilite
// Exemple: evaluation du taux de bonne prediction pour la methode SVM,
// sur un ensemble de jeux de donnees, chqaue experience etant repetee 10 fois
// (en 10-fold cross validation)
class KWStatisticalEvaluation : public Object
{
public:
	// Constructeur
	KWStatisticalEvaluation();
	~KWStatisticalEvaluation();

	////////////////////////////////////////////////////////////////////
	// Parametrage et collecte des resultats d'evaluation

	// Nom du critere d'evaluation
	void SetCriterionName(const ALString& sValue);
	const ALString& GetCriterionName() const;

	// Nom de la methode evaluee
	void SetMethodName(const ALString& sValue);
	const ALString& GetMethodName() const;

	// Nom du type d'experience (par exemple: "Benchmark", "Dataset"...)
	void SetExperimentName(const ALString& sValue);
	const ALString& GetExperimentName() const;

	// Direction d'optimisation du critere (par defaut: critere a maximiser)
	void SetMaximization(boolean bValue);
	boolean GetMaximization() const;

	// Nombre d'experiences
	// Toutes les valeurs et les libelles sont mises a 0 lors de ce parametrage
	void SetExperimentNumber(int nValue);
	int GetExperimentNumber() const;

	// Nombre de repetitions par experience
	// Toutes les valeurs sont mises a 0 lors de ce parametrage
	void SetRunNumber(int nValue);
	int GetRunNumber() const;

	// Supression de tous les resultats (nombre d'experiences et de repettions a 0)
	void ResetAllResults();

	// Reinitialisation de toutes les valeurs a 0
	void InitializeAllResults();

	// Resultat d'evaluation pour une experience et une repetition donnee
	void SetResultAt(int nExperiment, int nRun, double dValue);
	double GetResultAt(int nExperiment, int nRun) const;

	// Libelle d'une experience (facultatif)
	void SetExperimentLabelAt(int nExperiment, const ALString& sLabel);
	const ALString& GetExperimentLabelAt(int nExperiment) const;

	// Reinitialisation des libelles des experiences
	void InitializeAllLabels();

	//////////////////////////////////////////////////////////////////////
	// Acces aux statistiques sur les resultats d'evaluations

	// Statistiques par experience
	double GetMeanAt(int nExperiment) const;
	double GetGeometricMeanAt(int nExperiment) const;
	double GetStandardDeviationAt(int nExperiment) const;
	double GetMinAt(int nExperiment) const;
	double GetMaxAt(int nExperiment) const;

	// Statistiques globales
	double GetMean() const;
	double GetGeometricMean() const;
	double GetStandardDeviation() const;
	double GetMin() const;
	double GetMax() const;

	// Test si les differences par experience sont statistiquement differentes
	// pour les resultats obtenus avec une methode alternative, en utilisant
	// le test de Student pour une probabilite donnee (parametree dans une autre methode)
	// On renvoie +1, 0, ou -1 suivant le resultat du test
	int GetSignificantDifferenceAt(const KWStatisticalEvaluation* alternativeMethodResult, int nExperiment) const;

	// Statistiques sur le nombre total de differences significatives
	int GetSignificantWinNumber(const KWStatisticalEvaluation* alternativeMethodResult) const;
	int GetSignificantLossNumber(const KWStatisticalEvaluation* alternativeMethodResult) const;
	int GetSignificantTieNumber(const KWStatisticalEvaluation* alternativeMethodResult) const;

	// Parametrage de la probabilite pour le test des differences significatives
	// (par defaut: 5%)
	void SetSignificanceLevel(double dValue);
	double GetSignificanceLevel() const;

	// Rang de performance d'une methode par experience par rapport a
	// un ensemble de methodes evaluees (rang de la moyenne de ses valeur)
	// La methode dont on cherche le rang est precisee par son index
	static int GetMethodRankAt(const ObjectArray* oaEvaluatedMethodResults, int nMethodIndex, int nExperiment);

	// Rang moyen d'une methode sur l'ensemble des experiences
	static double GetMethodMeanRank(const ObjectArray* oaEvaluatedMethodResults, int nMethodIndex);

	////////////////////////////////////////////////////////////////////////
	// Production de rapports
	// Les rapport peuvent etre produits soit pour l'instance d'evaluation
	// courante, soit pour un tableau d'evaluations comparables (auquel cas,
	// la premiere evaluation du tableau sert de reference pour les libelles
	// des experiences et pour les tests de differences significatives)

	// Liste des mesures statistiques utilisables pour les rapports synthetiques
	// Ces indicateurs sont combinables entre eux par l'operateur ou (|)
	enum
	{
		None = 0,
		Mean = 1,
		GeometricMean = 2,
		StandardDeviation = 4,
		Min = 8,
		Max = 16,
	};

	// Mesures statistiques utilisables uniquement pour les rapport comparatifs
	enum
	{
		SignificantTests = 32,
		Rank = 64
	};

	// Mesures utilisees pour les rapports standard
	enum
	{
		StandardMeasures = Mean | StandardDeviation,
		StandardComparativeMeasures = StandardMeasures | SignificantTests,
		StandardSyntheticMeasures = Mean | GeometricMean,
		StandardSyntheticComparativeMeasures = Mean | GeometricMean | SignificantTests | Rank
	};

	// Libelle associe a des mesures statistiques
	static ALString GetStatisticalMeasuresLabel(int nStatisticalMeasures);

	// Rapport synthetiques global, en utilisant tout ou partie
	// des mesures statistiques
	void WriteReport(ostream& ost) const;
	void WriteSpecificReport(ostream& ost, int nStatisticalMeasures) const;
	static void WriteComparativeReport(ostream& ost, ObjectArray* oaEvaluatedMethodResults);
	static void WriteComparativeSpecificReport(ostream& ost, ObjectArray* oaEvaluatedMethodResults,
						   int nStatisticalMeasures);

	// Rapport synthetiques par experience, en utilisant tout ou partie
	// des mesures statistiques
	void WriteExperimentReport(ostream& ost) const;
	void WriteExperimentSpecificReport(ostream& ost, int nStatisticalMeasures) const;
	static void WriteExperimentComparativeReport(ostream& ost, ObjectArray* oaEvaluatedMethodResults);
	static void WriteExperimentComparativeSpecificReport(ostream& ost, ObjectArray* oaEvaluatedMethodResults,
							     int nStatisticalMeasures);

	// Rapport detaille par experience et par repetition
	void WriteValueReport(ostream& ost) const;
	static void WriteValueComparativeReport(ostream& ost, ObjectArray* oaEvaluatedMethodResults);

	////////////////////////////////////////////////////////////////////////
	// Services divers

	// Duplication
	KWStatisticalEvaluation* Clone() const;

	// Copie
	void CopyFrom(const KWStatisticalEvaluation* kwseSource);

	// Test de compatibilite avec une autre evaluation (meme critere et
	// memes experiences (les libelles des experiences peuvent etre vides))
	boolean IsComparable(const KWStatisticalEvaluation* kwseSource) const;

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Methode de test
	static void Test();

	////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Recopie de toutes les resultats dans un seul vecteur
	void CopyAllResults(DoubleVector* dvAllValues) const;

	// Calcul d'un libelle vide (ayant le bon nombre de TAB) a partir d'un libelle
	static ALString GetEmptyLabel(const ALString& sLabel);

	// Variables d'instance
	ALString sCriterionName;
	ALString sMethodName;
	ALString sExperimentName;
	boolean bMaximization;
	int nExperimentNumber;
	int nRunNumber;
	double dSignificanceLevel;

	// Tableau des vecteurs de resultats par experience
	ObjectArray oaExperimentValueVectors;

	// Libelles des experiences
	StringVector svExperimentLabels;
};
