// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridInitialSolutionSearcherIV.h"

KWDataGridInitialSolutionSearcherIV::KWDataGridInitialSolutionSearcherIV()
{
	learningSpec = NULL;
}

KWDataGridInitialSolutionSearcherIV::~KWDataGridInitialSolutionSearcherIV() {}

void KWDataGridInitialSolutionSearcherIV::SetLearningSpec(KWLearningSpec* specification)
{
	learningSpec = specification;
}

KWLearningSpec* KWDataGridInitialSolutionSearcherIV::GetLearningSpec() const
{
	return learningSpec;
}

void KWDataGridInitialSolutionSearcherIV::SearchInitialSolution(const KWDataGrid* initialDataGrid,
								KWDataGridMerger* initialDataGridSolution) const
{
	const boolean bTrace = true;
	const ObjectArray* oaAttributePairStats;
	KWAttributeStats* attributeStats;
	KWAttributePairStats* resultPairStats;
	KWDataGridStats* pairStats;
	const KWDGSAttributePartition* attributePartition;
	ObjectArray oaAllAttributesPartitions;
	ObjectDictionary odAllAttributesPartitions;
	ObjectArray* oaAttributePartitions;
	ObjectDictionary odInnerAttributePartitions;
	KWDGSAttributeDiscretization* attributeResultDiscretization;
	KWDGSAttributeGrouping* attributeResultGrouping;
	const KWDGSAttributePartition* attributeResultPartition;
	int n;
	int nAttribute;
	KWDataGridManager dataGridManager;
	KWDataGrid partitionnedDataGrid;

	require(learningSpec != NULL);
	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(initialDataGridSolution != NULL);
	require(initialDataGridSolution->GetCellNumber() == 0);

	// Calcul des paires de variables
	ComputeInternalAttributesBivariateStats(initialDataGrid);
	oaAttributePairStats = GetInternalAttributesBivariateStats()->GetAttributePairStats();

	// Parcours des paires pour collecter pour chaque attribut toutes les partitions
	// de l'attribut dans les paires le concernant
	for (n = 0; n < oaAttributePairStats->GetSize(); n++)
	{
		resultPairStats = cast(KWAttributePairStats*, oaAttributePairStats->GetAt(n));
		pairStats = resultPairStats->GetPreparedDataGridStats();

		// Analyse de chaque attribut des paires non nulles
		if (pairStats != NULL)
		{
			for (nAttribute = 0; nAttribute < pairStats->GetAttributeNumber(); nAttribute++)
			{
				attributePartition = pairStats->GetAttributeAt(nAttribute);

				// Recherche du tableau des partition pour cet attribut
				oaAttributePartitions =
				    cast(ObjectArray*,
					 odAllAttributesPartitions.Lookup(attributePartition->GetAttributeName()));

				// Creation si necessaire
				if (oaAttributePartitions == NULL)
				{
					oaAttributePartitions = new ObjectArray;
					odAllAttributesPartitions.SetAt(attributePartition->GetAttributeName(),
									oaAttributePartitions);
					oaAllAttributesPartitions.Add(oaAttributePartitions);
				}

				// Memorisation de la partition
				oaAttributePartitions->Add(cast(Object*, attributePartition));
			}
		}
	}

	// Extraction des partitions les plus fines pour chaque attribut, par intersection de ses partitions
	for (nAttribute = 0; nAttribute < oaAllAttributesPartitions.GetSize(); nAttribute++)
	{
		oaAttributePartitions = cast(ObjectArray*, oaAllAttributesPartitions.GetAt(nAttribute));

		// Acces a la premiere partition pour avoir le type de l'attribut
		attributePartition = cast(const KWDGSAttributePartition*, oaAttributePartitions->GetAt(0));

		// Recherche de la stats unibariee de l'attribut correspondant
		attributeStats =
		    GetInternalAttributesBivariateStats()->LookupAttributeStats(attributePartition->GetAttributeName());
		assert(attributeStats != NULL);

		// Cas d'un attribut numerique
		if (attributePartition->GetAttributeType() == KWType::Continuous)
		{
			attributeResultDiscretization = new KWDGSAttributeDiscretization;
			odInnerAttributePartitions.SetAt(attributePartition->GetAttributeName(),
							 attributeResultDiscretization);
			ComputeIntersectionDiscretizations(attributeStats, oaAttributePartitions,
							   attributeResultDiscretization);
		}
		else
		{
			attributeResultGrouping = new KWDGSAttributeGrouping;
			odInnerAttributePartitions.SetAt(attributePartition->GetAttributeName(),
							 attributeResultGrouping);
			ComputeIntersectionGroupings(attributeStats, oaAttributePartitions, attributeResultGrouping);
		}
	}

	// Creation d'une version partitionnee des attributs internes
	dataGridManager.ExportDataGridWithPartitionnedInnerAttributes(initialDataGrid, &odInnerAttributePartitions,
								      &partitionnedDataGrid);

	// Trace
	if (bTrace)
	{
		cout << "Variable partitions\t" << oaAllAttributesPartitions.GetSize() << "\n";
		for (nAttribute = 0; nAttribute < oaAllAttributesPartitions.GetSize(); nAttribute++)
		{
			// Acces aux info de l'attribut
			oaAttributePartitions = cast(ObjectArray*, oaAllAttributesPartitions.GetAt(nAttribute));
			attributePartition = cast(const KWDGSAttributePartition*, oaAttributePartitions->GetAt(0));
			attributeResultPartition =
			    cast(const KWDGSAttributePartition*,
				 odInnerAttributePartitions.Lookup(attributePartition->GetAttributeName()));

			// Affichage des ces infos
			cout << "  " << attributePartition->GetAttributeName() << "\t";
			cout << KWType::ToString(attributePartition->GetAttributeType()) << "\t";
			cout << oaAttributePartitions->GetSize() << "\n";
			cout << "  " << *attributeResultPartition << "\n";
		}

		//DDD
		//DDD cout << partitionnedDataGrid << "\n";
	}

	// Nettoyage
	oaAllAttributesPartitions.DeleteAll();
	odInnerAttributePartitions.DeleteAll();
	CleanInternalAttributesBivariateStats();
}

void KWDataGridInitialSolutionSearcherIV::ComputeInternalAttributesBivariateStats(
    const KWDataGrid* initialDataGrid) const
{
	const boolean bTrace = true;
	KWAttributePairsSpec bivariatePairSpec;
	ALString sBivariateReportPath;
	KWAttributePairName* pairName;
	int n1;
	int n2;

	require(GetLearningSpec() != NULL);
	require(initialDataGrid != NULL);

	// Parametrage des variables de travail pour  calculer les statistiques bivariees
	bivariateLearningSpec.CopyFrom(GetLearningSpec());
	bivariateLearningSpec.GetPreprocessingSpec()->SetDiscretizerUnsupervisedMethodName("none");
	bivariateLearningSpec.GetPreprocessingSpec()->SetGrouperUnsupervisedMethodName("none");
	bivariatePairSpec.SetClassName(bivariateLearningSpec.GetClass()->GetName());
	bivariateClassStats.SetLearningSpec(&bivariateLearningSpec);

	// Parametrage des paires a analyser
	bivariatePairSpec.GetSpecificAttributePairs()->DeleteAll();
	for (n1 = 0; n1 < initialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber(); n1++)
	{
		for (n2 = n1 + 1; n2 < initialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber(); n2++)
		{
			pairName = new KWAttributePairName;
			pairName->SetFirstName(
			    initialDataGrid->GetInnerAttributes()->GetInnerAttributeAt(n1)->GetAttributeName());
			pairName->SetSecondName(
			    initialDataGrid->GetInnerAttributes()->GetInnerAttributeAt(n2)->GetAttributeName());
			bivariatePairSpec.GetSpecificAttributePairs()->Add(pairName);
		}
	}
	bivariateClassStats.SetAttributePairsSpec(&bivariatePairSpec);
	bivariatePairSpec.SetMaxAttributePairNumber(bivariatePairSpec.GetSpecificAttributePairs()->GetSize());

	// Calcul des statistques sur les paires de variables
	bivariateClassStats.ComputeStats();

	// Suppression du parametrage des paires, qui est local a la methode
	bivariateClassStats.SetAttributePairsSpec(NULL);
	bivariatePairSpec.SetMaxAttributePairNumber(0);

	// Nettoyage
	bivariatePairSpec.GetSpecificAttributePairs()->DeleteAll();

	// Trace
	if (bTrace)
	{
		sBivariateReportPath =
		    FileService::BuildFilePathName(FileService::GetTmpDir(), "CoclusteringBivariate.khj");
		cout << GetLearningSpec()->GetClass()->GetName()
		     << " coclustering bivariate report: " << sBivariateReportPath << "\n";
		WriteJSONAnalysisReport(&bivariateClassStats, sBivariateReportPath);
	}
}

const KWClassStats* KWDataGridInitialSolutionSearcherIV::GetInternalAttributesBivariateStats() const
{
	require(bivariateClassStats.IsStatsComputed());
	return &bivariateClassStats;
}

void KWDataGridInitialSolutionSearcherIV::CleanInternalAttributesBivariateStats() const
{
	bivariateClassStats.DeleteAll();
}

void KWDataGridInitialSolutionSearcherIV::ComputeIntersectionDiscretizations(
    const KWAttributeStats* attributeStats, const ObjectArray* oaAttributeDiscretizations,
    KWDGSAttributeDiscretization* resultDiscretization) const
{
	const KWDGSAttributeDiscretization* attributeDiscretization;
	int n;
	int nBound;
	ContinuousVector cvAllBounds;
	ContinuousVector cvResultBounds;

	require(attributeStats != NULL);
	require(attributeStats->GetAttributeType() == KWType::Continuous);
	require(oaAttributeDiscretizations != NULL);
	require(oaAttributeDiscretizations->GetSize() > 0);
	require(resultDiscretization != NULL);

	// Initialisation de la discretisation
	resultDiscretization->SetAttributeName(attributeStats->GetAttributeName());
	resultDiscretization->SetInitialValueNumber(attributeStats->GetDescriptiveStats()->GetValueNumber());
	resultDiscretization->SetGranularizedValueNumber(attributeStats->GetDescriptiveStats()->GetValueNumber());

	// Collecte de l'ensemble de toutes les bornes pour toutes les discretisations
	for (n = 0; n < oaAttributeDiscretizations->GetSize(); n++)
	{
		attributeDiscretization =
		    cast(const KWDGSAttributeDiscretization*, oaAttributeDiscretizations->GetAt(n));
		assert(attributeDiscretization->GetAttributeName() ==
		       cast(const KWDGSAttributeDiscretization*, oaAttributeDiscretizations->GetAt(0))
			   ->GetAttributeName());

		// Memorisation des bornes
		for (nBound = 0; nBound < attributeDiscretization->GetIntervalBoundNumber(); nBound++)
			cvAllBounds.Add(attributeDiscretization->GetIntervalBoundAt(nBound));
	}

	// Tri de toutes les bornes
	cvAllBounds.Sort();

	// On garde les bornes uniques
	for (nBound = 0; nBound < cvAllBounds.GetSize(); nBound++)
	{
		if (nBound == 0 or cvAllBounds.GetAt(nBound) > cvAllBounds.GetAt(nBound - 1))
			cvResultBounds.Add(cvAllBounds.GetAt(nBound));
	}

	// Memorisation des bones des intervales
	resultDiscretization->SetPartNumber(cvResultBounds.GetSize() + 1);
	for (n = 0; n < cvResultBounds.GetSize(); n++)
		resultDiscretization->SetIntervalBoundAt(n, cvResultBounds.GetAt(n));

	ensure(resultDiscretization->GetAttributeName() == attributeStats->GetAttributeName());
	ensure(resultDiscretization->GetPartNumber() > 1);
	ensure(resultDiscretization->Check());
}

void KWDataGridInitialSolutionSearcherIV::ComputeIntersectionGroupings(const KWAttributeStats* attributeStats,
								       const ObjectArray* oaAttributeGroupings,
								       KWDGSAttributeGrouping* resultGrouping) const
{
	const boolean bTrace = true;
	const KWDGSAttributeGrouping* attributeGrouping;
	NumericKeyDictionary nkdValueSignatures;
	ObjectArray oaValueSignatures;
	KWValueSignature* valueSignature;
	SymbolVector svResultValues;
	IntVector ivResultGroupFirstValueIndexes;
	int n;
	int nGroup;
	int nValue;
	int nDefaultGroupIndex;
	Symbol sValue;

	require(attributeStats != NULL);
	require(attributeStats->GetAttributeType() == KWType::Symbol);
	require(oaAttributeGroupings != NULL);
	require(oaAttributeGroupings->GetSize() > 0);
	require(resultGrouping != NULL);

	// Initialisation du groupement de valeurs
	resultGrouping->SetAttributeName(attributeStats->GetAttributeName());
	resultGrouping->SetInitialValueNumber(attributeStats->GetDescriptiveStats()->GetValueNumber());
	resultGrouping->SetGranularizedValueNumber(attributeStats->GetDescriptiveStats()->GetValueNumber());

	// Premiere passe de collecte de toutes les valeurs pour creer les signatures
	// En effet chaque partition peut concerner des valeurs distinctes, selon la taille du groupe par defaut
	// qui ne reference pas toutes ses valeurs
	for (n = 0; n < oaAttributeGroupings->GetSize(); n++)
	{
		attributeGrouping = cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(n));
		assert(attributeGrouping->GetAttributeName() ==
		       cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(0))->GetAttributeName());

		// Analyse de la partition pour creer puis completer la signature de chaque valeur
		for (nGroup = 0; nGroup < attributeGrouping->GetGroupNumber(); nGroup++)
		{
			for (nValue = attributeGrouping->GetGroupFirstValueIndexAt(nGroup);
			     nValue <= attributeGrouping->GetGroupLastValueIndexAt(nGroup); nValue++)
			{
				sValue = attributeGrouping->GetValueAt(nValue);

				// Creation de la signature si necessaire
				valueSignature =
				    cast(KWValueSignature*, nkdValueSignatures.Lookup(sValue.GetNumericKey()));
				if (valueSignature == NULL)
				{
					valueSignature = new KWValueSignature;
					valueSignature->SetValue(sValue);

					// Enregistrement
					nkdValueSignatures.SetAt(sValue.GetNumericKey(), valueSignature);
					oaValueSignatures.Add(valueSignature);
				}
			}
		}
	}

	// Seconde passe de specification des signatures de toutes les valeurs pour l'ensemble des groupements de valeurs
	// La signature est le vecteur des index de groupes sur l'ensemble des partitions
	for (n = 0; n < oaAttributeGroupings->GetSize(); n++)
	{
		attributeGrouping = cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(n));
		assert(attributeGrouping->GetAttributeName() ==
		       cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(0))->GetAttributeName());

		// Analyse de la partition pour creer puis completer la signature de chaque valeur
		nDefaultGroupIndex = -1;
		for (nGroup = 0; nGroup < attributeGrouping->GetGroupNumber(); nGroup++)
		{
			// Parcours des valeurs decrites dans la partition
			for (nValue = attributeGrouping->GetGroupFirstValueIndexAt(nGroup);
			     nValue <= attributeGrouping->GetGroupLastValueIndexAt(nGroup); nValue++)
			{
				sValue = attributeGrouping->GetValueAt(nValue);

				// Memorisation de l'index du groupe par defaut
				if (sValue == Symbol::GetStarValue())
					nDefaultGroupIndex = nGroup;

				// Recherche de la signature sinon
				valueSignature =
				    cast(KWValueSignature*, nkdValueSignatures.Lookup(sValue.GetNumericKey()));
				assert(valueSignature != NULL);
				assert(valueSignature->GetSignature()->GetSize() == n);

				// Ajout de l'index de la partie en fin de signature
				valueSignature->GetSignature()->Add(nGroup);
			}
		}
		assert(nDefaultGroupIndex != -1);

		// On parcours maintenant toutes les valeurs pour completer la signature de celle du groupe poubelle
		// qui n'ont pas ete decrites explicitement dans la partition
		for (nValue = 0; nValue < oaValueSignatures.GetSize(); nValue++)
		{
			valueSignature = cast(KWValueSignature*, oaValueSignatures.GetAt(nValue));

			// Mise a jour de la signature avec l'index du groupe poubelle si necessaire
			if (valueSignature->GetSignature()->GetSize() == n)
				valueSignature->GetSignature()->Add(nDefaultGroupIndex);
		}
	}

	// Tri des signatures pour identifier les groupes uniques
	oaValueSignatures.SetCompareFunction(KWValueSignatureCompare);
	oaValueSignatures.Sort();

	// Parcours des signatures triee pour identifier les groupes de valeurs de l'intersection des partitions
	for (nValue = 0; nValue < oaValueSignatures.GetSize(); nValue++)
	{
		valueSignature = cast(KWValueSignature*, oaValueSignatures.GetAt(nValue));

		// Memorisation de la valeur
		svResultValues.Add(valueSignature->GetValue());

		// Memorisation d'un nouveau groupe si changement de signature
		if (nValue == 0 or
		    valueSignature->CompareSignature(cast(KWValueSignature*, oaValueSignatures.GetAt(nValue - 1))) > 0)
			ivResultGroupFirstValueIndexes.Add(ivResultGroupFirstValueIndexes.GetSize());
	}

	// Memorisation des specification du groupement de valeurs
	resultGrouping->SetKeptValueNumber(svResultValues.GetSize());
	resultGrouping->SetPartNumber(ivResultGroupFirstValueIndexes.GetSize());
	for (n = 0; n < svResultValues.GetSize(); n++)
		resultGrouping->SetValueAt(n, svResultValues.GetAt(n));
	for (n = 0; n < ivResultGroupFirstValueIndexes.GetSize(); n++)
		resultGrouping->SetGroupFirstValueIndexAt(n, ivResultGroupFirstValueIndexes.GetAt(n));

	// Trace
	if (bTrace)
	{
		attributeGrouping = cast(const KWDGSAttributeGrouping*, oaAttributeGroupings->GetAt(0));
		cout << "ComputeUnionGroups " << attributeGrouping->GetAttributeName() << "\n";
		for (nValue = 0; nValue < oaValueSignatures.GetSize(); nValue++)
		{
			valueSignature = cast(KWValueSignature*, oaValueSignatures.GetAt(nValue));
			cout << "\t" << *valueSignature << "\n";
		}
	}

	// Nettoyage
	oaValueSignatures.DeleteAll();

	ensure(resultGrouping->GetAttributeName() == attributeStats->GetAttributeName());
	ensure(resultGrouping->GetPartNumber() > 1);
	ensure(resultGrouping->Check());
}

void KWDataGridInitialSolutionSearcherIV::WriteJSONAnalysisReport(KWClassStats* classStats,
								  const ALString& sReportFileName) const
{
	JSONFile fJSON;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(sReportFileName != "");

	// Ouverture du rapport
	fJSON.SetFileName(sReportFileName);
	fJSON.OpenForWrite();

	// Ecriture
	if (fJSON.IsOpened())
	{

		// Outil et version
		fJSON.WriteKeyString("tool", GetLearningApplicationName());
		if (GetLearningModuleName() != "")
			fJSON.WriteKeyString("sub_tool", GetLearningModuleName());
		fJSON.WriteKeyString("version", GetLearningVersion());

		// Description courte
		fJSON.WriteKeyString("shortDescription", "Bivariate analysis for initialization a cocluystereing IxV");

		// Rapport de preparation complet
		classStats->SetWriteOptionStatsNativeOrConstructed(true);
		classStats->WriteJSONKeyReport(&fJSON, "preparationReport");
		classStats->SetWriteOptionStatsNativeOrConstructed(false);

		// Rapport de preparation bivarie
		if (classStats->GetAttributePairStats()->GetSize() > 0)
		{
			classStats->SetWriteOptionStats2D(true);
			classStats->WriteJSONKeyReport(&fJSON, "bivariatePreparationReport");
			classStats->SetWriteOptionStats2D(false);
		}

		// Fermeture du fichier
		fJSON.Close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Classe KWValueSignature

int KWValueSignature::Compare(const KWValueSignature* aSource) const
{
	int nCompare;

	require(aSource != NULL);
	require(ivSignature.GetSize() == aSource->ivSignature.GetSize());

	// Comparaison des signatures
	nCompare = CompareSignature(aSource);

	// Comparaison de la valeur, en prenant la StarValue en dernier
	if (nCompare == 0 and sValue != aSource->sValue)
	{
		if (sValue == Symbol::GetStarValue())
			nCompare = 1;
		else if (aSource->sValue == Symbol::GetStarValue())
			nCompare = -1;
		else
			nCompare = sValue.CompareValue(aSource->sValue);
	}
	return nCompare;
}

int KWValueSignature::CompareSignature(const KWValueSignature* aSource) const
{
	int i;
	int nCompare;

	require(aSource != NULL);

	// Comparaison de la taille des signatures
	nCompare = ivSignature.GetSize() - aSource->ivSignature.GetSize();

	// Comparaison terme a terme des vecteurs de signature
	if (nCompare == 0)
	{
		for (i = 0; i < ivSignature.GetSize(); i++)
		{
			nCompare = ivSignature.GetAt(i) - aSource->ivSignature.GetAt(i);
			if (nCompare != 0)
				break;
		}
	}
	return nCompare;
}

void KWValueSignature::Write(ostream& ost) const
{
	int i;

	ost << sValue << " [";
	for (i = 0; i < ivSignature.GetSize(); i++)
	{
		if (i > 0)
			ost << '.';
		ost << ivSignature.GetAt(i);
	}
	ost << ']';
}

int KWValueSignatureCompare(const void* elem1, const void* elem2)
{

	KWValueSignature* value1;
	KWValueSignature* value2;
	int nCompare;

	// Acces aux signatures
	value1 = (KWValueSignature*)*(Object**)elem1;
	value2 = (KWValueSignature*)*(Object**)elem2;
	assert(value1->GetSignature()->GetSize() == value2->GetSignature()->GetSize());

	// Comparaison
	nCompare = value1->Compare(value2);
	return nCompare;
}
