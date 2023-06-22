// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class SNBAttributeSelectionScorer;

class SNBHardAttributeSelection;
class SNBHardAttributeSelectionScorer;

class SNBWeightedAttributeSelection;
class SNBWeightedAttributeSelectionScorer;

#include "KWLearningSpec.h"
#include "SNBDataTableBinarySliceSet.h"
#include "SNBPredictorSelectionDataCostCalculator.h"

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
	void SetDataTableBinarySliceSet(SNBDataTableBinarySliceSet* someBinarySliceSet);
	SNBDataTableBinarySliceSet* GetDataTableBinarySliceSet() const;

	// Poids du prior du modele de selection
	void SetPriorWeight(double dValue);
	double GetPriorWeight() const;

	// Cout de construction de variables
	void SetConstructionCostEnabled(boolean bValue);
	boolean IsConstructionCostEnabled() const;

	// Cout de preparation de variables
	void SetPreparationCostEnabled(boolean bValue);
	boolean IsPreparationCostEnabled() const;

	// Initialise/reinitialise tous les donnees de travail (pas de liberation memoire)
	virtual void InitializeWorkingData();

	// Nettoyage de tous les donnees de travail (libere la memoire)
	virtual void CleanWorkingData();

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
	double ComputeSelectionDataCost();

	//////////////////////////////////
	// Services generiques

	// Test d'integrite
	boolean Check() const override;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Detail du calcul de cout de modele de selection
	virtual double ComputeSelectionModelAttributeWeightCost() const = 0;
	double ComputeSelectionModelAttributeCost(SNBDataTableBinarySliceSetAttribute* attribute) const;

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
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Selection dure d'attributs
class SNBHardAttributeSelection : public Object
{
public:
	// Constructeur
	SNBHardAttributeSelection();
	~SNBHardAttributeSelection();

	// Nombre d'attributs de la selection
	int GetAttributeNumber() const;

	///////////////////////////////////////////////////////
	// Modification de la selection d'attributs

	// Ajout d'un attribut a la selection
	void AddAttribute(SNBDataTableBinarySliceSetAttribute* attribute);

	// Supression d'un attribut de la selection
	void RemoveAttribute(SNBDataTableBinarySliceSetAttribute* attribute);

	// Supression de tous les attributs de la selection
	void RemoveAllAttributes();

	// True si la selection contiens l'attribut
	boolean Contains(SNBDataTableBinarySliceSetAttribute* attribute) const;

	//////////////////////////////
	// Services Divers

	// Clonation
	SNBHardAttributeSelection* Clone() const;

	// Tableau avec les objects SNBDataTableBinarySliceSetAttribute de la selection
	// Memoire: Appelant; attributs contenus au SNBDataTableBinarySliceSet courant
	ObjectArray* CollectAttributes() const;

	// Ecriture
	void Write(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nAttributes);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Ensemble d'attributs selectionnes sous forme de dictionnaire [attr -> attr]
	NumericKeyDictionary nkdAttributeSelectionSet;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// OBSOLETE (mais cela sert de base a la doc future)
// Evaluation d'une selection de variables d'un predicteur Bayesien naif selectif La classe est
// parametree par un KWLearningService et une KWNewDataPreparationBase, permettant d'evaluer les
// ajouts ou supressiosn de variables (KWDataPreparationAtribute)
//
// Rappel:
// Le predicteur Bayesien naif selectif s'ecrit
//   P(Y|X) = P(Y)P(X|Y) / P(X)
//   P(Y|X) = P(Y) \prod_i{P(X_i|Y)} / P(X)
// Dans le cas de la classification avec J classes cibles, on a
//   P(Y_j|X) = P(Y_j) \prod_i{P(X_i|Y_j)} / P(X)
// Comme \sum_j{PY_j|X) = 1, on peut eliminer le terme P(X):
//   P(Y_j|X) = P(Y_j) \prod_i{P(X_i|Y_j)} / sum_j'{P(Y_j') \prod_i{P(X_i|Y_j')}}
//
// Chaque ajout/supression d'attribut entraine une mise a jour (additive en prenant les log de prob)
// des probabilite conditionnelles. Celles-ci doivent alors etre maintenue pour l'ensemble des J
// classes cibles (en nombre fixe, ou variable si les estimateurs univaries partitionnent
// differement les classes cibles). La partition cible multivariee correspond ainsi a l'union des
// partitions cibles univariees pour les attribut de la selection d'attributs courante. Un des
// objectif de cette classe est d'encapsuler la gestion automatique de la partition cible, en
// rendant transparente la gestion dynamique de sa taille au moyen de sous-classes dediee.
class SNBHardAttributeSelectionScorer : public SNBAttributeSelectionScorer
{
public:
	// Constructeur
	SNBHardAttributeSelectionScorer();
	~SNBHardAttributeSelectionScorer();

	/////////////////////////////////////////
	// Initialisation e nettoyage

	// Initialisation avec une selection vide
	void InitializeWorkingData() override;

	// Nettoyage (libere memoire)
	void CleanWorkingData() override;

	//////////////////////////////////////
	// Gestion de la selection

	// Ajout d'un attribut
	boolean AddAttribute(SNBDataTableBinarySliceSetAttribute* attribute);

	// Supression d'un attribut
	boolean RemoveAttribute(SNBDataTableBinarySliceSetAttribute* attribute);

	// Defait la derniere modification
	// Utilisable seulement apres un appel a AddAttribute ou RemoveAttribute
	boolean UndoLastModification();

	// Acces a la selection courante
	// Memoire : Conteneur a l'appele; contenu a l'instance de SNBDataTableBinarySliceSet courant
	const SNBHardAttributeSelection* GetAttributeSelection() const;

	// Collecte de la selection courante
	// Memoire : Containeur a l'appelant; contenu a l'instance de SNBDataTableBinarySliceSet courant
	SNBHardAttributeSelection* CollectAttributeSelection() const;

	//////////////////////////////////
	// Services generiques

	// Test d'integrite
	boolean Check() const override;

	// Affichage de la partition cible en cours
	void Write(ostream& ost) const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nAttributeNumber, int nTargetValueNumber,
					      int nTargetType, boolean bIsTargetGrouped,
					      boolean bIncludeDataCostCalculator);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Reimplementation des methodes privees de SNBAttributeSelectionScorer
	double ComputeSelectionModelAttributeWeightCost() const override;

	// True seulement apres un appel a AddAttribute ou RemoveAttribute
	boolean IsUndoAllowed();

	////////////////////////////////
	// Objets de travail

	// Ensemble d'attribute selectionnes
	SNBHardAttributeSelection attributeSelection;

	// Dernier attribute ajoute/enleve
	SNBDataTableBinarySliceSetAttribute* lastModificationAttribute;

	// Cout avant la derniere modification
	double dLastModificationSelectionModelAllAttributeCost;

	// True si la deniere modifiaction etait un ajout
	boolean bWasLastModificationAdd;
};

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

class SNBWeightedAttributeSelectionScorer : public SNBAttributeSelectionScorer
{
public:
	// Constructeur
	SNBWeightedAttributeSelectionScorer();
	~SNBWeightedAttributeSelectionScorer();

	////////////////////////////////////////////
	// Initialisation et destruction

	void SetPriorExponent(double dExponent);
	double GetPriorExponent() const;

	// Initialisation/reinitialisation des donnees de travail
	void InitializeWorkingData() override;

	// Nettoyage des donnees de travail
	void CleanWorkingData() override;

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

	//////////////////////////////
	// Services Divers

	// Test d'integrite
	boolean Check() const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nAttributeNumber, int nTargetValueNumber,
					      int nTargetType, boolean bIsTargetGrouped,
					      boolean bIncludeDataCostCalculator);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// True seulement apres un appel de IncreaseAttributeWeight ou DecreaseAttributeWeight
	boolean IsUndoAllowed();

	// Reimplementation des methodes prives de SNBAttributeSelectionScorer
	double ComputeSelectionModelAttributeWeightCost() const override;

	/////////////////////////
	// Parametres

	// Exposant du prior des poids (de la penalisation)
	double dPenalizationExponent;

	////////////////////////////////
	// Objets de travail

	// Poids des attributs selectionnes
	SNBWeightedAttributeSelection weightedAttributeSelection;

	// Informations de la dernier modification des poids
	SNBDataTableBinarySliceSetAttribute* lastModificationAttribute;
	double dLastModificationDeltaWeight;
	double dLastModificationSelectionModelAllAttributeCost;
	boolean bWasLastModificationIncrease;
};