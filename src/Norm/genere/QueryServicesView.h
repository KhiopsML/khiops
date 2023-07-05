// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "QueryServices.h"
#include "UserInterface.h"

///////////////////////////////////////////////////////////////////////
// Classe QueryServicesView
// Visualisation des service de requete dans une interface utilisateur
// Certains services extraient une liste d'objets, qu'il est possible
// de consulter ensuite
class QueryServicesView : public UICard
{
public:
	// Constructeur
	// objectsView est une maquette liste permettant de visualiser les objets
	// des requetes de qsValue
	// Memoire: objectView appartient au QueryServicesView; il doit avoir ete
	// alloue par l'appelant
	QueryServicesView(QueryServices* qsValue, UIObjectArrayView* objectsView);
	~QueryServicesView();

	//// Commandes du menu
	void QueryAllObjects();
	void QueryObjectsMatching();
	void QueryStatistics();

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Methode de test
	static void Test(ostream& ost);

	////////// Implementation
protected:
	// Attributs de la classe
	QueryServices* qsQueryServices;
	UIObjectArrayView* queryResultField;
	ObjectArray* oaCurrentQuery;
};