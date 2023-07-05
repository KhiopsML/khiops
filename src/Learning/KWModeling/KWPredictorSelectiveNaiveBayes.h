// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPredictorSelectiveNaiveBayes;
class KWPredictorSNBWeightManager;
class KWPredictorEvaluatedAttribute;

#include "TaskProgression.h"
#include "KWPredictorNaiveBayes.h"
#include "KWDataPreparationBase.h"
#include "KWSelectionParameters.h"
#include "KWPredictorSelectionScore.h"

//////////////////////////////////////////////////////////////////////////////
// Predicteur bayesien naif selectif
// Le predicteur doit etre parametre par un objet KWClassStats correctement
// initialise pour l'apprentissage. Les statistiques ne seront reevaluees que
// si necessaire
class KWPredictorSelectiveNaiveBayes : public KWPredictorNaiveBayes
{
public:
	// Constructeur
	KWPredictorSelectiveNaiveBayes();
	~KWPredictorSelectiveNaiveBayes();

	// Type de predicteur disponible: classification et regression
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
	// Parametrage utilisateur des attributs a evaluer ou selectionner
	// Rappel: on peut acceder a GetTrainParameters() par une methode ancetre

	// Parametre de selection de variable
	// Memoire: appartient a l'appele
	KWSelectionParameters* GetSelectionParameters();

	////////////////////////////////////////////////////////////////////
	// Acces aux resultats apres apprentissage
	// L'apprentissage est gere avec suivi des taches (MainLabel, Progression, Label).
	// En cas d'interruption, l'apprentissage est invalide

	// Rapport d'apprentissage, avec la description des variables selectionnees
	// Memoire: l'objet rendu appartient a l'appele
	KWPredictorSelectionReport* GetPredictorSelectionReport();

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition de la methode de creation du rapport, pour integrer la description
	// des variables selectionnees (renvoie un KWPredictorSelectionReport)
	void CreatePredictorReport() override;

	///////////////////////////////////////////////////////////////////
	// Algorithme Selective Naive Bayes
	// Reference: Langley, P., Page, S. (1994) "Induction of Selective
	// Bayes Predictors", Proceedings of the Tenth Conference in
	// Artificial Intelligence
	// Principe de l'algorithme de reference:
	// Lors d'une premiere passe, on evalue tous les attributs, et on
	// garde celui qui maximise le taux de bonne prediction
	// Lors des passes suivantes, on evalue tous les attributs restants,
	// et on garde celui qui ajoutes aux attributs deja selectionnes,
	// permet de maximiser le taux de bonne prediction du predicteur
	// bayesien naif correspondant.
	// On s'arrete quand le taux d'erreur commence a decroitre.
	// En fin d'algorithme, on obtient un predicteur bayesien naif base
	// sur une selection des attributs initiaux
	//
	// Implementation
	// La bayesien naif est base sur l'evaluation des probabilites conditionnelles.
	// Pour eviter les problemes numeriques, les probabilites sont evaluees par leur
	// logarithme, ce qui transforme le produit de probabilite par attribut en
	// une somme de logarithme.
	// Les attributs candidats a la selection sont issus de la passe de statistiques
	// descriptives, ou les discretisations et groupages permettent d'evaluer les
	// probabilites utilisees dans le bayesien naif (accessible depuis un KWDataPreparationClass).
	// A la fin de l'algorithme, les resultats sont disponibles sous forme
	// d'une classe de prediction (comme pour tous les predicteurs) et sous forme
	// de la liste d'informations sur les attributs selectionnes (KWSelectedAttributeReport).

	// Redefinition de la methode d'apprentissage
	boolean InternalTrain() override;

	// Calcul des attributs a utiliser dans la base de preparation des donnees
	// Ces attributs peuvent etre limites en nombre, ou obligatoires selon une specification
	// utilisateur. Les attributs obligatoires sont en tete et marques en tant que tels.
	// Les attributs (obligatoire ou non) sont trie par importance univariee decroissante
	void ComputeUsableAttributes(KWDataPreparationBase* dataPreparationBase);

	// Ajout de meta-donnees (Weight, MAP) aux attributs du classifieur appris
	void FillPredictorAttributeMetaData(KWClass* kwcClass);

	////////////////////////////////////////////////////////////////////////
	// Methodes de pilotage de l'apprentissage du Selective Naive Bayes
	//
	// L'objectif est la minimisation du critere de selection
	// Toutes les methodes travaillent avec la variable de travail predictorSelectionScoreManager
	// et un parametre cout de selection devant etre consistant en entree (cf CheckSelectionScore()).
	// En sortie, le score de selection est fourni.
	// Seules les methodes principales fournissent explicitement la selection des variables retenues,
	// en sortie de la methode (sous la forme d'un dictionnaire appartenant a l'appelant).
	// Les methodes secondaires se contentent de maintenir le dictionnaire d'attributs selectionnes
	// via la variable de travail predictorSelectionScoreManager.

	// Optimisation de la selection des attributs par ajout/suppression
	// On renvoie le cout et les attributs selectionnes
	// Memoire: le dictionnaire rendu appartient a l'appelant, son contenu a l'appele
	NumericKeyDictionary* OptimizeAttributeSelection(double& dCurrentPredictorCost);

	// Optimisation multi-start en Fast Forward Backward Selection
	// On fait un premier glouton utilisant le tri initial des attributs, puis autant de glouton
	// randomise que demande. Pour chaque glouton randomise, en repart d'un ensemble d'attributs vide,
	// en parcourant la liste des attributs prepares, dans un ordre aleatoire dependant du start
	// On renvoie le cout et les attributs selectionnes
	// Note: le dictionnaire d'attribut de la variable de travail correspond a l'optimisation
	// mene lors du dernier "start", alors que celui rendu par la methode correspond a la
	// meilleure selection rencontree lors du "multi-start"
	// Memoire: le dictionnaire rendu appartient a l'appelant, son contenu a l'appele
	NumericKeyDictionary* MSOptimizeAttributeSelection(double& dCurrentPredictorCost);

	// Optimisation des ajouts et suppressions d'attribut par Fast Forward/Backward Selection
	// On fait autant de passe de supressions/ajouts rapides que necessaire
	void FFWBWOptimizeAttributeSelection(double& dCurrentPredictorCost);

	// Optimisation de l'ajout de nouveau attribut par Fast Forward Selection
	// On fait une seule passe d'ajout sur tous les attributs en partant du debut,
	// en examinant a chaque fois une fenetre d'attributs
	// On peut indiquer en parametre si l'optimisation porte sur le cout total,
	// ou sur le cout des donnees uniquement (quoi qu'il arrive, le dCurrentPredictorCost
	// est un cout total en entree comme en sortie)
	void FFWOptimizeAttributeSelection(double& dCurrentPredictorCost);

	// Optimisation de la suppression de nouveau attribut par Fast Backward Selection
	// On fait une seule passe de suppression sur tous les attributs en partant de la fin
	// en examinant a chaque fois une fenetre d'attributs
	void FBWOptimizeAttributeSelection(double& dCurrentPredictorCost);

	// Optimisation de l'ajout de nouveaus attributs par Forward Selection
	void FWOptimizeAttributeSelection(double& dCurrentPredictorCost);

	// Optimisation des ajouts et suppressions d'attribut par Forward/Backward Selection
	void FWBWOptimizeAttributeSelection(double& dCurrentPredictorCost);

	// Optimisation par recherche exhaustive
	// Memoire: le dictionnaire rendu appartient a l'appelant, son contenu a l'appele
	NumericKeyDictionary* OPTOptimizeAttributeSelection(double& dCurrentPredictorCost);

	// Verification des donnees de travail necessaire a l'apprentissage
	boolean CheckTrainWorkingData();

	// Verification du gestionnaire de score d'evaluation et de sa consistance avec le cout d'evaluation
	// Cette methode est appele en require de chaque methode d'optimisation de la selection
	// Cette methode appelle en prerequis CheckTrainWorkingData() et renvoie true systemeatiquement
	// si l'apprentissage doit etre stope
	boolean CheckSelectionScore(double dEvaluationScore);

	// Indique s'il faut stopper l'apprentissage
	// (cause: interruption demandee par l'utilisateur, ou erreur d'entrees-sorties)
	boolean StopTraining(KWDataPreparationBase* dataPreparationBase);

	// Enregistrement d'une evaluation d'attribut effectue par un algorithme de selection
	// (via le WeightManager)
	// Une trace (optionnelle) est emise par cette methode
	void RecordEvaluatedAttribute(int nEvaluationType,
				      KWDataPreparationAttribute* evaluatedDataPreparationAttribute,
				      double dPredictorCost);

	// Recherche des informations sur les attributs selectionnes
	// Le vecteur de poids (facultatif) indique les attributs utilise, avec un poids resultant du
	// moyennage de modele.
	// Le dictionnaire indique la sous partie des attributs appartenant a la meilleure selection de variables (MAP)
	// Creation des objets KWSelectedAttributeReport dans le rapport de prediction
	// Le vecteur de poids est facultatif. Dans le cas d'un vecteur de poids, tous les attributs
	// ayant un poids non nul sont pris en compte. Sinon, seul ceux presents dans la selection sont utilises.
	void CollectSelectedAttributes(ContinuousVector* cvAttributeWeights,
				       NumericKeyDictionary* nkdSelectedAttributes);

	// Filtrage des attributs selectionnes en fonction de la contrainte MaxFilteredAttributeNumber
	// Les attributs les moins interessant (poids multivarie le plus faible, ou univarie le plus faible
	// en cas d'egalite) sont elimines
	// Les resultats sont mises a jour en fonction du filtrage (poids a 0, attributs supprimes des containers)
	// En entree (et en sortie), le tableau des attributs selectionnes est trie par importance decroissante
	void FilterSelectedAttributes(ContinuousVector* cvAttributeWeights,
				      NumericKeyDictionary* nkdSelectedAttributes);

	//////////////////////////////////////////////////////////////
	// Calcul des offset a apporter aux scores pour maximiser la
	// performance du predictor en optimisant sa matrice de confusion

	// Initialisation de tous les attributs ayant un poids non nul dans l'evaluation courante
	// Cela permet de preparer un classifieur dans le cas CMA ou MA, en se basant
	// sur la variable de travail predictorSelectionScoreManager et les poids fournis par weightManager
	boolean InitializeAllWeightedAttributes();

	// Parametres de selection de variable
	KWSelectionParameters selectionParameters;

	// Gestionnaire des scores d'evaluation pour les algorithmes de selection
	KWPredictorSelectionScore* predictorSelectionScoreManager;

	// Gestionnaire des poids des attributs
	KWPredictorSNBWeightManager* weightManager;

	// Epsilon pour gerer le probleme de precision numerique
	// avec precision relative par jeu de donnees
	double dEpsilon;
};

//////////////////////////////////////////////////////////////////////////////
// Enregistrement des evaluations d'attributs pour la gestion de leur poids
class KWPredictorSNBWeightManager : public Object
{
public:
	// Constructeur
	KWPredictorSNBWeightManager();
	~KWPredictorSNBWeightManager();

	/////////////////////////////////////////////////////////////////////
	// Parametrage general

	// Base de preparation des attribut
	// Memoire: la base appartient a l'appelant
	void SetDataPreparationBase(KWDataPreparationBase* kwdpbBase);
	KWDataPreparationBase* GetDataPreparationBase();

	// Vecteur de poids des attributs (NULL si pas de methode de gestion des poids)
	// Memoire: appartient a l'appele
	ContinuousVector* GetAttributeWeights();

	// Methode utilisee pour la prise en compte des poids
	enum
	{
		None,                     // Pas de ponderation des attributs
		PredictorCompressionRate, // Ponderation par le taux de compression des classifieurs
		PredictorProb,            // Ponderation par les probabilites des classifieurs
	};
	void SetWeightingMethod(int nValue);
	int GetWeightingMethod() const;
	const ALString GetWeightingMethodLabel() const;

	// Niveau de trace
	//  0: pas de trace
	//  1: seul les optimaux locaux
	//  2: toutes les ameliorations
	//  3: toutes les evaluations
	void SetTraceLevel(int nValue);
	int GetTraceLevel() const;

	// Affichage de la selection des attributs dans la trace (uniquement si TraceLevel > 0)
	void SetTraceSelectedAttributes(boolean bValue);
	boolean GetTraceSelectedAttributes() const;

	/////////////////////////////////////////////////////////////////////
	// Gestion des poids des attributs

	// Reinitialisation des structures d'enregistrement des attributs
	// Remise a 0 des poids eventuel, ou a NULL du vecteur de poids si pas de gestion des poids
	void Reset();

	// Mise a jour des poids suite a une evaluation d'attribut
	// Permet uniquement la trace des evaluation si le poids n'est pas gere
	void UpdateWeightEvaluation(int nEvaluationType, NumericKeyDictionary* nkdSelectedAttributes,
				    KWDataPreparationAttribute* evaluatedDataPreparationAttribute,
				    double dNewPredictorModelCost, double dNewPredictorDataCost);

	// Calcul des poids des attributs par analyse du log des evaluations des selections d'attributs
	// Chaque poids d'attribut rend en compte la somme des probabilites des modeles contenant l'attribut
	// Les poids sont mise a jour dans le vecteur de poids passes en parametre (retaille si necessaire),
	// dont les index sont bases sur ceux des attributs de la base de preparation des attributs
	// Sans effet si le poids n'est pas gere
	void ComputeAttributeWeigths();

	//////////////////////////////////////////////////////////////////////
	// Services divers

	// Test d'integrite
	boolean Check() const override;

	// Affichage des poids des attributs
	void WriteAttributeWeights(ostream& ost) const;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Modification des poids des attributs en cours de selections
	void UpdateAttributeWeigths(NumericKeyDictionary* nkdSelectedAttributes, Continuous cDeltaWeight);

	// Parametres globaux
	KWDataPreparationBase* dataPreparationBase;
	ContinuousVector* cvAttributeWeights;
	int nWeightingMethod;
	int nTraceLevel;
	boolean bTraceSelectedAttributes;

	// Liste de toutes les evaluations d'attributs effectue au cours de l'algorithme
	// de selection des attributs (contient des objets KWPredictorEvaluatedAttribute)
	// On utilise une liste plutot qu'un tableau, parce que les mise a jour et les traitements
	// sont de nature sequentielles. De plus, la liste est basee sur de nombreuses allocations
	// de petits objets, alors que le tableau peut demander quelques allocations potentiellement
	// de tres grande taille, qui peuvent echouer.
	ObjectList olEvaluatedAttributes;
};

//////////////////////////////////////////////////////////////////////////////
// Informations sur un attribut evalue lors de la selection des attributs
class KWPredictorEvaluatedAttribute : public Object
{
public:
	// Constructeur
	KWPredictorEvaluatedAttribute();
	~KWPredictorEvaluatedAttribute();

	// Specifications de preparation de l'attribut evalue
	void SetDataPreparationAttribute(KWDataPreparationAttribute* kwdpaAttribute);
	KWDataPreparationAttribute* GetDataPreparationAttribute();

	// Type d'evaluation
	enum
	{
		Initial,           // Valeur initiale du classifieur, sans attribut
		Add,               // Evaluation d'un ajout d'attribut
		MandatoryAdd,      // Ajout effectif d'un attribut obligatoire
		BestAdd,           // Ajout effectif d'un attribut
		Remove,            // Evaluation d'une supression d'attribut
		BestRemove,        // Suppression effective d'un attribut
		LocalOptimum,      // Memorisation de la valeur d'un optimum local
		GlobalOptimum,     // Memorisation de la valeur d'un optimum global
		ForcedRemoveAll,   // Supression inconditionnelle de tous les attributs
		UnevaluatedAdd,    // Ajout inconditionnel d'un attribut, sans evaluation
		UnevaluatedRemove, // Supression inconditionnelle d'un attribut, sans evaluation
		ForcedEvaluation,  // Evaluation d'une selection (sans ajout ni supression d'attribut)
		Final              // Valeur finale du classifieur, avec les attributs de la meilleure selection
	};
	void SetType(int nValue);
	int GetType();

	// Libelle du type d'evaluation
	ALString GetTypeLabel() const;

	// Indique s'il s'agit d'une evaluation definitive
	// (Initial, BestAdd, BestRemove, ConfirmBestAdd, LocalOptimum, Final)
	boolean IsAcceptationType() const;

	// Evaluation de la partie modele
	void SetModelCost(double dValue);
	double GetModelCost() const;

	// Evaluation de la partie donnees connaissant le modele
	void SetDataCost(double dValue);
	double GetDataCost() const;

	// Evaluation totale
	double GetTotalCost() const;

	// Affichage ligne des informations sur l'attribut evalue
	void WriteLabel(ostream& ost) const;
	void Write(ostream& ost) const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nType;
	double dModelCost;
	double dDataCost;
};