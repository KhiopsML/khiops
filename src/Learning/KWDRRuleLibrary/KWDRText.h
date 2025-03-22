// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation de traitement des textes
// Toutes ces regles sont des transpositions exactes des regles correspondantes
// sur les chaines de caracteres definies dans KWDRString
//
// Les differences sont:
//  . le nom est prefixe par "Text"
//  . le libelle se refere a "text" au lieu de "categorical"
//  . le type du premier operande est Text au lieu de Symbol
//  . le type de la valeur retour est Text au lieu de Symbol, si necessaire
//
// Toutes les regles de KWDRString sont implementees de facon generique en sous-classes
// de la classe KWDRStringRule, ce qui permet de redefinir tres simplement chaque
// regle correspondante pour le type Text
// Seul la regle KWDRTextLoadFile est specifique au type Text

class KWDRTextLoadFile;
class KWDRTextLength;
class KWDRTextLeft;
class KWDRTextRight;
class KWDRTextMiddle;
class KWDRTextTokenLength;
class KWDRTextTokenLeft;
class KWDRTextTokenRight;
class KWDRTextTokenMiddle;
class KWDRTextTranslate;
class KWDRTextSearch;
class KWDRTextReplace;
class KWDRTextReplaceAll;
class KWDRTextRegexMatch;
class KWDRTextRegexSearch;
class KWDRTextRegexReplace;
class KWDRTextRegexReplaceAll;
class KWDRTextToUpper;
class KWDRTextToLower;
class KWDRTextConcat;
class KWDRTextHash;
class KWDRTextEncrypt;

#include "KWDRString.h"
#include "InputBufferedFile.h"
#include "HugeBuffer.h"

// Enregistrement de ces regles
void KWDRRegisterTextRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextLoadFile
// Chargement d'un fichier sous forme d'une variable Text
// Le fichier peut etre local ou avoir un nom ayant une URI
// Le chargement se fait a concurrence de la taille max d'une variable de type Text
// Les caractere '\0', '\r' et '\n' sont remplace par des blancs pour ne pas poser
// de probleme quand on ecrit les base en sortie
class KWDRTextLoadFile : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTextLoadFile();
	~KWDRTextLoadFile();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeTextResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextLength
class KWDRTextLength : public KWDRLength
{
public:
	// Constructeur
	KWDRTextLength();
	~KWDRTextLength();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextLeft
class KWDRTextLeft : public KWDRLeft
{
public:
	// Constructeur
	KWDRTextLeft();
	~KWDRTextLeft();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextRight
class KWDRTextRight : public KWDRRight
{
public:
	// Constructeur
	KWDRTextRight();
	~KWDRTextRight();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextMiddle
class KWDRTextMiddle : public KWDRMiddle
{
public:
	// Constructeur
	KWDRTextMiddle();
	~KWDRTextMiddle();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextTokenLength
class KWDRTextTokenLength : public KWDRTokenLength
{
public:
	// Constructeur
	KWDRTextTokenLength();
	~KWDRTextTokenLength();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextTokenLeft
class KWDRTextTokenLeft : public KWDRTokenLeft
{
public:
	// Constructeur
	KWDRTextTokenLeft();
	~KWDRTextTokenLeft();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextTokenRight
class KWDRTextTokenRight : public KWDRTokenRight
{
public:
	// Constructeur
	KWDRTextTokenRight();
	~KWDRTextTokenRight();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextTokenMiddle
class KWDRTextTokenMiddle : public KWDRTokenMiddle
{
public:
	// Constructeur
	KWDRTextTokenMiddle();
	~KWDRTextTokenMiddle();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextTranslate
class KWDRTextTranslate : public KWDRTranslate
{
public:
	// Constructeur
	KWDRTextTranslate();
	~KWDRTextTranslate();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextSearch
class KWDRTextSearch : public KWDRSearch
{
public:
	// Constructeur
	KWDRTextSearch();
	~KWDRTextSearch();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextReplace
class KWDRTextReplace : public KWDRReplace
{
public:
	// Constructeur
	KWDRTextReplace();
	~KWDRTextReplace();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextReplaceAll
class KWDRTextReplaceAll : public KWDRReplaceAll
{
public:
	// Constructeur
	KWDRTextReplaceAll();
	~KWDRTextReplaceAll();

	// Creation
	KWDerivationRule* Create() const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextRegexMatch
class KWDRTextRegexMatch : public KWDRRegexMatch
{
public:
	KWDRTextRegexMatch();
	~KWDRTextRegexMatch();

	// Creation
	KWDerivationRule* Create() const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextRegexSearch
class KWDRTextRegexSearch : public KWDRRegexSearch
{
public:
	KWDRTextRegexSearch();
	~KWDRTextRegexSearch();

	// Creation
	KWDerivationRule* Create() const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextRegexReplace
class KWDRTextRegexReplace : public KWDRRegexReplace
{
public:
	KWDRTextRegexReplace();
	~KWDRTextRegexReplace();

	// Creation
	KWDerivationRule* Create() const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextRegexReplaceAll
class KWDRTextRegexReplaceAll : public KWDRRegexReplaceAll
{
public:
	KWDRTextRegexReplaceAll();
	~KWDRTextRegexReplaceAll();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextToUpper
class KWDRTextToUpper : public KWDRToUpper
{
public:
	// Constructeur
	KWDRTextToUpper();
	~KWDRTextToUpper();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextToLower
class KWDRTextToLower : public KWDRToLower
{
public:
	// Constructeur
	KWDRTextToLower();
	~KWDRTextToLower();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextHash
class KWDRTextHash : public KWDRHash
{
public:
	// Constructeur
	KWDRTextHash();
	~KWDRTextHash();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextConcat
class KWDRTextConcat : public KWDRConcat
{
public:
	// Constructeur
	KWDRTextConcat();
	~KWDRTextConcat();

	// Creation
	KWDerivationRule* Create() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextEncrypt
class KWDRTextEncrypt : public KWDREncrypt
{
public:
	// Constructeur
	KWDRTextEncrypt();
	~KWDRTextEncrypt();

	// Creation
	KWDerivationRule* Create() const override;
};
