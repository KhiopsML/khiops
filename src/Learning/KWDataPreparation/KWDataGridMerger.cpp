// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridMerger.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridMerger

KWDataGridMerger::KWDataGridMerger()
{
	nMaxPartNumber = 0;
	nCellDictionaryCount = 0;
	dCost = 0;
	dataGridCosts = NULL;
	dAllPartsTargetDeltaCost = 0;
	dEpsilon = 1e-6;
}

KWDataGridMerger::~KWDataGridMerger() {}

int KWDataGridMerger::GetMaxPartNumber() const
{
	return nMaxPartNumber;
}

void KWDataGridMerger::SetMaxPartNumber(int nValue)
{
	require(nValue >= 0);
	nMaxPartNumber = nValue;
}

double KWDataGridMerger::Merge()
{
	double dDataGridCost;

	require(Check());
	require(GetDataGridCosts() != NULL);

	// Optimisation standard
	dDataGridCost = OptimizeMerge();
	assert(fabs(dDataGridCost - GetDataGridCosts()->ComputeDataGridMergerTotalCost(this)) < dEpsilon);
	return dDataGridCost;
}

void KWDataGridMerger::InitializeAllCosts()
{
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGPart* part;
	KWDGMPart* partM;
	KWDGCell* cell;
	KWDGMCell* cellM;

	require(GetDataGridCosts() != NULL);

	// Initialisation du cout du DataGrid
	SetCost(GetDataGridCosts()->ComputeDataGridCost(this, GetLnGridSize(), GetInformativeAttributeNumber()));

	// Prise en compte des cellules
	cell = GetHeadCell();
	while (cell != NULL)
	{
		cellM = cast(KWDGMCell*, cell);
		cellM->SetCost(GetDataGridCosts()->ComputeCellCost(cellM));
		GetNextCell(cell);
	}

	// Prise en compte des attributs
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Cout de l'attribut
		attributeM->SetCost(GetDataGridCosts()->ComputeAttributeCost(attributeM, attributeM->GetPartNumber()));

		// Cout des parties
		// Cout local et global des parties (les cellules sont deja evalues)
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			partM = cast(KWDGMPart*, part);
			partM->SetCost(GetDataGridCosts()->ComputePartCost(partM));
			attribute->GetNextPart(part);
		}
	}
}

boolean KWDataGridMerger::CheckAllCosts() const
{
	boolean bOk = true;
	ALString sTmp;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGPart* part;
	KWDGMPart* partM;
	KWDGCell* cell;
	KWDGMCell* cellM;

	require(GetDataGridCosts() != NULL);

	// Initialisation du cout du DataGrid
	if (GetCost() !=
	    GetDataGridCosts()->ComputeDataGridCost(this, GetLnGridSize(), GetInformativeAttributeNumber()))
	{
		AddError(sTmp + "Data grid cost (" + DoubleToString(GetCost()) + ") is incorrect");
		bOk = false;
	}

	// Prise en compte des cellules
	cell = GetHeadCell();
	while (cell != NULL)
	{
		cellM = cast(KWDGMCell*, cell);
		if (cellM->GetCost() != GetDataGridCosts()->ComputeCellCost(cellM))
		{
			cellM->AddError(sTmp + "Cell cost (" + DoubleToString(cellM->GetCost()) + ") is incorrect");
			bOk = false;
			break;
		}
		GetNextCell(cell);
	}

	// Prise en compte des attributs
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Cout de l'attribut
		if (attributeM->GetCost() !=
		    GetDataGridCosts()->ComputeAttributeCost(attributeM, attributeM->GetPartNumber()))
		{
			attributeM->AddError(sTmp + "Variable cost (" + DoubleToString(attributeM->GetCost()) +
					     ") is incorrect");
			bOk = false;
		}

		// Cout des parties
		// Cout local et global des parties (les cellules sont deja evalues)
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			partM = cast(KWDGMPart*, part);
			if (partM->GetCost() != GetDataGridCosts()->ComputePartCost(partM))
			{
				partM->AddError(sTmp + "Part cost (" + DoubleToString(partM->GetCost()) +
						") is incorrect");
				bOk = false;
				break;
			}
			attribute->GetNextPart(part);
		}

		// Arret si erreurs
		if (not bOk)
			break;
	}
	return bOk;
}

longint KWDataGridMerger::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDataGrid::GetUsedMemory();
	lUsedMemory += sizeof(KWDataGridMerger) - sizeof(KWDataGrid);
	lUsedMemory += oaCellDictionary.GetUsedMemory();
	return lUsedMemory;
}

KWDGAttribute* KWDataGridMerger::NewAttribute() const
{
	return new KWDGMAttribute;
}

KWDGCell* KWDataGridMerger::NewCell() const
{
	KWDGMCell* cell;

	// Creation de la cellule
	cell = new KWDGMCell;

	// Initialisation du vecteur des effectifs des valeurs cibles
	cell->ivFrequencyVector.SetSize(GetTargetValueNumber());
	return cell;
}

double KWDataGridMerger::OptimizeMerge()
{
	boolean bDisplayAllForDebug = false;
	boolean bDisplayMergeDetails = false;
	boolean bDisplayDataGrid = false;
	boolean bRecomputeAllAtEachStep = false;
	boolean bExhaustiveSearch = true;
	KWDataGridManager dataGridManager;
	KWDataGrid* optimizedDataGrid;
	double dDataGridTotalCost;
	double dBestDataGridTotalCost;
	double dBestDeltaCost;
	boolean bIsBestSolutionValid;
	boolean bIsSolutionValid;
	KWDGMPartMerge* bestPartMerge;
	boolean bContinue;
	int nCount;
	int nTotalExpectedMergeNumber;
	ALString sTaskLabel;

	// Messsage de depart
	if (bDisplayAllForDebug)
		cout << "Merge of data grid\t" << GetObjectLabel() << endl;

	// Debut de tache
	TaskProgression::BeginTask();
	sTaskLabel = "Data grid optimization";
	TaskProgression::DisplayMainLabel(sTaskLabel);

	// Initialisation des couts des entites du DataGrid
	InitializeAllCosts();
	dDataGridTotalCost = GetDataGridCosts()->ComputeDataGridMergerTotalCost(this);
	dBestDataGridTotalCost = dDataGridTotalCost;

	// Initialisation de la gestion de la contrainte sur le nombre max de partie par attribut
	bIsSolutionValid = true;
	if (GetMaxPartNumber() > 0)
		bIsSolutionValid = ComputeMaxPartNumber() <= GetMaxPartNumber();
	bIsBestSolutionValid = bIsSolutionValid;

	// Initialisation de la table de hash des cellules
	CellDictionaryInit();

	// Initialisation de toutes les fusions
	InitializeAllPartMerges();
	assert(CheckAllPartMerges());

	// Initialisation de la liste des parties triees par nombre de modalites
	InitializeAllPartLists();

	// Messsage sur les cout initiaux
	if (bDisplayAllForDebug)
	{
		cout << "Initial Cost\t" << GetDataGridCosts()->ComputeDataGridMergerTotalCost(this) << endl;
		GetDataGridCosts()->WriteDataGridAllCosts(this, cout);
		CellDictionaryWrite(cout);
		WriteAllPartMerges(cout);
	}

	// Affichage grille initiale
	if (bDisplayDataGrid)
	{
		cout << "Debut de OptimizeMerge" << endl;
		this->Write(cout);
	}

	// Boucle de recherche d'ameliorations
	nCount = 0;
	bContinue = true;
	optimizedDataGrid = NULL;
	nTotalExpectedMergeNumber = GetTotalPartNumber() - GetAttributeNumber();
	while (bContinue)
	{
		nCount++;

		// Niveau d'avancement de la tache
		if (TaskProgression::IsInterruptionRequested())
			break;
		TaskProgression::DisplayProgression((nCount * 100) / (nTotalExpectedMergeNumber + 1));
		TaskProgression::DisplayMainLabel(sTaskLabel);

		// Recalcul des structures internes complete du merger a chaque etape d'optimisation (pour raisons de
		// debug)
		if (bRecomputeAllAtEachStep)
		{
			CellDictionaryRemoveAll();
			DeleteAllPartMerges();
			InitializeAllCosts();
			CellDictionaryInit();
			InitializeAllPartMerges();
			InitializeAllPartLists();
		}
		assert(CheckAllPartMerges());

		// Affichage du contenu du merger a chaque etape d'optimisation
		if (bDisplayAllForDebug)
		{
			cout << "DataGrid costs\n";
			GetDataGridCosts()->WriteDataGridAllCosts(this, cout);
			cout << "All part merges\n";
			WriteAllPartMerges(cout);
		}

		// Recherche de la meilleure amelioration
		dBestDeltaCost = SearchBestPartMergeWithGarbageSearch(bestPartMerge);
		// CH Fin Lot7
		bContinue = (bestPartMerge != NULL);
		assert(bContinue or dBestDeltaCost == DBL_MAX);
		if (not bExhaustiveSearch)
			bContinue = bContinue and dBestDeltaCost <= dEpsilon;

		// Memorisation de la meilleure solution globale si necessaire
		// (quand le cout remonte juste apres un minimum local,
		// ou si c'est la derniere grille qui est optimale)
		if (dBestDeltaCost > dEpsilon and fabs(dBestDataGridTotalCost - dDataGridTotalCost) <= dEpsilon)
		{
			if (optimizedDataGrid != NULL)
				delete optimizedDataGrid;
			optimizedDataGrid = new KWDataGrid;
			dataGridManager.CopyInformativeDataGrid(this, optimizedDataGrid);
		}

		// Impact de la meilleure amelioration
		if (bContinue)
		{
			// Affichage des resultats de l'etape d'optimisation
			if (bDisplayMergeDetails)
			{
				cout << "Best part Merge\t" << nCount << "\t" << dBestDeltaCost << "\t"
				     << dDataGridTotalCost + dBestDeltaCost << "\t" << *bestPartMerge << flush;
			}

			// Realisation de la fusion
			PerformPartMerge(bestPartMerge);
			dDataGridTotalCost += dBestDeltaCost;

			// Gestion de la contrainte sur le nombre max de partie par attribut
			if (GetMaxPartNumber() > 0)
			{
				// On ne reevalue la contrainte que si la meilleure solution ne la respecte pas
				// car le max du nombre de partie ne fait que diminuer lors des etapes de fusions de
				// partie
				assert(not bIsBestSolutionValid or ComputeMaxPartNumber() <= GetMaxPartNumber());
				bIsSolutionValid = bIsBestSolutionValid or ComputeMaxPartNumber() <= GetMaxPartNumber();
			}

			// Memorisation du meilleur cout si amelioration ou si la solution passe de non valide a valide
			if ((dDataGridTotalCost < dBestDataGridTotalCost + dEpsilon) or
			    (not bIsBestSolutionValid and bIsSolutionValid))
			{
				dBestDataGridTotalCost = dDataGridTotalCost;
				bIsBestSolutionValid = bIsSolutionValid;
			}

			// Verification de la mise a jour correcte des structures et des couts
			assert(Check());
			assert(CheckAllPartMerges());

			// Affichage grille apres PerformPartMerge
			if (bDisplayDataGrid)
			{
				cout << "Apres PerformPartMerge " << nCount << "\t dDataGridTotalCost \t"
				     << dDataGridTotalCost << endl;
				this->Write(cout);
				cout << "Tous les couts de la grille" << endl;
				GetDataGridCosts()->WriteDataGridAllCosts(this, cout);
			}
		}
	}

	// Nettoyage
	CellDictionaryRemoveAll();
	DeleteAllPartMerges();

	// On restaure la meilleure solution rencontree lors de l'optimisation
	if (optimizedDataGrid != NULL)
	{
		dataGridManager.CopyInformativeDataGrid(optimizedDataGrid, this);
		InitializeAllCosts();
		delete optimizedDataGrid;
		optimizedDataGrid = NULL;
	}

	// Affichage grille apres PerformPartMerge
	if (bDisplayDataGrid)
	{
		cout << "Apres restauration de la meilleure solution " << nCount << endl;
		this->Write(cout);
	}

	// Fin de tache
	TaskProgression::EndTask();

	// Messsage de fin
	if (bDisplayAllForDebug)
	{
		Write(cout);
		cout << endl;
		cout << "BestDataGridTotalCost\t" << dBestDataGridTotalCost << endl;
		cout << "Final Cost\t" << GetDataGridCosts()->ComputeDataGridMergerTotalCost(this) << endl;
		GetDataGridCosts()->WriteDataGridAllCosts(this, cout);
		cout << endl;
	}
	ensure(Check());
	ensure(fabs(dBestDataGridTotalCost - GetDataGridCosts()->ComputeDataGridMergerTotalCost(this)) < dEpsilon);
	return dBestDataGridTotalCost;
}

void KWDataGridMerger::InitializeAllPartLists()
{
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGPart* part;
	boolean bDisplayResults = false;

	// Parcours des attributs du DataGrid
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Cas d'un attribut categoriel (donc eligible au groupe poubelle)
		if (attribute->GetAttributeType() == KWType::Symbol)
		{
			// Parcours des parties de l'attribut
			part = attribute->GetHeadPart();
			while (part != NULL)
			{
				// Ajout de la partie dans la liste triee de l'attribut
				attributeM->AddPartToValueNumberList(cast(KWDGMPart*, part));
				attribute->GetNextPart(part);
			}
			if (bDisplayResults)
			{
				cout << " KWDataGridMerger::InitializeAllPartLists() " << endl;
				cout << " Attribute\t " << attributeM->GetAttributeName() << endl;
				cout << *(attributeM->slPartValueNumbers) << endl;
			}
		}
	}
}

void KWDataGridMerger::RemoveAllPartLists()
{
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;

	// Parcours des attributs du DataGrid
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Destruction des fusions de l'attribut
		attributeM->RemoveAllPartsFromValueNumberList();
	}
}
void KWDataGridMerger::InitializeAllPartMerges()
{
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	ObjectArray oaParts;
	int nPart;
	int nPart1;
	int nPart2;
	KWDGPart* part;
	KWDGMPart* partM1;
	KWDGMPart* partM2;
	KWDGMPartMerge* partMerge;

	// Calcul de l'impact sur toutes les parties sources d'une grille de la decrementation
	// du nombre de partie cibles
	dAllPartsTargetDeltaCost = ComputeAllPartsTargetDeltaCost();

	// Parcours des attributs du DataGrid
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Rangement des parties dans un tableau
		oaParts.SetSize(attribute->GetPartNumber());
		nPart = 0;
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			oaParts.SetAt(nPart, part);
			nPart++;
			attribute->GetNextPart(part);
		}

		// Creation des fusions possibles de parties pour un attribut continu
		if (attribute->GetAttributeType() == KWType::Continuous)
		{
			// Tri des intervalles
			oaParts.SetCompareFunction(KWDGPartContinuousCompare);
			oaParts.Sort();

			// Creation des fusions entre intervalles adjacents
			for (nPart = 1; nPart < oaParts.GetSize(); nPart++)
			{
				// Acces aux deux parties a fusionner
				partM1 = cast(KWDGMPart*, oaParts.GetAt(nPart - 1));
				partM2 = cast(KWDGMPart*, oaParts.GetAt(nPart));

				// Creation et initialisation de la fusion
				partMerge = new KWDGMPartMerge;
				partMerge->SetPart1(partM1);
				partMerge->SetPart2(partM2);
				assert(partMerge->Check());

				// Evaluation de la fusion
				partMerge->SetMergeCost(ComputeMergeCost(partMerge));

				// Memorisation de la fusion dans les parties
				partM1->AddPartMerge(partMerge);
				partM2->AddPartMerge(partMerge);

				// Memorisation dans l'attribut
				attributeM->AddPartMerge(partMerge);
			}
		}
		// Creation des fusions possible de partie pour un attribut symbolique
		else
		{
			// Tri des parties (pour des raisons d'affichage uniquement)
			oaParts.SetCompareFunction(KWDGPartSymbolCompare);
			oaParts.Sort();

			// Creation des fusions entre toutes les paires de parties
			for (nPart1 = 0; nPart1 < oaParts.GetSize(); nPart1++)
			{
				for (nPart2 = nPart1 + 1; nPart2 < oaParts.GetSize(); nPart2++)
				{
					// Acces aux deux parties a fusionner
					partM1 = cast(KWDGMPart*, oaParts.GetAt(nPart1));
					partM2 = cast(KWDGMPart*, oaParts.GetAt(nPart2));

					// Creation et initialisation de la fusion
					partMerge = new KWDGMPartMerge;
					partMerge->SetPart1(partM1);
					partMerge->SetPart2(partM2);
					assert(partMerge->Check());

					// Evaluation de la fusion
					partMerge->SetMergeCost(ComputeMergeCost(partMerge));

					// Memorisation de la fusion dans les parties
					partM1->AddPartMerge(partMerge);
					partM2->AddPartMerge(partMerge);

					// Memorisation dans l'attribut
					attributeM->AddPartMerge(partMerge);
				}
			}
		}
	}
}

void KWDataGridMerger::DeleteAllPartMerges()
{
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGPart* part;
	KWDGMPart* partM;

	// Parcours des attributs du DataGrid
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Suppression des fusions references dans les parties
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			partM = cast(KWDGMPart*, part);
			partM->RemoveAllPartMerges();
			attribute->GetNextPart(part);
		}

		// Destruction des fusions de l'attribut
		attributeM->DeleteAllPartMerges();
	}
}

void KWDataGridMerger::WriteAllPartMerges(ostream& ost) const
{
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGMPartMerge* partMerge;
	POSITION position;
	boolean bWriteHeader;

	// Parcours des attributs du DataGrid
	bWriteHeader = true;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Affichage des fusions de parties pour l'attribut
		position = attributeM->slPartMerges->GetHeadPosition();
		while (position != NULL)
		{
			partMerge = cast(KWDGMPartMerge*, attributeM->slPartMerges->GetNext(position));
			if (bWriteHeader)
			{
				partMerge->WriteHeaderLine(ost);
				bWriteHeader = false;
			}
			ost << *partMerge;
		}
	}
}

boolean KWDataGridMerger::CheckAllPartMerges() const
{
	boolean bOk = true;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGMPartMerge* partMerge;
	POSITION position;
	ALString sTmp;

	// Verification des couts des composantes du DataGrid
	bOk = CheckAllCosts();

	// Verification de l'impact sur toutes les parties sources d'une grille de la decrementation
	// du nombre de partie cibles
	if (fabs(dAllPartsTargetDeltaCost - ComputeAllPartsTargetDeltaCost()) > dEpsilon)
	{
		bOk = false;
		AddError(sTmp + "Bad impact of the decrementation of the number of target parts (" +
			     DoubleToString(dAllPartsTargetDeltaCost) = ")");
	}

	// Parcours des attributs du DataGrid
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Verification du nombre de fusions de l'attribut selon son type
		if (attribute->GetAttributeType() == KWType::Continuous)
		{
			if (attributeM->slPartMerges->GetCount() != attributeM->GetPartNumber() - 1)
			{
				bOk = false;
				attributeM->AddError(sTmp + "Bad number of merges: " +
						     IntToString(attributeM->slPartMerges->GetCount()) + " for " +
						     IntToString(attributeM->GetPartNumber()) + " parts");
			}
		}
		else
		{
			if (attributeM->slPartMerges->GetCount() !=
			    attributeM->GetPartNumber() * (attributeM->GetPartNumber() - 1) / 2)
			{
				bOk = false;
				attributeM->AddError(sTmp + "Bad number of merges: " +
						     IntToString(attributeM->slPartMerges->GetCount()) + " for " +
						     IntToString(attributeM->GetPartNumber()) + " parts");
			}
		}

		// Verification des fusions de parties pour l'attribut
		position = attributeM->slPartMerges->GetHeadPosition();
		while (position != NULL)
		{
			partMerge = cast(KWDGMPartMerge*, attributeM->slPartMerges->GetNext(position));

			// Verification de la fusion
			if (not partMerge->Check())
			{
				bOk = false;
				partMerge->AddError("Incorrect merge");
			}
			if (bOk and partMerge->GetPart1()->GetAttribute() != attributeM)
			{
				bOk = false;
				partMerge->AddError("Incorrect merge for variable " + attributeM->GetAttributeName());
			}

			// Verification de la coherence de la fusion dans le cas continu
			if (bOk and attribute->GetAttributeType() == KWType::Continuous)
			{
				if (partMerge->GetPart1()->GetInterval()->GetUpperBound() !=
					partMerge->GetPart2()->GetInterval()->GetLowerBound() and
				    partMerge->GetPart2()->GetInterval()->GetUpperBound() !=
					partMerge->GetPart1()->GetInterval()->GetLowerBound())
				{
					bOk = false;
					partMerge->AddError("Merge between non adjacent intervals");
				}
			}

			// Verification du referencement de la fusion par ses parties extremites
			if (bOk and partMerge->GetPart1()->LookupPartMerge(partMerge->GetPart2()) != partMerge)
			{
				bOk = false;
				partMerge->AddError("Merge not referenced from the part " +
						    partMerge->GetPart1()->GetObjectLabel());
			}
			if (bOk and partMerge->GetPart2()->LookupPartMerge(partMerge->GetPart1()) != partMerge)
			{
				bOk = false;
				partMerge->AddError("Merge not referenced from the part " +
						    partMerge->GetPart2()->GetObjectLabel());
			}

			// Verification du cout de la fusion
			if (bOk and fabs(partMerge->GetMergeCost() - ComputeMergeCost(partMerge)) > dEpsilon)
			{
				bOk = false;
				partMerge->AddError(sTmp + "Merge cost (" + DoubleToString(partMerge->GetMergeCost()) +
						    ") different from its true cost (" +
						    DoubleToString(ComputeMergeCost(partMerge)) + ")");
			}

			// Arret si erreurs
			if (not bOk)
				break;
		}

		// Arret si erreurs
		if (not bOk)
			break;
	}
	return bOk;
}

double KWDataGridMerger::SearchBestPartMerge(KWDGMPartMerge*& bestPartMerge)
{
	boolean bDisplayDetails = false;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGMPartMerge* partMerge;
	double dNewLnGridSize;
	int nNewInformativeAttributeNumber;
	double dGridDeltaCost;
	double dAttributeDeltaCost;
	double dDeltaCost;
	double dBestDeltaCost;

	// Parcours des attribut pour rechercher la meilleure fusion
	dBestDeltaCost = DBL_MAX;
	bestPartMerge = NULL;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Recherche de la meilleure fusion de l'attribut
		partMerge = attributeM->GetBestPartMerge();

		// Test d'amelioration si existence d'une fusion
		dDeltaCost = 0;
		if (partMerge != NULL)
		{
			// Variation de cout locale a la grille
			assert(GetCost() == GetDataGridCosts()->ComputeDataGridCost(this, GetLnGridSize(),
										    GetInformativeAttributeNumber()));
			dNewLnGridSize = GetLnGridSize() + log(double(attribute->GetPartNumber() - 1)) -
					 log(double(attribute->GetPartNumber()));
			if (dNewLnGridSize < dEpsilon)
				dNewLnGridSize = 0;
			nNewInformativeAttributeNumber = GetInformativeAttributeNumber();
			if (attribute->GetPartNumber() == 2)
				nNewInformativeAttributeNumber--;
			dGridDeltaCost = GetDataGridCosts()->ComputeDataGridCost(this, dNewLnGridSize,
										 nNewInformativeAttributeNumber) -
					 GetCost();

			// Variation de cout locale a l'attribut
			assert(attributeM->GetCost() ==
			       GetDataGridCosts()->ComputeAttributeCost(attributeM, attributeM->GetPartNumber()));
			dAttributeDeltaCost =
			    GetDataGridCosts()->ComputeAttributeCost(attributeM, attributeM->GetPartNumber() - 1) -
			    attributeM->GetCost();

			// Cout total de l'amelioration
			dDeltaCost = dGridDeltaCost + dAttributeDeltaCost + partMerge->GetMergeCost();

			// Prise en compte d'un cout non additif supplementaire dans le cas de l'attribut cible
			if (attribute->GetAttributeTargetFunction())
				dDeltaCost += dAllPartsTargetDeltaCost;

			// Affichage des details
			if (bDisplayDetails)
			{
				cout << endl;
				cout << "InformativeAttribute\t" << GetInformativeAttributeNumber() << endl;
				cout << "Attribute\t" << attribute->GetAttributeName() << endl;
				cout << "PartNumber\t" << attribute->GetPartNumber() << endl;
				cout << "GridSize\t" << exp(GetLnGridSize()) << endl;
				cout << "NewGridSize\t" << exp(dNewLnGridSize) << endl;
				cout << "DeltaCost\t" << dDeltaCost << endl;
				cout << "GridCost\t" << GetCost() << endl;
				cout << "NewGridCost\t"
				     << GetDataGridCosts()->ComputeDataGridCost(this, dNewLnGridSize,
										nNewInformativeAttributeNumber)
				     << endl;
				cout << "GridDeltaCost\t" << dGridDeltaCost << endl;
				cout << "AttributeDeltaCost\t" << dAttributeDeltaCost << endl;
				cout << "PartMergeCost\t" << partMerge->GetMergeCost() << endl;
				cout << "Part1Cost\t" << partMerge->GetPart1()->GetCost() << endl;
				cout << "Part2Cost\t" << partMerge->GetPart2()->GetCost() << endl;
				cout << *partMerge << endl
				     << *(partMerge->GetPart1()) << endl
				     << *(partMerge->GetPart2()) << endl;
			}

			// Test d'amelioration
			if (dDeltaCost <= dBestDeltaCost + dEpsilon)
			{
				dBestDeltaCost = dDeltaCost;
				bestPartMerge = partMerge;
			}
		}
	}
	assert(bestPartMerge == NULL or dBestDeltaCost < DBL_MAX);
	return dBestDeltaCost;
}

double KWDataGridMerger::SearchBestPartMergeWithGarbageSearch(KWDGMPartMerge*& bestPartMerge)
{
	boolean bDisplayDetails = false;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGMPartMerge* partMerge;
	double dNewLnGridSize;
	int nNewInformativeAttributeNumber;
	double dGridDeltaCost;
	double dAttributeDeltaCost;
	KWDGPart* garbagePart;
	double dDeltaCostWithoutGarbage;
	double dDeltaCostWithGarbage;
	double dBestDeltaCost;
	boolean bGarbageBeforeMerge;

	// Parcours des attribut pour rechercher la meilleure fusion
	dBestDeltaCost = DBL_MAX;
	bestPartMerge = NULL;
	bGarbageBeforeMerge = false;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Recherche de la meilleure fusion de l'attribut
		partMerge = attributeM->GetBestPartMerge();

		// Test d'amelioration si existence d'une fusion
		dDeltaCostWithGarbage = 0;
		dDeltaCostWithoutGarbage = 0;
		if (partMerge != NULL)
		{
			// Variation de cout locale a la grille
			assert(GetCost() == GetDataGridCosts()->ComputeDataGridCost(this, GetLnGridSize(),
										    GetInformativeAttributeNumber()));
			dNewLnGridSize = GetLnGridSize() + log(double(attribute->GetPartNumber() - 1)) -
					 log(double(attribute->GetPartNumber()));
			if (dNewLnGridSize < dEpsilon)
				dNewLnGridSize = 0;
			nNewInformativeAttributeNumber = GetInformativeAttributeNumber();
			if (attribute->GetPartNumber() == 2)
				nNewInformativeAttributeNumber--;
			dGridDeltaCost = GetDataGridCosts()->ComputeDataGridCost(this, dNewLnGridSize,
										 nNewInformativeAttributeNumber) -
					 GetCost();

			// Cas d'un attribut categoriel pour lequel un groupe poubelle est envisageable
			if (attribute->GetAttributeType() == KWType::Symbol)
			{
				// Memorisation de la presence d'un groupe poubelle pour restitution ulterieure
				bGarbageBeforeMerge = attributeM->GetGarbageModalityNumber() > 0;

				// Cas d'un attribut sans groupe poubelle apres fusion
				partMerge->SetGarbagePresence(false);
				attributeM->SetGarbagePart(NULL);
			}

			// Variation de cout locale a l'attribut
			dAttributeDeltaCost =
			    GetDataGridCosts()->ComputeAttributeCost(attributeM, attributeM->GetPartNumber() - 1) -
			    attributeM->GetCost();

			// Cout total de l'amelioration pour une partition sans groupe poubelle
			dDeltaCostWithoutGarbage = dGridDeltaCost + dAttributeDeltaCost + partMerge->GetMergeCost();

			// Prise en compte d'un cout non additif supplementaire dans le cas de l'attribut cible
			if (attribute->GetAttributeTargetFunction())
				dDeltaCostWithoutGarbage += dAllPartsTargetDeltaCost;

			// Affichage des details
			if (bDisplayDetails)
			{
				cout << endl;
				cout << "InformativeAttribute\t" << GetInformativeAttributeNumber() << endl;
				cout << "Attribute\t" << attribute->GetAttributeName() << endl;
				cout << "PartNumber\t" << attribute->GetPartNumber() << endl;
				cout << "GridSize\t" << exp(GetLnGridSize()) << endl;
				cout << "NewGridSize\t" << exp(dNewLnGridSize) << endl;
				cout << "DeltaCost\t" << dDeltaCostWithoutGarbage << endl;
				cout << "GridCost\t" << GetCost() << endl;
				cout << "NewGridCost\t"
				     << GetDataGridCosts()->ComputeDataGridCost(this, dNewLnGridSize,
										nNewInformativeAttributeNumber)
				     << endl;
				cout << "GridDeltaCost\t" << dGridDeltaCost << endl;
				cout << "AttributeDeltaCost\t" << dAttributeDeltaCost << endl;
				cout << "PartMergeCost\t" << partMerge->GetMergeCost() << endl;
				cout << "Part1Cost\t" << partMerge->GetPart1()->GetCost() << endl;
				cout << "Part2Cost\t" << partMerge->GetPart2()->GetCost() << endl;
				cout << *partMerge << endl
				     << *(partMerge->GetPart1()) << endl
				     << *(partMerge->GetPart2()) << endl;
			}

			// Test d'amelioration
			if (dDeltaCostWithoutGarbage <= dBestDeltaCost + dEpsilon)
			{
				dBestDeltaCost = dDeltaCostWithoutGarbage;
				bestPartMerge = partMerge;
			}

			// Cas d'un attribut categoriel avec un nombre de parties apres fusion est >= 3 (sinon il ne
			// peut pas y avoir de groupe poubelle)
			if (attribute->GetAttributeType() == KWType::Symbol and attributeM->GetPartNumber() - 1 >= 3)
			{
				partMerge->SetGarbagePresence(true);
				garbagePart = NULL;

				// Cas d'un attribut AVEC groupe poubelle apres fusion
				// Cas ou les deux parties fusionnees deviennent le groupe poubelle
				if (partMerge->GetPart1()->GetValueSet()->GetTrueValueNumber() +
					partMerge->GetPart2()->GetValueSet()->GetTrueValueNumber() >
				    cast(KWDGPart*, attributeM->slPartValueNumbers->GetHead())
					->GetValueSet()
					->GetTrueValueNumber())
				{
					// Creation du nouveau groupe poubelle
					garbagePart = new KWDGPart;
					garbagePart->SetPartType(KWType::Symbol);
					// Copie des valeurs de la partie 1 de la fusion
					garbagePart->GetValueSet()->CopyFrom(partMerge->GetPart1()->GetValueSet());
					// Mise a jour avec les valeurs de la partie 2 de la fusion
					garbagePart->GetValueSet()->UpgradeFrom(partMerge->GetPart2()->GetValueSet());

					attributeM->SetGarbagePart(cast(KWDGPart*, garbagePart));
				}
				// Sinon : on garde le meme groupe poubelle
				else
					attributeM->SetGarbagePart(
					    cast(KWDGPart*, attributeM->slPartValueNumbers->GetHead()));

				// Variation de cout locale a l'attribut
				dAttributeDeltaCost = GetDataGridCosts()->ComputeAttributeCost(
							  attributeM, attributeM->GetPartNumber() - 1) -
						      attributeM->GetCost();

				// Cout total de l'amelioration pour une partition sans groupe poubelle
				dDeltaCostWithGarbage =
				    dGridDeltaCost + dAttributeDeltaCost + partMerge->GetMergeCost();

				// Prise en compte d'un cout non additif supplementaire dans le cas de l'attribut cible
				if (attribute->GetAttributeTargetFunction())
					dDeltaCostWithGarbage += dAllPartsTargetDeltaCost;

				// Affichage des details
				if (bDisplayDetails)
				{
					cout << endl;
					cout << "InformativeAttribute\t" << GetInformativeAttributeNumber() << endl;
					cout << "Attribute\t" << attribute->GetAttributeName() << endl;
					cout << "PartNumber\t" << attribute->GetPartNumber() << endl;
					cout << "GridSize\t" << exp(GetLnGridSize()) << endl;
					cout << "NewGridSize\t" << exp(dNewLnGridSize) << endl;
					cout << "DeltaCostWithGarbage\t" << dDeltaCostWithGarbage << endl;
					cout << "GridCost\t" << GetCost() << endl;
					cout << "NewGridCost\t"
					     << GetDataGridCosts()->ComputeDataGridCost(this, dNewLnGridSize,
											nNewInformativeAttributeNumber)
					     << endl;
					cout << "GridDeltaCost\t" << dGridDeltaCost << endl;
					cout << "AttributeDeltaCostWithGarbage\t" << dAttributeDeltaCost << endl;
					cout << "PartMergeCost\t" << partMerge->GetMergeCost() << endl;
					cout << "Part1Cost\t" << partMerge->GetPart1()->GetCost() << endl;
					cout << "Part2Cost\t" << partMerge->GetPart2()->GetCost() << endl;
					cout << *partMerge << endl
					     << *(partMerge->GetPart1()) << endl
					     << *(partMerge->GetPart2()) << endl;
				}

				// Test d'amelioration
				if (dDeltaCostWithGarbage <= dBestDeltaCost + dEpsilon)
				{
					assert(attributeM->GetGarbagePart() != NULL);
					dBestDeltaCost = dDeltaCostWithGarbage;
					bestPartMerge = partMerge;
				}
				else
				// Re-initialisation du partMerge sans poubelle
				{
					partMerge->SetGarbagePresence(false);
					attributeM->SetGarbagePart(NULL);
				}
				// Nettoyage
				if (garbagePart != NULL)
					delete garbagePart;
			}
			if (attribute->GetAttributeType() == KWType::Symbol)
			{
				// Restitution de la poubelle initiale
				if (not bGarbageBeforeMerge)
					attribute->SetGarbagePart(NULL);
				else
					attribute->SetGarbagePart(
					    cast(KWDGPart*, attributeM->slPartValueNumbers->GetHead()));
			}
		}
	}

	if (bDisplayDetails)
		if (bestPartMerge != NULL)
			cout << "Fin de SearchBestPartMergeWithGarbageSearch :" << *bestPartMerge;

	assert(bestPartMerge == NULL or dBestDeltaCost < DBL_MAX);
	return dBestDeltaCost;
}

double KWDataGridMerger::ComputeMergeCost(const KWDGMPartMerge* partMerge) const
{
	KWDGMPart* part1;
	KWDGMPart* part2;
	double dMergeCost;
	KWDGCell* cell1;
	KWDGMCell* cellM1;
	KWDGMCell* cellM2;
	KWDGMCell* evaluationCell;
	double dMergeCellCost;

	require(partMerge != NULL);
	require(partMerge->Check());

	// On recherche la partie ayant le moins de cellule pour la partie origine
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

	// Initialisation du cout de fusion avec les couts par partie
	dMergeCost = GetDataGridCosts()->ComputePartUnionCost(part1, part2) - part1->GetCost() - part2->GetCost();

	// Creation d'une cellule du bon type, pour l'evaluation
	evaluationCell = cast(KWDGMCell*, NewCell());

	// Le cout de fusion est augmente de la somme des cout de fusion des cellules
	// origines entrant en collision avec les cellules destination
	cell1 = part1->GetHeadCell();
	while (cell1 != NULL)
	{
		cellM1 = cast(KWDGMCell*, cell1);

		// Recherche d'une cellule destination en collision avec la cellule origine
		cellM2 = CellDictionaryLookupModifiedCell(cellM1, part2);
		assert(cellM2 == NULL or part2->CheckCell(cellM2));

		// Prise en compte de la cellule si collision
		if (cellM2 != NULL)
		{
			// Calcul du cout de la cellule fusionne
			evaluationCell->MergeFrequenciesFrom(cellM1, cellM2);
			dMergeCellCost = GetDataGridCosts()->ComputeCellCost(evaluationCell);

			// Prise en compte de la difference de cout
			dMergeCost += dMergeCellCost - cellM1->GetCost() - cellM2->GetCost();
		}

		// Cellule suivante
		part1->GetNextCell(cell1);
	}

	// Nettoyage
	delete evaluationCell;

	return dMergeCost;
}

double KWDataGridMerger::ComputeAllPartsTargetDeltaCost() const
{
	double dDeltaCost;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;

	// Variation de cout de toutes les parties sources de la grille
	dDeltaCost = 0;
	if (GetTargetAttribute() != NULL)
	{
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(nAttribute);

			// Cout des parties dans le cas des attributs source
			if (not attribute->GetAttributeTargetFunction())
			{
				part = attribute->GetHeadPart();
				while (part != NULL)
				{
					dDeltaCost += GetDataGridCosts()->ComputePartTargetDeltaCost(part);
					attribute->GetNextPart(part);
				}
			}
		}
	}
	return dDeltaCost;
}

KWDGMPart* KWDataGridMerger::PerformPartMerge(KWDGMPartMerge* bestPartMerge)
{
	KWDGMPartMergeAction partMergeAction;
	boolean bAttributeTargetFunction;
	ObjectArray oaImpactedAttributePartMerges;
	ObjectArray oaImpactedAttributePartMergesPartCosts;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	ObjectArray* oaAllPartMerges;
	DoubleVector* dvAllPartMergesPartCosts;
	double dMergePartCost;
	int nPartMerge;
	KWDGMPartMerge* partMerge;
	KWDGPart* part;
	KWDGMPart* partM;
	KWDGMPart* mergedPart;

	require(bestPartMerge != NULL);

	// Memorisation de la fonction de l'attribut dont dependent les parties fusionnees
	bAttributeTargetFunction = bestPartMerge->GetPart1()->GetAttribute()->GetAttributeTargetFunction();

	// Si attribute cible fusionne, memorisation des impacts sur les fusions d'attributs source
	if (bAttributeTargetFunction)
	{
		// Dimensionnement des tableaux d'impact
		oaImpactedAttributePartMerges.SetSize(GetAttributeNumber());
		oaImpactedAttributePartMergesPartCosts.SetSize(GetAttributeNumber());

		// Parcours des attributs du DataGrid
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(nAttribute);
			attributeM = cast(KWDGMAttribute*, attribute);

			// Cela ne concerne que les attributs source
			if (not attribute->GetAttributeTargetFunction())
			{
				// Rangement des fusions de parties existantes dans un tableau
				oaAllPartMerges = attributeM->GetAllPartMerges();
				oaImpactedAttributePartMerges.SetAt(nAttribute, oaAllPartMerges);

				// Creation d'un vecteur de cout pour la composante "Part" de la fusion
				dvAllPartMergesPartCosts = new DoubleVector;
				dvAllPartMergesPartCosts->SetSize(oaAllPartMerges->GetSize());
				oaImpactedAttributePartMergesPartCosts.SetAt(nAttribute, dvAllPartMergesPartCosts);

				// Parcours des fusions pour memoriser la composante "Part" initiale des couts de fusion
				for (nPartMerge = 0; nPartMerge < oaAllPartMerges->GetSize(); nPartMerge++)
				{
					partMerge = cast(KWDGMPartMerge*, oaAllPartMerges->GetAt(nPartMerge));
					assert(partMerge->Check());

					// Memorisation de cette composante du cout (dans sa version avant fusion)
					dMergePartCost =
					    GetDataGridCosts()->ComputePartUnionCost(partMerge->GetPart1(),
										     partMerge->GetPart2()) -
					    partMerge->GetPart1()->GetCost() - partMerge->GetPart2()->GetCost();
					dvAllPartMergesPartCosts->SetAt(nPartMerge, dMergePartCost);
				}
			}
		}
	}

	// Mise a jour de l'impact sur toutes les parties sources d'une grille de la decrementation
	// du nombre de partie cibles
	if (GetTargetAttribute() != NULL)
	{
		// Reactualisation du cout pour la nouvelle partie issue de la fusion
		if (not bAttributeTargetFunction)
		{
			dAllPartsTargetDeltaCost -=
			    GetDataGridCosts()->ComputePartTargetDeltaCost(bestPartMerge->GetPart1());
			dAllPartsTargetDeltaCost -=
			    GetDataGridCosts()->ComputePartTargetDeltaCost(bestPartMerge->GetPart2());
		}
	}

	// Sous-traitement de cette methode (complexe) a une classe dediee
	// pour tout ce qui concerne la partie addititive standard de la structure de cout
	mergedPart = partMergeAction.PerformPartMerge(bestPartMerge);

	// Si attribute cible fusionne, calcul des impacts sur les fusions d'attributs source
	if (bAttributeTargetFunction)
	{
		// Parcours des attributs du DataGrid
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			attribute = GetAttributeAt(nAttribute);
			attributeM = cast(KWDGMAttribute*, attribute);

			// Cela ne concerne que les attributs source
			if (not attribute->GetAttributeTargetFunction())
			{
				// Recalcul du cout des parties
				part = attribute->GetHeadPart();
				while (part != NULL)
				{
					partM = cast(KWDGMPart*, part);
					partM->SetCost(GetDataGridCosts()->ComputePartCost(partM));
					attribute->GetNextPart(part);
				}

				// Recherche des infos sur les fusions
				oaAllPartMerges = cast(ObjectArray*, oaImpactedAttributePartMerges.GetAt(nAttribute));
				dvAllPartMergesPartCosts =
				    cast(DoubleVector*, oaImpactedAttributePartMergesPartCosts.GetAt(nAttribute));

				// On enleve les fusions de l'attribut, pour reinitialiser la liste triee
				// Attention: les fusion ne sont pas supprimees
				attributeM->RemoveAllPartMerges();

				// Parcours des fusions pour recalculer leur cout, qui a change suite
				// a la modification du nombvre de parties cibles
				for (nPartMerge = 0; nPartMerge < oaAllPartMerges->GetSize(); nPartMerge++)
				{
					partMerge = cast(KWDGMPartMerge*, oaAllPartMerges->GetAt(nPartMerge));
					assert(partMerge->Check());

					// Reevaluation de la fusion
					partMerge->SetPosition(NULL);
					dMergePartCost =
					    GetDataGridCosts()->ComputePartUnionCost(partMerge->GetPart1(),
										     partMerge->GetPart2()) -
					    partMerge->GetPart1()->GetCost() - partMerge->GetPart2()->GetCost();
					partMerge->SetMergeCost(partMerge->GetMergeCost() + dMergePartCost -
								dvAllPartMergesPartCosts->GetAt(nPartMerge));

					// Memorisation dans l'attribut (dans sa liste triee de fusions)
					attributeM->AddPartMerge(partMerge);
				}
			}

			// Nettoyage
			oaImpactedAttributePartMerges.DeleteAll();
			oaImpactedAttributePartMergesPartCosts.DeleteAll();
		}
	}

	// Mise a jour de l'impact sur toutes les parties sources d'une grille de la decrementation
	// du nombre de partie cibles
	if (GetTargetAttribute() != NULL)
	{
		// Recalcul global si fusion de l'attribut cible
		if (bAttributeTargetFunction)
			dAllPartsTargetDeltaCost = ComputeAllPartsTargetDeltaCost();
		// Sinon, reactualisation du cout pour la nouvelle partie issue de la fusion
		else
			dAllPartsTargetDeltaCost += GetDataGridCosts()->ComputePartTargetDeltaCost(mergedPart);
	}

	return mergedPart;
}

//////////////////////////////////////////////////////////////////
// Gestion d'une table de hash dediee aux cellules

// Tableau des tailles d'allocations des dictionnaires
static const UINT nCellDictionaryPrimeSizes[] = {17,       37,       79,        163,       331,      673,      1361,
						 2729,     5471,     10949,     21911,     43853,    87719,    175447,
						 350899,   701819,   1403641,   2807303,   5614657,  11229331, 22458671,
						 44917381, 89834777, 179669557, 359339171, 718678369};
static const int nCellDictionaryPrimeSizeNumber = sizeof(nCellDictionaryPrimeSizes) / sizeof(UINT);

// Rend la taille de table superieure ou egale a une taille donnee
static int CellDictionaryGetNextTableSize(UINT nSize)
{
	int i;
	for (i = 0; i < nCellDictionaryPrimeSizeNumber; i++)
	{
		if (nCellDictionaryPrimeSizes[i] >= nSize)
			return nCellDictionaryPrimeSizes[i];
	}
	return nSize + 1;
}

void KWDataGridMerger::CellDictionaryInit()
{
	int nCellDictionarySize;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGPart* part;
	KWDGMPart* partM;
	KWDGCell* cell;
	KWDGMCell* cellM;
	int nAttributeHash;
	int nAttributeHash2;
	ObjectArray oaParts;
	int nPart;

	// Initialisation de la table de hash
	// Calcul de la taille de la table de hash, suffisante pour accueillir
	// tous les cellules
	// On prend un nombre premier pour minimiser le nombre de collisions
	nCellDictionarySize = CellDictionaryGetNextTableSize(2 * (GetCellNumber() + GetAttributeNumber()));
	oaCellDictionary.SetSize(0);
	oaCellDictionary.SetSize(nCellDictionarySize);
	nCellDictionaryCount = 0;

	////////////////////////////////////////////////////////////
	// Initialisation des cle de hash des entites du DataGrid

	// Initialisation des cles pour les attributs et les parties
	nAttributeHash = 1;
	nAttributeHash2 = 1;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);

		// Cles de hash de l'attribut
		assert(nAttributeHash != 0);
		attributeM->nHash = nAttributeHash;
		assert(nAttributeHash2 != 0);
		attributeM->nHash2 = nAttributeHash2;

		// Preparation des cles de hash pour l'attribut suivant
		nAttributeHash = ComputeProductModulo(nAttributeHash, attribute->GetPartNumber(), nCellDictionarySize);
		nAttributeHash2 = ComputeProductModulo(
		    nAttributeHash2, attribute->GetPartNumber() + 1 + RandomInt(GetCellNumber()), nCellDictionarySize);

		// On range les parties dans un tableau
		oaParts.SetSize(attribute->GetPartNumber());
		nPart = 0;
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			oaParts.SetAt(nPart, part);
			nPart++;
			attribute->GetNextPart(part);
		}

		// On melange les parties pour attribuer les cles de facon aleatoire
		assert(attribute->GetPartNumber() <= nCellDictionarySize);
		oaParts.Shuffle();
		for (nPart = 0; nPart < oaParts.GetSize(); nPart++)
		{
			partM = cast(KWDGMPart*, oaParts.GetAt(nPart));
			partM->nHash = nPart;
		}

		// On recommence pour les deuxieme cles de hash independante des premieres
		oaParts.Shuffle();
		for (nPart = 0; nPart < oaParts.GetSize(); nPart++)
		{
			partM = cast(KWDGMPart*, oaParts.GetAt(nPart));
			partM->nHash2 = nPart;
		}
	}

	// Initialisation des cles de hash des cellules
	// et rangement dans la table de hash
	cell = GetHeadCell();
	while (cell != NULL)
	{
		cellM = cast(KWDGMCell*, cell);

		CellDictionaryAddCell(cellM);

		// Cellule suivante
		GetNextCell(cell);
	}
	assert(nCellDictionaryCount >= GetCellNumber());
}

boolean KWDataGridMerger::CellDictionarySameCellKeys(KWDGMCell* cell1, KWDGMCell* cell2) const
{
	int nAttribute;

	require(cell1 != NULL);
	require(cell2 != NULL);
	require(cell1->GetAttributeNumber() == GetAttributeNumber());
	require(cell2->GetAttributeNumber() == GetAttributeNumber());

	// Deux cellules ont meme cles si toutes leurs parties sont identiques
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		if (cell1->GetPartAt(nAttribute) != cell2->GetPartAt(nAttribute))
			return false;
	}
	return true;
}

KWDGMCell* KWDataGridMerger::CellDictionaryLookupModifiedCell(KWDGMCell* cell, KWDGMPart* modifiedPart) const
{
	KWDGMCell* searchedCell;
	int nModifiedAttribute;
	KWDGMPart* initialPart;
	int nAttributeHash;
	int nAttributeHash2;
	int nPartDeltaHash;
	int nPartDeltaHash2;
	int nModifiedCellHash;
	int nModifiedCellHash2;

	require(cell != NULL);
	require(cell->GetAttributeNumber() == GetAttributeNumber());
	require(GetCellNumber() == nCellDictionaryCount);
	require(modifiedPart != NULL);
	require(modifiedPart->GetAttribute() != NULL);

	// Calcul de l'index de la partie modifie
	nModifiedAttribute = modifiedPart->GetAttribute()->GetAttributeIndex();
	assert(GetAttributeAt(nModifiedAttribute) == modifiedPart->GetAttribute());

	// Modification temporaire de la partie de la cellule
	initialPart = cast(KWDGMPart*, cell->oaParts.GetAt(nModifiedAttribute));
	cell->oaParts.SetAt(nModifiedAttribute, modifiedPart);
	assert(initialPart != modifiedPart);

	// Calcul de la cle de hash de la cellule modifiee
	nAttributeHash = cast(KWDGMAttribute*, GetAttributeAt(nModifiedAttribute))->nHash;
	nModifiedCellHash = cell->nHash;
	nPartDeltaHash =
	    (oaCellDictionary.GetSize() - initialPart->nHash + modifiedPart->nHash) % oaCellDictionary.GetSize();
	nModifiedCellHash += ComputeProductModulo(nAttributeHash, nPartDeltaHash, oaCellDictionary.GetSize());
	nModifiedCellHash %= oaCellDictionary.GetSize();
	assert(nModifiedCellHash >= 0);

	// Calcul de la deuxieme cle de hash de la cellule modifiee
	nAttributeHash2 = cast(KWDGMAttribute*, GetAttributeAt(nModifiedAttribute))->nHash2;
	nModifiedCellHash2 = cell->nHash2;
	nPartDeltaHash2 =
	    (oaCellDictionary.GetSize() - initialPart->nHash2 + modifiedPart->nHash2) % oaCellDictionary.GetSize();
	nModifiedCellHash2 += ComputeProductModulo(nAttributeHash2, nPartDeltaHash2, oaCellDictionary.GetSize());
	nModifiedCellHash2 %= oaCellDictionary.GetSize();

	// Recherche d'une cellule dans le dictionnaire a la position de hash de la cellule modifiee
	searchedCell = cast(KWDGMCell*, oaCellDictionary.GetAt(nModifiedCellHash));

	// Parcours de la liste chainee des cellules a cette position pour
	// retrouver une cellule similaire
	while (searchedCell != NULL)
	{
		assert(searchedCell->nHash == nModifiedCellHash);

		// Le cellule est similaire s'il a meme deuxieme cle de hash
		if (searchedCell->nHash2 == nModifiedCellHash2)
		{
			// et memes parties
			if (CellDictionarySameCellKeys(cell, searchedCell))
				break;
		}

		// Passage a la cellule suivante
		searchedCell = searchedCell->hashNextCell;
	}
	assert(searchedCell == NULL or CellDictionarySameCellKeys(cell, searchedCell));

	// Restitution de la partie initiale de la cellule
	cell->oaParts.SetAt(nModifiedAttribute, initialPart);

	ensure(GetCellNumber() == nCellDictionaryCount);
	return searchedCell;
}

KWDGMCell* KWDataGridMerger::CellDictionaryLookupTwiceModifiedCell(KWDGMCell* cell, KWDGMPart* modifiedPart1,
								   KWDGMPart* modifiedPart2) const
{
	KWDGMCell* searchedCell;
	int nModifiedAttribute1;
	int nModifiedAttribute2;
	KWDGMPart* initialPart1;
	KWDGMPart* initialPart2;
	int nAttribute1Hash;
	int nAttribute1Hash2;
	int nAttribute2Hash;
	int nAttribute2Hash2;
	int nPart1DeltaHash;
	int nPart1DeltaHash2;
	int nPart2DeltaHash;
	int nPart2DeltaHash2;
	int nModifiedCellHash;
	int nModifiedCellHash2;

	require(cell != NULL);
	require(cell->GetAttributeNumber() == GetAttributeNumber());
	require(GetCellNumber() == nCellDictionaryCount);
	require(modifiedPart1 != NULL);
	require(modifiedPart1->GetAttribute() != NULL);
	require(modifiedPart2 != NULL);
	require(modifiedPart2->GetAttribute() != NULL);
	require(modifiedPart1 != modifiedPart2);
	require(modifiedPart1->GetAttribute() != modifiedPart2->GetAttribute());

	// Calcul de l'index des parties modifiees
	nModifiedAttribute1 = modifiedPart1->GetAttribute()->GetAttributeIndex();
	assert(GetAttributeAt(nModifiedAttribute1) == modifiedPart1->GetAttribute());
	nModifiedAttribute2 = modifiedPart2->GetAttribute()->GetAttributeIndex();
	assert(GetAttributeAt(nModifiedAttribute2) == modifiedPart2->GetAttribute());

	// Modification temporaire des parties de la cellule
	initialPart1 = cast(KWDGMPart*, cell->oaParts.GetAt(nModifiedAttribute1));
	cell->oaParts.SetAt(nModifiedAttribute1, modifiedPart1);
	assert(initialPart1 != modifiedPart1);
	initialPart2 = cast(KWDGMPart*, cell->oaParts.GetAt(nModifiedAttribute2));
	cell->oaParts.SetAt(nModifiedAttribute2, modifiedPart2);
	assert(initialPart2 != modifiedPart2);

	// Calcul de la cle de hash de la cellule modifiee
	nAttribute1Hash = cast(KWDGMAttribute*, GetAttributeAt(nModifiedAttribute1))->nHash;
	nAttribute2Hash = cast(KWDGMAttribute*, GetAttributeAt(nModifiedAttribute2))->nHash;
	nModifiedCellHash = cell->nHash;
	nPart1DeltaHash =
	    (oaCellDictionary.GetSize() - initialPart1->nHash + modifiedPart1->nHash) % oaCellDictionary.GetSize();
	nModifiedCellHash += ComputeProductModulo(nAttribute1Hash, nPart1DeltaHash, oaCellDictionary.GetSize());
	nModifiedCellHash %= oaCellDictionary.GetSize();
	nPart2DeltaHash =
	    (oaCellDictionary.GetSize() - initialPart2->nHash + modifiedPart2->nHash) % oaCellDictionary.GetSize();
	nModifiedCellHash += ComputeProductModulo(nAttribute2Hash, nPart2DeltaHash, oaCellDictionary.GetSize());
	nModifiedCellHash %= oaCellDictionary.GetSize();
	assert(nModifiedCellHash >= 0);

	// Calcul de la deuxieme cle de hash de la cellule modifiee
	nAttribute1Hash2 = cast(KWDGMAttribute*, GetAttributeAt(nModifiedAttribute1))->nHash2;
	nAttribute2Hash2 = cast(KWDGMAttribute*, GetAttributeAt(nModifiedAttribute2))->nHash2;
	nModifiedCellHash2 = cell->nHash2;
	nPart1DeltaHash2 =
	    (oaCellDictionary.GetSize() - initialPart1->nHash2 + modifiedPart1->nHash2) % oaCellDictionary.GetSize();
	nModifiedCellHash2 += ComputeProductModulo(nAttribute1Hash2, nPart1DeltaHash2, oaCellDictionary.GetSize());
	nModifiedCellHash2 %= oaCellDictionary.GetSize();
	nPart2DeltaHash2 =
	    (oaCellDictionary.GetSize() - initialPart2->nHash2 + modifiedPart2->nHash2) % oaCellDictionary.GetSize();
	nModifiedCellHash2 += ComputeProductModulo(nAttribute2Hash2, nPart2DeltaHash2, oaCellDictionary.GetSize());
	nModifiedCellHash2 %= oaCellDictionary.GetSize();
	assert(nModifiedCellHash2 >= 0);

	// Recherche d'une cellule dans le dictionnaire a la position de hash de la cellule modifiee
	searchedCell = cast(KWDGMCell*, oaCellDictionary.GetAt(nModifiedCellHash));

	// Parcours de la liste chainee des cellules a cette position pour
	// retrouver une cellule similaire
	while (searchedCell != NULL)
	{
		assert(searchedCell->nHash == nModifiedCellHash);

		// Attention: la cellule recherchee peut avoir par hasard les meme cle de hashage
		// que la cellule originelle (temporellement modifiee)
		// On ne doit pas en tenir compte dans ce cas
		if (searchedCell != cell)
		{
			// Le cellule est similaire s'il a meme deuxieme cle de hash
			if (searchedCell->nHash2 == nModifiedCellHash2)
			{
				// et memes parties
				if (CellDictionarySameCellKeys(cell, searchedCell))
					break;
			}
		}

		// Passage a la cellule suivante
		searchedCell = searchedCell->hashNextCell;
	}
	assert(searchedCell == NULL or CellDictionarySameCellKeys(cell, searchedCell));

	// Restitution des parties initiaux de la cellule
	cell->oaParts.SetAt(nModifiedAttribute1, initialPart1);
	cell->oaParts.SetAt(nModifiedAttribute2, initialPart2);

	ensure(GetCellNumber() == nCellDictionaryCount);
	return searchedCell;
}

void KWDataGridMerger::CellDictionaryTransferModifiedCell(KWDGMCell* cell, KWDGMPart* modifiedPart)
{
	KWDGMCell* searchedCell;
	int nModifiedAttribute;
	KWDGMPart* initialPart;
	int nAttributeHash;
	int nAttributeHash2;
	int nPartDeltaHash;
	int nPartDeltaHash2;
	int nModifiedCellHash;
	int nModifiedCellHash2;

	require(cell != NULL);
	require(cell->GetAttributeNumber() == GetAttributeNumber());
	require(GetCellNumber() == nCellDictionaryCount);
	require(modifiedPart != NULL);
	require(modifiedPart->GetAttribute() != NULL);
	require(CellDictionaryLookupModifiedCell(cell, modifiedPart) == NULL);

	// Calcul de l'index de la partie modifie
	nModifiedAttribute = modifiedPart->GetAttribute()->GetAttributeIndex();
	assert(GetAttributeAt(nModifiedAttribute) == modifiedPart->GetAttribute());

	// Suppression de la cellule du dictionnaire
	CellDictionaryRemoveCell(cell);

	// Modification temporaire de la partie de la cellule
	initialPart = cast(KWDGMPart*, cell->oaParts.GetAt(nModifiedAttribute));
	cell->oaParts.SetAt(nModifiedAttribute, modifiedPart);
	assert(initialPart != modifiedPart);

	// Calcul de la cle de hash de la cellule modifie
	nAttributeHash = cast(KWDGMAttribute*, GetAttributeAt(nModifiedAttribute))->nHash;
	nModifiedCellHash = cell->nHash;
	nPartDeltaHash =
	    (oaCellDictionary.GetSize() - initialPart->nHash + modifiedPart->nHash) % oaCellDictionary.GetSize();
	nModifiedCellHash += ComputeProductModulo(nAttributeHash, nPartDeltaHash, oaCellDictionary.GetSize());
	nModifiedCellHash %= oaCellDictionary.GetSize();
	assert(nModifiedCellHash >= 0);

	// Calcul de la deuxieme cle de hash de la cellule modifiee
	nAttributeHash2 = cast(KWDGMAttribute*, GetAttributeAt(nModifiedAttribute))->nHash2;
	nModifiedCellHash2 = cell->nHash2;
	nPartDeltaHash2 =
	    (oaCellDictionary.GetSize() - initialPart->nHash2 + modifiedPart->nHash2) % oaCellDictionary.GetSize();
	nModifiedCellHash2 += ComputeProductModulo(nAttributeHash2, nPartDeltaHash2, oaCellDictionary.GetSize());
	nModifiedCellHash2 %= oaCellDictionary.GetSize();

	// Memorisation des nouvelles cle de hash
	cell->nHash = nModifiedCellHash;
	cell->nHash2 = nModifiedCellHash2;

	// Recherche d'une cellule dans le dictionnaire a la position de hash de la cellule modifiee
	searchedCell = cast(KWDGMCell*, oaCellDictionary.GetAt(cell->nHash));
	assert(searchedCell == NULL or searchedCell->nHash == cell->nHash);

	// Chainage avec la cellule existante avant le rangement dans la table
	cell->hashNextCell = searchedCell;
	oaCellDictionary.SetAt(cell->nHash, cell);
	nCellDictionaryCount++;

	// Restitution de la partie initial de la cellule
	cell->oaParts.SetAt(nModifiedAttribute, initialPart);

	ensure(GetCellNumber() == nCellDictionaryCount);
}

void KWDataGridMerger::CellDictionaryAddCell(KWDGMCell* cell)
{
	int nCellDictionarySize;
	int nCellHash;
	int nCellHash2;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGMAttribute* attributeM;
	KWDGMPart* partM;
	KWDGMCell* searchedCell;

	require(cell != NULL);

	// Calcul des cles de hash de la cellule
	nCellDictionarySize = oaCellDictionary.GetSize();
	nCellHash = 0;
	nCellHash2 = 0;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		// Acces a la partie et a son attribut
		attribute = GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);
		partM = cast(KWDGMPart*, cell->GetPartAt(nAttribute));
		assert(partM->GetAttribute() == attribute);

		// Contribution de la partie a la cle de hash
		nCellHash += ComputeProductModulo(attributeM->nHash, partM->nHash, nCellDictionarySize);
		nCellHash %= nCellDictionarySize;

		// Contribution de la partie a la deuxieme cle de hash
		nCellHash2 += ComputeProductModulo(attributeM->nHash2, partM->nHash2, nCellDictionarySize);
		nCellHash2 %= nCellDictionarySize;
	}

	// Affectation des cles de hash de la cellule
	cell->nHash = nCellHash;
	cell->nHash2 = nCellHash2;

	// Recherche d'une cellule pour la cle dans la table de hash
	searchedCell = cast(KWDGMCell*, oaCellDictionary.GetAt(cell->nHash));
	assert(searchedCell == NULL or searchedCell->nHash == cell->nHash);
	assert(cell != searchedCell);

	// Chainage avec la cellule existant avant le rangement dans la table
	cell->hashNextCell = searchedCell;
	oaCellDictionary.SetAt(cell->nHash, cell);
	nCellDictionaryCount++;
}

void KWDataGridMerger::CellDictionaryRemoveCell(KWDGMCell* cell)
{
	KWDGMCell* searchedCell;

	require(cell != NULL);
	require(cell->GetAttributeNumber() == GetAttributeNumber());
	require(GetCellNumber() == nCellDictionaryCount);

	// Recherche d'une cellule dans le dictionnaire a la position de hash
	searchedCell = cast(KWDGMCell*, oaCellDictionary.GetAt(cell->nHash));

	// Cas particulier ou la premiere cellule trouve est la cellule a supprimer
	if (searchedCell == cell)
	{
		oaCellDictionary.SetAt(cell->nHash, searchedCell->hashNextCell);
		nCellDictionaryCount--;
	}
	else
	// Sinon, parcours de la liste chainee des cellules a cette position pour
	// retrouver la cellule a supprimer
	{
		// Recherche de la cellule precedant la cellule a supprimer
		while (searchedCell->hashNextCell != cell)
		{
			// Le cellule suivant doit etre non nul tant que l'on a pas trouve
			// la cellule a supprimer
			assert(searchedCell->hashNextCell != NULL);

			// Passage au cellule suivant
			searchedCell = searchedCell->hashNextCell;
		}

		// Suppression de la cellule
		assert(searchedCell->hashNextCell == cell);
		searchedCell->hashNextCell = searchedCell->hashNextCell->hashNextCell;
		nCellDictionaryCount--;
	}

	ensure(GetCellNumber() == nCellDictionaryCount + 1);
}

void KWDataGridMerger::CellDictionaryRemoveAll()
{
	oaCellDictionary.SetSize(0);
	nCellDictionaryCount = 0;
}

void KWDataGridMerger::CellDictionaryWrite(ostream& ost) const
{
	int nHash;
	int nCell;
	int nChainLength;
	KWDGMCell* cellM;

	// Affichage de l'entete
	ost << "IdCell\tChainLength\tHash\tHash2\tCell\n";

	// Parcours de la table de hash
	nCell = 0;
	for (nHash = 0; nHash < oaCellDictionary.GetSize(); nHash++)
	{
		cellM = cast(KWDGMCell*, oaCellDictionary.GetAt(nHash));

		// Parcours de la liste chainee des cellules
		nChainLength = 0;
		while (cellM != NULL)
		{
			assert(cellM->nHash == nHash);
			nChainLength++;
			nCell++;

			// Affichage des statistique sur le hashage
			ost << nCell << "\t" << nChainLength << "\t" << nHash << "\t";

			// Affichage de la cellule (avec sa deuxieme cle de hash)
			ost << cellM->nHash2 << "\t";
			ost << cellM->GetObjectLabel() << "\n";

			// Cellule suivante
			cellM = cellM->hashNextCell;
		}
	}
}

int KWDataGridMerger::ComputeProductModulo(int nFactor1, int nFactor2, int nModuloRange) const
{
	double dProduct;
	double dRatio;
	int nResult;

	require(nModuloRange > 0);
	require(0 <= nFactor1 and nFactor1 < nModuloRange);
	require(0 <= nFactor2 and nFactor2 < nModuloRange);

	// Calcul du produit des facteurs en precision reelle
	dProduct = nFactor1 * 1.0 * nFactor2;

	// On renvoie le modulo si on reste dans la limite des entiers
	if (dProduct < INT_MAX)
		nResult = ((int)dProduct) % nModuloRange;
	// Sinon, on calcul le modulo a la main
	else
	{
		dRatio = dProduct / nModuloRange;
		nResult = (int)(dProduct - floor(dRatio) * nModuloRange);
	}
	ensure(0 <= nResult and nResult < nModuloRange);
	return nResult;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMAttribute

KWDGMAttribute::KWDGMAttribute()
{
	slPartMerges = new SortedList(KWDGMPartMergeCompare);
	nHash = 0;
	nHash2 = 0;
	dCost = 0;
	slPartValueNumbers = new SortedList(KWDGMPartValueNumberCompare);
}

KWDGMAttribute::~KWDGMAttribute()
{
	DeleteAllPartMerges();
	delete slPartMerges;
	RemoveAllPartsFromValueNumberList();
	delete slPartValueNumbers;
}

longint KWDGMAttribute::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDGAttribute::GetUsedMemory();
	lUsedMemory += sizeof(KWDGMAttribute) - sizeof(KWDGAttribute);
	if (slPartMerges != NULL)
		lUsedMemory += slPartMerges->GetCount() * (sizeof(AVLNode) + sizeof(ListNode) + sizeof(KWDGMPartMerge));
	return lUsedMemory;
}

KWDGPart* KWDGMAttribute::NewPart() const
{
	return new KWDGMPart;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMPart

boolean KWDGMPart::Check() const
{
	boolean bOk = true;
	int nTotalValueFrequency;
	ALString sTmp;

	// Verification de base
	bOk = KWDGPart::Check();

	// Verification de l'ensemble de valeurs
	if (bOk and GetPartType() == KWType::Symbol)
	{
		// Verification de la compatibilite entre l'effectif de la partie
		// et l'effectif cumule de ses valeurs
		// La verification ne se fait que si la partie a un effectif non nul,
		// ou si l'effectif cumule des valeurs est non nul.
		// Cela permet des verifications partielles en cours d'alimentation
		// de la grille (les parties et leurs valeurs sont creees avant
		// la creation des cellules).
		// La verification n'est pas faite dans tous les cas dans la classe ancetre
		nTotalValueFrequency = valueSet->ComputeTotalFrequency();
		if (bOk and GetPartFrequency() > 0 and nTotalValueFrequency > 0 and
		    GetPartFrequency() != nTotalValueFrequency)
		{
			AddError(sTmp + "Part frequency (" + IntToString(GetPartFrequency()) +
				 ") different from the cumulated frequency of its values (" +
				 IntToString(nTotalValueFrequency) + ")");
			bOk = false;
		}
	}
	return bOk;
}

longint KWDGMPart::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDGPart::GetUsedMemory();
	lUsedMemory += sizeof(KWDGMPart) - sizeof(KWDGMPart);
	lUsedMemory += nkdPartMerges.GetUsedMemory();
	return lUsedMemory;
}

void KWDGMPart::WriteAllPartMerges(ostream& ost) const
{
	POSITION positionPartMerge;
	KWDGMPartMerge* partMerge;

	// Parcours des fusions
	ost << "Merges of\t" << GetClassLabel() << "\t" << GetObjectLabel() << "\n";
	positionPartMerge = GetStartPartMerge();
	while (position != NULL)
	{
		GetNextPartMerge(positionPartMerge, partMerge);
		ost << "\t" << partMerge->GetObjectLabel() << "\n";
	}
}

int KWDGMPartValueNumberCompare(const void* elem1, const void* elem2)
{
	KWDGMPart* partM1;
	KWDGMPart* partM2;

	partM1 = cast(KWDGMPart*, *(Object**)elem1);
	partM2 = cast(KWDGMPart*, *(Object**)elem2);

	// Comparaison du nombre de modalites par valeurs decroissantes
	return (partM2->GetValueSet()->GetTrueValueNumber() - partM1->GetValueSet()->GetTrueValueNumber());
}
//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMCell

longint KWDGMCell::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDGCell::GetUsedMemory();
	lUsedMemory += sizeof(KWDGMCell) - sizeof(KWDGCell);
	return lUsedMemory;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMPartMerge

boolean KWDGMPartMerge::Check() const
{
	return part1 != NULL and part2 != NULL and part1 != part2 and part1->GetAttribute() == part2->GetAttribute();
}

void KWDGMPartMerge::WriteHeaderLine(ostream& ost) const
{
	ost << "Attribute\tPart1\tPart2\tMergeCost\tCellNumber\tGarbagePresence\n";
}

void KWDGMPartMerge::WriteLine(ostream& ost) const
{
	Write(ost);
}

void KWDGMPartMerge::Write(ostream& ost) const
{
	// Nom de l'attribut
	if (part1 != NULL and part1->GetAttribute() != NULL)
		ost << part1->GetAttribute()->GetObjectLabel();
	ost << "\t";

	// Nom de la premiere partie
	if (part1 != NULL)
		ost << part1->GetObjectLabel();
	ost << "\t";

	// Nom de la deuxieme partie
	if (part2 != NULL)
		ost << part2->GetObjectLabel();
	ost << "\t";

	// Valeur de la fusion
	ost << dMergeCost << "\t";

	// Nombre de cellules impliques
	if (part1 != NULL and part2 != NULL)
		ost << part1->GetCellNumber() + part2->GetCellNumber();
	ost << "\t" << bGarbagePresence;
	ost << "\n";
}

const ALString KWDGMPartMerge::GetClassLabel() const
{
	return "Merge";
}

const ALString KWDGMPartMerge::GetObjectLabel() const
{
	ALString sLabel;

	// Nom de l'attribut
	if (part1 != NULL and part1->GetAttribute() != NULL)
		sLabel = part1->GetAttribute()->GetObjectLabel();

	// Nom de la premiere partie (avec son nombre de cellules)
	sLabel += "(";
	if (part1 != NULL)
		sLabel += part1->GetObjectLabel();
	sLabel += ":";
	if (part1 != NULL)
		sLabel += IntToString(part1->GetCellNumber());

	// Nom de la deuxieme partie (avec son nombre de cellules)
	sLabel += ", ";
	if (part2 != NULL)
		sLabel += part2->GetObjectLabel();
	sLabel += ":";
	if (part2 != NULL)
		sLabel += IntToString(part2->GetCellNumber());
	sLabel += ")";

	// Cout
	sLabel += DoubleToString(GetMergeCost());

	return sLabel;
}

int KWDGMPartMergeCompare(const void* elem1, const void* elem2)
{
	// On utilise Epsilon=0 en escomptant le resultat du Diff est reproductible si les operandes sont les memes
	// Pour Epsilon > 0, on court le risque d'avoir diff(PM1,PM2) < Epsilon et diff(PM2,PM3) < Epsilon,
	// mais diff(PM1,PM3) >= Epsilon (ce bug (avec consequence desatreuse dans une SortedList) est deja arrive)
	const double dEpsilon = 0;
	KWDGMPartMerge* partMerge1;
	KWDGMPartMerge* partMerge2;
	double dDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux fusions
	partMerge1 = cast(KWDGMPartMerge*, *(Object**)elem1);
	partMerge2 = cast(KWDGMPartMerge*, *(Object**)elem2);
	assert(partMerge1->Check());
	assert(partMerge2->Check());

	// Calcul de la difference
	dDiff = partMerge1->GetMergeCost() - partMerge2->GetMergeCost();
	if (dDiff > dEpsilon)
		return 1;
	else if (dDiff < -dEpsilon)
		return -1;
	// Si egalite, on renvoie 0
	// On ne peut se baser sur le nombre de cellules des fusions, qui varie a cout egal,
	// ce qui empeche les optimisation poussees visant a ne reevaluer que les
	// fusions impactees
	else
		return 0;
}