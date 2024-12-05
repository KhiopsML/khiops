// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWDataTableDriver.h"
#include "KWObjectKey.h"
#include "KWLoadIndex.h"

////////////////////////////////////////////////////////////
// Classe KWMTDatabaseMapping
//    Multi-table mapping
class KWMTDatabaseMapping : public Object
{
public:
	// Constructeur
	KWMTDatabaseMapping();
	~KWMTDatabaseMapping();

	// Copie et duplication (sans les attributs de gestion)
	void CopyFrom(const KWMTDatabaseMapping* aSource);
	KWMTDatabaseMapping* Clone() const;

	// Comparaison des attributs de definition
	int Compare(const KWMTDatabaseMapping* aSource) const;

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

	// Table externe
	boolean GetExternalTable() const;
	void SetExternalTable(boolean bValue);

	// Dictionnaire origine du data path
	// - dictionnaire Root si on est dans le cas d'une table externe
	// - dictionnaire principal sinon
	const ALString& GetOriginClassName() const;
	void SetOriginClassName(const ALString& sValue);

	// Nom des attribut du data paths
	StringVector* GetAttributeNames();

	// Dictionaire decrivant la table specifiee
	const ALString& GetClassName() const;
	void SetClassName(const ALString& sValue);

	// Nom du fichier de donnees pour la table
	const ALString& GetDataTableName() const;
	void SetDataTableName(const ALString& sValue);

	////////////////////////////////////////////////////////////
	// Decomposition du DataPath
	//
	// Dans un schema multi-table, chaque data path fait reference a une variable de type Table ou Entity
	//  et identifie un fichier de table de donnees.
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
	// ToDataPathFormat pour le mettre entre '`', comme pour le format externes des nom de variables
	// dans les dictionnaires

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
	friend class KWMTDatabase;
	friend class PLMTDatabaseTextFile;
	friend class KWMTDatabaseStream;
	friend class PLShared_MTDatabaseTextFile;

	// Attributs de la classe
	boolean bExternalTable;
	ALString sOriginClassName;
	StringVector svAttributeNames;
	ALString sClassName;
	ALString sDataTableName;

	///////////////////////////////////////////////////////////////////////
	// Parametrage technique permettant la lecture des enregistrements
	// A parametrer et exploiter dans la classe utilisante

	// Tableau des mappings de la composition en sous-objects ou tableaux de sous-objets (pas les references)
	// Memoire: le tableau appartient a l'appele, son contenu est a gerer par l'appelant
	ObjectArray* GetComponentTableMappings();

	// Collecte de tous les mapping de la hierarchie de composition,
	// y compris le mapping courant (racine de la hierarchie)
	void CollectFullHierarchyComponentTableMappings(ObjectArray* oaResults);

	// Base de donnees associee au mapping
	// Appartient a l'appelant
	void SetDataTableDriver(KWDataTableDriver* dataTableDriver);
	KWDataTableDriver* GetDataTableDriver();

	// Index de l'attribut mappe dans sa classe utilisante
	void ResetMappedAttributeLoadIndex();
	void SetMappedAttributeLoadIndex(KWLoadIndex liLoadIndex);
	KWLoadIndex GetMappedAttributeLoadIndex() const;

	// Type (Object ou ObjectArray) de l'attribut mappe dans sa classe utilisante
	void SetMappedAttributeType(int nType);
	int GetMappedAttributeType() const;

	// Dernier objet lu (potentiellement objet a ratacher au prochain objet utilisant)
	// Appartient a l'appelant
	void SetLastReadObject(KWObject* kwoObject);
	KWObject* GetLastReadObject();

	// Cle du dernier objet lu (permet de verifier l'ordre des instances dans le fichier)
	// Memoire: le parametre du Set est recopie, le retour du Get appartient a l'appele
	void SetLastReadKey(const KWObjectKey* objectKey);
	const KWObjectKey* GetLastReadKey() const;

	// Nettoyage (mise a vide) de la derniere cle lue
	void CleanLastReadKey();

	// Parametres de lecture des enregistrements
	ObjectArray oaComponentTableMappings;
	KWDataTableDriver* mappedDataTableDriver;
	KWLoadIndex liMappedAttributeLoadIndex;
	int nMappedAttributeType;
	KWObject* kwoLastReadObject;
	KWObjectKey lastReadObjectKey;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KWMTDatabaseMapping::GetExternalTable() const
{
	return bExternalTable;
}

inline void KWMTDatabaseMapping::SetExternalTable(boolean bValue)
{
	bExternalTable = bValue;
}

inline const ALString& KWMTDatabaseMapping::GetOriginClassName() const
{
	return sOriginClassName;
}

inline void KWMTDatabaseMapping::SetOriginClassName(const ALString& sValue)
{
	sOriginClassName = sValue;
}

inline StringVector* KWMTDatabaseMapping::GetAttributeNames()
{
	return &svAttributeNames;
}

inline int KWMTDatabaseMapping::GetDataPathAttributeNumber() const
{
	return svAttributeNames.GetSize();
}

inline const ALString& KWMTDatabaseMapping::GetDataPathAttributeNameAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetDataPathAttributeNumber());
	return svAttributeNames.GetAt(nIndex);
}

inline const ALString& KWMTDatabaseMapping::GetClassName() const
{
	return sClassName;
}

inline void KWMTDatabaseMapping::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

inline const ALString& KWMTDatabaseMapping::GetDataTableName() const
{
	return sDataTableName;
}

inline void KWMTDatabaseMapping::SetDataTableName(const ALString& sValue)
{
	sDataTableName = sValue;
}

inline char KWMTDatabaseMapping::GetDataPathSeparator()
{
	return '/';
}

inline char KWMTDatabaseMapping::GetDataPathEscapeChar()
{
	return '`';
}

inline ObjectArray* KWMTDatabaseMapping::GetComponentTableMappings()
{
	return &oaComponentTableMappings;
}

inline void KWMTDatabaseMapping::SetDataTableDriver(KWDataTableDriver* dataTableDriver)
{
	mappedDataTableDriver = dataTableDriver;
}

inline KWDataTableDriver* KWMTDatabaseMapping::GetDataTableDriver()
{
	return mappedDataTableDriver;
}

inline void KWMTDatabaseMapping::ResetMappedAttributeLoadIndex()
{
	liMappedAttributeLoadIndex.Reset();
}

inline void KWMTDatabaseMapping::SetMappedAttributeLoadIndex(KWLoadIndex liLoadIndex)
{
	liMappedAttributeLoadIndex = liLoadIndex;
}

inline KWLoadIndex KWMTDatabaseMapping::GetMappedAttributeLoadIndex() const
{
	return liMappedAttributeLoadIndex;
}

inline void KWMTDatabaseMapping::SetMappedAttributeType(int nType)
{
	nMappedAttributeType = nType;
}

inline int KWMTDatabaseMapping::GetMappedAttributeType() const
{
	return nMappedAttributeType;
}

inline void KWMTDatabaseMapping::SetLastReadObject(KWObject* kwoObject)
{
	kwoLastReadObject = kwoObject;
}

inline KWObject* KWMTDatabaseMapping::GetLastReadObject()
{
	return kwoLastReadObject;
}

inline void KWMTDatabaseMapping::SetLastReadKey(const KWObjectKey* objectKey)
{
	require(objectKey != NULL);
	lastReadObjectKey.CopyFrom(objectKey);
}

inline const KWObjectKey* KWMTDatabaseMapping::GetLastReadKey() const
{
	return &lastReadObjectKey;
}

inline void KWMTDatabaseMapping::CleanLastReadKey()
{
	lastReadObjectKey.SetSize(0);
}
