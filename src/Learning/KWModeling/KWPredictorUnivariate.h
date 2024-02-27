// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPredictorUnivariate;
class KWEvaluatedDataGridStats;
class KWEvaluatedDataGrid;
class KWClassifierUnivariateEvaluation;
class KWRegressorUnivariateEvaluation;
class KWClassifierUnivariateEvaluationTask;
class KWRegressorUnivariateEvaluationTask;

#include "KWPredictor.h"
#include "KWPredictorNaiveBayes.h"
#include "KWPredictorEvaluation.h"
#include "KWPredictorEvaluationTask.h"

////////////////////////////////////////////////////////////////////////////////
// Predicteur univarie
// Le predicteur doit etre parametre par un objet KWClassStats initialise pour l'apprentissage
// Les statistiques ne seront reevaluees que si necessaire
class KWPredictorUnivariate : public KWPredictorNaiveBayes
{
public:
	// Constructeur
	KWPredictorUnivariate();
	~KWPredictorUnivariate();

	// Constructeur generique
	KWPredictor* Create() const override;

	// Nom du classifier
	const ALString GetName() const override;

	// Prefixe du predicteur
	const ALString GetPrefix() const override;

	// Suffixe du predicteur (nom de l'attribut)
	const ALString GetSuffix() const override;

	// Parametrage d'un rang de predicteur univarie
	// Si 0, les prefixes et suffixes par defaut sont conserves (defaut)
	// Sinon, le prefixe devient "BU<rank>" et le suffixe devient vide
	// Utile pour remplacer le nom base sur un attribut (potentiellement long) par un nom base sur un index
	void SetUnivariateRank(int nValue);
	int GetUnivariateRank() const;

	// Recopie des specifications du predicteur
	void CopyFrom(const KWPredictor* kwpSource) override;

	//////////////////////////////////////////////////////////////////////////////
	// Parametrage du predicteur univarie

	// Parametrage du type de predicteur: le meilleur univarie possible,
	// ou en parametrant le nnom de l'attribut (defaut: true)
	void SetBestUnivariate(boolean bValue);
	boolean GetBestUnivariate() const;

	// Parametrage du nom de l'attribut a utiliser pour le predicteur
	void SetAttributeName(const ALString& sValue);
	const ALString& GetAttributeName() const;

	//////////////////////////////////////////////////////////////////////////////
	// Acces au resultats apres apprentissage
	// La methode Evaluate(database) a ete reimplementee et renvoie un
	// objet KWPredictorClusterEvaluation permettant de comparer les tables
	// de contingences en apprentissage et test selon different criteres

	// Evaluation du predicteur sur une base de donnees
	// Redefinition de la methode ancetre pour renvoyer un objet
	// KWUnivariateClassifierEvaluation ou KWRegressorUnivariateEvaluation,
	// comportant une grille d'evaluation
	KWPredictorEvaluation* Evaluate(KWDatabase* database) override;

	// Attribut source pour le predicteur (si apprentissage reussi)
	const ALString& GetSourceAttributeName() const;

	// Attribut univarie utilise pour le predicteur (si apprentissage reussi)
	const ALString& GetUnivariateAttributeName() const;

	// Recherche de la grille de preparation du predicteur univarie issu
	// de la phase d'apprentissage
	// Memoire: l'objet rendu appartient a l'appele
	const KWDataGridStats* GetTrainDataGridStats() const;

	// Libelles standard
	const ALString GetObjectLabel() const override;

	//////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition de la methode d'apprentissage
	boolean InternalTrain() override;

	// Parametrage
	int nUnivariateRank;
	boolean bBestUnivariate;
	ALString sAttributeName;
	ALString sSourceAttributeName;
	ALString sUnivariateAttributeName;
	KWDataGridStats trainDataGridStats;
};

////////////////////////////////////////////////////////////////////////////////
// Specialisation de KWDataGridStats pour avoir des rapports avec des groupes
// de valeurs ordonnes de la meme facon en apprentissage et en test selon
// la grille en apprentissage
class KWEvaluatedDataGridStats : public KWDataGridStats
{
public:
	// Constructeur
	KWEvaluatedDataGridStats();
	~KWEvaluatedDataGridStats();

	// Parametrage de la grille de partitionnement a evaluer du predicteur
	// Cette grille, appartenant a l'appelant, est essentiellement utilise
	// le temps de la production du rapport
	void SetPredictorDataGridStats(const KWDataGridStats* dataGridStats);
	const KWDataGridStats* GetPredictorDataGridStats() const;

	// Initialisation de l'evaluation d'un predicteur
	// A appeler apres avoir parametre la grille de partitionnement a evaluer
	void InitializeEvaluation(const KWPredictor* predictor);

	// Ecriture du contenu d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON) override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition du tri des cellules sources pour un attribut cible donne
	// Methode reimplementable pour personnaliser les rapports
	void SortSourceCells(ObjectArray* oaSourceCells, int nTargetAttributeIndex) const override;

	// Parametrage de la grille a utiliser pendant la production du rapport
	const KWDataGridStats* predictorDataGridStats;

	// Vecteur des valeurs min et max par intervalle, pour l'ecriture des discretisations JSON
	// Dans le cas d'une evaluation de grille, il faut memorise ces vecteurs pour pouvoir
	// appeler la methode d'ecriture du rapport JSON de la classe ancetre
	ContinuousVector cvJSONEvaluatedAttributeDomainLowerBounds;
	ContinuousVector cvJSONEvaluatedAttributeDomainUpperBounds;
};

////////////////////////////////////////////////////////////////////////////////
// Evaluation d'un classifieur univarie
// Additionellement aux metriques de performance d'un classifieur, cette classe rendre
// un DataGridStats avec la distribution par classe cible des effectifs evaluaees
// Cette classe composite etends KWClassifierEvaluation avec les methodes de KWDataGridEvaluation
class KWClassifierUnivariateEvaluation : public KWClassifierEvaluation
{
public:
	// Constructeur
	KWClassifierUnivariateEvaluation();
	~KWClassifierUnivariateEvaluation();

	// Initialization des resultats d'evaluation
	void InitializeCriteria() override;

	// Methode principal pour declencher l'evaluation
	void Evaluate(KWPredictor* predictor, KWDatabase* database) override;

	// Acces a la grille evaluee (grille vide si aucune evaluation n'a eu lieu)
	KWDataGridStats* GetEvaluatedDataGridStats();

	// Ecriture d'un rapport abrege
	void WriteReport(ostream& ost) override;

	// Ecriture du contenu d'un rapport JSON pour un tableau ou un dictionnaire
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) override;

	// Parametrage de la prise en compte dans les rapports
	boolean IsJSONReported(boolean bSummary) const override;

	//////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Expedition de l'evaluation en parallele
	KWPredictorEvaluationTask* CreatePredictorEvaluationTask() override;

	// Resultat de l'evaluation
	KWEvaluatedDataGridStats dgsEvaluationResults;

	// La tache parallele associe ecrit directement sur le rapport d'evaluation demandeur
	friend class KWClassifierUnivariateEvaluationTask;
};

////////////////////////////////////////////////////////////////////////////////
// Evaluation d'un regresseur univarie,
// Cette classe composite etends KWRegressorEvaluation avec les methodes de KWDataGridEvaluation
class KWRegressorUnivariateEvaluation : public KWRegressorEvaluation
{
public:
	// Constructeur
	KWRegressorUnivariateEvaluation();
	~KWRegressorUnivariateEvaluation();

	// Initialisation (additionelle) du DataGridStats de l'evaluation univariaree
	void InitializeCriteria() override;

	// Methode principal pour declencher l'evaluation
	void Evaluate(KWPredictor* predictor, KWDatabase* database) override;

	// Acces a la grille evaluee
	KWDataGridStats* GetEvaluatedDataGridStats();

	// Ecriture d'un rapport abrege
	void WriteReport(ostream& ost) override;

	// Ecriture du contenu d'un rapport JSON pour un tableau ou un dictionnaire
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) override;

	// Parametrage de la prise en compte dans les rapports
	boolean IsJSONReported(boolean bSummary) const override;

	//////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Evaluation du regresseur en parallel
	KWPredictorEvaluationTask* CreatePredictorEvaluationTask() override;

	// Service d'evaluation de grille
	KWEvaluatedDataGridStats dgsEvaluationResults;

	// La tache parallele associe ecrit directement sur l'service d'evaluation demandeur
	friend class KWRegressorUnivariateEvaluationTask;
};

////////////////////////////////////////////////////////////////////////////////
// Evaluation d'une grille sur une base
// Permet d'evaluer une partition (discretisation/groupage en univarie) sur une base de test
class KWDataGridEvaluation : public Object
{
public:
	// Constructeur
	KWDataGridEvaluation();
	~KWDataGridEvaluation();

	// Parametrage de la grille de partitionnement a evaluer du predicteur
	// Cette grille, appartenant a l'appelant, est essentiellement utilise le temps
	// de l'evaluation (et pour la production du rapport)
	void SetPredictorDataGridStats(const KWDataGridStats* dataGridStats);
	const KWDataGridStats* GetPredictorDataGridStats() const;

	// Parametrage des index de chargement
	void SetDataGridAttributeLoadIndexes(const KWLoadIndexVector* attributeLoadIndexes);
	const KWLoadIndexVector* GetDataGridAttributeLoadIndexes() const;

	// Evaluation d'une grille sur une base
	// La grille en parametre est utilisee pour calculee les effectifs des cellules
	// de la grille d'evaluation a partir de la base
	// Redefinition du calcul de l'evaluation
	void Initialize();
	void Finalize();

	// Ajout d'une instance a evaluer
	void AddInstance(const KWObject* kwoObject);

	// Ajout d'une grille contenant un paquet d'evaluations
	void AddEvaluatedDataGridStats(KWDataGridStats* addedEvaluatedDataGrid);

	// Grille evaluee (grille vide si aucune evaluation n'a eu lieu)
	KWDataGridStats* GetEvaluatedDataGridStats();

	// Export de la grille evaluee
	void ExportDataGridStats(KWDataGridStats* dataGridStats) const;

	//////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Parametrage de la grille predictrice a utiliser pendant l'evaluation
	const KWDataGridStats* predictorDataGridStats;

	// Grille evaluee, memorisant l'evaluation
	KWEvaluatedDataGridStats* evaluatedDataGridStats;

	// Index de chargement des attributs a utiliser pour l'evaluation de la grille
	const KWLoadIndexVector* livDataGridAttributes;

	// Grille (algorithmique) utilisee pour l'evaluation
	KWDataGrid* evaluatedDataGrid;

	// Parties de la variable explicative
	ObjectArray oaExplicativeParts;

	// Parties de la variable cible
	ObjectArray oaTargetParts;
};

////////////////////////////////////////////////////////////////////////////////
// Specialisation de KWDataGrid pour les grilles peuplees
// lors de l'evaluation de predicteurs univaries
// Classe technique, necessaire pour les tache d'evaluation des predicteurs univaries
class KWEvaluatedDataGrid : public KWDataGrid
{
public:
	// Methode specialisee pour ce type de grille :
	// La verification de coherence entre la granularite de la grille et son effectif est relachee car
	// l'effectif provient des donnees a evaluer et non des donnees ayant servi a l'apprentissage
	boolean Check() const override;
};

////////////////////////////////////////////////////////////////////////////////
// Tache d'evaluation d'un classifier univariee
// Additionellement aux metriques usuelles de classifieur on renvoie une l'evaluation de la
// grille predictrice sur la database vers le rapport d'evaluation demandant
class KWClassifierUnivariateEvaluationTask : public KWClassifierEvaluationTask
{
public:
	// Constructeur
	KWClassifierUnivariateEvaluationTask();
	~KWClassifierUnivariateEvaluationTask();

	//////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de DatabaseTask
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcessExploitDatabase() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;

	// Initialisation et nettoyage des variables de travail lies au predicteur
	void InitializePredictorSharedVariables(KWPredictor* predictor) override;
	void CleanPredictorSharedVariables() override;

	// Service d'evaluation via DataGrid du maitre
	KWDataGridEvaluation masterDataGridEvaluation;

	// Service d'evaluation via DataGrid des esclaves
	KWDataGridEvaluation slaveDataGridEvaluation;

	// Service d'evaluation demandeur
	KWClassifierUnivariateEvaluation* classifierUnivariateEvaluation;

	// Resultats de l'evaluation via DataGrid des esclaves. Sous forme de DataGridStats
	PLShared_DataGridStats output_slaveDataGridEvaluation;

	// Indices des attributs necessaires a l'evaluation de la grille
	PLShared_LoadIndexVector shared_livDataGridAttributes;

	// DataGrid du classifieur univarie
	PLShared_DataGridStats shared_classifierDataGridStats;
};

////////////////////////////////////////////////////////////////////////////////
// Tache d'evaluation d'un regresseur univariee
// Additionellement aux metriques usuelles de regresseur on renvoie une l'evaluation de la
// grille predictrice sur la database vers le rapport d'evaluation demandant
class KWRegressorUnivariateEvaluationTask : public KWRegressorEvaluationTask
{
public:
	// Constructeur
	KWRegressorUnivariateEvaluationTask();
	~KWRegressorUnivariateEvaluationTask();

	//////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de DatabaseTask
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcessExploitDatabase() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;

	// Initialisation et nettoyage des variables de travail lies au predicteur
	void InitializePredictorSharedVariables(KWPredictor* predictor) override;

	// Service d'evaluation via DataGrid du maitre
	KWDataGridEvaluation masterDataGridEvaluation;

	// Service d'evaluation via DataGrid des esclaves
	KWDataGridEvaluation slaveDataGridEvaluation;

	// Service d'evaluation demandeur
	KWRegressorUnivariateEvaluation* regressorUnivariateEvaluation;

	// Resultats de l'evaluation via DataGrid des esclaves. Sous forme de DataGridStats
	PLShared_DataGridStats output_slaveDataGridEvaluation;

	// Indices des attributs necessaires a l'evaluation de la grille
	PLShared_LoadIndexVector shared_livDataGridAttributes;

	// DataGrid du classifieur univarie
	PLShared_DataGridStats shared_regressorDataGridStats;
};
