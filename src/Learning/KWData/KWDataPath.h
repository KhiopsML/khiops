// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

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

	// Dictionnaire origine du data path
	// - dictionnaire Root si on est dans le cas d'une table externe
	// - dictionnaire principal sinon
	const ALString& GetOriginClassName() const;
	void SetOriginClassName(const ALString& sValue);

	// Nom des attribut du data paths
	StringVector* GetAttributeNames();

	// Dictionaire decrivant les objets a l'extremite du data path
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
