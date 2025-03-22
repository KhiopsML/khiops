// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

///////////////////////////////////////////////////////////////////////////
// Regles de derivation de transcodage des valeurs d'un attribut

// Regles de memorisation des structures de partitionnement de valeur
// par discretisation et groupement de valeur, elementaires ou non
class KWDRUnivariatePartition;
class KWDRIntervalBounds;
class KWDRContinuousValueSet;
class KWDRValueGroup;
class KWDRValueGroups;
class KWDRSymbolValueSet;

// Test d'appartenance a un intervalle ou groupe de valeur
class KWDRInInterval;
class KWDRInGroup;

// Identifiant d'une partie pour une regle de partitionnement
class KWDRIntervalId;
class KWDRContinuousValueId;
class KWDRGroupId;
class KWDRSymbolValueId;

// Index d'une partie pour une regle de partitionnement
class KWDRIntervalIndex;
class KWDRContinuousValueIndex;
class KWDRGroupIndex;
class KWDRSymbolValueIndex;

#include "KWDerivationRule.h"
#include "KWStructureRule.h"
#include "KWDRVector.h"
#include "KWSortableIndex.h"
#include "KWContinuous.h"
#include "KWSymbol.h"
#include "KWDataGridStats.h"

// Enregistrement de ces regles
void KWDRRegisterPreprocessingRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRUnivariatePartition
// Classe abstraite, ancetre des regles specifiant des partitions univariees
// Permet un acces generiques aux caracteristiques principales
class KWDRUnivariatePartition : public KWDRStructureRule
{
public:
	////////////////////////////////////////////////
	// Methodes a redefinir

	// Nombre de parties
	virtual int GetPartNumber() const = 0;

	// Type de l'attribut partitionne (renvoie KWType::Continuous ou KWType::Symbol)
	virtual int GetAttributeType() const = 0;

	// Index de la partie associee a une valeur
	// Le type de valeur doit etre compatible avec celui de l'attribut
	virtual int GetContinuousPartIndex(Continuous cValue) const;
	virtual int GetSymbolPartIndex(const Symbol& sValue) const;

	// Libelle de la partie associee a un index "interne" de partie (entre 0 et N-1)
	// Renvoie vide si index non valide
	virtual const ALString GetPartLabelAt(int nIndex) const;

	// Prefixe des identifiants, forme selon <prefix><index+1>
	// Les index "internes" ont un role technique et sont compris entre 0 et N-1
	// Les index "externes" (renvoyes par les regles) ont un role documentaire et sont compris entre 1 et N
	// Les Id ont un role documentaire et sont compris entre 1 et N
	static const ALString GetIdPrefix();

	// Calcul d'un identifiant a partir d'un index interne
	static Symbol ComputeId(int nIndex);
};

///////////////////////////////////////////////////////////////
// Classe KWDRIntervalBounds
// Regle de derivation de type Structure, memorisant les bornes
// d'une discretisation
class KWDRIntervalBounds : public KWDRUnivariatePartition
{
public:
	// Constructeur
	KWDRIntervalBounds();
	~KWDRIntervalBounds();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant les
	// bornes des intervalles dans le vecteur prevu a cet effet

	// Nombre d'intervalles
	// Le setter fait basculer en interface de structure,
	// et le getter est accessible en interface de structure et de base
	// La methode peut etre appelee plusieurs fois pour retailler le vecteur
	void SetIntervalBoundNumber(int nValue);
	int GetIntervalBoundNumber() const;

	// Parametrage des bornes
	void SetIntervalBoundAt(int nIndex, Continuous cBound);
	Continuous GetIntervalBoundAt(int nIndex) const;

	// Nombre de parties
	int GetPartNumber() const override;

	// Type de l'attribut partitionne (renvoie KWType::Continuous)
	int GetAttributeType() const override;

	// Index de la partie associee a une valeur
	int GetContinuousPartIndex(Continuous cValue) const override;

	// Libelle de la partie associee a un index de partie
	const ALString GetPartLabelAt(int nIndex) const override;

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile

	// Index correspondant a l'intervalle contenant la valeur
	int ComputeIntervalIndex(Continuous cValue) const;

	//////////////////////////////////////////////////////
	// Import/export avec une partition d'attribut

	// Import
	void ImportAttributeDiscretization(const KWDGSAttributeDiscretization* attributeDiscretization);

	// Export
	// La regle source doit etre valide et compilee
	void ExportAttributeDiscretization(KWDGSAttributeDiscretization* attributeDiscretization) const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Nettoyage de l'interface de base une fois la regle compilee
	void CleanCompiledBaseInterface() override;

	//////////////////////////////////////////////////////
	// Redefinition des methodes de structure

	// Recopie de la partie structure de la regle
	void CopyStructureFrom(const KWDerivationRule* kwdrSource) override;

	// Transfert de la specification de base de la regle source
	// vers la specification de structure de la regle en cours
	void BuildStructureFromBase(const KWDerivationRule* kwdrSource) override;

	// Verification de la partie structure de la regle
	boolean CheckStructureDefinition() const override;

	// Affichage, ecriture dans un fichier
	void WriteStructureUsedRule(ostream& ost) const override;

	// Methode de comparaison entre deux regles
	int FullCompare(const KWDerivationRule* rule) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Bornes
	ContinuousVector cvIntervalBounds;
};

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueSet
// Regle de derivation de type Structure, memorisant les valeur
// d'un ensemble
// Analogue a une partition en intervalles elementaires centres
// sur les valeurs, ce qui permet de memoriser les valeurs
// elles-memes (au lieu des bornes des intervalles).
// Cela est notament utile dans les calculs d'esperance utilise
// dans les prediction des regresseurs
class KWDRContinuousValueSet : public KWDRUnivariatePartition
{
public:
	// Constructeur
	KWDRContinuousValueSet();
	~KWDRContinuousValueSet();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant les
	// valeurs (ordonnees) dans le vecteur prevu a cet effet

	// Nombre de valeurs
	// Le setter fait basculer en interface de structure,
	// et le getter est accessible en interface de structure et de base
	// La methode peut etre appelee plusieurs fois pour retailler le vecteur
	void SetValueNumber(int nValue);
	int GetValueNumber() const;

	// Parametrage des valeurs
	void SetValueAt(int nIndex, Continuous cValue);
	Continuous GetValueAt(int nIndex) const;

	// Nombre de parties
	int GetPartNumber() const override;

	// Type de l'attribut partitionne (renvoie KWType::Continuous)
	int GetAttributeType() const override;

	// Index de la partie associee a une valeur (celle la plus proche)
	int GetContinuousPartIndex(Continuous cValue) const override;

	// Libelle de la partie associee a un index de partie
	const ALString GetPartLabelAt(int nIndex) const override;

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile

	// Index correspondant a la valeur de l'ensemble specifie la
	// plus proche de la valeur passe en parametre
	int ComputeValueIndex(Continuous cValue) const;

	//////////////////////////////////////////////////////
	// Import/export avec une partition d'attribut

	// Import
	void ImportAttributeContinuousValues(const KWDGSAttributeContinuousValues* attributeContinuousValues);

	// Export
	// La regle source doit etre valide et compilee
	void ExportAttributeContinuousValues(KWDGSAttributeContinuousValues* attributeContinuousValues) const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Nettoyage de l'interface de base une fois la regle compilee
	void CleanCompiledBaseInterface() override;

	//////////////////////////////////////////////////////
	// Redefinition des methodes de structure

	// Recopie de la partie structure de la regle
	void CopyStructureFrom(const KWDerivationRule* kwdrSource) override;

	// Transfert de la specification de base de la regle source
	// vers la specification de structure de la regle en cours
	void BuildStructureFromBase(const KWDerivationRule* kwdrSource) override;

	// Verification de la partie structure de la regle
	boolean CheckStructureDefinition() const override;

	// Affichage, ecriture dans un fichier
	void WriteStructureUsedRule(ostream& ost) const override;

	// Methode de comparaison entre deux regles
	int FullCompare(const KWDerivationRule* rule) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Valeurs
	ContinuousVector cvContinuousValueSet;
};

///////////////////////////////////////////////////////////////
// Classe KWDRValueGroup
// Sous-classe de KWDRSymbolVector, permettant de gerer la
// valeur speciale "*"
class KWDRValueGroup : public KWDRSymbolVector
{
public:
	// Constructeur
	KWDRValueGroup();
	~KWDRValueGroup();

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	//////////////////////////////////////////////////////
	// Redefinition des methodes de structure

	// Transfert de la specification de base de la regle source
	// vers la specification de structure de la regle en cours
	void BuildStructureFromBase(const KWDerivationRule* kwdrSource) override;

	// Affichage, ecriture dans un fichier
	// Note: la valeur speciale (*) est laissee telle quelle,
	// et entouree de backquotes pour une valeur standard coincidant
	void WriteStructureUsedRule(ostream& ost) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRValueGroups
// Regle de derivation de type Structure, memorisant les groupes
// de valeurs d'une partition de valeurs en groupes
class KWDRValueGroups : public KWDRUnivariatePartition
{
public:
	// Constructeur
	KWDRValueGroups();
	~KWDRValueGroups();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant les
	// groupes dans le tableau de groupes prevu a cet effet
	// L'interface de structure est ici uniquement une encapsulation
	// de l'interface de base (les groupes sont des operandes
	// bases sur des regles KWDRSymbolValues)

	// Nombre de groupes
	// Les groupes sont crees par la methodes, et appartiennent
	// a l'objet appele
	// La methode peut etre appelee plusieurs fois pour changer le nombre de groupes
	void SetValueGroupNumber(int nValue);
	int GetValueGroupNumber() const;

	// Acces aux groupes pour parametrage
	KWDRValueGroup* GetValueGroupAt(int nIndex);

	// Nombre de parties
	int GetPartNumber() const override;

	// Type de l'attribut partitionne (renvoie KWType::Symbol)
	int GetAttributeType() const override;

	// Nombre total des valeurs contenues dans les parties
	int ComputeTotalPartValueNumber() const;

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile

	// Index de la partie associee a une valeur
	int GetSymbolPartIndex(const Symbol& sValue) const override;

	// Libelle de la partie associee a un index de partie
	const ALString GetPartLabelAt(int nIndex) const override;

	// Index correspondant au groupe contenant une valeur
	int ComputeGroupIndex(const Symbol& sValue) const;

	// Nombre total des valeurs contenues dans les parties
	int GetTotalPartValueNumber() const;

	//////////////////////////////////////////////////////
	// Import/export avec une partition d'attribut

	// Import
	void ImportAttributeGrouping(const KWDGSAttributeGrouping* attributeGrouping);

	// Export
	// La regle source doit etre valide et compilee
	void ExportAttributeGrouping(KWDGSAttributeGrouping* attributeGrouping) const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	// Verification
	// Les modalites sources doivent toutes etre distinctes
	// La modalite speciale StarValue doit etre utilise une et une seule fois
	boolean CheckDefinition() const override;

	//////////////////////////////////////////////////////
	// Redefinition des methodes de structure
	// Seule sous-classe de KWDRUnivariatePartition n'implementant pas
	// les service de structure

	// Methode de comparaison entre deux regles
	// A redefinir pour utiliser la methode standard
	int FullCompare(const KWDerivationRule* rule) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation dynamique, lors du premier calcul d'attribut derive

	// Compilation dynamique
	void DynamicCompile() const;

	// Vecteur de toutes les modalites sources et de leur index de groupe correspondant
	// Les modalites sources sont triees pour permettre une recherche par dichotomie
	mutable SymbolVector svSortedValues;
	mutable IntVector ivGroupIndexes;

	// Index de groupe de la modalite speciale
	mutable int nStarValueTargetIndex;

	// Fraicheur de compilation dynamique
	mutable int nDynamicCompileFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueSet
// Regle de derivation de type Structure, memorisant les valeur
// d'un ensemble
// Analogue a une partition en groupes elementaires singletons
// sur les valeurs, ce qui permet de memoriser les valeurs
// elles-memes (au lieu des groupes).
// Cela est notament utile dans les calculs lies aux classifieurs
class KWDRSymbolValueSet : public KWDRUnivariatePartition
{
public:
	// Constructeur
	KWDRSymbolValueSet();
	~KWDRSymbolValueSet();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant les
	// valeurs (ordonnees) dans le vecteur prevu a cet effet

	// Nombre de valeurs
	// Le setter fait basculer en interface de structure,
	// et le getter est accessible en interface de structure et de base
	// La methode peut etre appelee plusieurs fois pour retailler le vecteur
	void SetValueNumber(int nValue);
	int GetValueNumber() const;

	// Parametrage des valeurs
	void SetValueAt(int nIndex, const Symbol& sValue);
	Symbol& GetValueAt(int nIndex) const;

	// Nombre de parties
	int GetPartNumber() const override;

	// Type de l'attribut partitionne (renvoie KWType::Continuous)
	int GetAttributeType() const override;

	// Index de la partie associee a une valeur (dernier index si valeur inconnue)
	int GetSymbolPartIndex(const Symbol& sValue) const override;

	// Libelle de la partie associee a un index de partie
	const ALString GetPartLabelAt(int nIndex) const override;

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile

	// Index correspondant a la valeur de l'ensemble specifie egale
	// a la valeur passe en parametre
	// Si valeur non trouvee, index de la StarValue si presente, ou 1 sinon
	int ComputeValueIndex(const Symbol& sValue) const;

	//////////////////////////////////////////////////////
	// Import/export avec une partition d'attribut

	// Import
	void ImportAttributeSymbolValues(const KWDGSAttributeSymbolValues* attributeSymbolValues);

	// Export
	// La regle source doit etre valide et compilee
	void ExportAttributeSymbolValues(KWDGSAttributeSymbolValues* attributeSymbolValues) const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Nettoyage de l'interface de base une fois la regle compilee
	void CleanCompiledBaseInterface() override;

	//////////////////////////////////////////////////////
	// Redefinition des methodes de structure

	// Recopie de la partie structure de la regle
	void CopyStructureFrom(const KWDerivationRule* kwdrSource) override;

	// Transfert de la specification de base de la regle source
	// vers la specification de structure de la regle en cours
	void BuildStructureFromBase(const KWDerivationRule* kwdrSource) override;

	// Verification de la partie structure de la regle
	boolean CheckStructureDefinition() const override;

	// Affichage, ecriture dans un fichier
	void WriteStructureUsedRule(ostream& ost) const override;

	// Methode de comparaison entre deux regles
	int FullCompare(const KWDerivationRule* rule) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation dynamique, lors du premier calcul d'attribut derive

	// Compilation dynamique
	void DynamicCompile() const;

	// Vecteur de toutes les modalites sources et de leur index de groupe correspondant
	// Les modalites sources sont triees pour permettre une recherche par dichotomie
	mutable SymbolVector svSortedValues;
	mutable IntVector ivValueIndexes;
	mutable int nStarValueIndex;

	// Fraicheur de compilation dynamique
	mutable int nDynamicCompileFreshness;

	// Valeurs
	SymbolVector svSymbolValueSet;
};

///////////////////////////////////////////////////////////////
// Classe KWDRInInterval
// Regle de derivation renvoyant 1 si une valeur Continuous
// appartient a un intervalle
// Premier operande: regle KWDRIntervalBounds (avec deux bornes)
// Deuxieme operande: valeur Continuous
class KWDRInInterval : public KWDerivationRule
{
public:
	// Constructeur
	KWDRInInterval();
	~KWDRInInterval();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant
	// pour premier argument une sous-classe de KWDRUnivariatePartition,
	// et pour second argument un type simple compatible avec le type de partition

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	// Verification que le premier operande est bien un intervalle avec deux bornes
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Bornes de l'intervalle
	Continuous cLowerBound;
	Continuous cUpperBound;
};

///////////////////////////////////////////////////////////////
// Classe KWDRInGroup
// Regle de derivation renvoyant 1 si une valeur Symbol
// appartient a un groupe de valeurs
// Premier operande: regle KWDRValueGroup
// Deuxieme operande: valeur Symbol
class KWDRInGroup : public KWDerivationRule
{
public:
	// Constructeur
	KWDRInGroup();
	~KWDRInGroup();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant
	// pour premier argument une sous-classe de KWDRUnivariatePartition,
	// et pour second argument un type simple compatible avec le type de partition

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;

	// Compilation
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Valeurs triees pour recherche dichotomique
	SymbolVector svSortedValues;
};

///////////////////////////////////////////////////////////////
// Classe KWDRPartId
// Regle de derivation virtuelle basee sur la partition d'un attribut,
// renvoyant un identifiant de partie
class KWDRPartId : public KWDerivationRule
{
public:
	// Constructeur
	KWDRPartId();
	~KWDRPartId();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant
	// pour premier argument une sous-classe de KWDRUnivariatePartition,
	// et pour second argument un type simple compatible avec le type de partition

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Compilation dynamique, lors du premier calcul d'attribut derive

	// Acces a la regle de partitionnement (referencee uniquement)
	mutable KWDRUnivariatePartition* univariatePartition;

	// Identifiant des parties
	mutable SymbolVector svPartIds;

	// Fraicheur de compilation dynamique
	mutable int nDynamicCompileFreshness;
};

///////////////////////////////////////////////////////////////
// Classe KWDRIntervalId
// Regle de derivation basee sur la discretisation d'un attribut
// de type Continuous, renvoyant un identifiant d'intervalle
// Premier operande: regle KWDRIntervalBounds
class KWDRIntervalId : public KWDRPartId
{
public:
	// Constructeur
	KWDRIntervalId();
	~KWDRIntervalId();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueId
// Regle de derivation basee sur le partitionnement d'un attribut
// en un ensemble de valeurs, renvoyant un identifiant de valeur
// Premier operande: regle KWDRContinuousValueSet
class KWDRContinuousValueId : public KWDRPartId
{
public:
	// Constructeur
	KWDRContinuousValueId();
	~KWDRContinuousValueId();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRGroupId
// Regle de derivation basee sur le groupement de valeurs d'un attribut
// de type Symbol, renvoyant un identifiant de groupe
// Premier operande: regle KWDRValueGroups
class KWDRGroupId : public KWDRPartId
{
public:
	// Constructeur
	KWDRGroupId();
	~KWDRGroupId();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueId
// Regle de derivation basee sur le partitionnement d'un attribut
// en un ensemble de valeurs, renvoyant un identifiant de valeur
// Premier operande: regle KWDRSymbolValueSet
class KWDRSymbolValueId : public KWDRPartId
{
public:
	// Constructeur
	KWDRSymbolValueId();
	~KWDRSymbolValueId();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRPartIndex
// Regle de derivation virtuelle basee sur la partition d'un attribut,
// renvoyant un index de partie (entre 0 et le nombre de parties -1)
class KWDRPartIndex : public KWDerivationRule
{
public:
	// Constructeur
	KWDRPartIndex();
	~KWDRPartIndex();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant
	// pour premier argument une sous-classe de KWDRUnivariatePartition,
	// et pour second argument un type simple compatible avec le type de partition

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	// Renvoie un index "externe" compris entre 1 et N (nombre de parties=
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRIntervalIndex
// Regle de derivation basee sur la discretisation d'un attribut
// de type Continuous, renvoyant un identifiant d'intervalle
// Premier operande: regle KWDRIntervalBounds
class KWDRIntervalIndex : public KWDRPartIndex
{
public:
	// Constructeur
	KWDRIntervalIndex();
	~KWDRIntervalIndex();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueIndex
// Regle de derivation basee sur le partitionnement d'un attribut
// en un ensemble de valeurs, renvoyant un identifiant de valeur
// Premier operande: regle KWDRContinuousValueSet
class KWDRContinuousValueIndex : public KWDRPartIndex
{
public:
	// Constructeur
	KWDRContinuousValueIndex();
	~KWDRContinuousValueIndex();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRGroupIndex
// Regle de derivation basee sur le groupement de valeurs d'un attribut
// de type Symbol, renvoyant un identifiant de groupe
// Premier operande: regle KWDRValueGroups
class KWDRGroupIndex : public KWDRPartIndex
{
public:
	// Constructeur
	KWDRGroupIndex();
	~KWDRGroupIndex();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueIndex
// Regle de derivation basee sur le partitionnement d'un attribut
// en un ensemble de valeurs, renvoyant un identifiant de valeur
// Premier operande: regle KWDRSymbolValueSet
class KWDRSymbolValueIndex : public KWDRPartIndex
{
public:
	// Constructeur
	KWDRSymbolValueIndex();
	~KWDRSymbolValueIndex();

	// Creation
	KWDerivationRule* Create() const override;
};

///////////////////////////////////////////////////////////
// Methodes en inline

inline const ALString KWDRUnivariatePartition::GetIdPrefix()
{
	return "I";
}

inline Symbol KWDRUnivariatePartition::ComputeId(int nIndex)
{
	require(0 <= nIndex);
	return (Symbol)(GetIdPrefix() + IntToString(nIndex + 1));
}
