// Copyright (c) 2023-2026 Orange. All rights reserved.
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
	dCoclusteringGlobalImportance = 0;
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

void CCHierarchicalDataGrid::SetCoclusteringGlobalImportance(double dValue)
{
	require(dValue > 0);
	dCoclusteringGlobalImportance = dValue;
}

double CCHierarchicalDataGrid::GetCoclusteringGlobalImportance() const
{
	return dCoclusteringGlobalImportance;
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

void CCHierarchicalDataGrid::SetIdentifierAttributeName(const ALString& sValue)
{
	sIdentifierAttributeName = sValue;
}

const ALString& CCHierarchicalDataGrid::GetIdentifierAttributeName() const
{
	return sIdentifierAttributeName;
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
	sIdentifierAttributeName = "";
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

boolean CCHierarchicalDataGrid::CheckImportances() const
{
	int nAttribute;
	double dTotalAttributeImportance;
	double dTotalPartImportance;
	double dTotalPartImportanceInVariable;
	double dTotalValueImportance;
	double dEpsilon = 1e-6;
	boolean bOk = true;
	CCHDGAttribute* attribute;
	KWDGPart* dgPart;
	KWDGValue* dgValue;

	// La somme des importances des attributs de dimension doit etre egal a 1
	dTotalAttributeImportance = 0;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(CCHDGAttribute*, GetAttributeAt(nAttribute));
		dTotalAttributeImportance += attribute->GetImportance();

		// Parcours et verification de la somme des importances des parties
		dTotalPartImportance = 0;
		dgPart = attribute->GetHeadPart();
		while (dgPart != NULL)
		{
			dTotalPartImportance += cast(CCHDGPart*, dgPart)->GetImportance();

			// Cas d'une partie groupable : verification de la coherence de la somme des importances de ses elements (somme a 1)
			if (KWType::IsCoclusteringGroupableType(dgPart->GetPartType()))
			{
				dgValue = dgPart->GetValueSet()->GetHeadValue();
				dTotalValueImportance = 0;
				while (dgValue != NULL)
				{
					dTotalValueImportance += dgValue->GetImportance();
					dgPart->GetValueSet()->GetNextValue(dgValue);
				}
				bOk = fabs(dTotalValueImportance - 1) < dEpsilon;
				if (not bOk)
				{
					break;
					AddWarning("Inconsistent value importances for part\t" +
						   dgPart->GetClassLabel());
				}
			}
			attribute->GetNextPart(dgPart);
		}
		bOk = fabs(dTotalPartImportance - 1) < dEpsilon;
		if (not bOk)
		{
			break;
			AddWarning("Inconsistent part importances for attribute\t" + attribute->GetAttributeName());
		}
	}
	bOk = fabs(dTotalAttributeImportance - 1) < dEpsilon;

	// Affichage de la somme des importances des dimensions
	if (not bOk)
	{
		cout << "CheckImportances\t" << GetObjectLabel() << "\n";
		cout << "\tAttribute importances sum\t" << dTotalAttributeImportance << "\n";
	}

	// Cas d'un coclustering individus * variables
	if (bOk and IsVarPartDataGrid())
	{
		// La somme des importances des attributs internes doit etre egal a 1
		dTotalAttributeImportance = 0;
		for (nAttribute = 0; nAttribute < GetInnerAttributes()->GetInnerAttributeNumber(); nAttribute++)
		{
			attribute = cast(CCHDGAttribute*, GetInnerAttributes()->GetInnerAttributeAt(nAttribute));
			dTotalAttributeImportance += attribute->GetImportance();

			// Parcours et verification de la somme des importances des parties au sein de leur cluster et au sein de leur variable
			dTotalPartImportance = 0;
			dTotalPartImportanceInVariable = 0;
			dgPart = attribute->GetHeadPart();
			while (dgPart != NULL)
			{
				dTotalPartImportance += cast(CCHDGPart*, dgPart)->GetImportance();
				dTotalPartImportanceInVariable += cast(CCHDGPart*, dgPart)->GetImportanceInVariable();

				// Cas d'une partie groupable : verification de la coherence de la somme des importances de ses elements (somme a 1)
				if (KWType::IsCoclusteringGroupableType(dgPart->GetPartType()))
				{
					dgValue = dgPart->GetValueSet()->GetHeadValue();
					dTotalValueImportance = 0;
					while (dgValue != NULL)
					{
						dTotalValueImportance += dgValue->GetImportance();
						dgPart->GetValueSet()->GetNextValue(dgValue);
					}
					bOk = fabs(dTotalValueImportance - 1) < dEpsilon;
					if (not bOk)
					{
						break;
						AddWarning("Inconsistent value importances for part\t" +
							   dgPart->GetClassLabel());
					}
				}
				attribute->GetNextPart(dgPart);
			}
			// Nok : deux definitions -> sommer sur le cluster de PV ou sur les PV d'une variable
			//bOk = fabs(dTotalPartImportance - 1 < dEpsilon);
			if (not bOk)
			{
				break;
				AddWarning("Inconsistent part importances for attribute\t" +
					   attribute->GetAttributeName());
			}
			bOk = fabs(dTotalPartImportanceInVariable - 1) < dEpsilon;
			if (not bOk)
			{
				break;
				AddWarning("Inconsistent part importances in variable for attribute\t" +
					   attribute->GetAttributeName());
			}
		}
		bOk = fabs(dTotalAttributeImportance - 1) < dEpsilon;

		// Affichage de la somme des importances des inner Attributes
		if (not bOk)
		{
			cout << "CheckImportances\t" << GetObjectLabel() << "\n";
			cout << "\tInner attribute importances sum\t" << dTotalAttributeImportance << "\n";
		}
	}
	return bOk;
}

const ALString CCHierarchicalDataGrid::GetClassLabel() const
{
	return "Hierarchichal data grid";
}

KWDGAttribute* CCHierarchicalDataGrid::NewAttribute() const
{
	return new CCHDGAttribute;
}

KWDGCell* CCHierarchicalDataGrid::NewCell() const
{
	return new CCHDGCell;
}

void CCHierarchicalDataGrid::ComputeAllImportances()
{

	// Cas d'un coclustering informatif (non reduit au modele nul)
	if (GetCellNumber() > 1)
	{
		// Calcul de l'importance totale du coclustering
		ComputeCoclusteringImportance();

		// Calcul de l'importance de chaque cluster pour chaque dimension du coclustering
		ComputePartImportances();

		// Cas d'un coclustering instances x variables : calcul de l'importance des attributs internes
		// et de leurs parties au sein de leur cluster et au sein de leur attribut
		if (IsVarPartDataGrid())
			ComputeInnerAttributeImportances();

		// Verification de la coherence des importances
		assert(CheckImportances());
	}
}

void CCHierarchicalDataGrid::ComputeCoclusteringImportance()
{
	KWDGCell* cell;
	CCHDGCell* cchdgCell;
	double dTotalImportance;
	int nAttribute;
	double dFrequencyPartProductNormalized;

	// Calcul de l'importance du coclustering par sommation sur l'ensemble des cellules
	cell = GetHeadCell();
	dTotalImportance = 0;
	while (cell != NULL)
	{
		cchdgCell = cast(CCHDGCell*, cell);
		// Parcours des parties de la cellule pour le calcul de l'effectif attendu en cas d'independance (expected frequency)
		dFrequencyPartProductNormalized = 1.0;
		for (nAttribute = 0; nAttribute < cell->GetAttributeNumber(); nAttribute++)
			dFrequencyPartProductNormalized *=
			    (double)cell->GetPartAt(nAttribute)->GetPartFrequency() / (double)GetGridFrequency();

		// Calcul de l'information mutuelle de la cellule (sa contribution)
		cchdgCell->SetMutualInformation(((double)cell->GetCellFrequency() / (double)GetGridFrequency()) *
						log((double)cell->GetCellFrequency() /
						    (double)(GetGridFrequency() * dFrequencyPartProductNormalized)));

		// Ajout de la contribution de la cellule a l'importance globale du coclustering
		// L'importance totale est la somme des valeurs absolues des contributions a l'information mutuelle
		dTotalImportance += abs(cchdgCell->GetMutualInformation());

		// Cellule suivante
		GetNextCell(cell);
	}
	SetCoclusteringGlobalImportance(dTotalImportance);
}

void CCHierarchicalDataGrid::ComputePartImportances()
{
	int nAttribute;
	CCHDGAttribute* attribute;
	CCHDGPart* cchdgPart;
	CCHDGCell* cchdgCell;
	KWDGPart* part;
	KWDGCell* cell;
	KWDGValue* dgValue;
	double dImportance;
	double dTotalImportance;
	double dValueSetTotalFrequency;
	double dEpsilon = 1e-6;

	require(GetCoclusteringGlobalImportance() > 0 or GetCellNumber() == 1);

	// Calcul de l'importance par partie pour chaque dimension de la grille
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		// Partage de l'importance globale entre les attributs, simplement au prorata du nombre d'attributs
		attribute = cast(CCHDGAttribute*, GetAttributeAt(nAttribute));
		attribute->SetImportance(1.0 / GetAttributeNumber());
		dTotalImportance = 0;

		// Calcul de l'importance de chaque partie d'attribut
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			cchdgPart = cast(CCHDGPart*, part);

			// Somme sur les cellules des valeurs absolues de leur contribution a l'information mutuelle
			cell = part->GetHeadCell();
			dImportance = 0;
			while (cell != NULL)
			{
				cchdgCell = cast(CCHDGCell*, cell);
				dImportance += abs(cchdgCell->GetMutualInformation());
				part->GetNextCell(cell);
			}

			// Cas d'un coclustering informatif
			// Dans le cas d'un modele nul, l'importance est nulle et a deja ete initialisee a 0
			if (GetCoclusteringGlobalImportance() > 0)
			{
				cchdgPart->SetImportance(dImportance / GetCoclusteringGlobalImportance());
				dTotalImportance += cchdgPart->GetImportance();

				// Importance des elements de la partie dans le cas d'une partie groupable
				if (KWType::IsCoclusteringGroupableType(cchdgPart->GetPartType()))
				{
					dgValue = cchdgPart->GetValueSet()->GetHeadValue();
					dValueSetTotalFrequency = cchdgPart->GetValueSet()->ComputeTotalFrequency();
					while (dgValue != NULL)
					{
						if (dgValue->GetValueFrequency() > 0)
							dgValue->SetImportance((double)dgValue->GetValueFrequency() /
									       dValueSetTotalFrequency);
						cchdgPart->GetValueSet()->GetNextValue(dgValue);
					}
				}
			}

			// Passage a la partie suivante
			attribute->GetNextPart(part);
		}
		assert(fabs(dTotalImportance - 1) < dEpsilon);
	}
}

void CCHierarchicalDataGrid::ComputeInnerAttributeImportances()
{
	CCHDGAttribute* innerAttribute;
	CCHDGPart* innerAttributePart;
	CCHDGPart* varPartCluster;
	KWDGPart* part;
	KWDGValue* dgValue;
	int nAttribute;
	double dImportanceInVariable;
	double dInnerAttributeImportance;
	double dTotalInnerAttributeImportance;
	double dEpsilon = 1e-6;

	// Construction de la structure d'indexation pour acceder aux parties de variables depuis les innerAttributes
	BuildIndexingStructure();

	// Parcours des attributs internes
	dTotalInnerAttributeImportance = 0;
	for (nAttribute = 0; nAttribute < GetInnerAttributes()->GetInnerAttributeNumber(); nAttribute++)
	{
		innerAttribute = cast(CCHDGAttribute*, GetInnerAttributes()->GetInnerAttributeAt(nAttribute));

		// Calcul de l'importance de chaque partie de variable au sein de son cluster de parties de variable
		// On repartit l'importance du cluster de parties de variable au prorata des effectifs par partie,
		// en exploitant l'hypothese d independance entre les dimensions instances et parties de variables localement au sein de chaque cocluster.
		// Cela revient a avoir l'importance de la partie de variable egale au ratio de l'effectif de sa partie par l'effectif du cluster auquel elle appartient
		part = innerAttribute->GetHeadPart();
		while (part != NULL)
		{
			innerAttributePart = cast(CCHDGPart*, part);
			varPartCluster = cast(CCHDGPart*, GetVarPartAttribute()->LookupVarPart(innerAttributePart));
			innerAttributePart->SetImportance((double)innerAttributePart->GetPartFrequency() /
							  (double)varPartCluster->GetPartFrequency());

			// Importance des elements de la partie dans le cas d'une partie groupable
			if (KWType::IsCoclusteringGroupableType(innerAttributePart->GetPartType()))
			{
				dgValue = innerAttributePart->GetValueSet()->GetHeadValue();
				while (dgValue != NULL)
				{
					if (dgValue->GetValueFrequency() > 0)
						dgValue->SetImportance(
						    (double)dgValue->GetValueFrequency() /
						    (double)innerAttributePart->GetValueSet()->ComputeTotalFrequency());
					innerAttributePart->GetValueSet()->GetNextValue(dgValue);
				}
			}

			// Partie suivante
			innerAttribute->GetNextPart(part);
		}

		// Calcul de l'importance de chaque partie de variable au sein de sa variable
		part = innerAttribute->GetHeadPart();
		dInnerAttributeImportance = 0;

		// Somme sur toutes les PV de l'attribut
		while (part != NULL)
		{
			innerAttributePart = cast(CCHDGPart*, part);
			varPartCluster = cast(CCHDGPart*, GetVarPartAttribute()->LookupVarPart(innerAttributePart));
			dInnerAttributeImportance +=
			    innerAttributePart->GetImportance() * varPartCluster->GetImportance();

			// Partie suivante
			innerAttribute->GetNextPart(part);
		}
		innerAttribute->SetImportance(dInnerAttributeImportance);
		dTotalInnerAttributeImportance += innerAttribute->GetImportance();

		// Importance en pourcentage de la PV dans sa variable
		part = innerAttribute->GetHeadPart();
		while (part != NULL)
		{
			innerAttributePart = cast(CCHDGPart*, part);
			varPartCluster = cast(CCHDGPart*, GetVarPartAttribute()->LookupVarPart(innerAttributePart));
			dImportanceInVariable = innerAttributePart->GetImportance() * varPartCluster->GetImportance() /
						dInnerAttributeImportance;
			innerAttributePart->SetImportanceInVariable(dImportanceInVariable);

			// Partie suivante
			innerAttribute->GetNextPart(part);
		}
	}
	assert((dTotalInnerAttributeImportance > 1 - dEpsilon) and (dTotalInnerAttributeImportance < 1 + dEpsilon));

	// Nettoyage
	DeleteIndexingStructure();
}

//////////////////////////////////////////////////////////////////////////////
// Classe CCHDGAttribute

CCHDGAttribute::CCHDGAttribute()
{
	cMin = 0;
	cMax = 0;
	nInitialPartNumber = 0;
	dInterest = 0;
	dImportance = 0;
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

void CCHDGAttribute::SetImportance(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dImportance = dValue;
}

double CCHDGAttribute::GetImportance() const
{
	return dImportance;
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
	require(KWType::IsCoclusteringType(GetAttributeType()));

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
	const boolean bTrace = false;
	CCHDGPart* sourcePart;
	CCHDGPart* targetPart;
	KWDGCell* cell1;
	KWDGCell* cell2;
	KWDGCell* cellToDelete1;
	ObjectArray oaParts;
	int nAttribute;
	KWDGValue* dgValue;
	int nTotalPartFrequency;

	require(IsPartMergeable(part));

	// Donnes en entree
	if (bTrace)
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
		// Creation d'une nouvelle cellule sinon
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
	targetPart->GetPartValues()->Import(sourcePart->GetPartValues());

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

	// Calcul de l'importance de la partie fusionnee
	// L'importance de la partie est ici additive, car elle est relative a son cluster
	if (GetImportanceMode())
	{
		assert(sourcePart->GetImportance() > 0 and targetPart->GetImportance() > 0);
		targetPart->SetImportance(sourcePart->GetImportance() + targetPart->GetImportance());
	}

	// Cas d'un attribut groupable : renormalisation de l'importance des elements de la partie fusionnee par rapport a son effectif
	// et a la nouvelle importance de la partie fusionnee
	if (KWType::IsCoclusteringGroupableType(sourcePart->GetPartType()))
	{
		// On memorise l'effectif de la partie fusionnee
		nTotalPartFrequency = sourcePart->GetPartFrequency() + targetPart->GetPartFrequency();

		// Renormalisation des importances de chaque element des deux parties
		dgValue = sourcePart->GetValueSet()->GetHeadValue();
		while (dgValue != NULL)
		{
			dgValue->SetImportance(dgValue->GetValueFrequency() / (double)nTotalPartFrequency);
			sourcePart->GetValueSet()->GetNextValue(dgValue);
		}
		dgValue = targetPart->GetValueSet()->GetHeadValue();
		while (dgValue != NULL)
		{
			dgValue->SetImportance(dgValue->GetValueFrequency() / (double)nTotalPartFrequency);
			targetPart->GetValueSet()->GetNextValue(dgValue);
		}
	}

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
	if (bTrace)
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
		nkdExportedParts.SetAt(rootPart, rootPart);
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
			    nkdExportedParts.Lookup(hdgPart->GetChildPart1()) == NULL)
			{
				oaParts->Add(hdgPart->GetChildPart1());
				nkdExportedParts.SetAt(hdgPart->GetChildPart1(), hdgPart->GetChildPart1());
			}
			if (hdgPart->GetChildPart2() != NULL and
			    nkdExportedParts.Lookup(hdgPart->GetChildPart2()) == NULL)
			{
				oaParts->Add(hdgPart->GetChildPart2());
				nkdExportedParts.SetAt(hdgPart->GetChildPart2(), hdgPart->GetChildPart2());
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
	dImportance = 0;
	dImportanceInVariable = 0;
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

void CCHDGPart::SetImportance(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dImportance = dValue;
}

double CCHDGPart::GetImportance() const
{
	return dImportance;
}

void CCHDGPart::SetImportanceInVariable(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dImportanceInVariable = dValue;
}

double CCHDGPart::GetImportanceInVariable() const
{
	return dImportanceInVariable;
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

	// Verifications complementaires de chainahe coherent de la hierarchie, sans message d'erreur
	assert(not bOk or parentPart == NULL or parentPart->childPart1 == this or parentPart->childPart2 == this);
	assert(not bOk or childPart1 == NULL or childPart1->parentPart == this);
	assert(not bOk or childPart2 == NULL or childPart2->parentPart == this);
	return bOk;
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

/////////////////////////////////////////////////////////////////////////////
// Classe CCHDGCell

CCHDGCell::CCHDGCell()
{
	dMutualInformation = 0;
}

CCHDGCell::~CCHDGCell()
{
	// Reinitialisation pour faciliter le debug
	debug(dMutualInformation = 0);
}

double CCHDGCell::GetMutualInformation() const
{
	ensure(ivFrequencyVector.GetSize() == 0 or ComputeTotalFrequency() == nCellFrequency);
	return dMutualInformation;
}

void CCHDGCell::SetMutualInformation(double dValue)
{
	require(GetTargetValueNumber() == 0);
	dMutualInformation = dValue;
}
