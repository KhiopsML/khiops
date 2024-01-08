// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MYLearningProblem.h"

////////////////////////////////////////////////////////////
// Classe MYLearningProblem

MYLearningProblem::MYLearningProblem()
{
	// Specialisation des specifications d'analyse,
	// en detruisant le sous-objet cree dans la classe ancetre et en le remplacant par une version dediee
	delete analysisSpec;
	analysisSpec = new MYAnalysisSpec;

	// Specialisation des resultats d'analyse,
	// en detruisant le sous-objet cree dans la classe ancetre et en le remplacant par une version dediee
	delete analysisResults;
	analysisResults = new MYAnalysisResults;

	// Creation explicite des sous-objets supplementaire,
	// ce qui permet de creer des sous-objets specifiques dans des sous-classes
	classifierBenchmark = new KWLearningBenchmark;
	regressorBenchmark = new KWLearningBenchmark;

	// Specilisation du parametrage des benchmarks, principalement pour les classifieurs
	classifierBenchmark->SetTargetAttributeType(KWType::Symbol);
	regressorBenchmark->SetTargetAttributeType(KWType::Continuous);
}

MYLearningProblem::~MYLearningProblem()
{
	delete classifierBenchmark;
	delete regressorBenchmark;
}

KWLearningBenchmark* MYLearningProblem::GetClassifierBenchmark()
{
	// On retourne alors l'objet demande
	return classifierBenchmark;
}

KWLearningBenchmark* MYLearningProblem::GetRegressorBenchmark()
{
	// On retourne alors l'objet demande
	return regressorBenchmark;
}

void MYLearningProblem::CollectPredictors(KWClassStats* classStats, ObjectArray* oaPredictors)
{
	KWPredictorSelectiveNaiveBayes* predictorSelectiveNaiveBayes;
	KWPredictorNaiveBayes* predictorNaiveBayes;
	MYModelingSpec* khiopsModelingSpec;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(oaPredictors != NULL);
	require(classStats != NULL);
	require(classStats->IsStatsComputed());

	// Acces a la version specialisee des specification de modelisation
	khiopsModelingSpec = cast(MYModelingSpec*, analysisSpec->GetModelingSpec());

	// Predicteur Bayesien Naif Selectif
	if (khiopsModelingSpec->GetSelectiveNaiveBayesPredictor())
	{
		predictorSelectiveNaiveBayes =
		    cast(KWPredictorSelectiveNaiveBayes*,
			 KWPredictor::ClonePredictor("Selective Naive Bayes", classStats->GetTargetAttributeType()));
		if (predictorSelectiveNaiveBayes != NULL)
		{
			predictorSelectiveNaiveBayes->CopyFrom(khiopsModelingSpec->GetPredictorSelectiveNaiveBayes());
			oaPredictors->Add(predictorSelectiveNaiveBayes);
		}
		else
			AddWarning("Selective naive Bayes " +
				   KWType::GetPredictorLabel(classStats->GetTargetAttributeType()) +
				   " is not available");
	}

	// Predicteur Bayesien Naif
	if (khiopsModelingSpec->GetNaiveBayesPredictor())
	{
		predictorNaiveBayes =
		    cast(KWPredictorNaiveBayes*,
			 KWPredictor::ClonePredictor("Naive Bayes", classStats->GetTargetAttributeType()));
		if (predictorNaiveBayes != NULL)
			oaPredictors->Add(predictorNaiveBayes);
		else
			AddWarning("Naive Bayes " + KWType::GetPredictorLabel(classStats->GetTargetAttributeType()) +
				   " is not available");
	}

	// Appel de la methode ancetre pour completer la liste
	KWLearningProblem::CollectPredictors(classStats, oaPredictors);
}

void MYLearningProblem::ComputeStats()
{
	KWClassDomain* currentDomain;
	KWClass* currentClass;
	KWClassDomain* workDomain;
	KWClass* workClass;
	KWAttribute* attribute;
	KWClass* secondaryClass;
	KWAttribute* secondaryAttribute;
	KWAttribute* workAttribute;
	KWDRTableMean* tableMeanRule;
	KWDRTableMode* tableModeRule;
	KWDRGetContinuousValue* getContinuousValueRule;
	KWDRGetSymbolValue* getSymbolValueRule;
	ALString sTmp;

	// Creation si necessaire d'attribut pour le calcul de stats secondaires
	currentDomain = NULL;
	workDomain = NULL;
	if (cast(MYAnalysisResults*, GetAnalysisResults())->GetComputeBasicSecondaryStats())
	{
		// Creation d'un domaine de travail pour l'analyse des attributs secondaires
		currentDomain = KWClassDomain::GetCurrentDomain();
		currentClass = currentDomain->LookupClass(GetClassName());
		workDomain = currentDomain->CloneFromClass(currentClass);
		workClass = workDomain->LookupClass(currentClass->GetName());
		KWClassDomain::SetCurrentDomain(workDomain);

		// Tous les attributs sont inutilises, sauf l'eventuel attribut cible
		workClass->SetAllAttributesUsed(false);
		if (GetTargetAttributeName() != "")
			workClass->LookupAttribute(GetTargetAttributeName())->SetUsed(true);

		// Creation d'attributs d'analyse des attributs secondaires
		attribute = currentClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Traitement des attributs utilise de type ObjectArray
			if (attribute->GetUsed() and KWType::IsRelation(attribute->GetType()))
			{
				// Acces a la classe de l'attribut ObjectArray
				secondaryClass = attribute->GetClass();

				// Parcours de ses attributs simples utilises
				secondaryAttribute = secondaryClass->GetHeadAttribute();
				while (secondaryAttribute != NULL)
				{
					if (secondaryAttribute->GetUsed() and
					    KWType::IsSimple(secondaryAttribute->GetType()))
					{
						// Cas des attribut dans des tables en lien 0-n
						workAttribute = NULL;
						if (attribute->GetType() == KWType::ObjectArray)
						{
							// Creation d'un attribut de calcul de moyenne dans le cas
							// continu
							if (secondaryAttribute->GetType() == KWType::Continuous)
							{
								// Creation d'une regle pour la moyenne
								tableMeanRule = new KWDRTableMean;
								tableMeanRule->GetFirstOperand()->SetOrigin(
								    KWDerivationRuleOperand::OriginAttribute);
								tableMeanRule->GetFirstOperand()->SetAttributeName(
								    attribute->GetName());
								tableMeanRule->GetSecondOperand()->SetOrigin(
								    KWDerivationRuleOperand::OriginAttribute);
								tableMeanRule->GetSecondOperand()->SetAttributeName(
								    secondaryAttribute->GetName());

								// Ajout d'un attribut dans la classe de travail
								workAttribute = new KWAttribute;
								workAttribute->SetDerivationRule(tableMeanRule);
								workAttribute->SetName(workClass->BuildAttributeName(
								    attribute->GetName() + tableMeanRule->GetName() +
								    secondaryAttribute->GetName()));
								workClass->InsertAttribute(workAttribute);
							}
							// Creation d'un attribut de calcul de mode dans le cas
							// categoriel
							else if (secondaryAttribute->GetType() == KWType::Symbol)
							{
								// Creation d'une regle pour le mode
								tableModeRule = new KWDRTableMode;
								tableModeRule->GetFirstOperand()->SetOrigin(
								    KWDerivationRuleOperand::OriginAttribute);
								tableModeRule->GetFirstOperand()->SetAttributeName(
								    attribute->GetName());
								tableModeRule->GetSecondOperand()->SetOrigin(
								    KWDerivationRuleOperand::OriginAttribute);
								tableModeRule->GetSecondOperand()->SetAttributeName(
								    secondaryAttribute->GetName());

								// Ajout d'un attribut dans la classe de travail
								workAttribute = new KWAttribute;
								workAttribute->SetDerivationRule(tableModeRule);
								workAttribute->SetName(workClass->BuildAttributeName(
								    attribute->GetName() + tableModeRule->GetName() +
								    secondaryAttribute->GetName()));
								workClass->InsertAttribute(workAttribute);
							}
						}
						// Cas des attribut dans des tables en lien 0-1
						else if (attribute->GetType() == KWType::Object)
						{
							// Creation d'un attribut de calcul de moyenne dans le cas
							// continu
							if (secondaryAttribute->GetType() == KWType::Continuous)
							{
								// Creation d'une regle pour la moyenne
								getContinuousValueRule = new KWDRGetContinuousValue;
								getContinuousValueRule->GetFirstOperand()->SetOrigin(
								    KWDerivationRuleOperand::OriginAttribute);
								getContinuousValueRule->GetFirstOperand()
								    ->SetAttributeName(attribute->GetName());
								getContinuousValueRule->GetSecondOperand()->SetOrigin(
								    KWDerivationRuleOperand::OriginAttribute);
								getContinuousValueRule->GetSecondOperand()
								    ->SetAttributeName(secondaryAttribute->GetName());

								// Ajout d'un attribut dans la classe de travail
								workAttribute = new KWAttribute;
								workAttribute->SetDerivationRule(
								    getContinuousValueRule);
								workAttribute->SetName(workClass->BuildAttributeName(
								    attribute->GetName() +
								    getContinuousValueRule->GetName() +
								    secondaryAttribute->GetName()));
								workClass->InsertAttribute(workAttribute);
							}
							// Creation d'un attribut de calcul de mode dans le cas
							// categoriel
							else if (secondaryAttribute->GetType() == KWType::Symbol)
							{
								// Creation d'une regle pour le mode
								getSymbolValueRule = new KWDRGetSymbolValue;
								getSymbolValueRule->GetFirstOperand()->SetOrigin(
								    KWDerivationRuleOperand::OriginAttribute);
								getSymbolValueRule->GetFirstOperand()->SetAttributeName(
								    attribute->GetName());
								getSymbolValueRule->GetSecondOperand()->SetOrigin(
								    KWDerivationRuleOperand::OriginAttribute);
								getSymbolValueRule->GetSecondOperand()
								    ->SetAttributeName(secondaryAttribute->GetName());

								// Ajout d'un attribut dans la classe de travail
								workAttribute = new KWAttribute;
								workAttribute->SetDerivationRule(getSymbolValueRule);
								workAttribute->SetName(workClass->BuildAttributeName(
								    attribute->GetName() +
								    getSymbolValueRule->GetName() +
								    secondaryAttribute->GetName()));
								workClass->InsertAttribute(workAttribute);
							}
						}
						check(workAttribute);
					}

					// Suivant
					secondaryClass->GetNextAttribute(secondaryAttribute);
				}
			}

			// Attribut suivant
			currentClass->GetNextAttribute(attribute);
		}

		// Compilation de la classe de travail
		workClass->CompleteTypeInfo();
		workDomain->Compile();
	}

	// Appel de la methode ancetre
	KWLearningProblem::ComputeStats();

	// Restitution de l'etat initial
	if (cast(MYAnalysisResults*, GetAnalysisResults())->GetComputeBasicSecondaryStats())
	{
		KWClassDomain::SetCurrentDomain(currentDomain);
		delete workDomain;
	}
}

////////////////////////////////////////////////////////////
// Classe MYAnalysisSpec

MYAnalysisSpec::MYAnalysisSpec()
{
	// Specialisation des specifications dde modelisation
	// en detruisant le sous-objet cree dans la classe ancetre et en le remplacant par une version dediee
	delete modelingSpec;
	modelingSpec = new MYModelingSpec;
}

MYAnalysisSpec::~MYAnalysisSpec() {}
