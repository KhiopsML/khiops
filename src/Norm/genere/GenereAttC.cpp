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
	ost << "#include \"" << GetClassName() << ".h\""
	    << "\n";

	// Constructeur
	ost << ""
	    << "\n";
	ost << GetClassName() << "::" << GetClassName() << "()"
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
	ost << GetClassName() << "::~" << GetClassName() << "()"
	    << "\n";
	ost << "{"
	    << "\n";
	GenerateUserCodeSection(ost, "\t", "Custom destructor");
	ost << "}"
	    << "\n";

	// Copie
	ost << ""
	    << "\n";
	ost << "void " << GetClassName() << "::CopyFrom(const " << GetClassName() << "* aSource)"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\trequire(aSource != NULL);\n"
	    << "\n";
	bSkipLine = false;
	if (GetSuperClassName() != "")
		ost << "\t" << GetSuperClassName() << "::CopyFrom(aSource);\n\n";
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
	ost << GetClassName() << "* " << GetClassName() << "::Clone() const"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\t" << GetClassName() << "* aClone;"
	    << "\n";
	ost << ""
	    << "\n";
	ost << "\taClone = new " << GetClassName() << ";"
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

	// Champs de la cle
	if (not GetAttributeTable()->NoKeyFields())
	{
		ost << "const ALString& " << GetClassName() << "::GetKey() const"
		    << "\n";
		ost << "{"
		    << "\n";

		// Cas particulier d'un seul champ, pour gerer le cas cle alphanumerique
		if (GetAttributeTable()->GetKeyFieldsNumber() <= 1)
		{
			for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
			{
				att = GetFieldAt(nCurrent);

				if (att->GetKeyField() == true)
				{
					if (att->GetType() == "ALString")
						ost << "\treturn "
						    << att->GetTypeVarToString("Get" + att->GetName() + "()") << ";"
						    << "\n";
					else
					{
						ost << "\tsInstanceKey = "
						    << att->GetTypeVarToString("Get" + att->GetName() + "()") << ";"
						    << "\n";
						ost << "\treturn sInstanceKey;"
						    << "\n";
					}
				}
			}
		}
		// Cas general de cle multi-champs
		else
		{
			int nKeyIndex = 0;
			for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
			{
				att = GetFieldAt(nCurrent);

				if (att->GetKeyField() == true)
				{
					if (nKeyIndex == 0)
						ost << "\tsInstanceKey = "
						    << att->GetTypeVarToString("Get" + att->GetName() + "()") << ";"
						    << "\n";
					else
						ost << "\tsInstanceKey += "
						    << att->GetTypeVarToString("Get" + att->GetName() + "()") << ";"
						    << "\n";
					if (nKeyIndex < GetAttributeTable()->GetKeyFieldsNumber() - 1)
						ost << "\tsInstanceKey += ';';"
						    << "\n";
					nKeyIndex++;
				}
			}
			ost << "\treturn sInstanceKey;"
			    << "\n";
		}
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";
	}

	// Fonctionnalites de gestion de l'objet
	if (GetGenereManagement())
	{
		// managedObjectClass
		ost << "" << GetClassName() << " " << GetClassName() << "::managedObjectClass;"
		    << "\n";
		ost << ""
		    << "\n";

		// GetManagedObjectClass
		ost << "" << GetClassName() << "* " << GetClassName() << "::GetManagedObjectClass()"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\treturn &managedObjectClass;"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		// GetFieldNumber
		ost << "int " << GetClassName() << "::GetFieldNumber() const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\treturn LastField;"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		// SetFieldAt
		ost << "void " << GetClassName() << "::SetFieldAt(int nFieldIndex, const char* sValue)"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\trequire(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\tswitch(nFieldIndex)"
		    << "\n";
		ost << "\t{"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);
			if (not att->GetDerived())
				ost << "\tcase " << att->GetName() << ": Set" << att->GetName() << "("
				    << att->GetCharVarToType("sValue") << "); break;"
				    << "\n";
		}
		ost << "\tdefault: break;"
		    << "\n";
		ost << "\t};"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		// GetFieldAt
		ost << "const char* " << GetClassName() << "::GetFieldAt(int nFieldIndex) const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\trequire(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\tswitch(nFieldIndex)"
		    << "\n";
		ost << "\t{"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);
			if (att->GetDerived() and att->GetType() == "ALString")
				ost << "\tcase " << att->GetName() << ": return CharsToString("
				    << att->GetTypeVarToString("Get" + att->GetName() + "()") << ");"
				    << "\n";
			else
				ost << "\tcase " << att->GetName() << ": return "
				    << att->GetTypeVarToString("Get" + att->GetName() + "()") << ";"
				    << "\n";
		}
		ost << "\tdefault: return \"\";"
		    << "\n";
		ost << "\t};"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		// GetFieldNameAt
		ost << "const ALString " << GetClassName() << "::GetFieldNameAt(int nFieldIndex) const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\trequire(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\tswitch(nFieldIndex)"
		    << "\n";
		ost << "\t{"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);
			ost << "\tcase " << att->GetName() << ": return \"" << att->GetName() << "\";"
			    << "\n";
		}
		ost << "\tdefault: return \"\";"
		    << "\n";
		ost << "\t};"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		// GetFieldLabelAt
		ost << "const ALString " << GetClassName() << "::GetFieldLabelAt(int nFieldIndex) const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\trequire(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\tswitch(nFieldIndex)"
		    << "\n";
		ost << "\t{"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);
			ost << "\tcase " << att->GetName() << ": return \"" << att->GetLabel() << "\";"
			    << "\n";
		}
		ost << "\tdefault: return \"\";"
		    << "\n";
		ost << "\t};"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		// GetFieldStorageNameAt
		ost << "const ALString " << GetClassName() << "::GetFieldStorageNameAt(int nFieldIndex) const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\trequire(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\tswitch(nFieldIndex)"
		    << "\n";
		ost << "\t{"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);
			ost << "\tcase " << att->GetName() << ": return \"" << att->GetStorageName() << "\";"
			    << "\n";
		}
		ost << "\tdefault: return \"\";"
		    << "\n";
		ost << "\t};"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";
	}

	// Write
	ost << "void " << GetClassName() << "::Write(ostream& ost) const"
	    << "\n";
	ost << "{"
	    << "\n";
	if (GetSuperClassName() != "")
		ost << "\t" << GetSuperClassName() << "::Write(ost);\n";
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetVisible() == true)
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

	// Gestion du separateur de champs
	if (GetGenereManagement())
	{
		ost << "char " << GetClassName() << "::cFieldSeparator = '\\t';"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "void " << GetClassName() << "::SetFieldSeparator(char cValue)"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\tcFieldSeparator = cValue;"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "char " << GetClassName() << "::GetFieldSeparator()"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\treturn cFieldSeparator;"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";
	}

	// GetClassLabel
	ost << "const ALString " << GetClassName() << "::GetClassLabel() const"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\treturn \"" << GetClassUserLabel() << "\";"
	    << "\n";
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	if (GetGenereManagement())
	{
		GenereTitledComment(ost, "", "");
		ost << ""
		    << "\n";
	}

	// Fonctionnalites de gestion de l'objet
	if (GetGenereManagement())
	{
		// CloneManagedObject
		ost << "ManagedObject* " << GetClassName() << "::CloneManagedObject() const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\treturn Clone();"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		// ivStoredFieldIndexes
		ost << "IntVector " << GetClassName() << "::ivStoredFieldIndexes;"
		    << "\n";
		ost << ""
		    << "\n";

		// GetFieldStored
		ost << "boolean " << GetClassName() << "::GetFieldStored(int nFieldIndex) const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\tint i;"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\trequire(0 <= nFieldIndex and nFieldIndex < GetFieldNumber());"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// On force l'initialisation des index"
		    << "\n";
		ost << "\tGetStoredFieldIndexes();"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// On recherche dans le tableau des index"
		    << "\n";
		ost << "\t// des attributs stockes"
		    << "\n";
		ost << "\tfor (i = 0; i < ivStoredFieldIndexes.GetSize(); i++)"
		    << "\n";
		ost << "\t{"
		    << "\n";
		ost << "\t    if (ivStoredFieldIndexes.GetAt(i) == nFieldIndex)"
		    << "\n";
		ost << "\t        return true;"
		    << "\n";
		ost << "\t}"
		    << "\n";
		ost << "\treturn false;"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		// GetStoredFieldIndexes
		ost << "IntVector* " << GetClassName() << "::GetStoredFieldIndexes() const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\t// Initialisation si necessaire du tableau d'index"
		    << "\n";
		ost << "\tif (ivStoredFieldIndexes.GetSize() == 0)"
		    << "\n";
		ost << "\t{"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);
			if (att->GetPermanent())
				ost << "\t    ivStoredFieldIndexes.Add(" << att->GetName() << ");"
				    << "\n";
		}
		ost << "\t}"
		    << "\n";
		ost << "\treturn &ivStoredFieldIndexes;"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		// GetCompareKeyFunction
		ost << "CompareFunction " << GetClassName() << "::GetCompareKeyFunction() const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\treturn " << GetClassName() << "CompareKey;"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";

		GenereTitledComment(ost, "", "");
		ost << ""
		    << "\n";

		// GetSeparator
		ost << "char " << GetClassName() << "::GetSeparator() const"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\treturn cFieldSeparator;"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";
	}

	// Gestion des champs de la cle
	if (GetGenereManagement())
	{
		ost << "int " << GetClassName() << "CompareKey(const void* first, const void* second)"
		    << "\n";
		ost << "{"
		    << "\n";
		ost << "\t" << GetClassName() << "* aFirst;"
		    << "\n";
		ost << "\t" << GetClassName() << "* aSecond;"
		    << "\n";
		ost << "\tint nResult;"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\taFirst = cast(" << GetClassName() << "*, *(Object**)first);"
		    << "\n";
		ost << "\taSecond = cast(" << GetClassName() << "*, *(Object**)second);"
		    << "\n";
		if (GetAttributeTable()->NoKeyFields())
		{
			ost << "\tnResult = aFirst - aSecond;"
			    << "\n";
		}
		else
		{
			for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
			{
				att = GetFieldAt(nCurrent);

				if (att->GetKeyField() == true)
				{
					ost << "\tnResult = "
					    << att->GetTypeVarComparison("aFirst->Get" + att->GetName() + "()",
									 "aSecond->Get" + att->GetName() + "()")
					    << ";"
					    << "\n";

					ost << "\tif (nResult != 0)"
					    << "\n";
					ost << "\t  return nResult;"
					    << "\n";
				}
			}
		}
		ost << "\treturn nResult;"
		    << "\n";
		ost << "}"
		    << "\n";
		ost << ""
		    << "\n";
	}

	// Fonctions Compare<Att> et Get<Att>
	for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
	{
		att = GetFieldAt(nCurrent);

		if (att->GetStats() == true)
		{
			// Compare
			ost << "int " << GetClassName() << "Compare" << att->GetName()
			    << "(const void* first, const void* second)"
			    << "\n";
			ost << "{"
			    << "\n";
			ost << "\treturn"
			    << "\n";

			ost << "\t   "
			    << att->GetTypeVarComparison("cast(" + GetClassName() + "*, *(Object**)first)->Get" +
							     att->GetName() + "()\n       ",
							 "cast(" + GetClassName() + "*, *(Object**)second)->Get" +
							     att->GetName() + "()")
			    << ";"
			    << "\n";

			ost << "}"
			    << "\n";
			ost << ""
			    << "\n";

			// Get Attribute
			ost << "const ALString " << GetClassName() << "Get" << att->GetName()
			    << "(const Object* object)"
			    << "\n";
			ost << "{"
			    << "\n";
			ost << "\treturn "
			    << att->GetTypeVarToString("cast(" + GetClassName() + "*, object)->Get" + att->GetName() +
						       "()")
			    << ";"
			    << "\n";
			ost << "}"
			    << "\n";
			ost << ""
			    << "\n";
		}
	}

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

			ost << att->GetDerivedGetterType() << " " << GetClassName() << "::Get" << att->GetName()
			    << "() const"
			    << "\n";
			ost << "{"
			    << "\n";
			GenerateUserCodeHeader(ost, "\t", "Custom " + att->GetName() + " Getter");
			ost << "\treturn " << att->GetDefaultValue() << ";"
			    << "\n";
			GenerateUserCodeTrailer(ost, "\t", "Custom " + att->GetName() + "Getter");
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
	ost << "const ALString " << GetClassName() << "::GetObjectLabel() const"
	    << "\n";
	ost << "{"
	    << "\n";
	ost << "\tALString sLabel;"
	    << "\n";
	ost << ""
	    << "\n";

	{
		int nKeyIndex = 0;
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);

			if (att->GetKeyField() == true)
			{
				if (nKeyIndex == 0)
					ost << "\tsLabel = " << att->GetTypeVarToString("Get" + att->GetName() + "()")
					    << ";"
					    << "\n";
				else
					ost << "\tsLabel += ' ' + "
					    << att->GetTypeVarToString("Get" + att->GetName() + "()") << ";"
					    << "\n";
				nKeyIndex++;
			}
		}
		ost << "\treturn sLabel;"
		    << "\n";
	}
	ost << "}"
	    << "\n";
	ost << ""
	    << "\n";

	GenerateUserCodeTrailer(ost, "", "Method implementation");
}