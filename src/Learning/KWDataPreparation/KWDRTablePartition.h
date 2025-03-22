// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDRPartition;
class KWDRTablePartition;

// Classe ancetre des regles ayant un premier operande de type ObjectArray et
// d'eventuels operandes additionnels sur des attributs secondaires, pour un calcul de stats
class KWDRTablePartitionStats;
class KWDRTablePartitionStatsContinuous;
class KWDRTablePartitionStatsSymbol;

// Calcul de stats pour des regle ayant un premier operande TablePartition
class KWDRTablePartitionCount;
class KWDRTablePartitionCountDistinct;
class KWDRTablePartitionEntropy;
class KWDRTablePartitionMode;
class KWDRTablePartitionModeAt;
class KWDRTablePartitionMean;
class KWDRTablePartitionStandardDeviation;
class KWDRTablePartitionMedian;
class KWDRTablePartitionMin;
class KWDRTablePartitionMax;
class KWDRTablePartitionSum;
class KWDRTablePartitionCountSum;
class KWDRTablePartitionTrend;
class KWDRTablePartitionConcat;

#include "KWDerivationRule.h"
#include "KWDRPreprocessing.h"
#include "KWValueBlock.h"
#include "KWValueSparseVector.h"
#include "KWDRMultiTable.h"

// Enregistrement de ces regles
void KWDRRegisterTablePartitionRules();

///////////////////////////////////////////////////////////////
// Classe KWDRPartition
// Regle de derivation permettant de construire la specification
// d'une partition multivarie a base de discretisations et de
// groupements de valeurs
class KWDRPartition : public KWDerivationRule
{
public:
	// Constructeur
	KWDRPartition();
	~KWDRPartition();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait soit en specifiant les
	// partition univariee des attributs
	// La regle renvoie une Structure(Partition), caracterisee par:
	//    . le parametrage des partitions univariees pour chaque
	//      dimension de la grille
	//      - Structure(IntervalBounds) pour les discretisations
	//      - Structure(ValueGroups) pour les groupements de valeur
	//      - Structure(ValueSet) pour les ensembles de valeurs Continuous
	//      - Structure(ValueSetC) pour les ensembles de valeurs Symbol
	//
	// Exemple:
	//    MyPartition = Partition(IntervalBounds(0.8, 1.45, 1.75),
	//                            IntervalBounds(2.45, 4.45, 4.95));
	//
	// Exemple:
	//    MyPartition = Partition(IntervalBounds(4.95, 5.75),
	//                          IntervalBounds(3.15),
	//                          ValueGroups(ValueGroup("Iris-setosa"),
	//                                      ValueGroup("Iris-versicolor"),
	//                                      ValueGroup("Iris-virginica", "*")));

	// Ajout d'un operante de type partition univariee au moyen d'une regle
	// La regle sert a parametrer l'operande, et lui appartient ensuite
	void AddUnivariatePartitionOperand(KWDRUnivariatePartition* univariatePartitionRule);

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile

	// Nombre de dimensions de la partition
	int GetAttributeNumber() const;

	// Type d'un attribut (KWType::Symbol ou KWType::Continuous)
	int GetAttributeTypeAt(int nAttributeIndex) const;

	// Nombre de parties d'un attribut
	int GetAttributePartNumberAt(int nAttributeIndex) const;

	// Nombre total de partie elementaires de la partition
	int GetTotalPartNumber() const;

	// Cle entiere d'une partie (1, 2,...), base sur l'index de cellule dans la grille de partition
	int GetPartNKeyAt(int nPartIndex) const;

	// Index de la partie associee a une valeur pour un attribut
	// Le type de valeur doit etre compatible avec celui de l'attribut
	int GetContinuousAttributePartIndexAt(int nAttributeIndex, Continuous cValue) const;
	int GetSymbolAttributePartIndexAt(int nAttributeIndex, const Symbol& sValue) const;

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles avant compilation
	// (lent, mais utile pour le check d'autre regles)

	// Type d'un attribut, en mode non checke
	int GetUncheckedAttributeTypeAt(int nAttributeIndex) const;

	// Nombre de parties d'un attribut, en mode non checke
	int GetUncheckedAttributePartNumberAt(int nAttributeIndex) const;

	// Calcul du nombre total de parties
	int ComputeUncheckedTotalPartNumber() const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Redefinition de methodes virtuelles
	void Compile(KWClass* kwcOwnerClass) override;
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Verification rapide qu'un operande est de type partition univariee
	// Pas de message d'erreur
	boolean SilentCheckUnivariatePartitionOperand(const KWDerivationRuleOperand* operand) const;

	// Nombre total de partie
	int nTotalPartNumber;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartition
// Partition des enregistrements d'uns table secondaire en un
// ensemble de parties (stockee de facon sparse)
// Operandes:
//   . Table(...)
//   . Structure(Partition) (au niveau du scope initial)
//   . attributs Continuous ou Symbol de la table, selon la nature de la partition
// Le resultat est stocke dans la structure, et peut etre exploite
// pour produire des indicateurs statistiques par partie, ayant pour identifiant
// les index de cellule de la grille de partition (commence a 1)
class KWDRTablePartition : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTablePartition();
	~KWDRTablePartition();

	// Reimplementation de la methode qui indique le type de cle du bloc en retour
	int GetVarKeyType() const override;

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// La partition de la table en entree est memorisee
	KWObjectArrayValueBlock*
	ComputeObjectArrayValueBlockResult(const KWObject* kwoObject,
					   const KWIndexedKeyBlock* indexedKeyBlock) const override;

	// Cle associe a un index de partie, parmi les index presents dans la partition
	int GetPartNKeyAt(int nPartIndex) const;

	// Redefinition de methodes virtuelles
	void Compile(KWClass* kwcOwnerClass) override;
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;
	boolean IsSecondaryScopeOperand(int nOperandIndex) const override;
	boolean CheckBlockAttributes(const KWClass* kwcOwnerClass,
				     const KWAttributeBlock* attributeBlock) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation de la methode ancetre de completion des informations de specification de la regle
	void InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
				      NumericKeyDictionary* nkdCompletedAttributes) override;

	// Acces a la regle de grille de donnees (referencee uniquement)
	const KWDRPartition* partitionRule;

	// Tableau de travail des parties utilises pour le calcul des partitions
	// Pour chaque index sparse potentiel de partie, ce tableau contient:
	//  . NULL si c'est la partie n'a jamais ete rencontree (etat initial)
	//  . un partie (ObjectArray) si la partie est a conserver
	// Le tableau est initialise avec des NULL partout
	// Les parties utilisees sont memorisees lors du calcul de la partition
	// A la fin, on exploite ces parties pour fabriquer le vecteur sparse de parties en sortie,
	// et on remet a NULL ce qui a ete utilise dans le tableau de travail
	mutable ObjectArray oaSparseParts;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionStats
// Classe ancetre des regles ayant un premier operande de type partition de table
// Operandes:
//   . Structure(TablePartition) (au niveau du scope initial)
// Operandes additionnels et type retour a specifier dans les classes filles
// En sortie, les stats par partie stockees sous forme d'un bloc sparse,
// avec pour identifiant les index de partie de la partition
// Exemple de sous classe: TablePartitionMean(TablePartition(Usages) per Service, Duration)
class KWDRTablePartitionStats : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTablePartitionStats();
	~KWDRTablePartitionStats();

	// Type de cle utilisee pour les variables du bloc
	int GetVarKeyType() const override;

	// Acces en consultation a la regle de type TableStats, a l'origine de l'extention en TablePartitionStats
	// Exemple: TableCount, TableMean...
	const KWDRTableStats* GetTableStatsRule() const;

	// Redefinition de methodes virtuelles
	void Compile(KWClass* kwcOwnerClass) override;

	// Redefinition de methodes virtuelle pour parametrer le premier operande de scope multiple
	boolean CheckFirstMultiScopeOperand() const override;
	KWClass* LookupSecondaryScopeClass(const KWClass* kwcOwnerClass) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Initialisation a appeler dans le constructeur, pour specifier les parametres
	// de la regle de stats de partition a partir des parametres de stats d'une table
	// Initialisation du nom, du libelle, et des operandes de calcul des stats
	// (le premier operande etant une partition de table au lieu d'un ObjectArray)
	// Memoire: la regle en parametre est emmorise par l'appele et lui appartient
	void InitializeFromTableStatsRule(KWDRTableStats* rule);

	// Regle permettant d'effectuer le calcul des stats pour chaque partie de la partition
	// Cette regle emprunte les operandes de la regles de stats par partition pour effectuer
	// les calculs de stats par partie
	// Attention, ses operandes ne lui appartiennent pas
	// Elles sont "empruntees" temporairement au moment de la compilation pour pouvoir
	// parametrer le calcul des stats par partie
	KWDRTableStats* tableStatsRule;

	// Acces a au bloc d'index des cles de la partition de table,
	// qui est necessairement associe au bloc d'attribut de l'operande associe
	const KWIndexedNKeyBlock* tablePartitionIndexedKeyBlock;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionStatsContinuous
// Classe ancetre des regles ayant un premier operande de type ObjectArray
// ete d'eventuels operandes additionnels pour un calcul de stats
// Stats de type block de valeurs Continuous en sortie
class KWDRTablePartitionStatsContinuous : public KWDRTablePartitionStats
{
public:
	// Constructeur
	KWDRTablePartitionStatsContinuous();
	~KWDRTablePartitionStatsContinuous();

	// Calcul de l'attribut derive
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;
	Continuous GetValueBlockContinuousDefaultValue() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionStatsSymbol
// Classe ancetre des regles ayant un premier operande de type ObjectArray
// ete d'eventuels operandes additionnels pour un calcul de stats
// Stats de type block de valeurs Symbol en sortie
class KWDRTablePartitionStatsSymbol : public KWDRTablePartitionStats
{
public:
	// Constructeur
	KWDRTablePartitionStatsSymbol();
	~KWDRTablePartitionStatsSymbol();

	// Calcul de l'attribut derive
	KWSymbolValueBlock* ComputeSymbolValueBlockResult(const KWObject* kwoObject,
							  const KWIndexedKeyBlock* indexedKeyBlock) const override;
	Symbol& GetValueBlockSymbolDefaultValue() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionCount
class KWDRTablePartitionCount : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionCount();
	~KWDRTablePartitionCount();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionCountDistinct
class KWDRTablePartitionCountDistinct : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionCountDistinct();
	~KWDRTablePartitionCountDistinct();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionEntropy
class KWDRTablePartitionEntropy : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionEntropy();
	~KWDRTablePartitionEntropy();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMode
class KWDRTablePartitionMode : public KWDRTablePartitionStatsSymbol
{
public:
	// Constructeur
	KWDRTablePartitionMode();
	~KWDRTablePartitionMode();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMode
class KWDRTablePartitionModeAt : public KWDRTablePartitionStatsSymbol
{
public:
	// Constructeur
	KWDRTablePartitionModeAt();
	~KWDRTablePartitionModeAt();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMean
class KWDRTablePartitionMean : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionMean();
	~KWDRTablePartitionMean();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionStandardDeviation
class KWDRTablePartitionStandardDeviation : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionStandardDeviation();
	~KWDRTablePartitionStandardDeviation();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMedian
class KWDRTablePartitionMedian : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionMedian();
	~KWDRTablePartitionMedian();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMin
class KWDRTablePartitionMin : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionMin();
	~KWDRTablePartitionMin();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMax
class KWDRTablePartitionMax : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionMax();
	~KWDRTablePartitionMax();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionSum
class KWDRTablePartitionSum : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionSum();
	~KWDRTablePartitionSum();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionCountSum
// Comme KWDRTablePartitionSum, mais renvoie 0 par defaut au lieu de Missing
// Usage pour calcul des cumuls de comptes
class KWDRTablePartitionCountSum : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionCountSum();
	~KWDRTablePartitionCountSum();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionTrend
class KWDRTablePartitionTrend : public KWDRTablePartitionStatsContinuous
{
public:
	// Constructeur
	KWDRTablePartitionTrend();
	~KWDRTablePartitionTrend();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionConcat
class KWDRTablePartitionConcat : public KWDRTablePartitionStatsSymbol
{
public:
	// Constructeur
	KWDRTablePartitionConcat();
	~KWDRTablePartitionConcat();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////
// Methode en inline

// Class KWDRPartition

inline int KWDRPartition::GetAttributeNumber() const
{
	require(IsCompiled());
	return GetOperandNumber();
}

inline int KWDRPartition::GetAttributeTypeAt(int nAttributeIndex) const
{
	require(IsCompiled());
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())->GetAttributeType();
}

inline int KWDRPartition::GetAttributePartNumberAt(int nAttributeIndex) const
{
	require(IsCompiled());
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())->GetPartNumber();
}

inline int KWDRPartition::GetTotalPartNumber() const
{
	require(IsCompiled());
	return nTotalPartNumber;
}

inline int KWDRPartition::GetPartNKeyAt(int nPartIndex) const
{
	require(IsCompiled());
	require(0 <= nPartIndex and nPartIndex < nTotalPartNumber);
	return nPartIndex + 1;
}

inline int KWDRPartition::GetContinuousAttributePartIndexAt(int nAttributeIndex, Continuous cValue) const
{
	require(IsCompiled());
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	require(GetAttributeTypeAt(nAttributeIndex) == KWType::Continuous);
	return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())
	    ->GetContinuousPartIndex(cValue);
}

inline int KWDRPartition::GetSymbolAttributePartIndexAt(int nAttributeIndex, const Symbol& sValue) const
{
	require(IsCompiled());
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	require(GetAttributeTypeAt(nAttributeIndex) == KWType::Symbol);
	return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())
	    ->GetSymbolPartIndex(sValue);
}

inline int KWDRPartition::GetUncheckedAttributeTypeAt(int nAttributeIndex) const
{
	require(0 <= nAttributeIndex and nAttributeIndex < GetOperandNumber());
	if (SilentCheckUnivariatePartitionOperand(GetOperandAt(nAttributeIndex)))
		return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())
		    ->GetAttributeType();
	else
		return KWType::Unknown;
}

inline int KWDRPartition::GetUncheckedAttributePartNumberAt(int nAttributeIndex) const
{
	require(0 <= nAttributeIndex and nAttributeIndex < GetOperandNumber());
	if (SilentCheckUnivariatePartitionOperand(GetOperandAt(nAttributeIndex)))
		return cast(KWDRUnivariatePartition*, GetOperandAt(nAttributeIndex)->GetDerivationRule())
		    ->GetPartNumber();
	else
		return 0;
}

// Class KWDRTablePartition

inline int KWDRTablePartition::GetPartNKeyAt(int nPartIndex) const
{
	require(IsCompiled());
	return nPartIndex + 1;
}

// Class KWDRTablePartitionStats

inline const KWDRTableStats* KWDRTablePartitionStats::GetTableStatsRule() const
{
	return tableStatsRule;
}
