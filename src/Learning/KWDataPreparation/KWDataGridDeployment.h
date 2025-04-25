// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGridDeployment;
class KWDGDAttribute;
class KWDGDPart;

#include "KWDataGrid.h"
#include "KWDataGridMerger.h"
#include "KWDataGridCosts.h"

////////////////////////////////////////////////////////////////////////////
// Service de deploiement de distribution dans une grille de de donnees,
// constituee d'un attribut de deploiement (par exemple des textes ou des courbes)
// et d'un ou plusieurs attributs de distribution (par exemple des mots ou des points).
// On cherche a deployer un individu caracterise par sa distribution sur
// les attributs de distribution, en le rapporchant de son cluster de deploiement
// ayant la distribution la plus proche.
// On dispose de distribution de valeurs, que l'on transforme en
// vecteur d'effectifs sur les parties des attributs de distribution.
// Il s'agit de rechercher la partie de l'attribut de deploiement la plus
// proche d'un pour cette distribution de valeurs.
// Pour cela, on considere les distribution de valeurs comme une nouvelle
// partie de l'attribut de deploiement (par exemple: un texte caracterise
// par ses effectifs sur les groupes de mots) et on tente de fusionner
// cette nouvelle partie sur chaque partie de deploiement existante.
// On retient la partie qui apres fusion entraine un impact minimum sur
// le critere d'evaluation du coclustering
// PLusieurs services de dploiement sont disponibles:
//   . cluster de deploiement le plus probable
//   . vecteur des distance a tous les clusters de deploiement
//   . vecteur d'effectif par cluster pour les attributs de distribution
//   ...
class KWDataGridDeployment : public KWDataGridMerger
{
public:
	KWDataGridDeployment();
	~KWDataGridDeployment();

	// Parametrage de l'attribut de deploiement (par defaut: index -1)
	void SetDeploymentAttributeIndex(int nValue);
	int GetDeploymentAttributeIndex() const;

	// Acces a l'attribut de deploiement
	KWDGAttribute* GetDeploymentAttribute();

	// Preparation au deploiement une fois la grille de de donnees parametree (cf. classe ancetre)
	//  initialisation des couts, de la structure d'indexation...
	void PrepareForDeployment();

	// Nettoyage de la preparation
	void Clean();

	// Calcul des statistiques de deploiement pour une distribution
	// caracterise par un vecteur de k-upplet sur les k attributs de distribution
	// Techniquement, on utilise un tableau de (k+1) vecteur de valeurs (ContinuousVector ou SymbolVector)
	// avec les index des attributs de distribution (l'index de l'attribut de deploiement n'est pas
	// utilise et doit etre utilise avec NULL pour le vecteur de valeurs)
	// Le vecteur de frequence permet de multiplier le nombre de valeur. Ce parametre est optionnel, et ignore si
	// NULL.
	void ComputeDeploymentStats(const ObjectArray* oaDistributionValueVectors, const IntVector* ivFrequencyVector);
	boolean IsDeploymentStatsComputed() const;

	///////////////////////////////////////////////////////////////////////////////
	// Acces aux resultats de deploiement

	// Index de la partie de deploiement la plus proche
	int GetDeploymentIndex() const;

	// Effectifs sur les parties des attributs de distribution
	// Memoire: le vecteur resultat appartient a l'appele
	const IntVector* GetDistributionFrequenciesAt(int nDistributionAttributeIndex) const;

	// Distances aux parties de l'attribut de deploiement
	// Memoire: le vecteur resultat appartient a l'appele
	const DoubleVector* GetDeploymentDistances() const;

	// Nombre de valeurs de la distribution
	int GetDistributionTotalFrequency() const;

	// Verification de la preparation au deploiement
	boolean CheckDeploymentPreparation() const;

	// Test d'integrite
	// Attention: la classe n'est pour l'instant implementee que dans le cas d'un coclustering de
	// deux variables categorielles
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	KWDGAttribute* NewAttribute() const override;

	// Initialisation des index des parties
	void InitializePartIndexes();

	// Structure de cout de grille non supervise pour l'evaluation des variations de couts
	KWDataGridClusteringCosts dataGridCosts;

	// Nouvelle partie de deploiement destinee a accueuillir les distribution a deployer
	KWDGMPart* dgNewDeploymentPart;

	// Index de l'attribut de deploiement
	int nDeploymentAttributeIndex;

	// Resultats de deploiement
	int nDeploymentIndex;
	ObjectArray oaDistributionFrequencyVectors;
	DoubleVector dvDeploymentDistances;
	int nDistributionTotalFrequency;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGDAttribute
// Attribut d'un DataGridDeployment
class KWDGDAttribute : public KWDGMAttribute
{
public:
	// Constructeur
	KWDGDAttribute();
	~KWDGDAttribute();

	///////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	KWDGPart* NewPart() const override;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGDPart
// Partie d'un DataGridDeployment
class KWDGDPart : public KWDGMPart
{
public:
	// Constructeur
	KWDGDPart();
	~KWDGDPart();

	// Index de la partie
	void SetIndex(int nValue);
	int GetIndex() const;

	///////////////////////////////
	///// Implementation
protected:
	int nIndex;
};
