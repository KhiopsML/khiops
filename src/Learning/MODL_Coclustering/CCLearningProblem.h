// Copyright (c) 2024 Orange. All rights reserved.
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

	// Construction d'un chemin de fichier a partir d'un nom de fichier
	// et d'un prefixe pour la partie nom du fichier
	// Si le repertoire des fichiers resultats n'est pas specifie, on prend celui de la
	// base d'apprentissage (si bInputCoclustering=false) ou celui du fichier de d'entree
	// de coclustering (si bInputCoclustering=true)
	// On ajoute l'eventuel suffixe aux noms des fichiers de sortie
	// On rend vide si le fichier en entree est vide
	const ALString BuildOutputFilePathName(const ALString& sFileName, boolean bInputCoclustering) const;

	// Construction du nom du chemin des fichier en sortie
	// Si le repertoire des fichiers resultats n'est pas specifie, on prend celui de la base d'apprentissage
	// S'il est relatif, on le concatene a celui de la base d'apprentissage
	// S'il est absolu, on le prend tel quel
	// S'il commence par ./, on le considere comme absolu, ce qui revient a le traiter  en relatif par
	//  rapport au directory courant
	const ALString BuildOutputPathName(boolean bInputCoclustering) const;

	// Libelles utilisateur: nom du module de l'application (GetLearningModuleName())
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ecriture des clusters categoriels et numeriques
	void WriteSymbolClusters(const CCHDGAttribute* symbolCoclusteringAttribute, ostream& ost);
	void WriteContinuousClusters(const CCHDGAttribute* continuousCoclusteringAttribute, ostream& ost);

	// Sous-parties du probleme d'apprentissage
	KWClassManagement* classManagement;
	KWDatabase* database;
	CCAnalysisSpec* analysisSpec;
	CCAnalysisResults* analysisResults;
	CCPostProcessingSpec* postProcessingSpec;
	CCDeploymentSpec* deploymentSpec;
};
