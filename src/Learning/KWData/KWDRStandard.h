// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

///////////////////////////////////////////////////////////////////////////
// Regles de derivation standard

class KWDRCopySymbol;
class KWDRCopyContinuous;
class KWDRCopyDate;
class KWDRCopyTime;
class KWDRCopyTimestamp;
class KWDRCopyTimestampTZ;
class KWDRTextCopy;
class KWDRCopySymbolValueBlock;
class KWDRCopyContinuousValueBlock;
class KWDRAsContinuous;
class KWDRAsContinuousError;
class KWDRRecodeMissing;
class KWDRAsSymbol;
class KWDRFromText;
class KWDRToText;
class KWDRAsDate;
class KWDRFormatDate;
class KWDRAsTime;
class KWDRFormatTime;
class KWDRAsTimestampTZ;
class KWDRFormatTimestampTZ;

#include "KWDerivationRule.h"

// Enregistrement de ces regles
void KWDRRegisterStandardRules();

////////////////////////////////////////////////////////////////////////////
// Class KWDRConversionRule
// Classe ancetre des regles de conversion
// Possibilite de parametrage par un objet pour emetre un message d'erreur personnalise,
// Parametrage valide pour toutes les regles de conversion suimultanement
class KWDRConversionRule : public KWDerivationRule
{
public:
	// Controles de validite avec emission de message d'erreur
	// par la classe passee en parametre (par defaut: NULL, pas de message)
	static void SetErrorSender(const Object* errorSender);
	static const Object* GetErrorSender();

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	static const Object* oErrorSender;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCopySymbol
// Copie d'un attribut Symbol. Permet son renommage
class KWDRCopySymbol : public KWDerivationRule
{
public:
	// Constructeur
	KWDRCopySymbol();
	~KWDRCopySymbol();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCopyContinuous
// Copie d'un attribut Continuous. Permet son renommage
class KWDRCopyContinuous : public KWDerivationRule
{
public:
	// Constructeur
	KWDRCopyContinuous();
	~KWDRCopyContinuous();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCopyDate
// Copie d'un attribut Date. Permet son renommage
class KWDRCopyDate : public KWDerivationRule
{
public:
	// Constructeur
	KWDRCopyDate();
	~KWDRCopyDate();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Date ComputeDateResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCopyTime
// Copie d'un attribut Time. Permet son renommage
class KWDRCopyTime : public KWDerivationRule
{
public:
	// Constructeur
	KWDRCopyTime();
	~KWDRCopyTime();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Time ComputeTimeResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCopyTimestamp
// Copie d'un attribut Timestamp. Permet son renommage
class KWDRCopyTimestamp : public KWDerivationRule
{
public:
	// Constructeur
	KWDRCopyTimestamp();
	~KWDRCopyTimestamp();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Timestamp ComputeTimestampResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCopyTimestampTZ
// Copie d'un attribut TimestampTZ. Permet son renommage
class KWDRCopyTimestampTZ : public KWDerivationRule
{
public:
	// Constructeur
	KWDRCopyTimestampTZ();
	~KWDRCopyTimestampTZ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	TimestampTZ ComputeTimestampTZResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTextCopy
// Copie d'un attribut Text. Permet son renommage
class KWDRTextCopy : public KWDerivationRule
{
public:
	// Constructeur
	KWDRTextCopy();
	~KWDRTextCopy();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeTextResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRValueBlockRule
// Regle generique de type ValueBlock, dont le bloc en retour est de meme
// nature que celui d'un operande (a specifier): meme VarKeyType,
// sous ensemble des VarKey
class KWDRValueBlockRule : public KWDerivationRule
{
public:
	// Reimplementation de la methode qui indique le type de cle du bloc en retour
	int GetVarKeyType() const override;

	// Valeur par defaut des blocs pour les regle retournant un bloc de valeurs
	Continuous GetValueBlockContinuousDefaultValue() const override;
	Symbol& GetValueBlockSymbolDefaultValue() const override;

	// Reimplementation de la verification qu'une regle est completement renseignee et compilable
	boolean CheckCompleteness(const KWClass* kwcOwnerClass) const override;

	// Recopie des attributs de definition de la regle
	void CopyFrom(const KWDerivationRule* kwdrSource) override;

	// Memoire utilisee par la regle de derivation
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constructeur
	KWDRValueBlockRule();

	// Reimplementation de la methode ancetre de completion des informations de specification de la regle
	void InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
				      NumericKeyDictionary* nkdCompletedAttributes) override;

	// Compilation de la regle, a appeler en debut de l'implementation du calcul de l'attribut derive
	// Permet de parametrer correctement quels attributs du bloc source sont a utiliser pour
	// fabriquer le bloc cible
	// En theorie, on pourrait effectuer cette compilation des la compilation, mais on a ici besoin a la fois
	// de la regle a compiler, mais egalement du bloc resultat de la regle (et son indexedKeyBlock).
	// La methode Compile ne prenant pas ce type d'argument (pertinent uniquement dans le cas des blocs),
	// il est ici plus pratique (et peu couteux) d'effectuer cette optimisation via DynamicCompile
	void DynamicCompile(const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Memorisation du type de cle du bloc
	int nVarKeyType;

	// Index de l'operande source de type bloc
	// A specifier dans les sous-classes
	int nSourceValueBlockOperandIndex;

	// Indique s'il faut modifier et verifier que le type de block retourne est celui du bloc en operande (defaut:
	// true)
	boolean bReturnTypeSameAsOperandType;

	///////////////////////////////////////////////////////
	// Donnees de compilation dynamique permetttant calculer effeccacement les blocs cibles a partir des blocs
	// sources

	// Indique que les vecteurs sources et cible on meme index de valeurs
	mutable boolean bSameValueIndexes;

	// Vecteur des nouveaux index de valeurs, contenant pour chaque index de valeur source l'index de valeur cible
	// si elle est gardee, -1 sinon
	// Ce vecteur est calcule lors du DynamicCompile
	mutable IntVector ivNewValueIndexes;

	// Fraicheur de compilation dynamique
	mutable int nDynamicCompileFreshness;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCopySymbolValueBlock
// Copie d'un attribut SymbolValueBlock. Permet son renommage
class KWDRCopySymbolValueBlock : public KWDRValueBlockRule
{
public:
	// Constructeur
	KWDRCopySymbolValueBlock();
	~KWDRCopySymbolValueBlock();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWSymbolValueBlock* ComputeSymbolValueBlockResult(const KWObject* kwoObject,
							  const KWIndexedKeyBlock* indexedKeyBlock) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRCopyContinuousValueBlock
// Copie d'un attribut ContinuousValueBlock. Permet son renommage
class KWDRCopyContinuousValueBlock : public KWDRValueBlockRule
{
public:
	// Constructeur
	KWDRCopyContinuousValueBlock();
	~KWDRCopyContinuousValueBlock();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject,
					  const KWIndexedKeyBlock* indexedKeyBlock) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsContinuous
// Transformation d'un attribut Symbol en attribut Continuous
class KWDRAsContinuous : public KWDRConversionRule
{
public:
	// Constructeur
	KWDRAsContinuous();
	~KWDRAsContinuous();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsContinuousError
// Renvoie le libelle de l'eventuelle erreur de conversion d'un
// d'un attribut Symbol en attribut Continuous (vide si OK)
class KWDRAsContinuousError : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAsContinuousError();
	~KWDRAsContinuousError();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRRecodeMissing
// Renvoie la valeur Continuous en premier parametre si elle est different de Missing,
// et la valeur de remplacement en deuxieme parametre sinon
class KWDRRecodeMissing : public KWDerivationRule
{
public:
	// Constructeur
	KWDRRecodeMissing();
	~KWDRRecodeMissing();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsSymbol
// Transformation d'un attribut Continuous en attribut Symbol
class KWDRAsSymbol : public KWDRConversionRule
{
public:
	// Constructeur
	KWDRAsSymbol();
	~KWDRAsSymbol();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRFromText
// Transformation d'un attribut Text en attribut Symbol
class KWDRFromText : public KWDRConversionRule
{
public:
	// Constructeur
	KWDRFromText();
	~KWDRFromText();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRToText
// Transformation d'un attribut Symbol en attribut Text
class KWDRToText : public KWDRConversionRule
{
public:
	// Constructeur
	KWDRToText();
	~KWDRToText();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeTextResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsDate
// Recodage d'un Symbol en date, selon un format passe en parametre
// Le format doit etre valide
// La date resultante est potentiellement non valide
class KWDRAsDate : public KWDRConversionRule
{
public:
	// Constructeur
	KWDRAsDate();
	~KWDRAsDate();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Date ComputeDateResult(const KWObject* kwoObject) const override;

	// Verification de la validite du format de date
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation pour optimiser la gestion du format
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWDateFormat dtfDateFormat;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRFormatDate
// Recodage d'une date en symbol selon un format passe en parametre
// Le format doit etre valide
// Renvoie chaine vide si date non valide
class KWDRFormatDate : public KWDerivationRule
{
public:
	// Constructeur
	KWDRFormatDate();
	~KWDRFormatDate();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	// Verification de la validite du format de date
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation pour optimiser la gestion du format
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWDateFormat dtfDateFormat;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsTime
// Recodage d'un Symbol en time, selon un format passe en parametre
// Le format doit etre valide
// La time resultante est potentiellement non valide
class KWDRAsTime : public KWDRConversionRule
{
public:
	// Constructeur
	KWDRAsTime();
	~KWDRAsTime();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Time ComputeTimeResult(const KWObject* kwoObject) const override;

	// Verification de la validite du format de time
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation pour optimiser la gestion du format
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWTimeFormat tmfTimeFormat;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRFormatTime
// Recodage d'une time en symbol selon un format passe en parametre
// Le format doit etre valide
// Renvoie chaine vide si time non valide
class KWDRFormatTime : public KWDerivationRule
{
public:
	// Constructeur
	KWDRFormatTime();
	~KWDRFormatTime();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	// Verification de la validite du format de time
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation pour optimiser la gestion du format
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWTimeFormat tmfTimeFormat;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsTimestamp
// Recodage d'un Symbol en timestamp, selon un format passe en parametre
// Le format doit etre valide
// La timestamp resultante est potentiellement non valide
class KWDRAsTimestamp : public KWDRConversionRule
{
public:
	// Constructeur
	KWDRAsTimestamp();
	~KWDRAsTimestamp();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Timestamp ComputeTimestampResult(const KWObject* kwoObject) const override;

	// Verification de la validite du format de timestamp
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation pour optimiser la gestion du format
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWTimestampFormat tsfTimestampFormat;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRFormatTimestamp
// Recodage d'un timestamp en symbol selon un format passe en parametre
// Le format doit etre valide
// Renvoie chaine vide si timestamp non valide
class KWDRFormatTimestamp : public KWDerivationRule
{
public:
	// Constructeur
	KWDRFormatTimestamp();
	~KWDRFormatTimestamp();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	// Verification de la validite du format de timestamp
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation pour optimiser la gestion du format
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWTimestampFormat tsfTimestampFormat;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsTimestampTZ
// Recodage d'un Symbol en timestampTZ, selon un format passe en parametre
// Le format doit etre valide
// La timestampTZ resultante est potentiellement non valide
class KWDRAsTimestampTZ : public KWDRConversionRule
{
public:
	// Constructeur
	KWDRAsTimestampTZ();
	~KWDRAsTimestampTZ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	TimestampTZ ComputeTimestampTZResult(const KWObject* kwoObject) const override;

	// Verification de la validite du format de timestampTZ
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation pour optimiser la gestion du format
	void Compile(KWClass* kwcOwnerClass) override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWTimestampTZFormat tstzfTimestampTZFormat;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRFormatTimestampTZ
// Recodage d'un timestampTZ en symbol selon un format passe en parametre
// Le format doit etre valide
// Renvoie chaine vide si timestampTZ non valide
class KWDRFormatTimestampTZ : public KWDerivationRule
{
public:
	// Constructeur
	KWDRFormatTimestampTZ();
	~KWDRFormatTimestampTZ();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	// Verification de la validite du format de timestampTZ
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation pour optimiser la gestion du format
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWTimestampTZFormat tstzfTimestampTZFormat;
};