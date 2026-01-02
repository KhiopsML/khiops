// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

void TableGenerator::GenerateAttributeViewH(ostream& ost) const
{
	GenerateCopyrightHeader(ost);
	ost << "#pragma once\n";
	ost << "\n";
	GenerateFileHeader(ost);

	ost << ""
	    << "\n";
	ost << "#include \"UserInterface.h\""
	    << "\n";
	ost << ""
	    << "\n";
	ost << "#include \"" << GetModelClassName() << ".h\""
	    << "\n";
	if (GetSuperClassName() != "")
		ost << "#include \"" << GetViewSuperClassName() << ".h\""
		    << "\n";

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "", "Custom includes");

	ost << ""
	    << "\n";
	GenereClassHeaderComment(ost, "View");
	ost << "// Editeur de " << GetModelClassName() << "\n";
	ost << "class " << GetViewClassName() << " : public " << GetViewSuperClassName() << "\n";
	ost << "{"
	    << "\n";
	ost << "public:"
	    << "\n";
	ost << "\t// Constructeur"
	    << "\n";
	ost << "\t" << GetViewClassName() << "();"
	    << "\n";
	ost << "\t~" << GetViewClassName() << "();"
	    << "\n";
	ost << ""
	    << "\n";

	ost << "\t// Acces a l'objet edite"
	    << "\n";
	ost << "\t" << GetModelClassName() << "* Get" << GetModelClassName() << "();"
	    << "\n";
	ost << ""
	    << "\n";

	GenereTitledComment(ost, "\t", "Redefinition des methodes a reimplementer obligatoirement");
	ost << ""
	    << "\n";
	ost << "\t// Mise a jour de l'objet par les valeurs de l'interface"
	    << "\n";
	ost << "\tvoid EventUpdate(Object* object) override;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\t// Mise a jour des valeurs de l'interface par l'objet"
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
