// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	if (LMLicenseManager::IsEnabled())
		LMLicenseManager::DeclarePredefinedLicense(LMLicenseManager::KhiopsCoclustering);

	// Parametrage du nom du module applicatif
	SetLearningModuleName("Coclustering");

	// Parametrage de l'icone de l'application
	UIObject::SetIconImage("images/khiops_coclustering.gif");

	// Parametrage du menu about
	SetLearningAboutImage("images/khiops_coclustering_about.gif");
	SetLearningWebSite("https://khiops.org");

	// Documentation
	sDocumentation += "<h4> Reference Guide and Tutorial </h4>"
#ifdef _WIN32
			  "<p> In the 'doc' sub-directory of the installation directory </p>";
#elif defined __linux_or_apple__
			  "<p> In the '/usr/share/doc/khiops' directory </p>";
#endif

	// Sample datasets
	sDocumentation += "<h4> Sample Datasets </h4> "
			  "<p> See the 'samples' directory "
#ifdef _WIN32
			  "in the 'Public' directory, </p>"
			  "<p> usually C:\\Users\\Public\\khiops_data\\samples </p>";
#elif defined __linux_or_apple__
			  "in $HOME/khiops_data/samples </p>";
#endif

	// JSON Files
	sDocumentation +=
	    "<h4> JSON Dictionary and Modeling Results Files</h4> "
	    "<p> The modeling results of Khiops (.khj) and Khiops Coclustering (.khcj) are stored using </p> "
	    "<p> the JSON format. Dictionaries files (.kdic) may also be exported to JSON format (.kdicj). </p> "
	    "<p> These JSON files may be used with the Khiops Python library, </p>"
	    "<p> the Khiops Visualization tools (see below) or other custom external tools. </p>";

	// Outils de visualisation
	sDocumentation += "<h4> Visualization Tools </h4> "
			  "<p> The modeling result files (.khj and .khcj) can be visualized </p>"
			  "<p> using the Khiops Visualization or Khiops Covisualization tools. </p>";

	// Librairie Python
	sDocumentation += "<h4> Khiops Python Library </h4> "
			  "<p> This library allows to automatize the tool execution and to access </p>"
			  "<p> its analysis result files. More information at the Khiops website. </p>";

	// Parametrage de la documentation
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
