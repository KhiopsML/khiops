// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTAttributeSelection.h"

int DTTreeAttributeLevelCampare(const void* elem1, const void* elem2);
int DTTreeAttributeRankCampare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////
// Classe DTAttributeSelection

DTAttributeSelection::DTAttributeSelection()
{
	nIndex = 0;
	nRandomSeed = 1789;
	nUsableAttributesNumber = 0;
	oaTreeAttributeSelection.RemoveAll();
	oaTreeAttributeSelection.SetCompareFunction(DTTreeAttributeCompareName);
}

DTAttributeSelection::~DTAttributeSelection()
{
	oaTreeAttributeSelection.RemoveAll();
}

void DTAttributeSelection::Write(ostream& ost) const
{
	DTTreeAttribute* taAttribute;
	// KWAttributeStats* attributeStats;
	int nAttribute;
	// Affichages des attributs
	ost << "  DTAttributeSelection \n";
	ost << "  Variables\n";
	for (nAttribute = 0; nAttribute < oaTreeAttributeSelection.GetSize(); nAttribute++)
	{
		taAttribute = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(nAttribute));
		ost << "\t" << taAttribute->GetName() << "\n";
	}
}

boolean DTAttributeSelection::Check() const
{
	DTTreeAttribute* taAttribute;
	// KWAttributeStats* attributeStats;
	ObjectDictionary odlistattribute;
	int nAttribute;

	for (nAttribute = 0; nAttribute < oaTreeAttributeSelection.GetSize(); nAttribute++)
	{
		taAttribute = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(nAttribute));
		odlistattribute.SetAt(taAttribute->GetName(), taAttribute->aAttribute);
	}

	if (oaTreeAttributeSelection.GetSize() == odlistattribute.GetCount())
		return true;
	return false;
}

void DTAttributeSelection::SetSeed(int nSeed)
{
	nRandomSeed = nSeed;
}

int DTAttributeSelection::GetSeed()
{
	return nRandomSeed;
}

void DTAttributeSelection::SetIndex(int nValue)
{
	require(nValue >= 0);
	nIndex = nValue;
}

int DTAttributeSelection::GetIndex() const
{
	return nIndex;
}

const ObjectArray* DTAttributeSelection::GetTreeAttributeSelection() const
{
	return &oaTreeAttributeSelection;
}

void DTAttributeSelection::AddAttribute(DTTreeAttribute* attribute)
{
	oaTreeAttributeSelection.Add(attribute);
}

DTTreeAttribute* DTAttributeSelection::GetAttributeAt(int npos) const
{
	return (DTTreeAttribute*)oaTreeAttributeSelection.GetAt(npos);
}

const ALString& DTAttributeSelection::GetAttributeNameAt(int npos) const
{
	DTTreeAttribute* dtattribut = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(npos));

	return dtattribut->GetName();
}

int DTAttributeSelection::GetSize() const
{
	return oaTreeAttributeSelection.GetSize();
}

ObjectArray* DTAttributeSelection::GetAttributeSelection() const
{
	int nAttribute, nMax;
	DTTreeAttribute* dtAttribute;
	ObjectArray* oaretour = new ObjectArray;

	nMax = oaTreeAttributeSelection.GetSize();

	require(AreTreeAttributesSortedByBlock(&oaTreeAttributeSelection));

	for (nAttribute = 0; nAttribute < nMax; nAttribute++)
	{
		dtAttribute = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(nAttribute));
		oaretour->Add(dtAttribute->aAttribute);
	}

	return oaretour;
}

const KWAttributeBlock* DTAttributeSelection::GetFirstBlock() const
{
	DTTreeAttribute* attribute1 = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(0));
	DTTreeAttribute* attribute2 = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(1));

	require(GetSize() >= 2);
	require(attribute1 != NULL);
	require(attribute2 != NULL);
	require(attribute1->aAttribute != NULL);
	require(attribute2->aAttribute != NULL);
	require(attribute1->aAttribute != attribute2->aAttribute);

	if (attribute1->aAttribute->GetAttributeBlock() == NULL)
		return attribute2->aAttribute->GetAttributeBlock();
	else if (attribute2->aAttribute->GetAttributeBlock() == NULL)
		return attribute1->aAttribute->GetAttributeBlock();
	else if (attribute1->aAttribute->GetAttributeBlock()->GetName() <
		 attribute2->aAttribute->GetAttributeBlock()->GetName())
		return attribute1->aAttribute->GetAttributeBlock();
	else
		return attribute2->aAttribute->GetAttributeBlock();
}

const KWAttributeBlock* DTAttributeSelection::GetSecondBlock() const
{
	const KWAttributeBlock* firstAttributeBlock;
	DTTreeAttribute* attribute1 = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(0));
	DTTreeAttribute* attribute2 = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(1));

	firstAttributeBlock = GetFirstBlock();
	if (attribute1->aAttribute->GetAttributeBlock() == firstAttributeBlock)
		return attribute2->aAttribute->GetAttributeBlock();
	else
		return attribute1->aAttribute->GetAttributeBlock();
}

DTTreeAttribute* DTAttributeSelection::GetFirstBlockAttribute() const
{
	const KWAttributeBlock* firstAttributeBlock;
	DTTreeAttribute* attribute1 = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(0));
	DTTreeAttribute* attribute2 = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(1));

	firstAttributeBlock = GetFirstBlock();
	if (attribute1->aAttribute->GetAttributeBlock() == firstAttributeBlock)
		return attribute1;
	else
		return attribute2;
}

DTTreeAttribute* DTAttributeSelection::GetSecondBlockAttribute() const
{
	const KWAttributeBlock* firstAttributeBlock;
	DTTreeAttribute* attribute1 = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(0));
	DTTreeAttribute* attribute2 = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(1));

	firstAttributeBlock = GetFirstBlock();
	if (attribute1->aAttribute->GetAttributeBlock() == firstAttributeBlock)
		return attribute2;
	else
		return attribute1;
}

ObjectArray* DTAttributeSelection::GetTreeAttributesShuffled(const int nMaxAttributesNumber)
{
	assert(nMaxAttributesNumber >= 0);
	DTTreeAttribute* dtAttribute;
	int nAttribute, nMax;

	if (nMaxAttributesNumber <= 0)
		return NULL;

	ObjectArray* AttributesShuffledSelection = new ObjectArray;

	oaTreeAttributeSelection.Shuffle();

	nMax = (nMaxAttributesNumber > oaTreeAttributeSelection.GetSize()) ? oaTreeAttributeSelection.GetSize()
									   : nMaxAttributesNumber;

	for (nAttribute = 0; nAttribute < nMax; nAttribute++)
	{
		dtAttribute = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(nAttribute));
		AttributesShuffledSelection->Add(dtAttribute->aAttribute);
	}

	// Retour du vecteur avec size <= nMaxAttributesNumber
	return AttributesShuffledSelection;
}

ObjectArray* DTAttributeSelection::GetAttributesShuffled(const int nMaxAttributesNumber)
{
	assert(nMaxAttributesNumber >= 0);
	DTTreeAttribute* dtAttribute;
	int nAttribute, nMax;

	if (nMaxAttributesNumber <= 0)
		return NULL;

	ObjectArray* AttributesShuffledSelection = new ObjectArray;

	oaTreeAttributeSelection.Shuffle();

	nMax = (nMaxAttributesNumber > oaTreeAttributeSelection.GetSize()) ? oaTreeAttributeSelection.GetSize()
									   : nMaxAttributesNumber;

	for (nAttribute = 0; nAttribute < nMax; nAttribute++)
	{
		dtAttribute = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(nAttribute));
		AttributesShuffledSelection->Add(dtAttribute->aAttribute);
	}

	// Retour du vecteur avec size <= nMaxAttributesNumber
	return AttributesShuffledSelection;
}

ObjectArray* DTAttributeSelection::SortObjectArrayFromContinuous(const int nMaxAttributesNumber, DoubleVector& vLevels,
								 ObjectArray& oaListAttributes)
{
	// KWAttribute* aAttribute;
	int nMax;
	ObjectArray* AttributesShuffledSelection = new ObjectArray;

	if (nMaxAttributesNumber <= 0)
		return NULL;

	int i;
	int nIndexesNumber;
	double dRandom;
	double dKey;
	KWSortableValue* svKey;
	SortedList slKeys(KWSortableValueCompare);
	IntVector ivSortedListIndexes;

	ivSortedListIndexes.SetSize(0);

	nMax = (nMaxAttributesNumber > oaListAttributes.GetSize()) ? oaListAttributes.GetSize() : nMaxAttributesNumber;

	// Parcours de toutes les variables
	for (i = 0; i < vLevels.GetSize(); i++)
	{
		// Tirage uniforme sur [0,1]
		dRandom = RandomDouble();

		// Calcul de la cle
		require(vLevels.GetAt(i) != 0);
		dKey = pow(dRandom, 1 / vLevels.GetAt(i));

		// Insertion dans la liste triee par cle
		svKey = new KWSortableValue;
		svKey->SetIndex(i);
		svKey->SetSortValue(dKey);
		slKeys.Add(svKey);

		// Elagage : on ne garde qu'un reservoir de taille nSelectedAttributesNumber
		if (slKeys.GetCount() > nMax and slKeys.GetCount() > 0)
		{
			svKey = cast(KWSortableValue*, slKeys.GetHead());
			slKeys.RemoveHead();
			delete svKey;
		}
	}

	// Memorisation des nSelectedAttributesNumber meilleures cles
	for (nIndexesNumber = 0; nIndexesNumber < nMax; nIndexesNumber++)
	{
		if (nIndexesNumber == vLevels.GetSize())
			break; // le max a ete atteint

		// Extraction de la tete de liste
		svKey = cast(KWSortableValue*, slKeys.GetTail());
		assert(svKey != NULL);

		slKeys.RemoveTail();

		// Supression d'une instance
		i = svKey->GetIndex();
		ivSortedListIndexes.Add(i);
		delete svKey;
	}

	ivSortedListIndexes.Sort();
	slKeys.DeleteAll();

	for (i = 0; i < ivSortedListIndexes.GetSize(); i++)
	{
		// dtAttribute = cast(DTTreeAttribute*, oaAttributeSelection.GetAt(ivSortedListIndexes.GetAt(i));
		AttributesShuffledSelection->Add(oaListAttributes.GetAt(ivSortedListIndexes.GetAt(i)));
	}

	// cout << endl << "result list attribut bug : " << endl;
	// result->Write(cout);

	return AttributesShuffledSelection;
}

// parcourt une liste d'attributs, effectue un tirage aleatoire en fonction du level de ces attributs, et
// renvoie (au maximum) les 'maxAttributesNumber' index de chargements des attributs de plus fort level
ObjectArray* DTAttributeSelection::GetTreeAttributesFromLevels(const int nMaxAttributesNumber)
{
	// Repris de l'algo de Weighted Random Sampling with a reservoir Information Processing Letters 97(2006) 181-185
	// Pavlos S. Efraimidis, Paul G. Spirakis Interet : en une passe, directement parallelisable, utilise les poids
	// non normalises, Complexite en O(K*log(Ks)) on pourrait potentiellement avoir une infinite de variables car on
	// maintient uniquement les Ks meilleures cles Dans l'article une version avec "reservoir sampling" est presente
	// egalement qui permet de selectionner dans un flux de variables Implementation :
	// - pour chaque variable, on tire u_k aleatoirement dans [0,1] et on calcule la cle c_k=u_k^(1/poids_k)
	// - on stocke les Ks meilleures cles dans une Sorted List
	// - a la fin on selectionne les indices de ces Ks meilleurs cles

	// cout << "GetAttributesLoadIndexesFromLevels : maxAttributesNumber = " << maxAttributesNumber << " , vLevels
	// size = " << vLevels.GetSize() << ", oaListAttributes size = " << oaListAttributes.GetSize() << endl;

	DTTreeAttribute* dtAttribute;
	int nAttribute, nMax;

	if (nMaxAttributesNumber <= 0)
		return NULL;

	SortedList slKeys(KWSortableValueCompare);
	IntVector ivSortedListIndexes;
	DoubleVector vLevels;
	ObjectArray oaListAttributes;

	ivSortedListIndexes.SetSize(0);

	nMax = (nMaxAttributesNumber > oaTreeAttributeSelection.GetSize()) ? oaTreeAttributeSelection.GetSize()
									   : nMaxAttributesNumber;

	for (nAttribute = 0; nAttribute < nMaxAttributesNumber; nAttribute++)
	{
		dtAttribute = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(nAttribute));
		vLevels.Add(dtAttribute->dLevel);
		oaListAttributes.Add(dtAttribute->aAttribute);
	}
	return DTAttributeSelection::SortObjectArrayFromContinuous(nMaxAttributesNumber, vLevels, oaListAttributes);
}

// parcourt une liste d'attributs, effectue un tirage aleatoire en fonction du level de ces attributs, et
// renvoie (au maximum) les 'maxAttributesNumber' index de chargements des attributs de plus fort level
ObjectArray* DTAttributeSelection::GetAttributesFromLevels(const int nMaxAttributesNumber)
{
	// Repris de l'algo de Weighted Random Sampling with a reservoir Information Processing Letters 97(2006) 181-185
	// Pavlos S. Efraimidis, Paul G. Spirakis Interet : en une passe, directement parallelisable, utilise les poids
	// non normalises, Complexite en O(K*log(Ks)) on pourrait potentiellement avoir une infinite de variables car on
	// maintient uniquement les Ks meilleures cles Dans l'article une version avec "reservoir sampling" est presente
	// egalement qui permet de selectionner dans un flux de variables Implementation :
	// - pour chaque variable, on tire u_k aleatoirement dans [0,1] et on calcule la cle c_k=u_k^(1/poids_k)
	// - on stocke les Ks meilleures cles dans une Sorted List
	// - a la fin on selectionne les indices de ces Ks meilleurs cles

	// cout << "GetAttributesLoadIndexesFromLevels : maxAttributesNumber = " << maxAttributesNumber << " , vLevels
	// size = " << vLevels.GetSize() << ", oaListAttributes size = " << oaListAttributes.GetSize() << endl;

	DTTreeAttribute* dtAttribute;
	int nAttribute, nMax;

	if (nMaxAttributesNumber <= 0)
		return NULL;

	SortedList slKeys(KWSortableValueCompare);
	IntVector ivSortedListIndexes;
	DoubleVector vLevels;
	ObjectArray oaListAttributes;

	ivSortedListIndexes.SetSize(0);

	nMax = (nMaxAttributesNumber > oaTreeAttributeSelection.GetSize()) ? oaTreeAttributeSelection.GetSize()
									   : nMaxAttributesNumber;

	for (nAttribute = 0; nAttribute < nMax; nAttribute++)
	{
		dtAttribute = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(nAttribute));
		vLevels.Add(dtAttribute->dLevel);
		oaListAttributes.Add(dtAttribute->aAttribute);
	}
	return DTAttributeSelection::SortObjectArrayFromContinuous(nMaxAttributesNumber, vLevels, oaListAttributes);
}

int DTAttributeSelection::CompareBlocks(const DTAttributeSelection* otherAttributePair)
{
	const KWAttributeBlock* firstAttributeBlock;
	const KWAttributeBlock* secondAttributeBlock;
	const KWAttributeBlock* otherFirstAttributeBlock;
	const KWAttributeBlock* otherSecondAttributeBlock;
	int nCompare;

	require(otherAttributePair != NULL);

	// Acces aux blocs de la paire courante
	firstAttributeBlock = GetFirstBlock();
	secondAttributeBlock = GetSecondBlock();

	// Acces aux blocs de l'autre paire
	otherFirstAttributeBlock = otherAttributePair->GetFirstBlock();
	otherSecondAttributeBlock = otherAttributePair->GetSecondBlock();

	// Comparaison sur le premier bloc de chaque paire
	if (firstAttributeBlock == otherFirstAttributeBlock)
		nCompare = 0;
	else if (firstAttributeBlock == NULL)
		nCompare = 1;
	else if (otherFirstAttributeBlock == NULL)
		nCompare = -1;
	else
	{
		nCompare =
		    -(firstAttributeBlock->GetAttributeNumber() - otherFirstAttributeBlock->GetAttributeNumber());
		if (nCompare == 0)
			nCompare = firstAttributeBlock->GetName().Compare(otherFirstAttributeBlock->GetName());
	}

	// Comparaison sur le second bloc de chaque paire
	if (nCompare == 0)
	{
		if (secondAttributeBlock == otherSecondAttributeBlock)
			nCompare = 0;
		else if (secondAttributeBlock == NULL)
			nCompare = 1;
		else if (otherSecondAttributeBlock == NULL)
			nCompare = -1;
		else
		{
			nCompare = -(secondAttributeBlock->GetAttributeNumber() -
				     otherSecondAttributeBlock->GetAttributeNumber());
			if (nCompare == 0)
				nCompare =
				    secondAttributeBlock->GetName().Compare(otherSecondAttributeBlock->GetName());
		}
	}

	// Comparaison sur l'attribut du deuxieme bloc, qui conditionne l'appatetance a un meme groupe de paire
	if (nCompare == 0)
		nCompare = GetSecondBlockAttribute()->GetName().Compare(
		    otherAttributePair->GetSecondBlockAttribute()->GetName());

	// Comparaison sur l'attribut du premier bloc, pour assurer la reproductibilite
	if (nCompare == 0)
		nCompare = GetFirstBlockAttribute()->GetName().Compare(
		    otherAttributePair->GetFirstBlockAttribute()->GetName());
	return nCompare;
}

void DTAttributeSelection::SortByBlocks()
{
	oaTreeAttributeSelection.SetCompareFunction(DTTreeAttributeCompareBlocks);
	oaTreeAttributeSelection.Sort();
	require(AreTreeAttributesSortedByBlock(&oaTreeAttributeSelection));
}

int DTAttributeSelectionCompareBlocks(const void* elem1, const void* elem2)
{
	DTAttributeSelection* pair1;
	DTAttributeSelection* pair2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	pair1 = cast(DTAttributeSelection*, *(Object**)elem1);
	pair2 = cast(DTAttributeSelection*, *(Object**)elem2);

	// Difference
	nCompare = pair1->CompareBlocks(pair2);
	return nCompare;
}

int DTTreeAttributeCompareName(const void* elem1, const void* elem2)
{
	DTTreeAttribute* attribute1;
	DTTreeAttribute* attribute2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux attributs
	attribute1 = cast(DTTreeAttribute*, *(Object**)elem1);
	attribute2 = cast(DTTreeAttribute*, *(Object**)elem2);

	// nCompare = DTTreeAttributeCompareBlocks(elem1, elem2);
	//  Difference
	// if (nCompare != 0)
	//	return nCompare;
	nDiff = attribute1->GetName().Compare(attribute2->GetName());
	return nDiff;
}

const ALString& DTAttributeSelection::GetDrawingType() const
{
	return sDrawingType;
}

void DTAttributeSelection::SetDrawingType(const ALString& s)
{
	sDrawingType = s;
}

int DTAttributeSelection::GetUsableAttributesNumber()
{
	return nUsableAttributesNumber;
}
void DTAttributeSelection::SetUsableAttributesNumber(int nmax)
{
	nUsableAttributesNumber = nmax;
}

void DTAttributeSelection::WriteReport(ostream& ost)
{

	int nAttribute;
	DTTreeAttribute* taAttribute;

	ost << "Taille de la selection : \t" << oaTreeAttributeSelection.GetSize() << endl;
	for (nAttribute = 0; nAttribute < oaTreeAttributeSelection.GetSize(); nAttribute++)
	{
		taAttribute = cast(DTTreeAttribute*, oaTreeAttributeSelection.GetAt(nAttribute));
		if (taAttribute->aAttribute->GetAttributeBlock() == NULL)
			ost << "attribut : " << taAttribute->GetName() << endl;
		else
			ost << "  - block : " << taAttribute->aAttribute->GetAttributeBlock()->GetName()
			    << "attribut : " << taAttribute->GetName() << endl;
	}
}

boolean DTAttributeSelection::AreTreeAttributesSortedByBlock(const ObjectArray* oaAttributes) const
{
	boolean bOk = true;
	int i;
	DTTreeAttribute* treeatt1;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWAttributeBlock* currentAttributeBlock;
	ObjectDictionary odAttributeBlocks;

	require(oaAttributes != NULL);

	// Parcours des attributs pour verifier l'utilisation des bloc de facon contigues
	currentAttributeBlock = NULL;

	for (i = 0; i < oaAttributes->GetSize(); i++)
	{
		treeatt1 = cast(DTTreeAttribute*, oaAttributes->GetAt(i));
		attribute = treeatt1->aAttribute;
		attributeBlock = attribute->GetAttributeBlock();

		// Test si changement de block
		if (attributeBlock != currentAttributeBlock)
		{
			// Si on est dans un bloc
			if (attributeBlock != NULL)
			{
				// Il ne doit jamais avoir ete vu
				if (odAttributeBlocks.Lookup(attributeBlock->GetName()) != NULL)
				{
					Global::AddError("", "",
							 "AAA Variable " + attribute->GetName() +
							     " is not contiguous in block " +
							     attributeBlock->GetName());
					bOk = false;
					break;
				}
				// Si ok, on memorise le block
				else
					odAttributeBlocks.SetAt(attributeBlock->GetName(), attributeBlock);
			}

			// Memorisation du nouveau block courant
			currentAttributeBlock = attributeBlock;
		}
	}
	return bOk;
}

////////////////////////////////////////////////////////////////////
// Classe DTTreeAttribute

void DTTreeAttribute::CopyFrom(const DTTreeAttribute* source)
{
	require(source != NULL);

	aAttribute = source->aAttribute;
	dLevel = source->dLevel;
	nRank = source->nRank;
}

DTTreeAttribute* DTTreeAttribute::Clone() const
{
	DTTreeAttribute* copyModalityCount;

	copyModalityCount = new DTTreeAttribute;
	copyModalityCount->CopyFrom(this);

	return copyModalityCount;
}

DTTreeAttribute::DTTreeAttribute()
{
	aAttribute = NULL;
	dLevel = 0;
	nRank = 0;
}

const ALString& DTTreeAttribute::GetName() const
{
	require(aAttribute != NULL);

	return aAttribute->GetName();
}

int DTTreeAttributeLevelCampare(const void* elem1, const void* elem2)
{
	DTTreeAttribute* i1 = (DTTreeAttribute*)*(Object**)elem1;
	DTTreeAttribute* i2 = (DTTreeAttribute*)*(Object**)elem2;

	if (i1->dLevel < i2->dLevel)
		return 1;
	else if (i1->dLevel > i2->dLevel)
		return -1;
	else
		return 0;
}

int DTTreeAttributeRankCampare(const void* elem1, const void* elem2)
{
	DTTreeAttribute* i1 = (DTTreeAttribute*)*(Object**)elem1;
	DTTreeAttribute* i2 = (DTTreeAttribute*)*(Object**)elem2;

	if (i1->nRank < i2->nRank)
		return 1;
	else if (i1->nRank > i2->nRank)
		return -1;
	else
		return 0;
}

int DTTreeAttributeCompareBlocks(const void* elem1, const void* elem2)
{
	DTTreeAttribute* treeatt1;
	DTTreeAttribute* treeatt2;
	const KWAttributeBlock* attributeBlock1;
	const KWAttributeBlock* attributeBlock2;
	int nCompare = 0;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	treeatt1 = cast(DTTreeAttribute*, *(Object**)elem1);
	treeatt2 = cast(DTTreeAttribute*, *(Object**)elem2);
	attributeBlock1 = treeatt1->aAttribute->GetAttributeBlock();
	attributeBlock2 = treeatt2->aAttribute->GetAttributeBlock();

	// Comparaison des bloc
	if (attributeBlock1 == attributeBlock2)
		nCompare = 0;
	else if (attributeBlock1 == NULL)
		nCompare = 1;
	else if (attributeBlock2 == NULL)
		nCompare = -1;
	else
	{
		nCompare = -(attributeBlock1->GetAttributeNumber() - attributeBlock2->GetAttributeNumber());
		if (nCompare == 0)
			nCompare = attributeBlock1->GetName().Compare(attributeBlock2->GetName());
	}
	return nCompare;
}
