// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation de traitement des chaines de caracteres

class KWDRLength;
class KWDRLeft;
class KWDRRight;
class KWDRMiddle;
class KWDRTokenLength;
class KWDRTokenLeft;
class KWDRTokenRight;
class KWDRTokenMiddle;
class KWDRTranslate;
class KWDRSearch;
class KWDRReplace;
class KWDRReplaceAll;
class KWDRReplaceValues;
class KWDRRegexMatch;
class KWDRRegexSearch;
class KWDRRegexReplace;
class KWDRRegexReplaceAll;
class KWDRToUpper;
class KWDRToLower;
class KWDRConcat;
class KWDRHash;
class KWDREncrypt;

#include "KWDerivationRule.h"
#include "KWDRVector.h"
#include "Regex.h"

// Enregistrement de ces regles
void KWDRRegisterStringRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRLength
// Longueur d'un attribut Symbol
class KWDRLength : public KWDerivationRule
{
public:
	// Constructeur
	KWDRLength();
	~KWDRLength();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRLeft
// Extraction de la sous-chaine gauche d'un attribut Symbol
// Si le nombre de caracteres est negatif, retourne chaine vide
// Si le nombre de caracteres depasse la longueur, retourne la chaine initiale
class KWDRLeft : public KWDerivationRule
{
public:
	// Constructeur
	KWDRLeft();
	~KWDRLeft();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRRight
// Extraction de la sous-chaine droite d'un attribut Symbol
// Si le nombre de caracteres est negatif, retourne chaine vide
// Si le nombre de caracteres depasse la longueur, retourne la chaine initiale
class KWDRRight : public KWDerivationRule
{
public:
	// Constructeur
	KWDRRight();
	~KWDRRight();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRMiddle
// Extraction d'une sous-chaine d'un attribut Symbol
// Si la position de depart est incorrecte (debut a 1), retourne chaine vide
// Si le nombre de caracteres est negatif, retourne chaine vide
// Si la fin d'extraction depasse la chaine initiale, retourne la fin
//   de la chaine initiale
class KWDRMiddle : public KWDerivationRule
{
public:
	// Constructeur
	KWDRMiddle();
	~KWDRMiddle();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTokenLength
// Longueur d'un attribut Symbol en terme de nombre de tokens
// Un token est une sous-chaine de caracteres non vide ne contenant aucun
// caracteres delimiteurs. Les tokens sont separes par un ou plusieurs
// caracteres delimiteurs, qui prennent leur valeur dans un ensemble.
// Si aucun caractere delimiteur n'est specifie, il ne peut y avoir au plus
// qu'un seul token dans une chaine
// Exemple: avec des caracteres delimiteur " ," (blanc et espace), la chaine
// " Nombres: 1, 2, 3.14, 4,5 " contient exactement 6 tokens:
//   'Nombres:'  '1'  '2' '3.14'  '4'  '5'.
class KWDRTokenLength : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTokenLength();
	~KWDRTokenLength();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTokenLeft
// Extraction des tokens de gauche d'un attribut Symbol
// Si plusieurs tokens sont extraits, ils restent separes par les separateurs
//   initiaux presents dans la chaine initiale
// Si le nombre de tokens est negatif, retourne chaine vide
// Si le nombre de tokens depasse la longueur en token, retourne la chaine
//   initiale(expurgee de ses delimiteurs de debut et fin)
class KWDRTokenLeft : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTokenLeft();
	~KWDRTokenLeft();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTokenRight
// Extraction des tokens de droite d'un attribut Symbol
// Si plusieurs tokens sont extraits, ils restent separes par les separateurs
//   initiaux presents dans la chaine initiale
// Si le nombre de tokens est negatif, retourne chaine vide
// Si le nombre de tokens depasse la longueur en token, retourne la chaine
//   initiale(expurgee de ses delimiteurs de debut et fin)
class KWDRTokenRight : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTokenRight();
	~KWDRTokenRight();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTokenMiddle
// Extraction d'un ou plusieurs tokens d'un attribut Symbol
// Si la position de depart (en token avec debut a 1) est incorrecte,
//    retourne chaine vide
// Si plusieurs tokens sont extraits, ils restent separes par les separateurs
//   initiaux presents dans la chaine initiale
// Si le nombre de tokens est negatif, retourne chaine vide
// Si le nombre de tokens depasse la longueur en token, retourne la fin de
//   la chaine initiale(expurgee de ses delimiteurs de debut et fin)
class KWDRTokenMiddle : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTokenMiddle();
	~KWDRTokenMiddle();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTranslate
// Remplacement en sequence des valeurs d'une liste de valeurs recherchees
// les valeurs d'une liste de valeurs de remplacement,s de meme taille
// Pratique par exemple pour remplacer tous les caracteres accentues,
// quelque soit leur taille
class KWDRTranslate : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTranslate();
	~KWDRTranslate();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	// Verification de la validite: les listes de valeurs doivent etre de meme taille
	boolean CheckCompletness(const KWClass* kwcOwnerClass) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSearch
// Recherche de la position d'une sous-chaine dans un attributs Symbol
// Si la position de depart est invalide (debut a 1), retourne -1
// Si la sous-chaine n'est pas trouvee, retourne -1
class KWDRSearch : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSearch();
	~KWDRSearch();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRReplace
// Remplacement d'une sous-chaine par une autre dans un attribut Symbol
// Si la position de depart est invalide (debut a 1), retourne la chaine initiale
// Si la sous-chaine n'est pas trouvee, retourne la chaine initiale, sinon retourne la
//   la chaine modifiee
class KWDRReplace : public KWDerivationRule
{
public:
	// Constructeur
	KWDRReplace();
	~KWDRReplace();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRReplaceAll
// Remplacement d'une sous-chaine par une autre dans un attribut Symbol,
// avec remplacements multiples si la sous-chaine est presente plusieurs fois
// Si la position de depart est invalide (debut a 1), retourne la chaine initiale
// Si la sous-chaine n'est pas trouvee, retourne la chaine initiale, sinon retourne la
//   la chaine modifiee
class KWDRReplaceAll : public KWDerivationRule
{
public:
	// Constructeur
	KWDRReplaceAll();
	~KWDRReplaceAll();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRRegex
// classe ancetre (abstraite) des regles utilisant des regex
class KWDRRegex : public KWDerivationRule
{
public:
	KWDRRegex();
	~KWDRRegex();

	// Creation
	KWDerivationRule* Create() const override = 0;

	// Verification qu'une regle est completement renseignee et compilable
	boolean CheckCompletness(const KWClass* kwcOwnerClass) const override;

	///////////////////////////////////////////
	///// Implementation
protected:
	mutable Regex regEx;
};

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRRegexMatch
// Recherche si chaine de caractere correspond completement a une expression reguliere,
class KWDRRegexMatch : public KWDRRegex
{
public:
	KWDRRegexMatch();
	~KWDRRegexMatch();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRRegexSearch
// Recherche une chaine de caractere dans un texte via une expression reguliere,
// a partir d'une position de depart, et renvoie la position du premier caractere si trouve
class KWDRRegexSearch : public KWDRRegex
{
public:
	KWDRRegexSearch();
	~KWDRRegexSearch();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRRegexReplace
// Remplace la premiere occurence trouvee d'une exprtession reguliere dans un texte,
// en commencant la recherche a partir d'une position de depart
// Renvoie le texte modifie, ou la chaine originale, si aucune occurence n'a ete trouvee
class KWDRRegexReplace : public KWDRRegex
{
public:
	KWDRRegexReplace();
	~KWDRRegexReplace();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRRegexReplaceAll
// Remplace toutes les occurences trouvees d'une regex dans un texte,
// en commencant la recherche a partir d'une position de depart
// Renvoie le texte modifie, ou la chaine originale, si aucune occurence n'a ete trouvee
class KWDRRegexReplaceAll : public KWDRRegex
{
public:
	KWDRRegexReplaceAll();
	~KWDRRegexReplaceAll();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRToUpper
// Mise en majuscules d'un attribut Symbol
class KWDRToUpper : public KWDerivationRule
{
public:
	// Constructeur
	KWDRToUpper();
	~KWDRToUpper();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRToLower
// Mise en minuscules d'un attribut Symbol
class KWDRToLower : public KWDerivationRule
{
public:
	// Constructeur
	KWDRToLower();
	~KWDRToLower();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRHash
// Calcul d'une valeur de hachage d'un attribut Symbol,
// entre 0 et une valeur entiere max (non comprise)
class KWDRHash : public KWDerivationRule
{
public:
	// Constructeur
	KWDRHash();
	~KWDRHash();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRConcat
// Concatenation de un, deux (ou plus) attributs Symbol
class KWDRConcat : public KWDerivationRule
{
public:
	// Constructeur
	KWDRConcat();
	~KWDRConcat();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDREncrypt
// Encryptage d'un attribut Symbol a l'aide d'une cle
// L'algorithme utilise est un algorithme base sur une "randomisation"
// de la valeur a crypter. Cet algorithme n'est pas publique est parait
// donc suffisant pour des utilisations assez basiques.
// La valeur cryptee ne contient que des caracteres alphanumeriques
// (chiffres, lettres et '_')
// On ne fournit pas de methode de decryptage.
// Attention: seuls les caracteres imprimables sont encodables. Une valeur
// comportant des caracteres non imprimables sera au prealable transformee
// en remplacant les caracteres non imprimables par des blancs)
class KWDREncrypt : public KWDerivationRule
{
public:
	// Constructeur
	KWDREncrypt();
	~KWDREncrypt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Encryptage d'une valeur avec une cle
	const ALString EncryptString(ALString& sStringToEncrypt, const Symbol& sKey) const;

	// Preprocessing d'une valeur avant encodage
	// (remplacement des caracteres non encodables par des blancs, et gestion
	// des caracteres imprimables non alphanumeriques)
	ALString PreprocessStringForEncryption(const ALString& sStringToEncrypt) const;

	// Remplacement de 3 caracteres par les caracteres d'encryptage d'un block
	void EncryptCharTriple(ALString& sEncryptString, int nEncryptPos, int nBlockCode) const;

	// Recherche d'un dernier caractere de cryptage pour gerer les problemes
	// de type de codage (numerique ou non) et de padding (nombre de caracteres)
	char GetLastEncryptChar(const ALString& sValue, boolean bIsDigit, int nPadNumber) const;

	// Re-encryptage de la fin de la chaine
	void ReEncryptEndOfString(ALString& sEncryptString) const;

	// Test du type de la chaine a encoder
	// Un codage specifique est reserve aux valeurs numeriques, de facon a
	// le rendre plus compacte
	boolean IsDigitString(const ALString& sValue) const;

	// Code d'un caractere
	int GetCharCode(boolean bIsDigit, int nChar) const;

	// Code max d'un caractere
	int GetMaxCharCode(boolean bIsDigit) const;

	// Initialisation des tableaux de travail
	void InitWorkingArrays(const Symbol& sKey) const;

	// Valeur de hashage d'une chaine de caracteres
	int GetStringHashCode(const ALString& sValue) const;

	// Tableaux des paires de caracteres remplacant les caracteres originaux
	// (servant d'index) pour le preprocessing (index=-1: rien)
	mutable IntVector ivFirstPreprocessedChars;
	mutable IntVector ivSecondPreprocessedChars;

	// Tableaux de caracteres encodables
	mutable IntVector ivDigitChars;
	mutable IntVector ivAlphanumChars;

	// Tableaux  des index des caracteres encodables (-1 si non trouve)
	// dans les tableaux de caracteres encodables
	mutable IntVector ivDigitCharIndexes;
	mutable IntVector ivAlphanumCharIndexes;

	// Tableau des caracteres cryptes (caracteres alphanumeriques)
	mutable IntVector ivEncryptChars;

	// Tableau des codes de triplets de caracteres cryptes
	mutable IntVector ivEncryptCharTripleCodes;

	// Memorisation de la cle et d'une valeur de hashage associee
	mutable ALString sEncryptionKey;
	mutable int nEncryptionHashCode;
};
