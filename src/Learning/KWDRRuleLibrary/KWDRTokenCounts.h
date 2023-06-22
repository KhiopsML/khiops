// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDRTokenize;
class KWDRTokenCounts;

#include "KWDerivationRule.h"
#include "KWValueBlock.h"

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
// Classe KWDRCharNGramCounts
// ATTENTION: CLASSE TEMPORAIRE, POUR LES TEST DES DONNEES SPARSE
// Compte des n-grammes de lettres projetes sur une table hashage de taille donnee
// Entree:
//   Categorical: texte
//   Numerical: longueurs des n-grames (between 1 and 8)
//   Numerical: taille de la table de hashage
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

	// Calcul des compte par token pour une valeur Symbol
	KWContinuousValueBlock* ComputeCharNgramCounts(const Symbol& sValue,
						       const KWIndexedKeyBlock* indexedKeyBlock) const;

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
	// Cacteristique des n-grammes a extraire
	int nNGramLength;
	int nNGramNumber;

	// Masque a utiliser pour ne garder que la partie utile du n-gramme
	longint lNGramMask;

	// Decallage pesudo-aleatoire a utiliser pour hasher les n-gramme different en fonction
	// des caracteristiques des n-grammes
	longint lRandomOffset;

	// Vecteur de travail des ngrammes utilises
	// Pour chaque index sparse potentiel de n-gramme, ce vecteur contient:
	//  . 0 si c'est le n-gramme partie n'a jamais ete rencontree (etat initial)
	//  . un compte si le n-gramme est a conserver
	// Le vecteur est initialise avec des 0 partout avec une taille qui est celle
	// du bloc effectif (et non la taille max donnee par le nombre de n-grammes a extraire).
	// Les comptes utilises sont memorisees lors du calcul des n-grammes
	// A la fin, on exploite ces compte pour fabriquer le vecteur sparse de comptes en sortie,
	// et on remet a 0 ce qui a ete utilise dans le vecteur de travail
	mutable IntVector ivSparseCounts;
};
