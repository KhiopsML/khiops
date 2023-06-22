// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCPostProcessingSpec.h"

CCPostProcessingSpec::CCPostProcessingSpec()
{
	nInstanceNumber = 0;
	nNonEmptyCellNumber = 0;
	nCellNumber = 0;
	nMaxCellNumber = 0;
	nMaxPreservedInformation = 0;
	nTotalPartNumber = 0;
	nMaxTotalPartNumber = 0;

	// ## Custom constructor

	// ##
}

CCPostProcessingSpec::~CCPostProcessingSpec()
{
	// ## Custom destructor

	oaPostProcessedAttributes.DeleteAll();

	// ##
}

void CCPostProcessingSpec::CopyFrom(const CCPostProcessingSpec* aSource)
{
	require(aSource != NULL);

	sShortDescription = aSource->sShortDescription;
	nInstanceNumber = aSource->nInstanceNumber;
	nNonEmptyCellNumber = aSource->nNonEmptyCellNumber;
	nCellNumber = aSource->nCellNumber;
	nMaxCellNumber = aSource->nMaxCellNumber;
	nMaxPreservedInformation = aSource->nMaxPreservedInformation;
	nTotalPartNumber = aSource->nTotalPartNumber;
	nMaxTotalPartNumber = aSource->nMaxTotalPartNumber;
	sFrequencyAttribute = aSource->sFrequencyAttribute;

	// ## Custom copyfrom

	int i;
	CCPostProcessedAttribute* postProcessedAttribute;

	// Copies des attributs post processes
	oaPostProcessedAttributes.DeleteAll();
	oaPostProcessedAttributes.SetSize(aSource->oaPostProcessedAttributes.GetSize());
	for (i = 0; i < oaPostProcessedAttributes.GetSize(); i++)
	{
		postProcessedAttribute = cast(CCPostProcessedAttribute*, aSource->oaPostProcessedAttributes.GetAt(i));
		oaPostProcessedAttributes.SetAt(i, postProcessedAttribute->Clone());
	}

	// ##
}

CCPostProcessingSpec* CCPostProcessingSpec::Clone() const
{
	CCPostProcessingSpec* aClone;

	aClone = new CCPostProcessingSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void CCPostProcessingSpec::Write(ostream& ost) const
{
	ost << "ShortDescription\t" << GetShortDescription() << "\n";
	ost << "Instance number\t" << GetInstanceNumber() << "\n";
	ost << "Non empty cell number\t" << GetNonEmptyCellNumber() << "\n";
	ost << "Cell number\t" << GetCellNumber() << "\n";
	ost << "Max cell number\t" << GetMaxCellNumber() << "\n";
	ost << "Max preserved information\t" << GetMaxPreservedInformation() << "\n";
	ost << "Total part number\t" << GetTotalPartNumber() << "\n";
	ost << "Max total part number\t" << GetMaxTotalPartNumber() << "\n";
	ost << "Frequency variable\t" << GetFrequencyAttribute() << "\n";
}

const ALString CCPostProcessingSpec::GetClassLabel() const
{
	return "Simplification parameters";
}

// ## Method implementation

const ALString CCPostProcessingSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

ObjectArray* CCPostProcessingSpec::GetPostProcessedAttributes()
{
	return &oaPostProcessedAttributes;
}

boolean CCPostProcessingSpec::PostProcessCoclustering(CCHierarchicalDataGrid* postProcessedCoclusteringDataGrid)
{
	boolean bOk = true;
	boolean bDisplay = false;
	const double dEpsilon = 1e-6;
	CCCoclusteringReport coclusteringReport;
	SortedList slSortedMergeableParts(CCHDGPartCompareHierarchicalRank);
	int nAttribute;
	CCHDGAttribute* hdgAttribute;
	KWDGPart* dgPart;
	CCHDGPart* hdgPart;
	CCHDGPart* hdgMergedPart;
	double dActualCellNumber;
	int nActualTotalPartNumber;
	CCPostProcessedAttribute* postProcessedAttribute;
	IntVector ivAttributMaxPartNumbers;
	int nMaxPartViolationNumber;
	boolean bMerge;

	require(postProcessedCoclusteringDataGrid != NULL);
	require(postProcessedCoclusteringDataGrid->GetAttributeNumber() > 0);

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Preparation du post-traitement et des contraintes a verifier

	// Recherche de toutes les parties mergeables et rangement dans un rapport
	for (nAttribute = 0; nAttribute < postProcessedCoclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		hdgAttribute = cast(CCHDGAttribute*, postProcessedCoclusteringDataGrid->GetAttributeAt(nAttribute));

		// Parcours des parties
		dgPart = hdgAttribute->GetHeadPart();
		while (dgPart != NULL)
		{
			hdgPart = cast(CCHDGPart*, dgPart);

			// Evaluation des parties mergeable (forcement ancetre des parties feuilles)
			if (hdgAttribute->IsPartMergeable(hdgPart->GetParentPart()))
			{
				if (slSortedMergeableParts.Find(hdgPart->GetParentPart()) == NULL)
					slSortedMergeableParts.Add(hdgPart->GetParentPart());
			}

			// Partie suivante
			hdgAttribute->GetNextPart(dgPart);
		}
	}

	// Calcul du nombre de cellules et de partie
	dActualCellNumber = 1;
	nActualTotalPartNumber = 0;
	for (nAttribute = 0; nAttribute < postProcessedCoclusteringDataGrid->GetAttributeNumber(); nAttribute++)
	{
		hdgAttribute = cast(CCHDGAttribute*, postProcessedCoclusteringDataGrid->GetAttributeAt(nAttribute));
		dActualCellNumber *= hdgAttribute->GetPartNumber();
		nActualTotalPartNumber += hdgAttribute->GetPartNumber();
	}

	// Initialisation des contraintes en nombre de parties par attributs
	// On part des contraintes specifiees pour retrouver les attributs correspondant de la grille
	ivAttributMaxPartNumbers.SetSize(postProcessedCoclusteringDataGrid->GetAttributeNumber());
	nMaxPartViolationNumber = 0;
	for (nAttribute = 0; nAttribute < oaPostProcessedAttributes.GetSize(); nAttribute++)
	{
		postProcessedAttribute = cast(CCPostProcessedAttribute*, oaPostProcessedAttributes.GetAt(nAttribute));

		// Memorisation de la contrainte si elle est active, et si on retourve l'attribut correspondant
		hdgAttribute =
		    cast(CCHDGAttribute*,
			 postProcessedCoclusteringDataGrid->SearchAttribute(postProcessedAttribute->GetName()));
		if (postProcessedAttribute->GetMaxPartNumber() > 0 and hdgAttribute != NULL)
		{
			ivAttributMaxPartNumbers.SetAt(hdgAttribute->GetAttributeIndex(),
						       postProcessedAttribute->GetMaxPartNumber());
			if (hdgAttribute->GetPartNumber() > postProcessedAttribute->GetMaxPartNumber())
				nMaxPartViolationNumber++;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Execution du post-traitement en verifiant les contraintes

	// Affichage de l'entete
	if (bDisplay)
		cout << "Variable\tPart\tHierarchical rank\tHierarchical level\n";

	// Merges des parties de la grille tant que les contraintes ne sont pas verifiees
	postProcessedCoclusteringDataGrid->SetCellUpdateMode(true);
	while (slSortedMergeableParts.GetCount() > 0)
	{
		// Recherche et supression de la partie en tete de liste
		hdgPart = cast(CCHDGPart*, slSortedMergeableParts.RemoveHead());

		// Verification des contraintes
		bMerge = false;
		if (GetMaxPreservedInformation() > 0 and
		    hdgPart->GetHierarchicalLevel() * 100 > GetMaxPreservedInformation())
			bMerge = true;
		if (GetMaxCellNumber() > 0 and dActualCellNumber > GetMaxCellNumber() + dEpsilon)
			bMerge = true;
		if (GetMaxTotalPartNumber() > 0 and
		    nActualTotalPartNumber >
			max(GetMaxTotalPartNumber(), postProcessedCoclusteringDataGrid->GetAttributeNumber()))
			bMerge = true;
		if (nMaxPartViolationNumber > 0)
			bMerge = true;

		// Arret si toutes les contraintes sont respectees
		if (not bMerge)
			break;
		// Fusion de parties si au moins une contrainte n'est pas respectee
		else
		{
			// Merge
			hdgAttribute = cast(CCHDGAttribute*, hdgPart->GetAttribute());
			hdgMergedPart = hdgAttribute->MergePart(hdgPart);

			// On rajoute eventuellement une nouvelle partie a fusionner
			if (hdgAttribute->IsPartMergeable(hdgMergedPart->GetParentPart()))
			{
				assert(slSortedMergeableParts.Find(hdgMergedPart->GetParentPart()) == NULL);
				slSortedMergeableParts.Add(hdgMergedPart->GetParentPart());
			}

			// Actualisation des parametres de contraintes
			dActualCellNumber /= hdgAttribute->GetPartNumber() + 1;
			dActualCellNumber *= hdgAttribute->GetPartNumber();
			nActualTotalPartNumber--;
			if (ivAttributMaxPartNumbers.GetAt(hdgAttribute->GetAttributeIndex()) > 0 and
			    ivAttributMaxPartNumbers.GetAt(hdgAttribute->GetAttributeIndex()) ==
				hdgAttribute->GetPartNumber())
				nMaxPartViolationNumber--;
			assert(dActualCellNumber > 1 - dEpsilon);
			assert(nActualTotalPartNumber >= oaPostProcessedAttributes.GetSize());
			assert(nMaxPartViolationNumber >= 0);

			// Affichage de la partie fusionnees
			if (bDisplay)
			{
				cout << hdgAttribute->GetAttributeName() << "\t" << hdgMergedPart->GetObjectLabel()
				     << "\t" << hdgMergedPart->GetHierarchicalRank() << "\t"
				     << hdgMergedPart->GetHierarchicalLevel() << "\n";
			}
		}
	}
	postProcessedCoclusteringDataGrid->SetCellUpdateMode(false);
	return bOk;
}

void CCPostProcessingSpec::UpdateCoclusteringSpec(const ALString& sCoclusteringReportFileName)
{
	boolean bOk = true;
	CCCoclusteringReport coclusteringReport;
	CCHierarchicalDataGrid coclusteringDataGrid;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	CCPostProcessedAttribute* postProcessedAttribute;
	boolean bSameCoclustering;
	CCPostProcessingSpec refPostProcessingSpec;
	CCPostProcessedAttribute* refPostProcessedAttribute;

	// Memorisation des specifications en cours
	refPostProcessingSpec.CopyFrom(this);

	// Reinitialisation prealable
	nInstanceNumber = 0;
	nNonEmptyCellNumber = 0;
	nCellNumber = 0;
	nMaxCellNumber = 0;
	nMaxPreservedInformation = 0;
	nTotalPartNumber = 0;
	sFrequencyAttribute = "";
	oaPostProcessedAttributes.DeleteAll();

	// Si pas de fichier, cela revient a reinitialiser les infos de coclustering
	bOk = (sCoclusteringReportFileName != "");

	// Lecture de l'entete du rapport de coclustering
	if (bOk)
		bOk = coclusteringReport.ReadGenericReportHeader(sCoclusteringReportFileName, &coclusteringDataGrid,
								 nInstanceNumber, nNonEmptyCellNumber);

	// On rappatrie les informations du rapport
	if (bOk)
	{
		// Description courte
		sShortDescription = coclusteringDataGrid.GetShortDescription();

		// Variable de frequence
		sFrequencyAttribute = coclusteringDataGrid.GetFrequencyAttributeName();

		// Information sur les attributs de coclustering
		nCellNumber = 1;
		nTotalPartNumber = 0;
		for (nAttribute = 0; nAttribute < coclusteringDataGrid.GetAttributeNumber(); nAttribute++)
		{
			dgAttribute = coclusteringDataGrid.GetAttributeAt(nAttribute);

			// Ajout d'une caracteristique d'attribut de coclustering
			postProcessedAttribute = new CCPostProcessedAttribute;
			postProcessedAttribute->SetName(dgAttribute->GetAttributeName());
			postProcessedAttribute->SetType(KWType::ToString(dgAttribute->GetAttributeType()));
			postProcessedAttribute->SetPartNumber(dgAttribute->GetPartNumber());
			nCellNumber *= dgAttribute->GetPartNumber();
			nTotalPartNumber += dgAttribute->GetPartNumber();
			oaPostProcessedAttributes.Add(postProcessedAttribute);
		}
	}

	// On rappatrie les spec precedentes si coclustering de reference est egal au nouveau coclustering
	if (bOk)
	{
		// Evaluation si le coclustering est le meme
		bSameCoclustering = true;
		bSameCoclustering = bSameCoclustering and (nInstanceNumber == refPostProcessingSpec.nInstanceNumber);
		bSameCoclustering =
		    bSameCoclustering and (nNonEmptyCellNumber == refPostProcessingSpec.nNonEmptyCellNumber);
		bSameCoclustering = bSameCoclustering and (nCellNumber == refPostProcessingSpec.nCellNumber);
		bSameCoclustering =
		    bSameCoclustering and (sFrequencyAttribute == refPostProcessingSpec.sFrequencyAttribute);
		bSameCoclustering = bSameCoclustering and (oaPostProcessedAttributes.GetSize() ==
							   refPostProcessingSpec.oaPostProcessedAttributes.GetSize());
		if (bSameCoclustering)
		{
			for (nAttribute = 0; nAttribute < oaPostProcessedAttributes.GetSize(); nAttribute++)
			{
				postProcessedAttribute =
				    cast(CCPostProcessedAttribute*, oaPostProcessedAttributes.GetAt(nAttribute));
				refPostProcessedAttribute =
				    cast(CCPostProcessedAttribute*,
					 refPostProcessingSpec.oaPostProcessedAttributes.GetAt(nAttribute));

				// Test si spec d'attribut egal par rapport a la reference
				bSameCoclustering = bSameCoclustering and (postProcessedAttribute->GetName() ==
									   refPostProcessedAttribute->GetName());
				bSameCoclustering = bSameCoclustering and (postProcessedAttribute->GetType() ==
									   refPostProcessedAttribute->GetType());
				bSameCoclustering = bSameCoclustering and (postProcessedAttribute->GetPartNumber() ==
									   refPostProcessedAttribute->GetPartNumber());
			}
		}

		// On rappartie les spec de reference en cas de matching du coclustering de reference
		if (bSameCoclustering)
			CopyFrom(&refPostProcessingSpec);
	}
}

void CCPostProcessingSpec::ResetCoclusteringConstraints()
{
	CCPostProcessedAttribute* postProcessedAttribute;
	int nAttribute;

	// Contraintes globales
	nMaxCellNumber = 0;
	nMaxPreservedInformation = 0;

	// Contrainte en nombre de parties par attributs
	for (nAttribute = 0; nAttribute < oaPostProcessedAttributes.GetSize(); nAttribute++)
	{
		postProcessedAttribute = cast(CCPostProcessedAttribute*, oaPostProcessedAttributes.GetAt(nAttribute));
		postProcessedAttribute->SetMaxPartNumber(0);
	}
}

const ALString CCPostProcessingSpec::BuildConstraintSuffix()
{
	ALString sConstraintSuffix;
	CCPostProcessedAttribute* postProcessedAttribute;
	int nAttribute;

	// Nombre de cellules
	if (nMaxCellNumber > 0)
	{
		sConstraintSuffix += "_C";
		sConstraintSuffix += IntToString(nMaxCellNumber);
	}

	// Information preservee
	if (nMaxPreservedInformation > 0)
	{
		sConstraintSuffix += "_I";
		sConstraintSuffix += IntToString(nMaxPreservedInformation);
	}

	// Contrainte par variable de coclustering
	for (nAttribute = 0; nAttribute < oaPostProcessedAttributes.GetSize(); nAttribute++)
	{
		postProcessedAttribute = cast(CCPostProcessedAttribute*, oaPostProcessedAttributes.GetAt(nAttribute));
		if (postProcessedAttribute->GetMaxPartNumber() > 0)
		{
			sConstraintSuffix += "_V";
			sConstraintSuffix += IntToString(nAttribute + 1);
			if (postProcessedAttribute->GetMaxPartNumber() > 0)
			{
				sConstraintSuffix += "p";
				sConstraintSuffix += IntToString(postProcessedAttribute->GetMaxPartNumber());
			}
		}
	}

	return sConstraintSuffix;
}

// ##