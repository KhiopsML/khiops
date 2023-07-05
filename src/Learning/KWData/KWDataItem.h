// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "ALString.h"
#include "Object.h"
#include "JSONFile.h"

//////////////////////////////
// Donnee elementaire d'un dictionnaire
//  . soit attribut dense
//  . soit bloc d'attributs sparse
class KWDataItem : public Object
{
public:
	// Methode indiquant la nature de la donnees
	virtual boolean IsAttribute() const = 0;
	boolean IsAttributeBlock() const;

	// Nom du data item
	virtual const ALString& GetName() const = 0;

	////////////////////////////////////////////////////////
	// Gestion d'un rapport JSON

	// Ecriture du contenu d'un rapport JSON
	virtual void WriteJSONFields(JSONFile* fJSON);

	// Ecriture d'un rapport JSON
	// Avec une cle s'il est dans un objet, sans cle s'il est dans un tableau
	virtual void WriteJSONReport(JSONFile* fJSON);
	virtual void WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey);
};

inline boolean KWDataItem::IsAttributeBlock() const
{
	return not IsAttribute();
}