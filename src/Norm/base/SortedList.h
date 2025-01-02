// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"

class AVLNode;

///////////////////////////////////////////////////////
// Classe SortedList
// Liste triee en permanence
// a tout moment, on peut acceder au premier element
// de la liste.
// L'implementation est log(n) pour les operation
// d'insertion, supression et recherche, en 1 pour
// les parcours et les acces par position
class SortedList : public Object
{
public:
	// Constructeur
	SortedList();
	~SortedList();

	// Constructeur avec parametrage de la fonction de comparaison
	SortedList(CompareFunction fCompare);

	// Parametrage de la fonction de comparaison, uniquement si la liste est vide
	void SetCompareFunction(CompareFunction fCompare);

	// Acces a la fonction de comparaison
	CompareFunction GetCompareFunction() const;

	// Nombre d'elements
	int GetCount() const;
	boolean IsEmpty() const;

	//////////////////////////////////////////////////////////
	// Parcours de la liste
	// Apres un appel, la position contient la nouvelle position
	//
	// Exemple de parcours de liste
	//   position = myList->GetHeadPosition();
	//   while (position != NULL)
	//       myObject = myList->GetNext(position);

	// Initialisation de la position sur une extremite de la liste
	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;

	// Deplacement: on renvoie l'objet courant, et on deplace
	// la position
	// Si on force tiePosition a NULL en entree:
	//   on rend le premier(/dernier) doublon de la liste courante
	//   on se deplace d'une liste de doublons, vers le prochain
	//     premier(/dernier) doublon
	Object* GetNext(POSITION& position) const;
	Object* GetPrev(POSITION& position) const;

	// Recherche d'un element a une position (non NULL) donnee
	Object* GetAt(POSITION position) const;

	/////////////////////////////////////////////////////////
	// Gestion de la liste

	// Recherche du debut (ou fin) de la liste
	// (sur les listes non vides)
	Object* GetHead() const;
	Object* GetTail() const;

	// Supression du premier (ou dernier) element (qui est renvoye)
	Object* RemoveHead();
	Object* RemoveTail();

	// Ajout d'un element
	// Sa position dans la liste est renvoyee
	POSITION Add(Object* object);

	// Supression d'un element a une position donnee
	void RemoveAt(POSITION position);

	// Supression de tous les elements
	void RemoveAll();
	void DeleteAll(); // Destruction des elements eux-meme

	////////////////////////////////////////////////
	// Recherche de la position d'elements (en O(log(n))
	// On retourne NULL en cas d'echec

	// Recherche du premier element d'une valeur donnee
	POSITION Find(const Object* searchValue) const;

	// Recherche du premier element
	// superieur ou egal a une valeur
	POSITION FindGreater(const Object* searchValue) const;

	// Recherche du dernier element
	// inferieur ou egal a une valeur
	POSITION FindLower(const Object* searchValue) const;

	////////////////////////////////////////////////
	// Services divers

	// Copie du contenu d'une liste source
	void CopyFrom(const SortedList* slSource);

	// Clone: alloue et retourne le Clone
	SortedList* Clone() const;

	// Conversions vers les autres containers
	// Memoire: le contenu precedent du container resultat n'est plus reference (mais pas detruit)
	void ExportObjectArray(ObjectArray* oaResult) const;
	void ExportObjectList(ObjectList* olResult) const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Estimation de la memoire utilisee avec prise en compte des objet contenus
	longint GetOverallUsedMemory() const;

	// Estimation de la memoire utilisee par element, pour le dimensionnement a prior des containers
	longint GetUsedMemoryPerElement() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Methode de test de la classe
	static void Test();

	////////////////////////////////////////////////////
	///// Implementation
protected:
	// Ajout d'un element a un noeud de l'arbre binaire
	POSITION AVLNodeAdd(AVLNode* node, Object* object);

	// Supression d'un element a un noeud de l'arbre binaire
	void AVLNodeRemoveAt(AVLNode* node, POSITION position);

	// Procedure de reequilibrage de l'arbre, suite a une
	// modification de la profondeur du sous arbre gauche
	// Propagation vers la racine
	void AVLNodeLeftBalance(AVLNode* node, boolean bGrow);

	// Procedure de reequilibrage de l'arbre, suite a une
	// modification de la profondeur du sous arbre droite
	// Propagation vers la racine
	void AVLNodeRightBalance(AVLNode* node, boolean bGrow);

	// Creation/ destruction d'un noeud de liste triees
	AVLNode* NewNode();
	void FreeNode(AVLNode* pNode);

	// Destruction de tous les noeuds de la liste des noeuds libres
	void DeleteAllFreeNodes();

	// Liste triee
	// Chaque element de la liste contient une liste
	// d'arcs agreges constituant des doublons
	// On memorise la racine de l'arbre, et ses extremites
	AVLNode* rootNode;
	AVLNode* headNode;
	AVLNode* tailNode;
	ObjectList olSortedElements;
	CompareFunction fCompareFunction;

	// Gestion d'une liste de AVLNode libres pour optimiser les allocations/desallocations
	// Cela permet d'envisager des mises a jour de listes triee en evitant de passer par l'allocateur
	AVLNode* freeNodes;
	int nFreeNodeNumber;
	const int nMaxFreeNodeNumber = 5;
};

//////////////////////////////////////////////////
// Classe AVLNode
// Noeud d'un arbre binaire equilibre
// Classe interne utilisee pour l'implementation
// efficace de la listre triee (modifications en log(n)
class AVLNode : public Object
{
public:
	// Constructeur
	AVLNode();
	~AVLNode();

	// Initialisation
	void Initialize();

	// Premier doublons porte par le noeud
	void SetTieHeadPosition(POSITION position);
	POSITION GetTieHeadPosition() const;

	// Facteur d'equilibre de l'arbre
	// Difference de profondeur entre le sous abre droit
	// et le sous arbre gauche)
	//  Si hauteur(SAG) < hauteur(SAD): Balance = +1
	//  Si hauteur(SAG) = hauteur(SAD): Balance =  0
	//  Si hauteur(SAG) > hauteur(SAD): Balance = -1
	void SetBalance(int nValue);
	int GetBalance() const;

	// Sous arbre gauche
	void SetLeftNode(AVLNode* node);
	AVLNode* GetLeftNode();

	// Sous arbre droit
	void SetRightNode(AVLNode* node);
	AVLNode* GetRightNode();

	// Noeud pere
	void SetParentNode(AVLNode* node);
	AVLNode* GetParentNode();

	//////////////////////////////////////////
	// Deplacement dans la liste ordonnee des
	// noeuds induite par l'arbre binaire

	// Acces au premier noeud (le plus a gauche)
	AVLNode* GetHead();

	// Acces au dernier noeuds (le plus a droite)
	AVLNode* GetTail();

	// Acces au noeud precedent
	AVLNode* GetPrev();

	// Acces au noeud suivant
	AVLNode* GetNext();

	/////////////////////////////////////////////
	// Divers services

	// Profondeur max a partir du noeud
	int GetMaxDepth() const;

	// Supression des sous arbre
	void DeleteSubTrees();

	// Impression complete sous forme d'une expression
	// parenthesee
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////
	///// Implementation
protected:
	// Impression complete sous forme d'une expression
	// parenthesee, avec parametrage de la presentation
	void PrettyWrite(ostream& ost, const ALString sIndent, int nMaxSize) const;

	// Attributs
	POSITION headPosition;
	int nBalance;
	AVLNode* leftNode;
	AVLNode* rightNode;
	AVLNode* parentNode;
};

////////////////////////////////////////////////////////
// Implementation en inline

inline int SortedList::GetCount() const
{
	return olSortedElements.GetCount();
}

inline boolean SortedList::IsEmpty() const
{
	return olSortedElements.IsEmpty();
}

inline CompareFunction SortedList::GetCompareFunction() const
{
	return fCompareFunction;
}

inline POSITION SortedList::GetHeadPosition() const
{
	require(fCompareFunction != NULL);
	return olSortedElements.GetHeadPosition();
}

inline POSITION SortedList::GetTailPosition() const
{
	require(fCompareFunction != NULL);
	return olSortedElements.GetTailPosition();
}

inline Object* SortedList::GetNext(POSITION& position) const
{
	require(fCompareFunction != NULL);
	require(position != NULL);
	return olSortedElements.GetNext(position);
}

inline Object* SortedList::GetPrev(POSITION& position) const
{
	require(fCompareFunction != NULL);
	require(position != NULL);
	return olSortedElements.GetPrev(position);
}

inline Object* SortedList::GetAt(POSITION position) const
{
	require(fCompareFunction != NULL);
	require(position != NULL);
	return olSortedElements.GetAt(position);
}

inline Object* SortedList::GetHead() const
{
	require(fCompareFunction != NULL);
	require(not IsEmpty());
	return olSortedElements.GetHead();
}

inline Object* SortedList::GetTail() const
{
	require(fCompareFunction != NULL);
	require(not IsEmpty());
	return olSortedElements.GetTail();
}

inline Object* SortedList::RemoveHead()
{
	Object* object;
	require(fCompareFunction != NULL);
	require(not IsEmpty());
	object = olSortedElements.GetHead();
	AVLNodeRemoveAt(headNode, olSortedElements.GetHeadPosition());
	return object;
}

inline Object* SortedList::RemoveTail()
{
	Object* object;
	require(fCompareFunction != NULL);
	require(not IsEmpty());
	object = olSortedElements.GetTail();
	AVLNodeRemoveAt(tailNode, olSortedElements.GetTailPosition());
	return object;
}

inline AVLNode* SortedList::NewNode()
{
	if (freeNodes != NULL)
	{
		AVLNode* newNode;
		newNode = freeNodes;
		freeNodes = freeNodes->GetParentNode();
		nFreeNodeNumber--;
		newNode->Initialize();
		return newNode;
	}
	else
		return new AVLNode;
}

inline void SortedList::FreeNode(AVLNode* pNode)
{
	require(pNode != NULL);
	if (nFreeNodeNumber < nMaxFreeNodeNumber)
	{
		pNode->SetParentNode(freeNodes);
		freeNodes = pNode;
		nFreeNodeNumber++;
	}
	else
		delete pNode;
}

//////////////////////////////////////////////////

inline AVLNode::AVLNode()
{
	Initialize();
}

inline AVLNode::~AVLNode() {}

inline void AVLNode::Initialize()
{
	nBalance = 0;
	headPosition = NULL;
	leftNode = NULL;
	rightNode = NULL;
	parentNode = NULL;
}

inline void AVLNode::SetTieHeadPosition(POSITION position)
{
	headPosition = position;
}

inline POSITION AVLNode::GetTieHeadPosition() const
{
	return headPosition;
}

inline void AVLNode::SetBalance(int nValue)
{
	require(-1 <= nBalance and nBalance <= 1);
	nBalance = nValue;
}

inline int AVLNode::GetBalance() const
{
	return nBalance;
}

inline void AVLNode::SetLeftNode(AVLNode* node)
{
	leftNode = node;
}

inline AVLNode* AVLNode::GetLeftNode()
{
	return leftNode;
}

inline void AVLNode::SetRightNode(AVLNode* node)
{
	rightNode = node;
}

inline AVLNode* AVLNode::GetRightNode()
{
	return rightNode;
}

inline void AVLNode::SetParentNode(AVLNode* node)
{
	parentNode = node;
}

inline AVLNode* AVLNode::GetParentNode()
{
	return parentNode;
}
