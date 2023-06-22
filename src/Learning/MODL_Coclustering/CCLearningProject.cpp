// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProject.h"

CCLearningProject::CCLearningProject() {}

CCLearningProject::~CCLearningProject() {}

void CCLearningProject::OpenLearningEnvironnement()
{
	ALString sDocumentation;

	// Appel de la methode ancetre
	KWLearningProject::OpenLearningEnvironnement();

	// Declaration des licences
	LMLicenseManager::DeclarePredefinedLicense(LMLicenseManager::KhiopsCoclustering);

	// Parametrage du nom du module applicatif
	SetLearningModuleName("Coclustering");

	// Parametrage de l'icone de l'application
	UIObject::SetIconImage("khiops_coclustering.gif");

	// Parametrage du menu about
	SetLearningAboutImage("khiops_coclustering_about.gif");
	SetLearningWebSite("www.khiops.com");

	// Parametrage de la fenetre de documentation
	sDocumentation = "<html> ";
	sDocumentation += "<h3> " + GetLearningFullApplicationName() + "</h3> ";
	sDocumentation += "<p> Documentation and other resources are available in the installation directory and on "
			  "the web site </p> ";
	sDocumentation += "<h4> Reference guide and tutorial </h4> ";
	sDocumentation += "<p> In the 'doc' sub-directory of the installation directory </p> ";
	sDocumentation += "<p> KhiopsCoclusteringGuide.pdf </p> ";
	sDocumentation += "<p> KhiopsCoclusteringVisualizationGuide.pdf </p> ";
	sDocumentation += "<p> KhiopsTutorial.pdf </p> ";
	sDocumentation += "<h4> Sample data sets </h4> ";
	sDocumentation += "<p> In the 'samples' sub-directory of the installation directory </p> ";
	sDocumentation += "<h4> pykhiops </h4> ";
	sDocumentation += "<p> In the 'python' sub-directory of the installation directory </p> ";
	sDocumentation += "<p> Full python library to run the tool and to inspect its results from python </p> ";
	sDocumentation += "</html>";
	KWLearningProblemHelpCard::SetDocumentationText(sDocumentation);
}

Object* CCLearningProject::CreateGenericLearningProblem()
{
	return new CCLearningProblem;
}

UIObjectView* CCLearningProject::CreateGenericLearningProblemView()
{
	return new CCLearningProblemView;
}