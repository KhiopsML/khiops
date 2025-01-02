// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningProblem.h"
#include "KWLearningProblemView.h"
#include "KWDRAll.h"
#include "KWDRDataGridDeployment.h"
#include "KWDRTablePartition.h"
#include "KWDRTableBlock.h"
#include "KWDRDataGridBlock.h"
#include "KWClassStats.h"
#include "KWPredictorBaseline.h"
#include "KWPredictorUnivariate.h"
#include "KWPredictorBivariate.h"
#include "KWPredictorNaiveBayes.h"
#include "KWPredictorSelectiveNaiveBayes.h"
#include "SNBPredictorSelectiveNaiveBayes.h"
#include "KWPredictorDataGrid.h"
#include "KWPredictorSelectiveNaiveBayesView.h"
#include "SNBPredictorSelectiveNaiveBayesView.h"
#include "KWPredictorDataGridView.h"
#include "KWDiscretizer.h"
#include "KWDiscretizerMODL.h"
#include "KWDiscretizerUnsupervised.h"
#include "KWDatabase.h"
#include "KWSTDatabaseTextFile.h"
#include "KWMTDatabaseTextFile.h"
#include "KWDatabaseView.h"
#include "KWSTDatabaseTextFileView.h"
#include "KWMTDatabaseTextFileView.h"
#include "PLParallelTask.h"
#include "KWFileIndexerTask.h"
#include "KWChunkSorterTask.h"
#include "KWKeySampleExtractorTask.h"
#include "KWSortedChunkBuilderTask.h"
#include "KWKeySizeEvaluatorTask.h"
#include "KWFileKeyExtractorTask.h"
#include "KWDatabaseCheckTask.h"
#include "KWDatabaseTransferTask.h"
#include "KWKeyPositionSampleExtractorTask.h"
#include "KWKeyPositionFinderTask.h"
#include "KWDatabaseBasicStatsTask.h"
#include "KWDataPreparationUnivariateTask.h"
#include "KWDataPreparationBivariateTask.h"
#include "KWDatabaseSlicerTask.h"
#include "KDSelectionOperandSamplingTask.h"
#include "KDDataPreparationAttributeCreationTask.h"
#include "KDDataPreparationAttributeCreationTaskView.h"
#include "InputBufferedFile.h"
#include "DTDecisionTreeCreationTask.h"
#include "KIDRRegisterAllRules.h"

// Service de lancement d'un projet d'apprentissage, defini
// par un probleme d'apprentissage et sa vue
// Il suffit de reimplementer les methodes virtuelles (valides pour un projet de base)
// puis d'appeler la methode Start() depuis le main
class KWLearningProject : public Object
{
public:
	// Constructeur
	KWLearningProject();
	~KWLearningProject();

	// Lancement du projet: gestion d'un probleme d'apprentissage et de son ergonomie
	// Par defaut:
	//   - initialisation de l'environnement
	//   - creation d'un objet projet et de sa vue, et gestion de la fermeture de
	//     l'application par une fenetre de confirmation
	//   - terminaison de l'environnement
	void Start(int argc, char** argv);

	// Initialisation et fermeture explicite de l'environnement
	// Utile pour effectuer des traitements batch
	void Begin();
	void End();

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Lancement de l'application cote maitre ou cote esclave
	void StartMaster(int argc, char** argv);
	void StartSlave();

	// Initialisation de l'environnement d'apprentissage
	// Enregistrement:
	//   . de la technologie de base utilises
	//   . des regles de derivation
	//   . des discretiseurs et groupeurs
	//   . des predicteurs
	//   . des vues sur les predicteurs ou autre (uniquement si master)
	//   . des taches
	//   . de la gestion des licences (licences a declarer dans une sous-classe)
	//   . du noms de module et de l'icone de l'application
	// Peut-etre enrichi dans une sous-classe
	virtual void OpenLearningEnvironnement();

	// Terminaison de l'environnment d'apprentissage
	virtual void CloseLearningEnvironnement();

	// Creation du projet d'apprentissage et de sa vue
	// Redefinissable dans une sous-classe
	virtual KWLearningProblem* CreateLearningProblem();
	virtual KWLearningProblemView* CreateLearningProblemView();

	// Creation dans un mode plus generique d'un projet d'apprentissage
	// Par defaut, implemente avec les methode specifique base sur des KWLearningProject
	virtual Object* CreateGenericLearningProblem();
	virtual UIObjectView* CreateGenericLearningProblemView();

	// Methodes appelees lorsque l'utilisateur saisi les flag -l -u et -v dans la ligne de commande
	static boolean ShowLicenseInfo(const ALString&);
	static boolean UpdateLicense(const ALString& sFileName);
	static boolean ShowVersion(const ALString&);
	static boolean ShowSystemInformation(const ALString& sValue);
};
