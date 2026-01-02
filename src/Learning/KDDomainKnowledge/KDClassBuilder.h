// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDClassBuilder;
class KDSparseUsedConstructedRule;
class KDSparseUsedConstructedBlockRule;

#include "Object.h"
#include "KWClass.h"
#include "KDConstructionDomain.h"
#include "KDConstructedRule.h"
#include "KDMultiTableFeatureConstruction.h"

//////////////////////////////////////////////////////////////////////////
// Classe KDClassBuilder
// Enrichissement d'une classe par un ensemble des regles de derivation
// issue de la construction automatique de variables
class KDClassBuilder : public Object

{
public:
	// Constructeur
	KDClassBuilder();
	~KDClassBuilder();

	// Parametrage du domaine de connaissance
	// Attention, parametrage obligatoire (initialement a NULL)
	// Memoire: appartient a l'appelant
	void SetMultiTableFeatureConstruction(KDMultiTableFeatureConstruction* featureConstructionParam);
	KDMultiTableFeatureConstruction* GetMultiTableFeatureConstruction() const;

	// Acces au domaine de construction, depuis le domaine de connaissance
	KDConstructionDomain* GetConstructionDomain() const;

	///////////////////////////////////////////////////////
	// Construction d'une nouvelle classe
	//
	// Construction d'une nouvelle classe a partir d'une classe initiale et d'un ensemble de regles construites
	// (KDConstructedRule) issues de la construction automatique et de la specification de l'analyse des
	// operandes de selection.
	// Parametres en entree:
	//    . initialClass: classe d'origine, qui sera dupliquee pour produire la classe construite
	//    . oaConstructedRules: regles construites portant sur la classe initiale, qui serviront de base
	//                          a de nouveaux attributs dans la classe construite
	//    . selectionOperandAnalyser: objet d'analyse de l'ensemble des operandes de selection potentiellement
	//                                utilises ou utilisable dans les regles construites
	//
	// La nouvelle classe est cree dans un domaine propre, avec d'eventuelle autre classes (cas multi-tables)
	// Les nouveaux attributs sont nommes d'apres les regles de construction utilisees
	// La classe, et les autres classes du domaine cree, sont enrichies de variables intermediaire creees
	// pour l'optimisation du graphe de calcul, selon le parametre GetConstructionDomain()->GetRuleOptimization()
	// Les attributs construits a partir des regles construites sont tous en used, alors que les attributs
	// intermediaires sont en unused
	// Dans le cas des operandes de selection, chaque attribut de selection, construit ou non, sera parametre
	// et accessible en sortie par KDClassSelectionOperandStats::GetSelectionAttribute()

	// Construction d'une nouvelle classe a partir des regles construites et des operandes de selection
	KWClass* BuildClassFromConstructedRules(const KWClass* initialClass, const ObjectArray* oaConstructedRules,
						KDSelectionOperandAnalyser* selectionOperandAnalyser) const;

	// Construction d'une nouvelle classe a partir des operandes de selection
	// Utile pour la base de collecte d'un echantillon de valeurs de selection
	// Les comptes d'utilisation par operandes de selection doivent avoir ete calcules par l'appelant
	KWClass* BuildClassFromSelectionRules(const KWClass* initialClass,
					      KDSelectionOperandAnalyser* selectionOperandAnalyser) const;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Finalisation des cout de construction, en tenant compte des attributs initiaux
	// En entree:
	//  . les attributs construit sont marques avec un cout indivividuel
	// En sortie:
	//  . on met a jour tous les couts en tenant compte des attributs initiaux,
	//  . on met a jour les cout des attributs initiaux
	//  . On memorise tous ces couts dans les meta-donnees
	void UpdateAllAttributeCosts(KWClass* constructedClass) const;

	// Construction d'un nom de variable a partir d'une regle construite
	// Le nom sera interpretable ou simplement indexe en fonction du parametre du domaine de construction
	ALString BuildConstructedAttributeName(const KDConstructedRule* rule) const;
	ALString BuildConstructedAttributeBlockName(const KDConstructedRule* rule) const;
	ALString BuildPartitionAttributeName(const KDConstructedPartition* partition) const;
	ALString BuildTablePartitionAttributeBlockName(const KDConstructedPartition* partition) const;
	ALString BuildPartAttributeName(const KDConstructedPart* part) const;

	// Nom de variable base sur un index
	ALString BuildIndexedAttributeName() const;

	// Reinitialisation des index utilises pour nnommer les variables
	void ResetAttributeNameIndexes() const;

	//////////////////////////////////////////////////////////////////////////
	// Variantes de construction de la nouvelle classe a partir
	// des regles construites et des operandes de selection

	// Construction d'une nouvelle classe standard, sans variable intermediaires
	KWClass* BuildStandardClassFromConstructedRules(const KWClass* initialClass,
							const ObjectArray* oaConstructedRules,
							KDSelectionOperandAnalyser* selectionOperandAnalyser) const;

	// Construction d'une nouvelle classe optimisee, avec creation de variables intermediaires
	KWClass* BuildOptimizedClassFromConstructedRules(const KWClass* initialClass,
							 const ObjectArray* oaConstructedRules,
							 KDSelectionOperandAnalyser* selectionOperandAnalyser) const;

	// Construction d'une nouvelle classe optimisee, avec creation de variables intermediaires et de blocs sparses
	KWClass*
	BuildSparseOptimizedClassFromConstructedRules(const KWClass* initialClass,
						      const ObjectArray* oaConstructedRules,
						      KDSelectionOperandAnalyser* selectionOperandAnalyser) const;

	//////////////////////////////////////////////////////////////////////////
	// Gestion de liste de regles construites

	// Collecte des regles construites au moyen du domaine de construction, utilisees recursivement depuis une regle
	// La liste en entree doit avoir un tri selon KDSparseUsedConstructedRuleCompare
	// Des objets UsedConstructedRule sont crees si necessaire pour chaque regle ou sous regle, qui sont elles
	// uniquement referencees Memoire: le contenu de la liste appartient a l'appelant
	void CollectUsedRulesFromConstructedRule(SortedList* slUsedConstructedRules,
						 const KDConstructedRule* rule) const;

	// Recherche d'une regle construite utilise dans une liste trie de regles construites
	KDSparseUsedConstructedRule* LookupUsedConstructedRule(const SortedList* slUsedConstructedRules,
							       const KDConstructedRule* searchedRule) const;

	// Affichage d'un ensemble de regles construites utilisees d'une liste triee
	void DisplayUsedConstructedRules(const SortedList* slUsedConstructedRules, ostream& ost) const;

	//////////////////////////////////////////////////////////////////////////
	// Gestion de liste de regles construites de type block
	// dans le cadre de la generation de block sparse,

	// Collecte des regles construites de type block au moyen du domaine de construction, utilisees recursivement
	// depuis une regle La liste en entree doit avoir un tri selon KDSparseUsedConstructedBlockRuleCompare Des
	// objets UsedConstructedBlockRule sont crees si necessaire pour chaque regle ou sous regle, qui sont elles
	// uniquement referencees Une seul objet UsedConstructedBlockRule est cree par bloc, pour memoriser tous les
	// attributs du bloc Memoire: le contenu de la liste appartient a l'appelant
	KDSparseUsedConstructedBlockRule*
	CollectUsedBlockRulesFromConstructedRule(SortedList* slUsedConstructedBlockRules,
						 const KDConstructedRule* rule) const;

	// Recherche d'une regle construite utilise dans une liste trie de regles construites
	KDSparseUsedConstructedBlockRule*
	LookupUsedConstructedBlockRule(const SortedList* slUsedConstructedBlockRules,
				       const KDConstructedRule* searchedBlockRule) const;

	// Affichage d'un ensemble de regles construites utilisees d'une liste triee
	void DisplayUsedConstructedBlockRules(const SortedList* slUsedConstructedBlockRules, ostream& ost) const;

	//////////////////////////////////////////////////////////////////////////
	// Optimisation de la construction de la classe par creation de variables
	// intermediaires pour bufferiser les calculs commun a plusieurs regles
	// Les regles utilisees sont des KWBastractRule, qui peuvent indiferement etre
	// des KWDerivationRule ou des KDConstructedRule

	// Construction d'une nouvelle classe a partir des operandes de selection
	// Seuls les attributs intermediaires concernant les operandes de slection sont  crees
	// Cette methode interne est similaire a la methode publique BuildClassFromSelectionRules qui l'apelle,
	// mais avec des parametres supplementaires pour la rendre reutilisable par BuildClassFromConstructedRules
	//   . classDomainCompliantRules: pour acceder aux attributs derives initiaux du domaine de classe
	//   . slUsedConstructedRules: liste triee des toutes les regles construites,
	//                             avec attribut intermediaire si disponible
	// En sortie, le domaine est construit avec ses attribut derives cree pour chaque regle de selection,
	// et ces parametres, initialement vides, sont initialises par la methode
	KWClass* InternalBuildClassFromSelectionRules(const KWClass* initialClass,
						      KDClassDomainCompliantRules* classDomainCompliantRules,
						      SortedList* slUsedConstructedRules,
						      KDSelectionOperandAnalyser* selectionOperandAnalyser) const;

	// Creation d'un attribut a partir d'une regle construite utilisee, sans optimisation,
	// directement a partir d'une regle de derivation fabriquee a partir de la regle de construite en entree
	// En sortie, on renvoie true si un nouvel attribut a ete construit, false si on a reutilise un attribut
	// existant
	boolean CreateConstructedRuleAttribute(KWClassDomain* constructedDomain,
					       KDClassDomainCompliantRules* classDomainCompliantRules,
					       const KDConstructedRule* constructedRule,
					       KWAttribute*& constructedAttribute) const;

	// Creation d'un attribut a partir d'une regle construite utilisee,
	// avec optimisation de la creation d'attributs intermediaires
	// Entree:
	//   . classDomain: domaine dans l'attribut est cree
	//                  l'attribut classDomainDerivedAttributes doit avoir ete initialise pour ce domaine
	//   . classDomainCompliantRules: pour acceder aux attributs derives initiaux du domaine de classe
	//   . slUsedConstructedRules: liste triee de toutes les regles construites,
	//                             avec attribut intermediaire si disponible
	//   . usedConstructedRule: regle construite, presente dans la liste des regles construites,
	//                      sans attribut intermediaire
	// En sortie, un attribut est cree en Unused, derive avec une regle de derivation creer a partir
	// de la regle construite en entree.
	// La liste slUsedConstructedRules voit son element correspondant a la regle construite enrichi
	// de l'attribut portant la regle.
	// La creation se fait recursivement, et tous les attributs intermediaires sont crees si necessaire,
	// c'est a dire si non deja presents dans les listes en entree
	// En sortie, on renvoie true si un nouvel attribut a ete construit, false si on a reutilise un attribut
	// existant
	boolean CreateOptimizedUsedRuleAttribute(KWClassDomain* classDomain,
						 KDClassDomainCompliantRules* classDomainCompliantRules,
						 const SortedList* slUsedConstructedRules,
						 KDSparseUsedConstructedRule* usedConstructedRule) const;

	// Creation d'un attribut a partir d'une regle construite utilisee,
	// avec optimisation de la creation d'attributs intermediaires et de blocs sparse
	// Il s'agit d'une variante de la methode precedente, avec gestion des blocs sparse en plus
	// En entree, le parametre suivant additionnel est utilise
	//   . slUsedConstructedBlockRules: liste triee des toutes les regles de type blok a construire
	//                                  avec memorisation des attributs intermediaires du bloc
	boolean CreateSparseOptimizedUsedRuleAttribute(KWClassDomain* classDomain,
						       KDClassDomainCompliantRules* classDomainCompliantRules,
						       const SortedList* slUsedConstructedRules,
						       const SortedList* slUsedConstructedBlockRules,
						       KDSparseUsedConstructedRule* usedConstructedRule) const;

	// Reordonancement d'attributs dans leur classe
	// En entree:
	//   . kwcdInitialClassDomain: domaine de classe initial, pour gerer les attributs existant, a la position
	//   inchangee . oaConstructedAttributes: tableau d'attributs uniquement, devant apparaitre en tete .
	//   slUsedConstructedRules: liste triee de toutes les regles construites,
	//                             avec leur attribut intermediaire, potentiellement absent,
	//                             ou faisant partie des attributs existant ou construits
	// En sortie, les attributs derives sont tous mis en fin de classe selon leur ordre dans les tableaux
	// en entree, avec en premier les attributs principaux, suivi des attributs intermediaires
	// non deja traites.
	// Les attributs des blocs, meme principaux, ne sont pas deplaces
	void ReorderAttributesInClassDomain(const KWClassDomain* kwcdInitialClassDomain,
					    const ObjectArray* oaConstructedAttributes,
					    const SortedList* slUsedConstructedRules) const;

	// Reordonnancement des blocs d'attribut dans leur classe
	// En entree:
	//   . slUsedConstructedBlockRules: liste des blocs d'attributs (KDSparseUsedConstructedBlockRule)
	//                                  (s'ils existent)
	// En sortie, les blocs d'attribut sont mise en fin de classe, avec leurs attrributs tries par VarKey
	// Les attributs de type partition et les bloc de type partition de table sont mise encore apres
	void ReorderAttributeBlocksInClassDomain(const SortedList* slUsedConstructedBlockRules) const;

	// Creation d'un attribut et eventuellement de son bloc d'attribut, dans le cas
	// des partitions, des partitions de table, des blocs de type partition ou valeur
	// Le bloc d'attribut a creer est en dernier parametre, et sera cree si necessaire
	// La creation est potentiellement recursive, en creant les bloc utilises en operandes
	// En entree, le parametre usedConstructedRule ne doit pas avoir d'attribut associe,
	// En sortie, l'attribut cree ou reutilise doit etre parametre
	// On renvoie true si l'attribut cree, et false si un attribut existant a ete reutilise
	boolean CreateAttributeBlock(KWClassDomain* classDomain, KDClassDomainCompliantRules* classDomainCompliantRules,
				     const SortedList* slUsedConstructedRules,
				     const SortedList* slUsedConstructedBlockRules,
				     KDSparseUsedConstructedRule* usedConstructedRule,
				     KDSparseUsedConstructedBlockRule* usedConstructedBlockRule) const;
	boolean CreateAttributePartitionBlock(KWClassDomain* classDomain,
					      KDClassDomainCompliantRules* classDomainCompliantRules,
					      const SortedList* slUsedConstructedRules,
					      const SortedList* slUsedConstructedBlockRules,
					      KDSparseUsedConstructedRule* usedConstructedRule,
					      KDSparseUsedConstructedBlockRule* usedConstructedBlockRule) const;
	boolean CreateAttributePartitionAttribute(KWClassDomain* classDomain,
						  KDClassDomainCompliantRules* classDomainCompliantRules,
						  const SortedList* slUsedConstructedRules,
						  const SortedList* slUsedConstructedBlockRules,
						  KWClass* kwcPartitionOwnerClass,
						  KDConstructedPartition* constructedPartition) const;
	boolean CreateAttributeTablePartitionBlock(KWClassDomain* classDomain,
						   KDClassDomainCompliantRules* classDomainCompliantRules,
						   const SortedList* slUsedConstructedRules,
						   const SortedList* slUsedConstructedBlockRules,
						   KWClass* kwcPartitionOwnerClass, KDConstructedPart* constructedPart,
						   KDConstructedPartition* constructedPartition) const;
	boolean CreateAttributeValueBlock(KWClassDomain* classDomain,
					  KDClassDomainCompliantRules* classDomainCompliantRules,
					  const SortedList* slUsedConstructedRules,
					  const SortedList* slUsedConstructedBlockRules,
					  KDSparseUsedConstructedRule* usedConstructedRule,
					  KDSparseUsedConstructedBlockRule* usedConstructedBlockRule) const;

	// Domaine de connaissance
	KDMultiTableFeatureConstruction* multiTableFeatureConstruction;

	// Index du prochain attribut dans le cas d'un nommage indexe, non interpretable
	mutable int nAttributeNameIndex;
};

//////////////////////////////////////////////////////////////////////////
// Classe KDSparseUsedConstructedRule
// Informations sur une regle construite utilisee soit par un attribut,
// soit en operande  d'une autre regle, pour le pilotage de l'optimisation
// d'une classe par creation de variable intermediaires
class KDSparseUsedConstructedRule : public Object

{
public:
	// Constructeur
	KDSparseUsedConstructedRule();
	~KDSparseUsedConstructedRule();

	// Parametrage d'une regle de construction
	// Memoire: appartient a l'appelant
	void SetConstructedRule(const KDConstructedRule* rule);
	const KDConstructedRule* GetConstructedRule() const;

	// Memorisation d'un attribut calcule avec sa regle de derivation correspondant
	// a la regle construite
	// Attention: un meme attribut peut etre utilise par plusieurs usedConstructedRule,
	// car plusieurs regles construites peuvent correspondre au meme attribut existant en cas
	// d'attribut initiaux avec regles de derivation en collision avec un nouvel attribut construit
	// Exemple:
	//   DecimalTime = DecimalTime(Time) : attribut existant
	//   Time.DecimalTime = = DecimalTime(Time) : attribut construit a partir d'une regle appliquee sur Time
	// Memoire: appartient a l'appelant
	void SetAttribute(KWAttribute* attribute);
	KWAttribute* GetAttribute() const;

	//////////////////////////////////////////////////////////////////
	// Services divers

	// Nombre de regle utilisant la regle, a maintenir par l'appelant
	void SetUsingRuleNumber(int nNumber);
	int GetUsingRuleNumber() const;

	// Affichage, ecriture dans un fichier, de la definition formelle d'une regle ou de son usage
	void Write(ostream& ost) const override;
	void WriteHeaderLineReport(ostream& ost) const;
	void WriteLineReport(ostream& ost) const;

	// Verification de l'integrite
	boolean Check() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	const KDConstructedRule* usedConstructedRule;
	KWAttribute* usedAttribute;
	int nUsingRuleNumber;
};

// Methode de comparaison entre deux regles construites utilisees
int KDSparseUsedConstructedRuleCompareCostName(const void* elem1, const void* elem2);

// Methode de comparaison entre deux attributs (KWAttributeBlock) selon leurs regles de derivation,
// et selon leur VarKey en cas de deux attributs de bloc
int KDAttributeDerivationRuleCompare(const void* elem1, const void* elem2);

// Methode de comparaison entre deux blocs d'attributs (KWAttributeBlock) selon leurs regles de derivation
int KDAttributeBlockDerivationRuleCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////
// Classe KDSparseUsedConstructedBlockRule
// Informations sur une regle construite utilisee par un bloc d'attributs, pour le
// pilotage de l'optimisation d'une classe par creation de variable intermediaires
class KDSparseUsedConstructedBlockRule : public Object
{
public:
	// Constructeur
	KDSparseUsedConstructedBlockRule();
	~KDSparseUsedConstructedBlockRule();

	// Parametrage d'une regle de construction de type bloc
	// Cela peut etre la regle d'un attribut quelconque du bloc
	// Memoire: appartient a l'appelant
	void SetConstructedBlockRule(const KDConstructedRule* rule);
	const KDConstructedRule* GetConstructedBlockRule() const;

	// Memorisation d'un bloc d'attribut calcule avec sa regle de derivation correspondant
	// a la regle construite
	// Attention: un meme attributeBlock peut etre utilise par plusieurs usedConstructedBlockRule,
	// comme dans le cas des KDSparseUsedConstructedRule
	// Memoire: appartient a l'appelant
	void SetAttributeBlock(const KWAttributeBlock* attributeBlock);
	const KWAttributeBlock* GetAttributeBlock() const;

	//////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage, ecriture dans un fichier, de la definition formelle d'une regle ou de son usage
	void Write(ostream& ost) const override;

	// Verification de l'integrite
	boolean Check() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	const KDConstructedRule* usedConstructedBlockRule;
	const KWAttributeBlock* usedAttributeBlock;
};

// Methode de comparaison entre deux regles de type bloc construites utilisees
int KDSparseUsedConstructedBlockRuleCompareCostName(const void* elem1, const void* elem2);
