// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

///////////////////////////////////////////////////////////////////////////
// Regles de derivation portant sur des operations mathematiques
// De facon generale, toute regle ayant un argument MissingValue
// ou erronne (negatif pour un log par exemple) renvoie MissingValue

class KWDRFormatContinuous;
class KWDRSum;
class KWDRMinus;
class KWDRDiff;
class KWDRProduct;
class KWDRDivide;
class KWDRIndex;
class KWDRRound;
class KWDRFloor;
class KWDRCeil;
class KWDRAbs;
class KWDRSign;
class KWDRMod;
class KWDRLog;
class KWDRExp;
class KWDRPower;
class KWDRSqrt;
class KWDRPi;
class KWDRSin;
class KWDRCos;
class KWDRTan;
class KWDRASin;
class KWDRACos;
class KWDRATan;
class KWDRMean;
class KWDRStandardDeviation;
class KWDRMin;
class KWDRMax;
class KWDRArgMin;
class KWDRArgMax;

#include "KWDerivationRule.h"

// Enregistrement de ces regles
void KWDRRegisterMathRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRFormatContinuous
// Formatage d'un attribut continu en imposant un nombre de chiffres minimum
// avant la virgule, et un nombre de chiffres exact apres la virgule,
// avec completion eventuelle avec des 0.
class KWDRFormatContinuous : public KWDerivationRule
{
public:
	// Constructeur
	KWDRFormatContinuous();
	~KWDRFormatContinuous();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSum
// Somme de un, deux (ou plus) attributs Continuous
class KWDRSum : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSum();
	~KWDRSum();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRMinus
// Oppose d'un attribut Continuous
class KWDRMinus : public KWDerivationRule
{
public:
	// Constructeur
	KWDRMinus();
	~KWDRMinus();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDiff
// Difference de deux attributs Continuous
class KWDRDiff : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDiff();
	~KWDRDiff();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRProduct
// Produit de un, deux (ou plus) attributs Continuous
class KWDRProduct : public KWDerivationRule
{
public:
	// Constructeur
	KWDRProduct();
	~KWDRProduct();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRDivide
// Quotient de deux attributs Continuous
// Le cas du denominateur nul est gere
class KWDRDivide : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDivide();
	~KWDRDivide();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRIndex
// Index de creation d'un objet, en general associe au numero de ligne dont
// est issu un objet lors de la lecture d'une table de donnees
class KWDRIndex : public KWDerivationRule
{
public:
	// Constructeur
	KWDRIndex();
	~KWDRIndex();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRRound
// Entier le plus proche d'un attribut Continuous
class KWDRRound : public KWDerivationRule
{
public:
	// Constructeur
	KWDRRound();
	~KWDRRound();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRFloor
// Entier inferieur le plus proche d'un attribut Continuous
class KWDRFloor : public KWDerivationRule
{
public:
	// Constructeur
	KWDRFloor();
	~KWDRFloor();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCeil
// Entier superieur le plus proche d'un attribut Continuous
class KWDRCeil : public KWDerivationRule
{
public:
	// Constructeur
	KWDRCeil();
	~KWDRCeil();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAbs
// Valeur absolue d'un attribut Continuous
class KWDRAbs : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAbs();
	~KWDRAbs();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSign
// Signe d'un attribut Continuous
// Renvoie 1 pour les valeurs positives ou nulles, -1 pour les valeurs negatives
class KWDRSign : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSign();
	~KWDRSign();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRMod
// Modulo d'un attribut Continuous par un second attribut Continuous
// Renvoie x - y * floor(x / y)
class KWDRMod : public KWDerivationRule
{
public:
	// Constructeur
	KWDRMod();
	~KWDRMod();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRLog
// Logarithme neperien d'un attribut Continuous
// Renvoie MissingValue pour les valeur negative ou nulles
class KWDRLog : public KWDerivationRule
{
public:
	// Constructeur
	KWDRLog();
	~KWDRLog();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRExp
// Exponentielle d'un attribut Continuous
class KWDRExp : public KWDerivationRule
{
public:
	// Constructeur
	KWDRExp();
	~KWDRExp();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRPower
// Valeur d'un attribut elevee a la puissance d'un second attribut
class KWDRPower : public KWDerivationRule
{
public:
	// Constructeur
	KWDRPower();
	~KWDRPower();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSqrt
// Racine carree d'un attribut
class KWDRSqrt : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSqrt();
	~KWDRSqrt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRPi
// Constante Pi
class KWDRPi : public KWDerivationRule
{
public:
	// Constructeur
	KWDRPi();
	~KWDRPi();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSin
// Sinus d'un attribut
class KWDRSin : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSin();
	~KWDRSin();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCos
// Cosinus d'un attribut
class KWDRCos : public KWDerivationRule
{
public:
	// Constructeur
	KWDRCos();
	~KWDRCos();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTan
// Tangente d'un attribut
class KWDRTan : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTan();
	~KWDRTan();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRASin
// Arc sinus d'un attribut
class KWDRASin : public KWDerivationRule
{
public:
	// Constructeur
	KWDRASin();
	~KWDRASin();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRACos
// Arc cosinus d'un attribut
class KWDRACos : public KWDerivationRule
{
public:
	// Constructeur
	KWDRACos();
	~KWDRACos();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRATan
// Arc tangente d'un attribut
class KWDRATan : public KWDerivationRule
{
public:
	// Constructeur
	KWDRATan();
	~KWDRATan();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRMean
// Moyenne des valeurs d'une serie Continuous, en ignorant les Missing
class KWDRMean : public KWDerivationRule
{
public:
	// Constructeur
	KWDRMean();
	~KWDRMean();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRStandardDeviation
// Ecart type des valeurs d'une serie Continuous, en ignorant les Missing
class KWDRStandardDeviation : public KWDerivationRule
{
public:
	// Constructeur
	KWDRStandardDeviation();
	~KWDRStandardDeviation();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRMin
// Plus faible valeur dans une serie Continuous, en ignorant les Missing
class KWDRMin : public KWDerivationRule
{
public:
	// Constructeur
	KWDRMin();
	~KWDRMin();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRMax
// Plus forte valeur dans une serie Continuous, en ignorant les Missing
class KWDRMax : public KWDerivationRule
{
public:
	// Constructeur
	KWDRMax();
	~KWDRMax();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRArgMin
// Rang du champ ayant la plus faible valeur dans
// une serie Continuous (en partant de la fin)
class KWDRArgMin : public KWDerivationRule
{
public:
	// Constructeur
	KWDRArgMin();
	~KWDRArgMin();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRArgMax
// Rang du champ ayant la plus forte valeur dans une
// serie Continuous (en partant de la fin)
class KWDRArgMax : public KWDerivationRule
{
public:
	// Constructeur
	KWDRArgMax();
	~KWDRArgMax();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};
