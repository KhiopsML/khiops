// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MDKhiopsLearningProject.h"

MDKhiopsLearningProject::MDKhiopsLearningProject() {}

MDKhiopsLearningProject::~MDKhiopsLearningProject() {}

void MDKhiopsLearningProject::OpenLearningEnvironnement()
{
	ALString sDocumentation;
	ALString sQuickStartInfo;

	// Appel de la methode ancetre
	KWLearningProject::OpenLearningEnvironnement();

	// Enregistrement des regles pour les outils de l'eco-systeme Khiops: Enneade
	KMDRRegisterAllRules();

	// Enregistrement de methodes de pretraitement specifiques aux arbres de decision
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new DTDiscretizerMODL);
	KWGrouper::RegisterGrouper(KWType::Symbol, new DTGrouperMODL);

	// Parametrage de l'icone de l'application
	UIObject::SetIconImage("khiops.gif");

	// Parametrage du menu about
	SetLearningAboutImage("khiops_about.gif");
	SetLearningWebSite("www.khiops.com");

	// Parametrage de la fenetre de quick start
	// On peut s'aider de deepl (https://www.deepl.com/translator) pour la rédaction en anglais
	// et d'un editeur html en ligne (ex: https://html-online.com/editor/) pur la mise en forme
	sQuickStartInfo += "<html> ";
	sQuickStartInfo += "<h2>Fast path</h2> ";
	sQuickStartInfo += "<h3>Explore you data</h3> ";
	sQuickStartInfo += "<ul> ";
	sQuickStartInfo +=
	    "<li>Enter the name of the file in the 'Data table file' field of the 'Train database' pane</li> ";
	sQuickStartInfo += "<li>Click on the 'Train model' button</li> ";
	sQuickStartInfo += "<li>Click on the 'Visualize results' button in the 'Results' pane</li> ";
	sQuickStartInfo += "</ul> ";
	sQuickStartInfo += "<h3>Build a classification model</h3> ";
	sQuickStartInfo += "<ul> ";
	sQuickStartInfo +=
	    "<li>Enter the name of the input file in the 'Data table file' field of the 'Train database' pane</li> ";
	sQuickStartInfo += "<li>Enter the name of the variable to predict in the 'Target variable' field of the "
			   "'Parameters' pane.</li> ";
	sQuickStartInfo += "<li>Click on the 'Train model' button</li> ";
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

	///////////////////////////////////////////////////
	// Parametrage de la fenetre de documentation

	// Documentation
	sDocumentation = "<html> ";
	sDocumentation += "<h3> " + GetLearningFullApplicationName() + "</h3> ";
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