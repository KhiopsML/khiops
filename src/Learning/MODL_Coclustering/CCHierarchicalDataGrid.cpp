// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCHierarchicalDataGrid.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe CCHierarchicalDataGrid

CCHierarchicalDataGrid::CCHierarchicalDataGrid()
{
	dNullCost = 0;
	dCost = 0;
	nInitialAttributeNumber = 0;
}

CCHierarchicalDataGrid::~CCHierarchicalDataGrid() {}

void CCHierarchicalDataGrid::SetShortDescription(const ALString& sValue)
{
	sShortDescription = sValue;
}

const ALString& CCHierarchicalDataGrid::GetShortDescription() const
{
	return sShortDescription;
}

void CCHierarchicalDataGrid::SetNullCost(double dValue)
{
	require(dValue >= 0);
	dNullCost = dValue;
}

double CCHierarchicalDataGrid::GetNullCost() const
{
	return dNullCost;
}

void CCHierarchicalDataGrid::SetCost(double dValue)
{
	require(dValue >= 0);
	dCost = dValue;
}

double CCHierarchicalDataGrid::GetCost() const
{
	return dCost;
}

double CCHierarchicalDataGrid::GetLevel() const
{
	double dLevel;

	dLevel = 0;
	if (dNullCost != 0)
		dLevel = 1 - dCost / dNullCost;
	if (dLevel > 1)
		dLevel = 1;
	if (dLevel < 0)
		dLevel = 0;
	return dLevel;
}

void CCHierarchicalDataGrid::SetInitialAttributeNumber(int nValue)
{
	require(nValue >= 0);
	nInitialAttributeNumber = nValue;
}

int CCHierarchicalDataGrid::GetInitialAttributeNumber() const
{
	return nInitialAttributeNumber;
}

void CCHierarchicalDataGrid::SetFrequencyAttributeName(const ALString& sValue)
{
	sFrequencyAttributeName = sValue;
}

const ALString& CCHierarchicalDataGrid::GetFrequencyAttributeName() const
{
	return sFrequencyAttributeName;
}

KWDatabase* CCHierarchicalDataGrid::GetDatabaseSpec()
{
	return &databaseSpec;
}

const KWDatabase* CCHierarchicalDataGrid::GetConstDatabaseSpec() const
{
	return &databaseSpec;
}

void CCHierarchicalDataGrid::DeleteAll()
{
	const KWDatabase nullDatabase;

	KWDataGrid::DeleteAll();
	sShortDescription = "";
	dNullCost = 0;
	dCost = 0;
	nInitialAttributeNumber = 0;
	sFrequencyAttributeName = "";
	databaseSpec.CopyFrom(&nullDatabase);
}

boolean CCHierarchicalDataGrid::CheckHierarchy() const
{
	boolean bOk = true;
	int nAttribute;
	CCHDGAttribute* attribute;
	ALString sTmp;

	// Methode d'integrite de base
	require(KWDataGrid::Check());

	// Cout null
	if (bOk and dNullCost < 0)
	{
		AddError(sTmp + "Invalid null cost (" + DoubleToString(dNullCost) + ")");
		bOk = false;
	}

	// Cout
	if (bOk and dCost < 0)
	{
		AddError(sTmp + "Invalid cost (" + DoubleToString(dCost) + ")");
		bOk = false;
	}

	// Nombre initial de parties
	if (bOk and (nInitialAttributeNumber < GetAttributeNumber()))
	{
		AddError(sTmp + "Initial dimension number (" + IntToString(nInitialAttributeNumber) +
			 ") should be greater or equal to dimension number (" + IntToString(GetAttributeNumber()) +
			 ")");
		bOk = false;
	}

	// Verification des attributs
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(CCHDGAttribute*, oaAttributes.GetAt(nAttribute));

		// Verification de l'attribut
		bOk = bOk and attribute->CheckHierarchy();
		if (not bOk)
			break;
	}
	return bOk;
}

KWDGAttribute* CCHierarchicalDataGrid::NewAttribute() const
{
	return new CCHDGAttribute;
}

KWDGCell* CCHierarchicalDataGrid::NewCell() const
{
	return new CCHDGCell;
}

//////////////////////////////////////////////////////////////////////////////
// Classe CCHDGAttribute

CCHDGAttribute::CCHDGAttribute()
{
	cMin = 0;
	cMax = 0;
	nInitialPartNumber = 0;
	dInterest = 0;
	rootPart = NULL;
}

CCHDGAttribute::~CCHDGAttribute()
{
	ObjectArray oaParts;
	int nPart;
	CCHDGPart* hdgPart;

	// Export de toutes les parties de la hierarchie
	ExportHierarchyParts(&oaParts);

	// Analyse des parties de la hierarchie
	for (nPart = 0; nPart < oaParts.GetSize(); nPart++)
	{
		hdgPart = cast(CCHDGPart*, oaParts.GetAt(nPart));
		check(hdgPart);

		// Destruction uniquement des parties non feuilles (traitees dans le constructeur ancetre)
		if (not hdgPart->IsLeaf())
			delete hdgPart;
	}
}

void CCHDGAttribute::SetMin(Continuous cValue)
{
	require(GetAttributeType() == KWType::Continuous);
	cMin = cValue;
}

Continuous CCHDGAttribute::GetMin() const
{
	require(GetAttributeType() == KWType::Continuous);
	return cMin;
}

void CCHDGAttribute::SetMax(Continuous cValue)
{
	require(GetAttributeType() == KWType::Continuous);
	cMax = cValue;
}

Continuous CCHDGAttribute::GetMax() const
{
	require(GetAttributeType() == KWType::Continuous);
	return cMax;
}

void CCHDGAttribute::SetInitialPartNumber(int nValue)
{
	require(nValue >= 0);
	nInitialPartNumber = nValue;
}

int CCHDGAttribute::GetInitialPartNumber() const
{
	return nInitialPartNumber;
}

void CCHDGAttribute::SetInterest(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dInterest = dValue;
}

double CCHDGAttribute::GetInterest() const
{
	return dInterest;
}

void CCHDGAttribute::SetDescription(const ALString& sValue)
{
	sDescription = sValue;
}

const ALString& CCHDGAttribute::GetDescription() const
{
	return sDescription;
}

CCHDGPart* CCHDGAttribute::NewHierarchyPart()
{
	CCHDGPart* part;

	require(GetAttributeType() != KWType::Unknown);
	require(KWType::IsSimple(GetAttributeType()));

	// Creation d'une nouvelle partie en fonction du type de l'attribut
	part = cast(CCHDGPart*, NewPart());
	part->SetPartType(GetAttributeType());

	// On connecte la partie a l'attribut
	InitializePartOwner(part);

	// On retourne la partie cree
	return part;
}

void CCHDGAttribute::SetRootPart(CCHDGPart* part)
{
	rootPart = part;
}

CCHDGPart* CCHDGAttribute::GetRootPart()
{
	return rootPart;
}

CCHDGPart* CCHDGAttribute::MergePart(CCHDGPart* part)
{
	boolean bDisplay = false;
	CCHDGPart* sourcePart;
	CCHDGPart* targetPart;
	KWDGCell* cell1;
	KWDGCell* cell2;
	KWDGCell* cellToDelete1;
	ObjectArray oaParts;
	int nAttribute;

	require(IsPartMergeable(part));

	// Donnes en entree
	if (bDisplay)
	{
		cout << "Merge\n" << *part << endl;
		cout << "Child1\n" << *part->GetChildPart1() << endl;
		cout << "Child2\n" << *part->GetChildPart2() << endl;
	}

	// On recherche la partie ayant le moins de cellules pour la partie origine
	if (part->GetChildPart1()->GetCellNumber() <= part->GetChildPart2()->GetCellNumber())
	{
		sourcePart = part->GetChildPart1();
		targetPart = part->GetChildPart2();
	}
	else
	{
		sourcePart = part->GetChildPart2();
		targetPart = part->GetChildPart1();
	}

	// On supprime les celulles de la partie 1 vers la partie 2
	oaParts.SetSize(GetDataGrid()->GetAttributeNumber());
	cell1 = sourcePart->GetHeadCell();
	while (cell1 != NULL)
	{
		// Tableau des parties de la cellule
		for (nAttribute = 0; nAttribute < oaParts.GetSize(); nAttribute++)
			oaParts.SetAt(nAttribute, cell1->GetPartAt(nAttribute));

		// Modification avec la partie 2 pour l'attribut en cours
		oaParts.SetAt(GetAttributeIndex(), targetPart);

		// Recherche de la cellule correspondante dans la partie 2
		cell2 = GetDataGrid()->LookupCell(&oaParts);

		// Fusion des effectifs si collision de cellule
		if (cell2 != NULL)
			cell2->SetCellFrequency(cell2->GetCellFrequency() + cell1->GetCellFrequency());
		// Craetion d'une nouvelle cellules sinon
		else
		{
			cell2 = dataGrid->AddCell(&oaParts);
			cell2->SetCellFrequency(cell1->GetCellFrequency());
		}

		// Cellule suivante et supression de la cellule source
		cellToDelete1 = cell1;
		sourcePart->GetNextCell(cell1);
		dataGrid->DeleteCell(cellToDelete1);
	}

	// Import des valeurs de la partie source vers la partie destination
	if (GetAttributeType() == KWType::Continuous)
		targetPart->GetInterval()->Import(sourcePart->GetInterval());
	else
		targetPart->GetValueSet()->Import(sourcePart->GetValueSet());

	// Import des informations de la partie fusionnees
	targetPart->SetPartName(part->GetPartName());
	targetPart->SetInterest(part->GetInterest());
	targetPart->SetHierarchicalLevel(part->GetHierarchicalLevel());
	targetPart->SetRank(part->GetRank());
	targetPart->SetHierarchicalRank(part->GetHierarchicalRank());
	targetPart->SetExpand(part->GetExpand());
	targetPart->SetSelected(part->GetSelected());
	targetPart->SetShortDescription(part->GetShortDescription());
	targetPart->SetDescription(part->GetDescription());

	// Reconstitution du chainage de la hierarchie
	targetPart->SetParentPart(part->GetParentPart());
	if (GetRootPart() == part)
		SetRootPart(targetPart);
	if (part->GetParentPart() != NULL)
	{
		assert(part->GetParentPart()->GetChildPart1() == part or
		       part->GetParentPart()->GetChildPart2() == part);
		if (part->GetParentPart()->GetChildPart1() == part)
			part->GetParentPart()->SetChildPart1(targetPart);
		else
			part->GetParentPart()->SetChildPart2(targetPart);
	}

	// Supression des parties inutiles
	DeletePart(sourcePart);
	delete part;

	// Resultat du merge
	if (bDisplay)
		cout << "Merged part\n" << *targetPart << endl;
	return targetPart;
}

boolean CCHDGAttribute::IsPartMergeable(CCHDGPart* part) const
{
	boolean bOk = true;
	bOk = bOk and part != NULL;
	bOk = bOk and part->GetAttribute() == this;
	bOk = bOk and part->IsParent();
	bOk = bOk and part->GetChildPart1()->IsLeaf();
	bOk = bOk and part->GetChildPart2()->IsLeaf();
	return bOk;
}

void CCHDGAttribute::ExportHierarchyParts(ObjectArray* oaParts) const
{
	int nPart;
	int nFirstPart;
	int nLastPart;
	NumericKeyDictionary nkdExportedParts;
	CCHDGPart* hdgPart;

	require(oaParts != NULL);
	require(oaParts->GetSize() == 0);

	// On initialise avec la racine
	// Le dictionnaire de partie permet d'eviter les boucles infinis s'il y des cycles dans le graphe de parties
	nFirstPart = 0;
	if (rootPart != NULL)
	{
		oaParts->Add(rootPart);
		nkdExportedParts.SetAt((NUMERIC)rootPart, rootPart);
	}

	// On boucle sur l'ajout de parties tant que l'on en trouve de nouvelles
	while (oaParts->GetSize() > nFirstPart)
	{
		// Boucle sur les parties non encore traitees
		nLastPart = oaParts->GetSize();
		for (nPart = nFirstPart; nPart < nLastPart; nPart++)
		{
			hdgPart = cast(CCHDGPart*, oaParts->GetAt(nPart));

			// Ajout si necessaire des parties filles
			if (hdgPart->GetChildPart1() != NULL and
			    nkdExportedParts.Lookup((NUMERIC)hdgPart->GetChildPart1()) == NULL)
			{
				oaParts->Add(hdgPart->GetChildPart1());
				nkdExportedParts.SetAt((NUMERIC)hdgPart->GetChildPart1(), hdgPart->GetChildPart1());
			}
			if (hdgPart->GetChildPart2() != NULL and
			    nkdExportedParts.Lookup((NUMERIC)hdgPart->GetChildPart2()) == NULL)
			{
				oaParts->Add(hdgPart->GetChildPart2());
				nkdExportedParts.SetAt((NUMERIC)hdgPart->GetChildPart2(), hdgPart->GetChildPart2());
			}
		}
		nFirstPart = nLastPart;
	}
}

void CCHDGAttribute::SortPartsByRank()
{
	InternalSortParts(CCHDGPartCompareLeafRank);
}

boolean CCHDGAttribute::CheckHierarchy() const
{
	boolean bOk = true;
	ObjectDictionary odParts;
	ObjectArray oaParts;
	int nPart;
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	ALString sTmp;

	// Methode d'integrite de base
	bOk = KWDGAttribute::Check();

	// Verification des bornes
	if (bOk and GetAttributeType() == KWType::Continuous)
	{
		if (cMin > cMax)
		{
			AddError(sTmp + "Min value (" + KWContinuous::ContinuousToString(cMin) +
				 ") should be less or equal that max value (" + KWContinuous::ContinuousToString(cMax) +
				 ")");
			bOk = false;
		}
	}

	// Nombre initial de parties
	if (bOk and (nInitialPartNumber < GetPartNumber()))
	{
		AddError(sTmp + "Initial part number (" + IntToString(nInitialPartNumber) +
			 ") should be greater or equal to part number (" + IntToString(GetPartNumber()) + ")");
		bOk = false;
	}

	// Interest
	if (bOk and (dInterest < 0 or dInterest > 1))
	{
		AddError(sTmp + "Interest (" + DoubleToString(dInterest) + ") should be between 0 and 1");
		bOk = false;
	}

	// Verification de l'identication des parties par leur nom
	if (bOk)
	{
		// Rangement des parties dans un dictionnaire pour detecter les eventuels doublons de nom
		dgPart = GetHeadPart();
		while (dgPart != NULL)
		{
			hdgPart = cast(CCHDGPart*, dgPart);

			// Rangement dans le dictionnaire et erreur si doublon
			if (odParts.Lookup(hdgPart->GetPartName()) == NULL)
				odParts.SetAt(hdgPart->GetPartName(), hdgPart);
			else
			{
				AddError("Part " + hdgPart->GetPartName() + " already exists");
				bOk = false;
				break;
			}

			// Partie suivante
			GetNextPart(dgPart);
		}
	}

	// Verification de la racine de la hierarchie
	if (bOk and rootPart == NULL)
	{
		AddError("Missing root in part hierarchy");
		bOk = false;
	}

	// Verification de la hierarchie des parties
	if (bOk)
	{
		// Export de toutes les parties de la hierarchie
		ExportHierarchyParts(&oaParts);

		// Analyse des parties de la hierarchie
		for (nPart = 0; nPart < oaParts.GetSize(); nPart++)
		{
			hdgPart = cast(CCHDGPart*, oaParts.GetAt(nPart));
			check(hdgPart);

			// Traitement des parties non feuilles (deja traitees)
			if (not hdgPart->IsLeaf())
			{
				// Test d'integrite
				bOk = bOk and hdgPart->CheckHierarchy();

				// Test de non duplication
				if (bOk and odParts.Lookup(hdgPart->GetPartName()) == NULL)
					odParts.SetAt(hdgPart->GetPartName(), hdgPart);
				else
				{
					AddError("Part " + hdgPart->GetPartName() + " already exists");
					bOk = false;
				}
			}

			// Arret si erreur
			if (not bOk)
				break;
		}

		// Verification du nombre total de parties de la hierarchie
		if (bOk and oaParts.GetSize() != 2 * GetPartNumber() - 1)
		{
			AddError("Number of parts in hierarchy inconsistent with number of leaf parts");
			bOk = false;
		}
	}
	return bOk;
}

KWDGPart* CCHDGAttribute::NewPart() const
{
	return new CCHDGPart;
}

/////////////////////////////////////////////////////////////////////////////
// Classe CCHDGPart

CCHDGPart::CCHDGPart()
{
	dInterest = 0;
	dHierarchicalLevel = 0;
	nRank = 0;
	nHierachicalRank = 0;
	bExpand = false;
	bSelected = false;
	parentPart = NULL;
	childPart1 = NULL;
	childPart2 = NULL;
}

CCHDGPart::~CCHDGPart() {}

void CCHDGPart::SetPartName(const ALString& sValue)
{
	sPartName = sValue;
}

const ALString& CCHDGPart::GetPartName()
{
	return sPartName;
}

void CCHDGPart::SetInterest(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dInterest = dValue;
}

double CCHDGPart::GetInterest() const
{
	return dInterest;
}

void CCHDGPart::SetHierarchicalLevel(double dValue)
{
	require(dValue <= 1);
	dHierarchicalLevel = dValue;
}

double CCHDGPart::GetHierarchicalLevel() const
{
	return dHierarchicalLevel;
}

void CCHDGPart::SetRank(int nValue)
{
	require(nValue >= 0);
	nRank = nValue;
}

int CCHDGPart::GetRank() const
{
	return nRank;
}

void CCHDGPart::SetHierarchicalRank(int nValue)
{
	require(nValue >= 0);
	nHierachicalRank = nValue;
}

int CCHDGPart::GetHierarchicalRank() const
{
	return nHierachicalRank;
}

void CCHDGPart::SetExpand(boolean bValue)
{
	bExpand = bValue;
}

boolean CCHDGPart::GetExpand() const
{
	return bExpand;
}

void CCHDGPart::SetSelected(boolean bValue)
{
	bSelected = bValue;
}

boolean CCHDGPart::GetSelected() const
{
	return bSelected;
}

void CCHDGPart::SetShortDescription(const ALString& sValue)
{
	sShortDescription = sValue;
}

const ALString& CCHDGPart::GetShortDescription() const
{
	return sShortDescription;
}

void CCHDGPart::SetDescription(const ALString& sValue)
{
	sDescription = sValue;
}

const ALString& CCHDGPart::GetDescription() const
{
	return sDescription;
}

const ALString& CCHDGPart::GetUserLabel() const
{
	if (sShortDescription != "")
		return sShortDescription;
	else
		return sPartName;
}

void CCHDGPart::SetParentPart(CCHDGPart* part)
{
	parentPart = part;
}

CCHDGPart* CCHDGPart::GetParentPart()
{
	return parentPart;
}

void CCHDGPart::SetChildPart1(CCHDGPart* part)
{
	childPart1 = part;
}

CCHDGPart* CCHDGPart::GetChildPart1()
{
	return childPart1;
}

void CCHDGPart::SetChildPart2(CCHDGPart* part)
{
	childPart2 = part;
}

CCHDGPart* CCHDGPart::GetChildPart2()
{
	return childPart2;
}

const ALString& CCHDGPart::GetParentPartName()
{
	static const ALString sNullParentName;
	if (parentPart == NULL)
		return sNullParentName;
	else
		return parentPart->GetPartName();
}

boolean CCHDGPart::IsRoot() const
{
	return parentPart == NULL;
}

boolean CCHDGPart::IsParent() const
{
	return childPart1 != NULL and childPart2 != NULL;
}

boolean CCHDGPart::IsLeaf() const
{
	return childPart1 == NULL and childPart2 == NULL;
}

boolean CCHDGPart::CheckHierarchy() const
{
	boolean bOk = true;
	ALString sTmp;

	// Methode d'integrite de base uniquement pour les parties feuilles
	// Les autres parties de la hierarchies n'ont pas de contraintes sur leur composition en cellule
	require(GetAttribute() != NULL);
	require(GetPartType() == GetAttribute()->GetAttributeType());
	require(not IsLeaf() or KWDGPart::Check());

	// Nom de la partie
	if (bOk and sPartName == "")
	{
		AddError("Missing part name");
		bOk = false;
	}

	// Interest
	if (bOk and (dInterest < 0 or dInterest > 1))
	{
		AddError(sTmp + "Interest (" + DoubleToString(dInterest) + ") should be between 0 and 1");
		bOk = false;
	}

	// Niveau hierarchique
	if (bOk and dHierarchicalLevel > 1)
	{
		AddError(sTmp + "Hierarchical level (" + DoubleToString(dHierarchicalLevel) +
			 ") should be less than 1");
		bOk = false;
	}

	// Rang
	if (bOk and nRank <= 0)
	{
		AddError(sTmp + "Rank (" + IntToString(nRank) + ") should be greater than 1");
		bOk = false;
	}

	// Rang hierarchique
	if (bOk and nHierachicalRank < 0)
	{
		AddError(sTmp + "Hierarchical rank (" + IntToString(nRank) + ") should be greater than 0");
		bOk = false;
	}

	// Il ne peut y avoir une seule partie fille
	if (bOk and ((childPart1 == NULL and childPart2 != NULL) or (childPart1 != NULL and childPart2 == NULL) or
		     (childPart1 != NULL and childPart1 == childPart2)))
	{
		AddError(sTmp + "Only one child part is not allowed");
		bOk = false;
	}

	// Dans le cas d'une partie parente, l'effectif doit etre egal a la somme des effectifs des cellules
	if (bOk and IsLeaf() and GetPartFrequency() != ComputeCellsTotalFrequency())
	{
		AddError(sTmp + "Part frequency must be equal to the sum of the frequencies of the cells of the part");
		bOk = false;
	}

	// Dans le cas d'une partie parente, l'effectif doit etre egal a la somme des effectifs des feuilles
	if (bOk and IsParent() and
	    GetPartFrequency() != childPart1->GetPartFrequency() + childPart2->GetPartFrequency())
	{
		AddError(sTmp + "Part frequency must be equal to the sum of the frequencies of the two child parts");
		bOk = false;
	}

	// Dans le cas d'une partie parente, le rangg hierarchique doit etre strictement superieur au rang de chaque
	// feuille
	if (bOk and IsParent() and GetHierarchicalRank() > 0 and
	    (GetHierarchicalRank() >= childPart1->GetHierarchicalRank() or
	     GetHierarchicalRank() >= childPart2->GetHierarchicalRank()))
	{
		AddError(sTmp + "Hierachical rank must be equal less than that of each child part");
		bOk = false;
	}

	// Verification de l'ensemble de valeurs dans le cas categoriel
	if (GetPartType() == KWType::Symbol)
		bOk = bOk and cast(CCHDGValueSet*, valueSet)->CheckHierarchy();

	// Verifications complementaires de chainahe coherent de la hierarchie, sans message d'erreur
	assert(not bOk or parentPart == NULL or parentPart->childPart1 == this or parentPart->childPart2 == this);
	assert(not bOk or childPart1 == NULL or childPart1->parentPart == this);
	assert(not bOk or childPart2 == NULL or childPart2->parentPart == this);
	return bOk;
}

KWDGValueSet* CCHDGPart::NewValueSet() const
{
	return new CCHDGValueSet;
}

boolean CCHDGPart::GetEmulated() const
{
	return true;
}

int CCHDGPartCompareLeafRank(const void* elem1, const void* elem2)
{
	CCHDGPart* part1;
	CCHDGPart* part2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la parties
	part1 = cast(CCHDGPart*, *(Object**)elem1);
	part2 = cast(CCHDGPart*, *(Object**)elem2);

	// Comparaison: on compare en priorite sur le fait d'etre feuille, de facon decroissante
	if (part1->IsLeaf() == part2->IsLeaf())
		nCompare = 0;
	else if (part1->IsLeaf())
		nCompare = -1;
	else
		nCompare = 1;

	// Puis on compare le rang
	if (nCompare == 0)
		nCompare = part1->GetRank() - part2->GetRank();
	return nCompare;
}

int CCHDGPartCompareHierarchicalRank(const void* elem1, const void* elem2)
{
	CCHDGPart* part1;
	CCHDGPart* part2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);
	require(elem1 != elem2);

	// Acces a la parties
	part1 = cast(CCHDGPart*, *(Object**)elem1);
	part2 = cast(CCHDGPart*, *(Object**)elem2);

	// Comparaison du niveau hierarchique
	nCompare = -(part1->GetHierarchicalRank() - part2->GetHierarchicalRank());

	// Dans le cas des anciennes versions n'ayant pas de HierarchicalRank (a zero dans ce cas),
	// comparaison sur le niveau hierarchique pour compatibilite ascendante
	if (nCompare == 0 and part1->GetHierarchicalRank() == 0)
		nCompare = -CompareDouble(part1->GetHierarchicalLevel(), part2->GetHierarchicalLevel());

	// Puis on compare l'index de l'attribut
	if (nCompare == 0)
		nCompare = part1->GetAttribute()->GetAttributeIndex() - part2->GetAttribute()->GetAttributeIndex();

	// Enfin, on compare sur la partie elle meme (comparaison de deux pointeurs)
	if (nCompare == 0)
	{
		if (part1 == part2)
			nCompare = 0;
		else if (part1 > part2)
			nCompare = 1;
		else
			nCompare = -1;
	}
	return nCompare;
}

//////////////////////////////////////////////////////////////////////////////
// Classe CCHDGValueSet

CCHDGValueSet::CCHDGValueSet() {}

CCHDGValueSet::~CCHDGValueSet() {}

void CCHDGValueSet::SortValuesByTypicality()
{
	InternalSortValues(CCHDGValueCompareDecreasingTypicality);
}

boolean CCHDGValueSet::CheckHierarchy() const
{
	boolean bOk = true;
	KWDGValue* value;

	// Pas de controle d'integrite de base (invalide pour els parties non feuilles)
	// Test des valeurs de la partie
	if (bOk)
	{
		// Parcours des valeurs de la partie
		value = GetHeadValue();
		while (value != NULL)
		{
			// Test de la valeur
			bOk = cast(CCHDGValue*, value)->CheckHierarchy();
			if (not bOk)
				break;

			// Valeur suivante
			GetNextValue(value);
		}
	}
	return bOk;
}

KWDGValue* CCHDGValueSet::NewValue(const Symbol& sValue) const
{
	return new CCHDGValue(sValue);
}

//////////////////////////////////////////////////////////////////////////////
// Classe CCHDGValue

CCHDGValue::CCHDGValue(const Symbol& sValue) : KWDGValue(sValue)
{
	dTypicality = 0;
}

CCHDGValue::~CCHDGValue() {}

void CCHDGValue::SetTypicality(double dValue)
{
	require(0 <= dTypicality and dTypicality <= 1);
	dTypicality = dValue;
}

double CCHDGValue::GetTypicality() const
{
	return dTypicality;
}

boolean CCHDGValue::CheckHierarchy() const
{
	boolean bOk = true;
	ALString sTmp;

	// Methode d'integrite de base
	require(KWDGValue::Check());

	// Typicalite
	if (bOk and (dTypicality < 0 or dTypicality > 1))
	{
		AddError(sTmp + "Typicality (" + DoubleToString(dTypicality) + ") should be between 0 and 1");
		bOk = false;
	}
	return bOk;
}

int CCHDGValueCompareDecreasingTypicality(const void* elem1, const void* elem2)
{
	const double dPrecision = 1e-7;
	CCHDGValue* value1;
	CCHDGValue* value2;
	double dCompare;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la valueies
	value1 = cast(CCHDGValue*, *(Object**)elem1);
	value2 = cast(CCHDGValue*, *(Object**)elem2);

	// Comparaison a une precision pres (compatible avec l'affichage)
	dCompare = -value1->GetTypicality() + value2->GetTypicality();
	if (dCompare > dPrecision)
		nCompare = 1;
	else if (dCompare < -dPrecision)
		nCompare = -1;
	else
		nCompare = KWDGValueCompareDecreasingFrequency(elem1, elem2);
	return nCompare;
}

//////////////////////////////////////////////////////////////////////////////
// Classe CCHDGCell

CCHDGCell::CCHDGCell() {}

CCHDGCell::~CCHDGCell() {}