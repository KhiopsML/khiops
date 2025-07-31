// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWClass;
class KWAttribute;
class KWAttributeBlock;
class KWObject;
class KWClassDomain;
class KWDerivationRule;
class KWContinuousValueBlock;
class KWSymbolValueBlock;
class KWObjectArrayValueBlock;
class KWObjectDataPath;

#include "Standard.h"
#include "ALString.h"
#include "KWSymbol.h"
#include "Ermgt.h"
#include "Object.h"
#include "KWType.h"
#include "KWLoadIndex.h"
#include "KWValueBlock.h"
#include "KWDatabaseMemoryGuard.h"

/////////////////////////////////////////////////////////////////////////////
// Structure de donnees pour les connaissances
//
// Un KWObject permet de gerer les valeurs des attributs (Load) d'une KWClass.
// Les acces aux valeurs sont pilotes par la structure de la classe, et sont
// accessible soit de facon type (Symbol, Continuous, Object, ObjectArray, Structure),
// soit de facon generique.
// L'acces a ces attributs se fait en lecture/ecriture par index
// La description de cette composition provient d'une KWClass,
// connue du KWObject. Les attributs d'un KWObject sont les attributs
// charges en memoire (Loaded) d'une KWClass, et leur index est donne par
// la methode GetLoadIndex de KWAttribute.
//
// MEMORY
// Un KWObject contient et gere sa description (a savoir ses containers
// d'attributs). Les KWObject references sont soit consideres comme
// des sous-parties (GetReference=false dans les KWAttribute), auquel cas
// ils sont detruits avec leur proprietaire, soit comme des appels
// independants (GetReference=true dans les KWAttribute), auquel cas
// ils ne sont detruits avec le KWObject referencant.
// Un appel vers un objet inclu ne doit concerner qu'une autre partie du
// meme KWObject englobant, ce qui garantit que la destruction du KWObject
// radical englobant detruit bien tous les KWObjects inclus (et appeles).
// Attention, la destruction de la KWClass doit intervenir apres la
// destruction du dernier KWObject concerne.
//
// Les attributs de type Structure sont consideres comme appartenant
// a la KWClass et references par le KWObject.
// Ils ne sont detruits par leur KWObject.
//
// Les Symbols sont centralises dans un dictionnaire, ce qui assure leur
// unicite, et permet leur comparaison tres rapide (comparaison de
// pointeurs). A la mise a jour d'un attribut de type Symbol, il y a
// si necessaire allocation d'un Symbol pour mettre a jour l'attribut.
// A la consultation, le Symbol est rendu directement: attention a ne pas
// le liberer.
/////////////////////////////////////////////////////////////////////////////
class KWObject : public Object
{
public:
	// Construction et initialisation de l'objet avec des valeurs par defaut
	// L'index (> 0) permet de memoriser un numero de ligne lors de la lecture d'un fichier
	KWObject(const KWClass* kwcNew, longint lIndex);

	// Destructeur
	~KWObject();

	// Acces a la KWClass
	const KWClass* GetClass() const;

	// Index de creation de l'objet
	longint GetCreationIndex() const;

	// (Re)Initialisation de l'objet a partir de la structure de la classe
	void Init();

	///////////////////////////////////////////////////////////////////////
	// Acces aux valeurs des attributs
	// Les attributs derives peuvent etre accedes soit par des methodes
	// de type Compute*ValueAt, ce qui force leur calcul (une seule fois,
	// car calcul bufferise), soit directement par une methode de type
	// Get*ValueAt beaucoup plus efficace (dans ce cas, l'attribut derive
	// doit imperativement avoir ete calcule auparavent)
	// Les attributs non derives sont modifiables en ecriture par des methodes
	// de type Set*ValueAt, mais cela est en reserve aux methodes d'entre-sorties

	/// Gestion des valeurs de type Continuous
	Continuous ComputeContinuousValueAt(KWLoadIndex liLoadIndex) const;
	Continuous GetContinuousValueAt(KWLoadIndex liLoadIndex) const;
	void SetContinuousValueAt(KWLoadIndex liLoadIndex, Continuous cValue);

	/// Gestion des valeurs de type Symbol
	Symbol& ComputeSymbolValueAt(KWLoadIndex liLoadIndex) const;
	Symbol& GetSymbolValueAt(KWLoadIndex liLoadIndex) const;
	void SetSymbolValueAt(KWLoadIndex liLoadIndex, const Symbol& sValue);

	/// Gestion des valeurs de type Date
	Date ComputeDateValueAt(KWLoadIndex liLoadIndex) const;
	Date GetDateValueAt(KWLoadIndex liLoadIndex) const;
	void SetDateValueAt(KWLoadIndex liLoadIndex, Date dValue);

	/// Gestion des valeurs de type Time
	Time ComputeTimeValueAt(KWLoadIndex liLoadIndex) const;
	Time GetTimeValueAt(KWLoadIndex liLoadIndex) const;
	void SetTimeValueAt(KWLoadIndex liLoadIndex, Time tValue);

	/// Gestion des valeurs de type Timestamp
	Timestamp ComputeTimestampValueAt(KWLoadIndex liLoadIndex) const;
	Timestamp GetTimestampValueAt(KWLoadIndex liLoadIndex) const;
	void SetTimestampValueAt(KWLoadIndex liLoadIndex, Timestamp tsValue);

	/// Gestion des valeurs de type TimestampTS
	TimestampTZ ComputeTimestampTZValueAt(KWLoadIndex liLoadIndex) const;
	TimestampTZ GetTimestampTZValueAt(KWLoadIndex liLoadIndex) const;
	void SetTimestampTZValueAt(KWLoadIndex liLoadIndex, TimestampTZ tstzValue);

	/// Gestion des valeurs de type Text
	const Symbol& ComputeTextValueAt(KWLoadIndex liLoadIndex) const;
	const Symbol& GetTextValueAt(KWLoadIndex liLoadIndex) const;
	void SetTextValueAt(KWLoadIndex liLoadIndex, const Symbol& sValue);

	/// Gestion des valeurs de type TextList
	SymbolVector* ComputeTextListValueAt(KWLoadIndex liLoadIndex) const;
	SymbolVector* GetTextListValueAt(KWLoadIndex liLoadIndex) const;
	void SetTextListValueAt(KWLoadIndex liLoadIndex, SymbolVector* svValue);

	/// Gestion des valeurs de type Object (KWObject*)
	KWObject* ComputeObjectValueAt(KWLoadIndex liLoadIndex) const;
	KWObject* GetObjectValueAt(KWLoadIndex liLoadIndex) const;
	void SetObjectValueAt(KWLoadIndex liLoadIndex, KWObject* kwoValue);

	/// Gestion des valeurs de type ObjectArray (tableau de KWObject*)
	ObjectArray* ComputeObjectArrayValueAt(KWLoadIndex liLoadIndex) const;
	ObjectArray* GetObjectArrayValueAt(KWLoadIndex liLoadIndex) const;
	void SetObjectArrayValueAt(KWLoadIndex liLoadIndex, ObjectArray* oaValue);

	/// Gestion des valeurs de type Structure (Object*)
	Object* ComputeStructureValueAt(KWLoadIndex liLoadIndex) const;
	Object* GetStructureValueAt(KWLoadIndex liLoadIndex) const;
	void SetStructureValueAt(KWLoadIndex liLoadIndex, Object* kwoValue);

	/// Gestion des valeurs de type ContinuousValueBlock
	KWContinuousValueBlock* ComputeContinuousValueBlockAt(KWLoadIndex liLoadIndex) const;
	KWContinuousValueBlock* GetContinuousValueBlockAt(KWLoadIndex liLoadIndex) const;
	void SetContinuousValueBlockAt(KWLoadIndex liLoadIndex, KWContinuousValueBlock* cvbValue);
	void UpdateContinuousValueBlockAt(KWLoadIndex liLoadIndex, KWContinuousValueBlock* cvbValue);

	/// Gestion des valeurs de type SymbolValueBlock
	KWSymbolValueBlock* ComputeSymbolValueBlockAt(KWLoadIndex liLoadIndex) const;
	KWSymbolValueBlock* GetSymbolValueBlockAt(KWLoadIndex liLoadIndex) const;
	void SetSymbolValueBlockAt(KWLoadIndex liLoadIndex, KWSymbolValueBlock* svbValue);
	void UpdateSymbolValueBlockAt(KWLoadIndex liLoadIndex, KWSymbolValueBlock* svbValue);

	/// Gestion des valeurs de type ObjectArrayValueBlock
	KWObjectArrayValueBlock* ComputeObjectArrayValueBlockAt(KWLoadIndex liLoadIndex) const;
	KWObjectArrayValueBlock* GetObjectArrayValueBlockAt(KWLoadIndex liLoadIndex) const;
	void SetObjectArrayValueBlockAt(KWLoadIndex liLoadIndex, KWObjectArrayValueBlock* cvbValue);
	void UpdateObjectArrayValueBlockAt(KWLoadIndex liLoadIndex, KWObjectArrayValueBlock* cvbValue);

	// Methodes avancees de verification de l'initialisation d'un bloc natif
	// Renvoie true si le bloc est initialise, donc pret a l'utilisation
	boolean CheckContinuousValueBlockAt(KWLoadIndex liLoadIndex) const;
	boolean CheckSymbolValueBlockAt(KWLoadIndex liLoadIndex) const;
	boolean CheckObjectArrayValueBlockAt(KWLoadIndex liLoadIndex) const;

	///////////////////////////////////////////////////////////
	// Services divers:

	// Acces generique a une valeur stockee sous forme d'une chaine de caractere
	const char* ValueToString(const KWAttribute* attribute) const;

	// Verification de coherence
	boolean Check() const override;

	// Affichage, ecriture dans un fichier, de facon structuree, avec indentation
	void Write(ostream& ost) const override;
	void PrettyWrite(ostream& ost, const ALString& sIndent) const;

	// Estimation de la memoire utilisee par l'objet, et de tous ses sous-objets
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Creation d'un objet de test, a partir d'une classe et d'une valeur de depart
	// Des sous-objet inclus sont egalement crees
	static KWObject* CreateObject(KWClass* refClass, longint lObjectIndex);

	// Fonction de test de la classe KWObject
	static void Test();

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Declaration en friend des seules classe explotant un data path
	friend class KWSTDatabase;
	friend class KWMTDatabase;
	friend class KWDRRelationCreationRule;
	friend class KWDRRandom;

	// Parametrage d'un data path
	void SetObjectDataPath(const KWObjectDataPath* dataPath);

	// Acces au data path
	const KWObjectDataPath* GetObjectDataPath() const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Services de calul des valeurs et de destruction des attributs

	// Verification de la concordance entre la classe d'un objet (s'il est non NULL)
	// et celle d'un attribut, pour les attributs de type Objet ou ObjectArray
	// La correspondance doit etre exacte dans le cas des objets inclus, et peut etre
	// "approximative" pour les objet references (cf. KWDatabase: classes logique et physique)
	// Pas de message d'erreur emis par cette methode
	boolean CheckAttributeObjectClass(KWAttribute* attribute, KWObject* kwoValue) const;
	boolean CheckAttributeObjectArrayClass(KWAttribute* attribute, ObjectArray* oaValue) const;

	// Calcul de tous les attributs derives, recursivement sur les objets de la composition
	// Le memory guard permet de controler l'utilisation de la memoire au cours du calcul des attributs derives.
	// En cas de depassement de la limite memoire, l'objet est nettoye, et on ne garde que les attributs natifs,
	// les autres etant mis a Missing.
	void ComputeAllValues(KWDatabaseMemoryGuard* memoryGuard);

	// Destruction des attributs (recursive pour les objet inclus)
	void DeleteAttributes();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Services de nettoyage ou destruction totale ou partielle des attributs,
	// partage par les methodes de destruction et de mutation

	// Reinitialisation des valeurs de type Symbol dans la liste LoadedDenseSymbolAttribute
	void ResetLoadedSymbolValues(int nStartIndex, int nStopIndex);

	// Reinitialisation des valeurs de type Text dans la liste LoadedTextAttribute
	void ResetLoadedTextValues(int nStartIndex, int nStopIndex);

	// Destruction des valeurs de type TextList dans la liste LoadedTextListAttribute
	void DeleteLoadedTextListValues(int nStartIndex, int nStopIndex);

	// Destruction des blocs de valeurs dans la liste LoadedAttributeBlock
	void DeleteLoadedAttributeBlockValues(int nStartIndex, int nStopIndex);

	// Destruction des valeurs de type Object ou ObjectArray dans la liste LoadedRelationAttribute
	void DeleteLoadedRelationValues(int nStartIndex, int nStopIndex);

	// Destruction des valeurs de type Object ou ObjectArray inclus natifs ou crees non charges en memoire,
	// dans la liste UnloadedOwnedRelationAttribute
	void DeleteUnloadedOwnedRelationValues(int nStartIndex, int nStopIndex);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Services de nettoyage des attributs des instances volumineuses, dont ne peut pas calculer
	// tous les attributs derives

	// Nettoyage des elements de donnees attributs temporaires a calculer, pouvant etre nettoyes et recalcules
	// plusieurs fois
	void CleanTemporayDataItemsToComputeAndClean();

	// Nettoyage des donnees non natives, a utiliser pour nettoyer une instance
	//  dont on a pas pu calculer tous les attributs derives
	// Les attribut de type natif de type Relation sont nettoyes
	// Tous les attributs calcule sont mis a leur valeur par defaut ("", Missing...)
	// Seuls les attributs natifs sont gardes
	void CleanAllNonNativeAttributes();

	// Nettoyage des attributs natifs de type Relation charges en memoire (nettoyage recursif)
	// L'objet reste utilisable: seuls ses attributs de type Relation sont detruits et mis a NULL
	friend class KWMTDatabase;
	void CleanNativeRelationAttributes();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methode de mutation specifique a destination des classes KWDatabase et KWDataTableSliceSet
	// La nouvelle classe doit avoir le meme nom que celle de la classe en cours
	// Dans certains cas particuliers, la nouvelle classe peut-etre la meme. Dans ce cas,
	// l'objet est conserve tel quel, et seul son contenu est mute si necessaire
	// Sinon, la nouvelle classe doit avoir moins d'attributs Loaded que la classe
	// precedente, et ces attributs doivent coincider (meme type, meme nom) et etre en tete de
	// classe dans le meme ordre.
	//
	// Lors de la mutation, on a les operations suivantes:
	// - attributs commun gardes
	// - attributs en trop detruits
	// - valeur derivees transferees telles quelles
	// - objets internes inclus ou multi-inclus soit mutes egalement si gardes
	//   (si possible, sinon detruits), soit detruits s'ils sont a supprimer
	// - objets references laisses tels quels (ni mutes, ni detruits)
	//
	// Le dictionnaire des classe de mutation specifie pour chaque classe initiale
	// la classe a utiliser pour la mutation
	//
	// Le dictionnaire des attributs a garder (dans la nouvelle classe) indique
	// les attributs natifs non utilises object inclus ou multi-inclus a garder
	// lors de la mutation, car referencable par des regles de derivation
	void Mutate(const KWClass* kwcNewClass, const NumericKeyDictionary* nkdMutationClasses,
		    const NumericKeyDictionary* nkdUnusedNativeAttributesToKeep);

	// Classes utilisant le service de mutation
	friend class KWDatabase;
	friend class KWDataTableSliceSet;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Services specifiques a la mutation

	// Destruction des valeurs de ObjectArray sans leur contenu dans la liste LoadedRelationAttribute
	void DeleteLoadedReferencedObjectArrayValues(int nStartIndex, int nStopIndex);

	// Nettoyage des blocs de valeurs de la nouvelle classe, conserves potentiellement avec
	// une sous-partie de leurs attributs
	void CleanNewLoadedAttributeBlockValues(const KWClass* kwcNewClass);

	// Mutation des valeurs de type Object ou ObjectArray dans la liste LoadedRelationAttribute
	void MutateLoadedRelationValues(const NumericKeyDictionary* nkdMutationClasses,
					const NumericKeyDictionary* nkdUnusedNativeAttributesToKeep);

	// Destruction des valeurs de type Object ou ObjectArray inclus natifs ou crees non charges en memoire,
	// dans la liste UnloadedOwnedRelationAttribute
	void MutateUnloadedOwnedRelationValues(const NumericKeyDictionary* nkdMutationClasses,
					       const NumericKeyDictionary* nkdUnusedNativeAttributesToKeep);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Attributs de gestion de la classe

	// KWClass
	const KWClass* kwcClass;

	// Index de creation permettant de memoriser un numero de ligne lors de la lecture d'un fichier (0 si non
	// initialise) Permet d'implementer les regles de derivation Index et Random avec des resultats reproductibles
	// lors de la lecture des meme bases
	// Cet index est attribue a la creation de l'objet et n'est jamais modifie
	// Pour optimiser la taille memoire des KWObject, on se sert de l'index pour stocker trois informations
	// - index de creation
	//   - on stocke deux fois la valeur de l'index de creation
	// - taille du vecteur d'attribut (cf. Set|GetSmallSize)
	//   - stocke dans le signe: tres rapide a tester, utilise pour chaque acces a une valeur
	//     - petite taille: index positif
	//     - grande taille: index negatif
	// - utilisation de type vue (cf. Set|GetViewTypeUse)
	//   - stocke dans le premier bit
	//     - utilisation de type vue: valeur 1
	//     - utilisation standard: valeur 0
	longint lCreationIndex;

	// Datapath associe a l'objet en multi-table
	// Un data path est un chemin d'acces a l'objet dans un schema multi-table, que l'objet soit
	// lu depuis un fichier ou cree par une regle de creation d'instance
	// Cela permet d'identifier de facon unique chaque objet par la paire (dataPath, CreationIndex),
	// avec le CreationIndex correspondant a un numero de ligne dans un fichier dans le cas d'une
	// le cas de donnees mappees sur des fichiers, ou d'un numero de creation local a l'instance principale
	// Cet identification est necessaire pour la regle Random, pour generer des suites de valeurs aleatoire
	// reproductibles, car dependant de l'identifiant unique d'un objet lu ou cree, et independant entre elles,
	// chacune exploitant un couple (Seed, Leap) deuit par hashage de son data path et du contexte
	// d'utilisation de la fonction random
	// Un data path peut etre NULL, notamment dans le cas mon,o-table ou il n'est pas necessaire.
	const KWObjectDataPath* objectDataPath;

	//////////////////////////////////////////////////////////////////////////////
	// Pour gerer les objet avec un nombre potentiel d'attributs tres important,
	// les valeur ne sont pas accessible directment par un tableau (KWValue*),
	// mais par un tableau de tableaux de valeurs, chaque tableau de valeur ne
	// depassant pas la taille d'un segment memoire.
	// La gestion de ces tableaux de grande taille est analogue a celle mise en
	// place dans la classe MemVector, mais en plus otpimise en tenant compte
	// des particularite suivantes:
	//  . la taille des tableau est fixe une fois pour toute par la KWClass
	//  . seul l'acces aux valeur est necessaire (pas de fonctionnalite de type
	//    retaillage, tri, randomisation de l'odre...)

	// Gestion des valeurs dans un tableau
	//  . attributeValues contient toutes les valeurs dans un tableau
	//  . attributeValueArrays contient le tableau de tableaux de valeurs
	union ObjectValues
	{
		KWValue* attributeValues;
		KWValue** attributeValueArrays;
	} values;

	// Indicateur de petite taille, selon le nombre d'attribut de l'objet (cf. lCreationIndex)
	boolean GetSmallSize() const;
	void SetSmallSize(boolean bValue);

	// Classe des regles de derivation de creation d'objet en friend, pour acceder a SetViewTypeUsed
	friend class KWDRRelationCreationRule;

	// Indicateur de type vue pour un object, selon le type de creation d'un objet (cf. lCreationIndex)
	// - utilisation standard:
	//   - les attributs natifs de type Entity ou Table sont crees a partir des lectures dans une database
	//   - il sont detruits avec l'objet
	// - utilisation de type vue:
	//   - les attributs natifs de type Entity ou Table sont recopies a partir d'objets sources au moment
	//     de la creation de l'objet via une regle de derivation de creation de Table
	//   - il ne sont pas detruits avec l'objet, car deja detruits avec l'objet source
	boolean GetViewTypeUse() const;
	void SetViewTypeUse(boolean bValue);

	// Creation/destruction d'un container d'attribut de taille donnee
	ObjectValues NewValueVector(int nSize);
	void DeleteValueVector(ObjectValues valuesToDelete, int nSize);

	// Acces a une valeur par index
	// Une valeur peut etre une valeur dense ou un block
	KWValue& GetAt(int nValueIndex) const;
	KWValue& GetValueAt(ObjectValues valuesToGet, boolean bSmallSize, int nValueIndex) const;

	// Constantes specifiques pour un acces optimise aux valeurs des attributs
	// par des methodes de type attributeValues[nIndex/nBlockSize][nIndex%nBlockSize]
	static const int nElementSize = (int)sizeof(KWValue);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Informations disponibles uniquement en mode debug, pour le controle
	// de la coherence entre KWClass et KWObject
	debug(int nObjectLoadedDataItemNumber);
	debug(int nFreshness);
};

// Methode de comparaison entre deux objet sur le index de creation
int KWObjectCompareCreationIndex(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////
// Gestion des attributs derives
// Les attributs derives sont initialises avec des constantes speciales
// (cf valeurs interdites definies dans KWValue). Ces valeurs signifient
// "non encore calcules". Seule la classe KWObject a le  droit en interne
// de positionner explicitement ces valeurs.
// Il n'est pas possible de valoriser un attribut derive (controle par require).
// L'acces en lecture declenche le calcul d'un attribut derive. Ce calcul est
// bufferise. Les derivation recursives sont ainsi gerees automatiquement

#include "KWClass.h"
#include "KWAttributeBlock.h"
#include "KWDerivationRule.h"
#include "KWDataPath.h"

/////////////////////////////////////////////////////////////////
// Methodes en inline

inline const KWClass* KWObject::GetClass() const
{
	return kwcClass;
}

inline longint KWObject::GetCreationIndex() const
{
	assert(lCreationIndex != 0);
	if (lCreationIndex > 0)
		return lCreationIndex / 2;
	else
		return -lCreationIndex / 2;
}

inline void KWObject::SetObjectDataPath(const KWObjectDataPath* dataPath)
{
	require(dataPath == NULL or objectDataPath == NULL);
	objectDataPath = dataPath;
}

inline const KWObjectDataPath* KWObject::GetObjectDataPath() const
{
	return objectDataPath;
}

inline boolean KWObject::GetSmallSize() const
{
	assert(lCreationIndex != 0);
	return (lCreationIndex > 0);
}

inline void KWObject::SetSmallSize(boolean bValue)
{
	assert(lCreationIndex != 0);
	if (lCreationIndex < 0 and bValue)
		lCreationIndex = -lCreationIndex;
	else if (lCreationIndex > 0 and not bValue)
		lCreationIndex = -lCreationIndex;
	ensure(GetSmallSize() == bValue);
}

inline boolean KWObject::GetViewTypeUse() const
{
	assert(lCreationIndex != 0);
	if (lCreationIndex > 0)
		return lCreationIndex % 2;
	else
		return (-lCreationIndex) % 2;
}

inline void KWObject::SetViewTypeUse(boolean bValue)
{
	assert(lCreationIndex != 0);
	if (GetViewTypeUse() != bValue)
	{
		if (lCreationIndex > 0)
		{
			if (bValue)
				lCreationIndex += 1;
			else
				lCreationIndex -= 1;
		}
		else
		{
			if (bValue)
				lCreationIndex -= 1;
			else
				lCreationIndex += 1;
		}
	}
	ensure(GetViewTypeUse() == bValue);
}

inline KWValue& KWObject::GetAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < kwcClass->GetTotalInternallyLoadedDataItemNumber());
	assert(values.attributeValues != NULL);

	// Acces a la valeur dans le tableau ou le tableau de tableau
	return (GetSmallSize() ? values.attributeValues[nValueIndex]
			       : (values.attributeValueArrays[nValueIndex / nBlockSize])[nValueIndex % nBlockSize]);
}

inline KWValue& KWObject::GetValueAt(ObjectValues valuesToGet, boolean bSmallSize, int nValueIndex) const
{
	require(0 <= nValueIndex);
	assert(valuesToGet.attributeValues != NULL);

	// Acces a la valeur dans le tableau ou le tableau de tableau
	return (bSmallSize ? valuesToGet.attributeValues[nValueIndex]
			   : (valuesToGet.attributeValueArrays[nValueIndex / nBlockSize])[nValueIndex % nBlockSize]);
}

inline Continuous KWObject::ComputeContinuousValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Continuous));

	// Calcul eventuel de l'attribut derive dans le cas d'une valeur dense
	if (liLoadIndex.IsDense())
	{
		if (GetAt(liLoadIndex.GetDenseIndex()).IsContinuousForbidenValue())
		{
			// Verification que l'attribut est derive
			assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

			// Derivation
			GetAt(liLoadIndex.GetDenseIndex())
			    .SetContinuous(
				KWContinuous::DoubleToContinuous(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)
								     ->GetDerivationRule()
								     ->ComputeContinuousResult(this)));

			// Verification de la valeur de l'attribut derive
			assert(not GetAt(liLoadIndex.GetDenseIndex()).IsContinuousForbidenValue());
		}
		return GetAt(liLoadIndex.GetDenseIndex()).GetContinuous();
	}
	// Calcul eventuel de l'attribut derive dans le cas d'une valeur sparse, d'un bloc de valeur
	else
	{
		if (GetAt(liLoadIndex.GetDenseIndex()).IsContinuousValueBlockForbidenValue())
		{
			KWAttributeBlock* attributeBlock;
			KWContinuousValueBlock* cvbValue;

			// Verification que l'attribut est derive
			attributeBlock = kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex);
			assert(attributeBlock->GetDerivationRule() != NULL);

			// Derivation
			cvbValue = attributeBlock->GetDerivationRule()->ComputeContinuousValueBlockResult(
			    this, attributeBlock->GetLoadedAttributesIndexedKeyBlock());
			check(cvbValue);
			GetAt(liLoadIndex.GetDenseIndex()).SetContinuousValueBlock(cvbValue);

			// Verification de la valeur de l'attribut derive
			assert(not GetAt(liLoadIndex.GetDenseIndex()).IsContinuousValueBlockForbidenValue());
		}
		return GetAt(liLoadIndex.GetDenseIndex())
		    .GetContinuousValueBlock()
		    ->GetValueAtAttributeSparseIndex(
			liLoadIndex.GetSparseIndex(),
			kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetContinuousDefaultValue());
	}
}

inline Continuous KWObject::GetContinuousValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Continuous));
	require(not GetAt(liLoadIndex.GetDenseIndex()).IsContinuousForbidenValue());
	require(not GetAt(liLoadIndex.GetDenseIndex()).IsContinuousValueBlockForbidenValue());

	// Acces a la valeur dense ou sparse
	if (liLoadIndex.IsDense())
		return GetAt(liLoadIndex.GetDenseIndex()).GetContinuous();
	else
		return GetAt(liLoadIndex.GetDenseIndex())
		    .GetContinuousValueBlock()
		    ->GetValueAtAttributeSparseIndex(
			liLoadIndex.GetSparseIndex(),
			kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetContinuousDefaultValue());
}

inline void KWObject::SetContinuousValueAt(KWLoadIndex liLoadIndex, Continuous cValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(liLoadIndex.IsDense());
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Continuous));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetContinuous(cValue);
}

inline Symbol& KWObject::ComputeSymbolValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Symbol));

	// Calcul eventuel de l'attribut derive dans le cas d'une valeur dense
	if (liLoadIndex.IsDense())
	{
		if (GetAt(liLoadIndex.GetDenseIndex()).IsSymbolForbidenValue())
		{
			// Verification que l'attribut est derive
			assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

			// Derivation
			GetAt(liLoadIndex.GetDenseIndex())
			    .SetSymbol(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)
					   ->GetDerivationRule()
					   ->ComputeSymbolResult(this));

			// Verification de la valeur de l'attribut derive
			assert(not GetAt(liLoadIndex.GetDenseIndex()).IsSymbolForbidenValue());
		}
		return GetAt(liLoadIndex.GetDenseIndex()).GetSymbol();
	}
	// Calcul eventuel de l'attribut derive dans le cas d'une valeur sparse, d'un bloc de valeur
	else
	{
		if (GetAt(liLoadIndex.GetDenseIndex()).IsSymbolValueBlockForbidenValue())
		{
			KWAttributeBlock* attributeBlock;
			KWSymbolValueBlock* svbValue;

			// Verification que l'attribut est derive
			attributeBlock = kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex);
			assert(attributeBlock->GetDerivationRule() != NULL);

			// Derivation
			svbValue = attributeBlock->GetDerivationRule()->ComputeSymbolValueBlockResult(
			    this, attributeBlock->GetLoadedAttributesIndexedKeyBlock());
			check(svbValue);
			GetAt(liLoadIndex.GetDenseIndex()).SetSymbolValueBlock(svbValue);

			// Verification de la valeur de l'attribut derive
			assert(not GetAt(liLoadIndex.GetDenseIndex()).IsSymbolValueBlockForbidenValue());
		}
		return GetAt(liLoadIndex.GetDenseIndex())
		    .GetSymbolValueBlock()
		    ->GetValueAtAttributeSparseIndex(
			liLoadIndex.GetSparseIndex(),
			kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetSymbolDefaultValue());
	}
}

inline Symbol& KWObject::GetSymbolValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Symbol));
	require(not GetAt(liLoadIndex.GetDenseIndex()).IsSymbolForbidenValue());
	require(not GetAt(liLoadIndex.GetDenseIndex()).IsSymbolValueBlockForbidenValue());

	// Acces a la valeur dense ou sparse
	if (liLoadIndex.IsDense())
		return GetAt(liLoadIndex.GetDenseIndex()).GetSymbol();
	else
		return GetAt(liLoadIndex.GetDenseIndex())
		    .GetSymbolValueBlock()
		    ->GetValueAtAttributeSparseIndex(
			liLoadIndex.GetSparseIndex(),
			kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetSymbolDefaultValue());
}

inline void KWObject::SetSymbolValueAt(KWLoadIndex liLoadIndex, const Symbol& sValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(liLoadIndex.IsDense());
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Symbol));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetSymbol(sValue);
}

inline Date KWObject::ComputeDateValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Date));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsDateForbidenValue())
	{
		// Verification que l'attribut est derive
		assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

		// Derivation
		GetAt(liLoadIndex.GetDenseIndex())
		    .SetDate(
			kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule()->ComputeDateResult(this));

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsDateForbidenValue());
	}
	return GetAt(liLoadIndex.GetDenseIndex()).GetDate();
}

inline Date KWObject::GetDateValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Date));

	return GetAt(liLoadIndex.GetDenseIndex()).GetDate();
}

inline void KWObject::SetDateValueAt(KWLoadIndex liLoadIndex, Date dValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Date));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetDate(dValue);
}

inline Time KWObject::ComputeTimeValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Time));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsTimeForbidenValue())
	{
		// Verification que l'attribut est derive
		assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

		// Derivation
		GetAt(liLoadIndex.GetDenseIndex())
		    .SetTime(
			kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule()->ComputeTimeResult(this));

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsTimeForbidenValue());
	}
	return GetAt(liLoadIndex.GetDenseIndex()).GetTime();
}

inline Time KWObject::GetTimeValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Time));

	return GetAt(liLoadIndex.GetDenseIndex()).GetTime();
}

inline void KWObject::SetTimeValueAt(KWLoadIndex liLoadIndex, Time tValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Time));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetTime(tValue);
}

inline Timestamp KWObject::ComputeTimestampValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Timestamp));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsTimestampForbidenValue())
	{
		// Verification que l'attribut est derive
		assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

		// Derivation
		GetAt(liLoadIndex.GetDenseIndex())
		    .SetTimestamp(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)
				      ->GetDerivationRule()
				      ->ComputeTimestampResult(this));

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsTimestampForbidenValue());
	}
	return GetAt(liLoadIndex.GetDenseIndex()).GetTimestamp();
}

inline Timestamp KWObject::GetTimestampValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Timestamp));

	return GetAt(liLoadIndex.GetDenseIndex()).GetTimestamp();
}

inline void KWObject::SetTimestampValueAt(KWLoadIndex liLoadIndex, Timestamp tsValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Timestamp));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetTimestamp(tsValue);
}

inline TimestampTZ KWObject::ComputeTimestampTZValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::TimestampTZ));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsTimestampTZForbidenValue())
	{
		// Verification que l'attribut est derive
		assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

		// Derivation
		GetAt(liLoadIndex.GetDenseIndex())
		    .SetTimestampTZ(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)
					->GetDerivationRule()
					->ComputeTimestampTZResult(this));

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsTimestampTZForbidenValue());
	}
	return GetAt(liLoadIndex.GetDenseIndex()).GetTimestampTZ();
}

inline TimestampTZ KWObject::GetTimestampTZValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::TimestampTZ));

	return GetAt(liLoadIndex.GetDenseIndex()).GetTimestampTZ();
}

inline void KWObject::SetTimestampTZValueAt(KWLoadIndex liLoadIndex, TimestampTZ tstzValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::TimestampTZ));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetTimestampTZ(tstzValue);
}

inline const Symbol& KWObject::ComputeTextValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Text));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsTextForbidenValue())
	{
		// Verification que l'attribut est derive
		assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

		// Derivation
		GetAt(liLoadIndex.GetDenseIndex())
		    .SetText(
			kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule()->ComputeTextResult(this));

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsTextForbidenValue());
	}
	return GetAt(liLoadIndex.GetDenseIndex()).GetText();
}

inline const Symbol& KWObject::GetTextValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Text));

	return GetAt(liLoadIndex.GetDenseIndex()).GetText();
}

inline void KWObject::SetTextValueAt(KWLoadIndex liLoadIndex, const Symbol& sValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Text));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetText(sValue);
}

inline SymbolVector* KWObject::ComputeTextListValueAt(KWLoadIndex liLoadIndex) const
{
	SymbolVector* svTextList;

	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::TextList));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsTextListForbidenValue())
	{
		// Verification que l'attribut est derive
		assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

		// Derivation
		// Le vecteur de textes rendu par la regle est duplique, et appartient desormais a l'objet
		svTextList =
		    kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule()->ComputeTextListResult(this);
		if (svTextList != NULL)
			svTextList = svTextList->Clone();
		GetAt(liLoadIndex.GetDenseIndex()).SetTextList(svTextList);

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsTextListForbidenValue());
	}
	return GetAt(liLoadIndex.GetDenseIndex()).GetTextList();
}

inline SymbolVector* KWObject::GetTextListValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::TextList));

	return GetAt(liLoadIndex.GetDenseIndex()).GetTextList();
}

inline void KWObject::SetTextListValueAt(KWLoadIndex liLoadIndex, SymbolVector* sValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::TextList));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetTextList(sValue);
}

inline KWObject* KWObject::ComputeObjectValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Object));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsObjectForbidenValue())
	{
		// Verification que l'attribut est derive
		assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

		// Derivation
		GetAt(liLoadIndex.GetDenseIndex())
		    .SetObject(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)
				   ->GetDerivationRule()
				   ->ComputeObjectResult(this, liLoadIndex));

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsObjectForbidenValue());
	}
	ensure(CheckAttributeObjectClass(kwcClass->GetAttributeAtLoadIndex(liLoadIndex),
					 GetAt(liLoadIndex.GetDenseIndex()).GetObject()));
	return GetAt(liLoadIndex.GetDenseIndex()).GetObject();
}

inline KWObject* KWObject::GetObjectValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Object));

	ensure(CheckAttributeObjectClass(kwcClass->GetAttributeAtLoadIndex(liLoadIndex),
					 GetAt(liLoadIndex.GetDenseIndex()).GetObject()));
	return GetAt(liLoadIndex.GetDenseIndex()).GetObject();
}

inline void KWObject::SetObjectValueAt(KWLoadIndex liLoadIndex, KWObject* kwoValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Object));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);
	require(CheckAttributeObjectClass(kwcClass->GetAttributeAtLoadIndex(liLoadIndex), kwoValue));

	GetAt(liLoadIndex.GetDenseIndex()).SetObject(kwoValue);
}

inline ObjectArray* KWObject::ComputeObjectArrayValueAt(KWLoadIndex liLoadIndex) const
{
	ObjectArray* oaSubObjects;

	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ObjectArray));

	// Calcul eventuel de l'attribut derive dans le cas d'une valeur dense
	if (liLoadIndex.IsDense())
	{
		if (GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayForbidenValue())
		{
			// Verification que l'attribut est derive
			assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

			// Derivation
			// Le tableau d'objet rendu par la regle est duplique, et appartient desormais a l'objet
			oaSubObjects = kwcClass->GetAttributeAtLoadIndex(liLoadIndex)
					   ->GetDerivationRule()
					   ->ComputeObjectArrayResult(this, liLoadIndex);
			if (oaSubObjects != NULL)
				oaSubObjects = oaSubObjects->Clone();
			GetAt(liLoadIndex.GetDenseIndex()).SetObjectArray(oaSubObjects);

			// Verification de la valeur de l'attribut derive
			assert(not GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayForbidenValue());
		}
		ensure(CheckAttributeObjectArrayClass(kwcClass->GetAttributeAtLoadIndex(liLoadIndex),
						      GetAt(liLoadIndex.GetDenseIndex()).GetObjectArray()));
		return GetAt(liLoadIndex.GetDenseIndex()).GetObjectArray();
	}
	// Calcul eventuel de l'attribut derive dans le cas d'une valeur sparse, d'un bloc de valeur
	else
	{
		if (GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayValueBlockForbidenValue())
		{
			KWAttributeBlock* attributeBlock;
			KWObjectArrayValueBlock* cvbValue;

			// Verification que l'attribut est derive
			attributeBlock = kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex);
			assert(attributeBlock->GetDerivationRule() != NULL);

			// Derivation
			cvbValue = attributeBlock->GetDerivationRule()->ComputeObjectArrayValueBlockResult(
			    this, attributeBlock->GetLoadedAttributesIndexedKeyBlock());
			check(cvbValue);
			GetAt(liLoadIndex.GetDenseIndex()).SetObjectArrayValueBlock(cvbValue);

			// Verification de la valeur de l'attribut derive
			assert(not GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayValueBlockForbidenValue());
		}
		return GetAt(liLoadIndex.GetDenseIndex())
		    .GetObjectArrayValueBlock()
		    ->GetValueAtAttributeSparseIndex(liLoadIndex.GetSparseIndex());
	}
}

inline ObjectArray* KWObject::GetObjectArrayValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ObjectArray));

	// Acces a la valeur dense ou sparse
	if (liLoadIndex.IsDense())
	{
		ensure(CheckAttributeObjectArrayClass(kwcClass->GetAttributeAtLoadIndex(liLoadIndex),
						      GetAt(liLoadIndex.GetDenseIndex()).GetObjectArray()));
		return GetAt(liLoadIndex.GetDenseIndex()).GetObjectArray();
	}
	else
	{
		ensure(
		    CheckAttributeObjectArrayClass(kwcClass->GetAttributeAtLoadIndex(liLoadIndex),
						   GetAt(liLoadIndex.GetDenseIndex())
						       .GetObjectArrayValueBlock()
						       ->GetValueAtAttributeSparseIndex(liLoadIndex.GetSparseIndex())));
		return GetAt(liLoadIndex.GetDenseIndex())
		    .GetObjectArrayValueBlock()
		    ->GetValueAtAttributeSparseIndex(liLoadIndex.GetSparseIndex());
	}
}

inline void KWObject::SetObjectArrayValueAt(KWLoadIndex liLoadIndex, ObjectArray* oaValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(liLoadIndex.IsDense());
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ObjectArray));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);
	require(CheckAttributeObjectArrayClass(kwcClass->GetAttributeAtLoadIndex(liLoadIndex), oaValue));

	GetAt(liLoadIndex.GetDenseIndex()).SetObjectArray(oaValue);
}

inline Object* KWObject::ComputeStructureValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Structure));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsStructureForbidenValue())
	{
		// Verification que l'attribut est derive
		assert(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() != NULL);

		// Derivation
		GetAt(liLoadIndex.GetDenseIndex())
		    .SetStructure(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)
				      ->GetDerivationRule()
				      ->ComputeStructureResult(this));

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsStructureForbidenValue());
	}
	return GetAt(liLoadIndex.GetDenseIndex()).GetStructure();
}

inline Object* KWObject::GetStructureValueAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Structure));

	return GetAt(liLoadIndex.GetDenseIndex()).GetStructure();
}

inline void KWObject::SetStructureValueAt(KWLoadIndex liLoadIndex, Object* oValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::Structure));
	require(kwcClass->GetAttributeAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetStructure(oValue);
}

inline KWContinuousValueBlock* KWObject::ComputeContinuousValueBlockAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ContinuousValueBlock));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsContinuousValueBlockForbidenValue())
	{
		KWAttributeBlock* attributeBlock;
		KWContinuousValueBlock* cvbValue;

		// Verification que l'attribut est derive
		attributeBlock = kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex);
		assert(attributeBlock->GetDerivationRule() != NULL);

		// Derivation
		cvbValue = attributeBlock->GetDerivationRule()->ComputeContinuousValueBlockResult(
		    this, attributeBlock->GetLoadedAttributesIndexedKeyBlock());
		check(cvbValue);
		GetAt(liLoadIndex.GetDenseIndex()).SetContinuousValueBlock(cvbValue);

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsContinuousValueBlockForbidenValue());
	}
	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock() != NULL);
	return GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock();
}

inline KWContinuousValueBlock* KWObject::GetContinuousValueBlockAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ContinuousValueBlock));

	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock() != NULL);
	return GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock();
}

inline void KWObject::SetContinuousValueBlockAt(KWLoadIndex liLoadIndex, KWContinuousValueBlock* cvbValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ContinuousValueBlock));
	require(kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);
	require(GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock() == NULL);
	require(cvbValue != NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetContinuousValueBlock(cvbValue);
	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock() != NULL);
}

inline void KWObject::UpdateContinuousValueBlockAt(KWLoadIndex liLoadIndex, KWContinuousValueBlock* cvbValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ContinuousValueBlock));
	require(kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);
	require(GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock() != NULL);
	require(cvbValue != NULL);

	delete GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock();
	GetAt(liLoadIndex.GetDenseIndex()).SetContinuousValueBlock(cvbValue);
	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock() != NULL);
}

inline KWSymbolValueBlock* KWObject::ComputeSymbolValueBlockAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::SymbolValueBlock));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsSymbolValueBlockForbidenValue())
	{
		KWAttributeBlock* attributeBlock;
		KWSymbolValueBlock* svbValue;

		// Verification que l'attribut est derive
		attributeBlock = kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex);
		assert(attributeBlock->GetDerivationRule() != NULL);

		// Derivation
		svbValue = attributeBlock->GetDerivationRule()->ComputeSymbolValueBlockResult(
		    this, attributeBlock->GetLoadedAttributesIndexedKeyBlock());
		check(svbValue);
		GetAt(liLoadIndex.GetDenseIndex()).SetSymbolValueBlock(svbValue);

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsSymbolValueBlockForbidenValue());
	}
	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock() != NULL);
	return GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock();
}

inline KWSymbolValueBlock* KWObject::GetSymbolValueBlockAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::SymbolValueBlock));

	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock() != NULL);
	return GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock();
}

inline void KWObject::SetSymbolValueBlockAt(KWLoadIndex liLoadIndex, KWSymbolValueBlock* svbValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::SymbolValueBlock));
	require(kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);
	require(GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock() == NULL);
	require(svbValue != NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetSymbolValueBlock(svbValue);
	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock() != NULL);
}

inline void KWObject::UpdateSymbolValueBlockAt(KWLoadIndex liLoadIndex, KWSymbolValueBlock* cvbValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::SymbolValueBlock));
	require(kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);
	require(GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock() != NULL);
	require(cvbValue != NULL);

	delete GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock();
	GetAt(liLoadIndex.GetDenseIndex()).SetSymbolValueBlock(cvbValue);
	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock() != NULL);
}

inline KWObjectArrayValueBlock* KWObject::ComputeObjectArrayValueBlockAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ObjectArrayValueBlock));

	// Calcul eventuel de l'attribut derive
	if (GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayValueBlockForbidenValue())
	{
		KWAttributeBlock* attributeBlock;
		KWObjectArrayValueBlock* svbValue;

		// Verification que l'attribut est derive
		attributeBlock = kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex);
		assert(attributeBlock->GetDerivationRule() != NULL);

		// Derivation
		svbValue = attributeBlock->GetDerivationRule()->ComputeObjectArrayValueBlockResult(
		    this, attributeBlock->GetLoadedAttributesIndexedKeyBlock());
		check(svbValue);
		GetAt(liLoadIndex.GetDenseIndex()).SetObjectArrayValueBlock(svbValue);

		// Verification de la valeur de l'attribut derive
		assert(not GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayValueBlockForbidenValue());
	}
	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock() != NULL);
	return GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock();
}

inline KWObjectArrayValueBlock* KWObject::GetObjectArrayValueBlockAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ObjectArrayValueBlock));

	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock() != NULL);
	return GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock();
}

inline void KWObject::SetObjectArrayValueBlockAt(KWLoadIndex liLoadIndex, KWObjectArrayValueBlock* svbValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ObjectArrayValueBlock));
	require(kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);
	require(GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock() == NULL);
	require(svbValue != NULL);

	GetAt(liLoadIndex.GetDenseIndex()).SetObjectArrayValueBlock(svbValue);
	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock() != NULL);
}

inline void KWObject::UpdateObjectArrayValueBlockAt(KWLoadIndex liLoadIndex, KWObjectArrayValueBlock* cvbValue)
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ObjectArrayValueBlock));
	require(kwcClass->GetAttributeBlockAtLoadIndex(liLoadIndex)->GetDerivationRule() == NULL);
	require(GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock() != NULL);
	require(cvbValue != NULL);

	delete GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock();
	GetAt(liLoadIndex.GetDenseIndex()).SetObjectArrayValueBlock(cvbValue);
	ensure(GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock() != NULL);
}

inline boolean KWObject::CheckContinuousValueBlockAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ContinuousValueBlock));
	require(not GetAt(liLoadIndex.GetDenseIndex()).IsContinuousValueBlockForbidenValue());

	return GetAt(liLoadIndex.GetDenseIndex()).GetContinuousValueBlock() != NULL;
}

inline boolean KWObject::CheckSymbolValueBlockAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::SymbolValueBlock));
	require(not GetAt(liLoadIndex.GetDenseIndex()).IsSymbolValueBlockForbidenValue());

	return GetAt(liLoadIndex.GetDenseIndex()).GetSymbolValueBlock() != NULL;
}

inline boolean KWObject::CheckObjectArrayValueBlockAt(KWLoadIndex liLoadIndex) const
{
	debug(require(nObjectLoadedDataItemNumber == kwcClass->GetTotalInternallyLoadedDataItemNumber()));
	debug(require(nFreshness == kwcClass->GetFreshness()));
	require(kwcClass->CheckTypeAtLoadIndex(liLoadIndex, KWType::ObjectArrayValueBlock));
	require(not GetAt(liLoadIndex.GetDenseIndex()).IsObjectArrayValueBlockForbidenValue());

	return GetAt(liLoadIndex.GetDenseIndex()).GetObjectArrayValueBlock() != NULL;
}

inline const char* KWObject::ValueToString(const KWAttribute* attribute) const
{
	require(attribute != NULL);
	if (attribute->GetType() == KWType::Continuous)
	{
		return KWContinuous::ContinuousToString(GetContinuousValueAt(attribute->GetLoadIndex()));
	}
	else if (attribute->GetType() == KWType::Symbol)
	{
		return GetSymbolValueAt(attribute->GetLoadIndex());
	}
	else if (attribute->GetType() == KWType::Date)
	{
		return attribute->GetDateFormat()->DateToString(GetDateValueAt(attribute->GetLoadIndex()));
	}
	else if (attribute->GetType() == KWType::Time)
	{
		return attribute->GetTimeFormat()->TimeToString(GetTimeValueAt(attribute->GetLoadIndex()));
	}
	else if (attribute->GetType() == KWType::Timestamp)
	{
		return attribute->GetTimestampFormat()->TimestampToString(
		    GetTimestampValueAt(attribute->GetLoadIndex()));
	}
	else if (attribute->GetType() == KWType::TimestampTZ)
	{
		return attribute->GetTimestampTZFormat()->TimestampTZToString(
		    GetTimestampTZValueAt(attribute->GetLoadIndex()));
	}
	else if (attribute->GetType() == KWType::Text)
	{
		return GetTextValueAt(attribute->GetLoadIndex());
	}
	else
	{
		assert(not KWType::IsStored(attribute->GetType()));
		char* sBuffer = StandardGetBuffer();
		sBuffer[0] = '\0';
		return sBuffer;
	}
}

inline boolean KWObject::CheckAttributeObjectClass(KWAttribute* attribute, KWObject* kwoValue) const
{
	require(attribute != NULL);
	require(KWType::IsRelation(attribute->GetType()));
	if (kwoValue != NULL)
	{
		// La classe doit etre de meme nom, mais ce n'est pas necessairement la meme
		// En effet, lors d'une mutation d'objet par la methode Mutate, un objet du niveau
		// logique peut etre un sous-objet d'un objet du niveau physique, gere par une classe
		// du niveau physique
		return attribute->GetClass()->GetName() == kwoValue->GetClass()->GetName();
	}
	else
		return true;
}

inline boolean KWObject::CheckAttributeObjectArrayClass(KWAttribute* attribute, ObjectArray* oaValue) const
{
	require(attribute != NULL);
	require(attribute->GetType() == KWType::ObjectArray);
	if (oaValue != NULL and oaValue->GetSize() > 0)
		return CheckAttributeObjectClass(attribute, cast(KWObject*, oaValue->GetAt(0)));
	else
		return true;
}
