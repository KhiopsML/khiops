// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTupleTable.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Les fonctions de comparaison sont effectuees sur des tuples devant ayant meme structure
// et devant appartenir a la meme table de tuples
// En mode debug, les tuples ont acces a leur table, ce qui permet de connaitre leur structure.
// Il est important d'economiser de la memoire, car il peut y avoir de nombreuses tables
// de tuples simultanement en memoire, dans le cas de donnees sparse. A cet effet, les tuples
// ne memorisent pas leur table de tuple en mode release. Pour parametrer correctement les tris,
// la table de tuples courante est alors utilise en variable statique, pour avoir acces a la structure
// des tuples dans les fonctions de comparaiosn generiques qui en ont besoin.
// Cette table de tuples courante doit etre positionnee dans les methodes de tri de tableau ou
// avant chaque utilisation d'une SortedList, pour avoir acces a la bonne structure courante.

static const KWTupleTable* KWTupleTableSortTupleTable = NULL;

void KWTupleTable::SetSortTupleTable(const KWTupleTable* tupleTable) const
{
	KWTupleTableSortTupleTable = tupleTable;
}

const KWTupleTable* KWTupleTable::GetSortTupleTable() const
{
	return KWTupleTableSortTupleTable;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Fonctions de comparaison sur les valeurs, les valeurs utilisateur ou par effectif decroissant
// L'utilisation de fonctions de comparaison specifiques peut faire gagner de 10% a 20%
// de temps dans les tris
// Ce n'est pas lourd a maintenir: le copier-coller a ete effectue une fois pour toutes

///////////////////////////////////////////////////////////////////////////////////
// Fonctions de comparaison sur les valeurs, generiques et specialisees

int KWTupleCompare(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nAttributeNumber;
	int i;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));

	// Comparaison sur leurs attributs
	nAttributeNumber = KWTupleTableSortTupleTable->GetAttributeNumber();
	for (i = 0; i < nAttributeNumber; i++)
	{
		// Comparaison selon la valeur de l'attribut
		if (KWTupleTableSortTupleTable->GetAttributeTypeAt(i) == KWType::Continuous)
			nCompare = tuple1->CompareContinuousAt(tuple2, i);
		else
			nCompare = tuple1->CompareSymbolAt(tuple2, i);

		// Arret si difference
		if (nCompare != 0)
			return nCompare;
	}

	// Il y a egalite sur tous les attributs
	return 0;
}

int KWTupleCompareC(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareSymbolAt(tuple2, 0);
	return nCompare;
}

int KWTupleCompareN(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareContinuousAt(tuple2, 0);
	return nCompare;
}

int KWTupleCompareCC(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareSymbolAt(tuple2, 0);
	if (nCompare == 0)
		nCompare = tuple1->CompareSymbolAt(tuple2, 1);
	return nCompare;
}

int KWTupleCompareNN(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareContinuousAt(tuple2, 0);
	if (nCompare == 0)
		nCompare = tuple1->CompareContinuousAt(tuple2, 1);
	return nCompare;
}

int KWTupleCompareCN(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareSymbolAt(tuple2, 0);
	if (nCompare == 0)
		nCompare = tuple1->CompareContinuousAt(tuple2, 1);
	return nCompare;
}

int KWTupleCompareNC(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareContinuousAt(tuple2, 0);
	if (nCompare == 0)
		nCompare = tuple1->CompareSymbolAt(tuple2, 1);
	return nCompare;
}

///////////////////////////////////////////////////////////////////////////////////
// Fonctions de comparaison sur les valeurs utilisateur, generiques et specialisees
// Quand il n'y a pas de valeurs categorielles, le tri est le meme sur valeur ou valeur utilisateurs

int KWTupleCompareValues(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nAttributeNumber;
	int i;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));

	// Comparaison sur leurs attributs
	nAttributeNumber = KWTupleTableSortTupleTable->GetAttributeNumber();
	for (i = 0; i < nAttributeNumber; i++)
	{
		// Comparaison selon la valeur de l'attribut
		if (KWTupleTableSortTupleTable->GetAttributeTypeAt(i) == KWType::Continuous)
			nCompare = tuple1->CompareContinuousAt(tuple2, i);
		else
			nCompare = tuple1->CompareSymbolValueAt(tuple2, i);

		// Arret si difference
		if (nCompare != 0)
			return nCompare;
	}

	// Il y a egalite sur tous les attributs
	return 0;
}

int KWTupleCompareValuesC(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareSymbolValueAt(tuple2, 0);
	return nCompare;
}

int KWTupleCompareValuesCC(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareSymbolValueAt(tuple2, 0);
	if (nCompare == 0)
		nCompare = tuple1->CompareSymbolValueAt(tuple2, 1);
	return nCompare;
}

int KWTupleCompareValuesCN(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareSymbolValueAt(tuple2, 0);
	if (nCompare == 0)
		nCompare = tuple1->CompareContinuousAt(tuple2, 1);
	return nCompare;
}

int KWTupleCompareValuesNC(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareContinuousAt(tuple2, 0);
	if (nCompare == 0)
		nCompare = tuple1->CompareSymbolValueAt(tuple2, 1);
	return nCompare;
}

///////////////////////////////////////////////////////////////////////////////////
// Fonctions de comparaison par effectif decroissant, generiques et specialisees

int KWTupleCompareDecreasingFrequencies(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nAttributeNumber;
	int i;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));

	// Comparaison sur les effectifs de facon decroissantes
	nCompare = -tuple1->GetFrequency() + tuple2->GetFrequency();

	// Comparaison sur leurs attributs si necessaire
	if (nCompare != 0)
		return nCompare;
	else
	{
		nAttributeNumber = KWTupleTableSortTupleTable->GetAttributeNumber();
		for (i = 0; i < nAttributeNumber; i++)
		{
			// Comparaison selon la valeur de l'attribut
			if (KWTupleTableSortTupleTable->GetAttributeTypeAt(i) == KWType::Continuous)
				nCompare = tuple1->CompareContinuousAt(tuple2, i);
			else
				nCompare = tuple1->CompareSymbolValueAt(tuple2, i);

			// Arret si difference
			if (nCompare != 0)
				return nCompare;
		}
	}
	// Il y a egalite sur tous les attributs
	return 0;
}

int KWTupleCompareDecreasingFrequenciesC(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = -tuple1->GetFrequency() + tuple2->GetFrequency();
	if (nCompare == 0)
		nCompare = tuple1->CompareSymbolValueAt(tuple2, 0);
	return nCompare;
}

int KWTupleCompareDecreasingFrequenciesN(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = -tuple1->GetFrequency() + tuple2->GetFrequency();
	if (nCompare == 0)
		nCompare = tuple1->CompareContinuousAt(tuple2, 0);
	return nCompare;
}

int KWTupleCompareDecreasingFrequenciesCC(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = -tuple1->GetFrequency() + tuple2->GetFrequency();
	if (nCompare == 0)
	{
		nCompare = tuple1->CompareSymbolValueAt(tuple2, 0);
		if (nCompare == 0)
			nCompare = tuple1->CompareSymbolValueAt(tuple2, 1);
	}
	return nCompare;
}

int KWTupleCompareDecreasingFrequenciesNN(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = -tuple1->GetFrequency() + tuple2->GetFrequency();
	if (nCompare == 0)
	{
		nCompare = tuple1->CompareContinuousAt(tuple2, 0);
		if (nCompare == 0)
			nCompare = tuple1->CompareContinuousAt(tuple2, 1);
	}
	return nCompare;
}

int KWTupleCompareDecreasingFrequenciesCN(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = -tuple1->GetFrequency() + tuple2->GetFrequency();
	if (nCompare == 0)
	{
		nCompare = tuple1->CompareSymbolValueAt(tuple2, 0);
		if (nCompare == 0)
			nCompare = tuple1->CompareContinuousAt(tuple2, 1);
	}
	return nCompare;
}

int KWTupleCompareDecreasingFrequenciesNC(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = -tuple1->GetFrequency() + tuple2->GetFrequency();
	if (nCompare == 0)
	{
		nCompare = tuple1->CompareContinuousAt(tuple2, 0);
		if (nCompare == 0)
			nCompare = tuple1->CompareSymbolValueAt(tuple2, 1);
	}
	return nCompare;
}

///////////////////////////////////////////////////////////////////////////////////
// Methode de KWTupleTable pour extraire les tuples dans un tableu trie selon
// undes des variables

static int nKWTupleTableSortIndex1 = -1;
static int nKWTupleTableSortIndex2 = -1;

int KWTupleCompareCAt(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);
	require(nKWTupleTableSortIndex1 != -1);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareSymbolAt(tuple2, nKWTupleTableSortIndex1);
	return nCompare;
}

int KWTupleCompareNAt(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);
	require(nKWTupleTableSortIndex1 != -1);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareContinuousAt(tuple2, nKWTupleTableSortIndex1);
	return nCompare;
}

int KWTupleCompareCCAt(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);
	require(nKWTupleTableSortIndex1 != -1);
	require(nKWTupleTableSortIndex2 != -1);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareSymbolAt(tuple2, nKWTupleTableSortIndex1);
	if (nCompare == 0)
		nCompare = tuple1->CompareSymbolAt(tuple2, nKWTupleTableSortIndex2);
	return nCompare;
}

int KWTupleCompareNNAt(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);
	require(nKWTupleTableSortIndex1 != -1);
	require(nKWTupleTableSortIndex2 != -1);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareContinuousAt(tuple2, nKWTupleTableSortIndex1);
	if (nCompare == 0)
		nCompare = tuple1->CompareContinuousAt(tuple2, nKWTupleTableSortIndex2);
	return nCompare;
}

int KWTupleCompareCNAt(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);
	require(nKWTupleTableSortIndex1 != -1);
	require(nKWTupleTableSortIndex2 != -1);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareSymbolAt(tuple2, nKWTupleTableSortIndex1);
	if (nCompare == 0)
		nCompare = tuple1->CompareContinuousAt(tuple2, nKWTupleTableSortIndex2);
	return nCompare;
}

int KWTupleCompareNCAt(const void* elem1, const void* elem2)
{
	KWTuple* tuple1;
	KWTuple* tuple2;
	int nCompare;

	require(KWTupleTableSortTupleTable != NULL);
	require(nKWTupleTableSortIndex1 != -1);
	require(nKWTupleTableSortIndex2 != -1);

	// Acces aux tuples
	tuple1 = cast(KWTuple*, *(Object**)elem1);
	tuple2 = cast(KWTuple*, *(Object**)elem2);
	debug(assert(tuple1->GetTupleTable() == KWTupleTableSortTupleTable and
		     tuple2->GetTupleTable() == KWTupleTableSortTupleTable));
	nCompare = tuple1->CompareContinuousAt(tuple2, nKWTupleTableSortIndex1);
	if (nCompare == 0)
		nCompare = tuple1->CompareSymbolAt(tuple2, nKWTupleTableSortIndex2);
	return nCompare;
}

void KWTupleTable::SortTuplesByAttribute(const ALString& sSortAttributeName, ObjectArray* oaTuplesToSort) const
{
	CompareFunction compareFunction;

	require(LookupAttributeIndex(sSortAttributeName) != -1);
	require(oaTuplesToSort != NULL);
	require(nKWTupleTableSortIndex1 == -1);

	// Parametrage de la fonction de tri
	nKWTupleTableSortIndex1 = LookupAttributeIndex(sSortAttributeName);
	if (GetAttributeTypeAt(nKWTupleTableSortIndex1) == KWType::Symbol)
		compareFunction = KWTupleCompareCAt;
	else
		compareFunction = KWTupleCompareNAt;

	// Tri du tableau
	SetSortTupleTable(this);
	oaTuplesToSort->SetCompareFunction(compareFunction);
	oaTuplesToSort->Sort();
	SetSortTupleTable(NULL);

	// Nettoyage
	oaTuplesToSort->SetCompareFunction(NULL);
	nKWTupleTableSortIndex1 = -1;
}

void KWTupleTable::SortTuplesByAttributePair(const ALString& sSortAttributeName1, const ALString& sSortAttributeName2,
					     ObjectArray* oaTuplesToSort) const
{
	CompareFunction compareFunction;

	require(LookupAttributeIndex(sSortAttributeName1) != -1);
	require(LookupAttributeIndex(sSortAttributeName2) != -1);
	require(LookupAttributeIndex(sSortAttributeName1) != LookupAttributeIndex(sSortAttributeName2));
	require(oaTuplesToSort != NULL);
	require(nKWTupleTableSortIndex1 == -1);
	require(nKWTupleTableSortIndex2 == -1);

	// Parametrage de la fonction de tri
	nKWTupleTableSortIndex1 = LookupAttributeIndex(sSortAttributeName1);
	nKWTupleTableSortIndex2 = LookupAttributeIndex(sSortAttributeName2);
	if (GetAttributeTypeAt(nKWTupleTableSortIndex1) == KWType::Symbol)
	{
		if (GetAttributeTypeAt(nKWTupleTableSortIndex2) == KWType::Symbol)
			compareFunction = KWTupleCompareCCAt;
		else
			compareFunction = KWTupleCompareCNAt;
	}
	else
	{
		if (GetAttributeTypeAt(nKWTupleTableSortIndex2) == KWType::Symbol)
			compareFunction = KWTupleCompareNCAt;
		else
			compareFunction = KWTupleCompareNNAt;
	}

	// Tri du tableau
	SetSortTupleTable(this);
	oaTuplesToSort->SetCompareFunction(compareFunction);
	oaTuplesToSort->Sort();
	SetSortTupleTable(NULL);

	// Nettoyage
	oaTuplesToSort->SetCompareFunction(NULL);
	nKWTupleTableSortIndex1 = -1;
	nKWTupleTableSortIndex2 = -1;
}

///////////////////////////////////////////////////////////////////////////////////
// Methode de KWTupleTable renvoyant la fonction de comparaison idoine

CompareFunction KWTupleTable::GetCompareFunction() const
{
	// On rend si possible une fonction de tri specialisee
	if (GetAttributeNumber() == 1)
	{
		if (GetAttributeTypeAt(0) == KWType::Symbol)
			return KWTupleCompareC;
		else
			return KWTupleCompareN;
	}
	else if (GetAttributeNumber() == 2)
	{
		if (GetAttributeTypeAt(0) == KWType::Symbol)
		{
			if (GetAttributeTypeAt(1) == KWType::Symbol)
				return KWTupleCompareCC;
			else
				return KWTupleCompareCN;
		}
		else
		{
			if (GetAttributeTypeAt(1) == KWType::Symbol)
				return KWTupleCompareNC;
			else
				return KWTupleCompareNN;
		}
	}
	else
		return KWTupleCompare;
}

CompareFunction KWTupleTable::GetCompareValuesFunction() const
{
	boolean bUsedSymbolAttributes;
	int nAttribute;

	// On determine si des attribut Symbol sont utilises
	bUsedSymbolAttributes = false;
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		if (GetAttributeTypeAt(nAttribute) == KWType::Symbol)
		{
			bUsedSymbolAttributes = true;
			break;
		}
	}

	// On renvoie la meme fonction de comparaison que sur les valeurs s'il n'y a pas d'attribut Symbol
	if (not bUsedSymbolAttributes)
		return GetCompareFunction();
	// Sinon, on renvoie une fonction specifique aux valeurs utilisateurs
	else
	{
		// On rend si possible une fonction de tri specialisee
		if (GetAttributeNumber() == 1)
		{
			assert(GetAttributeTypeAt(0) == KWType::Symbol);
			return KWTupleCompareValuesC;
		}
		else if (GetAttributeNumber() == 2)
		{
			if (GetAttributeTypeAt(0) == KWType::Symbol)
			{
				if (GetAttributeTypeAt(1) == KWType::Symbol)
					return KWTupleCompareValuesCC;
				else
					return KWTupleCompareValuesCN;
			}
			else
			{
				assert(GetAttributeTypeAt(1) == KWType::Symbol);
				return KWTupleCompareValuesNC;
			}
		}
		else
			return KWTupleCompareValues;
	}
}

CompareFunction KWTupleTable::GetCompareDecreasingFrequenciesFunction() const
{
	// On rend si possible une fonction de tri specialisee
	if (GetAttributeNumber() == 1)
	{
		if (GetAttributeTypeAt(0) == KWType::Symbol)
			return KWTupleCompareDecreasingFrequenciesC;
		else
			return KWTupleCompareDecreasingFrequenciesN;
	}
	else if (GetAttributeNumber() == 2)
	{
		if (GetAttributeTypeAt(0) == KWType::Symbol)
		{
			if (GetAttributeTypeAt(1) == KWType::Symbol)
				return KWTupleCompareDecreasingFrequenciesCC;
			else
				return KWTupleCompareDecreasingFrequenciesCN;
		}
		else
		{
			if (GetAttributeTypeAt(1) == KWType::Symbol)
				return KWTupleCompareDecreasingFrequenciesNC;
			else
				return KWTupleCompareDecreasingFrequenciesNN;
		}
	}
	else
		return KWTupleCompareDecreasingFrequencies;
}
