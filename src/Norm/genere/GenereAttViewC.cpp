// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

void TableGenerator::GenerateAttributeViewC(ostream& ost) const
{
	int nCurrent;
	Attribute* att;

	GenerateFileHeader(ost);

	ost << ""
	    << "\n";
	ost << "#include \"" << GetClassName() << "View.h\""
	    << "\n";
	ost << ""
	    << "\n";

	ost << GetClassName() << "View::" << GetClassName() << "View()"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\tSetIdentifier(\"" << GetClassName() << "\");"
	    << "\n";
	ost << "\tSetLabel(\"" << GetClassUserLabel() << "\");"
	    << "\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetVisible() == true)
		{
			ost << "\tAdd" << att->GetFieldType() << "Field(\"" << att->GetName() << "\", \""
			    << att->GetLabel() << "\", " << att->GetTypeVarToField(att->GetDefaultValue()) << ");"
			    << "\n";
		}
	}

	if (GetAttributeTable()->GetStyleFieldsNumber() > 0)
	{
		ost << ""
		    << "\n";
		ost << "\t// Parametrage des styles;"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);

			if (att->GetVisible() == true and att->GetStyle() != "")
			{
				ost << "\tGetFieldAt(\"" << att->GetName() << "\")->SetStyle(\"" << att->GetStyle()
				    << "\");"
				    << "\n";
			}
		}
	}

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom constructor");
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	ost << GetClassName() << "View::~" << GetClassName() << "View()"
	    << "\n";
	ost << "{"
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom destructor");
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	ost << GetClassName() << "* " << GetClassName() << "View::Get" << GetClassName() << "()"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\trequire(objValue != NULL);\n";
	ost << "\treturn cast(" << GetClassName() << "*, objValue);\n";
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	ost << "void " << GetClassName() << "View::EventUpdate(Object* object)"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\t" << GetClassName() << "* editedObject;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\trequire(object != NULL);"
	    << "\n";
	ost << ""
	    << "\n";
	if (GetSuperClassName() != "")
		ost << "\t" << GetSuperClassName() << "View::EventUpdate(object);\n";
	ost << "\teditedObject = cast(" << GetClassName() << "*, object);"
	    << "\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetVisible() == true and not att->GetDerived())
		{
			ost << "\teditedObject->Set" << att->GetName() << "("
			    << "Get" << att->GetFieldType() << "ValueAt(\"" << att->GetName() << "\")"
			    << ");"
			    << "\n";
		}
	}

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom update");
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	ost << "void " << GetClassName() << "View::EventRefresh(Object* object)"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\t" << GetClassName() << "* editedObject;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\trequire(object != NULL);"
	    << "\n";
	ost << ""
	    << "\n";
	if (GetSuperClassName() != "")
		ost << "\t" << GetSuperClassName() << "View::EventRefresh(object);\n";
	ost << "\teditedObject = cast(" << GetClassName() << "*, object);"
	    << "\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetVisible() == true)
		{
			ost << "\tSet" << att->GetFieldType() << "ValueAt(\"" << att->GetName() << "\", "
			    << "editedObject->Get" << att->GetName() << "()"
			    << ");"
			    << "\n";
		}
	}

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom refresh");
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	ost << "const ALString " << GetClassName() << "View::GetClassLabel() const"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\treturn \"" << GetClassUserLabel() << "\";"
	    << "\n";
	ost << "}"
	    << "\n";

	ost << ""
	    << "\n";
	GenerateUserCodeHeader(ost, "", "Method implementation");
	ost << "\n";
	GenerateUserCodeTrailer(ost, "", "Method implementation", false);
}