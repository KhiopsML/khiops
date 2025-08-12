// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWObjectDataPath;
class KWObjectDataPathManager;

#include "KWDataPath.h"
#include "KWDatabaseMemoryGuard.h"

////////////////////////////////////////////////////////////
// Classe KWObjectDataPath
//
// Specialisation de KWDataPath a destination des KWObject pour identifier de facon unique
// chaque objet, qu'il soient stocke ou cree par une regle de derivation
// Cet identifiant unique des KWObject exploite
// - le data path de l'objet, qu'il soit lu ou cree par une regle
// - le CreationIndex de l'object
//   - s'il s'agit d'un objet lu: numero de ligne
//   - s'il s'agit d'un objet cree par une regle
//     - numero de creation local a son instance principale, plus CreationIndex de l'instance principal
//     - dans le cas des table externe, le numero est local a l'ensemble de toutes les instances
// Cette identication permet a la regle de derivation Random de generer des suites de valeurs aleatoires
// de facon reproductible, et independantes entre elles si la regle Random est utilisee plusieurs fois.
class KWObjectDataPath : public KWDataPath
{
public:
	// Constructeur
	KWObjectDataPath();
	~KWObjectDataPath();

	// Indique que la classe gere les data paths correspondant a des instances crees par des regle de creation d'instance
	boolean IsRuleCreationManaged() const override;

	// Indique si le data path correspond a des objets crees par des regles de creation d'instances
	// (defaut: false, correspondant a des objet stockes dans des fichiers)
	boolean GetCreatedObjects() const override;
	void SetCreatedObjects(boolean bValue) override;

	////////////////////////////////////////////////////////////////////////
	// Service de navigation dans les data paths

	// Acces au data path fils pour un index d'attribut relationnel du dictionnaire extremite
	const KWObjectDataPath* GetComponentDataPath(const KWLoadIndex liAttributeLoadIndex) const;

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

	// Service de protection memoire pour gerer les enregistrements trop volumineux,
	// notamment la creation d'instances en grand nombre
	KWDatabaseMemoryGuard* GetMemoryGuard() const;

	// Copie
	void CopyFrom(const KWDataPath* aSource) override;

	// Creation pour renvoyer une instance du meme type dynamique
	KWDataPath* Create() const override;

	// Comparaison
	int Compare(const KWDataPath* aSource) const override;

	// Ecriture
	void Write(ostream& ost) const override;
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

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

	// Gestionnaire de ObjectDataPath
	const KWObjectDataPathManager* GetObjectDataPathManager() const;

	/////////////////////////////////////////////////////////////////////
	// Attributs de base

	// Indicateur de data path dedie a des objets crees par des regles de creation d'instances
	boolean bCreatedObjects;

	// Index de creation principal, servant de reference aux instances crees dans son contexte
	mutable longint lMainCreationIndex;

	// Nombre d'index de creation generes
	mutable longint lCreationNumber;

	/////////////////////////////////////////////////////////////////////
	// Attributs de gestion de la compilation du data path

	// Index de chargement de la derniere variable du data path, aboutissant a son extremite
	KWLoadIndex liCompiledTerminalAttributeLoadIndex;

	// Tableau des data path indexes par leur index de chargement
	// On utilise la partie dense de l'index de chargement pour avoir un acces direct a un data path
	ObjectArray oaCompiledComponentDataPathsByLoadIndex;

	// Parametres pour le generateur de nombre aleatoire
	int nCompiledRandomSeed;
	int nCompiledRandomLeap;

	// Valeur de hash du data path au moment de compilation, permettant de verifier sa validite
	int nCompileHash;
};

////////////////////////////////////////////////////////////
// Classe KWObjectDataPathManager
// Specialisation dans le cas de KWObjectDataPath
class KWObjectDataPathManager : public KWDataPathManager
{
public:
	// Constructeur
	KWObjectDataPathManager();
	~KWObjectDataPathManager();

	// Calcul de tous les data path a partir du dictionnaire principal
	// d'une base multi-table
	void ComputeAllDataPaths(const KWClass* mainClass) override;

	// Acces aux data paths avec le type specialise
	const KWObjectDataPath* GetObjectDataPathAt(int nIndex) const;
	const KWObjectDataPath* GetExternalRootObjectDataPathAt(int nIndex) const;
	const KWObjectDataPath* GetMainObjectDataPath() const;
	const KWObjectDataPath* LookupObjectDataPath(const ALString& sDataPath) const;

	// Service de protection memoire pour gerer les enregistrements trop volumineux,
	// notamment la creation d'instances en grand nombre
	void SetMemoryGuard(KWDatabaseMemoryGuard* memoryGuard);
	KWDatabaseMemoryGuard* GetMemoryGuard() const;

	////////////////////////////////////////////////////////
	// Divers

	// Copie
	void CopyFrom(const KWDataPathManager* aSource) override;

	// Creation pour renvoyer une instance du meme type dynamique
	KWDataPathManager* Create() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	KWDatabaseMemoryGuard* databaseMemoryGuard;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KWObjectDataPath::GetCreatedObjects() const
{
	return bCreatedObjects;
}

inline void KWObjectDataPath::SetCreatedObjects(boolean bValue)
{
	bCreatedObjects = bValue;
}

inline const KWObjectDataPath* KWObjectDataPath::GetComponentDataPath(const KWLoadIndex liAttributeLoadIndex) const
{
	const KWObjectDataPath* foundComponentDataPath;

	require(IsCompiled());
	require(liAttributeLoadIndex.IsValid());
	require(liAttributeLoadIndex.IsDense());
	require(liAttributeLoadIndex.GetDenseIndex() < oaCompiledComponentDataPathsByLoadIndex.GetSize());

	foundComponentDataPath =
	    cast(const KWObjectDataPath*,
		 oaCompiledComponentDataPathsByLoadIndex.GetAt(liAttributeLoadIndex.GetDenseIndex()));
	ensure(foundComponentDataPath->GetTerminalLoadIndex().GetDenseIndex() == liAttributeLoadIndex.GetDenseIndex());
	return foundComponentDataPath;
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

inline KWDatabaseMemoryGuard* KWObjectDataPath::GetMemoryGuard() const
{
	require(GetObjectDataPathManager() != NULL);
	require(GetObjectDataPathManager()->GetMemoryGuard() != NULL);
	return GetObjectDataPathManager()->GetMemoryGuard();
}

inline const KWObjectDataPathManager* KWObjectDataPath::GetObjectDataPathManager() const
{
	return cast(const KWObjectDataPathManager*, dataPathManager);
}

inline const KWObjectDataPath* KWObjectDataPathManager::GetObjectDataPathAt(int nIndex) const
{
	return cast(const KWObjectDataPath*, GetDataPathAt(nIndex));
}

inline const KWObjectDataPath* KWObjectDataPathManager::GetMainObjectDataPath() const
{
	return cast(const KWObjectDataPath*, GetMainDataPath());
}

inline const KWObjectDataPath* KWObjectDataPathManager::GetExternalRootObjectDataPathAt(int nIndex) const
{
	return cast(const KWObjectDataPath*, GetExternalRootDataPathAt(nIndex));
}

inline const KWObjectDataPath* KWObjectDataPathManager::LookupObjectDataPath(const ALString& sDataPath) const
{
	return cast(const KWObjectDataPath*, LookupDataPath(sDataPath));
}

inline void KWObjectDataPathManager::SetMemoryGuard(KWDatabaseMemoryGuard* memoryGuard)
{
	databaseMemoryGuard = memoryGuard;
}

inline KWDatabaseMemoryGuard* KWObjectDataPathManager::GetMemoryGuard() const
{
	return databaseMemoryGuard;
}
