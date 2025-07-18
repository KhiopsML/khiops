// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataPath;
class KWObjectDataPath;
class KWObjectDataPathManager;

#include "KWClass.h"

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
// le dictionnaire principale de la base de donnees en cours de traitement, soit
// un dictionnaire racine d'une table externe.
class KWDataPath : public Object
{
public:
	// Constructeur
	KWDataPath();
	~KWDataPath();

	// Copie (sans les attributs de gestion)
	virtual void CopyFrom(const KWDataPath* aSource);

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplemente dans les sous-classes
	virtual KWDataPath* Create() const;

	// Duplication
	KWDataPath* Clone() const;

	// Comparaison des attributs de definition
	virtual int Compare(const KWDataPath* aSource) const;

	////////////////////////////////////////////////////////////////
	// Data path, identifiant calcule a partir de la specification
	// Les elements du data path sont formattes si necessaire,
	// dans le cas ou ils contiennt des caracteres '`' ou '/'

	// Data path, calcule d'apres les specifications
	ALString GetDataPath() const;

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

	// Recherche si l'attribut terminal du DataPath est Used, c'est a dire
	// si tous les attributs intermediaires sont Used
	// Utile notamment en ecriture, pour savoir si l'objet sera ecrit
	boolean IsTerminalAttributeUsed() const;

	// Validite du DataPath complet
	boolean CheckDataPath() const;

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
	// Divers

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
	// Attributs de la classe
	boolean bExternalTable;
	ALString sOriginClassName;
	StringVector svAttributeNames;
	ALString sClassName;
};

////////////////////////////////////////////////////////////
// Classe KWObjectDataPath
//
// Specialisation de KWDataPath a destination des KWObject pour gerer
// un identifiant unique,que les objet soient stockes ou crees par des regles de derivation
class KWObjectDataPath : public KWDataPath
{
public:
	// Constructeur
	KWObjectDataPath();
	~KWObjectDataPath();

	// Indique si le data path correspond a des objets crees par des regles de creation d'instances
	// (defaut: false, correspondant a des objet stockes dans des fichiers)
	boolean GetCreatedObjects() const;
	void SetCreatedObject(boolean bValue);

	// Copie
	void CopyFrom(const KWDataPath* aSource) override;

	// Creation pour renvoyer une instance du meme type dynamique
	KWDataPath* Create() const override;

	////////////////////////////////////////////////////////////////////////
	// Service de navigation dans les data paths

	// Acces au data path fils pour un index d'attribut relationnel du dictionnaire extremite
	const KWObjectDataPath* GetSubDataPath(const KWLoadIndex liAttributeLoadIndex) const;

	////////////////////////////////////////////////////////////////////////
	// Service d'identification des objets d'un schema multi-table
	//
	// Identifiant unique d'un objet dans une hierarchie de donnees multi-table,
	//  que les donnees soit stockee et lue depuis des fichier ou crees en memoire
	//  via des regles de derivation de creation d'instance
	// Utilisation des CreationIndex: cf. KWObject::GetCreationIndex()
	//   - objets stockes: CreationIndex unique et reproductible, base sur un numero de ligne dans un fichier
	//   - objets crees: CreationIndex unique, base sur un compteur de creation d'instance local au data path
	// Les objets crees peuvent alors etre identifie de facon unique par le CreationIndex de leur instance
	// principale (main ou Root), et leur CreationIndex localement a cette instance stockee.

	// Reinitialisation du compteur de creation d'instance, a appeler a chaque changement
	// d'objet principal (racine de l'objet principal d'un schema multi-table), ou Root (racine dans le cas d'une table externe)
	// Cette reinitialisation est propagee a tous
	void ResetCreationNumber(longint lNewMainCreationIndex) const;

	// Index de creation principal, servant de reference aux instances crees dans son contexte
	longint GetMainCreationIndex() const;

	// Obtention d'un nouvel index de creation, a memoirser our chaque nouvel objet cree
	longint NewCreationIndex() const;

	// Acces au nombre d'objet crees
	longint GetCreationNumber() const;

	////////////////////////////////////////////////////////////////////////
	// Services de parametrage (Seed, Leap) de generateur pseudo-aleatoire par hashage du data path,
	// pour le parametrage des generateur de nombre aleatoire, avec garantie de reproductibilite
	// en calcul distribue, et d'independance entre les series aleatoire generes par differents
	// appels a la regle de derivation Random

	// Graine de generateur aleatoire
	int GetRandomSeed() const;

	// Saut de type leap frog d'un generateur aleatoire
	int GetRandomLeap() const;

	////////////////////////////////////////////////////////
	// Divers

	// Memoire utilisee par le mapping
	longint GetUsedMemory() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KWObjectDataPathManager;

	// Compilation pour avoir acces efficacement aux services avances
	void Compile(const KWClass* mainClass);
	boolean IsCompiled() const;

	// Index de chargement de la derniere variable du data path, aboutissant a son extremite
	const KWLoadIndex GetTerminalLoadIndex() const;

	// Gestionnaire de DataPath
	const KWObjectDataPathManager* GetDataPathManager() const;
	void SetDataPathManager(const KWObjectDataPathManager* manager);

	/////////////////////////////////////////////////////////////////////
	// Attributs de base

	// Indicateur de data path dedie a des objets crees par des regles de creation d'instances
	boolean bCreatedObjects;

	// Tableau des sous-data path du data path courant
	ObjectArray oaSubDataPaths;

	// Index de creation principal, servant de reference aux instances crees dans son contexte
	mutable longint lMainCreationIndex;

	// Nombre d'index de creation generes
	mutable longint lCreationNumber;

	// Gestionnaire de data path
	const KWObjectDataPathManager* dataPathManager;

	/////////////////////////////////////////////////////////////////////
	// Attributs de gestion de la compilation du data path

	// Index de chargement de la derniere variable du data path, aboutissant a son extremite
	KWLoadIndex liCompiledTerminalAttributeLoadIndex;

	// Tableau des data path indexes par leur index de chargement
	// On utilise la partie dense de l'index de chargement pour avoir un acces direct a un data path
	ObjectArray oaCompiledSubDataPathsByLoadIndex;

	// Parametres pour le generateur de nombre aleatoire
	int nCompiledRandomSeed;
	int nCompiledRandomLeap;

	// Valeur de hash du data path au moment de compilation, permettant de verifier sa validite
	int nCompileHash;
};

////////////////////////////////////////////////////////////
// Classe KWObjectDataPathManager
// Gestion de l'ensemble des data path pour une base de donnees
class KWObjectDataPathManager : public Object
{
public:
	// Constructeur
	KWObjectDataPathManager();
	~KWObjectDataPathManager();

	// Calcul de tous les data path a partir du dictionnaire principal
	// d'une base multi-table
	void ComputeAllDataPaths(const KWClass* mainClass);

	// Reinitialisation
	void Reset();

	////////////////////////////////////////////////////////
	// Acces aux results d'analyse

	// Classe principale analysee
	const KWClass* GetMainClass();

	// Acces a tous les data paths
	int GetDataPathNumber() const;
	const KWObjectDataPath* GetDataPathAt(int nIndex) const;

	// Data path principal
	const KWObjectDataPath* GetMainDataPath() const;

	// Acces au data path Root des tables externes
	int GetExternalRootDataPathNumber() const;
	const KWObjectDataPath* GetExternalRootDataPathAt(int nIndex) const;

	// Acces a un maping par son chemin
	const KWObjectDataPath* LookupDataPath(const ALString& sDataPath) const;

	////////////////////////////////////////////////////////
	// Divers

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
	KWObjectDataPath* CreateDataPath(ObjectDictionary* odReferenceClasses, ObjectArray* oaRankedReferenceClasses,
					 ObjectDictionary* odAnalysedCreatedClasses, const KWClass* mappedClass,
					 boolean bIsExternalTable, boolean bCreatedObjects, //DDD
					 const ALString& sOriginClassName, StringVector* svAttributeNames,
					 ObjectArray* oaCreatedDataPaths);

	// Dictionnaire principal
	const KWClass* kwcMainClass;

	// Data path principal
	KWObjectDataPath* mainDataPath;

	// Tableau de tous les data paths
	ObjectArray oaDataPaths;

	// Tableau des data paths Root des tables externes
	ObjectArray oaExternalRootDataPaths;

	// Dictionnaire des data paths
	ObjectDictionary odDataPaths;
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

inline boolean KWObjectDataPath::GetCreatedObjects() const
{
	return bCreatedObjects;
}

inline void KWObjectDataPath::SetCreatedObject(boolean bValue)
{
	bCreatedObjects = bValue;
}

inline const KWObjectDataPath* KWObjectDataPath::GetSubDataPath(const KWLoadIndex liAttributeLoadIndex) const
{
	const KWObjectDataPath* foundSubDataPath;

	require(IsCompiled());
	require(liAttributeLoadIndex.IsValid());
	require(liAttributeLoadIndex.IsDense());
	require(liAttributeLoadIndex.GetDenseIndex() < oaCompiledSubDataPathsByLoadIndex.GetSize());

	foundSubDataPath = cast(const KWObjectDataPath*,
				oaCompiledSubDataPathsByLoadIndex.GetAt(liAttributeLoadIndex.GetDenseIndex()));
	ensure(foundSubDataPath->GetTerminalLoadIndex().GetDenseIndex() == liAttributeLoadIndex.GetDenseIndex());
	return foundSubDataPath;
}

inline longint KWObjectDataPath::GetMainCreationIndex() const
{
	require(IsCompiled());
	return lMainCreationIndex;
}

inline longint KWObjectDataPath::NewCreationIndex() const
{
	require(IsCompiled());
	lCreationNumber++;
	return lCreationNumber;
}

inline longint KWObjectDataPath::GetCreationNumber() const
{
	require(IsCompiled());
	return lCreationNumber;
}

inline const KWLoadIndex KWObjectDataPath::GetTerminalLoadIndex() const
{
	require(IsCompiled());
	return liCompiledTerminalAttributeLoadIndex;
}

inline int KWObjectDataPath::GetRandomSeed() const
{
	require(IsCompiled());
	return nCompiledRandomSeed;
}

inline int KWObjectDataPath::GetRandomLeap() const
{
	require(IsCompiled());
	return nCompiledRandomLeap;
}
