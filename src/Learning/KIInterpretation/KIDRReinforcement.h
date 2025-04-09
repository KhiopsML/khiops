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
class KIAttributeReinforcement;

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

	// Verification que la regle est completement renseignee
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////
	// Acces aux caracteristiques du renforceur

	// Noms des variables de renforcement
	int GetReinforcementAttributeNumber() const;
	const ALString& GetReinforcementAttributeNameAt(int nAttribute) const;

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

	// Indicateur de changement de classe suite a un renforcement pour une valeur cible et un rang de variable
	Continuous GetRankedReinforcementClassChangeTagAt(Symbol sTargetValue, int nAttributeRank) const;

	////////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage des caracteristique detaillees du renforceur
	void WriteDetails(ostream& ost) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Nettoyage
	void Clean() override;

	// Creation des structures des gestion des contributions pour les acces par rang
	void CreateRankedReinforcementStructures(int nTargetValueNumber, int nAttributeNumber,
						 const StringVector* svAttributeNames);

	// Calcul de toutes les informations de renforcement triees pour les acces aux contributions par rang
	void ComputeRankedReinforcements() const;

	// Calcul du renforcement pour unne valeur cible et un attribut donne
	// La methode renseigne toutes les information de renforcement de l'objet en parametre
	void ComputeReinforcementAt(KIAttributeReinforcement* attributeReinforcement, int nTargetIndex,
				    int nAttributeIndex, int nInitialPredictedTargetIndex) const;

	// Calcul de l'arg max d'un vecteur de scores
	int ComputeArgMaxScores(const ContinuousVector* cvScores) const;

	// Renforcement par valeur cible et par rang
	const KIAttributeReinforcement* GetRankedReinforcementAt(int nTarget, int nAttributeRank) const;

	// Vecteur des index des variables e renforcement
	mutable IntVector ivReinforcementAttributeIndexes;

	// Tableau par valeur cible de tableau de KIAttributeReinforcement, tries par score final decroissant
	ObjectArray oaTargetValueRankedAttributeReinforcements;

	// Vecteur d'indicateurs memorisant le besoin de calculer les structures de renforcement par valeur cible
	// Ce vecteur est alimente au fur et a mesure de l'execution globale, et sert a piloter les calcul de renforcement
	mutable IntVector ivTargetValueReinforcementNeeded;

	// Vecteur d'indicateurs memorisant que les structures de renforcement par valeur cible ont ete calculees
	// Ce vecteur est alimente au fur et a mesure de l'execution individuelle par instance, et sert a bufferiser
	// les calcul effectues
	mutable IntVector ivTargetValueReinforcementComputed;
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

////////////////////////////////////////////////////////////
// Classe KIAttributeReinforcement
// Memorisation d'un index d'attribut et de ses information de renforcement,
// permettant ensuite un tri par contribution decroissante
class KIAttributeReinforcement : public Object
{
public:
	// Constructeur
	KIAttributeReinforcement();
	~KIAttributeReinforcement();

	// Index de l'attribut
	void SetAttributeIndex(int nValue);
	int GetAttributeIndex() const;

	// Index de la partie utilise pour le renforcement
	void SetReinforcementPartIndex(int nValue);
	int GetReinforcementPartIndex() const;

	// Score final apres renforcement
	// Un score de 0 indique qu'aucun renforcement n'a ete obtenu
	void SetReinforcementFinalScore(Continuous cValue);
	Continuous GetReinforcementFinalScore() const;

	// Indique si la classe a change apres renforcement
	// - 0: la classe etait deja la bonne
	// - -1: pas de changement de classe
	// - 1: changement de classe
	void SetReinforcementClassChangeTag(int nValue);
	int GetReinforcementClassChangeTag() const;

	// Parametrage des noms des attributs
	// Memoire: appartient a l'appelant
	void SetAttributeNames(const StringVector* svNames);
	const StringVector* GetAttributeNames() const;

	// Nom de l'attribut correspondant a son index
	const ALString& GetAttributeName() const;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Variables de la classe
	int nAttributeIndex;
	int nReinforcementPartIndex;
	Continuous cReinforcementFinalScore;
	int nReinforcementClassChangeTag;
	const StringVector* svAttributeNames;
};

// Methode de comparaison par score final decroissant
int KIAttributeReinforcementCompare(const void* elem1, const void* elem2);

////////////////////////////////////
// Methodes en inline

inline int KIDRClassifierReinforcer::GetReinforcementAttributeNumber() const
{
	require(IsCompiled());
	return ivReinforcementAttributeIndexes.GetSize();
}

inline const ALString& KIDRClassifierReinforcer::GetReinforcementAttributeNameAt(int nAttribute) const
{
	require(IsCompiled());
	require(0 <= nAttribute and nAttribute < GetReinforcementAttributeNumber());
	return GetPredictorAttributeNameAt(ivReinforcementAttributeIndexes.GetAt(nAttribute));
}

inline const KIAttributeReinforcement* KIDRClassifierReinforcer::GetRankedReinforcementAt(int nTarget,
											  int nAttributeRank) const
{
	const ObjectArray* oaRankedAttributeReinforcements;

	require(IsCompiled());
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	require(0 <= nAttributeRank and nAttributeRank < GetPredictorAttributeNumber());
	require(ivTargetValueReinforcementNeeded.GetAt(nTarget) == 1);
	require(ivTargetValueReinforcementComputed.GetAt(nTarget) == 1);
	oaRankedAttributeReinforcements =
	    cast(const ObjectArray*, oaTargetValueRankedAttributeReinforcements.GetAt(nTarget));
	return cast(const KIAttributeReinforcement*, oaRankedAttributeReinforcements->GetAt(nAttributeRank));
}

inline void KIAttributeReinforcement::SetAttributeIndex(int nValue)
{
	require(nValue >= 0);
	nAttributeIndex = nValue;
}

inline int KIAttributeReinforcement::GetAttributeIndex() const
{
	return nAttributeIndex;
}

inline void KIAttributeReinforcement::SetReinforcementPartIndex(int nValue)
{
	require(nValue >= 0);
	nReinforcementPartIndex = nValue;
}

inline int KIAttributeReinforcement::GetReinforcementPartIndex() const
{
	return nReinforcementPartIndex;
}

inline void KIAttributeReinforcement::SetReinforcementFinalScore(Continuous cValue)
{
	require(cValue >= 0);
	cReinforcementFinalScore = cValue;
}

inline Continuous KIAttributeReinforcement::GetReinforcementFinalScore() const
{
	return cReinforcementFinalScore;
}

inline void KIAttributeReinforcement::SetReinforcementClassChangeTag(int nValue)
{
	require(-1 <= nValue and nValue <= 1);
	nReinforcementClassChangeTag = nValue;
}

inline int KIAttributeReinforcement::GetReinforcementClassChangeTag() const
{
	return nReinforcementClassChangeTag;
}

inline const ALString& KIAttributeReinforcement::GetAttributeName() const
{
	require(svAttributeNames != NULL);
	require(0 <= nAttributeIndex and nAttributeIndex < svAttributeNames->GetSize());
	return svAttributeNames->GetAt(nAttributeIndex);
}
