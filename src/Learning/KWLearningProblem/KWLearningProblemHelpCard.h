// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWVersion.h"

////////////////////////////////////////////////////////////
// Classe KWLearningProblemHelpCard
//    Actions d'analyse deportees de KWLearningProblemView
class KWLearningProblemHelpCard : public UICard
{
public:
	// Constructeur
	KWLearningProblemHelpCard();
	~KWLearningProblemHelpCard();

	// Actions de menu
	void ShowQuickStart();
	void ShowDocumentation();
	void ShowAbout();

	////////////////////////////////////////////////////////////
	// Parametrage des texte d'aide
	// Le texte a passer est au format html et sera visualise dans une boite de dialogue
	// Le parametrage doit etre fait des le depart, avant le lancement de l'application

	// Parametrage du quick start (par defaut: vide)
	static void SetQuickStartText(const ALString& sValue);
	static const ALString& GetQuickStartText();

	// Parametrage de la documentation (par defaut: vide)
	static void SetDocumentationText(const ALString& sValue);
	static const ALString& GetDocumentationText();

	////////////////////////////////////////////////////////////
	///// Implementation
protected:
	static ALString sQuickStartText;
	static ALString sDocumentationText;
};
