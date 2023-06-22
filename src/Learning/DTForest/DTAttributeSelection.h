// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDataPreparationClass.h"
#include "DTGlobalTag.h"
#include "DTConfig.h"

class DTAttributeSelection;
class DTTreeAttribute;

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
	// DTAttributeSelection(const ObjectArray* oaAttributeStats);
	~DTAttributeSelection();

	// Index de la selection, dans la liste des selection a analyser
	void SetIndex(int nValue);
	int GetIndex() const;

	// Premier attribut de la paire
	void AddAttribute(DTTreeAttribute* attribute);
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

	// revoie une liste d attribut aleatoire de taille nMaxAttributesNumber
	ObjectArray* GetTreeAttributesShuffled(const int nMaxAttributesNumber);
	ObjectArray* GetAttributesShuffled(const int nMaxAttributesNumber);
	// revoie la selection des attribut
	const ObjectArray* GetTreeAttributeSelection() const;
	ObjectArray* GetAttributeSelection() const;
	// parcourt une liste d'attributs, effectue un tirage aleatoire en fonction du level de ces attributs, et
	// renvoie les 'nSelectedAttributesNumber' index de chargements des attributs de plus fort level
	// se base sur les levels des attributs repertories dans le classStats
	ObjectArray* GetTreeAttributesFromLevels(const int maxAttributesNumber);
	ObjectArray* GetAttributesFromLevels(const int maxAttributesNumber);

	static ObjectArray* SortObjectArrayFromContinuous(const int nMaxAttributesNumber, DoubleVector& vLevels,
							  ObjectArray& oaListAttributes);

	// Comparaison avec une autre paire, vis a vis de leurs blocs
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
	int CompareBlocks(const DTAttributeSelection* otherAttributePair);

	void SortByBlocks();

	int GetUsableAttributesNumber();
	void SetUsableAttributesNumber(int nmax);

	const ALString& GetDrawingType() const;
	void SetDrawingType(const ALString& sdrawingtype);

	// Ecriture
	void Write(ostream& ost) const override;

	// Graine initiale de la suite pseudo-aleatoire
	//(permet de reproduire la meme suite)
	void SetSeed(int nSeed);
	int GetSeed();

	// Verification de l'integrite
	// Les attributs doivent etre unique
	// Les attributs doivent etre dans l'ordre de leurs nom

	boolean Check() const override;
	boolean AreTreeAttributesSortedByBlock(const ObjectArray* oaAttributes) const;

	void WriteReport(ostream& ost);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// index index de la selection
	int nIndex;
	int nUsableAttributesNumber;
	int nRandomSeed;
	ALString sDrawingType;
	// list des DTTreeAttribute
	ObjectArray oaTreeAttributeSelection;
};

/////////////////////////////////////////////////////////////////////
/// Classe DTTreeAttribute
class DTTreeAttribute : public Object
{
public:
	DTTreeAttribute();
	void CopyFrom(const DTTreeAttribute*);
	DTTreeAttribute* Clone() const;
	const ALString& GetName() const;

	// attribut
	KWAttribute* aAttribute;

	// level et rang
	double dLevel;
	int nRank;
};

// Methode de comparaison sur les blocs (cf DTAttributeSelections::CompareBlocks)
int DTAttributeSelectionCompareBlocks(const void* elem1, const void* elem2);
int DTTreeAttributeCompareName(const void* elem1, const void* elem2);
int DTTreeAttributeCompareBlocks(const void* elem1, const void* elem2);
