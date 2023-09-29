// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridManager.h"

KWDataGridManager::KWDataGridManager()
{
	sourceDataGrid = NULL;
}

KWDataGridManager::~KWDataGridManager() {}

void KWDataGridManager::CopyDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* targetDataGrid) const
{
	KWDataGridManager dataGridManager;

	require(initialDataGrid != NULL);
	require(targetDataGrid != NULL);

	// Utilisation d'un manager de grille pour effectuier la copie
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	targetDataGrid->DeleteAll();
	dataGridManager.ExportDataGrid(targetDataGrid);
}

void KWDataGridManager::CopyDataGridWithInnerAttributesCloned(const KWDataGrid* initialDataGrid,
							      KWDataGrid* targetDataGrid) const
{
	KWDataGridManager dataGridManager;

	require(initialDataGrid != NULL);
	require(targetDataGrid != NULL);

	// Utilisation d'un manager de grille pour effectuier la copie
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	targetDataGrid->DeleteAll();
	dataGridManager.ExportDataGridWithInnerAttributesCloned(targetDataGrid);
}

void KWDataGridManager::CopyInformativeDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* targetDataGrid) const
{
	KWDataGridManager dataGridManager;

	require(initialDataGrid != NULL);
	require(targetDataGrid != NULL);

	// Utilisation d'un manager de grille pour effectuer la copie
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	targetDataGrid->DeleteAll();

	// Export de la partie informative
	dataGridManager.ExportInformativeAttributes(targetDataGrid);
	dataGridManager.ExportParts(targetDataGrid);
	dataGridManager.ExportCells(targetDataGrid);
	ensure(dataGridManager.CheckDataGrid(targetDataGrid));
}

void KWDataGridManager::SetSourceDataGrid(const KWDataGrid* dataGrid)
{
	require(dataGrid == NULL or dataGrid->Check());
	sourceDataGrid = dataGrid;
}

const KWDataGrid* KWDataGridManager::GetSourceDataGrid() const
{
	return sourceDataGrid;
}

void KWDataGridManager::ExportDataGrid(KWDataGrid* targetDataGrid) const
{
	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Export des attributs
	ExportAttributes(targetDataGrid);

	// Export des partie des attributs
	ExportParts(targetDataGrid);

	// Export des cellules
	ExportCells(targetDataGrid);
	ensure(CheckDataGrid(targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportDataGridWithInnerAttributesCloned(KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Export des attributs
	ExportAttributes(targetDataGrid);

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
	ExportCells(targetDataGrid);
	ensure(CheckDataGrid(targetDataGrid));
}

// CH IV Begin
void KWDataGridManager::ExportDataGridWithSingletonVarParts(const KWDataGrid* referenceDataGrid,
							    KWDataGrid* targetDataGrid,
							    boolean bSourceSimpleAttributeParts) const
{
	KWDGAttribute* targetVarPartAttribute;
	const KWDataGrid* originDataGrid;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(sourceDataGrid->GetInformativeAttributeNumber() > 0);
	require(sourceDataGrid->IsVarPartDataGrid());
	require(referenceDataGrid != NULL);
	require(referenceDataGrid->IsVarPartDataGrid());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export des attributs
	ExportAttributes(targetDataGrid);

	// Attention, on reutilise les attribut internes de la grille optimisee
	if (targetDataGrid->IsVarPartDataGrid())
	{
		// Partage des partitions des attributs internes de la grille optimisee
		targetVarPartAttribute = targetDataGrid->GetVarPartAttribute();
		targetVarPartAttribute->SetInnerAttributes(referenceDataGrid->GetInnerAttributes());
		assert(targetVarPartAttribute->GetVarPartsShared());
	}

	// Export des partie des attributs si aucune variable informatives
	if (referenceDataGrid->GetInformativeAttributeNumber() == 0)
		ExportParts(targetDataGrid);
	// Et dans le cas de variables informatives
	else
	{
		// Parametrage de la grille d'origine selon la provenant des clusters d'instances
		if (bSourceSimpleAttributeParts)
			originDataGrid = sourceDataGrid;
		else
			originDataGrid = referenceDataGrid;

		// Initialisation des parties des attributs
		for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
		{
			targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

			// Recherche de l'attribut source correspondant
			sourceAttribute = originDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
			check(sourceAttribute);

			// Cas d'un attribut Continuous ou Symbol
			if (KWType::IsSimple(sourceAttribute->GetAttributeType()))
				InitialiseAttributeParts(sourceAttribute, targetAttribute);
			// Sinon, cas d'un attribut VarPart
			// Creation des parties de parties de variable de l'attribut, avec un cluster par partie de variable
			else
				targetAttribute->CreateVarPartsSet();
		}
		assert(CheckParts(targetDataGrid));
	}

	// Export des cellules
	ExportCells(targetDataGrid);

	ensure(CheckDataGrid(targetDataGrid));
	ensure(targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
	       referenceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}
// CH IV End

void KWDataGridManager::ExportTerminalDataGrid(KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export des attributs
	ExportAttributes(targetDataGrid);

	// Initialisation des attributs avec une seule partie
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut source et cible
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Creation d'une seule partie par attribut
		InitialiseAttributeNullPart(sourceAttribute, targetAttribute);
	}

	// Export des cellules
	ExportCells(targetDataGrid);

	ensure(CheckDataGrid(targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

// CH IV Begin
void KWDataGridManager::ExportNullDataGrid(KWDataGrid* targetDataGrid) const
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
	ExportAttributes(targetDataGrid);

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
			targetAttribute->SetVarPartsShared(false);

			// Creation de l'ensemble des valeur cible
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
	ExportCells(targetDataGrid);

	ensure(CheckDataGrid(targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() !=
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::InitializeQuantileBuilders(ObjectDictionary* odQuantilesBuilders,
						   IntVector* ivMaxPartNumbers) const
{
	KWDGAttribute* attribute;
	int nAttribute;
	KWQuantileBuilder* quantileBuilder;
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
		CreateAttributeQuantileBuilder(attribute, quantileBuilder, nMaxPartNumber);
		odQuantilesBuilders->SetAt(attribute->GetAttributeName(), quantileBuilder);

		// Memorisation du nombre maximal de parties
		ivMaxPartNumbers->Add(nMaxPartNumber);
	}
	assert(odQuantilesBuilders->GetCount() == sourceDataGrid->GetAttributeNumber());
	assert(ivMaxPartNumbers->GetSize() == sourceDataGrid->GetAttributeNumber());
}

double KWDataGridManager::ExportDataGridWithVarPartMergeOptimization(KWDataGrid* targetDataGrid,
								     const KWDataGridCosts* dataGridCosts) const
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

	// Parametrage du profiler
	KWDataGridOptimizer::GetProfiler()->BeginMethod("Post VarPart merge");

	// Export des attributs
	ExportAttributes(targetDataGrid);

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
	dFusionDeltaCost = MergePartsForVarPartAttributes(targetDataGrid);

	// Tri des parties attributs internes pour un attribut de grille de type VarPart,
	// celles-ci ayant potentiellement ete modifiees
	targetDataGrid->GetVarPartAttribute()->GetInnerAttributes()->SortInnerAttributeParts();

	// Export des cellules
	ExportCells(targetDataGrid);

	// Tri des parties des attributs
	// Ce tri ne peut etre fait qu'apres l'export des cellules qui donnent les effectifs
	targetDataGrid->SortAttributeParts();

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

	// Parametrage du profiler
	KWDataGridOptimizer::GetProfiler()->EndMethod("Post VarPart merge");

	ensure(CheckDataGrid(targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() !=
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
	return dFusionDeltaCost;
}

// CH IV Begin
void KWDataGridManager::UpdateVarPartDataGridFromVarPartGroups(KWDataGrid* targetDataGrid,
							       const IntVector* ivTargetGroupIndexes,
							       int nTargetGroupNumber) const
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
	ExportCells(targetDataGrid);

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Preparation d'une grille pour l'optimisation univariee\t"
		     << sourceDataGrid->GetVarPartAttribute()->GetAttributeName() << endl;
		cout << "Grille initiale\n" << *sourceDataGrid << endl;
		cout << "Grille optimisee\n" << *targetDataGrid << endl;
	}

	// Verification de la grille preparee
	ensure(targetAttribute->GetPartNumber() == nTargetGroupNumber);
	ensure(sourceDataGrid->GetGridFrequency() == targetDataGrid->GetGridFrequency());
	ensure(sourceDataGrid->GetCellNumber() >= targetDataGrid->GetCellNumber());
	ensure(targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
	       sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}
// CH IV End
void KWDataGridManager::ExportGranularizedDataGrid(KWDataGrid* targetDataGrid, int nGranularity,
						   const ObjectDictionary* odQuantilesBuilders) const
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
	ExportAttributes(targetDataGrid);

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
		InitialiseAttributeGranularizedParts(sourceAttribute, targetAttribute, nGranularity, quantileBuilder);
	}

	// Export des cellules
	ExportCells(targetDataGrid);

	// On verifie l'integrite de la grille en sortie avant de modifier sa granularite
	ensure(CheckDataGrid(targetDataGrid));

	// Memorisation de la granularite
	targetDataGrid->SetGranularity(nGranularity);
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

// CH IV Begin
void KWDataGridManager::InitializeInnerAttributesQuantileBuilders(ObjectDictionary* odInnerAttributesQuantilesBuilders,
								  IntVector* ivMaxPartNumbers) const
{
	KWDGAttribute* varPartAttribute;
	int nInnerAttribute;
	KWDGAttribute* innerAttribute;
	KWQuantileBuilder* quantileBuilder;
	int nMaxPartNumber;

	require(Check());
	require(sourceDataGrid->IsVarPartDataGrid());
	require(sourceDataGrid->GetInnerAttributes()->AreInnerAttributePartsSorted());
	require(odInnerAttributesQuantilesBuilders != NULL);
	require(ivMaxPartNumbers != NULL);
	require(odInnerAttributesQuantilesBuilders->GetCount() == 0);
	require(ivMaxPartNumbers->GetSize() == 0);

	// Acces a l'attribut source de type VarPart
	varPartAttribute = sourceDataGrid->GetVarPartAttribute();
	assert(varPartAttribute != NULL);

	// Parcours des attributs internes pour la construction des quantile builders (un par attribut interne)
	for (nInnerAttribute = 0; nInnerAttribute < varPartAttribute->GetInnerAttributeNumber(); nInnerAttribute++)
	{
		innerAttribute = varPartAttribute->GetInnerAttributeAt(nInnerAttribute);

		// Creation et rangement d'un quantile builder dans un dictionnaire
		CreateAttributeQuantileBuilder(innerAttribute, quantileBuilder, nMaxPartNumber);
		odInnerAttributesQuantilesBuilders->SetAt(innerAttribute->GetAttributeName(), quantileBuilder);

		// Memorisation du nombre maximal de parties
		ivMaxPartNumbers->Add(nMaxPartNumber);
	}
	assert(odInnerAttributesQuantilesBuilders->GetCount() == varPartAttribute->GetInnerAttributeNumber());
	assert(ivMaxPartNumbers->GetSize() == varPartAttribute->GetInnerAttributeNumber());
}

void KWDataGridManager::ExportGranularizedDataGridForVarPartAttributes(
    KWDataGrid* targetDataGrid, int nGranularity, const ObjectDictionary* odInnerAttributesQuantilesBuilders) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGInnerAttributes* granularizedInnerAttributes;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(nGranularity > 0);
	require(sourceDataGrid->IsVarPartDataGrid());
	require(odInnerAttributesQuantilesBuilders->GetCount() ==
		sourceDataGrid->GetInnerAttributes()->GetInnerAttributeNumber());

	// Export des attributs
	ExportAttributes(targetDataGrid);

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Export des parties telles quelle dans le cas standard
		if (KWType::IsSimple(sourceAttribute->GetAttributeType()))
			InitialiseAttributeParts(sourceAttribute, targetAttribute);
		// Cas des attributs sources de type VarPart
		else
		{
			assert(sourceAttribute->GetAttributeType() == KWType::VarPart);

			// Creation d'attributs internes en granularisant les attributs internes source
			granularizedInnerAttributes = CreateGranularizedInnerAttributes(
			    sourceDataGrid->GetInnerAttributes(), nGranularity, odInnerAttributesQuantilesBuilders);

			// Parametrage des attributs internes de l'attrbut VarPart
			targetAttribute->SetInnerAttributes(granularizedInnerAttributes);
			targetAttribute->SetVarPartsShared(false);

			// Creation d'un cluster par partie de variable
			targetAttribute->CreateVarPartsSet();

			// Initialisation du nombre total de parties de variables qui compose l'attribut de grille de type VarPart
			targetAttribute->SetInitialValueNumber(
			    granularizedInnerAttributes->ComputeTotalInnerAttributeVarParts());
		}
	}
	// Export des cellules
	ExportCells(targetDataGrid);

	// Memorisation de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Tri des parties des attributs
	// Ce tri ne peut etre fait qu'apres l'export des cellules qui donnent les effectifs
	targetDataGrid->SortAttributeParts();
	ensure(CheckDataGrid(targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() !=
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportFrequencyTableFromOneAttribute(KWFrequencyTable* kwFrequencyTable,
							     const ALString& sAttributeName) const
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
	InitialiseDataGrid(sourceDataGrid, &oneAttributeDataGrid, 1);

	// Recherche de l'attribut source et cible
	sourceAttribute = sourceDataGrid->SearchAttribute(sAttributeName);
	targetAttribute = oneAttributeDataGrid.GetAttributeAt(0);

	// Transfert du parametrage de l'attribut
	InitialiseAttribute(sourceAttribute, targetAttribute);

	// Export des parties de cette grille
	ExportParts(&oneAttributeDataGrid);

	// Export des cellules de cette grille
	ExportCells(&oneAttributeDataGrid);

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

void KWDataGridManager::ExportAttributes(KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Initialisation de la grille cible
	InitialiseDataGrid(sourceDataGrid, targetDataGrid, sourceDataGrid->GetAttributeNumber());

	// Initialisation des attributs
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut source et cible
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Transfert du parametrage de l'attribut
		InitialiseAttribute(sourceAttribute, targetAttribute);
	}
	ensure(CheckAttributes(targetDataGrid));
	ensure(not sourceDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportInformativeAttributes(KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	int nTargetAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Initialisation de la grille cible
	InitialiseDataGrid(sourceDataGrid, targetDataGrid, sourceDataGrid->GetInformativeAttributeNumber());

	// Initialisation des attributs
	nTargetAttribute = 0;
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut source
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);

		// Transfert du parametrage de l'attribut s'il est informatif
		if (sourceAttribute->GetPartNumber() > 1)
		{
			// Acces a l'attribut cible
			targetAttribute = targetDataGrid->GetAttributeAt(nTargetAttribute);
			nTargetAttribute++;

			// Transfert du parametrage de l'attribut
			InitialiseAttribute(sourceAttribute, targetAttribute);
		}
	}
	ensure(CheckAttributes(targetDataGrid));
	ensure(not targetDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportParts(KWDataGrid* targetDataGrid) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid) and CheckGranularity(targetDataGrid));

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
	ensure(CheckParts(targetDataGrid));
}

// CH IV Begin
void KWDataGridManager::ExportDataGridWithReferenceVarPartClusters(KWDataGrid* referenceDataGrid,
								   KWDataGrid* targetDataGrid)
{
	IntVector ivTargetGroupIndexes;
	int nTargetGroupNumber;
	LongintNumericKeyDictionary lnkdClusterIndexes;
	int nInitial;
	int nPartIndex;
	int nAttribute;
	KWDGAttribute* initialAttribute;
	KWDGAttribute* referenceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGAttribute* referenceInnerAttribute;
	KWDGPart* initialPart;
	KWDGPart* initialVarPart;
	KWDGPart* referencePart;
	KWDGPart* referenceVarPart;
	Continuous cValue;
	Symbol sValue;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(referenceDataGrid != NULL);
	require(sourceDataGrid->IsVarPartDataGrid());
	require(sourceDataGrid->IsVarPartDataGrid() == referenceDataGrid->IsVarPartDataGrid());
	require(sourceDataGrid->GetInnerAttributes() != referenceDataGrid->GetInnerAttributes() or
		sourceDataGrid->GetVarPartAttribute()->ContainsSubParts(referenceDataGrid->GetVarPartAttribute()));

	// Export des attributs depuis la grille initiale
	ExportAttributes(targetDataGrid);

	////////////////////////////////////////////////////////////////////////////////////
	// Export des parties pour tous les attributs de la grille, hors attribut VarPart

	// Export des parties de la grille de reference vers la grille cible
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);
		assert(targetAttribute->GetPartNumber() == 0);

		// Uniquement pour les attributs de type simple
		if (KWType::IsSimple(targetAttribute->GetAttributeType()))
		{
			// Recherche de l'attribut source correspondant
			referenceAttribute = referenceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());

			// Transfert du parametrage des parties de l'attribut
			InitialiseAttributeParts(referenceAttribute, targetAttribute);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// On refait les parties de l'attribut VarPart en utilisant la grille de reference
	// pour redispatcher les VarPart initiaux dans des nouveaux groupes

	// Acces aux attributs des grilles initiale et de reference pour l'attribut VarPart
	initialAttribute = sourceDataGrid->GetVarPartAttribute();
	referenceAttribute = referenceDataGrid->GetVarPartAttribute();

	// Initialisation du vecteur d'index de grpupe cible
	ivTargetGroupIndexes.SetSize(initialAttribute->GetPartNumber());
	nTargetGroupNumber = referenceAttribute->GetPartNumber();

	// Construction d'un dictionnaire d'index des groupes de l'attribut VarPart de la grille de reference
	nPartIndex = 0;
	referencePart = referenceAttribute->GetHeadPart();
	while (referencePart != NULL)
	{
		lnkdClusterIndexes.SetAt((NUMERIC)referencePart, nPartIndex);

		// Partie suivante
		nPartIndex++;
		referenceAttribute->GetNextPart(referencePart);
	}

	// Indexation de la grille de reference
	referenceDataGrid->BuildIndexingStructure();

	// Construction du vecteur de correspondance entre les groupes initiaux et les groupes de reference
	// Parcours des parties initiales pour determiner le groupe de destination dans la grille
	// de reference et memoriser son index
	initialPart = initialAttribute->GetHeadPart();
	nInitial = 0;
	while (initialPart != NULL)
	{
		// On prend la premiere partie de variable du groupe
		initialVarPart = initialPart->GetVarPartSet()->GetHeadValue()->GetVarPart();

		// Extraction de l'attribut interne issu de la grille de reference pour cette partie
		referenceInnerAttribute =
		    cast(KWDGAttribute*, referenceDataGrid->GetInnerAttributes()->LookupInnerAttribute(
					     initialVarPart->GetAttribute()->GetAttributeName()));

		// Recherche de la partie de variable de reference dns le cas d'un attribut interne numerique
		if (initialVarPart->GetPartType() == KWType::Continuous)
		{
			// Recherche d'une valeur typique: le milieu de l'intervalle (hors borne inf)
			cValue = KWContinuous::GetUpperMeanValue(initialVarPart->GetInterval()->GetLowerBound(),
								 initialVarPart->GetInterval()->GetUpperBound());

			// Recherche de la partie de variable contenant cette valeur dans l'attribut interne de la
			// grille de reference
			referenceVarPart = referenceInnerAttribute->LookupContinuousPart(cValue);
		}

		// Sinon cas d'une partie d'un attribut interne categoriel
		else
		{
			// Recherche d'une valeur typique: la premiere valeur
			assert(initialVarPart->GetValueSet()->GetHeadValue() != NULL);
			sValue = initialVarPart->GetValueSet()->GetHeadValue()->GetSymbolValue();

			// Recherche du groupe de valeurs contenant cette modalite dans l'attribut interne
			// de la grille de reference
			referenceVarPart = referenceInnerAttribute->LookupSymbolPart(sValue);
		}

		// Recherche de la partie (groupe de PV) contenant cette partie de variable
		// dans l'attribut VarPart de la grille de reference
		referencePart = referenceAttribute->LookupVarPart(referenceVarPart);
		assert(initialVarPart->IsSubPart(referenceVarPart));

		// Memorisation de l'index du groupe cible
		nPartIndex = (int)lnkdClusterIndexes.Lookup(referencePart);
		assert(0 <= nPartIndex and nPartIndex < referenceAttribute->GetPartNumber());
		ivTargetGroupIndexes.SetAt(nInitial, nPartIndex);

		// Partie initiale suivante
		initialAttribute->GetNextPart(initialPart);
		nInitial++;
	}

	// Mise a jour de la grille cible sur la base de la nouvelle partition specifiee
	// Les cellule sont reexportee par la methode appelee
	UpdateVarPartDataGridFromVarPartGroups(targetDataGrid, &ivTargetGroupIndexes, nTargetGroupNumber);

	ensure(targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
	       sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
	ensure(CheckDataGrid(targetDataGrid));
}
// CH IV End

void KWDataGridManager::ExportAttributeParts(KWDataGrid* targetDataGrid, const ALString& sAttributeName) const
{
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid));
	require(sourceDataGrid->SearchAttribute(sAttributeName) != NULL);
	require(targetDataGrid->SearchAttribute(sAttributeName) != NULL);
	require(targetDataGrid->SearchAttribute(sAttributeName)->GetPartNumber() == 0);

	// Recherche des attributs source et cible dans la grille directement
	sourceAttribute = sourceDataGrid->SearchAttribute(sAttributeName);
	targetAttribute = targetDataGrid->SearchAttribute(sAttributeName);

	// Transfert du parametrage des parties de l'attribut
	InitialiseAttributeParts(sourceAttribute, targetAttribute);
}

void KWDataGridManager::ExportCells(KWDataGrid* targetDataGrid) const
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
	// CH IV Begin
	KWDGPart* sourceVarPart;
	KWDGPart* targetVarPart;
	KWDGAttribute* innerAttribute;
	// CH IV End

	require(Check());
	require(targetDataGrid != NULL and CheckTargetValues(targetDataGrid) and CheckAttributes(targetDataGrid) and
		CheckParts(targetDataGrid) and targetDataGrid->GetCellNumber() == 0);

	// Passage de la grille cible en mode update
	targetDataGrid->SetCellUpdateMode(true);
	targetDataGrid->BuildIndexingStructure();
	oaTargetParts.SetSize(targetDataGrid->GetAttributeNumber());

	// Collecte une fois pour toutes des attributs sources correspondant aux attribut cible,
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

void KWDataGridManager::ExportRandomAttributes(KWDataGrid* targetDataGrid, int nAttributeNumber) const
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
	InitialiseDataGrid(sourceDataGrid, targetDataGrid, nAttributeNumber);

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
	ensure(CheckAttributes(targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
	ensure(not targetDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::ExportRandomParts(KWDataGrid* targetDataGrid, int nMeanAttributePartNumber) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid) and CheckGranularity(targetDataGrid));
	require(1 <= nMeanAttributePartNumber and nMeanAttributePartNumber <= sourceDataGrid->GetGridFrequency());

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
		check(sourceAttribute);

		// Export d'un sous ensemble de parties de l'attribut
		InitialiseAttributeRandomParts(sourceAttribute, targetAttribute, nMeanAttributePartNumber);
	}
	ensure(CheckParts(targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
}

void KWDataGridManager::AddRandomAttributes(KWDataGrid* targetDataGrid, const KWDataGrid* mandatoryDataGrid,
					    int nRequestedAttributeNumber) const
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
	require(CheckAttributes(mandatoryDataGrid));
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Calcul du nombre d'attribut a exporter
	nAttributeNumber = mandatoryDataGrid->GetAttributeNumber();
	if (nAttributeNumber < nRequestedAttributeNumber)
		nAttributeNumber = nRequestedAttributeNumber;

	// Initialisation de la grille cible
	InitialiseDataGrid(sourceDataGrid, targetDataGrid, nAttributeNumber);

	// Creation d'un vecteur d'index d'attributs cibles choisis aleatoirement,
	// parmi les attribut non deja present dans les attributs obligatoires
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
	ensure(CheckAttributes(targetDataGrid));
	ensure(targetDataGrid->GetAttributeNumber() >= mandatoryDataGrid->GetAttributeNumber());
	ensure(targetDataGrid->GetAttributeNumber() >= nRequestedAttributeNumber);
	ensure(targetDataGrid->GetCellNumber() == 0);
	ensure(not targetDataGrid->IsVarPartDataGrid() or
	       targetDataGrid->GetVarPartAttribute()->GetInnerAttributes() ==
		   sourceDataGrid->GetVarPartAttribute()->GetInnerAttributes());
}

void KWDataGridManager::AddRandomParts(KWDataGrid* targetDataGrid, const KWDataGrid* mandatoryDataGrid,
				       int nRequestedContinuousPartNumber, int nRequestedSymbolPartNumber,
				       double dMinPercentageAddedPart) const
{
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* mandatoryAttribute;
	KWDGAttribute* targetAttribute;
	int nRequestedPartNumber;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid) and CheckGranularity(targetDataGrid));
	require(mandatoryDataGrid != NULL and CheckAttributes(mandatoryDataGrid) and
		CheckGranularity(mandatoryDataGrid));
	require(1 <= nRequestedContinuousPartNumber and
		nRequestedContinuousPartNumber <= sourceDataGrid->GetGridFrequency());
	require(1 <= nRequestedSymbolPartNumber and nRequestedSymbolPartNumber <= sourceDataGrid->GetGridFrequency());
	require(0 <= dMinPercentageAddedPart and dMinPercentageAddedPart <= 1);

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

		// Nombre de partie demandees en fonction du type de l'attribut
		if (sourceAttribute->GetAttributeType() == KWType::Continuous)
			nRequestedPartNumber = nRequestedContinuousPartNumber;
		else
			nRequestedPartNumber = nRequestedSymbolPartNumber;

		// Prise en compte du minimum obligatoire
		nRequestedPartNumber =
		    (int)((dMinPercentageAddedPart + (1 - dMinPercentageAddedPart) * RandomDouble()) *
			  nRequestedPartNumber);
		if (nRequestedPartNumber < 1)
			nRequestedPartNumber = 1;

		// Ajout d'un sous ensemble de parties de l'attribut dans le cas d'un attribut obligatoire
		if (mandatoryAttribute != NULL)
		{
			assert(sourceAttribute->GetAttributeType() == mandatoryAttribute->GetAttributeType());
			AddAttributeRandomParts(sourceAttribute, mandatoryAttribute, targetAttribute,
						nRequestedPartNumber);
		}
		// et dans le cas general sinon
		else
		{
			InitialiseAttributeRandomParts(sourceAttribute, targetAttribute, nRequestedPartNumber);
		}
	}
	ensure(CheckParts(targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
}

void KWDataGridManager::BuildUnivariateDataGridFromAttributeStats(KWDataGrid* targetDataGrid,
								  KWAttributeStats* attributeStats) const
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
	InitialiseDataGrid(sourceDataGrid, targetDataGrid, 1);

	// Initialisation de l'attribut
	targetAttribute = targetDataGrid->GetAttributeAt(0);
	BuildDataGridAttributeFromUnivariateStats(targetAttribute, attributeStats);
	targetAttribute->SetAttributeTargetFunction(false);

	// Export des cellules
	ExportCells(targetDataGrid);
	ensure(CheckDataGrid(targetDataGrid));
}

boolean KWDataGridManager::BuildDataGridFromClassStats(KWDataGrid* targetDataGrid, KWClassStats* classStats) const
{
	boolean bOk = true;
	boolean bSmallSourceDataGrid;
	int nMaxAttributeNumber;
	int nAttributeNumber;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWAttributeStats* attributeStats;
	ObjectArray oaAllAttributeStats;
	NumericKeyDictionary nkdBestAttributeStats;
	ObjectArray oaTargetAttributeStats;

	require(Check());
	require(sourceDataGrid->GetTargetValueNumber() > 0);
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(classStats != NULL);
	require(classStats->GetInformativeAttributeNumber() > 0);

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
			oaTargetAttributeStats.Add(attributeStats);

		// Arret si grille cible complete
		if (oaTargetAttributeStats.GetSize() == nAttributeNumber)
			break;
	}

	// Creation de la grille si au moins dex attributs
	bOk = oaTargetAttributeStats.GetSize() >= 2;
	if (bOk)
	{
		// Initialisation de la grille cible
		InitialiseDataGrid(sourceDataGrid, targetDataGrid, oaTargetAttributeStats.GetSize());

		// Creation des partitions
		for (nAttribute = 0; nAttribute < oaTargetAttributeStats.GetSize(); nAttribute++)
		{
			targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

			// Initialisation de l'attribut
			attributeStats = cast(KWAttributeStats*, oaTargetAttributeStats.GetAt(nAttribute));
			BuildDataGridAttributeFromUnivariateStats(targetAttribute, attributeStats);
			targetAttribute->SetAttributeTargetFunction(false);
		}

		// Export des cellules
		ExportCells(targetDataGrid);
	}

	ensure(not bOk or CheckDataGrid(targetDataGrid));
	return bOk;
}

boolean KWDataGridManager::BuildDataGridFromUnivariateProduct(KWDataGrid* targetDataGrid,
							      KWClassStats* classStats) const
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
		InitialiseDataGrid(sourceDataGrid, targetDataGrid, oaSelectedAtttributes.GetSize());

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
			BuildDataGridAttributeFromGranularizedPartition(sourceAttribute, targetAttribute, classStats);
		}
		// Export des nouvelles cellules
		targetDataGrid->DeleteAllCells();
		ExportCells(targetDataGrid);
	}
	if (bDisplayResults)
		cout << " OptimizeWithMultipleUnivariatePartitions : construction grille initiale achevee" << endl;
	ensure(not bOk or CheckDataGrid(targetDataGrid));
	return bOk;
}

void KWDataGridManager::BuildPartsOfContinuousAttributeFromFrequencyTable(KWDGAttribute* targetAttribute,
									  KWFrequencyTable* kwftTable,
									  const ALString& sAttributeName) const
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

void KWDataGridManager::BuildPartsOfSymbolAttributeFromGroupsIndex(KWDGAttribute* targetAttribute,
								   const IntVector* ivGroups, int nGroupNumber,
								   int nGarbageModalityNumber,
								   const ALString& sAttributeName) const
{
	ObjectArray oaTargetParts;
	KWDGAttribute* initialAttribute;
	KWDGPart* initialPart;
	KWDGPart* targetPart;
	int nGroup;
	int nInitial;
	int nMaxValueNumber;
	boolean bDisplayResults = false;

	require(targetAttribute != NULL);
	require(sAttributeName != "");
	require(nGroupNumber > 0);
	require(ivGroups != NULL);

	// Acces aux attributs des grilles initiale et optimise pour l'attribut de post-optimisation
	initialAttribute = sourceDataGrid->SearchAttribute(sAttributeName);
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
		cout << "Preparation d'un attribut Symbol associe a un groupage univarie \t" << sAttributeName << endl;
		cout << "Grille initiale\n" << *initialAttribute << endl;
		cout << "Grille optimisee\n" << *targetAttribute << endl;
	}

	// Verification de la grille preparee
	ensure(targetAttribute->GetPartNumber() == nGroupNumber);
	ensure(targetAttribute->GetGarbageModalityNumber() == nGarbageModalityNumber);
}

void KWDataGridManager::BuildDataGridAttributeFromUnivariateStats(KWDGAttribute* targetAttribute,
								  KWAttributeStats* attributeStats) const
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
		ExportAttributeSymbolValueFrequencies(targetAttribute);
	}
}

void KWDataGridManager::BuildUnivariateDataGridFromGranularizedPartition(KWDataGrid* univariateTargetDataGrid,
									 int nAttributeIndex,
									 KWClassStats* classStats) const
{
	KWDGAttribute* targetAttribute;
	KWDGAttribute* sourceAttribute;

	require(0 < nAttributeIndex < sourceDataGrid->GetAttributeNumber());

	// Initialisation de la grille cible a une variable
	InitialiseDataGrid(sourceDataGrid, univariateTargetDataGrid, 1);

	// Initialisation de l'attribut cible
	sourceAttribute = sourceDataGrid->GetAttributeAt(nAttributeIndex);
	targetAttribute = univariateTargetDataGrid->GetAttributeAt(0);
	InitialiseAttribute(sourceAttribute, targetAttribute);

	// Construction de la partition optimale associee a la granularite de l'attribut source selon classStats
	BuildDataGridAttributeFromGranularizedPartition(sourceAttribute, targetAttribute, classStats);

	// Export des cellules selon la nouvelle partition
	univariateTargetDataGrid->DeleteAllCells();
	ExportCells(univariateTargetDataGrid);
}

void KWDataGridManager::BuildDataGridAttributeFromGranularizedPartition(KWDGAttribute* sourceAttribute,
									KWDGAttribute* targetAttribute,
									KWClassStats* classStats) const
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
		ExportFrequencyTableFromOneAttribute(kwftSource, sourceAttribute->GetAttributeName());

		// Discretisation univariee optimale de l'attribut granularise
		discretizerMODL->DiscretizeGranularizedFrequencyTable(kwftSource, kwftTarget);

		// Nettoyage
		delete kwftSource;
		kwftSource = NULL;

		bEvaluated = kwftTarget->GetFrequencyVectorNumber() > 1;
		if (bEvaluated)
		{
			// Construction des parties de l'attribut associee a cette discretisation
			BuildPartsOfContinuousAttributeFromFrequencyTable(targetAttribute, kwftTarget,
									  sourceAttribute->GetAttributeName());
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
		ExportFrequencyTableFromOneAttribute(kwftSource, sourceAttribute->GetAttributeName());

		// Groupage de la table d'effectifs source
		grouperMODL->GroupFrequencyTable(kwftSource, kwftTarget, ivGroups);
		delete kwftSource;
		kwftSource = NULL;

		bEvaluated = kwftTarget->GetFrequencyVectorNumber() > 1;
		if (bEvaluated)
		{
			// Construction des parties de l'attribut associee au groupage
			BuildPartsOfSymbolAttributeFromGroupsIndex(
			    targetAttribute, ivGroups, kwftTarget->GetFrequencyVectorNumber(),
			    kwftTarget->GetGarbageModalityNumber(), sourceAttribute->GetAttributeName());

			targetAttribute->SetAttributeTargetFunction(false);
		}
		else
		{
			// Creation de l'ensemble des valeur cible
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
		ExportAttributeSymbolValueFrequencies(targetAttribute);

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

boolean KWDataGridManager::CheckDataGrid(const KWDataGrid* targetDataGrid) const
{
	require(Check());
	require(targetDataGrid != NULL);

	return CheckGranularity(targetDataGrid) and CheckTargetValues(targetDataGrid) and
	       CheckAttributes(targetDataGrid) and CheckParts(targetDataGrid) and CheckCells(targetDataGrid);
}

boolean KWDataGridManager::CheckGranularity(const KWDataGrid* targetDataGrid) const
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

boolean KWDataGridManager::CheckTargetValues(const KWDataGrid* targetDataGrid) const
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

boolean KWDataGridManager::CheckAttributes(const KWDataGrid* targetDataGrid) const
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
			targetAttribute->AddError("Variable unknown in the input data grid");
			bOk = false;
		}
		// Test de compatibilite du type sinon
		else if (targetAttribute->GetAttributeType() != sourceAttribute->GetAttributeType())
		{
			targetAttribute->AddError("Type of the variable inconsistent with that in the input data grid");
			bOk = false;
		}
	}

	return bOk;
}

boolean KWDataGridManager::CheckParts(const KWDataGrid* targetDataGrid) const
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
	require(CheckAttributes(targetDataGrid));

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
					targetPart->AddError("No matching input interval");
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
						    "The matching input interval has not the same lower bound");
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
								    "No input interval with the same upper bound");
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
						sourcePart->AddError(sTmp + "Input value (" +
								     sourceValue->GetSymbolValue() +
								     ") belongs to an output group different from that "
								     "of the first input value");
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

boolean KWDataGridManager::CheckCells(const KWDataGrid* targetDataGrid) const
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
	require(CheckGranularity(targetDataGrid));
	require(CheckTargetValues(targetDataGrid));
	require(CheckAttributes(targetDataGrid));
	require(CheckParts(targetDataGrid));

	///////////////////////////////////////////////////////////////////////////////
	// On construit un nouveau DataGrid de verification, en partant des parties et
	// attributs du DataGrid cible et en y exportant les cellule du DataGrid source
	// On verifie alors que les cellules cible sont identique a celle de verification.
	// Note: cela n'a pas de sens si le test porte sur un DataGrid initialise
	// avec la methode ExportCells. Cela permet par contre de verifier la validite
	// d'un DataGrid construit autrement

	// Initialisation des attributs et partie du DataGrid de verification
	checkDataGridManager.SetSourceDataGrid(targetDataGrid);
	checkDataGridManager.ExportAttributes(&checkDataGrid);
	checkDataGridManager.ExportParts(&checkDataGrid);
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
	checkDataGridManager.SetSourceDataGrid(sourceDataGrid);
	checkDataGridManager.ExportCells(&checkDataGrid);
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
							     ") inconsistent with the reference frequency (" +
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
						    ") inconsistent with the reference frequency (" +
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

boolean KWDataGridManager::Check() const
{
	// La verification de la validite de la griile source est effectuee une fois pour toutes lors de son parametrage
	return sourceDataGrid != NULL;
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
	dataGridManager.SetSourceDataGrid(dataGrid);
	cout << "Input data grid" << endl;
	cout << *dataGrid << endl;

	// Export total (attribut, parties et cellules)
	targetDataGrid1.DeleteAll();
	targetDataGrid2.DeleteAll();
	dataGridManager.ExportDataGrid(&targetDataGrid1);
	cout << "Exported data grid" << endl;
	cout << targetDataGrid1 << endl;

	dataGridManager.InitializeQuantileBuilders(&odQuantileBuilders, &ivMaxPartNumbers);
	// Export avec granularisation (attribut, parties et cellules)
	int nGranularity;
	for (nGranularity = 2; nGranularity <= ceil(log(dataGrid->GetGridFrequency()) / log(2.0)); nGranularity++)
	{
		// Reinitialisation
		targetDataGrid1.DeleteAll();
		dataGridManager.ExportGranularizedDataGrid(&targetDataGrid1, nGranularity, &odQuantileBuilders);
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
		dataGridManager.ExportRandomAttributes(&targetDataGrid1, dataGrid->GetAttributeNumber());
		dataGridManager.ExportRandomParts(&targetDataGrid1, 3);
		dataGridManager.ExportCells(&targetDataGrid1);
		cout << "Random exported data grid" << endl;
		cout << targetDataGrid1 << endl;

		// Export d'une grille avec ajout aleatoire de nouvelles parties
		dataGridManager.AddRandomAttributes(&targetDataGrid2, &targetDataGrid1,
						    dataGrid->GetAttributeNumber() -
							targetDataGrid1.GetAttributeNumber());
		dataGridManager.AddRandomParts(&targetDataGrid2, &targetDataGrid1, 3, 3, 0.5);
		dataGridManager.ExportCells(&targetDataGrid2);
		cout << "Random modified exported data grid" << endl;
		cout << targetDataGrid2 << endl;
	}
}

void KWDataGridManager::InitialiseDataGrid(const KWDataGrid* originDataGrid, KWDataGrid* targetDataGrid,
					   int nAttributeNumber) const
{
	int nTarget;

	require(originDataGrid != NULL);
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(nAttributeNumber >= 0);

	// Export de la granularite
	targetDataGrid->SetGranularity(originDataGrid->GetGranularity());

	// Initialisation de la grille cible
	targetDataGrid->Initialize(nAttributeNumber, originDataGrid->GetTargetValueNumber());

	// Initialisation des valeurs cibles
	for (nTarget = 0; nTarget < originDataGrid->GetTargetValueNumber(); nTarget++)
	{
		targetDataGrid->SetTargetValueAt(nTarget, originDataGrid->GetTargetValueAt(nTarget));
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
		targetAttribute->SetVarPartsShared(true);
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

		// Memorisation de l'effectif de la partie pour les attribut internes
		// Pour les autre attribut, c'es calcule a partir des cellules
		if (sourceAttribute->IsInnerAttribute())
			targetPart->SetPartFrequency(sourcePart->GetPartFrequency());

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

	// Parametrage des attribut interne de l'attribut cible de type VarPart
	targetAttribute->SetInnerAttributes(clonedInnerAttributes);
	targetAttribute->SetVarPartsShared(false);

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

		// Creation de l'ensemble des valeur cible
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
}

void KWDataGridManager::InitialiseAttributeRandomParts(const KWDGAttribute* sourceAttribute,
						       KWDGAttribute* targetAttribute, int nPartNumber) const
{
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	ObjectArray oaSourceParts;
	IntVector ivInstanceIndexes;
	IntVector ivIntervalUpperBounds;
	IntVector ivValueIndexes;
	int n;
	int nBoundIndex;
	int nSourcePart;
	int nTargetPart;
	int nInstanceLastIndex;

	require(Check());
	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(targetAttribute->GetPartNumber() == 0);
	require(1 <= nPartNumber and nPartNumber <= sourceDataGrid->GetGridFrequency());

	// Partition aleatoire des bornes des intervalles (en rangs) dans le cas continu
	if (sourceAttribute->GetAttributeType() == KWType::Continuous)
	{
		// Export des parties de l'attribut source
		sourceAttribute->ExportParts(&oaSourceParts);

		// Tri des intervalles source par borne inf croissante
		oaSourceParts.SetCompareFunction(KWDGPartCompareValues);
		oaSourceParts.Sort();

		// Initialisation d'un ensemble de bornes aleatoires
		InitRandomIndexVector(&ivIntervalUpperBounds, nPartNumber - 1, sourceDataGrid->GetGridFrequency());

		// Creation des intervalles cibles en s'approchant au plus pret des bornes specifiees
		targetPart = NULL;
		nBoundIndex = 0;
		nInstanceLastIndex = 0;
		for (nSourcePart = 0; nSourcePart < oaSourceParts.GetSize(); nSourcePart++)
		{
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
	// Partition aleatoire des valeurs dans le cas d'un attribut groupable
	else
	{
		assert(KWType::IsCoclusteringGroupableType(sourceAttribute->GetAttributeType()));

		// Recopie du fourre-tout
		// Transfert du parametrage du fourre-tout
		// (methode tolerante au cas sans fourre-tout, ce qui permet de l'utiliser dans les cas Symbol et VarPart
		targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());

		// S'il y a moins de valeurs que de partie a constituer, on recopie directement
		// les meme parties
		if (sourceAttribute->GetPartNumber() <= nPartNumber)
		{
			// Transfert du parametrage des parties de l'attribut
			InitialiseAttributeParts(sourceAttribute, targetAttribute);
		}
		// Sinon, partitionnement aleatoire des parties sources
		else
		{
			// Export des parties de l'attribut source
			sourceAttribute->ExportParts(&oaSourceParts);

			// Permutation aleatoire
			oaSourceParts.Shuffle();

			// Initialisation d'un vecteur des index des valeurs
			// Ce vecteur permettra de choisir des bornes de partition apres permutation aleatoire
			ivValueIndexes.SetSize(oaSourceParts.GetSize());
			for (n = 0; n < ivValueIndexes.GetSize() - 1; n++)
				ivValueIndexes.SetAt(n, n);

			// Recherche d'une ensemble de "bornes" de parties aleatoires
			ivValueIndexes.Shuffle();
			ivValueIndexes.SetSize(nPartNumber - 1);
			ivValueIndexes.Sort();

			// Creation de la partition cible
			targetPart = NULL;
			nTargetPart = 0;
			for (nSourcePart = 0; nSourcePart < oaSourceParts.GetSize(); nSourcePart++)
			{
				sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSourcePart));

				// Creation si necessaire d'une partie cible
				if (targetPart == NULL)
				{
					targetPart = targetAttribute->AddPart();

					// Initialisation de ses valeurs
					targetPart->GetValueSet()->CopyFrom(sourcePart->GetValueSet());
				}
				// Sinon, mise a jour des valeurs de la partie en cours
				else
				{
					targetPart->GetValueSet()->UpgradeFrom(sourcePart->GetValueSet());
				}

				// La partie cible est valide si son nombre de partie sources est atteint
				if (nTargetPart < ivValueIndexes.GetSize() and
				    nSourcePart >= ivValueIndexes.GetAt(nTargetPart))
				{
					nTargetPart++;

					// On reinitialise l'indicateur de creation d'intervalle cible
					targetPart = NULL;
				}
			}
			assert(targetAttribute->GetPartNumber() == ivValueIndexes.GetSize() + 1);
		}
	}
}

void KWDataGridManager::AddAttributeRandomParts(const KWDGAttribute* sourceAttribute, KWDGAttribute* mandatoryAttribute,
						KWDGAttribute* targetAttribute, int nRequestedPartNumber) const
{
	KWDGPart* sourcePart;
	KWDGPart* mandatoryPart;
	KWDGPart* targetPart;
	ObjectArray oaSourceParts;
	ObjectArray oaMandatoryParts;
	IntVector ivInstanceIndexes;
	IntVector ivAddedIntervalUpperBounds;
	IntVector ivValueIndexes;
	int nAddedPartNumber;
	int n;
	int nBoundIndex;
	int nSourcePart;
	int nMandatoryPart;
	int nTargetSplit;
	int nInstanceLastIndex;

	require(Check());
	require(CheckAttributesConsistency(sourceAttribute, mandatoryAttribute));
	require(CheckAttributesConsistency(sourceAttribute, targetAttribute));
	require(targetAttribute->GetPartNumber() == 0);
	require(1 <= nRequestedPartNumber and nRequestedPartNumber <= sourceDataGrid->GetGridFrequency());

	// Calcul du nombre de partie supplementaires a ajouter
	nAddedPartNumber = nRequestedPartNumber;

	// Cas particulier: il n'y avait pas de parties dans l'attribut obligatoire
	if (mandatoryAttribute->GetPartNumber() <= 1)
	{
		// Export du nombre de parties demandee (d'au moins une en fait)
		InitialiseAttributeRandomParts(sourceAttribute, targetAttribute, nRequestedPartNumber);
	}
	// Cas particulier: il y a deja assez de partie dans l'attribut obligatoire
	else if (nAddedPartNumber == 0)
	{
		// Transfert du parametrage des parties de l'attribut
		InitialiseAttributeParts(mandatoryAttribute, targetAttribute);
	}
	// Partition aleatoire des bornes des intervalles (en rangs) dans le cas continu
	else if (sourceAttribute->GetAttributeType() == KWType::Continuous)
	{
		// Export des parties de l'attribut source
		sourceAttribute->ExportParts(&oaSourceParts);
		oaSourceParts.SetCompareFunction(KWDGPartCompareValues);
		oaSourceParts.Sort();

		// Export des parties de l'attribut obligatoire
		mandatoryAttribute->ExportParts(&oaMandatoryParts);
		oaMandatoryParts.SetCompareFunction(KWDGPartCompareValues);
		oaMandatoryParts.Sort();
		assert(oaMandatoryParts.GetSize() > 0);

		// Initialisation d'un ensemble de bornes aleatoires
		InitRandomIndexVector(&ivAddedIntervalUpperBounds, nAddedPartNumber,
				      sourceDataGrid->GetGridFrequency());

		// Creation des intervalles cibles en utilisant les intervalles initiaux et
		// en s'approchant au plus pret des bornes specifiees pour les nouveaux intervalles
		targetPart = NULL;
		nBoundIndex = 0;
		nInstanceLastIndex = 0;
		nMandatoryPart = 0;
		mandatoryPart = NULL;
		for (nSourcePart = 0; nSourcePart < oaSourceParts.GetSize(); nSourcePart++)
		{
			sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSourcePart));

			// Preparation de l'intervalle mandatory suivant
			if (mandatoryPart == NULL)
			{
				mandatoryPart = cast(KWDGPart*, oaMandatoryParts.GetAt(nMandatoryPart));
				nMandatoryPart++;
				assert(sourcePart->GetInterval()->GetLowerBound() ==
				       mandatoryPart->GetInterval()->GetLowerBound());
			}

			// Comptage du nombre d'instance sources traitees
			nInstanceLastIndex += sourcePart->GetPartFrequency();

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

			// L'intervalle cible est finalise si sa borne sup coincide avec celle d'un intervalle mandatory
			if (targetPart->GetInterval()->GetUpperBound() == mandatoryPart->GetInterval()->GetUpperBound())
			{
				// On reinitialise l'indicateur de creation d'intervalle cible
				targetPart = NULL;

				// On reinitialise l'indicateur de recherche d'intervalle mandatory
				mandatoryPart = NULL;
			}

			// L'intervalle cible est finalise si son effectif est atteint
			if (nBoundIndex < ivAddedIntervalUpperBounds.GetSize() and
			    nInstanceLastIndex >= ivAddedIntervalUpperBounds.GetAt(nBoundIndex))
			{
				// On reinitialise l'indicateur de creation d'intervalle cible
				targetPart = NULL;

				// On recherche la prochaine borne d'intervalle a depasser
				while (nBoundIndex < ivAddedIntervalUpperBounds.GetSize())
				{
					if (ivAddedIntervalUpperBounds.GetAt(nBoundIndex) <= nInstanceLastIndex)
						nBoundIndex++;
					else
						break;
				}
			}
		}
	}
	// Partition aleatoire des valeurs dans le cas d'un attribut groupable
	else
	{
		assert(KWType::IsCoclusteringGroupableType(sourceAttribute->GetAttributeType()));

		// S'il y a moins de valeurs que de partie a constituer, on recopie directement les meme parties
		if (sourceAttribute->GetPartNumber() <= mandatoryAttribute->GetPartNumber() + nAddedPartNumber)
		{
			// Transfert du parametrage des parties de l'attribut
			InitialiseAttributeParts(sourceAttribute, targetAttribute);
		}
		// Sinon, partitionnement aleatoire des parties sources
		else
		{
			// Tri des parties sources synchronisee selon les parties mandatory
			SortAttributePartsByTargetGroups(sourceAttribute, mandatoryAttribute, &oaSourceParts,
							 &oaMandatoryParts);

			// Reinitialisation d'un vecteur des index des valeurs
			// Ce vecteur permettra de choisir des bornes de partition apres permutation aleatoire
			ivValueIndexes.SetSize(oaSourceParts.GetSize());
			for (n = 0; n < ivValueIndexes.GetSize() - 1; n++)
				ivValueIndexes.SetAt(n, n);

			// Recherche d'une ensemble de "bornes" de parties aleatoires a ajouter
			ivValueIndexes.Shuffle();
			ivValueIndexes.SetSize(nAddedPartNumber);
			ivValueIndexes.Sort();

			// Creation de la partition cible
			targetPart = NULL;
			nTargetSplit = 0;
			mandatoryPart = NULL;
			for (nSourcePart = 0; nSourcePart < oaSourceParts.GetSize(); nSourcePart++)
			{
				sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSourcePart));
				mandatoryPart = cast(KWDGPart*, oaMandatoryParts.GetAt(nSourcePart));

				// Creation si necessaire d'une partie cible
				if (targetPart == NULL)
				{
					targetPart = targetAttribute->AddPart();

					// Reinitialisation de ses valeurs
					targetPart->GetValueSet()->CopyFrom(sourcePart->GetValueSet());
				}
				// Sinon, mise a jour des valeurs de la partie en cours
				else
				{
					targetPart->GetValueSet()->UpgradeFrom(sourcePart->GetValueSet());
				}

				// La partie cible est valide si elle finalise une partie mandatorye
				if (nSourcePart == oaSourceParts.GetSize() - 1 or
				    mandatoryPart != cast(KWDGPart*, oaMandatoryParts.GetAt(nSourcePart + 1)))
				{
					// On reinitialise l'indicateur de creation d'intervalle cible
					targetPart = NULL;
				}

				// La partie cible est valide si son nombre de partie sources est atteint
				if (nTargetSplit < ivValueIndexes.GetSize() and
				    nSourcePart >= ivValueIndexes.GetAt(nTargetSplit))
				{
					nTargetSplit++;

					// On reinitialise l'indicateur de creation d'intervalle cible
					targetPart = NULL;
				}
			}
			assert(targetAttribute->GetPartNumber() >= mandatoryAttribute->GetPartNumber());
		}
	}
}

void KWDataGridManager::InitialiseAttributeGranularizedParts(const KWDGAttribute* sourceAttribute,
							     KWDGAttribute* targetAttribute, int nGranularity,
							     KWQuantileBuilder* quantileBuilder) const
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
			    sourceAttribute, targetAttribute, nGranularity,
			    cast(KWQuantileIntervalBuilder*, quantileBuilder));
		}
		// Granularisation dans le cas d'un attribut groupable
		else
		{
			assert(KWType::IsCoclusteringGroupableType(sourceAttribute->GetAttributeType()));
			InitialiseAttributeGranularizedGroupableParts(sourceAttribute, targetAttribute, nGranularity,
								      cast(KWQuantileGroupBuilder*, quantileBuilder));
		}
	}
}

void KWDataGridManager::InitialiseAttributeGranularizedContinuousParts(
    const KWDGAttribute* sourceAttribute, KWDGAttribute* targetAttribute, int nGranularity,
    KWQuantileIntervalBuilder* quantileIntervalBuilder) const
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
	nValueNumber = sourceDataGrid->GetGridFrequency();
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

			// CH IV Begin
			// Cas de la granularisation d'un attribut interne dans un attribut de grille de type VarPart
			if (sourceAttribute->IsInnerAttribute())
			{
				// Memorisation de l'effectif de la partie interne
				// L'effectif des parties des attributs de grille est lui calcule a partir des cellules
				targetPart->SetPartFrequency(
				    quantileIntervalBuilder->GetIntervalFrequencyAt(nPartileIndex));
			}
			// CH IV End
		}
	}

	// Initialisation du nombre de valeurs apres granularisation
	// Cas d'un attribut explicatif dans le cadre d'une analyse supervisee
	// Mise a jour du parametrage du nombre de partiles par le nombre effectif de partiles
	if (IsSupervisedInputAttribute(targetAttribute))
		targetAttribute->SetGranularizedValueNumber(nPartileNumber);
	// Sinon, la granularisation n'est qu'un procede de construction d'une grille initiale
	else
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetInitialValueNumber());
}

void KWDataGridManager::InitialiseAttributeGranularizedGroupableParts(
    const KWDGAttribute* sourceAttribute, KWDGAttribute* targetAttribute, int nGranularity,
    KWQuantileGroupBuilder* quantileGroupBuilder) const
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
	nValueNumber = sourceDataGrid->GetGridFrequency();
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
		if (nPartileNumber == nValueNumber)
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
				    IsSupervisedInputAttribute(targetAttribute))
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
	if (IsSupervisedInputAttribute(targetAttribute))
		targetAttribute->SetGranularizedValueNumber(nActualPartileNumber);
	// Sinon, la granularisation n'est qu'un procede de construction d'une grille initiale
	else
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetInitialValueNumber());
}

boolean KWDataGridManager::IsSupervisedInputAttribute(const KWDGAttribute* attribute) const
{
	boolean bIsSupervisedInputAttribute;

	require(attribute != NULL);

	if (attribute->IsInnerAttribute())
		bIsSupervisedInputAttribute = false;
	else
	{
		assert(attribute->GetDataGrid() != NULL);
		bIsSupervisedInputAttribute = (attribute->GetDataGrid()->GetTargetValueNumber() > 0 or
					       (attribute->GetDataGrid()->GetTargetAttribute() != NULL and
						not attribute->GetAttributeTargetFunction()));
	}
	return bIsSupervisedInputAttribute;
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
	bOk = bOk and attribute1->GetInitialValueNumber() == attribute2->GetInitialValueNumber();
	bOk = bOk and attribute1->GetOwnerAttributeName() == attribute2->GetOwnerAttributeName();
	bOk = bOk and attribute1->GetCost() == attribute2->GetCost();
	return bOk;
}

KWDGInnerAttributes* KWDataGridManager::CloneInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes,
							     const KWDataGrid* targetDataGrid) const
{
	KWDGInnerAttributes* resultInnerAttributes;
	int nInnerAttribute;
	KWDGAttribute* sourceInnerAttribute;
	KWDGAttribute* targetInnerAttribute;

	require(sourceInnerAttributes != NULL);
	require(sourceInnerAttributes->Check());
	require(sourceInnerAttributes->AreInnerAttributePartsSorted());
	require(targetDataGrid != NULL);

	// Partage des partitions de la grille source
	resultInnerAttributes = new KWDGInnerAttributes;
	resultInnerAttributes->SetVarPartGranularity(sourceInnerAttributes->GetVarPartGranularity());

	// Parcours des attributs internes
	for (nInnerAttribute = 0; nInnerAttribute < sourceInnerAttributes->GetInnerAttributeNumber(); nInnerAttribute++)
	{
		// Extraction de l'attribut internes source
		sourceInnerAttribute = sourceInnerAttributes->GetInnerAttributeAt(nInnerAttribute);

		// Creation d'un attribut interne identique en exploitant le createur virtuelle de la grille cibke en parametre
		targetInnerAttribute = targetDataGrid->NewAttribute();

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

KWDGInnerAttributes*
KWDataGridManager::CreateGranularizedInnerAttributes(const KWDGInnerAttributes* sourceInnerAttributes, int nGranularity,
						     const ObjectDictionary* odInnerAttributesQuantilesBuilders) const
{
	KWDGInnerAttributes* resultInnerAttributes;
	int nInnerAttribute;
	KWDGAttribute* sourceInnerAttribute;
	KWDGAttribute* targetInnerAttribute;
	KWQuantileBuilder* quantileBuilder;

	require(sourceInnerAttributes != NULL);
	require(sourceInnerAttributes->Check());
	require(sourceInnerAttributes->AreInnerAttributePartsSorted());
	require(0 < nGranularity and
		nGranularity <= ceil(log(sourceInnerAttributes->ComputeTotalInnerAttributeFrequency()) / log(2.0)));
	require(odInnerAttributesQuantilesBuilders->GetCount() == sourceInnerAttributes->GetInnerAttributeNumber());

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

		// Initialisation des parties granularisees de l'attribut
		quantileBuilder =
		    cast(KWQuantileBuilder*,
			 odInnerAttributesQuantilesBuilders->Lookup(targetInnerAttribute->GetAttributeName()));
		InitialiseAttributeGranularizedParts(sourceInnerAttribute, targetInnerAttribute, nGranularity,
						     quantileBuilder);

		// Tri des parties de l'attribut interne
		// Necessaire, car la derniere partie issue de la granularisation peut etre d'effectif plus
		// important en raison du groupe par defaut
		targetInnerAttribute->SortParts();
	}

	// Memorisation de la granularite
	resultInnerAttributes->SetVarPartGranularity(nGranularity);

	ensure(resultInnerAttributes->Check());
	ensure(resultInnerAttributes->GetVarPartGranularity() == nGranularity);
	ensure(resultInnerAttributes->AreInnerAttributePartsSorted());
	ensure(resultInnerAttributes->ComputeTotalInnerAttributeFrequency() ==
	       sourceInnerAttributes->ComputeTotalInnerAttributeFrequency());
	ensure(sourceInnerAttributes->ContainsSubVarParts(resultInnerAttributes));
	return resultInnerAttributes;
}

double KWDataGridManager::MergePartsForVarPartAttributes(KWDataGrid* targetDataGrid) const
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

void KWDataGridManager::CreateAttributeQuantileBuilder(const KWDGAttribute* attribute,
						       KWQuantileBuilder*& quantileBuilder, int& nMaxPartNumber) const
{
	KWQuantileGroupBuilder* quantileGroupBuilder;
	KWQuantileIntervalBuilder* quantileIntervalBuilder;
	ObjectArray oaSourceParts;
	KWDGPart* sourcePart;
	int nSourcePart;
	IntVector ivFrequencies;
	boolean bSingleton;

	require(attribute != NULL);
	require(KWType::IsCoclusteringType(attribute->GetAttributeType()));
	require(attribute->GetPartNumber() > 0);
	require(attribute->ArePartsSorted());

	// Export des parties de l'attribut source
	attribute->ExportParts(&oaSourceParts);

	// Cas d'un attribut continu
	quantileBuilder = NULL;
	nMaxPartNumber = 0;
	if (attribute->GetAttributeType() == KWType::Continuous)
	{
		// Creation du vecteur des frequences par parties
		for (nSourcePart = 0; nSourcePart < oaSourceParts.GetSize(); nSourcePart++)
		{
			sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSourcePart));

			// Comptage du nombre d'instance sources traitees
			ivFrequencies.Add(sourcePart->GetPartFrequency());
		}

		// Creation et rangement d'un quantile builder dans un dictionnaire
		quantileIntervalBuilder = new KWQuantileIntervalBuilder;
		quantileBuilder = quantileIntervalBuilder;

		// Initialisation du quantileBuilder
		quantileIntervalBuilder->InitializeFrequencies(&ivFrequencies);

		// Memorisation du nombre maximal de parties
		nMaxPartNumber = attribute->GetPartNumber();
	}
	// Cas d'un attribut categoriel ou de type VarPart
	else
	{
		assert(KWType::IsCoclusteringGroupableType(attribute->GetAttributeType()));

		// Creation du vecteur des frequences par parties
		bSingleton = false;
		nMaxPartNumber = 0;
		for (nSourcePart = 0; nSourcePart < oaSourceParts.GetSize(); nSourcePart++)
		{
			sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSourcePart));

			// Comptage du nombre d'instance sources traitees
			ivFrequencies.Add(sourcePart->GetPartFrequency());
			assert(nSourcePart == 0 or
			       ivFrequencies.GetAt(nSourcePart) <= ivFrequencies.GetAt(nSourcePart - 1));

			// Cas d'une partie non singleton
			if (sourcePart->GetPartFrequency() > 1)
				nMaxPartNumber++;
			else
				bSingleton = true;
		}
		// Ajout d'une partie regroupant les eventuels singletons
		if (bSingleton)
			nMaxPartNumber++;

		// Creation et rangement d'un quantile builder dans un dictionnaire
		quantileGroupBuilder = new KWQuantileGroupBuilder;
		quantileBuilder = quantileGroupBuilder;

		// Initialisation du quantileBuilder
		quantileGroupBuilder->InitializeFrequencies(&ivFrequencies);
	}
	ensure(quantileBuilder != NULL);
	ensure(quantileBuilder->GetType() == attribute->GetAttributeType() or
	       attribute->GetAttributeType() == KWType::VarPart);
	ensure(nMaxPartNumber >= 1);
}

void KWDataGridManager::ExportAttributeSymbolValueFrequencies(KWDGAttribute* targetAttribute) const
{
	int nInstanceNumber;
	KWDGPart* part;
	KWDGValueSet* valueSet;
	KWDGValue* value;
	KWDGValue* defaultValue;
	KWDGAttribute* sourceAttribute;
	NumericKeyDictionary nkdSourceValues;
	KWDGValue* sourceValue;
	Symbol sValue;
	int nTotalValueFrequency;

	require(targetAttribute != NULL);
	require(targetAttribute->GetAttributeType() == KWType::Symbol);
	require(sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName()) != NULL);

	// Nombre d'instances
	nInstanceNumber = sourceDataGrid->GetGridFrequency();

	// Extraction de l'attribut source
	sourceAttribute = sourceDataGrid->SearchAttribute(targetAttribute->GetAttributeName());
	assert(sourceAttribute != NULL);

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

void KWDataGridManager::SortAttributePartsByTargetGroups(const KWDGAttribute* sourceAttribute,
							 KWDGAttribute* groupedAttribute,
							 ObjectArray* oaSortedSourceParts,
							 ObjectArray* oaSortedGroupedParts) const
{
	boolean bIsIndexed;
	ObjectArray oaSourceParts;
	ObjectArray oaAssociations;
	KWSortableObject* association;
	int nSource;
	int n;
	KWDGPart* sourcePart;
	KWDGPart* groupedPart;

	require(sourceAttribute != NULL);
	require(groupedAttribute != NULL);
	require(sourceAttribute->Check());
	require(groupedAttribute->Check());
	require(KWType::IsCoclusteringGroupableType(sourceAttribute->GetAttributeType()));
	require(sourceAttribute->GetAttributeType() == groupedAttribute->GetAttributeType());
	require(oaSortedSourceParts != NULL);
	require(oaSortedSourceParts->GetSize() == 0);
	require(oaSortedGroupedParts != NULL);
	require(oaSortedGroupedParts->GetSize() == 0);

	// Indexation si necessaire de l'attribut groupe
	bIsIndexed = groupedAttribute->IsIndexed();
	if (not bIsIndexed)
		groupedAttribute->BuildIndexingStructure();

	// On exporte les parties sources dans un tableau
	sourceAttribute->ExportParts(&oaSourceParts);

	// Initialisation d'un tableau d'associations entre index de partie source et
	// (premiere) valeur de groupe source
	oaAssociations.SetSize(oaSourceParts.GetSize());
	for (nSource = 0; nSource < oaSourceParts.GetSize(); nSource++)
	{
		sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSource));

		// Recherche de la partie groupee correspondante
		groupedPart = groupedAttribute->LookupGroupablePart(sourcePart->GetValueSet()->GetHeadValue());

		// Creation de l'association entre index de partie et premiere valeur du groupe
		association = new KWSortableObject;
		oaAssociations.SetAt(nSource, association);
		association->SetIndex(nSource);
		association->SetSortValue(groupedPart->GetValueSet()->GetHeadValue());
	}

	// Tri des association, apres une randomisation pour avoir un ordre aleatoire par groupe
	// CH IV Refactoring: a revoir plus tard apres avoir integre la retokenisation dans le cadre du GenerateVNS
	// CH IV Refactoring: Code specifique suite a refactoring et unification avec l'ancienne methode SortVarPartAttributeParts
	// CH IV Refactoring: Pourquoi le Shuffle est fait dans le cas Symbol et pas VarPart? on ne sait pas
	// CH IV Refactoring: Probleme potentiel supplementaire, faire un shuffle suivi d'un sort entraine des instabilite entre
	// CH IV Refactoring:  windows et linux, car le Sort de windows ne semble pas etre un "stable sort" (qui garantit qu'en
	// CH IV Refactoring:  cas d'egalite, les items sont dans le meme ordre que l'ordre initial)
	if (sourceAttribute->GetAttributeType() == KWType::Symbol)
		oaAssociations.Shuffle();

	// Tri des association, apres une randomisation pour avoir un ordre aleatoire par groupe
	oaAssociations.SetCompareFunction(KWSortableObjectComparePartValue);
	oaAssociations.Sort();

	// On range dans le tableau en sortie les parties sources, triees par groupe
	// (en fait, par leur premiere valeur, ce qui est equivalent), et leur groupe associe
	oaSortedSourceParts->SetSize(oaSourceParts.GetSize());
	oaSortedGroupedParts->SetSize(oaSourceParts.GetSize());
	for (n = 0; n < oaAssociations.GetSize(); n++)
	{
		association = cast(KWSortableObject*, oaAssociations.GetAt(n));

		// Recherche de la partie source
		nSource = association->GetIndex();
		sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSource));

		// Recherche de la partie groupee correspondante
		groupedPart = groupedAttribute->LookupGroupablePart(sourcePart->GetValueSet()->GetHeadValue());

		// Rangement dans les tableaux en sortie
		oaSortedSourceParts->SetAt(n, sourcePart);
		oaSortedGroupedParts->SetAt(n, groupedPart);
	}

	// Nettoyage du tableau d'indexation
	oaAssociations.DeleteAll();

	// Nettoyage eventuel de l'indexation
	if (not bIsIndexed)
		groupedAttribute->DeleteIndexingStructure();

	ensure(oaSortedSourceParts->GetSize() == sourceAttribute->GetPartNumber());
	ensure(oaSortedGroupedParts->GetSize() == sourceAttribute->GetPartNumber());
}

void KWDataGridManager::InitRandomIndexVector(IntVector* ivRandomIndexes, int nIndexNumber, int nMaxIndex) const
{
	double dInitialSize;
	const int nMaxInitialSize = 10000000;
	int nInitialSize;
	int n;
	int nLowerIndex;
	int nUpperIndex;

	require(ivRandomIndexes != NULL);
	require(nIndexNumber >= 0);
	require(nMaxIndex >= 0);
	require(nIndexNumber <= nMaxIndex);

	// Calcul d'une taille de vecteur initial du vecteur d'index
	if (nMaxIndex <= 10000)
		nInitialSize = nMaxIndex;
	else
	{
		dInitialSize = nIndexNumber * 100;
		if (dInitialSize > nMaxInitialSize)
			dInitialSize = nMaxInitialSize;
		nInitialSize = (int)dInitialSize;
		if (nInitialSize < nIndexNumber)
			nInitialSize = nIndexNumber;
	}

	// Initialisation des index dans le cas ou on peut stocker tous les index possible
	ivRandomIndexes->SetSize(nInitialSize);
	if (nInitialSize == nMaxIndex)
	{
		for (n = 0; n < nInitialSize; n++)
			ivRandomIndexes->SetAt(n, n);
	}
	// Sinon, on genere des index aleatoires, par plage d'index
	// Utile pour eviter d'allouer des tres grand vecteur d'entiers
	else
	{
		for (n = 0; n < nInitialSize; n++)
		{
			// Borne pour la generation d'un index aleatoire dans [LowerIndex, UpperIndex[
			nLowerIndex = int(floor((nMaxIndex * 1.0 * n) / nInitialSize));
			nUpperIndex = int(floor((nMaxIndex * 1.0 * (n + 1)) / nInitialSize));
			if (nUpperIndex > nMaxIndex)
				nUpperIndex = nMaxIndex;

			// Ajout d'un nombre aleatoire
			if (nUpperIndex > nLowerIndex + 1)
				ivRandomIndexes->SetAt(n, nLowerIndex + RandomInt(nUpperIndex - nLowerIndex - 1));
			else
				ivRandomIndexes->SetAt(n, nLowerIndex);
		}
	}

	// Randomisation de l'ordre des index et troncature pour obtenir le bon nombre d'index
	ivRandomIndexes->Shuffle();
	ivRandomIndexes->SetSize(nIndexNumber);

	// Tri des index
	ivRandomIndexes->Sort();
}
