// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class SNBAttributeSelectionWeightCalculator;
class SNBAttributeSelectionOptimizationRecord;

#include "SNBDataTableBinarySliceSet.h"
#include "SNBAttributeSelectionScorer.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculatrice des poids des attributs
// Permets de calculer les poids de l'estimateur SNB a partir de l'historique des passes
// d'evaluation d'attributs (ajouts et enlevements dans la selection).
class SNBAttributeSelectionWeightCalculator : public Object
{
public:
	// Constructeur
	SNBAttributeSelectionWeightCalculator();
	~SNBAttributeSelectionWeightCalculator();

	//////////////////////////
	// Parametrage

	// Base d'apprentissage dediee a l'apprentissage du predicteur (celle du selectionScore)
	void SetDataTableBinarySliceSet(SNBDataTableBinarySliceSet* database);
	SNBDataTableBinarySliceSet* GetDataTableBinarySliceSet() const;

	// Vecteur de poids des attributs (NULL si pas de methode de gestion des poids)
	// Memoire: appartient a l'appele
	ContinuousVector* GetAttributeWeights();

	// Vecteur de poids des attributs
	// Memoire: appartien a l'appelant
	ContinuousVector* CollectAttributeWeights();

	// Methode utilisee pour la prise en compte des poids
	enum WeightingMethod
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

	/////////////////////////////////////////////
	// Calcul des poids des attributs

	// Enregistrement d'une entree d'optimisation de la selection d'attributs
	// Permet uniquement la trace des evaluation si le poids n'est pas gere
	// Le parametre de selection d'attribut sert seulement pour la trace
	void AddSelectionOptimizationRecord(int nEvaluationType, SNBDataTableBinarySliceSetAttribute* attribute,
					    double dNewPredictorModelCost, double dNewPredictorDataCost,
					    const SNBHardAttributeSelection* attributeSelection);

	// Reinitialisation des structures d'enregistrement des attributs
	// Remise a 0 des poids eventuel, ou a NULL du vecteur de poids si pas de gestion des poids
	void Reset();

	void TraceSelectionOptimizationRecord(SNBAttributeSelectionOptimizationRecord* record,
					      const SNBHardAttributeSelection* attributeSelection);

	// Calcul des poids des attributs par analyse du log des evaluations des selections d'attributs
	// Chaque poids d'attribut rend en compte la somme des probabilites des modeles contenant l'attribut
	// Les poids sont mise a jour dans le vecteur de poids passes en parametre (retaille si necessaire),
	// dont les index sont bases sur ceux des attributs de la base de preparation des attributs
	// Sans effet si le poids n'est pas gere
	void ComputeAttributeWeigths();

	//////////////////////////////
	// Services divers

	// Test d'integrite
	boolean Check() const override;

	// Affichage des poids des attributs
	void WriteAttributeWeights(ostream& ost) const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Modification des poids des attributs en cours de selections
	void UpgradeAttributeWeigths(NumericKeyDictionary* oaSelectedAttributeSet, Continuous cDeltaWeight);

	// Parametres globaux
	SNBDataTableBinarySliceSet* binarySliceSet;
	ContinuousVector* cvAttributeWeights;
	int nWeightingMethod;
	int nTraceLevel;
	boolean bTraceSelectedAttributes;

	// Liste de SNBAttributeSelectionOptimizationRecord's enregistres lors des optimizations de selection la
	// attributs On utilise une liste plutot qu'un tableau parce que l'enregistrement de la trace de l'optimisation
	// fait de nombreuses allocations de petits objets, alors qu'un tableau peut demander des allocations
	// potentiellement de tres grande taille
	ObjectList olEvaluatedAttributes;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Enregistrement d'un pas de l'optimisation d'attributs de l'algorithme SNB classique
// Il y a deux groupes d'enregistrements:
//   - Les evaluations d'attributs (AddAttribute, MandatoryAdd, BestAdd, Remove, BestRemove)
//   - Les jalons de l'algorithme (Le reste)
class SNBAttributeSelectionOptimizationRecord : public Object
{
public:
	// Constructeur
	SNBAttributeSelectionOptimizationRecord();
	~SNBAttributeSelectionOptimizationRecord();

	// Attribut evalue (NULL pour les entrees Initial et Final)
	void SetAttribute(SNBDataTableBinarySliceSetAttribute* attribute);
	SNBDataTableBinarySliceSetAttribute* GetAttribute() const;

	// Type d'enregistrement
	enum
	{
		Start,             // Valeur initiale du classifieur, sans attribut
		AddAttribute,      // Evaluation d'un ajout d'attribut
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
	// Types : Initial, BestAdd, BestRemove, ConfirmBestAdd, LocalOptimum ou Final
	boolean IsAcceptationType() const;

	// Cout de la partie modele
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

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attribut evalue
	SNBDataTableBinarySliceSetAttribute* evaluatedAttribute;
	int nType;
	double dModelCost;
	double dDataCost;
};