// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KDConstructionRule.h"
#include "KWDRPreprocessing.h"
#include "KWDRTablePartition.h"
#include "KWDRTableBlock.h"

////////////////////////////////////////////////////////////
// Classe KDConstructionDomain
// Specification des regles de construction d'un domaine
class KDConstructionDomain : public Object
{
public:
	// Constructeur
	KDConstructionDomain();
	~KDConstructionDomain();

	// Nommage des variables de facon interpretable
	//   . true (defaut): un nom parlant est cree
	//   . false: les variables sont nommee par numerotation
	boolean GetInterpretableNames() const;
	void SetInterpretableNames(boolean bValue);

	// Optimisation de la classe construite par creation de variables intermediaires (defaut: true)
	boolean GetRuleOptimization() const;
	void SetRuleOptimization(boolean bValue);

	// Optimisation de la classe construite par creation de variables sparse (defaut: true)
	boolean GetSparseOptimization() const;
	void SetSparseOptimization(boolean bValue);

	// Taille minimale des block sparse generes (sinon, on ne genere pas en mdoe sparse (defaut: 0)
	int GetSparseBlockMinSize() const;
	void SetSparseBlockMinSize(int nValue);

	// Import des cout de construction depuis le dictionnaire en cours (Cost des meta-donnees des attributs)
	boolean GetImportAttributeConstructionCosts() const;
	void SetImportAttributeConstructionCosts(boolean bValue);

	// Prise en compte de la regularisation pour la construction (par defaut: true)
	boolean GetConstructionRegularization() const;
	void SetConstructionRegularization(boolean bValue);

	//////////////////////////////////////////////////////////////////////////
	// Ensemble des regles de construction de variable utilisables
	// Chaque regle de construction est represente par une regle de derivation
	// Chaque regle de construction a un tag d'utilisation permettant de
	// selectionner celle a utiliser sur un domaine particulier
	// Memoire: toutes les regles de construction appartiennent au domaine de construction

	// Initialisation (effectuee par defaut) avec un ensemble de regles de construction standard
	void InitializeStandardConstructionRules();

	// Selection des regles de construction a utiliser par defaut
	// Par defaut, on ne selectionne pas les regles de construction temporelles
	void SelectDefaultConstructionRules();

	// Recherche d'une regle (retourne NULL si echec)
	KDConstructionRule* LookupConstructionRule(const ALString& sName) const;

	// Insertion (echec si classe de meme nom existante)
	boolean InsertConstructionRule(KDConstructionRule* constructionRule);

	// Destruction
	boolean DeleteConstructionRule(const ALString& sName);

	// Acces massifs
	int GetConstructionRuleNumber() const;
	KDConstructionRule* GetConstructionRuleAt(int i) const;

	// Test si la regle de selection est utilisee
	boolean IsSelectionRuleUsed() const;

	// Destruction de toutes les regles de construction
	void DeleteAllConstructionRules();

	// Tableau des regles de construction disponibles
	ObjectArray* GetConstructionRules();

	//////////////////////////////////////////////////////////////////////////
	// Services standard

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Initialisation d'une regle de construction a partir d'une regle de derivation,
	// de sa regle de de partition correspondante et de sa regle de bloc de valeur correspondante
	// Ces deux dernier parametres peutvent etre a NULL s'il ne sont pas pertinent
	// Renvoie la regle de construction creee
	// Memoire: les regles de derivation en parametre appartiennent a la regle de construction cree
	KDConstructionRule* InitializeConstructionRule(const ALString& sFamilyName,
						       const KWDerivationRule* derivationRule,
						       const KWDRTablePartitionStats* partitionStatsRule,
						       const KWDRValueBlockRule* valueBlockRule);

	// Compteur de mise a jour
	// Permet de verifier la "fraicheur" de l'indexation des regles de construction
	int nUpdateNumber;

	//  Fraicheur d'indexation des classes
	mutable int nAllConstructionRulesFreshness;

	// Dictionnaire des regles de construction
	ObjectDictionary odConstructionRules;

	// Gestion indexee des regle de construction
	ObjectArray* AllConstructionRules() const;
	mutable ObjectArray oaConstructionRules;

	// Parametres generaux de la construction de variables
	int nSparseBlockMinSize;
	boolean bInterpretableNames;
	boolean bRuleOptimization;
	boolean bSparseOptimization;
	boolean bImportAttributeConstructionCosts;
	boolean bConstructionRegularization;
};
