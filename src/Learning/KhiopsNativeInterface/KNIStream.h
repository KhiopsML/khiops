// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KhiopsNativeInterface.h"
#include "KWMTDatabaseStream.h"

////////////////////////////////////////////////////////////////////
// Stream KNI, dediee a l'implementation de l'interface KNI
class KNIStream : public Object
{
public:
	// Constructeur
	KNIStream();
	~KNIStream();

	// Classe de gestion du stream
	// Memoire: appartient a l'appele
	inline void SetClass(KWClass* kwcClass)
	{
		streamClass = kwcClass;
	}
	inline KWClass* GetClass() const
	{
		return streamClass;
	}

	// Acces a la base stream d'entree
	inline KWMTDatabaseStream* GetInputStream()
	{
		return &inputStream;
	}

	// Acces a la base stream de sortie
	inline KWMTDatabaseStream* GetOutputStream()
	{
		return &outputStream;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Gestion de la limite memoire

	// Memoire utilisee par le stream pour son fonctionnement
	// (dictionnaire, et stream en input et output)
	// Cette methode est potentiellement couteuse en temps de calcul
	longint GetUsedMemory() const override;

	// Gestion utilisateur de la memoire en MB utilisee par le stream
	void SetStreamUsedMemory(longint lValue);
	longint GetStreamUsedMemory() const;

	// Memoire totale effectivement utilisable pour le stream en octet (en tenant compte d'un overhead d'allocation)
	longint GetStreamAvailableMemory() const;

	// Parametrage de la memoire disponible totale en MB pour la gestion du stream
	// (dictionnaire, tables externes, buffer des fichiers)
	// Valeur par defaut: KNI_DefaultMaxStreamMemory
	// Ne peut etre modifie que quand le stream n'est pas ouvert
	void SetStreamMemoryLimit(int nValue);
	int GetStreamMemoryLimit() const;

	// Modification de la memoire alouee a l'ensemble de tous les stream
	static void SetAllStreamsMemoryLimit(int nValue);
	static int GetAllStreamsMemoryLimit();

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Gestion du stream KNI
	KWClass* streamClass;
	KWMTDatabaseStream inputStream;
	KWMTDatabaseStream outputStream;

	// Limite memoire du stream
	int nStreamMemoryLimit;

	// Memoire actuellement utilisee par le stream, en MB
	longint lStreamUsedMemory;

	// Limite memoire pour l'ensemble des stream
	static int nAllStreamsMemoryLimit;
};