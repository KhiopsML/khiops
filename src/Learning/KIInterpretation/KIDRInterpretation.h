// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KIDRClassifierService;
class KIDRClassifierInterpreter;
class KIDRInterpretationRule;
class KIDRContributionAt;
class KIDRContributionAttributeAt;
class KIDRContributionPartAt;
class KIDRContributionValueAt;
class KIAttributeContribution;

#include "KWDerivationRule.h"
#include "KWDRNBPredictor.h"
#include "KWDRDataGrid.h"
#include "KIShapleyTable.h"

// Enregistrement des regles liee a l'interpretation des modeles
void KIDRRegisterInterpretationRules();

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierService
// Classe ancetre des services exploitant un classifier de type Bayesien naif (NB ou SNB)
class KIDRClassifierService : public KWDerivationRule
{
public:
	// Constructeur
	KIDRClassifierService();
	~KIDRClassifierService();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////
	// Acces aux caracteristiques du classifieur apres compilation

	// Valeurs cible
	int GetTargetValueNumber() const;
	Symbol GetTargetValueAt(int nTarget) const;

	// Rang d'une valeur cible (-1 si non trouve)
	int GetTargetValueRank(const Symbol& sValue) const;

	// Noms des variables, sous forme de Symbol pour etre directement exploitable dans les regles de derivation
	int GetPredictorAttributeNumber() const;
	Symbol GetPredictorAttributeNameAt(int nAttribute) const;

	// Rang d'une variable d'apres son nom (-1 si non trouve)
	int GetPredictorAttributeRank(const Symbol& sName) const;

	// Poids des variables
	Continuous GetPredictorAttributeWeightAt(int nAttribute) const;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a un objet

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	////////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage des caracteristiques detaillees de l'interpreteur
	virtual void WriteDetails(ostream& ost) const;
	virtual void WriteAttributeDetails(ostream& ost, int nAttribute) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Nettoyage
	virtual void Clean();

	// Construction du libelle d'une cellule source
	// - cas univarie: CellIndex = PartIndex
	// - cas bivarie: CellIndex = PartIndex1 + Part2Index2 * PartNumber1
	const ALString BuildSourceCellLabel(const KWDRDataGrid* dataGridRule, int nSourceCellIndex) const;

	// Calcul du vecteur des index source de grille pour chaque variable du predicteur
	void ComputeDataGridSourcesIndexes() const;

	// Regle associee au classifieur
	const KWDRNBClassifier* classifierRule;

	// Noms des variables du predicteur
	SymbolVector svPredictorAttributeNames;

	// Dictionnaire des rangs des variables, en memorisant le rang+1
	LongintNumericKeyDictionary lnkdPredictorAttributeRanks;

	// Tableau des regles de type KWDRDataGrid par variable du predicteur
	ObjectArray oaPredictorAttributeDataGridRules;

	// Tableau des regles de type KWDRDataGridStats par variable du predicteur
	ObjectArray oaPredictorAttributeDataGridStatsRules;

	// Index de chaque partie source des grilles, que l'on soit en sparse ou non
	// Dans le cas de l'interpretation, on a besoin de tous ces index pour acceder aux
	// tables de Shapley, ce qui fait que ce vecteur d'index est dense
	mutable IntVector ivDataGridSourceIndexes;

	// Index par defaut de chaque partie source des grilles
	// Permet d'avoir la reference dans le cas sparse, et de ne calculer que les index
	// que pour les valeurs presentes
	IntVector ivDataGridSourceDefaultIndexes;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierInterpreter
// Service d'interpretation d'un classifieur
class KIDRClassifierInterpreter : public KIDRClassifierService
{
public:
	// Constructeur
	KIDRClassifierInterpreter();
	~KIDRClassifierInterpreter();

	// Creation
	KWDerivationRule* Create() const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////
	// Acces aux caracteristiques specifiques de l'interpreteur apres compilation

	// Tables des valeurs de Shapley par variable
	const KIShapleyTable* GetPredictorAttributeShapleyTableAt(int nAttribute) const;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a un objet, et services associes,
	// pour des parametres de rang valides

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Valeur de contribution pour une valeur cible et un index de variable
	Continuous GetContributionAt(int nTargetValueRank, int nPredictorAttributeRank) const;

	// Nom de variable de contribution pour une valeur cible et un rang de variable
	Symbol GetRankedContributionAttributeAt(int nTargetValueRank, int nContributionRank) const;

	// Nom de partie de variable de contribution pour une valeur cible et un rang de variable
	Symbol GetRankedContributionPartAt(int nTargetValueRank, int nContributionRank) const;

	// Valeur de contribution pour une valeur cible et un rang de variable
	Continuous GetRankedContributionValueAt(int nTargetValueRank, int nContributionRank) const;

	////////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage des caracteristiques detaillees de l'interpreteur
	void WriteAttributeDetails(ostream& ost, int nAttribute) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Nettoyage
	void Clean() override;

	// Creation des structures de gestion des contributions pour les acces par rang
	void CreateRankedContributionStructures(int nTargetValueNumber, int nAttributeNumber,
						const SymbolVector* svAttributeNames);

	// Calcul de toutes les contributions triees pour les acces aux contributions par rang
	void ComputeRankedContributions() const;

	// Contribution par valeur cible et par rang
	const KIAttributeContribution* GetRankedContributionAt(int nTarget, int nAttributeRank) const;

	// Tableau des tables de Shapley par variable du predicteur
	ObjectArray oaPredictorAttributeShapleyTables;

	// Tableau par valeur cible de tableaux de KIAttributeContribution, tries par contribution decroissante
	ObjectArray oaTargetValueRankedAttributeContributions;

	// Indicateur de calcul des contributions par rang, pour bufferisation des calculs
	mutable boolean bIsRankedContributionComputed;
};

////////////////////////////////////////////////////////////
// Classe KIDRInterpretationRule
// Classe abstraite ancetre des regles exploitant un interpreteur
// pour indexer les operandes des regles
class KIDRInterpretationRule : public KWDerivationRule
{
public:
	// Constructeur
	KIDRInterpretationRule();
	~KIDRInterpretationRule();

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	////////////////////////////////////////////////////////////////////////
	// Rang des operandes obtenu de facon generiques pour toutes les regles
	// Lors de la compilation, on memorise un rang en dur si l'operande est
	// constant et valide, -1 sinon
	// Lors de l'acces au rang, on exploite ce rang si possible, sinon on le recalcule

	// Memorisation des rangs issus de la compilation
	int nConstantTargetValueRank;
	int nConstantPredictorAttributeRank;
	int nConstantContributionRank;
};

////////////////////////////////////////////////////////////
// Classe KIDRContributionAt
// Donne la valeur de la contribution pour une valeur cible
// et un nom de variable a partir d'un interpreteur
class KIDRContributionAt : public KIDRInterpretationRule
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
class KIDRContributionAttributeAt : public KIDRInterpretationRule
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
class KIDRContributionPartAt : public KIDRInterpretationRule
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
class KIDRContributionValueAt : public KIDRInterpretationRule
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
// Memorisation d'un index d'attribut et de sa contribution,
// permettant ensuite un tri par contribution decroissante
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
	void SetAttributeNames(const SymbolVector* svNames);
	const SymbolVector* GetAttributeNames() const;

	// Nom de l'attribut correspondant a son index
	Symbol GetAttributeName() const;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Variables de la classe
	int nAttributeIndex;
	Continuous cContribution;
	const SymbolVector* svAttributeNames;
};

// Methode de comparaison par contribution decroissante
int KIAttributeContributionCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////
// Methode en inline

inline int KIDRClassifierService::GetTargetValueNumber() const
{
	require(IsCompiled());
	return classifierRule->GetTargetValueNumber();
}

inline Symbol KIDRClassifierService::GetTargetValueAt(int nTarget) const
{
	require(IsCompiled());
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	return classifierRule->GetTargetValueAt(nTarget);
}

inline int KIDRClassifierService::GetTargetValueRank(const Symbol& sValue) const
{
	require(IsCompiled());
	return classifierRule->GetTargetValueRank(sValue);
}

inline int KIDRClassifierService::GetPredictorAttributeNumber() const
{
	require(IsCompiled());
	return classifierRule->GetDataGridStatsNumber();
}

inline Symbol KIDRClassifierService::GetPredictorAttributeNameAt(int nAttribute) const
{
	require(IsCompiled());
	require(0 <= nAttribute and nAttribute < GetPredictorAttributeNumber());
	return svPredictorAttributeNames.GetAt(nAttribute);
}

inline int KIDRClassifierService::GetPredictorAttributeRank(const Symbol& sName) const
{
	int nRank;
	require(IsCompiled());
	nRank = (int)lnkdPredictorAttributeRanks.Lookup(sName.GetNumericKey()) - 1;
	ensure(nRank == -1 or svPredictorAttributeNames.GetAt(nRank) == sName);
	return nRank;
}

inline Continuous KIDRClassifierService::GetPredictorAttributeWeightAt(int nAttribute) const
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

inline Symbol KIAttributeContribution::GetAttributeName() const
{
	require(svAttributeNames != NULL);
	require(0 <= nAttributeIndex and nAttributeIndex < svAttributeNames->GetSize());
	return svAttributeNames->GetAt(nAttributeIndex);
}
