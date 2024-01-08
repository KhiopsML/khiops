// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorSpec.h"

KWPredictorSpec::KWPredictorSpec()
{
	predictor = NULL;
	nTargetAttributeType = KWType::Unknown;
}

KWPredictorSpec::~KWPredictorSpec()
{
	if (predictor != NULL)
		delete predictor;
}

void KWPredictorSpec::SetPredictorName(const ALString& sValue)
{
	sPredictorName = sValue;
}

const ALString& KWPredictorSpec::GetPredictorName() const
{
	return sPredictorName;
}

void KWPredictorSpec::SetTargetAttributeType(int nValue)
{
	require(nValue == KWType::Symbol or nValue == KWType::Continuous or nValue == KWType::None);
	nTargetAttributeType = nValue;
}

int KWPredictorSpec::GetTargetAttributeType() const
{
	return nTargetAttributeType;
}

void KWPredictorSpec::SetPredictorLabel(const ALString& sValue)
{
	sPredictorLabel = sValue;
}

const ALString& KWPredictorSpec::GetPredictorLabel() const
{
	return sPredictorLabel;
}

KWPredictor* KWPredictorSpec::GetPredictor() const
{
	// On nettoie tout si le predicteur existe, mais qu'il ne correspond pas
	// au nom specifie
	if (predictor != NULL and
	    (predictor->GetName() != sPredictorName or not predictor->IsTargetTypeManaged(GetTargetAttributeType())))
	{
		delete predictor;
		predictor = NULL;
	}

	// Si necessaire, on demande a l'administration des predicteurs
	// d'en fabriquer un d'apres son nom
	if (predictor == NULL)
		predictor = KWPredictor::ClonePredictor(sPredictorName, nTargetAttributeType);

	return predictor;
}

KWAttributeConstructionSpec* KWPredictorSpec::GetAttributeConstructionSpec()
{
	return &attributeConstructionSpec;
}

KWPreprocessingSpec* KWPredictorSpec::GetPreprocessingSpec()
{
	return &preprocessingSpec;
}

boolean KWPredictorSpec::Check() const
{
	boolean bOk = true;

	// On force l'acces au predicteur pour le tester
	GetPredictor();

	// Test de la validite du predicteur
	if (sPredictorName == "")
	{
		bOk = false;
		AddError("Missing predictor name");
	}
	else if (nTargetAttributeType == KWType::Unknown)
	{
		bOk = false;
		AddError("Missing target variable type");
	}
	// Existance du predicteur
	else if (predictor == NULL)
	{
		bOk = false;
		AddError("Unknown " + KWType::GetPredictorLabel(nTargetAttributeType) + " " + sPredictorName);
	}
	// Validite de son parametrage "generique"
	else if (not predictor->GetTrainParameters()->Check())
		bOk = false;
	// Si necessaire, validite de son parametrage par le probleme
	else if (predictor->GetLearningSpec() != NULL and not predictor->GetLearningSpec()->Check())
		bOk = false;

	// Test de validite des parametres de preprocessing
	if (bOk and not preprocessingSpec.CheckForTargetType(nTargetAttributeType))
		bOk = false;

	return bOk;
}

boolean KWPredictorSpec::IsPredictorUsable(const ALString& sPredictorName, const ALString& sPredictorFilter)
{
	boolean bPredictorUsable = true;
	ALString sEndPredictorFilter;
	ALString sName;
	int nPos;

	// Si le filtre n'est pas vide, on regarde si le nom du predictor y est present
	if (sPredictorFilter != "")
	{
		bPredictorUsable = false;

		// Extraction des noms de predictor du filtre
		sEndPredictorFilter = sPredictorFilter;
		nPos = sEndPredictorFilter.Find(";");
		while (nPos != -1)
		{
			// Extraction du premier nom et de la nouvelle fin de filtre
			sName = sEndPredictorFilter.Left(nPos);
			sEndPredictorFilter = sEndPredictorFilter.Right(sEndPredictorFilter.GetLength() - nPos - 1);

			// Test si le nom est celui du predictor
			if (sPredictorName == sName)
			{
				bPredictorUsable = true;
				break;
			}

			// Position suivante
			nPos = sEndPredictorFilter.Find(";");
		}

		// On recherche egalement sur le dernier troncon
		if (not bPredictorUsable)
		{
			if (sPredictorName == sEndPredictorFilter)
				bPredictorUsable = true;
		}
	}
	return bPredictorUsable;
}

const ALString KWPredictorSpec::GetClassLabel() const
{
	return KWType::GetPredictorLabel(GetTargetAttributeType());
}

const ALString KWPredictorSpec::GetObjectLabel() const
{
	ALString sPredictorObjectLabel;
	ALString sPreprocessingObjectLabel;
	ALString sLabel;

	// On se base d'abord sur l'eventuel libelle utilisateur
	if (sPredictorLabel != "")
		sLabel = sPredictorLabel;
	// Sinon, on en fabrique un
	else
	{
		// Recherche des libelles pour chaque element de specification
		if (predictor == NULL)
			sPredictorObjectLabel = sPredictorName;
		else
			sPredictorObjectLabel = predictor->GetObjectLabel();
		sPreprocessingObjectLabel = preprocessingSpec.GetObjectLabel();

		// Fabrication du libelle
		sLabel = sPredictorObjectLabel;
		if (sPreprocessingObjectLabel != "")
			sLabel += " " + sPreprocessingObjectLabel;
	}
	return sLabel;
}
