// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

void TableGenerator::GenerateAttributeH(ostream& ost) const
{
	int nCurrent;
	Attribute* att;

	ost << "#pragma once\n";
	ost << "\n";
	GenerateFileHeader(ost);

	ost << ""
	    << "\n";
	ost << "#include \"Object.h\""
	    << "\n";
	if (GetSuperClassName() != "")
		ost << "#include \"" << GetSuperClassName() << ".h\""
		    << "\n";

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "", "Custom includes");

	ost << ""
	    << "\n";
	GenereClassHeaderComment(ost, "");
	if (GetSuperClassName() == "")
	{
		ost << "class " << GetClassName() << " : public Object"
		    << "\n";
	}
	else
		ost << "class " << GetClassName() << " : public " << GetSuperClassName() << "\n";
	ost << "{"
	    << "\n";
	ost << "public:"
	    << "\n";
	ost << "\t// Constructeur"
	    << "\n";
	ost << "\t" << GetClassName() << "();"
	    << "\n";
	ost << "\t~" << GetClassName() << "();"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\t// Copie et duplication"
	    << "\n";
	ost << "\tvoid CopyFrom(const " << GetClassName() << "* aSource);"
	    << "\n";
	ost << "\t" << GetClassName() << "* Clone() const;"
	    << "\n";

	// Declaration des getters et setters
	for (nCurrent = 0; nCurrent < GetAttributeNumber(); nCurrent++)
	{
		att = GetAttributeAt(nCurrent);

		ost << ""
		    << "\n";

		// Generation d'un titre predefini si le premier
		// attribut n'est pas un libelle
		if (nCurrent == 0 and not att->IsLabel())
		{
			GenereTitledComment(ost, "\t", "Acces aux attributs");
			ost << ""
			    << "\n";
		}

		// Test si attribut de type Field
		if (att->IsField())
		{
			ost << "\t// " << att->GetLabel();
			ost << "\n";

			if (not att->GetDerived())
				ost << "\t" << att->GetMethodDecl() << " Get" << att->GetName() << "() const;"
				    << "\n";
			else
				ost << "\t" << att->GetDerivedGetterType() << " Get" << att->GetName() << "() const;"
				    << "\n";
			if (not att->GetDerived())
				ost << "\tvoid Set" << att->GetName() << "(" << att->GetMethodDecl() << " "
				    << att->GetPrefix() << "Value);"
				    << "\n";
		}
		else
		// Sinon, generation d'un libelle titre
		{
			GenereTitledComment(ost, "\t", att->GetLabel());
		}
	}

	// Divers
	ost << ""
	    << "\n";
	GenereTitledComment(ost, "\t", "Divers");
	ost << ""
	    << "\n";
	ost << "\t// Ecriture"
	    << "\n";
	ost << "\tvoid Write(ostream& ost) const override;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\t// Libelles utilisateur"
	    << "\n";
	ost << "\tconst ALString GetClassLabel() const override;"
	    << "\n";
	ost << "\tconst ALString GetObjectLabel() const override;"
	    << "\n";

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom declarations");

	// Implementation
	ost << ""
	    << "\n";
	GenereImplementationComment(ost);
	ost << "protected:"
	    << "\n";

	// Attributs de la classe
	ost << "\t// Attributs de la classe"
	    << "\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (not att->GetDerived())
			ost << "\t" << att->GetType() << " " << att->GetPrefix() << att->GetName() << ";"
			    << "\n";
	}

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom implementation");

	ost << "};"
	    << "\n";

	ost << ""
	    << "\n";
	GenereTitledComment(ost, "", "Implementations inline");
	ost << ""
	    << "\n";

	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (not att->GetDerived())
		{
			ost << "inline " << att->GetMethodDecl() << " " << GetClassName() << "::Get" << att->GetName()
			    << "() const"
			    << "\n";
			ost << "{"
			    << "\n";
			ost << "\treturn " << att->GetTypeVarToField(att->GetPrefix() + att->GetName()) << ";"
			    << "\n";
			ost << "}"
			    << "\n";
			ost << ""
			    << "\n";
			ost << "inline void " << GetClassName() << "::Set" << att->GetName() << "("
			    << att->GetMethodDecl() << " " << att->GetPrefix() << "Value)"
			    << "\n";
			ost << "{"
			    << "\n";
			ost << "\t" << att->GetPrefix() << att->GetName() << " = "
			    << att->GetFieldToTypeVar(att->GetPrefix() + "Value") << ";"
			    << "\n";
			ost << "}"
			    << "\n";
			ost << ""
			    << "\n";
		}
	}

	GenerateUserCodeSection(ost, "", "Custom inlines");
}
