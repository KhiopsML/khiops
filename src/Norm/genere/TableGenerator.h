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

	//// Attributs

	// Nom de la classe a generer
	const ALString& GetClassName() const;
	void SetClassName(const ALString& sValue);

	// Nom de la classe mere de la classe a generer
	const ALString& GetSuperClassName() const;
	void SetSuperClassName(const ALString& sValue);

	// Libelle utilisateur de la classe a generer
	const ALString& GetClassUserLabel() const;
	void SetClassUserLabel(const ALString& sValue);

	//// Parametrage de ce qui est a generer (true par defaut)

	// Generation des fonctionnalites de gestion des objets
	// (possible si aucun attribut n'est stocke)
	void SetGenereManagement(boolean bValue);
	boolean GetGenereManagement() const;

	// Generation des fonctionnalites d'interface graphique
	// (GenereView==false => genereArrayView== false)
	void SetGenereView(boolean bValue);
	boolean GetGenereView() const;

	// Generation des fonctionnalites d'interface graphique
	// (GenereArrayView==false => GenereView==false)
	void SetGenereArrayView(boolean bValue);
	boolean GetGenereArrayView() const;

	// Generation de sections utilisateurs
	void SetGenereUserSection(boolean bValue);
	boolean GetGenereUserSection() const;

	//// Methodes de generation
	void Genere() const;
	void GenereWith(const ALString& sName, const ALString& sSuperName, const ALString& sLabel,
			const ALString& sAttributeFileName);

	// Generation de doc (provisoire)
	void GenereDoc(ostream& ost) const;

	///// Implementation
protected:
	// Acces aux attributs de type Field, dans le bon ordre
	int GetFieldNumber() const;
	Attribute* GetFieldAt(int i) const;

	// Acces a tous les attributs (Field et Label), dans le bon ordre
	int GetAttributeNumber() const;
	Attribute* GetAttributeAt(int i) const;

	// Attributs de la classe a generer
	AttributeTable* GetAttributeTable() const;
	void SetAttributeTable(AttributeTable* attTable);

	// Attributs de base
	ALString sClassName;
	ALString sClassUserLabel;
	ALString sSuperClassName;
	AttributeTable* attAttributeTable;
	QueryServices* qsAttributeRangServices;
	AttributeTable* attFieldTable;
	QueryServices* qsFieldRangServices;

	// Parametrage de la generation
	boolean bGenereManagement;
	boolean bGenereView;
	boolean bGenereArrayView;
	boolean bGenereUserSection;

	// Gestion des erreurs
	void Error(const ALString& sMessage) const;

	// Nom des fichiers de backup
	const ALString Backup(const ALString& sFileName, const ALString& sWhich) const;

	// Gestion de la consolidation des fichiers utilisateurs et generes
	void ConsolidateFiles(const ALString& sFileName) const;

	//// Methodes de generation

	// Utilitaires de generation
	void GenerateFileHeader(ostream& ost) const;
	void GenereClassHeaderComment(ostream& ost, const ALString& sClassFamily) const;
	void GenereTitledComment(ostream& ost, const ALString& sIndent, const ALString& sComment) const;
	void GenereImplementationComment(ostream& ost) const;
	void GenerateUserCodeSection(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const;
	void GenerateUserCodeHeader(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const;
	void GenerateUserCodeTrailer(ostream& ost, const ALString& sIndent, const ALString& sIdentifier) const;

	// Generation des tables d'attribut
	void GenerateAttributeH(ostream& ost) const;
	void GenerateAttributeC(ostream& ost) const;
	void GenerateAttributeTableH(ostream& ost) const;
	void GenerateAttributeTableC(ostream& ost) const;

	// Generation des composants d'interface utilisateur et du menu de query
	void GenerateAttributeViewH(ostream& ost) const;
	void GenerateAttributeViewC(ostream& ost) const;
	void GenerateAttributeArrayViewH(ostream& ost) const;
	void GenerateAttributeArrayViewC(ostream& ost) const;

	// Generation des services de statistiques sur les attributs des tables
	void GenerateAttributeTableHStats(ostream& ost) const;
	void GenerateAttributeTableHStatsImp(ostream& ost) const;
	void GenerateAttributeTableHStatsCompare(ostream& ost) const;
	void GenerateAttributeTableCStats(ostream& ost) const;
};
