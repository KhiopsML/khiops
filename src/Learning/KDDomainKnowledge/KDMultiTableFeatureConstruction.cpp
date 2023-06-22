// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDMultiTableFeatureConstruction.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KDMultiTableFeatureConstruction

KDMultiTableFeatureConstruction::KDMultiTableFeatureConstruction()
{
	nMaxRuleNumber = 1000000;
	nMaxRuleDepth = 100;
	dMaxRuleCost = 1000;
	bIsSelectionRuleForbidden = false;
	kwcConstructedClass = NULL;
	kwcdConstructedDomain = NULL;
	selectionOperandAnalyser = NULL;
	classDomainCompliantRules = new KDClassDomainCompliantRules;
}

KDMultiTableFeatureConstruction::~KDMultiTableFeatureConstruction()
{
	// Doit etre detruit en premier, car utilise des KDConstructionRule pour piloter sa propre destruction
	if (selectionOperandAnalyser != NULL)
		delete selectionOperandAnalyser;

	// Destruction de la classe construite, avec les contraintes par classe
	DeleteConstructedClass();

	// Destruction de l'objet de gestion des contraintes par classe
	delete classDomainCompliantRules;
}

boolean KDMultiTableFeatureConstruction::ComputeStats()
{
	int nLastRandomSeed;
	int nConstructedVariableNumber;
	ALString sTmp;

	require(Check());
	require(GetLearningSpec()->GetMultiTableConstruction());
	require(constructionDomain != NULL);
	require(DataPathGetLength() == 0);
	require(classDomainCompliantRules->GetAllClassesCompliantRules()->GetSize() == 0);

	// Trace memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " ConstructClass Begin");

	// On fixe la graine aleatoire pour ameliorer la reproductivite des tests
	nLastRandomSeed = GetRandomSeed();
	SetRandomSeed(1);

	// Demarrage du timer
	timerConstruction.Reset();
	timerConstruction.Start();

	// Nettoyage des specifications de construction
	DeleteConstructedClass();

	// Precalcul des regles applicables par classe et collecte des regles de derivation utilises
	ComputeAllClassesCompliantRules(GetClass(), classDomainCompliantRules);
	bIsStatsComputed = true;

	// Cas ou il n'y aucune possibilite de construction de variable
	if (classDomainCompliantRules->GetTotalClassCompliantRuleNumber() == 0)
	{
		// Pas de tentative de construction
		kwcdConstructedDomain = NULL;
		kwcConstructedClass = NULL;

		// Message s'il y avait des possibilites basiques de de construction de variable
		if (IsConstructionBasicallyPossible(GetClass()))
		{
			AddSimpleMessage(sTmp + "Variable construction time: " + SecondsToString(0) +
					 " (no constructed variables)");
		}
	}
	// Cas ou tente de construire des variables
	else
	{
		// Calcul de la classe en sortie
		bIsStatsComputed = ConstructClass(kwcConstructedClass);
		kwcdConstructedDomain = NULL;
		if (kwcConstructedClass != NULL)
			kwcdConstructedDomain = kwcConstructedClass->GetDomain();

		// Arret du timer
		timerConstruction.Stop();

		// Messsage de fin
		if (TaskProgression::IsInterruptionRequested())
		{
			bIsStatsComputed = false;
			AddSimpleMessage(sTmp + "Variable construction interrupted after " +
					 SecondsToString(timerConstruction.GetElapsedTime()));
		}
		else if (bIsStatsComputed)
		{
			if (kwcConstructedClass != NULL)
			{
				nConstructedVariableNumber =
				    kwcConstructedClass->ComputeInitialAttributeNumber(GetTargetAttributeName() != "") -
				    GetLearningSpec()->GetInitialAttributeNumber();
				assert(nConstructedVariableNumber >= 0);
				AddSimpleMessage(sTmp + "Variable construction time: " +
						 SecondsToString(timerConstruction.GetElapsedTime()) + " (" +
						 IntToString(nConstructedVariableNumber) + " constructed variables)");
			}
			else
				AddSimpleMessage(sTmp + "Variable construction time: " +
						 SecondsToString(timerConstruction.GetElapsedTime()) +
						 " (no constructed variables)");
		}
	}

	// Nettoyage
	SetRandomSeed(nLastRandomSeed);
	if (not bIsStatsComputed or TaskProgression::IsInterruptionRequested())
	{
		DeleteConstructedClass();
		bIsStatsComputed = false;
	}
	classDomainCompliantRules->Clean();
	DataPathInit();

	// Trace memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " ConstructClass End");
	ensure(kwcConstructedClass == NULL or AreAttributeCostsInitialized(kwcConstructedClass));
	return bIsStatsComputed;
}

void KDMultiTableFeatureConstruction::RemoveConstructedClass()
{
	kwcConstructedClass = NULL;
	kwcdConstructedDomain = NULL;
}

void KDMultiTableFeatureConstruction::DeleteConstructedClass()
{
	if (kwcdConstructedDomain != NULL)
	{
		// La classe de prediction doit etre dans son propre domaine, non enregistre dans l'ensemble des
		// domaines
		assert(KWClassDomain::GetCurrentDomain() != kwcdConstructedDomain);
		assert(KWClassDomain::LookupDomain(kwcdConstructedDomain->GetName()) != kwcdConstructedDomain);

		// Destruction du domaine et de ses classes
		delete kwcdConstructedDomain;
	}
	RemoveConstructedClass();
}

boolean KDMultiTableFeatureConstruction::IsClassConstructed() const
{
	require(IsStatsComputed());
	return kwcConstructedClass != NULL;
}

KWClass* KDMultiTableFeatureConstruction::GetConstructedClass()
{
	// Verification a minima que la preparation a ete effectue
	require(IsStatsComputed());
	require(IsClassConstructed());
	require(kwcdConstructedDomain != NULL);
	require(kwcConstructedClass->GetDomain() == kwcdConstructedDomain);
	return kwcConstructedClass;
}

KWClassDomain* KDMultiTableFeatureConstruction::GetConstructedDomain()
{
	// Verification a minima que la preparation a ete effectue
	require(IsStatsComputed());
	require(IsClassConstructed());
	require(kwcdConstructedDomain != NULL);
	require(kwcConstructedClass->GetDomain() == kwcdConstructedDomain);
	return kwcdConstructedDomain;
}

double KDMultiTableFeatureConstruction::GetConstructionTime() const
{
	require(IsStatsComputed());

	return timerConstruction.GetElapsedTime();
}

int KDMultiTableFeatureConstruction::RuleComputeUsedConstructionRuleNumber(
    const KWDerivationRule* constructedRule) const
{
	int nConstructionRuleNumber;
	int nOperand;
	KWDerivationRuleOperand* operand;

	require(constructedRule != NULL);

	// On compte 1 si la regle est une regle de construction
	if (GetConstructionDomain()->LookupConstructionRule(constructedRule->GetName()) != NULL)
		nConstructionRuleNumber = 1;
	else
		nConstructionRuleNumber = 0;

	// On parcours les operandes pour propager le calcul
	for (nOperand = 0; nOperand < constructedRule->GetOperandNumber(); nOperand++)
	{
		operand = constructedRule->GetOperandAt(nOperand);
		if (operand->GetDerivationRule() != NULL)
			nConstructionRuleNumber += RuleComputeUsedConstructionRuleNumber(operand->GetDerivationRule());
	}
	return nConstructionRuleNumber;
}

int KDMultiTableFeatureConstruction::RuleComputeUsedOperandNumber(const KWDerivationRule* constructedRule) const
{
	int nOperandNumber;
	int nOperand;
	KWDerivationRuleOperand* operand;

	require(constructedRule != NULL);

	// On initialise avec le nombre d'operandes
	nOperandNumber = constructedRule->GetOperandNumber();

	// On parcours les operandes pour propager le calcul
	for (nOperand = 0; nOperand < constructedRule->GetOperandNumber(); nOperand++)
	{
		operand = constructedRule->GetOperandAt(nOperand);
		if (operand->GetDerivationRule() != NULL)
			nOperandNumber += RuleComputeUsedOperandNumber(operand->GetDerivationRule());
	}
	return nOperandNumber;
}

int KDMultiTableFeatureConstruction::RuleComputeOperandTreeDepth(const KWDerivationRule* constructedRule) const
{
	int nMaxTreeDepth;
	int nTreeDepth;
	int nOperand;
	KWDerivationRuleOperand* operand;

	require(constructedRule != NULL);

	// On parcours les operandes pour rechercher la profondeur maximale
	nMaxTreeDepth = 0;
	for (nOperand = 0; nOperand < constructedRule->GetOperandNumber(); nOperand++)
	{
		operand = constructedRule->GetOperandAt(nOperand);

		// Calcul de la profondeur pour cet operande
		nTreeDepth = 1;
		if (operand->GetDerivationRule() != NULL)
			nTreeDepth += RuleComputeOperandTreeDepth(operand->GetDerivationRule());

		// Memorisation de la profondeur max
		if (nTreeDepth > nMaxTreeDepth)
			nMaxTreeDepth = nTreeDepth;
	}
	return nMaxTreeDepth;
}

int KDMultiTableFeatureConstruction::GetMaxRuleNumber() const
{
	return nMaxRuleNumber;
}

void KDMultiTableFeatureConstruction::SetMaxRuleNumber(int nValue)
{
	require(nValue >= 0);
	nMaxRuleNumber = nValue;
}

int KDMultiTableFeatureConstruction::GetMaxRuleDepth() const
{
	return nMaxRuleDepth;
}

void KDMultiTableFeatureConstruction::SetMaxRuleDepth(int nValue)
{
	require(nValue >= 0);
	nMaxRuleDepth = nValue;
}

double KDMultiTableFeatureConstruction::GetMaxRuleCost() const
{
	return dMaxRuleCost;
}

void KDMultiTableFeatureConstruction::SetMaxRuleCost(double dValue)
{
	require(dValue > 0);
	dMaxRuleCost = dValue;
}

void KDMultiTableFeatureConstruction::Write(ostream& ost) const
{
	int i;
	KDConstructionRule* constructionRule;

	// Entete
	ost << GetClassLabel() << " " << GetObjectLabel() << endl;

	// Regles de construction
	ost << "Construction rules:"
	    << "\n";
	for (i = 0; i < GetConstructionDomain()->GetConstructionRuleNumber(); i++)
	{
		constructionRule = GetConstructionDomain()->GetConstructionRuleAt(i);
		if (constructionRule->GetUsed())
			ost << "\t" << constructionRule->GetFamilyName() << " " << constructionRule->GetName() << "\n";
	}

	// Contraintes par classe
	cout << classDomainCompliantRules << "\n";
}

const ALString KDMultiTableFeatureConstruction::GetClassLabel() const
{
	return "Multi-table feature construction";
}

const ALString KDMultiTableFeatureConstruction::GetObjectLabel() const
{
	return GetClass()->GetName();
}

boolean KDMultiTableFeatureConstruction::ConstructClass(KWClass*& constructedClass)
{
	boolean bOk = true;
	boolean bDisplay = false;
	boolean bConstructOnlyForProfiling = false; // Pour les test de profiling
	KDClassBuilder classBuilder;
	ALString sVariableName;
	ObjectArray oaAllConstructedRules;
	int nInitialMaxRuleNumber;
	int nRequiredMaxRuleNumber;
	int nExistingDerivedAttributeNumber;
	boolean bSelectionBasedRulesFiltered;

	require(Check());

	// Affichage du nom de la methode
	if (bDisplay)
		cout << "KDMultiTableFeatureConstruction::ConstructClass\t" << GetMaxRuleNumber()
		     << "\t(seed=" << GetRandomSeed() << ")\n";

	// Creation de l'analyseur des operandes de construction
	assert(selectionOperandAnalyser == NULL);
	selectionOperandAnalyser = new KDSelectionOperandAnalyser;
	selectionOperandAnalyser->SetLearningSpec(GetLearningSpec());
	selectionOperandAnalyser->SetMultiTableFeatureConstruction(this);

	// Memorisation du nombre max de regles a construire initial
	nInitialMaxRuleNumber = GetMaxRuleNumber();

	// Prise en compte du nombre d'attribut derives existants,  potentiellement en collision
	// avec les nouvaux attributs qui seront construits
	nExistingDerivedAttributeNumber =
	    classDomainCompliantRules->GetMainClassCompliantRules()->GetConstructedAttributeNumber();
	nRequiredMaxRuleNumber = nInitialMaxRuleNumber + nExistingDerivedAttributeNumber;

	// Dans le cas de la regle de construction, on augmente le nombre de regles a construire:
	//  . pour identifier davantage de regle de selection (qui peuvent etre filtree s'il n'y a pas assez
	//    de valeurs dans les tables secondaires
	//  . pour se premunir des eventuelles regles de selection des tables secondaire, redondantes
	//    avec des attributs derives initiaux
	if (classDomainCompliantRules->IsSelectionRuleUsed())
		nRequiredMaxRuleNumber *= 2;

	// On modifie temporairement le nombre de regles demandees au double, pour ne pas forcer brutalement
	// l'arret de la construction des regles au milieu d'une famille de regles particulieres
	SetMaxRuleNumber(2 * nRequiredMaxRuleNumber);

	// Construction de toutes les regles de derivation applicables sur la classe principale
	// Premiere passe pour determiner les operandes de selection, sans exploiter les valeurs de la base
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() +
				   " BuildMainClassRequestedConstructedRules 1 Begin");
	BuildMainClassRequestedConstructedRules(&oaAllConstructedRules, nRequiredMaxRuleNumber);
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() +
				   " BuildMainClassRequestedConstructedRules 1 End");

	// Filtrage des regles construites inutiles, sauf dans le cas avec des attributs existants potentiellement
	// construits Si aucune regle construite utilisant la regle de selection n'est neccssaire pour attendre le
	// nombre demande, il n'est pas utile de "provisionner" des regles suplementaires
	bSelectionBasedRulesFiltered = false;
	if (nExistingDerivedAttributeNumber == 0)
		bSelectionBasedRulesFiltered =
		    classDomainCompliantRules->IsSelectionRuleUsed() and
		    FilterUselessSelectionBasedRules(&oaAllConstructedRules, nInitialMaxRuleNumber);

	// Nettoyage des spec du l'analyseur d'operande de selection si on les a toutes supprimees
	if (bSelectionBasedRulesFiltered)
		selectionOperandAnalyser->CleanAll();

	// Affichage des regles en premiere passe
	if (bDisplay)
	{
		cout << "\nFirst pass" << endl;
		DisplayConstructedRuleArray(&oaAllConstructedRules, cout);
	}

	// Cas de l'utilisation d'une regle de selection, si necessaire
	if (classDomainCompliantRules->IsSelectionRuleUsed() and oaAllConstructedRules.GetSize() > 0 and
	    not bSelectionBasedRulesFiltered and not bConstructOnlyForProfiling)
	{
		// Analyse des regles de derivation
		selectionOperandAnalyser->ComputeStats(&oaAllConstructedRules);

		// Construction de toutes les regles de derivation applicables sur la classe principale
		// Deuxieme passe pour construire les operandes de selection en exploitant les valeurs de la base
		oaAllConstructedRules.DeleteAll();
		bOk = selectionOperandAnalyser->IsStatsComputed();
		if (bOk)
		{
			MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() +
						   " BuildMainClassRequestedConstructedRules 2 Begin");
			BuildMainClassRequestedConstructedRules(&oaAllConstructedRules, nRequiredMaxRuleNumber);
			MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() +
						   " BuildMainClassRequestedConstructedRules 2 End");
		}
		assert(selectionOperandAnalyser->IsStatsComputed() or oaAllConstructedRules.GetSize() == 0);

		// Affichage des regles en premiere passe
		if (bDisplay)
		{
			cout << "\nSecond pass" << endl;
			DisplayConstructedRuleArray(&oaAllConstructedRules, cout);
		}
	}
	assert(bOk or oaAllConstructedRules.GetSize() == 0);

	// Affichage des operandes de selection
	if (bDisplay)
	{
		cout << "\nPartition operands" << endl;
		cout << *selectionOperandAnalyser << endl;
	}

	// Si on ne finalise pas la construction des variables,
	// on detruit les regle construites pour rester dans un cas coherent
	if (bConstructOnlyForProfiling)
	{
		cout << "Rules\tRules mem\tMem with\tMem without\n";
		cout << nRequiredMaxRuleNumber << "\t" << oaAllConstructedRules.GetOverallUsedMemory() << "\t"
		     << MemGetHeapMemory() << "\t";
		oaAllConstructedRules.DeleteAll();
		cout << MemGetHeapMemory() << endl;
	}

	// On restitue la contrainte sur le nombre de regles a fabriquer
	SetMaxRuleNumber(nInitialMaxRuleNumber);

	// On continue si possible et si necessaire
	constructedClass = NULL;
	if (bOk and oaAllConstructedRules.GetSize() > 0)
	{
		// Parametrage du service de construction de classe
		classBuilder.SetMultiTableFeatureConstruction(this);

		// Construction de la classe
		MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() +
					   " BuildClassFromConstructedRules Begin");
		constructedClass = classBuilder.BuildClassFromConstructedRules(GetClass(), &oaAllConstructedRules,
									       selectionOperandAnalyser);
		MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() +
					   " BuildClassFromConstructedRules End");

		// Destruction des regles construites
		oaAllConstructedRules.DeleteAll();
	}

	// Nettoyage de l'analyse des operandes de construction
	selectionOperandAnalyser->CleanAll();
	delete selectionOperandAnalyser;
	selectionOperandAnalyser = NULL;
	return bOk;
}

void KDMultiTableFeatureConstruction::DisplayConstructedRuleArray(const ObjectArray* oaConstructedRules,
								  ostream& ost) const
{
	int nRule;
	KDConstructedRule* rule;

	require(oaConstructedRules != NULL);

	// Entete
	ost << "Name\tCost\tSub rules\tOperands\tDepth\tFormula\n";

	// Affichage des regles
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
	{
		rule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));
		ost << "Feature" << nRule + 1 << "\t";
		cout << rule->GetCost() << "\t";
		cout << *rule;
		cout << "\n";
	}
}

boolean KDMultiTableFeatureConstruction::FilterUselessSelectionBasedRules(ObjectArray* oaConstructedRules,
									  int nMaxRules)
{
	boolean bFilter;
	int nRule;
	KDConstructedRule* rule;

	require(oaConstructedRules != NULL);
	require(nMaxRules > 0);

	// Cas ou il n'y a pas assez de regle
	if (oaConstructedRules->GetSize() <= nMaxRules)
		bFilter = false;
	// Analyse des regles sinon
	else
	{
		// On determine si une regle de selection est utilisee parmi les plus probables
		bFilter = true;
		for (nRule = 0; nRule < nMaxRules; nRule++)
		{
			rule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));
			assert(nRule == 0 or rule->GetCost() >=
						 cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule))->GetCost());
			if (rule->UsesSelectionRule())
			{
				bFilter = false;
				break;
			}
		}

		// Filtrage des regles inutiles si la regle de selection n'est pas utilisee parmi les premieres regles
		if (bFilter)
		{
			for (nRule = nMaxRules; nRule < oaConstructedRules->GetSize(); nRule++)
				delete oaConstructedRules->GetAt(nRule);
			oaConstructedRules->SetSize(nMaxRules);
		}
	}
	return bFilter;
}

void KDMultiTableFeatureConstruction::BuildMainClassRequestedConstructedRules(ObjectArray* oaAllConstructedRules,
									      int nRuleNumber) const
{
	boolean bDisplay = false;
	const int nInitialRandomSeed = 1;
	const int nMaxTrialNumber = 5;
	int nTrial;
	int nStep;
	double dRandomDrawingNumber;
	int nConstructedRuleNumber;
	int nNewRuleNumber;
	longint lInitialAvailableMemory;
	longint lAvailableMemory;
	longint lMeanRuleMemory;
	longint lNecessaryMemory;
	int i;
	ALString sTmp;

	require(oaAllConstructedRules != NULL);
	require(0 <= nRuleNumber and nRuleNumber <= GetMaxRuleNumber());

	// Affichage du nom de la methode
	if (bDisplay)
		cout << "KDMultiTableFeatureConstruction::BuildMainClassRequestedConstructedRules\t" << nRuleNumber
		     << "\t(seed=" << GetRandomSeed() << ")\n";

	// Debut de tache
	TaskProgression::BeginTask();
	if (selectionOperandAnalyser->IsStatsComputed())
		TaskProgression::DisplayMainLabel("Variable construction: using selection operands");
	else
		TaskProgression::DisplayMainLabel("Variable construction");

	// Nettoyage initial
	oaAllConstructedRules->RemoveAll();

	// Construction des regles sur la base d'un nombre de tirages equidistribues
	lInitialAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();
	dRandomDrawingNumber = nRuleNumber;
	nConstructedRuleNumber = 0;
	nNewRuleNumber = 0;
	nTrial = 0;
	nStep = 0;
	while (oaAllConstructedRules->GetSize() < nRuleNumber and nTrial <= nMaxTrialNumber)
	{
		nStep++;

		// On part de la meme graine au debut de chaque tirage d'echantillon de regles
		SetRandomSeed(nInitialRandomSeed);

		// Debut de sous tache
		TaskProgression::BeginTask();
		TaskProgression::DisplayMainLabel(sTmp + "Step " + IntToString(nStep));

		// Construction des regles
		oaAllConstructedRules->DeleteAll();
		BuildMainClassConstructedRules(oaAllConstructedRules, dRandomDrawingNumber);

		// Calcul du nombre de regles nouvelles par rapport au nombre de regles construites precedent
		nNewRuleNumber = oaAllConstructedRules->GetSize() - nConstructedRuleNumber;
		nConstructedRuleNumber = oaAllConstructedRules->GetSize();

		// Calcul de la quantite de memoire moyenne par regle
		lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();
		lMeanRuleMemory =
		    abs(lAvailableMemory - lInitialAvailableMemory) / (1 + oaAllConstructedRules->GetSize());

		// Fin de sous tache
		TaskProgression::EndTask();

		// Comptage du nombre d'essais sans ajout de nouvelles regles
		if (nNewRuleNumber > 0)
			nTrial = 0;
		else
			nTrial++;

		// Affichage
		if (bDisplay)
			cout << "\tRules\t" << dRandomDrawingNumber << "\t" << nConstructedRuleNumber << "\t"
			     << timerConstruction.GetElapsedTime() << endl;

		// Gestion de l'avancement
		if (oaAllConstructedRules->GetSize() < nRuleNumber)
			TaskProgression::DisplayProgression(100 * oaAllConstructedRules->GetSize() / nRuleNumber);
		else
			TaskProgression::DisplayProgression(100);
		TaskProgression::DisplayLabel(sTmp + IntToString(oaAllConstructedRules->GetSize()) +
					      " constructed features");
		if (TaskProgression::IsInterruptionRequested())
		{
			// On detruit toutes les regles construit si arret demande: cela inhibe les traitements suivants
			oaAllConstructedRules->DeleteAll();
			break;
		}

		// Iteration suivante, si possible
		dRandomDrawingNumber *= 2;
		if (dRandomDrawingNumber > 1e100)
			break;

		// Arret si on pense de pas avoir assez de memoire
		if (oaAllConstructedRules->GetSize() < nRuleNumber and nTrial <= nMaxTrialNumber)
		{
			// On table sur 3 fois le nombre de regles precedentes (peut-etre deux fois, mais des regles
			// plus longues...: regle sur le pouce)
			lNecessaryMemory = 3 * nConstructedRuleNumber * lMeanRuleMemory;
			if (lNecessaryMemory >= RMResourceManager::GetRemainingAvailableMemory())
			{
				AddWarning("Not enough memory to construct all requested rules" +
					   RMResourceManager::BuildMissingMemoryMessage(lNecessaryMemory));
				AddMessage(RMResourceManager::BuildMemoryLimitMessage());
				break;
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// S'il y a trop de regles, on ne garde que les regles les plus probables
	// Dans tous les cas, on tri les regles

	// Tri des regles selon le cout, apres un melange aleatoire pour les cas d'egalite
	SetRandomSeed(nInitialRandomSeed);
	oaAllConstructedRules->Shuffle();

	// On memorise l'index alatoire associe a chaque regle, pour assurer un ordre aleatoire reproductible
	// en cas d'égalite de cout des regles
	for (i = 0; i < oaAllConstructedRules->GetSize(); i++)
		cast(KDConstructedRule*, oaAllConstructedRules->GetAt(i))->SetRandomIndex(i);

	// Tri par cout, puis avec RandomIndex
	oaAllConstructedRules->SetCompareFunction(KDConstructedRuleCompareCostRandomIndex);
	oaAllConstructedRules->Sort();

	// Reinitialisation des index aleatoires
	for (i = 0; i < oaAllConstructedRules->GetSize(); i++)
		cast(KDConstructedRule*, oaAllConstructedRules->GetAt(i))->SetRandomIndex(0);

	// Affichage des regles
	if (bDisplay)
		DisplayConstructedRuleArray(oaAllConstructedRules, cout);

	// Fin de tache
	TaskProgression::EndTask();
}

void KDMultiTableFeatureConstruction::BuildMainClassConstructedRules(ObjectArray* oaAllConstructedRules,
								     double dRandomDrawingNumber) const
{
	const ALString sPriorTreeNodeName =
	    ""; // Mettre une valeur non vide ("C_" par exemple) pour avoir une trace de debugage
	KDMultinomialSampleGenerator sampleGenerator;
	KDClassCompliantRules* classCompliantRules;
	KDConstructionRule* constructionRule;
	KDConstructedRule* constructedRule;
	int nRule;
	double dRuleChoiceCost;
	double dRuleCost;
	int nMatchingRuleNumber;
	ALString sChildNodeName;
	DoubleVector dvConstructionRuleRandomDrawingNumbers;
	double dChildRandomDrawingNumber;
	ObjectArray oaSimpleConstructionRules;
	DoubleVector dvSimpleConstructionRuleProbs;
	ObjectArray oaConstructedRules;
	int i;

	require(oaAllConstructedRules != NULL);
	require(dRandomDrawingNumber >= 0);

	// Nettoyage initial
	oaAllConstructedRules->RemoveAll();

	// Recherche des regles applicables pour la classe principale
	classCompliantRules = GetClassDomainCompliantRules()->GetMainClassCompliantRules();

	// Recherche des regles applicables
	// Ce sont celle renvoyant un type simple
	for (nRule = 0; nRule < classCompliantRules->GetCompliantConstructionRuleNumber(); nRule++)
	{
		constructionRule = classCompliantRules->GetCompliantConstructionRuleAt(nRule);

		// Ajout si renvoie d'un type simple
		if (KWType::IsSimple(constructionRule->GetDerivationRule()->GetType()))
			oaSimpleConstructionRules.Add(constructionRule);
	}
	nMatchingRuleNumber = oaSimpleConstructionRules.GetSize();

	// Calcul du cout lie au choix de construire une regle au lieu de prendre un attribut (1/(NbAtt+1))
	dRuleChoiceCost = 0;
	if (nMatchingRuleNumber > 0)
		dRuleChoiceCost = GetLearningSpec()->GetSelectionCost();

	// Calcul des probabilites associee aux regles de construction, et des tirages par regle de construction
	if (dRandomDrawingNumber > 0 and nMatchingRuleNumber > 0)
		ComputeConstructionRuleProbs(dRandomDrawingNumber, &oaSimpleConstructionRules,
					     &dvSimpleConstructionRuleProbs, &dvConstructionRuleRandomDrawingNumbers);

	// Affichage des caracteristiques locale du noeud de l'arbre du prior en mode trace
	if (sPriorTreeNodeName != "")
	{
		cout << "BuildMainClassRules" << endl;
		cout << "\tPrior node\t" << sPriorTreeNodeName << endl;
		cout << "\tDrawing number\t" << dRandomDrawingNumber << endl;
		cout << "\tClass\t" << classCompliantRules->GetClassName() << endl;
		cout << "\tRule choice cost\t" << dRuleChoiceCost << endl;
		cout << "\tRules\t" << nMatchingRuleNumber << endl;
		if (dRandomDrawingNumber > 0)
		{
			for (nRule = 0; nRule < nMatchingRuleNumber; nRule++)
			{
				constructionRule = cast(KDConstructionRule*, oaSimpleConstructionRules.GetAt(nRule));
				cout << "\t\t" << constructionRule->GetName() << "\t"
				     << dvSimpleConstructionRuleProbs.GetAt(nRule) << "\t"
				     << dvConstructionRuleRandomDrawingNumbers.GetAt(nRule) << endl;
			}
		}
	}

	// Parcours des regles applicables a la classe
	if (dRandomDrawingNumber > 0)
	{
		if (nMatchingRuleNumber > 0 and dRuleChoiceCost <= dMaxRuleCost)
		{
			// Creation des regles pour chaque regles de construction
			for (nRule = 0; nRule < nMatchingRuleNumber; nRule++)
			{
				constructionRule = cast(KDConstructionRule*, oaSimpleConstructionRules.GetAt(nRule));

				// On ne traite la regle que s'il reste des tirages
				dChildRandomDrawingNumber = dvConstructionRuleRandomDrawingNumbers.GetAt(nRule);
				if (dChildRandomDrawingNumber == 0)
					continue;

				// On ne traite pas les regles trop cheres
				dRuleCost = dRuleChoiceCost - log(dvSimpleConstructionRuleProbs.GetAt(nRule));
				if (dRuleCost > dMaxRuleCost)
					continue;

				// Avancement
				TaskProgression::DisplayProgression(100 * (nRule + 1) / nMatchingRuleNumber);
				TaskProgression::DisplayLabel(constructionRule->GetName());
				if (TaskProgression::IsInterruptionRequested())
				{
					oaAllConstructedRules->DeleteAll();
					break;
				}

				// Construction de toutes les regles de derivations possibles
				if (sPriorTreeNodeName != "")
					sChildNodeName = sPriorTreeNodeName + "." + constructionRule->GetName() + "(";
				BuildAllConstructedRules(constructionRule, classCompliantRules, sChildNodeName, 0,
							 dRuleCost, dChildRandomDrawingNumber, &oaConstructedRules);

				// Ajout des regles venant d'etre construites
				oaAllConstructedRules->InsertObjectArrayAt(oaAllConstructedRules->GetSize(),
									   &oaConstructedRules);
				oaConstructedRules.RemoveAll();
			}
		}
	}

	// Affichage des caracteristiques des regles construites
	if (sPriorTreeNodeName != "")
	{
		cout << "All constructed rules\n";
		// Ajout du cout de choix de la regle a chaque nouvelle regle construite
		for (i = 0; i < oaAllConstructedRules->GetSize(); i++)
		{
			constructedRule = cast(KDConstructedRule*, oaAllConstructedRules->GetAt(i));
			constructedRule->WriteCostDetails(cout, "\t1");
		}
	}
}

void KDMultiTableFeatureConstruction::BuildAllConstructedRules(const KDConstructionRule* constructionRule,
							       const KDClassCompliantRules* classCompliantRules,
							       const ALString& sPriorTreeNodeName, int nDepth,
							       double dRuleCost, double dRandomDrawingNumber,
							       ObjectArray* oaAllConstructedRules) const
{
	KDConstructedRule templateConstructedRule;
	int i;
	KDConstructedRule* constructedRule;

	require(constructionRule != NULL);
	require(GetConstructionDomain()->LookupConstructionRule(constructionRule->GetName()) != NULL);
	require(classCompliantRules != NULL);
	require(LookupClassCompliantRules(classCompliantRules->GetClass()->GetName()) == classCompliantRules);
	require(dRuleCost >= 0);
	require(nDepth >= 0);
	require(dRandomDrawingNumber >= 0);
	require(oaAllConstructedRules != NULL);
	require(oaAllConstructedRules->GetSize() == 0);

	// Gestion particuliere pour empecher l'utilisation recursive des regles de selection
	// ainsi que l'utilisation de regles portant sur un ObjectArray en operande d'une regles de selection
	if (constructionRule->IsSelectionRule())
	{
		// Arret si regle actuellement interdite
		if (IsSelectionRuleForbidden())
			return;
		// Sinon, on continue en interdisant la regle
		else
			SetSelectionRuleForbidden(true);
	}

	// Initialisation du template de regle construite
	templateConstructedRule.SetConstructionRule(constructionRule);

	// Construction de toutes les regles possibles
	BuildAllConstructedRulesFromLastOperands(constructionRule, classCompliantRules, NULL, 0,
						 &templateConstructedRule, sPriorTreeNodeName, nDepth + 1, 0,
						 dRandomDrawingNumber, oaAllConstructedRules);

	// Ajout du cout de choix de la regle a chaque nouvelle regle construite
	for (i = 0; i < oaAllConstructedRules->GetSize(); i++)
	{
		constructedRule = cast(KDConstructedRule*, oaAllConstructedRules->GetAt(i));
		constructedRule->SetCost(constructedRule->GetCost() + dRuleCost);
	}

	// Gestion particuliere pour empecher l'utilisation recursive des regles de selection
	if (constructionRule->IsSelectionRule())
	{
		assert(IsSelectionRuleForbidden());
		SetSelectionRuleForbidden(false);
	}
}

void KDMultiTableFeatureConstruction::BuildAllConstructedRulesFromLastOperands(
    const KDConstructionRule* constructionRule, const KDClassCompliantRules* classCompliantRules,
    const KDClassCompliantRules* secondaryScopeClassCompliantRules, int nStartOperandIndex,
    KDConstructedRule* templateConstructedRule, const ALString& sPriorTreeNodeName, int nDepth, double dRuleCost,
    double dRandomDrawingNumber, ObjectArray* oaAllConstructedRules) const
{
	KDMultinomialSampleGenerator sampleGenerator;
	boolean bOk;
	ObjectArray oaMatchingAttributes;
	ObjectArray oaMatchingConstructionRules;
	DoubleVector dvMatchingConstructionRuleProbs;
	const KDClassCompliantRules* operandClassCompliantRules;
	const KDClassCompliantRules* operandObjectClassCompliantRules;
	int nAttribute;
	KWDerivationRuleOperand* templateOperand;
	KWAttribute* matchingAttribute;
	KDConstructedRule* constructedRule;
	int nRule;
	KDConstructionRule* matchingConstructionRule;
	ObjectArray oaAllOperandConstructedRules;
	int i;
	KDConstructedRule* operandConstructedRule;
	int nMatchingAttributeNumber;
	int nMatchingRuleNumber;
	double dAttributeOperandCost;
	double dRuleChoiceOperandCost;
	double dRuleOperandCost;
	ALString sChildNodeName;
	DoubleVector dvAttributeRandomDrawingNumbers;
	double dAllRulesRandomDrawingNumber;
	DoubleVector dvConstructionRuleRandomDrawingNumbers;
	DoubleVector dvOperandRandomDrawingNumbers;
	double dChildRandomDrawingNumber;
	double dOperandRandomDrawingNumber;

	require(constructionRule != NULL);
	require(GetConstructionDomain()->LookupConstructionRule(constructionRule->GetName()) != NULL);
	require(classCompliantRules != NULL);
	require(LookupClassCompliantRules(classCompliantRules->GetClass()->GetName()) == classCompliantRules);
	require(secondaryScopeClassCompliantRules == NULL or
		LookupClassCompliantRules(secondaryScopeClassCompliantRules->GetClass()->GetName()) ==
		    secondaryScopeClassCompliantRules);
	require(0 <= nStartOperandIndex and
		nStartOperandIndex < constructionRule->GetDerivationRule()->GetOperandNumber());
	require(templateConstructedRule != NULL);
	require(templateConstructedRule->GetName() == constructionRule->GetName());
	require(templateConstructedRule->GetOperandNumber() == constructionRule->GetOperandNumber());
	require(dRuleCost >= 0);
	require(nDepth >= 0);
	require(dRandomDrawingNumber > 0);

	// Utilisation des contraintes pour determiner si la regle est applicable sur la classe
	// TODO FUTURE IsRuleAllowed(constructionRule->GetName(), classCompliantRules->GetName())
	bOk = true;

	// Parcours des operandes de la regle pour savoir si on peut les alimenter
	if (bOk)
	{
		// Recherche du premier operande a tester
		templateOperand = templateConstructedRule->GetDerivationRule()->GetOperandAt(nStartOperandIndex);

		// Recherche de la classe a utiliser pour l'operande
		if (not templateConstructedRule->GetDerivationRule()->IsSecondaryScopeOperand(nStartOperandIndex))
			operandClassCompliantRules = classCompliantRules;
		else
			operandClassCompliantRules = secondaryScopeClassCompliantRules;

		// Cas particulier de l'operande de selection (deuxieme operande) d'une regle de selection
		if (constructionRule->IsSelectionRule() and nStartOperandIndex == 1 and
		    operandClassCompliantRules != NULL)
		{
			BuildAllSelectionRulesFromSelectionOperand(
			    constructionRule, classCompliantRules, secondaryScopeClassCompliantRules,
			    templateConstructedRule, sPriorTreeNodeName, nDepth, dRuleCost, dRandomDrawingNumber,
			    oaAllConstructedRules);
		}
		// Cas standard: extraction des operandes compatibles avec le type en parametre
		else if (operandClassCompliantRules != NULL)
		{
			// Extraction des attributs de la classe compatibles avec le type en parametre
			ExtractMatchingAttributes(constructionRule, templateOperand, operandClassCompliantRules,
						  &oaMatchingAttributes);

			// Extraction des regles applicables dont le code retour est compatible avec le type en
			// parametre
			ExtractMatchingRules(constructionRule, templateOperand, operandClassCompliantRules,
					     &oaMatchingConstructionRules);

			// Calcul des couts de regularisation par operande
			nMatchingAttributeNumber = oaMatchingAttributes.GetSize();
			nMatchingRuleNumber = oaMatchingConstructionRules.GetSize();
			dAttributeOperandCost = 0;
			dRuleChoiceOperandCost = 0;
			if (nMatchingRuleNumber == 0)
			{
				if (nMatchingAttributeNumber > 0)
					dAttributeOperandCost = log(1.0 * nMatchingAttributeNumber);
			}
			else
			{
				dAttributeOperandCost = log(1.0 * nMatchingAttributeNumber + 1.0);
				dRuleChoiceOperandCost = dAttributeOperandCost;
			}

			// Calcul de la repartition des tirages dans un noeud de l'arbre des prior en fonction du nombre
			// d'attributs et de regles
			DispatchAttributeRandomDrawingNumbers(
			    dRandomDrawingNumber, oaMatchingAttributes.GetSize(), oaMatchingConstructionRules.GetSize(),
			    &dvAttributeRandomDrawingNumbers, dAllRulesRandomDrawingNumber);

			// Calcul des probabilites et des tirages associee aux regles de construction
			if (dAllRulesRandomDrawingNumber > 0 and oaMatchingConstructionRules.GetSize() > 0)
				ComputeConstructionRuleProbs(dAllRulesRandomDrawingNumber, &oaMatchingConstructionRules,
							     &dvMatchingConstructionRuleProbs,
							     &dvConstructionRuleRandomDrawingNumbers);

			// Affichage des caracteristiques locale du noeud de l'arbre du prior en mode trace
			if (sPriorTreeNodeName != "")
			{
				cout << "BuildAllRulesFromLastOperands" << endl;
				cout << "\tSeed\t" << GetRandomSeed() << endl;
				cout << "\tPrior node\t" << sPriorTreeNodeName << endl;
				cout << "\tDrawing number\t" << dRandomDrawingNumber << endl;
				cout << "\tClass\t" << classCompliantRules->GetClassName() << endl;
				cout << "\tConstruction rule\t" << constructionRule->GetName() << endl;
				cout << "\tRuleCost\t" << dRuleCost << endl;
				cout << "\tAttribute operand cost\t" << dAttributeOperandCost << endl;
				cout << "\tRule choice operand cost\t" << dRuleChoiceOperandCost << endl;
				cout << "\tAttributes\t" << oaMatchingAttributes.GetSize() << endl;
				for (nAttribute = 0; nAttribute < oaMatchingAttributes.GetSize(); nAttribute++)
				{
					matchingAttribute = cast(KWAttribute*, oaMatchingAttributes.GetAt(nAttribute));
					cout << "\t\t" << matchingAttribute->GetName() << "\t"
					     << dvAttributeRandomDrawingNumbers.GetAt(nAttribute) << endl;
				}
				cout << "\tRules\t" << oaMatchingConstructionRules.GetSize() << endl;
				if (dAllRulesRandomDrawingNumber > 0)
				{
					for (nRule = 0; nRule < oaMatchingConstructionRules.GetSize(); nRule++)
					{
						matchingConstructionRule =
						    cast(KDConstructionRule*, oaMatchingConstructionRules.GetAt(nRule));
						cout << "\t\t" << matchingConstructionRule->GetName() << "\t"
						     << dvMatchingConstructionRuleProbs.GetAt(nRule) << "\t"
						     << dvConstructionRuleRandomDrawingNumbers.GetAt(nRule) << endl;
					}
				}
			}

			// Parcours des operandes attributs compatibles pour creer les regles
			if (oaMatchingAttributes.GetSize() > 0 and dRuleCost + dAttributeOperandCost <= dMaxRuleCost)
			{
				for (nAttribute = 0; nAttribute < oaMatchingAttributes.GetSize(); nAttribute++)
				{
					matchingAttribute = cast(KWAttribute*, oaMatchingAttributes.GetAt(nAttribute));

					// Arret si trop de regles
					// Pas de probleme de liberation des MatchingAttributes, qui ne sont que
					// references
					if (oaAllConstructedRules->GetSize() >= nMaxRuleNumber)
						break;

					// On ne traite l'attribut que s'il reste des tirages
					dChildRandomDrawingNumber = dvAttributeRandomDrawingNumbers.GetAt(nAttribute);
					if (dChildRandomDrawingNumber == 0)
						continue;

					// On ne traite pas l'attribut s'il est redondant
					if (operandClassCompliantRules->IsAttributeRedundant(
						matchingAttribute->GetName()))
						continue;

					// Parametrage de l'operande de la regle template
					templateConstructedRule->SetAttributeOperandAt(nStartOperandIndex,
										       matchingAttribute);

					// Si dernier operande, on construit la regle
					if (nStartOperandIndex == templateConstructedRule->GetOperandNumber() - 1)
					{
						// Construction de la regle
						constructedRule = templateConstructedRule->Clone();
						constructedRule->SetCost(dRuleCost + dAttributeOperandCost);
						oaAllConstructedRules->Add(constructedRule);
						if (sPriorTreeNodeName != "")
						{
							cout << "Prior leaf\t" << constructedRule->GetCost() << "\t"
							     << sPriorTreeNodeName << matchingAttribute->GetName()
							     << ")" << endl;
							cout << "\tFeature\t" << oaAllConstructedRules->GetSize()
							     << "\t" << *constructedRule << endl;
						}
					}
					// Sinon, on propage pour alimenter les derniers operandes
					else
					{
						operandObjectClassCompliantRules = NULL;
						if (KWType::IsRelation(matchingAttribute->GetType()))
							operandObjectClassCompliantRules = LookupClassCompliantRules(
							    matchingAttribute->GetClass()->GetName());

						// Mise a jour du chemin de donnees courant
						if (operandObjectClassCompliantRules != NULL)
							DataPathPushTableAttribute(matchingAttribute);

						// Propagation de la construction de la regle
						if (sPriorTreeNodeName != "")
							sChildNodeName =
							    sPriorTreeNodeName + matchingAttribute->GetName() + ",";
						BuildAllConstructedRulesFromLastOperands(
						    constructionRule, classCompliantRules,
						    operandObjectClassCompliantRules, nStartOperandIndex + 1,
						    templateConstructedRule, sChildNodeName, nDepth + 1,
						    dRuleCost + dAttributeOperandCost, dChildRandomDrawingNumber,
						    oaAllConstructedRules);

						// Mise a jour du chemin de donnees courant
						if (operandObjectClassCompliantRules != NULL)
							DataPathPopTableAttribute();
					}
				}
				templateConstructedRule->RemoveOperandAt(nStartOperandIndex);
			}

			// Parcours des operandes regles compatibles pour creer les regles
			if (dAllRulesRandomDrawingNumber > 0)
			{
				if (oaMatchingConstructionRules.GetSize() > 0 and nDepth <= nMaxRuleDepth and
				    dRuleCost + dRuleChoiceOperandCost <= dMaxRuleCost)
				{
					for (nRule = 0; nRule < oaMatchingConstructionRules.GetSize(); nRule++)
					{
						matchingConstructionRule =
						    cast(KDConstructionRule*, oaMatchingConstructionRules.GetAt(nRule));

						// Prise en compte du cout complet du choix de la regle
						dRuleOperandCost = dRuleChoiceOperandCost -
								   log(dvMatchingConstructionRuleProbs.GetAt(nRule));

						// Arret si trop de regles
						// Pas de probleme de liberation des MatchingConstructionRules, qui ne
						// sont que referencees
						if (oaAllConstructedRules->GetSize() >= nMaxRuleNumber)
							break;

						// On ne traite la regle que s'il reste des tirages
						dChildRandomDrawingNumber =
						    dvConstructionRuleRandomDrawingNumbers.GetAt(nRule);
						if (dChildRandomDrawingNumber == 0)
							continue;

						// Construction de toutes les regles utilisable comme operande
						oaAllOperandConstructedRules.SetSize(0);
						if (sPriorTreeNodeName != "")
							sChildNodeName = "[" + sPriorTreeNodeName + "]." +
									 matchingConstructionRule->GetName() + "(";
						BuildAllConstructedRules(
						    matchingConstructionRule, operandClassCompliantRules,
						    sChildNodeName, nDepth + 1, dRuleOperandCost,
						    dChildRandomDrawingNumber, &oaAllOperandConstructedRules);

						// On ne traite la regle que si l'on a pu construire des operandes
						// effectivement Il peut n'y avoir aucun operande construit si par
						// exemple on a atteint une limite (cout max, profondeur max...)
						if (oaAllOperandConstructedRules.GetSize() == 0)
							continue;
						// Repartition des tirages par regle utilisable en operande
						DispatchConstructedRuleRandomDrawingNumbers(
						    dChildRandomDrawingNumber, &oaAllOperandConstructedRules,
						    &dvOperandRandomDrawingNumbers);

						// Parcours des regles possibles
						for (i = 0; i < oaAllOperandConstructedRules.GetSize(); i++)
						{
							operandConstructedRule = cast(
							    KDConstructedRule*, oaAllOperandConstructedRules.GetAt(i));
							oaAllOperandConstructedRules.SetAt(i, NULL);

							// Arret si trop de regles
							// Destruction necessaire des regles inutilisees
							if (oaAllConstructedRules->GetSize() >= nMaxRuleNumber)
							{
								// Destruction de la regle courante et du reste du
								// tableau (tous les index avec object non NULL)
								delete operandConstructedRule;
								oaAllOperandConstructedRules.DeleteAll();

								// Arret;
								break;
							}

							// On ne traite la regle comme operande que s'il reste des
							// tirages
							dOperandRandomDrawingNumber =
							    dvOperandRandomDrawingNumbers.GetAt(i);
							if (dOperandRandomDrawingNumber == 0)
							{
								// Destruction de la regle courante uniquement (en
								// positionnant l'index du tableau a NULL)
								delete operandConstructedRule;
								oaAllOperandConstructedRules.SetAt(i, NULL);

								// Arret pour cette regle uniquement
								continue;
							}

							// Parametrage de l'operande de la regle template
							templateConstructedRule->SetRuleOperandAt(
							    nStartOperandIndex, operandConstructedRule);

							// Si dernier operande, on construit la regle
							if (nStartOperandIndex ==
							    templateConstructedRule->GetOperandNumber() - 1)
							{
								constructedRule = templateConstructedRule->Clone();
								constructedRule->SetCost(
								    dRuleCost + operandConstructedRule->GetCost());
								oaAllConstructedRules->Add(constructedRule);
								if (sPriorTreeNodeName != "")
								{
									cout << "Prior leaf\t"
									     << constructedRule->GetCost() << "\t"
									     << sPriorTreeNodeName
									     << matchingConstructionRule->GetName()
									     << "())" << endl;
									cout << "\tFeature\t"
									     << oaAllConstructedRules->GetSize() << "\t"
									     << *constructedRule << endl;
								}
							}
							// Sinon, on propage pour alimenter les derniers operandes
							else
							{
								// Completion des infos sur la regle pour piloter la
								// suite des algo (code retour de la regle...)
								// Attention: la completion d'info ne doit pas avoir
								// d'impact sur la classe et sa fraicheur
								operandObjectClassCompliantRules = NULL;
								if (KWType::IsRelation(
									operandConstructedRule->GetType()))
									operandObjectClassCompliantRules =
									    LookupClassCompliantRules(
										operandConstructedRule
										    ->GetObjectClassName());

								// Mise a jour du chemin de donnees courant, celui du
								// premier operande de la regle
								if (operandObjectClassCompliantRules != NULL)
								{
									assert(operandConstructedRule
										   ->GetAttributeOperandAt(0)
										   ->GetType() ==
									       operandConstructedRule->GetType());
									DataPathPushTableAttribute(
									    operandConstructedRule
										->GetAttributeOperandAt(0));
								}

								// Propagation de la construction de la regle
								if (sPriorTreeNodeName != "")
									sChildNodeName =
									    sPriorTreeNodeName +
									    matchingConstructionRule->GetName() + "(),";
								BuildAllConstructedRulesFromLastOperands(
								    constructionRule, classCompliantRules,
								    operandObjectClassCompliantRules,
								    nStartOperandIndex + 1, templateConstructedRule,
								    sPriorTreeNodeName, nDepth + 1,
								    dRuleCost + operandConstructedRule->GetCost(),
								    dOperandRandomDrawingNumber, oaAllConstructedRules);

								// Mise a jour du chemin de donnees courant
								if (operandObjectClassCompliantRules != NULL)
								{
									assert(operandConstructedRule
										   ->GetAttributeOperandAt(0)
										   ->GetType() ==
									       operandConstructedRule->GetType());
									DataPathPopTableAttribute();
								}
							}

							// Nettoyage
							templateConstructedRule->DeleteOperandAt(nStartOperandIndex);
						}
					}
					assert(templateConstructedRule->GetOperandOriginAt(nStartOperandIndex) ==
					       KDConstructedRule::None);
				}
			}
		}
	}
}

void KDMultiTableFeatureConstruction::DispatchAttributeRandomDrawingNumbers(
    double dRandomDrawingNumber, int nAttributeNumber, int nRuleNumber, DoubleVector* dvAttributeRandomDrawingNumbers,
    double& dAllRulesRandomDrawingNumber) const
{
	KDMultinomialSampleGenerator sampleGenerator;

	require(dRandomDrawingNumber >= 0);
	require(nAttributeNumber >= 0);
	require(nRuleNumber >= 0);
	require(dvAttributeRandomDrawingNumbers != NULL);

	// Initialisation des resultats
	dvAttributeRandomDrawingNumbers->SetSize(nAttributeNumber);
	dAllRulesRandomDrawingNumber = 0;

	// Cas ou il n'y a pas de regles
	if (nRuleNumber == 0)
		sampleGenerator.ComputeBestEquidistributedSample(dRandomDrawingNumber, nAttributeNumber,
								 dvAttributeRandomDrawingNumbers);
	// Cas ou il n'y a pas d'attributs
	else if (nAttributeNumber == 0)
		dAllRulesRandomDrawingNumber = dRandomDrawingNumber;
	// Cas ou il y a des attributs et des regles
	else
	{
		// Cas ou il y a moins de tirages que d'attributs: on privilegie les attributs
		if (dRandomDrawingNumber <= nAttributeNumber)
			sampleGenerator.ComputeBestEquidistributedSample(dRandomDrawingNumber, nAttributeNumber,
									 dvAttributeRandomDrawingNumbers);
		// Sinon, on repartit au mieux en fonction des regles tirables
		else
		{
			// Calcul du nombre de tirages attribues aux regles (potentiellement un de moins que par
			// attribut a cause de la division entiere)
			dAllRulesRandomDrawingNumber = floor(0.5 + dRandomDrawingNumber / (nAttributeNumber + 1));

			// Dispatch des tirages restant pour les attributs
			sampleGenerator.ComputeBestEquidistributedSample(
			    dRandomDrawingNumber - dAllRulesRandomDrawingNumber, nAttributeNumber,
			    dvAttributeRandomDrawingNumbers);
		}
	}
}

void KDMultiTableFeatureConstruction::ComputeConstructionRuleProbs(double dRandomDrawingNumber,
								   ObjectArray* oaConstructionRules,
								   DoubleVector* dvConstructionRuleProbs,
								   DoubleVector* dvRandomDrawingNumbers) const

{
	int nRule;
	KDConstructionRule* constructionRule;
	IntVector ivRecursionLevels;
	IntVector ivRecursionLevelRuleNumbers;
	DoubleVector dvLevelProbs;
	int nMaxRecursionLevel;
	int nMaxOperandNumber;
	const double dEpsilonProb = 1e-9;
	double dProb;
	double dTotalProb;
	DoubleVector dvLevelRandomDrawingNumbers;
	DoubleVector dvUniformRandomDrawingNumbers;
	KDMultinomialSampleGenerator sampleGenerator;

	require(dRandomDrawingNumber > 0);
	require(oaConstructionRules != NULL);
	require(oaConstructionRules->GetSize() > 0);
	require(dvConstructionRuleProbs != NULL);
	require(dvRandomDrawingNumbers != NULL);

	// Tri des regles par niveau de recursion
	oaConstructionRules->SetCompareFunction(KWConstructionRuleCompareRecursionLevel);
	oaConstructionRules->Sort();

	// Calcul des niveaux de recursion et nombre d'operandes amx
	nMaxRecursionLevel = 0;
	nMaxOperandNumber = 0;
	for (nRule = 0; nRule < oaConstructionRules->GetSize(); nRule++)
	{
		constructionRule = cast(KDConstructionRule*, oaConstructionRules->GetAt(nRule));
		nMaxRecursionLevel = max(nMaxRecursionLevel, constructionRule->GetRecursionLevel());
		nMaxOperandNumber = max(nMaxOperandNumber, constructionRule->GetOperandNumber());
	}

	// Memorisation des probas par regle
	// Proba decroissante par regle, de facon a prioriser les premieres regles
	dvConstructionRuleProbs->SetSize(oaConstructionRules->GetSize());
	dTotalProb = 0;
	for (nRule = 0; nRule < oaConstructionRules->GetSize(); nRule++)
	{
		constructionRule = cast(KDConstructionRule*, oaConstructionRules->GetAt(nRule));

		// Memorisation de la proba
		dProb = (1.0 / oaConstructionRules->GetSize()) *
			(1 + -dEpsilonProb * (constructionRule->GetRecursionLevel() * nMaxOperandNumber +
					      constructionRule->GetOperandNumber()));
		dTotalProb += dProb;
		dvConstructionRuleProbs->SetAt(nRule, dProb);
	}

	// Normalisation des probas
	for (nRule = 0; nRule < oaConstructionRules->GetSize(); nRule++)
	{
		dProb = dvConstructionRuleProbs->GetAt(nRule) / dTotalProb;
		dvConstructionRuleProbs->SetAt(nRule, dProb);
	}

	// Dispatching des tirages selon les probabilites des regles
	sampleGenerator.ComputeBestSample(dRandomDrawingNumber, dvConstructionRuleProbs, dvRandomDrawingNumbers);
}

void KDMultiTableFeatureConstruction::DispatchConstructedRuleRandomDrawingNumbers(
    double dRandomDrawingNumber, const ObjectArray* oaConstructedRules, DoubleVector* dvRandomDrawingNumbers) const
{
	KDMultinomialSampleGenerator sampleGenerator;
	boolean bDisplay = false;
	DoubleVector dvRuleProbs;
	int nRule;
	KDConstructedRule* rule;
	double dProb;

	require(dRandomDrawingNumber >= 0);
	require(oaConstructedRules != NULL);
	require(oaConstructedRules->GetSize() > 0);
	require(dvRandomDrawingNumbers != NULL);

	// Cas particulier d'une seule proba
	if (oaConstructedRules->GetSize() == 1)
	{
		dvRandomDrawingNumbers->SetSize(1);
		dvRandomDrawingNumbers->SetAt(0, dRandomDrawingNumber);
	}
	// Cas general
	else
	{
		// Collecte des probas des regles de derivation
		// On a collecte que les regles les plus probables, donc une partie de la distribution
		dvRuleProbs.SetSize(oaConstructedRules->GetSize());
		for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
		{
			rule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));
			assert(rule->GetCost() >= 0);

			// Le cout est le log negatif de la proba
			dProb = exp(-rule->GetCost());
			assert(0 <= dProb and dProb <= 1);
			dvRuleProbs.SetAt(nRule, dProb);
		}
		assert(sampleGenerator.CheckPartialProbVector(&dvRuleProbs));

		// Estimation heuristique de la distribution la plus probable pour un effectif donne
		sampleGenerator.ComputeBestSample(dRandomDrawingNumber, &dvRuleProbs, dvRandomDrawingNumbers);
	}

	// Affichage des resultats
	if (bDisplay)
	{
		cout << "DispatchRuleRandomDrawingNumbers\t" << dRandomDrawingNumber << endl;
		cout << "\tIndex\tNumber\tProb\tRule" << endl;
		for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
		{
			rule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));
			cout << "\t" << nRule + 1;
			cout << "\t" << dvRandomDrawingNumbers->GetAt(nRule);
			cout << "\t" << exp(-rule->GetCost());
			cout << "\t" << *rule;
			cout << endl;
		}
	}
}

boolean KDMultiTableFeatureConstruction::FilterConstructedRulesForRandomDrawing(double dRandomDrawingNumber,
										ObjectArray* oaConstructedRules) const
{
	boolean bFiltered;
	KDMultinomialSampleGenerator sampleGenerator;
	DoubleVector dvRuleProbs;
	int nRule;
	KDConstructedRule* rule;
	double dProb;
	double dMaxProb;
	double dProbThreshold;
	int i;
	int nRank;

	require(dRandomDrawingNumber >= 0);
	require(oaConstructedRules != NULL);
	require(oaConstructedRules->GetSize() > 0);

	// Cas particulier d'une seule proba
	if (oaConstructedRules->GetSize() == 1)
		return false;

	// Collecte des probas des regles de derivation
	// On a collecte que les regles les plus probables, donc une partie de la distribution
	dvRuleProbs.SetSize(oaConstructedRules->GetSize());
	for (nRule = 0; nRule < oaConstructedRules->GetSize(); nRule++)
	{
		rule = cast(KDConstructedRule*, oaConstructedRules->GetAt(nRule));
		assert(rule->GetCost() >= 0);

		// Le cout est le log negatif de la proba
		dProb = exp(-rule->GetCost());
		assert(0 <= dProb and dProb <= 1);
		dvRuleProbs.SetAt(nRule, dProb);
	}
	assert(sampleGenerator.CheckPartialProbVector(&dvRuleProbs));

	// Tri des probas obtenues
	dvRuleProbs.Sort();

	// Parcours de probas en partant de la plus grande pour rechercher la plus petite pouvant donner lieu a un
	// tirage
	dMaxProb = dvRuleProbs.GetAt(dvRuleProbs.GetSize() - 1);
	dProbThreshold = 0;
	nRank = 2;
	for (i = dvRuleProbs.GetSize() - 2; i >= 0; i--)
	{
		dProb = dvRuleProbs.GetAt(i);
		if (dProb <= dMaxProb / (dRandomDrawingNumber - nRank + 2))
		{
			dProbThreshold = dProb;
			break;
		}
		nRank++;
	}

	// Filtrage si necessaire des regles de proba trop petite
	bFiltered = (dProbThreshold > 0);
	if (bFiltered)
	{
		// Destruction des regles de probs inferieure au seuil
		nRule = 0;
		for (i = 0; i < oaConstructedRules->GetSize(); i++)
		{
			rule = cast(KDConstructedRule*, oaConstructedRules->GetAt(i));
			assert(rule->GetCost() >= 0);

			// Supression de la regle si proba sous le seuil
			dProb = exp(-rule->GetCost());
			if (dProb <= dProbThreshold)
				delete rule;
			// Sinon, on garde la regle
			else
			{
				oaConstructedRules->SetAt(nRule, rule);
				nRule++;
			}
		}

		// Retaillage du tableau
		oaConstructedRules->SetSize(nRule);
	}
	return bFiltered;
}

boolean KDMultiTableFeatureConstruction::IsConstructionBasicallyPossible(KWClass* mainClass) const
{
	int nRule;
	KDConstructionRule* constructionRule;
	const KWDerivationRuleOperand* operand;

	require(mainClass != NULL);
	require(mainClass->IsCompiled());

	// Parcours des regles de construction utilisables ou non pour detecter si elle peut rouver des operande dans la
	// classe principale
	for (nRule = 0; nRule < GetConstructionDomain()->GetConstructionRuleNumber(); nRule++)
	{
		constructionRule = GetConstructionDomain()->GetConstructionRuleAt(nRule);

		// On regarde si le premier operande est appliquable directement
		if (constructionRule->GetOperandNumber() > 0)
		{
			operand = constructionRule->GetOperandAt(0);
			if (mainClass->GetUsedAttributeNumberForType(operand->GetType()))
				return true;
		}
	}
	return false;
}

void KDMultiTableFeatureConstruction::ComputeAllClassesCompliantRules(
    KWClass* mainClass, KDClassDomainCompliantRules* outputClassDomainCompliantRules) const
{
	boolean bDisplay = false;
	int nClass;
	KDClassCompliantRules* classCompliantRules;
	const KWClass* kwcClass;
	const KWClass* kwcUsedClass;
	KDClassCompliantRules* usedClassCompliantRules;
	int nAttribute;
	KWAttribute* attribute;
	boolean bAddConstructionRules;
	int nRule;
	KDConstructionRule* constructionRule;
	KDConstructionRule* compliantConstructionRule;
	ObjectArray oaUsedConstructionRules;
	ObjectArray oaAllClassesCompliantConstructionRules;
	ObjectArray* oaCompliantConstructionRules;
	int nKey;
	int nRecursionLevel;

	require(Check());
	require(mainClass != NULL);
	require(mainClass->IsCompiled());
	require(outputClassDomainCompliantRules != NULL);

	// Nettoyage prealable
	outputClassDomainCompliantRules->Clean();

	// Initialisation
	outputClassDomainCompliantRules->SetClass(mainClass);
	outputClassDomainCompliantRules->SetConstructionDomain(GetConstructionDomain());

	// Ajout de la premiere contrainte de classe pour la classe principale
	classCompliantRules = new KDClassCompliantRules;
	classCompliantRules->SetClass(mainClass);
	outputClassDomainCompliantRules->AddClassCompliantRules(classCompliantRules);

	// Ajout de toute l'arborescence de classes par parcours iteratif des classes rajoutees
	// Le tableau de contraintes de classe s'agrandit au fur et a mesure, jusqu'a ce que
	// toutes les classes utilisees aient ete decouvertes
	for (nClass = 0; nClass < outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetSize(); nClass++)
	{
		classCompliantRules =
		    cast(KDClassCompliantRules*,
			 outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetAt(nClass));
		kwcClass = classCompliantRules->GetClass();

		// On interdit les cles de la classe
		// De facon generale, il s'agit d'un principe: la cle ne sert qu'a encoder une structure et la
		// memoriser: il ne s'agit pas d'attributs porteurs d'information. Pour la classe racine, les cle
		// apparaissent une seule fois instance, et ne peuvent etre informatives. Pour les classes secondaires
		// inclues, les cles sont soient unique par instance principale (la cle de l'incluant) sans interet,
		// soit avec un role d'identifiant dans la table secondaire, sans interet autre que compter le nombre
		// d'enregistrements (par CountDistinct, ici redondant avec Count).
		for (nKey = 0; nKey < kwcClass->GetKeyAttributeNumber(); nKey++)
		{
			classCompliantRules->GetForbiddenAttributes()->SetAt(kwcClass->GetKeyAttributeNameAt(nKey),
									     kwcClass->GetKeyAttributeAt(nKey));
		}

		// Parcours des attributs de la classe
		for (nAttribute = 0; nAttribute < kwcClass->GetUsedAttributeNumber(); nAttribute++)
		{
			attribute = kwcClass->GetUsedAttributeAt(nAttribute);

			// Ajout de la classe utilisee potentielle
			if (KWType::IsRelation(attribute->GetType()))
			{
				kwcUsedClass = attribute->GetClass();

				// Ajout d'une contrainte de classe si non deja memorisee
				if (outputClassDomainCompliantRules->LookupClassCompliantRules(
					kwcUsedClass->GetName()) == NULL)
				{
					assert(kwcUsedClass != GetClass());

					usedClassCompliantRules = new KDClassCompliantRules;
					usedClassCompliantRules->SetClass(kwcUsedClass);
					outputClassDomainCompliantRules->AddClassCompliantRules(
					    usedClassCompliantRules);
				}
			}
		}
	}

	// Affichage des classes concernees
	if (bDisplay)
	{
		cout << "All classes" << endl;
		for (nClass = 0; nClass < outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetSize();
		     nClass++)
		{
			classCompliantRules =
			    cast(KDClassCompliantRules*,
				 outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetAt(nClass));
			kwcClass = classCompliantRules->GetClass();
			cout << "\t" << kwcClass->GetName() << endl;
		}
	}

	// Recherche des regles de construction utilisees
	for (nRule = 0; nRule < GetConstructionDomain()->GetConstructionRuleNumber(); nRule++)
	{
		constructionRule = GetConstructionDomain()->GetConstructionRuleAt(nRule);
		if (constructionRule->GetUsed())
			oaUsedConstructionRules.Add(constructionRule);
	}

	// Affichage des regles de construction concernees
	if (bDisplay)
	{
		cout << "All construction rules" << endl;
		for (nRule = 0; nRule < oaUsedConstructionRules.GetSize(); nRule++)
		{
			constructionRule = cast(KDConstructionRule*, oaUsedConstructionRules.GetAt(nRule));
			cout << "\t" << constructionRule->GetFamilyName() << "\t" << constructionRule->GetName()
			     << endl;
		}
	}

	// Ajout des regles de construction applicables pour chaque classe
	// Pour etre applicable, une regle doit pouvoir prendre ses operandes parmi
	// les variables de la classe ou parmi les codes retours des regles applicables
	// sur la classe. Le calcul se fait recursivement, tant que chaque nouvelle passe
	// entraine l'ajout de regles applicables sur au moins une classe
	bAddConstructionRules = true;
	nRecursionLevel = 0;
	while (bAddConstructionRules)
	{
		// Au debut d'une passe, on reinitialise l'indicateur d'ajout de regles
		bAddConstructionRules = false;
		nRecursionLevel++;

		// Initialisation d'un tableau par classe, de tableau de regles compatibles
		oaAllClassesCompliantConstructionRules.SetSize(
		    outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetSize());
		for (nClass = 0; nClass < oaAllClassesCompliantConstructionRules.GetSize(); nClass++)
			oaAllClassesCompliantConstructionRules.SetAt(nClass, new ObjectArray);

		// Parcours des regles de construction utilisees pour identifier les regles compatibles
		// Dans cette premiere passe, on ne fait que les identifier, en se basant sur les attributs
		// ou les regles issues de l'etape precedente
		for (nRule = 0; nRule < oaUsedConstructionRules.GetSize(); nRule++)
		{
			constructionRule = cast(KDConstructionRule*, oaUsedConstructionRules.GetAt(nRule));

			// Parcours des contraintes de classe pour ajouter des regles applicables
			for (nClass = 0; nClass < oaAllClassesCompliantConstructionRules.GetSize(); nClass++)
			{
				classCompliantRules =
				    cast(KDClassCompliantRules*,
					 outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetAt(nClass));
				oaCompliantConstructionRules =
				    cast(ObjectArray*, oaAllClassesCompliantConstructionRules.GetAt(nClass));

				// Test si la regle de construction est applicable sur la classe ou ses regles
				// applicables
				if (classCompliantRules->SearchCompliantConstructionRule(constructionRule->GetName()) ==
					NULL and
				    IsConstructionRuleApplicable(constructionRule, classCompliantRules))
				{
					bAddConstructionRules = true;
					compliantConstructionRule = constructionRule->Clone();
					compliantConstructionRule->SetClassName(classCompliantRules->GetClassName());
					compliantConstructionRule->SetRecursionLevel(nRecursionLevel);
					oaCompliantConstructionRules->Add(compliantConstructionRule);
					if (bDisplay)
					{
						cout << "Add\t" << compliantConstructionRule->GetRecursionLevel()
						     << "\t" << classCompliantRules->GetClass()->GetName() << "\t"
						     << compliantConstructionRule->GetFamilyName() << "\t"
						     << compliantConstructionRule->GetName() << endl;
					}
				}
			}
		}

		// Ajout effectif des regles compatibles, par classe
		for (nClass = 0; nClass < oaAllClassesCompliantConstructionRules.GetSize(); nClass++)
		{
			classCompliantRules =
			    cast(KDClassCompliantRules*,
				 outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetAt(nClass));
			oaCompliantConstructionRules =
			    cast(ObjectArray*, oaAllClassesCompliantConstructionRules.GetAt(nClass));

			// Ajout des regles compatibles
			for (nRule = 0; nRule < oaCompliantConstructionRules->GetSize(); nRule++)
			{
				compliantConstructionRule =
				    cast(KDConstructionRule*, oaCompliantConstructionRules->GetAt(nRule));
				classCompliantRules->InsertCompliantConstructionRule(compliantConstructionRule);
			}
		}
		oaAllClassesCompliantConstructionRules.DeleteAll();
	}

	// Ajout des attributs et blocs derives potentiellement constructibles
	for (nClass = 0; nClass < outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetSize(); nClass++)
	{
		classCompliantRules =
		    cast(KDClassCompliantRules*,
			 outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetAt(nClass));
		classCompliantRules->CollectConstructedAttributesAndBlocks();
	}

	// Affichage des regles concernees par classe
	if (bDisplay)
	{
		cout << "All compliant rules" << endl;
		for (nClass = 0; nClass < outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetSize();
		     nClass++)
		{
			classCompliantRules =
			    cast(KDClassCompliantRules*,
				 outputClassDomainCompliantRules->GetAllClassesCompliantRules()->GetAt(nClass));
			kwcClass = classCompliantRules->GetClass();

			// Parcours des regles applicables a la classe
			for (nRule = 0; nRule < classCompliantRules->GetCompliantConstructionRuleNumber(); nRule++)
			{
				constructionRule = classCompliantRules->GetCompliantConstructionRuleAt(nRule);
				cout << "\t" << kwcClass->GetName();
				cout << "\t" << constructionRule->GetRecursionLevel() << "\t"
				     << constructionRule->GetFamilyName() << "\t" << constructionRule->GetName()
				     << endl;
			}

			// Affichages des regles construites initiales
			classCompliantRules->DisplayConstructedAttributes(cout);
			classCompliantRules->DisplayConstructedAttributeBlocks(cout);
		}
	}
}

KDClassDomainCompliantRules* KDMultiTableFeatureConstruction::GetClassDomainCompliantRules() const
{
	return classDomainCompliantRules;
}

KDClassCompliantRules* KDMultiTableFeatureConstruction::LookupClassCompliantRules(const ALString& sClassName) const
{
	return classDomainCompliantRules->LookupClassCompliantRules(sClassName);
}

boolean
KDMultiTableFeatureConstruction::IsConstructionRuleApplicable(const KDConstructionRule* constructionRule,
							      const KDClassCompliantRules* classCompliantRules) const
{
	return IsConstructionRuleApplicableFromLastOperands(constructionRule, 0, classCompliantRules, NULL);
}

boolean KDMultiTableFeatureConstruction::IsConstructionRuleApplicableFromLastOperands(
    const KDConstructionRule* constructionRule, int nStartOperandIndex,
    const KDClassCompliantRules* classCompliantRules,
    const KDClassCompliantRules* secondaryScopeClassCompliantRules) const
{
	boolean bOk;
	const KWDerivationRule* derivationRule;
	KWDerivationRuleOperand* operand;
	ObjectArray oaMatchingAttributes;
	ObjectArray oaMatchingConstructionRules;
	const KDClassCompliantRules* operandClassCompliantRules;
	const KDClassCompliantRules* operandObjectClassCompliantRules;
	int nAttribute;
	KWAttribute* attribute;

	require(constructionRule != NULL);
	require(classCompliantRules != NULL);
	require(0 <= nStartOperandIndex and
		nStartOperandIndex < constructionRule->GetDerivationRule()->GetOperandNumber());

	// Utilisation des contraintes pour determiner si la regle est applicable sur la classe
	// TODO FUTURE IsRuleAllowed(constructionRule->GetName(), classCompliantRules->GetName())
	bOk = true;

	// Parcours des operandes de la regle pour savoir si on peut les alimenter
	if (bOk)
	{
		derivationRule = constructionRule->GetDerivationRule();

		// Recherche du premier operande a tester
		operand = derivationRule->GetOperandAt(nStartOperandIndex);

		// Recherche de la classe a utiliser pour l'operande
		if (not derivationRule->IsSecondaryScopeOperand(nStartOperandIndex))
			operandClassCompliantRules = classCompliantRules;
		else
			operandClassCompliantRules = secondaryScopeClassCompliantRules;

		// Extraction des attributs de la classe compatibles avec le type en parametre
		if (operandClassCompliantRules == NULL)
			bOk = false;
		else
		{
			ExtractMatchingAttributes(constructionRule, operand, operandClassCompliantRules,
						  &oaMatchingAttributes);
			bOk = (oaMatchingAttributes.GetSize() > 0);
		}

		// Extraction des regles applicables dont le code retour est compatible avec le type en parametre
		// Seulement si pas d'operande trouve parmi les variables natives
		if (not bOk and operandClassCompliantRules != NULL)
		{
			ExtractMatchingRules(constructionRule, operand, operandClassCompliantRules,
					     &oaMatchingConstructionRules);
			bOk = (oaMatchingConstructionRules.GetSize() > 0);
		}

		// On propage le test d'applicabilite aux operandes suivants
		if (bOk and nStartOperandIndex < derivationRule->GetOperandNumber() - 1)
		{
			// Recherche de l'eventuelle classe d'objet associe a l'operande
			// Si l'operande est deja au sous-niveau, pas de changement de contexte
			if (derivationRule->IsSecondaryScopeOperand(nStartOperandIndex))
			{
				bOk = IsConstructionRuleApplicableFromLastOperands(
				    constructionRule, nStartOperandIndex + 1, classCompliantRules,
				    secondaryScopeClassCompliantRules);
			}
			// Si operande au premier niveau, recherche parmi tous les sous-niveaux possibles
			else
			{
				// Parcours des attributs potentiels pour alimenter le contexte
				bOk = false;
				for (nAttribute = 0; nAttribute < oaMatchingAttributes.GetSize(); nAttribute++)
				{
					attribute = cast(KWAttribute*, oaMatchingAttributes.GetAt(nAttribute));

					// On ne traite pas l'attribut s'il est redondant
					if (operandClassCompliantRules->IsAttributeRedundant(attribute->GetName()))
						continue;

					// Recherche de la classe Object de l'attribut
					operandObjectClassCompliantRules = NULL;
					if (KWType::IsRelation(operand->GetType()))
						operandObjectClassCompliantRules =
						    LookupClassCompliantRules(attribute->GetClass()->GetName());

					// Propagation du test d'applicabilite
					bOk = IsConstructionRuleApplicableFromLastOperands(
					    constructionRule, nStartOperandIndex + 1, classCompliantRules,
					    operandObjectClassCompliantRules);

					// Arret si OK
					if (bOk)
						break;
				}
			}
		}
	}
	return bOk;
}

void KDMultiTableFeatureConstruction::ExtractMatchingAttributes(const KDConstructionRule* constructionRule,
								const KWDerivationRuleOperand* operand,
								const KDClassCompliantRules* classCompliantRules,
								ObjectArray* oaMatchingAttributes) const
{
	const KWClass* kwcClass;
	int nAttribute;
	KWAttribute* inputAttribute;
	boolean bMatching;
	ALString sAttributeObjectClassName;

	require(constructionRule != NULL);
	require(GetConstructionDomain()->LookupConstructionRule(constructionRule->GetName()) != NULL);
	require(operand != NULL);
	require(classCompliantRules != NULL);
	require(oaMatchingAttributes != NULL);

	// Parcours des attributs de la classe pour selectionner ceux qui correspondent au type demande
	oaMatchingAttributes->SetSize(0);
	kwcClass = classCompliantRules->GetClass();
	for (nAttribute = 0; nAttribute < kwcClass->GetUsedAttributeNumber(); nAttribute++)
	{
		inputAttribute = kwcClass->GetUsedAttributeAt(nAttribute);

		// Utilisation des contraintes pour determiner si la regle est applicable sur la classe
		// TODO FUTURE IsRuleOperandAllowed(constructionRule->GetName(), classCompliantRules->GetName())
		bMatching = true;

		// Test s'il s'agit d'un attribut de donnee
		if (bMatching)
			bMatching = KWType::IsData(inputAttribute->GetType());

		// Test si l'attribut n'est pas interdit
		if (bMatching)
			bMatching = not classCompliantRules->IsAttributeForbidden(inputAttribute->GetName());

		// Test si l'attribut n'est pas l'attribut cible
		if (bMatching and kwcClass == GetClass() and inputAttribute->GetName() == GetTargetAttributeName())
			bMatching = false;

		// Test si correspondance de type
		if (bMatching)
		{
			// Classe Object de l'attribut si necessaire
			sAttributeObjectClassName = "";
			if (inputAttribute->GetClass() != NULL)
				sAttributeObjectClassName = inputAttribute->GetClass()->GetName();

			// Cas des regles de selection
			if (constructionRule->IsSelectionRule())
			{
				bMatching = IsTypeMatchingSelectionOperand(
				    constructionRule, operand, inputAttribute->GetType(), sAttributeObjectClassName);
			}
			// Cas standard
			else
			{
				bMatching = IsTypeMatching(operand->GetType(), operand->GetSupplementTypeName(),
							   inputAttribute->GetType(), sAttributeObjectClassName);
			}
		}

		// Ajout de l'attribut si correspondance
		if (bMatching)
			oaMatchingAttributes->Add(inputAttribute);
	}
}

void KDMultiTableFeatureConstruction::ExtractMatchingRules(const KDConstructionRule* constructionRule,
							   const KWDerivationRuleOperand* operand,
							   const KDClassCompliantRules* classCompliantRules,
							   ObjectArray* oaMatchingConstructionRules) const
{
	int nRule;
	KDConstructionRule* inputConstructionRule;
	const KWDerivationRule* inputDerivationRule;
	boolean bMatching;

	require(constructionRule != NULL);
	require(GetConstructionDomain()->LookupConstructionRule(constructionRule->GetName()) != NULL);
	require(operand != NULL);
	require(classCompliantRules != NULL);
	require(oaMatchingConstructionRules != NULL);

	// Parcours des attributs de la classe pour selectionner ceux qui correspondent au type demande
	oaMatchingConstructionRules->SetSize(0);
	for (nRule = 0; nRule < classCompliantRules->GetCompliantConstructionRuleNumber(); nRule++)
	{
		inputConstructionRule = classCompliantRules->GetCompliantConstructionRuleAt(nRule);
		inputDerivationRule = inputConstructionRule->GetDerivationRule();

		// Utilisation des contraintes pour determiner si la regle est applicable sur la classe
		// TODO FUTURE IsRuleOperandAllowed(constructionRule->GetName(), classCompliantRules->GetName())
		bMatching = true;

		// Si regle de selection interdite: pas le droit de reutiliser une regle de selection
		// L'interdiction a ici un impact sur le cout de codage (la regle de selection n'est plus un choix
		// possible)
		if (IsSelectionRuleForbidden())
			bMatching = not inputConstructionRule->IsSelectionRule();

		// Test si correspondance de type
		if (bMatching)
		{
			// Cas des regles de selection
			if (constructionRule->IsSelectionRule())
			{
				bMatching = IsTypeMatchingSelectionOperand(
				    constructionRule, operand, inputDerivationRule->GetType(),
				    inputDerivationRule->GetSupplementTypeName());
			}
			// Cas standard
			else
			{
				bMatching = IsTypeMatching(operand->GetType(), operand->GetSupplementTypeName(),
							   inputDerivationRule->GetType(),
							   inputDerivationRule->GetSupplementTypeName());
			}
		}

		// Ajout de la regle si correspondance
		if (bMatching)
			oaMatchingConstructionRules->Add(inputConstructionRule);
	}
}

boolean KDMultiTableFeatureConstruction::IsTypeMatching(int nRefType, const ALString& sRefObjectClassName, int nType,
							const ALString& sObjectClassName) const
{
	boolean bMatching;

	require(KWType::IsData(nRefType));
	require(KWType::IsData(nType));

	// Test si correspondance de type
	bMatching = (nType == nRefType);
	if (bMatching and KWType::IsRelation(nRefType) and sRefObjectClassName != "" and sObjectClassName != "")
		bMatching = (sObjectClassName == sRefObjectClassName);
	return bMatching;
}

boolean KDMultiTableFeatureConstruction::IsTypeMatchingSelectionOperand(const KDConstructionRule* selectionRule,
									const KWDerivationRuleOperand* operand,
									int nType,
									const ALString& sObjectClassName) const
{
	boolean bMatching;

	require(selectionRule != NULL);
	require(selectionRule->IsSelectionRule());
	require(operand->GetType() == selectionRule->GetOperandAt(0)->GetType() or
		operand->GetType() == selectionRule->GetOperandAt(1)->GetType());

	// Si premier operande (ObjectArray), traitement standard
	if (operand->GetType() == selectionRule->GetOperandAt(0)->GetType())
	{
		assert(operand->GetType() == KWType::ObjectArray);
		bMatching =
		    IsTypeMatching(operand->GetType(), operand->GetSupplementTypeName(), nType, sObjectClassName);
	}
	// Si deuxieme operande, on accepte les numeriques ou les categoriels
	else
	{
		bMatching =
		    IsTypeMatching(KWType::Continuous, operand->GetSupplementTypeName(), nType, sObjectClassName) or
		    IsTypeMatching(KWType::Symbol, operand->GetSupplementTypeName(), nType, sObjectClassName);
	}
	return bMatching;
}

void KDMultiTableFeatureConstruction::BuildAllSelectionRulesFromSelectionOperand(
    const KDConstructionRule* selectionRule, const KDClassCompliantRules* classCompliantRules,
    const KDClassCompliantRules* secondaryScopeClassCompliantRules, KDConstructedRule* templateConstructedRule,
    const ALString& sPriorTreeNodeName, int nDepth, double dRuleCost, double dRandomDrawingNumber,
    ObjectArray* oaAllConstructedRules) const
{
	KDMultinomialSampleGenerator sampleGenerator;
	KWDerivationRuleOperand* templateSelectionOperand;
	ObjectArray oaMatchingAttributes;
	ObjectArray oaMatchingConstructionRules;
	DoubleVector dvMatchingConstructionRuleProbs;
	int nAttribute;
	KWAttribute* matchingAttribute;
	KDConstructedRule* constructedRule;
	int nRule;
	KDConstructionRule* matchingConstructionRule;
	ObjectArray oaAllOperandConstructedRules;
	ObjectArray oaAllOperandSelectionRules;
	int i;
	KDConstructedRule* operandConstructedRule;
	int nMatchingAttributeNumber;
	int nMatchingRuleNumber;
	double dOperandConstructedRule;
	double dAttributeOperandCost;
	double dRuleChoiceOperandCost;
	double dRuleOperandCost;
	ALString sChildNodeName;
	DoubleVector dvAttributeRandomDrawingNumbers;
	DoubleVector dvConstructionRuleRandomDrawingNumbers;
	DoubleVector dvOperandRandomDrawingNumbers;
	double dAllRulesRandomDrawingNumber;
	double dChildRandomDrawingNumber;
	double dOperandRandomDrawingNumber;
	double dSelectionRandomDrawingNumber;
	DoubleVector dvSelectionSizeRandomDrawingNumbers;
	int nSize;
	DoubleVector dvAllUnivariateSelectionOperandProbs;
	ObjectArray oaAllUnivariateSelectionOperands;
	ObjectArray oaSelectionOperandIndexedFrequencies;
	ObjectArray oaAllSelectionParts;
	KDConstructedPart* completedSelectionPart;
	double dSelectionOperandProb;
	KDClassSelectionStats* classSelectionStats;
	KDClassSelectionOperandStats* classSelectionOperandStats;
	boolean bRuleFiltered;
	int nInitialRuleNumber;

	require(selectionRule != NULL);
	require(selectionRule->IsSelectionRule());
	require(secondaryScopeClassCompliantRules != NULL);
	require(GetConstructionDomain()->LookupConstructionRule(selectionRule->GetName()) != NULL);
	require(classCompliantRules != NULL);
	require(LookupClassCompliantRules(classCompliantRules->GetClass()->GetName()) == classCompliantRules);
	require(secondaryScopeClassCompliantRules != NULL);
	require(LookupClassCompliantRules(secondaryScopeClassCompliantRules->GetClass()->GetName()) ==
		secondaryScopeClassCompliantRules);
	require(templateConstructedRule != NULL);
	require(templateConstructedRule->GetName() == selectionRule->GetName());
	require(templateConstructedRule->GetOperandNumber() == selectionRule->GetOperandNumber());
	require(dRuleCost >= 0);
	require(nDepth >= 0);
	require(dRandomDrawingNumber > 0);
	require(oaAllConstructedRules != NULL);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Collecte des operandes attributs et regles (recursivement) utilisables
	// des distribution  de probabilites a exploiter

	// Recherche de l'operande de selection
	templateSelectionOperand = templateConstructedRule->GetDerivationRule()->GetOperandAt(1);

	// Extraction des attributs de la classe compatibles avec le type en parametre
	ExtractMatchingAttributes(selectionRule, templateSelectionOperand, secondaryScopeClassCompliantRules,
				  &oaMatchingAttributes);

	// Extraction des regles applicables dont le code retour est compatible avec le type en parametre
	ExtractMatchingRules(selectionRule, templateSelectionOperand, secondaryScopeClassCompliantRules,
			     &oaMatchingConstructionRules);

	// Calcul des couts de regularisation par operande
	nMatchingAttributeNumber = oaMatchingAttributes.GetSize();
	nMatchingRuleNumber = oaMatchingConstructionRules.GetSize();
	dAttributeOperandCost = 0;
	dRuleChoiceOperandCost = 0;
	if (nMatchingRuleNumber == 0)
	{
		if (nMatchingAttributeNumber > 0)
			dAttributeOperandCost = log(1.0 * nMatchingAttributeNumber);
	}
	else
	{
		dAttributeOperandCost = log(1.0 * nMatchingAttributeNumber + 1.0);
		dRuleChoiceOperandCost = dAttributeOperandCost;
	}

	// Calcul de la repartition des tirages dans un noeud de l'arbre des prior en fonction du nombre d'attributs et
	// de regles
	DispatchAttributeRandomDrawingNumbers(dRandomDrawingNumber, oaMatchingAttributes.GetSize(),
					      oaMatchingConstructionRules.GetSize(), &dvAttributeRandomDrawingNumbers,
					      dAllRulesRandomDrawingNumber);

	// Calcul des probabilites et des tirages associee aux regles de construction
	if (dAllRulesRandomDrawingNumber > 0 and oaMatchingConstructionRules.GetSize() > 0)
		ComputeConstructionRuleProbs(dAllRulesRandomDrawingNumber, &oaMatchingConstructionRules,
					     &dvMatchingConstructionRuleProbs, &dvConstructionRuleRandomDrawingNumbers);

	// Affichage des caracteristiques locale du noeud de l'arbre du prior en mode trace
	if (sPriorTreeNodeName != "")
	{
		cout << "BuildAllSelectionRulesFromSelectionOperand" << endl;
		cout << "\tSeed\t" << GetRandomSeed() << endl;
		cout << "\tPrior node\t" << sPriorTreeNodeName << endl;
		cout << "\tDrawing number\t" << dRandomDrawingNumber << endl;
		cout << "\tClass\t" << classCompliantRules->GetClassName() << endl;
		cout << "\tRule cost\t" << dRuleCost << endl;
		cout << "\tConstruction rule\t" << selectionRule->GetName() << endl;
		cout << "\tAttribute operand cost\t" << dAttributeOperandCost << endl;
		cout << "\tRule choice operand cost\t" << dRuleChoiceOperandCost << endl;
		cout << "\tSelection table\t";
		templateConstructedRule->WriteOperandAt(0, cout);
		cout << endl;
		cout << "\tAttributes\t" << oaMatchingAttributes.GetSize() << endl;
		for (nAttribute = 0; nAttribute < oaMatchingAttributes.GetSize(); nAttribute++)
		{
			matchingAttribute = cast(KWAttribute*, oaMatchingAttributes.GetAt(nAttribute));
			cout << "\t\t" << matchingAttribute->GetName() << "\t"
			     << dvAttributeRandomDrawingNumbers.GetAt(nAttribute) << endl;
		}
		cout << "\tRules\t" << oaMatchingConstructionRules.GetSize() << endl;
		if (dAllRulesRandomDrawingNumber > 0)
		{
			for (nRule = 0; nRule < oaMatchingConstructionRules.GetSize(); nRule++)
			{
				matchingConstructionRule =
				    cast(KDConstructionRule*, oaMatchingConstructionRules.GetAt(nRule));
				cout << "\t\t" << matchingConstructionRule->GetName() << "\t"
				     << dvMatchingConstructionRuleProbs.GetAt(nRule) << "\t"
				     << dvConstructionRuleRandomDrawingNumbers.GetAt(nRule) << endl;
			}
		}
	}

	// Parcours des operandes regles compatibles pour identifier les regles utilisables
	if (dAllRulesRandomDrawingNumber > 0)
	{
		for (nRule = 0; nRule < oaMatchingConstructionRules.GetSize(); nRule++)
		{
			matchingConstructionRule = cast(KDConstructionRule*, oaMatchingConstructionRules.GetAt(nRule));

			// Prise en compte du cout complet du choix de la regle
			dRuleOperandCost = dRuleChoiceOperandCost - log(dvMatchingConstructionRuleProbs.GetAt(nRule));

			// On ne traite la regle que s'il reste des tirages
			dChildRandomDrawingNumber = dvConstructionRuleRandomDrawingNumbers.GetAt(nRule);
			if (dChildRandomDrawingNumber == 0)
				continue;

			// Construction de toutes les regles utilisable comme operande
			oaAllOperandConstructedRules.SetSize(0);
			if (sPriorTreeNodeName != "")
				sChildNodeName =
				    "[" + sPriorTreeNodeName + "]." + matchingConstructionRule->GetName() + "(";
			BuildAllConstructedRules(matchingConstructionRule, secondaryScopeClassCompliantRules,
						 sChildNodeName, nDepth + 1, 0 + dRuleOperandCost,
						 dChildRandomDrawingNumber, &oaAllOperandConstructedRules);

			// On ne traite la regle que si l'on a pu construire des operandes effectivement
			// Il peut n'y avoir aucun operande construit si par exemple on a atteint une limite (cout max,
			// profondeur max...)
			if (oaAllOperandConstructedRules.GetSize() == 0)
				continue;

			// Repartition des tirages par regle utilisable en operande
			DispatchConstructedRuleRandomDrawingNumbers(
			    dChildRandomDrawingNumber, &oaAllOperandConstructedRules, &dvOperandRandomDrawingNumbers);

			// Parcours des regles possibles
			for (i = 0; i < oaAllOperandConstructedRules.GetSize(); i++)
			{
				operandConstructedRule =
				    cast(KDConstructedRule*, oaAllOperandConstructedRules.GetAt(i));
				oaAllOperandConstructedRules.SetAt(i, NULL);

				// On ne traite la regle comme operande que s'il reste des tirages
				dOperandRandomDrawingNumber = dvOperandRandomDrawingNumbers.GetAt(i);
				if (dOperandRandomDrawingNumber == 0)
				{
					// Destruction de la regle courante uniquement (en positionnant l'index du
					// tableau a NULL)
					delete operandConstructedRule;
					oaAllOperandConstructedRules.SetAt(i, NULL);

					// Arret pour cette regle uniquement
					continue;
				}

				// Memorisation de la regle comme une regle de selection possible
				oaAllOperandSelectionRules.Add(operandConstructedRule);
			}
		}

		// Affichage des caracteristiques locale du noeud de l'arbre du prior en mode trace, pour les regles
		// construites
		if (sPriorTreeNodeName != "")
		{
			cout << "BuildAllSelectionRulesFromSelectionOperand\tRules" << endl;
			cout << "\tSeed\t" << GetRandomSeed() << endl;
			cout << "\tPrior node\t" << sPriorTreeNodeName << endl;
			cout << "\tDrawing number\t" << dRandomDrawingNumber << endl;
			cout << "\tClass\t" << classCompliantRules->GetClassName() << endl;
			cout << "\tRule cost\t" << dRuleCost << endl;
			cout << "\tConstruction rule\t" << selectionRule->GetName() << endl;
			cout << "\tSelection table\t";
			templateConstructedRule->WriteOperandAt(0, cout);
			cout << endl;
			cout << "\tSelection rules\t" << oaAllOperandSelectionRules.GetSize() << endl;
			for (i = 0; i < oaAllOperandSelectionRules.GetSize(); i++)
			{
				operandConstructedRule = cast(KDConstructedRule*, oaAllOperandSelectionRules.GetAt(i));
				cout << "\t\t" << operandConstructedRule->GetCost();
				cout << "\t" << *operandConstructedRule;
				cout << endl;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Generation des operandes de selection

	// Creation si necessaire de statistiques de selection pour la classe en cours
	classSelectionStats =
	    selectionOperandAnalyser->LookupClassSelectionStats(secondaryScopeClassCompliantRules->GetClassName());
	if (classSelectionStats == NULL)
		classSelectionStats =
		    selectionOperandAnalyser->AddClassSelectionStats(secondaryScopeClassCompliantRules);

	// Collecte dans un seul ensemble des operandes soit attributs, soit regles
	// Calcul egalement des probabilites par operande potentiel de selection, attribut ou regle
	// L'utilisation d'operandes permet un traitement generique
	for (nAttribute = 0; nAttribute < oaMatchingAttributes.GetSize(); nAttribute++)
	{
		matchingAttribute = cast(KWAttribute*, oaMatchingAttributes.GetAt(nAttribute));

		// On ne traite pas l'attribut s'il est redondant
		if (secondaryScopeClassCompliantRules->IsAttributeRedundant(matchingAttribute->GetName()))
			continue;

		// Recherche de la dimension de partition dans la classe de stats associee
		classSelectionOperandStats =
		    classSelectionStats->SearchAttributeClassSelectionOperandStats(matchingAttribute);
		assert(classSelectionOperandStats != NULL or not selectionOperandAnalyser->IsStatsComputed());

		// Ajout si necessaire de l'operande de selection associe dans sa classe de stats
		if (classSelectionOperandStats == NULL)
			classSelectionOperandStats =
			    classSelectionStats->AddAttributeClassSelectionOperandStats(matchingAttribute);

		// Memorisation de l'operande et de sa proba, systematiquement dans la premiere passe,
		// seulement si utile dans la deuxieme passe
		dSelectionOperandProb = 0;
		if (not selectionOperandAnalyser->IsStatsComputed() or
		    classSelectionOperandStats->GetGranularityNumber() > 0)
		{
			// Cout de selection dans le cas d'un attribut
			dSelectionOperandProb = exp(-dAttributeOperandCost);
		}

		// Ajout de l'operande et de sa proba, si proba suffisante
		if (dSelectionOperandProb > DBL_MIN)
		{
			// Memorisation de l'operande de selection
			oaAllUnivariateSelectionOperands.Add(classSelectionOperandStats);
			dvAllUnivariateSelectionOperandProbs.Add(dSelectionOperandProb);
		}
	}
	for (i = 0; i < oaAllOperandSelectionRules.GetSize(); i++)
	{
		operandConstructedRule = cast(KDConstructedRule*, oaAllOperandSelectionRules.GetAt(i));
		dOperandConstructedRule = operandConstructedRule->GetCost();

		// Recherche de la dimension de partition dans la classe de stats associee
		classSelectionOperandStats =
		    classSelectionStats->SearchRuleClassSelectionOperandStats(operandConstructedRule);

		// Si non trouve, recherche une dimension de partition de type attribut, mais derive avec une regle de
		// derivation correspond a la regle construite On ne doit alors prendre en compte cette regle, deja
		// prise en compte dans un attribut derive existant
		if (classSelectionOperandStats == NULL)
		{
			classSelectionOperandStats =
			    classSelectionStats->SearchAttributeClassSelectionOperandStatsFromRule(
				operandConstructedRule);

			// Si existant et utilise, on ne doit pas prendre en compte la regle
			// Si l'attribut etait non utilise, on ignore ce test: l'attribut non utilise sera exploite dans
			// la phase de construction des regles de derivation, qui s'appuiront sur les attribut
			// disponibles, utilises ou non
			if (classSelectionOperandStats != NULL and
			    classSelectionOperandStats->GetPartitionDimension()->GetAttribute()->GetUsed())
			{
				// Destruction de la regle
				delete operandConstructedRule;

				// On passe a l'operande suivant
				continue;
			}
		}

		// Ajout si necessaire de l'operande de selection associe dans sa classe de stats
		if (classSelectionOperandStats == NULL)
			classSelectionOperandStats =
			    classSelectionStats->AddRuleClassSelectionOperandStats(operandConstructedRule);
		// Destruction de la regle sinon
		else
			delete operandConstructedRule;

		// Memorisation de l'operande et de sa proba, systematiquement dans la premiere passe,
		// seulement si utile dans la deuxieme passe
		dSelectionOperandProb = 0;
		if (not selectionOperandAnalyser->IsStatsComputed() or
		    classSelectionOperandStats->GetGranularityNumber() > 0)
		{
			// Cout de l'operande de selection dans le cas d'une regle
			dSelectionOperandProb = exp(-dOperandConstructedRule);
		}

		// Ajout de l'operande et de sa proba, si proba suffisante
		if (dSelectionOperandProb > DBL_MIN)
		{
			// Memorisation de l'operande de selection
			oaAllUnivariateSelectionOperands.Add(classSelectionOperandStats);
			dvAllUnivariateSelectionOperandProbs.Add(dSelectionOperandProb);
		}
	}
	oaAllOperandSelectionRules.SetSize(0);

	// Tri des operandes de selection, pour garantir que toutes les partitions seront definies sur des dimensions
	// dans le meme ordre afin de garantir l'unicite des partitions, quelles que soient leurs dimensions
	SortClassSelectionOperandStatsAndProbs(&oaAllUnivariateSelectionOperands,
					       &dvAllUnivariateSelectionOperandProbs);

	// Calcul de la repartition des tirages concernant le nombre d'operandes de selection
	if (oaAllUnivariateSelectionOperands.GetSize() > 0)
		sampleGenerator.ComputeBestNaturalNumbersUniversalPriorSample(
		    dRandomDrawingNumber, oaAllUnivariateSelectionOperands.GetSize(),
		    &dvSelectionSizeRandomDrawingNumbers);

	// Affichage des caracteristiques locale du noeud de l'arbre du prior en mode trace, pour les regles selection
	if (sPriorTreeNodeName != "")
	{
		cout << "BuildAllSelectionRulesFromSelectionOperand\tSelection" << endl;
		cout << "\tSeed\t" << GetRandomSeed() << endl;
		cout << "\tPrior node\t" << sPriorTreeNodeName << endl;
		cout << "\tSelection operands\t" << oaAllUnivariateSelectionOperands.GetSize() << endl;
		for (i = 0; i < oaAllUnivariateSelectionOperands.GetSize(); i++)
		{
			classSelectionOperandStats =
			    cast(KDClassSelectionOperandStats*, oaAllUnivariateSelectionOperands.GetAt(i));
			cout << "\t" << dvSelectionSizeRandomDrawingNumbers.GetAt(i);
			cout << "\t" << dvAllUnivariateSelectionOperandProbs.GetAt(i);
			cout << "\t" << classSelectionOperandStats->GetObjectLabel() << endl;
		}
	}
	// Parcours des tailles de selection
	bRuleFiltered = false;
	for (nSize = 1; nSize <= dvSelectionSizeRandomDrawingNumbers.GetSize(); nSize++)
	{
		dSelectionRandomDrawingNumber = dvSelectionSizeRandomDrawingNumbers.GetAt(nSize - 1);

		// Traitement si necessaire
		nInitialRuleNumber = oaAllConstructedRules->GetSize();
		if (dSelectionRandomDrawingNumber > 0)
		{
			// Calcul des selections de variables pour la taille de selection
			sampleGenerator.ComputeBestSelectionSample(dSelectionRandomDrawingNumber, nSize,
								   &dvAllUnivariateSelectionOperandProbs,
								   &oaSelectionOperandIndexedFrequencies);

			// Calcul de toutes les regles de selection possibles pour une taille de selection donnee
			BuildAllSelectionParts(secondaryScopeClassCompliantRules, dSelectionRandomDrawingNumber, nSize,
					       nMaxRuleNumber - oaAllConstructedRules->GetSize(), dMaxRuleCost,
					       sPriorTreeNodeName, &oaAllUnivariateSelectionOperands,
					       &oaSelectionOperandIndexedFrequencies, &oaAllSelectionParts);

			// Construction des regles avec les operandes de selection
			for (i = 0; i < oaAllSelectionParts.GetSize(); i++)
			{
				completedSelectionPart = cast(KDConstructedPart*, oaAllSelectionParts.GetAt(i));

				// Duplication de la regle de reference
				constructedRule = templateConstructedRule->Clone();

				// Parametrage de son operande de selection avec la regle de selection
				constructedRule->SetPartOperandAt(1, completedSelectionPart);

				// Ajout du cout de la regle en operande
				constructedRule->SetCost(dRuleCost + completedSelectionPart->GetCost());

				// Memorisation de la regle construite
				oaAllConstructedRules->Add(constructedRule);
			}
			oaAllSelectionParts.SetSize(0);

			// Filtrage des regles pour ne garder que les regles de probabilite suffisantes pour etre tirees
			bRuleFiltered =
			    FilterConstructedRulesForRandomDrawing(dRandomDrawingNumber, oaAllConstructedRules);

			// Nettoyage des selections d'operandes
			oaSelectionOperandIndexedFrequencies.DeleteAll();
		}

		// Arret si inutile d'aller plus loin
		if (oaAllConstructedRules->GetSize() >= nMaxRuleNumber or
		    oaAllConstructedRules->GetSize() == nInitialRuleNumber)
			break;
	}
}

void KDMultiTableFeatureConstruction::BuildAllSelectionParts(
    const KDClassCompliantRules* secondaryScopeClassCompliantRules, double dSelectionRandomDrawingNumber,
    int nSelectionSize, int nMaxSelectionOperandNumber, double dMaxSelectionCost, const ALString& sPriorTreeNodeName,
    const ObjectArray* oaSelectionOperands, const ObjectArray* oaSelectionOperandIndexedFrequencies,
    ObjectArray* oaAllSelectionParts) const
{
	KDMultinomialSampleGenerator sampleGenerator;
	KDIndexedFrequency* selectionOperandIndexFrequency;
	int i;
	int nIndex;
	int nOperand;
	KDClassSelectionStats* classSelectionStats;
	const KDClassSelectionOperandStats* classSelectionOperandStats;
	const KDConstructedPartitionDimension* partitionDimension;
	ObjectArray oaSelectionValueArrays;
	ObjectArray oaSelectionValueProbVectors;
	ObjectArray* oaSelectionValues;
	DoubleVector* dvSelectionValueProbs;
	KDSelectionSpec* selectionValue;
	int nPart;
	ObjectArray oaCompleteSelectionValueIndexedFrequencies;
	int nCompleteOperand;
	KDIndexedFrequency* completeOperandIndexFrequency;
	// On passe par un objet static pour des raison d'optimisation, pour eviter des reallocations inutiles
	static KDConstructedPartition searchedSelectionPartition;
	KDConstructedPartition* foundSelectionPartition;
	IntVector searchedSelectionPartIndexes;
	KDConstructedPart* foundSelectionPart;
	double dSizeCost;
	double dOperandsCost;
	double dPartilesCost;
	double dTotalCost;

	require(secondaryScopeClassCompliantRules != NULL);
	require(dSelectionRandomDrawingNumber > 0);
	require(nSelectionSize > 0);
	require(nMaxSelectionOperandNumber >= 0);
	require(dMaxSelectionCost >= 0);
	// require(dRuleCost >= 0);
	require(oaSelectionOperands != NULL);
	require(oaSelectionOperands->GetSize() > 0);
	require(oaSelectionOperandIndexedFrequencies != NULL);
	require(oaAllSelectionParts != NULL);
	require(oaAllSelectionParts->GetSize() == 0);

	// Recherche des statistique de selection de valeur pour la classe en cours
	classSelectionStats =
	    selectionOperandAnalyser->LookupClassSelectionStats(secondaryScopeClassCompliantRules->GetClassName());
	check(classSelectionStats);

	// Initialisation des tableaux de gestion des valeurs de selection
	oaSelectionValueArrays.SetSize(nSelectionSize);
	oaSelectionValueProbVectors.SetSize(nSelectionSize);
	for (nIndex = 0; nIndex < nSelectionSize; nIndex++)
	{
		oaSelectionValueArrays.SetAt(nIndex, new ObjectArray);
		oaSelectionValueProbVectors.SetAt(nIndex, new DoubleVector);
	}

	// Cout lie a la taille de la selection
	dSizeCost = KWStat::NaturalNumbersUniversalCodeLength(nSelectionSize);

	// Parcours des selections d'operandes
	for (i = 0; i < oaSelectionOperandIndexedFrequencies->GetSize(); i++)
	{
		selectionOperandIndexFrequency =
		    cast(KDIndexedFrequency*, oaSelectionOperandIndexedFrequencies->GetAt(i));
		assert(selectionOperandIndexFrequency->GetIndexSize() == nSelectionSize);

		// Traitement si necessaire
		if (selectionOperandIndexFrequency->GetFrequency() > 0)
		{
			// Cout lie a la selection des operandes
			dOperandsCost = -log(selectionOperandIndexFrequency->GetProb());

			// Parcours des operandes de selection pour creer les probas de valeurs de selection par
			// operande
			for (nIndex = 0; nIndex < nSelectionSize; nIndex++)
			{
				nOperand = selectionOperandIndexFrequency->GetIndexAt(nIndex);

				// Recherche des statistique de selection de valeur pour la dimension en cours
				classSelectionOperandStats =
				    cast(KDClassSelectionOperandStats*, oaSelectionOperands->GetAt(nOperand));

				// Acces a la dimension de partition
				partitionDimension = classSelectionOperandStats->GetPartitionDimension();

				// Calcul des valeurs de selection pour cet operande, si disponible
				oaSelectionValues = cast(ObjectArray*, oaSelectionValueArrays.GetAt(nIndex));
				ComputeSelectionValues(classSelectionOperandStats,
						       selectionOperandIndexFrequency->GetFrequency(),
						       partitionDimension, oaSelectionValues);

				// Memorisation des probas par valeur de selection
				dvSelectionValueProbs = cast(DoubleVector*, oaSelectionValueProbVectors.GetAt(nIndex));
				dvSelectionValueProbs->SetSize(oaSelectionValues->GetSize());
				for (nPart = 0; nPart < oaSelectionValues->GetSize(); nPart++)
				{
					selectionValue = cast(KDSelectionSpec*, oaSelectionValues->GetAt(nPart));
					dvSelectionValueProbs->SetAt(nPart, exp(-selectionValue->GetCost()));
				}
			}

			// Calcul de toutes les selections d'operandes en prenant en compte les valeurs de selection
			sampleGenerator.ComputeBestMultipleProductSample(selectionOperandIndexFrequency->GetFrequency(),
									 &oaSelectionValueProbVectors,
									 &oaCompleteSelectionValueIndexedFrequencies);

			// Parcours de toutes les selections a construire effectivement
			for (nCompleteOperand = 0;
			     nCompleteOperand < oaCompleteSelectionValueIndexedFrequencies.GetSize();
			     nCompleteOperand++)
			{
				completeOperandIndexFrequency =
				    cast(KDIndexedFrequency*,
					 oaCompleteSelectionValueIndexedFrequencies.GetAt(nCompleteOperand));
				assert(completeOperandIndexFrequency->GetIndexSize() == nSelectionSize);

				// Construction d'une operande complet de selection si effectif suffisant
				if (completeOperandIndexFrequency->GetFrequency() > 0)
				{
					// Creation d'un objet partition correspondant a la selection
					searchedSelectionPartition.SetPartitionClass(
					    secondaryScopeClassCompliantRules->GetClass());
					searchedSelectionPartition.SetDimensionNumber(nSelectionSize);

					// Parametrage du vecteur d'index de la partie
					searchedSelectionPartIndexes.SetSize(nSelectionSize);

					// Parcours des operandes de la selection pour initialiser la partition et la
					// partie
					dPartilesCost = 0;
					for (nIndex = 0; nIndex < nSelectionSize; nIndex++)
					{
						nOperand = selectionOperandIndexFrequency->GetIndexAt(nIndex);

						// Recherche des statistique de selection de valeur pour la dimension en
						// cours
						classSelectionOperandStats = cast(KDClassSelectionOperandStats*,
										  oaSelectionOperands->GetAt(nOperand));

						// Acces a la dimension de partition
						partitionDimension =
						    classSelectionOperandStats->GetPartitionDimension();

						// Acces aux valeurs pour cet operande
						oaSelectionValues =
						    cast(ObjectArray*, oaSelectionValueArrays.GetAt(nIndex));

						// Acces au partile
						nPart = completeOperandIndexFrequency->GetIndexAt(nIndex);
						selectionValue =
						    cast(KDSelectionSpec*, oaSelectionValues->GetAt(nPart));

						// Parametrage de la partition
						searchedSelectionPartition.SetGranularityAt(
						    nIndex, selectionValue->GetGranularity());
						searchedSelectionPartition.SetDimensionAt(nIndex, partitionDimension);

						// Creation d'une regle univariee pour l'operande et son partile,
						// "conceptuel" en premiere passe et "reel" en deuxieme passe
						searchedSelectionPartIndexes.SetAt(nIndex,
										   selectionValue->GetPartIndex());
						dPartilesCost += selectionValue->GetCost();
					}

					// Parametrage du data path
					searchedSelectionPartition.SetTableAttribute(DataPathGetHeadTableAttribute());

					// Memorisation du cout total
					dTotalCost = dSizeCost + dOperandsCost + dPartilesCost;

					// Recherche si la partition existe deja, et memorisation si necessaire
					foundSelectionPartition =
					    classSelectionStats->LookupPartition(&searchedSelectionPartition);
					if (foundSelectionPartition == NULL)
					{
						// Creation d'une nouvelle partition sur la base de la partition
						// recherchee
						foundSelectionPartition = new KDConstructedPartition;
						foundSelectionPartition->SetPartitionClass(
						    searchedSelectionPartition.GetPartitionClass());
						foundSelectionPartition->SetTableAttribute(
						    searchedSelectionPartition.GetTableAttribute());
						foundSelectionPartition->SetDimensionNumber(
						    searchedSelectionPartition.GetDimensionNumber());
						for (nIndex = 0;
						     nIndex < searchedSelectionPartition.GetDimensionNumber(); nIndex++)
						{
							foundSelectionPartition->SetGranularityAt(
							    nIndex,
							    searchedSelectionPartition.GetGranularityAt(nIndex));
							foundSelectionPartition->SetDimensionAt(
							    nIndex, searchedSelectionPartition.GetDimensionAt(nIndex));
						}

						// Ajout de cette partition
						classSelectionStats->AddPartition(foundSelectionPartition);
					}
					assert(foundSelectionPartition ==
					       classSelectionStats->LookupPartition(&searchedSelectionPartition));

					// Recherche si la partie existe deja, et memorisation si necessaire
					foundSelectionPart =
					    foundSelectionPartition->LookupPart(&searchedSelectionPartIndexes);
					if (foundSelectionPart == NULL)
					{
						foundSelectionPart =
						    foundSelectionPartition->AddPart(&searchedSelectionPartIndexes);
						foundSelectionPart->SetCost(dTotalCost);
					}

					// Enregistrement de la partie de selection
					oaAllSelectionParts->Add(foundSelectionPart);

					// Affichage
					if (sPriorTreeNodeName != "")
					{
						cout << "BuildAllSelectionParts" << endl;
						cout << "\tSeed\t" << GetRandomSeed() << endl;
						cout << "\tPrior node\t" << sPriorTreeNodeName << endl;
						cout << "\tDrawing number\t" << dSelectionRandomDrawingNumber << endl;
						cout << "\tSelection\t" << nSelectionSize << "\t"
						     << foundSelectionPart->GetCost() << "\t";
						cout << "(" << dTotalCost << ")\t";
						cout << "\t" << *foundSelectionPart << endl;

						// Details
						cout << "\tSize cost\t" << dSizeCost << endl;
						cout << "\tOperands cost\t" << dOperandsCost << endl;
						cout << "\tPartiles cost\t" << dPartilesCost << endl;

						// Operandes de selection
						cout << "\tSelection operands" << endl;
						cout << "\t\tPart cost\tGranularity\tPartile\tOperand" << endl;
						for (nIndex = 0; nIndex < nSelectionSize; nIndex++)
						{
							nOperand = selectionOperandIndexFrequency->GetIndexAt(nIndex);

							// Recherche des statistique de selection de valeur pour la
							// dimension en cours
							classSelectionOperandStats =
							    cast(KDClassSelectionOperandStats*,
								 oaSelectionOperands->GetAt(nOperand));

							// Acces a la dimension de partition
							partitionDimension =
							    classSelectionOperandStats->GetPartitionDimension();

							// Acces au partile
							oaSelectionValues =
							    cast(ObjectArray*, oaSelectionValueArrays.GetAt(nIndex));
							nPart = completeOperandIndexFrequency->GetIndexAt(nIndex);
							selectionValue =
							    cast(KDSelectionSpec*, oaSelectionValues->GetAt(nPart));

							// Affichage
							cout << "\t\t" << selectionValue->GetCost() << "\t"
							     << selectionValue->GetGranularity() << "\t"
							     << selectionValue->GetPartIndex() << "\t";
							cout << *partitionDimension;
							cout << endl;
						}
					}
					assert(fabs(dTotalCost - foundSelectionPart->GetCost()) < 1e-5);
				}
			}

			// Nettoyage
			oaCompleteSelectionValueIndexedFrequencies.DeleteAll();
			for (nIndex = 0; nIndex < nSelectionSize; nIndex++)
			{
				oaSelectionValues = cast(ObjectArray*, oaSelectionValueArrays.GetAt(nIndex));
				oaSelectionValues->DeleteAll();
			}
		}
	}

	// Nettoyage final
	oaSelectionValueArrays.DeleteAll();
	oaSelectionValueProbVectors.DeleteAll();
}

void KDMultiTableFeatureConstruction::ComputeSelectionValues(
    const KDClassSelectionOperandStats* classSelectionOperandStats, double dRandomDrawingNumber,
    const KDConstructedPartitionDimension* partitionDimension, ObjectArray* oaSelectionValues) const
{
	const int nMaxGranularity = 16384;
	KDMultinomialSampleGenerator sampleGenerator;
	KDSelectionSpec* selectionValue;
	DoubleVector dvGranularityProbs;
	IntVector ivGranularities;
	DoubleVector dvGranularityFrequencies;
	DoubleVector dvPartileFrequencies;
	int nGranularity;
	int nGranularityExponent;
	double dGranularityFrequency;
	int i;
	int j;
	int nPart;
	const ObjectArray* oaSelectionParts;
	KDSelectionPart* selectionPart;

	require(classSelectionOperandStats != NULL);
	require(dRandomDrawingNumber > 0);
	require(partitionDimension != NULL);
	require(oaSelectionValues != NULL);
	require(oaSelectionValues->GetSize() == 0);

	// Cas de la premiere passe: generation d'operandes de selection "conceptuels"
	if (not selectionOperandAnalyser->IsStatsComputed())
	{
		// Generation des probabilites des granularites par puissance de deux
		nGranularity = 2;
		nGranularityExponent = 1;
		while (nGranularity <= nMaxGranularity)
		{
			ivGranularities.Add(nGranularity);
			dvGranularityProbs.Add(exp(-KWStat::NaturalNumbersUniversalCodeLength(nGranularityExponent)));
			nGranularity *= 2;
			nGranularityExponent++;

			// Il n'est pas utile d'aller plus loin si la granularite depasse le nombre de tirage demandes
			if (nGranularity >= dRandomDrawingNumber)
				break;
		}

		// Generation des effectifs par granularite
		sampleGenerator.ComputeBestSample(dRandomDrawingNumber, &dvGranularityProbs, &dvGranularityFrequencies);

		// Generation des partiles par granularite
		for (i = 0; i < dvGranularityFrequencies.GetSize(); i++)
		{
			nGranularity = ivGranularities.GetAt(i);
			dGranularityFrequency = dvGranularityFrequencies.GetAt(i);

			// Traitement uniquement si necessaire
			if (dGranularityFrequency > 0)
			{
				// Calcul des effectifs par partile
				sampleGenerator.ComputeBestEquidistributedSample(dGranularityFrequency, nGranularity,
										 &dvPartileFrequencies);

				// Memorisation des partiles utiles
				for (nPart = 0; nPart < dvPartileFrequencies.GetSize(); nPart++)
				{
					if (dvPartileFrequencies.GetAt(nPart) > 0)
					{
						selectionValue = new KDSelectionSpec;
						selectionValue->SetGranularity(nGranularity);
						selectionValue->SetPartIndex(nPart);
						oaSelectionValues->Add(selectionValue);
					}
				}
			}
		}
	}
	// Cas de la deuxieme passe: construction d'operandes de construction exploitant les valeurs de la base
	else
	{
		// Generation des probabilites des granularites disponibles
		for (i = 0; i < classSelectionOperandStats->GetGranularityNumber(); i++)
		{
			nGranularity = classSelectionOperandStats->GetGranularityAt(i);
			ivGranularities.Add(nGranularity);
			nGranularityExponent = classSelectionOperandStats->GetGranularityExponentAt(i);
			dvGranularityProbs.Add(exp(-KWStat::NaturalNumbersUniversalCodeLength(nGranularityExponent)));
		}

		// Generation des effectifs par granularite
		sampleGenerator.ComputeBestSample(dRandomDrawingNumber, &dvGranularityProbs, &dvGranularityFrequencies);

		// Generation des partiles par granularite
		for (i = 0; i < dvGranularityFrequencies.GetSize(); i++)
		{
			nGranularity = ivGranularities.GetAt(i);
			dGranularityFrequency = dvGranularityFrequencies.GetAt(i);

			// Calcul des effectifs par partile disponible
			oaSelectionParts = classSelectionOperandStats->GetPartsAt(i);
			sampleGenerator.ComputeBestEquidistributedSample(
			    dGranularityFrequency, oaSelectionParts->GetSize(), &dvPartileFrequencies);

			// Parcours des parties disponibles
			for (j = 0; j < oaSelectionParts->GetSize(); j++)
			{
				selectionPart = cast(KDSelectionPart*, oaSelectionParts->GetAt(j));
				nPart = selectionPart->GetIndex();

				// Enregistrement si partie utile
				if (dvPartileFrequencies.GetAt(j) > 0)
				{
					selectionValue = new KDSelectionSpec;
					selectionValue->SetGranularity(nGranularity);
					selectionValue->SetPartIndex(nPart);
					oaSelectionValues->Add(selectionValue);
				}
			}
		}
	}
}

boolean KDMultiTableFeatureConstruction::IsSelectionRuleForbidden() const
{
	return bIsSelectionRuleForbidden;
}

void KDMultiTableFeatureConstruction::SetSelectionRuleForbidden(boolean bValue) const
{
	bIsSelectionRuleForbidden = bValue;
}

int KWSortableObjectCompareClassSelectionOperand(const void* elem1, const void* elem2)
{
	KWSortableObject* sortableObject1;
	KWSortableObject* sortableObject2;
	KDClassSelectionOperandStats* selectionOperandStast1;
	KDClassSelectionOperandStats* selectionOperandStast2;

	// Acces aux objets a comparer
	sortableObject1 = cast(KWSortableObject*, *(Object**)elem1);
	sortableObject2 = cast(KWSortableObject*, *(Object**)elem2);
	selectionOperandStast1 = cast(KDClassSelectionOperandStats*, sortableObject1->GetSortValue());
	selectionOperandStast2 = cast(KDClassSelectionOperandStats*, sortableObject2->GetSortValue());

	// Comparaison de leur operande
	return selectionOperandStast1->GetPartitionDimension()->Compare(
	    selectionOperandStast2->GetPartitionDimension());
}

void KDMultiTableFeatureConstruction::SortClassSelectionOperandStatsAndProbs(
    ObjectArray* oaClassSelectionOperandStats, DoubleVector* dvClassSelectionOperand) const
{
	ObjectArray oaSortableObjects;
	KWSortableObject* sortableObject;
	ObjectArray oaClassSelectionOperandStatsCopy;
	DoubleVector dvClassSelectionOperandCopy;
	int i;

	require(oaClassSelectionOperandStats != NULL);
	require(dvClassSelectionOperand != NULL);
	require(oaClassSelectionOperandStats->GetSize() == dvClassSelectionOperand->GetSize());

	// Initialisation d'un tableau de d'objet a trier
	for (i = 0; i < oaClassSelectionOperandStats->GetSize(); i++)
	{
		sortableObject = new KWSortableObject;
		sortableObject->SetIndex(i);
		sortableObject->SetSortValue(oaClassSelectionOperandStats->GetAt(i));
		oaSortableObjects.Add(sortableObject);
	}

	// Tri des objets
	oaSortableObjects.SetCompareFunction(KWSortableObjectCompareClassSelectionOperand);
	oaSortableObjects.Sort();

	// Collecte des resultats, apres avoir duplique les tableaux et vecteur originaux
	oaClassSelectionOperandStatsCopy.CopyFrom(oaClassSelectionOperandStats);
	dvClassSelectionOperandCopy.CopyFrom(dvClassSelectionOperand);
	for (i = 0; i < oaSortableObjects.GetSize(); i++)
	{
		sortableObject = cast(KWSortableObject*, oaSortableObjects.GetAt(i));
		oaClassSelectionOperandStats->SetAt(i,
						    oaClassSelectionOperandStatsCopy.GetAt(sortableObject->GetIndex()));
		dvClassSelectionOperand->SetAt(i, dvClassSelectionOperandCopy.GetAt(sortableObject->GetIndex()));
	}

	// Nettoyage
	oaSortableObjects.DeleteAll();
}

///////////////////////////////////////////////////////////////////////////////
// Class KDSelectionSpec

KDSelectionSpec::KDSelectionSpec()
{
	nGranularity = 2;
	nPart = 0;
}

KDSelectionSpec::~KDSelectionSpec() {}

void KDSelectionSpec::SetGranularity(int nValue)
{
	require(KDClassSelectionOperandStats::CheckGranularity(nGranularity));
	nGranularity = nValue;
}

int KDSelectionSpec::GetGranularity() const
{
	return nGranularity;
}

void KDSelectionSpec::SetPartIndex(int nValue)
{
	require(nValue >= 0);
	nPart = nValue;
}

int KDSelectionSpec::GetPartIndex() const
{
	return nPart;
}

double KDSelectionSpec::GetCost() const
{
	KDMultinomialSampleGenerator sampleGenerator;
	double dCost;
	int nGranularityExponent;

	require(Check());

	nGranularityExponent = KDClassSelectionOperandStats::GetGranularityExponent(nGranularity);
	dCost = KWStat::NaturalNumbersUniversalCodeLength(nGranularityExponent);
	dCost += log(nGranularity * 1.0);
	return dCost;
}

void KDSelectionSpec::Write(ostream& ost) const
{
	ost << nGranularity << "\t" << nPart << "\t" << GetCost();
}

boolean KDSelectionSpec::Check() const
{
	boolean bOk = true;

	if (nGranularity < 2)
		bOk = false;
	if (nPart < 0 or nPart >= nGranularity)
		bOk = false;
	return bOk;
}