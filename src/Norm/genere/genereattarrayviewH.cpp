// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

void TableGenerator::GenerateAttributeArrayViewH(ostream& ost) const
{
	ost << "#pragma once\n";
	ost << "\n";
	GenerateFileHeader(ost);

	ost << ""
	    << "\n";
	ost << "#include \"UserInterface.h\""
	    << "\n";
	ost << ""
	    << "\n";
	ost << "#include \"" << GetClassName() << ".h\""
	    << "\n";
	ost << "#include \"" << GetClassName() << "View.h\""
	    << "\n";
	if (GetSuperClassName() != "")
		ost << "#include \"" << GetSuperClassName() << "ArrayView.h\""
		    << "\n";

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "", "Custom includes");

	ost << ""
	    << "\n";
	GenereClassHeaderComment(ost, "ArrayView");
	ost << "// Editeur de tableau de " << GetClassName() << "\n";
	if (GetSuperClassName() == "")
		ost << "class " << GetClassName() << "ArrayView : public UIObjectArrayView"
		    << "\n";
	else
		ost << "class " << GetClassName() << "ArrayView : public " << GetSuperClassName() << "ArrayView\n";
	ost << "{"
	    << "\n";
	ost << "public:"
	    << "\n";
	ost << "\t// Constructeur"
	    << "\n";
	ost << "\t" << GetClassName() << "ArrayView();"
	    << "\n";
	ost << "\t~" << GetClassName() << "ArrayView();"
	    << "\n";
	ost << ""
	    << "\n";

	GenereTitledComment(ost, "\t", "Redefinition des methodes a reimplementer obligatoirement");
	ost << ""
	    << "\n";
	ost << "\t// Creation d'un objet (du bon type), suite a une demande d'insertion utilisateur"
	    << "\n";
	ost << "\tObject* EventNew() override;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\t// Mise a jour de l'objet correspondant a l'index courant par les valeurs de l'interface"
	    << "\n";
	ost << "\tvoid EventUpdate(Object* object) override;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\t// Mise a jour des valeurs de l'interface par l'objet correspondant a l'index courant"
	    << "\n";
	ost << "\tvoid EventRefresh(Object* object) override;"
	    << "\n";

	ost << ""
	    << "\n";
	ost << "\t// Libelles utilisateur"
	    << "\n";
	ost << "\tconst ALString GetClassLabel() const override;"
	    << "\n";

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom declarations");

	ost << ""
	    << "\n";
	GenereImplementationComment(ost);
	ost << "protected:"
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom implementation");

	ost << "};"
	    << "\n";

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "", "Custom inlines");
}