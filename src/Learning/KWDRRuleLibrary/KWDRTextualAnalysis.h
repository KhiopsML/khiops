// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDRTextTokens;
class KWDRTextListTokens;
class KWDRTextAllNGrams;
class KWDRTextListAllNGrams;
class KWDRTextInit;

#include "KWDerivationRule.h"
#include "KWValueBlock.h"

#include "KWDRTextualAnalysisPROTO.h"

// Enregistrement de ces regles
void KWDRRegisterTextualAnalysisRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextTokens
// Calcul l'effectif de chaque token d'une chaine de caracteres
// Un token est une suite de caracteres, precede et suivi
// de caracteres separateurs blancs
// Les cles de variables en parametres permettent de specifier
// les tokens a compter effectivement
class KWDRTextTokens : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTextTokens();
	~KWDRTextTokens();

	// Type de cle utilisee pour les variables du bloc
	int GetVarKeyType() const override;

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Le resultat appartient a l'appelant
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;
	Continuous GetValueBlockContinuousDefaultValue() const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Methode de test
	static void Test();

	// Test unitaire du compte de tokens pour une regle correctement parametree
	void TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock);

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul des compte par token pour une valeur Symbol ou pour un vecteur de Symbol
	KWContinuousValueBlock* ComputeTokenCountsFromText(const Symbol& sValue,
							   const KWIndexedKeyBlock* indexedKeyBlock) const;
	KWContinuousValueBlock* ComputeTokenCountsFromTextList(const SymbolVector* svValues,
							       const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Tokenisation basique
	// Le texte est tokenise en sequences de caracteres de type espace (ignorees),
	//  ou de tout autre caracteres (gardees)
	KWContinuousValueBlock* BasicComputeTokenCountsFromTextList(const SymbolVector* svValues,
								    const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Tokenisation utilisant les caracteres de pontuation
	// Le texte est tokenise en sequences de caracteres de type espace (ignorees),
	//  de caracteres ponctuation uniquement (gardees),
	//  ou de tout autre caracteres (gardees)
	KWContinuousValueBlock* AdvancedComputeTokenCountsFromTextList(const SymbolVector* svValues,
								       const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Compte des caracteres par type d'une chaine de caracteres
	int GetSpaceCharNumber(const ALString& sValue) const;
	int GetPunctuationCharNumber(const ALString& sValue) const;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListTokens
// Specialisation de la classe KWDRTextTokens, en prenant un TextList en enteee au lieu d'un Text
class KWDRTextListTokens : public KWDRTextTokens
{
public:
	// Constructeur
	KWDRTextListTokens();
	~KWDRTextListTokens();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Le resultat appartient a l'appelant
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextAllNGrams
// Compte des n-grammes de lettres projetes sur un bloc de taille donnee
// en combinant une serie de longueurs de n-grammes et de tailles de tables de hashage
// Entree:
//   Categorical: texte
// Sortie:
//   NumericalValueBlock: bloc sparse des comptes par n-gramme
// Les cles de variables sont les index dans le bloc.
// La taille du bloc est implicite, par analyse des cles utilisees
//
// Les n-grammes de grande longueur ont tendance a etre plus informatif, mais il faut plus d'instances
// pour avoir des effectifs significatifs.
// Les tables de hashage de grande taille tendent a mieux separer les variables de type n-grammes en evitant les
// collision, mais leur penalisation bayesienne est plus importante, ce qui n'est pas adapte pour les basses de petite
// taille. On genere donc une serie de table de hashage de taille variee pour tenir compte de la variete des bases de
// texte.
//
// L'objectif AutoML est d'avoir une representation tres simple, avec un seul parametre: la taille de la representation.
// La represenattion dit egalement etre monotone par rapport a ce parametre: les representation de petites taille
// doivent etre des sous-parties des representations de plus grande taille. On adopte la methode suivante pour la
// generation automatique de variables pour les texte:
//  . generer des variable pour des n-grammes de longueur croissante
//  . genere des tailles de hashage de taille croissante, pour minimiser le risque de collisions.
//
// Une etude preliminaire sur plusieurs bases de textes conduit a adopter le schema generique suivant:
//  . 1-grammes : tables de hashage de tailles 2^i, 0, 4 <= i <= 7, 1, puis entre 16 et 128,
//  . 2-grammes : tables de hashage de tailles 2^i, 8 <= i <= 10, entre 256 et 1024,
//  . 3-grammes : tables de hashage de tailles 2^i, 11 <= i <= 13, entre 2048 et 8192,
//  . 4-grammes : tables de hashage de tailles 2^i, 14 <= i <= 15, entre 16384 et 32768,
//  . k-grammes, k >= 4: comme pour les 4-grammes, avec deux tables de hasshages de tailles 13384 et 35768.
// Les variables sont toujours generees dans le meme ordre, en partant des longueurs de n-grammes et des tailles de
// tables de hashage; de telles facon que les representation sont emboitees. Les dernieres tables de hashage peuvent ne
// pas etre complete: dans ce cas, les variable sont choisie au hasard (ce qui est fait en prenant les premiers index de
// hashage). Il est a noter que l'exposant i de la taille 2^i de la table de hashage designe de facon unique a la fois
// la longueur des n-grammes et la taille de la table de hashage
class KWDRTextAllNGrams : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTextAllNGrams();
	~KWDRTextAllNGrams();

	// Type de cle utilisee pour les variables du bloc
	int GetVarKeyType() const override;

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Le resultat appartient a l'appelant
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;
	Continuous GetValueBlockContinuousDefaultValue() const override;

	// Redefinition de methodes virtuelles
	boolean CheckBlockAttributes(const KWClass* kwcOwnerClass,
				     const KWAttributeBlock* attributeBlock) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////////////
	// Information sur la structure des tables de hashages utilisees

	// Nombre total de tables de hashages successives potentielles
	static int GetMaxHashTableNumber();

	// Longueur de n-gramme gere par table de hashage
	static int GetHashTableNGramLengthAt(int nIndex);

	// Taille par table de hashage
	static int GetHashTableSizeAt(int nIndex);

	// Valeur max des VarKey utilisables
	static int GetMaxVarKey();

	///////////////////////////////////////////////////////
	// Methodes de test

	// Methode de test
	static void Test();

	// Test unitaire du compte de tokens pour une regle correctement parametree
	void TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock);

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Compilation de la regle, a appeler en debut de l'implementation du calcul de l'attribut derive
	// Permet de parametrer correctement le max des VarKey a utiliser pour
	void DynamicCompile(const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Initialisation des variable globales
	static void InitializeGlobalVariables();
	static boolean AreGlobalVariablesInitialized();

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Calcul des compte de token pour toutes les tables de hashage
	// Les methodes sont codees pour le type type TextList (SymbolVector) et reutilises
	// dans le cas du type Text (Symbol) via des SymbolVector ayant une seule valeur

	// Calcul des comptes par token pour une valeur Symbol ou pour un vecteur de Symbol, pour toutes les tables de
	// hashage
	KWContinuousValueBlock* ComputeAllTablesCharNgramCountsFromText(const Symbol& sValue,
									const KWIndexedKeyBlock* indexedKeyBlock) const;
	KWContinuousValueBlock*
	ComputeAllTablesCharNgramCountsFromTextList(const SymbolVector* svValues,
						    const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Verification de la validite d'un block calcule
	boolean CheckCharNgramCountsValueBlockFromTextList(const SymbolVector* svValues,
							   const KWIndexedKeyBlock* indexedKeyBlock,
							   const KWContinuousValueBlock* valueBlock) const;

	// Vecteur de travail des ngrammes utilises
	// Pour chaque index sparse potentiel de n-gramme, ce vecteur contient:
	//  . 0 si le n-gramme n'a jamais ete rencontree (etat initial)
	//  . un compte si le n-gramme est a conserver
	// Le vecteur est initialise avec des 0 partout avec une taille qui est celle
	// du bloc effectif (et non la taille max donnee par le nombre de n-grammes a extraire).
	// Les comptes utilises sont memorisees lors du calcul des n-grammes
	// A la fin, on exploite ces comptes pour fabriquer le vecteur sparse de comptes en sortie,
	// et on remet a 0 ce qui a ete utilise dans le vecteur de travail
	mutable LongintVector lvSparseCounts;

	// Index max des VarKey utilisees
	mutable int nMaxVarKeyIndex;

	// Nombre de tables de hashage utilisees
	mutable int nUsedHashTableNumber;

	// Fraicheur de compilation dynamique
	mutable int nDynamicCompileFreshness;

	// Longueur max de n-grammes pris en compte
	static const int nMaxNGramLength = 8;

	// Vecteur des longueurs de n-grammes et de taille de table de hashage par index de table de hashage
	static IntVector ivNGramLengths;
	static IntVector ivHashTableSizes;

	// Taille cumulee des tables de hashage, en commencant a 0
	static IntVector ivHashTableCumulatedSizes;

	// Masques a utiliser pour ne garder que la partie utile du n-gramme, par longueur de n-gramme
	static LongintVector lvNGramMasks;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListAllNGrams
// Specialisation de la classe KWDRTextAllNGrams, en prenant un TextList en enteee au lieu d'un Text
class KWDRTextListAllNGrams : public KWDRTextAllNGrams
{
public:
	// Constructeur
	KWDRTextListAllNGrams();
	~KWDRTextListAllNGrams();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Le resultat appartient a l'appelant
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextInit
// Creation d'une chaine de caracteres
//   . premier operande: taille en nombre de caracteres
//   . deuxieme operande: caractere, repete pour obtenir la taille souhaitee
class KWDRTextInit : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTextInit();
	~KWDRTextInit();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeTextResult(const KWObject* kwoObject) const override;

	// Redefinition de methodes virtuelles
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;
};
