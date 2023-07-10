// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDFeatureConstruction.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KDFeatureConstruction

KDFeatureConstruction::KDFeatureConstruction()
{
	constructionDomain = NULL;
}

KDFeatureConstruction::~KDFeatureConstruction() {}

void KDFeatureConstruction::SetConstructionDomain(KDConstructionDomain* constructionDomainParam)
{
	constructionDomain = constructionDomainParam;
}

KDConstructionDomain* KDFeatureConstruction::GetConstructionDomain() const
{
	require(constructionDomain != NULL);
	return constructionDomain;
}

void KDFeatureConstruction::ComputeInitialAttributeCosts(KWClass* kwcValue) const
{
	double dInitialAttributeCost;
	KWAttribute* attribute;

	require(kwcValue != NULL);
	require(GetTargetAttributeName() == "" or kwcValue->LookupAttribute(GetTargetAttributeName()) != NULL);
	require(Check());
	require(constructionDomain != NULL);

	// Calcul du cout de selection d'un attribut natif
	dInitialAttributeCost = GetLearningSpec()->GetSelectionCost();

	// Nettoyage des meta-data des attributs de la classe
	kwcValue->RemoveAllAttributesMetaDataKey(KWAttribute::GetCostMetaDataKey());

	// Memorisation du cout des attributs construits
	attribute = kwcValue->GetHeadAttribute();
	while (attribute != NULL)
	{
		attribute->SetCost(0);

		// Test si attribut initial
		if (attribute->GetUsed() and KWType::IsSimple(attribute->GetType()) and
		    attribute->GetName() != GetTargetAttributeName())
		{
			// Meta-data sur le cout de construction
			if (GetConstructionDomain()->GetConstructionRegularization())
			{
				attribute->SetCost(dInitialAttributeCost);
				attribute->SetMetaDataCost(attribute->GetCost());
			}
		}

		// Attribut suivant
		kwcValue->GetNextAttribute(attribute);
	}
	ensure(AreAttributeCostsInitialized(kwcValue));
}

boolean KDFeatureConstruction::ImportAttributeMetaDataCosts(KWClass* kwcValue)
{
	boolean bOk = true;
	const double dNullConstructionCost = log(2.0);
	double dAttributeCost;
	double dAttributeProb;
	double dAllAttributeProb;
	KWAttribute* attribute;

	require(kwcValue != NULL);
	require(Check());
	require(constructionDomain != NULL);

	// Verification des couts des attributs stockes dans les meta-data
	dAllAttributeProb = 0;
	attribute = kwcValue->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Test si attribut simple utilise
		if (attribute->GetUsed() and KWType::IsSimple(attribute->GetType()) and
		    attribute->GetName() != GetTargetAttributeName())
		{
			// On recupere le cout
			dAttributeCost = attribute->GetMetaDataCost();

			// Test de validite du cout
			if (dAttributeCost < 0)
			{
				AddError(KWAttribute::GetCostMetaDataKey() + " meta-data (" +
					 DoubleToString(dAttributeCost) + ") for variable " + attribute->GetName() +
					 " should be positive");
				bOk = false;
				break;
			}
			else if (not attribute->GetMetaData()->IsKeyPresent(KWAttribute::GetCostMetaDataKey()))
			{
				AddError("Missing " + KWAttribute::GetCostMetaDataKey() + " meta-data for variable " +
					 attribute->GetName());
				bOk = false;
				break;
			}

			// Calcul de la probabilite de l'attribut
			// Le cout du model NULL est ici pris en dur (le ComputeTargetStats n'a pas encore ete appele)
			dAttributeProb = exp(-(dNullConstructionCost + dAttributeCost));
			dAllAttributeProb += dAttributeProb;

			// Arret si probabilite totale des attributs superieure a 1
			if (dAllAttributeProb > 1 + 1e-6)
			{
				AddError("Sum of probabilities of all variables (originating from " +
					 KWAttribute::GetCostMetaDataKey() + ") is above 1");
				bOk = false;
				break;
			}
		}

		// Attribut suivant
		kwcValue->GetNextAttribute(attribute);
	}

	// Message recapitulatif
	if (not bOk)
		AddError("Failed to import " + KWAttribute::GetCostMetaDataKey() + " meta-data from dictionary " +
			 kwcValue->GetName());
	else
		AddMessage("Import " + KWAttribute::GetCostMetaDataKey() + " meta-data from dictionary " +
			   kwcValue->GetName());

	// Si Ok, import des couts des attributs stockes dans les meta-data
	if (bOk)
	{
		attribute = kwcValue->GetHeadAttribute();
		while (attribute != NULL)
		{
			attribute->SetCost(0);

			// Test si attribut simple utilise
			if (attribute->GetUsed() and KWType::IsSimple(attribute->GetType()) and
			    attribute->GetName() != GetTargetAttributeName())
			{
				// On recupere le cout
				dAttributeCost = attribute->GetMetaDataCost();

				// Mise a jour du  le cout de construction
				if (GetConstructionDomain()->GetConstructionRegularization())
					attribute->SetCost(dAttributeCost);
			}

			// Attribut suivant
			kwcValue->GetNextAttribute(attribute);
		}
	}
	ensure(not bOk or AreAttributeCostsInitialized(kwcValue));
	return bOk;
}

boolean KDFeatureConstruction::AreAttributeCostsInitialized(KWClass* kwcValue) const
{
	const double dNullConstructionCost = log(2.0);
	double dAttributeCost;
	double dAttributeProb;
	double dAllAttributeProb;
	KWAttribute* attribute;

	// Verification que tous les attributs simples utilises ont un cout correctement initialise
	dAllAttributeProb = 0;
	attribute = kwcValue->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Test si attribut simple utilise
		if (attribute->GetUsed() and KWType::IsSimple(attribute->GetType()) and
		    attribute->GetName() != GetTargetAttributeName())
		{
			// On recupere le cout depuis les meta-donnes
			dAttributeCost = attribute->GetMetaDataCost();

			// Test de validite du cout
			if (dAttributeCost < 0)
			{
				AddError(KWAttribute::GetCostMetaDataKey() + " meta-data (" +
					 DoubleToString(dAttributeCost) + ") for variable " + attribute->GetName() +
					 " should be positive");
				return false;
			}
			else if (not attribute->GetMetaData()->IsKeyPresent(KWAttribute::GetCostMetaDataKey()))
			{
				AddError("Missing " + KWAttribute::GetCostMetaDataKey() + " meta-data for variable " +
					 attribute->GetName());
				return false;
			}

			// Calcul de la probabilite de l'attribut
			// Le cout du model NULL est ici pris en dur (le ComputeTargetStats n'a pas encore ete appele)
			dAttributeProb = exp(-(dNullConstructionCost + dAttributeCost));
			dAllAttributeProb += dAttributeProb;

			// Arret si probabilite totale des attributs superieure a 1
			if (dAllAttributeProb > 1 + 1e-6)
			{
				AddError("Sum of probabilities of all variables (originating from " +
					 KWAttribute::GetCostMetaDataKey() + ") is above 1");
				return false;
			}

			// Verification
			if (GetConstructionDomain()->GetConstructionRegularization())
			{
				if (attribute->GetCost() != dAttributeCost)
				{
					AddError("Cost of variable " + attribute->GetName() +
						 " inconsistent with its " + KWAttribute::GetCostMetaDataKey() +
						 " meta-data");
					return false;
				}
			}
		}

		// Attribut suivant
		kwcValue->GetNextAttribute(attribute);
	}
	return true;
}

void KDFeatureConstruction::CollectConstructedAttributes(const KWClass* kwcInitialClass,
							 const KWClass* kwcConstructedClass,
							 ObjectDictionary* odConstructedAttributes) const
{
	KWAttribute* initialAttribute;
	KWAttribute* constructedAttribute;
	int nFoundInitialAttributeNumber;

	require(kwcInitialClass != NULL);
	require(kwcConstructedClass != NULL);
	require(kwcInitialClass->GetName() == kwcConstructedClass->GetName());
	require(kwcInitialClass->GetAttributeNumber() <= kwcConstructedClass->GetAttributeNumber());
	require(odConstructedAttributes != NULL);
	require(odConstructedAttributes->GetCount() == 0);

	// Parcours des attributs de la classe construite pour identifier ceux qui n'existaient pas dans la classe
	// initiales
	nFoundInitialAttributeNumber = 0;
	constructedAttribute = kwcConstructedClass->GetHeadAttribute();
	while (constructedAttribute != NULL)
	{
		// Recherche de l'attribut initial correspondant
		initialAttribute = kwcInitialClass->LookupAttribute(constructedAttribute->GetName());

		// Verification si on l'a trouve
		if (initialAttribute != NULL)
		{
			nFoundInitialAttributeNumber++;
			assert(initialAttribute->GetType() == constructedAttribute->GetType());
			assert(initialAttribute->GetUsed() == constructedAttribute->GetUsed());
		}
		// Memorisation sinon si l'attribut est utilise
		else if (constructedAttribute->GetUsed())
		{
			assert(KWType::IsSimple(constructedAttribute->GetType()));
			odConstructedAttributes->SetAt(constructedAttribute->GetName(), constructedAttribute);
		}

		// Attribut suivant
		kwcConstructedClass->GetNextAttribute(constructedAttribute);
	}
	assert(nFoundInitialAttributeNumber == kwcInitialClass->GetAttributeNumber());
	ensure(odConstructedAttributes->GetCount() + kwcInitialClass->GetUsedAttributeNumberForType(KWType::Symbol) +
		   kwcInitialClass->GetUsedAttributeNumberForType(KWType::Continuous) ==
	       kwcConstructedClass->GetUsedAttributeNumberForType(KWType::Symbol) +
		   kwcConstructedClass->GetUsedAttributeNumberForType(KWType::Continuous));
}

const ALString KDFeatureConstruction::GetClassLabel() const
{
	return "Feature construction";
}

const ALString KDFeatureConstruction::GetObjectLabel() const
{
	return GetClass()->GetName();
}
