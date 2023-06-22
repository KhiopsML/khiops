// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

void TableGenerator::GenerateAttributeArrayViewC(ostream& ost) const
{
	int nCurrent;
	Attribute* att;

	GenerateFileHeader(ost);

	ost << ""
	    << "\n";
	ost << "#include \"" << GetClassName() << "ArrayView.h\""
	    << "\n";
	ost << ""
	    << "\n";

	ost << GetClassName() << "ArrayView::" << GetClassName() << "ArrayView()"
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

		if (att->GetVisible() == true)
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
	ost << "\tSetItemView(new " << GetClassName() << "View);"
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

	ost << GetClassName() << "ArrayView::~" << GetClassName() << "ArrayView()"
	    << "\n";
	ost << "{"
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom destructor");
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	ost << "Object* " << GetClassName() << "ArrayView::EventNew()"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\treturn new " << GetClassName() << ";"
	    << "\n";
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	ost << "void " << GetClassName() << "ArrayView::EventUpdate(Object* object)"
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
		ost << "\t" << GetSuperClassName() << "ArrayView::EventUpdate(object);\n";
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

	ost << "void " << GetClassName() << "ArrayView::EventRefresh(Object* object)"
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
		ost << "\t" << GetSuperClassName() << "ArrayView::EventRefresh(object);\n";
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
	ost << "const ALString " << GetClassName() << "ArrayView::GetClassLabel() const"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\treturn \"" << GetClassUserLabel() << "s\";"
	    << "\n";
	ost << "}"
	    << "\n";

	ost << ""
	    << "\n";
	GenerateUserCodeSection(ost, "", "Method implementation");
}