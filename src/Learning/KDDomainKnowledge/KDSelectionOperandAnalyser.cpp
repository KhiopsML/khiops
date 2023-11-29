// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDSelectionOperandAnalyser.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionOperandAnalyser

KDSelectionOperandAnalyser::KDSelectionOperandAnalyser()
{
	multiTableFeatureConstruction = NULL;
}

KDSelectionOperandAnalyser::~KDSelectionOperandAnalyser()
{
	CleanAll();
}

void KDSelectionOperandAnalyser::SetMultiTableFeatureConstruction(
    KDMultiTableFeatureConstruction* featureConstructionParam)
{
	multiTableFeatureConstruction = featureConstructionParam;
}

KDMultiTableFeatureConstruction* KDSelectionOperandAnalyser::GetMultiTableFeatureConstruction() const
{
	require(multiTableFeatureConstruction != NULL);
	return multiTableFeatureConstruction;
}

KDConstructionDomain* KDSelectionOperandAnalyser::GetConstructionDomain() const
{
	require(multiTableFeatureConstruction != NULL);
	return multiTableFeatureConstruction->GetConstructionDomain();
}

KDClassSelectionStats*
KDSelectionOperandAnalyser::AddClassSelectionStats(const KDClassCompliantRules* classCompliantRulesParam)
{
	KDClassSelectionStats* classSelectionStats;

	require(multiTableFeatureConstruction != NULL);
	require(classCompliantRulesParam != NULL);
	require(LookupClassSelectionStats(classCompliantRulesParam->GetClassName()) == NULL);

	// Creation et memorisation d'un objet de statistiques par classe
	classSelectionStats = new KDClassSelectionStats;
	classSelectionStats->SetClassCompliantRules(classCompliantRulesParam);
	oaClassSelectionStats.Add(classSelectionStats);
	odClassSelectionStats.SetAt(classCompliantRulesParam->GetClassName(), classSelectionStats);
	return classSelectionStats;
}

boolean KDSelectionOperandAnalyser::ComputeStats(const ObjectArray* oaAllConstructedRules)
{
	boolean bDisplay = false;
	int nRule;
	KDConstructedRule* constructedRule;

	require(Check());

	// Trace memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " Begin");

	// Nettoyage prealable des resultats
	CleanStats();

	// Flag de calcul (positionne au debut pour avoir acces aux resultats
	bIsStatsComputed = true;

	// Comptage des nombres d'utilisation des parties, partitions et dimensions de partition
	ResetAllUseCounts();
	for (nRule = 0; nRule < oaAllConstructedRules->GetSize(); nRule++)
	{
		constructedRule = cast(KDConstructedRule*, oaAllConstructedRules->GetAt(nRule));
		constructedRule->IncrementUseCounts();
	}

	// Analyse de la base pour le pretraitement des partitions de valeurs, si necessaire
	if (bIsStatsComputed and GetClassSelectionStats()->GetSize() > 0)
		bIsStatsComputed = ExtractSelectionOperandPartitions();

	// Affichage des results
	if (bDisplay)
	{
		cout << "KDSelectionOperandAnalyser::ComputeStats" << endl;
		DisplayUsedSelectionOperands(cout);
		Write(cout);
		cout << endl;
	}

	// Nettoyage si echec
	if (not bIsStatsComputed or TaskProgression::IsInterruptionRequested())
		CleanStats();

	// Trace memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " End");
	return bIsStatsComputed;
}

void KDSelectionOperandAnalyser::CleanStats()
{
	KDClassSelectionStats* classSelectionStats;
	int i;

	// Reinitialisation des resultats par classe
	for (i = 0; i < oaClassSelectionStats.GetSize(); i++)
	{
		classSelectionStats = cast(KDClassSelectionStats*, oaClassSelectionStats.GetAt(i));
		classSelectionStats->CleanStats();
	}

	// Reinitialisation du flag d'analyse des stats
	bIsStatsComputed = false;
}

void KDSelectionOperandAnalyser::CleanAll()
{
	// Nettoyage
	CleanStats();

	// Destruction des resultats d'analyse
	odClassSelectionStats.RemoveAll();
	oaClassSelectionStats.DeleteAll();
}

const ObjectArray* KDSelectionOperandAnalyser::GetClassSelectionStats() const
{
	require(IsStatsComputed() or oaClassSelectionStats.GetSize() == 0);
	return &oaClassSelectionStats;
}

KDClassSelectionStats* KDSelectionOperandAnalyser::LookupClassSelectionStats(const ALString& sClassName) const
{
	return cast(KDClassSelectionStats*, odClassSelectionStats.Lookup(sClassName));
}

void KDSelectionOperandAnalyser::ResetAllUseCounts()
{
	KDClassSelectionStats* classSelectionStats;
	KDClassSelectionOperandStats* classSelectionOperandStats;
	KDConstructedPartition* constructedPartition;
	int nClass;
	int nOperand;
	int nPartition;

	// Parcours des classes a analyser
	for (nClass = 0; nClass < oaClassSelectionStats.GetSize(); nClass++)
	{
		classSelectionStats = cast(KDClassSelectionStats*, oaClassSelectionStats.GetAt(nClass));

		// Parcours des operandes de selection de la classe
		for (nOperand = 0; nOperand < classSelectionStats->GetClassSelectionOperandStats()->GetSize();
		     nOperand++)
		{
			classSelectionOperandStats =
			    cast(KDClassSelectionOperandStats*,
				 classSelectionStats->GetClassSelectionOperandStats()->GetAt(nOperand));

			// Reinitialisation du compteur d'utilisation associe la dimension de partition
			cast(KDConstructedPartitionDimension*, classSelectionOperandStats->GetPartitionDimension())
			    ->ResetUseCount();
		}

		// Parcours des partitions basees sur la classe
		for (nPartition = 0; nPartition < classSelectionStats->GetPartitions()->GetSize(); nPartition++)
		{
			constructedPartition =
			    cast(KDConstructedPartition*, classSelectionStats->GetPartitions()->GetAt(nPartition));

			// Reinitialisation du compteur d'utilisation associe a la partition et a ses parties
			constructedPartition->ResetUseCount();
			constructedPartition->ResetAllPartsUseCounts();
		}
	}
}

void KDSelectionOperandAnalyser::DisplayUsedSelectionOperands(ostream& ost) const
{
	int nClass;
	KDClassSelectionStats* classSelectionStats;
	KDClassSelectionOperandStats* classSelectionOperandStats;
	const KDConstructedPartitionDimension* partitionDimension;
	int nAttribute;

	// Entete
	ost << "Selection operand stats\n";
	ost << "Class\tCount\tOrigin\tName\n";

	// Creation et parametrage des attributs de selection dans les operandes de selection
	for (nClass = 0; nClass < oaClassSelectionStats.GetSize(); nClass++)
	{
		classSelectionStats = cast(KDClassSelectionStats*, oaClassSelectionStats.GetAt(nClass));

		// Affichage des operandes de selection de type attribut our regle
		for (nAttribute = 0; nAttribute < classSelectionStats->GetClassSelectionOperandStats()->GetSize();
		     nAttribute++)
		{
			classSelectionOperandStats =
			    cast(KDClassSelectionOperandStats*,
				 classSelectionStats->GetClassSelectionOperandStats()->GetAt(nAttribute));
			partitionDimension = classSelectionOperandStats->GetPartitionDimension();

			// Si operande de type attribut, identification de l'attribut pour le mettre en Used
			if (partitionDimension->GetOrigin() == KDConstructedPartitionDimension::Attribute)
			{
				// Affichage de l'attribut
				ost << classSelectionStats->GetClassName() << "\t";
				ost << partitionDimension->GetUseCount() << "\t";
				ost << "Attribute\t";
				ost << partitionDimension->GetAttribute()->GetName() << endl;
			}
			// Si operande de type regle, on cree l'attribut
			else
			{
				// Affichage de la regle
				ost << classSelectionStats->GetClassName() << "\t";
				ost << partitionDimension->GetUseCount() << "\t";
				ost << "Rule\t";
				ost << *partitionDimension->GetRule() << endl;
			}
		}
	}
}

boolean KDSelectionOperandAnalyser::Check() const
{
	boolean bOk = true;
	const KDClassSelectionStats* classSelectionStats;
	int i;

	// Methode ancetre
	bOk = KWLearningReport::Check();

	// Verification des resultats
	if (bOk)
	{
		// Verification des stats par classe
		assert(oaClassSelectionStats.GetSize() == odClassSelectionStats.GetCount());
		for (i = 0; i < oaClassSelectionStats.GetSize(); i++)
		{
			classSelectionStats = cast(const KDClassSelectionStats*, oaClassSelectionStats.GetAt(i));
			assert(LookupClassSelectionStats(classSelectionStats->GetClassName()) == classSelectionStats);
			bOk = classSelectionStats->Check();
			if (not bOk)
				break;
		}
	}
	return bOk;
}

void KDSelectionOperandAnalyser::Write(ostream& ost) const
{
	const KDClassSelectionStats* classSelectionStats;
	int i;

	// Parcours des stats par classe
	ost << "Selection operand analyser\n";
	for (i = 0; i < oaClassSelectionStats.GetSize(); i++)
	{
		classSelectionStats = cast(const KDClassSelectionStats*, oaClassSelectionStats.GetAt(i));
		classSelectionStats->Write(ost);
	}
}

const ALString KDSelectionOperandAnalyser::GetClassLabel() const
{
	return "Selection operand analyser";
}

const ALString KDSelectionOperandAnalyser::GetObjectLabel() const
{
	return "";
}

boolean KDSelectionOperandAnalyser::ExtractSelectionOperandPartitions()
{
	boolean bOk = true;
	boolean bDisplay = false;
	KWClassDomain* currentDomain;
	KWClassDomain* selectionDomain;
	KWClass* kwcClass;
	int nClass;
	KDClassSelectionStats* classSelectionStats;
	int nOperand;
	KDClassSelectionOperandStats* classSelectionOperandStats;
	int nReadObjectNumber;

	// Construction d'un domaine de lecture de la base oriente operandes de selection
	selectionDomain = BuildSelectionDomain();

	// Affichage du domaine construit
	if (bDisplay)
	{
		cout << "\nKDSelectionOperandAnalyser::ExtractSelectionOperandPartitions\n";
		cout << "Main class\n";
		kwcClass = selectionDomain->LookupClass(GetClass()->GetName());
		cout << *kwcClass << endl;
		cout << "Selection classes\n";
		for (nClass = 0; nClass < GetClassSelectionStats()->GetSize(); nClass++)
		{
			classSelectionStats = cast(KDClassSelectionStats*, GetClassSelectionStats()->GetAt(nClass));
			kwcClass = selectionDomain->LookupClass(classSelectionStats->GetClassName());
			cout << *kwcClass << endl;
		}
	}

	// Remplacement du domaine courant par le domaine de selection
	currentDomain = KWClassDomain::GetCurrentDomain();
	KWClassDomain::SetCurrentDomain(selectionDomain);
	selectionDomain->Compile();

	// Lecture de la base pour collecter un echantillon de valeurs par variable secondaire de selection
	// On passe par une tache pour paralleliser cette lecture de la base
	// On fait l'hypothese que le dimensionnement de la tache qui doit collecter un echantillon de valeur
	// globalement pour le maitre et par esclave est suffisant pour le resume de cet echantillon en partiles
	bOk = TaskCollectSelectionOperandSamples(nReadObjectNumber);

	// Analyse des partitions de chaque operande de selection si ok, et nettoyage
	for (nClass = 0; nClass < oaClassSelectionStats.GetSize(); nClass++)
	{
		classSelectionStats = cast(KDClassSelectionStats*, oaClassSelectionStats.GetAt(nClass));

		// Parcours des operandes de selection
		for (nOperand = 0; nOperand < classSelectionStats->GetClassSelectionOperandStats()->GetSize();
		     nOperand++)
		{
			classSelectionOperandStats =
			    cast(KDClassSelectionOperandStats*,
				 classSelectionStats->GetClassSelectionOperandStats()->GetAt(nOperand));

			// Analyse des operandes de selection effectivement utilises
			assert(classSelectionOperandStats->GetPartitionDimension()->GetUseCount() > 0 or
			       classSelectionOperandStats->GetSelectionAttribute() == NULL);
			if (classSelectionOperandStats->GetSelectionAttribute() != NULL)
			{
				// Analyse de la partition des valeurs de selection
				if (bOk)
					classSelectionOperandStats->AnalysePartStats(nReadObjectNumber,
										     nMinPartileFrequency);

				// Nettoyage des valeurs en entree
				classSelectionOperandStats->CleanInputData();

				// Nettoyage de la variable de selection
				classSelectionOperandStats->SetSelectionAttribute(NULL);
			}
		}

		// Affichage des partitions par operande de selection
		if (bDisplay)
			cout << *classSelectionStats << endl;
	}

	// Nettoyage du domaine
	KWClassDomain::SetCurrentDomain(currentDomain);
	delete selectionDomain;
	return bOk;
}

boolean KDSelectionOperandAnalyser::TaskCollectSelectionOperandSamples(int& nReadObjectNumber)
{
	boolean bOk = true;
	KDSelectionOperandDataSampler* selectionOperandDataSampler;
	KDSelectionOperandSamplingTask selectionOperandSamplingTask;

	// Construction de specifications d'echantillonnage des donnees par operande
	selectionOperandDataSampler = BuildSelectionOperandDataSamplerSpec();

	// Lancement de la tache de collecte de l'echantillon si necessaire
	if (selectionOperandDataSampler->GetClassSelectionData()->GetSize() > 0)
	{
		// Lancement de la tache
		selectionOperandSamplingTask.SetReusableDatabaseIndexer(GetLearningSpec()->GetDatabaseIndexer());
		bOk = selectionOperandSamplingTask.CollectSelectionOperandSamples(selectionOperandDataSampler,
										  GetDatabase(), nReadObjectNumber);

		// Prise en compte des donnees collectees par un echantilonneur de donnees
		if (bOk)
			CollectClassSelectionData(selectionOperandDataSampler);
	}

	// Nettoyage
	delete selectionOperandDataSampler;
	return bOk;
}

KDSelectionOperandDataSampler* KDSelectionOperandAnalyser::BuildSelectionOperandDataSamplerSpec() const
{
	KDSelectionOperandDataSampler* selectionOperandDataSampler;
	KDClassSelectionData* classSelectionData;
	KDClassSelectionOperandData* classSelectionOperandData;
	KDClassSelectionStats* classSelectionStats;
	KDClassSelectionOperandStats* classSelectionOperandStats;
	int nClass;
	int nOperand;

	// Creation du sampler
	selectionOperandDataSampler = new KDSelectionOperandDataSampler;

	// Parametrage du nom de la base
	selectionOperandDataSampler->SetDatabaseName(GetDatabase()->GetDatabaseName());

	// Ajout des classes de selection
	for (nClass = 0; nClass < GetClassSelectionStats()->GetSize(); nClass++)
	{
		classSelectionStats = cast(KDClassSelectionStats*, GetClassSelectionStats()->GetAt(nClass));

		// Creation d'une classe de collecte de donnees
		classSelectionData = new KDClassSelectionData;
		classSelectionData->sClassName = classSelectionStats->GetClassName();

		// Parametrage de la granularite max de tous les operandes de selection utilisees en dimension d'une
		// partition
		classSelectionData->nMaxOperandGranularity = classSelectionStats->ComputeMaxOperandGranularity();

		// Ajout des operandes de selection
		for (nOperand = 0; nOperand < classSelectionStats->GetClassSelectionOperandStats()->GetSize();
		     nOperand++)
		{
			classSelectionOperandStats =
			    cast(KDClassSelectionOperandStats*,
				 classSelectionStats->GetClassSelectionOperandStats()->GetAt(nOperand));

			// Ajout d'une operande de collecte de donnees si necessaire
			if (classSelectionOperandStats->GetSelectionAttribute() != NULL)
			{
				classSelectionOperandData = new KDClassSelectionOperandData;
				classSelectionOperandData->sSelectionAttributeName =
				    classSelectionOperandStats->GetSelectionAttribute()->GetName();
				classSelectionOperandData->selectionAttribute =
				    classSelectionOperandStats->GetSelectionAttribute();
				classSelectionData->oaClassSelectionOperandData.Add(classSelectionOperandData);
			}
		}

		// Ajout de cette classe s'il a au moins une operande de selection
		if (classSelectionData->GetClassSelectionOperandData()->GetSize() > 0)
			selectionOperandDataSampler->oaClassSelectionData.Add(classSelectionData);
		// Nettoyage sinon
		else
			delete classSelectionData;
	}

	// Mise en phase du dictionnaire des classes
	selectionOperandDataSampler->RefreshClassSelectionData();
	ensure(selectionOperandDataSampler->Check());
	return selectionOperandDataSampler;
}

void KDSelectionOperandAnalyser::CollectClassSelectionData(
    KDSelectionOperandDataSampler* sourceSelectionOperandDataSampler)
{
	KDClassSelectionStats* classSelectionStats;
	KDClassSelectionOperandStats* selectionOperandStats;
	KDClassSymbolSelectionOperandStats* symbolSelectionOperandStats;
	KDClassContinuousSelectionOperandStats* continuousSelectionOperandStats;
	KDClassSelectionData* sourceClassSelectionData;
	KDClassSelectionOperandData* sourceSelectionOperandData;
	int nClass;
	int nStatClass;
	int nOperand;
	int nStatsOperand;
	KWAttribute* selectionAttribute;

	require(Check());
	require(sourceSelectionOperandDataSampler != NULL);
	require(sourceSelectionOperandDataSampler->Check());
	require(sourceSelectionOperandDataSampler->GetClassSelectionData()->GetSize() <=
		GetClassSelectionStats()->GetSize());

	// Nettoyage prealable des references aux objets selectionnes de l'echantillonneur, pour liberer de la memoire
	for (nClass = 0; nClass < sourceSelectionOperandDataSampler->GetClassSelectionData()->GetSize(); nClass++)
	{
		sourceClassSelectionData = cast(
		    KDClassSelectionData*, sourceSelectionOperandDataSampler->GetClassSelectionData()->GetAt(nClass));
		sourceClassSelectionData->GetSelectionObjects()->DeleteAll();
	}

	// Parcours des classes de selection
	nStatClass = 0;
	for (nClass = 0; nClass < sourceSelectionOperandDataSampler->GetClassSelectionData()->GetSize(); nClass++)
	{
		sourceClassSelectionData = cast(
		    KDClassSelectionData*, sourceSelectionOperandDataSampler->GetClassSelectionData()->GetAt(nClass));

		// Parcours des classes cote stats jusqu'a trouver la classe correspondante
		classSelectionStats = NULL;
		while (nStatClass < GetClassSelectionStats()->GetSize())
		{
			classSelectionStats = cast(KDClassSelectionStats*, GetClassSelectionStats()->GetAt(nStatClass));
			nStatClass++;

			// On s'arrete quand on a trouve la classe correspondante
			if (sourceClassSelectionData->GetClassName() == classSelectionStats->GetClassName())
				break;
		}
		assert(nStatClass <= GetClassSelectionStats()->GetSize());
		assert(classSelectionStats != NULL);
		assert(sourceClassSelectionData->GetClassName() == classSelectionStats->GetClassName());
		assert(sourceClassSelectionData->GetClassSelectionOperandData()->GetSize() <=
		       classSelectionStats->GetClassSelectionOperandStats()->GetSize());

		// Parcours des operandes de selection cote donnee et synchronisation avec les operandes cotes stats
		nStatsOperand = 0;
		for (nOperand = 0; nOperand < sourceClassSelectionData->GetClassSelectionOperandData()->GetSize();
		     nOperand++)
		{
			sourceSelectionOperandData =
			    cast(KDClassSelectionOperandData*,
				 sourceClassSelectionData->GetClassSelectionOperandData()->GetAt(nOperand));

			// Acces a l'attribut de selection
			selectionAttribute = sourceSelectionOperandData->GetSelectionAttribute();
			check(selectionAttribute);
			assert(KWType::IsSimple(selectionAttribute->GetType()));

			// Parcours des operandes cote stats jusqu'a trouver l'operande correspondant
			selectionOperandStats = NULL;
			while (nStatsOperand < classSelectionStats->GetClassSelectionOperandStats()->GetSize())
			{
				selectionOperandStats =
				    cast(KDClassSelectionOperandStats*,
					 classSelectionStats->GetClassSelectionOperandStats()->GetAt(nStatsOperand));
				nStatsOperand++;

				// On saute les operandes non utilises, sans attribut de selection
				if (selectionOperandStats->GetSelectionAttribute() != NULL)
					break;
			}
			assert(nStatsOperand <= classSelectionStats->GetClassSelectionOperandStats()->GetSize());
			assert(selectionOperandStats != NULL);
			assert(selectionOperandStats->GetSelectionAttribute() == selectionAttribute);

			// Mise a jour des valeurs de selection selon le type de l'operande de selection
			if (selectionAttribute->GetType() == KWType::Symbol)
			{
				symbolSelectionOperandStats =
				    cast(KDClassSymbolSelectionOperandStats*, selectionOperandStats);

				// Recopie des valeurs collectees, puis nettoyage
				assert(symbolSelectionOperandStats->GetInputData()->GetSize() == 0);
				symbolSelectionOperandStats->GetInputData()->CopyFrom(
				    sourceSelectionOperandData->GetSymbolInputData());
				sourceSelectionOperandData->GetSymbolInputData()->SetSize(0);
			}
			else
			{
				continuousSelectionOperandStats =
				    cast(KDClassContinuousSelectionOperandStats*, selectionOperandStats);

				// Recopie des valeurs collectees, puis nettoyage
				assert(continuousSelectionOperandStats->GetInputData()->GetSize() == 0);
				continuousSelectionOperandStats->GetInputData()->CopyFrom(
				    sourceSelectionOperandData->GetContinuousInputData());
				sourceSelectionOperandData->GetContinuousInputData()->SetSize(0);
			}
		}
	}
}

KWClassDomain* KDSelectionOperandAnalyser::BuildSelectionDomain()
{
	KDClassBuilder classBuilder;
	KWClass* selectionClass;
	KWClassDomain* selectionDomain;
	int nClass;
	KWClass* kwcClass;
	int nAttribute;
	KWAttribute* attribute;
	boolean bIsClassToLoad;
	ObjectDictionary odClassesToLoad;
	NumericKeyDictionary nkdAttributesToLoad;
	ObjectArray oaAttributesToLoad;
	boolean bNewClasses;
	KDClassSelectionStats* classSelectionStats;
	KDClassSelectionOperandStats* classSelectionOperandStats;
	boolean bIsSelectionAttributeUsed;

	require(GetMultiTableFeatureConstruction() != NULL);

	// Parametrage du service de construction de classe
	classBuilder.SetMultiTableFeatureConstruction(GetMultiTableFeatureConstruction());

	// Construction de la classe
	selectionClass = classBuilder.BuildClassFromSelectionRules(GetClass(), this);
	selectionDomain = selectionClass->GetDomain();

	// Identification des classes utilisant (potentiellement recursivement) une classe de selection
	// On parcours les classes du domaine tant qu'il y en une nouvelle ajoutee
	bNewClasses = true;
	while (bNewClasses)
	{
		bNewClasses = false;

		// Parcours du domaine
		for (nClass = 0; nClass < selectionDomain->GetClassNumber(); nClass++)
		{
			kwcClass = selectionDomain->GetClassAt(nClass);

			// Traitement des classes non encore identifiees
			if (odClassesToLoad.Lookup(kwcClass->GetName()) == NULL)
			{
				// Parcours des attributs utilises pour identifier les attributs ObjectArray des classes
				// de selection
				attribute = kwcClass->GetHeadAttribute();
				while (attribute != NULL)
				{
					// Determination si attribut et classe a charger
					bIsClassToLoad = attribute->GetUsed() and
							 KWType::IsRelation(attribute->GetType()) and
							 attribute->GetClass() != NULL;
					if (bIsClassToLoad)
						bIsClassToLoad =
						    odClassesToLoad.Lookup(attribute->GetClass()->GetName()) != NULL or
						    LookupClassSelectionStats(attribute->GetClass()->GetName()) != NULL;

					// Memorisation si classe a charger
					if (bIsClassToLoad)
					{
						// Enregistrement de la classe a charger
						odClassesToLoad.SetAt(kwcClass->GetName(), kwcClass);

						// Memorisation du flag d'insertion
						bNewClasses = true;
					}

					// Attribut suivant
					kwcClass->GetNextAttribute(attribute);
				}
			}
		}
	}

	// Identification des attributs utilisant (potentiellement recursivement) une classe de selection
	for (nClass = 0; nClass < selectionDomain->GetClassNumber(); nClass++)
	{
		kwcClass = selectionDomain->GetClassAt(nClass);

		// Parcours des attributs utilises pour identifier les attributs ObjectArray des classes de selection
		attribute = kwcClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Determination si l'attribut est a charger
			bIsClassToLoad = attribute->GetUsed() and KWType::IsRelation(attribute->GetType()) and
					 attribute->GetClass() != NULL;
			if (bIsClassToLoad)
				bIsClassToLoad = odClassesToLoad.Lookup(attribute->GetClass()->GetName()) != NULL or
						 LookupClassSelectionStats(attribute->GetClass()->GetName()) != NULL;

			// Memorisation de l'attribut a charger
			if (bIsClassToLoad)
				oaAttributesToLoad.Add(attribute);

			// Attribut suivant
			kwcClass->GetNextAttribute(attribute);
		}
	}

	// On commence a mettre tous les attributs du domaine en Unused
	for (nClass = 0; nClass < selectionDomain->GetClassNumber(); nClass++)
	{
		kwcClass = selectionDomain->GetClassAt(nClass);
		kwcClass->SetAllAttributesUsed(false);
	}

	// Ensuite, en met tous les attributs a charger en Used et Loaded
	for (nAttribute = 0; nAttribute < oaAttributesToLoad.GetSize(); nAttribute++)
	{
		attribute = cast(KWAttribute*, oaAttributesToLoad.GetAt(nAttribute));
		attribute->SetUsed(true);
		attribute->SetLoaded(true);
	}

	// On met tous les attributs de selection a charger en Used et Loaded
	for (nClass = 0; nClass < GetClassSelectionStats()->GetSize(); nClass++)
	{
		classSelectionStats = cast(KDClassSelectionStats*, GetClassSelectionStats()->GetAt(nClass));

		// Parametrages des attributs pour les operandes de selection effectivement utilisees
		for (nAttribute = 0; nAttribute < classSelectionStats->GetClassSelectionOperandStats()->GetSize();
		     nAttribute++)
		{
			classSelectionOperandStats =
			    cast(KDClassSelectionOperandStats*,
				 classSelectionStats->GetClassSelectionOperandStats()->GetAt(nAttribute));

			// Parametrage de l'attribut
			assert(classSelectionOperandStats->GetPartitionDimension()->GetUseCount() > 0 or
			       classSelectionOperandStats->GetSelectionAttribute() == NULL);
			if (classSelectionOperandStats->GetSelectionAttribute() != NULL)
			{
				classSelectionOperandStats->GetSelectionAttribute()->SetUsed(true);
				classSelectionOperandStats->GetSelectionAttribute()->SetLoaded(true);
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Simplification de la classe en supprimant tous les attributs calcules en Unused,
	// sauf l'eventuel attribut de selection

	// On met en used l'eventuel attribut de selection, car ils peut avoir une regle de calcul
	bIsSelectionAttributeUsed = false;
	if (GetDatabase()->GetSelectionAttribute() != "")
	{
		attribute = selectionClass->LookupAttribute(GetDatabase()->GetSelectionAttribute());
		bIsSelectionAttributeUsed = attribute->GetUsed();
		attribute->SetUsed(true);
	}

	// Nettoyage du domaine apres la necessaire compilation
	selectionClass->GetDomain()->Compile();
	selectionClass->DeleteUnusedDerivedAttributes(NULL);

	// On remet l'eventuel attribut de selection dans son etat used initial
	if (GetDatabase()->GetSelectionAttribute() != "")
	{
		attribute = selectionClass->LookupAttribute(GetDatabase()->GetSelectionAttribute());
		attribute->SetUsed(bIsSelectionAttributeUsed);
	}
	return selectionDomain;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionStats

KDClassSelectionStats::KDClassSelectionStats()
{
	classCompliantRules = NULL;
	slClassSelectionOperandStats.SetCompareFunction(KDSparseClassSelectionOperandStatsCompare);
	slConstructedPartitions.SetCompareFunction(KDConstructedPartitionCompare);
	nDatabaseObjectNumber = 0;
	nAnalysedObjectNumber = 0;
}

KDClassSelectionStats::~KDClassSelectionStats()
{
	CleanAll();
}

void KDClassSelectionStats::SetClassCompliantRules(const KDClassCompliantRules* classCompliantRulesParam)
{
	classCompliantRules = classCompliantRulesParam;
}

const KDClassCompliantRules* KDClassSelectionStats::GetClassCompliantRules() const
{
	return classCompliantRules;
}

const ALString KDClassSelectionStats::GetClassName() const
{
	if (classCompliantRules == NULL)
		return "";
	else
		return classCompliantRules->GetClassName();
}

const ObjectArray* KDClassSelectionStats::GetClassSelectionOperandStats() const
{
	return &oaClassSelectionOperandStats;
}

KDClassSelectionOperandStats*
KDClassSelectionStats::SearchAttributeClassSelectionOperandStats(const KWAttribute* dimensionAttribute) const
{
	// Les objets de travail searchedClassSelectionOperandStats et searchedPartitionDimension sont en static pour
	// des raisons d'optimisation Cela evite en efet de repasser par le constructeur a chaque appel de la methode
	static KDClassSelectionOperandStats searchedClassSelectionOperandStats;
	static KDConstructedPartitionDimension searchedPartitionDimension;
	debug(KDClassSelectionOperandStats* searchedClassSelectionOperandStatsPointer =
		  &searchedClassSelectionOperandStats);
	KDClassSelectionOperandStats* foundClassSelectionOperandStats;
	POSITION foundPosition;

	require(dimensionAttribute != NULL);

	// Utilisation d'un objet temporaire pour parametrer la recherche
	searchedPartitionDimension.SetAttribute(dimensionAttribute);
	searchedClassSelectionOperandStats.SetPartitionDimension(&searchedPartitionDimension);

	// Recherche en utilisant l'objet temporaire
	foundClassSelectionOperandStats = NULL;
	foundPosition = slClassSelectionOperandStats.Find(&searchedClassSelectionOperandStats);
	if (foundPosition != NULL)
		foundClassSelectionOperandStats =
		    cast(KDClassSelectionOperandStats*, slClassSelectionOperandStats.GetAt(foundPosition));
	debug(ensure(foundClassSelectionOperandStats == NULL or
		     KDSparseClassSelectionOperandStatsCompare(&searchedClassSelectionOperandStatsPointer,
							       &foundClassSelectionOperandStats) == 0));

	// Nettoyage de la cle pour qu'elle n'entraine pas la destruction de l'operande en parametre
	searchedClassSelectionOperandStats.SetPartitionDimension(NULL);
	searchedPartitionDimension.Clean();
	return foundClassSelectionOperandStats;
}

KDClassSelectionOperandStats*
KDClassSelectionStats::SearchRuleClassSelectionOperandStats(const KDConstructedRule* dimensionRule) const
{
	// Les objets de travail searchedClassSelectionOperandStats et searchedPartitionDimension sont en static pour
	// des raison d'optimisation Cela evite en efet de repasser par le constructeur a chaque appel de la methode
	static KDClassSelectionOperandStats searchedClassSelectionOperandStats;
	static KDConstructedPartitionDimension searchedPartitionDimension;
	debug(KDClassSelectionOperandStats* searchedClassSelectionOperandStatsPointer =
		  &searchedClassSelectionOperandStats);
	KDClassSelectionOperandStats* foundClassSelectionOperandStats;
	POSITION foundPosition;

	require(dimensionRule != NULL);

	// Utilisation d'un objet temporaire pour parametrer la recherche
	searchedPartitionDimension.SetRule(dimensionRule);
	searchedClassSelectionOperandStats.SetPartitionDimension(&searchedPartitionDimension);

	// Recherche en utilisant l'objet temporaire
	foundClassSelectionOperandStats = NULL;
	foundPosition = slClassSelectionOperandStats.Find(&searchedClassSelectionOperandStats);
	if (foundPosition != NULL)
		foundClassSelectionOperandStats =
		    cast(KDClassSelectionOperandStats*, slClassSelectionOperandStats.GetAt(foundPosition));
	debug(ensure(foundClassSelectionOperandStats == NULL or
		     KDSparseClassSelectionOperandStatsCompare(&searchedClassSelectionOperandStatsPointer,
							       &foundClassSelectionOperandStats) == 0));

	// Nettoyage de la cle pour qu'elle n'entraine pas la destruction de l'operande en parametre
	searchedClassSelectionOperandStats.SetPartitionDimension(NULL);
	searchedPartitionDimension.Clean();
	return foundClassSelectionOperandStats;
}

KDClassSelectionOperandStats*
KDClassSelectionStats::SearchAttributeClassSelectionOperandStatsFromRule(const KDConstructedRule* dimensionRule) const
{
	KDClassSelectionOperandStats* foundClassSelectionOperandStats;
	const KWAttribute* derivedAttribute;

	require(dimensionRule != NULL);

	// Recherche d'un attribut derive utilise en operande
	// L'objet classOperandDerivedAttributes est utilise pour rechercher les attributs derives, a partir d'une regle
	// de construction
	derivedAttribute = classCompliantRules->LookupConstructedAttribute(dimensionRule);

	// Recherche de la dimension parmi les dimensions de type attributs si l'attribut derive existe et est utilise
	// Si l'attribut etait non utilise, on ignore ce test: l'attribut non utilise sera exploite dans la phase
	// de construction des regles de derivation, qui s'appuiront sur les attribut disponibles, utilises ou non
	foundClassSelectionOperandStats = NULL;
	if (derivedAttribute != NULL and derivedAttribute->GetUsed())
		foundClassSelectionOperandStats = SearchAttributeClassSelectionOperandStats(derivedAttribute);
	return foundClassSelectionOperandStats;
}

KDClassSelectionOperandStats*
KDClassSelectionStats::AddAttributeClassSelectionOperandStats(const KWAttribute* dimensionAttribute)
{
	KDClassSelectionOperandStats* selectionOperandStats;
	KDConstructedPartitionDimension* partitionDimension;

	require(dimensionAttribute != NULL);
	require(SearchAttributeClassSelectionOperandStats(dimensionAttribute) == NULL);

	// Creation et memorisation de la dimension de type attribut
	if (dimensionAttribute->GetType() == KWType::Symbol)
		selectionOperandStats = new KDClassSymbolSelectionOperandStats;
	else
		selectionOperandStats = new KDClassContinuousSelectionOperandStats;
	partitionDimension = new KDConstructedPartitionDimension;
	partitionDimension->SetAttribute(dimensionAttribute);
	partitionDimension->SetClassSelectionOperandStats(selectionOperandStats);
	selectionOperandStats->SetPartitionDimension(partitionDimension);
	oaClassSelectionOperandStats.Add(selectionOperandStats);
	slClassSelectionOperandStats.Add(selectionOperandStats);
	return selectionOperandStats;
}

KDClassSelectionOperandStats*
KDClassSelectionStats::AddRuleClassSelectionOperandStats(const KDConstructedRule* dimensionRule)
{
	KDClassSelectionOperandStats* selectionOperandStats;
	KDConstructedPartitionDimension* partitionDimension;

	require(dimensionRule != NULL);
	require(SearchRuleClassSelectionOperandStats(dimensionRule) == NULL);
	require(SearchAttributeClassSelectionOperandStatsFromRule(dimensionRule) == NULL);

	// Creation et memorisation de la dimension de type regle
	if (dimensionRule->GetType() == KWType::Symbol)
		selectionOperandStats = new KDClassSymbolSelectionOperandStats;
	else
		selectionOperandStats = new KDClassContinuousSelectionOperandStats;
	partitionDimension = new KDConstructedPartitionDimension;
	partitionDimension->SetRule(dimensionRule);
	partitionDimension->SetClassSelectionOperandStats(selectionOperandStats);
	selectionOperandStats->SetPartitionDimension(partitionDimension);
	oaClassSelectionOperandStats.Add(selectionOperandStats);
	slClassSelectionOperandStats.Add(selectionOperandStats);
	return selectionOperandStats;
}

const ObjectArray* KDClassSelectionStats::GetPartitions() const
{
	return &oaConstructedPartitions;
}

KDConstructedPartition* KDClassSelectionStats::LookupPartition(const KDConstructedPartition* partition)
{
	KDConstructedPartition* searchedPartition;
	POSITION searchedPosition;

	require(partition != NULL);
	require(partition->GetPartitionClass()->GetName() == GetClassName());
	require(partition->Check());

	searchedPartition = NULL;
	searchedPosition = slConstructedPartitions.Find(partition);
	if (searchedPosition != NULL)
		searchedPartition = cast(KDConstructedPartition*, slConstructedPartitions.GetAt(searchedPosition));
	return searchedPartition;
}

void KDClassSelectionStats::AddPartition(KDConstructedPartition* partition)
{
	require(partition != NULL);
	require(partition->GetPartitionClass()->GetName() == GetClassName());
	require(LookupPartition(partition) == NULL);

	// Memorisation de la partition
	oaConstructedPartitions.Add(partition);
	slConstructedPartitions.Add(partition);
}

int KDClassSelectionStats::ComputeMaxOperandGranularity() const
{
	int nMaxOperandGranularity;
	const KDConstructedPartition* partition;
	int nPartition;
	int i;

	// Parcours des partition pour trouver la plus fine utilisee
	nMaxOperandGranularity = 0;
	for (nPartition = 0; nPartition < GetPartitions()->GetSize(); nPartition++)
	{
		partition = cast(const KDConstructedPartition*, GetPartitions()->GetAt(nPartition));

		// On ne prend en compte que les partition utilisees
		if (partition->GetUseCount() > 0)
		{
			// Parcours de dimension de la partition
			for (i = 0; i < partition->GetDimensionNumber(); i++)
			{
				// Granularite max
				nMaxOperandGranularity = max(nMaxOperandGranularity, partition->GetGranularityAt(i));
			}
		}
	}
	return nMaxOperandGranularity;
}

void KDClassSelectionStats::SetDatabaseObjectNumber(int nValue)
{
	require(nValue >= 0);
	nDatabaseObjectNumber = nValue;
}

int KDClassSelectionStats::GetDatabaseObjectNumber() const
{
	return nDatabaseObjectNumber;
}

void KDClassSelectionStats::SetAnalysedObjectNumber(int nValue)
{
	require(nValue >= 0);
	nAnalysedObjectNumber = nValue;
}

int KDClassSelectionStats::GetAnalysedObjectNumber() const
{
	return nAnalysedObjectNumber;
}

void KDClassSelectionStats::CleanStats()
{
	int i;
	KDClassSelectionOperandStats* classSelectionOperandStats;

	// Reinitialisationd des statistiques globales
	nDatabaseObjectNumber = 0;
	nAnalysedObjectNumber = 0;

	// Nettoyage des stats des operandes de selection
	for (i = 0; i < oaClassSelectionOperandStats.GetSize(); i++)
	{
		classSelectionOperandStats = cast(KDClassSelectionOperandStats*, oaClassSelectionOperandStats.GetAt(i));
		classSelectionOperandStats->CleanStats();
	}
}

void KDClassSelectionStats::CleanAll()
{
	// Reinitialisationd des statistiques globales
	nDatabaseObjectNumber = 0;
	nAnalysedObjectNumber = 0;

	// Destruction des operandes de selection, et de leur eventuelle regle
	slClassSelectionOperandStats.RemoveAll();
	oaClassSelectionOperandStats.DeleteAll();

	// Destruction des partition construites
	slConstructedPartitions.RemoveAll();
	oaConstructedPartitions.DeleteAll();
}

boolean KDClassSelectionStats::Check() const
{
	boolean bOk = true;
	int i;
	const KDClassSelectionOperandStats* selectionOperandStats;

	// Parcours des operandes de selection de la classe
	for (i = 0; i < oaClassSelectionOperandStats.GetSize(); i++)
	{
		selectionOperandStats =
		    cast(const KDClassSelectionOperandStats*, oaClassSelectionOperandStats.GetAt(i));
		bOk = selectionOperandStats->Check();
		if (not bOk)
			break;
	}
	return bOk;
}

void KDClassSelectionStats::Write(ostream& ost) const
{
	const KDClassSelectionOperandStats* selectionOperandStats;
	const KDConstructedPartition* constructedPartition;
	ObjectArray oaConstructedParts;
	const KDConstructedPart* part;
	int i;
	int nPart;

	// Parcours des stats par classe
	ost << "Dictionary\t" << GetClassName() << endl;
	ost << "Total entity number\t" << GetDatabaseObjectNumber() << endl;
	ost << "Analysed entity number\t" << GetAnalysedObjectNumber() << endl;
	ost << "Selection operands\t" << oaClassSelectionOperandStats.GetSize() << endl;

	// Affichage des operandes de selection
	for (i = 0; i < oaClassSelectionOperandStats.GetSize(); i++)
	{
		selectionOperandStats =
		    cast(const KDClassSelectionOperandStats*, oaClassSelectionOperandStats.GetAt(i));
		ost << "\t" << selectionOperandStats->GetPartitionDimension()->GetUseCount() << "\t"
		    << *selectionOperandStats;
	}

	// Affichage des partitions
	ost << "Partitions\t" << oaConstructedPartitions.GetSize() << endl;
	for (i = 0; i < oaConstructedPartitions.GetSize(); i++)
	{
		constructedPartition = cast(const KDConstructedPartition*, oaConstructedPartitions.GetAt(i));
		ost << "\t" << constructedPartition->GetUseCount() << "\t" << *constructedPartition << endl;

		// Affichages des parties
		constructedPartition->ExportParts(&oaConstructedParts);
		for (nPart = 0; nPart < oaConstructedParts.GetSize(); nPart++)
		{
			part = cast(const KDConstructedPart*, oaConstructedParts.GetAt(nPart));
			ost << "\t\t" << part->GetUseCount() << "\t" << *part << endl;
		}
	}
}

const ALString KDClassSelectionStats::GetClassLabel() const
{
	return "Dictionary selection stats";
}

const ALString KDClassSelectionStats::GetObjectLabel() const
{
	return GetClassName();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSelectionOperandStats

KDClassSelectionOperandStats::KDClassSelectionOperandStats()
{
	partitionDimension = NULL;
	selectionAttribute = NULL;
}

KDClassSelectionOperandStats::~KDClassSelectionOperandStats()
{
	if (partitionDimension != NULL)
		delete partitionDimension;
	CleanStats();
}

void KDClassSelectionOperandStats::SetPartitionDimension(const KDConstructedPartitionDimension* dimension)
{
	require(partitionDimension == NULL or dimension == NULL);
	partitionDimension = dimension;
}

const KDConstructedPartitionDimension* KDClassSelectionOperandStats::GetPartitionDimension() const
{
	return partitionDimension;
}

int KDClassSelectionOperandStats::GetOperandOrigin() const
{
	if (partitionDimension == NULL)
		return KDConstructedRule::None;
	else
		return partitionDimension->GetOrigin();
}

int KDClassSelectionOperandStats::GetOperandType() const
{
	if (partitionDimension == NULL)
		return KWType::Unknown;
	else
		return partitionDimension->GetType();
}

void KDClassSelectionOperandStats::CleanStats()
{
	int i;
	ObjectArray* oaParts;

	ivSelectionGranularities.SetSize(0);
	for (i = 0; i < oaGranularityPartArrays.GetSize(); i++)
	{
		oaParts = cast(ObjectArray*, oaGranularityPartArrays.GetAt(i));
		oaParts->DeleteAll();
	}
	oaGranularityPartArrays.DeleteAll();
}

int KDClassSelectionOperandStats::GetGranularityNumber() const
{
	return ivSelectionGranularities.GetSize();
}

int KDClassSelectionOperandStats::GetGranularityAt(int nGranularityIndex) const
{
	return ivSelectionGranularities.GetAt(nGranularityIndex);
}

int KDClassSelectionOperandStats::GetGranularityExponentAt(int nGranularityIndex) const
{
	int nGranularityExponent;
	nGranularityExponent = GetGranularityExponent(ivSelectionGranularities.GetAt(nGranularityIndex));
	return nGranularityExponent;
}

const ObjectArray* KDClassSelectionOperandStats::GetPartsAt(int nGranularityIndex) const
{
	return cast(const ObjectArray*, oaGranularityPartArrays.GetAt(nGranularityIndex));
}

int KDClassSelectionOperandStats::SearchGranularityIndex(int nGranularity) const
{
	int nLowerIndex;
	int nUpperIndex;
	int nIndex;

	// Si aucune granularite enregistree: retour immediat
	if (ivSelectionGranularities.GetSize() == 0)
		return -1;

	// Initialisation des index extremites
	nLowerIndex = 0;
	nUpperIndex = ivSelectionGranularities.GetSize() - 1;

	// Recherche dichotomique de l'intervalle
	nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	while (nLowerIndex + 1 < nUpperIndex)
	{
		// Deplacement des bornes de recherche en fonction
		// de la comparaison avec la borne courante
		if (nGranularity <= ivSelectionGranularities.GetAt(nIndex))
			nUpperIndex = nIndex;
		else
			nLowerIndex = nIndex;

		// Modification du prochain intervalle teste
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	}
	assert(nLowerIndex <= nUpperIndex);
	assert(nUpperIndex <= nLowerIndex + 1);

	// On compare par rapport aux deux index restant
	if (nGranularity == ivSelectionGranularities.GetAt(nLowerIndex))
		nIndex = nLowerIndex;
	else if (nGranularity == ivSelectionGranularities.GetAt(nUpperIndex))
		nIndex = nUpperIndex;
	else
		nIndex = -1;
	ensure(nIndex == -1 or ivSelectionGranularities.GetAt(nIndex) == nGranularity);
	return nIndex;
}

KDSelectionPart* KDClassSelectionOperandStats::SearchPart(const ObjectArray* oaParts, int nPartIndex) const
{
	int nLowerIndex;
	int nUpperIndex;
	int nIndex;
	KDSelectionPart* selectionPart;
	KDSelectionPart* searchedPart;

	require(oaParts != NULL);

	// Si aucune granularite enregistree: retour immediat
	if (oaParts->GetSize() == 0)
		return NULL;

	// Initialisation des index extremites
	nLowerIndex = 0;
	nUpperIndex = oaParts->GetSize() - 1;

	// Recherche dichotomique de l'intervalle
	nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	while (nLowerIndex + 1 < nUpperIndex)
	{
		selectionPart = cast(KDSelectionPart*, oaParts->GetAt(nIndex));
		assert(nIndex == 0 or
		       selectionPart->GetIndex() > cast(KDSelectionPart*, oaParts->GetAt(nIndex - 1))->GetIndex());

		// Deplacement des bornes de recherche en fonction
		// de la comparaison avec la borne courante
		if (nPartIndex <= selectionPart->GetIndex())
			nUpperIndex = nIndex;
		else
			nLowerIndex = nIndex;

		// Modification du prochain intervalle teste
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	}
	assert(nLowerIndex <= nUpperIndex);
	assert(nUpperIndex <= nLowerIndex + 1);

	// On compare par rapport aux deux index restant
	searchedPart = NULL;
	if (searchedPart == NULL)
	{
		selectionPart = cast(KDSelectionPart*, oaParts->GetAt(nLowerIndex));
		if (nPartIndex == selectionPart->GetIndex())
			searchedPart = selectionPart;
	}
	if (searchedPart == NULL)
	{
		selectionPart = cast(KDSelectionPart*, oaParts->GetAt(nUpperIndex));
		if (nPartIndex == selectionPart->GetIndex())
			searchedPart = selectionPart;
	}
	ensure(searchedPart == NULL or searchedPart->GetIndex() == nPartIndex);
	return searchedPart;
}

boolean KDClassSelectionOperandStats::Check() const
{
	boolean bOk = true;
	int i;
	int nGranularity;
	ObjectArray* oaParts;
	int j;
	KDSelectionPart* previousSelectionPart;
	KDSelectionPart* selectionPart;

	// Verification de la dimension de partition
	if (bOk)
		bOk = partitionDimension != NULL;
	if (bOk)
		bOk = partitionDimension->Check();

	// Verification des resultats
	if (bOk)
	{
		assert(ivSelectionGranularities.GetSize() == oaGranularityPartArrays.GetSize());

		// Verification des granularites
		for (i = 0; i < ivSelectionGranularities.GetSize(); i++)
		{
			nGranularity = ivSelectionGranularities.GetAt(i);
			oaParts = cast(ObjectArray*, oaGranularityPartArrays.GetAt(i));
			check(oaParts);

			// Verification de la granularite
			bOk = bOk and CheckGranularity(nGranularity);
			if (i > 0)
				bOk = bOk and nGranularity > ivSelectionGranularities.GetAt(i - 1);

			// Verification des partiles
			if (bOk)
			{
				for (j = 0; j < oaParts->GetSize(); j++)
				{
					selectionPart = cast(KDSelectionPart*, oaParts->GetAt(j));
					check(selectionPart);

					// Verification de l'index du partile
					bOk = bOk and selectionPart->GetIndex() >= 0;
					bOk = bOk and selectionPart->GetIndex() < nGranularity;

					// Verification du partile
					bOk = bOk and selectionPart->Check();

					// Verification de l'ordre des partiles
					if (j > 0)
					{
						previousSelectionPart = cast(KDSelectionPart*, oaParts->GetAt(j - 1));
						bOk = bOk and
						      previousSelectionPart->GetIndex() < selectionPart->GetIndex();
					}

					// Arret si erreur
					if (not bOk)
						break;
				}
			}

			// Arret si erreur
			if (not bOk)
				break;
		}
	}
	return bOk;
}

boolean KDClassSelectionOperandStats::CheckGranularity(int nGranularity)
{
	boolean bOk = true;
	int nTestGranularity;

	if (nGranularity < 2)
		bOk = false;
	else
	{
		// On passe par une boucle plutot que par des log
		// cela evite les probleme de precsiion numerique et est plus rapide
		nTestGranularity = 2;
		while (nTestGranularity < nGranularity)
			nTestGranularity *= 2;
		bOk = (nTestGranularity == nGranularity);
	}
	return bOk;
}

int KDClassSelectionOperandStats::GetGranularityExponent(int nGranularity)
{
	int nExponent;
	int nTestGranularity;

	require(CheckGranularity(nGranularity));

	// On passe par une boucle plutot que par des log
	// cela evite les probleme de precsiion numerique et est plus rapide
	nExponent = 1;
	nTestGranularity = 2;
	while (nTestGranularity < nGranularity)
	{
		nTestGranularity *= 2;
		nExponent++;
	}
	assert(nTestGranularity == nGranularity);
	assert(fabs(nGranularity - pow(2.0, nExponent)) < 1e-5);
	return nExponent;
}

void KDClassSelectionOperandStats::Write(ostream& ost) const
{
	int i;
	int nGranularity;
	ObjectArray* oaParts;
	int j;
	KDSelectionPart* selectionPart;

	// Affichage de la dimension de partition
	ost << "Selection operand\t";
	if (partitionDimension != NULL)
		ost << *partitionDimension << endl;

	// Affichage par granularite
	for (i = 0; i < ivSelectionGranularities.GetSize(); i++)
	{
		nGranularity = ivSelectionGranularities.GetAt(i);
		oaParts = cast(ObjectArray*, oaGranularityPartArrays.GetAt(i));

		// Affichage la granularite
		ost << "\tGranularity\t" << nGranularity << endl;

		// Affichage des partiles
		if (oaParts != NULL)
		{
			for (j = 0; j < oaParts->GetSize(); j++)
			{
				selectionPart = cast(KDSelectionPart*, oaParts->GetAt(j));

				// Verification de l'index du partile
				if (selectionPart != NULL)
					ost << "\t\t" << selectionPart->GetIndex() << "\t" << *selectionPart << endl;
			}
		}
	}
}

const ALString KDClassSelectionOperandStats::GetClassLabel() const
{
	return "Dictionary selection operand stats";
}

const ALString KDClassSelectionOperandStats::GetObjectLabel() const
{
	return GetPartitionDimension()->GetObjectLabel();
}

void KDClassSelectionOperandStats::SetSelectionAttribute(KWAttribute* attribute)
{
	selectionAttribute = attribute;
	ensure(attribute == NULL or CheckSelectionAttribute());
}

KWAttribute* KDClassSelectionOperandStats::GetSelectionAttribute() const
{
	ensure(selectionAttribute == NULL or CheckSelectionAttribute());
	return selectionAttribute;
}

boolean KDClassSelectionOperandStats::CheckSelectionAttribute() const
{
	boolean bOk = true;

	require(partitionDimension != NULL);
	require(partitionDimension->GetOrigin() != KDConstructedPartitionDimension::None);
	require(selectionAttribute != NULL);

	// Cas d'une dimension attribut
	if (partitionDimension->GetOrigin() == KDConstructedPartitionDimension::Attribute)
	{
		bOk = (selectionAttribute->GetName() == partitionDimension->GetAttribute()->GetName());
		if (bOk)
			bOk = (selectionAttribute->GetType() == partitionDimension->GetAttribute()->GetType());
	}
	// Cas d'une dimension regle
	else if (partitionDimension->GetOrigin() == KDConstructedPartitionDimension::Attribute)
	{
		// L'attribut doit etre calcule
		bOk = (selectionAttribute->GetDerivationRule() != NULL);

		// Sa regle doit etre la meme que celle de la dimension de partition
		if (bOk)
			bOk = partitionDimension->GetRule()->CompareWithDerivationRule(
			    selectionAttribute->GetDerivationRule());
	}
	return bOk;
}

void KDClassSelectionOperandStats::AnalysePartStats(int nMaxPartileNumber, int nMinPartileFrequency) {}

void KDClassSelectionOperandStats::CleanInputData() {}

int KDSparseClassSelectionOperandStatsCompare(const void* elem1, const void* elem2)
{
	KDClassSelectionOperandStats* selectionOperandStast1;
	KDClassSelectionOperandStats* selectionOperandStast2;
	const KDConstructedPartitionDimension* partitionDimension1;
	const KDConstructedPartitionDimension* partitionDimension2;

	// Acces aux donnees a comparer
	selectionOperandStast1 = cast(KDClassSelectionOperandStats*, *(Object**)elem1);
	selectionOperandStast2 = cast(KDClassSelectionOperandStats*, *(Object**)elem2);

	// Acces aux partitions
	partitionDimension1 = selectionOperandStast1->GetPartitionDimension();
	partitionDimension2 = selectionOperandStast2->GetPartitionDimension();

	// Comparaison de leur operande
	return KDConstructedPartitionDimensionCompare(&partitionDimension1, &partitionDimension2);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassContinuousSelectionOperandStats

KDClassContinuousSelectionOperandStats::KDClassContinuousSelectionOperandStats() {}

KDClassContinuousSelectionOperandStats::~KDClassContinuousSelectionOperandStats() {}

boolean KDClassContinuousSelectionOperandStats::Check() const
{
	boolean bOk = true;
	int i;
	ObjectArray* oaParts;
	int j;
	SortedList slAllIntervals(KDSparseSelectionIntervalCompare);
	KDSelectionInterval* previousSelectionInterval;
	KDSelectionInterval* selectionInterval;

	// Classe ancetre
	bOk = KDClassSelectionOperandStats::Check();

	// Verification de la coherence de l'ensemble des partiles
	if (bOk)
	{
		// Verification par granularite
		for (i = 0; i < ivSelectionGranularities.GetSize(); i++)
		{
			oaParts = cast(ObjectArray*, oaGranularityPartArrays.GetAt(i));
			check(oaParts);

			// Verification de l'ordre des intervalles et de leur unicite globale
			for (j = 1; j < oaParts->GetSize(); j++)
			{
				previousSelectionInterval = cast(KDSelectionInterval*, oaParts->GetAt(j - 1));
				selectionInterval = cast(KDSelectionInterval*, oaParts->GetAt(j));

				// Verification de l'ordre des bornes entre intervalles successifs
				bOk = bOk and
				      previousSelectionInterval->GetUpperBound() <= selectionInterval->GetLowerBound();

				// Verification de l'unicite globale des intervalles
				bOk = bOk and slAllIntervals.Find(selectionInterval) == NULL;

				// Arret si erreur
				if (not bOk)
					break;

				// Enregistrement de l'intervalle
				slAllIntervals.Add(selectionInterval);
			}
		}
	}
	return bOk;
}

void KDClassContinuousSelectionOperandStats::Write(ostream& ost) const
{
	const int nMaxValueNumber = 100;
	ContinuousVector cvSortedValues;
	KWQuantileIntervalBuilder quantileBuilder;
	int i;
	int nValueNumber;

	// Methode ancetre
	KDClassSelectionOperandStats::Write(ost);

	// Affichage des valeurs
	if (cvInputData.GetSize() > 0)
	{
		// Tri des valeurs
		cvSortedValues.CopyFrom(&cvInputData);
		cvSortedValues.Sort();

		// Initialisation d'un quantile builder pour trier les valeur par frequence decroissante
		quantileBuilder.InitializeValues(&cvSortedValues);
		nValueNumber = quantileBuilder.GetValueNumber();

		ost << "\tInput values\t" << nValueNumber << endl;
		ost << "\t\tIndex\tValue\tFrequency\n";
		for (i = 0; i < nValueNumber; i++)
		{
			ost << "\t\t" << i + 1 << "\t" << quantileBuilder.GetValueAt(i) << "\t"
			    << quantileBuilder.GetValueFrequencyAt(i) << endl;
			if (i >= nMaxValueNumber)
			{
				ost << "\t\t...\n";
				break;
			}
		}
	}
}

ContinuousVector* KDClassContinuousSelectionOperandStats::GetInputData()
{
	return &cvInputData;
}

void KDClassContinuousSelectionOperandStats::AnalysePartStats(int nMaxPartileNumber, int nMinPartileFrequency)
{
	boolean bDisplay = false;
	KWQuantileIntervalBuilder quantileBuilder;
	int nInstanceNumber;
	int nValueNumber;
	int nGranularity;
	ObjectArray oaParts;
	ObjectArray* oaGranularityParts;
	SortedList slAllIntervals(KDSparseSelectionIntervalCompare);
	int nPart;
	KDSelectionInterval candidateSelectionInterval;
	KDSelectionInterval* selectionInterval;

	require(nMaxPartileNumber > 0);
	require(nMinPartileFrequency > 0);

	// Nettoyage prealable du resultat
	CleanStats();

	// Tri des valeurs pour permettre l'analyse en quantiles
	cvInputData.Sort();

	// Initialisation d'un quantile builder
	quantileBuilder.InitializeValues(&cvInputData);
	nInstanceNumber = quantileBuilder.GetInstanceNumber();
	nValueNumber = quantileBuilder.GetValueNumber();

	// Parcours des granularites par puissance de deux pour fabriquer les partitions
	nGranularity = 2;
	while (nGranularity <= nInstanceNumber / nMinPartileFrequency and nGranularity <= nMaxPartileNumber)
	{
		// Calcul des partiles pour une granularite donnee
		quantileBuilder.ComputeQuantiles(nGranularity);

		// Memorisation de la partition, si elle contient des intervalles nouveaux
		if (quantileBuilder.GetIntervalNumber() > 1)
		{
			// Creation d'un nouveau tableau de parties
			oaGranularityParts = new ObjectArray;

			// Ajout des parties non deja enregistrees
			for (nPart = 0; nPart < quantileBuilder.GetIntervalNumber(); nPart++)
			{
				// Parametrage d'un intervalle de selection
				candidateSelectionInterval.SetLowerBound(
				    quantileBuilder.GetIntervalLowerBoundAt(nPart));
				candidateSelectionInterval.SetUpperBound(
				    quantileBuilder.GetIntervalUpperBoundAt(nPart));
				candidateSelectionInterval.SetIndex(quantileBuilder.GetIntervalQuantileIndexAt(nPart));

				// Enregistrement et ajout de l'intervalle si intervalle nouveau
				if (slAllIntervals.Find(&candidateSelectionInterval) == NULL)
				{
					selectionInterval = candidateSelectionInterval.Clone();
					slAllIntervals.Add(selectionInterval);
					oaGranularityParts->Add(selectionInterval);
				}
			}

			// Memorisation du tableau de partie pour cette granularite si au moins un nouvel intervalle
			if (oaGranularityParts->GetSize() > 0)
			{
				ivSelectionGranularities.Add(nGranularity);
				oaGranularityPartArrays.Add(oaGranularityParts);
			}
			// Sinon, destruction du tableau de parties
			else
			{
				oaGranularityParts->DeleteAll();
				delete oaGranularityParts;
			}
		}

		// Arret si on a epuise toutes les valeurs
		if (quantileBuilder.GetIntervalNumber() == nValueNumber)
			break;

		// Changement de granularite
		nGranularity *= 2;
	}

	// Affichage
	if (bDisplay)
		Write(cout);
	ensure(Check());
}

void KDClassContinuousSelectionOperandStats::CleanInputData()
{
	cvInputData.SetSize(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDClassSymbolSelectionOperandStats

KDClassSymbolSelectionOperandStats::KDClassSymbolSelectionOperandStats() {}

KDClassSymbolSelectionOperandStats::~KDClassSymbolSelectionOperandStats() {}

boolean KDClassSymbolSelectionOperandStats::Check() const
{
	boolean bOk = true;
	int i;
	ObjectArray* oaParts;
	int j;
	KDSelectionValue* selectionValue;
	NumericKeyDictionary nkdSelectionValues;

	// Classe ancetre
	bOk = KDClassSelectionOperandStats::Check();

	// Verification de l'unicite des valeurs sur l'ensemble de toutes les granularites
	if (bOk)
	{
		// Affichage par granularite
		for (i = 0; i < ivSelectionGranularities.GetSize(); i++)
		{
			oaParts = cast(ObjectArray*, oaGranularityPartArrays.GetAt(i));
			check(oaParts);

			// Verification de l'unicite des valeurs
			for (j = 1; j < oaParts->GetSize(); j++)
			{
				selectionValue = cast(KDSelectionValue*, oaParts->GetAt(j));

				// Verification de l'unicite globale des valeurs
				bOk = bOk and
				      nkdSelectionValues.Lookup(selectionValue->GetValue().GetNumericKey()) == NULL;

				// Arret si erreur
				if (not bOk)
					break;

				// Enregistrement de la valeur
				nkdSelectionValues.SetAt(selectionValue->GetValue().GetNumericKey(), selectionValue);
			}
		}
	}
	return bOk;
}

void KDClassSymbolSelectionOperandStats::Write(ostream& ost) const
{
	const int nMaxValueNumber = 100;
	SymbolVector svSortedValues;
	KWQuantileGroupBuilder quantileBuilder;
	int i;
	int nValueNumber;
	Symbol sValue;

	// Methode ancetre
	KDClassSelectionOperandStats::Write(ost);

	// Affichage des valeurs
	if (svInputData.GetSize() > 0)
	{
		// Tri des valeurs
		svSortedValues.CopyFrom(&svInputData);
		svSortedValues.SortKeys();

		// Initialisation d'un quantile builder pour trier les valeur par frequence decroissante
		quantileBuilder.InitializeValues(&svSortedValues);
		nValueNumber = quantileBuilder.GetValueNumber();

		ost << "\tInput values\t" << nValueNumber << endl;
		ost << "\t\tIndex\tValue\tFrequency\n";
		for (i = 0; i < nValueNumber; i++)
		{
			ost << "\t\t" << i + 1 << "\t" << quantileBuilder.GetValueAt(i) << "\t"
			    << quantileBuilder.GetValueFrequencyAt(i) << endl;
			if (i >= nMaxValueNumber)
			{
				ost << "\t\t...\n";
				break;
			}
		}
	}
}

SymbolVector* KDClassSymbolSelectionOperandStats::GetInputData()
{
	return &svInputData;
}

void KDClassSymbolSelectionOperandStats::AnalysePartStats(int nMaxPartileNumber, int nMinPartileFrequency)
{
	boolean bDisplay = false;
	KWQuantileGroupBuilder quantileBuilder;
	int nInstanceNumber;
	int nValueNumber;
	int nLastGroupNumber;
	int i;
	int nGranularity;
	ObjectArray* oaGranularityParts;
	int nPart;
	KDSelectionValue* selectionValue;

	require(nMaxPartileNumber > 0);
	require(nMinPartileFrequency > 0);

	// Nettoyage prealable du resultat
	CleanStats();

	// Tri des valeurs par cle (methode la plus rapide)
	svInputData.SortKeys();

	// Initialisation d'un quantile builder
	quantileBuilder.InitializeValues(&svInputData);
	nInstanceNumber = quantileBuilder.GetInstanceNumber();
	nValueNumber = quantileBuilder.GetValueNumber();

	// Parcours des granularites par puissance de deux pour fabriquer les partitions
	nGranularity = 2;
	nLastGroupNumber = 1;
	while (nGranularity <= nInstanceNumber / nMinPartileFrequency and nGranularity <= nMaxPartileNumber)
	{
		// Calcul des partiles pour une granularite donnee
		quantileBuilder.ComputeQuantiles(nGranularity);

		// Memorisation de la partition, si elle contient des groupes nouveaux
		if (quantileBuilder.GetGroupNumber() > nLastGroupNumber)
		{
			// Creation d'un nouveau tableau de parties
			oaGranularityParts = new ObjectArray;

			// Ajout des parties non deja enregistrees
			for (nPart = nLastGroupNumber - 1; nPart < quantileBuilder.GetGroupNumber(); nPart++)
			{
				// Creation d'une valeur de selection a partir de groupe
				selectionValue = new KDSelectionValue;
				selectionValue->SetValue(
				    quantileBuilder.GetValueAt(quantileBuilder.GetGroupFirstValueIndexAt(nPart)));
				selectionValue->SetIndex(quantileBuilder.GetGroupQuantileIndexAt(nPart));
				oaGranularityParts->Add(selectionValue);

				// Cas particulier du groupe poubelle
				if (nPart == quantileBuilder.GetGroupNumber() - 1)
				{
					assert(nPart == quantileBuilder.GetGroupNumber() - 1);

					// Initialisation de valeurs hors groupes: celles des groupes precedents
					for (i = 0; i < nPart; i++)
						selectionValue->GetOutsideValues()->Add(quantileBuilder.GetValueAt(i));
				}
			}

			// Memorisation du tableau de partie pour cette granularite si au moins une nouvelle partie
			assert(oaGranularityParts->GetSize() > 0);
			ivSelectionGranularities.Add(nGranularity);
			oaGranularityPartArrays.Add(oaGranularityParts);

			// On memorise le dernier nombre de groupes, qui doit etre depasse pour avoir de nouveaux
			// groupes distincts
			nLastGroupNumber = quantileBuilder.GetGroupNumber();
		}

		// Arret si on a epuise toutes les valeurs
		if (quantileBuilder.GetGroupNumber() == nValueNumber)
			break;

		// Changement de granularite
		nGranularity *= 2;
	}

	// Affichage
	if (bDisplay)
		Write(cout);
	ensure(Check());
}

void KDClassSymbolSelectionOperandStats::CleanInputData()
{
	svInputData.SetSize(0);
}

//////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionPart

KDSelectionPart::KDSelectionPart()
{
	nIndex = 0;
}

KDSelectionPart::~KDSelectionPart() {}

void KDSelectionPart::SetIndex(int nValue)
{
	require(nValue >= 0);
	nIndex = nValue;
}

int KDSelectionPart::GetIndex() const
{
	return nIndex;
}

/////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionInterval

KDSelectionInterval::KDSelectionInterval()
{
	cLowerBound = 0;
	cUpperBound = 0;
}

KDSelectionInterval::~KDSelectionInterval() {}

void KDSelectionInterval::SetLowerBound(Continuous cValue)
{
	cLowerBound = cValue;
}

Continuous KDSelectionInterval::GetLowerBound() const
{
	return cLowerBound;
}

void KDSelectionInterval::SetUpperBound(Continuous cValue)
{
	cUpperBound = cValue;
}

Continuous KDSelectionInterval::GetUpperBound() const
{
	return cUpperBound;
}

void KDSelectionInterval::CopyFrom(const KDSelectionInterval* aSource)
{
	require(aSource != NULL);

	nIndex = aSource->nIndex;
	cLowerBound = aSource->cLowerBound;
	cUpperBound = aSource->cUpperBound;
}

KDSelectionInterval* KDSelectionInterval::Clone() const
{
	KDSelectionInterval* aClone;

	aClone = new KDSelectionInterval;
	aClone->CopyFrom(this);
	return aClone;
}

boolean KDSelectionInterval::Check() const
{
	boolean bOk = true;

	// Test des bornes: cas missing
	if (cUpperBound == KWContinuous::GetMissingValue())
	{
		if (cLowerBound != KWContinuous::GetMissingValue())
			bOk = false;
	}
	// Cas standard
	// Verification exacte, le Check servant a filtrer les intervalles de selection inutiles
	else if (cLowerBound >= cUpperBound)
		bOk = false;
	return bOk;
}

void KDSelectionInterval::Write(ostream& ost) const
{
	ALString sLabel;

	// Cas particulier d'un intervalle reduit a la valeur manquante
	if (cUpperBound == KWContinuous::GetMissingValue())
		ost << "Missing";
	// Cas standard
	else
	{
		if (cLowerBound == KWContinuous::GetMissingValue())
			ost << "]-inf;";
		else
			ost << "]" << KWContinuous::ContinuousToString(cLowerBound) << ";";
		if (cUpperBound == KWContinuous::GetMaxValue())
			ost << "+inf[";
		else
			ost << KWContinuous::ContinuousToString(cUpperBound) << "]";
	}
}

int KDSparseSelectionIntervalCompare(const void* elem1, const void* elem2)
{
	KDSelectionInterval* interval1;
	KDSelectionInterval* interval2;
	int nCompare;

	// Comparaison sur le critere de tri
	interval1 = cast(KDSelectionInterval*, *(Object**)elem1);
	interval2 = cast(KDSelectionInterval*, *(Object**)elem2);

	// Comparaison des intervalles
	nCompare = KWContinuous::Compare(interval1->GetLowerBound(), interval2->GetLowerBound());
	if (nCompare == 0)
		nCompare = KWContinuous::Compare(interval1->GetUpperBound(), interval2->GetUpperBound());
	return nCompare;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionValue

KDSelectionValue::KDSelectionValue() {}

KDSelectionValue::~KDSelectionValue() {}

void KDSelectionValue::SetValue(const Symbol& sValue)
{
	sSymbolValue = sValue;
}

Symbol& KDSelectionValue::GetValue() const
{
	return sSymbolValue;
}

SymbolVector* KDSelectionValue::GetOutsideValues()
{
	return &svOutsideValues;
}

boolean KDSelectionValue::IsGarbagePart() const
{
	return svOutsideValues.GetSize() > 0;
}

void KDSelectionValue::CopyFrom(const KDSelectionValue* aSource)
{
	require(aSource != NULL);

	nIndex = aSource->nIndex;
	sSymbolValue = aSource->sSymbolValue;
	svOutsideValues.CopyFrom(&(aSource->svOutsideValues));
}

KDSelectionValue* KDSelectionValue::Clone() const
{
	KDSelectionValue* aClone;

	aClone = new KDSelectionValue;
	aClone->CopyFrom(this);
	return aClone;
}

boolean KDSelectionValue::Check() const
{
	boolean bOk = true;
	NumericKeyDictionary nkdValues;
	int nValue;

	// Test d'unicite des valeurs: a tester uniquement pour le partie poubelle
	if (svOutsideValues.GetSize() > 0)
	{
		// Ajout de toutes les valeurs dans le dictionnaire
		nkdValues.SetAt(sSymbolValue.GetNumericKey(), &nkdValues);
		for (nValue = 0; nValue < svOutsideValues.GetSize(); nValue++)
			nkdValues.SetAt(svOutsideValues.GetAt(nValue).GetNumericKey(), &nkdValues);

		// Il y a unicite si le dictionnaire contient autant de valeurs qu'il y en a de potentielles
		bOk = (nkdValues.GetCount() == 1 + svOutsideValues.GetSize());
	}
	return bOk;
}

void KDSelectionValue::Write(ostream& ost) const
{
	int nValue;

	// Cas d'une valeur unique
	if (svOutsideValues.GetSize() == 0)
		ost << "{" << sSymbolValue << "}";
	// Cas de plusieurs valeurs
	else
	{
		ost << "{" << sSymbolValue << "," << Symbol::GetStarValue() << "} = not {";
		for (nValue = 0; nValue < svOutsideValues.GetSize(); nValue++)
		{
			if (nValue > 0)
				ost << ", ";
			ost << svOutsideValues.GetAt(nValue);
		}
		cout << "}}";
	}
}

int KDSparseSelectionValueCompare(const void* elem1, const void* elem2)
{
	KDSelectionValue* value1;
	KDSelectionValue* value2;
	int nCompare;
	int nValue;

	// Comparaison sur le critere de tri
	value1 = cast(KDSelectionValue*, *(Object**)elem1);
	value2 = cast(KDSelectionValue*, *(Object**)elem2);

	// Comparaison des valeurs
	nCompare = value1->GetValue().Compare(value2->GetValue());

	// Si egalite, comparaison sur les valeurs hors du partile
	if (nCompare == 0)
		nCompare = value1->GetOutsideValues()->GetSize() - value2->GetOutsideValues()->GetSize();
	if (nCompare == 0)
	{
		for (nValue = 0; nValue < value1->GetOutsideValues()->GetSize(); nValue++)
		{
			nCompare = value1->GetOutsideValues()->GetAt(nValue).Compare(
			    value2->GetOutsideValues()->GetAt(nValue));
			if (nCompare != 0)
				break;
		}
	}
	return nCompare;
}
