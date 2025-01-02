// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SortedList.h"

SortedList::SortedList()
{
	rootNode = NULL;
	headNode = NULL;
	tailNode = NULL;
	fCompareFunction = NULL;
	freeNodes = NULL;
	nFreeNodeNumber = 0;
}

SortedList::~SortedList()
{
	RemoveAll();
}

SortedList::SortedList(CompareFunction fCompare)
{
	require(fCompare != NULL);

	rootNode = NULL;
	headNode = NULL;
	tailNode = NULL;
	fCompareFunction = fCompare;
	freeNodes = NULL;
	nFreeNodeNumber = 0;
}

void SortedList::SetCompareFunction(CompareFunction fCompare)
{
	require(fCompare != NULL);
	fCompareFunction = fCompare;
}

POSITION SortedList::Add(Object* object)
{
	require(fCompareFunction != NULL);
	require(object != NULL);

	// Cas ou la liste est vide: creation d'une nouvelle racine
	if (IsEmpty())
	{
		assert(rootNode == NULL);

		// Creation de la racine
		rootNode = NewNode();
		headNode = rootNode;
		tailNode = rootNode;
		olSortedElements.AddHead(object);
		rootNode->SetTieHeadPosition(olSortedElements.GetHeadPosition());
		return olSortedElements.GetHeadPosition();
	}
	// Sinon, il faut ajouter l'element au noeud racine
	else
		return AVLNodeAdd(rootNode, object);
}

POSITION SortedList::AVLNodeAdd(AVLNode* node, Object* object)
{
	AVLNode* nodeRef;
	Object* objectRef;
	int nCompare;
	AVLNode* newNode;
	POSITION newPosition;

	require(node != NULL);
	require(node->GetTieHeadPosition() != NULL);
	require(object != NULL);

	// Recherche du noeud de l'arbre ou effectuer la destruction
	nodeRef = node;
	nCompare = 1;
	while (nCompare != 0)
	{
		check(nodeRef);

		// Comparaison avec le nouvel element
		objectRef = olSortedElements.GetAt(nodeRef->GetTieHeadPosition());
		nCompare = fCompareFunction(&object, &objectRef);

		// Si egalite: ajout dans la liste des doublons apres le
		// premier doublons porte par le noeud
		if (nCompare == 0)
			return olSortedElements.InsertAfter(nodeRef->GetTieHeadPosition(), object);
		// Si negatif, il faut inserer avant
		else if (nCompare < 0)
		{
			// Si pas de sous arbre gauche: creation d'un nouveau noeud
			if (nodeRef->GetLeftNode() == NULL)
			{
				// Creation et chainage
				newNode = NewNode();
				nodeRef->SetLeftNode(newNode);
				newNode->SetParentNode(nodeRef);

				// Modification eventuellement du noeud debut de liste
				if (nodeRef == headNode)
					headNode = newNode;

				// Ajout de l'objet dans la liste globale
				newPosition = olSortedElements.InsertBefore(nodeRef->GetTieHeadPosition(), object);
				newNode->SetTieHeadPosition(newPosition);

				// Reequilibrage de l'arbre
				AVLNodeLeftBalance(nodeRef, true);

				return newPosition;
			}
			// Sinon, insertion dans le sous arbre gauche
			else
				nodeRef = nodeRef->GetLeftNode();
		}
		// Si positif, il faut inserer apres
		else
		{
			// Si pas de sous arbre droite: creation d'un nouveau noeud
			if (nodeRef->GetRightNode() == NULL)
			{
				// Creation et chainage
				newNode = NewNode();
				nodeRef->SetRightNode(newNode);
				newNode->SetParentNode(nodeRef);

				// Modification eventuellement du noeud fin de liste
				if (nodeRef == tailNode)
					tailNode = newNode;

				// Ajout de l'objet dans la liste globale
				if (newNode == tailNode)
					newPosition = olSortedElements.AddTail(object);
				else
					newPosition = olSortedElements.InsertBefore(
					    newNode->GetNext()->GetTieHeadPosition(), object);
				newNode->SetTieHeadPosition(newPosition);

				// Reequilibrage de l'arbre
				AVLNodeRightBalance(nodeRef, true);

				return newPosition;
			}
			// Sinon, insertion dans le sous arbre gauche
			else
				nodeRef = nodeRef->GetRightNode();
		}
	}
	assert(false);
	return NULL;
}

void SortedList::RemoveAt(POSITION position)
{
	Object* object;
	Object* objectRef;
	AVLNode* node;
	int nCompare;

	require(fCompareFunction != NULL);
	require(position != NULL);
	require(not IsEmpty());

	// Recherche de l'objet courant
	object = olSortedElements.GetAt(position);

	// Si debut de liste, on connait le premier noeud
	if (position == olSortedElements.GetHeadPosition())
	{
		AVLNodeRemoveAt(headNode, position);
		return;
	}
	// Si au moins un element precedent ayant meme valeur,
	// on peut detruire le doublon sans probleme
	else
	{
		POSITION prevPosition;

		// Recherche de l'objet precedent
		prevPosition = position;
		olSortedElements.GetPrev(prevPosition);
		objectRef = olSortedElements.GetAt(prevPosition);

		// Comparaison avec cet objet
		if (fCompareFunction(&object, &objectRef) == 0)
		{
			olSortedElements.RemoveAt(position);
			return;
		}
	}

	// Si fin de liste: on connait le dernier noeud
	if (position == olSortedElements.GetTailPosition())
	{
		AVLNodeRemoveAt(tailNode, position);
		return;
	}

	// Recherche du noeud de l'arbre ou effectuer la destruction
	node = rootNode;
	nCompare = 1;
	while (nCompare != 0)
	{
		check(node);

		// Comparaison avec le nouvel element
		objectRef = olSortedElements.GetAt(node->GetTieHeadPosition());
		nCompare = fCompareFunction(&object, &objectRef);

		// Si negatif recherche dans le sous-arbre gauche
		if (nCompare < 0)
			node = node->GetLeftNode();
		// Si positif recherche dans le sous-arbre droite
		else if (nCompare > 0)
			node = node->GetRightNode();
	}

	// Supression dans le noeud trouve
	AVLNodeRemoveAt(node, position);
}

void SortedList::AVLNodeRemoveAt(AVLNode* node, POSITION position)
{
	boolean bIsNodeEmpty;

	require(node != NULL);
	require(position != NULL);

	// On verifie la coherence de la position et du noeud
	debug(Object * nodeObjectDebug);
	debug(Object * removedObjectDebug);
	debug(nodeObjectDebug = olSortedElements.GetAt(node->GetTieHeadPosition()));
	debug(removedObjectDebug = olSortedElements.GetAt(position));
	assert(fCompareFunction(&nodeObjectDebug, &removedObjectDebug) == 0);

	// Supression de la liste des doublons
	// Si position differente de celle du noeud: pas de probleme
	if (node->GetTieHeadPosition() != position)
	{
		olSortedElements.RemoveAt(position);
		bIsNodeEmpty = false;
	}
	// Si fin de liste: le noeud sera vide
	else if (olSortedElements.GetTailPosition() == position)
	{
		assert(node == tailNode);
		assert(node->GetTieHeadPosition() == position);
		olSortedElements.RemoveTail();
		bIsNodeEmpty = true;
	}
	// Cas general
	else
	{
		Object* nodeObject;
		POSITION nextPosition;
		Object* nextObject;

		// Recherche de l'element porte par le noeud
		assert(node->GetTieHeadPosition() == position);
		nodeObject = olSortedElements.GetAt(position);

		// Recherche de l'element suivant
		nextPosition = position;
		olSortedElements.GetNext(nextPosition);
		nextObject = olSortedElements.GetAt(nextPosition);

		// Si meme valeur de cle: on peut simplement deplacer
		// le noeud sur la liste
		if (fCompareFunction(&nodeObject, &nextObject) == 0)
		{
			node->SetTieHeadPosition(nextPosition);
			olSortedElements.RemoveAt(position);
			bIsNodeEmpty = false;
		}
		// Sinon: le noeud deviendra vide
		else
		{
			olSortedElements.RemoveAt(position);
			bIsNodeEmpty = true;
		}
	}

	// Si liste de doublons vide: mise a jour de l'arbre par
	// supression du noeud
	if (bIsNodeEmpty)
	{
		// Si noeud terminal: remplacement par son noeud parent
		if (node->GetLeftNode() == NULL and node->GetRightNode() == NULL)
		{
			// Si noeud racine: supression de la racine
			if (node == rootNode)
			{
				assert(node->GetParentNode() == NULL);
				rootNode = NULL;
				headNode = NULL;
				tailNode = NULL;
			}
			// Sinon: supression
			else
			{
				assert(node->GetParentNode() != NULL);

				// Mise a jour du parent
				if (node->GetParentNode()->GetLeftNode() == node)
				{
					node->GetParentNode()->SetLeftNode(NULL);
					if (node == headNode)
						headNode = node->GetParentNode();

					// Reequilibrage de l'arbre
					AVLNodeLeftBalance(node->GetParentNode(), false);
				}
				else
				{
					node->GetParentNode()->SetRightNode(NULL);
					if (node == tailNode)
						tailNode = node->GetParentNode();

					// Reequilibrage de l'arbre
					AVLNodeRightBalance(node->GetParentNode(), false);
				}
			}
		}
		// Si noeud avec deux fils: reorganisation de l'arbre
		// On remplace le noeud par le dernier noeud de sous-arbre gauche
		else if (node->GetLeftNode() != NULL and node->GetRightNode() != NULL)
		{
			AVLNode* movedNode;
			AVLNode* leftNode;
			AVLNode* rightNode;
			AVLNode* unbalancedNode;
			boolean bLeftUnbalance;

			// Recherche du noeud a deplacer
			movedNode = node->GetLeftNode()->GetTail();
			assert(movedNode->GetRightNode() == NULL);
			assert(movedNode == node->GetLeftNode() or
			       movedNode->GetParentNode()->GetRightNode() == movedNode);

			// Recherche du noeud a reequilibrer
			if (movedNode == node->GetLeftNode())
			{
				unbalancedNode = movedNode;
				bLeftUnbalance = true;
			}
			else
			{
				unbalancedNode = movedNode->GetParentNode();
				bLeftUnbalance = false;
			}
			movedNode->SetBalance(node->GetBalance());

			// Recherche des sous arbres du futur noeud
			leftNode = node->GetLeftNode();
			if (leftNode == movedNode)
				leftNode = leftNode->GetLeftNode();
			rightNode = node->GetRightNode();

			// Dechainage du noeud deplace
			// Si pas de descendant: on met a jour le parent
			if (movedNode->GetLeftNode() == NULL)
				movedNode->GetParentNode()->SetRightNode(NULL);
			// Sinon, on le remplace par son sous-arbre gauche
			else
			{
				movedNode->GetParentNode()->SetRightNode(movedNode->GetLeftNode());
				movedNode->GetLeftNode()->SetParentNode(movedNode->GetParentNode());
				movedNode->SetLeftNode(NULL);
			}

			// Chainage des sous-arbres au noeud deplace
			assert(movedNode->GetLeftNode() == NULL);
			movedNode->SetLeftNode(leftNode);
			if (leftNode != NULL)
				leftNode->SetParentNode(movedNode);
			movedNode->SetRightNode(rightNode);
			rightNode->SetParentNode(movedNode);

			// Si noeud racine: remplacement de la racine
			if (node == rootNode)
			{
				movedNode->SetParentNode(NULL);
				rootNode = movedNode;
			}
			// Sinon, chainage avec le parent
			else
			{
				movedNode->SetParentNode(node->GetParentNode());
				if (node->GetParentNode()->GetLeftNode() == node)
					node->GetParentNode()->SetLeftNode(movedNode);
				else
					node->GetParentNode()->SetRightNode(movedNode);
			}

			// Reequilibrage de l'arbre
			if (bLeftUnbalance)
				AVLNodeLeftBalance(unbalancedNode, false);
			else
				AVLNodeRightBalance(unbalancedNode, false);
		}
		// Si un seul descendant (a gauche): remplacement par le descendant
		else if (node->GetLeftNode() != NULL)
		{
			assert(node->GetLeftNode() != NULL);
			assert(node->GetRightNode() == NULL);

			// Si noeud racine: remplacement de la racine
			if (node == rootNode)
			{
				assert(node->GetParentNode() == NULL);
				rootNode = node->GetLeftNode();
				rootNode->SetParentNode(NULL);
				tailNode = rootNode->GetTail();

				// La racine devrait etre equilibree
				rootNode->SetBalance(0);
			}
			// Sinon: remplacement dans le noeud pere
			else
			{
				assert(node->GetParentNode() != NULL);

				// Accrochage du noeud fils au noeud pere
				if (node->GetParentNode()->GetLeftNode() == node)
				{
					node->GetParentNode()->SetLeftNode(node->GetLeftNode());
					node->GetLeftNode()->SetParentNode(node->GetParentNode());

					// Reequilibrage de l'arbre
					AVLNodeLeftBalance(node->GetParentNode(), false);
				}
				else
				{
					node->GetParentNode()->SetRightNode(node->GetLeftNode());
					node->GetLeftNode()->SetParentNode(node->GetParentNode());
					if (tailNode == node)
						tailNode = node->GetLeftNode()->GetTail();

					// Reequilibrage de l'arbre
					AVLNodeRightBalance(node->GetParentNode(), false);
				}
			}
		}
		// Si un seul descendant (a droite): remplacement par le descendant
		else
		{
			assert(node->GetLeftNode() == NULL);
			assert(node->GetRightNode() != NULL);

			// Si noeud racine: remplacement de la racine
			if (node == rootNode)
			{
				assert(node->GetParentNode() == NULL);
				rootNode = node->GetRightNode();
				rootNode->SetParentNode(NULL);
				headNode = rootNode->GetHead();

				// La racine devrait etre equilibree
				rootNode->SetBalance(0);
			}
			// Sinon: remplacement dans le noeud pere
			else
			{
				assert(node->GetParentNode() != NULL);

				// Accrochage du noeud fils au noeud pere
				if (node->GetParentNode()->GetLeftNode() == node)
				{
					node->GetParentNode()->SetLeftNode(node->GetRightNode());
					node->GetRightNode()->SetParentNode(node->GetParentNode());
					if (headNode == node)
						headNode = node->GetRightNode()->GetHead();

					// Reequilibrage de l'arbre
					AVLNodeLeftBalance(node->GetParentNode(), false);
				}
				else
				{
					node->GetParentNode()->SetRightNode(node->GetRightNode());
					node->GetRightNode()->SetParentNode(node->GetParentNode());

					// Reequilibrage de l'arbre
					AVLNodeRightBalance(node->GetParentNode(), false);
				}
			}
		}

		// Destruction du noeud
		FreeNode(node);
	}
}

void SortedList::RemoveAll()
{
	if (rootNode != NULL)
	{
		// Suppression des elements de la liste
		olSortedElements.RemoveAll();

		// Supression recursive de l'arbre binaire depuis la racine
		rootNode->DeleteSubTrees();
		FreeNode(rootNode);

		// Reinitialisation des pointeurs
		rootNode = NULL;
		headNode = NULL;
		tailNode = NULL;
	}

	// Destructin des noeuds libres
	DeleteAllFreeNodes();
}

void SortedList::DeleteAll()
{
	// Destruction des elements de la liste
	olSortedElements.DeleteAll();

	// Supression de l'arbre binaire
	RemoveAll();

	// Destructin des noeuds libres
	DeleteAllFreeNodes();
}

POSITION SortedList::Find(const Object* searchValue) const
{
	AVLNode* node;
	Object* objectRef;
	int nCompare;

	require(fCompareFunction != NULL);
	require(searchValue != NULL);

	// Recherche du noeud de l'arbre ayant meme valeur
	node = rootNode;
	nCompare = 1;
	while (node != NULL and nCompare != 0)
	{
		// Comparaison avec le nouvel element
		objectRef = olSortedElements.GetAt(node->GetTieHeadPosition());
		nCompare = fCompareFunction(&searchValue, &objectRef);

		// Si negatif recherche dans le sous-arbre gauche
		if (nCompare < 0)
			node = node->GetLeftNode();
		// Si positif recherche dans le sous-arbre droite
		else if (nCompare > 0)
			node = node->GetRightNode();
		else
			return node->GetTieHeadPosition();
	}
	return NULL;
}

POSITION SortedList::FindGreater(const Object* searchValue) const
{
	AVLNode* node;
	AVLNode* bestNode;
	Object* objectRef;
	int nCompare;

	require(fCompareFunction != NULL);
	require(searchValue != NULL);

	// Recherche du noeud de l'arbre une valeur superieure
	node = rootNode;
	bestNode = NULL;
	while (node != NULL)
	{
		// Comparaison avec le nouvel element
		objectRef = olSortedElements.GetAt(node->GetTieHeadPosition());
		nCompare = fCompareFunction(&searchValue, &objectRef);

		// Si negatif recherche dans le sous-arbre gauche
		if (nCompare < 0)
		{
			bestNode = node;
			node = node->GetLeftNode();
		}
		// Si positif recherche dans le sous-arbre droite
		else if (nCompare > 0)
			node = node->GetRightNode();
		else
			return node->GetTieHeadPosition();
	}

	// On retourne le premier doublon de l'eventuel
	// noeud trouve
	if (bestNode == NULL)
		return NULL;
	else
		return bestNode->GetTieHeadPosition();
}

POSITION SortedList::FindLower(const Object* searchValue) const
{
	AVLNode* node;
	AVLNode* bestNode;
	Object* objectRef;
	int nCompare;

	require(fCompareFunction != NULL);
	require(searchValue != NULL);

	// Recherche du noeud de l'arbre une valeur inferieure
	node = rootNode;
	bestNode = NULL;
	while (node != NULL)
	{
		// Comparaison avec le nouvel element
		objectRef = olSortedElements.GetAt(node->GetTieHeadPosition());
		nCompare = fCompareFunction(&searchValue, &objectRef);

		// Si negatif recherche dans le sous-arbre gauche
		if (nCompare < 0)
			node = node->GetLeftNode();
		// Si positif recherche dans le sous-arbre droite
		else if (nCompare > 0)
		{
			bestNode = node;
			node = node->GetRightNode();
		}
		else
		{
			bestNode = node;
			node = NULL;
		}
	}

	// On retourne le dernier doublon de l'eventuel
	// noeud trouve
	if (bestNode == NULL)
		return NULL;
	else
	{
		if (bestNode == tailNode)
			return olSortedElements.GetTailPosition();
		else
		{
			POSITION foundPosition;

			// On retourne la position precedent celle du
			// premier doublon du noued suivant
			bestNode = bestNode->GetNext();
			foundPosition = bestNode->GetTieHeadPosition();
			olSortedElements.GetPrev(foundPosition);
			return foundPosition;
		}
	}
}

void SortedList::CopyFrom(const SortedList* slSource)
{
	POSITION position;

	require(slSource != NULL);

	// Cas particulier ou source egale cible
	if (slSource == this)
		return;

	// Nettoyage
	RemoveAll();

	// Parametrage de la fonction de comparaison
	SetCompareFunction(slSource->GetCompareFunction());

	// Recopie
	position = slSource->olSortedElements.GetHeadPosition();
	while (position != NULL)
		Add(slSource->olSortedElements.GetNext(position));
}

SortedList* SortedList::Clone() const
{
	SortedList* slClone;
	POSITION position;

	slClone = new SortedList(GetCompareFunction());
	position = olSortedElements.GetHeadPosition();
	while (position != NULL)
		slClone->Add(olSortedElements.GetNext(position));
	return slClone;
}

void SortedList::ExportObjectArray(ObjectArray* oaResult) const
{
	require(oaResult != NULL);

	olSortedElements.ExportObjectArray(oaResult);
	oaResult->SetCompareFunction(GetCompareFunction());

	ensure(oaResult->GetSize() == GetCount());
}

void SortedList::ExportObjectList(ObjectList* olResult) const
{
	require(olResult != NULL);

	olResult->CopyFrom(&olSortedElements);

	ensure(olResult->GetCount() == GetCount());
}

void SortedList::Write(ostream& ost) const
{
	POSITION position;
	Object* object;
	const int nMax = 10;
	int n;

	// Parcours des elements de la liste pour alimenter le tableau
	ost << GetClassLabel() << " [" << GetCount() << "]\n";
	position = olSortedElements.GetHeadPosition();
	n = 0;
	while (position != NULL)
	{
		n++;
		object = olSortedElements.GetNext(position);
		if (object == NULL)
			ost << "\tnull\n";
		else
			ost << "\t" << *object << "\n";
		if (n >= nMax)
		{
			ost << "\t...\n";
			break;
		}
	}
}

longint SortedList::GetUsedMemory() const
{
	return sizeof(SortedList) + GetCount() * sizeof(AVLNode) + olSortedElements.GetUsedMemory() -
	       sizeof(ObjectList);
}

longint SortedList::GetOverallUsedMemory() const
{
	return sizeof(SortedList) + olSortedElements.GetOverallUsedMemory() - sizeof(ObjectList);
}

longint SortedList::GetUsedMemoryPerElement() const
{
	return sizeof(AVLNode) + olSortedElements.GetUsedMemoryPerElement();
}

const ALString SortedList::GetClassLabel() const
{
	return "Sorted list";
}

void SortedList::AVLNodeLeftBalance(AVLNode* node, boolean bGrow)
{
	require(node != NULL);

	// Si le SAG a diminue: equivalent augmentation SAD
	if (not bGrow)
	{
		// Si SAG max: amelioration de l'equilibre
		if (node->GetBalance() == -1)
		{
			node->SetBalance(0);

			// Propagation au noeud pere
			if (node != rootNode)
			{
				check(node->GetParentNode());
				if (node->GetParentNode()->GetLeftNode() == node)
					AVLNodeLeftBalance(node->GetParentNode(), false);
				else
					AVLNodeRightBalance(node->GetParentNode(), false);
			}
		}
		// Si noeud equilibre: le SAD devient max
		else if (node->GetBalance() == 0)
			node->SetBalance(1);
		// Si SAD max: il faut reequilibrer l'arbre
		else
		{
			AVLNode* parentNode;

			AVLNodeRightBalance(node, true);

			// Le noeud issu du reequilibrage est le noeud pere
			parentNode = node->GetParentNode();
			check(parentNode);

			// Propagation au noeud pere si diminution
			// detectable par un arbre desormais equilibre
			if (parentNode != rootNode and parentNode->GetBalance() == 0)
			{
				check(parentNode->GetParentNode());
				if (parentNode->GetParentNode()->GetLeftNode() == parentNode)
					AVLNodeLeftBalance(parentNode->GetParentNode(), false);
				else
					AVLNodeRightBalance(parentNode->GetParentNode(), false);
			}
		}
	}
	// Si le SAG a grandi
	else
	{
		// Si SAD max: amelioration de l'equilibre
		if (node->GetBalance() == 1)
			node->SetBalance(0);
		// Si noeud equilibre: le SAG devient max
		else if (node->GetBalance() == 0)
		{
			node->SetBalance(-1);

			// Propagation au noeud pere
			if (node != rootNode)
			{
				check(node->GetParentNode());
				if (node->GetParentNode()->GetLeftNode() == node)
					AVLNodeLeftBalance(node->GetParentNode(), true);
				else
					AVLNodeRightBalance(node->GetParentNode(), true);
			}
		}
		// Si SAG max: il faut reequilibrer l'arbre
		else
		{
			AVLNode* leftNode;

			// Acces au noeud gauche
			leftNode = node->GetLeftNode();
			check(leftNode);

			// Cas gauche-gauche: le depassement est du
			// a un SAG max du fils gauche
			if (leftNode->GetBalance() == -1 or leftNode->GetBalance() == 0)
			{
				// Rotation vers la droite des trois noeuds
				// leftNode->leftNode, leftNode, node
				check(leftNode->GetLeftNode());

				// Rattachement au pere de la nouvelle
				// racine locale: leftNode
				leftNode->SetParentNode(node->GetParentNode());
				if (rootNode == node)
					rootNode = leftNode;
				else
				{
					if (node->GetParentNode()->GetLeftNode() == node)
						node->GetParentNode()->SetLeftNode(leftNode);
					else
						node->GetParentNode()->SetRightNode(leftNode);
				}

				// Nouveaux chainages
				node->SetLeftNode(leftNode->GetRightNode());
				if (node->GetLeftNode() != NULL)
					node->GetLeftNode()->SetParentNode(node);
				//
				leftNode->SetRightNode(node);
				node->SetParentNode(leftNode);
				//
				if (leftNode->GetBalance() == -1)
				{
					node->SetBalance(0);
					leftNode->SetBalance(0);
				}
				else
				{
					node->SetBalance(-1);
					leftNode->SetBalance(1);
				}
			}
			// Cas gauche-droite: le depassement est du
			// a un SAD max du fils gauche
			else
			{
				AVLNode* leftRightNode;

				// Reorganisation plus complexe: voir
				// documentation sur l'algorithmique AVL

				// Acces au noeud droite du noeud gauche
				leftRightNode = leftNode->GetRightNode();
				check(leftRightNode);

				// Rattachement au pere de la nouvelle
				// racine locale: leftRightNode
				leftRightNode->SetParentNode(node->GetParentNode());
				if (rootNode == node)
					rootNode = leftRightNode;
				else
				{
					if (node->GetParentNode()->GetLeftNode() == node)
						node->GetParentNode()->SetLeftNode(leftRightNode);
					else
						node->GetParentNode()->SetRightNode(leftRightNode);
				}

				// Nouveaux chainages
				leftNode->SetRightNode(leftRightNode->GetLeftNode());
				if (leftNode->GetRightNode() != NULL)
					leftNode->GetRightNode()->SetParentNode(leftNode);
				//
				leftRightNode->SetLeftNode(leftNode);
				leftNode->SetParentNode(leftRightNode);
				//
				node->SetLeftNode(leftRightNode->GetRightNode());
				if (node->GetLeftNode() != NULL)
					node->GetLeftNode()->SetParentNode(node);
				//
				leftRightNode->SetRightNode(node);
				node->SetParentNode(leftRightNode);

				// Mise a jour de la balance
				if (leftRightNode->GetBalance() == 0)
				{
					node->SetBalance(0);
					leftNode->SetBalance(0);
				}
				else
				{
					if (leftRightNode->GetBalance() == -1)
					{
						node->SetBalance(1);
						leftNode->SetBalance(0);
					}
					else
					{
						node->SetBalance(0);
						leftNode->SetBalance(-1);
					}
					leftRightNode->SetBalance(0);
				}
			}
		}
	}
}

void SortedList::AVLNodeRightBalance(AVLNode* node, boolean bGrow)
{
	require(node != NULL);

	// Si le SAD a diminue: equivalent augmentation SAG
	if (not bGrow)
	{
		// Si SAD max: amelioration de l'equilibre
		if (node->GetBalance() == 1)
		{
			node->SetBalance(0);

			// Propagation au noeud pere
			if (node != rootNode)
			{
				check(node->GetParentNode());
				if (node->GetParentNode()->GetLeftNode() == node)
					AVLNodeLeftBalance(node->GetParentNode(), false);
				else
					AVLNodeRightBalance(node->GetParentNode(), false);
			}
		}
		// Si noeud equilibre: le SAG devient max
		else if (node->GetBalance() == 0)
			node->SetBalance(-1);
		// Si SAG max: il faut reequilibrer l'arbre
		else
		{
			AVLNode* parentNode;

			AVLNodeLeftBalance(node, true);

			// Le noeud issu du reequilibrage est le noeud pere
			parentNode = node->GetParentNode();
			check(parentNode);

			// Propagation au noeud pere si diminution
			// detectable par un arbre desormais equilibre
			if (parentNode != rootNode and parentNode->GetBalance() == 0)
			{
				check(parentNode->GetParentNode());
				if (parentNode->GetParentNode()->GetLeftNode() == parentNode)
					AVLNodeLeftBalance(parentNode->GetParentNode(), false);
				else
					AVLNodeRightBalance(parentNode->GetParentNode(), false);
			}
		}
	}
	// Si le SAD a grandi
	else
	{
		// Si SAG max: amelioration de l'equilibre
		if (node->GetBalance() == -1)
			node->SetBalance(0);
		// Si noeud equilibre: le SAD devient max
		else if (node->GetBalance() == 0)
		{
			node->SetBalance(1);

			// Propagation au noeud pere
			if (node != rootNode)
			{
				check(node->GetParentNode());
				if (node->GetParentNode()->GetLeftNode() == node)
					AVLNodeLeftBalance(node->GetParentNode(), true);
				else
					AVLNodeRightBalance(node->GetParentNode(), true);
			}
		}
		// Si SAD max: il faut reequilibrer l'arbre
		else
		{
			AVLNode* rightNode;

			// Acces au noeud droite
			rightNode = node->GetRightNode();
			check(rightNode);

			// Cas droite-droite: le depassement est du
			// a un SAD max du fils droite
			if (rightNode->GetBalance() == 1 or rightNode->GetBalance() == 0)
			{
				// Rotation vers la gauche des trois noeuds
				// node, rightNode, rightNode->rightNode
				check(rightNode->GetRightNode());

				// Rattachement au pere de la nouvelle
				// racine locale: rightNode
				rightNode->SetParentNode(node->GetParentNode());
				if (rootNode == node)
					rootNode = rightNode;
				else
				{
					if (node->GetParentNode()->GetLeftNode() == node)
						node->GetParentNode()->SetLeftNode(rightNode);
					else
						node->GetParentNode()->SetRightNode(rightNode);
				}

				// Nouveaux chainages
				node->SetRightNode(rightNode->GetLeftNode());
				if (node->GetRightNode() != NULL)
					node->GetRightNode()->SetParentNode(node);
				//
				rightNode->SetLeftNode(node);
				node->SetParentNode(rightNode);
				//
				if (rightNode->GetBalance() == 1)
				{
					node->SetBalance(0);
					rightNode->SetBalance(0);
				}
				else
				{
					node->SetBalance(1);
					rightNode->SetBalance(-1);
				}
			}
			// Cas droite-gauche: le depassement est du
			// a un SAG max du fils droite
			else
			{
				AVLNode* rightLeftNode;

				// Reorganisation plus complexe: voir
				// documentation sur l'algorithmique AVL

				// Acces au noeud gauche du noeud droite
				rightLeftNode = rightNode->GetLeftNode();
				check(rightLeftNode);

				// Rattachement au pere de la nouvelle
				// racine locale: rightLeftNode
				rightLeftNode->SetParentNode(node->GetParentNode());
				if (rootNode == node)
					rootNode = rightLeftNode;
				else
				{
					if (node->GetParentNode()->GetLeftNode() == node)
						node->GetParentNode()->SetLeftNode(rightLeftNode);
					else
						node->GetParentNode()->SetRightNode(rightLeftNode);
				}

				// Nouveaux chainages
				rightNode->SetLeftNode(rightLeftNode->GetRightNode());
				if (rightNode->GetLeftNode() != NULL)
					rightNode->GetLeftNode()->SetParentNode(rightNode);
				//
				rightLeftNode->SetRightNode(rightNode);
				rightNode->SetParentNode(rightLeftNode);
				//
				node->SetRightNode(rightLeftNode->GetLeftNode());
				if (node->GetRightNode() != NULL)
					node->GetRightNode()->SetParentNode(node);
				//
				rightLeftNode->SetLeftNode(node);
				node->SetParentNode(rightLeftNode);

				// Mise a jour de la balance
				if (rightLeftNode->GetBalance() == 0)
				{
					node->SetBalance(0);
					rightNode->SetBalance(0);
				}
				else
				{
					if (rightLeftNode->GetBalance() == 1)
					{
						node->SetBalance(-1);
						rightNode->SetBalance(0);
					}
					else
					{
						node->SetBalance(0);
						rightNode->SetBalance(1);
					}
					rightLeftNode->SetBalance(0);
				}
			}
		}
	}
}

void SortedList::Test()
{
	SortedList slTest;
	SampleObject* soElement;
	SampleObject searchObject;
	int i;
	int nTie;
	int nIndex;
	POSITION position;
	int nNbElements;
	int nElementTieNumber;
	int nInspectionNumber;
	ObjectArray oaElements;
	ObjectArray oaAllElements;
	ObjectArray oaElementListPositions;
	int nStartClock;
	int nStopClock;
	double dSeconds;
	int nNbError;

	// Nombre d'elements
	nNbElements = AcquireRangedInt("Nombre d'elements differents", 0, 1000000, 1000);
	nElementTieNumber = AcquireRangedInt("Nombre de doublons par element", 1, 1000, 10);
	nInspectionNumber = AcquireRangedInt("Nombre de parcours de listes", 1, 1000, 1000);

	// Creation des elements
	cout << "Creation des elements" << endl;
	oaElements.SetSize(nNbElements);
	oaAllElements.SetSize(nNbElements * nElementTieNumber);
	for (i = 0; i < nNbElements; i++)
	{
		soElement = new SampleObject();
		soElement->SetInt(i);
		oaElements.SetAt(i, soElement);

		// Memorisation des doublons
		for (nTie = 0; nTie < nElementTieNumber; nTie++)
			oaAllElements.SetAt(i * nElementTieNumber + nTie, soElement);
	}
	oaAllElements.Shuffle();

	// Initialisation de la liste a partir d'un tableau
	cout << "Initialisation de la liste a partir d'un tableau" << endl;
	nStartClock = clock();
	oaAllElements.SetCompareFunction(SampleObjectCompare);
	slTest.SetCompareFunction(SampleObjectCompare);
	oaAllElements.ExportSortedList(&slTest);
	nStopClock = clock();
	dSeconds = (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC;
	cout << "Depth=" << slTest.rootNode->GetMaxDepth() << endl;
	cout << "SYS TIME\tSortedList from ObjectArray\t" << SecondsToString(dSeconds) << endl;

	// Verification de la validite du tri
	cout << "Verification de la validite du tri" << endl;
	nStartClock = clock();
	position = slTest.GetHeadPosition();
	nNbError = 0;
	for (i = 0; i < nNbElements; i++)
	{
		// Parcours des doublons
		for (nTie = 0; nTie < nElementTieNumber; nTie++)
		{
			soElement = cast(SampleObject*, slTest.GetNext(position));
			if (soElement->GetInt() != i)
			{
				nNbError++;
				if (nNbError < 20)
					cout << "Error en (" << i << ", " << nTie << "):\t" << soElement->GetInt()
					     << "\n";
			}
		}
	}
	nStopClock = clock();
	dSeconds = (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC;
	cout << "SYS TIME\tSortedList check\t" << SecondsToString(dSeconds) << endl;

	// Parcours de la liste dans les deux sens
	cout << "Parcours de la liste dans les deux sens" << endl;
	nStartClock = clock();
	for (i = 0; i < nInspectionNumber; i++)
	{
		position = slTest.GetHeadPosition();
		while (position != NULL)
			slTest.GetNext(position);
		position = slTest.GetTailPosition();
		while (position != NULL)
			slTest.GetPrev(position);
	}
	nStopClock = clock();
	dSeconds = (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC;
	cout << "SYS TIME\tSorted list access\t" << SecondsToString(dSeconds) << endl;

	// Parcours de la liste dans les deux sens sans les doublons
	cout << "Parcours de la liste dans les deux sens sans les doublons" << endl;
	nStartClock = clock();
	for (i = 0; i < nInspectionNumber; i++)
	{
		nIndex = 0;
		position = slTest.GetHeadPosition();
		while (position != NULL)
		{
			nIndex++;
			searchObject.SetInt(nIndex);
			position = slTest.FindGreater(&searchObject);
			assert(nIndex == nNbElements or
			       cast(SampleObject*, slTest.GetAt(position))->GetInt() == nIndex);
		}
		nIndex = nNbElements - 1;
		position = slTest.GetTailPosition();
		while (position != NULL)
		{
			nIndex--;
			searchObject.SetInt(nIndex);
			position = slTest.FindLower(&searchObject);
			assert(nIndex == -1 or cast(SampleObject*, slTest.GetAt(position))->GetInt() == nIndex);
		}
	}
	nStopClock = clock();
	dSeconds = (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC;
	cout << "SYS TIME\tSortedList access without ties\t" << SecondsToString(dSeconds) << endl;

	// Supresssion des elements a partir des extremites
	cout << "Supresssion des elements a partir des extremites" << endl;
	nStartClock = clock();
	while (not slTest.IsEmpty())
	{
		slTest.RemoveHead();
		if (not slTest.IsEmpty())
		{
			slTest.RemoveTail();
		}
	}
	nStopClock = clock();
	dSeconds = (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC;
	cout << "SYS TIME\tSortedList remove\t" << SecondsToString(dSeconds) << endl;

	// Ajout de tous les elements un a un
	cout << "Ajout de tous les elements un a un" << endl;
	// oaAllElements.Shuffle();
	nStartClock = clock();
	for (i = 0; i < oaAllElements.GetSize(); i++)
	{
		slTest.Add(oaAllElements.GetAt(i));
	}
	nStopClock = clock();
	dSeconds = (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC;
	cout << "Depth=" << slTest.rootNode->GetMaxDepth() << endl;
	cout << "SYS TIME\tSortedList add\t" << SecondsToString(dSeconds) << endl;

	// Verification de la validite du tri
	cout << "Verification de la validite du tri" << endl;
	nStartClock = clock();
	position = slTest.GetHeadPosition();
	nNbError = 0;
	for (i = 0; i < nNbElements; i++)
	{
		// Parcours des doublons
		for (nTie = 0; nTie < nElementTieNumber; nTie++)
		{
			soElement = cast(SampleObject*, slTest.GetNext(position));
			if (soElement->GetInt() != i)
			{
				nNbError++;
				if (nNbError < 20)
					cout << "Error en (" << i << ", " << nTie << "):\t" << soElement->GetInt()
					     << "\n";
			}
		}
	}
	nStopClock = clock();
	dSeconds = (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC;
	cout << "SYS TIME\tSortedList check\t" << SecondsToString(dSeconds) << endl;

	// Suppression des elements en ordre aleatoire
	cout << "Suppression des elements en ordre aleatoire" << endl;
	// Recherche des positions
	oaElementListPositions.SetSize(oaAllElements.GetSize());
	position = slTest.GetHeadPosition();
	i = 0;
	while (position != NULL)
	{
		oaElementListPositions.SetAt(i, (Object*)position);
		slTest.GetNext(position);
		i++;
	}
	oaElementListPositions.Shuffle();
	// Supression de tous les elements
	for (i = 0; i < oaElementListPositions.GetSize(); i++)
	{
		position = oaElementListPositions.GetAt(i);
		slTest.RemoveAt(position);
	}
	assert(slTest.GetCount() == 0);
	nStopClock = clock();
	dSeconds = (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC;
	cout << "SYS TIME\tSortedList remove\t" << SecondsToString(dSeconds) << endl;

	// Destruction des elements
	oaElements.DeleteAll();
}

void SortedList::DeleteAllFreeNodes()
{
	AVLNode* firstFreeNode;

	// Parcours des noeuds libres existant pour les detruire
	while (nFreeNodeNumber > 0)
	{
		firstFreeNode = freeNodes;
		assert(firstFreeNode != NULL);
		freeNodes = freeNodes->GetParentNode();
		nFreeNodeNumber--;
		delete firstFreeNode;
	}
	ensure(nFreeNodeNumber == 0);
	ensure(freeNodes == NULL);
}

//////////////////////////////////////////////////////////////////
// Classe AVLNode

AVLNode* AVLNode::GetHead()
{
	AVLNode* node;

	node = this;
	while (node->leftNode != NULL)
		node = node->leftNode;
	return node;
}

AVLNode* AVLNode::GetTail()
{
	AVLNode* node;

	node = this;
	while (node->rightNode != NULL)
		node = node->rightNode;
	return node;
}

AVLNode* AVLNode::GetPrev()
{
	// Si arbre gauche non vide: dernier noeud de l'arbre gauche
	if (leftNode != NULL)
		return leftNode->GetTail();
	// Si pas de parent, on retourne NULL
	else if (parentNode == NULL)
		return NULL;
	// Sinon, il faut remonter les parents
	// jusqu'a une remontee par la droite
	else
	{
		AVLNode* node;

		// On remonte les parents tant qu'on arrive par la gauche
		node = this;
		while (node->parentNode != NULL and node->parentNode->leftNode == node)
			node = node->parentNode;

		// On rend le pere
		assert(node->parentNode == NULL or node->parentNode->rightNode == node);
		return node->parentNode;
	}
}

AVLNode* AVLNode::GetNext()
{
	// Si arbre droite non vide: premier noeud de l'arbre droite
	if (rightNode != NULL)
		return rightNode->GetHead();
	// Si pas de parent, on retourne NULL
	else if (parentNode == NULL)
		return NULL;
	// Sinon, il faut remonter les parents
	// jusqu'a une remontee par la droite
	else
	{
		AVLNode* node;

		// On remonte les parents tant qu'on arrive par la droite
		node = this;
		while (node->parentNode != NULL and node->parentNode->rightNode == node)
			node = node->parentNode;

		// On rend le pere
		assert(node->parentNode == NULL or node->parentNode->leftNode == node);
		return node->parentNode;
	}
}

int AVLNode::GetMaxDepth() const
{
	int nLeftDepth;
	int nRightDepth;

	// Hauteur a gauche
	nLeftDepth = 0;
	if (leftNode != NULL)
		nLeftDepth = 1 + leftNode->GetMaxDepth();

	// Hauteur a droite
	nRightDepth = 0;
	if (rightNode != NULL)
		nRightDepth = 1 + rightNode->GetMaxDepth();

	// Hauteur max
	if (nLeftDepth >= nRightDepth)
		return nLeftDepth;
	else
		return nRightDepth;
}

void AVLNode::DeleteSubTrees()
{
	if (leftNode != NULL)
	{
		leftNode->DeleteSubTrees();
		delete leftNode;
		leftNode = NULL;
	}
	if (rightNode != NULL)
	{
		rightNode->DeleteSubTrees();
		delete rightNode;
		rightNode = NULL;
	}
}

void AVLNode::Write(ostream& ost) const
{
	PrettyWrite(ost, "", 5);
}

void AVLNode::PrettyWrite(ostream& ost, const ALString sIndent, int nMaxSize) const
{
	require(nMaxSize >= 0);
	ALString sNewIndent;

	// Impression complete si taille max non atteinte
	if (nMaxSize == 0)
	{
		// Si arbre terminal: on affiche uniquement la
		// valeur des doublons
		if (leftNode == NULL and rightNode == NULL)
		{
			ost << sIndent << "-";
			ost << *(((ListNode*)headPosition)->data) << "\n";
		}
		// Sinon, on signale la suite de l'arbre
		else
			ost << sIndent << "-..."
			    << "\n";
	}
	else
	{
		// Si arbre terminal: on affiche uniquement la
		// valeur des doublons
		if (leftNode == NULL and rightNode == NULL)
		{
			ost << sIndent << "-";
			ost << *(((ListNode*)headPosition)->data) << "\n";
		}
		// Sinon, on utilise le parenthesage
		else
		{
			// Sous arbre gauche
			sNewIndent = sIndent;
			if (parentNode != NULL and parentNode->GetLeftNode() == this)
			{
				if (sNewIndent.GetLength() >= 1)
					sNewIndent.SetAt(sNewIndent.GetLength() - 1, ' ');
			}
			if (leftNode != NULL)
			{
				if (nMaxSize == 0)
					ost << sIndent << "-..."
					    << "\n";
				else
					leftNode->PrettyWrite(ost, sNewIndent + " |", nMaxSize - 1);
			}
			else
				ost << sNewIndent << " |-."
				    << "\n";

			// Valeur courante
			ost << sIndent << "-|";
			if (nBalance == -1)
				ost << "<";
			else if (nBalance == 1)
				ost << ">";
			else
				ost << "=";
			ost << *(((ListNode*)headPosition)->data) << "\n";

			// Sous arbre droit
			sNewIndent = sIndent;
			if (parentNode != NULL and parentNode->GetRightNode() == this)
			{
				if (sNewIndent.GetLength() >= 1)
					sNewIndent.SetAt(sNewIndent.GetLength() - 1, ' ');
			}
			if (rightNode != NULL)
			{
				if (nMaxSize == 0)
					ost << sIndent << "-..."
					    << "\n";
				else
					rightNode->PrettyWrite(ost, sNewIndent + " |", nMaxSize - 1);
			}
			else
				ost << sNewIndent << " |-."
				    << "\n";
		}
	}
}
