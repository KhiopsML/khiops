// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "UITest.h"

//////////////////////////////////////////////////////////////////////
// StringList Class

StringList::StringList()
{
	SetIdentifier("stringList");
	SetLabel("Liste de chaines");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");

	// Chaine de caracteres dans TextField
	AddRangedStringField("stringTextField", "Chaine de caracteres dans TextField", "DEFONTAINE", 0, 10);
	GetFieldAt("stringTextField")->SetStyle("TextField");
	GetFieldAt("stringTextField")->SetHelpText("TextField contenant une chaine de caracteres");

	// Chaine de caracteres dans ComboBox
	AddRangedStringField("stringComboBox", "Chaine de caracteres dans ComboBox", "TISON", 0, 10);
	GetFieldAt("stringComboBox")->SetParameters("DEFONTAINE\nBALCOU\nBEZIAU\nTISON\nLE CORNEC\nLE GUEN");
	GetFieldAt("stringComboBox")->SetStyle("ComboBox");
	GetFieldAt("stringComboBox")->SetHelpText("ComboBox contenant une chaine de caracteres");

	// Chaine de caracteres dans EditableComboBox
	AddRangedStringField("stringEditableComboBox", "Chaine de caracteres dans EditableComboBox", "DEFONTAINE", 0,
			     10);
	GetFieldAt("stringEditableComboBox")->SetParameters("DEFONTAINE\nBALCOU\nBEZIAU\nTISON\nLE CORNEC\nLE GUEN");
	GetFieldAt("stringEditableComboBox")->SetStyle("EditableComboBox");
	GetFieldAt("stringEditableComboBox")->SetHelpText("EditableComboBox contenant une chaine de caracteres");

	// Chaine de caracteres dans HelpedComboBox
	AddRangedStringField("stringHelpedComboBox", "Chaine de caracteres dans HelpedComboBox", "DEFONTAINE", 0, 10);
	GetFieldAt("stringHelpedComboBox")->SetParameters("stringCard.stringList:stringComboBox");
	GetFieldAt("stringHelpedComboBox")->SetStyle("HelpedComboBox");
	GetFieldAt("stringHelpedComboBox")->SetHelpText("HelpedComboBox contenant une chaine de caracteres");

	// Chaine de caracteres dans ImageComboBox
	AddStringField("stringImageComboBox", "Chaine de caracteres dans ImageComboBox", "circle");
	GetFieldAt("stringImageComboBox")
	    ->SetParameters("circle\nfilledCircle\nsquare\nfilledSquare\ndiamond\nfilledDiamond\ntriangle\nfilledTriang"
			    "le\ncross\nplus");
	GetFieldAt("stringImageComboBox")->SetStyle("ImageComboBox");
	GetFieldAt("stringImageComboBox")->SetHelpText("ImageComboBox contenant une chaine de caracteres");

	// Chaine de caracteres dans FileChooser
	AddRangedStringField("stringFileChooser", "Chaine de caracteres dans FileChooser", "C:/", 0, 50);
	GetFieldAt("stringFileChooser")->SetStyle("FileChooser");
	GetFieldAt("stringFileChooser")->SetHelpText("FileChooser contenant une chaine de caracteres");

	// Chaine de caracteres dans Password
	AddRangedStringField("stringPassword", "Chaine de caracteres dans Password", "DEFONTAINE", 0, 10);
	GetFieldAt("stringPassword")->SetStyle("Password");
	GetFieldAt("stringPassword")->SetHelpText("Password contenant une chaine de caracteres");

	SetErgonomy(Inspectable);

	// On affecte une fiche d'edition personnalisee
	UICard* customView = new UICard();
	customView->SetLabel("Update StringList");
	customView->AddStringField("stringTextField", "Chaine de caracteres dans TextField", "");
	customView->AddStringField("stringEditableComboBox", "Chaine de caracteres dans EditableComboBox", "");
	customView->AddStringField("stringFileChooser", "Chaine de caracteres dans FileChooser", "");
	customView->GetFieldAt("stringFileChooser")->SetStyle("FileChooser");
	SetItemCard(customView);

	AddItem();
	AddItem();

	GetActionAt("InsertItemBefore")->SetAccelKey("control I");
	GetActionAt("RemoveItem")->SetAccelKey("control R");
}

StringList::~StringList() {}

//////////////////////////////////////////////////////////////////////
// StringCard Class

StringCard::StringCard()
{
	SetIdentifier("stringCard");
	SetLabel("Fiche de chaines");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");
	SetShortCut('c');

	// Chaine de caracteres dans TextField
	AddRangedStringField("stringTextField", "Chaine de caracteres dans TextField", "DEFONTAINE", 0, 10);
	GetFieldAt("stringTextField")->SetStyle("TextField");
	GetFieldAt("stringTextField")->SetHelpText("TextField contenant une chaine de caracteres");

	// Chaine de caracteres dans TextArea
	AddRangedStringField("stringTextArea", "Chaine de caracteres dans StringTextArea", "commentaires", 0, 100);
	GetFieldAt("stringTextArea")->SetStyle("TextArea");
	GetFieldAt("stringTextArea")->SetHelpText("TextArea contenant une chaine de caracteres");

	// Chaine de caracteres dans ComboBox
	AddRangedStringField("stringComboBox", "Chaine de caracteres dans ComboBox", "TISON", 0, 10);
	GetFieldAt("stringComboBox")->SetParameters("DEFONTAINE\nBALCOU\nBEZIAU\nTISON\nLE CORNEC\nLE GUEN");
	GetFieldAt("stringComboBox")->SetStyle("ComboBox");
	GetFieldAt("stringComboBox")->SetHelpText("ComboBox contenant une chaine de caracteres");

	// Chaine de caracteres dans RadioButton
	AddRangedStringField("stringRadioButton", "Chaine de caracteres dans RadioButton", "TISON", 0, 10);
	GetFieldAt("stringRadioButton")->SetParameters("DEFONTAINE\nBALCOU\nBEZIAU\nTISON\nLE CORNEC\nLE GUEN");
	GetFieldAt("stringRadioButton")->SetStyle("RadioButton");
	GetFieldAt("stringRadioButton")->SetHelpText("RadioButton contenant une chaine de caracteres");

	// Chaine de caracteres dans EditableComboBox
	AddRangedStringField("stringEditableComboBox", "Chaine de caracteres dans EditableComboBox", "DEFONTAINE", 0,
			     10);
	GetFieldAt("stringEditableComboBox")->SetParameters("DEFONTAINE\nBALCOU\nBEZIAU\nTISON\nLE CORNEC\nLE GUEN");
	GetFieldAt("stringEditableComboBox")->SetStyle("EditableComboBox");
	GetFieldAt("stringEditableComboBox")->SetHelpText("EditableComboBox contenant une chaine de caracteres");

	// Chaine de caracteres dans HelpedComboBox
	AddRangedStringField("stringHelpedComboBox", "Chaine de caracteres dans EditableComboBox", "", 0, 10);
	GetFieldAt("stringHelpedComboBox")->SetParameters("stringCard.stringList:stringImageComboBox");
	GetFieldAt("stringHelpedComboBox")->SetStyle("HelpedComboBox");
	GetFieldAt("stringHelpedComboBox")->SetHelpText("HelpedComboBox contenant une chaine de caracteres");

	// Chaine de caracteres dans ImageComboBox
	AddStringField("stringImageComboBox", "Chaine de caracteres dans ImageComboBox", "circle");
	GetFieldAt("stringImageComboBox")
	    ->SetParameters("circle\nfilledCircle\nsquare\nfilledSquare\ndiamond\nfilledDiamond\ntriangle\nfilledTriang"
			    "le\ncross\nplus");
	GetFieldAt("stringImageComboBox")->SetStyle("ImageComboBox");
	GetFieldAt("stringImageComboBox")->SetHelpText("Password contenant une chaine de caracteres");

	// Chaine de caracteres dans FileChooser
	AddRangedStringField("stringFileChooser", "Chaine de caracteres dans FileChooser", "C:/", 0, 50);
	GetFieldAt("stringFileChooser")->SetStyle("FileChooser");
	GetFieldAt("stringFileChooser")->SetHelpText("FileChooser contenant une chaine de caracteres");

	// Chaine de caracteres dans Password
	AddRangedStringField("stringPassword", "Chaine de caracteres dans Password", "DEFONTAINE", 0, 10);
	GetFieldAt("stringPassword")->SetStyle("Password");
	GetFieldAt("stringPassword")->SetHelpText("Password contenant une chaine de caracteres");

	StringList* stringList = new StringList();
	AddListField("stringList", "sous liste string", stringList);
}

StringCard::~StringCard() {}

//////////////////////////////////////////////////////////////////////
// TextAreaCard Class

TextAreaCard::TextAreaCard()
{
	SetIdentifier("textAreaCard");
	SetLabel("Fiche de textes");

	// Chaine de caracteres dans TextArea
	AddRangedStringField("FirstText", "Text area", "commentaires", 0, 1000);
	GetFieldAt("FirstText")->SetParameters("8\n50");
	GetFieldAt("FirstText")->SetStyle("TextArea");
	GetFieldAt("FirstText")->SetHelpText("TextArea contenant une chaine de caracteres");

	// Chaine de caracteres dans TextArea
	AddRangedStringField("SecondText", "", "commentaires", 0, 1000);
	GetFieldAt("SecondText")->SetEditable(false);
	GetFieldAt("SecondText")->SetParameters("5\n50");
	GetFieldAt("SecondText")->SetStyle("TextArea");
}

TextAreaCard::~TextAreaCard() {}

void TextAreaCard::Test()
{
	TextAreaCard textAreaCard;
	ALString sText;

	// Initialisation d'un texte
	sText = "References guides\n"
		" Khiops: KhiopsGuide.pdf\n"
		" Khiops visualization: KhiopsVisualizationGuide.pdf\n"
		" Khiops coclustering: KhiopsCoclusteringGuide.pdf\n"
		" Khiops covisualization: KhiopsCovisualizationGuide.pdf\n"
		" Tutorial: KhiopsTutorial.pdf";

	// Parametrage de la boite et ouverture
	UIObject::SetUIMode(UIObject::Graphic);
	textAreaCard.SetStringValueAt("FirstText", sText);
	textAreaCard.SetStringValueAt("SecondText", "NON EDITABLE\n" + sText);
	textAreaCard.Open();
}

//////////////////////////////////////////////////////////////////////
// FormattedLabelCard Class

FormattedLabelCard::FormattedLabelCard()
{
	SetIdentifier("textAreaCard");
	SetLabel("Fiche de textes");

	// Chaine de caracteres dans FormattedLabel
	AddStringField("FirstText", "Formatted label", "");
	GetFieldAt("FirstText")->SetStyle("FormattedLabel");
	GetFieldAt("FirstText")->SetHelpText("FormattedLabel contenant une chaine de caracteres");

	// Chaine de caracteres dans FormattedLabel
	AddStringField("SecondText", "", "");
	GetFieldAt("SecondText")->SetStyle("FormattedLabel");
}

FormattedLabelCard::~FormattedLabelCard() {}

void FormattedLabelCard::Test()
{
	FormattedLabelCard textAreaCard;
	ALString sText;

	// Initialisation d'un texte
	sText = "<html>\n"
		"<h2> References guides </h2> <p>\n"
		"<ul>\n"
		"<li> Khiops: KhiopsGuide.pdf\n"
		"<li> Khiops visualization: KhiopsVisualizationGuide.pdf\n"
		"<li> Khiops coclustering: KhiopsCoclusteringGuide.pdf\n"
		"<li> Khiops covisualization: KhiopsCovisualizationGuide.pdf\n"
		"<li> Tutorial: KhiopsTutorial.pdf\n"
		"</ul>\n"
		"<h3> See doc directory under Khiops installation directory. </h3> <p>\n"
		"<a href=\"http://khiops.org\">Khiops (c) Orange software for data mining</a> \n"
		"</html>";

	// Parametrage de la boite et ouverture
	UIObject::SetUIMode(UIObject::Graphic);
	textAreaCard.SetStringValueAt("FirstText", sText);
	textAreaCard.SetStringValueAt("SecondText", sText);
	textAreaCard.Open();
}

//////////////////////////////////////////////////////////////////////
// LabelCard Class

LabelCard::LabelCard()
{
	SetIdentifier("textAreaCard");
	SetLabel("Fiche de textes");

	// Chaine de caracteres dans SelectableLabel
	AddStringField("FirstText", "Selectable label", "");
	GetFieldAt("FirstText")->SetStyle("SelectableLabel");
	GetFieldAt("FirstText")->SetHelpText("SelectableLabel contenant une chaine de caracteres");

	// Chaine de caracteres dans FormattedLabel
	AddStringField("SecondText", "Formatted label", "");
	GetFieldAt("SecondText")->SetStyle("FormattedLabel");
	GetFieldAt("SecondText")->SetParameters("images/sample2.png");
	GetFieldAt("SecondText")->SetHelpText("FormattedLabel contenant une chaine de caracteres");

	// Chaine de caracteres dans FormattedLabel, sans titre
	AddStringField("ThirdText", "", "");
	GetFieldAt("ThirdText")->SetStyle("FormattedLabel");

	// Chaine de caracteres dans UriLabel
	AddStringField("FourthText", "", "");
	GetFieldAt("FourthText")->SetStyle("UriLabel");
	GetFieldAt("FourthText")->SetParameters("khiops.org");
}

LabelCard::~LabelCard() {}

void LabelCard::Test()
{
	LabelCard textAreaCard;
	ALString sText;

	// Initialisation d'un texte
	sText = "<h2> References guides </h2> <p>\n"
		"<ul>\n"
		"<li> Khiops: KhiopsGuide.pdf\n"
		"<li> Khiops visualization: KhiopsVisualizationGuide.pdf\n"
		"<li> Khiops coclustering: KhiopsCoclusteringGuide.pdf\n"
		"<li> Khiops covisualization: KhiopsCovisualizationGuide.pdf\n"
		"<li> Tutorial: KhiopsTutorial.pdf\n"
		"</ul>\n"
		"<h3> See doc directory under Khiops installation directory. </h3> <p>\n"
		"<a href=\"http://khiops.org\">Khiops (c) Orange software for data mining</a> \n";

	// Parametrage de la boite et ouverture
	UIObject::SetUIMode(UIObject::Graphic);
	textAreaCard.SetStringValueAt("FirstText", "<html>\n"
						   "<h1 > Selectable title </h1>"
						   "</html>\n");
	textAreaCard.SetStringValueAt("SecondText", "<html>\n"
						    "<h1 > Main title </h1>"
						    "<h1> </h1> <h1> </h1> <h1> </h1> <h1> </h1> " +
							sText + "</html>\n");
	textAreaCard.SetStringValueAt("ThirdText", "<html>\n"
						   "<h3 > Copyright Orange 2021 </h3>"
						   "</html>\n");
	textAreaCard.SetStringValueAt("FourthText", "<html> <h4> Web site <a href=\"\">khiops</a> </h4> </html>");
	textAreaCard.Open();
}

//////////////////////////////////////////////////////////////////////
// CharList Class

CharList::CharList()
{
	SetIdentifier("charList");
	SetLabel("Liste de caracteres");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");

	// Caractere dans TextField
	AddCharField("charTextField", "Caractere dans TextField", 'M');
	GetFieldAt("charTextField")->SetStyle("TextField");
	GetFieldAt("charTextField")->SetHelpText("TextField contenant un caractere");

	// Caractere dans ComboBox
	AddCharField("charComboBox", "Caractere dans ComboBox", 'M');
	GetFieldAt("charComboBox")->SetParameters("A\nB\nC\nD\nE\nF\nG\nH\nI\nJ\nK\nL");
	GetFieldAt("charComboBox")->SetStyle("ComboBox");
	GetFieldAt("charComboBox")->SetHelpText("ComboBox contenant un caractere");

	// Caractere dans EditableComboBox
	AddCharField("charEditableComboBox", "Caractere dans EditableComboBox", 'M');
	GetFieldAt("charEditableComboBox")->SetParameters("M\nF");
	GetFieldAt("charEditableComboBox")->SetStyle("EditableComboBox");
	GetFieldAt("charEditableComboBox")->SetHelpText("EditableComboBox contenant un caractere");

	AddItem();
	AddItem();

	// On ne selectionne aucun item
	SetSelectedItemIndex(-1);
}

CharList::~CharList() {}

//////////////////////////////////////////////////////////////////////
// CharCard Class

CharCard::CharCard()
{
	SetIdentifier("charCard");
	SetLabel("Fiche de caracteres");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");
	SetShortCut('a');

	// Caractere dans TextField
	AddCharField("charTextField", "Caractere dans TextField", 'M');
	GetFieldAt("charTextField")->SetStyle("TextField");
	GetFieldAt("charTextField")->SetHelpText("TextField contenant un caractere");

	// Caractere dans ComboBox
	AddCharField("charComboBox", "Caractere dans ComboBox", 'A');
	GetFieldAt("charComboBox")->SetParameters("A\nB\nC\nD\nE\nF\nG\nH\nI\nJ\nK\nL");
	GetFieldAt("charComboBox")->SetStyle("ComboBox");
	GetFieldAt("charComboBox")->SetHelpText("ComboBox contenant un caractere");

	// Caractere dans RadioButton
	AddCharField("charRadioButton", "Caractere dans RadioButton", 'M');
	GetFieldAt("charRadioButton")->SetParameters("M\nF");
	GetFieldAt("charRadioButton")->SetStyle("RadioButton");
	GetFieldAt("charRadioButton")->SetHelpText("RadioButton contenant un caractere");

	// Caractere dans EditableComboBox
	AddCharField("charEditableComboBox", "Caractere dans EditableComboBox", 'M');
	GetFieldAt("charEditableComboBox")->SetParameters("M\nF");
	GetFieldAt("charEditableComboBox")->SetStyle("EditableComboBox");
	GetFieldAt("charEditableComboBox")->SetHelpText("EditableComboBox contenant un caractere");

	CharList* charList = new CharList();
	AddListField("charList", "sous liste char", charList);
}

CharCard::~CharCard() {}

//////////////////////////////////////////////////////////////////////
// BooleanList Class

BooleanList::BooleanList()
{
	SetIdentifier("booleanList");
	SetLabel("Liste de booleens");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");

	// Booleen dans TextField
	AddBooleanField("booleanTextField", "Booleen dans TextField", false);
	GetFieldAt("booleanTextField")->SetStyle("TextField");
	GetFieldAt("booleanTextField")->SetHelpText("TextField contenant un booleen");

	// Booleen dans ComboBox
	AddBooleanField("booleanComboBox", "Booleen dans ComboBox", false);
	GetFieldAt("booleanComboBox")->SetStyle("ComboBox");
	GetFieldAt("booleanComboBox")->SetHelpText("ComboBox contenant un booleen");

	// Booleen dans CheckBox
	AddBooleanField("booleanCheckBox", "Booleen dans CheckBox", false);
	GetFieldAt("booleanCheckBox")->SetStyle("CheckBox");
	GetFieldAt("booleanCheckBox")->SetHelpText("CheckBox contenant un booleen");

	AddItem();
	AddItem();
}

BooleanList::~BooleanList() {}

//////////////////////////////////////////////////////////////////////
// BooleanCard Class

BooleanCard::BooleanCard()
{
	SetIdentifier("booleanCard");
	SetLabel("Fiche de booleens");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");
	SetShortCut('b');

	// Booleen dans TextField
	AddBooleanField("booleanTextField", "Booleen dans TextField", false);
	GetFieldAt("booleanTextField")->SetStyle("TextField");
	GetFieldAt("booleanTextField")->SetHelpText("TextField contenant un booleen");

	// Booleen dans ComboBox
	AddBooleanField("booleanComboBox", "Booleen dans ComboBox", false);
	GetFieldAt("booleanComboBox")->SetStyle("ComboBox");
	GetFieldAt("booleanComboBox")->SetHelpText("ComboBox contenant un booleen");

	// Booleen dans RadioButton
	AddBooleanField("booleanRadioButton", "Booleen dans RadioButton", false);
	GetFieldAt("booleanRadioButton")->SetStyle("RadioButton");
	GetFieldAt("booleanRadioButton")->SetHelpText("RadioButton contenant un booleen");

	// Booleen dans CheckBox
	AddBooleanField("booleanCheckBox", "Booleen dans CheckBox", false);
	GetFieldAt("booleanCheckBox")->SetStyle("CheckBox");
	GetFieldAt("booleanCheckBox")->SetHelpText("CheckBox contenant un booleen");

	BooleanList* booleanList = new BooleanList();
	AddListField("booleanList", "sous liste boolean", booleanList);
}

BooleanCard::~BooleanCard() {}

//////////////////////////////////////////////////////////////////////
// IntList Class

IntList::IntList()
{
	SetIdentifier("intList");
	SetLabel("Liste d'entiers");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");

	// Entier dans TextField
	AddRangedIntField("intTextField", "Entier dans TextField", 30, 0, 100);
	GetFieldAt("intTextField")->SetStyle("TextField");
	GetFieldAt("intTextField")->SetHelpText("TextField contenant un entier");

	// Entier dans ComboBox
	AddRangedIntField("intComboBox", "Entier dans ComboBox", 30, 0, 100);
	GetFieldAt("intComboBox")
	    ->SetParameters("0\n5\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55\n60\n65\n70\n75\n80\n85\n90\n95\n100");
	GetFieldAt("intComboBox")->SetStyle("ComboBox");
	GetFieldAt("intComboBox")->SetHelpText("ComboBox contenant un entier");

	// Entier dans EditableComboBox
	AddRangedIntField("intEditableComboBox", "Entier dans EditableComboBox", 30, 0, 100);
	GetFieldAt("intEditableComboBox")
	    ->SetParameters("0\n5\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55\n60\n65\n70\n75\n80\n85\n90\n95\n100");
	GetFieldAt("intEditableComboBox")->SetStyle("EditableComboBox");
	GetFieldAt("intEditableComboBox")->SetHelpText("EditableComboBox contenant un entier");

	// Entier dans Spinner
	AddRangedIntField("intSpinner", "Entier dans Spinner", 30, 0, 100);
	GetFieldAt("intSpinner")->SetStyle("Spinner");
	GetFieldAt("intSpinner")->SetHelpText("Spinner contenant un entier");

	// SetErgonomy(Inspectable);

	AddItem();
	AddItem();
	AddItem();
	AddItem();
}

IntList::~IntList() {}

//////////////////////////////////////////////////////////////////////
// IntCard Class

IntCard::IntCard()
{
	SetIdentifier("intCard");
	SetLabel("Fiche d'entiers");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");
	SetStyle("TabbedPanes");
	SetShortCut('e');

	// Entier dans TextField
	AddRangedIntField("intTextField", "Entier dans TextField", 30, 0, 100);
	GetFieldAt("intTextField")->SetStyle("TextField");
	GetFieldAt("intTextField")->SetHelpText("TextField contenant un entier");

	// Entier dans ComboBox
	AddRangedIntField("intComboBox", "Entier dans ComboBox", 30, 0, 100);
	GetFieldAt("intComboBox")
	    ->SetParameters("0\n5\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55\n60\n65\n70\n75\n80\n85\n90\n95\n100");
	GetFieldAt("intComboBox")->SetStyle("ComboBox");
	GetFieldAt("intComboBox")->SetHelpText("ComboBox contenant un entier");

	// Entier dans RadioButton
	AddRangedIntField("intRadioButton", "Entier dans RadioButton", 30, 0, 100);
	GetFieldAt("intRadioButton")->SetParameters("0\n10\n20\n30\n40\n50\n60\n70\n80\n90\n100");
	GetFieldAt("intRadioButton")->SetStyle("RadioButton");
	GetFieldAt("intRadioButton")->SetHelpText("RadioButton contenant un entier");

	// Entier dans Slider
	AddRangedIntField("intSlider", "Entier dans Slider", 0, 0, 100);
	GetFieldAt("intSlider")->SetStyle("Slider");
	GetFieldAt("intSlider")->SetHelpText("Slider contenant un entier");

	// Entier dans Spinner
	AddRangedIntField("intSpinner", "Entier dans Spinner", 10, 0, 100);
	GetFieldAt("intSpinner")->SetStyle("Spinner");
	GetFieldAt("intSpinner")->SetHelpText("Spinner contenant un entier");

	// Entier dans EditableComboBox
	AddRangedIntField("intEditableComboBox", "Entier dans EditableComboBox", 30, 0, 100);
	GetFieldAt("intEditableComboBox")
	    ->SetParameters("0\n5\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55\n60\n65\n70\n75\n80\n85\n90\n95\n100");
	GetFieldAt("intEditableComboBox")->SetStyle("EditableComboBox");
	GetFieldAt("intEditableComboBox")->SetHelpText("EditableComboBox contenant un entier");

	IntList* intList = new IntList();
	AddListField("intList", "sous liste int", intList);

	StringCard* stringCard = new StringCard();
	AddCardField("stringCard", "sous fiche string", stringCard);
}

IntCard::~IntCard() {}

//////////////////////////////////////////////////////////////////////
// DoubleList Class

DoubleList::DoubleList()
{
	SetIdentifier("doubleList");
	SetLabel("Liste de reels");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");

	// Reel dans TextField
	AddRangedDoubleField("doubleTextField", "Reel dans TextField", 1.75, 1.00, 2.00);
	GetFieldAt("doubleTextField")->SetStyle("TextField");
	GetFieldAt("doubleTextField")->SetHelpText("TextField contenant un reel");

	// Reel dans ComboBox
	AddRangedDoubleField("doubleComboBox", "Reel dans ComboBox", 1.75, 1.00, 2.00);
	GetFieldAt("doubleComboBox")->SetParameters("1.50\n1.55\n1.60\n1.65\n1.70\n1.75\n1.80\n1.85\n1.90\n1.95\n2.00");
	GetFieldAt("doubleComboBox")->SetStyle("ComboBox");
	GetFieldAt("doubleComboBox")->SetHelpText("ComboBox contenant un reel");

	// Reel dans EditableComboBox
	AddRangedDoubleField("doubleEditableComboBox", "Reel dans EditableComboBox", 1.75, 1.00, 2.00);
	GetFieldAt("doubleEditableComboBox")
	    ->SetParameters("1.50\n1.55\n1.60\n1.65\n1.70\n1.75\n1.80\n1.85\n1.90\n1.95\n2.00");
	GetFieldAt("doubleEditableComboBox")->SetStyle("EditableComboBox");
	GetFieldAt("doubleEditableComboBox")->SetHelpText("EditableComboBox contenant un reel");

	// Reel dans Spinner
	AddRangedDoubleField("doubleSpinner", "Reel dans Spinner", 1.75, 1.00, 2.00);
	GetFieldAt("doubleSpinner")->SetParameters("1");
	GetFieldAt("doubleSpinner")->SetStyle("Spinner");
	GetFieldAt("doubleSpinner")->SetHelpText("Spinner contenant un reel");

	AddItem();
	AddItem();
}

DoubleList::~DoubleList() {}

//////////////////////////////////////////////////////////////////////
// DoubleCard Class

DoubleCard::DoubleCard()
{
	SetIdentifier("doubleCard");
	SetLabel("Fiche de reels");
	SetHelpText("Test d'une interface liste elementaire, sans synchronisation automatique");
	SetStyle("TabbedPanes");
	SetShortCut('r');

	// Reel dans TextField
	AddRangedDoubleField("doubleTextField", "Reel dans TextField", 1.75, 1.00, 2.00);
	GetFieldAt("doubleTextField")->SetStyle("TextField");
	GetFieldAt("doubleTextField")->SetHelpText("TextField contenant un reel");

	// Reel dans ComboBox
	AddRangedDoubleField("doubleComboBox", "Reel dans ComboBox", 1.75, 1.00, 2.00);
	GetFieldAt("doubleComboBox")->SetParameters("1.50\n1.55\n1.60\n1.65\n1.70\n1.75\n1.80\n1.85\n1.90\n1.95\n2.00");
	GetFieldAt("doubleComboBox")->SetStyle("ComboBox");
	GetFieldAt("doubleComboBox")->SetHelpText("ComboBox contenant un reel");

	// Reel dans RadioButton
	AddRangedDoubleField("doubleRadioButton", "Reel dans RadioButton", 1.70, 1.00, 2.00);
	GetFieldAt("doubleRadioButton")->SetParameters("1.50\n1.60\n1.70\n1.80\n1.90\n2.00");
	GetFieldAt("doubleRadioButton")->SetStyle("RadioButton");
	GetFieldAt("doubleRadioButton")->SetHelpText("RadioButton contenant un reel");

	// Reel dans EditableComboBox
	AddRangedDoubleField("doubleEditableComboBox", "Reel dans EditableComboBox", 1.75, 1.00, 2.00);
	GetFieldAt("doubleEditableComboBox")
	    ->SetParameters("1.50\n1.55\n1.60\n1.65\n1.70\n1.75\n1.80\n1.85\n1.90\n1.95\n2.00");
	GetFieldAt("doubleEditableComboBox")->SetStyle("EditableComboBox");
	GetFieldAt("doubleEditableComboBox")->SetHelpText("EditableComboBox contenant un reel");

	// Reel dans Spinner
	AddRangedDoubleField("doubleSpinner", "Reel dans Spinner", 1.75, 1.00, 2.00);
	GetFieldAt("doubleSpinner")->SetParameters("1");
	GetFieldAt("doubleSpinner")->SetStyle("Spinner");
	GetFieldAt("doubleSpinner")->SetHelpText("Spinner contenant un reel");

	DoubleList* doubleList = new DoubleList();
	AddListField("doubleList", "sous liste double", doubleList);

	StringCard* stringCard = new StringCard();
	AddCardField("stringCard", "sous fiche string", stringCard);

	IntCard* intCard = new IntCard();
	AddCardField("ic", "sous fiche int", intCard);

	AddAction("Action1", "Action 1", (ActionMethod)(&UITest::DisplaySampleObject));
	GetActionAt("Action1")->SetStyle("Button");
	GetActionAt("Action1")->SetHelpText("Aide sur l'action Action1");

	AddAction("Action2", "Action 2", (ActionMethod)(&UITest::DisplaySampleObject));
	GetActionAt("Action2")->SetStyle("Button");
	GetActionAt("Action2")->SetHelpText("Aide sur l'action Action2");

	AddAction("Action3", "Action 3", (ActionMethod)(&UITest::DisplaySampleObject));
	GetActionAt("Action3")->SetStyle("Button");
	GetActionAt("Action3")->SetHelpText("Aide sur l'action Action3");
}

DoubleCard::~DoubleCard() {}

//////////////////////////////////////////////////////////////////////
// UITest Class

UITest::UITest()
{
	SetIdentifier("PersonManagement");
	SetLabel("Gestion des personnes");
	SetHelpText("Tests d'interface utilisateur");
	SetStyle("TabbedPanes");
	SetShortCut('g');

	StringCard* stringCard = new StringCard();
	AddCardField("stringCard", "Fiche de chaines de caracteres", stringCard);

	CharCard* charCard = new CharCard();
	AddCardField("charCard", "Fiche de caracteres", charCard);

	BooleanCard* booleanCard = new BooleanCard();
	AddCardField("booleanCard", "Fiche de booleens", booleanCard);

	IntCard* intCard = new IntCard();
	AddCardField("intCard", "Fiche d'entiers", intCard);

	DoubleCard* doubleCard = new DoubleCard();
	AddCardField("doubleCard", "Fiche de reels", doubleCard);

	// Action de gestion d'un objet
	AddAction("DisplaySampleObject", "Gestion d'un SampleObject", (ActionMethod)(&UITest::DisplaySampleObject));
	GetActionAt("DisplaySampleObject")->SetStyle("Button");
	GetActionAt("DisplaySampleObject")->SetHelpText("Aide sur l'action DisplaySampleObject");
	GetActionAt("DisplaySampleObject")->SetAccelKey("control shift J");
	GetActionAt("DisplaySampleObject")->SetShortCut('o');

	// Action de gestion d'un tableau d'objet
	AddAction("DisplaySampleObjectArray", "Gestion d'un tableau de SampleObjects",
		  (ActionMethod)(&UITest::DisplaySampleObjectArray));
	GetActionAt("DisplaySampleObjectArray")->SetStyle("Button");
	GetActionAt("DisplaySampleObjectArray")->SetHelpText("Aide sur l'action DisplaySampleObjectArray");
	GetActionAt("DisplaySampleObjectArray")->SetAccelKey("control K");
	GetActionAt("DisplaySampleObjectArray")->SetShortCut('t');

	// Action de test de la barre de progression
	AddAction("ProgressionTest", "Test barre de progression", (ActionMethod)(&UITest::ProgressionTest));
	GetActionAt("ProgressionTest")->SetStyle("Button");
	GetActionAt("ProgressionTest")->SetHelpText("Test de la barre de progression");
	GetActionAt("ProgressionTest")->SetShortCut('b');

	// Idem sous forme de menu
	AddAction("ProgressionTestMenu", "Test barre de progression", (ActionMethod)(&UITest::ProgressionTest));
	GetActionAt("ProgressionTestMenu")->SetHelpText("Test de la barre de progression");
}

UITest::~UITest()
{
	oaSampleObjects.DeleteAll();
}

void UITest::ProgressionTest()
{
	TaskProgression::Test();
}

void UITest::DisplaySampleObject()
{
	SampleObjectView view;

	// On caracterise la vue logique avec l'objet metier
	view.SetObject(&aSampleObject);

	// Ouverture de la fiche d'edition du SampleObject
	view.Open();

	// Mise a jour de l'objet
	view.GetObject();
}

void UITest::DisplaySampleObjectArray()
{
	SampleObjectArrayView view;

	// Personnalisation de la fiche de mise a jour utilisee
	view.SetItemView(new SampleObjectView);

	// Ouverture de la liste d'edition des SampleObjects
	view.SetObjectArray(&oaSampleObjects);
	view.Open();
}

////////////////////////////////////////////////////////////////////////
// Classe FileReaderCard

#if defined _MSC_VER || defined __MSVCRT_VERSION__
const ALString FileReaderCard::sRootPath = "C:/temp";
#else
const ALString FileReaderCard::sRootPath = "/temp";
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4310) // disable C4310 warning("le cast tronque la valeur constante")
#endif

FileReaderCard::FileReaderCard()
{
	SetIdentifier("FileReaderCard");
	SetLabel("Lecture de fichier");

	// Nom du fichier
	AddStringField("FileName", "FileName",
		       sRootPath + "/TestStressedChars(" + char(232) + ")/_233_" + char(233) + "_.txt");
	GetFieldAt("FileName")->SetStyle("FileChooser");

	// Action de creation des fichiers pour tous les caracteres ANSI
	AddAction("OpenFile", "Open file", (ActionMethod)(&FileReaderCard::OpenFile));
	GetActionAt("OpenFile")->SetStyle("Button");

	// Action de creation des fichiers pour tous les caracteres ANSI
	AddAction("CreateFiles", "Create files", (ActionMethod)(&FileReaderCard::CreateFiles));
	GetActionAt("CreateFiles")->SetStyle("Button");

	// Action de lecture du fichier
	AddAction("TestFileServices", "Test file services", (ActionMethod)(&FileReaderCard::TestFileServices));
	GetActionAt("TestFileServices")->SetStyle("Button");
}

FileReaderCard::~FileReaderCard() {}

void FileReaderCard::OpenFile()
{
	boolean bFileChooserMode = true;
	UIFileChooserCard fileChooserCard;
	UIConfirmationCard confirmationCard;
	boolean bContinue;
	ALString sFileName;
	ALString sOutputFileName;

	// Gestion par une fiche de type FileChooser
	if (bFileChooserMode)
	{
		sFileName = GetStringValueAt("FileName");
		fileChooserCard.SetApproveActionHelpText("Open dictionary that will be loaded into memory");
		sOutputFileName = fileChooserCard.ChooseFile("File via FileChooser", "Open dictionary", "FileChooser",
							     "Text\ntxt\ndat", "ChoosenFile", "File name", sFileName);
		if (sOutputFileName != "" and sOutputFileName != sFileName)
		{
			SetStringValueAt("FileName", sOutputFileName);
		}
	}
	// Gestion par une fiche de type Confirmation
	else
	{
		// Parametrage de la boite d'ouverture
		confirmationCard.SetLabel("Open");
		confirmationCard.GetActionAt("OK")->SetLabel("Open");
		confirmationCard.GetActionAt("Exit")->SetLabel("Cancel");

		// Ajout d'un champ de saisie du fichier des dictionnaires
		confirmationCard.AddStringField("FileName", "File", "");
		confirmationCard.GetFieldAt("FileName")->SetStyle("FileChooser");
		confirmationCard.GetFieldAt("FileName")->SetParameters("Text\ntxt\ndat");

		// On initialise
		sFileName = GetStringValueAt("FileName");
		confirmationCard.SetStringValueAt("FileName", sFileName);

		// Affichage du nom de fichier choisi
		bContinue = confirmationCard.OpenAndConfirm();
		if (bContinue and confirmationCard.GetStringValueAt("FileName") != sFileName)
		{
			sFileName = confirmationCard.GetStringValueAt("FileName");
			SetStringValueAt("FileName", sFileName);
		}
	}

	// Test d'existence du fichier
	sFileName = GetStringValueAt("FileName");
	AddSimpleMessage("Is file " + sFileName + ": " + BooleanToString(FileService::FileExists(sFileName)));
}

void FileReaderCard::CreateFiles()
{
	int i;
	char c;
	ALString sRootDir;
	ALString sFileName;
	ALString sTmp;
	StringVector svDirectoryNames;
	StringVector svFileNames;
	boolean bOk;

	// Creation de fichiers avec tous les caracteres ANSI possibles
	sRootDir = sRootPath + "/TestStressedChars(" + char(232) + ")";
	FileService::MakeDirectory(sRootDir);
	for (i = 0; i < 256; i++)
	{
		c = (char)i;
		sFileName = sTmp + '_' + IntToString(i) + "_" + c + "_.txt";
		bOk = FileService::CreateEmptyFile(FileService::BuildFilePathName(sRootDir, sFileName));
		AddMessage(sFileName + " " + c + " (" + IntToString(i) + "): " + BooleanToString(bOk));
	}

	// Creation d'un fichier avec un caractere UTF_8 special (GREEK CAPITAL LETTER THETA (U+0398) ceb8)
	sFileName = sTmp + "_theta" + "_" + (char)0xce + (char)0xb8 + "_.txt";
	bOk = FileService::CreateEmptyFile(FileService::BuildFilePathName(sRootDir, sFileName));
	AddMessage(sFileName + " " + BooleanToString(bOk));

	// Creation d'un fichier avec un caractere e accent aigu code UTF8l (c3a9)
	sFileName = sTmp + "_233_utf8" + "_" + (char)0xc3 + (char)0xa9 + "_.txt";
	bOk = FileService::CreateEmptyFile(FileService::BuildFilePathName(sRootDir, sFileName));
	AddMessage(sFileName + " " + BooleanToString(bOk));

	// Parcours des fichiers existants
	bOk = FileService::GetDirectoryContent(sRootDir, &svDirectoryNames, &svFileNames);
	for (i = 0; i < svFileNames.GetSize(); i++)
	{
		sFileName = svFileNames.GetAt(i);
		bOk = FileService::FileExists(FileService::BuildFilePathName(sRootDir, sFileName));
		AddMessage("-> " + sFileName + " " + IntToString(sFileName.GetLength()) + " " + BooleanToString(bOk));
		if (i > svFileNames.GetSize() - 2 or sFileName.Find("233") >= 0)
		{
			int j;
			cout << "-> " + sTmp + " " + IntToString(sFileName.GetLength()) + " " + BooleanToString(bOk)
			     << endl;
			for (j = 0; j < sFileName.GetLength(); j++)
			{
				cout << "\t" << j << "\t" << int(sFileName.GetAt(j)) << endl;
				AddMessage(sTmp + "\t" + IntToString(j) + "\t" + sFileName.GetAt(j) + "\t" +
					   IntToString(int(sFileName.GetAt(j))));
			}
		}
	}
}

void FileReaderCard::TestFileServices()
{
	ALString sFileName;
	ALString sDirName;
	fstream fst;
	FILE* fFile;
	boolean bOk;
	longint lFileSize;
	longint lDiskGetFreeSpace;

	////////////////////////////////////////////////////////////////////////////
	// Test des services de fichiers avec des caracteres accentues
	// Effectuer le test avec le fichier _232_e_.txt (e accent grave)

	// Acces au nom du fichier
	sFileName = GetStringValueAt("FileName");
	AddMessage("Test des services pour le fichier " + sFileName);

	// Existence du fichier
	bOk = FileService::FileExists(sFileName);
	AddMessage("Existence de " + sFileName + ": " + IntToString(bOk) + " " +
		   FileService::GetLastSystemIOErrorMessage());

	// Taille du fichier
	lFileSize = FileService::GetFileSize(sFileName);
	AddMessage("Taille de " + sFileName + ": " + LongintToString(lFileSize));

	// Taille disponible sur le repertoire
	sDirName = FileService::GetPathName(sFileName);
	lDiskGetFreeSpace = DiskGetFreeSpace(sDirName);
	AddMessage("Taille disponible sur le disque de " + sDirName + ": " + LongintToString(lDiskGetFreeSpace));

	// Ouverture/fermeture du fichier
	bOk = FileService::OpenInputFile(sFileName, fst);
	if (bOk)
		FileService::CloseInputFile(sFileName, fst);
	AddMessage("Ouverture de " + sFileName + ": " + IntToString(bOk) + " " +
		   FileService::GetLastSystemIOErrorMessage());

	// Ouverture/fermeture du fichier en binaire
	bOk = FileService::OpenInputBinaryFile(sFileName, fFile);
	if (bOk)
		FileService::CloseInputBinaryFile(sFileName, fFile);
	AddMessage("Ouverture en binaire de " + sFileName + ": " + IntToString(bOk) + " " +
		   FileService::GetLastSystemIOErrorMessage());
}

void FileReaderCard::Test()
{
	FileReaderCard fileReader;

	// Parametrage de la boite et ouverture
	UIObject::SetUIMode(UIObject::Graphic);
	fileReader.Open();
}
