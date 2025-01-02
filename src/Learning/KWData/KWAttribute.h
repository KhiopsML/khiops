// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWClass;
class KWAttribute;
class KWAttributeBlock;
class KWDerivationRule;

#include "Standard.h"
#include "ALString.h"
#include "KWSymbol.h"
#include "Ermgt.h"
#include "Object.h"
#include "Vector.h"
#include "KWType.h"
#include "KWMetaData.h"
#include "KWDataItem.h"
#include "KWLoadIndex.h"

//////////////////////////////
// Attribut d'un dictionnaire
class KWAttribute : public KWDataItem
{
public:
	// Constructeur et destructeur
	KWAttribute();
	~KWAttribute();

	// Type de DataItem
	boolean IsAttribute() const override;

	// Nom de l'attribut
	// Modification interdite si l'attribut est utilise dans une classe
	const ALString& GetName() const override;
	void SetName(const ALString& sValue);

	// Type de l'attribut
	// Par defaut: KWType::Unknown
	void SetType(int nValue);
	int GetType() const;

	// Classe de description des attributs de type objet
	KWClass* GetClass() const;
	void SetClass(KWClass* kwcValue);

	// Nom de la structure des attributs de type structure
	const ALString& GetStructureName() const;
	void SetStructureName(const ALString& sValue);

	// Utilisation des attributs de type objets par referencement (sinon: sous-partie)
	// Faux si pas de regle de derivation ou type non Object, sinon selon la regle
	boolean GetReference() const;

	// Regle de derivation associee(NULL si aucune)
	// Memoire: la regle de derivation appartient a l'appele
	void SetDerivationRule(KWDerivationRule* kwdrValue);
	KWDerivationRule* GetDerivationRule() const;

	// Supression de l'eventuelle regle de derivation, sans la detruire
	void RemoveDerivationRule();

	// Meta-donnees sous forme d'une ensemble de paires (cle, valeur)
	const KWMetaData* GetConstMetaData() const;
	KWMetaData* GetMetaData();

	// Cle de meta-donnee predefinie pour stocker le format des types complexes: Date, Time; Timestamp et
	// TimestampTZ Les attributs de type complexe sont selon les formats par defaut des types correspondant. On peut
	// cependant sppecifier un format specifique au moyen d'une meta donnee (par exemple: <DateFormat="DDMMYYYY").
	// Dans ce cas, les lectures/ecritures dans les bases de donnees se feront au moyen du format specifie
	// Cette meta-donnee de format ne doit etre utilisee qu'avec un format valide, pour les attributs
	// du type correspondant
	static const ALString GetFormatMetaDataKey(int nComplexType);

	// Acces au format pour les type complexes
	// Acces uniquement pour le format du bon type, pour une classe compilee
	const KWDateFormat* GetDateFormat() const;
	const KWTimeFormat* GetTimeFormat() const;
	const KWTimestampFormat* GetTimestampFormat() const;
	const KWTimestampTZFormat* GetTimestampTZFormat() const;

	// Libelle
	const ALString& GetLabel() const;
	void SetLabel(const ALString& sValue);

	// Attribut effectivement utilise, chargeable en memoire
	// Un attribut non utilise devient non charge
	// Par defaut: true
	boolean GetUsed() const;
	void SetUsed(boolean bValue);

	// Attribut charge en memoire
	// Un attribut non utilise ne peut etre charge
	// Par defaut: true
	boolean GetLoaded() const;
	void SetLoaded(boolean bValue);

	// Index de chargement de l'attribut parmi les attributs charges en memoire
	// Permet l'acces a la valeur dans les KWObjets
	// Attention: toute modification de la classe englobante ou
	// d'un de ses attributs invalide la classe, qui doit
	// etre recompilee
	KWLoadIndex GetLoadIndex() const;

	// Acces a la classe ou l'attribut est defini (NULL si aucune)
	KWClass* GetParentClass() const;

	/////////////////////////////////////////
	// Lien enre attribut et bloc

	// Indique si l'attribut est dans un bloc
	boolean IsInBlock() const;

	// Indique si l'attribut est le premier ou le dernier d'un bloc
	// Permet de traiter les blocs un seule fois en conditionnant par ces tests
	boolean IsFirstInBlock() const;
	boolean IsLastInBlock() const;

	// Acces a l'eventuel bloc d'attribut (initialisation depuis KWClass)
	KWAttributeBlock* GetAttributeBlock() const;

	// Acces a la regle de derivation du bloc (si le bloc existe et sa regle existe)
	KWDerivationRule* GetBlockDerivationRule() const;

	// Acces a la regle de derivation de l'attribut ou du bloc
	KWDerivationRule* GetAnyDerivationRule() const;

	/////////////////////////////////////////
	// Services divers

	// Indique si une variable est native (stocke, non calcule, et hors bloc)
	boolean IsNative() const;

	// Cout d'une regle utilise pour la regularisation des regles de construction
	// Attention, les variable initiales ont un cout; mais pour les variable construites,
	// il ne faut pas oublier de positionner leur indicateur "Initial" a false
	// Usage avance; par defaut 0
	void SetCost(double dValue);
	double GetCost() const;

	// Cle de meta-donnee predefinie pour stocker le cout d'une variable <Cost="...">
	// permettant d'identifier chaque attribut d'un bloc
	static const ALString& GetCostMetaDataKey();

	// Acces au cout depuis les meta-donnees
	void SetMetaDataCost(double dValue);
	double GetMetaDataCost() const;

	// Completion eventuelle de l'attribut avec les informations de type de l'eventuelle regle utilisee
	void CompleteTypeInfo(KWClass* kwcOwnerClass);

	// Duplication
	// L'attribut duplique n'est pas rattache a une classe ni a un block
	KWAttribute* Clone() const;

	// Verification de l'integrite de l'attribut
	// Peu de controles si l'attribut ne fait pas partie d'une classe
	boolean Check() const override;

	// Compilation de l'attribut: regle de derivation et format pour les type complexes
	void Compile();

	// Verification de l'absence de cycle de derivation
	// On passe en parametre une liste d'attributs colore en Grey ou en Black
	// (cf. algo decrit dans l'implementation de KWClassDomain::Compile())
	// Prerequis: la classe doit etre compilee
	// Retourne true si pas de cycle, sinon false en emmetant des messages d'erreur
	boolean ContainsCycle(NumericKeyDictionary* nkdGreyAttributes, NumericKeyDictionary* nkdBlackAttributes) const;

	// Memoire utilisee par l'attribut
	longint GetUsedMemory() const override;

	// Cle de hashage de l'attribut et sa regle de derivation
	longint ComputeHashValue() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Ecriture du contenu d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON) override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation

	// Completion eventuelle de la regle avec les informations de type
	// en maintenant un dictionnaire d'attributs pour eviter les boucles
	void InternalCompleteTypeInfo(KWClass* kwcOwnerClass, NumericKeyDictionary* nkdCompletedAttributes);

protected:
	// Seule la KWClass englobante peut acceder au fonctionnalites internes
	friend class KWClass;

	// Les classe Object et KWMTDatabase ont acces a la methode suivante
	friend class KWObject;
	friend class KWMTDatabase;

	// Rang de l'attribut parmi les attributs charges en memoire, pour un attribut natif ou cree inutilise de type Relation
	KWLoadIndex GetInternalLoadIndex() const;

	// Construction de l'objet de format pour les type complexes
	// Nettoie la version precedente
	void BuildAdvancedTypeSpecification();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gestion des attributs Used mais pas Loaded, pour la lecture/ecriture de dictionnaire dans les fichiers
	// Permet de transferer cette information "privee", par exemple pour une tache parallele

	// Ecriture si necessaire des informations prives dans les meta-data (_NotLoaded)
	void WritePrivateMetaData(ostream& ost) const;

	// Lecture et prise en compte des informations privees depuis les meta-data et nettoyage de ceux-ci
	void ReadPrivateMetaData();

	// Bloc d'attribut eventuel auquel l'attribut appartient
	KWAttributeBlock* attributeBlock;

	// Specification avancee pour certains types d'attributs
	//   . format pour la conversion des types complexe
	// Un seul objet sert a stocker toutes ces specification avancee (gain memoire)
	union
	{
		KWDateFormat* dateFormat;
		KWTimeFormat* timeFormat;
		KWTimestampFormat* timestampFormat;
		KWTimestampTZFormat* timestampTZFormat;
		Object* genericSpecification;
	} advancedTypeSpecification;

	// Classe utilisant l'attribut
	KWClass* parentClass;

	// Position dans la liste des attributs de la classe mere
	POSITION listPosition;

	// Specifications de l'attribut
	KWCDUniqueString usName;
	KWCDUniqueString usLabel;
	KWMetaData metaData;
	KWClass* attributeClass;
	KWCDUniqueString usStructureName;
	KWDerivationRule* kwdrRule;
	double dCost;
	KWLoadIndex liLoadIndex;
	char cType;
	boolean bUsed;
	boolean bLoaded;
};

// Methode de comparaison base sur le nom de l'attribut
int KWAttributeCompareName(const void* elem1, const void* elem2);

// Methode de comparaison base sur le nom du bloc de l'attribut, puis de son nom
int KWAttributeCompareBlockName(const void* elem1, const void* elem2);

// Methode de comparaison base sur le nom de la classe contenant l'attribut puis celui de l'attribut
int KWAttributeCompareClassAndAttributeName(const void* elem1, const void* elem2);

// Methode de comparaison base sur la VarKey d'un attribut pour deux attribut d'un meme bloc
int KWAttributeCompareVarKey(const void* elem1, const void* elem2);

#include "KWDerivationRule.h"
#include "KWAttributeBlock.h"
#include "KWClass.h"

////////////////////////////////////////////////////////
// methodes en inline

inline boolean KWAttribute::IsAttribute() const
{
	return true;
}

inline const ALString& KWAttribute::GetName() const
{
	return usName.GetValue();
}

inline void KWAttribute::SetName(const ALString& sValue)
{
	require(parentClass == NULL);
	usName.SetValue(sValue);
}

inline void KWAttribute::SetType(int nValue)
{
	require(KWType::Check(nValue));

	cType = char(nValue);
	if (parentClass != NULL)
		parentClass->UpdateFreshness();
}

inline int KWAttribute::GetType() const
{
	return cType;
}

inline KWClass* KWAttribute::GetClass() const
{
	return attributeClass;
}

inline void KWAttribute::SetClass(KWClass* kwcValue)
{
	attributeClass = kwcValue;
	if (parentClass != NULL)
		parentClass->UpdateFreshness();
}

inline const ALString& KWAttribute::GetStructureName() const
{
	return usStructureName.GetValue();
}

inline void KWAttribute::SetStructureName(const ALString& sValue)
{
	usStructureName.SetValue(sValue);
}

inline boolean KWAttribute::GetReference() const
{
	KWDerivationRule* kwdrAnyRule;

	require(KWType::IsRelation(GetType()));

	kwdrAnyRule = GetAnyDerivationRule();
	if (kwdrAnyRule == NULL)
		return false;
	else
		return kwdrAnyRule->GetReference();
}

inline void KWAttribute::SetDerivationRule(KWDerivationRule* kwdrValue)
{
	if (kwdrRule != NULL)
		delete kwdrRule;
	kwdrRule = kwdrValue;
	if (parentClass != NULL)
		parentClass->UpdateFreshness();
}

inline KWDerivationRule* KWAttribute::GetDerivationRule() const
{
	return kwdrRule;
}

inline void KWAttribute::RemoveDerivationRule()
{
	kwdrRule = NULL;
	if (parentClass != NULL)
		parentClass->UpdateFreshness();
}

inline const KWMetaData* KWAttribute::GetConstMetaData() const
{
	return &metaData;
}

inline KWMetaData* KWAttribute::GetMetaData()
{
	return &metaData;
}

inline const KWDateFormat* KWAttribute::GetDateFormat() const
{
	require(GetType() == KWType::Date and advancedTypeSpecification.dateFormat != NULL);
	return advancedTypeSpecification.dateFormat;
}

inline const KWTimeFormat* KWAttribute::GetTimeFormat() const
{
	require(GetType() == KWType::Time and advancedTypeSpecification.timeFormat != NULL);
	return advancedTypeSpecification.timeFormat;
}

inline const KWTimestampFormat* KWAttribute::GetTimestampFormat() const
{
	require(GetType() == KWType::Timestamp and advancedTypeSpecification.timestampFormat != NULL);
	return advancedTypeSpecification.timestampFormat;
}

inline const KWTimestampTZFormat* KWAttribute::GetTimestampTZFormat() const
{
	require(GetType() == KWType::TimestampTZ and advancedTypeSpecification.timestampTZFormat != NULL);
	return advancedTypeSpecification.timestampTZFormat;
}

inline const ALString& KWAttribute::GetLabel() const
{
	return usLabel.GetValue();
}

inline void KWAttribute::SetLabel(const ALString& sValue)
{
	usLabel.SetValue(sValue);
}

inline boolean KWAttribute::GetUsed() const
{
	return bUsed;
}

inline void KWAttribute::SetUsed(boolean bValue)
{
	bUsed = bValue;
	if (not bUsed)
		bLoaded = false;
	if (parentClass != NULL)
		parentClass->UpdateFreshness();
}

inline boolean KWAttribute::GetLoaded() const
{
	ensure(not bLoaded or bUsed);
	return bLoaded;
}

inline void KWAttribute::SetLoaded(boolean bValue)
{
	if (bUsed)
		bLoaded = bValue;
	if (parentClass != NULL)
		parentClass->UpdateFreshness();
}

inline KWLoadIndex KWAttribute::GetLoadIndex() const
{
	require(parentClass != NULL);
	require(parentClass->IsIndexed());
	ensure(not GetLoaded() or liLoadIndex.IsValid());
	ensure(not GetLoaded() or (0 <= liLoadIndex.GetDenseIndex() and
				   liLoadIndex.GetDenseIndex() < parentClass->GetLoadedDataItemNumber()));
	ensure(not GetLoaded() or liLoadIndex.IsDense() or
	       (0 <= liLoadIndex.GetSparseIndex() and
		liLoadIndex.GetSparseIndex() <
		    parentClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetLoadedAttributeNumber()));
	return liLoadIndex;
}

inline KWLoadIndex KWAttribute::GetInternalLoadIndex() const
{
	require(parentClass != NULL);
	require(parentClass->IsIndexed());
	require(KWType::IsRelation(GetType()));
	require(not GetReference());
	require(not GetLoaded());
	ensure(parentClass->GetLoadedDataItemNumber() <= liLoadIndex.GetDenseIndex() and
	       liLoadIndex.GetDenseIndex() < parentClass->GetTotalInternallyLoadedDataItemNumber());
	return liLoadIndex;
}

inline KWClass* KWAttribute::GetParentClass() const
{
	return parentClass;
}

inline boolean KWAttribute::IsInBlock() const
{
	return (attributeBlock != NULL);
}

inline boolean KWAttribute::IsFirstInBlock() const
{
	return (attributeBlock != NULL and attributeBlock->GetFirstAttribute() == this);
}

inline boolean KWAttribute::IsLastInBlock() const
{
	return (attributeBlock != NULL and attributeBlock->GetLastAttribute() == this);
}

inline KWAttributeBlock* KWAttribute::GetAttributeBlock() const
{
	return attributeBlock;
}

inline KWDerivationRule* KWAttribute::GetBlockDerivationRule() const
{
	if (attributeBlock == NULL)
		return NULL;
	else
		return attributeBlock->GetDerivationRule();
}

inline KWDerivationRule* KWAttribute::GetAnyDerivationRule() const
{
	if (kwdrRule != NULL)
		return GetDerivationRule();
	else
		return GetBlockDerivationRule();
}

inline boolean KWAttribute::IsNative() const
{
	return KWType::IsStored(GetType()) and GetAnyDerivationRule() == NULL;
}

inline void KWAttribute::SetCost(double dValue)
{
	require(dValue >= 0);
	dCost = dValue;
}

inline double KWAttribute::GetCost() const
{
	return dCost;
}
