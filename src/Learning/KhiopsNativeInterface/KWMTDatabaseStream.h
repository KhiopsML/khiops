// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWMTDatabase.h"
#include "KWDataTableDriverStream.h"
#include "KWDataTableDriverTextFile.h"

//////////////////////////////////////////////////////////////////////
// Base simple-table dans gere dans un stream
class KWMTDatabaseStream : public KWMTDatabase
{
public:
	// Constructeur
	KWMTDatabaseStream();
	~KWMTDatabaseStream();

	// Creation dynamique
	KWDatabase* Create() const override;

	// Nom de la technologie de base de donnees
	// Ici: table simple en mode fichier stream
	ALString GetTechnologyName() const override;

	///////////////////////////////////////////////////////////
	// Parametrage de la structure du stream

	// Ligne d'entete pour une table secondaire identifiee par son DataPath
	void SetHeaderLineAt(const ALString& sDataPath, const ALString& sHeaderLine);
	const ALString GetHeaderLineAt(const ALString& sDataPath) const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;

	///////////////////////////////////////////////////////
	// Lecture/ecriture a partir d'un buffer

	// Preparation d'un record secondaire, prealable a la lecture d'un objet
	// Retaillage eventuel du buffer interne (par doublement de taille) si necessaire
	// Renvoie false en cas de depassement du buffer interne du mapping
	boolean SetSecondaryRecordAt(KWMTDatabaseMapping* mapping, const char* sRecord);

	// Nombre de records secondaires rencontres
	void SetSecondaryRecordNumber(longint lValue);
	longint GetSecondaryRecordNumber() const;

	// Nombre d'erreurs sur les records secondaires, a gerer explicitement par l'appelant
	// On peut memoriser une telle erreur, tenter une reprise sur erreur si possible
	void SetSecondaryRecordErrorNumber(longint lValue);
	longint GetSecondaryRecordErrorNumber() const;

	// Taille courante du buffer d'un mapping
	// Permet de controler la taille du buffer par mapping dynamiquement,
	// pour la prochaine utilisation du mapping, par exemple pour tenter
	// de preparer a nouveau un record secondaire apres agrandissement du buffer
	void SetMappingMaxBufferSize(KWMTDatabaseMapping* mapping, int nSize);
	int GetMappingMaxBufferSize(KWMTDatabaseMapping* mapping) const;

	// Lecture d'une instance depuis un buffer
	// Repositionne les flag d'erreur a false
	// Renvoie NULL si echec
	KWObject* ReadFromBuffer(const char* sBuffer);

	// Ecriture d'un instance dans un buffer
	// Renvoie false si pas assez de place dans le buffer
	boolean WriteToBuffer(KWObject* kwoObject, char* sBuffer, int nMaxBufferLength);

	// Parametrage de la taille max des buffers au moment de l'ouverture de la base
	void SetMaxBufferSize(int nValue);
	int GetMaxBufferSize() const;

	// Liberation de la memoire des buffers des tables secondaires
	// Utile notament si on a consomme trop de memoire pour une instance, et que l'on veut repartir
	// de l'etat initial et reallouant les buffers au fur et a mesure
	void FreeSecondaryTableBuffers();

	// Memoire utilisee par la database pour son fonctionnement (reimplementation)
	longint GetUsedMemory() const override;

	////////////////////////////////////////////////////////////////////////////////////////////
	// Redefinition minimal de quelque methode pour intercepter un eventuel depassement memoire
	// lors de la lecture des tables externes, afin de specialiser les messages d'erreurs

	// Redefinition de l'ouverture
	boolean PhysicalOpenForRead() override;

	// Lecture et chargement en memoire des tables des objets references
	// Redefinition uniquement pour memorise l'information de depassement de limite
	boolean PhysicalReadAllReferenceObjects(longint lMaxAvailableMemory, longint& lNecessaryMemory,
						longint& lTotalExternalObjectNumber,
						boolean& bMemoryLimitReached) override;

	// Indique si la derniere lecture des tables externes a declenche un depassement memoire
	boolean IsMemoryLimitReachedDuringLastReadAllReferenceObjects() const;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Nombre total de records secondaires
	longint lSecondaryRecordNumber;

	// Nombre d'erreurs sur les records secondaires
	longint lSecondaryRecordErrorNumber;

	// Creation de driver, a l'usage des mappings des tables principales et secondaires
	// Parametrage a la volee des headerlines des driver de type stream
	KWDataTableDriver* CreateDataTableDriver(KWMTDatabaseMapping* mapping) const override;

	// Memorisation des headerlines par DataPath (dans des StringObject)
	ObjectDictionary odHeaderLines;

	// Indiqcateur de depassement memoire durant la derniere lecture des tables externes
	boolean bIsMemoryLimitReachedDuringLastReadAllReferenceObjects;
};
