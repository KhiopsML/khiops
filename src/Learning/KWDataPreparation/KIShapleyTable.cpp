// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIShapleyTable.h"

KIShapleyTable::KIShapleyTable()
{
	nTableSourceSize = 0;
	nTableTargetSize = 0;
}

KIShapleyTable::~KIShapleyTable() {}

void KIShapleyTable::InitializeFromDataGridStats(const KWDataGridStats* attributeDataGridStats,
						 const KWDataGridStats* targetDataGridStats, double dAttributeWeight)
{
	KWDataGridStats univariateDataGrid;
	const KWDataGridStats* workingDataGrid;

	require(attributeDataGridStats != NULL);
	require(attributeDataGridStats->Check());
	require(attributeDataGridStats->GetAttributeNumber() == 2 or attributeDataGridStats->GetAttributeNumber() == 3);
	require(attributeDataGridStats->GetAttributeAt(attributeDataGridStats->GetAttributeNumber() - 1)
		    ->GetAttributeType() == KWType::Symbol);
	require(targetDataGridStats != NULL);
	require(targetDataGridStats->Check());
	require(targetDataGridStats->GetAttributeNumber() == 1);
	require(targetDataGridStats->GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
	require(attributeDataGridStats->ComputeGridFrequency() == targetDataGridStats->ComputeGridFrequency());
	require(0 < dAttributeWeight);

	// Cas univarie: on utilise la grille telle quelle
	if (attributeDataGridStats->GetAttributeNumber() == 2)
		workingDataGrid = attributeDataGridStats;
	// Cas bivarie: on passe par une grille univarie interprediaire, construiote a partir le la grille bivariee
	else
	{
		BuildUnivariateDataGridStats(attributeDataGridStats, &univariateDataGrid);
		workingDataGrid = &univariateDataGrid;
	}

	// Appel de la methode univariee avec la grille univariee de travail
	InitializeFromUnivariateDataGridStats(workingDataGrid, targetDataGridStats, dAttributeWeight);
}

double KIShapleyTable::ComputeMeanAbsoluteShapleyValues(const KWDataGridStats* attributeDataGridStats,
							const KWDataGridStats* targetDataGridStats,
							double dAttributeWeight)
{
	const boolean bTrace = false;
	double dMeanAbsoluteShapleyValues;
	double dMeanAbsoluteShapleyValuesPerTarget;
	KWDataGridStats workingUnivariateDataGridStats;
	KWDataGridStats workingTargetDataGridStats;
	const KWDataGridStats* currentAttributeDataGridStats;
	KIShapleyTable shapleyTable;
	IntVector ivSourcePartFrequencies;
	IntVector ivTargetPartFrequencies;
	int nTotalFrequency;
	int nSourcePartNumber;
	int nTargetPartNumber;
	int nSourcePart;
	int nTarget;
	int nSingletonIndex;

	require(attributeDataGridStats != NULL);
	require(attributeDataGridStats->GetAttributeNumber() == 2 or attributeDataGridStats->GetAttributeNumber() == 3);
	require(targetDataGridStats != NULL);
	require(0 < dAttributeWeight);

	// Cas de la classification
	currentAttributeDataGridStats = NULL;
	if (attributeDataGridStats->GetAttributeAt(attributeDataGridStats->GetAttributeNumber() - 1)
		->GetAttributeType() == KWType::Symbol)
	{
		// Cas univarie: on utilise la grille telle quelle
		if (attributeDataGridStats->GetAttributeNumber() == 2)
			currentAttributeDataGridStats = attributeDataGridStats;
		// Cas bivarie: on passe par une grille univarie intermediaire, construite a partir de la grille bivariee
		else
		{
			assert(attributeDataGridStats->GetAttributeNumber() == 3);
			assert(attributeDataGridStats->GetAttributeAt(2)->GetAttributeType() == KWType::Symbol);
			shapleyTable.BuildUnivariateDataGridStats(attributeDataGridStats,
								  &workingUnivariateDataGridStats);
			currentAttributeDataGridStats = &workingUnivariateDataGridStats;
		}

		// Appel de la methode univariee avec la grille univariee courante
		shapleyTable.InitializeFromUnivariateDataGridStats(currentAttributeDataGridStats, targetDataGridStats,
								   dAttributeWeight);
		assert(shapleyTable.GetSourceSize() ==
		       currentAttributeDataGridStats->GetAttributeAt(0)->GetPartNumber());
		assert(shapleyTable.GetTargetSize() == targetDataGridStats->GetAttributeAt(0)->GetPartNumber());

		// Calcul des effectifs par partie pour l'attribut source et cible de la grille de travail
		currentAttributeDataGridStats->ExportAttributePartFrequenciesAt(0, &ivSourcePartFrequencies);
		currentAttributeDataGridStats->ExportAttributePartFrequenciesAt(1, &ivTargetPartFrequencies);
		nSourcePartNumber = ivSourcePartFrequencies.GetSize();
		nTargetPartNumber = ivTargetPartFrequencies.GetSize();
		nTotalFrequency = targetDataGridStats->ComputeGridFrequency();

		// Calcul pondere sur l'ensemble des valeur cibles
		dMeanAbsoluteShapleyValues = 0;
		for (nTarget = 0; nTarget < nTargetPartNumber; nTarget++)
		{
			// Calcul de la moyennne des valeur absolue de Shapley pour une valeur cible donnees
			dMeanAbsoluteShapleyValuesPerTarget = 0;
			for (nSourcePart = 0; nSourcePart < shapleyTable.GetSourceSize(); nSourcePart++)
			{
				dMeanAbsoluteShapleyValuesPerTarget +=
				    ivSourcePartFrequencies.GetAt(nSourcePart) *
				    abs(shapleyTable.GetShapleyValueAt(nSourcePart, nTarget));
			}
			dMeanAbsoluteShapleyValuesPerTarget /= nTotalFrequency;

			// Prise en compte de l'effectif de la cible
			dMeanAbsoluteShapleyValues +=
			    ivTargetPartFrequencies.GetAt(nTarget) * dMeanAbsoluteShapleyValuesPerTarget;
		}
		dMeanAbsoluteShapleyValues /= nTotalFrequency;
	}
	// Cas de la regression
	else
	{
		assert(attributeDataGridStats->GetAttributeNumber() == 2);
		assert(attributeDataGridStats->GetAttributeAt(1)->GetAttributeType() == KWType::Continuous);

		// On passe transforme la grille de regression en grille de classification avec groupement
		// des valeurs cibles, avec par intervalle une valeur singleton representative d'un rang
		// de l'intervalle et une seconde valeur contenant le reste de l'intervalle
		shapleyTable.BuildRegressionAnalysisDataGridStats(
		    attributeDataGridStats, &workingUnivariateDataGridStats, &workingTargetDataGridStats);
		currentAttributeDataGridStats = &workingUnivariateDataGridStats;
		assert(2 * workingUnivariateDataGridStats.GetAttributeAt(1)->GetPartNumber() ==
		       workingTargetDataGridStats.GetAttributeAt(0)->GetPartNumber());

		// Appel de la methode univariee avec la grille univariee courante
		shapleyTable.InitializeFromUnivariateDataGridStats(currentAttributeDataGridStats,
								   &workingTargetDataGridStats, dAttributeWeight);
		assert(shapleyTable.GetSourceSize() ==
		       workingUnivariateDataGridStats.GetAttributeAt(0)->GetPartNumber());
		assert(shapleyTable.GetTargetSize() == workingTargetDataGridStats.GetAttributeAt(0)->GetPartNumber());

		// Calcul des effectifs par partie pour l'attribut source et cible de la grille de travail
		currentAttributeDataGridStats->ExportAttributePartFrequenciesAt(0, &ivSourcePartFrequencies);
		currentAttributeDataGridStats->ExportAttributePartFrequenciesAt(1, &ivTargetPartFrequencies);
		nSourcePartNumber = ivSourcePartFrequencies.GetSize();
		nTargetPartNumber = ivTargetPartFrequencies.GetSize();
		nTotalFrequency = targetDataGridStats->ComputeGridFrequency();

		// Calcul pondere sur l'ensemble de intervalles cibles, en exploitant
		dMeanAbsoluteShapleyValues = 0;
		for (nTarget = 0; nTarget < nTargetPartNumber; nTarget++)
		{
			// Calcul de la moyennne des valeur absolue de Shapley pour un rang d'un interval donne
			dMeanAbsoluteShapleyValuesPerTarget = 0;
			for (nSourcePart = 0; nSourcePart < shapleyTable.GetSourceSize(); nSourcePart++)
			{
				// Acces au rang de la partie singleton representative d'un intervalle cible
				nSingletonIndex = 2 * nTarget;
				assert(workingTargetDataGridStats.GetUnivariateCellFrequencyAt(nSingletonIndex) == 1);
				assert(
				    workingTargetDataGridStats.GetUnivariateCellFrequencyAt(nSingletonIndex) +
					workingTargetDataGridStats.GetUnivariateCellFrequencyAt(nSingletonIndex + 1) ==
				    ivTargetPartFrequencies.GetAt(nTarget));

				// Calcul de la valeur de Shapley pour ce singleton
				dMeanAbsoluteShapleyValuesPerTarget +=
				    ivSourcePartFrequencies.GetAt(nSourcePart) *
				    abs(shapleyTable.GetShapleyValueAt(nSourcePart, nSingletonIndex));
			}
			dMeanAbsoluteShapleyValuesPerTarget /= nTotalFrequency;

			// Prise en compte de l'effectif de tout l'intervalle cible, dont tous les rang
			// se comportent de la meme facon
			dMeanAbsoluteShapleyValues +=
			    ivTargetPartFrequencies.GetAt(nTarget) * dMeanAbsoluteShapleyValuesPerTarget;
		}
		dMeanAbsoluteShapleyValues /= nTotalFrequency;
	}
	assert(currentAttributeDataGridStats != NULL);
	assert(currentAttributeDataGridStats->GetSourceAttributeNumber() == 1);

	// Trace
	if (bTrace)
	{
		cout << "ComputeMeanAbsoluteShapleyValues\t";
		cout << currentAttributeDataGridStats->GetAttributeAt(0)->GetAttributeName() << "\t"
		     << currentAttributeDataGridStats->GetAttributeAt(1)->GetAttributeName() << "\t";
		cout << dAttributeWeight << "\t";
		cout << dMeanAbsoluteShapleyValues << "\n";
		currentAttributeDataGridStats->WriteCellArrayLineReport(cout);
		cout << shapleyTable << "\n";
	}
	ensure(dMeanAbsoluteShapleyValues > 0);
	return dMeanAbsoluteShapleyValues;
}

void KIShapleyTable::Initialize(int nSourceSize, int nTargetSize)
{
	require(nSourceSize >= 0);
	require(nTargetSize >= 0);

	// Memorisation des catacteristiques de la table
	nTableSourceSize = nSourceSize;
	nTableTargetSize = nTargetSize;

	// Creation des valeurs (apres reinitialisation)
	cvTableValues.SetSize(0);
	cvTableValues.SetSize(nTableSourceSize * nTableTargetSize);
}

void KIShapleyTable::CopyFrom(const KIShapleyTable* kwptSource)
{
	require(kwptSource != NULL);

	nTableSourceSize = kwptSource->nTableSourceSize;
	nTableTargetSize = kwptSource->nTableTargetSize;
	cvTableValues.CopyFrom(&(kwptSource->cvTableValues));
}

KIShapleyTable* KIShapleyTable::Clone() const
{
	KIShapleyTable* kwptClone;

	kwptClone = new KIShapleyTable;
	kwptClone->CopyFrom(this);
	return kwptClone;
}

boolean KIShapleyTable::Check() const
{
	if (cvTableValues.GetSize() == nTableSourceSize * nTableTargetSize)
		return true;

	return false;
}

void KIShapleyTable::Write(ostream& ost) const
{
	const ALString sSourcePrefix = "S";
	const ALString sTargetPrefix = "T";
	int nSource;
	int nTarget;

	// Titre
	ost << "Target";
	ost << "\tShapley Value";
	ost << "\n";

	// Libelles des cibles
	for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
		ost << "\t" << sTargetPrefix << nTarget + 1;
	ost << "\n";

	// Affichage des valeurs de la table
	for (nSource = 0; nSource < GetSourceSize(); nSource++)
	{
		// Libelle de la source
		ost << sSourcePrefix << nSource + 1;

		// Valeur par cible
		for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
			ost << "\t" << GetShapleyValueAt(nSource, nTarget);
		ost << "\n";
	}
}

longint KIShapleyTable::GetUsedMemory() const
{
	return sizeof(KIShapleyTable) + cvTableValues.GetUsedMemory() - sizeof(ContinuousVector);
}

const ALString KIShapleyTable::GetClassLabel() const
{
	return "Shapley table";
}

void KIShapleyTable::InitializeFromUnivariateDataGridStats(const KWDataGridStats* attributeDataGridStats,
							   const KWDataGridStats* targetDataGridStats,
							   double dAttributeWeight)
{
	const KWDGSAttributePartition* attributeTargetPartition;
	const KWDGSAttributeSymbolValues* targetPartition;
	IntVector ivSourcePartFrequencies;
	IntVector ivTargetPartFrequencies;
	int nTotalFrequency;
	int nSourcePartNumber;
	int nTargetPartNumber;
	int nSourcePart;
	int nTargetPart;
	int nTarget;
	int nTargetFrequency;
	double dLaplaceEpsilon;
	double dConditionalOneFrequency;
	double dConditionalOneLaplaceEpsilon;
	double dConditionalAllFrequency;
	double dConditionalAllLaplaceEpsilon;
	double dProbOne;
	double dProbAll;
	double dTerm;
	double dExpectedTerm;
	double dShapleyValue;

	require(attributeDataGridStats != NULL);
	require(attributeDataGridStats->Check());
	require(attributeDataGridStats->GetAttributeNumber() == 2);
	require(attributeDataGridStats->GetAttributeAt(attributeDataGridStats->GetAttributeNumber() - 1)
		    ->GetAttributeType() == KWType::Symbol);
	require(targetDataGridStats != NULL);
	require(targetDataGridStats->Check());
	require(targetDataGridStats->GetAttributeNumber() == 1);
	require(targetDataGridStats->GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
	require(attributeDataGridStats->ComputeGridFrequency() == targetDataGridStats->ComputeGridFrequency());
	require(0 < dAttributeWeight);

	// Acces a la partition cible de l'attribut et a celle des valeurs cibles
	attributeTargetPartition = attributeDataGridStats->GetAttributeAt(1);
	targetPartition = cast(KWDGSAttributeSymbolValues*, targetDataGridStats->GetAttributeAt(0));

	// Calcul des effectifs par partie pour l'attribut source et cible de la grille
	attributeDataGridStats->ExportAttributePartFrequenciesAt(0, &ivSourcePartFrequencies);
	attributeDataGridStats->ExportAttributePartFrequenciesAt(1, &ivTargetPartFrequencies);
	nSourcePartNumber = ivSourcePartFrequencies.GetSize();
	nTargetPartNumber = ivTargetPartFrequencies.GetSize();
	nTotalFrequency = targetDataGridStats->ComputeGridFrequency();

	// Initialisation de la taille avec le nombre de parties sources et le nombre de valeurs cibles
	// Attention, le cacul des valeurs de Shapley se fait par valeur cible, et non par partie cible,
	// notamment pour le cas avec groupement des valeurs cibles
	// Dans le cas avec groupement des valeurs cibles, on exploite l'hypothese de densite uniforme
	// par morceaux pour faire le calcul des valeurs de Shapley en repartissant les effectifs de chaque
	// groupe de valeurs cibles au prorata des effectifs des valeurs du groupe
	Initialize(attributeDataGridStats->GetAttributeAt(0)->GetPartNumber(), targetPartition->GetValueNumber());

	// On calcul d'abord les log(p(x_i|yOne)/p(x_i|yAll)) pour chaque x_i et chaque y_j
	// en mode One versus All pour chaque classe y_j
	//
	// Utilisation d'un epsilon de Laplace pour eviter les probabilite nulles
	// On prend l'effectif total plus un pour eviter les effets de bord (cf. classe KWProbabilityTable)
	// Pour etre au plus proche des calculs de probabilites utilises dans le SNB, on considere toujours
	// le meme nombre de valeurs cibles dans l'utilisation de l'epsilon de Laplace, en utilisant 1 espilon pour
	// la probabilite de la partie cible d'interet et (nTargetNumber-1) epsilon pour l'ensemble des autres parties cibles
	dLaplaceEpsilon = 1.0 / (nTotalFrequency + 1);
	for (nTarget = 0; nTarget < GetTargetSize(); nTarget++)
	{
		// Index de la partie cible correspondant a la valeur cible
		nTargetPart = attributeTargetPartition->ComputeSymbolPartIndex(targetPartition->GetValueAt(nTarget));

		// Effectif de la valeur cible
		nTargetFrequency = targetDataGridStats->GetUnivariateCellFrequencyAt(nTarget);

		// Epsilon de Laplace associe a la valeur cible, dans le cas One
		// Meme en cas de groupement de la valeur cible, on ne divise pas le epsilon au prorata
		// de l'effectif de la valeur cible dans son eventuelle partie cible, pour exploiter toujours
		// le meme epsilon qui fournit un majorant unique pour les logs de probabilite en cas de cellule vide
		dConditionalOneLaplaceEpsilon = dLaplaceEpsilon;

		// Idem pour le cas All
		dConditionalAllLaplaceEpsilon = nTargetPartNumber * dLaplaceEpsilon - dConditionalOneLaplaceEpsilon;

		// Premiere passe pour calcul le terme de valeur et d'esperance
		dExpectedTerm = 0;
		for (nSourcePart = 0; nSourcePart < GetSourceSize(); nSourcePart++)
		{
			// Effectif de la cellule (partie source, valeur cible) pour la valeur dans le cas One
			// - effectif exact dans le cas de valeurs cible singleton
			// - effectif au prorata des valeurs de la partie dans le cas avec groupement de valeur cibles
			dConditionalOneFrequency =
			    attributeDataGridStats->GetBivariateCellFrequencyAt(nSourcePart, nTargetPart);
			if (not attributeTargetPartition->ArePartsSingletons())
				dConditionalOneFrequency = (dConditionalOneFrequency * nTargetFrequency) /
							   ivTargetPartFrequencies.GetAt(nTargetPart);

			// Effectif de la cellule et epsilon de Laplace dans le cas All
			dConditionalAllFrequency =
			    ivSourcePartFrequencies.GetAt(nSourcePart) - dConditionalOneFrequency;

			// Probabilite conditionnelle pour la valeur source sachant la valeur cible
			dProbOne = (dConditionalOneFrequency + dConditionalOneLaplaceEpsilon) /
				   (nTargetFrequency + nSourcePartNumber * dConditionalOneLaplaceEpsilon);

			// Probabilite conditionnelle pour la valeur source sachant toutes les autres valeurs cibles
			dProbAll =
			    (dConditionalAllFrequency + dConditionalAllLaplaceEpsilon) /
			    (nTotalFrequency - nTargetFrequency + nSourcePartNumber * dConditionalAllLaplaceEpsilon);

			// Memorisation du resultats intermediaire
			dTerm = log(dProbOne / dProbAll);
			SetShapleyValueAt(nSourcePart, nTarget, dTerm);

			// Mise a jour du calcul d'esperance de la valeur
			dExpectedTerm += (ivSourcePartFrequencies.GetAt(nSourcePart) / (double)nTotalFrequency) * dTerm;
		}

		// Deuxieme passe pour soustraire l'esperance et obtenir la valeur de Shapley
		for (nSourcePart = 0; nSourcePart < GetSourceSize(); nSourcePart++)
		{
			dTerm = GetShapleyValueAt(nSourcePart, nTarget);
			dShapleyValue = dAttributeWeight * (dTerm - dExpectedTerm);
			SetShapleyValueAt(nSourcePart, nTarget, dShapleyValue);
		}
	}
}

void KIShapleyTable::BuildUnivariateDataGridStats(const KWDataGridStats* bivariateDataGridStats,
						  KWDataGridStats* univariateDataGridStats) const
{
	const boolean bTrace = false;
	const ALString sCellPrefix = "C";
	KWDGSAttributeSymbolValues* sourceDataGridAttribute;
	KWDGSAttributeSymbolValues* targetDataGridAttribute;
	IntVector ivPartIndexes;
	int nSource1;
	int nSource2;
	int nSource;
	int nTarget;
	int nCellFrequency;
	int i;

	require(bivariateDataGridStats != NULL);
	require(bivariateDataGridStats->Check());
	require(bivariateDataGridStats->GetAttributeNumber() == 3);
	require(bivariateDataGridStats->GetAttributeAt(2)->GetAttributeType() == KWType::Symbol);
	require(univariateDataGridStats != NULL);

	// Nettoyage prealable
	univariateDataGridStats->DeleteAll();

	// Creation d'un attribut representant la paire d'attributs sources
	sourceDataGridAttribute = new KWDGSAttributeSymbolValues;
	sourceDataGridAttribute->SetAttributeName(bivariateDataGridStats->GetAttributeAt(0)->GetAttributeName() + '`' +
						  bivariateDataGridStats->GetAttributeAt(1)->GetAttributeName());
	sourceDataGridAttribute->SetPartNumber(bivariateDataGridStats->GetAttributeAt(0)->GetPartNumber() *
					       bivariateDataGridStats->GetAttributeAt(1)->GetPartNumber());
	sourceDataGridAttribute->SetInitialValueNumber(sourceDataGridAttribute->GetPartNumber());
	sourceDataGridAttribute->SetGranularizedValueNumber(sourceDataGridAttribute->GetPartNumber());
	for (i = 0; i < sourceDataGridAttribute->GetValueNumber(); i++)
		sourceDataGridAttribute->SetValueAt(i, Symbol(sCellPrefix + IntToString(i + 1)));

	// Creation d'un attribut representant l'attribut cible
	targetDataGridAttribute = new KWDGSAttributeSymbolValues;
	targetDataGridAttribute->SetAttributeName(bivariateDataGridStats->GetAttributeAt(2)->GetAttributeName());
	targetDataGridAttribute->SetPartNumber(bivariateDataGridStats->GetAttributeAt(2)->GetPartNumber());
	targetDataGridAttribute->SetInitialValueNumber(targetDataGridAttribute->GetPartNumber());
	targetDataGridAttribute->SetGranularizedValueNumber(targetDataGridAttribute->GetPartNumber());
	for (i = 0; i < targetDataGridAttribute->GetValueNumber(); i++)
		targetDataGridAttribute->SetValueAt(
		    i, cast(KWDGSAttributeSymbolValues*, bivariateDataGridStats->GetAttributeAt(2))->GetValueAt(i));

	// Alimentation de la grille univariee a partir des attributs source et cible reconstruits
	univariateDataGridStats->AddAttribute(sourceDataGridAttribute);
	univariateDataGridStats->AddAttribute(targetDataGridAttribute);
	univariateDataGridStats->SetSourceAttributeNumber(1);
	univariateDataGridStats->CreateAllCells();

	// On commence a importer les effectifs des cellules de la grille bivariee vers la grille univariee
	ivPartIndexes.SetSize(3);
	for (nSource1 = 0; nSource1 < bivariateDataGridStats->GetAttributeAt(0)->GetPartNumber(); nSource1++)
	{
		ivPartIndexes.SetAt(0, nSource1);
		for (nSource2 = 0; nSource2 < bivariateDataGridStats->GetAttributeAt(1)->GetPartNumber(); nSource2++)
		{
			ivPartIndexes.SetAt(1, nSource2);
			for (nTarget = 0; nTarget < bivariateDataGridStats->GetAttributeAt(2)->GetPartNumber();
			     nTarget++)
			{
				ivPartIndexes.SetAt(2, nTarget);

				// Recherche de l'effectif de la cellule
				nCellFrequency = bivariateDataGridStats->GetCellFrequencyAt(&ivPartIndexes);

				// Memorisation de l'effectif dans la grille univarie de destination
				nSource =
				    nSource1 + nSource2 * bivariateDataGridStats->GetAttributeAt(0)->GetPartNumber();
				univariateDataGridStats->SetBivariateCellFrequencyAt(nSource, nTarget, nCellFrequency);
			}
		}
	}
	assert(univariateDataGridStats->Check());
	assert(univariateDataGridStats->ComputeGridSize() == bivariateDataGridStats->ComputeGridSize());
	assert(univariateDataGridStats->ComputeCellNumber() == bivariateDataGridStats->ComputeCellNumber());
	assert(univariateDataGridStats->ComputeGridFrequency() == bivariateDataGridStats->ComputeGridFrequency());

	// Trace
	if (bTrace)
	{
		cout << "BuildUnivariateDataGridStats\n";
		cout << "Bivariate grid\n" << *bivariateDataGridStats << "\n";
		cout << "Univariate grid\n" << *univariateDataGridStats << "\n";
		cout << "\n";
	}
}

void KIShapleyTable::BuildRegressionAnalysisDataGridStats(const KWDataGridStats* regressionDataGridStats,
							  KWDataGridStats* univariateDataGridStats,
							  KWDataGridStats* targetDataGridStats) const
{
	const boolean bTrace = false;
	const ALString sSingletonPrefix = "OneOf_I";
	const ALString sIntervalPrefix = "RestOf_I";
	IntVector ivTargetPartFrequencies;
	KWDGSAttributePartition* sourceDataGridAttribute;
	KWDGSAttributeGrouping* targetDataGridAttribute;
	KWDGSAttributeSymbolValues* targetValueDataGridAttribute;
	int nTargetIntervalNumber;
	int nTargetValueNumber;
	int nTrueTargetValueNumber;
	IntVector ivPartIndexes;
	int nSource;
	int nTarget;
	int nCellFrequency;

	require(regressionDataGridStats != NULL);
	require(regressionDataGridStats->Check());
	require(regressionDataGridStats->GetAttributeNumber() == 2);
	require(regressionDataGridStats->GetAttributeAt(1)->GetAttributeType() == KWType::Continuous);
	require(not regressionDataGridStats->GetAttributeAt(1)->ArePartsSingletons());
	require(univariateDataGridStats != NULL);
	require(targetDataGridStats != NULL);

	// Nettoyage prealable
	univariateDataGridStats->DeleteAll();
	targetDataGridStats->DeleteAll();

	// On garde le premier attribut de la grille
	sourceDataGridAttribute = regressionDataGridStats->GetAttributeAt(0)->Clone();

	// On cree un attribut de type groupement de valeur a partir du second attribut de discretisation
	targetDataGridAttribute = new KWDGSAttributeGrouping;
	targetDataGridAttribute->SetAttributeName(regressionDataGridStats->GetAttributeAt(1)->GetAttributeName());
	nTargetIntervalNumber = regressionDataGridStats->GetAttributeAt(1)->GetPartNumber();
	targetDataGridAttribute->SetPartNumber(nTargetIntervalNumber);

	// Parametrage de la partition cible, avec la StarValue
	nTargetValueNumber = 2 * nTargetIntervalNumber + 1;
	targetDataGridAttribute->SetKeptValueNumber(nTargetValueNumber);
	targetDataGridAttribute->SetInitialValueNumber(nTargetValueNumber);
	targetDataGridAttribute->SetGranularizedValueNumber(nTargetValueNumber);
	targetDataGridAttribute->SetValueAt(nTargetValueNumber - 1, Symbol::GetStarValue());
	for (nTarget = 0; nTarget < nTargetIntervalNumber; nTarget++)
	{
		// Parametrage des valeurs du groupe
		targetDataGridAttribute->SetValueAt(2 * nTarget, Symbol(sSingletonPrefix + IntToString(nTarget + 1)));
		targetDataGridAttribute->SetValueAt(2 * nTarget + 1,
						    Symbol(sIntervalPrefix + IntToString(nTarget + 1)));

		// Parametrage du groupe
		targetDataGridAttribute->SetGroupFirstValueIndexAt(nTarget, 2 * nTarget);
	}

	// Alimentation de la grille univariee a partir des attributs source et cible reconstruits
	univariateDataGridStats->AddAttribute(sourceDataGridAttribute);
	univariateDataGridStats->AddAttribute(targetDataGridAttribute);
	univariateDataGridStats->SetSourceAttributeNumber(1);
	univariateDataGridStats->CreateAllCells();

	// Alimentation des effectifs des cellules
	for (nSource = 0; nSource < sourceDataGridAttribute->GetPartNumber(); nSource++)
	{
		for (nTarget = 0; nTarget < targetDataGridAttribute->GetPartNumber(); nTarget++)
		{
			nCellFrequency = regressionDataGridStats->GetBivariateCellFrequencyAt(nSource, nTarget);
			univariateDataGridStats->SetBivariateCellFrequencyAt(nSource, nTarget, nCellFrequency);
		}
	}
	assert(univariateDataGridStats->Check());
	assert(univariateDataGridStats->ComputeGridSize() == regressionDataGridStats->ComputeGridSize());
	assert(univariateDataGridStats->ComputeCellNumber() == regressionDataGridStats->ComputeCellNumber());
	assert(univariateDataGridStats->ComputeGridFrequency() == regressionDataGridStats->ComputeGridFrequency());

	// Creation de l'attribut contenant les valeur de la grille cible, sans la StarValue
	nTrueTargetValueNumber = nTargetValueNumber - 1;
	targetValueDataGridAttribute = new KWDGSAttributeSymbolValues;
	targetValueDataGridAttribute->SetAttributeName(regressionDataGridStats->GetAttributeAt(1)->GetAttributeName());
	targetValueDataGridAttribute->SetPartNumber(nTrueTargetValueNumber);
	targetValueDataGridAttribute->SetInitialValueNumber(nTrueTargetValueNumber);
	targetValueDataGridAttribute->SetGranularizedValueNumber(nTrueTargetValueNumber);
	for (nTarget = 0; nTarget < nTrueTargetValueNumber; nTarget++)
		targetValueDataGridAttribute->SetValueAt(nTarget, targetDataGridAttribute->GetValueAt(nTarget));

	// Export des effectifs par intervalle cible
	regressionDataGridStats->ExportAttributePartFrequenciesAt(1, &ivTargetPartFrequencies);
	assert(ivTargetPartFrequencies.GetSize() * 2 == nTrueTargetValueNumber);

	// Alimentation de la grille cible et de ses efefctifs par valeurs
	targetDataGridStats->AddAttribute(targetValueDataGridAttribute);
	targetDataGridStats->CreateAllCells();
	for (nTarget = 0; nTarget < ivTargetPartFrequencies.GetSize(); nTarget++)
	{
		targetDataGridStats->SetUnivariateCellFrequencyAt(2 * nTarget, 1);
		targetDataGridStats->SetUnivariateCellFrequencyAt(2 * nTarget + 1,
								  ivTargetPartFrequencies.GetAt(nTarget) - 1);
	}
	assert(targetDataGridStats->Check());
	assert(targetDataGridStats->ComputeGridFrequency() == regressionDataGridStats->ComputeGridFrequency());

	// Trace
	if (bTrace)
	{
		cout << "BuildRegressionAnalysisDataGridStats\n";
		cout << "Regression grid\n" << *regressionDataGridStats << "\n";
		cout << "Univariate grid\n" << *univariateDataGridStats << "\n";
		cout << "Target grid\n" << *targetDataGridStats << "\n";
		cout << "\n";
	}
}
