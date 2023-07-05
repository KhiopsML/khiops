// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class SNBPredictorSelectionDataCostCalculator;
class SNBTargetPart;

class SNBClassifierSelectionDataCostCalculator;
class SNBSingletonTargetPart;

class SNBRegressorSelectionDataCostCalculator;
class SNBIntervalTargetPart;

class SNBGeneralizedClassifierSelectionDataCostCalculator;
class SNBGroupTargetPart;
class SNBGroupTargetPartSignatureSchema;

#include "KWLearningSpec.h"
#include "SNBDataTableBinarySliceSet.h"

class SNBPredictorSelectionDataCostCalculator : public KWLearningService
{
public:
	// Constructeur
	SNBPredictorSelectionDataCostCalculator();
	~SNBPredictorSelectionDataCostCalculator();

	////////////////////////////////////////////
	// Initialisation et destruction

	// Parametrage du SNBDataTableBinarySliceSet de la database d'apprentissage
	void SetDataTableBinarySliceSet(SNBDataTableBinarySliceSet* dataTableBinarySliceSet);
	SNBDataTableBinarySliceSet* GetDataTableBinarySliceSet() const;

	// Trace la partition cible
	void SetDisplay(boolean bValue);
	boolean GetDisplay() const;

	// Allocation de la calculatrice
	// Peut echouer s'il n'y a pas assez de memoire pour les vecteur de scores de la partition de la cible
	virtual boolean Create() = 0;

	// Initialisation/reinitialisation
	virtual void Initialize() = 0;

	// Destruction
	virtual void Delete() = 0;

	//////////////////////////////////////////////////////////////////////
	// Calcul des scores et modification de la partition cible

	// Calcule le score de la selection courant
	virtual double ComputeSelectionDataCost() = 0;

	// Mise a jour de la partition de la cible quand un attribut rentre dans la selection
	virtual void UpdateTargetPartitionWithAddedAttribute(SNBDataTableBinarySliceSetAttribute* attribute) = 0;

	// Mise a jour de la partition de la cible quand un attribut sort de la selection
	virtual void UpdateTargetPartitionWithRemovedAttribute(SNBDataTableBinarySliceSetAttribute* attribute) = 0;

	// Mise a jour des vecteurs de "probabilites" conditionnelles dans la partition cible
	virtual boolean UpdateTargetPartScoresWithWeightedAttribute(SNBDataTableBinarySliceSetAttribute* attribute,
								    Continuous cWeight) = 0;

	/////////////////////////////////////////////////////////////////////////
	// Gestion des objets SNBTargetPart
	// Ces methodes permettent reutiliser des objets deja alloues

	// Aquisition d'une partie pret a l'utilisation
	// Renvoie NULL si probleme d'allocation du vecteur de scores
	virtual SNBTargetPart* GetOrCreatePart();

	// Libere une partie, elle n'est pas detruit pour une eventuelle reutilisation
	virtual void ReleasePart(SNBTargetPart* targetPart);

	// Creation generique d'une partie (sous-classe de SNBTargetPart)
	virtual SNBTargetPart* CreatePart() = 0;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	///  Implementation
protected:
	// Acces a la cible
	int GetTargetValueNumber() const;
	Symbol& GetTargetValueAt(int nTargetValue) const;
	int GetTargetValueFrequencyAt(int nTargetValue) const;

	/////////////////////////
	// Parametres

	// Learning database de l'appentissage
	SNBDataTableBinarySliceSet* binarySliceSet;

	// Flag pour la trace
	boolean bDisplay;

	////////////////////////////////
	// Objets de travail

	// Parties de la cible
	ObjectArray oaTargetPartition;

	// Parties de la cible effacees (cache)
	ObjectList olDeletedPartsCache;

	// Nombre d'instances caste en double
	double dInstanceNumber;

	// Epsilon de Laplace
	double dLaplaceEpsilon;

	// Denominateur de l'estimation de Laplace
	double dLaplaceDenominator;

	// Valeur maximal de du score d'une instance
	Continuous cMaxScore;

	// Valeur maximal de l'exponentiel du score d'une instance
	double dMaxExpScore;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Partie d'une partition des valeurs de la modalite cible (classe abstraite)
// Donne acces aux scores non-normalises de chaque instance d'une base de donnees d'apprentissage
// Le score pour l'instance i est donne par :
//
//   log P(Y = C_j) + Sum_k w_k * log P(X_ik| Y = C_j )  (classification)
//
//   Sum_k w_k * log P( X_ik | Y = I_m) (regression)
//
//   Sum_k w_k * log P( X_ik | Y = G_jm) (classification generalise : cible groupee)
//
//   C_j  : j-eme partie cible (celle representee par cet objet)
//   X_ik : Valeur du k-eme attribut pour l'instance i
//   w_k  : Poids du k-eme attribut
//
// Le sous-classes implementent les cas ou la partition de la cible est compose de :
//   - SNBSingletonTargetPart : Cas de la classification, utilise par SNBClassifierSelectionDataCostCalculator
//   - SNBIntervalTargetPart : Cas de la regression, utilise par KWRegressionSelectionDataCostCalculator
//   - SNBGroupTargetPart : Cas de la classification generalisee, utilise par
//   SNBGeneralizedClassifierSelectionDataCostCalculator
class SNBTargetPart : public Object
{
public:
	// Constructeur
	SNBTargetPart();
	~SNBTargetPart();

	// Score non-normalise de la partie pour chaque instance i : log P(Y = C_j) + Sum_k w_k * log P( X_ik | Y = C_j
	// )
	//   C_j  : j-eme partie cible (celle representee par cet objet)
	//   X_ik : Valeur du k-eme attribut pour l'instance i
	//   w_k  : Poids du k-eme attribut
	ContinuousVector* GetScores();

	// Rapport synthetique destine a rentrer dans un tableau
	virtual void WriteHeaderLineReport(ostream& ost) const = 0;
	virtual void WriteLineReport(ostream& ost) const = 0;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Score non-normalise pour chaque instance: voir GetScores
	ContinuousVector cvScores;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculatrice de couts de donnees pour la classification
class SNBClassifierSelectionDataCostCalculator : public SNBPredictorSelectionDataCostCalculator
{
public:
	// Constructeur
	SNBClassifierSelectionDataCostCalculator();
	~SNBClassifierSelectionDataCostCalculator();

	// Reimplementations de l'interface de SNBPredictorSelectionDataCostCalculator
	boolean Create() override;
	void Initialize() override;
	void Delete() override;
	double ComputeSelectionDataCost() override;
	void UpdateTargetPartitionWithAddedAttribute(SNBDataTableBinarySliceSetAttribute* attribute) override;
	void UpdateTargetPartitionWithRemovedAttribute(SNBDataTableBinarySliceSetAttribute* attribute) override;
	boolean UpdateTargetPartScoresWithWeightedAttribute(SNBDataTableBinarySliceSetAttribute* attribute,
							    Continuous cWeight) override;
	SNBTargetPart* CreatePart() override;

	//////////////////////////////
	// Services divers

	// True si l'objet est dans un etat valide
	boolean Check() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nTargetPartNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
private:
	// Verification de la coherence de la partition cible
	boolean CheckParts() const;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Element d'une partition de la cible en singletons
// Correspond au cas de la classification standard : une partie pour chaque modalite cible
class SNBSingletonTargetPart : public SNBTargetPart
{
public:
	// Constructeur
	SNBSingletonTargetPart();
	~SNBSingletonTargetPart();

	// Parametrage de l'index valeur cible associee
	void SetTargetValueIndex(int nIndex);
	int GetTargetValueIndex() const;

	// Reimplementation interface SNBTargetPart
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	int nTargetValue;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculatrice de couts de donnees pour la regression
class SNBRegressorSelectionDataCostCalculator : public SNBPredictorSelectionDataCostCalculator
{
public:
	// Constructeur
	SNBRegressorSelectionDataCostCalculator();
	~SNBRegressorSelectionDataCostCalculator();

	// Reimplementation de l'interface de SNBPredictorSelectionDataCostCalculator
	boolean Create() override;
	void Initialize() override;
	void Delete() override;
	double ComputeSelectionDataCost() override;
	void UpdateTargetPartitionWithAddedAttribute(SNBDataTableBinarySliceSetAttribute* attribute) override;
	void UpdateTargetPartitionWithRemovedAttribute(SNBDataTableBinarySliceSetAttribute* attribute) override;
	boolean UpdateTargetPartScoresWithWeightedAttribute(SNBDataTableBinarySliceSetAttribute* attribute,
							    Continuous cWeight) override;
	SNBTargetPart* CreatePart() override;

	//////////////////////////////
	// Services divers

	// Ecriture standard
	void Write(ostream& ost) const override;

	// Verification de la coherence
	boolean Check() const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nDistinctTargetValueNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Vue de liste de la partition cible
	ObjectList olTargetPartition;

	// Vecteur d'index de partie cible pour chaque instance de la base (ordonnee par valeur cible)
	IntVector ivTargetPartIndexes;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Element d'une partition d'une cible numerique en intervalles
// Correspond au cas de la regression
class SNBIntervalTargetPart : public SNBTargetPart
{
public:
	// Constructeur
	SNBIntervalTargetPart();
	~SNBIntervalTargetPart();

	// Effectif de l'intervalle
	void SetFrequency(int nValue);
	int GetFrequency() const;

	// Effectif cumule, en tenant compte les intervalles precedents
	void SetCumulativeFrequency(int nValue);
	int GetCumulativeFrequency() const;

	// Compteur d'utilisation de partie cible (identifiee par son effectif cumule)
	// Permet de gerer l'ajout/supression des parties cibles lors des MAJ par des partitions cibles univariees
	void SetRefCount(int nValue);
	int GetRefCount() const;

	// Rapport synthetique
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Nombre d'instances dans l'intervalle
	int nFrequency;

	// Nombre d'instances cumuleess jusqu'a cet intervalle
	int nCumulativeFrequency;

	// Nombre de references
	int nRefCount;
};

class SNBGeneralizedClassifierSelectionDataCostCalculator : public SNBPredictorSelectionDataCostCalculator
{
public:
	// Constructeur
	SNBGeneralizedClassifierSelectionDataCostCalculator();
	~SNBGeneralizedClassifierSelectionDataCostCalculator();

	// Reimplementation de l'interface de SNBPredictorSelectionDataCostCalculator
	boolean Create() override;
	void Initialize() override;
	void Delete() override;
	double ComputeSelectionDataCost() override;
	void UpdateTargetPartitionWithAddedAttribute(SNBDataTableBinarySliceSetAttribute* attribute) override;
	void UpdateTargetPartitionWithRemovedAttribute(SNBDataTableBinarySliceSetAttribute* attribute) override;
	boolean UpdateTargetPartScoresWithWeightedAttribute(SNBDataTableBinarySliceSetAttribute* attribute,
							    Continuous cWeight) override;
	SNBTargetPart* CreatePart() override;

	//////////////////////////////
	// Services divers

	// Test d'integrite
	boolean Check() const override;

	// Ecriture standard
	void Write(ostream& ost) const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nAttributeNumber, int nTargetValueNumber);

	/////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Initialisation des services d'indexation de groupes cible pour les attributs
	void InitializeTargetValueGroupMatchings();
	void CleanTargetValueGroupMatchings();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Gestion des signatures
	// La signature d'une valeur est le vecteur d'index des groupes cibles correspondant
	// a la valeur, pour chaque attribut present dans la signature

	// Mise a jour d'une signature par ajout d'un index de groupe (en fin de signature)
	void UpdateSignatureWithAddedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute, int nTargetValue,
					       SNBGroupTargetPart* groupTargetPart) const;

	// Mise a jour d'une signature par supression d'un index de groupe
	// Le dernier attribut de la signature prend l'index de celui supprime
	void UpgradeTargetSignatureWithRemovedAttribute(const SNBDataTableBinarySliceSetAttribute* attribute,
							int nTargetValue, SNBGroupTargetPart* groupTargetPart) const;

	// Verficiation d'une partie cible par rapport a un indice de valeur cible
	boolean CheckTargetSignature(SNBGroupTargetPart* groupTargetPart, int nTargetValue) const;

	// Index de groupe pour un attribut et valeur cible donne
	int GetTargetValueGroupIndexAt(const SNBDataTableBinarySliceSetAttribute* attribute, int nTargetValue) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Variables de travail utilisees pour accelerer le calcul du cout de donnees
	// Ces variables de travail sont preallouees une fois pour toutes, et utilisee uniquement dans la
	// methode ComputeSelectionDataCost

	// Tableau codifiant la relation [TargetValueIndex -> GroupTargetPart]
	ObjectArray oaGroupTargetPartsByTargetValueIndex;

	// Dictionnaire indexe par SNBDataTableBinarySliceSetAttribute's contenant des vecteurs d'entiers
	// Chaque vecteur codifie la relation [TargetValue -> AttributeTargetGroup] pour chaque attribut
	// Cette structure permet donc de codifier la relation [(Attribute, TargetValue) -> AttributeTargetGroup]
	// qui est implemente dans la methode GetTargetValueGroupIndexAt
	// Il n'est pas modifie au cours des calculs
	NumericKeyDictionary nkdTargetValueGroupMatchingsByAttribute;

	// Schema des signatures des parties de la cible
	SNBGroupTargetPartSignatureSchema* signatureSchema;

	// Le tableau herite oaTargetPartition n'est utilise que pour accelerer le calcul du cout de selection
	// Il se mets a jour seulement au debut de ComputeSelectionDataCost
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Gestion d'un score de prediction pour une partie cible
// Cas de la classification ou une partie correspond a un ensemble de valeurs
class SNBGroupTargetPart : public SNBTargetPart
{
public:
	// Constructeur
	SNBGroupTargetPart();
	~SNBGroupTargetPart();

	// Effectif de la partie
	void SetFrequency(int nValue);
	int GetFrequency() const;

	// Signature cible de la partie
	IntVector* GetSignature();
	int GetSignatureSize() const;
	int GetGroupIndexAt(int nSignature) const;

	// Rapport synthetique
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Libelle
	const ALString GetObjectLabel() const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nTargetValueNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Nombre d'individus dans la partition
	int nFrequency;

	// Signature de la partition
	IntVector ivTargetSignature;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Gestion de la specification d'une signature cible
// On definit la signature cible d'une valeur, ou d'un groupe de valeurs, comme le vecteur
// d'index de partie cible par attribut selectionne
// Deux valeurs ayant meme signature cible doivent appartenir a la meme partie, et une partie
// ne doit contenir que des valeurs de meme signature.
class SNBGroupTargetPartSignatureSchema : public Object
{
public:
	// Constructeur
	SNBGroupTargetPartSignatureSchema();
	~SNBGroupTargetPartSignatureSchema();

	// Initialisation avec aucun attribut
	void Initialize();

	//////////////////////////////////
	// Acces aux attributs

	// Taille de la signature cible
	int GetSize() const;

	// Acces a un attribut de la signature cible par son index
	SNBDataTableBinarySliceSetAttribute* GetAttributeAt(int nSignatureAttribute) const;

	// Acces a l'index de la signature d'un attribut; retourne -1 s'il n'en fait pas partie)
	int GetSignatureIndexAt(const SNBDataTableBinarySliceSetAttribute* attribute) const;

	// Ajout d'un index d'attribut dans la signature: son index est le dernier de la signature
	void AddAttribute(SNBDataTableBinarySliceSetAttribute* attribute);

	// Supression d'un attribut de la signature
	// Le dernier attribut de la signatureprend l'index de l'attribut supprime
	void RemoveAttribute(const SNBDataTableBinarySliceSetAttribute* attribute);

	// True si l'attribut appartient a la signature
	boolean Contains(const SNBDataTableBinarySliceSetAttribute* attribute) const;

	//////////////////////////////
	// Services divers

	// Test d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost);

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nAttributeNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Dictionnaire des paires KWSortableObject (SNBDataTableBinarySliceSetAttribute, index)
	// permettant d'associer chaque attribut selectionne a son index dans la signature
	// La cle du dictionnaire est l'objet SNBDataTableBinarySliceSetAttribute
	NumericKeyDictionary nkdAttributeIndexes;

	// Tableau des attributs de la signature cible
	ObjectArray oaAttributes;
};

// Comparaison de parties d'apres leur signature
int KWGroupTargetPartCompareTargetSignature(const void* elem1, const void* elem2);