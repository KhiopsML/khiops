// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"
#include "OutputBufferedFile.h"
#include "PLSharedObject.h"

class PLTracer;
class PLShared_Tracer; // serialisatuion de la configuration

///////////////////////////////////////////////////////////////////////////////
// Classe PLTracer
// Service de traces pour le debuggage
class PLTracer : public Object
{
public:
	// Constructeur
	PLTracer();
	~PLTracer();

	// Copie
	void CopyFrom(const PLTracer*);

	// Duplication
	PLTracer* Clone() const;

	// Affiche les messages dans la console des qu'ils sont ajoutes
	// Sinon ils sont stockes (par defaut false)
	void SetSynchronousMode(boolean bValue);
	boolean GetSynchronousMode() const;

	// Ajoute un timestamp a chaque message (par defaut true)
	void SetTimeDecorationMode(boolean bValue);
	boolean GetTimeDecorationMode() const;

	// Message abrege ou normal
	void SetShortDescription(boolean bShort);
	boolean GetShortDescription() const;

	// Mode passif ou actif
	//	si le mode est passif, le tracer ne fait strictement rien
	//	actif par defaut
	void SetActiveMode(boolean bValue);
	boolean GetActiveMode() const;

	// Ajoute un message simple
	void AddTrace(const ALString&);

	// Ajoute un message simple avec un resultat de test
	void AddTrace(const ALString&, boolean ret);

	// Ajout eun messag simple avec une duree
	void AddTraceWithDuration(const ALString&, double dElapsedTime);

	// Affiche tous les messages dans la console
	void PrintTraces() const;

	// Ecrit tous les messages dans un fichier
	void PrintTracesToFile() const;

	// Supprime toutes les traces
	void Clean();

	// Fichier dans lequel les traces seront ecrites (concatenation des differentes taches)
	// Le fichier est detruit si deja existant
	void SetFileName(const ALString& sLogFileName);
	const ALString& GetFileName() const;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Fonction utilitaire qui decore le message convertit  avant l'insertion dans le tableau
	void AddTraceAsString(const ALString& sTrace);

	// Decoration du message commun a tous les messages
	// cette methode est systematiquement appalee a chaque ajout de message
	virtual ALString Decoration(const ALString& sMessage);

	// Stockage toutes les traces
	StringVector traces;

	// Mode synchrone
	boolean bSynchronous;

	// Ajout de timesatmp
	boolean bTimeDecoration;

	// Actif ou non
	boolean bActive;

	// Fichier de trace
	ALString sFileName;

	// Abrege ou long
	boolean bShortDescription;

	friend class PLShared_Tracer;
};

///////////////////////////////////////////////////////////////////////////////
// Classe PLShared_Tracer
// Serialisation des parametre d'un tracer
class PLShared_Tracer : public PLSharedObject
{
public:
	// Constructeur
	PLShared_Tracer();
	~PLShared_Tracer();

	// Acces au PLTracer
	void SetTracer(PLTracer* tracer);
	PLTracer* GetTracer();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline void PLTracer::AddTraceAsString(const ALString& sTrace)
{
	ALString sDecoratedTrace;

	if (not bActive)
		return;

	sDecoratedTrace = Decoration(sTrace);
	if (bSynchronous)
	{
		cout << sDecoratedTrace << endl;
	}
	else
	{
		traces.Add(sDecoratedTrace);
	}
}

inline boolean PLTracer::GetSynchronousMode() const
{
	return bSynchronous;
}

inline boolean PLTracer::GetActiveMode() const
{
	return bActive;
}

inline const ALString& PLTracer::GetFileName() const
{
	return sFileName;
}

inline boolean PLTracer::GetTimeDecorationMode() const
{
	return bTimeDecoration;
}

inline ALString PLTracer::Decoration(const ALString& sMessage)
{
	ALString sRes;

	sRes = sRes + "[" + IntToString(GetProcessId()) + "]";

	if (bTimeDecoration)
	{
		sRes += "\t";
		sRes += ALString(CurrentPreciseTimestamp());
	}
	return sRes + "\t" + sMessage;
}

inline void PLTracer::SetShortDescription(boolean bShort)
{
	bShortDescription = bShort;
}

inline boolean PLTracer::GetShortDescription() const
{
	return bShortDescription;
}
