// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour le deploiement des modeles en grilles de donnees
// pour des bloc sparse de variables numeriques ou categorielles

class KWDRDataGridBlock;
class KWDRCellIndexBlock;
class KWDRDataGridStatsBlock;

#include "KWDRDataGrid.h"

// Enregistrement de ces regles
void KWDRRegisterDataGridBlockRules();

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridBlock
// Regle de derivation permettant de construire la specification
// d'un bloc de DataGrid, avec en premier operande un vecteur de VarKey
// puis un operande de type DataGrid par VarKey.
// Le vecteur de VarKey est de type Structure(ValueSet) pour les VarKey de
// type Continuous,  Structure(ValueSetC) pour les VarKey de type Symbol
// On impose ici que les VarKey soient correctement ordonnes
// Les grilles sont toutes bivariees, avec les memes types pour l'attribut
// source et l'attribut cible
class KWDRDataGridBlock : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDataGridBlock();
	~KWDRDataGridBlock();

	/////////////////////////////////////////////////////
	// Services specifiques disponibles une fois compile

	// Nombre de grilles, et de VarKeys
	int GetDataGridNumber() const;

	// Type de VarKey associee aux grilles
	// On ne peut reutiliser la nom GetVarKeyType, qui est reserve aux regles de type blocks
	int GetDataGridVarKeyType() const;

	// Acces aux VarKey selon leur type
	Symbol GetSymbolVarKeyAt(int nIndex) const;
	int GetContinuousVarKeyAt(int nIndex) const;

	// Acces a une grille
	KWDRDataGrid* GetDataGridAt(int nIndex) const;

	/////////////////////////////////////////////////////
	// Services specifiques disponibles, en mode non checke
	// (plus lents, mais utiles pour le check d'autre regles)

	// Nombre de grilles (renvoie -1 si erreur)
	int GetUncheckedDataGridNumber() const;

	// Nombre de VarKey (renvoie -1 si erreur)
	int GetUncheckedVarKeyNumber() const;

	// Type de VarKey (renvoie KWType::Unknown si erreur)
	int GetUncheckedDataGridVarKeyType() const;

	// Acces aux VarKey selon leur type, si le type et le nombre de VarKey sont corrects
	Symbol GetUncheckedSymbolVarKeyAt(int nIndex) const;
	int GetUncheckedContinuousVarKeyAt(int nIndex) const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Verification des operandes de la regle, pour gerer les operandes non standards de la regle
	//   . le premier operande est de type ValueSet ou ValueSetC selon le type de VarKey associe aux grilles
	//   . les operandes suivant sont les grilles
	//       . accessible soit directement par une regle, soit via un attribut portant la regle
	//       . en meme nombre que le nombre de VarKey
	//       . bivarie supervises, toutes de meme premier type (pour des valeurs sources provenant d'un bloc)
	//         et d'un meme deuxieme type, pour l'attribut cible
	//   ...
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

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

	// Nombre d'attribut de la grille
	int nDataGridAttributeNumber;

	// Type de VarKey des grilles
	int nDataGridVarKeyType;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridBlockRule
// Regle de derivation basee sur un DataGridBlock en premier operande
// et un bloc de valeurs en deuxieme operande, pour effectuer un
// pretraitement sparse sur l'ensemble des valeurs du bloc concernes
// par les VarKey du DataGridBlock
// Ancetre des regle basees sur les blocs de grilles, non instanciable
class KWDRDataGridBlockRule : public KWDerivationRule
{
public:
	// Type de VarKey des grilles, accessible des que la regle est specifiee
	int GetDataGridVarKeyType() const;

	// Regle DataGridBlock utilise, accessibible apres compilation
	KWDRDataGridBlock* GetDataGridBlock() const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Verification des operandes de la regle, pour gerer les operandes non standards de la regle
	//   . le premier operande de type DataGridBlock est lui meme non standard
	//   . il est accessible soit directement par une regle, soit via un attribut portant la regle
	//   . le second operande de type bloc a un type qui depend du DataGridBlovk
	//   . le type de VarKey du DataGridBlock n'est pas acessible de facon standard
	//   ...
	boolean CheckOperandsDefinition() const override;
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Verification du type du dernier argument sur la premiere grille du DataGridBlock
	//    Symbol pour un classifier, Continuous pour un regresseur
	boolean CheckPredictorCompletness(int nPredictorType, const KWClass* kwcOwnerClass) const;

	// Construction du dictionnaire de tous les attributs utilises
	// Redefinition de la methode ancetre pour integrer comme attributs utilises
	// les attributs du bloc source correspondant aux grilles de recodage
	//
	// En effet, la methode ancetre est generique et se base sur l'analyse de l'attribut en sortie de regle
	// et des operandes pour determiner les attributs utilises recursivement via les operandes.
	// Cette methode se base sur les types SymbolValueBlock, ContinuousValueBlock et ObjectArrayBlock
	// pour piloter la gestion des attributs utilises dans les bloc.
	// Comme il n'existe pas de type StructureBlock, le premier operande DataGridBlock utilise un ValueSet
	// ou ValueSetC en premier operande pour specifier ses VarKeys et se comporter comme un bloc de DataGrids,
	// qui sont considerees comme toutes utilisees.
	// La methode ancetre est alors reimplementee pour cette gestion specifique de la recherche des attributs
	// utilises
	void BuildAllUsedAttributes(const KWAttribute* derivedAttribute,
				    NumericKeyDictionary* nkdAllUsedAttributes) const override;

	// Recopie des attributs de definition de la regle
	void CopyFrom(const KWDerivationRule* kwdrSource) override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constructeur
	KWDRDataGridBlockRule();

	// Reimplementation de la methode ancetre de completion des informations de specification de la regle
	// Permet de memorise le type de VarKey associe aux grilles
	void InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
				      NumericKeyDictionary* nkdCompletedAttributes) override;

	// Optimisation apres la compilation, pour le calcul du tableau oaRecodingDataGrids des grilles associees
	// aux VarKey du bloc de valeurs en deuxieme operande, reperee par leur index d'utilisation dans el bloc
	virtual void Optimize(KWClass* kwcOwnerClass);

	// Test si optimisee
	boolean IsOptimized() const;

	// Calcul d'un bloc de valeurs a recoder en precisant pour chaque index de valeur source l'index la valeur a
	// recoder s'il faut la recoder, -1 sinon
	KWContinuousValueBlock* BuildRecodedBlock(const KWValueBlock* sourceValueBlock, int nSourceBlockType,
						  IntVector* ivTargetRecodedValueIndexes) const;

	// Calcul de l'index d'une valeur pour un grille donnees
	int ComputeContinuousCellIndex(const KWDRDataGrid* dataGridRule, Continuous cValue) const;
	int ComputeSymbolCellIndex(const KWDRDataGrid* dataGridRule, Symbol sValue) const;

	// Nombre d'index utilises (differents de -1) dans un vecteur d'index
	int ComputeUsedIndexNumber(IntVector* ivIndexes) const;

	// Tableau de toutes les grilles du DataGridBlock
	mutable ObjectArray oaAllRecodingDataGrids;

	// Vecteur des index de grilles utilises, contenant pour chaque index de valeur source l'index de la grille de
	// recodage si utilisee, -1 sinon
	mutable IntVector ivUsedRecodingDataGridIndexes;

	// Memorisation du type de VarKey du DataGridBlock en premier operande
	mutable int nDataGridVarKeyType;

	// Memorisation de la regle DataGridBlock
	mutable KWDRDataGridBlock* usedDataGridBlock;

	// Fraicheur d'optimisation
	int nOptimizationFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRCellIndexBlock
// Extension de KWDRCellIndex au cas d'un bloc de DataGrid en premier operande
// et d'un bloc de valeur en deuxieme operande
class KWDRCellIndexBlock : public KWDRDataGridBlockRule
{
public:
	// Constructeur
	KWDRCellIndexBlock();
	~KWDRCellIndexBlock();

	// Creation
	KWDerivationRule* Create() const override;

	// Type de cle numerique utilise pour le bloc en retour
	// Methode redefinie specifiquement pour le bloc de valeur en retour de la methode
	int GetVarKeyType() const override;

	// Valeur par defaut du block en retour
	Continuous GetValueBlockContinuousDefaultValue() const override;

	// Calcul de l'attribut derive
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;

	// Verification que les attributs du bloc en sortie de la methode sont tous presents via leur VarKey dans le
	// bloc de grille Methode redefinie specifiquement pour le premier operande de type DataGridBlock
	boolean CheckBlockAttributesAt(const KWClass* kwcOwnerClass, const KWAttributeBlock* attributeBlock,
				       int nOperandIndex) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Compilation de la regle, a appeler en debut de l'implementation du calcul de l'attribut derive
	// Permet de parametrer correctement quels attributs du bloc source sont a utiliser pour
	// fabriquer le bloc cible
	// En theorie, on pourrait effectuer cette compilation des la compilation, mais on a ici besoin a la fois
	// de la regle a compiler, mais egalement du bloc resultat de la regle (et son indexedKeyBlock).
	// La methode Compile ne prenant pas ce type d'argument (pertinent uniquement dans le cas des blocs),
	// il est ici plus pratique (et peu couteux) d'effectuer cette optimisation via DynamicCompile
	void DynamicCompile(const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Vecteur des index de valeurs recodes, contenant pour chaque index de valeur source l'index de valeur recodee
	// si la valeur recodee est gardee, -1 sinon
	// Ce vecteur est calcule lors du DynamicCompile
	mutable IntVector ivRecodedValueIndexes;

	// Fraicheur de compilation dynamique
	mutable int nDynamicCompileFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridStatsBlock
// Extension de KWDRDataGridStats au cas d'un bloc de DataGrid en premier operande
// et d'un bloc de valeur en deuxieme operande
// Les resultats pour le bloc d'index de cellule en sortie sont disponible
// via les methodes de la classe ancetre
class KWDRDataGridStatsBlock : public KWDRDataGridBlockRule
{
public:
	// Constructeur
	KWDRDataGridStatsBlock();
	~KWDRDataGridStatsBlock();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	//////////////////////////////////////////////////////
	// Service disponible une fois la structure calculee
	// On a principalement une methode renvoyant le bloc de valeur recodee
	// plus des services permettant d'exploiter ces information

	// Bloc de recodage des valeurs sources en deuxieme operande pour chaque grille du DataGridBloc en premier
	// operande Le resultat est similaire a celui obtenu pour la regle CellIndexBlock, excepte que les variables
	// sont recodee pour toute les grilles du DataGridBlock, et non pour les variable du bloc en sortie comme dans
	// CellIndexBlock Pour chaque index sparse, on obtient l'index de la grille de recodage et la valeur recodee Il
	// s'agit d'un bloc d'index "internes", compris entre 0 et N-1
	KWContinuousValueBlock* GetCellIndexBlock() const;

	// Taille du bloc d'index de cellule
	int GetCellIndexBlockSize() const;

	// Acces a l'index de grille du DataGridBlock
	int GetDataGridIndexAt(int nValueIndex) const;

	// Acces a un index de cellule d'une variable recodee
	int GetCellIndexAt(int nValueIndex) const;

	// Acces a la VarKey correspondante
	Symbol GetSymbolVarKeyAt(int nValueIndex) const;
	int GetContinuousVarKeyAt(int nValueIndex) const;

	// Acces a la DataGrid correspondante
	const KWDRDataGrid* GetDataGridAt(int nValueIndex) const;

	// Acces a la DataGridStats correspondante, ce qui permet d'utiliser tous ses services
	const KWDRDataGridStats* GetDataGridStatsAt(int nValueIndex) const;

	// Memoire utilisee par la regle de derivation
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Compilation optimisee
	void Optimize(KWClass* kwcOwnerClass) override;

	// Nettoyage du tableau de regles DataGridStats
	void CleanAllDataGridStatsRules();

	// Memorisation du bloc sparse des index de valeur recodee pour toutes les grilles du DataGridBlock en premier
	// operande
	mutable KWContinuousValueBlock* resultCellIndexBlock;

	// Tableau de regles DataGridsStats pour chaque grille du DataGridBlock
	mutable ObjectArray oaAllDataGridStatsRules;
};

///////////////////////////////////////////////////////////////
// Classe KWDRDataGridStatsBlockTest
// Classe de test de KWDRDataGridStatsBlock
// Classe interne uniquement
class KWDRDataGridStatsBlockTest : public KWDerivationRule
{
public:
	// Constructeur
	KWDRDataGridStatsBlockTest();
	~KWDRDataGridStatsBlockTest();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Renvoie une chaine de caracteres avec pour chaque VarKey de valeur presente l'index de grille
	// ainsi que le vecteur des probabilites conditionnelles par classe cible
	// Cette methode fournit un exemple d'utilisation des services de DataGridStatsBlock
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

///////////////////////////////
// Methodes en inline

inline int KWDRDataGridBlock::GetDataGridNumber() const
{
	require(IsCompiled());
	return nDataGridAttributeNumber;
}

inline int KWDRDataGridBlock::GetDataGridVarKeyType() const
{
	require(IsCompiled());
	return nDataGridVarKeyType;
}

inline Symbol KWDRDataGridBlock::GetSymbolVarKeyAt(int nIndex) const
{
	require(IsCompiled());
	require(nDataGridVarKeyType == KWType::Symbol);
	require(0 <= nIndex and nIndex < GetDataGridNumber());
	return cast(KWDRSymbolValueSet*, GetFirstOperand()->GetDerivationRule())->GetValueAt(nIndex);
}

inline int KWDRDataGridBlock::GetContinuousVarKeyAt(int nIndex) const
{
	require(IsCompiled());
	require(nDataGridVarKeyType == KWType::Continuous);
	require(0 <= nIndex and nIndex < GetDataGridNumber());
	return (int)cast(KWDRContinuousValueSet*, GetFirstOperand()->GetDerivationRule())->GetValueAt(nIndex);
}

inline KWDRDataGrid* KWDRDataGridBlock::GetDataGridAt(int nIndex) const
{
	require(IsCompiled());
	require(0 <= nIndex and nIndex < GetDataGridNumber());
	return cast(KWDRDataGrid*, GetOperandAt(nIndex + 1)->GetReferencedDerivationRule(GetOwnerClass()));
}

inline int KWDRDataGridBlock::GetUncheckedDataGridNumber() const
{
	return GetOperandNumber() - 1;
}

inline Symbol KWDRDataGridBlock::GetUncheckedSymbolVarKeyAt(int nIndex) const
{
	require(GetUncheckedDataGridVarKeyType() == KWType::Symbol);
	require(0 <= nIndex and nIndex < GetUncheckedVarKeyNumber());
	return cast(KWDRSymbolValueSet*, GetFirstOperand()->GetDerivationRule())->GetValueAt(nIndex);
}

inline int KWDRDataGridBlock::GetUncheckedContinuousVarKeyAt(int nIndex) const
{
	require(GetUncheckedDataGridVarKeyType() == KWType::Continuous);
	require(0 <= nIndex and nIndex < GetUncheckedVarKeyNumber());
	return (int)cast(KWDRContinuousValueSet*, GetFirstOperand()->GetDerivationRule())->GetValueAt(nIndex);
}

inline int KWDRDataGridBlockRule::GetDataGridVarKeyType() const
{
	return nDataGridVarKeyType;
}

inline KWDRDataGridBlock* KWDRDataGridBlockRule::GetDataGridBlock() const
{
	require(IsCompiled());
	return usedDataGridBlock;
}

inline KWContinuousValueBlock* KWDRDataGridStatsBlock::GetCellIndexBlock() const
{
	require(IsCompiled());
	require(resultCellIndexBlock != NULL);
	return resultCellIndexBlock;
}

inline int KWDRDataGridStatsBlock::GetCellIndexBlockSize() const
{
	return GetCellIndexBlock()->GetValueNumber();
}

inline int KWDRDataGridStatsBlock::GetDataGridIndexAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < GetCellIndexBlockSize());
	return GetCellIndexBlock()->GetAttributeSparseIndexAt(nValueIndex);
}

inline int KWDRDataGridStatsBlock::GetCellIndexAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < GetCellIndexBlockSize());
	assert(GetCellIndexBlock()->GetValueAt(nValueIndex) == (int)GetCellIndexBlock()->GetValueAt(nValueIndex));
	return (int)GetCellIndexBlock()->GetValueAt(nValueIndex);
}

inline Symbol KWDRDataGridStatsBlock::GetSymbolVarKeyAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < GetCellIndexBlockSize());
	return GetDataGridBlock()->GetSymbolVarKeyAt(GetDataGridIndexAt(nValueIndex));
}

inline int KWDRDataGridStatsBlock::GetContinuousVarKeyAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < GetCellIndexBlockSize());
	require(GetDataGridVarKeyType() == KWType::Continuous);
	return GetDataGridBlock()->GetContinuousVarKeyAt(GetDataGridIndexAt(nValueIndex));
}

inline const KWDRDataGrid* KWDRDataGridStatsBlock::GetDataGridAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < GetCellIndexBlockSize());
	require(GetDataGridVarKeyType() == KWType::Symbol);
	return cast(const KWDRDataGrid*, oaAllRecodingDataGrids.GetAt(GetDataGridIndexAt(nValueIndex)));
}

inline const KWDRDataGridStats* KWDRDataGridStatsBlock::GetDataGridStatsAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < GetCellIndexBlockSize());
	return cast(const KWDRDataGridStats*, oaAllDataGridStatsRules.GetAt(GetDataGridIndexAt(nValueIndex)));
}
