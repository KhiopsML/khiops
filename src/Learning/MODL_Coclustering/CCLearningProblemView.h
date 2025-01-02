// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

#include "CCLearningProblem.h"

#include "KWClassManagementActionView.h"
#include "KWDatabaseView.h"
#include "CCAnalysisSpecView.h"
#include "CCAnalysisResultsView.h"
#include "CCLearningProblemActionView.h"
#include "KWClassBuilderView.h"
#include "KWClassAttributeHelpList.h"
#include "KWDatabaseAttributeValuesHelpList.h"
#include "KWLearningProblemHelpCard.h"

////////////////////////////////////////////////////////////
// Classe CCLearningProblemView
// Editeur de CCLearningProblem
class CCLearningProblemView : public UIObjectView
{
public:
	// Constructeur
	CCLearningProblemView();
	~CCLearningProblemView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions disponibles
	void BuildCoclustering();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;
	CCLearningProblem* GetLearningProblem();

	/////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Construction de dictionnaire a partir d'un fichier
	// On fait une detection automatique de format suivi d'une construction de dictionnaire,
	// pour permettre l'analyse de donnees sans dictionnaire prealable
	// On renvoie true OK
	// Sinon, un nouveau dictionnaire de nom base sur le nom du fichier, est cree est disponible en memoire
	// Il n'est pas sauve dansd le fichier des dictionnaires
	boolean BuildClassFromDataTable();

	// Rafraichissement des listes d'aide
	void RefreshHelpLists();

	// Gestion des listes d'aide
	KWClassAttributeHelpList simpleAttributeHelpList;
	KWClassAttributeHelpList continuousAttributeHelpList;
	// CH IV Begin
	KWClassAttributeHelpList categoricalAttributeHelpList;
	// CH IV End
};
