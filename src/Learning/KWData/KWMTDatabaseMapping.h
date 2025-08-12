// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWMTDatabaseMapping;
class KWMTDatabaseMappingManager;

#include "Object.h"
#include "KWDataPath.h"
#include "KWDataTableDriver.h"
#include "KWObjectKey.h"
#include "KWLoadIndex.h"

////////////////////////////////////////////////////////////
// Classe KWMTDatabaseMapping
// Specialisation de KWDataPath pour la partie logique d'un schema-multi-table,
// en permettant de specifier un fichier de donnees par data path
// Service de gestion de la lecture des objet d'un schema-multi-tables
class KWMTDatabaseMapping : public KWDataPath
{
public:
	// Constructeur
	KWMTDatabaseMapping();
	~KWMTDatabaseMapping();

	// Nom du fichier de donnees pour la table
	const ALString& GetDataTableName() const;
	void SetDataTableName(const ALString& sValue);

	////////////////////////////////////////////////////////
	// Divers

	// Copie (sans les attributs de gestion)
	void CopyFrom(const KWDataPath* aSource) override;

	// Creation
	KWDataPath* Create() const override;

	// Comparaison des attributs de definition
	int Compare(const KWDataPath* aSource) const override;

	// Ecriture
	void Write(ostream& ost) const override;
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// Memoire utilisee par le mapping
	longint GetUsedMemory() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KWMTDatabase;
	friend class PLMTDatabaseTextFile;
	friend class KWMTDatabaseStream;
	friend class PLShared_MTDatabaseTextFile;

	// Nom de la table de donnees
	ALString sDataTableName;

	///////////////////////////////////////////////////////////////////////
	// Parametrage technique permettant la lecture des enregistrements
	// A parametrer et exploiter dans la classe utilisante

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

	// Dernier objet lu (potentiellement objet a rattacher au prochain objet utilisant)
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
	KWDataTableDriver* mappedDataTableDriver;
	KWLoadIndex liMappedAttributeLoadIndex;
	int nMappedAttributeType;
	KWObject* kwoLastReadObject;
	KWObjectKey lastReadObjectKey;
};

////////////////////////////////////////////////////////////
// Classe KWMTDatabaseMappingManager
// Specialisation dans le cas de KWMTDatabaseMapping
class KWMTDatabaseMappingManager : public KWDataPathManager
{
public:
	// Constructeur
	KWMTDatabaseMappingManager();
	~KWMTDatabaseMappingManager();

	// Initialisation du mapping principal
	// Memoire: le mapping en parametre appartient a l'appele apres l'execution de la methode
	void InitializeMainMapping(KWMTDatabaseMapping* mapping);
	boolean IsMainMappingInitialized() const;

	////////////////////////////////////////////////////////////////////////
	// Redefinition des methodes ancetre pour avoir la terminologie mapping

	// Acces aux nombres de mapping
	int GetMappingNumber() const;
	int GetExternalRootMappingNumber() const;

	// Acces aux mapping
	KWMTDatabaseMapping* GetMappingAt(int nIndex) const;
	KWMTDatabaseMapping* GetExternalRootMappingAt(int nIndex) const;
	KWMTDatabaseMapping* GetMainMapping() const;
	KWMTDatabaseMapping* LookupMapping(const ALString& sDataPath) const;

	// Acces direct au tableau de l'ensemble des mappings
	ObjectArray* GetMappings();

	////////////////////////////////////////////////////////
	// Divers

	// Creation pour renvoyer une instance du meme type dynamique
	KWDataPathManager* Create() const override;

	// Verification de la validite des specifications des tables par data path
	boolean CheckPartially(boolean bWriteOnly) const;

	// Libelles utilisateur redefinis pour etre ceux de la base de donnee
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWMTDatabaseMapping::GetDataTableName() const
{
	return sDataTableName;
}

inline void KWMTDatabaseMapping::SetDataTableName(const ALString& sValue)
{
	sDataTableName = sValue;
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

inline void KWMTDatabaseMappingManager::InitializeMainMapping(KWMTDatabaseMapping* mapping)
{
	require(mainDataPath == NULL);
	require(GetMappingNumber() == 0);
	require(mapping != NULL);

	mainDataPath = mapping;
	oaDataPaths.Add(mainDataPath);
	odDataPaths.SetAt(mapping->GetDataPath(), mapping);
}

inline boolean KWMTDatabaseMappingManager::IsMainMappingInitialized() const
{
	return mainDataPath != NULL and oaDataPaths.GetSize() > 0 and oaDataPaths.GetAt(0) == mainDataPath;
}

inline int KWMTDatabaseMappingManager::GetMappingNumber() const
{
	return oaDataPaths.GetSize();
}

inline int KWMTDatabaseMappingManager::GetExternalRootMappingNumber() const
{
	return oaExternalRootDataPaths.GetSize();
}

inline KWMTDatabaseMapping* KWMTDatabaseMappingManager::GetMappingAt(int nIndex) const
{
	return cast(KWMTDatabaseMapping*, oaDataPaths.GetAt(nIndex));
}

inline KWMTDatabaseMapping* KWMTDatabaseMappingManager::GetMainMapping() const
{
	return cast(KWMTDatabaseMapping*, mainDataPath);
}

inline KWMTDatabaseMapping* KWMTDatabaseMappingManager::GetExternalRootMappingAt(int nIndex) const
{
	return cast(KWMTDatabaseMapping*, oaExternalRootDataPaths.GetAt(nIndex));
}

inline KWMTDatabaseMapping* KWMTDatabaseMappingManager::LookupMapping(const ALString& sDataPath) const
{
	return cast(KWMTDatabaseMapping*, odDataPaths.Lookup(sDataPath));
}

inline ObjectArray* KWMTDatabaseMappingManager::GetMappings()
{
	return &oaDataPaths;
}
