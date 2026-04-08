// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDataPreparationClass.h"

class DTAttributeSelection;
class DTTreeAttribute;

int DTTreeAttributeLevelCompare(const void* elem1, const void* elem2);
int DTTreeAttributeRankCompare(const void* elem1, const void* elem2);
int DTAttributeSelectionCompareAttributesNumber(const void* elem1, const void* elem2);
int DTAttributeSelectionCompareAttributesIndex(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////
// Classe DTAttributeSelection
// Specification d'une selection d'attributs
// Class interne pour l'implementation de l'analyse bivariee
// Memoire: le contenu des specifications appartient a l'appelant
class DTAttributeSelection : public Object
{
public:
	// Constructeur
	DTAttributeSelection();
	~DTAttributeSelection();

	// copie / clonage
	void CopyFrom(const DTAttributeSelection* source);
	DTAttributeSelection* Clone() const;

	// initialisation des membres KWAttribute des DTTreeAttribute contenus par la classe, a partir d'un dictionnaire
	void InitializeTreeAttributesFromClass(const KWClass*);

	// Index de la selection, dans la liste des selection a analyser
	void SetIndex(int nValue);
	int GetIndex() const;

	// Premier attribut de la paire
	void AddAttribute(DTTreeAttribute* attribute);

	// Acces aux attributs de la selection par position
	DTTreeAttribute* GetAttributeAt(int npos) const;
	const ALString& GetAttributeNameAt(int npos) const;
	int GetSize() const;

	// Acces aux blocs des deux attributs dans l'ordre des blocs
	// (donc pas forecemnt dans l'ordre des attributs)
	const KWAttributeBlock* GetFirstBlock() const;
	const KWAttributeBlock* GetSecondBlock() const;

	// Acces aux attributs du premier et du second bloc
	DTTreeAttribute* GetFirstBlockAttribute() const;
	DTTreeAttribute* GetSecondBlockAttribute() const;

	// Renvoie une selection aleatoire de taille <= nMaxAttributesNumber (tirage uniforme)
	ObjectArray* GetTreeAttributesShuffled(const int nMaxAttributesNumber);
	ObjectArray* GetAttributesShuffled(const int nMaxAttributesNumber);

	// Renvoie le tableau complet de la selection courante
	const ObjectArray* GetTreeAttributeSelection() const;
	ObjectArray* GetAttributeSelection() const;

	// Tirage aleatoire pondere par le level : renvoie au maximum nMaxAttributesNumber attributs
	// parmi ceux ayant le level le plus eleve, selon l'algorithme de Weighted Random Sampling
	//
	// l'algo de Weighted Random Sampling with a reservoir Information Processing Letters 97(2006) 181-185
	// Pavlos S. Efraimidis, Paul G. Spirakis Interet : en une passe, directement parallelisable, utilise les poids
	// non normalises, Complexite en O(K*log(Ks)) on pourrait potentiellement avoir une infinite de variables car on
	// maintient uniquement les Ks meilleures cles Dans l'article une version avec "reservoir sampling" est presente
	// egalement qui permet de selectionner dans un flux de variables Implementation :
	// - pour chaque variable, on tire u_k aleatoirement dans [0,1] et on calcule la cle c_k=u_k^(1/poids_k)
	// - on stocke les Ks meilleures cles dans une Sorted List
	// - a la fin on selectionne les indices de ces Ks meilleurs cles
	ObjectArray* GetTreeAttributesFromLevels(const int maxAttributesNumber);
	ObjectArray* GetAttributesFromLevels(const int maxAttributesNumber);

	// Trie un tableau d'attributs par tirage pondere sur les levels fournis et renvoie les nMaxAttributesNumber
	// meilleurs elements. Algorithme de reservoir pondere (Efraimidis & Spirakis, 2006).
	static ObjectArray* SortObjectArrayFromContinuous(const int nMaxAttributesNumber, const DoubleVector& dvLevels,
							  const ObjectArray& oaListAttributes);

	// Comparaison avec une autre selection, vis a vis de leurs blocs
	// Les selections sont comparees selon les criteres hierarchiques suivant
	//   . premier bloc (taille decroissante, puis nom)
	//   . second bloc (taille decroissante, puis nom)
	//   . attribut du second bloc
	//   . attribut du premier bloc
	// L'utilisation des trois premier criteres permet, apres tri, d'identifier
	// des groupes de selections de variables ayant meme premier bloc et meme attribut du
	// second groupe, en maximisant la taille du premier bloc.
	// On peut alors faire appel a la class KWTupleTableLoader en chargeant les
	// table de tuples par groupes pour beneficier de la represenattion sparse
	// des attribut, dans le cas de l'analyse bivariee
	// L'ensemble des 4 criteres assure la reproductibilite des traitement
	int CompareBlocks(const DTAttributeSelection* otherAttributePair) const;

	void SortByBlocks();

	int GetUsableAttributesNumber();
	void SetUsableAttributesNumber(int nmax);

	const ALString& GetDrawingType() const;
	void SetDrawingType(const ALString& sdrawingtype);

	// Ecriture
	void Write(ostream& ost) const override;

	// Graine initiale de la suite pseudo-aleatoire
	//(permet de reproduire la meme suite)
	void SetRandomSeed(int nSeed);
	int GetRandomSeed();

	// Verification de l'integrite
	// Les attributs doivent etre unique
	// Les attributs doivent etre dans l'ordre de leurs nom

	boolean Check() const override;
	boolean AreTreeAttributesSortedByBlock(const ObjectArray* oaAttributes) const;

	void WriteReport(ostream& ost) const;

	virtual longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	friend class PLShared_AttributeSelection;

	// index index de la selection
	int nIndex;
	int nUsableAttributesNumber;
	int nRandomSeed;
	ALString sDrawingType;
	// list des DTTreeAttribute
	ObjectArray oaTreeAttributeSelection;
};

/////////////////////////////////////////////////////////////////////
// Classe DTTreeAttribute
class DTTreeAttribute : public Object
{
public:
	DTTreeAttribute();
	void CopyFrom(const DTTreeAttribute*);
	DTTreeAttribute* Clone() const;
	const ALString& GetName() const;

	virtual longint GetUsedMemory() const;

	// attribut
	KWAttribute* aAttribute;

	ALString sAttributeName;

	// level et rang
	double dLevel;
	int nRank;
};

////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_TreeAttribute
// Serialisation de la classe DTTreeAttribute
class PLShared_TreeAttribute : public PLSharedObject
{
public:
	// Constructor
	PLShared_TreeAttribute();
	~PLShared_TreeAttribute();

	void SetTreeAttribute(DTTreeAttribute*);

	DTTreeAttribute* GetTreeAttribute() const;

	void DeserializeObject(PLSerializer* serializer, Object* object) const override;

	void SerializeObject(PLSerializer* serializer, const Object* object) const override;

protected:
	Object* Create() const override;
};
////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_AttributeSelection
// Serialisation de la classe DTAttributeSelection
class PLShared_AttributeSelection : public PLSharedObject
{
public:
	// Constructor
	PLShared_AttributeSelection();
	~PLShared_AttributeSelection();

	void SetAttributeSelection(DTAttributeSelection*);

	DTAttributeSelection* GetAttributeSelection() const;

	void DeserializeObject(PLSerializer* serializer, Object* object) const override;

	void SerializeObject(PLSerializer* serializer, const Object* object) const override;

protected:
	Object* Create() const override;

	PLShared_ObjectArray* shared_oaTreeAttributeSelection;
};

// Methode de comparaison sur les blocs (cf DTAttributeSelections::CompareBlocks)
int DTAttributeSelectionCompareBlocks(const void* elem1, const void* elem2);
int DTTreeAttributeCompareName(const void* elem1, const void* elem2);
int DTTreeAttributeCompareBlocks(const void* elem1, const void* elem2);
