// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDRTextNgrams;
class KWDRTextListNgrams;
class KWDRTextWords;
class KWDRTextListWords;
class KWDRTextTokens;
class KWDRTextListTokens;
class KWDRTextInit;

#include "KWDerivationRule.h"
#include "KWValueBlock.h"
#include "TextService.h"
#include "KWTextTokenizer.h"

#include "KWDRTextualAnalysisPROTO.h"

// Enregistrement de ces regles
void KWDRRegisterTextualAnalysisRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTokenizationRule
// Regle generique de tokenization de texte
class KWDRTokenizationRule : public KWDerivationRule
{
public:
	// Constructeur par defaut, pour un operande de type Text
	KWDRTokenizationRule();
	~KWDRTokenizationRule();

	// Reimplementation de la methode qui indique le type de cle du bloc en retour
	int GetVarKeyType() const override;

	// Calcul de l'attribut derive
	// Le resultat appartient a l'appelant
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;

	// Valeur par defaut des blocs pour les regle retournant un bloc de valeurs
	Continuous GetValueBlockContinuousDefaultValue() const override;

	// Calcul d'une cle d'attribut a partir d'un token, resultant d'un encodage en format lisible par l'utilisateur
	// Renvoie "" si token invalide
	const ALString BuildAttributeKeyFromToken(const ALString& sToken) const;

	// Calcul d'un token a partir d'une VarKey
	// Renvoie "" si VarKey invalide
	const ALString BuildTokenFromAttributeKey(const ALString& sToken) const;

	// Taille max des tokens acceptes (defaut: 0, signifie pas de contrainte
	virtual int GetMaxTokenLength() const;

	// Verification de la validite des VarKeys du bloc en sortiere, associe a la regle a verifier
	boolean CheckBlockAttributes(const KWClass* kwcOwnerClass,
				     const KWAttributeBlock* attributeBlock) const override;

	// Creation generique d'une regle de tokenisation pour un type d'attribut textuel et un type de d'attribut
	// construit de type texte Memoire: la regle en retour appartient a l'appelant
	static KWDRTokenizationRule* CreateTokenizationRule(int textualTupe, const ALString& sTextFeatures);

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Compilation de la regle, a appeler en debut de l'implementation du calcul de l'attribut derive
	// Permet de parametrer correctement le tokenizer en deploiement pour fabriquer le bloc cible
	// En theorie, on pourrait effectuer cette compilation des la compilation, mais on a ici besoin a la fois
	// de la regle a compiler, mais egalement du bloc resultat de la regle (et son indexedKeyBlock).
	// La methode Compile ne prenant pas ce type d'argument (pertinent uniquement dans le cas des blocs),
	// il est ici plus pratique (et peu couteux) d'effectuer cette optimisation via DynamicCompile
	void DynamicCompile(const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Fraicheur de compilation dynamique
	mutable int nDynamicCompileFreshness;

	// Tokenizer de texte utilise
	KWTextTokenizer* textTokenizer;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextNgrams
// Calcul l'effectif de chaque ngram d'une chaine de caracteres
// Les cles de variables en parametres permettent de specifier
// les ngrams a compter effectivement
class KWDRTextNgrams : public KWDRTokenizationRule
{
public:
	// Constructeur
	KWDRTextNgrams();
	~KWDRTextNgrams();

	// Creation
	KWDerivationRule* Create() const override;

	// Taille max des tokens acceptes
	int GetMaxTokenLength() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListNgrams
// Specialisation de la classe KWDRTextNgrams, en prenant un TextList en enteee au lieu d'un Text
class KWDRTextListNgrams : public KWDRTextNgrams
{
public:
	// Constructeur
	KWDRTextListNgrams();
	~KWDRTextListNgrams();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextWords
// Calcul l'effectif de chaque mot d'une chaine de caracteres
// Un mot est une suite de caracteres sans ponctuation, ou une suite de ponctuations,
// en utilsiant des caracteres separateurs blancs
// Les cles de variables en parametres permettent de specifier
// les mots a compter effectivement
class KWDRTextWords : public KWDRTokenizationRule
{
public:
	// Constructeur
	KWDRTextWords();
	~KWDRTextWords();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextListWords
// Specialisation de la classe KWDRTextTokens, en prenant un TextList en enteee au lieu d'un Text
class KWDRTextListWords : public KWDRTextWords
{
public:
	// Constructeur
	KWDRTextListWords();
	~KWDRTextListWords();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextTokens
// Calcul l'effectif de chaque mot d'une chaine de caracteres
// Un mot est une suite de caracteres sans ponctuation, ou une suite de ponctuations,
// en utilsiant des caracteres separateurs blancs
// Les cles de variables en parametres permettent de specifier
// les mots a compter effectivement
class KWDRTextTokens : public KWDRTokenizationRule
{
public:
	// Constructeur
	KWDRTextTokens();
	~KWDRTextTokens();

	// Creation
	KWDerivationRule* Create() const override;
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
