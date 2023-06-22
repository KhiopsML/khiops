// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class PLKnapsackProblem;
class PLClass;
class PLItem;

#include "Object.h"
#include "Vector.h"
#include "SortedList.h"

//////////////////////////////////////////////////////////////////////////
// Classe PLKnapsackProblem
// Cette classe permet de resoudre une variante du probleme du sac a dos (Knapsack Problem)
// Dans cette variante :
//  -le sac a dos est a choix multiple : les objets sont regroupes en classes, on ne prend qu'un objet de chaque classe
//	-le poids du sac est un entier
//  -les items qui composent les classes, ont un poids croissant de 1 a 1, le plus petit item a un poids de 1
class PLKnapsackProblem : public Object
{
public:
	PLKnapsackProblem();
	~PLKnapsackProblem();

	// Poids max
	void SetWeight(int n);
	int GetWeight() const;

	// Ajout d'une classe
	void AddClass(PLClass* plClass); // La classe appartient a l'appelant
	void RemoveClasses();

	// Resoud le probleme: maximise les valeurs des items en respectant le poids maximum du sac
	// Solution : positionne les items courants dans les classes
	void Solve();

	// Renvoie true si aucune solution ne verifie la contrainte globale
	boolean IsGlobalConstraintViolated() const;

	// Ecriture
	void Write(ostream& ost) const override;

	// Test
	static void Test();

protected:
	// Methode virtuelle qui permet d'ajouter une contrainte globale
	// Renvoie toujours true, a specialiser
	virtual boolean DoesSolutionFitGlobalConstraint();

	// Tag toutes les positions courantes comme valide
	void TagCurrentSolutionAsValid();

	int nWeight;           // Poids Maximum
	SortedList slClasses;  // Liste triee de classes
	ObjectArray oaClasses; // Liste des classes
};

//////////////////////////////////////////////////////////////////////////
// Classe PLClass
// Une classe est un ensemble d'items, on ne peut prendre qu'un seul item de chaque classe
// pour remplir le sac (Probleme du sac a dos a choix multiple).
// Pour l'algorithme de resolution , il y a la notion d'item courant (c'est l'item de cette classe qui est dans le sac)
// et la notion d'item de la derniere solution valide L'ordre entre 2 classes est definie par la comparaison entre leurs
// "apports potentiel". L'apport potentiel d'une classe est l'apport en valeur que l'on a si on remplace l'item courant
// par l'item suivant. Il y a plusieurs valeurs, c'est donc un tri multiple, c'est la place de la valeur dans le tableau
// qui defini sa priorite : la premiere valeur est prioritaire sur la deuxieme etc...
class PLClass : public Object
{
public:
	PLClass();
	~PLClass();

	// Index de l'item selectionne
	void SetCurrentItemIndex(int nIndex);
	int GetCurrentItemIndex() const; // -1 si pas d'item selectionne
	void UnselectItem();

	// Le dernier item est selectionne
	boolean IsLastItem() const;

	// Index de l'item valide
	void SetValidItemIndex(int nIndex);
	int GetValidItemIndex() const; // -1 si pas d'item valide

	// Ajout des items (ils appartiennent a l'appele)
	void SetItems(const ObjectArray* oaItems);

	// Acces aux item
	PLItem* GetItemAt(int nIndex) const; // Si l'index est au dehors des bornes return NULL
	int GetItemNumber() const;
	PLItem* GetCurrentItem() const;
	PLItem* GetValidItem() const;

	// Acces au deltas
	const LongintVector* GetDelta();

	// Poids supplementaire entre l'item d'index nIndex et celui de nIndex-1
	int GetDeltaWeight(int nIndex) const;

	// Ecriture
	void Write(ostream& ost) const override;

protected:
	ObjectArray oaItems; // Tableau d'items
	int nCurrentItem;
	int nValidItem;
	LongintVector vDelta; // Apport des valeurs si on remplace l'item courant par le suivant
};

// Fonction de comparaison entre 2 PLClass
int PLClassCompare(const void* first, const void* second);

//////////////////////////////////////////////////////////////////////////
// Classe PLItem
// Un item represente un des objets contenus dans le sac a dos
// Un item a un poids et plusieurs valeurs, le poids constitue la borne du sac
// les valeurs sont les quantites a optimiser.
// L'ordre entre 2 items est defini par l'ordre des poids.
class PLItem : public Object
{
public:
	PLItem();
	~PLItem();

	// Acces au poids
	int GetWeight() const;
	void SetWeight(int w);

	// Acces aux valeurs
	longint GetValueAt(int nIndex) const;
	void Add(longint nValue);

	// Calcul de l'apport de l'item2 par rapport a l'item1
	// Si l'item1 est null, l'apport = les valeurs de l'item2
	// Si l'item2 est null l'apport vaut 0
	// TODO metter cette methode dans la classe PLClass et nepas recalculer le delta tout le temps...
	static void ComputeDelta(const PLItem* item1, const PLItem* item2, LongintVector* ivDelta);

	// Ecriture
	void Write(ostream& ost) const override;

protected:
	LongintVector vValues;
	int nWeight;
};

// Fonction de comparaison entre deux PLItem
int PLItemCompare(const void* first, const void* second);
