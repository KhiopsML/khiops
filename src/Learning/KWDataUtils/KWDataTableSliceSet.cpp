// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataTableSliceSet.h"

// Fonction de comparaison de deux blocs d'attributs par taille decroissante
int KWDataTableSliceSetCompareAttributeBlock(const void* elem1, const void* elem2)
{
	KWAttributeBlock* attributeBlock1;
	KWAttributeBlock* attributeBlock2;
	int nCompare;

	// Acces aux blocs d'attributs
	attributeBlock1 = cast(KWAttributeBlock*, *(Object**)elem1);
	attributeBlock2 = cast(KWAttributeBlock*, *(Object**)elem2);

	// Comparaison sur leur nombre d'attributs de facon decroissante
	nCompare = -(attributeBlock1->GetLoadedAttributeNumber() - attributeBlock2->GetLoadedAttributeNumber());
	return nCompare;
}

///////////////////////////////////////////////////////////////////////////////////
// Class KWDataTableSliceSet

KWDataTableSliceSet::KWDataTableSliceSet()
{
	nTotalInstanceNumber = 0;
	bDeleteFilesAtClean = true;

	// Initialisation des variables de gestion de lecture
	read_Class = NULL;
	read_PhysicalClass = NULL;
	read_CurrentDomain = NULL;
	read_FirstPhysicalSlice = NULL;
}

KWDataTableSliceSet::~KWDataTableSliceSet()
{
	Clean();
}

void KWDataTableSliceSet::ComputeSpecification(const KWClass* kwcClassToPartition, const ALString& sTargetAttributeName,
					       int nInstanceNumber, int nMaxDenseAttributeNumberPerSlice)
{
	boolean bForceManySubSlices = false;
	int nRemainingAttributeNumber;
	IntVector ivBaseLexicographicIndex;
	IntVector ivSliceAttributeNumbers;
	IntVector ivSliceBlockNumbers;
	IntVector ivUsedAttributesSliceIndexes;
	NumericKeyDictionary nkdAttributeIndexes;
	IntObject* ioIndex;
	int nAttributeIndex;
	int nSliceIndex;
	int nEmptySliceNumber;
	KWAttribute* targetAttribute;
	KWAttributeBlock* targetAttributeBlock;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	int nBlockSize;
	int nBlockSliceNumber;
	ObjectArray oaAttributes;
	ObjectArray oaAttributeBlocks;
	int nRandomSeed;
	int nBlock;
	int i;
	int n;

	require(kwcClassToPartition != NULL);
	require(kwcClassToPartition->IsCompiled());
	require(sTargetAttributeName == "" or kwcClassToPartition->LookupAttribute(sTargetAttributeName) != NULL);
	require(GetUsedSimpleAttributeNumber() == 0 or nMaxDenseAttributeNumberPerSlice > 0);
	require(nInstanceNumber >= 0);

	// Methode de debug pour forcer la creation de nombreuses sous-tranches
	if (bForceManySubSlices)
		nMaxDenseAttributeNumberPerSlice = 2;

	// Initialisation
	Clean();
	nRandomSeed = GetRandomSeed();

	// Parametrage de la classe
	sClassName = kwcClassToPartition->GetName();
	sClassTargetAttributeName = sTargetAttributeName;

	// Memorisation du nombre d'instances
	nTotalInstanceNumber = nInstanceNumber;

	// Recherche de l'attribut cible et de son bloc
	targetAttribute = NULL;
	targetAttributeBlock = NULL;
	if (sTargetAttributeName != "")
	{
		targetAttribute = kwcClassToPartition->LookupAttribute(sTargetAttributeName);
		targetAttributeBlock = targetAttribute->GetAttributeBlock();
	}

	// Memorisation de l'index de chaque attribut de la classe
	// On va en effet traiter les attribut dans un ordre aleatroire pour les melanger dans les tranches,
	// et on aura besoin de retrouver leur index pour les associer a leur index de tranche
	nAttributeIndex = 0;
	attribute = kwcClassToPartition->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Memorisation de l'index associe a l'attribut
		ioIndex = new IntObject;
		ioIndex->SetInt(nAttributeIndex);
		nkdAttributeIndexes.SetAt((NUMERIC)attribute, ioIndex);

		// Attribut suivant
		nAttributeIndex++;
		kwcClassToPartition->GetNextAttribute(attribute);
	}

	// Initialisation du vecteur resultat de la partition
	ivUsedAttributesSliceIndexes.SetSize(kwcClassToPartition->GetAttributeNumber());
	for (nAttributeIndex = 0; nAttributeIndex < ivUsedAttributesSliceIndexes.GetSize(); nAttributeIndex++)
		ivUsedAttributesSliceIndexes.SetAt(nAttributeIndex, -1);

	//////////////////////////////////////////////////////////////////////////////
	// Collecte des attributs denses et des blocs d'attributs

	// Collecte de tous les attributs denses
	for (n = 0; n < kwcClassToPartition->GetLoadedDenseAttributeNumber(); n++)
	{
		attribute = kwcClassToPartition->GetLoadedDenseAttributeAt(n);
		if (KWType::IsSimple(attribute->GetType()) and attribute != targetAttribute)
			oaAttributes.Add(attribute);
	}

	// Permutation aleatoire de l'ordre des attributs
	// pour favoriser la creation de tranches homogenes
	oaAttributes.Shuffle();

	// Collecte de tous les blocs d'attributs
	for (nBlock = 0; nBlock < kwcClassToPartition->GetLoadedAttributeBlockNumber(); nBlock++)
		oaAttributeBlocks.Add(kwcClassToPartition->GetLoadedAttributeBlockAt(nBlock));

	// Permutation aleatoire des l'ordre des blocs
	// pour favoriser la creation de tranches homogenes
	oaAttributeBlocks.Shuffle();

	// Tri des blocs par taille decroissante
	oaAttributeBlocks.SetCompareFunction(KWDataTableSliceSetCompareAttributeBlock);
	oaAttributeBlocks.Sort();

	// Premiere tranche si necessaire
	nSliceIndex = -1;
	if (oaAttributes.GetSize() > 0 or oaAttributeBlocks.GetSize() > 0)
	{
		ivSliceAttributeNumbers.Add(0);
		ivSliceBlockNumbers.Add(0);
		nSliceIndex = 0;
	}

	//////////////////////////////////////////////////////////////////////////////
	// On commence par mettre les blocs dans des tranches

	// Parcours des blocs et creation des blocs au fur et a mesure
	// On met les bloc dans une seule tranche s'il peuvent tenir, sinon, on divise les gros blocs en
	// autant de tranches que necessaire. On aura ainsi des tranches de taille comprise entre le
	// le nombre max d'attributs par tranche, et la moitie pour les gros blocs, voire 2 attributs
	// seulement dans le pire des cas (si une nouvelle tranche doit etre creer pour un petit bloc
	// de 2 attributs). Ce cas degenere est marginal, et ce n'est pas grave dans la mesure ou
	// les algoritmes paralleles ordonnanceront le traitement des tranches par taille decroissante
	// de facon a minimiser le temps d'attente du traitement de la derniere tranche.
	for (nBlock = 0; nBlock < oaAttributeBlocks.GetSize(); nBlock++)
	{
		attributeBlock = cast(KWAttributeBlock*, oaAttributeBlocks.GetAt(nBlock));

		// Calcul de la taille du bloc
		nBlockSize = attributeBlock->GetLoadedAttributeNumber();
		if (attributeBlock == targetAttributeBlock)
			nBlockSize--;

		// On decoupe le bloc en plusieurs tranches s'il ne peut tenir en une seule tranche
		if (nBlockSize > nMaxDenseAttributeNumberPerSlice)
		{
			// Creation d'une nouvelle tranche si necessaire, en fait des que la tranche en cours est non
			// vide On va en effet repartir le bloc uniquement sur des tranches vides
			if (ivSliceAttributeNumbers.GetAt(nSliceIndex) > 0)
			{
				ivSliceAttributeNumbers.Add(0);
				ivSliceBlockNumbers.Add(0);
				nSliceIndex++;
			}
			assert(nSliceIndex == ivSliceAttributeNumbers.GetSize() - 1);

			// Creation d'autant de tranches que necessaire
			nBlockSliceNumber = (int)ceil(nBlockSize / (double)nMaxDenseAttributeNumberPerSlice);
			assert(nBlockSliceNumber >= 2);
			for (n = 1; n < nBlockSliceNumber; n++)
			{
				ivSliceAttributeNumbers.Add(0);
				ivSliceBlockNumbers.Add(0);
			}

			// On ajoute prend en compte le bloc pour chaque tranche
			for (n = 0; n < nBlockSliceNumber; n++)
			{
				nSliceIndex = ivSliceBlockNumbers.GetSize() - nBlockSliceNumber + n;
				ivSliceBlockNumbers.UpgradeAt(nSliceIndex, 1);
			}

			// Repartition des attributs dans ces tranches
			i = 0;
			for (n = 0; n < attributeBlock->GetLoadedAttributeNumber(); n++)
			{
				attribute = attributeBlock->GetLoadedAttributeAt(n);

				// Memorisation de la tranche associee
				if (attribute != targetAttribute)
				{
					// Les attributs sont repartis de facon contigue dans les tranches
					nAttributeIndex =
					    cast(IntObject*, nkdAttributeIndexes.Lookup((NUMERIC)attribute))->GetInt();
					nSliceIndex = ivSliceAttributeNumbers.GetSize() - nBlockSliceNumber +
						      (i * nBlockSliceNumber) / nBlockSize;
					ivUsedAttributesSliceIndexes.SetAt(nAttributeIndex, nSliceIndex);
					ivSliceAttributeNumbers.UpgradeAt(nSliceIndex, 1);

					// Attention, on compte les attributs hors attribut cible, pour eviter d'vaoire
					// une tranche vide dans les cas extreme
					i++;
				}
			}

			// Reactualisation du dernier index de tranche en cours
			nSliceIndex = ivSliceAttributeNumbers.GetSize() - 1;
			assert(ivSliceAttributeNumbers.GetAt(nSliceIndex) > 0);
			assert(ivSliceAttributeNumbers.GetAt(nSliceIndex) >= ivSliceBlockNumbers.GetAt(nSliceIndex));
			assert(ivSliceAttributeNumbers.GetAt(nSliceIndex) >= nBlockSize / nBlockSliceNumber);
			assert(ivSliceAttributeNumbers.GetAt(nSliceIndex - nBlockSliceNumber + 1) >=
			       nBlockSize / nBlockSliceNumber);
		}
		// On ajoute les blocs en entier s'il peuvent tenir dans une seule tranche
		else
		{
			assert(nBlockSize <= nMaxDenseAttributeNumberPerSlice);

			// Creation d'une nouvelle tranche si necessaire, en prenant en egalement les blocs
			if (ivSliceAttributeNumbers.GetAt(nSliceIndex) + ivSliceBlockNumbers.GetAt(nSliceIndex) +
				nBlockSize >
			    nMaxDenseAttributeNumberPerSlice)
			{
				ivSliceAttributeNumbers.Add(0);
				ivSliceBlockNumbers.Add(0);
				nSliceIndex++;
			}

			// On prend en compte le bloc
			ivSliceBlockNumbers.UpgradeAt(nSliceIndex, 1);

			// Prise en compte de tous les attributs du bloc
			for (n = 0; n < attributeBlock->GetLoadedAttributeNumber(); n++)
			{
				attribute = attributeBlock->GetLoadedAttributeAt(n);

				// Memorisation de la tranche associee a l'attribut
				if (attribute != targetAttribute)
				{
					nAttributeIndex =
					    cast(IntObject*, nkdAttributeIndexes.Lookup((NUMERIC)attribute))->GetInt();
					ivUsedAttributesSliceIndexes.SetAt(nAttributeIndex, nSliceIndex);
					ivSliceAttributeNumbers.UpgradeAt(nSliceIndex, 1);
				}
			}
			assert(ivSliceAttributeNumbers.GetAt(nSliceIndex) <= nMaxDenseAttributeNumberPerSlice);
			assert(ivSliceAttributeNumbers.GetAt(nSliceIndex) + ivSliceBlockNumbers.GetAt(nSliceIndex) <=
			       nMaxDenseAttributeNumberPerSlice + 1);
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	// On continue en rangeant les attributs dense dans des tranches

	// Calcul du nombre d'attributs qui pourraient encore rentrer dans les tranches
	// Attention, le nombre d'attribut collecte peut depasser de 1 la limite, s'il contient une seule grosse tranche
	nRemainingAttributeNumber = 0;
	for (n = 0; n < ivSliceAttributeNumbers.GetSize(); n++)
	{
		assert(ivSliceAttributeNumbers.GetAt(n) > 0 or
		       (ivSliceAttributeNumbers.GetSize() == 1 and
			kwcClassToPartition->GetLoadedAttributeBlockNumber() == 0));
		assert(ivSliceAttributeNumbers.GetAt(n) <= nMaxDenseAttributeNumberPerSlice);
		assert(ivSliceAttributeNumbers.GetAt(n) + ivSliceBlockNumbers.GetAt(n) <=
		       nMaxDenseAttributeNumberPerSlice + 1);
		nRemainingAttributeNumber += nMaxDenseAttributeNumberPerSlice - ivSliceAttributeNumbers.GetAt(n);
	}

	// Creation de tranches vides supplementaires si necessaire
	assert(ivSliceAttributeNumbers.GetSize() > 0 or
	       (oaAttributes.GetSize() == 0 and oaAttributeBlocks.GetSize() == 0));
	assert(ivSliceAttributeNumbers.GetSize() <= 1 or
	       ivSliceAttributeNumbers.GetAt(ivSliceAttributeNumbers.GetSize() - 2) > 0);
	nEmptySliceNumber = 0;
	if (ivSliceAttributeNumbers.GetSize() > 0 and
	    ivSliceAttributeNumbers.GetAt(ivSliceAttributeNumbers.GetSize() - 1) == 0)
		nEmptySliceNumber = 1;
	while (nRemainingAttributeNumber < oaAttributes.GetSize())
	{
		ivSliceAttributeNumbers.Add(0);
		ivSliceBlockNumbers.Add(0);
		nRemainingAttributeNumber += nMaxDenseAttributeNumberPerSlice;
		nEmptySliceNumber++;
	}

	// Parcours des attributs en remplissant les tranches, en partant des tranches vides disponibles,
	// puis en repartant au debut qui contient potentiellement les tranches ayant le moins d'attributs
	nSliceIndex = 0;
	for (n = 0; n < oaAttributes.GetSize(); n++)
	{
		attribute = cast(KWAttribute*, oaAttributes.GetAt(n));
		assert(attribute != targetAttribute);

		// Acces a l'index de l'attribut
		nAttributeIndex = cast(IntObject*, nkdAttributeIndexes.Lookup((NUMERIC)attribute))->GetInt();

		// On cherche d'abord a remplir uniformenent les eventuelles tranches supplementaires
		if (n < nEmptySliceNumber * nMaxDenseAttributeNumberPerSlice)
		{
			nSliceIndex = (ivSliceAttributeNumbers.GetSize() - nEmptySliceNumber) + n % nEmptySliceNumber;
		}
		// Sinon, on recherche la premiere tranche ayant assez de place
		else
		{
			while (ivSliceAttributeNumbers.GetAt(nSliceIndex) == nMaxDenseAttributeNumberPerSlice)
				nSliceIndex = (nSliceIndex + 1) % ivSliceAttributeNumbers.GetSize();
		}

		// On range associe l'attribut a sa tranche
		ivUsedAttributesSliceIndexes.SetAt(nAttributeIndex, nSliceIndex);
		ivSliceAttributeNumbers.UpgradeAt(nSliceIndex, 1);
	}

	// Specification du sliceset  sur la base de cette partition
	BuildSpecificationFromClassPartition(kwcClassToPartition, sTargetAttributeName, nInstanceNumber,
					     ivSliceAttributeNumbers.GetSize(), &ivBaseLexicographicIndex,
					     &ivUsedAttributesSliceIndexes);
	assert(GetSliceNumber() == ivSliceAttributeNumbers.GetSize());

	// Nettoyage
	nkdAttributeIndexes.DeleteAll();
	SetRandomSeed(nRandomSeed);
	ensure(Check());
}

void KWDataTableSliceSet::BuildSpecificationFromClassPartition(const KWClass* kwcClassToPartition,
							       const ALString& sTargetAttributeName,
							       int nInstanceNumber, int nSliceNumber,
							       const IntVector* ivBaseLexicographicIndex,
							       const IntVector* ivAttributeSliceIndexes)
{
	boolean bShowSliceClasses = false;
	int nAttribute;
	KWAttribute* attribute;
	KWAttributeBlock* lastAttributeBlock;
	KWAttributeBlock* nextAttributeBlock;
	KWAttribute* sliceAttribute;
	KWAttribute* sliceBlockFirstAttribute;
	KWAttribute* sliceNextAttribute;
	KWAttributeBlock* sliceAttributeBlock;
	KWDataTableSlice* slice;
	KWClass* kwcSliceClass;
	int nSlice;

	require(kwcClassToPartition != NULL);
	require(kwcClassToPartition->IsCompiled());
	require(sTargetAttributeName == "" or kwcClassToPartition->LookupAttribute(sTargetAttributeName) != NULL);
	require(nInstanceNumber >= 0);
	require(nSliceNumber >= 0);
	require(ivBaseLexicographicIndex != NULL);
	require(ivAttributeSliceIndexes != NULL);
	require(kwcClassToPartition->GetAttributeNumber() == ivAttributeSliceIndexes->GetSize());

	// Initialisation
	Clean();

	// Parametrage de la classe
	sClassName = kwcClassToPartition->GetName();
	sClassTargetAttributeName = sTargetAttributeName;

	// Memorisation du nombre d'instances
	nTotalInstanceNumber = nInstanceNumber;

	// Creation des tranches demandees
	for (nSlice = 0; nSlice < nSliceNumber; nSlice++)
		oaSlices.Add(new KWDataTableSlice);

	// Parcours des attributs de la classe a partitionner pour les dispatcher dans les tranches
	attribute = kwcClassToPartition->GetHeadAttribute();
	nAttribute = 0;
	while (attribute != NULL)
	{
		// Recherche de l'index de la tranche
		nSlice = ivAttributeSliceIndexes->GetAt(nAttribute);
		assert(-1 <= nSlice and nSlice < nSliceNumber);

		// Ajout dans une tranche si necessaire
		if (nSlice >= 0)
		{
			slice = GetSliceAt(nSlice);

			// Verification
			assert(attribute->GetUsed());
			assert(KWType::IsSimple(attribute->GetType()));
			assert(attribute->GetName() != sTargetAttributeName);

			// Creation d'un nouvel attribut
			sliceAttribute = new KWAttribute;
			sliceAttribute->SetName(attribute->GetName());
			sliceAttribute->SetType(attribute->GetType());
			sliceAttribute->SetCost(attribute->GetCost());
			sliceAttribute->GetMetaData()->CopyFrom(attribute->GetConstMetaData());

			// Ajout dans la classe de la tranche
			slice->GetClass()->InsertAttribute(sliceAttribute);
		}

		// Attribut suivant
		kwcClassToPartition->GetNextAttribute(attribute);
		nAttribute++;
	}

	// Parcours des tranches pour creer les blocs
	for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

		// Finalisation de la classe
		kwcSliceClass = slice->GetClass();

		// Parcours des attributs de la tranche
		sliceBlockFirstAttribute = NULL;
		lastAttributeBlock = NULL;
		sliceAttribute = kwcSliceClass->GetHeadAttribute();
		while (sliceAttribute != NULL)
		{
			// Recherche de l'attribut suivant
			sliceNextAttribute = sliceAttribute;
			kwcSliceClass->GetNextAttribute(sliceNextAttribute);

			// Recherche de l'attribut de la classe a partitionner
			attribute = kwcClassToPartition->LookupAttribute(sliceAttribute->GetName());

			// On memorise si necessaire le premier attribut d'un bloc dans la tranche
			// On se rappelle que les attributs d'un bloc doivent etre distribues par plages contigues dans
			// les tranches
			if (attribute->GetAttributeBlock() != NULL and
			    attribute->GetAttributeBlock() != lastAttributeBlock)
			{
				sliceBlockFirstAttribute = sliceAttribute;
				lastAttributeBlock = attribute->GetAttributeBlock();
			}

			// Creation si necessaire d'un bloc d'attributs dans la tranche
			if (sliceBlockFirstAttribute != NULL)
			{
				assert(attribute->GetAttributeBlock() == lastAttributeBlock);

				// Recherche du block associe a l'attribut suivant
				nextAttributeBlock = NULL;
				if (sliceNextAttribute != NULL)
					nextAttributeBlock =
					    kwcClassToPartition->LookupAttribute(sliceNextAttribute->GetName())
						->GetAttributeBlock();

				// Creation d'un bloc s'il y a changement de bloc a l'attribut suivant
				if (nextAttributeBlock != lastAttributeBlock)
				{
					// On regroupe les attributs crees sous forme d'un bloc
					sliceAttributeBlock = kwcSliceClass->CreateAttributeBlock(
					    lastAttributeBlock->GetName(), sliceBlockFirstAttribute, sliceAttribute);

					// On tri les attribut du bloc selon leur VarkKey, ce qui necessaire pour la
					// gestion des blocs sur plusieurs tranches
					sliceAttributeBlock->SortAttributesByVarKey();

					// On recopie les meta-data, mais pas la regle de derivation potentielle
					// Rajout si necessaire d'une meta-data pour memoriser la valeur par defaut
					// associee au bloc
					sliceAttributeBlock->ImportMetaDataFrom(lastAttributeBlock);

					// Preparation pour le prochain bloc
					sliceBlockFirstAttribute = NULL;
				}
			}

			// Attribut suivant
			lastAttributeBlock = attribute->GetAttributeBlock();
			kwcSliceClass->GetNextAttribute(sliceAttribute);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	// Finalisation des tranches

	// Finalisation de la specification des classes de chaque tranche
	for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

		// Index lexicographique
		assert(slice->GetLexicographicIndex()->GetSize() == 0);
		slice->GetLexicographicIndex()->CopyFrom(ivBaseLexicographicIndex);
		slice->GetLexicographicIndex()->Add(nSlice);

		// Finalisation de la classe
		kwcSliceClass = slice->GetClass();
		slice->GetClass()->SetName(KWClassDomain::GetCurrentDomain()->BuildClassName(
					       KWDataTableSlice::BuildPrefix(slice->GetLexicographicIndex(), -1)) +
					   kwcClassToPartition->GetName());
		kwcSliceClass->IndexClass();
		assert(kwcSliceClass->GetUsedAttributeNumber() == kwcSliceClass->GetLoadedAttributeNumber());

		// On l'insere dans le domaine pour reserver le nom de la classe autant que possible
		KWClassDomain::GetCurrentDomain()->InsertClass(kwcSliceClass);
	}

	// Affichage des classes du domaine
	if (bShowSliceClasses)
	{
		cout << "KWDataTableSliceSet::BuildSpecificationFromClassPartition\n";
		cout << *KWClassDomain::GetCurrentDomain() << endl;
	}

	// Supression des classes du domaine
	// Les classes appartiennent aux tranches, et ne sont a inserer dans un domaine que le temps de leur utilisation
	for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

		// Supression de la classe de son domaine
		KWClassDomain::GetCurrentDomain()->RemoveClass(slice->GetClass()->GetName());
	}
	ensure(Check());
}

void KWDataTableSliceSet::ComputeSlicesLoadIndexes(const KWClass* kwcClassToPartition)
{
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWAttribute* sliceAttribute;
	KWAttributeBlock* sliceAttributeBlock;
	KWDataTableSlice* slice;
	int nValueBlockFirstSparseIndex;
	int nValueBlockLastSparseIndex;
	int nSlice;

	require(kwcClassToPartition != NULL);
	require(kwcClassToPartition->IsCompiled());

	// Parcours de toutes les tranches
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
	{
		slice = GetSliceAt(nSlice);

		// Reinitialisation des caracteristiques de la tranche
		slice->GetDataItemLoadIndexes()->SetSize(0);
		slice->GetValueBlockFirstSparseIndexes()->SetSize(0);
		slice->GetValueBlockLastSparseIndexes()->SetSize(0);

		// Parcours des attributs de la tranche
		sliceAttribute = slice->GetClass()->GetHeadAttribute();
		while (sliceAttribute != NULL)
		{
			// Recherche de l'attribut dans la classe a partitionner
			attribute = kwcClassToPartition->LookupAttribute(sliceAttribute->GetName());
			assert(attribute != NULL);

			// Cas d'un attribut d'un bloc
			sliceAttributeBlock = sliceAttribute->GetAttributeBlock();
			if (sliceAttributeBlock != NULL)
			{
				// Recherche du bloc dans la classe a partitionner
				attributeBlock = attribute->GetAttributeBlock();
				assert(attributeBlock != NULL);
				assert(attributeBlock->GetName() == sliceAttributeBlock->GetName());

				// Recherche de la position des premier et derniers attributs du bloc de la slice dans
				// le bloc de la classe a partitionner
				nValueBlockFirstSparseIndex = attribute->GetLoadIndex().GetSparseIndex();
				nValueBlockLastSparseIndex =
				    nValueBlockFirstSparseIndex + sliceAttributeBlock->GetAttributeNumber() - 1;

				// On met a jour les index permettant la correspondance entre les attributs dans la
				// tranche et la classe partitionnee
				slice->GetDataItemLoadIndexes()->Add(attributeBlock->GetLoadIndex());
				slice->GetValueBlockFirstSparseIndexes()->Add(nValueBlockFirstSparseIndex);
				slice->GetValueBlockLastSparseIndexes()->Add(nValueBlockLastSparseIndex);

				// On se positionne sur le dernier attribut du bloc, pour preparer le passage a
				// l'attribut suivant
				sliceAttribute = sliceAttributeBlock->GetLastAttribute();
				assert(nValueBlockLastSparseIndex ==
				       kwcClassToPartition->LookupAttribute(sliceAttribute->GetName())
					   ->GetLoadIndex()
					   .GetSparseIndex());
			}
			else
			// Cas d'un attribut hors bloc
			{
				// On met a jour les index permettant la correspondance entre les attributs dans la
				// tranche et la classe partitionnee
				slice->GetDataItemLoadIndexes()->Add(attribute->GetLoadIndex());
				slice->GetValueBlockFirstSparseIndexes()->Add(-1);
				slice->GetValueBlockLastSparseIndexes()->Add(-1);
			}

			// Attribut suivant
			slice->GetClass()->GetNextAttribute(sliceAttribute);
		}
	}
}

void KWDataTableSliceSet::CleanSlicesLoadIndexes()
{
	KWDataTableSlice* slice;
	int nSlice;

	// Parcours de toutes les tranches
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
	{
		slice = GetSliceAt(nSlice);

		// Reinitialisation des caracteristiques de la tranche
		slice->GetDataItemLoadIndexes()->SetSize(0);
		slice->GetValueBlockFirstSparseIndexes()->SetSize(0);
		slice->GetValueBlockLastSparseIndexes()->SetSize(0);
	}
}

const ALString& KWDataTableSliceSet::GetClassName() const
{
	return sClassName;
}

const ALString& KWDataTableSliceSet::GetTargetAttributeName() const
{
	return sClassTargetAttributeName;
}

int KWDataTableSliceSet::GetSliceNumber() const
{
	return oaSlices.GetSize();
}

KWDataTableSlice* KWDataTableSliceSet::GetSliceAt(int nIndex)
{
	require(0 <= nIndex and nIndex < GetSliceNumber());
	return cast(KWDataTableSlice*, oaSlices.GetAt(nIndex));
}

int KWDataTableSliceSet::GetChunkNumber() const
{
	if (oaSlices.GetSize() == 0)
		return 0;
	else
		return cast(KWDataTableSlice*, oaSlices.GetAt(0))->GetDataFileNames()->GetSize();
}

int KWDataTableSliceSet::GetTotalInstanceNumber() const
{
	return nTotalInstanceNumber;
}

IntVector* KWDataTableSliceSet::GetChunkInstanceNumbers()
{
	return &ivChunkInstanceNumbers;
}

void KWDataTableSliceSet::UpdateTotalInstanceNumber()
{
	int i;

	nTotalInstanceNumber = 0;
	for (i = 0; i < ivChunkInstanceNumbers.GetSize(); i++)
		nTotalInstanceNumber += ivChunkInstanceNumbers.GetAt(i);
}

void KWDataTableSliceSet::Clean()
{
	assert(GetReadClass() == NULL or not IsOpenedForRead());
	if (GetDeleteFilesAtClean())
		DeleteAllSliceFiles();
	oaSlices.DeleteAll();
	ivChunkInstanceNumbers.SetSize(0);
	nTotalInstanceNumber = 0;
}

void KWDataTableSliceSet::SetDeleteFilesAtClean(boolean bValue)
{
	bDeleteFilesAtClean = bValue;
}

boolean KWDataTableSliceSet::GetDeleteFilesAtClean() const
{
	return bDeleteFilesAtClean;
}

int KWDataTableSliceSet::GetTotalAttributeNumber() const
{
	int nTotalAttributeNumber;
	int nSlice;
	KWDataTableSlice* slice;

	// Calcul du total cumule sur les tranches
	nTotalAttributeNumber = 0;
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		nTotalAttributeNumber += slice->GetClass()->GetAttributeNumber();
	}
	return nTotalAttributeNumber;
}

int KWDataTableSliceSet::GetTotalDenseAttributeNumber() const
{
	int nTotalDenseAttributeNumber;
	int nSlice;
	KWDataTableSlice* slice;

	// Calcul du total cumule sur les tranches
	nTotalDenseAttributeNumber = 0;
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		nTotalDenseAttributeNumber += slice->GetClass()->GetLoadedDenseAttributeNumber();
	}
	return nTotalDenseAttributeNumber;
}

int KWDataTableSliceSet::GetTotalAttributeBlockNumber() const
{
	int nTotalAttributeBlockNumber;
	int nSlice;
	KWDataTableSlice* slice;

	// Calcul du total cumule sur les tranches
	nTotalAttributeBlockNumber = 0;
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		assert(slice->GetClass()->GetLoadedAttributeNumber() == slice->GetClass()->GetUsedAttributeNumber());
		nTotalAttributeBlockNumber += slice->GetClass()->GetLoadedAttributeBlockNumber();
	}
	return nTotalAttributeBlockNumber;
}

longint KWDataTableSliceSet::GetTotalDenseSymbolAttributeDiskSize() const
{
	longint lTotalDenseSymbolAttributeDiskSize;
	int nSlice;
	KWDataTableSlice* slice;

	// Calcul du total cumule sur les tranches
	lTotalDenseSymbolAttributeDiskSize = 0;
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		lTotalDenseSymbolAttributeDiskSize += slice->GetTotalDenseSymbolAttributeDiskSize();
	}
	return lTotalDenseSymbolAttributeDiskSize;
}

longint KWDataTableSliceSet::GetTotalDataFileSize() const
{
	longint lTotalDataFileSize;
	int nSlice;
	KWDataTableSlice* slice;

	// Calcul du total cumule sur les tranches
	lTotalDataFileSize = 0;
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		lTotalDataFileSize += slice->GetTotalDataFileSize();
	}
	return lTotalDataFileSize;
}

longint KWDataTableSliceSet::GetTotalAttributeBlockValueNumber() const
{
	longint lTotalAttributeBlockValueNumber;
	int nSlice;
	KWDataTableSlice* slice;

	// Calcul du total cumule sur les tranches
	lTotalAttributeBlockValueNumber = 0;
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		lTotalAttributeBlockValueNumber += slice->GetTotalAttributeBlockValueNumber();
	}
	return lTotalAttributeBlockValueNumber;
}

boolean KWDataTableSliceSet::CheckReadClass(const KWClass* kwcInputClass) const
{
	boolean bOk = true;
	ObjectDictionary odSliceAttributes;
	KWAttribute* attribute;
	int nAttribute;
	KWAttribute* sliceAttribute;
	KWDataTableSlice* slice;
	ObjectDictionary odUsedSlices;
	ObjectArray oaDerivedAttributes;
	KWDerivationRule* currentDerivationRule;
	NumericKeyDictionary nkdAllUsedSliceAttributes;
	NumericKeyDictionary nkdAllUsedDerivedAttributes;
	ObjectArray oaClassNeededAttributes;
	ALString sErrorAttribute;

	require(kwcInputClass != NULL);
	require(kwcInputClass->IsCompiled());

	// Recherche de tous les attributs disponibles dans les tranches
	FillSliceAttributes(&odSliceAttributes);

	// Parcours des attributs a charger de la classe
	currentDerivationRule = NULL;
	for (nAttribute = 0; nAttribute < kwcInputClass->GetLoadedAttributeNumber(); nAttribute++)
	{
		attribute = kwcInputClass->GetLoadedAttributeAt(nAttribute);

		// On recherche s'il est dans une tranche
		slice = cast(KWDataTableSlice*, odSliceAttributes.Lookup(attribute->GetName()));

		// On memorise sa tranche si necessaire
		if (slice != NULL)
		{
			odUsedSlices.SetAt(slice->GetClass()->GetName(), slice);

			// L'attribut doit etre de meme type que dans la tranche
			sliceAttribute = slice->GetClass()->LookupAttribute(attribute->GetName());
			check(sliceAttribute);
			assert(KWType::IsSimple(sliceAttribute->GetType()));
			if (attribute->GetType() != sliceAttribute->GetType())
			{
				AddError("Variable " + attribute->GetName() + " in read dictionnary " +
					 kwcInputClass->GetName() + " has not the same type (" +
					 KWType::ToString(attribute->GetType()) +
					 ") as that of the corresponding variable (" +
					 KWType::ToString(sliceAttribute->GetType()) + ") in slice " +
					 slice->GetObjectLabel());
				bOk = false;
				break;
			}
			// Cas ou un des attribut est dans un bloc
			else if (attribute->IsInBlock() or sliceAttribute->IsInBlock())
			{
				// Message si seul l'attribut dans la tranche n'est pas dans un bloc
				if (not attribute->IsInBlock())
				{
					assert(sliceAttribute->IsInBlock());
					AddError("Variable " + attribute->GetName() + " in read dictionnary " +
						 kwcInputClass->GetName() +
						 " is not in a block whereas the corresponding variable in slice " +
						 slice->GetObjectLabel() + " is in block " +
						 sliceAttribute->GetAttributeBlock()->GetName());
					bOk = false;
					break;
				}
				// Message si seul l'attribut n'est pas dans un bloc
				else if (not sliceAttribute->IsInBlock())
				{
					assert(attribute->IsInBlock());
					AddError("Variable " + attribute->GetName() + " in read dictionnary " +
						 kwcInputClass->GetName() + " is in block " +
						 attribute->GetAttributeBlock()->GetName() +
						 " whereas the corresponding variable in slice " +
						 slice->GetObjectLabel() + " is not in a block");
					bOk = false;
					break;
				}
				// Cas ou chaque attribut est dans un bloc
				else
				{
					assert(attribute->IsInBlock());
					assert(sliceAttribute->IsInBlock());

					// Ils doivent avoir le meme nom de block
					if (attribute->GetAttributeBlock()->GetName() !=
					    sliceAttribute->GetAttributeBlock()->GetName())
					{
						AddError("Variable " + attribute->GetName() + " in read dictionnary " +
							 kwcInputClass->GetName() + " is in block " +
							 attribute->GetAttributeBlock()->GetName() +
							 " whereas the corresponding variable in slice " +
							 slice->GetObjectLabel() + " is in block " +
							 sliceAttribute->GetAttributeBlock()->GetName());
						bOk = false;
						break;
					}
				}
			}
		}
		// Sinon, on traite le cas d'un attribut qui n'est pas dans les tranches
		else
		{
			// L'attribut ne doit pas etre natif
			if (attribute->GetAnyDerivationRule() == NULL)
			{
				AddError("Variable " + attribute->GetName() + " in read dictionnary " +
					 kwcInputClass->GetName() +
					 " is not in a slice: it should not be a native variable");
				bOk = false;
				break;
			}
			// L'attribut ne doit pas etre de type relation
			else if (KWType::IsRelation(attribute->GetType()))
			{
				AddError("Variable " + attribute->GetName() + " in read dictionnary " +
					 kwcInputClass->GetName() + " is not in a slice: it should not be of type " +
					 KWType::ToString(attribute->GetType()));
				bOk = false;
				break;
			}
			// Cas ok
			else
			{
				// Detection de changement de regle de derivation (notamment pour les blocs)
				if (attribute->GetAnyDerivationRule() != currentDerivationRule)
				{
					currentDerivationRule = attribute->GetAnyDerivationRule();
					check(currentDerivationRule);

					// Ajout des attribut terminaux de la regle de derivation, en s'arretant aux
					// attribut du sliceset
					bOk = BuildAllUsedSliceAttributes(
					    currentDerivationRule, &odSliceAttributes, &nkdAllUsedSliceAttributes,
					    &nkdAllUsedDerivedAttributes, sErrorAttribute);
					if (not bOk)
						AddError("Variable " + attribute->GetName() + " in read dictionnary " +
							 kwcInputClass->GetName() +
							 " relies on a derivation rule involving variable " +
							 sErrorAttribute + " that is not in a slice");
				}
			}
		}
	}

	// Recherche parmi les operandes de tous les attributs necessaires
	if (bOk)
	{
		nkdAllUsedSliceAttributes.ExportObjectArray(&oaClassNeededAttributes);
		for (nAttribute = 0; nAttribute < oaClassNeededAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, oaClassNeededAttributes.GetAt(nAttribute));
			assert(odSliceAttributes.Lookup(attribute->GetName()) != NULL);

			// L'attribut doit appartenir a la classe courante
			// Il pourrait y a avoir un attribut du meme nom dans le sliceset, mais dans une sous-table par
			// exemple
			if (kwcInputClass->LookupAttribute(attribute->GetName()) == NULL)
			{
				AddError("Variable " + attribute->GetName() +
					 " used as operand not found in read dictionnary " + kwcInputClass->GetName());
				bOk = false;
				break;
			}
			// Sinon, on memorise sa tranche
			else
			{
				// On memorise sa tranche si necessaire
				slice = cast(KWDataTableSlice*, odSliceAttributes.Lookup(attribute->GetName()));
				assert(slice != NULL);
				odUsedSlices.SetAt(slice->GetClass()->GetName(), slice);
			}
		}
	}

	// On verifie enfin qu'au moins un attribut natif d'une tranche est utilise
	if (bOk)
	{
		if (odUsedSlices.GetCount() == 0)
		{
			AddError("Read dictionnary " + kwcInputClass->GetName() + " does not use any slice variable");
			bOk = false;
		}
	}
	return bOk;
}

void KWDataTableSliceSet::SetReadClass(const KWClass* kwcInputClass)
{
	read_Class = kwcInputClass;
}

const KWClass* KWDataTableSliceSet::GetReadClass() const
{
	return read_Class;
}

boolean KWDataTableSliceSet::OpenForRead()
{
	boolean bOk = true;
	int nSlice;
	KWDataTableSlice* slice;
	KWAttributeBlock* attributeBlock;
	KWAttribute* attribute;
	KWDataItem* dataItem;
	int i;

	require(read_Class != NULL);
	require(read_Class->IsCompiled());
	require(not IsOpenedForRead());

	///////////////////////////////////////////////////////////////////////////////
	// Initialisation

	// Calcul des informations necessaire a la lecture
	ComputeReadInformation(read_Class, read_PhysicalClass, &read_oaPhysicalSlices);
	read_FirstPhysicalSlice = cast(KWDataTableSlice*, read_oaPhysicalSlices.GetAt(0));

	// Parametrage d'un domaine de classe physique
	read_CurrentDomain = KWClassDomain::GetCurrentDomain();
	read_PhysicalDomain.SetName("Physical");
	KWClassDomain::SetCurrentDomain(&read_PhysicalDomain);

	// On positionne toutes les classes physiques le domaine physique
	read_PhysicalDomain.InsertClass(read_PhysicalClass);
	for (nSlice = 0; nSlice < read_oaPhysicalSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, read_oaPhysicalSlices.GetAt(nSlice));
		read_PhysicalDomain.InsertClass(slice->GetClass());
	}

	// Compilation du domaine
	assert(read_PhysicalDomain.Check());
	read_PhysicalDomain.Compile();

	///////////////////////////////////////////////////////////////////////////////
	// Preparation des traitements a faire pour la lecture, le calcul et
	// la finalisation des objets

	// Recherche des data items calcules
	for (i = 0; i < read_PhysicalClass->GetLoadedDataItemNumber(); i++)
	{
		dataItem = read_PhysicalClass->GetLoadedDataItemAt(i);
		if (dataItem->IsAttribute())
		{
			attribute = cast(KWAttribute*, dataItem);
			if (attribute->GetDerivationRule() != NULL)
				read_livDerivedDataItems.Add(attribute->GetLoadIndex());
		}
		else
		{
			attributeBlock = cast(KWAttributeBlock*, dataItem);
			if (attributeBlock->GetDerivationRule() != NULL)
				read_livDerivedDataItems.Add(attributeBlock->GetLoadIndex());
		}
	}

	// Creation et initialisation des drivers des tranches impliquees
	// Il est a note que toutes les tranches sont parametrees par la meme classe physique,
	// ce qui permettra d'un avoir un seul objet dont l'alimentation sera completee au fur et
	// a mesure par les driver de chaque tranche.
	// Dans le cas des blocs decoupes sur plusieurs tranches, les SparseIndex sont les memes
	// pour toutes les tranches, et chaque tranche ne lit que les attributs la concernant
	// (possible, car ce sont les VarKey et non les SparseIndex qui sont dans les fichiers).
	// Au cours des lectures, les blocs seront concatenes, et une derniere passe de tri
	// des bloc par SparseIndex sera necessaire pour obtenir des blocs de valeurs valides
	for (nSlice = 0; nSlice < read_oaPhysicalSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, read_oaPhysicalSlices.GetAt(nSlice));
		bOk = bOk and slice->PhysicalOpenForRead(read_PhysicalClass);
	}

	// Fermeture en cas de probleme
	if (not bOk)
		Close();
	return bOk;
}

boolean KWDataTableSliceSet::IsOpenedForRead() const
{
	require(read_Class != NULL);
	return read_FirstPhysicalSlice != NULL and read_FirstPhysicalSlice->IsOpenedForRead();
}

boolean KWDataTableSliceSet::IsEnd() const
{
	require(IsOpenedForRead());
	return read_FirstPhysicalSlice->IsEnd();
}

KWObject* KWDataTableSliceSet::Read()
{
	require(IsOpenedForRead());
	return PhysicalRead();
}

void KWDataTableSliceSet::Skip()
{
	int nSlice;
	KWDataTableSlice* slice;

	require(IsOpenedForRead());

	// Saut d 'une ligne dans toutes les slices
	for (nSlice = 0; nSlice < read_oaPhysicalSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, read_oaPhysicalSlices.GetAt(nSlice));
		slice->Skip();
	}
}

void KWDataTableSliceSet::SkipMultiple(int nSkipNumber)
{
	int nNewDataFileIndex;
	int nNewDataFileRecordIndex;
	int nRemainingSkipNumber;
	int nSlice;
	KWDataTableSlice* slice;
	int i;

	require(IsOpenedForRead());
	require(read_FirstPhysicalSlice->read_nDataFileIndex < GetChunkNumber());
	require(0 <= nSkipNumber and nSkipNumber <= GetTotalInstanceNumber());

	// Initialisation des index  de chunk et de record dans le chunk a atteindre a partir de la position courante
	nNewDataFileIndex = read_FirstPhysicalSlice->read_nDataFileIndex;
	assert(read_FirstPhysicalSlice->read_SliceDataTableDriver->GetRecordIndex() <= INT_MAX);
	nNewDataFileRecordIndex = (int)read_FirstPhysicalSlice->read_SliceDataTableDriver->GetRecordIndex();

	// Calcul de l'index a atteindre
	nRemainingSkipNumber = nSkipNumber;
	while (nRemainingSkipNumber > 0)
	{
		assert(nNewDataFileIndex < GetChunkNumber());
		if (nRemainingSkipNumber >=
		    GetChunkInstanceNumbers()->GetAt(nNewDataFileIndex) - nNewDataFileRecordIndex)
		{
			nRemainingSkipNumber -=
			    GetChunkInstanceNumbers()->GetAt(nNewDataFileIndex) - nNewDataFileRecordIndex;
			nNewDataFileIndex++;
			nNewDataFileRecordIndex = 0;
		}
		else
		{
			nNewDataFileRecordIndex += nRemainingSkipNumber;
			nRemainingSkipNumber = 0;
		}
	}

	// Avancement de l'index jusqu'au prochain tranche non vide
	while (GetChunkInstanceNumbers()->GetAt(nNewDataFileIndex) == 0 and nNewDataFileIndex < GetChunkNumber())
		nNewDataFileIndex++;

	assert((nNewDataFileIndex == GetChunkNumber() and nNewDataFileRecordIndex == 0) or
	       nNewDataFileRecordIndex < GetChunkInstanceNumbers()->GetAt(nNewDataFileIndex));

	// Cas ou on reste dans le meme chunk qu'au depart
	if (nNewDataFileIndex == read_FirstPhysicalSlice->read_nDataFileIndex)
	{
		// On se contente de se deplacer dans le chunk
		for (i = 0; i < nSkipNumber; i++)
			Skip();
	}
	// Cas ou on change de chunk
	else
	{
		// Changement de chunk dans toutes les slices
		for (nSlice = 0; nSlice < read_oaPhysicalSlices.GetSize(); nSlice++)
		{
			slice = cast(KWDataTableSlice*, read_oaPhysicalSlices.GetAt(nSlice));

			// Fermeture si necessaire du chunk de depart
			if (slice->read_SliceDataTableDriver->IsOpenedForRead())
				slice->PhysicalCloseCurrentChunk();

			// Parametrage du prochain chunk en lecture
			slice->read_nDataFileIndex = nNewDataFileIndex;
		}

		// On se deplace dans le nouveau chunk
		for (i = 0; i < nNewDataFileRecordIndex; i++)
			Skip();
	}
	assert(nNewDataFileIndex == read_FirstPhysicalSlice->read_nDataFileIndex);
	assert(nNewDataFileRecordIndex == read_FirstPhysicalSlice->read_SliceDataTableDriver->GetRecordIndex());
}

boolean KWDataTableSliceSet::IsError() const
{
	require(IsOpenedForRead());
	return read_FirstPhysicalSlice->IsError();
}

double KWDataTableSliceSet::GetReadPercentage()
{
	require(IsOpenedForRead());
	return read_FirstPhysicalSlice->GetReadPercentage();
}

int KWDataTableSliceSet::GetChunkIndex() const
{
	require(IsOpenedForRead());
	return read_FirstPhysicalSlice->GetChunkIndex();
}

boolean KWDataTableSliceSet::Close()
{
	int nSlice;
	KWDataTableSlice* slice;

	require(IsOpenedForRead());

	// Fermeture de toutes les tranches
	for (nSlice = 0; nSlice < read_oaPhysicalSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, read_oaPhysicalSlices.GetAt(nSlice));
		if (slice->IsOpenedForRead())
			slice->Close();
	}

	// Nettoyage du domaine et restitution du domaine courant
	read_PhysicalDomain.RemoveAllClasses();
	KWClassDomain::SetCurrentDomain(read_CurrentDomain);

	// On repasse les attributs des tranches impacte en Loaded
	for (nSlice = 0; nSlice < read_oaPhysicalSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, read_oaPhysicalSlices.GetAt(nSlice));
		slice->GetClass()->SetAllAttributesLoaded(true);
		slice->GetClass()->IndexClass();
	}

	// Nettoyage
	delete read_PhysicalClass;
	read_PhysicalClass = NULL;
	read_oaPhysicalSlices.RemoveAll();
	read_CurrentDomain = NULL;
	read_FirstPhysicalSlice = NULL;
	read_livDerivedDataItems.SetSize(0);
	return true;
}

boolean KWDataTableSliceSet::ReadAllObjectsWithClass(const KWClass* kwcInputClass, ObjectArray* oaReadObjects)
{
	boolean bOk = true;
	PeriodicTest periodicTestInterruption;
	KWObject* kwoObject;
	longint lRecordNumber;
	ALString sTmp;

	require(read_Class == NULL);
	require(kwcInputClass != NULL);
	require(kwcInputClass->IsCompiled());
	require(oaReadObjects != NULL);
	require(oaReadObjects->GetSize() == 0);

	// Demarrage des serveurs de fichiers : les slices peuvent etre sur des machines distantes
	if (GetProcessId() == 0)
		PLParallelTask::GetDriver()->StartFileServers();

	// Parametrage temporaire de la classe de lecture
	read_Class = kwcInputClass;

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Read data table slice set " + GetReadClass()->GetName());

	// Ouverture de la base en lecture
	bOk = OpenForRead();

	// Lecture d'objets dans la base
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		lRecordNumber = 0;
		while (not IsEnd())
		{
			kwoObject = Read();
			lRecordNumber++;

			// Ajout de l'objet au tableau
			if (kwoObject != NULL)
				oaReadObjects->Add(kwoObject);
			// Arret sinon
			else
			{
				bOk = false;
				break;
			}

			// Suivi de la tache
			if (periodicTestInterruption.IsTestAllowed(lRecordNumber))
				TaskProgression::DisplayProgression((int)(100 * GetReadPercentage()));
		}
		Global::DesactivateErrorFlowControl();

		// Test si interruption sans qu'il y ait d'erreur
		if (IsError() or TaskProgression::IsInterruptionRequested())
		{
			bOk = false;

			// Warning ou erreur selon le cas
			if (IsError())
				Object::AddError("Read using dictionary " + read_Class->GetName() +
						 " interrupted because of errors");
			else
				Object::AddWarning("Read using dictionary " + read_Class->GetName() +
						   " interrupted by user");
		}

		// Fermeture
		bOk = Close() and bOk;
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();

	// Verification du nombre total d'instances
	if (bOk and GetTotalInstanceNumber() != oaReadObjects->GetSize())
	{
		bOk = false;
		Object::AddError(sTmp + "Read " + IntToString(oaReadObjects->GetSize()) + " objects using dictionary " +
				 read_Class->GetName() + " instead of " + LongintToString(GetTotalInstanceNumber()) +
				 " expected objects");
	}

	// Arret des serveurs de fichiers
	if (GetProcessId() == 0)
		PLParallelTask::GetDriver()->StopFileServers();

	// Nettoyage si necessaire
	if (not bOk)
		oaReadObjects->DeleteAll();

	// Nettoyage
	read_Class = NULL;
	return bOk;
}

KWClass* KWDataTableSliceSet::BuildClassFromAttributeNames(const ALString& sInputClassName,
							   const StringVector* svInputAttributeNames)
{
	KWClass* kwcNewClass;
	ObjectDictionary odSliceAttributes;
	ObjectArray oaInputAttributes;
	ObjectArray oaAttributeBlocks;
	KWAttribute* attribute;
	KWAttribute* nextAttribute;
	KWAttribute* blockFirstAttribute;
	KWAttribute* newBlockFirstAttribute;
	KWAttribute* newAttribute;
	KWAttributeBlock* newAttributeBlock;
	int i;

	require(sInputClassName != "");
	require(svInputAttributeNames != NULL);
	require(svInputAttributeNames->GetSize() > 0);

	// Memoirsation du nom de la classe
	sClassName = sInputClassName;

	// Creation de la classe
	kwcNewClass = new KWClass;
	kwcNewClass->SetName(sInputClassName);

	// Alimentation du dictionnaire des attributs des tranches
	FillAttributes(&odSliceAttributes);

	// Recherche de tous les attributs dans les classes des tranches
	for (i = 0; i < svInputAttributeNames->GetSize(); i++)
	{
		attribute = cast(KWAttribute*, odSliceAttributes.Lookup(svInputAttributeNames->GetAt(i)));
		check(attribute);
		oaInputAttributes.Add(attribute);
	}

	// Tri des attributs par nom de bloc, puis par nom d'attributs
	oaInputAttributes.SetCompareFunction(KWAttributeCompareBlockName);
	oaInputAttributes.Sort();

	// Creation des attributs dans la nouvelle classe
	blockFirstAttribute = NULL;
	newBlockFirstAttribute = NULL;
	for (i = 0; i < oaInputAttributes.GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaInputAttributes.GetAt(i));
		assert(attribute->GetAttributeBlock() == NULL or
		       attribute->GetAttributeBlock()->GetDerivationRule() == NULL);

		// Creation du nouvel attribut
		newAttribute = attribute->Clone();
		kwcNewClass->InsertAttribute(newAttribute);

		// Actualisation si necessaire du premier attribut de bloc
		if (blockFirstAttribute == NULL and attribute->GetAttributeBlock() != NULL)
		{
			blockFirstAttribute = attribute;
			newBlockFirstAttribute = newAttribute;
		}

		// Acces a l'attribut suivant
		if (i == oaInputAttributes.GetSize() - 1)
			nextAttribute = NULL;
		else
			nextAttribute = cast(KWAttribute*, oaInputAttributes.GetAt(i + 1));

		// Creation si necessaire d'un nouveau bloc
		if (blockFirstAttribute != NULL)
		{
			// Les attributs peuvent provenir de deux tranche differentes, mais avoir ete initialement dans
			// le meme bloc, ce que l'on peut retrouver si leur nom de bloc est identique
			if (nextAttribute == NULL or nextAttribute->GetAttributeBlock() == NULL or
			    blockFirstAttribute->GetAttributeBlock()->GetName() !=
				nextAttribute->GetAttributeBlock()->GetName())
			{
				// Creation du nouveau bloc
				newAttributeBlock = kwcNewClass->CreateAttributeBlock(
				    blockFirstAttribute->GetAttributeBlock()->GetName(), newBlockFirstAttribute,
				    newAttribute);
				newAttributeBlock->ImportMetaDataFrom(blockFirstAttribute->GetAttributeBlock());

				// Reinitialisation a NULL du premier attribut de bloc
				blockFirstAttribute = NULL;
				newBlockFirstAttribute = NULL;
			}
		}
	}
	return kwcNewClass;
}

ObjectArray* KWDataTableSliceSet::GetSlices()
{
	return &oaSlices;
}

void KWDataTableSliceSet::DeleteAllSliceFiles()
{
	int nSlice;
	KWDataTableSlice* slice;
	PLFileConcatenater concatenater;

	// Supression des fichier pour toutes les tranches
	if (TaskProgression::IsInTask())
		concatenater.SetDisplayProgression(true);
	for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

		// Destruction des fichiers de la tranche
		concatenater.SetProgressionBegin(nSlice * 1.0 / oaSlices.GetSize());
		concatenater.SetProgressionEnd((nSlice + 1) * 1.0 / oaSlices.GetSize());
		concatenater.RemoveChunks(slice->GetDataFileNames());

		// Supression de la specification des fichiers
		slice->GetDataFileNames()->SetSize(0);
		slice->GetDataFileSizes()->SetSize(0);

		// Supression des vacteurs de statistiques sur les valeurs
		slice->lvAttributeBlockValueNumbers.SetSize(0);
		slice->lvDenseSymbolAttributeDiskSizes.SetSize(0);
	}

	// Suppression des nombres d'instances par chunk
	ivChunkInstanceNumbers.SetSize(0);
	nTotalInstanceNumber = 0;
}

void KWDataTableSliceSet::FillAttributes(ObjectDictionary* odAttributes) const
{
	int nSlice;
	KWDataTableSlice* slice;
	KWAttribute* sliceAttribute;

	require(odAttributes != NULL);
	require(odAttributes->GetCount() == 0);

	// Collecte des attributs des tranches
	for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

		// Collecte des attributs utilises de la tranche
		sliceAttribute = slice->GetClass()->GetHeadAttribute();
		while (sliceAttribute != NULL)
		{
			if (sliceAttribute->GetUsed())
				odAttributes->SetAt(sliceAttribute->GetName(), sliceAttribute);
			slice->GetClass()->GetNextAttribute(sliceAttribute);
		}
	}
}

void KWDataTableSliceSet::FillSliceAttributes(ObjectDictionary* odSliceAttributes) const
{
	int nSlice;
	KWDataTableSlice* slice;
	KWAttribute* sliceAttribute;

	require(odSliceAttributes != NULL);
	require(odSliceAttributes->GetCount() == 0);

	// Collecte des attributs des tranches
	for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

		// Collecte des attributs utilises de la tranche
		sliceAttribute = slice->GetClass()->GetHeadAttribute();
		while (sliceAttribute != NULL)
		{
			if (sliceAttribute->GetUsed())
				odSliceAttributes->SetAt(sliceAttribute->GetName(), slice);
			slice->GetClass()->GetNextAttribute(sliceAttribute);
		}
	}
}

void KWDataTableSliceSet::SliceDictionariesWrite(ostream& ost) const
{
	int nSlice;
	KWDataTableSlice* slice;
	KWClassDomain exportClassDomain;

	// Initialisation du domaine avec les dictionnaire des tranches
	exportClassDomain.SetName(GetClassLabel());
	for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		exportClassDomain.InsertClass(slice->GetClass());
	}

	// Export vers le fichier
	exportClassDomain.Write(ost);

	// Nettoyage
	exportClassDomain.RemoveAllClasses();
}

void KWDataTableSliceSet::SliceDictionariesWriteFile(const ALString& sFileName) const
{
	int nSlice;
	KWDataTableSlice* slice;
	KWClassDomain exportClassDomain;

	// Initialisation du domaine avec les dictionnaire des tranches
	exportClassDomain.SetName(GetClassLabel());
	for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		exportClassDomain.InsertClass(slice->GetClass());
	}

	// Export vers le fichier
	exportClassDomain.WriteFile(sFileName);

	// Nettoyage
	exportClassDomain.RemoveAllClasses();
}

boolean KWDataTableSliceSet::Check() const
{
	boolean bOk = true;
	KWClass* kwcClass;
	int nSlice;
	KWDataTableSlice* slice;
	ObjectDictionary odSliceAttributes;
	ObjectDictionary odSliceDataFileNames; // URI
	ObjectDictionary odSliceBlockPreviousAttributes;
	int nAttribute;
	KWAttribute* attribute;
	KWAttribute* sliceAttribute;
	KWAttribute* sliceBlockPreviousAttribute;
	int nCheckedAttributeNumber;
	int nFile;
	ALString sDataFileName;

	// Verification de base
	kwcClass = NULL;
	if (GetClassName() != "")
	{
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
		bOk = bOk and kwcClass != NULL;
		bOk = bOk and kwcClass->IsCompiled();
		bOk = bOk and (GetUsedSimpleAttributeNumber() == 0 or GetSliceNumber() > 0);
	}

	// Verification des tranches et collecte des attributs des tranches
	if (bOk)
	{
		for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
		{
			slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

			// Verification de la tranche elle meme
			bOk = bOk and slice->Check();
			if (not bOk)
				break;

			// Verification de l'ordre des tranches
			if (nSlice > 0)
				bOk = bOk and cast(KWDataTableSlice*, oaSlices.GetAt(nSlice - 1))
						      ->CompareLexicographicOrder(slice) < 0;

			// Collecte et verification des attributs de la tranche
			for (nAttribute = 0; nAttribute < slice->GetClass()->GetUsedAttributeNumber(); nAttribute++)
			{
				sliceAttribute = slice->GetClass()->GetUsedAttributeAt(nAttribute);

				// L'attribut doit etre unique parmi les tranches
				bOk = bOk and odSliceAttributes.Lookup(sliceAttribute->GetName()) == NULL;

				// Il doit exister dans le dictionnaire principal
				if (kwcClass != NULL)
					bOk = bOk and kwcClass->LookupAttribute(sliceAttribute->GetName()) != NULL;

				// On le memorise si ok
				if (bOk)
					odSliceAttributes.SetAt(sliceAttribute->GetName(), sliceAttribute);
				else
					break;
			}
			if (not bOk)
				break;

			// Collecte et verification des fichiers de donnees des tranches
			assert(slice->GetDataFileNames()->GetSize() == slice->GetDataFileSizes()->GetSize());
			for (nFile = 0; nFile < slice->GetDataFileNames()->GetSize(); nFile++)
			{
				sDataFileName = slice->GetDataFileNames()->GetAt(nFile);

				// Le fichier doit etre unique parmi les tranches
				bOk = bOk and odSliceDataFileNames.Lookup(sDataFileName) == NULL;

				// On le memorise si ok
				if (bOk)
					odSliceDataFileNames.SetAt(sDataFileName, slice);
				else
					break;
			}
			bOk = bOk and slice->GetDataFileNames()->GetSize() ==
					  cast(KWDataTableSlice*, oaSlices.GetAt(0))->GetDataFileNames()->GetSize();
		}
	}

	// Verification que chaque attribut du dictionnaire principal est present dans une tranche
	if (bOk and kwcClass != NULL)
	{
		nCheckedAttributeNumber = 0;
		for (nAttribute = 0; nAttribute < kwcClass->GetUsedAttributeNumber(); nAttribute++)
		{
			attribute = kwcClass->GetUsedAttributeAt(nAttribute);

			// Prise en compte des attributs concernes
			if (KWType::IsSimple(attribute->GetType()) and attribute->GetName() != GetTargetAttributeName())
			{
				// Recherche de l'attribut de tranche correspondant, et verifications de type et de bloc
				sliceAttribute = cast(KWAttribute*, odSliceAttributes.Lookup(attribute->GetName()));
				bOk = bOk and (sliceAttribute != NULL or attribute->GetDerivationRule() != NULL);
				if (sliceAttribute != NULL)
				{
					nCheckedAttributeNumber++;
					bOk = bOk and (attribute->GetType() == sliceAttribute->GetType());
					bOk = bOk and (attribute->IsInBlock() == sliceAttribute->IsInBlock());
					bOk = bOk and (not attribute->IsInBlock() or
						       attribute->GetAttributeBlock()->GetName() ==
							   sliceAttribute->GetAttributeBlock()->GetName());
				}
				if (not bOk)
					break;
			}
		}
		bOk = bOk and nCheckedAttributeNumber <= odSliceAttributes.GetCount();
	}

	// Verification que les attributs de chaque blocs se trouvent par plages contigues dans des tranches
	// dont l'ordre est le meme que celui des plages d'attributs
	// Verification des tranches et collecte des attributs des tranches
	if (bOk)
	{
		// Parcours des tranches
		for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
		{
			slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

			// Parcours des attributs de la tranche
			for (nAttribute = 0; nAttribute < slice->GetClass()->GetUsedAttributeNumber(); nAttribute++)
			{
				sliceAttribute = slice->GetClass()->GetUsedAttributeAt(nAttribute);

				// Verification pour les attributs dans les blocs
				if (sliceAttribute->IsInBlock())
				{
					// Recherche du precedent attribut present dans un bloc de meme nom, dans la eme
					// tranche ou non
					sliceBlockPreviousAttribute =
					    cast(KWAttribute*, odSliceBlockPreviousAttributes.Lookup(
								   sliceAttribute->GetAttributeBlock()->GetName()));

					// Verification si un attribut precedent existe pour le meme bloc
					if (sliceBlockPreviousAttribute != NULL)
					{
						// Il doit alors avoir un VarKey plus petite
						bOk = bOk and (KWAttributeCompareVarKey(&sliceBlockPreviousAttribute,
											&sliceAttribute) < 0);
						if (not bOk)
							break;
					}

					// Memorisation de l'attribut pour cette tanche
					odSliceBlockPreviousAttributes.SetAt(
					    sliceAttribute->GetAttributeBlock()->GetName(), sliceAttribute);
				}
			}
			if (not bOk)
				break;
		}
	}
	return bOk;
}

void KWDataTableSliceSet::CopyFrom(const KWDataTableSliceSet* aSource)
{
	int i;
	KWDataTableSlice* aSlice;

	require(aSource != NULL);

	Clean();
	sClassName = aSource->sClassName;
	sClassTargetAttributeName = aSource->sClassTargetAttributeName;
	nTotalInstanceNumber = aSource->nTotalInstanceNumber;
	ivChunkInstanceNumbers.CopyFrom(&aSource->ivChunkInstanceNumbers);
	oaSlices.SetSize(aSource->oaSlices.GetSize());
	for (i = 0; i < oaSlices.GetSize(); i++)
	{
		aSlice = cast(KWDataTableSlice*, aSource->oaSlices.GetAt(i));
		oaSlices.SetAt(i, aSlice->Clone());
	}
}

KWDataTableSliceSet* KWDataTableSliceSet::Clone() const
{
	KWDataTableSliceSet* clone;
	clone = new KWDataTableSliceSet;
	clone->CopyFrom(this);
	return clone;
}

longint KWDataTableSliceSet::GetUsedMemory() const
{
	longint lUsedMemory;
	int nIndex;

	lUsedMemory = sizeof(KWDataTableSliceSet);
	lUsedMemory += sClassName.GetLength();
	lUsedMemory += sClassTargetAttributeName.GetLength();
	for (nIndex = 0; nIndex < GetSliceNumber(); nIndex++)
		lUsedMemory += oaSlices.GetAt(nIndex)->GetUsedMemory();
	return lUsedMemory;
}

void KWDataTableSliceSet::Write(ostream& ost) const
{
	int nSlice;
	int nChunk;
	longint lTotalChunkFileSize;
	int nInstanceNumber;
	KWDataTableSlice* dataTableSlice;

	// Entete
	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\n";

	// Nombre d'attributs, d'attributs denses etd e blocs d'attributs
	ost << "Variables\t" << GetTotalAttributeNumber() << "\n";
	ost << "Dense variables\t" << GetTotalDenseAttributeNumber() << "\n";
	ost << "Variable blocks\t" << GetTotalAttributeBlockNumber() << "\n";

	// Nombre d'instances
	ost << "Instances\t" << GetTotalInstanceNumber() << "\n";

	// Decoupage en slices et chunks
	ost << "Slices\t" << GetSliceNumber() << "\n";
	ost << "Chunks\t" << GetChunkNumber() << "\n";

	// Nombre total des nombres de valeur par blocs d'attribut
	ost << "Total block value number\t" << GetTotalAttributeBlockValueNumber() << "\n";

	// Taille totale occupee sur disque par les attributs denses Symbol
	ost << "Total dense cat variable disk size\t" << GetTotalDenseSymbolAttributeDiskSize() << "\n";

	// Taille totale des fichiers
	ost << "Total data file size\t" << GetTotalDataFileSize() << "\n";

	// Descriptif detaille par tranches
	ost << "Slice\tDictionary\tVars\tDense vars\tVar blocks\t"
	    << "Max block size\tMax block values\tTotal block values\t"
	    << "Cat vars le disk size\tData file size\n";
	for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
	{
		dataTableSlice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		ost << nSlice + 1 << "\t";
		ost << dataTableSlice->GetClass()->GetName() << "\t";
		ost << dataTableSlice->GetClass()->GetUsedAttributeNumber() << "\t";
		ost << dataTableSlice->GetClass()->GetLoadedDenseAttributeNumber() << "\t";
		ost << dataTableSlice->GetClass()->GetLoadedAttributeBlockNumber() << "\t";
		ost << dataTableSlice->GetMaxBlockAttributeNumber() << "\t";
		ost << dataTableSlice->GetMaxAttributeBlockValueNumber() << "\t";
		ost << dataTableSlice->GetTotalAttributeBlockValueNumber() << "\t";
		ost << dataTableSlice->GetTotalDenseSymbolAttributeDiskSize() << "\t";
		ost << dataTableSlice->GetTotalDataFileSize() << "\n";
	}

	// Descriptif detaille par chunk
	ost << "Chunk\tInstances\tTotal instances\tData file size\n";
	nInstanceNumber = 0;
	for (nChunk = 0; nChunk < GetChunkNumber(); nChunk++)
	{
		// Calcul de la taille totale des fichiers de chunk
		lTotalChunkFileSize = 0;
		for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
		{
			dataTableSlice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
			lTotalChunkFileSize += dataTableSlice->GetDataFileSizes()->GetAt(nChunk);
		}

		// Affichage
		ost << nChunk + 1 << "\t";
		ost << ivChunkInstanceNumbers.GetAt(nChunk) << "\t";
		ost << nInstanceNumber << "\t";
		ost << lTotalChunkFileSize << "\n";

		// Mise a jour du nombre total d'instances lus
		nInstanceNumber += ivChunkInstanceNumbers.GetAt(nChunk);
	}
}

const ALString KWDataTableSliceSet::GetClassLabel() const
{
	return "Data table slice set";
}

const ALString KWDataTableSliceSet::GetObjectLabel() const
{
	return sClassName;
}

KWDataTableSliceSet* KWDataTableSliceSet::CreateDataTableSliceSet(const KWClass* kwcClassToPartition,
								  const ALString& sTargetAttributeName,
								  int nObjectNumber, int nMaxAttributeNumberPerSlice,
								  int nChunkNumber)
{
	boolean bOk;
	KWDataTableSliceSet* dataTableSliceSet;
	KWDataTableSlice* dataTableSlice;
	KWClassDomain* currentDomain;
	KWClassDomain testClassDomain;
	int nSlice;
	int nChunk;
	ALString sSliceBaseName;
	ALString sSliceFileName;
	KWSTDatabaseTextFile databaseTextFile;
	KWObject* kwoObject;
	int nObject;
	int nSliceObjectNumber;
	int i;
	ALString sTmp;

	require(kwcClassToPartition != NULL);
	require(nMaxAttributeNumberPerSlice > 0);
	require(nChunkNumber > 0);
	require(nObjectNumber > 0);

	// Creation de la table
	dataTableSliceSet = new KWDataTableSliceSet;

	// Calcul des specifications de la table
	dataTableSliceSet->ComputeSpecification(kwcClassToPartition, sTargetAttributeName, nObjectNumber,
						nMaxAttributeNumberPerSlice);
	dataTableSliceSet->ComputeSlicesLoadIndexes(kwcClassToPartition);

	// Enregistrement d'un domaine de classe le temps de la cretaion de la table de test
	currentDomain = KWClassDomain::GetCurrentDomain();
	KWClassDomain::SetCurrentDomain(&testClassDomain);

	// Creation des objets dans chaque tranche
	bOk = true;
	for (nSlice = 0; nSlice < dataTableSliceSet->GetSliceNumber(); nSlice++)
	{
		dataTableSlice = dataTableSliceSet->GetSliceAt(nSlice);

		// Enregistrement de la classe dans le domaine
		assert(dataTableSlice->GetClass()->GetDomain() == NULL);
		testClassDomain.InsertClass(dataTableSlice->GetClass());
		dataTableSlice->GetClass()->Compile();

		// Creation des chunks
		nObject = 0;
		for (nChunk = 0; nChunk < nChunkNumber; nChunk++)
		{
			// Creation d'un fichier, dans le repertoire temporaire
			sSliceBaseName = sTmp + "Slice_" + IntToString(nChunk + 1) + "_" + IntToString(nSlice + 1) +
					 "-" + kwcClassToPartition->GetName() + ".txt";
			sSliceFileName = FileService::CreateUniqueTmpFile(sSliceBaseName, dataTableSliceSet);

			// Test d'erreur
			bOk = sSliceFileName != "";
			if (not bOk)
			{
				dataTableSliceSet->AddError("Unable to create file " + sSliceFileName);
				break;
			}

			// Memorisation de ce nom de fichier
			dataTableSlice->GetDataFileNames()->Add(FileService::BuildLocalURI(sSliceFileName));

			// Initialisation d'une base pour ce fichier
			databaseTextFile.SetDatabaseName(sSliceFileName);
			databaseTextFile.SetClassName(dataTableSlice->GetClass()->GetName());
			databaseTextFile.SetHeaderLineUsed(false);
			databaseTextFile.SetSilentMode(true);

			// Creation des objets dans la base
			nSliceObjectNumber = ((nObjectNumber - nObject) / (nChunkNumber - nChunk));
			for (i = 0; i < nSliceObjectNumber; i++)
			{
				kwoObject = KWObject::CreateObject(dataTableSlice->GetClass(), nObject + 1);
				nObject++;
				databaseTextFile.GetObjects()->Add(kwoObject);
			}

			// Ecriture de la base
			databaseTextFile.WriteAll(&databaseTextFile);

			// Memorisation de la taille du de fichier
			dataTableSlice->GetDataFileSizes()->Add(FileService::GetFileSize(sSliceFileName));

			// Nettoyage de la base
			databaseTextFile.DeleteAll();
		}

		// Nettoyage du domaine
		testClassDomain.RemoveClass(dataTableSlice->GetClass()->GetName());
		if (not bOk)
			break;
	}

	// Restitution du  domaine de classe courant
	KWClassDomain::SetCurrentDomain(currentDomain);

	// Nettoyage des index de chargement
	dataTableSliceSet->CleanSlicesLoadIndexes();

	// Nettoyage des fichiers si erreur
	if (not bOk)
		dataTableSliceSet->DeleteAllSliceFiles();

	// Retour de l'objet cree
	return dataTableSliceSet;
}

void KWDataTableSliceSet::TestReadDataTableSliceSet(const ALString& sTestLabel,
						    KWDataTableSliceSet* inputDataTableSliceSet,
						    const KWClass* kwcInputClass)
{
	boolean bDisplay = true;
	boolean bOk;
	KWSTDatabaseTextFile databaseTextFile;
	ALString sTestDatabaseName;
	int i;
	KWObject* kwoObject;

	require(inputDataTableSliceSet != NULL);
	require(kwcInputClass != NULL);

	// Affichage
	if (bDisplay)
	{
		cout << "==========================================================\n";
		cout << "TestReadDataTableSliceSet " << sTestLabel << endl;
		cout << "==========================================================\n";
		cout << "Input dictionary\n" << *kwcInputClass << endl;
	}

	// Verification de la validite de la classe de lecture
	bOk = inputDataTableSliceSet->CheckReadClass(kwcInputClass);
	cout << sTestLabel << " check:" << bOk << endl;

	// Calcul des informations de lecture
	if (bOk)
	{
		// Creation d'un fichier temporaire pour ecrire la base
		sTestDatabaseName = FileService::CreateUniqueTmpFile(sTestLabel + ".txt", inputDataTableSliceSet);

		// Parametrage d'une base au format texte
		databaseTextFile.SetClassName(kwcInputClass->GetName());
		databaseTextFile.SetDatabaseName(sTestDatabaseName);

		// Lecture des objets
		bOk = inputDataTableSliceSet->ReadAllObjectsWithClass(kwcInputClass, databaseTextFile.GetObjects());
		cout << sTestLabel << " read:" << bOk << "; " << databaseTextFile.GetObjects()->GetSize()
		     << " objects read" << endl;

		// Ecriture des objets
		databaseTextFile.WriteAll(&databaseTextFile);

		// Affichage des premiers objets
		if (bDisplay)
		{
			cout << "Database " << databaseTextFile.GetDatabaseName() << ": "
			     << databaseTextFile.GetObjects()->GetSize() << endl;
			for (i = 0; i < databaseTextFile.GetObjects()->GetSize(); i++)
			{
				kwoObject = cast(KWObject*, databaseTextFile.GetObjects()->GetAt(i));
				cout << *kwoObject << endl;
				if (i == 2)
				{
					cout << "...\n";
					break;
				}
			}
		}

		// Nettoyage des objets crees
		databaseTextFile.GetObjects()->DeleteAll();
	}
}

void KWDataTableSliceSet::Test()
{
	boolean bDisplay = true;
	KWDataTableSliceSet* testSliceSet;
	ALString sTestClassName;
	KWClass* kwcTestClass;
	KWClass* kwcTestInputClass;
	const int nAttributeNumber = 32;
	const int nChunkNumber = 4;
	const int nSliceNumber = 9;
	const int nObjectNumber = 1000;
	int nAttribute;
	KWAttribute* initialAttribute;
	KWAttribute* attribute;
	KWDerivationRule* rule;

	// Creation d'une classe
	sTestClassName = KWClassDomain::GetCurrentDomain()->BuildClassName("TestClass");
	kwcTestClass = KWClass::CreateClass(sTestClassName, 0, nAttributeNumber / 2, nAttributeNumber / 2, 0, 0, 0, 0,
					    0, 0, true, NULL);
	KWClassDomain::GetCurrentDomain()->InsertClass(kwcTestClass);
	kwcTestClass->Compile();

	// Creation de la table
	testSliceSet =
	    CreateDataTableSliceSet(kwcTestClass, "", nObjectNumber, nAttributeNumber / nSliceNumber, nChunkNumber);

	// Affichage
	if (bDisplay)
	{
		cout << "Dictionary\n" << *kwcTestClass << endl;
		cout << "Slice set\n" << *testSliceSet << endl;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Cas d'une lecture d'une seule tranche

	// Creation d'un classe de lecture a partir de la table complete
	kwcTestInputClass = testSliceSet->GetSliceAt(0)->GetClass()->Clone();
	KWClassDomain::GetCurrentDomain()->InsertClassWithNewName(
	    kwcTestInputClass, KWClassDomain::GetCurrentDomain()->BuildClassName("TestInputClass"));
	kwcTestInputClass->Compile();

	// Test de lecture avec cette classe
	TestReadDataTableSliceSet("Check first slice dictionary", testSliceSet, kwcTestInputClass);

	// Creation d'un classe de lecture a partir de la table complete
	kwcTestInputClass = testSliceSet->GetSliceAt(testSliceSet->GetSliceNumber() - 1)->GetClass()->Clone();
	KWClassDomain::GetCurrentDomain()->InsertClassWithNewName(
	    kwcTestInputClass, KWClassDomain::GetCurrentDomain()->BuildClassName("TestInputClass"));
	kwcTestInputClass->Compile();

	// Test de lecture avec cette classe
	TestReadDataTableSliceSet("Check last slice dictionary", testSliceSet, kwcTestInputClass);

	////////////////////////////////////////////////////////////////////////////////
	// Cas d'une lecture complete

	// Creation d'un classe de lecture a partir de la table complete
	kwcTestInputClass = kwcTestClass->Clone();
	KWClassDomain::GetCurrentDomain()->InsertClassWithNewName(
	    kwcTestInputClass, KWClassDomain::GetCurrentDomain()->BuildClassName("TestInputClass"));
	kwcTestInputClass->Compile();

	// Test de lecture avec cette classe
	TestReadDataTableSliceSet("Check all slices dictionary", testSliceSet, kwcTestInputClass);

	////////////////////////////////////////////////////////////////////////////////
	// Cas d'une lecture avec classe erroneee

	// Creation du base erronee, avec attribut natif non dans la table
	attribute = kwcTestInputClass->GetLoadedAttributeAt(0)->Clone();
	attribute->SetName(kwcTestInputClass->BuildAttributeName("New" + attribute->GetName()));
	kwcTestInputClass->InsertAttribute(attribute);
	kwcTestInputClass->Compile();

	// Test de lecture avec cette classe
	TestReadDataTableSliceSet("Check wrong dictionary", testSliceSet, kwcTestInputClass);

	// Supression de l'attribut ajoute
	kwcTestInputClass->DeleteAttribute(attribute->GetName());

	////////////////////////////////////////////////////////////////////////////////
	// Cas d'une lecture avec dictionnaire transforme de facon complexe

	// Transformation en classe complexe
	//  . sous partie des attribut natifs, dont certains devient calcules
	//  . ajout de nouveau attributs calcules
	for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
	{
		initialAttribute = kwcTestClass->GetUsedAttributeAt(nAttribute);

		// Supression de deux attributs sur trois
		if (nAttribute % 9 > 3)
			kwcTestInputClass->DeleteAttribute(initialAttribute->GetName());
		// Transformation sinon
		else
		{
			attribute = kwcTestInputClass->LookupAttribute(initialAttribute->GetName());
			assert(attribute->GetDerivationRule() == NULL);

			// On ajoute une regle de derivation dans 1/9 des cas
			if (nAttribute % 9 == 0 and not attribute->IsInBlock())
			{
				// Cas numerique
				if (attribute->GetType() == KWType::Continuous)
				{
					rule = new KWDRCopyContinuous;
					rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
					rule->GetFirstOperand()->SetContinuousConstant(0);
					attribute->SetDerivationRule(rule);
				}
				// Cas categoriel
				else if (attribute->GetType() == KWType::Symbol)
				{
					rule = new KWDRCopySymbol;
					rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
					rule->GetFirstOperand()->SetSymbolConstant("");
					attribute->SetDerivationRule(rule);
				}
			}
			// On cree un nouvel attribut derive dans 1/9 des cas, avec derivation par rapport a un attribut
			// existant
			else if (nAttribute % 9 == 1)
			{
				// L'attribut est mis en Unused
				attribute->SetUsed(false);

				// Creation d'un nouvel attribut
				attribute = attribute->Clone();
				attribute->SetLabel("New");
				attribute->GetMetaData()->RemoveAllKeys();

				// Cas numerique
				rule = NULL;
				if (attribute->GetType() == KWType::Continuous)
				{
					rule = new KWDRCopyContinuous;
					attribute->SetDerivationRule(rule);
				}
				// Cas categoriel
				else if (attribute->GetType() == KWType::Symbol)
				{
					rule = new KWDRCopySymbol;
					attribute->SetDerivationRule(rule);
				}

				// La regle est une copie par rapport a l'attribut d'origine
				rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
				rule->GetFirstOperand()->SetAttributeName(attribute->GetName());

				// Insertion de l'attribut avec un nouveau nom
				attribute->SetName(
				    kwcTestInputClass->BuildAttributeName("Copy" + attribute->GetName()));
				attribute->SetUsed(true);
				attribute->SetLoaded(true);
				kwcTestInputClass->InsertAttribute(attribute);
			}
		}
	}

	// Compilation
	kwcTestInputClass->CompleteTypeInfo();
	kwcTestInputClass->Compile();

	// Test de lecture avec cette classe
	TestReadDataTableSliceSet("Check complex dictionary", testSliceSet, kwcTestInputClass);

	// Nettoyage
	testSliceSet->DeleteAllSliceFiles();
	delete testSliceSet;
	KWClassDomain::GetCurrentDomain()->DeleteClass(kwcTestClass->GetName());
	KWClassDomain::GetCurrentDomain()->DeleteClass(kwcTestInputClass->GetName());
}

void KWDataTableSliceSet::ComputeReadInformation(const KWClass* kwcInputClass, KWClass*& physicalClass,
						 ObjectArray* oaPhysicalSlices)
{
	boolean bOk = true;
	boolean bDisplay = false;
	boolean bDisplayDetails = false;
	KWClassDomain* physicalDomain;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	int nAttribute;
	int nAttributeBlock;
	ObjectDictionary odSliceAttributes;
	int nSlice;
	KWDataTableSlice* slice;
	ObjectDictionary odUsedSlices;
	ObjectArray oaDerivedAttributes;
	KWDerivationRule* currentDerivationRule;
	NumericKeyDictionary nkdAllUsedSliceAttributes;
	NumericKeyDictionary nkdAllUsedDerivedAttributes;
	ObjectArray oaClassNeededAttributes;
	ObjectArray oaNeededAttributes;
	NumericKeyDictionary nkdLoadedAttributeBlocks;
	ObjectArray oaAttributesToClean;
	ObjectArray oaAttributesToDelete;
	ObjectArray oaNeededAttributeBlocks;
	ALString sErrorAttribute;

	require(kwcInputClass != NULL);
	require(kwcInputClass->IsCompiled());
	require(CheckReadClass(kwcInputClass));
	require(oaPhysicalSlices != NULL);
	require(oaPhysicalSlices->GetSize() == 0);

	// Affichage du debut de methode
	if (bDisplay)
	{
		cout << GetClassLabel() << " " << GetObjectLabel() << "\n";
		cout << "\nComputeReadInformation(" << kwcInputClass->GetName() << ") (process " << GetProcessId()
		     << ")\n";
		cout << "Input dictionary\n" << *kwcInputClass << endl;
		cout << "SliceSet\n" << *this << endl;
	}

	// Recherche de tous les attributs disponibles dans les tranches
	FillSliceAttributes(&odSliceAttributes);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etape initiale de collecte des informations sur la classe en entree a traiter
	// On a besoin de creer la classe physique pour recuperer toutes les informations de type
	// regle de derivation ou gestion des donnees sparse qui seront potentiellement utile pour calculer
	// les variables non presentes dans les slices
	// On devra ensuite nettoyer les regles de derivation inutiles, pour les variables deja calculee dans les
	// slices, et deplacer en fin de classe les variables necessaire au calcul des variable derivees, mais non
	// presentes dans la classe en entree

	// Creation d'un nouveau domaine (on a potentiellement besoin du domaine complet pour recompiler la classe
	physicalDomain = kwcInputClass->GetDomain()->CloneFromClass(kwcInputClass);
	physicalDomain->SetName("Physical");
	physicalDomain->Compile();

	// Recherche de la classe physique correspondante
	physicalClass = physicalDomain->LookupClass(kwcInputClass->GetName());
	assert(kwcInputClass->GetLoadedAttributeNumber() == physicalClass->GetLoadedAttributeNumber());

	// Parcours des attributs a charger de la classe
	currentDerivationRule = NULL;
	for (nAttribute = 0; nAttribute < physicalClass->GetLoadedAttributeNumber(); nAttribute++)
	{
		attribute = physicalClass->GetLoadedAttributeAt(nAttribute);

		// On recherche s'il est dans une tranche
		slice = cast(KWDataTableSlice*, odSliceAttributes.Lookup(attribute->GetName()));

		// On memorise sa tranche si necessaire
		if (slice != NULL)
		{
			assert(KWType::IsSimple(attribute->GetType()));
			odUsedSlices.SetAt(slice->GetClass()->GetName(), slice);

			// Memorisation de l'attribut a nettoyer de sa regle de derivation
			oaAttributesToClean.Add(attribute);
		}
		// Sinon, on analyse la regle de derivation de l'attribut
		else
		{
			assert(attribute->GetAnyDerivationRule() != NULL);

			// Detection de changement de regle de derivation (notamment pour les blocs)
			if (attribute->GetAnyDerivationRule() != currentDerivationRule)
			{
				currentDerivationRule = attribute->GetAnyDerivationRule();
				check(currentDerivationRule);

				// Ajout des attribut terminaux de la regle de derivation, en s'arretant aux
				// attribut du sliceset
				BuildAllUsedSliceAttributes(currentDerivationRule, &odSliceAttributes,
							    &nkdAllUsedSliceAttributes, &nkdAllUsedDerivedAttributes,
							    sErrorAttribute);
				assert(sErrorAttribute == "");
			}
		}
	}

	// Recherche de tous les attributs necessaires pour le calcul des regles de derivation
	nkdAllUsedSliceAttributes.ExportObjectArray(&oaClassNeededAttributes);
	for (nAttribute = 0; nAttribute < oaClassNeededAttributes.GetSize(); nAttribute++)
	{
		attribute = cast(KWAttribute*, oaClassNeededAttributes.GetAt(nAttribute));
		assert(kwcInputClass->LookupAttribute(attribute->GetName()) != NULL);

		// On memorise sa tranche
		slice = cast(KWDataTableSlice*, odSliceAttributes.Lookup(attribute->GetName()));
		assert(slice != NULL);
		odUsedSlices.SetAt(slice->GetClass()->GetName(), slice);

		// Memorisation de l'attribut a nettoyer de sa regle de derivation
		oaAttributesToClean.Add(attribute);
	}

	// Destruction des regles de derivation des attributs a nettoyer
	for (nAttribute = 0; nAttribute < oaAttributesToClean.GetSize(); nAttribute++)
	{
		attribute = cast(KWAttribute*, oaAttributesToClean.GetAt(nAttribute));

		// Cas d'un attribut standard
		if (attribute->GetDerivationRule() != NULL)
			attribute->SetDerivationRule(NULL);
		// Cas d'un attribut dans un block
		else if (attribute->IsInBlock() and attribute->GetAttributeBlock()->GetDerivationRule() != NULL)
			attribute->GetAttributeBlock()->SetDerivationRule(NULL);
	}
	oaAttributesToClean.SetSize(0);

	// Affichage des attributs necessaires
	if (bDisplayDetails and bOk)
	{
		// Affichage des attributs necessaires
		cout << "Needed attributes\n";
		for (nAttribute = 0; nAttribute < oaClassNeededAttributes.GetSize(); nAttribute++)
		{
			attribute = cast(KWAttribute*, oaClassNeededAttributes.GetAt(nAttribute));
			cout << "\t" << attribute->GetName() << endl;
		}
	}

	////////////////////////////////////////////////////////////////////////
	// Reamenagement de la classe: on va garder les attributs Loaded en tete,
	// puis ajouter les autres attributs necessaires aux derivations.
	// Tous ces attributs seront marques comme Used et Loaded, les autres
	// seront detruits
	// Les blocs ayant des attributs Loaded sont garde en tete, les blocs
	// sans attributs Loaded mais avec des attributs utiles aux derivation
	// sont gardes en fin de classe, les blocs sont attribut Loaded ou
	// utiles sont detruits avec leurs attributs
	// Code quasiment recopie de la fin de la methode KWDatabase::BuildPhysicalClass()

	// Indexation de la classe physique
	physicalClass->IndexClass();

	// Identification des blocs d'attributs charges en memoire
	// Ces blocs initialement charges en memoire ne sont ni a supprimer, ni a deplacer en fin de classe
	nkdLoadedAttributeBlocks.RemoveAll();
	for (nAttributeBlock = 0; nAttributeBlock < physicalClass->GetLoadedAttributeBlockNumber(); nAttributeBlock++)
	{
		attributeBlock = physicalClass->GetLoadedAttributeBlockAt(nAttributeBlock);
		nkdLoadedAttributeBlocks.SetAt(attributeBlock, attributeBlock);
	}

	// Recherche des attributs et blocs d'attribut a charger en memoire pour cette classe
	oaAttributesToDelete.SetSize(0);
	oaNeededAttributes.SetSize(0);
	oaNeededAttributeBlocks.SetSize(0);
	attribute = physicalClass->GetHeadAttribute();
	attributeBlock = NULL;
	while (attribute != NULL)
	{
		attributeBlock = attribute->GetAttributeBlock();

		// Si attribut Loaded, on le laisse, sinon, on le retranche
		if (not attribute->GetLoaded())
		{
			// Cas d'un attribut hors bloc
			if (not attribute->IsInBlock())
			{
				// Memorisation si attribut necessaire pour le reintegrer en fin de classe
				if (nkdAllUsedSliceAttributes.Lookup(attribute) != NULL or
				    nkdAllUsedDerivedAttributes.Lookup(attribute) != NULL)
					oaNeededAttributes.Add(attribute);
				// Sinon, on enregistre l'attribut a detruire
				else
					oaAttributesToDelete.Add(attribute);
			}
			// Cas d'un attribut d'un bloc
			else
			{
				assert(attributeBlock != NULL);

				// Memorisation si attribut necessaire pour le reintegrer en fin de bloc
				if (nkdAllUsedSliceAttributes.Lookup(attribute) != NULL or
				    nkdAllUsedDerivedAttributes.Lookup(attribute) != NULL)
				{
					oaNeededAttributes.Add(attribute);

					// Memorisation si bloc necessaire pour le reintegrer en fin de classe
					if (oaNeededAttributeBlocks.GetSize() == 0 or
					    oaNeededAttributeBlocks.GetAt(oaNeededAttributeBlocks.GetSize() - 1) !=
						attributeBlock)
					{
						// Si le bloc n'est pas charge en memoire initialement, on le memorise
						// comme etant a deplacer
						if (nkdLoadedAttributeBlocks.Lookup(attributeBlock) == NULL)
							oaNeededAttributeBlocks.Add(attributeBlock);
					}
				}
				// Sinon, on enregistre l'attribut a detruire
				else
					oaAttributesToDelete.Add(attribute);
			}
		}

		// Passage a l'attribut suivant
		physicalClass->GetNextAttribute(attribute);
	}
	assert(physicalClass->GetAttributeNumber() >= oaAttributesToDelete.GetSize() + oaNeededAttributes.GetSize());

	// Destruction des attributs a detruire
	for (nAttribute = 0; nAttribute < oaAttributesToDelete.GetSize(); nAttribute++)
	{
		attribute = cast(KWAttribute*, oaAttributesToDelete.GetAt(nAttribute));

		// La destruction d'un attribut peut entrainer la destruction des son bloc englobant s'uil en est le
		// dernier
		physicalClass->DeleteAttribute(attribute->GetName());
	}

	// Deplacement des attributs necessaires en fin de classe ou de bloc pour le calcul des attributs charges en
	// memoire
	for (nAttribute = 0; nAttribute < oaNeededAttributes.GetSize(); nAttribute++)
	{
		attribute = cast(KWAttribute*, oaNeededAttributes.GetAt(nAttribute));

		// Deplacement uniquement des attributs intermediaires
		if (not attribute->GetLoaded())
		{
			// On passe l'attribut en Used et Loaded
			attribute->SetUsed(true);
			attribute->SetLoaded(true);

			// Deplacement en fin de classe ou de bloc
			if (not attribute->IsInBlock())
				physicalClass->MoveAttributeToClassTail(attribute);
			else
				physicalClass->MoveAttributeToBlockTail(attribute);
		}
	}

	// Deplacement des attributs necessaires en fin de classe ou de bloc pour le calcul des attributs charges en
	// memoire
	for (nAttributeBlock = 0; nAttributeBlock < oaNeededAttributeBlocks.GetSize(); nAttributeBlock++)
	{
		attributeBlock = cast(KWAttributeBlock*, oaNeededAttributeBlocks.GetAt(nAttributeBlock));

		// Deplacement du bloc en fin de classe
		physicalClass->MoveAttributeBlockToClassTail(attributeBlock);
	}

	// On supprime l'eventuelle cle de la classe physique, celle-ci pouvant referencer
	// des champs potentiellement supprimes
	physicalClass->SetKeyAttributeNumber(0);
	physicalClass->SetRoot(false);

	// Nettoyage du domaine physique: seule la classe physique est necessaire
	// Les autres classes potentielles doivent desormais etre deconnectees des regles de derivation
	physicalDomain->RemoveClass(physicalClass->GetName());
	physicalDomain->DeleteAllClasses();

	// On verifie que la classe physique est correcte, meme quand elle est toute seule dans son domaine
	debug(physicalDomain->InsertClass(physicalClass));
	debug(assert(physicalClass->Check()));
	debug(physicalDomain->RemoveClass(physicalClass->GetName()));

	// Destruction du domaine
	delete physicalDomain;

	// Indexation de la classe
	physicalClass->IndexClass();
	assert(physicalClass->GetAttributeNumber() ==
	       kwcInputClass->GetLoadedAttributeNumber() + oaNeededAttributes.GetSize());

	// Export des tranches a utilisees, selon leur ordre initial
	for (nSlice = 0; nSlice < GetSliceNumber(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));
		if (odUsedSlices.Lookup(slice->GetClass()->GetName()) != NULL)
			oaPhysicalSlices->Add(slice);
	}

	// Parametrage des attributs a charger de chaque tranche
	for (nSlice = 0; nSlice < oaPhysicalSlices->GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, oaPhysicalSlices->GetAt(nSlice));

		// Parametrage des attributs a charger de la tranche
		attribute = slice->GetClass()->GetHeadAttribute();
		while (attribute != NULL)
		{
			// L'attribut est a charger s'il fait partie de la classe physique
			attribute->SetLoaded(physicalClass->LookupAttribute(attribute->GetName()) != NULL);

			// Attribut suivant
			slice->GetClass()->GetNextAttribute(attribute);
		}

		// Indexation de la classe
		slice->GetClass()->IndexClass();
	}

	// Affichage de la classe physique
	if (bDisplay)
	{
		cout << "Physical dictionary\n" << *physicalClass << endl;

		// Affichage des tranches necessaires
		cout << "Needed slices with their dictionary\n";
		for (nSlice = 0; nSlice < oaPhysicalSlices->GetSize(); nSlice++)
		{
			slice = cast(KWDataTableSlice*, oaPhysicalSlices->GetAt(nSlice));
			cout << "(slice " << nSlice + 1 << ")\t" << *slice->GetClass() << endl;
		}
	}
}

boolean KWDataTableSliceSet::BuildAllUsedSliceAttributes(const KWDerivationRule* rule,
							 const ObjectDictionary* odSliceAttributes,
							 NumericKeyDictionary* nkdAllUsedSliceAttributes,
							 NumericKeyDictionary* nkdAllUsedDerivedAttributes,
							 ALString& sErrorAttribute) const
{
	boolean bOk = true;
	int nOperand;
	KWDerivationRuleOperand* operand;
	KWAttribute* originAttribute;
	KWAttributeBlock* originAttributeBlock;
	KWDataTableSlice* slice;

	require(rule != NULL);
	require(rule->IsCompiled());
	require(odSliceAttributes != NULL);
	require(nkdAllUsedSliceAttributes != NULL);
	require(nkdAllUsedDerivedAttributes != NULL);

	// Alimentation par parcours des operandes de la regle
	for (nOperand = 0; nOperand < rule->GetOperandNumber(); nOperand++)
	{
		operand = rule->GetOperandAt(nOperand);

		// Recherche recursive si presence effective d'une regle
		if (operand->GetOrigin() == KWDerivationRuleOperand::OriginRule and
		    operand->GetDerivationRule() != NULL)
			bOk = BuildAllUsedSliceAttributes(operand->GetDerivationRule(), odSliceAttributes,
							  nkdAllUsedSliceAttributes, nkdAllUsedDerivedAttributes,
							  sErrorAttribute);
		// Recherche si presence effective d'un attribut calcule avec une regle, selon un attribut ou un bloc
		// d'attributs
		else if (operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
		{
			// Cas d'un operande de type attribut
			if (KWType::IsValue(operand->GetType()))
			{
				originAttribute = operand->GetOriginAttribute();
				assert(originAttribute != NULL);

				// Recherche de l'attribut dans le sliceset
				slice = cast(KWDataTableSlice*, odSliceAttributes->Lookup(originAttribute->GetName()));

				// Enregistrement si attribut dans le sliceset
				if (slice != NULL)
					nkdAllUsedSliceAttributes->SetAt((NUMERIC)originAttribute, originAttribute);
				// Sinon, erreur
				else
				{
					// Erreur si attribut natif et absent du sliceset
					if (originAttribute->GetAnyDerivationRule() == NULL)
					{
						sErrorAttribute = originAttribute->GetName();
						bOk = false;
						break;
					}
					// Sinon cas ou l'attribut est calcule: On propage recursivement la verification
					else
					{
						// On propage uniquement si necessaire
						if (nkdAllUsedDerivedAttributes->Lookup((NUMERIC)originAttribute) ==
						    NULL)
						{
							nkdAllUsedDerivedAttributes->SetAt((NUMERIC)originAttribute,
											   originAttribute);
							bOk = BuildAllUsedSliceAttributes(
							    originAttribute->GetAnyDerivationRule(), odSliceAttributes,
							    nkdAllUsedSliceAttributes, nkdAllUsedDerivedAttributes,
							    sErrorAttribute);
						}
					}
				}
			}
			// Cas d'un operande de type bloc d'attribut
			else
			{
				assert(KWType::IsValueBlock(operand->GetType()));
				originAttributeBlock = operand->GetOriginAttributeBlock();
				assert(originAttributeBlock != NULL);

				// On parcours tous les attributs du bloc pour les rechercher dans les slices
				originAttribute = originAttributeBlock->GetFirstAttribute();
				while (originAttribute != NULL)
				{
					// Recherche de l'attribut dans le sliceset
					slice = cast(KWDataTableSlice*,
						     odSliceAttributes->Lookup(originAttribute->GetName()));

					// Enregistrement si attribut dans le sliceset
					if (slice != NULL)
						nkdAllUsedSliceAttributes->SetAt((NUMERIC)originAttribute,
										 originAttribute);
					// Sinon on verifie sa provenance
					else
					{
						// Erreur si attribut natif et absent du sliceset
						if (originAttribute->GetAnyDerivationRule() == NULL)
						{
							sErrorAttribute = originAttribute->GetName();
							bOk = false;
							break;
						}
						// Sinon cas ou l'attribut est calcule: on propage recursivement la
						// verification
						else
						{
							// On propage la verification une seule fois (la meme regle est
							// utilisee pour tous les attributs du bloc)
							if (originAttribute ==
							    originAttributeBlock->GetFirstAttribute())
							{
								// On propage uniquement si necessaire
								if (nkdAllUsedDerivedAttributes->Lookup(
									(NUMERIC)originAttribute) == NULL)
								{
									nkdAllUsedDerivedAttributes->SetAt(
									    (NUMERIC)originAttribute, originAttribute);
									bOk = BuildAllUsedSliceAttributes(
									    originAttribute->GetAnyDerivationRule(),
									    odSliceAttributes,
									    nkdAllUsedSliceAttributes,
									    nkdAllUsedDerivedAttributes,
									    sErrorAttribute);
									if (not bOk)
										break;
								}
								// Sinon, on se contente d'enregistrer l'attribut du
								// bloc
								else
									nkdAllUsedDerivedAttributes->SetAt(
									    (NUMERIC)originAttribute, originAttribute);
							}
						}
					}

					// Passage a l'attribut suivant, jusqu'a la fin du bloc
					if (originAttribute != originAttributeBlock->GetLastAttribute())
						originAttribute->GetParentClass()->GetNextAttribute(originAttribute);
					else
						originAttribute = NULL;
				}
				if (not bOk)
					break;
			}
		}
		if (not bOk)
			break;
	}
	return bOk;
}

KWObject* KWDataTableSliceSet::PhysicalRead()
{
	boolean bOk;
	KWObject* kwoObject;
	int nSlice;
	KWDataTableSlice* slice;
	int nIndex;
	KWLoadIndex liLoadIndex;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	NumericKeyDictionary nkdUnusedNativeAttributesToKeep;
	ALString sMessage;

	require(IsOpenedForRead());
	require(read_Class->IsCompiled());
	require(read_PhysicalClass->IsCompiled());

	// Creation d'un objet par lecture dans la premiere tranche
	bOk = read_FirstPhysicalSlice->PhysicalReadObject(kwoObject, true);

	// Parcours des tranches suivante pour lire les autres valeurs de l'objet
	for (nSlice = 1; nSlice < read_oaPhysicalSlices.GetSize(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, read_oaPhysicalSlices.GetAt(nSlice));
		bOk = bOk and slice->PhysicalReadObject(kwoObject, false);
	}

	// Calcul des regles de derivation
	if (bOk)
	{
		for (nIndex = 0; nIndex < read_livDerivedDataItems.GetSize(); nIndex++)
		{
			liLoadIndex = read_livDerivedDataItems.GetAt(nIndex);
			dataItem = read_PhysicalClass->GetDataItemAtLoadIndex(liLoadIndex);

			// Calcul des attributs derives
			if (dataItem->IsAttribute())
			{
				attribute = cast(KWAttribute*, dataItem);
				assert(attribute->GetDerivationRule() != NULL);
				assert(KWType::IsValue(attribute->GetType()));
				assert(not KWType::IsRelation(attribute->GetType()));

				// Calcul selon le type
				switch (attribute->GetType())
				{
				case KWType::Symbol:
					kwoObject->ComputeSymbolValueAt(liLoadIndex);
					break;
				case KWType::Continuous:
					kwoObject->ComputeContinuousValueAt(liLoadIndex);
					break;
				case KWType::Date:
					kwoObject->ComputeDateValueAt(liLoadIndex);
					break;
				case KWType::Time:
					kwoObject->ComputeTimeValueAt(liLoadIndex);
					break;
				case KWType::Timestamp:
					kwoObject->ComputeTimestampValueAt(liLoadIndex);
					break;
				case KWType::Structure:
					kwoObject->ComputeStructureValueAt(liLoadIndex);
					break;
				}
			}
			// Calcul des blocks d'attributs derives
			else
			{
				attributeBlock = cast(KWAttributeBlock*, dataItem);
				assert(attributeBlock->GetDerivationRule() != NULL);
				assert(KWType::IsSimple(attributeBlock->GetType()));

				// Calcul selon le type
				if (attributeBlock->GetType() == KWType::Symbol)
					kwoObject->ComputeSymbolValueBlockAt(liLoadIndex);
				else
					kwoObject->ComputeContinuousValueBlockAt(liLoadIndex);
			}
		}
	}

	// Mutation de l'objet
	if (bOk)
	{
		assert(nkdUnusedNativeAttributesToKeep.GetCount() == 0);
		kwoObject->Mutate(read_Class, &nkdUnusedNativeAttributesToKeep);
	}

	// Destruction de l'objet en cas d'erreur
	if (not bOk)
	{
		delete kwoObject;
		kwoObject = NULL;
	}
	return kwoObject;
}

int KWDataTableSliceSet::GetUsedSimpleAttributeNumber() const
{
	KWClass* kwcClass;
	int nAttributeNumber;

	nAttributeNumber = 0;
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName);
	if (kwcClass != NULL)
	{
		nAttributeNumber = kwcClass->GetUsedAttributeNumberForType(KWType::Continuous) +
				   kwcClass->GetUsedAttributeNumberForType(KWType::Symbol);
		if (kwcClass->LookupAttribute(GetTargetAttributeName()) != NULL)
			nAttributeNumber--;
	}
	return nAttributeNumber;
}

///////////////////////////////////////////////////////////////////////////////////
// Class KWDataTableSlice

KWDataTableSlice::KWDataTableSlice()
{
	// Initialisation des variables de gestion de lecture
	read_SliceDataTableDriver = NULL;
	read_nDataFileIndex = 0;
}

KWDataTableSlice::~KWDataTableSlice()
{
	assert(oaObjects.GetSize() == 0);
}

IntVector* KWDataTableSlice::GetLexicographicIndex()
{
	return &ivLexicographicIndex;
}

int KWDataTableSlice::CompareLexicographicOrder(KWDataTableSlice* otherSlice) const
{
	int nCompare;
	int nMaxSize;
	int i;

	require(otherSlice != NULL);

	// Comparaison des vecteur d'index lexicographique
	nCompare = 0;
	nMaxSize = max(ivLexicographicIndex.GetSize(), otherSlice->ivLexicographicIndex.GetSize());
	for (i = 0; i < nMaxSize; i++)
	{
		if (i >= ivLexicographicIndex.GetSize())
			nCompare = -1;
		else if (i >= otherSlice->ivLexicographicIndex.GetSize())
			nCompare = 1;
		else
			nCompare = ivLexicographicIndex.GetAt(i) - otherSlice->ivLexicographicIndex.GetAt(i);
		if (nCompare != 0)
			break;
	}
	return nCompare;
}

KWClass* KWDataTableSlice::GetClass()
{
	return &kwcClass;
}

StringVector* KWDataTableSlice::GetDataFileNames()
{
	return &svDataFileNames;
}

LongintVector* KWDataTableSlice::GetDataFileSizes()
{
	return &lvDataFileSizes;
}

void KWDataTableSlice::DeleteSliceFiles()
{
	PLFileConcatenater concatenater;

	// Supression des fichier pour toutes les tranches
	concatenater.RemoveChunks(GetDataFileNames());

	// Supression de la specification des fichiers
	GetDataFileNames()->SetSize(0);
	GetDataFileSizes()->SetSize(0);

	// Supression desstatistiques sur les valeurs
	lvAttributeBlockValueNumbers.SetSize(0);
	lvDenseSymbolAttributeDiskSizes.SetSize(0);
}

longint KWDataTableSlice::GetTotalDataFileSize() const
{
	longint lTotalDataFileSize;
	int i;

	lTotalDataFileSize = 0;
	for (i = 0; i < lvDataFileSizes.GetSize(); i++)
		lTotalDataFileSize += lvDataFileSizes.GetAt(i);
	return lTotalDataFileSize;
}

longint KWDataTableSlice::GetTotalDenseSymbolAttributeDiskSize() const
{
	longint lTotalDenseSymbolAttributeDiskSize;
	KWAttribute* attribute;
	int nDenseAttributeIndex;

	// Parcours de tous les attributs pour ne traiter que ceux charges en memoire
	lTotalDenseSymbolAttributeDiskSize = 0;
	nDenseAttributeIndex = 0;
	attribute = kwcClass.GetHeadAttribute();
	while (attribute != NULL)
	{
		// Cas d'un attribut dense
		if (not attribute->IsInBlock())
		{
			// On le prend en compte s'il est charge en memoire et de type Symbol
			if (attribute->GetLoaded() and attribute->GetType() == KWType::Symbol)
				lTotalDenseSymbolAttributeDiskSize +=
				    lvDenseSymbolAttributeDiskSizes.GetAt(nDenseAttributeIndex);
			nDenseAttributeIndex++;
		}
		// Cas d'un bloc
		else
			attribute = attribute->GetAttributeBlock()->GetLastAttribute();

		// Attribut suivant
		kwcClass.GetNextAttribute(attribute);
	}
	return lTotalDenseSymbolAttributeDiskSize;
}

int KWDataTableSlice::GetMaxBlockAttributeNumber() const
{
	int nMaxBlockAttributeNumber;
	KWAttributeBlock* attributeBlock;
	int i;

	// Parcours de tous les blocs pour identifier celui de plus grand taille
	nMaxBlockAttributeNumber = 0;
	for (i = 0; i < kwcClass.GetLoadedAttributeBlockNumber(); i++)
	{
		attributeBlock = kwcClass.GetLoadedAttributeBlockAt(i);
		nMaxBlockAttributeNumber = max(nMaxBlockAttributeNumber, attributeBlock->GetLoadedAttributeNumber());
	}
	return nMaxBlockAttributeNumber;
}

longint KWDataTableSlice::GetMaxAttributeBlockValueNumber() const
{
	longint lMaxAttributeBlockValueNumber;
	KWAttributeBlock* attributeBlock;
	int nBlockIndex;

	// Parcours de tous les bloc Used pour se synchroniser avec les index des blocs loaded
	lMaxAttributeBlockValueNumber = 0;
	nBlockIndex = 0;
	attributeBlock = kwcClass.GetHeadAttributeBlock();
	while (attributeBlock != NULL)
	{
		// Traitement uniquement des blocs utilises
		if (attributeBlock->GetLoaded())
			lMaxAttributeBlockValueNumber =
			    max(lMaxAttributeBlockValueNumber, lvAttributeBlockValueNumbers.GetAt(nBlockIndex));

		// Bloc suivant
		kwcClass.GetNextAttributeBlock(attributeBlock);
		nBlockIndex++;
	}
	assert(nBlockIndex == lvAttributeBlockValueNumbers.GetSize());
	return lMaxAttributeBlockValueNumber;
}

longint KWDataTableSlice::GetTotalAttributeBlockValueNumber() const
{
	longint lTotalAttributeBlockValueNumber;
	KWAttributeBlock* attributeBlock;
	int nBlockIndex;

	// Parcours de tous les bloc Used pour se synchroniser avec les index des blocs loaded
	lTotalAttributeBlockValueNumber = 0;
	nBlockIndex = 0;
	attributeBlock = kwcClass.GetHeadAttributeBlock();
	while (attributeBlock != NULL)
	{
		// Traitement uniquement des blocs utilises
		if (attributeBlock->GetLoaded())
			lTotalAttributeBlockValueNumber += lvAttributeBlockValueNumbers.GetAt(nBlockIndex);

		// Bloc suivant
		kwcClass.GetNextAttributeBlock(attributeBlock);
		nBlockIndex++;
	}
	assert(nBlockIndex == lvAttributeBlockValueNumbers.GetSize());
	return lTotalAttributeBlockValueNumber;
}

boolean KWDataTableSlice::OpenForRead()
{
	return PhysicalOpenForRead(GetClass());
}

boolean KWDataTableSlice::IsOpenedForRead() const
{
	// On se sert de l'existence du driver comme indcateur de lecture en cours
	return read_SliceDataTableDriver != NULL;
}

boolean KWDataTableSlice::IsEnd() const
{
	// On est a la fin quand l'index du fichier courant depasse le nombre de fichiers
	return read_nDataFileIndex >= svDataFileNames.GetSize();
}

KWObject* KWDataTableSlice::Read()
{
	boolean bOk;
	KWObject* kwoObject;

	// Lecture local locale a la slice, avec creation de l'objet
	bOk = PhysicalReadObject(kwoObject, true);

	// Destruction de l'objet en cas d'erreur
	if (not bOk)
	{
		delete kwoObject;
		kwoObject = NULL;
	}
	return kwoObject;
}

void KWDataTableSlice::Skip()
{
	boolean bOk = true;
	ALString sTmp;

	require(read_SliceDataTableDriver->GetClass() != NULL);
	require(read_SliceDataTableDriver->IsOpenInformationComputed());
	require(not IsEnd());
	require(not IsError());

	// Ouverture du prochain fichier de tranche en lecture si necessaire
	if (read_SliceDataTableDriver->GetRecordIndex() == 0)
		bOk = PhysicalOpenCurrentChunk();

	// Saut d'une ligne
	if (bOk)
		read_SliceDataTableDriver->Skip();

	// Fermeture si necessaire du fichier courant
	if (read_SliceDataTableDriver->IsEnd())
		bOk = PhysicalCloseCurrentChunk();
}

boolean KWDataTableSlice::IsError() const
{
	require(IsOpenedForRead());
	return read_SliceDataTableDriver->IsError();
}

double KWDataTableSlice::GetReadPercentage()
{
	double dReadPercentage;

	require(IsOpenedForRead());

	dReadPercentage = read_nDataFileIndex / (double)GetDataFileNames()->GetSize();
	dReadPercentage += read_SliceDataTableDriver->GetReadPercentage() / GetDataFileNames()->GetSize();
	return dReadPercentage;
}

int KWDataTableSlice::GetChunkIndex() const
{
	require(IsOpenedForRead());
	return read_nDataFileIndex;
}

boolean KWDataTableSlice::Close()
{
	require(IsOpenedForRead());

	// Fermeture si necessaire pour la base en cours
	if (read_SliceDataTableDriver->IsOpenedForRead())
		read_SliceDataTableDriver->Close();

	// Nettoyage du driver
	read_SliceDataTableDriver->CleanOpenInformation();
	read_SliceDataTableDriver->SetClass(NULL);
	delete read_SliceDataTableDriver;
	read_SliceDataTableDriver = NULL;
	read_nDataFileIndex = 0;
	return true;
}

boolean KWDataTableSlice::ReadAll()
{
	boolean bOk = true;
	PeriodicTest periodicTestInterruption;
	KWObject* kwoObject;
	longint lRecordNumber;

	require(Check());
	require(GetClass()->IsCompiled());
	require(oaObjects.GetSize() == 0);
	require(not IsOpenedForRead());

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Read data table slice " + GetClass()->GetName());

	// Ouverture de la base en lecture
	bOk = OpenForRead();

	// Lecture d'objets dans la base
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		lRecordNumber = 0;
		while (not IsEnd())
		{
			kwoObject = Read();
			lRecordNumber++;

			// Ajout de l'objet au tableau
			if (kwoObject != NULL)
				oaObjects.Add(kwoObject);
			// Arret sinon
			else
			{
				bOk = false;
				break;
			}

			// Suivi de la tache
			if (periodicTestInterruption.IsTestAllowed(lRecordNumber))
				TaskProgression::DisplayProgression((int)(100 * GetReadPercentage()));
		}
		Global::DesactivateErrorFlowControl();

		// Test si interruption sans qu'il y ait d'erreur
		if (IsError() or TaskProgression::IsInterruptionRequested())
		{
			bOk = false;

			// Warning ou erreur selon le cas
			if (IsError())
				AddError("Read data table slice interrupted because of errors");
			else
				AddWarning("Read data table slice interrupted by user");
		}

		// Fermeture
		bOk = Close() and bOk;
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();

	// Nettoyage si necessaire
	if (not bOk)
		DeleteAll();
	return bOk;
}

ObjectArray* KWDataTableSlice::GetObjects()
{
	return &oaObjects;
}

void KWDataTableSlice::RemoveAll()
{
	oaObjects.RemoveAll();
}

void KWDataTableSlice::DeleteAll()
{
	oaObjects.DeleteAll();
}

const ALString KWDataTableSlice::BuildPrefix(const IntVector* ivSliceLexicographicIndex, int nChunkIndex)
{
	ALString sPrefix;
	int i;

	require(ivSliceLexicographicIndex != NULL);
	require(ivSliceLexicographicIndex->GetSize() >= 1);
	require(nChunkIndex >= -1);

	// Construction du prefix
	sPrefix = GetSlicePrefix();
	for (i = 0; i < ivSliceLexicographicIndex->GetSize(); i++)
	{
		assert(ivSliceLexicographicIndex->GetAt(i) >= 0);
		if (i > 0)
			sPrefix += GetSubSlicePrefix();
		sPrefix += IntToString(ivSliceLexicographicIndex->GetAt(i) + 1);
	}
	if (nChunkIndex >= 0)
	{
		sPrefix += GetChunkPrefix();
		sPrefix += IntToString(nChunkIndex + 1);
	}
	sPrefix += GetPrefixEnd();
	return sPrefix;
}

const ALString KWDataTableSlice::BuildSliceFileName(int nSliceIndex, int nChunkIndex, const ALString& sFilePathName)
{
	ALString sSliceFileName;
	IntVector ivSliceLexicographicIndex;

	require(nSliceIndex >= 0);
	require(nChunkIndex >= 0);
	require(sFilePathName != "");

	// Construction du nom de fichier de tranche
	ivSliceLexicographicIndex.Add(nSliceIndex);
	sSliceFileName = BuildSubSliceFileName(&ivSliceLexicographicIndex, nChunkIndex, sFilePathName);
	return sSliceFileName;
}

const ALString KWDataTableSlice::BuildSubSliceFileName(const IntVector* ivSliceLexicographicIndex, int nChunkIndex,
						       const ALString& sFilePathName)
{
	ALString sSliceFileName;

	require(ivSliceLexicographicIndex != NULL);
	require(ivSliceLexicographicIndex->GetSize() >= 1);
	require(nChunkIndex >= 0);
	require(sFilePathName != "");

	// Construction du nom de fichier de tranche
	sSliceFileName = BuildPrefix(ivSliceLexicographicIndex, nChunkIndex);
	sSliceFileName += FileService::GetFileName(sFilePathName);
	return sSliceFileName;
}

const ALString KWDataTableSlice::GetSliceFilePrefix(const ALString& sSliceFilePathName)
{
	ALString sSliceFileName;
	int nSlicePrefixPos;
	int nPrefixEndPos;

	require(sSliceFilePathName != "");

	// Extarction du nom d fichier
	sSliceFileName = FileService::GetFileName(sSliceFilePathName);

	// Recherche du debut de prefix et du separateur
	nSlicePrefixPos = sSliceFileName.Find(GetSlicePrefix());
	nPrefixEndPos = sSliceFileName.Find(GetPrefixEnd());
	assert(nSlicePrefixPos >= 0);
	assert(nPrefixEndPos > nSlicePrefixPos);
	assert(nPrefixEndPos + 1 < sSliceFileName.GetLength());

	// Extraction du nom de base du fichier
	return sSliceFileName.Mid(nSlicePrefixPos, nPrefixEndPos - nSlicePrefixPos);
}

const ALString KWDataTableSlice::GetSliceFileBaseName(const ALString& sSliceFilePathName)
{
	ALString sSliceFileName;
	int nPrefixEndPos;

	require(sSliceFilePathName != "");

	// Extarction du nom d fichier
	sSliceFileName = FileService::GetFileName(sSliceFilePathName);
	assert(sSliceFileName.Find(GetSlicePrefix()) >= 0);
	assert(sSliceFileName.Find(GetPrefixEnd()) >= sSliceFileName.Find(GetSlicePrefix()));

	// Recherche du debut de prefix et du separateur
	nPrefixEndPos = sSliceFileName.Find(GetPrefixEnd());
	assert(nPrefixEndPos >= 0);
	assert(nPrefixEndPos + 1 < sSliceFileName.GetLength());

	// Extraction du nom de base du fichier
	return sSliceFileName.Mid(nPrefixEndPos + 1);
}

char KWDataTableSlice::GetSlicePrefix()
{
	return 'S';
}

char KWDataTableSlice::GetSubSlicePrefix()
{
	return 's';
}

char KWDataTableSlice::GetChunkPrefix()
{
	return 'C';
}

char KWDataTableSlice::GetPrefixEnd()
{
	return '_';
}

DoubleVector* KWDataTableSlice::GetLexicographicSortCriterion()
{
	return &dvLexicographicSortCriterion;
}

int KWDataTableSlice::CompareLexicographicSortCriterion(KWDataTableSlice* otherSlice) const
{

	int nCompare;
	int nMaxSize;
	int i;

	require(otherSlice != NULL);

	// Comparaison des vecteur d'index lexicographique
	nCompare = 0;
	nMaxSize = max(dvLexicographicSortCriterion.GetSize(), otherSlice->dvLexicographicSortCriterion.GetSize());
	for (i = 0; i < nMaxSize; i++)
	{
		if (i >= dvLexicographicSortCriterion.GetSize())
			nCompare = -1;
		else if (i >= otherSlice->dvLexicographicSortCriterion.GetSize())
			nCompare = 1;
		else
			nCompare = CompareDouble(dvLexicographicSortCriterion.GetAt(i),
						 otherSlice->dvLexicographicSortCriterion.GetAt(i));
		if (nCompare != 0)
			break;
	}
	if (nCompare == 0)
		nCompare = CompareLexicographicOrder(otherSlice);
	return nCompare;
}

void KWDataTableSlice::DisplayLexicographicSortCriterionHeaderLineReport(ostream& ost,
									 const ALString& sSortCriterion) const
{
	ost << "Slice\tClass\t" << sSortCriterion << "\n";
}

void KWDataTableSlice::DisplayLexicographicSortCriterionLineReport(ostream& ost) const
{
	int i;

	ost << BuildPrefix(&ivLexicographicIndex, -1) << "\t";
	ost << kwcClass.GetName();
	for (i = 0; i < dvLexicographicSortCriterion.GetSize(); i++)
	{
		ost << "\t";
		ost << dvLexicographicSortCriterion.GetAt(i);
	}
	ost << "\n";
}

boolean KWDataTableSlice::Check() const
{
	boolean bOk = true;
	KWClassDomain checkClassDomain;
	KWClass* checkClass;
	KWAttribute* attribute;
	int i;
	int nDenseAttributeIndex;

	// Verification de l'index lexicographique
	bOk = bOk and ivLexicographicIndex.GetSize() > 0;
	for (i = 0; i < ivLexicographicIndex.GetSize(); i++)
		bOk = bOk and ivLexicographicIndex.GetAt(i) >= 0;

	// Verification du type simple et non calcule de tous les attributs
	attribute = kwcClass.GetHeadAttribute();
	while (attribute != NULL)
	{
		// Verification que l'attribut est de type simple et non calcule
		// Il peut eventuellement etre non charge en memoire
		bOk = bOk and attribute->GetUsed();
		bOk = bOk and KWType::IsSimple(attribute->GetType());
		bOk = bOk and attribute->GetAnyDerivationRule() == NULL;
		bOk = bOk and attribute->GetLabel() == "";
		if (not bOk)
			break;

		// Attribut suivant
		kwcClass.GetNextAttribute(attribute);
	}

	// Verifications globales sur la classe
	if (bOk)
	{
		bOk = bOk and kwcClass.GetKeyAttributeNumber() == 0;
		bOk = bOk and kwcClass.GetLabel() == "";
		bOk = bOk and kwcClass.GetAttributeNumber() > 0;
	}

	// Insertion si necessaire de la classe dans un domaine le temps de sa verification
	if (bOk)
	{
		checkClass = cast(KWClass*, &kwcClass);
		if (kwcClass.GetDomain() == NULL)
			checkClassDomain.InsertClass(checkClass);
		bOk = bOk and kwcClass.Check();
		if (kwcClass.GetDomain() == &checkClassDomain)
			checkClassDomain.RemoveClass(checkClass->GetName());
	}

	// Test des vecteurs d'index
	bOk = bOk and livDataItemLoadIndexes.GetSize() == ivValueBlockFirstSparseIndexes.GetSize();
	bOk = bOk and livDataItemLoadIndexes.GetSize() == ivValueBlockLastSparseIndexes.GetSize();
	if (bOk and livDataItemLoadIndexes.GetSize() > 0)
		bOk = bOk and livDataItemLoadIndexes.GetSize() == kwcClass.GetLoadedDataItemNumber();

	// Test des vecteurs de statistiques sur les valeurs
	if (bOk and lvAttributeBlockValueNumbers.GetSize() > 0)
		bOk = bOk and lvAttributeBlockValueNumbers.GetSize() ==
				  kwcClass.GetUsedAttributeNumberForType(KWType::ContinuousValueBlock) +
				      kwcClass.GetUsedAttributeNumberForType(KWType::SymbolValueBlock);
	if (bOk and lvDenseSymbolAttributeDiskSizes.GetSize() > 0)
	{
		bOk = bOk and lvDenseSymbolAttributeDiskSizes.GetSize() ==
				  kwcClass.GetUsedDenseAttributeNumberForType(KWType::Symbol) +
				      kwcClass.GetUsedDenseAttributeNumberForType(KWType::Continuous);

		// On verifie que le vecteur contient des zeros pour les attributs dense numeriques
		nDenseAttributeIndex = 0;
		for (i = 0; i < kwcClass.GetUsedAttributeNumber(); i++)
		{
			attribute = kwcClass.GetUsedAttributeAt(i);
			if (not attribute->IsInBlock())
			{
				if (attribute->GetType() != KWType::Symbol)
				{
					bOk = bOk and lvDenseSymbolAttributeDiskSizes.GetAt(nDenseAttributeIndex) == 0;
					if (not bOk)
						break;
				}
				nDenseAttributeIndex++;
			}
		}
	}
	return bOk;
}

void KWDataTableSlice::CopyFrom(const KWDataTableSlice* aSource)
{
	require(aSource != NULL);

	ivLexicographicIndex.CopyFrom(&aSource->ivLexicographicIndex);
	kwcClass.CopyFrom(&aSource->kwcClass);
	svDataFileNames.CopyFrom(&aSource->svDataFileNames);
	lvDataFileSizes.CopyFrom(&aSource->lvDataFileSizes);
	lvAttributeBlockValueNumbers.CopyFrom(&aSource->lvAttributeBlockValueNumbers);
	lvDenseSymbolAttributeDiskSizes.CopyFrom(&aSource->lvDenseSymbolAttributeDiskSizes);
	livDataItemLoadIndexes.CopyFrom(&aSource->livDataItemLoadIndexes);
	ivValueBlockFirstSparseIndexes.CopyFrom(&aSource->ivValueBlockFirstSparseIndexes);
	ivValueBlockLastSparseIndexes.CopyFrom(&aSource->ivValueBlockLastSparseIndexes);
	oaObjects.SetSize(0);
}

KWDataTableSlice* KWDataTableSlice::Clone() const
{
	KWDataTableSlice* clone;
	clone = new KWDataTableSlice;
	clone->CopyFrom(this);
	return clone;
}

longint KWDataTableSlice::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KWDataTableSlice);
	lUsedMemory += ivLexicographicIndex.GetUsedMemory() - sizeof(IntVector);
	lUsedMemory += kwcClass.GetUsedMemory() - sizeof(KWClass);
	lUsedMemory += svDataFileNames.GetUsedMemory() - sizeof(StringVector);
	lUsedMemory += lvAttributeBlockValueNumbers.GetUsedMemory() - sizeof(LongintVector);
	lUsedMemory += lvDenseSymbolAttributeDiskSizes.GetUsedMemory() - sizeof(LongintVector);
	lUsedMemory += lvDataFileSizes.GetUsedMemory() - sizeof(LongintVector);
	lUsedMemory += livDataItemLoadIndexes.GetUsedMemory() - sizeof(KWLoadIndexVector);
	lUsedMemory += ivValueBlockFirstSparseIndexes.GetUsedMemory() - sizeof(IntVector);
	lUsedMemory += ivValueBlockLastSparseIndexes.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

void KWDataTableSlice::Write(ostream& ost) const
{
	int nFile;

	// Entete
	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\n";

	// Index
	ost << "Lexicographic index\t" << BuildPrefix(&ivLexicographicIndex, -1) << "\n";

	// Dictionnaire
	ost << "Slice dictionary\t" << kwcClass.GetName() << "\n";

	// Attributs et blocs d'attributs
	assert(kwcClass.GetUsedAttributeNumber() == kwcClass.GetLoadedAttributeNumber());
	ost << "Variables\t" << kwcClass.GetUsedAttributeNumber() << "\n";
	ost << "Dense variables\t" << kwcClass.GetLoadedDenseAttributeNumber() << "\n";
	ost << "Variable blocks\t" << kwcClass.GetLoadedAttributeBlockNumber() << "\n";

	// Nombre total des nombres de valeur par blocs d'attribut
	ost << "Total block value number\t" << GetTotalAttributeBlockValueNumber() << "\n";

	// Taille totale des fichiers
	ost << "Total data file size\t" << GetTotalDataFileSize() << "\n";

	// Fichiers
	if (svDataFileNames.GetSize() > 0)
	{
		ost << "Files\t" << svDataFileNames.GetSize() << "\n";
		ost << "File\tSize\n";
		for (nFile = 0; nFile < svDataFileNames.GetSize(); nFile++)
			ost << svDataFileNames.GetAt(nFile) << "\t" << lvDataFileSizes.GetAt(nFile) << "\n";
	}
}

const ALString KWDataTableSlice::GetClassLabel() const
{
	return "Data table slice";
}

const ALString KWDataTableSlice::GetObjectLabel() const
{
	return kwcClass.GetName();
}

boolean KWDataTableSlice::PhysicalOpenForRead(KWClass* driverClass)
{
	require(Check());
	require(GetClass()->IsCompiled());
	require(not IsOpenedForRead());
	require(svDataFileNames.GetSize() > 0);
	require(read_nDataFileIndex == 0);
	require(driverClass != NULL);

	// Parametrage du driver dedie a la gestion des tranches
	read_SliceDataTableDriver = new KWDataTableDriverSlice;
	read_SliceDataTableDriver->SetClass(driverClass);
	read_SliceDataTableDriver->ComputeOpenInformation(GetClass());

	// On saute les eventuels fichiers vides
	PhysicalSkipEmptyChunks();
	return true;
}

boolean KWDataTableSlice::PhysicalReadObject(KWObject*& kwoObject, boolean bCreate)
{
	boolean bOk = true;
	ALString sTmp;

	require(IsOpenedForRead());
	require(read_SliceDataTableDriver->GetClass() != NULL);
	require(read_SliceDataTableDriver->IsOpenInformationComputed());
	require(not IsEnd());
	require(not IsError());

	// Ouverture du prochain fichier de tranche en lecture si necessaire
	if (read_SliceDataTableDriver->GetRecordIndex() == 0)
		bOk = PhysicalOpenCurrentChunk();

	// Creation inconditionnelle d'un objet si demande, pour qu'un objet soit systematiquement disponible en fin de
	// methode
	if (bCreate)
		kwoObject = new KWObject(read_SliceDataTableDriver->GetClass(),
					 read_SliceDataTableDriver->GetRecordIndex() + 1);

	// Lecture d'un objet dans la base
	if (bOk)
	{
		assert(not read_SliceDataTableDriver->IsEnd());

		// Lecture de la partie de l'objet concernee par la tranche
		bOk = read_SliceDataTableDriver->ReadObject(kwoObject);
		assert(not bOk or kwoObject->GetCreationIndex() == read_SliceDataTableDriver->GetRecordIndex());

		// Message si erreur (sans interruption utilisateur)
		if (not bOk and not TaskProgression::IsInterruptionRequested())
		{
			AddError(sTmp + "Read slice file interrupted because of corrupted line " +
				 LongintToString(read_SliceDataTableDriver->GetRecordIndex()) + " (slice " +
				 FileService::GetURIUserLabel(svDataFileNames.GetAt(read_nDataFileIndex)) + ")");
		}

		// Arret si erreur du driver
		if (bOk and read_SliceDataTableDriver->IsError())
		{
			bOk = false;
			AddError(sTmp + "Read slice file interrupted because of errors after line " +
				 LongintToString(read_SliceDataTableDriver->GetRecordIndex()) + " (slice " +
				 FileService::GetURIUserLabel(svDataFileNames.GetAt(read_nDataFileIndex)) + ")");
		}
	}

	// Fermeture si necessaire du fichier courant
	if (bOk and read_SliceDataTableDriver->IsEnd())
		bOk = PhysicalCloseCurrentChunk();
	return bOk;
}

boolean KWDataTableSlice::PhysicalOpenCurrentChunk()
{
	boolean bOk = true;

	require(IsOpenedForRead());
	require(not read_SliceDataTableDriver->IsOpenedForRead());
	require(read_nDataFileIndex < svDataFileNames.GetSize());

	bOk = read_SliceDataTableDriver->OpenChunkForRead(svDataFileNames.GetAt(read_nDataFileIndex),
							  lvDataFileSizes.GetAt(read_nDataFileIndex));
	if (not bOk)
		AddError("Unable to open slice file " +
			 FileService::GetURIUserLabel(svDataFileNames.GetAt(read_nDataFileIndex)));
	return bOk;
}

boolean KWDataTableSlice::PhysicalCloseCurrentChunk()
{
	boolean bOk = true;

	require(IsOpenedForRead());
	require(not IsEnd());
	require(IsOpenedForRead());
	require(read_SliceDataTableDriver->IsOpenedForRead());

	// Fermeture du fichier courant
	bOk = read_SliceDataTableDriver->Close() and bOk;

	// Preparation pour le fichier suivant
	read_nDataFileIndex++;

	// On saut si necessaire si les fichiers suivants sont vides
	if (not IsEnd())
		PhysicalSkipEmptyChunks();
	return bOk;
}

void KWDataTableSlice::PhysicalSkipEmptyChunks()
{
	require(IsOpenedForRead());
	require(not IsEnd());
	require(read_SliceDataTableDriver->IsClosed());

	// On saute les fichiers de taille 0
	while (read_nDataFileIndex < lvDataFileSizes.GetSize() and lvDataFileSizes.GetAt(read_nDataFileIndex) == 0)
		read_nDataFileIndex++;
}

int KWDataTableSliceCompareLexicographicIndex(const void* elem1, const void* elem2)
{
	KWDataTableSlice* slice1;
	KWDataTableSlice* slice2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	slice1 = cast(KWDataTableSlice*, *(Object**)elem1);
	slice2 = cast(KWDataTableSlice*, *(Object**)elem2);

	// Difference
	nCompare = slice1->CompareLexicographicOrder(slice2);
	return nCompare;
}

int KWDataTableSliceCompareLexicographicSortCriterion(const void* elem1, const void* elem2)
{
	KWDataTableSlice* slice1;
	KWDataTableSlice* slice2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	slice1 = cast(KWDataTableSlice*, *(Object**)elem1);
	slice2 = cast(KWDataTableSlice*, *(Object**)elem2);

	// Difference
	nCompare = slice1->CompareLexicographicSortCriterion(slice2);
	return nCompare;
}

///////////////////////////////////////////////////
// Classe KWDataTableDriverSlice

KWDataTableDriverSlice::KWDataTableDriverSlice()
{
	bHeaderLineUsed = false;
	assert(cFieldSeparator == '\t');
}

KWDataTableDriverSlice::~KWDataTableDriverSlice() {}

void KWDataTableDriverSlice::ComputeOpenInformation(const KWClass* kwcSliceClass)
{
	require(GetClass() != NULL);
	require(GetClass()->IsCompiled());
	require(kwcSliceClass != NULL);
	require(kwcSliceClass->IsCompiled());
	require(not IsOpenInformationComputed());

	// Calcul des index de la classe du driver a alimenter avec la tranche
	ComputeSliceDataItemLoadIndexes(kwcSliceClass);

	// Creation du buffer de lecture une fois pour toutes
	inputBuffer = new InputBufferedFile;
	inputBuffer->SetFieldSeparator(cFieldSeparator);
	inputBuffer->SetHeaderLineUsed(bHeaderLineUsed);
	inputBuffer->SetBufferSize(nBufferedFileSize);
}

boolean KWDataTableDriverSlice::IsOpenInformationComputed() const
{
	return inputBuffer != NULL;
}

void KWDataTableDriverSlice::CleanOpenInformation()
{
	require(IsOpenInformationComputed());
	require(not IsOpenedForRead());

	// Nettoyage
	livDataItemLoadIndexes.SetSize(0);
	delete inputBuffer;
	inputBuffer = NULL;
}

boolean KWDataTableDriverSlice::OpenChunkForRead(const ALString& sDataFileName, longint lDataFileSize)
{
	boolean bOk;
	longint lInputFileSize;
	int nInputBufferSize;
	ALString sTmp;

	require(IsOpenInformationComputed());
	require(not IsOpenedForRead());
	require(sDataFileName != "");
	require(lDataFileSize >= 0);

	// Reinitialisation de la base de donnees
	ResetDatabaseFile();

	// Parametrage du nom du fichier
	SetDataTableName(sDataFileName);
	inputBuffer->SetFileName(GetDataTableName());

	// Parametrage de la taille du buffer
	if (lDataFileSize > BufferedFile::nDefaultBufferSize)
		nInputBufferSize = BufferedFile::nDefaultBufferSize;
	else
	{
		nInputBufferSize = (int)lDataFileSize;
		nInputBufferSize =
		    MemSegmentByteSize * ((nInputBufferSize + MemSegmentByteSize - 1) / MemSegmentByteSize);
		nInputBufferSize = min((int)BufferedFile::nDefaultBufferSize, nInputBufferSize);
	}
	inputBuffer->SetBufferSize(nInputBufferSize);

	// Ouverture du fichier
	bOk = inputBuffer->Open();

	// Verification de la taille du fichier
	if (bOk)
	{
		lInputFileSize = inputBuffer->GetFileSize();
		bOk = lDataFileSize == lInputFileSize;
		if (not bOk)
			AddError(sTmp + "Read size " + LongintToString(lInputFileSize) + " for expected size " +
				 LongintToString(lDataFileSize));
	}

	// Lecture d'un premier buffer
	if (bOk)
		bOk = UpdateInputBuffer();

	// Nettoyage si erreur
	if (not bOk and IsOpenedForRead())
		inputBuffer->Close();
	assert(bOk or not IsOpenedForRead());
	return inputBuffer->IsOpened();
}

boolean KWDataTableDriverSlice::IsOpenedForRead() const
{
	require(IsOpenInformationComputed());
	return inputBuffer->IsOpened();
}

boolean KWDataTableDriverSlice::Close()
{
	boolean bOk;

	require(IsOpenInformationComputed());
	require(IsOpenedForRead());

	bOk = inputBuffer->Close();
	ResetDatabaseFile();
	return bOk;
}

boolean KWDataTableDriverSlice::IsClosed() const
{
	return not inputBuffer->IsOpened();
}

boolean KWDataTableDriverSlice::ReadObject(KWObject* kwoObject)
{
	boolean bOk = true;
	char* sField;
	int nFieldError;
	boolean bEndOfLine;
	ALString sTmp;
	int nField;
	int nError;
	Continuous cValue;
	KWLoadIndex liLoadIndex;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWContinuousValueBlock* cvbCurrentValue;
	KWContinuousValueBlock* cvbValue;
	KWSymbolValueBlock* svbCurrentValue;
	KWSymbolValueBlock* svbValue;
	ALString sMessage;
	boolean bValueBlockOk;

	require(IsOpenedForRead());
	require(not bWriteMode);
	require(not inputBuffer->IsFileEnd());
	require(not inputBuffer->IsBufferEnd());
	require(kwoObject != NULL);
	require(kwoObject->GetClass() == GetClass());

	// On retourne NULL, sans message, si interruption utilisateur
	if (periodicTestInterruption.IsTestAllowed(lRecordIndex))
	{
		if (TaskProgression::IsInterruptionRequested())
			return false;
	}

	// Lecture des champs de la ligne
	bEndOfLine = false;
	nField = 0;
	sField = NULL;
	nFieldError = inputBuffer->FieldNoError;
	lRecordIndex++;
	while (not bEndOfLine)
	{
		// Acces a l'index du champ dans la classe de l'objet
		if (nField < livDataItemLoadIndexes.GetSize())
			liLoadIndex = livDataItemLoadIndexes.GetAt(nField);

		// Index du champs suivant
		nField++;

		// On lit toujours le champ ou oon le saute selon que le champ soit traite ou non
		if (liLoadIndex.IsValid())
			bEndOfLine = inputBuffer->GetNextField(sField, nFieldError);
		else
			bEndOfLine = inputBuffer->SkipField();

		// Analyse des attributs a traiter
		if (liLoadIndex.IsValid())
		{
			// Acces au dataItem correspondant a l'index de chargement
			dataItem = kwcClass->GetDataItemAtLoadIndex(liLoadIndex);

			// Erreur ou warning si probleme sur le champ
			if (nFieldError != inputBuffer->FieldNoError)
			{
				// Erreur si probleme de double quote (normalement correctement gere lors de l'ecriture)
				if (nFieldError == inputBuffer->FieldMiddleDoubleQuote or
				    nFieldError == inputBuffer->FieldMissingEndDoubleQuote)
				{
					AddError(sTmp + "Field " + IntToString(nField) + ", " +
						 dataItem->GetClassLabel() + " " + dataItem->GetObjectLabel() +
						 " with value <" + InputBufferedFile::GetDisplayValue(sField) +
						 "> : " + inputBuffer->GetFieldErrorLabel(nFieldError));
					bOk = false;
				}
				// Warning sinon (un champ peut par exemple etre trop long s'il a ete cree par une regle
				// de derivation)
				else
					AddWarning(sTmp + "Field " + IntToString(nField) + ", " +
						   dataItem->GetClassLabel() + " " + dataItem->GetObjectLabel() +
						   " with value <" + InputBufferedFile::GetDisplayValue(sField) +
						   "> : " + inputBuffer->GetFieldErrorLabel(nFieldError));
			}

			// Cas d'un attribut
			if (dataItem->IsAttribute())
			{
				// Acces aux caracteristiques de l'attribut
				attribute = cast(KWAttribute*, dataItem);
				assert(attribute->GetDerivationRule() == NULL);
				assert(KWType::IsSimple(attribute->GetType()));

				// Cas attribut Symbol
				if (attribute->GetType() == KWType::Symbol)
				{
					kwoObject->SetSymbolValueAt(liLoadIndex, Symbol(sField));
				}
				// Cas attribut Continuous
				else
				{
					assert(attribute->GetType() == KWType::Continuous);

					// Conversion en Continuous
					// (pas de transformation necessaire de l'eventuel separateur decimal ', ' en
					// '.')
					nError = KWContinuous::StringToContinuousError(sField, cValue);
					kwoObject->SetContinuousValueAt(liLoadIndex, cValue);

					// Test de validite du champs
					if (nError != 0)
					{
						AddError(sTmp + "Field " + IntToString(nField) + ", " +
							 "Numerical variable " + attribute->GetName() +
							 " with invalid value <" +
							 InputBufferedFile::GetDisplayValue(sField) +
							 ">: " + KWContinuous::ErrorLabel(nError));
						bOk = false;
					}
				}
			}
			// Cas d'un bloc d'attributs
			else
			{
				// Acces aux caracteristiques du bloc d'attributs
				attributeBlock = cast(KWAttributeBlock*, dataItem);
				assert(attributeBlock->GetDerivationRule() == NULL);

				// Cas des blocs d'attributs Symbol
				bValueBlockOk = true;
				if (attributeBlock->GetType() == KWType::Symbol)
				{
					// Lecture et alimentation d'un bloc de valeurs
					svbValue = KWSymbolValueBlock::BuildBlockFromField(
					    attributeBlock->GetLoadedAttributesIndexedKeyBlock(), sField,
					    attributeBlock->GetSymbolDefaultValue(), bValueBlockOk, sMessage);

					// Alimentation si le bloc n'est pas initialise
					if (not kwoObject->CheckSymbolValueBlockAt(liLoadIndex))
						kwoObject->SetSymbolValueBlockAt(liLoadIndex, svbValue);
					// Concatenation au bloc existant sinon
					else
					{
						svbCurrentValue = kwoObject->GetSymbolValueBlockAt(liLoadIndex);

						// On garde le bloc courant si le nouveau bloc est vide
						if (svbValue->GetValueNumber() == 0)
							delete svbValue;
						// On remplace le bloc courant s'il est vide
						else if (svbCurrentValue->GetValueNumber() == 0)
							kwoObject->UpdateSymbolValueBlockAt(liLoadIndex, svbValue);
						// On remplace par la concatenation des deux blocs s'il sont tous les
						// deux non vide
						else
						{
							kwoObject->UpdateSymbolValueBlockAt(
							    liLoadIndex, KWSymbolValueBlock::ConcatValueBlocks(
									     svbCurrentValue, svbValue));
							delete svbValue;
						}
					}
				}
				// Cas des blocs d'attributs Continuous
				else if (attributeBlock->GetType() == KWType::Continuous)
				{
					// Lecture et alimentation d'un bloc de valeurs
					cvbValue = KWContinuousValueBlock::BuildBlockFromField(
					    attributeBlock->GetLoadedAttributesIndexedKeyBlock(), sField,
					    attributeBlock->GetContinuousDefaultValue(), bValueBlockOk, sMessage);

					// Alimentation si le bloc n'est pas initialise
					if (not kwoObject->CheckContinuousValueBlockAt(liLoadIndex))
						kwoObject->SetContinuousValueBlockAt(liLoadIndex, cvbValue);
					// Concatenation au bloc existant sinon
					else
					{
						cvbCurrentValue = kwoObject->GetContinuousValueBlockAt(liLoadIndex);

						// On garde le bloc courant si le nouveau bloc est vide
						if (cvbValue->GetValueNumber() == 0)
							delete cvbValue;
						// On remplace le bloc courant s'il est vide
						else if (cvbCurrentValue->GetValueNumber() == 0)
							kwoObject->UpdateContinuousValueBlockAt(liLoadIndex, cvbValue);
						// On remplace par la concatenation des deux blocs s'il sont tous les
						// deux non vide
						else
						{
							kwoObject->UpdateContinuousValueBlockAt(
							    liLoadIndex, KWContinuousValueBlock::ConcatValueBlocks(
									     cvbCurrentValue, cvbValue));
							delete cvbValue;
						}
					}
				}

				// Warning si erreur de parsing des valeurs du bloc de variables
				if (not bValueBlockOk)
				{
					AddError(sTmp + "Field " + IntToString(nField) + ", " +
						 "Sparse variable block " + attributeBlock->GetName() +
						 " with invalid value <" + InputBufferedFile::GetDisplayValue(sField) +
						 "> because of parsing error (" + sMessage + ")");
					bOk = false;
				}
			}
		}
	}

	// Verification du nombre de champs
	if (nField != livDataItemLoadIndexes.GetSize())
	{
		// Emission d'un warning
		AddError(sTmp + " Incorrect record, with bad field number (" + IntToString(nField) +
			 " read fields for " + IntToString(livDataItemLoadIndexes.GetSize()) + " expected fields)");
		bOk = false;
	}

	// Remplissage du buffer si necessaire
	UpdateInputBuffer();
	bOk = not IsError();
	return bOk;
}

void KWDataTableDriverSlice::ComputeSliceDataItemLoadIndexes(const KWClass* kwcSliceClass)
{
	boolean bDisplay = false;
	int i;
	ObjectArray oaNativeLogicalDataItems;
	int nSliceDataItemIndex;
	KWDataItem* sliceDataItem;
	KWAttribute* sliceAttribute;
	KWAttributeBlock* sliceAttributeBlock;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	ALString sAttributeName;
	ALString sTmp;

	require(GetClass() != NULL);
	require(GetClass()->IsCompiled());
	require(kwcSliceClass != NULL);
	require(kwcSliceClass->IsCompiled());
	require(kwcSliceClass->GetUsedAttributeNumber() == kwcSliceClass->GetAttributeNumber());
	require(kwcSliceClass->GetUsedAttributeNumberForType(KWType::Continuous) +
		    kwcSliceClass->GetUsedAttributeNumberForType(KWType::Symbol) ==
		kwcSliceClass->GetAttributeNumber());
	require(livDataItemLoadIndexes.GetSize() == 0);

	// On dimensionne le vecteur d'index a collecter en fonction de la taille de la classe de la tranche
	livDataItemLoadIndexes.SetSize(kwcSliceClass->GetNativeDataItemNumber());

	// Collecte des champs natifs de la classe de la tranche
	nSliceDataItemIndex = 0;
	sliceAttribute = kwcSliceClass->GetHeadAttribute();
	while (sliceAttribute != NULL)
	{
		assert(sliceAttribute->IsNative());

		// Memorisation si attribut natif
		if (not sliceAttribute->IsInBlock())
		{
			// Memorisation si attribut charge en memoire
			if (sliceAttribute->GetLoaded())
			{
				// Recherche de l'attribut physique correspondant
				attribute = kwcClass->LookupAttribute(sliceAttribute->GetName());
				assert(attribute != NULL);
				assert(attribute->GetType() == sliceAttribute->GetType());
				assert(attribute->IsNative());
				assert(attribute->GetLoaded());

				// Memorisation de l'index de l'attribut dans la classe du driver
				livDataItemLoadIndexes.SetAt(nSliceDataItemIndex, attribute->GetLoadIndex());
			}
			else
				livDataItemLoadIndexes.ResetAt(nSliceDataItemIndex);

			// Passage au data item suivant
			nSliceDataItemIndex++;
		}
		// Test pour les blocs d'attribut, uniquement pour le premier attribut de chaque bloc
		else if (sliceAttribute->IsFirstInBlock())
		{
			sliceAttributeBlock = sliceAttribute->GetAttributeBlock();
			assert(sliceAttributeBlock->IsNative());

			// Memorisation si bloc d'attribut charge en memoire
			if (sliceAttributeBlock->GetLoaded())
			{
				// Recherche de l'attribut physique correspondant
				attributeBlock = kwcClass->LookupAttributeBlock(sliceAttributeBlock->GetName());
				assert(attributeBlock != NULL);
				assert(attributeBlock->GetType() == sliceAttributeBlock->GetType());
				assert(attributeBlock->IsNative());
				assert(attributeBlock->GetLoaded());

				// Memorisation de l'index de l'attribut dans la classe du driver
				livDataItemLoadIndexes.SetAt(nSliceDataItemIndex, attributeBlock->GetLoadIndex());
			}
			else
				livDataItemLoadIndexes.ResetAt(nSliceDataItemIndex);

			// Passage au data item suivant
			nSliceDataItemIndex++;
		}

		// Attribut suivant
		kwcSliceClass->GetNextAttribute(sliceAttribute);
	}
	assert(nSliceDataItemIndex == livDataItemLoadIndexes.GetSize());

	// Affichage du resultat d'indexation
	if (bDisplay)
	{
		cout << "Compute data item indexes of dictionary " << kwcClass->GetName() << endl;
		cout << " Slice dictionary " << kwcSliceClass->GetName() << endl;
		for (i = 0; i < kwcSliceClass->GetLoadedDataItemNumber(); i++)
		{
			sliceDataItem = kwcSliceClass->GetLoadedDataItemAt(i);
			dataItem = kwcClass->LookupDataItem(sliceDataItem->GetName());
			cout << "\t" << i;
			cout << "\t" << sliceDataItem->GetName();
			cout << "\t" << livDataItemLoadIndexes.GetAt(i);
			if (dataItem != NULL)
				cout << "\t" << dataItem->GetName();
			cout << endl;
		}
	}
}

boolean KWDataTableDriverSlice::BuildDataTableClass(KWClass* kwcDataTableClass)
{
	assert(false);
	return false;
}

boolean KWDataTableDriverSlice::OpenForRead(const KWClass* kwcLogicalClass)
{
	assert(false);
	return false;
}

boolean KWDataTableDriverSlice::OpenForWrite()
{
	assert(false);
	return false;
}

boolean KWDataTableDriverSlice::IsOpenedForWrite() const
{
	assert(false);
	return false;
}

KWObject* KWDataTableDriverSlice::Read()
{
	assert(false);
	return NULL;
}

void KWDataTableDriverSlice::Write(const KWObject* kwoObject)
{
	assert(false);
}

///////////////////////////////////////////////////
// Classe PLShared_DataTableSliceSet

PLShared_DataTableSliceSet::PLShared_DataTableSliceSet() {}

PLShared_DataTableSliceSet::~PLShared_DataTableSliceSet() {}

void PLShared_DataTableSliceSet::SetDataTableSliceSet(KWDataTableSliceSet* dataTableSliceSet)
{
	require(dataTableSliceSet != NULL);
	SetObject(dataTableSliceSet);
}

KWDataTableSliceSet* PLShared_DataTableSliceSet::GetDataTableSliceSet()
{
	return cast(KWDataTableSliceSet*, GetObject());
}

void PLShared_DataTableSliceSet::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDataTableSliceSet* dataTableSliceSet;
	PLShared_ObjectArray shared_oa(new PLShared_DataTableSlice);

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Acces a l'objet a deserialiser
	dataTableSliceSet = cast(KWDataTableSliceSet*, o);
	dataTableSliceSet->Clean();

	// Deserialisation
	dataTableSliceSet->bDeleteFilesAtClean = serializer->GetBoolean();
	dataTableSliceSet->sClassName = serializer->GetString();
	dataTableSliceSet->sClassTargetAttributeName = serializer->GetString();
	dataTableSliceSet->nTotalInstanceNumber = serializer->GetInt();
	serializer->GetIntVector(&(dataTableSliceSet->ivChunkInstanceNumbers));
	shared_oa.DeserializeObject(serializer, &dataTableSliceSet->oaSlices);
}

void PLShared_DataTableSliceSet::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDataTableSliceSet* dataTableSliceSet;
	PLShared_ObjectArray shared_oa(new PLShared_DataTableSlice);

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Acces a l'objet a serialiser
	dataTableSliceSet = cast(KWDataTableSliceSet*, o);

	// Serialisation
	serializer->PutBoolean(dataTableSliceSet->GetDeleteFilesAtClean());
	serializer->PutString(dataTableSliceSet->GetClassName());
	serializer->PutString(dataTableSliceSet->GetTargetAttributeName());
	serializer->PutInt(dataTableSliceSet->GetTotalInstanceNumber());
	serializer->PutIntVector(&(dataTableSliceSet->ivChunkInstanceNumbers));
	shared_oa.SerializeObject(serializer, &dataTableSliceSet->oaSlices);
}

Object* PLShared_DataTableSliceSet::Create() const
{
	return new KWDataTableSliceSet;
}

///////////////////////////////////////////////////
// Classe PLShared_DataTableSlice

PLShared_DataTableSlice::PLShared_DataTableSlice() {}

PLShared_DataTableSlice::~PLShared_DataTableSlice() {}

void PLShared_DataTableSlice::SetDataTableSlice(KWDataTableSlice* dataTableSlice)
{
	require(dataTableSlice != NULL);
	SetObject(dataTableSlice);
}

KWDataTableSlice* PLShared_DataTableSlice::GetDataTableSlice()
{
	return cast(KWDataTableSlice*, GetObject());
}

void PLShared_DataTableSlice::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWDataTableSlice* dataTableSlice;
	KWClass* kwcDataTableClass;
	PLShared_MetaData shared_MetaData;
	PLShared_LoadIndexVector shared_livDataItemLoadIndexes;
	int nAttributeNumber;
	KWAttribute* attribute;
	int nBlockNumber;
	ALString sBlockName;
	KWAttributeBlock* attributeBlock;
	KWAttribute* firstAttribute;
	KWAttribute* lastAttribute;
	int i;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Acces a l'objet a deserialiser
	dataTableSlice = cast(KWDataTableSlice*, o);
	assert(dataTableSlice->GetObjects()->GetSize() == 0);

	// Deserialisation de l'index lexicographique
	serializer->GetIntVector(&(dataTableSlice->ivLexicographicIndex));

	// Nettoyage prealable de la classe
	kwcDataTableClass = &(dataTableSlice->kwcClass);
	kwcDataTableClass->DeleteAllAttributes();

	// Deserialisation de la classe
	kwcDataTableClass->SetName(serializer->GetString());
	shared_MetaData.DeserializeObject(serializer, kwcDataTableClass->GetMetaData());

	// Deserialisation des attributs de la classe
	nAttributeNumber = serializer->GetInt();
	for (i = 0; i < nAttributeNumber; i++)
	{
		attribute = new KWAttribute;
		attribute->SetName(serializer->GetString());
		attribute->SetType(serializer->GetInt());
		attribute->SetCost(serializer->GetDouble());
		shared_MetaData.DeserializeObject(serializer, attribute->GetMetaData());
		kwcDataTableClass->InsertAttribute(attribute);
	}
	assert(kwcDataTableClass->GetAttributeNumber() == nAttributeNumber);

	// Deserialisation des blocs d'attributs de la classe
	nBlockNumber = serializer->GetInt();
	for (i = 0; i < nBlockNumber; i++)
	{
		sBlockName = serializer->GetString();
		firstAttribute = kwcDataTableClass->LookupAttribute(serializer->GetString());
		lastAttribute = kwcDataTableClass->LookupAttribute(serializer->GetString());
		attributeBlock = kwcDataTableClass->CreateAttributeBlock(sBlockName, firstAttribute, lastAttribute);
		shared_MetaData.DeserializeObject(serializer, attributeBlock->GetMetaData());
	}

	// Indexation de la classe
	kwcDataTableClass->IndexClass();

	// Deserialisation des noms et tailles de fichier
	serializer->GetStringVector(&(dataTableSlice->svDataFileNames));
	serializer->GetLongintVector(&(dataTableSlice->lvDataFileSizes));

	// Deserialisation des vecteurs de statistique sur les valeurs
	serializer->GetLongintVector(&(dataTableSlice->lvAttributeBlockValueNumbers));
	serializer->GetLongintVector(&(dataTableSlice->lvDenseSymbolAttributeDiskSizes));

	// Deserialisation des vecteurs d'index
	shared_livDataItemLoadIndexes.DeserializeObject(serializer, dataTableSlice->GetDataItemLoadIndexes());
	serializer->GetIntVector(dataTableSlice->GetValueBlockFirstSparseIndexes());
	serializer->GetIntVector(dataTableSlice->GetValueBlockLastSparseIndexes());
}

void PLShared_DataTableSlice::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWDataTableSlice* dataTableSlice;
	KWClass* kwcDataTableClass;
	PLShared_MetaData shared_MetaData;
	PLShared_LoadIndexVector shared_livDataItemLoadIndexes;
	KWAttribute* attribute;
	int nBlockNumber;
	KWAttributeBlock* attributeBlock;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Acces a l'objet a serialiser
	dataTableSlice = cast(KWDataTableSlice*, o);
	assert(dataTableSlice->GetObjects()->GetSize() == 0);

	// Serialisation de l'index lexicographique
	serializer->PutIntVector(&(dataTableSlice->ivLexicographicIndex));

	// Serialisation de la classe
	kwcDataTableClass = &(dataTableSlice->kwcClass);
	serializer->PutString(kwcDataTableClass->GetName());
	shared_MetaData.SerializeObject(serializer, kwcDataTableClass->GetConstMetaData());

	// Serialisation des attributs de la classe
	serializer->PutInt(kwcDataTableClass->GetAttributeNumber());
	attribute = kwcDataTableClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		serializer->PutString(attribute->GetName());
		serializer->PutInt(attribute->GetType());
		serializer->PutDouble(attribute->GetCost());
		shared_MetaData.SerializeObject(serializer, attribute->GetConstMetaData());
		kwcDataTableClass->GetNextAttribute(attribute);
	}

	// Comptage du nombre de blocks
	nBlockNumber = 0;
	attributeBlock = kwcDataTableClass->GetHeadAttributeBlock();
	while (attributeBlock != NULL)
	{
		nBlockNumber++;
		kwcDataTableClass->GetNextAttributeBlock(attributeBlock);
	}

	// Serialisation des blocs d'attributs de la classe
	serializer->PutInt(nBlockNumber);
	attributeBlock = kwcDataTableClass->GetHeadAttributeBlock();
	while (attributeBlock != NULL)
	{
		serializer->PutString(attributeBlock->GetName());
		serializer->PutString(attributeBlock->GetFirstAttribute()->GetName());
		serializer->PutString(attributeBlock->GetLastAttribute()->GetName());
		shared_MetaData.SerializeObject(serializer, attributeBlock->GetConstMetaData());
		kwcDataTableClass->GetNextAttributeBlock(attributeBlock);
	}

	// Serialisation des noms et tailles de fichier
	serializer->PutStringVector(&(dataTableSlice->svDataFileNames));
	serializer->PutLongintVector(&(dataTableSlice->lvDataFileSizes));

	// Serialisation des vecteurs de statistique sur les valeurs
	serializer->PutLongintVector(&(dataTableSlice->lvAttributeBlockValueNumbers));
	serializer->PutLongintVector(&(dataTableSlice->lvDenseSymbolAttributeDiskSizes));

	// Serialisation des vecteurs d'index
	shared_livDataItemLoadIndexes.SerializeObject(serializer, dataTableSlice->GetDataItemLoadIndexes());
	serializer->PutIntVector(dataTableSlice->GetValueBlockFirstSparseIndexes());
	serializer->PutIntVector(dataTableSlice->GetValueBlockLastSparseIndexes());
}

Object* PLShared_DataTableSlice::Create() const
{
	return new KWDataTableSlice;
}