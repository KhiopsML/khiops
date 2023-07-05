// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour le deploiement des modeles predictifs
// Les classes KWDRClassifier et KWDRRegressor sont des classes virtuelles
// a redefinir pour specialiser un predicteur.
// Ces classe de type Structure renvoie une structure "Classifier" ou "Regressor"
// (la regle elle meme), qui donne acces a toutes les methodes de prediction.
// Les services de prediction sont des regles generique, prenant en premier argument
// la structure du predicteur.

// Specification d'un classifieur
class KWDRClassifier;

// Services de prediction sur les valeurs Symbol
class KWDRTargetValue;
class KWDRTargetProb;
class KWDRTargetProbAt;
class KWDRBiasedTargetValue;

// Specification d'un regresseur de rangs
class KWDRRankRegressor;

// Services de prediction sur les rangs
class KWDRTargetRankMean;
class KWDRTargetRankStandardDeviation;
class KWDRTargetRankDensityAt;
class KWDRTargetRankCumulativeProbAt;

// Specification d'un regresseur
class KWDRRegressor;

// Services de prediction sur les valeurs
class KWDRTargetMean;
class KWDRTargetStandardDeviation;
class KWDRTargetDensityAt;
class KWDRTargetQuantileDistribution;

#include "KWDerivationRule.h"
#include "KWStructureRule.h"
#include "KWDRPreprocessing.h"
#include "KWDataGrid.h"

// Enregistrement de ces regles
void KWDRRegisterPredictorRules();

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRClassifier
// Renvoie une structure "Classifier"
// Methodes virtuelles a definir dans des sous classes
// Par defaut, le classifieur renvoie systematiquement ""
class KWDRClassifier : public KWDerivationRule
{
public:
	// Constructeur
	KWDRClassifier();
	~KWDRClassifier();

	// Creation
	KWDerivationRule* Create() const override;

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile

	// Valeur predite (defaut: renvoie "")
	virtual Symbol ComputeTargetValue() const;

	// Probabilite de la valeur predite (defaut: renvoie 1)
	virtual Continuous ComputeTargetProb() const;

	// Probabilite d'une valeur particuliere (defaut: 1 si "", 0 sinon)
	virtual Continuous ComputeTargetProbAt(const Symbol& sValue) const;

	// Valeur predite en prenant en compte un biais (defaut: renvoie "")
	// Chaque probabilite conditionnelle (ou score) est modifiee par un offset,
	// avant d'en prendre le max
	// La methode est tolerante a un vecteur de taille differente du nombre de
	// valeurs cibles: les valeurs en trop sont ignoree, celles en moins sont
	// considerees comme nulles
	virtual Symbol ComputeBiasedTargetValue(const ContinuousVector* cvOffsets) const;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetValue: valeur predite (la plus probable)
//  Regle ayant un argument de type Structure(KWDRClassifier)
class KWDRTargetValue : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetValue();
	~KWDRTargetValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetProb: proba de la valeur predite
//  Regle ayant un argument de type Structure(KWDRClassifier)
class KWDRTargetProb : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetProb();
	~KWDRTargetProb();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetProbAt: proba d'une valeur
//  Regle ayant un argument de type Structure(KWDRClassifier)
class KWDRTargetProbAt : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetProbAt();
	~KWDRTargetProbAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRBiasedTargetValue: valeur predite en prenant en compte un biais
//  Regle ayant un argument de type Structure(KWDRClassifier) et
//   un vecteur d'offsets (KWDRContinuousVEctor) par valeur cible
class KWDRBiasedTargetValue : public KWDerivationRule
{
public:
	// Constructeur
	KWDRBiasedTargetValue();
	~KWDRBiasedTargetValue();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRRankRegressor
// Les rang consideres sont des rangs normalise entre 0 et 1
// Renvoie une structure "RankRegressor"
// Methodes virtuelles a definir dans des sous classes
// Par defaut, le regresseur de rang suppose une densite uniforme sur les rangs
class KWDRRankRegressor : public KWDerivationRule
{
public:
	// Constructeur
	KWDRRankRegressor();
	~KWDRRankRegressor();

	// Creation
	KWDerivationRule* Create() const override;

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile

	// Rang moyen predit (defaut: renvoie 0.5)
	virtual Continuous ComputeTargetRankMean() const;

	// Ecart type predit (defaut: renvoie racine(1/12))
	virtual Continuous ComputeTargetRankStandardDeviation() const;

	// Densite predite (defaut: renvoie 1)
	virtual Continuous ComputeTargetRankDensityAt(Continuous cRank) const;

	// Fonction de repartition cumulative predite: prob([0, cRank] (defaut: renvoie cRank)
	virtual Continuous ComputeTargetRankCumulativeProbAt(Continuous cRank) const;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetRankMean: rang moyen
//  Regle ayant un argument de type Structure(RankRegressor)
class KWDRTargetRankMean : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetRankMean();
	~KWDRTargetRankMean();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetRankStandardDeviation: ecart type du rang
//  Regle ayant un argument de type Structure(RankRegressor)
class KWDRTargetRankStandardDeviation : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetRankStandardDeviation();
	~KWDRTargetRankStandardDeviation();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetRankDensityAt: densite de rang
//  Regle ayant un argument de type Structure(RankRegressor)
class KWDRTargetRankDensityAt : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetRankDensityAt();
	~KWDRTargetRankDensityAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetRankCumulativeProbAt: fonction cumulative des rangs
//  Regle ayant un argument de type Structure(RankRegressor)
class KWDRTargetRankCumulativeProbAt : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetRankCumulativeProbAt();
	~KWDRTargetRankCumulativeProbAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRRegressor
// Renvoie une structure "Regressor"
// Methodes virtuelles a definir dans des sous classes
// Par defaut, le regresseur de rang suppose une densite uniforme sur les rangs
class KWDRRegressor : public KWDerivationRule
{
public:
	// Constructeur
	KWDRRegressor();
	~KWDRRegressor();

	// Creation
	KWDerivationRule* Create() const override;

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile

	// Rang moyen predit (defaut: renvoie 0)
	virtual Continuous ComputeTargetMean() const;

	// Ecart type predit (defaut: renvoie 0)
	virtual Continuous ComputeTargetStandardDeviation() const;

	// Densite predite (defaut: renvoie 0)
	virtual Continuous ComputeTargetDensityAt(Continuous cValue) const;

	// Distribution des quantiles au format texte suivant:
	//   a_1 q_1 a_2 q_2 ... a_n q_n
	// avec 0 < a_k < a_{k+1} < 1  et  p(y < q_k | x) = a_k
	// Par defaut: renvoie ""
	virtual Symbol ComputeTargetQuantileDistribution() const;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetMean: valeur moyen
//  Regle ayant un argument de type Structure(Regressor)
class KWDRTargetMean : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetMean();
	~KWDRTargetMean();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetStandardDeviation: ecart type
//  Regle ayant un argument de type Structure(Regressor)
class KWDRTargetStandardDeviation : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetStandardDeviation();
	~KWDRTargetStandardDeviation();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetDensityAt: densite de rang
//  Regle ayant un argument de type Structure(Regressor)
class KWDRTargetDensityAt : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetDensityAt();
	~KWDRTargetDensityAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////////////////////
// Classe KWDRTargetQuantileDistribution: densite de rang
//  Regle ayant un argument de type Structure(Regressor)
class KWDRTargetQuantileDistribution : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTargetQuantileDistribution();
	~KWDRTargetQuantileDistribution();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};