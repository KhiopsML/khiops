// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KIDRClassifierReinforcer;
class KIDRReinforcementInitialScoreAt;
class KIDRReinforcementAttributeAt;
class KIDRReinforcementPartAt;
class KIDRReinforcementFinalScoreAt;
class KIDRReinforcementClassChangeTagAt;

#include "KIDRInterpretation.h"

// Enregistrement des regles liee au renforcement des modeles
void KIDRRegisterReinforcementRules();

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierReinforcer
// Service d'interpretation d'un classifieur
class KIDRClassifierReinforcer : public KIDRClassifierService
{
public:
	// Constructeur
	KIDRClassifierReinforcer();
	~KIDRClassifierReinforcer();

	// Creation
	KWDerivationRule* Create() const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a un objet, et services associes

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Score de renforcement initial pour une valeur cible
	Continuous GetReinforcementInitialScoreAt(Symbol sTargetValue) const;

	// Nom de variable de renforcement pour une valeur cible et un rang de variable
	Symbol GetRankedReinforcementAttributeAt(Symbol sTargetValue, int nAttributeRank) const;

	// Nom de partie de variable de renforcement pour une valeur cible et un rang de variable
	Symbol GetRankedReinforcementPartAt(Symbol sTargetValue, int nAttributeRank) const;

	// Score de renforcement pour une valeur cible et un rang de variable
	Continuous GetRankedReinforcementFinalScoreAt(Symbol sTargetValue, int nAttributeRank) const;

	// Indicateur de changement de classe suite a unrenforcement pour une valeur cible et un rang de variable
	Continuous GetRankedReinforcementClassChangeTagAt(Symbol sTargetValue, int nAttributeRank) const;

	////////////////////////////////////////////////////////////////////
	// Services divers

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Nettoyage
	void Clean() override;

	// Calcul de toutes les information de renforcement triees pour les acces aux contributions par rang,
	// pour une valeur cible
	void ComputeRankedReinforcementAt(int nTarget) const;

	// Vecteur de score initial par valeur cible
	mutable ContinuousVector cvInitialScores;
};

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementInitialScoreAt
// Donne la valeur de renforcement initiale pour une valeur cible
// a partir d'un renforceur
class KIDRReinforcementInitialScoreAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementInitialScoreAt();
	~KIDRReinforcementInitialScoreAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementAttributeAt
// Donne le nom de la variable de renforcement pour une valeur cible
// et un rang de variable a partir d'un renforceur
class KIDRReinforcementAttributeAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementAttributeAt();
	~KIDRReinforcementAttributeAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementPartAt
// Donne la partie de la variable de renforcement pour une valeur cible
// et un rang de variable a partir d'un renforceur
class KIDRReinforcementPartAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementPartAt();
	~KIDRReinforcementPartAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementFinalScoreAt
// Donne la valeur de renforcement finale apres reinforcement pour une valeur cible
// et un rang de variable a partir d'un renforceur
class KIDRReinforcementFinalScoreAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementFinalScoreAt();
	~KIDRReinforcementFinalScoreAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementClassChangeTagAt
// Donne le tag de renforcement pour une valeur cible
// et un rang de variable a partir d'un renforceur
class KIDRReinforcementClassChangeTagAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRReinforcementClassChangeTagAt();
	~KIDRReinforcementClassChangeTagAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};
