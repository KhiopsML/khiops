// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningErrorManager.h"

int KWLearningErrorManager::nTaskNumber = 0;
ObjectArray KWLearningErrorManager::oaCollectedErrors;
DisplayErrorFunction KWLearningErrorManager::currentDisplayErrorFunction = NULL;

void KWLearningErrorManager::BeginErrorCollection()
{
	require(currentDisplayErrorFunction == NULL);
	require(nTaskNumber == 0);
	require(oaCollectedErrors.GetSize() == 0);
	require(Error::GetDisplayErrorFunction() != CollectErrorFunction);

	// Parametrage de la fonction de gestion des erreurs
	currentDisplayErrorFunction = Error::GetDisplayErrorFunction();
	Error::SetDisplayErrorFunction(CollectErrorFunction);
}

void KWLearningErrorManager::EndErrorCollection()
{
	require(Error::GetDisplayErrorFunction() == CollectErrorFunction);

	// Destruction des erreurs collectees
	nTaskNumber = 0;
	oaCollectedErrors.DeleteAll();

	// Restitution de la fonction de gestion des erreurs courante
	Error::SetDisplayErrorFunction(currentDisplayErrorFunction);
	currentDisplayErrorFunction = NULL;
}

void KWLearningErrorManager::AddTask(const ALString& sTitle)
{
	Error* errorTask;

	require(sTitle != "");

	// On cree un message d'un format special pour gerer ces titre de section
	errorTask = new Error;
	errorTask->SetGravity(Error::GravityMessage);
	errorTask->SetCategory("Error section");
	errorTask->SetLabel(sTitle);
	assert(IsTask(errorTask));

	// Memorisdation dans le tableau des erreurs collectees
	nTaskNumber++;
	oaCollectedErrors.Add(errorTask);
}

void KWLearningErrorManager::WriteJSONKeyReport(JSONFile* fJSON)
{
	int i;
	Error* error;
	Error* nextError;

	require(fJSON != NULL);

	// Ecriture des erreurs que s'il y en a au moins une
	// donc s'il y a plus d'erreurs enregistrees que de sections
	if (oaCollectedErrors.GetSize() > nTaskNumber)
	{
		fJSON->BeginKeyArray("logs");
		for (i = 0; i < oaCollectedErrors.GetSize(); i++)
		{
			error = cast(Error*, oaCollectedErrors.GetAt(i));

			// Acces a l'erreur suivante
			if (i + 1 < oaCollectedErrors.GetSize())
				nextError = cast(Error*, oaCollectedErrors.GetAt(i + 1));
			else
				nextError = NULL;

			// Cas d'une tache
			if (IsTask(error))
			{
				// Debut de tache si au moins une erreur pour cette tache
				if (nextError != NULL and not IsTask(nextError))
				{
					fJSON->BeginObject();
					fJSON->WriteKeyString("taskName", error->GetLabel());
					fJSON->BeginKeyArray("messages");
				}
			}
			// cas d'une erreur
			else
			{
				fJSON->WriteString(Error::BuildDisplayMessage(error));

				// Fin de tache si ce n'est pas un erreur ensuite
				if (nextError == NULL or IsTask(nextError))
				{
					fJSON->EndArray();
					fJSON->EndObject();
				}
			}
		}
		fJSON->EndArray();
	}
}

void KWLearningErrorManager::CollectErrorFunction(const Error* e)
{
	require(nTaskNumber > 0);
	require(e != NULL);

	// Collecte de l'erreur si elle est de type warning
	if (e->GetGravity() != Error::GravityMessage)
		oaCollectedErrors.Add(e->Clone());

	// Gestion standard de l'erreur
	if (currentDisplayErrorFunction != NULL)
		currentDisplayErrorFunction(e);
}

boolean KWLearningErrorManager::IsTask(Error* error)
{
	require(error != NULL);
	return error->GetGravity() == Error::GravityMessage and error->GetCategory() == "Error section" and
	       error->GetLocalisation() == "" and error->GetLabel() != "";
}
