// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWAttributeSpec.h"
#include "KWAttributeSpecView.h"

// ## Custom includes

#include "KWClassDomain.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWAttributeSpecArrayView
//    Variable
// Editeur de tableau de KWAttributeSpec
class KWAttributeSpecArrayView : public UIObjectArrayView
{
public:
	// Constructeur
	KWAttributeSpecArrayView();
	~KWAttributeSpecArrayView();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Creation d'un objet (du bon type), suite a une demande d'insertion utilisateur
	Object* EventNew() override;

	// Mise a jour de l'objet correspondant a l'index courant par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet correspondant a l'index courant
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Parametrage de la classe editee
	void SetEditedClass(KWClass* kwcClass);

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Construction des informations permettant de savoir pour chaque attribut d'une classe s'il est utilise la cle
	void BuildKeyAttributes(KWClass* kwcClass, NumericKeyDictionary* nkdKeyAttributes) const;

	// Construction des informations permettant de savoir pour chaque attribut d'une classe s'il est utilise
	// dans une regle de derivation par un data item (attribut ou un bloc d'attribut)
	// Dans le dictionnaire en sortie, la cle est le data item utilise, et la valeur un data item referencant
	// le data item utilise au moyen d'une regle
	void BuildClassUsedAttributeReferences(KWClass* kwcClass,
					       NumericKeyDictionary* nkdUsedAttributeReferences) const;

	// Idem pour une regle de derivation
	// Comme on n'analyse pas les regles attributs parametres de regles, il n'y a pas de risque de recursion infinie
	void BuildRuleUsedAttributeReferences(KWDerivationRule* rule, KWDataItem* referenceDataItem,
					      NumericKeyDictionary* nkdUsedAttributeReferences) const;

	// Classe editee
	KWClass* kwcEditedClass;

	// Dictionnaires des infortion d'utilisation des attributs de la classe editees (dans cle ou dans regles de
	// derivation) pour permettre la correction des informations de types a la volee
	NumericKeyDictionary nkdEditedClassKeyAttributes;
	NumericKeyDictionary nkdEditedClassUsedAttributeReferences;

	// Liste des types d'attributs stockes, pour les message d'erreur
	ALString sStoredTypes;

	// ##
};

// ## Custom inlines

// ##
