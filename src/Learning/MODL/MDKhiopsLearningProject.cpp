// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MDKhiopsLearningProject.h"

MDKhiopsLearningProject::MDKhiopsLearningProject() {}

MDKhiopsLearningProject::~MDKhiopsLearningProject() {}

void MDKhiopsLearningProject::OpenLearningEnvironnement()
{
	ALString sDocumentation;

	// Appel de la methode ancetre
	KWLearningProject::OpenLearningEnvironnement();

	// Declaration des licences
	if (LMLicenseManager::IsEnabled())
		LMLicenseManager::DeclarePredefinedLicense(LMLicenseManager::Khiops);

	// Enregistrement des regles pour les outils de l'eco-systeme Khiops: Enneade
	KMDRRegisterAllRules();

	// Enregistrement de methodes de pretraitement specifiques aux arbres de decision
	KWDiscretizer::RegisterDiscretizer(new DTDiscretizerMODL);
	KWGrouper::RegisterGrouper(new DTGrouperMODL);

	// Parametrage de l'icone de l'application
	UIObject::SetIconImage("khiops.gif");

	// Parametrage du menu about
	SetLearningAboutImage("khiops_about.gif");
	SetLearningWebSite("www.khiops.com");

	// Parametrage de la fenetre de documentation
	sDocumentation = "<html> ";
	sDocumentation += "<h3> " + GetLearningFullApplicationName() + "</h3> ";

	// Documentation
	sDocumentation += "<h4> Reference guide and tutorial </h4> ";
#ifdef _WIN32
	sDocumentation += "<p> In the 'doc' sub-directory of the installation directory </p> ";
#elif defined __linux__
	sDocumentation += "<p> In the '/usr/share/doc/khiops' directory </p> ";
#endif

	// Examples
	sDocumentation += "<h4> Sample data sets </h4> ";
#ifdef _WIN32
	sDocumentation += "<p> In the 'samples' sub-directory of the installation directory </p> ";
#elif defined __linux__
	sDocumentation += "<p> In the '/usr/share/doc/khiops/samples' directory </p> ";
#endif

	// Autre resources
	sDocumentation += "<h4> Dictionaries and modeling results under the json format </h4> ";
	sDocumentation +=
	    "<p> The modeling results of Khiops (.khj) and Khiops coclustrering (.khcj) are stored using </p> "
	    "<p> the json format. Dictionaries (.kdic) can also be exported under the json format (.kdicj). </p> "
	    "<p> All these json files can be exploited from external tools, like pykhiops.</p> ";
	sDocumentation += "<h4> Visualization tools </h4> ";
	sDocumentation += "<p> All modeling results can be visualized using the Khiops visualization or Khiops "
			  "covisualization tools.</p> ";
	sDocumentation += "<h4> Scripting libraries </h4> ";
	sDocumentation += "<p> Full API to run the tool and to inspect its modeling results </p> ";
	sDocumentation += "<p> pykhiops: python library </p> ";
	sDocumentation += "<h4> KNI: Khiops Native Interface </h4> ";
	sDocumentation += "<p> DLL for online deployment of models </p> ";
	sDocumentation += "<h4> Web site </h4> ";
	sDocumentation += "<p> Documentation and other resources are available for download </p> ";
	sDocumentation += "</html>";
	KWLearningProblemHelpCard::SetDocumentationText(sDocumentation);
}

KWLearningProblem* MDKhiopsLearningProject::CreateLearningProblem()
{
	return new MDKhiopsLearningProblem;
}

KWLearningProblemView* MDKhiopsLearningProject::CreateLearningProblemView()
{
	return new MDKhiopsLearningProblemView;
}