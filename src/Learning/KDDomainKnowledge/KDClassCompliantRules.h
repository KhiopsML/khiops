// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDClassDomainCompliantRules;
class KDClassCompliantRules;

#include "KWClass.h"
#include "KDConstructionRule.h"
#include "KDConstructedRule.h"

/////////////////////////////////////////////////////////////////////
// Classe KDClassDomainCompliantRules
// Cette classe algorithmique contient un ensemble coherent d'objets
// de type KDClassCompliantRules, dont le role est de
//   . memoriser les regles de calcul effectivement appliquables sur une classe,
//     recursivement ou non, en tenant compte des contraintes du domaine
//   . gerer efficacement les collision potentielle entre regles construites
//     et attributs derives initiaux
// L'initialisation de cette classe est a faire depuis l'appelant ayant
// a disposition l'ensemble du contexte
class KDClassDomainCompliantRules : public Object
{
public:
	// Constructeur et destructeur
	KDClassDomainCompliantRules();
	~KDClassDomainCompliantRules();

	// Domaine de construction
	void SetConstructionDomain(KDConstructionDomain* constructionDomainParam);
	KDConstructionDomain* GetConstructionDomain() const;

	// Classe principale
	void SetClass(KWClass* kwcMainClass);
	KWClass* GetClass() const;

	// Ajout d'une classe avec ses contraintes
	// Memoire: l'objet en parametre appartient desormais a l'appele
	void AddClassCompliantRules(KDClassCompliantRules* classCompliantRules);

	// Nettoyage complet
	void Clean();

	// Acces aux resultats sous la forme d'un tableau de contraintes par classe
	// Memoire: le tableau et son contenu (KDClassCompliantRules) appartiennent a l'appele
	ObjectArray* GetAllClassesCompliantRules();

	// Acces direct aux regles applicables pour la classe principale (premier element du tableau)
	KDClassCompliantRules* GetMainClassCompliantRules() const;

	// Acces aux regles applicable par nom de classe
	KDClassCompliantRules* LookupClassCompliantRules(const ALString& sClassName) const;

	// Nombre total de regles appliquables sur l'ensemble des classes
	int GetTotalClassCompliantRuleNumber() const;

	// Indique si la regle de selection est appliquable sur au moins une classe
	boolean IsSelectionRuleUsed() const;

	// Nombre total d'attribut derives initiaux identifie dans l'ensemble des classes
	int GetTotalInitialConstructedAttributeNumber() const;

	// Affichage de la liste d'attributs derives pour toutes les classes
	void DisplayAllConstructedAttributes(ostream& ost) const;

	//////////////////////////////////////////////////////////////////////
	// Services generiques

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Domaine de construction
	KDConstructionDomain* constructionDomain;

	// Classe principale
	KWClass* kwcClass;

	// Acces indexe et par cle aux contraintes par classes
	ObjectArray oaAllClassesCompliantRules;
	ObjectDictionary odAllClassesCompliantRules;
};

///////////////////////////////////////////////////////////////////////////////////
// Classe KDClassCompliantRules
// Cette classe algorithmique, associee a une classe, permet de
//   . memoriser les regles de calcul effectivement appliquables sur une classe,
//     recursivement ou non, en tenant compte des contraintes du domaine
//   . gerer efficacement les collision potentielle entre regles construites
//     et attributs derives initiaux
// L'initialisation de cette classe est a faire depuis l'appelant ayant
// a disposition l'ensemble du contexte
class KDClassCompliantRules : public Object
{
public:
	// Constructeur et destructeur
	KDClassCompliantRules();
	~KDClassCompliantRules();

	// Nom de la classe referencee
	const ALString GetClassName() const;

	// Reference a la classe sur laquelle portent les contraintes
	// Memoire: appartient a l'appelant
	void SetClass(const KWClass* aClass);
	const KWClass* GetClass() const;

	// Gestion d'attributs interdits
	// Memoire: il s'agit d'attribut de la classe, identifie par leur nom
	ObjectDictionary* GetForbiddenAttributes();

	// Test si un attribut est interdit
	boolean IsAttributeForbidden(const ALString& sAttributeName) const;

	// Test si un attribut est redondant, parce que base sur la meme regle de derivation
	// qu'un autre attribut utilise
	boolean IsAttributeRedundant(const ALString& sAttributeName) const;

	////////////////////////////////////////////////////////////////////////////
	// Gestion des regles de construction applicables sur la classe
	// Memoire: les regles appartiennent a l'appele

	// Insertion (echec si regle de meme nom existante)
	boolean InsertCompliantConstructionRule(KDConstructionRule* constructionRule);

	// Acces massifs
	int GetCompliantConstructionRuleNumber() const;
	KDConstructionRule* GetCompliantConstructionRuleAt(int i) const;

	// Supression de toutes les regles de construction
	void RemoveAllCompliantConstructionRules();

	// Recherche d'une regle de constuction par son nom (recherche couteuse, par parcours exhaustif des regles)
	// Renvoie NULL si non trouve
	KDConstructionRule* SearchCompliantConstructionRule(const ALString& sRuleName) const;

	/////////////////////////////////////////////////////////////////////////////////
	// Services de gestion des collisions entre regles construite et attribut derives

	// Collecte de tous les attributs et blocs derives de la classe, potentiellement reconstructibles avec une regle
	// de construction et identification des attributs redondants
	void CollectConstructedAttributesAndBlocks();

	// Collecte de tous les attributs et blocs derives de la classe, cette fois en passant un dictionnaire des
	// regles de derivation utilisables, et identification des attributs redondants
	void CollectConstructedAttributesAndBlocksUsingDerivationRules(const ObjectDictionary* odUsableDerivationRules);

	// Nombre d'attributs ou de blocs derives de la classe
	int GetConstructedAttributeNumber() const;
	int GetConstructedAttributeBlockNumber() const;

	// Affichage de la liste d'attributs ou de blocs derives
	void DisplayConstructedAttributes(ostream& ost) const;
	void DisplayConstructedAttributeBlocks(ostream& ost) const;

	// Recherche d'un attribut ou d'un bloc derive a partir d'une formule construite
	// La methode est couteuse au premier appel: on construit une regle de derivation intermediaire pour rechercher
	// s'il existe un attribut derive correspondant
	// Les appels suivants beneficient d'une bufferisation de la recherche
	const KWAttribute* LookupConstructedAttribute(const KDConstructedRule* searchedConstructedRule) const;

	// Recherche d'un attribut ou d'un bloc derive a partir d'une regle de derivation
	// On renvoie un attribut ayant une formule equivalente, ou NULL si non trouve
	const KWAttribute* LookupDerivedAttribute(const KWDerivationRule* searchedDerivationRule) const;
	const KWAttributeBlock* LookupDerivedAttributeBlock(const KWDerivationRule* searchedDerivationRule) const;

	// Enregistrement d'un nouvel attribut ou bloc derive selon sa regle de derivation
	void RegisterDerivedAttribute(const KWAttribute* attribute);
	void RegisterDerivedAttributeBlock(const KWAttributeBlock* attributeBlock);

	// Nettoyage des attributs et bloc potentiellement construit collectes
	// Doit etre appele avant le destructeur
	void CleanCollectedConstructedAttributesAndBlocks();

	//////////////////////////////////////////////////////////////////////
	// Services generiques

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	/////////////////////////////////////////////////////////////////
	// Gestion des attributs derives a partir de regles construites
	//
	// Quand on recherche un attribut derive correspondant a une regle construite, il est necessaire
	// de construire la regle de derivation a partir de la regle construite, pour pouvoir la comparer
	// aux regles des attributs derives initiaux
	// Ce processus etant couteux en temps de calcul, il est optimise.
	// Premierement, on ne memorise que les attributs derives dont la regle est une des regles applicables
	// de la clase (cf. methodes CollectConstructedAttributes).
	// Ensuite, on bufferise les rercherches pour ne construire les attributs derives que lors de
	// la premiere recherche. La recherche optimisee utilise alors l'algorithme suivant:
	//   . si pas d'attribut derive (si nDerivedAttributeNumber==0)
	//     -> retourne NULL
	//   . sinon
	//     . recherche dans la liste de regles construites utilisees (slSearchedUsedConstructedRules)
	//     . si trouve, on renvoie l'attribut derive (ou NULL) associe
	//     . sinon
	//       . si on a identifie toutes les regles construites correspondant aux attribut derives
	//         (c'est a dire si nDerivedAttributeNumber==nDerivedAttributesUsedConstructedRulesNumber)
	//         -> retourne NULL
	//       . sinon, on creer une nouvelle regles construite utilisee pour bufferiser la requete
	//         . on recherche si elle correspond a un attribut derive (ou NULL) et on memorise ce resultat
	//         . si on a cette fois identifies toues les attributs derives
	//             . on nettoie les requetes bufferisees devenues inutiles (associees a NULL)
	//             . on memorise cette informartion (nDerivedAttributesUsedConstructedRulesNumber =
	//             nDerivedAttributeNumber)
	//         -> on renvoie le resultat

	// Recherche d'une regle construite pour un attribut ou un bloc utilisee dans la liste trie de regles
	// construites
	KDSparseUsedConstructedRule* LookupUsedConstructedRule(const KDConstructedRule* searchedRule) const;
	KDSparseUsedConstructedBlockRule*
	LookupUsedConstructedBlockRule(const KDConstructedRule* searchedBlockRule) const;

	// Destruction des regles construites utilisee associee a NULL
	void DeleteNullUsedConstructedRules() const;

	// Liste triee des attributs et blocs derives de la classe, permettant de tester si une regle de contruction
	// correspond a un attribut derive
	// Il est a noter que ce test est "cher" a calculer, puisque qu'il faut construire une regle de
	// derivation temporaire a partir d'une regle construite a tester
	SortedList* slDerivedAttributes;
	SortedList* slDerivedAttributeBlocks;

	// Liste des triee de regles construites, correspondant a des attributs ou bloc derives
	// Cela permet de bufferiser le test d'existence d'un attribut derive a partir d'une regle construite:
	// quand une regle construite est identifiee comme correspondant a un attribut derive, on la duplique
	// et on l'insere dans la liste suivante. Pour les tests suivants, on fera le test dans cette liste
	// de regles construites en prealable, et seulement si necessaire dans la liste des attributs derives
	// Les UsedConstructedRules permettent de memoriser s'il y a ou non un attribut derive associe
	// Memoire: le contenu de la liste (KDSparseUsedConstructedRule) et les regles construites correspondantes
	// appartiennent a l'objet en cours
	SortedList* slSearchedUsedConstructedRules;
	SortedList* slSearchedUsedConstructedBlockRules;

	// Nombre de recherches, pour des raisons de collecte de statistiques uniquement
	mutable int nLookupNumber;

	// Nombre d'attributs et bloc derives pour la classe
	// Redondant avec la taille de slDerivedAttributes et slDerivedAttributeBlocks, mais permet d'etre plus efficace
	// dans le cas ou ce nombre est 0
	int nDerivedAttributeNumber;
	int nDerivedAttributeBlockNumber;

	// Nombre de UsedConstructedRules de slSearchedUsedConstructedRules, referencant un attribut derive de la classe
	// Quand on a identifie toutes les regle construites aboutissant aux attributs derives, la liste
	// slSearchedUsedConstructedRules peut etre nettoyee pour ne garder que les regles construites correspondant aux
	// attributs derives
	mutable int nDerivedAttributesUsedConstructedRulesNumber;

	// Caracteristque principale de la classe
	const KWClass* kwcClass;
	ObjectArray oaCompliantConstructionRules;
	ObjectDictionary odForbiddenAttributes;
	ObjectDictionary odRedundantAttributes;
};