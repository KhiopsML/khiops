// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridCosts.h"

////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridCosts

const double KWDataGridCosts::dEpsilon = 1e-6;

KWDataGridCosts::KWDataGridCosts()
{
	dModelFamilySelectionCost = 0;
	dataGridDefaultCosts = NULL;
	dTotalDefaultCost = 0;
	dAllValuesDefaultCost = 0;
}

KWDataGridCosts::~KWDataGridCosts()
{
	if (dataGridDefaultCosts != NULL)
		delete dataGridDefaultCosts;
}

KWDataGridCosts* KWDataGridCosts::Clone() const
{
	return new KWDataGridCosts;
}

void KWDataGridCosts::SetModelFamilySelectionCost(double dValue)
{
	require(dValue >= 0);
	dModelFamilySelectionCost = dValue;
}

double KWDataGridCosts::GetModelFamilySelectionCost() const
{
	return dModelFamilySelectionCost;
}

double KWDataGridCosts::ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
					    int nInformativeAttributeNumber) const
{
	require(dataGrid != NULL);
	require(dLnGridSize >= 0);
	require(nInformativeAttributeNumber >= 0);
	return 0;
}

double KWDataGridCosts::ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	require(attribute != NULL);
	require(nPartitionSize >= 1);
	return 0;
}

double KWDataGridCosts::ComputePartCost(const KWDGPart* part) const
{
	require(part != NULL);
	return 0;
}

double KWDataGridCosts::ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const
{
	require(part1 != NULL);
	require(part2 != NULL);
	return 0;
}

double KWDataGridCosts::ComputeCellCost(const KWDGCell* cell) const
{
	require(cell != NULL);
	return 0;
}
// CH IV Begin
double KWDataGridCosts::ComputeInnerAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	require(attribute != NULL);
	return 0;
}

double KWDataGridCosts::ComputeInnerAttributePartCost(const KWDGPart* part) const
{
	require(part != NULL);
	return 0;
}
// CH IV End
double KWDataGridCosts::ComputeDataGridCompressionCoefficient(const KWDataGrid* dataGrid) const
{
	double dLevel;
	double dTotalCost;

	dLevel = 0;
	dTotalCost = ComputeDataGridTotalCost(dataGrid);
	if (dTotalDefaultCost != 0)
	{
		dLevel = 1 - dTotalCost / dTotalDefaultCost;
		if (dLevel < 0)
			dLevel = 0;
	}
	return dLevel;
}

double KWDataGridCosts::ComputeDataGridTotalCost(const KWDataGrid* dataGrid) const
{
	double dTotalCost;

	require(CheckDataGrid(dataGrid));

	// Initialisation avec le cout cumulatif du DataGrid
	dTotalCost = ComputeDataGridCumulativeCost(dataGrid);

	// Prise en compte du cout des valeurs
	dTotalCost += dAllValuesDefaultCost;

	// Calcul du cout par defaut des attributs (et parties) absent d'une grille
	dTotalCost += ComputeDataGridTotalMissingAttributeCost(dataGrid);

	return dTotalCost;
}

double KWDataGridCosts::ComputeDataGridMergerTotalCost(const KWDataGridMerger* dataGridMerger) const
{
	double dTotalCost;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGCell* cell;
	KWDGMAttribute* attributeM;
	KWDGMPart* partM;
	KWDGMCell* cellM;

	require(CheckDataGrid(dataGridMerger));

	// Initialisation avec le cout du DataGrid
	assert(fabs(dataGridMerger->GetCost() - ComputeDataGridCost(dataGridMerger, dataGridMerger->GetLnGridSize(),
								    dataGridMerger->GetInformativeAttributeNumber())) <
	       dEpsilon);
	dTotalCost = dataGridMerger->GetCost();

	// Prise en compte des attributs
	for (nAttribute = 0; nAttribute < dataGridMerger->GetAttributeNumber(); nAttribute++)
	{
		attribute = dataGridMerger->GetAttributeAt(nAttribute);
		attributeM = cast(KWDGMAttribute*, attribute);
		assert(fabs(attributeM->GetCost() - ComputeAttributeCost(attributeM, attributeM->GetPartNumber())) <
		       dEpsilon);

		// Cout de l'attribut
		dTotalCost += attributeM->GetCost();

		// Cout des parties
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			partM = cast(KWDGMPart*, part);
			assert(fabs(partM->GetCost() - ComputePartCost(partM)) < dEpsilon);
			dTotalCost += partM->GetCost();
			attribute->GetNextPart(part);
		}
	}

	// Prise en compte des cellules
	cell = dataGridMerger->GetHeadCell();
	while (cell != NULL)
	{
		cellM = cast(KWDGMCell*, cell);
		assert(fabs(cellM->GetCost() - ComputeCellCost(cellM)) < dEpsilon);
		dTotalCost += cellM->GetCost();
		dataGridMerger->GetNextCell(cell);
	}

	// Prise en compte du cout des valeurs
	dTotalCost += dAllValuesDefaultCost;

	// Calcul du cout par defaut des attributs (et parties) absent d'une grille
	dTotalCost += ComputeDataGridTotalMissingAttributeCost(dataGridMerger);

	return dTotalCost;
}

boolean KWDataGridCosts::CheckDataGrid(const KWDataGrid* dataGrid) const
{
	boolean bOk = true;
	int nDefaultAttribute;
	KWDGAttribute* defaultAttribute;
	int nAttribute;
	ALString sAttributeName;
	boolean bAttributeFound;

	require(dataGrid != NULL);

	// Les couts par defauts doivent etre initialises
	if (not IsInitialized())
	{
		AddError("Default data grid costs not initialiszed");
		bOk = false;
	}

	// Test de consistance des attributs avec les attributs des couts par defaut
	// Les attributs sont censes etre dans un ordre coherent
	nAttribute = 0;
	if (nAttribute < dataGrid->GetAttributeNumber())
	{
		sAttributeName = dataGrid->GetAttributeAt(nAttribute)->GetAttributeName();
		for (nDefaultAttribute = 0; nDefaultAttribute < dataGridDefaultCosts->GetAttributeNumber();
		     nDefaultAttribute++)
		{
			defaultAttribute = dataGridDefaultCosts->GetAttributeAt(nDefaultAttribute);

			// Recherche de l'index de l'attribut correspondant parmi les attribut de la grille partielle
			bAttributeFound = defaultAttribute->GetAttributeName() == sAttributeName;

			// Recherche du prochain nom d'attribut partiel si attribut trouve
			if (bAttributeFound)
			{
				nAttribute++;
				if (nAttribute < dataGrid->GetAttributeNumber())
					sAttributeName = dataGrid->GetAttributeAt(nAttribute)->GetAttributeName();
				else
					break;
			}
			// Erreur si non trouve et plus d'attribut
			else if (nDefaultAttribute == dataGridDefaultCosts->GetAttributeNumber() - 1)
			{
				AddError("Variable " + sAttributeName + " not found in default data grid");
				bOk = false;
				break;
			}
		}
	}

	return bOk;
}

void KWDataGridCosts::InitializeDefaultCosts(const KWDataGrid* dataGrid)
{
	boolean bOptimizeMemorySpace = true;
	KWDataGridManager dataGridManager;
	int nAttribute;
	KWDGAttribute* defaultAttribute;
	KWDGPart* defaultPart;

	require(dataGrid != NULL);
	require(dataGrid->Check());

	// Nettoyage prealable
	CleanDefaultCosts();

	// Creation du DataGrid des couts par defaut
	dataGridDefaultCosts = new KWDataGridMerger;
	dataGridDefaultCosts->SetDataGridCosts(this);

	// Creation d'une grille par default correspondant a la grille source
	dataGridManager.SetSourceDataGrid(dataGrid);
	dataGridManager.ExportTerminalDataGrid(dataGridDefaultCosts);

	// Initialisation des couts par defaut par entite
	dataGridDefaultCosts->InitializeAllCosts();

	// Memorisation des couts des valeurs symboliques
	dAllValuesDefaultCost = ComputeDataGridAllValuesCost(dataGridDefaultCosts);

	// Memorisation du cout total
	dTotalDefaultCost = ComputeDataGridMergerTotalCost(dataGridDefaultCosts);

	// Optimisation de la place memoire, en remplacant dans chaque attribut symbolique
	// l'ensemble des valeurs par une seule valeur "*"
	// Initialisation des attributs avec une seule partie
	if (bOptimizeMemorySpace)
	{
		for (nAttribute = 0; nAttribute < dataGridDefaultCosts->GetAttributeNumber(); nAttribute++)
		{
			// Recherche de l'attribut default
			defaultAttribute = dataGridDefaultCosts->GetAttributeAt(nAttribute);

			// Modification de l'ensemble des valeurs des attributs symbolique
			if (defaultAttribute->GetAttributeType() == KWType::Symbol)
			{
				// Acces a la partie unique de l'attribut
				assert(defaultAttribute->GetPartNumber() == 1);
				defaultPart = defaultAttribute->GetHeadPart();

				// Remplacement de ses valeurs par une valeur unique
				defaultPart->GetValueSet()->CompressValueSet();
			}
		}
	}
	ensure(dataGridManager.CheckDataGrid(dataGridDefaultCosts));
}

void KWDataGridCosts::CleanDefaultCosts()
{
	if (dataGridDefaultCosts != NULL)
		delete dataGridDefaultCosts;
	dataGridDefaultCosts = NULL;
	dTotalDefaultCost = 0;
	dAllValuesDefaultCost = 0;
}

boolean KWDataGridCosts::IsInitialized() const
{
	return dataGridDefaultCosts != NULL;
}

int KWDataGridCosts::GetTotalAttributeNumber() const
{
	require(IsInitialized());
	return dataGridDefaultCosts->GetAttributeNumber();
}

double KWDataGridCosts::GetTotalDefaultCost() const
{
	require(IsInitialized());
	return dTotalDefaultCost;
}

const ALString& KWDataGridCosts::GetAttributeNameAt(int nIndex) const
{
	require(IsInitialized());
	require(0 <= nIndex and nIndex < GetTotalAttributeNumber());
	return dataGridDefaultCosts->GetAttributeAt(nIndex)->GetAttributeName();
}

double KWDataGridCosts::GetAttributeDefaultCostAt(int nIndex) const
{
	double dDefaultCost;
	KWDGMAttribute* defaultAttributeM;
	KWDGMPart* defaultPartM;

	require(IsInitialized());
	require(0 <= nIndex and nIndex < GetTotalAttributeNumber());

	// Acces a l'attribut
	defaultAttributeM = cast(KWDGMAttribute*, dataGridDefaultCosts->GetAttributeAt(nIndex));
	assert(fabs(defaultAttributeM->GetCost() - ComputeAttributeCost(defaultAttributeM, 1)) < dEpsilon);
	dDefaultCost = defaultAttributeM->GetCost();

	// Cout de la partie
	assert(defaultAttributeM->GetPartNumber() == 1);
	defaultPartM = cast(KWDGMPart*, defaultAttributeM->GetHeadPart());
	assert(fabs(defaultPartM->GetCost() - ComputePartCost(defaultPartM)) < 1e-5);
	dDefaultCost += defaultPartM->GetCost();

	return dDefaultCost;
}

double KWDataGridCosts::GetAllValuesDefaultCost() const
{
	require(IsInitialized());
	return dAllValuesDefaultCost;
}

double KWDataGridCosts::ComputeDataGridTotalMissingAttributeCost(const KWDataGrid* dataGrid) const
{
	double dTotalCost;
	int nDefaultAttribute;
	KWDGAttribute* defaultAttribute;
	KWDGPart* part;
	KWDGMAttribute* defaultAttributeM;
	KWDGMPart* partM;
	int nAttribute;
	ALString sAttributeName;
	int nUnusedAttributeNumber;
	boolean bAttributeFound;
	KWDGAttribute* targetAttribute;
	KWDGAttribute* defaultTargetAttribute;

	require(CheckDataGrid(dataGrid));

	// Recherche des attributs cibles courant et par defaut
	// Le cout par defaut se rapporte en effet a un attribut cible ayant une seule partie,
	// alors qu'un attribut source non informatif peut disparaitre. Dans ce cas, le cout
	// par defaut de ce dernier doit etre recalcule avec le bon nombre de partie cible
	targetAttribute = dataGrid->GetTargetAttribute();
	defaultTargetAttribute = dataGridDefaultCosts->GetTargetAttribute();
	assert(defaultTargetAttribute == NULL or defaultTargetAttribute->GetPartNumber() == 1);

	// Actualisation du bon nombre de partie cible pour la grille par defaut
	if (defaultTargetAttribute != NULL and targetAttribute != NULL and targetAttribute->GetPartNumber() > 1)
	{
		while (defaultTargetAttribute->GetPartNumber() < targetAttribute->GetPartNumber())
			defaultTargetAttribute->AddPart();
	}

	// Recherche des attributs par defaut ne se trouvant pas dans la grille a evaluer
	// Les attributs sont censes etre dans un ordre coherent
	dTotalCost = 0;
	nUnusedAttributeNumber = 0;
	nAttribute = 0;
	if (nAttribute < dataGrid->GetAttributeNumber())
		sAttributeName = dataGrid->GetAttributeAt(nAttribute)->GetAttributeName();
	for (nDefaultAttribute = 0; nDefaultAttribute < dataGridDefaultCosts->GetAttributeNumber(); nDefaultAttribute++)
	{
		defaultAttribute = dataGridDefaultCosts->GetAttributeAt(nDefaultAttribute);
		defaultAttributeM = cast(KWDGMAttribute*, defaultAttribute);

		// Recherche de l'index de l'attribut correspondant parmi les attribut de la grille partielle
		bAttributeFound = defaultAttribute->GetAttributeName() == sAttributeName;

		// Prise en compte du cout de l'attribut terminal s'il n'est pas utilise dans la grille partielle
		if (not bAttributeFound)
		{
			nUnusedAttributeNumber++;

			// Cout par defaut si une seule partie cible
			if (defaultTargetAttribute == NULL or defaultTargetAttribute->GetPartNumber() == 1)
			{
				// Cout de l'attribut par defaut
				dTotalCost += defaultAttributeM->GetCost();

				// Cout de la partie
				assert(defaultAttribute->GetPartNumber() == 1);
				part = defaultAttribute->GetHeadPart();
				partM = cast(KWDGMPart*, part);
				dTotalCost += partM->GetCost();
			}
			// Sinon, on recalcul les couts des attributs manquants pour tenir compte du nombre
			// de parties cibles
			else
			{
				// Cout de l'attribut par defaut
				assert(defaultAttributeM->GetPartNumber() == 1);
				dTotalCost += ComputeAttributeCost(defaultAttributeM, 1);

				// Cout de la partie
				assert(defaultAttribute->GetPartNumber() == 1);
				part = defaultAttribute->GetHeadPart();
				partM = cast(KWDGMPart*, part);
				dTotalCost += ComputePartCost(partM);
			}
		}
		// Sinon, recherche du prochain nom d'attribut partiel
		else
		{
			nAttribute++;
			if (nAttribute < dataGrid->GetAttributeNumber())
				sAttributeName = dataGrid->GetAttributeAt(nAttribute)->GetAttributeName();
			else
				sAttributeName = "";
		}
	}
	assert(dataGridDefaultCosts->GetAttributeNumber() == dataGrid->GetAttributeNumber() + nUnusedAttributeNumber);

	// Reactualisation a une seule partie cible pour la grille par defaut
	if (defaultTargetAttribute != NULL and targetAttribute != NULL and targetAttribute->GetPartNumber() > 1)
	{
		while (defaultTargetAttribute->GetPartNumber() > 1)
		{
			part = defaultTargetAttribute->GetTailPart();
			defaultTargetAttribute->DeletePart(part);
		}
	}
	assert(defaultTargetAttribute == NULL or defaultTargetAttribute->GetPartNumber() == 1);

	return dTotalCost;
}

double KWDataGridCosts::ComputeDataGridCumulativeCost(const KWDataGrid* dataGrid) const
{
	double dGlobalCost;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGCell* cell;

	require(dataGrid != NULL);

	// Initialisation avec le cout du DataGrid
	dGlobalCost =
	    ComputeDataGridCost(dataGrid, dataGrid->GetLnGridSize(), dataGrid->GetInformativeAttributeNumber());

	// Prise en compte des attributs
	for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = dataGrid->GetAttributeAt(nAttribute);

		// Cout de l'attribut
		dGlobalCost += ComputeAttributeCost(attribute, attribute->GetPartNumber());

		// Cout des parties
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			dGlobalCost += ComputePartCost(part);
			attribute->GetNextPart(part);
		}
	}

	// Prise en compte des cellules
	cell = dataGrid->GetHeadCell();
	while (cell != NULL)
	{
		dGlobalCost += ComputeCellCost(cell);
		dataGrid->GetNextCell(cell);
	}

	return dGlobalCost;
}

double KWDataGridCosts::ComputeAttributeCumulativeCost(const KWDGAttribute* attribute) const
{
	double dGlobalValue;
	KWDGPart* part;

	require(attribute != NULL);

	// Cout de l'attribut
	dGlobalValue = ComputeAttributeCost(attribute, attribute->GetPartNumber());

	// Cout globale des parties
	part = attribute->GetHeadPart();
	while (part != NULL)
	{
		dGlobalValue += ComputePartCumulativeCost(part);
		attribute->GetNextPart(part);
	}

	return dGlobalValue;
}

double KWDataGridCosts::ComputePartCumulativeCost(const KWDGPart* part) const
{
	double dGlobalValue;
	KWDGCell* cell;

	require(part != NULL);

	// Valeur de la partie
	dGlobalValue = ComputePartCost(part);

	// Prise en compte des cellules
	cell = part->GetHeadCell();
	while (cell != NULL)
	{
		dGlobalValue += ComputeCellCost(cell);
		part->GetNextCell(cell);
	}

	return dGlobalValue;
}

double KWDataGridCosts::ComputeValueCost(const KWDGValue* value) const
{
	require(value != NULL);
	return 0;
}

double KWDataGridCosts::ComputeDataGridAllValuesCost(const KWDataGrid* dataGrid) const
{
	double dAllValuesCost;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGValue* value;
	// CH IV Begin
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	// CH IV End

	require(dataGrid != NULL);

	// Parcours des attributs symboliques
	dAllValuesCost = 0;
	for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = dataGrid->GetAttributeAt(nAttribute);

		// Gestion uniquement dans le cas symbolique
		if (attribute->GetAttributeType() == KWType::Symbol)
		{
			// Parcours des parties
			part = attribute->GetHeadPart();
			while (part != NULL)
			{
				// Parcours des valeurs de la partie
				value = part->GetValueSet()->GetHeadValue();
				while (value != NULL)
				{
					// Prise en compte du cout de la valeur
					dAllValuesCost += ComputeValueCost(value);

					// Valeur suivante
					part->GetValueSet()->GetNextValue(value);
				}

				// Partie suivante
				attribute->GetNextPart(part);
			}
		}
		// CH IV Begin

		// Prise en compte du cout des valeurs des attributs symboliques internes dans des attributs de type
		// VarPart
		else if (attribute->GetAttributeType() == KWType::VarPart)
		{
			for (nInnerAttribute = 0; nInnerAttribute < attribute->GetInnerAttributesNumber();
			     nInnerAttribute++)
			{
				innerAttribute = attribute->GetDataGrid()->GetInnerAttributes()->LookupInnerAttribute(
				    attribute->GetInnerAttributeNameAt(nInnerAttribute));
				// Gestion uniquement dans le cas symbolique
				if (innerAttribute->GetAttributeType() == KWType::Symbol)
				{
					// Parcours des parties
					part = innerAttribute->GetHeadPart();
					while (part != NULL)
					{
						// Parcours des valeurs de la partie
						value = part->GetValueSet()->GetHeadValue();
						while (value != NULL)
						{
							// Prise en compte du cout de la valeur
							dAllValuesCost += ComputeValueCost(value);

							// Valeur suivante
							part->GetValueSet()->GetNextValue(value);
						}

						// Partie suivante
						innerAttribute->GetNextPart(part);
					}
				}
			}
		}
		// CH IV End
	}
	return dAllValuesCost;
}

void KWDataGridCosts::WriteDataGridAllCosts(const KWDataGrid* dataGrid, ostream& ost) const
{
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGCell* cell;

	// Cout du DataGrid
	WriteDataGridCost(dataGrid, ost);

	// Couts des attributs
	for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = dataGrid->GetAttributeAt(nAttribute);

		// Cout de l'attribut
		if (nAttribute == 0)
			WriteAttributeCostHeaderLine(attribute, ost);
		WriteAttributeCostLine(attribute, ost);
	}

	// Couts des parties
	for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = dataGrid->GetAttributeAt(nAttribute);

		// Cout des parties
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			if (nAttribute == 0 and part == attribute->GetHeadPart())
				WritePartCostHeaderLine(part, ost);
			WritePartCostLine(part, ost);
			attribute->GetNextPart(part);
		}
	}

	// Cout des cellules
	cell = dataGrid->GetHeadCell();
	while (cell != NULL)
	{
		if (cell == dataGrid->GetHeadCell())
			WriteCellCostHeaderLine(cell, ost);
		WriteCellCostLine(cell, ost);
		dataGrid->GetNextCell(cell);
	}
}

void KWDataGridCosts::WriteDataGridCost(const KWDataGrid* dataGrid, ostream& ost) const
{
	double dDataGridCost;
	double dDataGridTotalCost;

	require(dataGrid != NULL);
	require(IsInitialized());

	// Calcul des couts locaux et globaux de la grille de donnees
	dDataGridCost =
	    ComputeDataGridCost(dataGrid, dataGrid->GetLnGridSize(), dataGrid->GetInformativeAttributeNumber());
	dDataGridTotalCost = ComputeDataGridTotalCost(dataGrid);

	// Affichage
	ost << dataGrid->GetClassLabel() << "\t" << dataGrid->GetObjectLabel() << "\n";
	cout << "\tDataGrid\t" << dDataGridCost << "\n";
	cout << "\tDataGridTotal\t" << dDataGridTotalCost << "\n";
	cout << "\tMissingAttributes\t" << ComputeDataGridTotalMissingAttributeCost(dataGrid) << "\n";
	cout << "\tAllValuesDefaultCost\t" << GetAllValuesDefaultCost() << "\n";
}

void KWDataGridCosts::WriteAttributeCostHeaderLine(const KWDGAttribute* attribute, ostream& ost) const
{
	ost << attribute->GetClassLabel() << "\t"
	    << "ValueNumber\tPartNumber\tCost\tCumulativeCost\n";
}

void KWDataGridCosts::WriteAttributeCostLine(const KWDGAttribute* attribute, ostream& ost) const
{
	double dAttributeCost;
	double dAttributeCumulativeCost;

	require(attribute != NULL);

	// Calcul des couts locaux et globaux de la grille de donnees
	dAttributeCost = ComputeAttributeCost(attribute, attribute->GetPartNumber());
	dAttributeCumulativeCost = ComputeAttributeCumulativeCost(attribute);

	// Affichage
	ost << attribute->GetObjectLabel() << "\t" << attribute->GetTrueValueNumber() << "\t"
	    << attribute->GetPartNumber() << "\t" << dAttributeCost << "\t" << dAttributeCumulativeCost << "\n";
}

void KWDataGridCosts::WritePartCostHeaderLine(const KWDGPart* part, ostream& ost) const
{
	ost << "Partition\tAttribute\tPart\tCellNumber\tFrequency\tValueNumber\tCost\tCumulativeCost\n";
}

void KWDataGridCosts::WritePartCostLine(const KWDGPart* part, ostream& ost) const
{
	double dPartCost;
	double dPartCumulativeCost;

	require(part != NULL);

	// Calcul des couts locaux et globaux de la grille de donnees
	dPartCost = ComputePartCost(part);
	dPartCumulativeCost = ComputePartCumulativeCost(part);

	// Affichage
	ost << part->GetClassLabel() << "\t";
	if (part->GetAttribute() != NULL)
		ost << part->GetAttribute()->GetObjectLabel() << "\t";
	ost << part->GetObjectLabel() << "\t";
	ost << part->GetCellNumber() << "\t" << part->GetPartFrequency() << "\t";
	if (part->GetPartType() == KWType::Symbol)
		ost << part->GetValueSet()->GetTrueValueNumber() << "\t";
	// CH IV Begin
	else if (part->GetPartType() == KWType::Continuous)
		ost << part->GetPartFrequency() << "\t";
	else
		ost << part->GetVarPartSet()->GetVarPartNumber() << "\t";
	// Avant integration coclustering IV ost << part->GetPartFrequency() << "\t";
	// CH IV End
	ost << dPartCost << "\t" << dPartCumulativeCost << "\n";
}

void KWDataGridCosts::WriteCellCostHeaderLine(const KWDGCell* cell, ostream& ost) const
{
	int nTarget;

	ost << cell->GetClassLabel() << "\t";
	for (nTarget = 0; nTarget < cell->GetTargetValueNumber(); nTarget++)
	{
		ost << "Class" << nTarget + 1 << "\t";
	}
	ost << "Total\tCost\n";
}

void KWDataGridCosts::WriteCellCostLine(const KWDGCell* cell, ostream& ost) const
{
	int nTarget;
	int nTotal;

	require(cell != NULL);

	// Nom de la cellule
	ost << cell->GetObjectLabel() << "\t";

	// Distribution des effectifs cibles
	nTotal = cell->GetCellFrequency();
	for (nTarget = 0; nTarget < cell->GetTargetValueNumber(); nTarget++)
	{
		ost << cell->GetTargetFrequencyAt(nTarget) << "\t";
	}
	ost << nTotal << "\t";

	// Couts
	ost << ComputeCellCost(cell) << "\n";
}

double KWDataGridCosts::ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
						 int nInformativeAttributeNumber) const
{
	return 0;
}

double KWDataGridCosts::ComputeAttributeModelCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	return 0;
}

double KWDataGridCosts::ComputePartModelCost(const KWDGPart* part) const
{
	return 0;
}

double KWDataGridCosts::ComputeCellModelCost(const KWDGCell* cell) const
{
	return 0;
}

double KWDataGridCosts::ComputeValueModelCost(const KWDGValue* value) const
{
	return 0;
}

double KWDataGridCosts::ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
							int nInformativeAttributeNumber) const
{
	return 0;
}

double KWDataGridCosts::ComputeAttributeConstructionCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	return 0;
}

double KWDataGridCosts::ComputePartConstructionCost(const KWDGPart* part) const
{
	return 0;
}

double KWDataGridCosts::ComputeCellConstructionCost(const KWDGCell* cell) const
{
	return 0;
}

double KWDataGridCosts::ComputeValueConstructionCost(const KWDGValue* value) const
{
	return 0;
}

double KWDataGridCosts::ComputeDataGridPreparationCost(const KWDataGrid* dataGrid, double dLnGridSize,
						       int nInformativeAttributeNumber) const
{
	double dPreparationCost;
	require(dataGrid != NULL);
	dPreparationCost = ComputeDataGridModelCost(dataGrid, dLnGridSize, nInformativeAttributeNumber) -
			   ComputeDataGridConstructionCost(dataGrid, dLnGridSize, nInformativeAttributeNumber);
	assert(dPreparationCost >= -dEpsilon);
	if (dPreparationCost < dEpsilon)
		dPreparationCost = 0;
	return dPreparationCost;
}

double KWDataGridCosts::ComputeAttributePreparationCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	double dPreparationCost;
	require(attribute != NULL);
	dPreparationCost = ComputeAttributeModelCost(attribute, nPartitionSize) -
			   ComputeAttributeConstructionCost(attribute, nPartitionSize);
	assert(dPreparationCost >= -dEpsilon);
	if (dPreparationCost < dEpsilon)
		dPreparationCost = 0;
	return dPreparationCost;
}

double KWDataGridCosts::ComputePartPreparationCost(const KWDGPart* part) const
{
	double dPreparationCost;
	require(part != NULL);
	dPreparationCost = ComputePartModelCost(part) - ComputePartConstructionCost(part);
	assert(dPreparationCost >= -dEpsilon);
	if (dPreparationCost < dEpsilon)
		dPreparationCost = 0;
	return dPreparationCost;
}

double KWDataGridCosts::ComputeCellPreparationCost(const KWDGCell* cell) const
{
	double dPreparationCost;
	require(cell != NULL);
	dPreparationCost = ComputeCellModelCost(cell) - ComputeCellConstructionCost(cell);
	assert(dPreparationCost >= -dEpsilon);
	if (dPreparationCost < dEpsilon)
		dPreparationCost = 0;
	return dPreparationCost;
}

double KWDataGridCosts::ComputeValuePreparationCost(const KWDGValue* value) const
{
	double dPreparationCost;
	require(value != NULL);
	dPreparationCost = ComputeValueModelCost(value) - ComputeValueConstructionCost(value);
	assert(dPreparationCost >= -dEpsilon);
	if (dPreparationCost < dEpsilon)
		dPreparationCost = 0;
	return dPreparationCost;
}

double KWDataGridCosts::ComputeDataGridDataCost(const KWDataGrid* dataGrid, double dLnGridSize,
						int nInformativeAttributeNumber) const
{
	double dDataCost;
	require(dataGrid != NULL);
	dDataCost = ComputeDataGridCost(dataGrid, dLnGridSize, nInformativeAttributeNumber) -
		    ComputeDataGridModelCost(dataGrid, dLnGridSize, nInformativeAttributeNumber);
	assert(dDataCost >= -dEpsilon);
	if (dDataCost < dEpsilon)
		dDataCost = 0;
	return dDataCost;
}

double KWDataGridCosts::ComputeAttributeDataCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	double dDataCost;
	require(attribute != NULL);
	dDataCost =
	    ComputeAttributeCost(attribute, nPartitionSize) - ComputeAttributeModelCost(attribute, nPartitionSize);
	assert(dDataCost >= -dEpsilon);
	if (dDataCost < dEpsilon)
		dDataCost = 0;
	return dDataCost;
}

double KWDataGridCosts::ComputePartDataCost(const KWDGPart* part) const
{
	double dDataCost;
	require(part != NULL);
	dDataCost = ComputePartCost(part) - ComputePartModelCost(part);
	assert(dDataCost >= -dEpsilon);
	if (dDataCost < dEpsilon)
		dDataCost = 0;
	return dDataCost;
}

double KWDataGridCosts::ComputeCellDataCost(const KWDGCell* cell) const
{
	double dDataCost;
	require(cell != NULL);
	dDataCost = ComputeCellCost(cell) - ComputeCellModelCost(cell);
	assert(dDataCost >= -dEpsilon);
	if (dDataCost < dEpsilon)
		dDataCost = 0;
	return dDataCost;
}

double KWDataGridCosts::ComputeValueDataCost(const KWDGValue* value) const
{
	double dDataCost;
	require(value != NULL);
	dDataCost = ComputeValueCost(value) - ComputeValueModelCost(value);
	assert(dDataCost >= -dEpsilon);
	if (dDataCost < dEpsilon)
		dDataCost = 0;
	return dDataCost;
}

double KWDataGridCosts::ComputeDataGridTotalModelCost(const KWDataGrid* dataGrid) const
{
	double dGlobalModelCost;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGValue* value;
	KWDGCell* cell;

	require(dataGrid != NULL);

	// Initialisation avec le cout du DataGrid
	dGlobalModelCost =
	    ComputeDataGridModelCost(dataGrid, dataGrid->GetLnGridSize(), dataGrid->GetInformativeAttributeNumber());

	// Prise en compte des attributs
	for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = dataGrid->GetAttributeAt(nAttribute);

		// Cout de l'attribut
		dGlobalModelCost += ComputeAttributeModelCost(attribute, attribute->GetPartNumber());

		// Cout des parties
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			dGlobalModelCost += ComputePartModelCost(part);
			attribute->GetNextPart(part);
		}

		// Cout des valeurs uniquement dans le cas symbolique
		if (attribute->GetAttributeType() == KWType::Symbol)
		{
			// Parcours des parties
			part = attribute->GetHeadPart();
			while (part != NULL)
			{
				// Parcours des valeurs de la partie
				value = part->GetValueSet()->GetHeadValue();
				while (value != NULL)
				{
					// Prise en compte du cout de la valeur
					assert(ComputeValueModelCost(value) == 0);
					dGlobalModelCost += ComputeValueModelCost(value);

					// Valeur suivante
					part->GetValueSet()->GetNextValue(value);
				}

				// Partie suivante
				attribute->GetNextPart(part);
			}
		}
	}

	// Prise en compte des cellules
	cell = dataGrid->GetHeadCell();
	while (cell != NULL)
	{
		dGlobalModelCost += ComputeCellModelCost(cell);
		dataGrid->GetNextCell(cell);
	}

	return dGlobalModelCost;
}

double KWDataGridCosts::ComputeDataGridTotalConstructionCost(const KWDataGrid* dataGrid) const
{
	double dGlobalConstructionCost;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGValue* value;
	KWDGCell* cell;

	require(dataGrid != NULL);

	// Initialisation avec le cout du DataGrid
	dGlobalConstructionCost = ComputeDataGridConstructionCost(dataGrid, dataGrid->GetLnGridSize(),
								  dataGrid->GetInformativeAttributeNumber());

	// Prise en compte des attributs
	for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = dataGrid->GetAttributeAt(nAttribute);

		// Cout de l'attribut
		dGlobalConstructionCost += ComputeAttributeConstructionCost(attribute, attribute->GetPartNumber());

		// Cout des parties
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			dGlobalConstructionCost += ComputePartConstructionCost(part);
			attribute->GetNextPart(part);
		}

		// Cout des valeurs uniquement dans le cas symbolique
		if (attribute->GetAttributeType() == KWType::Symbol)
		{
			// Parcours des parties
			part = attribute->GetHeadPart();
			while (part != NULL)
			{
				// Parcours des valeurs de la partie
				value = part->GetValueSet()->GetHeadValue();
				while (value != NULL)
				{
					// Prise en compte du cout de la valeur
					assert(ComputeValueConstructionCost(value) == 0);
					dGlobalConstructionCost += ComputeValueConstructionCost(value);

					// Valeur suivante
					part->GetValueSet()->GetNextValue(value);
				}

				// Partie suivante
				attribute->GetNextPart(part);
			}
		}
	}

	// Prise en compte des cellules
	cell = dataGrid->GetHeadCell();
	while (cell != NULL)
	{
		dGlobalConstructionCost += ComputeCellConstructionCost(cell);
		dataGrid->GetNextCell(cell);
	}

	return dGlobalConstructionCost;
}

double KWDataGridCosts::ComputeDataGridTotalPreparationCost(const KWDataGrid* dataGrid) const
{
	double dPreparationCost;
	require(dataGrid != NULL);
	dPreparationCost = ComputeDataGridTotalModelCost(dataGrid) - ComputeDataGridTotalConstructionCost(dataGrid);
	assert(dPreparationCost >= -dEpsilon);
	if (dPreparationCost < dEpsilon)
		dPreparationCost = 0;
	return dPreparationCost;
}

double KWDataGridCosts::ComputeDataGridTotalDataCost(const KWDataGrid* dataGrid) const
{
	double dDataCost;
	require(dataGrid != NULL);
	dDataCost = ComputeDataGridTotalCost(dataGrid) - ComputeDataGridTotalModelCost(dataGrid);
	assert(dDataCost >= -dEpsilon);
	if (dDataCost < dEpsilon)
		dDataCost = 0;
	return dDataCost;
}

double KWDataGridCosts::ComputePartTargetDeltaCost(const KWDGPart* part) const
{
	require(part != NULL);
	return 0;
}

const ALString KWDataGridCosts::GetClassLabel() const
{
	return "Data grid costs";
}

////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridClassificationCosts

KWDataGridClassificationCosts::KWDataGridClassificationCosts() {}

KWDataGridClassificationCosts::~KWDataGridClassificationCosts() {}

KWDataGridCosts* KWDataGridClassificationCosts::Clone() const
{
	return new KWDataGridClassificationCosts;
}

double KWDataGridClassificationCosts::ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
							  int nInformativeAttributeNumber) const
{
	double dDataGridCost;
	int nGranularity;
	int nGranularityMax;

	require(dataGrid != NULL);
	require(dLnGridSize >= 0);
	require(nInformativeAttributeNumber >= 0);
	require(nInformativeAttributeNumber <= GetTotalAttributeNumber());

	// Initialisation de la granularite courante et maximale
	nGranularity = dataGrid->GetGranularity();
	if (nGranularity > 0)
		nGranularityMax = (int)ceil(log(dataGrid->GetGridFrequency() * 1.0) / log(2.0));
	else
		nGranularityMax = 0;

	assert(nGranularity > 0 or nInformativeAttributeNumber == 0);

	// Cout du choix entre modele nul et modele informatif
	dDataGridCost = log(2.0);

	// Cout de selection des variables
	if (nInformativeAttributeNumber > 0)
	{
		dDataGridCost += GetModelFamilySelectionCost();
		dDataGridCost += KWStat::NaturalNumbersUniversalCodeLength(nInformativeAttributeNumber);
		dDataGridCost -= KWStat::LnFactorial(nInformativeAttributeNumber);

		// Cout de la granularite
		dDataGridCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);
	}
	return dDataGridCost;
}

double KWDataGridClassificationCosts::ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	double dAttributeCost;
	int nPartileNumber;
	int nGarbageModalityNumber;

	require(attribute != NULL);
	require(attribute->GetGranularizedValueNumber() > 0);
	require(KWType::IsSimple(attribute->GetAttributeType()));

	// Nouveaux cas : poubelle avec ou sans granu; granu sans poubelle
	nPartileNumber = attribute->GetGranularizedValueNumber();

	require(nPartileNumber > 0);
	require(nPartitionSize >= 1);
	require(nPartitionSize <= nPartileNumber);
	// Une partition avec poubelle contient au moins 2 parties informatives + 1 groupe poubelle
	require(attribute->GetGarbageModalityNumber() == 0 or nPartitionSize >= 3);

	// Cout uniquement si attribut selectionne (partition univariee non nulle)
	if (nPartitionSize == 1)
		dAttributeCost = 0;
	// Sinon
	else
	{
		assert(nPartileNumber > 1);

		// Initialisation au cout de selection/construction
		dAttributeCost = attribute->GetCost();

		// Cout de structure si attribut continu
		if (attribute->GetAttributeType() == KWType::Continuous)
		{
			// Cout de codage du nombre d'intervalles
			dAttributeCost +=
			    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1, nPartileNumber - 1);

			// Nouveau codage avec description du choix des coupures selon une multinomiale
			dAttributeCost += (nPartitionSize - 1) * log((nPartileNumber - 1) * 1.0);
			dAttributeCost -= KWStat::LnFactorial(nPartitionSize - 1);
		}
		// Cout de structure si attribut symbolique
		else
		{
			// Taille de la poubelle
			nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

			// Cout de codage du choix d'une poubelle ou pas
			if (nPartileNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
				dAttributeCost += log(2.0);

			// Cout de codage du choix des modalites informatives (hors poubelle)
			if (nGarbageModalityNumber > 0)
			{
				// Cout de codage de la taille de la non-poubelle
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartileNumber - nGarbageModalityNumber - 1, nPartileNumber - 2);

				// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage par
				// variable
				dAttributeCost +=
				    (nPartileNumber - nGarbageModalityNumber) * log(nPartileNumber * 1.0) -
				    KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

				// Cout de codage du nombre de parties informatives (nPartitionSize-1)
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartitionSize - 2, nPartileNumber - nGarbageModalityNumber - 1);

				// Cout de codage du choix des parties informatives (nPartitionSize - 1)
				dAttributeCost +=
				    KWStat::LnBell(nPartileNumber - nGarbageModalityNumber, nPartitionSize - 1);
			}
			else
			{
				// Cout de codage du nombre de parties
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1,
												   nPartileNumber - 1);

				// Cout de codage du choix des parties
				dAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
			}
		}
	}
	return dAttributeCost;
}

double KWDataGridClassificationCosts::ComputePartCost(const KWDGPart* part) const
{
	require(part != NULL);
	return 0;
}

double KWDataGridClassificationCosts::ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const
{
	require(part1 != NULL);
	require(part2 != NULL);
	return 0;
}

double KWDataGridClassificationCosts::ComputeCellCost(const KWDGCell* cell) const
{
	double dCellCost;
	int nTargetValueNumber;
	int nFrequency;
	int nCellFrequency;
	int i;

	require(cell != NULL);
	require(cell->GetTargetValueNumber() >= 1);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne
	dCellCost = 0;
	nCellFrequency = 0;
	nTargetValueNumber = cell->GetTargetValueNumber();
	for (i = 0; i < nTargetValueNumber; i++)
	{
		nFrequency = cell->GetTargetFrequencyAt(i);
		dCellCost -= KWStat::LnFactorial(nFrequency);
		nCellFrequency += nFrequency;
	}
	dCellCost += KWStat::LnFactorial(nCellFrequency + nTargetValueNumber - 1);
	dCellCost -= KWStat::LnFactorial(nTargetValueNumber - 1);
	return dCellCost;
}
// CH IV Begin
double KWDataGridClassificationCosts::ComputeInnerAttributeCost(const KWDGAttribute* attribute,
								int nPartitionSize) const
{
	assert(false);
	return 0;
}

double KWDataGridClassificationCosts::ComputeInnerAttributePartCost(const KWDGPart* part) const
{
	assert(false);
	return 0;
}
// CH IV End
double KWDataGridClassificationCosts::ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
								      int nInformativeAttributeNumber) const
{
	double dGranularityCost;
	int nGranularity;
	int nGranularityMax;

	// Prise en compte du cout de granularite en cas de variable informatives
	dGranularityCost = 0;
	if (nInformativeAttributeNumber > 0)
	{
		// calcul du cout a partir de la granularite courante et maximale
		nGranularity = dataGrid->GetGranularity();
		if (nGranularity > 0)
			nGranularityMax = (int)ceil(log(dataGrid->GetGridFrequency() * 1.0) / log(2.0));
		else
			nGranularityMax = 0;
		dGranularityCost = KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);
	}

	// Le cout de construction est egal au cout total moins le cout du choix de la granularite
	return ComputeDataGridCost(dataGrid, dLnGridSize, nInformativeAttributeNumber) - dGranularityCost;
}

double KWDataGridClassificationCosts::ComputeAttributeConstructionCost(const KWDGAttribute* attribute,
								       int nPartitionSize) const
{
	if (nPartitionSize > 1)
		return attribute->GetCost();
	else
		return 0;
}

double KWDataGridClassificationCosts::ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
							       int nInformativeAttributeNumber) const
{
	// Le cout de modele est egal au cout total
	return ComputeDataGridCost(dataGrid, dLnGridSize, nInformativeAttributeNumber);
}

double KWDataGridClassificationCosts::ComputeAttributeModelCost(const KWDGAttribute* attribute,
								int nPartitionSize) const
{
	// Le cout de modele est egal au cout total
	return ComputeAttributeCost(attribute, nPartitionSize);
}

double KWDataGridClassificationCosts::ComputePartModelCost(const KWDGPart* part) const
{
	return 0;
}

double KWDataGridClassificationCosts::ComputeCellModelCost(const KWDGCell* cell) const
{
	double dCellCost;
	int nTargetValueNumber;
	int nCellFrequency;
	int i;

	require(cell != NULL);
	require(cell->GetTargetValueNumber() >= 1);

	// Cout de codage de la loi multinomiale de la ligne
	dCellCost = 0;
	nCellFrequency = 0;
	nTargetValueNumber = cell->GetTargetValueNumber();
	for (i = 0; i < nTargetValueNumber; i++)
		nCellFrequency += cell->GetTargetFrequencyAt(i);
	dCellCost += KWStat::LnFactorial(nCellFrequency + nTargetValueNumber - 1);
	dCellCost -= KWStat::LnFactorial(nTargetValueNumber - 1);
	dCellCost -= KWStat::LnFactorial(nCellFrequency);
	return dCellCost;
}

double KWDataGridClassificationCosts::ComputeValueModelCost(const KWDGValue* value) const
{
	return 0;
}

const ALString KWDataGridClassificationCosts::GetClassLabel() const
{
	return "Data grid classification costs";
}

////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridClusteringCosts

KWDataGridClusteringCosts::KWDataGridClusteringCosts() {}

KWDataGridClusteringCosts::~KWDataGridClusteringCosts() {}

KWDataGridCosts* KWDataGridClusteringCosts::Clone() const
{
	return new KWDataGridClusteringCosts;
}

double KWDataGridClusteringCosts::ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
						      int nInformativeAttributeNumber) const
{
	double dDataGridCost;
	int nGridSize;

	require(dataGrid != NULL);
	require(dLnGridSize >= 0);
	require(nInformativeAttributeNumber >= 0);
	require(nInformativeAttributeNumber <= GetTotalAttributeNumber());

	// Cout du choix entre modele nul et modele informatif
	dDataGridCost = log(2.0);

	// Cout de selection des variables (modele informatif)
	if (nInformativeAttributeNumber > 0)
	{
		dDataGridCost += GetModelFamilySelectionCost();
		dDataGridCost += KWStat::NaturalNumbersUniversalCodeLength(nInformativeAttributeNumber);
		dDataGridCost -= KWStat::LnFactorial(nInformativeAttributeNumber);
	}

	// Acces a la taille de la grille si elle n'est pas trop grande
	nGridSize = -1;
	if (dLnGridSize < log(INT_MAX / 2.0))
		nGridSize = int(floor(exp(dLnGridSize) + 0.5));

	// Valeur exacte si la grille n'est pas trop grande
	if (nGridSize > 0)
	{
		// Distribution des individus sur les cellules de la grille (parametres de la multi-nomiale)
		// plus numerateur (N!) du terme de multinome de la distribution effective (terme de multinome)
		dDataGridCost += KWStat::LnFactorial(dataGrid->GetGridFrequency() + nGridSize - 1) -
				 KWStat::LnFactorial(nGridSize - 1);
	}
	// Approximation sinon
	else
	{
		dDataGridCost += dataGrid->GetGridFrequency() * dLnGridSize;
	}

	return dDataGridCost;
}

double KWDataGridClusteringCosts::ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	double dAttributeCost;
	int nPartileNumber;
	int nGarbageModalityNumber;

	require(attribute != NULL);
	require(attribute->GetTrueValueNumber() > 0);
	require(KWType::IsSimple(attribute->GetAttributeType()));

	nPartileNumber = attribute->GetTrueValueNumber();
	// Sans prise en compte granularite : pas de sens en non supervise
	require(nPartileNumber > 0);
	require(nPartitionSize >= 1);
	require(nPartitionSize <= nPartileNumber);
	// Une partition avec poubelle contient au moins 2 parties informatives + 1 groupe poubelle
	require(attribute->GetGarbageModalityNumber() == 0 or nPartitionSize >= 3);

	// Cout nul si partition nulle
	if (nPartitionSize == 1)
		dAttributeCost = 0;
	else
	{
		assert(nPartileNumber > 1);

		// Initialisation au cout de selection/construction
		dAttributeCost = attribute->GetCost();

		// Cout de structure si attribut continu
		if (attribute->GetAttributeType() == KWType::Continuous)
		{
			// Cout de codage du nombre d'intervalles
			dAttributeCost +=
			    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1, nPartileNumber - 1);
		}
		// Cout de structure si attribut symbolique
		else
		{
			// Taille de la poubelle
			nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

			// Cout de codage du choix d'une poubelle ou pas
			if (nPartileNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
				dAttributeCost += log(2.0);

			// Cout de codage du choix des modalites informatives (hors poubelle)
			if (nGarbageModalityNumber > 0)
			{
				// Cout de codage de la taille de la non-poubelle
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartileNumber - nGarbageModalityNumber - 1, nPartileNumber - 2);

				// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage par
				// variable
				dAttributeCost +=
				    (nPartileNumber - nGarbageModalityNumber) * log(nPartileNumber * 1.0) -
				    KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

				// Cout de codage du nombre de parties informatives (nPartitionSize-1)
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartitionSize - 2, nPartileNumber - nGarbageModalityNumber - 1);

				// Cout de codage du choix des parties informatives (nPartitionSize - 1)
				dAttributeCost +=
				    KWStat::LnBell(nPartileNumber - nGarbageModalityNumber, nPartitionSize - 1);
			}
			else
			{
				// Cout de codage du nombre de parties
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1,
												   nPartileNumber - 1);

				// Cout de codage du choix des parties
				dAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
			}
		}
	}
	return dAttributeCost;
}

double KWDataGridClusteringCosts::ComputePartCost(const KWDGPart* part) const
{
	double dPartCost;
	int nValueNumber;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));

	// Cout de rangement des instances dans le cas continu
	if (part->GetAttribute()->GetAttributeType() == KWType::Continuous)
	{
		dPartCost = KWStat::LnFactorial(part->GetPartFrequency());
	}
	// Cout de distribution des valeurs (modele + donnees) dans le cas symbolique
	// On ignore le cout LnFactorial par valeur, qui est constant quelle que soit la grille
	else
	{
		dPartCost = 0;
		if (part->GetPartFrequency() > 0)
		{
			nValueNumber = part->GetValueSet()->GetTrueValueNumber();
			dPartCost = KWStat::LnFactorial(part->GetPartFrequency());

			// Distribution des valeurs dans la partie
			dPartCost += KWStat::LnFactorial(part->GetPartFrequency() + nValueNumber - 1);
			dPartCost -= KWStat::LnFactorial(nValueNumber - 1);
			dPartCost -= KWStat::LnFactorial(part->GetPartFrequency());
		}
	}
	return dPartCost;
}

double KWDataGridClusteringCosts::ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const
{
	double dPartUnionCost;
	int nPartFrequency;
	int nValueNumber;

	require(part1 != NULL);
	require(part2 != NULL);
	require(part1->GetAttribute() == part2->GetAttribute());
	require(KWType::IsSimple(part1->GetAttribute()->GetAttributeType()));

	// Cout de rangement des instances dans le cas continu
	nPartFrequency = part1->GetPartFrequency() + part2->GetPartFrequency();
	if (part1->GetAttribute()->GetAttributeType() == KWType::Continuous)
	{
		dPartUnionCost = KWStat::LnFactorial(nPartFrequency);
	}
	// Cout de distribution des valeurs (modele + donnees) dans le cas symbolique
	// On ignore le cout LnFactorial par valeur, qui est constant quelle que soit la grille
	else
	{
		nValueNumber = part1->GetValueSet()->GetTrueValueNumber() + part2->GetValueSet()->GetTrueValueNumber();
		dPartUnionCost = KWStat::LnFactorial(nPartFrequency);

		// Distribution des valeurs dans la partie
		dPartUnionCost += KWStat::LnFactorial(nPartFrequency + nValueNumber - 1);
		dPartUnionCost -= KWStat::LnFactorial(nValueNumber - 1);
		dPartUnionCost -= KWStat::LnFactorial(nPartFrequency);
	}
	return dPartUnionCost;
}

double KWDataGridClusteringCosts::ComputeCellCost(const KWDGCell* cell) const
{
	double dCellCost;

	require(cell != NULL);

	dCellCost = -KWStat::LnFactorial(cell->GetCellFrequency());
	return dCellCost;
}
// CH IV Begin
double KWDataGridClusteringCosts::ComputeInnerAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	assert(false);
	return 0;
}

double KWDataGridClusteringCosts::ComputeInnerAttributePartCost(const KWDGPart* part) const
{
	assert(false);
	return 0;
}
// CH IV End

double KWDataGridClusteringCosts::ComputeValueCost(const KWDGValue* value) const
{
	double dValueCost;

	require(value != NULL);

	dValueCost = -KWStat::LnFactorial(value->GetValueFrequency());
	return dValueCost;
}

double KWDataGridClusteringCosts::ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
								  int nInformativeAttributeNumber) const
{
	double dDataGridConstructionCost;

	// Cout du choix entre modele nul et modele informatif
	dDataGridConstructionCost = log(2.0);

	// Cout de selection des variables (modele informatif)
	if (nInformativeAttributeNumber > 0)
	{
		dDataGridConstructionCost += GetModelFamilySelectionCost();
		dDataGridConstructionCost += KWStat::NaturalNumbersUniversalCodeLength(nInformativeAttributeNumber);
		dDataGridConstructionCost -= KWStat::LnFactorial(nInformativeAttributeNumber);
	}
	return dDataGridConstructionCost;
}

double KWDataGridClusteringCosts::ComputeAttributeConstructionCost(const KWDGAttribute* attribute,
								   int nPartitionSize) const
{
	if (nPartitionSize > 1)
		return attribute->GetCost();
	else
		return 0;
}

double KWDataGridClusteringCosts::ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
							   int nInformativeAttributeNumber) const
{
	double dDataGridCost;

	// Le cout de modele est egal au cout total moins ln N!
	dDataGridCost = ComputeDataGridCost(dataGrid, dLnGridSize, nInformativeAttributeNumber);
	dDataGridCost -= KWStat::LnFactorial(dataGrid->GetGridFrequency());
	assert(dDataGridCost >= 0);
	return dDataGridCost;
}

double KWDataGridClusteringCosts::ComputeAttributeModelCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	// Le cout de modele est egal au cout total
	return ComputeAttributeCost(attribute, nPartitionSize);
}

double KWDataGridClusteringCosts::ComputePartModelCost(const KWDGPart* part) const
{
	double dPartCost;
	int nValueNumber;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));

	// Cout de distribution des valeurs dans le cas categoriel
	dPartCost = 0;
	if (part->GetAttribute()->GetAttributeType() == KWType::Symbol)
	{
		if (part->GetPartFrequency() > 0)
		{
			nValueNumber = part->GetValueSet()->GetTrueValueNumber();

			// Distribution des valeurs dans la partie
			dPartCost += KWStat::LnFactorial(part->GetPartFrequency() + nValueNumber - 1);
			dPartCost -= KWStat::LnFactorial(nValueNumber - 1);
			dPartCost -= KWStat::LnFactorial(part->GetPartFrequency());
		}
	}
	return dPartCost;
}

double KWDataGridClusteringCosts::ComputeCellModelCost(const KWDGCell* cell) const
{
	return 0;
}

double KWDataGridClusteringCosts::ComputeValueModelCost(const KWDGValue* value) const
{
	return 0;
}

const ALString KWDataGridClusteringCosts::GetClassLabel() const
{
	return "Data grid clustering costs";
}

////////////////////////////////////////////////////////////////////////////////////////
// CH IV Begin
// Classe KWVarPartDataGridClusteringCosts

KWVarPartDataGridClusteringCosts::KWVarPartDataGridClusteringCosts() {}

KWVarPartDataGridClusteringCosts::~KWVarPartDataGridClusteringCosts() {}

KWDataGridCosts* KWVarPartDataGridClusteringCosts::Clone() const
{
	return new KWVarPartDataGridClusteringCosts;
}

double KWVarPartDataGridClusteringCosts::ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
							     int nInformativeAttributeNumber) const
{
	double dDataGridCost;
	int nGridSize;

	require(dataGrid != NULL);
	require(dLnGridSize >= 0);
	require(nInformativeAttributeNumber >= 0);
	require(nInformativeAttributeNumber <= GetTotalAttributeNumber());
	require(dataGrid->GetVarPartDataGrid());

	// Coût du choix entre modele nul et modele informatif
	dDataGridCost = log(2.0);

	// Pas de cout de selection de variables dans ce cas de coclustering

	// Acces a la taille de la grille si elle n'est pas trop grande
	nGridSize = -1;
	if (dLnGridSize < log(INT_MAX / 2.0))
		nGridSize = int(floor(exp(dLnGridSize) + 0.5));

	// Valeur exacte si la grille n'est pas trop grande
	if (nGridSize > 0)
	{
		// Distribution des individus sur les cellules de la grille (parametres de la multi-nomiale)
		// plus numerateur (N!) du terme de multinome de la distribution effective (terme de multinome)
		dDataGridCost += KWStat::LnFactorial(dataGrid->GetGridFrequency() + nGridSize - 1) -
				 KWStat::LnFactorial(nGridSize - 1);
	}
	// Approximation sinon
	else
	{
		dDataGridCost += dataGrid->GetGridFrequency() * dLnGridSize;
	}

	return dDataGridCost;
}

double KWVarPartDataGridClusteringCosts::ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	double dAttributeCost;
	int nPartileNumber;
	int nGarbageModalityNumber;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	KWDGPart* innerAttributePart;

	require(attribute != NULL);
	require(attribute->GetTrueValueNumber() > 0);
	require(KWType::IsSimple(attribute->GetAttributeType()) or attribute->GetAttributeType() == KWType::VarPart);
	require(attribute->GetDataGrid()->GetVarPartDataGrid());

	nPartileNumber = attribute->GetTrueValueNumber();
	// Sans prise en compte granularite : pas de sens en non supervise
	require(nPartileNumber > 0);
	require(nPartitionSize >= 1);
	require(nPartitionSize <= nPartileNumber);
	// Une partition avec poubelle contient au moins 2 parties informatives + 1 groupe poubelle
	require(attribute->GetGarbageModalityNumber() == 0 or nPartitionSize >= 3);

	// Cout nul si partition nulle
	if (nPartitionSize == 1)
		dAttributeCost = 0;
	else
	{
		assert(nPartileNumber > 1);

		// Initialisation au cout de selection/construction
		dAttributeCost = attribute->GetCost();

		// Cout de structure si attribut continu
		if (attribute->GetAttributeType() == KWType::Continuous)
		{
			// Cout de codage du nombre d'intervalles
			dAttributeCost +=
			    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1, nPartileNumber - 1);
		}
		// Cout de structure si attribut symbolique ou de type parties de variable
		else if (attribute->GetAttributeType() == KWType::Symbol or
			 (attribute->GetAttributeType() == KWType::VarPart and GetVarPartAttributeGarbage()))
		{
			// Taille de la poubelle
			nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

			// Cout de codage du choix d'une poubelle ou pas
			if (nPartileNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
				dAttributeCost += log(2.0);

			// Cout de codage du choix des modalites informatives (hors poubelle)
			if (nGarbageModalityNumber > 0)
			{
				// Cout de codage de la taille de la non-poubelle
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartileNumber - nGarbageModalityNumber - 1, nPartileNumber - 2);

				// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage par
				// variable
				dAttributeCost +=
				    (nPartileNumber - nGarbageModalityNumber) * log(nPartileNumber * 1.0) -
				    KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

				// Cout de codage du nombre de parties informatives (nPartitionSize-1)
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartitionSize - 2, nPartileNumber - nGarbageModalityNumber - 1);

				// Cout de codage du choix des parties informatives (nPartitionSize - 1)
				dAttributeCost +=
				    KWStat::LnBell(nPartileNumber - nGarbageModalityNumber, nPartitionSize - 1);
			}
			else
			{
				// Cout de codage du nombre de parties
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1,
												   nPartileNumber - 1);

				// Cout de codage du choix des parties
				dAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
			}
		}
		// Cout de structure si attribut de type parties de variable sans prise en compte d'un groupe poubelle
		// CH AB AF temporaire : obsolete a l'integration definitive du groupe poubelle
		else
		{
			// Cout de codage du nombre de clusters de parties de variable
			dAttributeCost +=
			    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1, nPartileNumber - 1);

			// Cout de codage du choix des parties de variable
			dAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
		}
	}

	// Cas d'une grille et d'un attribut de type VarPart
	// Prise en compte du cout des attributs internes (quelle que soit la taille de la partition)
	if (attribute->GetDataGrid()->GetVarPartDataGrid() and attribute->GetAttributeType() == KWType::VarPart)
	{
		// CH AB AF pour optimiser les calculs, memoriser le dInnerAttributeCost et ne le calculer que si
		// necessaire (changement de partition ou post-fusion) Prise en compte du cout des attributs internes
		for (nInnerAttribute = 0; nInnerAttribute < attribute->GetInnerAttributesNumber(); nInnerAttribute++)
		{
			innerAttribute = attribute->GetDataGrid()->GetInnerAttributes()->LookupInnerAttribute(
			    attribute->GetInnerAttributeNameAt(nInnerAttribute));

			// Cas d'un attribut internes qui ne fait pas partie du modele nul
			if (innerAttribute->GetPartNumber() > 1 or nPartitionSize > 1)
			{
				dAttributeCost +=
				    ComputeInnerAttributeCost(innerAttribute, innerAttribute->GetPartNumber());

				innerAttributePart = innerAttribute->GetHeadPart();
				// Prise en compte du cout des parties internes
				while (innerAttributePart != NULL)
				{
					dAttributeCost += ComputeInnerAttributePartCost(innerAttributePart);
					innerAttribute->GetNextPart(innerAttributePart);
				}
			}
		}
	}

	return dAttributeCost;
}

double KWVarPartDataGridClusteringCosts::ComputeInnerAttributeCost(const KWDGAttribute* attribute,
								   int nPartitionSize) const
{
	double dInnerAttributeCost;
	int nGarbageModalityNumber;
	int nPartileNumber;

	require(attribute->GetOwnerAttributeName() != "");
	require(KWType::IsSimple(attribute->GetAttributeType()));
	require(attribute->GetTrueValueNumber() > 0);

	// Initialisation
	dInnerAttributeCost = 0;
	nPartileNumber = attribute->GetTrueValueNumber();

	// Cas d'un attribut continu
	if (attribute->GetAttributeType() == KWType::Continuous)
	{
		// Cout de codage du nombre d'intervalles entre 1 et nPartileNumber (la partition peut etre de taille 1)
		dInnerAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize, nPartileNumber);
	}
	// Sinon attribut symbolique
	else
	{
		// CH AB temporaire : a conserver
		// cout prenant en compte groupe poubelle
		if (GetInnerAttributeGarbage())
		{
			// Taille de la poubelle
			nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

			// Cout de codage du choix d'une poubelle ou pas
			if (nPartileNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
				dInnerAttributeCost += log(2.0);

			// Cout de codage du choix des modalites informatives (hors poubelle)
			if (nGarbageModalityNumber > 0)
			{
				// Cout de codage de la taille de la non-poubelle : superieur ou egal a 1 (mais pas a 2
				// comme en classification supervisee)
				dInnerAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartileNumber - nGarbageModalityNumber, nPartileNumber - 1);

				// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage par
				// variable
				dInnerAttributeCost +=
				    (nPartileNumber - nGarbageModalityNumber) * log(nPartileNumber * 1.0) -
				    KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

				// Cout de codage du nombre de parties informatives (nPartitionSize-1)
				dInnerAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartitionSize - 1, nPartileNumber - nGarbageModalityNumber);

				// Cout de codage du choix des parties informatives (nPartitionSize - 1)
				dInnerAttributeCost +=
				    KWStat::LnBell(nPartileNumber - nGarbageModalityNumber, nPartitionSize - 1);
			}
			else
			{
				// Cout de codage du nombre de parties entre 1 et nPartileNumber (la partition peut etre
				// de taille 1)
				dInnerAttributeCost +=
				    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize, nPartileNumber);

				// Cout de codage du choix des parties
				dInnerAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
			}
		}
		// CH AB AF temporaire : obsolete apres integration groupe poubelle
		else
		{
			// Cout de codage du nombre de parties entre 1 et nPartileNumber (la partition peut etre de
			// taille 1)
			dInnerAttributeCost +=
			    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize, nPartileNumber);

			// Cout de codage du choix des parties
			dInnerAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
		}
	}

	return dInnerAttributeCost;
}

double KWVarPartDataGridClusteringCosts::ComputeInnerAttributePartCost(const KWDGPart* part) const
{
	double dInnerAttributePartCost;
	int nValueNumber;

	require(part->GetAttribute()->GetOwnerAttributeName() != "");

	dInnerAttributePartCost = 0;

	if (part->GetPartType() == KWType::Symbol)
	{
		nValueNumber = part->GetValueSet()->GetTrueValueNumber();
		// Distribution des valeurs dans la partie
		dInnerAttributePartCost += KWStat::LnFactorial(part->GetPartFrequency() + nValueNumber - 1);
		dInnerAttributePartCost -= KWStat::LnFactorial(nValueNumber - 1);
		dInnerAttributePartCost -= KWStat::LnFactorial(part->GetPartFrequency());
	}
	return dInnerAttributePartCost;
}

double KWVarPartDataGridClusteringCosts::ComputePartCost(const KWDGPart* part) const
{
	double dPartCost;
	int nValueNumber;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()) or
		part->GetAttribute()->GetAttributeType() == KWType::VarPart);
	require(part->GetAttribute()->GetDataGrid()->GetVarPartDataGrid());

	// Cout de rangement des instances dans le cas continu
	if (part->GetAttribute()->GetAttributeType() == KWType::Continuous)
	{
		dPartCost = KWStat::LnFactorial(part->GetPartFrequency());
	}
	// Cout de distribution des valeurs (modele + donnees) dans le cas symbolique
	// On ignore le cout LnFactorial par valeur, qui est constant quelle que soit la grille
	else
	{
		dPartCost = 0;
		if (part->GetPartFrequency() > 0)
		{
			if (part->GetAttribute()->GetAttributeType() == KWType::Symbol)
				nValueNumber = part->GetValueSet()->GetTrueValueNumber();
			else
				nValueNumber = part->GetVarPartSet()->GetVarPartNumber();
			dPartCost = KWStat::LnFactorial(part->GetPartFrequency());

			// Distribution des valeurs dans la partie
			dPartCost += KWStat::LnFactorial(part->GetPartFrequency() + nValueNumber - 1);
			dPartCost -= KWStat::LnFactorial(nValueNumber - 1);
			dPartCost -= KWStat::LnFactorial(part->GetPartFrequency());
		}
	}

	return dPartCost;
}

double KWVarPartDataGridClusteringCosts::ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const
{
	double dPartUnionCost;
	int nPartFrequency;
	int nValueNumber;
	int nVarPartNumber;

	require(part1 != NULL);
	require(part2 != NULL);
	require(part1->GetAttribute() == part2->GetAttribute());
	require(part1->GetAttribute()->GetDataGrid()->GetVarPartDataGrid());
	require(KWType::IsSimple(part1->GetAttribute()->GetAttributeType()) or
		part1->GetAttribute()->GetAttributeType() == KWType::VarPart);

	// Cout de rangement des instances dans le cas continu
	nPartFrequency = part1->GetPartFrequency() + part2->GetPartFrequency();
	if (part1->GetAttribute()->GetAttributeType() == KWType::Continuous)
	{
		dPartUnionCost = KWStat::LnFactorial(nPartFrequency);
	}
	// Cout de distribution des valeurs (modele + donnees) dans le cas symbolique
	// On ignore le cout LnFactorial par valeur, qui est constant quelle que soit la grille
	else if (part1->GetAttribute()->GetAttributeType() == KWType::Symbol)
	{
		nValueNumber = part1->GetValueSet()->GetTrueValueNumber() + part2->GetValueSet()->GetTrueValueNumber();
		dPartUnionCost = KWStat::LnFactorial(nPartFrequency);

		// Distribution des valeurs dans la partie
		dPartUnionCost += KWStat::LnFactorial(nPartFrequency + nValueNumber - 1);
		dPartUnionCost -= KWStat::LnFactorial(nValueNumber - 1);
		dPartUnionCost -= KWStat::LnFactorial(nPartFrequency);
	}
	// Cout de distribution des parties de variable dans le cas VarPart
	else
	{
		nVarPartNumber =
		    part1->GetVarPartSet()->GetVarPartNumber() + part2->GetVarPartSet()->GetVarPartNumber();
		dPartUnionCost = KWStat::LnFactorial(nPartFrequency);

		// Distribution des parties de variable dans la partie
		dPartUnionCost += KWStat::LnFactorial(nPartFrequency + nVarPartNumber - 1);
		dPartUnionCost -= KWStat::LnFactorial(nVarPartNumber - 1);
		dPartUnionCost -= KWStat::LnFactorial(nPartFrequency);
	}
	return dPartUnionCost;
}

double KWVarPartDataGridClusteringCosts::ComputeCellCost(const KWDGCell* cell) const
{
	double dCellCost;

	require(cell != NULL);

	dCellCost = -KWStat::LnFactorial(cell->GetCellFrequency());
	return dCellCost;
}

double KWVarPartDataGridClusteringCosts::ComputeValueCost(const KWDGValue* value) const
{
	double dValueCost;

	require(value != NULL);

	dValueCost = -KWStat::LnFactorial(value->GetValueFrequency());
	return dValueCost;
}

double KWVarPartDataGridClusteringCosts::ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
									 int nInformativeAttributeNumber) const
{
	double dDataGridConstructionCost;

	// Coût du choix entre modele nul et modele informatif
	dDataGridConstructionCost = log(2.0);

	// Cout de selection des variables (modele informatif)
	if (nInformativeAttributeNumber > 0)
	{
		dDataGridConstructionCost += KWStat::NaturalNumbersUniversalCodeLength(nInformativeAttributeNumber);
		dDataGridConstructionCost -= KWStat::LnFactorial(nInformativeAttributeNumber);
	}
	return dDataGridConstructionCost;
}

double KWVarPartDataGridClusteringCosts::ComputeAttributeConstructionCost(const KWDGAttribute* attribute,
									  int nPartitionSize) const
{
	if (nPartitionSize > 1)
		return attribute->GetCost();
	else
		return 0;
}

double KWVarPartDataGridClusteringCosts::ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
								  int nInformativeAttributeNumber) const
{
	double dDataGridCost;

	// Le cout de modele est egal au cout total moins ln N!
	dDataGridCost = ComputeDataGridCost(dataGrid, dLnGridSize, nInformativeAttributeNumber);
	dDataGridCost -= KWStat::LnFactorial(dataGrid->GetGridFrequency());
	assert(dDataGridCost >= 0);
	return dDataGridCost;
}

double KWVarPartDataGridClusteringCosts::ComputeAttributeModelCost(const KWDGAttribute* attribute,
								   int nPartitionSize) const
{
	double dAttributeModelCost;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	KWDGPart* innerAttributePart;

	dAttributeModelCost = ComputeAttributeCost(attribute, nPartitionSize);

	// Cas d'une grille et d'un attribut de type VarPart
	if (attribute->GetDataGrid()->GetVarPartDataGrid() and attribute->GetAttributeType() == KWType::VarPart)
	{
		// Prise en compte du cout des attributs internes
		for (nInnerAttribute = 0; nInnerAttribute < attribute->GetInnerAttributesNumber(); nInnerAttribute++)
		{
			innerAttribute = attribute->GetDataGrid()->GetInnerAttributes()->LookupInnerAttribute(
			    attribute->GetInnerAttributeNameAt(nInnerAttribute));
			dAttributeModelCost +=
			    ComputeInnerAttributeCost(innerAttribute, innerAttribute->GetPartNumber());

			// Prise en compte du cout des parties internes
			innerAttributePart = innerAttribute->GetHeadPart();
			while (innerAttributePart != NULL)
			{
				dAttributeModelCost += ComputeInnerAttributePartCost(innerAttributePart);
				innerAttribute->GetNextPart(innerAttributePart);
			}
		}
	}

	return dAttributeModelCost;
}

double KWVarPartDataGridClusteringCosts::ComputePartModelCost(const KWDGPart* part) const
{
	double dPartCost;
	int nValueNumber;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));

	// Cout de distribution des valeurs dans le cas categoriel
	dPartCost = 0;
	if (part->GetAttribute()->GetAttributeType() == KWType::Symbol)
	{
		if (part->GetPartFrequency() > 0)
		{
			nValueNumber = part->GetValueSet()->GetTrueValueNumber();

			// Distribution des valeurs dans la partie
			dPartCost += KWStat::LnFactorial(part->GetPartFrequency() + nValueNumber - 1);
			dPartCost -= KWStat::LnFactorial(nValueNumber - 1);
			dPartCost -= KWStat::LnFactorial(part->GetPartFrequency());
		}
	}
	return dPartCost;
}

double KWVarPartDataGridClusteringCosts::ComputeCellModelCost(const KWDGCell* cell) const
{
	return 0;
}

double KWVarPartDataGridClusteringCosts::ComputeValueModelCost(const KWDGValue* value) const
{
	return 0;
}

const ALString KWVarPartDataGridClusteringCosts::GetClassLabel() const
{
	return "VarPart data grid clustering costs";
}
// CH IV End

////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridRegressionCosts

KWDataGridRegressionCosts::KWDataGridRegressionCosts() {}

KWDataGridRegressionCosts::~KWDataGridRegressionCosts() {}

KWDataGridCosts* KWDataGridRegressionCosts::Clone() const
{
	return new KWDataGridRegressionCosts;
}

double KWDataGridRegressionCosts::ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
						      int nInformativeAttributeNumber) const
{
	double dDataGridCost;
	int nGranularity;
	int nGranularityMax;
	require(dataGrid != NULL);
	require(dLnGridSize >= 0);
	require(nInformativeAttributeNumber >= 0);

	// Initialisation de la granularite courante et maximale
	nGranularity = dataGrid->GetGranularity();
	if (nGranularity > 0)
		nGranularityMax = (int)ceil(log(dataGrid->GetGridFrequency() * 1.0) / log(2.0));
	else
		nGranularityMax = 0;

	assert(nGranularity > 0 or nInformativeAttributeNumber == 0);

	// Cout du choix entre modele nul et modele informatif
	dDataGridCost = log(2.0);

	if (nInformativeAttributeNumber > 0)
	{
		dDataGridCost += GetModelFamilySelectionCost();

		// Cout de la granularite
		dDataGridCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);
	}

	// Implemente uniquement en univarie
	assert(dataGrid->GetAttributeNumber() <= 2);
	assert(dataGrid->GetAttributeNumber() <= 1 or dataGrid->GetTargetAttribute() != NULL);

	return dDataGridCost;
}

double KWDataGridRegressionCosts::ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	double dAttributeCost;
	// int nValueNumber;
	int nPartileNumber;
	int nGarbageModalityNumber;

	require(attribute != NULL);
	require(attribute->GetGranularizedValueNumber() > 0);
	require(nPartitionSize >= 1);
	require(nPartitionSize <= attribute->GetGranularizedValueNumber());
	require(KWType::IsSimple(attribute->GetAttributeType()));

	nPartileNumber = attribute->GetGranularizedValueNumber();

	require(nPartileNumber > 0);
	require(nPartitionSize >= 1);
	require(nPartitionSize <= nPartileNumber);
	// Une partition avec poubelle contient au moins 2 parties informatives + 1 groupe poubelle
	require(attribute->GetGarbageModalityNumber() == 0 or nPartitionSize >= 3);

	// Cout uniquement si attribut selectionne (partition univariee non nulle)
	if (nPartitionSize == 1)
		dAttributeCost = 0;
	// Sinon
	else
	{
		assert(nPartileNumber > 1);

		// Initialisation au cout de selection/construction
		// A zero pour l'attribut cible
		dAttributeCost = 0;
		if (not attribute->GetAttributeTargetFunction())
			dAttributeCost += attribute->GetCost();

		// Cout de structure si attribut continu
		if (attribute->GetAttributeType() == KWType::Continuous)
		{
			// Cout de codage du nombre d'intervalles
			dAttributeCost +=
			    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1, nPartileNumber - 1);

			// Complement de cout pour le choix des bornes si l'attribut est source
			if (not attribute->GetAttributeTargetFunction())
			{
				// Nouveau codage avec description du choix des coupures selon une multinomiale
				dAttributeCost += (nPartitionSize - 1) * log((nPartileNumber - 1) * 1.0);
				dAttributeCost -= KWStat::LnFactorial(nPartitionSize - 1);
			}
		}
		// Cout de structure si attribut symbolique
		else
		{
			// On s'assure que la fonction de l'attribut n'est pas cible sinon il
			// s'agirait de classification et non de regression
			assert(not attribute->GetAttributeTargetFunction());

			// Taille de la poubelle
			nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

			// Cout de codage du choix d'une poubelle ou pas
			if (nPartileNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
				dAttributeCost += log(2.0);

			// Cout de codage du choix des modalites informatives (hors poubelle)
			if (nGarbageModalityNumber > 0)
			{
				// Cout de codage de la taille de la non-poubelle
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartileNumber - nGarbageModalityNumber - 1, nPartileNumber - 2);

				// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage par
				// variable
				dAttributeCost +=
				    (nPartileNumber - nGarbageModalityNumber) * log(nPartileNumber * 1.0) -
				    KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

				// Cout de codage du nombre de parties informatives (nPartitionSize-1)
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartitionSize - 2, nPartileNumber - nGarbageModalityNumber - 1);

				// Cout de codage du choix des parties informatives (nPartitionSize - 1)
				dAttributeCost +=
				    KWStat::LnBell(nPartileNumber - nGarbageModalityNumber, nPartitionSize - 1);
			}
			else
			{
				// Cout de codage du nombre de parties
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1,
												   nPartileNumber - 1);

				// Cout de codage du choix des parties
				dAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
			}
		}
	}
	return dAttributeCost;
}

double KWDataGridRegressionCosts::ComputePartCost(const KWDGPart* part) const
{
	double dPartCost;
	int nCurrentTargetPartitionSize;
	KWDGAttribute* targetAttribute;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));

	// Dans le cas d'un attribut cible il y a juste un terme qui depend de l'effectif
	// de la partie
	if (part->GetAttribute()->GetAttributeTargetFunction())
		dPartCost = KWStat::LnFactorial(part->GetPartFrequency());
	// Dans le cas d'un attribut source, on doit ajouter un terme qui depend
	// du nombre courant de parties cible
	else
	{
		// Acces a la taille de la partition cible (l'attribut cible peut etre absent s'il est non informatif)
		nCurrentTargetPartitionSize = 1;
		targetAttribute = part->GetAttribute()->GetDataGrid()->GetTargetAttribute();
		if (targetAttribute != NULL)
			nCurrentTargetPartitionSize = targetAttribute->GetPartNumber();

		// Calcul du cout
		dPartCost = KWStat::LnFactorial(part->GetPartFrequency() + nCurrentTargetPartitionSize - 1);
		dPartCost -= KWStat::LnFactorial(nCurrentTargetPartitionSize - 1);
	}

	return dPartCost;
}

double KWDataGridRegressionCosts::ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const
{
	int nPartFrequency;
	double dPartUnionCost;
	int nCurrentTargetPartitionSize;
	KWDGAttribute* targetAttribute;

	require(part1 != NULL);
	require(part2 != NULL);
	require(part1->GetAttribute() == part2->GetAttribute());
	require(KWType::IsSimple(part1->GetAttribute()->GetAttributeType()));

	// Effectif de l'union des parties
	nPartFrequency = part1->GetPartFrequency() + part2->GetPartFrequency();

	// Dans le cas d'un attribut cible il y a juste un terme qui depend de l'effectif
	// de la partie
	if (part1->GetAttribute()->GetAttributeTargetFunction())
		dPartUnionCost = KWStat::LnFactorial(nPartFrequency);
	// Dans le cas d'un attribut source, on doit ajouter un terme qui depend
	// du nombre courant de parties cible
	else
	{
		// Acces a la taille de la partition cible (l'attribut cible peut etre absent s'il est non informatif)
		nCurrentTargetPartitionSize = 1;
		targetAttribute = part1->GetAttribute()->GetDataGrid()->GetTargetAttribute();
		if (targetAttribute != NULL)
			nCurrentTargetPartitionSize = targetAttribute->GetPartNumber();

		// Calcul du cout
		dPartUnionCost = KWStat::LnFactorial(nPartFrequency + nCurrentTargetPartitionSize - 1);
		dPartUnionCost -= KWStat::LnFactorial(nCurrentTargetPartitionSize - 1);
	}
	return dPartUnionCost;
}

double KWDataGridRegressionCosts::ComputePartTargetDeltaCost(const KWDGPart* part) const
{
	double dPartTargetDeltaCost;
	int nCurrentTargetPartitionSize;
	KWDGAttribute* targetAttribute;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));
	require(part->GetAttribute()->GetDataGrid()->GetTargetAttribute() != NULL);

	// Dans le cas d'un attribut source, on doit tenir compte du nouveau nombre de paties cibles
	dPartTargetDeltaCost = 0;
	if (not part->GetAttribute()->GetAttributeTargetFunction())
	{
		// Acces a la taille de la partition cible (l'attribut cible peut etre absent s'il est non informatif)
		nCurrentTargetPartitionSize = 1;
		targetAttribute = part->GetAttribute()->GetDataGrid()->GetTargetAttribute();
		if (targetAttribute != NULL)
			nCurrentTargetPartitionSize = targetAttribute->GetPartNumber();

		// Calcul du cout
		if (nCurrentTargetPartitionSize >= 2)
		{
			dPartTargetDeltaCost = log(nCurrentTargetPartitionSize - 1.0) -
					       log(part->GetPartFrequency() + nCurrentTargetPartitionSize - 1.0);
		}
	}

	return dPartTargetDeltaCost;
}

double KWDataGridRegressionCosts::ComputeCellCost(const KWDGCell* cell) const
{
	double dCellCost;

	require(cell != NULL);

	dCellCost = -KWStat::LnFactorial(cell->GetCellFrequency());
	return dCellCost;
}
// CH IV Begin
double KWDataGridRegressionCosts::ComputeInnerAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	assert(false);
	return 0;
}

double KWDataGridRegressionCosts::ComputeInnerAttributePartCost(const KWDGPart* part) const
{
	assert(false);
	return 0;
}
// CH IV End
double KWDataGridRegressionCosts::ComputeDataGridConstructionCost(const KWDataGrid* dataGrid, double dLnGridSize,
								  int nInformativeAttributeNumber) const
{
	double dGranularityCost;
	int nGranularity;
	int nGranularityMax;

	// Prise en compte du cout de granularite en cas de variable informatives
	dGranularityCost = 0;
	if (nInformativeAttributeNumber > 0)
	{
		// calcul du cout a partir de la granularite courante et maximale
		nGranularity = dataGrid->GetGranularity();
		if (nGranularity > 0)
			nGranularityMax = (int)ceil(log(dataGrid->GetGridFrequency() * 1.0) / log(2.0));
		else
			nGranularityMax = 0;
		dGranularityCost = KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);
	}

	// Le cout de construction est egal au cout total moins le cout du choix de la granularite
	return ComputeDataGridCost(dataGrid, dLnGridSize, nInformativeAttributeNumber) - dGranularityCost;
}

double KWDataGridRegressionCosts::ComputeAttributeConstructionCost(const KWDGAttribute* attribute,
								   int nPartitionSize) const
{
	if (nPartitionSize > 1 and not attribute->GetAttributeTargetFunction())
		return attribute->GetCost();
	else
		return 0;
}

double KWDataGridRegressionCosts::ComputeDataGridModelCost(const KWDataGrid* dataGrid, double dLnGridSize,
							   int nInformativeAttributeNumber) const
{
	// Le cout de modele est egal au cout total
	return ComputeDataGridCost(dataGrid, dLnGridSize, nInformativeAttributeNumber);
}

double KWDataGridRegressionCosts::ComputeAttributeModelCost(const KWDGAttribute* attribute, int nPartitionSize) const
{
	// Le cout de modele est egal au cout total
	return ComputeAttributeCost(attribute, nPartitionSize);
}

double KWDataGridRegressionCosts::ComputePartModelCost(const KWDGPart* part) const
{
	double dPartCost;
	int nCurrentTargetPartitionSize;
	KWDGAttribute* targetAttribute;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));

	// Dans le cas d'un attribut source, on a un terme qui depend
	// du nombre courant de parties cible
	dPartCost = 0;
	if (not part->GetAttribute()->GetAttributeTargetFunction())
	{
		// Acces a la taille de la partition cible (l'attribut cible peut etre absent s'il est non informatif)
		nCurrentTargetPartitionSize = 1;
		targetAttribute = part->GetAttribute()->GetDataGrid()->GetTargetAttribute();
		if (targetAttribute != NULL)
			nCurrentTargetPartitionSize = targetAttribute->GetPartNumber();

		// Calcul du cout
		dPartCost = KWStat::LnFactorial(part->GetPartFrequency() + nCurrentTargetPartitionSize - 1);
		dPartCost -= KWStat::LnFactorial(nCurrentTargetPartitionSize - 1);
		dPartCost -= KWStat::LnFactorial(part->GetPartFrequency());
	}
	return dPartCost;
}

double KWDataGridRegressionCosts::ComputeCellModelCost(const KWDGCell* cell) const
{
	return 0;
}

double KWDataGridRegressionCosts::ComputeValueModelCost(const KWDGValue* value) const
{
	return 0;
}

const ALString KWDataGridRegressionCosts::GetClassLabel() const
{
	return "Data grid regression costs";
}

////////////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridGeneralizedClassificationCosts

KWDataGridGeneralizedClassificationCosts::KWDataGridGeneralizedClassificationCosts() {}

KWDataGridGeneralizedClassificationCosts::~KWDataGridGeneralizedClassificationCosts() {}

KWDataGridCosts* KWDataGridGeneralizedClassificationCosts::Clone() const
{
	return new KWDataGridGeneralizedClassificationCosts;
}

double KWDataGridGeneralizedClassificationCosts::ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
								     int nInformativeAttributeNumber) const
{
	double dDataGridCost;
	int nGranularity;
	int nGranularityMax;

	require(dataGrid != NULL);
	require(dLnGridSize >= 0);
	require(nInformativeAttributeNumber >= 0);

	// Initialisation de la granularite courante et maximale
	nGranularity = dataGrid->GetGranularity();
	if (nGranularity > 0)
		nGranularityMax = (int)ceil(log(dataGrid->GetGridFrequency() * 1.0) / log(2.0));
	else
		nGranularityMax = 0;

	assert(nGranularity > 0 or nInformativeAttributeNumber == 0);

	// Cout du choix entre modele nul et modele informatif
	dDataGridCost = log(2.0);

	if (nInformativeAttributeNumber > 0)
	{
		dDataGridCost += GetModelFamilySelectionCost();

		// Cout de la granularite
		dDataGridCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);
	}

	// Implemente uniquement en univarie
	assert(dataGrid->GetAttributeNumber() <= 2);
	assert(dataGrid->GetAttributeNumber() <= 1 or dataGrid->GetTargetAttribute() != NULL);

	return dDataGridCost;
}

double KWDataGridGeneralizedClassificationCosts::ComputeAttributeCost(const KWDGAttribute* attribute,
								      int nPartitionSize) const
{
	double dAttributeCost;
	// int nValueNumber;
	int nPartileNumber;
	int nGarbageModalityNumber;

	require(attribute != NULL);
	require(KWType::IsSimple(attribute->GetAttributeType()));

	nPartileNumber = attribute->GetGranularizedValueNumber();

	require(nPartileNumber > 0);
	require(nPartitionSize >= 1);
	require(nPartitionSize <= nPartileNumber);
	// Une partition avec poubelle contient au moins 2 parties informatives + 1 groupe poubelle
	require(attribute->GetGarbageModalityNumber() == 0 or nPartitionSize >= 3);

	// Cout uniquement si attribut selectionne (partition univariee non nulle)
	if (nPartitionSize == 1)
		dAttributeCost = 0;
	// Sinon
	else
	{
		assert(nPartileNumber > 1);

		// Initialisation au cout de selection/construction
		// A zero pour l'attribut cible
		dAttributeCost = 0;
		if (not attribute->GetAttributeTargetFunction())
			dAttributeCost = attribute->GetCost();

		// Cout de structure si attribut continu
		if (attribute->GetAttributeType() == KWType::Continuous)
		{
			// Cout de codage du nombre d'intervalles
			dAttributeCost +=
			    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1, nPartileNumber - 1);

			// Cout de codage des intervalles
			// Nouveau codage avec description du choix des coupures selon une multinomiale
			dAttributeCost += (nPartitionSize - 1) * log((nPartileNumber - 1) * 1.0);
			dAttributeCost -= KWStat::LnFactorial(nPartitionSize - 1);
		}
		// Cout de structure si attribut symbolique
		else
		{
			// Taille de la poubelle
			nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

			// Cout de codage du choix d'une poubelle ou pas
			if (nPartileNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
				dAttributeCost += log(2.0);

			// Cout de codage du choix des modalites informatives (hors poubelle)
			if (nGarbageModalityNumber > 0)
			{
				// Cout de codage de la taille de la non-poubelle
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartileNumber - nGarbageModalityNumber - 1, nPartileNumber - 2);

				// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage par
				// variable
				dAttributeCost +=
				    (nPartileNumber - nGarbageModalityNumber) * log(nPartileNumber * 1.0) -
				    KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

				// Cout de codage du nombre de parties informatives (nPartitionSize-1)
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartitionSize - 2, nPartileNumber - nGarbageModalityNumber - 1);

				// Cout de codage du choix des parties informatives (nPartitionSize - 1)
				dAttributeCost +=
				    KWStat::LnBell(nPartileNumber - nGarbageModalityNumber, nPartitionSize - 1);
			}
			else
			{
				// Cout de codage du nombre de parties
				dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1,
												   nPartileNumber - 1);

				// Cout de codage du choix des parties
				dAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
			}
		}
	}
	return dAttributeCost;
}

double KWDataGridGeneralizedClassificationCosts::ComputePartCost(const KWDGPart* part) const
{
	double dPartCost;
	int nCurrentTargetPartitionSize;
	KWDGAttribute* targetAttribute;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));

	// Dans le cas d'un attribut cible il y a juste un terme qui depend de l'effectif
	// de la partie, et du nombre de valeurs de la partie dans le cas symbolique
	if (part->GetAttribute()->GetAttributeTargetFunction())
	{
		if (part->GetAttribute()->GetAttributeType() == KWType::Continuous)
			dPartCost = KWStat::LnFactorial(part->GetPartFrequency());
		else
		{
			assert(part->GetAttribute()->GetAttributeType() == KWType::Symbol);
			assert(part->GetPartFrequency() >= part->GetValueSet()->GetTrueValueNumber() - 1);
			dPartCost = 0;
			if (part->GetPartFrequency() > 0)
			{
				dPartCost = KWStat::LnFactorial(part->GetPartFrequency() +
								part->GetValueSet()->GetTrueValueNumber() - 1);
				dPartCost -= KWStat::LnFactorial(part->GetValueSet()->GetTrueValueNumber() - 1);
			}
		}
	}
	// Dans le cas d'un attribut source, on doit ajouter un terme qui depend
	// du nombre courant de parties cibles
	else
	{
		// Acces a la taille de la partition cible (l'attribut cible peut etre absent s'il est non informatif)
		nCurrentTargetPartitionSize = 1;
		targetAttribute = part->GetAttribute()->GetDataGrid()->GetTargetAttribute();
		if (targetAttribute != NULL)
			nCurrentTargetPartitionSize = targetAttribute->GetPartNumber();

		// Calcul du cout
		dPartCost = KWStat::LnFactorial(part->GetPartFrequency() + nCurrentTargetPartitionSize - 1);
		dPartCost -= KWStat::LnFactorial(nCurrentTargetPartitionSize - 1);
	}

	return dPartCost;
}

double KWDataGridGeneralizedClassificationCosts::ComputePartUnionCost(const KWDGPart* part1,
								      const KWDGPart* part2) const
{
	int nPartFrequency;
	int nPartValueNumber;
	double dPartUnionCost;
	int nCurrentTargetPartitionSize;
	KWDGAttribute* targetAttribute;

	require(part1 != NULL);
	require(part2 != NULL);
	require(part1->GetAttribute() == part2->GetAttribute());
	require(KWType::IsSimple(part1->GetAttribute()->GetAttributeType()));

	// Effectif de l'union des parties
	nPartFrequency = part1->GetPartFrequency() + part2->GetPartFrequency();

	// Dans le cas d'un attribut cible il y a juste un terme qui depend de l'effectif
	// de la partie, et du nombre de valeurs de la partie dans le cas symbolique
	if (part1->GetAttribute()->GetAttributeTargetFunction())
	{
		if (part1->GetAttribute()->GetAttributeType() == KWType::Continuous)
			dPartUnionCost = KWStat::LnFactorial(nPartFrequency);
		else
		{
			assert(part1->GetAttribute()->GetAttributeType() == KWType::Symbol);
			nPartValueNumber =
			    part1->GetValueSet()->GetTrueValueNumber() + part2->GetValueSet()->GetTrueValueNumber();
			assert(nPartFrequency >= nPartValueNumber - 1);
			dPartUnionCost = 0;
			if (nPartFrequency > 0)
			{
				dPartUnionCost = KWStat::LnFactorial(nPartFrequency + nPartValueNumber - 1);
				dPartUnionCost -= KWStat::LnFactorial(nPartValueNumber - 1);
			}
		}
	}
	// Dans le cas d'un attribut source, on doit ajouter un terme qui depend
	// du nombre courant de parties cible
	else
	{
		// Acces a la taille de la partition cible (l'attribut cible peut etre absent s'il est non informatif)
		nCurrentTargetPartitionSize = 1;
		targetAttribute = part1->GetAttribute()->GetDataGrid()->GetTargetAttribute();
		if (targetAttribute != NULL)
			nCurrentTargetPartitionSize = targetAttribute->GetPartNumber();

		// Calcul du cout
		dPartUnionCost = KWStat::LnFactorial(nPartFrequency + nCurrentTargetPartitionSize - 1);
		dPartUnionCost -= KWStat::LnFactorial(nCurrentTargetPartitionSize - 1);
	}
	return dPartUnionCost;
}

double KWDataGridGeneralizedClassificationCosts::ComputePartTargetDeltaCost(const KWDGPart* part) const
{
	double dPartTargetDeltaCost;
	int nCurrentTargetPartitionSize;
	KWDGAttribute* targetAttribute;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));
	require(part->GetAttribute()->GetDataGrid()->GetTargetAttribute() != NULL);

	// Dans le cas d'un attribut source, on doit tenir compte du nouveau nombre de paties cibles
	dPartTargetDeltaCost = 0;
	if (not part->GetAttribute()->GetAttributeTargetFunction())
	{
		// Acces a la taille de la partition cible (l'attribut cible peut etre absent s'il est non informatif)
		nCurrentTargetPartitionSize = 1;
		targetAttribute = part->GetAttribute()->GetDataGrid()->GetTargetAttribute();
		if (targetAttribute != NULL)
			nCurrentTargetPartitionSize = targetAttribute->GetPartNumber();

		// Calcul du cout
		if (nCurrentTargetPartitionSize >= 2)
		{
			dPartTargetDeltaCost = log(nCurrentTargetPartitionSize - 1.0) -
					       log(part->GetPartFrequency() + nCurrentTargetPartitionSize - 1.0);
		}
	}

	return dPartTargetDeltaCost;
}

double KWDataGridGeneralizedClassificationCosts::ComputeCellCost(const KWDGCell* cell) const
{
	double dCellCost;

	require(cell != NULL);

	dCellCost = -KWStat::LnFactorial(cell->GetCellFrequency());
	return dCellCost;
}
// CH IV Begin
double KWDataGridGeneralizedClassificationCosts::ComputeInnerAttributeCost(const KWDGAttribute* attribute,
									   int nPartitionSize) const
{
	assert(false);
	return 0;
}

double KWDataGridGeneralizedClassificationCosts::ComputeInnerAttributePartCost(const KWDGPart* part) const
{
	assert(false);
	return 0;
}
// CH IV End

double KWDataGridGeneralizedClassificationCosts::ComputeValueCost(const KWDGValue* value) const
{
	double dValueCost;

	require(value != NULL);

	dValueCost = -KWStat::LnFactorial(value->GetValueFrequency());
	return dValueCost;
}

double KWDataGridGeneralizedClassificationCosts::ComputeDataGridAllValuesCost(const KWDataGrid* dataGrid) const
{
	double dAllValuesCost;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGValue* value;

	require(dataGrid != NULL);

	// Parcours des attributs symboliques
	dAllValuesCost = 0;
	for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = dataGrid->GetAttributeAt(nAttribute);

		// Gestion uniquement dans le cas symbolique et cible
		if (attribute->GetAttributeType() == KWType::Symbol and attribute->GetAttributeTargetFunction())
		{
			// Parcours des parties
			part = attribute->GetHeadPart();
			while (part != NULL)
			{
				// Parcours des valeurs de la partie
				value = part->GetValueSet()->GetHeadValue();
				while (value != NULL)
				{
					// Prise en compte du cout de la valeur
					dAllValuesCost += ComputeValueCost(value);

					// Valeur suivante
					part->GetValueSet()->GetNextValue(value);
				}

				// Partie suivante
				attribute->GetNextPart(part);
			}
		}
	}
	return dAllValuesCost;
}

double KWDataGridGeneralizedClassificationCosts::ComputeDataGridConstructionCost(const KWDataGrid* dataGrid,
										 double dLnGridSize,
										 int nInformativeAttributeNumber) const
{
	double dGranularityCost;
	int nGranularity;
	int nGranularityMax;

	// Prise en compte du cout de granularite en cas de variable informatives
	dGranularityCost = 0;
	if (nInformativeAttributeNumber > 0)
	{
		// calcul du cout a partir de la granularite courante et maximale
		nGranularity = dataGrid->GetGranularity();
		if (nGranularity > 0)
			nGranularityMax = (int)ceil(log(dataGrid->GetGridFrequency() * 1.0) / log(2.0));
		else
			nGranularityMax = 0;
		dGranularityCost = KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);
	}

	// Le cout de construction est egal au cout total moins le cout du choix de la granularite
	return ComputeDataGridCost(dataGrid, dLnGridSize, nInformativeAttributeNumber) - dGranularityCost;
}

double KWDataGridGeneralizedClassificationCosts::ComputeAttributeConstructionCost(const KWDGAttribute* attribute,
										  int nPartitionSize) const
{
	if (nPartitionSize > 1 and not attribute->GetAttributeTargetFunction())
		return attribute->GetCost();
	else
		return 0;
}

double KWDataGridGeneralizedClassificationCosts::ComputeDataGridModelCost(const KWDataGrid* dataGrid,
									  double dLnGridSize,
									  int nInformativeAttributeNumber) const
{
	// Le cout de modele est egal au cout total
	return ComputeDataGridCost(dataGrid, dLnGridSize, nInformativeAttributeNumber);
}

double KWDataGridGeneralizedClassificationCosts::ComputeAttributeModelCost(const KWDGAttribute* attribute,
									   int nPartitionSize) const
{
	// Le cout de modele est egal au cout total
	return ComputeAttributeCost(attribute, nPartitionSize);
}

double KWDataGridGeneralizedClassificationCosts::ComputePartModelCost(const KWDGPart* part) const
{
	double dPartCost;
	int nCurrentTargetPartitionSize;
	KWDGAttribute* targetAttribute;

	require(part != NULL);
	require(KWType::IsSimple(part->GetAttribute()->GetAttributeType()));

	// Dans le cas d'un attribut cible il y a juste un terme qui depend de l'effectif
	// de la partie, et du nombre de valeurs de la partie dans le cas symbolique
	if (part->GetAttribute()->GetAttributeTargetFunction())
	{
		if (part->GetAttribute()->GetAttributeType() == KWType::Continuous)
			dPartCost = KWStat::LnFactorial(part->GetPartFrequency());
		else
		{
			assert(part->GetAttribute()->GetAttributeType() == KWType::Symbol);
			assert(part->GetPartFrequency() >= part->GetValueSet()->GetTrueValueNumber() - 1);
			dPartCost = 0;
			if (part->GetPartFrequency() > 0)
			{
				dPartCost = KWStat::LnFactorial(part->GetPartFrequency() +
								part->GetValueSet()->GetTrueValueNumber() - 1);
				dPartCost -= KWStat::LnFactorial(part->GetValueSet()->GetTrueValueNumber() - 1);
				dPartCost -= KWStat::LnFactorial(part->GetPartFrequency());
			}
		}
	}
	// Dans le cas d'un attribut source, on doit ajouter un terme qui depend
	// du nombre courant de parties cibles
	else
	{
		// Acces a la taille de la partition cible (l'attribut cible peut etre absent s'il est non informatif)
		nCurrentTargetPartitionSize = 1;
		targetAttribute = part->GetAttribute()->GetDataGrid()->GetTargetAttribute();
		if (targetAttribute != NULL)
			nCurrentTargetPartitionSize = targetAttribute->GetPartNumber();

		// Calcul du cout
		dPartCost = KWStat::LnFactorial(part->GetPartFrequency() + nCurrentTargetPartitionSize - 1);
		dPartCost -= KWStat::LnFactorial(nCurrentTargetPartitionSize - 1);
		dPartCost -= KWStat::LnFactorial(part->GetPartFrequency());
	}

	return dPartCost;
}

double KWDataGridGeneralizedClassificationCosts::ComputeCellModelCost(const KWDGCell* cell) const
{
	return 0;
}

double KWDataGridGeneralizedClassificationCosts::ComputeValueModelCost(const KWDGValue* value) const
{
	return 0;
}

const ALString KWDataGridGeneralizedClassificationCosts::GetClassLabel() const
{
	return "Data grid generalized classification costs";
}
