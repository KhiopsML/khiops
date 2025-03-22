// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGridMerger;
class KWDGMAttribute;
class KWDGMPart;
class KWDGMCell;
class KWDGMPartMerge;

#include "KWDataGrid.h"
#include "KWDataGridCosts.h"
#include "KWDataGridManager.h"
#include "KWStat.h"
#include "KWDGMPartMergeAction.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridMerger
// Sous-classe de KWDataGrid permettant de gerer une valuation du DataGrid,
// additive sur l'ensemble de ses entites (grille, attributs, parties, cellules).
// Chacune des entites concernees est egalement redefinie pour specialiser
// le DataGrid value, et permettre une optimisation de la valuation par fusions
// des parties sur chaque attribut.
class KWDataGridMerger : public KWDataGrid
{
public:
	// Constructeur
	KWDataGridMerger();
	~KWDataGridMerger();

	// Parametrage de la structure des couts de la grille de donnees
	// Memoire: les specifications sont referencees et destinees a etre partagees par plusieurs algorithmes
	void SetDataGridCosts(const KWDataGridCosts* kwdgcCosts);
	const KWDataGridCosts* GetDataGridCosts() const;

	// Nombre maximum de parties par dimension (defaut: 0, signifie pas de contrainte)
	int GetMaxPartNumber() const;
	void SetMaxPartNumber(int nValue);

	// Optimisation du groupage du DataGrid
	// Renvoie le cout optimise
	double Merge();

	// Cout local du DataGrid
	void SetCost(double dValue);
	double GetCost() const;

	// Initialisation des couts des entites du DataGrid
	// (DataGrid, Attributs, Parties, Cellules)
	// Methode de type utilitaire, permettant de memoriser les couts par entite d'une grille,
	// et d'y avoir ensuite acces par les methodes GetCost
	void InitializeAllCosts();

	// Verification de tous les cout
	boolean CheckAllCosts() const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////
	//// Implementation
protected:
	friend class KWDGMPartMergeAction;
	friend class CCCoclusteringBuilder;

	// Reimplementation des methodes virtuelles
	KWDGAttribute* NewAttribute() const override;
	KWDGCell* NewCell() const override;

	//////////////////////////////////////////////////////////////////
	// Algorithme de fusion des parties
	// Les donnees de travail sont portees par les entites du DataGrid,
	// mais l'essentiel des methodes est centralise ci-dessous

	// Pilotage de l'algorithme de fusion des parties
	// Renvoie le cout optimise
	double OptimizeMerge();

	// Initialisation de toutes les listes de parties triees par nombre de modalites
	void InitializeAllPartLists();

	// Suppression de toutes les listes de parties triees par nombre de modalites
	void RemoveAllPartLists();

	// Initialisation de toutes les fusions de parties
	// Les fusions sont evaluees, chainees dans les deux sens avec les parties
	// et triees dans les attributs
	void InitializeAllPartMerges();

	// Destruction de toute les fusions et nettoyage de la structure
	void DeleteAllPartMerges();

	// Affichage de toutes les fusions de parties
	void WriteAllPartMerges(ostream& ost) const;

	// Verification de toutes les fusions de parties
	boolean CheckAllPartMerges() const;

	// Recherche de la meilleure fusion
	// (parmi toutes les fusions de parties de tous les attributs)
	// Renvoie le DeltaCost de la fusion, negatif s'il y a amelioration, ainsi
	// que la meilleure fusion correspondant (en parametre par reference).
	// Renvoie DBL_MAX (et meilleure fusion a NULL) s'il n'y a plus de fusion possible
	double SearchBestPartMerge(KWDGMPartMerge*& bestPartMerge);

	// Recherche de la meilleure fusion
	// (parmi toutes les fusions de parties de tous les attributs)
	// Chaque fusion est consideree dans le cadre d'une partition avec et sans poubelle
	// Le DeltaCost tient compte de la variation de cout de partition qui est envisagee avec et sans poubelle
	// Renvoie le DeltaCost de la fusion, negatif s'il y a amelioration, ainsi
	// que la meilleure fusion correspondant (en parametre par reference).
	// Renvoie DBL_MAX (et meilleure fusion a NULL) s'il n'y a plus de fusion possible
	double SearchBestPartMergeWithGarbageSearch(KWDGMPartMerge*& bestPartMerge);

	// Calcul du cout de merge de deux parties
	double ComputeMergeCost(const KWDGMPartMerge* partMerge) const;

	// Calcul de l'impact sur toutes les parties sources d'une grille de la decrementation
	// du nombre de parties cibles
	double ComputeAllPartsTargetDeltaCost() const;

	// Realisation d'une fusion
	// Mise a jour des structures du DataGrid, de la table hash des cellules
	// Prise en compte de la structure des fusions en evaluant les fusions impactees
	// Destruction de la fusion passee en parametre
	// Retourne la partie resultat de la fusion
	KWDGMPart* PerformPartMerge(KWDGMPartMerge* bestPartMerge);

	//////////////////////////////////////////////////////////////////
	// Gestion d'une table de hash dediee aux cellules
	// Pour K attributs et en moyenne N parties par attributs, il y a
	// potentiellement N^K cellules, fournissant naturellement une cle
	// unique identifiant chaque cellule:
	//  Key(Cell)=somme(key(Part_k)*N^k)
	// On cherche un nombre premier P superieur a N, donc superieur
	// au nombre effectif de cellules.
	// Une cle de hashage simple est de prendre le modulo (Hash(i) = i mod P,
	// ce qui dans le cas des cellules permet un calcul simple:
	//  Hash(Cell)=Hash(somme(Hash(key(Part_k))*Hash(N^k))
	// Il suffit alors de memoriser pour chaque attribut, partie et cellule
	// sa valeur de Hash pour permettre de recherche des cellule en O(1) quand
	// on modifie une de leur partie (pour tester si deux cellules doivent etre
	// fusionnes lors d'une fusion de parties)
	// Une deuxieme cle de hash, independant de la premiere, est egalement geree
	// pour discriminer facilement les difference de cellules entrant en collision.

	// Initialisation du dictionnaire des cellules
	// Toutes les valeurs de hash des attributs, parties et cellules sont initialises
	// et la table de hashage est remplie avec tous les cellules du DataGrid
	void CellDictionaryInit();

	// Test d'egalite de la cle de deux cellules (cle: ensemble des parties)
	boolean CellDictionarySameCellKeys(KWDGMCell* cell1, KWDGMCell* cell2) const;

	// Recherche d'une cellule ayant meme cle que la cellule en parametre, exepte
	// une composante partie sur un attribut
	// Retourne NULL si non trouve
	KWDGMCell* CellDictionaryLookupModifiedCell(KWDGMCell* cell, KWDGMPart* modifiedPart) const;

	// Recherche d'une cellule ayant meme cle que la cellule en parametre, exepte
	// deux composantes parties sur deux attributs
	// Retourne NULL si non trouve
	KWDGMCell* CellDictionaryLookupTwiceModifiedCell(KWDGMCell* cell, KWDGMPart* modifiedPart1,
							 KWDGMPart* modifiedPart2) const;

	// Transfert d'une cellule dont une composante partie sur un attribut a ete modifiee
	// Le transfer ne se fait que dans le dictionnaire, la cellule initial restant
	// inchange.
	// Le cellule doit exister dans sa version initiale, et etre absent
	// du dictionnaire dans sa version modifiee
	void CellDictionaryTransferModifiedCell(KWDGMCell* cell, KWDGMPart* modifiedPart);

	// Ajout d'une cellule dans le dictionnaire 'n initialisant ses cles de hash)
	// Le cellule doit ne doit exister
	void CellDictionaryAddCell(KWDGMCell* cell);

	// Supression d'une cellule du dictionnaire
	// Le cellule doit exister
	void CellDictionaryRemoveCell(KWDGMCell* cell);

	// Supression de toutes les cellules du dictionnaire
	void CellDictionaryRemoveAll();

	// Affichage de la table de hash
	void CellDictionaryWrite(ostream& ost) const;

	// Calcul du modulo du produit de deux entiers
	// (probleme si depassement de capacite des entiers lors du produit)
	int ComputeProductModulo(int nFactor1, int nFactor2, int nModuloRange) const;

	// Contrainte sur le nombre max de partie par dimension
	int nMaxPartNumber;

	// Table de hashage des cellules, geree au moyen d'un tableau, et de l'attribut
	// hashNextCell des cellules pour gerer les collision
	ObjectArray oaCellDictionary;
	int nCellDictionaryCount;

	// Cout
	double dCost;

	// Parametrage de la structure des couts
	const KWDataGridCosts* dataGridCosts;

	// Impact sur toutes les parties sources d'une grille de la decrementation du nombre de partie cibles
	double dAllPartsTargetDeltaCost;

	// Epsilon de precision numerique
	double dEpsilon;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMAttribute
// Attribut d'un DataGridMerger
class KWDGMAttribute : public KWDGAttribute
{
public:
	// Constructeur
	KWDGMAttribute();
	~KWDGMAttribute();

	// Cout local de l'attribut
	void SetCost(double dValue);
	double GetCost() const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDataGridMerger;
	friend class KWDGMPartMergeAction;
	friend class CCCoclusteringBuilder;

	// Reimplementation des methodes virtuelles
	KWDGPart* NewPart() const override;

	/////////////////////////////////////////////////////////////////
	// Gestion des fusions de parties de l'attribut, classees par
	// cout de fusion

	// Ajout d'un fusion
	void AddPartMerge(KWDGMPartMerge* partMerge);

	// Recherche de la meilleure fusion
	// Retourne NULL si aucune
	KWDGMPartMerge* GetBestPartMerge();

	// Suppression (sans destruction) d'une fusion qui doit exister
	void RemovePartMerge(KWDGMPartMerge* partMerge);

	// Acces au tableau de toutes les fusions
	// Memoire: le tableau (pas son contenu) est a liberer par l'appelant
	ObjectArray* GetAllPartMerges();

	// Supression de toutes les fusions
	void RemoveAllPartMerges();

	// Destruction de toutes les fusions
	void DeleteAllPartMerges();

	// Liste de toutes les fusions de parties de l'attribut
	SortedList* slPartMerges;

	// Ajout d'une partie avec son nombre de modalites
	void AddPartToValueNumberList(KWDGMPart* partM);

	// Suppression (sans destruction) d'une partie qui doit exister
	void RemovePartFromValueNumberList(KWDGMPart* partM);

	// Supression de toutes les parties
	void RemoveAllPartsFromValueNumberList();

	// Liste des nombres de modalites pour chaque partie de l'attribut
	SortedList* slPartValueNumbers;

	// Cles de hashage, pour gerer la table de hash des cellules
	int nHash;
	int nHash2;

	// Cout
	double dCost;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMPart
// Partie d'un DataGridMerger
class KWDGMPart : public KWDGPart
{
public:
	// Constructeur
	KWDGMPart();
	~KWDGMPart();

	// Cout local de la partie
	void SetCost(double dValue);
	double GetCost() const;

	// Position dans une liste triee des nombres de modalites par partie (categoriel)
	void SetPosition(POSITION pos);
	POSITION GetPosition() const;

	// Controle d'integrite
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDataGridMerger;
	friend class KWDGMPartMergeAction;
	friend class KWDGMAttribute;
	friend class KWDGMPartMergeArray;
	friend class CCCoclusteringBuilder;

	//////////////////////////////////////////////////////////////
	// Gestion des fusions dans lesquelles la partie est impliquee
	// Les fusions sont accessible par cle de la partie extremite de la fusion

	// Ajout d'une fusion
	void AddPartMerge(KWDGMPartMerge* partMerge);

	// Recherche d'une fusion (avec comme cle l'autre partie extremite de la fusion
	KWDGMPartMerge* LookupPartMerge(KWDGMPart* oppositePart) const;

	// Suppression d'une fusion dont on precise la cle
	void RemovePartMerge(KWDGMPart* oppositePart);

	// Parcours de toutes les fusions
	//  position = GetStartPartMerge();
	//	while (position != NULL)
	//	{
	//		GetNextPartMerge(position, partMerge);
	//		cout << *partMerge << "\n";
	//	}
	POSITION GetStartPartMerge() const;
	void GetNextPartMerge(POSITION& positionPartMerge, KWDGMPartMerge*& partMerge) const;

	// Suppression de toutes les fusions
	void RemoveAllPartMerges();

	// Affichage de toutes les fusions
	void WriteAllPartMerges(ostream& ost) const;

	// Dictionnaire des fusions
	NumericKeyDictionary nkdPartMerges;

	// Dans le cas ou valueSet n'est pas nul, on renseigne aussi sa position dans une liste triee par nombre de
	// modalites des parties
	POSITION position;

	// Cles de hashage, pour gerer la table de hash des cellules
	int nHash;
	int nHash2;

	// Cout
	double dCost;
};

// Comparaison des nombres de modalites des parties
int KWDGMPartValueNumberCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMCell
// Cellule d'un DataGridMerger
class KWDGMCell : public KWDGCell
{
public:
	// Constructeur
	KWDGMCell();
	~KWDGMCell();

	// Cout local de la cellule
	void SetCost(double dValue);
	double GetCost() const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDataGridMerger;
	friend class KWDGMPartMergeAction;
	friend class KWDGMPart;

	// Cles de hashage, pour gerer la table de hash des cellules
	int nHash;
	int nHash2;

	// Cellule suivante dans la table de hash des cellules
	KWDGMCell* hashNextCell;

	// Cout
	double dCost;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMPartMerge
// Evaluation de la fusion de deux parties
class KWDGMPartMerge : public Object
{
public:
	// Constructeur
	KWDGMPartMerge();
	~KWDGMPartMerge();

	/////////////////////////////////////////////////////
	// Gestions des parties impliquees dans le groupage

	// Parametrage de la partie origine
	void SetPart1(KWDGMPart* part);
	KWDGMPart* GetPart1() const;

	// Parametrage de la partie destination
	void SetPart2(KWDGMPart* part);
	KWDGMPart* GetPart2() const;

	// Renvoie la partie oppose a la partie passe en parametre
	KWDGMPart* GetOppositePart(const KWDGMPart* part) const;

	/////////////////////////////////////////////////////////////
	// Evaluation de la fusion

	// Cout de fusion
	void SetMergeCost(double dValue);
	double GetMergeCost() const;

	// Position dans une liste
	void SetPosition(POSITION pos);
	POSITION GetPosition() const;

	// Presence d'un groupe poubelle apres la fusion
	void SetGarbagePresence(boolean bValue);
	boolean GetGarbagePresence();

	///////////////////////////////
	// Services divers

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void WriteHeaderLine(ostream& ost) const;
	void WriteLine(ostream& ost) const;
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation

	// Variante tronquee de la variation de cout, a utiliser uniquement dans les methodes de comparaison
	// Cela permet de stabilise le comportement des algoritjme quand on change de d'OS ou de processeur
	// en minimisant les cas de changement d'ordre a epsilon pret
	// Cette variante tronque est stockee ici pour des raison d'optimisation des methodes de comparaison
	double GetTruncatedMergeCost() const;

protected:
	// Parties origine et destination
	KWDGMPart* part1;
	KWDGMPart* part2;

	// Cout de fusion
	double dMergeCost;
	double dTruncatedMergeCost;

	// Position dans une liste
	POSITION position;

	// Presence d'un groupe poubelle pour l'attribut apres ce merge
	boolean bGarbagePresence;
};

// Comparaison de deux fusions de parties numerique, sur la base de leur cout
int KWDGMPartMergeCompare(const void* elem1, const void* elem2);

/////////////////////////////////////////////////
// Methodes en inline

// Classe KWDataGridMerger

inline void KWDataGridMerger::SetDataGridCosts(const KWDataGridCosts* kwdgcCosts)
{
	dataGridCosts = kwdgcCosts;
}

inline const KWDataGridCosts* KWDataGridMerger::GetDataGridCosts() const
{
	return dataGridCosts;
}

inline void KWDataGridMerger::SetCost(double dValue)
{
	dCost = dValue;
}

inline double KWDataGridMerger::GetCost() const
{
	return dCost;
}

// Classe KWDGMAttribute

inline void KWDGMAttribute::SetCost(double dValue)
{
	dCost = dValue;
}

inline double KWDGMAttribute::GetCost() const
{
	return dCost;
}

inline void KWDGMAttribute::AddPartToValueNumberList(KWDGMPart* partM)
{
	require(partM != NULL);
	require(partM->Check());
	require(partM->GetAttribute() == this);
	require(partM->GetPosition() == NULL);

	partM->SetPosition(slPartValueNumbers->Add(partM));
}

inline void KWDGMAttribute::RemovePartFromValueNumberList(KWDGMPart* partM)
{
	require(partM != NULL);
	require(partM->Check());
	require(partM->GetAttribute() == this);
	require(partM->GetPosition() != NULL);
	require(slPartValueNumbers->GetAt(partM->GetPosition()) == partM);

	slPartValueNumbers->RemoveAt(partM->GetPosition());
	partM->SetPosition(NULL);
}

inline void KWDGMAttribute::RemoveAllPartsFromValueNumberList()
{
	KWDGPart* part;
	KWDGMPart* partM;

	slPartValueNumbers->RemoveAll();

	// Dereferencement des positions dans la liste qui vient d'etre detruite
	part = GetHeadPart();
	while (part != NULL)
	{
		partM = cast(KWDGMPart*, part);
		partM->SetPosition(NULL);
		GetNextPart(part);
	}
}

inline void KWDGMAttribute::AddPartMerge(KWDGMPartMerge* partMerge)
{
	require(partMerge != NULL);
	require(partMerge->Check());
	require(partMerge->GetPart1()->GetAttribute() == this);
	require(partMerge->GetPosition() == NULL);

	partMerge->SetPosition(slPartMerges->Add(partMerge));
}

inline KWDGMPartMerge* KWDGMAttribute::GetBestPartMerge()
{
	KWDGMPartMerge* partMerge;

	if (slPartMerges->IsEmpty())
		return NULL;
	else
	{
		partMerge = cast(KWDGMPartMerge*, slPartMerges->GetHead());
		ensure(partMerge->Check());
		ensure(partMerge->GetPart1()->GetAttribute() == this);
		ensure(partMerge->GetPosition() != NULL);
		ensure(slPartMerges->GetAt(partMerge->GetPosition()) == partMerge);
		return partMerge;
	}
}

inline void KWDGMAttribute::RemovePartMerge(KWDGMPartMerge* partMerge)
{
	require(partMerge != NULL);
	require(partMerge->Check());
	require(partMerge->GetPart1()->GetAttribute() == this);
	require(partMerge->GetPosition() != NULL);
	require(slPartMerges->GetAt(partMerge->GetPosition()) == partMerge);

	slPartMerges->RemoveAt(partMerge->GetPosition());
	partMerge->SetPosition(NULL);
}

inline ObjectArray* KWDGMAttribute::GetAllPartMerges()
{
	ObjectArray* oaResults;
	oaResults = new ObjectArray;
	slPartMerges->ExportObjectArray(oaResults);
	return oaResults;
}

inline void KWDGMAttribute::RemoveAllPartMerges()
{
	slPartMerges->RemoveAll();
}

inline void KWDGMAttribute::DeleteAllPartMerges()
{
	slPartMerges->DeleteAll();
}

// Classe KWDGMPart

inline KWDGMPart::KWDGMPart()
{
	nHash = 0;
	nHash2 = 0;
	dCost = 0;
	position = NULL;
}

inline KWDGMPart::~KWDGMPart()
{
	debug(position = NULL);
}

inline void KWDGMPart::SetPosition(POSITION pos)
{
	position = pos;
}

inline POSITION KWDGMPart::GetPosition() const
{
	return position;
}

inline void KWDGMPart::SetCost(double dValue)
{
	dCost = dValue;
}

inline double KWDGMPart::GetCost() const
{
	return dCost;
}

inline void KWDGMPart::AddPartMerge(KWDGMPartMerge* partMerge)
{
	require(partMerge != NULL);
	require(partMerge->Check());
	require(partMerge->GetPart1() == this or partMerge->GetPart2() == this);
	require(LookupPartMerge(partMerge->GetOppositePart(this)) == NULL);

	nkdPartMerges.SetAt(partMerge->GetOppositePart(this), partMerge);
}

inline KWDGMPartMerge* KWDGMPart::LookupPartMerge(KWDGMPart* oppositePart) const
{
	KWDGMPartMerge* partMerge;

	require(oppositePart != NULL);
	require(oppositePart->GetAttribute() == GetAttribute());

	partMerge = cast(KWDGMPartMerge*, nkdPartMerges.Lookup(oppositePart));
	ensure(partMerge == NULL or partMerge->GetOppositePart(this) == oppositePart);
	return partMerge;
}

inline void KWDGMPart::RemovePartMerge(KWDGMPart* oppositePart)
{
	require(LookupPartMerge(oppositePart) != NULL);

	nkdPartMerges.RemoveKey(oppositePart);
}

inline POSITION KWDGMPart::GetStartPartMerge() const
{
	return nkdPartMerges.GetStartPosition();
}

inline void KWDGMPart::GetNextPartMerge(POSITION& positionPartMerge, KWDGMPartMerge*& partMerge) const
{
	NUMERIC key;
	Object* object;

	require(positionPartMerge != NULL);

	nkdPartMerges.GetNextAssoc(positionPartMerge, key, object);
	partMerge = cast(KWDGMPartMerge*, object);
	ensure(partMerge->Check());
	ensure(partMerge->GetPart1() == this or partMerge->GetPart2() == this);
	ensure(partMerge->GetOppositePart(this) == key);
}

inline void KWDGMPart::RemoveAllPartMerges()
{
	nkdPartMerges.RemoveAll();
}

// Classe KWDGMCell

inline KWDGMCell::KWDGMCell()
{
	nHash = 0;
	nHash2 = 0;
	hashNextCell = NULL;
	dCost = 0;
}

inline KWDGMCell::~KWDGMCell()
{
	debug(nHash = 0);
	debug(nHash2 = 0);
	debug(hashNextCell = NULL);
	debug(dCost = 0);
}

inline void KWDGMCell::SetCost(double dValue)
{
	dCost = dValue;
}

inline double KWDGMCell::GetCost() const
{
	return dCost;
}

// Classe KWDGMPartMerge

inline KWDGMPartMerge::KWDGMPartMerge()
{
	part1 = NULL;
	part2 = NULL;
	dMergeCost = 0;
	dTruncatedMergeCost = 0;
	position = NULL;
	bGarbagePresence = false;
}

inline KWDGMPartMerge::~KWDGMPartMerge()
{
	debug(part1 = NULL);
	debug(part2 = NULL);
	debug(dMergeCost = 0);
	debug(position = NULL);
}

inline void KWDGMPartMerge::SetPart1(KWDGMPart* part)
{
	part1 = part;
}

inline KWDGMPart* KWDGMPartMerge::GetPart1() const
{
	return part1;
}

inline void KWDGMPartMerge::SetPart2(KWDGMPart* part)
{
	part2 = part;
}

inline KWDGMPart* KWDGMPartMerge::GetPart2() const
{
	return part2;
}

inline KWDGMPart* KWDGMPartMerge::GetOppositePart(const KWDGMPart* part) const
{
	require(Check());
	require(part1 == part or part2 == part);

	return part == part1 ? part2 : part1;
}

inline void KWDGMPartMerge::SetMergeCost(double dValue)
{
	// Controle que le PartMerge n'est pas deja dans une liste triee (qui utilise le MergeCost pour les
	// comparaisons)
	require(position == NULL);
	dMergeCost = dValue;
	dTruncatedMergeCost = KWContinuous::DoubleToContinuous(dMergeCost);
}

inline double KWDGMPartMerge::GetMergeCost() const
{
	return dMergeCost;
}

inline double KWDGMPartMerge::GetTruncatedMergeCost() const
{
	return dTruncatedMergeCost;
}

inline void KWDGMPartMerge::SetPosition(POSITION pos)
{
	require(pos == NULL or position == NULL);
	position = pos;
}

inline POSITION KWDGMPartMerge::GetPosition() const
{
	return position;
}

inline void KWDGMPartMerge::SetGarbagePresence(boolean bValue)
{
	bGarbagePresence = bValue;
}
inline boolean KWDGMPartMerge::GetGarbagePresence()
{
	return bGarbagePresence;
}
