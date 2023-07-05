// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDRPreprocessing.h"
#include "KWDataGridStats.h"
#include "KWAttributeStats.h"
#include "DTBaseLoader.h"

///////////////////////////////////////////////////////////////
/// Separe une DTBaseLoader en plusieurs DTBaseLoader
/// selon une regle de derivation du type Group ou Discretisation
/// La base mere et les bases crees appartiennent a l'appelant
///(elles ne sont pas detruites lors de la destruction de l'objet)
class DTBaseLoaderSplitter : public Object
{
public:
	/// Constructeur
	DTBaseLoaderSplitter();
	~DTBaseLoaderSplitter();

	/// Acces a la DTBaseLoader mere de Train
	DTBaseLoader* GetOrigineBaseLoader() const;
	void SetOrigineBaseLoader(DTBaseLoader* newDatabase);

	// DTBaseLoader* GetdatabaseloaderOutOfBag() const;
	// void SetdatabaseloaderOutOfBag(DTBaseLoader* newDatabase);

	/// Acces au tableau de DTBaseLoader filles
	ObjectArray* GetDaughterBaseloader();

	/// Acces au tableau de DTBaseLoader out-of-bag filles
	// ObjectArray * GetOutOfBagDaughterDatabases();

	/// Acces au nombre de DTBaseLoader filles
	int GetDaughterBaseloaderNumber();

	/// Acces au nombre de DTBaseLoader out-of-bag filles
	// int GetOutOfBagDaughterDatabaseNumber();

	/// Acces a la ieme DTBaseLoader fille
	DTBaseLoader* GetDaughterBaseloaderAt(int i);

	/// Acces a la ieme DTBaseLoader out-of-bag fille
	// DTBaseLoader* GetOutOfBagDaughterDatabaseAt(int i);

	/// Separe les DTBaseLoader meres (train et OOB) selon un attribut de partitionnement
	boolean CreateDaughterBaseloaderFromSplitAttribute(KWAttributeStats* splitAttributeStats);

	/// Libere la memoire
	void CleanDaughterBaseloader();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	/// Pointeur vers la DTBaseLoader train d'origine
	/// Appartient a l'appelant
	DTBaseLoader* databaseloaderOrigine;

	/// Pointeur vers la DTBaseLoader OOB d'origine
	/// Appartient a l'appelant
	// DTBaseLoader* databaseloaderOutOfBag;

	/// Tableau de pointeurs des DTBaseLoader obtenues apres application de la regle de derivation
	/// Les DTBaseLoader creees appartiennent a l'appelant
	ObjectArray oaTrainDaughterBaseLoader;

	/// Tableau de pointeurs des DTBaseLoader obtenues apres application de la regle de derivation. Ces databases ne
	/// contiennent que les instances qui sont marquees comme "out of bag" Les DTBaseLoader creees appartiennent a
	/// l'appelant
	// ObjectArray oaOutOfBagDaughterBaseLoader;

	/// Tableau de pointeurs des DTBaseLoader obtenues apres application de la regle de derivation
	/// Les DTBaseLoader creees appartiennent a l'appelant
	// ObjectArray oaTrainDaughterObjects;
	// ObjectArray oaOutOfBagDaughterObjects;

	/// Tableau de pointeurs des DTBaseLoader obtenues apres application de la regle de derivation
	/// Les DTBaseLoader creees appartiennent a l'appelant
	// ObjectArray oaTrainDaughterTargetVector;
	// ObjectArray oaOutOfBagDaughterTargetVector;

	/// Tableau de pointeurs des DTBaseLoader obtenues apres application de la regle de derivation
	/// Les DTBaseLoader creees appartiennent a l'appelant
	// ObjectArray oaTrainDaughterTargetTupleTable;
	// ObjectArray oaOutOfBagDaughterTargetTupleTable;

	// ObjectArray oaTrainDaughterTableLoader;
};