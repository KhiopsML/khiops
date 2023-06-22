// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDRTokenize;
class KWDRTokenCounts;
class KWDRCharNGramCounts;
class KWDRStudyCharNGramCounts;
class KWDRTextInit;

#include "KWDerivationRule.h"
#include "KWValueBlock.h"

// Enregistrement de ces regles
void KWDRRegisterTextAdvancedRules();

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
// Classe KWDRMultipleCharNGramCounts
// ATTENTION: CLASSE TEMPORAIRE, POUR LES TEST DES DONNEES SPARSE
// Compte des n-grammes de lettres projetes sur un bloc de taille donnee
// en combinant une serie de longueurs de n-grammes et de tailles de tables de hashage
// Entree:
//   Categorical: texte
//   Numerical: taille de bloc
// Sortie:
//   NumericalValueBlock: bloc sparse des comptes par n-gramme
// Les cles de variables sont les index dans le bloc
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
// Une etude preliminaire sur plusieurs bases de texte conduit a adopter le schema generique suivant:
//  . 1-grammes : tables de hashage de tailles 2^i, 0 <= i <= 7, entre 1 et 128,
//  . 2-grammes : tables de hashage de tailles 2^i, 8 <= i <= 10, entre 256 et 1024,
//  . 3-grammes : tables de hashage de tailles 2^i, 11 <= i <= 13, entre 2048 et 8192,
//  . 4-grammes : tables de hashage de tailles 2^i, 14 <= i <= 16, entre 16384 et 65536,
//  ...
// Les variables sont toujours generees dans le meme ordre, en partant des longueurs de n-grammes et des tailles de
// tables de hashage; de telles facon que les representation sont emboitees. Les dernieres tables de hashage peuvent ne
// pas etre complete: dans ce cas, les variable sont choisie au hasard (ce qui est fait en prenant les premiers index de
// hashage). Il est a noter que l'exposant i de la taille 2^i de la table de hashage designe de facon unique a la fois
// la longueur des n-grammes et la taille de la table de hashage
class KWDRMultipleCharNGramCounts : public KWDerivationRule
{
public:
	// Constructeur
	KWDRMultipleCharNGramCounts();
	~KWDRMultipleCharNGramCounts();

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

	// Calcul des comptes par token pour une valeur Symbol pour toutes les tables de hashage
	KWContinuousValueBlock* ComputeAllTablesCharNgramCounts(const Symbol& sValue,
								const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Verification de la validite d'un block calcule
	boolean CheckCharNgramCountsValueBlock(const Symbol& sValue, const KWIndexedKeyBlock* indexedKeyBlock,
					       const KWContinuousValueBlock* valueBlock) const;

	// Redefinition de methodes virtuelles
	void Compile(KWClass* kwcOwnerClass) override;
	boolean CheckOperandsCompletness(const KWClass* kwcOwnerClass) const override;
	boolean CheckBlockAttributes(const KWClass* kwcOwnerClass,
				     const KWAttributeBlock* attributeBlock) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Methode de test
	static void Test();

	// Test unitaire du compte de tokens pour une regle correctement parametree
	void TestCount(const ALString& sInputValue, const KWIndexedKeyBlock* indexedKeyBlock);

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Optimisation de la regle
	void Optimize();

	// Initialisation des variable globales
	static void InitializeGlobalVariables();
	static boolean AreGlobalVariablesInitialized();

	// Vecteur de travail des ngrammes utilises
	// Pour chaque index sparse potentiel de n-gramme, ce vecteur contient:
	//  . 0 si c'est le n-gramme n'a jamais ete rencontree (etat initial)
	//  . un compte si le n-gramme est a conserver
	// Le vecteur est initialise avec des 0 partout avec une taille qui est celle
	// du bloc effectif (et non la taille max donnee par le nombre de n-grammes a extraire).
	// Les comptes utilises sont memorisees lors du calcul des n-grammes
	// A la fin, on exploite ces comptes pour fabriquer le vecteur sparse de comptes en sortie,
	// et on remet a 0 ce qui a ete utilise dans le vecteur de travail
	mutable IntVector ivSparseCounts;

	// Nombre de tables de hashage utilisees
	int nHashTableNumber;

	// Longueur max de n-grammes pris en compte
	static const int nMaxNGramLength = 8;

	// Vecteur des longueurs de n-grammes et de taille de table de hashage par index de table de hashage
	static IntVector ivNGramLengths;
	static IntVector ivHashTableSizes;

	// Masques a utiliser pour ne garder que la partie utile du n-gramme, par longueur de n-gramme
	static LongintVector lvNGramMasks;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCharNGramCounts
// ATTENTION: CLASSE TEMPORAIRE, POUR LES TEST DES DONNEES SPARSE
// Compte des n-grammes de lettres projetes sur une table hashage de taille donnee
// Entree:
//   Categorical: texte
//   Numerical: taille de la table de hashage
//   Numerical: longueur des n-grames (entre 1 et 8)
// Sortie:
//   NumericalValueBlock: bloc sparse des comptes par n-gramme
// Les cles de variables sont les index dans la table de hashage
class KWDRCharNGramCounts : public KWDerivationRule
{
public:
	// Constructeur
	KWDRCharNGramCounts();
	~KWDRCharNGramCounts();

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

	// Calcul des comptes par token pour une valeur Symbol
	KWContinuousValueBlock* ComputeCharNgramCounts(const Symbol& sValue,
						       const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Redefinition de methodes virtuelles
	void Compile(KWClass* kwcOwnerClass) override;
	boolean CheckOperandsCompletness(const KWClass* kwcOwnerClass) const override;
	boolean CheckBlockAttributes(const KWClass* kwcOwnerClass,
				     const KWAttributeBlock* attributeBlock) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Caracteristique des n-grammes a extraire
	int nFirstNGramLength;
	int nLastNGramLength;
	int nNGramNumber;
	boolean bWithRandomSign;

	// Decallage pseudo-aleatoire a utiliser pour hasher les n-gramme different en fonction
	// des caracteristiques des n-grammes
	longint lRandomOffset;

	// Vecteur de travail des ngrammes utilises
	// Pour chaque index sparse potentiel de n-gramme, ce vecteur contient:
	//  . 0 si c'est le n-gramme n'a jamais ete rencontree (etat initial)
	//  . un compte si le n-gramme est a conserver
	// Le vecteur est initialise avec des 0 partout avec une taille qui est celle
	// du bloc effectif (et non la taille max donnee par le nombre de n-grammes a extraire).
	// Les comptes utilises sont memorisees lors du calcul des n-grammes
	// A la fin, on exploite ces comptes pour fabriquer le vecteur sparse de comptes en sortie,
	// et on remet a 0 ce qui a ete utilise dans le vecteur de travail
	mutable IntVector ivSparseCounts;

	// Longuer max de n-grammes pris en compte
	static const int nMaxNGramLength = 8;

	// Masques a utiliser pour ne garder que la partie utile du n-gramme, par longueur de n-gramme
	static LongintVector lvNGramMasks;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRStudyCharNGramCounts
// ATTENTION: CLASSE TEMPORAIRE, POUR LES TEST DES DONNEES SPARSE
// Compte des n-grammes de lettres projetes sur une table hashage de taille donnee
// Entree:
//   Categorical: texte
//   Numerical: taille de la table de hashage
//   Numerical: longueurs min des n-grames (entre 1 et 8)
//   Numerical: longueur max des n-gramme
//   Numerical: en utilisant un signe aleatoire pour l'insertion dans la table de hashage
// Sortie:
//   NumericalValueBlock: bloc sparse des comptes par n-gramme
// Les cles de variables sont les index dans la table de hashage
class KWDRStudyCharNGramCounts : public KWDRCharNGramCounts
{
public:
	// Constructeur
	KWDRStudyCharNGramCounts();
	~KWDRStudyCharNGramCounts();

	// Creation
	KWDerivationRule* Create() const override;

	// Redefinition de methodes virtuelles
	void Compile(KWClass* kwcOwnerClass) override;
	boolean CheckOperandsCompletness(const KWClass* kwcOwnerClass) const override;
};

////////////////////////////////////////////////////////////////////////////
// ATTENTION: CLASSE TEMPORAIRE, POUR LES TEST DES DONNEES SPARSE
// Classe KWDRTextInit
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
	boolean CheckOperandsCompletness(const KWClass* kwcOwnerClass) const override;
};
