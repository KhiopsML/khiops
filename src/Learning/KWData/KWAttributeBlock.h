// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWClass;
class KWAttribute;
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
#include "KWIndexedKeyBlock.h"

///////////////////////////////////////////////////////////////////////////////
// Classe KWAttributeBlock
// Bloc d'atttributs d'un dictionnaire
// Permet de specifier un sous-ensemble d'attributs contigus d'un dictionnaire
// de meme type simple (Symbol, Continuous ou ObjectArray), produit par une regle de derivation,
// et pouvant etre stocke de facon efficace dans le cas sparse
class KWAttributeBlock : public KWDataItem
{
public:
	// Constructeur et destructeur
	KWAttributeBlock();
	~KWAttributeBlock();

	// Type de DataItem
	boolean IsAttribute() const override;

	// Nom du bloc d'attribut
	// Modification interdite si le bloc d'attribut est utilise dans une classe
	const ALString& GetName() const override;
	void SetName(const ALString& sValue);

	// Regle de derivation associee(NULL si aucune)
	// Memoire: la regle de derivation appartient a l'appele
	void SetDerivationRule(KWDerivationRule* kwdrValue);
	KWDerivationRule* GetDerivationRule() const;

	// Supression de l'eventuelle regle de derivation, sans la detruire
	void RemoveDerivationRule();

	// Indique si un bloc est natif (non calcule)
	boolean IsNative() const;

	// Meta-donnees sous forme d'une ensemble de paires (cle, valeur)
	const KWMetaData* GetConstMetaData() const;
	KWMetaData* GetMetaData();

	// Acces a la valeur par defaut pour les types bloc d'attributs
	// Acces uniquement pour la valeur par defaut du bon type, pour une classe compilee
	// La valeur par defaut est necessairement NULL dans les cas des blocs de type ObjectArrayValueBlock
	// Le bloc doit etre compile pour actualiser cette valeur par defaut
	Continuous GetContinuousDefaultValue() const;
	Symbol& GetSymbolDefaultValue() const;

	// Import des meta-donne d'un bloc source
	// Creation si necessaire d'une meta-donnee de valeur par defaut a partir d'un bloc source
	// Cela n'est necessaire que si le bloc source est calcule, avec une regle ayant une valeur par defaut non
	// standard
	void ImportMetaDataFrom(const KWAttributeBlock* sourceAttributeBlock);

	// Libelle
	// Fin de ligne prefixee par '//' suivant la declaration du bloc d'attributs dans le fichier dictionnaire
	const ALString& GetLabel() const;
	void SetLabel(const ALString& sValue);

	// Commentaires
	// Ensemble des lignes prefixees par '//' precedant le debut du bloc d'attributs '{' dans le fichier dictionnaire
	const StringVector* GetComments() const;
	void SetComments(const StringVector* svValue);

	// Commentaires internes
	// Ensemble des lignes prefixees par '//' precedant la fin du bloc d'attributs '}' dans le fichier dictionnaire
	const StringVector* GetInternalComments() const;
	void SetInternalComments(const StringVector* svValue);

	/////////////////////////////////////////////////////////
	// Informations de specification du bloc, suite a sa
	// creation depuis la classe englobante

	// Attributs extremites du bloc dans la classe
	// Le parcours de tous les attributs d'un bloc se fait par exemple de la facon suivante
	//
	//    attribute = attributeBlock->GetFirstAttribute();
	//    while (attribute != NULL)
	//    {
	//    		// Traitement sur l'attribut en cours
	//
	//    	    // Arret si fin du bloc
	//    		if (attribute == attributeBlock->GetLastAttribute())
	//    			break;
	//    		attributeBlock->GetParentClass()->GetNextAttribute(attribute);
	//    	}
	KWAttribute* GetFirstAttribute() const;
	KWAttribute* GetLastAttribute() const;

	// Calcul du nombre total d'attributs du bloc, qui n'a pas besoin d'etre indexe
	// a la difference du cas de la methode GetAttributeNumber
	int ComputeAttributeNumber() const;

	// Type de block
	int GetBlockType() const;

	// Type des attributs du bloc
	int GetType() const;

	// Classe de description des attributs de type objet
	KWClass* GetClass() const;

	// Acces a la classe ou le bloc est defini (NULL si aucune)
	KWClass* GetParentClass() const;

	/////////////////////////////////////////////////////////
	// Informations sur les attributs du bloc, disponibles
	// apres l'indexation de la classe et deduit des attributs du bloc

	// Bloc utilise
	boolean GetUsed() const;

	// Bloc charge en memoire
	boolean GetLoaded() const;

	// Index de chargement du bloc dans la classe parmi les attributs charges en memoire
	// Permet l'acces a la valeur du bloc dans les KWObjets
	KWLoadIndex GetLoadIndex() const;

	// Attributs charges en memoire (tous sparse)
	// Attention: les attributs charges ne sont pas necessairement dans le meme ordre que les attributs de la classe
	int GetLoadedAttributeNumber() const;
	KWAttribute* GetLoadedAttributeAt(int nIndex) const;

	// Index d'un attribut charge en memoire selon l'ordre des attributs dans le bloc, selon son index pparse dans le bloc
	int GetLoadedAttributeIndexAtSparseIndex(int nSparseIndex) const;

	// Ensemble des cles d'attribut (VarKey) pour les attributs du blocs charges en memoire
	// A chaque cle correspond l'index sparse de l'attribut dans son bloc
	const KWIndexedKeyBlock* GetLoadedAttributesIndexedKeyBlock() const;

	// Nombre d'attributs total du bloc
	int GetAttributeNumber() const;

	// Estimation heuristique du nombre de valeurs moyen effectivement dans un bloc
	// En effet, les bloc sont souvent sparse, et contiennent moins de valeur
	// que ce qui est possible, ce d'autant plus que les blocs sont de grande tailles
	// Pour chaque bloc de K variable, l'estimation du nombre de valeur V est V = sqrt(K log2(K))
	static int GetEstimatedMeanValueNumber(int nBlockAttributeNumber);

	/////////////////////////////////////////
	// Service lies aux VarKey

	// Type de cle utilisee pour les variables du bloc
	// Valeurs possibles:
	//   KWType::Symbol: cles categorielles (ex: <VarKey="dix">, <VarKey="1">)
	//   KWType::Continuous: cles numeriques entieres, plus grande que 1 (ex: <VarKey=1>, <VarKey=10>)
	//   KWType::None: a l'initialisation, avant que la valeur soit specifiee
	// Le type de cle doit etre initialise a la creation du bloc, et ne doit l'etre qu'une seule fois
	void SetVarKeyType(int nValue);
	int GetVarKeyType() const;

	// VarKey d'un attribut du bloc selon le type de VarKey
	int GetContinuousVarKey(const KWAttribute* attribute) const;
	Symbol GetSymbolVarKey(const KWAttribute* attribute) const;

	// VarKey d'un attribut du bloc, sous un format generique
	const ALString GetStringVarKey(const KWAttribute* attribute) const;

	// Recherche d'un attribut du bloc par VarKey, pour un bloc d'une classe indexee
	KWAttribute* LookupAttributeByContinuousVarKey(int nVarKey) const;
	KWAttribute* LookupAttributeBySymbolVarKey(Symbol sVarKey) const;

	// Construction d'un bloc de cle pour tous les attributs du bloc, que la classe soit indexee ou non
	// Seules les cles valides sont memorisees, avec les attributs associes a leur cles dans le dictionnaire
	// en parametres (en prenant la cle telle quelle dans le cas numerique, par sa NumericKey sinon)
	// Utile pour les check avant indexation de la classe
	// Memoire: le bloc de cle en retour appartient a l'appelant
	KWIndexedKeyBlock* BuildAttributesIndexedKeyBlock(NumericKeyDictionary* nkdBlockAttributes) const;

	/////////////////////////////////////////
	// Services divers

	// Tri de tous les attributs du bloc par VarKey
	void SortAttributesByVarKey();

	// Cle de meta-donnee predefinie pour stocker la cle d'attribut <VarKey="...">
	// permettant d'identifier chaque attribut d'un bloc
	static const ALString& GetAttributeKeyMetaDataKey();

	// Cle de meta-donnee predefinie pour stocker la valeur par defaut des bloc
	// quand ceux-ci sont stocke, sans regle de derivation
	static const ALString& GetDefaultValueMetaDataKey(int nType);

	// Verification de l'integrite du bloc
	// Peu de controles si le bloc ne fait pas partie d'une classe
	boolean Check() const override;

	// Compilation du bloc d'attribut: regle de derivation et valeur par defaut
	void Compile();

	// Verification de l'absence de cycle de derivation
	// On passe en parametre une liste d'attributs colore en Grey ou en Black
	// (cf. algo decrit dans l'implementation de KWClassDomain::Compile())
	// Prerequis: la classe doit etre compilee
	// Retourne true si pas de cycle, sinon false en emmetant des messages d'erreur
	boolean ContainsCycle(NumericKeyDictionary* nkdGreyAttributes, NumericKeyDictionary* nkdBlackAttributes) const;

	// Memoire utilisee par le bloc d'attributs
	longint GetUsedMemory() const override;

	// Cle de hashage de l'attribut et sa regle de derivation
	longint ComputeHashValue() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Ecriture du contenu d'un rapport JSON
	void WriteJSONFields(JSONFile* fJSON) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Seule la KWClass englobante peut acceder aux fonctionnalites internes
	friend class KWClass;

	// Construction de l'objet valeur par defaut
	// Nettoie la version precedente
	void BuildAdvancedTypeSpecification();

	/////////////////////////////////////////
	// Service avances lies aux VarKey
	// A n'utiliser que pour se servir d'un bloc pour gerer temporairement les attributs par VarKey,
	// hors indexation de la classe
	friend class KDClassBuilder;

	// Memorisation d'un attribut par VarKey
	void InternalIndexAttributeByContinuousVarKey(int nVarKey, KWAttribute* attribute);
	void InternalIndexAttributeBySymbolVarKey(Symbol sVarKey, KWAttribute* attribute);

	// Recherche d'un attribut du bloc par VarKey
	KWAttribute* InternalLookupAttributeByContinuousVarKey(int nVarKey) const;
	KWAttribute* InternalLookupAttributeBySymbolVarKey(Symbol sVarKey) const;

	//////////////////////////////////////////////////
	// Service avances lies a la mutation d'un objet
	friend class KWObject;
	friend class KWDatabase;
	friend class KWDataTableSliceSet;

	// Calcul des index de mutation des attributs charges du bloc
	// Le bloc courant est cense etre inclus dans le bloc cible, et on calcul ici un vecteur de correspondance
	// entre les index sparse du bloc courant et ceux du bloc cible
	void ComputeLoadedAttributeMutationIndexes(const KWAttributeBlock* targetAttributeBlock) const;

	// Acces aux index de mutation des attributs charges du bloc
	// Renvoie NULL s'il n'y a pas de mutation a effectuer
	IntVector* GetLoadedAttributeMutationIndexes() const;

	////////////////////////////////////////////////
	// Variable de gestion d'un bloc

	// Specifications du bloc
	KWCDUniqueString usName;
	KWCDUniqueString usLabel;
	StringVector svComments;
	StringVector svInternalComments;
	KWDerivationRule* kwdrRule;
	KWMetaData metaData;

	// Specification avancee pour certains types d'attributs
	// Valeur par defaut pour les types ContinuousValueBlock et SymbolValueBlock
	// On utilise une valeur pour les Symbol et pour les Continuous, plutot qu'une KWValue
	// pour generer simplement les blocs changeant de type ou devanant vide, en ce qui
	// concerne la gestion automatique des Symbol
	mutable Symbol sBlockDefaultValue;
	Continuous cBlockDefaultValue;

	// Premier et dernier attribut du bloc dans la classe
	KWAttribute* firstAttribute;
	KWAttribute* lastAttribute;

	// Attributs calcules apres indexation de la classe
	KWLoadIndex liLoadIndex;
	ObjectArray oaLoadedAttributes;
	KWIndexedKeyBlock* loadedAttributesIndexedKeyBlock;
	int nAttributeNumber;
	NumericKeyDictionary nkdAttributesByVarKeys;
	IntVector ivLoadedAttributeIndexesBySparseIndex;

	// Gestion des index de mutation du bloc vers un bloc cible constituant un sous-bloc
	// Utilise pour la mutation des objets lors de la lecture d'une base
	// A chaque index sparse d'un attribut de bloc, correspondant soit l'index sparse
	// de l'attribut destination (-1 si absent ou non charge)
	// Le vecteur vaut NULL si les blocs source et destination coincide
	mutable IntVector* ivLoadedAttributeMutationIndexes;

	// Flag d'utilisation et de chargement
	boolean bUsed;
	boolean bLoaded;
};

#include "KWDerivationRule.h"
#include "KWClass.h"

////////////////////////////////////////////////////////
// methodes en inline

inline boolean KWAttributeBlock::IsAttribute() const
{
	return false;
}

inline const ALString& KWAttributeBlock::GetName() const
{
	return usName.GetValue();
}

inline void KWAttributeBlock::SetName(const ALString& sValue)
{
	require(GetParentClass() == NULL);
	usName.SetValue(sValue);
}

inline const KWMetaData* KWAttributeBlock::GetConstMetaData() const
{
	return &metaData;
}

inline KWMetaData* KWAttributeBlock::GetMetaData()
{
	return &metaData;
}

inline void KWAttributeBlock::SetDerivationRule(KWDerivationRule* kwdrValue)
{
	if (kwdrRule != NULL)
		delete kwdrRule;
	kwdrRule = kwdrValue;
	if (GetParentClass() != NULL)
		GetParentClass()->UpdateFreshness();
}

inline KWDerivationRule* KWAttributeBlock::GetDerivationRule() const
{
	return kwdrRule;
}

inline void KWAttributeBlock::RemoveDerivationRule()
{
	kwdrRule = NULL;
	if (GetParentClass() != NULL)
		GetParentClass()->UpdateFreshness();
}

inline boolean KWAttributeBlock::IsNative() const
{
	return kwdrRule == NULL;
}

inline Continuous KWAttributeBlock::GetContinuousDefaultValue() const
{
	require(GetType() == KWType::Continuous);
	return cBlockDefaultValue;
}

inline Symbol& KWAttributeBlock::GetSymbolDefaultValue() const
{
	require(GetType() == KWType::Symbol);
	return sBlockDefaultValue;
}

inline const ALString& KWAttributeBlock::GetLabel() const
{
	return usLabel.GetValue();
}

inline void KWAttributeBlock::SetLabel(const ALString& sValue)
{
	usLabel.SetValue(sValue);
}

inline const StringVector* KWAttributeBlock::GetComments() const
{
	return &svComments;
}

inline void KWAttributeBlock::SetComments(const StringVector* svValue)
{
	svComments.CopyFrom(svValue);
}

inline const StringVector* KWAttributeBlock::GetInternalComments() const
{
	return &svInternalComments;
}

inline void KWAttributeBlock::SetInternalComments(const StringVector* svValue)
{
	svInternalComments.CopyFrom(svValue);
}

inline KWAttribute* KWAttributeBlock::GetFirstAttribute() const
{
	return firstAttribute;
}

inline KWAttribute* KWAttributeBlock::GetLastAttribute() const
{
	return lastAttribute;
}

inline int KWAttributeBlock::GetBlockType() const
{
	return KWType::GetValueBlockType(GetType());
}

inline int KWAttributeBlock::GetType() const
{
	if (firstAttribute == NULL)
		return KWType::Unknown;
	else
		return firstAttribute->GetType();
}

inline KWClass* KWAttributeBlock::GetClass() const
{
	if (firstAttribute == NULL)
		return NULL;
	else
		return firstAttribute->GetClass();
}

inline KWClass* KWAttributeBlock::GetParentClass() const
{
	if (firstAttribute == NULL)
		return NULL;
	else
		return firstAttribute->GetParentClass();
}

inline boolean KWAttributeBlock::GetLoaded() const
{
	require(GetParentClass()->IsIndexed());
	return bLoaded;
}

inline boolean KWAttributeBlock::GetUsed() const
{
	require(GetParentClass()->IsIndexed());
	return bUsed;
}

inline KWLoadIndex KWAttributeBlock::GetLoadIndex() const
{
	require(GetParentClass()->IsIndexed());
	return liLoadIndex;
}

inline int KWAttributeBlock::GetLoadedAttributeNumber() const
{
	require(GetParentClass()->IsIndexed());
	return oaLoadedAttributes.GetSize();
}

inline KWAttribute* KWAttributeBlock::GetLoadedAttributeAt(int nIndex) const
{
	require(GetParentClass()->IsIndexed());
	require(cast(KWAttribute*, oaLoadedAttributes.GetAt(nIndex))->GetLoadIndex().GetSparseIndex() == nIndex);
	return cast(KWAttribute*, oaLoadedAttributes.GetAt(nIndex));
}

inline int KWAttributeBlock::GetLoadedAttributeIndexAtSparseIndex(int nSparseIndex) const
{
	require(GetParentClass()->IsIndexed());
	require(0 <= nSparseIndex and nSparseIndex < GetLoadedAttributeNumber());
	return ivLoadedAttributeIndexesBySparseIndex.GetAt(nSparseIndex);
}

inline const KWIndexedKeyBlock* KWAttributeBlock::GetLoadedAttributesIndexedKeyBlock() const
{
	require(GetParentClass()->IsIndexed());
	require(GetVarKeyType() != KWType::None);
	ensure(oaLoadedAttributes.GetSize() == loadedAttributesIndexedKeyBlock->GetKeyNumber());
	return loadedAttributesIndexedKeyBlock;
}

inline int KWAttributeBlock::GetAttributeNumber() const
{
	require(GetParentClass()->IsIndexed());
	return nAttributeNumber;
}

inline int KWAttributeBlock::GetEstimatedMeanValueNumber(int nBlockAttributeNumber)
{
	require(nBlockAttributeNumber >= 0);
	return (int)ceil(sqrt(nBlockAttributeNumber * log(nBlockAttributeNumber + 1.0) / log(2.0)));
}

inline int KWAttributeBlock::GetVarKeyType() const
{
	require(loadedAttributesIndexedKeyBlock != NULL);
	return loadedAttributesIndexedKeyBlock->GetVarKeyType();
}

inline int KWAttributeBlock::GetContinuousVarKey(const KWAttribute* attribute) const
{
	int nVarKey;

	require(attribute != NULL);
	require(attribute->GetAttributeBlock() == this);
	require(GetVarKeyType() == KWType::Continuous);

	nVarKey = int(floor(0.5 + attribute->GetConstMetaData()->GetDoubleValueAt(GetAttributeKeyMetaDataKey())));
	return nVarKey;
}

inline Symbol KWAttributeBlock::GetSymbolVarKey(const KWAttribute* attribute) const
{
	Symbol sVarKey;

	require(attribute != NULL);
	require(attribute->GetAttributeBlock() == this);
	require(GetVarKeyType() == KWType::Symbol);

	sVarKey = Symbol(attribute->GetConstMetaData()->GetStringValueAt(GetAttributeKeyMetaDataKey()));
	return sVarKey;
}

inline const ALString KWAttributeBlock::GetStringVarKey(const KWAttribute* attribute) const
{
	ALString sStringVarKey;

	require(attribute != NULL);
	require(attribute->GetAttributeBlock() == this);
	sStringVarKey = attribute->GetConstMetaData()->GetExternalValueAt(GetAttributeKeyMetaDataKey());
	return sStringVarKey;
}

inline KWAttribute* KWAttributeBlock::LookupAttributeByContinuousVarKey(int nVarKey) const
{
	KWAttribute* attribute;

	require(GetParentClass()->IsIndexed());
	require(GetVarKeyType() == KWType::Continuous);

	attribute = cast(KWAttribute*, nkdAttributesByVarKeys.Lookup(nVarKey));
	return attribute;
}

inline KWAttribute* KWAttributeBlock::LookupAttributeBySymbolVarKey(Symbol sVarKey) const
{
	KWAttribute* attribute;

	require(GetParentClass()->IsIndexed());
	require(GetVarKeyType() == KWType::Symbol);

	attribute = cast(KWAttribute*, nkdAttributesByVarKeys.Lookup(sVarKey.GetNumericKey()));
	return attribute;
}

inline void KWAttributeBlock::InternalIndexAttributeByContinuousVarKey(int nVarKey, KWAttribute* attribute)
{
	require(attribute != NULL);
	require(attribute->GetAttributeBlock() == this);
	nkdAttributesByVarKeys.SetAt(nVarKey, attribute);
}

inline void KWAttributeBlock::InternalIndexAttributeBySymbolVarKey(Symbol sVarKey, KWAttribute* attribute)
{
	require(attribute != NULL);
	require(attribute->GetAttributeBlock() == this);
	nkdAttributesByVarKeys.SetAt(sVarKey.GetNumericKey(), attribute);
}

inline KWAttribute* KWAttributeBlock::InternalLookupAttributeByContinuousVarKey(int nVarKey) const
{
	KWAttribute* attribute;

	require(GetVarKeyType() == KWType::Continuous);

	attribute = cast(KWAttribute*, nkdAttributesByVarKeys.Lookup(nVarKey));
	return attribute;
}

inline KWAttribute* KWAttributeBlock::InternalLookupAttributeBySymbolVarKey(Symbol sVarKey) const
{
	KWAttribute* attribute;

	require(GetVarKeyType() == KWType::Symbol);

	attribute = cast(KWAttribute*, nkdAttributesByVarKeys.Lookup(sVarKey.GetNumericKey()));
	return attribute;
}

inline IntVector* KWAttributeBlock::GetLoadedAttributeMutationIndexes() const
{
	require(GetParentClass()->IsCompiled());
	ensure(ivLoadedAttributeMutationIndexes == NULL or
	       ivLoadedAttributeMutationIndexes->GetSize() == GetLoadedAttributeNumber());
	return ivLoadedAttributeMutationIndexes;
}
