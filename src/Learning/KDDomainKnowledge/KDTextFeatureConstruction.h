// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDTextFeatureConstruction;
class KDTextAttributePath;
class KDTextClass;

#include "KDFeatureConstruction.h"
#include "KWClass.h"
#include "KDTokenFrequency.h"
#include "KWDRTextualAnalysis.h"
#include "KDMultinomialSampleGenerator.h"
#include "KDTextTokenSampleCollectionTask.h"
#include "KDClassCompliantRules.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KDTextFeatureConstruction
// Pilotage de la construction de variables pour des donnees de type texte (Text ou TextList)
class KDTextFeatureConstruction : public KDFeatureConstruction
{
public:
	// Constructeur et destructeur
	KDTextFeatureConstruction();
	~KDTextFeatureConstruction();

	// Construction de variables interpretables (defaut: false)
	// En mode non interpretable, les variables sont basee sur des n-grammes de bytes:
	//   . entierement automatique
	//   . rapide
	//   . bonne performances attendues en general
	//   . boite noire: pas d'interpretation possible
	// En mode interpretable, les variables sont bases sur les tokens frequents dans les texte,
	// un token etant une suite de caracteres obtenu lors du parsing d'un texte avec le caractere ' '
	// comme separateur:
	//   . demande un pretraitement des textes pour etre efficace
	//      . lemmatisation, stemmatisation, supression des caracteres accentues, des chiffres...
	//      . expertise necessaire
	//   . demande une passe de lecture initiale sur les donnees pour collecter les token les plus frequents
	//   . performances attendues selon la qualite des pretraitements des textes
	void SetInterpretableMode(boolean bValue);
	boolean GetInterpretableMode() const;

	// Indique si un dictionnaire contient des variables de type texte utilisees directement ou via un autre
	// dictionnaire accessible via ses sous-entite et sous-tables
	boolean ContainsTextAttributes(const KWClass* kwcClass) const;

	// Construction de variables a partir des variables de type texte disponible dans une classe
	// Ajout des variables construites au dictionnaire en parametre, supposee correctement parametree
	// avec les couts des variables initiales ou construites par ailleurs
	// En sortie, on fournit les attributs utilises construits references par leur nom
	boolean ConstructTextFeatures(KWClass* kwcClass, int nFeatureNumber,
				      ObjectDictionary* odConstructedAttributes) const;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Analyse du schema multi-table pour identifier toutes les variable de type texte (Text ou TextList)
	// a ramener dans la table principale pour gerer des variables basee sur les textes
	// Le type TextList est utilise pour collecter des variable de type texte presente dans des
	// sous-tables en relation 0-n d'un schema multi-table et les ramenerr dans la table principale.
	// Il s'agit d'une alternative efficcae a la concatenation de variables Text, qui poserait des problemes
	// de volumetrie, avec un des variables Symbol sans limite de taille. Avec des TextList geree dans des
	// SymbolVector, tout ce qui peut etre charge dans une sous-table tient sans problème en memoire dans
	// un SymbolVector, qui ne fait que reference les Symbol des sous-tables.

	// Methode interne de detection si un dictionnaire contient des variables de type texte utilises,
	// avec un dictionnaire de classes en parametre pour arreter les recursion potentielles
	boolean InternalContainsTextAttributes(const KWClass* kwcClass, NumericKeyDictionary* nkdExploredClasses) const;

	// Collecte des information sur les classes impliquees dans des chemins d'attribut menant a des attributs
	// utilises de type texte. Les informations par classe sont memorisee dans des objet KDTextClass du dictionnaire
	// en sortie. Les chemins d'attribut (KDTextAttributePath) menant a tous les attributs utilises de type texte
	// accessible depuis la classe principale soit directement soit via des sous-tables, sont collectes dans l'Objet
	// KDTextClass correspondant a cette classe principale. Memoire: le contenu du dictionnaire en sortie appartient
	// a l'appelant
	void CollectAllTextClasses(const KWClass* kwcMainClass, ObjectDictionary* odTextClasses) const;

	// Methode interne prenant en parametre le chemin de base a partir duquel on rajoute recursivement les attributs
	// rencontres jusqu'a construire les chemins completes
	// Toute nouvelle classe detectee donne lieu a creation d'un objet KDTextClass memorise dans le dictionanire en
	// sortie On renvoie true si la classe en entree contient recursivement des variables de type texte Parametres
	//   . kwcMainClass: classe principale, pour laquelle on ajoutera tous les chemins menant aux attributs de type
	//   texte . kwcClass: classe en cours d'analyse . baseTextAttributePath: chemin d'attribut en cours, depuis la
	//   classe principale . odTextClasses: dictionnaire des KDTextClass pour les classe analysees
	boolean InternalCollectAllTextClasses(const KWClass* kwcMainClass, const KWClass* kwcClass,
					      KDTextAttributePath* baseTextAttributePath,
					      ObjectDictionary* odTextClasses) const;

	// Calcul des couts de construction pour les chemins d'acces aux attribut de type texte d'une classe
	// Ces couts sont memorise dans le chemins d'attributs
	void ComputeTextClassConstructionCosts(const KWClass* kwcClass, const ObjectDictionary* odTextClasses) const;

	// Calcul des nombres de variables construites pour les chemins d'acces aux attributs de type texte d'une classe
	// Ces nombres sont memorises dans les chemins d'attributs (ConstructeFeatureNumber)
	void ComputeTextClassConstructedFeatureNumbers(const KWClass* kwcClass, const ObjectDictionary* odTextClasses,
						       int nFeatureNumber) const;

	// Collecte d'un echantillon des tokens valeurs par variable secondaire de type texte par analyse de la base
	// Renvoie true si OK (false si par exemple tache interrompue)
	boolean ExtractTokenSamples(const KWClass* kwcClass, const ObjectDictionary* odTextClasses) const;

	// Tache de lecture de la base pour collecter un echantillon des tokens valeurs par variable secondaire de type
	// texte Les chemins d'attributs en entree on un nombre d'attributs a construire specifie. En sortie, un
	// echantillon de token est alimente pour chaque chemin d'attribut.
	boolean TaskCollectTokenSamples(const KWClass* tokenExtractionClass,
					const ObjectDictionary* odTextClasses) const;

	// Construction d'un domaine de lecture de la base pour la collecte des echantillons de tokens,
	// c'est a dire ayant tout en Unused, sauf les variable de type texet permettant d'acceder aux
	// variable secondaire de type texte. Le domain fabrique est compile et pret a l'emploi
	//
	// Les nombres de variables construites existantes de type token sont collectes pour les chemins d'acces aux
	// attributs de type texte d'une classe, et sont memorises dans les chemins d'attributs (ExistingTokenNumber) Le
	// domain est enssuite nettoye de ses variables inutiles pour optimiser les acces aux donnees Memoire: le
	// domaine et son contenu appartiennent a l'appelant
	KWClassDomain* BuildTokenExtractionDomainWithExistingTokenNumbers(const KWClass* kwcClass,
									  const ObjectDictionary* odTextClasses) const;

	//////////////////////////////////////////////////////////////////////////////////
	// Construction des variables de type texte dans le dictionnaire principal

	// Creation des blocks de variables de type texte selon les specifications par chemin d'attributs de la classe
	// principale
	void ConstructAllTextAttributeBlocks(KDClassCompliantRules* classCompliantRules, KWClass* kwcClass,
					     ObjectDictionary* odTextClasses,
					     ObjectDictionary* odConstructedAttributes) const;

	// Construction d'un attribut de type texte a partir d'un chemin d'attribut dans un schema multi-table
	// L'attribut construit est insere en unused dans la classe et permet de ramener a la racine l'attribut
	// texte correspondant au chemin d'attribut, avec le type Text ou TextList selon la nature simple ou multiple du
	// chemin
	KWAttribute* ConstructPathAttribute(KDClassCompliantRules* classCompliantRules, KWClass* kwcClass,
					    const KDTextAttributePath* textAttributePath) const;

	// Construction d'un bloc de variables de type ngram pour un attribut de type texte, dans le cas non
	// interpretable Le nombre de variable a construire est nBlockAttributeNUmber. En cas de variables construites
	// existantes, on utilise des VarKey de plus en plus grandes pour cree de nouvelles variables. Le cout de
	// construction est celui de selection ou construction de l'attribut texte lui meme
	void ConstructTextAttributeNgramBlock(KDClassCompliantRules* classCompliantRules, KWClass* kwcClass,
					      const KWAttribute* textAttribute, double dTextConstructionCost,
					      int nBlockAttributeNUmber,
					      ObjectDictionary* odConstructedAttributes) const;

	// Construction d'un bloc de variables de type token pour un attribut de type texte, dans le cas interpretable
	// Le nombre de variable a construire est nBlockAttributeNUmber. En cas de variables construites existantes,
	// on exploite de plus en plus de tokens ex exploitant les tokens en parametres (KDTokenFrequency) en
	// parametres, ordonnes du plus frequent au moins frequent. Le cout de construction est celui de selection ou
	// construction de l'attribut texte lui meme
	void ConstructTextAttributeTokenBlock(KDClassCompliantRules* classCompliantRules, KWClass* kwcClass,
					      const KWAttribute* textAttribute, double dTextConstructionCost,
					      int nBlockAttributeNUmber, const ObjectArray* oaTokens,
					      ObjectDictionary* odConstructedAttributes) const;

	//////////////////////////////////////////////////////////////////////////////
	// Gestion des collisions entre variables construites et attributs derives
	// presents initialement
	//
	// Cette gestion est assuree de facon optimisee par la classe KDClassCompliantRules,
	// en s'inspirant de ce qui est fait dans le cas multi-table dans KDMultiTableFeatureConstruction.
	// Le cas est ici plus simple, puisque toutes les variables sont generees dans la classe principale.
	// On utilise simplement un objet mainClassCompliantRules pour collecter et rechercher des variables
	// ou blocs de variables existants, basee sur les regles de derivation dediees au traitement des Text.
	//
	// Les variables de chemins d'acces aux donnees de type texte (cas multi-table) sont reutiliseees telles
	// quelles. Les blocs sont de deux typse, bases sur des ngrammes ou des tokens. Dans les deux cas, le bloc est
	// derive d'une regle n'ayant qu'un seul operande (le texte a analyser), la dimension du bloc et la liste des
	// variable du bloc etant determinee par les VarKey des variables du bloc. Cela permet de reutiliser des blocs
	// existants (ayant les memes regles de derivation), en ajoutant autant de variables que souhaite auyx variables
	// existant deja. Dans le cas des ngrammes, il suffit d'ajoiter des nouvelle VarKey en incrementant leur index
	// si necessaire. Dans le cas des tokens, il faut avoir extrait plus de tokens que ce qui est specifie pour
	// tenir compte des variables existantes, qui exploitent deja potentiellement une partie des tokens extraits.

	// Initialisation du service de gestion des attributs derives existants
	void InitializeMainClassConstructedAttributesAndBlocks(KDClassCompliantRules* classCompliantRules,
							       const KWClass* kwcClass) const;

	// Creation d'un attribut a partir d'une regle de derivation construite
	// On utilise le service de gestion des attribut derives existant pour les reutiliser si possible
	// La regle de derivation est utilise pour construire l'attribut s'il n'y a pas d'attribut existant equivalent,
	// elle est detruite sinon.
	// Le nom d'attribut n'est utilise que si un nouvel attribut doit etre construit.
	// En sortie, on renvoie true si un nouvel attribut a ete construit, false si on a reutilise un attribut
	// existant
	boolean CreateConstructedDerivationRuleAttribute(KDClassCompliantRules* classCompliantRules, KWClass* kwcClass,
							 KWDerivationRule* constructedDerivationRule,
							 const ALString& sConstructedAttributeName,
							 KWAttribute*& constructedAttribute) const;

	//////////////////////////////////////////////////////////////////////////////////
	// Nommage des variables

	// Nom de variable base sur un index
	ALString BuildIndexedAttributeName() const;

	// Reinitialisation des index utilises pour nnommer les variables
	void ResetAttributeNameIndexes() const;

	// Construction d'un nom d'attribut construit de type texte a partir d'un chemin d'attribut aboutissant a un
	// attribut d'une sous-table
	const ALString BuildMainTextAttributeName(const KDTextAttributePath* textAttributePath) const;

	// Construction d'un nom d'attribut de type ngrammme construit a partir d'un attribut de type texte
	const ALString BuildNGramBasedAttributeName(const KWAttribute* textAttribute, int nGramLength,
						    int nHashTableSize, int nIndex) const;

	// Construction d'un nom de bloc d'attributs de type ngrammme construits a partir d'un attribut de type texte
	const ALString BuildNGramBasedAttributeBlockName(const KWAttribute* textAttribute) const;

	// Construction d'un nom d'attribut de type token construit a partir d'un attribut de type texte
	const ALString BuildTokenBasedAttributeName(const KWAttribute* textAttribute, const ALString& sToken) const;

	// Construction d'un nom de bloc d'attributs de type token construits a partir d'un attribut de type texte
	const ALString BuildTokenBasedAttributeBlockName(const KWAttribute* textAttribute) const;

	// Mode interpretable pour la construction des variables de type texte
	boolean bInterpretableMode;

	// Index du prochain attribut dans le cas d'un nommage indexe, non interpretable
	mutable int nAttributeNameIndex;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KDTextAttributePath
// Chemin d'acces a un attribut de type texte (Text ou TextList) depuis le dictionnaire principal
class KDTextAttributePath : public Object
{
public:
	// Constructeur et destructeur
	KDTextAttributePath();
	~KDTextAttributePath();

	// Reinitialisation
	void Reset();

	////////////////////////////////////////////////
	// Chemin d'acces l'attribut de type texte
	// Le dernier attribut du chemin est celui de type texte
	// Les autres sont des attributs de type Entity ou Table
	// Il ne doit pas y avoir de cycle dans le chemin d'acces
	// permettant d'acceder a l'attribut texte

	// Taille du chemin
	int GetAttributePathSize() const;

	// Ajout d'un attribut en fin d'acces en fin de chemin
	void AddAttributePathEnd(const KWAttribute* attribute);

	// Suppression du dernier attribut du chemin
	void RemoveAttributePathEnd();

	// Test si un chemin contient une classe via un de ses attributs
	boolean ContainsClass(const KWClass* kwcClass) const;

	// Acces a un attribut du chemin
	const KWAttribute* GetAttributePathAt(int nIndex) const;

	// Specification d'un attribut de type texte dans la classe principale, permettant
	// de ramener l'attribut texte de fin de chemin present dans une sous-table
	// dans le cas d'un chemin comportant un acces aux sous-tables
	// Si l'attribut est directement accessible dans la classe principale (chemin de taille 1),
	void SetMainTextAttribute(const KWAttribute* attribute);
	const KWAttribute* GetMainTextAttribute() const;

	//////////////////////////////////////////////////
	// Services d'acces a des informations specifiques
	// pour un chemin valide

	// Acces a l'attribut de type texte, en fin de chemin
	const KWAttribute* GetTextAttribute() const;

	// Acces a la classe principale a l' origine du chemin
	const KWClass* GetMainClass() const;

	// Acces a la classe d'un attribut du chemin
	const KWClass* GetAttributePathClassAt(int nIndex) const;

	//////////////////////////////////////////////////
	// Services pour la construction de variables

	// Cout d'acces a l'attribut texte dans la classe principale pour la regularisation
	void SetCost(double dValue);
	double GetCost() const;

	// Nombre d'attributs a construire
	void SetConstructedFeatureNumber(int nValue);
	int GetConstructedFeatureNumber() const;

	// Nombre de tokens utilises parmi les variables construites existantes
	void SetExistingTokenNumber(int nValue);
	int GetExistingTokenNumber() const;

	// Tableau des tokens avec leur effectif (KDTokenFrequency), a utiliser pour la construction de variable en mode
	// interpretable Memoire: les tokens appartiennent a l'appele
	ObjectArray* GetTokens();

	/////////////////////////////////////////
	// Services divers

	// Verification de l'integrite
	boolean Check() const override;

	// Copie
	void CopyFrom(const KDTextAttributePath* aSource);

	// Duplication
	KDTextAttributePath* Clone() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Chemin d'acces a l'attribut de type texte
	ObjectArray oaAttributePath;

	// Attribut text de la classe principale
	const KWAttribute* mainTextAttribute;

	// Cout d'acces a l'attribut de type texte
	double dCost;

	// Nombre d'attributs a construire
	int nConstructedFeatureNumber;

	// Nommbre de token utilises parmi les variables construites existantes
	int nExistingTokenNumber;

	// Tableau des tokens
	ObjectArray oaTokens;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KDTextClass
// Classe de gestion de la construction des variables de type texte (Text ou TextList)
class KDTextClass : public Object
{
public:
	// Constructeur et destructeur
	KDTextClass();
	~KDTextClass();

	// Reinitialisation
	void Reset();

	// Parametrage de la classe geree
	void SetClass(const KWClass* kwcClass);
	const KWClass* GetClass();

	// Variables de type texte de la classe
	ObjectDictionary* GetTextAttributes();

	// Variables de type Entity ou Table aboutissant a des variables de type texte
	ObjectDictionary* GetRelationAttributes();

	// Chemins d'acces aux attributs de type texte depuis la classe
	// Memoire: les chemins appartienennt a l'appele
	ObjectArray* GetTextAttributePaths();

	// Verification de l'integrite de l'attribut
	boolean Check() const override;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	const KWClass* kwcTextClass;
	ObjectDictionary odTextAttributes;
	ObjectDictionary odRelationAttributes;
	ObjectArray oaTextAttributePaths;
};

// Fonction de comparaison sur le nom d'une KDTextClass
int KDTextClassCompareName(const void* first, const void* second);
