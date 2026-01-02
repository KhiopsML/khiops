// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWTextTokenizer;
class KWTextNgramTokenizer;
class KWTextWordTokenizer;
class KWTextBasicTokenizer;

#include "TextService.h"
#include "Object.h"
#include "ALString.h"
#include "Vector.h"
#include "CharVector.h"
#include "Timer.h"
#include "KWTokenFrequency.h"
#include "KWSymbol.h"
#include "KWIndexedKeyBlock.h"
#include "KWValueBlock.h"

//////////////////////////////////////////
// Classe de tokenization d'un texte
// Classe acnetre de toutes les classes de tokenization
// Dans cette classe, la tokenization est basique, en utilisant uniquement le separateur blanc
class KWTextTokenizer : public TextService
{
public:
	KWTextTokenizer();
	~KWTextTokenizer();

	// Reinitialisation complete
	void Reset();

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Parametres de tokenisation
	// A specifier avant le debut de la tokenization
	//
	// La tokenization peut se faire selon deux mode:
	//  . mode initial de collecte de tous les tokens d'un corpus de texte
	//     . on peut alors exploiter le parametre MaxCollectedTokenNumber pour avoir une version en flux
	//       de la collecte des tokens, qui economise de la memoire, mais rend des compte de tokens avec une
	//       incertitude
	//  . mode de calcul des effectifs uniquement pour un jeu de tokens specifiques
	//     . ce deuxieme mode est notamment utile pour calculer les effectifs exacts par tokens dans
	//       le cas ou le premier mode de tokenisation a ete prelablement utilise

	// Nombre max de token a garder durant la collecte (defaut: 0, signifie pas de limite)
	// Si cette contrainte est active, les structures internes de collecte des tokens sont regulierement nettoyees
	// pour ne garder que les tokens les plus frequents
	// Cette methode est utile notamment pour le traitement des grands corpus de texte, est permet de garder les
	// tokens effectivement les plus frequent, aux prix d'une erreur sur leur effectif
	void SetMaxCollectedTokenNumber(int nValue);
	int GetMaxCollectedTokenNumber() const;

	// Specification des tokens specifiques collecter via un tableau de KWTokenFrequency (defaut: aucun, signifie
	// que l'on doit tout garder) En cas de tokens specifiques, la tokenisation ne calcule que les effectifs des ces
	// tokens, en ignorant tous les autres
	//
	// On recopie en interne uniquement les valeurs des tokens du tableau, en ignorant leur effectif
	// Si le tableau en entree est NULL, cela revient reinitialiser a vide ce parametre
	// Memoire: les tokens en parametre appartiennent a l'appelant et ne sont pas gared par l'appele
	virtual void SetSpecificTokens(const ObjectArray* oaTokens);

	// Nombre de token a collecter (defaut 0: signifie pas de limite)
	int GetSpecificTokenNumber() const;

	// Verification de la coherence des parametres, qui sont exclusifs
	boolean CheckParameters() const;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Services de collecte des tokens par analyse d'un corpus de textes
	// Ces methodes collectent tous les tokens d'une chaine de caracteres et mettent a jour leur effectif dans une
	// structure interne Ces methodes peuvent etre appelees plusieurs fois successivement pour cumuler les effectifs
	// sur un ensemble de chaines de caracteres

	// Methodes de tokenization a partir de chaines de caracteres ou de symbol
	void TokenizeString(const ALString& sValue);
	void TokenizeSymbol(const Symbol& sValue);
	void TokenizeStringVector(const StringVector* svValues);
	void TokenizeSymbolVector(const SymbolVector* svValues);

	// Prise en compte des effectifs d'un tableau de tokens pour mettre a jour les effectifs globaux des tokens
	// collectes
	virtual void CumulateTokenFrequencies(const ObjectArray* oaTokens);

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Exploitation des tokens collectes
	// Ces methodes peuvent etre appelee a tout moment de la collecte

	// Nombre de tokens collectes
	int GetCollectedTokenNumber() const;

	// Export de l'ensemble des tokens extraits (KWTokenFrequency)
	// Les tokens sont extraits effectifs decroissants, puis longueur de token croissante, puis token croissant,
	// Memoire: les tokens du tableau en sortie appartiennent a l'appelant
	void ExportTokens(ObjectArray* oaTokenFrequencies) const;

	// Variante de la methode en se limitant aux tokens les plus frequents
	virtual void ExportFrequentTokens(ObjectArray* oaTokenFrequencies, int nMaxTokenNumber) const;

	// Nettoyage des tokens collectes
	void CleanCollectedTokens();

	// Affichage des tokens collectes, quelque soit leur nature, au format imprimable
	void DisplayTokens(ostream& ost) const;
	void DisplayFrequentTokens(int nMaxTokenNumber, ostream& ost) const;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Services de calcul des effectifs par tokens
	// Permet de le deploiement de la tokenisation via des regles de derivation

	// Parametrage des tokens a garder en deploiement
	// On peut passer NULL en parametre pour quiter le mode deploiement
	// Memoire: appartient a l'appelant
	virtual void SetDeploymentTokens(const StringVector* svDeploymentTokens);

	// Indicateur du mode de deploiement
	boolean IsDeploymentMode() const;

	// Methodes de calcul des effectifs par token a partir de chaines de caracteres ou de symbol
	// Ces methodes ne peuvent etre appelees qu'une seule fois pour calculer le dictionnaire (Word, Frequency) en
	// sortie
	KWContinuousValueBlock* BuildBlockFromString(const ALString& sValue);
	KWContinuousValueBlock* BuildBlockFromSymbol(const Symbol& sValue);
	KWContinuousValueBlock* BuildBlockFromStringVector(const StringVector* svValues);
	KWContinuousValueBlock* BuildBlockFromSymbolVector(const SymbolVector* svValues);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Service de gestion de tableau de tokens

	// Tri des tokens par effectifs decroissants, puis longueur de token croissante, puis token croissant
	static void SortTokenArray(ObjectArray* oaTokens);

	// Filtrage de tokens surnumeraires par rapport a un max demande, en detruisant les tokens de fin du tableau
	static void FilterTokenArray(ObjectArray* oaTokens, int nMaxTokenNumber);

	// Affichage d'un tableau de tokens, au format imprimable
	static void DisplayTokenArray(const ObjectArray* oaTokens, ostream& ost);
	static void DisplayHeadTokenArray(const ObjectArray* oaTokens, int nMaxTokenNumber, ostream& ost);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Creation generique d'un tokenizer

	// Test de validite d'un type de tokenisation: ngrams, words, tokens
	static boolean CheckTextFeatures(const ALString& sValue);

	// Creation generique d'un tokenizer pour un type de tokenization
	// Memoire: la regle en retour appartient a l'appelant
	static KWTextTokenizer* CreateTextTokenizer(const ALString& sTextFeatures);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Services divers

	// Compte des caracteres par type de token
	static int GetSpaceCharNumber(const ALString& sValue);
	static int GetPunctuationCharNumber(const ALString& sValue);

	// Taille max des mots prix en compte
	static int GetMaxWordLength();

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Test de la classe
	static void Test();

	//////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////////////////
	// Algorithmes de tokenisation
	// Ces algorithmes constituent une section critique du code, et un soin particulier a ete
	// apporter pour optimiser a la fois la complexite algorithme et les constantes par algorithmes
	// via le choix de structure algorithmique parcimonieuses
	// Les algorithme et structure algorithme ci-dessous ont ete evaluees et on t demontre des gains
	// tres significatifs par rapport a une implementation plus basique
	//
	// Notations pour l'apprentissage de la tokenisation
	//  Nc: taille globale d'un corpus de texte en caracteres
	//  Nct: nombre total de tokens presents dans le corpus de textes
	//  Mt: nombre max de tokens a extraire
	//  Nst: nombre de tokens specifiques (en principe un sous-ensemble des tokens du corpus)
	// Notations pour le deploiement de la tokenisation
	//  Nst: nombre de tokens specifiques a exploiter (potentiellement plus que ceux present dans un texte)
	//  nc: taille d'un texte a deployer
	//  nct: nombre total de tokens presents dans le texte
	//  nst: nombre de tokens specifiques effectivement presents dans le texte
	//
	// Note:
	//  . Nct = O(Nc)
	//     . cas des tokens de type words, avec des mots de taille moyenne 5: Nct ~ Nc/5
	//     . cas des tokens de type ngrams jusqu'a une longueur 8: Nct ~ Nc * 8
	//     . il y a donc beaucoup plus de tokens de type ngrams, et leur gestion est particulierement optimisee
	//  . idem avec nct = O(nc)
	//
	// Structures algorithmiques utilisees pour l'apprentissage de la tokenisation
	//  . LongintDictionary: cas des tokens de type chaine de caracteres
	//     . cle = token, valeur = effectif
	//     . plus performant qu'un ObjectifDictionary general, qui doit passer par de objets intermediaires
	//       pour stocker les effectifs
	//  . LongintNumericKeyDictionary: cas des tokens de type ngrams, stockes de facon optimises dans les longint
	//  . KWTokenFrequency: paire (token, valeur)
	//     . pour la finalisation de la collecte avec le tri par effectif decroissant
	//     . pour l'export des resultats
	//
	// Algorithme d'apprentissage, premiere passe sans tokens specifiques: O(Nct + Mt log Mt)
	//  . parsing des textes pour extraire tous les tokens: O(Nct)
	//  . gestion des tokens en stream selon l'algorithme de Karp: O(1) par tokens, O(Nct) globalement
	//     . via les dictionnaires, dont la taille max Mt est mise a jour effeccament selon l'algorithme de Karp
	//  . collecte des resultats tries: O(Mt log Mt)
	//     . via un tableau de KWTokenFrequency extrait en O(Mt) a partir du dictionnaire, puis trie
	//
	// Algorithme d'apprentissage, deuxieme passe de calcul des effectifs de tokens specifiques:  O(Nct + Nst log
	// Nst)
	//  . stockage dans un dictionnaire des index de tokens specifique, de 1 a Nst: O(Nst)
	//  . initialisation d'un vecteur d'effectif a vide de taille Nst: O(Nst)
	//  . initialisation d'un vecteur de presence des index de tokens effectivement utilises
	//  . parsing des textes pour extraire tous les tokens: O(Nct)
	//  . incrementation de l'effectif des tokens specifiques: O(1) par token, O(Nct) globalement
	//     . recherche de son index dans le dictionnaire d'index de tokens
	//     . mise a jour de son effectif via le vecteur d'effectif
	//     . ajout d'un element dans vecteur de presence si necessaire (si effectif vide initialement)
	//  . collecte des resultats tries: O(Nst log Nst)
	//     . via un tableau de KWTokenFrequency extrait en O(Nst) a partir du dictionnaire, puis trie
	//     . comme tous le corpus est analyse, le vecteur d'effectif est plein
	//
	// Algorithme de deploiement, initialisation des tokens a collecter:  O(Nst)
	//  . stockage dans un dictionnaire des index de tokens specifique, de 1 a Nst: O(Nst)
	//  . initialisation d'un vecteur d'effectif a vide de taille Nst: O(Nst)
	//  . initialisation d'un vecteur de presence des index de tokens effectivement utilises
	// Algorithme de deploiement, calcul d'un bloc sparse pour un texte a deployer:  O(nct + nst log nst)
	//  . parsing des textes pour extraire tous les tokens: O(nct)
	//  . incrementation de l'effectif des tokens specifiques: O(1) par token, O(Nct) globalement
	//     . recherche de son index dans le dictionnaire d'index de tokens
	//     . mise a jour de son effectif via le vecteur d'effectif
	//     . ajout d'un element dans vecteur de presence si necessaire (si effectif vide initialement)
	//  . collecte des resultats tries: O(nst log nst)
	//     . exploitation du vecteur de presence pour ne rechercher que les tokens ayant des effectifs non nuls
	//     . exploitation des structures algorithmiques dediee a la gestion des bloc sparse, pour
	//       indexer directement les tokens a colelcter
	//     . tri des pair (index sparse de token, effectif) pour fabriquer le bloc sparse
	//     . attention, il faut remettre a comme tous le corpus est analyse, le vecteur d'effectifs
	//        . utilisation du vecteur de presence des tokens pour reinitialiser les effectifs non nul: O(nst)
	//          (si on remettait a zero tout le vecteur d'efefctif, cela couterait O(Nst) par deploiement)
	//        . remise a vide du vecteur de presence

	// Methode de tokenization par defaut, a reimplementer
	// Dans la methode par defaut, le caractere blanc est utilise comme unbique separateur de tokens
	virtual void TokenizeText(const char* sText, int nTextLength);

	// Mise a jour de l'effectif d'un token
	void UpgradeTokenFrequency(const ALString& sToken, longint lFrequency);

	// Nettoyage d'un dictionnaire trop gros en mode flux
	// Si nMaxTokenNumber vaut 0, il n'y a pas de limite et pas de nettoyage
	//
	// On utilise le principe de l'algorithme de
	//  R. Karp, S. Shenker, and C. Papadimitriou. A Simple Algorithm for Finding Frequent Elements
	//  in Streams and Bags. ACM Transactions on Database Systems, 28(1):51--55, 2003.
	// Cet algorithme garantie que l'on trouvera en O(1) par token toutes les valeurs sufisament frequentes,
	// avec eventuellement des faux positifs
	// En en demandant 10 fois plus, les plus frequents sont probablement retrouves de facon plus stable
	//
	// Note pour le passage en parallelle: les premiers essais montrent que l'algorithme permet effectivement
	// de trouver garder les tokens les plus frequents, mais meme un facteur 10 n'est pas suffisant pour assurer
	// que les effectifs de tokens les plus frequents restent dans le bon ordre. Il faut donc s'attendre a une
	// variation si l'on des resultats si on passe en parallele.
	// Il est necessaire d'effectuer une deuxieme passe pour compter de facon exacte les tokens selectionnes
	// dans la premiere passse, mais cela augmente le temps de calcul.
	// Autre possibilite: imaginer une variante de l'algorithme, approximatif, mais dont le resultat ne dependrait
	// pas du nombre de process paralleles utilises. Cela ne semble pas faisable a priori.
	void StreamCleanTokenDictionary(GenericDictionary* gdTokenDictionary, int nMaxTokenNumber);

	// Calcul des stats par effectif de token pour un dictionnaire generique
	// Chaque index du vecteur de stats en sortie correspond a un effectif
	void ComputeTokenDictionaryFrequencyStats(const GenericDictionary* gdTokenDictionary, int nMaxFrequency,
						  IntVector* ivNumbersOfTokensPerFrequency) const;

	// Construction d'un bloc sparse a partir des resultats de deploiement de la tokenisation
	KWContinuousValueBlock* BuildBlockFromDeployedTokens();

	// Nettoyage des resultats de deploiement
	void CleanDeployedTokens();

	// Calcul de l'effectif total cumule des tokens
	longint ComputeTotalTokenFrequency() const;

	// Dictionnaire generique de longint definissant les tokens specifiques a collecter avec
	// une paire (token, index) par token a collecter
	// Les index commencent a 1 pour se distinguer de la valeur 0, qui signifie absent du dictionnaire
	// Il servent a indexer directement une effectif dans le vecteur des effectifs par token specifique
	GenericDictionary* gdSpecificTokens;

	// Vecteur des effectifs par token specifique
	// Ce vecteur sert egalement d'indicateur d'utilisation par token specifique, en testant si l'effectif est non
	// nul
	LongintVector lvSpecificTokenFrequencies;

	// Memorisation des index des tokens specifiques effectivement utilises
	IntVector ivUsedSpecificTokenIndexes;

	// Nombre max de tokens a collecter
	int nMaxCollectedTokenNumber;

	// Dictionnaire generique de longint pour gerer efficacement les paires (token, frequency)
	GenericDictionary* gdCollectedTokens;

	// Indicateur du mode deploiement
	boolean bIsDeploymentMode;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Tokenisation d'un texte en ngrams
//   ngrams: sequence de bytes quelconques, se chevauchant
//    - completement automatique et generique, meme si les "textes" sont des blobs non textuels
//    - tokenisation optimisee en memoire et temps de calcul en utilisant des longint pour stocker les ngrams
class KWTextNgramTokenizer : public KWTextTokenizer
{
public:
	// Constructeur
	KWTextNgramTokenizer();
	~KWTextNgramTokenizer();

	// Redefinition des methodes virtuelles
	void SetSpecificTokens(const ObjectArray* oaTokens) override;
	void ExportFrequentTokens(ObjectArray* oaTokenFrequencies, int nMaxTokenNumber) const override;
	void CumulateTokenFrequencies(const ObjectArray* oaTokens) override;
	void SetDeploymentTokens(const StringVector* svDeploymentTokens) override;

	// Taille max des ngrams prise en compte
	static int GetMaxNgramLength();

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;

	// Test de la classe
	static void Test();

	///////////////////////
	///// Implementation
protected:
	// Reimplementation de la methode de tokenisation
	void TokenizeText(const char* sText, int nTextLength) override;

	// Mise a jour de l'effectif d'un token de type ngrams
	void UpgradeNgramTokenFrequency(longint lEncodedNgram, longint lFrequency);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Gestion optimisee des ngrams
	// En convertissant des ngrams de bytes en longint, on peut obtenir des optimisations
	// importantes a la fois en memoire (pas besoin d'llouer une chaie de caracteres) et
	// en temps (la valeur du longint fournit directement une cle pour une table de hashage)

	// Conversion d'une chaine de caracteres en longint
	static longint NgramToLongint(const ALString& sNgramBytes);

	// Conversion d'une sous-chaine de caracteres en longint
	static longint SubNgramToLongint(const ALString& sNgramBytes, int nStart, int nGramLength);

	// Conversion de longint en chaine de caracteres
	static const ALString LongintToNgram(longint lValue);
};

/////////////////////////////////////////////////////////////////////////////////////////
// Tokenisation d'un texte en words
//   words: mots tokenises selon des pretraitements basiques
//    - sequence de ponctuation ou de caracteres hors separateurs ou ponctuation de la plage ascii
//    - les separateurs sont ceux de la plage ascii ainsi que les caracteres de controle de la base ascii
//    - pretraitement linguistique basique par defaut, entierement automatique
class KWTextWordTokenizer : public KWTextTokenizer
{
public:
	// Constructeur
	KWTextWordTokenizer();
	~KWTextWordTokenizer();

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;

	// Test de la classe
	static void Test();

	///////////////////////
	///// Implementation
protected:
	// Reimplementation de la methode de tokenisation
	void TokenizeText(const char* sText, int nTextLength) override;
};

//////////////////////////////////////
// Methodes en inline

inline void KWTextTokenizer::SetMaxCollectedTokenNumber(int nValue)
{
	require(nValue >= 0);
	require(GetCollectedTokenNumber() == 0);

	nMaxCollectedTokenNumber = nValue;
}

inline int KWTextTokenizer::GetMaxCollectedTokenNumber() const
{
	return nMaxCollectedTokenNumber;
}

inline int KWTextTokenizer::GetCollectedTokenNumber() const
{
	require(CheckParameters());
	if (gdSpecificTokens == NULL)
		return gdCollectedTokens->GetCount();
	else
		return ivUsedSpecificTokenIndexes.GetSize();
}

inline void KWTextTokenizer::TokenizeString(const ALString& sValue)
{
	require(not IsDeploymentMode());
	require(CheckParameters());

	TokenizeText(sValue, sValue.GetLength());
	StreamCleanTokenDictionary(gdCollectedTokens, GetMaxCollectedTokenNumber());
}

inline void KWTextTokenizer::TokenizeSymbol(const Symbol& sValue)
{
	require(not IsDeploymentMode());
	require(CheckParameters());

	TokenizeText(sValue, sValue.GetLength());
	StreamCleanTokenDictionary(gdCollectedTokens, GetMaxCollectedTokenNumber());
}

inline boolean KWTextTokenizer::IsDeploymentMode() const
{
	return bIsDeploymentMode;
}

inline int KWTextTokenizer::GetMaxWordLength()
{
	return 30;
}

inline int KWTextNgramTokenizer::GetMaxNgramLength()
{
	return 8;
}
