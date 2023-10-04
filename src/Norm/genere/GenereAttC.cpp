// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

void TableGenerator::GenerateAttributeC(ostream& ost) const
{
	int nCurrent;
	Attribute* att;
	int nCount;
	boolean bSkipLine;

	GenerateFileHeader(ost);

	ost << ""
	    << "\n";
	ost << "#include \"" << GetModelClassName() << ".h\""
	    << "\n";

	// Constructeur
	ost << ""
	    << "\n";
	ost << GetModelClassName() << "::" << GetModelClassName() << "()"
	    << "\n";
	ost << "{"
	    << "\n";
	bSkipLine = false;
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->IsConstructorInit() and not att->GetDerived())
		{
			ost << "\t" << att->GetPrefix() << att->GetName() << " = " << att->GetDefaultValue() << ";"
			    << "\n";
			bSkipLine = true;
		}
	}
	if (bSkipLine)
		ost << ""
		    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom constructor");
	ost << "}"
	    << "\n";

	ost << ""
	    << "\n";
	ost << GetModelClassName() << "::~" << GetModelClassName() << "()"
	    << "\n";
	ost << "{"
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom destructor");
	ost << "}"
	    << "\n";

	// Copie
	ost << ""
	    << "\n";
	ost << "void " << GetModelClassName() << "::CopyFrom(const " << GetModelClassName() << "* aSource)"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\trequire(aSource != NULL);\n"
	    << "\n";
	bSkipLine = false;
	if (GetSuperClassName() != "")
		ost << "\t" << GetModelSuperClassName() << "::CopyFrom(aSource);\n\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (not att->GetDerived())
		{
			ost << "\t" << att->GetPrefix() << att->GetName() << " = aSource->" << att->GetPrefix()
			    << att->GetName() << ";"
			    << "\n";
			bSkipLine = true;
		}
	}
	if (bSkipLine)
		ost << ""
		    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom copyfrom");
	ost << "}"
	    << "\n";

	// Duplication
	ost << ""
	    << "\n";
	ost << GetModelClassName() << "* " << GetModelClassName() << "::Clone() const"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\t" << GetModelClassName() << "* aClone;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\taClone = new " << GetModelClassName() << ";"
	    << "\n";
	ost << "\taClone->CopyFrom(this);"
	    << "\n";
	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom clone");
	ost << "\treturn aClone;"
	    << "\n";
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	// Write
	ost << "void " << GetModelClassName() << "::Write(ostream& ost) const"
	    << "\n";
	ost << "{"
	    << "\n";
	if (GetSuperClassName() != "")
		ost << "\t" << GetModelSuperClassName() << "::Write(ost);\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetVisible())
		{
			ost << "\tost << \"" << att->GetLabel() << "\\t\" << "
			    << att->GetTypeVarToStream("Get" + att->GetName() + "()") << " << \"\\n\";"
			    << "\n";
		}
	}
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	// GetClassLabel
	ost << "const ALString " << GetModelClassName() << "::GetClassLabel() const"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\treturn \"" << GetClassUserLabel() << "\";"
	    << "\n";
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	// Implementation des getters des attributs derives
	nCount = 0;
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetDerived())
		{
			if (nCount == 0)
			{
				GenereTitledComment(ost, "", "");
				ost << ""
				    << "\n";
			}

			ost << att->GetDerivedGetterType() << " " << GetModelClassName() << "::Get" << att->GetName()
			    << "() const"
			    << "\n";
			ost << "{"
			    << "\n";
			GenerateUserCodeHeader(ost, "\t", "Custom " + att->GetName() + " Getter");
			ost << "\treturn " << att->GetDefaultValue() << ";"
			    << "\n";
			GenerateUserCodeTrailer(ost, "\t", "Custom " + att->GetName() + "Getter", true);
			ost << "}"
			    << "\n";
			ost << ""
			    << "\n";
			nCount++;
		}
	}

	GenerateUserCodeHeader(ost, "", "Method implementation");

	// Methode GetObjectLabel
	ost << ""
	    << "\n";
	ost << "const ALString " << GetModelClassName() << "::GetObjectLabel() const"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\tALString sLabel;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\treturn sLabel;"
	    << "\n";
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	GenerateUserCodeTrailer(ost, "", "Method implementation", false);
}
