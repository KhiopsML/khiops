// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProject.h"

CCLearningProject::CCLearningProject() {}

CCLearningProject::~CCLearningProject() {}

void CCLearningProject::OpenLearningEnvironnement()
{
	ALString sDocumentation;
	ALString sQuickStartInfo;

	// Appel de la methode ancetre
	KWLearningProject::OpenLearningEnvironnement();

	// Parametrage du nom du module applicatif
	SetLearningModuleName("Coclustering");

	// Parametrage de l'icone de l'application
	UIObject::SetIconImage("khiops_coclustering.gif");

	// Parametrage du menu about
	SetLearningAboutImage("khiops_coclustering_about.gif");
	SetLearningWebSite("www.khiops.com");

	// Parametrage de la fenetre de quick start
	// On peut s'aider de deepl (https://www.deepl.com/translator) pour la rédaction en anglais
	// et d'un editeur html en ligne (ex: https://html-online.com/editor/) pur la mise en forme
	sQuickStartInfo += "<html> ";
	sQuickStartInfo += "<h2>Fast path</h2> ";
	sQuickStartInfo += "<h3>Build a coclustering report</h3> ";
	sQuickStartInfo += "<ul> ";
	sQuickStartInfo +=
	    "<li>Enter the name of the input file in the 'Data table file' field of the 'Database' pane</li> ";
	sQuickStartInfo += "<li>Insert the coclustering variables to analyze in the 'Parameters' pane</li> ";
	sQuickStartInfo += "<li>Click on the 'Train coclustering' button</li> ";
	sQuickStartInfo += "<li>Click on the 'Visualize results' button in the 'Results' pane</li> ";
	sQuickStartInfo += "</ul> ";
	sQuickStartInfo += "<h2>What is a data dictionary?</h2> ";
	sQuickStartInfo += "<p>A data dictionary allows you to specify the type and name of variables in a data file, "
			   "with additional key features:</p> ";
	sQuickStartInfo += "<ul> ";
	sQuickStartInfo += "<li>Select variables to exclude or not from the analysis</li> ";
	sQuickStartInfo +=
	    "<li>Organize your data in a multi-table schema, such as a star schema or a snowflake schema</li> ";
	sQuickStartInfo += "<li>Create new variables calculated via derivation rules</li> ";
	sQuickStartInfo += "<li>Store the data transformation flows of the prediction models obtained from the machine "
			   "learning output</li> ";
	sQuickStartInfo +=
	    "<li>Allow recoding of data or deployment of prediction scores via the 'Deploy Model' feature</li> ";
	sQuickStartInfo += "</ul> ";
	sQuickStartInfo += "<p>A dictionary file contains one or several dictionaries.</p> ";
	sQuickStartInfo += "<h2>Normal path</h2> ";
	sQuickStartInfo += "<h3>Manage data dictionaries</h3> ";
	sQuickStartInfo += "<ul> ";
	sQuickStartInfo += "<li>Click on the 'Manage dictionaries' sub-menu of the 'Data dictionary' menu</li> ";
	sQuickStartInfo += "</ul> ";
	sQuickStartInfo += "<p>A dialog box appears, which allows you to build a dictionary from a data file and edit "
			   "the dictionaries of a dictionary file.</p> ";
	sQuickStartInfo += "<h3>Use a data dictionary</h3> ";
	sQuickStartInfo += "<ul> ";
	sQuickStartInfo += "<li>Click on the 'Open' sub-menu of the 'Data dictionary' menu</li> ";
	sQuickStartInfo += "<li>Choose the dictionary file (extentions .kdic)</li> ";
	sQuickStartInfo += "<li>Enter the name the the dictionary in the 'Analysis dictionary' field of the 'Train "
			   "database' pane</li> ";
	sQuickStartInfo += "</ul> ";
	sQuickStartInfo += "</html> ";
	KWLearningProblemHelpCard::SetQuickStartText(sQuickStartInfo);

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