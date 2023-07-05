// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPredictorDataGrid;
class KWPredictorDataGridReport;

#include "KWPredictor.h"
#include "KWPredictorNaiveBayes.h"
#include "KWAttributeSubsetStats.h"
#include "KWDataGridOptimizerParameters.h"
#include "KWDataGridStats.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"

//////////////////////////////////////////////////////////////////////////////
// Predicteur en grille de donnees
// Le predicteur doit etre parametre par un objet KWClassStats correctement
// initialise pour l'apprentissage. Les statistiques ne seront reevaluees que
// si necessaire
// On herite de KWPredictorNaiveBayes pour reutiliser les methodes
// de calcul du predicteur (une grille correspond a une estimation de
// densite conditionnelle multivariee, exploitee comme un seul attribut
// dans un Bayesien naif).
class KWPredictorDataGrid : public KWPredictorNaiveBayes
{
public:
	// Constructeur
	KWPredictorDataGrid();
	~KWPredictorDataGrid();

	// Type de predicteur disponible: classification et non supervise
	boolean IsTargetTypeManaged(int nType) const override;

	// Constructeur generique
	KWPredictor* Create() const override;

	// Recopie des specifications du predicteurs
	void CopyFrom(const KWPredictor* kwpSource) override;

	// Nom du predictor
	const ALString GetName() const override;

	// Prefixe du predictor
	const ALString GetPrefix() const override;

	//////////////////////////////////////////////////////////////////////////////////////
	// Parametres d'apprentissage
	// Rappel: on peut acceder a GetTrainParameters() par une methode ancetre

	// Parametre d'apprentissage du predicteur en grille
	// Memoire: appartient a l'appele
	KWDataGridOptimizerParameters* GetDataGridOptimizerParameters();

	////////////////////////////////////////////////////////////////////
	// Acces aux resultats apres apprentissage
	// L'apprentissage est gere avec suivi des taches (MainLabel, Progression, Label).
	// En cas d'interruption, l'apprentissage est invalide

	// Rapport d'apprentissage, avec la description des grilles utilisees
	// Memoire: l'objet rendu appartient a l'appele
	KWPredictorDataGridReport* GetPredictorDataGridReport();

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition de la methode de creation du rapport, pour integrer la description
	// des grilles utilisees (renvoie un KWPredictorDataGridReport)
	void CreatePredictorReport() override;

	// Alimentation du rapport de prediction par les statistiques sur les grilles selectionnees
	// Le tableau d'attributs prepares permet d'acceder au grilles multivariees utilises
	// Le vecteur de poids est facultatif (poids tous a 1 par defaut)
	// Le nombre d'attributs utilises est memorise dans la rapport et retourne par la methode
	int FillPredictorDataGridReport(ObjectArray* oaDataPreparationAttributes, ContinuousVector* cvWeights);

	// Redefinition de la methode d'apprentissage
	boolean InternalTrain() override;

	// Apprentissage non supervise
	void InternalTrainUnsupervisedDG(KWDataPreparationClass* dataPreparationClass,
					 ObjectArray* oaDataPreparationUsedAttributes);

	// Preparation de la classe en precisant les attributs a charger
	void PrepareClassForLoad(ObjectArray* oaSelectedAttributeStats);

	// Reinitialisation de la classe pour tout charger
	void InitClassForLoad();

	// Selection des attributs participant a l'apprentissage
	// En sortie, le tableau contient les specifications des attributs a evaluer
	// Memoire: le tableau resultat appartient a l'appelant, mais son contenu appartient
	// a l'appele (il est compose d'objets KWSymbolAttributeStats et KWContinuousAttributeStats)
	ObjectArray* SelectTrainAttributeStats();

	// Parametres d'apprentissage
	KWDataGridOptimizerParameters dataGridOptimizerParameters;
};

///////////////////////////////////////////////////////////////////////////////
// Rapport d'apprentissage, avec utilisation d'une ou plusieurs grilles de donnees
// Ces resultat sont alimentes par la methode Train des predicteurs
class KWPredictorDataGridReport : public KWPredictorReport
{
public:
	// Constructeur
	KWPredictorDataGridReport();
	~KWPredictorDataGridReport();

	// Parametrage (facultatif) par des statistiques sur le probleme d'apprentissage
	// Memoire: l'objet de statistique est gere par l'appelant
	void SetClassStats(KWClassStats* stats);
	KWClassStats* GetClassStats() const;

	// Gestion des attributs selectionnes (classe KWSelectedAttributeReport)
	// Memoire: le tableau et son contenu appartiennent a l'appele
	ObjectArray* GetSelectedDataGridReports();

	// Ecriture d'un rapport detaille du predicteur, avec ses grilles selectionnees
	void WriteReport(ostream& ost) override;

	// Ecriture du contenu d'un rapport JSON pour un tableau ou un dictionnaire
	void WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) override;

	// Parametrage de la prise en compte dans les rapports
	boolean IsJSONReported(boolean bSummary) const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attribut de statistiques
	KWClassStats* classStats;

	// Tableau des grilles utilises
	ObjectArray oaSelectedDataGridReports;
};

//////////////////////////////////////////////////////////////////////////////
// Informations sur une grille selectionnee
class KWSelectedDataGridReport : public KWLearningReport
{
public:
	// Constructeur
	KWSelectedDataGridReport();
	~KWSelectedDataGridReport();

	// Grille de preparation
	// Memoire: la grille en parametre appartient a l'appele
	//   (le setter detruit et remplace l'eventuelle grille precedente)
	void SetPreparedDataGridStats(KWDataGridStats* dataGridStats);
	const KWDataGridStats* GetPreparedDataGridStats() const;

	// Evaluation univariee de l'attribut: apport de l'attribut seul
	void SetUnivariateEvaluation(double dValue);
	double GetUnivariateEvaluation() const;

	// Poids de l'attribut
	// (par defaut 0 si le classifieur ne genere pas de poids)
	void SetWeight(double dValue);
	double GetWeight() const;

	// Rapport synthetique destine a rentrer dans un tableau
	// Tous les champs sont affiches: noms des attributs, Level, Weight, caracteristiques de la grille
	void WriteHeaderLineReport(ostream& ost) override;
	void WriteLineReport(ostream& ost) override;

	// Ecriture d'un rapport
	void WriteReport(ostream& ost) override;

	// Redefinition des criteres de tri pour trier les grilles d'un classifieur
	const ALString GetSortName() const override;
	double GetSortValue() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	double dUnivariateEvaluation;
	double dWeight;
	KWDataGridStats* preparedDataGridStats;
};