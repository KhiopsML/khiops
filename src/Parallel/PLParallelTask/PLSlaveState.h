// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Object.h"
#include "Vector.h"
#include "PLErrorWithIndex.h"

////////////////////////////////////////////////////////////
// Classe PLSlaveState.
//	Cette classe permet de refleter l'etat d'un esclave.
//	Elle contient un etat, le rang et la progression
class PLSlaveState : public Object
{
public:
	// Constructeur
	PLSlaveState();
	~PLSlaveState();

	// (Re)Initialisation de l'esclave
	void Initialize();

	// Rang de l'esclave
	void SetRank(int nValue);
	int GetRank() const;

	// Nom de la machine sur laquelle est lance l'esclave
	void SetHostName(ALString sValue);
	ALString GetHostName() const;

	// Progression en pourcentage
	void SetProgression(int nValue);
	int GetProgression() const;

	// Proportion de la tache traite par l'esclave par rapport au job complet
	void SetTaskPercent(double dPercent);
	double GetTaskPercent() const;

	// Etats de l'esclave
	boolean IsVoid() const;
	boolean IsInit() const;
	boolean IsReady() const;
	boolean IsProcessing() const;
	boolean IsEnding() const;
	boolean IsFinalize() const;

	// Est-ce que l'esclave doit etre mis au repos
	// (independant du status, on peut decider de le mettre au repos alors qu'il est en court de traitement,
	//  lorsque le traitement sera termine il passera en ready...)
	void SetAtRest(boolean bRest);
	boolean GetAtRest() const;

	// Index de la tache
	void SetTaskIndex(int nIndex);
	int GetTaskIndex() const;

	// Est-ce que l'esclave a travaille
	boolean HasWorked() const;

	// Etats des esclaves
	enum State
	{
		VOID,       // SlaveInitialize pas effectuee
		INIT,       // En cours d'initialisation
		READY,      // Initialisation effectuee, peut travailler
		PROCESSING, // En slave Process
		FINALIZE,   // Finalization en cours
		DONE,       // Apres le finalize, ou en cas d'ereur
		UNKNOWN
	};
	void SetState(State nState);
	State GetState() const;
	const ALString PrintState() const;

	// Affiche sous forme de string l'etat passer en parametre
	static const ALString GetStateAsString(State nState);

	// Ecriture des attributs pour le debug
	void Write(ostream& ost) const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Rang
	int nRank;

	// Machine
	ALString sHostName;

	// Etat
	State state;

	// Progression dans la tache en cours
	int nProgression;

	// Pourcentage du job total alloue a l'esclave
	double dPercentOfTheJob;

	// Index de la tache en cours de traitement par l'esclave
	int nTaskIndex;

	// Est-ce que l'esclave a travaille
	boolean bHasWorked;

	// Est-ce qu el'esclave doit se reposer
	boolean bMustRest;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline int PLSlaveState::GetProgression() const
{
	return nProgression;
}

inline boolean PLSlaveState::IsVoid() const
{
	return state == VOID;
}

inline boolean PLSlaveState::IsInit() const
{
	return state == INIT;
}

inline boolean PLSlaveState::IsReady() const
{
	return state == READY;
}

inline boolean PLSlaveState::IsProcessing() const
{
	return state == PROCESSING;
}

inline boolean PLSlaveState::IsEnding() const
{
	return state == DONE;
}

inline boolean PLSlaveState::IsFinalize() const
{
	return state == FINALIZE;
}
inline boolean PLSlaveState::HasWorked() const
{
	return bHasWorked;
}

inline PLSlaveState::State PLSlaveState::GetState() const
{
	ensure(VOID <= state and state <= DONE);
	return state;
}
