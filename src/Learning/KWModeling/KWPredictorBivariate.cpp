// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorBivariate.h"

KWPredictorBivariate::KWPredictorBivariate()
{
	bBestBivariate = true;
}

KWPredictorBivariate::~KWPredictorBivariate() {}

boolean KWPredictorBivariate::IsTargetTypeManaged(int nType) const
{
	return nType == KWType::Symbol;
}

KWPredictor* KWPredictorBivariate::Create() const
{
	return new KWPredictorBivariate;
}

const ALString KWPredictorBivariate::GetName() const
{
	return "Bivariate";
}

const ALString KWPredictorBivariate::GetPrefix() const
{
	return "BB";
}

const ALString KWPredictorBivariate::GetSuffix() const
{
	return GetAttributeName1() + "_" + GetAttributeName2();
}

void KWPredictorBivariate::CopyFrom(const KWPredictor* kwpSource)
{
	const KWPredictorBivariate* kwpbSource = cast(const KWPredictorBivariate*, kwpSource);

	// Appel de la methode ancetre
	KWPredictor::CopyFrom(kwpSource);

	// Recopie des nouveau attributs de specification
	SetBestBivariate(kwpbSource->GetBestBivariate());
	SetAttributeName1(kwpbSource->GetAttributeName1());
	SetAttributeName2(kwpbSource->GetAttributeName2());
}

void KWPredictorBivariate::SetBestBivariate(boolean bValue)
{
	bBestBivariate = bValue;
}

boolean KWPredictorBivariate::GetBestBivariate() const
{
	return bBestBivariate;
}

void KWPredictorBivariate::SetAttributeName1(const ALString& sValue)
{
	sAttributeName1 = sValue;
}

const ALString& KWPredictorBivariate::GetAttributeName1() const
{
	return sAttributeName1;
}

void KWPredictorBivariate::SetAttributeName2(const ALString& sValue)
{
	sAttributeName2 = sValue;
}

const ALString& KWPredictorBivariate::GetAttributeName2() const
{
	return sAttributeName2;
}

const ALString& KWPredictorBivariate::GetSourceAttributeName1() const
{
	require(IsTrained());
	return sSourceAttributeName1;
}

const ALString& KWPredictorBivariate::GetSourceAttributeName2() const
{
	require(IsTrained());
	return sSourceAttributeName2;
}

const ALString& KWPredictorBivariate::GetBivariateAttributeName() const
{
	require(IsTrained());
	ensure(trainedPredictor->GetPredictorClass()->LookupAttribute(sBivariateAttributeName) != NULL);
	return sBivariateAttributeName;
}

const KWDataGridStats* KWPredictorBivariate::GetTrainDataGridStats() const
{
	require(Check());
	require(IsTrained());
	require(GetBivariateAttributeName() != "");

	return &trainDataGridStats;
}

const ALString KWPredictorBivariate::GetObjectLabel() const
{
	return GetName() + " " + GetAttributeName1() + " " + GetAttributeName2();
}

boolean KWPredictorBivariate::InternalTrain()
{
	const ALString sSourceVariable1MetaDataKey = "SourceVariable1";
	const ALString sSourceVariable2MetaDataKey = "SourceVariable2";
	boolean bUnivariateTolerance = true;
	KWDataPreparationClass dataPreparationClass;
	ObjectArray oaTheDataPreparationAttribute;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	double dCost;
	double dBestCost;
	int nBestAttribute;
	KWAttribute* predictorSourceAttribute;
	KWPredictionAttributeSpec* predictionAttributeSpec;

	require(Check());
	require(GetClassStats() != NULL);
	require(GetBestBivariate() or (GetClass()->LookupAttribute(GetAttributeName1()) != NULL and
				       GetClass()->LookupAttribute(GetAttributeName1())->GetUsed() and
				       GetClass()->LookupAttribute(GetAttributeName2()) != NULL and
				       GetClass()->LookupAttribute(GetAttributeName2())->GetUsed()));

	// Calcul des statistiques si necessaire
	if (not GetClassStats()->IsStatsComputed())
		GetClassStats()->ComputeStats();
	assert(not GetClassStats()->IsStatsComputed() or GetTargetDescriptiveStats() != NULL);

	// Apprentissage si au moins une classe cible
	if (GetLearningSpec()->IsTargetStatsComputed() and GetTargetDescriptiveStats()->GetValueNumber() > 0)
	{
		// Parametrage de la preparation de donnees
		dataPreparationClass.SetLearningSpec(GetLearningSpec());

		// Generation de la classe de preparation des donnees
		dataPreparationClass.ComputeDataPreparationFromClassStats(GetClassStats());

		// Recherche de l'attribut a utiliser pour un predicteur bivarie
		// Cas du meilleurs possible
		nBestAttribute = -1;
		if (GetBestBivariate())
		{
			// Recherche du meilleur attribut bivarie
			dBestCost = -DBL_MAX;
			for (nAttribute = 0;
			     nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize(); nAttribute++)
			{
				dataPreparationAttribute =
				    cast(KWDataPreparationAttribute*,
					 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

				// Test si attribut bivarie pretraite
				if (dataPreparationAttribute->GetNativeAttributeNumber() == 2)
				{
					dCost = dataPreparationAttribute->GetPreparedStats()->GetLevel();

					// Test si amelioration
					if (dCost > dBestCost)
					{
						dBestCost = dCost;
						nBestAttribute = nAttribute;
					}
				}
			}

			// Si echec, on se rabat sur le meilleur univarie (paire dont un attribut est mono-partie)
			if (nBestAttribute == -1 and bUnivariateTolerance)
			{
				for (nAttribute = 0;
				     nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize();
				     nAttribute++)
				{
					dataPreparationAttribute = cast(
					    KWDataPreparationAttribute*,
					    dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

					// Test si l'attribut univarie pretraite
					if (dataPreparationAttribute->GetNativeAttributeNumber() == 1)
					{
						dCost = dataPreparationAttribute->GetPreparedStats()->GetLevel();

						// Test si amelioration
						if (dCost > dBestCost)
						{
							dBestCost = dCost;
							nBestAttribute = nAttribute;
						}
					}
				}
			}
		}
		// Sinon, recherche de l'attribut correspondant au nom specifie
		else
		{
			// Recherche de l'attribut discretise ou groupe base sur l'attribut source
			nBestAttribute = -1;
			for (nAttribute = 0;
			     nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize(); nAttribute++)
			{
				dataPreparationAttribute =
				    cast(KWDataPreparationAttribute*,
					 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

				// Test si l'attribut correspond
				if (dataPreparationAttribute->GetNativeAttributeNumber() == 2 and
				    dataPreparationAttribute->GetNativeAttribute1()->GetName() ==
					GetAttributeName1() and
				    dataPreparationAttribute->GetNativeAttribute2()->GetName() == GetAttributeName2())
				{
					nBestAttribute = nAttribute;
					break;
				}
			}

			// Si echec, on se rabat sur le meilleur univarie (paire dont un attribut est mono-partie)
			if (nBestAttribute == -1 and bUnivariateTolerance)
			{
				for (nAttribute = 0;
				     nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize();
				     nAttribute++)
				{
					dataPreparationAttribute = cast(
					    KWDataPreparationAttribute*,
					    dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

					// Test si l'attribut correspond
					if (dataPreparationAttribute->GetNativeAttributeNumber() == 1 and
					    (dataPreparationAttribute->GetNativeAttribute()->GetName() ==
						 GetAttributeName1() or
					     dataPreparationAttribute->GetNativeAttribute()->GetName() ==
						 GetAttributeName2()))
					{
						nBestAttribute = nAttribute;
						break;
					}
				}
			}
		}

		// Si echec: nettoyage
		sSourceAttributeName1 = "";
		sSourceAttributeName2 = "";
		sBivariateAttributeName = "";
		trainDataGridStats.DeleteAll();
		if (nBestAttribute == -1)
		{
			dataPreparationClass.DeleteDataPreparation();
			return false;
		}
		// Sinon, construction du predicteur univarie
		else
		{
			// Initialisation des donnees correspondant a l'attribut choisi
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*,
				 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nBestAttribute));
			oaTheDataPreparationAttribute.Add(dataPreparationAttribute);
			assert(dataPreparationAttribute->GetNativeAttributeNumber() == 2 or
			       (bUnivariateTolerance and dataPreparationAttribute->GetNativeAttributeNumber() == 1));

			// Memorisation des caracteristiques d'apprentissage
			sSourceAttributeName1 = dataPreparationAttribute->GetNativeAttribute1()->GetName();
			if (dataPreparationAttribute->GetNativeAttributeNumber() == 2)
				sSourceAttributeName2 = dataPreparationAttribute->GetNativeAttribute2()->GetName();
			sBivariateAttributeName = dataPreparationAttribute->GetPreparedAttribute()->GetName();

			// Memorisation des statistiques sur l'attribut
			trainDataGridStats.CopyFrom(
			    dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats());

			// Construction d'un predicteur bayesien naif a partir de l'attribut selectionne
			// Appele en dernier, car l'objet dataPreparationClass est nettoye suite a cet a l'appel de
			// InternalTrainNB
			InternalTrainNB(&dataPreparationClass, &oaTheDataPreparationAttribute);

			// Nettoyage initial des meta-donnees des attributs
			GetTrainedPredictor()->GetPredictorClass()->RemoveAllAttributesMetaDataKey(
			    sSourceVariable1MetaDataKey);
			GetTrainedPredictor()->GetPredictorClass()->RemoveAllAttributesMetaDataKey(
			    sSourceVariable2MetaDataKey);

			// Ajout du premier attribut source dans les specification d'apprentissage, pour
			// permettre l'evaluation de la grille de preparation de l'attribut sur
			// une base de test
			predictorSourceAttribute =
			    GetTrainedPredictor()->GetPredictorClass()->LookupAttribute(sSourceAttributeName1);
			predictionAttributeSpec = new KWPredictionAttributeSpec;
			predictionAttributeSpec->SetLabel(sSourceVariable1MetaDataKey);
			predictionAttributeSpec->SetType(predictorSourceAttribute->GetType());
			predictionAttributeSpec->SetMandatory(false);
			predictionAttributeSpec->SetEvaluation(true);
			predictionAttributeSpec->SetAttribute(predictorSourceAttribute);
			GetTrainedPredictor()->AddPredictionAttributeSpec(predictionAttributeSpec);

			// Ajout du second attribut source dans les specification d'apprentissage
			if (dataPreparationAttribute->GetNativeAttributeNumber() == 2)
			{
				predictorSourceAttribute =
				    GetTrainedPredictor()->GetPredictorClass()->LookupAttribute(sSourceAttributeName2);
				predictionAttributeSpec = new KWPredictionAttributeSpec;
				predictionAttributeSpec->SetLabel(sSourceVariable2MetaDataKey);
				predictionAttributeSpec->SetType(predictorSourceAttribute->GetType());
				predictionAttributeSpec->SetMandatory(false);
				predictionAttributeSpec->SetEvaluation(true);
				predictionAttributeSpec->SetAttribute(predictorSourceAttribute);
				GetTrainedPredictor()->AddPredictionAttributeSpec(predictionAttributeSpec);
			}
			return true;
		}
	}
	return false;
}
