// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDTextFeatureConstruction.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KDTextFeatureConstruction

KDTextFeatureConstruction::KDTextFeatureConstruction()
{
	sTextFeatures = "ngrams";
	nAttributeNameIndex = 0;
}

KDTextFeatureConstruction::~KDTextFeatureConstruction() {}

void KDTextFeatureConstruction::SetTextFeatures(const ALString& sValue)
{
	require(KWTextTokenizer::CheckTextFeatures(sValue));
	sTextFeatures = sValue;
}

const ALString& KDTextFeatureConstruction::GetTextFeatures() const
{
	ensure(KWTextTokenizer::CheckTextFeatures(sTextFeatures));
	return sTextFeatures;
}

boolean KDTextFeatureConstruction::ContainsTextAttributes(const KWClass* kwcClass) const
{
	NumericKeyDictionary nkdExploredClasses;

	return InternalContainsTextAttributes(kwcClass, &nkdExploredClasses);
}

boolean KDTextFeatureConstruction::ConstructTextFeatures(KWClass* kwcClass, int nFeatureNumber,
							 ObjectDictionary* odConstructedAttributes) const
{
	boolean bOk = true;
	boolean bDisplay = false;
	ObjectDictionary odTextClasses;
	KDClassCompliantRules mainClassCompliantRules;
	int nConstructedVariableNumber;
	Timer timerConstruction;
	ALString sTmp;

	require(kwcClass != NULL);
	require(kwcClass->IsCompiled());
	require(nFeatureNumber >= 0);
	require(odConstructedAttributes != NULL);
	require(odConstructedAttributes->GetCount() == 0);

	// Demarrage du timer
	timerConstruction.Reset();
	timerConstruction.Start();

	// Collecte de tous les attributs derives existants
	// On utilise l'objet mainClassCompliantRules uniquement pour ses service des gestion des attributs derives
	// existants
	InitializeMainClassConstructedAttributesAndBlocks(&mainClassCompliantRules, kwcClass);
	if (bDisplay)
	{
		cout << "Existing text based attributes\n";
		mainClassCompliantRules.DisplayConstructedAttributes(cout);
		mainClassCompliantRules.DisplayConstructedAttributeBlocks(cout);
	}

	// Reinitialisation des index utilises pour nommer les variables
	ResetAttributeNameIndexes();

	// Collecte des attributs Text accessibles via tous les chemins de donnees possibles
	CollectAllTextClasses(kwcClass, &odTextClasses);

	// Calcul des couts de construction pour les chemins d'acces aux attribut de type Text d'une classe
	ComputeTextClassConstructionCosts(kwcClass, &odTextClasses);

	// Calcul des nombres de variables construites pour les chemins d'acces aux attribut de type Text d'une classe
	ComputeTextClassConstructedFeatureNumbers(kwcClass, &odTextClasses, nFeatureNumber);

	// Collecte des tokens
	bOk = ExtractTokenSamples(kwcClass, &odTextClasses);

	// Creation des blocks de variables de type texte selon les specifications par chemin d'attributs de la classe
	// principale
	if (bOk)
		ConstructAllTextAttributeBlocks(&mainClassCompliantRules, kwcClass, &odTextClasses,
						odConstructedAttributes);

	// Compilation du domaine
	if (bOk)
	{
		assert(kwcClass->GetDomain()->Check());
		kwcClass->GetDomain()->Compile();
	}

	// Nettoyage
	odTextClasses.DeleteAll();
	mainClassCompliantRules.CleanCollectedConstructedAttributesAndBlocks();

	// Message de fin
	timerConstruction.Stop();
	if (TaskProgression::IsInterruptionRequested())
	{
		AddSimpleMessage(sTmp + "Text feature construction interrupted after " +
				 SecondsToString(timerConstruction.GetElapsedTime()));
	}
	else if (bOk)
	{
		nConstructedVariableNumber = kwcClass->ComputeInitialAttributeNumber(GetTargetAttributeName() != "") -
					     GetLearningSpec()->GetInitialAttributeNumber();

		// Cas ou on construit toutes les variables demandees
		if (nConstructedVariableNumber == nFeatureNumber)
		{
			assert(nConstructedVariableNumber >= 0);
			AddSimpleMessage(sTmp + "Text feature construction time: " +
					 SecondsToString(timerConstruction.GetElapsedTime()) + " (" +
					 IntToString(nConstructedVariableNumber) + " " + GetTextFeatures() + ")");
		}
		// Cas ou il n'y a pas assez de tokens dan pour construire toutes les variables demandees: message
		// informatif
		//  - ce cas peu arriver avec un seul texte n'ayant pas assez de tokens: on a fait le maximum
		//  - dans le cas de plusieurs textes, il se peut qu'un des textes soit "sature",  au maximum de ses
		//  tokens presents
		//     - on aurait pu genere d'avantage de variables pour les textes non satures
		//     - traitement non retenu pour les raison suivantes:
		//        - complexe a traiter, car plusieurs passes sont potentiellement necessaires
		//        - potentiellement non scalable, car il faudrait demander a priori a collecter un grand nombre
		//        de tokens
		//          sur tous les textes pour etre sur d'en avoir assez
		//        - le cas de plusieurs textes, dont certains sont satures est a priori rare
		else
			AddSimpleMessage(sTmp + "Text feature construction time: " +
					 SecondsToString(timerConstruction.GetElapsedTime()) + " (" +
					 IntToString(nConstructedVariableNumber) + " " + GetTextFeatures() +
					 ", out of " + IntToString(nFeatureNumber) + " planned)");
	}

	ensure(odConstructedAttributes->GetCount() <= nFeatureNumber);
	return bOk;
}

boolean KDTextFeatureConstruction::InternalContainsTextAttributes(const KWClass* kwcClass,
								  NumericKeyDictionary* nkdExploredClasses) const
{
	KWAttribute* attribute;
	int i;

	require(kwcClass != NULL);
	require(kwcClass->IsIndexed());
	require(nkdExploredClasses != NULL);

	// Arret de l'exploration si classe deja prise en compte
	if (nkdExploredClasses->Lookup(kwcClass) != NULL)
		return false;
	// Sinon, enregistrement de la classe
	else
		nkdExploredClasses->SetAt(kwcClass, cast(Object*, kwcClass));

	// Parcours des attributs utilise pour detecter recursivement ceux de type Text
	for (i = 0; i < kwcClass->GetUsedAttributeNumber(); i++)
	{
		attribute = kwcClass->GetUsedAttributeAt(i);
		if (KWType::IsTextBased(attribute->GetType()))
			return true;
		else if (KWType::IsRelation(attribute->GetType()) and
			 InternalContainsTextAttributes(attribute->GetClass(), nkdExploredClasses))
			return true;
	}
	return false;
}

void KDTextFeatureConstruction::CollectAllTextClasses(const KWClass* kwcMainClass,
						      ObjectDictionary* odTextClasses) const
{
	boolean bDisplay = false;
	KDTextAttributePath emptyTextAttributePath;
	ObjectArray oaTextClasses;
	KDTextClass* textClass;
	int n;

	require(kwcMainClass != NULL);
	require(kwcMainClass->IsIndexed());
	require(odTextClasses != NULL);
	require(odTextClasses->GetCount() == 0);

	// Collecte recursive des attribut de type texte ramenes sur la classe principale
	InternalCollectAllTextClasses(kwcMainClass, kwcMainClass, &emptyTextAttributePath, odTextClasses);

	// Affichage des resultats
	if (bDisplay)
	{
		// Extraction dans un tableau trie
		odTextClasses->ExportObjectArray(&oaTextClasses);
		oaTextClasses.SetCompareFunction(KDTextClassCompareName);
		oaTextClasses.Sort();

		// Affichage des resultats
		cout << "CollectAllTextClasses\t" << kwcMainClass->GetName() << "\n";
		for (n = 0; n < oaTextClasses.GetSize(); n++)
		{
			textClass = cast(KDTextClass*, oaTextClasses.GetAt(n));
			cout << *textClass << "\n";
		}
	}
}

boolean KDTextFeatureConstruction::InternalCollectAllTextClasses(const KWClass* kwcMainClass, const KWClass* kwcClass,
								 KDTextAttributePath* baseTextAttributePath,
								 ObjectDictionary* odTextClasses) const
{
	boolean bDisplay = false;
	KWAttribute* attribute;
	KDTextClass* mainTextClass;
	KDTextClass* textClass;
	KDTextAttributePath* textAttributePath;
	boolean bClassContainsTextAttributes;
	boolean bRelationAttributeContainsTextAttributes;
	int i;

	require(kwcMainClass != NULL);
	require(kwcClass != NULL);
	require(baseTextAttributePath != NULL);
	require(odTextClasses != NULL);

	// Creation si necessaire d'un dictionnaire dedie aux texte
	// Par la suite, ce dictionnaire ne sera initialise que la premiere fois lors de sa creation
	// Si le dictionnaire est vu plusieurs fois suite a utilisations multiples (recursives ou non)
	// dans le meme schema multi-table, il n'est ainsi traite qu'une seule fois
	textClass = cast(KDTextClass*, odTextClasses->Lookup(kwcClass->GetName()));
	if (textClass == NULL)
	{
		textClass = new KDTextClass;
		textClass->SetClass(kwcClass);
		odTextClasses->SetAt(kwcClass->GetName(), textClass);
	}
	if (bDisplay)
		cout << "InternalCollectAllTextClasses\t" << baseTextAttributePath->GetObjectLabel() << "\t"
		     << kwcClass->GetName() << "\t\tBEGIN\n";

	// Recherche de la TextClass principale
	mainTextClass = cast(KDTextClass*, odTextClasses->Lookup(kwcMainClass->GetName()));

	// Parcours des attributs utilises pour detecter recursivement ceux de type Text
	bClassContainsTextAttributes = false;
	for (i = 0; i < kwcClass->GetUsedAttributeNumber(); i++)
	{
		attribute = kwcClass->GetUsedAttributeAt(i);

		// On ne prend en compte que les attribut de type relation ou texte
		if (KWType::IsTextBased(attribute->GetType()) or KWType::IsRelation(attribute->GetType()))
		{
			if (bDisplay)
				cout << "InternalCollectAllTextClasses\t" << baseTextAttributePath->GetObjectLabel()
				     << "\t" << kwcClass->GetName() << "\t" << attribute->GetName() << "\t"
				     << BooleanToString(baseTextAttributePath->ContainsClass(kwcClass)) << "\n";

			// Prise en compte uniquement si on ne cree pas un cycle en rajoutant le nouvel attribut de
			// relation
			//
			// On cherche a recuperer tous les attributs Text disponibles, en evitant au maximum de
			// les inclure plusieurs fois, ici en empechant la recursion via le chemin d'attributs.
			// Cela reste une solution heuristique: il ne semble pas y avoir de solution generale
			// gerant correctement tous les cas possible (schema recursif...)
			// Dans tous les cas, il reste pour l'utilisateur expert la solution specifique consistant a
			// ramener les variables Text souhaitees dans la table principale via des regles de drivation
			// dediees
			if (not baseTextAttributePath->ContainsClass(kwcClass))
			{
				// Si attribute de type Text, finalisation du chemin
				if (KWType::IsTextBased(attribute->GetType()))
				{
					bClassContainsTextAttributes = true;

					// Memorisation de l'attribut Text
					textClass->GetTextAttributes()->SetAt(attribute->GetName(), attribute);

					// Memorisation du chemin pour la classe principale
					textAttributePath = baseTextAttributePath->Clone();
					textAttributePath->AddAttributePathEnd(attribute);
					mainTextClass->GetTextAttributePaths()->Add(textAttributePath);
					assert(textAttributePath->Check());
					if (bDisplay)
						cout << "InternalCollectAllTextClasses\t"
						     << baseTextAttributePath->GetObjectLabel() << "\t"
						     << kwcClass->GetName() << "\t" << attribute->GetName()
						     << "\tAdd text\t" << textAttributePath->GetObjectLabel() << "\n";
					;
				}
				else
				{
					assert(KWType::IsRelation(attribute->GetType()));
					if (bDisplay)
						cout << "InternalCollectAllTextClasses\t"
						     << baseTextAttributePath->GetObjectLabel() << "\t"
						     << kwcClass->GetName() << "\t" << attribute->GetName()
						     << "\tAdd relation\n";

					// Ajout de l'attribut courant en fin de chemin de base
					baseTextAttributePath->AddAttributePathEnd(attribute);

					// Appel recursif de la methode
					bRelationAttributeContainsTextAttributes = InternalCollectAllTextClasses(
					    kwcMainClass, attribute->GetClass(), baseTextAttributePath, odTextClasses);
					if (bRelationAttributeContainsTextAttributes)
						bClassContainsTextAttributes = true;

					// Suppression de l'attribut courant de la fin du chemin de base
					assert(baseTextAttributePath->GetAttributePathAt(
						   baseTextAttributePath->GetAttributePathSize() - 1) == attribute);
					baseTextAttributePath->RemoveAttributePathEnd();

					// Memorisation de l'atttribut de type relation s'il abouti a des variables de
					// type Text
					if (bRelationAttributeContainsTextAttributes)
						textClass->GetRelationAttributes()->SetAt(attribute->GetName(),
											  attribute);
				}
			}
		}
	}

	if (bDisplay)
		cout << "InternalCollectAllTextClasses\t" << baseTextAttributePath->GetObjectLabel() << "\t"
		     << kwcClass->GetName() << "\t" << textClass->GetTextAttributePaths()->GetSize() << "\tEND\n";

	// On retourne true si la classe contient des attribut de type Text
	return bClassContainsTextAttributes;
}

void KDTextFeatureConstruction::ComputeTextClassConstructionCosts(const KWClass* kwcClass,
								  const ObjectDictionary* odTextClasses) const
{
	boolean bDisplay = false;
	KDTextClass* mainTextClass;
	KDTextClass* textClass;
	KDTextAttributePath* textAttributePath;
	const KWAttribute* attribute;
	const KWClass* attributeClass;
	int nPath;
	double dCost;
	int nAttribute;
	int nTextAttributeNumber;
	int nRelationAttributeNumber;

	require(kwcClass != NULL);
	require(odTextClasses != NULL);
	require(odTextClasses->Lookup(kwcClass->GetName()) != NULL);

	// Parcours des chemin d'attributs de la classe principale
	if (bDisplay)
		cout << "ComputeTextClassConstructionCosts\t" << kwcClass->GetName() << "\n";
	mainTextClass = cast(KDTextClass*, odTextClasses->Lookup(kwcClass->GetName()));
	check(mainTextClass);
	assert(mainTextClass->Check());
	for (nPath = 0; nPath < mainTextClass->GetTextAttributePaths()->GetSize(); nPath++)
	{
		textAttributePath = cast(KDTextAttributePath*, mainTextClass->GetTextAttributePaths()->GetAt(nPath));

		// Parcours de attributs du chemin
		dCost = 0;
		for (nAttribute = 0; nAttribute < textAttributePath->GetAttributePathSize(); nAttribute++)
		{
			attribute = textAttributePath->GetAttributePathAt(nAttribute);
			attributeClass = textAttributePath->GetAttributePathClassAt(nAttribute);
			assert(KWType::IsTextBased(attribute->GetType()) or KWType::IsRelation(attribute->GetType()));

			// Acces a la classe de Text de gestion de l'attribut
			textClass = cast(KDTextClass*, odTextClasses->Lookup(attributeClass->GetName()));
			check(textClass);
			assert(textClass->Check());

			// Parametres des couts d'acces a l'attribut
			nTextAttributeNumber = textClass->GetTextAttributes()->GetCount();
			nRelationAttributeNumber = textClass->GetRelationAttributes()->GetCount();

			// Calcul des couts d'acces dans le cas d'un attribut terminal de type texte
			if (KWType::IsTextBased(attribute->GetType()))
			{
				assert(nAttribute == textAttributePath->GetAttributePathSize() - 1);

				// Choix parmi les textes, plus 1 s'il existe des relations
				// Il est a noter qu'en ayant arrete la recursion dans le cas de schemas multi-table
				// avec des cycles, on explore pas tous les chemins possibles potentiels (un infinites),
				// et que le cout total de tous les chemins peut etre strictement inferieur a 1
				if (nRelationAttributeNumber == 0)
					dCost += log(nTextAttributeNumber);
				else
					dCost += log(nTextAttributeNumber + 1);
			}
			// Calcul des couts d'acces dans le cas d'un attribut de type relation
			else
			{
				assert(KWType::IsRelation(attribute->GetType()));
				assert(nRelationAttributeNumber > 0);
				assert(nAttribute < textAttributePath->GetAttributePathSize() - 1);

				// On chosit parmi les attribut texte, plus 1 pour undiquer le choix d'une relation
				dCost += log(nTextAttributeNumber + 1);

				// Puis parmi les relation
				dCost += log(nRelationAttributeNumber);
			}
		}

		// Mise a jour du cout
		textAttributePath->SetCost(dCost);
		if (bDisplay)
		{
			if (nPath == 0)
				cout << "\tCost\tProb\tFrac\tText variable path\n";
			cout << "\t" << textAttributePath->GetCost() << "\t" << exp(-textAttributePath->GetCost())
			     << "\t 1/" << 1 / exp(-textAttributePath->GetCost()) << "\t"
			     << textAttributePath->GetObjectLabel() << "\n";
		}
	}
}

void KDTextFeatureConstruction::ComputeTextClassConstructedFeatureNumbers(const KWClass* kwcClass,
									  const ObjectDictionary* odTextClasses,
									  int nFeatureNumber) const
{
	boolean bDisplay = false;
	KDMultinomialSampleGenerator multinomialSampleGenerator;
	DoubleVector dvProbs;
	DoubleVector dvFrequencies;
	KDTextClass* mainTextClass;
	KDTextAttributePath* textAttributePath;
	int nPath;
	double dProb;
	double dTotalProb;
	int nFrequency;

	require(kwcClass != NULL);
	require(odTextClasses != NULL);
	require(odTextClasses->Lookup(kwcClass->GetName()) != NULL);
	require(nFeatureNumber >= 0);

	// Collecte des probabilite de construction de variables a partir des cout des chemin d'attributs de la classe
	// principale
	if (bDisplay)
		cout << "ComputeTextClassConstructedFeatureNumbers\t" << kwcClass->GetName() << "\n";
	mainTextClass = cast(KDTextClass*, odTextClasses->Lookup(kwcClass->GetName()));
	check(mainTextClass);
	dvProbs.SetSize(mainTextClass->GetTextAttributePaths()->GetSize());
	dTotalProb = 0;
	for (nPath = 0; nPath < mainTextClass->GetTextAttributePaths()->GetSize(); nPath++)
	{
		textAttributePath = cast(KDTextAttributePath*, mainTextClass->GetTextAttributePaths()->GetAt(nPath));
		assert(textAttributePath->GetCost() > 0 or mainTextClass->GetTextAttributePaths()->GetSize() == 1);

		// Le cout est le log negatif de la probabilite
		dProb = exp(-textAttributePath->GetCost());
		dvProbs.SetAt(nPath, dProb);
		dTotalProb += dProb;
	}
	assert(0 <= dTotalProb and dTotalProb < 1 + 1e-6);

	// Normalisation des probas
	// En effet, en cas de cycle de le schema multi-table, les chemins recursifs sont ignoree et le total
	// de probabilite ne somme pas a 1
	for (nPath = 0; nPath < mainTextClass->GetTextAttributePaths()->GetSize(); nPath++)
	{
		dProb = dvProbs.GetAt(nPath) / dTotalProb;
		dvProbs.SetAt(nPath, dProb);
	}
	assert(multinomialSampleGenerator.CheckProbVector(&dvProbs));

	// Generation d'un echantillon multinomial
	multinomialSampleGenerator.ComputeBestSample(nFeatureNumber, &dvProbs, &dvFrequencies);
	assert(multinomialSampleGenerator.CheckFrequencyVector(nFeatureNumber, &dvFrequencies));

	// Memorisation des nombres de variables construites par chemin d'attributs
	for (nPath = 0; nPath < mainTextClass->GetTextAttributePaths()->GetSize(); nPath++)
	{
		textAttributePath = cast(KDTextAttributePath*, mainTextClass->GetTextAttributePaths()->GetAt(nPath));

		// Memorisation du nombre de variables construites
		nFrequency = (int)floor(dvFrequencies.GetAt(nPath) + 0.5);
		textAttributePath->SetConstructedFeatureNumber(nFrequency);

		// Affichage des resultats
		if (bDisplay)
		{
			if (nPath == 0)
				cout << "\tProb\tFrequency\tText variable path\n";
			cout << "\t" << dvProbs.GetAt(nPath) << "\t" << nFrequency << "\t"
			     << textAttributePath->GetObjectLabel() << "\n";
		}
	}
}

boolean KDTextFeatureConstruction::ExtractTokenSamples(const KWClass* kwcClass,
						       const ObjectDictionary* odTextClasses) const
{
	boolean bOk = true;
	KWClassDomain* currentDomain;
	KWClassDomain* tokenExtractionDomain;
	KWClass* tokenExtractionClass;

	require(kwcClass != NULL);
	require(odTextClasses != NULL);
	require(odTextClasses->Lookup(kwcClass->GetName()) != NULL);

	// Construction d'un domaine de lecture de la base oriente extraction des tokens pour les variables de type
	// texte
	tokenExtractionDomain = BuildTokenExtractionDomainWithExistingTokenNumbers(kwcClass, odTextClasses);
	tokenExtractionClass = tokenExtractionDomain->LookupClass(kwcClass->GetName());

	// Remplacement du domaine courant par le domaine d'extraction des tokens
	currentDomain = KWClassDomain::GetCurrentDomain();
	KWClassDomain::SetCurrentDomain(tokenExtractionDomain);

	// Lecture de la base pour collecter un echantillon des tokens par variable secondaire de type texte
	// On passe par une tache pour paralleliser cette lecture de la base
	// On fait l'hypothese que le dimensionnement de la tache qui doit collecter un echantillon de valeur
	// globalement pour le maitre et par esclave est suffisant pour le resume de cet echantillon en partiles
	bOk = TaskCollectTokenSamples(tokenExtractionClass, odTextClasses);

	// Nettoyage du domaine
	KWClassDomain::SetCurrentDomain(currentDomain);
	delete tokenExtractionDomain;
	return bOk;
}

boolean KDTextFeatureConstruction::TaskCollectTokenSamples(const KWClass* tokenExtractionClass,
							   const ObjectDictionary* odTextClasses) const
{
	boolean bOk = true;
	KDTextClass* mainTextClass;
	int nPath;
	KDTextAttributePath* textAttributePath;
	IntVector ivTokenNumbers;
	ObjectArray oaCollectedTokenSamples;
	KDTextTokenSampleCollectionTask tokenSampleCollectionTask;

	require(tokenExtractionClass != NULL);
	require(tokenExtractionClass->GetUsedAttributeNumber() > 0);
	require(tokenExtractionClass->GetUsedAttributeNumber() ==
		tokenExtractionClass->GetUsedAttributeNumberForType(KWType::Text) +
		    tokenExtractionClass->GetUsedAttributeNumberForType(KWType::TextList));
	require(odTextClasses != NULL);
	require(odTextClasses->Lookup(tokenExtractionClass->GetName()) != NULL);

	// Recherche la la classe de texte principale
	mainTextClass = cast(KDTextClass*, odTextClasses->Lookup(tokenExtractionClass->GetName()));
	check(mainTextClass);

	// Parcours des attributs par chemin d'acces aux attributs de type Text pour initialiser les parametres de la
	// tache
	for (nPath = 0; nPath < mainTextClass->GetTextAttributePaths()->GetSize(); nPath++)
	{
		textAttributePath = cast(KDTextAttributePath*, mainTextClass->GetTextAttributePaths()->GetAt(nPath));
		assert(textAttributePath->GetTokens()->GetSize() == 0);

		// Initialisation des parametres s'il y a des tokens a extraire
		if (textAttributePath->GetConstructedFeatureNumber() > 0)
		{
			// On tient compte des token existantes pour en demander davantage si necessaire
			ivTokenNumbers.Add(textAttributePath->GetConstructedFeatureNumber() +
					   textAttributePath->GetExistingTokenNumber());
			oaCollectedTokenSamples.Add(textAttributePath->GetTokens());
		}
	}
	assert(tokenExtractionClass->GetUsedAttributeNumber() == ivTokenNumbers.GetSize());
	assert(tokenExtractionClass->GetUsedAttributeNumber() == oaCollectedTokenSamples.GetSize());

	// Lancement de la tache
	tokenSampleCollectionTask.SetReusableDatabaseIndexer(GetLearningSpec()->GetDatabaseIndexer());
	tokenSampleCollectionTask.SetTextFeatures(GetTextFeatures());
	bOk = tokenSampleCollectionTask.CollectTokenSamples(GetDatabase(), &ivTokenNumbers, &oaCollectedTokenSamples);

	return bOk;
}

KWClassDomain* KDTextFeatureConstruction::BuildTokenExtractionDomainWithExistingTokenNumbers(
    const KWClass* kwcClass, const ObjectDictionary* odTextClasses) const
{
	boolean bDisplay = false;
	KDClassCompliantRules extractionClassCompliantRules;
	KWClassDomain* tokenExtractionDomain;
	KWClass* tokenExtractionClass;
	ObjectArray oaTextClasses;
	KDTextClass* mainTextClass;
	int nPath;
	int nUsefullPathNumber;
	KDTextAttributePath* textAttributePath;
	KWAttribute* textAttribute;
	KWDRTokenizationRule* textVariableBlockRule;
	KWAttributeBlock* attributeBlock;

	require(kwcClass != NULL);
	require(odTextClasses != NULL);
	require(odTextClasses->Lookup(kwcClass->GetName()) != NULL);

	// Initialisation par duplication de la classe principale
	tokenExtractionDomain = kwcClass->GetDomain()->CloneFromClass(kwcClass);
	tokenExtractionDomain->SetName("Token extraction");
	tokenExtractionDomain->Compile();
	tokenExtractionClass = tokenExtractionDomain->LookupClass(kwcClass->GetName());

	// Collecte de tous les attributs derives existants
	// Attention, on n'utise par le meme objet pour la classe d'extraction que pour la classe principale
	InitializeMainClassConstructedAttributesAndBlocks(&extractionClassCompliantRules, tokenExtractionClass);

	// Recherche la la classe de texte principale
	mainTextClass = cast(KDTextClass*, odTextClasses->Lookup(kwcClass->GetName()));
	check(mainTextClass);
	assert(mainTextClass->Check());

	// On met tout en unused dans la classe,
	tokenExtractionClass->SetAllAttributesUsed(false);

	// Generation des attributs par chemin d'acces aux attributs de type Text
	nUsefullPathNumber = 0;
	for (nPath = 0; nPath < mainTextClass->GetTextAttributePaths()->GetSize(); nPath++)
	{
		textAttributePath = cast(KDTextAttributePath*, mainTextClass->GetTextAttributePaths()->GetAt(nPath));
		assert(textAttributePath->Check());
		assert(textAttributePath->GetCost() > 0 or mainTextClass->GetTextAttributePaths()->GetSize() == 1);
		assert(textAttributePath->GetExistingTokenNumber() == 0);

		// Generation d'attributs uniquement si necessaire
		if (textAttributePath->GetConstructedFeatureNumber() > 0)
		{
			nUsefullPathNumber++;

			// Generation des attributs d'acces au texte si necessaire
			if (textAttributePath->GetAttributePathSize() > 1)
				textAttribute = ConstructPathAttribute(&extractionClassCompliantRules,
								       tokenExtractionClass, textAttributePath);
			// Recherche de l'attribut texte  dans la classe d'extraction sinon
			else
				textAttribute = tokenExtractionClass->LookupAttribute(
				    textAttributePath->GetAttributePathAt(0)->GetName());
			check(textAttribute);
			assert(KWType::IsTextBased(textAttribute->GetType()));

			// On met l'attribut en used et charge en memoire
			textAttribute->SetUsed(true);
			textAttribute->SetLoaded(true);

			// Creation de la regle de derivation, selon le type de l'attribut a analyser et selon le type
			// de tokens
			textVariableBlockRule =
			    KWDRTokenizationRule::CreateTokenizationRule(textAttribute->GetType(), GetTextFeatures());
			textVariableBlockRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			textVariableBlockRule->GetFirstOperand()->SetAttributeName(textAttribute->GetName());
			textVariableBlockRule->CompleteTypeInfo(tokenExtractionClass);

			// Recherche du bloc d'attribut parmi les blocs initiaux
			attributeBlock =
			    cast(KWAttributeBlock*,
				 extractionClassCompliantRules.LookupDerivedAttributeBlock(textVariableBlockRule));
			if (attributeBlock != NULL)
			{
				assert(tokenExtractionClass == attributeBlock->GetParentClass());

				// Recherche du nombre de variables existantes sur la base du nombre d'attribut du bloc
				textAttributePath->SetExistingTokenNumber(attributeBlock->ComputeAttributeNumber());
			}

			// Destruction de la regle de bloc ayant servi a le retrouver
			delete textVariableBlockRule;
			textVariableBlockRule = NULL;

			// Affichage des resultats
			if (bDisplay)
			{
				if (nPath == 0)
				{
					cout << "BuildTokenExtractionDomainWithExistingTokenNumbers\n";
					cout << "\tFeature number\tExisting tokens\tText variable path\n";
				}
				cout << "\t" << textAttributePath->GetConstructedFeatureNumber() << "\t"
				     << textAttributePath->GetExistingTokenNumber() << "\t"
				     << textAttributePath->GetObjectLabel() << "\n";
			}
		}
	}

	// Nettoyage de l'objet de collecte des attributs et blocs existants
	extractionClassCompliantRules.CleanCollectedConstructedAttributesAndBlocks();

	/////////////////////////////////////////////////////////////////////////////////////
	// Simplification de la classe en supprimant tous les attributs calcules en Unused,
	// sauf l'eventuel attribut de selection

	// On met en used l'eventuel attribut de selection, car ils peut avoir une regle de calcul
	if (GetDatabase()->GetSelectionAttribute() != "")
		tokenExtractionClass->LookupAttribute(GetDatabase()->GetSelectionAttribute())->SetUsed(true);

	// Nettoyage du domaine apres la necessaire compilation
	tokenExtractionClass->GetDomain()->Compile();
	tokenExtractionClass->DeleteUnusedDerivedAttributes(NULL);

	// On remet l'eventuel attribut de selection en etat unused
	if (GetDatabase()->GetSelectionAttribute() != "")
		tokenExtractionClass->LookupAttribute(GetDatabase()->GetSelectionAttribute())->SetUsed(false);

	// Compilation du domaine
	tokenExtractionClass->GetDomain()->Compile();
	ensure(tokenExtractionClass->GetUsedAttributeNumber() ==
	       tokenExtractionClass->GetUsedAttributeNumberForType(KWType::Text) +
		   tokenExtractionClass->GetUsedAttributeNumberForType(KWType::TextList));
	ensure(tokenExtractionClass->GetUsedAttributeNumber() == nUsefullPathNumber);
	return tokenExtractionDomain;
}

void KDTextFeatureConstruction::ConstructAllTextAttributeBlocks(KDClassCompliantRules* classCompliantRules,
								KWClass* kwcClass, ObjectDictionary* odTextClasses,
								ObjectDictionary* odConstructedAttributes) const
{
	boolean bDisplay = false;
	ObjectArray oaTextClasses;
	int i;
	KDTextClass* mainTextClass;
	KDTextClass* textClass;
	int nPath;
	KDTextAttributePath* textAttributePath;
	const KWAttribute* textAttribute;
	boolean bIsFeatrureConstructionNecessary;

	require(classCompliantRules != NULL);
	require(kwcClass != NULL);
	require(classCompliantRules->GetClass() == kwcClass);
	require(odTextClasses != NULL);
	require(odTextClasses->Lookup(kwcClass->GetName()) != NULL);
	require(odConstructedAttributes != NULL);
	require(odConstructedAttributes->GetCount() == 0);

	// Affichage des specifications initiales
	if (bDisplay)
	{
		cout << "ConstructTextFeatures " << kwcClass->GetName() << "\n";
		odTextClasses->ExportObjectArray(&oaTextClasses);
		for (i = 0; i < oaTextClasses.GetSize(); i++)
		{
			textClass = cast(KDTextClass*, oaTextClasses.GetAt(i));
			assert(textClass->Check());
			cout << *textClass << "\n";
		}
	}

	// Recherche la la classe de texte principale
	mainTextClass = cast(KDTextClass*, odTextClasses->Lookup(kwcClass->GetName()));
	check(mainTextClass);
	assert(mainTextClass->Check());

	// Generation des attributs par chemin d'acces aux attributs de type Text
	for (nPath = 0; nPath < mainTextClass->GetTextAttributePaths()->GetSize(); nPath++)
	{
		textAttributePath = cast(KDTextAttributePath*, mainTextClass->GetTextAttributePaths()->GetAt(nPath));
		assert(textAttributePath->Check());
		assert(textAttributePath->GetCost() > 0 or mainTextClass->GetTextAttributePaths()->GetSize() == 1);

		// Generation d'attributs uniquement si necessaire
		bIsFeatrureConstructionNecessary = textAttributePath->GetConstructedFeatureNumber() > 0;
		bIsFeatrureConstructionNecessary =
		    bIsFeatrureConstructionNecessary and textAttributePath->GetTokens()->GetSize() > 0;
		if (bIsFeatrureConstructionNecessary)
		{
			// Generation et memorisation des attributs d'acces au texte si necessaire
			if (textAttributePath->GetAttributePathSize() > 1)
			{
				textAttribute =
				    ConstructPathAttribute(classCompliantRules, kwcClass, textAttributePath);
				textAttributePath->SetMainTextAttribute(textAttribute);
			}
			// Recherche de l'attribut texte sinon
			else
				textAttribute = textAttributePath->GetAttributePathAt(0);
			assert(textAttributePath->Check());
			check(textAttribute);
			assert(KWType::IsTextBased(textAttribute->GetType()));

			// Construction d'un bloc de variables pour un attribut de type Text, selon le type de features
			ConstructTextAttributeTokenBlock(classCompliantRules, kwcClass, textAttribute,
							 GetLearningSpec()->GetSelectionCost() +
							     textAttributePath->GetCost(),
							 textAttributePath->GetConstructedFeatureNumber(),
							 textAttributePath->GetTokens(), odConstructedAttributes);
		}
	}
}

KWAttribute* KDTextFeatureConstruction::ConstructPathAttribute(KDClassCompliantRules* classCompliantRules,
							       KWClass* kwcClass,
							       const KDTextAttributePath* textAttributePath) const
{
	KWAttribute* mainTextAttribute;
	KWDerivationRule* rule;
	KWDerivationRule* subRule;
	const KWAttribute* attribute;
	int nContainedTextType;
	int i;
	boolean bNewAttribute;

	require(classCompliantRules != NULL);
	require(kwcClass != NULL);
	require(classCompliantRules->GetClass() == kwcClass);
	require(textAttributePath != NULL);
	require(textAttributePath->Check());
	require(textAttributePath->GetAttributePathSize() > 1);
	require(textAttributePath->GetMainTextAttribute() == NULL);
	require(kwcClass->GetName() == textAttributePath->GetMainClass()->GetName());

	// Creation des regles permettant de chercher l'attribut de type texte au bout du chemin
	// en partant de l'extremite du chemin
	rule = NULL;
	subRule = NULL;
	nContainedTextType = textAttributePath->GetTextAttribute()->GetType();
	for (i = textAttributePath->GetAttributePathSize() - 2; i >= 0; i--)
	{
		assert(KWType::IsTextBased(nContainedTextType));

		// Acces a l'attribut contenant
		attribute = textAttributePath->GetAttributePathAt(i);
		assert(KWType::IsRelation(attribute->GetType()));

		// Creation de la regle pour un attribut de type Object
		if (attribute->GetType() == KWType::Object)
		{
			if (nContainedTextType == KWType::Text)
				rule = new KWDRGetTextValue;
			else
				rule = new KWDRGetTextListValue;
		}
		// Creation de la regle pour un attribut de type ObjectArray
		else
		{
			if (nContainedTextType == KWType::Text)
				rule = new KWDRTableAllTexts;
			else
				rule = new KWDRTableAllTextLists;
		}
		assert(rule->GetOperandNumber() == 2);
		assert(KWType::IsRelation(rule->GetFirstOperand()->GetType()));
		assert(KWType::IsTextBased(rule->GetSecondOperand()->GetType()));

		// Parametrage de l'operande contenant de la regle
		rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		rule->GetFirstOperand()->SetAttributeName(attribute->GetName());

		// Parametrage de l'operande contenu de la regle
		if (subRule != NULL)
		{
			rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
			rule->GetSecondOperand()->SetDerivationRule(subRule);
		}
		else
		{
			rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			rule->GetSecondOperand()->SetAttributeName(textAttributePath->GetTextAttribute()->GetName());
		}

		// Memorisation du contexte
		subRule = rule;
		nContainedTextType = subRule->GetType();
	}
	assert(rule != NULL);
	rule->CompleteTypeInfo(kwcClass);

	// Creation d'un nouvel attribut dans la classe
	bNewAttribute = CreateConstructedDerivationRuleAttribute(
	    classCompliantRules, kwcClass, rule,
	    kwcClass->BuildAttributeName(BuildMainTextAttributeName(textAttributePath)), mainTextAttribute);
	if (bNewAttribute)
		mainTextAttribute->SetUsed(false);
	ensure(mainTextAttribute != NULL);
	return mainTextAttribute;
}

void KDTextFeatureConstruction::ConstructTextAttributeTokenBlock(KDClassCompliantRules* classCompliantRules,
								 KWClass* kwcClass, const KWAttribute* textAttribute,
								 double dTextConstructionCost,
								 int nBlockAttributeNumber, const ObjectArray* oaTokens,
								 ObjectDictionary* odConstructedAttributes) const
{
	boolean bDisplay = false;
	KWDRTokenizationRule* textVariableBlockRule;
	KWAttributeBlock* attributeBlock;
	KWAttribute* attribute;
	int nAttribute;
	KWIndexedCKeyBlock* existingAttributesIndexKeyBlock;
	ALString sAttributeKey;
	Symbol sKey;
	KWTokenFrequency* token;
	int nToken;
	double dCost;

	require(classCompliantRules != NULL);
	require(kwcClass != NULL);
	require(classCompliantRules->GetClass() == kwcClass);
	require(textAttribute != NULL);
	require(KWType::IsTextBased(textAttribute->GetType()));
	require(kwcClass->LookupAttribute(textAttribute->GetName()) == textAttribute);
	require(dTextConstructionCost >= 0);
	require(nBlockAttributeNumber > 0);
	require(oaTokens != NULL);
	require(odConstructedAttributes != NULL);

	// Affichage: entete
	if (bDisplay)
		cout << "ConstructTextAttributeTokenBlock\t" << GetTextFeatures() << "\t" << kwcClass->GetName() << "\t"
		     << textAttribute->GetName() << "\t" << oaTokens->GetSize() << "\t" << nBlockAttributeNumber
		     << "\n";

	// Creation de la regle de derivation, selon le type de l'attribute a analyser
	textVariableBlockRule =
	    KWDRTokenizationRule::CreateTokenizationRule(textAttribute->GetType(), GetTextFeatures());
	textVariableBlockRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	textVariableBlockRule->GetFirstOperand()->SetAttributeName(textAttribute->GetName());
	textVariableBlockRule->CompleteTypeInfo(kwcClass);

	// Recherche du bloc d'attribut parmi les blocs initiaux
	existingAttributesIndexKeyBlock = NULL;
	attributeBlock =
	    cast(KWAttributeBlock*, classCompliantRules->LookupDerivedAttributeBlock(textVariableBlockRule));
	if (attributeBlock != NULL)
	{
		assert(kwcClass == attributeBlock->GetParentClass());

		// Indexation des attributs existants
		existingAttributesIndexKeyBlock =
		    cast(KWIndexedCKeyBlock*, attributeBlock->BuildAttributesIndexedKeyBlock());
	}

	// Creation des variables du bloc en parcourant les tokens
	nAttribute = 0;
	for (nToken = 0; nToken < oaTokens->GetSize(); nToken++)
	{
		token = cast(KWTokenFrequency*, oaTokens->GetAt(nToken));
		assert(nToken == 0 or
		       token->CompareFrequency(cast(KWTokenFrequency*, oaTokens->GetAt(nToken - 1))) > 0);

		// Creation d'un attribut s'il n'est pas existant pour cette cle
		sAttributeKey = textVariableBlockRule->BuildAttributeKeyFromToken(token->GetToken());
		sKey = Symbol(sAttributeKey);
		if (existingAttributesIndexKeyBlock == NULL or not existingAttributesIndexKeyBlock->IsKeyPresent(sKey))
		{
			nAttribute++;

			// Creation d'un nouvel attribut pour le bloc
			attribute = new KWAttribute;
			attribute->SetName(kwcClass->BuildAttributeName(
			    BuildTokenBasedAttributeName(textAttribute, token->GetToken())));
			attribute->GetMetaData()->SetStringValueAt(KWAttributeBlock::GetAttributeKeyMetaDataKey(),
								   sAttributeKey);
			attribute->SetType(KWType::Continuous);
			attribute->SetUsed(true);

			// Memorisation dans les attributs construits
			odConstructedAttributes->SetAt(attribute->GetName(), attribute);

			// Mise a jour du cout
			dCost = dTextConstructionCost + KWStat::NaturalNumbersUniversalCodeLength(nToken + 1);
			attribute->SetCost(dCost);
			attribute->SetMetaDataCost(attribute->GetCost());

			// Si bloc existant, insertion de l'attribut en fin de son bloc
			if (attributeBlock != NULL)
				kwcClass->InsertAttributeInBlock(attribute, attributeBlock);
			// Sinon, ajout de l'attribut dans sa classe, et creation du bloc
			else
			{
				assert(textVariableBlockRule != NULL);

				// Ajout dans la classe
				kwcClass->InsertAttribute(attribute);

				// Creation du bloc dans la classe
				attributeBlock = kwcClass->CreateAttributeBlock(
				    kwcClass->BuildAttributeBlockName(BuildTokenBasedAttributeBlockName(textAttribute)),
				    attribute, attribute);
				attributeBlock->SetDerivationRule(textVariableBlockRule);
			}
		}

		// Affichage pour le token courant
		if (bDisplay)
			cout << "\t" << nToken + 1 << "\t" << nAttribute << "\t(" << token->GetToken() << ")\t"
			     << token->GetFrequency() << "\n";

		// Arret si assez d'attributs construits
		if (nAttribute >= nBlockAttributeNumber)
			break;
	}

	// Nettoyage
	if (existingAttributesIndexKeyBlock != NULL)
	{
		delete existingAttributesIndexKeyBlock;
		delete textVariableBlockRule;
	}
}

void KDTextFeatureConstruction::InitializeMainClassConstructedAttributesAndBlocks(
    KDClassCompliantRules* classCompliantRules, const KWClass* kwcClass) const
{
	ObjectDictionary odUsableDerivationRules;
	const KWDRGetTextValue refGetTextValueRule;
	const KWDRGetTextListValue refGetTextListValue;
	const KWDRTableAllTexts refTableAllTexts;
	const KWDRTableAllTextLists refTableAllTextLists;
	const KWDRTextNgrams refTextNGrams;
	const KWDRTextListNgrams refTextListNGrams;
	const KWDRTextWords refTextTokens;
	const KWDRTextListWords refTextListTokens;

	require(classCompliantRules != NULL);
	require(classCompliantRules->GetClass() == NULL);
	require(classCompliantRules->GetConstructedAttributeNumber() == 0);
	require(classCompliantRules->GetConstructedAttributeBlockNumber() == 0);
	require(kwcClass != NULL);

	// Parametrage par la classe
	classCompliantRules->SetClass(kwcClass);

	// Parametrage des regles de derivation utilisables
	odUsableDerivationRules.SetAt(refGetTextValueRule.GetName(), cast(Object*, &refGetTextValueRule));
	odUsableDerivationRules.SetAt(refGetTextListValue.GetName(), cast(Object*, &refGetTextListValue));
	odUsableDerivationRules.SetAt(refTableAllTexts.GetName(), cast(Object*, &refTableAllTexts));
	odUsableDerivationRules.SetAt(refTableAllTextLists.GetName(), cast(Object*, &refTableAllTextLists));
	odUsableDerivationRules.SetAt(refTextNGrams.GetName(), cast(Object*, &refTextNGrams));
	odUsableDerivationRules.SetAt(refTextListNGrams.GetName(), cast(Object*, &refTextListNGrams));
	odUsableDerivationRules.SetAt(refTextTokens.GetName(), cast(Object*, &refTextTokens));
	odUsableDerivationRules.SetAt(refTextListTokens.GetName(), cast(Object*, &refTextListTokens));

	// Ajout des attributs et blocs derives potentiellement constructibles
	classCompliantRules->CollectConstructedAttributesAndBlocksUsingDerivationRules(&odUsableDerivationRules);
}

boolean KDTextFeatureConstruction::CreateConstructedDerivationRuleAttribute(KDClassCompliantRules* classCompliantRules,
									    KWClass* kwcClass,
									    KWDerivationRule* constructedDerivationRule,
									    const ALString& sConstructedAttributeName,
									    KWAttribute*& constructedAttribute) const
{
	boolean bDisplay = false;
	boolean bNewAttribute;
	const KWAttribute* foundDerivedAttribute;

	require(classCompliantRules != NULL);
	require(kwcClass != NULL);
	require(classCompliantRules->GetClass() == kwcClass);
	require(constructedDerivationRule != NULL);
	require(sConstructedAttributeName != "");
	require(GetClass()->LookupAttribute(sConstructedAttributeName) == NULL);

	// Recherche de l'attribut parmi les attribut existants
	foundDerivedAttribute = classCompliantRules->LookupDerivedAttribute(constructedDerivationRule);

	// Memorisation de l'attribut associe a la regle construite
	constructedAttribute = NULL;
	if (foundDerivedAttribute != NULL)
	{
		constructedAttribute = cast(KWAttribute*, foundDerivedAttribute);
		bNewAttribute = false;

		// Destruction de la regle de derivation ayant servi a rechercher l'attribut
		delete constructedDerivationRule;
	}
	// Sinon, creation si necessaire d'un nouvel attribut
	else
	{
		bNewAttribute = true;

		// Ajout d'un nouvel attribut a la classe
		constructedAttribute = new KWAttribute;
		constructedAttribute->SetName(sConstructedAttributeName);
		constructedAttribute->SetDerivationRule(constructedDerivationRule);

		// Completion de l'attribut cree
		constructedAttribute->SetType(constructedDerivationRule->GetType());
		constructedAttribute->CompleteTypeInfo(kwcClass);

		// Insertion de l'attribut dans sa classe
		kwcClass->InsertAttribute(constructedAttribute);

		// Memorisation de l'attribut selon sa regle de derivation
		classCompliantRules->RegisterDerivedAttribute(constructedAttribute);
	}

	// Affichage de l'attribut cree
	if (bDisplay)
	{
		cout << kwcClass->GetName() << "\t";
		cout << bNewAttribute << "\t";
		cout << constructedAttribute->GetName() << "\t";
		constructedAttribute->GetDerivationRule()->WriteUsedRule(cout);
		cout << endl;
	}

	return bNewAttribute;
}

ALString KDTextFeatureConstruction::BuildIndexedAttributeName() const
{
	const ALString sVariablePrefix = "TextFeature";
	ALString sVariableName;

	require(nAttributeNameIndex >= 0);

	// Construction d'un nom indexe
	nAttributeNameIndex++;
	sVariableName = sVariablePrefix + IntToString(nAttributeNameIndex);
	return sVariableName;
}

void KDTextFeatureConstruction::ResetAttributeNameIndexes() const
{
	nAttributeNameIndex = 0;
}

const ALString KDTextFeatureConstruction::BuildMainTextAttributeName(const KDTextAttributePath* textAttributePath) const
{
	ALString sAttributeName;
	int i;

	require(textAttributePath != NULL);
	require(textAttributePath->Check());
	require(textAttributePath->GetAttributePathSize() > 1);

	// Construction d'un nom "interpretable"
	if (GetConstructionDomain()->GetInterpretableNames())
	{
		for (i = 0; i < textAttributePath->GetAttributePathSize(); i++)
		{
			if (i > 0)
				sAttributeName += '.';
			sAttributeName += textAttributePath->GetAttributePathAt(i)->GetName();
		}
	}
	// Numerotation des variables
	else
		sAttributeName = BuildIndexedAttributeName();
	return sAttributeName;
}

const ALString KDTextFeatureConstruction::BuildTokenBasedAttributeName(const KWAttribute* textAttribute,
								       const ALString& sToken) const
{
	ALString sAttributeName;

	require(textAttribute != NULL);

	// Construction d'un nom "interpretable"
	if (GetConstructionDomain()->GetInterpretableNames())
	{
		// Construction du nom de variable a partir des caracteristiques du token
		sAttributeName = textAttribute->GetName();
		if (GetTextFeatures() == "ngrams")
		{
			sAttributeName += '.';
			sAttributeName += IntToString(sToken.GetLength());
			sAttributeName += "gram";
		}
		sAttributeName += '(';
		sAttributeName += KWTextService::ByteStringToWord(sToken);
		sAttributeName += ')';
	}
	// Numerotation des variables
	else
		sAttributeName = BuildIndexedAttributeName();
	return sAttributeName;
}

const ALString KDTextFeatureConstruction::BuildTokenBasedAttributeBlockName(const KWAttribute* textAttribute) const
{
	ALString sAttributeName;

	require(textAttribute != NULL);

	// Construction d'un nom "interpretable"
	if (GetConstructionDomain()->GetInterpretableNames())
	{
		// Construction du nom de variable a partir des caracteristiques du ngram
		sAttributeName = textAttribute->GetName();
		sAttributeName += '.';
		sAttributeName += GetTextFeatures();
	}
	// Numerotation des variables
	else
		sAttributeName = BuildIndexedAttributeName();
	return sAttributeName;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KDTextAttributePath

KDTextAttributePath::KDTextAttributePath()
{
	dCost = 0;
	nConstructedFeatureNumber = 0;
	nExistingTokenNumber = 0;
	mainTextAttribute = NULL;
}

KDTextAttributePath::~KDTextAttributePath()
{
	oaTokens.DeleteAll();
}

void KDTextAttributePath::Reset()
{
	oaAttributePath.SetSize(0);
	dCost = 0;
	nConstructedFeatureNumber = 0;
	nExistingTokenNumber = 0;
	mainTextAttribute = NULL;
	oaTokens.DeleteAll();
}

int KDTextAttributePath::GetAttributePathSize() const
{
	return oaAttributePath.GetSize();
}

void KDTextAttributePath::AddAttributePathEnd(const KWAttribute* attribute)
{
	require(attribute != NULL);
	assert(attribute->GetParentClass() != NULL);
	oaAttributePath.Add(cast(Object*, attribute));
}

void KDTextAttributePath::RemoveAttributePathEnd()
{
	require(GetAttributePathSize() >= 1);
	oaAttributePath.SetSize(oaAttributePath.GetSize() - 1);
}

boolean KDTextAttributePath::ContainsClass(const KWClass* kwcClass) const
{
	int i;

	require(kwcClass != NULL);

	for (i = 0; i < GetAttributePathSize(); i++)
	{
		if (GetAttributePathClassAt(i) == kwcClass)
			return true;
	}
	return false;
}

const KWAttribute* KDTextAttributePath::GetAttributePathAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetAttributePathSize());
	return cast(const KWAttribute*, oaAttributePath.GetAt(nIndex));
}

void KDTextAttributePath::SetMainTextAttribute(const KWAttribute* attribute)
{
	mainTextAttribute = attribute;
}

const KWAttribute* KDTextAttributePath::GetMainTextAttribute() const
{
	return mainTextAttribute;
}

const KWAttribute* KDTextAttributePath::GetTextAttribute() const
{
	require(Check());
	return GetAttributePathAt(GetAttributePathSize() - 1);
}

const KWClass* KDTextAttributePath::GetMainClass() const
{
	if (GetAttributePathSize() == 0)
		return NULL;
	else
		return GetAttributePathAt(0)->GetParentClass();
}

const KWClass* KDTextAttributePath::GetAttributePathClassAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetAttributePathSize());
	return GetAttributePathAt(nIndex)->GetParentClass();
}

void KDTextAttributePath::SetCost(double dValue)
{
	require(dValue >= 0);
	dCost = dValue;
}

double KDTextAttributePath::GetCost() const
{
	return dCost;
}

void KDTextAttributePath::SetConstructedFeatureNumber(int nValue)
{
	require(nValue >= 0);
	nConstructedFeatureNumber = nValue;
}

int KDTextAttributePath::GetConstructedFeatureNumber() const
{
	return nConstructedFeatureNumber;
}

void KDTextAttributePath::SetExistingTokenNumber(int nValue)
{
	require(nValue >= 0);
	nExistingTokenNumber = nValue;
}

int KDTextAttributePath::GetExistingTokenNumber() const
{
	return nExistingTokenNumber;
}

ObjectArray* KDTextAttributePath::GetTokens()
{
	return &oaTokens;
}

boolean KDTextAttributePath::Check() const
{
	boolean bOk = true;
	const KWAttribute* attribute;
	const KWAttribute* containedAttribute;
	KWClass* kwcContainingClass;
	NumericKeyDictionary nkdAttributes;
	int i;

	// Le chemin doit etre de longueur non nulle
	bOk = bOk and GetAttributePathSize() > 0;
	assert(bOk);

	// Les attributs du path doivent amenee a l'attribut Text par un chemin utilise valide
	if (bOk)
	{
		// Parcours du chemin
		for (i = 0; i < GetAttributePathSize(); i++)
		{
			attribute = GetAttributePathAt(i);

			// Verification de l'attribut de chemin
			bOk = bOk and attribute != NULL;
			bOk = bOk and attribute->GetUsed();
			assert(bOk);

			// Les premiers attribut du chemin doivent etre de type Entity ou Table
			if (i < GetAttributePathSize() - 1)
				bOk = bOk and KWType::IsRelation(attribute->GetType());
			// Le dernier attribut du chemin doit etre de type Text
			else
				bOk = bOk and KWType::IsTextBased(attribute->GetType());
			assert(bOk);

			// Verification que l'attribut de chemin contient l'attribut suivant
			if (bOk and i < GetAttributePathSize() - 1)
			{
				kwcContainingClass = attribute->GetClass();
				assert(kwcContainingClass != NULL);

				// Recherche de l'attribut suivant
				containedAttribute = GetAttributePathAt(i + 1);

				// Verification que l'attribut suivant est accessible depuis la classe de l'attribut
				// courant
				bOk = bOk and containedAttribute != NULL;
				bOk = bOk and kwcContainingClass->LookupAttribute(containedAttribute->GetName()) ==
						  containedAttribute;
				assert(bOk);
			}

			// Verification de l'absence de cycle, par l'unicite des attributs utilises dans le chemins
			bOk = bOk and nkdAttributes.Lookup(attribute) == NULL;
			if (bOk)
				nkdAttributes.SetAt(attribute, cast(Object*, attribute));
			assert(bOk);
		}
	}

	// Verification de l'attribut texte de la classe principale, s'il est specifie
	if (bOk and mainTextAttribute != NULL)
	{
		bOk = bOk and mainTextAttribute->GetParentClass() == GetMainClass();
		bOk = bOk and KWType::IsTextBased(mainTextAttribute->GetType());
		bOk = bOk and GetAttributePathSize() > 1;
		bOk = bOk and not mainTextAttribute->GetUsed();
		assert(bOk);
	}
	return bOk;
}

void KDTextAttributePath::CopyFrom(const KDTextAttributePath* aSource)
{
	require(aSource != NULL);

	oaAttributePath.CopyFrom(&aSource->oaAttributePath);
	dCost = aSource->dCost;
	nConstructedFeatureNumber = aSource->nConstructedFeatureNumber;
	nExistingTokenNumber = aSource->nExistingTokenNumber;
}

KDTextAttributePath* KDTextAttributePath::Clone() const
{
	KDTextAttributePath* aClone;

	aClone = new KDTextAttributePath;
	aClone->CopyFrom(this);
	return aClone;
}

void KDTextAttributePath::Write(ostream& ost) const
{
	int i;
	const KWAttribute* attribute;

	for (i = 0; i < GetAttributePathSize(); i++)
	{
		attribute = GetAttributePathAt(i);
		if (i > 0)
			ost << ".";
		if (attribute == NULL)
			ost << "?";
		else
			ost << attribute->GetName();
	}
	ost << " (" << dCost << ", " << nConstructedFeatureNumber << ", " << nExistingTokenNumber << ")";
}

const ALString KDTextAttributePath::GetClassLabel() const
{
	return "Text variable path";
}

const ALString KDTextAttributePath::GetObjectLabel() const
{
	ALString sLabel;
	int i;
	const KWAttribute* attribute;

	for (i = 0; i < GetAttributePathSize(); i++)
	{
		attribute = GetAttributePathAt(i);
		if (i > 0)
			sLabel += ".";
		if (attribute == NULL)
			sLabel += "?";
		else
			sLabel += attribute->GetName();
	}
	return sLabel;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KDTextClass

KDTextClass::KDTextClass()
{
	kwcTextClass = NULL;
}

KDTextClass::~KDTextClass()
{
	oaTextAttributePaths.DeleteAll();
}

void KDTextClass::Reset()
{
	kwcTextClass = NULL;
	odTextAttributes.RemoveAll();
	odRelationAttributes.RemoveAll();
	oaTextAttributePaths.SetSize(0);
}

void KDTextClass::SetClass(const KWClass* kwcClass)
{
	kwcTextClass = kwcClass;
}

const KWClass* KDTextClass::GetClass()
{
	return kwcTextClass;
}

ObjectDictionary* KDTextClass::GetTextAttributes()
{
	return &odTextAttributes;
}

ObjectDictionary* KDTextClass::GetRelationAttributes()
{
	return &odRelationAttributes;
}

ObjectArray* KDTextClass::GetTextAttributePaths()
{
	return &oaTextAttributePaths;
}

boolean KDTextClass::Check() const
{
	boolean bOk = true;
	int i;
	ObjectArray oaTextAttributes;
	ObjectArray oaRelationAttributes;
	KWAttribute* attribute;
	NumericKeyDictionary nkdAllAttributes;
	KDTextAttributePath* textAttributePath;

	// Verification de la classe
	bOk = bOk and kwcTextClass != NULL;
	bOk = bOk and kwcTextClass->Check();
	assert(bOk);

	// Export des attributs vers des tableaux
	odTextAttributes.ExportObjectArray(&oaTextAttributes);
	odRelationAttributes.ExportObjectArray(&oaRelationAttributes);

	// Verification des attributs Text
	bOk = bOk and odTextAttributes.GetCount() == kwcTextClass->GetUsedAttributeNumberForType(KWType::Text) +
							 kwcTextClass->GetUsedAttributeNumberForType(KWType::TextList);
	assert(bOk);
	for (i = 0; i < oaTextAttributes.GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaTextAttributes.GetAt(i));

		// Existence et type de l'attribut dans la classe
		bOk = bOk and attribute != NULL;
		bOk = bOk and attribute->GetUsed();
		bOk = bOk and kwcTextClass->LookupAttribute(attribute->GetName()) == attribute;
		bOk = bOk and KWType::IsTextBased(attribute->GetType());

		// Unicite de l'attribut
		bOk = bOk and nkdAllAttributes.Lookup(attribute) == NULL;
		nkdAllAttributes.SetAt(attribute, attribute);
		assert(bOk);
	}

	// Verification des attributs Relation
	bOk = bOk and
	      odRelationAttributes.GetCount() <= kwcTextClass->GetUsedAttributeNumberForType(KWType::Object) +
						     kwcTextClass->GetUsedAttributeNumberForType(KWType::ObjectArray);
	assert(bOk);
	for (i = 0; i < oaRelationAttributes.GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaRelationAttributes.GetAt(i));

		// Existence et type de l'attribut dans la classe
		bOk = bOk and attribute != NULL;
		bOk = bOk and attribute->GetUsed();
		bOk = bOk and kwcTextClass->LookupAttribute(attribute->GetName()) == attribute;
		bOk = bOk and KWType::IsRelation(attribute->GetType());

		// Unicite de l'attribut
		bOk = bOk and nkdAllAttributes.Lookup(attribute) == NULL;
		nkdAllAttributes.SetAt(attribute, attribute);
		assert(bOk);
	}

	// Verification des chemins d'acces aux donnees textuelles
	assert(bOk);
	for (i = 0; i < oaTextAttributePaths.GetSize(); i++)
	{
		textAttributePath = cast(KDTextAttributePath*, oaTextAttributePaths.GetAt(i));
		bOk = bOk and textAttributePath->Check();
		bOk = bOk and textAttributePath->GetMainClass() == kwcTextClass;
		bOk = bOk and textAttributePath->GetAttributePathAt(0) ==
				  kwcTextClass->LookupAttribute(textAttributePath->GetAttributePathAt(0)->GetName());
		assert(bOk);
	}
	return bOk;
}

void KDTextClass::Write(ostream& ost) const
{
	int i;
	ObjectArray oaTextAttributes;
	ObjectArray oaRelationAttributes;
	KWAttribute* attribute;
	KDTextAttributePath* textAttributePath;

	// Titre
	ost << GetClassLabel() << " " << GetObjectLabel() << "\n";

	// Export des attributs vers des tableaux tries
	odTextAttributes.ExportObjectArray(&oaTextAttributes);
	oaTextAttributes.SetCompareFunction(KWAttributeCompareName);
	oaTextAttributes.Sort();
	odRelationAttributes.ExportObjectArray(&oaRelationAttributes);
	oaRelationAttributes.SetCompareFunction(KWAttributeCompareName);
	oaRelationAttributes.Sort();

	// Attributs Text
	ost << "  Text variables\t" << oaTextAttributes.GetSize() << "\n";
	for (i = 0; i < oaTextAttributes.GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaTextAttributes.GetAt(i));
		ost << "\t" << attribute->GetName() << "\n";
	}

	// Attributs Relation
	ost << "  Relation variables\t" << oaRelationAttributes.GetSize() << "\n";
	for (i = 0; i < oaRelationAttributes.GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaRelationAttributes.GetAt(i));
		ost << "\t" << attribute->GetName() << "\n";
	}

	// Chemins d'acces aux donnees textuelles
	ost << "  Text variables paths\t" << oaTextAttributePaths.GetSize() << "\n";
	for (i = 0; i < oaTextAttributePaths.GetSize(); i++)
	{
		textAttributePath = cast(KDTextAttributePath*, oaTextAttributePaths.GetAt(i));
		ost << "\t" << *textAttributePath << "\n";
	}
}

const ALString KDTextClass::GetClassLabel() const
{
	return "Text dictionary variables";
}

const ALString KDTextClass::GetObjectLabel() const
{
	if (kwcTextClass == NULL)
		return "";
	else
		return kwcTextClass->GetName();
}

int KDTextClassCompareName(const void* first, const void* second)
{
	KDTextClass* aFirst;
	KDTextClass* aSecond;
	int nResult;

	aFirst = cast(KDTextClass*, *(Object**)first);
	aSecond = cast(KDTextClass*, *(Object**)second);
	assert(aFirst->GetClass() != NULL);
	assert(aSecond->GetClass() != NULL);
	nResult = aFirst->GetClass()->GetName().Compare(aSecond->GetClass()->GetName());
	return nResult;
}