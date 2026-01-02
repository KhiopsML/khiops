// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDGMPartMergeAction;
class KWDGMPartMergeArray;

#include "KWDataGridMerger.h"
#include "KWDataGridCosts.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMPartMergeAction
// Action de fusions de deux parties
// Mise a a jour complete et coherente d'un DataGridMerge, en effectuant la fusion
// et evaluant l'impact sur toutes les fusions pour l'attribut interne et
// les attributs externes de la fusion
class KWDGMPartMergeAction : public Object
{
public:
	// Constructeur
	KWDGMPartMergeAction();
	~KWDGMPartMergeAction();

	// Realisation d'une fusion
	// Mise a jour des structures du DataGrid, de la table hash des cellules
	// Prise en compte de la structure des fusion en evaluant les fusions impactees
	// Destruction de la fusion passee en parametre
	// Retourne la partie resultat de la fusion
	// Prise en compte du groupe poubelle
	KWDGMPart* PerformPartMerge(KWDGMPartMerge* partMerge);

	///////////////////////////////
	///// Implementation
protected:
	// Test si les parametres de la fusion sont correctement initialises
	boolean IsInitialized() const;

	/////////////////////////////////////////////////////////////////////////////
	// Gestion des impact d'une fusion sur les attributs externes de la fusion
	// L'ensemble des impact est gere dans un container a plusieurs niveaux:
	//    - un tableau des impacts, indexe par les attributs externes
	//	  - pour chaque attribut, un dictionnaires des impacts, indexe par les
	//      parties impactees de l'attribut externe
	//    - pour chaque partie impactee, un tableau des fusions impactees
	//	    (stocke dans un objet KWDGMPartMergeArray)
	// Les fusions impactees sont presentes en un unique exemplaire dans la structure

	// Mise a jour complete et optimisee des fusions impactees
	// Cette methode realise en une seule passe l'identification, l'evaluation
	// et la mise a jour des fusions impactees
	void GlobalUpdateImpactedPartMerges(ObjectArray* oaTransferredCells1, ObjectArray* oaMergedCells1,
					    ObjectArray* oaMergedCells2);

	// Mise a jour complete et optimisee des fusions impactees, pour un attribut externe
	void
	GlobalUpdateImpactedPartMergesForAttribute(KWDGMAttribute* impactedAttribute, ObjectArray* oaTransferredCells1,
						   NumericKeyDictionary* nkdTransferredCells1,
						   ObjectArray* oaMergedCells1, NumericKeyDictionary* nkdMergedCells1,
						   ObjectArray* oaMergedCells2, NumericKeyDictionary* nkdMergedCells2);

	// Initialisation d'un container de fusions externes impactees
	// On renvoie un tableau de dictionnaires de KWDGMPartMergeArray
	void InitializeImpactedPartMerges();

	// Recherche des fusions externes impactees lors de la fusion de deux parties,
	// pour un attribut externe donne
	// On renvoie un dictionnaire de tableaux de fusions (KWDGMPartMergeArray)
	// indexe par les parties externes des cellules de la partie origine de la fusion
	NumericKeyDictionary* InitializeImpactedPartMergesForAttribute(KWDGMAttribute* impactedAttribute);

	// Suppression des fusions de parties de leur attribut
	void RemoveImpactedPartMergesFromAttributes() const;

	// Evaluation des couts des fusions
	void EvaluateImpactedPartMerges() const;

	// Ajout des fusions de parties a leur attribut
	void AddImpactedPartMergesToAttributes() const;

	// Destruction du container de fusions externes impactees
	// Sont detruits le tableau principal, ses dictionnaires et leurs
	// KWDGMPartMergeArray (mais pas les fusions referencees par la structure)
	void RemoveImpactedPartMerges();

	// Verification d'un container de fusions externes impactees
	boolean CheckImpactedPartMerges() const;

	// Affichage d'un container de fusions externes impactees
	void WriteImpactedPartMerges(ostream& ost) const;

	// Ensemble des impacts sur les fusions externes
	ObjectArray* oaImpactedPartMerges;

	// Parametres principaux de la fusion
	KWDGMPart* part1;
	KWDGMPart* part2;
	KWDataGridMerger* dataGridMerger;
	const KWDataGridCosts* dataGridCosts;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGMPartMergeArray
// Impacts de la fusion de deux parties, sous la forme d'une liste de parties
// destination dont la fusion est a reevaluer avec une partie origine
class KWDGMPartMergeArray : public Object
{
public:
	// Constructeur
	KWDGMPartMergeArray();
	~KWDGMPartMergeArray();

	/////////////////////////////////////////////////////
	// Gestions des parties impliquees dans le groupage

	// Parametrage de la partie origine
	void SetPart1(KWDGMPart* part);
	KWDGMPart* GetPart1() const;

	// Parametrage des merges avec des parties destinations (KWDGMPartMerge)
	ObjectArray* GetPartMerges();

	///////////////////////////////
	// Services divers

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Parties origine et destination
	KWDGMPart* part1;
	ObjectArray oaPartMerges;
};

/////////////////////////////////////////////////
// Methodes en inline

// Classe KWDGMPartMergeArray

inline KWDGMPartMergeArray::KWDGMPartMergeArray()
{
	part1 = NULL;
}

inline KWDGMPartMergeArray::~KWDGMPartMergeArray()
{
	debug(part1 = NULL);
	debug(oaPartMerges.SetSize(0));
}

inline void KWDGMPartMergeArray::SetPart1(KWDGMPart* part)
{
	part1 = part;
}

inline KWDGMPart* KWDGMPartMergeArray::GetPart1() const
{
	return part1;
}

inline ObjectArray* KWDGMPartMergeArray::GetPartMerges()
{
	return &oaPartMerges;
}
