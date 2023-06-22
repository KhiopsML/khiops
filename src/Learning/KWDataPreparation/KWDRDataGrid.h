// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour le deploiement des modeles en grilles de donnees

class KWDRDataGrid;
class KWDRFrequencies;
class KWDRCellId;
class KWDRCellIndex;
class KWDRCellIndexWithMissing;
class KWDRCellLabel;
class KWDRValueIndex;
class KWDRValueRank;
class KWDRInverseValueRank;
class KWDRDataGridStats;
class KWDRSourceConditionalInfo;

#include "KWDerivationRule.h"
#include "KWDRPreprocessing.h"
#include "KWDataGridStats.h"
#include "KWProbabilityTable.h"
#include "KWContinuous.h"
#include "KWSymbol.h"

// Enregistrement de ces regles
void KWDRRegisterDataGridRules();

///////////////////////////////////////////////////////////////
// Classe KWDRDataGrid
// Regle de derivation permettant de construire la specification
// complete d'une grille de donnees.
class KWDRDataGrid : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDataGrid();
	~KWDRDataGrid();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait soit en specifiant les
	// partition univariee des attributs et les effectifs des
	// cellules de la grille.
	// La regle renvoie une Structure(DataGrid), caracterisee par:
	//    . le parametrage des partitions univariees pour chaque
	//      dimension de la grille
	//      - Structure(IntervalBounds) pour les discretisations
	//      - Structure(ValueGroups) pour les groupements de valeur
	//      - Structure(ValueSet) pour les ensembles de valeurs Continuous
	//      - Structure(ValueSetC) pour les ensembles de valeurs Symbol
	//    . le parametrage des effectifs des cellules de la grille
	//      au moyen d'une Structure(Frequencies)
	// Optionnellement, la Structure(DataGrid) comporte un parametrage supplementaire
	// utile pour l'evalution des cout de grille, notament dans le cas du deploiement
	// de coclustering:
	//    . la granularite de la grille (entier stocke avec un Continuous)
	//    . les effectifs initiaux par attributs, stockes avec une Structure(Frequencies)
	//    . les effectifs granularises par attributs, stockes avec une Structure(Frequencies)
	// Tous les effectifs correspondant a la taille de la grille
	// doivent etre precises (meme pour les cellules vides.
	// Pour une grille de dimension D=I1*I2*...*IK, l'effectif
	// d'une cellule (i1, i2, ..., iK) est precise en index
	// i = i1 + I1*i2 + I1*I2*i3+ ... +I1*I2*...*I(K-1)*iK
	// Exemple en dimension 2, avec I1 = 3 (source) et I2 = 2 (cible), index des cellules:
	//        T1 T2
	//     S1  0  3
	//     S2  1  4
	//     S3  2  5
	//
	// Exemple: Grille (oriente regression) pour deux variables Continuous
	//    MyDataGrid = DataGrid(IntervalBounds(0.8, 1.45, 1.75),
	//                          IntervalBounds(2.45, 4.45, 4.95),
	//                          Frequencies(31, 0, 0, 0, 0, 20, 0, 0, 0, 5, 15, 1, 0, 1, 5, 23));
	//
	// Exemple: Grille (orientee classification) pour deux variables Continuous et une variable Symbol
	//    MyDataGrid = DataGrid(IntervalBounds(4.95, 5.75),
	//                          IntervalBounds(3.15),
	//                          ValueGroups(ValueGroup("Iris-setosa"), ValueGroup("Iris-versicolor"),
	//                          ValueGroup("Iris-virginica", "*")), Frequencies(9, 0, 0, 5, 17, 0, 0, 14, 20, 0, 0,
	//                          3, 1, 1, 21, 0, 0, 10));

	// Import d'un KWDataGridStats complet: attributs et cellules
	// La grille source doit etre valide
	// Si l'option bImportGranularityFeatures, on importe egalement la granularite de la grille, ainsi que
	// les effectifs initiaux et granularises par attributs de la grille
	void ImportDataGridStats(const KWDataGridStats* dataGridStats, boolean bImportGranularityFeatures);

	// Export d'un KWDataGridStats complet: attributs et cellules
	// La regle source doit etre valide et compilee
	// Remarque: la grille rendue est non supervisee (nombre d'attributs sources a zero),
	// a parametrer eventuellement par l'appelant
	void ExportDataGridStats(KWDataGridStats* dataGridStats) const;

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile

	// Nombre de dimensions de la grille
	int GetAttributeNumber() const;

	// Nombre total de cellules (vides ou non)
	int GetTotalCellNumber() const;

	// Type d'un attribut (KWType::Symbol ou KWType::Continuous)
	int GetAttributeTypeAt(int nAttributeIndex) const;

	// Nombre de parties d'un attribut
	int GetAttributePartNumberAt(int nAttributeIndex) const;

	// Index de la partie associee a une valeur pour un attribut
	// Le type de valeur doit etre compatible avec celui de l'attribut
	int GetContinuousAttributePartIndexAt(int nAttributeIndex, Continuous cValue) const;
	int GetSymbolAttributePartIndexAt(int nAttributeIndex, const Symbol& sValue) const;

	// Effectif d'une cellule
	int GetCellFrequencyAt(int nCellIndex) const;

	/////////////////////////////////////////////////////
	// Services specifiques disponibles, en mode non checke
	// (plus lents, mais utiles pour le check d'autre regles)

	// Nombre d'attributs de la grille, en mode non checke
	int GetUncheckedAttributeNumber() const;

	// Type d'un attribut, en mode non checke (renvoie KWType::Unknown si erreur)
	int GetUncheckedAttributeTypeAt(int nAttributeIndex) const;

	// Nombre de parties d'un attribut, en mode non checke
	int GetUncheckedAttributePartNumberAt(int nAttributeIndex) const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Verification des operandes
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Verification rapide qu'un operande est de type partition univariee
	// Pas de message d'erreur
	boolean SilentCheckUnivariatePartitionOperand(const KWDerivationRuleOperand* operand) const;

	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Optimisation apres la compilation
	virtual void Optimize(KWClass* kwcOwnerClass);

	// Test si optimisee
	boolean IsOptimized() const;

	// Acces au nombre d'attribut de la grille
	int nDataGridAttributeNumber;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRFrequencies
// Regle de derivation de type Structure, memorisant des effectif
class KWDRFrequencies : public KWDRStructureRule
{
public:
	// Constructeur
	KWDRFrequencies();
	~KWDRFrequencies();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant les
	// valeurs dans le vecteur prevu a cet effet

	// Nombre de d'effectifs (de cellules)
	// Le setter fait basculer en interface de structure,
	// et le getter est accessible en interface de structure et de base
	void SetFrequencyNumber(int nFrequency);
	int GetFrequencyNumber() const;

	// Parametrage des effectifs
	void SetFrequencyAt(int nIndex, int nFrequency);
	int GetFrequencyAt(int nIndex) const;

	// Calcul de l'effectif total
	int ComputeTotalFrequency() const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Nettoyage de l'interface de base une fois la regle compilee
	void CleanCompiledBaseInterface() override;

	// Verification de la valeur entiere positive des operandes
	boolean CheckOperandsDefinition() const override;

	//////////////////////////////////////////////////////
	// Redefinition des methodes de structure

	// Recopie de la partie structure de la regle
	void CopyStructureFrom(const KWDerivationRule* kwdrSource) override;

	// Transfert de la specification de base de la regle source
	// vers la specification de structure de la regle en cours
	void BuildStructureFromBase(const KWDerivationRule* kwdrSource) override;

	// Affichage, ecriture dans un fichier
	// Note: la valeur speciale (*) est laissee telle quelle,
	// et entouree de backquotes pour une valeur standard coincidant
	void WriteStructureUsedRule(ostream& ost) const override;

	// Methode de comparaison entre deux regles
	int FullCompare(const KWDerivationRule* rule) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Valeurs
	IntVector ivFrequencies;
};

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridRule
// Regle de derivation basee sur le partitionnement d'un objet
// au moyen d'une grille, calculant un identifiant de cellule
// Ancetre des regle basees sur les grilles, non instanciable
class KWDRDataGridRule : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDataGridRule();
	~KWDRDataGridRule();

	//////////////////////////////////////////////////////
	// Service a utiliser dans les sous-classes

	// Calcul de l'index de cellule
	void ComputeCellIndex(const KWObject* kwoObject) const;

	// Acces a l'index de cellule une fois calcule
	// Il s'agit d'un index "interne" compris entre 0 et N-1
	// Attention a la regle de derivation CellIndex, qui elle expose un index "utilisateur" entre 1 et N
	int GetCellIndex() const;

	// Acces a l'indication de valeur manquante de l'objet pour une des dimensions, une fois l'index calcule
	// Les valeurs manquantes sont Missing dans le cas numérique et "" dans le cas categoriel
	boolean IsMissingValue() const;

	// Calcul d'un libelle de cellule
	const ALString ComputeCellLabel(const KWObject* kwoObject) const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Verification des operandes d'une regle basee sur une grille
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Verification que le nombre d'arguments est egal a la dimension
	// (moins un de la grille), pour la prediction de la derniere dimension
	// Verification egalement du type du dernier argument
	//   Symbol pour un classifier, Continuous pour un regresseur
	boolean CheckPredictorCompletness(int nPredictorType, const KWClass* kwcOwnerClass) const;

	// Verification que la grille  est univariee et qu'il y a un argument,
	// ce qui correspond a la specification d'un partitionnement elementaire de l'attribut cible
	boolean CheckTargetAttributeCompletness(const KWClass* kwcOwnerClass) const;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Optimisation apres la compilation
	virtual void Optimize(KWClass* kwcOwnerClass);

	// Test si optimisee
	boolean IsOptimized() const;

	// Index de la partie source
	mutable int nCellIndex;

	// Indication de valeur manquante dans l'objet
	mutable boolean bIsMissingValue;

	// Acces a la regle de grille de donnees (referencee uniquement)
	KWDRDataGrid* dataGridRule;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRCellIndex
// Regle de derivation basee sur le partitionnement d'un objet
// au moyen d'une grille, renvoyant un index de cellule (entre 1 et N)
class KWDRCellIndex : public KWDRDataGridRule
{
public:
	// Constructeur
	KWDRCellIndex();
	~KWDRCellIndex();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Renvoie un index compris entre 1 et N (nombre de cellules)
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRCellIndexWithMissing
// Comme KWDRCellIndex, en renvoyant -1 dans le cas ou l'objet indexe contient une valeur manquante
class KWDRCellIndexWithMissing : public KWDRDataGridRule
{
public:
	// Constructeur
	KWDRCellIndexWithMissing();
	~KWDRCellIndexWithMissing();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Renvoie un index compirs entre 1 et N (nombre de cellules)
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRCellLabel
// Regle de derivation basee sur le partitionnement d'un objet
// au moyen d'une grille, renvoyant un libelle de cellule
class KWDRCellLabel : public KWDRDataGridRule
{
public:
	// Constructeur
	KWDRCellLabel();
	~KWDRCellLabel();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Renvoie un index compirs entre 1 et N (nombre de cellules)
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRCellId
// Regle de derivation basee sur le partitionnement d'un objet
// au moyen d'une grille, renvoyant un identifiant de cellule
class KWDRCellId : public KWDRDataGridRule
{
public:
	// Constructeur
	KWDRCellId();
	~KWDRCellId();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Optimisation apres la compilation
	void Optimize(KWClass* kwcOwnerClass) override;

	// Identifiant des cellules
	SymbolVector svCellIds;
};

///////////////////////////////////////////////////////////////
// Classe KWDRValueIndex
// Regle de derivation basee sur le partitionnement d'un objet
// au moyen d'une grille (un operande KWDRDataGrid),
// renvoyant un index de valeur (entre 1 et V)
// Valide uniquement en univarie, avec une partition en valeurs elementaires
//  En symbolique: rang de valeur dans la liste des valeurs de l'attribut
//  En continuous: rang de l'instance, selon l'ordre des valeurs de l'attribut,
//                 en cas d'egalite, renvoie le rang moyen correspondant a la valeur
class KWDRValueIndex : public KWDRDataGridRule
{
public:
	// Constructeur
	KWDRValueIndex();
	~KWDRValueIndex();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Optimisation apres la compilation
	void Optimize(KWClass* kwcOwnerClass) override;

	// Vecteur de transcodage de l'index de partie de la grille univarie en rang
	ContinuousVector cvValueIndexes;
};

///////////////////////////////////////////////////////////////
// Classe KWDRValueRank
// Regle de derivation basee sur le partitionnement d'un objet
// au moyen d'une grille (un operande KWDRDataGrid),
// renvoyant un rang normalise de valeur (compris entre 0 et 1)
// Valide uniquement en univarie continuous, avec une partition en valeurs elementaires
// ou une partition en intervalles (on renvoie alors le rang moyen dans l'intervalle)
class KWDRValueRank : public KWDRDataGridRule
{
public:
	// Constructeur
	KWDRValueRank();
	~KWDRValueRank();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Optimisation apres la compilation
	void Optimize(KWClass* kwcOwnerClass) override;

	// Vecteur de transcodage de l'index de partie de la grille univarie en rang
	ContinuousVector cvValueRanks;
};

///////////////////////////////////////////////////////////////
// Classe KWDRValueRankSelfDistance
// Sous-classe de KWDRValueRank pour le calcul des self-distances (mode expert uniquement)
class KWDRValueRankSelfDistance : public KWDRValueRank
{
public:
	// Constructeur
	KWDRValueRankSelfDistance();
	~KWDRValueRankSelfDistance();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Optimisation apres la compilation
	void Optimize(KWClass* kwcOwnerClass) override;

	// Vecteur de transcodage de l'index de partie de la grille univarie en self-distance
	ContinuousVector cvValueRankSelfDistances;
};

///////////////////////////////////////////////////////////////
// Classe KWDRInverseValueRank
// Regle ayant le meme parametrage que KWDRValueRank, mais
// renvoyant cette fois une valeur a partir d'un rang normalise
// Valide uniquement en univarie continuous, avec une partition en valeurs elementaires
class KWDRInverseValueRank : public KWDRValueRank
{
public:
	// Constructeur
	KWDRInverseValueRank();
	~KWDRInverseValueRank();

	// Creation
	KWDerivationRule* Create() const override;

	// Verification que la regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Optimisation apres la compilation
	void Optimize(KWClass* kwcOwnerClass) override;

	// Calcul de l'index de valeur correspondant a un rang normalise
	int ComputeNormalizedRankIndex(Continuous cRank) const;

	// Regle de partitionnement univariee associee a la grille
	KWDRContinuousValueSet* continuousValueSetRule;
};

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridStats
// Regle de derivation basee sur le partitionnement d'un objet
// au moyen d'une grille, renvoyant une structure fournissant
// des statistiques utiles pour la prediction
// Le nombre d'attributs en parametre de la regle determine
// la repartition entre attributs sources et cibles de la grilles,
// utile pour le calcul des probabilites conditionnelles:
// s'il y a autant d'attributs que de dimension dans la grille,
// tous les attributs sont cible. Sinon, le nombre d'attributs
// en parametres de la regle est le nombre de dimensions sources.
class KWDRDataGridStats : public KWDRDataGridRule
{
public:
	// Constructeur
	KWDRDataGridStats();
	~KWDRDataGridStats();

	// Creation
	KWDerivationRule* Create() const override;

	////////////////////////////////////////////////////////////////////
	// Application  de la regle a un objet, et services associes

	// Calcul de l'attribut derive
	// L'index de la partie source est alors mise a jour et
	// disponible de facon bufferisee
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	////////////////////////////////////////////////////////////////////
	// Services disponible apres compilation

	// Type de prediction, lie au type de la derniere dimension de la grille
	// (prediction supervisee uniquement si un seul attribut cible)
	//    KWTYpe::Continuous: regression
	//    KWTYpe::Symbol: classification
	//    KWType::Unknown: non supervise
	int GetPredictionType() const;

	// Nombre d'attributs source et cible
	int GetDataGridSourceAttributeNumber() const;
	int GetDataGridTargetAttributeNumber() const;

	// Nombre de cellules des grilles d'entree et de sortie
	// Dans le cas d'un seul attribut cible, le nombre de cellules cibles
	// est egal au nombre de parties cibles
	int GetDataGridSourceCellNumber() const;
	int GetDataGridTargetCellNumber() const;

	// Log des probabilites conditionnelles sources
	Continuous GetDataGridSourceConditionalLogProbAt(int nSource, int nTarget) const;

	// Effectif cumule par partie cible, dans le cas de la regression
	Continuous GetDataGridTargetCumulativeFrequencyAt(int nTarget) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation optimisee

	// Optimisation apres la compilation
	void Optimize(KWClass* kwcOwnerClass) override;

	// Type de prediction
	int nPredictionType;

	// Table des logs des probabilites conditionnelles des sources (P(i, j) = P(i|j) = Nij/N.j)
	KWProbabilityTable ptSourceConditionalLogProbs;

	// Vecteurs des effectifs cumules par partie cible (S(j) = N.0 + N.1 + ... + N.j)
	ContinuousVector cvTargetCumulativeFrequencies;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSourceConditionalInfo
// Calcul de l'information source conditionnelle (-log(prob(X|Y))
//  pour une partie cible donne en parametre, la partie source
//  etant donnee par la projection des donnees sur la grille
// Regle ayant en premier operande une structure KWDRDataGridStats
// et en second operande un index de partie cible (entre 1 et J) de la grille
// Dans le cas d'une grille ne contenant que des attributs cibles,
// l'information conditionnelle source vaut 0
class KWDRSourceConditionalInfo : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSourceConditionalInfo();
	~KWDRSourceConditionalInfo();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////
// Methode en inline

inline int KWDRDataGrid::GetAttributeNumber() const
{
	require(IsCompiled());
	return nDataGridAttributeNumber;
}

inline int KWDRDataGrid::GetTotalCellNumber() const
{
	require(IsCompiled());
	return cast(KWDRFrequencies*, GetOperandAt(nDataGridAttributeNumber)->GetDerivationRule())
	    ->GetFrequencyNumber();
}

inline int KWDRDataGrid::GetAttributeTypeAt(int nAttributeIndex) const
{
	require(IsCompiled());
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())->GetAttributeType();
}

inline int KWDRDataGrid::GetAttributePartNumberAt(int nAttributeIndex) const
{
	require(IsCompiled());
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())->GetPartNumber();
}

inline int KWDRDataGrid::GetContinuousAttributePartIndexAt(int nAttributeIndex, Continuous cValue) const
{
	require(IsCompiled());
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	require(GetAttributeTypeAt(nAttributeIndex) == KWType::Continuous);
	return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())
	    ->GetContinuousPartIndex(cValue);
}

inline int KWDRDataGrid::GetSymbolAttributePartIndexAt(int nAttributeIndex, const Symbol& sValue) const
{
	require(IsCompiled());
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	require(GetAttributeTypeAt(nAttributeIndex) == KWType::Symbol);
	return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())
	    ->GetSymbolPartIndex(sValue);
}

inline int KWDRDataGrid::GetCellFrequencyAt(int nCellIndex) const
{
	require(IsCompiled());
	require(0 <= nCellIndex and nCellIndex < GetTotalCellNumber());

	return cast(KWDRFrequencies*, GetOperandAt(GetAttributeNumber())->GetDerivationRule())
	    ->GetFrequencyAt(nCellIndex);
}

inline int KWDRDataGrid::GetUncheckedAttributeTypeAt(int nAttributeIndex) const
{
	require(0 <= nAttributeIndex and nAttributeIndex < GetUncheckedAttributeNumber());
	if (SilentCheckUnivariatePartitionOperand(GetOperandAt(nAttributeIndex)))
		return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())
		    ->GetAttributeType();
	else
		return KWType::Unknown;
}

inline int KWDRDataGrid::GetUncheckedAttributePartNumberAt(int nAttributeIndex) const
{
	require(0 <= nAttributeIndex and nAttributeIndex < GetUncheckedAttributeNumber());
	if (SilentCheckUnivariatePartitionOperand(GetOperandAt(nAttributeIndex)))
		return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())
		    ->GetPartNumber();
	else
		return 0;
}

inline int KWDRDataGridRule::GetCellIndex() const
{
	require(IsOptimized());
	assert(nCellIndex != -1);
	return nCellIndex;
}

inline boolean KWDRDataGridRule::IsMissingValue() const
{
	require(IsOptimized());
	assert(nCellIndex != -1);
	return bIsMissingValue;
}

inline int KWDRDataGridStats::GetPredictionType() const
{
	require(IsOptimized());
	return nPredictionType;
}

inline int KWDRDataGridStats::GetDataGridSourceAttributeNumber() const
{
	require(IsOptimized());
	return GetOperandNumber() - 1;
}

inline int KWDRDataGridStats::GetDataGridTargetAttributeNumber() const
{
	require(IsOptimized());
	return dataGridRule->GetAttributeNumber() - GetOperandNumber() + 1;
}

inline int KWDRDataGridStats::GetDataGridSourceCellNumber() const
{
	require(IsOptimized());
	return ptSourceConditionalLogProbs.GetSourceSize();
}

inline int KWDRDataGridStats::GetDataGridTargetCellNumber() const
{
	require(IsOptimized());
	return ptSourceConditionalLogProbs.GetTargetSize();
}

inline Continuous KWDRDataGridStats::GetDataGridTargetCumulativeFrequencyAt(int nTarget) const
{
	require(IsOptimized());
	require(GetPredictionType() == KWType::Continuous);
	require(0 <= nTarget and nTarget < GetDataGridTargetCellNumber());
	return cvTargetCumulativeFrequencies.GetAt(nTarget);
}

inline Continuous KWDRDataGridStats::GetDataGridSourceConditionalLogProbAt(int nSource, int nTarget) const
{
	require(IsOptimized());
	require(0 <= nSource and nSource < GetDataGridSourceCellNumber());
	require(0 <= nTarget and nTarget < GetDataGridTargetCellNumber());
	return ptSourceConditionalLogProbs.GetSourceConditionalLogProbAt(nSource, nTarget);
}
