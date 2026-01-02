// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataPath;
class KWDataPathManager;
class KWClassDomain;
class KWClass;

#include "Object.h"
#include "Vector.h"
#include "KWLoadIndex.h"

////////////////////////////////////////////////////////////
// Classe KWDataPath
//
// Dans un schema multi-table, chaque data path fait reference a une variable de type Table ou Entity
//  et identifie un fichier de table de donnees pour la sous-partie du schema
// La table principale a un data path vide.
// Dans un schema en etoile,
//  les data paths sont les noms de  variable de type Table ou Entity de chaque table secondaire.
// Dans un schema en flocon,
//   les data paths  consistent en une liste de noms de variables avec un separateur "/".
// Pour les tables externes,
//   les data paths commencent par un un nom de dictionnaire Root prefixe par "/"
// Exemples de DataPath pour differents types de mapping:
//    classe principale:
//    sous-objet: Address
//    tableau de sous-objets: Logs
//    Sous-objet de second niveau: Address/City
//    table externe: /City
//
// Un composant de data path peut contenir des caractere separateur '/'.
// S'il contient des caracteres '/' ou '`', il doit etre formate a l'aide de la methode
// GetFormattedName pour le mettre entre '`', comme pour le format externes des noms de variables
//
// La notion data path d'un schema multi-table s'applique a tous les objets (KWObject)
// - objets lus depuis les fichiers de la base multi-tables
// - objets crees par des regles de creation d'instances
// En effet, qu'il soient lus ou crees en memoire, la base reste hierarchique,
// et chaque objet est rattache a sa racine via un chemin unique, ayant pour racine soit
// le dictionnaire principal de la base de donnees en cours de traitement, soit
// un dictionnaire racine d'une table externe.
//
// La classe KWDataPath reunit les fonctionnalite communes aux deux type d'usages, pour
// specialisation dans deux-sous-classes
// - KWMTDatabaseMapping: mapping des data paths sur des fichiers de donnees, pour gerer
//   les objets lus dans une base de donneee
// - KWObjectDataPath: gestion en memoire de tous types d'objets, qu'il soit issus d'une lecture
//   depuis un fichier ou crees par une regle de derivation de creation d'instance
class KWDataPath : public Object
{
public:
	// Constructeur
	KWDataPath();
	~KWDataPath();

	////////////////////////////////////////////////////////////////
	// Gestion des objet cree par des regles de creation d'instances
	// Methode virtuelles definie pour des raisons de genericite

	// Indique si la classe gere les data paths correspondant a des instances crees par des regle de creation d'instance
	// Defaut: false, signifie que les data path ne correspondent qu'a des instances stockes dans des fichier
	virtual boolean IsRuleCreationManaged() const;

	// Indique si le data path correspond a des objets crees par des regles de creation d'instances
	// Defaut: false, correspondant a des objet stockes dans des fichiers
	// Le setter est defini, mais son utilisation est interdite dans cette classe
	virtual boolean GetCreatedObjects() const;
	virtual void SetCreatedObjects(boolean bValue);

	////////////////////////////////////////////////////////////////
	// Data path, identifiant calcule a partir de la specification
	// Les elements du data path sont formattes si necessaire,
	// dans le cas ou ils contiennt des caracteres '`' ou '/'

	// Data path, calcule d'apres les specifications
	ALString GetDataPath() const;

	// Data path parent a la profondeur donne, entre 0 (courant) et le nombre d'attribut du data path (origine)
	ALString GetParentDataPathAt(int nDepth) const;

	// Partie du data path relative aux attributs, calculee d'apres les specifications
	ALString GetDataPathAttributeNames() const;

	////////////////////////////////////////////////////////////////
	// Specification du data path

	// Table externe (defaut: false)
	boolean GetExternalTable() const;
	void SetExternalTable(boolean bValue);

	// Nom du dictionnaire origine du data path
	// - dictionnaire Root si on est dans le cas d'une table externe
	// - dictionnaire principal sinon
	const ALString& GetOriginClassName() const;
	void SetOriginClassName(const ALString& sValue);

	// Nom des attribut du data paths
	StringVector* GetAttributeNames();

	// Nom du dictionaire decrivant les objets a l'extremite du data path
	const ALString& GetClassName() const;
	void SetClassName(const ALString& sValue);

	////////////////////////////////////////////////////////////
	// Decomposition du DataPath

	// Attributs du DataPath
	int GetDataPathAttributeNumber() const;
	const ALString& GetDataPathAttributeNameAt(int nIndex) const;

	// Type des attributs du DataPath
	int GetDataPathAttributeTypeAt(int nIndex) const;

	// Recherche si l'attribut terminal du DataPath est Used, c'est a dire
	// si tous les attributs intermediaires sont Used
	// Utile notamment en ecriture, pour savoir si l'objet sera ecrit
	boolean IsTerminalAttributeUsed() const;

	// Conversion d'un element de data path vers le format externe
	// Cela concerne les noms de dictionnaire ou de variable
	// S'il contiennent le caractere '`' ou le separateur '/', il doivent
	// etre mis au format externe defini pour les dictionnaires, entre '`'
	static ALString GetFormattedName(const ALString& sValue);

	// Separateur utilise dans les data paths
	static char GetDataPathSeparator();

	// Separateur d'echappement pour formatter les nom contenant le separateur ou le character d'echappement
	static char GetDataPathEscapeChar();

	////////////////////////////////////////////////////////
	// Acces aux data paths fils du data path

	// Tableau des data paths fils decrivant les sous-objects (Entity) ou tableaux de sous-objets (Table)
	// decrivant la sous structure (pas les references aux tables externes)
	// Memoire: le tableau appartient a l'appele, son contenu est a gerer par l'appelant
	ObjectArray* GetComponents();
	const ObjectArray* GetConstComponents() const;

	// Collecte de tous les data path de la hierarchie de composition,
	// y compris le data path courant (data path de la hierarchie)
	void CollectFullHierarchyComponents(ObjectArray* oaResults);

	////////////////////////////////////////////////////////
	// Services de dimensionnement pour des data path de creation d'instances
	// Ne s'applique qu'aux data path correspondant a des objects stockes, en
	// explorant dans les data paths de leur hierarchie correspondant a des
	// objects crees par des regles de derivation

	// Estimation heuristique de la memoire necessaire pour creer les sous objets
	// de la hierarchie par des regles de creation d'instances
	// Entree :
	// - un data path correspondant a un objet stocke
	// Sortie :
	// - estimation de la memoire pour les objets a creer, en exemplaire unique, ou via des tables
	// - estimation indicative du nombre total d'instances creees, en exemplaire unique
	// Les objets uniques sont issus des debut de branches ne comportant que des variable de type Entity
	// Des que l'on passe par une variable de type Tablle, la sous-branche est systematiquement
	// en mode exemplaire multiple.
	// L'estimation est effectuee dans la hierarchie des data paths dediee a la creation
	// d'instances, en n'explorant pas les sous-branches commencant par un data path
	// dedie a des instances stockees
	void ComputeEstimatedMemoryForObjectCreation(KWClassDomain* classDomain,
						     longint& lEstimatedMemoryForSingleObjectCreation,
						     longint& lEstimatedMemoryForMultipleObjectCreation,
						     longint& lEstimatedTotalCreatedSingleObjectNumber,
						     longint& lEstimatedTotalCreatedMultipleObjectNumber) const;

	////////////////////////////////////////////////////////
	// Divers

	// Copie, uniquement des attributs de base de la specification
	// Les attributs de composition ou de gestion interne sont reinitialises
	virtual void CopyFrom(const KWDataPath* aSource);

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplemente dans les sous-classes
	virtual KWDataPath* Create() const;

	// Duplication
	KWDataPath* Clone() const;

	// Comparaison des attributs de definition
	virtual int Compare(const KWDataPath* aSource) const;

	// Validite de la specification du data path
	boolean Check() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Rapport synthetique destine a rentrer dans un tableau
	// Ces methodes ne doivent pas creer de retour charriot ('\n') en fin de ligne, de facon a permettre
	// leur reimplementation. Ces retours charriot sont a generer par les methodes qui les utilisent.
	virtual void WriteHeaderLineReport(ostream& ost) const;
	virtual void WriteLineReport(ostream& ost) const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Memoire utilisee par le mapping
	longint GetUsedMemory() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KWDataPathManager;

	// Gestionnaire de DataPath
	const KWDataPathManager* GetDataPathManager() const;
	void SetDataPathManager(const KWDataPathManager* manager);

	// Attributs de la classe
	boolean bExternalTable;
	ALString sOriginClassName;
	StringVector svAttributeNames;
	IntVector ivAttributeTypes;
	ALString sClassName;
	ObjectArray oaComponents;

	// Gestionnaire de data path
	const KWDataPathManager* dataPathManager;
};

////////////////////////////////////////////////////////////
// Classe KWDataPathManager
// Gestion generique de l'ensemble des data path pour une base de donnees
class KWDataPathManager : public Object
{
public:
	// Constructeur
	KWDataPathManager();
	~KWDataPathManager();

	// Indique que la classe gere les data paths correspondant a des instances crees par des regle de creation d'instance
	boolean IsRuleCreationManaged() const;

	// Calcul de tous les data path a partir du dictionnaire principal
	// d'une base multi-table
	virtual void ComputeAllDataPaths(const KWClass* mainClass);

	// Reinitialisation
	void Reset();

	////////////////////////////////////////////////////////
	// Acces aux resultats d'analyse

	// Nom de la classe principale analysee, celle du data path principal
	const ALString GetMainClassName() const;

	// Acces a tous les data paths
	int GetDataPathNumber() const;
	const KWDataPath* GetDataPathAt(int nIndex) const;

	// Data path principal
	const KWDataPath* GetMainDataPath() const;

	// Acces au data path Root des tables externes
	int GetExternalRootDataPathNumber() const;
	const KWDataPath* GetExternalRootDataPathAt(int nIndex) const;

	// Acces a un mapping par son chemin
	const KWDataPath* LookupDataPath(const ALString& sDataPath) const;

	// Warnings pour le cas des tables externes non utilisees, gardees pour generer des warnings
	// lors du Check d'une base
	//
	// Ce cas arrive si la table principale est une table secondaire d'une table externe qu'elle reference.
	// Par exemple, dans le cas d'un schema de molecules avec des atomes et des liaisons, les atomes et les liaisons
	// referencent leur molecule pour pouvoir recreer dynamiquement le graphe de la structure de la molecule, avec chaque
	// liaison referencant ses atomes extremites et chaque atome referencant ses liaisons adjacentes.
	// Si on choisit les atomes en table d'analyse principale, on doit couper le lien avec la table des molecules
	// pour eviter les cycles dans les donnees
	const StringVector* GetUnusedRootDictionaryWarnings() const;

	////////////////////////////////////////////////////////
	// Divers

	// Copie
	virtual void CopyFrom(const KWDataPathManager* aSource);

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplemente dans les sous-classes
	virtual KWDataPathManager* Create() const;

	// Duplication
	KWDataPathManager* Clone() const;

	// Comparaison
	virtual int Compare(const KWDataPathManager* aSource) const;

	// Verification d'une specification complete
	boolean Check() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Memoire utilisee par le mapping
	longint GetUsedMemory() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Calcul recursif des data paths
	// Le data path est une chaine avec ses data paths composants.
	// Le tableau exhaustif des data paths est egalement egalement mis a jour
	// Les classes referencees sont memorisees dans un dictionnaire et un tableau,
	// pour gerer les data paths externes a creer ulterieurement
	// Les data paths crees recursivement sont memorises dans un tableau
	// Les classes creees analysees sont egalement memorisees dans un dictionnaire, pour eviter des analyses multiples
	KWDataPath* CreateDataPath(ObjectDictionary* odReferenceClasses, ObjectArray* oaRankedReferenceClasses,
				   ObjectDictionary* odAnalysedCreatedClasses, const KWClass* mappedClass,
				   boolean bIsExternalTable, boolean bCreatedObjects, const ALString& sOriginClassName,
				   StringVector* svAttributeNames, IntVector* ivAttributeTypes,
				   ObjectArray* oaCreatedDataPaths);

	// Affichage d'un tableau de data paths
	void WriteDataPathArray(ostream& ost, const ALString& sTitle, const ObjectArray* oaDataPathArray) const;

	// Data path principal
	KWDataPath* mainDataPath;

	// Tableau de tous les data paths
	ObjectArray oaDataPaths;

	// Tableau des data paths Root des tables externes
	ObjectArray oaExternalRootDataPaths;

	// Dictionnaire des data paths
	ObjectDictionary odDataPaths;

	// Warning lie aux cycle d'utilisation dans les data paths
	StringVector svUnusedRootDictionaryWarnings;

	// Objet de type KWDataPath permettant de parametrer la creation des data path
	// Cet objet, cree dans le constructeur, peut tere redefini dans une sous-classe
	// pour specialiser la creation des data path
	KWDataPath* dataPathCreator;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KWDataPath::GetExternalTable() const
{
	return bExternalTable;
}

inline void KWDataPath::SetExternalTable(boolean bValue)
{
	bExternalTable = bValue;
}

inline const ALString& KWDataPath::GetOriginClassName() const
{
	return sOriginClassName;
}

inline void KWDataPath::SetOriginClassName(const ALString& sValue)
{
	sOriginClassName = sValue;
}

inline StringVector* KWDataPath::GetAttributeNames()
{
	return &svAttributeNames;
}

inline int KWDataPath::GetDataPathAttributeNumber() const
{
	return svAttributeNames.GetSize();
}

inline const ALString& KWDataPath::GetDataPathAttributeNameAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetDataPathAttributeNumber());
	return svAttributeNames.GetAt(nIndex);
}

inline int KWDataPath::GetDataPathAttributeTypeAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetDataPathAttributeNumber());
	return ivAttributeTypes.GetAt(nIndex);
}

inline const ALString& KWDataPath::GetClassName() const
{
	return sClassName;
}

inline void KWDataPath::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

inline char KWDataPath::GetDataPathSeparator()
{
	return '/';
}

inline char KWDataPath::GetDataPathEscapeChar()
{
	return '`';
}

inline ObjectArray* KWDataPath::GetComponents()
{
	return &oaComponents;
}

inline const ObjectArray* KWDataPath::GetConstComponents() const
{
	return &oaComponents;
}

inline const KWDataPathManager* KWDataPath::GetDataPathManager() const
{
	return dataPathManager;
}

inline void KWDataPath::SetDataPathManager(const KWDataPathManager* manager)
{
	dataPathManager = manager;
}

inline const ALString KWDataPathManager::GetMainClassName() const
{
	if (mainDataPath == NULL)
		return "";
	else
		return mainDataPath->GetClassName();
}

inline int KWDataPathManager::GetDataPathNumber() const
{
	return oaDataPaths.GetSize();
}

inline const KWDataPath* KWDataPathManager::GetDataPathAt(int nIndex) const
{
	return cast(const KWDataPath*, oaDataPaths.GetAt(nIndex));
}

inline const KWDataPath* KWDataPathManager::GetMainDataPath() const
{
	return mainDataPath;
}

inline int KWDataPathManager::GetExternalRootDataPathNumber() const
{
	return oaExternalRootDataPaths.GetSize();
}

inline const KWDataPath* KWDataPathManager::GetExternalRootDataPathAt(int nIndex) const
{
	return cast(const KWDataPath*, oaExternalRootDataPaths.GetAt(nIndex));
}

inline const KWDataPath* KWDataPathManager::LookupDataPath(const ALString& sDataPath) const
{
	return cast(const KWDataPath*, odDataPaths.Lookup(sDataPath));
}

inline const StringVector* KWDataPathManager::GetUnusedRootDictionaryWarnings() const
{
	return &svUnusedRootDictionaryWarnings;
}
