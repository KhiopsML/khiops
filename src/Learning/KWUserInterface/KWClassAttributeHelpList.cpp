// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClassAttributeHelpList.h"

KWClassAttributeHelpList::KWClassAttributeHelpList()
{
	lHelpListLastClassHashValue = 0;
	lHelpListLastClassDomainHashValue = 0;
}

KWClassAttributeHelpList::~KWClassAttributeHelpList() {}

void KWClassAttributeHelpList::FillAttributeNames(const ALString& sClassName, boolean bContinuousAttributes,
						  boolean bSymbolAttributes, boolean bNativeAttributesOnly,
						  boolean bUsedAttributesOnly, UIList* uilAttributeNameHelpList,
						  const ALString& sListAttributeFieldName)
{
	KWClass* kwcClass;
	const int nMaxAttributeNumber = 1001;
	int nAttributeNumber;
	KWAttribute* attribute;

	require(uilAttributeNameHelpList != NULL);
	require(uilAttributeNameHelpList->GetFieldIndex(sListAttributeFieldName) != -1);

	// Arret s'il n'est pas necessaire de rafraichir la liste d'aide
	if (not NeedsRefresh(sClassName))
		return;

	// On commence par la vider
	uilAttributeNameHelpList->RemoveAllItems();

	// On alimente les attributs symboliques si la classe est valide
	kwcClass = NULL;
	if (sClassName != "")
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);
	if (kwcClass != NULL)
	{
		// Ajout d'une ligne vide au debut
		nAttributeNumber = 0;
		uilAttributeNameHelpList->AddItem();
		uilAttributeNameHelpList->SetStringValueAt(sListAttributeFieldName, "");
		nAttributeNumber++;

		// Ajout des attributs selon leur type
		attribute = kwcClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Ajout si attribut utilise (si necessaire) et du bon type
			if ((attribute->GetUsed() or not bUsedAttributesOnly) and
			    ((bContinuousAttributes and attribute->GetType() == KWType::Continuous) or
			     (bSymbolAttributes and attribute->GetType() == KWType::Symbol)))
			{
				if (not bNativeAttributesOnly or attribute->GetAnyDerivationRule() == NULL)
				{
					uilAttributeNameHelpList->AddItem();
					uilAttributeNameHelpList->SetStringValueAt(sListAttributeFieldName,
										   attribute->GetName());

					// Incrementation
					nAttributeNumber++;
					if (nAttributeNumber >= nMaxAttributeNumber)
						break;
				}
			}

			// Passage a l'attribut suivant
			kwcClass->GetNextAttribute(attribute);
		}

		// Si le nombre max est atteint, on remplace la deuxieme moitie des attributs
		// par les attributs de la fin de la classe
		if (uilAttributeNameHelpList->GetItemNumber() >= nMaxAttributeNumber)
		{
			nAttributeNumber = uilAttributeNameHelpList->GetItemNumber();
			attribute = kwcClass->GetTailAttribute();
			while (attribute != NULL)
			{
				// Ajout si attribut utilise (si necessaire) et du bon type
				if ((attribute->GetUsed() or not bUsedAttributesOnly) and
				    ((bContinuousAttributes and attribute->GetType() == KWType::Continuous) or
				     (bSymbolAttributes and attribute->GetType() == KWType::Symbol)))
				{
					if (not bNativeAttributesOnly or attribute->GetAnyDerivationRule() == NULL)
					{
						uilAttributeNameHelpList->SetCurrentItemIndex(nAttributeNumber - 1);
						uilAttributeNameHelpList->SetStringValueAt(sListAttributeFieldName,
											   attribute->GetName());

						// Arret quand on a atteint la moitie
						nAttributeNumber--;
						if (nAttributeNumber <= nMaxAttributeNumber / 2)
						{
							// On marque l'attribut en blanc pour indiquer qu'il y en a
							// d'autres
							uilAttributeNameHelpList->SetStringValueAt(
							    sListAttributeFieldName, "");
							break;
						}
					}
				}

				// Passage a l'attribut precedent
				kwcClass->GetPrevAttribute(attribute);
			}
		}
	}

	// Memorisation de la fraicheur de la liste d'aide
	UpdateFreshness(sClassName);
}

boolean KWClassAttributeHelpList::NeedsRefresh(const ALString& sClassName) const
{
	boolean bNeedsRefresh;
	KWClass* kwcClass;
	longint lClassDomainHashValue;
	longint lClassHashValue;

	// En mode batch, les liste d'aide sont inutiles
	if (UIObject::IsBatchMode())
		return false;

	// Comparison du nom de la classe
	bNeedsRefresh = false;
	if (sHelpListLastClassName != sClassName)
		bNeedsRefresh = true;

	// Comparaison de la valeur de hash de la classe
	if (not bNeedsRefresh)
	{
		// Calcul de la valeur de hashage de la classe
		lClassHashValue = 0;
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);
		if (kwcClass != NULL)
			lClassHashValue = kwcClass->ComputeHashValue();

		// Recalcul necessaire si la fraicheur a change
		if (lClassHashValue != lHelpListLastClassHashValue)
			bNeedsRefresh = true;
	}

	// Comparaison de la valeur de hash du domaine
	if (not bNeedsRefresh)
	{
		lClassDomainHashValue = KWClassDomain::GetCurrentDomain()->ComputeHashValue();
		if (lClassDomainHashValue != lHelpListLastClassDomainHashValue)
			bNeedsRefresh = true;
	}
	return bNeedsRefresh;
}

void KWClassAttributeHelpList::UpdateFreshness(const ALString& sClassName)
{
	KWClass* kwcClass;
	longint lClassDomainHashValue;
	longint lClassHashValue;

	// Memorisation du nom de la classe
	sHelpListLastClassName = sClassName;

	// Memorisation de la valeur de hashage de la classe
	lClassHashValue = 0;
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);
	if (kwcClass != NULL)
		lClassHashValue = kwcClass->ComputeHashValue();
	lHelpListLastClassHashValue = lClassHashValue;

	// Memorisation de la valeur de hashage du domaine
	lClassDomainHashValue = KWClassDomain::GetCurrentDomain()->ComputeHashValue();
	lHelpListLastClassDomainHashValue = lClassDomainHashValue;
	ensure(not NeedsRefresh(sClassName));
}
