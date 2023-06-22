// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLKnapsackProblem.h"

PLKnapsackProblem::PLKnapsackProblem()
{
	nWeight = 0;
	slClasses.SetCompareFunction(PLClassCompare);
}

PLKnapsackProblem::~PLKnapsackProblem()
{
	oaClasses.RemoveAll();
}

void PLKnapsackProblem::SetWeight(int n)
{
	require(n >= 0);
	nWeight = n;
}

int PLKnapsackProblem::GetWeight() const
{
	return nWeight;
}

void PLKnapsackProblem::AddClass(PLClass* plClass)
{
	require(plClass != NULL);
	slClasses.Add(plClass);
	oaClasses.Add(plClass);
}

void PLKnapsackProblem::RemoveClasses()
{
	slClasses.RemoveAll();
	oaClasses.RemoveAll();
}

void PLKnapsackProblem::Solve()
{
	POSITION position;
	PLClass* plClass;
	PLItem* item;
	int nCurrentWeight;
	int nCurrentItemIndex;
	int nDeltaWeight;
	boolean bKnapsackIsFilled;
	int i;
	int nLastPosNumber; // nombre de classes ou l'item selectionne est le dernier

	///////////////////////////////////////////////////////////////
	// Cas particulier : tous les plus grands items rentrent dans le sac
	position = slClasses.GetHeadPosition();
	nCurrentWeight = 0;

	// Parcours des classe pour acceder au poids des items les plus lourds
	while (position != NULL)
	{
		plClass = cast(PLClass*, slClasses.GetNext(position));
		item = plClass->GetItemAt(plClass->GetItemNumber() - 1);
		ensure(item != NULL);
		nCurrentWeight += item->GetWeight();
	}

	// Si la somme des poids est inferieure au poids du sac
	if (nCurrentWeight <= this->GetWeight())
	{
		// Parcours des classe pour positionner le courant sur le dernier item
		for (i = 0; i < oaClasses.GetSize(); i++)
		{
			plClass = cast(PLClass*, oaClasses.GetAt(i));
			plClass->SetCurrentItemIndex(plClass->GetItemNumber() - 1);
		}

		// Si la solution verifie la contrainte globale
		if (DoesSolutionFitGlobalConstraint())
		{
			TagCurrentSolutionAsValid();
			return;
		}
		else
		{
			// Parcours des classe pour positionner le courant dans l'etat initial
			for (i = 0; i < oaClasses.GetSize(); i++)
			{
				plClass = cast(PLClass*, oaClasses.GetAt(i));
				plClass->UnselectItem();
			}
		}
	}

	///////////////////////////////////////////////////////////////
	// Cas standard
	nCurrentWeight = 0;

	bKnapsackIsFilled = false;
	nLastPosNumber = 0;
	while (not bKnapsackIsFilled and nLastPosNumber != oaClasses.GetSize())
	{
		// Acces a la classe qui apporte le plus de ressources
		plClass = cast(PLClass*, slClasses.RemoveTail());

		// Choix du prochain item
		nCurrentItemIndex = plClass->GetCurrentItemIndex();
		nCurrentItemIndex++;

		// Apport en poids du nouvel item
		nDeltaWeight = plClass->GetDeltaWeight(nCurrentItemIndex);
		if (nCurrentWeight + nDeltaWeight <= nWeight)
		{
			nCurrentWeight += nDeltaWeight;

			plClass->SetCurrentItemIndex(nCurrentItemIndex);

			if (plClass->IsLastItem())
				nLastPosNumber++;

			// Si la contrainte globale est respectee, on tag cette solution
			if (DoesSolutionFitGlobalConstraint())
				TagCurrentSolutionAsValid();

			// Si ce n'est pas le dernier item, on remet la classe dans la liste
			if (not plClass->IsLastItem())
				slClasses.Add(plClass);
			bKnapsackIsFilled = nCurrentWeight == nWeight;
		}
		else
			bKnapsackIsFilled = true;
	}
}

boolean PLKnapsackProblem::DoesSolutionFitGlobalConstraint()
{
	return true;
}

boolean PLKnapsackProblem::IsGlobalConstraintViolated() const
{
	int i;
	i = 0;
	for (i = 0; i < oaClasses.GetSize(); i++)
	{
		// Si il y a au moins un item valide, La solution est valide
		if (cast(PLClass*, oaClasses.GetAt(i))->GetValidItemIndex() != -1)
			return false;
	}
	return true;
}

void PLKnapsackProblem::Write(ostream& ost) const
{
	int i;

	ost << "--   Knapsack Problem  W <= " << nWeight << " --" << endl << endl;

	// Affichage de chaque classe
	i = 0;
	for (i = 0; i < oaClasses.GetSize(); i++)
	{
		cast(PLClass*, oaClasses.GetAt(i))->Write(ost);
		ost << endl << endl;
	}
}

void PLKnapsackProblem::Test()
{
	PLKnapsackProblem* problem;
	PLClass* plClass;
	ObjectArray oaClass;
	PLItem* item;
	int i, j, k;
	longint lValue;
	ObjectArray oaItems;

	// Parametres
	const int nClassesNumber = 4;
	const int nItemNumber = 8;
	const int nValueNumber = 4;
	const int nWeightMax = 30; // Max = nItemNumber * nClassesNumber

	// Construction du probleme
	problem = new PLKnapsackProblem;
	problem->SetWeight(nWeightMax);

	// Construction de chaque classe
	for (k = 0; k < nClassesNumber; k++)
	{
		plClass = new PLClass;

		// Construction des items de la classe
		lValue = 0;
		oaItems.SetSize(0);
		for (i = 0; i < nItemNumber; i++)
		{
			item = new PLItem;
			item->SetWeight(i + 1);
			for (j = 0; j < nValueNumber; j++)
			{
				lValue += RandomInt(10);
				item->Add(lValue);
			}
			oaItems.Add(item);
		}
		plClass->SetItems(&oaItems);
		oaClass.Add(plClass);
		problem->AddClass(plClass);
	}

	// Affichage
	cout << *problem;

	// Resolution
	problem->Solve();

	// Affichage de la solution
	cout << endl << "Solution : " << endl << endl;
	for (i = 0; i < oaClass.GetSize(); i++)
	{
		cout << *oaClass.GetAt(i) << endl << endl;
	}

	// Nettoyage
	delete problem;
	oaClass.DeleteAll();
}

void PLKnapsackProblem::TagCurrentSolutionAsValid()
{
	PLClass* myClass;
	int nCurrentItemIndex;
	int i;

	for (i = 0; i < oaClasses.GetSize(); i++)
	{
		myClass = cast(PLClass*, oaClasses.GetAt(i));
		nCurrentItemIndex = myClass->GetCurrentItemIndex();

		// Si cette classe fait partie de la solution, on tag l'index valide
		if (nCurrentItemIndex != -1)
			myClass->SetValidItemIndex(nCurrentItemIndex);
	}
}

////////////////////////////////////////////////////
// Implementation de la classe PLClass

PLClass::PLClass()
{
	nCurrentItem = -1;
	nValidItem = -1;
}
PLClass::~PLClass()
{
	oaItems.DeleteAll();
}

void PLClass::SetCurrentItemIndex(int nIndex)
{
	require(nIndex >= 0);
	nCurrentItem = nIndex;

	// Mise a jour du delta
	PLItem::ComputeDelta(GetItemAt(nCurrentItem), GetItemAt(nCurrentItem + 1), &vDelta);
}

int PLClass::GetCurrentItemIndex() const
{
	return nCurrentItem;
}

void PLClass::UnselectItem()
{
	nCurrentItem = -1;

	// Initialisation
	PLItem::ComputeDelta(NULL, GetItemAt(0), &vDelta);
}

boolean PLClass::IsLastItem() const
{
	return nCurrentItem == oaItems.GetSize() - 1;
}

void PLClass::SetValidItemIndex(int nIndex)
{
	require(nIndex >= 0);
	nValidItem = nIndex;
}
int PLClass::GetValidItemIndex() const
{
	return nValidItem;
}

void PLClass::SetItems(const ObjectArray* oaValues)
{
	int i;
	require(oaValues != NULL);
	require(oaValues->GetSize() > 0);

	// Ajout de chaque item
	for (i = 0; i < oaValues->GetSize(); i++)
	{
		require(cast(PLItem*, oaValues->GetAt(i)) != NULL);
		oaItems.Add(cast(PLItem*, oaValues->GetAt(i)));
	}

	// Tri des items suivant leur poids
	oaItems.SetCompareFunction(PLItemCompare);
	oaItems.Sort();

	// Initialisation
	PLItem::ComputeDelta(NULL, cast(PLItem*, oaItems.GetAt(0)), &vDelta);
}

PLItem* PLClass::GetItemAt(int nIndex) const
{
	if (nIndex == -1 or nIndex >= oaItems.GetSize())
		return NULL;
	return cast(PLItem*, oaItems.GetAt(nIndex));
}

int PLClass::GetItemNumber() const
{
	return oaItems.GetSize();
}

PLItem* PLClass::GetCurrentItem() const
{
	require(nCurrentItem < oaItems.GetSize());
	if (nCurrentItem == -1)
		return NULL;
	return cast(PLItem*, oaItems.GetAt(nCurrentItem));
}

PLItem* PLClass::GetValidItem() const
{
	require(nValidItem < oaItems.GetSize());
	if (nValidItem == -1)
		return NULL;
	return cast(PLItem*, oaItems.GetAt(nValidItem));
}

const LongintVector* PLClass::GetDelta()
{
	return &vDelta;
}

int PLClass::GetDeltaWeight(int nIndex) const
{
	int nOldWeight;
	require(nIndex < oaItems.GetSize());

	// Si l'item est le premier, le poids d'avant est nul, il apporte tout son poids
	if (nIndex == 0)
		nOldWeight = 0;
	else
		nOldWeight = cast(PLItem*, oaItems.GetAt(nIndex - 1))->GetWeight();
	return cast(PLItem*, oaItems.GetAt(nIndex))->GetWeight() - nOldWeight;
}

void PLClass::Write(ostream& ost) const
{
	int i;
	longint lDelta;

	// Ecriture des items
	for (i = 0; i < oaItems.GetSize(); i++)
	{
		cast(PLItem*, oaItems.GetAt(i))->Write(ost);
		if (i == nCurrentItem)
			ost << " <= Current Item";
		if (i == nValidItem)
			ost << " <= Valid Item";
		ost << endl;
	}

	// Ecriture du Delta
	ost << "Delta :";
	for (i = 0; i < vDelta.GetSize(); i++)
	{
		lDelta = vDelta.GetAt(i);
		if (lDelta > 0)
			ost << " " << LongintToHumanReadableString(lDelta);
		else
			ost << " " << lDelta;
	}
}

int PLClassCompare(const void* first, const void* second)
{
	PLClass* class1;
	PLClass* class2;
	int i;

	require(first != NULL);
	require(second != NULL);

	// Acces aux objets
	class1 = cast(PLClass*, *(Object**)first);
	class2 = cast(PLClass*, *(Object**)second);

	require(not class1->IsLastItem());
	require(not class2->IsLastItem());
	require(class1->GetDelta()->GetSize() == class2->GetDelta()->GetSize());

	// Comparaison des deltas
	for (i = 0; i < class1->GetDelta()->GetSize(); i++)
	{
		if (class1->GetDelta()->GetAt(i) > class2->GetDelta()->GetAt(i))
			return 1;
		else if (class1->GetDelta()->GetAt(i) < class2->GetDelta()->GetAt(i))
			return -1;
	}
	return 0;
}

////////////////////////////////////////////////////
// Implementation de la classe PLItem
PLItem::PLItem()
{
	nWeight = -1;
}
PLItem::~PLItem() {}

int PLItem::GetWeight() const
{
	return nWeight;
}

void PLItem::SetWeight(int w)
{
	require(w >= 0);
	nWeight = w;
}

longint PLItem::GetValueAt(int nIndex) const
{
	require(vValues.GetSize() > nIndex);
	return vValues.GetAt(nIndex);
}

void PLItem::Add(longint nValue)
{
	vValues.Add(nValue);
}

void PLItem::ComputeDelta(const PLItem* item1, const PLItem* item2, LongintVector* ivDelta)
{
	int i;
	longint nDelta;

	require(not(item1 == NULL and item2 == NULL));
	require(ivDelta != NULL);

	// Si les 2 items sont non null
	if (item1 != NULL and item2 != NULL)
	{
		require(item1->vValues.GetSize() == item2->vValues.GetSize());

		// Initialisation du vecteur
		ivDelta->SetSize(0);

		// Soustraction valeur par valeur
		for (i = 0; i < item1->vValues.GetSize(); i++)
		{
			nDelta = item2->vValues.GetAt(i) - item1->vValues.GetAt(i);
			// ensure(nDelta >= -1);// En theorie c'est 0 mais il peut y avoir des pbms d'arrondi
			if (nDelta < 0)
				nDelta = 0;
			ivDelta->Add(nDelta);
		}
	}
	else
		// Si pas de premier item, c'est l'apport de l'ajout du premier item de la classe : item2
		if (item1 == NULL)
		{
			// Copie des valeurs de item2
			ivDelta->CopyFrom(&item2->vValues);
		}
		else
		{
			// Si pas de second item, le dernier item a deja ete ajoute, c'est un effet de bord, le delta
			// vaut 0
			ivDelta->SetSize(item1->vValues.GetSize());
			ivDelta->Initialize();
		}
}

void PLItem::Write(ostream& ost) const
{
	int i;

	// Affichage du poids
	ost << "W:" << nWeight;

	// Affichage des valeurs
	ost << " Values";
	for (i = 0; i < vValues.GetSize(); i++)
		ost << " " << LongintToHumanReadableString(vValues.GetAt(i));
}

int PLItemCompare(const void* first, const void* second)
{
	PLItem* item1;
	PLItem* item2;

	require(first != NULL);
	require(second != NULL);

	// acces aux objets
	item1 = cast(PLItem*, *(Object**)first);
	item2 = cast(PLItem*, *(Object**)second);

	return item1->GetWeight() - item2->GetWeight();
}