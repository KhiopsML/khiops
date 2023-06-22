// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Ermgt.h"
#include "Section.h"

////////////////////////////////
// Classe SectionTable
// Represente un fichier decoupe en sections
// Fichier =
//   Section1
//   Section2
//   Section3
//   Section...
// Chaque section est soit identifiee, c'est a dire entouree
// de 2 lignes speciales:
//     //## <section identifier>
//     ...
//     //##
// soit non identifiee, ce qui est (eventuellement) le cas
// entre 2 sections identifiees.
// La classe SectionTable permet de parser un fichier, et de
// consolider une SectionTable cible a partir d'une SectionTable source,
// en remplacant ses sections identifiees par celles de la source
class SectionTable : public Object
{
public:
	// Constructeur
	SectionTable();
	~SectionTable();

	// Acces
	Section* GetAt(int i) const;
	int GetSize() const;

	//// Chargement et dechargement depuis un fichier

	// Chargement
	int Load(fstream& fst);

	// Dechargement
	void Unload(ostream& ost) const;

	//// Import des sections d'un fichier source
	// Prerequis: les deux fichiers doivent etre en etat valide
	// Les sections identifiees sont remplacees par celles du fichier source
	// Les sections identifiees non remplacees donne lieu a un Warning
	// Les sections identifiees du fichier source non utilisees donnent lieu
	// a un Warning, et sont rajoutees en fin de tableau des sections
	void ImportSectionsFrom(SectionTable* stSource);

	// Ecriture avec le detail des sections
	void Write(ostream& ost) const override;

	// Recherche d'une Section grace aux champs de sa cle:
	//  Identifier
	// On passe en parametre une Section dont ces champs ont ete initialises
	// On retourne soit NULL, soit la Section correspondante de
	// la base de donnees
	Section* LookupSection(Section* aKey) const;

	// Indicateur de validite
	boolean IsValid() const;

	// Nom du fichier (facultatif, pour les messages d'erreur)
	void SetFileName(const ALString& sValue);
	const ALString& GetFileName();

	///// Implementation
protected:
	// Table des Sections
	ObjectArray oaTable;

	// Gestion de l'index, base sur les champs de la cle d'une Section
	ObjectArray oaSectionIndexOnKeyFields;
	void ComputeSectionIndex();

	// Gestion des erreurs
	// La politique de gestion des erreurs est d'informer l'utilisateur
	// systematiquement, mais si possible de rattraper ces erreurs
	// Par exemple, un Unload doit toujours rendre le fichier charge avec
	// un Load, meme s'il y a eu des erreurs de parsing
	boolean bIsValid;
	ALString sFileName;
	void CheckDuplicateSections();
	void Message(const ALString& sMessage, int nLineNumber = -1);
	void Warning(const ALString& sMessage, int nLineNumber = -1);
	// La methode suivante positionne bIsValid a false
	void Error(const ALString& sMessage, int nLineNumber = -1);
};

// Comparaison de sections, basee sur les champs de la cle
int SectionCompareKeyFields(const void* first, const void* second);

// Redefinition de l'operateur <<, pour les stream
inline ostream& operator<<(ostream& ost, const SectionTable& table)
{
	table.Write(ost);
	return ost;
}

///// Implementations inline

inline SectionTable::SectionTable()
{
	bIsValid = true;
}

inline int SectionTable::GetSize() const
{
	return oaTable.GetSize();
}

inline Section* SectionTable::GetAt(int i) const
{
	require(0 <= i and i < GetSize());
	return (Section*)oaTable.GetAt(i);
}

inline boolean SectionTable::IsValid() const
{
	return bIsValid;
}

inline void SectionTable::SetFileName(const ALString& sValue)
{
	sFileName = sValue;
}

inline const ALString& SectionTable::GetFileName()
{
	return sFileName;
}
