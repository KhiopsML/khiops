// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDClassCompliantRules.h"

/////////////////////////////////////////////////////////////////////
// Classe KDClassDomainCompliantRules

KDClassDomainCompliantRules::KDClassDomainCompliantRules()
{
	constructionDomain = NULL;
	kwcClass = NULL;
}

KDClassDomainCompliantRules::~KDClassDomainCompliantRules()
{
	Clean();
}

void KDClassDomainCompliantRules::SetConstructionDomain(KDConstructionDomain* constructionDomainParam)
{
	constructionDomain = constructionDomainParam;
}

KDConstructionDomain* KDClassDomainCompliantRules::GetConstructionDomain() const
{
	return constructionDomain;
}

void KDClassDomainCompliantRules::SetClass(KWClass* kwcMainClass)
{
	kwcClass = kwcMainClass;
}

KWClass* KDClassDomainCompliantRules::GetClass() const
{
	return kwcClass;
}

void KDClassDomainCompliantRules::AddClassCompliantRules(KDClassCompliantRules* classCompliantRules)
{
	require(classCompliantRules != NULL);
	require(LookupClassCompliantRules(classCompliantRules->GetClassName()) == NULL);

	oaAllClassesCompliantRules.Add(classCompliantRules);
	odAllClassesCompliantRules.SetAt(classCompliantRules->GetClassName(), classCompliantRules);
	ensure(oaAllClassesCompliantRules.GetSize() == odAllClassesCompliantRules.GetCount());
	ensure(LookupClassCompliantRules(classCompliantRules->GetClassName()) == classCompliantRules);
}

void KDClassDomainCompliantRules::Clean()
{
	KDClassCompliantRules* classCompliantRule;
	int i;

	// Nettoyage de base
	constructionDomain = NULL;
	kwcClass = NULL;

	// Nettoyage prealable des regles collectes
	for (i = 0; i < oaAllClassesCompliantRules.GetSize(); i++)
	{
		classCompliantRule = cast(KDClassCompliantRules*, oaAllClassesCompliantRules.GetAt(i));
		classCompliantRule->CleanCollectedConstructedAttributesAndBlocks();
	}

	// Destruction par classe du domaine
	odAllClassesCompliantRules.RemoveAll();
	oaAllClassesCompliantRules.DeleteAll();
}

ObjectArray* KDClassDomainCompliantRules::GetAllClassesCompliantRules()
{
	return &oaAllClassesCompliantRules;
}

KDClassCompliantRules* KDClassDomainCompliantRules::GetMainClassCompliantRules() const
{
	if (oaAllClassesCompliantRules.GetSize() == 0)
		return NULL;
	else
		return cast(KDClassCompliantRules*, oaAllClassesCompliantRules.GetAt(0));
}

KDClassCompliantRules* KDClassDomainCompliantRules::LookupClassCompliantRules(const ALString& sClassName) const
{
	return cast(KDClassCompliantRules*, odAllClassesCompliantRules.Lookup(sClassName));
}

int KDClassDomainCompliantRules::GetTotalClassCompliantRuleNumber() const
{
	int nTotalClassCompliantRuleNumber;
	int nClass;
	KDClassCompliantRules* classCompliantRules;

	// Parcours des classe pour compter les regles appliquabkes
	nTotalClassCompliantRuleNumber = 0;
	for (nClass = 0; nClass < oaAllClassesCompliantRules.GetSize(); nClass++)
	{
		classCompliantRules = cast(KDClassCompliantRules*, oaAllClassesCompliantRules.GetAt(nClass));
		nTotalClassCompliantRuleNumber += classCompliantRules->GetCompliantConstructionRuleNumber();
	}
	return nTotalClassCompliantRuleNumber;
}

boolean KDClassDomainCompliantRules::IsSelectionRuleUsed() const
{
	int nClass;
	KDClassCompliantRules* classCompliantRules;
	int nRule;
	KDConstructionRule* constructionRule;

	// Parcours des classe pour compter les regles appliquabkes
	for (nClass = 0; nClass < oaAllClassesCompliantRules.GetSize(); nClass++)
	{
		classCompliantRules = cast(KDClassCompliantRules*, oaAllClassesCompliantRules.GetAt(nClass));

		// Parcours des regles applicables a la classe
		for (nRule = 0; nRule < classCompliantRules->GetCompliantConstructionRuleNumber(); nRule++)
		{
			constructionRule = classCompliantRules->GetCompliantConstructionRuleAt(nRule);
			if (constructionRule->IsSelectionRule())
				return true;
		}
	}
	return false;
}

int KDClassDomainCompliantRules::GetTotalInitialConstructedAttributeNumber() const
{
	int nConstructedAttributeNumber;
	int nClass;
	KDClassCompliantRules* classCompliantRules;

	// Parcours de classe pour calcul le nombre total d'attribut (potentiellement) construits
	nConstructedAttributeNumber = 0;
	for (nClass = 0; nClass < oaAllClassesCompliantRules.GetSize(); nClass++)
	{
		classCompliantRules = cast(KDClassCompliantRules*, oaAllClassesCompliantRules.GetAt(nClass));
		nConstructedAttributeNumber += classCompliantRules->GetConstructedAttributeNumber();
	}
	return nConstructedAttributeNumber;
}

void KDClassDomainCompliantRules::DisplayAllConstructedAttributes(ostream& ost) const
{
	int nClass;
	KDClassCompliantRules* classCompliantRules;

	// Parcours de classe pour affichage
	for (nClass = 0; nClass < oaAllClassesCompliantRules.GetSize(); nClass++)
	{
		classCompliantRules = cast(KDClassCompliantRules*, oaAllClassesCompliantRules.GetAt(nClass));
		classCompliantRules->DisplayConstructedAttributes(ost);
		classCompliantRules->DisplayConstructedAttributeBlocks(ost);
	}
}

void KDClassDomainCompliantRules::Write(ostream& ost) const
{
	int i;
	KDClassCompliantRules* classCompliantRules;

	ost << GetClassLabel() << " " << GetObjectLabel() << ":";
	for (i = 0; i < oaAllClassesCompliantRules.GetSize(); i++)
	{
		classCompliantRules = cast(KDClassCompliantRules*, oaAllClassesCompliantRules.GetAt(i));
		ost << "\t" << *classCompliantRules;
	}
	ost << "\n";
}

const ALString KDClassDomainCompliantRules::GetClassLabel() const
{
	return "Dictionary domain compliant rules";
}

const ALString KDClassDomainCompliantRules::GetObjectLabel() const
{
	if (kwcClass == NULL)
		return "";
	else
		return kwcClass->GetName();
}

/////////////////////////////////////////////////////////////////////
// Classe KDClassCompliantRules

KDClassCompliantRules::KDClassCompliantRules()
{
	kwcClass = NULL;
	slDerivedAttributes = new SortedList(KDAttributeDerivationRuleCompare);
	slDerivedAttributeBlocks = new SortedList(KDAttributeBlockDerivationRuleCompare);
	slSearchedUsedConstructedRules = new SortedList(KDSparseUsedConstructedRuleCompareCostName);
	slSearchedUsedConstructedBlockRules = new SortedList(KDSparseUsedConstructedBlockRuleCompareCostName);
	nLookupNumber = 0;
	nDerivedAttributeNumber = 0;
	nDerivedAttributeBlockNumber = 0;
	nDerivedAttributesUsedConstructedRulesNumber = 0;
}

KDClassCompliantRules::~KDClassCompliantRules()
{
	assert(slDerivedAttributes->GetCount() == 0);
	assert(slDerivedAttributeBlocks->GetCount() == 0);
	assert(slSearchedUsedConstructedRules->GetCount() == 0);
	assert(slSearchedUsedConstructedBlockRules->GetCount() == 0);
	assert(nLookupNumber == 0);
	assert(nDerivedAttributeNumber == 0);
	assert(nDerivedAttributesUsedConstructedRulesNumber == 0);
	delete slDerivedAttributes;
	delete slDerivedAttributeBlocks;
	delete slSearchedUsedConstructedRules;
	delete slSearchedUsedConstructedBlockRules;
	oaCompliantConstructionRules.DeleteAll();
}

const ALString KDClassCompliantRules::GetClassName() const
{
	if (kwcClass == NULL)
		return "";
	else
		return kwcClass->GetName();
}

void KDClassCompliantRules::SetClass(const KWClass* aClass)
{
	kwcClass = aClass;
}

const KWClass* KDClassCompliantRules::GetClass() const
{
	return kwcClass;
}

ObjectDictionary* KDClassCompliantRules::GetForbiddenAttributes()
{
	return &odForbiddenAttributes;
}

boolean KDClassCompliantRules::IsAttributeForbidden(const ALString& sAttributeName) const
{
	return odForbiddenAttributes.Lookup(sAttributeName) != NULL;
}

boolean KDClassCompliantRules::IsAttributeRedundant(const ALString& sAttributeName) const
{
	return odRedundantAttributes.Lookup(sAttributeName) != NULL;
}

boolean KDClassCompliantRules::InsertCompliantConstructionRule(KDConstructionRule* constructionRule)
{
	require(constructionRule != NULL);

	if (SearchCompliantConstructionRule(constructionRule->GetName()))
		return false;
	else
	{
		oaCompliantConstructionRules.Add(constructionRule);
		return true;
	}
}

int KDClassCompliantRules::GetCompliantConstructionRuleNumber() const
{
	return oaCompliantConstructionRules.GetSize();
}

KDConstructionRule* KDClassCompliantRules::GetCompliantConstructionRuleAt(int i) const
{
	require(0 <= i and i < GetCompliantConstructionRuleNumber());
	return cast(KDConstructionRule*, oaCompliantConstructionRules.GetAt(i));
}

void KDClassCompliantRules::RemoveAllCompliantConstructionRules()
{
	oaCompliantConstructionRules.SetSize(0);
}

KDConstructionRule* KDClassCompliantRules::SearchCompliantConstructionRule(const ALString& sRuleName) const
{
	KDConstructionRule* constructionRule;
	int nRule;

	// Recherche par parcours des regles de constructions applicables
	for (nRule = 0; nRule < oaCompliantConstructionRules.GetSize(); nRule++)
	{
		constructionRule = cast(KDConstructionRule*, oaCompliantConstructionRules.GetAt(nRule));
		if (constructionRule->GetName() == sRuleName)
			return constructionRule;
	}

	// On retourne NULL si non trouve
	return NULL;
}

void KDClassCompliantRules::CollectConstructedAttributesAndBlocks()
{
	const KWDRPartition refPartitionRule;
	const KWDRTablePartition refTablePartitionRule;
	KDConstructionRule* constructionRule;
	ObjectDictionary odConstructionDerivationRules;
	int nRule;

	require(GetClass() != NULL);
	require(slDerivedAttributes->GetCount() == 0);
	require(slDerivedAttributeBlocks->GetCount() == 0);
	require(odRedundantAttributes.GetCount() == 0);

	// Memorisation dans un dictionnaire des regles de construction applicables
	for (nRule = 0; nRule < oaCompliantConstructionRules.GetSize(); nRule++)
	{
		constructionRule = cast(KDConstructionRule*, oaCompliantConstructionRules.GetAt(nRule));

		// Ajout des regles predefinies de gestion des partition
		if (constructionRule->IsSelectionRule())
		{
			odConstructionDerivationRules.SetAt(refPartitionRule.GetName(),
							    cast(Object*, &refPartitionRule));
			odConstructionDerivationRules.SetAt(refTablePartitionRule.GetName(),
							    cast(Object*, &refTablePartitionRule));
		}

		// Ajout de la regle de derivation associee a la regle de construction
		odConstructionDerivationRules.SetAt(constructionRule->GetDerivationRule()->GetName(),
						    cast(Object*, constructionRule->GetDerivationRule()));

		// Ajout egalement des regle de type bloc, si elles existent
		if (constructionRule->GetPartitionStatsRule() != NULL)
			odConstructionDerivationRules.SetAt(constructionRule->GetPartitionStatsRule()->GetName(),
							    cast(Object*, constructionRule->GetDerivationRule()));
		if (constructionRule->GetValueBlockRule() != NULL)
			odConstructionDerivationRules.SetAt(constructionRule->GetValueBlockRule()->GetName(),
							    cast(Object*, constructionRule->GetDerivationRule()));
	}

	// Collecte de tous les attributs et blocs derives de la classe avec le dictionnaire des regles de derivation
	// utilisables
	CollectConstructedAttributesAndBlocksUsingDerivationRules(&odConstructionDerivationRules);
}

void KDClassCompliantRules::CollectConstructedAttributesAndBlocksUsingDerivationRules(
    const ObjectDictionary* odUsableDerivationRules)
{
	KWAttribute* attribute;
	KWAttributeBlock* currentAttributeBlock;
	SortedList slNonRedundantAttributes(KDAttributeDerivationRuleCompare);
	POSITION position;

	require(odUsableDerivationRules != NULL);
	require(GetClass() != NULL);
	require(slDerivedAttributes->GetCount() == 0);
	require(slDerivedAttributeBlocks->GetCount() == 0);
	require(odRedundantAttributes.GetCount() == 0);

	// Parcours des attributs de la classe pour en analyser les regles
	attribute = GetClass()->GetHeadAttribute();
	currentAttributeBlock = NULL;
	while (attribute != NULL)
	{
		// Analyse de l'eventuelle regle de derivation pour les attributs, hors bloc ou dans les blocs
		// (distinques par leur VarKey)
		if (attribute->GetAnyDerivationRule() != NULL)
		{
			// Ajout de l'attribut dans la liste s'il correspond a une regle de construction
			// Si deux attributs sont associes
			if (odUsableDerivationRules->Lookup(attribute->GetAnyDerivationRule()->GetName()) != NULL)
			{
				// Enregistrement de l'attribut si la regle n'a pas ete enregistree
				position = slDerivedAttributes->Find(attribute);
				if (position == NULL)
					slDerivedAttributes->Add(attribute);
			}
		}

		// Detection du changement de bloc
		if (attribute->GetAttributeBlock() != currentAttributeBlock)
		{
			currentAttributeBlock = attribute->GetAttributeBlock();

			// Analyse de la regle de derivation du bloc
			if (currentAttributeBlock != NULL and currentAttributeBlock->GetDerivationRule() != NULL)
			{
				// Ajout du bloc d'attribut dans la liste s'il correspond a une regle de construction
				// Si deux attributs sont associes
				if (odUsableDerivationRules->Lookup(
					currentAttributeBlock->GetDerivationRule()->GetName()) != NULL)
				{
					// Enregistrement de l'attribut si la regle n'a pas ete enregistree
					position = slDerivedAttributeBlocks->Find(currentAttributeBlock);
					if (position == NULL)
						slDerivedAttributeBlocks->Add(currentAttributeBlock);
				}
			}
		}

		// Detection des attributs utilise calcules redondants
		if (attribute->GetUsed() and attribute->GetAnyDerivationRule() != NULL)
		{
			// Enregistrement de l'attribut parmi les attributs non redondant si la regle n'a pas ete
			// enregistree
			position = slNonRedundantAttributes.Find(attribute);
			if (position == NULL)
				slNonRedundantAttributes.Add(attribute);
			// Sinon, memorisation parmi les attributs redondants
			else
				odRedundantAttributes.SetAt(attribute->GetName(), attribute);
		}

		// Attribut suivant
		GetClass()->GetNextAttribute(attribute);
	}
	nDerivedAttributeNumber = slDerivedAttributes->GetCount();
	nDerivedAttributeBlockNumber = slDerivedAttributeBlocks->GetCount();
}

int KDClassCompliantRules::GetConstructedAttributeNumber() const
{
	assert(nDerivedAttributeNumber == slDerivedAttributes->GetCount());
	return nDerivedAttributeNumber;
}

int KDClassCompliantRules::GetConstructedAttributeBlockNumber() const
{
	assert(nDerivedAttributeBlockNumber == slDerivedAttributeBlocks->GetCount());
	return nDerivedAttributeBlockNumber;
}

void KDClassCompliantRules::DisplayConstructedAttributes(ostream& ost) const
{
	POSITION position;
	KWAttribute* attribute;
	int nRule;
	ALString sName;

	// Parcours des regles de la liste
	ost << GetClass()->GetName() << "\tInitial constructed attribute derivation rules" << endl;
	nRule = 0;
	position = slDerivedAttributes->GetHeadPosition();
	while (position != NULL)
	{
		attribute = cast(KWAttribute*, slDerivedAttributes->GetNext(position));
		nRule++;

		// Header
		if (nRule == 1)
			ost << "Class\tAttribute\tVarKey\tRule\n";

		// Nom de l'attribut et de la regle
		cout << attribute->GetParentClass()->GetName() << "\t";
		cout << attribute->GetName() << "\t";
		if (attribute->GetAttributeBlock() != NULL)
			cout << attribute->GetAttributeBlock()->GetStringVarKey(attribute);
		cout << "\t";
		attribute->GetAnyDerivationRule()->WriteUsedRule(cout);
		ost << "\n";
	}
	ost << endl;
}

void KDClassCompliantRules::DisplayConstructedAttributeBlocks(ostream& ost) const
{
	POSITION position;
	KWAttributeBlock* attributeBlock;
	int nRule;
	ALString sName;

	// Parcours des regles de la liste
	ost << GetClass()->GetName() << "\tInitial constructed attribute block derivation rules" << endl;
	nRule = 0;
	position = slDerivedAttributeBlocks->GetHeadPosition();
	while (position != NULL)
	{
		attributeBlock = cast(KWAttributeBlock*, slDerivedAttributeBlocks->GetNext(position));
		nRule++;

		// Header
		if (nRule == 1)
			ost << "Class\tAttribute block\tRule\n";

		// Nom de l'attribut et de la regle
		cout << attributeBlock->GetParentClass()->GetName() << "\t";
		cout << attributeBlock->GetName() << "\t";
		attributeBlock->GetDerivationRule()->WriteUsedRule(cout);
		ost << "\n";
	}
	ost << endl;
}

const KWAttribute*
KDClassCompliantRules::LookupConstructedAttribute(const KDConstructedRule* searchedConstructedRule) const
{
	const KWAttribute* usedAttribute;
	KWAttribute* headAttribute;
	KWClassDomain* classDomain;
	KWClass* kwcRuleClass;
	KDSparseUsedConstructedRule* usedConstructedRule;
	KWDerivationRule* searchedDerivationRule;

	require(searchedConstructedRule != NULL);
	require(slDerivedAttributes->GetCount() == nDerivedAttributeNumber);

	// Mise a jour du nombre de recherches
	nLookupNumber++;

	// On renvoie NULL s'il n'y a pas d'attribut derive
	if (nDerivedAttributeNumber == 0)
		usedAttribute = NULL;
	// Sinon
	else
	{
		// On recherche parmi les regles construites pour lesquelle on a deja fait une recherche
		usedConstructedRule = LookupUsedConstructedRule(searchedConstructedRule);

		// Si on a deja fait la recherche, on a acces directement a l'attribut derive correspondant
		if (usedConstructedRule != NULL)
			usedAttribute = usedConstructedRule->GetAttribute();
		// Si on avait deja trouve toutes les regles construites correspondant aux attributs derives,
		// c'est qu'il n'y a pas d'attribut derive correspondant
		else if (nDerivedAttributeNumber == nDerivedAttributesUsedConstructedRulesNumber)
		{
			assert(slSearchedUsedConstructedRules->GetCount() ==
			       nDerivedAttributesUsedConstructedRulesNumber);
			usedAttribute = NULL;
		}
		// Sinon, construction d'une regle de derivation a partir de la regle recherchee, pour
		// chercher dans la liste des attributs derives
		else
		{
			// Recherche du domaine a partir de la premiere regle de la liste
			assert(slDerivedAttributes->GetCount() > 0);
			headAttribute = cast(KWAttribute*, slDerivedAttributes->GetHead());
			classDomain = headAttribute->GetParentClass()->GetDomain();

			// Construction d'une regle de derivation temporaire
			searchedDerivationRule = searchedConstructedRule->BuildDerivationRule();

			// Completion de ses infos par rapport a sa classe
			kwcRuleClass = classDomain->LookupClass(searchedConstructedRule->GetClassName());
			searchedDerivationRule->CompleteTypeInfo(kwcRuleClass);
			assert(searchedDerivationRule->CheckCompleteness(kwcRuleClass));

			// Recherche de l'attribut derive correspondant
			usedAttribute = LookupDerivedAttribute(searchedDerivationRule);

			// Destruction de la regle de derivation ayant servie a la recherche, car desormais inutile
			delete searchedDerivationRule;

			// On memorise que l'on a deja fait la recherche
			usedConstructedRule = new KDSparseUsedConstructedRule;
			usedConstructedRule->SetConstructedRule(searchedConstructedRule->Clone());
			usedConstructedRule->SetAttribute(cast(KWAttribute*, usedAttribute));
			slSearchedUsedConstructedRules->Add(usedConstructedRule);

			// Si on a identifie un nouvel attribut derive, on met a jour son compte
			if (usedAttribute != NULL)
			{
				nDerivedAttributesUsedConstructedRulesNumber++;

				// Si on a identifie tous les attributs derives, on peut nettoyer la liste des
				// des regles construites utilisees associees a NULL
				if (nDerivedAttributesUsedConstructedRulesNumber == nDerivedAttributeNumber)
				{
					assert(slSearchedUsedConstructedRules->GetCount() >=
					       slDerivedAttributes->GetCount());

					// Destruction uniquement si necessaire
					if (slSearchedUsedConstructedRules->GetCount() > nDerivedAttributeNumber)
						DeleteNullUsedConstructedRules();
					assert(slSearchedUsedConstructedRules->GetCount() ==
					       slDerivedAttributes->GetCount());
				}
			}
		}
	}
	return usedAttribute;
}

void KDClassCompliantRules::CleanCollectedConstructedAttributesAndBlocks()
{
	POSITION position;
	KDSparseUsedConstructedRule* usedConstructedRule;

	// Remise a zero du nombre de recherches
	nLookupNumber = 0;

	// Dereferencement des attributs derives
	slDerivedAttributes->RemoveAll();
	nDerivedAttributeNumber = 0;

	// Dereferencement des blocs d'attributs derives
	slDerivedAttributeBlocks->RemoveAll();
	nDerivedAttributeBlockNumber = 0;

	// Nettoyage des attributs redondants
	odRedundantAttributes.RemoveAll();

	// Destruction des regles construites referencees par les regles utilisees de la liste
	position = slSearchedUsedConstructedRules->GetHeadPosition();
	while (position != NULL)
	{
		usedConstructedRule =
		    cast(KDSparseUsedConstructedRule*, slSearchedUsedConstructedRules->GetNext(position));
		delete usedConstructedRule->GetConstructedRule();
	}

	// Destruction des regles utilisees elle-meme
	slSearchedUsedConstructedRules->DeleteAll();
	nDerivedAttributesUsedConstructedRulesNumber = 0;
}

void KDClassCompliantRules::Write(ostream& ost) const
{
	int nRule;
	KDConstructionRule* constructionRule;

	ost << GetClassLabel() << " " << GetObjectLabel() << ":";
	for (nRule = 0; nRule < oaCompliantConstructionRules.GetSize(); nRule++)
	{
		constructionRule = cast(KDConstructionRule*, oaCompliantConstructionRules.GetAt(nRule));
		if (nRule > 0)
			ost << ",";
		ost << " " << constructionRule->GetName();
	}
	ost << "\n";
}

const ALString KDClassCompliantRules::GetClassLabel() const
{
	return "Dictionary compliant rules";
}

const ALString KDClassCompliantRules::GetObjectLabel() const
{
	if (kwcClass == NULL)
		return "";
	else
		return kwcClass->GetName();
}

const KWAttribute* KDClassCompliantRules::LookupDerivedAttribute(const KWDerivationRule* searchedDerivationRule) const
{
	POSITION position;
	const KWAttribute* usedAttribute;
	static KWAttribute searchedAttribute;

	require(searchedDerivationRule != NULL);
	require(searchedDerivationRule->CheckCompleteness(searchedDerivationRule->GetOwnerClass()));

	// Recherche de la regle utilisee
	searchedAttribute.SetDerivationRule(cast(KWDerivationRule*, searchedDerivationRule));
	usedAttribute = NULL;
	position = slDerivedAttributes->Find(&searchedAttribute);
	searchedAttribute.RemoveDerivationRule();
	if (position != NULL)
		usedAttribute = cast(const KWAttribute*, slDerivedAttributes->GetAt(position));
	return usedAttribute;
}

const KWAttributeBlock*
KDClassCompliantRules::LookupDerivedAttributeBlock(const KWDerivationRule* searchedDerivationRule) const
{
	POSITION position;
	const KWAttributeBlock* usedAttributeBlock;
	static KWAttributeBlock searchedAttributeBlock;

	require(searchedDerivationRule != NULL);
	require(searchedDerivationRule->CheckCompleteness(searchedDerivationRule->GetOwnerClass()));

	// Recherche de la regle utilisee
	searchedAttributeBlock.SetDerivationRule(cast(KWDerivationRule*, searchedDerivationRule));
	usedAttributeBlock = NULL;
	position = slDerivedAttributeBlocks->Find(&searchedAttributeBlock);
	searchedAttributeBlock.RemoveDerivationRule();
	if (position != NULL)
		usedAttributeBlock = cast(const KWAttributeBlock*, slDerivedAttributeBlocks->GetAt(position));
	return usedAttributeBlock;
}

void KDClassCompliantRules::RegisterDerivedAttribute(const KWAttribute* attribute)
{
	require(attribute != NULL);
	require(attribute->GetParentClass() == GetClass());
	require(attribute->GetDerivationRule() != NULL);
	require(LookupDerivedAttribute(attribute->GetDerivationRule()) == NULL);
	slDerivedAttributes->Add(cast(KWAttribute*, attribute));
}

void KDClassCompliantRules::RegisterDerivedAttributeBlock(const KWAttributeBlock* attributeBlock)
{
	require(attributeBlock != NULL);
	require(attributeBlock->GetParentClass() == GetClass());
	require(attributeBlock->GetDerivationRule() != NULL);
	require(LookupDerivedAttribute(attributeBlock->GetDerivationRule()) == NULL);
	slDerivedAttributeBlocks->Add(cast(KWAttributeBlock*, attributeBlock));
}

KDSparseUsedConstructedRule*
KDClassCompliantRules::LookupUsedConstructedRule(const KDConstructedRule* searchedRule) const
{
	POSITION position;
	KDSparseUsedConstructedRule* usedConstructedRule;
	static KDSparseUsedConstructedRule searchedUsedConstructedRule;

	require(searchedRule != NULL);

	// Recherche de la regle utilisee
	searchedUsedConstructedRule.SetConstructedRule(searchedRule);
	usedConstructedRule = NULL;
	position = slSearchedUsedConstructedRules->Find(&searchedUsedConstructedRule);
	if (position != NULL)
		usedConstructedRule =
		    cast(KDSparseUsedConstructedRule*, slSearchedUsedConstructedRules->GetAt(position));
	return usedConstructedRule;
}

KDSparseUsedConstructedBlockRule*
KDClassCompliantRules::LookupUsedConstructedBlockRule(const KDConstructedRule* searchedBlockRule) const
{
	POSITION position;
	KDSparseUsedConstructedBlockRule* usedConstructedBlockRule;
	static KDSparseUsedConstructedBlockRule searchedUsedConstructedBlockRule;

	require(searchedBlockRule != NULL);

	// Recherche de la regle utilisee
	searchedUsedConstructedBlockRule.SetConstructedBlockRule(searchedBlockRule);
	usedConstructedBlockRule = NULL;
	position = slSearchedUsedConstructedBlockRules->Find(&searchedUsedConstructedBlockRule);
	if (position != NULL)
		usedConstructedBlockRule =
		    cast(KDSparseUsedConstructedBlockRule*, slSearchedUsedConstructedBlockRules->GetAt(position));
	return usedConstructedBlockRule;
}

void KDClassCompliantRules::DeleteNullUsedConstructedRules() const
{
	POSITION position;
	POSITION positionToDelete;
	KDSparseUsedConstructedRule* usedConstructedRule;

	// Destruction des regles construites referencees par les regles utilisees de la liste
	position = slSearchedUsedConstructedRules->GetHeadPosition();
	while (position != NULL)
	{
		positionToDelete = position;
		usedConstructedRule =
		    cast(KDSparseUsedConstructedRule*, slSearchedUsedConstructedRules->GetNext(position));

		// Destruction de la regle construite utilisee si elle est associe a NULL
		if (usedConstructedRule->GetAttribute() == NULL)
		{
			// Supression de l'element precedent de la liste
			// On le fait avant de detruire l'objet, car la liste utilise encore des assertions basee sur
			// ses objets
			slSearchedUsedConstructedRules->RemoveAt(positionToDelete);

			// Destruction de la regle construite et de la regle construite utilisee
			delete usedConstructedRule->GetConstructedRule();
			delete usedConstructedRule;
		}
	}
}
