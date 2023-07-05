// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWPreprocessingSpec.h"
#include "KWBenchmarkSpec.h"
#include "KWClassStats.h"
#include "KWStatisticalEvaluation.h"
#include "KWPredictorSpec.h"
#include "KWPredictorNaiveBayes.h"
#include "KWPredictorUnivariate.h"
#include "KWPredictor.h"
#include "KWTrainedPredictor.h"
#include "KWPredictorEvaluation.h"
#include "KDMultiTableFeatureConstruction.h"
#include "KDTextFeatureConstruction.h"
#include "KDDataPreparationAttributeCreationTask.h"

////////////////////////////////////////////////////////////
// Classe KWLearningBenchmark
//    Benchmark de predicteurs
// Le benchmark est parametre par un ensemble de jeux d'essais
// et un ensemble de predicteurs, et par le parametrage de la
// strategie de validation (X Cross Validation)
// Il s'agit d'une classe destinees a etre reimplementee, chaque
// sous-classe definissant les type de predicteurs a comparer
// et les criteres d'evaluation a mesurer
class KWLearningBenchmark : public Object
{
public:
	// Constructeur
	KWLearningBenchmark();
	~KWLearningBenchmark();

	// Type de l'attribut cible (Continuous, Symbol, ou None en nom supervise)
	// Par defaut: Unknown (non defini)
	// Doit etre defini, et ne peut etre defini qu'une seule fois
	void SetTargetAttributeType(int nValue);
	int GetTargetAttributeType() const;

	// Filtrage des predicteurs specifiables pour l'evaluation,
	// sous forme de la liste des noms de predictors separes par des ';'
	// Les predicteurs enregistres dans KWPredictor sont par defaut
	// tous utilisables (pas de filtre). Sinon, ils doivent egalement
	// faire partie du filtre pour etre utilisables
	virtual const ALString GetPredictorFilter() const;

	//////////////////////////////////////////////////////////////
	// Parametrage du benchmark

	// Nombre de validations croisees
	int GetCrossValidationNumber() const;
	void SetCrossValidationNumber(int nValue);

	// Nombre de parties utilises
	int GetFoldNumber() const;
	void SetFoldNumber(int nValue);

	// Echantillonage stratifie
	boolean GetStratified() const;
	void SetStratified(boolean bValue);

	// Parametrage de la probabilite pour le test des differences significatives
	// (par defaut: 5%)
	void SetSignificanceLevel(double dValue);
	double GetSignificanceLevel() const;

	// Nom du fichier d'evaluation
	void SetReportFileName(const ALString& sValue);
	const ALString& GetReportFileName() const;

	// Rapport detaille par run
	void SetRunReport(boolean bValue);
	boolean GetRunReport() const;

	// Export des base d'apprentissage et de test de chaque fold (par defaut: false)
	// Utile pour le debug
	void SetExportBenchmarkDatabases(boolean bValue);
	boolean GetExportBenchmarkDatabases() const;

	// Parametrage des benchmarks utilises
	// Memoire: le tableau appartient a l'appele, ainsi que son contenu
	// une fois range dans le tableau (contient des KWBenchmarkSpec)
	ObjectArray* GetBenchmarkSpecs();

	// Parametrages des predictors a evaluer
	// Chacun doit etre parametre par son LearningSpec en propre
	// permettant par exemple de parametrer le preprocessing
	// Memoire: le tableau appartient a l'appele, ainsi que son contenu
	// une fois range dans le tableau (contient des KWPredictorSpec)
	ObjectArray* GetPredictorSpecs();

	////////////////////////////////////////////////////////
	// Evaluation et acces aux resultats (de la derniere evaluation)

	// Evaluation des predictors (ceux dont les specifications le permettent)
	// sur l'ensemble des benchmarks
	// Tache interruptible par le suivi des taches (TaskProgression)
	virtual void Evaluate();

	// Destruction des resultats d'evaluation
	void DeleteEvaluations();

	// Liste des criteres predefinis evalues (peut etre completer dans des sous-classes)
	// En classification supervisee
	//    TrainAccuracy, TestAccuracy, RatioAccuracy,
	//    TrainBalancedAccuracy, TestBalancedAccuracy, RatioBalancedAccuracy,
	//    TrainAUC, TestAUC, RatioAUC,
	//    TrainCompressionRate, TestCompressionRate, RatioCompressionRate
	// En regression
	//    TrainRMSE, TestRMSE, RatioRMSE,
	//    TrainMAE, TestMAE, RatioMAE,
	//    TrainNLPD, TestNLPD, RatioNLPD,
	//    TrainRankRMSE, TestRankRMSE, RatioRankRMSE,
	//    TrainRankMAE, TestRankMAE, RatioRankMAE,
	//    TrainRankNLPD, TestRankNLPD, RatioRankNLPD
	// En non supervise
	//    ... (a definir)
	// De facon generique et automatique
	//    FeatureNumber: nombre d'attribut natifs ou construits evalues
	//    ConstructedFeatureNumber: nombre d'attribut construits
	//    InformativeFeatureNumber: nombre d'attribut natifs ou construits informatifs
	//    UsedAttributeNumber: nombre d'attributs natifs utilises par le predicteur
	//	  TotalComputingTime
	//	  PreprocessingComputingTime
	int GetCriterionNumber() const;
	int GetCriterionIndexAt(const ALString& sCriterionName);
	const ALString& GetCriterionNameAt(int nCriterion);
	const ALString& GetCriterionLabelAt(int nCriterion);
	boolean GetCriterionMaximizationAt(int nCriterion);

	// Acces aux stastiques d'evaluation par critere, ou par critere et methode
	// Memoire: les objets retourne appartiennet a l'appele
	const ObjectArray* GetAllEvaluationsAt(int nCriterion) const;
	const KWStatisticalEvaluation* GetEvaluationAt(int nCriterion, int nPredictor) const;

	// Ecriture d'un rapport, avec resume synthetique et detaille par experience
	void WriteReportFile(const ALString& sFileName) const;
	virtual void WriteReport(ostream& ost) const;

	////////////////////////////////////////////////////////
	// Divers

	// Verification du parametrage
	boolean Check() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
	// Les methodes virtual sont a redefinir dans les sous classe
protected:
	////////////////////////////////////////////////////////
	// Initialisation des criteres d'evaluation utilises pour le rapport

	// Initialisation du tableau des evaluation par criteres
	void InitEvaluationResults();

	// Creation d'un resultat d'evaluation "typique" portant sur l'ensemble
	// des experiences, avec positionnement des libelles
	virtual KWStatisticalEvaluation* CreateTemplateEvaluation() const;

	// Acces a la classe correspondant a un benchmark pour en inspecter le contenu
	// Renvoie une instance de la classe uniquement si tout est OK, NULL sinon
	// Memoire: la classe retournee appartient a l'appelant
	KWClass* BuildBenchmarkClass(KWBenchmarkSpec* benchmarkSpec) const;

	// Creation de la liste des criteres, generiques ou par type de predicteur
	// La methode generique appel la methode specifique selon le type de predicteur du benchmark
	virtual void CreateCriterions();
	virtual void CreateClassifierCriterions();
	virtual void CreateRegressorCriterions();
	virtual void CreateClustererCriterions();

	// Ajout d'un critere
	void AddCriterion(const ALString& sCriterionName, const ALString& sCriterionLabel,
			  boolean bCriterionMaximization);

	// Destruction des criteres
	void DeleteCriterions();

	// Ecriture de la partie du rapport concernant les specification des benchmarks
	virtual void WriteBenchmarkReport(ostream& ost) const;

	// Ecriture de la partie du rapport concernant les specification des predictors
	virtual void WritePredictorReport(ostream& ost) const;

	// Ecriture de la partie du rapport concernant les resultats d'evaluation
	virtual void WriteEvaluationReport(ostream& ost) const;

	///////////////////////////////////////////////////////////
	// Collecte des mesures d'evaluation

	// Evaluation d'une experience donnees
	// En entree,les donnees referencees sont validees
	// (KWBenchmarkSpec et KWPredictorSpec) et le tableau des index
	// de cross-validation est disponible
	// En sortie, les criteres d'evaluations doivent etre enregistrees
	// pour l'experimentation
	virtual void EvaluateExperiment(int nBenchmark, int nPredictor, int nValidation, int nFold,
					IntVector* ivFoldIndexes);

	// Construction de nouvelles variables selon specification (GetAttributeConstructionSpec()) du predicteur
	// Si aucune variable n'est construite, retourne NULL
	// Sinon, les nouvelle variables sont construites dans la nouvelle classe construite et un nouveau domaine,
	// Les couts de construction par variables sont mis a jour, dans la classe construite ou
	// dans la classe initiale si rien n'a etet construit
	// En sortie, on fournit les attributs utilises construits references par leur nom, par type de construction
	virtual KWClass* BuildLearningSpecConstructedClass(KWLearningSpec* learningSpec, KWPredictorSpec* predictorSpec,
							   ObjectDictionary* odMultiTableConstructedAttributes,
							   ObjectDictionary* odTextConstructedAttributes);

	// Mise a jour des resultats d'evaluation sur tous les criteres
	// en apprentissage et en test
	// Cette methode est appelee par EvaluateExperiment(), d'abord en
	// apprentissage (bTrain=true), ensuite en test (bTrain=false)
	// La methode generique est dispatchee sur des methodes specialisees par type
	// de predicteur, redefinissable dans des sous-classes
	virtual void CollectAllResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment, int nRun,
				       KWPredictor* trainedPredictor, KWPredictorEvaluation* predictorEvaluation);
	virtual void CollectAllClassifierResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment,
						 int nRun, KWPredictor* trainedPredictor,
						 KWPredictorEvaluation* predictorEvaluation);
	virtual void CollectAllRegressorResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment,
						int nRun, KWPredictor* trainedPredictor,
						KWPredictorEvaluation* predictorEvaluation);
	virtual void CollectAllClustererResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment,
						int nRun, KWPredictor* trainedPredictor,
						KWPredictorEvaluation* predictorEvaluation);

	// Acces en mise a jour a une evaluation
	KWStatisticalEvaluation* GetUpdatableEvaluationAt(int nCriterion, int nPredictor);

	// Export d'une base, en construisant un nom de base contextuel
	void ExportBenchmarkDatabase(KWBenchmarkSpec* benchmarkSpec, int nValidation, int nFold, boolean bTrain,
				     IntVector* ivFoldIndexes);

	// Attributs de la classe
	int nTargetAttributeType;
	int nCrossValidationNumber;
	int nFoldNumber;
	boolean bStratified;
	double dSignificanceLevel;
	ALString sReportFileName;
	ObjectArray oaBenchmarkSpecs;
	ObjectArray oaPredictorSpecs;

	// Gestion des criteres
	StringVector svCriterionNames;
	StringVector svCriterionLabels;
	IntVector ivCriterionMaximizations;

	// Tableau des resultats d'evaluation (KWStatisticalEvaluation) par critere.
	// Pour chaque critere, on a un tableau par methode
	ObjectArray oaEvaluationArrays;

	// Parametrage de l'ecriture des rapports
	// Par defaut: true, true, false pour les trois niveaux de rapport
	boolean bSyntheticReport;
	boolean bExperimentReport;
	boolean bRunReport;

	// Parametrage des l'export des bases
	boolean bExportBenchmarkDatabases;
};