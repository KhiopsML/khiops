// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTrainedPredictor.h"

/////////////////////////////////////////////////////////////////////////////
// Classe KWTrainedPredictor

KWTrainedPredictor::KWTrainedPredictor()
{
	predictorClass = NULL;
}

KWTrainedPredictor::~KWTrainedPredictor()
{
	KWClassDomain* predictorDomain;

	oaPredictionAttributeSpecs.DeleteAll();
	if (predictorClass != NULL)
	{
		// La classe de prediction doit etre dans son propre domaine, non enregistre dans l'ensemble des
		// domaines
		predictorDomain = predictorClass->GetDomain();
		assert(predictorDomain != NULL);
		assert(KWClassDomain::GetCurrentDomain() != predictorDomain);
		assert(KWClassDomain::LookupDomain(predictorDomain->GetName()) != predictorDomain);

		// Destruction de toutes les classes du domaine, puis du domaine lui meme
		predictorDomain->DeleteAllClasses();
		delete predictorDomain;
	}
}

void KWTrainedPredictor::SetName(const ALString& sValue)
{
	sName = sValue;
}

const ALString& KWTrainedPredictor::GetName() const
{
	return sName;
}

void KWTrainedPredictor::SetPredictorClass(KWClass* aClass, int nPredictorType, const ALString sPredictorLabel)
{
	int nIndex;
	KWPredictionAttributeSpec* attributeSpec;
	KWClassDomain* predictorDomain;
	KWClass* kwcClass;
	int nClass;

	require(aClass == NULL or aClass->GetDomain() != NULL);
	require(KWType::IsPredictorType(nPredictorType));
	require(sPredictorLabel != "");

	// Parametrage de la classe de prediction
	if (predictorClass != NULL)
	{
		// La classe de prediction doit etre dans son propre domaine, non enregistre dans l'ensemble des
		// domaines
		predictorDomain = predictorClass->GetDomain();
		assert(predictorDomain != NULL);
		assert(KWClassDomain::GetCurrentDomain() != predictorDomain);
		assert(KWClassDomain::LookupDomain(predictorDomain->GetName()) != predictorDomain);

		// Destruction de toutes les classes du domaine, puis du domaine lui meme
		predictorDomain->DeleteAllClasses();
		delete predictorDomain;
	}
	predictorClass = aClass;

	// Reinitialisation du parametrage
	SetName("");
	for (nIndex = 0; nIndex < GetPredictionAttributeNumber(); nIndex++)
	{
		attributeSpec = cast(KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nIndex));
		attributeSpec->SetAttribute(NULL);
	}

	// Memorisation des meta-donnees associee a la classe du predicteur
	if (predictorClass != NULL)
	{
		// Type et libelle du predicteur
		SetMetaDataPredictorType(predictorClass, nPredictorType);
		SetMetaDataPredictorLabel(predictorClass, sPredictorLabel);

		// On memorise la classe d'origine pour chaque classe du domaine, si ce n'est pas deja fait
		// Cela permettra de reperer par un nom unique (sa version initiale) la structures des classes
		// dans le cas multi-tables, ce pour plusieurs predicteurs (de noms differents)
		// On ne le fait que la premiere fois en supposant que pour cette premiere fois, les noms
		// initiaux sont utilises
		predictorDomain = predictorClass->GetDomain();
		for (nClass = 0; nClass < predictorDomain->GetClassNumber(); nClass++)
		{
			kwcClass = predictorDomain->GetClassAt(nClass);
			if (GetMetaDataInitialClassName(kwcClass) == "")
				SetMetaDataInitialClassName(kwcClass, kwcClass->GetName());
		}

		// Pour chaque attribut de prediction, supression de la meta-donne associee pour tous les attributs de
		// la classe
		for (nIndex = 0; nIndex < GetPredictionAttributeNumber(); nIndex++)
		{
			attributeSpec = cast(KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nIndex));
			predictorClass->RemoveAllAttributesMetaDataKey(attributeSpec->GetLabel());
		}
	}
}

KWClass* KWTrainedPredictor::GetPredictorClass() const
{
	return predictorClass;
}

KWClassDomain* KWTrainedPredictor::GetPredictorDomain() const
{
	if (predictorClass == NULL)
		return NULL;
	else
		return predictorClass->GetDomain();
}

void KWTrainedPredictor::RemovePredictor()
{
	oaPredictionAttributeSpecs.DeleteAll();
	sName = "";
	predictorClass = NULL;
}

void KWTrainedPredictor::DeletePredictor()
{
	KWClassDomain* predictorDomain;

	if (predictorClass != NULL)
	{
		// La classe de prediction doit etre dans son propre domaine, non enregistre dans l'ensemble des
		// domaines
		predictorDomain = predictorClass->GetDomain();
		assert(predictorDomain != NULL);
		assert(KWClassDomain::GetCurrentDomain() != predictorDomain);
		assert(KWClassDomain::LookupDomain(predictorDomain->GetName()) != predictorDomain);

		// Destruction de toutes les classes du domaine, puis du domaine lui meme
		predictorDomain->DeleteAllClasses();
		delete predictorDomain;
	}
	RemovePredictor();
}

KWAttribute* KWTrainedPredictor::GetTargetAttribute() const
{
	return NULL;
}

KWAttribute* KWTrainedPredictor::CreatePredictionAttribute(const ALString& sAttributeName, KWDerivationRule* rule)
{
	KWAttribute* attribute;

	require(predictorClass != NULL);
	require(sAttributeName != "");
	require(rule != NULL);

	// Creation de l'attribut et de son nom
	attribute = new KWAttribute;
	attribute->SetName(predictorClass->BuildAttributeName(sAttributeName));

	// Initialisation
	attribute->SetDerivationRule(rule);
	attribute->CompleteTypeInfo(predictorClass);

	// Insertion dans la classe de prediction
	predictorClass->InsertAttribute(attribute);

	return attribute;
}

void KWTrainedPredictor::PrepareDeploymentClass(boolean bMandatoryAttributes, boolean bEvaluationAttributes)
{
	const KWPredictionAttributeSpec* attributeSpec;
	KWAttribute* attribute;
	int nIndex;

	require(Check());

	//////////////////////////////////////////////////////////////////////////
	// Specification des attributs a utiliser

	// Tous les attributs en not Used
	predictorClass->SetAllAttributesUsed(false);

	// On garde les attributs de prediction obligatoires ou utiles pour l'evaluation
	for (nIndex = 0; nIndex < GetPredictionAttributeNumber(); nIndex++)
	{
		attributeSpec = GetPredictionAttributeSpecAt(nIndex);
		attribute = attributeSpec->GetAttribute();

		// Parametrage du chargement de l'attribut
		if ((bMandatoryAttributes and attributeSpec->GetMandatory()) or
		    (bEvaluationAttributes and attributeSpec->GetEvaluation()))
		{
			// Parametrage du chargement de l'attribut
			assert(attribute != NULL or not attributeSpec->GetMandatory());
			if (attribute != NULL and KWType::IsSimple(attribute->GetType()))
			{
				assert(predictorClass->LookupAttribute(attribute->GetName()) == attribute);

				// On met l'attribut en Used, sauf pour l'attribut cible si l'evaluation n'est pas
				// demandee
				if (bEvaluationAttributes or attribute != GetTargetAttribute())
				{
					attribute->SetUsed(true);
					attribute->SetLoaded(true);
				}
			}
		}
	}
}

void KWTrainedPredictor::CleanPredictorClass(const KWClassDomain* initialDomain)
{
	const KWPredictionAttributeSpec* attributeSpec;
	KWAttribute* attribute;
	int i;

	require(Check());
	require(initialDomain != NULL);
	require(initialDomain->LookupClass(GetMetaDataInitialClassName(predictorClass)) != NULL);

	// On garde tous les attributs dedies au predicteurs
	predictorClass->SetAllAttributesUsed(false);
	for (i = 0; i < GetPredictionAttributeNumber(); i++)
	{
		attributeSpec = GetPredictionAttributeSpecAt(i);
		attribute = attributeSpec->GetAttribute();
		if (attribute != NULL)
		{
			assert(predictorClass->LookupAttribute(attribute->GetName()) == attribute);
			attribute->SetUsed(true);
			attribute->SetLoaded(true);
		}
	}

	// Simplification de la classe
	// (Ce qui necessite de la compiler provisoirement)
	predictorClass->GetDomain()->Compile();
	predictorClass->DeleteUnusedDerivedAttributes(initialDomain);
}

boolean KWTrainedPredictor::ImportPredictorClass(KWClass* aClass)
{
	boolean bOk;
	KWClassDomain* predictorDomain;
	int nIndex;
	KWPredictionAttributeSpec* attributeSpec;
	ObjectDictionary odPredictionAttributeLabels;
	KWAttribute* attribute;
	boolean bSilentMode;
	int nKey;
	ALString sMetaDataKey;

	require(predictorClass == NULL);
	require(aClass != NULL);

	// Test de l'integrite de la classe
	require(aClass->Check());
	require(aClass->GetConstMetaData()->IsKeyPresent("PredictorType"));

	// Creation du predicteur potentiel et de son domaine
	predictorDomain = aClass->GetDomain()->CloneFromClass(aClass);
	predictorClass = predictorDomain->LookupClass(aClass->GetName());

	// Memorisation du nom du predicteur
	SetName(GetMetaDataPredictorLabel(predictorClass));

	// Enregistrement dans un dictionnaire des libelles des specifications d'attributs de prediction
	for (nIndex = 0; nIndex < GetPredictionAttributeNumber(); nIndex++)
	{
		attributeSpec = cast(KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nIndex));
		assert(attributeSpec->GetAttribute() == NULL);

		// Enregistrement de l'attribut par son libelle
		assert(odPredictionAttributeLabels.Lookup(attributeSpec->GetLabel()) == NULL);
		odPredictionAttributeLabels.SetAt(attributeSpec->GetLabel(), attributeSpec);
	}

	// Parcours de la classe pour identifier les attributs de prediction par leur meta-donnees
	bOk = true;
	attribute = predictorClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Parcours des meta-donnees de l'attribut
		for (nKey = 0; nKey < attribute->GetConstMetaData()->GetKeyNumber(); nKey++)
		{
			sMetaDataKey = attribute->GetConstMetaData()->GetKeyAt(nKey);

			// Recherche si on a un attribut de prediction associe
			attributeSpec =
			    cast(KWPredictionAttributeSpec*, odPredictionAttributeLabels.Lookup(sMetaDataKey));

			// Memorisation de l'attribut de la classe si OK
			if (attributeSpec != NULL)
			{
				// Il ne pas y avoir deja d'attribut de prediction enregistre
				bOk = attributeSpec->GetAttribute() == NULL;

				// Memorisation si OK, arret sinon
				if (bOk)
					attributeSpec->SetAttribute(attribute);
				else
					break;
			}
		}

		// Attribut suivant
		predictorClass->GetNextAttribute(attribute);
	}

	// Test de la validite en mode silencieux, tester si l'import conduit a un predicteur valide
	// sans emettre de message utilisateur
	if (bOk)
	{
		bSilentMode = Global::GetSilentMode();
		Global::SetSilentMode(true);
		bOk = Check();
		Global::SetSilentMode(bSilentMode);
	}

	// Si echec, destruction de la classe
	if (not bOk)
		DeletePredictor();

	// Code retour
	assert(bOk or predictorClass == NULL);
	return bOk;
}

int KWTrainedPredictor::GetPredictionAttributeNumber() const
{
	return oaPredictionAttributeSpecs.GetSize();
}

boolean KWTrainedPredictor::IsConsistentWith(const KWTrainedPredictor* otherPredictor) const
{
	boolean bOk = true;

	require(otherPredictor != NULL);

	// Test si meme type
	if (GetTargetType() != otherPredictor->GetTargetType())
	{
		bOk = false;
		AddError("Inconsistent with " + otherPredictor->GetClassLabel() + " " +
			 otherPredictor->GetObjectLabel() + " (different type of predictor)");
	}

	// Test si meme attribut cible
	if (bOk and KWType::IsSimple(GetTargetType()) and
	    GetTargetAttribute()->GetName() != otherPredictor->GetTargetAttribute()->GetName())
	{
		bOk = false;
		AddError("Inconsistent with " + otherPredictor->GetClassLabel() + " " +
			 otherPredictor->GetObjectLabel() + " (different target attribute)");
	}
	return bOk;
}

const KWPredictionAttributeSpec* KWTrainedPredictor::GetPredictionAttributeSpecAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < oaPredictionAttributeSpecs.GetSize());
	return cast(const KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nIndex));
}

void KWTrainedPredictor::AddPredictionAttributeSpec(KWPredictionAttributeSpec* predictionAttributeSpec)
{
	require(predictionAttributeSpec != NULL);
	require(predictionAttributeSpec->Check());
	oaPredictionAttributeSpecs.Add(predictionAttributeSpec);
}

boolean KWTrainedPredictor::Check() const
{
	boolean bOk = true;
	KWClassDomain* predictorDomain;
	KWClass* kwcClass;
	int nClass;
	ALString sInitialClassName;
	ObjectDictionary odInitialClassNames;
	KWAttribute* attribute;
	int nKey;
	ALString sKey;
	KWPredictionAttributeSpec* attributeSpec;
	int nIndex;
	ObjectDictionary odCheckPredictionAttributes;

	// Verification de la classe
	if (bOk and predictorClass == NULL)
	{
		AddError("Missing predictor dictionary");
		bOk = false;
	}

	// Verification des meta-donnees de la classe
	if (bOk and not KWType::IsPredictorType(GetMetaDataPredictorType(predictorClass)))
	{
		AddError("Missing predictor type in meta-data of dictionary " + predictorClass->GetName());
		bOk = false;
	}
	if (bOk and GetMetaDataPredictorLabel(predictorClass) == "")
	{
		AddError("Missing predictor label in meta-data of dictionary " + predictorClass->GetName());
		bOk = false;
	}

	// Verification du domaine de classe du predicteur
	if (bOk and predictorClass->GetDomain() == NULL)
	{
		AddError("Missing predictor dictionary domain");
		bOk = false;
	}

	// Verification de l'integrite de la classe, quand elle n'appartient pas a un domaine
	// La classe produite par la methode KWPredictor::InternalTrain doit etre detachee d'un domaine,
	// mais elle peut par la suite y etre attachee pour cause d'evaluation par exemple
	if (bOk)
	{
		predictorDomain = predictorClass->GetDomain();

		// Test de l'integrite de la classe
		if (not predictorDomain->Check())
		{
			AddError("Predictor dictionary (" + predictorClass->GetName() + ") is not valid");
			bOk = false;
		}

		// Test de la specification de la classe initiale pour toutes les classes du domaine
		if (bOk)
		{
			for (nClass = 0; nClass < predictorDomain->GetClassNumber(); nClass++)
			{
				kwcClass = predictorDomain->GetClassAt(nClass);

				// Test d'existence d'un nom de classe initial
				sInitialClassName = GetMetaDataInitialClassName(kwcClass);
				if (sInitialClassName == "")
				{
					AddError("Missing initial dictionary name for predictor dictionary (" +
						 kwcClass->GetName() + ")");
					bOk = false;
					break;
				}

				// Test de validite des noms de classe initiaux
				if (not kwcClass->CheckName(sInitialClassName, KWClass::Class, kwcClass))
				{
					AddError("Bad initial dictionary name (" + sInitialClassName +
						 ") for predictor dictionary (" + kwcClass->GetName() + ")");
					bOk = false;
					break;
				}

				// Test d'unicite des noms de classe initiaux
				if (odInitialClassNames.Lookup(sInitialClassName) == NULL)
					odInitialClassNames.SetAt(sInitialClassName, kwcClass);
				else
				{
					AddError("Initial dictionary name (" + sInitialClassName +
						 ") for predictor dictionary (" + kwcClass->GetName() +
						 ") already used");
					bOk = false;
					break;
				}
			}
		}
	}

	// Verification des attributs de prediction et de leur meta-donnees
	if (bOk)
		bOk = CheckPredictionAttributes();

	// Verification que les meta-donnees ne sont pas utilisee a tort dans d'autre attributs de la classe du
	// predicteur
	if (bOk)
	{
		// Memorisation prealable des attributs de prediction dans un dictionnaire
		for (nIndex = 0; nIndex < oaPredictionAttributeSpecs.GetSize(); nIndex++)
		{
			attributeSpec = cast(KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nIndex));
			odCheckPredictionAttributes.SetAt(attributeSpec->GetLabel(), attributeSpec);
		}

		// Parcours des attribut de la classe du predicteur
		attribute = predictorClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Parcours des meta-donnees de l'attribut
			for (nKey = 0; nKey < attribute->GetMetaData()->GetKeyNumber(); nKey++)
			{
				sKey = attribute->GetMetaData()->GetKeyAt(nKey);

				// Test si la cle de meta-donne correspond a une spec d'attribut de prediction
				attributeSpec =
				    cast(KWPredictionAttributeSpec*, odCheckPredictionAttributes.Lookup(sKey));

				// Erreur si le tag est utilise ailleurs que sur l'attribut de predcition
				if (attributeSpec != NULL and attributeSpec->GetAttribute() != attribute)
				{
					attributeSpec->AddError("Variable " + attribute->GetName() +
								" should not have meta-data specific to predictors");
					bOk = false;
					break;
				}
			}

			// Arret si erreur
			if (not bOk)
				break;

			// Attribut suivant
			predictorClass->GetNextAttribute(attribute);
		}
	}

	return bOk;
}

void KWTrainedPredictor::Write(ostream& ost) const
{
	const KWPredictionAttributeSpec* attributeSpec;
	int nIndex;

	ost << GetClassLabel() << "\t" << GetObjectLabel() << "\n";

	// Variable de prediction
	ost << "Predictor variables\n";
	for (nIndex = 0; nIndex < GetPredictionAttributeNumber(); nIndex++)
	{
		attributeSpec = GetPredictionAttributeSpecAt(nIndex);
		ost << attributeSpec->GetLabel();
		if (attributeSpec->GetAttribute() != NULL)
			ost << "\t" << attributeSpec->GetAttribute()->GetName();
		ost << "\n";
	}

	// Dictionnaire de prediction
	ost << "Predictor dictionary\n";
	if (GetPredictorClass() != NULL)
		ost << *GetPredictorClass();
	ost << "\n";
}

const ALString KWTrainedPredictor::GetClassLabel() const
{
	return KWType::GetPredictorLabel(GetTargetType());
}

const ALString KWTrainedPredictor::GetObjectLabel() const
{
	ALString sObjectLabel;

	sObjectLabel = GetName();
	if (predictorClass != NULL)
		sObjectLabel += " (" + predictorClass->GetName() + ")";

	return sObjectLabel;
}

void KWTrainedPredictor::SetMetaDataInitialClassName(KWClass* kwcClass, const ALString& sInitialClassName)
{
	require(kwcClass != NULL);
	require(sInitialClassName != "");

	kwcClass->GetMetaData()->SetStringValueAt("InitialDictionary", sInitialClassName);
}

const ALString KWTrainedPredictor::GetMetaDataInitialClassName(const KWClass* kwcClass)
{
	ALString sInitialClassName;

	require(kwcClass != NULL);

	sInitialClassName = kwcClass->GetConstMetaData()->GetStringValueAt("InitialDictionary");
	return sInitialClassName;
}

void KWTrainedPredictor::SetMetaDataPredictorType(KWClass* kwcClass, int nType)
{
	require(kwcClass != NULL);
	require(KWType::IsPredictorType(nType));

	kwcClass->GetMetaData()->SetStringValueAt("PredictorType", KWType::GetPredictorLabel(nType));
}

int KWTrainedPredictor::GetMetaDataPredictorType(const KWClass* kwcClass)
{
	ALString sPredictorType;

	require(kwcClass != NULL);

	// Recherche du nom de la classe d'origine a partir de son libelle
	sPredictorType = kwcClass->GetConstMetaData()->GetStringValueAt("PredictorType");
	return KWType::ToPredictorType(sPredictorType);
}

void KWTrainedPredictor::SetMetaDataPredictorLabel(KWClass* kwcClass, const ALString& sPredictorLabel)
{
	require(kwcClass != NULL);
	require(sPredictorLabel != "");

	kwcClass->GetMetaData()->SetStringValueAt("PredictorLabel", sPredictorLabel);
}

const ALString KWTrainedPredictor::GetMetaDataPredictorLabel(const KWClass* kwcClass)
{
	ALString sPredictorLabel;

	require(kwcClass != NULL);

	// Recherche du nom de la classe d'origine a partir de son libelle
	sPredictorLabel = kwcClass->GetConstMetaData()->GetStringValueAt("PredictorLabel");
	return sPredictorLabel;
}

void KWTrainedPredictor::SetPredictionAttributeNumber(int nValue)
{
	int nAttribute;

	require(nValue >= 0);
	require(oaPredictionAttributeSpecs.GetSize() == 0);

	oaPredictionAttributeSpecs.SetSize(nValue);
	for (nAttribute = 0; nAttribute < oaPredictionAttributeSpecs.GetSize(); nAttribute++)
		oaPredictionAttributeSpecs.SetAt(nAttribute, new KWPredictionAttributeSpec);
}

void KWTrainedPredictor::SetPredictionAttributeSpecAt(int nIndex, const ALString& sLabel, int nType, boolean bMandatory,
						      boolean bEvaluation)
{
	KWPredictionAttributeSpec* attributeSpec;

	require(0 <= nIndex and nIndex < oaPredictionAttributeSpecs.GetSize());
	require(sLabel != "");
	require(nType != KWType::Unknown);

	// Recherche de la specification d'attribut a mettre a jour
	attributeSpec = cast(KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nIndex));
	check(attributeSpec);
	require(attributeSpec->GetLabel() == "");
	require(attributeSpec->GetAttribute() == NULL);

	// Mise a jour de la specification
	attributeSpec->SetLabel(sLabel);
	attributeSpec->SetType(nType);
	attributeSpec->SetMandatory(bMandatory);
	attributeSpec->SetEvaluation(bEvaluation);
}

void KWTrainedPredictor::SetAttributeAt(int nIndex, KWAttribute* attribute)
{
	KWPredictionAttributeSpec* attributeSpec;

	require(0 <= nIndex and nIndex < oaPredictionAttributeSpecs.GetSize());

	// Recherche de la specification d'attribut a mettre a jour
	attributeSpec = cast(KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nIndex));
	check(attributeSpec);
	require(attributeSpec->GetLabel() != "");

	// Mise a jour de l'attribut
	attributeSpec->SetAttribute(attribute);
}

KWAttribute* KWTrainedPredictor::GetAttributeAt(int nIndex) const
{
	KWPredictionAttributeSpec* attributeSpec;

	require(0 <= nIndex and nIndex < oaPredictionAttributeSpecs.GetSize());

	// Recherche de la specification d'attribut a mettre a jour
	attributeSpec = cast(KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nIndex));
	check(attributeSpec);
	require(attributeSpec->GetLabel() != "");

	// On retourne l'attribut
	return attributeSpec->GetAttribute();
}

boolean KWTrainedPredictor::CheckPredictionAttributes() const
{
	boolean bOk = true;
	const KWPredictionAttributeSpec* attributeSpec;
	int nIndex;
	ObjectDictionary odCheckPredictionAttributes;

	require(predictorClass != NULL);

	// Il doit y avoir au moins un attribut de prediction dans le cas supervise
	if (oaPredictionAttributeSpecs.GetSize() == 0 and GetTargetType() != KWType::None)
	{
		AddError("Missing prediction variable");
		bOk = false;
	}

	// Verification de chaque attribut
	for (nIndex = 0; nIndex < GetPredictionAttributeNumber(); nIndex++)
	{
		attributeSpec = GetPredictionAttributeSpecAt(nIndex);

		// Verification de la specification d'attribut
		if (not attributeSpec->Check())
			bOk = false;

		// Verification de l'unicite des libelles d'attributs
		// (le cast en Object* est necessaire car la methode SetAt n'attend pas un object const)
		if (odCheckPredictionAttributes.Lookup(attributeSpec->GetLabel()) == NULL)
			odCheckPredictionAttributes.SetAt(attributeSpec->GetLabel(), cast(Object*, attributeSpec));
		else
		{
			attributeSpec->AddError("Several prediction variables have the same name");
			bOk = false;
		}

		// Verification de la presence de l'attribut dans la classe
		if (bOk and attributeSpec->GetAttribute() != NULL and
		    predictorClass->LookupAttribute(attributeSpec->GetAttribute()->GetName()) !=
			attributeSpec->GetAttribute())
		{
			attributeSpec->AddError("Prediction variable not found in prediction dictionary");
			bOk = false;
		}

		// Arret si erreurs
		if (not bOk)
			break;
	}
	return bOk;
}

/////////////////////////////////////////////////////////////////////////////
// Classe KWTrainedClassifier

KWTrainedClassifier::KWTrainedClassifier()
{
	SetPredictionAttributeNumber(PredictionAttributeNumber);
	SetPredictionAttributeSpecAt(TargetAttribute, "TargetVariable", KWType::Symbol, true, true);
	SetPredictionAttributeSpecAt(TargetValues, "TargetValues", KWType::Structure, false, false);
	SetPredictionAttributeSpecAt(Prediction, "Prediction", KWType::Symbol, true, true);
	SetPredictionAttributeSpecAt(Score, "Score", KWType::Continuous, false, true);
	nFirstProbAttributeIndex = -1;
}

KWTrainedClassifier::~KWTrainedClassifier() {}

void KWTrainedClassifier::SetPredictorClass(KWClass* aClass, int nPredictorType, const ALString sPredictorLabel)
{
	const ALString sTargetProbMetaDataKey = GetTargetProbMetaDataKey();
	KWAttribute* attribute;
	int nKey;
	ALString sKey;
	ALString sIndex;
	int nIndex;

	// Methode ancetre
	KWTrainedPredictor::SetPredictorClass(aClass, nPredictorType, sPredictorLabel);

	// Nettoyage des meta-donnees correspondantes associee a tous les attributs de la classe
	attribute = predictorClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Parcours des cles de l'attribut
		for (nKey = 0; nKey < attribute->GetMetaData()->GetKeyNumber(); nKey++)
		{
			sKey = attribute->GetMetaData()->GetKeyAt(nKey);

			// On supprime la cle de meta-donnee si elle est de meme nature
			if (sKey.GetLength() > sTargetProbMetaDataKey.GetLength() and
			    sKey.Left(sTargetProbMetaDataKey.GetLength()) == sTargetProbMetaDataKey)
			{
				sIndex = sKey.Right(sKey.GetLength() - sTargetProbMetaDataKey.GetLength());
				nIndex = StringToInt(sIndex);
				if (IntToString(nIndex) == sIndex)
					attribute->GetMetaData()->RemoveKey(sKey);
			}
		}

		// Attribut suivant
		predictorClass->GetNextAttribute(attribute);
	}
}

void KWTrainedClassifier::RemovePredictor()
{
	KWTrainedPredictor::RemovePredictor();
	nFirstProbAttributeIndex = -1;
	svTargetValues.SetSize(0);
}

int KWTrainedClassifier::GetTargetType() const
{
	return KWType::Symbol;
}

void KWTrainedClassifier::SetTargetAttribute(KWAttribute* attribute)
{
	SetAttributeAt(TargetAttribute, attribute);
}

KWAttribute* KWTrainedClassifier::GetTargetAttribute() const
{
	return GetAttributeAt(TargetAttribute);
}

void KWTrainedClassifier::SetTargetValuesAttribute(KWAttribute* attribute)
{
	SetAttributeAt(TargetValues, attribute);
}

KWAttribute* KWTrainedClassifier::GetTargetValuesAttribute() const
{
	return GetAttributeAt(TargetValues);
}

void KWTrainedClassifier::SetPredictionAttribute(KWAttribute* attribute)
{
	SetAttributeAt(Prediction, attribute);
}

KWAttribute* KWTrainedClassifier::GetPredictionAttribute() const
{
	return GetAttributeAt(Prediction);
}

void KWTrainedClassifier::SetScoreAttribute(KWAttribute* attribute)
{
	SetAttributeAt(Score, attribute);
}

KWAttribute* KWTrainedClassifier::GetScoreAttribute() const
{
	return GetAttributeAt(Score);
}

void KWTrainedClassifier::SetTargetValueNumber(int nNumber)
{
	const ALString sTargetProbMetaDataKey = GetTargetProbMetaDataKey();
	int nTarget;
	KWPredictionAttributeSpec* attributeSpec;

	require(predictorClass != NULL);
	require(nNumber >= 0);
	require(svTargetValues.GetSize() == 0);
	require(nFirstProbAttributeIndex == -1);

	// Memorisation de l'index du premier attribut de probab conditionnelle
	nFirstProbAttributeIndex = oaPredictionAttributeSpecs.GetSize();

	// Initialisation du contenu des tableaux
	svTargetValues.SetSize(nNumber);
	for (nTarget = 0; nTarget < nNumber; nTarget++)
	{
		// Creation d'une specification d'attribut
		attributeSpec = new KWPredictionAttributeSpec;
		oaPredictionAttributeSpecs.Add(attributeSpec);

		// Initialisation de cette specification
		attributeSpec->SetLabel(sTargetProbMetaDataKey + IntToString(nTarget + 1));
		attributeSpec->SetType(KWType::Continuous);
		attributeSpec->SetMandatory(true);
		attributeSpec->SetEvaluation(true);
	}
}

int KWTrainedClassifier::GetTargetValueNumber() const
{
	return svTargetValues.GetSize();
}

void KWTrainedClassifier::SetProbAttributeAt(int nIndex, const Symbol& sTargetValue, KWAttribute* attribute)
{
	const ALString sTargetProbMetaDataKey = GetTargetProbMetaDataKey();
	KWPredictionAttributeSpec* attributeSpec;

	require(nFirstProbAttributeIndex >= 0);
	require(0 <= nIndex and nIndex < GetTargetValueNumber());

	// Mise a jour de la valeur
	svTargetValues.SetAt(nIndex, sTargetValue);

	// Recherche de la specification d'attribut a mettre a jour
	attributeSpec =
	    cast(KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nFirstProbAttributeIndex + nIndex));
	check(attributeSpec);
	assert(attributeSpec->GetLabel() != "");

	// Mise a jour du libelle
	attributeSpec->SetLabel(sTargetProbMetaDataKey + IntToString(nIndex + 1));

	// Mise a jour de l'attribut
	attributeSpec->SetAttribute(attribute);

	// Mise a jour de la meta-donnee associee a l'attribut
	attribute->GetMetaData()->SetStringValueAt(attributeSpec->GetLabel(), sTargetValue.GetValue());
}

Symbol& KWTrainedClassifier::GetTargetValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetTargetValueNumber());
	return svTargetValues.GetAt(nIndex);
}

KWAttribute* KWTrainedClassifier::GetProbAttributeAt(int nIndex) const
{
	require(nFirstProbAttributeIndex >= 0);
	require(0 <= nIndex and nIndex < GetTargetValueNumber());
	return cast(KWPredictionAttributeSpec*, oaPredictionAttributeSpecs.GetAt(nFirstProbAttributeIndex + nIndex))
	    ->GetAttribute();
}

boolean KWTrainedClassifier::ImportPredictorClass(KWClass* aClass)
{
	const ALString sTargetProbMetaDataKey = GetTargetProbMetaDataKey();
	boolean bOk;
	KWAttribute* attribute;
	ALString sIndex;
	int nIndex;
	ALString sTargetValue;
	ObjectArray oaTargetProbAttributes;
	KWSortableObject* sortableAttribute;
	boolean bTargetProbAttributesOk;
	int nKey;
	ALString sMetaDataKey;

	// Appel de la methode ancetre
	bOk = KWTrainedPredictor::ImportPredictorClass(aClass);
	assert(GetTargetValueNumber() == 0);

	// Recherche des attributs de prediction des probabilites conditionnelles
	if (bOk)
	{
		assert(predictorClass->GetName() == aClass->GetName());
		assert(predictorClass->GetDomain() != aClass->GetDomain());

		// Parcours de la classe pour identifier les attributs de prediction par leur libelle
		bTargetProbAttributesOk = true;
		attribute = predictorClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Parcours des meta-donnees de l'attribut
			sortableAttribute = NULL;
			for (nKey = 0; nKey < attribute->GetConstMetaData()->GetKeyNumber(); nKey++)
			{
				sMetaDataKey = attribute->GetConstMetaData()->GetKeyAt(nKey);

				// Recherche s'il s'agit d'un attribut de prediction de probabilite conditionnelle
				if (sMetaDataKey.GetLength() > sTargetProbMetaDataKey.GetLength() and
				    sMetaDataKey.Left(sTargetProbMetaDataKey.GetLength()) == sTargetProbMetaDataKey)
				{
					// Recherche de l'index de la valeur (potentiellement avant un ":")
					sIndex = sMetaDataKey.Right(sMetaDataKey.GetLength() -
								    sTargetProbMetaDataKey.GetLength());
					nIndex = StringToInt(sIndex);

					// Memorisation de l'attribut s'il s'agit bien d'un index pour une meta-donne de
					// type chaine de caractere
					if (IntToString(nIndex) == sIndex and
					    attribute->GetConstMetaData()->IsStringTypeAt(sMetaDataKey))
					{
						// Erreur si on a deja trouve
						if (sortableAttribute != NULL)
						{
							bTargetProbAttributesOk = false;
							break;
						}
						// Memorisation sinon
						else
						{
							sortableAttribute = new KWSortableObject;
							sortableAttribute->SetIndex(nIndex);
							sortableAttribute->SetSortValue(attribute);
							oaTargetProbAttributes.Add(sortableAttribute);
						}
					}
				}
			}

			// Arret si erreur
			if (not bTargetProbAttributesOk)
				break;

			// Attribut suivant
			predictorClass->GetNextAttribute(attribute);
		}

		// Si OK, tri des attributs potentiellement interessant pour verifier leur validite
		if (bTargetProbAttributesOk)
		{
			oaTargetProbAttributes.SetCompareFunction(KWSortableIndexCompare);
			oaTargetProbAttributes.Sort();
			for (nIndex = 0; nIndex < oaTargetProbAttributes.GetSize(); nIndex++)
			{
				sortableAttribute = cast(KWSortableObject*, oaTargetProbAttributes.GetAt(nIndex));

				// Verification de la coherence entre l'ordre des attributs identifies et leur
				// numerotation utilisateur
				if (sortableAttribute->GetIndex() != nIndex + 1)
				{
					bTargetProbAttributesOk = false;
					break;
				}
			}
		}

		// Si OK, memorisation des attributs de prediction de proba conditionnelle
		if (bTargetProbAttributesOk)
		{
			// Memorisation du nombre de valeurs
			SetTargetValueNumber(oaTargetProbAttributes.GetSize());

			// Enregistrement des attributs de prediction
			for (nIndex = 0; nIndex < oaTargetProbAttributes.GetSize(); nIndex++)
			{
				sortableAttribute = cast(KWSortableObject*, oaTargetProbAttributes.GetAt(nIndex));

				// Extraction de l'attribut
				attribute = cast(KWAttribute*, sortableAttribute->GetSortValue());

				// Extraction de la valeur
				sMetaDataKey = sTargetProbMetaDataKey + IntToString(nIndex + 1);
				sTargetValue = attribute->GetConstMetaData()->GetStringValueAt(sMetaDataKey);
				sTargetValue.TrimLeft();
				sTargetValue.TrimRight();

				// Enregistrement des specifications
				SetProbAttributeAt(nIndex, (Symbol)sTargetValue, attribute);
			}
		}

		// Nettoyage
		oaTargetProbAttributes.DeleteAll();
	}

	// Si echec, destruction de la classe (cree dans la methode ancetre)
	if (not bOk)
		DeletePredictor();

	ensure(not bOk or Check());
	return bOk;
}

boolean KWTrainedClassifier::IsConsistentWith(const KWTrainedPredictor* otherPredictor) const
{
	boolean bOk = true;
	KWTrainedClassifier* otherClassifier;
	int nTarget;

	require(otherPredictor != NULL);

	// Appel de la methode ancetre
	bOk = KWTrainedPredictor::IsConsistentWith(otherPredictor);

	// Test plus pousse s'il sagit d'un classifieurs a comparer
	if (bOk)
	{
		otherClassifier = cast(KWTrainedClassifier*, otherPredictor);

		// Test du nombre de valeurs cibles, qui peut etre 0, ou doit etre le meme
		if (GetTargetValueNumber() > 0 and otherClassifier->GetTargetValueNumber() > 0 and
		    GetTargetValueNumber() != otherClassifier->GetTargetValueNumber())
		{
			bOk = false;
			AddError("Inconsistent with " + otherPredictor->GetClassLabel() + " " +
				 otherPredictor->GetObjectLabel() +
				 " (number of target values are different :" + IntToString(GetTargetValueNumber()) +
				 " versus " + IntToString(otherClassifier->GetTargetValueNumber()) + ")");
		}

		// Test des valeurs cibles
		if (bOk and GetTargetValueNumber() == otherClassifier->GetTargetValueNumber())
		{
			for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
			{
				if (GetTargetValueAt(nTarget) != otherClassifier->GetTargetValueAt(nTarget))
				{
					bOk = false;
					AddError("Inconsistent with " + otherPredictor->GetClassLabel() + " " +
						 otherPredictor->GetObjectLabel() + " (target values at index " +
						 IntToString(nTarget + 1) +
						 " are different :" + GetTargetValueAt(nTarget) + " versus " +
						 otherClassifier->GetTargetValueAt(nTarget) + ")");
					break;
				}
			}
		}
	}
	return bOk;
}

const ALString KWTrainedClassifier::GetClassLabel() const
{
	return "Classifier";
}

const ALString KWTrainedClassifier::GetObjectLabel() const
{
	ALString sObjectLabel;

	sObjectLabel = GetName();
	if (predictorClass != NULL)
	{
		sObjectLabel += " (" + predictorClass->GetName();
		if (GetTargetAttribute() != NULL)
			sObjectLabel += " -> " + GetTargetAttribute()->GetName();
		sObjectLabel += ")";
	}

	return sObjectLabel;
}

const ALString& KWTrainedClassifier::GetTargetProbMetaDataKey() const
{
	static const ALString sTargetProbMetaDataKey = "TargetProb";
	return sTargetProbMetaDataKey;
}

/////////////////////////////////////////////////////////////////////////////
// Classe KWTrainedRegressor

KWTrainedRegressor::KWTrainedRegressor()
{
	SetPredictionAttributeNumber(PredictionAttributeNumber);
	SetPredictionAttributeSpecAt(TargetAttribute, "TargetVariable", KWType::Continuous, true, true);
	SetPredictionAttributeSpecAt(TargetAttributeRank, "TargetVariableRank", KWType::Continuous, false, true);
	SetPredictionAttributeSpecAt(TargetValues, "TargetValues", KWType::Structure, false, true);
	SetPredictionAttributeSpecAt(Mean, "Mean", KWType::Continuous, true, true);
	SetPredictionAttributeSpecAt(Density, "Density", KWType::Continuous, false, true);
	SetPredictionAttributeSpecAt(MeanRank, "MeanRank", KWType::Continuous, false, true);
	SetPredictionAttributeSpecAt(DensityRank, "DensityRank", KWType::Continuous, false, true);
}

KWTrainedRegressor::~KWTrainedRegressor() {}

int KWTrainedRegressor::GetTargetType() const
{
	return KWType::Continuous;
}

void KWTrainedRegressor::SetTargetAttribute(KWAttribute* attribute)
{
	SetAttributeAt(TargetAttribute, attribute);
}

KWAttribute* KWTrainedRegressor::GetTargetAttribute() const
{
	return GetAttributeAt(TargetAttribute);
}

void KWTrainedRegressor::SetTargetAttributeRank(KWAttribute* attribute)
{
	SetAttributeAt(TargetAttributeRank, attribute);
}

KWAttribute* KWTrainedRegressor::GetTargetAttributeRank() const
{
	return GetAttributeAt(TargetAttributeRank);
}

void KWTrainedRegressor::SetTargetValuesAttribute(KWAttribute* attribute)
{
	SetAttributeAt(TargetValues, attribute);
}

KWAttribute* KWTrainedRegressor::GetTargetValuesAttribute() const
{
	return GetAttributeAt(TargetValues);
}

void KWTrainedRegressor::SetMeanAttribute(KWAttribute* attribute)
{
	SetAttributeAt(Mean, attribute);
}

KWAttribute* KWTrainedRegressor::GetMeanAttribute() const
{
	return GetAttributeAt(Mean);
}

void KWTrainedRegressor::SetDensityAttribute(KWAttribute* attribute)
{
	SetAttributeAt(Density, attribute);
}

KWAttribute* KWTrainedRegressor::GetDensityAttribute() const
{
	return GetAttributeAt(Density);
}

void KWTrainedRegressor::SetMeanRankAttribute(KWAttribute* attribute)
{
	SetAttributeAt(MeanRank, attribute);
}

KWAttribute* KWTrainedRegressor::GetMeanRankAttribute() const
{
	return GetAttributeAt(MeanRank);
}

void KWTrainedRegressor::SetDensityRankAttribute(KWAttribute* attribute)
{
	SetAttributeAt(DensityRank, attribute);
}

KWAttribute* KWTrainedRegressor::GetDensityRankAttribute() const
{
	return GetAttributeAt(DensityRank);
}

const ALString KWTrainedRegressor::GetClassLabel() const
{
	return "Regressor";
}

const ALString KWTrainedRegressor::GetObjectLabel() const
{
	ALString sObjectLabel;

	sObjectLabel = GetName();
	if (predictorClass != NULL)
	{
		sObjectLabel += " (" + predictorClass->GetName();
		if (GetTargetAttribute() != NULL)
			sObjectLabel += " -> " + GetTargetAttribute()->GetName();
		sObjectLabel += ")";
	}

	return sObjectLabel;
}

/////////////////////////////////////////////////////////////////////////////
// Classe KWTrainedClusterer

KWTrainedClusterer::KWTrainedClusterer() {}

KWTrainedClusterer::~KWTrainedClusterer() {}

int KWTrainedClusterer::GetTargetType() const
{
	return KWType::None;
}

const ALString KWTrainedClusterer::GetClassLabel() const
{
	return "Clusterer";
}

/////////////////////////////////////////////////////////////////////////////
// Classe KWPredictionAttributeSpec

KWPredictionAttributeSpec::KWPredictionAttributeSpec()
{
	nType = KWType::Unknown;
	bMandatory = false;
	bEvaluation = false;
	attribute = NULL;
}

KWPredictionAttributeSpec::~KWPredictionAttributeSpec() {}

void KWPredictionAttributeSpec::SetLabel(const ALString& sValue)
{
	sLabel = sValue;
}

const ALString& KWPredictionAttributeSpec::GetLabel() const
{
	return sLabel;
}

void KWPredictionAttributeSpec::SetType(int nValue)
{
	nType = nValue;
}

int KWPredictionAttributeSpec::GetType() const
{
	return nType;
}

void KWPredictionAttributeSpec::SetMandatory(boolean bValue)
{
	bMandatory = bValue;
}

boolean KWPredictionAttributeSpec::GetMandatory() const
{
	return bMandatory;
}

void KWPredictionAttributeSpec::SetEvaluation(boolean bValue)
{
	bEvaluation = bValue;
}

boolean KWPredictionAttributeSpec::GetEvaluation() const
{
	return bEvaluation;
}

void KWPredictionAttributeSpec::SetAttribute(KWAttribute* kwaValue)
{
	// Nettoyage eventuel de l'attribut precedent
	if (attribute != NULL)
		attribute->GetMetaData()->RemoveKey(GetLabel());

	// Memorisation du nouvel attribut
	attribute = kwaValue;

	// Mise a jour des meta-donnees associee a l'attribut (un tag sans valeur)
	// sauf si la cle est deja presente (avec potentiellement une valeur)
	if (attribute != NULL and not attribute->GetMetaData()->IsKeyPresent(GetLabel()))
		attribute->GetMetaData()->SetNoValueAt(GetLabel());
}

KWAttribute* KWPredictionAttributeSpec::GetAttribute() const
{
	return attribute;
}

boolean KWPredictionAttributeSpec::Check() const
{
	boolean bOk = true;

	// Test de presence du libelle
	if (bOk and sLabel == "")
	{
		AddError("Missing variable name");
		bOk = false;
	}

	// Test de presence du type
	if (bOk and nType == KWType::Unknown)
	{
		AddError("Missing variable type");
		bOk = false;
	}

	// Test de presence dans le cas obligatoire seulement
	if (bOk and bMandatory and attribute == NULL)
	{
		AddError("Missing mandatory prediction variable");
		bOk = false;
	}

	// Test de la validite du type
	if (bOk and attribute != NULL and attribute->GetType() != GetType())
	{
		AddError("Type " + KWType::ToString(attribute->GetType()) + " of prediction variable " +
			 attribute->GetName() + " should be " + KWType::ToString(GetType()));
		bOk = false;
	}

	// Test de la meta-donnee associe a l'attribut
	if (bOk and attribute != NULL and not attribute->GetMetaData()->IsKeyPresent(GetLabel()))
	{
		AddError("Missing meta-data for prediction variable " + attribute->GetName());
		bOk = false;
	}

	return bOk;
}

void KWPredictionAttributeSpec::Write(ostream& ost) const
{
	ost << sLabel << " " << KWType::ToString(nType);
	if (attribute != NULL)
		ost << " " << attribute->GetName();
	ost << ": M" << bMandatory << " E" << bEvaluation;
}

const ALString KWPredictionAttributeSpec::GetClassLabel() const
{
	return "Prediction variable";
}

const ALString KWPredictionAttributeSpec::GetObjectLabel() const
{
	return sLabel;
}
