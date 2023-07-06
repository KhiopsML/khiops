// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWLearningProblem;

#include "Object.h"
#include "KWVersion.h"
#include "TaskProgression.h"
#include "KWClassManagement.h"
#include "KWDatabase.h"
#include "KWAnalysisSpec.h"
#include "KWAnalysisResults.h"
#include "KWDatabaseCheckTask.h"

#include "KWClassStats.h"
#include "KWPreprocessingSpec.h"
#include "KWAttributeConstructionReport.h"

#include "KWPredictorEvaluator.h"

#include "KWPredictor.h"
#include "KWPredictorBaseline.h"
#include "KWPredictorUnivariate.h"
#include "KWPredictorEvaluation.h"
#include "KWDataPreparationClass.h"
#include "KDMultiTableFeatureConstruction.h"
#include "KDTextFeatureConstruction.h"
#include "KDDataPreparationAttributeCreationTask.h"
#include "MHDiscretizerMODLHistogram.h"
#include "KWLearningErrorManager.h"
#include "JSONFile.h"

////////////////////////////////////////////////////////////
// Classe KWLearningProblem
//    Khiops: preparation des donnees
class KWLearningProblem : public Object
{
public:
	// Constructeur
	KWLearningProblem();
	~KWLearningProblem();

	////////////////////////////////////////////////////////////
	// Acces aux sous-parties du probleme
	// Memoire: les objet sont propriete de l'appele

	// Gestion des dictionnaire
	KWClassManagement* GetClassManagement();

	// Base d'apprentissage
	KWDatabase* GetTrainDatabase();

	// Base de test
	KWDatabase* GetTestDatabase();

	// Parametres d'analyse
	KWAnalysisSpec* GetAnalysisSpec();

	// Resultats d'analyse
	KWAnalysisResults* GetAnalysisResults();

	//////////////////////////////////////////////////////////////////////////////////
	// Synchronisation du dictionnaire entre le AnalysisDictionary de ClassManagement
	// et celui de TrainDatabase et TestDatabase.
	// En effet, ces trois dictionnaires doivent etre les meme, mais il peuvent etre
	// edites depuis l'interface depuis plusieurs vues, et doivent etre synchronises.

	// Synchronisation depuis ClassManagement
	void UpdateClassNameFromClassManagement();

	// Synchronisation depuis TrainDatabase
	void UpdateClassNameFromTrainDatabase();

	////////////////////////////////////////////////////////
	// Acces direct aux attributs principaux

	// Fichier des dictionnaires
	ALString GetClassFileName() const;

	// Dictionnaire
	ALString GetClassName() const;

	// Nombre d'attributs
	int GetAttributeNumber() const;

	// Fichier de donnees
	ALString GetDatabaseName() const;

	// Attribut cible
	ALString GetTargetAttributeName() const;

	// Modalite cible principale
	ALString GetMainTargetModality() const;

	// Parametres de pretraitement
	KWPreprocessingSpec* GetPreprocessingSpec();

	// Evaluateur de predicteur
	KWPredictorEvaluator* GetPredictorEvaluator();

	//////////////////////////////////////////////////////////////
	// Fonctionnalites disponibles

	// Verification des donnees
	void CheckData();

	// Analyse de la base: preparation, modelisation, evaluation, ecriture des rapports
	virtual void ComputeStats();

	// Construction du dictionnaire de variables et export dans un fichier de dictionnaire, avec les couts
	void BuildConstructedDictionary();

	// Verification de l'existence et de la validite de la classe
	// Emission de message d'erreur en cas de probleme
	boolean CheckClass() const;

	// Verification de la validite de l'attribut cible (s'il est specifie)
	boolean CheckTargetAttribute() const;

	// Verification si le nom du fichier d'apprentissage est bien renseigne
	// Emission de message d'erreur si non renseigne
	boolean CheckTrainDatabaseName() const;

	// Verification des specifications de preprocessing
	// Elles doivent notamment etre compatibles avec le type de prediction
	boolean CheckPreprocessingSpecs() const;

	// Verification des specifications de recodage (si presentes)
	// Elles doivent notamment etre compatible avec le type de prediction
	boolean CheckRecodingSpecs() const;

	// Verification des noms des fichiers de resultats
	// Ils doivent etre differents des fichiers d'entree, et different entre eux
	// Emission de message d'erreur en cas de probleme
	boolean CheckResultFileNames() const;

	// Libelles utilisateur: nom du module de l'application (GetLearningModuleName())
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Initialisation des specifications d'apprentissage
	virtual void InitializeLearningSpec(KWLearningSpec* learningSpec, KWClass* kwcClass);

	// Construction de nouvelles variables
	// On renvoie une nouvelle classe dans son domaine (duplication de la classe initiale si rien n'est construit)
	// Les couts de construction par variables sont mis a jour, dans la classe construite
	// dans la classe initiale si rien n'a ete construit
	// Les specifications d'apprentissage sont mis a jour avec la nouvelle classe et la specification des
	// familles de construction de variables
	// En sortie, on fournit les attributs utilises construits references par leur nom, par type de construction
	// Code retour a false si un probleme ou une interruption utilisateur est survenue (et classe construite NULL)
	virtual boolean BuildConstructedClass(KWLearningSpec* learningSpec, KWClass*& constructedClass,
					      ObjectDictionary* odMultiTableConstructedAttributes,
					      ObjectDictionary* odTextConstructedAttributes);

	// Creation d'une classe en important les couts specifie dans les meta-donnees
	// Code retour a false si un probleme ou une interruption utilisateur est survenue (et classe construite NULL)
	boolean ImportAttributeMetaDataCosts(KWLearningSpec* learningSpec, KWClass*& constructedClass);

	// Initialisation des objets de calculs des statistiques
	virtual void InitializeClassStats(KWClassStats* classStats, KWLearningSpec* learningSpec);

	// Gestion du cas particulier de la regression dans le cas ou la classe cible contient des valeur manquantes
	// Dans ce cas, pour pouvoir apprendre un modle sur les valeurs presente, il faut passer temporairement
	// par un learningSpec intermediaire ou les valeur mnanquante sont filtrees le temps de l'apprentissage
	// au moyen d'une variable de selection specifique
	boolean IsSpecificRegressionLearningSpecNecessary(const KWLearningSpec* learningSpec) const;
	void PrepareLearningSpecForRegression(KWLearningSpec* learningSpec) const;
	void RestoreInitialLearningSpec(KWLearningSpec* learningSpec, const KWDatabase* initialDatabase) const;

	// Destruction de tous les rapports potentiels
	virtual void DeleteAllOutputFiles();
	void DeleteOutputFile(const ALString& sOutputFilePathName);

	// Ecriture des rapports de preparation des donnees au format tabulaire xls
	virtual void WritePreparationReports(KWClassStats* classStats);

	// Ecriture du rapport JSON
	// Seuls les rapports ecrits sont inclus dans le rapport JSON
	virtual void WriteJSONAnalysisReport(KWClassStats* classStats, ObjectArray* oaTrainedPredictorReports,
					     ObjectArray* oaTrainPredictorEvaluations,
					     ObjectArray* oaTestPredictorEvaluations);

	// Creation d'une classe de recodage dans le domaine en sortie
	// Le domaine initial permet apres apprentissage de nettoyer la classe du predicteur de ses attributs
	// construction ou preparation) inutiles
	virtual void BuildRecodingClass(const KWClassDomain* initialDomain, KWClassStats* classStats,
					KWClassDomain* trainedClassDomain);

	//////////////////////////////////////////
	// Apprentissage

	// Methode principale: apprentissage des predicteurs et memorisation de leur dictionnaire dans le domaine
	// courant Renvoie en cas d'erreur ou d'interruption utilisateur Memoire: les objets du tableau en sortie sont a
	// liberer par l'appelant
	virtual boolean TrainPredictors(const KWClassDomain* initialDomain, KWClassStats* classStats,
					ObjectArray* oaTrainedPredictors);

	// Collecte des classes de prediction de predicteurs dans un domaine de classe
	// Les classes des predicteurs sont transferees dans le domaine de classe en sortie, et dereferencees des
	// predicteurs
	virtual void CollectTrainedPredictorClasses(ObjectArray* oaTrainedPredictors,
						    KWClassDomain* trainedClassDomain);

	// Recherche des predicteurs a utiliser
	// Memoire: les objets du tableau en sortie sont a liberer par l'appelant
	virtual void CollectPredictors(KWClassStats* classStats, ObjectArray* oaPredictors);
	virtual void CollectUnivariatePredictors(KWClassStats* classStats, ObjectArray* oaUnivariatePredictors);

	// Apprentissage d'un predicteur
	// Le domaine initial permet apres apprentissage de nettoyer la classe du predicteurs de ses attributs
	// construction ou preparation) inutiles
	// Les stats permettent de reutiliser la preparation pour chaque predicteur
	// Renvoie en cas d'erreur ou d'interruption utilisateur
	virtual boolean TrainPredictor(const KWClassDomain* initialDomain, KWClassStats* classStats,
				       KWPredictor* predictor);

	// Sous-parties du probleme d'apprentissage
	KWClassManagement* classManagement;
	KWDatabase* trainDatabase;
	KWDatabase* testDatabase;
	KWAnalysisSpec* analysisSpec;
	KWAnalysisResults* analysisResults;
	KWPredictorEvaluator* predictorEvaluator;
};
