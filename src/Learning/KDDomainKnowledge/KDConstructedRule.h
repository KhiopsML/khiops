// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDConstructedRule;
class KDDataPath;
class KDConstructedPartition;
class KDConstructedPartitionDimension;
class KDConstructedPart;

#include "SortedList.h"
#include "KDSelectionOperandAnalyser.h"
#include "KWDRPreprocessing.h"
#include "KWDRTablePartition.h"
#include "KWDRTableBlock.h"

//////////////////////////////////////////////////////////////////////////
// Classe KDConstructedRule
// Regle construite dans le contexte des algorithmes de constructions de variables
//
// L'ensemble des classes utilise pour la construction de variables est le suivant
//   . KDConstructionRule: specification formelle des regles de construction a utiliser
//   . KDConstructedRule: ensemble des regles effectivement construites par application
//                        des regles de construction
//       . objet compact, de specification des regles de derivation a construire
//       . faible emprunte memoire
//       . destine a permettre la generation optimisee du graphe de calcul des attributs construits,
//         avec factorisation de regles communes via des attributs intermediares en Unused et
//         utilisation de blocs sparse pour les partitions de table secondaire liees aux TableSelection
//   . KDDerivationRule: regle de derivation obtenue par transformation des regles construites
//       . a la fois specification formelle du formule de calcul, stockee dans un fichier dictionnaire
//       . et version optimisee (avec index d'attributs...) pour executer les calculs de
//         facon performante
class KDConstructedRule : public Object
{
public:
	// Constructeur et destructeur
	KDConstructedRule();
	~KDConstructedRule();

	// Regle de construction associee
	// La specification provoque la reinitialisation complete de la regle
	// Memoire: appartient a l'appelant
	void SetConstructionRule(const KDConstructionRule* rule);
	const KDConstructionRule* GetConstructionRule() const;

	// Cout d'une regle utilise pour la regularisation
	void SetCost(double dValue);
	double GetCost() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Acces aux caracteristiques de la regle

	// Nom de la classe sur laquelle porte la regle
	const ALString& GetClassName() const;

	// Nom de la regle
	const ALString& GetName() const;

	// Type de la regle
	int GetType() const;

	// Type d'objet de la regle, pour les regle de type Object ou ObjectArray
	// Il s'agit alors du type de son premier operande
	const ALString& GetObjectClassName() const;

	// Acces aux caracteristiques de la regle de derivation associee a la regle de construction
	const KWDerivationRule* GetDerivationRule() const;

	// Nombre d'operande de la regle: celui de la regle de construction
	int GetOperandNumber() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Mise a jour des operandes
	// Ne peut etre specifie qu'une fois apres la specification de la regle de construction
	// Un operande peut etre de type: attribut, regle de construction ou partie dans une partition
	// dans le cas d'une regle de type TableSelection

	// Operande d'origine attribut (reference)
	// Les attributs appartiennent a la classe (KWClass les contenant)
	void SetAttributeOperandAt(int nIndex, KWAttribute* attribute);
	KWAttribute* GetAttributeOperandAt(int nIndex) const;

	// Operande d'origine regle (appartient a l'appele)
	void SetRuleOperandAt(int nIndex, KDConstructedRule* rule);
	KDConstructedRule* GetRuleOperandAt(int nIndex) const;

	// Operande d'origine partie dans une partition (reference)
	// Les parties appartiennent a une partition, et plus generalement
	// a une structure globale de collecte des informations sur toutes les partitions
	// rencontree (cf. documentation de la classe KDSelectionOperandAnalyser)
	void SetPartOperandAt(int nIndex, KDConstructedPart* part);
	KDConstructedPart* GetPartOperandAt(int nIndex) const;

	// Origine de l'operande
	enum
	{
		None,      // Non specifie (apres initialisation)
		Attribute, // Attribute
		Rule,      // Regle
		Part       // Partie dans une partition
	};

	// Acces a l'origine des operandes
	int GetOperandOriginAt(int nIndex) const;

	// Destruction/supression d'un operande, sans modifier le nombre d'operande
	// La destruction en concerne que les operandes d'origine regle
	void DeleteOperandAt(int nIndex);
	void RemoveOperandAt(int nIndex);

	// Destruction/supression de tous les operandes, sans modifier le nombre d'operande
	void DeleteAllOperands();
	void RemoveAllOperands();

	//////////////////////////////////////////////////////////////////////////////////////
	// Services avances

	// Index aleatoire associee a la regle pour obtenir un ordre alatoire en cas d'egalite dans les
	// tris par cout uniquement. C'est necessaire pour assurer la reproductibilite cross-plateforme.
	// A specifier par l'appelant avant chaque tri
	void SetRandomIndex(int nValue);
	int GetRandomIndex() const;

	// Incrementation du nombre d'utilisations des parties, partitions et dimensions de partition
	void IncrementUseCounts();

	// Construction d'une regle de derivation a partir de la regle construite
	// Il s'agit de la construction de base, sans bloc sparse
	// Memoire: la regle construite appartient a l'appelant
	KWDerivationRule* BuildDerivationRule() const;

	// Construction d'un nom de variable ou de bloc, interpretable a partir de la regle construite
	ALString BuildAttributeName(boolean bIsBlockName) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Services pour la generation de blocks sparse

	// Test du type de regle
	//  . Selection, pouvant la generation d'une regle de type TablePartition
	//  . PartitionBlock, pour la generation d'un bloc d'attributs s'appuyant sur une regle de type TablePartition
	//  . ValueBlock, pour la generation d'un bloc d'attributs s'appuyant sur une variable secondaire dans un bloc
	//  . Block: un des deux cas de bloc
	//  . standard: les autres cas
	boolean IsSelectionRule() const;
	boolean IsPartitionBlockRule() const;
	boolean IsValueBlockRule() const;
	boolean IsBlockRule() const;
	boolean IsStandardRule() const;

	// Test si une regle utilise directement ou indirectement une regle de selection
	boolean UsesSelectionRule() const;

	// Acces a la partie concernee dans le cas d'une regle de selection ou de bloc
	// Renvoie NULL dans le cas d'une regle standard
	const KDConstructedPart* GetUsedPart() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Services standard

	// Duplication
	KDConstructedRule* Clone() const;

	// Verification de l'integrite
	boolean Check() const override;

	// Methode de comparaison entre deux regles, basee sur le nom des regles et operandes
	int Compare(const KDConstructedRule* rule) const;

	// Methode de comparaison entre deux regles de type block
	// Les deux regles sont identiques si elles ne different que sur leur partie,
	// mais sont relatives a la meme partition, quand on est sur la premiere partition rencontree
	// Aux niveaux superieurs, on distingue par contre les parties: deux regles
	// partageant une partie de partition premier niveau seront differentes si
	// elles impliquent une partie differente au deuxieme niveau
	int CompareBlock(const KDConstructedRule* rule) const;
	int CompareBlockCostName(const KDConstructedRule* rule) const;
	int InternalCompareBlock(const KDConstructedRule* rule, int& nPartitionLevel) const;

	// Methode de comparaison avec une regle de derivation, en comparant avec le regle
	// qui serait generee si on utilisait la methode BuildDerivationRule
	// Attention: methode couteuse, qui se base sur la construction d'une regle
	// de derivation temporaire a partir de la reglce construite
	// Utile neanmoins dans le cadre des assertions
	int CompareWithDerivationRule(const KWDerivationRule* derivationRule) const;

	// Methode de comparaison du cout de deux regles
	int CompareCost(const KDConstructedRule* rule) const;

	// Methode de comparaison du cout de deux regles, puis sur le l'index aleatoire en cas d'egalite
	// pour preserver un ordre aleatoire reproductible cross-plateforme
	int CompareCostRandomIndex(const KDConstructedRule* rule) const;

	// Methode de comparaison du cout de deux regles, puis sur les nom en cas d'egalite
	int CompareCostName(const KDConstructedRule* rule) const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Affichage d'un operande
	void WriteOperandAt(int nIndex, ostream& ost) const;

	// Ecriture des details du cout, sous forme hierarchique
	void WriteCostDetails(ostream& ost, const ALString& sTreePrefix) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KDConstructedPart;
	friend class KDConstructedPartitionDimension;

	//////////////////////////////////////////////////////////////////////////
	// Construction de nom de variable

	// Construction d'un nom de variable "interpretable", depuis une regle, une regle de table, ou un operande
	ALString BuildInterpretableName(boolean bIsBlockName) const;
	ALString BuildTableInterpretableName(boolean bIsBlockName) const;
	ALString BuildInterpretableNameFromOperandAt(int nIndex, boolean bIsBlockName) const;

	//////////////////////////////////////////////////////////////////////////
	// Attributs de la classe

	// Regle de construction de reference
	const KDConstructionRule* constructionRule;

	// Operandes de la regle
	ObjectArray oaConstructedOperands;

	// Cout de la regle
	double dCost;

	// Index alatoire pour gerer les cas d'egalite de tri de facon reproductible
	int nRandomIndex;

	// Tableau des type d'operandes (limite a 4 operandes max, ce qui est suffisant)
	char cOperandOrigins[4];
};

// Methode de comparaison entre deux regles, basee sur le nom
int KDConstructedRuleCompare(const void* elem1, const void* elem2);

// Methode de comparaison entre deux regles base sur leur cout et leur simplicite uniquement
int KDConstructedRuleCompareCost(const void* elem1, const void* elem2);

// Methode de comparaison entre deux regles base sur de leur cout, puis sur sur le RandomIndex
int KDConstructedRuleCompareCostRandomIndex(const void* elem1, const void* elem2);

// Methode de comparaison entre deux regles sur leur cout, puis sur leur nom
int KDConstructedRuleCompareCostName(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////
// Classe KDConstructedPartition
// Partition construite dans le contexte des algorithmes de construction de variables
// pour factoriser les parties correspondant a des TableSelection
// La partition contient un ensemble des parties produites par les memes
// partition univariee portant sur des attributs ou regles d'une table secondaire
// Elle est destinee a la creation d'un bloc sparse
class KDConstructedPartition : public Object
{
public:
	// Constructeur et destructeur
	KDConstructedPartition();
	~KDConstructedPartition();

	///////////////////////////////////////////////////
	// Specification des dimensions de la partition

	// Classe de la table sur laquelle porte la partition
	// La specification provoque la reinitialisation complete de la partition
	void SetPartitionClass(const KWClass* kwcClass);
	const KWClass* GetPartitionClass() const;

	// Attribut utilise pour acceder a la table partitionnee
	// A parametrer par l'appelant
	void SetTableAttribute(const KWAttribute* kwaAttribute);
	const KWAttribute* GetTableAttribute() const;

	// Classe de la table contenant l'attribut de partition
	const KWClass* GetParentClass() const;

	// Nombre de dimensions de la partition
	// La specification provoque la reinitialisation complete de la partition
	void SetDimensionNumber(int nValue);
	int GetDimensionNumber() const;

	// Specification des granularites par dimension
	void SetGranularityAt(int nIndex, int nValue);
	int GetGranularityAt(int nIndex) const;

	// Parametrage d'une dimension (referencee)
	// Cf. documentation de la classe KDConstructedPartitionDimension
	void SetDimensionAt(int nIndex, const KDConstructedPartitionDimension* dimension);
	const KDConstructedPartitionDimension* GetDimensionAt(int nIndex) const;

	// Taille de la partition, basee sur le produit des tailles par dimension
	int GetPartitionSize() const;

	// Taille effective de la partition, basee sur le produit des tailles effectives par dimension
	// Alors que la taille par dimension depend de la granularite uniquement, la taille effective
	// depend des partiles effectivement utiles, parce que non traites a une dimension inferieure
	int GetActualPartitionSize() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Comptage des utilisations

	// Acces au nombre d'utilisations
	longint GetUseCount() const;

	// Incrementation du nombre d'utilisations
	void IncrementUseCount();

	// Incrementation du nombre d'utilisations, et propagation aux dimensions
	void IncrementUseCounts();

	// Remise a zero du nombre d'utilisations de la partition
	void ResetUseCount();

	// Remise a zero du nombre d'utilisations de chaque partie de la partition
	void ResetAllPartsUseCounts();

	////////////////////////////////////////////////////////////////////
	// Specification des parties de la partition

	// Ajout de parties effectivement utilisees dans la partition
	// On utilise un vecteur d'index pour identifier chaque partie.
	// Chaque index identifie un partie par attribut, selon la granularite de l'attribut
	// Memoire: le vecteur d'index appartient a l'appelant, la partie retournee appartient a l'appele
	KDConstructedPart* AddPart(const IntVector* ivPartileIndexes);

	// Recherche d'une partie selon un vecteur d'index
	// Renvoie NULL si partie non existante
	KDConstructedPart* LookupPart(const IntVector* ivPartileIndexes) const;

	// Nombre total de parties
	int GetPartNumber() const;

	// Export de l'ensemble des parties dans un tableau
	// Memoire: les parties appartiennent a l'appele
	void ExportParts(ObjectArray* oaExportedParts) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des attributs generes a partir de la partition
	// Ces attributs sont memorises uniquement dans cette classe
	// Il sont generes puis utilises lors de la construction de la classe par le ClassBuilder

	// Creation d'une regle de derivation de type Partition, specifiant le produit cartesien des partition univariee
	KWDerivationRule* BuildPartitionDerivationRule() const;

	// Creation d'une regle de derivation de type TablePartition, specifiant la partition sparse de la table
	// secondaire selon l'attribut de Partition Attention: tous les attributs derives necessaires doivent avoir ete
	// prealablement crees et parametres
	//  . l'attribut de partition
	//  . les attribut de selection pour chaque dimension de la partition
	KWDerivationRule* BuildTablePartitionBlockDerivationRule() const;

	// Construction d'un nom d'attribut pour chaque type de partition (nom dediee a un bloc)
	const ALString BuildPartitionAttributeName() const;
	const ALString BuildTablePartitionAttributeBlockName() const;

	// Construction d'un nom caracterisant les attributs de selection (nom dediee a un bloc)
	const ALString BuildSelectionName() const;

	// Memorisation d'un attribut de type Partition
	const KWAttribute* GetPartitionAttribute() const;
	void SetPartitionAttribute(const KWAttribute* attribute);

	// Memorisation d'un bloc d'attributs de type TablePartition
	const KWAttributeBlock* GetTablePartitionAttributeBlock() const;
	void SetTablePartitionAttributeBlock(const KWAttributeBlock* attributeBlock);

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des attributs lies au partie de la partition

	// Calcul de l'index de variable dans un bloc de type TablePartition,
	// pour une partie de la partition
	// Les attributs de partition doivent avoir ete construits et memorises
	int ComputeVariableKeyIndex(const KDConstructedPart* constructedPart) const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Services standard

	// Verification de l'integrite
	boolean Check() const override;

	// Methode de comparaison entre deux partitions (sans leurs parties)
	int Compare(const KDConstructedPartition* partition) const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Affichage d'une dimension
	void WriteDimensionAt(int nIndex, ostream& ost);

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Classe sur laquelle porte la partition
	const KWClass* kwcPartitionClass;

	// Attribut d'acces a la table partitionnee
	const KWAttribute* tableAttribute;

	// Specification des attributs de la partition, et de leur granularite
	ObjectArray oaPartitionDimensions;
	IntVector ivPartitionGranularities;

	// Liste triee des parties, pour une recherche efficace selon les index de partiles
	SortedList slSortedParts;

	// Nombre d'utilisations
	longint lUseCount;

	//////////////////////////////////////////////////////////////////////
	// Gestion des partitions construites dans les attributs
	// Attention, le nombre de parties construites effectivement peut etre potentiellement
	// inferieur au nombre de partie theorique (granularite) par dimension
	// Et il faut alors mettre en place une correspondance entre les index de partie theoriques,
	// et les index des parties construites

	// Nombre de parties construites par dimension
	mutable IntVector ivPartitionBuiltPartNumbers;

	// Tableau par dimension de vecteurs de correspondance, de taille la granularite theorique
	// Chaque vecteur permet d'associe a chaque index theorique sont index de partie construite
	ObjectArray oaPartitionBuiltPartIndexes;

	// Attributs de gestion de la partition
	const KWAttribute* kwaPartitionAttribute;
	const KWAttributeBlock* kwabTablePartitionAttributeBlock;

	// Dictionaire des attributs du bloc, par VarKey
	NumericKeyDictionary nkdBlockAttributesByVarKeys;
};

// Methode de comparaison entre deux partitions
int KDConstructedPartitionCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////
// Classe KDConstructedPartitionDimension
// Dimension d'une partition construite dans le contexte des algorithmes de constructions de variables
// Permet un acces generique a la specification d'une dimension, en lecture
// Une dimension de partition appartient a un objet KDClassSelectionOperandStats
// afin de mutualiser les dimensions dans une objet KDClassSelectionStats
// pour toutes les partitions d'une meme classe
class KDConstructedPartitionDimension : public Object
{
public:
	// Dimension de la partition d'origine attribut (reference)
	const KWAttribute* GetAttribute() const;

	// Dimension de la partition d'origine regle (appartient a la dimension)
	const KDConstructedRule* GetRule() const;

	// Origine de la dimension
	int GetOrigin() const;

	// Origine de dimension
	enum
	{
		None,      // Non specifie (apres initialisation)
		Attribute, // Attribute
		Rule       // Regle
	};

	// Type de la dimension
	int GetType() const;

	// Operande de selection correspondant a la dimension
	const KDClassSelectionOperandStats* GetClassSelectionOperandStats() const;

	// Attribut de selection
	//  . soit celui de la dimension dans le cas d'un origine de type attribut,
	//  . soit l'attribut derive genere pour la regle (ou NULL s'il na pas ete genere)
	const KWAttribute* GetSelectionAttribute() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Comptage des utilisations

	// Acces au nombre d'utilisations
	longint GetUseCount() const;

	// Incrementation du nombre d'utilisations
	void IncrementUseCount();

	// Remise a zero du nombre d'utilisations
	void ResetUseCount();

	//////////////////////////////////////////////////////////////////////////////////////
	// Services standard

	// Verification de l'integrite
	boolean Check() const override;

	// Methode de comparaison entre deux dimensions de partition
	int Compare(const KDConstructedPartitionDimension* partitionDimension) const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KDConstructedPartition;
	friend class KDConstructedPart;
	friend class KDMultiTableFeatureConstruction;
	friend class KDClassSelectionStats;
	friend class KDClassSelectionOperandStats;

	// Constructeur et destructeur
	KDConstructedPartitionDimension();
	~KDConstructedPartitionDimension();

	// Dimension de la partition d'origine attribut (reference)
	void SetAttribute(const KWAttribute* attribute);

	// Dimension de la partition d'origine regle (appartient a l'appele)
	void SetRule(const KDConstructedRule* rule);

	// Construction d'un nom d'operande de selection interpretable pour une dimension de partition
	// Le nom fabrique ne peut pas etre un nom de bloc
	ALString BuildInterpretableName() const;

	// Supression de la specification (sans detruire l'eventuelle regle)
	void Clean();

	// Operande de selection correspondant a la dimension
	void SetClassSelectionOperandStats(const KDClassSelectionOperandStats* operandStats);

	// Dimension d'origine Attribute (KWAttribute*), reference
	// ou d'origine Rule (KDConstructedRule*), appartenant a l'appele
	const Object* oDimension;

	// Origine de la dimension
	char cOrigin;

	// Stats sur l'operande de selection correspondant a la dimension
	const KDClassSelectionOperandStats* classSelectionOperandStats;

	// Nombre d'utilisations
	longint lUseCount;
};

// Methode de comparaison entre deux dimensions de partitions
int KDConstructedPartitionDimensionCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////
// Classe KDConstructedPart
// Partie dans une partition construite dans le contexte des algorithmes de constructions de variables
class KDConstructedPart : public Object
{
public:
	// Acces a la partition contenant la partie
	KDConstructedPartition* GetPartition() const;

	// Acces aux index de partiles par attribut
	const IntVector* GetPartilesIndexes() const;

	// Cout d'une partie utilise pour la regularisation
	void SetCost(double dValue);
	double GetCost() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Comptage des utilisations

	// Acces au nombre d'utilisations
	longint GetUseCount() const;

	// Incrementation du nombre d'utilisations
	void IncrementUseCount();

	// Remise a zero du nombre d'utilisations
	void ResetUseCount();

	//////////////////////////////////////////////////////////////////////////////////////
	// Services avances

	// Construction d'une regle de derivation a partir de la partie
	// La construction utilise les attributs derives associes aux dimensions de partition
	// s'ils sont disponibles
	// Memoire: la regle construite appartient a l'appelant
	KWDerivationRule* BuildDerivationRule() const;

	// Construction d'un nom d'attribut pour une partie de partition
	const ALString BuildPartAttributeName() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Services standard

	// Verification de l'integrite
	boolean Check() const override;

	// Methode de comparaison entre deux parties d'une meme partition (sans leur partition)
	int Compare(const KDConstructedPart* part) const;

	// Methode de comparaison du cout de deux parties
	// En cas d'egalite, un critere heuristique de "simplicite" est utilise, base sur
	// un preference des faibles nombres de dimensions, et sur des operandes simples
	int CompareCost(const KDConstructedPart* part) const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Ecriture des details du cout, sous forme hierarchique
	void WriteCostDetails(ostream& ost, const ALString& sTreePrefix) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KDConstructedPartition;
	friend class KDConstructedRule;
	friend class KDMultiTableFeatureConstruction;

	// Constructeur et destructeur
	KDConstructedPart();
	~KDConstructedPart();

	// Parametrage de la partition (referencee)
	// Le vecteur de partile est retaille en consequence
	void SetPartition(KDConstructedPartition* constructedPartition);

	// Creation d'un operande de selection pour une regle de derivation de type conjonction
	// a partir d'une dimension, d'une granularite et d'un index de partile
	//   Cas numerique:
	//      Missing: operande de type EQ(attribute|rule, Missing)
	//      not Missing (]Missing; +inf[): operande de type NEQ(attribute|rule, Missing)
	//      ]Missing; ub]: operande de type LE(attribute|rule, ub)  (pas d'intervalle missing)
	//      ]-inf; ub]: operande de type InInterval(IntervalBounds(Missing, ub), attribute|rule)  (un intervalle
	//      missing precedent l'intervalle) ]lb; ub]: operande de type InInterval(IntervalBounds(lb, ub),
	//      attribute|rule) ]lb; +inf[: operande de type G(attribute|rule, lb)
	//   Cas categoriel:
	//      value: operande de type EQc(attribute|rule, value)
	//      hors d'un groupe {val1, val2...}: operand du type Not(InGroup(ValueGroup(val1, val2...),
	//      attribute|rule))
	KWDerivationRule* BuildSelectionRuleOperand(const KDConstructedPartitionDimension* partitionDimension,
						    int nGranularity, int nPartIndex) const;

	// Parametrage d'un operande a partir d'une dimension de partition
	void FillOperandFromPartitionDimension(KWDerivationRuleOperand* operand,
					       const KDConstructedPartitionDimension* partitionDimension) const;

	// Creation d'une regle de type And a partir d'un ensemble d'operandes de type regle
	// Les operandes sont tries en sortie
	KWDRAnd* BuildSelectionRuleFromOperands(ObjectArray* oaOperandRules) const;

	//////////////////////////////////////////////////////////////////////////
	// Construction de nom de variable

	// Construction d'un nom d'operande de selection interpretable, en exploitant le path d'acces a la table
	ALString BuildInterpretableSelectionName() const;

	// Construction d'un nom d'operande de selection interpretable pour une dimenssion de partition
	ALString BuildInterpretableSelectionOperandName(const KDConstructedPartitionDimension* partitionDimension,
							int nGranularity, int nPartIndex) const;

	// Nettoyage d'une valeur categorielle: supression des tabulations et trim
	ALString CleanValue(const char* sValue) const;

	//////////////////////////////////////////////////////////////////////////
	// Attributes de la classe

	// Partition contenant la partie
	KDConstructedPartition* partition;

	// Index de partiles par attribut
	IntVector ivPartilesIndexes;

	// Cout de la partie
	double dCost;

	// Nombre d'utilisations
	longint lUseCount;
};

// Methode de comparaison entre deux parties d'une meme partition
int KDConstructedPartCompare(const void* elem1, const void* elem2);

///// Methode en inline

inline void KDConstructedRule::SetCost(double dValue)
{
	require(dValue >= 0);
	dCost = dValue;
}

inline double KDConstructedRule::GetCost() const
{
	return dCost;
}
