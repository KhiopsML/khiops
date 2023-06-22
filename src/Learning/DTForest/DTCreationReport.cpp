// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTCreationReport.h"

int DTTreeSpecsCompareLevels(const void* elem1, const void* elem2);

DTCreationReport::DTCreationReport() {}

DTCreationReport::~DTCreationReport()
{
	Clean();
}

void DTCreationReport::Clean()
{
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

	// Recherche des rapports a ecrire
	for (nIndex = 0; nIndex < GetTreeNumber(); nIndex++)
	{
		sName = GetTreeNameAt(nIndex);
		treeSpec = LookupTree(sName);

		oaSortedReports.Add((Object*)treeSpec);
	}

	// Affichage si tableau non vide
	if (oaSortedReports.GetSize() > 0)
	{
		// Tri par importance
		oaSortedReports.SetCompareFunction(DTTreeSpecsCompareLevels);
		oaSortedReports.Sort();

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
	ContinuousVector cvAttributeMinValues;
	ContinuousVector cvAttributeMaxValues;
	KWDGSAttributePartition* attributePartition;
	KWDGSAttributeDiscretization* numericalAttributePartition;

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
			for (int nValue = 0; nValue < treeNodeSpec->GetTargetModalitiesCountTrain()->GetSize();
			     nValue++)
			{
				DTDecisionTree::TargetModalityCount* tmc =
				    cast(DTDecisionTree::TargetModalityCount*,
					 treeNodeSpec->GetTargetModalitiesCountTrain()->GetAt(nValue));
				fJSON->WriteString(ALString(tmc->sModality));
			}
			fJSON->EndList();

			// Ecriture des effectifs
			fJSON->BeginKeyList("frequencies");
			for (int nValue = 0; nValue < treeNodeSpec->GetTargetModalitiesCountTrain()->GetSize();
			     nValue++)
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
				if (sonNode->GetVariableName() == "")
					continue;

				WriteJSONSonNodes(fJSON, sonNode, oaLeavesNodes);
			}
		}

		// chercher les noeuds feuilles dont ce noeud est le pere
		for (int i = 0; i < oaLeavesNodes.GetSize(); i++)
		{
			DTDecisionTreeNodeSpec* sonNode = cast(DTDecisionTreeNodeSpec*, oaLeavesNodes.GetAt(i));

			if (sonNode->GetFatherNode() == treeNodeSpec)
				WriteJSONSonNodes(fJSON, sonNode, oaLeavesNodes);
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

void DTCreationReport::ComputeRankIdentifiers()
{
	const double dEpsilon = 1e-10;
	ObjectArray oaSortedReports;
	DTDecisionTreeSpec* treeSpec;
	int nReport;
	int i;
	int nMaxDigitNumber;
	int nDigitNumber;
	ALString sNewIdentifier;

	// require(oaDecisionTreesSpecs != NULL);

	// Affichage si tableau non vide
	if (odCreatedTrees.GetCount() > 0)
	{
		// Tri par importance
		odCreatedTrees.ExportObjectArray(&oaSortedReports);
		oaSortedReports.SetCompareFunction(DTTreeSpecsCompareLevels);
		oaSortedReports.Sort();

		// Calcul du nombre max de chiffres necessaires
		nMaxDigitNumber = 1 + (int)floor(dEpsilon + log((double)odCreatedTrees.GetCount()) / log(10.0));

		// Parcours des rapports
		for (nReport = 0; nReport < oaSortedReports.GetSize(); nReport++)
		{
			treeSpec = cast(DTDecisionTreeSpec*, oaSortedReports.GetAt(nReport));

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
	DTDecisionTreeSpec* s1 = (DTDecisionTreeSpec*)*(Object**)elem1;
	DTDecisionTreeSpec* s2 = (DTDecisionTreeSpec*)*(Object**)elem2;

	if (s1->GetLevel() == s2->GetLevel())
		return 0;
	else if (s1->GetLevel() > s2->GetLevel())
		return -1;
	else
		return 1;
}
