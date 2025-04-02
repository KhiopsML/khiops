// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KIDRClassifierInterpreter;
class KIDRContributionAt;
class KIDRContributionAttributeAt;
class KIDRContributionPartAt;
class KIDRContributionValueAt;
class KIAttributeContribution;

#include "KWDerivationRule.h"
#include "KWDRNBPredictor.h"
#include "KWDRDataGrid.h"
#include "KIShapleyTable.h"

// Enregistrement des regles liee a l'interpretation ou au renforcement des modeles
void KIDRRegisterInterpretationRules();

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierInterpreter
// Service d'interpretation d'un classifier
class KIDRClassifierInterpreter : public KWDerivationRule
{
public:
	// Constructeur
	KIDRClassifierInterpreter();
	~KIDRClassifierInterpreter();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////
	// Acces aux caracteristique de l'interpreteur apres compilation

	// Valeurs cible
	int GetTargetValueNumber() const;
	Symbol GetTargetValueAt(int nTarget) const;

	// Rang d'une valeur cible (-1 si non trouve)
	int GetTargetValueRank(Symbol sValue) const;

	// Noms des variables
	int GetPredictorAttributeNumber() const;
	const ALString& GetPredictorAttributeNameAt(int nAttribute) const;

	// Rang d'une variable d'apres son nom (-1 si non trouve)
	int GetPredictorAttributeRank(const ALString& sName) const;

	// Poids des variables
	Continuous GetPredictorAttributeWeightAt(int nAttribute) const;

	// Tables des valeur de Shapley par variable
	const KIShapleyTable* GetPredictorAttributeShapleyTableAt(int nAttribute) const;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a un objet, et services associes

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Valeur de contribution pour un valeur cible et un index de variable
	Continuous GetContributionAt(Symbol sTargetValue, Symbol sAttributeName) const;

	// Nom de variable de contribution pour une valeur cible et un rang de variable
	Symbol GetRankedContributionAttributeAt(Symbol sTargetValue, int nAttributeRank) const;

	// Nom de partie de variable de contribution pour une valeur cible et un rang de variable
	Symbol GetRankedContributionPartAt(Symbol sTargetValue, int nAttributeRank) const;

	// Valeur de contribution pour une valeur cible et un rang de variable
	Continuous GetRankedContributionValueAt(Symbol sTargetValue, int nAttributeRank) const;

	////////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage des caracteristique detaillees de l'interpreteur
	void WriteDetails(ostream& ost) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Nettoyage
	virtual void Clean();

	// Creation des structures des gestion des contributions pour les acces par rang
	void CreateRankedContributionStructures(int nTargetValueNumber, int nAttributeNumber,
						const StringVector* svAttributeNames);

	// Calcul du vecteur de index source de grille
	void ComputeDataGridSourcesIndexes() const;

	// Calcul de toute els contribution triee pour les acces aux cointributions par rang
	void ComputeRankedContributions() const;

	// Contribution par valeur cible et par rang
	const KIAttributeContribution* GetRankedContributionAt(int nTarget, int nAttributeRank) const;

	// Regle associee au classifieur
	const KWDRNBClassifier* classifierRule;

	// Dictionnaire des rangs des valeurs cibles, en memorisant le rang+1
	LongintNumericKeyDictionary lnkdTargetValueRanks;

	// Noms des variables du predicteur
	StringVector svPredictorAttributeNames;

	// Dictionnaire des rangs des variables, en memorisant le rang+1
	LongintDictionary ldPredictorAttributeRanks;

	// Tableau des regles de type data grid par variable du predicteur
	ObjectArray oaPredictorAttributeDataGridRules;

	// Tableau des tables de Shapley par variable du predicteur
	ObjectArray oaPredictorAttributeShapleyTables;

	// Index de chaque partie source des grilles, que l'on soit en sparse ou non
	// Dans le cas de l'interpretation, on a besoin de tous ces index pour acceder aux
	// tables de Shapley, ce qui fait que ce vecteur d'index est dense
	mutable IntVector ivDataGridSourceIndexes;

	// Index par defaut de chaque partie source des grilles
	// Permet d'avoir la reference dans le cas sparse, et de ne calculer que les index
	// que pour les valeurs presentes
	mutable IntVector ivDataGridSourceDefaultIndexes;

	// Tableau par valeur cible de tableau de KIAttributeContribution, tries par contribution decroissante
	ObjectArray oaTargetValueRankedAttributeContributions;

	// Indicateur de calcul des contributions par rang, pour bufferisation des calcul
	mutable boolean bIsRankedContributionComputed;
};

////////////////////////////////////////////////////////////
// Classe KIDRContributionAt
// Donne la valeur de la contribution pour une valeur cible
// et un nom de variable a partir d'un interpreteur
class KIDRContributionAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRContributionAt();
	~KIDRContributionAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////
// Classe KIDRContributionAttributeAt
// Donne le nom de la variable de contribution pour une valeur cible
// et un rang de variable a partir d'un interpreteur
class KIDRContributionAttributeAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRContributionAttributeAt();
	~KIDRContributionAttributeAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////
// Classe KIDRContributionPartAt
// Donne la partie de la variable de contribution pour une valeur cible
// et un rang de variable a partir d'un interpreteur
class KIDRContributionPartAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRContributionPartAt();
	~KIDRContributionPartAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////
// Classe KIDRContributionValueAt
// Donne la valeur de contribution pour une valeur cible
// et un rang de variable a partir d'un interpreteur
class KIDRContributionValueAt : public KWDerivationRule
{
public:
	// Constructeur
	KIDRContributionValueAt();
	~KIDRContributionValueAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////
// Classe KIAttributeContribution
// Memorisation d'un index d'attribut et de sa contribution, permettant
// ensuite un tri par contribution decroissante
class KIAttributeContribution : public Object
{
public:
	// Constructeur
	KIAttributeContribution();
	~KIAttributeContribution();

	// Index de l'attribut
	void SetAttributeIndex(int nValue);
	int GetAttributeIndex() const;

	// Contribution de l'attribut
	void SetContribution(Continuous cValue);
	Continuous GetContribution() const;

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
	Continuous cContribution;
	const StringVector* svAttributeNames;
};

// Methode de compraison
int KIAttributeContributionCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////
// Methode en inline

inline int KIDRClassifierInterpreter::GetTargetValueNumber() const
{
	require(IsCompiled());
	return classifierRule->GetTargetValueNumber();
}

inline Symbol KIDRClassifierInterpreter::GetTargetValueAt(int nTarget) const
{
	require(IsCompiled());
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	return classifierRule->GetTargetValueAt(nTarget);
}

inline int KIDRClassifierInterpreter::GetTargetValueRank(Symbol sValue) const
{
	int nRank;
	require(IsCompiled());
	nRank = (int)lnkdTargetValueRanks.Lookup(sValue.GetNumericKey()) - 1;
	ensure(nRank == -1 or GetTargetValueAt(nRank) == sValue);
	return nRank;
}

inline int KIDRClassifierInterpreter::GetPredictorAttributeNumber() const
{
	require(IsCompiled());
	return classifierRule->GetDataGridStatsNumber();
}

inline const ALString& KIDRClassifierInterpreter::GetPredictorAttributeNameAt(int nAttribute) const
{
	require(IsCompiled());
	require(0 <= nAttribute and nAttribute < GetPredictorAttributeNumber());
	return svPredictorAttributeNames.GetAt(nAttribute);
}

inline int KIDRClassifierInterpreter::GetPredictorAttributeRank(const ALString& sName) const
{
	int nRank;
	require(IsCompiled());
	nRank = (int)ldPredictorAttributeRanks.Lookup(sName) - 1;
	ensure(nRank == -1 or svPredictorAttributeNames.GetAt(nRank) == sName);
	return nRank;
}

inline Continuous KIDRClassifierInterpreter::GetPredictorAttributeWeightAt(int nAttribute) const
{
	require(IsCompiled());
	require(0 <= nAttribute and nAttribute < GetPredictorAttributeNumber());
	return classifierRule->GetDataGridWeightAt(nAttribute);
}

inline const KIShapleyTable* KIDRClassifierInterpreter::GetPredictorAttributeShapleyTableAt(int nAttribute) const
{
	require(IsCompiled());
	require(0 <= nAttribute and nAttribute < GetPredictorAttributeNumber());
	return cast(const KIShapleyTable*, oaPredictorAttributeShapleyTables.GetAt(nAttribute));
}

inline const KIAttributeContribution* KIDRClassifierInterpreter::GetRankedContributionAt(int nTarget,
											 int nAttributeRank) const
{
	const ObjectArray* oaRankedAttributeContributions;

	require(IsCompiled());
	require(bIsRankedContributionComputed);
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	require(0 <= nAttributeRank and nAttributeRank < GetPredictorAttributeNumber());
	oaRankedAttributeContributions =
	    cast(const ObjectArray*, oaTargetValueRankedAttributeContributions.GetAt(nTarget));
	return cast(const KIAttributeContribution*, oaRankedAttributeContributions->GetAt(nAttributeRank));
}

inline void KIAttributeContribution::SetAttributeIndex(int nValue)
{
	require(nValue >= 0);
	nAttributeIndex = nValue;
}

inline int KIAttributeContribution::GetAttributeIndex() const
{
	return nAttributeIndex;
}

inline void KIAttributeContribution::SetContribution(Continuous cValue)
{
	cContribution = cValue;
}

inline Continuous KIAttributeContribution::GetContribution() const
{
	return cContribution;
}

inline const ALString& KIAttributeContribution::GetAttributeName() const
{
	require(svAttributeNames != NULL);
	require(0 <= nAttributeIndex and nAttributeIndex < svAttributeNames->GetSize());
	return svAttributeNames->GetAt(nAttributeIndex);
}
