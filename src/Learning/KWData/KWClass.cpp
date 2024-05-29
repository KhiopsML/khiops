// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClass.h"

// On a besoin de ce header pour parametrer les regle Random
// Mais on ne peut pas l'inclure dans le .h pour cause de references cycliques
#include "KWDRRandom.h"

//////////////////////////////
// KWClass

KWClass::KWClass()
{
	bRoot = false;
	lClassHashValue = 0;
	nHashFreshness = 0;
	nFreshness = 0;
	nIndexFreshness = 0;
	nCompileFreshness = 0;
	domain = NULL;
	ivUsedAttributeNumbers.SetSize(KWType::None);
	ivUsedDenseAttributeNumbers.SetSize(KWType::None);
	ivUsedSparseAttributeNumbers.SetSize(KWType::None);
	nNativeAttributeBlockNumber = 0;
	nNativeAttributeNumber = 0;
}

KWClass::~KWClass()
{
	// Destruction des blocs d'attributs
	DeleteAllAttributeBlocks();

	// Destruction des attributs
	olAttributes.DeleteAll();
}

void KWClass::SetKeyAttributeNumber(int nValue)
{
	require(nValue >= 0);
	svKeyAttributeNames.SetSize(nValue);
	nFreshness++;
}

void KWClass::SetKeyAttributeNameAt(int nIndex, const ALString& sValue)
{
	require(0 <= nIndex and nIndex < GetKeyAttributeNumber());
	svKeyAttributeNames.SetAt(nIndex, sValue);
	nFreshness++;
}

void KWClass::InsertAttribute(KWAttribute* attribute)
{
	require(attribute != NULL);
	require(CheckName(attribute->GetName(), KWClass::Attribute, attribute));
	require(attribute->parentClass == NULL);
	require(LookupAttribute(attribute->GetName()) == NULL);
	require(LookupAttributeBlock(attribute->GetName()) == NULL);

	// Ajout dans le dictionnaire des attributs
	odAttributes.SetAt(attribute->GetName(), attribute);
	attribute->parentClass = this;

	// Ajout dans la liste
	attribute->listPosition = olAttributes.AddTail(attribute);

	// Fraicheur
	nFreshness++;

	assert(odAttributes.GetCount() == olAttributes.GetCount());
}

void KWClass::InsertAttributeBefore(KWAttribute* attribute, KWAttribute* attributeRef)
{
	require(attributeRef != NULL);
	require(attributeRef == cast(KWAttribute*, odAttributes.Lookup(attributeRef->GetName())));
	require(attributeRef->parentClass == this);
	require(not attributeRef->IsInBlock() or
		attributeRef == attributeRef->GetAttributeBlock()->GetFirstAttribute());
	require(attribute != NULL);
	require(CheckName(attribute->GetName(), KWClass::Attribute, this));
	require(attribute->parentClass == NULL);
	require(LookupAttribute(attribute->GetName()) == NULL);
	require(LookupAttributeBlock(attribute->GetName()) == NULL);

	// Ajout dans le dictionnaire
	odAttributes.SetAt(attribute->GetName(), attribute);
	attribute->parentClass = this;

	// Ajout dans la liste
	attribute->listPosition = olAttributes.InsertBefore(attributeRef->listPosition, attribute);

	// Fraicheur
	nFreshness++;

	assert(odAttributes.GetCount() == olAttributes.GetCount());
}

void KWClass::InsertAttributeAfter(KWAttribute* attribute, KWAttribute* attributeRef)
{
	require(attributeRef != NULL);
	require(attributeRef == cast(KWAttribute*, odAttributes.Lookup(attributeRef->GetName())));
	require(attributeRef->parentClass == this);
	require(not attributeRef->IsInBlock() or attributeRef == attributeRef->GetAttributeBlock()->GetLastAttribute());
	require(attribute != NULL);
	require(CheckName(attribute->GetName(), KWClass::Attribute, attribute));
	require(attribute->parentClass == NULL);
	require(LookupAttribute(attribute->GetName()) == NULL);
	require(LookupAttributeBlock(attribute->GetName()) == NULL);

	// Ajout dans le dictionnaire
	odAttributes.SetAt(attribute->GetName(), attribute);
	attribute->parentClass = this;

	// Ajout dans la liste
	attribute->listPosition = olAttributes.InsertAfter(attributeRef->listPosition, attribute);

	// Fraicheur
	nFreshness++;

	assert(odAttributes.GetCount() == olAttributes.GetCount());
}

void KWClass::RenameAttribute(KWAttribute* refAttribute, const ALString& sNewName)
{
	KWAttribute* attribute;
	KWDerivationRule* currentDerivationRule;

	require(refAttribute != NULL);
	require(refAttribute == cast(KWAttribute*, odAttributes.Lookup(refAttribute->GetName())));
	require(refAttribute->parentClass == this);
	require(LookupAttribute(sNewName) == NULL);
	require(CheckName(sNewName, KWClass::Attribute, refAttribute));

	// Renommage par manipulation dans le dictionnaire
	// Propagation du renommage a toutes les regles de derivation
	// des classes du domaine referencant l'attribut
	attribute = GetHeadAttribute();
	currentDerivationRule = NULL;
	while (attribute != NULL)
	{
		// Detection de changement de regle de derivation (notamment pour les blocs)
		if (attribute->GetAnyDerivationRule() != currentDerivationRule)
		{
			currentDerivationRule = attribute->GetAnyDerivationRule();

			// Renommage dans les regles de derivation (et au plus une seule fois par bloc)
			if (currentDerivationRule != NULL)
				currentDerivationRule->RenameAttribute(this, refAttribute, sNewName);
		}

		// Attribut suivant
		GetNextAttribute(attribute);
	}

	// Renommage de l'attribut dans la classe
	odAttributes.RemoveKey(refAttribute->GetName());
	refAttribute->usName.SetValue(sNewName);
	odAttributes.SetAt(refAttribute->GetName(), refAttribute);
	assert(odAttributes.GetCount() == olAttributes.GetCount());
	nFreshness++;
}

void KWClass::UnsafeRenameAttribute(KWAttribute* refAttribute, const ALString& sNewName)
{
	require(refAttribute != NULL);
	require(refAttribute == cast(KWAttribute*, odAttributes.Lookup(refAttribute->GetName())));
	require(refAttribute->parentClass == this);
	require(CheckName(sNewName, KWClass::Attribute, refAttribute));
	require(odAttributes.Lookup(sNewName) == NULL);
	require(odAttributeBlocks.Lookup(sNewName) == NULL);

	// Renommage de l'attribut dans la classe
	odAttributes.RemoveKey(refAttribute->GetName());
	refAttribute->usName.SetValue(sNewName);
	odAttributes.SetAt(refAttribute->GetName(), refAttribute);
	assert(odAttributes.GetCount() == olAttributes.GetCount());
	nFreshness++;
}

void KWClass::SetAllAttributesUsed(boolean bValue)
{
	KWAttribute* attribute;

	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		attribute->SetUsed(bValue);
		GetNextAttribute(attribute);
	}
}

void KWClass::SetAllAttributesLoaded(boolean bValue)
{
	KWAttribute* attribute;

	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		attribute->SetLoaded(bValue);
		GetNextAttribute(attribute);
	}
}

void KWClass::RemoveAllAttributesMetaDataKey(const ALString& sKey)
{
	KWAttribute* attribute;

	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		attribute->GetMetaData()->RemoveKey(sKey);
		GetNextAttribute(attribute);
	}
}

boolean KWClass::RemoveAttribute(const ALString& sAttributeName)
{
	KWAttribute* attribute;

	// Recherche de l'attribut
	attribute = cast(KWAttribute*, odAttributes.Lookup(sAttributeName));

	// Supression si trouve
	if (attribute != NULL)
	{
		// Mise a jour et destruction si neccessaire du bloc eventuel contenant l'attribut
		if (attribute->GetAttributeBlock() != NULL)
		{
			// Cas d'un bloc mono-attribut
			if (attribute->GetAttributeBlock()->GetFirstAttribute() ==
			    attribute->GetAttributeBlock()->GetLastAttribute())
			{
				assert(attribute == attribute->GetAttributeBlock()->GetFirstAttribute());
				odAttributeBlocks.RemoveKey(attribute->GetAttributeBlock()->GetName());
				delete attribute->GetAttributeBlock();
			}
			// Cas ou l'attribut est le premier du bloc
			else if (attribute == attribute->GetAttributeBlock()->GetFirstAttribute())
				GetNextAttribute(attribute->GetAttributeBlock()->firstAttribute);
			// Cas ou l'attribut est le dernier du bloc
			else if (attribute == attribute->GetAttributeBlock()->GetLastAttribute())
				GetPrevAttribute(attribute->GetAttributeBlock()->lastAttribute);
		}

		// Supression des containers
		odAttributes.RemoveKey(attribute->GetName());
		olAttributes.RemoveAt(attribute->listPosition);

		// Nettoyage de l'attribut
		attribute->attributeBlock = NULL;
		attribute->parentClass = NULL;
		attribute->listPosition = NULL;

		// Fraicheur
		nFreshness++;
		assert(odAttributes.GetCount() == olAttributes.GetCount());
		return true;
	}
	return false;
}

boolean KWClass::DeleteAttribute(const ALString& sAttributeName)
{
	KWAttribute* attribute;

	// Recherche de l'attribut
	attribute = cast(KWAttribute*, odAttributes.Lookup(sAttributeName));

	// Destruction si trouve
	if (attribute != NULL)
	{
		// Destruction si neccessaire du bloc eventuel contenant l'attribut
		if (attribute->GetAttributeBlock() != NULL)
		{
			// Cas d'un bloc mono-attribut
			if (attribute->GetAttributeBlock()->GetFirstAttribute() ==
			    attribute->GetAttributeBlock()->GetLastAttribute())
			{
				assert(attribute == attribute->GetAttributeBlock()->GetFirstAttribute());
				odAttributeBlocks.RemoveKey(attribute->GetAttributeBlock()->GetName());
				delete attribute->GetAttributeBlock();
			}
			// Cas ou l'attribut est le premier du bloc
			else if (attribute == attribute->GetAttributeBlock()->GetFirstAttribute())
				GetNextAttribute(attribute->GetAttributeBlock()->firstAttribute);
			// Cas ou l'attribut est le dernier du bloc
			else if (attribute == attribute->GetAttributeBlock()->GetLastAttribute())
				GetPrevAttribute(attribute->GetAttributeBlock()->lastAttribute);
		}

		// Supression des containers
		odAttributes.RemoveKey(attribute->GetName());
		olAttributes.RemoveAt(attribute->listPosition);

		// Destruction de l'attribut
		delete attribute;

		// Fraicheur
		nFreshness++;
		assert(odAttributes.GetCount() == olAttributes.GetCount());
		return true;
	}
	return false;
}

void KWClass::DeleteAllAttributes()
{
	// Destruction des blocs d'attributs
	DeleteAllAttributeBlocks();

	// Destruction des attributs
	SetKeyAttributeNumber(0);
	odAttributes.DeleteAll();
	olAttributes.RemoveAll();
	nFreshness++;
}

KWAttribute* KWClass::GetHeadAttribute() const
{
	POSITION position;

	position = olAttributes.GetHeadPosition();
	if (position == NULL)
		return NULL;
	else
		return cast(KWAttribute*, olAttributes.GetAt(position));
}

KWAttribute* KWClass::GetTailAttribute() const
{
	POSITION position;

	position = olAttributes.GetTailPosition();
	if (position == NULL)
		return NULL;
	else
		return cast(KWAttribute*, olAttributes.GetAt(position));
}

void KWClass::GetNextAttribute(KWAttribute*& attribute) const
{
	POSITION position;

	require(attribute != NULL);
	require(attribute == cast(KWAttribute*, odAttributes.Lookup(attribute->GetName())));
	require(attribute->parentClass == this);

	position = attribute->listPosition;
	olAttributes.GetNext(position);
	if (position == NULL)
		attribute = NULL;
	else
		attribute = cast(KWAttribute*, olAttributes.GetAt(position));
}

void KWClass::GetPrevAttribute(KWAttribute*& attribute) const
{
	POSITION position;

	require(attribute != NULL);
	require(attribute == cast(KWAttribute*, odAttributes.Lookup(attribute->GetName())));
	require(attribute->parentClass == this);

	position = attribute->listPosition;
	olAttributes.GetPrev(position);
	if (position == NULL)
		attribute = NULL;
	else
		attribute = cast(KWAttribute*, olAttributes.GetAt(position));
}

void KWClass::IndexClass()
{
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	int nIndex;
	int nInternalLoadIndex;
	int nAttributeBlock;
	int nAttribute;
	ALString sAttributeKeyMetaDataKey;
	Symbol sVarKey;
	int nVarKey;

	// Arret si deja indexe
	if (nFreshness == nIndexFreshness)
		return;

	// Mise a jour de la fraicheur d'indexation
	nIndexFreshness = nFreshness;

	// Reinitialisation des tableaux d'attributs
	oaUsedAttributes.SetSize(0);
	oaLoadedAttributes.SetSize(0);
	oaLoadedDenseAttributes.SetSize(0);
	oaLoadedAttributeBlocks.SetSize(0);
	oaLoadedDenseSymbolAttributes.SetSize(0);
	oaLoadedTextAttributes.SetSize(0);
	oaLoadedTextListAttributes.SetSize(0);
	oaLoadedRelationAttributes.SetSize(0);
	oaUnloadedNativeRelationAttributes.SetSize(0);
	oaLoadedDataItems.SetSize(0);
	livKeyAttributeLoadIndexes.SetSize(0);

	// Reinitialisation des statistiques par type d'attributs utilises
	ivUsedAttributeNumbers.Initialize();
	ivUsedDenseAttributeNumbers.Initialize();
	ivUsedSparseAttributeNumbers.Initialize();

	// Indexage des tableaux d'attributs par parcours de la liste
	sAttributeKeyMetaDataKey = KWAttributeBlock::GetAttributeKeyMetaDataKey();
	nNativeAttributeBlockNumber = 0;
	nNativeAttributeNumber = 0;
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		assert(attribute->GetType() != KWType::Unknown);

		// Calcul du nombre d'attributs natifs
		if (attribute->IsInBlock())
		{
			if (attribute->GetAttributeBlock()->IsNative())
			{
				if (attribute->IsFirstInBlock())
					nNativeAttributeBlockNumber++;
			}
		}
		else
		{
			if (attribute->IsNative())
				nNativeAttributeNumber++;
		}

		// Reinitialisation de chaque bloc quand on les rencontre pour la premiere fois
		if (attribute->IsFirstInBlock())
		{
			attributeBlock = attribute->GetAttributeBlock();
			// Le type de VarKey  doit avoir ete initialise
			assert(KWType::IsSimple(attributeBlock->GetVarKeyType()));
			attributeBlock->bUsed = false;
			attributeBlock->bLoaded = false;
			attributeBlock->liLoadIndex.Reset();
			attributeBlock->oaLoadedAttributes.RemoveAll();
			attributeBlock->loadedAttributesIndexedKeyBlock->Clean();
			attributeBlock->nAttributeNumber = 0;
			attributeBlock->nkdAttributesByVarKeys.RemoveAll();
		}

		// Incrementation des nombres d'attributs par bloc
		if (attribute->IsInBlock())
			attribute->GetAttributeBlock()->nAttributeNumber++;

		// Enregistrement des attributs de bloc dans un dictionnaire du bloc a acces par VarKey
		if (attribute->IsInBlock())
		{
			attributeBlock = attribute->GetAttributeBlock();
			if (attributeBlock->GetVarKeyType() == KWType::Symbol)
			{
				sVarKey = attributeBlock->GetSymbolVarKey(attribute);
				assert(attributeBlock->nkdAttributesByVarKeys.Lookup(sVarKey.GetNumericKey()) == NULL);
				attributeBlock->nkdAttributesByVarKeys.SetAt(sVarKey.GetNumericKey(), attribute);
			}
			else
			{
				nVarKey = attributeBlock->GetContinuousVarKey(attribute);
				assert(attributeBlock->nkdAttributesByVarKeys.Lookup(nVarKey) == NULL);
				attributeBlock->nkdAttributesByVarKeys.SetAt(nVarKey, attribute);
			}
		}

		// Insertion dans les tableaux en fonction des caracteristiques
		// de l'attribut
		attribute->liLoadIndex.Reset();
		if (attribute->GetUsed())
		{
			oaUsedAttributes.Add(attribute);

			// Calcul des statistiques par type d'attribut utilise
			ivUsedAttributeNumbers.UpgradeAt(attribute->GetType(), 1);
			if (attribute->IsInBlock())
				ivUsedSparseAttributeNumbers.UpgradeAt(attribute->GetType(), 1);
			else
				ivUsedDenseAttributeNumbers.UpgradeAt(attribute->GetType(), 1);

			// Mise ajour de flag Used des blocs
			if (attribute->IsInBlock())
			{
				attributeBlock = attribute->GetAttributeBlock();

				// On ne prend en compte le bloc que la premiere fois, pour ne le compter qu'une seule
				// fois
				if (not attributeBlock->bUsed)
				{
					attributeBlock->bUsed = true;

					// Calcul des statistiques par type de bloc d'attribut utilise
					ivUsedAttributeNumbers.UpgradeAt(attributeBlock->GetBlockType(), 1);
				}
			}

			// Gestion des attributs charges en memoire
			if (attribute->GetLoaded())
			{
				// Memorisation en tant qu'attribut charge en memoire
				oaLoadedAttributes.Add(attribute);

				// Memorisation en tant qu'attribut dense charge en memoire
				if (not attribute->IsInBlock())
				{
					oaLoadedDenseAttributes.Add(attribute);
					oaLoadedDataItems.Add(attribute);
				}

				// Memorisation en tant que block, pour chaque premier attribut de bloc
				if (attribute->IsInBlock() and
				    (oaLoadedAttributeBlocks.GetSize() == 0 or
				     oaLoadedAttributeBlocks.GetAt(oaLoadedAttributeBlocks.GetSize() - 1) !=
					 attribute->GetAttributeBlock()))
				{
					check(attribute->GetAttributeBlock());
					oaLoadedAttributeBlocks.Add(attribute->GetAttributeBlock());
					oaLoadedDataItems.Add(attribute->GetAttributeBlock());

					// Mise a jour des caracteristiques du bloc
					attributeBlock = attribute->GetAttributeBlock();
					attributeBlock->bLoaded = true;
					attributeBlock->liLoadIndex.SetDenseIndex(oaLoadedDataItems.GetSize() - 1);
				}

				// Memorisation l'index de l'attribut et eventuellement du bloc
				attribute->liLoadIndex.SetDenseIndex(oaLoadedDataItems.GetSize() - 1);
				if (attribute->IsInBlock())
				{
					// Memorisation de l'attribut charge dans le bloc
					attributeBlock = attribute->GetAttributeBlock();
					attributeBlock->oaLoadedAttributes.Add(attribute);
				}

				// Memorisation des attributs Symbol, hors bloc
				if (attribute->GetType() == KWType::Symbol and not attribute->IsInBlock())
					oaLoadedDenseSymbolAttributes.Add(attribute);
				// Memorisation des attribut de type relation, hors bloc
				else if (KWType::IsRelation(attribute->GetType()) and not attribute->IsInBlock())
					oaLoadedRelationAttributes.Add(attribute);
				// Memorisation des attributs Text
				else if (attribute->GetType() == KWType::Text)
					oaLoadedTextAttributes.Add(attribute);
				// Memorisation des attributs TextList
				else if (attribute->GetType() == KWType::TextList)
					oaLoadedTextListAttributes.Add(attribute);
			}
		}

		// Cas des attributs Object ou ObjectArray natifs non utilises ni charges en memoire
		if (not attribute->GetLoaded())
		{
			if (not attribute->IsInBlock() and attribute->GetDerivationRule() == NULL)
			{
				if (KWType::IsRelation(attribute->GetType()))
					oaUnloadedNativeRelationAttributes.Add(attribute);
			}
		}

		// Attribut suivant
		GetNextAttribute(attribute);
	}

	// Parcours des blocs d'attribut charges, pour les indexer sur la base d'un tri par VarKey
	for (nAttributeBlock = 0; nAttributeBlock < oaLoadedAttributeBlocks.GetSize(); nAttributeBlock++)
	{
		attributeBlock = cast(KWAttributeBlock*, oaLoadedAttributeBlocks.GetAt(nAttributeBlock));

		// Tri des attributs charges par VarKey, ce qui est necessaire pour ensuite
		// initialiser les IndexKeyBlock selon l'ordre requis
		attributeBlock->oaLoadedAttributes.SetCompareFunction(KWAttributeCompareVarKey);
		attributeBlock->oaLoadedAttributes.Sort();

		// Parcours des attributs charges du blocs
		for (nAttribute = 0; nAttribute < attributeBlock->oaLoadedAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, attributeBlock->oaLoadedAttributes.GetAt(nAttribute));

			if (attributeBlock->GetVarKeyType() == KWType::Symbol)
			{
				sVarKey = attributeBlock->GetSymbolVarKey(attribute);
				cast(KWIndexedCKeyBlock*, attributeBlock->loadedAttributesIndexedKeyBlock)
				    ->AddKey(sVarKey);
			}
			else
			{
				nVarKey = attributeBlock->GetContinuousVarKey(attribute);
				cast(KWIndexedNKeyBlock*, attributeBlock->loadedAttributesIndexedKeyBlock)
				    ->AddKey(nVarKey);
			}

			// Mise a jour de la partie sparse de l'index de l'attribut
			attribute->liLoadIndex.SetSparseIndex(nAttribute);
		}
	}

	// Calcul des index internes des attributs natifs a stocker
	nInternalLoadIndex = oaLoadedDataItems.GetSize();
	for (nIndex = 0; nIndex < oaUnloadedNativeRelationAttributes.GetSize(); nIndex++)
	{
		attribute = cast(KWAttribute*, oaUnloadedNativeRelationAttributes.GetAt(nIndex));
		attribute->liLoadIndex.SetDenseIndex(nInternalLoadIndex);
		nInternalLoadIndex++;
	}
	assert(nInternalLoadIndex == oaLoadedDataItems.GetSize() + oaUnloadedNativeRelationAttributes.GetSize());

	// Memorisation des index de chargement des attributs de la cle
	livKeyAttributeLoadIndexes.SetSize(GetKeyAttributeNumber());
	for (nIndex = 0; nIndex < GetKeyAttributeNumber(); nIndex++)
	{
		attribute = cast(KWAttribute*, odAttributes.Lookup(GetKeyAttributeNameAt(nIndex)));
		livKeyAttributeLoadIndexes.ResetAt(nIndex);

		// Memorisation de l'index du champ de la cle
		if (attribute != NULL)
			livKeyAttributeLoadIndexes.SetAt(nIndex, attribute->GetLoadIndex());

		// Si au moins un champ n'est pas charge, supression des acces aux index des champs cles
		if (not livKeyAttributeLoadIndexes.GetAt(nIndex).IsValid())
		{
			livKeyAttributeLoadIndexes.SetSize(0);
			break;
		}
	}
}

int KWClass::ComputeOverallNativeRelationAttributeNumber(boolean bIncludingReferences) const
{
	int nOverallNativeRelationAttributeNumber;
	ObjectDictionary odReferenceClasses;
	ObjectArray oaImpactedClasses;
	KWClass* kwcImpactedClass;
	KWClass* kwcRefClass;
	int nClass;
	KWAttribute* attribute;

	// On part de la classe de depart
	kwcImpactedClass = cast(KWClass*, this);
	oaImpactedClasses.Add(kwcImpactedClass);
	if (kwcImpactedClass->GetRoot())
		odReferenceClasses.SetAt(kwcImpactedClass->GetName(), kwcImpactedClass);

	// Duplication des classes referencees en memorisant les classes dupliquees
	// pour lesquelles il faut propager la duplication
	nOverallNativeRelationAttributeNumber = 0;
	for (nClass = 0; nClass < oaImpactedClasses.GetSize(); nClass++)
	{
		kwcImpactedClass = cast(KWClass*, oaImpactedClasses.GetAt(nClass));

		// Attributs de type Object ou ObjectArray
		attribute = kwcImpactedClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Si attribut avec classe referencee
			if (KWType::IsRelation(attribute->GetType()))
			{
				// Analyse si classe presente
				kwcRefClass = attribute->GetClass();
				if (kwcRefClass != NULL)
				{
					// Prise en compte si attribut natif interne
					if (attribute->GetAnyDerivationRule() == NULL)
					{
						nOverallNativeRelationAttributeNumber++;

						// Ajout de la classe a analyser
						oaImpactedClasses.Add(kwcRefClass);
					}
					// Prise en compte dans le cas d'une classe referencee
					else if (bIncludingReferences and
						 attribute->GetAnyDerivationRule()->GetName() ==
						     KWDerivationRule::GetReferenceRuleName())
					{
						// Ajout de la classe a analyser si elle ne l'a pas deja ete
						if (cast(KWClass*, odReferenceClasses.Lookup(kwcRefClass->GetName())) ==
						    NULL)
						{
							nOverallNativeRelationAttributeNumber++;

							// Memorisation de la classe externe pour ne pas faire l'analyse
							// plusieurs fois
							odReferenceClasses.SetAt(kwcRefClass->GetName(), kwcRefClass);
							oaImpactedClasses.Add(kwcRefClass);
						}
					}
				}
			}

			// Attribut suivant
			kwcImpactedClass->GetNextAttribute(attribute);
		}
	}
	return nOverallNativeRelationAttributeNumber;
}

int KWClass::ComputeInitialAttributeNumber(boolean bIsSupervised) const
{
	int nNumber;

	require(IsIndexed());

	nNumber = ivUsedAttributeNumbers.GetAt(KWType::Continuous) + ivUsedAttributeNumbers.GetAt(KWType::Symbol);
	if (bIsSupervised)
		nNumber--;
	return nNumber;
}

KWAttributeBlock* KWClass::CreateAttributeBlock(const ALString& sBlockName, KWAttribute* firstAttribute,
						KWAttribute* lastAttribute)
{
	boolean bOk = true;
	boolean bFullCheck = false;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;

	require(CheckName(sBlockName, KWClass::AttributeBlock, this));
	require(LookupAttribute(sBlockName) == NULL);
	require(LookupAttributeBlock(sBlockName) == NULL);
	require(firstAttribute != NULL);
	require(lastAttribute != NULL);
	require(LookupAttribute(firstAttribute->GetName()) == firstAttribute);
	require(LookupAttribute(lastAttribute->GetName()) == lastAttribute);

	// Verification de la coherence du bloc, uniquement en mode debug
	debug(bFullCheck = true);
	if (bFullCheck)
	{
		// Parcours des attributs
		attribute = firstAttribute;
		while (attribute != NULL)
		{
			// Test si l'attribut n'est pas deja dans un bloc
			if (attribute->GetAttributeBlock() != NULL)
			{
				AddError("Variable " + attribute->GetName() +
					 " cannot be in a block before block creation");
				bOk = false;
			}

			// Arret si derniere variable du bloc trouvee
			if (attribute == lastAttribute)
				break;
			GetNextAttribute(attribute);
		}

		// Erreur si l'attribut vaut NULL, signifiant que la derniere variable du bloc n'a pas ete trouvee
		if (attribute == NULL)
		{
			AddError("Last variable " + lastAttribute->GetName() + " not found after first variable " +
				 firstAttribute->GetName() + " during block creation");
			bOk = false;
		}
		if (not bOk)
			return NULL;
	}

	// Creation du block
	attributeBlock = new KWAttributeBlock;
	attributeBlock->SetName(sBlockName);
	odAttributeBlocks.SetAt(sBlockName, attributeBlock);

	// Parametrage du bloc
	attributeBlock->firstAttribute = firstAttribute;
	attributeBlock->lastAttribute = lastAttribute;

	// Parametrage du type de cle du bloc, selon le premier attribut du bloc
	if (firstAttribute->GetConstMetaData()->IsKeyPresent(attributeBlock->GetAttributeKeyMetaDataKey()))
	{
		if (firstAttribute->GetConstMetaData()->IsDoubleTypeAt(attributeBlock->GetAttributeKeyMetaDataKey()))
			attributeBlock->SetVarKeyType(KWType::Continuous);
		else if (firstAttribute->GetConstMetaData()->IsStringTypeAt(
			     attributeBlock->GetAttributeKeyMetaDataKey()))
			attributeBlock->SetVarKeyType(KWType::Symbol);
	}

	// Parametrage des attributs du bloc
	// Le bloc appartient a la classe a partir du momment ou il reference par au moin un de ses attributs
	attribute = firstAttribute;
	while (attribute != NULL)
	{
		attribute->attributeBlock = attributeBlock;

		// Arret si derniere variable du bloc trouvee
		if (attribute == lastAttribute)
			break;
		GetNextAttribute(attribute);
	}

	// Incrementation de la fraicheur de la classe
	UpdateFreshness();
	return attributeBlock;
}

void KWClass::InsertAttributeInBlock(KWAttribute* attribute, KWAttributeBlock* attributeBlockRef)
{
	require(attribute != NULL);
	require(CheckName(attribute->GetName(), KWClass::Attribute, attribute));
	require(attribute->parentClass == NULL);
	require(attribute->GetAttributeBlock() == NULL);
	require(attributeBlockRef != NULL);
	require(attributeBlockRef->GetFirstAttribute() != NULL);
	require(attributeBlockRef->GetLastAttribute() != NULL);
	require(LookupAttribute(attributeBlockRef->GetFirstAttribute()->GetName()) ==
		attributeBlockRef->GetFirstAttribute());
	require(LookupAttribute(attributeBlockRef->GetLastAttribute()->GetName()) ==
		attributeBlockRef->GetLastAttribute());
	require(LookupAttribute(attribute->GetName()) == NULL);
	require(LookupAttributeBlock(attribute->GetName()) == NULL);

	// Ajout dans le dictionnaire des attributs
	odAttributes.SetAt(attribute->GetName(), attribute);
	attribute->parentClass = this;

	// Ajout dans la liste, apres le dernier attribut du bloc
	attribute->listPosition = olAttributes.InsertAfter(attributeBlockRef->lastAttribute->listPosition, attribute);

	// Mise a jour des informations sur le block
	attribute->attributeBlock = attributeBlockRef;
	attributeBlockRef->lastAttribute = attribute;

	// Fraicheur
	nFreshness++;

	assert(odAttributes.GetCount() == olAttributes.GetCount());
}

KWAttributeBlock* KWClass::LookupAttributeBlock(const ALString& sBlockName) const
{
	return cast(KWAttributeBlock*, odAttributeBlocks.Lookup(sBlockName));
}

void KWClass::RemoveAttributeBlock(KWAttributeBlock* attributeBlock)
{
	KWAttribute* attribute;

	require(attributeBlock != NULL);
	require(attributeBlock->GetFirstAttribute() != NULL);
	require(attributeBlock->GetLastAttribute() != NULL);
	require(LookupAttribute(attributeBlock->GetFirstAttribute()->GetName()) == attributeBlock->GetFirstAttribute());
	require(LookupAttribute(attributeBlock->GetLastAttribute()->GetName()) == attributeBlock->GetLastAttribute());

	// Reinitialisation des attributs du bloc, en dereferencant le bloc
	attribute = attributeBlock->GetFirstAttribute();
	while (attribute != NULL)
	{
		attribute->attributeBlock = NULL;

		// Arret si derniere variable du bloc trouvee
		if (attribute == attributeBlock->GetLastAttribute())
			break;
		GetNextAttribute(attribute);
	}

	// Dereferencement du bloc de son dictionnaire
	odAttributeBlocks.RemoveKey(attributeBlock->GetName());

	// Incrementation de la fraicheur de la classe
	UpdateFreshness();
}

void KWClass::DeleteAttributeBlock(KWAttributeBlock* attributeBlock)
{
	RemoveAttributeBlock(attributeBlock);
	delete attributeBlock;
}

void KWClass::DeleteAllAttributeBlocks()
{
	KWAttributeBlock* attributeBlock;
	KWAttributeBlock* attributeBlockToDelete;

	// Parcours des blocks a detruire
	attributeBlock = GetHeadAttributeBlock();
	while (attributeBlock != NULL)
	{
		attributeBlockToDelete = attributeBlock;
		GetNextAttributeBlock(attributeBlock);
		DeleteAttributeBlock(attributeBlockToDelete);
	}
}

KWAttributeBlock* KWClass::GetHeadAttributeBlock() const
{
	KWAttribute* attribute;

	// Parcours des attribut de la classe, jusqu'a trouver le premier block
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// On retourne le premier bloc trouve
		if (attribute->GetAttributeBlock() != NULL)
		{
			assert(attribute->GetAttributeBlock()->GetFirstAttribute() == attribute);
			assert(LookupAttributeBlock(attribute->GetAttributeBlock()->GetName()) ==
			       attribute->GetAttributeBlock());
			return attribute->GetAttributeBlock();
		}
		GetNextAttribute(attribute);
	}

	// On retourne NULL si aucun bloc trouve
	return NULL;
}

KWAttributeBlock* KWClass::GetTailAttributeBlock() const
{
	KWAttribute* attribute;

	// Parcours des attribut de la classe, jusqu'a trouver le premier block
	attribute = GetTailAttribute();
	while (attribute != NULL)
	{
		// On retourne le premier bloc trouve en partant de la fin
		if (attribute->GetAttributeBlock() != NULL)
		{
			assert(attribute->GetAttributeBlock()->GetLastAttribute() == attribute);
			assert(LookupAttributeBlock(attribute->GetAttributeBlock()->GetName()) ==
			       attribute->GetAttributeBlock());
			return attribute->GetAttributeBlock();
		}
		GetPrevAttribute(attribute);
	}

	// On retourne NULL si aucun bloc trouve
	return NULL;
}

void KWClass::GetNextAttributeBlock(KWAttributeBlock*& attributeBlock) const
{
	KWAttribute* attribute;

	require(attributeBlock != NULL);
	require(attributeBlock->GetFirstAttribute() ==
		cast(KWAttribute*, odAttributes.Lookup(attributeBlock->GetFirstAttribute()->GetName())));
	require(attributeBlock->GetFirstAttribute()->GetParentClass() == this);
	require(LookupAttributeBlock(attributeBlock->GetName()) == attributeBlock);

	// Parcours des attributs suivant la fin du bloc courant
	attribute = attributeBlock->GetLastAttribute();
	GetNextAttribute(attribute);
	while (attribute != NULL)
	{
		// On retourne le premier bloc trouve
		if (attribute->GetAttributeBlock() != NULL)
		{
			assert(attribute->GetAttributeBlock()->GetFirstAttribute() == attribute);
			attributeBlock = attribute->GetAttributeBlock();
			ensure(LookupAttributeBlock(attributeBlock->GetName()) == attributeBlock);
			return;
		}
		GetNextAttribute(attribute);
	}

	// On retourne NULL si aucun bloc trouve
	attributeBlock = NULL;
}

void KWClass::GetPrevAttributeBlock(KWAttributeBlock*& attributeBlock) const
{
	KWAttribute* attribute;

	require(attributeBlock != NULL);
	require(attributeBlock->GetLastAttribute() ==
		cast(KWAttribute*, odAttributes.Lookup(attributeBlock->GetLastAttribute()->GetName())));
	require(attributeBlock->GetLastAttribute()->GetParentClass() == this);
	require(LookupAttributeBlock(attributeBlock->GetName()) == attributeBlock);

	// Parcours des attributs precedent le debut du bloc courant
	attribute = attributeBlock->GetFirstAttribute();
	GetPrevAttribute(attribute);
	while (attribute != NULL)
	{
		// On retourne le premier bloc trouve
		if (attribute->GetAttributeBlock() != NULL)
		{
			assert(attribute->GetAttributeBlock()->GetLastAttribute() == attribute);
			attributeBlock = attribute->GetAttributeBlock();
			ensure(LookupAttributeBlock(attributeBlock->GetName()) == attributeBlock);
			return;
		}
		GetPrevAttribute(attribute);
	}

	// On retourne NULL si aucun bloc trouve
	attributeBlock = NULL;
}

void KWClass::MoveAttributeBefore(KWAttribute* attribute, KWAttribute* attributeRef)
{
	require(attributeRef != NULL);
	require(attributeRef == cast(KWAttribute*, odAttributes.Lookup(attributeRef->GetName())));
	require(attributeRef->parentClass == this);
	require(not attributeRef->IsInBlock() or
		attributeRef == attributeRef->GetAttributeBlock()->GetFirstAttribute());
	require(attribute != NULL);
	require(LookupAttribute(attribute->GetName()) == attribute);
	require(not attribute->IsInBlock());
	require(attribute != attributeRef);

	// Suppression de l'attribut de sa position courante
	olAttributes.RemoveAt(attribute->listPosition);

	// Ajout dans la liste a la nouvelle position
	attribute->listPosition = olAttributes.InsertBefore(attributeRef->listPosition, attribute);

	// Fraicheur
	nFreshness++;
	ensure(odAttributes.GetCount() == olAttributes.GetCount());
}

void KWClass::MoveAttributeAfter(KWAttribute* attribute, KWAttribute* attributeRef)
{
	require(attributeRef != NULL);
	require(attributeRef == cast(KWAttribute*, odAttributes.Lookup(attributeRef->GetName())));
	require(attributeRef->parentClass == this);
	require(not attributeRef->IsInBlock() or attributeRef == attributeRef->GetAttributeBlock()->GetLastAttribute());
	require(attribute != NULL);
	require(LookupAttribute(attribute->GetName()) == attribute);
	require(not attribute->IsInBlock());
	require(attribute != attributeRef);

	// Suppression de l'attribut de sa position courante
	olAttributes.RemoveAt(attribute->listPosition);

	// Ajout dans la liste a la nouvelle position
	attribute->listPosition = olAttributes.InsertAfter(attributeRef->listPosition, attribute);

	// Fraicheur
	nFreshness++;
	ensure(odAttributes.GetCount() == olAttributes.GetCount());
}

void KWClass::MoveAttributeToClassTail(KWAttribute* attribute)
{
	require(attribute != NULL);
	require(LookupAttribute(attribute->GetName()) == attribute);
	require(not attribute->IsInBlock());

	// Suppression de l'attribut de sa position courante
	olAttributes.RemoveAt(attribute->listPosition);

	// Ajout en fin de liste
	attribute->listPosition = olAttributes.AddTail(attribute);

	// Fraicheur
	nFreshness++;
	ensure(odAttributes.GetCount() == olAttributes.GetCount());
}

void KWClass::MoveAttributeToBlockTail(KWAttribute* attribute)
{
	KWAttributeBlock* attributeBlock;

	require(attribute != NULL);
	require(LookupAttribute(attribute->GetName()) == attribute);
	require(attribute->IsInBlock());

	// Depalacement a effectuer seulement si l'attribut n'est pas deja le dernier de son bloc
	attributeBlock = attribute->GetAttributeBlock();
	if (attribute != attributeBlock->GetLastAttribute())
	{
		// Changement de premier attribut si l'attribut etait le premier
		if (attribute == attributeBlock->GetFirstAttribute())
			GetNextAttribute(attributeBlock->firstAttribute);

		// Suppression de l'attribut de sa position courante dans la liste de tous les attributs
		olAttributes.RemoveAt(attribute->listPosition);

		// Ajout en fin de bloc
		attribute->listPosition =
		    olAttributes.InsertAfter(attributeBlock->lastAttribute->listPosition, attribute);
		attributeBlock->lastAttribute = attribute;
	}

	// Fraicheur
	nFreshness++;
	ensure(odAttributes.GetCount() == olAttributes.GetCount());
}

void KWClass::MoveAttributeBlockToClassTail(KWAttributeBlock* attributeBlock)
{
	KWAttribute* attribute;
	KWAttribute* nextAttribute;

	require(attributeBlock != NULL);
	require(attributeBlock->GetFirstAttribute() != NULL);
	require(attributeBlock->GetLastAttribute() != NULL);
	require(LookupAttribute(attributeBlock->GetFirstAttribute()->GetName()) == attributeBlock->GetFirstAttribute());
	require(LookupAttribute(attributeBlock->GetLastAttribute()->GetName()) == attributeBlock->GetLastAttribute());
	require(LookupAttributeBlock(attributeBlock->GetName()) == attributeBlock);

	// Deplacement des attributs du bloc
	// Les attribut restent dans le dictionnaire des attribut de la classes et leur bloc ne change pas
	// Il suffit de les deplacer dans la liste des attributs
	attribute = attributeBlock->GetFirstAttribute();
	while (attribute != NULL)
	{
		attribute->attributeBlock = attributeBlock;
		assert(attribute->GetAttributeBlock() == attributeBlock);

		// On accede a l'attribut suivant sans changer l'attribut en cours
		nextAttribute = attribute;
		GetNextAttribute(nextAttribute);

		// Suppression de l'attribut de sa position courante
		olAttributes.RemoveAt(attribute->listPosition);

		// Ajout en fin de liste
		attribute->listPosition = olAttributes.AddTail(attribute);

		// Arret si derniere variable du bloc trouvee
		if (attribute == attributeBlock->GetLastAttribute())
			break;

		// Passage a l'attribut suivant
		attribute = nextAttribute;
	}

	// Fraicheur
	nFreshness++;
	ensure(odAttributes.GetCount() == olAttributes.GetCount());
}

void KWClass::Compile()
{
	KWAttribute* attribute;

	require(Check());

	// Arret si deja compile
	if (nFreshness == nCompileFreshness)
		return;

	// Indexation de la classe
	IndexClass();

	// Mise a jour de la fraicheur de compilation
	nCompileFreshness = nFreshness;

	// Compilation des regles de derivation
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		attribute->Compile();
		GetNextAttribute(attribute);
	}

	// Parametrage specifique de toutes les regles Random utilisees dans la classe
	InitializeAllRandomRuleParameters();
}

const ALString KWClass::BuildAttributeName(const ALString& sPrefix)
{
	ALString sAttributeName;
	ALString sSuffix;
	int nIndex;

	// Recherche d'un nom inutilise par les attributs ou blocs, en suffixant par un index croissant
	nIndex = 1;
	sAttributeName = sPrefix;
	if (sAttributeName == "")
		sAttributeName = "_";
	while ((sAttributeName.GetLength() > KWClass::GetNameMaxLength()) or
	       odAttributes.Lookup(sAttributeName) != NULL or odAttributeBlocks.Lookup(sAttributeName) != NULL)
	{
		sSuffix = IntToString(nIndex);
		sAttributeName = sPrefix + "_" + sSuffix;

		// Reduction si necessaire de la taille du nom
		if (sAttributeName.GetLength() > KWClass::GetNameMaxLength())
		{
			sAttributeName = BuildUTF8SubString(sAttributeName.Left(KWClass::GetNameMaxLength() -
										sSuffix.GetLength() - 1)) +
					 "_" + sSuffix;
		}

		// Passage au nom suivant
		nIndex++;
	}
	return sAttributeName;
}

const ALString KWClass::BuildAttributeBlockName(const ALString& sPrefix)
{
	return BuildAttributeName(sPrefix);
}

void KWClass::DeleteUnusedDerivedAttributes(const KWClassDomain* referenceDomain)
{
	ObjectDictionary odImpactedClasses;
	ObjectArray oaImpactedClasses;
	int nClass;
	KWClass* kwcImpactedClass;
	NumericKeyDictionary nkdAllUsedAttributes;
	ObjectArray oaUnusedAttributes;
	KWAttribute* attribute;
	KWDerivationRule* currentDerivationRule;
	KWAttribute* referenceAttribute;
	KWClass* referenceClass;
	int i;

	require(Check());
	require(IsCompiled());

	// Enregistrement de la classe a analyser
	odImpactedClasses.SetAt(GetName(), this);
	oaImpactedClasses.Add(this);

	// Parcours de toutes les classe en partant de la classe analysee avec analyse des attributs
	// pour identifier toutes les operandes necessaire dans le calcul des attributs derives utilises.
	// Dans le cas d'un domaine de reference, on analyse egalement les classe impactees de ce domaine
	// de reference pour garder tous les attributs initiaux et les attributs necessaires a leur calcul
	// Attention, les formules de calcul sont utilisees est celles du domaine courant, pas celle du domaine initial
	for (nClass = 0; nClass < oaImpactedClasses.GetSize(); nClass++)
	{
		kwcImpactedClass = cast(KWClass*, oaImpactedClasses.GetAt(nClass));

		// Parcours des attributs pour identifier les attributs a detruire
		attribute = kwcImpactedClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Recherche si necessaire de l'attribut de reference correspondant
			referenceAttribute = NULL;
			if (referenceDomain != NULL)
			{
				referenceClass = referenceDomain->LookupClass(attribute->GetParentClass()->GetName());
				if (referenceClass != NULL)
					referenceAttribute = referenceClass->LookupAttribute(attribute->GetName());
			}

			// Analyse de la regle de derivation si l'attribut est utilise, ou present dans le domaine de
			// reference, ou s'il s'agit d'une regle de reference necessaire a la structure du schema
			// multi-table Dans le cas d'un bloc, il faut en effet la reanalyser pour chaque attribut du
			// bloc pour detecter les attributs utilises des blocs potentiellement en operande
			currentDerivationRule = attribute->GetAnyDerivationRule();
			if (currentDerivationRule != NULL)
			{
				if (attribute->GetUsed() or referenceAttribute != NULL)
					currentDerivationRule->BuildAllUsedAttributes(attribute, &nkdAllUsedAttributes);
				else if (currentDerivationRule->GetName() == KWDerivationRule::GetReferenceRuleName())
				{
					currentDerivationRule->BuildAllUsedAttributes(attribute, &nkdAllUsedAttributes);

					// Memorisation de l'attribut porteur de la reference vers une table externe
					nkdAllUsedAttributes.SetAt(attribute, attribute);
				}
			}

			// Si attribut avec classe referencee, enregistrement de la classe impactee correspondante
			if (KWType::IsRelation(attribute->GetType()))
			{
				if (attribute->GetClass() != NULL)
				{
					// Prise en compte si necessaire d'une nouvelle classe
					if (odImpactedClasses.Lookup(attribute->GetClass()->GetName()) == NULL)
					{
						odImpactedClasses.SetAt(attribute->GetClass()->GetName(),
									attribute->GetClass());
						oaImpactedClasses.Add(attribute->GetClass());
					}
				}
			}

			// Attribut suivant
			kwcImpactedClass->GetNextAttribute(attribute);
		}
	}

	// Nettoyage
	odImpactedClasses.RemoveAll();
	oaImpactedClasses.SetSize(0);

	// Enregistrement de la classe a analyser
	odImpactedClasses.SetAt(GetName(), this);
	oaImpactedClasses.Add(this);

	// Parcours de toutes les classes impactees
	for (nClass = 0; nClass < oaImpactedClasses.GetSize(); nClass++)
	{
		kwcImpactedClass = cast(KWClass*, oaImpactedClasses.GetAt(nClass));

		// Parcours des attributs pour identifier les attributs a detruire
		attribute = kwcImpactedClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Attribut a detruire s'il est derive ou dans un bloc, et non utilise
			if (not attribute->GetUsed() and
			    (attribute->GetDerivationRule() != NULL or attribute->IsInBlock()) and
			    nkdAllUsedAttributes.Lookup(attribute) == NULL)
			{
				// Attribut a detruire si absent du domaine de reference
				referenceAttribute = NULL;
				if (referenceDomain != NULL)
				{
					referenceClass =
					    referenceDomain->LookupClass(attribute->GetParentClass()->GetName());
					if (referenceClass != NULL)
						referenceAttribute =
						    referenceClass->LookupAttribute(attribute->GetName());
				}
				if (referenceDomain == NULL or referenceAttribute == NULL)
					oaUnusedAttributes.Add(attribute);
			}

			// Si attribut avec classe referencee, enregistrement de la classe impactee correspondante
			if (KWType::IsRelation(attribute->GetType()))
			{
				if (attribute->GetClass() != NULL)
				{
					// Prise en compte si necessaire d'une nouvelle classe
					if (odImpactedClasses.Lookup(attribute->GetClass()->GetName()) == NULL)
					{
						odImpactedClasses.SetAt(attribute->GetClass()->GetName(),
									attribute->GetClass());
						oaImpactedClasses.Add(attribute->GetClass());
					}
				}
			}

			// Attribut suivant
			kwcImpactedClass->GetNextAttribute(attribute);
		}
	}

	// Destruction des attributs ainsi identifies
	for (i = 0; i < oaUnusedAttributes.GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaUnusedAttributes.GetAt(i));
		attribute->GetParentClass()->DeleteAttribute(attribute->GetName());
	}
	ensure(Check());
}

void KWClass::CompleteTypeInfo()
{
	NumericKeyDictionary nkdAttributes;
	KWAttribute* attribute;

	// Parcours des attributs
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		attribute->InternalCompleteTypeInfo(this, &nkdAttributes);

		// Attribut suivant
		GetNextAttribute(attribute);
	}
}

void KWClass::BuildAllUsedClasses(ObjectArray* oaUsedClasses) const
{
	NumericKeyDictionary nkdUsedClasses;
	int nClass;
	KWClass* kwcUsedClass;
	KWAttribute* attribute;

	require(oaUsedClasses != NULL);
	require(IsCompiled());

	// Initialisation avec la classe courante
	oaUsedClasses->RemoveAll();
	oaUsedClasses->Add((Object*)this);
	nkdUsedClasses.SetAt(this, (Object*)this);

	// Parcours des classes utilises, en completant le tableau au fur et a mesure
	// pour lesquelles il faut propager la duplication
	for (nClass = 0; nClass < oaUsedClasses->GetSize(); nClass++)
	{
		kwcUsedClass = cast(KWClass*, oaUsedClasses->GetAt(nClass));

		// Attributs de type Object ou ObjectArray
		attribute = kwcUsedClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Si sttribut avec classe referencee
			if (KWType::IsRelation(attribute->GetType()))
			{
				if (attribute->GetClass() != NULL)
				{
					// Memorisation si necessaire
					if (nkdUsedClasses.Lookup(attribute->GetClass()) == NULL)
					{
						nkdUsedClasses.SetAt(attribute->GetClass(), attribute->GetClass());
						oaUsedClasses->Add(attribute->GetClass());
					}
				}
			}

			// Attribut suivant
			kwcUsedClass->GetNextAttribute(attribute);
		}
	}
}

void KWClass::ExportNativeFieldNames(StringVector* svNativeFieldNames) const
{
	KWAttribute* attribute;

	require(svNativeFieldNames != NULL);

	svNativeFieldNames->SetSize(0);
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// On ne n'exporte que les attribut natifs
		if (attribute->IsNative())
		{
			// Cas d'un attribut dense
			if (not attribute->IsInBlock())
				svNativeFieldNames->Add(attribute->GetName());
			// Cas d'un bloc d'attributs
			else
			{
				// On ne traite le bloc qu'une seule fois, pour le premier attribut du bloc
				if (attribute->IsFirstInBlock())
					svNativeFieldNames->Add(attribute->GetAttributeBlock()->GetName());
			}
		}
		GetNextAttribute(attribute);
	}
}

void KWClass::ExportStoredFieldNames(StringVector* svStoredFieldNames) const
{
	int i;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;

	require(svStoredFieldNames != NULL);

	// Parcours de tous les dataItems Loaded
	svStoredFieldNames->SetSize(0);
	for (i = 0; i < GetLoadedDataItemNumber(); i++)
	{
		dataItem = GetLoadedDataItemAt(i);

		// Cas d'un attribut dense
		if (dataItem->IsAttribute())
		{
			attribute = cast(KWAttribute*, dataItem);
			if (KWType::IsStored(attribute->GetType()) and attribute->GetLoaded())
				svStoredFieldNames->Add(attribute->GetName());
		}
		// Cas d'un bloc d'attributs
		else
		{
			attributeBlock = cast(KWAttributeBlock*, dataItem);
			svStoredFieldNames->Add(attributeBlock->GetName());
		}
	}
}

void KWClass::ExportKeyAttributeNames(StringVector* svAttributeNames) const
{
	int i;
	KWAttribute* attribute;

	require(svAttributeNames != NULL);

	svAttributeNames->SetSize(GetKeyAttributeNumber());
	for (i = 0; i < GetKeyAttributeNumber(); i++)
	{
		attribute = GetKeyAttributeAt(i);
		svAttributeNames->SetAt(i, attribute->GetName());
	}
}

void KWClass::CopyFrom(const KWClass* aSource)
{
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWAttributeBlock* copyAttributeBlock;
	KWAttribute* copyFirstAttribute;
	KWAttribute* copyLastAttribute;
	int i;

	require(aSource != NULL);
	require(GetAttributeNumber() == 0);
	require(GetDomain() == NULL);
	require(nFreshness == 0);
	require(nIndexFreshness == 0);
	require(nCompileFreshness == 0);

	// Duplication des attributs de base
	usName = aSource->usName;
	usLabel = aSource->usLabel;
	bRoot = aSource->bRoot;

	// Duplication des meta-donnees
	metaData.CopyFrom(&aSource->metaData);

	// Duplication des noms d'attributs de la cle
	SetKeyAttributeNumber(aSource->GetKeyAttributeNumber());
	for (i = 0; i < aSource->GetKeyAttributeNumber(); i++)
		SetKeyAttributeNameAt(i, aSource->GetKeyAttributeNameAt(i));

	// Duplication des attributs
	attribute = aSource->GetHeadAttribute();
	while (attribute != NULL)
	{
		InsertAttribute(attribute->Clone());

		// Attribut suivant
		aSource->GetNextAttribute(attribute);
	}

	// Duplication des blocs
	attributeBlock = aSource->GetHeadAttributeBlock();
	while (attributeBlock != NULL)
	{
		// Duplication si bloc valide
		if (attributeBlock->GetFirstAttribute() != NULL and attributeBlock->GetLastAttribute() != NULL)
		{
			// Caracteristique du bloc duplique
			copyFirstAttribute = LookupAttribute(attributeBlock->GetFirstAttribute()->GetName());
			copyLastAttribute = LookupAttribute(attributeBlock->GetLastAttribute()->GetName());

			// Duplication si on abouti a un bloc valide
			if (copyFirstAttribute != NULL and copyLastAttribute != NULL)
			{
				// Duplication du bloc dans la classe copiee
				copyAttributeBlock = CreateAttributeBlock(attributeBlock->GetName(), copyFirstAttribute,
									  copyLastAttribute);

				// Memorisation des caracteristique du bloc duplique
				if (attributeBlock->GetDerivationRule() != NULL)
					copyAttributeBlock->SetDerivationRule(
					    attributeBlock->GetDerivationRule()->Clone());
				copyAttributeBlock->GetMetaData()->CopyFrom(attributeBlock->GetConstMetaData());
				copyAttributeBlock->SetLabel(attributeBlock->GetLabel());
			}
		}

		// Bloc suivant
		aSource->GetNextAttributeBlock(attributeBlock);
	}
}

KWClass* KWClass::Clone() const
{
	KWClass* kwcClone;

	kwcClone = new KWClass;
	kwcClone->CopyFrom(this);
	return kwcClone;
}

boolean KWClass::Check() const
{
	boolean bResult = true;
	KWAttribute* attribute;
	int i;
	NumericKeyDictionary nkdKeyAttributes;
	NumericKeyDictionary nkdComponentClasses;
	ALString sTmp;

	assert(odAttributes.GetCount() == olAttributes.GetCount());

	// Arret si deja compile
	if (nFreshness == nCompileFreshness)
		return true;

	// Verification de l'existence d'un domaine de classe
	if (domain == NULL)
	{
		bResult = false;
		AddError(sTmp + "No domain for the dictionary");
	}
	else if (domain->LookupClass(GetName()) != this)
	{
		bResult = false;
		AddError(sTmp + "The dictionary is not found in its domain");
	}

	// Verification du Name
	if (not CheckName(GetName(), KWClass::Class, this)) // Emission d'un message
	{
		bResult = false;
	}

	// Verification du Label
	if (not CheckLabel(GetLabel(), KWClass::Class, this)) // Emission d'un message
	{
		bResult = false;
	}

	// Verification des attributs
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// Test de l'attribut
		if (not attribute->Check())
			bResult = false;

		// Attribut suivant
		GetNextAttribute(attribute);
	}

	// Verification de la coherence entre statut racine/composant et presence de cle
	if (bResult and GetRoot() and GetKeyAttributeNumber() == 0)
	{
		bResult = false;
		AddError(sTmp + "Root dictionary should have a key");
	}

	// Verification de la cle
	if (bResult and GetKeyAttributeNumber() > 0)
	{
		// Verification de chaque champ de la cle
		for (i = 0; i < GetKeyAttributeNumber(); i++)
		{
			attribute = LookupAttribute(GetKeyAttributeNameAt(i));

			// Existance de la cle
			if (attribute == NULL)
			{
				bResult = false;
				AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) +
					 " not found in dictionary");
			}
			// La cle doit etre de type Symbol
			else if (attribute->GetType() != KWType::Symbol)
			{
				bResult = false;
				AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) + " must be of type " +
					 KWType::ToString(KWType::Symbol));
			}
			// La cle ne doit pas etre calculee
			else if (attribute->GetDerivationRule() != NULL)
			{
				bResult = false;
				AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) +
					 " must be a native variable, without derivation rule");
			}
			// La cle ne doit pas etre dans un bloc
			else if (attribute->GetBlockDerivationRule() != NULL)
			{
				bResult = false;
				AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) +
					 " must be not belong to a sparse value block computed from a derivation rule");
			}
			// Un meme attribut ne doit pas etre utilise plusieurs fois pour la cle
			else
			{
				// Test d'utilisation de l'attribut pour la cle
				if (nkdKeyAttributes.Lookup(attribute) != NULL)
				{
					bResult = false;
					AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) +
						 " used several times in the key");
				}
				// Memorisation de l'utilisation de l'attribut pour la cle
				else
					nkdKeyAttributes.SetAt(attribute, attribute);
			}
		}
	}

	// Verification de l'absence de cycle dans la composition
	if (bResult)
		bResult = CheckClassComposition(NULL, &nkdComponentClasses);

	return bResult;
}

longint KWClass::GetUsedMemory() const
{
	longint lUsedMemory;
	longint lAttributeUsedMemory;
	longint lAttributeBlockUsedMemory;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;

	// Memoire occupee par la classe elle meme
	// On prend en compte la memoire du nom et du libelle, meme s'ils sont potentiellement partages
	lUsedMemory = sizeof(KWClass);
	lUsedMemory += usName.GetUsedMemory();
	lUsedMemory += usLabel.GetUsedMemory();
	lUsedMemory += svKeyAttributeNames.GetUsedMemory();
	lUsedMemory += livKeyAttributeLoadIndexes.GetUsedMemory();

	// Prise en compte du referencement des attributs
	lUsedMemory += odAttributes.GetUsedMemory();
	lUsedMemory += olAttributes.GetUsedMemory();
	lUsedMemory += odAttributeBlocks.GetUsedMemory();
	lUsedMemory += oaUsedAttributes.GetUsedMemory();
	lUsedMemory += oaLoadedAttributes.GetUsedMemory();
	lUsedMemory += oaLoadedDenseAttributes.GetUsedMemory();
	lUsedMemory += oaLoadedAttributeBlocks.GetUsedMemory();
	lUsedMemory += oaLoadedDenseSymbolAttributes.GetUsedMemory();
	lUsedMemory += oaLoadedTextAttributes.GetUsedMemory();
	lUsedMemory += oaLoadedTextListAttributes.GetUsedMemory();
	lUsedMemory += oaLoadedRelationAttributes.GetUsedMemory();
	lUsedMemory += oaUnloadedNativeRelationAttributes.GetUsedMemory();
	lUsedMemory += oaLoadedDataItems.GetUsedMemory();
	lUsedMemory += ivUsedAttributeNumbers.GetUsedMemory();
	lUsedMemory += ivUsedDenseAttributeNumbers.GetUsedMemory();
	lUsedMemory += ivUsedSparseAttributeNumbers.GetUsedMemory();

	// Prise en compte du contenu des attributs eux-meme
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		lAttributeUsedMemory = attribute->GetUsedMemory();
		lUsedMemory += lAttributeUsedMemory;
		GetNextAttribute(attribute);
	}

	// Prise en compte des blocs d'attribut
	attributeBlock = GetHeadAttributeBlock();
	while (attributeBlock != NULL)
	{
		lAttributeBlockUsedMemory = attributeBlock->GetUsedMemory();
		lUsedMemory += lAttributeBlockUsedMemory;
		GetNextAttributeBlock(attributeBlock);
	}
	return lUsedMemory;
}

longint KWClass::ComputeHashValue() const
{
	KWAttribute* attribute;
	int nKey;

	// Calcul uniquement si necessaire
	if (nFreshness != nHashFreshness)
	{
		// Parcours des attributs de la classe
		lClassHashValue = HashValue(GetName());
		attribute = GetHeadAttribute();
		while (attribute != NULL)
		{
			// Prise en compte du bloc
			if (attribute->IsFirstInBlock())
				lClassHashValue = LongintUpdateHashValue(
				    lClassHashValue, attribute->GetAttributeBlock()->ComputeHashValue());
			lClassHashValue = LongintUpdateHashValue(lClassHashValue, attribute->ComputeHashValue());
			GetNextAttribute(attribute);
		}

		// Prise en compte de la racine
		if (GetRoot())
			lClassHashValue = LongintUpdateHashValue(lClassHashValue, HashValue("Root"));

		// Prise en compte des attributs cles
		for (nKey = 0; nKey < GetKeyAttributeNumber(); nKey++)
			lClassHashValue =
			    LongintUpdateHashValue(lClassHashValue, HashValue(GetKeyAttributeNameAt(nKey)));

		// Prise en compte des meta-data
		lClassHashValue = LongintUpdateHashValue(lClassHashValue, metaData.ComputeHashValue());

		// Memorisation de la fraicheur de calcul
		nHashFreshness = nFreshness;
	}
	return lClassHashValue;
}

void KWClass::Write(ostream& ost) const
{
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	int i;

	// Impression de l'entete de la classe
	ost << "\n";
	if (GetLabel() != "")
		ost << "// " << GetLabel() << "\n";
	if (GetRoot())
		ost << "Root\t";
	ost << "Dictionary\t" << GetExternalName(GetName());
	if (GetKeyAttributeNumber() > 0)
	{
		ost << "\t(";
		for (i = 0; i < GetKeyAttributeNumber(); i++)
		{
			if (i > 0)
				ost << ", ";
			ost << KWClass::GetExternalName(GetKeyAttributeNameAt(i));
		}
		ost << ")";
	}
	ost << "\n";
	if (metaData.GetKeyNumber() > 0)
	{
		metaData.Write(ost);
		ost << "\n";
	}
	ost << "{\n";

	// Impression de tous les attributs
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// Debut de bloc si necessaire
		if (attribute->IsFirstInBlock())
			ost << "\t{\n";

		// Impression de l'attribut
		ost << *attribute << "\n";

		// Fin de bloc si necessaire
		if (attribute->IsLastInBlock())
		{
			ost << "\t}";

			// Nom du bloc
			attributeBlock = attribute->GetAttributeBlock();
			ost << "\t" << KWClass::GetExternalName(attributeBlock->GetName());
			ost << "\t";

			// Regle de derivation
			if (attributeBlock->GetDerivationRule() != NULL)
			{
				// Dans le cas de la regle predefinie de Reference, on n'utilise pas le signe '='
				if (attributeBlock->GetDerivationRule()->GetName() !=
				    KWDerivationRule::GetReferenceRuleName())
					ost << " = ";
				attributeBlock->GetDerivationRule()->WriteUsedRule(ost);
			}

			// Fin de declaration
			ost << "\t;";

			// Meta-donnees
			if (attributeBlock->GetConstMetaData()->GetKeyNumber() > 0)
			{
				ost << ' ';
				attributeBlock->GetConstMetaData()->Write(ost);
			}
			ost << "\t";

			// Commentaire
			if (attributeBlock->GetLabel() != "")
				ost << "// " << attributeBlock->GetLabel();
			ost << "\n";
		}

		// Attribut suivant
		GetNextAttribute(attribute);
	}

	// Impression de la fin de la classe
	ost << "};\n";
}

const ALString KWClass::GetClassLabel() const
{
	return "Dictionary";
}

const ALString KWClass::GetObjectLabel() const
{
	return GetName();
}

void KWClass::WriteJSONFields(JSONFile* fJSON)
{
	KWAttribute* attribute;
	int i;

	// Entete de la classe
	fJSON->WriteKeyString("name", GetName());
	if (GetLabel() != "")
		fJSON->WriteKeyString("label", GetLabel());
	if (GetRoot())
		fJSON->WriteKeyBoolean("root", true);
	if (GetKeyAttributeNumber() > 0)
	{
		fJSON->BeginKeyList("key");
		for (i = 0; i < GetKeyAttributeNumber(); i++)
			fJSON->WriteString(GetKeyAttributeNameAt(i));
		fJSON->EndList();
	}
	if (metaData.GetKeyNumber() > 0)
		metaData.WriteJSONKeyReport(fJSON, "metaData");

	// Attributs et blocs d'attributs
	fJSON->BeginKeyArray("variables");
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// Debut de bloc si necessaire
		if (attribute->IsFirstInBlock())
			attribute->GetAttributeBlock()->WriteJSONReport(fJSON);
		// Impression de l'attribut s'il n'est pas dans un bloc
		else if (not attribute->IsInBlock())
			attribute->WriteJSONReport(fJSON);

		// Attribut suivant
		GetNextAttribute(attribute);
	}
	fJSON->EndArray();
}

void KWClass::WriteJSONReport(JSONFile* fJSON)
{
	fJSON->BeginObject();
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}

void KWClass::WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey)
{
	fJSON->BeginKeyObject(sKey);
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}

int KWClass::GetNameMaxLength()
{
	return 128;
}

const ALString KWClass::EntityToString(int nEntity)
{
	require(0 <= nEntity and nEntity < Unknown);
	switch (nEntity)
	{
	case ClassDomain:
		return "dictionary domain";
	case Class:
		return "dictionary";
	case Attribute:
		return "variable";
	case AttributeBlock:
		return "sparse variable block";
	case Rule:
		return "rule";
	case Structure:
		return "structure";
	case None:
		return "";
	default:
		assert(false);
		return "";
	}
}

boolean KWClass::CheckName(const ALString& sValue, int nEntity, const Object* errorSender)
{
	ALString sMessage;
	boolean bOk;

	require(0 <= nEntity and nEntity < Unknown);

	bOk = CheckNameWithMessage(sValue, nEntity, sMessage);
	if (not bOk and errorSender != NULL)
		errorSender->AddError(sMessage);
	return bOk;
}

boolean KWClass::CheckLabel(const ALString& sValue, int nEntity, const Object* errorSender)
{
	ALString sMessage;
	boolean bOk;

	require(0 <= nEntity and nEntity < Unknown);

	bOk = CheckLabelWithMessage(sValue, nEntity, sMessage);
	if (not bOk and errorSender != NULL)
		errorSender->AddError(sMessage);
	return bOk;
}

boolean KWClass::CheckNameWithMessage(const ALString& sValue, int nEntity, ALString& sMessage)
{
	ALString sTmp;
	boolean bOk;

	require(0 <= nEntity and nEntity < Unknown);

	// Test de la taille minimale
	sMessage = "";
	bOk = sValue.GetLength() >= 1;
	if (not bOk)
		sMessage = "Empty " + EntityToString(nEntity) + " name";

	// Test de la taille maximale
	if (bOk)
	{
		bOk = sValue.GetLength() <= GetNameMaxLength();
		if (not bOk)
			sMessage = sTmp + "Incorrect " + EntityToString(nEntity) + " name : length > " +
				   IntToString(GetNameMaxLength()) + "\n\t(" + sValue.Left(GetNameMaxLength()) + "...)";
	}

	// Test des caracteres interdits
	if (bOk)
	{
		// Test du saut de ligne
		bOk = sValue.Find('\n') == -1;
		if (not bOk)
			sMessage = "Incorrect " + EntityToString(nEntity) + " name (" + sValue +
				   ") : must not contain end-of-line chararacters";
	}

	// Test de l'absence de caractere d'espace en debut et fin
	if (bOk)
	{
		assert(sValue.GetLength() >= 1);
		bOk = not iswspace(sValue.GetAt(0));
		if (not bOk)
			sMessage = "Incorrect " + EntityToString(nEntity) + " name (" + sValue +
				   ") : must not contain any leading space chararacters";
	}
	if (bOk)
	{
		assert(sValue.GetLength() >= 1);
		bOk = not iswspace(sValue.GetAt(sValue.GetLength() - 1));
		if (not bOk)
			sMessage = "Incorrect " + EntityToString(nEntity) + " name (" + sValue +
				   ") : must not contain any trailing space chararacters";
	}

	return bOk;
}

boolean KWClass::CheckLabelWithMessage(const ALString& sValue, int nEntity, ALString& sMessage)
{
	boolean bOk = true;

	require(0 <= nEntity and nEntity < Unknown);

	// Test de la taille maximale
	bOk = sValue.GetLength() <= 100000;
	if (not bOk)
		sMessage = "Incorrect " + EntityToString(nEntity) + " label : length > 100000\n\t<" + sValue.Left(100) +
			   "...>";

	// Test de caractere fin de ligne
	if (bOk)
	{
		bOk = sValue.Find('\n') == -1;
		if (not bOk)
			sMessage = "Incorrect " + EntityToString(nEntity) +
				   " label : must not contain end-of-line chararacters\t(" + sValue + ")";
	}

	return bOk;
}

ALString KWClass::BuildUTF8SubString(const ALString sValue)
{
	ALString sUTF8SubString;
	int i;
	int nLength;
	int nValidUTF8CharNumber;

	// Lit la fin de chaine pour eliminer les fins de sequence utf8
	nLength = sValue.GetLength();
	i = nLength - 1;
	while (i > 0)
	{
		nValidUTF8CharNumber = TextService::GetValidUTF8CharLengthAt(sValue, i);
		if (nValidUTF8CharNumber > 0)
		{
			nLength = i + nValidUTF8CharNumber;
			break;
		}
		i--;
	}

	// Recopie le resultat
	sUTF8SubString = sValue.Left(nLength);
	return sUTF8SubString;
}

boolean KWClass::IsStringIdentifier(const ALString& sValue)
{
	boolean bOk;
	int i;
	char c;

	// Test si la valeur est compose de lettres, chiffres ou '_'
	bOk = (sValue.GetLength() > 0);
	if (bOk)
	{
		c = sValue.GetAt(0);
		bOk = ('a' <= c and c <= 'z') or ('A' <= c and c <= 'Z') or c == '_';
		for (i = 1; i < sValue.GetLength(); i++)
		{
			c = sValue.GetAt(i);
			bOk = bOk and (('a' <= c and c <= 'z') or ('A' <= c and c <= 'Z') or ('0' <= c and c <= '9') or
				       c == '_');
		}
	}
	return bOk;
}

ALString KWClass::GetExternalName(const ALString& sValue)
{
	ALString sResult;
	int nLength;
	char c;
	int i;

	// Les variables ayant une syntaxe d'identifiant sont laissee telle quelles
	if (IsStringIdentifier(sValue))
		return sValue;
	// Sinon, on entoure de back-quotes, en doublant les eventuels back-quotes internes
	else
	{
		sResult = '`';
		nLength = sValue.GetLength();
		for (i = 0; i < nLength; i++)
		{
			c = sValue.GetAt(i);
			if (c == '`')
				sResult += '`';
			sResult += c;
		}
		sResult += '`';
		return sResult;
	}
}

ALString KWClass::GetExternalSymbolConstant(const Symbol& sValue)
{
	int nLength;
	ALString sResult;
	char c;
	int i;

	// Ajout de doubles quotes autour de la chaine, et de '\' devant
	// les doubles quotes et les backslash
	sResult = '"';
	nLength = sValue.GetLength();
	for (i = 0; i < nLength; i++)
	{
		c = sValue.GetAt(i);
		if (c == '"')
			sResult += '"';
		sResult += c;
	}
	sResult += '"';
	return sResult;
}

ALString KWClass::GetExternalContinuousConstant(Continuous cValue)
{
	ALString sResult;

	// Cas special de la valeur manquante
	if (cValue == KWContinuous::GetMissingValue())
		sResult = "#Missing";
	else
		sResult = KWContinuous::ContinuousToString(cValue);
	return sResult;
}

KWClass* KWClass::CreateClass(const ALString& sClassName, int nKeySize, int nSymbolNumber, int nContinuousNumber,
			      int nDateNumber, int nTimeNumber, int nTimestampNumber, int nTimestampTZNumber,
			      int nTextNumber, int nTextListNumber, int nObjectNumber, int nObjectArrayNumber,
			      int nStructureNumber, boolean bCreateAttributeBlocks, KWClass* attributeClass)
{
	KWClass* kwcClass;
	int i;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	ALString sDensePrefix = "Att";
	ALString sSparsePrefix = "SAtt";
	ALString sPrefix;

	require(nKeySize >= 0);
	require(nSymbolNumber >= 0);
	require(nContinuousNumber >= 0);
	require(nDateNumber >= 0);
	require(nTimeNumber >= 0);
	require(nTimestampNumber >= 0);
	require(nTimestampTZNumber >= 0);
	require(nTextNumber >= 0);
	require(nObjectNumber >= 0);
	require(nObjectArrayNumber >= 0);
	require(nStructureNumber >= 0);

	// Creation de la classe
	kwcClass = new KWClass;
	kwcClass->SetName(sClassName);
	kwcClass->SetLabel("Label of " + sClassName);

	// Creation des attributs de la cle
	kwcClass->SetKeyAttributeNumber(nKeySize);
	for (i = 0; i < nKeySize; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + "Key" + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::Symbol);
		kwcClass->InsertAttribute(attribute);
		kwcClass->SetKeyAttributeNameAt(i, attribute->GetName());
	}

	// Creation des attributs Symbol
	for (i = 0; i < nSymbolNumber; i++)
	{
		attribute = new KWAttribute;
		sPrefix = sDensePrefix;
		if (bCreateAttributeBlocks and i >= nSymbolNumber / 2)
			sPrefix = sSparsePrefix;
		attribute->SetName(sPrefix + KWType::ToString(KWType::Symbol).GetAt(0) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::Symbol);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation d'un bloc pour une moitie des attributs Symbol
	if (bCreateAttributeBlocks and nSymbolNumber / 2 > 0)
	{
		// Ajout de meta-donnees aux attributs du bloc
		for (i = nSymbolNumber / 2; i < nSymbolNumber; i++)
		{
			attribute = kwcClass->LookupAttribute(
			    sSparsePrefix + KWType::ToString(KWType::Symbol).GetAt(0) + IntToString(i + 1));
			attribute->GetMetaData()->SetStringValueAt(KWAttributeBlock::GetAttributeKeyMetaDataKey(),
								   IntToString(i + 1));
		}

		// Creation du bloc
		attributeBlock = kwcClass->CreateAttributeBlock(
		    sSparsePrefix + KWType::ToString(KWType::SymbolValueBlock),
		    kwcClass->LookupAttribute(sSparsePrefix + KWType::ToString(KWType::Symbol).GetAt(0) +
					      IntToString(1 + nSymbolNumber / 2)),
		    kwcClass->LookupAttribute(sSparsePrefix + KWType::ToString(KWType::Symbol).GetAt(0) +
					      IntToString(nSymbolNumber)));
	}

	// Creation des attributs Continuous
	for (i = 0; i < nContinuousNumber; i++)
	{
		attribute = new KWAttribute;
		sPrefix = sDensePrefix;
		if (bCreateAttributeBlocks and i >= nContinuousNumber / 2)
			sPrefix = sSparsePrefix;
		attribute->SetName(sPrefix + KWType::ToString(KWType::Continuous).GetAt(0) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::Continuous);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation d'un bloc pour une moitie des attributs Continuous
	if (bCreateAttributeBlocks and nContinuousNumber / 2 > 0)
	{
		// Ajout de mata-donnees aux attributs du bloc
		for (i = nContinuousNumber / 2; i < nContinuousNumber; i++)
		{
			attribute = kwcClass->LookupAttribute(
			    sSparsePrefix + KWType::ToString(KWType::Continuous).GetAt(0) + IntToString(i + 1));
			attribute->GetMetaData()->SetStringValueAt(KWAttributeBlock::GetAttributeKeyMetaDataKey(),
								   IntToString(i + 1));
		}

		// Creation du bloc
		attributeBlock = kwcClass->CreateAttributeBlock(
		    sSparsePrefix + KWType::ToString(KWType::ContinuousValueBlock),
		    kwcClass->LookupAttribute(sSparsePrefix + KWType::ToString(KWType::Continuous).GetAt(0) +
					      IntToString(1 + nContinuousNumber / 2)),
		    kwcClass->LookupAttribute(sSparsePrefix + KWType::ToString(KWType::Continuous).GetAt(0) +
					      IntToString(nContinuousNumber)));
		attributeBlock->GetMetaData()->SetDoubleValueAt(
		    KWAttributeBlock::GetDefaultValueMetaDataKey(KWType::Continuous), 0);
	}

	// Creation des attributs Date
	for (i = 0; i < nDateNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + KWType::ToString(KWType::Date) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::Date);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation des attributs Time
	for (i = 0; i < nTimeNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + KWType::ToString(KWType::Time) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::Time);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation des attributs Timestamp
	for (i = 0; i < nTimestampNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + KWType::ToString(KWType::Timestamp) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::Timestamp);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation des attributs TimestampTZ
	for (i = 0; i < nTimestampTZNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + KWType::ToString(KWType::TimestampTZ) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::TimestampTZ);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation des attributs Text
	for (i = 0; i < nTextNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + KWType::ToString(KWType::Text) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::Text);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation des attributs TextList
	for (i = 0; i < nTextListNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + KWType::ToString(KWType::TextList) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::TextList);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation des attributs Object
	for (i = 0; i < nObjectNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + KWType::ToString(KWType::Object) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::Object);
		attribute->SetClass(attributeClass);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation des attributs ObjectArray
	for (i = 0; i < nObjectArrayNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + KWType::ToString(KWType::ObjectArray) + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::ObjectArray);
		attribute->SetClass(attributeClass);
		kwcClass->InsertAttribute(attribute);
	}

	// Creation des attributs Structure
	for (i = 0; i < nStructureNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(sPrefix + "Structure" + IntToString(i + 1));
		attribute->SetLabel("Label of " + attribute->GetName());
		attribute->SetType(KWType::Structure);
		attribute->SetStructureName("TestStructureName");
		kwcClass->InsertAttribute(attribute);
	}
	return kwcClass;
}

void KWClass::TestAttributeBrowsings(boolean bList, boolean bInverseList, boolean bUsed, boolean bLoaded,
				     boolean bTypes)
{
	int i;
	KWAttribute* attribute;

	// Parcours de type liste
	if (bList)
	{
		AddMessage("List iteration");
		attribute = GetHeadAttribute();
		while (attribute != NULL)
		{
			cout << "\t" << *attribute << "\n";
			GetNextAttribute(attribute);
		}
	}
	if (bInverseList)
	{
		AddMessage("Inverse list iteration");
		attribute = GetTailAttribute();
		while (attribute != NULL)
		{
			cout << "\t" << *attribute << "\n";
			GetPrevAttribute(attribute);
		}
	}

	// Parcours de type indexe
	if (not IsIndexed())
		AddMessage("Dictionary iteration");
	else
	{
		if (bUsed)
		{
			AddMessage("Used variables");
			for (i = 0; i < GetUsedAttributeNumber(); i++)
			{
				attribute = GetUsedAttributeAt(i);
				cout << "\t" << *attribute << "\n";
			}
		}
		if (bLoaded)
		{
			AddMessage("Loaded variables");
			for (i = 0; i < GetLoadedAttributeNumber(); i++)
			{
				attribute = GetLoadedAttributeAt(i);
				cout << "\t" << *attribute << "\n";
			}
		}
		if (bTypes)
		{
			AddMessage("Dense categorical variables");
			for (i = 0; i < GetLoadedDenseSymbolAttributeNumber(); i++)
			{
				attribute = GetLoadedDenseSymbolAttributeAt(i);
				cout << "\t" << *attribute << "\n";
			}
			AddMessage("Text variables");
			for (i = 0; i < GetLoadedTextAttributeNumber(); i++)
			{
				attribute = GetLoadedTextAttributeAt(i);
				cout << "\t" << *attribute << "\n";
			}
			AddMessage("TextList variables");
			for (i = 0; i < GetLoadedTextListAttributeNumber(); i++)
			{
				attribute = GetLoadedTextListAttributeAt(i);
				cout << "\t" << *attribute << "\n";
			}
			AddMessage("Dense relation variables");
			for (i = 0; i < GetLoadedRelationAttributeNumber(); i++)
			{
				attribute = GetLoadedRelationAttributeAt(i);
				cout << "\t" << *attribute << "\n";
			}
		}
	}
}

void KWClass::Test()
{
	KWClass* testClass;
	KWClass* testClassClone;
	KWClass* attributeClass;
	KWAttribute* attribute;
	KWAttribute* attributeNew;
	KWAttribute* attributeToRemove;
	int nIndex;

	// Creation d'une classe de test
	attributeClass = CreateClass("AttributeClass", 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, true, NULL);
	testClass = CreateClass("TestClass", 1, 3, 3, 1, 1, 1, 1, 1, 0, 2, 2, 0, true, attributeClass);
	testClass->SetRoot(true);
	KWClassDomain::GetCurrentDomain()->InsertClass(attributeClass);
	KWClassDomain::GetCurrentDomain()->InsertClass(testClass);

	// Check
	attributeClass->Check();
	testClass->Check();

	// Affichage
	cout << *attributeClass << endl;
	cout << *testClass << endl;

	// Test parcours
	testClass->Compile();
	testClass->TestAttributeBrowsings(true, true, true, true, true);

	// Modification des caracteristiques des attributs
	cout << "\n\nModification of variables specs\n\n";
	attribute = testClass->GetHeadAttribute();
	nIndex = 0;
	while (attribute != NULL)
	{
		nIndex++;
		attribute->SetUsed(nIndex % 3 != 0);
		attribute->SetLoaded(nIndex % 2 != 0);
		testClass->GetNextAttribute(attribute);
	}
	testClass->Compile();
	testClass->TestAttributeBrowsings(true, true, true, true, true);
	cout << *testClass << endl;

	// Duplication et selection des attributs charges en memoire
	cout << "\n\nDuplication and selection of loaded variables\n\n";
	testClassClone = testClass->Clone();
	testClassClone->SetName(testClassClone->GetName() + "Clone");
	testClassClone->SetLabel(testClassClone->GetLabel() + " (Clone)");
	KWClassDomain::GetCurrentDomain()->InsertClass(testClassClone);
	attribute = testClassClone->GetHeadAttribute();
	while (attribute != NULL)
	{
		attributeToRemove = NULL;
		if (not attribute->GetUsed() or not attribute->GetLoaded())
			attributeToRemove = attribute;
		testClassClone->GetNextAttribute(attribute);
		if (attributeToRemove != NULL)
			testClassClone->DeleteAttribute(attributeToRemove->GetName());
	}
	cout << *testClassClone << endl;

	// Insertion autout des extremites
	cout << "\n\nInsertion around the borders\n\n";
	// Avant debut
	attribute = testClassClone->GetHeadAttribute();
	testClassClone->RenameAttribute(attribute, "FirstAtt");
	attributeNew = attribute->Clone();
	attributeNew->SetName(testClassClone->BuildAttributeName(attributeNew->GetName()));
	attributeNew->SetType(KWType::Continuous);
	attributeNew->SetClass(NULL);
	testClassClone->InsertAttributeBefore(attributeNew, attribute);
	// Apres debut
	attributeNew = attribute->Clone();
	attributeNew->SetName(testClassClone->BuildAttributeName(attributeNew->GetName()));
	attributeNew->SetType(KWType::Symbol);
	attributeNew->SetClass(NULL);
	testClassClone->InsertAttributeAfter(attributeNew, attribute);
	// Avant fin
	attribute = testClassClone->GetTailAttribute();
	testClassClone->RenameAttribute(attribute, "LastAtt");
	attributeNew = attribute->Clone();
	attributeNew->SetName(testClassClone->BuildAttributeName(attributeNew->GetName()));
	attributeNew->SetType(KWType::Continuous);
	attributeNew->SetClass(NULL);
	testClassClone->InsertAttributeBefore(attributeNew, attribute);
	// Apres fin
	attributeNew = attribute->Clone();
	attributeNew->SetName(testClassClone->BuildAttributeName(attributeNew->GetName()));
	attributeNew->SetType(KWType::Symbol);
	attributeNew->SetClass(NULL);
	testClassClone->InsertAttributeAfter(attributeNew, attribute);
	// Restitution de la variable cle si necessaire
	if (testClassClone->GetKeyAttributeNumber() > 0 and
	    testClassClone->LookupAttribute(testClassClone->GetKeyAttributeNameAt(0)) == NULL)
	{
		attributeNew = new KWAttribute;
		attributeNew->SetName(testClassClone->GetKeyAttributeNameAt(0));
		attributeNew->SetType(KWType::Symbol);
		testClassClone->InsertAttributeBefore(attributeNew, testClassClone->GetHeadAttribute());
	}
	// Affichage
	cout << *testClassClone << endl;
	testClassClone->Compile();
	testClassClone->TestAttributeBrowsings(true, false, false, true, false);
	cout << endl;

	// Destruction de toutes les classes enregistrees
	KWClassDomain::DeleteAllDomains();
}

KWDataItem* KWClass::LookupDataItem(const ALString& sDataItemName) const
{
	KWDataItem* dataItem;
	dataItem = cast(KWDataItem*, odAttributes.Lookup(sDataItemName));
	if (dataItem == NULL)
		dataItem = cast(KWDataItem*, odAttributeBlocks.Lookup(sDataItemName));
	return dataItem;
}

void KWClass::InitializeAllRandomRuleParameters()
{
	KWAttribute* attribute;
	KWDerivationRule* rule;
	int nRuleRankInAttribute;

	require(IsCompiled());

	// Parcours des attributs
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// Pour chaque attribut, on initialise les regles de type Random potentiellement utilisees
		nRuleRankInAttribute = 1;
		rule = attribute->GetDerivationRule();
		if (rule != NULL)
			InitializeRandomRuleParameters(rule, attribute->GetName(), nRuleRankInAttribute);
		GetNextAttribute(attribute);
	}
}

void KWClass::InitializeRandomRuleParameters(KWDerivationRule* rule, const ALString& sAttributeName,
					     int& nRuleRankInAttribute)
{
	const KWDRRandom refRandomRule;
	KWDRRandom* randomRule;
	KWDerivationRuleOperand* operand;
	int i;

	require(rule != NULL);
	require(sAttributeName != "");
	require(nRuleRankInAttribute > 0);

	// Parametrage de la regle si c'est une regle de type Random
	if (rule->GetName() == refRandomRule.GetName())
	{
		randomRule = cast(KWDRRandom*, rule);
		randomRule->InitializeRandomParameters(GetName(), sAttributeName, nRuleRankInAttribute);
		nRuleRankInAttribute++;
	}
	// Propagation aux operandes sinon
	else
	{
		for (i = 0; i < rule->GetOperandNumber(); i++)
		{
			operand = rule->GetOperandAt(i);
			if (operand->GetOrigin() == KWDerivationRuleOperand::OriginRule)
				InitializeRandomRuleParameters(operand->GetDerivationRule(), sAttributeName,
							       nRuleRankInAttribute);
		}
	}
}

boolean KWClass::CheckClassComposition(KWAttribute* parentAttribute, NumericKeyDictionary* nkdComponentClasses) const
{
	boolean bOk = true;
	KWAttribute* attribute;
	KWClass* parentClass;
	KWClass* attributeClass;

	require(nkdComponentClasses != NULL);

	// Recherche de la classe parent a partir de l'attribut parent
	parentClass = NULL;
	if (parentAttribute != NULL)
		parentClass = parentAttribute->GetParentClass();

	// Ajout de la classe courante dans le dictionnaire des classes utilisees
	nkdComponentClasses->SetAt(this, (Object*)this);

	// Parcours des attributs de la classe
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// Traitement des attributs de composition
		if (KWType::IsRelation(attribute->GetType()) and not attribute->GetReference() and
		    attribute->GetAnyDerivationRule() == NULL and attribute->GetClass() != NULL)
		{
			attributeClass = attribute->GetClass();

			// Test si classe deja utilisee
			if (nkdComponentClasses->Lookup(attributeClass) == attributeClass)
			{
				AddError("Existing composition cycle caused by the recursive use of dictionary " +
					 attributeClass->GetName() + " by variable " + attribute->GetName());
				bOk = false;
				break;
			}
			// Propagation du test si ok
			else
			{
				bOk = attributeClass->CheckClassComposition(attribute, nkdComponentClasses);
				if (not bOk)
					break;
			}

			// La cle de la classe utilisee doit etre strictement plus longue que
			// celle de la classe utilisante dans le cas d'un lien de composition multiple a trois niveau
			// (ou plus)
			assert(parentClass == NULL or parentClass->GetKeyAttributeNumber() <= GetKeyAttributeNumber());
			assert(GetKeyAttributeNumber() <= attributeClass->GetKeyAttributeNumber());
			if (parentClass != NULL and parentAttribute->GetType() == KWType::ObjectArray and
			    parentClass->GetKeyAttributeNumber() == GetKeyAttributeNumber())
			{
				AddError("The length of a key in a dictionary used as a Table, having a sub-" +
					 KWType::ToString(attribute->GetType()) +
					 " in its composition, must be strictly greater than that of its parent "
					 "Dictionary;\n " +
					 "in dictionary " + parentClass->GetName() + " (key length=" +
					 IntToString(parentClass->GetKeyAttributeNumber()) + "), dictionary " +
					 GetName() + "(key length = " + IntToString(GetKeyAttributeNumber()) +
					 ") is used as a Table in variable " + parentAttribute->GetName() +
					 ", and uses dictionary " + attributeClass->GetName() + " with variable " +
					 attribute->GetName() + " as a sub-" + KWType::ToString(attribute->GetType()) +
					 " in its composition.");
				bOk = false;
			}
		}

		// Attribut suivant
		GetNextAttribute(attribute);
	}

	// Supression de la classe courante dans le dictionnaire des classes utilisees
	nkdComponentClasses->RemoveKey(this);
	return bOk;
}

boolean KWClass::CheckTypeAtLoadIndex(KWLoadIndex liIndex, int nType) const
{
	boolean bOk;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;

	bOk = liIndex.IsValid();
	if (bOk)
		bOk = 0 <= liIndex.GetDenseIndex() and liIndex.GetDenseIndex() < oaLoadedDataItems.GetSize();
	if (bOk)
	{
		dataItem = cast(KWDataItem*, oaLoadedDataItems.GetAt(liIndex.GetDenseIndex()));

		// Acces au DataItem dans le cas dense
		if (liIndex.IsDense())
		{
			// Cas d'un attribut
			if (dataItem->IsAttribute())
			{
				attribute = cast(KWAttribute*, dataItem);
				bOk = attribute->GetType() == nType;
			}
			// Cas d'un bloc d'attributs
			else
			{
				attributeBlock = cast(KWAttributeBlock*, dataItem);
				bOk = attributeBlock->GetBlockType() == nType;
			}
		}
		// Acces au bloc dans le cas sparse
		else
		{
			bOk = KWType::GetValueBlockType(nType) != KWType::Unknown;
			if (bOk)
				bOk = dataItem->IsAttributeBlock();
			if (bOk)
			{
				attributeBlock = cast(KWAttributeBlock*, dataItem);
				bOk = attributeBlock->GetType() == nType;
			}
		}
	}
	return bOk;
}

void KWClass::ReadNotLoadedMetaData()
{
	KWAttribute* attribute;

	// Parcours des attributs de la classe
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		attribute->ReadNotLoadedMetaData();
		GetNextAttribute(attribute);
	}
}
