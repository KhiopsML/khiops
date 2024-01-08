// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Object.h"
#include "Vector.h"

//////////////////////////////////
// Classe Section
// Decrit une portion de fichier
// Une section est soit identifiee, c'est a dire entouree
// de 2 lignes speciales (incluses dans la section):
//     //## <section identifier>
//     ...
//     //##
// soit non identifiee, ce qui est (eventuellement) le cas
// entre 2 sections identifiees dans un fichier a sections
class Section : public Object
{
public:
	// Constructeur
	Section();

	// Clone et comparaison
	Section* Clone();
	int Compare(Section* sectionValue);

	//// Acces aux attributs

	// Identifiant de section
	const ALString& GetIdentifier() const;
	void SetIdentifier(const ALString& sValue);

	// Lignes de texte
	const ALString& GetLines() const;
	void SetLines(const ALString& sValue);
	void AddLine(const ALString& sValue);

	//// Chargement et dechargement depuis un fichier

	// Dechargement des lignes uniquement
	// (le chargement d'un fichier a section est fait par la classe SectionTable)
	void Unload(ostream& ost) const;

	// Ecriture
	void Write(ostream& ost) const override;

	///// Implementation
protected:
	// Attributs de la classe
	ALString sIdentifier;
	ALString sLines;
	IntVector ivLineOffsets;
};

// Redefinition de l'operateur <<, pour les stream
inline ostream& operator<<(ostream& ost, const Section& value)
{
	value.Write(ost);
	return ost;
}

///// Implementations inline

inline Section::Section() {}

inline const ALString& Section::GetIdentifier() const
{
	return sIdentifier;
}

inline void Section::SetIdentifier(const ALString& sValue)
{
	sIdentifier = sValue;
}

inline const ALString& Section::GetLines() const
{
	return sLines;
}

inline void Section::SetLines(const ALString& sValue)
{
	sLines = sValue;
}
