// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

void TableGenerator::GenerateAttributeArrayViewC(ostream& ost) const
{
	int nCurrent;
	Attribute* att;

	GenerateCopyrightHeader(ost);
	GenerateFileHeader(ost);

	ost << ""
	    << "\n";
	ost << "#include \"" << GetArrayViewClassName() << ".h\""
	    << "\n";
	ost << ""
	    << "\n";

	ost << GetArrayViewClassName() << "::" << GetArrayViewClassName() << "()"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\tSetIdentifier(\""
	    << "Array." << GetClassName() << "\");"
	    << "\n";
	ost << "\tSetLabel(\"" << GetClassUserLabel() << "s\");"
	    << "\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetVisible())
		{
			ost << "\tAdd" << att->GetFieldType() << "Field(\"" << att->GetName() << "\", \""
			    << att->GetLabel() << "\", " << att->GetTypeVarToField(att->GetDefaultValue()) << ");"
			    << "\n";
		}
	}
	ost << ""
	    << "\n";
	ost << "\t// Card and help prameters"
	    << "\n";
	ost << "\tSetItemView(new " << GetViewClassName() << ");"
	    << "\n";
	ost << "\tCopyCardHelpTexts();"
	    << "\n";

	if (GetAttributeTable()->GetStyleFieldsNumber() > 0)
	{
		ost << ""
		    << "\n";
		ost << "\t// Parametrage des styles;"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);

			if (att->GetVisible() and att->GetStyle() != "")
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

	ost << GetArrayViewClassName() << "::~" << GetArrayViewClassName() << "()"
	    << "\n";
	ost << "{"
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom destructor");
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	ost << "Object* " << GetArrayViewClassName() << "::EventNew()"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\treturn new " << GetModelClassName() << ";"
	    << "\n";
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	ost << "void " << GetArrayViewClassName() << "::EventUpdate(Object* object)"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\t" << GetModelClassName() << "* editedObject;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\trequire(object != NULL);"
	    << "\n";
	ost << ""
	    << "\n";
	if (GetSuperClassName() != "")
		ost << "\t" << GetArrayViewSuperClassName() << "::EventUpdate(object);\n";
	ost << "\teditedObject = cast(" << GetModelClassName() << "*, object);"
	    << "\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetVisible() and not att->GetDerived())
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

	ost << "void " << GetArrayViewClassName() << "::EventRefresh(Object* object)"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\t" << GetModelClassName() << "* editedObject;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\trequire(object != NULL);"
	    << "\n";
	ost << ""
	    << "\n";
	if (GetSuperClassName() != "")
		ost << "\t" << GetArrayViewSuperClassName() << "::EventRefresh(object);\n";
	ost << "\teditedObject = cast(" << GetModelClassName() << "*, object);"
	    << "\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetVisible())
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
	ost << "const ALString " << GetArrayViewClassName() << "::GetClassLabel() const"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\treturn \"" << GetClassUserLabel() << "s\";"
	    << "\n";
	ost << "}"
	    << "\n";

	ost << ""
	    << "\n";
	GenerateUserCodeHeader(ost, "", "Method implementation");
	ost << "\n";
	GenerateUserCodeTrailer(ost, "", "Method implementation", true);
}
