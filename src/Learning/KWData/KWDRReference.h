// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDRReference;
class KWObjectReferenceResolver;

#include "Object.h"
#include "KWClass.h"
#include "KWObject.h"
#include "KWObjectKey.h"
#include "KWDerivationRule.h"

// Enregistrement de la regle de resolution de references
// Il s'agit d'une regle predefinie, indispensable a la gestion du multi-tables
void KWDRRegisterReferenceRule();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRReference
// Regle specifique pour gerer la reference a un objet principal d'un autre table
// Les operandes sont les noms de champ cle
// On retourne l'objet principal reference selon sa cle
class KWDRReference : public KWDerivationRule
{
public:
	// Constructeur
	KWDRReference();
	~KWDRReference();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const override;

	// Parametrage par un resolveur de reference, qui permet de retrouver un objet par sa cle
	// pour toute classe referencee
	// Ce parametrage est a la charge de l'appelant (en general, lors de certaines phase de la
	// lecture d'une base), qui correctement le parametrer (et l'inhiber par parametrage a NULL)
	// a chaque fois qu'il est necessaire de calculer une reference
	static void SetObjectReferenceResolver(KWObjectReferenceResolver* resolver);
	static KWObjectReferenceResolver* GetObjectReferenceResolver();

	// Verification de la compatibilite des operandes avec la cle de la classe referencee
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Affichage, ecriture dans un fichier, de l'usage de la regle
	void WriteUsedRule(ostream& ost) const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	static KWObjectReferenceResolver* objectReferenceResolver;
};

////////////////////////////////////////////////////////////////////////////
// Service de resolution des references aux objets pour lors de la lecture
// d'une base de donnees
// Permet de recherche un objet d'une classe donnees par sa cle
class KWObjectReferenceResolver : public Object
{
public:
	// Constructeur
	KWObjectReferenceResolver();
	~KWObjectReferenceResolver();

	// Supression (dereferencement) de toutes les classes et tous les objets
	void RemoveAll();

	// Supression (dereferencement) de toutes les classes et destruction de tous les objets
	void DeleteAll();

	//////////////////////////////////////////////////////////////////
	// Gestion des classes de donnees
	// Les classes sont referencees, et n'appartiennent pas a l'appele

	// Nombre de classes
	int GetClassNumber() const;

	// Ajout d'une nouvelle classe
	void AddClass(KWClass* kwcClass);

	// Recherche d'une classe par son nom
	// Renvoie NULL si classe absente
	KWClass* LookupClass(const ALString& sClassName) const;

	// Export des classes dans un tableau, triees par nom
	void ExportClasses(ObjectArray* oaClasses) const;

	//////////////////////////////////////////////////////////////////
	// Gestion des objets
	// Les objets sont references, et n'appartiennent pas a l'appele

	// Nombre de objets pour une classe
	int GetObjectNumber(KWClass* kwcClass) const;

	// Ajout d'un nouvel objet
	// La classe doit deja ete referencee
	void AddObject(KWClass* kwcClass, const KWObjectKey* objectKey, KWObject* kwoObject);

	// Dereferencement d'un objet (devant etre enregistre)
	void RemoveObject(KWClass* kwcClass, const KWObjectKey* objectKey);

	// Recherche d'un objet par sa cle
	// Renvoie NULL si classe absente
	KWObject* LookupObject(KWClass* kwcClass, const KWObjectKey* objectKey) const;

	// Export des objets d'une classes dans un tableau
	void ExportObjects(KWClass* kwcClass, ObjectArray* oaClassObjects) const;

	// Memoire utilisee par la classe pour stocker l'ensemble des objets
	longint GetUsedMemory() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Retourne une cle systeme pouvant servir d'identifiant unique, dans le cas mono-champ et multi-champs
	NUMERIC GetSymbolSystemKey(const KWObjectKey* objectKey) const;
	const ALString ComputeStringSystemKey(const KWObjectKey* objectKey) const;

	// Dictionnaire des classes
	// Chaque poste (index par un nom de classe) contient un KWClass
	ObjectDictionary odAllClasses;

	// Dictionnaire des objets de toutes les classes
	// Chaque poste (index par un ponteur sur une classe) contient un dictionnaire de tous les KWObject de la classe
	//   Cas des cles mono-champs: utilisation d'un NumericKeyDictionary indexe par la valeur Symbol de la cle
	//   Cas des cles multi-champs: utilisation d'un ObjectDictionary indexe par une valeur ALSTring "systeme"
	//   calculee
	NumericKeyDictionary nkdAllClassesObjects;
};
