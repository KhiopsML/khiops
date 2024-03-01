// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDClassBuilder.h"

//////////////////////////////////////////////////////////////////////////
// KDClassBuilder

KDClassBuilder::KDClassBuilder()
{
	multiTableFeatureConstruction = NULL;
	nAttributeNameIndex = 0;
}

KDClassBuilder::~KDClassBuilder() {}

void KDClassBuilder::SetMultiTableFeatureConstruction(KDMultiTableFeatureConstruction* featureConstructionParam)
{
	multiTableFeatureConstruction = featureConstructionParam;
}

KDMultiTableFeatureConstruction* KDClassBuilder::GetMultiTableFeatureConstruction() const
{
	require(multiTableFeatureConstruction != NULL);
	return multiTableFeatureConstruction;
}

KDConstructionDomain* KDClassBuilder::GetConstructionDomain() const
{
	require(multiTableFeatureConstruction != NULL);
	return multiTableFeatureConstruction->GetConstructionDomain();
}

KWClass* KDClassBuilder::BuildClassFromConstructedRules(const KWClass* initialClass,
							const ObjectArray* oaConstructedRules,
							KDSelectionOperandAnalyser* selectionOperandAnalyser) const
{
	KWClass* constructedClass;

	require(multiTableFeatureConstruction != NULL);
	require(initialClass != NULL);
	require(oaConstructedRules != NULL);
	require(selectionOperandAnalyser != NULL);
	require(selectionOperandAnalyser->GetClass() == initialClass);

	// Reinitialisation des index utilises pour nnommer les variables
	ResetAttributeNameIndexes();

	// Cas standard, sans regles intermediaires
	if (not GetConstructionDomain()->GetRuleOptimization())
		constructedClass =
		    BuildStandardClassFromConstructedRules(initialClass, oaConstructedRules, selectionOperandAnalyser);
	// Cas avec variable intermedaires
	else if (not GetConstructionDomain()->GetSparseOptimization())
		constructedClass =
		    BuildOptimizedClassFromConstructedRules(initialClass, oaConstructedRules, selectionOperandAnalyser);
	// Cas avec variable intermedaires et blocs sparse
	else
		constructedClass = BuildSparseOptimizedClassFromConstructedRules(initialClass, oaConstructedRules,
										 selectionOperandAnalyser);

	// Mise a jour de tous les couts des attributs
	UpdateAllAttributeCosts(constructedClass);

	// On retourne la classe construite
	return constructedClass;
}

KWClass* KDClassBuilder::BuildClassFromSelectionRules(const KWClass* initialClass,
						      KDSelectionOperandAnalyser* selectionOperandAnalyser) const
{
	KWClass* constructedClass;
	KDClassDomainCompliantRules classDomainCompliantRules;
	SortedList slUsedConstructedRules(KDSparseUsedConstructedRuleCompareCostName);

	require(multiTableFeatureConstruction != NULL);
	require(initialClass != NULL);
	require(selectionOperandAnalyser != NULL);
	require(selectionOperandAnalyser->GetClass() == initialClass);

	// Reinitialisation des index utilises pour nommer les variables
	ResetAttributeNameIndexes();

	// Construction de la classe avec ses regles de selection
	constructedClass = InternalBuildClassFromSelectionRules(initialClass, &classDomainCompliantRules,
								&slUsedConstructedRules, selectionOperandAnalyser);

	// Nettoyage
	classDomainCompliantRules.Clean();
	slUsedConstructedRules.DeleteAll();

	// On retourne la classe construite
	return constructedClass;
}

void KDClassBuilder::UpdateAllAttributeCosts(KWClass* constructedClass) const
{
	double dInitialAttributeCost;
	KWAttribute* attribute;

	require(constructedClass != NULL);
	require(GetMultiTableFeatureConstruction()->GetLearningSpec()->GetInitialAttributeNumber() != -1);

	// Calcul du cout de selection d'un attribut natif
	dInitialAttributeCost = GetMultiTableFeatureConstruction()->GetLearningSpec()->GetSelectionCost();

	// Nettoyage des meta-data des attributs de la classe
	constructedClass->RemoveAllAttributesMetaDataKey(KWAttribute::GetCostMetaDataKey());

	// Memorisation du cout des attributs construits
	attribute = constructedClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Si prise en compte de la regularization
		if (GetConstructionDomain()->GetConstructionRegularization())
		{
			// Test si attribut construit
			if (attribute->GetCost() > 0)
				attribute->SetMetaDataCost(attribute->GetCost());
			// Sinon, test si attribut initial
			else if (attribute->GetUsed() and KWType::IsSimple(attribute->GetType()) and
				 attribute->GetName() != GetMultiTableFeatureConstruction()->GetTargetAttributeName())
			{
				attribute->SetCost(dInitialAttributeCost);
				attribute->SetMetaDataCost(attribute->GetCost());
			}
			// Sinon, remise a zero, au cas ou
			else
				attribute->SetCost(0);
		}
		// Sinon, remise a zero
		else
			attribute->SetCost(0);

		// Attribut suivant
		constructedClass->GetNextAttribute(attribute);
	}
}

ALString KDClassBuilder::BuildConstructedAttributeName(const KDConstructedRule* rule) const
{
	ALString sAttributeName;

	require(rule != NULL);
	require(GetConstructionDomain() != NULL);
	require(GetConstructionDomain()->LookupConstructionRule(rule->GetName()) != NULL);

	// Construction d'un nom "interpretable"
	if (GetConstructionDomain()->GetInterpretableNames())
		sAttributeName = rule->BuildAttributeName(false);
	// Numerotation des variables
	else
		sAttributeName = BuildIndexedAttributeName();
	return sAttributeName;
}

ALString KDClassBuilder::BuildConstructedAttributeBlockName(const KDConstructedRule* rule) const
{
	ALString sAttributeName;

	require(rule != NULL);
	require(GetConstructionDomain() != NULL);
	require(GetConstructionDomain()->LookupConstructionRule(rule->GetName()) != NULL);

	// Construction d'un nom "interpretable"
	if (GetConstructionDomain()->GetInterpretableNames())
		sAttributeName = rule->BuildAttributeName(true);
	// Numerotation des variables
	else
		sAttributeName = BuildIndexedAttributeName();
	return sAttributeName;
}

ALString KDClassBuilder::BuildPartitionAttributeName(const KDConstructedPartition* partition) const
{
	ALString sAttributeName;

	require(partition != NULL);
	require(GetConstructionDomain() != NULL);

	// Construction d'un nom "interpretable"
	if (GetConstructionDomain()->GetInterpretableNames())
		sAttributeName = partition->BuildPartitionAttributeName();
	// Numerotation des variables
	else
		sAttributeName = BuildIndexedAttributeName();
	return sAttributeName;
}

ALString KDClassBuilder::BuildTablePartitionAttributeBlockName(const KDConstructedPartition* partition) const
{
	ALString sAttributeName;

	require(partition != NULL);
	require(GetConstructionDomain() != NULL);

	// Construction d'un nom "interpretable"
	if (GetConstructionDomain()->GetInterpretableNames())
		sAttributeName = partition->BuildTablePartitionAttributeBlockName();
	// Numerotation des variables
	else
		sAttributeName = BuildIndexedAttributeName();
	return sAttributeName;
}

ALString KDClassBuilder::BuildPartAttributeName(const KDConstructedPart* part) const
{
	ALString sAttributeName;

	require(part != NULL);
	require(GetConstructionDomain() != NULL);

	// Construction d'un nom "interpretable"
	if (GetConstructionDomain()->GetInterpretableNames())
		sAttributeName = part->BuildPartAttributeName();
	// Numerotation des variables
	else
		sAttributeName = BuildIndexedAttributeName();
	return sAttributeName;
}

ALString KDClassBuilder::BuildIndexedAttributeName() const
{
	const ALString sAttributePrefix = "ConstructedFeature";
	ALString sAttributeName;

	require(nAttributeNameIndex >= 0);

	// Construction d'un nom indexe
	nAttributeNameIndex++;
	sAttributeName = sAttributePrefix + IntToString(nAttributeNameIndex);
	return sAttributeName;
}

void KDClassBuilder::ResetAttributeNameIndexes() const
{
	nAttributeNameIndex = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

KWClass*
KDClassBuilder::BuildStandardClassFromConstructedRules(const KWClass* initialClass,
						       const ObjectArray* oaConstructedRules,
						       KDSelectionOperandAnalyser* selectionOperandAnalyser) const
{
	KWClassDomain* constructedDomain;
	KWClass* constructedClass;
	KDClassDomainCompliantRules classDomainCompliantRules;
	KDConstructedRule* constructedRule;
	KWAttribute* constructedAttribute;
	SortedList slUsedConstructedRules(KDSparseUsedConstructedRuleCompareCostName);
	KDSparseUsedConstructedRule* usedConstructedRule;
	ObjectArray oaNewUsedConstructedRules;
	ObjectArray oaConstructedAttributes;
	int nConstructedRuleNumber;
	int nRule;
	boolean bNewAttribute;

	require(multiTableFeatureConstruction != NULL);
	require(initialClass != NULL);
	require(oaConstructedRules != NULL);
	require(selectionOperandAnalyser != NULL);
	require(selectionOperandAnalyser->GetClass() == initialClass);
	require(not GetConstructionDomain()->GetRuleOptimization());

	////////////////////////////////////////////////////////////////////////////////////
	// En cas sans optimisation des regles, construction directe des regles
	// sans attributs intermediaires

	// Initialisation par duplication de la classe principale
	constructedDomain = initialClass->GetDomain()->CloneFromClass(initialClass);
	constructedDomain->SetName("Variable construction");
	constructedClass = constructedDomain->LookupClass(initialClass->GetName());

	// Collecte des attribut derives s'ils existent deja dans une classe du domaine
	constructedDomain->Compile();
	multiTableFeatureConstruction->ComputeAllClassesCompliantRules(constructedClass, &classDomainCompliantRules);

	// Creation d'une regle de derivation par regle construite
	nConstructedRuleNumber = 0;
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
	{
		constructedRule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));

		// Arret si assez de regles construites
		if (nConstructedRuleNumber == multiTableFeatureConstruction->GetMaxRuleNumber())
			break;

		// Creation directe, sans optimisation
		bNewAttribute = CreateConstructedRuleAttribute(constructedDomain, &classDomainCompliantRules,
							       constructedRule, constructedAttribute);

		// Memorisation de la regle construite associe a son attribut construit, egalement present dans les
		// attributs intermediaires en cas d'optimisation On garde la regle construite utilisee plutot que
		// l'attribut, pour pouvoir ensuite faire un tri par regle construite
		if (bNewAttribute)
		{
			// On cree un regle construite utilisee intermediaire, pour pouvoir les trier par la suite
			usedConstructedRule = new KDSparseUsedConstructedRule;
			usedConstructedRule->SetConstructedRule(constructedRule);
			usedConstructedRule->SetAttribute(constructedAttribute);
			usedConstructedRule->SetUsingRuleNumber(1);
			oaNewUsedConstructedRules.Add(usedConstructedRule);
		}

		// Comptage des nouveaux attribut crees
		if (bNewAttribute)
			nConstructedRuleNumber++;
	}

	// Tri des regles construites utilisee par regle construites, ce qui permettra d'obtenir les
	// attributs construits dans le bon ordre dans leur classe
	oaNewUsedConstructedRules.SetCompareFunction(KDSparseUsedConstructedRuleCompareCostName);
	oaNewUsedConstructedRules.Sort();

	// On collecte tous les attributs construits pour parametrer leur cout et les mettre en used
	// Attention: certains attributs peuvent etre potentiellement construits et intermediaires
	// Cela explique qu'on ne pouvait les mettre en used au moment de leur cosntruction
	oaConstructedAttributes.SetSize(oaNewUsedConstructedRules.GetSize());
	for (nRule = 0; nRule < oaNewUsedConstructedRules.GetSize(); nRule++)
	{
		usedConstructedRule = cast(KDSparseUsedConstructedRule*, oaNewUsedConstructedRules.GetAt(nRule));
		constructedAttribute = usedConstructedRule->GetAttribute();
		constructedAttribute->SetCost(usedConstructedRule->GetConstructedRule()->GetCost());
		constructedAttribute->SetUsed(true);
		constructedAttribute->SetLoaded(true);
		oaConstructedAttributes.SetAt(nRule, constructedAttribute);
	}

	// Nettoyage: il faut ici detruire les regles construites utilisee intermediaire, qui n'ont etet cree que pour
	// le tri
	oaNewUsedConstructedRules.DeleteAll();

	// Deplacement des attributs crees dans leur classe, avec ici aucun attribut intermediaires
	ReorderAttributesInClassDomain(initialClass->GetDomain(), &oaConstructedAttributes, &slUsedConstructedRules);

	// Completion et compilation de la classe et de son domaine
	constructedDomain->CompleteTypeInfo();
	constructedDomain->Compile();

	// On retourne la classe construite
	return constructedClass;
}

KWClass*
KDClassBuilder::BuildOptimizedClassFromConstructedRules(const KWClass* initialClass,
							const ObjectArray* oaConstructedRules,
							KDSelectionOperandAnalyser* selectionOperandAnalyser) const
{
	boolean bDisplay = false;
	KWClassDomain* constructedDomain;
	KDClassDomainCompliantRules classDomainCompliantRules;
	KWClass* constructedClass;
	KDConstructedRule* constructedRule;
	SortedList slUsedConstructedRules(KDSparseUsedConstructedRuleCompareCostName);
	KDSparseUsedConstructedRule* usedConstructedRule;
	ObjectArray oaNewUsedConstructedRules;
	ObjectArray oaConstructedAttributes;
	KWAttribute* constructedAttribute;
	int nConstructedRuleNumber;
	int nRule;
	boolean bNewAttribute;

	require(multiTableFeatureConstruction != NULL);
	require(initialClass != NULL);
	require(oaConstructedRules != NULL);
	require(selectionOperandAnalyser != NULL);
	require(selectionOperandAnalyser->GetClass() == initialClass);
	require(GetConstructionDomain()->GetRuleOptimization());
	require(not GetConstructionDomain()->GetSparseOptimization());

	////////////////////////////////////////////////////////////////////////////////////
	// En cas avec optimisation des regles, construction des regles apres analyse
	// du graphe de calcul et creation d'attributs intermediaires

	// Comptage des nombres d'utilisation des parties, partitions et dimensions de partition
	selectionOperandAnalyser->ResetAllUseCounts();
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
	{
		constructedRule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));
		constructedRule->IncrementUseCounts();
	}

	// Construction d'une nouvelle classe a partir des operandes de selection
	// En sortie, les attributs intermediaires sont crees pour les operandes de selection
	// et les attributs derives sont collectes dans classDomainDerivedAttributes
	constructedClass = InternalBuildClassFromSelectionRules(initialClass, &classDomainCompliantRules,
								&slUsedConstructedRules, selectionOperandAnalyser);
	constructedDomain = constructedClass->GetDomain();

	// Collecte de toutes les regles ou sous regles construites depuis les regles de construction
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
		CollectUsedRulesFromConstructedRule(&slUsedConstructedRules,
						    cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule)));

	// Affichage des regles trouvees
	if (bDisplay)
	{
		cout << "\nBuildClassFromConstructedRules" << endl;
		selectionOperandAnalyser->DisplayUsedSelectionOperands(cout);
		cout << "Rules\t" << oaConstructedRules->GetSize() << endl;
		cout << "=== All used constructed rules\t" << slUsedConstructedRules.GetCount() << " ===" << endl;
		DisplayUsedConstructedRules(&slUsedConstructedRules, cout);
		cout << "Initial rule attribute number\t"
		     << GetMultiTableFeatureConstruction()
			    ->GetClassDomainCompliantRules()
			    ->GetTotalInitialConstructedAttributeNumber()
		     << endl;
	}

	// Construction des attributs par parcours des regles construites
	nConstructedRuleNumber = 0;
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
	{
		constructedRule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));
		assert(LookupUsedConstructedRule(&slUsedConstructedRules, constructedRule) != NULL);

		// Arret si assez de regles construites
		if (nConstructedRuleNumber == multiTableFeatureConstruction->GetMaxRuleNumber())
			break;

		// Recherche si attribut existant
		usedConstructedRule = LookupUsedConstructedRule(&slUsedConstructedRules, constructedRule);
		constructedAttribute = cast(KWAttribute*, usedConstructedRule->GetAttribute());

		// Creation de l'attribut de facon optimisee
		bNewAttribute = false;
		if (constructedAttribute == NULL)
		{
			// Comme on cree les attribut les plus simples en premier, les attribut crees recursivement sont
			// soit des attributs cree precedemment (donc marque en bNewAttribute), soit des attributs
			// temporaires
			bNewAttribute = CreateOptimizedUsedRuleAttribute(constructedDomain, &classDomainCompliantRules,
									 &slUsedConstructedRules, usedConstructedRule);
			constructedAttribute = usedConstructedRule->GetAttribute();
		}
		assert(constructedAttribute != NULL);
		assert(usedConstructedRule->GetAttribute() == constructedAttribute);

		// Memorisation de la regle construite associe a son attribut construit, egalement present dans les
		// attributs intermediaires en cas d'optimisation On garde la regle construite utilisee plutot que
		// l'attribut, pour pouvoir ensuite faire un tri par regle construite
		if (bNewAttribute)
			oaNewUsedConstructedRules.Add(usedConstructedRule);

		// Comptage des nouveaux attribut crees
		if (bNewAttribute)
			nConstructedRuleNumber++;

		// Affichage
		if (bDisplay)
			cout << "Build attribute " << constructedAttribute->GetName() << " " << *constructedRule
			     << endl;
	}
	assert(oaNewUsedConstructedRules.GetSize() <= oaConstructedRules->GetSize());

	// Tri des regles construites utilisee par regle construites, ce qui permettra d'obtenir les
	// attributs construits dans le bon ordre dans leur classe
	oaNewUsedConstructedRules.SetCompareFunction(KDSparseUsedConstructedRuleCompareCostName);
	oaNewUsedConstructedRules.Sort();

	// On collecte tous les attributs construits pour parametrer leur cout et les mettre en used
	// Attention: certains attributs peuvent etre potentiellement construits et intermediaires
	// Cela explique qu'on ne pouvait les mettre en used au moment de leur cosntruction
	oaConstructedAttributes.SetSize(oaNewUsedConstructedRules.GetSize());
	for (nRule = 0; nRule < oaNewUsedConstructedRules.GetSize(); nRule++)
	{
		usedConstructedRule = cast(KDSparseUsedConstructedRule*, oaNewUsedConstructedRules.GetAt(nRule));
		constructedAttribute = usedConstructedRule->GetAttribute();
		constructedAttribute->SetCost(usedConstructedRule->GetConstructedRule()->GetCost());
		constructedAttribute->SetUsed(true);
		constructedAttribute->SetLoaded(true);
		oaConstructedAttributes.SetAt(nRule, constructedAttribute);
	}

	// Deplacement des attributs crees dans leur classe, avec les eventuels attributs intermediaires en dernier
	ReorderAttributesInClassDomain(initialClass->GetDomain(), &oaConstructedAttributes, &slUsedConstructedRules);

	// Nettoyage
	classDomainCompliantRules.Clean();
	slUsedConstructedRules.DeleteAll();

	// Completion et compilation de la classe et de son domaine
	constructedDomain->CompleteTypeInfo();
	constructedDomain->Compile();

	// On retourne la classe construite
	return constructedClass;
}

KWClass* KDClassBuilder::BuildSparseOptimizedClassFromConstructedRules(
    const KWClass* initialClass, const ObjectArray* oaConstructedRules,
    KDSelectionOperandAnalyser* selectionOperandAnalyser) const
{
	boolean bDisplay = false;
	KWClassDomain* constructedDomain;
	KDClassDomainCompliantRules classDomainCompliantRules;
	KWClass* constructedClass;
	KDConstructedRule* constructedRule;
	const KDConstructedPart* usedPart;
	SortedList slUsedConstructedRules(KDSparseUsedConstructedRuleCompareCostName);
	SortedList slUsedConstructedBlockRules(KDSparseUsedConstructedBlockRuleCompareCostName);
	ObjectArray oaUsedRules;
	KDSparseUsedConstructedRule* usedConstructedRule;
	ObjectArray oaNewUsedConstructedRules;
	ObjectArray oaConstructedAttributes;
	KWAttribute* constructedAttribute;
	int nConstructedRuleNumber;
	int nRule;
	boolean bNewAttribute;
	boolean bSparseConstruction;

	require(multiTableFeatureConstruction != NULL);
	require(initialClass != NULL);
	require(oaConstructedRules != NULL);
	require(selectionOperandAnalyser != NULL);
	require(selectionOperandAnalyser->GetClass() == initialClass);
	require(GetConstructionDomain()->GetRuleOptimization());
	require(GetConstructionDomain()->GetSparseOptimization());

	////////////////////////////////////////////////////////////////////////////////////
	// En cas avec optimisation des regles, construction des regles apres analyse
	// du graphe de calcul et creation d'attributs intermediaires

	// Comptage des nombres d'utilisation des parties, partitions et dimensions de partition
	selectionOperandAnalyser->ResetAllUseCounts();
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
	{
		constructedRule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));
		constructedRule->IncrementUseCounts();
	}

	// Construction d'une nouvelle classe a partir des operandes de selection
	// En sortie, les attributs intermediaires sont crees pour les operandes de selection
	// et les attributs derives sont collectes dans classDomainDerivedAttributes
	constructedClass = InternalBuildClassFromSelectionRules(initialClass, &classDomainCompliantRules,
								&slUsedConstructedRules, selectionOperandAnalyser);
	constructedDomain = constructedClass->GetDomain();

	// Collecte de toutes les regles ou sous regles construites depuis les regles de construction
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
		CollectUsedRulesFromConstructedRule(&slUsedConstructedRules,
						    cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule)));

	// Collecte de toutes les regles de type block construites depuis les regles de construction
	// Une seule regle utilisee de type bloc est memorisee par bloc de regles du meme type
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
		CollectUsedBlockRulesFromConstructedRule(&slUsedConstructedBlockRules,
							 cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule)));

	// Affichage des regles trouvees
	if (bDisplay)
	{
		cout << "\nBuildClassFromConstructedRules" << endl;
		selectionOperandAnalyser->DisplayUsedSelectionOperands(cout);
		cout << "Rules\t" << oaConstructedRules->GetSize() << endl;
		cout << "=== All used constructed rules\t" << slUsedConstructedRules.GetCount() << " ===" << endl;
		DisplayUsedConstructedRules(&slUsedConstructedRules, cout);
		cout << "=== All used constructed block rules\t" << slUsedConstructedBlockRules.GetCount()
		     << " ===" << endl;
		DisplayUsedConstructedBlockRules(&slUsedConstructedBlockRules, cout);
		cout << "============================================\n";
		cout << "Initial rule attribute number\t"
		     << GetMultiTableFeatureConstruction()
			    ->GetClassDomainCompliantRules()
			    ->GetTotalInitialConstructedAttributeNumber()
		     << endl;
	}

	// Construction des attributs par parcours des regles construites
	nConstructedRuleNumber = 0;
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
	{
		constructedRule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));
		assert(LookupUsedConstructedRule(&slUsedConstructedRules, constructedRule) != NULL);

		// Arret si assez de regles construites
		if (nConstructedRuleNumber == multiTableFeatureConstruction->GetMaxRuleNumber())
			break;

		// Recherche si attribut existant
		usedConstructedRule = LookupUsedConstructedRule(&slUsedConstructedRules, constructedRule);
		constructedAttribute = cast(KWAttribute*, usedConstructedRule->GetAttribute());

		// Creation de l'attribut de facon optimisee
		bNewAttribute = false;
		if (constructedAttribute == NULL)
		{
			// On determine s'il faut construire des blocs sparse ou non
			if (GetConstructionDomain()->GetSparseBlockMinSize() == 0)
				bSparseConstruction = true;
			else
			{
				// Acces a l'eventuelle partie en operande terminale de la, regle
				usedPart = constructedRule->GetUsedPart();

				// La partition doit avoir une taille minimale
				bSparseConstruction =
				    usedPart != NULL and usedPart->GetPartition()->GetActualPartitionSize() >=
							     GetConstructionDomain()->GetSparseBlockMinSize();
			}

			// Construction sparse ou non
			// Comme on cree les attribut les plus simples en premier, les attribut crees recursivement sont
			// soit des attributs cree precedemment (donc marque en bNewAttribute), soit des attributs
			// temporaires
			if (bSparseConstruction)
				bNewAttribute = CreateSparseOptimizedUsedRuleAttribute(
				    constructedDomain, &classDomainCompliantRules, &slUsedConstructedRules,
				    &slUsedConstructedBlockRules, usedConstructedRule);
			else
				bNewAttribute =
				    CreateOptimizedUsedRuleAttribute(constructedDomain, &classDomainCompliantRules,
								     &slUsedConstructedRules, usedConstructedRule);
			constructedAttribute = usedConstructedRule->GetAttribute();
		}
		assert(constructedAttribute != NULL);
		assert(usedConstructedRule->GetAttribute() == constructedAttribute);

		// Memorisation de la regle construite associe a son attribut construit, egalement present dans les
		// attributs intermediaires en cas d'optimisation On garde la regle construite utilisee plutot que
		// l'attribut, pour pouvoir ensuite faire un tri par regle construite
		if (bNewAttribute)
			oaNewUsedConstructedRules.Add(usedConstructedRule);

		// Comptage des nouveaux attribut crees
		if (bNewAttribute)
			nConstructedRuleNumber++;

		// Affichage
		if (bDisplay)
			cout << "Build attribute " << constructedAttribute->GetName() << " " << *constructedRule
			     << endl;
	}
	assert(oaNewUsedConstructedRules.GetSize() <= oaConstructedRules->GetSize());

	// Tri des regles construites utilisee par regle construites, ce qui permettra d'obtenir les
	// attributs construits dans le bon ordre dans leur classe
	oaNewUsedConstructedRules.SetCompareFunction(KDSparseUsedConstructedRuleCompareCostName);
	oaNewUsedConstructedRules.Sort();

	// On collecte tous les attributs construits pour parametrer leur cout et les mettre en used
	// Attention: certains attributs peuvent etre potentiellement construits et intermediaires
	// Cela explique qu'on ne pouvait les mettre en used au moment de leur cosntruction
	oaConstructedAttributes.SetSize(oaNewUsedConstructedRules.GetSize());
	for (nRule = 0; nRule < oaNewUsedConstructedRules.GetSize(); nRule++)
	{
		usedConstructedRule = cast(KDSparseUsedConstructedRule*, oaNewUsedConstructedRules.GetAt(nRule));
		constructedAttribute = usedConstructedRule->GetAttribute();
		constructedAttribute->SetCost(usedConstructedRule->GetConstructedRule()->GetCost());
		constructedAttribute->SetUsed(true);
		constructedAttribute->SetLoaded(true);
		oaConstructedAttributes.SetAt(nRule, constructedAttribute);
	}

	// Deplacement des attributs crees dans leur classe, avec les eventuels attributs intermediaires en dernier
	ReorderAttributesInClassDomain(initialClass->GetDomain(), &oaConstructedAttributes, &slUsedConstructedRules);

	// Reordonnancement des attributs de partition et bloc de type partition de table en fin de classe
	ReorderAttributeBlocksInClassDomain(&slUsedConstructedBlockRules);

	// Nettoyage
	classDomainCompliantRules.Clean();
	slUsedConstructedRules.DeleteAll();
	slUsedConstructedBlockRules.DeleteAll();

	// Affichage du dictionnaire construit
	if (bDisplay)
	{
		cout << "===============================================================" << endl;
		cout << "BuildSparseOptimizedClassFromConstructedRules" << endl;
		cout << "\tConstructed dictionary" << endl;
		cout << *constructedDomain << endl;
		cout << "===============================================================" << endl;
	}

	// Completion et compilation de la classe et de son domaine
	constructedDomain->CompleteTypeInfo();
	constructedDomain->Compile();

	// On retourne la classe construite
	return constructedClass;
}

/////////////////////////////////////////////////////////////////////////////////////////

void KDClassBuilder::CollectUsedRulesFromConstructedRule(SortedList* slUsedConstructedRules,
							 const KDConstructedRule* rule) const
{
	int nOperand;
	KDSparseUsedConstructedRule* usedConstructedRule;
	KDConstructedPartition* constructedPartition;
	const KDConstructedPartitionDimension* constructedPartitionDimension;
	int nDimension;

	require(GetConstructionDomain() != NULL);
	require(slUsedConstructedRules->GetCompareFunction() == KDSparseUsedConstructedRuleCompareCostName);
	require(slUsedConstructedRules != NULL);
	require(rule != NULL);

	// Recherche de la regle utilisee
	usedConstructedRule = LookupUsedConstructedRule(slUsedConstructedRules, rule);

	// Creation et memorisation si non trouve
	if (usedConstructedRule == NULL)
	{
		usedConstructedRule = new KDSparseUsedConstructedRule;
		usedConstructedRule->SetConstructedRule(rule);
		slUsedConstructedRules->Add(usedConstructedRule);
	}
	usedConstructedRule->SetUsingRuleNumber(usedConstructedRule->GetUsingRuleNumber() + 1);

	// Analyse des operandes pour identifier l'utilisation de regles construites
	for (nOperand = 0; nOperand < rule->GetOperandNumber(); nOperand++)
	{
		if (rule->GetOperandOriginAt(nOperand) == KDConstructedRule::Rule)
			CollectUsedRulesFromConstructedRule(slUsedConstructedRules, rule->GetRuleOperandAt(nOperand));
		else if (rule->GetOperandOriginAt(nOperand) == KDConstructedRule::Part)
		{
			constructedPartition = rule->GetPartOperandAt(nOperand)->GetPartition();
			for (nDimension = 0; nDimension < constructedPartition->GetDimensionNumber(); nDimension++)
			{
				constructedPartitionDimension = constructedPartition->GetDimensionAt(nDimension);
				if (constructedPartitionDimension->GetOrigin() == KDConstructedPartitionDimension::Rule)
					CollectUsedRulesFromConstructedRule(slUsedConstructedRules,
									    constructedPartitionDimension->GetRule());
			}
		}
	}
}

KDSparseUsedConstructedRule* KDClassBuilder::LookupUsedConstructedRule(const SortedList* slUsedConstructedRules,
								       const KDConstructedRule* searchedRule) const
{
	POSITION position;
	KDSparseUsedConstructedRule* usedConstructedRule;
	static KDSparseUsedConstructedRule searchedUsedConstructedRule;

	require(slUsedConstructedRules != NULL);
	require(slUsedConstructedRules->GetCompareFunction() == KDSparseUsedConstructedRuleCompareCostName);
	require(searchedRule != NULL);

	// Recherche de la regle utilisee
	searchedUsedConstructedRule.SetConstructedRule(searchedRule);
	usedConstructedRule = NULL;
	position = slUsedConstructedRules->Find(&searchedUsedConstructedRule);
	if (position != NULL)
		usedConstructedRule = cast(KDSparseUsedConstructedRule*, slUsedConstructedRules->GetAt(position));
	return usedConstructedRule;
}

void KDClassBuilder::DisplayUsedConstructedRules(const SortedList* slUsedConstructedRules, ostream& ost) const
{
	POSITION position;
	KDSparseUsedConstructedRule* usedConstructedRule;
	int nRule;

	require(slUsedConstructedRules != NULL);
	require(slUsedConstructedRules->GetCompareFunction() == KDSparseUsedConstructedRuleCompareCostName);

	// Parcours des regles de la liste
	ost << "Used constructed rules" << endl;
	nRule = 0;
	position = slUsedConstructedRules->GetHeadPosition();
	while (position != NULL)
	{
		usedConstructedRule = cast(KDSparseUsedConstructedRule*, slUsedConstructedRules->GetNext(position));
		nRule++;

		// Header
		if (nRule == 1)
		{
			usedConstructedRule->WriteHeaderLineReport(ost);
			ost << "\n";
		}

		// Nom de la regle
		usedConstructedRule->WriteLineReport(ost);
		ost << "\n";
	}
	ost << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////

KDSparseUsedConstructedBlockRule*
KDClassBuilder::CollectUsedBlockRulesFromConstructedRule(SortedList* slUsedConstructedBlockRules,
							 const KDConstructedRule* rule) const
{
	KDSparseUsedConstructedBlockRule* usedConstructedBlockRule;
	int nOperand;
	KDConstructedRule* operandRule;
	KDConstructedPartition* constructedPartition;
	const KDConstructedPartitionDimension* constructedPartitionDimension;
	int nDimension;
	boolean bIsFirstOperandSelection;
	boolean bIsSecondOperandBlock;

	require(GetConstructionDomain() != NULL);
	require(slUsedConstructedBlockRules->GetCompareFunction() == KDSparseUsedConstructedBlockRuleCompareCostName);
	require(slUsedConstructedBlockRules != NULL);
	require(rule != NULL);

	// Analyse des operandes pour identifier recursivement l'utilisation de regles construites
	bIsFirstOperandSelection = false;
	bIsSecondOperandBlock = false;
	for (nOperand = 0; nOperand < rule->GetOperandNumber(); nOperand++)
	{
		if (rule->GetOperandOriginAt(nOperand) == KDConstructedRule::Rule)
		{
			operandRule = rule->GetRuleOperandAt(nOperand);
			usedConstructedBlockRule =
			    CollectUsedBlockRulesFromConstructedRule(slUsedConstructedBlockRules, operandRule);

			// On analyse le premier operande pour savoir si on peut construire une regle de type partition
			if (nOperand == 0)
				bIsFirstOperandSelection = (operandRule->IsSelectionRule());
			// On analyse le second operande pour savoir si on peut construire une regle de type bloc
			else if (nOperand == 1)
				bIsSecondOperandBlock = (usedConstructedBlockRule != NULL);
		}
		else if (rule->GetOperandOriginAt(nOperand) == KDConstructedRule::Part)
		{
			constructedPartition = rule->GetPartOperandAt(nOperand)->GetPartition();
			for (nDimension = 0; nDimension < constructedPartition->GetDimensionNumber(); nDimension++)
			{
				constructedPartitionDimension = constructedPartition->GetDimensionAt(nDimension);
				if (constructedPartitionDimension->GetOrigin() == KDConstructedPartitionDimension::Rule)
					CollectUsedBlockRulesFromConstructedRule(
					    slUsedConstructedBlockRules, constructedPartitionDimension->GetRule());
			}
		}
	}

	// Test si on peut construire une regle de type partition
	usedConstructedBlockRule = NULL;
	if (usedConstructedBlockRule == NULL and rule->GetConstructionRule()->GetPartitionStatsRule() != NULL)
	{
		// Test si le premier operande est de type selection
		if (bIsFirstOperandSelection)
		{
			usedConstructedBlockRule = LookupUsedConstructedBlockRule(slUsedConstructedBlockRules, rule);

			// Creation et memorisation si non trouve
			if (usedConstructedBlockRule == NULL)
			{
				usedConstructedBlockRule = new KDSparseUsedConstructedBlockRule;
				usedConstructedBlockRule->SetConstructedBlockRule(rule);
				slUsedConstructedBlockRules->Add(usedConstructedBlockRule);
			}
		}
	}

	// Test si on peut construire une regle ayant un operande bloc de valeur
	if (usedConstructedBlockRule == NULL and rule->GetConstructionRule()->GetValueBlockRule() != NULL)
	{
		// Test si deuxieme operande de type bloc
		if (bIsSecondOperandBlock)
		{
			usedConstructedBlockRule = LookupUsedConstructedBlockRule(slUsedConstructedBlockRules, rule);

			// Creation et memorisation si non trouve
			if (usedConstructedBlockRule == NULL)
			{
				usedConstructedBlockRule = new KDSparseUsedConstructedBlockRule;
				usedConstructedBlockRule->SetConstructedBlockRule(rule);
				slUsedConstructedBlockRules->Add(usedConstructedBlockRule);
			}
		}
	}
	return usedConstructedBlockRule;
}

KDSparseUsedConstructedBlockRule*
KDClassBuilder::LookupUsedConstructedBlockRule(const SortedList* slUsedConstructedBlockRules,
					       const KDConstructedRule* searchedBlockRule) const
{
	POSITION position;
	KDSparseUsedConstructedBlockRule* usedConstructedBlockRule;
	static KDSparseUsedConstructedBlockRule searchedUsedConstructedBlockRule;

	require(slUsedConstructedBlockRules != NULL);
	require(slUsedConstructedBlockRules->GetCompareFunction() == KDSparseUsedConstructedBlockRuleCompareCostName);
	require(searchedBlockRule != NULL);

	// Recherche de la regle utilisee
	searchedUsedConstructedBlockRule.SetConstructedBlockRule(searchedBlockRule);
	usedConstructedBlockRule = NULL;
	position = slUsedConstructedBlockRules->Find(&searchedUsedConstructedBlockRule);
	if (position != NULL)
		usedConstructedBlockRule =
		    cast(KDSparseUsedConstructedBlockRule*, slUsedConstructedBlockRules->GetAt(position));
	return usedConstructedBlockRule;
}

void KDClassBuilder::DisplayUsedConstructedBlockRules(const SortedList* slUsedConstructedBlockRules, ostream& ost) const
{
	POSITION position;
	KDSparseUsedConstructedBlockRule* usedConstructedBlockRule;
	int nRule;

	require(slUsedConstructedBlockRules != NULL);
	require(slUsedConstructedBlockRules->GetCompareFunction() == KDSparseUsedConstructedBlockRuleCompareCostName);

	// Parcours des regles de la liste
	ost << "Used constructed block rules\t" << slUsedConstructedBlockRules->GetCount() << endl;
	nRule = 0;
	position = slUsedConstructedBlockRules->GetHeadPosition();
	while (position != NULL)
	{
		usedConstructedBlockRule =
		    cast(KDSparseUsedConstructedBlockRule*, slUsedConstructedBlockRules->GetNext(position));
		nRule++;

		// Affichage de la regle
		ost << *usedConstructedBlockRule->GetConstructedBlockRule() << "\t";
		ost << usedConstructedBlockRule->GetConstructedBlockRule()->BuildAttributeName(true) << "\n";
	}
	ost << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

KWClass* KDClassBuilder::InternalBuildClassFromSelectionRules(
    const KWClass* initialClass, KDClassDomainCompliantRules* classDomainCompliantRules,
    SortedList* slUsedConstructedRules, KDSelectionOperandAnalyser* selectionOperandAnalyser) const
{
	boolean bDisplay = false;
	boolean bFilterUnusedOperands = true;
	KWClassDomain* constructedDomain;
	KWClass* constructedClass;
	KWClass* kwcClass;
	int nClass;
	KDClassSelectionStats* classSelectionStats;
	KDClassSelectionOperandStats* classSelectionOperandStats;
	const KDConstructedPartitionDimension* partitionDimension;
	int nAttribute;
	KWAttribute* selectionAttribute;
	ObjectArray oaUsedRules;
	KDSparseUsedConstructedRule* usedConstructedRule;
	boolean bNewSelectionAttribute;
	ObjectArray oaSelectionConstructedAttributes;

	require(initialClass != NULL);
	require(multiTableFeatureConstruction != NULL);
	require(classDomainCompliantRules != NULL);
	require(classDomainCompliantRules->GetAllClassesCompliantRules()->GetSize() == 0);
	require(slUsedConstructedRules != NULL);
	require(slUsedConstructedRules->GetCount() == 0);
	require(selectionOperandAnalyser != NULL);
	require(selectionOperandAnalyser->GetClass() == initialClass);

	// Initialisation par duplication de la classe principale
	constructedDomain = initialClass->GetDomain()->CloneFromClass(initialClass);
	constructedDomain->SetName("Variable construction");
	constructedDomain->Compile();
	constructedClass = constructedDomain->LookupClass(initialClass->GetName());

	// Collecte des attribut derives s'ils existent deja dans une classe du domaine
	multiTableFeatureConstruction->ComputeAllClassesCompliantRules(constructedClass, classDomainCompliantRules);

	// En cas d'optimisation des regles, recherche des regles intermediaires
	if (GetConstructionDomain()->GetRuleOptimization())
	{
		// Collecte des regles ou sous regles construites pour les operandes de selection
		for (nClass = 0; nClass < selectionOperandAnalyser->GetClassSelectionStats()->GetSize(); nClass++)
		{
			classSelectionStats = cast(KDClassSelectionStats*,
						   selectionOperandAnalyser->GetClassSelectionStats()->GetAt(nClass));

			// Collecte pour la classe en cours
			for (nAttribute = 0;
			     nAttribute < classSelectionStats->GetClassSelectionOperandStats()->GetSize(); nAttribute++)
			{
				classSelectionOperandStats =
				    cast(KDClassSelectionOperandStats*,
					 classSelectionStats->GetClassSelectionOperandStats()->GetAt(nAttribute));
				partitionDimension = classSelectionOperandStats->GetPartitionDimension();

				// Si on ne filtre pas les operandes, on force l'incrementation de leur compteur
				// d'utilisation
				if (not bFilterUnusedOperands)
					cast(KDConstructedPartitionDimension*, partitionDimension)->IncrementUseCount();

				// Collecte si operande de type regle, si elle est effectivement utilisee
				if (partitionDimension->GetOrigin() == KDConstructedPartitionDimension::Rule and
				    partitionDimension->GetUseCount() > 0)
					CollectUsedRulesFromConstructedRule(slUsedConstructedRules,
									    partitionDimension->GetRule());
			}
		}

		// Affichage des regles trouvees
		if (bDisplay)
		{
			cout << "\nBuildClassFromSelectionRules" << endl;
			selectionOperandAnalyser->DisplayUsedSelectionOperands(cout);
			cout << "All used constructed rules for selection\t" << slUsedConstructedRules->GetCount()
			     << endl;
			DisplayUsedConstructedRules(slUsedConstructedRules, cout);
			cout << "Initial derived attribute number\t"
			     << classDomainCompliantRules->GetTotalInitialConstructedAttributeNumber() << endl;
			classDomainCompliantRules->DisplayAllConstructedAttributes(cout);
		}
	}

	// Creation et parametrage des attributs de selection dans les operandes de selection
	for (nClass = 0; nClass < selectionOperandAnalyser->GetClassSelectionStats()->GetSize(); nClass++)
	{
		classSelectionStats =
		    cast(KDClassSelectionStats*, selectionOperandAnalyser->GetClassSelectionStats()->GetAt(nClass));
		kwcClass = constructedDomain->LookupClass(classSelectionStats->GetClassName());

		// Creation d'autant d'attributs que d'operandes de selection effectivement utilisees
		for (nAttribute = 0; nAttribute < classSelectionStats->GetClassSelectionOperandStats()->GetSize();
		     nAttribute++)
		{
			classSelectionOperandStats =
			    cast(KDClassSelectionOperandStats*,
				 classSelectionStats->GetClassSelectionOperandStats()->GetAt(nAttribute));
			partitionDimension = classSelectionOperandStats->GetPartitionDimension();

			// Test si operande de selection effectivement utilise
			selectionAttribute = NULL;
			if (partitionDimension->GetUseCount() > 0)
			{
				// Si operande de type attribut, identification de l'attribut pour le mettre en Used
				if (partitionDimension->GetOrigin() == KDConstructedPartitionDimension::Attribute)
				{
					// Recherche de l'attribut dans la classe de selection
					selectionAttribute =
					    kwcClass->LookupAttribute(partitionDimension->GetAttribute()->GetName());
					check(selectionAttribute);

					// Affichage
					if (bDisplay)
						cout << "BuildSelectionDomain\tAtt\t" << kwcClass->GetName() << "\t"
						     << selectionAttribute->GetName() << endl;
				}
				// Si operande de type regle, on cree l'attribut
				else
				{
					assert(partitionDimension->GetOrigin() ==
					       KDConstructedPartitionDimension::Rule);

					// Creation de l'attribut de facon optimisee
					if (GetConstructionDomain()->GetRuleOptimization())
					{
						// Recherche dans les regles construites
						usedConstructedRule = LookupUsedConstructedRule(
						    slUsedConstructedRules, partitionDimension->GetRule());
						assert(usedConstructedRule != NULL);

						// Recherche de l'attribut associe a la regle construite
						selectionAttribute =
						    cast(KWAttribute*, usedConstructedRule->GetAttribute());

						// On construit l'attribut si necessaire
						bNewSelectionAttribute = false;
						if (selectionAttribute == NULL)
						{
							bNewSelectionAttribute = CreateOptimizedUsedRuleAttribute(
							    constructedDomain, classDomainCompliantRules,
							    slUsedConstructedRules, usedConstructedRule);
							selectionAttribute = usedConstructedRule->GetAttribute();
						}
						assert(selectionAttribute != NULL);
						assert(usedConstructedRule->GetAttribute() == selectionAttribute);
					}
					// Creation directe, sans optimisation
					else
						bNewSelectionAttribute = CreateConstructedRuleAttribute(
						    constructedDomain, classDomainCompliantRules,
						    partitionDimension->GetRule(), selectionAttribute);

					// Memorisation de l'attribut de selection construit, egalement present dans les
					// attributs intermediares en cas d'optimisation
					if (bNewSelectionAttribute)
						oaSelectionConstructedAttributes.Add(selectionAttribute);

					// Affichage
					if (bDisplay)
						cout << "BuildSelectionDomain\tCreate\t" << kwcClass->GetName() << "\t"
						     << *partitionDimension->GetRule() << endl;
				}
			}

			// Memorisation de l'attribut de selection dans son operande de selection
			classSelectionOperandStats->SetSelectionAttribute(selectionAttribute);
		}
	}

	// Deplacement des attributs crees dans leur classe, avec les eventuels attributs intermediaires en dernier
	ReorderAttributesInClassDomain(initialClass->GetDomain(), &oaSelectionConstructedAttributes,
				       slUsedConstructedRules);

	// Recompilation si necessaire du domaine
	if (oaSelectionConstructedAttributes.GetSize() > 0)
		constructedDomain->Compile();

	// On retourne la classe construite
	return constructedClass;
}

boolean KDClassBuilder::CreateConstructedRuleAttribute(KWClassDomain* classDomain,
						       KDClassDomainCompliantRules* classDomainCompliantRules,
						       const KDConstructedRule* constructedRule,
						       KWAttribute*& constructedAttribute) const
{
	boolean bNewAttribute;
	boolean bDisplay = false;
	KWDerivationRule* usedRuleDerivationRule;
	KWClass* usedRuleClass;
	const KWAttribute* foundDerivedAttribute;
	KDClassCompliantRules* classCompliantRule;
	ALString sVariableName;

	require(classDomain != NULL);
	require(classDomainCompliantRules != NULL);
	require(constructedRule != NULL);

	// Recherche de la classe sur laquelle porte la regle
	usedRuleClass = classDomain->LookupClass(constructedRule->GetClassName());
	check(usedRuleClass);

	// Creation de la regle directement a partir de la regle construite
	usedRuleDerivationRule = constructedRule->BuildDerivationRule();
	usedRuleDerivationRule->CompleteTypeInfo(usedRuleClass);

	// Recherche de l'attribut parmi les attribut existants
	classCompliantRule = classDomainCompliantRules->LookupClassCompliantRules(constructedRule->GetClassName());
	foundDerivedAttribute = classCompliantRule->LookupDerivedAttribute(usedRuleDerivationRule);

	// Memorisation de l'attribut associe a la regle construite
	constructedAttribute = NULL;
	if (foundDerivedAttribute != NULL)
	{
		constructedAttribute = cast(KWAttribute*, foundDerivedAttribute);
		bNewAttribute = false;

		// Destruction de la regle de derivation ayant servi a rechercher l'attribut
		delete usedRuleDerivationRule;
	}
	// Sinon, creation si necessaire d'un nouvel attribut
	else
	{
		bNewAttribute = true;

		// Nom de la variable
		sVariableName = BuildConstructedAttributeName(constructedRule);

		// Ajout d'un nouvel attribut a la classe
		constructedAttribute = new KWAttribute;
		constructedAttribute->SetName(usedRuleClass->BuildAttributeName(sVariableName));
		constructedAttribute->SetDerivationRule(usedRuleDerivationRule);

		// Completion de l'attribut cree
		assert(KWType::IsSimple(usedRuleDerivationRule->GetType()));
		constructedAttribute->SetType(usedRuleDerivationRule->GetType());

		// Insertion de l'attribut dans sa classe
		usedRuleClass->InsertAttribute(constructedAttribute);

		// Memorisation de l'attribut selon sa regle de derivation
		classCompliantRule->RegisterDerivedAttribute(constructedAttribute);
	}

	// Affichage de l'attribut cree
	if (bDisplay)
	{
		cout << usedRuleClass->GetDomain()->GetName() << "\t";
		cout << usedRuleClass->GetName() << "\t";
		cout << bNewAttribute << "\t";
		cout << constructedAttribute->GetName() << endl;
	}
	return bNewAttribute;
}

boolean KDClassBuilder::CreateOptimizedUsedRuleAttribute(KWClassDomain* classDomain,
							 KDClassDomainCompliantRules* classDomainCompliantRules,
							 const SortedList* slUsedConstructedRules,
							 KDSparseUsedConstructedRule* usedConstructedRule) const
{
	boolean bNewAttribute;
	boolean bDisplay = false;
	const KDConstructedRule* constructedRule;
	KWDerivationRule* usedRuleDerivationRule;
	KWClass* usedRuleClass;
	const KWAttribute* foundDerivedAttribute;
	KWAttribute* constructedAttribute;
	ALString sAttributeName;
	KWDerivationRuleOperand* operand;
	KWDerivationRule* operandRule;
	int i;
	KDConstructedRule* rule;
	KDConstructedPart* part;
	KDSparseUsedConstructedRule* foundUsedConstructedRule;
	KWAttribute* operandAttribute;
	KDClassCompliantRules* classCompliantRule;

	require(GetConstructionDomain()->GetRuleOptimization());
	require(classDomain != NULL);
	require(classDomainCompliantRules != NULL);
	require(classDomainCompliantRules->GetMainClassCompliantRules() != NULL);
	require(classDomainCompliantRules->GetMainClassCompliantRules()->GetClass()->GetDomain() == classDomain);
	require(usedConstructedRule != NULL);
	require(usedConstructedRule->GetConstructedRule() != NULL);
	require(usedConstructedRule->GetAttribute() == NULL);
	require(slUsedConstructedRules != NULL);
	require(LookupUsedConstructedRule(slUsedConstructedRules, usedConstructedRule->GetConstructedRule()) ==
		usedConstructedRule);

	// Acces a la regle construite
	constructedRule = usedConstructedRule->GetConstructedRule();

	// Recherche de la classe sur laquelle porte la regle
	usedRuleClass = classDomain->LookupClass(constructedRule->GetClassName());
	check(usedRuleClass);

	// Creation de la regle
	usedRuleDerivationRule = constructedRule->GetConstructionRule()->GetDerivationRule()->Clone();

	// Parametrage de ses operandes
	usedRuleDerivationRule->DeleteAllOperands();
	for (i = 0; i < constructedRule->GetOperandNumber(); i++)
	{
		// Creation de l'operande
		operand = new KWDerivationRuleOperand;
		usedRuleDerivationRule->AddOperand(operand);

		// Operande de type Attribute
		if (constructedRule->GetOperandOriginAt(i) == KDConstructedRule::Attribute)
		{
			operandAttribute = constructedRule->GetAttributeOperandAt(i);

			// Parametrage par les caracteristiques de l'attribut
			operand->SetType(operandAttribute->GetType());
			if (KWType::IsRelation(operandAttribute->GetType()))
				operand->SetObjectClassName(operandAttribute->GetClass()->GetName());
			operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			operand->SetAttributeName(operandAttribute->GetName());
		}
		// Operandes de type Rule
		else if (constructedRule->GetOperandOriginAt(i) == KDConstructedRule::Rule)
		{
			rule = constructedRule->GetRuleOperandAt(i);

			// Recherche dans les regles construites
			foundUsedConstructedRule = LookupUsedConstructedRule(slUsedConstructedRules, rule);
			assert(foundUsedConstructedRule != NULL);

			// Recherche de l'attribut associe a la regle construite
			operandAttribute = cast(KWAttribute*, foundUsedConstructedRule->GetAttribute());

			// Si non present, creation de l'attribut
			if (operandAttribute == NULL)
			{
				CreateOptimizedUsedRuleAttribute(classDomain, classDomainCompliantRules,
								 slUsedConstructedRules, foundUsedConstructedRule);
				operandAttribute = foundUsedConstructedRule->GetAttribute();
			}
			assert(operandAttribute != NULL);
			assert(foundUsedConstructedRule->GetAttribute() == operandAttribute);

			// Parametrage par les caracteristiques de l'attribut
			operand->SetType(operandAttribute->GetType());
			if (KWType::IsRelation(operandAttribute->GetType()))
				operand->SetObjectClassName(operandAttribute->GetClass()->GetName());
			operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			operand->SetAttributeName(operandAttribute->GetName());
		}
		// Test des operandes de type Part
		else if (constructedRule->GetOperandOriginAt(i) == KDConstructedRule::Part)
		{
			part = constructedRule->GetPartOperandAt(i);

			// Parametrage par la regle de derivation construite a partir de la partie de partition en
			// operande
			operandRule = part->BuildDerivationRule();
			operand->SetType(operandRule->GetType());
			if (KWType::IsRelation(operandRule->GetType()))
				operand->SetObjectClassName(operandRule->GetObjectClassName());
			operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
			operand->SetDerivationRule(operandRule);
		}
	}

	// Completion de la regle creee
	usedRuleDerivationRule->CompleteTypeInfo(usedRuleClass);

	// Recherche de l'attribut parmi les attribut existants
	classCompliantRule = classDomainCompliantRules->LookupClassCompliantRules(constructedRule->GetClassName());
	foundDerivedAttribute = classCompliantRule->LookupDerivedAttribute(usedRuleDerivationRule);

	// Memorisation de l'attribut associe a la regle construite
	constructedAttribute = NULL;
	if (foundDerivedAttribute != NULL)
	{
		constructedAttribute = cast(KWAttribute*, foundDerivedAttribute);
		bNewAttribute = false;

		// Destruction de la regle de derivation ayant servi a rechercher l'attribut
		delete usedRuleDerivationRule;
	}
	// Sinon, creation si necessaire d'un nouvel attribut
	else
	{
		bNewAttribute = true;

		// Nom de la variable
		sAttributeName = BuildConstructedAttributeName(usedConstructedRule->GetConstructedRule());

		// Ajout d'un nouvel attribut a la classe
		constructedAttribute = new KWAttribute;
		constructedAttribute->SetName(usedRuleClass->BuildAttributeName(sAttributeName));
		constructedAttribute->SetDerivationRule(usedRuleDerivationRule);
		constructedAttribute->SetUsed(false);

		// Completion de l'attribut cree
		assert(KWType::IsValue(usedRuleDerivationRule->GetType()));
		constructedAttribute->SetType(usedRuleDerivationRule->GetType());
		if (KWType::IsRelation(usedRuleDerivationRule->GetType()))
			constructedAttribute->SetClass(
			    usedRuleClass->GetDomain()->LookupClass(usedRuleDerivationRule->GetObjectClassName()));

		// Insertion de l'attribut dans sa classe
		usedRuleClass->InsertAttribute(constructedAttribute);

		// Memorisation de l'attribut selon sa regle de derivation
		classCompliantRule->RegisterDerivedAttribute(constructedAttribute);
	}

	// Memorisation dans la liste triee, avec l'attribut cree
	usedConstructedRule->SetAttribute(constructedAttribute);
	assert(LookupUsedConstructedRule(slUsedConstructedRules, usedConstructedRule->GetConstructedRule())
		   ->GetAttribute() == constructedAttribute);

	// Affichage de l'attribut cree
	if (bDisplay)
	{
		cout << usedRuleClass->GetDomain()->GetName() << "\t";
		cout << usedRuleClass->GetName() << "\t";
		cout << usedConstructedRule->GetUsingRuleNumber() << "\t";
		cout << constructedAttribute->GetName() << endl;
	}
	ensure(usedConstructedRule->GetAttribute() != NULL);
	return bNewAttribute;
}

boolean KDClassBuilder::CreateSparseOptimizedUsedRuleAttribute(KWClassDomain* classDomain,
							       KDClassDomainCompliantRules* classDomainCompliantRules,
							       const SortedList* slUsedConstructedRules,
							       const SortedList* slUsedConstructedBlockRules,
							       KDSparseUsedConstructedRule* usedConstructedRule) const
{
	boolean bNewAttribute;
	boolean bDisplay = false;
	const KDConstructedRule* constructedRule;
	KWDerivationRule* usedRuleDerivationRule;
	KWClass* usedRuleClass;
	ALString sAttributeName;
	KWDerivationRuleOperand* operand;
	int i;
	KDConstructedRule* rule;
	KDSparseUsedConstructedRule* foundUsedConstructedRule;
	KDSparseUsedConstructedBlockRule* foundUsedConstructedBlockRule;
	const KWAttribute* foundDerivedAttribute;
	KWAttribute* constructedAttribute;
	KWAttribute* operandAttribute;
	KDClassCompliantRules* classCompliantRule;

	require(GetConstructionDomain()->GetRuleOptimization());
	require(GetConstructionDomain()->GetSparseOptimization());
	require(classDomain != NULL);
	require(classDomainCompliantRules != NULL);
	require(classDomainCompliantRules->GetMainClassCompliantRules() != NULL);
	require(classDomainCompliantRules->GetMainClassCompliantRules()->GetClass()->GetDomain() == classDomain);
	require(slUsedConstructedRules != NULL);
	require(slUsedConstructedBlockRules != NULL);
	require(usedConstructedRule != NULL);
	require(usedConstructedRule->GetConstructedRule() != NULL);
	require(usedConstructedRule->GetAttribute() == NULL);
	require(not usedConstructedRule->GetConstructedRule()->IsSelectionRule());
	require(LookupUsedConstructedRule(slUsedConstructedRules, usedConstructedRule->GetConstructedRule()) ==
		usedConstructedRule);

	// Acces a la regle construite
	bNewAttribute = false;
	constructedRule = usedConstructedRule->GetConstructedRule();

	// Recherche de la classe sur laquelle porte la regle
	usedRuleClass = classDomain->LookupClass(constructedRule->GetClassName());
	check(usedRuleClass);

	// Affichage
	if (bDisplay)
		cout << "CreateSparseOptimizedUsedRuleAttribute " << usedRuleClass->GetName() << " " << *constructedRule
		     << endl;

	// Parcours des operandes pour construire les eventuels attributs supplementaires en partant de la base
	for (i = 0; i < constructedRule->GetOperandNumber(); i++)
	{
		// Operandes de type Rule
		if (constructedRule->GetOperandOriginAt(i) == KDConstructedRule::Rule)
		{
			rule = constructedRule->GetRuleOperandAt(i);

			// Affichage
			if (bDisplay)
				cout << "  operand " << i << " " << *rule << endl;

			// On ne traite pas les regles de selection, deja traitees dans les partitions
			if (rule->IsSelectionRule())
				continue;

			// Recherche dans les regles construites
			foundUsedConstructedRule = LookupUsedConstructedRule(slUsedConstructedRules, rule);
			assert(foundUsedConstructedRule != NULL);

			// Recherche de l'attribut associe a la regle construite
			operandAttribute = cast(KWAttribute*, foundUsedConstructedRule->GetAttribute());

			// Si non present, creation de l'attribut
			if (operandAttribute == NULL)
			{
				CreateSparseOptimizedUsedRuleAttribute(
				    classDomain, classDomainCompliantRules, slUsedConstructedRules,
				    slUsedConstructedBlockRules, foundUsedConstructedRule);
				assert(foundUsedConstructedRule->GetAttribute() != NULL);
			}
		}
	}

	// Recherche dans les regles construite de type bloc
	foundUsedConstructedBlockRule = LookupUsedConstructedBlockRule(slUsedConstructedBlockRules, constructedRule);

	// Cas d'une regle de type block
	if (foundUsedConstructedBlockRule != NULL)
	{
		assert(constructedRule->IsBlockRule());

		// Construction si necessaire du bloc et creation de l'attribut dans le bloc
		bNewAttribute = CreateAttributeBlock(classDomain, classDomainCompliantRules, slUsedConstructedRules,
						     slUsedConstructedBlockRules, usedConstructedRule,
						     foundUsedConstructedBlockRule);
		assert(usedConstructedRule->GetAttribute() != NULL);

		// Affichage de l'attribut cree
		if (bDisplay)
		{
			sAttributeName = BuildConstructedAttributeName(constructedRule);
			if (constructedRule->IsPartitionBlockRule())
				cout << " Partition block attribute\t";
			else
				cout << " Value block attribute\t";
			cout << usedRuleClass->GetDomain()->GetName() << "\t";
			cout << usedRuleClass->GetName() << "\t";
			cout << usedConstructedRule->GetUsingRuleNumber() << "\t";
			cout << sAttributeName << endl;
		}
	}
	// Cas standard
	else
	{
		assert(constructedRule->IsStandardRule());

		// Creation de la regle
		usedRuleDerivationRule = constructedRule->GetConstructionRule()->GetDerivationRule()->Clone();

		// Parametrage de ses operandes
		usedRuleDerivationRule->DeleteAllOperands();
		for (i = 0; i < constructedRule->GetOperandNumber(); i++)
		{
			// Creation de l'operande
			operand = new KWDerivationRuleOperand;
			usedRuleDerivationRule->AddOperand(operand);
			assert(constructedRule->GetOperandOriginAt(i) != KDConstructedRule::Part);

			// Operande de type Attribute
			if (constructedRule->GetOperandOriginAt(i) == KDConstructedRule::Attribute)
			{
				foundDerivedAttribute = constructedRule->GetAttributeOperandAt(i);

				// Parametrage par les caracteristiques de l'attribut
				operand->SetType(foundDerivedAttribute->GetType());
				if (KWType::IsRelation(foundDerivedAttribute->GetType()))
					operand->SetObjectClassName(foundDerivedAttribute->GetClass()->GetName());
				operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
				operand->SetAttributeName(foundDerivedAttribute->GetName());
			}
			// Operandes de type Rule
			else if (constructedRule->GetOperandOriginAt(i) == KDConstructedRule::Rule)
			{
				rule = constructedRule->GetRuleOperandAt(i);

				// Recherche dans les regles construites
				foundUsedConstructedRule = LookupUsedConstructedRule(slUsedConstructedRules, rule);
				assert(foundUsedConstructedRule != NULL);

				// Recherche de l'attribut associe a la regle construite
				foundDerivedAttribute = foundUsedConstructedRule->GetAttribute();
				assert(foundDerivedAttribute != NULL);

				// Parametrage par les caracteristiques de l'attribut
				operand->SetType(foundDerivedAttribute->GetType());
				if (KWType::IsRelation(foundDerivedAttribute->GetType()))
					operand->SetObjectClassName(foundDerivedAttribute->GetClass()->GetName());
				operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
				operand->SetAttributeName(foundDerivedAttribute->GetName());
			}
		}

		// Completion de la regle cree
		usedRuleDerivationRule->CompleteTypeInfo(usedRuleClass);

		// Recherche de l'attribut parmi les attribut existants
		classCompliantRule =
		    classDomainCompliantRules->LookupClassCompliantRules(constructedRule->GetClassName());
		foundDerivedAttribute = classCompliantRule->LookupDerivedAttribute(usedRuleDerivationRule);

		// Memorisation de l'attribut associe a la regle construite
		constructedAttribute = NULL;
		if (foundDerivedAttribute != NULL)
		{
			constructedAttribute = cast(KWAttribute*, foundDerivedAttribute);

			// Destruction de la regle de derivation ayant servi a rechercher l'attribut
			delete usedRuleDerivationRule;
		}
		// Sinon, creation si necessaire d'un nouvel attribut
		else
		{
			bNewAttribute = true;

			// Nom de la variable
			sAttributeName = BuildConstructedAttributeName(usedConstructedRule->GetConstructedRule());

			// Ajout d'un nouvel attribut a la classe
			constructedAttribute = new KWAttribute;
			constructedAttribute->SetName(usedRuleClass->BuildAttributeName(sAttributeName));
			constructedAttribute->SetDerivationRule(usedRuleDerivationRule);
			constructedAttribute->SetUsed(false);

			// Completion de l'attribut cree
			assert(KWType::IsValue(usedRuleDerivationRule->GetType()));
			constructedAttribute->SetType(usedRuleDerivationRule->GetType());
			if (KWType::IsRelation(usedRuleDerivationRule->GetType()))
				constructedAttribute->SetClass(usedRuleClass->GetDomain()->LookupClass(
				    usedRuleDerivationRule->GetObjectClassName()));

			// Insertion de l'attribut dans sa classe, ce qui permet la construction des autres attributs
			// avec les bons noms de variables
			usedRuleClass->InsertAttribute(constructedAttribute);

			// Memorisation de l'attribut selon sa regle de derivation
			classCompliantRule->RegisterDerivedAttribute(constructedAttribute);
		}

		// Memorisation dans la liste triee, avec l'attribut cree
		usedConstructedRule->SetAttribute(constructedAttribute);
		assert(LookupUsedConstructedRule(slUsedConstructedRules, usedConstructedRule->GetConstructedRule())
			   ->GetAttribute() == constructedAttribute);

		// Affichage de l'attribut cree
		if (bDisplay)
		{
			cout << " Standard attribute\t";
			cout << usedRuleClass->GetDomain()->GetName() << "\t";
			cout << usedRuleClass->GetName() << "\t";
			cout << usedConstructedRule->GetUsingRuleNumber() << "\t";
			cout << constructedAttribute->GetName() << endl;
		}
	}
	ensure(usedConstructedRule->GetAttribute() != NULL);
	return bNewAttribute;
}

void KDClassBuilder::ReorderAttributesInClassDomain(const KWClassDomain* kwcdInitialClassDomain,
						    const ObjectArray* oaConstructedAttributes,
						    const SortedList* slUsedConstructedRules) const
{
	NumericKeyDictionary nkdConstructedAttributes;
	KWClass* kwcClass;
	int nAttribute;
	KDSparseUsedConstructedRule* usedConstructedRule;
	KWAttribute* attribute;
	ObjectArray oaUsedConstructedRules;
	KWClass* kwcInitialClass;
	KWAttribute* initialAttribute;

	require(kwcdInitialClassDomain != NULL);
	require(oaConstructedAttributes != NULL);
	require(slUsedConstructedRules != NULL);

	// Parcours des attributs construits pour les deplacer en fin de classe
	// Sauf s'il sont dans des blocs
	for (nAttribute = 0; nAttribute < oaConstructedAttributes->GetSize(); nAttribute++)
	{
		attribute = cast(KWAttribute*, oaConstructedAttributes->GetAt(nAttribute));

		// Si attribut hors bloc
		if (attribute->GetAttributeBlock() == NULL)
		{
			// Recherche de la classe correspondant a l'attribut a deplacer
			kwcClass = attribute->GetParentClass();

			// Deplacement vers la fin de la classe
			kwcClass->MoveAttributeToClassTail(attribute);

			// Memorisation dans un dictionnaire
			nkdConstructedAttributes.SetAt(attribute, attribute);
		}
	}

	// Export des regles construites vers le tableau
	slUsedConstructedRules->ExportObjectArray(&oaUsedConstructedRules);

	// Parcours des attributs du second tableau pour les deplacer si necessaire en fin de classe
	for (nAttribute = 0; nAttribute < oaUsedConstructedRules.GetSize(); nAttribute++)
	{
		usedConstructedRule = cast(KDSparseUsedConstructedRule*, oaUsedConstructedRules.GetAt(nAttribute));

		// Acces a l'attribut
		attribute = cast(KWAttribute*, usedConstructedRule->GetAttribute());

		// On ne traite l'attribut que s'il est disponible et pas dans un bloc
		if (attribute != NULL and attribute->GetAttributeBlock() == NULL)
		{
			// On ne gere que les attributs non present dans la classe initiale
			kwcInitialClass = kwcdInitialClassDomain->LookupClass(attribute->GetParentClass()->GetName());
			assert(kwcInitialClass != NULL);
			initialAttribute = kwcInitialClass->LookupAttribute(attribute->GetName());
			if (initialAttribute == NULL)
			{
				// On ne deplace l'attribut que s'il n'etait pas principal
				if (nkdConstructedAttributes.Lookup(NUMERIC(attribute)) == NULL)
				{
					// Recherche de la classe correspondant a l'attribut a deplacer
					kwcClass = attribute->GetParentClass();

					// Deplacement vers la fin de la classe
					kwcClass->MoveAttributeToClassTail(attribute);
				}
			}
		}
	}
}

void KDClassBuilder::ReorderAttributeBlocksInClassDomain(const SortedList* slUsedConstructedBlockRules) const
{
	require(slUsedConstructedBlockRules != NULL);

	ObjectArray oaUsedConstructedBlockRules;
	int nBlock;
	KDSparseUsedConstructedBlockRule* usedConstructedBlockRule;
	const KDConstructedRule* constructedBlockRule;
	KWAttributeBlock* attributeBlock;
	KDConstructedPartition* constructedPartition;
	KWClass* kwcClass;
	NumericKeyDictionary nkdUsedPartitions;
	ObjectArray oaUsedPartitions;
	int nPartition;

	require(slUsedConstructedBlockRules != NULL);

	// Export des blocs d'attribut vers un tableau
	slUsedConstructedBlockRules->ExportObjectArray(&oaUsedConstructedBlockRules);

	// Parcours des blocs d'attributs
	for (nBlock = 0; nBlock < oaUsedConstructedBlockRules.GetSize(); nBlock++)
	{
		usedConstructedBlockRule =
		    cast(KDSparseUsedConstructedBlockRule*, oaUsedConstructedBlockRules.GetAt(nBlock));

		// Acces a la regle construite de type bloc et a la partition
		constructedBlockRule = usedConstructedBlockRule->GetConstructedBlockRule();
		assert(constructedBlockRule->IsBlockRule());

		// Deplacement du bloc en fin de classe, s'il exite
		attributeBlock = cast(KWAttributeBlock*, usedConstructedBlockRule->GetAttributeBlock());
		if (attributeBlock != NULL)
		{
			kwcClass = attributeBlock->GetParentClass();
			kwcClass->MoveAttributeBlockToClassTail(attributeBlock);

			// Tri des attributs du bloc par VarKey
			attributeBlock->SortAttributesByVarKey();

			// Memorisation des partitions
			if (constructedBlockRule->IsPartitionBlockRule())
			{
				// Recherche de la partition associe a une partie de l'operande de selection
				constructedPartition = cast(
				    KDConstructedPartition*,
				    constructedBlockRule->GetRuleOperandAt(0)->GetPartOperandAt(1)->GetPartition());
				assert(constructedPartition->GetPartitionAttribute() != NULL);

				// Memorisation de la partition pour laquelle des attributs ont ete crees
				nkdUsedPartitions.SetAt(constructedPartition, constructedPartition);
			}
		}
	}

	// Export de toutes les partitions identifiees dans un tableau
	nkdUsedPartitions.ExportObjectArray(&oaUsedPartitions);

	// On met les attributs de partition en fin de leur classe
	oaUsedPartitions.SetCompareFunction(KDConstructedPartitionCompare);
	oaUsedPartitions.Sort();
	for (nPartition = 0; nPartition < oaUsedPartitions.GetSize(); nPartition++)
	{
		constructedPartition = cast(KDConstructedPartition*, oaUsedPartitions.GetAt(nPartition));
		attributeBlock = cast(KWAttributeBlock*, constructedPartition->GetTablePartitionAttributeBlock());
		assert(attributeBlock != NULL);

		// On met les attributs de partition en fin de classe
		kwcClass = constructedPartition->GetPartitionAttribute()->GetParentClass();
		kwcClass->MoveAttributeToClassTail(cast(KWAttribute*, constructedPartition->GetPartitionAttribute()));
		kwcClass->MoveAttributeBlockToClassTail(attributeBlock);

		// Tri des attributs du bloc par VarKey
		attributeBlock->SortAttributesByVarKey();
	}
}

boolean KDClassBuilder::CreateAttributeBlock(KWClassDomain* classDomain,
					     KDClassDomainCompliantRules* classDomainCompliantRules,
					     const SortedList* slUsedConstructedRules,
					     const SortedList* slUsedConstructedBlockRules,
					     KDSparseUsedConstructedRule* usedConstructedRule,
					     KDSparseUsedConstructedBlockRule* usedConstructedBlockRule) const
{
	boolean bNewAttribute;
	const KDConstructedRule* constructedBlockRule;

	require(usedConstructedRule->GetAttribute() == NULL);

	// Acces a la regle construite de type bloc et a la partition
	constructedBlockRule = usedConstructedBlockRule->GetConstructedBlockRule();
	assert(constructedBlockRule->IsBlockRule());

	// Cas d'un bloc de type partition
	if (constructedBlockRule->IsPartitionBlockRule())
	{
		bNewAttribute = CreateAttributePartitionBlock(classDomain, classDomainCompliantRules,
							      slUsedConstructedRules, slUsedConstructedBlockRules,
							      usedConstructedRule, usedConstructedBlockRule);
	}
	// Cas de bloc de type valeur
	else
	{
		bNewAttribute = CreateAttributeValueBlock(classDomain, classDomainCompliantRules,
							  slUsedConstructedRules, slUsedConstructedBlockRules,
							  usedConstructedRule, usedConstructedBlockRule);
	}
	ensure(usedConstructedRule->GetAttribute() != NULL);
	return bNewAttribute;
}

boolean KDClassBuilder::CreateAttributePartitionBlock(KWClassDomain* classDomain,
						      KDClassDomainCompliantRules* classDomainCompliantRules,
						      const SortedList* slUsedConstructedRules,
						      const SortedList* slUsedConstructedBlockRules,
						      KDSparseUsedConstructedRule* usedConstructedRule,
						      KDSparseUsedConstructedBlockRule* usedConstructedBlockRule) const
{
	boolean bNewAttribute;
	const KDConstructedRule* constructedRule;
	const KDConstructedRule* constructedBlockRule;
	KDConstructedPart* constructedPart;
	KDConstructedPartition* constructedPartition;
	KWAttribute* partAttribute;
	KWClass* kwcClass;
	KWAttributeBlock* attributeBlock;
	const KWAttributeBlock* foundAttributeBlock;
	int nVarKey;
	KWDRTablePartitionStats* partitionBlockRule;
	int nOperand;
	KWDerivationRuleOperand* operand;
	const KWAttribute* attributeOperand;
	const KDConstructedRule* constructedRuleOperand;
	KDSparseUsedConstructedRule* foundUsedConstructedRuleOperand;
	ALString sBlockName;
	KDClassCompliantRules* classCompliantRule;

	require(GetConstructionDomain()->GetRuleOptimization());
	require(GetConstructionDomain()->GetSparseOptimization());
	require(classDomain != NULL);
	require(classDomainCompliantRules != NULL);
	require(classDomainCompliantRules->GetMainClassCompliantRules() != NULL);
	require(classDomainCompliantRules->GetMainClassCompliantRules()->GetClass()->GetDomain() == classDomain);
	require(slUsedConstructedRules != NULL);
	require(slUsedConstructedBlockRules != NULL);
	require(usedConstructedRule != NULL);
	require(usedConstructedRule->GetConstructedRule() != NULL);
	require(usedConstructedRule->GetAttribute() == NULL);
	require(not usedConstructedRule->GetConstructedRule()->IsSelectionRule());
	require(LookupUsedConstructedRule(slUsedConstructedRules, usedConstructedRule->GetConstructedRule()) ==
		usedConstructedRule);
	require(usedConstructedBlockRule != NULL);
	require(usedConstructedBlockRule->GetConstructedBlockRule() != NULL);
	require(LookupUsedConstructedBlockRule(slUsedConstructedBlockRules,
					       usedConstructedBlockRule->GetConstructedBlockRule()) ==
		usedConstructedBlockRule);

	// On ne traite la regle que s'il un attribut n'a pas deja ete cree
	bNewAttribute = false;
	if (usedConstructedRule->GetAttribute() == NULL)
	{
		// Acces a la regle construite
		constructedRule = usedConstructedRule->GetConstructedRule();

		// Acces a la regle construite de type bloc et a la partition
		constructedBlockRule = usedConstructedBlockRule->GetConstructedBlockRule();
		assert(constructedBlockRule->IsBlockRule());

		// Acces la classe
		kwcClass = classDomain->LookupClass(constructedBlockRule->GetClassName());

		// Recherche de la partie de l'operande de selection et de sa partition
		constructedPart = constructedRule->GetRuleOperandAt(0)->GetPartOperandAt(1);
		constructedPartition = constructedPart->GetPartition();

		// Creation de l'attribut de partition si necessaire
		if (constructedPartition->GetPartitionAttribute() == NULL)
			CreateAttributePartitionAttribute(classDomain, classDomainCompliantRules,
							  slUsedConstructedRules, slUsedConstructedBlockRules, kwcClass,
							  constructedPartition);

		// Creation de l'attribut associe a une partie de partition de table si necessaire
		// Le bloc associe a la partition de table est cree si necessaire par la methode appelee
		CreateAttributeTablePartitionBlock(classDomain, classDomainCompliantRules, slUsedConstructedRules,
						   slUsedConstructedBlockRules, kwcClass, constructedPart,
						   constructedPartition);

		// Recherche de l'attribut de partition de table s'il est deja parametre
		attributeBlock = cast(KWAttributeBlock*, usedConstructedBlockRule->GetAttributeBlock());
		partitionBlockRule = NULL;
		classCompliantRule = NULL;
		if (attributeBlock == NULL)
		{
			// Creation de la regle de derivation de type bloc de type partition
			partitionBlockRule =
			    cast(KWDRTablePartitionStats*,
				 constructedBlockRule->GetConstructionRule()->GetPartitionStatsRule()->Clone());
			assert(partitionBlockRule->GetOperandNumber() == constructedBlockRule->GetOperandNumber());

			// Parametrage du premier operande
			partitionBlockRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			partitionBlockRule->GetFirstOperand()->SetAttributeBlockName(
			    constructedPartition->GetTablePartitionAttributeBlock()->GetName());

			// Parametrage des autres operandes
			// Les deux premiers operandes de la regle construite sont la table secondaire et la partie de
			// selection, qui ont servi pour alimenter le premier operande de la regle de derivation, de
			// type TablePartition
			for (nOperand = 1; nOperand < constructedBlockRule->GetOperandNumber(); nOperand++)
			{
				operand = partitionBlockRule->GetOperandAt(nOperand);
				assert(constructedBlockRule->GetOperandOriginAt(nOperand) != KDConstructedRule::Part);

				// Operande de type Attribute
				if (constructedBlockRule->GetOperandOriginAt(nOperand) == KDConstructedRule::Attribute)
					attributeOperand = constructedBlockRule->GetAttributeOperandAt(nOperand);
				// Operandes de type Rule
				else
				{
					assert(constructedBlockRule->GetOperandOriginAt(nOperand) ==
					       KDConstructedRule::Rule);
					constructedRuleOperand = constructedBlockRule->GetRuleOperandAt(nOperand);

					// Recherche dans les regles construites
					foundUsedConstructedRuleOperand =
					    LookupUsedConstructedRule(slUsedConstructedRules, constructedRuleOperand);
					assert(foundUsedConstructedRuleOperand != NULL);

					// Recherche de l'attribut associe a la regle construite
					attributeOperand = foundUsedConstructedRuleOperand->GetAttribute();

					// Tous les attributs doivent avoir ete prealablement construits, et memorises
					// dans la liste des regles construites
					assert(attributeOperand != NULL);
				}

				// Parametrage par les caracteristiques de l'attribut, qui definit un operande de calcul
				operand->SetType(attributeOperand->GetType());
				assert(KWType::IsSimple(attributeOperand->GetType()));
				operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
				operand->SetAttributeName(attributeOperand->GetName());
			}

			// Finalisation de la regle creee
			partitionBlockRule->CompleteTypeInfo(kwcClass);

			// Recherche du bloc d'attribut parmi les blocs initiaux
			classCompliantRule =
			    classDomainCompliantRules->LookupClassCompliantRules(partitionBlockRule->GetClassName());
			foundAttributeBlock = classCompliantRule->LookupDerivedAttributeBlock(partitionBlockRule);

			// Cas d'un bloc deja existant, a completer
			if (foundAttributeBlock != NULL)
			{
				attributeBlock = cast(KWAttributeBlock*, foundAttributeBlock);

				// Destruction de la regle de block
				delete partitionBlockRule;

				// Memorisation du bloc d'attribut associe a la partition
				usedConstructedBlockRule->SetAttributeBlock(attributeBlock);
			}
		}

		// Recherche de la VarKey de la partie pour laquelle il faut creer un attribut
		nVarKey = constructedPartition->ComputeVariableKeyIndex(constructedPart);

		// Test d'existence de l'attribut dans l'eventuel bloc existant
		partAttribute = NULL;
		if (attributeBlock != NULL)
			partAttribute = attributeBlock->InternalLookupAttributeByContinuousVarKey(nVarKey);

		// Creation de l'attribut si necessaire
		if (partAttribute == NULL)
		{
			bNewAttribute = true;

			// Ajout d'un nouvel attribut a la classe, sans sa regle de derivation qui sera portee par le
			// bloc
			partAttribute = new KWAttribute;
			partAttribute->SetName(
			    kwcClass->BuildAttributeName(BuildConstructedAttributeName(constructedRule)));
			partAttribute->GetMetaData()->SetDoubleValueAt(KWAttributeBlock::GetAttributeKeyMetaDataKey(),
								       nVarKey);
			partAttribute->SetType(constructedRule->GetType());
			partAttribute->SetUsed(false);

			// Si bloc existant, insertion de l'attribut en fin de son bloc
			if (attributeBlock != NULL)
				kwcClass->InsertAttributeInBlock(partAttribute, attributeBlock);
			// Sinon, ajout de l'attribut dans sa classe, et creation du bloc
			else
			{
				kwcClass->InsertAttribute(partAttribute);

				// Creation du bloc dans la classe
				sBlockName = BuildConstructedAttributeBlockName(constructedBlockRule);
				attributeBlock = kwcClass->CreateAttributeBlock(
				    kwcClass->BuildAttributeBlockName(sBlockName), partAttribute, partAttribute);
				attributeBlock->SetDerivationRule(partitionBlockRule);

				// Memorisation du bloc d'attribut
				usedConstructedBlockRule->SetAttributeBlock(attributeBlock);

				// Memorisation du bloc d'attributs selon sa regle de derivation
				classCompliantRule->RegisterDerivedAttributeBlock(attributeBlock);
			}

			// Memorisation dans son bloc selon sa VarKey
			attributeBlock->InternalIndexAttributeByContinuousVarKey(nVarKey, partAttribute);
		}
		ensure(usedConstructedBlockRule->GetAttributeBlock() != NULL);

		// Memorisation dans la liste des de regles utilisee
		usedConstructedRule->SetAttribute(partAttribute);
	}
	ensure(usedConstructedRule->GetAttribute() != NULL);
	return bNewAttribute;
}

boolean KDClassBuilder::CreateAttributePartitionAttribute(KWClassDomain* classDomain,
							  KDClassDomainCompliantRules* classDomainCompliantRules,
							  const SortedList* slUsedConstructedRules,
							  const SortedList* slUsedConstructedBlockRules,
							  KWClass* kwcPartitionOwnerClass,
							  KDConstructedPartition* constructedPartition) const
{
	boolean bNewAttribute;
	ObjectArray oaAllSelectionParts;
	ALString sAttributeName;
	KWDerivationRule* partitionRule;
	const KWAttribute* foundPartitionAttribute;
	KWAttribute* partitionAttribute;
	KDClassCompliantRules* classCompliantRule;

	require(slUsedConstructedBlockRules != NULL);
	require(slUsedConstructedRules != NULL);
	require(kwcPartitionOwnerClass != NULL);
	require(classDomain->LookupClass(kwcPartitionOwnerClass->GetName()) == kwcPartitionOwnerClass);
	require(constructedPartition != NULL);
	require(constructedPartition->GetPartitionAttribute() == NULL);

	// Creation d'une regle de partition
	partitionRule = constructedPartition->BuildPartitionDerivationRule();
	partitionRule->CompleteTypeInfo(kwcPartitionOwnerClass);

	// Recherche de l'attribut parmi les attributs initiaux
	classCompliantRule = classDomainCompliantRules->LookupClassCompliantRules(partitionRule->GetClassName());
	foundPartitionAttribute = classCompliantRule->LookupDerivedAttribute(partitionRule);

	// Si attribut deja existant, on detruit la regle de construction
	if (foundPartitionAttribute != NULL)
	{
		bNewAttribute = false;
		partitionAttribute = cast(KWAttribute*, foundPartitionAttribute);
		delete partitionRule;
	}
	// Sinon, on creer un nouvel attribut dans la classe
	else
	{
		bNewAttribute = true;

		// Ajout d'un nouvel attribut a la classe
		partitionAttribute = new KWAttribute;
		partitionAttribute->SetDerivationRule(partitionRule);
		partitionAttribute->SetName(
		    kwcPartitionOwnerClass->BuildAttributeName(BuildPartitionAttributeName(constructedPartition)));
		partitionAttribute->SetUsed(false);

		// Completion du type de l'attribut cree
		partitionAttribute->SetType(partitionRule->GetType());
		partitionAttribute->SetStructureName(partitionRule->GetStructureName());

		// Insertion de l'attribut dans sa classe
		kwcPartitionOwnerClass->InsertAttribute(partitionAttribute);

		// Memorisation de l'attribut selon sa regle de derivation
		classCompliantRule->RegisterDerivedAttribute(partitionAttribute);
	}

	// Memorisation de l'attribut pour la partition
	constructedPartition->SetPartitionAttribute(partitionAttribute);
	ensure(constructedPartition->GetPartitionAttribute() != NULL);
	return bNewAttribute;
}

boolean KDClassBuilder::CreateAttributeTablePartitionBlock(KWClassDomain* classDomain,
							   KDClassDomainCompliantRules* classDomainCompliantRules,
							   const SortedList* slUsedConstructedRules,
							   const SortedList* slUsedConstructedBlockRules,
							   KWClass* kwcPartitionOwnerClass,
							   KDConstructedPart* constructedPart,
							   KDConstructedPartition* constructedPartition) const
{
	boolean bNewAttribute;
	KWClass* kwcPartitionClass;
	KWAttribute* partAttribute;
	ALString sAttributeName;
	KWDerivationRule* tablePartitionRule;
	KWAttributeBlock* attributeBlock;
	const KWAttributeBlock* foundAttributeBlock;
	int nVarKey;
	ALString sBlockName;
	KDClassCompliantRules* classCompliantRule;

	require(slUsedConstructedBlockRules != NULL);
	require(slUsedConstructedRules != NULL);
	require(kwcPartitionOwnerClass != NULL);
	require(classDomain->LookupClass(kwcPartitionOwnerClass->GetName()) == kwcPartitionOwnerClass);
	require(constructedPart != NULL);
	require(constructedPartition != NULL);
	require(constructedPart->GetPartition() == constructedPartition);

	// Recherche de l'attribut de partition de table s'il est deja parametre
	bNewAttribute = false;
	attributeBlock = cast(KWAttributeBlock*, constructedPartition->GetTablePartitionAttributeBlock());
	tablePartitionRule = NULL;
	classCompliantRule = NULL;
	if (attributeBlock == NULL)
	{
		// Creation d'une regle de partition de table
		tablePartitionRule = constructedPartition->BuildTablePartitionBlockDerivationRule();
		tablePartitionRule->CompleteTypeInfo(kwcPartitionOwnerClass);

		// Recherche du bloc d'attribut parmi les blocs initiaux
		classCompliantRule =
		    classDomainCompliantRules->LookupClassCompliantRules(tablePartitionRule->GetClassName());
		foundAttributeBlock = classCompliantRule->LookupDerivedAttributeBlock(tablePartitionRule);

		// Cas d'un bloc deja existant, a completer
		// (sinon, le bloc sera cree par la suite, en meme temps que l'attruibut associe a la partie)
		if (foundAttributeBlock != NULL)
		{
			attributeBlock = cast(KWAttributeBlock*, foundAttributeBlock);

			// Destruction de la regle de block
			delete tablePartitionRule;

			// Memorisation du bloc d'attribut associe a la partition
			constructedPartition->SetTablePartitionAttributeBlock(attributeBlock);
		}
	}

	// Recherche de la VarKey de la partie pour laquelle il faut creer un attribut
	assert(constructedPart->GetUseCount() >= 1);
	nVarKey = constructedPart->GetPartition()->ComputeVariableKeyIndex(constructedPart);

	// Test d'existence de l'attribut dans l'eventuel bloc existant
	partAttribute = NULL;
	if (attributeBlock != NULL)
		partAttribute = attributeBlock->InternalLookupAttributeByContinuousVarKey(nVarKey);

	// Creation de l'attribut si necessaire
	if (partAttribute == NULL)
	{
		bNewAttribute = true;

		// Recherche de la classe associee a la partition
		kwcPartitionClass = kwcPartitionOwnerClass->GetDomain()->LookupClass(
		    constructedPartition->GetPartitionClass()->GetName());

		// Creation de l'attribut
		partAttribute = new KWAttribute;
		partAttribute->SetName(
		    kwcPartitionOwnerClass->BuildAttributeName(BuildPartAttributeName(constructedPart)));
		partAttribute->GetMetaData()->SetDoubleValueAt(KWAttributeBlock::GetAttributeKeyMetaDataKey(), nVarKey);
		partAttribute->SetType(KWType::ObjectArray);
		partAttribute->SetClass(kwcPartitionClass);
		partAttribute->SetUsed(false);

		// Si bloc existant, insertion de l'attribut en fin de son bloc
		if (attributeBlock != NULL)
			kwcPartitionOwnerClass->InsertAttributeInBlock(partAttribute, attributeBlock);
		// Sinon, ajout de l'attribut dans sa classe, et creation du bloc
		else
		{
			kwcPartitionOwnerClass->InsertAttribute(partAttribute);

			// Creation du bloc dans la classe
			sBlockName = BuildTablePartitionAttributeBlockName(constructedPartition);
			attributeBlock = kwcPartitionOwnerClass->CreateAttributeBlock(
			    kwcPartitionOwnerClass->BuildAttributeBlockName(sBlockName), partAttribute, partAttribute);
			attributeBlock->SetDerivationRule(tablePartitionRule);

			// Memorisation du bloc d'attribut associe a la partition
			constructedPartition->SetTablePartitionAttributeBlock(attributeBlock);

			// Memorisation du bloc d'attributs selon sa regle de derivation
			classCompliantRule->RegisterDerivedAttributeBlock(attributeBlock);
		}

		// Memorisation dans sa partition selon sa VarKey
		attributeBlock->InternalIndexAttributeByContinuousVarKey(nVarKey, partAttribute);
	}
	ensure(constructedPartition->GetTablePartitionAttributeBlock() != NULL);
	ensure(attributeBlock->InternalLookupAttributeByContinuousVarKey(
		   constructedPartition->ComputeVariableKeyIndex(constructedPart)) != NULL);
	return bNewAttribute;
}

boolean KDClassBuilder::CreateAttributeValueBlock(KWClassDomain* classDomain,
						  KDClassDomainCompliantRules* classDomainCompliantRules,
						  const SortedList* slUsedConstructedRules,
						  const SortedList* slUsedConstructedBlockRules,
						  KDSparseUsedConstructedRule* usedConstructedRule,
						  KDSparseUsedConstructedBlockRule* usedConstructedBlockRule) const
{
	boolean bNewAttribute;
	boolean bDisplay = false;
	const KDConstructedRule* constructedRule;
	const KDConstructedRule* constructedBlockRule;
	NumericKeyDictionary nkdMainAttributes;
	KWClass* kwcClass;
	KWAttributeBlock* attributeBlock;
	const KWAttributeBlock* foundAttributeBlock;
	int nVarKey;
	KWDRValueBlockRule* valueBlockRule;
	const KWAttribute* operandAttribute;
	const KDConstructedRule* operandConstructedRule;
	KDSparseUsedConstructedRule* foundUsedConstructedRuleOperand;
	KDSparseUsedConstructedBlockRule* foundUsedConstructedBlockRuleOperand;
	ALString sBlockName;
	ALString sAttributeName;
	KWAttribute* blockCurrentAttribute;
	const KWAttribute* blockCurrentOperandAttribute;
	KDClassCompliantRules* classCompliantRule;

	require(GetConstructionDomain()->GetRuleOptimization());
	require(GetConstructionDomain()->GetSparseOptimization());
	require(classDomain != NULL);
	require(classDomainCompliantRules != NULL);
	require(classDomainCompliantRules->GetMainClassCompliantRules() != NULL);
	require(classDomainCompliantRules->GetMainClassCompliantRules()->GetClass()->GetDomain() == classDomain);
	require(slUsedConstructedRules != NULL);
	require(slUsedConstructedBlockRules != NULL);
	require(usedConstructedRule != NULL);
	require(usedConstructedRule->GetConstructedRule() != NULL);
	require(usedConstructedRule->GetAttribute() == NULL);
	require(not usedConstructedRule->GetConstructedRule()->IsSelectionRule());
	require(LookupUsedConstructedRule(slUsedConstructedRules, usedConstructedRule->GetConstructedRule()) ==
		usedConstructedRule);
	require(usedConstructedBlockRule != NULL);
	require(usedConstructedBlockRule->GetConstructedBlockRule() != NULL);
	require(LookupUsedConstructedBlockRule(slUsedConstructedBlockRules,
					       usedConstructedBlockRule->GetConstructedBlockRule()) ==
		usedConstructedBlockRule);

	// Acces a la regle construite
	constructedRule = usedConstructedRule->GetConstructedRule();

	// Acces a la regle construite de type bloc et a la partition
	constructedBlockRule = usedConstructedBlockRule->GetConstructedBlockRule();
	assert(constructedBlockRule->IsBlockRule());

	// Acces la classe
	kwcClass = classDomain->LookupClass(constructedBlockRule->GetClassName());

	// Recherche de l'attribut associe au deuxieme operande, parmi les blocs contsruits
	assert(constructedRule->GetOperandOriginAt(1) == KDConstructedRule::Rule);
	foundUsedConstructedRuleOperand =
	    LookupUsedConstructedRule(slUsedConstructedRules, constructedRule->GetRuleOperandAt(1));
	assert(foundUsedConstructedRuleOperand != NULL);

	// Recherche du bloc d'attribut associe au deuxieme operande, parmi les blocs contsruits
	assert(constructedBlockRule->GetOperandOriginAt(1) == KDConstructedRule::Rule);
	foundUsedConstructedBlockRuleOperand =
	    LookupUsedConstructedBlockRule(slUsedConstructedBlockRules, constructedBlockRule->GetRuleOperandAt(1));
	assert(foundUsedConstructedBlockRuleOperand != NULL);

	// AFfichage
	if (bDisplay)
	{
		cout << "CreateAttributeValueBlock " << kwcClass->GetName() << endl;
		cout << "\tRule " << *constructedRule << endl;
		cout << "\tBlock rule " << *constructedBlockRule << endl;
		cout << "\tUsed rule " << *foundUsedConstructedRuleOperand->GetConstructedRule() << endl;
		cout << "\tUsed block rule " << *foundUsedConstructedBlockRuleOperand->GetConstructedBlockRule()
		     << endl;
	}

	// Construction prealable de l'attribut utilise et de son bloc si necessaire, pour propagation recursive
	// Les bloc en operande doivent etre en effet construit au prealable,
	// car on s'appuie sur eux pour gerer correctement les VarKey
	if (foundUsedConstructedRuleOperand->GetAttribute() == NULL)
	{
		// Cas d'une regle de type partition
		if (foundUsedConstructedBlockRuleOperand->GetConstructedBlockRule()->IsPartitionBlockRule())
			CreateAttributePartitionBlock(classDomain, classDomainCompliantRules, slUsedConstructedRules,
						      slUsedConstructedBlockRules, foundUsedConstructedRuleOperand,
						      foundUsedConstructedBlockRuleOperand);
		// Cas d'une regle de type valeur
		else
			CreateAttributeValueBlock(classDomain, classDomainCompliantRules, slUsedConstructedRules,
						  slUsedConstructedBlockRules, foundUsedConstructedRuleOperand,
						  foundUsedConstructedBlockRuleOperand);
	}
	assert(foundUsedConstructedRuleOperand->GetAttribute() != NULL);

	// Recherche du bloc d'attribut s'il est deja parametre
	attributeBlock = cast(KWAttributeBlock*, usedConstructedBlockRule->GetAttributeBlock());
	valueBlockRule = NULL;
	classCompliantRule = NULL;
	if (attributeBlock == NULL)
	{
		// Creation de la regle de derivation de type bloc de valeur
		valueBlockRule = cast(KWDRValueBlockRule*,
				      constructedBlockRule->GetConstructionRule()->GetValueBlockRule()->Clone());
		assert(valueBlockRule->GetOperandNumber() == constructedBlockRule->GetOperandNumber());
		assert(valueBlockRule->GetOperandNumber() == 2);

		// Recherche de l'attribut associe au premier operande: le meme que celui de la construction rule
		// Operande de type Attribute
		if (constructedBlockRule->GetOperandOriginAt(0) == KDConstructedRule::Attribute)
		{
			operandAttribute = constructedBlockRule->GetAttributeOperandAt(0);
		}
		// Operande de type Rule
		else
		{
			assert(constructedBlockRule->GetOperandOriginAt(0) != KDConstructedRule::Part);
			assert(constructedBlockRule->GetOperandOriginAt(0) == KDConstructedRule::Rule);
			operandConstructedRule = constructedBlockRule->GetRuleOperandAt(0);

			// Recherche dans les regles construites
			foundUsedConstructedRuleOperand =
			    LookupUsedConstructedRule(slUsedConstructedRules, operandConstructedRule);
			assert(foundUsedConstructedRuleOperand != NULL);

			// Recherche de l'attribut associe a la regle construite
			operandAttribute = foundUsedConstructedRuleOperand->GetAttribute();

			// Tous les attributs doivent avoir ete prealablement construits, et memorises dans la liste des
			// regles construites
			assert(operandAttribute != NULL);
		}

		// Parametrage du premier l'operande
		assert(KWType::IsRelation(valueBlockRule->GetFirstOperand()->GetType()));
		valueBlockRule->GetFirstOperand()->SetObjectClassName(operandAttribute->GetClass()->GetName());
		valueBlockRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		valueBlockRule->GetFirstOperand()->SetAttributeName(operandAttribute->GetName());

		// Parametrage du bloc d'attribut pour le deuxieme operande, en recherchant dans les blocs contsruits
		assert(constructedBlockRule->GetOperandOriginAt(1) == KDConstructedRule::Rule);
		foundUsedConstructedBlockRuleOperand = LookupUsedConstructedBlockRule(
		    slUsedConstructedBlockRules, constructedBlockRule->GetRuleOperandAt(1));
		assert(foundUsedConstructedBlockRuleOperand != NULL);

		// Parametrage du deuxieme operande
		assert(foundUsedConstructedBlockRuleOperand->GetAttributeBlock() != NULL);
		valueBlockRule->GetSecondOperand()->SetType(
		    foundUsedConstructedBlockRuleOperand->GetAttributeBlock()->GetType());
		valueBlockRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		valueBlockRule->GetSecondOperand()->SetAttributeName(
		    foundUsedConstructedBlockRuleOperand->GetAttributeBlock()->GetName());

		// Finalisation de la regle creee
		valueBlockRule->CompleteTypeInfo(kwcClass);

		// Recherche du bloc d'attribut parmi les blocs initiaux
		classCompliantRule =
		    classDomainCompliantRules->LookupClassCompliantRules(valueBlockRule->GetClassName());
		foundAttributeBlock = classCompliantRule->LookupDerivedAttributeBlock(valueBlockRule);

		// Cas d'un bloc deja existant, a completer
		if (foundAttributeBlock != NULL)
		{
			attributeBlock = cast(KWAttributeBlock*, foundAttributeBlock);

			// Destruction de la regle de block
			delete valueBlockRule;

			// Memorisation du bloc d'attribut associe a la partition
			usedConstructedBlockRule->SetAttributeBlock(attributeBlock);
		}
	}

	// Recherche de l'attribut associe a la regle en operande construite
	blockCurrentOperandAttribute = foundUsedConstructedRuleOperand->GetAttribute();
	assert(blockCurrentOperandAttribute != NULL);
	assert(blockCurrentOperandAttribute->GetAttributeBlock() != NULL);

	// Recherche de la VarKey de variable
	nVarKey = blockCurrentOperandAttribute->GetAttributeBlock()->GetContinuousVarKey(blockCurrentOperandAttribute);

	// Recherche de l'attribut dans le bloc existant
	blockCurrentAttribute = NULL;
	if (attributeBlock != NULL)
		blockCurrentAttribute = attributeBlock->InternalLookupAttributeByContinuousVarKey(nVarKey);

	// Creation de l'attribut si necessaire
	bNewAttribute = false;
	if (blockCurrentAttribute == NULL)
	{
		bNewAttribute = true;

		// Ajout d'un nouvel attribut a la classe, sans sa regle de derivation qui sera portee par le bloc
		blockCurrentAttribute = new KWAttribute;
		blockCurrentAttribute->SetName(
		    kwcClass->BuildAttributeName(BuildConstructedAttributeName(constructedRule)));
		blockCurrentAttribute->GetMetaData()->SetDoubleValueAt(KWAttributeBlock::GetAttributeKeyMetaDataKey(),
								       nVarKey);
		blockCurrentAttribute->SetType(constructedRule->GetType());
		blockCurrentAttribute->SetUsed(false);

		// Si bloc existant, insertion de l'attribut en fin de son bloc
		if (attributeBlock != NULL)
			kwcClass->InsertAttributeInBlock(blockCurrentAttribute, attributeBlock);
		// Sinon, ajout de l'attribut dans sa classe, et creation du bloc
		else
		{
			kwcClass->InsertAttribute(blockCurrentAttribute);

			// Creation du bloc dans la classe
			sBlockName = BuildConstructedAttributeBlockName(constructedBlockRule);
			attributeBlock = kwcClass->CreateAttributeBlock(kwcClass->BuildAttributeBlockName(sBlockName),
									blockCurrentAttribute, blockCurrentAttribute);
			attributeBlock->SetDerivationRule(valueBlockRule);

			// Memorisation du bloc d'attribut
			usedConstructedBlockRule->SetAttributeBlock(attributeBlock);

			// Memorisation du bloc d'attributs selon sa regle de derivation
			classCompliantRule->RegisterDerivedAttributeBlock(attributeBlock);
		}

		// Memorisation dans son bloc selon sa VarKey
		attributeBlock->InternalIndexAttributeByContinuousVarKey(nVarKey, blockCurrentAttribute);
	}
	ensure(usedConstructedBlockRule->GetAttributeBlock() != NULL);

	// Memorisation dans la liste des de regles utilisee
	usedConstructedRule->SetAttribute(blockCurrentAttribute);
	ensure(usedConstructedRule->GetAttribute() != NULL);
	return bNewAttribute;
}

//////////////////////////////////////////////////////////////////////////
// KDSparseUsedConstructedRule

KDSparseUsedConstructedRule::KDSparseUsedConstructedRule()
{
	usedConstructedRule = NULL;
	usedAttribute = NULL;
	nUsingRuleNumber = 0;
}

KDSparseUsedConstructedRule::~KDSparseUsedConstructedRule() {}

void KDSparseUsedConstructedRule::SetConstructedRule(const KDConstructedRule* rule)
{
	usedConstructedRule = rule;
}

const KDConstructedRule* KDSparseUsedConstructedRule::GetConstructedRule() const
{
	return usedConstructedRule;
}

void KDSparseUsedConstructedRule::SetAttribute(KWAttribute* attribute)
{
	usedAttribute = attribute;
}

KWAttribute* KDSparseUsedConstructedRule::GetAttribute() const
{
	return usedAttribute;
}

void KDSparseUsedConstructedRule::SetUsingRuleNumber(int nNumber)
{
	require(nNumber >= 0);
	nUsingRuleNumber = nNumber;
}

int KDSparseUsedConstructedRule::GetUsingRuleNumber() const
{
	return nUsingRuleNumber;
}

void KDSparseUsedConstructedRule::Write(ostream& ost) const
{
	WriteLineReport(ost);
}

void KDSparseUsedConstructedRule::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Class\tAttribute\tUse nb\tRule\tName";
}

void KDSparseUsedConstructedRule::WriteLineReport(ostream& ost) const
{
	if (usedConstructedRule != NULL)
		ost << usedConstructedRule->GetClassName();
	ost << "\t";
	if (usedAttribute != NULL)
		ost << usedAttribute->GetName();
	ost << "\t";
	ost << nUsingRuleNumber << "\t";
	if (usedConstructedRule != NULL)
		ost << *usedConstructedRule;
	ost << "\t";
	if (usedConstructedRule != NULL)
		ost << usedConstructedRule->BuildAttributeName(false);
}

boolean KDSparseUsedConstructedRule::Check() const
{
	boolean bOk = true;

	// Dans le cas ou il n'y a pas de regle, il ne doit pas y avoir d'attribut
	if (usedConstructedRule == NULL)
		bOk = (usedAttribute == NULL);
	else
	{
		// Dans le cas ou il y a un attribut, verification de sa regle
		if (usedAttribute != NULL)
		{
			bOk = usedAttribute->GetDerivationRule() != NULL;
			if (bOk)
				bOk =
				    usedConstructedRule->CompareWithDerivationRule(usedAttribute->GetDerivationRule());
		}
	}
	return bOk;
}

const ALString KDSparseUsedConstructedRule::GetClassLabel() const
{
	return "Used rule";
}

const ALString KDSparseUsedConstructedRule::GetObjectLabel() const
{
	if (usedAttribute == NULL)
		return "";
	else
		return usedAttribute->GetName();
}

int KDSparseUsedConstructedRuleCompareCostName(const void* elem1, const void* elem2)
{
	KDSparseUsedConstructedRule* usedRule1;
	KDSparseUsedConstructedRule* usedRule2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	usedRule1 = cast(KDSparseUsedConstructedRule*, *(Object**)elem1);
	usedRule2 = cast(KDSparseUsedConstructedRule*, *(Object**)elem2);
	assert(usedRule1->GetConstructedRule() != NULL);
	assert(usedRule2->GetConstructedRule() != NULL);

	// Difference entre les regles construites
	nDiff = usedRule1->GetConstructedRule()->CompareCostName(usedRule2->GetConstructedRule());
	return nDiff;
}

int KDAttributeDerivationRuleCompare(const void* elem1, const void* elem2)
{
	KWAttribute* attribute1;
	KWAttribute* attribute2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	attribute1 = cast(KWAttribute*, *(Object**)elem1);
	attribute2 = cast(KWAttribute*, *(Object**)elem2);
	assert(attribute1->GetAnyDerivationRule() != NULL);
	assert(attribute2->GetAnyDerivationRule() != NULL);
	assert(
	    attribute1->GetAnyDerivationRule()->CheckCompleteness(attribute1->GetAnyDerivationRule()->GetOwnerClass()));
	assert(
	    attribute2->GetAnyDerivationRule()->CheckCompleteness(attribute2->GetAnyDerivationRule()->GetOwnerClass()));

	// Difference entre les regles de derivation
	nDiff = attribute1->GetAnyDerivationRule()->FullCompare(attribute2->GetAnyDerivationRule());

	// Si egalite et attribut de bloc, comparaison sur les VarKey
	if (nDiff == 0 and attribute1->IsInBlock())
	{
		assert(attribute2->IsInBlock());
		assert(attribute1->GetAttributeBlock()->GetVarKeyType() ==
		       attribute2->GetAttributeBlock()->GetVarKeyType());

		// Comparaison selon le type de cle
		if (attribute1->GetAttributeBlock()->GetVarKeyType() == KWType::Continuous)
			nDiff = attribute1->GetAttributeBlock()->GetContinuousVarKey(attribute1) -
				attribute2->GetAttributeBlock()->GetContinuousVarKey(attribute2);
		else
			nDiff = attribute1->GetAttributeBlock()
				    ->GetSymbolVarKey(attribute1)
				    .CompareValue(attribute2->GetAttributeBlock()->GetSymbolVarKey(attribute2));
	}
	return nDiff;
}

int KDAttributeBlockDerivationRuleCompare(const void* elem1, const void* elem2)
{
	KWAttributeBlock* attributeBlock1;
	KWAttributeBlock* attributeBlock2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	attributeBlock1 = cast(KWAttributeBlock*, *(Object**)elem1);
	attributeBlock2 = cast(KWAttributeBlock*, *(Object**)elem2);
	assert(attributeBlock1->GetDerivationRule() != NULL);
	assert(attributeBlock2->GetDerivationRule() != NULL);
	assert(attributeBlock1->GetDerivationRule()->CheckCompleteness(
	    attributeBlock1->GetDerivationRule()->GetOwnerClass()));
	assert(attributeBlock2->GetDerivationRule()->CheckCompleteness(
	    attributeBlock2->GetDerivationRule()->GetOwnerClass()));

	// Difference entre les regles de derivation
	nDiff = attributeBlock1->GetDerivationRule()->FullCompare(attributeBlock2->GetDerivationRule());
	return nDiff;
}

//////////////////////////////////////////////////////////////////////////
// Classe KDSparseUsedConstructedBlockRule

KDSparseUsedConstructedBlockRule::KDSparseUsedConstructedBlockRule()
{
	usedConstructedBlockRule = NULL;
	usedAttributeBlock = NULL;
}

KDSparseUsedConstructedBlockRule::~KDSparseUsedConstructedBlockRule() {}

void KDSparseUsedConstructedBlockRule::SetConstructedBlockRule(const KDConstructedRule* rule)
{
	usedConstructedBlockRule = rule;
}

const KDConstructedRule* KDSparseUsedConstructedBlockRule::GetConstructedBlockRule() const
{
	return usedConstructedBlockRule;
}

void KDSparseUsedConstructedBlockRule::SetAttributeBlock(const KWAttributeBlock* attributeBlock)
{
	usedAttributeBlock = attributeBlock;
}

const KWAttributeBlock* KDSparseUsedConstructedBlockRule::GetAttributeBlock() const
{
	return usedAttributeBlock;
}

void KDSparseUsedConstructedBlockRule::Write(ostream& ost) const
{
	// Affichage de la regle et de son attribut
	if (usedConstructedBlockRule != NULL)
	{
		ost << usedConstructedBlockRule->GetClassName() << "\t";
		if (usedAttributeBlock != NULL)
			ost << usedAttributeBlock->GetName();
		ost << "\t";
		ost << *usedConstructedBlockRule << "\n";
	}
}

boolean KDSparseUsedConstructedBlockRule::Check() const
{
	boolean bOk = true;

	// Dans le cas ou il n'y a pas de regle, il ne doit pas y avoir d'attribut
	if (usedConstructedBlockRule != NULL)
	{
		assert(usedConstructedBlockRule->IsBlockRule());

		// Verification de l'attribut de bloc
		bOk = bOk and usedAttributeBlock != NULL;
		bOk = bOk and usedAttributeBlock->GetDerivationRule() != NULL;
	}
	return bOk;
}

const ALString KDSparseUsedConstructedBlockRule::GetClassLabel() const
{
	return "Used block rule";
}

const ALString KDSparseUsedConstructedBlockRule::GetObjectLabel() const
{
	ALString sLabel;

	// Libelle l'eventuel attribut du bloc
	if (GetAttributeBlock() != NULL)
		sLabel = GetAttributeBlock()->GetName();
	return sLabel;
}

int KDSparseUsedConstructedBlockRuleCompareCostName(const void* elem1, const void* elem2)
{
	KDSparseUsedConstructedBlockRule* usedBlockRule1;
	KDSparseUsedConstructedBlockRule* usedBlockRule2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux regles
	usedBlockRule1 = cast(KDSparseUsedConstructedBlockRule*, *(Object**)elem1);
	usedBlockRule2 = cast(KDSparseUsedConstructedBlockRule*, *(Object**)elem2);
	assert(usedBlockRule1->GetConstructedBlockRule() != NULL);
	assert(usedBlockRule2->GetConstructedBlockRule() != NULL);

	// Difference entre les regles construites
	nDiff =
	    usedBlockRule1->GetConstructedBlockRule()->CompareBlockCostName(usedBlockRule2->GetConstructedBlockRule());
	return nDiff;
}
