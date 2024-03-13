// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGrid.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGrid

KWDataGrid::KWDataGrid()
{
	nGridFrequency = 0;
	dLnGridSize = 0;
	nInformativeAttributeNumber = 0;
	nTotalPartNumber = 0;
	headCell = NULL;
	tailCell = NULL;
	nCellNumber = 0;
	slCells = NULL;
	nSortValue = 0;
	targetAttribute = NULL;
	nGranularity = 0;
	varPartAttribute = NULL;
}

KWDataGrid::~KWDataGrid()
{
	DeleteAll();
}

int KWDataGrid::GetGranularity() const
{
	return nGranularity;
}

void KWDataGrid::SetGranularity(int nIndex)
{
	nGranularity = nIndex;
}

void KWDataGrid::Initialize(int nAttributeNumber, int nTargetValueNumber)
{
	int nAttribute;
	KWDGAttribute* attribute;

	require(0 <= nAttributeNumber);
	require(0 <= nTargetValueNumber);

	// Nettoyage initial
	DeleteAll();

	// Creation du tableau d'attribut et de son contenu
	targetAttribute = NULL;
	oaAttributes.SetSize(nAttributeNumber);
	for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
	{
		// Creation de l'attribut dans la liste
		attribute = NewAttribute();
		oaAttributes.SetAt(nAttribute, attribute);

		// Initialisation de l'attribut pour le mettre dans un etat coherent
		attribute->dataGrid = this;
		attribute->nAttributeIndex = nAttribute;
	}

	// Creation du vecteur de valeurs cibles
	svTargetValues.SetSize(nTargetValueNumber);
}

void KWDataGrid::AddAttribute()
{
	KWDGAttribute* attribute;

	require(GetCellNumber() == 0);
	require(not GetCellUpdateMode());

	// Creation de l'attribut dans la liste
	attribute = NewAttribute();
	oaAttributes.Add(attribute);

	// Initialisation de l'attribut pour le mettre dans un etat coherent
	attribute->dataGrid = this;
	attribute->nAttributeIndex = oaAttributes.GetSize() - 1;
}

KWDGAttribute* KWDataGrid::SearchAttribute(const ALString& sAttributeName) const
{
	KWDGAttribute* attribute;
	int nAttributeIndex;

	// Parcours des attributs
	for (nAttributeIndex = 0; nAttributeIndex < GetAttributeNumber(); nAttributeIndex++)
	{
		attribute = GetAttributeAt(nAttributeIndex);
		if (attribute->GetAttributeName() == sAttributeName)
			return attribute;
	}
	return NULL;
}

void KWDataGrid::DeleteNonInformativeAttributes()
{
	int nAttribute;
	KWDGAttribute* attribute;

	require(GetCellNumber() == 0);
	require(GetCellUpdateMode() == false);

	// Parcours des attributs et destruction des attributs non informatifs
	nInformativeAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < oaAttributes.GetSize(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		assert(attribute->GetPartNumber() >= 1);

		// Supression de l'attribut s'il n'est pas informatif
		if (attribute->GetPartNumber() <= 1)
			delete attribute;
		// Sinon, on le range a son nouvel index dans le tableau des attributs
		else
		{
			attribute->nAttributeIndex = nInformativeAttributeNumber;
			oaAttributes.SetAt(attribute->nAttributeIndex, attribute);
			nInformativeAttributeNumber++;
		}
	}
	oaAttributes.SetSize(nInformativeAttributeNumber);
}

void KWDataGrid::DeleteAll()
{
	KWDGCell* cell;
	KWDGCell* cellToDelete;

	// Reinitialisation des statistiques globales
	nGridFrequency = 0;
	dLnGridSize = 0;
	nInformativeAttributeNumber = 0;
	nTotalPartNumber = 0;

	// Destruction de la structure d'indexation
	DeleteIndexingStructure();
	SetCellUpdateMode(false);

	// Destruction des cellules
	cell = headCell;
	while (cell != NULL)
	{
		cellToDelete = cell;
		cell = cell->nextCell;
		delete cellToDelete;
	}
	headCell = NULL;
	tailCell = NULL;
	nCellNumber = 0;

	// Destruction des attributs
	oaAttributes.DeleteAll();
	varPartAttribute = NULL;

	// Destruction des valeurs cibles
	svTargetValues.SetSize(0);
}

void KWDataGrid::DeleteAllCells()
{
	KWDGCell* cell;
	KWDGCell* cellToDelete;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;

	// Reinitialisation de l'effectif global
	nGridFrequency = 0;

	// Destruction de la structure d'indexation
	SetCellUpdateMode(false);

	// Destruction des cellules
	cell = headCell;
	while (cell != NULL)
	{
		cellToDelete = cell;
		cell = cell->nextCell;
		delete cellToDelete;
	}
	headCell = NULL;
	tailCell = NULL;
	nCellNumber = 0;

	// Dereferencement des cellules depuis les parties des attributs
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));

		// Calcul de l'effectif cumule des cellule de la partie
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			// Derefencement des cellules de la partie
			part->headCell = NULL;
			part->tailCell = NULL;
			part->nCellNumber = 0;

			// Remise a 0 de l'effectif de la partie
			part->nPartFrequency = 0;

			// Partie suivante
			attribute->GetNextPart(part);
		}
	}
}

void KWDataGrid::SetCellUpdateMode(boolean bValue)
{
	KWDGCell* cell;

	// Indexation si necessaire
	if (bValue and slCells == NULL)
	{
		// Creation de la liste triee des cellules
		slCells = new SortedList(KWDGCellCompare);

		// Rangement de tous les cellules en cours dans cette liste
		cell = headCell;
		while (cell != NULL)
		{
			slCells->Add(cell);
			cell = cell->nextCell;
		}
	}

	// Destruction de l'indexation si necessaire
	if (not bValue and slCells != NULL)
	{
		delete slCells;
		slCells = NULL;

		// Mise a jour des statistiques globales
		UpdateAllStatistics();
	}

	// Validation
	ensure(bValue == GetCellUpdateMode());
	ensure(slCells == NULL or slCells->GetCount() == nCellNumber);
}

boolean KWDataGrid::GetCellUpdateMode() const
{
	return slCells != NULL;
}

void KWDataGrid::BuildIndexingStructure()
{
	int nAttribute;
	KWDGAttribute* attribute;

	// Construction de la structure d'indexation de chaque attribut
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attribute->BuildIndexingStructure();
	}
}

void KWDataGrid::DeleteIndexingStructure()
{
	int nAttribute;
	KWDGAttribute* attribute;

	// Destruction de la structure d'indexation de chaque attribut
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attribute->DeleteIndexingStructure();
	}
}

KWDGCell* KWDataGrid::AddCell(ObjectArray* oaParts)
{
	KWDGCell* cell;
	int nAttribute;
	KWDGPart* part;

	require(GetCellUpdateMode());
	require(CheckCellParts(oaParts));
	require(LookupCell(oaParts) == NULL);

	// Creation d'une nouvelle cellule
	cell = NewCell();

	// Memorisation des reference aux parties de la cellule
	cell->oaParts.CopyFrom(oaParts);

	// Ajustement de la taille des tableau de gestion des chainages
	cell->oaPrevCells.SetSize(oaParts->GetSize());
	cell->oaNextCells.SetSize(oaParts->GetSize());

	// Ajout en fin de la liste globale des cellules
	nCellNumber++;
	if (headCell == NULL)
		headCell = cell;
	if (tailCell != NULL)
	{
		tailCell->nextCell = cell;
		cell->prevCell = tailCell;
	}
	tailCell = cell;

	// Chainage dans la liste de cellules de chaque partie d'attribut
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		// Recherche de la partie de l'attribut
		part = cast(KWDGPart*, oaParts->GetAt(nAttribute));

		// Ajout en fin de sa liste de cellules
		part->nCellNumber++;
		if (part->headCell == NULL)
			part->headCell = cell;
		if (part->tailCell != NULL)
		{
			part->tailCell->oaNextCells.SetAt(nAttribute, cell);
			cell->oaPrevCells.SetAt(nAttribute, part->tailCell);
		}
		part->tailCell = cell;
	}

	// Ajout dans la structure d'indexation
	slCells->Add(cell);

	// On retourne la cellule cree
	ensure(cell->Check());
	return cell;
}

KWDGCell* KWDataGrid::LookupCell(ObjectArray* oaParts) const
{
	static KWDGCell cellSortKey;
	KWDGCell* cell;
	POSITION position;

	require(GetCellUpdateMode());
	require(CheckCellParts(oaParts));

	// Recopie des criteres de trie dans la cellule cle
	cellSortKey.oaParts.CopyFrom(oaParts);

	// Recherche de la cellule dans la liste triee des cellules
	cell = NULL;
	position = slCells->Find(&cellSortKey);
	if (position != NULL)
		cell = cast(KWDGCell*, slCells->GetAt(position));
	return cell;
}

void KWDataGrid::DeleteCell(KWDGCell* cell)
{
	KWDGCell* prevPartCell;
	KWDGCell* nextPartCell;
	int nAttribute;
	KWDGPart* part;
	POSITION position;

	require(cell != NULL);
	require(GetCellUpdateMode() == false or CheckCell(cell));

	// Supression de la liste globale des cellules
	nCellNumber--;
	if (cell->prevCell != NULL)
		cell->prevCell->nextCell = cell->nextCell;
	if (cell->nextCell != NULL)
		cell->nextCell->prevCell = cell->prevCell;
	if (headCell == cell)
		headCell = cell->nextCell;
	if (tailCell == cell)
		tailCell = cell->prevCell;

	// Suppresion dans la liste de cellules de chaque partie
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		// Recherche de la partie de l'attribut
		part = cell->GetPartAt(nAttribute);

		// Supression de sa liste de cellules
		part->nCellNumber--;
		prevPartCell = cast(KWDGCell*, cell->oaPrevCells.GetAt(nAttribute));
		nextPartCell = cast(KWDGCell*, cell->oaNextCells.GetAt(nAttribute));
		assert(prevPartCell != NULL or part->headCell == cell);
		assert(nextPartCell != NULL or part->tailCell == cell);
		if (prevPartCell != NULL)
			prevPartCell->oaNextCells.SetAt(nAttribute, nextPartCell);
		if (nextPartCell != NULL)
			nextPartCell->oaPrevCells.SetAt(nAttribute, prevPartCell);
		if (part->headCell == cell)
		{
			assert(prevPartCell == NULL);
			part->headCell = nextPartCell;
		}
		if (part->tailCell == cell)
		{
			assert(nextPartCell == NULL);
			part->tailCell = prevPartCell;
		}
	}

	// Suppression de la structure d'indexation
	if (GetCellUpdateMode())
	{
		position = slCells->Find(cell);
		assert(position != NULL);
		assert(cell == cast(KWDGCell*, slCells->GetAt(position)));
		slCells->RemoveAt(position);
	}

	// Destruction de la cellule
	delete cell;
}

boolean KWDataGrid::CheckCell(KWDGCell* cell) const
{
	boolean bOk = true;
	int nAttribute;
	KWDGPart* part;

	require(GetCellUpdateMode());
	require(cell != NULL);
	require(cell->GetAttributeNumber() == GetAttributeNumber());
	require(cell->GetTargetValueNumber() == GetTargetValueNumber());

	// Validite interne de la cellule
	bOk = cell->Check();

	// Validite des parties d'attribut de la cellule
	if (bOk)
	{
		bOk = CheckCellParts(&(cell->oaParts));
		if (not bOk)
			cell->AddError("Oncorrect referenced parts");
	}

	// Existence de la cellule dans la structure de grille
	if (bOk)
	{
		bOk = LookupCell(&(cell->oaParts)) == cell;
		if (not bOk)
			cell->AddError("Not referenced in the data grid");
	}

	// Verification du referencement dans les listes des parties par attribut
	if (bOk)
	{
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			// Test d'existence de la cellule dans la liste de la partie
			part = cell->GetPartAt(nAttribute);
			bOk = bOk and part->CheckCell(cell);
			if (not bOk)
				break;
		}
	}
	return bOk;
}

boolean KWDataGrid::CheckCellParts(ObjectArray* oaParts) const
{
	boolean bOk = true;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;

	require(oaParts != NULL);
	require(oaParts->GetSize() == GetAttributeNumber());

	// Verification des parties referencees
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		// Test de validite de la partie dans son attribut d'appartenance
		part = cast(KWDGPart*, oaParts->GetAt(nAttribute));
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		bOk = bOk and attribute->CheckPart(part);
		if (not bOk)
			break;
	}
	return bOk;
}

void KWDataGrid::UpdateAllStatistics()
{
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;

	// Mise a jour des effectifs par partie pour chaque attribut
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));

		// Calcul de l'effectif cumule des cellule de la partie
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			part->nPartFrequency = part->ComputeCellsTotalFrequency();
			attribute->GetNextPart(part);
		}
	}

	// Mise a jour des statistiques globales de la grille
	nGridFrequency = ComputeGridFrequency();
	dLnGridSize = ComputeLnGridSize();
	nInformativeAttributeNumber = ComputeInformativeAttributeNumber();
	nTotalPartNumber = ComputeTotalPartNumber();
}

int KWDataGrid::ComputeMaxPartNumber() const
{
	int nResult;
	int nAttribute;
	KWDGAttribute* attribute;

	// Calcul par parcours des attributs
	nResult = 0;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		nResult = max(nResult, attribute->GetPartNumber());
	}
	return nResult;
}

double KWDataGrid::ComputeSourceEntropy()
{
	double dResult;
	double dGridFrequency;
	double dProba;
	KWDGCell* cell;

	// Parcours des cellules pour calculer la valeur de l'entropie source
	dResult = 0;
	dGridFrequency = GetGridFrequency();
	if (dGridFrequency != 0)
	{
		cell = GetHeadCell();
		while (cell != NULL)
		{
			dProba = cell->GetCellFrequency() / dGridFrequency;
			if (dProba != 0)
				dResult += dProba * log(dProba);
			GetNextCell(cell);
		}
	}
	dResult /= -log(2.0);
	assert(dResult > -1e-10);
	dResult = fabs(dResult);
	if (dResult < 1e-10)
		dResult = 0;

	return dResult;
}

double KWDataGrid::ComputeTargetEntropy()
{
	double dResult;
	double dGridFrequency;
	double dProba;
	IntVector ivTargetValueFrequencies;
	int nTarget;
	KWDGCell* cell;

	// Parcours de la matrice pour calculer la valeur de l'entropie
	dResult = 0;
	dGridFrequency = GetGridFrequency();
	if (dGridFrequency != 0 and GetTargetValueNumber() > 0)
	{
		// Calcul des effectifs par valeur cible, par cumul sur les cellules
		ivTargetValueFrequencies.SetSize(GetTargetValueNumber());
		cell = GetHeadCell();
		while (cell != NULL)
		{
			for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
				ivTargetValueFrequencies.UpgradeAt(nTarget, cell->GetTargetFrequencyAt(nTarget));
			GetNextCell(cell);
		}

		// Calcul de l'entropie cible
		for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		{
			dProba = ivTargetValueFrequencies.GetAt(nTarget) / dGridFrequency;
			if (dProba != 0)
				dResult += dProba * log(dProba);
		}
	}
	dResult /= -log(2.0);
	assert(dResult > -1e-10);
	dResult = fabs(dResult);
	if (dResult < 1e-10)
		dResult = 0;

	return dResult;
}

double KWDataGrid::ComputeMutualEntropy()
{
	double dResult;
	double dGridFrequency;
	double dProba;
	KWDGCell* cell;
	IntVector ivTargetValueFrequencies;
	int nTarget;

	// Parcours des cellules pour calculer la valeur de l'entropie mutuelle
	dResult = 0;
	dGridFrequency = GetGridFrequency();
	if (dGridFrequency != 0 and GetTargetValueNumber() > 0)
	{
		// Calcul des effectifs par valeur cible, par cumul sur les cellules
		ivTargetValueFrequencies.SetSize(GetTargetValueNumber());
		cell = GetHeadCell();
		while (cell != NULL)
		{
			for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
				ivTargetValueFrequencies.UpgradeAt(nTarget, cell->GetTargetFrequencyAt(nTarget));
			GetNextCell(cell);
		}

		// Calcul de l'entropie mutuelle
		cell = GetHeadCell();
		while (cell != NULL)
		{
			for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
			{
				dProba = cell->GetTargetFrequencyAt(nTarget) / dGridFrequency;
				if (dProba != 0)
					dResult +=
					    dProba *
					    log(dProba / ((cell->GetCellFrequency() / dGridFrequency) *
							  (ivTargetValueFrequencies.GetAt(nTarget) / dGridFrequency)));
			}
			GetNextCell(cell);
		}
	}
	dResult /= log(2.0);
	assert(dResult > -1e-10);
	dResult = fabs(dResult);
	if (dResult < 1e-10)
		dResult = 0;

	return dResult;
}

KWDGAttribute* KWDataGrid::NewAttribute() const
{
	return new KWDGAttribute;
}

boolean KWDataGrid::Check() const
{
	boolean bOk = true;
	ObjectDictionary odAttributes;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGCell* cell;
	int nAttributeCellNumber;
	int nTargetAttributeNumber;
	int nVarTypeAttributeNumber;
	ALString sTmp;

	// Initialisation des nombres d'attributs a verifier
	nTargetAttributeNumber = 0;
	nVarTypeAttributeNumber = 0;

	// Verification de la granularite
	if (nGranularity < 0)
	{
		AddError(sTmp + "Granularity " + IntToString(nGranularity) + "  must be an integer greater than 0");
		bOk = false;
	}
	else if (GetGridFrequency() > 0 and (nGranularity > (int)ceil(log(GetGridFrequency()) / log(2.0))))
	{
		AddError(sTmp + "Granularity " + IntToString(nGranularity) +
			 "  must be an integer smaller than log2(N) " + IntToString(GetGridFrequency()));
		bOk = false;
	}

	// Verification des attributs
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));

		// Verification de l'attribut
		bOk = bOk and attribute->Check();
		if (not bOk)
			break;

		// On compte le nombre d'attributs tagges comme cible
		if (attribute->GetAttributeTargetFunction())
		{
			nTargetAttributeNumber++;
		}

		// On compte le nombre d'attributs de type VarPart
		if (attribute->GetAttributeType() == KWType::VarPart)
		{
			nVarTypeAttributeNumber++;
		}

		// Verification du lien entre l'attribut et la structure
		if (attribute->GetAttributeIndex() != nAttribute)
		{
			attribute->AddError(sTmp + "The variable index does not correspond to " +
					    "its rank in the parent structure (" + IntToString(nAttribute) + ")");
			bOk = false;
		}
		if (attribute->dataGrid != this)
		{
			attribute->AddError("Variable incorrectly linked to its parent structure");
			bOk = false;
		}

		// Rangement de l'attribut dans un dictionnaire, pour verifier son unicite
		if (odAttributes.Lookup(attribute->GetAttributeName()) != NULL)
		{
			attribute->AddError("Another variable already exists with the same name");
			bOk = false;
		}
		else
			odAttributes.SetAt(attribute->GetAttributeName(), attribute);

		// Calcul du nombre total de cellules references par l'attribut
		nAttributeCellNumber = 0;
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			nAttributeCellNumber += part->GetCellNumber();
			attribute->GetNextPart(part);
		}

		// Verification de la coherence avec le nombre total de cellules
		if (nAttributeCellNumber != nCellNumber)
		{
			attribute->AddError(
			    sTmp + "The number of cells in the variable (" + IntToString(nAttributeCellNumber) +
			    ") is different with that of the data grid (" + IntToString(nCellNumber) + ")");
			bOk = false;
			break;
		}
	}

	// Verification qu'il n'y a pas plus d'un attribut cible
	if (bOk and nTargetAttributeNumber > 1)
	{
		AddError(sTmp + "There are " + IntToString(nTargetAttributeNumber) +
			 " target variables in the data grid");
		bOk = false;
	}

	// Verification que l'eventuel attribut cible est bien le dernier
	if (bOk and nTargetAttributeNumber == 1)
	{
		check(GetTargetAttribute());
		assert(GetAttributeAt(GetTargetAttribute()->GetAttributeIndex()) == GetTargetAttribute());
		if (GetTargetAttribute()->GetAttributeIndex() != GetAttributeNumber() - 1)
		{
			AddError(sTmp + "The target variable should be the last variable in the data grid");
			bOk = false;
		}
	}

	// Verification de l'absence d'attribut cible si necessaire
	if (bOk and nTargetAttributeNumber == 0)
	{
		if (GetTargetAttribute() != NULL)
		{
			AddError(sTmp + "A target variable is wrongly referenced");
			bOk = false;
		}
	}

	// Verification qu'il n'y a pas de valeurs cible en meme temps qu'un attribut cible
	if (bOk and nTargetAttributeNumber == 1 and GetTargetValueNumber() > 0)
	{
		AddError(sTmp + "There are both " + IntToString(GetTargetValueNumber()) +
			 " target values and one target variable (" + GetTargetAttribute()->GetAttributeName() +
			 ") in the data grid");
		bOk = false;
	}

	// Verifications par assertion de la memorisation de l'eventuel attribut de type VarPart
	assert(not bOk or varPartAttribute == NULL or varPartAttribute->GetAttributeType() == KWType::VarPart);
	assert(not bOk or varPartAttribute == NULL or
	       SearchAttribute(varPartAttribute->GetAttributeName()) == varPartAttribute);
	assert(not bOk or varPartAttribute == NULL or varPartAttribute->GetInnerAttributes() != NULL);
	assert(not bOk or varPartAttribute == NULL or nVarTypeAttributeNumber > 0);
	assert(not bOk or varPartAttribute != NULL or nVarTypeAttributeNumber == 0);

	// Verification qu'il y au plus un attribut de type VarPart
	if (bOk and nVarTypeAttributeNumber > 1)
	{
		AddError(sTmp + "There are " + IntToString(nVarTypeAttributeNumber) +
			 " variables of type VarPart in the data grid");
		bOk = false;
	}

	// Verification dans le cas d'un attribut de type VarPart
	if (bOk and IsVarPartDataGrid())
	{
		assert(nVarTypeAttributeNumber == 1);

		// Verification des attributs internes
		bOk = GetInnerAttributes()->Check();

		// Verification que les attributs de la grille sont distincts des attributs internes
		if (bOk)
		{
			for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
			{
				attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));

				// Test d'existence parmi les attribut internes
				if (GetVarPartAttribute()->GetInnerAttributes()->LookupInnerAttribute(
					attribute->GetAttributeName()) != NULL)
				{
					AddError(sTmp + "Internal variable " + attribute->GetAttributeName() +
						 " is already used with the same name among the data grid variables "
						 "(index " +
						 IntToString(attribute->GetAttributeIndex()) + ")");
					break;
					bOk = false;
				}
			}
		}

		// Verification de l'effectif total de la grille
		// On ne fait la verification que si des cellules sont presentes, ce qui permet de verifier une grille

		if (bOk and GetCellNumber() > 0 and
		    GetInnerAttributes()->ComputeTotalInnerAttributeFrequency() != ComputeGridFrequency())
		{
			AddError(sTmp + "Grid frequency (" + IntToString(ComputeGridFrequency()) +
				 ") is different from the total part frequency of the inner variables (" +
				 IntToString(GetInnerAttributes()->ComputeTotalInnerAttributeFrequency()) + ")");
		}
	}

	// Verification de toutes les cellules
	if (bOk)
	{
		cell = headCell;
		while (cell != NULL)
		{
			// Validite des parties d'attribut de la cellule
			// (l'existence de la cellule est forcement verifiee)
			if (bOk)
			{
				bOk = CheckCellParts(&(cell->oaParts));
				if (not bOk)
					cell->AddError("Incorrect referenced parts");
			}

			// Verification du referencement dans les listes des parties par attribut
			if (bOk)
			{
				for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
				{
					// Test d'existence de la cellule dans la liste de la partie
					part = cell->GetPartAt(nAttribute);
					bOk = bOk and part->CheckCell(cell);
					if (not bOk)
						break;
				}
			}
			if (not bOk)
				break;
			cell = cell->nextCell;
		}
	}

	return bOk;
}

longint KWDataGrid::GetUsedMemory() const
{
	longint lUsedMemory;
	int nAttribute;
	KWDGAttribute* dgAttribute;

	// Memoire de base
	lUsedMemory = sizeof(KWDataGrid);
	lUsedMemory += svTargetValues.GetUsedMemory();

	// Prise en compte des attribut (et leurs parties)
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = GetAttributeAt(nAttribute);
		if (dgAttribute != NULL)
			lUsedMemory += dgAttribute->GetUsedMemory();
	}

	// Prise en compte des cellules en se basant sur la taille de la premiere cellule
	if (GetHeadCell() != NULL)
		lUsedMemory += nCellNumber * GetHeadCell()->GetUsedMemory();
	if (slCells != NULL)
	{
		lUsedMemory += slCells->GetUsedMemory();
	}
	return lUsedMemory;
}

void KWDataGrid::SortAttributeParts()
{
	int nAttribute;
	KWDGAttribute* attribute;

	// Tri des parties pour chaque attribut
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		attribute->SortParts();
	}
}

boolean KWDataGrid::AreAttributePartsSorted() const
{
	int nAttribute;
	KWDGAttribute* attribute;
	boolean bIsSorted = true;

	// Verification du tri des parties pour chaque attribut
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		bIsSorted = attribute->ArePartsSorted();
		if (not bIsSorted)
			break;
	}
	return bIsSorted;
}

void KWDataGrid::ImportDataGridStats(const KWDataGridStats* dataGridStats)
{
	int nAttributeNumber;
	int nTargetValueNumber;
	int nTarget;
	int nAttribute;
	const KWDGSAttributePartition* attributePartition;
	const KWDGSAttributeSymbolValues* targetAttributeSymbolValues;
	KWDGAttribute* attribute;
	int nPart;
	KWDGPart* part;
	KWDGInterval* interval;
	KWDGSymbolValueSet* symbolValueSet;
	int nValue;
	ObjectArray oaAttributePartitions;
	ObjectArray* oaPartition;
	int nGridSize;
	int nCell;
	KWDGCell* cell;
	int nCellFrequency;
	ObjectArray oaParts;
	IntVector ivPartIndexes;

	require(IsEmpty());
	require(dataGridStats != NULL);
	require(dataGridStats->Check());
	require(dataGridStats->GetSourceAttributeNumber() == 0 or dataGridStats->GetTargetAttributeNumber() == 1);

	// On determine les caracteristiques de la grille a creer
	nTargetValueNumber = 0;
	nAttributeNumber = dataGridStats->GetAttributeNumber();
	nGranularity = dataGridStats->GetGranularity();
	targetAttributeSymbolValues = NULL;
	if (dataGridStats->GetTargetAttributeNumber() == 1)
	{
		attributePartition = dataGridStats->GetAttributeAt(dataGridStats->GetFirstTargetAttributeIndex());

		// Creation de valeurs cible uniquement dans le cas de valeurs symboliques
		if (attributePartition->GetAttributeType() == KWType::Symbol and
		    attributePartition->ArePartsSingletons())
		{
			targetAttributeSymbolValues = cast(const KWDGSAttributeSymbolValues*, attributePartition);
			nTargetValueNumber = attributePartition->GetPartNumber();
			nAttributeNumber--;
		}
	}
	assert(targetAttributeSymbolValues == NULL or nTargetValueNumber > 0);

	// Creation de la grille
	Initialize(nAttributeNumber, nTargetValueNumber);

	// Initialisation des eventuelles valeurs cibles
	if (targetAttributeSymbolValues != NULL)
	{
		for (nTarget = 0; nTarget < targetAttributeSymbolValues->GetValueNumber(); nTarget++)
			SetTargetValueAt(nTarget, targetAttributeSymbolValues->GetValueAt(nTarget));
	}

	// Initialisation des attributs, en memorisant leur partition (qui permettra de construire les cellules)
	oaAttributePartitions.SetSize(nAttributeNumber);
	for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
	{
		attributePartition = dataGridStats->GetAttributeAt(nAttribute);
		attribute = GetAttributeAt(nAttribute);

		// Initialisation des caracteristiques standards de l'attribut
		attribute->SetAttributeName(attributePartition->GetAttributeName());
		attribute->SetAttributeType(attributePartition->GetAttributeType());

		attribute->SetInitialValueNumber(attributePartition->GetInitialValueNumber());
		attribute->SetGranularizedValueNumber(attributePartition->GetGranularizedValueNumber());

		// L'attribut est marque comme cible s'il s'agit d'un probleme supervise, avec des attributs sources
		if (dataGridStats->GetSourceAttributeNumber() > 0 and targetAttributeSymbolValues == NULL and
		    nAttribute == dataGridStats->GetFirstTargetAttributeIndex())
			attribute->SetAttributeTargetFunction(true);

		// Creation d'un tableau pour memoriser la partition de l'attribut
		oaPartition = new ObjectArray;
		oaPartition->SetSize(attributePartition->GetPartNumber());
		oaAttributePartitions.SetAt(nAttribute, oaPartition);

		// Initialisation des parties des attributs en fonction du type d'attribut
		// Cas d'une discretisation
		if (attributePartition->GetAttributeType() == KWType::Continuous and
		    not attributePartition->ArePartsSingletons())
		{
			const KWDGSAttributeDiscretization* attributeDiscretization;
			attributeDiscretization = cast(const KWDGSAttributeDiscretization*, attributePartition);

			// Initialisation des intervalles
			for (nPart = 0; nPart < attributeDiscretization->GetPartNumber(); nPart++)
			{
				part = attribute->AddPart();
				oaPartition->SetAt(nPart, part);
				interval = part->GetInterval();

				// Borne inf
				interval->SetLowerBound(KWDGInterval::GetMinLowerBound());
				if (nPart > 0)
					interval->SetLowerBound(attributeDiscretization->GetIntervalBoundAt(nPart - 1));

				// Borne sup
				interval->SetUpperBound(KWDGInterval::GetMaxUpperBound());
				if (nPart < attributeDiscretization->GetPartNumber() - 1)
					interval->SetUpperBound(attributeDiscretization->GetIntervalBoundAt(nPart));
			}
		}
		// Cas d'un ensemble de valeurs, transforme en discretisation
		else if (attributePartition->GetAttributeType() == KWType::Continuous and
			 attributePartition->ArePartsSingletons())
		{
			const KWDGSAttributeContinuousValues* attributeContinuousValues;
			attributeContinuousValues = cast(const KWDGSAttributeContinuousValues*, attributePartition);

			// Initialisation des intervalles a partir des valeurs
			for (nPart = 0; nPart < attributeContinuousValues->GetPartNumber(); nPart++)
			{
				part = attribute->AddPart();
				oaPartition->SetAt(nPart, part);
				interval = part->GetInterval();

				// Borne inf
				interval->SetLowerBound(KWDGInterval::GetMinLowerBound());
				if (nPart > 0)
					interval->SetLowerBound(KWContinuous::GetHumanReadableLowerMeanValue(
					    attributeContinuousValues->GetValueAt(nPart - 1),
					    attributeContinuousValues->GetValueAt(nPart)));

				// Borne sup
				interval->SetUpperBound(KWDGInterval::GetMaxUpperBound());
				if (nPart < attributeContinuousValues->GetPartNumber() - 1)
					interval->SetUpperBound(KWContinuous::GetHumanReadableLowerMeanValue(
					    attributeContinuousValues->GetValueAt(nPart),
					    attributeContinuousValues->GetValueAt(nPart + 1)));
			}
		}
		// Cas d'un groupement de valeurs
		else if (attributePartition->GetAttributeType() == KWType::Symbol and
			 not attributePartition->ArePartsSingletons())
		{
			const KWDGSAttributeGrouping* attributeGrouping;
			attributeGrouping = cast(const KWDGSAttributeGrouping*, attributePartition);

			// Initialisation des groupes
			for (nPart = 0; nPart < attributeGrouping->GetPartNumber(); nPart++)
			{
				part = attribute->AddPart();
				oaPartition->SetAt(nPart, part);
				symbolValueSet = part->GetSymbolValueSet();

				// Initialisation des valeurs du groupe
				for (nValue = attributeGrouping->GetGroupFirstValueIndexAt(nPart);
				     nValue <= attributeGrouping->GetGroupLastValueIndexAt(nPart); nValue++)
					symbolValueSet->AddSymbolValue(attributeGrouping->GetValueAt(nValue));

				// Cas du groupe poubelle : memorisation de la partie au niveau de l'attribut
				if (nPart == attributeGrouping->GetGarbageGroupIndex())
					attribute->SetGarbagePart(part);
			}
		}
		// Cas d'un groupement de valeurs construit a partir d'une liste de valeurs
		else if (attributePartition->GetAttributeType() == KWType::Symbol and
			 attributePartition->ArePartsSingletons())
		{
			const KWDGSAttributeSymbolValues* attributeSymbolValues;
			attributeSymbolValues = cast(const KWDGSAttributeSymbolValues*, attributePartition);

			// Initialisation des groupes
			for (nPart = 0; nPart < attributeSymbolValues->GetValueNumber(); nPart++)
			{
				part = attribute->AddPart();
				oaPartition->SetAt(nPart, part);
				symbolValueSet = part->GetSymbolValueSet();

				// Initialisation de la valeur du groupe
				assert(attributeSymbolValues->GetValueAt(nPart) != Symbol::GetStarValue());
				symbolValueSet->AddSymbolValue(attributeSymbolValues->GetValueAt(nPart));
			}

			// Ajout de la valeur speciale
			assert(attributeSymbolValues->GetValueNumber() > 0);
			symbolValueSet = attribute->GetTailPart()->GetSymbolValueSet();
			symbolValueSet->AddSymbolValue(Symbol::GetStarValue());
		}
	}

	// Passage en mode update pour la creation des cellules
	SetCellUpdateMode(true);

	// Creation des cellules
	ivPartIndexes.SetSize(dataGridStats->GetAttributeNumber());
	oaParts.SetSize(GetAttributeNumber());
	nGridSize = dataGridStats->ComputeTotalGridSize();
	for (nCell = 0; nCell < nGridSize; nCell++)
	{
		// Calcul des index de partie correspondant a l'index de cellule
		dataGridStats->ComputePartIndexes(nCell, &ivPartIndexes);

		// Creation/mise a jour d'une cellule si effectif non nul
		nCellFrequency = dataGridStats->GetCellFrequencyAt(&ivPartIndexes);
		if (nCellFrequency > 0)
		{
			// Specification des parties de la cellule
			for (nAttribute = 0; nAttribute < oaParts.GetSize(); nAttribute++)
			{
				oaPartition = cast(ObjectArray*, oaAttributePartitions.GetAt(nAttribute));
				oaParts.SetAt(nAttribute, oaPartition->GetAt(ivPartIndexes.GetAt(nAttribute)));
			}

			// Dans le cas il n'y a pas d'attribut cible implicite, toutes les cellules sont transferees
			if (GetTargetValueNumber() == 0)
			{
				cell = AddCell(&oaParts);
				cell->SetCellFrequency(nCellFrequency);
			}
			// Sinon, il faut transferer les cellules pour chaque classe cible
			else
			{
				// Creation de la cellule si necessaire
				cell = LookupCell(&oaParts);
				if (cell == NULL)
					cell = AddCell(&oaParts);

				// Mise a jour des effectifs pour la classe cible
				nTarget = ivPartIndexes.GetAt(ivPartIndexes.GetSize() - 1);
				cell->UpgradeTargetFrequencyAt(nTarget, nCellFrequency);
			}
		}
	}

	// Fin du mode update de creation des cellules
	SetCellUpdateMode(false);

	// Nettoyage
	oaAttributePartitions.DeleteAll();

	ensure(Check());
	ensure(GetGridFrequency() == dataGridStats->ComputeGridFrequency());
	ensure(GetTargetValueNumber() > 0 or GetAttributeNumber() == dataGridStats->GetAttributeNumber());
	ensure(GetTargetValueNumber() == 0 or GetAttributeNumber() == dataGridStats->GetAttributeNumber() - 1);
}

void KWDataGrid::ExportDataGridStats(KWDataGridStats* dataGridStats) const
{
	int nTarget;
	int nAttribute;
	KWDGSAttributePartition* attributePartition;
	KWDGSAttributeDiscretization* attributeDiscretization;
	KWDGSAttributeGrouping* attributeGrouping;
	KWDGSAttributeSymbolValues* targetAttributeSymbolValues;
	ObjectDictionary odAttributeNames;
	const ALString sTargetPrefix = "Target";
	ALString sTargetAttributeName;
	int nIndex;
	KWDGAttribute* attribute;
	int nPart;
	KWDGPart* part;
	KWDGInterval* interval;
	KWDGSymbolValueSet* symbolValueSet;
	int nValue;
	int nValueNumber;
	int nSuppressedValueNumber;
	KWDGValue* value;
	KWDGCell* cell;
	IntVector ivPartIndexes;
	NumericKeyDictionary nkdPartIndexes;
	KWSortableIndex* partIndex;
	KWDGSymbolValueSet* cleanedSymbolValueSet;

	require(Check());
	require(dataGridStats != NULL);
	require(dataGridStats->GetAttributeNumber() == 0);

	dataGridStats->SetGranularity(nGranularity);

	cleanedSymbolValueSet = NULL;

	// Creation des attributs sources de la grille
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		assert(attribute != GetTargetAttribute() or nAttribute == GetAttributeNumber() - 1);

		// Creation d'un attribut dans le cas Symbol
		attributePartition = NULL;
		if (attribute->GetAttributeType() == KWType::Symbol)
		{
			// Memorisation de la taille du fourre-tout de l'attribut avant nettoyage eventuel
			attribute->SetCatchAllValueNumber(attribute->GetCatchAllValueNumber());

			// Creation
			attributeGrouping = new KWDGSAttributeGrouping;
			attributePartition = attributeGrouping;
			attributePartition->SetAttributeName(attribute->GetAttributeName());
			attributePartition->SetInitialValueNumber(attribute->GetInitialValueNumber());
			attributePartition->SetGranularizedValueNumber(attribute->GetGranularizedValueNumber());
			attributeGrouping->SetCatchAllValueNumber(attribute->GetCatchAllValueNumber());
			attributeGrouping->SetGarbageModalityNumber(attribute->GetGarbageModalityNumber());

			// Comptage du nombre total de valeurs, en partant de 1 pour compter la StarValue
			nSuppressedValueNumber = 0;
			nValueNumber = 1;
			part = attribute->GetHeadPart();
			while (part != NULL)
			{
				symbolValueSet = part->GetSymbolValueSet();
				nValueNumber += symbolValueSet->GetValueNumber();

				// Calcul du nombre eventuel de modalites qui seront nettoyees (non memorisees pour
				// l'affichage) En presence d'un fourre-tout, les modalites du fourre-tout autres que
				// celle d'effectif le plus eleve sont remplacees par la StarValue (cela est fait au
				// moment de la granularisation) Si le fourre-tout est dans un groupe de modalites, les
				// autres modalites sont conservees En l'absence de fourre-tout et en presence d'un
				// groupe poubelle, ce groupe poubelle est represente par la modalite la plus frequente
				// + StarValue Cas de la partie poubelle en l'absence de fourre-tout
				if (attribute->GetGarbagePart() == part and attribute->GetCatchAllValueNumber() == 0)
					nSuppressedValueNumber = symbolValueSet->GetValueNumber() - 1;
				attribute->GetNextPart(part);
			}

			nValueNumber -= nSuppressedValueNumber;
			attributeGrouping->SetKeptValueNumber(nValueNumber);

			// Initialisation des groupes de valeurs
			attributeGrouping->SetPartNumber(attribute->GetPartNumber());
			part = attribute->GetHeadPart();
			nPart = 0;
			nValue = 0;
			while (part != NULL)
			{
				symbolValueSet = part->GetSymbolValueSet();

				// Regles d'affectation de la StarValue
				// - dans le fourre-tout en presence d'un fourre-tout et en l'absence d'un groupe
				// poubelle
				// - dans le groupe poubelle en la presence d'un groupe poubelle (qu'un groupe
				// fourre-tout soit present ou pas)
				// - dans le groupe contenant la modalite de plus faible effectif en l'absence de
				// fourre-tout et de groupe poubelle

				// Cas de la partie par defaut differente du groupe poubelle en l'absence de fourre-tout
				// et en presence d'un groupe poubelle
				if (part->GetValueSet()->IsDefaultPart() and part != attribute->GetGarbagePart() and
				    attribute->GetCatchAllValueNumber() == 0 and
				    attribute->GetGarbageModalityNumber() > 0)
				{
					// Creation d'un valueSet nettoye sans la StarValue
					cleanedSymbolValueSet = cast(KWDGSymbolValueSet*, symbolValueSet->Create());
					value = symbolValueSet->GetHeadValue();
					while (value != NULL)
					{
						// Cas d'une modalite differente de la StarValue
						if (value->GetSymbolValue() != Symbol::GetStarValue())
							cleanedSymbolValueSet->AddSymbolValue(value->GetSymbolValue());
						symbolValueSet->GetNextValue(value);
					}
				}

				// Cas de la partie groupe poubelle
				else if (part == attribute->GetGarbagePart())
				{
					attributeGrouping->SetGarbageGroupIndex(nPart);

					// Cas de la presence d'un groupe poubelle sans presence de fourre-tout
					if (attribute->GetCatchAllValueNumber() == 0)
					{
						// Creation d'un ValueSet nettoye pour un affichage reduit a la modalite
						// la plus frequente
						cleanedSymbolValueSet =
						    cast(KWDGSymbolValueSet*, symbolValueSet->ComputeCleanedValueSet());
					}
				}

				// Memorisation de l'index de la partie dans un dictionnaire
				partIndex = new KWSortableIndex;
				partIndex->SetIndex(nPart);
				nkdPartIndexes.SetAt(part, partIndex);

				// Parametrage des valeurs de la partie
				attributeGrouping->SetGroupFirstValueIndexAt(nPart, nValue);

				// Cas ou une version nettoyee du valueSet de la partie a ete cree
				if (cleanedSymbolValueSet != NULL)
				{
					value = cleanedSymbolValueSet->GetHeadValue();
					while (value != NULL)
					{
						attributeGrouping->SetValueAt(nValue, value->GetSymbolValue());
						nValue++;
						cleanedSymbolValueSet->GetNextValue(value);
					}

					// Nettoyage
					delete cleanedSymbolValueSet;
					cleanedSymbolValueSet = NULL;
				}
				// Sinon
				else
				{
					value = symbolValueSet->GetHeadValue();
					while (value != NULL)
					{
						attributeGrouping->SetValueAt(nValue, value->GetSymbolValue());
						nValue++;
						symbolValueSet->GetNextValue(value);
					}
				}

				// Partie suivante
				attribute->GetNextPart(part);
				nPart++;
			}
			assert(nValue == nValueNumber);
		}
		// Creation d'un attribut dans le cas Continuous
		else if (attribute->GetAttributeType() == KWType::Continuous)
		{
			// Creation
			attributeDiscretization = new KWDGSAttributeDiscretization;
			attributePartition = attributeDiscretization;
			attributePartition->SetAttributeName(attribute->GetAttributeName());
			attributePartition->SetInitialValueNumber(attribute->GetInitialValueNumber());
			attributePartition->SetGranularizedValueNumber(attribute->GetGranularizedValueNumber());

			// Initialisation des bornes des intervalles
			attributeDiscretization->SetPartNumber(attribute->GetPartNumber());
			part = attribute->GetHeadPart();
			nPart = 0;
			while (part != NULL)
			{
				interval = part->GetInterval();

				// Memorisation de l'index de la partie dans un dictionnaire
				partIndex = new KWSortableIndex;
				partIndex->SetIndex(nPart);
				nkdPartIndexes.SetAt(part, partIndex);

				// On memorise la borne inf a partir du deusiemme intervalle
				if (nPart > 0)
					attributeDiscretization->SetIntervalBoundAt(nPart - 1,
										    interval->GetLowerBound());

				// Partie suivante
				attribute->GetNextPart(part);
				nPart++;
			}
		}
		check(attributePartition);

		// Ajout de l'attribut
		dataGridStats->AddAttribute(attributePartition);
	}

	// Gestion du cas particulier de l'attribut cible symbol implicite
	if (GetTargetValueNumber() > 0)
	{
		targetAttributeSymbolValues = new KWDGSAttributeSymbolValues;
		attributePartition = targetAttributeSymbolValues;

		// Calcul d'un nom unique pour l'attribut cible, en exploitant un dictionnaire des noms d'attribut
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
			odAttributeNames.SetAt(GetAttributeAt(nAttribute)->GetAttributeName(),
					       GetAttributeAt(nAttribute));
		sTargetAttributeName = sTargetPrefix;
		nIndex = 1;
		while (odAttributeNames.Lookup(sTargetAttributeName) != NULL)
		{
			sTargetAttributeName = sTargetPrefix + IntToString(nIndex);
			nIndex++;
		}
		attributePartition->SetAttributeName(sTargetAttributeName);

		// Initialisation des valeurs cibles
		targetAttributeSymbolValues->SetPartNumber(GetTargetValueNumber());
		for (nTarget = 0; nTarget < targetAttributeSymbolValues->GetValueNumber(); nTarget++)
			targetAttributeSymbolValues->SetValueAt(nTarget, GetTargetValueAt(nTarget));

		// Initialisation du nombre de valeurs initiales et granularisees
		attributePartition->SetInitialValueNumber(GetTargetValueNumber());
		attributePartition->SetGranularizedValueNumber(GetTargetValueNumber());

		// Ajout de l'attribut
		dataGridStats->AddAttribute(attributePartition);
	}

	// Parametrage du nombre d'attribut sources
	if (GetTargetAttribute() != NULL or GetTargetValueNumber() > 0)
		dataGridStats->SetSourceAttributeNumber(dataGridStats->GetAttributeNumber() - 1);

	// Creation des cellules
	dataGridStats->CreateAllCells();
	ivPartIndexes.SetSize(dataGridStats->GetAttributeNumber());
	cell = GetHeadCell();
	while (cell != NULL)
	{
		// Specification des index de parties de la cellule
		for (nAttribute = 0; nAttribute < cell->GetAttributeNumber(); nAttribute++)
		{
			// Recherche de l'index associe a la partie
			part = cell->GetPartAt(nAttribute);
			partIndex = cast(KWSortableIndex*, nkdPartIndexes.Lookup(part));
			check(partIndex);
			ivPartIndexes.SetAt(nAttribute, partIndex->GetIndex());
		}

		// Dans le cas il n'y a pas d'attribut cible implicite, creation directe de la cellule
		if (GetTargetValueNumber() == 0)
			dataGridStats->SetCellFrequencyAt(&ivPartIndexes, cell->GetCellFrequency());
		// Sinon, il faut transferer les cellules pour chaque classe cible
		else
		{
			assert(nAttribute == dataGridStats->GetFirstTargetAttributeIndex());
			for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
			{
				ivPartIndexes.SetAt(nAttribute, nTarget);
				dataGridStats->SetCellFrequencyAt(&ivPartIndexes, cell->GetTargetFrequencyAt(nTarget));
			}
		}

		// Cellule suivante
		GetNextCell(cell);
	}

	// Nettoyage
	nkdPartIndexes.DeleteAll();

	ensure(dataGridStats->Check());
	ensure(GetGridFrequency() == dataGridStats->ComputeGridFrequency());
	ensure(GetTargetValueNumber() > 0 or dataGridStats->GetAttributeNumber() == GetAttributeNumber());
	ensure(GetTargetValueNumber() == 0 or dataGridStats->GetAttributeNumber() == GetAttributeNumber() + 1);
	ensure(dataGridStats->GetSourceAttributeNumber() == 0 or dataGridStats->GetTargetAttributeNumber() <= 1);
	ensure(dataGridStats->GetSourceAttributeNumber() == 0 or GetTargetValueNumber() > 0 or
	       GetTargetAttribute() != NULL);
}

void KWDataGrid::Write(ostream& ost) const
{
	// Identification
	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\n";

	// Contenu
	if (GetTargetValueNumber() > 0)
		WriteTargetValues(ost);
	if (GetTargetAttribute() != NULL)
		ost << "TargetVariable\t" << GetTargetAttribute()->GetAttributeName() << "\n";
	ost << "Granularity\t" << GetGranularity() << "\n";
	if (IsVarPartDataGrid())
	{
		ost << "VariableParts Granularity\t"
		    << GetVarPartAttribute()->GetInnerAttributes()->GetVarPartGranularity() << "\n";
		WriteInnerAttributes(ost);
	}
	if (GetAttributeNumber() > 0)
	{
		WriteAttributes(ost);
		WriteAttributeParts(ost);
	}
	if (GetCellNumber() > 0)
		WriteCells(ost);
}

void KWDataGrid::WriteTargetValues(ostream& ost) const
{
	int nTargetIndex;

	// Liste des valeurs cibles
	ost << "Target values"
	    << "\t" << GetTargetValueNumber() << "\n";
	for (nTargetIndex = 0; nTargetIndex < GetTargetValueNumber(); nTargetIndex++)
		ost << "\t" << svTargetValues.GetAt(nTargetIndex) << "\n";
}

void KWDataGrid::WriteAttributes(ostream& ost) const
{
	int nAttribute;
	KWDGAttribute* attribute;

	// Liste des attributs
	ost << "Variables"
	    << "\t" << GetAttributeNumber() << "\n";
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		ost << "\t" << attribute->GetAttributeName() << "\t" << KWType::ToString(attribute->GetAttributeType());

		// Cas d'un attribut groupable : affichage du groupe poubelle eventuel
		if (KWType::IsCoclusteringGroupableType(attribute->GetAttributeType()) and
		    not attribute->GetAttributeTargetFunction())
		{
			ost << "\tCatch all size " << attribute->GetCatchAllValueNumber() << "\n";
			ost << "\t"
			    << (attribute->GetInitialValueNumber() >
				KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
			    << "\t" << (attribute->GetGarbagePart() != NULL);
			ost << "\t" << attribute->GetGarbageModalityNumber();
		}
		ost << "\n";
	}
}

void KWDataGrid::WriteAttributeParts(ostream& ost) const
{
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	const int nMaxDisplayedValue = 20;
	int nDisplayedValue;
	KWDGValue* value;
	boolean bDisplayAll = false;
	boolean bDisplayPartDetails = true;

	// Liste des attributs et de leurs parties
	ost << "Parts by variable"
	    << "\t" << GetTotalPartNumber() << "\n";
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		ost << "\t" << attribute->GetAttributeName() << "\t" << attribute->GetPartNumber() << "\n";

		// Parties de l'attribut
		if (bDisplayPartDetails)
		{
			part = attribute->GetHeadPart();
			while (part != NULL)
			{
				ost << "\t\t" << part->GetObjectLabel() << "\t" << part->GetPartFrequency();

				// Affichage des premieres valeurs dans le cas d'un attribut groupable
				if (KWType::IsCoclusteringGroupableType(attribute->GetAttributeType()))
				{
					nDisplayedValue = 0;
					value = part->GetValueSet()->GetHeadValue();
					while (value != NULL)
					{
						nDisplayedValue++;
						if (nDisplayedValue > nMaxDisplayedValue)
						{
							ost << "\t...";
							break;
						}
						else
							ost << "\t" << value->GetObjectLabel();
						part->GetValueSet()->GetNextValue(value);
					}
				}

				// Fin de ligne
				ost << "\n";

				// Affichage complet des valeurs dans le cas d'un attribut groupable
				if (bDisplayAll and KWType::IsCoclusteringGroupableType(attribute->GetAttributeType()))
				{
					cout << part->GetValueSet()->GetClassLabel() << "\n";
					part->GetValueSet()->Write(cout);
				}

				// Partie suivante
				attribute->GetNextPart(part);
			}
		}
	}
}

void KWDataGrid::WriteCells(ostream& ost) const
{
	int nAttribute;
	int nTargetIndex;
	KWDGAttribute* attribute;
	ObjectArray oaCells;
	KWDGCell* cell;
	int nCell;
	int nCellFrequency;
	int nCellCumulativeFrequency;
	double dGridFrequency;

	// Affichage de l'entete
	ost << "Cells\t" << GetCellNumber() << "\n";
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		ost << attribute->GetAttributeName() << "\t";
	}
	for (nTargetIndex = 0; nTargetIndex < GetTargetValueNumber(); nTargetIndex++)
		ost << svTargetValues.GetAt(nTargetIndex) << "\t";
	ost << "Frequency\tCoverage\tCumulative coverage\n";

	// Tri des cellules par valeurs des parties d'attribut (et non par adresse)
	// en les rentrant prealablement dans un tableau
	oaCells.SetSize(GetCellNumber());
	cell = headCell;
	nCell = 0;
	while (cell != NULL)
	{
		oaCells.SetAt(nCell, cell);
		cell = cell->nextCell;
		nCell++;
	}
	oaCells.SetCompareFunction(KWDGCellCompareDecreasingFrequency);
	oaCells.Sort();

	// Affichage des cellules
	dGridFrequency = GetGridFrequency();
	nCellCumulativeFrequency = 0;
	for (nCell = 0; nCell < oaCells.GetSize(); nCell++)
	{
		cell = cast(KWDGCell*, oaCells.GetAt(nCell));

		// Affichage des identifiants des parties de la cellule
		for (nAttribute = 0; nAttribute < cell->GetAttributeNumber(); nAttribute++)
		{
			if (cell->GetPartAt(nAttribute) == NULL)
				ost << "null\t";
			else
				ost << cell->GetPartAt(nAttribute)->GetObjectLabel() << "\t";
		}

		// Affichage des effectifs par classe cible
		nCellFrequency = cell->GetCellFrequency();
		nCellCumulativeFrequency += nCellFrequency;
		for (nTargetIndex = 0; nTargetIndex < cell->GetTargetValueNumber(); nTargetIndex++)
		{
			if (nCellFrequency == 0)
				ost << "0\t";
			else
				ost << cell->GetTargetFrequencyAt(nTargetIndex) * 1.0 / nCellFrequency << "\t";
		}
		ost << nCellFrequency << "\t";
		if (dGridFrequency == 0)
			ost << "0\t0\n";
		else
			ost << nCellFrequency / dGridFrequency << "\t" << nCellCumulativeFrequency / dGridFrequency
			    << "\n";
	}
}

void KWDataGrid::WriteInnerAttributes(ostream& ost) const
{
	if (GetVarPartAttribute() != NULL)
		GetVarPartAttribute()->GetInnerAttributes()->Write(ost);
	else
		cout << "No inner variable\n";
}

void KWDataGrid::WriteCrossTableStats(ostream& ost, int nTargetIndex) const
{
	KWDGAttribute* attribute1;
	KWDGAttribute* attribute2;
	ObjectArray oaAttribute1Parts;
	ObjectArray oaAttribute2Parts;
	KWDGPart* part1;
	KWDGPart* part2;
	int nPart1;
	int nPart2;
	NumericKeyDictionary nkdPartCells;
	KWDGCell* cell;
	double dGridFrequency;

	require(GetAttributeNumber() == 2);
	require(0 <= nTargetIndex and nTargetIndex <= GetTargetValueNumber());

	// Acces aux attributs
	attribute1 = cast(KWDGAttribute*, oaAttributes.GetAt(0));
	attribute2 = cast(KWDGAttribute*, oaAttributes.GetAt(1));

	// Tri des parties de l'attribut 1
	oaAttribute1Parts.SetSize(attribute1->GetPartNumber());
	nPart1 = 0;
	part1 = attribute1->GetHeadPart();
	while (part1 != NULL)
	{
		oaAttribute1Parts.SetAt(nPart1, part1);
		nPart1++;
		attribute1->GetNextPart(part1);
	}
	oaAttribute1Parts.SetCompareFunction(KWDGPartCompareValues);
	oaAttribute1Parts.Sort();

	// Tri des parties de l'attribut 2
	oaAttribute2Parts.SetSize(attribute2->GetPartNumber());
	nPart2 = 0;
	part2 = attribute2->GetHeadPart();
	while (part2 != NULL)
	{
		oaAttribute2Parts.SetAt(nPart2, part2);
		nPart2++;
		attribute2->GetNextPart(part2);
	}
	oaAttribute1Parts.SetCompareFunction(KWDGPartCompareValues);
	oaAttribute2Parts.Sort();

	// Affichage de la ligne d'entete
	if (nTargetIndex == GetTargetValueNumber())
		ost << "Cell frequencies\n";
	else
		ost << "% target value\t" << GetTargetValueAt(nTargetIndex) << "\n";
	ost << "\t" << TSV::Export(attribute2->GetAttributeName()) << "\n";
	ost << attribute1->GetAttributeName();
	dGridFrequency = GetGridFrequency();
	for (nPart2 = 0; nPart2 < oaAttribute2Parts.GetSize(); nPart2++)
	{
		part2 = cast(KWDGPart*, oaAttribute2Parts.GetAt(nPart2));

		// Libelle ligne de la partie de l'attribut 2
		ost << "\t" << TSV::Export(part2->GetObjectLabel());
	}
	ost << "\n";

	// Affichage des statistiques des lignes par partie de l'attribut 1
	for (nPart1 = 0; nPart1 < oaAttribute1Parts.GetSize(); nPart1++)
	{
		part1 = cast(KWDGPart*, oaAttribute1Parts.GetAt(nPart1));

		// Libelle ligne de la partie de l'attribut 1
		ost << TSV::Export(part1->GetObjectLabel());

		// Rangement des cellules lies a la partie de l'attribut 1 en utilisant
		// les parties comme cle d'acces
		nkdPartCells.RemoveAll();
		cell = part1->GetHeadCell();
		while (cell != NULL)
		{
			nkdPartCells.SetAt(cell->GetPartAt(1), cell);
			part1->GetNextCell(cell);
		}

		// Parcours des parties de l'attribut 2
		for (nPart2 = 0; nPart2 < oaAttribute2Parts.GetSize(); nPart2++)
		{
			part2 = cast(KWDGPart*, oaAttribute2Parts.GetAt(nPart2));

			// Affichage de la cellule si elle existe
			cell = cast(KWDGCell*, nkdPartCells.Lookup(part2));
			ost << "\t";
			if (cell == NULL or cell->GetCellFrequency() == 0 or dGridFrequency == 0)
				ost << "0";
			else
			{
				// Effectif de la cellule
				if (nTargetIndex == GetTargetValueNumber())
					ost << cell->GetCellFrequency();
				// Proportion de classe cible
				else
					ost << cell->GetTargetFrequencyAt(nTargetIndex) * 1.0 /
						   cell->GetCellFrequency();
			}
		}

		// Fin de ligne
		ost << "\n";
	}
}

const ALString KWDataGrid::GetClassLabel() const
{
	return "Data grid";
}

const ALString KWDataGrid::GetObjectLabel() const
{
	ALString sTmp;

	// Libelle base sur le nombre de valeurs, d'attributs, de parties et de cellules
	if (GetTargetValueNumber() > 0)
		return sTmp + "(" + IntToString(GetTargetValueNumber()) + ", " + IntToString(GetAttributeNumber()) +
		       ", " + IntToString(GetTotalPartNumber()) + ", " + IntToString(GetCellNumber()) + ")";
	else
		return sTmp + "(" + IntToString(GetAttributeNumber()) + ", " + IntToString(GetTotalPartNumber()) +
		       ", " + IntToString(GetCellNumber()) + ")";
}

KWDataGrid* KWDataGrid::CreateTestDataGrid(int nSymbolAttributeNumber, int nContinuousAttributeNumber,
					   int nAttributePartNumber, int nTargetValueNumber, int nInstanceNumber)
{
	KWDataGrid* testDataGrid;
	boolean bDisplayInstanceCreation = false;
	const ALString sAttributePrefix = "Att";
	const ALString sTargetValuePrefix = "T";
	const ALString sValuePrefix = "V";
	int nAttributeValueNumber;
	int nTargetValue;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGAttribute* continuousAttributeTemplate;
	KWDGAttribute* symbolAttributeTemplate;
	KWDGPart* partTemplate;
	KWDGPart* part;
	int nInstance;
	ObjectArray oaParts;
	Continuous cValue;
	Symbol sValue;
	KWDGCell* cell;

	require(nSymbolAttributeNumber >= 0);
	require(nContinuousAttributeNumber >= 0);
	require(nTargetValueNumber >= 0);
	require(nInstanceNumber >= 0);

	// Creation du DataGrid
	testDataGrid = new KWDataGrid;
	testDataGrid->Initialize(nSymbolAttributeNumber + nContinuousAttributeNumber, nTargetValueNumber);

	// Initialisation des valeurs cibles
	for (nTargetValue = 0; nTargetValue < testDataGrid->GetTargetValueNumber(); nTargetValue++)
	{
		testDataGrid->SetTargetValueAt(nTargetValue,
					       (Symbol)(sTargetValuePrefix + IntToString(nTargetValue + 1)));
	}

	/////////////////////////////////////////////////////////////////////////
	// Creation des attributs et de leurs parties

	// Creation d'attributs templates
	continuousAttributeTemplate = KWDGAttribute::CreateTestContinuousAttribute(1, 1, nAttributePartNumber);
	nAttributeValueNumber = (int)(nAttributePartNumber * 2.5);
	symbolAttributeTemplate = KWDGAttribute::CreateTestSymbolAttribute(nAttributeValueNumber, nAttributePartNumber);

	// Creation de ses attributs
	for (nAttribute = 0; nAttribute < testDataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = testDataGrid->GetAttributeAt(nAttribute);

		// Type de l'attribut
		if (nAttribute < nSymbolAttributeNumber)
			attribute->SetAttributeType(KWType::Symbol);
		else
			attribute->SetAttributeType(KWType::Continuous);

		// Nom de l'attribut
		attribute->SetAttributeName(sAttributePrefix + IntToString(nAttribute + 1));

		// Creation des parties par recopie des parties d'un attribut template
		if (attribute->GetAttributeType() == KWType::Symbol)
		{
			partTemplate = symbolAttributeTemplate->GetHeadPart();
			while (partTemplate != NULL)
			{
				part = attribute->AddPart();
				part->GetValueSet()->CopyFrom(partTemplate->GetValueSet());
				symbolAttributeTemplate->GetNextPart(partTemplate);
			}
		}
		else
		{
			partTemplate = continuousAttributeTemplate->GetHeadPart();
			while (partTemplate != NULL)
			{
				part = attribute->AddPart();
				part->GetInterval()->CopyFrom(partTemplate->GetInterval());
				continuousAttributeTemplate->GetNextPart(partTemplate);
			}
		}
	}

	// Nettoyage
	delete continuousAttributeTemplate;
	delete symbolAttributeTemplate;

	///////////////////////////////////////////////////////////////////////
	// Creation des cellules

	// Passage en mode update
	testDataGrid->SetCellUpdateMode(true);
	testDataGrid->BuildIndexingStructure();

	// Ajout d'instances dans le DataGrid
	oaParts.SetSize(testDataGrid->GetAttributeNumber());
	for (nInstance = 0; nInstance < nInstanceNumber; nInstance++)
	{
		// Recherche des parties pour les valeurs d'une instances cree aleatoirement
		for (nAttribute = 0; nAttribute < testDataGrid->GetAttributeNumber(); nAttribute++)
		{
			attribute = testDataGrid->GetAttributeAt(nAttribute);

			// Creation d'une valeur aleatoire selon le type d'attribut,
			// puis recherche de sa partie
			if (attribute->GetAttributeType() == KWType::Continuous)
			{
				cValue = (Continuous)(nAttributePartNumber * RandomDouble());
				part = attribute->LookupContinuousPart(cValue);
				oaParts.SetAt(nAttribute, part);
				if (bDisplayInstanceCreation)
					cout << cValue << "\t";
			}
			else
			{
				sValue = (Symbol)(sValuePrefix + IntToString(RandomInt(nAttributeValueNumber + 2)));
				part = attribute->LookupSymbolPart(sValue);
				oaParts.SetAt(nAttribute, part);
				if (bDisplayInstanceCreation)
					cout << sValue << "\t";
			}
		}

		// Creation de la classe cible
		nTargetValue = -1;
		if (testDataGrid->GetTargetValueNumber() > 0)
		{
			nTargetValue = RandomInt(testDataGrid->GetTargetValueNumber() - 1);
			if (bDisplayInstanceCreation)
				cout << testDataGrid->GetTargetValueAt(nTargetValue) << "\t";
		}

		// Recherche de la cellule
		cell = testDataGrid->LookupCell(&oaParts);

		// Creation si necessaire
		if (cell == NULL)
			cell = testDataGrid->AddCell(&oaParts);

		// Mise a jour des effectif de la classe cible
		if (testDataGrid->GetTargetValueNumber() > 0)
			cell->UpgradeTargetFrequencyAt(nTargetValue, 1);

		// Affichage de la cellule
		if (bDisplayInstanceCreation)
			cout << *cell;
	}

	// Fin du mode update
	testDataGrid->SetCellUpdateMode(false);
	testDataGrid->DeleteIndexingStructure();

	return testDataGrid;
}

void KWDataGrid::Test()
{
	boolean bTestCreate = true;
	boolean bTestImport = true;
	boolean bTestExport = true;
	boolean bShowDatagrids = false;
	KWDataGrid* testDataGrid;
	KWDataGridStats* testDataGridStats;
	KWDataGridStats exportedtestDataGridStats;
	KWDGAttribute* attribute;
	clock_t tBegin;
	clock_t tEnd;
	boolean bOk;
	double dTotalComputeTime;

	//////////////////////////////////////////////////////////////////////
	// Creation d'un DataGrid

	if (bTestCreate)
	{
		// Creation d'un DataGrid de test elementaire
		cout << "Creation of a basic test data grid" << endl;
		testDataGrid = CreateTestDataGrid(1, 1, 2, 2, 5);
		cout << *testDataGrid << endl;
		bOk = testDataGrid->Check();
		cout << "\tChecked\t" << bOk << endl;
		attribute = testDataGrid->GetAttributeAt(0);
		cout << "Variable complete report" << endl;
		cout << *attribute << endl;
		delete testDataGrid;

		// Creation d'un DataGrid de test
		cout << "Creation of a test data grid" << endl;
		tBegin = clock();
		testDataGrid = CreateTestDataGrid(5, 5, 3, 2, 100);
		tEnd = clock();
		dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;
		cout << "Created"
		     << "\t" << testDataGrid->GetObjectLabel() << "\t" << SecondsToString((int)dTotalComputeTime)
		     << endl;

		// Verification si pas trop de cellules
		if (testDataGrid->GetCellNumber() < 1000)
		{
			bOk = testDataGrid->Check();
			cout << "\tChecked\t" << bOk << endl;
		}

		// Affichage des cellules d'un attribut
		if (bShowDatagrids and testDataGrid->GetAttributeNumber() > 0 and testDataGrid->GetCellNumber() < 1000)
		{
			attribute = testDataGrid->GetAttributeAt(0);
			cout << "Variable complete report" << endl;
			cout << *attribute << endl;
		}

		// Affichage
		if (bShowDatagrids and testDataGrid->GetCellNumber() < 1000)
			cout << *testDataGrid << endl;

		// Nettoyage
		delete testDataGrid;
	}

	//////////////////////////////////////////////////////////////////////
	// Import/export de DataGridStats

	// Import d'un DataGridStats
	if (bTestImport)
	{
		// Non supervise
		cout << "Unsupervised (1)\n--------------------\n";
		testDataGridStats = KWDataGridStats::CreateTestDataGrid(0, 1, false, 0, 2, 3);
		cout << "Import of DataGridStats\n" << *testDataGridStats << endl;
		testDataGrid = new KWDataGrid;
		testDataGrid->ImportDataGridStats(testDataGridStats);
		if (bShowDatagrids)
			cout << "Imported DataGrid\n" << *testDataGrid << endl;
		if (bTestExport)
		{
			exportedtestDataGridStats.DeleteAll();
			testDataGrid->ExportDataGridStats(&exportedtestDataGridStats);
			if (bShowDatagrids)
				cout << "Re-exported of DataGridStats\n" << exportedtestDataGridStats << endl;
			cout << "\tChecked\t" << exportedtestDataGridStats.Check() << endl;
			cout << "\tCompare\t" << (testDataGridStats->Compare(&exportedtestDataGridStats) == 0) << endl;
		}
		delete testDataGrid;
		delete testDataGridStats;
		//
		cout << "Unsupervised (2)\n--------------------\n";
		testDataGridStats = KWDataGridStats::CreateTestDataGrid(1, 1, false, 0, 2, 3);
		if (bShowDatagrids)
			cout << "Import of DataGridStats\n" << *testDataGridStats << endl;
		testDataGrid = new KWDataGrid;
		testDataGrid->ImportDataGridStats(testDataGridStats);
		if (bShowDatagrids)
			cout << "Imported DataGrid\n" << *testDataGrid << endl;
		if (bTestExport)
		{
			exportedtestDataGridStats.DeleteAll();
			testDataGrid->ExportDataGridStats(&exportedtestDataGridStats);
			if (bShowDatagrids)
				cout << "Re-exported of DataGridStats\n" << exportedtestDataGridStats << endl;
			cout << "\tChecked\t" << exportedtestDataGridStats.Check() << endl;
			cout << "\tCompare\t" << (testDataGridStats->Compare(&exportedtestDataGridStats) == 0) << endl;
		}
		delete testDataGrid;
		delete testDataGridStats;
		//
		cout << "Unsupervised simple(3)\n--------------------\n";
		testDataGridStats = KWDataGridStats::CreateTestDataGrid(2, 1, true, 0, 2, 3);
		if (bShowDatagrids)
			cout << "Import of DataGridStats\n" << *testDataGridStats << endl;
		testDataGrid = new KWDataGrid;
		testDataGrid->ImportDataGridStats(testDataGridStats);
		if (bShowDatagrids)
			cout << "Imported DataGrid\n" << *testDataGrid << endl;
		if (bTestExport)
		{
			exportedtestDataGridStats.DeleteAll();
			testDataGrid->ExportDataGridStats(&exportedtestDataGridStats);
			if (bShowDatagrids)
				cout << "Re-exported of DataGridStats\n" << exportedtestDataGridStats << endl;
			cout << "\tChecked\t" << exportedtestDataGridStats.Check() << endl;
			cout << "\tCompare\t" << (testDataGridStats->Compare(&exportedtestDataGridStats) == 0) << endl;
		}
		delete testDataGrid;
		delete testDataGridStats;

		// Supervise
		cout << "Supervised (1, 1)\n--------------------\n";
		testDataGridStats = KWDataGridStats::CreateTestDataGrid(1, 1, false, 1, 2, 3);
		if (bShowDatagrids)
			cout << "Import of DataGridStats\n" << *testDataGridStats << endl;
		testDataGrid = new KWDataGrid;
		testDataGrid->ImportDataGridStats(testDataGridStats);
		if (bShowDatagrids)
			cout << "Imported DataGrid\n" << *testDataGrid << endl;
		if (bTestExport)
		{
			exportedtestDataGridStats.DeleteAll();
			testDataGrid->ExportDataGridStats(&exportedtestDataGridStats);
			if (bShowDatagrids)
				cout << "Re-exported of DataGridStats\n" << exportedtestDataGridStats << endl;
			cout << "\tChecked\t" << exportedtestDataGridStats.Check() << endl;
			cout << "\tCompare\t" << (testDataGridStats->Compare(&exportedtestDataGridStats) == 0) << endl;
		}
		delete testDataGrid;
		delete testDataGridStats;
		//
		cout << "Supervised simple (2, 1)\n--------------------\n";
		testDataGridStats = KWDataGridStats::CreateTestDataGrid(3, 0, true, 2, 2, 3);
		if (bShowDatagrids)
			cout << "Import of DataGridStats\n" << *testDataGridStats << endl;
		testDataGrid = new KWDataGrid;
		testDataGrid->ImportDataGridStats(testDataGridStats);
		if (bShowDatagrids)
			cout << "Imported DataGrid\n" << *testDataGrid << endl;
		if (bTestExport)
		{
			exportedtestDataGridStats.DeleteAll();
			testDataGrid->ExportDataGridStats(&exportedtestDataGridStats);
			if (bShowDatagrids)
				cout << "Re-exported of DataGridStats\n" << exportedtestDataGridStats << endl;
			cout << "\tChecked\t" << exportedtestDataGridStats.Check() << endl;
			cout << "\tCompare\t" << testDataGridStats->Compare(&exportedtestDataGridStats) << endl;
		}
		delete testDataGrid;
		delete testDataGridStats;
	}
}

int KWDataGrid::ComputeGridFrequency() const
{
	int nResult;
	KWDGCell* cell;

	// Calcul par parcours des cellules
	nResult = 0;
	cell = GetHeadCell();
	while (cell != NULL)
	{
		nResult += cell->GetCellFrequency();
		GetNextCell(cell);
	}
	return nResult;
}

double KWDataGrid::ComputeLnGridSize() const
{
	double dResult;
	int nAttribute;
	KWDGAttribute* attribute;

	// Calcul par parcours des attributs
	dResult = 0;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		dResult += log((double)attribute->GetPartNumber());
	}
	return dResult;
}

int KWDataGrid::ComputeInformativeAttributeNumber() const
{
	int nResult;
	int nAttribute;
	KWDGAttribute* attribute;

	// Calcul par parcours des attributs
	nResult = 0;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		if (attribute->GetPartNumber() > 1)
			nResult++;
	}
	return nResult;
}

int KWDataGrid::ComputeTotalPartNumber() const
{
	int nResult;
	int nAttribute;
	KWDGAttribute* attribute;

	// Calcul par parcours des attributs
	nResult = 0;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));
		nResult += attribute->GetPartNumber();
	}
	return nResult;
}

KWDGCell* KWDataGrid::NewCell() const
{
	KWDGCell* cell;

	// Creation de la cellule
	cell = new KWDGCell;

	// Initialisation du vecteur des effectifs des valeurs cibles
	cell->ivFrequencyVector.SetSize(GetTargetValueNumber());
	return cell;
}

boolean KWDataGrid::GetEmulated() const
{
	return false;
}

int KWDataGridCompare(const void* elem1, const void* elem2)
{
	KWDataGrid* dataGrid1;
	KWDataGrid* dataGrid2;
	int nAttribute;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la grille
	dataGrid1 = cast(KWDataGrid*, *(Object**)elem1);
	dataGrid2 = cast(KWDataGrid*, *(Object**)elem2);

	// Comparaison sur la valeur de tri
	nDiff = dataGrid1->GetSortValue() - dataGrid2->GetSortValue();

	// En cas d'egalite, on se base sur la comparaison des nom des attributs
	if (nDiff == 0)
	{
		// Comparaison des noms des premiers attributs
		for (nAttribute = 0; nAttribute < dataGrid1->GetAttributeNumber(); nAttribute++)
		{
			// Comparaison du nom de l'attribut courant, s'il y a assez d'attribut dans l'autre grille
			if (nAttribute < dataGrid2->GetAttributeNumber())
				nDiff = dataGrid1->GetAttributeAt(nAttribute)
					    ->GetAttributeName()
					    .Compare(dataGrid2->GetAttributeAt(nAttribute)->GetAttributeName());
			else
				break;
			if (nDiff != 0)
				break;
		}

		// Si egalite, comparaison sur les nombres d'attributs
		if (nDiff == 0)
			nDiff = dataGrid1->GetAttributeNumber() - dataGrid2->GetAttributeNumber();
	}
	return nDiff;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGAttribute

KWDGAttribute::KWDGAttribute()
{
	dataGrid = NULL;
	nAttributeType = KWType::Unknown;
	nAttributeIndex = -1;
	dCost = 0;
	headPart = NULL;
	tailPart = NULL;
	nPartNumber = 0;
	bIsIndexed = false;
	defaultPart = NULL;
	bTargetAttribute = false;
	nInitialValueNumber = 0;
	nGranularizedValueNumber = 0;
	nCatchAllValueNumber = -1;
	catchAllValueSet = NULL;
	garbagePart = NULL;
	sOwnerAttributeName = "";
	bVarPartsShared = false;
	innerAttributes = NULL;
}

KWDGAttribute::~KWDGAttribute()
{
	// Destruction de tous les parties
	DeleteAllParts();

	// Destruction du fourre-tout
	if (catchAllValueSet != NULL)
	{
		catchAllValueSet->DeleteAllValues();
		delete catchAllValueSet;
	}

	// Destruction des attribut internes s'ils ne sont pas partages
	if (not bVarPartsShared and innerAttributes != NULL)
	{
		innerAttributes->DeleteAll();
		delete innerAttributes;
	}

	// Reinitialisation en mode debug, pour faciliter le diagnostique
	debug(dataGrid = NULL);
	debug(nAttributeType = KWType::Unknown);
	debug(nAttributeIndex = -1);
	debug(nInitialValueNumber = 0);
	debug(nGranularizedValueNumber = 0);
	debug(headPart = NULL);
	debug(tailPart = NULL);
	debug(nPartNumber = 0);
	debug(bIsIndexed = false);
	debug(catchAllValueSet = NULL);
	debug(innerAttributes = NULL);
}

// CH IV Begin
void KWDGAttribute::CreateVarPartsSet()
{
	KWDGPart* part;
	KWDGPart* currentPart;
	KWDGAttribute* innerAttribute;
	int nInnerAttribute;

	require(nAttributeType == KWType::VarPart);
	require(GetInnerAttributeNumber() > 0);

	for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber(); nInnerAttribute++)
	{
		innerAttribute = GetInnerAttributeAt(nInnerAttribute);
		currentPart = innerAttribute->GetHeadPart();

		// Parcours des parties de variables de l'attribut
		while (currentPart != NULL)
		{
			// On cree un cluster par parties de variables
			part = AddPart();
			part->GetVarPartSet()->AddVarPart(currentPart);

			// Partie suivante
			innerAttribute->GetNextPart(currentPart);
		}
	}
}
// CH IV End

KWDGPart* KWDGAttribute::AddPart()
{
	KWDGPart* part;

	require(KWType::IsCoclusteringType(GetAttributeType()));
	require(not IsIndexed());

	// Creation d'une nouvelle partie en fonction du type de l'attribut
	part = NewPart();
	part->SetPartType(GetAttributeType());

	// On connecte la partie a l'attribut
	part->attribute = this;

	// Ajout en fin de la liste des parties
	nPartNumber++;
	if (headPart == NULL)
		headPart = part;
	if (tailPart != NULL)
	{
		tailPart->nextPart = part;
		part->prevPart = tailPart;
	}
	tailPart = part;

	// On retourne la partie cree
	return part;
}

void KWDGAttribute::DeletePart(KWDGPart* part)
{
	require(part != NULL);
	require(CheckPart(part));
	require(not IsIndexed());

	// Supression de la liste des parties
	nPartNumber--;
	if (part->prevPart != NULL)
		part->prevPart->nextPart = part->nextPart;
	if (part->nextPart != NULL)
		part->nextPart->prevPart = part->prevPart;
	if (headPart == part)
		headPart = part->nextPart;
	if (tailPart == part)
		tailPart = part->prevPart;

	// Cas ou la partie est le groupe poubelle
	if (part == garbagePart)
		garbagePart = NULL;

	// Destruction de la partie
	delete part;
}

void KWDGAttribute::DeleteAllParts()
{
	KWDGPart* part;
	KWDGPart* partToDelete;

	require(not IsIndexed());

	// Destruction des parties
	part = headPart;
	while (part != NULL)
	{
		partToDelete = part;
		part = part->nextPart;
		delete partToDelete;
	}
	headPart = NULL;
	tailPart = NULL;
	nPartNumber = 0;
	garbagePart = NULL;
}

boolean KWDGAttribute::CheckPart(KWDGPart* part) const
{
	boolean bOk;
	KWDGPart* currentPart;

	require(part != NULL);

	// Recherche de la partie dans la liste des parties de l'attribut
	bOk = false;
	currentPart = headPart;
	while (currentPart != NULL)
	{
		if (currentPart == part)
		{
			bOk = true;
			break;
		}
		currentPart = currentPart->nextPart;
	}
	return bOk;
}

void KWDGAttribute::ExportParts(ObjectArray* oaParts) const
{
	KWDGPart* part;
	int nPart;

	require(oaParts != NULL);
	require(oaParts->GetSize() == 0);

	// Ajout des parties dans le tableau
	oaParts->SetSize(nPartNumber);
	nPart = 0;
	part = headPart;
	while (part != NULL)
	{
		oaParts->SetAt(nPart, part);
		nPart++;
		part = part->nextPart;
	}
}

void KWDGAttribute::BuildIndexingStructure()
{
	KWDGPart* part;
	KWDGValueSet* valueSet;
	KWDGValue* value;
	KWDGAttribute* innerAttribute;
	int nInnerAttribute;

	require(KWType::IsCoclusteringType(GetAttributeType()));
	require(Check());

	// Indexation si necessaire
	if (not bIsIndexed)
	{
		assert(oaIntervals.GetSize() == 0);
		assert(nkdParts.GetCount() == 0);

		// Indexation des intervalles si attribut numerique
		if (GetAttributeType() == KWType::Continuous)
		{
			// Ajout des parties numeriques dans le tableau des intervalles
			ExportParts(&oaIntervals);

			// Tri des intervalles par borne inf
			oaIntervals.SetCompareFunction(KWDGPartCompareValues);
			oaIntervals.Sort();
		}
		// Sinon, indexation des parties par les valeurs
		else
		{
			assert(KWType::IsCoclusteringGroupableType(GetAttributeType()));

			// Parcours des parties pour les indexer par leurs valeurs
			nkdParts.RemoveAll();
			defaultPart = NULL;
			part = headPart;
			while (part != NULL)
			{
				// Parcours des valeurs de la partie
				valueSet = part->GetValueSet();
				value = valueSet->GetHeadValue();
				while (value != NULL)
				{
					// Ajout de la partie avec la valeur pour cle
					nkdParts.SetAt(value->GetNumericKeyValue(), part);

					// Memorisation de la partie associe a la valeur speciale
					if (value->IsDefaultValue())
						defaultPart = part;

					// Valeur suivante
					valueSet->GetNextValue(value);
				}

				// Partie suivante
				part = part->nextPart;
			}
			assert(defaultPart != NULL or GetAttributeType() == KWType::VarPart);

			// Indexation des attributs internes dans le cas d'un attribut VarPart
			if (GetAttributeType() == KWType::VarPart)
			{
				for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber();
				     nInnerAttribute++)
				{
					innerAttribute = GetInnerAttributeAt(nInnerAttribute);

					// Indexation des valeurs des parties de l'attribut
					innerAttribute->BuildIndexingStructure();
				}
			}
		}

		// Memorisation du flag d'indexation
		bIsIndexed = true;
	}
}

void KWDGAttribute::DeleteIndexingStructure()
{
	// CH IV Begin
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	// CH IV End

	// Suppression de l'indexation si necessaire
	if (bIsIndexed)
	{
		// CH IV Begin
		assert(KWType::IsCoclusteringType(GetAttributeType()));
		if (GetAttributeType() == KWType::Continuous)
			oaIntervals.SetSize(0);
		else
		{
			assert(KWType::IsCoclusteringGroupableType(GetAttributeType()));

			// Nettoyage de l'indexation
			nkdParts.RemoveAll();
			defaultPart = NULL;

			// Nettoyage de l'indexation des attributs internes dans le cas d'un attribut VarPart
			if (GetAttributeType() == KWType::VarPart)
			{
				for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber();
				     nInnerAttribute++)
				{
					innerAttribute = GetInnerAttributeAt(nInnerAttribute);
					innerAttribute->DeleteIndexingStructure();
				}
			}
		}
		bIsIndexed = false;
	}
}

KWDGPart* KWDGAttribute::LookupContinuousPart(Continuous cValue)
{
	int nIndex;
	Continuous cUpperBound;

	require(IsIndexed());
	require(GetAttributeType() == KWType::Continuous);

	// Recherche de l'index de l'intervalle de discretization
	// Recherche sequentielle s'il y a peu d'intervalles
	if (oaIntervals.GetSize() <= 10)
	{
		for (nIndex = 0; nIndex < oaIntervals.GetSize(); nIndex++)
		{
			cUpperBound = cast(KWDGPart*, oaIntervals.GetAt(nIndex))->GetInterval()->GetUpperBound();
			if (cValue <= cUpperBound)
				return cast(KWDGPart*, oaIntervals.GetAt(nIndex));
		}
		assert(nIndex == oaIntervals.GetSize());
		assert(false);
		return NULL;
	}
	// Recherche dichotomique sinon
	else
	{
		int nLowerIndex;
		int nUpperIndex;

		// Initialisation des index extremites
		nLowerIndex = 0;
		nUpperIndex = oaIntervals.GetSize() - 2;

		// Recherche dichotomique de l'intervalle
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			cUpperBound = cast(KWDGPart*, oaIntervals.GetAt(nIndex))->GetInterval()->GetUpperBound();
			if (cValue <= cUpperBound)
				nUpperIndex = nIndex;
			else
				nLowerIndex = nIndex;

			// Modification du prochain intervalle teste
			nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// On compare par rapport aux deux index restant
		if (cValue <= cast(KWDGPart*, oaIntervals.GetAt(nLowerIndex))->GetInterval()->GetUpperBound())
			nIndex = nLowerIndex;
		else if (cValue > cast(KWDGPart*, oaIntervals.GetAt(nUpperIndex))->GetInterval()->GetUpperBound())
			nIndex = nUpperIndex + 1;
		else
			nIndex = nUpperIndex;

		// On retourne le resultat
		assert(nIndex == oaIntervals.GetSize() - 1 or
		       cValue <= cast(KWDGPart*, oaIntervals.GetAt(nIndex))->GetInterval()->GetUpperBound());
		assert(nIndex == 0 or
		       cValue > cast(KWDGPart*, oaIntervals.GetAt(nIndex - 1))->GetInterval()->GetUpperBound());
		return cast(KWDGPart*, oaIntervals.GetAt(nIndex));
	}
}

KWDGPart* KWDGAttribute::LookupSymbolPart(const Symbol& sValue)
{
	KWDGPart* part;

	require(IsIndexed());
	require(GetAttributeType() == KWType::Symbol);

	part = cast(KWDGPart*, nkdParts.Lookup(sValue.GetNumericKey()));
	if (part == NULL)
		part = defaultPart;
	ensure(part != NULL);
	return part;
}

KWDGPart* KWDGAttribute::LookupVarPart(KWDGPart* varPart)
{
	KWDGPart* part;

	require(IsIndexed());
	require(GetAttributeType() == KWType::VarPart);

	part = cast(KWDGPart*, nkdParts.Lookup((NUMERIC)varPart));

	ensure(part != NULL);
	return part;
}

KWDGPart* KWDGAttribute::LookupGroupablePart(const KWDGValue* value)
{
	KWDGPart* part;

	require(IsIndexed());
	require(KWType::IsCoclusteringGroupableType(GetAttributeType()));

	part = cast(KWDGPart*, nkdParts.Lookup(value->GetNumericKeyValue()));
	if (part == NULL)
		part = defaultPart;
	ensure(part != NULL);
	return part;
}

int KWDGAttribute::ComputeTotalPartFrequency() const
{
	int nTotalFrequency;
	KWDGPart* part;

	// Parcours des parties
	nTotalFrequency = 0;
	part = headPart;
	while (part != NULL)
	{
		nTotalFrequency += part->GetPartFrequency();
		part = part->nextPart;
	}
	return nTotalFrequency;
}

boolean KWDGAttribute::ContainsSubParts(const KWDGAttribute* otherAttribute) const
{
	boolean bOk;
	KWDGPart* part;
	KWDGPart* otherPart;
	KWDGValue* value;
	NumericKeyDictionary nkdOtherPartsPerValue;

	require(otherAttribute != NULL);
	require(GetAttributeName() == otherAttribute->GetAttributeName());
	require(GetAttributeType() == otherAttribute->GetAttributeType());
	require(GetAttributeType() != KWType::VarPart or GetInnerAttributes() == otherAttribute->GetInnerAttributes());

	// On doit avoir un nombre superieur de partie
	bOk = GetPartNumber() >= otherAttribute->GetPartNumber();

	// Test d'inclusion des parties
	if (bOk)
	{
		// Test d'inclusion des parties dans le cas numerique
		if (otherAttribute->GetAttributeType() == KWType::Continuous)
		{
			// Test des borne extremes
			bOk = bOk and GetHeadPart()->GetInterval()->GetLowerBound() ==
					  otherAttribute->GetHeadPart()->GetInterval()->GetLowerBound();
			bOk = bOk and GetTailPart()->GetInterval()->GetUpperBound() ==
					  otherAttribute->GetTailPart()->GetInterval()->GetUpperBound();

			// Parcours des intervalles pour verifier leur inclusion
			part = GetHeadPart();
			otherPart = otherAttribute->GetHeadPart();
			while (part != NULL)
			{
				// Test si l'intervalle est dans l'autre intervalle
				if (part->IsSubPart(otherPart))
					GetNextPart(part);
				// Sinon, on teste dans l'autre intervalle suivant
				else
				{
					otherAttribute->GetNextPart(otherPart);

					// Ok si on existence d'un intervalle suivant le contenant
					if (otherPart != NULL and part->IsSubPart(otherPart))
						GetNextPart(part);
					// Ko sinon
					else
					{
						bOk = false;
						break;
					}
				}
			}
		}
		// Test d'inclusion des parties dans le cas d'un attribut groupable
		else
		{
			assert(KWType::IsCoclusteringGroupableType(GetAttributeType()));

			// Indexation prealable des partie cibles
			otherPart = otherAttribute->GetHeadPart();
			while (otherPart != NULL)
			{
				// Parcours de valeurs de l'autre partie pour memoriser l'association
				value = otherPart->GetValueSet()->GetHeadValue();
				while (value != NULL)
				{
					nkdOtherPartsPerValue.SetAt(value->GetNumericKeyValue(), otherPart);
					otherPart->GetValueSet()->GetNextValue(value);
				}

				// Autre partie suivante
				otherAttribute->GetNextPart(otherPart);
			}

			// Parcours des parties pour verifier leur inclusion
			part = GetHeadPart();
			while (part != NULL)
			{
				// Recherche de l'autre partie sur la base de la premiere valeur de la partie en cours
				otherPart =
				    cast(KWDGPart*, nkdOtherPartsPerValue.Lookup(
							part->GetValueSet()->GetHeadValue()->GetNumericKeyValue()));

				// Pas d'inclusion si partie non trouvee ou non inclusante
				if (otherPart == NULL or not part->IsSubPart(otherPart))
				{
					bOk = false;
					break;
				}

				// Partie suivante
				GetNextPart(part);
			}
		}
	}
	return bOk;
}

boolean KWDGAttribute::Check() const
{
	boolean bOk = true;
	KWDGPart* part;
	int nPart;
	KWDGPart* searchedPart;
	KWDGValueSet* valueSet;
	KWDGValue* value;
	KWDGPart* usedVarPart;
	ObjectArray oaCheckIntervals;
	int nInterval;
	Continuous cPreviousUpperBound;
	NumericKeyDictionary nkdCheckParts;
	boolean bGarbagePartFound;
	boolean bDefaultValueFound;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	ALString sTmp;

	// Verifications de l'attribut, hors lien a la structure de grille
	if (nAttributeType == KWType::Unknown)
	{
		AddError("Missing type");
		bOk = false;
	}
	else if (not KWType::IsCoclusteringType(nAttributeType))
	{
		AddError("Type must be Numerical, Categorical or VarPart");
		bOk = false;
	}

	// Verification de l'existence d'un nom d'attribut
	if (sAttributeName == "")
	{
		AddError("Missing variable name");
		bOk = false;
	}

	// Verification du nombre de parties
	if (bOk and nPartNumber == 0)
	{
		AddError("No variable part is specified");
		bOk = false;
	}

	// Verification du nombre de valeurs initiales et granularisees
	// et de leur coherence avec nPartNumber
	if (nInitialValueNumber < 0)
	{
		AddError("Initial value number must be greater than 0");
		bOk = false;
	}
	else if (nGranularizedValueNumber < 0)
	{
		AddError("Granularized value number must be greater than 0");
		bOk = false;
	}
	// CH IV Begin
	else if (KWType::IsSimple(nAttributeType) and nInitialValueNumber < nGranularizedValueNumber)
	{
		AddError("Initial value number must be greater or equal than granularized value number");
		bOk = false;
	}
	// Verification uniquement si on a des infos sur la granularisation
	else if (KWType::IsSimple(nAttributeType) and nPartNumber > nGranularizedValueNumber and
		 nGranularizedValueNumber > 0)
	// CH IV End
	{
		AddError("Granularized value number must be greater or equal than part number");
		bOk = false;
	}

	// Verification des variables internes de cas cas VarPart
	if (bOk and nAttributeType == KWType::VarPart)
	{
		// Test de presence des specifications des variables internes
		if (bOk and innerAttributes == NULL)
		{
			AddError("Missing specification of inner variables for a variable of type VarPart");
			bOk = false;
		}

		// Test de validite des specifications des variables internes
		if (bOk and not innerAttributes->Check())
		{
			AddError("Wrong specification of inner variables for a variable of type VarPart");
			bOk = false;
		}
	}
	if (bOk and nAttributeType != KWType::VarPart and innerAttributes != NULL)
	{
		AddError("Inner variables should not be specified for variable that is not of type VarPart");
		bOk = false;
	}

	// Test de coherence entre variable interne et variable de grille
	if (bOk)
	{
		// Une variable interne doit avoir une variable de grille la contenant
		if (bOk and dataGrid == NULL and sOwnerAttributeName == "")
		{
			AddError("Inner variable should have an owner variable");
			bOk = false;
		}

		// Une variable de grille ne peut avoir de variable la contenant
		if (bOk and dataGrid != NULL and sOwnerAttributeName != "")
		{
			AddError("Data grid variable should not have an owner variable (" + sOwnerAttributeName + ")");
			bOk = false;
		}
	}

	// Verification des parties
	if (bOk)
	{
		// Parcours des parties a verifier
		bGarbagePartFound = false;
		part = headPart;
		while (part != NULL)
		{
			// Coherence avec les extremites de la liste
			assert(part->prevPart != NULL or headPart == part);
			assert(part->nextPart != NULL or tailPart == part);

			// Coherence de chainage
			assert(part->nextPart == NULL or part->nextPart->prevPart == part);

			// Test si on a trouve la partie de type Garbage
			if (part == garbagePart)
				bGarbagePartFound = true;

			// Verification locale de la partie
			bOk = bOk and part->Check();
			if (not bOk)
			{
				AddError("Invalid " + part->GetClassLabel() + " " + part->GetObjectLabel());
				bOk = false;
			}

			// Verification de la coherence de la partie dans l'attribut
			if (part->GetAttribute() != this)
			{
				AddError(part->GetClassLabel() + " " + part->GetObjectLabel() +
					 " not linked to a variable");
				bOk = false;
			}
			if (part->GetPartType() != nAttributeType)
			{
				AddError(part->GetClassLabel() + " " + part->GetObjectLabel() +
					 ": type of the part inconsistent with that of the variable");
				bOk = false;
			}

			// Passage a la partie suivant si pas d'erreur
			if (not bOk)
				break;
			part = part->nextPart;
		}

		// Test de la partie poubelle
		assert(garbagePart == NULL or GetAttributeType() != KWType::Continuous);
		if (garbagePart != NULL and not bGarbagePartFound)
		{
			AddError(part->GetClassLabel() + " " + part->GetObjectLabel() +
				 ": the garbage part in not found among the parts of the variable");
			bOk = false;
		}
	}

	// Si attribut numerique, indexation local des intervalles pour validation
	if (bOk and GetAttributeType() == KWType::Continuous)
	{
		// Ajout des parties numeriques dans le tableau des intervalles
		oaCheckIntervals.SetSize(nPartNumber);
		nPart = 0;
		part = cast(KWDGPart*, headPart);
		while (part != NULL)
		{
			oaCheckIntervals.SetAt(nPart, part);
			nPart++;
			part = part->nextPart;
		}

		// Tri des intervalles par borne inf
		oaCheckIntervals.SetCompareFunction(KWDGPartCompareValues);
		oaCheckIntervals.Sort();

		// Verification des intervalles
		cPreviousUpperBound = KWDGInterval::GetMinLowerBound();
		for (nInterval = 0; nInterval < oaCheckIntervals.GetSize(); nInterval++)
		{
			part = cast(KWDGPart*, oaCheckIntervals.GetAt(nInterval));

			// Borne inf du premier intervalle
			if (nInterval == 0 and part->GetInterval()->GetLowerBound() != KWDGInterval::GetMinLowerBound())
			{
				part->AddError("The lower bound of the first interval should be -inf or missing");
				bOk = false;
			}

			// Borne sup du dernier intervalle
			if (nInterval == oaCheckIntervals.GetSize() - 1 and
			    part->GetInterval()->GetUpperBound() != KWDGInterval::GetMaxUpperBound())
			{
				part->AddError("The upper bound of the last interval should be +inf");
				bOk = false;
			}

			// Coherence entre deux intervalles successifs
			if (nInterval > 0 and part->GetInterval()->GetLowerBound() != cPreviousUpperBound)
			{
				part->AddError(sTmp + "The lower bound of the interval differs " +
					       "from the upper bound of the preceding interval (" +
					       KWContinuous::ContinuousToString(cPreviousUpperBound) + ")");
				bOk = false;
			}

			// Memorisation de la borne sup de l'intervalle
			cPreviousUpperBound = part->GetInterval()->GetUpperBound();

			// Arret si erreurs
			if (not bOk)
				break;
		}
	}
	// Si attribut groupable, indexation locale des parties par les valeurs pour validation
	else if (bOk and KWType::IsCoclusteringGroupableType(GetAttributeType()))
	{
		// Parcours des parties pour les indexer par leurs valeurs
		nkdCheckParts.RemoveAll();
		bDefaultValueFound = false;
		part = headPart;
		while (part != NULL)
		{
			// Parcours des valeurs de la partie
			valueSet = part->GetValueSet();
			value = valueSet->GetHeadValue();
			while (value != NULL)
			{
				// Recherche si la valeur est deja enregistree
				searchedPart = cast(KWDGPart*, nkdCheckParts.Lookup(value->GetNumericKeyValue()));

				// Erreur si partie deja enregistree avec cette valeur
				if (searchedPart != NULL)
				{
					part->AddError(sTmp + value->GetClassLabel() + " " + value->GetObjectLabel() +
						       " already belongs to part " + part->GetObjectLabel());
					bOk = false;
					break;
				}
				// On continue si pas d'erreur
				else
				{
					// Ajout de la partie avec la valeur pour cle
					nkdCheckParts.SetAt(value->GetNumericKeyValue(), part);

					// Test si valeur speciale
					if (value->IsDefaultValue())
						bDefaultValueFound = true;

					// Valeur suivante
					valueSet->GetNextValue(value);
				}
			}

			// Partie suivante
			part = part->nextPart;
		}

		// Test si la valeur speciale est definie dans le groupage dans le cas d'un attribut Symbol
		if (bOk and GetAttributeType() == KWType::Symbol and not bDefaultValueFound)
		{
			AddError(sTmp + "Special grouping value " + Symbol::GetStarValue() +
				 " is not specified in any part");
			bOk = false;
		}

		// Tests specifiques dans le cas VarPart
		if (GetAttributeType() == KWType::VarPart)
		{
			// Verification du nombre total de partie de variables indexes dans les partie de la variable de type VarPart
			if (bOk and innerAttributes->ComputeTotalInnerAttributeVarParts() != nkdCheckParts.GetCount())
			{
				AddError(sTmp + "Number of variable parts used in groups (" +
					 IntToString(nkdCheckParts.GetCount()) +
					 ") is different from the total number of parts in the inner variables (" +
					 IntToString(innerAttributes->ComputeTotalInnerAttributeVarParts()) + ")");
				bOk = false;
			}

			// Parcours de VarPart des variables internes pour verifier qu'elles sont referencees dans les parties
			// de la variable de type VarPart
			if (bOk)
			{
				// Parcours des attributs internes
				for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber();
				     nInnerAttribute++)
				{
					innerAttribute = GetInnerAttributeAt(nInnerAttribute);

					// Parcours des partie de l'attribut interne
					usedVarPart = innerAttribute->GetHeadPart();
					while (usedVarPart != NULL)
					{
						// Recherche de la VarPart parmi les VarPart des parties de la variable de type VarPart
						searchedPart =
						    cast(KWDGPart*, nkdCheckParts.Lookup((NUMERIC)usedVarPart));

						// Erreur si VarPart non trouvee
						if (searchedPart == NULL)
						{
							searchedPart->AddError(sTmp + "Inner variable VarPart " +
									       usedVarPart->GetObjectLabel() +
									       " not found amond parts of variables");
							bOk = false;
							break;
						}

						// Partie suivante
						innerAttribute->GetNextPart(usedVarPart);
					}
				}
			}
		}
	}
	return bOk;
}

longint KWDGAttribute::GetUsedMemory() const
{
	longint lUsedMemory;

	// Memoire de base
	lUsedMemory = sizeof(KWDGAttribute);
	lUsedMemory += sAttributeName.GetLength();
	lUsedMemory++;

	// Prise en compte des parties et des valeurs
	if (headPart != NULL)
		lUsedMemory *= nPartNumber * headPart->GetUsedMemory();
	if (KWType::IsCoclusteringGroupableType(GetAttributeType()))
		lUsedMemory += nGranularizedValueNumber * headPart->GetValueSet()->GetHeadValue()->GetUsedMemory();

	// Prise en compte de la structure d'indexation
	lUsedMemory += oaIntervals.GetUsedMemory();
	lUsedMemory += nkdParts.GetUsedMemory();
	return lUsedMemory;
}

void KWDGAttribute::SortParts()
{
	ObjectArray oaParts;
	KWDGPart* part;

	// Tri des valeurs par effectif decroissant dans chaque partie d'attribut symbolique
	if (GetAttributeType() == KWType::Symbol)
	{
		part = GetHeadPart();
		while (part != NULL)
		{
			part->GetValueSet()->SortValueByDecreasingFrequencies();
			GetNextPart(part);
		}
	}
	// Tri par partie de variable (attribut, puis valeur de la parties) dans le cas d'un attribut VarPart
	else if (GetAttributeType() == KWType::VarPart)
	{
		part = GetHeadPart();
		while (part != NULL)
		{
			part->GetValueSet()->SortValues();
			GetNextPart(part);
		}
	}

	// Tri des parties par intervalle croissant pour les attribut continus et
	// ou par effectif decroissant pour les attributs symboliques ou de type varpart
	InternalSortParts(KWDGPartCompare);
}

boolean KWDGAttribute::ArePartsSorted() const
{
	ObjectArray oaParts;
	KWDGPart* part;
	KWDGPart* nextPart;
	boolean bIsSorted = true;
	int nCompare;

	// Verification du tri des valeurs dans chaque partie d'attribut symbolique
	if (GetAttributeType() == KWType::Symbol)
	{
		part = GetHeadPart();
		while (part != NULL)
		{
			bIsSorted = part->GetValueSet()->AreValuesSortedByDecreasingFrequencies();
			if (not bIsSorted)
				break;
			GetNextPart(part);
		}
	}
	// Verification du tri des valeurs dans chaque partie d'attribut VarPart
	else if (GetAttributeType() == KWType::VarPart)
	{
		part = GetHeadPart();
		while (part != NULL)
		{
			bIsSorted = part->GetValueSet()->AreValuesSorted();
			if (not bIsSorted)
				break;
			GetNextPart(part);
		}
	}

	// Verification du tri des parties si ok
	if (bIsSorted)
	{
		// Verification du tri des parties par intervalle croissant pour les attribut continus et
		// ou par effectif decroissant pour les attributs symboliques

		// Initialisation
		part = GetHeadPart();
		nextPart = GetHeadPart();
		GetNextPart(nextPart);

		// Parcours des parties
		while (nextPart != NULL)
		{
			// Comparaison des partie par intervalles, ou par effectif decroissant, puis valeurs Symbol ou VarPart)
			nCompare = part->ComparePart(nextPart);
			if (nCompare > 0)
			{
				bIsSorted = false;
				break;
			}

			// Paire suivante
			GetNextPart(part);
			GetNextPart(nextPart);
		}
	}
	return bIsSorted;
}

void KWDGAttribute::Write(ostream& ost) const
{
	// En tete de l'attribut
	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\n";

	// Affichage des caracteristiques du fourre-tout
	ost << "CatchAllNumber\t" << GetCatchAllValueNumber() << endl;

	//  Affichage des caracteristiques de la poubelle
	ost << "Garbage\tPossible\tExist\tSize\n";
	ost << "\t" << (GetInitialValueNumber() > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage()) << "\t"
	    << (GetGarbagePart() != NULL);
	if (GetGarbagePart() != NULL)
		ost << "\t" << cast(KWDGValueSet*, GetGarbagePart())->GetValueNumber() << "\n";
	else
		ost << "\n";
	// Parties de l'attribut
	if (GetPartNumber() > 0)
		WriteParts(ost);
}

void KWDGAttribute::WriteParts(ostream& ost) const
{
	KWDGPart* part;

	// Parties de l'attribut
	ost << "Parts"
	    << "\t" << GetPartNumber() << "\n";
	part = GetHeadPart();
	while (part != NULL)
	{
		// Cas de la partie poubelle
		if (part == garbagePart)
			ost << "\tGarbage:" << *part;
		// Sinon
		else
			ost << "\t" << *part;
		GetNextPart(part);
	}
}

const ALString KWDGAttribute::GetClassLabel() const
{
	return "Variable";
}

const ALString KWDGAttribute::GetObjectLabel() const
{
	return GetAttributeName();
}

KWDGAttribute* KWDGAttribute::CreateTestContinuousAttribute(Continuous cFirstBound, Continuous cIntervalWidth,
							    int nIntervalNumber)
{
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGInterval* interval;
	int nInterval;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(cIntervalWidth > 0);
	require(nIntervalNumber > 0);

	// Creation de l'attribut
	attribute = new KWDGAttribute;
	attribute->SetAttributeName("Var");
	attribute->SetAttributeType(KWType::Continuous);

	// Parametrage de l'attribut par des intervalles
	cLowerBound = KWDGInterval::GetMinLowerBound();
	cUpperBound = cFirstBound;
	for (nInterval = 0; nInterval < nIntervalNumber; nInterval++)
	{
		part = attribute->AddPart();
		interval = part->GetInterval();

		// Parametrage des bornes de l'intervalle
		interval->SetLowerBound(cLowerBound);
		if (nInterval == nIntervalNumber - 1)
			interval->SetUpperBound(KWDGInterval::GetMaxUpperBound());
		else
			interval->SetUpperBound(cUpperBound);

		// Preparation des bornes de l'intervalle suivant
		cLowerBound = cUpperBound;
		cUpperBound += cIntervalWidth;
	}

	ensure(attribute->Check());
	return attribute;
}

KWDGAttribute* KWDGAttribute::CreateTestSymbolAttribute(int nValueNumber, int nPartNumber)
{
	const ALString sValuePrefix = "V";
	KWDGAttribute* attribute;
	int nValue;
	KWDGPart* part;
	KWDGSymbolValueSet* symbolValueSet;
	KWDGValue* value;

	require(nValueNumber > 0);
	require(nPartNumber > 0);
	require(nValueNumber >= nPartNumber);

	// Creation de l'attribut
	attribute = new KWDGAttribute;
	attribute->SetAttributeName("Var");
	attribute->SetAttributeType(KWType::Symbol);

	// Creation des parties
	while (attribute->GetPartNumber() < nPartNumber)
		attribute->AddPart();

	// Ajout des valeurs dans les parties
	nValue = 0;
	part = attribute->GetHeadPart();
	while (nValue < nValueNumber)
	{
		// Acces en boucle au prochain part disponible
		attribute->GetNextPart(part);
		if (part == NULL)
			part = attribute->GetHeadPart();
		check(part);
		symbolValueSet = part->GetSymbolValueSet();

		// Ajout d'une valeur a la partie courant
		value = symbolValueSet->AddSymbolValue((Symbol)(sValuePrefix + IntToString(nValue)));
		value->SetValueFrequency(0);
		nValue++;
	}

	// Ajout de la valeur speciale
	attribute->GetNextPart(part);
	if (part == NULL)
		part = attribute->GetHeadPart();
	check(part);
	symbolValueSet = part->GetSymbolValueSet();
	value = symbolValueSet->AddSymbolValue(Symbol::GetStarValue());
	value->SetValueFrequency(0);

	ensure(attribute->Check());
	return attribute;
}

void KWDGAttribute::Test()
{
	boolean bTestContinuous = true;
	boolean bTestSymbol = true;
	const ALString sValuePrefix = "V";
	const int nIndexingTestNumber = 100;
	Continuous cValue;
	Symbol sValue;
	int nTest;
	KWDGAttribute* continuousAttribute;
	KWDGAttribute* symbolAttribute;
	KWDGPart* part;

	///////////////////////////////////////////////////////////
	// Test d'un attribut continu

	// Test uniquement si demande
	if (bTestContinuous)
	{
		// Creation de l'attribut
		cout << "Test of a numerical variable" << endl;
		continuousAttribute = CreateTestContinuousAttribute(1, 1, 5);
		continuousAttribute->Check();
		cout << *continuousAttribute << endl;

		// Indexation de l'attribut et pour recherche de parties correspondant a des valeurs
		cout << "Indexation and search of parts" << endl;
		continuousAttribute->BuildIndexingStructure();
		for (nTest = 0; nTest < nIndexingTestNumber; nTest++)
		{
			cValue = continuousAttribute->GetPartNumber() * (Continuous)RandomDouble();
			part = continuousAttribute->LookupContinuousPart(cValue);
			check(part);
			cout << cValue << "\t" << part->GetObjectLabel() << endl;
		}
		continuousAttribute->DeleteIndexingStructure();

		// Nettoyage
		delete continuousAttribute;
	}

	///////////////////////////////////////////////////////////
	// Test d'un attribut symbolique

	// Test uniquement si demande
	if (bTestSymbol)
	{
		// Creation de l'attribut
		cout << "Test of a categorical variable" << endl;
		symbolAttribute = CreateTestSymbolAttribute(13, 5);
		symbolAttribute->Check();
		cout << *symbolAttribute << endl;

		// Indexation de l'attribut et pour recherche de parties correspondant a des valeurs
		cout << "Indexation and search of parts" << endl;
		symbolAttribute->BuildIndexingStructure();
		for (nTest = 0; nTest < nIndexingTestNumber; nTest++)
		{
			sValue = (Symbol)(sValuePrefix + IntToString(RandomInt(symbolAttribute->GetPartNumber() * 3)));
			part = symbolAttribute->LookupSymbolPart(sValue);
			check(part);
			cout << sValue << "\t" << part->GetObjectLabel() << endl;
		}
		symbolAttribute->DeleteIndexingStructure();

		// Nettoyage
		delete symbolAttribute;
	}
}

void KWDGAttribute::InternalSortParts(CompareFunction fCompare)
{
	ObjectArray oaParts;
	int i;
	KWDGPart* part;

	require(fCompare != NULL);

	// Rangement des parties dans un tableau
	oaParts.SetSize(GetPartNumber());
	part = GetHeadPart();
	i = 0;
	while (part != NULL)
	{
		oaParts.SetAt(i, part);
		i++;
		GetNextPart(part);
	}

	// Tri des valeurs selon la fonction de tri
	oaParts.SetCompareFunction(fCompare);
	oaParts.Sort();

	// Rangement des parties dans la liste, selon l'ordre du tableau trie
	headPart = NULL;
	tailPart = NULL;
	for (i = 0; i < oaParts.GetSize(); i++)
	{
		part = cast(KWDGPart*, oaParts.GetAt(i));

		// Reinitialisation du chainage de la partie
		part->prevPart = NULL;
		part->nextPart = NULL;

		// Ajout de la partie en fin de liste
		if (headPart == NULL)
			headPart = part;
		if (tailPart != NULL)
		{
			tailPart->nextPart = part;
			part->prevPart = tailPart;
		}
		tailPart = part;
	}
}

KWDGPart* KWDGAttribute::NewPart() const
{
	return new KWDGPart;
}

boolean KWDGAttribute::GetEmulated() const
{
	return false;
}

void KWDGAttribute::InitializePartOwner(KWDGPart* part)
{
	require(part != NULL);
	require(part->GetAttribute() == NULL);
	part->attribute = this;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGPart

KWDGPart::KWDGPart()
{
	attribute = NULL;
	prevPart = NULL;
	nextPart = NULL;
	nPartFrequency = 0;
	headCell = NULL;
	tailCell = NULL;
	nCellNumber = 0;
	partValues = NULL;
}

KWDGPart::~KWDGPart()
{
	if (partValues != NULL)
		delete partValues;

	// Reinitialisation pour faciliter le debug
	debug(attribute = NULL);
	debug(prevPart = NULL);
	debug(nextPart = NULL);
	debug(nPartFrequency = 0);
	debug(headCell = NULL);
	debug(tailCell = NULL);
	debug(nCellNumber = 0);
	debug(partValues = NULL);
}

void KWDGPart::SetPartType(int nValue)
{
	require(GetPartType() == KWType::Unknown);
	// CH IV Begin
	require(KWType::IsCoclusteringType(nValue));

	// Creation de l'objet interval ou ensemble de valeurs selon le type
	if (nValue == KWType::Continuous)
		partValues = NewInterval();
	else if (nValue == KWType::Symbol)
		partValues = NewSymbolValueSet();
	else
		partValues = NewVarPartSet();
	ensure(GetPartType() != KWType::Unknown);
}

boolean KWDGPart::CheckCell(KWDGCell* cell) const
{
	boolean bOk;
	KWDGCell* currentCell;

	require(cell != NULL);
	require(cell->GetAttributeNumber() > GetAttributeIndex());

	// Test de coherence interne de la cellule
	bOk = cell->Check();

	// Test de coherence avec la partie
	if (bOk)
	{
		if (cell->GetPartAt(GetAttributeIndex()) != this)
		{
			AddError("Cell not referencing the part (" + cell->GetObjectLabel() + ")");
			bOk = false;
		}
	}

	// Test d'existence de la cellule dans la liste des cellules de la partie
	if (bOk)
	{
		bOk = false;
		currentCell = GetHeadCell();
		while (currentCell != NULL)
		{
			if (cell == currentCell)
			{
				bOk = true;
				break;
			}
			GetNextCell(currentCell);
		}
		if (not bOk)
			AddError("Cell not found in the part cells (" + cell->GetObjectLabel() + ")");
	}
	return bOk;
}

void KWDGPart::SetPartFrequency(int nValue)
{
	require(nValue >= 0);
	// CH IV Begin
	require(GetAttribute()->IsInnerAttribute() or GetEmulated() or nValue == ComputeCellsTotalFrequency());
	// CH IV End
	nPartFrequency = nValue;
}

boolean KWDGPart::IsSubPart(const KWDGPart* otherPart) const
{
	require(otherPart != NULL);
	require(GetPartType() != KWType::Unknown);
	require(otherPart->GetPartType() == GetPartType());
	require(otherPart->GetAttribute()->GetAttributeName() == GetAttribute()->GetAttributeName());

	return GetPartValues()->IsSubPart(otherPart->GetPartValues());
}

int KWDGPart::ComparePart(const KWDGPart* otherPart) const
{
	int nCompare;

	require(otherPart != NULL);
	require(GetPartType() == otherPart->GetPartType());

	if (GetPartType() == KWType::Continuous)
		nCompare = GetInterval()->ComparePartValues(otherPart->GetInterval());
	else
	{
		nCompare = -GetPartFrequency() + otherPart->GetPartFrequency();
		if (nCompare == 0)
			nCompare = GetValueSet()->ComparePartValues(otherPart->GetValueSet());
	}
	return nCompare;
}

int KWDGPart::ComparePartValues(const KWDGPart* otherPart) const
{
	int nCompare;

	require(otherPart != NULL);
	require(GetPartType() == otherPart->GetPartType());

	nCompare = GetPartValues()->ComparePartValues(otherPart->GetPartValues());
	return nCompare;
}

boolean KWDGPart::Check() const
{
	boolean bOk = true;
	ALString sTmp;
	KWDGCell* cell;
	KWDGCell* nextCell;
	KWDGCell* prevCell;
	int nTotalValueFrequency;

	// Test du type
	if (GetPartType() == KWType::Unknown)
	{
		AddError("Missing part type");
		bOk = false;
	}
	// Verification de l'intervalle
	else if (GetPartType() == KWType::Continuous)
		bOk = bOk and partValues->Check();
	// Verification de l'ensemble de valeurs dans le cas groupable
	else
	{
		assert(KWType::IsCoclusteringGroupableType(GetPartType()));

		bOk = bOk and partValues->Check();

		// Verification de la compatibilite entre l'effectif de la partie
		// et l'effectif cumule de ses valeurs
		// La verification ne se fait que si la partie a un effectif non nul,
		// ou si l'effectif cumule des valeurs est non nul.
		// Cela permet des verifications partielles en cours d'alimentation
		// de la grille (les parties et leurs valeurs sont creees avant
		// la creation des cellules).
		// Cela permet egalement de verifier la validite d'une grille
		// construite pour le deploiement de modele, qui n'a pas besoin
		// des effectifs par valeur.
		nTotalValueFrequency = GetValueSet()->ComputeTotalFrequency();
		if (bOk and GetPartFrequency() > 0 and nTotalValueFrequency > 0 and
		    GetPartFrequency() != nTotalValueFrequency)
		{
			assert(GetValueSet()->GetHeadValue() != NULL);
			AddError(sTmp + "Part frequency (" + IntToString(GetPartFrequency()) +
				 ") different from the cumulated frequency of its " +
				 GetValueSet()->GetHeadValue()->GetClassLabel() + "s (" +
				 IntToString(nTotalValueFrequency) + ")");
			bOk = false;
		}
	}

	// Test de coherence des cellules de la parties
	if (bOk)
	{
		cell = GetHeadCell();
		while (cell != NULL)
		{
			assert(cell->GetAttributeNumber() > GetAttributeIndex());
			assert(cell->Check());

			// Recherche des cellules suivants et precedants
			prevCell = cast(KWDGCell*, cell->oaPrevCells.GetAt(GetAttributeIndex()));
			nextCell = cast(KWDGCell*, cell->oaNextCells.GetAt(GetAttributeIndex()));

			// Coherence avec les extremites de la liste
			assert(cast(KWDGCell*, cell->oaPrevCells.GetAt(GetAttributeIndex())) != NULL or
			       cell == GetHeadCell());
			assert(cast(KWDGCell*, cell->oaNextCells.GetAt(GetAttributeIndex())) != NULL or
			       cell == GetTailCell());

			// Coherence de chainage
			if (nextCell != NULL and
			    cast(KWDGCell*, nextCell->oaPrevCells.GetAt(GetAttributeIndex())) != cell)
			{
				AddError("Cell (" + cell->GetObjectLabel() + ") has a next cell (" +
					 nextCell->GetObjectLabel() + ") whose previous cell (" +
					 cast(KWDGCell*, nextCell->oaPrevCells.GetAt(GetAttributeIndex()))
					     ->GetObjectLabel() +
					 ") is inconsistent");
				bOk = false;
			}
			if (prevCell != NULL and
			    cast(KWDGCell*, prevCell->oaNextCells.GetAt(GetAttributeIndex())) != cell)
			{
				AddError("Cell (" + cell->GetObjectLabel() + ") has a previous cell (" +
					 prevCell->GetObjectLabel() + ") whose next cell (" +
					 cast(KWDGCell*, prevCell->oaNextCells.GetAt(GetAttributeIndex()))
					     ->GetObjectLabel() +
					 ") is inconsistent");
				bOk = false;
			}

			// Test de coherence avec la partie
			if (cell->GetPartAt(GetAttributeIndex()) != this)
			{
				AddError("Cell not referencing the part (" + cell->GetObjectLabel() + ")");
				bOk = false;
			}

			// Passage a la cellule suivant si pas d'erreur
			if (not bOk)
				break;
			GetNextCell(cell);
		}
	}

	// Verification de l'effectif total de la partie
	// CH IV Begin
	if (bOk and (not GetAttribute()->IsInnerAttribute() and nPartFrequency != ComputeCellsTotalFrequency()))
	// CH IV End
	{
		AddError(sTmp + "Part frequency (" + IntToString(nPartFrequency) +
			 ") different from the cumulated frequency of its cells (" +
			 IntToString(ComputeCellsTotalFrequency()) + ")");
		bOk = false;
	}
	return bOk;
}

longint KWDGPart::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDGPart);
	if (partValues != NULL)
		lUsedMemory += partValues->GetUsedMemory();
	return lUsedMemory;
}

void KWDGPart::Write(ostream& ost) const
{
	// Identification de la partie
	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\t" << GetPartFrequency() << "\n";

	// Valeurs et cellules de la partie
	if (KWType::IsCoclusteringGroupableType(GetPartType()) and GetValueSet()->GetValueNumber() > 0)
		WriteValues(ost);
	if (GetCellNumber() > 0)
		WriteCells(ost);
}

void KWDGPart::WriteValues(ostream& ost) const
{
	// Des valeurs sont a afficher uniquement dans le cas groupable
	// (l'intervalle est le libelle de la partie dans le cas continu)
	if (KWType::IsCoclusteringGroupableType(GetPartType()))
		GetValueSet()->WriteValues(ost);
}

void KWDGPart::WriteCells(ostream& ost) const
{
	KWDGCell* cell;

	// Cellules de la partie
	ost << "Cells"
	    << "\t" << GetCellNumber() << "\n";
	cell = GetHeadCell();
	while (cell != NULL)
	{
		ost << "\t" << *cell;
		GetNextCell(cell);
	}
}

const ALString KWDGPart::GetVarPartLabel() const
{
	require(KWType::IsSimple(GetPartType()));
	require(GetAttribute() != NULL);
	require(GetAttribute()->IsInnerAttribute());

	return GetAttribute()->GetAttributeName() + " " + GetObjectLabel();
}

const ALString KWDGPart::GetClassLabel() const
{
	if (partValues == NULL)
		return "Part";
	else
		return partValues->GetClassLabel();
}

const ALString KWDGPart::GetObjectLabel() const
{
	if (partValues == NULL)
		return "";
	else
		return partValues->GetObjectLabel();
}

KWDGInterval* KWDGPart::NewInterval() const
{
	return new KWDGInterval;
}

KWDGSymbolValueSet* KWDGPart::NewSymbolValueSet() const
{
	return new KWDGSymbolValueSet;
}
// CH IV Begin
KWDGVarPartSet* KWDGPart::NewVarPartSet() const
{
	return new KWDGVarPartSet;
}
// CH IV End
int KWDGPart::ComputeCellsTotalFrequency() const
{
	int nCellsTotalFrequency;
	KWDGCell* cell;

	// Parcours des cellules de la partie pour en calculer l'effectif total
	nCellsTotalFrequency = 0;
	cell = GetHeadCell();
	while (cell != NULL)
	{
		nCellsTotalFrequency += cell->GetCellFrequency();
		GetNextCell(cell);
	}
	return nCellsTotalFrequency;
}

boolean KWDGPart::GetEmulated() const
{
	return false;
}

int KWDGPartCompare(const void* elem1, const void* elem2)
{
	KWDGPart* part1;
	KWDGPart* part2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la parties
	part1 = cast(KWDGPart*, *(Object**)elem1);
	part2 = cast(KWDGPart*, *(Object**)elem2);

	// Comparaison: on compare en priorite sur les bornes inf
	return part1->ComparePart(part2);
}

int KWDGPartCompareValues(const void* elem1, const void* elem2)
{
	KWDGPart* part1;
	KWDGPart* part2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la parties
	part1 = cast(KWDGPart*, *(Object**)elem1);
	part2 = cast(KWDGPart*, *(Object**)elem2);

	// Comparaison: on compare en priorite sur les bornes inf
	return part1->ComparePartValues(part2);
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGInterval

boolean KWDGInterval::IsSubPart(const KWDGPartValues* otherPartValues) const
{
	KWDGInterval* otherInterval;

	require(otherPartValues != NULL);
	require(otherPartValues->GetValueType() == GetValueType());

	// Comparaison des bornes
	otherInterval = cast(KWDGInterval*, otherPartValues);
	return GetLowerBound() >= otherInterval->GetLowerBound() and GetUpperBound() <= otherInterval->GetUpperBound();
}

void KWDGInterval::Import(KWDGPartValues* sourcePartValues)
{
	KWDGInterval* sourceInterval;

	require(Check());
	require(sourcePartValues->Check());
	require(sourcePartValues != NULL);
	require(sourcePartValues->GetValueType() == GetValueType());

	// Transfert d'une borne pour fusionner les intervalles
	sourceInterval = cast(KWDGInterval*, sourcePartValues);
	assert(sourceInterval->GetUpperBound() == GetLowerBound() or
	       GetUpperBound() == sourceInterval->GetLowerBound());
	if (sourceInterval->GetUpperBound() == GetLowerBound())
		SetLowerBound(sourceInterval->GetLowerBound());
	else
		SetUpperBound(sourceInterval->GetUpperBound());

	// Reinitialisation de l'intervalle source
	sourceInterval->SetLowerBound(0);
	sourceInterval->SetUpperBound(0);
	ensure(Check());
}

void KWDGInterval::UpgradeFrom(const KWDGPartValues* sourcePartValues)
{
	KWDGInterval* sourceInterval;

	require(Check());
	require(sourcePartValues->Check());
	require(sourcePartValues != NULL);
	require(sourcePartValues->GetValueType() == GetValueType());

	// Transfert d'une borne pour fusionner les intervalles
	sourceInterval = cast(KWDGInterval*, sourcePartValues);
	assert(sourceInterval->GetUpperBound() == GetLowerBound() or
	       GetUpperBound() == sourceInterval->GetLowerBound());
	if (sourceInterval->GetUpperBound() == GetLowerBound())
		SetLowerBound(sourceInterval->GetLowerBound());
	else
		SetUpperBound(sourceInterval->GetUpperBound());
	ensure(Check());
}

void KWDGInterval::CopyFrom(const KWDGPartValues* sourcePartValues)
{
	KWDGInterval* sourceInterval;

	require(sourcePartValues != NULL);
	require(sourcePartValues->GetValueType() == GetValueType());

	// Copie des bornes
	sourceInterval = cast(KWDGInterval*, sourcePartValues);
	cLowerBound = sourceInterval->cLowerBound;
	cUpperBound = sourceInterval->cUpperBound;
}

int KWDGInterval::ComparePartValues(const KWDGPartValues* otherPartValues) const
{
	int nCompare;
	KWDGInterval* otherInterval;

	require(otherPartValues != NULL);
	require(otherPartValues->GetValueType() == GetValueType());

	// Comparaison: on compare en priorite sur les bornes inf
	otherInterval = cast(KWDGInterval*, otherPartValues);
	nCompare = KWContinuous::Compare(GetLowerBound(), otherInterval->GetLowerBound());
	if (nCompare == 0)
		nCompare = KWContinuous::Compare(GetUpperBound(), otherInterval->GetUpperBound());
	return nCompare;
}

boolean KWDGInterval::Check() const
{
	boolean bOk = true;

	// Test des bornes: cas missing
	if (cUpperBound == KWContinuous::GetMissingValue())
	{
		if (cLowerBound != KWContinuous::GetMissingValue())
		{
			AddError("The lower and upper bounds must be equal for the missing value interval");
			bOk = false;
		}
	}
	// Cas standard (tolerance si egalite des bornes, possible dans les limites de precision numerique)
	else if (cLowerBound > cUpperBound)
	{
		AddError("The lower bound must be less than the upper bound");
		bOk = false;
	}
	return bOk;
}

void KWDGInterval::Write(ostream& ost) const
{
	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\n";
}

longint KWDGInterval::GetUsedMemory() const
{
	return sizeof(KWDGInterval);
}

const ALString KWDGInterval::GetClassLabel() const
{
	return "Interval";
}

const ALString KWDGInterval::GetObjectLabel() const
{
	ALString sLabel;

	// Cas particulier d'un intervalle reduit a la valeur manquante
	if (cUpperBound == KWContinuous::GetMissingValue())
		sLabel = "Missing";
	// Cas standard
	else
	{
		if (cLowerBound == GetMinLowerBound())
			sLabel = "]-inf;";
		else
			sLabel = sLabel + "]" + KWContinuous::ContinuousToString(cLowerBound) + ";";
		if (cUpperBound == GetMaxUpperBound())
			sLabel += "+inf[";
		else
			sLabel = sLabel + KWContinuous::ContinuousToString(cUpperBound) + "]";
	}
	return sLabel;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGValueSet

void KWDGValueSet::DeleteValue(KWDGValue* value)
{
	require(value != NULL);

	// Supression de la liste des values
	if (value->prevValue != NULL)
		value->prevValue->nextValue = value->nextValue;
	if (value->nextValue != NULL)
		value->nextValue->prevValue = value->prevValue;
	if (headValue == value)
		headValue = value->nextValue;
	if (tailValue == value)
		tailValue = value->prevValue;

	// Partie "standard" si la valeur detruite est la valeur par defaut
	if (value->IsDefaultValue())
		bIsDefaultPart = false;
	// Decrementation du nombre de valeurs sinon
	else
		nValueNumber--;

	// Destruction de la valeur
	delete value;
}

void KWDGValueSet::DeleteAllValues()
{
	KWDGValue* value;
	KWDGValue* valueToDelete;

	// Destruction des valeurs
	value = headValue;
	while (value != NULL)
	{
		valueToDelete = value;
		value = value->nextValue;
		delete valueToDelete;
	}

	// Reinitialisation de la gestion de la liste des valeurs
	headValue = NULL;
	tailValue = NULL;
	nValueNumber = 0;
	bIsDefaultPart = false;
}

boolean KWDGValueSet::CheckValue(KWDGValue* value) const
{
	boolean bOk;
	KWDGValue* currentValue;

	require(value != NULL);

	// Test si la valeur fait partie de la partie
	bOk = false;
	currentValue = headValue;
	while (currentValue != NULL)
	{
		if (value == currentValue)
		{
			bOk = true;
			break;
		}
		currentValue = currentValue->nextValue;
	}
	return bOk;
}

void KWDGValueSet::ExportValues(ObjectArray* oaValues) const
{
	KWDGValue* value;

	require(oaValues != NULL);
	require(oaValues->GetSize() == 0);

	// Ajout des parties dans le tableau
	value = headValue;
	while (value != NULL)
	{
		oaValues->Add(value);
		value = value->nextValue;
	}
}

int KWDGValueSet::ComputeTotalFrequency() const
{
	int nTotalFrequency;
	KWDGValue* value;

	// Parcours des valeurs de la partie pour calcul l'effectif cumule
	nTotalFrequency = 0;
	value = GetHeadValue();
	while (value != NULL)
	{
		nTotalFrequency += value->GetValueFrequency();
		GetNextValue(value);
	}
	return nTotalFrequency;
}

void KWDGValueSet::SortValues()
{
	InternalSortValues(KWDGValueCompareValue);
}

void KWDGValueSet::SortValueByDecreasingFrequencies()
{
	InternalSortValues(KWDGValueCompareFrequency);
}

void KWDGValueSet::SortValuesByDecreasingTypicalities()
{
	InternalSortValues(KWDGValueCompareTypicality);
}

boolean KWDGValueSet::AreValuesSorted() const
{
	return InternalAreValuesSorted(KWDGValueCompareValue);
}

boolean KWDGValueSet::AreValuesSortedByDecreasingFrequencies() const
{
	return InternalAreValuesSorted(KWDGValueCompareFrequency);
}

boolean KWDGValueSet::AreValuesSortedByDecreasingTypicalities() const
{
	return InternalAreValuesSorted(KWDGValueCompareTypicality);
}

boolean KWDGValueSet::IsSubPart(const KWDGPartValues* otherPartValues) const
{
	boolean bOk = true;
	KWDGValueSet* otherValueSet;
	NumericKeyDictionary nkdOtherValues;
	KWDGValue* value;

	require(otherPartValues != NULL);
	require(otherPartValues->GetValueType() == GetValueType());

	// Acces a l'autre groupe de valeurs
	otherValueSet = cast(KWDGValueSet*, otherPartValues);

	// On doit avoir moins de valeurs
	if (GetValueNumber() > otherValueSet->GetValueNumber())
		bOk = false;
	// Sinon, on teste effectivement l'inclusion
	else
	{
		// Memorisation prealable des valeurs de l'autre ensemble de valeur
		value = otherValueSet->GetHeadValue();
		while (value != NULL)
		{
			nkdOtherValues.SetAt(value->GetNumericKeyValue(), value);
			otherValueSet->GetNextValue(value);
		}

		// Parcours des valeurs pour verifie s'i elles existe dans l'autre ensemble de valeur
		value = GetHeadValue();
		while (value != NULL)
		{
			if (nkdOtherValues.Lookup(value->GetNumericKeyValue()) == NULL)
			{
				bOk = false;
				break;
			}
			GetNextValue(value);
		}
	}
	return bOk;
}

void KWDGValueSet::Import(KWDGPartValues* sourcePartValues)
{
	KWDGValueSet* sourceValueSet;

	require(sourcePartValues != NULL);
	require(sourcePartValues->GetValueType() == GetValueType());

	// Acces au groupe de valeurs source
	sourceValueSet = cast(KWDGValueSet*, sourcePartValues);

	// Transfert des valeurs de la premiere partie en tete du second
	sourceValueSet->tailValue->nextValue = headValue;
	headValue->prevValue = sourceValueSet->tailValue;
	headValue = sourceValueSet->headValue;
	nValueNumber += sourceValueSet->nValueNumber;
	bIsDefaultPart = bIsDefaultPart or sourceValueSet->IsDefaultPart();

	// On dereference les valeurs de la premiere partie
	sourceValueSet->headValue = NULL;
	sourceValueSet->tailValue = NULL;
	sourceValueSet->nValueNumber = 0;
	sourceValueSet->bIsDefaultPart = false;
	ensure(Check());
}

void KWDGValueSet::UpgradeFrom(const KWDGPartValues* sourcePartValues)
{
	KWDGValueSet* sourceValueSet;
	KWDGValue* value;
	KWDGValue* valueUpgrade;
	int nOldValueNumber;

	require(sourcePartValues != NULL);
	require(sourcePartValues->GetValueType() == GetValueType());

	// Acces au groupe de valeurs source
	sourceValueSet = cast(KWDGValueSet*, sourcePartValues);

	// Pour garantir la valeur correcte de nValueNumber si le sourceValueSet a ete compresse avant la copie
	nOldValueNumber = nValueNumber;

	// Recopie de la liste de valeurs source
	value = sourceValueSet->GetHeadValue();
	while (value != NULL)
	{
		valueUpgrade = AddValueCopy(value);
		valueUpgrade->SetValueFrequency(value->GetValueFrequency());
		sourceValueSet->GetNextValue(value);
	}
	// Pour garantir la valeur correcte de nValueNumber si le sourceValueSet a ete compresse avant la copie
	nValueNumber = nOldValueNumber + sourceValueSet->nValueNumber;
}

void KWDGValueSet::CopyFrom(const KWDGPartValues* sourcePartValues)
{
	KWDGValueSet* sourceValueSet;
	KWDGValue* value;
	KWDGValue* valueCopy;

	require(sourcePartValues != NULL);
	require(sourcePartValues->GetValueType() == GetValueType());

	// Acces au groupe de valeurs source
	sourceValueSet = cast(KWDGValueSet*, sourcePartValues);

	// Nettoyage des valeurs actuelles
	DeleteAllValues();

	// Recopie de la liste de valeurs source
	value = sourceValueSet->GetHeadValue();
	while (value != NULL)
	{
		valueCopy = AddValueCopy(value);
		valueCopy->SetValueFrequency(value->GetValueFrequency());
		sourceValueSet->GetNextValue(value);
	}
	bIsDefaultPart = sourceValueSet->IsDefaultPart();

	// Pour garantir la valeur correcte de nValueNumber si le sourceValueSet a ete compresse avant la copie
	nValueNumber = sourceValueSet->nValueNumber;
}

int KWDGValueSet::ComparePartValues(const KWDGPartValues* otherPartValues) const
{
	KWDGValueSet* otherValueSet;

	require(otherPartValues != NULL);
	require(otherPartValues->GetValueType() == GetValueType());

	// Comparaison de la premiere valeur de la partie
	otherValueSet = cast(KWDGValueSet*, otherPartValues);
	if (GetHeadValue() == NULL)
	{
		if (otherValueSet->GetHeadValue() == NULL)
			return 0;
		else
			return -1;
	}
	else if (otherValueSet->GetHeadValue() == NULL)
		return 1;
	else
		return GetHeadValue()->CompareValue(otherValueSet->GetHeadValue());
}

boolean KWDGValueSet::Check() const
{
	boolean bOk = true;
	boolean bDefaultValuePresent;
	NumericKeyDictionary nkdCheckValues;
	KWDGValue* value;
	boolean bCheckFrequencies;
	ALString sTmp;

	// Test d'existence d'au moins une valeur
	if (GetValueNumber() == 0)
	{
		AddError("No value specified");
		bOk = false;
	}

	// Test des valeurs de la partie
	if (bOk)
	{
		// On ne verifie les effectifs que si au moins un est specifie et si on n'est pas le groupe par defaut
		// En effet, le groupe par defaut pouvant etre compresse, il peut y a voir quelques incoherences
		// sur les effectifs des valeurs des valeur du groupe
		bCheckFrequencies = not bIsDefaultPart and ComputeTotalFrequency() > 0;

		// Parcours des valeurs de la partie
		bDefaultValuePresent = false;
		value = GetHeadValue();
		while (value != NULL)
		{
			// Detection de la star value
			if (value->IsDefaultValue())
				bDefaultValuePresent = true;

			// Erreur si partie deja enregistree avec cette valeur
			if (nkdCheckValues.Lookup(value->GetNumericKeyValue()) != NULL)
			{
				AddError(sTmp + value->GetClassLabel() + " " + value->GetObjectLabel() +
					 " already exists in the part");
				bOk = false;
				break;
			}
			// Erreur si effectif a 0 pour une valeur qui n'est pas la valeur par defaut
			else if (bCheckFrequencies and not value->IsDefaultValue() and value->GetValueFrequency() == 0)
			{
				AddError(sTmp + value->GetClassLabel() + " " + value->GetObjectLabel() +
					 " should have a non-zero frequency");
				bOk = false;
				break;
			}
			// Erreur si typicalite incorrecte
			else if (value->GetTypicality() < 0 or value->GetTypicality() > 1)
			{
				AddError(sTmp + value->GetClassLabel() + " " + value->GetObjectLabel() +
					 " should have a typicality (" + DoubleToString(value->GetTypicality()) +
					 ") between 0 and 1");
				bOk = false;
			}
			// On continue si pas d'erreur
			else
			{
				// Ajout de la partie avec la valeur pour cle
				nkdCheckValues.SetAt(value->GetNumericKeyValue(), value);

				// Valeur suivante
				GetNextValue(value);
			}
		}

		// Test d'integrite sur la valeur par defaut
		if (bDefaultValuePresent and not bIsDefaultPart)
		{
			AddError(sTmp + "Special default value is used used in a standard part");
			bOk = false;
		}
		else if (not bDefaultValuePresent and bIsDefaultPart)
		{
			AddError(sTmp + "Special default value is missing in default part");
			bOk = false;
		}
	}
	return bOk;
}

void KWDGValueSet::Write(ostream& ost) const
{
	// Identification
	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\n";

	// Affichage des valeurs
	WriteValues(ost);
}

void KWDGValueSet::WriteValues(ostream& ost) const
{
	KWDGValue* value;

	// Affichage des valeurs
	cout << "Values"
	     << "\t" << GetValueNumber() << "\n";
	value = GetHeadValue();
	while (value != NULL)
	{
		ost << "\t" << value->GetObjectLabel() << "\t" << value->GetValueFrequency() << "\n";
		GetNextValue(value);
	}
}

longint KWDGValueSet::GetUsedMemory() const
{
	return sizeof(KWDGValueSet);
}

const ALString KWDGValueSet::GetClassLabel() const
{
	return "Value set";
}

const ALString KWDGValueSet::GetObjectLabel() const
{
	ALString sLabel;
	int nValue;
	KWDGValue* dgValue;

	// Libelle base sur l'ensemble des valeurs
	sLabel = "{";
	nValue = 0;
	dgValue = GetHeadValue();
	while (dgValue != NULL)
	{
		// On n'utilise la valeur par defaut pour fabriquer le libelle
		if (not dgValue->IsDefaultValue())
		{
			// Prise en compte si moins de trois valeurs
			if (nValue < 3)
			{
				if (nValue > 0)
					sLabel += ", ";
				sLabel += dgValue->GetExternalValueLabel();
				nValue++;
			}
			// Arret si au moins quatre valeurs
			else
			{
				sLabel += ", ...";
				break;
			}
		}
		GetNextValue(dgValue);
	}
	sLabel += "}";
	return sLabel;
}

void KWDGValueSet::SetValueNumber(int nValue)
{
	require(nValue >= 0);
	nValueNumber = nValue;
}

void KWDGValueSet::AddTailValue(KWDGValue* value)
{
	require(value != NULL);
	require(value->GetValueType() == GetValueType());

	// Ajout en fin de la liste des valeurs
	if (headValue == NULL)
		headValue = value;
	if (tailValue != NULL)
	{
		tailValue->nextValue = value;
		value->prevValue = tailValue;
	}
	tailValue = value;

	// Partie par defaut si la valeur est la valeur par defaut
	if (value->IsDefaultValue())
		bIsDefaultPart = true;
	// Incrementation du nombre de valeurs sinon
	else
		nValueNumber++;
}

void KWDGValueSet::InternalSortValues(CompareFunction fCompare)
{
	ObjectArray oaValues;
	int i;
	KWDGValue* value;
	KWDGValue* defaultValue;

	require(fCompare != NULL);

	// Rangement des valeurs dans un tableau (hors valeur speciale)
	// Pour pouvoir traiter les valueSet qui ont ete compressees, la taille est
	// deduite du parcours des valeurs
	defaultValue = NULL;
	value = GetHeadValue();
	while (value != NULL)
	{
		if (not value->IsDefaultValue())
			oaValues.Add(value);
		else
			defaultValue = value;
		GetNextValue(value);
	}

	// Tri des valeurs selon la fonction de tri
	oaValues.SetCompareFunction(fCompare);
	oaValues.Sort();

	// Ajout de l'eventuelle valeu speciale en fin de tableau
	if (defaultValue != NULL)
		oaValues.Add(defaultValue);

	// Rangement des valeurs dans la liste, selon l'ordre du tableau trie
	headValue = NULL;
	tailValue = NULL;
	for (i = 0; i < oaValues.GetSize(); i++)
	{
		value = cast(KWDGValue*, oaValues.GetAt(i));

		// Reinitialisation du chainage de la partie
		value->prevValue = NULL;
		value->nextValue = NULL;

		// Ajout de la valeur en fin de liste
		if (headValue == NULL)
			headValue = value;
		if (tailValue != NULL)
		{
			tailValue->nextValue = value;
			value->prevValue = tailValue;
		}
		tailValue = value;
	}
}

boolean KWDGValueSet::InternalAreValuesSorted(CompareFunction fCompare) const
{
	KWDGValue* value1;
	KWDGValue* value2;
	int nCompare;
	boolean bIsSorted = true;

	value1 = GetHeadValue();
	value2 = GetHeadValue();
	GetNextValue(value2);

	// Parcours des valeurs
	while (value2 != NULL)
	{
		// La valeur manquante doit etre en dernier
		if (value1->IsDefaultValue())
		{
			bIsSorted = false;
			break;
		}
		if (value2->IsDefaultValue())
		{
			if (value2 != GetTailValue())
				bIsSorted = false;
			break;
		}

		// Comparaison de la paire courante
		nCompare = fCompare(&value1, &value2);

		if (nCompare > 0)
		{
			bIsSorted = false;
			break;
		}
		GetNextValue(value1);
		GetNextValue(value2);
	}
	return bIsSorted;
}

boolean KWDGValueSet::GetEmulated() const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSymbolValueSet

void KWDGSymbolValueSet::CompressValueSet()
{
	int nCurrentValueNumber;
	int nCurrentPartFrequency;
	KWDGValue* value;

	require(bIsDefaultPart);

	// Memorisation des statistique
	nCurrentValueNumber = nValueNumber;
	nCurrentPartFrequency = ComputeTotalFrequency();

	// Remplacement de l'ensemble des valeurs par une seule valeur
	DeleteAllValues();
	value = AddSymbolValue(Symbol::GetStarValue());
	value->SetValueFrequency(nCurrentPartFrequency);
	nValueNumber = nCurrentValueNumber;
}

KWDGValueSet* KWDGSymbolValueSet::ConvertToCleanedValueSet()
{
	int nCurrentPartFrequency;
	KWDGValue* value;
	KWDGValueSet* valueSet;

	require(bIsDefaultPart);

	// Memorisation des statistique
	nCurrentPartFrequency = ComputeTotalFrequency();
	valueSet = Clone();

	// Remplacement de l'ensemble des valeurs par une seule valeur + modalite StarValue
	DeleteAllValues();
	value = AddSymbolValue(valueSet->GetHeadValue()->GetSymbolValue());
	value->SetValueFrequency(valueSet->GetHeadValue()->GetValueFrequency());
	value = AddSymbolValue(Symbol::GetStarValue());
	value->SetValueFrequency(nCurrentPartFrequency - valueSet->GetHeadValue()->GetValueFrequency());

	// Suppression dans le fourre tout des valeurs conservees dans la partie granularisee
	// (pour eviter la redondance des modalites)
	valueSet->DeleteValue(valueSet->GetHeadValue());
	valueSet->DeleteValue(valueSet->GetTailValue());

	return valueSet;
}

KWDGValueSet* KWDGSymbolValueSet::ComputeCleanedValueSet() const
{
	KWDGSymbolValueSet* symbolValueSet;
	KWDGValue* value;

	// Creation d'un nouveau ValueSet
	symbolValueSet = cast(KWDGSymbolValueSet*, Create());

	// Remplacement de l'ensemble des valeurs par une seule valeur + modalite StarValue
	value = symbolValueSet->AddSymbolValue(GetHeadValue()->GetSymbolValue());
	value->SetValueFrequency(GetHeadValue()->GetValueFrequency());
	value = symbolValueSet->AddSymbolValue(Symbol::GetStarValue());
	value->SetValueFrequency(ComputeTotalFrequency() - GetHeadValue()->GetValueFrequency());

	return symbolValueSet;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGValue

int KWDGValueCompareValue(const void* elem1, const void* elem2)
{
	KWDGValue* value1;
	KWDGValue* value2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la valeur
	value1 = cast(KWDGValue*, *(Object**)elem1);
	value2 = cast(KWDGValue*, *(Object**)elem2);

	// Comparaison
	return value1->CompareValue(value2);
}

int KWDGValueCompareFrequency(const void* elem1, const void* elem2)
{
	KWDGValue* value1;
	KWDGValue* value2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la valeur
	value1 = cast(KWDGValue*, *(Object**)elem1);
	value2 = cast(KWDGValue*, *(Object**)elem2);

	// Comparaison
	return value1->CompareFrequency(value2);
}

int KWDGValueCompareTypicality(const void* elem1, const void* elem2)
{
	KWDGValue* value1;
	KWDGValue* value2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces a la valeur
	value1 = cast(KWDGValue*, *(Object**)elem1);
	value2 = cast(KWDGValue*, *(Object**)elem2);

	// Comparaison
	return value1->CompareTypicality(value2);
}

int KWSortableObjectCompareValue(const void* elem1, const void* elem2)
{
	int nCompare;
	KWDGValue* value1;
	KWDGValue* value2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux parties de variable
	value1 = cast(KWDGValue*, cast(KWSortableObject*, *(Object**)elem1)->GetSortValue());
	value2 = cast(KWDGValue*, cast(KWSortableObject*, *(Object**)elem2)->GetSortValue());

	// Comparaison sur la valeur
	nCompare = value1->CompareValue(value2);

	// Comparaison sur l'index si egal
	if (nCompare == 0)
		nCompare = cast(KWSortableObject*, *(Object**)elem1)->GetIndex() -
			   cast(KWSortableObject*, *(Object**)elem2)->GetIndex();
	return nCompare;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSymbolValue

int KWDGSymbolValue::CompareTypicality(const KWDGValue* otherValue) const
{
	int nCompare;

	require(otherValue != NULL);

	// Comparaison selon la precison du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(GetTypicality(), otherValue->GetTypicality());

	// Comparaison par effectif decroissaqnt si egalite
	if (nCompare == 0)
		nCompare = CompareFrequency(otherValue);
	return nCompare;
}

void KWDGSymbolValue::Write(ostream& ost) const
{
	ost << sSymbolValue << "\t" << nValueFrequency;
}

const ALString KWDGSymbolValue::GetExternalValueLabel() const
{
	ALString sResult;
	int nLength;
	char c;
	int i;

	// On prend la valeur telle quelle
	sResult = sSymbolValue;

	// On la met entre quotes si elle contient un caractere special utilise par ObjectLabel
	if (sResult.FindOneOf("'{},") != -1)
	{
		sResult = '\'';
		nLength = sSymbolValue.GetLength();
		for (i = 0; i < nLength; i++)
		{
			c = sSymbolValue.GetAt(i);
			if (c == '\'')
				sResult += '\'';
			sResult += c;
		}
		sResult += '\'';
	}
	return sResult;
}

longint KWDGSymbolValue::GetUsedMemory() const
{
	return sizeof(KWDGSymbolValue);
}

const ALString KWDGSymbolValue::GetClassLabel() const
{
	return "value";
}

const ALString KWDGSymbolValue::GetObjectLabel() const
{
	return sSymbolValue.GetValue();
}

//////////////////////////////////////////////////////////////////////////////
// CH IV Begin

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGVarPartValue

int KWDGVarPartValue::CompareValue(const KWDGValue* otherValue) const
{
	int nCompare;

	require(otherValue != NULL);

	// Comparaison des noms d'attributs des parties de variable
	nCompare = GetVarPart()->GetAttribute()->GetAttributeName().Compare(
	    otherValue->GetVarPart()->GetAttribute()->GetAttributeName());

	// Si egalite, comparaison sur les valeurs de la partie (intervalle, ou premiere valeur du groupe)
	if (nCompare == 0)
		nCompare = GetVarPart()->ComparePartValues(otherValue->GetVarPart());
	return nCompare;
}

int KWDGVarPartValue::CompareTypicality(const KWDGValue* otherValue) const
{
	int nCompare;

	require(otherValue != NULL);

	// Comparaison selon la precison du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(GetTypicality(), otherValue->GetTypicality());

	// Comparaison par valeur si egalite
	if (nCompare == 0)
		nCompare = CompareValue(otherValue);
	return nCompare;
}

void KWDGVarPartValue::Write(ostream& ost) const
{
	ost << *varPart << "\t" << GetValueFrequency();
}

const ALString KWDGVarPartValue::GetExternalValueLabel() const
{
	return GetObjectLabel();
}

longint KWDGVarPartValue::GetUsedMemory() const
{
	return sizeof(KWDGVarPartValue);
}

const ALString KWDGVarPartValue::GetClassLabel() const
{
	return "VarPart";
}

const ALString KWDGVarPartValue::GetObjectLabel() const
{
	return varPart->GetVarPartLabel();
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGInnerAttributes

KWDGAttribute* KWDGInnerAttributes::GetInnerAttributeAt(int nAttributeIndex) const
{
	require(0 <= nAttributeIndex and nAttributeIndex < oaInnerAttributes.GetSize());

	return cast(KWDGAttribute*, oaInnerAttributes.GetAt(nAttributeIndex));
}

KWDGAttribute* KWDGInnerAttributes::LookupInnerAttribute(const ALString& sAttributeName)
{
	return cast(KWDGAttribute*, odInnerAttributes.Lookup(sAttributeName));
}

void KWDGInnerAttributes::AddInnerAttribute(KWDGAttribute* innerAttribute)
{
	require(innerAttribute != NULL);
	require(innerAttribute->GetAttributeName() != "");
	require(odInnerAttributes.Lookup(innerAttribute->GetAttributeName()) == NULL);
	require(KWType::IsSimple(innerAttribute->GetAttributeType()));
	require(odInnerAttributes.GetCount() == oaInnerAttributes.GetSize());

	// Ajout de l'attribut dans le tableau des attributs internes
	oaInnerAttributes.Add(innerAttribute);

	// Indexation dans le dictionnaire
	odInnerAttributes.SetAt(innerAttribute->GetAttributeName(), innerAttribute);
	ensure(odInnerAttributes.GetCount() == oaInnerAttributes.GetSize());
}

int KWDGInnerAttributes::GetVarPartGranularity() const
{
	return nVarPartGranularity;
}

void KWDGInnerAttributes::SetVarPartGranularity(int nValue)
{
	require(nValue >= 0);

	nVarPartGranularity = nValue;
}

void KWDGInnerAttributes::DeleteAll()
{
	odInnerAttributes.RemoveAll();
	oaInnerAttributes.DeleteAll();
}

void KWDGInnerAttributes::CleanEmptyInnerAttributes()
{
	int nInnerAttribute;
	int nNewIndex;
	KWDGAttribute* innerAttribute;

	// Nettoyage des attributs n'ayant aucune partie
	nNewIndex = 0;
	for (nInnerAttribute = 0; nInnerAttribute < oaInnerAttributes.GetSize(); nInnerAttribute++)
	{
		innerAttribute = GetInnerAttributeAt(nInnerAttribute);
		if (innerAttribute->GetPartNumber() == 0)
		{
			// Suppression de l'indexation dans le dictionnaire
			odInnerAttributes.RemoveKey(innerAttribute->GetAttributeName());

			delete innerAttribute;
		}
		else
		{
			oaInnerAttributes.SetAt(nNewIndex, innerAttribute);
			nNewIndex++;
		}
	}
	oaInnerAttributes.SetSize(nNewIndex);
	ensure(oaInnerAttributes.GetSize() == odInnerAttributes.GetCount());
}

void KWDGInnerAttributes::ExportAllInnerAttributeVarParts(ObjectArray* oaInnerAttributeVarParts) const
{
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	KWDGPart* part;

	require(oaInnerAttributeVarParts != NULL);
	require(oaInnerAttributeVarParts->GetSize() == 0);

	// Parcours de attribut inerne pour compter le nombre total de partie
	for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber(); nInnerAttribute++)
	{
		innerAttribute = GetInnerAttributeAt(nInnerAttribute);

		// Ajout des parties dans le tableau
		part = innerAttribute->GetHeadPart();
		while (part != NULL)
		{
			oaInnerAttributeVarParts->Add(part);
			innerAttribute->GetNextPart(part);
		}
	}
}

int KWDGInnerAttributes::ComputeTotalInnerAttributeVarParts() const
{
	int nTotalPartNumber;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;

	// Parcours de attribut inerne pour compter le nombre total de partie
	nTotalPartNumber = 0;
	for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber(); nInnerAttribute++)
	{
		innerAttribute = GetInnerAttributeAt(nInnerAttribute);
		nTotalPartNumber += innerAttribute->GetPartNumber();
	}
	return nTotalPartNumber;
}

int KWDGInnerAttributes::ComputeTotalInnerAttributeFrequency() const
{
	int nTotalFrequency;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;

	// Parcours de attribut inerne pour compter le nombre total de partie
	nTotalFrequency = 0;
	for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber(); nInnerAttribute++)
	{
		innerAttribute = GetInnerAttributeAt(nInnerAttribute);
		nTotalFrequency += innerAttribute->ComputeTotalPartFrequency();
	}
	return nTotalFrequency;
}

void KWDGInnerAttributes::SortInnerAttributeParts() const
{
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;

	// Tri des attributs internes pour un attribut de grille de type VarPart
	for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber(); nInnerAttribute++)
	{
		innerAttribute = GetInnerAttributeAt(nInnerAttribute);
		innerAttribute->SortParts();
	}
}

boolean KWDGInnerAttributes::AreInnerAttributePartsSorted() const
{
	boolean bIsSorted;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;

	// Verification du tri des parties pour chaque attribut interne
	bIsSorted = true;
	for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber(); nInnerAttribute++)
	{
		innerAttribute = GetInnerAttributeAt(nInnerAttribute);
		bIsSorted = innerAttribute->ArePartsSorted();
		if (not bIsSorted)
			break;
	}
	return bIsSorted;
}

boolean KWDGInnerAttributes::ContainsSubVarParts(const KWDGInnerAttributes* otherInnerAttributes) const
{
	boolean bOk;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	KWDGAttribute* otherInnerAttribute;

	require(otherInnerAttributes != NULL);

	// Cas particuliezr oou les variable internes en parametre sont les memes
	if (otherInnerAttributes == this)
		return true;

	// On doit avoir le meme nombre d'attributs internes
	bOk = GetInnerAttributeNumber() == otherInnerAttributes->GetInnerAttributeNumber();

	// Comparaison pour chaque pour chaque attribut interne
	if (bOk)
	{
		for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber(); nInnerAttribute++)
		{
			innerAttribute = GetInnerAttributeAt(nInnerAttribute);
			otherInnerAttribute = otherInnerAttributes->GetInnerAttributeAt(nInnerAttribute);

			// On doit avoir la meme variable, avec un nombre inferieur de partie
			bOk = bOk and innerAttribute->GetAttributeName() == otherInnerAttribute->GetAttributeName();
			bOk = bOk and innerAttribute->GetAttributeType() == otherInnerAttribute->GetAttributeType();
			bOk = bOk and
			      innerAttribute->GetOwnerAttributeName() == otherInnerAttribute->GetOwnerAttributeName();

			// Test d'inclusion des parties
			bOk = bOk and innerAttribute->ContainsSubParts(otherInnerAttribute);
			if (not bOk)
				break;
		}
	}
	return bOk;
}

boolean KWDGInnerAttributes::Check() const
{
	boolean bOk = true;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	boolean bIsCompletelySpecified;

	require(odInnerAttributes.GetCount() == oaInnerAttributes.GetSize());

	// Parcours des attributs internes
	for (nInnerAttribute = 0; nInnerAttribute < oaInnerAttributes.GetSize(); nInnerAttribute++)
	{
		innerAttribute = GetInnerAttributeAt(nInnerAttribute);

		// Verifications de base
		bOk = bOk and innerAttribute->Check();

		// Verification de l'attribut interne
		if (bOk and innerAttribute->GetOwnerAttributeName() == "")
		{
			AddError("No owner variable for inner variable " + innerAttribute->GetAttributeName() + "\n");
			bOk = false;
		}
		if (bOk and not KWType::IsSimple(innerAttribute->GetAttributeType()))
		{
			AddError("Type of inner variable " + innerAttribute->GetAttributeName() +
				 "\t must be Numerical or Categorical");
			bOk = false;
		}

		// Verification du tri des parties de l'attribut interne, uniquement si l'attribut est completement specifie
		// avec des parties d'effectif non vide, pour pouvoir faire des verifications en cours de construction d'une grille
		bIsCompletelySpecified =
		    innerAttribute->GetPartNumber() > 0 and innerAttribute->GetHeadPart()->GetPartFrequency() > 0;
		if (bIsCompletelySpecified and KWType::IsCoclusteringGroupableType(innerAttribute->GetAttributeType()))
			bIsCompletelySpecified =
			    innerAttribute->GetHeadPart()->GetValueSet()->GetHeadValue()->GetValueFrequency() > 0;
		if (bOk and bIsCompletelySpecified and not innerAttribute->ArePartsSorted())
		{
			AddError("Parts of inner variable " + innerAttribute->GetAttributeName() + " should be sorted");
			bOk = false;
		}
	}
	return bOk;
}

void KWDGInnerAttributes::Write(ostream& ost) const
{
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;

	ost << "Inner variable number \t" << GetInnerAttributeNumber() << endl;
	for (nInnerAttribute = 0; nInnerAttribute < GetInnerAttributeNumber(); nInnerAttribute++)
	{
		innerAttribute = GetInnerAttributeAt(nInnerAttribute);
		innerAttribute->Write(ost);
	}
}

const ALString KWDGInnerAttributes::GetClassLabel() const
{
	return "Inner variables";
}

const ALString KWDGInnerAttributes::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGCell

void KWDGCell::AddFrequenciesFrom(const KWDGCell* cell)
{
	int nTarget;

	require(cell != NULL);
	require(GetTargetValueNumber() == cell->GetTargetValueNumber());

	// Mise a jour de l'effectif global de la cellule
	nCellFrequency += cell->nCellFrequency;
	assert(nCellFrequency >= 0);

	// Mise a jour de l'effectif par classe cible
	for (nTarget = 0; nTarget < ivFrequencyVector.GetSize(); nTarget++)
	{
		ivFrequencyVector.UpgradeAt(nTarget, cell->ivFrequencyVector.GetAt(nTarget));
		assert(ivFrequencyVector.GetAt(nTarget) >= 0);
	}
}

void KWDGCell::RemoveFrequenciesFrom(const KWDGCell* cell)
{
	int nTarget;

	require(cell != NULL);
	require(GetTargetValueNumber() == cell->GetTargetValueNumber());

	// Mise a jour de l'effectif global de la cellule
	nCellFrequency -= cell->nCellFrequency;
	assert(nCellFrequency >= 0);

	// Mise a jour de l'effectif par classe cible
	for (nTarget = 0; nTarget < ivFrequencyVector.GetSize(); nTarget++)
	{
		ivFrequencyVector.UpgradeAt(nTarget, -cell->ivFrequencyVector.GetAt(nTarget));
		assert(ivFrequencyVector.GetAt(nTarget) >= 0);
	}
}

void KWDGCell::MergeFrequenciesFrom(const KWDGCell* cell1, const KWDGCell* cell2)
{
	int nTarget;

	require(cell1 != NULL);
	require(GetTargetValueNumber() == cell1->GetTargetValueNumber());
	require(cell2 != NULL);
	require(GetTargetValueNumber() == cell2->GetTargetValueNumber());

	// Mise a jour de l'effectif global de la cellule
	nCellFrequency = cell1->nCellFrequency + cell2->nCellFrequency;
	assert(nCellFrequency >= 0);

	// Mise a jour de l'effectif par classe cible
	for (nTarget = 0; nTarget < ivFrequencyVector.GetSize(); nTarget++)
	{
		ivFrequencyVector.SetAt(nTarget, cell1->ivFrequencyVector.GetAt(nTarget) +
						     cell2->ivFrequencyVector.GetAt(nTarget));
		assert(ivFrequencyVector.GetAt(nTarget) >= 0);
	}
}

boolean KWDGCell::Check() const
{
	boolean bOk = true;
	int nIndex;
	ALString sTmp;

	// Verification de coherence interne, avec des assertions
	assert(oaParts.GetSize() == oaNextCells.GetSize());
	assert(oaParts.GetSize() == oaPrevCells.GetSize());

	// Verification du lien vers des parties
	for (nIndex = 0; nIndex < GetAttributeNumber(); nIndex++)
	{
		if (oaParts.GetAt(nIndex) == NULL)
		{
			AddError(sTmp + "No referenced part for variable " + IntToString(nIndex));
			bOk = false;
			break;
		}
	}

	// Verification de la coherence de l'effectif total avec l'effectif par classe cible
	if (bOk and ivFrequencyVector.GetSize() != 0)
	{
		if (ComputeTotalFrequency() != nCellFrequency)
		{
			AddError(sTmp + "Total frequency (" + IntToString(nCellFrequency) +
				 " different from the cumulated frequency by target value (" +
				 IntToString(ComputeTotalFrequency()));
			bOk = false;
		}
	}
	return bOk;
}

longint KWDGCell::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDGCell);
	lUsedMemory += ivFrequencyVector.GetUsedMemory();
	lUsedMemory += oaParts.GetUsedMemory();
	lUsedMemory += oaPrevCells.GetUsedMemory();
	lUsedMemory += oaNextCells.GetUsedMemory();
	return lUsedMemory;
}

void KWDGCell::Write(ostream& ost) const
{
	int nAttribute;
	int nTargetValue;

	// Affichage des identifiants des parties de la cellule
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		if (oaParts.GetAt(nAttribute) == NULL)
			ost << "null";
		else
			ost << oaParts.GetAt(nAttribute)->GetObjectLabel();
		ost << "\t";
	}

	// Affichage des effectifs par classe cible
	for (nTargetValue = 0; nTargetValue < ivFrequencyVector.GetSize(); nTargetValue++)
	{
		ost << ivFrequencyVector.GetAt(nTargetValue) << "\t";
	}
	ost << nCellFrequency;
	ost << "\n";
}

void KWDGCell::WriteTargetStats(ostream& ost) const
{
	int nTargetValue;
	int nPercentage;

	// Affichage des effectif par classe cible
	nPercentage = 0;
	if (ivFrequencyVector.GetSize() > 0)
	{
		ost << "(";
		for (nTargetValue = 0; nTargetValue < ivFrequencyVector.GetSize(); nTargetValue++)
		{
			if (nTargetValue > 0)
				ost << ", ";

			// Affichage du pourcentage par classe cible
			if (nCellFrequency > 0)
				nPercentage = (ivFrequencyVector.GetAt(nTargetValue) * 100) / nCellFrequency;
			ost << nPercentage << "%";
		}
		ost << ") ";
	}

	// Effectif total
	ost << nCellFrequency;
}

const ALString KWDGCell::GetClassLabel() const
{
	return "Cell";
}

const ALString KWDGCell::GetObjectLabel() const
{
	ALString sLabel;
	int nAttribute;

	// Label base sur les libelles des parties de la cellule
	sLabel = "(";
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		if (nAttribute > 0)
			sLabel += ", ";
		if (oaParts.GetAt(nAttribute) == NULL)
			sLabel += "null";
		else
			sLabel += oaParts.GetAt(nAttribute)->GetObjectLabel();
	}
	sLabel += ")";
	return sLabel;
}

int KWDGCell::ComputeTotalFrequency() const
{
	int nTotalFrequency;
	int nTarget;

	// Calcul de l'effectif total dans le cas supervise
	if (ivFrequencyVector.GetSize() > 0)
	{
		nTotalFrequency = 0;
		for (nTarget = 0; nTarget < ivFrequencyVector.GetSize(); nTarget++)
			nTotalFrequency += ivFrequencyVector.GetAt(nTarget);
	}
	// Sinon, effectif de la cellule
	else
		nTotalFrequency = nCellFrequency;
	return nTotalFrequency;
}

boolean KWDGCell::GetEmulated() const
{
	return false;
}

int KWDGCellCompare(const void* elem1, const void* elem2)
{
	KWDGCell* cell1;
	KWDGCell* cell2;
	int nPartCompare;
	int nAttributeNumber;
	int nIndex;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux cellules a comparer
	cell1 = cast(KWDGCell*, *(Object**)elem1);
	cell2 = cast(KWDGCell*, *(Object**)elem2);
	assert(cell1->GetAttributeNumber() == cell2->GetAttributeNumber());

	// Comparaison lexicographique des parties references
	nAttributeNumber = cell1->GetAttributeNumber();
	for (nIndex = 0; nIndex < nAttributeNumber; nIndex++)
	{
		// Comparaison de deux pointeurs: attention une simple diference ne rend pas necessairement un int (en
		// 64 bits)
		if (cell1->GetPartAt(nIndex) == cell2->GetPartAt(nIndex))
			nPartCompare = 0;
		else if (cell1->GetPartAt(nIndex) > cell2->GetPartAt(nIndex))
			nPartCompare = 1;
		else
			nPartCompare = -1;
		if (nPartCompare != 0)
			return nPartCompare;
	}
	return 0;
}

int KWDGCellCompareValue(const void* elem1, const void* elem2)
{
	KWDGCell* cell1;
	KWDGCell* cell2;
	KWDGPart* part1;
	KWDGPart* part2;
	int nPartCompare;
	int nAttributeNumber;
	int nIndex;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux cellules a comparer
	cell1 = cast(KWDGCell*, *(Object**)elem1);
	cell2 = cast(KWDGCell*, *(Object**)elem2);
	assert(cell1->GetAttributeNumber() == cell2->GetAttributeNumber());

	// Comparaison lexicographique des parties references
	nAttributeNumber = cell1->GetAttributeNumber();
	for (nIndex = 0; nIndex < nAttributeNumber; nIndex++)
	{
		part1 = cell1->GetPartAt(nIndex);
		part2 = cell2->GetPartAt(nIndex);
		assert(part1->GetPartType() != KWType::Unknown);
		assert(part1->GetPartType() == part2->GetPartType());

		// Comparaison selon le type de partie
		nPartCompare = part1->ComparePartValues(part2);
		if (nPartCompare != 0)
			return nPartCompare;
	}
	return 0;
}

int KWDGCellCompareDecreasingFrequency(const void* elem1, const void* elem2)
{
	KWDGCell* cell1;
	KWDGCell* cell2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux cellules a comparer
	cell1 = cast(KWDGCell*, *(Object**)elem1);
	cell2 = cast(KWDGCell*, *(Object**)elem2);
	assert(cell1->GetAttributeNumber() == cell2->GetAttributeNumber());

	// Comparaison par effectif decroissant
	nCompare = -cell1->GetCellFrequency() + cell2->GetCellFrequency();

	// Prise en compte des valeurs des parties en cas d'egalite
	if (nCompare == 0)
		nCompare = KWDGCellCompareValue(elem1, elem2);
	return nCompare;
}

void KWDataGrid::SetLabel(const ALString& sValue)
{
	sLabel = sValue;
}

const ALString& KWDataGrid::GetLabel() const
{
	return sLabel;
}

void KWDataGrid::SetTargetAttribute(KWDGAttribute* attribute)
{
	targetAttribute = attribute;
}
