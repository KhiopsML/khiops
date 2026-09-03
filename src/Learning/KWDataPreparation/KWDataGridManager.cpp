// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridManager.h"

KWDataGridManager::KWDataGridManager() {}

KWDataGridManager::~KWDataGridManager() {}

void KWDataGridManager::CopyDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const
{
	KWDataGridManager dataGridManager;

	require(targetDataGrid != NULL);

	// Utilisation d'un manager de grille pour effectuer la copie
	targetDataGrid->DeleteAll();
	dataGridManager.ExportDataGrid(sourceDataGrid, targetDataGrid);
}

void KWDataGridManager::CopyDataGridWithInnerAttributesCloned(const KWDataGrid* sourceDataGrid,
							      KWDataGrid* targetDataGrid) const
{
	KWDataGridManager dataGridManager;

	require(targetDataGrid != NULL);

	// Utilisation d'un manager de grille pour effectuier la copie
	targetDataGrid->DeleteAll();
	dataGridManager.ExportDataGridWithInnerAttributesCloned(sourceDataGrid, targetDataGrid);
}

void KWDataGridManager::ExportDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const
{
	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Export des attributs
	ExportAttributes(sourceDataGrid, targetDataGrid);

	// Export des partie des attributs
	ExportParts(sourceDataGrid, targetDataGrid);

	// Export des cellules
	ExportCells(sourceDataGrid, targetDataGrid);
	ensure(CheckDataGrid(sourceDataGrid, targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportDataGridWithInnerAttributesCloned(const KWDataGrid* sourceDataGrid,
								KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Export des attributs
	ExportAttributes(sourceDataGrid, targetDataGrid);

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Pour un attribut simple, export des parties
		if (KWType::IsSimple(sourceAttribute->GetAttributeType()))
			InitialiseAttributeParts(sourceAttribute, targetAttribute);
		// Pour un attribut VarPart, export des parties d'un clone des attributs internes
		else
		{
			InitialiseVarPartAttributeClonedParts(sourceAttribute, targetAttribute);
			assert(targetAttribute->GetInnerAttributes() != sourceAttribute->GetInnerAttributes());
		}
	}

	// Export des cellules
	ExportCells(sourceDataGrid, targetDataGrid);
	ensure(CheckDataGrid(sourceDataGrid, targetDataGrid));
}

void KWDataGridManager::ExportDataGridWithSingletonVarParts(const KWDataGrid* sourceDataGrid,
							    const KWDataGrid* mandatoryDataGrid,
							    KWDataGrid* targetDataGrid) const
{
	KWDGAttribute* targetVarPartAttribute;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(sourceDataGrid->GetInformativeAttributeNumber() > 0);
	require(sourceDataGrid->IsVarPartDataGrid());
	require(mandatoryDataGrid != NULL);
	require(mandatoryDataGrid->IsVarPartDataGrid());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export des attributs
	ExportAttributes(sourceDataGrid, targetDataGrid);

	// Attention, on reutilise les attributs internes de la grille optimisee
	if (targetDataGrid->IsVarPartDataGrid())
	{
		// Partage des partitions des attributs internes de la grille optimisee
		targetVarPartAttribute = targetDataGrid->GetVarPartAttribute();
		targetVarPartAttribute->SetInnerAttributes(mandatoryDataGrid->GetInnerAttributes());
	}

	// Export des partie des attributs si aucune variable informative
	if (mandatoryDataGrid->GetInformativeAttributeNumber() == 0)
		ExportParts(sourceDataGrid, targetDataGrid);
	// Et dans le cas de variables informatives
	else
	{
		// Initialisation des parties des attributs
		for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
		{
			targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

			// Recherche de l'attribut source correspondant
			sourceAttribute = mandatoryDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
			check(sourceAttribute);

			// Cas d'un attribut Continuous ou Symbol
			if (KWType::IsSimple(sourceAttribute->GetAttributeType()))
				InitialiseAttributeParts(sourceAttribute, targetAttribute);
			// Sinon, cas d'un attribut VarPart
			// Creation des parties de parties de variable de l'attribut, avec un cluster par partie de variable
			else
				targetAttribute->CreateVarPartsSet();
		}
		assert(CheckParts(sourceDataGrid, targetDataGrid));
	}

	// Export des cellules
	ExportCells(sourceDataGrid, targetDataGrid);

	ensure(CheckDataGrid(sourceDataGrid, targetDataGrid));
	ensure(targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
	       mandatoryDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportNullDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGAttribute* targetInnerAttribute;
	int nInnerAttribute;
	KWDGInnerAttributes* nullInnerAttributes;
	KWDGPart* targetPart;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export des attributs
	ExportAttributes(sourceDataGrid, targetDataGrid);

	// Initialisation des attributs avec une seule partie
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut source et cible
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Creation d'une seule partie par attribut pour les attributs simples
		if (KWType::IsSimple(sourceAttribute->GetAttributeType()))
			InitialiseAttributeNullPart(sourceAttribute, targetAttribute);
		// Et pour l'attribut de type VarPart
		else
		{
			assert(sourceAttribute->GetAttributeType() == KWType::VarPart);

			// Creation d'attributs internes avec une seule partie par attribut
			nullInnerAttributes = CreateNullInnerAttributes(sourceAttribute->GetInnerAttributes());

			// Partage des partitions de la grille source
			targetAttribute->SetInnerAttributes(nullInnerAttributes);

			// Creation de l'ensemble des valeurs cible
			targetPart = targetAttribute->AddPart();

			// Parcours des attributs internes
			for (nInnerAttribute = 0; nInnerAttribute < targetAttribute->GetInnerAttributeNumber();
			     nInnerAttribute++)
			{
				// Extraction de l'attribut interne source
				targetInnerAttribute = targetAttribute->GetInnerAttributeAt(nInnerAttribute);

				// Ajout de la partie de l'attribut interne
				targetPart->GetVarPartSet()->AddVarPart(targetInnerAttribute->GetHeadPart());
			}
		}
	}

	// Export des cellules
	ExportCells(sourceDataGrid, targetDataGrid);

	ensure(targetDataGrid->Check());
	ensure(CheckDataGrid(sourceDataGrid, targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() !=
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportDataGridWithRandomizedInnerAttributes(const KWDataGrid* sourceDataGrid,
								    const KWDataGrid* mandatoryDataGrid,
								    int nTargetTokenNumber, KWDataGrid* targetDataGrid)
{
	const boolean bTrace = false;
	int nSourceTokenNumber;
	int nCurrentTokenNumber;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* mandatoryAttribute;
	KWDGAttribute* targetAttribute;
	KWDGInnerAttributes* surtokenizedInnerAttributes;

	require(Check());
	require(sourceDataGrid != NULL);
	require(sourceDataGrid->IsVarPartDataGrid());
	require(mandatoryDataGrid != NULL);
	require(mandatoryDataGrid->IsVarPartDataGrid());
	require(sourceDataGrid->GetInnerAttributes()->ContainsSubVarParts(mandatoryDataGrid->GetInnerAttributes()));
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(nTargetTokenNumber <=
		sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts());
	require(nTargetTokenNumber >=
		mandatoryDataGrid->GetVarPartAttribute()->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts());

	// Trace initiale
	if (bTrace)
	{
		cout << "ExportDataGridWithRandomizedInnerAttributes\t" << nTargetTokenNumber << "\n";
		cout << "\tsourceDataGrid\t" << sourceDataGrid->GetObjectLabel() << endl;
		cout << "\tmandatoryDataGrid\t" << mandatoryDataGrid->GetObjectLabel() << endl;
	}

	// Nombre de tokens de la grille obligatoire
	nCurrentTokenNumber = mandatoryDataGrid->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts();

	// Nombre de tokens des innerAttributes sources
	nSourceTokenNumber = sourceDataGrid->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts();
	assert(nSourceTokenNumber >= nCurrentTokenNumber);

	// Cas ou le nombre de tokens objectif est inferieur au nombre de tokens obligatoire : recopie de la grille obligatoire a l'identique
	if (nTargetTokenNumber <= nCurrentTokenNumber)
		CopyDataGrid(mandatoryDataGrid, targetDataGrid);
	// Sinon
	else
	{
		// Export des attributs (avec innerAtributes non surtokenises a ce stade)
		ExportAttributes(mandatoryDataGrid, targetDataGrid);

		// Initialisation de la granularite
		targetDataGrid->SetGranularity(mandatoryDataGrid->GetGranularity());

		// Initialisation des parties des attributs
		for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
		{
			targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

			// Recherche de l'attribut source et obligatoire correspondant
			sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
			mandatoryAttribute = mandatoryDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
			assert(CheckAttributesConsistency(sourceAttribute, targetAttribute));
			assert(CheckAttributesConsistency(mandatoryAttribute, targetAttribute));

			// Pour un attribut simple, export des parties
			if (KWType::IsSimple(sourceAttribute->GetAttributeType()))
				InitialiseAttributeParts(mandatoryAttribute, targetAttribute);
			// Pour l'attribut VarPart, export des parties apres surtokenisation des attributs internes
			else
			{
				// Creation d'un nouveau KWDGInnerAttributes surtokenise
				surtokenizedInnerAttributes = CreateRandomInnerAttributes(
				    sourceDataGrid->GetInnerAttributes(), mandatoryDataGrid->GetInnerAttributes(),
				    nTargetTokenNumber);

				// Creation de l'attribut VarPart associe a ces innerAttributes selon la meme partition que l'attribut en entree
				InitialiseVarPartAttributeWithNewSurtokenisedInnerAttributes(
				    mandatoryDataGrid->GetVarPartAttribute(), surtokenizedInnerAttributes,
				    targetAttribute);
			}
		}
		ExportCells(sourceDataGrid, targetDataGrid);
	}

	// Trace finale
	if (bTrace)
	{
		cout << "\ttargetDataGrid\t" << targetDataGrid->GetObjectLabel() << endl;
	}
	ensure(targetDataGrid->IsVarPartDataGrid());
	ensure(targetDataGrid->GetAttributeAt(0)->GetPartNumber() ==
	       mandatoryDataGrid->GetAttributeAt(0)->GetPartNumber());
	ensure(targetDataGrid->GetAttributeAt(1)->GetPartNumber() ==
	       mandatoryDataGrid->GetAttributeAt(1)->GetPartNumber());
}

void KWDataGridManager::ExportDataGridWithMergedInnerAttributes(const KWDataGrid* sourceDataGrid,
								const KWDGInnerAttributes* mandatoryInnerAttributes,
								KWDataGrid* targetDataGrid)
{
	int nAttribute;
	KWDGAttribute* targetAttribute;
	KWDGAttribute* sourceAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(sourceDataGrid->IsVarPartDataGrid());
	require(sourceDataGrid->GetInnerAttributes()->ContainsSubVarParts(mandatoryInnerAttributes));

	// Export des attributs (avec innerAtributes non surtokenises a ce stade)
	targetDataGrid->DeleteAll();
	ExportAttributes(sourceDataGrid, targetDataGrid);

	// Initialisation de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Pour un attribut simple, export des parties
		if (KWType::IsSimple(sourceAttribute->GetAttributeType()))
			InitialiseAttributeParts(sourceAttribute, targetAttribute);
		// Pour l'attribut VarPart, export des parties apres surtokenisation des attributs internes
		else
		{
			// Creation de l'attribut VarPart associe a ces innerAttributes selon la meme partition que l'attribut en entree
			InitialiseVarPartAttributeWithMergedInnerAttributes(sourceDataGrid->GetVarPartAttribute(),
									    mandatoryInnerAttributes, targetAttribute);
		}
	}
	ExportCells(sourceDataGrid, targetDataGrid);
}

void KWDataGridManager::ExportDataGridWithPartitionnedInnerAttributes(
    const KWDataGrid* sourceDataGrid, const ObjectDictionary* odInnerAttributePartitions, KWDataGrid* targetDataGrid)
{
	int nAttribute;
	KWDGAttribute* targetAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGInnerAttributes* partitionnedInnerAttributes;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(sourceDataGrid->IsVarPartDataGrid());
	require(odInnerAttributePartitions != NULL);
	require(odInnerAttributePartitions->GetCount() > 0);
	require(odInnerAttributePartitions->GetCount() <=
		sourceDataGrid->GetInnerAttributes()->GetInnerAttributeNumber());

	// Export des attributs (avec innerAtributes non surtokenises a ce stade)
	targetDataGrid->DeleteAll();
	ExportAttributes(sourceDataGrid, targetDataGrid);

	// Initialisation de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Pour un attribut simple, export des parties
		if (KWType::IsSimple(sourceAttribute->GetAttributeType()))
			InitialiseAttributeParts(sourceAttribute, targetAttribute);
		// Pour l'attribut VarPart, export des parties apres surtokenisation des attributs internes
		else
		{
			// Creation d'une version partitionnee des attributs internes
			partitionnedInnerAttributes = CreatePartitionnedInnerAttributes(
			    sourceDataGrid->GetInnerAttributes(), odInnerAttributePartitions);

			// Creation de l'attribut VarPart associe a ces innerAttributes selon la meme partition que l'attribut en entree
			InitialiseVarPartAttributeWithMergedInnerAttributes(
			    sourceDataGrid->GetVarPartAttribute(), partitionnedInnerAttributes, targetAttribute);
		}
	}
	ExportCells(sourceDataGrid, targetDataGrid);
}

void KWDataGridManager::InitializeQuantileBuilders(const KWDataGrid* sourceDataGrid,
						   ObjectDictionary* odQuantilesBuilders,
						   IntVector* ivMaxPartNumbers) const
{
	KWDGAttribute* attribute;
	int nAttribute;
	KWQuantileBuilder* quantileBuilder;
	KWQuantileIntervalBuilder* quantileIntervalBuilder;
	KWQuantileGroupBuilder* quantileGroupBuilder;
	int nMaxPartNumber;

	require(Check());
	require(sourceDataGrid->AreAttributePartsSorted());
	require(odQuantilesBuilders != NULL);
	require(ivMaxPartNumbers != NULL);
	require(odQuantilesBuilders->GetCount() == 0);
	require(ivMaxPartNumbers->GetSize() == 0);

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = sourceDataGrid->GetAttributeAt(nAttribute);

		// Creation et rangement d'un quantile builder dans un dictionnaire
		if (attribute->GetAttributeType() == KWType::Continuous)
		{
			quantileIntervalBuilder = new KWQuantileIntervalBuilder;
			nMaxPartNumber = InitializeQuantileIntervalBuilder(attribute, quantileIntervalBuilder);
			quantileBuilder = quantileIntervalBuilder;
		}
		else
		{
			quantileGroupBuilder = new KWQuantileGroupBuilder;
			nMaxPartNumber = InitializeQuantileGroupBuilder(attribute, quantileGroupBuilder);
			quantileBuilder = quantileGroupBuilder;
		}
		odQuantilesBuilders->SetAt(attribute->GetAttributeName(), quantileBuilder);

		// Memorisation du nombre maximal de parties
		ivMaxPartNumbers->Add(nMaxPartNumber);
	}
	assert(odQuantilesBuilders->GetCount() == sourceDataGrid->GetAttributeNumber());
	assert(ivMaxPartNumbers->GetSize() == sourceDataGrid->GetAttributeNumber());
}

double KWDataGridManager::ExportDataGridWithVarPartMergeOptimization(const KWDataGrid* sourceDataGrid,
								     const KWDataGridCosts* dataGridCosts,
								     KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGPart* part;
	KWDGPart* garbagePart;
	int nGarbageModalityNumber;
	double dFusionDeltaCost;
	double dNewAttributeCost;
	double dNewAttributeAttributeCostWithGarbage;

	require(Check());
	require(sourceDataGrid->GetInformativeAttributeNumber() > 0);
	require(sourceDataGrid->IsVarPartDataGrid());
	require(sourceDataGrid->GetVarPartAttribute()->GetPartNumber() > 1);
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export des attributs
	ExportAttributes(sourceDataGrid, targetDataGrid);

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Pour un attribut simple, export des partie
		if (KWType::IsSimple(sourceAttribute->GetAttributeType()))
			InitialiseAttributeParts(sourceAttribute, targetAttribute);
		// Pour un attribut VarPart, export des parties d'un clone des attributs internes
		else
		{
			InitialiseVarPartAttributeClonedParts(sourceAttribute, targetAttribute);
			assert(targetAttribute->GetInnerAttributes() != sourceAttribute->GetInnerAttributes());
		}
	}
	// Fusion des parties des attributs
	dFusionDeltaCost = MergePartsForVarPartAttributes(sourceDataGrid, targetDataGrid);

	// Tri des parties attributs internes pour un attribut de grille de type VarPart,
	// celles-ci ayant potentiellement ete modifiees
	targetDataGrid->GetVarPartAttribute()->GetInnerAttributes()->SortInnerAttributeParts();

	// Export des cellules
	ExportCells(sourceDataGrid, targetDataGrid);

	// Mise a jour de cout de fusion pour les attributs internes
	if (targetDataGrid->IsVarPartDataGrid())
	{
		// Recherche des attributs cible et source de type VarPart
		targetAttribute = targetDataGrid->GetVarPartAttribute();
		sourceAttribute = sourceDataGrid->GetVarPartAttribute();

		// Cas d'une partition sans groupe poubelle
		targetAttribute->SetGarbagePart(NULL);
		dNewAttributeCost =
		    dataGridCosts->ComputeAttributeCost(targetAttribute, targetAttribute->GetPartNumber());

		// Cas ou un groupe poubelle est envisageable (au moins 3 clusters)
		if (targetAttribute->GetPartNumber() >= 3)
		{
			garbagePart = NULL;
			nGarbageModalityNumber = 0;
			part = targetAttribute->GetHeadPart();
			while (part != NULL)
			{
				if (part->GetVarPartSet()->GetValueNumber() > nGarbageModalityNumber)
				{
					nGarbageModalityNumber = part->GetVarPartSet()->GetValueNumber();
					garbagePart = part;
				}
				targetAttribute->GetNextPart(part);
			}
			// Cout de l'attribut avec groupe poubelle
			targetAttribute->SetGarbagePart(garbagePart);
			dNewAttributeAttributeCostWithGarbage =
			    dataGridCosts->ComputeAttributeCost(targetAttribute, targetAttribute->GetPartNumber());

			// Mise a jour du cout le plus econome
			if (dNewAttributeAttributeCostWithGarbage < dNewAttributeCost)
				dNewAttributeCost = dNewAttributeAttributeCostWithGarbage;
			else
				targetAttribute->SetGarbagePart(NULL);
		}

		// Mise a jour du cout de l'attribut VarPart
		dFusionDeltaCost -=
		    dataGridCosts->ComputeAttributeCost(sourceAttribute, sourceAttribute->GetPartNumber());
		dFusionDeltaCost += dNewAttributeCost;
	}

	ensure(targetDataGrid->Check());
	ensure(CheckDataGrid(sourceDataGrid, targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() !=
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
	return dFusionDeltaCost;
}

void KWDataGridManager::UpdateVarPartDataGridFromVarPartGroups(const KWDataGrid* sourceDataGrid,
							       const IntVector* ivTargetGroupIndexes,
							       int nTargetGroupNumber, KWDataGrid* targetDataGrid) const
{
	boolean bDisplayResults = false;
	KWDGAttribute* initialAttribute;
	KWDGAttribute* targetAttribute;
	KWDGPart* initialPart;
	KWDGPart* targetPart;
	ObjectArray oaTargetParts;
	int nInitial;
	int nTarget;

	require(Check());
	require(sourceDataGrid->IsVarPartDataGrid());
	require(sourceDataGrid->GetVarPartAttribute()->GetPartNumber() == ivTargetGroupIndexes->GetSize());
	require(targetDataGrid->IsVarPartDataGrid());

	// Acces aux attributs des grilles initiale et optimise pour l'attribut de post-optimisation
	initialAttribute = sourceDataGrid->GetVarPartAttribute();
	targetAttribute = targetDataGrid->GetVarPartAttribute();

	// On vide la grille optimisee de ses cellules, en preservant ses attributs et leur partition
	targetDataGrid->DeleteAllCells();

	// On reinitialise a vide les partie pour l'attribut a post-optimiser
	targetAttribute->DeleteAllParts();

	// Reinitialisation a vide du groupe poubelle
	targetAttribute->SetGarbagePart(NULL);

	// Creation des parties de l'attribut groupe et memorisation dans un tableau
	oaTargetParts.SetSize(nTargetGroupNumber);
	for (nTarget = 0; nTarget < nTargetGroupNumber; nTarget++)
	{
		// Creation d'une nouvelle partie optimisee
		targetPart = targetAttribute->AddPart();
		oaTargetParts.SetAt(nTarget, targetPart);
	}

	// Parcours des parties initiales pour determiner les definitions des groupes
	initialPart = initialAttribute->GetHeadPart();
	nInitial = 0;
	while (initialPart != NULL)
	{
		// Recherche de l'index du groupe correspondant
		nTarget = ivTargetGroupIndexes->GetAt(nInitial);
		assert(0 <= nTarget and nTarget < nTargetGroupNumber);

		// Recherche de la partie optimisee a mettre a jour
		targetPart = cast(KWDGPart*, oaTargetParts.GetAt(nTarget));
		assert(targetPart->GetPartType() == KWType::VarPart);

		// Mise a jour de la definition du group
		targetPart->GetVarPartSet()->UpgradeFrom(initialPart->GetVarPartSet());

		// Mise a jour du groupe poubelle comme le groupe contenant le plus de parties de variables
		if (GetVarPartAttributeGarbage() and
		    targetPart->GetVarPartSet()->GetValueNumber() > targetAttribute->GetGarbageModalityNumber())
			targetAttribute->SetGarbagePart(targetPart);

		// Partie initiale suivante
		initialAttribute->GetNextPart(initialPart);
		nInitial++;
	}

	// Nettoyage eventuel des parties vides
	for (nTarget = 0; nTarget < oaTargetParts.GetSize(); nTarget++)
	{
		// Recherche de la partie optimisee a mettre a jour
		targetPart = cast(KWDGPart*, oaTargetParts.GetAt(nTarget));
		assert(targetPart->GetPartType() == KWType::VarPart);

		// Destruction si elle est vide
		if (targetPart->GetVarPartSet()->GetValueNumber() == 0)
		{
			targetAttribute->DeletePart(targetPart);
			nTargetGroupNumber--;
		}
	}
	assert(targetAttribute->GetPartNumber() == nTargetGroupNumber);

	// Export des cellules pour la grille initiale univariee
	ExportCells(sourceDataGrid, targetDataGrid);

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Preparation d'une grille pour l'optimisation univariee\t"
		     << sourceDataGrid->GetVarPartAttribute()->GetAttributeName() << endl;
		cout << "Grille initiale\n" << *sourceDataGrid << endl;
		cout << "Grille optimisee\n" << *targetDataGrid << endl;
	}

	// Verification de la grille preparee
	ensure(targetDataGrid->Check());
	ensure(targetAttribute->GetPartNumber() == nTargetGroupNumber);
	ensure(sourceDataGrid->GetGridFrequency() == targetDataGrid->GetGridFrequency());
	ensure(sourceDataGrid->GetCellNumber() >= targetDataGrid->GetCellNumber());
	ensure(targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
	       sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportGranularizedDataGrid(const KWDataGrid* sourceDataGrid,
						   const ObjectDictionary* odQuantilesBuilders, int nGranularity,
						   KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWQuantileBuilder* quantileBuilder;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(nGranularity >= 0);
	require(odQuantilesBuilders->GetCount() == sourceDataGrid->GetAttributeNumber());

	// Export des attributs
	ExportAttributes(sourceDataGrid, targetDataGrid);

	// Initialisation des parties granularisees des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Initialisation des parties granularisees
		quantileBuilder =
		    cast(KWQuantileBuilder*, odQuantilesBuilders->Lookup(targetAttribute->GetAttributeName()));
		InitialiseAttributeGranularizedParts(sourceAttribute, quantileBuilder, nGranularity, targetAttribute);
	}

	// Export des cellules
	ExportCells(sourceDataGrid, targetDataGrid);

	// On verifie l'integrite de la grille en sortie avant de modifier sa granularite
	ensure(CheckDataGrid(sourceDataGrid, targetDataGrid));

	// Memorisation de la granularite
	targetDataGrid->SetGranularity(nGranularity);
	ensure(targetDataGrid->Check());
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportFrequencyTableFromOneAttribute(const KWDataGrid* sourceDataGrid,
							     const ALString& sAttributeName,
							     KWFrequencyTable* kwFrequencyTable) const
{
	boolean bDisplayResults = false;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	KWDataGrid oneAttributeDataGrid;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGPart* dgPart;
	KWDGCell* dgCell;
	ObjectArray oaParts;
	IntVector* ivFrequency;
	int nPartIndex;
	int nTargetIndex;
	int nTargetValueNumber;
	int nSourceValueNumber;

	require(kwFrequencyTable != NULL);
	require(kwFrequencyTable->GetFrequencyVectorCreator() ==
		cast(KWDenseFrequencyVector*, kwFrequencyTable->GetFrequencyVectorCreator()));
	require(kwFrequencyTable->GetFrequencyVectorNumber() == 0);
	require(sAttributeName != "");

	// Initialisation de la grille reduite a l'attribut
	InitialiseDataGrid(sourceDataGrid, 1, &oneAttributeDataGrid);

	// Recherche de l'attribut source et cible
	sourceAttribute = sourceDataGrid->SearchAttribute(sAttributeName);
	targetAttribute = oneAttributeDataGrid.GetAttributeAt(0);

	// Transfert du parametrage de l'attribut
	InitialiseAttribute(sourceAttribute, targetAttribute);

	// Export des parties de cette grille
	ExportParts(sourceDataGrid, &oneAttributeDataGrid);

	// Export des cellules de cette grille
	ExportCells(sourceDataGrid, &oneAttributeDataGrid);

	// Export des parties de l'attribut
	oneAttributeDataGrid.GetAttributeAt(0)->ExportParts(&oaParts);

	// Initialisation du nombre de parties sources de la table de contingence
	nSourceValueNumber = oaParts.GetSize();
	nTargetValueNumber = 0;

	// Parametrage de la table d'effectif
	kwFrequencyTable->SetFrequencyVectorNumber(nSourceValueNumber);
	kwFrequencyTable->SetInitialValueNumber(oneAttributeDataGrid.GetAttributeAt(0)->GetInitialValueNumber());
	kwFrequencyTable->SetGranularizedValueNumber(
	    oneAttributeDataGrid.GetAttributeAt(0)->GetGranularizedValueNumber());
	kwFrequencyTable->SetGranularity(sourceDataGrid->GetGranularity());
	kwFrequencyTable->SetGarbageModalityNumber(oneAttributeDataGrid.GetAttributeAt(0)->GetGarbageModalityNumber());

	// Parcours des parties sources
	for (nPartIndex = 0; nPartIndex < nSourceValueNumber; nPartIndex++)
	{
		// Extraction de la partie courant a partir de l'attribut de grille
		dgPart = cast(KWDGPart*, oaParts.GetAt(nPartIndex));

		assert(dgPart->GetCellNumber() == 1);

		dgCell = dgPart->GetHeadCell();
		nTargetValueNumber = dgCell->GetTargetValueNumber();

		if (bDisplayResults)
		{
			cout << " Partie " << nPartIndex << " Contenu " << endl;
			dgCell->Write(cout);
		}

		// Acces au vecteur (sense etre en representation dense)
		kwdfvFrequencyVector =
		    cast(KWDenseFrequencyVector*, kwFrequencyTable->GetFrequencyVectorAt(nPartIndex));

		// Recopie de son contenu
		ivFrequency = kwdfvFrequencyVector->GetFrequencyVector();
		ivFrequency->SetSize(nTargetValueNumber);
		for (nTargetIndex = 0; nTargetIndex < ivFrequency->GetSize(); nTargetIndex++)
			ivFrequency->SetAt(nTargetIndex, dgCell->GetTargetFrequencyAt(nTargetIndex));

		// Memorisation eventuelle du groupe poubelle
		if (KWType::IsCoclusteringGroupableType(oneAttributeDataGrid.GetAttributeAt(0)->GetAttributeType()))
		{
			// Recopie du nombre de modalites
			kwdfvFrequencyVector->SetModalityNumber(dgPart->GetValueSet()->GetValueNumber());
		}
	}
	if (bDisplayResults)
	{
		cout << "Table " << endl;
		cout << *kwFrequencyTable;
	}
	assert(kwFrequencyTable->GetTotalFrequency() == sourceDataGrid->GetGridFrequency());
}

void KWDataGridManager::ExportAttributes(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Initialisation de la grille cible
	InitialiseDataGrid(sourceDataGrid, sourceDataGrid->GetAttributeNumber(), targetDataGrid);

	// Initialisation des attributs
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut source et cible
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Transfert du parametrage de l'attribut
		InitialiseAttribute(sourceAttribute, targetAttribute);
	}
	ensure(CheckAttributes(sourceDataGrid, targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportParts(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(sourceDataGrid, targetDataGrid) and
		CheckGranularity(sourceDataGrid, targetDataGrid));

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);
		assert(targetAttribute->GetPartNumber() == 0);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());

		// Transfert du parametrage des parties de l'attribut
		InitialiseAttributeParts(sourceAttribute, targetAttribute);
	}
	ensure(CheckParts(sourceDataGrid, targetDataGrid));
}

void KWDataGridManager::ExportAttributeParts(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid,
					     const ALString& sAttributeName) const
{
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(sourceDataGrid, targetDataGrid));
	require(sourceDataGrid->SearchAttribute(sAttributeName) != NULL);
	require(targetDataGrid->SearchAttribute(sAttributeName) != NULL);
	require(targetDataGrid->SearchAttribute(sAttributeName)->GetPartNumber() == 0);

	// Recherche des attributs source et cible dans la grille directement
	sourceAttribute = sourceDataGrid->SearchAttribute(sAttributeName);
	targetAttribute = targetDataGrid->SearchAttribute(sAttributeName);

	// Transfert du parametrage des parties de l'attribut
	InitialiseAttributeParts(sourceAttribute, targetAttribute);
}

void KWDataGridManager::ExportCells(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const
{
	KWDGCell* sourceCell;
	KWDGCell* targetCell;
	int nAttribute;
	ObjectArray oaSourceAttributes;
	ObjectArray oaTargetParts;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	Continuous cValue;
	Symbol sValue;
	KWDGPart* sourceVarPart;
	KWDGPart* targetVarPart;
	KWDGAttribute* innerAttribute;

	require(Check());
	require(targetDataGrid != NULL and CheckTargetValues(sourceDataGrid, targetDataGrid) and
		CheckAttributes(sourceDataGrid, targetDataGrid) and CheckParts(sourceDataGrid, targetDataGrid) and
		targetDataGrid->GetCellNumber() == 0);

	// Passage de la grille cible en mode update
	targetDataGrid->SetCellUpdateMode(true);
	targetDataGrid->BuildIndexingStructure();
	oaTargetParts.SetSize(targetDataGrid->GetAttributeNumber());

	// Collecte une fois pour toutes des attributs sources correspondant aux attributs cibles,
	// car il faudra y acceder rapidement autant de fois qu'il y a de cellules
	oaSourceAttributes.SetSize(targetDataGrid->GetAttributeNumber());
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Memorisation au meme index
		oaSourceAttributes.SetAt(nAttribute, sourceAttribute);
	}

	// Transfert des cellules sources
	sourceCell = sourceDataGrid->GetHeadCell();
	while (sourceCell != NULL)
	{
		// Recherche des parties cible pour les valeurs de la cellule courante
		for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
		{
			targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

			// Recherche de l'attribut source correspondant dans le tableau ou ils ont ete collectes
			sourceAttribute = cast(KWDGAttribute*, oaSourceAttributes.GetAt(nAttribute));

			// Recherche de la partie associee a la cellule selon son type
			sourcePart = sourceCell->GetPartAt(sourceAttribute->GetAttributeIndex());

			// Cas d'une partie de type Continuous
			if (sourcePart->GetPartType() == KWType::Continuous)
			{
				// Recherche d'une valeur typique: le milieu de l'intervalle (hors borne inf)
				cValue = KWContinuous::GetUpperMeanValue(sourcePart->GetInterval()->GetLowerBound(),
									 sourcePart->GetInterval()->GetUpperBound());

				// Recherche de l'intervalle cible correspondant
				targetPart = targetAttribute->LookupContinuousPart(cValue);
				oaTargetParts.SetAt(nAttribute, targetPart);
			}
			// Cas d'une partie de type Symbol
			else if (sourcePart->GetPartType() == KWType::Symbol)
			{
				// Recherche d'une valeur typique: la premiere valeur
				assert(sourcePart->GetValueSet()->GetHeadValue() != NULL);
				sValue = sourcePart->GetValueSet()->GetHeadValue()->GetSymbolValue();

				// Recherche du groupe de valeurs cible correspondant
				targetPart = targetAttribute->LookupSymbolPart(sValue);
				oaTargetParts.SetAt(nAttribute, targetPart);
			}
			// Cas d'une partie de type partie de variable
			else
			{
				// Recherche d'une partie de variable typique : la premiere partie de variable
				assert(sourcePart->GetVarPartSet()->GetHeadValue() != NULL);

				sourceVarPart = sourcePart->GetVarPartSet()->GetHeadValue()->GetVarPart();

				// Recherche de la partie de variable cible qui contient cette partie de variable source
				// (suite a la granularisation) Extraction de l'attribut de cette partie de variable
				innerAttribute = targetAttribute->GetInnerAttributes()->LookupInnerAttribute(
				    sourceVarPart->GetAttribute()->GetAttributeName());

				// Cas d'une partie de variable continue
				if (sourceVarPart->GetPartType() == KWType::Continuous)
				{
					// Recherche d'une valeur typique: le milieu de l'intervalle (hors borne inf)
					cValue = KWContinuous::GetUpperMeanValue(
					    sourceVarPart->GetInterval()->GetLowerBound(),
					    sourceVarPart->GetInterval()->GetUpperBound());

					// Recherche de l'intervalle cible correspondant pour l'attribut interne
					targetVarPart = innerAttribute->LookupContinuousPart(cValue);

					// Recherche de la partie de l'attribut correspondant
					targetPart = targetAttribute->LookupVarPart(targetVarPart);
					oaTargetParts.SetAt(nAttribute, targetPart);
				}
				else if (sourceVarPart->GetPartType() == KWType::Symbol)
				{
					// Recherche d'une valeur typique: la premiere valeur
					assert(sourceVarPart->GetValueSet()->GetHeadValue() != NULL);
					sValue = sourceVarPart->GetValueSet()->GetHeadValue()->GetSymbolValue();

					// Recherche du groupe de valeurs cible correspondant
					targetVarPart = innerAttribute->LookupSymbolPart(sValue);

					// Recherche de la partie de l'attribut correspondant
					targetPart = targetAttribute->LookupVarPart(targetVarPart);
					oaTargetParts.SetAt(nAttribute, targetPart);
				}
			}
		}

		// Creation de la cellule cible si necessaire
		targetCell = targetDataGrid->LookupCell(&oaTargetParts);
		if (targetCell == NULL)
			targetCell = targetDataGrid->AddCell(&oaTargetParts);
		check(targetCell);

		// Mise a jour de la cellule cible
		targetCell->AddFrequenciesFrom(sourceCell);

		// Cellule source suivante
		sourceDataGrid->GetNextCell(sourceCell);
	}

	// Fin du mode update
	targetDataGrid->SetCellUpdateMode(false);
	targetDataGrid->DeleteIndexingStructure();

	// Pas d'ensure avec CheckCells (qui appelle ExportCells pour sa verification)
}

void KWDataGridManager::ExportRandomAttributes(const KWDataGrid* sourceDataGrid, int nAttributeNumber,
					       KWDataGrid* targetDataGrid) const
{
	int nSourceAttribute;
	int nTargetAttribute;
	IntVector ivSourceAttributeIndexes;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(0 <= nAttributeNumber and nAttributeNumber <= sourceDataGrid->GetAttributeNumber());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Initialisation de la grille cible
	InitialiseDataGrid(sourceDataGrid, nAttributeNumber, targetDataGrid);

	// Creation d'un vecteur d'index d'attributs cibles choisis aleatoirement
	ivSourceAttributeIndexes.SetSize(sourceDataGrid->GetAttributeNumber());
	for (nSourceAttribute = 0; nSourceAttribute < ivSourceAttributeIndexes.GetSize(); nSourceAttribute++)
		ivSourceAttributeIndexes.SetAt(nSourceAttribute, nSourceAttribute);
	ivSourceAttributeIndexes.Shuffle();
	ivSourceAttributeIndexes.SetSize(nAttributeNumber);
	ivSourceAttributeIndexes.Sort();

	// Initialisation des attributs
	for (nTargetAttribute = 0; nTargetAttribute < targetDataGrid->GetAttributeNumber(); nTargetAttribute++)
	{
		// Recherche de l'attribut source et cible
		nSourceAttribute = ivSourceAttributeIndexes.GetAt(nTargetAttribute);
		sourceAttribute = sourceDataGrid->GetAttributeAt(nSourceAttribute);
		targetAttribute = targetDataGrid->GetAttributeAt(nTargetAttribute);

		// Transfert du parametrage de l'attribut
		InitialiseAttribute(sourceAttribute, targetAttribute);
	}
	ensure(CheckAttributes(sourceDataGrid, targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
	ensure(not targetDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::AddRandomAttributes(const KWDataGrid* sourceDataGrid, const KWDataGrid* mandatoryDataGrid,
					    int nRequestedAttributeNumber, KWDataGrid* targetDataGrid) const
{
	int nSourceAttribute;
	int nTargetAttribute;
	int nAttributeNumber;
	IntVector ivSourceAttributeIndexes;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(0 <= nRequestedAttributeNumber and nRequestedAttributeNumber <= sourceDataGrid->GetAttributeNumber());
	require(mandatoryDataGrid != NULL);
	require(CheckAttributes(sourceDataGrid, mandatoryDataGrid));
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Calcul du nombre d'attribut a exporter
	nAttributeNumber = mandatoryDataGrid->GetAttributeNumber();
	if (nAttributeNumber < nRequestedAttributeNumber)
		nAttributeNumber = nRequestedAttributeNumber;

	// Initialisation de la grille cible
	InitialiseDataGrid(sourceDataGrid, nAttributeNumber, targetDataGrid);

	// Creation d'un vecteur d'index d'attributs cibles choisis aleatoirement,
	// parmi les attributs non deja present dans les attributs obligatoires
	for (nSourceAttribute = 0; nSourceAttribute < sourceDataGrid->GetAttributeNumber(); nSourceAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nSourceAttribute);
		if (mandatoryDataGrid->SearchAttribute(sourceAttribute->GetAttributeName()) == NULL)
			ivSourceAttributeIndexes.Add(nSourceAttribute);
	}
	assert(ivSourceAttributeIndexes.GetSize() ==
	       sourceDataGrid->GetAttributeNumber() - mandatoryDataGrid->GetAttributeNumber());
	ivSourceAttributeIndexes.Shuffle();
	ivSourceAttributeIndexes.SetSize(nAttributeNumber - mandatoryDataGrid->GetAttributeNumber());

	// On rajoute les attributs obligatoires, transferes inconditionnellements
	for (nSourceAttribute = 0; nSourceAttribute < sourceDataGrid->GetAttributeNumber(); nSourceAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nSourceAttribute);
		if (mandatoryDataGrid->SearchAttribute(sourceAttribute->GetAttributeName()) != NULL)
			ivSourceAttributeIndexes.Add(nSourceAttribute);
	}
	assert(ivSourceAttributeIndexes.GetSize() == nAttributeNumber);
	ivSourceAttributeIndexes.Sort();

	// Initialisation des attributs
	for (nTargetAttribute = 0; nTargetAttribute < targetDataGrid->GetAttributeNumber(); nTargetAttribute++)
	{
		// Recherche de l'attribut source et cible
		nSourceAttribute = ivSourceAttributeIndexes.GetAt(nTargetAttribute);
		sourceAttribute = sourceDataGrid->GetAttributeAt(nSourceAttribute);
		targetAttribute = targetDataGrid->GetAttributeAt(nTargetAttribute);

		// Transfert du parametrage de l'attribut
		InitialiseAttribute(sourceAttribute, targetAttribute);
	}
	ensure(CheckAttributes(sourceDataGrid, targetDataGrid));
	ensure(targetDataGrid->GetAttributeNumber() >= mandatoryDataGrid->GetAttributeNumber());
	ensure(targetDataGrid->GetAttributeNumber() >= nRequestedAttributeNumber);
	ensure(targetDataGrid->GetCellNumber() == 0);
	ensure(not targetDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::AddRandomParts(const KWDataGrid* sourceDataGrid, const KWDataGrid* mandatoryDataGrid,
				       int nAddedPartNumber, KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* mandatoryAttribute;
	KWDGAttribute* targetAttribute;
	int nRequestedPartNumber;
	int nMinimimEqualFrequencyPartNumber;
	boolean bEqualFrequencyConstraint;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(sourceDataGrid, targetDataGrid) and
		CheckGranularity(sourceDataGrid, targetDataGrid));
	require(mandatoryDataGrid != NULL and CheckAttributes(sourceDataGrid, mandatoryDataGrid) and
		CheckGranularity(sourceDataGrid, mandatoryDataGrid));
	require(1 <= nAddedPartNumber and nAddedPartNumber <= sourceDataGrid->GetGridFrequency());

	// Parametrage de la contrainte d'effectif egaux par partie, pour tous les attributs
	bEqualFrequencyConstraint = RandomDouble() <= 0.5;

	// Ajout des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche des attributs cible, initial et source
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);
		assert(sourceAttribute->GetAttributeType() == targetAttribute->GetAttributeType());

		// Recherche de l'attribut obligatoire correspondant
		mandatoryAttribute = mandatoryDataGrid->SearchAttribute(targetAttribute->GetAttributeName());

		// Nombre initial de parties demandees
		nRequestedPartNumber = nAddedPartNumber;

		// Prise en compte des partie existantes en cas d'attribut obligatoire
		if (mandatoryAttribute != NULL)
			nRequestedPartNumber += mandatoryAttribute->GetPartNumber();
		nRequestedPartNumber = min(nRequestedPartNumber, sourceAttribute->GetPartNumber());

		// On prend en compte la contrainte d'equilibrage des effectifs des parties de facon aleatoire,
		// avec une fois sur deux pas de contrainte
		if (bEqualFrequencyConstraint)
			nMinimimEqualFrequencyPartNumber = nRequestedPartNumber;
		// En cas sans contrainte, on choisit aleatoirement le niveau de contrainte
		else
			nMinimimEqualFrequencyPartNumber =
			    nRequestedPartNumber +
			    (int)(RandomDouble() * (sourceDataGrid->GetGridFrequency() - -nRequestedPartNumber));

		// Ajout d'un sous ensemble de parties aleatoire dans le cas continuous
		if (sourceAttribute->GetAttributeType() == KWType::Continuous)
			ExportContinuousAttributeRandomParts(sourceAttribute, mandatoryAttribute, nRequestedPartNumber,
							     nMinimimEqualFrequencyPartNumber, true, targetAttribute);
		// Ajout d'un sous ensemble de parties aleatoire dans le cas grouable
		else
			ExportGroupableAttributeRandomParts(sourceAttribute, mandatoryAttribute, nRequestedPartNumber,
							    nMinimimEqualFrequencyPartNumber, true, targetAttribute);
	}
	ensure(CheckParts(sourceDataGrid, targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
}

void KWDataGridManager::BuildUnivariateDataGridFromAttributeStats(const KWDataGrid* sourceDataGrid,
								  const KWAttributeStats* attributeStats,
								  KWDataGrid* targetDataGrid) const
{
	KWDGAttribute* targetAttribute;

	require(Check());
	require(sourceDataGrid->GetTargetValueNumber() > 0);
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(attributeStats != NULL);
	require(attributeStats->GetAttributeType() == KWType::Symbol or
		attributeStats->GetAttributeType() == KWType::Continuous);
	require(sourceDataGrid->SearchAttribute(attributeStats->GetAttributeName()) != NULL);
	require(attributeStats->GetPreparedDataGridStats()->GetSourceAttributeNumber() == 1);

	// Initialisation de la grille cible
	InitialiseDataGrid(sourceDataGrid, 1, targetDataGrid);

	// Initialisation de l'attribut
	targetAttribute = targetDataGrid->GetAttributeAt(0);
	BuildDataGridAttributeFromUnivariateStats(sourceDataGrid, attributeStats, targetAttribute);
	targetAttribute->SetAttributeTargetFunction(false);

	// Export des cellules
	ExportCells(sourceDataGrid, targetDataGrid);
	ensure(targetDataGrid->Check());
	ensure(CheckDataGrid(sourceDataGrid, targetDataGrid));
}

void KWDataGridManager::BuildDataGridAttributeFromUnivariateStats(const KWDataGrid* sourceDataGrid,
								  const KWAttributeStats* attributeStats,
								  KWDGAttribute* targetAttribute) const
{
	int nInstanceNumber;
	const KWDGSAttributePartition* attributePartition;
	int nPart;
	KWDGPart* part;
	KWDGInterval* interval;
	KWDGSymbolValueSet* symbolValueSet;
	int nValue;
	KWAttribute* attribute;
	KWDGAttribute* sourceAttribute;

	require(targetAttribute != NULL);
	require(attributeStats != NULL);

	// Nombre d'instances
	nInstanceNumber = sourceDataGrid->GetGridFrequency();
	assert(attributeStats->GetPreparedDataGridStats()->ComputeGridFrequency() == nInstanceNumber);

	// Extraction de l'attribut source
	sourceAttribute = sourceDataGrid->SearchAttribute(attributeStats->GetAttributeName());
	assert(sourceAttribute != NULL);

	// Initialisation de l'attribut
	targetAttribute->SetAttributeName(attributeStats->GetAttributeName());
	targetAttribute->SetAttributeType(attributeStats->GetAttributeType());

	// Recuperation du cout de selection/construction de l'attribut
	attribute = attributeStats->GetClass()->LookupAttribute(attributeStats->GetAttributeName());
	check(attribute);
	targetAttribute->SetCost(attribute->GetCost());

	// Acces a la partition
	attributePartition = attributeStats->GetPreparedDataGridStats()->GetAttributeAt(0);
	assert(not attributePartition->ArePartsSingletons());

	// Initialisation des parties des attributs en fonction du type d'attribut
	// Cas d'une discretisation
	if (attributePartition->GetAttributeType() == KWType::Continuous)
	{
		const KWDGSAttributeDiscretization* attributeDiscretization;
		attributeDiscretization = cast(const KWDGSAttributeDiscretization*, attributePartition);

		targetAttribute->SetInitialValueNumber(attributeDiscretization->GetInitialValueNumber());
		targetAttribute->SetGranularizedValueNumber(attributeDiscretization->GetGranularizedValueNumber());

		// Initialisation des intervalles
		for (nPart = 0; nPart < attributeDiscretization->GetPartNumber(); nPart++)
		{
			part = targetAttribute->AddPart();
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
	// Cas d'un groupement de valeurs
	else if (attributePartition->GetAttributeType() == KWType::Symbol)
	{
		const KWDGSAttributeGrouping* attributeGrouping;
		attributeGrouping = cast(const KWDGSAttributeGrouping*, attributePartition);

		targetAttribute->SetInitialValueNumber(attributeGrouping->GetInitialValueNumber());
		targetAttribute->SetGranularizedValueNumber(attributeGrouping->GetGranularizedValueNumber());

		// Creation des parties, en collectant les valeurs et leur effectif
		for (nPart = 0; nPart < attributeGrouping->GetPartNumber(); nPart++)
		{
			part = targetAttribute->AddPart();
			symbolValueSet = part->GetSymbolValueSet();

			// Initialisation des valeurs du groupe
			for (nValue = attributeGrouping->GetGroupFirstValueIndexAt(nPart);
			     nValue <= attributeGrouping->GetGroupLastValueIndexAt(nPart); nValue++)
				symbolValueSet->AddSymbolValue(attributeGrouping->GetValueAt(nValue));

			// Memorisation du groupe poubelle
			if (nPart == attributeGrouping->GetGarbageGroupIndex())
				targetAttribute->SetGarbagePart(part);
		}

		// Export des effectif des valeurs de la grille initiale pour finaliser la specification
		ExportAttributeSymbolValueFrequencies(sourceAttribute, targetAttribute);
	}
}

boolean KWDataGridManager::BuildDataGridFromUnivariateProduct(const KWDataGrid* sourceDataGrid,
							      KWClassStats* classStats,
							      KWDataGrid* targetDataGrid) const
{
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWAttributeStats* attributeStats;
	ObjectArray oaAllAttributeStats;
	ObjectArray oaSelectedAtttributes;
	NumericKeyDictionary nkdBestAttributeStats;
	int nMaxAttributeNumber;
	int nAttributeNumber;
	int nInstanceNumber;
	int nAttribute;
	boolean bOk = true;
	boolean bSmallSourceDataGrid;
	boolean bDisplayResults = false;

	require(Check());
	require(sourceDataGrid->GetTargetValueNumber() > 0);
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(classStats != NULL);
	require(classStats->GetInformativeAttributeNumber() > 0);

	// Nombre d'instance
	nInstanceNumber = sourceDataGrid->GetGridFrequency();

	// Calcul du nombre d'attributs a prendre en compte
	nAttributeNumber = sourceDataGrid->GetAttributeNumber();
	nMaxAttributeNumber = 1 + (int)(2 * log((double)sourceDataGrid->GetGridFrequency()) / log(2.0));

	// Test sur la taille du DataGrid source pour determiner le mode d'utilisation
	// En mode de recherche d'agregat (bSmallSourceDataGrid), la grille a optimiser
	// est relative a un sous-ensemble restreint de l'ensemble des attributs.
	// En mode de modalisation, la grille a optimiser porte sur tous les attributs,
	// et on effectue au prealable une selection sur les meilleurs attributs univaries
	bSmallSourceDataGrid = nAttributeNumber <= nMaxAttributeNumber or
			       (nAttributeNumber < classStats->GetAttributeStats()->GetSize() and
				nAttributeNumber < classStats->GetInformativeAttributeNumber());

	// On fait une selection sur les attributs si necessaire
	// On conserve la selection sur le Level qui correspond a la partition optimale toute granularites confondues
	if (not bSmallSourceDataGrid)
	{
		// Rangement de toutes les statistiques d'attributs d'interet non nul dans un tableau
		for (nAttribute = 0; nAttribute < classStats->GetAttributeStats()->GetSize(); nAttribute++)
		{
			attributeStats = cast(KWAttributeStats*, classStats->GetAttributeStats()->GetAt(nAttribute));
			if (attributeStats->GetLevel() > 0 and
			    attributeStats->GetPreparedDataGridStats()->GetSourceAttributeNumber() > 0)
				oaAllAttributeStats.Add(attributeStats);
		}

		// Tri par importance
		oaAllAttributeStats.SetCompareFunction(KWLearningReportCompareSortValue);
		oaAllAttributeStats.Sort();

		// Limitation du nombre d'attributs a prendre en compte
		if (oaAllAttributeStats.GetSize() > nMaxAttributeNumber)
			oaAllAttributeStats.SetSize(nMaxAttributeNumber);
		nAttributeNumber = oaAllAttributeStats.GetSize();

		// On memorise dans un dictionnaire les attributs selectionnes
		for (nAttribute = 0; nAttribute < oaAllAttributeStats.GetSize(); nAttribute++)
		{
			nkdBestAttributeStats.SetAt(oaAllAttributeStats.GetAt(nAttribute),
						    oaAllAttributeStats.GetAt(nAttribute));
		}
	}

	// Collecte des attributs
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);

		// Initialisation de l'attribut s'il a ete selectionne
		attributeStats = classStats->LookupAttributeStats(sourceAttribute->GetAttributeName());
		if (attributeStats->GetLevel() > 0 and
		    (bSmallSourceDataGrid or nkdBestAttributeStats.Lookup(attributeStats) != NULL))
			oaSelectedAtttributes.Add(sourceAttribute);

		// Arret si grille cible complete
		if (oaSelectedAtttributes.GetSize() == nAttributeNumber)
			break;
	}

	// Creation de la grille si au moins dex attributs
	bOk = oaSelectedAtttributes.GetSize() >= 2;
	if (bOk)
	{
		// Initialisation de la grille cible
		InitialiseDataGrid(sourceDataGrid, oaSelectedAtttributes.GetSize(), targetDataGrid);

		// Creation des partitions
		for (nAttribute = 0; nAttribute < oaSelectedAtttributes.GetSize(); nAttribute++)
		{
			// Extraction attribut initial granularise
			sourceAttribute = cast(KWDGAttribute*, oaSelectedAtttributes.GetAt(nAttribute));
			targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

			// Transfert du parametrage de l'attribut
			InitialiseAttribute(sourceAttribute, targetAttribute);

			// Appel de la methode de construction de l'attribut cible par calcul de la partition optimale
			// pour la granularite de l'attribut source
			BuildDataGridAttributeFromGranularizedPartition(sourceDataGrid, classStats, sourceAttribute,
									targetAttribute);
		}
		// Export des nouvelles cellules
		targetDataGrid->DeleteAllCells();
		ExportCells(sourceDataGrid, targetDataGrid);
	}
	if (bDisplayResults)
		cout << " OptimizeWithMultipleUnivariatePartitions : construction grille initiale achevee" << endl;
	ensure(not bOk or targetDataGrid->Check());
	ensure(not bOk or CheckDataGrid(sourceDataGrid, targetDataGrid));
	return bOk;
}

void KWDataGridManager::BuildPartsOfContinuousAttributeFromFrequencyTable(const KWDataGrid* sourceDataGrid,
									  const KWFrequencyTable* kwftTable,
									  const ALString& sAttributeName,
									  KWDGAttribute* targetAttribute) const
{
	KWDGAttribute* sourceAttribute;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	ObjectArray oaSourceParts;
	IntVector ivIntervalUpperBounds;
	int nSourcePart;
	int nTargetPart;
	int nBoundIndex;
	int nInstanceNumber;
	int nInstanceLastIndex;
	boolean bDisplayResults = false;

	require(targetAttribute != NULL);
	require(sAttributeName != "");

	// Nombre d'instances
	nInstanceNumber = sourceDataGrid->GetGridFrequency();
	assert(kwftTable->GetTotalFrequency() == nInstanceNumber);

	// Extraction de l'attribut source associe
	sourceAttribute = sourceDataGrid->SearchAttribute(sAttributeName);
	assert(sourceAttribute != NULL);

	// Nettoyage des parties eventuelles de l'attribut cible
	targetAttribute->DeleteAllParts();

	// Export des parties de l'attribut source
	sourceAttribute->ExportParts(&oaSourceParts);

	// Tri des intervalles source par borne inf croissante
	oaSourceParts.SetCompareFunction(KWDGPartCompareValues);
	oaSourceParts.Sort();

	// Recuperation des effectifs "source" de la table d'effectifs
	nBoundIndex = 0;
	for (nTargetPart = 0; nTargetPart < kwftTable->GetFrequencyVectorNumber(); nTargetPart++)
	{
		nBoundIndex += kwftTable->GetFrequencyVectorAt(nTargetPart)->ComputeTotalFrequency();
		ivIntervalUpperBounds.Add(nBoundIndex);
	}

	// Creation des intervalles cibles selon les effectifs recuperes
	targetPart = NULL;
	nBoundIndex = 0;
	nInstanceLastIndex = 0;
	for (nSourcePart = 0; nSourcePart < oaSourceParts.GetSize(); nSourcePart++)
	{
		// Affichage de l'index de la partie
		if (bDisplayResults)
			cout << " part " << nSourcePart << endl;

		sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSourcePart));

		// Comptage du nombre d'instance sources traitees
		nInstanceLastIndex += sourcePart->GetPartFrequency();

		// Creation si necessaire d'un intervalle cible
		if (targetPart == NULL)
		{
			targetPart = targetAttribute->AddPart();
			// Initialisation de ses bornes
			targetPart->GetInterval()->CopyFrom(sourcePart->GetInterval());
		}
		// Sinon, mise a jour de la borne sup de l'intervalle cible en cours
		else
		{
			targetPart->GetInterval()->SetUpperBound(sourcePart->GetInterval()->GetUpperBound());
		}

		// L'intervalle cible est finalise si son effectif est atteint
		if (nBoundIndex < ivIntervalUpperBounds.GetSize() and
		    nInstanceLastIndex >= ivIntervalUpperBounds.GetAt(nBoundIndex))
		{
			// On reinitialise l'indicateur de creation d'intervalle cible
			targetPart = NULL;

			// On recherche la prochaine borne d'intervalle a depasser
			while (nBoundIndex < ivIntervalUpperBounds.GetSize())
			{
				if (ivIntervalUpperBounds.GetAt(nBoundIndex) <= nInstanceLastIndex)
					nBoundIndex++;
				else
					break;
			}
		}
	}
}

void KWDataGridManager::BuildPartsOfSymbolAttributeFromGroupsIndex(const KWDGAttribute* initialAttribute,
								   const IntVector* ivGroups, int nGroupNumber,
								   int nGarbageModalityNumber,
								   KWDGAttribute* targetAttribute) const
{
	ObjectArray oaTargetParts;
	KWDGPart* initialPart;
	KWDGPart* targetPart;
	int nGroup;
	int nInitial;
	int nMaxValueNumber;
	boolean bDisplayResults = false;

	require(targetAttribute != NULL);
	require(nGroupNumber > 0);
	require(ivGroups != NULL);

	// Acces aux attributs des grilles initiale et optimise pour l'attribut de post-optimisation
	assert(initialAttribute != NULL);

	// Nettoyage des parties eventuelles de l'attribut cible
	targetAttribute->DeleteAllParts();

	// Creation des parties de l'attribut groupee et memorisation dans un tableau
	oaTargetParts.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
	{
		// Creation d'une nouvelle partie optimisee
		targetPart = targetAttribute->AddPart();
		oaTargetParts.SetAt(nGroup, targetPart);
	}

	// Parcours des parties initiales pour determiner les definitions des groupes
	initialPart = initialAttribute->GetHeadPart();
	nInitial = 0;
	nMaxValueNumber = 0;
	while (initialPart != NULL)
	{
		// Recherche de l'index du groupe correspondant
		nGroup = ivGroups->GetAt(nInitial);
		assert(0 <= nGroup and nGroup < nGroupNumber);

		// Recherche de la partie optimisee a mettre a jour
		targetPart = cast(KWDGPart*, oaTargetParts.GetAt(nGroup));

		// Mise a jour de la definition du group
		targetPart->GetValueSet()->UpgradeFrom(initialPart->GetValueSet());

		// Memorisation de la partie comme partie poubelle.
		// Si elle existe, elle maximise le nombre de modalites
		if (nGarbageModalityNumber > 0 and targetPart->GetValueSet()->GetValueNumber() > nMaxValueNumber)
		{
			targetAttribute->SetGarbagePart(targetPart);
			nMaxValueNumber = targetPart->GetValueSet()->GetValueNumber();
		}

		// Partie initiale suivante
		initialAttribute->GetNextPart(initialPart);
		nInitial++;
	}
	// On doit avoir identifie un groupe poubelle dont le nombre de modalites est nGarbageModalityNumber
	// CH V9 TODO cas d'egalite avec deux groupes maximisant le nombre de modalites : la valeur du critere est la
	// meme quelque soit le groupe choisi comme groupe poubelle
	assert(nMaxValueNumber == nGarbageModalityNumber);

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Preparation d'un attribut Symbol associe a un groupage univarie \t"
		     << initialAttribute->GetAttributeName() << endl;
		cout << "Grille initiale\n" << *initialAttribute << endl;
		cout << "Grille optimisee\n" << *targetAttribute << endl;
	}

	// Verification de la grille preparee
	ensure(targetAttribute->GetPartNumber() == nGroupNumber);
	ensure(targetAttribute->GetGarbageModalityNumber() == nGarbageModalityNumber);
}

void KWDataGridManager::BuildUnivariateDataGridFromGranularizedPartition(const KWDataGrid* sourceDataGrid,
									 KWClassStats* classStats, int nAttributeIndex,
									 KWDataGrid* univariateTargetDataGrid) const
{
	KWDGAttribute* targetAttribute;
	KWDGAttribute* sourceAttribute;

	require(0 <= nAttributeIndex and nAttributeIndex < sourceDataGrid->GetAttributeNumber());

	// Initialisation de la grille cible a une variable
	InitialiseDataGrid(sourceDataGrid, 1, univariateTargetDataGrid);

	// Initialisation de l'attribut cible
	sourceAttribute = sourceDataGrid->GetAttributeAt(nAttributeIndex);
	targetAttribute = univariateTargetDataGrid->GetAttributeAt(0);
	InitialiseAttribute(sourceAttribute, targetAttribute);

	// Construction de la partition optimale associee a la granularite de l'attribut source selon classStats
	BuildDataGridAttributeFromGranularizedPartition(sourceDataGrid, classStats, sourceAttribute, targetAttribute);

	// Export des cellules selon la nouvelle partition
	univariateTargetDataGrid->DeleteAllCells();
	ExportCells(sourceDataGrid, univariateTargetDataGrid);
	ensure(univariateTargetDataGrid->Check());
}

void KWDataGridManager::BuildDataGridAttributeFromGranularizedPartition(const KWDataGrid* sourceDataGrid,
									KWClassStats* classStats,
									const KWDGAttribute* sourceAttribute,
									KWDGAttribute* targetAttribute) const
{
	const KWDiscretizerMODL discretizerMODLRef;
	const KWGrouperMODL grouperMODLRef;
	KWDiscretizerMODL* discretizerMODL;
	KWGrouperMODL* grouperMODL;
	KWFrequencyTable* kwftSource;
	KWFrequencyTable* kwftTarget;
	KWDGPart* targetPart;
	KWDGPart* sourcePart;
	IntVector* ivGroups;
	boolean bEvaluated;

	// Initialisation
	kwftTarget = NULL;
	ivGroups = NULL;

	// Cas d'un attribut continu
	if (sourceAttribute->GetAttributeType() == KWType::Continuous)
	{
		discretizerMODL = NULL;

		assert(classStats->GetLearningSpec()
			   ->GetPreprocessingSpec()
			   ->GetDiscretizerSpec()
			   ->GetDiscretizer(classStats->GetTargetAttributeType())
			   ->GetName() == discretizerMODLRef.GetName());

		discretizerMODL =
		    cast(KWDiscretizerMODL*,
			 classStats->GetLearningSpec()->GetPreprocessingSpec()->GetDiscretizerSpec()->GetDiscretizer(
			     classStats->GetTargetAttributeType()));

		// Parametrage du cout de l'attribut
		discretizerMODL->GetDiscretizationCosts()->SetAttributeCost(sourceAttribute->GetCost());

		// On doit calculer la partition univariee associee a l'attribut granularise
		kwftSource = new KWFrequencyTable;
		ExportFrequencyTableFromOneAttribute(sourceDataGrid, sourceAttribute->GetAttributeName(), kwftSource);

		// Discretisation univariee optimale de l'attribut granularise
		discretizerMODL->DiscretizeGranularizedFrequencyTable(kwftSource, kwftTarget);

		// Nettoyage
		delete kwftSource;
		kwftSource = NULL;

		bEvaluated = kwftTarget->GetFrequencyVectorNumber() > 1;
		if (bEvaluated)
		{
			// Construction des parties de l'attribut associee a cette discretisation
			BuildPartsOfContinuousAttributeFromFrequencyTable(
			    sourceDataGrid, kwftTarget, sourceAttribute->GetAttributeName(), targetAttribute);
			targetAttribute->SetAttributeTargetFunction(false);
		}
		else
		{
			// Creation de l'intervalle
			targetPart = targetAttribute->AddPart();

			// Mise a jour de ses bornes
			targetPart->GetInterval()->SetLowerBound(KWDGInterval::GetMinLowerBound());
			targetPart->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());
		}
	}
	// Cas d'un attribut categoriel
	else
	{
		// Initialisation
		grouperMODL = NULL;
		ivGroups = NULL;
		assert(classStats->GetLearningSpec()
			   ->GetPreprocessingSpec()
			   ->GetGrouperSpec()
			   ->GetGrouper(classStats->GetTargetAttributeType())
			   ->GetName() == grouperMODLRef.GetName());

		grouperMODL = cast(KWGrouperMODL*,
				   classStats->GetLearningSpec()->GetPreprocessingSpec()->GetGrouperSpec()->GetGrouper(
				       classStats->GetTargetAttributeType()));

		// Parametrage du cout de l'attribut
		grouperMODL->GetGroupingCosts()->SetAttributeCost(sourceAttribute->GetCost());

		// Creation de la table a partir de la partition univariee decrite dans la grille
		kwftSource = new KWFrequencyTable;
		ExportFrequencyTableFromOneAttribute(sourceDataGrid, sourceAttribute->GetAttributeName(), kwftSource);

		// Groupage de la table d'effectifs source
		grouperMODL->GroupFrequencyTable(kwftSource, kwftTarget, ivGroups);
		delete kwftSource;
		kwftSource = NULL;

		bEvaluated = kwftTarget->GetFrequencyVectorNumber() > 1;
		if (bEvaluated)
		{
			// Construction des parties de l'attribut associee au groupage
			BuildPartsOfSymbolAttributeFromGroupsIndex(
			    sourceAttribute, ivGroups, kwftTarget->GetFrequencyVectorNumber(),
			    kwftTarget->GetGarbageModalityNumber(), targetAttribute);

			targetAttribute->SetAttributeTargetFunction(false);
		}
		else
		{
			// Creation de l'ensemble des valeurs cible
			targetPart = targetAttribute->AddPart();

			// Transfert des valeurs des parties de l'attribut source
			sourcePart = sourceAttribute->GetHeadPart();
			while (sourcePart != NULL)
			{
				// Concatenation dans la partie cible des valeurs source
				targetPart->GetValueSet()->UpgradeFrom(sourcePart->GetValueSet());

				// Partie suivante
				sourceAttribute->GetNextPart(sourcePart);
			}
		}

		// Export des effectif des valeurs de la grille initiale pour finaliser la specification
		ExportAttributeSymbolValueFrequencies(sourceAttribute, targetAttribute);

		delete kwftTarget;
		kwftTarget = NULL;
	}

	// Nettoyage
	if (kwftTarget != NULL)
		delete kwftTarget;
	if (ivGroups != NULL)
	{
		delete ivGroups;
		ivGroups = NULL;
	}
}

boolean KWDataGridManager::CheckDataGrid(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const
{
	require(Check());
	require(targetDataGrid != NULL);

	return CheckGranularity(sourceDataGrid, targetDataGrid) and
	       CheckTargetValues(sourceDataGrid, targetDataGrid) and CheckAttributes(sourceDataGrid, targetDataGrid) and
	       CheckParts(sourceDataGrid, targetDataGrid) and CheckCells(sourceDataGrid, targetDataGrid);
}

boolean KWDataGridManager::CheckGranularity(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const
{
	boolean bOk = true;
	ALString sTmp;

	require(Check());
	require(targetDataGrid != NULL);

	// Verification de la granularite
	if (sourceDataGrid->GetGranularity() != targetDataGrid->GetGranularity())
	{
		targetDataGrid->AddError(sTmp + "Incorrect granularity index (" +
					 IntToString(targetDataGrid->GetGranularity()) + ")");
		bOk = false;
	}

	return bOk;
}

boolean KWDataGridManager::CheckTargetValues(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const
{
	boolean bOk = true;
	int nTarget;
	ALString sTmp;

	require(Check());
	require(targetDataGrid != NULL);

	// Verification du nombre de valeurs cibles
	if (sourceDataGrid->GetTargetValueNumber() != targetDataGrid->GetTargetValueNumber())
	{
		targetDataGrid->AddError(sTmp + "Incorrect number of target values (" +
					 IntToString(targetDataGrid->GetTargetValueNumber()) + ")");
		bOk = false;
	}

	// Verification des valeurs cibles
	if (bOk)
	{
		for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
		{
			if (sourceDataGrid->GetTargetValueAt(nTarget) != targetDataGrid->GetTargetValueAt(nTarget))
			{
				targetDataGrid->AddError(sTmp + "Incorrect target value " + IntToString(nTarget) +
							 " (" + targetDataGrid->GetTargetValueAt(nTarget) + ")");
				bOk = false;
				break;
			}
		}
	}
	return bOk;
}

boolean KWDataGridManager::CheckAttributes(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const
{
	boolean bOk = true;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	ALString sTmp;

	require(Check());
	require(targetDataGrid != NULL);

	// Rercherche d'un attribut source correspondant a chaque attribut cible
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Erreur si pas d'attribut correspondant
		if (sourceAttribute == NULL)
		{
			targetAttribute->AddError("Variable unknown in the source data grid");
			bOk = false;
		}
		// Test de compatibilite du type sinon
		else if (targetAttribute->GetAttributeType() != sourceAttribute->GetAttributeType())
		{
			targetAttribute->AddError(
			    "Type of the variable inconsistent with that in the source data grid");
			bOk = false;
		}
	}
	return bOk;
}

boolean KWDataGridManager::CheckParts(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const
{
	boolean bOk = true;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	int nSourcePart;
	int nTargetPart;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	ObjectArray oaSourceIntervals;
	ObjectArray oaTargetIntervals;
	boolean bIsTargetAttributeIndexed;
	KWDGValueSet* sourceValueSet;
	KWDGValue* sourceValue;
	KWDGPart* headTargetPart;
	ALString sTmp;

	require(Check());
	require(targetDataGrid != NULL);
	require(targetDataGrid->Check());
	require(CheckAttributes(sourceDataGrid, targetDataGrid));

	// Rercherche d'un attribut source correspondant a chaque attribut cible
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Comparaison des intervalles dans le cas numerique
		if (targetAttribute->GetAttributeType() == KWType::Continuous)
		{
			// Ajout des parties source numeriques dans un tableau d'intervalles
			sourceAttribute->ExportParts(&oaSourceIntervals);
			oaSourceIntervals.SetCompareFunction(KWDGPartCompareValues);
			oaSourceIntervals.Sort();

			// Ajout des parties cibles numeriques dans un tableau d'intervalles
			targetAttribute->ExportParts(&oaTargetIntervals);
			oaTargetIntervals.SetCompareFunction(KWDGPartCompareValues);
			oaTargetIntervals.Sort();

			// Parcours des intervalles cibles pour verifier leur compatibilite
			// avec les intervalles source
			// la methode CheckParts est utile afin de test d'integrite pour le developpeur,
			// dans les cas "standard")
			nSourcePart = 0;
			for (nTargetPart = 0; nTargetPart < oaTargetIntervals.GetSize(); nTargetPart++)
			{
				targetPart = cast(KWDGPart*, oaTargetIntervals.GetAt(nTargetPart));

				// Erreur si plus d'intervalle source disponible
				if (nSourcePart >= oaSourceIntervals.GetSize())
				{
					targetPart->AddError("No matching source interval");
					bOk = false;
					break;
				}
				// Controle des bornes de l'intervalle
				else
				{
					sourcePart = cast(KWDGPart*, oaSourceIntervals.GetAt(nSourcePart));

					// Controle de la borne inf
					if (sourcePart->GetInterval()->GetLowerBound() !=
					    targetPart->GetInterval()->GetLowerBound())
					{
						targetPart->AddError(
						    "The matching source interval has not the same lower bound");
						bOk = false;
						break;
					}
					// Recherche d'un intervalle source ayant meme borne sup
					else
					{
						// Parcours des intervalles sources pour trouver celui ayant meme borne
						// sup
						while (sourcePart->GetInterval()->GetUpperBound() !=
						       targetPart->GetInterval()->GetUpperBound())
						{
							nSourcePart++;

							// Erreur si plus d'intervalle source disponible
							if (nSourcePart >= oaSourceIntervals.GetSize())
							{
								targetPart->AddError(
								    "No source interval with the same upper bound");
								bOk = false;
								break;
							}
							// Sinon, on accede a l'intervalle suivant
							else
								sourcePart = cast(KWDGPart*,
										  oaSourceIntervals.GetAt(nSourcePart));
						}

						// Passage a l'intervalle source suivant pour la prochaine etape
						nSourcePart++;
					}
				}
			}

			// Nettoyage
			oaSourceIntervals.SetSize(0);
			oaTargetIntervals.SetSize(0);
		}
		// Comparaison des groupes de valeurs dans le cas symbolique
		else if (targetAttribute->GetAttributeType() == KWType::Symbol)
		{
			// Indexation de l'attribut cible si necessaire
			bIsTargetAttributeIndexed = targetAttribute->IsIndexed();
			if (not bIsTargetAttributeIndexed)
				targetAttribute->BuildIndexingStructure();

			// Parcours des parties sources pour determiner si chacune est incluse integralement
			// dans une partie cible
			sourcePart = sourceAttribute->GetHeadPart();
			while (sourcePart != NULL)
			{
				sourceValueSet = sourcePart->GetValueSet();

				// Recherche de la partie cible associee a la premiere valeur source
				sourceValue = sourceValueSet->GetHeadValue();
				check(sourceValue);
				headTargetPart = targetAttribute->LookupSymbolPart(sourceValue->GetSymbolValue());
				check(headTargetPart);

				// Parcours des valeurs de la partie source
				sourceValue = sourceValueSet->GetHeadValue();
				while (sourceValue != NULL)
				{
					// Recherche de la partie cible associee a la valeur source
					targetPart = targetAttribute->LookupSymbolPart(sourceValue->GetSymbolValue());
					check(targetPart);

					// Erreur si la partie est differente de la premiere partie
					// Tolerance pour la valeur speciale
					if (targetPart != headTargetPart and
					    sourceValue->GetSymbolValue() != Symbol::GetStarValue())
					{
						sourcePart->AddError(sTmp + "Source value (" +
								     sourceValue->GetSymbolValue() +
								     ") belongs to a target group different from that "
								     "of the first source value");
						bOk = false;
						break;
					}

					// Valeur source suivante
					sourceValueSet->GetNextValue(sourceValue);
				}

				// Partie source suivante
				sourceAttribute->GetNextPart(sourcePart);
			}

			// Restitutiuon de l'etat initial
			if (not bIsTargetAttributeIndexed)
				targetAttribute->DeleteIndexingStructure();
		}

		// Arret si erreurs
		if (not bOk)
			break;
	}
	return bOk;
}

boolean KWDataGridManager::CheckCells(const KWDataGrid* sourceDataGrid, const KWDataGrid* targetDataGrid) const
{
	boolean bOk = true;
	boolean bDisplayResults = false;
	KWDataGridManager checkDataGridManager;
	KWDataGrid checkDataGrid;
	ObjectArray oaCheckParts;
	int nAttribute;
	int nTarget;
	KWDGAttribute* targetAttribute;
	KWDGAttribute* checkAttribute;
	KWDGPart* targetPart;
	KWDGPart* checkPart;
	KWDGCell* targetCell;
	KWDGCell* checkCell;
	Continuous cValue;
	ALString sTmp;

	require(Check());
	require(targetDataGrid != NULL);
	require(targetDataGrid->Check());
	require(CheckGranularity(sourceDataGrid, targetDataGrid));
	require(CheckTargetValues(sourceDataGrid, targetDataGrid));
	require(CheckAttributes(sourceDataGrid, targetDataGrid));
	require(CheckParts(sourceDataGrid, targetDataGrid));

	///////////////////////////////////////////////////////////////////////////////
	// On construit un nouveau DataGrid de verification, en partant des parties et
	// attributs du DataGrid cible et en y exportant les cellule du DataGrid source
	// On verifie alors que les cellules cible sont identique a celle de verification.
	// Note: cela n'a pas de sens si le test porte sur un DataGrid initialise
	// avec la methode ExportCells. Cela permet par contre de verifier la validite
	// d'un DataGrid construit autrement

	// Initialisation des attributs et partie du DataGrid de verification
	checkDataGridManager.ExportAttributes(targetDataGrid, &checkDataGrid);
	checkDataGridManager.ExportParts(targetDataGrid, &checkDataGrid);
	if (bDisplayResults and targetDataGrid->IsVarPartDataGrid())
	{
		cout << "Inner Attributes au Debut de CheckCells" << endl;
		cout << "Source" << endl;
		sourceDataGrid->WriteInnerAttributes(cout);
		cout << "Target" << endl;
		targetDataGrid->WriteInnerAttributes(cout);
		cout << "Check" << endl;
		checkDataGrid.WriteInnerAttributes(cout);
	}

	// Export des cellules sources vers le DataGrid de verification
	checkDataGridManager.ExportCells(sourceDataGrid, &checkDataGrid);
	assert(checkDataGrid.GetGridFrequency() == sourceDataGrid->GetGridFrequency());

	// Verification de l'effectif total
	if (checkDataGrid.GetGridFrequency() != targetDataGrid->GetGridFrequency())
	{
		targetDataGrid->AddError(sTmp + "The data grid frequency (" +
					 IntToString(targetDataGrid->GetGridFrequency()) + ") is not that expected (" +
					 IntToString(checkDataGrid.GetGridFrequency()) + ")");
		bOk = false;
	}
	// Verification du nombre de cellules
	else if (checkDataGrid.GetCellNumber() != targetDataGrid->GetCellNumber())
	{
		targetDataGrid->AddError(sTmp + "The data grid cell number (" +
					 IntToString(targetDataGrid->GetCellNumber()) + ") is not that expected (" +
					 IntToString(checkDataGrid.GetCellNumber()) + ")");
		bOk = false;
	}

	// Verification des cellules cibles
	if (bOk)
	{
		// Passage de la grille de verification en mode update
		checkDataGrid.SetCellUpdateMode(true);
		checkDataGrid.BuildIndexingStructure();
		oaCheckParts.SetSize(targetDataGrid->GetAttributeNumber());

		// Parcours des cellules cibles
		targetCell = targetDataGrid->GetHeadCell();
		while (targetCell != NULL)
		{
			// Recherche des parties cible pour les valeurs de la cellule courante
			for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
			{
				targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);
				checkAttribute = checkDataGrid.GetAttributeAt(nAttribute);
				assert(targetAttribute->GetAttributeName() == checkAttribute->GetAttributeName());

				// Recherche de la partie associee a la cellule selon son type
				targetPart = targetCell->GetPartAt(nAttribute);
				if (targetPart->GetPartType() == KWType::Continuous)
				{
					// Recherche d'une valeur typique: le milieu de l'intervalle (hors borne inf)
					cValue =
					    KWContinuous::GetUpperMeanValue(targetPart->GetInterval()->GetLowerBound(),
									    targetPart->GetInterval()->GetUpperBound());

					// Recherche de l'intervalle cible correspondant
					checkPart = checkAttribute->LookupContinuousPart(cValue);
					oaCheckParts.SetAt(nAttribute, checkPart);
				}
				else
				{
					assert(KWType::IsCoclusteringGroupableType(targetPart->GetPartType()));

					// Recherche du groupe de valeurs cible correspondant a la premiere valeur
					assert(targetPart->GetValueSet()->GetHeadValue() != NULL);
					checkPart = checkAttribute->LookupGroupablePart(
					    targetPart->GetValueSet()->GetHeadValue());
					oaCheckParts.SetAt(nAttribute, checkPart);
				}
			}

			// Recherche de la cellule correspondante dans le DataGrid de verification
			checkCell = checkDataGrid.LookupCell(&oaCheckParts);

			// Erreur si cellule non trouvee
			if (checkCell == NULL)
			{
				targetCell->AddError("Cell not found in the check data grid");
				bOk = false;
			}
			// Verification de la coherence de la cellule sinon
			else
			{
				// Verification de l'effectif de la cellule
				if (targetCell->GetCellFrequency() != checkCell->GetCellFrequency())
				{
					targetCell->AddError(sTmp + "Frequency (" +
							     IntToString(targetCell->GetCellFrequency()) +
							     ") inconsistent with the source frequency (" +
							     IntToString(checkCell->GetCellFrequency()) + ")");
					bOk = false;
				}

				// Verification de l'effectif de la cellule par classe cible
				for (nTarget = 0; nTarget < targetDataGrid->GetTargetValueNumber(); nTarget++)
				{
					if (targetCell->GetTargetFrequencyAt(nTarget) !=
					    checkCell->GetTargetFrequencyAt(nTarget))
					{
						targetCell->AddError(
						    sTmp + "Frequency of target value " +
						    targetDataGrid->GetTargetValueAt(nTarget) + " (" +
						    IntToString(targetCell->GetTargetFrequencyAt(nTarget)) +
						    ") inconsistent with the source frequency (" +
						    IntToString(checkCell->GetTargetFrequencyAt(nTarget)) + ")");
						bOk = false;
						break;
					}
				}
			}

			// Arret si erreurs
			if (not bOk)
				break;

			// Cellule target suivante
			targetDataGrid->GetNextCell(targetCell);
		}

		// Fin du mode update
		checkDataGrid.SetCellUpdateMode(false);
		checkDataGrid.DeleteIndexingStructure();
	}
	return true;
}

void KWDataGridManager::Test(const KWDataGrid* dataGrid)
{
	ObjectDictionary odQuantileBuilders;
	KWDataGridManager dataGridManager;
	KWDataGrid targetDataGrid1;
	KWDataGrid targetDataGrid2;
	IntVector ivMaxPartNumbers;
	int nTry;

	// Parametrage
	cout << "Source data grid" << endl;
	cout << *dataGrid << endl;

	// Export total (attribut, parties et cellules)
	targetDataGrid1.DeleteAll();
	targetDataGrid2.DeleteAll();
	dataGridManager.ExportDataGrid(dataGrid, &targetDataGrid1);
	cout << "Exported data grid" << endl;
	cout << targetDataGrid1 << endl;

	dataGridManager.InitializeQuantileBuilders(dataGrid, &odQuantileBuilders, &ivMaxPartNumbers);
	// Export avec granularisation (attribut, parties et cellules)
	int nGranularity;
	for (nGranularity = 2; nGranularity <= ceil(log(dataGrid->GetGridFrequency()) / log(2.0)); nGranularity++)
	{
		// Reinitialisation
		targetDataGrid1.DeleteAll();
		dataGridManager.ExportGranularizedDataGrid(dataGrid, &odQuantileBuilders, nGranularity,
							   &targetDataGrid1);
		cout << "Granularized data grid with granularity = " << IntToString(nGranularity) << endl;
		cout << targetDataGrid1 << endl;
		// on arrete le parcours si on a atteint la finesse de la grille initiale
		if (targetDataGrid1.GetLnGridSize() == dataGrid->GetLnGridSize())
			break;
	}

	// Export aleatoire
	for (nTry = 0; nTry < 10; nTry++)
	{
		// Reinitialisation
		targetDataGrid1.DeleteAll();
		targetDataGrid2.DeleteAll();

		// Export d'une grille aleatoire
		dataGridManager.ExportRandomAttributes(dataGrid, dataGrid->GetAttributeNumber(), &targetDataGrid1);
		dataGridManager.AddRandomParts(dataGrid, NULL, 3, &targetDataGrid1);
		dataGridManager.ExportCells(dataGrid, &targetDataGrid1);
		cout << "Random exported data grid" << endl;
		cout << targetDataGrid1 << endl;

		// Export d'une grille avec ajout aleatoire de nouvelles parties
		dataGridManager.AddRandomAttributes(
		    dataGrid, &targetDataGrid2, dataGrid->GetAttributeNumber() - targetDataGrid1.GetAttributeNumber(),
		    &targetDataGrid1);
		dataGridManager.AddRandomParts(dataGrid, &targetDataGrid1, 3, &targetDataGrid2);
		dataGridManager.ExportCells(dataGrid, &targetDataGrid2);
		cout << "Random modified exported data grid" << endl;
		cout << targetDataGrid2 << endl;
	}
}

void KWDataGridManager::InitialiseDataGrid(const KWDataGrid* sourceDataGrid, int nAttributeNumber,
					   KWDataGrid* targetDataGrid) const
{
	int nTarget;

	require(sourceDataGrid != NULL);
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(nAttributeNumber >= 0);

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation de la grille cible
	targetDataGrid->Initialize(nAttributeNumber, sourceDataGrid->GetTargetValueNumber());

	// Initialisation des valeurs cibles
	for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
	{
		targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
	}
}

void KWDataGridManager::InitialiseAttribute(const KWDGAttribute* sourceAttribute, KWDGAttribute* targetAttribute) const
{
	require(sourceAttribute != NULL);
	require(not sourceAttribute->GetAttributeTargetFunction() or
		sourceAttribute->GetGranularizedValueNumber() == sourceAttribute->GetInitialValueNumber());
	require(KWType::IsCoclusteringType(sourceAttribute->GetAttributeType()));
	require(targetAttribute != NULL);
	require(targetAttribute->GetAttributeType() == KWType::Unknown);
	require(targetAttribute->GetCatchAllValueSet() == NULL);

	// Initialisation des caracteristiques principale de l'attribut cible
	targetAttribute->SetAttributeName(sourceAttribute->GetAttributeName());
	targetAttribute->SetAttributeType(sourceAttribute->GetAttributeType());
	targetAttribute->SetAttributeTargetFunction(sourceAttribute->GetAttributeTargetFunction());
	targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());
	targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetGranularizedValueNumber());
	targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());
	targetAttribute->SetOwnerAttributeName(sourceAttribute->GetOwnerAttributeName());
	targetAttribute->SetCost(sourceAttribute->GetCost());

	// Partage des partitions des attributs internes de la grille source
	if (sourceAttribute->GetAttributeType() == KWType::VarPart)
	{
		targetAttribute->SetInnerAttributes(sourceAttribute->GetInnerAttributes());
	}
	ensure(targetAttribute->GetAttributeType() != KWType::VarPart or targetAttribute->GetInnerAttributes() != NULL);
}

void KWDataGridManager::InitialiseAttributeParts(const KWDGAttribute* sourceAttribute,
						 KWDGAttribute* targetAttribute) const
{
	KWDGPart* sourcePart;
	KWDGPart* targetPart;

	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(targetAttribute->GetPartNumber() == 0);

	// Recopie des parties de l'attribut source
	sourcePart = sourceAttribute->GetHeadPart();
	while (sourcePart != NULL)
	{
		// Creation de la partie cible
		targetPart = targetAttribute->AddPart();

		// Copie des valeurs de la partie, quel que soit son type
		targetPart->GetPartValues()->CopyFrom(sourcePart->GetPartValues());

		// Transfert du parametrage du groupe poubelle (methode tolerante au cas continu)
		if (sourcePart == sourceAttribute->GetGarbagePart())
			targetAttribute->SetGarbagePart(targetPart);

		// Mise a jour des effectifs dans le cas d'un innerAttribute
		// Pour les autre attributs, c'est calcule a partir des cellules
		if (sourceAttribute->IsInnerAttribute())
			targetPart->SetPartFrequency(targetPart->GetPartFrequency() + sourcePart->GetPartFrequency());

		// Partie suivante
		sourceAttribute->GetNextPart(sourcePart);
	}
	assert(targetAttribute->GetPartNumber() == sourceAttribute->GetPartNumber());
}

void KWDataGridManager::InitialiseVarPartAttributeClonedParts(const KWDGAttribute* sourceAttribute,
							      KWDGAttribute* targetAttribute) const
{
	KWDGInnerAttributes* clonedInnerAttributes;
	ObjectArray oaSourceInnerAttributeVarParts;
	ObjectArray oaTargetInnerAttributeVarParts;
	NumericKeyDictionary nkdTargetInnerAttributeVarParts;
	int n;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	KWDGPart* sourceVarPart;
	KWDGPart* targetVarPart;
	KWDGValue* sourceValue;

	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(targetAttribute->GetAttributeType() == KWType::VarPart);
	require(targetAttribute->GetPartNumber() == 0);

	// Creation d'un clone des attributs internes
	clonedInnerAttributes =
	    CloneInnerAttributes(sourceAttribute->GetInnerAttributes(), targetAttribute->GetDataGrid());

	// Parametrage des attributs internes de l'attribut cible de type VarPart
	targetAttribute->SetInnerAttributes(clonedInnerAttributes);

	// Memorisation de l'association entre VarPart source et cible via un dictionnaire
	sourceAttribute->GetInnerAttributes()->ExportAllInnerAttributeVarParts(&oaSourceInnerAttributeVarParts);
	targetAttribute->GetInnerAttributes()->ExportAllInnerAttributeVarParts(&oaTargetInnerAttributeVarParts);
	for (n = 0; n < oaSourceInnerAttributeVarParts.GetSize(); n++)
	{
		sourceVarPart = cast(KWDGPart*, oaSourceInnerAttributeVarParts.GetAt(n));
		targetVarPart = cast(KWDGPart*, oaTargetInnerAttributeVarParts.GetAt(n));
		nkdTargetInnerAttributeVarParts.SetAt(sourceVarPart, targetVarPart);
	}

	// Recopie des parties de l'attribut source, en utilisant les VarPartCibles
	sourcePart = sourceAttribute->GetHeadPart();
	while (sourcePart != NULL)
	{
		// Creation de la partie cible
		targetPart = targetAttribute->AddPart();

		// Transfert des parties de parties de variable avec de nouvelles parties de variable
		sourceValue = sourcePart->GetVarPartSet()->GetHeadValue();
		while (sourceValue != NULL)
		{
			sourceVarPart = sourceValue->GetVarPart();

			// Memorisation de la partie cible correspondante
			targetVarPart = cast(KWDGPart*, nkdTargetInnerAttributeVarParts.Lookup(sourceVarPart));
			targetPart->GetVarPartSet()->AddVarPart(targetVarPart);

			// Ajout de cette partie pour l'attribut interne
			sourcePart->GetVarPartSet()->GetNextValue(sourceValue);
		}

		// Transfert du parametrage du groupe poubelle
		if (sourcePart == sourceAttribute->GetGarbagePart())
			targetAttribute->SetGarbagePart(targetPart);

		// Partie suivante
		sourceAttribute->GetNextPart(sourcePart);
	}
	assert(targetAttribute->GetPartNumber() == sourceAttribute->GetPartNumber());
}

void KWDataGridManager::InitialiseVarPartAttributeWithNewSurtokenisedInnerAttributes(
    const KWDGAttribute* sourceVarPartAttribute, const KWDGInnerAttributes* newInnerAttributes,
    KWDGAttribute* targetVarPartAttribute) const
{
	boolean bDisplayResults = false;
	int nInnerAttribute;
	KWDGAttribute* sourceInnerAttribute;
	KWDGAttribute* targetInnerAttribute;
	ObjectArray* oaVarPart;
	NumericKeyDictionary nkdTargetInnerAttributeVarParts;
	int nTargetIndex;
	NumericKeyDictionary nkdTargetInnerAttributeValues;
	KWDGValue* value;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	KWDGPart* sourceVarPart;
	KWDGPart* targetVarPart;
	KWDGValue* sourceValue;

	require(CheckAttributesConsistency(sourceVarPartAttribute, targetVarPartAttribute));
	require(targetVarPartAttribute->GetAttributeType() == KWType::VarPart);
	require(targetVarPartAttribute->GetPartNumber() == 0);

	targetVarPartAttribute->SetInnerAttributes(newInnerAttributes);

	// Memorisation de l'association entre VarPart source et une a plusieurs VarParts dans les nouveaux innerAttributes
	for (nInnerAttribute = 0; nInnerAttribute < newInnerAttributes->GetInnerAttributeNumber(); nInnerAttribute++)
	{
		sourceInnerAttribute =
		    sourceVarPartAttribute->GetInnerAttributes()->GetInnerAttributeAt(nInnerAttribute);
		targetInnerAttribute = newInnerAttributes->GetInnerAttributeAt(nInnerAttribute);
		assert(sourceInnerAttribute->GetAttributeName() == targetInnerAttribute->GetAttributeName());
		assert(sourceInnerAttribute->GetPartNumber() <= targetInnerAttribute->GetPartNumber());

		// Cas Continuous
		if (sourceInnerAttribute->GetAttributeType() == KWType::Continuous)
		{
			// Parcours synchronise des parties de type intervalles, ordonnee par valeurs
			oaVarPart = NULL;
			sourceVarPart = sourceInnerAttribute->GetHeadPart();
			targetVarPart = targetInnerAttribute->GetHeadPart();
			while (targetVarPart != NULL)
			{
				if (oaVarPart == NULL or not targetVarPart->IsSubPart(sourceVarPart))
				{
					// Changement de partie source, sauf sur la premiere partie
					if (oaVarPart != NULL)
						sourceInnerAttribute->GetNextPart(sourceVarPart);

					// Creation du tableau des partie cibles associees a la partie sourve
					oaVarPart = new ObjectArray;
					nkdTargetInnerAttributeVarParts.SetAt(sourceVarPart, oaVarPart);
				}
				oaVarPart->Add(targetVarPart);

				// Partie suivante
				targetInnerAttribute->GetNextPart(targetVarPart);
			}
			assert(sourceVarPart == sourceInnerAttribute->GetTailPart());
		}
		// Cas Symbol
		else
		{
			assert(sourceInnerAttribute->GetAttributeType() == KWType::Symbol);

			// Indexation prealable des parties cibles par leur premiere valeur
			assert(nkdTargetInnerAttributeValues.GetCount() == 0);
			targetVarPart = targetInnerAttribute->GetHeadPart();
			while (targetVarPart != NULL)
			{
				// Chaque premiere valeur est associee a sa partie
				value = targetVarPart->GetSymbolValueSet()->GetHeadValue();
				nkdTargetInnerAttributeValues.SetAt(value->GetNumericKeyValue(), targetVarPart);

				// Partie suivante
				targetInnerAttribute->GetNextPart(targetVarPart);
			}

			// Parcours des parties source pour identifier leur composition en parties cibles
			sourceVarPart = sourceInnerAttribute->GetHeadPart();
			targetVarPart = targetInnerAttribute->GetHeadPart();
			while (sourceVarPart != NULL)
			{
				// Creation du tableau des partie cibles associees a la partie sourve
				oaVarPart = new ObjectArray;
				nkdTargetInnerAttributeVarParts.SetAt(sourceVarPart, oaVarPart);
				assert(nkdTargetInnerAttributeValues.GetCount() > 0);

				// Parcours des valeurs source pour identifier les parties cible (par leur premiere valeur)
				value = sourceVarPart->GetSymbolValueSet()->GetHeadValue();
				while (value != NULL)
				{
					// Recherche de la partie cible dont la premier valeur coincide
					targetVarPart =
					    cast(KWDGPart*,
						 nkdTargetInnerAttributeValues.Lookup(value->GetNumericKeyValue()));

					// Insertion dans le tableau, une et une seule fois
					if (targetVarPart != NULL)
					{
						assert(targetVarPart->IsSubPart(sourceVarPart));
						oaVarPart->Add(targetVarPart);
						nkdTargetInnerAttributeValues.RemoveKey(value->GetNumericKeyValue());
					}

					// Partie suivante
					sourceVarPart->GetSymbolValueSet()->GetNextValue(value);
				}

				// Partie suivante
				sourceInnerAttribute->GetNextPart(sourceVarPart);
			}
			assert(nkdTargetInnerAttributeValues.GetCount() == 0);
		}
	}

	// Recopie des parties de l'attribut source, en utilisant les VarPartCibles
	sourcePart = sourceVarPartAttribute->GetHeadPart();
	while (sourcePart != NULL)
	{
		// Creation de la partie cible
		targetPart = targetVarPartAttribute->AddPart();

		// Transfert des parties de parties de variable avec de nouvelles parties de variable
		sourceValue = sourcePart->GetVarPartSet()->GetHeadValue();
		while (sourceValue != NULL)
		{
			sourceVarPart = sourceValue->GetVarPart();

			// Memorisation de la partie cible correspondante
			oaVarPart = cast(ObjectArray*, nkdTargetInnerAttributeVarParts.Lookup(sourceVarPart));
			for (nTargetIndex = 0; nTargetIndex < oaVarPart->GetSize(); nTargetIndex++)
			{
				targetPart->GetVarPartSet()->AddVarPart(
				    cast(KWDGPart*, oaVarPart->GetAt(nTargetIndex)));
			}

			// Ajout de cette partie pour l'attribut interne
			sourcePart->GetVarPartSet()->GetNextValue(sourceValue);
		}

		if (targetPart->GetValueSet()->GetValueNumber() == 0)
			targetVarPartAttribute->DeletePart(targetPart);

		// Transfert du parametrage du groupe poubelle
		if (sourcePart == sourceVarPartAttribute->GetGarbagePart())
			targetVarPartAttribute->SetGarbagePart(targetPart);

		// Partie suivante
		sourceVarPartAttribute->GetNextPart(sourcePart);
	}

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "InitialiseVarPartAttribute with surtokenised Attribute" << endl;
		cout << "Attribut varPart source" << endl;
		sourceVarPartAttribute->Write(cout);
		cout << "Attribut varPart surtokenise" << endl;
		targetVarPartAttribute->Write(cout);
	}

	// Nettoyage
	nkdTargetInnerAttributeVarParts.DeleteAll();
	assert(targetVarPartAttribute->GetPartNumber() == sourceVarPartAttribute->GetPartNumber());
}

void KWDataGridManager::InitialiseVarPartAttributeWithMergedInnerAttributes(
    const KWDGAttribute* sourceVarPartAttribute, const KWDGInnerAttributes* mergedInnerAttributes,
    KWDGAttribute* targetVarPartAttribute) const
{
	boolean bDisplayResults = false;
	int nInnerAttribute;
	KWDGAttribute* antecedentInnerAttribute;
	KWDGAttribute* mergedInnerAttribute;
	NumericKeyDictionary nkdTargetInnerAttributeVarParts;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	KWDGPart* antecedentVarPart;
	KWDGPart* mergedVarPart;
	KWDGValue* sourceValue;
	NumericKeyDictionary nkdAddedMergedInnerAttributeVarParts;
	boolean bMergedVarPartFound;

	require(CheckAttributesConsistency(sourceVarPartAttribute, targetVarPartAttribute));
	require(targetVarPartAttribute->GetAttributeType() == KWType::VarPart);
	require(targetVarPartAttribute->GetPartNumber() == 0);

	targetVarPartAttribute->SetInnerAttributes(mergedInnerAttributes);

	// Memorisation de l'association entre VarPart de l'antecedentInnerAttribute et sa VarPart mergee dans le mergedInnerAttributes
	for (nInnerAttribute = 0; nInnerAttribute < mergedInnerAttributes->GetInnerAttributeNumber(); nInnerAttribute++)
	{
		antecedentInnerAttribute =
		    sourceVarPartAttribute->GetInnerAttributes()->GetInnerAttributeAt(nInnerAttribute);
		mergedInnerAttribute = mergedInnerAttributes->GetInnerAttributeAt(nInnerAttribute);
		assert(antecedentInnerAttribute->GetAttributeName() == mergedInnerAttribute->GetAttributeName());
		assert(mergedInnerAttribute->GetPartNumber() <= antecedentInnerAttribute->GetPartNumber());
		assert(antecedentInnerAttribute->ContainsSubParts(mergedInnerAttribute));

		// Cas Continuous
		if (antecedentInnerAttribute->GetAttributeType() == KWType::Continuous)
		{
			// Parcours synchronise des parties de type intervalles, ordonnee par valeurs
			antecedentVarPart = antecedentInnerAttribute->GetHeadPart();
			mergedVarPart = mergedInnerAttribute->GetHeadPart();
			while (antecedentVarPart != NULL)
			{
				bMergedVarPartFound = false;
				while (not bMergedVarPartFound)
				{
					if (antecedentVarPart->IsSubPart(mergedVarPart))
					{
						nkdTargetInnerAttributeVarParts.SetAt(antecedentVarPart, mergedVarPart);
						bMergedVarPartFound = true;
					}
					else
						mergedInnerAttribute->GetNextPart(mergedVarPart);
				}
				// Partie suivante
				antecedentInnerAttribute->GetNextPart(antecedentVarPart);
			}
		}
		// Cas Symbol
		else
		{
			assert(antecedentInnerAttribute->GetAttributeType() == KWType::Symbol);

			// Parcours des parties antecedentes pour identifier la partie cible mergee associee
			antecedentVarPart = antecedentInnerAttribute->GetHeadPart();

			while (antecedentVarPart != NULL)
			{
				mergedVarPart = mergedInnerAttribute->GetHeadPart();

				// Recherche a optimiser par creation d'un dictionnaire ?
				while (mergedVarPart != NULL)
				{
					if (antecedentVarPart->IsSubPart(mergedVarPart))
					{
						// Memorisation de l'association
						nkdTargetInnerAttributeVarParts.SetAt(antecedentVarPart, mergedVarPart);
						break;
					}
					else
						// Partie suivante
						mergedInnerAttribute->GetNextPart(mergedVarPart);
				}
				// Partie suivante
				antecedentInnerAttribute->GetNextPart(antecedentVarPart);
			}
		}
	}

	// Parcours des parties de l'attribut antecedent, en remplacant les VarPart antecedentes par VarPart merges
	sourcePart = sourceVarPartAttribute->GetHeadPart();
	while (sourcePart != NULL)
	{
		// Creation de la partie cible
		targetPart = targetVarPartAttribute->AddPart();

		sourceValue = sourcePart->GetVarPartSet()->GetHeadValue();
		while (sourceValue != NULL)
		{
			antecedentVarPart = sourceValue->GetVarPart();
			mergedVarPart = cast(KWDGPart*, nkdTargetInnerAttributeVarParts.Lookup(antecedentVarPart));

			if (nkdAddedMergedInnerAttributeVarParts.Lookup(mergedVarPart) == NULL)
			{
				targetPart->GetVarPartSet()->AddVarPart(mergedVarPart);
				// autre dictionnaire possible ?
				nkdAddedMergedInnerAttributeVarParts.SetAt(mergedVarPart, mergedVarPart);
			}
			sourcePart->GetVarPartSet()->GetNextValue(sourceValue);
		}
		if (targetPart->GetValueSet()->GetValueNumber() == 0)
			targetVarPartAttribute->DeletePart(targetPart);

		// Transfert du parametrage du groupe poubelle
		if (sourcePart == sourceVarPartAttribute->GetGarbagePart())
			targetVarPartAttribute->SetGarbagePart(targetPart);

		// Partie suivante
		sourceVarPartAttribute->GetNextPart(sourcePart);
	}

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "InitialiseVarPartAttribute with mergedInnerAttributes" << endl;
		cout << "Attribut varPart source" << endl;
		sourceVarPartAttribute->Write(cout);
		cout << "Attribut varPart avec PV mergees" << endl;
		targetVarPartAttribute->Write(cout);
	}

	// Nettoyage
	assert(nkdAddedMergedInnerAttributeVarParts.GetCount() ==
	       mergedInnerAttributes->ComputeTotalInnerAttributeVarParts());
}

void KWDataGridManager::InitialiseAttributeNullPart(const KWDGAttribute* sourceAttribute,
						    KWDGAttribute* targetAttribute) const
{
	KWDGPart* sourcePart;
	KWDGPart* targetPart;

	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(targetAttribute->GetPartNumber() == 0);

	// Creation d'un intervalle unique dans le cas continu
	if (sourceAttribute->GetAttributeType() == KWType::Continuous)
	{
		// Creation de l'intervalle
		targetPart = targetAttribute->AddPart();

		// Mise a jour de ses bornes
		targetPart->GetInterval()->SetLowerBound(KWDGInterval::GetMinLowerBound());
		targetPart->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());
	}
	// Creation d'une partie unique comportant toutes les valeurs, dans le cas symbolique
	else
	{
		assert(KWType::IsCoclusteringGroupableType(sourceAttribute->GetAttributeType()));

		// Creation de l'ensemble des valeurs cible
		targetPart = targetAttribute->AddPart();

		// Transfert des valeurs des parties de l'attribut source
		sourcePart = sourceAttribute->GetHeadPart();
		while (sourcePart != NULL)
		{
			// Concatenation dans la partie cible des valeurs source
			targetPart->GetValueSet()->UpgradeFrom(sourcePart->GetValueSet());

			// Partie suivante
			sourceAttribute->GetNextPart(sourcePart);
		}

		// Tri des valeus cible
		targetPart->GetValueSet()->SortValueByDecreasingFrequencies();
	}
}

void KWDataGridManager::ExportContinuousAttributeRandomParts(const KWDGAttribute* sourceAttribute,
							     const KWDGAttribute* mandatoryAttribute,
							     int nRequestedPartNumber,
							     int nMinimimEqualFrequencyPartNumber, boolean bForce,
							     KWDGAttribute* targetAttribute) const
{
	const boolean bTrace = false;
	const boolean bTraceParts = false;
	int nEqualFrequencyPartNumber;
	int nMinIntervalFrequency;
	KWQuantileIntervalBuilder quantileIntervalBuilder;
	int nTotalFrequency;
	int nTotalPartNumber;
	IntVector ivTargetIntervalsCumulatedFrequencies;
	IntVector ivMandatoryIntervalsCumulatedFrequencies;
	IntVector ivTmpIntervals;
	int nMandatoryPartNumber;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	int nBoundIndex;
	int nCumulatedFrequency;
	int nTarget;
	int nMandatory;
	int nTmp;

	require(Check());
	require(sourceAttribute->GetAttributeType() == KWType::Continuous);
	require(sourceAttribute->ArePartsSorted());
	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(mandatoryAttribute == NULL or CheckAttributesConsistency(sourceAttribute, mandatoryAttribute));
	require(mandatoryAttribute == NULL or sourceAttribute->ContainsSubParts(mandatoryAttribute));
	require(mandatoryAttribute == NULL or mandatoryAttribute->ArePartsSorted());
	require(targetAttribute->GetPartNumber() == 0);
	require(1 <= nRequestedPartNumber);
	require(mandatoryAttribute == NULL or nRequestedPartNumber >= mandatoryAttribute->GetPartNumber());
	require(nRequestedPartNumber <= sourceAttribute->GetPartNumber());
	require(nRequestedPartNumber <= nMinimimEqualFrequencyPartNumber);

	// Trace initiale
	if (bTrace)
	{
		cout << "AddContinuousAttributeRandomParts\t" << sourceAttribute->GetAttributeName() << "\t"
		     << nRequestedPartNumber << "\t" << nMinimimEqualFrequencyPartNumber << "\t"
		     << BooleanToString(bForce) << "\n";
		cout << "- Source\t" << sourceAttribute->GetPartNumber() << "\n";
		if (mandatoryAttribute != NULL)
			cout << "- Mandatory\t" << mandatoryAttribute->GetPartNumber() << "\n";
		cout << "- Total frequency\t" << sourceAttribute->ComputeTotalPartFrequency() << "\n";
	}

	// Cas particulier: l'attribut obligatoire contient le nombre de parties demandees
	if (mandatoryAttribute != NULL and mandatoryAttribute->GetPartNumber() == nRequestedPartNumber)
	{
		// Transfert du parametrage des parties de l'attribut obligatoire
		InitialiseAttributeParts(mandatoryAttribute, targetAttribute);
	}
	// Cas particulier: l'attribut source contient le nombre de parties demandees et
	// on doit obtenir le nombre de parties exact
	else if (sourceAttribute->GetPartNumber() == nRequestedPartNumber and bForce)
	{
		// Transfert du parametrage des parties de l'attribut source
		InitialiseAttributeParts(sourceAttribute, targetAttribute);
	}
	// Partition aleatoire des bornes des intervalles (en rangs) dans le cas continu
	else
	{
		// Initialisation du nombre de parties obligatoires
		nMandatoryPartNumber = 1;
		if (mandatoryAttribute != NULL)
			nMandatoryPartNumber = mandatoryAttribute->GetPartNumber();

		///////////////////////////////////////////////////////////////////////////////////
		// Utilisation d'un quantileBuilder pour calculer les quantiles de l'attribut source
		// Si bForce=false, on va obtenir une partition equilibre avec potentiellement moins
		// d'intervalles que demandes.
		// Si bForce=true, on va augmenter iterativement le nombre de quantiles a calculer
		// pour obtenir au moins le nombre d'intervalles demandes.

		// On exploite le quantileIntervalBuilder uniquement pour des effectifs moyen au moins de 2
		// Sinon, la partition source suffit
		nTotalFrequency = sourceAttribute->ComputeTotalPartFrequency();
		nMinIntervalFrequency = nTotalFrequency / nMinimimEqualFrequencyPartNumber;
		if (nMinIntervalFrequency >= 2)
		{
			// Initialisation d'un quantileBuilder pour calculer les quantiles de l'attribut source
			nTotalPartNumber = InitializeQuantileIntervalBuilder(sourceAttribute, &quantileIntervalBuilder);

			// Construction du nombre de quantile demandes
			quantileIntervalBuilder.ComputeQuantiles(nMinimimEqualFrequencyPartNumber);
			if (bTrace)
				cout << "  - InitialQuantiles\t" << nMinimimEqualFrequencyPartNumber << "\t"
				     << quantileIntervalBuilder.GetIntervalNumber() << endl;

			// Si bForce est active, on augmente le nombre de quantiles jusqu a obtenir
			// au moins nRequestedPartNumber
			// On saute les valeurs intermediaires qui ne changent pas l effectif moyen
			// par intervalle (N / k), pour limiter les recalculs, en evitant un complexite en O(N^2)
			// Cette strategie conserve une exploration pertinente des partitionnements
			// tout en gardant une complexite globale en O(N log N) = N * (1/1 + 1/2 + ... + 1/N)
			if (bForce)
			{
				nEqualFrequencyPartNumber = nMinimimEqualFrequencyPartNumber;
				while (quantileIntervalBuilder.GetIntervalNumber() < nRequestedPartNumber and
				       quantileIntervalBuilder.GetIntervalNumber() <
					   quantileIntervalBuilder.GetValueNumber())
				{
					// Recherche du prochain k pour lequel N / k change.
					while (nTotalFrequency / nEqualFrequencyPartNumber == nMinIntervalFrequency and
					       nEqualFrequencyPartNumber < nTotalFrequency)
						nEqualFrequencyPartNumber++;

					// Si l'effectif moyen minimal atteint 1, la partition source suffit.
					nMinIntervalFrequency = nTotalFrequency / nEqualFrequencyPartNumber;
					if (nMinIntervalFrequency == 1)
						break;

					// Recalcul des quantiles pour ce nouveau nombre de parties elementaires.
					quantileIntervalBuilder.ComputeQuantiles(nEqualFrequencyPartNumber);
					if (bTrace)
						cout << "  - ComputeQuantiles\t" << nEqualFrequencyPartNumber << "\t"
						     << quantileIntervalBuilder.GetIntervalNumber() << endl;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Extraction des bornes des intervalles cibles
		// Dans le cas de la presence d'un attribut obligatoire, les intervalles obligatoires
		// doivent faire partie des intervalles cibles

		// Extraction des effectifs cumules des intervalles cible si on a pu extraire suffisament d'intervalles
		if (quantileIntervalBuilder.IsComputed() and
		    (quantileIntervalBuilder.GetIntervalNumber() >= nRequestedPartNumber or not bForce))
		{
			assert(quantileIntervalBuilder.GetQuantileNumber() >= nMinimimEqualFrequencyPartNumber);

			// On utilise la quantile builder
			ivTargetIntervalsCumulatedFrequencies.SetSize(quantileIntervalBuilder.GetIntervalNumber());
			for (nTarget = 0; nTarget < ivTargetIntervalsCumulatedFrequencies.GetSize(); nTarget++)
			{
				// L'effectif cumule de l'intervalle est egal a l'index de la derniere instance, plus un
				ivTargetIntervalsCumulatedFrequencies.SetAt(
				    nTarget, quantileIntervalBuilder.GetIntervalLastInstanceIndexAt(nTarget) + 1);
			}
		}
		// Extraction des effectif cumules de l'attribut source sinon
		else
			ComputeContinuousAttributeCumulatedFrequencies(sourceAttribute,
								       &ivTargetIntervalsCumulatedFrequencies);
		assert(ivTargetIntervalsCumulatedFrequencies.GetSize() >= nRequestedPartNumber or not bForce);
		if (bTrace)
			cout << "  - Initial new intervals\t" << ivTargetIntervalsCumulatedFrequencies.GetSize()
			     << "\n";

		// Si l'attribut obligatoire est fourni et contient au moins deux intervalles,
		// on va forcer la creation d'intervalles cibles
		if (nMandatoryPartNumber > 1)
		{
			// Extraction des effectif cumules de l'attribut obligatoire
			ComputeContinuousAttributeCumulatedFrequencies(mandatoryAttribute,
								       &ivMandatoryIntervalsCumulatedFrequencies);

			// On supprime d'abord les intervalles obligatoires communs avec les intervalles cible
			// Cela ne concerne pas le dernier intervalle, qui est toujours conserve
			nMandatory = 0;
			nTarget = 0;
			for (nTmp = 0; nTmp < ivTargetIntervalsCumulatedFrequencies.GetSize(); nTmp++)
			{
				// On ignore les intervalles obligatoires precedent l'intervalle cible
				while (nMandatory < nMandatoryPartNumber - 1)
				{
					if (ivMandatoryIntervalsCumulatedFrequencies.GetAt(nMandatory) <
					    ivTargetIntervalsCumulatedFrequencies.GetAt(nTmp))
						nMandatory++;
					else
						break;
				}

				// Si l'intervalle cible est un intervalle obligatoire, on ne le conserve pas
				if (nMandatory < nMandatoryPartNumber - 1 and
				    ivMandatoryIntervalsCumulatedFrequencies.GetAt(nMandatory) ==
					ivTargetIntervalsCumulatedFrequencies.GetAt(nTmp))
					nMandatory++;
				// Sinon, on memorise l'intervalle cible
				else
				{
					ivTargetIntervalsCumulatedFrequencies.SetAt(
					    nTarget, ivTargetIntervalsCumulatedFrequencies.GetAt(nTmp));
					nTarget++;
				}
			}

			// Retaillage suite a la suppression des intervalle communs
			assert(ivTargetIntervalsCumulatedFrequencies.GetSize() - nTarget <= nMandatoryPartNumber - 1);
			if (bTrace)
				cout << "  - Common intervals removed\t"
				     << ivTargetIntervalsCumulatedFrequencies.GetSize() - nTarget << "\n";
			ivTargetIntervalsCumulatedFrequencies.SetSize(nTarget);
		}

		// Si on a trop de bornes (apres rajout des intervalles obligatoires), on en extrait une sous partie aleatoire
		if (ivTargetIntervalsCumulatedFrequencies.GetSize() + (nMandatoryPartNumber - 1) > nRequestedPartNumber)
		{
			// On supprime temporairement la borne du dernier intervalle, qui sera gardee de toute facon
			ivTargetIntervalsCumulatedFrequencies.SetSize(ivTargetIntervalsCumulatedFrequencies.GetSize() -
								      1);

			// Permutation aleatoire des bornes
			ivTargetIntervalsCumulatedFrequencies.Shuffle();

			// On tronque la liste des bornes, ce qui revient a en fait un choix aleatoire
			ivTargetIntervalsCumulatedFrequencies.SetSize(nRequestedPartNumber - 1 -
								      (nMandatoryPartNumber - 1));

			// On retrie les bornes
			ivTargetIntervalsCumulatedFrequencies.Sort();

			// On rajoute la borne du dernier intervalle
			ivTargetIntervalsCumulatedFrequencies.Add(nTotalFrequency);
			if (bTrace)
				cout << "  - New intervals\t" << ivTargetIntervalsCumulatedFrequencies.GetSize()
				     << "\n";
		}
		assert(ivTargetIntervalsCumulatedFrequencies.GetSize() + (nMandatoryPartNumber - 1) <=
		       nRequestedPartNumber);
		assert(ivTargetIntervalsCumulatedFrequencies.GetSize() + (nMandatoryPartNumber - 1) ==
			   nRequestedPartNumber or
		       not bForce);

		// Ajout si necessaire des bornes des intervalles obligatoires
		if (nMandatoryPartNumber > 1)
		{
			// Memorisation des intervalles cible a prendre en compte
			ivTmpIntervals.CopyFrom(&ivTargetIntervalsCumulatedFrequencies);

			// Dimensionnement du vecteur des intervalles cibles
			ivTargetIntervalsCumulatedFrequencies.SetSize(ivTmpIntervals.GetSize() +
								      (nMandatoryPartNumber - 1));

			// Creation des intervalles cible par union des intervalles obligatoires et des nouveaux intervalles
			nMandatory = 0;
			nTarget = 0;
			for (nTmp = 0; nTmp < ivTmpIntervals.GetSize(); nTmp++)
			{
				// On insere les intervalles obligatoires precedent l'intervalle cible
				while (nMandatory < nMandatoryPartNumber - 1)
				{
					if (ivMandatoryIntervalsCumulatedFrequencies.GetAt(nMandatory) <
					    ivTmpIntervals.GetAt(nTmp))
					{
						ivTargetIntervalsCumulatedFrequencies.SetAt(
						    nTarget,
						    ivMandatoryIntervalsCumulatedFrequencies.GetAt(nMandatory));
						nTarget++;
						nMandatory++;
					}
					else
						break;
				}
				assert(nMandatory >= nMandatoryPartNumber - 1 or
				       ivMandatoryIntervalsCumulatedFrequencies.GetAt(nMandatory) >
					   ivTmpIntervals.GetAt(nTmp));

				// Memorisation de l'intervalle sinon
				ivTargetIntervalsCumulatedFrequencies.SetAt(nTarget, ivTmpIntervals.GetAt(nTmp));
				nTarget++;
			}
			assert(nTarget == ivTargetIntervalsCumulatedFrequencies.GetSize());
		}
		assert(ivTargetIntervalsCumulatedFrequencies.GetSize() <= nRequestedPartNumber);
		assert(ivTargetIntervalsCumulatedFrequencies.GetSize() == nRequestedPartNumber or not bForce);

		////////////////////////////////////////////////////////////////////////////////
		// Creation de l'attribut cible a partir des bornes obtenues

		// Creation des intervalles cibles en utilisant les intervalles initiaux et
		// en utilisant les bornes specifiees pour les nouveaux intervalles
		targetPart = NULL;
		nBoundIndex = 0;
		nCumulatedFrequency = 0;
		sourcePart = sourceAttribute->GetHeadPart();
		while (sourcePart != NULL)
		{
			// Comptage du nombre d'instance sources traitees
			nCumulatedFrequency += sourcePart->GetPartFrequency();
			assert(nCumulatedFrequency > 0);

			// Creation si necessaire d'un intervalle cible
			if (targetPart == NULL)
			{
				targetPart = targetAttribute->AddPart();

				// Reinitialisation de ses bornes
				targetPart->GetInterval()->CopyFrom(sourcePart->GetInterval());
			}
			// Sinon, mise a jour de la borne sup de l'intervalle cible en cours
			else
			{
				targetPart->GetInterval()->SetUpperBound(sourcePart->GetInterval()->GetUpperBound());
			}

			// Mise a jour des effectifs dans le cas d'un innerAttribute
			// Pour les autre attributs, c'est calcule a partir des cellules
			if (sourceAttribute->IsInnerAttribute())
				targetPart->SetPartFrequency(targetPart->GetPartFrequency() +
							     sourcePart->GetPartFrequency());

			// L'intervalle cible est finalise si son effectif est atteint
			assert(nCumulatedFrequency <= ivTargetIntervalsCumulatedFrequencies.GetAt(nBoundIndex));
			if (nCumulatedFrequency == ivTargetIntervalsCumulatedFrequencies.GetAt(nBoundIndex))
			{
				// On reinitialise l'indicateur de creation d'intervalle cible
				targetPart = NULL;

				// On positionne la prochaine borne d'intervalle
				nBoundIndex++;
			}

			// Intervalle source suivant
			sourceAttribute->GetNextPart(sourcePart);
		}
		assert(nBoundIndex == ivTargetIntervalsCumulatedFrequencies.GetSize());
	}

	// Trace finale
	if (bTrace)
	{
		cout << "- Result\t" << targetAttribute->GetPartNumber() << "\n";
		if (bTraceParts)
		{
			cout << "Source intervals\n";
			sourceAttribute->WriteParts(cout);
			cout << "Mandatory intervals\n";
			if (mandatoryAttribute != NULL)
				mandatoryAttribute->WriteParts(cout);
			cout << "Target intervals\n";
			targetAttribute->WriteParts(cout);
		}
	}

	ensure(targetAttribute->GetPartNumber() <= nRequestedPartNumber);
	ensure(targetAttribute->GetPartNumber() == nRequestedPartNumber or not bForce);
	ensure(targetAttribute->ArePartsSorted());
	ensure(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	ensure(not sourceAttribute->IsInnerAttribute() or
	       targetAttribute->ComputeTotalPartFrequency() == sourceAttribute->ComputeTotalPartFrequency());
	ensure(sourceAttribute->ContainsSubParts(targetAttribute));
	ensure(mandatoryAttribute == NULL or targetAttribute->ContainsSubParts(mandatoryAttribute));
}

void KWDataGridManager::ExportGroupableAttributeRandomParts(const KWDGAttribute* sourceAttribute,
							    const KWDGAttribute* mandatoryAttribute,
							    int nRequestedPartNumber,
							    int nMinimimEqualFrequencyPartNumber, boolean bForce,
							    KWDGAttribute* targetAttribute) const
{
	const boolean bTrace = false;
	const boolean bTraceParts = false;
	ObjectArray oaGroupableAttributePartInformations;
	KWDGMGroupableAttributePartInformation* partInformation;
	KWDGMGroupableAttributePartInformation* nextPartInformation;
	KWDGMGroupableAttributePartInformation* unfrequentPartInformation;
	IntVector ivGroupSplitIndexes;
	ObjectArray oaTargetParts;
	IntVector ivUnfrequentPartIndexes;
	int nTargetPartNumber;
	int nTarget;
	int nSource;
	int nSplit;
	int n;
	int nMinTargetPartFrequency;
	int nMandatoryPartNumber;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	KWQuantileGroupBuilder quantileGroupBuilder;
	int nTotalPartNumber;

	require(Check());
	require(KWType::IsCoclusteringGroupableType(sourceAttribute->GetAttributeType()));
	require(not sourceAttribute->IsInnerAttribute() or sourceAttribute->ArePartsSorted());
	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(mandatoryAttribute == NULL or CheckAttributesConsistency(sourceAttribute, mandatoryAttribute));
	require(mandatoryAttribute == NULL or sourceAttribute->ContainsSubParts(mandatoryAttribute));
	require(mandatoryAttribute == NULL or not mandatoryAttribute->IsInnerAttribute() or
		mandatoryAttribute->ArePartsSorted());
	require(targetAttribute->GetPartNumber() == 0);
	require(1 <= nRequestedPartNumber);
	require(mandatoryAttribute == NULL or nRequestedPartNumber >= mandatoryAttribute->GetPartNumber());
	require(nRequestedPartNumber <= sourceAttribute->GetPartNumber());
	require(nRequestedPartNumber <= nMinimimEqualFrequencyPartNumber);

	// Trace initiale
	if (bTrace)
	{
		cout << "AddSymbolAttributeRandomParts\t" << sourceAttribute->GetAttributeName() << "\t"
		     << nRequestedPartNumber << "\t" << nMinimimEqualFrequencyPartNumber << "\t"
		     << BooleanToString(bForce) << "\n";
		cout << "- Source\t" << sourceAttribute->GetPartNumber() << "\n";
		if (mandatoryAttribute != NULL)
			cout << "- Mandatory\t" << mandatoryAttribute->GetPartNumber() << "\n";
		cout << "- Total frequency\t" << sourceAttribute->ComputeTotalPartFrequency() << "\n";
	}

	// Cas particulier: l'attribut obligatoire contient le nombre de parties demandees
	if (mandatoryAttribute != NULL and mandatoryAttribute->GetPartNumber() == nRequestedPartNumber)
	{
		// Transfert du parametrage des parties de l'attribut obligatoire
		InitialiseAttributeParts(mandatoryAttribute, targetAttribute);
	}
	// Cas particulier: l'attribut source contient le nombre de parties demandees et
	// on doit obtenir le nombre de parties exact
	else if (sourceAttribute->GetPartNumber() == nRequestedPartNumber and bForce)
	{
		// Transfert du parametrage des parties de l'attribut source
		InitialiseAttributeParts(sourceAttribute, targetAttribute);
	}
	// Partition aleatoire des bornes des intervalles (en rangs) dans le cas continu
	else
	{
		// Initialisation du nombre de parties obligatoires
		nMandatoryPartNumber = 1;
		if (mandatoryAttribute != NULL)
			nMandatoryPartNumber = mandatoryAttribute->GetPartNumber();

		///////////////////////////////////////////////////////////////////////////////////
		// Utilisation d'un quantileBuilder pour calculer les quantiles de l'attribut source
		// et en deduire l'effectif minimum par partie source a utiliser
		// Si bForce=false, on va obtenir une partition equilibre avec potentiellement moins
		// de groupes que demandes.
		// Si bForce=true, on va diminuer l'effectif minimum par partie source
		// pour obtenir au moins le nombre de groupes demandes

		// Initialisation d'un quantileBuilder pour calculer les quantiles de l'attribut source
		nTotalPartNumber = InitializeQuantileGroupBuilder(sourceAttribute, &quantileGroupBuilder);

		// Construction du nombre de quantile demandes
		quantileGroupBuilder.ComputeQuantiles(nMinimimEqualFrequencyPartNumber);
		if (bTrace)
			cout << "  - InitialQuantiles\t" << nMinimimEqualFrequencyPartNumber << "\t"
			     << quantileGroupBuilder.GetGroupNumber() << endl;

		// On determine l'effectif minimum correspondant avec celui de la premiere valeur du dernier groupe
		nSource = quantileGroupBuilder.GetGroupFirstValueIndexAt(quantileGroupBuilder.GetGroupNumber() - 1);
		nMinTargetPartFrequency = quantileGroupBuilder.GetValueFrequencyAt(nSource);
		if (bTrace)
			cout << "  - InitialMinTargetPartFrequency\t" << nMinTargetPartFrequency << endl;

		// Si bForce est active, on diminue l'effectif minimum pour augmenter le nombre de quantiles
		// jusqu a obtenir au moins nRequestedPartNumber
		if (bForce and quantileGroupBuilder.GetGroupNumber() < nRequestedPartNumber)
		{
			assert(nRequestedPartNumber <= quantileGroupBuilder.GetValueNumber());
			nMinTargetPartFrequency = quantileGroupBuilder.GetValueFrequencyAt(nRequestedPartNumber - 1);
			if (bTrace)
			{
				cout << "  - FinalQuantiles\t" << nMinimimEqualFrequencyPartNumber << "\t"
				     << nSource + 1 << endl;
				cout << "  - FinalMinTargetPartFrequency\t" << nMinTargetPartFrequency << endl;
			}
		}

		///////////////////////////////////////////////////////////////////////////////////
		// Extraction d'informations sur les parties sources et leur association avec les
		// parties obligatoire. Cela permettra de les trier par partie obligatoire, et
		// aleatoirement localement a chaque partie aleatoire, de facon a permettre
		// un sur-partitionnement aleatoire compatible avec les parties obligatoires et
		// avec les contraintes d'effectifs minimum par partie source

		// Extraction d'information sur la partie source et leur association avec l'attribut obligatoire
		InitializeGroupableAttributePartInformations(sourceAttribute, mandatoryAttribute,
							     &oaGroupableAttributePartInformations);

		// Ajout d'un index aleatoire permettant de perturber l'ordre des parties
		oaGroupableAttributePartInformations.Shuffle();
		for (nSource = 0; nSource < oaGroupableAttributePartInformations.GetSize(); nSource++)
		{
			partInformation = cast(KWDGMGroupableAttributePartInformation*,
					       oaGroupableAttributePartInformations.GetAt(nSource));
			partInformation->SetRandomIndex(nSource);
		}

		// Tri des parties sources selon la partie obligatoire, puis selon l'index aleatoire
		// En cas d'absence d'attribut obligatoire, c'est l'index aleatoire qui est utilise pour le tri
		oaGroupableAttributePartInformations.SetCompareFunction(
		    KWDGMGroupableAttributePartInformationCompareMandatoryAndRandomIndexes);
		oaGroupableAttributePartInformations.Sort();

		// Identification des points de partitionnement possibles dans les parties obligatoires:
		// - hors point de partionnement entre groupes obligatoires
		// - uniquement apres des parties sources d'effectif suffisant:
		//   toutes les parties d'effectif insuffisant d'une meme partie obligatoire seront gardees ensemble
		// On determinera la nouvelle partition en choisissant aleatoirement parmi ces points de partitionnement
		for (nSource = 0; nSource < oaGroupableAttributePartInformations.GetSize() - 1; nSource++)
		{
			// Acces aux informations sur la partie et la suivante
			partInformation = cast(KWDGMGroupableAttributePartInformation*,
					       oaGroupableAttributePartInformations.GetAt(nSource));
			nextPartInformation = cast(KWDGMGroupableAttributePartInformation*,
						   oaGroupableAttributePartInformations.GetAt(nSource + 1));
			assert(partInformation->GetMandatoryPartIndex() <=
			       nextPartInformation->GetMandatoryPartIndex());

			// On garde l'index de partitionnement si la partie source suivante est dans la partie obligatoire,
			// et si elle est d'effectif suffisant
			if (partInformation->GetMandatoryPartIndex() == nextPartInformation->GetMandatoryPartIndex() and
			    partInformation->GetSourcePartFrequency() >= nMinTargetPartFrequency)
				ivGroupSplitIndexes.Add(nSource);
		}
		assert(ivGroupSplitIndexes.GetSize() <= sourceAttribute->GetPartNumber() - 1);
		assert(mandatoryAttribute == NULL or
		       ivGroupSplitIndexes.GetSize() <=
			   sourceAttribute->GetPartNumber() - mandatoryAttribute->GetPartNumber());

		// Trace sur les split initiaux
		if (bTrace)
		{
			cout << "  - ivGroupSplitIndexes\t" << ivGroupSplitIndexes.GetSize() << "\t:";
			for (nSplit = 0; nSplit < ivGroupSplitIndexes.GetSize(); nSplit++)
				cout << " " << ivGroupSplitIndexes.GetAt(nSplit);
			cout << "\n";
		}

		// On choisit aleatoirement des points de partitionnement parmi les points de partitionnement possibles
		if (ivGroupSplitIndexes.GetSize() + nMandatoryPartNumber > nRequestedPartNumber)
		{
			// On garde aleatoirement les points de partitionnement a conserver
			ivGroupSplitIndexes.Shuffle();
			ivGroupSplitIndexes.SetSize(nRequestedPartNumber - nMandatoryPartNumber);

			// On retrie les points de partitionnement conserves
			ivGroupSplitIndexes.Sort();
		}

		// On cree les index de parties cibles a partir des points de partitionnement choisis
		nTargetPartNumber = 0;
		nSplit = 0;
		for (nSource = 0; nSource < oaGroupableAttributePartInformations.GetSize(); nSource++)
		{
			// Acces aux informations sur la partie et la suivante
			partInformation = cast(KWDGMGroupableAttributePartInformation*,
					       oaGroupableAttributePartInformations.GetAt(nSource));
			nextPartInformation = NULL;
			if (nSource < oaGroupableAttributePartInformations.GetSize() - 1)
				nextPartInformation = cast(KWDGMGroupableAttributePartInformation*,
							   oaGroupableAttributePartInformations.GetAt(nSource + 1));
			assert(nextPartInformation == NULL or partInformation->GetMandatoryPartIndex() <=
								  nextPartInformation->GetMandatoryPartIndex());

			// Memorisation de l'index de la partie cible le cas ou l'effectif minimim n'est pas atteint.
			// Toutes les parties d'effectif insuffisant d'une meme partie obligatoire sont gardees ensemble
			// dans la derniere oartie cible de la partie obligatoire en cours de traitement
			if (partInformation->GetSourcePartFrequency() < nMinTargetPartFrequency)
				ivUnfrequentPartIndexes.Add(nSource);
			// Memorisation de l'index de la partie cible le cas standard
			else
				partInformation->SetTargetPartIndex(nTargetPartNumber);

			// L'index de la partie cible doit etre croissant, et ne peut pas sauter plus d'une unite
			// Cas particulier: il y a des part d'effectif insuffisant non encore traitees
			assert(nSource == 0 or ivUnfrequentPartIndexes.GetSize() > 0 or
			       cast(KWDGMGroupableAttributePartInformation*,
				    oaGroupableAttributePartInformations.GetAt(nSource - 1))
				       ->GetTargetPartIndex() <= partInformation->GetTargetPartIndex());
			assert(nSource == 0 or ivUnfrequentPartIndexes.GetSize() > 0 or
			       cast(KWDGMGroupableAttributePartInformation*,
				    oaGroupableAttributePartInformations.GetAt(nSource - 1))
				       ->GetTargetPartIndex() >= partInformation->GetTargetPartIndex() - 1);

			// Nouveau groupe cible si l'on est sur un point de partitionnement
			if (nSplit < ivGroupSplitIndexes.GetSize() and nSource == ivGroupSplitIndexes.GetAt(nSplit))
			{
				assert(partInformation->GetSourcePartFrequency() >= nMinTargetPartFrequency);
				assert(nextPartInformation == NULL or partInformation->GetMandatoryPartIndex() ==
									  nextPartInformation->GetMandatoryPartIndex());
				nTargetPartNumber++;
				nSplit++;
			}
			// Nouveau groupe cible si on change de partie obligatoire ou si on est sur la derniere partie
			else if (nextPartInformation == NULL or partInformation->GetMandatoryPartIndex() !=
								    nextPartInformation->GetMandatoryPartIndex())
			{
				assert(nSplit >= ivGroupSplitIndexes.GetSize() or
				       nSource != ivGroupSplitIndexes.GetAt(nSplit));

				// On integre toute les parties d'effectif minimum insuffisant dans le groupe en cours
				for (n = 0; n < ivUnfrequentPartIndexes.GetSize(); n++)
				{
					unfrequentPartInformation = cast(KWDGMGroupableAttributePartInformation*,
									 oaGroupableAttributePartInformations.GetAt(
									     ivUnfrequentPartIndexes.GetAt(n)));
					unfrequentPartInformation->SetTargetPartIndex(nTargetPartNumber);
				}
				ivUnfrequentPartIndexes.SetSize(0);

				// Passage a un nouveau groupe
				nTargetPartNumber++;
			}
		}
		assert(nSplit == ivGroupSplitIndexes.GetSize());
		assert(nTargetPartNumber == nMandatoryPartNumber + ivGroupSplitIndexes.GetSize());
		assert(nTargetPartNumber <= nRequestedPartNumber);

		// Trace intermediaire
		if (bTraceParts)
		{
			for (nSource = 0; nSource < oaGroupableAttributePartInformations.GetSize(); nSource++)
			{
				partInformation = cast(KWDGMGroupableAttributePartInformation*,
						       oaGroupableAttributePartInformations.GetAt(nSource));
				if (nSource == 0)
					partInformation->WriteHeaderLineReport(cout);
				partInformation->WriteLineReport(cout);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Creation de l'attribut cible a partir des informations sur la parties sources

		// Creation des groupes cible vides
		oaTargetParts.SetSize(nTargetPartNumber);
		for (nTarget = 0; nTarget < nTargetPartNumber; nTarget++)
		{
			targetPart = targetAttribute->AddPart();
			oaTargetParts.SetAt(nTarget, targetPart);
		}

		// Tri des information de partie selon l'attribut source
		oaGroupableAttributePartInformations.SetCompareFunction(
		    KWDGMGroupableAttributePartInformationCompareSourceIndexes);
		oaGroupableAttributePartInformations.Sort();

		// Initialisation des groupes cible a partir des groupes sources
		assert(sourceAttribute->GetPartNumber() == oaGroupableAttributePartInformations.GetSize());
		sourcePart = sourceAttribute->GetHeadPart();
		nSource = 0;
		while (sourcePart != NULL)
		{
			// Acces a l'information de partition
			partInformation = cast(KWDGMGroupableAttributePartInformation*,
					       oaGroupableAttributePartInformations.GetAt(nSource));
			assert(partInformation->GetSourcePartIndex() == nSource);

			// Recherche de la partie cible
			nTarget = partInformation->GetTargetPartIndex();
			targetPart = cast(KWDGPart*, oaTargetParts.GetAt(nTarget));

			// Mise a jour des valeurs de la partie cible
			targetPart->GetValueSet()->UpgradeFrom(sourcePart->GetValueSet());

			// Mise a jour des effectifs dans le cas d'un attribut interne
			// Pour les autre attributs, c'est calcule a partir des cellules
			if (sourceAttribute->IsInnerAttribute())
				targetPart->SetPartFrequency(targetPart->GetPartFrequency() +
							     sourcePart->GetPartFrequency());

			// Groupe source suivant
			sourceAttribute->GetNextPart(sourcePart);
			nSource++;
		}

		// Tri des parties dans cas d'un attribut interne
		if (sourceAttribute->IsInnerAttribute())
			targetAttribute->SortParts();
	}

	// Nettoyage
	oaGroupableAttributePartInformations.DeleteAll();

	// Trace finale
	if (bTrace)
	{
		cout << "- Result\t" << targetAttribute->GetPartNumber() << "\n";
		if (bTraceParts)
		{
			cout << "Source groups\n";
			sourceAttribute->WriteParts(cout);
			cout << "Mandatory groups\n";
			if (mandatoryAttribute != NULL)
				mandatoryAttribute->WriteParts(cout);
			cout << "Target groups\n";
			targetAttribute->WriteParts(cout);
		}
	}

	ensure(targetAttribute->GetPartNumber() <= nRequestedPartNumber);
	ensure(targetAttribute->GetPartNumber() == nRequestedPartNumber or not bForce);
	ensure(not targetAttribute->IsInnerAttribute() or targetAttribute->ArePartsSorted());
	ensure(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	ensure(not sourceAttribute->IsInnerAttribute() or
	       targetAttribute->ComputeTotalPartFrequency() == sourceAttribute->ComputeTotalPartFrequency());
	ensure(sourceAttribute->ContainsSubParts(targetAttribute));
	ensure(mandatoryAttribute == NULL or targetAttribute->ContainsSubParts(mandatoryAttribute));
}

void KWDataGridManager::InitialiseAttributeGranularizedParts(const KWDGAttribute* sourceAttribute,
							     KWQuantileBuilder* quantileBuilder, int nGranularity,
							     KWDGAttribute* targetAttribute) const
{
	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(quantileBuilder != NULL);

	// Cas d'un attribut "cible" (regression, classif avec groupage) : pas de granularisation mais poubelle envisageable
	if (sourceAttribute->GetAttributeTargetFunction())
	{
		InitialiseAttributeParts(sourceAttribute, targetAttribute);
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetInitialValueNumber());
	}
	// Cas des attributs sources
	else
	{
		// Granularisation dans le cas continu
		if (sourceAttribute->GetAttributeType() == KWType::Continuous)
		{
			InitialiseAttributeGranularizedContinuousParts(
			    sourceAttribute, cast(KWQuantileIntervalBuilder*, quantileBuilder), nGranularity,
			    targetAttribute);
		}
		// Granularisation dans le cas d'un attribut groupable
		else
		{
			assert(KWType::IsCoclusteringGroupableType(sourceAttribute->GetAttributeType()));
			InitialiseAttributeGranularizedGroupableParts(sourceAttribute,
								      cast(KWQuantileGroupBuilder*, quantileBuilder),
								      nGranularity, targetAttribute);
		}
	}
}

void KWDataGridManager::InitialiseAttributeGranularizedContinuousParts(
    const KWDGAttribute* sourceAttribute, KWQuantileIntervalBuilder* quantileIntervalBuilder, int nGranularity,
    KWDGAttribute* targetAttribute) const
{
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	ObjectArray oaSourceParts;
	int nValueNumber;
	int nPartileIndex;
	int nPartileNumber;
	int nActualPartileNumber;
	double dPartileSize;
	boolean bDisplayResults = false;

	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(not targetAttribute->GetAttributeTargetFunction());
	require(quantileIntervalBuilder != NULL);

	// Nombre potentiel de partiles associes a cette granularite
	// Cas d'un attribut de grille Variable * variable
	if (not sourceAttribute->IsInnerAttribute())
		nValueNumber = sourceAttribute->GetDataGrid()->GetGridFrequency();
	// Sinon cas d'un innerAttribute
	else
		nValueNumber = sourceAttribute->ComputeTotalPartFrequency();
	nPartileNumber = (int)pow(2, nGranularity);
	if (nPartileNumber > nValueNumber)
		nPartileNumber = nValueNumber;

	// Cas ou la granularisation n'est pas appliquee : non prise en compte de la granularite ou granularite maximale
	if (nGranularity == 0 or nPartileNumber >= nValueNumber)
	{
		InitialiseAttributeParts(sourceAttribute, targetAttribute);
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetInitialValueNumber());
	}

	// Granularisation
	else
	{
		// Effectif theorique par partile
		dPartileSize = (double)nValueNumber / (double)nPartileNumber;

		if (bDisplayResults)
		{
			cout << "Attribut " << targetAttribute->GetAttributeName() << endl;
			cout << "nPartileNumber = " << nPartileNumber << " \t dPartileSize = " << dPartileSize << endl;
		}

		// Export des parties de l'attribut source
		sourceAttribute->ExportParts(&oaSourceParts);

		// Calcul des quantiles
		quantileIntervalBuilder->ComputeQuantiles(nPartileNumber);

		// Initialisation du nombre effectif de partiles (peut etre inferieur au nombre theorique du fait de
		// doublons)
		nActualPartileNumber = quantileIntervalBuilder->GetIntervalNumber();

		// Creation des partiles
		for (nPartileIndex = 0; nPartileIndex < nActualPartileNumber; nPartileIndex++)
		{
			targetPart = targetAttribute->AddPart();

			// Extraction du premier l'intervalle du partile pour la borne inf
			sourcePart = cast(
			    KWDGPart*,
			    oaSourceParts.GetAt(quantileIntervalBuilder->GetIntervalFirstValueIndexAt(nPartileIndex)));
			targetPart->GetInterval()->SetLowerBound(sourcePart->GetInterval()->GetLowerBound());

			// Extraction du dernier intervalle du partile pour la borne sup
			sourcePart = cast(
			    KWDGPart*,
			    oaSourceParts.GetAt(quantileIntervalBuilder->GetIntervalLastValueIndexAt(nPartileIndex)));
			targetPart->GetInterval()->SetUpperBound(sourcePart->GetInterval()->GetUpperBound());

			// Cas de la granularisation d'un attribut interne dans un attribut de grille de type VarPart
			if (sourceAttribute->IsInnerAttribute())
			{
				// Memorisation de l'effectif de la partie interne
				// L'effectif des parties des attributs de grille est lui calcule a partir des cellules
				targetPart->SetPartFrequency(
				    quantileIntervalBuilder->GetIntervalFrequencyAt(nPartileIndex));
			}
		}
	}

	// Initialisation du nombre de valeurs apres granularisation
	// Cas d'un attribut explicatif dans le cadre d'une analyse supervisee
	// Mise a jour du parametrage du nombre de partiles par le nombre effectif de partiles
	if (IsSupervisedSourceAttribute(targetAttribute))
		targetAttribute->SetGranularizedValueNumber(nPartileNumber);
	// Sinon, la granularisation n'est qu'un procede de construction d'une grille initiale
	else
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetInitialValueNumber());
}

void KWDataGridManager::InitialiseAttributeGranularizedGroupableParts(const KWDGAttribute* sourceAttribute,
								      KWQuantileGroupBuilder* quantileGroupBuilder,
								      int nGranularity,
								      KWDGAttribute* targetAttribute) const
{
	ObjectArray oaSourceParts;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	KWDGValueSet* cleanedValueSet;
	int nValueNumber;
	int nPartileNumber;
	int nActualPartileNumber;
	int nPartileIndex;
	int nSourceIndex;

	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(KWType::IsCoclusteringGroupableType(sourceAttribute->GetAttributeType()));
	require(not targetAttribute->GetAttributeTargetFunction());
	require(quantileGroupBuilder != NULL);

	// Nombre potentiel de partiles associes a cette granularite
	if (not sourceAttribute->IsInnerAttribute())
		nValueNumber = sourceAttribute->GetDataGrid()->GetGridFrequency();
	else
		nValueNumber = sourceAttribute->ComputeTotalPartFrequency();

	nPartileNumber = (int)pow(2, nGranularity);
	if (nPartileNumber > nValueNumber)
		nPartileNumber = nValueNumber;
	// Initialisation
	nActualPartileNumber = nPartileNumber;

	// Cas ou la granularisation n'est pas appliquee : non prise en compte de la granularite
	if (nGranularity == 0)
	{
		InitialiseAttributeParts(sourceAttribute, targetAttribute);
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetInitialValueNumber());
	}
	// Granularisation
	else
	{
		// Export des parties de l'attribut source
		sourceAttribute->ExportParts(&oaSourceParts);

		// Cas du nombre de partiles associe a la granularite maximale
		// Ajout d'une condition afin que cela ne s'applique pas aux attributs de type VarPart.
		// Sinon on a des VarPart initiaux avec des PV d'innerAttributes differents
		if (nPartileNumber == nValueNumber and sourceAttribute->GetAttributeType() != KWType::VarPart)
			// Seuillage de nPartileNumber au nombre de partiles associe a la granularite precedente
			// pour que la granularisation rassemble les eventuelles valeurs sources singletons dans le fourre-tout
			// Pour G tel que 2^G < N <= 2^(G+1) on aura 1 < N/2^G <= 2 c'est a dire un effectif minimal par
			// partile de 2 (donc pas de singleton apres granularisation)
			// Cas VarPart:
			//   Les parties de variable categorielles ne contiennent pas de singletons qui sont
			//   deja groupes dans un fourre-tout lors du pre-partitionnement. En revanche les parties de
			//   variable numeriques peuvent contenir des singletons : intervalles d'effectif 1
			nPartileNumber = (int)pow(2, nGranularity - 1);

		// Calcul des quantiles
		quantileGroupBuilder->ComputeQuantiles(nPartileNumber);

		// Initialisation du nombre effectif de partiles (peut etre inferieur au nombre theorique du fait de doublons)
		nActualPartileNumber = quantileGroupBuilder->GetGroupNumber();

		// Creation des partiles
		for (nPartileIndex = 0; nPartileIndex < nActualPartileNumber; nPartileIndex++)
		{
			targetPart = targetAttribute->AddPart();

			// Parcours des instances du partile
			for (nSourceIndex = quantileGroupBuilder->GetGroupFirstValueIndexAt(nPartileIndex);
			     nSourceIndex <= quantileGroupBuilder->GetGroupLastValueIndexAt(nPartileIndex);
			     nSourceIndex++)
			{
				// Extraction de la partie a ajouter dans le groupe
				sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSourceIndex));

				// Ajout de ses valeurs
				targetPart->GetValueSet()->UpgradeFrom(sourcePart->GetValueSet());
			}

			// Cas de la granularisation d'un attribut interne dans un attribut de grille de type VarPart
			if (sourceAttribute->IsInnerAttribute())
			{
				assert(sourceAttribute->GetAttributeType() == KWType::Symbol);

				// Memorisation de l'effectif de la partie interne
				// L'effectif des parties des attributs de grille est lui calcule a partir des cellules
				targetPart->SetPartFrequency(quantileGroupBuilder->GetGroupFrequencyAt(nPartileIndex));
			}

			// Compression et memorisation du fourre-tout si necessaire (mode supervise, attribut non cible)
			// La partie qui contient la StarValue est compressee uniquement si elle contient plus d'une
			// modalite (cas d'un vrai fourre-tout)
			// Ce cas est traite uniquement dans le cas Symbol
			if (sourceAttribute->GetAttributeType() == KWType::Symbol)
			{
				if (targetPart->GetValueSet()->IsDefaultPart() and
				    targetPart->GetValueSet()->GetValueNumber() > 1 and
				    IsSupervisedSourceAttribute(targetAttribute))
				{
					// Compression du fourre-tout et memorisation de ses valeurs
					cleanedValueSet = cast(KWDGSymbolValueSet*, targetPart->GetValueSet())
							      ->ConvertToCleanedValueSet();
					targetAttribute->InitializeCatchAllValueSet(cleanedValueSet);
					delete cleanedValueSet;
				}
			}
		}
	}

	// Cas d'un attribut explicatif dans le cadre d'une analyse supervisee
	// Mise a jour du parametrage du nombre de partiles par le nombre effectif de groupes distincts
	if (IsSupervisedSourceAttribute(targetAttribute))
		targetAttribute->SetGranularizedValueNumber(nActualPartileNumber);
	// Sinon, la granularisation n'est qu'un procede de construction d'une grille initiale
	else
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetInitialValueNumber());
}

boolean KWDataGridManager::IsSupervisedSourceAttribute(const KWDGAttribute* attribute) const
{
	boolean bIsSupervisedSourceAttribute;

	require(attribute != NULL);

	if (attribute->IsInnerAttribute())
		bIsSupervisedSourceAttribute = false;
	else
	{
		assert(attribute->GetDataGrid() != NULL);
		bIsSupervisedSourceAttribute = (attribute->GetDataGrid()->GetTargetValueNumber() > 0 or
						(attribute->GetDataGrid()->GetTargetAttribute() != NULL and
						 not attribute->GetAttributeTargetFunction()));
	}
	return bIsSupervisedSourceAttribute;
}

boolean KWDataGridManager::CheckAttributesConsistency(const KWDGAttribute* attribute1,
						      const KWDGAttribute* attribute2) const
{
	boolean bOk = true;

	require(attribute1 != NULL);
	require(attribute2 != NULL);
	require(attribute1 != attribute2);

	// Comaraison sur les caracteristiques principale des attributs
	bOk = bOk and attribute1->GetAttributeName() == attribute2->GetAttributeName();
	bOk = bOk and attribute1->GetAttributeType() == attribute2->GetAttributeType();
	bOk = bOk and attribute1->GetAttributeTargetFunction() == attribute2->GetAttributeTargetFunction();
	bOk = bOk and (attribute1->GetAttributeType() == KWType::VarPart or
		       attribute1->GetInitialValueNumber() == attribute2->GetInitialValueNumber());
	bOk = bOk and attribute1->GetOwnerAttributeName() == attribute2->GetOwnerAttributeName();
	bOk = bOk and attribute1->GetCost() == attribute2->GetCost();
	return bOk;
}

KWDGInnerAttributes* KWDataGridManager::CloneInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes,
							     const KWDataGrid* creatorDataGrid) const
{
	KWDGInnerAttributes* resultInnerAttributes;
	int nInnerAttribute;
	KWDGAttribute* sourceInnerAttribute;
	KWDGAttribute* targetInnerAttribute;

	require(sourceInnerAttributes != NULL);
	require(sourceInnerAttributes->Check());
	require(sourceInnerAttributes->AreInnerAttributePartsSorted());
	require(creatorDataGrid != NULL);

	// Partage des partitions de la grille source
	resultInnerAttributes = new KWDGInnerAttributes;

	// Parcours des attributs internes
	for (nInnerAttribute = 0; nInnerAttribute < sourceInnerAttributes->GetInnerAttributeNumber(); nInnerAttribute++)
	{
		// Extraction de l'attribut internes source
		sourceInnerAttribute = sourceInnerAttributes->GetInnerAttributeAt(nInnerAttribute);

		// Creation d'un attribut interne identique en exploitant le createur virtuelle de la grille cible en parametre
		targetInnerAttribute = creatorDataGrid->NewAttribute();

		// Parametrage
		InitialiseAttribute(sourceInnerAttribute, targetInnerAttribute);
		resultInnerAttributes->AddInnerAttribute(targetInnerAttribute);

		// Initialisation des parties de l'attribut
		InitialiseAttributeParts(sourceInnerAttribute, targetInnerAttribute);
	}
	ensure(resultInnerAttributes->Check());
	ensure(resultInnerAttributes->AreInnerAttributePartsSorted());
	ensure(resultInnerAttributes->ComputeTotalInnerAttributeFrequency() ==
	       sourceInnerAttributes->ComputeTotalInnerAttributeFrequency());
	ensure(sourceInnerAttributes->ContainsSubVarParts(resultInnerAttributes));
	return resultInnerAttributes;
}

KWDGInnerAttributes*
KWDataGridManager::CreateNullInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes) const
{
	KWDGInnerAttributes* resultInnerAttributes;
	int nInnerAttribute;
	KWDGAttribute* sourceInnerAttribute;
	KWDGAttribute* targetInnerAttribute;

	require(sourceInnerAttributes != NULL);
	require(sourceInnerAttributes->Check());
	require(sourceInnerAttributes->AreInnerAttributePartsSorted());

	// Partage des partitions de la grille source
	resultInnerAttributes = new KWDGInnerAttributes;

	// Parcours des attributs internes
	for (nInnerAttribute = 0; nInnerAttribute < sourceInnerAttributes->GetInnerAttributeNumber(); nInnerAttribute++)
	{
		// Extraction de l'attribut internes source
		sourceInnerAttribute = sourceInnerAttributes->GetInnerAttributeAt(nInnerAttribute);

		// Creation d'un attribut interne identique
		targetInnerAttribute = new KWDGAttribute;

		// Parametrage
		InitialiseAttribute(sourceInnerAttribute, targetInnerAttribute);
		resultInnerAttributes->AddInnerAttribute(targetInnerAttribute);

		// Initialisation d'une seule partie par attribut
		InitialiseAttributeNullPart(sourceInnerAttribute, targetInnerAttribute);

		// Memorisation de l'effectif de la partie, pour un attribut interne
		assert(targetInnerAttribute->GetPartNumber() == 1);
		targetInnerAttribute->GetHeadPart()->SetPartFrequency(
		    sourceInnerAttribute->ComputeTotalPartFrequency());
	}
	ensure(resultInnerAttributes->Check());
	ensure(resultInnerAttributes->AreInnerAttributePartsSorted());
	ensure(resultInnerAttributes->ComputeTotalInnerAttributeFrequency() ==
	       sourceInnerAttributes->ComputeTotalInnerAttributeFrequency());
	ensure(sourceInnerAttributes->ContainsSubVarParts(resultInnerAttributes));
	return resultInnerAttributes;
}

KWDGInnerAttributes* KWDataGridManager::CreateRandomInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes,
								    const KWDGInnerAttributes* mandatoryInnerAttributes,
								    int nTotalTargetTokenNumber) const
{
	boolean bTrace = false;
	boolean bTraceDetails = false;
	KWDGInnerAttributes* targetInnerAttributes;
	int nInnerAttribute;
	KWDGAttribute* sourceInnerAttribute;
	KWDGAttribute* mandatoryInnerAttribute;
	KWDGAttribute* targetInnerAttribute;
	IntVector ivInnerAttributesIndexes;
	int n;
	int nTotalSourcePartNumber;
	int nTotalRemainingPartNumberToCreate;
	int nAttributeFrequency;
	int nMinimimEqualFrequencyPartNumber;
	boolean bEqualFrequencyConstraint;
	IntVector ivInnerAttributeTargetTokenNumbers;
	int nMeanTokenNumberToCreate;
	int nTokenNumberToCreate;
	int nUnsaturatedInnerAttributeNumber;
	int nMaxPassNumber;
	int nPass;

	require(sourceInnerAttributes != NULL);
	require(sourceInnerAttributes->ContainsSubVarParts(mandatoryInnerAttributes));
	require(mandatoryInnerAttributes != NULL);
	require(0 < nTotalTargetTokenNumber);
	require(nTotalTargetTokenNumber <= sourceInnerAttributes->ComputeTotalInnerAttributeVarParts());
	require(nTotalTargetTokenNumber > mandatoryInnerAttributes->ComputeTotalInnerAttributeVarParts());

	// Trace initiale
	if (bTrace)
	{
		cout << "CreateRandomInnerAttributes\t" << nTotalTargetTokenNumber << "\n";
		cout << "- Inner attributes\t" << sourceInnerAttributes->GetInnerAttributeNumber() << "\n";
		cout << "- Source inner attribute parts\t"
		     << sourceInnerAttributes->ComputeTotalInnerAttributeVarParts() << "\n";
		cout << "- Mandatory inner attribute parts\t"
		     << mandatoryInnerAttributes->ComputeTotalInnerAttributeVarParts() << "\n";
	}

	// Creation du nouvel innerAttributes
	targetInnerAttributes = new KWDGInnerAttributes;

	// Creation des attributs internes en sortie
	for (nInnerAttribute = 0; nInnerAttribute < mandatoryInnerAttributes->GetInnerAttributeNumber();
	     nInnerAttribute++)
	{
		mandatoryInnerAttribute = mandatoryInnerAttributes->GetInnerAttributeAt(nInnerAttribute);

		// Creation d'un attribut interne cible
		targetInnerAttribute = new KWDGAttribute;
		InitialiseAttribute(mandatoryInnerAttribute, targetInnerAttribute);
		targetInnerAttributes->AddInnerAttribute(targetInnerAttribute);
	}

	// Permutation aleatoire des index des attributs internes
	ivInnerAttributesIndexes.SetSize(mandatoryInnerAttributes->GetInnerAttributeNumber());
	for (n = 0; n < ivInnerAttributesIndexes.GetSize(); n++)
		ivInnerAttributesIndexes.SetAt(n, n);
	ivInnerAttributesIndexes.Shuffle();

	// Initialisation des nombres de parties a creer avec celles obligatoires
	ivInnerAttributeTargetTokenNumbers.SetSize(mandatoryInnerAttributes->GetInnerAttributeNumber());
	for (n = 0; n < ivInnerAttributesIndexes.GetSize(); n++)
	{
		nInnerAttribute = ivInnerAttributesIndexes.GetAt(n);

		// On initialise avec le nombre de l'attribut obligatoire
		mandatoryInnerAttribute = mandatoryInnerAttributes->GetInnerAttributeAt(nInnerAttribute);
		ivInnerAttributeTargetTokenNumbers.SetAt(nInnerAttribute, mandatoryInnerAttribute->GetPartNumber());
	}

	// On detemine le nombre de partie a creer par attribut interne en les parcourant en ordre aleatoire
	// pour repartir de facon equilibree les tokens a ajouter
	// On fait plusieurs passes si necessaire s'il n'y a pas assez de valeur pour certains attributs
	// En pratique, on n'a jamais observe plus de 4 ou 5 passes. Mais un garde-fou n'est pas inutile
	nTotalSourcePartNumber = sourceInnerAttributes->ComputeTotalInnerAttributeVarParts();
	nTotalRemainingPartNumberToCreate =
	    nTotalTargetTokenNumber - mandatoryInnerAttributes->ComputeTotalInnerAttributeVarParts();
	nUnsaturatedInnerAttributeNumber = mandatoryInnerAttributes->GetInnerAttributeNumber();
	nMaxPassNumber = int(nTotalRemainingPartNumberToCreate * log(nTotalRemainingPartNumberToCreate + 1) / log(2));
	nPass = 0;
	while (nTotalRemainingPartNumberToCreate > 0 and nPass < nMaxPassNumber)
	{
		assert(nUnsaturatedInnerAttributeNumber > 0);
		nPass++;
		if (bTrace)
			cout << "  - Tokens to add\t" << nPass << "\t" << nTotalRemainingPartNumberToCreate << "\t"
			     << nUnsaturatedInnerAttributeNumber << "\n";

		// Nombre de totens moyen a cree par attribut (partie entiere inferieure
		nMeanTokenNumberToCreate = nTotalRemainingPartNumberToCreate / nUnsaturatedInnerAttributeNumber;

		// S'il y a moins de tokens a cree que d'attribut, seul les premiers attributs en creeront un
		// dans la derniere passe
		if (nMeanTokenNumberToCreate == 0)
			nMeanTokenNumberToCreate = 1;

		// Parcours des attributs internes en ordre aleatoire
		nUnsaturatedInnerAttributeNumber = 0;
		for (n = 0; n < ivInnerAttributesIndexes.GetSize(); n++)
		{
			nInnerAttribute = ivInnerAttributesIndexes.GetAt(n);

			// Acces aux attributs
			sourceInnerAttribute = sourceInnerAttributes->GetInnerAttributeAt(nInnerAttribute);

			// Prise en compte des tokens a creer
			nTokenNumberToCreate = nMeanTokenNumberToCreate;
			if (nTokenNumberToCreate > 0)
			{

				// On reduit au nombre de token qu'il est possible de creer
				if (ivInnerAttributeTargetTokenNumbers.GetAt(nInnerAttribute) + nTokenNumberToCreate >=
				    sourceInnerAttribute->GetPartNumber())
					nTokenNumberToCreate =
					    sourceInnerAttribute->GetPartNumber() -
					    ivInnerAttributeTargetTokenNumbers.GetAt(nInnerAttribute);
				// Sinon, on incremente le nombre d'attributs internes non satures en parties creees
				else
					nUnsaturatedInnerAttributeNumber++;

				// Mise a jour du nombre de token a creer
				ivInnerAttributeTargetTokenNumbers.UpgradeAt(nInnerAttribute, nTokenNumberToCreate);
				nTotalRemainingPartNumberToCreate -= nTokenNumberToCreate;
				assert(nTotalRemainingPartNumberToCreate >= 0);
				if (nTotalRemainingPartNumberToCreate == 0)
					break;
			}
		}
	}

	// Parametrage de la contrainte d'effectif egaux par partie, pour tous les attributs
	bEqualFrequencyConstraint = RandomDouble() <= 0.5;

	// Creation des parties de chaque attribut interne
	for (n = 0; n < ivInnerAttributesIndexes.GetSize(); n++)
	{
		nInnerAttribute = ivInnerAttributesIndexes.GetAt(n);

		// Acces aux attributs
		sourceInnerAttribute = sourceInnerAttributes->GetInnerAttributeAt(nInnerAttribute);
		mandatoryInnerAttribute = mandatoryInnerAttributes->GetInnerAttributeAt(nInnerAttribute);
		targetInnerAttribute = targetInnerAttributes->GetInnerAttributeAt(nInnerAttribute);

		// Parametrage du nombre de tokens a extraire et de a contrainte d'equilibre des effectifs par partie
		nTokenNumberToCreate = ivInnerAttributeTargetTokenNumbers.GetAt(nInnerAttribute);
		if (bEqualFrequencyConstraint)
			nMinimimEqualFrequencyPartNumber = nTokenNumberToCreate;
		else
		{
			// En cas sans contrainte, on choisit aleatoirement le niveau de contrainte
			nAttributeFrequency = sourceInnerAttribute->ComputeTotalPartFrequency();
			nMinimimEqualFrequencyPartNumber =
			    nTokenNumberToCreate + (int)(RandomDouble() * (nAttributeFrequency - nTokenNumberToCreate));
		}
		assert(nMinimimEqualFrequencyPartNumber >= nTokenNumberToCreate);

		// Export des partie selon le type de l'attribut
		if (mandatoryInnerAttribute->GetAttributeType() == KWType::Continuous)
			ExportContinuousAttributeRandomParts(sourceInnerAttribute, mandatoryInnerAttribute,
							     nTokenNumberToCreate, nMinimimEqualFrequencyPartNumber,
							     true, targetInnerAttribute);
		else
			ExportGroupableAttributeRandomParts(sourceInnerAttribute, mandatoryInnerAttribute,
							    nTokenNumberToCreate, nMinimimEqualFrequencyPartNumber,
							    true, targetInnerAttribute);
		assert(targetInnerAttribute->GetPartNumber() <= nTokenNumberToCreate);
	}

	// Trace finale
	if (bTrace)
	{
		cout << "- Target inner attributes parts\t"
		     << targetInnerAttributes->ComputeTotalInnerAttributeVarParts() << "\n";
		if (bTraceDetails)
		{
			cout << "Source inner attributes:\n";
			sourceInnerAttributes->Write(cout);
			cout << "Mandatory inner attributes:\n";
			mandatoryInnerAttributes->Write(cout);
			cout << "Target inner attributes:\n";
			targetInnerAttributes->Write(cout);
		}
	}

	return targetInnerAttributes;
}

KWDGInnerAttributes*
KWDataGridManager::CreatePartitionnedInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes,
						     const ObjectDictionary* odInnerAttributePartitions) const
{
	boolean bTrace = true;
	boolean bTraceDetails = false;
	KWDGInnerAttributes* targetInnerAttributes;
	int nInnerAttribute;
	KWDGAttribute* sourceInnerAttribute;
	KWDGAttribute* targetInnerAttribute;
	const KWDGSAttributeDiscretization* attributeDiscretization;
	const KWDGSAttributeGrouping* attributeGrouping;
	const KWDGSAttributePartition* attributePartition;
	int nPartitionNumber;

	require(sourceInnerAttributes != NULL);
	require(odInnerAttributePartitions != NULL);
	require(odInnerAttributePartitions->GetCount() > 0);
	require(odInnerAttributePartitions->GetCount() <= sourceInnerAttributes->GetInnerAttributeNumber());

	// Trace initiale
	if (bTrace)
	{
		cout << "CreatePartitionnedInnerAttributes\n";
		cout << "- Inner attributes\t" << sourceInnerAttributes->GetInnerAttributeNumber() << "\n";
		cout << "- Partitions\t" << odInnerAttributePartitions->GetCount() << "\n";
	}

	// Creation du nouvel innerAttributes
	targetInnerAttributes = new KWDGInnerAttributes;

	// Creation des attributs internes en sortie
	nPartitionNumber = 0;
	for (nInnerAttribute = 0; nInnerAttribute < sourceInnerAttributes->GetInnerAttributeNumber(); nInnerAttribute++)
	{
		sourceInnerAttribute = sourceInnerAttributes->GetInnerAttributeAt(nInnerAttribute);

		// Creation d'un attribut interne cible
		targetInnerAttribute = new KWDGAttribute;
		InitialiseAttribute(sourceInnerAttribute, targetInnerAttribute);
		targetInnerAttributes->AddInnerAttribute(targetInnerAttribute);

		// Recherche de la partition associee a l'attribut
		attributePartition = cast(const KWDGSAttributePartition*,
					  odInnerAttributePartitions->Lookup(sourceInnerAttribute->GetAttributeName()));

		// Initialisation d'une seule partie par attribut si pas de partition
		if (attributePartition == NULL)
			InitialiseAttributeNullPart(sourceInnerAttribute, targetInnerAttribute);
		// Creation des parties de l'attribut interne cible selon la partition
		else
		{
			nPartitionNumber++;

			// Cas d'un attribut Continuous
			if (sourceInnerAttribute->GetAttributeType() == KWType::Continuous)
			{
				attributeDiscretization = cast(const KWDGSAttributeDiscretization*, attributePartition);
			}
			// Cas d'un atrtribut Symbol
			else
			{
				assert(sourceInnerAttribute->GetAttributeType() == KWType::Symbol);
				attributeGrouping = cast(const KWDGSAttributeGrouping*, attributePartition);
			}

			//DDD
			InitialiseAttributeNullPart(sourceInnerAttribute, targetInnerAttribute);
		}
	}
	assert(nPartitionNumber == odInnerAttributePartitions->GetCount());

	// Trace finale
	if (bTrace)
	{
		cout << "- Target inner attributes parts\t"
		     << targetInnerAttributes->ComputeTotalInnerAttributeVarParts() << "\n";
		if (bTraceDetails)
		{
			cout << "Source inner attributes:\n";
			sourceInnerAttributes->Write(cout);
			cout << "Target inner attributes:\n";
			targetInnerAttributes->Write(cout);
		}
	}
	return targetInnerAttributes;
}

double KWDataGridManager::MergePartsForVarPartAttributes(const KWDataGrid* sourceDataGrid,
							 KWDataGrid* targetDataGrid) const
{
	KWDGAttribute* sourceVarPartAttribute;
	KWDGAttribute* targetVarPartAttribute;
	KWDGAttribute* innerAttribute;
	KWDGPart* initialPart;
	KWDGPart* initialSourcePart;
	KWDGValue* currentValue;
	KWDGValue* nextValue;
	boolean bNewVarPart;
	double dDeltaClusterCost;

	require(sourceDataGrid->IsVarPartDataGrid());
	require(targetDataGrid->IsVarPartDataGrid());
	require(targetDataGrid->GetVarPartAttribute()->GetAttributeName() ==
		sourceDataGrid->GetVarPartAttribute()->GetAttributeName());

	// Initialisation
	dDeltaClusterCost = 0;

	// Recherche des attributs VarPart source et cible
	sourceVarPartAttribute = sourceDataGrid->GetVarPartAttribute();
	targetVarPartAttribute = targetDataGrid->GetVarPartAttribute();

	// Extraction du cluster de parties de variables avant fusion
	initialPart = targetVarPartAttribute->GetHeadPart();

	// Parcours synchronise des parties de chaque attribut
	initialSourcePart = sourceVarPartAttribute->GetHeadPart();
	while (initialPart != NULL)
	{
		// Tri des parties de variable du cluster
		// Attention, les VarPart sont ici trie d'abord par attribut, puis par valeurs de la partie,
		// de facon a pouvoir detecter la fusion de deux parties consecutives issues du meme attribut
		initialPart->GetVarPartSet()->SortValues();

		// Initialisation des deux premieres parties de variable
		currentValue = initialPart->GetVarPartSet()->GetHeadValue();
		nextValue = currentValue;
		initialPart->GetVarPartSet()->GetNextValue(nextValue);

		// Deplacement avec une partie et la suivante
		while (nextValue != NULL)
		{
			bNewVarPart = false;
			assert(currentValue->GetVarPart()->GetAttribute()->GetAttributeName() <=
			       nextValue->GetVarPart()->GetAttribute()->GetAttributeName());

			// Cas de non fusion
			// Parties d'attributs distincts
			if (currentValue->GetVarPart()->GetAttribute()->GetAttributeName() !=
			    nextValue->GetVarPart()->GetAttribute()->GetAttributeName())
				bNewVarPart = true;
			// Parties (intervalles) non consecutives d'un attribut numerique
			else if (currentValue->GetVarPart()->GetPartType() == KWType::Continuous)
			{
				assert(currentValue->GetVarPart()->GetInterval()->GetUpperBound() <=
				       nextValue->GetVarPart()->GetInterval()->GetLowerBound());
				if (currentValue->GetVarPart()->GetInterval()->GetUpperBound() <
				    nextValue->GetVarPart()->GetInterval()->GetLowerBound())
					bNewVarPart = true;
			}

			// Pas de fusion a realiser
			if (bNewVarPart)
			{
				// Parties de variable suivantes
				initialPart->GetVarPartSet()->GetNextValue(currentValue);
				initialPart->GetVarPartSet()->GetNextValue(nextValue);
			}
			// Cas de fusion
			else
			{
				targetVarPartAttribute->SetInitialValueNumber(
				    targetVarPartAttribute->GetInitialValueNumber() - 1);

				// Transfert des valeurs de la partie suivante
				currentValue->GetVarPart()->GetPartValues()->Import(
				    nextValue->GetVarPart()->GetPartValues());

				// Cumul des effectifs
				currentValue->GetVarPart()->SetPartFrequency(
				    currentValue->GetVarPart()->GetPartFrequency() +
				    nextValue->GetVarPart()->GetPartFrequency());

				// Suppression de la partie de variable de l'attribut interne
				innerAttribute = targetDataGrid->GetInnerAttributes()->LookupInnerAttribute(
				    currentValue->GetVarPart()->GetAttribute()->GetAttributeName());
				innerAttribute->DeletePart(nextValue->GetVarPart());
				if (innerAttribute->GetPartNumber() > innerAttribute->GetGranularizedValueNumber())
					innerAttribute->SetGranularizedValueNumber(
					    innerAttribute->GetGranularizedValueNumber() - 1);

				// Evaluation de la variation de cout du cluster du fait de la diminution du nombre de
				// parties
				dDeltaClusterCost += -log(initialSourcePart->GetPartFrequency() +
							  initialPart->GetVarPartSet()->GetValueNumber() - 1) +
						     log(initialPart->GetVarPartSet()->GetValueNumber() - 1);

				// Suppression de la partie de variable du cluster
				initialPart->GetVarPartSet()->DeleteValue(nextValue);

				// Parties de variable suivantes
				nextValue = currentValue;
				initialPart->GetVarPartSet()->GetNextValue(nextValue);
			}
		}
		targetVarPartAttribute->GetNextPart(initialPart);
		sourceVarPartAttribute->GetNextPart(initialSourcePart);
	}
	return dDeltaClusterCost;
}

int KWDataGridManager::InitializeQuantileIntervalBuilder(const KWDGAttribute* attribute,
							 KWQuantileIntervalBuilder* quantileIntervalBuilder) const
{
	int nMaxPartNumber;
	KWDGPart* sourcePart;
	IntVector ivFrequencies;

	require(attribute != NULL);
	require(KWType::IsCoclusteringType(attribute->GetAttributeType()));
	require(attribute->GetAttributeType() == KWType::Continuous);
	require(attribute->GetPartNumber() > 0);
	require(attribute->ArePartsSorted());
	require(quantileIntervalBuilder != NULL);

	// Creation du vecteur des frequences par parties
	sourcePart = attribute->GetHeadPart();
	while (sourcePart != NULL)
	{
		// Comptage du nombre d'instance sources traitees
		ivFrequencies.Add(sourcePart->GetPartFrequency());

		// Partie suivante
		attribute->GetNextPart(sourcePart);
	}

	// Initialisation du quantileBuilder
	quantileIntervalBuilder->InitializeFrequencies(&ivFrequencies);

	// Retour du nombre maximal de parties
	nMaxPartNumber = attribute->GetPartNumber();
	ensure(nMaxPartNumber >= 1);
	return nMaxPartNumber;
}

int KWDataGridManager::InitializeQuantileGroupBuilder(const KWDGAttribute* attribute,
						      KWQuantileGroupBuilder* quantileGroupBuilder) const
{
	int nMaxPartNumber;
	KWDGPart* sourcePart;
	IntVector ivFrequencies;
	boolean bSingleton;
	int nSize;
	int i;
	int nTmp;

	require(attribute != NULL);
	require(KWType::IsCoclusteringType(attribute->GetAttributeType()));
	require(KWType::IsCoclusteringGroupableType(attribute->GetAttributeType()));
	require(attribute->GetPartNumber() > 0);
	require(not attribute->IsInnerAttribute() or attribute->ArePartsSorted());
	require(quantileGroupBuilder != NULL);

	// Creation du vecteur des frequences par parties
	bSingleton = false;
	nMaxPartNumber = 0;
	sourcePart = attribute->GetHeadPart();
	while (sourcePart != NULL)
	{
		// Comptage du nombre d'instance sources traitees
		ivFrequencies.Add(sourcePart->GetPartFrequency());

		// Cas d'une partie non singleton
		if (sourcePart->GetPartFrequency() > 1)
			nMaxPartNumber++;
		else
			bSingleton = true;

		// Partie suivante
		attribute->GetNextPart(sourcePart);
	}
	// Ajout d'une partie regroupant les eventuels singletons
	if (bSingleton)
		nMaxPartNumber++;

	// Tri si necessaire des effectifs
	if (not attribute->IsInnerAttribute())
	{
		// Tri par ordre croissant
		ivFrequencies.Sort();

		// Inversion de l'ordre du tri
		nSize = ivFrequencies.GetSize();
		for (i = 0; i < nSize / 2; i++)
		{
			nTmp = ivFrequencies.GetAt(nSize - 1 - i);
			ivFrequencies.SetAt(nSize - 1 - i, ivFrequencies.GetAt(i));
			ivFrequencies.SetAt(i, nTmp);
		}
	}

	// Initialisation du quantileBuilder
	quantileGroupBuilder->InitializeFrequencies(&ivFrequencies);

	// Retour du nombre maximal de parties
	ensure(nMaxPartNumber >= 1);
	return nMaxPartNumber;
}

void KWDataGridManager::ExportAttributeSymbolValueFrequencies(const KWDGAttribute* sourceAttribute,
							      KWDGAttribute* targetAttribute) const
{
	int nInstanceNumber;
	KWDGPart* part;
	KWDGValueSet* valueSet;
	KWDGValue* value;
	KWDGValue* defaultValue;
	NumericKeyDictionary nkdSourceValues;
	KWDGValue* sourceValue;
	Symbol sValue;
	int nTotalValueFrequency;

	require(targetAttribute != NULL);
	require(targetAttribute->GetAttributeType() == KWType::Symbol);
	require(sourceAttribute->GetAttributeName() == targetAttribute->GetAttributeName());
	require(not sourceAttribute->IsInnerAttribute());

	// Nombre d'instances
	nInstanceNumber = sourceAttribute->GetDataGrid()->GetGridFrequency();

	// Collecte des valeurs de l'attribut source pour avoir acces a leur effectif
	part = sourceAttribute->GetHeadPart();
	while (part != NULL)
	{
		// Parcours des valeurs de la partie
		valueSet = part->GetValueSet();
		value = valueSet->GetHeadValue();
		while (value != NULL)
		{
			nkdSourceValues.SetAt(value->GetNumericKeyValue(), value);
			valueSet->GetNextValue(value);
		}

		// Partie suivante
		sourceAttribute->GetNextPart(part);
	}

	// Parcours des valeurs de l'attribut cible pour specifier leur effectif
	nTotalValueFrequency = 0;
	defaultValue = NULL;
	part = targetAttribute->GetHeadPart();
	while (part != NULL)
	{
		// Parcours des valeurs de la partie
		valueSet = part->GetValueSet();
		value = valueSet->GetHeadValue();
		while (value != NULL)
		{
			// Recherche de son effectif, precedement collecte a partir de l'attribut source
			sourceValue = cast(KWDGValue*, nkdSourceValues.Lookup(value->GetNumericKeyValue()));

			// Cas ou la sourceValue est bien presente
			// Dans le cas particulier ou sourceAttribute provient d'une grille construite a partir d'un
			// KWAtttributeStats, certaines modalites peuvent etre manquantes. En effet lors de la
			// construction de la grille de preparation (methode BuildPreparedGroupingDataGridStats), les
			// modalites du fourre-tout sont resumees par une modalite + StarValue
			if (sourceValue != NULL)
			{
				value->SetValueFrequency(sourceValue->GetValueFrequency());

				// On test si on est sur la valeur par defaut
				if (value->GetSymbolValue() == Symbol::GetStarValue())
					defaultValue = value;
				// Sinon cumul de l'effectif hors fourre-tout
				else
					nTotalValueFrequency += sourceValue->GetValueFrequency();
			}

			// Valeur suivante
			valueSet->GetNextValue(value);
		}

		// Partie suivante
		targetAttribute->GetNextPart(part);
	}

	// Alimentation de l'effectif de la valeur par defaut
	check(defaultValue);
	assert(defaultValue->GetValueFrequency() == nInstanceNumber - nTotalValueFrequency);
	defaultValue->SetValueFrequency(nInstanceNumber - nTotalValueFrequency);
}

void KWDataGridManager::InitializeGroupableAttributePartInformations(
    const KWDGAttribute* sourceAttribute, const KWDGAttribute* mandatoryAttribute,
    ObjectArray* oaGroupableAttributePartInformation) const
{
	boolean bIsIndexed;
	KWDGMGroupableAttributePartInformation* partInformation;
	LongintNumericKeyDictionary lnkdMandatoryParts;
	KWDGPart* sourcePart;
	KWDGPart* mandatoryPart;
	int nSourceIndex;
	int nMandatoryIndex;

	require(sourceAttribute != NULL);
	require(sourceAttribute->Check());
	require(mandatoryAttribute == NULL or mandatoryAttribute->Check());
	require(KWType::IsCoclusteringGroupableType(sourceAttribute->GetAttributeType()));
	require(mandatoryAttribute == NULL or
		sourceAttribute->GetAttributeType() == mandatoryAttribute->GetAttributeType());
	require(mandatoryAttribute == NULL or sourceAttribute->ContainsSubParts(mandatoryAttribute));
	require(oaGroupableAttributePartInformation != NULL);
	require(oaGroupableAttributePartInformation->GetSize() == 0);

	// Initialisation concernant l'attribut obligatoire
	bIsIndexed = false;
	if (mandatoryAttribute != NULL)
	{
		// Indexation de l'attribut obligatoire si necessaire,
		// pour pouvoir retrouver les parties sources correspondantes
		bIsIndexed = mandatoryAttribute->IsIndexed();
		if (not bIsIndexed)
			mandatoryAttribute->BuildIndexingStructure();

		// Memorisation de l'index des parties de l'attribut obligatoire
		mandatoryPart = mandatoryAttribute->GetHeadPart();
		nMandatoryIndex = 0;
		while (mandatoryPart != NULL)
		{
			lnkdMandatoryParts.SetAt(mandatoryPart, nMandatoryIndex + 1);

			// Partie suivante
			mandatoryAttribute->GetNextPart(mandatoryPart);
			nMandatoryIndex++;
		}
	}

	// Creation des informations sur les parties de l'attribut source
	oaGroupableAttributePartInformation->SetSize(sourceAttribute->GetPartNumber());
	sourcePart = sourceAttribute->GetHeadPart();
	nSourceIndex = 0;
	while (sourcePart != NULL)
	{
		assert(sourcePart->GetPartFrequency() > 0);

		// Creation d'une information sur la partie source
		partInformation = new KWDGMGroupableAttributePartInformation;
		oaGroupableAttributePartInformation->SetAt(nSourceIndex, partInformation);

		// Informations sur la partie source
		partInformation->SetSourcePartIndex(nSourceIndex);
		partInformation->SetSourcePartFrequency(sourcePart->GetPartFrequency());

		// Recherche de l'index de la partie correspondante dans l'attribut obligatoire
		if (mandatoryAttribute != NULL)
		{
			// Recherche de la partie obligatoire contenant la partie source
			mandatoryPart =
			    mandatoryAttribute->LookupGroupablePart(sourcePart->GetValueSet()->GetHeadValue());
			assert(mandatoryAttribute != NULL);

			// Memorisation de l'index de la partie obligatoire
			nMandatoryIndex = (int)(lnkdMandatoryParts.Lookup(mandatoryPart) - 1);
			partInformation->SetMandatoryPartIndex(nMandatoryIndex);
			assert(nMandatoryIndex >= 0);
		}

		// Partie suivante
		sourceAttribute->GetNextPart(sourcePart);
		nSourceIndex++;
	}

	// Nettoyage concernant l'attribut obligatoire
	if (mandatoryAttribute != NULL)
	{
		if (not bIsIndexed)
			mandatoryAttribute->DeleteIndexingStructure();
	}
	ensure(oaGroupableAttributePartInformation->GetSize() == sourceAttribute->GetPartNumber());
}

void KWDataGridManager::ComputeContinuousAttributeCumulatedFrequencies(const KWDGAttribute* attribute,
								       IntVector* ivCumulatedFrequencies) const
{
	KWDGPart* part;
	int nPart;

	require(attribute != NULL);
	require(attribute->GetAttributeType() == KWType::Continuous);
	require(attribute->ArePartsSorted());
	require(ivCumulatedFrequencies != NULL);

	// Parcours des intervalles pour en deduire les effectifs cumules
	ivCumulatedFrequencies->SetSize(attribute->GetPartNumber());
	nPart = 0;
	part = attribute->GetHeadPart();
	while (part != NULL)
	{
		assert(part->GetPartFrequency() > 0);

		// Prise en compte de la partie courante
		ivCumulatedFrequencies->SetAt(nPart, part->GetPartFrequency());

		// Cumul avec les partie precedentes
		if (nPart > 0)
			ivCumulatedFrequencies->UpgradeAt(nPart, ivCumulatedFrequencies->GetAt(nPart - 1));
		nPart++;
		attribute->GetNextPart(part);
	}
	ensure(ivCumulatedFrequencies->GetSize() == attribute->GetPartNumber());
	ensure(ivCumulatedFrequencies->GetAt(ivCumulatedFrequencies->GetSize() - 1) ==
	       attribute->ComputeTotalPartFrequency());
}

//////////////////////////////////////////////////////////////////////////////

void KWDGMGroupableAttributePartInformation::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Source\tMandatory\tFrequency\tRandom\tTarget\n";
}

void KWDGMGroupableAttributePartInformation::WriteLineReport(ostream& ost) const
{
	ost << GetSourcePartIndex() << "\t";
	ost << GetMandatoryPartIndex() << "\t";
	ost << GetSourcePartFrequency() << "\t";
	ost << GetRandomIndex() << "\t";
	ost << GetTargetPartIndex() << "\n";
}

int KWDGMGroupableAttributePartInformationCompareSourceIndexes(const void* elem1, const void* elem2)
{
	int nCompare;
	KWDGMGroupableAttributePartInformation* partInformation1;
	KWDGMGroupableAttributePartInformation* partInformation2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux informations sur les parties
	partInformation1 = cast(KWDGMGroupableAttributePartInformation*, *(Object**)elem1);
	partInformation2 = cast(KWDGMGroupableAttributePartInformation*, *(Object**)elem2);

	// Comparaison sur l'index de la partie source
	nCompare = partInformation1->GetSourcePartIndex() - partInformation2->GetSourcePartIndex();
	return nCompare;
}

int KWDGMGroupableAttributePartInformationCompareMandatoryAndRandomIndexes(const void* elem1, const void* elem2)
{
	int nCompare;
	KWDGMGroupableAttributePartInformation* partInformation1;
	KWDGMGroupableAttributePartInformation* partInformation2;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux informations sur les parties
	partInformation1 = cast(KWDGMGroupableAttributePartInformation*, *(Object**)elem1);
	partInformation2 = cast(KWDGMGroupableAttributePartInformation*, *(Object**)elem2);

	// Comparaison sur l'index de la partie obligatoire
	nCompare = partInformation1->GetMandatoryPartIndex() - partInformation2->GetMandatoryPartIndex();

	// Comparaison sur l'index aleatoire de la partie source
	if (nCompare == 0)
		nCompare = partInformation1->GetRandomIndex() - partInformation2->GetRandomIndex();
	assert(nCompare != 0);
	return nCompare;
}
