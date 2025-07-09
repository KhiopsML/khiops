// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

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

	// Copie (sans les attributs de gestion)
	void CopyFrom(const KWDataPath* aSource) override;

	// Creation
	KWDataPath* Create() const override;

	// Comparaison des attributs de definition
	int Compare(const KWDataPath* aSource) const override;

	// Nom du fichier de donnees pour la table
	const ALString& GetDataTableName() const;
	void SetDataTableName(const ALString& sValue);

	////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

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

	// Tableau des mappings de la composition en sous-objects ou tableaux de sous-objets (pas les references)
	// Memoire: le tableau appartient a l'appele, son contenu est a gerer par l'appelant
	ObjectArray* GetComponentTableMappings();

	// Collecte de tous les mapping de la hierarchie de composition,
	// y compris le mapping courant (mapping principal de la hierarchie)
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
	ObjectArray oaComponentTableMappings;
	KWDataTableDriver* mappedDataTableDriver;
	KWLoadIndex liMappedAttributeLoadIndex;
	int nMappedAttributeType;
	KWObject* kwoLastReadObject;
	KWObjectKey lastReadObjectKey;
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
