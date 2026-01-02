// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Ermgt.h"
#include "Object.h"
#include "Vector.h"
#include "TableServices.h"

////////////////////////////////////////////////////////////
// Classe ManagedObject
//    Ancetre de tous les objets geres dans un projet,
//    c'est a dire possedant un identifiant, des
//    fonctionnalites de stockage
//    On ne peut l'utiliser que via une sous-classe,
//    comme celles produites par le generateur Norm.
class ManagedObject : public Object
{
public:
	////////////////////////////////////////////////////////
	// Chargement et dechargement depuis un fichier
	//
	// Les methodes suivantes peuvent facilement etre
	// personnalisees par heritage ou par reutilisation.
	// Les methodes de chargement renvoient la chaine de caractere
	// complete en cours d'analyse, ou NULL en cas de probleme
	// (fin de fichiers, ou lignes ou champs vides)
	// Les methodes de dechargement ecrivent les champs, mais pas
	// le retour charriot (pour faciliter la personnalisation).

	// Chargement de tous les attributs stockes
	virtual char* Load(fstream& fst);

	// Dechargement de tous les attributs stockes
	virtual void Unload(ostream& ost) const;

	// Chargement/dechargement d'attributs (stockes ou non)
	// Les champs sont specifies par leur index generique
	// On peut utiliser l'index -1 pour ignorer une position
	// En lecture, seuls les champs stockes sont pris en compte
	virtual char* LoadFields(fstream& fst, IntVector* ivFieldIndexes);
	virtual void UnloadFields(ostream& ost, IntVector* ivFieldIndexes) const;

	////////////////////////////////////////////////////////
	// Fonctionnalites generiques, a redefinir
	// dans les sous-classes

	// Nombre de champs
	virtual int GetFieldNumber() const = 0;

	// Acces generique aux champs, par une valeur de
	// type chaine de caracteres
	// (les champs derives sont ignores en ecriture)
	virtual void SetFieldAt(int nFieldIndex, const char* sValue) = 0;
	virtual const char* GetFieldAt(int nFieldIndex) const = 0;

	// Cle de l'objet
	virtual const ALString& GetKey() const;

	// Nom d'un champ d'apres son index
	virtual const ALString GetFieldNameAt(int nFieldIndex) const = 0;

	// Libelle d'un champ d'apres son index
	virtual const ALString GetFieldLabelAt(int nFieldIndex) const = 0;

	// Nom de stockage d'un champ d'apres son index
	virtual const ALString GetFieldStorageNameAt(int nFieldIndex) const = 0;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class ManagedObjectTable;

	// Constructeur
	ManagedObject();
	~ManagedObject();

	// Duplication (generique) d'un objet
	virtual ManagedObject* CloneManagedObject() const = 0;

	// Indique si un champs est stocke
	virtual boolean GetFieldStored(int nFieldIndex) const = 0;

	// Renvoie les index des champs stockes
	// Memoire: le tableau appartient a l'appele
	virtual IntVector* GetStoredFieldIndexes() const = 0;

	// Fonction de comparaison des champs de la cle
	virtual CompareFunction GetCompareKeyFunction() const = 0;

	// Separateur de champs
	virtual char GetSeparator() const = 0;
};

// Getter des champs de la cle
const ALString ManagedObjectGetKey(const Object* object);
