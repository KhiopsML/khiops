// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

// Classe ancetre des regles ayant un premier operande de type ObjectArray et
// d'eventuels operandes additionnels sur des attributs secondaires, pour un calcul de stats
class KWDRTableBlockStats;
class KWDRTableBlockStatsContinuous;
class KWDRTableBlockStatsSymbol;

// Calcul de stats pour des regle ayant un premier operande TableBlock
class KWDRTableBlockCountDistinct;
class KWDRTableBlockEntropy;
class KWDRTableBlockMode;
class KWDRTableBlockMean;
class KWDRTableBlockStandardDeviation;
class KWDRTableBlockMedian;
class KWDRTableBlockMin;
class KWDRTableBlockMax;
class KWDRTableBlockSum;
class KWDRTableBlockCountSum;
class KWDRTableBlockConcat;

#include "KWDerivationRule.h"
#include "KWDRPreprocessing.h"
#include "KWValueBlock.h"
#include "KWValueSparseVector.h"
#include "KWDRMultiTable.h"
#include "KWDRTablePartition.h"

// Enregistrement de ces regles
void KWDRRegisterTableBlockRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableBlockStats
// Classe ancetre des regles ayant un premier operande de type table et
// un deuxieme operande de type bloc sparse numerique ou categoriel
// Operandes:
//   . Table
//   . AttributeBlock (au niveau du scope de la table)
// Operandes additionnels et type retour a specifier dans les classes filles
// En sortie, les stats par attribut du sous-bloc sparse sont stockees sous forme
// d'un bloc sparse, avec pour identifiant les index des attributs du sous-bloc sparse
// Exemple de sous classe: TableBlockMean(Services, Max(Usages) per Duration)
class KWDRTableBlockStats : public KWDRValueBlockRule
{
public:
	// Constructeur
	KWDRTableBlockStats();
	~KWDRTableBlockStats();

	// Type de cle utilisee pour les variables du bloc
	int GetVarKeyType() const override;

	// Acces en consultation a la regle de type TableStats, a l'origine de l'extention en TableBlockStats
	// Exemple: TableMean, TableMode (toujours avec un operand table et un operande de type simple)
	const KWDRTableStats* GetTableStatsRule() const;

	// Redefinition de methodes virtuelles
	void Compile(KWClass* kwcOwnerClass) override;

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

	// Methode generique de collecte de l'ensemble des valeurs utiles des blocs sparses
	// d'une sous-table, pour permettre une implementation generique des calcul de stats
	// par bloc, dans les sous-classes
	// Calcul d'un tableau de vecteurs de valeurs (Continuous ou Symbol) selon le type
	// de bloc sparse en deuxieme operande de la regle
	// Chaque poste du tableau correspond a un index sparse du bloc de variables en
	// sortie, et contient autant de valeurs qu'il y a en a dans les blocs
	// des enregistrements de la table en premier operande
	// Le tableau est non sparse, mais il ne contient que les valeurs presentes dans
	// la sous-table, avec soit un vecteur NULL si aucune valeur n'est presente, soit
	// le vecteur des valeurs presentes.
	// On renvoie en sortie le nombre d'enregistrements secondaires analyses
	int ComputeContinuousVectors(const KWObject* kwoObject, const KWIndexedKeyBlock* indexedKeyBlock,
				     ObjectArray* oaOutputVectors) const;
	int ComputeSymbolVectors(const KWObject* kwoObject, const KWIndexedKeyBlock* indexedKeyBlock,
				 ObjectArray* oaOutputVectors) const;

	// Regle permettant d'effectuer le calcul des stats pour chaque vecteur de valeur
	// collecte par attribut du bloc sparse
	// Cette regle est alors utilise via ses methodes ComputeContinuousStatsFromVector
	// ou ComputeSymbolStatsFromVector
	// On n'utilise donc pas les operande de la regle et sa methode de calcul
	// standard a partir d'un KWObject
	KWDRTableStats* tableStatsRule;

	// Acces au bloc d'attribut de l'operande dans la table secondaire
	KWAttributeBlock* tableAttributeBlock;

	// Acces a au bloc d'index des cles du block de table,
	// qui est necessairement associe au bloc d'attribut de l'operande associe
	const KWIndexedNKeyBlock* tableBlockIndexedKeyBlock;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableBlockStatsContinuous
// Classe ancetre des regles ayant un premier operande de type ObjectArray
// ete d'eventuels operandes additionnels pour un calcul de stats
// Stats de type block de valeurs Continuous en sortie
class KWDRTableBlockStatsContinuous : public KWDRTableBlockStats
{
public:
	// Constructeur
	KWDRTableBlockStatsContinuous();
	~KWDRTableBlockStatsContinuous();

	// Calcul de l'attribut derive
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;
	Continuous GetValueBlockContinuousDefaultValue() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableBlockStatsSymbol
// Classe ancetre des regles ayant un premier operande de type ObjectArray
// ete d'eventuels operandes additionnels pour un calcul de stats
// Stats de type block de valeurs Symbol en sortie
class KWDRTableBlockStatsSymbol : public KWDRTableBlockStats
{
public:
	// Constructeur
	KWDRTableBlockStatsSymbol();
	~KWDRTableBlockStatsSymbol();

	// Calcul de l'attribut derive
	KWSymbolValueBlock* ComputeSymbolValueBlockResult(const KWObject* kwoObject,
							  const KWIndexedKeyBlock* indexedKeyBlock) const override;
	Symbol& GetValueBlockSymbolDefaultValue() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockCountDistinct
class KWDRTableBlockCountDistinct : public KWDRTableBlockStatsContinuous
{
public:
	// Constructeur
	KWDRTableBlockCountDistinct();
	~KWDRTableBlockCountDistinct();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockEntropy
class KWDRTableBlockEntropy : public KWDRTableBlockStatsContinuous
{
public:
	// Constructeur
	KWDRTableBlockEntropy();
	~KWDRTableBlockEntropy();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMode
class KWDRTableBlockMode : public KWDRTableBlockStatsSymbol
{
public:
	// Constructeur
	KWDRTableBlockMode();
	~KWDRTableBlockMode();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMean
class KWDRTableBlockMean : public KWDRTableBlockStatsContinuous
{
public:
	// Constructeur
	KWDRTableBlockMean();
	~KWDRTableBlockMean();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockStandardDeviation
class KWDRTableBlockStandardDeviation : public KWDRTableBlockStatsContinuous
{
public:
	// Constructeur
	KWDRTableBlockStandardDeviation();
	~KWDRTableBlockStandardDeviation();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMedian
class KWDRTableBlockMedian : public KWDRTableBlockStatsContinuous
{
public:
	// Constructeur
	KWDRTableBlockMedian();
	~KWDRTableBlockMedian();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMin
class KWDRTableBlockMin : public KWDRTableBlockStatsContinuous
{
public:
	// Constructeur
	KWDRTableBlockMin();
	~KWDRTableBlockMin();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMax
class KWDRTableBlockMax : public KWDRTableBlockStatsContinuous
{
public:
	// Constructeur
	KWDRTableBlockMax();
	~KWDRTableBlockMax();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockSum
class KWDRTableBlockSum : public KWDRTableBlockStatsContinuous
{
public:
	// Constructeur
	KWDRTableBlockSum();
	~KWDRTableBlockSum();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockCountSum
// Comme KWDRTableBlockSum, mais renvoie 0 par defaut au lieu de Missing
// Usage pour calcul des cumuls de comptes
class KWDRTableBlockCountSum : public KWDRTableBlockStatsContinuous
{
public:
	// Constructeur
	KWDRTableBlockCountSum();
	~KWDRTableBlockCountSum();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockConcat
class KWDRTableBlockConcat : public KWDRTableBlockStatsSymbol
{
public:
	// Constructeur
	KWDRTableBlockConcat();
	~KWDRTableBlockConcat();

	// Creation
	KWDerivationRule* Create() const override;
};