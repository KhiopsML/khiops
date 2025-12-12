// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	UIObject::SetIconImage("images/khiops_coclustering.gif");

	// Parametrage du menu about
	SetLearningAboutImage("images/khiops_coclustering_about.gif");
	SetLearningWebSite("https://khiops.org");

	// Parametrage de la fenetre de quick start
	// On peut s'aider de deepl (https://www.deepl.com/translator) pour la redaction en anglais
	// et d'un editeur html en ligne (ex: https://html-online.com/editor/) pur la mise en forme
	sQuickStartInfo += "<html> ";
	sQuickStartInfo += "<h3>Fast path</h3> ";
	sQuickStartInfo += "<h4>Build a coclustering report</h4> ";
	sQuickStartInfo +=
	    "<li>Enter the name of the input file in the 'Data table file' field of the 'Database' pane</li> ";
	sQuickStartInfo += "<li>Insert the coclustering variables to analyze in the 'Parameters' pane</li> ";
	sQuickStartInfo += "<li>Click on the 'Train coclustering' button</li> ";
	sQuickStartInfo += "<li>Click on the 'Visualize results' button in the 'Results' pane</li> ";
	sQuickStartInfo += "<h3>What is a data dictionary?</h3> ";
	sQuickStartInfo += "<p>A data dictionary allows you to define the type and name of variables in a data file, "
			   "with additional key features:</p> ";
	sQuickStartInfo += "<ul>";
	sQuickStartInfo += "<li>Selecting variables to include or exclude from analysis,</li> ";
	sQuickStartInfo +=
	    "<li>Organizing data within a multi-table schema, such as a star schema or snowflake schema,</li> ";
	sQuickStartInfo += "<li>Creating new variables through derivation rules,</li> ";
	sQuickStartInfo +=
	    "<li>Storing data transformation workflows derived from machine learning model outputs,</li> ";
	sQuickStartInfo += "<li>Facilitating data transformation of the input database via the 'Deploy model' feature, "
			   "which includes:";
	sQuickStartInfo += "<ul>";
	sQuickStartInfo += "<li> Deploying prediction scores using a prediction model,</li>";
	sQuickStartInfo += "<li> Recoding data with a recoder model,</li>";
	sQuickStartInfo += "<li> Generating interpretation indicators with an interpreter model,</li>";
	sQuickStartInfo += "<li> Deploying or reinforcing scores using a reinforcer model,</li>";
	sQuickStartInfo += "<li>...</li>";
	sQuickStartInfo += "</ul>";
	sQuickStartInfo += "</li>";
	sQuickStartInfo += "</ul>";
	sQuickStartInfo += "<p>A dictionary file contains one or several dictionaries.</p> ";
	sQuickStartInfo += "<h3>Standard path</h3> ";
	sQuickStartInfo += "<h4>Manage data dictionaries</h4> ";
	sQuickStartInfo += "<li>Click on the 'Manage dictionaries' sub-menu of the 'Data dictionary' menu</li> ";
	sQuickStartInfo += "<p>A dialog box appears, which allows you to build a dictionary from a data file and edit "
			   "the dictionaries of a dictionary file.</p> ";
	sQuickStartInfo += "<h4>Use a data dictionary</h4> ";
	sQuickStartInfo += "<li>Click on the 'Open' sub-menu of the 'Data dictionary' menu</li> ";
	sQuickStartInfo += "<li>Choose the dictionary file (extentions .kdic)</li> ";
	sQuickStartInfo += "<li>Enter the name the dictionary in the 'Analysis dictionary' field of the 'Train "
			   "database' pane</li> ";
	sQuickStartInfo += "</html> ";
	KWLearningProblemHelpCard::SetQuickStartText(sQuickStartInfo);

	// Parametrage de la fenetre de documentation
	sDocumentation += "<h4> Reference Guide and Tutorial </h4>"
			  "<p> On the web site </p>";

	/// Sample datasets
	sDocumentation += "<h4> Sample Datasets </h4> "
#ifdef _WIN32
			  "<p> See the 'samples' directory "
			  "in the 'Public' directory, usually C:\\Users\\Public\\khiops_data\\samples </p>";
#elif defined __linux_or_apple__
			  "<p> On the web site, go to \"Installation\" of the \"Khiops application\" and select a "
			  "Linux distribution. </p>";
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
