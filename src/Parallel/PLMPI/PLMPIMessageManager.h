// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "PLSlaveState.h"
#include "PLErrorWithIndex.h"
#include "SortedList.h"
#include "Ermgt.h"

/////////////////////////////////////////////////////////////////////////////
// Classe PLMPIMessageManager
// Gestion des messages utilisateur par le maitre
// Permet de garantir que les messages sont affiches dans l'ordre du traitement (TaskIndex)
// (le premier esclave qui termine n'est pas necessairement le premier qui a commence, il faut donc stocker ses messages
// tant que le premier esclave n'a pas envoye les siens)
class PLMPIMessageManager : public Object
{
public:
	PLMPIMessageManager();
	~PLMPIMessageManager();

	void AddMessage(int nTaskIndex, PLErrorWithIndex* message);

	// Declaration d'une nouvelle tache
	// Doit etre appele avant d'appeler SetTaskLineNumberpour le meme index
	void AddTaskId(int nTaskIndex);

	// Affectation du  nombre de lignes lues pour une tache
	void SetTaskLineNumber(int nTaskIndex, longint lLocalLineNumber);

	// Affichage et nettoyage des messages dont la taskIndex est inferieure ou egale a nTaskIndexToPrint
	// N'envoie les erreurs que d'un seul esclave (celui dont la taskindex est la plus petite) les autres erreurs
	// sont ignorees
	void PrintMessages();

	boolean IsEmpty() const;

protected:
	// Tableau de MessageWithTaskIndex
	SortedList slMessages;

	// taskIndex des prochains messages a afficher
	int nTaskIndexToPrint;

	// Association des taskId avec le nombre de lignes lues
	LongintVector lvLines; // index = taskId / contenu = nombre de lignes

	// Nombre de lignes lues avant le lancement de la tache nTaskIndexToPrint
	longint lGlobalLineNumber;

	// Compteur du nombre d'ajout de messages pour indexer les messages
	int nAddIndex;

	// Tableau qui indique si on a atteind le max de message pour chaque type de gravite
	// Ca n'est pas un tableau de boolean car on doit envoyer un message en plus apres avoir atteind le max
	// C'est la seul facon d'avoir un affichage des "..." vers l'utilisateur.
	int nIsMaxErrorFlowReachedPerGravity[3];

	// Index de la tache pour laquelle on a affiche la premiere erreur
	int nTaskIndexError;
};

/////////////////////////////////////////////////////////////////////////////
// Classe MessageWithTaskIndex
// Message genere par un esclave associe au TaskIndex et au AddedIndex
class MessageWithTaskIndex : public Object
{
public:
	MessageWithTaskIndex();
	~MessageWithTaskIndex();

	// Acces a l'index de la tache des messages
	void SetTaskIndex(int nIndex);
	int GetTaskIndex() const;

	// Acces a l'index de l'ajout des messages
	void SetAddIndex(int nIndex);
	int GetAddIndex() const;

	void SetMessage(PLErrorWithIndex* error);
	PLErrorWithIndex* GetMessage();

	// Affichage du message
	// Si le message a un index different de -1, il est decore avec le numero de record (nStartingLineNumber +
	// index)
	void Print(longint lStartingLineNumber) const;

	void Write(ostream& ost) const override;

protected:
	PLErrorWithIndex* errorWithIndex;
	int nTaskIndex;
	int nAddIndex;
};

inline int MessageWithTaskIndex::GetTaskIndex() const
{
	return nTaskIndex;
}

inline void MessageWithTaskIndex::SetTaskIndex(int nIndex)
{
	nTaskIndex = nIndex;
}

inline void MessageWithTaskIndex::SetAddIndex(int nIndex)
{
	nAddIndex = nIndex;
}
inline int MessageWithTaskIndex::GetAddIndex() const
{
	return nAddIndex;
}

inline PLErrorWithIndex* MessageWithTaskIndex::GetMessage()
{
	return errorWithIndex;
}