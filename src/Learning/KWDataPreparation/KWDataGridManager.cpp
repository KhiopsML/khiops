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
}

void KWDataGridManager::ExportTerminalDataGrid(KWDataGrid* targetDataGrid) const
{
	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	int nTarget;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation de la grille cible
	targetDataGrid->Initialize(sourceDataGrid->GetAttributeNumber(), sourceDataGrid->GetTargetValueNumber());

	// Initialisation des valeurs cibles
	for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
	{
		targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
	}

	// Initialisation des attributs avec une seule partie
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut source et cible
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Transfert du parametrage de l'attribut
		targetAttribute->SetAttributeName(sourceAttribute->GetAttributeName());
		targetAttribute->SetAttributeType(sourceAttribute->GetAttributeType());
		targetAttribute->SetAttributeTargetFunction(sourceAttribute->GetAttributeTargetFunction());
		targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetGranularizedValueNumber());
		targetAttribute->SetCost(sourceAttribute->GetCost());

		// Transfert du parametrage du fourre-tout
		if (sourceAttribute->GetCatchAllValueSet() != NULL)
			targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());

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

	// Export des cellules
	ExportCells(targetDataGrid);

	ensure(CheckDataGrid(targetDataGrid));
}

void KWDataGridManager::InitializeQuantileBuildersBeforeGranularization(ObjectDictionary* odQuantilesBuilders,
									IntVector* ivMaxPartNumbers) const
{
	KWQuantileGroupBuilder* quantileGroupBuilder;
	KWQuantileIntervalBuilder* quantileIntervalBuilder;
	KWDGAttribute* attribute;
	ObjectArray oaSourceParts;
	KWDGPart* sourcePart;
	int nAttribute;
	int nSourcePart;
	IntVector ivFrequencies;
	int nPartNumber;
	boolean bSingleton;

	require(odQuantilesBuilders->GetCount() == 0);
	require(sourceDataGrid->AreAttributePartsSorted());

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		nPartNumber = 0;
		bSingleton = false;
		attribute = sourceDataGrid->GetAttributeAt(nAttribute);

		// Export des parties de l'attribut source
		attribute->ExportParts(&oaSourceParts);

		// Cas d'un attribut continu
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
			odQuantilesBuilders->SetAt(attribute->GetAttributeName(), new KWQuantileIntervalBuilder);
			quantileIntervalBuilder = cast(KWQuantileIntervalBuilder*,
						       odQuantilesBuilders->Lookup(attribute->GetAttributeName()));

			// Initialisation du quantileBuilder
			quantileIntervalBuilder->InitializeFrequencies(&ivFrequencies);

			// Memorisation du nombre maximal de parties
			ivMaxPartNumbers->Add(attribute->GetPartNumber());
		}
		// Cas d'un attribut categoriel
		else
		{
			// Creation du vecteur des frequences par parties
			for (nSourcePart = 0; nSourcePart < oaSourceParts.GetSize(); nSourcePart++)
			{
				sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSourcePart));

				// Comptage du nombre d'instance sources traitees
				ivFrequencies.Add(sourcePart->GetPartFrequency());

				// Cas d'une partie non singleton
				if (sourcePart->GetPartFrequency() > 1)
					nPartNumber++;
				else
					bSingleton = true;
			}
			// Ajout d'une partie regroupant les eventuels singletons
			if (bSingleton)
				nPartNumber++;

			// Memorisation du nombre maximal de parties
			ivMaxPartNumbers->Add(nPartNumber);

			// Creation et rangement d'un quantile builder dans un dictionnaire
			odQuantilesBuilders->SetAt(attribute->GetAttributeName(), new KWQuantileGroupBuilder);
			quantileGroupBuilder =
			    cast(KWQuantileGroupBuilder*, odQuantilesBuilders->Lookup(attribute->GetAttributeName()));

			// Initialisation du quantileBuilder
			quantileGroupBuilder->InitializeFrequencies(&ivFrequencies);
		}
		// Nettoyage
		oaSourceParts.RemoveAll();
		ivFrequencies.SetSize(0);
	}
	assert(odQuantilesBuilders->GetCount() == sourceDataGrid->GetAttributeNumber());
}

void KWDataGridManager::ExportGranularizedDataGrid(KWDataGrid* targetDataGrid, int nGranularity,
						   ObjectDictionary* odQuantilesBuilders) const
{
	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(nGranularity > 0);
	require(odQuantilesBuilders->GetCount() == sourceDataGrid->GetAttributeNumber());

	// Export des attributs
	ExportAttributes(targetDataGrid);

	// Export des parties granularisees des attributs
	ExportGranularizedParts(targetDataGrid, nGranularity, odQuantilesBuilders);

	// Export des cellules
	ExportCells(targetDataGrid);
	ensure(CheckDataGrid(targetDataGrid));

	// Memorisation de la granularite
	targetDataGrid->SetGranularity(nGranularity);
}

void KWDataGridManager::ExportGranularizedParts(KWDataGrid* targetDataGrid, int nGranularity,
						ObjectDictionary* odQuantileBuilders) const
{
	ObjectDictionary odSourceAttributes;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWQuantileGroupBuilder* quantileGroupBuilder;
	KWQuantileIntervalBuilder* quantileIntervalBuilder;
	boolean bDisplayResults = false;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid) and CheckGranularity(targetDataGrid));
	require(0 < nGranularity and nGranularity <= ceil(log(sourceDataGrid->GetGridFrequency()) / log(2.0)));
	require(odQuantileBuilders->GetCount() == sourceDataGrid->GetAttributeNumber());

	// Rangement des attributs sources dans un dictionnaire
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		odSourceAttributes.SetAt(sourceAttribute->GetAttributeName(), sourceAttribute);
	}

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche des attributs cible et source
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);
		sourceAttribute = cast(KWDGAttribute*, odSourceAttributes.Lookup(targetAttribute->GetAttributeName()));

		targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());

		// Cas d'un attribut "cible" (regression, classif avec groupage) : pas de granularisation mais poubelle
		// envisageable
		if (sourceAttribute->GetAttributeTargetFunction())
		{
			targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetTrueValueNumber());

			ExportPartsForAttribute(targetDataGrid, sourceAttribute->GetAttributeName());
			if (bDisplayResults)
			{
				cout << "Attribut cible " << targetAttribute->GetAttributeName() << endl;
				cout << "Partile number = " << targetAttribute->GetGranularizedValueNumber() << endl;
			}
		}

		// Cas des attributs sources
		else
		{
			// Granularisation dans le cas continu
			if (sourceAttribute->GetAttributeType() == KWType::Continuous)
			{
				quantileIntervalBuilder =
				    cast(KWQuantileIntervalBuilder*,
					 odQuantileBuilders->Lookup(sourceAttribute->GetAttributeName()));

				ExportGranularizedPartsForContinuousAttribute(targetDataGrid, sourceAttribute,
									      targetAttribute, nGranularity,
									      quantileIntervalBuilder);
			}
			// Granularisation dans le cas symbolique
			else
			{
				quantileGroupBuilder =
				    cast(KWQuantileGroupBuilder*,
					 odQuantileBuilders->Lookup(sourceAttribute->GetAttributeName()));

				ExportGranularizedPartsForSymbolAttribute(targetDataGrid, sourceAttribute,
									  targetAttribute, nGranularity,
									  quantileGroupBuilder);
			}
		}
	}

	ensure(CheckParts(targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
}

void KWDataGridManager::ExportGranularizedPartsForContinuousAttribute(KWDataGrid* targetDataGrid,
								      KWDGAttribute* sourceAttribute,
								      KWDGAttribute* targetAttribute, int nGranularity,
								      KWQuantileIntervalBuilder* quantileBuilder) const
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

	require(quantileBuilder != NULL);

	nValueNumber = sourceDataGrid->GetGridFrequency();

	// Nombre potentiel de partiles associes a cette granularite
	nPartileNumber = (int)pow(2, nGranularity);
	if (nPartileNumber > nValueNumber)
		nPartileNumber = nValueNumber;

	// Cas ou la granularisation n'est pas appliquee : non prise en compte de la granularite ou granularite maximale
	if (nGranularity == 0 or nPartileNumber >= nValueNumber)
	{
		ExportPartsForAttribute(targetDataGrid, sourceAttribute->GetAttributeName());
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetTrueValueNumber());
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
		quantileBuilder->ComputeQuantiles(nPartileNumber);

		// Initialisation du nombre effectif de partiles (peut etre inferieur au nombre theorique du fait de
		// doublons)
		nActualPartileNumber = quantileBuilder->GetIntervalNumber();

		// Creation des partiles
		for (nPartileIndex = 0; nPartileIndex < nActualPartileNumber; nPartileIndex++)
		{
			targetPart = targetAttribute->AddPart();

			// Extraction du premier l'intervalle du partile
			sourcePart =
			    cast(KWDGPart*,
				 oaSourceParts.GetAt(quantileBuilder->GetIntervalFirstValueIndexAt(nPartileIndex)));
			// Memorisation de sa borne inf
			targetPart->GetInterval()->SetLowerBound(sourcePart->GetInterval()->GetLowerBound());

			// Extraction du dernier intervalle du partile
			sourcePart =
			    cast(KWDGPart*,
				 oaSourceParts.GetAt(quantileBuilder->GetIntervalLastValueIndexAt(nPartileIndex)));
			// Memorisation de sa borne sup
			targetPart->GetInterval()->SetUpperBound(sourcePart->GetInterval()->GetUpperBound());
		}
	}

	// Initialisation du nombre de valeurs apres granularisation
	// Cas d'un attribut explicatif dans le cadre d'une analyse supervisee
	// Mise a jour du parametrage du nombre de partiles par le nombre effectif de partiles
	if ((targetDataGrid->GetTargetValueNumber() > 0 or
	     (targetDataGrid->GetTargetAttribute() != NULL and not sourceAttribute->GetAttributeTargetFunction())))
		targetAttribute->SetGranularizedValueNumber(nPartileNumber);
	// Sinon, la granularisation n'est qu'un procede de construction d'une grille initiale
	else
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetTrueValueNumber());
}

void KWDataGridManager::ExportGranularizedPartsForSymbolAttribute(KWDataGrid* targetDataGrid,
								  KWDGAttribute* sourceAttribute,
								  KWDGAttribute* targetAttribute, int nGranularity,
								  KWQuantileGroupBuilder* quantileBuilder) const
{
	ObjectArray oaSourceParts;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	int nValueNumber;
	int nPartileNumber;
	int nActualPartileNumber;
	int nPartileIndex;
	int nSourceIndex;

	require(quantileBuilder != NULL);

	nValueNumber = sourceDataGrid->GetGridFrequency();

	// Nombre potentiel de partiles associes a cette granularite
	nPartileNumber = (int)pow(2, nGranularity);
	if (nPartileNumber > nValueNumber)
		nPartileNumber = nValueNumber;
	// Initialisation
	nActualPartileNumber = nPartileNumber;

	// Cas ou la granularisation n'est pas appliquee : non prise en compte de la granularite
	if (nGranularity == 0)
	{
		ExportPartsForAttribute(targetDataGrid, sourceAttribute->GetAttributeName());
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetTrueValueNumber());
	}

	// Granularisation
	else
	{
		// Export des parties de l'attribut source
		sourceAttribute->ExportParts(&oaSourceParts);

		// Cas du nombre de partiles associe a la granularite maximale
		if (nPartileNumber == nValueNumber)
			// Seuillage de nPartileNumber au nombre de partiles associe a la granularite precedente
			// pour que la granularisation rassemble les eventuelles valeurs sources
			// singletons dans le fourre-tout
			// Pour G tel que 2^G < N <= 2^(G+1) on aura 1 < N/2^G <= 2 c'est a dire un effectif minimal par
			// partile de 2 (donc pas de singleton apres granularisation)
			nPartileNumber = (int)pow(2, nGranularity - 1);

		// Calcul des quantiles
		quantileBuilder->ComputeQuantiles(nPartileNumber);

		// Initialisation du nombre effectif de partiles (peut etre inferieur au nombre theorique du fait de
		// doublons)
		nActualPartileNumber = quantileBuilder->GetGroupNumber();

		// Creation des partiles
		for (nPartileIndex = 0; nPartileIndex < nActualPartileNumber; nPartileIndex++)
		{
			targetPart = targetAttribute->AddPart();

			// Parcours des instances du partile
			for (nSourceIndex = quantileBuilder->GetGroupFirstValueIndexAt(nPartileIndex);
			     nSourceIndex <= quantileBuilder->GetGroupLastValueIndexAt(nPartileIndex); nSourceIndex++)
			{
				// Extraction de la partie a ajouter dans le groupe
				sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSourceIndex));

				// Ajout de ses valeurs
				targetPart->GetValueSet()->UpgradeFrom(sourcePart->GetValueSet());
			}
			// Compression et memorisation du fourre-tout si necessaire (mode supervise, attribut non cible)
			// La partie qui contient la StarValue est compressee uniquement si elle contient plus d'une
			// modalite (cas d'un vrai fourre-tout)
			if ((targetDataGrid->GetTargetValueNumber() > 0 or
			     (targetDataGrid->GetTargetAttribute() != NULL and
			      not sourceAttribute->GetAttributeTargetFunction())) and
			    targetPart->GetValueSet()->IsDefaultPart() and
			    targetPart->GetValueSet()->GetTrueValueNumber() > 1)
			{
				// Compression du fourre-tout et memorisation de ses valeurs
				targetAttribute->SetCatchAllValueSet(
				    targetPart->GetValueSet()->ConvertToCleanedValueSet());
			}
			// Tri des valeurs du fourre tout
			if (targetPart->GetValueSet()->IsDefaultPart())
				targetPart->GetValueSet()->SortValues();
		}
	}

	// Cas d'un attribut explicatif dans le cadre d'une analyse supervisee
	// Mise a jour du parametrage du nombre de partiles par le nombre effectif de groupes distincts
	if ((targetDataGrid->GetTargetValueNumber() > 0 or
	     (targetDataGrid->GetTargetAttribute() != NULL and not sourceAttribute->GetAttributeTargetFunction())))
		targetAttribute->SetGranularizedValueNumber(nActualPartileNumber);
	// Sinon, la granularisation n'est qu'un procede de construction d'une grille initiale
	else
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetTrueValueNumber());
}

void KWDataGridManager::ExportFrequencyTableFromOneAttribute(const KWFrequencyVector* kwfvCreator,
							     KWFrequencyTable* kwFrequencyTable,
							     const ALString& sAttributeName) const
{
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	boolean bDisplayResults = false;
	KWDataGrid oneAttributeDataGrid;
	KWDGPart* dgPart;
	KWDGCell* dgCell;
	ObjectArray oaParts;
	IntVector* ivFrequency;
	int nPartIndex;
	int nTargetIndex;
	int nTargetValueNumber;
	int nSourceValueNumber;

	require(kwFrequencyTable != NULL);
	require(sAttributeName != "");

	// Export de la granularite
	kwFrequencyTable->SetGranularity(sourceDataGrid->GetGranularity());

	// Export d'une grille reduite a l'attribut
	ExportOneAttribute(&oneAttributeDataGrid, sAttributeName);
	// Export des parties de cette grille
	ExportParts(&oneAttributeDataGrid);
	// Export des cellules de cette grille
	ExportCells(&oneAttributeDataGrid);

	// Export des parties de l'attribut
	oneAttributeDataGrid.GetAttributeAt(0)->ExportParts(&oaParts);

	// Initialisation du nombre de parties sources de la table de contingence
	nSourceValueNumber = oaParts.GetSize();
	nTargetValueNumber = 0;

	kwFrequencyTable->SetFrequencyVectorCreator(kwfvCreator->Clone());
	kwFrequencyTable->Initialize(nSourceValueNumber);
	kwFrequencyTable->SetInitialValueNumber(oneAttributeDataGrid.GetAttributeAt(0)->GetInitialValueNumber());
	kwFrequencyTable->SetGranularizedValueNumber(
	    oneAttributeDataGrid.GetAttributeAt(0)->GetGranularizedValueNumber());

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
		if (oneAttributeDataGrid.GetAttributeAt(0)->GetAttributeType() == KWType::Symbol)
		{
			// Recopie du nombre de modalites
			kwdfvFrequencyVector->SetModalityNumber(dgPart->GetValueSet()->GetTrueValueNumber());

			if (oneAttributeDataGrid.GetAttributeAt(0)->GetGarbagePart() == dgPart)
				kwFrequencyTable->SetGarbageModalityNumber(dgPart->GetValueSet()->GetTrueValueNumber());
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
	int nTarget;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation de la grille cible
	targetDataGrid->Initialize(sourceDataGrid->GetAttributeNumber(), sourceDataGrid->GetTargetValueNumber());

	// Initialisation des valeurs cibles
	for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
	{
		targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
	}

	// Initialisation des attributs
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut source et cible
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Transfert du parametrage de l'attribut
		targetAttribute->SetAttributeName(sourceAttribute->GetAttributeName());
		targetAttribute->SetAttributeType(sourceAttribute->GetAttributeType());
		targetAttribute->SetAttributeTargetFunction(sourceAttribute->GetAttributeTargetFunction());
		targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetGranularizedValueNumber());
		targetAttribute->SetCost(sourceAttribute->GetCost());

		// Transfert du parametrage du fourre-tout
		if (sourceAttribute->GetCatchAllValueSet() != NULL)
			targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());
	}
	ensure(CheckAttributes(targetDataGrid));
}

void KWDataGridManager::ExportOneAttribute(KWDataGrid* targetDataGrid, const ALString& sAttributeName) const
{
	int nTarget;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(sourceDataGrid->SearchAttribute(sAttributeName) != NULL);

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation de la grille cible
	targetDataGrid->Initialize(1, sourceDataGrid->GetTargetValueNumber());

	// Initialisation des valeurs cibles
	for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
	{
		targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
	}

	// Recherche de l'attribut source et cible
	sourceAttribute = sourceDataGrid->SearchAttribute(sAttributeName);
	targetAttribute = targetDataGrid->GetAttributeAt(0);

	// Transfert du parametrage de l'attribut
	targetAttribute->SetAttributeName(sourceAttribute->GetAttributeName());
	targetAttribute->SetAttributeType(sourceAttribute->GetAttributeType());
	targetAttribute->SetAttributeTargetFunction(sourceAttribute->GetAttributeTargetFunction());
	targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());
	targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetGranularizedValueNumber());
	targetAttribute->SetCost(sourceAttribute->GetCost());

	// Transfert du parametrage du fourre-tout
	if (sourceAttribute->GetCatchAllValueSet() != NULL)
		targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());

	ensure(CheckAttributes(targetDataGrid));
}

void KWDataGridManager::ExportInformativeAttributes(KWDataGrid* targetDataGrid) const
{
	int nTarget;
	int nAttribute;
	int nTargetAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation de la grille cible
	targetDataGrid->Initialize(sourceDataGrid->GetInformativeAttributeNumber(),
				   sourceDataGrid->GetTargetValueNumber());

	// Initialisation des valeurs cibles
	for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
	{
		targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
	}

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

			// Transfert des caracteristiques de l'attribut source
			targetAttribute->SetAttributeName(sourceAttribute->GetAttributeName());
			targetAttribute->SetAttributeType(sourceAttribute->GetAttributeType());
			targetAttribute->SetAttributeTargetFunction(sourceAttribute->GetAttributeTargetFunction());
			targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());
			targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetGranularizedValueNumber());
			targetAttribute->SetCost(sourceAttribute->GetCost());

			// Transfert du parametrage du fourre-tout
			if (sourceAttribute->GetCatchAllValueSet() != NULL)
				targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());
		}
	}
	ensure(CheckAttributes(targetDataGrid));
}

void KWDataGridManager::ExportParts(KWDataGrid* targetDataGrid) const
{
	ObjectDictionary odSourceAttributes;
	IntVector ivSourceAttributeIndexes;
	int nAttribute;
	int nSourceAttributeIndex;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid) and CheckGranularity(targetDataGrid));

	// Rangement des attributs sources dans un dictionnaire
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		odSourceAttributes.SetAt(sourceAttribute->GetAttributeName(), sourceAttribute);
	}

	// Rercherche de l'index de l'attribut source correspondant a chaque attribut cible
	ivSourceAttributeIndexes.SetSize(targetDataGrid->GetAttributeNumber());
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant et rangement dans le tableau
		sourceAttribute = cast(KWDGAttribute*, odSourceAttributes.Lookup(targetAttribute->GetAttributeName()));
		check(sourceAttribute);
		ivSourceAttributeIndexes.SetAt(nAttribute, sourceAttribute->GetAttributeIndex());
	}

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'attribut source et cible
		nSourceAttributeIndex = ivSourceAttributeIndexes.GetAt(nAttribute);
		sourceAttribute = sourceDataGrid->GetAttributeAt(nSourceAttributeIndex);
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);
		assert(sourceAttribute->GetAttributeName() == targetAttribute->GetAttributeName());

		// Transfert du parametrage des parties de l'attribut
		sourcePart = sourceAttribute->GetHeadPart();
		while (sourcePart != NULL)
		{
			// Creation de la partie cible
			targetPart = targetAttribute->AddPart();

			// Transfert des valeurs de la partie cible
			if (sourceAttribute->GetAttributeType() == KWType::Continuous)
				targetPart->GetInterval()->CopyFrom(sourcePart->GetInterval());
			else
			{
				targetPart->GetValueSet()->CopyFrom(sourcePart->GetValueSet());
				// Transfert du parametrage du groupe poubelle
				if (sourcePart == sourceAttribute->GetGarbagePart())
					targetAttribute->SetGarbagePart(targetPart);
				// CH RefontePrior2-P-Inside
				// targetPart->SetModalityNumber(sourcePart->GetModalityNumber());
				// targetPart->SetPosition(targetAttribute->GetPartsSizesList()->Add(targetPart));
				// Fin CH RefontePrior2
			}

			// Partie suivante
			sourceAttribute->GetNextPart(sourcePart);
		}
	}
	ensure(CheckParts(targetDataGrid));
}

void KWDataGridManager::ExportPartsForAttribute(KWDataGrid* targetDataGrid, const ALString& sAttributeName) const
{
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid)); // and CheckGranularity(targetDataGrid));
	require(sourceDataGrid->SearchAttribute(sAttributeName) != NULL);
	require(targetDataGrid->SearchAttribute(sAttributeName) != NULL);
	require(targetDataGrid->SearchAttribute(sAttributeName)->GetPartNumber() == 0);

	// Recherche des attributs source et cible
	sourceAttribute = sourceDataGrid->SearchAttribute(sAttributeName);
	targetAttribute = targetDataGrid->SearchAttribute(sAttributeName);

	// Transfert du parametrage des parties de l'attribut
	sourcePart = sourceAttribute->GetHeadPart();
	while (sourcePart != NULL)
	{
		// Creation de la partie cible
		targetPart = targetAttribute->AddPart();

		// Transfert des valeurs de la partie cible
		if (sourceAttribute->GetAttributeType() == KWType::Continuous)
			targetPart->GetInterval()->CopyFrom(sourcePart->GetInterval());
		else
		{
			targetPart->GetValueSet()->CopyFrom(sourcePart->GetValueSet());
			// CH RefontePrior2-P-Inside
			// targetPart->SetModalityNumber(sourcePart->GetModalityNumber());
			// targetPart->SetPosition(targetAttribute->GetPartsSizesList()->Add(targetPart));
			// Fin CH RefontePrior2
		}

		// Partie suivante
		sourceAttribute->GetNextPart(sourcePart);
	}
}

void KWDataGridManager::ExportCells(KWDataGrid* targetDataGrid) const
{
	ObjectDictionary odSourceAttributes;
	IntVector ivSourceAttributeIndexes;
	KWDGCell* sourceCell;
	KWDGCell* targetCell;
	int nAttribute;
	int nSourceAttributeIndex;
	ObjectArray oaTargetParts;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;
	KWDGPart* sourcePart;
	KWDGPart* targetPart;
	Continuous cValue;
	Symbol sValue;

	require(Check());
	require(targetDataGrid != NULL and CheckTargetValues(targetDataGrid) and CheckAttributes(targetDataGrid) and
		CheckParts(targetDataGrid) and targetDataGrid->GetCellNumber() == 0);

	// Rangement des attributs sources dans un dictionnaire
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		odSourceAttributes.SetAt(sourceAttribute->GetAttributeName(), sourceAttribute);
	}

	// Rercherche de l'index de l'attribut source correspondant a chaque attribut cible
	ivSourceAttributeIndexes.SetSize(targetDataGrid->GetAttributeNumber());
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant et rangement dans le tableau
		sourceAttribute = cast(KWDGAttribute*, odSourceAttributes.Lookup(targetAttribute->GetAttributeName()));
		check(sourceAttribute);
		ivSourceAttributeIndexes.SetAt(nAttribute, sourceAttribute->GetAttributeIndex());
	}

	// Passage de la grille cible en mode update
	targetDataGrid->SetCellUpdateMode(true);
	targetDataGrid->BuildIndexingStructure();
	oaTargetParts.SetSize(targetDataGrid->GetAttributeNumber());

	// Transfert des cellules sources
	sourceCell = sourceDataGrid->GetHeadCell();
	while (sourceCell != NULL)
	{
		// Recherche des parties cible pour les valeurs de la cellule courante
		for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
		{
			targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

			// Index de l'attribut source associe
			nSourceAttributeIndex = ivSourceAttributeIndexes.GetAt(nAttribute);
			assert(sourceDataGrid->GetAttributeAt(nSourceAttributeIndex)->GetAttributeName() ==
			       targetAttribute->GetAttributeName());

			// Recherche de la partie associee a la cellule selon son type
			sourcePart = sourceCell->GetPartAt(nSourceAttributeIndex);
			if (sourcePart->GetPartType() == KWType::Continuous)
			{
				// Recherche d'une valeur typique: le milieu de l'intervalle (hors borne inf)
				cValue = KWContinuous::GetUpperMeanValue(sourcePart->GetInterval()->GetLowerBound(),
									 sourcePart->GetInterval()->GetUpperBound());

				// Recherche de l'intervalle cible correspondant
				targetPart = targetAttribute->LookupContinuousPart(cValue);
				oaTargetParts.SetAt(nAttribute, targetPart);
			}
			else
			{
				// Recherche d'une valeur typique: la premiere valeur
				assert(sourcePart->GetValueSet()->GetHeadValue() != NULL);
				sValue = sourcePart->GetValueSet()->GetHeadValue()->GetValue();

				// Recherche du groupe de valeurs cible correspondant
				targetPart = targetAttribute->LookupSymbolPart(sValue);
				oaTargetParts.SetAt(nAttribute, targetPart);
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
	int nTarget;
	int nSourceAttribute;
	int nTargetAttribute;
	IntVector ivSourceAttributeIndexes;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(0 <= nAttributeNumber and nAttributeNumber <= sourceDataGrid->GetAttributeNumber());
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation de la grille cible
	targetDataGrid->Initialize(nAttributeNumber, sourceDataGrid->GetTargetValueNumber());

	// Initialisation des valeurs cibles
	for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
	{
		targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
	}

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
		targetAttribute->SetAttributeName(sourceAttribute->GetAttributeName());
		targetAttribute->SetAttributeType(sourceAttribute->GetAttributeType());
		targetAttribute->SetAttributeTargetFunction(sourceAttribute->GetAttributeTargetFunction());
		targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetGranularizedValueNumber());
		targetAttribute->SetCost(sourceAttribute->GetCost());

		// Transfert du parametrage du fourre-tout
		if (sourceAttribute->GetCatchAllValueSet() != NULL)
			targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());
	}
	ensure(CheckAttributes(targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
}

void KWDataGridManager::ExportRandomParts(KWDataGrid* targetDataGrid, int nMeanAttributePartNumber) const
{
	ObjectDictionary odSourceAttributes;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid) and CheckGranularity(targetDataGrid));
	require(1 <= nMeanAttributePartNumber and nMeanAttributePartNumber <= sourceDataGrid->GetGridFrequency());

	// Rangement des attributs sources dans un dictionnaire
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		odSourceAttributes.SetAt(sourceAttribute->GetAttributeName(), sourceAttribute);
	}

	// Initialisation des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche des attributs cible et source
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);
		sourceAttribute = cast(KWDGAttribute*, odSourceAttributes.Lookup(targetAttribute->GetAttributeName()));

		// Export d'un sous ensemble de parties de l'attribut
		ExportRandomAttributeParts(targetDataGrid, sourceAttribute, targetAttribute, nMeanAttributePartNumber);
	}
	ensure(CheckParts(targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
}

void KWDataGridManager::ExportRandomAttributeParts(KWDataGrid* targetDataGrid, KWDGAttribute* sourceAttribute,
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
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid) and CheckGranularity(targetDataGrid));
	require(sourceAttribute != NULL);
	require(targetAttribute != NULL);
	require(sourceAttribute->GetAttributeName() == targetAttribute->GetAttributeName());
	require(sourceAttribute->GetAttributeType() == targetAttribute->GetAttributeType());
	require(sourceAttribute->GetAttributeTargetFunction() == targetAttribute->GetAttributeTargetFunction());
	require(targetAttribute->GetPartNumber() == 0);
	require(1 <= nPartNumber and nPartNumber <= sourceDataGrid->GetGridFrequency());

	// Partition aleatoire des bornes des intervalles (en rangs) dans le cas continu
	if (sourceAttribute->GetAttributeType() == KWType::Continuous)
	{
		// Export des parties de l'attribut source
		sourceAttribute->ExportParts(&oaSourceParts);

		// Tri des intervalles source par borne inf croissante
		oaSourceParts.SetCompareFunction(KWDGPartContinuousCompare);
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
	// Partition aleatoire des valeurs dans le cas symbolique
	else
	{
		// Recopie du fourre-tout
		// Transfert du parametrage du fourre-tout
		if (sourceAttribute->GetCatchAllValueSet() != NULL)
			targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());

		// S'il y a moins de valeurs que de partie a constituer, on recopie directement
		// les meme parties
		if (sourceAttribute->GetPartNumber() <= nPartNumber)
		{
			// Transfert du parametrage des parties de l'attribut
			sourcePart = sourceAttribute->GetHeadPart();
			while (sourcePart != NULL)
			{
				// Creation de la partie cible
				targetPart = targetAttribute->AddPart();

				// Transfert des valeurs de la partie cible
				targetPart->GetValueSet()->CopyFrom(sourcePart->GetValueSet());

				// Partie suivante
				sourceAttribute->GetNextPart(sourcePart);
			}
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

void KWDataGridManager::AddRandomAttributes(KWDataGrid* targetDataGrid, const KWDataGrid* mandatoryDataGrid,
					    int nRequestedAttributeNumber) const
{
	ObjectDictionary odMandatoryAttributes;
	int nTarget;
	int nSourceAttribute;
	int nTargetAttribute;
	int nAttributeNumber;
	int nAttribute;
	IntVector ivSourceAttributeIndexes;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* mandatoryAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(0 <= nRequestedAttributeNumber and nRequestedAttributeNumber <= sourceDataGrid->GetAttributeNumber());
	require(mandatoryDataGrid != NULL);
	require(CheckAttributes(mandatoryDataGrid));
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation de la grille cible a partir de la grille initiale
	nAttributeNumber = mandatoryDataGrid->GetAttributeNumber();
	if (nAttributeNumber < nRequestedAttributeNumber)
		nAttributeNumber = nRequestedAttributeNumber;
	targetDataGrid->Initialize(nAttributeNumber, sourceDataGrid->GetTargetValueNumber());

	// Initialisation des valeurs cibles
	for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
	{
		targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
	}

	// Rangement des attributs obligatoires dans un dictionnaire
	for (nAttribute = 0; nAttribute < mandatoryDataGrid->GetAttributeNumber(); nAttribute++)
	{
		mandatoryAttribute = mandatoryDataGrid->GetAttributeAt(nAttribute);
		odMandatoryAttributes.SetAt(mandatoryAttribute->GetAttributeName(), mandatoryAttribute);
	}

	// Creation d'un vecteur d'index d'attributs cibles choisis aleatoirement,
	// parmi les attribut non deja present dans les attributs obligatoires
	for (nSourceAttribute = 0; nSourceAttribute < sourceDataGrid->GetAttributeNumber(); nSourceAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nSourceAttribute);
		if (odMandatoryAttributes.Lookup(sourceAttribute->GetAttributeName()) == NULL)
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
		if (odMandatoryAttributes.Lookup(sourceAttribute->GetAttributeName()) != NULL)
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
		targetAttribute->SetAttributeName(sourceAttribute->GetAttributeName());
		targetAttribute->SetAttributeType(sourceAttribute->GetAttributeType());
		targetAttribute->SetAttributeTargetFunction(sourceAttribute->GetAttributeTargetFunction());
		targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());
		targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetGranularizedValueNumber());
		targetAttribute->SetCost(sourceAttribute->GetCost());

		// Transfert du parametrage du fourre-tout
		if (sourceAttribute->GetCatchAllValueSet() != NULL)
			targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());
	}
	ensure(CheckAttributes(targetDataGrid));
	ensure(targetDataGrid->GetAttributeNumber() >= mandatoryDataGrid->GetAttributeNumber());
	ensure(targetDataGrid->GetAttributeNumber() >= nRequestedAttributeNumber);
	ensure(targetDataGrid->GetCellNumber() == 0);
}

void KWDataGridManager::AddRandomParts(KWDataGrid* targetDataGrid, const KWDataGrid* mandatoryDataGrid,
				       int nRequestedContinuousPartNumber, int nRequestedSymbolPartNumber,
				       double dMinPercentageAddedPart) const
{
	ObjectDictionary odSourceAttributes;
	ObjectDictionary odMandatoryAttributes;
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

	// Rangement des attributs sources dans un dictionnaire
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		odSourceAttributes.SetAt(sourceAttribute->GetAttributeName(), sourceAttribute);
	}

	// Rangement des attributs obligatoires dans un dictionnaire
	for (nAttribute = 0; nAttribute < mandatoryDataGrid->GetAttributeNumber(); nAttribute++)
	{
		mandatoryAttribute = mandatoryDataGrid->GetAttributeAt(nAttribute);
		odMandatoryAttributes.SetAt(mandatoryAttribute->GetAttributeName(), mandatoryAttribute);
	}

	// Ajout des parties des attributs
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche des attributs cible, initial et source
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);
		mandatoryAttribute =
		    cast(KWDGAttribute*, odMandatoryAttributes.Lookup(targetAttribute->GetAttributeName()));
		sourceAttribute = cast(KWDGAttribute*, odSourceAttributes.Lookup(targetAttribute->GetAttributeName()));

		// Verifications d'integrite
		check(sourceAttribute);
		assert(sourceAttribute->GetAttributeType() == targetAttribute->GetAttributeType());

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
			AddRandomAttributeParts(targetDataGrid, sourceAttribute, mandatoryAttribute, targetAttribute,
						nRequestedPartNumber);
		}
		// et dans le cas general sinon
		else
		{
			ExportRandomAttributeParts(targetDataGrid, sourceAttribute, targetAttribute,
						   nRequestedPartNumber);
		}
	}
	ensure(CheckParts(targetDataGrid));
	ensure(targetDataGrid->GetCellNumber() == 0);
}

void KWDataGridManager::AddRandomAttributeParts(KWDataGrid* targetDataGrid, KWDGAttribute* sourceAttribute,
						KWDGAttribute* mandatoryAttribute, KWDGAttribute* targetAttribute,
						int nRequestedPartNumber) const
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
	require(targetDataGrid != NULL and CheckAttributes(targetDataGrid) and CheckGranularity(targetDataGrid));
	require(sourceAttribute != NULL);
	require(mandatoryAttribute != NULL);
	require(targetAttribute != NULL);
	require(sourceAttribute->GetAttributeName() == mandatoryAttribute->GetAttributeName());
	require(sourceAttribute->GetAttributeName() == targetAttribute->GetAttributeName());
	require(sourceAttribute->GetAttributeType() == mandatoryAttribute->GetAttributeType());
	require(sourceAttribute->GetAttributeType() == targetAttribute->GetAttributeType());
	require(targetAttribute->GetPartNumber() == 0);
	require(1 <= nRequestedPartNumber and nRequestedPartNumber <= sourceDataGrid->GetGridFrequency());

	// Calcul du nombre de partie supplementaires a ajouter
	nAddedPartNumber = nRequestedPartNumber;

	// Cas particulier: il n'y avait pas de parties dans l'attribut obligatoire
	if (mandatoryAttribute->GetPartNumber() <= 1)
	{
		// Export du nombre de parties demandee (d'au moins une en fait)
		ExportRandomAttributeParts(targetDataGrid, sourceAttribute, targetAttribute, nRequestedPartNumber);
	}
	// Cas particulier: il y a deja assez de partie dans l'attribut obligatoire
	else if (nAddedPartNumber == 0)
	{
		// Transfert du parametrage des parties de l'attribut
		mandatoryPart = mandatoryAttribute->GetHeadPart();
		while (mandatoryPart != NULL)
		{
			// Creation de la partie cible
			targetPart = targetAttribute->AddPart();

			// Transfert des valeurs de la partie cible
			if (mandatoryAttribute->GetAttributeType() == KWType::Continuous)
				targetPart->GetInterval()->CopyFrom(mandatoryPart->GetInterval());
			else
				targetPart->GetValueSet()->CopyFrom(mandatoryPart->GetValueSet());

			// Partie suivante
			mandatoryAttribute->GetNextPart(mandatoryPart);
		}
	}
	// Partition aleatoire des bornes des intervalles (en rangs) dans le cas continu
	else if (sourceAttribute->GetAttributeType() == KWType::Continuous)
	{
		// Export des parties de l'attribut source
		sourceAttribute->ExportParts(&oaSourceParts);
		oaSourceParts.SetCompareFunction(KWDGPartContinuousCompare);
		oaSourceParts.Sort();

		// Export des parties de l'attribut obligatoire
		mandatoryAttribute->ExportParts(&oaMandatoryParts);
		oaMandatoryParts.SetCompareFunction(KWDGPartContinuousCompare);
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
	// Partition aleatoire des valeurs dans le cas symbolique
	else
	{
		// S'il y a moins de valeurs que de partie a constituer, on recopie directement
		// les meme parties
		if (sourceAttribute->GetPartNumber() <= mandatoryAttribute->GetPartNumber() + nAddedPartNumber)
		{
			// Transfert du parametrage des parties de l'attribut
			sourcePart = sourceAttribute->GetHeadPart();
			while (sourcePart != NULL)
			{
				// Creation de la partie cible
				targetPart = targetAttribute->AddPart();

				// Transfert des valeurs de la partie cible
				targetPart->GetValueSet()->CopyFrom(sourcePart->GetValueSet());

				// Partie suivante
				sourceAttribute->GetNextPart(sourcePart);
			}
		}
		// Sinon, partitionnement aleatoire des parties sources
		else
		{
			// Tri des parties sources synchronisee selon les parties mandatorys
			SortAttributeParts(sourceAttribute, mandatoryAttribute, &oaSourceParts, &oaMandatoryParts);

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

void KWDataGridManager::BuildDataGridFromUnivariateStats(KWDataGrid* targetDataGrid,
							 KWAttributeStats* attributeStats) const
{
	int nTarget;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(sourceDataGrid->GetTargetValueNumber() > 0);
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(attributeStats != NULL);
	require(attributeStats->GetAttributeType() == KWType::Symbol or
		attributeStats->GetAttributeType() == KWType::Continuous);
	require(sourceDataGrid->SearchAttribute(attributeStats->GetAttributeName()) != NULL);
	require(attributeStats->GetPreparedDataGridStats()->GetSourceAttributeNumber() == 1);

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

	// Initialisation de la grille cible avec le nombre d'attributs demandes
	targetDataGrid->Initialize(1, sourceDataGrid->GetTargetValueNumber());

	// Initialisation des valeurs cibles
	for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
	{
		targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
	}

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
	int nTarget;
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

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

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
			nkdBestAttributeStats.SetAt((NUMERIC)oaAllAttributeStats.GetAt(nAttribute),
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
		    (bSmallSourceDataGrid or nkdBestAttributeStats.Lookup((NUMERIC)attributeStats) != NULL))
			oaTargetAttributeStats.Add(attributeStats);

		// Arret si grille cible complete
		if (oaTargetAttributeStats.GetSize() == nAttributeNumber)
			break;
	}

	// Creation de la grille si au moins dex attributs
	bOk = oaTargetAttributeStats.GetSize() >= 2;
	if (bOk)
	{
		// Initialisation de la grille cible avec le nombre d'attributs demandes
		targetDataGrid->Initialize(oaTargetAttributeStats.GetSize(), sourceDataGrid->GetTargetValueNumber());

		// Initialisation des valeurs cibles
		for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
		{
			targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
		}

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
	int nTarget;
	int nAttribute;
	boolean bOk = true;
	boolean bSmallSourceDataGrid;
	boolean bDisplayResults = false;

	require(Check());
	require(sourceDataGrid->GetTargetValueNumber() > 0);
	require(targetDataGrid != NULL and targetDataGrid->IsEmpty());
	require(classStats != NULL);
	require(classStats->GetInformativeAttributeNumber() > 0);

	// Export de la granularite
	targetDataGrid->SetGranularity(sourceDataGrid->GetGranularity());

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
			nkdBestAttributeStats.SetAt((NUMERIC)oaAllAttributeStats.GetAt(nAttribute),
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
		    (bSmallSourceDataGrid or nkdBestAttributeStats.Lookup((NUMERIC)attributeStats) != NULL))
			oaSelectedAtttributes.Add(sourceAttribute);

		// Arret si grille cible complete
		if (oaSelectedAtttributes.GetSize() == nAttributeNumber)
			break;
	}

	// Creation de la grille si au moins dex attributs
	bOk = oaSelectedAtttributes.GetSize() >= 2;
	if (bOk)
	{
		// Initialisation de la grille cible avec le nombre d'attributs demandes
		targetDataGrid->Initialize(oaSelectedAtttributes.GetSize(), sourceDataGrid->GetTargetValueNumber());

		// Initialisation des valeurs cibles
		for (nTarget = 0; nTarget < sourceDataGrid->GetTargetValueNumber(); nTarget++)
		{
			targetDataGrid->SetTargetValueAt(nTarget, sourceDataGrid->GetTargetValueAt(nTarget));
		}

		// Creation des partitions
		for (nAttribute = 0; nAttribute < oaSelectedAtttributes.GetSize(); nAttribute++)
		{
			// Extraction attribut initial granularise
			sourceAttribute = cast(KWDGAttribute*, oaSelectedAtttributes.GetAt(nAttribute));

			targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);
			// Transfert du parametrage de l'attribut
			targetAttribute->SetAttributeName(sourceAttribute->GetAttributeName());
			targetAttribute->SetAttributeType(sourceAttribute->GetAttributeType());
			targetAttribute->SetAttributeTargetFunction(sourceAttribute->GetAttributeTargetFunction());
			targetAttribute->SetInitialValueNumber(sourceAttribute->GetInitialValueNumber());
			targetAttribute->SetGranularizedValueNumber(sourceAttribute->GetGranularizedValueNumber());
			targetAttribute->SetCost(sourceAttribute->GetCost());

			// Transfert du parametrage du fourre-tout
			if (sourceAttribute->GetCatchAllValueSet() != NULL)
				targetAttribute->InitializeCatchAllValueSet(sourceAttribute->GetCatchAllValueSet());

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
	oaSourceParts.SetCompareFunction(KWDGPartContinuousCompare);
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
		if (nGarbageModalityNumber > 0 and targetPart->GetValueSet()->GetTrueValueNumber() > nMaxValueNumber)
		{
			targetAttribute->SetGarbagePart(targetPart);
			nMaxValueNumber = targetPart->GetValueSet()->GetTrueValueNumber();
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
	KWDGValueSet* valueSet;
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
			valueSet = part->GetValueSet();

			// Initialisation des valeurs du groupe
			for (nValue = attributeGrouping->GetGroupFirstValueIndexAt(nPart);
			     nValue <= attributeGrouping->GetGroupLastValueIndexAt(nPart); nValue++)
				valueSet->AddValue(attributeGrouping->GetValueAt(nValue));

			// Memorisation du groupe poubelle
			if (nPart == attributeGrouping->GetGarbageGroupIndex())
				targetAttribute->SetGarbagePart(part);
		}

		// Export des effectif des valeurs de la grille initiale pour finaliser la specification
		ExportSymbolAttributeValueFrequencies(targetAttribute);
	}
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
		ExportFrequencyTableFromOneAttribute(discretizerMODL->GetFrequencyVectorCreator(), kwftSource,
						     sourceAttribute->GetAttributeName());

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
		ExportFrequencyTableFromOneAttribute(grouperMODL->GetFrequencyVectorCreator(), kwftSource,
						     sourceAttribute->GetAttributeName());

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
		ExportSymbolAttributeValueFrequencies(targetAttribute);

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
	ALString sTmp;
	int nTarget;

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
	ALString sTmp;
	ObjectDictionary odSourceAttributes;
	int nAttribute;
	KWDGAttribute* sourceAttribute;
	KWDGAttribute* targetAttribute;

	require(Check());
	require(targetDataGrid != NULL);

	// Rangement des attributs source dans un dictionnaire
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		odSourceAttributes.SetAt(sourceAttribute->GetAttributeName(), sourceAttribute);
	}

	// Rercherche d'un attribut source correspondant a chaque attribut cible
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = cast(KWDGAttribute*, odSourceAttributes.Lookup(targetAttribute->GetAttributeName()));

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
	ALString sTmp;
	ObjectDictionary odSourceAttributes;
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

	require(Check());
	require(targetDataGrid != NULL);
	require(targetDataGrid->Check());
	require(CheckAttributes(targetDataGrid));

	// Rangement des attributs source dans un dictionnaire
	for (nAttribute = 0; nAttribute < sourceDataGrid->GetAttributeNumber(); nAttribute++)
	{
		sourceAttribute = sourceDataGrid->GetAttributeAt(nAttribute);
		odSourceAttributes.SetAt(sourceAttribute->GetAttributeName(), sourceAttribute);
	}

	// Rercherche d'un attribut source correspondant a chaque attribut cible
	for (nAttribute = 0; nAttribute < targetDataGrid->GetAttributeNumber(); nAttribute++)
	{
		targetAttribute = targetDataGrid->GetAttributeAt(nAttribute);

		// Recherche de l'attribut source correspondant
		sourceAttribute = cast(KWDGAttribute*, odSourceAttributes.Lookup(targetAttribute->GetAttributeName()));
		assert(sourceAttribute != NULL);
		assert(targetAttribute->GetAttributeType() == sourceAttribute->GetAttributeType());

		// Comparaison des intervalles dans le cas numerique
		if (targetAttribute->GetAttributeType() == KWType::Continuous)
		{
			// Ajout des parties source numeriques dans un tableau d'intervalles
			sourceAttribute->ExportParts(&oaSourceIntervals);
			oaSourceIntervals.SetCompareFunction(KWDGPartContinuousCompare);
			oaSourceIntervals.Sort();

			// Ajout des parties cibles numeriques dans un tableau d'intervalles
			targetAttribute->ExportParts(&oaTargetIntervals);
			oaTargetIntervals.SetCompareFunction(KWDGPartContinuousCompare);
			oaTargetIntervals.Sort();

			// Parcours des intervalles cibles pour verifier leur compatibilite
			// avec les intervalles source
			// Attention: si des attribut ont ete fabrique en utilisant (recursivement ou non)
			// la regles de derivation Random, les bornes des intervalles ne sont pas necessaire les
			// meme lors de la lecture en univarie ou en bivarie, ce qui entraine des erreurs
			// Ce cas est neanmoins rare, et l'erreur n'a lieu qu'en mode debug: on ne la
			// corrige pas (cela n'a en fait pas de consequence pour l'utilisateur:
			// la methode CheckParts est utille afin de test d'integrite pour le developpeur,
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
				headTargetPart = targetAttribute->LookupSymbolPart(sourceValue->GetValue());
				check(headTargetPart);

				// Parcours des valeurs de la partie source
				sourceValue = sourceValueSet->GetHeadValue();
				while (sourceValue != NULL)
				{
					// Recherche de la partie cible associee a la valeur source
					targetPart = targetAttribute->LookupSymbolPart(sourceValue->GetValue());
					check(targetPart);

					// Erreur si la partie est differente de la premiere partie
					// Tolerance pour la valeur speciale
					if (targetPart != headTargetPart and
					    sourceValue->GetValue() != Symbol::GetStarValue())
					{
						sourcePart->AddError(sTmp + "Input value (" + sourceValue->GetValue() +
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
	ALString sTmp;
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
	Symbol sValue;

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
					// Recherche d'une valeur typique: la premiere valeur
					assert(targetPart->GetValueSet()->GetHeadValue() != NULL);
					sValue = targetPart->GetValueSet()->GetHeadValue()->GetValue();

					// Recherche du groupe de valeurs cible correspondant
					checkPart = checkAttribute->LookupSymbolPart(sValue);
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
	return sourceDataGrid != NULL and sourceDataGrid->Check();
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

	dataGridManager.InitializeQuantileBuildersBeforeGranularization(&odQuantileBuilders, &ivMaxPartNumbers);
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

void KWDataGridManager::ExportSymbolAttributeValueFrequencies(KWDGAttribute* targetAttribute) const
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
			nkdSourceValues.SetAt(value->GetValue().GetNumericKey(), value);
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
			sourceValue = cast(KWDGValue*, nkdSourceValues.Lookup(value->GetValue().GetNumericKey()));

			// Cas ou la sourceValue est bien presente
			// Dans le cas particulier ou sourceAttribute provient d'une grille construite a partir d'un
			// KWAtttributeStats, certaines modalites peuvent etre manquantes. En effet lors de la
			// construction de la grille de preparation (methode BuildPreparedGroupingDataGridStats), les
			// modalites du fourre-tout sont resumees par une modalite + StarValue
			if (sourceValue != NULL)
			{
				value->SetValueFrequency(sourceValue->GetValueFrequency());

				// On test si on est sur la valeur par defaut
				if (value->GetValue() == Symbol::GetStarValue())
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

void KWDataGridManager::SortAttributeParts(KWDGAttribute* sourceAttribute, KWDGAttribute* groupedAttribute,
					   ObjectArray* oaSortedSourceParts, ObjectArray* oaSortedGroupedParts) const
{
	boolean bIsIndexed;
	ObjectArray oaSourceParts;
	ObjectArray oaAssociations;
	KWSortableSymbol* association;
	int nSource;
	int n;
	KWDGPart* sourcePart;
	KWDGPart* groupedPart;

	require(sourceAttribute != NULL);
	require(groupedAttribute != NULL);
	require(sourceAttribute->Check());
	require(groupedAttribute->Check());
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
		groupedPart = groupedAttribute->LookupSymbolPart(sourcePart->GetValueSet()->GetHeadValue()->GetValue());

		// Creation de l'association entre index de partie et premiere valeur du groupe
		association = new KWSortableSymbol;
		oaAssociations.SetAt(nSource, association);
		association->SetIndex(nSource);
		association->SetSortValue(groupedPart->GetValueSet()->GetHeadValue()->GetValue());
	}

	// Tri des association, apres une randomisation pour avoir un ordre aleatoire par groupe
	oaAssociations.Shuffle();
	oaAssociations.SetCompareFunction(KWSortableSymbolCompareValue);
	oaAssociations.Sort();

	// On range dans le tableau en sortie les parties sources, triees par groupe
	// (en fait, par leur premiere valeur, ce qui est equivalent), et leur groupe associe
	oaSortedSourceParts->SetSize(oaSourceParts.GetSize());
	oaSortedGroupedParts->SetSize(oaSourceParts.GetSize());
	for (n = 0; n < oaAssociations.GetSize(); n++)
	{
		association = cast(KWSortableSymbol*, oaAssociations.GetAt(n));

		// Recherche de la partie source
		nSource = association->GetIndex();
		sourcePart = cast(KWDGPart*, oaSourceParts.GetAt(nSource));

		// Recherche de la partie groupee correspondante
		groupedPart = groupedAttribute->LookupSymbolPart(sourcePart->GetValueSet()->GetHeadValue()->GetValue());

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