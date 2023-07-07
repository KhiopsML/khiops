// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWVersion.h"
#include "TaskProgression.h"
#include "KWClassManagement.h"
#include "KWDatabase.h"
#include "FileService.h"
#include "CCAnalysisSpec.h"
#include "CCAnalysisResults.h"
#include "CCPostProcessingSpec.h"
#include "CCDeploymentSpec.h"
#include "KWClassStats.h"
#include "CCCoclusteringBuilder.h"
#include "CCCoclusteringReport.h"
#include "KWAttributeAxisName.h"

////////////////////////////////////////////////////////////
// Classe CCLearningProblem
//    Khiops: preparation des donnees
class CCLearningProblem : public Object
{
public:
	// Constructeur
	CCLearningProblem();
	~CCLearningProblem();

	////////////////////////////////////////////////////////////
	// Acces aux sous-parties du probleme
	// Memoire: les objet sont propriete de l'appele

	// Gestion des dictionnaire
	KWClassManagement* GetClassManagement();

	// Base de donnees
	KWDatabase* GetDatabase();

	// Parametres d'analyse
	CCAnalysisSpec* GetAnalysisSpec();

	// Parametre de post-traitement
	CCPostProcessingSpec* GetPostProcessingSpec();

	// Parametre de deploiement
	CCDeploymentSpec* GetDeploymentSpec();

	// Resultats d'analyse, post-processing et deploiement
	CCAnalysisResults* GetAnalysisResults();

	//////////////////////////////////////////////////////////////////////////////////
	// Synchronisation du dictionnaire entre le AnalysisDictionary de ClassManagement
	// et celui de Database
	// En effet, ces deux dictionnaires doivent etre les meme, mais il peuvent etre
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

	// CH IV Begin
	// Nombre d'enregistrements lus
	// CH IV Refactoring: renommer en GetInstanceNumber (cf. KWLearningService::GetInstanceNumber)?
	//    a supprimer si jamais utilise
	int GetSampleNumber() const;
	void SetSampleNumber(int nValue);
	// CH IV End

	//////////////////////////////////////////////////////////////
	// Fonctionnalites disponibles

	// Analyse de la base par coclustering
	void BuildCoclustering();

	// Post-traitement du coclustering
	void PostProcessCoclustering();

	// Extraction des clusters pour un des attributs du coclustering
	void ExtractClusters(const ALString& sCoclusteringAttributeName);

	// Preparation de deploiement
	void PrepareDeployment();

	/////////////////////////////////////////////////////////////////////////
	// Verification du parametrage, avec emission de message d'erreur

	// Verification de l'existence et de la validite de la classe
	boolean CheckClass() const;

	// Verification si le nom du fichier d'apprentissage est bien renseigne
	boolean CheckDatabaseName() const;

	// Verification des attributs utilisee pour le coclustering
	boolean CheckCoclusteringAttributeNames() const;

	// Verification des noms des fichiers de resultats par type de tache
	// Ils doivent etre differents des fichiers d'entree, et different entre eux
	boolean CheckResultFileNames(int nTaskId) const;

	// Liste des taches
	enum
	{
		TaskBuildCoclustering,
		TaskExtractClusters,
		TaskPostProcessCoclustering,
		TaskPrepareDeployment
	};

	//////////////////////////////////////////////////////////////
	// Services de base

	// Verification du chemin de repertoire complet en sortie, pour une tache donnee
	// On tente de construire les repertoires en sortie
	// On rend false avec message d'erreur si echec
	boolean CheckResultDirectory(int nTaskId) const;

	// Construction d'un chemin de fichier en sortie, pour un des fichiers resultats possibles,
	const ALString BuildOutputFilePathName(int nTaskId) const;

	// Construction d'un chemin de fichier khc en sortie, pour un des fichiers resultats possibles,
	const ALString BuildKhcOutputFilePathName(int nTaskId) const;

	// Libelles utilisateur: nom du module de l'application (GetLearningModuleName())
	const ALString GetClassLabel() const override;

	// CH IV Begin
	// Insertion d'un attribut d'identifiant dans la classe
	// Cet attribut est de type Symbol et contient le label de l'individu
	// Le 1er individu possede le label "1" que le fichier de donnees contient un header ou pas
	KWAttribute* InsertIdentifierAttribute(KWClass* kwcClass);
	// CH IV End

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ecriture des clusters categoriels et numeriques
	void WriteSymbolClusters(const CCHDGAttribute* symbolCoclusteringAttribute, ostream& ost);
	void WriteContinuousClusters(const CCHDGAttribute* continuousCoclusteringAttribute, ostream& ost);

	// Nom du chemin du fichier specifie en sortie pour une tache donnees
	const ALString GetSpecifiedOutputFileName(int nTaskId) const;

	// Libelle du fichier en sortie en sortie pour une tache donnees
	const ALString GetOutputFileLabel(int nTaskId) const;

	// Retourne le service de construction des chemins de fichier en sortie
	const KWResultFilePathBuilder* GetResultFilePathBuilder(int nTaskId) const;

	// Sous-parties du probleme d'apprentissage
	KWClassManagement* classManagement;
	KWDatabase* database;
	CCAnalysisSpec* analysisSpec;
	CCAnalysisResults* analysisResults;
	CCPostProcessingSpec* postProcessingSpec;
	CCDeploymentSpec* deploymentSpec;

	// Service de construction du chemin des fichiers en sortie
	mutable KWResultFilePathBuilder resultFilePathBuilder;
};
