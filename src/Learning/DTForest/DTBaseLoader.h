// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "KWLearningSpec.h"
#include "KWTupleTableLoader.h"

////////////////////////////////////////////////////////////////////
// Classe DTBaseLoader
// Encapsule les donnees necessaires a un noeud de l'arbre pour calculer
// ses statistiques : les specifications d'apprentissage, le chargeur de tuples
// et le tableau des instances de la partition de base de donnees associee.
class DTBaseLoader : public Object
{
public:
	DTBaseLoader();
	~DTBaseLoader();

	// Initialisation a partir des specifications d'apprentissage, du chargeur de tuples et du tableau d'instances
	void Initialize(KWLearningSpec* lSpec, KWTupleTableLoader* maintupleTableLoader, ObjectArray* oadb);
	boolean Check();

	// Acces au chargeur de tuples associe
	KWTupleTableLoader* GetTupleLoader() const;

	// Acces au tableau des instances de la base
	ObjectArray* GetDatabaseObjects() const;

	// Acces aux specifications d'apprentissage
	KWLearningSpec* GetLearningSpec() const;

	// Ecriture d'un rapport abrege
	void Write(ostream& ost);

	void DeleteAll();

	// Construction des loaders train et out-of-bag a partir du loader courant
	void BuildTrainOutOfBagBaseLoader(DTBaseLoader* blTrain, DTBaseLoader* blOutOfBag);

	// Alimentation d'une table de tuples univariee a partir d'un vecteur de valeurs symboliques
	// Parametres en entree :
	//   - kwcInputClass : classe d'entree contenant l'attribut cible
	//   - sAttributeName : nom de l'attribut cible a charger dans la table de tuples
	//   - svInputValues : vecteur de valeurs symboliques a charger (une valeur par instance)
	// Parametre en sortie :
	//   - outputTupleTable : table de tuples alimentee avec les valeurs du vecteur
	//                        (table nettoyee puis remplie en mode edition)
	void LoadTupleTableFromSymbolValues(KWClass* kwcInputClass, const ALString& sAttributeName,
					    const SymbolVector* svInputValues, KWTupleTable* outputTupleTable);

protected:
	KWTupleTableLoader* tupleTableLoader;
	ObjectArray* database;
	KWLearningSpec* learningSpec;
};
