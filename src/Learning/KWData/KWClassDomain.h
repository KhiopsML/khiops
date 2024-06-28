// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWClass;
class KWAttribute;
class KWObject;
class KWClassDomain;
class KWDerivationRule;

#include "Object.h"
#include "ALString.h"
#include "Ermgt.h"
#include "FileService.h"
#include "KWClass.h"
#include "JSONFile.h"
#include "KWCDUniqueString.h"
#include "MemoryStatsManager.h"
#include "KWResultFilePathBuilder.h"
#include "KWVersion.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KWClassDomain
// Domaine de KWClass
//
// Un domaine de KWClass est un ensemble coherent de KWClass. Les domaines
// sont independants, et forment une partition des KWClass.
// On peut utiliser egalement les domaines de KWClass pour versionner les KWClass,
// differentes version d'une meme KWClass se retrouvant dans plusieurs domaines
// sous le meme nom.
//
// On trouve dans KWClassDomain les familles de fonctionnalites suivantes:
//		. gestion des KWClass d'un domaine
//		. gestion des domaines et d'un domaine courant (methodes de classes)
//
// L'essentiel des manipulations doivent se faire par rapport au domaine courant
class KWClassDomain : public Object
{
public:
	// Constructeur
	KWClassDomain();
	~KWClassDomain(); // Toutes les KWClass sont detruites a la destruction

	/////////////////////////////////////////////////////////////////
	// Gestion des KWClass d'un domaine

	// Nom d'un domaine
	// Le SetName correspond a un renommage du domaine si le domaine
	// est enregistre globalement. Dans ce cas, la methode peut
	// echouer si le nom est deja utilise (et ne rien faire)
	void SetName(const ALString& sValue);
	const ALString& GetName() const;

	// Libelle
	void SetLabel(const ALString& sValue);
	const ALString& GetLabel() const;

	// Lecture d'un fichier
	boolean ReadFile(const ALString& sFileName);

	// Ecriture dans un fichier
	boolean WriteFile(const ALString& sFileName) const;

	// Ecriture dans un fichier d'une classe avec toutes ses classes dependantes
	boolean WriteFileFromClass(const KWClass* rootClass, const ALString& sFileName) const;

	// Ecriture dans un fichier au format JSON
	boolean WriteJSONFile(const ALString& sJSONFileName) const;

	// Recherche (retourne NULL si echec)
	KWClass* LookupClass(const ALString& sClassName) const;

	// Insertion d'une nouvelle classe
	void InsertClass(KWClass* newObject);

	// Insertion avec nouveau non (ce nom ne doit pas deja exister)
	// Apres insertion, la classe a changee de nom
	void InsertClassWithNewName(KWClass* newObject, const ALString& sNewName);

	// Supression
	void RemoveClass(const ALString& sClassName);

	// Supression et destruction
	void DeleteClass(const ALString& sClassName);

	// Renommage d'une classe
	// Le nom cible ne doit pas exister
	// Propagation a toutes les utilisations dans les regles de derivation
	void RenameClass(KWClass* refClass, const ALString& sNewClassName);

	// Acces massifs
	int GetClassNumber() const;
	KWClass* GetClassAt(int i) const;

	// Export vers un container de classes (appartenant topujours au domaine)
	// Memoire: le contenu precedent du container resultat n'est plus reference (mais pas detruit)
	void ExportClassArray(ObjectArray* oaResult) const;
	void ExportClassDictionary(ObjectDictionary* odResult) const;

	// Nettoyage du domaine de KWClass
	void RemoveAllClasses(); // Supression de tous les objets KWClass
	void DeleteAllClasses(); // Supression et destruction de tous les objets KWClass

	// Calcul d'un nouveau nom de dictionnaire a partir d'un prefix
	const ALString BuildClassName(const ALString& sPrefix);

	// Verification de toutes les KWClass d'un domaine
	boolean Check() const override;

	// Completion eventuelle des attributs avec les informations de type de leur regle de derivation
	// pour toutes les classes du domaine
	// L'appel de cette methode est necessaire au niveau du domaine en cas de dependance entre classes
	void CompleteTypeInfo();

	// Compilation de toutes les classes d'un domaine
	// Il s'agit de la compilation des regles de derivation
	// Toute compilation devrait se faire par cette methode, car une classe
	// peut avoir des dependances avec d'autres classes via des regles de derivation.
	// La compilation du domaine verifie egalement la presence de cycles
	// de derivations, qui dans ce cas annule la validite des classes
	// Prerequis: la classe doit etre valide (Check)
	boolean Compile();

	// Duplication d'un domaine
	// Toutes les classes sont dupliquees
	// Toutes les classes referencees par des attributs
	// sont celles du nouveau domaine (si possible)
	KWClassDomain* Clone() const;

	// Duplication partielle d'un domaine a partir d'une classe
	// La classe et ses classes referencees recursivement sont dupliquees de facon coherente
	KWClassDomain* CloneFromClass(const KWClass* rootClass) const;

	// Import de toutes les classes d'un domaine, en y ajoutant un prefixe  et un suffixe
	// et en les renommant si necessaire
	// Le domaine source est vide a l'issue de l'import
	void ImportDomain(KWClassDomain* kwcdInputDomain, const ALString& sClassPrefix, const ALString& sClassSuffix);

	// Calcul de l'ensemble des classes (y compris la racine) dependante d'une classe
	// Le resultat est un dictionnaire referencant les classes resultats par leur nom
	void ComputeClassDependence(const KWClass* rootClass, ObjectDictionary* odDependentClasses) const;

	// Memoire utilisee par le domaine
	longint GetUsedMemory() const override;

	// Memoire utilisee par la classe et toutes ses classes referencees recursivement
	longint GetClassDependanceUsedMemory(const KWClass* rootClass) const;

	// Cle de hashage du domaine et de sa composition
	longint ComputeHashValue() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Methodes de test
	static void TestReadWrite(const ALString& sReadFileName, const ALString& sWriteFileName);
	static void Test();

	////////////////////////////////////////////////////////////////////////
	// Gestion des domaines de KWClass
	// Methodes de classe

	// Gestion d'un domaine courant
	// Il y a toujours un domaine courant. Meme si aucun domaine n'a encore ete
	// cree explicitement ou si on a indique NULL comme domaine courant, le domaine
	// "Root" sera cree automatiquement et deviendra le domaine courant
	// Le domaine courant ne fait par necessairement partie des domaines geres,
	// ce qui permet de parametrer un domaine temporaire comme domaine courant
	static KWClassDomain* GetCurrentDomain();
	static void SetCurrentDomain(KWClassDomain* kwcdDomain);

	// Recherche (retourne NULL si echec)
	static KWClassDomain* LookupDomain(const ALString& sName);

	// Insertion
	static void InsertDomain(KWClassDomain* newObject);

	// Supression
	static void RemoveDomain(const ALString& sName);

	// Supression et destruction
	static void DeleteDomain(const ALString& sName);

	// Renommage du domaine
	// Retourne true si OK. Sans effet si le nom cible existe deja.
	static void RenameDomain(KWClassDomain* domain, const ALString& sNewName);

	// Acces massifs
	static int GetDomainNumber();
	static KWClassDomain* GetDomainAt(int i);

	// Nettoyage de tous les domaines de KWClass
	static void RemoveAllDomains(); // Supression de tous les objets KWClassDomain
	static void DeleteAllDomains(); // Supression et destruction de tous les objets KWClassDomain

	// Memoire utilisee par tous les domaines
	static longint GetAllDomainsUsedMemory();

	// Calcul d'un nouveau nom de domaine a partir d'un prefix
	static const ALString BuildDomainName(const ALString& sPrefix);

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	//////////////////////////////////////////////////////
	// Gestion des KWClass d'un domaine

	// Nom
	KWCDUniqueString usName;

	// Libelle
	KWCDUniqueString usLabel;

	// Dictionnaire des classes du domaine
	ObjectDictionary odClasses;

	// Gestion indexee des classes d'un domaine
	ObjectArray* AllClasses() const;
	mutable ObjectArray oaClasses;

	// Compteur de mise a jour
	// Permet de verifier la "fraicheur" de l'indexation des classes
	int nUpdateNumber;

	//  Fraicheur d'indexation des classes
	mutable int nAllClassesFreshness;

	//////////////////////////////////////////////////////
	// Gestion des KWClassDomain

	// Domaine courant
	static KWClassDomain* kwcdCurrentDomain;

	// Dictionnaire des domaines
	static ObjectDictionary* odDomains;

	// Gestion indexee des domaines
	static ObjectArray* CDAllClassDomains();
	static ObjectArray* oaDomains;

	// Compteur de mise a jour
	// Permet de verifier la "fraicheur" de l'indexation des domaines
	static int nCDUpdateNumber;

	//  Fraicheur d'indexation des domaines
	static int nCDAllClassDomainsFreshness;

	//////////////////////////////////////////////////////////////
	// Service de memorisation du scope, partage par l'ensemble
	// des classe d'un domaine
	// Lorsque la gestion du scope est necessaire (test de validite, compilation...),
	// les classes disposent de service de gestion du scope, partage par
	// l'ensemble des classe d'un meme domaine, qui en assure la memorisation

	friend class KWDerivationRule;
	friend class KWDerivationRuleOperand;

	// Acces aux tableau des classe et regles du scope
	ObjectArray* GetScopeClasses() const;
	ObjectArray* GetScopeRules() const;

	// Tableaux des classes et regles pour les scopes de niveau superieur
	// Les scopes empiles sont en ordre inverse dans le tableau
	mutable ObjectArray oaScopeClasses;
	mutable ObjectArray oaScopeRules;
};

// Fonction de comparaison sur le nom d'une KWClass
int KWClassCompareName(const void* first, const void* second);

// Fonction de comparaison sur le nom d'un KWClassDomain
int KWClassDomainCompareName(const void* first, const void* second);

///////////////////////
// Methodes en inline

inline ObjectArray* KWClassDomain::GetScopeClasses() const
{
	return &oaScopeClasses;
}

inline ObjectArray* KWClassDomain::GetScopeRules() const
{
	return &oaScopeRules;
}
