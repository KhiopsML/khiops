// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Section.h"

Section* Section::Clone()
{
	Section* sectionClone;
	int i;

	sectionClone = new Section;
	sectionClone->SetIdentifier(GetIdentifier());
	sectionClone->SetLines(GetLines());

	// Duplication du tableau des offsets
	sectionClone->ivLineOffsets.SetSize(ivLineOffsets.GetSize());
	for (i = 0; i < ivLineOffsets.GetSize(); i++)
	{
		sectionClone->ivLineOffsets.SetAt(i, ivLineOffsets.GetAt(i));
	}

	return sectionClone;
}

int Section::Compare(Section* sectionValue)
{
	int nResult;

	nResult = GetIdentifier().Compare(sectionValue->GetIdentifier());
	if (nResult != 0)
		return nResult;

	// Comparaison de la partie centrale de la section (si possible)
	if (ivLineOffsets.GetSize() < 2 or sectionValue->ivLineOffsets.GetSize() < 2)
		return (GetLines().Compare(sectionValue->GetLines()));
	else
	{
		ALString internalLines;
		ALString sectionValueInternalLines;

		// Calcul de la partie interne des sections
		internalLines = GetLines().Mid(
		    ivLineOffsets.GetAt(1), ivLineOffsets.GetAt(ivLineOffsets.GetSize() - 1) - ivLineOffsets.GetAt(1));
		internalLines.TrimRight();
		internalLines.TrimLeft();
		sectionValueInternalLines = sectionValue->GetLines().Mid(
		    sectionValue->ivLineOffsets.GetAt(1),
		    sectionValue->ivLineOffsets.GetAt(sectionValue->ivLineOffsets.GetSize() - 1) -
			sectionValue->ivLineOffsets.GetAt(1));
		sectionValueInternalLines.TrimRight();
		sectionValueInternalLines.TrimLeft();

		// Retour de la comparaison des partie internes des sections
		return internalLines.Compare(sectionValueInternalLines);
	}
}

void Section::AddLine(const ALString& sValue)
{
	ivLineOffsets.SetSize(ivLineOffsets.GetSize() + 1);
	ivLineOffsets.SetAt(ivLineOffsets.GetSize() - 1, sLines.GetLength());
	sLines += sValue + "\n";
}

void Section::Write(ostream& ost) const
{
	ost << "Identifiant de section : " << GetIdentifier() << "\n";
	ost << "Lignes de texte : "
	    << "\n"
	    << GetLines() << "\n";
}

void Section::Unload(ostream& ost) const
{
	ost << sLines;
}
