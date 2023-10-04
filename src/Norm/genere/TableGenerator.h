// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Attribute.h"
#include "AttributeTable.h"

#include "SectionTable.h"

///////////////////////////////
// Classe TableGenerator
// Generation de la gestion d'une structure de donnes a
// partir de la description de sa structure de donnees dans un fichier
// contenant des instance de la classe Attribute (fichier .dd: data definition)
// On genere une classe decrivant ses attributs avec getter et setter, ainsi
// que des methodes de lecture/ecriture dans un fichier.
// On genere egalement une classe gerant une liste d'enregistrements de
// la premiere classe
class TableGenerator : public Object
{
public:
	// Constructeur
	TableGenerator();
	~TableGenerator();

	///////////////////////////////////////////////////////////
	// Attributs de specification des classe a generer

	// Nom de base de la classe a generer
	const ALString& GetClassName() const;
	void SetClassName(const ALString& sValue);

	// Nom de base de la classe mere de la classe a generer
	// Par defaut vide, si on herite directement des classe standard de la librairie
	const ALString& GetSuperClassName() const;
	void SetSuperClassName(const ALString& sValue);

	// Nom specifique de classe de modele a generer
	// Par defaut vide, si l'on prend le nom de base a la fois pour le modele et les vue
	const ALString& GetSpecificModelClassName() const;
	void SetSpecificModelClassName(const ALString& sValue);

	// Libelle utilisateur de la classe a generer
	const ALString& GetClassUserLabel() const;
	void SetClassUserLabel(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Nom des classes a generer

	// Classe de model a generer
	const ALString GetModelClassName() const;

	// Classes de vue a generer
	const ALString GetViewClassName() const;
	const ALString GetArrayViewClassName() const;

	// Classes ancetre des classes a generer
	const ALString GetModelSuperClassName() const;
	const ALString GetViewSuperClassName() const;
	const ALString GetArrayViewSuperClassName() const;

	///////////////////////////////////////////////////////////
	// Parametrage de ce qui est a generer (true par defaut)

	// Generation de la classe principale
	void SetGenereModel(boolean bValue);
	boolean GetGenereModel() const;

	// Generation des fonctionnalites d'interface graphique
	// (GenereView==false => genereArrayView== false)
	void SetGenereView(boolean bValue);
	boolean GetGenereView() const;

	// Generation des fonctionnalites d'interface graphique
	// (GenereArrayView==false => GenereView==false)
	void SetGenereArrayView(boolean bValue);
	boolean GetGenereArrayView() const;

	// Generation des sections utilisateurs
	void SetGenereUserSection(boolean bValue);
	boolean GetGenereUserSection() const;

	// Repertoire en sortie de generation (defaut: "", repertoire courant)
	void SetOutputDir(const ALString& sValue);
	const ALString& GetOutputDir() const;

	// Methodes de generation
	void Genere() const;
	void GenereWith(const ALString& sName, const ALString& sSpecificModelName, const ALString& sSuperName,
			const ALString& sLabel, const ALString& sAttributeFileName);

	//////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Verification de la syntaxe d'un nom de classe
	boolean CheckClassName(const ALString& sValue) const;

	// Acces aux attributs de type Field, dans le bon ordre
	int GetFieldNumber() const;
	Attribute* GetFieldAt(int i) const;

	// Acces a tous les attributs (Field et Label), dans le bon ordre
	int GetAttributeNumber() const;
	Attribute* GetAttributeAt(int i) const;

	// Attributs de la classe a generer
	AttributeTable* GetAttributeTable() const;
	void SetAttributeTable(AttributeTable* attTable);

	// Gestion des erreurs
	void Error(const ALString& sMessage) const;

	// Nom des fichiers de backup
	const ALString BuildBackupFileName(const ALString& sFileName, const ALString& sWhich) const;

	// Gestion de la consolidation des fichiers utilisateurs et generes
	void ConsolidateFiles(const ALString& sFileName) const;

	///////////////////////////////////////////////////////////
	// Methodes de generation

	// Utilitaires de generation
	void GenerateCopyrightHeader(ostream& ost) const;
	void GenerateFileHeader(ostream& ost) const;
	void GenereClassHeaderComment(ostream& ost, const ALString& sClassFamily) const;
	void GenereTitledComment(ostream& ost, const ALString& sIndent, const ALString& sComment) const;
	void GenereImplementationComment(ostream& ost) const;
	void GenerateUserCodeSection(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const;
	void GenerateUserCodeHeader(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const;
	void GenerateUserCodeTrailer(ostream& ost, const ALString& sIndent, const ALString& sIdentifier,
				     boolean bNewLine) const;

	// Generation des tables d'attribut
	void GenerateAttributeH(ostream& ost) const;
	void GenerateAttributeC(ostream& ost) const;

	// Generation des composants d'interface utilisateur et du menu de query
	void GenerateAttributeViewH(ostream& ost) const;
	void GenerateAttributeViewC(ostream& ost) const;
	void GenerateAttributeArrayViewH(ostream& ost) const;
	void GenerateAttributeArrayViewC(ostream& ost) const;

	///////////////////////////////////////////////////////////
	// Attributs

	// Attributs de base
	ALString sClassName;
	ALString sSuperClassName;
	ALString sSpecificModelClassName;
	ALString sClassUserLabel;
	AttributeTable* attAttributeTable;
	QueryServices* qsAttributeRangServices;
	AttributeTable* attFieldTable;
	QueryServices* qsFieldRangServices;

	// Parametrage de la generation
	boolean bGenereModel;
	boolean bGenereView;
	boolean bGenereArrayView;
	boolean bGenereUserSection;
	ALString sOutputDir;
};
