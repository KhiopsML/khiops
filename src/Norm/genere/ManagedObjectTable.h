// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Ermgt.h"
#include "ManagedObject.h"
#include "FileService.h"

////////////////////////////////////////////////////////////
// Classe ManagedObjectTable
//    Table de d'objet geres
//    Fonctionnalites analogues a celles d'une base de donnees
//      - chargement et dechargement vers un fichier
//      - insertion, recherche, suppression, parcours des objets
class ManagedObjectTable : public Object
{
public:
	// Constructeur
	// L'objet classObject sera reference par la table tout au
	// long de son existence. Il permet de parametrer le
	// comportement de la table de'objets geres

	ManagedObjectTable(ManagedObject* theManagedObjectClass);
	~ManagedObjectTable();

	// Exemplaire d'objet gere par la table
	ManagedObject* GetManagedObjectClass();

	////////////////////////////////////////////////////////
	// Chargement et dechargement depuis un fichier

	// Chargement de tous les attributs stockes
	boolean Load(fstream& fst);
	boolean Load(const ALString& sFileName);

	// Chargement de tous les attributs stockes disponibles
	// dans un fichier. La disponibilite d'un attribut est
	// determinee par la presence de son libelle de colonne
	// dans la ligne d'en-tete du fichier.
	// Les champs specifie par leur index generique indiquent
	// les attributs obligatoires
	boolean LoadFields(fstream& fst, IntVector* ivMandatoryFieldIndexes);
	boolean LoadFields(const ALString& sFileName, IntVector* ivMandatoryFieldIndexes);

	// Dechargement de tous les attributs stockes
	void Unload(ostream& ost);
	void Unload(const ALString& sFileName);

	// Dechargement d'attributs (stockes ou non)
	// Les champs sont specifie par leur index generique
	// On peut utiliser l'index -1 pour ignorer une position
	void UnloadFields(ostream& ost, IntVector* ivFieldIndexes);
	void UnloadFields(const ALString& sFileName, IntVector* ivFieldIndexes);

	/////////////////////////////////////////////////////////
	// gestion de la ligne d'entete du fichier
	// Ces methodes permettent une personnalisation facile
	// de l'analyse d'un fichier.

	// Parametrage de l'utilisation de la premiere ligne pour les
	// operations de chargement (true par defaut)
	void SetUsingHeaderLine(boolean bValue);
	boolean GetUsingHeaderLine() const;

	// Chargement de la ligne d'en-tete
	// On remplit les index des champs de l'en-tete
	// On renvoie la chaine de caractere du fichier  contenant
	// l'en-tete.
	char* LoadHeaderLine(fstream& fst, IntVector* ivHeaderLoadedFieldIndexes);

	// Verification (avec emission eventuelle de messages
	// d'erreur qu'une liste de champs contient les
	// champs obligatoires passes en parametres
	boolean CheckMandatoryFields(IntVector* ivFieldIndexes, IntVector* ivMandatoryFieldIndexes);

	// Dechargement d'une ligne d'en-tete de fichier
	// (sans le retour charriot)
	void UnloadHeaderLine(ostream& ost, IntVector* ivFieldIndexes);

	/////////////////////////////////////////////////////////
	// Gestion du contenu de la table en memoire

	// Recherche (retourne NULL si echec)
	ManagedObject* Lookup(const ALString& sKey) const;

	// Insertion
	boolean Insert(ManagedObject* newObject);

	// Supression
	boolean Remove(const ALString& sKey);

	// Acces massifs
	// Attention: l'index utilise pour les acces massifs
	// est recalcule des que la table a ete modifiee
	int GetSize() const;
	ManagedObject* GetAt(int i) const;

	// Parametrage du tri des acces massifs
	// Par defaut NULL (dans ce cas, on tri sur la cle)
	void SetCompareFunction(CompareFunction fCompare);
	CompareFunction GetCompareFunction();

	// Nettoyage de la table
	void RemoveAll(); // Supression de tous les objets
	void DeleteAll(); // Supression et destruction de tous les objets

	// Verification de l'integrite de tous les elements
	// du container (appel de la methode Check() de ces elements)
	boolean Check() const override;

	// Export vers les containers algorithmiques
	// Les containers tableaux et listes obtenus sont
	// tries d'apres la cle des objets contenus
	// Memoire: le contenu precedent du container resultat n'est plus reference (mais pas detruit)
	void ExportObjectArray(ObjectArray* oaResult) const;
	void ExportObjectDictionary(ObjectDictionary* odResult) const;
	void ExportObjectList(ObjectList* olResult) const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Table des objets, pour les acces indexes
	ObjectArray* GetAllObjects();

	// Utilisation de la ligne de header
	boolean bUsingHeaderLine;

	// Gestion de l'index, base sur les champs de la cle
	ObjectDictionary dicKeyIndex;

	// Compteur de mise a jour
	// Permet de verfier la "fraicheur" des requetes
	int nUpdateNumber;

	//  Gestion des requetes
	// (le compteur de mise a jour a ete enregistre au moment du calcul
	// des requetes, pour gerer leur bufferisation)
	ObjectArray oaAllObjects;
	int nAllObjectsFreshness;

	// Exemplaire du ManagedObject gere, permettant
	// l'implementation generique des traitements
	ManagedObject* managedObjectClass;
};
