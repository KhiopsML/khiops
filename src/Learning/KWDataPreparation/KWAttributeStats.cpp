// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributeStats.h"

KWAttributeStats::KWAttributeStats()
{
	nAttributeType = KWType::Unknown;

	// L'objet kwDescriptiveStats n'est cree que lorsque l'on donne
	// le type de l'attribut via SetAttributeType()
	kwDescriptiveStats = NULL;
	symbolValueStats = NULL;
	modlHistogramResults = NULL;
}

KWAttributeStats::~KWAttributeStats()
{
	// Attention: il faut explicitement appeler cette methode dans le destructeur,
	// bien qu'elle soit appelee dans le destructeur ancetre
	CleanDataPreparationResults();

	// Nettoyage des statistiques descriptives
	if (kwDescriptiveStats != NULL)
		delete kwDescriptiveStats;
	kwDescriptiveStats = NULL;

	// Nettoyage des statistiques par valeur
	if (symbolValueStats != NULL)
		delete symbolValueStats;
	symbolValueStats = NULL;

	// Nettoage des histogrammes MODL
	if (modlHistogramResults != NULL)
		delete modlHistogramResults;
	modlHistogramResults = NULL;
}

void KWAttributeStats::SetAttributeName(const ALString& sValue)
{
	sAttributeName = sValue;
	bIsStatsComputed = false;
}

const ALString& KWAttributeStats::GetAttributeName() const
{
	return sAttributeName;
}

int KWAttributeStats::GetAttributeNumber() const
{
	return 1;
}

const ALString& KWAttributeStats::GetAttributeNameAt(int nIndex) const
{
	require(nIndex == 0);
	return sAttributeName;
}

void KWAttributeStats::SetAttributeType(int nValue)
{
	require(nAttributeType == KWType::Unknown);
	require(KWType::IsSimple(nValue));

	nAttributeType = nValue;

	// Allocation de kwDescriptiveStats en fonction du type de l'attribut
	if (nValue == KWType::Continuous)
		kwDescriptiveStats = new KWDescriptiveContinuousStats;
	else if (nValue == KWType::Symbol)
		kwDescriptiveStats = new KWDescriptiveSymbolStats;

	// Allocation de symbolValueStats uniquement dans le cas Symbol
	if (nValue == KWType::Symbol)
		symbolValueStats = new KWDataGridStats;
}

const int KWAttributeStats::GetAttributeType() const
{
	return nAttributeType;
}

boolean KWAttributeStats::ComputeStats(const KWTupleTable* tupleTable)
{
	require(Check());
	require(GetClass()->LookupAttribute(sAttributeName) != NULL);
	require(GetClass()->LookupAttribute(sAttributeName)->GetUsed());
	require(GetClass()->LookupAttribute(sAttributeName)->GetLoaded());
	require(GetAttributeType() != KWType::Unknown);

	// Nettoyage des donnees de preparation
	CleanDataPreparationResults();
	bIsStatsComputed = true;

	// Parametrages des statistiques descriptives
	kwDescriptiveStats->SetLearningSpec(GetLearningSpec());
	kwDescriptiveStats->SetAttributeName(GetAttributeName());

	// Calcul des stats descriptives de l'attribut source considere.
	// En sortie, la base est triee selon cet attribut.
	bIsStatsComputed = kwDescriptiveStats->ComputeStats(tupleTable);

	// Calcul des stats par valeur dans le cas Symbol
	if (bIsStatsComputed and GetAttributeType() == KWType::Symbol)
		bIsStatsComputed =
		    GetLearningSpec()->ComputeSymbolValueStats(GetAttributeName(), tupleTable, true, symbolValueStats);

	// Pretraitement si base non vide
	if (bIsStatsComputed and tupleTable->GetTotalFrequency() > 0)
	{
		// Tentative de partitionnement supervise
		if (GetTargetAttributeName() != "")
		{
			// Cas particulier de la classification supervisee sans groupage de la cible
			// ou l'on fait appel a l'algorithme de partitionnement 1D
			if (GetTargetAttributeType() == KWType::Symbol and not IsTargetGrouped())
			{
				// Discretisation dans le cas d'un attribut source de type Continuous
				if (GetAttributeType() == KWType::Continuous)
					Discretize(tupleTable);

				// Groupage dans le cas d'un attribut source de type Symbol
				else if (GetAttributeType() == KWType::Symbol)
					Group(tupleTable);
			}

			// Cas de la classification avec groupage de la cible
			else if (GetTargetAttributeType() == KWType::Symbol and IsTargetGrouped())
			{
				Group2D(tupleTable);
			}

			// Cas de la regression univariee
			else if (GetTargetAttributeType() == KWType::Continuous)
			{
				Group2D(tupleTable);
			}
		}
		// Tentative de partitionnement non supervise
		else if (GetTargetAttributeName() == "")
		{
			assert(GetTargetAttributeType() == KWType::None);

			// Discretisation dans le cas d'un attribut source de type Continuous
			if (GetAttributeType() == KWType::Continuous)
			{
				UnsupervisedDiscretize(tupleTable);
			}

			// Groupage dans le cas d'un attribut source de type Symbol
			else if (GetAttributeType() == KWType::Symbol)
			{
				UnsupervisedGroup(tupleTable);
			}
		}
	}

	// Reinitialisation des resultats si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
	{
		CleanDataPreparationResults();
		bIsStatsComputed = false;
	}
	return bIsStatsComputed;
}

KWDescriptiveStats* KWAttributeStats::GetDescriptiveStats()
{
	require(GetAttributeType() != KWType::Unknown);
	return kwDescriptiveStats;
}

KWDataGridStats* KWAttributeStats::GetSymbolValueStats()
{
	require(GetAttributeType() == KWType::Symbol);
	return symbolValueStats;
}

void KWAttributeStats::WriteReport(ostream& ost) const
{
	boolean bWriteTarget;
	KWDGSAttributeSymbolValues* symbolValues;
	int nValue;
	Symbol sValue;
	int nFrequency;

	require(IsStatsComputed());

	// Affichage des stats cibles uniquement si groupement cible et si
	// moins de groupes cibles que de valeurs cibles
	bWriteTarget = false;
	if (IsTargetGrouped() and GetTargetAttributeType() == KWType::Symbol and preparedDataGridStats != NULL)
	{
		assert(preparedDataGridStats->GetTargetAttributeNumber() == 1);
		assert(preparedDataGridStats->GetAttributeAt(preparedDataGridStats->GetFirstTargetAttributeIndex())
			   ->GetAttributeName() == GetTargetAttributeName());
		if (preparedDataGridStats->ComputeTargetGridSize() < GetTargetDescriptiveStats()->GetValueNumber())
			bWriteTarget = true;
	}

	// Entete
	ost << "Rank\t" << GetIdentifier() << "\n";

	// Grille de preparation
	if (preparedDataGridStats != NULL)
		preparedDataGridStats->WritePartial(ost, true, GetTargetAttributeType() == KWType::None);
	if (bWriteTarget)
	{
		ost << "\n";
		preparedDataGridStats->WriteAttributePartArrayLineReports(ost, false, true);
	}

	// Valeurs dans le cas categoriel
	if (symbolValueStats != NULL and symbolValueStats->GetAttributeAt(0)->GetPartNumber() > 1)
	{
		// Entete
		ost << "\nValues\n";
		ost << "Value\tFrequency\tCoverage\n";

		// Valeurs
		symbolValues = cast(KWDGSAttributeSymbolValues*, symbolValueStats->GetAttributeAt(0));
		for (nValue = 0; nValue < symbolValues->GetPartNumber(); nValue++)
		{
			sValue = symbolValues->GetValueAt(nValue);
			nFrequency = symbolValueStats->GetUnivariateCellFrequencyAt(nValue);

			// Cas standard
			if (sValue != Symbol::GetStarValue())
				ost << TSV::Export(sValue.GetValue()) << "\t" << nFrequency << "\t"
				    << nFrequency * 1.0 / GetInstanceNumber() << "\n";
			else
				ost << "...\n";
		}
	}
}

boolean KWAttributeStats::IsReported() const
{
	return GetSortValue() > 0 or
	       (GetTargetAttributeType() == KWType::None and preparedDataGridStats != NULL and
		preparedDataGridStats->ComputeTotalCellNumber() > 1) or
	       (GetTargetAttributeType() == KWType::None and preparedDataGridStats != NULL and
		GetAttributeType() == KWType::Continuous and kwDescriptiveStats != NULL and
		kwDescriptiveStats->GetValueNumber() > 1);
}

void KWAttributeStats::WriteHeaderLineReport(ostream& ost) const
{
	int i;
	KWAttribute* attribute;
	boolean bDerivationUsed;

	require(IsStatsComputed());

	// Identification de l'attribut
	ost << "Rank";
	ost << "\tName";

	// Evaluation de la variable si apprentissage supervise
	if (GetTargetAttributeName() != "")
	{
		// Niveau
		ost << "\tLevel";

		// Nombre de parties cibles
		if (GetTargetAttributeType() == KWType::Continuous)
			ost << "\tTarget intervals";
		else if (GetTargetAttributeType() == KWType::Symbol and IsTargetGrouped())
			ost << "\tTarget groups";
	}

	// Nombre de parties sources, y compris en non supervise
	if (nAttributeType == KWType::Continuous)
		ost << "\tIntervals";
	else if (nAttributeType == KWType::Symbol)
		ost << "\tGroups";

	// Statistiques descriptives
	ost << "\t";
	kwDescriptiveStats->WriteHeaderLineReport(ost);

	// Couts MODL
	if (GetWriteCosts())
	{
		ost << "\tConstr. cost";
		if (GetTargetAttributeName() != "")
			ost << "\tPrep. cost\tData cost";
	}
	if (KWFrequencyTable::GetWriteGranularityAndGarbage())
	{
		ost << "\tBest granularity\tGranularity max\tGarbage presence";
	}

	// Recherche d'utilisation de regles de derivation dans la classe
	bDerivationUsed = false;
	for (i = 0; i < GetClass()->GetUsedAttributeNumber(); i++)
	{
		// On ignore l'attribut cible
		attribute = GetClass()->GetUsedAttributeAt(i);
		if (attribute->GetName() != GetTargetAttributeName())
		{
			// Mise a jour des flags globaux
			if (attribute->GetAnyDerivationRule() != NULL)
				bDerivationUsed = true;
		}
	}

	// Libelle pour le type utilisateur et la derivation
	// Attention; on est contraint dans l'ordre des champs (la methode WriteLineReport
	// travaille en aveugle, sans connaissance de la presence des deux colonnes
	// au niveau global)
	if (bDerivationUsed)
		ost << "\tDerivation rule";
}

void KWAttributeStats::WriteLineReport(ostream& ost) const
{
	KWAttribute* attribute;
	int nSource;
	int nTarget;

	require(IsStatsComputed());

	// Nom de l'attribut
	ost << GetIdentifier();
	ost << "\t" << TSV::Export(GetAttributeName());

	// Initialisation
	nSource = -1;

	// Evaluation de la variable si discretisation ou groupement pertinente
	// dans le cas supervise uniquement
	if (GetTargetAttributeName() != "")
	{
		if (GetPreparedDataGridStats() != NULL)
		{
			assert(GetPreparedDataGridStats()->GetAttributeNumber() <= 2);
			assert(GetPreparedDataGridStats()->GetAttributeNumber() >= 1);

			// Recherche des index des attributs sources et cibles
			// La grille peut ne comporter que l'attribut cible
			nTarget = GetPreparedDataGridStats()->GetFirstTargetAttributeIndex();
			nSource = nTarget - 1;

			// Statistiques
			ost << "\t" << GetLevel();

			// Nombre de parties cibles (intervalles ou groupes)
			if (GetTargetAttributeType() == KWType::Continuous or
			    (GetTargetAttributeType() == KWType::Symbol and IsTargetGrouped()))
				ost << "\t" << GetPreparedDataGridStats()->GetAttributeAt(nTarget)->GetPartNumber();

			// Nombre de parties sources (intervalles ou groupes)
			if (nSource >= 0)
				ost << "\t" << GetPreparedDataGridStats()->GetAttributeAt(nSource)->GetPartNumber();
			else
				ost << "\t1";
		}
		// Pas d'infos sinon
		else
		{
			ost << "\t0";
			if (GetTargetAttributeType() == KWType::Continuous or
			    (GetTargetAttributeType() == KWType::Symbol and IsTargetGrouped()))
				ost << "\t1";
			ost << "\t1";
		}
	}
	// Dans le cas non supervise, on ecrit eventuellement le nombre de parties
	else
	{
		assert(GetTargetAttributeName() == "");

		if (GetPreparedDataGridStats() != NULL)
		{
			assert(GetPreparedDataGridStats()->GetAttributeNumber() == 1);

			ost << "\t" << GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber();
		}
		// Pas d'infos sinon
		else
		{
			ost << "\t1";
		}
	}

	// Statistiques descriptives
	ost << "\t";
	kwDescriptiveStats->WriteLineReport(ost);

	// Couts MODL
	if (GetWriteCosts())
	{
		ost << "\t" << GetConstructionCost();
		if (GetTargetAttributeName() != "")
		{
			ost << "\t" << GetPreparationCost() << "\t" << GetDataCost();
		}
	}

	// CH V9 TODO: faire un test explicite plutot que d'utiliser un test sur nSource??
	// Reporting granularite et groupe poubelle
	if (KWFrequencyTable::GetWriteGranularityAndGarbage() and GetPreparedDataGridStats() != NULL)
	{
		ost << "\t" << GetPreparedDataGridStats()->GetGranularity() << "\t"
		    << ceil(log(GetPreparedDataGridStats()->ComputeGridFrequency() * 1.0) / log(2.0));
		// Cas d'un attribute categoriel (et informatif)
		if (nSource >= 0 and
		    GetPreparedDataGridStats()->GetAttributeAt(nSource)->GetAttributeType() == KWType::Symbol)
		{
			ost << "\t"
			    << (cast(KWDGSAttributeGrouping*, GetPreparedDataGridStats()->GetAttributeAt(nSource))
				    ->GetGarbageGroupIndex() >= 0);
		}
		// Cas du modele nul
		else
			ost << "\t0";
	}

	// Informations utilisateur supplementaires
	// L'attribut peut etre absent de la classe dans le cas d'une stats sur un attribut construit de facon
	// temporaire
	attribute = GetClass()->LookupAttribute(GetAttributeName());
	if (attribute != NULL)
	{
		if (attribute->GetAnyDerivationRule() != NULL)
		{
			ost << "\t";
			attribute->GetAnyDerivationRule()->WriteUsedRule(ost);
		}
	}
}

void KWAttributeStats::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary) const
{
	KWAttribute* attribute;
	int nSource;
	int nTarget;
	ALString sUsedRule;
	ContinuousVector cvAttributeDomainLowerBounds;
	ContinuousVector cvAttributeDomainUpperBounds;
	const KWDescriptiveContinuousStats* descriptiveContinuousStats;

	require(IsStatsComputed());

	//////////////////////////////////////////////////////////////////////
	// Ecriture des champs de base
	if (bSummary)
	{
		// Nom de l'attribut
		fJSON->WriteKeyString("rank", GetIdentifier());
		fJSON->WriteKeyString("name", GetAttributeName());

		// Type de l'attribut
		fJSON->WriteKeyString("type", KWType::ToString(GetAttributeType()));

		// Initialisation
		nSource = -1;

		// Evaluation de la variable si discretisation ou groupement pertinent
		// dans le cas supervise uniquement
		if (GetTargetAttributeName() != "")
		{
			if (GetPreparedDataGridStats() != NULL)
			{
				assert(GetPreparedDataGridStats()->GetAttributeNumber() <= 2);
				assert(GetPreparedDataGridStats()->GetAttributeNumber() >= 1);

				// Recherche des index des attributs sources et cibles
				// La grille peut ne comporter que l'attribut cible
				nTarget = GetPreparedDataGridStats()->GetFirstTargetAttributeIndex();
				nSource = nTarget - 1;

				// Statistiques
				fJSON->WriteKeyDouble("level", GetLevel());

				// Nombre de parties cibles (intervalles ou groupes)
				if (GetTargetAttributeType() == KWType::Continuous or
				    (GetTargetAttributeType() == KWType::Symbol and IsTargetGrouped()))
					fJSON->WriteKeyInt(
					    "targetParts",
					    GetPreparedDataGridStats()->GetAttributeAt(nTarget)->GetPartNumber());

				// Nombre de parties sources (intervalles ou groupes)
				if (nSource >= 0)
					fJSON->WriteKeyInt(
					    "parts",
					    GetPreparedDataGridStats()->GetAttributeAt(nSource)->GetPartNumber());
			}
			else
			{
				assert(GetLevel() == 0);

				// Statistiques
				fJSON->WriteKeyDouble("level", GetLevel());

				// Nombre de parties cibles (intervalles ou groupes)
				if (GetTargetAttributeType() == KWType::Continuous or
				    (GetTargetAttributeType() == KWType::Symbol and IsTargetGrouped()))
					fJSON->WriteKeyInt("targetParts", 1);

				// Nombre de parties sources (intervalles ou groupes)
				fJSON->WriteKeyInt("parts", 1);
			}
		}
		// Dans le cas non supervise, on ecrit eventuellement le nombre de parties
		else
		{
			assert(GetTargetAttributeName() == "");
			if (GetPreparedDataGridStats() != NULL)
			{
				assert(GetPreparedDataGridStats()->GetAttributeNumber() == 1);
				fJSON->WriteKeyInt("parts",
						   GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber());
			}
		}

		// Statistiques descriptives
		kwDescriptiveStats->WriteJSONFields(fJSON);

		// Couts MODL
		if (GetWriteCosts())
		{
			fJSON->WriteKeyDouble("constructionCost", GetConstructionCost());
			if (GetTargetAttributeName() != "")
			{
				fJSON->WriteKeyDouble("preparationCost", GetPreparationCost());
				fJSON->WriteKeyDouble("dataCost", GetDataCost());
			}
		}

		// CH V9 TODO: faire un test explicite plutot que d'utiliser un test sur nSource??
		// Reporting granularite et groupe poubelle
		if (KWFrequencyTable::GetWriteGranularityAndGarbage() and GetPreparedDataGridStats() != NULL)
		{
			fJSON->WriteKeyInt("bestGranularity", GetPreparedDataGridStats()->GetGranularity());
			fJSON->WriteKeyInt(
			    "granularityMax",
			    int(ceil(log(GetPreparedDataGridStats()->ComputeGridFrequency() * 1.0) / log(2.0))));

			// Cas d'un attribute categoriel (et informatif)
			if (nSource >= 0 and
			    GetPreparedDataGridStats()->GetAttributeAt(nSource)->GetAttributeType() == KWType::Symbol)
			{
				fJSON->WriteKeyBoolean(
				    "garbagePresence",
				    (cast(KWDGSAttributeGrouping*, GetPreparedDataGridStats()->GetAttributeAt(nSource))
					 ->GetGarbageGroupIndex() >= 0));
			}
		}

		// Informations utilisateur supplementaires
		// L'attribut peut etre absent de la classe dans le cas d'une stats sur un attribut construit de facon
		// temporaire
		attribute = GetClass()->LookupAttribute(GetAttributeName());
		if (attribute != NULL)
		{
			if (attribute->GetAnyDerivationRule() != NULL)
			{
				attribute->GetAnyDerivationRule()->WriteUsedRuleToString(sUsedRule);
				fJSON->WriteKeyString("derivationRule", sUsedRule);
			}
		}
	}
	//////////////////////////////////////////////////////////////////////
	// Ecriture des champs detailles
	else
	{
		// Grille de preparation
		if (preparedDataGridStats != NULL)
		{
			// Collecte des valeurs min et max des attributs
			cvAttributeDomainLowerBounds.SetSize(preparedDataGridStats->GetAttributeNumber());
			cvAttributeDomainUpperBounds.SetSize(preparedDataGridStats->GetAttributeNumber());
			if (GetAttributeType() == KWType::Continuous)
			{
				descriptiveContinuousStats =
				    cast(const KWDescriptiveContinuousStats*, kwDescriptiveStats);

				// On prend les bornes issues de l'histogramme dans le cas d'une discretisation non
				// supervisee MODL
				if (modlHistogramResults != NULL)
				{
					assert(GetTargetAttributeType() == KWType::None);
					assert(GetPreprocessingSpec()
						   ->GetDiscretizerSpec()
						   ->GetDiscretizer(GetTargetAttributeType())
						   ->IsMODLFamily());

					// Parametrage des bornes du domaine de l'histogramme
					cvAttributeDomainLowerBounds.SetAt(0,
									   modlHistogramResults->GetDomainLowerBound());
					cvAttributeDomainUpperBounds.SetAt(0,
									   modlHistogramResults->GetDomainUpperBound());
				}
				// Sinon, on prend les valeurs extremes de l'attribut
				else
				{
					cvAttributeDomainLowerBounds.SetAt(0, descriptiveContinuousStats->GetMin());
					cvAttributeDomainUpperBounds.SetAt(0, descriptiveContinuousStats->GetMax());
				}
			}
			if (GetTargetAttributeType() == KWType::Continuous)
			{
				descriptiveContinuousStats =
				    cast(KWDescriptiveContinuousStats*, GetTargetDescriptiveStats());
				cvAttributeDomainLowerBounds.SetAt(1, descriptiveContinuousStats->GetMin());
				cvAttributeDomainUpperBounds.SetAt(1, descriptiveContinuousStats->GetMax());
			}

			// Parametrage de la grille
			preparedDataGridStats->SetJSONAttributeDomainLowerBounds(&cvAttributeDomainLowerBounds);
			preparedDataGridStats->SetJSONAttributeDomainUpperBounds(&cvAttributeDomainUpperBounds);

			// Ecriture du rapport JSON
			preparedDataGridStats->WriteJSONKeyReport(fJSON, "dataGrid");

			// Nettoyage du parametrage
			preparedDataGridStats->SetJSONAttributeDomainLowerBounds(NULL);
			preparedDataGridStats->SetJSONAttributeDomainUpperBounds(NULL);
		}

		// Ecriture des details sur les histogrammes MODL si presents et si plus de une valeur
		if (modlHistogramResults != NULL)
			modlHistogramResults->WriteJSONKeyReport(fJSON, "modlHistograms");

		// Valeurs dans le cas categoriel
		if (symbolValueStats != NULL and symbolValueStats->GetAttributeAt(0)->GetPartNumber() > 1)
			symbolValueStats->WriteJSONKeyValueFrequencies(fJSON, "inputValues");
	}
}

const ALString KWAttributeStats::GetSortName() const
{
	return GetAttributeName();
}

double KWAttributeStats::GetSortValue() const
{
	require(IsStatsComputed());

	if (preparedDataGridStats != NULL)
		return GetLevel();
	else
		return 0;
}

longint KWAttributeStats::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDataPreparationStats::GetUsedMemory();
	lUsedMemory += sizeof(KWAttributeStats) - sizeof(KWDataPreparationStats);
	lUsedMemory += sAttributeName.GetUsedMemory();
	if (kwDescriptiveStats != NULL)
		lUsedMemory += kwDescriptiveStats->GetUsedMemory();
	if (symbolValueStats != NULL)
		lUsedMemory += symbolValueStats->GetUsedMemory();
	if (modlHistogramResults != NULL)
		lUsedMemory += modlHistogramResults->GetUsedMemory();
	return lUsedMemory;
}

void KWAttributeStats::CleanDataPreparationResults()
{
	// Nettoyage de base
	KWDataPreparationStats::CleanDataPreparationResults();

	// Nettoyage supplementaire
	if (kwDescriptiveStats != NULL)
		kwDescriptiveStats->Init();
	if (modlHistogramResults != NULL)
		delete modlHistogramResults;
	modlHistogramResults = NULL;
}

void KWAttributeStats::UnsupervisedDiscretize(const KWTupleTable* tupleTable)
{
	const KWDiscretizer* unsupervisedDiscretiser;
	KWLoadIndex liAttributeLoadIndex;
	KWAttribute* attribute;
	ContinuousVector* cvSourceValues;
	IntVector* ivTargetIndexes;
	KWFrequencyTable* kwftPreparedTable;
	ContinuousVector* cvBounds;

	require(Check());
	require(GetTargetAttributeType() == KWType::None);
	require(tupleTable->GetTotalFrequency() > 0);
	require(preparedDataGridStats == NULL);

	// Seules methodes de discretisation non supervisees
	if (GetPreprocessingSpec()->GetDiscretizerSpec()->GetMethodName(GetTargetAttributeType()) != "none")
	{
		// Recherche de l'index de l'attribut
		liAttributeLoadIndex = GetClass()->LookupAttribute(GetAttributeName())->GetLoadIndex();

		// Creation des vecteur de valeurs source, avec une pseudo valeur cible constante
		cvSourceValues = ComputeInitialSourceValues(tupleTable);
		ivTargetIndexes = new IntVector;
		ivTargetIndexes->SetSize(cvSourceValues->GetSize());

		// Recherche du discretiseur a utiliser
		unsupervisedDiscretiser =
		    GetPreprocessingSpec()->GetDiscretizerSpec()->GetDiscretizer(GetTargetAttributeType());
		assert(unsupervisedDiscretiser->IsUsingSourceValues());

		// Utilisation du discretiseur specifie dans les pretraitements,
		// avec utilisation des valeurs sources (puisqu'on est dans le cas non supervise)
		kwftPreparedTable = NULL;
		unsupervisedDiscretiser->DiscretizeValues(cvSourceValues, ivTargetIndexes, 1, kwftPreparedTable,
							  cvBounds);
		assert(kwftPreparedTable != NULL);

		// Memorisation des histogrammes MODL
		if (unsupervisedDiscretiser->IsMODLFamily())
		{
			// Acces generique aux resultats, potentiellement NULL si pas d'histogramme disponible
			modlHistogramResults =
			    cast(KWDiscretizerMODLFamily*, unsupervisedDiscretiser)->BuildMODLHistogramResults();
			assert(modlHistogramResults == NULL or
			       modlHistogramResults->GetDiscretizerName() == unsupervisedDiscretiser->GetName());
		}

		// Nettoyage
		delete cvSourceValues;
		delete ivTargetIndexes;

		// Initialisation par defaut des nombres de valeurs
		kwftPreparedTable->SetInitialValueNumber(kwftPreparedTable->GetTotalFrequency());
		kwftPreparedTable->SetGranularizedValueNumber(kwftPreparedTable->GetTotalFrequency());

		// Creation de la grille de preparation
		BuildPreparedDiscretizationDataGridStats(tupleTable, kwftPreparedTable, cvBounds);

		// Nettoyage des donnees de travail initiales
		delete kwftPreparedTable;
		if (cvBounds != NULL)
			delete cvBounds;
	}
	// Cas sans pretraitement: on se contente de produire une grille a un seul intervalle
	else
	{
		// Creation d'une table d'effectifs mono-cellule
		kwftPreparedTable = new KWFrequencyTable;
		kwftPreparedTable->SetFrequencyVectorNumber(1);
		cast(KWDenseFrequencyVector*, kwftPreparedTable->GetFrequencyVectorAt(0))
		    ->GetFrequencyVector()
		    ->SetSize(1);
		cast(KWDenseFrequencyVector*, kwftPreparedTable->GetFrequencyVectorAt(0))
		    ->GetFrequencyVector()
		    ->SetAt(0, tupleTable->GetTotalFrequency());

		// Creation de la grille de preparation
		BuildPreparedDiscretizationDataGridStats(tupleTable, kwftPreparedTable, NULL);

		// Nettoyage des donnees de travail initiales
		delete kwftPreparedTable;
	}

	// Rappatriement du cout de construction
	attribute = GetClass()->LookupAttribute(GetAttributeName());
	check(attribute);
	SetConstructionCost(GetNullConstructionCost() + attribute->GetCost());
}

void KWAttributeStats::UnsupervisedGroup(const KWTupleTable* tupleTable)
{
	KWAttribute* attribute;
	SymbolVector* svInitialSourceModalities;
	KWFrequencyTable* kwftInitialTable;
	KWFrequencyTable* kwftPreparedTable;
	IntVector* ivGroups;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;

	require(Check());
	require(GetTargetAttributeType() == KWType::None);
	require(tupleTable->GetTotalFrequency() > 0);
	require(GetAttributeType() == KWType::Symbol);
	require(preparedDataGridStats == NULL);

	// Seules methodes de groupement non supervisees
	if (GetPreprocessingSpec()->GetGrouperSpec()->GetMethodName(GetTargetAttributeType()) != "none")
	{
		// Creation d'une table de contingence initiale et du vecteur des valeurs
		// de l'attribut source associe par parcours de la base triee
		BuildInitialFrequencyTable(tupleTable, GetDescriptiveStats()->GetValueNumber(),
					   svInitialSourceModalities, kwftInitialTable);

		// Tri par frequence decroissante (pour des raisons de reporting et pour
		// preparer le travail des algorithmes de groupage)
		kwftInitialTable->SortTableAndModalitiesBySourceFrequency(svInitialSourceModalities, false, NULL, NULL);

		// Utilisation du groupeur specifie dans les pretraitements, si au moins 2 modalites sources et cibles
		kwftPreparedTable = NULL;
		GetPreprocessingSpec()
		    ->GetGrouperSpec()
		    ->GetGrouper(GetTargetAttributeType())
		    ->Group(kwftInitialTable, kwftPreparedTable, ivGroups);
		assert(kwftPreparedTable != NULL);
		assert(dPreparedLevel == 0);
		assert(ivGroups != NULL);

		// Nettoyage de la table initiale
		delete kwftInitialTable;

		// Initialisation par defaut des nombres de valeurs
		kwftPreparedTable->SetInitialValueNumber(svInitialSourceModalities->GetSize());
		kwftPreparedTable->SetGranularizedValueNumber(ivGroups->GetSize());

		// Creation de la grille de preparation
		BuildPreparedGroupingDataGridStats(svInitialSourceModalities, kwftPreparedTable, ivGroups);

		// Nettoyage des donnees de travail initiales
		delete ivGroups;
		delete svInitialSourceModalities;
		delete kwftPreparedTable;
	}
	// Cas sans pretraitement: on se contente de produire une grille a un seul groupe
	else
	{
		// Creation d'une table de contingence mono-cellule
		ivGroups = new IntVector;
		ivGroups->Add(0);
		svInitialSourceModalities = new SymbolVector;
		svInitialSourceModalities->Add(cast(KWDescriptiveSymbolStats*, GetDescriptiveStats())->GetMode());
		kwftPreparedTable = new KWFrequencyTable;
		kwftPreparedTable->SetFrequencyVectorNumber(1);
		kwftPreparedTable->SetInitialValueNumber(1);
		kwftPreparedTable->SetGranularizedValueNumber(1);

		// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftPreparedTable->GetFrequencyVectorAt(0));
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();

		ivFrequencyVector->SetSize(1);
		ivFrequencyVector->SetAt(0, tupleTable->GetTotalFrequency());

		// Creation de la grille de preparation
		BuildPreparedGroupingDataGridStats(svInitialSourceModalities, kwftPreparedTable, ivGroups);

		// Nettoyage des donnees de travail initiales
		delete ivGroups;
		delete svInitialSourceModalities;
		delete kwftPreparedTable;
	}

	// Rappatriement du cout de construction
	attribute = GetClass()->LookupAttribute(GetAttributeName());
	check(attribute);
	SetConstructionCost(GetNullConstructionCost() + attribute->GetCost());
}

void KWAttributeStats::Discretize(const KWTupleTable* tupleTable)
{
	KWDiscretizerMODLFamily* discretizerMODLFamily;
	KWAttribute* attribute;
	ContinuousVector* cvSourceValues;
	IntVector* ivTargetIndexes;
	KWFrequencyTable* kwftInitialTable;
	KWFrequencyTable* kwftPreparedTable;
	ContinuousVector* cvBounds;
	int i;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;

	require(Check());
	require(GetTargetAttributeType() == KWType::Symbol);
	require(tupleTable->GetTotalFrequency() > 0);
	require(preparedDataGridStats == NULL);

	// Recherche de l'index de l'attribut
	attribute = GetClass()->LookupAttribute(GetAttributeName());
	check(attribute);

	// Initialisation du cout de construction, quelque soit la methode
	SetConstructionCost(GetNullConstructionCost() + attribute->GetCost());

	// Traitement special si discretisation non supervisee. En effet, ces
	// discretisation ne peuvent etre implementee comme des transformations
	// generiques de tables d'effectifs : elles ont besoin (au moins EqualWidth)
	// des valeurs des instances
	kwftInitialTable = NULL;
	kwftPreparedTable = NULL;
	cvBounds = NULL;
	if (GetTargetDescriptiveStats()->GetValueNumber() >= 2)
	{
		if (GetPreprocessingSpec()
			->GetDiscretizerSpec()
			->GetDiscretizer(GetTargetAttributeType())
			->IsUsingSourceValues())
		{
			// Creation des vecteur de valeurs source et d'index des valeur cibles
			cvSourceValues = ComputeInitialSourceValues(tupleTable);
			ivTargetIndexes = ComputeInitialTargetIndexes(tupleTable);

			// Utilisation du discretiseur specifie dans les pretraitements,
			// avec utilisation des valeurs sources
			GetPreprocessingSpec()
			    ->GetDiscretizerSpec()
			    ->GetDiscretizer(GetTargetAttributeType())
			    ->DiscretizeValues(cvSourceValues, ivTargetIndexes,
					       GetTargetDescriptiveStats()->GetValueNumber(), kwftPreparedTable,
					       cvBounds);

			// Nettoyage
			delete cvSourceValues;
			delete ivTargetIndexes;
		}
		// Cas des discretisations supervisees, traitees de facon generique
		else
		{
			// Creation d'une table de contingence initiale
			// Dans le cas du discretiseur MODL, les intervalles purs ne sont plus fusionnes lors de cette
			// etape de creation Seules les valeurs sources identiques sont affectees au meme intervalle La
			// fusion des intervalles purs est effectuee apres l'etape de granularisation
			if (GetPreprocessingSpec()
				->GetDiscretizerSpec()
				->GetDiscretizer(GetTargetAttributeType())
				->IsMODLFamily())
			{
				kwftInitialTable =
				    ComputeInitialContinuousFrequencyTableWithoutPureIntervals(tupleTable);
			}
			else
				kwftInitialTable = ComputeInitialContinuousFrequencyTable(tupleTable);

			// Discretisation uniquement si au moins deux valeurs
			if (kwftInitialTable != NULL)
			{
				// Recherche si c'est la methode MODL qui est utilisee
				discretizerMODLFamily = NULL;
				if (GetPreprocessingSpec()
					->GetDiscretizerSpec()
					->GetDiscretizer(GetTargetAttributeType())
					->IsMODLFamily())
				{
					discretizerMODLFamily =
					    cast(KWDiscretizerMODLFamily*,
						 GetPreprocessingSpec()->GetDiscretizerSpec()->GetDiscretizer(
						     GetTargetAttributeType()));

					// Parametrage du cout de l'attribut
					discretizerMODLFamily->GetDiscretizationCosts()->SetAttributeCost(
					    attribute->GetCost());
				}

				// Utilisation du discretiseur specifie dans les pretraitements
				GetPreprocessingSpec()
				    ->GetDiscretizerSpec()
				    ->GetDiscretizer(GetTargetAttributeType())
				    ->Discretize(kwftInitialTable, kwftPreparedTable);

				// Memorisation des couts MODL
				if (discretizerMODLFamily != NULL)
				{
					SetConstructionCost(
					    discretizerMODLFamily->ComputeDiscretizationConstructionCost(
						kwftPreparedTable));
					SetPreparationCost(discretizerMODLFamily->ComputeDiscretizationPreparationCost(
					    kwftPreparedTable));
					SetDataCost(
					    discretizerMODLFamily->ComputeDiscretizationDataCost(kwftPreparedTable));
				}

				// Nettoyage
				delete kwftInitialTable;
			}
		}
	}

	// Si pas de discretization possible, creation d'un seul intervalle
	if (kwftPreparedTable == NULL)
	{
		assert(GetTargetDescriptiveStats()->GetValueNumber() > 0);

		// Creation d'une table de contingence resultat avec une seule ligne
		kwftPreparedTable = new KWFrequencyTable;
		kwftPreparedTable->SetFrequencyVectorNumber(1);

		// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftPreparedTable->GetFrequencyVectorAt(0));
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();
		ivFrequencyVector->SetSize(GetTargetDescriptiveStats()->GetValueNumber());

		// Alimentation de cette ligne par les frequences globales des valeurs cibles
		assert(GetTargetDescriptiveStats()->GetValueNumber() == GetTargetValueStats()->ComputeTargetGridSize());
		for (i = 0; i < GetTargetDescriptiveStats()->GetValueNumber(); i++)
		{
			ivFrequencyVector->SetAt(i, GetTargetValueStats()->GetUnivariateCellFrequencyAt(i));
		}

		// Memorisation de cout MODL du modele nul si necessaire
		if (GetPreprocessingSpec()
			->GetDiscretizerSpec()
			->GetDiscretizer(GetTargetAttributeType())
			->IsMODLFamily())
		{
			SetConstructionCost(GetNullConstructionCost());
			SetPreparationCost(GetNullPreparationCost());
			SetDataCost(GetNullDataCost());
		}
	}

	assert(kwftPreparedTable != NULL);

	// Creation de la grille de preparation
	BuildPreparedDiscretizationDataGridStats(tupleTable, kwftPreparedTable, cvBounds);

	// Nettoyage des donnees de travail initiales
	delete kwftPreparedTable;
	if (cvBounds != NULL)
		delete cvBounds;

	// Calcul si necessaire d'une evaluation
	ComputeDefaultEvaluation();
}

KWFrequencyTable*
KWAttributeStats::ComputeInitialContinuousFrequencyTableWithoutPureIntervals(const KWTupleTable* tupleTable)
{
	KWFrequencyTable* resultTable;
	int nTargetValueNumber;
	int nTargetIndex;
	int nSourceValueNumber;
	int nSource;
	int nTuple;
	const KWTuple* tuple;
	Continuous cValue;
	Continuous cRef;
	Symbol sTargetValue;
	KWDenseFrequencyVector* kwdfvSourceFrequencyVector;
	IntVector* ivSourceFrequencies;

	require(Check());
	require(GetClass()->LookupAttribute(GetAttributeName()) != NULL);
	require(GetAttributeType() == KWType::Continuous);
	require(GetTargetAttributeType() == KWType::Symbol);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == GetAttributeName());
	require(tupleTable->GetAttributeNameAt(1) == GetTargetAttributeName());

	// Memorisation du nombre de valeurs source differentes
	nSourceValueNumber = GetDescriptiveStats()->GetValueNumber();

	// Memorisation du nombre de classes cibles
	nTargetValueNumber = GetTargetDescriptiveStats()->GetValueNumber();

	// Initialisation de la taille des resultats
	resultTable = new KWFrequencyTable;
	resultTable->SetFrequencyVectorNumber(nSourceValueNumber);
	for (nSource = 0; nSource < nSourceValueNumber; nSource++)
	{
		// Acces au vecteur du partile (sense etre en representation dense)
		kwdfvSourceFrequencyVector = cast(KWDenseFrequencyVector*, resultTable->GetFrequencyVectorAt(nSource));
		ivSourceFrequencies = kwdfvSourceFrequencyVector->GetFrequencyVector();
		ivSourceFrequencies->SetSize(nTargetValueNumber);
	}

	// Dans le cas d'une variable numerique, le nombre de valeurs est le nombre d'instance.
	// Ce n'est pas le nombre de valeurs distinctes qui depend de la precision de codage des donnees intiales
	resultTable->SetInitialValueNumber(tupleTable->GetTotalFrequency());
	resultTable->SetGranularizedValueNumber(resultTable->GetInitialValueNumber());
	// Parcours de la table de tuples pour initialiser le contenu des resultats
	nSource = 0;
	cRef = Continuous();
	nTargetIndex = 0;
	for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
	{
		tuple = tupleTable->GetAt(nTuple);

		// Acces a la valeur
		cValue = tuple->GetContinuousAt(0);
		assert(nSource == 0 or cValue >= cRef);

		// Test si premiere valeur, ou si valeur differente
		if (nTuple == 0 or cValue != cRef)
		{
			// Preparation pour la modalite suivante
			nSource++;
			cRef = cValue;
		}

		// Recherche de l'index de la classe cible
		nTargetIndex = GetTargetValueStats()->GetAttributeAt(0)->ComputeSymbolPartIndex(tuple->GetSymbolAt(1));

		// Mise a jour des statistiques du vecteur de la table
		// Acces au vecteur du partile (sense etre en representation dense)
		kwdfvSourceFrequencyVector =
		    cast(KWDenseFrequencyVector*, resultTable->GetFrequencyVectorAt(nSource - 1));
		ivSourceFrequencies = kwdfvSourceFrequencyVector->GetFrequencyVector();
		ivSourceFrequencies->UpgradeAt(nTargetIndex, tuple->GetFrequency());
	}
	assert(nSource == nSourceValueNumber);
	assert(resultTable->GetTotalFrequency() == tupleTable->GetTotalFrequency());
	return resultTable;
}

KWFrequencyTable* KWAttributeStats::ComputeInitialContinuousFrequencyTable(const KWTupleTable* tupleTable)
{
	int nTuple;
	int nSource;
	const KWTuple* tuple;
	KWFrequencyTable* resultTable;
	Continuous cSourceValue;
	Continuous cSourceRef;
	Symbol sTargetValue;
	int nTupleRef;
	int nNextTupleRef;
	int nTargetIndex;
	int nTargetRef;
	boolean bMultipleTargets;
	IntVector ivIntervalFirstObjects;
	int nIntervalIndex;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencies;

	require(Check());
	require(GetClass()->LookupAttribute(GetAttributeName()) != NULL);
	require(GetTargetAttributeType() == KWType::Symbol);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == GetAttributeName());
	require(tupleTable->GetAttributeNameAt(1) == GetTargetAttributeName());

	// Comptage du nombre de valeurs se comportant differement
	// pour la loi source de la table de contingence initiale
	cSourceRef = 0;
	nTargetRef = 0;
	bMultipleTargets = false;
	nTupleRef = 0;
	nNextTupleRef = 0;
	for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
	{
		tuple = tupleTable->GetAt(nTuple);

		// Caracteristiques du nouveau tuple
		cSourceValue = tuple->GetContinuousAt(0);
		sTargetValue = tuple->GetSymbolAt(1);
		nTargetIndex = GetTargetValueStats()->GetAttributeAt(0)->ComputeSymbolPartIndex(sTargetValue);

		// Initialisation si premier tuple
		if (nTuple == 0)
		{
			nTupleRef = nTuple;
			nNextTupleRef = nTuple;
			cSourceRef = cSourceValue;
			bMultipleTargets = false;
			nTargetRef = nTargetIndex;
		}
		else
		{
			// Si changement de valeur source: on decide
			// si l'on peut fusionner ou non avec la valeur predecente
			if (cSourceValue != cSourceRef)
			{
				assert(cSourceValue > cSourceRef); // La base est censee triee
				cSourceRef = cSourceValue;

				// Si changement de valeur ou valeur avec plusieurs  cibles:
				// nouvel intervalle
				if (bMultipleTargets or nTargetIndex != nTargetRef)
				{
					ivIntervalFirstObjects.Add(nTupleRef);
					nTargetRef = nTargetIndex;

					// S'il y a eu un intervalle mono-valeur suivi d'un
					// intervalle multivaleur: deuxieme intervalle a cree
					if (bMultipleTargets and nTupleRef != nNextTupleRef)
						ivIntervalFirstObjects.Add(nNextTupleRef);

					// Le debut de l'intervalle potentiel suivant est deplace
					nTupleRef = nTuple;
				}
				nNextTupleRef = nTuple;
				bMultipleTargets = false;
			}

			// On determine s'il y a plusieurs valeurs cible
			// pour la meme valeur source
			if (nTargetIndex != nTargetRef)
				bMultipleTargets = true;
		}
	}

	// Ajout eventuel de la derniere reference
	if (ivIntervalFirstObjects.GetSize() > 0 and
	    ivIntervalFirstObjects.GetAt(ivIntervalFirstObjects.GetSize() - 1) != nTupleRef)
		ivIntervalFirstObjects.Add(nTupleRef);
	// S'il y a eu uniquement un intervalle mono-valeur suivi d'un
	// intervalle multivaleur: deux intervalles a creer
	else if (bMultipleTargets and nTupleRef != nNextTupleRef and ivIntervalFirstObjects.GetSize() == 0)
	{
		assert(nTupleRef == 0);
		assert(nNextTupleRef > nTupleRef);
		ivIntervalFirstObjects.Add(nTupleRef);
		ivIntervalFirstObjects.Add(nNextTupleRef);
	}

	// Creation de la table d'effectifs
	if (ivIntervalFirstObjects.GetSize() > 0)
	{
		resultTable = new KWFrequencyTable;
		resultTable->SetFrequencyVectorNumber(ivIntervalFirstObjects.GetSize());

		for (nSource = 0; nSource < ivIntervalFirstObjects.GetSize(); nSource++)
		{
			// Acces au vecteur de la table (sense etre en representation dense)
			kwdfvFrequencyVector =
			    cast(KWDenseFrequencyVector*, resultTable->GetFrequencyVectorAt(nSource));
			ivFrequencies = kwdfvFrequencyVector->GetFrequencyVector();
			ivFrequencies->SetSize(GetTargetDescriptiveStats()->GetValueNumber());
		}

		nIntervalIndex = ivIntervalFirstObjects.GetSize() - 1;
		nTupleRef = ivIntervalFirstObjects.GetAt(nIntervalIndex);
		for (nTuple = tupleTable->GetSize() - 1; nTuple >= 0; nTuple--)
		{
			tuple = tupleTable->GetAt(nTuple);

			// Caracteristiques de l'objet
			sTargetValue = tuple->GetSymbolAt(1);
			nTargetIndex = GetTargetValueStats()->GetAttributeAt(0)->ComputeSymbolPartIndex(sTargetValue);

			// Test de changement d'intervalle
			if (nTuple < nTupleRef)
			{
				assert(nIntervalIndex >= 1);
				nIntervalIndex--;
				nTupleRef = ivIntervalFirstObjects.GetAt(nIntervalIndex);
			}
			assert(nTuple >= nTupleRef);

			// Mise a jour des statistiques du vecteur de la table
			// Acces au vecteur du partile (sense etre en representation dense)
			kwdfvFrequencyVector =
			    cast(KWDenseFrequencyVector*, resultTable->GetFrequencyVectorAt(nIntervalIndex));
			ivFrequencies = kwdfvFrequencyVector->GetFrequencyVector();
			ivFrequencies->UpgradeAt(nTargetIndex, tuple->GetFrequency());
		}
		assert(resultTable->GetTotalFrequency() == tupleTable->GetTotalFrequency());
		return resultTable;
	}
	return NULL;
}

ContinuousVector* KWAttributeStats::ComputeInitialSourceValues(const KWTupleTable* tupleTable)
{
	ContinuousVector* cvSourceValues;
	int nValue;
	int nTuple;
	const KWTuple* tuple;
	int i;
	Continuous cSourceValue;

	require(Check());
	require(GetClass()->LookupAttribute(GetAttributeName()) != NULL);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == GetAttributeName());

	// Initialisation du vecteur resultat
	cvSourceValues = new ContinuousVector;
	cvSourceValues->SetSize(tupleTable->GetTotalFrequency());

	// Recopie des valeurs sources a partir des tuples
	nValue = 0;
	for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
	{
		tuple = tupleTable->GetAt(nTuple);

		// Transfert de la valeur du tuple au vecteur, selon l'effectif du tuple
		cSourceValue = tuple->GetContinuousAt(0);
		assert(nTuple == 0 or cSourceValue > tupleTable->GetAt(nTuple - 1)->GetContinuousAt(0) or
		       (cSourceValue >= tupleTable->GetAt(nTuple - 1)->GetContinuousAt(0) and
			tupleTable->GetAttributeNumber() > 1));
		for (i = 0; i < tuple->GetFrequency(); i++)
		{
			cvSourceValues->SetAt(nValue, cSourceValue);
			nValue++;
		}
	}
	assert(nValue == cvSourceValues->GetSize());
	return cvSourceValues;
}

IntVector* KWAttributeStats::ComputeInitialTargetIndexes(const KWTupleTable* tupleTable)
{
	IntVector* ivTargetIndexes;
	int nValue;
	int nTuple;
	const KWTuple* tuple;
	int i;
	Symbol sTargetValue;
	int nTargetIndex;

	require(Check());
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(1) == GetTargetAttributeName());

	// Initialisation du vecteur resultat
	ivTargetIndexes = new IntVector;
	ivTargetIndexes->SetSize(tupleTable->GetTotalFrequency());

	// Recopie des index de valeurs cibles a partir des tuples
	nValue = 0;
	for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
	{
		tuple = tupleTable->GetAt(nTuple);

		// Transfert de l'index de la valeur cible du tuple au vecteur, selon l'effectif du tuple
		sTargetValue = tuple->GetSymbolAt(1);
		nTargetIndex = GetTargetValueStats()->GetAttributeAt(0)->ComputeSymbolPartIndex(sTargetValue);
		for (i = 0; i < tuple->GetFrequency(); i++)
		{
			ivTargetIndexes->SetAt(nValue, nTargetIndex);
			nValue++;
		}
	}
	assert(nValue == ivTargetIndexes->GetSize());
	return ivTargetIndexes;
}

void KWAttributeStats::BuildPreparedDiscretizationDataGridStats(const KWTupleTable* tupleTable,
								const KWFrequencyTable* kwftDiscretizedTable,
								const ContinuousVector* cvBounds)
{
	KWDGSAttributeDiscretization* attributeDiscretization;
	KWDGSAttributeSymbolValues* attributeSymbolValues;
	int nIntervalFrequency;
	Continuous cBound1;
	Continuous cBound2;
	Continuous cIntervalBound;
	int nTuple;
	const KWTuple* tuple;
	int nIntervalIndex;
	int nFrequency;
	int nSource;
	int nTarget;
	KWDenseFrequencyVector* kwdfvSourceFrequencyVector;
	IntVector* ivSourceFrequencies;

	require(Check());
	require(GetClass()->LookupAttribute(GetAttributeName()) != NULL);
	require(GetClass()->LookupAttribute(GetAttributeName())->GetType() == KWType::Continuous);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == GetAttributeName());
	require(tupleTable->GetTotalFrequency() == kwftDiscretizedTable->GetTotalFrequency());
	require(kwftDiscretizedTable != NULL);
	require(kwftDiscretizedTable->GetTotalFrequency() == tupleTable->GetTotalFrequency());
	require(kwftDiscretizedTable->GetFrequencyVectorNumber() >= 1);
	require(cvBounds != NULL or kwftDiscretizedTable->CheckNoEmptyFrequencyVectors());
	require(cvBounds == NULL or cvBounds->GetSize() == kwftDiscretizedTable->GetFrequencyVectorNumber() - 1);
	require(cvBounds == NULL or cvBounds->GetSize() == 0 or
		tupleTable->GetAt(0)->GetContinuousAt(0) <= cvBounds->GetAt(0));
	require(cvBounds == NULL or cvBounds->GetSize() == 0 or
		cvBounds->GetAt(cvBounds->GetSize() - 1) <
		    tupleTable->GetAt(tupleTable->GetSize() - 1)->GetContinuousAt(0));
	require(preparedDataGridStats == NULL);

	// Creation de la partition source
	attributeDiscretization = new KWDGSAttributeDiscretization;
	attributeDiscretization->SetAttributeName(GetAttributeName());
	attributeDiscretization->SetPartNumber(kwftDiscretizedTable->GetFrequencyVectorNumber());

	// Initialisation du nombre initial de valeurs
	attributeDiscretization->SetInitialValueNumber(kwftDiscretizedTable->GetInitialValueNumber());
	// Initialisation du nombre de valeurs apres granularisation
	attributeDiscretization->SetGranularizedValueNumber(kwftDiscretizedTable->GetGranularizedValueNumber());

	// Parcours des valeurs (intervalles) de la table discretisee
	// pour parametrer la partition source (discretisation) de la grille preparee
	// Cas avec vecteur de bornes
	if (cvBounds != NULL)
	{
		for (nIntervalIndex = 0; nIntervalIndex < kwftDiscretizedTable->GetFrequencyVectorNumber() - 1;
		     nIntervalIndex++)
		{
			assert(nIntervalIndex == 0 or
			       cvBounds->GetAt(nIntervalIndex) > cvBounds->GetAt(nIntervalIndex - 1));
			attributeDiscretization->SetIntervalBoundAt(nIntervalIndex, cvBounds->GetAt(nIntervalIndex));
		}
	}
	// Cas sans vecteur de bornes
	else
	{
		nTuple = 0;
		tuple = NULL;
		for (nIntervalIndex = 0; nIntervalIndex < kwftDiscretizedTable->GetFrequencyVectorNumber() - 1;
		     nIntervalIndex++)
		{
			// La frequence de l'intervalle est extrait de la table de contingence
			nIntervalFrequency =
			    kwftDiscretizedTable->GetFrequencyVectorAt(nIntervalIndex)->ComputeTotalFrequency();

			// Borne sup de l'intervalle en cours en recherchant a partir du tuple courant
			// le prochain tuple permettant d'atteindre l'effectif de l'intervalle
			nFrequency = 0;
			while (nFrequency < nIntervalFrequency)
			{
				tuple = tupleTable->GetAt(nTuple);
				nTuple++;
				nFrequency += tuple->GetFrequency();
			}
			assert(nFrequency == nIntervalFrequency);

			// Valeur sup de l'intervalle courant
			cBound1 = tuple->GetContinuousAt(0);

			// Valeur inf de l'intervalle suivant
			cBound2 = tupleTable->GetAt(nTuple)->GetContinuousAt(0);
			assert(cBound2 > cBound1);

			// Calcul de la borne sup de l'intervalle courant, comme moyenne de la valeur
			// des deux objets de part et d'autre de l'intervalle
			cIntervalBound = KWContinuous::GetHumanReadableLowerMeanValue(cBound1, cBound2);

			// Borne retenue: moyenne des deux bornes
			attributeDiscretization->SetIntervalBoundAt(nIntervalIndex, cIntervalBound);
		}
	}

	// Parametrage de la grille
	// Cas non supervise
	if (GetTargetAttributeType() == KWType::None)
	{
		assert(kwftDiscretizedTable->GetFrequencyVectorSize() == 1);

		// Creation de la grille de preparation
		preparedDataGridStats = new KWDataGridStats;
		preparedDataGridStats->AddAttribute(attributeDiscretization);

		// Parametrage des effectifs des cellules de la grille
		preparedDataGridStats->CreateAllCells();
		for (nSource = 0; nSource < kwftDiscretizedTable->GetFrequencyVectorNumber(); nSource++)
		{
			preparedDataGridStats->SetUnivariateCellFrequencyAt(
			    nSource, kwftDiscretizedTable->GetFrequencyVectorAt(nSource)->ComputeTotalFrequency());
		}
	}
	// Cas supervise
	else
	{
		// Creation de la partition cible
		attributeSymbolValues =
		    cast(KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0)->Clone());

		// Creation de la grille de preparation
		preparedDataGridStats = new KWDataGridStats;
		preparedDataGridStats->AddAttribute(attributeDiscretization);
		preparedDataGridStats->AddAttribute(attributeSymbolValues);
		preparedDataGridStats->SetSourceAttributeNumber(1);
		preparedDataGridStats->SetMainTargetModalityIndex(GetMainTargetModalityIndex());

		// Memorisation de la granularite
		preparedDataGridStats->SetGranularity(kwftDiscretizedTable->GetGranularity());

		// Parametrage des effectifs des cellules de la grille
		preparedDataGridStats->CreateAllCells();
		for (nSource = 0; nSource < kwftDiscretizedTable->GetFrequencyVectorNumber(); nSource++)
		{
			// Acces au vecteur du partile (sense etre en representation dense)
			kwdfvSourceFrequencyVector =
			    cast(KWDenseFrequencyVector*, kwftDiscretizedTable->GetFrequencyVectorAt(nSource));
			ivSourceFrequencies = kwdfvSourceFrequencyVector->GetFrequencyVector();

			for (nTarget = 0; nTarget < kwftDiscretizedTable->GetFrequencyVectorSize(); nTarget++)
			{
				preparedDataGridStats->SetBivariateCellFrequencyAt(nSource, nTarget,
										   ivSourceFrequencies->GetAt(nTarget));
			}
		}
	}
	ensure(preparedDataGridStats->ComputeGridFrequency() == kwftDiscretizedTable->GetTotalFrequency());
}

void KWAttributeStats::Group(const KWTupleTable* tupleTable)
{
	KWGrouperMODLFamily* grouperMODLFamily;
	KWAttribute* attribute;
	SymbolVector* svInitialSourceModalities;
	KWFrequencyTable* kwftInitialTable;
	KWFrequencyTable* kwftPreparedTable;
	IntVector* ivGroups;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;
	int i;

	require(Check());
	require(GetTargetAttributeType() == KWType::Symbol);
	require(tupleTable->GetTotalFrequency() > 0);
	require(GetAttributeType() == KWType::Symbol);
	require(preparedDataGridStats == NULL);

	// Initialisation du cout de construction, quelque soit la methode
	attribute = GetClass()->LookupAttribute(GetAttributeName());
	check(attribute);
	SetConstructionCost(GetNullConstructionCost() + attribute->GetCost());

	// Creation d'une table initiale d'effectifs et du vecteur des valeurs
	// de l'attribut source associe par parcours de la base triee
	BuildInitialFrequencyTable(tupleTable, GetDescriptiveStats()->GetValueNumber(), svInitialSourceModalities,
				   kwftInitialTable);

	// Tri par frequence decroissante (pour des raisons de reporting et pour
	// preparer le travail des algorithmes de groupage)
	kwftInitialTable->SortTableAndModalitiesBySourceFrequency(svInitialSourceModalities, false, NULL, NULL);

	// Utilisation du groupeur specifie dans les pretraitements, si au moins 2 modalites sources et cibles
	kwftPreparedTable = NULL;
	if (kwftInitialTable->GetFrequencyVectorNumber() > 1 and GetTargetDescriptiveStats()->GetValueNumber() > 1)
	{
		// Recherche si c'est la methode MODL qui est utilisee
		grouperMODLFamily = NULL;
		if (GetPreprocessingSpec()->GetGrouperSpec()->GetGrouper(GetTargetAttributeType())->IsMODLFamily())
		{
			grouperMODLFamily =
			    cast(KWGrouperMODLFamily*,
				 GetPreprocessingSpec()->GetGrouperSpec()->GetGrouper(GetTargetAttributeType()));

			// Parametrage du cout de l'attribut
			grouperMODLFamily->GetGroupingCosts()->SetAttributeCost(attribute->GetCost());
		}

		// Utilisation du grouper specifie dans les pretraitements
		GetPreprocessingSpec()
		    ->GetGrouperSpec()
		    ->GetGrouper(GetTargetAttributeType())
		    ->Group(kwftInitialTable, kwftPreparedTable, ivGroups);

		// Memorisation des couts MODL
		if (grouperMODLFamily != NULL)
		{
			// CH V9 TODO faire porter le nombre de modalites par la table dans le cadre du refactoring des
			// methodes de couts Verification avant remplacement
			assert(kwftPreparedTable->GetGranularizedValueNumber() == ivGroups->GetSize());
			SetConstructionCost(
			    grouperMODLFamily->ComputeGroupingConstructionCost(kwftPreparedTable, ivGroups->GetSize()));
			SetPreparationCost(
			    grouperMODLFamily->ComputeGroupingPreparationCost(kwftPreparedTable, ivGroups->GetSize()));
			SetDataCost(grouperMODLFamily->ComputeGroupingDataCost(kwftPreparedTable, ivGroups->GetSize()));
		}
	}
	// Si une seule modalite source ou cible, pas de groupage effectif
	else
	{
		kwftPreparedTable = new KWFrequencyTable;
		kwftPreparedTable->SetFrequencyVectorNumber(1);

		// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftPreparedTable->GetFrequencyVectorAt(0));
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();

		ivFrequencyVector->SetSize(GetTargetDescriptiveStats()->GetValueNumber());

		// Alimentation de cette ligne par les frequences globales des valeurs cibles
		assert(GetTargetDescriptiveStats()->GetValueNumber() == GetTargetValueStats()->ComputeTargetGridSize());
		for (i = 0; i < GetTargetDescriptiveStats()->GetValueNumber(); i++)
		{
			ivFrequencyVector->SetAt(i, GetTargetValueStats()->GetUnivariateCellFrequencyAt(i));
		}

		kwftPreparedTable->SetInitialValueNumber(kwftInitialTable->GetInitialValueNumber());
		kwftPreparedTable->SetGranularizedValueNumber(kwftInitialTable->GetGranularizedValueNumber());

		// Creation d'un tableau d'index elementaire dans ce cas ou aucun groupage
		// effectif n'a ete realise
		ivGroups = new IntVector;
		ivGroups->SetSize(kwftInitialTable->GetFrequencyVectorNumber());
		for (i = 0; i < ivGroups->GetSize(); i++)
			ivGroups->SetAt(i, 0);

		// Memorisation de cout MODL du modele nul si necessaire
		if (GetPreprocessingSpec()->GetGrouperSpec()->GetGrouper(GetTargetAttributeType())->IsMODLFamily())
		{
			SetConstructionCost(GetNullConstructionCost());
			SetPreparationCost(GetNullPreparationCost());
			SetDataCost(GetNullDataCost());
		}
	}
	assert(kwftPreparedTable != NULL);
	assert(ivGroups != NULL);

	// Creation de la grille de preparation
	BuildPreparedGroupingDataGridStats(svInitialSourceModalities, kwftPreparedTable, ivGroups);

	// Nettoyage des donnees de travail initiales
	delete ivGroups;
	delete svInitialSourceModalities;
	delete kwftInitialTable;
	delete kwftPreparedTable;

	// Calcul si necessaire d'une evaluation
	ComputeDefaultEvaluation();
}

void KWAttributeStats::BuildInitialFrequencyTable(const KWTupleTable* tupleTable, int nSourceValueNumber,
						  SymbolVector*& svInitialSourceModalities,
						  KWFrequencyTable*& kwftInitialTable) const
{
	int nTargetValueNumber;
	int nTargetIndex;
	int nSource;
	int nTuple;
	const KWTuple* tuple;
	Symbol sValue;
	Symbol sTargetValue;
	Symbol sRef;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;

	require(nSourceValueNumber >= 0);
	require(GetAttributeType() == KWType::Symbol);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == GetAttributeName());
	require(GetTargetAttributeName() == "" or tupleTable->GetAttributeNameAt(1) == GetTargetAttributeName());

	// Memorisation du nombre de classes cibles (1 dans le cas non supervise)
	nTargetValueNumber = 1;
	if (GetTargetAttributeName() != "")
		nTargetValueNumber = GetTargetDescriptiveStats()->GetValueNumber();

	// Initialisation de la taille des resultats
	svInitialSourceModalities = new SymbolVector;
	svInitialSourceModalities->SetSize(nSourceValueNumber);
	kwftInitialTable = new KWFrequencyTable;
	kwftInitialTable->SetFrequencyVectorNumber(nSourceValueNumber);
	// Initialisation par defaut des nombres de valeurs
	kwftInitialTable->SetInitialValueNumber(nSourceValueNumber);
	kwftInitialTable->SetGranularizedValueNumber(nSourceValueNumber);

	// Parametrage de la taille des vecteurs de la table d'effectifs
	for (nSource = 0; nSource < nSourceValueNumber; nSource++)
	{
		// Acces au vecteur (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftInitialTable->GetFrequencyVectorAt(nSource));

		// Recopie de son contenu
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();
		ivFrequencyVector->SetSize(nTargetValueNumber);
	}

	// Parcours de la base pour initialiser le contenu des resultats
	nSource = 0;
	sRef.Reset();
	nTargetIndex = 0;
	for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
	{
		tuple = tupleTable->GetAt(nTuple);

		// Acces a la valeur
		sValue = tuple->GetSymbolAt(0);
		assert(nSource == 0 or sValue >= sRef);

		// Test si premiere valeur, ou si valeur differente
		if (nTuple == 0 or sValue != sRef)
		{
			// Ajout d'une modalite source
			svInitialSourceModalities->SetAt(nSource, sValue);

			// Preparation pour la modalite suivante
			nSource++;
			sRef = sValue;
		}

		// Recherche de l'index de la classe cible
		if (nTargetValueNumber > 1)
			nTargetIndex =
			    GetTargetValueStats()->GetAttributeAt(0)->ComputeSymbolPartIndex(tuple->GetSymbolAt(1));

		// Mise a jour des statistiques dans la table d'effectifs
		cast(KWDenseFrequencyVector*, kwftInitialTable->GetFrequencyVectorAt(nSource - 1))
		    ->GetFrequencyVector()
		    ->UpgradeAt(nTargetIndex, tuple->GetFrequency());
	}
	assert(nSource == nSourceValueNumber);
}

void KWAttributeStats::BuildPreparedGroupingDataGridStats(SymbolVector* svInitialSourceModalities,
							  KWFrequencyTable* kwftGroupedTable, IntVector* ivGroups)
{
	KWDGSAttributeGrouping* attributeGrouping;
	KWDGSAttributeSymbolValues* attributeSymbolValues;
	int nGroupNumber;
	ObjectArray oaGroups;
	SymbolVector* svGroupValues;
	int nGroup;
	Symbol sValue;
	int nValue;
	int nValueNumber;
	int nIndex;
	int nSource;
	int nTarget;
	boolean bDisplayResults = false;
	boolean bCatchAllPresence;
	IntVector ivGroupSizes;
	int nGroupSizeMax;
	int nGarbageGroupIndex;
	int nSuppressedValueNumber;

	require(Check());
	require(preparedDataGridStats == NULL);
	require(svInitialSourceModalities != NULL);
	require(kwftGroupedTable != NULL);
	require(ivGroups != NULL);
	require(1 <= kwftGroupedTable->GetFrequencyVectorNumber() and
		kwftGroupedTable->GetFrequencyVectorNumber() <= ivGroups->GetSize());
	require(ivGroups->GetSize() <= svInitialSourceModalities->GetSize());
	require(ivGroups->GetSize() == kwftGroupedTable->GetGranularizedValueNumber());
	require(svInitialSourceModalities->GetSize() == kwftGroupedTable->GetInitialValueNumber());

	// Creation de la partition source
	attributeGrouping = new KWDGSAttributeGrouping;
	attributeGrouping->SetAttributeName(GetAttributeName());
	nGroupNumber = kwftGroupedTable->GetFrequencyVectorNumber();
	attributeGrouping->SetPartNumber(nGroupNumber);
	// Initialisation du nombre initial de valeurs
	attributeGrouping->SetInitialValueNumber(kwftGroupedTable->GetInitialValueNumber());
	// Initialisation du nombre de valeurs apres granularisation
	attributeGrouping->SetGranularizedValueNumber(kwftGroupedTable->GetGranularizedValueNumber());

	// Creation de groupes de valeurs vides initialement
	oaGroups.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		oaGroups.SetAt(nGroup, new SymbolVector);

	// La presence d'un fourre-tout est detectee si le nombre de modalites apres granularisation est inferieur au
	// nombre de modalites initial Dans ce cas les dernieres modalites de svInitialSourceModalites ne sont pas
	// memorises car le fourre-tout est decrit par une modalite + StarValue En mode V8, il peut y avoir un
	// fourre-tout suite au pretraitement BuildPreprocessedTable
	bCatchAllPresence =
	    attributeGrouping->GetGranularizedValueNumber() < attributeGrouping->GetInitialValueNumber();

	nSuppressedValueNumber = 0;
	if (kwftGroupedTable->GetGarbageModalityNumber() > 0)
	{
		ivGroupSizes.SetSize(nGroupNumber);
	}

	// Initialisation des valeurs des groupes
	// Parcours des modalites en s'arretant a la premiere modalite du fourre tout
	for (nValue = 0; nValue < ivGroups->GetSize(); nValue++)
	{
		// Recherche des caracteristiques de la modalite source
		sValue = svInitialSourceModalities->GetAt(nValue);
		nGroup = ivGroups->GetAt(nValue);
		assert(0 <= nGroup and nGroup < nGroupNumber);

		// Ajout de la valeur dans le groupe correspondant
		svGroupValues = cast(SymbolVector*, oaGroups.GetAt(nGroup));
		svGroupValues->Add(sValue);

		// Incrementation du nombre de modalites du groupe pour calculer ensuite le groupe poubelle comme celui
		// qui en contient le plus
		if (kwftGroupedTable->GetGarbageModalityNumber() > 0)
			ivGroupSizes.SetAt(nGroup, ivGroupSizes.GetAt(nGroup) + 1);

		// Traitement specifique pour la derniere modalite i.e la modalite de plus faible effectif
		if (nValue == ivGroups->GetSize() - 1)
		{
			// Cas de non-presence d'une poubelle
			if (kwftGroupedTable->GetGarbageModalityNumber() == 0 or bCatchAllPresence)
			{
				// Ajout de la modalite etoile
				svGroupValues->Add(Symbol::GetStarValue());

				// Cas d'un fourre-tout : memorisation du nombre de modalites du fourre tout
				if (bCatchAllPresence)
					attributeGrouping->SetCatchAllValueNumber(svInitialSourceModalities->GetSize() -
										  ivGroups->GetSize());
			}
		}
	}

	// Traitement specifique pour la modalite speciale
	// Si presence poubelle mais pas de granularite
	// Le groupe poubelle est celui qui contient le plus de modalites
	if (kwftGroupedTable->GetGarbageModalityNumber() > 0)
	{
		// Recherche de l'index du groupe poubelle qui est celui dont le groupe contient le plus de modalites
		// (modalites apres granularisation eventuelle)
		nGroupSizeMax = 0;
		nGarbageGroupIndex = 0;
		for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		{
			if (ivGroupSizes.GetAt(nGroup) > nGroupSizeMax)
			{
				nGarbageGroupIndex = nGroup;
				nGroupSizeMax = ivGroupSizes.GetAt(nGroup);
			}
		}
		attributeGrouping->SetGarbageGroupIndex(nGarbageGroupIndex);

		// Memorisation du nombre de modalites du groupe poubelle
		// il s'agit du nombre de modalites apres granularisation qui ne tient pas compte des modalites
		// eventuelles dans le fourre-tout si le groupe poubelle contient le fourre-tout
		attributeGrouping->SetGarbageModalityNumber(nGroupSizeMax);

		// Extraction du groupe poubelle
		svGroupValues = cast(SymbolVector*, oaGroups.GetAt(nGarbageGroupIndex));

		// Cas d'une poubelle sans fourre-tout : nettoyage du groupe poubelle
		if (not bCatchAllPresence)
		{
			// Evaluation du nombre de modalites a nettoyer (pour pouvoir maintenir le nombre total de
			// modalites non nettoyees)
			nSuppressedValueNumber = svGroupValues->GetSize() - 1;

			// Nettoyage des modalites autres que la premiere inseree qui est celle d'effectif le plus eleve
			svGroupValues->SetSize(1);

			// Ajout de la modalite etoile
			svGroupValues->Add(Symbol::GetStarValue());
		}
	}

	// Parametrage de la partition source
	nValueNumber = ivGroups->GetSize() + 1 - nSuppressedValueNumber;
	attributeGrouping->SetKeptValueNumber(nValueNumber);
	nValue = 0;
	for (nGroup = 0; nGroup < attributeGrouping->GetGroupNumber(); nGroup++)
	{
		// Parametrage du groupe
		svGroupValues = cast(SymbolVector*, oaGroups.GetAt(nGroup));
		attributeGrouping->SetGroupFirstValueIndexAt(nGroup, nValue);
		for (nIndex = 0; nIndex < svGroupValues->GetSize(); nIndex++)
		{
			attributeGrouping->SetValueAt(nValue, svGroupValues->GetAt(nIndex));
			nValue++;
		}
	}
	if (bDisplayResults)
	{
		cout << "AttributeGrouping " << attributeGrouping->GetAttributeName() << " nKeptValueNumber "
		     << attributeGrouping->GetKeptValueNumber() << " nGarbageNumber "
		     << attributeGrouping->GetGarbageModalityNumber() << endl;
	}

	// Nettoyage
	oaGroups.DeleteAll();

	// Cas non supervise
	if (GetTargetAttributeType() == KWType::None)
	{
		assert(kwftGroupedTable->GetFrequencyVectorSize() == 1);

		// Creation de la grille de preparation
		preparedDataGridStats = new KWDataGridStats;
		preparedDataGridStats->AddAttribute(attributeGrouping);

		// Parametrage des effectifs des cellules de la grille
		preparedDataGridStats->CreateAllCells();
		for (nSource = 0; nSource < kwftGroupedTable->GetFrequencyVectorNumber(); nSource++)
		{
			preparedDataGridStats->SetUnivariateCellFrequencyAt(
			    nSource, cast(KWDenseFrequencyVector*, kwftGroupedTable->GetFrequencyVectorAt(nSource))
					 ->GetFrequencyVector()
					 ->GetAt(0));
		}
	}
	// Cas supervise
	else
	{
		// Creation de la partition cible
		attributeSymbolValues =
		    cast(KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0)->Clone());

		// Creation de la grille de preparation
		preparedDataGridStats = new KWDataGridStats;
		// Memorisation de la granularite
		preparedDataGridStats->SetGranularity(kwftGroupedTable->GetGranularity());

		preparedDataGridStats->AddAttribute(attributeGrouping);
		preparedDataGridStats->AddAttribute(attributeSymbolValues);
		preparedDataGridStats->SetSourceAttributeNumber(1);
		preparedDataGridStats->SetMainTargetModalityIndex(GetMainTargetModalityIndex());

		// Parametrage des effectifs des cellules de la grille
		preparedDataGridStats->CreateAllCells();
		for (nSource = 0; nSource < kwftGroupedTable->GetFrequencyVectorNumber(); nSource++)
		{
			for (nTarget = 0; nTarget < kwftGroupedTable->GetFrequencyVectorSize(); nTarget++)
			{
				preparedDataGridStats->SetBivariateCellFrequencyAt(
				    nSource, nTarget,
				    cast(KWDenseFrequencyVector*, kwftGroupedTable->GetFrequencyVectorAt(nSource))
					->GetFrequencyVector()
					->GetAt(nTarget));
			}
		}
	}
	ensure(preparedDataGridStats->ComputeGridFrequency() == kwftGroupedTable->GetTotalFrequency());
}

void KWAttributeStats::Group2D(const KWTupleTable* tupleTable)
{
	KWAttributeSubsetStats* attributeSubsetStats;

	require(Check());
	require(GetTargetAttributeName() != "");
	require(tupleTable->GetTotalFrequency() > 0);
	require(preparedDataGridStats == NULL);
	require(dPreparedLevel == 0);

	// Creation et initialisation d'un objet de stats pour le couple
	// (attribut source, attribut cible)
	attributeSubsetStats = new KWAttributeSubsetStats;
	attributeSubsetStats->SetLearningSpec(GetLearningSpec());
	attributeSubsetStats->SetAttributeNumber(1);
	attributeSubsetStats->SetAttributeNameAt(0, GetAttributeName());
	attributeSubsetStats->SetTargetAttributePartitioned(true);

	// Calcul des stats
	attributeSubsetStats->ComputeStats(tupleTable);

	// Rappatriement des couts MODL
	SetConstructionCost(attributeSubsetStats->GetConstructionCost());
	SetPreparationCost(attributeSubsetStats->GetPreparationCost());
	SetDataCost(attributeSubsetStats->GetDataCost());

	// On memorise uniquement les attributs ayant une grille en deux attributs, dont un source
	if (attributeSubsetStats->IsStatsComputed() and attributeSubsetStats->GetPreparedDataGridStats() != NULL and
	    attributeSubsetStats->GetPreparedDataGridStats()->GetAttributeNumber() == 2 and
	    attributeSubsetStats->GetPreparedDataGridStats()->GetSourceAttributeNumber() == 1)
	{
		// Memorisation de la grille de preparation
		preparedDataGridStats = attributeSubsetStats->GetPreparedDataGridStats()->Clone();
	}

	// Nettoyage
	delete attributeSubsetStats;

	// Calcul si necessaire d'une evaluation
	ComputeDefaultEvaluation();
}

void KWAttributeStats::ComputeDefaultEvaluation()
{
	KWFrequencyTable preparedTable;
	double dTargetEntropy;
	double dMutualEntropy;
	const double dEpsilon = 1e-10;
	IntVector ivTargetFrequencies;

	// Evaluation nulle si pas d'information
	if (preparedDataGridStats == NULL or preparedDataGridStats->ComputeInformativeAttributeNumber() == 0)
		dPreparedLevel = 0;
	// Sinon
	else
	{
		// Cas de presence de couts MODL (hors couts de construction, definis pour toutes methdoes)
		if (dPreparationCost > 0 or dDataCost > 0)
			ComputeLevel();
		// Sinon, on se rabat sur un calcul d'entropie
		else
		{
			// On passe par une table de contingence intermediaire
			preparedTable.ImportDataGridStats(preparedDataGridStats);

			// Calcul du vecteur des effectifs par modalites cible
			preparedTable.ComputeTargetFrequencies(&ivTargetFrequencies);

			// Calcul des entropies mutuelles et cibles
			dTargetEntropy = preparedTable.ComputeTargetEntropy(&ivTargetFrequencies);
			dMutualEntropy = preparedTable.ComputeMutualEntropy(&ivTargetFrequencies);
			assert(dMutualEntropy - dTargetEntropy <= dEpsilon);

			// Evaluation d'un taux de compression
			if (dTargetEntropy <= 0)
				dPreparedLevel = 1;
			else
				dPreparedLevel = dMutualEntropy / dTargetEntropy;

			// Correction autour de zero si necessaire
			if (fabs(dPreparedLevel) < dEpsilon)
				dPreparedLevel = 0;
		}
	}
}

int KWAttributeStatsCompareLevel(const void* elem1, const void* elem2)
{
	KWAttributeStats* attributeStats1;
	KWAttributeStats* attributeStats2;
	int nCompare;

	// Acces aux objects
	attributeStats1 = cast(KWAttributeStats*, *(Object**)elem1);
	attributeStats2 = cast(KWAttributeStats*, *(Object**)elem2);
	assert(attributeStats1 != NULL);
	assert(attributeStats2 != NULL);
	assert(attributeStats1->Check());
	assert(attributeStats2->Check());

	// Comparaison selon la precison du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(attributeStats1->GetLevel(), attributeStats2->GetLevel());

	// Comparaison par nom si match nul
	if (nCompare == 0)
		nCompare = attributeStats1->GetAttributeName().Compare(attributeStats2->GetAttributeName());

	return nCompare;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_AttributeStats

PLShared_AttributeStats::PLShared_AttributeStats() {}

PLShared_AttributeStats::~PLShared_AttributeStats() {}

void PLShared_AttributeStats::SetAttributeStats(KWAttributeStats* attributeStats)
{
	require(attributeStats != NULL);
	SetObject(attributeStats);
}

KWAttributeStats* PLShared_AttributeStats::GetAttributeStats()
{
	return cast(KWAttributeStats*, GetObject());
}

void PLShared_AttributeStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWAttributeStats* attributeStats;
	PLShared_DescriptiveContinuousStats sharedDescriptiveContinuousStats;
	PLShared_DescriptiveSymbolStats sharedDescriptiveSymbolStats;
	PLShared_DataGridStats shared_dataGridStat;
	const KWDiscretizer* unsupervisedDiscretiser;
	ALString sMODLUnsupervisedDiscretizerName;

	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Appel de la methode ancetre
	PLShared_DataPreparationStats::SerializeObject(serializer, o);

	// Serialisation des attributs specifiques
	attributeStats = cast(KWAttributeStats*, o);
	assert(attributeStats->GetAttributeType() != KWType::Unknown);
	serializer->PutString(attributeStats->GetAttributeName());
	serializer->PutInt(attributeStats->GetAttributeType());

	// Serialisation des stats descriptive suivant le type de l'attribut
	assert(attributeStats->GetDescriptiveStats() != NULL);
	if (attributeStats->GetAttributeType() == KWType::Symbol)
		sharedDescriptiveSymbolStats.SerializeObject(serializer, attributeStats->GetDescriptiveStats());
	else if (attributeStats->GetAttributeType() == KWType::Continuous)
		sharedDescriptiveContinuousStats.SerializeObject(serializer, attributeStats->GetDescriptiveStats());

	// Serialisation des stats par valeur dans le cas Symbol
	assert(attributeStats->GetAttributeType() != KWType::Symbol or attributeStats->symbolValueStats != NULL);
	if (attributeStats->GetAttributeType() == KWType::Symbol)
		shared_dataGridStat.SerializeObject(serializer, attributeStats->symbolValueStats);

	// Serialisation des histogrammes MODL s'il sont presents
	serializer->PutBoolean(attributeStats->modlHistogramResults != NULL);
	if (attributeStats->modlHistogramResults != NULL)
	{
		// Serialisation du nom du discretiseur a l'origine des histogrammes
		sMODLUnsupervisedDiscretizerName = attributeStats->modlHistogramResults->GetDiscretizerName();
		serializer->PutString(attributeStats->modlHistogramResults->GetDiscretizerName());

		// Recherche du discretiseur correspondant
		unsupervisedDiscretiser =
		    KWDiscretizer::LookupDiscretizer(KWType::None, sMODLUnsupervisedDiscretizerName);
		assert(unsupervisedDiscretiser->IsMODLFamily());

		// On passe par son shared object pour serialiser
		cast(KWDiscretizerMODLFamily*, unsupervisedDiscretiser)
		    ->GetMODLHistogramResultsSharedObject()
		    ->SerializeObject(serializer, attributeStats->modlHistogramResults);
	}
}

void PLShared_AttributeStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWAttributeStats* attributeStats;
	PLShared_DescriptiveContinuousStats sharedDescriptiveContinuousStats;
	PLShared_DescriptiveSymbolStats sharedDescriptiveSymbolStats;
	PLShared_DataGridStats shared_dataGridStat;
	boolean bIsStatsComputed;
	const KWDiscretizer* unsupervisedDiscretiser;
	ALString sMODLUnsupervisedDiscretizerName;

	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Appel de la methode ancetre
	PLShared_DataPreparationStats::DeserializeObject(serializer, o);

	// Deserialisation des attributs specifiques
	// On passe par la methode SetAttributeType, qui construit les sous-structure adaptee,
	// mais il ne faut pas oublier de remettre la valeur deserialisee de bIsStatsComputed
	attributeStats = cast(KWAttributeStats*, o);
	bIsStatsComputed = attributeStats->bIsStatsComputed;
	assert(attributeStats->GetAttributeType() == KWType::Unknown);
	attributeStats->SetAttributeName(serializer->GetString());
	attributeStats->SetAttributeType(serializer->GetInt());
	attributeStats->bIsStatsComputed = bIsStatsComputed;
	// Deserialization des stats descriptive suivant le type de l'attribut
	assert(attributeStats->GetDescriptiveStats() != NULL);
	if (attributeStats->GetAttributeType() == KWType::Symbol)
		sharedDescriptiveSymbolStats.DeserializeObject(serializer, attributeStats->kwDescriptiveStats);
	else if (attributeStats->GetAttributeType() == KWType::Continuous)
		sharedDescriptiveContinuousStats.DeserializeObject(serializer, attributeStats->kwDescriptiveStats);

	// Deserialization des stats par valeur dans le cas Symbol
	assert(attributeStats->GetAttributeType() != KWType::Symbol or attributeStats->symbolValueStats != NULL);
	if (attributeStats->GetAttributeType() == KWType::Symbol)
		shared_dataGridStat.DeserializeObject(serializer, attributeStats->symbolValueStats);

	// Serialisation des histogrammes MODL s'il sont present
	if (serializer->GetBoolean())
	{
		assert(attributeStats->modlHistogramResults == NULL);

		// Deserialisation du nom du discretiseru a l'origine des histogrammes
		sMODLUnsupervisedDiscretizerName = serializer->GetString();

		// Recherche du discretiseur correspondant
		unsupervisedDiscretiser =
		    KWDiscretizer::LookupDiscretizer(KWType::None, sMODLUnsupervisedDiscretizerName);
		assert(unsupervisedDiscretiser->IsMODLFamily());

		// On passe par son shared object pour deserialiser
		attributeStats->modlHistogramResults =
		    cast(KWDiscretizerMODLFamily*, unsupervisedDiscretiser)->CreateMODLHistogramResults();
		cast(KWDiscretizerMODLFamily*, unsupervisedDiscretiser)
		    ->GetMODLHistogramResultsSharedObject()
		    ->DeserializeObject(serializer, attributeStats->modlHistogramResults);
	}
}

Object* PLShared_AttributeStats::Create() const
{
	return new KWAttributeStats;
}
