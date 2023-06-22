// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableGenerator.h"

void TableGenerator::GenerateAttributeH(ostream& ost) const
{
	int nCurrent;
	Attribute* att;
	int nCount;

	ost << "#pragma once\n";
	ost << "\n";
	GenerateFileHeader(ost);

	ost << ""
	    << "\n";
	if (GetGenereManagement())
		ost << "#include \"ManagedObject.h\""
		    << "\n";
	else
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
		if (GetGenereManagement())
			ost << "class " << GetClassName() << " : public ManagedObject"
			    << "\n";
		else
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
			if (att->GetPermanent())
				ost << " (stored)"
				    << "\n";
			else
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

	// Gestion de la cle
	ost << ""
	    << "\n";
	if (not GetAttributeTable()->NoKeyFields())
	{
		ost << "\t// Champs de la cle"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);

			if (att->GetKeyField() == true)
				ost << "\t//    " << att->GetName() << "\n";
		}
		if (GetAttributeTable()->GetKeyFieldsNumber() > 1)
			ost << "\t// (le separateur des champs de la cle est ';')"
			    << "\n";
		ost << "\tconst ALString& GetKey() const;"
		    << "\n";
		ost << ""
		    << "\n";
	}

	// Fonctionnalites generiques
	if (GetGenereManagement())
	{
		GenereTitledComment(ost, "\t", "Fonctionnalites generiques");
		ost << ""
		    << "\n";
		ost << "\t// Acces a un exemplaire d'objet gere"
		    << "\n";
		ost << "\t// Permet d'initialiser facilement un container par"
		    << "\n";
		ost << "\t// new ManagedObjectTable(" << GetClassName() << "::GetManagedObjectClass())"
		    << "\n";
		ost << "\tstatic " << GetClassName() << "* GetManagedObjectClass();"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// Liste des index des champs"
		    << "\n";
		ost << "\tenum {\n";
		nCount = 0;
		for (nCurrent = 0; nCurrent < GetAttributeNumber(); nCurrent++)
		{
			att = GetAttributeAt(nCurrent);

			if (att->IsField())
			{
				ost << "\t    ";
				ost << att->GetName() << ","
				    << "\n";
				nCount++;
			}
		}
		ost << "\t    LastField"
		    << "\n";
		ost << "\t};"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// Nombre de champs"
		    << "\n";
		ost << "\tint GetFieldNumber() const;"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// Acces generique aux champs, par une valeur de"
		    << "\n";
		ost << "\t// type chaine de caracteres"
		    << "\n";
		ost << "\t// (les champs derives sont ignores en ecriture)"
		    << "\n";
		ost << "\tvoid SetFieldAt(int nFieldIndex, const char* sValue);"
		    << "\n";
		ost << "\tconst char* GetFieldAt(int nFieldIndex) const;"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// Nom d'un champ d'apres son index"
		    << "\n";
		ost << "\tconst ALString GetFieldNameAt(int nFieldIndex) const;"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// Libelle d'un champ d'apres son index"
		    << "\n";
		ost << "\tconst ALString GetFieldLabelAt(int nFieldIndex) const;"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// Nom de stockage d'un champ d'apres son index"
		    << "\n";
		ost << "\tconst ALString GetFieldStorageNameAt(int nFieldIndex) const;"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// Parametrage du separateur de champs (par defaut: '\\t')"
		    << "\n";
		ost << "\tstatic void SetFieldSeparator(char cValue);"
		    << "\n";
		ost << "\tstatic char GetFieldSeparator();"
		    << "\n";
		ost << ""
		    << "\n";
	}

	// Divers
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

	// Implementation des methodes virtuelles de ManagedObject
	if (GetGenereManagement())
	{
		ost << "\t// Implementation des methodes virtuelles de ManagedObject"
		    << "\n";
		ost << "\tManagedObject* CloneManagedObject() const;"
		    << "\n";
		ost << "\tboolean GetFieldStored(int nFieldIndex) const;"
		    << "\n";
		ost << "\tIntVector* GetStoredFieldIndexes() const;"
		    << "\n";
		ost << "\tCompareFunction GetCompareKeyFunction() const;"
		    << "\n";
		ost << "\tchar GetSeparator() const;"
		    << "\n";
		ost << ""
		    << "\n";
		ost << "\t// Exemplaire d'objet gere"
		    << "\n";
		ost << "\tstatic " << GetClassName() << " managedObjectClass;"
		    << "\n";
		ost << ""
		    << "\n";
	}

	// Attributs de la classe
	ost << "\t// Attributs de la classe"
	    << "\n";
	if (GetAttributeTable()->IsKeyBuilt())
		ost << "\tmutable ALString sInstanceKey;"
		    << "\n";
	if (GetGenereManagement())
	{
		ost << "\tstatic char cFieldSeparator;"
		    << "\n";
		ost << "\tstatic IntVector ivStoredFieldIndexes;"
		    << "\n";
	}
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

	if (GetGenereManagement())
	{
		ost << ""
		    << "\n";
		ost << "// Fonction de comparaison sur les champs de la cle"
		    << "\n";
		ost << "int " << GetClassName() << "CompareKey(const void* first, const void* second);"
		    << "\n";
		ost << ""
		    << "\n";
	}

	if (not GetAttributeTable()->NoStats())
	{
		ost << ""
		    << "\n";
		ost << "// Fonctions de comparaison et d'acces aux attributs"
		    << "\n";
		for (nCurrent = 0; nCurrent < GetFieldNumber(); nCurrent++)
		{
			att = GetFieldAt(nCurrent);

			if (att->GetStats())
			{
				ost << "int " << GetClassName() << "Compare" << att->GetName()
				    << "(const void* first, const void* second);"
				    << "\n";
				ost << "const ALString " << GetClassName() << "Get" << att->GetName()
				    << "(const Object* object);"
				    << "\n";
			}
		}
	}

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