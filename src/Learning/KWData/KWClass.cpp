// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	bForceUnique = false;
	bIsUnique = false;
	bIsKeyBasedStorable = false;
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
	nFreshness++;
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
	nFreshness++;
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
	const boolean bTrace = false;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	int nIndex;
	int nInternalLoadIndex;
	int nAttributeBlock;
	int nAttribute;
	ALString sAttributeKeyMetaDataKey;
	Symbol sVarKey;
	int nVarKey;
	ObjectArray oaBlockLoadedAttributes;

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
	oaUnloadedOwnedRelationAttributes.SetSize(0);
	oaLoadedDataItems.SetSize(0);
	livKeyAttributeLoadIndexes.SetSize(0);

	// Reinitialisation des statistiques par type d'attributs utilises
	ivUsedAttributeNumbers.Initialize();
	ivUsedDenseAttributeNumbers.Initialize();
	ivUsedSparseAttributeNumbers.Initialize();

	// Calcul de la capacite a etre stocke, en verifiant la coherence des cle en mode non verbeux
	bIsKeyBasedStorable = CheckNativeComposition(true, false);

	// Il y a unicite dans le cas d'une classe racine, ou si l'unicite est forcee
	bIsUnique = bRoot or bForceUnique;

	// Indexage des tableaux d'attributs par parcours de la liste
	sAttributeKeyMetaDataKey = KWAttributeBlock::GetAttributeKeyMetaDataKey();
	nNativeAttributeBlockNumber = 0;
	nNativeAttributeNumber = 0;
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		assert(attribute->GetType() != KWType::Unknown);

		// Il y a unicite dans le cas d'utilisation d'attribut relation non calcule
		if (KWType::IsRelation(attribute->GetType()) and attribute->GetAnyDerivationRule() == NULL)
			bIsUnique = true;

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
			attributeBlock->ivLoadedAttributeIndexesBySparseIndex.SetSize(0);
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

		// Cas des attributs Object ou ObjectArray natifs ou crees par une regle, non utilises ni charges en memoire
		if (not attribute->GetLoaded())
		{
			if (KWType::IsRelation(attribute->GetType()))
			{
				if (not attribute->IsInBlock() and not attribute->GetReference())
					oaUnloadedOwnedRelationAttributes.Add(attribute);
			}
		}

		// Attribut suivant
		GetNextAttribute(attribute);
	}

	// Parcours des blocs d'attribut charges, pour les indexer sur la base d'un tri par VarKey
	for (nAttributeBlock = 0; nAttributeBlock < oaLoadedAttributeBlocks.GetSize(); nAttributeBlock++)
	{
		attributeBlock = cast(KWAttributeBlock*, oaLoadedAttributeBlocks.GetAt(nAttributeBlock));

		// Memorisation des attributs du bloc dans leur ordre initial, selon l'ordre dans la classe
		oaBlockLoadedAttributes.CopyFrom(&attributeBlock->oaLoadedAttributes);

		// Tri des attributs charges par VarKey, ce qui est necessaire pour ensuite
		// initialiser les IndexKeyBlock selon l'ordre requis
		attributeBlock->oaLoadedAttributes.SetCompareFunction(KWAttributeCompareVarKey);
		attributeBlock->oaLoadedAttributes.Sort();

		// Parcours des attributs charges du bloc selon l'ordre par VarKey pour specifier leur index sparse
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

		// Parcours des attributs charges du bloc selon l'ordre initial, pour memoriser l'association
		// entre index sparse et index de l'attribut selon l'ordre dans la classe
		attributeBlock->ivLoadedAttributeIndexesBySparseIndex.SetSize(oaBlockLoadedAttributes.GetSize());
		for (nAttribute = 0; nAttribute < oaBlockLoadedAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, oaBlockLoadedAttributes.GetAt(nAttribute));
			attributeBlock->ivLoadedAttributeIndexesBySparseIndex.SetAt(
			    attribute->liLoadIndex.GetSparseIndex(), nAttribute);
		}
	}

	// Calcul des index internes des attributs natifs a stocker
	nInternalLoadIndex = oaLoadedDataItems.GetSize();
	for (nIndex = 0; nIndex < oaUnloadedOwnedRelationAttributes.GetSize(); nIndex++)
	{
		attribute = cast(KWAttribute*, oaUnloadedOwnedRelationAttributes.GetAt(nIndex));
		attribute->liLoadIndex.SetDenseIndex(nInternalLoadIndex);
		nInternalLoadIndex++;
	}
	assert(nInternalLoadIndex == oaLoadedDataItems.GetSize() + oaUnloadedOwnedRelationAttributes.GetSize());

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

	// Affichage des tableaux d'attributs indexes
	if (bTrace)
	{
		cout << "Index dictionary\t" << GetName() << "\n";
		if (GetDomain() != NULL)
			cout << " Domain\t" << GetDomain()->GetName() << "\n";
		cout << " Root\t" << BooleanToString(GetRoot()) << "\n";
		cout << " ForceUnique\t" << BooleanToString(GetForceUnique()) << "\n";
		cout << " IsUnique\t" << BooleanToString(IsUnique()) << "\n";
		cout << " IsKeyBasedStorable\t" << BooleanToString(IsKeyBasedStorable()) << "\n";
		WriteAttributes("  Used attributes", &oaUsedAttributes, cout);
		WriteAttributes("  Loaded attributes", &oaLoadedAttributes, cout);
		WriteAttributes("  Loaded dense attributes", &oaLoadedDenseAttributes, cout);
		WriteAttributes("  Loaded dense Categorical attributes", &oaLoadedDenseSymbolAttributes, cout);
		WriteAttributes("  Loaded Text attributes attributes", &oaLoadedTextAttributes, cout);
		WriteAttributes("  Loaded TextList  attributes", &oaLoadedTextListAttributes, cout);
		WriteAttributes("  Loaded Relation attributes", &oaLoadedRelationAttributes, cout);
		WriteAttributes("  Unloaded native Relation attributes", &oaUnloadedOwnedRelationAttributes, cout);
	}
}

int KWClass::ComputeOverallNativeRelationAttributeNumber(boolean bIncludingReferences) const
{
	int nOverallNativeRelationAttributeNumber;
	ObjectDictionary odReferenceClasses;
	ObjectArray oaImpactedClasses;
	ObjectDictionary odAnalysedCreatedClasses;
	KWClass* kwcImpactedClass;
	KWClass* kwcRefClass;
	int nClass;
	KWAttribute* attribute;
	KWClass* kwcTargetClass;
	ObjectArray oaUsedClass;
	KWClass* kwcUsedClass;
	int nUsedClass;

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
					// Cas d'un attribut issue d'une regle de creation de table, pour rechercher
					// les classes referencees depuis les tables creees par des regles
					else if (bIncludingReferences and not attribute->GetReference())
					{
						assert(attribute->GetDerivationRule() != NULL);

						// Recherche de la classe cible
						kwcTargetClass = GetDomain()->LookupClass(
						    attribute->GetDerivationRule()->GetObjectClassName());
						assert(kwcTargetClass != NULL);

						// Analyse uniquement si la classe cible na pas deja ete analysees
						if (odAnalysedCreatedClasses.Lookup(kwcTargetClass->GetName()) == NULL)
						{
							// Memorisation de la classe cible
							odAnalysedCreatedClasses.SetAt(kwcTargetClass->GetName(),
										       kwcTargetClass);

							// Recherche de toutes les classe utilisees recursivement
							kwcTargetClass->BuildAllUsedClasses(&oaUsedClass);

							// Recherches des classes externes
							for (nUsedClass = 0; nUsedClass < oaUsedClass.GetSize();
							     nUsedClass++)
							{
								kwcUsedClass =
								    cast(KWClass*, oaUsedClass.GetAt(nUsedClass));

								// Ajout de la classe a analyser si elle ne l'a pas deja ete
								if (kwcUsedClass->GetRoot())
								{
									if (odReferenceClasses.Lookup(
										kwcUsedClass->GetName()) == NULL)
									{
										nOverallNativeRelationAttributeNumber++;

										// Memorisation de la classe externe pour ne pas faire l'analyse plusieurs fois
										odReferenceClasses.SetAt(
										    kwcUsedClass->GetName(),
										    kwcUsedClass);
										oaImpactedClasses.Add(kwcUsedClass);
									}
								}
							}
						}
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

							// Memorisation de la classe externe pour ne pas faire l'analyse plusieurs fois
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
	ObjectArray oaAllUsedClasses;
	NumericKeyDictionary nkdAllUsedAttributes;
	NumericKeyDictionary nkdAllUsedClasses;
	NumericKeyDictionary nkdAllLoadedClasses;
	NumericKeyDictionary nkdAllNativeClasses;
	KWClass* kwcUsedClass;
	ObjectArray oaUnusedAttributes;
	ObjectArray oaUnusedClasses;
	KWAttribute* attribute;
	KWAttribute* referenceAttribute;
	KWClass* referenceClass;
	int nClass;
	int nAttribute;

	require(Check());
	require(IsCompiled());

	// Recherche de toutes les classe utilisees recursivement
	BuildAllUsedClasses(&oaAllUsedClasses);

	// Dans le cas d'un domaine de reference, on recherche la liste des attributs de reference a conserver
	if (referenceDomain != NULL)
	{
		// Parcours des classes utilises pour rechercher leur pendant dans le domaine de reference
		for (nClass = 0; nClass < oaAllUsedClasses.GetSize(); nClass++)
		{
			kwcUsedClass = cast(KWClass*, oaAllUsedClasses.GetAt(nClass));

			// Parcours des attributs pour identifier les attributs a detruire
			attribute = kwcUsedClass->GetHeadAttribute();
			while (attribute != NULL)
			{
				// Recherche de l'attribut de reference correspondant potentiel
				referenceAttribute = NULL;
				referenceClass = referenceDomain->LookupClass(attribute->GetParentClass()->GetName());
				assert(referenceClass != NULL);
				referenceAttribute = referenceClass->LookupAttribute(attribute->GetName());

				// Memorisation de l'attribut s'il existe dans le domaine de reference
				if (referenceAttribute != NULL)
					nkdAllUsedAttributes.SetAt(attribute, attribute);

				// Attribut suivant
				kwcUsedClass->GetNextAttribute(attribute);
			}
		}
	}

	// Collecte des attributs utilises recursivement
	BuildAllUsedAttributes(&nkdAllUsedAttributes, &nkdAllUsedClasses, &nkdAllLoadedClasses, &nkdAllNativeClasses);

	// Parcours de toutes les classes utilisees pour identifier les attributs non utilises
	for (nClass = 0; nClass < oaAllUsedClasses.GetSize(); nClass++)
	{
		kwcUsedClass = cast(KWClass*, oaAllUsedClasses.GetAt(nClass));

		// On ne nettoie que les classes utilisees
		if (nkdAllUsedClasses.Lookup(kwcUsedClass) != NULL)
		{
			// Parcours des attributs pour identifier les attributs a detruire
			attribute = kwcUsedClass->GetHeadAttribute();
			while (attribute != NULL)
			{
				// Attribut a detruire s'il n'est pas utilise, et s'il est calcule
				// On garde les attributs natifs, meme s'ils ne sont pas utilises, pour ne pas
				// generer de warning lors de la lecture des bases
				if (not attribute->IsNative())
				{
					// Attribut a detruire s'il est inutilise
					if (nkdAllUsedAttributes.Lookup(attribute) == NULL)
						oaUnusedAttributes.Add(attribute);
				}

				// Attribut suivant
				kwcUsedClass->GetNextAttribute(attribute);
			}
		}
		// On supprime les classes non utilisees, dont l'integrite structurelle peut etre compromise
		// (manque de leur champs cles, reference a des attributs supprimes...)
		else
			oaUnusedClasses.Add(kwcUsedClass);
	}

	// Prise en compte des attributs a detruire
	if (oaUnusedAttributes.GetSize() > 0 or oaUnusedClasses.GetSize() > 0)
	{
		// Destruction des attributs
		for (nAttribute = 0; nAttribute < oaUnusedAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, oaUnusedAttributes.GetAt(nAttribute));
			attribute->GetParentClass()->DeleteAttribute(attribute->GetName());
		}

		// Mise a jour des fraicheurs des classes pour forcer la compilation
		// En effet, une destruction d'attributs sur une classe peut avoir des
		// implication sur une autre classe du domaine
		for (nClass = 0; nClass < oaAllUsedClasses.GetSize(); nClass++)
		{
			kwcUsedClass = cast(KWClass*, oaAllUsedClasses.GetAt(nClass));
			kwcUsedClass->UpdateFreshness();
		}

		// Destruction des classes non utilisees
		for (nClass = 0; nClass < oaUnusedClasses.GetSize(); nClass++)
		{
			kwcUsedClass = cast(KWClass*, oaUnusedClasses.GetAt(nClass));
			GetDomain()->DeleteClass(kwcUsedClass->GetName());
		}
	}
	ensure(Check());
}

void KWClass::BuildAllUsedAttributes(NumericKeyDictionary* nkdAllUsedAttributes,
				     NumericKeyDictionary* nkdAllUsedClasses, NumericKeyDictionary* nkdAllLoadedClasses,
				     NumericKeyDictionary* nkdAllNativeClasses) const
{
	const boolean bTrace = false;
	ObjectArray oaAllLoadedClasses;
	ObjectArray oaAllUsedClasses;
	ObjectArray oaAllNativeClasses;
	ObjectArray oaAllUsedAttributes;
	KWClass* kwcCurrentClass;
	KWAttribute* attribute;
	KWDerivationRule* rule;
	int nClass;
	int nAttribute;
	int i;

	require(nkdAllUsedAttributes != NULL);
	require(nkdAllUsedClasses != NULL);
	require(nkdAllLoadedClasses != NULL);
	require(nkdAllNativeClasses != NULL);
	require(nkdAllUsedClasses->GetCount() == 0);
	require(nkdAllLoadedClasses->GetCount() == 0);
	require(nkdAllNativeClasses->GetCount() == 0);

	// Calcul de l'ensemble des classes chargees (Loaded) recursivement par les attributs de la classe physique
	BuildAllLoadedClasses(&oaAllLoadedClasses);
	for (nClass = 0; nClass < oaAllLoadedClasses.GetSize(); nClass++)
	{
		kwcCurrentClass = cast(KWClass*, oaAllLoadedClasses.GetAt(nClass));
		nkdAllLoadedClasses->SetAt(kwcCurrentClass, kwcCurrentClass);
	}

	// Parcours des classes chargees pour declarer tous leurs attribut charges comme utilises, en propageant
	// la detection des attributs charges indirectement via leur regle de derivation
	for (nClass = 0; nClass < oaAllLoadedClasses.GetSize(); nClass++)
	{
		kwcCurrentClass = cast(KWClass*, oaAllLoadedClasses.GetAt(nClass));

		// Recherche des attributs necessaires pour cette classe
		for (nAttribute = 0; nAttribute < kwcCurrentClass->GetLoadedAttributeNumber(); nAttribute++)
		{
			attribute = kwcCurrentClass->GetLoadedAttributeAt(nAttribute);

			// Pour la classe d'analyse ainsi que toutes les classes utilises indirectement, ajout de l'attribut lui-meme,
			// dont on a besoin pour parametrer correctement les regles de creation de table qui exploitent les attributs
			// utilises en sortie  pour en deduire les attributs utilises en entree des vues
			nkdAllUsedAttributes->SetAt(attribute, attribute);

			// Analyse de la regle de derivation
			// Dans le cas d'un bloc, il faut en effet la reanalyser pour chaque attribut du bloc
			// pour detecter les attributs utilises des blocs potentiellement en operande
			rule = attribute->GetAnyDerivationRule();
			if (rule != NULL)
				rule->BuildAllUsedAttributes(attribute, nkdAllUsedAttributes);
		}
	}

	// Finalisation de la collecte des attributs utilises via les regles de construction de table
	FinalizeBuildAllUsedAttributes(nkdAllUsedAttributes, nkdAllUsedClasses);

	// Calcul de l'ensemble des classes natives utilisees recursivement par les attributs de la classe courante
	BuildAllNativeClasses(&oaAllNativeClasses);
	for (nClass = 0; nClass < oaAllNativeClasses.GetSize(); nClass++)
	{
		kwcCurrentClass = cast(KWClass*, oaAllNativeClasses.GetAt(nClass));
		nkdAllNativeClasses->SetAt(kwcCurrentClass, kwcCurrentClass);
	}

	// Ajout de la cle necessaire pour la lecture des donnees multi-table, uniquement pour les
	// classe utilisee par le schema natif
	//
	// Il est possible qu'on integre des cles en trop dans des cas particuliers, comme celui d'un dictionnaire
	// natif d'une table secondaire no necessaire pour les donnees a traiter, mais utilise en sortie
	// d'une regle de construction de table.
	// Ce cas d'effet de bord est a priori rare, et sans grande consquence. Le code necessaire pour le traiter
	// proprement ajoute une complexite additionnelle, peu utile a prendre en compte
	nkdAllUsedClasses->ExportObjectArray(&oaAllUsedClasses);
	for (nClass = 0; nClass < oaAllUsedClasses.GetSize(); nClass++)
	{
		kwcCurrentClass = cast(KWClass*, oaAllUsedClasses.GetAt(nClass));

		// Ajout des cles pour les classes natives
		if (nkdAllNativeClasses->Lookup(kwcCurrentClass) != NULL)
		{
			for (i = 0; i < kwcCurrentClass->GetKeyAttributeNumber(); i++)
			{
				attribute = kwcCurrentClass->GetKeyAttributeAt(i);
				check(attribute);
				nkdAllUsedAttributes->SetAt(attribute, attribute);
			}
		}
	}

	// Affichage des classes et attributs necessaires
	if (bTrace)
	{
		//Entete
		cout << "BuildAllUsedAttributes\t" << GetName() << "\n";

		// Affichages des classes necessaires
		cout << "  Used classes\t" << nkdAllUsedClasses->GetCount() << endl;
		nkdAllUsedClasses->ExportObjectArray(&oaAllUsedClasses);
		oaAllUsedClasses.SetCompareFunction(KWClassCompareName);
		oaAllUsedClasses.Sort();
		for (nClass = 0; nClass < oaAllUsedClasses.GetSize(); nClass++)
		{
			kwcCurrentClass = cast(KWClass*, oaAllUsedClasses.GetAt(nClass));
			cout << "\t" << kwcCurrentClass->GetName();
			cout << "\t";
			if (nkdAllLoadedClasses->Lookup(kwcCurrentClass) != NULL)
				cout << "Loaded";
			cout << "\t";
			if (nkdAllNativeClasses->Lookup(kwcCurrentClass) != NULL)
				cout << "Native";
			cout << "\n";
		}

		// Affichage des attributs necessaires
		cout << "  Used attributes\t" << nkdAllUsedAttributes->GetCount() << endl;
		nkdAllUsedAttributes->ExportObjectArray(&oaAllUsedAttributes);
		oaAllUsedAttributes.SetCompareFunction(KWAttributeCompareClassAndAttributeName);
		oaAllUsedAttributes.Sort();
		for (nAttribute = 0; nAttribute < oaAllUsedAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, oaAllUsedAttributes.GetAt(nAttribute));
			cout << "\t" << attribute->GetParentClass()->GetName() << "\t" << attribute->GetName() << endl;
		}
	}
	ensure(nkdAllUsedClasses->GetCount() >= 1);
	ensure(nkdAllUsedClasses->Lookup(this) == this);
}

void KWClass::FinalizeBuildAllUsedAttributes(NumericKeyDictionary* nkdAllUsedAttributes,
					     NumericKeyDictionary* nkdAllUsedClasses) const
{
	const boolean bTrace = false;
	const boolean bTraceAttributes = false;
	ObjectArray oaAllUsedAttributes;
	KWAttribute* attribute;
	KWDerivationRule* rule;
	int nAttribute;
	boolean bContinue;
	int nStep;

	require(nkdAllUsedAttributes != NULL);
	require(nkdAllUsedClasses != NULL);

	// Trace de debut
	if (bTrace)
	{
		cout << "FinalizeBuildAllUsedAttributes"
		     << "\n";
		cout << "\tInitial attributes\t" << nkdAllUsedAttributes->GetCount() << endl;
		cout << "\tInitial classes\t" << nkdAllUsedClasses->GetCount() << endl;
	}

	// On enregistre prealablement la classe en cours ce qui est necessaire meme si
	// aucun attribut n'est selectionne initialement
	nkdAllUsedClasses->SetAt(this, (Object*)this);

	// Passe de propagation de la collecte des attributs utilises
	bContinue = true;
	nStep = 0;
	while (bContinue)
	{
		nStep++;

		// Export d'u dictionnaire d'attribut sous forme de tableau
		nkdAllUsedAttributes->ExportObjectArray(&oaAllUsedAttributes);

		// Tri par nom de classe et d'attribut
		// Ce tri n'est pas obligatoire, mais il n'est pas vraiment couteux en temps, et il assure la
		// reproductibilite de l'execution des passes d'analyse, ce qui est critique pour la mise au point
		oaAllUsedAttributes.SetCompareFunction(KWAttributeCompareClassAndAttributeName);
		oaAllUsedAttributes.Sort();

		// Parcours des attributs pour une nouvelle passe
		for (nAttribute = 0; nAttribute < oaAllUsedAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, oaAllUsedAttributes.GetAt(nAttribute));
			assert(attribute->GetParentClass() != NULL);
			assert(attribute->GetParentClass()->IsCompiled());

			// Memorisation de la classe contenant l'attribut
			nkdAllUsedClasses->SetAt(attribute->GetParentClass(), attribute->GetParentClass());

			// Memorisation de la classe de l'attribut dans le cas d'une relation
			if (KWType::IsRelation(attribute->GetType()))
				nkdAllUsedClasses->SetAt(attribute->GetClass(), attribute->GetClass());

			// Recalcul des attributs utilises uniquement pour les regles de creation de table
			rule = attribute->GetAnyDerivationRule();
			if (rule != NULL and KWType::IsRelation(rule->GetType()) and not rule->GetReference())
				rule->BuildAllUsedAttributes(attribute, nkdAllUsedAttributes);

			// Trace des attributs
			if (bTraceAttributes)
			{
				cout << "\t\t" << attribute->GetParentClass()->GetName() << "\t" << attribute->GetName()
				     << "\n";
			}
		}
		bContinue = nkdAllUsedAttributes->GetCount() > oaAllUsedAttributes.GetSize();

		// Trace
		if (bTrace)
			cout << "\tPass " << nStep << "\t" << nkdAllUsedAttributes->GetCount() << "\t"
			     << nkdAllUsedClasses->GetCount() << endl;
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
				assert(attribute->GetClass() != NULL);

				// Memorisation si necessaire
				if (nkdUsedClasses.Lookup(attribute->GetClass()) == NULL)
				{
					nkdUsedClasses.SetAt(attribute->GetClass(), attribute->GetClass());
					oaUsedClasses->Add(attribute->GetClass());
				}
			}

			// Attribut suivant
			kwcUsedClass->GetNextAttribute(attribute);
		}
	}
}

void KWClass::BuildAllNativeClasses(ObjectArray* oaNativeClasses) const
{
	NumericKeyDictionary nkdNativeClasses;
	NumericKeyDictionary nkdAnalyzedCreatedClasses;
	int nClass;
	KWClass* kwcNativeClass;
	KWAttribute* attribute;
	KWClass* kwcAttributeClass;
	ObjectArray oaAttributeUsedClasses;
	int nUsedClass;
	KWClass* kwcUsedClass;

	require(oaNativeClasses != NULL);
	require(IsCompiled());

	// Initialisation avec la classe courante
	oaNativeClasses->RemoveAll();
	oaNativeClasses->Add((Object*)this);
	nkdNativeClasses.SetAt(this, (Object*)this);

	// Parcours des classes utilises, en completant le tableau au fur et a mesure
	// pour lesquelles il faut propager la duplication
	for (nClass = 0; nClass < oaNativeClasses->GetSize(); nClass++)
	{
		kwcNativeClass = cast(KWClass*, oaNativeClasses->GetAt(nClass));

		// Attributs de type Object ou ObjectArray
		attribute = kwcNativeClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Si sttribut avec classe referencee
			if (KWType::IsRelation(attribute->GetType()))
			{
				kwcAttributeClass = attribute->GetClass();
				assert(kwcAttributeClass != NULL);

				// Cas d'un attribut natif, a memoriser
				if (attribute->GetAnyDerivationRule() == NULL)
				{
					assert(attribute->GetAttributeBlock() == NULL);

					// Memorisation si necessaire
					if (nkdNativeClasses.Lookup(kwcAttributeClass) == NULL)
					{
						nkdNativeClasses.SetAt(kwcAttributeClass, kwcAttributeClass);
						oaNativeClasses->Add(kwcAttributeClass);
					}
				}
				// Cas d'un attribut issue d'une regle de creation de table, pour rechercher
				// les classes referencees depuis les tables creees par des regles
				else if (not attribute->GetReference())
				{
					assert(attribute->GetAnyDerivationRule() != NULL);

					// Analyse uniquement si la classe de l'attribut na pas deja ete analysees
					if (nkdAnalyzedCreatedClasses.Lookup(kwcAttributeClass) == NULL)
					{
						// Recherche de toutes les classe utilisee recursivement
						kwcAttributeClass->BuildAllUsedClasses(&oaAttributeUsedClasses);

						// Recherches des classes externes
						for (nUsedClass = 0; nUsedClass < oaAttributeUsedClasses.GetSize();
						     nUsedClass++)
						{
							kwcUsedClass =
							    cast(KWClass*, oaAttributeUsedClasses.GetAt(nUsedClass));

							// Memorisation de la classe analysee
							nkdAnalyzedCreatedClasses.SetAt(kwcAttributeClass,
											kwcAttributeClass);

							// Memorisation si classe racine
							if (kwcUsedClass->GetRoot())
							{
								// Memorisation si necessaire
								if (nkdNativeClasses.Lookup(kwcUsedClass) == NULL)
								{
									nkdNativeClasses.SetAt(kwcUsedClass,
											       kwcUsedClass);
									oaNativeClasses->Add(kwcUsedClass);
								}
							}
						}
					}
				}
				// Cas d'un attribut natif reference (avec regle de derivation predefinie), a garder
				else if (attribute->GetAnyDerivationRule()->GetName() ==
					 KWDerivationRule::GetReferenceRuleName())
				{
					// Memorisation si necessaire
					if (nkdNativeClasses.Lookup(kwcAttributeClass) == NULL)
					{
						nkdNativeClasses.SetAt(kwcAttributeClass, kwcAttributeClass);
						oaNativeClasses->Add(kwcAttributeClass);
					}
				}
			}

			// Attribut suivant
			kwcNativeClass->GetNextAttribute(attribute);
		}
	}
}

void KWClass::BuildAllLoadedClasses(ObjectArray* oaLoadedClasses) const
{
	NumericKeyDictionary nkdLoadedClasses;
	int nClass;
	KWClass* kwcLoadedClass;
	KWAttribute* attribute;
	int i;

	require(oaLoadedClasses != NULL);
	require(IsCompiled());

	// Initialisation avec la classe courante
	oaLoadedClasses->RemoveAll();
	oaLoadedClasses->Add((Object*)this);
	nkdLoadedClasses.SetAt(this, (Object*)this);

	// Parcours des classes utilises, en completant le tableau au fur et a mesure
	// pour lesquelles il faut propager la duplication
	for (nClass = 0; nClass < oaLoadedClasses->GetSize(); nClass++)
	{
		kwcLoadedClass = cast(KWClass*, oaLoadedClasses->GetAt(nClass));

		// Attributs de type relation
		for (i = 0; i < kwcLoadedClass->GetLoadedRelationAttributeNumber(); i++)
		{
			attribute = kwcLoadedClass->GetLoadedRelationAttributeAt(i);
			assert(attribute->GetClass() != NULL);

			// Memorisation si necessaire
			if (nkdLoadedClasses.Lookup(attribute->GetClass()) == NULL)
			{
				nkdLoadedClasses.SetAt(attribute->GetClass(), attribute->GetClass());
				oaLoadedClasses->Add(attribute->GetClass());
			}
		}

		// Bloc d'attributs, portentiellement de type relation
		for (i = 0; i < kwcLoadedClass->GetLoadedAttributeBlockNumber(); i++)
		{
			attribute = kwcLoadedClass->GetLoadedAttributeBlockAt(i)->GetFirstAttribute();

			// Memorisation si necessaire
			if (KWType::IsRelation(attribute->GetType()) and
			    nkdLoadedClasses.Lookup(attribute->GetClass()) == NULL)
			{
				assert(attribute->GetClass() != NULL);
				nkdLoadedClasses.SetAt(attribute->GetClass(), attribute->GetClass());
				oaLoadedClasses->Add(attribute->GetClass());
			}
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
		// On ne n'exporte que les attributs natifs
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

void KWClass::ExportStoredFieldNames(StringVector* svStoredFieldNames, boolean bDenseOutputFormat) const
{
	int i;
	int j;
	int nDenseIndex;
	int nStartSize;
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

			// On ne gere que les attributs stockables
			if (KWType::IsStored(attribute->GetType()))
				svStoredFieldNames->Add(attribute->GetName());
		}
		// Cas d'un bloc d'attributs
		else
		{
			attributeBlock = cast(KWAttributeBlock*, dataItem);

			// On ne gere que les blocs attributs stockables
			if (KWType::IsStored(attributeBlock->GetType()))
			{
				// On garde le nom du bloc dans le cas standard
				if (not bDenseOutputFormat)
					svStoredFieldNames->Add(attributeBlock->GetName());
				// On prend tous les attributs du bloc en cas de format dense
				else
				{
					// On prevoit la place necessaire pour les attributs du bloc
					nStartSize = svStoredFieldNames->GetSize();
					svStoredFieldNames->SetSize(nStartSize +
								    attributeBlock->GetLoadedAttributeNumber());

					// On prend les attributs du bloc, selon l'ordre initial des attribut dans la classe
					for (j = 0; j < attributeBlock->GetLoadedAttributeNumber(); j++)
					{
						// On passe de l'index dans le bloc sparse a l'index dense selon l'ordre initial des attributs dans la classe
						nDenseIndex = attributeBlock->GetLoadedAttributeIndexAtSparseIndex(j);
						svStoredFieldNames->SetAt(
						    nStartSize + nDenseIndex,
						    attributeBlock->GetLoadedAttributeAt(j)->GetName());
					}
				}
			}
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
	svComments.CopyFrom(&aSource->svComments);
	svInternalComments.CopyFrom(&aSource->svInternalComments);
	bRoot = aSource->bRoot;
	bForceUnique = aSource->bForceUnique;
	bIsUnique = aSource->bIsUnique;
	bIsKeyBasedStorable = aSource->bIsKeyBasedStorable;

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
				copyAttributeBlock->SetComments(attributeBlock->GetComments());
				copyAttributeBlock->SetInternalComments(attributeBlock->GetInternalComments());
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
	boolean bOk = true;
	boolean bAttributeOk = true;
	KWAttribute* attribute;
	int i;
	NumericKeyDictionary nkdKeyAttributes;
	ALString sTmp;

	assert(odAttributes.GetCount() == olAttributes.GetCount());

	// Arret si deja compile
	if (nFreshness == nCompileFreshness)
		return true;

	// Verification de l'existence d'un domaine de classe
	if (domain == NULL)
	{
		bOk = false;
		AddError(sTmp + "No domain for the dictionary");
	}
	else if (domain->LookupClass(GetName()) != this)
	{
		bOk = false;
		AddError(sTmp + "The dictionary is not found in its domain");
	}

	// Verification du Name
	if (not CheckName(GetName(), KWClass::Class, this))
		bOk = false;

	// Verification du Label
	if (not CheckLabel(GetLabel(), KWClass::Class, this))
		bOk = false;

	// Verification des commentaires
	if (not CheckComments(GetComments(), KWClass::Class, this))
		bOk = false;

	// Verification des commentaires internes
	if (not CheckComments(GetInternalComments(), KWClass::Class, this))
		bOk = false;

	// Verification des attributs
	bAttributeOk = true;
	Global::ActivateErrorFlowControl();
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// Test de l'attribut
		if (not attribute->Check())
			bAttributeOk = false;

		// Attribut suivant
		GetNextAttribute(attribute);
	}
	Global::DesactivateErrorFlowControl();
	if (not bAttributeOk)
	{
		bOk = false;
		AddError("Integrity errors");
	}

	// Verification de la cle
	if (bOk and GetKeyAttributeNumber() > 0)
	{
		// Verification de chaque champ de la cle
		for (i = 0; i < GetKeyAttributeNumber(); i++)
		{
			attribute = LookupAttribute(GetKeyAttributeNameAt(i));

			// Existance de la cle
			if (attribute == NULL)
			{
				bOk = false;
				AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) +
					 " not found in dictionary");
			}
			// La cle doit etre de type Symbol
			else if (attribute->GetType() != KWType::Symbol)
			{
				bOk = false;
				AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) + " must be of type " +
					 KWType::ToString(KWType::Symbol));
			}
			// La cle ne doit pas etre calculee
			else if (attribute->GetDerivationRule() != NULL)
			{
				bOk = false;
				AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) +
					 " must be a native variable, without derivation rule");
			}
			// La cle ne doit pas etre dans un bloc
			else if (attribute->GetBlockDerivationRule() != NULL)
			{
				bOk = false;
				AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) +
					 " must be not belong to a sparse value block computed from a derivation rule");
			}
			// Un meme attribut ne doit pas etre utilise plusieurs fois pour la cle
			else
			{
				// Test d'utilisation de l'attribut pour la cle
				if (nkdKeyAttributes.Lookup(attribute) != NULL)
				{
					bOk = false;
					AddError(sTmp + "Key variable " + GetKeyAttributeNameAt(i) +
						 " used several times in the key");
				}
				// Memorisation de l'utilisation de l'attribut pour la cle
				else
					nkdKeyAttributes.SetAt(attribute, attribute);
			}
		}
	}

	// Verification de l'absence de cycle dans la composition, et de la validite des cle
	// dans le cas d'une classe racine
	if (bOk)
		bOk = CheckNativeComposition(GetRoot(), true);

	return bOk;
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
	lUsedMemory += svComments.GetUsedMemory();
	lUsedMemory += svInternalComments.GetUsedMemory();
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
	lUsedMemory += oaUnloadedOwnedRelationAttributes.GetUsedMemory();
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
	int i;

	// Entete de la classe
	ost << "\n";
	if (GetLabel() != "")
		ost << "// " << GetLabel() << "\n";
	for (i = 0; i < GetComments()->GetSize(); i++)
		ost << "// " << GetComments()->GetAt(i) << "\n";
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

	// Meta-donnees
	WritePrivateMetaData(ost);
	if (metaData.GetKeyNumber() > 0)
	{
		metaData.Write(ost);
		ost << "\n";
	}
	ost << "{\n";

	// Attributs et blocs d'attributs
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// Bloc, au moment du premier attribut du bloc
		if (attribute->IsFirstInBlock())
			ost << *attribute->GetAttributeBlock() << "\n";
		// Attribut, s'il n'est pas dans un bloc
		else if (not attribute->IsInBlock())
			ost << *attribute << "\n";

		// Attribut suivant
		GetNextAttribute(attribute);
	}

	// Commentaire internes
	for (i = 0; i < GetInternalComments()->GetSize(); i++)
		ost << "\t// " << GetInternalComments()->GetAt(i) << "\n";
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
	if (GetComments()->GetSize() > 0)
	{
		fJSON->BeginKeyArray("comments");
		for (i = 0; i < GetComments()->GetSize(); i++)
			fJSON->WriteString(GetComments()->GetAt(i));
		fJSON->EndArray();
	}
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
		// Bloc, au moment du premier attribut du bloc
		if (attribute->IsFirstInBlock())
			attribute->GetAttributeBlock()->WriteJSONReport(fJSON);
		// Attribut, s'il n'est pas dans un bloc
		else if (not attribute->IsInBlock())
			attribute->WriteJSONReport(fJSON);

		// Attribut suivant
		GetNextAttribute(attribute);
	}
	fJSON->EndArray();

	// Commentaires internes
	if (GetInternalComments()->GetSize() > 0)
	{
		fJSON->BeginKeyArray("internalComments");
		for (i = 0; i < GetInternalComments()->GetSize(); i++)
			fJSON->WriteString(GetInternalComments()->GetAt(i));
		fJSON->EndArray();
	}
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
	return nNameMaxLength;
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

boolean KWClass::CheckComments(const StringVector* svValue, int nEntity, const Object* errorSender)
{
	ALString sMessage;
	boolean bOk;

	require(svValue != NULL);
	require(0 <= nEntity and nEntity < Unknown);

	bOk = CheckCommentsWithMessage(svValue, nEntity, sMessage);
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
	bOk = sValue.GetLength() <= nLabelMaxLength;
	if (not bOk)
		sMessage = "Incorrect " + EntityToString(nEntity) + " label : length > " +
			   IntToString(nLabelMaxLength) + " (" + GetShortValue(sValue) + ")";

	// Test de caractere fin de ligne
	if (bOk)
	{
		bOk = sValue.Find('\n') == -1;
		if (not bOk)
			sMessage = "Incorrect " + EntityToString(nEntity) +
				   " label : must not contain end-of-line chararacters (" + GetShortValue(sValue) + ")";
	}

	return bOk;
}

boolean KWClass::CheckCommentsWithMessage(const StringVector* svValue, int nEntity, ALString& sMessage)
{
	boolean bOk = true;
	int i;

	require(svValue != NULL);
	require(0 <= nEntity and nEntity < Unknown);

	// Test de la taille maximale
	bOk = svValue->GetSize() <= nCommentMaxNumber;
	if (not bOk)
		sMessage = "Incorrect " + EntityToString(nEntity) + " comments : comment line number " +
			   IntToString(svValue->GetSize()) + " > " + IntToString(nCommentMaxNumber);

	// Test des commentaires
	if (bOk)
	{
		for (i = 0; i < svValue->GetSize(); i++)
		{
			// Test de la taille maximale
			bOk = svValue->GetAt(i).GetLength() <= nCommentMaxLength;
			if (not bOk)
				sMessage = "Incorrect " + EntityToString(nEntity) + " comment line " +
					   IntToString(i + 1) + " : length > " + IntToString(nCommentMaxLength) + " (" +
					   GetShortValue(svValue->GetAt(i)) + ")";

			// Test de caractere fin de ligne
			if (bOk)
			{
				bOk = svValue->GetAt(i).Find('\n') == -1;
				if (not bOk)
					sMessage = "Incorrect " + EntityToString(nEntity) + " comment line " +
						   IntToString(i + 1) +
						   " : must not contain end-of-line chararacters (" +
						   GetShortValue(svValue->GetAt(i)) + ")";
			}
			if (not bOk)
				break;
		}
	}

	return bOk;
}

const ALString KWClass::GetShortValue(const ALString& sValue)
{
	static const int nMaxLength = 100;
	int nEndOfLinePosition;

	// On ignore ce qui se trouve apres la fin de ligne
	nEndOfLinePosition = sValue.Find('\n');
	if (nEndOfLinePosition >= 0)
		return sValue.Left(min(nEndOfLinePosition, nMaxLength)) + "...";
	else if (sValue.GetLength() > nMaxLength)
		return sValue.Left(min(nEndOfLinePosition, nMaxLength)) + "...";
	else
		return sValue;
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
	testClassClone->UnsafeRenameAttribute(attribute, "FirstAtt");
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
	testClassClone->UnsafeRenameAttribute(attribute, "LastAtt");
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

boolean KWClass::CheckNativeComposition(boolean bCheckKeys, boolean bVerboseCheckKeys) const
{
	NumericKeyDictionary nkdComponentClasses;

	// On passe en parametre le dictionnaire nkdComponentClasses de classes traitees, pour detecter les cycles
	return InternalCheckNativeComposition(bCheckKeys, bVerboseCheckKeys, NULL, &nkdComponentClasses);
}

boolean KWClass::InternalCheckNativeComposition(boolean bCheckKeys, boolean bVerboseCheckKeys,
						KWAttribute* parentAttribute,
						NumericKeyDictionary* nkdComponentClasses) const
{
	boolean bOk = true;
	boolean bClassKeyCheckedOnce;
	KWAttribute* attribute;
	KWClass* parentClass;
	KWClass* attributeClass;
	ALString sTmp;

	require(nkdComponentClasses != NULL);

	// Recherche de la classe parent a partir de l'attribut parent
	parentClass = NULL;
	if (parentAttribute != NULL)
		parentClass = parentAttribute->GetParentClass();

	// Ajout de la classe courante dans le dictionnaire des classes utilisees
	// pour l'analyse de l'utilisation recursive
	nkdComponentClasses->SetAt(this, (Object*)this);

	// Verification de la coherence entre statut racine/composant et presence de cle
	bClassKeyCheckedOnce = false;
	if (bOk and bCheckKeys and GetRoot() and GetKeyAttributeNumber() == 0)
	{
		if (bVerboseCheckKeys)
			AddError(sTmp + "Root dictionary must a key");
		bClassKeyCheckedOnce = true;
		bOk = false;
	}

	// Parcours des attributs de la classe
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		// Traitement des attributs de composition native
		if (KWType::IsRelation(attribute->GetType()) and attribute->GetAnyDerivationRule() == NULL and
		    attribute->GetClass() != NULL)
		{
			attributeClass = attribute->GetClass();

			// Test si necessaire de la presence de cle sur la classe courante si c'est la classe principale
			if (bOk and bCheckKeys and parentClass == NULL and not bClassKeyCheckedOnce and
			    GetKeyAttributeNumber() == 0)
			{
				if (bVerboseCheckKeys)
					AddError(sTmp +
						 "Dictionary must have a key, as it is composed of relation "
						 "variables, for example " +
						 attribute->GetName() + " of type " + RelationTypeToString(attribute));
				bClassKeyCheckedOnce = true;
				bOk = false;
			}

			// Test si classe deja utilisee
			if (bOk and nkdComponentClasses->Lookup(attributeClass) == attributeClass)
			{
				AddError("Existing composition cycle caused by the recursive use of dictionary " +
					 attributeClass->GetName() + " by variable " + attribute->GetName());
				bOk = false;
			}

			// Propagation du test si ok
			if (bOk)
				bOk = attributeClass->InternalCheckNativeComposition(bCheckKeys, bVerboseCheckKeys,
										     attribute, nkdComponentClasses);

			// Verification des cle si demande
			if (bOk and bCheckKeys)
			{
				// La classe utilisee doit avoir une cle
				if (attributeClass->GetKeyAttributeNumber() == 0)
				{
					if (bVerboseCheckKeys)
						AddError(sTmp + "The dictionary related to variable " +
							 attribute->GetName() + " of type " +
							 RelationTypeToString(attribute) + " must have a key");
					bOk = false;
				}
				// La cle de la classe utilisee doit etre au moins aussi longue que
				// celle de la classe utilisante dans le cas d'un lien de composition
				else if (attributeClass->GetKeyAttributeNumber() < GetKeyAttributeNumber())
				{
					if (bVerboseCheckKeys)
						AddError(sTmp + "The key length (" +
							 IntToString(attributeClass->GetKeyAttributeNumber()) +
							 ") of the dictionary related to variable " +
							 attribute->GetName() + " of type " +
							 RelationTypeToString(attribute) +
							 " must not be less than that of its parent dictionary " +
							 GetName() + " (" + IntToString(GetKeyAttributeNumber()) + ")");
					bOk = false;
				}
				// La cle de la classe utilisee doit etre strictement plus longue que celle de la classe utilisante
				// dans le cas d'un lien de composition multiple a trois niveau (ou plus)
				// Exception dans le cas d'une classe sans-cle pouvant etre issue d'une regle de creation de table,
				// auquel cas les cles ne sont pas necessaires y compris dans le cas d'un flocon cree par des regles
				else if (parentClass != NULL and parentAttribute->GetType() == KWType::ObjectArray and
					 parentClass->GetKeyAttributeNumber() > 0 and
					 parentClass->GetKeyAttributeNumber() >= GetKeyAttributeNumber())
				{
					if (bVerboseCheckKeys)
						AddError(sTmp + "As dictionary " + parentClass->GetName() +
							 " has a variable " + parentAttribute->GetName() + " of type " +
							 RelationTypeToString(parentAttribute) + " and dictionary " +
							 parentAttribute->GetClass()->GetName() +
							 " itself has a variable " + attribute->GetName() +
							 " of type " + RelationTypeToString(attribute) +
							 ", the key length (" + IntToString(GetKeyAttributeNumber()) +
							 ") in dictionary " + GetName() +
							 " must be strictly greater than that of its parent "
							 "dictionary " +
							 parentClass->GetName() + " (" +
							 IntToString(parentClass->GetKeyAttributeNumber()) + ")");
					bOk = false;
				}
			}
		}

		// Arret des la premiere erreur
		if (not bOk)
			break;

		// Attribut suivant
		GetNextAttribute(attribute);
	}

	// Supression de la classe courante dans le dictionnaire des classes utilisees
	nkdComponentClasses->RemoveKey(this);
	return bOk;
}

const ALString KWClass::RelationTypeToString(const KWAttribute* attribute) const
{
	require(attribute != NULL);
	require(KWType::IsRelation(attribute->GetType()));
	require(attribute->GetClass() != NULL);
	return KWType::ToString(attribute->GetType()) + '(' + attribute->GetClass()->GetName() + ')';
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

void KWClass::WriteAttributes(const ALString& sTitle, const ObjectArray* oaAttributes, ostream& ost) const
{
	KWAttribute* attribute;
	int i;

	require(oaAttributes != NULL);

	ost << sTitle << "\n";
	for (i = 0; i < oaAttributes->GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaAttributes->GetAt(i));
		ost << "\t" << i + 1 << "\t" << attribute->GetName() << "\n";
	}
}

void KWClass::WritePrivateMetaData(ostream& ost) const
{
	KWMetaData privateMetaData;

	// Memorisation dans une meta-data temporaire de l'information d'utilisation d'un attribut non charge en memoire
	// Permet de transferer cette information "privee", par exemple pour une tache parallele
	if (GetForceUnique())
	{
		privateMetaData.SetNoValueAt("_ForceUnique");
		privateMetaData.Write(ost);
		ost << ' ';
	}
}

void KWClass::ReadPrivateMetaData()
{
	KWAttribute* attribute;

	// Lecture de la meta-donne gerant le ForceUnique
	assert(not GetForceUnique());
	if (GetMetaData()->GetKeyNumber() > 0 and GetMetaData()->IsMissingTypeAt("_ForceUnique"))
	{
		SetForceUnique(true);
		GetMetaData()->RemoveKey("_ForceUnique");
	}

	// Parcours des attributs de la classe
	attribute = GetHeadAttribute();
	while (attribute != NULL)
	{
		attribute->ReadPrivateMetaData();
		GetNextAttribute(attribute);
	}
}
