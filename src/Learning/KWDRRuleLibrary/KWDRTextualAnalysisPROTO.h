// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDRTextAllNGrams;
class KWDRTextListAllNGrams;
class KWDRTokenize;
class KWDRTokenCounts;
class KWDRNGramCounts;
class KWDRStudyCharNGramCounts;

#include "KWDerivationRule.h"
#include "KWValueBlock.h"
#include "KWDRVector.h"

// Enregistrement de ces regles
void KWDRRegisterTextualAnalysisPROTORules();

////////////////////////////////////////////////////////////////////////////
// ATTENTION: CLASSE TEMPORAIRE// Classe KWDRTextAllNGrams
// Compte des ngrams de lettres projetes sur un bloc de taille donnee
// en combinant une serie de longueurs de ngrams et de tailles de tables de hashage
// Entree:
//   Categorical: texte
// Sortie:
//   NumericalValueBlock: bloc sparse des comptes par ngram
// Les cles de variables sont les index dans le bloc.
// La taille du bloc est implicite, par analyse des cles utilisees
//
// Les ngrams de grande longueur ont tendance a etre plus informatif, mais il faut plus d'instances
// pour avoir des effectifs significatifs.
// Les tables de hashage de grande taille tendent a mieux separer les variables de type ngrams en evitant les collision,
// mais leur penalisation bayesienne est plus importante, ce qui n'est pas adapte pour les basses de petite taille.
// On genere donc une serie de table de hashage de taille variee pour tenir compte de la variete des bases de texte.
//
// L'objectif AutoML est d'avoir une representation tres simple, avec un seul parametre: la taille de la representation.
// La represenattion dit egalement etre monotone par rapport a ce parametre: les representation de petites taille
// doivent etre des sous-parties des representations de plus grande taille. On adopte la methode suivante pour la
// generation automatique de variables pour les texte:
//  . generer des variable pour des ngrams de longueur croissante
//  . genere des tailles de hashage de taille croissante, pour minimiser le risque de collisions.
//
// Une etude preliminaire sur plusieurs bases de textes conduit a adopter le schema generique suivant:
//  . 1-grammes : tables de hashage de tailles 2^i, 0, 4 <= i <= 7, 1 (longueur de texte) puis entre 16 et 128,
//  . 2-grammes : tables de hashage de tailles 2^i, 8 <= i <= 10, entre 256 et 1024,
//  . 3-grammes : tables de hashage de tailles 2^i, 11 <= i <= 13, entre 2048 et 8192,
//  . 4-grammes : tables de hashage de tailles 2^i, 14 <= i <= 15, entre 16384 et 32768,
//  . k-grammes, k >= 4: comme pour les 4-grammes, avec deux tables de hasshages de tailles 16384 et 32768.
// Les variables sont toujours generees dans le meme ordre, en partant des longueurs de ngrams et des tailles de
// tables de hashage; de telles facon que les representation sont emboitees. Les dernieres tables de hashage peuvent ne
// pas etre complete: dans ce cas, les variables sont choisies au hasard (ce qui est fait en prenant les premiers index
// de hashage). Il est a noter que l'exposant i de la taille 2^i de la table de hashage designe de facon unique a la
// fois la longueur des ngrams et la taille de la table de hashage
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

	// Longueur de ngram gere par table de hashage
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

	// Vecteur de travail des ngrams utilises
	// Pour chaque index sparse potentiel de ngram, ce vecteur contient:
	//  . 0 si le ngram n'a jamais ete rencontree (etat initial)
	//  . un compte si le ngram est a conserver
	// Le vecteur est initialise avec des 0 partout avec une taille qui est celle
	// du bloc effectif (et non la taille max donnee par le nombre de ngrams a extraire).
	// Les comptes utilises sont memorisees lors du calcul des ngrams
	// A la fin, on exploite ces comptes pour fabriquer le vecteur sparse de comptes en sortie,
	// et on remet a 0 ce qui a ete utilise dans le vecteur de travail
	mutable LongintVector lvSparseCounts;

	// Index max des VarKey utilisees
	mutable int nMaxVarKeyIndex;

	// Nombre de tables de hashage utilisees
	mutable int nUsedHashTableNumber;

	// Fraicheur de compilation dynamique
	mutable int nDynamicCompileFreshness;

	// Longueur max de ngrams pris en compte
	static const int nMaxNGramLength = 8;

	// Vecteur des longueurs de ngrams et de taille de table de hashage par index de table de hashage
	static IntVector ivNGramLengths;
	static IntVector ivHashTableSizes;

	// Taille cumulee des tables de hashage, en commencant a 0
	static IntVector ivHashTableCumulatedSizes;

	// Masques a utiliser pour ne garder que la partie utile du ngram, par longueur de ngram
	static LongintVector lvNGramMasks;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListAllNGrams
// ATTENTION: CLASSE TEMPORAIRE
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
// Classe KWDRTokenize
// ATTENTION: CLASSE TEMPORAIRE EN LIEN AVEC KWDRTOKENCOUNTS
// Recodage d'une d'une valeur categorielle en une serie de valeurs separes par des caracteres blancs.
class KWDRTokenize : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTokenize();
	~KWDRTokenize();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTokenCounts
// ATTENTION: CLASSE TEMPORAIRE, POUR LES TEST DES DONNEES SPARSE
// Calcul l'effectif de chaque token d'une chaine de caracteres
// Un token est une suite de caracteres, precede et suivi
// de caracteres separateurs
// Les cles de variables en parametres permettent de specifier
// les tokens a compter effectivement
class KWDRTokenCounts : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTokenCounts();
	~KWDRTokenCounts();

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

	// Calcul des compte par token pour une valeur Symbol
	KWContinuousValueBlock* ComputeTokenCounts(const Symbol& sValue,
						   const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Methode de test
	static void Test();

	// Test unitaire du compte de tokens pour une regle correctement parametree
	void TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock);
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRNGramCounts
// ATTENTION: CLASSE TEMPORAIRE, POUR LES TESTS DES DONNEES SPARSE
// Compte des ngrams de lettres projetes sur une table hashage de taille donnee
// Entree:
//   Categorical: texte
//   Numerical: taille de la table de hashage
//   Numerical: longueur des ngrams (entre 1 et 8)
// Sortie:
//   NumericalValueBlock: bloc sparse des comptes par ngram
// Les cles de variables sont les index dans la table de hashage
class KWDRNGramCounts : public KWDerivationRule
{
public:
	// Constructeur
	KWDRNGramCounts();
	~KWDRNGramCounts();

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
	void Compile(KWClass* kwcOwnerClass) override;
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;
	boolean CheckBlockAttributes(const KWClass* kwcOwnerClass,
				     const KWAttributeBlock* attributeBlock) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul des comptes par token pour une valeur Symbol
	KWContinuousValueBlock* ComputeCharNgramCounts(const Symbol& sValue,
						       const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Caracteristique des ngrams a extraire
	int nFirstNGramLength;
	int nLastNGramLength;
	int nNGramNumber;
	boolean bWithRandomSign;

	// Decallage pseudo-aleatoire a utiliser pour hasher les ngram different en fonction
	// des caracteristiques des ngrams
	longint lRandomOffset;

	// Vecteur de travail des ngrams utilises
	// Pour chaque index sparse potentiel de ngram, ce vecteur contient:
	//  . 0 si c'est le ngram n'a jamais ete rencontree (etat initial)
	//  . un compte si le ngram est a conserver
	// Le vecteur est initialise avec des 0 partout avec une taille qui est celle
	// du bloc effectif (et non la taille max donnee par le nombre de ngrams a extraire).
	// Les comptes utilises sont memorisees lors du calcul des ngrams
	// A la fin, on exploite ces comptes pour fabriquer le vecteur sparse de comptes en sortie,
	// et on remet a 0 ce qui a ete utilise dans le vecteur de travail
	mutable LongintVector lvSparseCounts;

	// Longuer max de ngrams pris en compte
	static const int nMaxNGramLength = 8;

	// Masques a utiliser pour ne garder que la partie utile du ngram, par longueur de ngram
	static LongintVector lvNGramMasks;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRStudyNGramCounts
// ATTENTION: CLASSE TEMPORAIRE, POUR LES TESTS DES DONNEES SPARSE
// Compte des ngrams de lettres projetes sur une table hashage de taille donnee
// Entree:
//   Categorical: texte
//   Numerical: taille de la table de hashage
//   Numerical: longueurs min des ngrams (entre 1 et 8)
//   Numerical: longueur max des ngrams
//   Numerical: en utilisant un signe aleatoire pour l'insertion dans la table de hashage
// Sortie:
//   NumericalValueBlock: bloc sparse des comptes par ngram
// Les cles de variables sont les index dans la table de hashage
class KWDRStudyNGramCounts : public KWDRNGramCounts
{
public:
	// Constructeur
	KWDRStudyNGramCounts();
	~KWDRStudyNGramCounts();

	// Creation
	KWDerivationRule* Create() const override;

	// Redefinition de methodes virtuelles
	void Compile(KWClass* kwcOwnerClass) override;
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRExtractWords
// Regle de derivation de type Structure(VectorC), basee sur l'extraction
// de mots a partir d'une chaine de caracteres.
// Les caracteres sont d'abord pretraites et transformes selon les parametres
// d'extraction, tous les autres caracteres etant transformes en caracteres blancs.
// Les mots sont alors extraits de la chaine de catacteres pretraites avec separateur blanc.
// Les parametres d'extraction sont les suivants
//      - SourceString: chaine de caracteres initiale
//		- ToLower: mise en minuscule
//      - KeepNumerical: on garde les caracteres numeriques
//      - AdditionalChars: caracteres supplementaires a garder
//      - TranslatedAdditionalChars: version recodee des caracteres additionnels
//      - MaxLength: longueur max des mots, au dela de laquels les mots sont tronques
class KWDRExtractWords : public KWDerivationRule
{
public:
	// Constructeur
	KWDRExtractWords();
	~KWDRExtractWords();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Verification des parametres d'extraction
	boolean CheckOperandsDefinition() const override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRSymbolVector symbolVector;

	/////////////////////////////////////////////////////////////////////////////////
	// Resultats de l'optimisation des parametres d'extraction

	// Vecteur des caracteres recodes suite a application des parametres d'extraction
	// Chaque caracteres (entre 0 et 255) est ainsi garde tel quel, mise en minuscule,
	// recode ou transforme en blanc
	ALString sTranslatedChars;

	// Longuer max des mots
	int nMaxLength;
};