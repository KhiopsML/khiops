// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SectionTable.h"

SectionTable::~SectionTable()
{
	// Liberation du contenu de la table
	oaTable.DeleteAll();
}

void SectionTable::Write(ostream& ost) const
{
	ost << "Nombre total de Sections : " << GetSize() << "\n";

	for (int i = 0; i < GetSize(); i++)
		ost << "\n" << *GetAt(i);
	ost << endl;
}

int SectionTable::Load(fstream& fst)
{
	char sBuffer[1000];
	ALString sWork;
	const ALString sSectionSeparator = "//##";

	// Donnees courantes lors du parsing, correctement initialisees
	int nCurrentState;
	int nCurrentTransition;
	ALString sCurrentLine;
	ALString sCurrentIdentifier;
	Section* currentSection = NULL;
	int nLineNumber = 0;

	// Liste des etats lors de la lecture d'un fichier
	const int nStateBegin = 0;         // Debut de fichier, ou fin de section identifiee
	const int nStateSection = 1;       // Section non identifiee
	const int nStateSectionWithId = 2; // Section identifiee
	const int nStateEnd = 3;           // Fin de fichier

	// Liste des transitions
	const int nTransitionNormal = 10;
	const int nTransitionBeginSection = 11;
	const int nTransitionEndSection = 12;
	const int nTransitionEndFile = 13;

	// Le tableau est libere
	oaTable.DeleteAll();
	bIsValid = true; // Reinitialisation de l'indicateur de validite

	// Parsing du fichier
	nCurrentState = nStateBegin;
	while (nCurrentState != nStateEnd)
	{
		// Calcul de la transition, et des donnees courantes
		nCurrentTransition = nTransitionNormal;
		if (not fst.getline(sBuffer, sizeof(sBuffer)))
			nCurrentTransition = nTransitionEndFile;
		else
		{
			// Gestion de la ligne
			sCurrentLine = sBuffer;
			nLineNumber++;

			// Recherche du separateur de  section
			sWork = sCurrentLine;
			sWork.TrimLeft();
			sWork.TrimRight();
			if (sWork == sSectionSeparator)
			{
				nCurrentTransition = nTransitionEndSection;
				sCurrentIdentifier = "";
			}
			else
			{
				if (sWork.Left(sSectionSeparator.GetLength()) == sSectionSeparator)
				{
					nCurrentTransition = nTransitionBeginSection;
					sWork = sWork.Mid(sSectionSeparator.GetLength());
					sWork.TrimLeft();
					sWork.TrimRight();
					sCurrentIdentifier = sWork;
					assert(sCurrentIdentifier.GetLength() > 0);
				}
				else
				{
					nCurrentTransition = nTransitionNormal;
					sCurrentIdentifier = "";
				}
			}
		}

		// Calcul du nouvel etat en fonction de l'ancien, et de la transition
		assert(nCurrentState != nStateEnd);
		assert(nCurrentTransition == nTransitionNormal or nCurrentTransition == nTransitionBeginSection or
		       nCurrentTransition == nTransitionEndSection or nCurrentTransition == nTransitionEndFile);
		if (nCurrentState == nStateBegin)
		{
			if (nCurrentTransition == nTransitionNormal)
			{
				currentSection = new Section;
				oaTable.Add(currentSection);
				currentSection->AddLine(sCurrentLine);
				nCurrentState = nStateSection;
			}
			else if (nCurrentTransition == nTransitionBeginSection)
			{
				currentSection = new Section;
				oaTable.Add(currentSection);
				currentSection->AddLine(sCurrentLine);
				currentSection->SetIdentifier(sCurrentIdentifier);
				nCurrentState = nStateSectionWithId;
			}
			else if (nCurrentTransition == nTransitionEndSection)
			{
				Error("Fin de section dectectee avant un debut de section", nLineNumber);

				// On cree une section sans Id pour rattraper l'erreur
				currentSection = new Section;
				oaTable.Add(currentSection);
				currentSection->AddLine(sCurrentLine);
				nCurrentState = nStateSection;
			}
			else if (nCurrentTransition == nTransitionEndFile)
			{
				nCurrentState = nStateEnd; // Fichier vide
			}
		}

		else if (nCurrentState == nStateSection)
		{
			check(currentSection);
			if (nCurrentTransition == nTransitionNormal)
			{
				currentSection->AddLine(sCurrentLine);
			}
			else if (nCurrentTransition == nTransitionBeginSection)
			{
				currentSection = new Section;
				oaTable.Add(currentSection);
				currentSection->AddLine(sCurrentLine);
				currentSection->SetIdentifier(sCurrentIdentifier);
				nCurrentState = nStateSectionWithId;
			}
			else if (nCurrentTransition == nTransitionEndSection)
			{
				Error("Fin de section dectectee avant un debut de section", nLineNumber);

				// On cree une section sans identifier pour rattraper l'erreur
				currentSection = new Section;
				oaTable.Add(currentSection);
				currentSection->AddLine(sCurrentLine);
				nCurrentState = nStateSection;
			}
			else if (nCurrentTransition == nTransitionEndFile)
			{
				nCurrentState = nStateEnd; // Fin de la section courante
			}
		}

		else if (nCurrentState == nStateSectionWithId)
		{
			check(currentSection);
			if (nCurrentTransition == nTransitionNormal)
			{
				currentSection->AddLine(sCurrentLine);
			}
			else if (nCurrentTransition == nTransitionBeginSection)
			{
				Error("Debut de section dectecte dans une section non terminee", nLineNumber);

				// On cree une section avec identifier pour rattraper l'erreur
				currentSection = new Section;
				oaTable.Add(currentSection);
				currentSection->AddLine(sCurrentLine);
				currentSection->SetIdentifier(sCurrentIdentifier);
				nCurrentState = nStateSectionWithId;
			}
			else if (nCurrentTransition == nTransitionEndSection)
			{
				currentSection->AddLine(sCurrentLine);
				nCurrentState = nStateBegin; // On ne sait pas ce qui suit
			}
			else if (nCurrentTransition == nTransitionEndFile)
			{
				Error("Fin de fichier detectee dans une section non terminee", nLineNumber);

				// Pas de rattrapage d'erreur necessaire
				nCurrentState = nStateEnd; // Fin de la section courante
			}
		}
		assert(nCurrentState == nStateBegin or nCurrentState == nStateSection or
		       nCurrentState == nStateSectionWithId or nCurrentState == nStateEnd);
	}
	assert(nCurrentState == nStateEnd);

	// Calcul de l'index
	ComputeSectionIndex();

	// Verification des sections identifiees en double
	CheckDuplicateSections();

	return 1;
}

void SectionTable::Unload(ostream& ost) const
{
	Section* currentSection;

	// Sortie de chaque sectionligne
	for (int i = 0; i < oaTable.GetSize(); i++)
	{
		currentSection = cast(Section*, oaTable.GetAt(i));
		currentSection->Unload(ost);
	}
}

void SectionTable::ImportSectionsFrom(SectionTable* stSource)
{
	Section* sourceSection;
	Section* targetSection;
	int i;
	ObjectArray oaTargetTable;
	ObjectArray* oaSortedTargetTable;

	require(stSource != NULL);
	require(IsValid());
	require(stSource->IsValid());

	// Dans le cas ou la source est vide: termine
	if (stSource->GetSize() == 0)
		return;

	// Recalcul des index (on n'est jamais trop prudent)
	ComputeSectionIndex();
	stSource->ComputeSectionIndex();

	// Parcours du fichier cible, et remplacement par les sections sources
	for (i = 0; i < GetSize(); i++)
	{
		targetSection = GetAt(i);

		// S'il s'agit d'une section non identifiee, on la garde
		if (targetSection->GetIdentifier() == "")
		{
			oaTargetTable.Add(targetSection);
		}

		// Sinon, on teste son existence dans le tableau source
		else
		{
			sourceSection = stSource->LookupSection(targetSection);

			// Warning si section non trouvee
			if (sourceSection == NULL)
			{
				oaTargetTable.Add(targetSection);
				Warning("Section <" + targetSection->GetIdentifier() +
					"> non trouvee dans le fichier importe");
			}

			// Sinon, on procede au remplacement
			else
			{
				// Message si sections source et cibles sont differentes
				if (targetSection->Compare(sourceSection) != 0)
					Message("Section <" + targetSection->GetIdentifier() + "> remplacee");

				// Remplacement
				delete targetSection;
				targetSection = sourceSection->Clone();
				oaTargetTable.Add(targetSection);
			}
		}
	}

	// Recherche des sections sources non utilises
	// Utilisation d'un tableau trie pour permettre une recherche rapide
	oaSortedTargetTable = oaTargetTable.Clone();
	oaSortedTargetTable->SetCompareFunction(SectionCompareKeyFields);
	oaSortedTargetTable->Sort();
	for (i = 0; i < stSource->GetSize(); i++)
	{
		sourceSection = stSource->GetAt(i);
		if (sourceSection->GetIdentifier() != "")
		{
			targetSection = cast(Section*, oaSortedTargetTable->Lookup(sourceSection));

			// Warning si section non trouvee, et ajout en fin de fichier
			if (targetSection == NULL)
			{
				Warning("Section <" + sourceSection->GetIdentifier() +
					"> du fichier importe, non trouvee " + "dans le fichier cible");
				Message("  -> Ajout en fin de fichier cible");
				targetSection = sourceSection->Clone();
				oaTargetTable.Add(targetSection);
			}
		}
	}
	delete oaSortedTargetTable;

	// Mise a jour de la table de section
	oaTable.RemoveAll();
	for (i = 0; i < oaTargetTable.GetSize(); i++)
	{
		oaTable.Add(oaTargetTable.GetAt(i));
	}
	ComputeSectionIndex();
}

Section* SectionTable::LookupSection(Section* aKey) const
{
	return cast(Section*, oaSectionIndexOnKeyFields.Lookup(aKey));
}

int SectionCompareKeyFields(const void* first, const void* second)
{
	Section* aFirst;
	Section* aSecond;
	int nResult;

	aFirst = cast(Section*, *(Object**)first);
	aSecond = cast(Section*, *(Object**)second);
	nResult = aFirst->GetIdentifier().Compare(aSecond->GetIdentifier());
	if (nResult != 0)
		return nResult;
	return nResult;
}

void SectionTable::ComputeSectionIndex()
{
	if (oaSectionIndexOnKeyFields.GetSize() == 0)
	{
		// Recopie de la table
		oaSectionIndexOnKeyFields.SetSize(oaTable.GetSize());
		for (int i = 0; i < oaTable.GetSize(); i++)
			oaSectionIndexOnKeyFields.SetAt(i, oaTable.GetAt(i));

		// Tri de la table d'index
		oaSectionIndexOnKeyFields.SetCompareFunction(SectionCompareKeyFields);
		oaSectionIndexOnKeyFields.Sort();
	}
}

void SectionTable::CheckDuplicateSections()
// On va parcourir le tableau des section triees selon l'identifiant
// pour detecter les doubles
{
	Section* currentSection;
	ObjectArray* oaSorted = &oaSectionIndexOnKeyFields;
	ALString sCurrentIdentifier;
	ALString sReference = "";
	int nCurrent;
	int nNbElements = 0;

	// Parcours du tableau trie, pour detecter les doubles
	for (nCurrent = 0; nCurrent < oaSorted->GetSize(); nCurrent++)
	{
		currentSection = cast(Section*, oaSorted->GetAt(nCurrent));
		sCurrentIdentifier = currentSection->GetIdentifier();
		assert(sCurrentIdentifier >= sReference); // Le tableau doit etre trie

		// Calcul du nombre de section avec meme identifiant
		if (sCurrentIdentifier != sReference)
		{
			if (nNbElements >= 2 and sReference != "")
				Error("La section <" + sReference + "> apparait " + IntToString(nNbElements) + " fois");
			sReference = sCurrentIdentifier;
			nNbElements = 1;
		}
		else
		{
			nNbElements++;
		}

		// Fin du tableau
		if (nCurrent == oaSorted->GetSize() - 1)
		{
			if (nNbElements >= 2 and sReference != "")
				Error("La section <" + sReference + "> apparait " + IntToString(nNbElements) + " fois");
		}
	}
}

void SectionTable::Message(const ALString& sMessage, int nLineNumber)
{
	ALString sCategory;
	ALString sLineLocalisation;

	// Preparation des elements du message
	if (GetFileName() != "")
		sCategory = "fichier";
	if (nLineNumber >= 0)
	{
		sLineLocalisation = " ligne";
		sLineLocalisation += IntToString(nLineNumber);
	}

	Global::AddMessage(sCategory, GetFileName() + sLineLocalisation, sMessage);
}

void SectionTable::Warning(const ALString& sMessage, int nLineNumber)
{
	ALString sCategory;
	ALString sLineLocalisation;

	// Preparation des elements du message
	if (GetFileName() != "")
		sCategory = "fichier";
	if (nLineNumber >= 0)
	{
		sLineLocalisation = " ligne";
		sLineLocalisation += IntToString(nLineNumber);
	}

	Global::AddWarning(sCategory, GetFileName() + sLineLocalisation, sMessage);
}

void SectionTable::Error(const ALString& sMessage, int nLineNumber)
{
	ALString sCategory;
	ALString sLineLocalisation;

	// Preparation des elements du message
	if (GetFileName() != "")
		sCategory = "fichier";
	if (nLineNumber >= 0)
	{
		sLineLocalisation = " ligne";
		sLineLocalisation += IntToString(nLineNumber);
	}

	Global::AddError(sCategory, GetFileName() + sLineLocalisation, sMessage);

	bIsValid = false;
}