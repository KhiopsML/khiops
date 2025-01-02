// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTForestAttributeSelection.h"

int DTTreeAttributeLevelCompare(const void* elem1, const void* elem2);
int DTTreeAttributeRankCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////
// Classe DTForestAttributeSelection

DTForestAttributeSelection::DTForestAttributeSelection()
{
	sDrawingType = DTGlobalTag::UNIFORM_SAMPLING_WITH_REPLACEMENT_LABEL;
	nMaxSelectionNumber = 0;
	nOriginalAttributesNumber = 0;
}

DTForestAttributeSelection::~DTForestAttributeSelection()
{
	CleanAll();
}

void DTForestAttributeSelection::CleanAll()
{
	svAttributeNames.Initialize();
	odOriginalAttributesUsed.RemoveAll();
	oaOriginalAttributesUsed.DeleteAll();
	Clean();
}

void DTForestAttributeSelection::Clean()
{
	ivSelectionAttributeNumber.Initialize();
	ivSeedselection.Initialize();
	oaSelectionAttributes.DeleteAll();

	// selections generees
	ivSelectionAttributeNumberInf.Initialize();
	ivSelectionAttributeNumberNull.Initialize();
}

int DTForestAttributeSelection::GetUsableAttributesNumber(int npos)
{
	return ivSelectionAttributeNumber.GetAt(npos);
}

void DTForestAttributeSelection::Initialization(const ObjectDictionary* odInputAttributeStats)
{
	require(odInputAttributeStats != NULL);

	longint nNbInstence = 0;
	DTTreeAttribute* taAttribute;
	KWAttributeStats* attributeStats;
	KWAttribute* attribute;
	int nAttribute;
	ObjectArray oaAttributeStats;

	CleanAll();

	odInputAttributeStats->ExportObjectArray(&oaAttributeStats);
	nOriginalAttributesNumber = oaAttributeStats.GetSize();

	// nombre d'objet dasn la base
	attributeStats = cast(KWAttributeStats*, oaAttributeStats.GetAt(0));
	nNbInstence = attributeStats->GetInstanceNumber();
	//	cout << "les attibut : " << nOriginalAttributesNumber << endl;
	for (nAttribute = 0; nAttribute < nOriginalAttributesNumber; nAttribute++)
	{
		attributeStats = cast(KWAttributeStats*, oaAttributeStats.GetAt(nAttribute));

		attribute = attributeStats->GetClass()->LookupAttribute(attributeStats->GetAttributeName());

		if (attribute->GetUsed() and attribute->GetLoaded())
		{
			taAttribute = new DTTreeAttribute;
			svAttributeNames.Add(attributeStats->GetAttributeName());
			taAttribute->aAttribute = attribute;
			taAttribute->sAttributeName = attributeStats->GetAttributeName();
			taAttribute->dLevel = attributeStats->GetLevel(); //  + 1.0 / nNbInstence);
			oaOriginalAttributesUsed.Add(
			    taAttribute); //			cout << "les attibut " << attribute->GetName() << " : "
					  //<< vLevels.GetAt(nAttribute) << endl;
			odOriginalAttributesUsed.SetAt(attribute->GetName(), taAttribute);
		}
	}
	oaOriginalAttributesUsed.SetCompareFunction(DTTreeAttributeLevelCompare);
	oaOriginalAttributesUsed.Sort();

	for (nAttribute = 0; nAttribute < oaOriginalAttributesUsed.GetSize(); nAttribute++)
	{
		taAttribute = cast(DTTreeAttribute*, oaOriginalAttributesUsed.GetAt(nAttribute));
		taAttribute->nRank = nAttribute + 1;
	}
}

void DTForestAttributeSelection::BuildForestUniformSelections(int nmaxselectionnumber, const ALString& sSelectionType,
							      double dPct)
{
	{
		require(nmaxselectionnumber > 0);
		nMaxSelectionNumber = nmaxselectionnumber;
		int nSelection, nAttribute;
		int nTinf, nTnull;
		int nKinf, nKnull;
		int nAttributesNumber;
		int nSeed = 1968;
		ObjectArray oaVariablesNull;
		ObjectArray oaVariablesInf;
		ObjectArray oaVariablesInfRand;
		DTTreeAttribute* taAttribute;

		DTAttributeSelection* svRandattibuteselection;
		int nOrigineAttributesNumber = oaOriginalAttributesUsed.GetSize();
		int nusableAttributesNumber;

		Clean();

		nKinf = 0;
		nKnull = 0;
		// calcul des ensemble de variable inf et null
		for (nAttribute = 0; nAttribute < nOrigineAttributesNumber; nAttribute++)
		{
			taAttribute = cast(DTTreeAttribute*, oaOriginalAttributesUsed.GetAt(nAttribute));
			if (taAttribute->dLevel == 0.0)
			{
				nKnull++;
				oaVariablesNull.Add(taAttribute);
			}
			else
			{
				nKinf++;
				oaVariablesInf.Add(taAttribute);
			}
		}

		for (nSelection = 0; nSelection < nmaxselectionnumber; nSelection++)
		{
			// Si necessaire (ou demande via IHM), les attributs non informatifs ont deja ete decharges dans
			// le dictionnaire de lecture de la base (i.e : flaggues avec SetUsed(false) et SetLoaded(false)
			// ) D'autre part, on  a deja verifie que les attributs restants peuvent etre traites en memoire
			// (et le cas echeant, on a deja decharge les attributs les moins informatifs)

			if (dPct == 0.0)
				nAttributesNumber =
				    (int)(sqrtl(nOrigineAttributesNumber) + log2l(nOrigineAttributesNumber)); // MB
			else
				nAttributesNumber = (int)(nOrigineAttributesNumber * dPct); // MB

			if (nAttributesNumber > nOrigineAttributesNumber && nOrigineAttributesNumber > 2)
				nAttributesNumber = nOrigineAttributesNumber;

			if (nOrigineAttributesNumber <= 2)
				nAttributesNumber =
				    nOrigineAttributesNumber; // on met toujours au moins un attribut dans l'arbre

			if (nAttributesNumber <= 0)
				nAttributesNumber = 1; // on met toujours au moins un attribut dans l'arbre
			nusableAttributesNumber = nAttributesNumber;
			if (sSelectionType == DTGlobalTag::NODE_UNIFORM_SAMPLING_WITH_REPLACEMENT_LABEL ||
			    sSelectionType == DTGlobalTag::NODE_LEVEL_SAMPLING_WITH_REPLACEMENT_LABEL)
				nAttributesNumber = nOrigineAttributesNumber;

			ivSelectionAttributeNumberInf.Add(0);
			ivSelectionAttributeNumberNull.Add(0);

			ivSelectionAttributeNumber.Add(nAttributesNumber);
			if (sSelectionType == DTGlobalTag::LEVEL_SAMPLING_WITH_REPLACEMENT_LABEL)
			{
				oaOriginalAttributesUsed.SetCompareFunction(DTTreeAttributeLevelCompare);
				oaOriginalAttributesUsed.Sort();
			}
			else
				oaOriginalAttributesUsed.Shuffle();

			// cretation nouvelle liste de variables
			svRandattibuteselection = new DTAttributeSelection;

			svRandattibuteselection->SetSeed(nSeed);
			nSeed = nSeed + 5555;

			nTinf = 0;
			nTnull = 0;
			for (nAttribute = 0; nAttribute < nAttributesNumber; nAttribute++)
			{
				taAttribute = cast(DTTreeAttribute*, oaOriginalAttributesUsed.GetAt(nAttribute));
				svRandattibuteselection->AddAttribute(taAttribute);
				if (taAttribute->dLevel == 0)
					nTnull++;
				else
					nTinf++;
			}
			svRandattibuteselection->SetUsableAttributesNumber(nusableAttributesNumber);
			oaSelectionAttributes.Add(svRandattibuteselection);
			ivSelectionAttributeNumberInf.Add(nTinf);
			ivSelectionAttributeNumberNull.Add(nTnull);
		}
	}
}

void DTForestAttributeSelection::BuildForestSelections(int nmaxselectionnumber, int nvariableNumberMin)
{
	{
		require(nmaxselectionnumber > 0);
		nMaxSelectionNumber = nmaxselectionnumber;
		int nSelection, nAttribute;
		int nrand, nT, nTmax;
		int nTinf, nTnull;
		int nKinf, nKnull;
		int nSeed = 1968;
		ObjectArray oaVariablesNull;
		ObjectArray oaVariablesInf;
		ObjectArray oaVariablesInfRand;
		double dPNull, dPInf, dct;
		DTTreeAttribute* taAttribute;
		double nRmax;
		DTAttributeSelection* svRandattibuteselection;
		require(oaOriginalAttributesUsed.GetSize() == odOriginalAttributesUsed.GetCount());

		Clean();

		nKinf = 0;
		nKnull = 0;
		// calcul des ensemble de variable inf et null
		for (nAttribute = 0; nAttribute < oaOriginalAttributesUsed.GetSize(); nAttribute++)
		{
			taAttribute = cast(DTTreeAttribute*, oaOriginalAttributesUsed.GetAt(nAttribute));
			if (taAttribute->dLevel == 0.0)
			{
				nKnull++;
				oaVariablesNull.Add(taAttribute);
			}
			else
			{
				nKinf++;
				oaVariablesInf.Add(taAttribute);
			}
		}

		if (oaVariablesInf.GetSize() == 0)
			return;

		require(oaOriginalAttributesUsed.GetSize() == (oaVariablesInf.GetSize() + oaVariablesNull.GetSize()));

		for (nSelection = 0; nSelection < nmaxselectionnumber; nSelection++)
		{
			// Nombre max de variables d un tirage Tmax: tire aleatoirement de facon uniforme dans [Tmax_lb,
			// Tmax_ub]
			nTmax = MIN_VARIABLE_2_BUILTREE_BORNE +
				RandomInt(MAX_VARIABLE_2_BUILTREE_BORNE - MIN_VARIABLE_2_BUILTREE_BORNE);

			// Probabilite de tirer une variable de level null Pnull: tire aléatoirement de façon uniforme
			// dans [Pnull_lb, Pnull_ub]
			dPNull = MIN_DRAWING_NULL_VARIABLE_PROBABILITY +
				 RandomDouble() *
				     (MAX_DRAWING_NULL_VARIABLE_PROBABILITY - MIN_DRAWING_NULL_VARIABLE_PROBABILITY);

			// Probabilité de tirer une variable informative Pinf = 1-Pnul
			dPInf = 1 - dPNull;

			// Nombre de variables à tirer nT = min(Tmax, sqrt(K log2(K+ct))
			// dct est une constante dependant MIN_VARIABLE_2_BUILTREE telque MIN_VARIABLE_2_BUILTREE =
			// sqrt(MIN_VARIABLE_2_BUILTREE log2(MIN_VARIABLE_2_BUILTREE+ct)
			dct = pow(2.0, nvariableNumberMin) - nvariableNumberMin;
			nrand = (int)sqrt(1.0 * nOriginalAttributesNumber * log2(nOriginalAttributesNumber + dct));
			nT = (nTmax < nrand) ? nTmax : nrand;

			if (nOriginalAttributesNumber <= nvariableNumberMin)
				nT = nOriginalAttributesNumber;

			// Nombre de variables informatives a tirer nTinf = min(nKinf, nT * dPinf)
			nrand = (int)(nT * dPInf);
			nrand = (nrand < 1) ? 1 : nrand;
			nTinf = (nKinf < nrand) ? nKinf : nrand;

			// Nombre de variables de level null a tirer nTnul = min(nKnull, nT-dTinf)
			nTnull = (nKnull < nT - nTinf) ? nKnull : nT - nTinf;

			// Rang max parmi les variable informatives, pour biaiser le tirage par le rang
			// Borne inf:
			// nRmax_lb = nTinf;
			// Borne sup:
			// nRmax_ub = nKinf;
			// Rang max choisi: Rmax, tiré aléatoirement de façon uniforme dans [Rmax_lb, Rmax_ub]
			nRmax = nTinf + RandomInt(nKinf - nTinf);

			ivSelectionAttributeNumberInf.Add(nTinf);
			ivSelectionAttributeNumberNull.Add(nTnull);
			nT = nTinf + nTnull;
			ivSelectionAttributeNumber.Add(nT);

			// cretation nouvelle liste de variables
			svRandattibuteselection = new DTAttributeSelection;

			svRandattibuteselection->SetSeed(nSeed);
			nSeed = nSeed + 5555;

			oaVariablesInfRand.RemoveAll();

			for (nAttribute = 0; nAttribute < nRmax; nAttribute++)
			{
				oaVariablesInfRand.Add(oaVariablesInf.GetAt(nAttribute));
			}
			oaVariablesInfRand.Shuffle();

			for (nAttribute = 0; nAttribute < nTinf; nAttribute++)
			{
				taAttribute = cast(DTTreeAttribute*, oaVariablesInfRand.GetAt(nAttribute));
				svRandattibuteselection->AddAttribute(taAttribute);
			}

			oaVariablesNull.Shuffle();

			for (nAttribute = 0; nAttribute < nTnull; nAttribute++)
			{
				taAttribute = cast(DTTreeAttribute*, oaVariablesNull.GetAt(nAttribute));
				svRandattibuteselection->AddAttribute(taAttribute);
			}

			svRandattibuteselection->SetUsableAttributesNumber(nT);
			svRandattibuteselection->SetIndex(nSelection);
			svRandattibuteselection->SortByBlocks();
			oaSelectionAttributes.Add(svRandattibuteselection);
			ensure(svRandattibuteselection->Check());
		}
	}
}

void DTForestAttributeSelection::WriteReport(ostream& ost)
{
	int nSelection, nAttribute;
	ObjectArray oaVariablesNull;
	ObjectArray oaVariablesInf;
	ObjectArray oaVariablesInfRand;
	DTTreeAttribute* taAttribute;
	DTAttributeSelection* svRandattibuteselection;

	for (nAttribute = 0; nAttribute < oaOriginalAttributesUsed.GetSize(); nAttribute++)
	{
		taAttribute = cast(DTTreeAttribute*, oaOriginalAttributesUsed.GetAt(nAttribute));
		ost << taAttribute->GetName() << endl;
	}
	ost << "nombre de selection : " << ivSelectionAttributeNumber.GetSize() << endl;

	for (nSelection = 0; nSelection < ivSelectionAttributeNumber.GetSize(); nSelection++)
	{
		ost << "selection : " << nSelection
		    << " nombre d attribut : " << ivSelectionAttributeNumber.GetAt(nSelection) << endl;
		ost << "selection Inf: " << nSelection
		    << " nombre d attribut Inf: " << ivSelectionAttributeNumberInf.GetAt(nSelection) << endl;
		ost << "selection Null: " << nSelection
		    << " nombre d attribut Null: " << ivSelectionAttributeNumberNull.GetAt(nSelection) << endl;
		svRandattibuteselection = cast(DTAttributeSelection*, oaSelectionAttributes.GetAt(nSelection));
		for (nAttribute = 0; nAttribute < svRandattibuteselection->GetSize(); nAttribute++)
		{
			ost << (char*)svRandattibuteselection->GetAttributeAt(nAttribute)->GetName() << endl;
		}
	}
}

StringVector* DTForestAttributeSelection::GetAllSelectedAttributes()
{
	int nSelection, nAttribute;
	POSITION position;
	ALString sname;
	Object* oElement;
	StringVector* svallselectedattibutes = NULL;
	ObjectDictionary odlistattibutes;
	DTTreeAttribute* taAttribute;
	DTAttributeSelection* asAttibuteSelection;

	if (oaSelectionAttributes.GetSize() == 0)
		return NULL;

	svallselectedattibutes = new StringVector;

	for (nSelection = 0; nSelection < oaSelectionAttributes.GetSize(); nSelection++)
	{
		asAttibuteSelection = cast(DTAttributeSelection*, oaSelectionAttributes.GetAt(nSelection));
		for (nAttribute = 0; nAttribute < asAttibuteSelection->GetSize(); nAttribute++)
		{
			taAttribute = cast(DTTreeAttribute*, asAttibuteSelection->GetAttributeAt(nAttribute));
			odlistattibutes.SetAt(taAttribute->GetName(), taAttribute);
		}
	}
	position = odlistattibutes.GetStartPosition();
	while (position != NULL)
	{
		odlistattibutes.GetNextAssoc(position, sname, oElement);
		taAttribute = cast(DTTreeAttribute*, oElement);
		svallselectedattibutes->Add(taAttribute->GetName());
	}
	return svallselectedattibutes;
}

int DTForestAttributeSelection::GetSelectionNumber()
{
	return ivSelectionAttributeNumber.GetSize();
}

StringVector* DTForestAttributeSelection::GetSelectedAttributesAt(int npos)
{
	require(npos >= 0 && npos < oaSelectionAttributes.GetSize());
	return cast(StringVector*, oaSelectionAttributes.GetAt(npos));
}

int DTForestAttributeSelection::GetMaxAttributesNumber()
{
	int nSelection;
	int nMax;
	ALString sname;
	ObjectDictionary odlistattibutes;
	DTAttributeSelection* svattibuteselection;

	if (ivSelectionAttributeNumber.GetSize() == 0)
		return 0;

	nMax = 0;

	for (nSelection = 0; nSelection < ivSelectionAttributeNumber.GetSize(); nSelection++)
	{
		svattibuteselection = cast(DTAttributeSelection*, oaSelectionAttributes.GetAt(nSelection));
		if (svattibuteselection->GetSize() > nMax)
			nMax = svattibuteselection->GetSize();
	}
	return nMax;
}

ObjectArray* DTForestAttributeSelection::GetAttributeSelections()
{
	return &oaSelectionAttributes;
}

const ALString& DTForestAttributeSelection::GetDrawingType() const
{
	return sDrawingType;
}

void DTForestAttributeSelection::SetDrawingType(const ALString& s)
{
	sDrawingType = s;
}

// parcourt une liste d'attributs, effectue un tirage aleatoire en fonction du level de ces attributs, et
// renvoie (au maximum) les 'maxAttributesNumber' index de chargements des attributs de plus fort level
ObjectArray* DTForestAttributeSelection::GetAttributesFromLevels(const int maxAttributesNumber, DoubleVector& vLevels,
								 ObjectArray& oaListAttributes)
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

	if (maxAttributesNumber == 0 or vLevels.GetSize() == 0 or oaListAttributes.GetSize() == 0)
		return NULL;

	assert(vLevels.GetSize() == oaListAttributes.GetSize());

	ObjectArray* result = new ObjectArray;

	int i;
	int nIndexesNumber;
	double dRandom;
	double dKey;
	KWSortableValue* svKey;
	SortedList slKeys(KWSortableValueCompare);
	IntVector ivSortedListIndexes;

	ivSortedListIndexes.SetSize(0);

	// Parcours de toutes les variables
	for (i = 0; i < vLevels.GetSize(); i++)
	{
		// Tirage uniforme sur [0,1]
		dRandom = RandomDouble();

		// Calcul de la cle
		dKey = pow(dRandom, 1 / vLevels.GetAt(i));

		// Insertion dans la liste triee par cle
		svKey = new KWSortableValue;
		svKey->SetIndex(i);
		svKey->SetSortValue(dKey);
		slKeys.Add(svKey);

		// Elagage : on ne garde qu'un reservoir de taille nSelectedAttributesNumber
		if (slKeys.GetCount() > maxAttributesNumber and slKeys.GetCount() > 0)
		{
			svKey = cast(KWSortableValue*, slKeys.GetHead());
			slKeys.RemoveHead();
			delete svKey;
		}
	}

	// Memorisation des nSelectedAttributesNumber meilleures cles
	for (nIndexesNumber = 0; nIndexesNumber < maxAttributesNumber; nIndexesNumber++)
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
		result->Add(oaListAttributes.GetAt(ivSortedListIndexes.GetAt(i)));

	return result;
}

////////////////////////////////////////////////////////////////////
// Classe DTAttributeSelectionSlices

DTAttributeSelectionsSlices::DTAttributeSelectionsSlices()
{
	oaTreeAttributes.SetCompareFunction(DTTreeAttributeCompareName);
}

DTAttributeSelectionsSlices::~DTAttributeSelectionsSlices() {}

ObjectArray* DTAttributeSelectionsSlices::GetAttributeSelections()
{
	return &oaAttributeSelections;
}

ObjectArray* DTAttributeSelectionsSlices::GetTreeAttributes()
{
	return &oaTreeAttributes;
}

NumericKeyDictionary* DTAttributeSelectionsSlices::GetSlices()
{
	return &nkdSlices;
}

void DTAttributeSelectionsSlices::AddAttributeSelectionsSlices(
    const DTAttributeSelectionsSlices* otherAttributePairsSlices)
{
	int i;
	ObjectArray oaTmpFirstArray;

	require(otherAttributePairsSlices != NULL);
	require(Check());
	require(otherAttributePairsSlices->Check());
	require(this != otherAttributePairsSlices);

	// Ajout des selections d'attributs
	for (i = 0; i < otherAttributePairsSlices->oaAttributeSelections.GetSize(); i++)
		oaAttributeSelections.Add(otherAttributePairsSlices->oaAttributeSelections.GetAt(i));

	// Ajout des attributs en respectant le tri
	oaTmpFirstArray.CopyFrom(&oaTreeAttributes);
	oaTreeAttributes.SetSize(0);
	MergeArrayContent(&oaTmpFirstArray, &otherAttributePairsSlices->oaTreeAttributes, DTTreeAttributeCompareName,
			  &oaTreeAttributes);

	// Ajout des slices en respectant le tri
	AddNumericDictionaryContent(&nkdSlices, &otherAttributePairsSlices->nkdSlices);

	ensure(Check());
}

void DTAttributeSelectionsSlices::AddAttributeSelection(const DTAttributeSelection* otherAttributeselection,
							const ObjectDictionary* odSliceAttributes)
{
	int nattribute;
	ObjectArray oaTmpFirstArray;
	ObjectArray oaTmpSecondArray;
	NumericKeyDictionary ndSlices;
	KWDataTableSlice* slice1;
	DTTreeAttribute* dtattribute1;
	require(otherAttributeselection != NULL);
	// require(Check());
	// require(otherAttributeselection->Check());

	oaAttributeSelections.Add((Object*)otherAttributeselection);

	// Ajout des attributs en respectant le tri
	oaTmpFirstArray.CopyFrom(&oaTreeAttributes);
	oaTmpFirstArray.SetCompareFunction(DTTreeAttributeCompareName);
	oaTmpFirstArray.Sort();
	// oaTreeAttributes.SetSize(0);
	oaTreeAttributes.RemoveAll();
	oaTreeAttributes.SetCompareFunction(DTTreeAttributeCompareName);

	// oaTmpSecondArray = (ObjectArray*)otherAttributeselection->GetTreeAttributeSelection();
	oaTmpSecondArray.CopyFrom(otherAttributeselection->GetTreeAttributeSelection());
	oaTmpSecondArray.SetCompareFunction(DTTreeAttributeCompareName);
	oaTmpSecondArray.Sort();

	MergeArrayContent(&oaTmpFirstArray, &oaTmpSecondArray, DTTreeAttributeCompareName, &oaTreeAttributes);

	for (nattribute = 0; nattribute < otherAttributeselection->GetSize(); nattribute++)
	{
		dtattribute1 = cast(DTTreeAttribute*, otherAttributeselection->GetAttributeAt(nattribute));
		slice1 = cast(KWDataTableSlice*, odSliceAttributes->Lookup(dtattribute1->GetName()));
		nkdSlices.SetAt(slice1, slice1);
	}

	oaTreeAttributes.Sort();

	ensure(Check());
}

void DTAttributeSelectionsSlices::AddNumericDictionaryContent(NumericKeyDictionary* oaFirst,
							      const NumericKeyDictionary* oaSecond)
{
	POSITION position;
	NUMERIC key;
	Object* oElement;

	position = oaSecond->GetStartPosition();
	while (position != NULL)
	{
		oaSecond->GetNextAssoc(position, key, oElement);
		oaFirst->SetAt(key, oElement);
	}
}

int DTAttributeSelectionsSlices::UnionAttributesCount(const DTAttributeSelection* otherAttributeselection)
{
	ObjectArray oaTmpFirstArray;
	ObjectArray oaTmpCmp;
	require(otherAttributeselection != NULL);
	oaTmpFirstArray.CopyFrom(&oaTreeAttributes);
	oaTmpCmp.SetCompareFunction(DTTreeAttributeCompareName);
	MergeArrayContent(&oaTmpFirstArray, otherAttributeselection->GetTreeAttributeSelection(),
			  DTTreeAttributeCompareName, &oaTmpCmp);

	return oaTmpCmp.GetSize();
}

int DTAttributeSelectionsSlices::CompareSlices(const DTAttributeSelectionsSlices* otherAttributePairsSlices) const
{
	int nSliceIdentique;
	int nCompare;

	require(otherAttributePairsSlices != NULL);

	POSITION position;
	NUMERIC key;
	Object* oElement;
	nSliceIdentique = 0;
	position = otherAttributePairsSlices->nkdSlices.GetStartPosition();
	while (position != NULL)
	{
		otherAttributePairsSlices->nkdSlices.GetNextAssoc(position, key, oElement);
		if (nkdSlices.Lookup(key) != NULL)
			nSliceIdentique++;
	}

	// Comparaison des tranches
	nCompare = nkdSlices.GetCount() + otherAttributePairsSlices->nkdSlices.GetCount() - 2 * nSliceIdentique;

	return nCompare;
}

DoubleVector* DTAttributeSelectionsSlices::GetLexicographicSortCriterion()
{
	return &dvLexicographicSortCriterion;
}

int DTAttributeSelectionsSlices::CompareLexicographicSortCriterion(
    const DTAttributeSelectionsSlices* otherAttributePairsSlices) const
{
	int nCompare;
	int nMaxSize;
	int i;

	require(otherAttributePairsSlices != NULL);

	// Comparaison des vecteur d'index lexicographique
	nCompare = 0;
	nMaxSize = max(dvLexicographicSortCriterion.GetSize(),
		       otherAttributePairsSlices->dvLexicographicSortCriterion.GetSize());
	for (i = 0; i < nMaxSize; i++)
	{
		if (i >= dvLexicographicSortCriterion.GetSize())
			nCompare = -1;
		else if (i >= otherAttributePairsSlices->dvLexicographicSortCriterion.GetSize())
			nCompare = 1;
		else
			nCompare = CompareDouble(dvLexicographicSortCriterion.GetAt(i),
						 otherAttributePairsSlices->dvLexicographicSortCriterion.GetAt(i));
		if (nCompare != 0)
			break;
	}
	return nCompare;
}

void DTAttributeSelectionsSlices::DisplayLexicographicSortCriterionHeaderLineReport(
    ostream& ost, const ALString& sSortCriterion) const
{
	ost << GetClassLabel() << "\t" << sSortCriterion << "\n";
}

void DTAttributeSelectionsSlices::DisplayLexicographicSortCriterionLineReport(ostream& ost) const
{
	int i;

	ost << GetObjectLabel() << "\t";
	for (i = 0; i < dvLexicographicSortCriterion.GetSize(); i++)
	{
		ost << "\t";
		ost << dvLexicographicSortCriterion.GetAt(i);
	}
	ost << "\n";
}

boolean DTAttributeSelectionsSlices::Check() const
{
	boolean bOk = true;
	ObjectDictionary odAttributesInPairs;
	ObjectDictionary odAttributes;
	ObjectDictionary odAttributeSlices;
	ObjectDictionary odSlices;
	ObjectDictionary odSlicePerAttributes;
	int nSlice;
	int i;
	KWAttribute* previousAttribute;
	KWDataTableSlice* previousSlice;
	KWAttribute* attribute;
	DTTreeAttribute* treeattribute;
	KWDataTableSlice* slice;
	ALString sTmp;
	ObjectArray oaSlices;
	nkdSlices.ExportObjectArray(&oaSlices);

	// Il doit y avoir au moins une paire

	// Collecte dans un dictionnaire des tranches et de leurs attributs
	if (bOk)
	{
		previousSlice = NULL;
		for (nSlice = 0; nSlice < oaSlices.GetSize(); nSlice++)
		{
			slice = cast(KWDataTableSlice*, oaSlices.GetAt(nSlice));

			// Erreur si probleme d'ordre
			// if (previousSlice != NULL and previousSlice->CompareLexicographicOrder(slice) >= 0)
			//{
			//	AddError("Wrong ordering between " + slice->GetClassLabel() + "s " +
			//		 previousSlice->GetObjectLabel() + " and " + slice->GetObjectLabel());
			//	bOk = false;
			//	break;
			//}
			previousSlice = slice;

			// Erreur si tranche deja utilisee
			if (odSlices.Lookup(slice->GetClass()->GetName()) != NULL)
			{
				bOk = false;
				AddError(slice->GetClassLabel() + " " + slice->GetObjectLabel() +
					 " used several times");
				break;
			}
			// Ajout de la tranche sinon
			else
			{
				// Ajout de la tranche
				odSlices.SetAt(slice->GetClass()->GetName(), slice);

				// Collecte des attributs de la tranche
				for (i = 0; i < slice->GetClass()->GetUsedAttributeNumber(); i++)
				{
					attribute = slice->GetClass()->GetUsedAttributeAt(i);
					assert(odSlicePerAttributes.Lookup(attribute->GetName()) == NULL);
					odSlicePerAttributes.SetAt(attribute->GetName(), slice);
				}
			}
		}
	}

	// Collecte dans un dictionnaire des attributs et verification et de leur tranche
	if (bOk)
	{
		previousAttribute = NULL;
		for (i = 0; i < oaTreeAttributes.GetSize(); i++)
		{
			treeattribute = cast(DTTreeAttribute*, oaTreeAttributes.GetAt(i));
			attribute = treeattribute->aAttribute;

			// Erreur si probleme d'ordre
			if (previousAttribute != NULL and previousAttribute->GetName() > attribute->GetName())
			{
				AddError("Wrong ordering between " + attribute->GetClassLabel() + "s " +
					 previousAttribute->GetName() + " and " + attribute->GetName());
				bOk = false;
				break;
			}
			previousAttribute = attribute;

			// Erreur si attribut deja utilisee
			if (odAttributes.Lookup(attribute->GetName()) != NULL)
			{
				bOk = false;
				AddError(attribute->GetClassLabel() + " " + attribute->GetObjectLabel() +
					 " used several times");
				break;
			}
			// Ajout de l'attribut sinon
			else
			{
				// Ajout de l'attribut
				odAttributes.SetAt(attribute->GetName(), attribute);

				// Erreur si aucune tranche ne le contient
				slice = cast(KWDataTableSlice*, odSlicePerAttributes.Lookup(attribute->GetName()));
				if (slice == NULL)
				{
					bOk = false;
					AddError("No data set slice found for " + attribute->GetClassLabel() + " " +
						 attribute->GetObjectLabel());
					break;
				}
				// Sinon, on memorise la tranche
				else
					odAttributeSlices.SetAt(slice->GetClass()->GetName(), slice);
			}
		}
	}

	// On verifie qu'il n'y a pas tranche inutile
	if (bOk)
	{
		assert(odAttributeSlices.GetCount() <= oaSlices.GetSize());
		if (odAttributeSlices.GetCount() < oaSlices.GetSize())
		{
			bOk = false;
			AddError(sTmp + "More data set slice than necessary to store the variables (" +
				 IntToString(oaSlices.GetSize()) + " instead of " +
				 IntToString(odAttributeSlices.GetCount()) + ")");
		}
	}

	// Collecte dans un dictionnaire des attributs utilises dans les paires

	// On verifie qu'il n'y a pas d'attribut inutile
	// if (bOk)
	//{
	//	assert(odAttributesInPairs.GetCount() <= odAttributes.GetCount());
	//	if (odAttributesInPairs.GetCount() < odAttributes.GetCount())
	//	{
	//		bOk = false;
	//		AddError(sTmp + "More attributes than necessary to store the variables in the pairs (" +
	//			 IntToString(odAttributes.GetCount()) + " instead of " +
	// IntToString(odAttributesInPairs.GetCount()) + ")");
	//	}
	//}
	return bOk;
}

void DTAttributeSelectionsSlices::Write(ostream& ost) const
{
	int i;
	DTTreeAttribute* treeattribute;
	KWDataTableSlice* slice;
	ObjectArray oaSlices;
	DTAttributeSelection* atselection;

	nkdSlices.ExportObjectArray(&oaSlices);

	ost << GetClassLabel() << " " << GetObjectLabel() << "\n";

	// Affichages des paires d'attributs

	for (i = 0; i < oaAttributeSelections.GetSize(); i++)
	{
		ost << "  Selections : " << i << "\n";
		atselection = cast(DTAttributeSelection*, oaAttributeSelections.GetAt(i));
		atselection->Write(ost);
	}

	// Affichages des attributs
	ost << " union des Variables\n";
	for (i = 0; i < oaTreeAttributes.GetSize(); i++)
	{
		treeattribute = cast(DTTreeAttribute*, oaTreeAttributes.GetAt(i));
		ost << "\t" << treeattribute->GetName() << "\n";
	}

	// Affichages des tranches
	ost << " union slices\n";
	for (i = 0; i < oaSlices.GetSize(); i++)
	{
		slice = cast(KWDataTableSlice*, oaSlices.GetAt(i));
		ost << "\t" << slice->GetClass()->GetName() << "\n";
	}
}

const ALString DTAttributeSelectionsSlices::GetClassLabel() const
{
	return "Tree attributes slices";
}

const ALString DTAttributeSelectionsSlices::GetObjectLabel() const
{
	ALString sLabel;
	ObjectArray oaSlices;
	nkdSlices.ExportObjectArray(&oaSlices);
	sLabel = "(";
	if (oaSlices.GetSize() > 0)
		sLabel += cast(KWDataTableSlice*, oaSlices.GetAt(0))->GetClass()->GetName();
	if (oaSlices.GetSize() > 1)
	{
		sLabel += ";";
		sLabel += cast(KWDataTableSlice*, oaSlices.GetAt(1))->GetClass()->GetName();
	}
	sLabel += ": ";
	sLabel += IntToString(oaTreeAttributes.GetSize());
	sLabel += ", ";
	sLabel += IntToString(oaSlices.GetSize());
	sLabel += ")";

	return sLabel;
}

void DTAttributeSelectionsSlices::MergeArrayContent(const ObjectArray* oaFirst, const ObjectArray* oaSecond,
						    CompareFunction fCompare, ObjectArray* oaMergedResult) const
{
	Object* object1;
	Object* object2;
	int nCompare;
	int i;
	int j;

	require(oaFirst != NULL);
	require(oaSecond != NULL);
	require(fCompare != NULL);
	require(oaMergedResult != NULL);
	require(oaMergedResult->GetCompareFunction() != NULL);
	require(oaMergedResult->GetSize() == 0);

	// Cas ou un des tableau est vide
	if (oaFirst->GetSize() == 0)
		oaMergedResult->CopyFrom(oaSecond);
	else if (oaSecond->GetSize() == 0)
		oaMergedResult->CopyFrom(oaFirst);
	// Cas ou aucun tableau n'est vide
	else
	{
		// Acces au premier objet de chaque tableau
		object1 = oaFirst->GetAt(0);
		i = 1;
		object2 = oaSecond->GetAt(0);
		j = 1;

		// Ajout des objets en respectant le tri
		while (object1 != NULL or object2 != NULL)
		{
			// On recopie la fin du premier tableau si on a fini le second
			if (object2 == NULL)
			{
				oaMergedResult->Add(object1);
				while (i < oaFirst->GetSize())
				{
					oaMergedResult->Add(oaFirst->GetAt(i));
					i++;
				}
				break;
			}
			// On recopie la fin du second tableau si on a fini le premier
			else if (object1 == NULL)
			{
				oaMergedResult->Add(object2);
				while (j < oaSecond->GetSize())
				{
					oaMergedResult->Add(oaSecond->GetAt(j));
					j++;
				}
				break;
			}
			// Sinon, on compare les elements
			else
			{
				nCompare = fCompare(&object1, &object2);

				// On rajoute le plus petit element
				if (nCompare <= 0)
					oaMergedResult->Add(object1);
				else
					oaMergedResult->Add(object2);

				// On passe a l'element suivant pour chaque liste, en tenant compte des cas d'egalite
				if (nCompare <= 0)
				{
					object1 = NULL;
					if (i < oaFirst->GetSize())
					{
						object1 = oaFirst->GetAt(i);
						i++;
					}
				}
				if (nCompare >= 0)
				{
					object2 = NULL;
					if (j < oaSecond->GetSize())
					{
						object2 = oaSecond->GetAt(j);
						j++;
					}
				}
			}
		}
	}
}

int DTAttributeSelectionsSlicesCompareSlices(const void* elem1, const void* elem2)
{
	DTAttributeSelectionsSlices* pairs1;
	DTAttributeSelectionsSlices* pairs2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	pairs1 = cast(DTAttributeSelectionsSlices*, *(Object**)elem1);
	pairs2 = cast(DTAttributeSelectionsSlices*, *(Object**)elem2);

	// Difference
	nCompare = pairs1->CompareSlices(pairs2);
	return nCompare;
}

int DTAttributeSelectionsSlicesCompareLexicographicSortCriterion(const void* elem1, const void* elem2)
{
	DTAttributeSelectionsSlices* pairs1;
	DTAttributeSelectionsSlices* pairs2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	pairs1 = cast(DTAttributeSelectionsSlices*, *(Object**)elem1);
	pairs2 = cast(DTAttributeSelectionsSlices*, *(Object**)elem2);

	// Difference
	nCompare = pairs1->CompareLexicographicSortCriterion(pairs2);
	if (nCompare == 0)
		nCompare = pairs1->CompareSlices(pairs2);
	return nCompare;
}
