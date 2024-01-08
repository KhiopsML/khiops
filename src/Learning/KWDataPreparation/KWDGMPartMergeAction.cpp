// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDGMPartMergeAction.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMPartMergeAction

KWDGMPartMergeAction::KWDGMPartMergeAction()
{
	oaImpactedPartMerges = NULL;
	part1 = NULL;
	part2 = NULL;
	dataGridMerger = NULL;
	dataGridCosts = NULL;
}

KWDGMPartMergeAction::~KWDGMPartMergeAction()
{
	assert(not IsInitialized());
	assert(oaImpactedPartMerges == NULL);
}

KWDGMPart* KWDGMPartMergeAction::PerformPartMerge(KWDGMPartMerge* partMerge)
{
	boolean bOptimizeUpdateOfImpactedPartMerges = true;
	KWDGMPart* mergedPart;
	KWDGMAttribute* attribute;
	int nAttribute;
	double dMergeCost;
	ObjectArray oaTransferredCells1;
	ObjectArray oaMergedCells1;
	ObjectArray oaMergedCells2;
	int nCell;
	KWDGCell* cell1;
	KWDGMCell* cellM1;
	KWDGMCell* cellM2;
	KWDGCell* prevPartCell;
	KWDGCell* nextPartCell;
	double dMergeCellCost;
	POSITION position;
	KWDGMPartMerge* impactedPartMerge;
	KWDGMPart* oppositePart;
	boolean bGarbagePresence;
	debug(double dPartUnionCost);

	require(not IsInitialized());
	require(partMerge != NULL);
	require(partMerge->Check());

	///////////////////////////////////////////////////////////////////////////
	// Preparation: recherche des cellules impliquees

	// Memorisation du DataGridMerger, et de sa structure de couts
	dataGridMerger = cast(KWDataGridMerger*, partMerge->GetPart1()->GetAttribute()->GetDataGrid());
	dataGridCosts = dataGridMerger->GetDataGridCosts();

	// Recherche de l'attribut possedant les parties
	attribute = cast(KWDGMAttribute*, partMerge->GetPart1()->GetAttribute());
	nAttribute = attribute->GetAttributeIndex();
	bGarbagePresence = partMerge->GetGarbagePresence();

	// On recherche la partie ayant le moins de cellules pour la partie origine
	if (partMerge->GetPart1()->GetCellNumber() <= partMerge->GetPart2()->GetCellNumber())
	{
		part1 = partMerge->GetPart1();
		part2 = partMerge->GetPart2();
	}
	else
	{
		part1 = partMerge->GetPart2();
		part2 = partMerge->GetPart1();
	}
	mergedPart = part2;

	// Memorisation du cout de l'union, pour verification de constistence une fois l'union effectuee
	debug(dPartUnionCost = dataGridCosts->ComputePartUnionCost(part1, part2));

	// Identification des cellules impliquees dans la fusion
	//    TransferredCells1: cellules origine a transferer telles quelles dans la fusion
	//    MergedCells1: cellules origine a fusionner
	//    MergedCells2: cellules destination a fusionner avec leur homologue origine
	cell1 = part1->GetHeadCell();
	while (cell1 != NULL)
	{
		cellM1 = cast(KWDGMCell*, cell1);

		// Recherche d'une cellule destination en collision avec la cellule origine
		cellM2 = dataGridMerger->CellDictionaryLookupModifiedCell(cellM1, part2);
		assert(cellM2 == NULL or part2->CheckCell(cellM2));

		// Dispatching des cellules dans les ensembles concernes
		if (cellM2 != NULL)
		{
			oaMergedCells1.Add(cellM1);
			oaMergedCells2.Add(cellM2);
		}
		else
			oaTransferredCells1.Add(cellM1);

		// Cellule suivante
		part1->GetNextCell(cell1);
	}
	assert(part1->GetCellNumber() == oaTransferredCells1.GetSize() + oaMergedCells1.GetSize());
	assert(oaMergedCells1.GetSize() == oaMergedCells2.GetSize());

	// L'initialisation doit etre compelete a ce niveau
	assert(IsInitialized());

	/////////////////////////////////////////////////////////////////////////////
	// Preparation de l'impact sur les fusions, avant realisation de la fusion

	// Dereferencement des parties de la liste triee par nombre de modalites
	// Restitution de la poubelle
	if (attribute->GetAttributeType() == KWType::Symbol)
	{
		attribute->RemovePartFromValueNumberList(part1);
		attribute->RemovePartFromValueNumberList(part2);
	}

	// Dereferencement de la fusion depuis ses parties
	part1->RemovePartMerge(part2);
	part2->RemovePartMerge(part1);

	// Dereferencement puis destruction de la fusion depuis son attribut
	attribute->RemovePartMerge(partMerge);
	delete partMerge;

	// Suppressions des fusions internes avec la partie origine
	position = part1->GetStartPartMerge();
	while (position != NULL)
	{
		part1->GetNextPartMerge(position, impactedPartMerge);
		attribute->RemovePartMerge(impactedPartMerge);
	}

	// Suppressions des fusions internes avec la partie destination
	position = part2->GetStartPartMerge();
	while (position != NULL)
	{
		part2->GetNextPartMerge(position, impactedPartMerge);
		attribute->RemovePartMerge(impactedPartMerge);
	}

	// Mise a jour complete et optimisee des fusions impactees
	if (bOptimizeUpdateOfImpactedPartMerges)
	{
		GlobalUpdateImpactedPartMerges(&oaTransferredCells1, &oaMergedCells1, &oaMergedCells2);
	}

	// On retire les fusions externes impactees de leurs attributs
	if (not bOptimizeUpdateOfImpactedPartMerges)
	{
		InitializeImpactedPartMerges();
		RemoveImpactedPartMergesFromAttributes();
	}

	////////////////////////////////////////////////////////////////////////
	// Realisation de la fusion des parties

	// Import des valeurs de la partie origine vers la partie destination
	if (attribute->GetAttributeType() == KWType::Continuous)
		part2->GetInterval()->Import(part1->GetInterval());
	else
		part2->GetValueSet()->Import(part1->GetValueSet());

	// Transfert des cellules a transferer
	for (nCell = 0; nCell < oaTransferredCells1.GetSize(); nCell++)
	{
		cellM1 = cast(KWDGMCell*, oaTransferredCells1.GetAt(nCell));

		// Deplacement de la cellule dans le dictionnaire des cellules
		dataGridMerger->CellDictionaryTransferModifiedCell(cellM1, part2);

		// Modification de la partie de la cellule transfere
		cellM1->oaParts.SetAt(nAttribute, part2);

		// On supprime le chainage de la cellule dans la partie initial
		part1->nCellNumber--;
		prevPartCell = cast(KWDGCell*, cellM1->oaPrevCells.GetAt(nAttribute));
		nextPartCell = cast(KWDGCell*, cellM1->oaNextCells.GetAt(nAttribute));
		assert(prevPartCell != NULL or part1->headCell == cellM1);
		assert(nextPartCell != NULL or part1->tailCell == cellM1);
		if (prevPartCell != NULL)
			prevPartCell->oaNextCells.SetAt(nAttribute, nextPartCell);
		if (nextPartCell != NULL)
			nextPartCell->oaPrevCells.SetAt(nAttribute, prevPartCell);
		if (part1->headCell == cellM1)
		{
			assert(prevPartCell == NULL);
			part1->headCell = nextPartCell;
		}
		if (part1->tailCell == cellM1)
		{
			assert(nextPartCell == NULL);
			part1->tailCell = prevPartCell;
		}

		// Chainage en fin de la liste des cellules de la partie destination
		part2->nCellNumber++;
		if (part2->headCell == NULL)
		{
			part2->headCell = cellM1;
			part2->tailCell = cellM1;
			cellM1->oaPrevCells.SetAt(nAttribute, NULL);
			cellM1->oaNextCells.SetAt(nAttribute, NULL);
		}
		else
		{
			assert(part2->tailCell != NULL);
			part2->tailCell->oaNextCells.SetAt(nAttribute, cellM1);
			cellM1->oaPrevCells.SetAt(nAttribute, part2->tailCell);
			cellM1->oaNextCells.SetAt(nAttribute, NULL);
			part2->tailCell = cellM1;
		}
	}

	// Fusion des cellules a fusionner
	dMergeCost = 0;
	for (nCell = 0; nCell < oaMergedCells1.GetSize(); nCell++)
	{
		cellM1 = cast(KWDGMCell*, oaMergedCells1.GetAt(nCell));
		cellM2 = cast(KWDGMCell*, oaMergedCells2.GetAt(nCell));

		// Actualisation des effectifs des classes cibles dans la cellule destination
		cellM2->AddFrequenciesFrom(cellM1);

		// Mise a jour du cout de la cellule destination
		dMergeCellCost = dataGridCosts->ComputeCellCost(cellM2);
		dMergeCost += dMergeCellCost - cellM1->GetCost() - cellM2->GetCost();
		cellM2->SetCost(dMergeCellCost);

		// Suppression de la cellule origine du dictionnaire des cellules
		dataGridMerger->CellDictionaryRemoveCell(cellM1);

		// Supression de la cellule origine
		dataGridMerger->DeleteCell(cellM1);
	}

	// Mise a jour de l'effectif total de la partie 2 qui accueille la fusion
	part2->nPartFrequency += part1->nPartFrequency;

	// Mise a jour du cout de la partie 2
	part2->SetCost(dataGridCosts->ComputePartCost(part2));

	if (attribute->GetAttributeType() == KWType::Symbol)
	{
		// Reinsertion de la partie fusionnee dans la liste triee par nombre de modalites
		attribute->AddPartToValueNumberList(part2);
		// Mise a jour du groupe poubelle selon les caracteristiques enregistrees dans le partMerge
		if (bGarbagePresence)
			attribute->SetGarbagePart(cast(KWDGPart*, attribute->slPartValueNumbers->GetHead()));
		else
			attribute->SetGarbagePart(NULL);
	}

	// Verification
	assert(part2->Check());

	////////////////////////////////////////////////////////////////////
	// Impact sur les fusions internes

	// Reevaluation des fusions avec la partie destination
	position = part2->GetStartPartMerge();
	while (position != NULL)
	{
		// Acces a la fusion impactee
		part2->GetNextPartMerge(position, impactedPartMerge);
		assert(impactedPartMerge->GetOppositePart(part2) != part1);

		// Reevaluation de la fusion
		impactedPartMerge->SetMergeCost(dataGridMerger->ComputeMergeCost(impactedPartMerge));
		attribute->AddPartMerge(impactedPartMerge);
	}

	// Reevaluation des fusions avec la partie origine
	position = part1->GetStartPartMerge();
	while (position != NULL)
	{
		// Acces a la fusion impactee
		part1->GetNextPartMerge(position, impactedPartMerge);
		assert(impactedPartMerge->GetOppositePart(part1) != part2);
		oppositePart = impactedPartMerge->GetOppositePart(part1);

		// On dereference la fusion depuis la partie oppose
		assert(oppositePart->LookupPartMerge(part1) == impactedPartMerge);
		oppositePart->RemovePartMerge(part1);

		// On detruit cette fusion si une fusion analogue existe deja
		// avec la partie destination
		if (part2->LookupPartMerge(oppositePart) != NULL)
			delete impactedPartMerge;
		// Sinon, on revalue cette fusion en la connectant a la nouvelle nouveau partie (part2)
		else
		{
			// Changement d'extremite
			if (impactedPartMerge->GetPart1() == part1)
				impactedPartMerge->SetPart1(part2);
			else
				impactedPartMerge->SetPart2(part2);

			// Reevaluation de la fusion
			impactedPartMerge->SetMergeCost(dataGridMerger->ComputeMergeCost(impactedPartMerge));
			attribute->AddPartMerge(impactedPartMerge);

			// Memorisation de la fusion dans la nouvelle partie
			part2->AddPartMerge(impactedPartMerge);

			// Referencement a nouveau de la fusion dans la partie origine
			oppositePart->AddPartMerge(impactedPartMerge);
		}
	}

	// Revaluation des fusions externes impactees
	if (not bOptimizeUpdateOfImpactedPartMerges)
	{
		EvaluateImpactedPartMerges();
		AddImpactedPartMergesToAttributes();
		RemoveImpactedPartMerges();
	}

	// Suppression de la partie origine de la liste des parties de son attribut
	part1->nCellNumber = 0;
	part1->headCell = NULL;
	part1->tailCell = NULL;
	attribute->DeletePart(part1);

	// Recalcul du cout de l'attribut
	attribute->SetCost(dataGridCosts->ComputeAttributeCost(attribute, attribute->GetPartNumber()));

	// Mise a jour des statistiques et du cout de la grille
	dataGridMerger->dLnGridSize +=
	    log(double(attribute->GetPartNumber())) - log(double(attribute->GetPartNumber() + 1));
	if (dataGridMerger->dLnGridSize < 1e-10)
		dataGridMerger->dLnGridSize = 0;
	if (attribute->GetPartNumber() == 1)
		dataGridMerger->nInformativeAttributeNumber--;
	dataGridMerger->nTotalPartNumber--;
	dataGridMerger->SetCost(dataGridCosts->ComputeDataGridCost(dataGridMerger, dataGridMerger->GetLnGridSize(),
								   dataGridMerger->GetInformativeAttributeNumber()));

	// Verification de consistence entre cout de l'union et cout de la partie fusionnee
	// uniquement dans le cas d'un attribut source car le ComputePartCost
	// ne tient pas encore compte du changement de la taille de la partition cible
	assert((attribute->GetAttributeTargetFunction()) or
	       fabs(dPartUnionCost - dataGridCosts->ComputePartCost(part2)) < 1e-5);

	// Nettoyage
	part1 = NULL;
	part2 = NULL;
	dataGridMerger = NULL;
	dataGridCosts = NULL;

	// Retourne le resulatt de la fusion
	return mergedPart;
}

boolean KWDGMPartMergeAction::IsInitialized() const
{
	boolean bOk = true;

	bOk = bOk and part1 != NULL;
	bOk = bOk and part2 != NULL;
	bOk = bOk and dataGridMerger != NULL;
	bOk = bOk and dataGridCosts == dataGridMerger->GetDataGridCosts();
	bOk = bOk and part1->GetAttribute() == part2->GetAttribute();
	bOk = bOk and part1 != part2;
	bOk = bOk and part1->GetAttribute() != NULL;
	bOk = bOk and part1->GetAttribute()->GetDataGrid() == dataGridMerger;
	bOk = bOk and part1->GetCellNumber() <= part2->GetCellNumber();

	return bOk;
}

void KWDGMPartMergeAction::GlobalUpdateImpactedPartMerges(ObjectArray* oaTransferredCells1, ObjectArray* oaMergedCells1,
							  ObjectArray* oaMergedCells2)
{
	NumericKeyDictionary nkdTransferredCells1;
	NumericKeyDictionary nkdMergedCells1;
	NumericKeyDictionary nkdMergedCells2;
	int nCell;
	KWDGMCell* cell;
	KWDGMCell* mergedCell;
	int nAttribute;
	int nImpactedAttribute;
	KWDGMAttribute* impactedAttribute;

	require(IsInitialized());
	require(oaImpactedPartMerges == NULL);
	require(oaTransferredCells1 != NULL);
	require(oaMergedCells1 != NULL);
	require(oaMergedCells2 != NULL);
	require(oaTransferredCells1->GetSize() + oaMergedCells1->GetSize() == part1->GetCellNumber());
	require(oaMergedCells1->GetSize() == oaMergedCells2->GetSize());

	// Rangement des cellules transferes dans un dictionnaire
	for (nCell = 0; nCell < oaTransferredCells1->GetSize(); nCell++)
	{
		cell = cast(KWDGMCell*, oaTransferredCells1->GetAt(nCell));
		assert(nkdTransferredCells1.Lookup((NUMERIC)cell) == NULL);
		nkdTransferredCells1.SetAt((NUMERIC)cell, cell);
	}
	assert(oaTransferredCells1->GetSize() == nkdTransferredCells1.GetCount());

	// Rangement des cellules fusionnees dans deux dictionnaires
	for (nCell = 0; nCell < oaMergedCells1->GetSize(); nCell++)
	{
		// Rangement dans le premier dictionnaire
		cell = cast(KWDGMCell*, oaMergedCells1->GetAt(nCell));
		assert(nkdMergedCells1.Lookup((NUMERIC)cell) == NULL);
		nkdMergedCells1.SetAt((NUMERIC)cell, cell);

		// Rangement dans le second dictionnaire, avec la meme cle
		mergedCell = cast(KWDGMCell*, oaMergedCells2->GetAt(nCell));
		assert(nkdMergedCells2.Lookup((NUMERIC)cell) == NULL);
		nkdMergedCells2.SetAt((NUMERIC)cell, mergedCell);
	}
	assert(oaMergedCells1->GetSize() == nkdMergedCells1.GetCount());
	assert(oaMergedCells2->GetSize() == nkdMergedCells2.GetCount());

	// Traitement des containers par attribut externe
	nAttribute = part1->GetAttribute()->GetAttributeIndex();
	for (nImpactedAttribute = 0; nImpactedAttribute < dataGridMerger->GetAttributeNumber(); nImpactedAttribute++)
	{
		// Traitement si container non vide
		if (nImpactedAttribute != nAttribute)
		{
			impactedAttribute = cast(KWDGMAttribute*, dataGridMerger->GetAttributeAt(nImpactedAttribute));
			GlobalUpdateImpactedPartMergesForAttribute(impactedAttribute, oaTransferredCells1,
								   &nkdTransferredCells1, oaMergedCells1,
								   &nkdMergedCells1, oaMergedCells2, &nkdMergedCells2);
		}
	}
}

void KWDGMPartMergeAction::GlobalUpdateImpactedPartMergesForAttribute(
    KWDGMAttribute* impactedAttribute, ObjectArray* oaTransferredCells1, NumericKeyDictionary* nkdTransferredCells1,
    ObjectArray* oaMergedCells1, NumericKeyDictionary* nkdMergedCells1, ObjectArray* oaMergedCells2,
    NumericKeyDictionary* nkdMergedCells2)
{
	boolean bDisplay = false;
	int nCell;
	KWDGMCell* evaluationCell;
	KWDGMCell* evaluationCell2;
	KWDGMCell* initialMergeCell;
	KWDGMCell* cell;
	KWDGMCell* mergedCell;
	KWDGMCell* oppositeCell;
	KWDGMCell* oppositeCell2;
	int nImpactedAttribute;
	KWDGMPart* impactedPart;
	KWDGMPart* oppositePart;
	KWDGMPartMerge* partMerge;
	int nMerge;
	double dMergeDeltaCost;
	double dInitialMergeCost;
	double dInitialMergeCost2;
	NumericKeyDictionary nkdUpdatedPartMerges;
	ObjectArray oaUpdatedPartMerges;
	POSITION position;

	require(IsInitialized());
	require(impactedAttribute != NULL);
	require(impactedAttribute != part1->GetAttribute());
	require(oaImpactedPartMerges == NULL);
	require(oaTransferredCells1 != NULL);
	require(oaMergedCells1 != NULL);
	require(oaMergedCells2 != NULL);
	require(oaTransferredCells1->GetSize() + oaMergedCells1->GetSize() == part1->GetCellNumber());
	require(oaMergedCells1->GetSize() == oaMergedCells2->GetSize());
	require(nkdTransferredCells1 != NULL);
	require(nkdTransferredCells1->GetCount() == oaTransferredCells1->GetSize());
	require(nkdMergedCells1 != NULL);
	require(nkdMergedCells1->GetCount() == oaMergedCells1->GetSize());
	require(nkdMergedCells2 != NULL);
	require(nkdMergedCells2->GetCount() == oaMergedCells2->GetSize());

	// Affichage
	if (bDisplay)
	{
		cout << impactedAttribute->GetAttributeName() << "\t"
		     << "Impacts sur les fusions externes"
		     << "\n";
	}

	// Index de l'attribut impacte
	nImpactedAttribute = impactedAttribute->GetAttributeIndex();

	// Creation des cellules servant a evaluer les merges de cellules, avec le bon type
	evaluationCell = cast(KWDGMCell*, dataGridMerger->NewCell());
	evaluationCell2 = cast(KWDGMCell*, dataGridMerger->NewCell());
	initialMergeCell = cast(KWDGMCell*, dataGridMerger->NewCell());

	// Traitement des cellules transferees
	for (nCell = 0; nCell < oaTransferredCells1->GetSize(); nCell++)
	{
		cell = cast(KWDGMCell*, oaTransferredCells1->GetAt(nCell));
		assert(nkdTransferredCells1->Lookup((NUMERIC)cell) == cell);

		// Affichage
		if (bDisplay)
			cout << "Cellule transferee\t" << cell->GetObjectLabel() << "\n";

		// Recherche de la partie externe
		impactedPart = cast(KWDGMPart*, cell->GetPartAt(nImpactedAttribute));

		// Parcours des fusions de la partie
		position = impactedPart->GetStartPartMerge();
		while (position != NULL)
		{
			impactedPart->GetNextPartMerge(position, partMerge);
			dMergeDeltaCost = 0;

			// Recherche de l'autre partie extremite de la fusion
			oppositePart = partMerge->GetOppositePart(impactedPart);

			// Affichage
			if (bDisplay)
				cout << "\tPartie opposee\t" << oppositePart->GetObjectLabel() << "\n";

			// Recherche d'une cellule entrant en collision avec la cellule initial
			// lors de la fusion externe
			oppositeCell = dataGridMerger->CellDictionaryLookupModifiedCell(cell, oppositePart);

			// Pour eviter les impacts doublons, on compare les cells
			if (cell < oppositeCell)
			{
				// Impact de l'eventuelle collision sur la fusion
				//   Pas de collision: pas d'impact
				if (oppositeCell == NULL)
				{
					if (bDisplay)
						cout << "\t" << dMergeDeltaCost << "\tPas de collision\n";
				}
				//   Collision avec autre cellule transferee: pas d'impact
				else if (nkdTransferredCells1->Lookup((NUMERIC)oppositeCell) != NULL)
				{
					if (bDisplay)
						cout << "\t" << dMergeDeltaCost
						     << "\tCollision avec une autre cellule transfere\n";
				}
				//   Collision avec cellule fusionnee: mise a jour necessaire
				else if (nkdMergedCells1->Lookup((NUMERIC)oppositeCell) != NULL)
				{
					// On supprime le cout de merge des cellules origines
					evaluationCell->MergeFrequenciesFrom(cell, oppositeCell);
					dMergeDeltaCost = -(dataGridCosts->ComputeCellCost(evaluationCell) -
							    cell->GetCost() - oppositeCell->GetCost());

					// Recherche de de la cellule interne destination de la fusion
					mergedCell = cast(KWDGMCell*, nkdMergedCells2->Lookup((NUMERIC)oppositeCell));
					check(mergedCell);

					// Calcul du cout de la fusion interne initiale
					initialMergeCell->MergeFrequenciesFrom(oppositeCell, mergedCell);
					dInitialMergeCost = dataGridCosts->ComputeCellCost(initialMergeCell);

					// On rajoute le cout de merge en integrant la cellule interne fusionnee
					evaluationCell->AddFrequenciesFrom(mergedCell);
					dMergeDeltaCost += (dataGridCosts->ComputeCellCost(evaluationCell) -
							    cell->GetCost() - dInitialMergeCost);

					// Affichage
					if (bDisplay)
					{
						cout << "\t" << dMergeDeltaCost
						     << "\tCollision avec cellule fusionnee\t"
						     << oppositeCell->GetObjectLabel() << "\n";
					}
				}
			}

			// Si pas de collision, recherche d'une collision avec une cellule destination
			if (oppositeCell == NULL)
			{
				oppositeCell2 =
				    dataGridMerger->CellDictionaryLookupTwiceModifiedCell(cell, part2, oppositePart);

				// Si collision, prise en compte du cout de cette nouvelle collision
				// dans la fusion
				if (oppositeCell2 != NULL)
				{
					evaluationCell->MergeFrequenciesFrom(cell, oppositeCell2);
					dMergeDeltaCost = dataGridCosts->ComputeCellCost(evaluationCell) -
							  cell->GetCost() - oppositeCell2->GetCost();

					// Affichage
					if (bDisplay)
					{
						cout << "\t" << dMergeDeltaCost
						     << "\tCollision avec cellule destination\t"
						     << oppositeCell2->GetObjectLabel() << "\n";
					}
				}
			}

			// Traitement de la fusion si son cout a change
			if (fabs(dMergeDeltaCost) > dataGridMerger->dEpsilon)
			{
				// On la retire (si necessaire) de son attribut
				if (partMerge->GetPosition() != NULL)
					impactedAttribute->RemovePartMerge(partMerge);

				// Modification de son cout
				partMerge->SetMergeCost(partMerge->GetMergeCost() + dMergeDeltaCost);

				// On memorise la fusion modifiee
				nkdUpdatedPartMerges.SetAt((NUMERIC)partMerge, partMerge);
			}
		}
	}

	// Traitement des cellules fusionnees
	for (nCell = 0; nCell < oaMergedCells1->GetSize(); nCell++)
	{
		cell = cast(KWDGMCell*, oaMergedCells1->GetAt(nCell));
		assert(nkdMergedCells1->Lookup((NUMERIC)cell) == cell);

		// Affichage
		if (bDisplay)
			cout << "Cellule fusionnee\t" << cell->GetObjectLabel() << "\n";

		// Recherche de la partie externe
		impactedPart = cast(KWDGMPart*, cell->GetPartAt(nImpactedAttribute));

		// Parcours des fusions de la partie
		position = impactedPart->GetStartPartMerge();
		while (position != NULL)
		{
			impactedPart->GetNextPartMerge(position, partMerge);
			dMergeDeltaCost = 0;

			// Recherche de l'autre partie extremite de la fusion
			oppositePart = partMerge->GetOppositePart(impactedPart);

			// Affichage
			if (bDisplay)
				cout << "\tPartie opposee\t" << oppositePart->GetObjectLabel() << "\n";

			// Recherche d'une cellule entrant en collision avec la cellule initial
			// lors de la fusion externe
			oppositeCell = dataGridMerger->CellDictionaryLookupModifiedCell(cell, oppositePart);

			// Pour eviter les impacts doublons, on compare les cells
			if (cell < oppositeCell)
			{
				// Impact de l'eventuelle collision sur la fusion
				//   Pas de collision: pas d'impact
				if (oppositeCell == NULL)
				{
					if (bDisplay)
						cout << "\t" << dMergeDeltaCost << "\tPas de collision\n";
				}
				//   Collision avec cellule transferee: mise a jour necessaire
				else if (nkdTransferredCells1->Lookup((NUMERIC)oppositeCell) != NULL)
				{
					// On supprime le cout de merge des cellules origines
					evaluationCell->MergeFrequenciesFrom(cell, oppositeCell);
					dMergeDeltaCost = -(dataGridCosts->ComputeCellCost(evaluationCell) -
							    cell->GetCost() - oppositeCell->GetCost());

					// Recherche de la cellule interne destination de la fusion
					mergedCell = cast(KWDGMCell*, nkdMergedCells2->Lookup((NUMERIC)cell));
					check(mergedCell);

					// Calcul du cout de la fusion interne initiale
					initialMergeCell->MergeFrequenciesFrom(cell, mergedCell);
					dInitialMergeCost = dataGridCosts->ComputeCellCost(initialMergeCell);

					// On rajoute le cout de merge en integrant la cellule interne fusionnee
					evaluationCell->AddFrequenciesFrom(mergedCell);
					dMergeDeltaCost += (dataGridCosts->ComputeCellCost(evaluationCell) -
							    oppositeCell->GetCost() - dInitialMergeCost);

					// Affichage
					if (bDisplay)
					{
						cout << "\t" << dMergeDeltaCost
						     << "\tCollision avec cellule transferee\t"
						     << oppositeCell->GetObjectLabel() << "\n";
					}
				}
				//   Collision avec cellule fusionnee: mise a jour necessaire
				else if (nkdMergedCells1->Lookup((NUMERIC)oppositeCell) != NULL)
				{
					// On supprime le cout de merge des cellules origines
					evaluationCell->MergeFrequenciesFrom(cell, oppositeCell);
					dMergeDeltaCost = -(dataGridCosts->ComputeCellCost(evaluationCell) -
							    cell->GetCost() - oppositeCell->GetCost());

					// Recherche de la cellule interne destination de la fusion
					mergedCell = cast(KWDGMCell*, nkdMergedCells2->Lookup((NUMERIC)cell));
					check(mergedCell);

					// Recherche de la cellule fusionnee extremite
					oppositeCell2 =
					    cast(KWDGMCell*, nkdMergedCells2->Lookup((NUMERIC)oppositeCell));
					assert(oppositeCell2 == dataGridMerger->CellDictionaryLookupModifiedCell(
								    mergedCell, oppositePart));

					// Calcul du cout de la fusion interne initiale
					initialMergeCell->MergeFrequenciesFrom(cell, mergedCell);
					dInitialMergeCost = dataGridCosts->ComputeCellCost(initialMergeCell);

					// Calcul du cout de la fusion interne initiale
					initialMergeCell->MergeFrequenciesFrom(oppositeCell, oppositeCell2);
					dInitialMergeCost2 = dataGridCosts->ComputeCellCost(initialMergeCell);

					// On supprime le cout de merge des cellules destination
					evaluationCell2->MergeFrequenciesFrom(mergedCell, oppositeCell2);
					dMergeDeltaCost -= (dataGridCosts->ComputeCellCost(evaluationCell2) -
							    mergedCell->GetCost() - oppositeCell2->GetCost());

					// On rajoute le cout de merge en integrant les cellules internes fusionnees
					evaluationCell->AddFrequenciesFrom(evaluationCell2);
					dMergeDeltaCost += (dataGridCosts->ComputeCellCost(evaluationCell) -
							    dInitialMergeCost - dInitialMergeCost2);

					// Affichage
					if (bDisplay)
					{
						cout << "\t" << dMergeDeltaCost
						     << "\tCollision avec cellule fusionnee\t"
						     << oppositeCell->GetObjectLabel() << "\n";
					}
				}
			}

			// Si pas de collision, recherche d'une collision avec une cellule destination
			if (oppositeCell == NULL)
			{
				// Recherche de la cellule interne destination de la fusion
				mergedCell = cast(KWDGMCell*, nkdMergedCells2->Lookup((NUMERIC)cell));
				check(mergedCell);

				// Recherche d'une collision destination
				oppositeCell2 =
				    dataGridMerger->CellDictionaryLookupModifiedCell(mergedCell, oppositePart);
				assert(oppositeCell2 == dataGridMerger->CellDictionaryLookupTwiceModifiedCell(
							    cell, part2, oppositePart));

				// Si collision, prise en compte du cout de cette nouvelle collision
				// dans la fusion
				if (oppositeCell2 != NULL)
				{
					// On supprime le cout de merge des cellules destination
					evaluationCell->MergeFrequenciesFrom(mergedCell, oppositeCell2);
					dMergeDeltaCost = -(dataGridCosts->ComputeCellCost(evaluationCell) -
							    mergedCell->GetCost() - oppositeCell2->GetCost());

					// Calcul du cout de la fusion interne initiale
					initialMergeCell->MergeFrequenciesFrom(cell, mergedCell);
					dInitialMergeCost = dataGridCosts->ComputeCellCost(initialMergeCell);

					// On rajoute le cout de merge en integrant la cellule interne fusionnee
					evaluationCell->AddFrequenciesFrom(cell);
					dMergeDeltaCost += (dataGridCosts->ComputeCellCost(evaluationCell) -
							    oppositeCell2->GetCost() - dInitialMergeCost);

					// Affichage
					if (bDisplay)
					{
						cout << "\t" << dMergeDeltaCost
						     << "\tCollision avec cellule destination\t"
						     << oppositeCell2->GetObjectLabel() << "\n";
					}
				}
			}

			// Traitement de la fusion si son cout a change
			if (fabs(dMergeDeltaCost) > dataGridMerger->dEpsilon)
			{
				// On la retire (si necessaire) de son attribut
				if (partMerge->GetPosition() != NULL)
					impactedAttribute->RemovePartMerge(partMerge);

				// Modification de son cout
				partMerge->SetMergeCost(partMerge->GetMergeCost() + dMergeDeltaCost);

				// On memorise la fusion modifiee
				nkdUpdatedPartMerges.SetAt((NUMERIC)partMerge, partMerge);
			}
		}
	}

	// On reintegre dans l'attribut impacte les fusions modifiees
	nkdUpdatedPartMerges.ExportObjectArray(&oaUpdatedPartMerges);
	for (nMerge = 0; nMerge < oaUpdatedPartMerges.GetSize(); nMerge++)
	{
		partMerge = cast(KWDGMPartMerge*, oaUpdatedPartMerges.GetAt(nMerge));
		assert(partMerge->GetPosition() == NULL);
		impactedAttribute->AddPartMerge(partMerge);
	}

	// Nettoyage
	delete evaluationCell;
	delete evaluationCell2;
	delete initialMergeCell;
}

void KWDGMPartMergeAction::InitializeImpactedPartMerges()
{
	KWDGMAttribute* impactedAttribute;
	int nImpactedAttribute;
	int nAttribute;
	NumericKeyDictionary* nkdAttributeImpactedPartMerges;

	require(IsInitialized());
	require(oaImpactedPartMerges == NULL);

	// Parcours des attributs externes
	oaImpactedPartMerges = new ObjectArray;
	oaImpactedPartMerges->SetSize(dataGridMerger->GetAttributeNumber());
	nAttribute = part1->GetAttribute()->GetAttributeIndex();
	for (nImpactedAttribute = 0; nImpactedAttribute < oaImpactedPartMerges->GetSize(); nImpactedAttribute++)
	{
		// Uniquement pour les attributs externes
		if (nImpactedAttribute != nAttribute)
		{
			impactedAttribute = cast(KWDGMAttribute*, dataGridMerger->GetAttributeAt(nImpactedAttribute));
			nkdAttributeImpactedPartMerges = InitializeImpactedPartMergesForAttribute(impactedAttribute);
			oaImpactedPartMerges->SetAt(nImpactedAttribute, nkdAttributeImpactedPartMerges);
		}
	}
	ensure(CheckImpactedPartMerges());
}

NumericKeyDictionary* KWDGMPartMergeAction::InitializeImpactedPartMergesForAttribute(KWDGMAttribute* impactedAttribute)
{
	NumericKeyDictionary* nkdAttributeImpactedPartMerges;
	int nImpactedAttribute;
	KWDGCell* cell1;
	KWDGCell* cell2;
	POSITION position;
	KWDGPart* part;
	KWDGMPart* impactedPart;
	KWDGMPartMerge* impactedPartMerge;
	KWDGMPart* oppositePart;
	int nPart;
	int nOtherPart;
	NumericKeyDictionary nkdImpactedParts1;
	NumericKeyDictionary nkdImpactedParts2;
	ObjectArray oaImpactedParts1;
	ObjectArray oaImpactedParts2;
	NumericKeyDictionary nkdImpactedPartMerges;
	KWDGMPartMergeArray* partMergeArray;

	require(IsInitialized());
	require(oaImpactedPartMerges != NULL);
	require(impactedAttribute != NULL);
	require(impactedAttribute != part1->GetAttribute());

	// Creation du dictionnaire resultat
	nkdAttributeImpactedPartMerges = new NumericKeyDictionary;

	// Initialisations
	nImpactedAttribute = impactedAttribute->GetAttributeIndex();

	// Recherche des parties externes impactes en sa basant sur les
	// cellules de la partie origine de la fusion
	cell1 = part1->GetHeadCell();
	while (cell1 != NULL)
	{
		impactedPart = cast(KWDGMPart*, cell1->GetPartAt(nImpactedAttribute));
		nkdImpactedParts1.SetAt((NUMERIC)impactedPart, impactedPart);
		part1->GetNextCell(cell1);
	}
	nkdImpactedParts1.ExportObjectArray(&oaImpactedParts1);
	assert(oaImpactedParts1.GetSize() <= part1->GetCellNumber());

	// Recherche des fusions externes potentiellement impactees,
	// pour un attribut continu
	if (impactedAttribute->GetAttributeType() == KWType::Continuous)
	{
		// Creation d'un tableau de fusions par partie impacte
		for (nPart = 0; nPart < oaImpactedParts1.GetSize(); nPart++)
		{
			impactedPart = cast(KWDGMPart*, oaImpactedParts1.GetAt(nPart));

			// Creation et enregistrement d'un tableau de fusions
			partMergeArray = new KWDGMPartMergeArray;
			partMergeArray->SetPart1(impactedPart);
			nkdAttributeImpactedPartMerges->SetAt((NUMERIC)impactedPart, partMergeArray);

			// Recherche des fusions impactees
			position = impactedPart->GetStartPartMerge();
			while (position != NULL)
			{
				impactedPart->GetNextPartMerge(position, impactedPartMerge);

				// Traitement de la fusion si elle n'a pas deja ete enregistree
				if (nkdImpactedPartMerges.Lookup((NUMERIC)impactedPartMerge) == NULL)
				{
					// Enregistrement de la fusion
					nkdImpactedPartMerges.SetAt((NUMERIC)impactedPartMerge, impactedPartMerge);

					// Memorisation dans le tableau des fusions associees a la partie
					partMergeArray->GetPartMerges()->Add(impactedPartMerge);
				}
			}
		}
	}
	// Recherche des fusions externes potentiellement impactees,
	// pour un attribut symbolique
	else
	{
		// Si plus de parties que de cellules destination, on recherche
		// les parties destinations impactes a partir des cellules
		if (impactedAttribute->GetPartNumber() > part2->GetCellNumber())
		{
			cell2 = part2->GetHeadCell();
			while (cell2 != NULL)
			{
				impactedPart = cast(KWDGMPart*, cell2->GetPartAt(nImpactedAttribute));

				// On prend en compte les parties non deja recenses parmi
				// les parties origines
				if (nkdImpactedParts1.Lookup((NUMERIC)impactedPart) == NULL)
					nkdImpactedParts2.SetAt((NUMERIC)impactedPart, impactedPart);
				part2->GetNextCell(cell2);
			}
			nkdImpactedParts2.ExportObjectArray(&oaImpactedParts2);
		}
		// Sinon, on se base sur un parcours de tous les parties
		else
		{
			part = impactedAttribute->GetHeadPart();
			while (part != NULL)
			{
				impactedPart = cast(KWDGMPart*, part);

				// On prend en compte les parties non deja recenses parmi
				// les parties origines
				if (nkdImpactedParts1.Lookup((NUMERIC)impactedPart) == NULL)
					nkdImpactedParts2.SetAt((NUMERIC)impactedPart, impactedPart);
				impactedAttribute->GetNextPart(part);
			}
			nkdImpactedParts2.ExportObjectArray(&oaImpactedParts2);
		}
		assert(oaImpactedParts2.GetSize() <= part2->GetCellNumber());
		assert(oaImpactedParts1.GetSize() + oaImpactedParts2.GetSize() <= impactedAttribute->GetPartNumber());

		// Creation d'un tableau de fusions par partie impactee
		for (nPart = 0; nPart < oaImpactedParts1.GetSize(); nPart++)
		{
			impactedPart = cast(KWDGMPart*, oaImpactedParts1.GetAt(nPart));

			// Creation et enregistrement d'un tableau de fusions
			partMergeArray = new KWDGMPartMergeArray;
			partMergeArray->SetPart1(impactedPart);
			nkdAttributeImpactedPartMerges->SetAt((NUMERIC)impactedPart, partMergeArray);

			// Il faut cree les fusions intra parties externes origines
			for (nOtherPart = nPart + 1; nOtherPart < oaImpactedParts1.GetSize(); nOtherPart++)
			{
				oppositePart = cast(KWDGMPart*, oaImpactedParts1.GetAt(nOtherPart));

				// Memorisation dans le tableau des fusions associees a la partie
				impactedPartMerge = impactedPart->LookupPartMerge(oppositePart);
				check(impactedPartMerge);
				partMergeArray->GetPartMerges()->Add(impactedPartMerge);
			}

			// Il faut cree egalement les fusions entre parties externes origines et destination
			for (nOtherPart = 0; nOtherPart < oaImpactedParts2.GetSize(); nOtherPart++)
			{
				oppositePart = cast(KWDGMPart*, oaImpactedParts2.GetAt(nOtherPart));

				// Memorisation dans le tableau des fusions associees a la partie
				impactedPartMerge = impactedPart->LookupPartMerge(oppositePart);
				check(impactedPartMerge);
				partMergeArray->GetPartMerges()->Add(impactedPartMerge);
			}
		}
	}
	assert(nkdAttributeImpactedPartMerges->GetCount() == oaImpactedParts1.GetSize());

	return nkdAttributeImpactedPartMerges;
}

void KWDGMPartMergeAction::RemoveImpactedPartMergesFromAttributes() const
{
	int nImpactedAttribute;
	KWDGMAttribute* impactedAttribute;
	NumericKeyDictionary* nkdAttributeImpactedPartMerges;
	ObjectArray oaAttributeImpactedPartMerges;
	int nPart;
	KWDGMPartMergeArray* partMergeArray;
	int nMerge;
	KWDGMPartMerge* partMerge;
	NumericKeyDictionary nkdAllPartMerges;

	require(IsInitialized());
	require(oaImpactedPartMerges != NULL);

	// Traitement des containers par attribut externe
	for (nImpactedAttribute = 0; nImpactedAttribute < oaImpactedPartMerges->GetSize(); nImpactedAttribute++)
	{
		// Acces au container par attribut
		nkdAttributeImpactedPartMerges =
		    cast(NumericKeyDictionary*, oaImpactedPartMerges->GetAt(nImpactedAttribute));

		// Traitement si container non vide
		if (nkdAttributeImpactedPartMerges != NULL)
		{
			// Acces a l'attribut impacte
			impactedAttribute = cast(KWDGMAttribute*, dataGridMerger->GetAttributeAt(nImpactedAttribute));

			// Conversion du dictionnaire en tableau
			nkdAttributeImpactedPartMerges->ExportObjectArray(&oaAttributeImpactedPartMerges);

			// Traitement des tableaux de fusions impactees
			for (nPart = 0; nPart < oaAttributeImpactedPartMerges.GetSize(); nPart++)
			{
				partMergeArray = cast(KWDGMPartMergeArray*, oaAttributeImpactedPartMerges.GetAt(nPart));

				// Suppression des fusions impactees de l'attribut
				for (nMerge = 0; nMerge < partMergeArray->GetPartMerges()->GetSize(); nMerge++)
				{
					partMerge =
					    cast(KWDGMPartMerge*, partMergeArray->GetPartMerges()->GetAt(nMerge));
					impactedAttribute->RemovePartMerge(partMerge);
				}
			}
		}
	}
}

void KWDGMPartMergeAction::EvaluateImpactedPartMerges() const
{
	int nImpactedAttribute;
	NumericKeyDictionary* nkdAttributeImpactedPartMerges;
	ObjectArray oaAttributeImpactedPartMerges;
	int nPart;
	KWDGMPartMergeArray* partMergeArray;
	int nMerge;
	KWDGMPartMerge* partMerge;
	NumericKeyDictionary nkdAllPartMerges;

	require(IsInitialized());
	require(oaImpactedPartMerges != NULL);

	// Traitement des containers par attribut externe
	for (nImpactedAttribute = 0; nImpactedAttribute < oaImpactedPartMerges->GetSize(); nImpactedAttribute++)
	{
		// Acces au container par attribut
		nkdAttributeImpactedPartMerges =
		    cast(NumericKeyDictionary*, oaImpactedPartMerges->GetAt(nImpactedAttribute));

		// Traitement si container non vide
		if (nkdAttributeImpactedPartMerges != NULL)
		{
			// Conversion du dictionnaire en tableau
			nkdAttributeImpactedPartMerges->ExportObjectArray(&oaAttributeImpactedPartMerges);

			// Traitement des tableaux de fusions impactees
			for (nPart = 0; nPart < oaAttributeImpactedPartMerges.GetSize(); nPart++)
			{
				partMergeArray = cast(KWDGMPartMergeArray*, oaAttributeImpactedPartMerges.GetAt(nPart));

				// Ajout des fusions impactees a l'attribut
				for (nMerge = 0; nMerge < partMergeArray->GetPartMerges()->GetSize(); nMerge++)
				{
					partMerge =
					    cast(KWDGMPartMerge*, partMergeArray->GetPartMerges()->GetAt(nMerge));
					assert(partMerge->GetPosition() == NULL);
					partMerge->SetMergeCost(dataGridMerger->ComputeMergeCost(partMerge));
				}
			}
		}
	}
}

void KWDGMPartMergeAction::AddImpactedPartMergesToAttributes() const
{
	int nImpactedAttribute;
	KWDGMAttribute* impactedAttribute;
	NumericKeyDictionary* nkdAttributeImpactedPartMerges;
	ObjectArray oaAttributeImpactedPartMerges;
	int nPart;
	KWDGMPartMergeArray* partMergeArray;
	int nMerge;
	KWDGMPartMerge* partMerge;
	NumericKeyDictionary nkdAllPartMerges;

	require(IsInitialized());
	require(oaImpactedPartMerges != NULL);

	// Traitement des containers par attribut externe
	for (nImpactedAttribute = 0; nImpactedAttribute < oaImpactedPartMerges->GetSize(); nImpactedAttribute++)
	{
		// Acces au container par attribut
		nkdAttributeImpactedPartMerges =
		    cast(NumericKeyDictionary*, oaImpactedPartMerges->GetAt(nImpactedAttribute));

		// Traitement si container non vide
		if (nkdAttributeImpactedPartMerges != NULL)
		{
			// Acces a l'attribut impacte
			impactedAttribute = cast(KWDGMAttribute*, dataGridMerger->GetAttributeAt(nImpactedAttribute));

			// Conversion du dictionnaire en tableau
			nkdAttributeImpactedPartMerges->ExportObjectArray(&oaAttributeImpactedPartMerges);

			// Traitement des tableaux de fusions impactees
			for (nPart = 0; nPart < oaAttributeImpactedPartMerges.GetSize(); nPart++)
			{
				partMergeArray = cast(KWDGMPartMergeArray*, oaAttributeImpactedPartMerges.GetAt(nPart));

				// Evaluation des fusions impactees
				for (nMerge = 0; nMerge < partMergeArray->GetPartMerges()->GetSize(); nMerge++)
				{
					partMerge =
					    cast(KWDGMPartMerge*, partMergeArray->GetPartMerges()->GetAt(nMerge));
					impactedAttribute->AddPartMerge(partMerge);
				}
			}
		}
	}
}

void KWDGMPartMergeAction::RemoveImpactedPartMerges()
{
	int nImpactedAttribute;
	NumericKeyDictionary* nkdAttributeImpactedPartMerges;

	require(IsInitialized());
	require(oaImpactedPartMerges != NULL);

	// Traitement des containers par attribut externe
	for (nImpactedAttribute = 0; nImpactedAttribute < oaImpactedPartMerges->GetSize(); nImpactedAttribute++)
	{
		// Acces au container par attribut
		nkdAttributeImpactedPartMerges =
		    cast(NumericKeyDictionary*, oaImpactedPartMerges->GetAt(nImpactedAttribute));

		// Destruction si necessaire, avec son contenu
		if (nkdAttributeImpactedPartMerges != NULL)
		{
			nkdAttributeImpactedPartMerges->DeleteAll();
			delete nkdAttributeImpactedPartMerges;
		}
	}

	// Destruction du tableau global
	delete oaImpactedPartMerges;
	oaImpactedPartMerges = NULL;
}

boolean KWDGMPartMergeAction::CheckImpactedPartMerges() const
{
	boolean bOk = true;
	int nImpactedAttribute;
	NumericKeyDictionary* nkdAttributeImpactedPartMerges;
	ObjectArray oaAttributeImpactedPartMerges;
	int nPart;
	KWDGMPartMergeArray* partMergeArray;
	int nMerge;
	KWDGMPartMerge* partMerge;
	NumericKeyDictionary nkdAllPartMerges;

	require(IsInitialized());
	require(oaImpactedPartMerges != NULL);

	// Verification des containers par attribut externe
	for (nImpactedAttribute = 0; nImpactedAttribute < oaImpactedPartMerges->GetSize(); nImpactedAttribute++)
	{
		// Acces au container par attribut
		nkdAttributeImpactedPartMerges =
		    cast(NumericKeyDictionary*, oaImpactedPartMerges->GetAt(nImpactedAttribute));

		// Verification si container non vide
		if (nkdAttributeImpactedPartMerges != NULL)
		{
			// Conversion du dictionnaire en tableau
			nkdAttributeImpactedPartMerges->ExportObjectArray(&oaAttributeImpactedPartMerges);

			// Verification des tableaux de fusions impactees
			for (nPart = 0; nPart < oaAttributeImpactedPartMerges.GetSize(); nPart++)
			{
				partMergeArray = cast(KWDGMPartMergeArray*, oaAttributeImpactedPartMerges.GetAt(nPart));

				// Verification du tableau de fusions
				if (not partMergeArray->GetPart1()->Check())
				{
					partMergeArray->AddError("Incorrect impacted merges");
					bOk = false;
					break;
				}

				// Verification de l'unicite globale des fusions impactees
				for (nMerge = 0; nMerge < partMergeArray->GetPartMerges()->GetSize(); nMerge++)
				{
					partMerge =
					    cast(KWDGMPartMerge*, partMergeArray->GetPartMerges()->GetAt(nMerge));

					// Test d'unicite en utilisant un dictionnaire global
					if (nkdAllPartMerges.Lookup((NUMERIC)partMerge) == NULL)
					{
						nkdAllPartMerges.SetAt((NUMERIC)partMerge, partMerge);
					}
					// Erreur si fusion enregistree plus d'une fois
					else
					{
						partMerge->AddError("Merge registered more than once");
						bOk = false;
						break;
					}
				}

				// Arret si erreurs
				if (bOk)
					break;
			}
		}

		// Arret si erreurs
		if (bOk)
			break;
	}
	return bOk;
}

void KWDGMPartMergeAction::WriteImpactedPartMerges(ostream& ost) const
{
	int nImpactedAttribute;
	NumericKeyDictionary* nkdAttributeImpactedPartMerges;
	ObjectArray oaAttributeImpactedPartMerges;
	int nPart;
	KWDGMPartMergeArray* partMergeArray;

	require(IsInitialized());
	require(oaImpactedPartMerges != NULL);

	// Traitement des containers par attribut externe
	for (nImpactedAttribute = 0; nImpactedAttribute < oaImpactedPartMerges->GetSize(); nImpactedAttribute++)
	{
		// Acces au container par attribut
		nkdAttributeImpactedPartMerges =
		    cast(NumericKeyDictionary*, oaImpactedPartMerges->GetAt(nImpactedAttribute));

		// Affichage si container non vide
		if (nkdAttributeImpactedPartMerges != NULL)
		{
			// Conversion du dictionnaire en tableau
			nkdAttributeImpactedPartMerges->ExportObjectArray(&oaAttributeImpactedPartMerges);

			// Affichage des tableaux de fusions impactees
			for (nPart = 0; nPart < oaAttributeImpactedPartMerges.GetSize(); nPart++)
			{
				partMergeArray = cast(KWDGMPartMergeArray*, oaAttributeImpactedPartMerges.GetAt(nPart));
				ost << *partMergeArray;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMPartMergeArray

boolean KWDGMPartMergeArray::Check() const
{
	boolean bOk = true;
	int nPartMerge;
	KWDGMPartMerge* partMerge;

	// Verification de la partie origine
	bOk = part1 != NULL and part1->GetAttribute() != NULL;

	// Verification des parties destination
	for (nPartMerge = 0; nPartMerge < oaPartMerges.GetSize(); nPartMerge++)
	{
		partMerge = cast(KWDGMPartMerge*, oaPartMerges.GetAt(nPartMerge));

		// Verification de la partie destination
		bOk = bOk and partMerge->Check();
		bOk = bOk and (partMerge->GetPart1() == part1 or partMerge->GetPart2() == part1);
	}
	return bOk;
}

void KWDGMPartMergeArray::Write(ostream& ost) const
{
	ALString sPartLabel;
	int nPartMerge;
	KWDGMPartMerge* partMerge;

	// Calcul d'un libelle constant pour la partie origine
	sPartLabel = "Merge\t";
	if (part1 != NULL and part1->GetAttribute() != NULL)
		sPartLabel += part1->GetAttribute()->GetObjectLabel() + "(" + part1->GetObjectLabel() + ")";

	// Merges
	for (nPartMerge = 0; nPartMerge < oaPartMerges.GetSize(); nPartMerge++)
	{
		partMerge = cast(KWDGMPartMerge*, oaPartMerges.GetAt(nPartMerge));
		ost << sPartLabel << "\t" << partMerge->GetObjectLabel() << "\n";
	}
}

const ALString KWDGMPartMergeArray::GetClassLabel() const
{
	return "Merge impact";
}

const ALString KWDGMPartMergeArray::GetObjectLabel() const
{
	ALString sLabel;

	// Nom de l'attribut
	if (part1 != NULL and part1->GetAttribute() != NULL)
		sLabel = part1->GetAttribute()->GetObjectLabel();

	// Nom de la premiere partie
	sLabel += "(";
	if (part1 != NULL)
		sLabel += part1->GetObjectLabel();
	sLabel += ")";

	// Nombre de fusions impactees
	sLabel += ":";
	sLabel += IntToString(oaPartMerges.GetSize());

	return sLabel;
}
