// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningProblemHelpCard.h"

KWLearningProblemHelpCard::KWLearningProblemHelpCard()
{
	ALString sUserName;

	// Libelles
	SetIdentifier("KWLearningProblemHelpCard");
	SetLabel("Help");

	// Declaration des actions
	AddAction("ShowQuickStart", "Quick start...", (ActionMethod)(&KWLearningProblemHelpCard::ShowQuickStart));
	AddAction("ShowDocumentation", "Documentation...",
		  (ActionMethod)(&KWLearningProblemHelpCard::ShowDocumentation));
	AddAction("ShowAbout", "About " + GetLearningFullApplicationName() + "... ",
		  (ActionMethod)(&KWLearningProblemHelpCard::ShowAbout));

	// Action de documentation non visible si pas de texte
	GetActionAt("ShowDocumentation")->SetVisible(sDocumentationText != "");

	// Short cuts
	SetShortCut('H');
	GetActionAt("ShowQuickStart")->SetShortCut('Q');
	GetActionAt("ShowAbout")->SetShortCut('A');
	GetActionAt("ShowDocumentation")->SetShortCut('D');
}

KWLearningProblemHelpCard::~KWLearningProblemHelpCard() {}

void KWLearningProblemHelpCard::ShowQuickStart()
{
	UICard quickStartCard;

	require(GetFieldNumber() == 0);

	// Titre
	quickStartCard.SetIdentifier("QuickStart");
	quickStartCard.SetLabel(GetLearningFullApplicationName() + " quick start");

	// Parametrage du texte a afficher
	quickStartCard.AddStringField("Information", "", sQuickStartText);
	quickStartCard.GetFieldAt("Information")->SetStyle("FormattedLabel");

	// Affichage
	quickStartCard.Open();
}

void KWLearningProblemHelpCard::ShowDocumentation()
{
	UICard documentationCard;
	ALString sDocumentationHeader;

	require(GetFieldNumber() == 0);

	// Parametrage basique de la carte
	documentationCard.SetIdentifier("Documentation");
	documentationCard.SetLabel("Documentation " + GetLearningFullApplicationName());

	// Entete
	sDocumentationHeader = "<h3> " + GetLearningFullApplicationName() + "</h3>" + sDocumentationHeader +=
	    "<p> Khiops is an AutoML suite for supervised and unsupervised learning </p>";
	if (GetLearningWebSite() != "")
	{
		sDocumentationHeader += "<p> Official website: ";
		sDocumentationHeader += "<a href=\"\">" + GetLearningWebSite() + "</a> </p>";
	}
	documentationCard.AddStringField("Header", "", "<html>" + sDocumentationHeader + "</html>");
	if (GetLearningWebSite() != "")
	{
		documentationCard.GetFieldAt("Header")->SetStyle("UriLabel");
		documentationCard.GetFieldAt("Header")->SetParameters(GetLearningWebSite());
	}
	else
	{
		documentationCard.GetFieldAt("Header")->SetStyle("FormattedLabel");
	}

	// Contenus
	documentationCard.AddStringField("Body", "", "<html>" + sDocumentationText + "</html>");
	documentationCard.GetFieldAt("Body")->SetStyle("FormattedLabel");

	// Affichage du widget
	documentationCard.Open();
}

void KWLearningProblemHelpCard::ShowAbout()
{
	UICard aboutCard;
	ALString sInformation;
	ALString sCopyrightLabel;

	require(GetFieldNumber() == 0);

	// Titre
	aboutCard.SetIdentifier("About");
	aboutCard.SetLabel("About " + GetLearningFullApplicationName());

	// Formatage html du copyright
	sCopyrightLabel = GetLearningCopyrightLabel();
	assert(sCopyrightLabel.Find("(c)") == 0);
	sCopyrightLabel = " &copy " + GetLearningCopyrightLabel().Right(GetLearningCopyrightLabel().GetLength() - 3);

	// Informations generales
	sInformation = "<html> ";
	sInformation += "<p> <strong> " + GetLearningFullApplicationName() + "</strong> </p> ";
	sInformation += "<p> Version " + GetLearningVersion() + " </p> ";
	sInformation += "<p> </p>";
	sInformation += "<p> </p>";
	sInformation += "<p> </p>";
	sInformation += "<p> " + sCopyrightLabel + " </p> ";

	// Affichage
	sInformation += "</html>";
	aboutCard.AddStringField("Information", "", sInformation);
	aboutCard.GetFieldAt("Information")->SetStyle("FormattedLabel");
	if (GetLearningAboutImage() != "")
		aboutCard.GetFieldAt("Information")->SetParameters(GetLearningAboutImage());

	// Site web
	if (GetLearningWebSite() != "")
	{
		aboutCard.AddStringField("WebSite", "",
					 "<html> <a href=\"\">" + GetLearningWebSite() + "</a>  </html>");
		aboutCard.GetFieldAt("WebSite")->SetStyle("UriLabel");
		aboutCard.GetFieldAt("WebSite")->SetParameters(GetLearningWebSite());
	}

	// Affichage
	aboutCard.Open();
}

void KWLearningProblemHelpCard::SetQuickStartText(const ALString& sValue)
{
	sQuickStartText = sValue;
}

const ALString& KWLearningProblemHelpCard::GetQuickStartText()
{
	return sQuickStartText;
}

void KWLearningProblemHelpCard::SetDocumentationText(const ALString& sValue)
{
	sDocumentationText = sValue;
}

const ALString& KWLearningProblemHelpCard::GetDocumentationText()
{
	return sDocumentationText;
}

ALString KWLearningProblemHelpCard::sQuickStartText;
ALString KWLearningProblemHelpCard::sDocumentationText;
