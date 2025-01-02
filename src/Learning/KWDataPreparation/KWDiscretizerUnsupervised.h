// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDiscretizer.h"
#include "KWStat.h"
#include "KWQuantileBuilder.h"

//////////////////////////////////////////////////////////////////////////////////
// Algorithme de discretisation utilisant les valeurs de l'attribut source
// Classe ancetre des methode de discretisation non-supervisees
class KWDiscretizerUsingSourceValues : public KWDiscretizer
{
public:
	// Constructeur
	KWDiscretizerUsingSourceValues();
	~KWDiscretizerUsingSourceValues();

	// Utilisation des valeurs sources pour la discretisation. Ici: true
	boolean IsUsingSourceValues() const override;

	// Verification des parametres
	boolean Check() const override;

	// Libelles utilisateur
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	////////////////////////////////////////////////////////////////////////
	// Methodes utilitaires d'interet general pour l'ensemble des methodes
	// de discretisation non supervisees

	// Discretisation en intervalle de frequence egale (sinon largeur egale)
	// Les eventuels intervalles vides sont fusionnes.
	void EqualBinsDiscretizeValues(boolean bIsEqualFrequency, ContinuousVector* cvSourceValues,
				       IntVector* ivTargetIndexes, int nTargetValueNumber,
				       KWFrequencyTable*& kwftTarget) const;

	// Calcul des vecteurs de frequences cumulees par modalite cible
	// Ces vecteurs sont indexes par les index des valeurs sources, et correspondent
	// aux cumuls des valeur cible pour la derniere instance de chaque valeur source unique.
	// L'interet de memoriser les cumuls est de pouvoir calculer les frequences cibles
	// d'un intervalle simplement par difference des cumuls entre ses bornes.
	// L'interet d'utiliser les index de valeurs plutot que d'instance est qu'il y a
	// souvent beaucoup moins de valeurs sources differentes que d'instances, ce qui
	// permet d'optimsier les traitements
	//    En entree: le quantileBuilder initialise a partir des valeurs source des instances
	//               le tableau correspondant des valeurs cible des instances
	//               le nombre de valeurs cibles
	//    En sortie: le tableau des vecteurs de frequences cible cumulees
	//                  (chaque vecteur est indexe par l'index des valeurs sources,
	//                   et est donc de taille ValueNumber et non InstanceNumber)
	// Memoire: les vecteurs de frequences cumules par cible sont creees dans le tableau en sortie,
	// qui doit etre initialement vide
	void ComputeCumulativeTargetFrequencies(KWQuantileIntervalBuilder* quantileBuilder, IntVector* ivTargetIndexes,
						int nTargetValueNumber,
						ObjectArray* oaCumulativeTargetFrequencies) const;

	// Creation de la table d'effectifs resultat a partir d'une discretisation d'un quantileBuilder
	// Toutes les entrees sont disponibles grace a la methode ComputeCumulativeTargetFrequencies
	// La table d'effectif est fabriquee en sortie
	void ComputeTargetFrequencyTable(KWQuantileIntervalBuilder* quantileBuilder,
					 ObjectArray* oaCumulativeTargetFrequencies,
					 KWFrequencyTable*& kwftTarget) const;
};

//////////////////////////////////////////////////////////////////////////////////
// Algorithme EqualWidth de discretisation non supervises en intervalles
// de largeur egale (dont le nombre est parametre par MaxIntervalNumber)
class KWDiscretizerEqualWidth : public KWDiscretizerUsingSourceValues
{
public:
	// Constructeur
	KWDiscretizerEqualWidth();
	~KWDiscretizerEqualWidth();

	// Nom de l'algorithme
	const ALString GetName() const;

	// Constructeur generique
	KWDiscretizer* Create() const;

	// Discretisation
	// Les eventuels intervalles vides sont fusionnes.
	void DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes, int nTargetValueNumber,
			      KWFrequencyTable*& kwftTarget) const;
};

//////////////////////////////////////////////////////////////////////////////////
// Algorithme EqualFrequency de discretisation non supervises en intervalles
// de frequence egale (dont le nombre est parametre par MaxIntervalNumber)
class KWDiscretizerEqualFrequency : public KWDiscretizerUsingSourceValues
{
public:
	// Constructeur
	KWDiscretizerEqualFrequency();
	~KWDiscretizerEqualFrequency();

	// Nom de l'algorithme
	const ALString GetName() const;

	// Constructeur generique
	KWDiscretizer* Create() const;

	// Discretisation
	// Chaque intervalle etant defini par ses bornes, il ne peut y avoir
	// au final moins d'intervalles que demandes si les decoupages par frequence
	// tombent sur des valeurs doublons en grand  nombre
	void DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes, int nTargetValueNumber,
			      KWFrequencyTable*& kwftTarget) const;
};

//////////////////////////////////////////////////////////////////////////////////
// Algorithme MODL de discretisation non supervises en intervalles de frequence egale
// ou largeur egale
// Le nombre d'intervalles est determine automatiquement pour maximiser la valeur
// predictive de l'attribut discretise par rapport a la variable cible
// Le parametre MaxIntervalNumber peut etre utilise pour borner le nombre d'intervalles
class KWDiscretizerMODLEqualBins : public KWDiscretizerUsingSourceValues
{
public:
	// Constructeur
	KWDiscretizerMODLEqualBins();
	~KWDiscretizerMODLEqualBins();

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Discretisation par minimisation d'un critere de cout, en intervalle
	// de frequence egale (sinon de largeur egale)
	void MODLEqualBinsDiscretizeValues(boolean bIsEqualFrequency, ContinuousVector* cvSourceValues,
					   IntVector* ivTargetIndexes, int nTargetValueNumber,
					   KWFrequencyTable*& kwftTarget) const;

	// Calcul du cout d'une partition en n intervalles
	virtual double ComputePartitionCost(int nIntervalNumber, int nValueNumber) const;
	virtual double OldComputePartitionCost(int nIntervalNumber) const;

	// Calcul du cout total d'un intervalle
	virtual double ComputeIntervalCost(IntVector* ivFrequencyVector) const;

	// Indique si tous les couts sont positifs (par defaut: true)
	// (permet des optimisation de l'algorithme)
	boolean bAreCostPositive;
};

//////////////////////////////////////////////////////////////////////////////////
// Algorithme MODLEqualWidth de discretisation non supervises en intervalles
// de frequence egale
// Le nombre d'intervalles est determine automatiquement pour maximiser la valeur
// predictive de l'attribut discretise par rapport a la variable cible
// Le parametre MaxIntervalNumber peut etre utilise pour borner le nombre d'intervalles
class KWDiscretizerMODLEqualWidth : public KWDiscretizerMODLEqualBins
{
public:
	// Constructeur
	KWDiscretizerMODLEqualWidth();
	~KWDiscretizerMODLEqualWidth();

	// Nom de l'algorithme
	const ALString GetName() const;

	// Constructeur generique
	KWDiscretizer* Create() const;

	// Discretisation par minimisation d'un critere de cout
	// Chaque intervalle etant defini par ses bornes, il ne peut y avoir
	// au final moins d'intervalles que demandes si les decoupages par frequence
	// tombent sur des valeurs doublon en grand  nombre
	void DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes, int nTargetValueNumber,
			      KWFrequencyTable*& kwftTarget) const;
};

//////////////////////////////////////////////////////////////////////////////////
// Algorithme MODLEqualFrequency de discretisation non supervises en intervalles
// de frequence egale
// Le nombre d'intervalles est determine automatiquement pour maximiser la valeur
// predictive de l'attribut discretise par rapport a la variable cible
// Le parametre MaxIntervalNumber peut etre utilise pour borner le nombre d'intervalles
class KWDiscretizerMODLEqualFrequency : public KWDiscretizerMODLEqualBins
{
public:
	// Constructeur
	KWDiscretizerMODLEqualFrequency();
	~KWDiscretizerMODLEqualFrequency();

	// Nom de l'algorithme
	const ALString GetName() const;

	// Constructeur generique
	KWDiscretizer* Create() const;

	// Discretisation par minimisation d'un critere de cout
	// Chaque intervalle etant defini par ses bornes, il peut y avoir
	// au final moins d'intervalles que demandes si les decoupages par frequence
	// tombent sur des valeurs doublon en grand  nombre
	void DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes, int nTargetValueNumber,
			      KWFrequencyTable*& kwftTarget) const;
};
