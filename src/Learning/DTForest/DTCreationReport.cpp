// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTCreationReport.h"

int DTTreeSpecsCompareLevels(const void* elem1, const void* elem2);

DTCreationReport::DTCreationReport()
{
	classStats = NULL;
}

DTCreationReport::~DTCreationReport()
{
	Clean();
}

void DTCreationReport::Clean()
{
	classStats = NULL;
	svCreatedTreeNames.SetSize(0);
	odCreatedTrees.DeleteAll();
}

void DTCreationReport::AddTree(const ALString& sName, DTDecisionTreeSpec* dtTree)
{
	require(sName != "");
	require(dtTree != NULL);
	require(LookupTree(sName) == NULL);

	svCreatedTreeNames.Add(sName);
	dtTree->SetTreeVariableName(sName);
	odCreatedTrees.SetAt(sName, dtTree);
}

void DTCreationReport::DeleteTree(const ALString& sName)
{
	require(sName != "");
	require(LookupTree(sName) != NULL);
	int nIndex;
	ALString sName2;
	StringVector svCreatedTreeNamesTemps;

	DTDecisionTreeSpec* treeSpec = cast(DTDecisionTreeSpec*, LookupTree(sName));
	delete treeSpec;
	odCreatedTrees.RemoveKey(sName);

	// enleve un nom d'arbre
	for (nIndex = 0; nIndex < GetTreeNumber(); nIndex++)
	{
		sName2 = GetTreeNameAt(nIndex);
		if (sName2 != sName)
			svCreatedTreeNamesTemps.Add(svCreatedTreeNames.GetAt(nIndex));
	}
	svCreatedTreeNames.CopyFrom(&svCreatedTreeNamesTemps);
}

int DTCreationReport::GetTreeNumber() const
{
	return svCreatedTreeNames.GetSize();
}

const DTDecisionTreeSpec* DTCreationReport::GetTreeAt(int nIndex) const
{
	return LookupTree(svCreatedTreeNames.GetAt(nIndex));
}

void DTCreationReport::SetLevelAt(int nIndex, double dLevel)
{
	DTDecisionTreeSpec* treeSpec = cast(DTDecisionTreeSpec*, LookupTree(svCreatedTreeNames.GetAt(nIndex)));
	treeSpec->SetLevel(dLevel);
}

const ALString& DTCreationReport::GetTreeNameAt(int nIndex) const
{
	return svCreatedTreeNames.GetAt(nIndex);
}

const DTDecisionTreeSpec* DTCreationReport::LookupTree(const ALString& sName) const
{
	return cast(const DTDecisionTreeSpec*, odCreatedTrees.Lookup(sName));
}

void DTCreationReport::SetClassStats(KWClassStats* stats)
{
	classStats = stats;
}

KWClassStats* DTCreationReport::GetClassStats() const
{
	return classStats;
}

void DTCreationReport::WriteJSONFields(JSONFile* fJSON)
{
	require(fJSON != NULL);

	// Rapports detailles
	WriteJSONTreeReport(fJSON, false);
}

void DTCreationReport::WriteJSONTreeReport(JSONFile* fJSON, boolean bSummary)
{
	ObjectArray oaSortedReports;
	int i, j, nIndex;
	const DTDecisionTreeSpec* treeSpec;
	DTDecisionTreeNodeSpec* treeNodeSpec;
	ALString sName;
	KWAttributeStats* attributeStats;

	// Recherche des rapports a ecrire, dans le cas ou on ne garde que les attributs selectionnes par le predicteur
	if (classStats != NULL and classStats->GetKeepSelectedAttributesOnly())
	{
		for (i = 0; i < classStats->GetTreeAttributeStats()->GetSize(); i++)
		{
			attributeStats = cast(KWAttributeStats*, classStats->GetTreeAttributeStats()->GetAt(i));

			// Ajout si la preparation est utilise par un predicteur
			if (classStats->GetRecursivelySelectedDataPreparationStats()->Lookup(attributeStats) != NULL)
			{
				sName = attributeStats->GetSortName();
				treeSpec = LookupTree(sName);
				oaSortedReports.Add((Object*)treeSpec);
			}
		}
	}
	// Recherche des rapports a ecrire, dans le cas sans filtrage
	else
	{
		for (nIndex = 0; nIndex < GetTreeNumber(); nIndex++)
		{
			sName = GetTreeNameAt(nIndex);
			treeSpec = LookupTree(sName);
			oaSortedReports.Add((Object*)treeSpec);
		}
	}

	// Affichage si tableau non vide
	if (oaSortedReports.GetSize() > 0)
	{
		// Calcul des rangs des arbres, avec tri des rapport
		ComputeRankIdentifiers(&oaSortedReports);

		// Ecriture du tableau des statistic global des arbres au format JSON
		// fJSON->BeginKeyArray("decisionTreeStatistics");
		// for (i = 0; i < oaSortedReports.GetSize(); i++)
		//{
		//	treeSpec2 = cast(DTDecisionTreeSpec*, oaSortedReports.GetAt(i));
		//	fJSON->BeginObject();
		//	treeSpec2->WriteJSONArrayFields(fJSON, bSummary);
		//	fJSON->EndObject();
		//}
		// fJSON->EndArray();

		// Ecriture du tableau des statistic global des arbres au format JSON
		// fJSON->BeginKeyObject("decisionTreeDetailedStatistics");
		for (i = 0; i < oaSortedReports.GetSize(); i++)
		{
			treeSpec = cast(const DTDecisionTreeSpec*, oaSortedReports.GetAt(i));

			ObjectArray oaLeavesNodes;
			for (j = 0; j < treeSpec->GetTreeNodes().GetSize(); j++)
			{
				treeNodeSpec = cast(DTDecisionTreeNodeSpec*, treeSpec->GetTreeNodes().GetAt(j));
				if (treeNodeSpec->IsLeaf())
					oaLeavesNodes.Add(treeNodeSpec);
			}

			treeNodeSpec = cast(DTDecisionTreeNodeSpec*, treeSpec->GetTreeNodes().GetAt(0));
			assert(treeNodeSpec->GetFatherNode() == NULL);
			fJSON->BeginKeyObject(treeSpec->GetRank());
			treeSpec->WriteJSONArrayFields(fJSON, bSummary);
			fJSON->BeginKeyObject("treeNodes");
			WriteJSONSonNodes(fJSON, treeNodeSpec, oaLeavesNodes);
			fJSON->EndObject();
			fJSON->EndObject();
		}
		// fJSON->EndObject();
	}
}

void DTCreationReport::WriteJSONSonNodes(JSONFile* fJSON, DTDecisionTreeNodeSpec* treeNodeSpec,
					 const ObjectArray& oaLeavesNodes) const
{
	// methode appelee recursivement
	int nPart;
	int nDefaultPartIndex;
	int nValue;
	int nFirstValue;
	int nLastValue;
	ContinuousVector cvAttributeMinValues;
	ContinuousVector cvAttributeMaxValues;
	KWDGSAttributePartition* attributePartition;
	KWDGSAttributeDiscretization* numericalAttributePartition;
	KWDGSAttributeGrouping* categoricalAttributePartition;

	if (treeNodeSpec->GetFatherNode() != NULL)
		fJSON->BeginObject();

	treeNodeSpec->WriteJSONArrayFields(fJSON, false);

	// ecriture des effectifs par valeur cible
	if (treeNodeSpec->IsLeaf())
	{
		if (treeNodeSpec->GetTargetModalitiesCountTrain() != NULL)
		{
			fJSON->BeginKeyObject("targetValues");
			fJSON->BeginKeyList("values");
			for (nValue = 0; nValue < treeNodeSpec->GetTargetModalitiesCountTrain()->GetSize(); nValue++)
			{
				DTDecisionTree::TargetModalityCount* tmc =
				    cast(DTDecisionTree::TargetModalityCount*,
					 treeNodeSpec->GetTargetModalitiesCountTrain()->GetAt(nValue));
				fJSON->WriteString(ALString(tmc->sModality));
			}
			fJSON->EndList();

			// Ecriture des effectifs
			fJSON->BeginKeyList("frequencies");
			for (nValue = 0; nValue < treeNodeSpec->GetTargetModalitiesCountTrain()->GetSize(); nValue++)
			{
				DTDecisionTree::TargetModalityCount* tmc =
				    cast(DTDecisionTree::TargetModalityCount*,
					 treeNodeSpec->GetTargetModalitiesCountTrain()->GetAt(nValue));
				fJSON->WriteLongint(tmc->iCount);
			}
			fJSON->EndList();
			fJSON->EndObject();
		}
	}
	if (!treeNodeSpec->IsLeaf())
	{
		attributePartition = treeNodeSpec->GetAttributePartitionSpec();
		if (attributePartition->GetAttributeType() == KWType::Continuous)
		{
			// cas numerical
			numericalAttributePartition =
			    cast(KWDGSAttributeDiscretization*, treeNodeSpec->GetAttributePartitionSpec());
			// numericalAttributePartition->WriteJSONFieldsWithBounds(fJSON, treeNodeSpec->GetJSONMin(),
			// treeNodeSpec->GetJSONMax());
			require(treeNodeSpec->GetJSONMin() <= treeNodeSpec->GetJSONMax());

			// Entete
			fJSON->WriteKeyString("variable", numericalAttributePartition->GetAttributeName());
			fJSON->WriteKeyString("type",
					      KWType::ToString(numericalAttributePartition->GetAttributeType()));

			// Partition
			fJSON->BeginKeyArray("partition");
			for (nPart = 0; nPart < numericalAttributePartition->GetPartNumber(); nPart++)
				numericalAttributePartition->WriteJSONPartFieldsAtWithBounds(
				    fJSON, nPart, treeNodeSpec->GetJSONMin(), treeNodeSpec->GetJSONMax());
			fJSON->EndArray();
		}
		else
		{
			// cas categorical
			// attributePartition->WriteJSONFields(fJSON);
			//  Entete

			// cas numerical
			categoricalAttributePartition =
			    cast(KWDGSAttributeGrouping*, treeNodeSpec->GetAttributePartitionSpec());

			fJSON->WriteKeyString("variable", attributePartition->GetAttributeName());
			fJSON->WriteKeyString("type", KWType::ToString(attributePartition->GetAttributeType()));

			// Partition
			if (attributePartition->ArePartsSingletons())
			{
				fJSON->BeginKeyList("partition");
				for (nPart = 0; nPart < attributePartition->GetPartNumber(); nPart++)
					attributePartition->WriteJSONPartFieldsAt(fJSON, nPart);
				fJSON->EndList();
			}
			else
			{
				fJSON->BeginKeyArray("partition");
				for (nPart = 0; nPart < attributePartition->GetPartNumber(); nPart++)
					attributePartition->WriteJSONPartFieldsAt(fJSON, nPart);
				fJSON->EndArray();
			}

			// ecriture de la partie par defaut
			// Recherche de l'index de la partie par defaut
			nDefaultPartIndex = -1;

			for (nPart = 0; nPart < categoricalAttributePartition->GetPartNumber(); nPart++)
			{
				// Recherche dans la partie courante
				nFirstValue = categoricalAttributePartition->GetGroupFirstValueIndexAt(nPart);
				nLastValue = categoricalAttributePartition->GetGroupLastValueIndexAt(nPart);
				for (nValue = nFirstValue; nValue <= nLastValue; nValue++)
				{
					if (categoricalAttributePartition->GetValueAt(nValue) == Symbol::GetStarValue())
					{
						nDefaultPartIndex = nPart;
						break;
					}
				}
			}
			assert(nDefaultPartIndex != -1);

			// Indication de groupe par defaut
			fJSON->WriteKeyInt("defaultGroupIndex", nDefaultPartIndex);
		}
	}

	// Parcours des noeuds fils (noeuds internes uniquement)

	if (!treeNodeSpec->IsLeaf())
	{
		fJSON->BeginKeyArray("childNodes");

		if (treeNodeSpec->GetChildNodes().GetSize())
		{
			for (int iNodeIndex = 0; iNodeIndex < treeNodeSpec->GetChildNodes().GetSize(); iNodeIndex++)
			{
				// Extraction du noeud fils courant
				DTDecisionTreeNodeSpec* sonNode =
				    cast(DTDecisionTreeNodeSpec*, treeNodeSpec->GetChildNodes().GetAt(iNodeIndex));
				// if (sonNode->GetVariableName() == "")
				//	continue;

				WriteJSONSonNodes(fJSON, sonNode, oaLeavesNodes);
			}
		}

		fJSON->EndArray();
	}

	if (treeNodeSpec->GetFatherNode() != NULL)
		fJSON->EndObject();
}

KWAttributeStats* DTCreationReport::GetAttributeStats(const ALString sVariableName,
						      const ObjectArray* oaAttributesStats) const
{
	assert(oaAttributesStats != NULL);
	assert(oaAttributesStats->GetSize() > 0);

	KWAttributeStats* attributeStats = NULL;

	for (int i = 0; i < oaAttributesStats->GetSize(); i++)
	{
		attributeStats = cast(KWAttributeStats*, oaAttributesStats->GetAt(i));
		if (sVariableName == attributeStats->GetAttributeName())
			return attributeStats;
	}

	return NULL;
}

void DTCreationReport::ComputeRankIdentifiers(ObjectArray* oaReports)
{
	const double dEpsilon = 1e-10;
	DTDecisionTreeSpec* treeSpec;
	int nReport;
	int i;
	int nMaxDigitNumber;
	int nDigitNumber;
	ALString sNewIdentifier;

	require(oaReports != NULL);

	// Affichage si tableau non vide
	if (oaReports->GetSize() > 0)
	{
		// Tri par level
		oaReports->SetCompareFunction(DTTreeSpecsCompareLevels);
		oaReports->Sort();

		// Calcul du nombre max de chiffres necessaires
		nMaxDigitNumber = 1 + (int)floor(dEpsilon + log((double)oaReports->GetSize()) / log(10.0));

		// Parcours des rapports
		for (nReport = 0; nReport < oaReports->GetSize(); nReport++)
		{
			treeSpec = cast(DTDecisionTreeSpec*, oaReports->GetAt(nReport));

			// Calcul du nombre de chiffre necessaires
			nDigitNumber = 1 + (int)floor(dEpsilon + log((double)(nReport + 1)) / log(10.0));

			// Fabrication de l'identifiant, en ajoutant des caracteres '0' a gauche si necessaire
			// pour avoir une longueur fixe
			sNewIdentifier = "R";
			for (i = nDigitNumber; i < nMaxDigitNumber; i++)
				sNewIdentifier += '0';
			sNewIdentifier += IntToString(nReport + 1);

			// Memorisation de l'identifiant
			treeSpec->SetRank(sNewIdentifier);
		}
	}
}

int DTTreeSpecsCompareLevels(const void* elem1, const void* elem2)
{
	longint lLevel1;
	longint lLevel2;
	int nCompare;

	DTDecisionTreeSpec* s1 = (DTDecisionTreeSpec*)*(Object**)elem1;
	DTDecisionTreeSpec* s2 = (DTDecisionTreeSpec*)*(Object**)elem2;

	// Comparaison des levels des attributs (ramenes a longint)
	lLevel1 = longint(floor(s1->GetLevel() * 1e10));
	lLevel2 = longint(floor(s2->GetLevel() * 1e10));
	nCompare = -CompareLongint(lLevel1, lLevel2);

	// Comparaison par nom d'arbre, si match nul
	if (nCompare == 0)
		nCompare = s1->GetTreeVariableName().Compare(s2->GetTreeVariableName());

	return nCompare;
}
