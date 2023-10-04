// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataItem;
class KWAttribute;
class KWAttributeBlock;
class KWClass;
class KWObject;
class KWClassDomain;
class KWDerivationRule;

#include "Standard.h"
#include "ALString.h"
#include "KWSymbol.h"
#include "Ermgt.h"
#include "Object.h"
#include "Vector.h"
#include "KWType.h"
#include "KWMetaData.h"
#include "KWLoadIndex.h"
#include "KWTextService.h"
#include "KWCDUniqueString.h"

///////////////////////////////////////////////////////////////////////////
// Description d'un dictionnaire decrivant la structure des donnees
// Un KWClass permet de decrire la composition d'un KWObject, a savoir
// sa structuration en attributs de type
//  - donnees elementaires
//    . Continuous
//    . Symbol
//  - donnees de type temporel ou textuel
//  - donnees structurees
//    . Object
//    . ObjectArray
//  - donnees algorithmiques, pour stocker les calculs intermediaires, les modeles et les appliquer
//    . Structure
// Elle possede:
//  . un nom (obligatoire)
//  . un libelle
//  . un status de classe racine ou composante
// Les attributs de type Object ou Array d'Object possede un statut
//  Reference:
//    false: objet interne
//    true: objet reference
//
// MEMORY
// Une KWClass contient et gere sa description(a savoir ses variables,
// ses containers, et le contenu de ses containers)
// Les KWClass referencees sont par contre independantes.
class KWClass : public Object
{
public:
	// Constructeur et destructeur
	KWClass();
	~KWClass();

	// Nom de la classe
	// Le SetName n'est possible que si la classe n'est pas inseree dans
	// un domaine de classe
	const ALString& GetName() const;
	void SetName(const ALString& sValue);

	// Meta-donnees sous forme d'une ensemble de paires (cle, valeur)
	const KWMetaData* GetConstMetaData() const;
	KWMetaData* GetMetaData();

	// Libelle
	const ALString& GetLabel() const;
	void SetLabel(const ALString& sValue);

	// Classe racine ou composant(par defaut: false -> component)
	// Une classe racine gere sa destruction memoire, alors qu'une
	// classe composant est geree par sa classe englobante.
	// Une classe racine a necessairement une cle, avec une verification
	// de l'uncite des objets selon cette cle
	// Un classe racine ne peut etre que referencee, et pas utilise
	// en tant que sous-partie
	boolean GetRoot() const;
	void SetRoot(boolean bValue);

	/////////////////////////////////////////////////////////////
	// Specification des attributs de la cle de la classe
	// Facultatif: utile dans pour les chainage entre classes dans le cas
	// du multi-table avec attributs de type Object ou ObjectArray
	// Les attributs de la cles doivent etre Symbol, non derives

	// Nombre d'attribut de la cle
	int GetKeyAttributeNumber() const;
	void SetKeyAttributeNumber(int nValue);

	// Nom des attributs servant de cle
	const ALString& GetKeyAttributeNameAt(int nIndex) const;
	void SetKeyAttributeNameAt(int nIndex, const ALString& sValue);

	// Acces aux attributs servant de cle
	// Prerequis: la classe doit etre valide
	KWAttribute* GetKeyAttributeAt(int nIndex) const;

	// Indique s'il y a une cle (au moins un champ) et si tous les champs de cles sont charges en memoire
	// Prerequis: la classe doit etre compilee
	boolean IsKeyLoaded() const;

	// Rang d'un attribut cle parmi les attributs charges en memoire (-1 si non charge)
	// Permet l'acces a la valeur de la cle dans les KWObjets
	// Prerequis: la classe doit etre compilee
	KWLoadIndex GetKeyAttributeLoadIndexAt(int nIndex) const;

	/////////////////////////////////////////////////////////////
	// Specification des attributs de la classe

	// Nombre d'attributs
	int GetAttributeNumber() const;

	// Recherche d'un attribut par son nom
	// L'espace de nommage est commun aux attributs et aux bloc d'attributs
	// ce qui permet d'identifier des colonnes de type attribut ou bloc dans
	// les entetes de fichier de donnees
	// Retourne NUL si attribut inexistant
	KWAttribute* LookupAttribute(const ALString& sAttributeName) const;

	// Ajout d'un attribut en fin de liste, et par rapport a un autre attribut
	// Renvoie true si OK, false sinon (attribut ou bloc existant)
	boolean InsertAttribute(KWAttribute* attribute);

	// Insertion avant ou apres un attribut existant
	// Erreur de programmation si attribut de reference inexistant,
	// ou au milieu d'un bloc (on peut etre au debut d'un bloc pour
	// le InsertBefore ou a la fin d'un bloc pour le InsertAfter)
	// Renvoie true si OK, false sinon (attribut ou bloc existant)
	boolean InsertAttributeBefore(KWAttribute* attribute, KWAttribute* attributeRef);
	boolean InsertAttributeAfter(KWAttribute* attribute, KWAttribute* attributeRef);

	// Renommage d'un attribut
	// Retourne true si OK. Sans effet si le nom cible existe deja
	// parmi les attribut ou les blocs.
	// Propagation a toutes les references a cet attribut dans les regles
	// de derivations des attributs de la classe
	boolean RenameAttribute(KWAttribute* refAttribute, const ALString& sNewName);

	// Renommage d'un attribut sans se soucier des utilisation dans les regles
	// Le nom doit etre inexistant dans la classe
	void UnsafeRenameAttribute(KWAttribute* refAttribute, const ALString& sNewName);

	// Modification du flag Used de tous les attributs (not Used => not Loaded)
	void SetAllAttributesUsed(boolean bValue);

	// Modification du flag Used de tous les attributs (sans effet si not Used)
	void SetAllAttributesLoaded(boolean bValue);

	// Supression d'une cle de meta-donnes sur tous les attributs de la classe
	void RemoveAllAttributesMetaDataKey(const ALString& sKey);

	// Supression/destruction d'un attribut par cle
	// Return false si attribute inexistant
	boolean RemoveAttribute(const ALString& sAttributeName);
	boolean DeleteAttribute(const ALString& sAttributeName);

	// Destruction de tous les attributs
	void DeleteAllAttributes();

	/////////////////////////////////////////////////////////////
	// Acces par liste
	// Parcours typique
	//    attribute = myclass->GetHeadAttribute();
	//    while(attribute != NULL)
	//    {
	//       ...
	//       myclass->GetNextAttribute(attribute);
	//    }

	// Debut et fin de liste
	KWAttribute* GetHeadAttribute() const;
	KWAttribute* GetTailAttribute() const;

	// Precedent et suivant
	void GetNextAttribute(KWAttribute*& attribute) const;
	void GetPrevAttribute(KWAttribute*& attribute) const;

	////////////////////////////////////////////////////////////////
	// Acces par index, par famille d'attributs
	// Attention: a n'utiliser qu'une fois la classe indexee

	// Indexation de la classe
	void IndexClass();
	boolean IsIndexed() const;

	// Attributs utilises
	int GetUsedAttributeNumber() const;
	KWAttribute* GetUsedAttributeAt(int nIndex) const;

	// Attributs charges en memoire, denses ou non
	int GetLoadedAttributeNumber() const;
	KWAttribute* GetLoadedAttributeAt(int nIndex) const;

	// Attributs denses charges en memoire (sous partie des attributs charges en memoire)
	int GetLoadedDenseAttributeNumber() const;
	KWAttribute* GetLoadedDenseAttributeAt(int nIndex) const;

	// Blocs d'attribut charges en memoire (contenant les attributs sparses charges en memoire)
	int GetLoadedAttributeBlockNumber() const;
	KWAttributeBlock* GetLoadedAttributeBlockAt(int nIndex) const;

	// Attributs Object ou ObjectArray charges en memoire (hors blocs)
	int GetLoadedRelationAttributeNumber() const;
	KWAttribute* GetLoadedRelationAttributeAt(int nIndex) const;

	////////////////////////////////////////////////////////////////
	// Statistiques par famille d'attributs
	// Attention: a n'utiliser qu'une fois la classe indexee

	// Statistiques par type d'attributs ou de bloc d'attributs utilises
	// Pour les attributs potentiellement denses ou sparses,
	// il s'agit du nombre total, dans ou hors blocs
	int GetUsedAttributeNumberForType(int nType) const;

	// Statistiques par type d'attributs, soit denses, soit sparses
	int GetUsedDenseAttributeNumberForType(int nType) const;
	int GetUsedSparseAttributeNumberForType(int nType) const;

	// Nombre de DataItem natifs (non calcules), utilises ou non
	int GetNativeDataItemNumber() const;

	// Nombre d'attributs natifs de type relation utilises recursivement par la classe
	// Le parametre InternalOnly indique que l'on ignore les classe referencees
	int ComputeOverallNativeRelationAttributeNumber(boolean bIncludingReferences) const;

	// Nombre d'attribut initiaux d'une classe a analyser
	// Il s'agit des attributs utilises de type simple, calcule ou non,
	// en tenant compte d'un eventuel attribut cible dans le cas supervise
	int ComputeInitialAttributeNumber(boolean bIsSupervised) const;

	////////////////////////////////////////////////////////////////
	// Gestion des blocs d'attributs (cf. class KWAttributeBlock)
	// Les bloc sont cree explicitement apres creation des attributs.
	// Qaund de nouveaux attributs sont inseres, ils sont affectes au bloc
	// eventuel les englobant, et lorsqu'ils sont detruits, les
	// bloc sont detruits quand ils deviennent vides

	// Insertion d'un bloc d'attributs, a partir d'un nom,
	// du premier et du dernier attribut du bloc
	// Les attributs doivent etre consecutifs, et sans bloc de ratachement
	// Le nom doit etre correct et unique parmi les noms de bloc et d'attribut
	// Renvoie le block cree, appartenant a l'appele
	KWAttributeBlock* CreateAttributeBlock(const ALString& sBlockName, KWAttribute* firstAttribute,
					       KWAttribute* lastAttribute);

	// Ajout d'un attribut en fin d'un bloc existant
	// Renvoie true si OK, false sinon (attribut ou bloc existant)
	boolean InsertAttributeInBlock(KWAttribute* attribute, KWAttributeBlock* attributeBlockRef);

	// Recherche d'un bloc par son nom
	// Retourne NUL si bloc inexistant
	KWAttributeBlock* LookupAttributeBlock(const ALString& sBlockName) const;

	// Supression ou destruction d'un bloc, sans impact sur ses attributs
	void RemoveAttributeBlock(KWAttributeBlock* attributeBlock);
	void DeleteAttributeBlock(KWAttributeBlock* attributeBlock);

	// Destruction de tous les blocks d'attributs
	void DeleteAllAttributeBlocks();

	// Debut et fin de liste
	KWAttributeBlock* GetHeadAttributeBlock() const;
	KWAttributeBlock* GetTailAttributeBlock() const;

	// Precedent et suivant
	void GetNextAttributeBlock(KWAttributeBlock*& attributeBlock) const;
	void GetPrevAttributeBlock(KWAttributeBlock*& attributeBlock) const;

	//////////////////////////////////////////////////////////////
	// Services avances de deplacement d'un attribut ou d'un bloc

	// Deplacement d'un attribut (hors bloc) avant ou apres un attribut existant
	void MoveAttributeBefore(KWAttribute* attribute, KWAttribute* attributeRef);
	void MoveAttributeAfter(KWAttribute* attribute, KWAttribute* attributeRef);

	// Deplacement d'un attribut (hors bloc) vers la fin de la classe
	void MoveAttributeToClassTail(KWAttribute* attribute);

	// Deplacement d'un attribut (d'un bloc) vers la fin de son bloc
	void MoveAttributeToBlockTail(KWAttribute* attribute);

	// Deplacement d'un bloc et de tous ses attributs vers la fin de la classe
	void MoveAttributeBlockToClassTail(KWAttributeBlock* attributeBlock);

	///////////////////////////////////////////////////////////
	// Services divers

	// Compilation d'une classe (et indexation)
	// Il s'agit de la compilation de ses regles de derivation,
	// relativement au domaine de la classe, ainsi que du calcul
	// des acces par index aux attributs de la classe
	// Attention, des qu'une modification intervient sur la classe
	// ou sur un de ses attributs, la classe doit etre recompilee
	// Cette methode ne devrait etre utilisee que dans le cas de domaine
	// contenant une seule classe. Sinon, en cas de plusieurs classes
	// dans une meme domaine avec potentiellement des dependances entre classe
	// via des regles de derivation, il est preferable de passer par
	// la methode de compilation du domaine de classes.
	void Compile();
	boolean IsCompiled() const;

	// Fraicheur de compilation
	int GetCompileFreshness() const;

	// Mise a jour de la fraicheur de la classe
	// (provoque la necessite de recompiler)
	int GetFreshness() const;
	void UpdateFreshness();

	// Acces au domaine de classe contenant la classe
	KWClassDomain* GetDomain() const;

	// Calcul d'un nouveau nom d'attribut ou de bloc a partir d'un prefix
	const ALString BuildAttributeName(const ALString& sPrefix);
	const ALString BuildAttributeBlockName(const ALString& sPrefix);

	// Simplification d'une classe en supprimant les attributs derives
	// non utilises directement ou indirectement, y compris dans une
	// autre classe du domaine.
	// Si le domaine de reference n'est pas NULL, les attributs doivent
	// etre absent de ce dernier pour etre supprimes
	// Prerequis: la classe doit etre compilee (elle ne le sera plus apres)
	void DeleteUnusedDerivedAttributes(const KWClassDomain* referenceDomain);

	// Completion eventuelle des attributs avec les informations de type de leur regle de derivation
	void CompleteTypeInfo();

	// Calcul de l'ensemble des classes utilisees recursivement par les attributs de la classe courante (y compris
	// la classe courante) Prerequis: la classe doit etre compilee Memoire: le tableau du code retour appartient a
	// l'appelant, et contient des references aux classes utilisees
	void BuildAllUsedClasses(ObjectArray* oaUsedClasses) const;

	// Export des noms des champs natifs (stockes et non calcules, utilises ou non), dans l'ordre du dictionnaire
	// (utile pour constituer une ligne de header) Il peut s'agir d'attributs denses natifs ou de blocs d'attributs
	// non calcules
	void ExportNativeFieldNames(StringVector* svNativeFieldNames) const;

	// Export des noms des champs stockes (et loades), dans l'ordre du dictionnaire (utile pour constituer une ligne
	// de header) Il peut s'agir d'attributs denses ou de blocs d'attributs
	void ExportStoredFieldNames(StringVector* svStoredFieldNames) const;

	// Export des noms des attributs cles dans l'ordre des cles
	void ExportKeyAttributeNames(StringVector* svAttributedNames) const;

	// Recopie, uniquemet a partir d'une classe vide
	// Toute la description (attributs, derivations sont dupliques,
	// les KWClasses referencees sont les memes
	// La classe dupliquee n'est pas affectee a un domaine ni compilee
	void CopyFrom(const KWClass* aSource);

	// Duplication, basee sur la recopie
	KWClass* Clone() const;

	// Verification de l'integrite de la classe
	// Il s'agit d'une verification complete, qui doit etre
	// effectuee quand la classe et l'ensemble de ses
	// classes referencees sont entierement definies
	boolean Check() const override;

	// Memoire utilisee par la classe
	longint GetUsedMemory() const override;

	// Cle de hashage de la classe et de sa composition
	longint ComputeHashValue() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	// Gestion d'un rapport JSON

	// Ecriture du contenu d'un rapport JSON
	virtual void WriteJSONFields(JSONFile* fJSON);

	// Ecriture d'un rapport JSON
	// Avec une cle s'il est dans un objet, sans cle s'il est dans un tableau
	virtual void WriteJSONReport(JSONFile* fJSON);
	virtual void WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey);

	////////////////////////////////////////////////////////////////
	// Gestion du format externe des Name
	// Un identifiant peut contenir n'importe quel caractere.
	// En externe, il doit etre encadre par des backquote s'il
	// n'est de type Identifier(alphanumerique)

	// Taille limite des noms de variables
	static int GetNameMaxLength();

	// Controles de validite avec emission de message d'erreur
	// par la classe passee en parametre (pas de message si NULL)
	static boolean CheckName(const ALString& sValue, const Object* errorSender);
	static boolean CheckLabel(const ALString& sValue, const Object* errorSender);

	// Methode similaire avec message est alimente avec la cause de l'erreur
	static boolean CheckNameWithMessage(const ALString& sValue, ALString& sMessage);
	static boolean CheckLabelWithMessage(const ALString& sValue, ALString& sMessage);

	// Extraction d'une sous-chaine valide pour le format utf8
	static ALString BuildUTF8SubString(const ALString sValue);

	// Test si une chaine a une structure d'identifier
	static boolean IsStringIdentifier(const ALString& sValue);

	// Format externe d'un nom: tel quel s'il a une structure d'identifier,
	// entre back-quotes sinon, avec doublement des back-quotes internes
	static ALString GetExternalName(const ALString& sValue);

	// Format externe d'une valeur Symbol:
	// Ajout de doubles quotes autour de la chaine, et de '\' devant
	// les doubles quotes et les back-slash
	static ALString GetExternalSymbolConstant(const Symbol& sValue);

	// Format externe d'une valeur Continuous:
	// Standard, sauf pour la valeur manquante, codee par #Missing
	// dans les dictionnaires
	static ALString GetExternalContinuousConstant(Continuous cValue);

	/////////////////////////////////////////
	// Fonctionnalites de test

	// Creation d'un dictionnaire
	static KWClass* CreateClass(const ALString& sClassName, int nKeySize, int nSymbolNumber, int nContinuousNumber,
				    int nDateNumber, int nTimeNumber, int nTimestampNumber, int nTimestampTZNumber,
				    int nTextNumber, int nTextListNumber, int nObjectNumber, int nObjectArrayNumber,
				    int nStructureNumber, boolean bCreateAttributeBlocks, KWClass* attributeClass);

	// Test des parcours des attributs avec affichage
	void TestAttributeBrowsings(boolean bList, boolean bInverseList, boolean bUsed, boolean bLoaded,
				    boolean bTypes);

	// Methode de test principale
	static void Test();

	/////////////////////////////////////////////////////////////////
	//// Implementation

	// Methode avancees

	// Recherche d'un element de donne par son nom
	// Retourne NUL si attribut et bloc inexistant
	KWDataItem* LookupDataItem(const ALString& sDataItemName) const;

	// Nombre total de valeurs (attributs denses et de blocs) chargees en memoire
	int GetLoadedDataItemNumber() const;

	// Acces a un element de donnes par index de chargement
	KWDataItem* GetLoadedDataItemAt(int nIndex) const;

	// Acces a un attribut dense ou un bloc selon la nature dense ou sparse de l'index de chargement
	KWAttribute* GetAttributeAtLoadIndex(KWLoadIndex liIndex) const;
	KWAttributeBlock* GetAttributeBlockAtLoadIndex(KWLoadIndex liIndex) const;

	// Acces generique a un attribut dense ou un bloc selon  son index de chargement
	KWDataItem* GetDataItemAtLoadIndex(KWLoadIndex liIndex) const;

protected:
	///////////////////////////////////////////////////////////////////////////////////////////////
	// Services pour l'optimisation des la gestion des KWObjet par les KWDatabase

	// Seule la classe KWDatabase a l'usage des deux listes ci-dessous
	friend class KWDatabase;

	// Liste des elements de donnees devant etre calcules
	ObjectArray* GetDatabaseDataItemsToCompute();
	const ObjectArray* GetConstDatabaseDataItemsToCompute() const;

	// Liste des elements de donnees temporaires � calculer, pouvant etre nettoyes et recalcules plusieurs fois
	ObjectArray* GetDatabaseTemporayDataItemsToComputeAndClean();
	const ObjectArray* GetConstDatabaseTemporayDataItemsToComputeAndClean() const;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Services internes de gestion de la classe

	// Seule la classe KWClassDomain peut modifier
	// le domaine de classe gerant la classe
	friend class KWClassDomain;

	// Les classes KWObject et KWAttribute ont acces aux caracteristiques des attributs natifs non utilises, mais
	// charges en memoire pour des raisons internes
	friend class KWObject;
	friend class KWAttribute;

	// Parametrage specifique de toutes les regles Random utilisees dans la classe
	void InitializeAllRandomRuleParameters();

	// Parametrage d'une regle si elle est de type random, avec propagation a ses operandes
	// L'entier nRuleRankInAttribute est incremente a chaque nouvelle regle Random rencontree
	void InitializeRandomRuleParameters(KWDerivationRule* rule, const ALString& sAttributeName,
					    int& nRuleRankInAttribute);

	// Verification de l'integrite de la classe en ce qui concerne sa composition
	// Il ne doit pas y avoir de cycle dans le graphe des utilisation entre classes par composition
	// La taille des cles doit etre croissante avec la profondeur d'utilisation dans la composition
	// L'attribut parent en parametre (potentiellement NULL) fournit des informations sur la profondeur
	// dans l'arbre de composition, ce qui permet de tester la coherence de longueur des cle
	// Le dictionnaire de classes en parametre permet de detecter les cycles
	boolean CheckClassComposition(KWAttribute* parentAttribute, NumericKeyDictionary* nkdComponentClasses) const;

	// Attributs Symbol charges en memoire, pour optimiser leur destruction et mutation
	// Attention, seuls les attributs Symbol dense sont concernes. Les attributs faisant
	// faisant partie de bloc de type SymbolValueBlock sont geres par leur bloc
	int GetLoadedDenseSymbolAttributeNumber() const;
	KWAttribute* GetLoadedDenseSymbolAttributeAt(int nIndex) const;

	// Attributs Text charges en memoire, pour optimiser leur destruction et mutation
	int GetLoadedTextAttributeNumber() const;
	KWAttribute* GetLoadedTextAttributeAt(int nIndex) const;

	// Attributs TextList charges en memoire, pour optimiser leur destruction et mutation
	int GetLoadedTextListAttributeNumber() const;
	KWAttribute* GetLoadedTextListAttributeAt(int nIndex) const;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes internes utilises en particulier dans les KWObject, pour s'assurer de la
	// coherence entre les valeurs d'un KWObject et les attributs charges en memoire
	// et pour gerer efficament les attribut dense et les blocs d'attributs

	// Methode interne de verification qu'un LoadIndex correspond a un attribut du bon type
	boolean CheckTypeAtLoadIndex(KWLoadIndex liIndex, int nType) const;

	// Nombre total de valeurs (attributs denses et de blocs) chargees en memoire,
	// plus les attributs Object et ObjectArray natifs a stocker en memoire
	int GetTotalInternallyLoadedDataItemNumber() const;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Gestion des attributs Object et ObjectArray natifs, non utilises (not Used) et non
	// charges en memoire (not Loaded) selon les specifications.
	// Il faut prevoir de les charger en interne au cas ou ils serait utilises comme arguments de regles
	// de derivation renvoyant des Object ou ObjectArray. En effet, dans ce cas, les attributs
	// issu de regles peuvent referencer des Object natifs, qui ne doivent pas etre detruits.

	// Attributs Relation natifs non charges en memoire
	int GetUnloadedNativeRelationAttributeNumber() const;
	KWAttribute* GetUnloadedNativeRelationAttributeAt(int nIndex) const;

	// Prise en compte des attributs utilises non charges en memoire suite a une lecture de dictionnaire (cf.
	// KWAttribute::ReadNotLoadedMetaData)
	void ReadNotLoadedMetaData();

	// Nom de la classe
	KWCDUniqueString usName;

	// Meta-donnees
	KWMetaData metaData;

	// Libelle
	KWCDUniqueString usLabel;

	// Statut racine ou composant
	boolean bRoot;

	// Nom des attributs cles (potentiellement specifies avant la specification des attributs de la classe)
	StringVector svKeyAttributeNames;

	// Index de chargement en memoire des champs de la cle
	KWLoadIndexVector livKeyAttributeLoadIndexes;

	// Domaine de classe
	KWClassDomain* domain;

	// Gestion des attributs sous forme de liste et dictionnaire
	// Les attribut appartiennent au dictionnaire et sont references par la liste
	ObjectDictionary odAttributes;
	ObjectList olAttributes;

	// Dictionnaire des blocs utilises
	// Les blocs appartiennent directement a leurs attribut et sont seulement reference par leur dictionnaire
	// Les blocs peuvent etre detruits explicitement; il sont egalement detruit quand leur dernier attribut est
	// detruit Attention, les noms d'attributs et de blocs, geres par deux dictionnaire, doivent etre uniques
	// collectivement
	ObjectDictionary odAttributeBlocks;

	// Nombre de blocs, d'attributs natifs denses ou sparses, utilises ou non
	int nNativeAttributeNumber;
	int nNativeAttributeBlockNumber;

	// Acces indexes aux attributs
	ObjectArray oaUsedAttributes;
	ObjectArray oaLoadedAttributes;
	ObjectArray oaLoadedDenseAttributes;
	ObjectArray oaLoadedAttributeBlocks;
	ObjectArray oaLoadedDenseSymbolAttributes;
	ObjectArray oaLoadedTextAttributes;
	ObjectArray oaLoadedTextListAttributes;
	ObjectArray oaLoadedRelationAttributes;
	ObjectArray oaUnloadedNativeRelationAttributes;

	// Tableau de tous les valeurs denses (attributs dense ou blocs) charges en memoire
	// pour permettre une verification des LoadIndex, dont la partie DenseIndex
	// correspond a l'index dans ce tableau
	ObjectArray oaLoadedDataItems;

	// Statistiques par type d'attributs utilises
	IntVector ivUsedAttributeNumbers;
	IntVector ivUsedDenseAttributeNumbers;
	IntVector ivUsedSparseAttributeNumbers;

	// Listes d'attributs dont l'usage est reserve a la classe KWDatabase
	ObjectArray oaDatabaseDataItemsToCompute;
	ObjectArray oaDatabaseTemporayDataItemsToComputeAndClean;

	// Valeur de hash de la classe, bufferise avec une fraicheur
	// Ces variables sont mutable, car modifiee par ComputeHashValue()
	mutable longint lClassHashValue;
	mutable int nHashFreshness;

	// Fraicheur de la classe et de compilation
	int nFreshness;
	int nIndexFreshness;
	int nCompileFreshness;
};

#include "KWAttribute.h"
#include "KWClassDomain.h"
#include "KWDerivationRule.h"

////////////////////////////////////////////////////////
// methodes en inline

inline const ALString& KWClass::GetName() const
{
	return usName.GetValue();
}

inline void KWClass::SetName(const ALString& sValue)
{
	require(domain == NULL);

	usName.SetValue(sValue);
	UpdateFreshness();
}

inline const KWMetaData* KWClass::GetConstMetaData() const
{
	return &metaData;
}

inline KWMetaData* KWClass::GetMetaData()
{
	return &metaData;
}

inline const ALString& KWClass::GetLabel() const
{
	return usLabel.GetValue();
}

inline void KWClass::SetLabel(const ALString& sValue)
{
	require(CheckLabel(sValue, this));
	usLabel.SetValue(sValue);
}

inline boolean KWClass::GetRoot() const
{
	return bRoot;
}

inline void KWClass::SetRoot(boolean bValue)
{
	bRoot = bValue;
	UpdateFreshness();
}

inline int KWClass::GetKeyAttributeNumber() const
{
	return svKeyAttributeNames.GetSize();
}

inline const ALString& KWClass::GetKeyAttributeNameAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetKeyAttributeNumber());
	return svKeyAttributeNames.GetAt(nIndex);
}

inline KWAttribute* KWClass::GetKeyAttributeAt(int nIndex) const
{
	require(Check());
	return cast(KWAttribute*, odAttributes.Lookup(GetKeyAttributeNameAt(nIndex)));
}

inline boolean KWClass::IsKeyLoaded() const
{
	require(IsCompiled());
	ensure(livKeyAttributeLoadIndexes.GetSize() == 0 or
	       livKeyAttributeLoadIndexes.GetSize() == GetKeyAttributeNumber());
	return livKeyAttributeLoadIndexes.GetSize() > 0;
}

inline KWLoadIndex KWClass::GetKeyAttributeLoadIndexAt(int nIndex) const
{
	require(IsKeyLoaded());
	require(0 <= nIndex and nIndex < GetKeyAttributeNumber());
	return livKeyAttributeLoadIndexes.GetAt(nIndex);
}

inline int KWClass::GetAttributeNumber() const
{
	return olAttributes.GetCount();
}

inline KWAttribute* KWClass::LookupAttribute(const ALString& sAttributeName) const
{
	return cast(KWAttribute*, odAttributes.Lookup(sAttributeName));
}

inline boolean KWClass::IsIndexed() const
{
	return nIndexFreshness == nFreshness;
}

inline int KWClass::GetUsedAttributeNumber() const
{
	require(IsIndexed());
	return oaUsedAttributes.GetSize();
}

inline KWAttribute* KWClass::GetUsedAttributeAt(int nIndex) const
{
	require(IsIndexed());
	return cast(KWAttribute*, oaUsedAttributes.GetAt(nIndex));
}

inline int KWClass::GetLoadedAttributeNumber() const
{
	require(IsIndexed());
	return oaLoadedAttributes.GetSize();
}

inline KWAttribute* KWClass::GetLoadedAttributeAt(int nIndex) const
{
	require(IsIndexed());
	return cast(KWAttribute*, oaLoadedAttributes.GetAt(nIndex));
}

inline int KWClass::GetLoadedDenseAttributeNumber() const
{
	require(IsIndexed());
	return oaLoadedDenseAttributes.GetSize();
}

inline KWAttribute* KWClass::GetLoadedDenseAttributeAt(int nIndex) const
{
	require(IsIndexed());
	return cast(KWAttribute*, oaLoadedDenseAttributes.GetAt(nIndex));
}

inline int KWClass::GetLoadedAttributeBlockNumber() const
{
	require(IsIndexed());
	return oaLoadedAttributeBlocks.GetSize();
}

inline KWAttributeBlock* KWClass::GetLoadedAttributeBlockAt(int nIndex) const
{
	require(IsIndexed());
	return cast(KWAttributeBlock*, oaLoadedAttributeBlocks.GetAt(nIndex));
}

inline int KWClass::GetLoadedRelationAttributeNumber() const
{
	require(IsIndexed());
	return oaLoadedRelationAttributes.GetSize();
}

inline KWAttribute* KWClass::GetLoadedRelationAttributeAt(int nIndex) const
{
	require(IsIndexed());
	return cast(KWAttribute*, oaLoadedRelationAttributes.GetAt(nIndex));
}

inline int KWClass::GetUsedAttributeNumberForType(int nType) const
{
	require(IsIndexed());
	require(0 <= nType and nType < KWType::None);
	return ivUsedAttributeNumbers.GetAt(nType);
}

inline int KWClass::GetUsedDenseAttributeNumberForType(int nType) const
{
	require(IsIndexed());
	require(0 <= nType and nType < KWType::None);
	return ivUsedDenseAttributeNumbers.GetAt(nType);
}

inline int KWClass::GetUsedSparseAttributeNumberForType(int nType) const
{
	require(IsIndexed());
	require(0 <= nType and nType < KWType::None);
	return ivUsedSparseAttributeNumbers.GetAt(nType);
}

inline int KWClass::GetNativeDataItemNumber() const
{
	require(IsIndexed());
	return nNativeAttributeBlockNumber + nNativeAttributeNumber;
}

inline int KWClass::GetLoadedDenseSymbolAttributeNumber() const
{
	require(IsIndexed());
	return oaLoadedDenseSymbolAttributes.GetSize();
}

inline KWAttribute* KWClass::GetLoadedDenseSymbolAttributeAt(int nIndex) const
{
	require(IsIndexed());
	return cast(KWAttribute*, oaLoadedDenseSymbolAttributes.GetAt(nIndex));
}

inline int KWClass::GetLoadedTextAttributeNumber() const
{
	require(IsIndexed());
	return oaLoadedTextAttributes.GetSize();
}

inline KWAttribute* KWClass::GetLoadedTextAttributeAt(int nIndex) const
{
	require(IsIndexed());
	return cast(KWAttribute*, oaLoadedTextAttributes.GetAt(nIndex));
}

inline int KWClass::GetLoadedTextListAttributeNumber() const
{
	require(IsIndexed());
	return oaLoadedTextListAttributes.GetSize();
}

inline KWAttribute* KWClass::GetLoadedTextListAttributeAt(int nIndex) const
{
	require(IsIndexed());
	return cast(KWAttribute*, oaLoadedTextListAttributes.GetAt(nIndex));
}

inline int KWClass::GetLoadedDataItemNumber() const
{
	require(IsIndexed());
	assert(oaLoadedDenseAttributes.GetSize() + oaLoadedAttributeBlocks.GetSize() == oaLoadedDataItems.GetSize());
	return oaLoadedDataItems.GetSize();
}

inline KWDataItem* KWClass::GetLoadedDataItemAt(int nIndex) const
{
	require(IsIndexed());
	return cast(KWDataItem*, oaLoadedDataItems.GetAt(nIndex));
}

inline KWAttribute* KWClass::GetAttributeAtLoadIndex(KWLoadIndex liIndex) const
{
	require(liIndex.IsDense());
	return cast(KWAttribute*, oaLoadedDataItems.GetAt(liIndex.GetDenseIndex()));
}

inline KWAttributeBlock* KWClass::GetAttributeBlockAtLoadIndex(KWLoadIndex liIndex) const
{
	require(liIndex.IsValid());
	return cast(KWAttributeBlock*, oaLoadedDataItems.GetAt(liIndex.GetDenseIndex()));
}

inline KWDataItem* KWClass::GetDataItemAtLoadIndex(KWLoadIndex liIndex) const
{
	require(liIndex.IsValid());
	return cast(KWDataItem*, oaLoadedDataItems.GetAt(liIndex.GetDenseIndex()));
}

inline ObjectArray* KWClass::GetDatabaseDataItemsToCompute()
{
	return &oaDatabaseDataItemsToCompute;
}

inline const ObjectArray* KWClass::GetConstDatabaseDataItemsToCompute() const
{
	return &oaDatabaseDataItemsToCompute;
}

inline ObjectArray* KWClass::GetDatabaseTemporayDataItemsToComputeAndClean()
{
	return &oaDatabaseTemporayDataItemsToComputeAndClean;
}

inline const ObjectArray* KWClass::GetConstDatabaseTemporayDataItemsToComputeAndClean() const
{
	return &oaDatabaseTemporayDataItemsToComputeAndClean;
}

inline int KWClass::GetTotalInternallyLoadedDataItemNumber() const
{
	require(IsIndexed());
	return oaLoadedDataItems.GetSize() + oaUnloadedNativeRelationAttributes.GetSize();
}

inline int KWClass::GetUnloadedNativeRelationAttributeNumber() const
{
	require(IsIndexed());
	return oaUnloadedNativeRelationAttributes.GetSize();
}

inline KWAttribute* KWClass::GetUnloadedNativeRelationAttributeAt(int nIndex) const
{
	return cast(KWAttribute*, oaUnloadedNativeRelationAttributes.GetAt(nIndex));
}

inline boolean KWClass::IsCompiled() const
{
	return nFreshness == nCompileFreshness;
}

inline int KWClass::GetCompileFreshness() const
{
	return nCompileFreshness;
}

inline int KWClass::GetFreshness() const
{
	return nFreshness;
}

inline void KWClass::UpdateFreshness()
{
	nFreshness++;
}

inline KWClassDomain* KWClass::GetDomain() const
{
	return domain;
}
