// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPredictorSelectionScore;
class KWPSTargetPartScore;

class KWClassifierSelectionScore;
class KWPSTargetValueScore;

class KWRegressorSelectionScore;
class KWPSTargetIntervalScore;

class KWGeneralizedClassifierSelectionScore;
class KWTargetSignatureSpec;
class KWPSTargetValueSetScore;

#include "KWLearningSpec.h"
#include "KWDataPreparationClass.h"
#include "KWDataPreparationBase.h"

//////////////////////////////////////////////////////////////////////////////
// Evaluation d'une selection de variables d'un predicteur Bayesien naif selectif
// La classe est parametree par un KWLearningService et une KWDataPreparationBase,
// permettant d'evaluer les ajout ou supression de variables (KWDataPreparationAttribute)
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
// Chaque ajout/supression d'attribut entraine une mise a jour (additive en prenant les
// log de prob) des probabilite conditionnelles.
// Celles-ci doivent alors etre maintenue pour l'ensemble des J classes cibles
// (en nombre fixe, ou variable si les estimateurs univaries partitionnent differement
// les classes cibles). La partition cible multivariee correspond ainsi a l'union
// des partitions cibles univariees pour les attribut de la selection d'attributs courante.
// Un des objectif de cette classe est d'encapsuler la gestion automatique de la
// partition cible, en rendant transparente la gestion dynamique de sa taille
// au moyen de sous-classes dediee.
class KWPredictorSelectionScore : public KWLearningService
{
public:
	// Constructeur
	KWPredictorSelectionScore();
	~KWPredictorSelectionScore();

	// Parametrage des donnees de preparation a prendre en compte
	// Provoque un nettoyage des donnees de travail
	// Memoire: uniquement reference par l'appele
	void SetDataPreparationBase(KWDataPreparationBase* kwdpbValue);
	KWDataPreparationBase* GetDataPreparationBase();

	//////////////////////////////////////////////////////////////////
	// Gestion des etats permettant l'utilisation des methodes
	// de selection d'attributs
	// Les etapes standard sont les suivantes
	//  . SetLearningSpec: comme pour tout LearningService
	//  . SetDataPreparationBase: dataPreparationBase ne doit pas etre calcule,
	//    car utilise potentiellement beaucoup de memoire et peut echouer
	//  . CreateWorkingData: creation des structures, pouvant echouer
	//  . si IsWorkingDataCreated, calcul de dataPreparationBase
	//  . si ok, InitializeWorkingData, puis travail d'ajout/suppresion d'attributs

	// Creation des donnees de travail
	// Cette methode peut echouer en cas de manque de memoire (message d'erreur dans ce cas)
	//  Preconditions: Check()
	//  PostConditions: IsWorkingDataCreated() selon succes ou echec des creations
	// Il n'est pas necessaire que DataPreparationBase soit calcule
	boolean CreateWorkingData();
	boolean IsWorkingDataCreated() const;

	// Initialisation/reinitialisation de la selection d'attributs
	// On part alors d'une selection d'attribut vide et valide)
	//  Preconditions: Check(), IsWorkingDataCreated()
	//                 dataPreparationBase->IsPreparedDataComputed()
	//  Postcondition: IsWorkingDataInitialized()
	// Peut etre reappele pour reinitialiser les donneees de travail,
	// sans les recreer
	virtual void InitializeWorkingData();
	boolean IsWorkingDataInitialized() const;

	// Nettoyage complet des donnees de travail (appele par le destructeur)
	//  Postcondition -> not IsWorkingDataCreated() et not IsWorkingDataInitialized()
	virtual void CleanWorkingData();

	//////////////////////////////////////////////////////////////////
	// Modification de la selection et evaluation de son cout

	// Ajout/suppression d'un attribut
	void AddAttribute(KWDataPreparationAttribute* dataPreparationAttribute);
	void RemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute);

	// Ajout pondere d'un attribut
	// Methode avance permettant d'evaluer un moyennage de modeles
	// Attention, dans ce cas, seule le cout de selection partie donnees garde un sens
	void AddWeightedAttribute(KWDataPreparationAttribute* dataPreparationAttribute, Continuous cWeight);

	// Calcul du cout de la selection
	virtual double ComputeSelectionTotalCost();
	virtual double ComputeSelectionModelCost();
	virtual double ComputeSelectionDataCost();

	// Detail de calcul du cout de selection pour la partie modele
	virtual double ComputeSelectionModelAttributeNumberCost(int nAttributeNumber);
	virtual double ComputeSelectionModelAttributeCost(KWDataPreparationAttribute* dataPreparationAttribute);

	// OBSOLETE Prise en compte du poids du prior
	double GetPriorWeight() const;
	void SetPriorWeight(double dValue);

	// OBSOLETE Prise en compte du cout de construction
	boolean GetConstructionCost() const;
	void SetConstructionCost(boolean bValue);

	// OBSOLETE Prise en compte du cout de preparation
	boolean GetPreparationCost() const;
	void SetPreparationCost(boolean bValue);

	///////////////////////////////////////////////////////////////
	// Acces aux attributs de la selection
	// (objets KWDataPreparationAttribute indexes par eux-meme)

	// Nombre d'attributs
	virtual int GetSelectedAttributeNumber() const;

	// Test si un attribut est selectionne
	boolean IsAttributeSelected(KWDataPreparationAttribute* dataPreparationAttribute) const;

	// Dictionnaire des attribut selectionnes
	// Memoire: le dictionnaire rendu appartient a l'appele
	NumericKeyDictionary* GetSelectedAttributes();

	//////////////////////////////////////////////////////////////////
	// Services generiques

	// Test d'integrite du parametrage
	boolean Check() const override;

	// Affichage de la partition cible en cours
	void Write(ostream& ost) const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////////
	// Gestion optimisee de la partition de l'attribut cible (TP pour TargetPartition).
	// Toutes ces methodes sont redefinissables dans des sous-classes

	// Creation de la partition cible et des donnees de travail minimales
	// Peut echouer s'il n'y a pas assez de memoire
	virtual boolean TPCreate();

	// Initialisation/reinitialisation de la partition cible, pour une selection d'attribut vide
	virtual void TPInitialize();

	// Destruction de la partition cible
	virtual void TPDelete();

	// Verification de la coherence de la partition cible
	virtual boolean TPCheck() const;

	// Affichage
	virtual void TPWrite(ostream& ost) const;

	// Ajout/suppression d'un attribut avec impacts sur la partition cible
	virtual void TPAddAttribute(KWDataPreparationAttribute* dataPreparationAttribute);
	virtual void TPRemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute);

	// Mise a jour des vecteurs de probabilites conditionnelles dans la partition cible
	// Le boolen indique le sens de la mise a jour (Add ou Remove sinon)
	// La partition cible multivariee doit etre compatible avec la partition cible univariee
	// de l'attribut passe en parametre
	virtual void TPUpgradeAttributeSourceConditionalProbs(KWDataPreparationAttribute* dataPreparationAttribute,
							      Continuous cWeight);

	///////////////////////////////////////////////////////////////////////////////////
	// Creation/destruction des parties
	// Permet de bufferiser l'allocation des parties cibles

	// Creation d'une partie
	// Renvoie NULL si probleme d'allocation (une partie est associee a un vecteur de probas conditionnelles)
	virtual KWPSTargetPartScore* TPNewPart();

	// Destruction d'une partie
	// La partie est en fait liberee est prete a reutilisation si necessaire
	virtual void TPDeletePart(KWPSTargetPartScore* targetPart);

	// Creation generique d'une partie (sous-classe de KWPSTargetPartScore)
	virtual KWPSTargetPartScore* TPCreatePart();

	// Affichage de la partition cible
	void TPSetDisplay(boolean bValue);
	boolean TPGetDisplay() const;

	///////////////////////////////////////////////////////////////////////////////////
	// Donnees de travail

	// Calcul des effectifs par partie de l'attribut cible
	// Le resultat est rendu dans le vecteur d'entier en parametre
	// Methodes legerement couteuse en temps de calcul, mais cela reste negligeable par
	// rapport aux traitement principaux
	// On evite ainsi la mise en place d'une structure de bufferisation des calculs
	void ExportTargetPartFrequencies(KWDataPreparationAttribute* dataPreparationAttribute,
					 IntVector* ivTargetPartFrequencies) const;

	// Base de preparation des donnees
	KWDataPreparationBase* dataPreparationBase;

	// Gestion des parties en reserve
	ObjectList olDeletedParts;

	// Dictionnaires des attributs selectionnes
	NumericKeyDictionary nkdSelectedAttributes;

	// Cout modele total des attributs de la selection, mise a jour incrementalement
	double dSelectionModelAllAttributCost;

	// Gestion des etats
	boolean bIsWorkingDataCreated;
	boolean bIsWorkingDataInitialized;

	// Affichage de la partition cible
	boolean bTPDisplay;

	// OBSOLETE
	double dPriorWeight;
	boolean bIsConstructionCost;
	boolean bIsPreparationCost;
};

//////////////////////////////////////////////////////////////////////////////
// Evaluation d'une selection de variables d'un predicteur Bayesien naif selectif
// dans le cas de la classification supervisee, avec partition fixe de la
// variable cible (autant de parties que de valeurs)
class KWClassifierSelectionScore : public KWPredictorSelectionScore
{
public:
	// Constructeur
	KWClassifierSelectionScore();
	~KWClassifierSelectionScore();

	// Calcul du cout de la selection
	double ComputeSelectionDataCost() override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////////
	// Gestion optimisee de la partition de l'attribut cible (TP pour TargetPartition).

	// Redefinition des methodes virtuelles
	boolean TPCreate() override;
	void TPInitialize() override;
	void TPDelete() override;
	boolean TPCheck() const override;
	void TPWrite(ostream& ost) const override;
	void TPAddAttribute(KWDataPreparationAttribute* dataPreparationAttribute) override;
	void TPRemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute) override;
	void TPUpgradeAttributeSourceConditionalProbs(KWDataPreparationAttribute* dataPreparationAttribute,
						      Continuous cWeight) override;
	KWPSTargetPartScore* TPCreatePart() override;

	///////////////////////////////////////////////////////////////////////////////////
	// Donnees de travail
	//
	// Note: il serait possible d'optimiser les calcul en utilisant deux partitions de travail
	// afin de se contenter de swapper les partition pour un Remove suivant un Add du meme
	// attribut (ou inversement)
	// Cela a ete essayer, a diminuer d'environ 50% le nombre de mises a jour effectuees,
	// mais eu un impact negligeable sur la performance globale (envirion 5%) au prix
	// d'une complexite du code tres superieure, et d'un doublement de l'espace memoire
	// L'option est donc abandonnee

	// Gestion d'une partition cible au moyen d'un tableau de parties
	ObjectArray oaTargetPartition;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Gestion d'un score de prediction pour une partie cible donnee
// Pour une partie cible j (avant passage au log): P(Y_j) \prod_i{P(X_i|Y_j)}
// Classe interne pour la gestion de la classe KWPredictorSelectionScore,
// specialisable dans des sous-classes
class KWPSTargetPartScore : public Object
{
public:
	// Constructeur
	KWPSTargetPartScore();
	~KWPSTargetPartScore();

	// Vecteur indexe par les instances de la base d'apprentissage des probabilites
	// conditionnelles multivariees
	ContinuousVector* GetLnSourceConditionalProbs();

	// Rapport synthetique destine a rentrer dans un tableau
	virtual void WriteHeaderLineReport(ostream& ost);
	virtual void WriteLineReport(ostream& ost);

	// Affichage standard
	void Write(ostream& ost);

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	ContinuousVector cvLnSourceConditionalProbs;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Gestion d'un score de prediction pour une partie cible
// Cas de la classification supervisee ou une partie correspond a une valeur
class KWPSTargetValueScore : public KWPSTargetPartScore
{
public:
	// Constructeur
	KWPSTargetValueScore();
	~KWPSTargetValueScore();

	// Rapport synthetique
	void WriteHeaderLineReport(ostream& ost);
	void WriteLineReport(ostream& ost);
};

//////////////////////////////////////////////////////////////////////////////
// Evaluation d'une selection de variables d'un predicteur Bayesien naif selectif
// dans le cas de la regression, avec partition de la  variable cible
// en parties dans une liste ordonnees
class KWRegressorSelectionScore : public KWPredictorSelectionScore
{
public:
	// Constructeur
	KWRegressorSelectionScore();
	~KWRegressorSelectionScore();

	// Calcul du cout de la selection
	double ComputeSelectionDataCost() override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////////
	// Gestion optimisee de la partition de l'attribut cible (TP pour TargetPartition).

	// Redefinition des methodes virtuelles
	boolean TPCreate() override;
	void TPInitialize() override;
	void TPDelete() override;
	boolean TPCheck() const override;
	void TPWrite(ostream& ost) const override;
	void TPAddAttribute(KWDataPreparationAttribute* dataPreparationAttribute) override;
	void TPRemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute) override;
	void TPUpgradeAttributeSourceConditionalProbs(KWDataPreparationAttribute* dataPreparationAttribute,
						      Continuous cWeight) override;
	KWPSTargetPartScore* TPCreatePart() override;

	///////////////////////////////////////////////////////////////////////////////////
	// Donnees de travail
	//
	// Note: il serait possible d'optimiser les calcul en utilisant deux partitions de travail
	// afin de se contenter de swapper les partition pour un Remove suivant un Add du meme
	// attribut (ou inversement)
	// Cela a ete essaye, a diminue d'environ 50% le nombre de mises a jour effectuees,
	// mais eu un impact negligeable sur la performance globale (environ 5%) au prix
	// d'une complexite du code tres superieure, et d'un doublement de l'espace memoire
	// L'option est donc abandonnee

	// Gestion d'une partition cible au moyen d'une liste de parties
	ObjectList olTargetPartition;

	/////////////////////////////////////////////////////////////////////////////////////////
	// Variable de travail utilise uniquement pour accelerer le calcul du cout de selection
	// Ces variables de travail sont preallouee une fois pour toutes, et utilisee uniquement
	// dans la methode ComputeSelectionDataCost

	// Gestion d'une partition cible au moyen d'une tableau de parties, pour un acces indexe rapide
	ObjectArray oaTargetPartition;

	// Vecteur d'index de partie cible pour chaque instance de la base (ordonnee par valeur cible)
	IntVector ivTargetPartIndexes;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Gestion d'un score de prediction pour une partie cible
// Cas de la regesssion ou une partie correspond a un intervalle de valeurs
class KWPSTargetIntervalScore : public KWPSTargetPartScore
{
public:
	// Constructeur
	KWPSTargetIntervalScore();
	~KWPSTargetIntervalScore();

	// Effectif de la partie
	void SetFrequency(int nValue);
	int GetFrequency() const;

	// Effectif cumule, en tenant compte des parties precedentes
	void SetCumulativeFrequency(int nValue);
	int GetCumulativeFrequency() const;

	// Compteur d'utilisation de partie cible (identifiee par son effectif cumule)
	// Permet de gere l'ajout et supression des parties cibles lors
	// de mise a jour par des partitions cibles univariees
	void SetRefCount(int nValue);
	int GetRefCount() const;

	// Rapport synthetique
	void WriteHeaderLineReport(ostream& ost);
	void WriteLineReport(ostream& ost);

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	int nFrequency;
	int nCumulativeFrequency;
	int nRefCount;
};

//////////////////////////////////////////////////////////////////////////////
// Evaluation d'une selection de variables d'un predicteur Bayesien naif selectif
// dans le cas de la classification, avec partition de la  variable cible
class KWGeneralizedClassifierSelectionScore : public KWPredictorSelectionScore
{
public:
	// Constructeur
	KWGeneralizedClassifierSelectionScore();
	~KWGeneralizedClassifierSelectionScore();

	// Calcul du cout de la selection
	double ComputeSelectionDataCost() override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////////
	// Gestion optimisee de la partition de l'attribut cible (TP pour TargetPartition).

	// Redefinition des methodes virtuelles
	boolean TPCreate() override;
	void TPInitialize() override;
	void TPDelete() override;
	boolean TPCheck() const override;
	void TPWrite(ostream& ost) const override;
	void TPAddAttribute(KWDataPreparationAttribute* dataPreparationAttribute) override;
	void TPRemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute) override;
	void TPUpgradeAttributeSourceConditionalProbs(KWDataPreparationAttribute* dataPreparationAttribute,
						      Continuous cWeight) override;
	KWPSTargetPartScore* TPCreatePart() override;

	///////////////////////////////////////////////////////////
	// Services sur la correspondance entre valeurs et groupe

	// Initialisation des services d'indexation de groupes cible pour les attributs
	void InitialiseTargetValueGroupMatchings();
	void CleanValueTargetGroupMatchings();

	// Acces aux valeurs cibles
	int GetTargetValueNumber() const;
	Symbol& GetTargetValueAt(int nIndex) const;
	int GetTargetValueFrequencyAt(int nIndex) const;

	// Acces au vecteur d'index des groupes cibles d'un attribut, permettant d'associer
	// un index de groupe cible a chaque index de valeur cible
	const IntVector*
	GetAttributeTargetValueGroupMatching(KWDataPreparationAttribute* dataPreparationAttribute) const;

	///////////////////////////////////////////////////////////////////////////////////////////
	// Services de gestion des signatures, en se basant sur la specification des signatures
	// La signature d'une valeur est le vecteur d'index des groupes cibles correspondant
	// a la valeur, pour chaque attribut present dans la signature

	// Calcul de la signature d'une valeur
	void ComputeTargetSignature(int nTargetValueIndex, IntVector* ivTargetSignature) const;

	// Mise a jour d'une signature par ajout d'un index de groupe (en fin de signature)
	void UpgradeTargetSignatureWithAddedAttribute(int nTargetValueIndex,
						      const IntVector* ivAddedAttributeTargetValueGroupMatching,
						      IntVector* ivTargetSignature) const;

	// Mise a jour d'une signature par supression d'un index de groupe
	// Le dernier attribut de la signature prend l'index de l'attribut supprime
	void UpgradeTargetSignatureWithRemovedAttribute(int nTargetValueIndex, int nRemovedSignatureAttributeIndex,
							IntVector* ivTargetSignature) const;

	// Verification de la signature d'une valeur
	boolean CheckTargetSignature(int nTargetValueIndex, const IntVector* ivTargetSignature) const;

	/////////////////////////////////////////////////////////////////////////////////////////
	// Variable de travail utilise uniquement pour accelerer le calcul du cout de selection
	// Ces variables de travail sont preallouee une fois pour toutes, et utilisee uniquement
	// dans la methode ComputeSelectionDataCost

	// Acces par attribut a vecteur d'index permettant le matching entre les index de valeurs et les
	// index de groupe
	NumericKeyDictionary nkdAttributeTargetValueGroupMatchings;

	// Tableau des partie correspondant a chaque valeur cible, reference par son index cible
	// Attention, chaque partie (KWPSTargetValueSetScore) possede une signature unique, mais peut
	// etre referencee plusieurs fois dans le tableau
	ObjectArray oaTargetValueParts;

	/////////////////////////////////////////////////////////////////////////////////////////
	// Variable de travail utilise uniquement pour accelerer le calcul du cout de selection
	// Ces variables de travail sont preallouee une fois pour toutes, et utilisee uniquement
	// dans la methode ComputeSelectionDataCost

	// Gestion d'une partition cible au moyen d'une tableau de parties, pour un acces indexe rapide
	// Cette partition est synchronisee en permanence
	ObjectArray oaTargetPartition;

	// Specification de la signature cible
	KWTargetSignatureSpec* targetSignatureSpec;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Gestion de la specification d'une signature cible
// On definit la signature cible d'une valeur, ou d'un groupe de valeurs, comme le vecteur
// d'index de partie cible par attribut selectionne
// Deux valeurs ayant meme signature cible doivent appartenir a la meme partie, et une partie
// ne doit contenir que des valeurs de meme signature.
class KWTargetSignatureSpec : public Object
{
public:
	// Constructeur
	KWTargetSignatureSpec();
	~KWTargetSignatureSpec();

	///////////////////////////////////////////////////////////
	// Gestion de la signature cible

	// Initialisation avec aucun attribut
	void Initialize();

	// Taille de la signature cible
	int GetAttributeNumber() const;

	// Acces a un attribut de la signature cible par son index
	KWDataPreparationAttribute* GetAttributeAt(int nIndex) const;

	// Acces a l'index d'un attribut dans la signature cible (-1 s'il n'en fait pas partie)
	int GetAttributeIndex(KWDataPreparationAttribute* dataPreparationAttribute) const;

	// Ajout d'un attribut dans la signature: son index est le dernier de la signature
	void AddAttribute(KWDataPreparationAttribute* dataPreparationAttribute);

	// Supression d'un attribut de la signature
	// Le dernier attribut de la signature prend l'index de l'attribut supprime
	void RemoveAttribute(KWDataPreparationAttribute* dataPreparationAttribute);

	///////////////////////////////////////////////////////////
	// Services generiques

	// Test d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost);

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Dictionnaire des paires KWSortableObject (dataPreparationAttribute, index), permettant
	// d'associer chaque attribut selectionne a son index dans la signature
	// La cle du dictionnaire est l'objet dataPreparationAttribute
	NumericKeyDictionary nkdTargetSignatureAttributeIndexes;

	// Tableau des attributs de la signature cible
	ObjectArray oaTargetSignatureAttributes;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Gestion d'un score de prediction pour une partie cible
// Cas de la classification ou une partie correspond a un ensemble de valeurs
class KWPSTargetValueSetScore : public KWPSTargetPartScore
{
public:
	// Constructeur
	KWPSTargetValueSetScore();
	~KWPSTargetValueSetScore();

	// Effectif de la partie
	void SetFrequency(int nValue);
	int GetFrequency() const;

	// Signature cible de la partie
	IntVector* GetTargetSignature();

	// Rapport synthetique
	void WriteHeaderLineReport(ostream& ost) override;
	void WriteLineReport(ostream& ost) override;

	// Libelle
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	int nFrequency;
	IntVector ivTargetSignature;
};

// Comparaison de parties d'apres leur signature
int KWPSTargetValueSetScoreCompareTargetSignature(const void* elem1, const void* elem2);

/////////////////////////////////////
// Methodes en inline

inline double KWPredictorSelectionScore::GetPriorWeight() const
{
	return dPriorWeight;
}

inline void KWPredictorSelectionScore::SetPriorWeight(double dValue)
{
	dPriorWeight = dValue;
}

inline boolean KWPredictorSelectionScore::GetConstructionCost() const
{
	return bIsConstructionCost;
}

inline void KWPredictorSelectionScore::SetConstructionCost(boolean bValue)
{
	bIsConstructionCost = bValue;
}

inline boolean KWPredictorSelectionScore::GetPreparationCost() const
{
	return bIsPreparationCost;
}

inline void KWPredictorSelectionScore::SetPreparationCost(boolean bValue)
{
	bIsPreparationCost = bValue;
}

inline ContinuousVector* KWPSTargetPartScore::GetLnSourceConditionalProbs()
{
	return &cvLnSourceConditionalProbs;
}

inline void KWPSTargetIntervalScore::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

inline int KWPSTargetIntervalScore::GetFrequency() const
{
	return nFrequency;
}

inline void KWPSTargetIntervalScore::SetCumulativeFrequency(int nValue)
{
	require(nValue >= 0);
	nCumulativeFrequency = nValue;
}

inline int KWPSTargetIntervalScore::GetCumulativeFrequency() const
{
	return nCumulativeFrequency;
}

inline void KWPSTargetIntervalScore::SetRefCount(int nValue)
{
	require(nValue >= 0);
	nRefCount = nValue;
}

inline int KWPSTargetIntervalScore::GetRefCount() const
{
	return nRefCount;
}
