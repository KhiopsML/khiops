// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation de creation de Table

class KWDRRelationCreationRule;
class KWDRTableCreationRule;

#include "KWDerivationRule.h"

////////////////////////////////////////////////////////////////////////////
// Classe KWDRRelationCreationRule
// Toute la gestion specifique aux regles de creation de Table ou Entity
// est definie dans cette classe, destinee a etre specialisee dans des sous-classes.
//
// Dans ce type de regle:
// - les objets crees sont detruits automatiquement
// - le type des objet produits en sortie (ObjectClassName) provient
//   de la declaration de la regle
// - la regle peut etre utilise uniquement pour definir une variable d'un dictionnaire,
//   et ne peut etre utilise en operande d'une autre regle
// - toute variable native (non calculee) des objets en sortie doit etre alimentee
//   par la regle, selon un des deux mode d'alimentation suivant:
//   - alimentation de type vue
//   - alimentation de type calcul
// - le dictionnaire en sortie peut avoir ses variable utilisees ou non (Used),
//   et des variable calculee si besoin
// - un dictionnaire en sortie n'est pas specifique a aux regle de creation de Table:
//   a priori, un meme dictionnaire pourrait etre utilise pour une variable native alimentee
//   depuis un fichier, et pour une variable calculee alimentee par une regle de creation
//
// Alimentation de type vue:
// - par defaut, le premier operande de ce type de regle est de type relation, ce qui permet
// de definir la source de la vue dans les objets en entree
// - toute variable native en sortie correspondant a une variable en entree de meme nom
//   est alimentee par la valeur de cette variable
//   - cette alimentation de type vue est similaire a l'alimentation des objets en memoire
//     a partir des enregistremenst d'un fichier de donnees, en se basant sur la correspondance
//     des noms de variables dfans le dictionnaire avec les noms des colonnes dans le fichier
//   - les variables correspondantes doivent etre de meme type en entree et en sortie
//   - les variables en sortie peuvent etre alimentee par des variables calculee en entree
//   - les variable en sortie peuvent etre de type Stored (Numerical, Categorical, Timestamp, Text...)
//     comme dans les fichier, ou de type Entity ou Table
//   - on peut avoir des variables calculees en sortie
//     - les variables calculees en sortie peuvent avoir le meme nom que des variables
//       en entree, calculees ou non, meme avec des type differents; cela permet de creer
//       de nouvelles variables independament en entree et en sortie
//
// Alimentation de type calcul
// - une regle doit specifier explicitement dans ces operandes la liste des variables natives en
//   sortie a alimenter, en utilisant ':' pour separer les operande en entree des operandes en sortie
// - il peut y avoir plusieurs operandes en sortie, separees par des ','
// - les operandes en sortie designent explicitement les noms des variables natives a alimenter dans
//   le dictionnaire en sortie
//   - cela ne concerne ques les variables natives en sortie du premier niveau dans le cas de variables
//     de type relation: on ne peut pas specifier l'amilementation d'une variable secondaire d'une
//     sous-table en sortie
class KWDRRelationCreationRule : public KWDerivationRule
{
public:
	// Constructeur
	KWDRRelationCreationRule();
	~KWDRRelationCreationRule();

	// On indique que la regle cree de nouveau objets
	boolean GetReference() const override;

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des operandes en sortie de regles
	// Cela ne concerne que les regles de creation de Table ou Entity,
	// c'est a dire avec une methode GetReference() renvoyant false
	// On peut alors designer la liste des attributs natifs de la table en sortie
	// a alimenter par la regle.
	// On utilise la classe KWDerivationRuleOperand de facon generique comme
	// pour les operandes en entree, bien que seuls soient autorises les
	// operande avec origine de type attribut (ni constante, ni regle)

	// Nombre d'operandes en sortie
	int GetOutputOperandNumber() const override;

	// Initialisation/modification du nombre d'operande
	// Les operandes en trop sont supprimes, ceux en plus sont ajoutes
	// avec une origine de type attribut
	void SetOutputOperandNumber(int nValue);

	// Indicateur de nombre variable d'operandes en sortie (defaut: false)
	// Fonctionnalite similaire aux operandes en entree
	// Si le type du dernier operande en sortie n'est pas specifie (KWType::Unknown),
	// on verifie que le type est un type de variable native compatible avec celui
	// du dictonnaire en sortie
	// Dans le cas d'un nombre variable d'operandes a la fois en entree et en sortie,
	// ceux ci doivent etre en meme nombre et de meme type
	void SetVariableOutputOperandNumber(boolean bValue);
	boolean GetVariableOutputOperandNumber() const override;

	// Acces aux operandes en sortie
	KWDerivationRuleOperand* GetOutputOperandAt(int nIndex) const override;

	// Ajout d'un operande en sortie en fin de liste
	void AddOutputOperand(KWDerivationRuleOperand* operand);

	// Destruction de tous les operandes en sortie
	void DeleteAllOutputOperands();

	// Supression de tous les operandes en sortie, sans les detruire
	void RemoveAllOutputOperands();

	// Destruction de tous les operandes variables en sortie
	// Les operandes de tete (en nombre fixe) sont preserves
	// Methode sans effet pour les regles a nombre fixe d'operandes
	void DeleteAllVariableOutputOperands();

	//////////////////////////////////////////////////////////////////////////////////////
	// Redefinition des methodes pour tenir compte des alimentation de type vue et calcul

	// Renommage d'une classe
	void RenameClass(const KWClass* refClass, const ALString& sNewClassName) override;

	// Verification de la definbition d'une regle
	boolean CheckRuleDefinition() const override;
	boolean CheckOperandsDefinition() const override;

	// Verification qu'une regle est une specialisation d'une regle plus generale
	boolean CheckRuleFamily(const KWDerivationRule* ruleFamily) const override;
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;

	// Verification qu'une regle est completement renseignee et compilable
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Verification de l'absence de cycle de derivation
	boolean ContainsCycle(NumericKeyDictionary* nkdGreyAttributes,
			      NumericKeyDictionary* nkdBlackAttributes) const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	// Construction du dictionnaire de tous les operandes utilises
	void BuildAllUsedOperands(NumericKeyDictionary* nkdAllUsedOperands) const override;

	// Construction du dictionnaire de tous les attributs utilises
	void BuildAllUsedAttributes(const KWAttribute* derivedAttribute,
				    NumericKeyDictionary* nkdAllUsedAttributes) const override;

	// Copie
	void CopyFrom(const KWDerivationRule* kwdrSource) override;

	// Comparaison
	int FullCompare(const KWDerivationRule* rule) const override;

	// Memoire utilisee par la regle de derivation
	longint GetUsedMemory() const override;

	// Cle de hashage de la regle et des ses operandes
	longint ComputeHashValue() const override;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteUsedRuleOperands(ostream& ost) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Redefinition des methodes virtuelles
	void InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
				      NumericKeyDictionary* nkdCompletedAttributes) override;

	// Test si le type d'un operande en sortie est valid
	boolean IsValidOutputOperandType(int nType) const;

	// Indique si l'alimentation de type vue est activee
	// Dans ce cas, le premier operande est de type Relation pour le dictionnaire en entree de la vue,
	// et on verifie la compatibilite entre les attributs natif du dictionnaire en sortie et
	// les attributs correspondants du dictionnaire en entree
	// Par defaut: true
	virtual boolean IsViewModeActivated() const;

	// Creation d'un objet de la vue avec le dictionnaire en sortie
	KWObject* NewTargetObject(longint lCreationIndex) const;

	// Alimentation de type vue des attributs cibles
	void FillViewModeTargetAttributes(const KWObject* kwoSourceObject, KWObject* kwoTargetObject) const;

	// Alimentation de type calcul des attributs cibles dans le cas d'un nombre variable d'operandes en entree
	// Dans ce cas, on doit avoir egalement un nombre variable d'operandes en sortie, qui doivent
	// correspondre aux operande en entree
	void FillComputeModeTargetAttributesForVariableOperandNumber(const KWObject* kwoSourceObject,
								     KWObject* kwoTargetObject) const;

	// Operandes en sortie
	ObjectArray oaOutputOperands;

	// Indicateur de regle a nombre variable d'operandes en sortie
	boolean bVariableOutputOperandNumber;

	// Index de chargement des attributs pour une alimentation de type calcul
	// On precise pour chaque attribut concerne lie a un operande en sortie son index
	// dans le dictionnaire cible
	KWLoadIndexVector livComputeModeTargetAttributeLoadIndexes;
	IntVector ivComputeModeTargetAttributeTypes;

	// Index de chargement des attributs pour une alimentation de type vue
	// On precise pour chaque attribut concerne son index dans le dictionnaire source et cible
	KWLoadIndexVector livViewModeSourceAttributeLoadIndexes;
	KWLoadIndexVector livViewModeTargetAttributeLoadIndexes;
	IntVector ivViewModeTargetAttributeTypes;

	// Classe de la cible de la vue
	KWClass* kwcCompiledTargetClass;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableCreationRule
// Classe ancetre des regle de creation de Table
class KWDRTableCreationRule : public KWDRRelationCreationRule
{
public:
	// Constructeur
	KWDRTableCreationRule();
	~KWDRTableCreationRule();

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Object Array en retour de la regle
	mutable ObjectArray oaResult;
};

/////////////////////////////////////////////////////////////////
// Methodes en inline

inline int KWDRRelationCreationRule::GetOutputOperandNumber() const
{
	return oaOutputOperands.GetSize();
}

inline void KWDRRelationCreationRule::SetVariableOutputOperandNumber(boolean bValue)
{
	bVariableOutputOperandNumber = bValue;
}

inline boolean KWDRRelationCreationRule::GetVariableOutputOperandNumber() const
{
	return bVariableOutputOperandNumber;
}

inline KWDerivationRuleOperand* KWDRRelationCreationRule::GetOutputOperandAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < oaOutputOperands.GetSize());
	return cast(KWDerivationRuleOperand*, oaOutputOperands.GetAt(nIndex));
}

inline boolean KWDRRelationCreationRule::IsValidOutputOperandType(int nType) const
{
	return KWType::IsStored(nType) or KWType::IsRelation(nType);
}

inline KWObject* KWDRRelationCreationRule::NewTargetObject(longint lCreationIndex) const
{
	KWObject* kwoTargetObject;

	require(IsCompiled());
	require(lCreationIndex >= 1);

	kwoTargetObject = new KWObject(kwcCompiledTargetClass, 1);
	kwoTargetObject->SetViewTypeUse(true);
	return kwoTargetObject;
}
