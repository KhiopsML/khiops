// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIDRReinforcement.h"

void KIDRRegisterReinforcementRules()
{
	KWDerivationRule::RegisterDerivationRule(new KIDRClassifierReinforcer);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementInitialScoreAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementAttributeAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementPartAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementFinalScoreAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementClassChangeTagAt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KIDRClassifierReinforcer

KIDRClassifierReinforcer::KIDRClassifierReinforcer()
{
	SetName("ClassifierReinforcer");
	SetLabel("Classifier reinforcer");
	SetStructureName("ClassifierReinforcer");

	// Ajout d'un deuxieme operande en plus du classifier, pour les attributs de renforcement
	// Le dernier operande en nombre variable dediee aux pairers de variable est deplace en derniere position
	assert(GetOperandNumber() == 2);
	assert(GetVariableOperandNumber());
	SetOperandNumber(3);
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("VectorC");
	GetOperandAt(2)->SetType(KWType::Symbol);
}

KIDRClassifierReinforcer::~KIDRClassifierReinforcer()
{
	// Il faut appeler explicitement la methode dans le destructeur
	// Car meme si la methode est virtuelle, le destructeur ancetre
	// appelle la version de sa propre classe
	Clean();
}

KWDerivationRule* KIDRClassifierReinforcer::Create() const
{
	return new KIDRClassifierReinforcer;
}

boolean KIDRClassifierReinforcer::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDRNBClassifier* checkedNBClassifierRule;
	ObjectDictionary odCheckedPredictorPairVariables;
	KWDRSymbolVector* checkedReinforcementAttributeNames;
	StringVector svCheckedPredictorAttributeNames;
	StringVector svCheckedPredictorDataGridAttributeNames;
	ObjectArray oaCheckedPredictorDenseAttributeDataGridStatsRules;
	KWDerivationRule* referenceRule;
	int nFirstPairOperandIndex;
	int nPairIndex;
	KWDerivationRuleOperand* pairOperand;
	LongintDictionary ldCheckedPredictorAttributes;
	LongintNumericKeyDictionary lnkdUniqueReinforcedAttributes;
	int nAttribute;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KIDRClassifierService::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification des noms de variables de renforcement
	if (bOk)
	{
		// Acces a la regle de reference
		referenceRule = KWDerivationRule::LookupDerivationRule(GetName());
		assert(referenceRule != NULL);
		assert(referenceRule->GetVariableOperandNumber());
		assert(GetFirstOperand()->GetType() == referenceRule->GetFirstOperand()->GetType());

		// Recherche de l'index de la premiere paire de variable, correspondant au dernier operande
		// de la regle de reference, qui peut avoir ete redefini dans une sous-classe
		nFirstPairOperandIndex = referenceRule->GetOperandNumber() - 1;
		assert(referenceRule->GetOperandAt(nFirstPairOperandIndex)->GetType() == KWType::Symbol);

		// Acces aux parametres de la regle
		checkedNBClassifierRule =
		    cast(KWDRNBClassifier*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
		checkedReinforcementAttributeNames =
		    cast(KWDRSymbolVector*, GetSecondOperand()->GetReferencedDerivationRule(kwcOwnerClass));

		// Recherche de ses variables
		checkedNBClassifierRule->ExportAttributeNames(kwcOwnerClass, &svCheckedPredictorAttributeNames,
							      &svCheckedPredictorDataGridAttributeNames,
							      &oaCheckedPredictorDenseAttributeDataGridStatsRules);

		// Completion des noms de variables dans le cas des paires
		nPairIndex = 0;
		for (nAttribute = 0; nAttribute < svCheckedPredictorAttributeNames.GetSize(); nAttribute++)
		{
			// Les paires correspondent aux variables du predicteur, n'ayant pas de nom
			// Leur validite a ete verifiee precedement, et une variable a ete genere dans les
			// operandes en fin de regle
			if (svCheckedPredictorAttributeNames.GetAt(nAttribute) == "")
			{
				// Recherche de l'operande pour la paire utilisee par le predicteur
				assert(nFirstPairOperandIndex + nPairIndex < GetOperandNumber());
				pairOperand = GetOperandAt(nFirstPairOperandIndex + nPairIndex);

				// Memorisation du nom de l'attribut lie a la paire
				svCheckedPredictorAttributeNames.SetAt(nAttribute, pairOperand->GetAttributeName());

				// Incrementation de l'index de paire
				nPairIndex++;
			}
		}

		// On range les variables dans un dictionnaire
		for (nAttribute = 0; nAttribute < svCheckedPredictorAttributeNames.GetSize(); nAttribute++)
			ldCheckedPredictorAttributes.SetAt(svCheckedPredictorAttributeNames.GetAt(nAttribute), 1);

		// On verifie que les variables de renforcement sont bien des variables du predicteur
		for (nAttribute = 0; nAttribute < checkedReinforcementAttributeNames->GetValueNumber(); nAttribute++)
		{
			// Test si la variable existe pour le predicteur
			if (ldCheckedPredictorAttributes.Lookup(
				checkedReinforcementAttributeNames->GetValueAt(nAttribute).GetValue()) == 0)
			{
				AddError(sTmp + "Reinforced variable " +
					 checkedReinforcementAttributeNames->GetValueAt(nAttribute).GetValue() +
					 " not found among the classifier variables in the first rule operand");
				bOk = false;
			}

			// Test de l'unicite de la variable de renforcement
			if (lnkdUniqueReinforcedAttributes.Lookup(
				checkedReinforcementAttributeNames->GetValueAt(nAttribute).GetNumericKey()) == 1)
			{
				AddError(sTmp + "Reinforced variable " +
					 checkedReinforcementAttributeNames->GetValueAt(nAttribute).GetValue() +
					 " used twice");
				bOk = false;
			}
			lnkdUniqueReinforcedAttributes.SetAt(
			    checkedReinforcementAttributeNames->GetValueAt(nAttribute).GetNumericKey(), 1);

			// Arret si erreurs
			if (not bOk)
				break;
		}
	}
	return bOk;
}

void KIDRClassifierReinforcer::Compile(KWClass* kwcOwnerClass)
{
	const boolean bTrace = false;
	KWDRSymbolVector* reinforcementAttributeNames;
	int nAttribute;
	int nAttributeIndex;

	// Appel de la methode ancetre
	KIDRClassifierService::Compile(kwcOwnerClass);
	assert(classifierRule != NULL);

	// Memorisation des index des  variables de renforcement
	reinforcementAttributeNames =
	    cast(KWDRSymbolVector*, GetSecondOperand()->GetReferencedDerivationRule(kwcOwnerClass));
	ivReinforcementAttributeIndexes.SetSize(reinforcementAttributeNames->GetValueNumber());
	for (nAttribute = 0; nAttribute < reinforcementAttributeNames->GetValueNumber(); nAttribute++)
	{
		nAttributeIndex = GetPredictorAttributeRank(reinforcementAttributeNames->GetValueAt(nAttribute));
		assert(nAttributeIndex >= 0);
		ivReinforcementAttributeIndexes.SetAt(nAttribute, nAttributeIndex);
	}

	// Creation des structures de gestion des renforcement pour les acces par rang
	CreateRankedReinforcementStructures(GetTargetValueNumber(), GetReinforcementAttributeNumber(),
					    &svPredictorAttributeNames);

	// Initialisation des vecteurs de gestion de la bufferisation des calcul
	assert(ivTargetValueReinforcementNeeded.GetSize() == 0);
	ivTargetValueReinforcementNeeded.SetSize(GetTargetValueNumber());
	ivTargetValueReinforcementComputed.SetSize(GetTargetValueNumber());

	// Trace
	if (bTrace)
		WriteDetails(cout);
}

Object* KIDRClassifierReinforcer::ComputeStructureResult(const KWObject* kwoObject) const
{
	int nTarget;

	// Appel de la methode ancetre
	KIDRClassifierService::ComputeStructureResult(kwoObject);

	// On indique que les renforcements par classe cible sont a recalculer si necessaire
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		ivTargetValueReinforcementComputed.SetAt(nTarget, 0);
	return (Object*)this;
}

Continuous KIDRClassifierReinforcer::GetReinforcementInitialScoreAt(int nTargetValueRank) const
{
	require(IsCompiled());
	require(0 <= nTargetValueRank and nTargetValueRank < GetTargetValueNumber());

	// Retourne le score initial produit par le classifieur
	return classifierRule->GetTargetProbs()->GetAt(nTargetValueRank);
}

Symbol KIDRClassifierReinforcer::GetRankedReinforcementAttributeAt(int nTargetValueRank, int nReinforcementRank) const
{
	const KIAttributeReinforcement* attributeReinforcement;

	require(IsCompiled());
	require(0 <= nTargetValueRank and nTargetValueRank < GetTargetValueNumber());
	require(0 <= nReinforcementRank and nReinforcementRank < GetReinforcementAttributeNumber());

	// Calcul des renforcement pour des acces par rang
	ivTargetValueReinforcementNeeded.SetAt(nTargetValueRank, 1);
	if (ivTargetValueReinforcementComputed.GetAt(nTargetValueRank) == 0)
		ComputeRankedReinforcements();

	// Recherche des informations de renforcement
	attributeReinforcement = GetRankedReinforcementAt(nTargetValueRank, nReinforcementRank);

	// Retourne vide si pas de renforcement
	if (attributeReinforcement->GetReinforcementFinalScore() == 0)
		return Symbol();
	// Retourne le nom de l'attribut de renforcement sinon
	else
		return attributeReinforcement->GetAttributeName();
}

Symbol KIDRClassifierReinforcer::GetRankedReinforcementPartAt(int nTargetValueRank, int nReinforcementRank) const
{
	const KIAttributeReinforcement* attributeReinforcement;
	int nAttributeIndex;
	const KWDRDataGrid* dataGridRule;
	KWDataGridStats dataGridStats;
	ALString sCellLabel;

	require(IsCompiled());
	require(0 <= nTargetValueRank and nTargetValueRank < GetTargetValueNumber());
	require(0 <= nReinforcementRank and nReinforcementRank < GetReinforcementAttributeNumber());

	// Calcul des renforcement pour des acces par rang
	ivTargetValueReinforcementNeeded.SetAt(nTargetValueRank, 1);
	if (ivTargetValueReinforcementComputed.GetAt(nTargetValueRank) == 0)
		ComputeRankedReinforcements();

	// Recherche des informations de renforcement
	attributeReinforcement = GetRankedReinforcementAt(nTargetValueRank, nReinforcementRank);

	// Retourne vide si pas de renforcement
	if (attributeReinforcement->GetReinforcementFinalScore() == 0)
		return Symbol();
	// Retourne le nom de de la partie de l'attribut de renforcement sinon
	else
	{
		// Acces a l'attribut, la grille corespondante, et l'index source dans la grille
		nAttributeIndex = attributeReinforcement->GetAttributeIndex();
		dataGridRule = cast(const KWDRDataGrid*, oaPredictorAttributeDataGridRules.GetAt(nAttributeIndex));

		// Construction du libelle de la partie, valide dans les cas univaries et bivaries
		sCellLabel = BuildSourceCellLabel(dataGridRule, attributeReinforcement->GetReinforcementPartIndex());
		return (Symbol)sCellLabel;
	}
}

Continuous KIDRClassifierReinforcer::GetRankedReinforcementFinalScoreAt(int nTargetValueRank,
									int nReinforcementRank) const
{
	const KIAttributeReinforcement* attributeReinforcement;

	require(IsCompiled());
	require(0 <= nTargetValueRank and nTargetValueRank < GetTargetValueNumber());
	require(0 <= nReinforcementRank and nReinforcementRank < GetReinforcementAttributeNumber());

	// Calcul des renforcement pour des acces par rang
	ivTargetValueReinforcementNeeded.SetAt(nTargetValueRank, 1);
	if (ivTargetValueReinforcementComputed.GetAt(nTargetValueRank) == 0)
		ComputeRankedReinforcements();

	// Recherche des informations de renforcement
	attributeReinforcement = GetRankedReinforcementAt(nTargetValueRank, nReinforcementRank);

	// Retourne valeur manquante si pas de renforcement
	if (attributeReinforcement->GetReinforcementFinalScore() == 0)
		return KWContinuous::GetMissingValue();
	// Retourne le score final de renforcement sinon
	else
		return attributeReinforcement->GetReinforcementFinalScore();
}

Continuous KIDRClassifierReinforcer::GetRankedReinforcementClassChangeTagAt(int nTargetValueRank,
									    int nReinforcementRank) const
{
	const KIAttributeReinforcement* attributeReinforcement;

	require(IsCompiled());
	require(0 <= nTargetValueRank and nTargetValueRank < GetTargetValueNumber());
	require(0 <= nReinforcementRank and nReinforcementRank < GetReinforcementAttributeNumber());

	// Calcul des renforcement pour des acces par rang
	ivTargetValueReinforcementNeeded.SetAt(nTargetValueRank, 1);
	if (ivTargetValueReinforcementComputed.GetAt(nTargetValueRank) == 0)
		ComputeRankedReinforcements();

	// Recherche des informations de renforcement
	attributeReinforcement = GetRankedReinforcementAt(nTargetValueRank, nReinforcementRank);

	// Retourne valeur manquante si pas de renforcement
	if (attributeReinforcement->GetReinforcementFinalScore() == 0)
		return KWContinuous::GetMissingValue();
	// Retourne le tag de changement de classe sinon
	else
		return attributeReinforcement->GetReinforcementClassChangeTag();
}

void KIDRClassifierReinforcer::WriteDetails(ostream& ost) const
{
	int nAttribute;

	require(IsCompiled());

	// Appel de la methode ancetre
	KIDRClassifierService::WriteDetails(ost);

	// Variables de renforcement
	ost << " ## Reinforcement variables\t" << GetReinforcementAttributeNumber() << "\n";
	for (nAttribute = 0; nAttribute < GetReinforcementAttributeNumber(); nAttribute++)
		ost << "   - " << GetReinforcementAttributeNameAt(nAttribute) << "\n";
}

longint KIDRClassifierReinforcer::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KIDRClassifierService::GetUsedMemory();
	lUsedMemory += sizeof(KIDRClassifierInterpreter) - sizeof(KIDRClassifierService);
	lUsedMemory += oaTargetValueRankedAttributeReinforcements.GetSize() *
		       (sizeof(ObjectArray) + svPredictorAttributeNames.GetSize() * (sizeof(KIAttributeReinforcement*) +
										     sizeof(KIAttributeReinforcement)));
	lUsedMemory += ivTargetValueReinforcementNeeded.GetSize() - sizeof(IntVector);
	lUsedMemory += ivTargetValueReinforcementComputed.GetSize() - sizeof(IntVector);
	return lUsedMemory;
}

void KIDRClassifierReinforcer::Clean()
{
	ObjectArray* oaRankedAttributeReinforcements;
	int nTarget;

	// Methode ancetre
	KIDRClassifierService::Clean();

	// Nettoyage des structures specifiques
	for (nTarget = 0; nTarget < oaTargetValueRankedAttributeReinforcements.GetSize(); nTarget++)
	{
		oaRankedAttributeReinforcements =
		    cast(ObjectArray*, oaTargetValueRankedAttributeReinforcements.GetAt(nTarget));
		oaRankedAttributeReinforcements->DeleteAll();
	}
	oaTargetValueRankedAttributeReinforcements.DeleteAll();
	ivTargetValueReinforcementNeeded.SetSize(0);
	ivTargetValueReinforcementComputed.SetSize(0);
}

void KIDRClassifierReinforcer::CreateRankedReinforcementStructures(int nTargetValueNumber, int nAttributeNumber,
								   const SymbolVector* svAttributeNames)
{
	ObjectArray* oaRankedAttributeReinforcements;
	KIAttributeReinforcement* attributeReinforcement;
	int nTarget;
	int nAttribute;

	require(nTargetValueNumber > 0);
	require(nAttributeNumber > 0);
	require(svAttributeNames != NULL);
	require(nAttributeNumber <= svAttributeNames->GetSize());
	require(oaTargetValueRankedAttributeReinforcements.GetSize() == 0);

	// Creation des tableaux par valeur cible
	oaTargetValueRankedAttributeReinforcements.SetSize(nTargetValueNumber);
	for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
	{
		// Creation du tableau pour la valeur cible
		oaRankedAttributeReinforcements = new ObjectArray;
		oaTargetValueRankedAttributeReinforcements.SetAt(nTarget, oaRankedAttributeReinforcements);

		// Parametrage du tri du tableau
		oaRankedAttributeReinforcements->SetCompareFunction(KIAttributeReinforcementCompare);

		// Creation d'un objet de renforcement par variable
		oaRankedAttributeReinforcements->SetSize(nAttributeNumber);
		for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
		{
			attributeReinforcement = new KIAttributeReinforcement;
			attributeReinforcement->SetAttributeNames(svAttributeNames);
			oaRankedAttributeReinforcements->SetAt(nAttribute, attributeReinforcement);
		}
	}
}

void KIDRClassifierReinforcer::ComputeRankedReinforcements() const
{
	const boolean bTrace = false;
	ObjectArray* oaRankedAttributeReinforcements;
	KIAttributeReinforcement* attributeReinforcement;
	int nInitialPredictedTargetIndex;
	Symbol sInitialPredictedTarget;
	int nTarget;
	int i;
	int nAttribute;

	require(IsCompiled());
	require(ivDataGridSourceIndexes.GetSize() == GetPredictorAttributeNumber());
	require(classifierRule != NULL);
	require(oaTargetValueRankedAttributeReinforcements.GetSize() == GetTargetValueNumber());

	// Recherche de l'index de la valeur cible predite initial
	sInitialPredictedTarget = classifierRule->ComputeTargetValue();
	nInitialPredictedTargetIndex = GetTargetValueRank(sInitialPredictedTarget);

	// Calcul du renforcement par valeur cible
	for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
	{
		// On effectue le calcul que s'il est ne cessaire
		if (ivTargetValueReinforcementNeeded.GetAt(nTarget) == 1)
		{
			assert(ivTargetValueReinforcementComputed.GetAt(nTarget) == 0);

			// Acces au tableau des structures de renforcement
			oaRankedAttributeReinforcements =
			    cast(ObjectArray*, oaTargetValueRankedAttributeReinforcements.GetAt(nTarget));
			assert(oaRankedAttributeReinforcements->GetSize() == GetReinforcementAttributeNumber());

			// Calcul du renforcement par attribut
			for (i = 0; i < GetReinforcementAttributeNumber(); i++)
			{
				attributeReinforcement =
				    cast(KIAttributeReinforcement*, oaRankedAttributeReinforcements->GetAt(i));

				// Index de l'attribut de renforcement
				nAttribute = ivReinforcementAttributeIndexes.GetAt(i);

				// Calcul des information de renforcement
				ComputeReinforcementAt(attributeReinforcement, nTarget, nAttribute,
						       nInitialPredictedTargetIndex);
			}

			// Tri des Reinforcement
			oaRankedAttributeReinforcements->Sort();

			// On memorise que la calcul est effectue
			ivTargetValueReinforcementComputed.SetAt(nTarget, 1);
		}
	}

	// Trace des resultats
	if (bTrace)
	{
		// Affichage par valeur cible
		for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
		{
			// Affichage pour les valeurs effectivement calculees
			if (ivTargetValueReinforcementComputed.GetAt(nTarget) == 1)
			{
				cout << "Reinforcements for " << GetTargetValueAt(nTarget) << "\n";
				cout << "\t" << GetReinforcementInitialScoreAt(nTarget) << "\n";

				// Affichage par attribut
				for (i = 0; i < GetReinforcementAttributeNumber(); i++)
				{
					cout << "\t" << i + 1 << "\t";
					cout << GetRankedReinforcementAttributeAt(nTarget, i) << "\t";
					cout << GetRankedReinforcementPartAt(nTarget, i) << "\t";
					cout << KWContinuous::ContinuousToString(
						    GetRankedReinforcementFinalScoreAt(nTarget, i))
					     << "\t";
					cout << KWContinuous::ContinuousToString(
						    GetRankedReinforcementClassChangeTagAt(nTarget, i))
					     << "\n";
				}
			}
		}
	}
}

void KIDRClassifierReinforcer::ComputeReinforcementAt(KIAttributeReinforcement* attributeReinforcement,
						      int nTargetIndex, int nAttributeIndex,
						      int nInitialPredictedTargetIndex) const
{
	const boolean bTrace = false;
	const KWDRDataGrid* dataGridRule;
	const KWDRDataGridStats* dataGridStatsRule;
	int nSourcePartIndex;
	int nTargetPartIndex;
	int nSourcePartNumber;
	int nSource;
	int nTarget;
	Continuous cAtttributeWeight;
	Continuous cInitialScore;
	Continuous cNewScore;
	Continuous cFinalScore;
	ContinuousVector cvTargetLogProbNumeratorTerms;
	ContinuousVector cvNewScores;
	Continuous cCurrentSourceConditionalLogProb;
	Continuous cNewSourceConditionalLogProb;
	int nClassChangeTag;
	int nFinalPredictedTargetIndex;

	require(IsCompiled());
	require(attributeReinforcement != NULL);
	require(0 <= nTargetIndex and nTargetIndex < GetTargetValueNumber());
	require(0 <= nAttributeIndex and nAttributeIndex < GetPredictorAttributeNumber());
	require(0 <= nInitialPredictedTargetIndex and nInitialPredictedTargetIndex < GetTargetValueNumber());

	// Poids de l'attribut dans le classifieur
	cAtttributeWeight = classifierRule->GetDataGridWeightAt(nAttributeIndex);

	// Acces a la grille de l'attribut et a ses stats
	dataGridRule = cast(const KWDRDataGrid*, oaPredictorAttributeDataGridRules.GetAt(nAttributeIndex));
	dataGridStatsRule =
	    cast(const KWDRDataGridStats*, oaPredictorAttributeDataGridStatsRules.GetAt(nAttributeIndex));

	// Verification dans les cas univaries et bivaries
	assert(dataGridRule->GetAttributeNumber() == 2 or dataGridRule->GetAttributeNumber() == 3);
	assert(dataGridStatsRule->GetDataGridSourceAttributeNumber() == dataGridRule->GetAttributeNumber() - 1);
	assert(dataGridRule->GetAttributePartNumberAt(dataGridRule->GetAttributeNumber() - 1) ==
		   GetTargetValueNumber() or
	       dataGridRule->GetAttributeNumber() == 3);
	assert(dataGridStatsRule->GetDataGridTargetCellNumber() == GetTargetValueNumber());

	// Nombre de parties sources, dans le cas univarie ou bivarie
	nSourcePartNumber = dataGridStatsRule->GetDataGridSourceCellNumber();
	assert((dataGridRule->GetAttributeNumber() == 2 and
		nSourcePartNumber == dataGridRule->GetAttributePartNumberAt(0)) or
	       (dataGridRule->GetAttributeNumber() == 3 and
		nSourcePartNumber ==
		    dataGridRule->GetAttributePartNumberAt(0) * dataGridRule->GetAttributePartNumberAt(1)));

	// Recheche de l'index source dans la grille correspondante
	nSourcePartIndex = ivDataGridSourceIndexes.GetAt(nAttributeIndex);

	// Score initial avant renforcement
	cInitialScore = classifierRule->ComputeTargetProbAt(GetTargetValueAt(nTargetIndex));

	// Dimensionnement du vecteur de score
	cvNewScores.SetSize(GetTargetValueNumber());

	// Initialisation des resultats
	attributeReinforcement->SetAttributeIndex(nAttributeIndex);
	attributeReinforcement->SetReinforcementPartIndex(0);
	attributeReinforcement->SetReinforcementFinalScore(0);
	attributeReinforcement->SetReinforcementClassChangeTag(0);

	// Trace de debut pour le score initial
	if (bTrace)
	{
		cout << "- Compute reinforcement\t" << GetTargetValueAt(nTargetIndex) << "\t"
		     << GetPredictorAttributeNameAt(nAttributeIndex) << "\n";
		cout << "  - " << cInitialScore << "\t"
		     << cast(KWDRUnivariatePartition*, dataGridRule->GetOperandAt(0)->GetDerivationRule())
			    ->GetPartLabelAt(nSourcePartIndex)
		     << "\t" << classifierRule->ComputeTargetValue() << "\t"
		     << "Inital score"
		     << "\n";
	}

	// Parcours des cellules sources de la grille pour simuler un changement de partie dans les cas univaries et bivaries
	cFinalScore = cInitialScore;
	for (nSource = 0; nSource < nSourcePartNumber; nSource++)
	{
		// On ne traite pas la partie en cours
		if (nSource != nSourcePartIndex)
		{
			// On part des numerateurs des logs de probabilites du predicteur
			cvTargetLogProbNumeratorTerms.CopyFrom(classifierRule->GetTargetLogProbNumeratorTerms());

			// On met a jour ces termes en ajout celui de la nouvelle partie et en soustrayant celui de la partie en cours
			for (nTarget = 0; nTarget < GetTargetValueNumber(); nTarget++)
			{
				nTargetPartIndex =
				    classifierRule->GetDataGridSetTargetCellIndexAt(nAttributeIndex, nTarget);

				// Valeur de la log probabilite conditionnelle pour la partie source courante
				cCurrentSourceConditionalLogProb =
				    dataGridStatsRule->GetDataGridSourceConditionalLogProbAt(nSourcePartIndex,
											     nTargetPartIndex);

				// Valeur de la log probabilite conditionnelle pour la partie source a evaluer
				cNewSourceConditionalLogProb =
				    dataGridStatsRule->GetDataGridSourceConditionalLogProbAt(nSource, nTargetPartIndex);

				// Modification du terme de numerateur
				cvTargetLogProbNumeratorTerms.UpgradeAt(
				    nTarget, cAtttributeWeight *
						 (cNewSourceConditionalLogProb - cCurrentSourceConditionalLogProb));
			}

			// Calcul des nouveaux scores pour toute els classe, et memorisation du nouveau score
			classifierRule->ComputeTargetProbsFromNumeratorTerms(&cvTargetLogProbNumeratorTerms,
									     &cvNewScores);
			cNewScore = cvNewScores.GetAt(nTargetIndex);
			assert(0 <= cNewScore and cNewScore <= 1);

			// Memorisation des nouvelles informations de renforcement
			if (KWContinuous::CompareIndicatorValue(cNewScore, cFinalScore) > 0)
			{
				cFinalScore = cNewScore;

				// Recherche de l'index de valeur finale predite
				nFinalPredictedTargetIndex = ComputeArgMaxScores(&cvNewScores);

				// Calcul de l'indicateur de changement de classe
				if (nInitialPredictedTargetIndex == nTargetIndex)
					nClassChangeTag = 0;
				else if (nFinalPredictedTargetIndex == nTargetIndex)
					nClassChangeTag = 1;
				else
					nClassChangeTag = -1;

				// Memorisation des informations de renforcement
				attributeReinforcement->SetReinforcementPartIndex(nSource);
				attributeReinforcement->SetReinforcementFinalScore(cNewScore);
				attributeReinforcement->SetReinforcementClassChangeTag(nClassChangeTag);
			}

			// Trace pour la partie evaluee
			if (bTrace)
			{
				cout << "  - " << cNewScore << "\t" << nSource << "\t"
				     << BuildSourceCellLabel(dataGridRule, nSource) << "\t"
				     << GetTargetValueAt(ComputeArgMaxScores(&cvNewScores)) << "\n";
			}
		}
	}

	// Trace de fin pour le score final
	if (bTrace)
	{
		if (attributeReinforcement->GetReinforcementFinalScore() > cInitialScore)
		{
			cout << "  - " << attributeReinforcement->GetReinforcementFinalScore() << "\t"
			     << BuildSourceCellLabel(dataGridRule, attributeReinforcement->GetReinforcementPartIndex())
			     << "\t" << GetTargetValueAt(ComputeArgMaxScores(&cvNewScores)) << "\t"
			     << "Final score"
			     << "\n";
		}
	}
}

int KIDRClassifierReinforcer::ComputeArgMaxScores(const ContinuousVector* cvScores) const
{
	int nArgMax;
	Continuous cMaxScore;
	int nTarget;

	require(cvScores != NULL);
	require(cvScores->GetSize() > 0);

	// Recherche de l'index de plus grand score
	nArgMax = -1;
	cMaxScore = 0;
	for (nTarget = 0; nTarget < cvScores->GetSize(); nTarget++)
	{
		assert(cvScores->GetAt(nTarget) > 0);
		if (cvScores->GetAt(nTarget) > cMaxScore)
		{
			cMaxScore = cvScores->GetAt(nTarget);
			nArgMax = nTarget;
		}
	}
	assert(nArgMax != -1);
	return nArgMax;
}

////////////////////////////////////////////////////////////
// Classe KIDRReinforcementRule

KIDRReinforcementRule::KIDRReinforcementRule()
{
	nConstantTargetValueRank = -1;
	nConstantReinforcementRank = -1;
}

KIDRReinforcementRule::~KIDRReinforcementRule() {}

void KIDRReinforcementRule::Compile(KWClass* kwcOwnerClass)
{
	KIDRClassifierReinforcer* classifierReinforcer;
	Symbol sTargetValue;
	Symbol sPredictorAttributeName;

	require(GetOperandNumber() >= 2);
	require(GetOperandAt(1)->GetType() == KWType::Symbol);
	require(GetOperandNumber() == 2 or GetOperandAt(2)->GetType() == KWType::Continuous);

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Initialisation des rangs optimises
	nConstantTargetValueRank = -1;
	nConstantReinforcementRank = -1;

	// Acces au renforceur du premier operande
	classifierReinforcer =
	    cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));

	// Calcul du rang de la valeur cible si le deuxieme operande est constant
	if (GetOperandAt(1)->GetOrigin() == KWDerivationRuleOperand::OriginConstant)
	{
		sTargetValue = GetOperandAt(1)->GetSymbolConstant();
		nConstantTargetValueRank = classifierReinforcer->GetTargetValueRank(sTargetValue);
	}

	// Calcul du rang du second parametre si le troisieme operande est constant
	if (GetOperandNumber() >= 3 and GetOperandAt(2)->GetOrigin() == KWDerivationRuleOperand::OriginConstant)
	{
		nConstantReinforcementRank = (int)floor(GetOperandAt(2)->GetContinuousConstant() - 0.5);

		// On verifie sa validite
		if (nConstantReinforcementRank < 0 or
		    nConstantReinforcementRank >= classifierReinforcer->GetReinforcementAttributeNumber())
			nConstantReinforcementRank = -1;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementInitialScoreAt

KIDRReinforcementInitialScoreAt::KIDRReinforcementInitialScoreAt()
{
	SetName("ReinforcementInitialScoreAt");
	SetLabel("Reinforcement initial score");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
}

KIDRReinforcementInitialScoreAt::~KIDRReinforcementInitialScoreAt() {}

KWDerivationRule* KIDRReinforcementInitialScoreAt::Create() const
{
	return new KIDRReinforcementInitialScoreAt;
}

Continuous KIDRReinforcementInitialScoreAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;
	int nTargetValueRank;

	// Acces au renforceur
	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Calcul des parametres, uniquement si necessaire
	nTargetValueRank = nConstantTargetValueRank;
	if (nTargetValueRank == -1)
		nTargetValueRank = classifierReinforcer->GetTargetValueRank(GetOperandAt(1)->GetSymbolValue(kwoObject));

	// Attribut de contribution si valide
	if (nTargetValueRank >= 0)
		return classifierReinforcer->GetReinforcementInitialScoreAt(nTargetValueRank);
	// Valeur manquante sinon
	else
		return KWContinuous::GetMissingValue();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementAttributeAt

KIDRReinforcementAttributeAt::KIDRReinforcementAttributeAt()
{
	SetName("ReinforcementVariableAt");
	SetLabel("Reinforcement variable name");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRReinforcementAttributeAt::~KIDRReinforcementAttributeAt() {}

KWDerivationRule* KIDRReinforcementAttributeAt::Create() const
{
	return new KIDRReinforcementAttributeAt;
}

Symbol KIDRReinforcementAttributeAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;
	int nTargetValueRank;
	int nReinforcementRank;

	// Acces au renforceur
	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Calcul des parametres, uniquement si necessaire
	nTargetValueRank = nConstantTargetValueRank;
	if (nTargetValueRank == -1)
		nTargetValueRank = classifierReinforcer->GetTargetValueRank(GetOperandAt(1)->GetSymbolValue(kwoObject));
	nReinforcementRank = nConstantReinforcementRank;
	if (nReinforcementRank == -1)
		nReinforcementRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);

	// Attribut de renforcement si valide
	if (nTargetValueRank >= 0)
		return classifierReinforcer->GetRankedReinforcementAttributeAt(nTargetValueRank, nReinforcementRank);
	// Valeur manquante sinon
	else
		return Symbol();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementPartAt

KIDRReinforcementPartAt::KIDRReinforcementPartAt()
{
	SetName("ReinforcementPartAt");
	SetLabel("Reinforcement variable part");
	SetType(KWType::Symbol);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRReinforcementPartAt::~KIDRReinforcementPartAt() {}

KWDerivationRule* KIDRReinforcementPartAt::Create() const
{
	return new KIDRReinforcementPartAt;
}

Symbol KIDRReinforcementPartAt::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;
	int nTargetValueRank;
	int nReinforcementRank;

	// Acces au renforceur
	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Calcul des parametres, uniquement si necessaire
	nTargetValueRank = nConstantTargetValueRank;
	if (nTargetValueRank == -1)
		nTargetValueRank = classifierReinforcer->GetTargetValueRank(GetOperandAt(1)->GetSymbolValue(kwoObject));
	nReinforcementRank = nConstantReinforcementRank;
	if (nReinforcementRank == -1)
		nReinforcementRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);

	// Partie de l'attribut de renforcement si valide
	if (nTargetValueRank >= 0)
		return classifierReinforcer->GetRankedReinforcementPartAt(nTargetValueRank, nReinforcementRank);
	// Valeur manquante sinon
	else
		return Symbol();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementFinalScoreAt

KIDRReinforcementFinalScoreAt::KIDRReinforcementFinalScoreAt()
{
	SetName("ReinforcementFinalScoreAt");
	SetLabel("Reinforcement final score");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRReinforcementFinalScoreAt::~KIDRReinforcementFinalScoreAt() {}

KWDerivationRule* KIDRReinforcementFinalScoreAt::Create() const
{
	return new KIDRReinforcementFinalScoreAt;
}

Continuous KIDRReinforcementFinalScoreAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;
	int nTargetValueRank;
	int nReinforcementRank;

	// Acces au renforceur
	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Calcul des parametres, uniquement si necessaire
	nTargetValueRank = nConstantTargetValueRank;
	if (nTargetValueRank == -1)
		nTargetValueRank = classifierReinforcer->GetTargetValueRank(GetOperandAt(1)->GetSymbolValue(kwoObject));
	nReinforcementRank = nConstantReinforcementRank;
	if (nReinforcementRank == -1)
		nReinforcementRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);

	// Score final de l'attribut de renforcement si valide
	if (nTargetValueRank >= 0)
		return classifierReinforcer->GetRankedReinforcementFinalScoreAt(nTargetValueRank, nReinforcementRank);
	// Valeur manquante sinon
	else
		return KWContinuous::GetMissingValue();
}

///////////////////////////////////////////////////////////////////////////////
// Classe KIDRReinforcementClassChangeTagAt

KIDRReinforcementClassChangeTagAt::KIDRReinforcementClassChangeTagAt()
{
	SetName("ReinforcementClassChangeTagAt");
	SetLabel("Variable reinforcement class change tag");
	SetType(KWType::Continuous);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ClassifierReinforcer");
	GetOperandAt(1)->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KIDRReinforcementClassChangeTagAt::~KIDRReinforcementClassChangeTagAt() {}

KWDerivationRule* KIDRReinforcementClassChangeTagAt::Create() const
{
	return new KIDRReinforcementClassChangeTagAt;
}

Continuous KIDRReinforcementClassChangeTagAt::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KIDRClassifierReinforcer* classifierReinforcer;
	int nTargetValueRank;
	int nReinforcementRank;

	// Acces au renforceur
	classifierReinforcer = cast(KIDRClassifierReinforcer*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Calcul des parametres, uniquement si necessaire
	nTargetValueRank = nConstantTargetValueRank;
	if (nTargetValueRank == -1)
		nTargetValueRank = classifierReinforcer->GetTargetValueRank(GetOperandAt(1)->GetSymbolValue(kwoObject));
	nReinforcementRank = nConstantReinforcementRank;
	if (nReinforcementRank == -1)
		nReinforcementRank = (int)floor(GetOperandAt(2)->GetContinuousValue(kwoObject) - 0.5);

	// Indicateur de changement de classe de l'attribut de renforcement si valide
	if (nTargetValueRank >= 0)
		return classifierReinforcer->GetRankedReinforcementClassChangeTagAt(nTargetValueRank,
										    nReinforcementRank);
	// Valeur manquante sinon
	else
		return KWContinuous::GetMissingValue();
}

////////////////////////////////////////////////////////////
// Classe KIAttributeReinforcement

KIAttributeReinforcement::KIAttributeReinforcement()
{
	nAttributeIndex = 0;
	nReinforcementPartIndex = 0;
	cReinforcementFinalScore = 0;
	nReinforcementClassChangeTag = 0;
	svAttributeNames = NULL;
}

KIAttributeReinforcement::~KIAttributeReinforcement() {}

void KIAttributeReinforcement::SetAttributeNames(const SymbolVector* svNames)
{
	svAttributeNames = svNames;
}

const SymbolVector* KIAttributeReinforcement::GetAttributeNames() const
{
	return svAttributeNames;
}

int KIAttributeReinforcementCompare(const void* elem1, const void* elem2)
{
	int nCompare;
	KIAttributeReinforcement* attributeReinforcement1;
	KIAttributeReinforcement* attributeReinforcement2;

	// Acces aux objets
	attributeReinforcement1 = cast(KIAttributeReinforcement*, *(Object**)elem1);
	attributeReinforcement2 = cast(KIAttributeReinforcement*, *(Object**)elem2);

	// Comparaison selon la precision du type Continuous, pour eviter les differences a epsilon pres
	nCompare = -KWContinuous::CompareIndicatorValue(attributeReinforcement1->GetReinforcementFinalScore(),
							attributeReinforcement2->GetReinforcementFinalScore());

	// Comparaison sur le nom de l'attribut en cas d'egalite
	// Attention a prendre la valeur du Symbol contenant le nom
	if (nCompare == 0)
		nCompare = attributeReinforcement1->GetAttributeName().CompareValue(
		    attributeReinforcement2->GetAttributeName());
	return nCompare;
}
