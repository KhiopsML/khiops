// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class SNBAttributeSelectionScorer;
class SNBWeightedAttributeSelection;

#include "KWLearningSpec.h"
#include "SNBDataTableBinarySliceSet.h"
#include "SNBPredictorSelectionDataCostCalculator.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Selection ponderee d'attributs
class SNBWeightedAttributeSelection : public Object
{
public:
	// Constructeur
	SNBWeightedAttributeSelection();
	~SNBWeightedAttributeSelection();

	// Nettoyage (libere memoire)
	void Clean();

	//////////////////////////////////////////////////
	// Gestion de la selection d'attributs

	// Increment du poids de un attribut
	// Retourne le increment effectif si le delta fait depasser 1 le poids de l'attribute
	double IncreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute, double dDeltaWeight);

	// Decrement du poids de un attribut; retourne le decrement effectif
	// Retourne le decrement effectif si le delta fait depasser 0 le poids de l'attribut
	// Si l'attribut atteint un poids de 0 alors il est elimine de la selection
	double DecreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute, double dDeltaWeight);

	// Access au poids d'un attribut; renvoie 0.0 s'il n'est pas dans la selection
	double GetAttributeWeightAt(SNBDataTableBinarySliceSetAttribute* attribute) const;

	// True si l'attribut est selectionne
	boolean Contains(SNBDataTableBinarySliceSetAttribute* attribute) const;

	// Nombre d'attributs selectiones
	int GetAttributeNumber() const;

	// Somme des poids
	double GetWeightSum() const;

	//////////////////////////////
	// Services Divers

	// Test d'integrite
	boolean Check() const override;

	// Clone
	// Memoire : appelant
	SNBWeightedAttributeSelection* Clone() const;

	// Collecte les attributs selectiones
	// Memoire: appelant, contenus au binarySliceSet courant
	ObjectArray* CollectSelectedAttributes() const;

	// Ecriture
	void Write(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nAttributeNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Relation [attribut -> poids]
	NumericKeyDictionary nkdAttributeWeights;

	// Somme des poids
	double dWeightSum;
};

class SNBAttributeSelectionScorer : public KWLearningService
{
public:
	// Constructeur
	SNBAttributeSelectionScorer();
	~SNBAttributeSelectionScorer();

	////////////////////////////////////////////
	// Initialisation et destruction

	// Base de donnees dediee a l'apprentissage d'un predicteur
	// Son parametrage provoque un nettoyage des donnees de travail
	void SetDataTableBinarySliceSet(SNBDataTableBinarySliceSet* dataTableBinarySliceSet);
	SNBDataTableBinarySliceSet* GetDataTableBinarySliceSet() const;

	// Poids du prior du modele de selection (penalisation)
	void SetPriorWeight(double dValue);
	double GetPriorWeight() const;

	// Exposant du prior de poids (penalisation)
	void SetPriorExponent(double dValue);
	double GetPriorExponent() const;

	// Cout de construction de variables
	void SetConstructionCostEnabled(boolean bValue);
	boolean GetConstructionCostEnabled() const;

	// Cout de preparation de variables
	void SetPreparationCostEnabled(boolean bValue);
	boolean GetPreparationCostEnabled() const;

	// Initialise/reinitialise tous les donnees de travail (pas de liberation memoire)
	virtual void InitializeWorkingData();

	// Nettoyage de tous les donnees de travail (libere la memoire)
	void CleanWorkingData();

	// Cree la calculatrice de cout de donnees; retourne false si erreur de memoire
	boolean CreateDataCostCalculator();

	// True si la calculatrice de cout de donnees es creee
	boolean IsDataCostCalculatorCreated() const;

	// Initialise/reinitialise la calculatrice de cout de donnes
	// Elle doit etre creee au prealable (IsDataCostCalculatorCreated)
	void InitializeDataCostCalculator();

	// True si la calculatrice de cout de donnees est cree et pret a utiliser
	boolean IsDataCostCalculatorInitialized() const;

	// Nettoyage de la calculatrice de cout de donnees (libere memoire)
	void CleanDataCostCalculator();

	///////////////////////////////////////////////////////
	// Calculs des scores et couts de selection

	// Calcul du score de la selection
	// Disponible seulement si IsDataCostCalculatorInitialized
	double ComputeSelectionScore();

	// Calcul de cout du modele de selection
	double ComputeSelectionModelCost() const;

	// Calcul du cout de donnees de la selection
	// Disponible seulement si IsDataCostCalculatorInitialized
	double GetSelectionDataCost();

	//////////////////////////////////////
	// Gestion de la selection

	// Increment du poids de un attribut dans la selection
	boolean IncreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute, double dDeltaWeight);

	// Decrement du poids de un attribut dans la selection
	boolean DecreaseAttributeWeight(SNBDataTableBinarySliceSetAttribute* attribute, double dDeltaWeight);

	// Annulation du dernier increrement/decrement; ne peux pas etre appele deux fois de suite
	boolean UndoLastModification();

	// Acces a la selection courante
	// Memoire: Appelle
	const SNBWeightedAttributeSelection* GetAttributeSelection() const;

	// Recollection de la selection courante
	// Memoire: Appelant
	SNBWeightedAttributeSelection* CollectAttributeSelection() const;

	//////////////////////////////////
	// Services generiques

	// Test d'integrite
	boolean Check() const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nAttributeNumber, int nTargetValueNumber,
					      int nTargetType, boolean bIsTargetGrouped,
					      boolean bIncludeDataCostCalculator);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Detail du calcul de cout de modele de selection
	double ComputeSelectionModelAttributeWeightCost() const;
	double ComputeSelectionModelAttributeCost(SNBDataTableBinarySliceSetAttribute* attribute) const;

	// True seulement apres un appel de IncreaseAttributeWeight ou DecreaseAttributeWeight
	boolean IsUndoAllowed();

	/////////////////////////
	// Parametres

	// Base de donnees dediee a l'apprentissage de predicteurs
	SNBDataTableBinarySliceSet* binarySliceSet;

	// True si le cout de construction est considere
	boolean bIsConstructionCostEnabled;

	// True si le cout de preparation est considere
	boolean bIsPreparationCostEnabled;

	// Poids du prior du modele de selection
	double dPriorWeight;

	// Exposant du prior des poids (de la penalisation)
	double dPriorExponent;

	///////////////////////////////////
	// Variables de travail

	// Cout total des attributs de la selection
	double dSelectionModelAllAttributeCost;

	// Calculatrice du cout de selection de donnees
	SNBPredictorSelectionDataCostCalculator* dataCostCalculator;

	// True si la calculatrice de cout de donnees est cree
	boolean bIsDataCostCalculatorCreated;

	// True si les calculatrice de cout de donees est initialisee
	boolean bIsDataCostCalculatorInitialized;

	// Poids des attributs selectionnes
	SNBWeightedAttributeSelection weightedAttributeSelection;

	// Informations de la dernier modification des poids
	SNBDataTableBinarySliceSetAttribute* lastModificationAttribute;
	double dLastModificationDeltaWeight;
	double dLastModificationSelectionModelAllAttributeCost;
	boolean bWasLastModificationIncrease;
};
