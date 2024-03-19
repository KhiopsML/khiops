// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPIMessageManager.h"

///////////////////////////////////////////////////////////////////////
// Implementation de la classe PLMPIMessageManager

int MessageWithTaskIndexCompare(const void* elem1, const void* elem2)
{
	MessageWithTaskIndex* message1;
	MessageWithTaskIndex* message2;
	longint lIndexComp;

	message1 = cast(MessageWithTaskIndex*, *(Object**)elem1);
	message2 = cast(MessageWithTaskIndex*, *(Object**)elem2);

	// Comparaison sur le TaskIndex puis sur le numero de ligne, puis sur l'ordre de reception
	lIndexComp = (longint)message1->GetTaskIndex() - message2->GetTaskIndex();
	if (lIndexComp == 0)
	{
		lIndexComp = message1->GetMessage()->GetIndex() - message2->GetMessage()->GetIndex();
		if (lIndexComp == 0)
			lIndexComp = (longint)message1->GetAddIndex() - message2->GetAddIndex();
	}
	return (int)((lIndexComp > 0) - (lIndexComp < 0)); // 1 si > 0; -1 si >0; 0 sinon
}

PLMPIMessageManager::PLMPIMessageManager()
{
	slMessages.SetCompareFunction(MessageWithTaskIndexCompare);
	nTaskIndexToPrint = 0;
	lGlobalLineNumber = 0;
	nAddIndex = 0;
	nIsMaxErrorFlowReachedPerGravity[0] = 0;
	nIsMaxErrorFlowReachedPerGravity[1] = 0;
	nIsMaxErrorFlowReachedPerGravity[2] = 0;
	nTaskIndexError = -1;
}

PLMPIMessageManager::~PLMPIMessageManager()
{
	slMessages.DeleteAll();
}

void PLMPIMessageManager::AddMessage(int nTaskIndex, PLErrorWithIndex* error)
{
	MessageWithTaskIndex* message;
	require(error != NULL);
	message = new MessageWithTaskIndex;
	message->SetAddIndex(nAddIndex);
	message->SetTaskIndex(nTaskIndex);
	message->SetMessage(error);
	slMessages.Add(message);
	nAddIndex++;
}

void PLMPIMessageManager::AddTaskId(int nTaskIndex)
{
	lvLines.Add(-1);
	ensure(lvLines.GetSize() == nTaskIndex);
}

void PLMPIMessageManager::SetTaskLineNumber(int nTaskIndex, longint lLocalLineNumber)
{
	assert(lvLines.GetSize() > nTaskIndex);
	assert(lvLines.GetAt(nTaskIndex) == -1);

	// Ajout d'un message factice pour etre sur qu'il y ait au moins un message avec ce taskIndex
	// dans le dictionaire des messages, c'est indispensable car ca permet d'iterer nTaskIndexToPrint
	// dans la methode PrintMessages
	AddMessage(nTaskIndex, new PLErrorWithIndex);

	// Ajout dans le dictionnaire
	lvLines.SetAt(nTaskIndex, lLocalLineNumber);
}

void PLMPIMessageManager::PrintMessages()
{
	MessageWithTaskIndex* message;
	int nMessageTaskIndex;
	longint lTaskLineNumber;
	int nMessageGravity;
	boolean bHideMessage;

	// Parcours des messages ordonnes par le taskIndex
	while (slMessages.GetCount() > 0)
	{
		message = cast(MessageWithTaskIndex*, slMessages.GetHead());
		nMessageTaskIndex = message->GetTaskIndex();

		// Si la tache precedente est terminee (on a un nombre de lignes non nul pour son taskId)
		// Il faut mettre a jour le nombre de lignes total et autoriser l'affichage
		// des nouveaux messages
		if (nMessageTaskIndex == 0)
			lTaskLineNumber = -1;
		else
			lTaskLineNumber = lvLines.GetAt(nMessageTaskIndex - 1);
		if (lTaskLineNumber != -1)
		{
			nTaskIndexToPrint = nMessageTaskIndex;

			// Mise a jour du nombre global de lignes traitees
			lGlobalLineNumber += lTaskLineNumber;
		}

		// On affiche puis detruit les messages si c'est le moment de les afficher
		if (nMessageTaskIndex <= nTaskIndexToPrint)
		{
			if (message->GetMessage()->GetError() != NULL)
			{
				nMessageGravity = message->GetMessage()->GetError()->GetGravity();

				// Dans le cas des erreurs on n'envoie que les messages du premier esclave
				// les messages des autres esclaves sont ignores
				bHideMessage = false;
				if (nMessageGravity == Error::GravityError)
				{
					if (nTaskIndexError == -1)
						nTaskIndexError = message->GetTaskIndex();

					if (nTaskIndexError != message->GetTaskIndex())
						bHideMessage = true;
				}

				// On affiche seulement si le max n'est pas atteint
				// TODO pourquoi est-ce qu'on gere le max ici, c'est deja gere par Ermgt
				// c'est probablement pour affichier les ... Du coup, il faut modifier
				// la methode boolean Global::IsMaxErrorFlowReachedPerGravity(int nErrorGravity)
				// en remplacant >= par > et enlver le if ci-dessous
				if ((nIsMaxErrorFlowReachedPerGravity[nMessageGravity] <= 1 or
				     Global::IgnoreErrorFlow(message->GetMessage()->GetError())) and
				    not bHideMessage)
				{
					message->Print(lGlobalLineNumber);

					// On ajoute 1 si le max est atteint, on arrete d'envoyer lorsque on a 2,
					// ca permet d'afficher 1 message de plus que le max et ainsi d'afficher "..."
					// vers l'utilisateur
					nIsMaxErrorFlowReachedPerGravity[nMessageGravity] +=
					    Global::IsMaxErrorFlowReachedPerGravity(nMessageGravity);
				}
			}
			// Nettoyage
			slMessages.RemoveHead();
			delete message;
		}
		else
		{
			// Les premiers messages sont d'une taskIndex plus grande que la tache a afficher
			// (nTaskIndexToPrint) Il faut attendre de recevoir les message qui correspondent au
			// nTaskIndexToPrint
			assert(message->GetTaskIndex() > nTaskIndexToPrint);
			break;
		}
	}
}

boolean PLMPIMessageManager::IsEmpty() const
{
	return slMessages.IsEmpty();
}
///////////////////////////////////////////////////////////////////////
// Implementation de la classe MessageWithTaskIndex

MessageWithTaskIndex::MessageWithTaskIndex()
{
	nTaskIndex = 0;
	nAddIndex = 0;
	errorWithIndex = NULL;
}

MessageWithTaskIndex::~MessageWithTaskIndex()
{
	if (errorWithIndex != NULL)
		delete errorWithIndex;
}

void MessageWithTaskIndex::SetMessage(PLErrorWithIndex* error)
{
	errorWithIndex = error;
}

void MessageWithTaskIndex::Print(longint lStartingLineNumber) const
{
	Error* error;
	longint lLocalIndex;
	ALString sTmp;

	error = errorWithIndex->GetError();
	lLocalIndex = errorWithIndex->GetIndex();

	assert(error != NULL);

	// Si c'est un erreur indexee, on decore le label
	if (lLocalIndex != -1)
		error->SetLabel(sTmp + "Record " + LongintToReadableString(lStartingLineNumber + lLocalIndex) + " : " +
				error->GetLabel());

	// Affichage de l'erreur
	Global::AddErrorObject(error);
}

void MessageWithTaskIndex::Write(ostream& ost) const
{
	Error* error;

	error = errorWithIndex->GetError();
	if (error != NULL)
	{
		ost << error->GetLabel();
	}
	else
	{
		ost << "NULL";
	}
}
