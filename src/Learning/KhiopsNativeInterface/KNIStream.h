// Copyright (c) 2023-2026 Orange. All rights reserved.
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
	void SetClass(KWClass* kwcClass);
	KWClass* GetClass() const;

	// Acces a la base stream d'entree
	KWMTDatabaseStream* GetInputStream();

	// Acces a la base stream de sortie
	KWMTDatabaseStream* GetOutputStream();

	////////////////////////////////////////////////////////////////////////////////////
	// Gestion de la limite memoire utilisateur
	// Il s'agit de la memoire physique RAM, et non de la memoire logique
	// Pour estimer cette memoire de facon la plus precise possible, on doit enregistrer les
	// variation de memoire de la heap suite a chaque operation de gestion du stream, et emettre
	// des messages d'erreur en cas de depassement de la limite

	// Memoire physique utilisee pour l'ouverture du stream, a mettre a jour apres chaque etape de l'ouverture
	// - parametrage du stream (lignes d'entete...)
	// - lecture du dictionnaire
	// - compilation du dictionnaire
	// - ouverture de la base (qui comprend la creation des dictionnaire physique, et le chargement
	//   des tables externes
	void SetStreamOpeningUsedMemory(longint lValue);
	longint GetStreamOpeningUsedMemory() const;

	// Memoire physique utilise pour l'exploitation du stream, a mettre ajour apres chaque etape d'une recodage
	// - gestion des buffers d'enregistrement des record de l'objet en cours
	// - lecture effective pour recodage, avec potentiellement instance elephant
	void SetStreamRecodingUsedMemory(longint lValue);
	longint GetStreamRecodingUsedMemory() const;

	// Memoire disponible pour l'exploitation du stream apres son ouverture
	// Il s'agit de la moitie limite du stream diminue de la memoire necessaire a son ouverture
	longint GetStreamRecodingAvailableMemory() const;

	// Memoire disponible pour le stockage des records dans des buffers en vue du recodage
	// Il s'agit de la moitie de la memoire disponible pour le recodage
	longint GetStreamRecodingAvailableBufferMemory() const;

	// Memoire disponible pour la creation du KWObject a partir des buffers en vue du recodage
	// Il s'agit de la moitie de la memoire disponible pour le recodage
	longint GetStreamRecodingAvailableComputationMemory() const;

	// Memoire physique minimale reservee pour le recodage
	longint GetStreamMinimumRecodingMemoryLimit() const;

	// Memoire disponible totale en octets, issu d'une simple conversion de GetStreamMemoryLimit
	longint GetStreamActualMemoryLimit() const;

	// Parametrage de la memoire disponible totale en MB pour la gestion du stream
	// (dictionnaire, tables externes, buffer des fichiers)
	// Valeur par defaut: KNI_DefaultMaxStreamMemory
	// Ne peut etre modifie que quand le stream n'est pas ouvert
	void SetStreamMemoryLimit(int nValue);
	int GetStreamMemoryLimit() const;

	// Modification de la memoire alouee a l'ensemble de tous les stream
	static void SetAllStreamsMemoryLimit(int nValue);
	static int GetAllStreamsMemoryLimit();

	// Memoire utilisee par le stream pour son fonctionnement, hors dictionnaire
	// Cette methode est potentiellement couteuse en temps de calcul
	// Il s'agit ici de la reimplementation de la method standard, pour la memoire logique
	longint GetUsedMemory() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Gestion du stream KNI
	KWClass* streamClass;
	KWMTDatabaseStream inputStream;
	KWMTDatabaseStream outputStream;

	// Memoire physique utilisee pour l'ouverture du stream
	longint lStreamOpeningUsedMemory;

	// Memoire physique utilisee pour le recordage d'une instance
	longint lStreamRecodingUsedMemory;

	// Limite memoire du stream
	int nStreamMemoryLimit;

	// Limite memoire pour l'ensemble des stream
	static int nAllStreamsMemoryLimit;
};

///////////////////////////////////
// Methodes en inline

inline void KNIStream::SetClass(KWClass* kwcClass)
{
	streamClass = kwcClass;
}

inline KWClass* KNIStream::GetClass() const
{
	return streamClass;
}

inline KWMTDatabaseStream* KNIStream::GetInputStream()
{
	return &inputStream;
}

inline KWMTDatabaseStream* KNIStream::GetOutputStream()
{
	return &outputStream;
}
