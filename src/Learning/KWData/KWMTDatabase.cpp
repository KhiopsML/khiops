// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMTDatabase.h"

KWMTDatabase::KWMTDatabase()
{
	nSkippedRecordNumber = 0;
	mainMultiTableMapping = new KWMTDatabaseMapping;
	oaMultiTableMappings.SetSize(1);
	oaMultiTableMappings.SetAt(0, mainMultiTableMapping);
	dataTableDriverCreator = new KWDataTableDriver;
	lExternalTablesEncodingErrorNumber = 0;
	nFreshness = 0;
	nCheckReadFreshness = 0;
	nCheckWriteFreshness = 0;
	bCheckRead = false;
	bCheckWrite = false;
}

KWMTDatabase::~KWMTDatabase()
{
	int nReference;
	KWMTDatabaseMapping* referenceMapping;

	// Nettoyage prealable du mapping physique
	assert(mainMultiTableMapping == oaMultiTableMappings.GetAt(0));
	DMTMPhysicalTerminateMapping(mainMultiTableMapping);
	for (nReference = 0; nReference < oaRootReferenceTableMappings.GetSize(); nReference++)
	{
		referenceMapping = cast(KWMTDatabaseMapping*, oaRootReferenceTableMappings.GetAt(nReference));
		DMTMPhysicalTerminateMapping(referenceMapping);
	}
	assert(objectReferenceResolver.GetClassNumber() == 0);

	// Destruction des mappings, y compris du mapping principal
	oaMultiTableMappings.DeleteAll();

	// Destruction du driver
	delete dataTableDriverCreator;
}

KWDatabase* KWMTDatabase::Create() const
{
	return new KWMTDatabase;
}

void KWMTDatabase::CopyFrom(const KWDatabase* kwdSource)
{
	const KWMTDatabase* kwmtdSource = cast(KWMTDatabase*, kwdSource);
	KWMTDatabaseMapping* mapping;
	KWMTDatabaseMapping* mappingComponent;
	KWMTDatabaseMapping* mappingCopy;
	int i;
	int j;

	// Copie standard
	KWDatabase::CopyFrom(kwdSource);
	lExternalTablesEncodingErrorNumber = 0;
	nSkippedRecordNumber = 0;

	// Copie des parametres du driver
	dataTableDriverCreator->CopyFrom(kwmtdSource->dataTableDriverCreator);

	// Copie du mapping, apres avoir reinitialise la table de mapping
	assert(oaMultiTableMappings.GetSize() >= 1);
	assert(kwmtdSource->oaMultiTableMappings.GetSize() >= 1);
	oaMultiTableMappings.DeleteAll();
	for (i = 0; i < kwmtdSource->oaMultiTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, kwmtdSource->oaMultiTableMappings.GetAt(i));
		oaMultiTableMappings.Add(mapping->Clone());
	}

	// Memorisation du mapping principal
	mainMultiTableMapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(0));

	// Memorisation des mappings principaux des classes referencees
	oaRootReferenceTableMappings.SetSize(0);
	for (i = 0; i < kwmtdSource->oaRootReferenceTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, kwmtdSource->oaRootReferenceTableMappings.GetAt(i));
		assert(mapping != kwmtdSource->mainMultiTableMapping);
		assert(mapping->GetDataPathAttributeNames() == "");

		// Recherche de la copie du mapping source
		mappingCopy = LookupMultiTableMapping(mapping->GetDataPath());
		assert(mappingCopy->GetDataPathAttributeNames() == "");
		assert(mappingCopy->GetDataPath() == mapping->GetDataPath());

		// Insertion dans le tableau des mappings principaux des classes referencees
		oaRootReferenceTableMappings.Add(mappingCopy);
	}

	// Reconstruction de la structure des mapping, qui connaissent chacun les mapping de leur composition
	for (i = 0; i < kwmtdSource->oaMultiTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, kwmtdSource->oaMultiTableMappings.GetAt(i));

		// Recherche de la copie du mapping source
		mappingCopy = LookupMultiTableMapping(mapping->GetDataPath());

		// Reconstitution du tableau des mapping des classes de la composition
		for (j = 0; j < mapping->GetComponentTableMappings()->GetSize(); j++)
		{
			mappingComponent = cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(j));

			// Insertion de la copie du mapping composant correspondant
			mappingCopy->GetComponentTableMappings()->Add(
			    LookupMultiTableMapping(mappingComponent->GetDataPath()));
		}
		assert(mappingCopy->GetComponentTableMappings()->GetSize() ==
		       mapping->GetComponentTableMappings()->GetSize());
	}

	// Memorisation des warnings pour les dictionnaires racines non utilises
	svUnusedRootDictionaryWarnings.CopyFrom(&kwmtdSource->svUnusedRootDictionaryWarnings);

	// Memorisation des indicateurs de fraicheur
	nFreshness = kwmtdSource->nFreshness;
	nCheckReadFreshness = kwmtdSource->nCheckReadFreshness;
	nCheckWriteFreshness = kwmtdSource->nCheckWriteFreshness;
	bCheckRead = kwmtdSource->bCheckRead;
	bCheckWrite = kwmtdSource->bCheckWrite;
}

int KWMTDatabase::Compare(const KWDatabase* kwdSource) const
{
	int nCompare;
	const KWMTDatabase* kwmtdSource = cast(KWMTDatabase*, kwdSource);
	KWMTDatabaseMapping* mapping;
	KWMTDatabaseMapping* sourceMapping;
	int i;

	// Comparaison de base
	nCompare = KWDatabase::Compare(kwdSource);

	// Comparaison du driver
	if (nCompare == 0)
		nCompare = dataTableDriverCreator->Compare(kwmtdSource->dataTableDriverCreator);

	// Comparaison du mapping
	if (nCompare == 0)
		nCompare = oaMultiTableMappings.GetSize() - kwmtdSource->oaMultiTableMappings.GetSize();
	if (nCompare == 0)
	{
		for (i = 0; i < kwmtdSource->oaMultiTableMappings.GetSize(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));
			sourceMapping = cast(KWMTDatabaseMapping*, kwmtdSource->oaMultiTableMappings.GetAt(i));

			// Comparaison et arret si difference
			nCompare = mapping->Compare(sourceMapping);
			if (nCompare != 0)
				break;
		}
	}
	return nCompare;
}

void KWMTDatabase::SetDatabaseName(const ALString& sValue)
{
	assert(oaMultiTableMappings.GetAt(0) == mainMultiTableMapping);
	KWDatabase::SetDatabaseName(sValue);
	mainMultiTableMapping->SetDataTableName(sValue);
	nFreshness++;
}

const ALString& KWMTDatabase::GetDatabaseName() const
{
	assert(oaMultiTableMappings.GetAt(0) == mainMultiTableMapping);
	return mainMultiTableMapping->GetDataTableName();
}

void KWMTDatabase::SetClassName(const ALString& sValue)
{
	assert(oaMultiTableMappings.GetAt(0) == mainMultiTableMapping);

	KWDatabase::SetClassName(sValue);
	mainMultiTableMapping->SetClassName(sValue);
	mainMultiTableMapping->SetOriginClassName(sValue);
	nFreshness++;
	ensure(GetClassName() == sValue);
}

const ALString& KWMTDatabase::GetClassName() const
{
	assert(oaMultiTableMappings.GetAt(0) == mainMultiTableMapping);
	return mainMultiTableMapping->GetClassName();
}

ObjectArray* KWMTDatabase::GetMultiTableMappings()
{
	assert(oaMultiTableMappings.GetAt(0) == mainMultiTableMapping);
	return &oaMultiTableMappings;
}

int KWMTDatabase::GetTableNumber() const
{
	return oaMultiTableMappings.GetSize();
}

int KWMTDatabase::GetMainTableNumber() const
{
	int nMainTableNumber;
	KWMTDatabaseMapping* mapping;
	int i;

	// Parcours de la table de mapping
	nMainTableNumber = 0;
	for (i = 0; i < oaMultiTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));

		// On s'arrete a la premiere table referencee
		if (IsReferencedClassMapping(mapping))
			break;
		// Sinon, on incremente le nombre de tables principales
		else
			nMainTableNumber++;
	}
	return nMainTableNumber;
}

int KWMTDatabase::GetReferencedTableNumber() const
{
	return GetTableNumber() - GetMainTableNumber();
}

KWMTDatabaseMapping* KWMTDatabase::LookupMultiTableMapping(const ALString& sDataPath) const
{
	KWMTDatabaseMapping* mapping;
	int i;

	// Parcours de la table de mapping
	for (i = 0; i < oaMultiTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));
		if (mapping->GetDataPath() == sDataPath)
			return mapping;
	}
	return NULL;
}

boolean KWMTDatabase::IsReferencedClassMapping(const KWMTDatabaseMapping* mapping) const
{
	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping or not Check());
	return (mapping->GetOriginClassName() != GetClassName());
}

// Fonction de comparaison sur le nom de la premiere classe (principale) d'un table de mapping
// Permet d'avoir les mappings tries selon leur classe principale
int KWMTDatabaseCompareMappingMainClass(const void* first, const void* second)
{
	ObjectArray* aFirst;
	ObjectArray* aSecond;
	KWMTDatabaseMapping* firstMapping;
	KWMTDatabaseMapping* secondMapping;
	int nResult;

	aFirst = cast(ObjectArray*, *(Object**)first);
	aSecond = cast(ObjectArray*, *(Object**)second);
	firstMapping = cast(KWMTDatabaseMapping*, aFirst->GetAt(0));
	secondMapping = cast(KWMTDatabaseMapping*, aSecond->GetAt(0));
	nResult = firstMapping->GetClassName().Compare(secondMapping->GetClassName());
	return nResult;
}

void KWMTDatabase::UpdateMultiTableMappings()
{
	const boolean bTrace = false;
	static ALString sLastUpdatedClassName;
	KWClass* mainClass;
	ObjectArray oaPreviousMultiTableMappings;
	ObjectDictionary odReferenceClasses;
	ObjectArray oaRankedReferenceClasses;
	ObjectDictionary odAnalysedCreatedClasses;
	ObjectDictionary odWorkingReferenceClasses;
	ObjectArray oaWorkingRankedReferenceClasses;
	ObjectDictionary odWorkingAnalysedCreatedClasses;
	ObjectArray oaWorkingCreatedMappings;
	KWClass* referenceClass;
	StringVector svAttributeName;
	KWMTDatabaseMapping* mapping;
	KWMTDatabaseMapping* previousMapping;
	ObjectArray oaAllRootCreatedMappings;
	ObjectArray* oaCreatedMappings;
	int i;
	int j;
	boolean bIsRootDictionaryUsable;
	ALString sWarning;

	require(oaMultiTableMappings.GetAt(0) == mainMultiTableMapping);
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());

	// Nettoyage prealable du mapping physique
	DMTMPhysicalTerminateMapping(mainMultiTableMapping);

	// Recherche du dictionnaire associe a la base
	mainClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

	// Ajout d'un message d'erreur si on parametre un dictionnaire inexistant
	// Permet de generer une erreur explicite dans les scenarios dans ce cas
	// Sinon, on obtient une erreur fatale en essayant ensuite de parametrer
	// les fichiers d'une base multi-table
	if (GetClassName() != "" and mainClass == NULL)
	{
		// On evite les messages d'erreur repete en memorisant le dernier dictionnaire manquant
		// Sinon, les bases d'apprentissage et de test, ainsi que les multiples Refresh appelant
		// cette methode generent des erreurs qui doublonnent
		if (sLastUpdatedClassName != GetClassName())
			AddError("Dictionary " + GetClassName() + " not found");
	}
	sLastUpdatedClassName = GetClassName();

	// Si pas de dictionnaire associe, on nettoie le mapping
	if (mainClass == NULL)
	{
		// Duplication prealable du mapping principal (sans les attributs de gestion)
		mainMultiTableMapping = cast(KWMTDatabaseMapping*, mainMultiTableMapping->Clone());

		// Nettoyage
		oaMultiTableMappings.DeleteAll();
		oaRootReferenceTableMappings.SetSize(0);
		svUnusedRootDictionaryWarnings.SetSize(0);

		// On rajoute le mapping principal
		oaMultiTableMappings.Add(mainMultiTableMapping);
	}
	// Sinon, parcours des champs du dictionnaire pour rechercher les mappings a specifier
	else
	{
		// Memorisation des anciens mappings
		oaPreviousMultiTableMappings.CopyFrom(&oaMultiTableMappings);

		// Dereferencement des mappings en cours
		mainMultiTableMapping = NULL;
		oaMultiTableMappings.SetSize(0);
		oaRootReferenceTableMappings.SetSize(0);
		svUnusedRootDictionaryWarnings.SetSize(0);

		// Creation du mapping de la table principale
		assert(svAttributeName.GetSize() == 0);
		mainMultiTableMapping =
		    CreateMapping(&odReferenceClasses, &oaRankedReferenceClasses, &odAnalysedCreatedClasses, mainClass,
				  false, mainClass->GetName(), &svAttributeName, &oaMultiTableMappings);
		assert(svAttributeName.GetSize() == 0);
		if (bTrace)
			WriteMapingArray(cout, "- main mappings " + mainClass->GetName(), &oaMultiTableMappings);

		// Parcours des classes referencees pour creer leur mapping
		// Ce mapping des classes referencees n'est pas effectue dans le cas d'une base en ecriture
		i = 0;
		while (i < oaRankedReferenceClasses.GetSize())
		{
			referenceClass = cast(KWClass*, oaRankedReferenceClasses.GetAt(i));

			// Creation du mapping sauf si classe principale
			if (referenceClass != mainClass)
			{
				// Premiere passe de creation du mapping de la table racine externe, avec des containers de travail
				// Cela permet d'analyser la structure des mappings, sans impacter directement le contenu des containers globaux
				// On part de containers vides pour analyser le dictionnaire de reference dans un espace de travail independant,
				// avant de decider s'il est utilisable et de le prendre en compte dans les mappings
				odWorkingReferenceClasses.RemoveAll();
				oaWorkingRankedReferenceClasses.RemoveAll();
				odWorkingAnalysedCreatedClasses.RemoveAll();
				oaWorkingCreatedMappings.RemoveAll();
				assert(svAttributeName.GetSize() == 0);
				mapping = CreateMapping(&odWorkingReferenceClasses, &oaWorkingRankedReferenceClasses,
							&odWorkingAnalysedCreatedClasses, referenceClass, true,
							referenceClass->GetName(), &svAttributeName,
							&oaWorkingCreatedMappings);
				assert(svAttributeName.GetSize() == 0);

				// On determine si le dictionnaire de reference est utilisable, c'est a dire s'il n'utilise
				// pas la classe principale dans ses mappings
				bIsRootDictionaryUsable = true;
				for (j = 0; j < oaWorkingCreatedMappings.GetSize(); j++)
				{
					mapping = cast(KWMTDatabaseMapping*, oaWorkingCreatedMappings.GetAt(j));

					// Transfert des specifications de la table mappee si comparaison positive
					if (mapping->GetClassName() == mainClass->GetName())
					{
						bIsRootDictionaryUsable = false;
						break;
					}
				}
				oaWorkingCreatedMappings.DeleteAll();

				// Prise en compte de la classe referencee si elle est utilisable
				if (bIsRootDictionaryUsable)
				{
					// Creation et memorisation d'un tableau de mapping, pour accueillir les mappings
					// pour la classe externe en cours de traitement
					oaCreatedMappings = new ObjectArray;
					oaAllRootCreatedMappings.Add(oaCreatedMappings);

					// Creation du mapping et memorisation de tous les mappings des sous-classes
					// Il est plus simple de rappeler la meme methode avec les container globaux
					// que de fusionner les containers de travail
					// Et il n'y a aucun enjeu d'optimisation
					assert(svAttributeName.GetSize() == 0);
					mapping = CreateMapping(&odReferenceClasses, &oaRankedReferenceClasses,
								&odAnalysedCreatedClasses, referenceClass, true,
								referenceClass->GetName(), &svAttributeName,
								oaCreatedMappings);
					assert(svAttributeName.GetSize() == 0);
					if (bTrace)
						WriteMapingArray(cout,
								 "- external mappings " + referenceClass->GetName(),
								 oaCreatedMappings);
				}
				// Sinon, memorisation d'un warning expliquant pourquoi on ne garde le dictionnaire racine en reference
				else
				{
					sWarning =
					    "Root dictionary " + referenceClass->GetName() +
					    " referenced from the main dictionary " + mainClass->GetName() +
					    " is not kept to specify database files in order to avoid circular "
					    "references in the schema, as it itself uses the main dictionary in its "
					    "secondary tables";
					svUnusedRootDictionaryWarnings.Add(sWarning);
					if (bTrace)
						cout << "- " << sWarning << "\n";
				}
			}

			// Passage a la classe suivante
			i++;
		}

		// Tri des tableau de mappings de classes references selon leur classe principal
		oaAllRootCreatedMappings.SetCompareFunction(KWMTDatabaseCompareMappingMainClass);
		oaAllRootCreatedMappings.Sort();

		// Memorisation des mapping des classes externes dans l'ordre du tri
		for (i = 0; i < oaAllRootCreatedMappings.GetSize(); i++)
		{
			// Memorisation des mappings
			oaCreatedMappings = cast(ObjectArray*, oaAllRootCreatedMappings.GetAt(i));
			assert(oaCreatedMappings->GetSize() > 0);
			oaMultiTableMappings.InsertObjectArrayAt(oaMultiTableMappings.GetSize(), oaCreatedMappings);

			// Memorisation de la classe referencee principale
			mapping = cast(KWMTDatabaseMapping*, oaCreatedMappings->GetAt(0));
			oaRootReferenceTableMappings.Add(mapping);
		}
		oaAllRootCreatedMappings.DeleteAll();

		// On recupere si possible les specifications de base a utiliser a partir des mapping precedents
		//
		// Attention, il s'agit juste d'une heuristique pour ameliorer l'ergonomie, qui a minima permet
		// de recuperer toutes les specifications de data tables existantes au cas ou on relit le
		// meme fichier de dictionnaire
		// Pour la table principale, on recupere quoi qu'il arrive la data table precedente, en mono-table ou multi-table
		// Cela permet de rester sur le meme fichier dans le cas ou on passe d'un dictionnaire (ex: Iris)
		// a sa variante de type modele de prediction (ex: SNB_Iris)
		// L'implementation marche egalement en multi-table quand on choisit un dictionnaire racine de table externe,
		// dont on recupere les mappings
		// Par contre, quand on passe du dictionnaire d'analyse a celui d'une des ses sous-tables, tous les
		// mappings changent, et on ne garde que la table principale, qui est erronees. Ce probleme n'est pas vraiment
		// un enjeu important, et il n'aurait pas de solution simple de toute facon
		for (i = 0; i < oaPreviousMultiTableMappings.GetSize(); i++)
		{
			previousMapping = cast(KWMTDatabaseMapping*, oaPreviousMultiTableMappings.GetAt(i));

			// Recherche du nouveau mapping correspondant
			// Attention, les data paths qui servent d'identifiants aux mappings peuvent etres contextuels
			// dans le cas de tables externes
			// Par exemple, le dictionnaire principal es systematique identifie par un data path vide,
			// et si on en change, comme par exemple en passant a une table externe, c'est ce
			// dictionnaire Root associe a la table externe qui sera identifie par un data path vide.
			// Il faut donc parcourir se base sur la comparaison du dictionnaire et des attribut du mapping
			// pour faire l'appariement correct
			for (j = 0; j < oaMultiTableMappings.GetSize(); j++)
			{
				mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(j));

				// Transfert des specifications de la table mappee si comparaison positive
				if (mapping->GetOriginClassName() == previousMapping->GetOriginClassName() and
				    mapping->GetDataPathAttributeNames() ==
					previousMapping->GetDataPathAttributeNames())
				{
					mapping->SetDataTableName(previousMapping->GetDataTableName());
					break;
				}
			}
		}

		// Destruction des anciens mappings
		oaPreviousMultiTableMappings.DeleteAll();
	}

	// Mise a jour de la fraicheur
	nFreshness++;

	// Affichage des mappings finaux
	if (bTrace)
		WriteMapingArray(cout, "Database " + GetDatabaseName() + " " + GetClassName() + " mappings",
				 &oaMultiTableMappings);
	ensure(mainClass == NULL or
	       mainClass->ComputeOverallNativeRelationAttributeNumber(true) == oaMultiTableMappings.GetSize() - 1 or
	       svUnusedRootDictionaryWarnings.GetSize() > 0);
	ensure(oaMultiTableMappings.GetAt(0) == mainMultiTableMapping);
}

const KWObjectReferenceResolver* KWMTDatabase::GetObjectReferenceSolver() const
{
	require(IsOpenedForRead());
	return &objectReferenceResolver;
}

KWDataTableDriver* KWMTDatabase::GetDataTableDriver()
{
	require(not IsOpenedForRead() and not IsOpenedForWrite());
	return dataTableDriverCreator;
}

boolean KWMTDatabase::Check() const
{
	return CheckPartially(false);
}

boolean KWMTDatabase::CheckPartially(boolean bWriteOnly) const
{
	boolean bOk;
	ObjectDictionary odDataTableNames;
	KWMTDatabaseMapping* mapping;
	int nMapping;
	KWClass* mainClass;
	KWClass* originClass;
	ALString sOriginLabel;
	ALString sAttributeName;
	KWAttribute* attribute;
	int nAttributeNumber;
	int nAttribute;
	KWClass* pathClass;
	KWMTDatabaseMapping parentMapping;
	KWMTDatabase checkDatabase;
	KWMTDatabaseMapping* checkMapping;

	// Test pour la base ancetre
	bOk = KWDatabase::CheckPartially(bWriteOnly);

	// Arret immediat si erreur de base, pour eviter l'analyse du mapping
	if (not bOk)
		return bOk;

	// Bufferisation de la verification uniquement a partir de la verification du mapping;
	// qui est tres couteuse
	if (bWriteOnly)
	{
		if (nCheckWriteFreshness == nFreshness)
			return bCheckWrite;
		nCheckWriteFreshness = nFreshness;
	}
	else
	{
		if (nCheckReadFreshness == nFreshness)
			return bCheckRead;
		nCheckReadFreshness = nFreshness;
	}

	// Activation du controle d'erreur
	Global::ActivateErrorFlowControl();

	// Recherche de la classe principale
	mainClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	assert(mainClass != NULL and mainClass->Check());

	// On commence par verifier que la classe est stockable sur un systeme a base de cle
	if (not mainClass->IsKeyBasedStorable())
	{
		// Affichage des diagnostics d'erreur sur les problemes lies au cles
		bOk = mainClass->CheckKeyBasedStorability();
		assert(not bOk);

		// Message synthetique
		AddError("Dictionary " + GetClassName() +
			 " cannot be used to read multi-table data from data table files");
	}

	// Verification de la validite des specifications de mapping
	if (bOk)
	{
		// Verification de structure
		assert(oaMultiTableMappings.GetSize() >= 1);
		assert(oaMultiTableMappings.GetAt(0) == mainMultiTableMapping);
		assert(mainMultiTableMapping->GetClassName() == GetClassName());

		// On part de la classe principale
		originClass = mainClass;

		// Verification de la table de mapping
		for (nMapping = 0; nMapping < oaMultiTableMappings.GetSize(); nMapping++)
		{
			mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(nMapping));

			// Verification de l'unicite des noms de tables utilises
			if (mapping->GetDataTableName() != "")
			{
				if (odDataTableNames.Lookup(mapping->GetDataTableName()) != NULL)
				{
					bOk = false;
					AddError("Data path " + mapping->GetObjectLabel() + " : Data table " +
						 mapping->GetDataTableName() + " already used");
				}
				odDataTableNames.SetAt(mapping->GetDataTableName(), mapping);
			}

			// En mode ecriture, les tables externes ne doivent pas etre renseignees
			if (bWriteOnly and mapping->GetDataTableName() != "" and IsReferencedClassMapping(mapping))
			{
				bOk = false;
				AddError("Data path " + mapping->GetObjectLabel() + " : External data table " +
					 mapping->GetDataTableName() + " should not be specified for output database");
			}

			// En mode ecriture, si une table secondaire non externe est renseignee, sa table parente doit l'etre egalement
			// En theorie, on pourrait developper du code pour autoriser ce type de specification, mais le rapport cout/benefice
			// est tres peu favorable pour cas cas d'usage marginal
			if (bWriteOnly and mapping->GetDataTableName() != "" and
			    not IsReferencedClassMapping(mapping) and mapping->GetAttributeNames()->GetSize() > 0)
			{
				// Recherche du mapping parent
				parentMapping.CopyFrom(mapping);
				parentMapping.GetAttributeNames()->SetSize(mapping->GetAttributeNames()->GetSize() - 1);
				checkMapping = LookupMultiTableMapping(parentMapping.GetDataPath());

				// Verification dans le cas valide que ce mapping a egalement un nom de table specifie
				if (checkMapping != NULL and checkMapping->GetDataTableName() == "")
				{
					bOk = false;
					AddError("Data path " + mapping->GetObjectLabel() + " : data table " +
						 mapping->GetDataTableName() +
						 " cannot be specified without a data table being specified for its "
						 "owner table (data path " +
						 parentMapping.GetDataPath() + ")");
				}
			}

			// Recherche de la classe principale du chemin de mapping
			originClass = KWClassDomain::GetCurrentDomain()->LookupClass(mapping->GetOriginClassName());
			assert(originClass == NULL or originClass->GetName() == GetClassName() or
			       originClass->GetRoot());

			// Existence de cette classe
			if (originClass == NULL)
			{
				bOk = false;
				if (mapping->GetOriginClassName() == GetClassName())
					sOriginLabel = "Main";
				else
					sOriginLabel = "Root";
				AddError("Data path " + mapping->GetObjectLabel() + " : " + sOriginLabel +
					 " dictionary " + mapping->GetOriginClassName() + " does not exist");
			}

			// Validite du chemin de donnee
			if (bOk)
			{
				// Parcours des attributs du chemin de donnees du mapping
				nAttributeNumber = mapping->GetDataPathAttributeNumber();
				pathClass = originClass;
				for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
				{
					check(pathClass);

					// Recherche de l'attribut correspondant au mapping
					sAttributeName = mapping->GetDataPathAttributeNameAt(nAttribute);
					attribute = pathClass->LookupAttribute(sAttributeName);

					// Existence de l'attribut mappe
					if (attribute == NULL)
					{
						bOk = false;
						AddError("Data path " + mapping->GetObjectLabel() + " : Variable " +
							 sAttributeName + " not found in dictionary " +
							 pathClass->GetName());
					}
					// Son type doit etre Object ou ObjectArray
					else if (not KWType::IsRelation(attribute->GetType()))
					{
						bOk = false;
						AddError("Data path " + mapping->GetObjectLabel() +
							 " : Type of variable " + sAttributeName + " should be " +
							 KWType::ToString(KWType::Object) + " or " +
							 KWType::ToString(KWType::ObjectArray));
					}
					// Sa classe doit etre coherente avec celle du mapping,
					// pour le dernier attribut du chemin
					else if (mapping->GetClassName() != attribute->GetClass()->GetName())
					{
						if (nAttribute == nAttributeNumber - 1)
						{
							bOk = false;
							AddError("Data path " + mapping->GetObjectLabel() +
								 " : Dictionary of variable " + sAttributeName + " (" +
								 mapping->GetClassName() + ") should be " +
								 attribute->GetClass()->GetName());
						}
					}
					// La classe utilisante doit avoir une cle
					else if (pathClass->GetKeyAttributeNumber() == 0)
					{
						bOk = false;
						AddError("Dictionary (" + pathClass->GetName() +
							 ") should have a key to use variable " + sAttributeName);
					}
					// Sa classe doit avoir une cle
					else if (attribute->GetClass()->GetKeyAttributeNumber() == 0)
					{
						bOk = false;
						AddError("Data path " + mapping->GetObjectLabel() +
							 " : Dictionary of variable " + sAttributeName + " (" +
							 mapping->GetClassName() + ") should have a key");
					}
					// La cle de la classe utilisee doit etre au moins aussi longue que
					// celle de la classe utilisante dans le cas d'un lien de composition
					else if (not attribute->GetReference() and
						 attribute->GetDerivationRule() == NULL and
						 attribute->GetClass()->GetKeyAttributeNumber() <
						     pathClass->GetKeyAttributeNumber())
					{
						bOk = false;
						AddError("Data path " + mapping->GetObjectLabel() +
							 " : In dictionary " + mapping->GetClassName() +
							 " of variable " + sAttributeName + ", the key length (" +
							 IntToString(attribute->GetClass()->GetKeyAttributeNumber()) +
							 ") must not be less than that of its parent "
							 "dictionary " +
							 pathClass->GetName() + "(" +
							 IntToString(pathClass->GetKeyAttributeNumber()) + ")");
					}

					// Passage a la classe suivante dans le path
					if (bOk)
						pathClass = attribute->GetClass();

					// Arret si erreur
					if (not bOk)
						break;
				}
			}
		}
	}

	// Erreur si mapping en trop, warning en cas d'absence de specifications de mapping,
	if (bOk)
	{
		// Parametrage d'une copie de la base et creation de sa structure de mapping
		checkDatabase.SetClassName(GetClassName());
		checkDatabase.UpdateMultiTableMappings();

		// Verification qu'il n'y a pas de mapping en trop
		for (nMapping = 0; nMapping < oaMultiTableMappings.GetSize(); nMapping++)
		{
			mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(nMapping));

			// Recherche du mapping correspondant, puis verification
			checkMapping = checkDatabase.LookupMultiTableMapping(mapping->GetDataPath());
			if (checkMapping == NULL)
			{
				AddError("No data table file specified for data path " + mapping->GetObjectLabel());
				bOk = false;
				break;
			}
		}

		// Parcours des mappings pour verifier le parametrage des tables associees a chaque mapping
		// Uniquement en lecture: en ecriture, tout ou partie des mappings peut ne pas etre renseigne
		if (bOk and not bWriteOnly)
		{
			for (nMapping = 0; nMapping < checkDatabase.GetMultiTableMappings()->GetSize(); nMapping++)
			{
				checkMapping =
				    cast(KWMTDatabaseMapping*, checkDatabase.GetMultiTableMappings()->GetAt(nMapping));

				// Recherche du mapping correspondant, puis verification
				mapping = LookupMultiTableMapping(checkMapping->GetDataPath());
				if (mapping == NULL)
				{
					AddError("Data path " + checkMapping->GetObjectLabel() +
						 " : No data path specification");
					bOk = false;
				}
				else if (mapping->GetDataTableName() == "")
				{
					AddError("Data path " + checkMapping->GetObjectLabel() +
						 " : No data table file specified");
					bOk = false;
				}
			}
		}
	}

	// Desactivation du controle d'erreur
	Global::DesactivateErrorFlowControl();

	// Memorisation de la verification
	if (bWriteOnly)
	{
		assert(nCheckWriteFreshness == nFreshness);
		bCheckWrite = bOk;
	}
	else
	{
		assert(nCheckReadFreshness == nFreshness);
		bCheckRead = bOk;
	}
	return bOk;
}

boolean KWMTDatabase::CheckFormat() const
{
	boolean bOk;
	KWDataTableDriver* dataTableDriverCheck;

	// Test du format du driver de table, en passant par une variable temporaire dont on peut parametrer le nom
	dataTableDriverCheck = dataTableDriverCreator->Clone();
	dataTableDriverCheck->SetDataTableName(GetDatabaseName());
	bOk = dataTableDriverCheck->CheckFormat();
	delete dataTableDriverCheck;

	// Test pour la base ancetre
	bOk = bOk and KWDatabase::CheckFormat();
	return bOk;
}

longint KWMTDatabase::GetExternalTablesEncodingErrorNumber() const
{
	require(not IsOpenedForWrite());
	require(GetClassName() != "");
	return lExternalTablesEncodingErrorNumber;
}

void KWMTDatabase::SetExternalTablesEncodingErrorNumber(longint lValue) const
{
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());
	require(GetClassName() != "");
	require(lValue >= 0);

	lExternalTablesEncodingErrorNumber = lValue;
}

void KWMTDatabase::DisplayMultiTableMappingWarnings() const
{
	int n;

	require(Check());

	// Emission des eventuels warnings en cas de table externe non utilisable
	Global::ActivateErrorFlowControl();
	for (n = 0; n < svUnusedRootDictionaryWarnings.GetSize(); n++)
		AddWarning(svUnusedRootDictionaryWarnings.GetAt(n));
	Global::DesactivateErrorFlowControl();
}

void KWMTDatabase::SetVerboseMode(boolean bValue)
{
	KWMTDatabaseMapping* mapping;
	int i;

	// Appel de la methode ancetre
	KWDatabase::SetVerboseMode(bValue);

	// Parametrage du driver
	dataTableDriverCreator->SetVerboseMode(bValue);

	// Propagation du mode verbeux a toutes les tables principales et secondaires
	for (i = 0; i < oaMultiTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));
		if (mapping->GetDataTableDriver() != NULL)
			mapping->GetDataTableDriver()->SetVerboseMode(bValue);
	}
}

void KWMTDatabase::SetSilentMode(boolean bValue)
{
	KWMTDatabaseMapping* mapping;
	int i;

	// Appel de la methode ancetre
	KWDatabase::SetSilentMode(bValue);

	// Parametrage du driver
	dataTableDriverCreator->SetSilentMode(bValue);

	// Propagation du mode verbeux a toutes les tables principales et secondaires
	for (i = 0; i < oaMultiTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));
		if (mapping->GetDataTableDriver() != NULL)
			mapping->GetDataTableDriver()->SetSilentMode(bValue);
	}
}

longint KWMTDatabase::GetUsedMemory() const
{
	boolean bShowMemory = false;
	longint lUsedMemory;
	KWMTDatabaseMapping* mapping;
	int i;

	// Methode ancetre
	lUsedMemory = KWDatabase::GetUsedMemory();

	// Specialisation
	lUsedMemory += sizeof(KWDataTableDriver*);
	if (dataTableDriverCreator != NULL)
		lUsedMemory += dataTableDriverCreator->GetUsedMemory();
	lUsedMemory += oaRootReferenceTableMappings.GetUsedMemory();
	lUsedMemory += oaMultiTableMappings.GetUsedMemory();
	lUsedMemory += svUnusedRootDictionaryWarnings.GetUsedMemory();

	// Memoire utilise par les mappings
	for (i = 0; i < oaMultiTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));
		lUsedMemory += mapping->GetUsedMemory();
	}

	// Memoire utilisee par les objets references
	lUsedMemory += objectReferenceResolver.GetUsedMemory();

	// Affichage des stats memoire
	if (bShowMemory)
	{
		cout << "KWMTDatabase::GetUsedMemory, base memory: " << KWDatabase::GetUsedMemory() << endl;
		cout << "KWMTDatabase::GetUsedMemory, reference memory: " << objectReferenceResolver.GetUsedMemory()
		     << endl;
		cout << "KWMTDatabase::GetUsedMemory, total memory: " << lUsedMemory << endl;
	}
	return lUsedMemory;
}

longint KWMTDatabase::ComputeOpenNecessaryMemory(boolean bRead, boolean bIncludingClassMemory)
{
	longint lNecessaryMemory;
	int i;
	KWMTDatabaseMapping* mapping;
	KWDataTableDriver* driver;

	require(not IsOpenedForRead() and not IsOpenedForWrite());
	require(kwcClass != NULL);

	// Appel de la methode ancetre
	lNecessaryMemory = KWDatabase::ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);

	// Nettoyage prealable
	DMTMPhysicalTerminateMapping(mainMultiTableMapping);

	// Initialisation recursive du mapping a partir de la table principale pour avoir des driver initialises
	if (bRead)
	{
		// En lecture, on utilise la classe physique
		check(kwcPhysicalClass);
		DMTMPhysicalInitializeMapping(mainMultiTableMapping, kwcPhysicalClass, true);
	}
	else
	{
		// En ecriture, on utile la classe logique
		check(kwcClass);
		DMTMPhysicalInitializeMapping(mainMultiTableMapping, kwcClass, false);
	}

	// On complete par la taille demandee par le driver pour chaque table a ouvrir
	for (i = 0; i < GetMultiTableMappings()->GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));
		driver = mapping->GetDataTableDriver();

		// Memorisation du mapping si driver utilise
		if (driver != NULL)
			lNecessaryMemory += driver->ComputeOpenNecessaryMemory(bRead);
	}

	// Nettoyage prealable
	DMTMPhysicalTerminateMapping(mainMultiTableMapping);

	// En lecture, ajout de la place necessaire pour le chargement des tables externes
	if (bRead)
		lNecessaryMemory += ComputeSamplingBasedNecessaryMemoryForReferenceObjects();
	ensure(lNecessaryMemory >= 0);
	return lNecessaryMemory;
}

boolean KWMTDatabase::CheckObjectConsistency() const
{
	boolean bOk = true;
	KWClass* mainClass;
	ObjectArray oaClasses;
	NumericKeyDictionary nkdClasses;
	int nClass;
	KWClass* secondaryClass;
	KWAttribute* attribute;
	KWDerivationRule* rule;
	KWDerivationRule* lastCheckedRule;
	NumericKeyDictionary nkdAllIndirectlydAttributes;

	require(KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName())->IsCompiled());

	// Acces a la classe principale
	mainClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

	// Traitement des classes depuis la classe principale, on les memorisant dans un dictionnaire pour ne les
	// traiter qu'une seule fois
	oaClasses.Add(mainClass);
	nkdClasses.SetAt(mainClass, mainClass);
	for (nClass = 0; nClass < oaClasses.GetSize(); nClass++)
	{
		secondaryClass = cast(KWClass*, oaClasses.GetAt(nClass));

		// Parcours des attributs de la classes pour identifier les attributs utilises calcules de type Object
		lastCheckedRule = NULL;
		attribute = secondaryClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Traitement si attribut charge et de type Object
			if (attribute->GetLoaded() and KWType::IsRelation(attribute->GetType()))
			{
				check(attribute->GetClass());

				// Propagation aux classes des attribut natifs
				rule = attribute->GetAnyDerivationRule();
				if (rule == NULL)
				{
					// Ajout de la classe a verifier
					if (nkdClasses.Lookup(attribute->GetClass()) == NULL)
					{
						oaClasses.Add(attribute->GetClass());
						nkdClasses.SetAt(attribute->GetClass(), attribute->GetClass());
					}
				}
				// Verification des operandes de la regle sinon
				else
				{
					// On evite de verifier plusieurs fois la meme  regle, ce qui peut arriver dans
					// le cas d'attribut d'un bloc
					if (rule != lastCheckedRule)
					{
						lastCheckedRule = rule;
						bOk = CheckObjectRuleConsistency(rule);
						if (not bOk)
						{
							AddError("Native entities used as operand in rule " +
								 rule->GetName() + " to build variable " +
								 attribute->GetName() + " in dictionary " +
								 secondaryClass->GetName() +
								 " should be loaded in memory");
							break;
						}
					}
				}
			}

			// Attribut suivant
			secondaryClass->GetNextAttribute(attribute);
		}
	}
	return bOk;
}

boolean KWMTDatabase::CheckObjectRuleConsistency(KWDerivationRule* rule) const
{
	boolean bOk = true;
	int nOperand;
	KWDerivationRuleOperand* operand;
	KWAttribute* attributeOperand;
	KWAttributeBlock* attributeBlockOperand;

	require(rule != NULL);
	require(KWType::IsGeneralRelation(rule->GetType()));

	// Analyse des operandes de la regles
	for (nOperand = 0; nOperand < rule->GetOperandNumber(); nOperand++)
	{
		operand = rule->GetOperandAt(nOperand);

		// Seuls les operandes de type Object sont concernes
		if (KWType::IsGeneralRelation(operand->GetType()))
		{
			// Les operandes natifs doivent etre charge en memoire
			if (operand->GetOrigin() == KWDerivationRuleOperand::OriginAttribute)
			{
				// Cas d'un attribut
				if (not KWType::IsValueBlock(operand->GetType()))
				{
					attributeOperand = operand->GetOriginAttribute();

					// Si attribut natif, il doit etre charge
					if (attributeOperand->GetDerivationRule() == NULL)
					{
						if (not attributeOperand->GetLoaded() and
						    not attributeOperand->GetInternalLoadIndex().IsValid())
						{
							AddError("Variable " + attributeOperand->GetName() +
								 " of dictionary " +
								 attributeOperand->GetParentClass()->GetName() +
								 " used as operand " + IntToString(nOperand + 1) +
								 " in rule " + rule->GetName() +
								 " should be loaded in memory");
							bOk = false;
						}
					}
					// Sinon, propagation a la regle utilisee
					else
						bOk = CheckObjectRuleConsistency(attributeOperand->GetDerivationRule());
				}
				// Cas d'un bloc d'attributs
				else
				{
					attributeBlockOperand = operand->GetOriginAttributeBlock();
					assert(attributeBlockOperand->GetDerivationRule() != NULL);

					// Propagation a la regle utilisee
					bOk = CheckObjectRuleConsistency(attributeBlockOperand->GetDerivationRule());
				}
			}
			// Propagation dans le cas de regle
			else if (operand->GetOrigin() == KWDerivationRuleOperand::OriginRule)
				bOk = CheckObjectRuleConsistency(operand->GetDerivationRule());
		}

		// Arret si erreur
		if (not bOk)
			break;
	}
	return bOk;
}

KWDataTableDriver* KWMTDatabase::CreateDataTableDriver(KWMTDatabaseMapping* mapping) const
{
	require(mapping != NULL);
	return dataTableDriverCreator->Clone();
}

longint KWMTDatabase::GetEncodingErrorNumber() const
{
	// Collecte des erreur d'encodage uniquement si la base est ouverte en lecture
	// Sinon, on renvoie les erreurs memorisees au moment de la fermeture
	if (IsOpenedForRead())
	{
		// Collecte des erreurs d'encodage a artir de la table principale
		lEncodingErrorNumber = DMTMPhysicalComputeEncodingErrorNumber(mainMultiTableMapping);

		// Ajout des erreur d'encodage liees au tables externes
		lEncodingErrorNumber += lExternalTablesEncodingErrorNumber;
	}
	return lEncodingErrorNumber;
}

boolean KWMTDatabase::BuildDatabaseClass(KWClass* kwcDatabaseClass)
{
	boolean bOk;

	dataTableDriverCreator->SetDataTableName(GetDatabaseName());
	bOk = dataTableDriverCreator->BuildDataTableClass(kwcDatabaseClass);
	dataTableDriverCreator->SetDataTableName("");
	return bOk;
}

boolean KWMTDatabase::IsTypeInitializationManaged() const
{
	return dataTableDriverCreator->IsTypeInitializationManaged();
}

boolean KWMTDatabase::PhysicalOpenForRead()
{
	boolean bOk = true;

	require(Check());
	require(CheckObjectConsistency());

	// Nettoyage prealable
	DMTMPhysicalTerminateMapping(mainMultiTableMapping);
	nSkippedRecordNumber = 0;

	// Ouverture si Ok
	if (bOk)
	{
		// Initialisation recursive du mapping a partir de la table principale
		DMTMPhysicalInitializeMapping(mainMultiTableMapping, kwcPhysicalClass, true);

		// Ouverture recursive des tables a partir de la table principale
		if (bOk)
			bOk = DMTMPhysicalOpenForRead(mainMultiTableMapping, kwcClass);

		// Ouverture des tables referencees
		if (bOk)
			bOk = PhysicalReadAllReferenceObjects(1);
	}
	return bOk;
}

boolean KWMTDatabase::PhysicalOpenForWrite()
{
	boolean bOk;

	require(CheckPartially(true));

	// Parametrage du format de sortie dense dans le driver
	dataTableDriverCreator->SetDenseOutputFormat(GetDenseOutputFormat());

	// Nettoyage prealable
	DMTMPhysicalTerminateMapping(mainMultiTableMapping);
	nSkippedRecordNumber = 0;

	// Initialisation recursive du mapping a partir de la table principale
	DMTMPhysicalInitializeMapping(mainMultiTableMapping, kwcClass, false);

	// Ouverture recursive des tables a partir de la table principale
	bOk = DMTMPhysicalOpenForWrite(mainMultiTableMapping);
	return bOk;
}

boolean KWMTDatabase::IsPhysicalEnd() const
{
	// Test de fin de la table principale
	return mainMultiTableMapping->GetDataTableDriver()->IsEnd();
}

KWObject* KWMTDatabase::PhysicalRead()
{
	KWObject* kwoObject;

	// Lecture d'un enregistrement de la table principale
	kwoObject = DMTMPhysicalRead(mainMultiTableMapping);

	// Prise en compte dans le memory guard
	if (kwoObject != NULL)
	{
		// Nettoyage des objets natifs inclus si le memory guard a detecte un depassement de limite memoire
		if (memoryGuard.IsSingleInstanceMemoryLimitReached())
			kwoObject->CleanNativeRelationAttributes();
	}

	// Lecture apres la fin de la base pour effectuer des controles
	if (mainMultiTableMapping->GetDataTableDriver()->IsEnd())
		PhysicalReadAfterEndOfDatabase();
	return kwoObject;
}

void KWMTDatabase::PhysicalReadAfterEndOfDatabase()
{
	KWMTDatabaseMapping* componentMapping;
	int i;
	KWDataTableDriver* mappedDataTableDriver;
	KWObject* kwoLastSubObject;
	KWObject* kwoSubObject;
	KWObjectKey lastSubObjectKey;
	KWObjectKey subObjectKey;
	ALString sTmp;
	ALString sObjectLabel;
	ALString sOtherObjectLabel;

	require(IsPhysicalEnd());
	require(IsOpenedForRead());

	// Positionnement du flag d'erreur
	bIsError = bIsError or mainMultiTableMapping->GetDataTableDriver()->IsError();

	// Lecture de chaque sous-base jusqu'a la fin pour detecter les erreurs
	for (i = 1; i < oaMultiTableMappings.GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));

		// Lecture dans la sous base
		mappedDataTableDriver = componentMapping->GetDataTableDriver();
		if (mappedDataTableDriver != NULL and mappedDataTableDriver->GetDataTableName() != "" and
		    mappedDataTableDriver->IsOpenedForRead())
		{
			// On recupere le dernier objet non traite
			kwoLastSubObject = componentMapping->GetLastReadObject();
			componentMapping->SetLastReadObject(NULL);
			componentMapping->CleanLastReadKey();

			// On signale un objet orphelin pour l'eventuele dernier objet non traite
			// La table principale etant fini, tous les enregistrements secondaires sont orphelins
			// Warning uniquement si aucun enregistrement principal saute
			// En effet, si l'on a "saute" (Skip) des enregistrements du fichier principal,
			// il est licite de trouver des enregistrement secondaires non rattachable a l'objet principal
			if (kwoLastSubObject != NULL)
			{
				if (not bIsError and nSkippedRecordNumber == 0)
				{
					lastSubObjectKey.InitializeFromObject(kwoLastSubObject);
					mappedDataTableDriver->AddWarning(
					    sTmp + "Ignored record" + ", orphan record " +
					    componentMapping->GetClassName() + lastSubObjectKey.GetObjectLabel() +
					    " found beyond the key of the last record of the main table");
				}
			}

			// Lecture des sous-objets
			kwoSubObject = NULL;
			while (not mappedDataTableDriver->IsEnd() and not bIsError)
			{
				// Test d'interruption utilisateur
				if (TaskProgression::IsInterruptionRequested())
					break;

				// Lecture d'un enregistrement de la table principale
				// On ne prend pas la lecture en compte dans le memory guard, car les objet lus sont
				// immediatement detruits
				kwoSubObject = componentMapping->GetDataTableDriver()->Read();

				//DDD
				// Parametrage du data path de l'objet
				if (kwoSubObject != NULL)
				{
					assert(kwoSubObject->GetClass() != kwcPhysicalClass);
					kwoSubObject->SetDataPath(
					    objectDataPathManager->LookupDataPath(componentMapping->GetDataPath()));
				}

				// Positionnement du flag d'erreur
				bIsError = bIsError or componentMapping->GetDataTableDriver()->IsError();

				// On continue si l'objet ramene est NULL
				if (kwoSubObject == NULL)
					continue;

				// Test de coherence en cas de deux objets successifs disponibles
				if (kwoLastSubObject != NULL)
				{
					assert(kwoSubObject != NULL);

					// Recherche de la cle des objets a comparer
					lastSubObjectKey.InitializeFromObject(kwoLastSubObject);
					subObjectKey.InitializeFromObject(kwoSubObject);

					// Erreur si probleme d'ordonnancement
					if (subObjectKey.StrictCompare(&lastSubObjectKey) < 0)
					{
						// Creation de libelles distincts
						subObjectKey.BuildDistinctObjectLabels(&lastSubObjectKey, sObjectLabel,
										       sOtherObjectLabel);

						mappedDataTableDriver->AddError(
						    sTmp + "Unsorted record " + componentMapping->GetClassName() +
						    sObjectLabel +
						    ", with key inferior to that of the preceding record " +
						    componentMapping->GetClassName() + sOtherObjectLabel +
						    ", beyond the key of the last record of the main table");
						bIsError = true;
					}
				}

				// On signale un objet orphelin
				if (not bIsError and nSkippedRecordNumber == 0)
				{
					subObjectKey.InitializeFromObject(kwoSubObject);
					mappedDataTableDriver->AddWarning(
					    sTmp + "Ignored record" + ", orphan record " +
					    componentMapping->GetClassName() + subObjectKey.GetObjectLabel() +
					    " found beyond the key of the last record of the main table");
				}

				// Memorisation du dernier sous-objet lu
				if (kwoLastSubObject != NULL)
					delete kwoLastSubObject;
				kwoLastSubObject = kwoSubObject;
			}

			// Destruction du dernier sous-objet lu
			if (kwoLastSubObject != NULL)
				delete kwoLastSubObject;
		}

		// Arret si erreur
		if (bIsError)
			break;
	}
	ensure(IsPhysicalEnd() or bIsError);
}

void KWMTDatabase::PhysicalSkip()
{
	// Saut d'un enregistrement sur la table principale
	mainMultiTableMapping->GetDataTableDriver()->Skip();
	bIsError = bIsError or mainMultiTableMapping->GetDataTableDriver()->IsError();

	// Memorisation inconditionnelle de la cle du dernier enregistremnt lu, dans le cas d'une classe principale,
	// meme si l'objet n'a pas pu etre lu
	// Cela permet de gere les lignes dupliquees, que l'objet soit lu ou non (a cause d'une erreur de parsing)
	assert(mainMultiTableMapping->GetDataTableDriver()->GetClass()->GetName() == kwcClass->GetName());
	assert(mainMultiTableMapping->GetDataTableDriver()->GetClass()->GetRoot() == kwcClass->GetRoot());
	if (kwcClass->IsUnique())
	{
		assert(mainMultiTableMapping->GetDataTableDriver()->GetLastReadMainKey()->GetSize() ==
		       mainMultiTableMapping->GetDataTableDriver()->GetClass()->GetKeyAttributeNumber());
		mainMultiTableMapping->SetLastReadKey(
		    mainMultiTableMapping->GetDataTableDriver()->GetLastReadMainKey());
	}

	// Attention, il n'est pas possible de propager les skip sur les sous-tables
	// Consequence: pas de warning quand un objet d'une sous-table a une cle superieure a celle de l'objet
	//  principal courant
	// Memorisation du nombre d'enregistrement sautes pour gerer ce cas
	nSkippedRecordNumber++;

	// Lecture apres la fin de la base pour effectuer des controles
	if (not bIsError and mainMultiTableMapping->GetDataTableDriver()->IsEnd())
		PhysicalReadAfterEndOfDatabase();
}

void KWMTDatabase::PhysicalWrite(const KWObject* kwoObject)
{
	// Ecriture d'un enregistrement de la table principale
	DMTMPhysicalWrite(mainMultiTableMapping, kwoObject);
}

boolean KWMTDatabase::PhysicalClose()
{
	boolean bOk;

	// Fermeture de la base et de toutes ses sous-bases
	bOk = DMTMPhysicalClose(mainMultiTableMapping);
	nSkippedRecordNumber = 0;

	// Destruction des objets references
	PhysicalDeleteAllReferenceObjects();
	return bOk;
}

void KWMTDatabase::PhysicalDeleteDatabase()
{
	// Destruction des tables de la hierarchie principale, hors classe referencees
	DMTMPhysicalDeleteDatabase(mainMultiTableMapping);
}

longint KWMTDatabase::GetPhysicalEstimatedObjectNumber()
{
	longint lPhysicalEstimatedObjectNumber;

	require(mainMultiTableMapping->GetDataTableDriver() == NULL);

	// Parametrage du mapping principal
	mainMultiTableMapping->SetDataTableDriver(CreateDataTableDriver(mainMultiTableMapping));
	mainMultiTableMapping->GetDataTableDriver()->SetDataTableName(mainMultiTableMapping->GetDataTableName());
	mainMultiTableMapping->GetDataTableDriver()->SetClass(
	    KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()));

	// Appel de la methode du driver
	lPhysicalEstimatedObjectNumber = mainMultiTableMapping->GetDataTableDriver()->GetEstimatedObjectNumber();

	// Nettoyage (attention a nettoyer le driver avant de le detruire)
	mainMultiTableMapping->GetDataTableDriver()->SetDataTableName("");
	mainMultiTableMapping->GetDataTableDriver()->SetClass(NULL);
	delete mainMultiTableMapping->GetDataTableDriver();
	mainMultiTableMapping->SetDataTableDriver(NULL);
	return lPhysicalEstimatedObjectNumber;
}

double KWMTDatabase::GetPhysicalReadPercentage() const
{
	require(mainMultiTableMapping->GetDataTableDriver() != NULL);
	return mainMultiTableMapping->GetDataTableDriver()->GetReadPercentage();
}

longint KWMTDatabase::GetPhysicalRecordIndex() const
{
	if (mainMultiTableMapping->GetDataTableDriver() != NULL)
		return mainMultiTableMapping->GetDataTableDriver()->GetRecordIndex();
	else
		return 0;
}

void KWMTDatabase::CollectPhysicalStatsMessages(ObjectArray* oaPhysicalMessages)
{
	KWMTDatabaseMapping* mapping;
	KWDataTableDriver* mappedDataTableDriver;
	int i;
	Error* errorMessage;
	ALString sTmp;

	require(oaPhysicalMessages != NULL);
	require(oaPhysicalMessages->GetSize() == 0);

	// Propagation des messages aux databases secondaires
	for (i = 1; i < oaMultiTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));

		// Nombre d'enregistrements lus par mapping
		mappedDataTableDriver = mapping->GetDataTableDriver();
		if (mappedDataTableDriver != NULL and mappedDataTableDriver->GetDataTableName() != "" and
		    not mappedDataTableDriver->GetSilentMode() and mappedDataTableDriver->GetVerboseMode())
		{
			errorMessage = new Error;

			// On ne passe par mappedDataTableDriver->GetObjectLabel() pour eviter d'avoir un numero de
			// record
			errorMessage->Initialize(
			    Error::GravityMessage, "", "",
			    sTmp + "  Table " + mappedDataTableDriver->GetDataTableName() + " " +
				"Records: " + LongintToReadableString(mappedDataTableDriver->GetUsedRecordNumber()));
			oaPhysicalMessages->Add(errorMessage);
		}
	}
}

void KWMTDatabase::DeletePhysicalClass()
{
	KWMTDatabaseMapping* mapping;
	int i;

	// Appel de la methode ancetre
	KWDatabase::DeletePhysicalClass();

	// Dereferencement des classes physique dans tous les tables principales et secondaires
	for (i = 0; i < oaMultiTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));

		// Dereferencement des classes dans les drivers de tables mappes
		if (mapping->GetDataTableDriver() != NULL)
			mapping->GetDataTableDriver()->SetClass(NULL);
	}
}

void KWMTDatabase::MutatePhysicalObject(KWObject* kwoPhysicalObject) const
{
	require(kwcPhysicalClass != NULL);
	require(kwcPhysicalClass->GetName() == GetClassName());
	require(kwcPhysicalClass->IsCompiled());
	require(kwoPhysicalObject != NULL);
	require(kwoPhysicalObject->GetClass() == kwcPhysicalClass);
	require(kwcPhysicalClass->GetLoadedAttributeNumber() >= kwcClass->GetLoadedAttributeNumber());

	// Parametrage du resolveur de reference dans la regle de derivation gerant les references
	assert(KWDRReference::GetObjectReferenceResolver() == NULL);
	KWDRReference::SetObjectReferenceResolver(&objectReferenceResolver);

	// Referencement du nouvel objet principal lu depuis le resolveur de reference, s'il est Root
	if (kwcPhysicalClass->GetRoot() and not mainMultiTableMapping->GetLastReadKey()->IsEmpty())
		objectReferenceResolver.AddObject(kwcPhysicalClass, mainMultiTableMapping->GetLastReadKey(),
						  kwoPhysicalObject);

	// Appel de la methode ancetre
	KWDatabase::MutatePhysicalObject(kwoPhysicalObject);

	// Dereferencement du precedent objet principal lu depuis le resolveur de reference
	// En effet, l'objet precedement lu est potentiellement detruit et inutilisable pour
	// la resolution des references (intra-objet dans le cas de l'objet principal)
	if (kwcPhysicalClass->GetRoot() and not mainMultiTableMapping->GetLastReadKey()->IsEmpty())
		objectReferenceResolver.RemoveObject(kwcPhysicalClass, mainMultiTableMapping->GetLastReadKey());

	// Remise a NULL du resolveur de reference dans la regle de derivation gerant les references
	KWDRReference::SetObjectReferenceResolver(NULL);

	ensure(kwoPhysicalObject->GetClass() == kwcClass);
	ensure(kwoPhysicalObject->Check());
}

boolean KWMTDatabase::IsPhysicalObjectSelected(KWObject* kwoPhysicalObject)
{
	boolean bSelected;

	require(kwcPhysicalClass != NULL);
	require(kwcPhysicalClass->GetName() == GetClassName());
	require(kwcPhysicalClass->IsCompiled());
	require(kwoPhysicalObject != NULL);
	require(kwoPhysicalObject->GetClass() == kwcPhysicalClass);

	// Calcul du critere de selection si necessaire
	if (not liSelectionAttributeLoadIndex.IsValid())
		bSelected = true;
	else
	{
		// Parametrage du resolveur de reference dans la regle de derivation gerant les references
		assert(KWDRReference::GetObjectReferenceResolver() == NULL);
		KWDRReference::SetObjectReferenceResolver(&objectReferenceResolver);

		// Referencement du nouvel objet principal lu depuis le resolveur de reference
		if (kwcPhysicalClass->GetRoot() and not mainMultiTableMapping->GetLastReadKey()->IsEmpty())
			objectReferenceResolver.AddObject(kwcPhysicalClass, mainMultiTableMapping->GetLastReadKey(),
							  kwoPhysicalObject);

		// Calcul du critere de selection
		if (nSelectionAttributeType == KWType::Symbol)
			bSelected =
			    kwoPhysicalObject->ComputeSymbolValueAt(liSelectionAttributeLoadIndex) == sSelectionSymbol;
		else if (nSelectionAttributeType == KWType::Continuous)
			bSelected = kwoPhysicalObject->ComputeContinuousValueAt(liSelectionAttributeLoadIndex) ==
				    cSelectionContinuous;
		else
			bSelected = false;

		// Dereferencement du precedent objet principal lu depuis le resolveur de reference
		// En effet, l'objet precedement lu est potentiellement detruit et inutilisable pour
		// la resolution des references (intra-objet dans le cas de l'objet principal)
		if (kwcPhysicalClass->GetRoot() and not mainMultiTableMapping->GetLastReadKey()->IsEmpty())
			objectReferenceResolver.RemoveObject(kwcPhysicalClass, mainMultiTableMapping->GetLastReadKey());

		// Remise a NULL du resolveur de reference dans la regle de derivation gerant les references
		KWDRReference::SetObjectReferenceResolver(NULL);
	}
	return bSelected;
}

KWMTDatabaseMapping* KWMTDatabase::CreateMapping(ObjectDictionary* odReferenceClasses,
						 ObjectArray* oaRankedReferenceClasses,
						 ObjectDictionary* odAnalysedCreatedClasses, const KWClass* mappedClass,
						 boolean bIsExternalTable, const ALString& sOriginClassName,
						 StringVector* svAttributeNames, ObjectArray* oaCreatedMappings)
{
	KWMTDatabaseMapping* mapping;
	KWMTDatabaseMapping* subMapping;
	KWAttribute* attribute;
	KWClass* kwcTargetClass;
	ObjectArray oaUsedClass;
	KWClass* kwcUsedClass;
	int nUsedClass;

	require(odReferenceClasses != NULL);
	require(oaRankedReferenceClasses != NULL);
	require(odAnalysedCreatedClasses != NULL);
	require(odReferenceClasses->GetCount() == oaRankedReferenceClasses->GetSize());
	require(mappedClass != NULL);
	require(sOriginClassName != "");
	require(not mappedClass->GetRoot() or mappedClass->GetName() == sOriginClassName);
	require(not mappedClass->GetRoot() or svAttributeNames->GetSize() == 0);
	require(oaCreatedMappings != NULL);

	// Creation et initialisation d'un mapping
	mapping = new KWMTDatabaseMapping;
	mapping->SetExternalTable(bIsExternalTable);
	mapping->SetClassName(mappedClass->GetName());
	mapping->SetOriginClassName(sOriginClassName);
	mapping->GetAttributeNames()->CopyFrom(svAttributeNames);
	assert(LookupMultiTableMapping(mapping->GetDataPath()) == NULL);

	// Memorisation de ce mapping dans le tableau exhaustif de tous les mapping
	oaCreatedMappings->Add(mapping);

	// Ajout des mapping pour la composition de la classe ainsi que les classe referencees
	attribute = mappedClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Test si attribut natif est de type Object ou ObjectArray
		if (KWType::IsRelation(attribute->GetType()) and attribute->GetClass() != NULL)
		{
			// Cas d'un attribut natif de la composition (sans regle de derivation)
			if (attribute->GetAnyDerivationRule() == NULL)
			{
				// Ajout temporaire d'un attribut au mapping
				svAttributeNames->Add(attribute->GetName());

				// Creation du mapping dans une nouvelle table de mapping temporaire
				subMapping =
				    CreateMapping(odReferenceClasses, oaRankedReferenceClasses,
						  odAnalysedCreatedClasses, attribute->GetClass(), bIsExternalTable,
						  sOriginClassName, svAttributeNames, oaCreatedMappings);

				// Supression de l'attribut ajoute temporairement
				svAttributeNames->SetSize(svAttributeNames->GetSize() - 1);

				// Chainage du sous-mapping
				mapping->GetComponentTableMappings()->Add(subMapping);
			}
			// Cas d'un attribut issue d'une regle de creation de table, pour rechercher
			// les classes referencees depuis les tables creees par des regles
			else if (not attribute->GetReference())
			{
				assert(attribute->GetAnyDerivationRule() != NULL);

				// Recherche de la classe cible
				kwcTargetClass = mappedClass->GetDomain()->LookupClass(
				    attribute->GetDerivationRule()->GetObjectClassName());
				assert(kwcTargetClass != NULL);

				// Analyse uniquement si la classe cible na pas deja ete analysees
				if (odAnalysedCreatedClasses->Lookup(kwcTargetClass->GetName()) == NULL)
				{
					// Memorisation de la classe cible
					odAnalysedCreatedClasses->SetAt(kwcTargetClass->GetName(), kwcTargetClass);

					// Recherche de toutes les classes utilisees recursivement
					kwcTargetClass->BuildAllUsedClasses(&oaUsedClass);

					// Recherches des classes externes
					for (nUsedClass = 0; nUsedClass < oaUsedClass.GetSize(); nUsedClass++)
					{
						kwcUsedClass = cast(KWClass*, oaUsedClass.GetAt(nUsedClass));

						// Memorisation des mappings a traiter dans le cas de classe externes
						if (kwcUsedClass->GetRoot())
						{
							if (odReferenceClasses->Lookup(kwcUsedClass->GetName()) == NULL)
							{
								odReferenceClasses->SetAt(kwcUsedClass->GetName(),
											  kwcUsedClass);
								oaRankedReferenceClasses->Add(kwcUsedClass);
							}
						}
					}
				}
			}
			// Cas d'un attribut natif reference (avec regle de derivation predefinie)
			else if (attribute->GetAnyDerivationRule()->GetName() ==
				 KWDerivationRule::GetReferenceRuleName())
			{
				// Memorisation du mapping a traiter
				if (odReferenceClasses->Lookup(attribute->GetClass()->GetName()) == NULL)
				{
					odReferenceClasses->SetAt(attribute->GetClass()->GetName(),
								  attribute->GetClass());
					oaRankedReferenceClasses->Add(attribute->GetClass());
				}
			}
		}

		// Attribut suivant
		mappedClass->GetNextAttribute(attribute);
	}
	return mapping;
}

boolean KWMTDatabase::PhysicalReadAllReferenceObjects(double dSamplePercentage)
{
	boolean bOk = true;
	int nReference;
	KWMTDatabaseMapping* referenceMapping;
	KWClass* kwcReferenceClass;
	KWClass* kwcReferencePhysicalClass;
	KWObject* kwoObject;
	KWObjectKey objectKey;
	ObjectArray oaAllReferenceObjects;
	longint lRecordNumber;
	longint lObjectNumber;
	int nObject;
	ALString sTmp;
	ALString sMessage;
	boolean bIsInTask;
	longint lCurrentMemoryGuardMaxSecondaryRecordNumber;
	longint lCurrentMemoryGuardSingleInstanceMemoryLimit;

	require(objectReferenceResolver.GetClassNumber() == 0);
	require(kwcPhysicalClass != NULL);
	require(kwcPhysicalClass->GetName() == GetClassName());
	require(0 <= dSamplePercentage and dSamplePercentage <= 1);

	// On desactive temporairement le memory guard, le temps de la lecture des objets references
	// En effet, le dimensionnement est calcule pour pouvoir charger en memoire l'ensemble de toutes
	// les tables extertnes, et il n'y a pas a se soucier de la gestion des instances "elephants"
	lCurrentMemoryGuardMaxSecondaryRecordNumber = memoryGuard.GetMaxSecondaryRecordNumber();
	lCurrentMemoryGuardSingleInstanceMemoryLimit = memoryGuard.GetSingleInstanceMemoryLimit();
	memoryGuard.Reset();

	// On teste si on en en cours de suivi de tache
	bIsInTask = TaskProgression::IsInTask();

	// Activation du controle d'erreur, car la lecture des tables references
	// peut potentiellement en declencher un grand nombre
	Global::ActivateErrorFlowControl();

	// Enregistrement du dictionnaire physique principal dans le resolveur de reference, s'il est Root
	// Cela permettra de resoudre les references intra-classe pour la classe principale
	if (kwcPhysicalClass->GetRoot())
		objectReferenceResolver.AddClass(kwcPhysicalClass);

	// Reinitialisation du nombre d'erreurs d'encodage liees aux tables externes
	lExternalTablesEncodingErrorNumber = 0;

	// Reinitialisation des index de creation d'instances pour les objets des tables externes
	//
	// Pour les objets de la table principale, ces compteurs sont reinitialises pour chaque instance
	// principale, en utilisant son index de creation comme reference pour les index de
	// creation d'instance, de facon locale a chaque instance principale
	//
	// Pour les instances racines de tables externes, on procede differemment en prenant 0 comme index unique
	// de reference pour l'ensemble de toutes les instances racine de chaque table externe:
	// - l'identification unique de chaque objet issu d'une regle de derivation de creation d'instance est
	//   garantie, car chaque process lit en memoire toutes les instances externes, contrairement aux cas des
	//   instances de la table principale, qui ont besoin de leur numero de ligne pour garantir un identifiant unique
	// - cette fois, les index de creation des instances crees sont globaux a toutes les instances de chaque
	//   table secondaire, et non locaux a leur instance racine de rattachement
	// - on est oblige techniquement de proceder ainsi, en raison du traitement particulier des instances
	//   des tables externes
	//  - phase 1: on lit toutes les instances de toutes les tables externes, en ne traitant que la partie
	//   stockee du flocon
	//    - et on memorise dans un objectReferenceResolver un pointeur sur chaque objet externe
	//  - phase 2: on calcule la valeur de toutes les regles de derivation des instances externes, recursivement
	//    a partir de chaque instance principale
	//    - le objectReferenceResolver permet de resoudre les references aux objets externes, entre les tables externes
	//      - deux tables externes peuvent ainsi avoir des objets se referencant mutuellement
	//    - un calcul sur un objet racine externe peut ainsi se propager sur d'autres objets racines externes
	//      d'autres tables, voir de la meme table
	//    - dans ce cas, il n'est plus possible de reinitialiser les compteurs de creation d'instance sequentiellement
	//      par instance racine, et on utilise donc ici une initialisation unique, une fois pour toute, par data path
	//      de racine de table externe
	for (nReference = 0; nReference < objectDataPathManager->GetExternalRootDataPathNumber(); nReference++)
		objectDataPathManager->GetExternalRootDataPathAt(nReference)->ResetCreationNumber(0);

	// Ouverture de chaque table secondaire
	for (nReference = 0; nReference < oaRootReferenceTableMappings.GetSize(); nReference++)
	{
		referenceMapping = cast(KWMTDatabaseMapping*, oaRootReferenceTableMappings.GetAt(nReference));

		// Acces aux classes physique et logique referencees, en passant par le domaine de la classe principale
		kwcReferenceClass = kwcClass->GetDomain()->LookupClass(referenceMapping->GetClassName());
		kwcReferencePhysicalClass =
		    kwcPhysicalClass->GetDomain()->LookupClass(referenceMapping->GetClassName());
		check(kwcReferenceClass);
		check(kwcReferencePhysicalClass);

		// Nettoyage prealable
		DMTMPhysicalTerminateMapping(referenceMapping);

		// Initialisation recursive du mapping a partir de la table principale
		DMTMPhysicalInitializeMapping(referenceMapping, kwcReferencePhysicalClass, true);

		// Ouverture recursive des tables a partir de la table principale, si sa cle est chargee
		// Les etapes precedentes ont determine si la table etait utile, et dans le cas inverse
		// la cle n'est pas chargee
		if (referenceMapping->GetDataTableDriver()->GetDataTableName() != "" and
		    kwcReferencePhysicalClass->IsKeyLoaded())
		{
			bOk = DMTMPhysicalOpenForRead(referenceMapping, kwcReferenceClass);

			// Lecture exhaustive de chaque table secondaire et enregistrement dans le resolveur de
			// reference
			if (bOk)
			{
				// Enregistrement du dictionnaire physique dans le resolveur de reference
				objectReferenceResolver.AddClass(kwcReferencePhysicalClass);

				// Debut de suivi de tache
				if (bIsInTask)
				{
					TaskProgression::BeginTask();
					sMessage = "Read external data table " + referenceMapping->GetDataTableName();
					TaskProgression::DisplayLabel(sMessage);
				}

				// Lecture des objets
				lRecordNumber = 0;
				lObjectNumber = 0;
				while (not referenceMapping->GetDataTableDriver()->IsEnd())
				{
					// Lecture d'un enregistrement de la table principale
					kwoObject = DMTMPhysicalRead(referenceMapping);
					lRecordNumber++;

					// Enregistrement dans le resolveur de reference
					if (kwoObject != NULL)
					{
						assert(kwoObject->GetClass() == kwcReferencePhysicalClass);
						objectKey.InitializeFromObject(kwoObject);
						objectReferenceResolver.AddObject(kwcReferencePhysicalClass, &objectKey,
										  kwoObject);
						lObjectNumber++;

						// Enregistrement dans l'ensemble de tous les objets de reference
						oaAllReferenceObjects.Add(kwoObject);
					}

					// Arret si erreur
					if (IsError())
					{
						bOk = false;
						AddError(sTmp + "Read external data table " +
							 referenceMapping->GetDataTableName() +
							 " interrupted because of errors");
						break;
					}

					// Arret l'objet rendu est NULL en raison d'une interruption utilisateur
					if (kwoObject == NULL and TaskProgression::IsInterruptionRequested())
					{
						bOk = false;
						break;
					}

					// Arret si pourcentage atteint
					if (dSamplePercentage < 1 and
					    referenceMapping->GetDataTableDriver()->GetReadPercentage() >
						dSamplePercentage)
						break;

					// Suivi de la tache
					if (bIsInTask and TaskProgression::IsRefreshNecessary())
					{
						TaskProgression::DisplayLabel(sMessage + ": " +
									      LongintToReadableString(lObjectNumber) +
									      " records");
						TaskProgression::DisplayProgression(int(
						    100 * referenceMapping->GetDataTableDriver()->GetReadPercentage()));
					}
				}

				// Test si interruption sans qu'il y ait d'erreur
				if (bIsInTask and not IsError() and TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					Object::AddWarning("Read database interrupted by user");
				}

				// Fin de suivi de tache
				if (bIsInTask)
				{
					TaskProgression::DisplayLabel("");
					TaskProgression::DisplayProgression(0);
					TaskProgression::EndTask();
				}
			}

			// Mise a jour du nombre d'erreurs d'encodage liees aux tables externes
			lExternalTablesEncodingErrorNumber += DMTMPhysicalComputeEncodingErrorNumber(referenceMapping);

			// Fermeture de toutes les sous-bases referencees (qu'il y ait echec ou non de l'ouverture)
			bOk = DMTMPhysicalClose(referenceMapping) and bOk;
		}

		// Nettoyage final
		DMTMPhysicalTerminateMapping(referenceMapping);

		// Arret si erreurs
		if (not bOk)
			break;
	}

	// Calcul de tous les attributs derives pour les objets de chaque table secondaire
	// Comme les regles peuvent se faire entre objets de tables differentes, ce calcul
	// ne peut etre effectue qu'une fois tous les objets charges en memoire
	// Attention, contrairement aux objets de la classe principale d'analyse et ses object inclus,
	// les objets des tables externes sont calcules au niveau physique, mais ne sont pas mutes
	// (methode MutatePhysicalObject()) au niveau logique. En effet, les objets des tables externes
	// serviront au calculs d'attributs derives pour toutes les objets physiques de la classe
	// principale d'analyse avant leur mutation
	if (bOk)
	{
		// Parametrage du resolveur de reference dans la regle de derivation gerant les references
		assert(KWDRReference::GetObjectReferenceResolver() == NULL);
		KWDRReference::SetObjectReferenceResolver(&objectReferenceResolver);

		// Debut de suivi de tache
		if (bIsInTask)
		{
			TaskProgression::BeginTask();
			sMessage = "Compute external table record values";
			TaskProgression::DisplayLabel(sMessage);
		}

		// Parcours de tous les objet racine des tables externe dans l'ordre de leur lecture
		// Attention, cet ordre est necessaire pour garantir la reproductibilite de resultats,
		// notamment en ce qui concernes les objets issu d'une regle de derivation de creation d'instance
		// dont l'index de creation est locl a l'ensemble de toutes les instances
		for (nObject = 0; nObject < oaAllReferenceObjects.GetSize(); nObject++)
		{
			kwoObject = cast(KWObject*, oaAllReferenceObjects.GetAt(nObject));
			assert(kwoObject->GetClass()->GetDomain() == kwcPhysicalClass->GetDomain());

			// Calcul recursif de toutes les valeurs des objets
			kwoObject->ComputeAllValues(&memoryGuard);

			// Suivi de la tache
			if (bIsInTask and TaskProgression::IsRefreshNecessary())
			{
				if (TaskProgression::IsInterruptionRequested())
				{
					TaskProgression::DisplayLabel(
					    sMessage + LongintToReadableString((longint)nObject) + " records");
					TaskProgression::DisplayProgression(
					    int(100 * double(nObject) / oaAllReferenceObjects.GetSize()));
					bOk = false;
					AddWarning(sMessage + " interrupted by user");
					break;
				}
			}
		}

		// Fin de suivi de tache
		if (bIsInTask)
		{
			TaskProgression::DisplayLabel("");
			TaskProgression::DisplayProgression(0);
			TaskProgression::EndTask();
		}

		// Remise a NULL du resolveur de reference dans la regle de derivation gerant les references
		KWDRReference::SetObjectReferenceResolver(NULL);
	}

	// Desactivation du controle d'erreur
	Global::DesactivateErrorFlowControl();

	// On reactive le memory guard
	memoryGuard.SetMaxSecondaryRecordNumber(lCurrentMemoryGuardMaxSecondaryRecordNumber);
	memoryGuard.SetSingleInstanceMemoryLimit(lCurrentMemoryGuardSingleInstanceMemoryLimit);
	return bOk;
}

void KWMTDatabase::PhysicalDeleteAllReferenceObjects()
{
	objectReferenceResolver.DeleteAll();
}

longint KWMTDatabase::ComputeSamplingBasedNecessaryMemoryForReferenceObjects()
{
	boolean bDisplay = false;
	longint lSamplingBasedNecessaryMemory;
	longint lHeapMemory;
	longint lReadBasedNecessaryMemory;
	longint lDeleteBasedNecessaryMemory;
	longint lNecessaryMemory;
	longint lRemainingMemory;
	longint lUsableMemory;
	double dSamplePercentage;
	boolean bCurrentSilentMode;

	require(Check());
	require(kwcClass != NULL);
	require(kwcPhysicalClass != NULL);
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());

	// Premiere passe basique d'estimation de la memoire necessaire, sans acces aux donnees
	lNecessaryMemory = ComputeNecessaryMemoryForReferenceObjects();
	if (bDisplay)
	{
		cout << "Necessary memory for reference objects for database " << GetDatabaseName() << endl;
		cout << "\tIn memory estimation\t" << LongintToHumanReadableString(lNecessaryMemory) << endl;
	}

	// Estimation de la memoire totale restante pour en donner au max la moitie au maitre
	lRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();
	if (bDisplay)
		cout << "\tRemaining memory: " << LongintToHumanReadableString(lRemainingMemory) << endl;

	// Calcul du taux d'echantillonnage des objets references a utiliser
	// On prend de la marge au cas ou l'estimation initiale serait tres mauvaise
	lUsableMemory = min(lRemainingMemory / 4, (longint)SystemFile::nMaxPreferredBufferSize);
	lUsableMemory = max(lUsableMemory, (longint)InputBufferedFile::nDefaultBufferSize);
	if (lNecessaryMemory > lUsableMemory)
		dSamplePercentage = min(0.1, double(lUsableMemory) / lNecessaryMemory);
	else
		dSamplePercentage = 0.1;
	if (bDisplay)
		cout << "\tSampling rate\t" << DoubleToString(dSamplePercentage) << endl;

	// Initialisation de tous les data paths a destination des objets lus ou cree
	// le temps de la lecture des objet externes
	objectDataPathManager->ComputeAllDataPaths(kwcPhysicalClass);

	// Ouverture et lecture des tables referencees, sans emission d'erreur
	// ce qui permet de calculer la memoire necessaire a leur lecture
	lHeapMemory = MemGetHeapMemory();
	bCurrentSilentMode = Global::GetSilentMode();
	Global::SetSilentMode(true);
	PhysicalReadAllReferenceObjects(dSamplePercentage);
	Global::SetSilentMode(bCurrentSilentMode);
	lReadBasedNecessaryMemory = MemGetHeapMemory() - lHeapMemory;

	// Destruction des objets references
	// ce qui permet de calculer la memoire necessaire a leur destruction
	lHeapMemory = MemGetHeapMemory();
	PhysicalDeleteAllReferenceObjects();
	lDeleteBasedNecessaryMemory = lHeapMemory - MemGetHeapMemory();

	// Destruction des data paths de gestion des objets
	objectDataPathManager->Reset();

	// Memoire dediee aux objets references
	// On prend le max des deux, car il peut y avoir des effet de bord dans l'allocateur, avec la gestion des
	// segments memoire
	lSamplingBasedNecessaryMemory = max(lReadBasedNecessaryMemory, lDeleteBasedNecessaryMemory);
	assert(lSamplingBasedNecessaryMemory >= 0);
	lSamplingBasedNecessaryMemory = (longint)(lSamplingBasedNecessaryMemory / dSamplePercentage);

	// On passe de la memoire physique a la memoire logique en tenant compte de l'ovehead d'(allocation
	lSamplingBasedNecessaryMemory = longint(lSamplingBasedNecessaryMemory / (1 + MemGetAllocatorOverhead()));
	if (bDisplay)
		cout << "\tSampling based estimation\t" << LongintToHumanReadableString(lSamplingBasedNecessaryMemory)
		     << endl;
	ensure(lSamplingBasedNecessaryMemory >= 0);
	return lSamplingBasedNecessaryMemory;
}

longint KWMTDatabase::ComputeNecessaryMemoryForReferenceObjects()
{
	boolean bDisplay = false;
	longint lTotalNecessaryMemory;
	longint lNecessaryMemory;
	int nReference;
	KWMTDatabaseMapping* referenceMapping;
	KWClass* kwcReferenceClass;
	KWClass* kwcReferencePhysicalClass;
	ObjectArray oaFullHierarchyComponentTableMappings;
	KWMTDatabaseMapping* mapping;
	KWClass* kwcMappingLogicalClass;
	int i;
	ALString sTmp;

	require(Check());
	require(kwcClass != NULL);
	require(kwcPhysicalClass != NULL);
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());

	// Ouverture de chaque table secondaire pour une premiere estimation basique de la memoire necessaire, sans
	// acces aux donnees
	lTotalNecessaryMemory = 0;
	for (nReference = 0; nReference < oaRootReferenceTableMappings.GetSize(); nReference++)
	{
		referenceMapping = cast(KWMTDatabaseMapping*, oaRootReferenceTableMappings.GetAt(nReference));

		// Acces aux classes physique et logique referencees, en passant par le domaine de la classe principale
		kwcReferenceClass = kwcClass->GetDomain()->LookupClass(referenceMapping->GetClassName());
		kwcReferencePhysicalClass =
		    kwcPhysicalClass->GetDomain()->LookupClass(referenceMapping->GetClassName());
		check(kwcReferenceClass);
		check(kwcReferencePhysicalClass);

		// Nettoyage prealable
		DMTMPhysicalTerminateMapping(referenceMapping);

		// Initialisation recursive du mapping a partir de la table principale
		DMTMPhysicalInitializeMapping(referenceMapping, kwcReferencePhysicalClass, true);

		// Collecte de tous les mapping de la hierarchie de composition
		oaFullHierarchyComponentTableMappings.SetSize(0);
		referenceMapping->CollectFullHierarchyComponentTableMappings(&oaFullHierarchyComponentTableMappings);

		// Parcours des mapping de la hierarchie
		for (i = 0; i < oaFullHierarchyComponentTableMappings.GetSize(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, oaFullHierarchyComponentTableMappings.GetAt(i));

			// On ne prend en compte que les mapping effectivement utilises
			lNecessaryMemory = 0;
			if (mapping->GetDataTableDriver() != NULL)
			{
				// Recherche de la classe de reference du mapping, pour connaitre le nombre d'attribut
				// natifs initial avant le passage par la classe physique
				kwcMappingLogicalClass = kwcClass->GetDomain()->LookupClass(mapping->GetClassName());

				// Calcul de la memoire, en tenant compte du nombre d'attributs natifs
				lNecessaryMemory =
				    mapping->GetDataTableDriver()->ComputeNecessaryMemoryForFullExternalRead(
					kwcMappingLogicalClass);
			}
			lTotalNecessaryMemory += lNecessaryMemory;
			if (bDisplay)
				cout << "Necessary memory for " << mapping->GetObjectLabel() << ": "
				     << LongintToHumanReadableString(lNecessaryMemory) << endl;
		}

		// Nettoyage final
		DMTMPhysicalTerminateMapping(referenceMapping);
	}
	if (bDisplay)
		cout << "Necessary memory for reference objects for database " << GetDatabaseName() << ": "
		     << LongintToHumanReadableString(lTotalNecessaryMemory) << endl;
	return lTotalNecessaryMemory;
}

void KWMTDatabase::DMTMPhysicalInitializeMapping(KWMTDatabaseMapping* mapping, KWClass* mappedClass, boolean bRead)
{
	int i;
	ALString sAttributeName;
	KWAttribute* attribute;
	KWDataTableDriver* mappedDataTableDriver;
	KWMTDatabaseMapping* componentMapping;
	int nDataPathAttributeNumber;

	require(mapping != NULL);
	require(mappedClass != NULL);

	// Recherche du nombre d'attributs dans le chemin de donnees du mapping
	nDataPathAttributeNumber = mapping->GetDataPathAttributeNumber();

	// Cas ou le mapping est un mapping de classe, sans chemin d'attributs
	if (nDataPathAttributeNumber == 0)
	{
		assert(mapping->GetDataPathAttributeNames() == "");
		assert(mapping->GetClassName() == mappedClass->GetName());
		assert(mapping->GetDataTableDriver() == NULL);

		// Creation et parametrage du driver associe au mapping
		mappedDataTableDriver = CreateDataTableDriver(mapping);
		mapping->SetDataTableDriver(mappedDataTableDriver);
		mappedDataTableDriver->SetClass(mappedClass);
		mappedDataTableDriver->SetDataTableName(mapping->GetDataTableName());
	}
	// En lecture (dans le domaine classe physique), la classe mappee doit avoir les attributs cle en load
	assert(nDataPathAttributeNumber == 0 or mappedClass->GetDomain() == kwcClass->GetDomain() or
	       mapping->GetDataTableDriver()->GetClass()->IsKeyLoaded());

	// Parametrage du mapping de la composition, si necessaire en lecture, toujours en ecriture
	if (mappedClass->IsKeyLoaded() or not bRead)
	{
		assert(mapping->GetDataTableDriver() != NULL);
		for (i = 0; i < mapping->GetComponentTableMappings()->GetSize(); i++)
		{
			componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(i));
			assert(componentMapping->GetDataPathAttributeNumber() == nDataPathAttributeNumber + 1);
			assert(componentMapping->GetDataTableDriver() == NULL);

			// Recherche de l'index de l'attribut mappe, s'il est present dans la classe physique
			sAttributeName = componentMapping->GetDataPathAttributeNameAt(nDataPathAttributeNumber);
			attribute = mappedClass->LookupAttribute(sAttributeName);
			assert(attribute == NULL or attribute->GetClass() != NULL);

			// Parametrage du mapping si base specifiee et si la classe mappe est indexable
			if (componentMapping->GetDataTableName() != "" and attribute != NULL and
			    attribute->GetClass()->GetKeyAttributeNumber() > 0 and attribute->GetLoaded())
			{
				// Creation d'un driver de table ayant par defaut les memes caracteristiques que la base
				// courante
				assert(componentMapping->GetDataTableDriver() == NULL);
				mappedDataTableDriver = CreateDataTableDriver(componentMapping);
				componentMapping->SetDataTableDriver(mappedDataTableDriver);

				// Specialisation du parametrage: c'est la classe physique qui est utilisee en lecture
				assert(KWType::IsRelation(attribute->GetType()));
				mappedDataTableDriver->SetClass(attribute->GetClass());
				mappedDataTableDriver->SetDataTableName(componentMapping->GetDataTableName());

				// Parametrage de l'attribut mappe
				componentMapping->SetMappedAttributeLoadIndex(attribute->GetLoadIndex());
				componentMapping->SetMappedAttributeType(attribute->GetType());

				// Propagation aux sous-composants
				DMTMPhysicalInitializeMapping(componentMapping, attribute->GetClass(), bRead);
			}
		}
	}
}

void KWMTDatabase::DMTMPhysicalTerminateMapping(KWMTDatabaseMapping* mapping)
{
	KWMTDatabaseMapping* componentMapping;
	int i;

	require(mapping != NULL);

	// Destruction du driver de table
	if (mapping->GetDataTableDriver() != NULL)
	{
		assert(not mapping->GetDataTableDriver()->IsOpenedForRead() and
		       not mapping->GetDataTableDriver()->IsOpenedForWrite());
		mapping->GetDataTableDriver()->SetClass(NULL);
		delete mapping->GetDataTableDriver();
		mapping->SetDataTableDriver(NULL);
	}

	// Reinitialisation de l'index de l'attribut mappe
	mapping->ResetMappedAttributeLoadIndex();

	// Destruction du dernier objet lu en cours si necessaire
	if (mapping->GetLastReadObject() != NULL)
	{
		delete mapping->GetLastReadObject();
		mapping->SetLastReadObject(NULL);
	}
	mapping->CleanLastReadKey();

	// Propagation aux mappings de la composition
	for (i = 0; i < mapping->GetComponentTableMappings()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(i));
		DMTMPhysicalTerminateMapping(componentMapping);
	}
}

boolean KWMTDatabase::DMTMPhysicalOpenForRead(KWMTDatabaseMapping* mapping, const KWClass* kwcLogicalClass)
{
	boolean bOk;
	KWMTDatabaseMapping* componentMapping;
	int i;
	ALString sAttributeName;
	KWAttribute* physicalAttribute;
	KWAttribute* attribute;
	int nDataPathAttributeNumber;

	require(mapping != NULL);
	require(mapping->GetDataTableDriver() != NULL);
	require(kwcLogicalClass != NULL);

	// Arret immediat si pas de fichier specifie
	if (mapping->GetDataTableDriver()->GetDataTableName() == "")
		return false;

	// Recherche du nombre d'attributs dans le chemin de donnees du mapping
	nDataPathAttributeNumber = mapping->GetDataPathAttributeNumber();

	// Cas ou le mapping est un mapping de classe, sans chemin d'attributs
	bOk = true;
	if (nDataPathAttributeNumber == 0)
	{
		assert(mapping->GetDataPathAttributeNames() == "");
		assert(mapping->GetClassName() == mapping->GetDataTableDriver()->GetClass()->GetName());
		assert(not mapping->GetDataTableDriver()->IsOpenedForRead());
		assert(not mapping->GetDataTableDriver()->IsOpenedForWrite());

		// Ouverture de la table du mapping
		bOk = mapping->GetDataTableDriver()->OpenForRead(kwcLogicalClass);
		mapping->CleanLastReadKey();
	}

	// Ouverture des sous-bases
	if (bOk)
	{
		assert(mapping->GetDataTableDriver()->IsOpenedForRead());

		// Ouverture de chaque sous-table, si necessaire
		if (mapping->GetDataTableDriver()->GetClass()->IsKeyLoaded())
		{
			// Propagation aux mappings de la composition
			for (i = 0; i < mapping->GetComponentTableMappings()->GetSize(); i++)
			{
				componentMapping =
				    cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(i));

				// Ouverture de la table du mapping composant
				if (componentMapping->GetDataTableDriver() != NULL and
				    componentMapping->GetDataTableDriver()->GetDataTableName() != "")
				{
					// Ouverture physique de la sous-base si necessaire
					sAttributeName =
					    componentMapping->GetDataPathAttributeNameAt(nDataPathAttributeNumber);
					physicalAttribute =
					    mapping->GetDataTableDriver()->GetClass()->LookupAttribute(sAttributeName);
					attribute = kwcLogicalClass->LookupAttribute(sAttributeName);
					if (physicalAttribute != NULL)
					{
						assert(KWType::IsRelation(physicalAttribute->GetType()));
						assert(physicalAttribute->GetClass()->IsCompiled());
						assert(componentMapping->GetDataTableDriver()->GetClass() ==
						       physicalAttribute->GetClass());
						assert(attribute->GetClass() != NULL);
						assert(attribute->GetClass()->GetName() ==
						       physicalAttribute->GetClass()->GetName());

						// Ouverture de la sous-table
						componentMapping->GetDataTableDriver()->OpenForRead(
						    attribute->GetClass());
						bOk = bOk and componentMapping->GetDataTableDriver()->IsOpenedForRead();

						// Propagation si OK
						if (bOk)
							bOk = DMTMPhysicalOpenForRead(componentMapping,
										      attribute->GetClass());
						// Arret sinon
						else
							break;
					}
				}
			}
		}
	}
	return bOk;
}

boolean KWMTDatabase::DMTMPhysicalOpenForWrite(KWMTDatabaseMapping* mapping)
{
	boolean bOk;
	KWMTDatabaseMapping* componentMapping;
	int i;
	ALString sAttributeName;
	KWAttribute* physicalAttribute;
	int nDataPathAttributeNumber;

	require(mapping != NULL);
	require(mapping->GetDataTableDriver() != NULL);

	// Arret immediat si pas de fichier specifie
	if (mapping->GetDataTableDriver()->GetDataTableName() == "")
		return false;

	// Recherche du nombre d'attributs dans le chemin de donnees du mapping
	nDataPathAttributeNumber = mapping->GetDataPathAttributeNumber();

	// Cas ou le mapping est un mapping de classe, sans chemin d'attributs
	bOk = true;
	if (nDataPathAttributeNumber == 0)
	{
		assert(mapping->GetDataPathAttributeNames() == "");
		assert(mapping->GetClassName() == mapping->GetDataTableDriver()->GetClass()->GetName());
		assert(not mapping->GetDataTableDriver()->IsOpenedForRead());
		assert(not mapping->GetDataTableDriver()->IsOpenedForWrite());

		// Ouverture de la table du mapping
		bOk = mapping->GetDataTableDriver()->OpenForWrite();
		mapping->CleanLastReadKey();
	}

	// Ouverture des sous-bases
	if (bOk)
	{
		assert(mapping->GetDataTableDriver()->IsOpenedForWrite());

		// Ouverture de chaque sous-table, meme si les cle ne sont pas chargee en memoire
		// La semantique de l'ecriture et d'ecrire tout ce qui en Used
		// Propagation aux mappings de la composition
		for (i = 0; i < mapping->GetComponentTableMappings()->GetSize(); i++)
		{
			componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(i));

			// Ouverture de la table du mapping composant
			if (componentMapping->GetDataTableDriver() != NULL and
			    componentMapping->GetDataTableDriver()->GetDataTableName() != "")
			{
				// Ouverture physique de la sous-base si necessaire
				sAttributeName = componentMapping->GetDataPathAttributeNameAt(nDataPathAttributeNumber);
				physicalAttribute =
				    mapping->GetDataTableDriver()->GetClass()->LookupAttribute(sAttributeName);
				if (physicalAttribute != NULL)
				{
					assert(KWType::IsRelation(physicalAttribute->GetType()));
					assert(physicalAttribute->GetClass()->IsCompiled());
					assert(componentMapping->GetDataTableDriver()->GetClass() ==
					       physicalAttribute->GetClass());

					// Ouverture de la sous-table
					componentMapping->GetDataTableDriver()->OpenForWrite();
					bOk = bOk and componentMapping->GetDataTableDriver()->IsOpenedForWrite();

					// Propagation si ok
					if (bOk)
						bOk = DMTMPhysicalOpenForWrite(componentMapping);
					// Arret sinon
					else
						break;
				}
			}
		}
	}
	return bOk;
}

boolean KWMTDatabase::DMTMPhysicalClose(KWMTDatabaseMapping* mapping)
{
	boolean bOk = true;
	KWDataTableDriver* mappedDataTableDriver;
	KWMTDatabaseMapping* componentMapping;
	int i;

	require(mapping != NULL);

	// Fermeture de la base
	mappedDataTableDriver = mapping->GetDataTableDriver();
	if (mappedDataTableDriver != NULL and mappedDataTableDriver->GetDataTableName() != "")
	{
		if (mappedDataTableDriver->IsOpenedForRead() or mappedDataTableDriver->IsOpenedForWrite())
			bOk = mappedDataTableDriver->Close() and bOk;
	}

	// Destruction du driver
	if (mappedDataTableDriver != NULL)
	{
		mappedDataTableDriver->SetClass(NULL);
		delete mappedDataTableDriver;
		mapping->SetDataTableDriver(NULL);
	}

	// Reinitialisation de l'index de l'attribut mappe
	mapping->ResetMappedAttributeLoadIndex();

	// Destruction du dernier objet lu en cours si necessaire
	if (mapping->GetLastReadObject() != NULL)
	{
		delete mapping->GetLastReadObject();
		mapping->SetLastReadObject(NULL);
	}
	mapping->CleanLastReadKey();

	// Propagation aux mappings de la composition
	for (i = 0; i < mapping->GetComponentTableMappings()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(i));
		bOk = DMTMPhysicalClose(componentMapping) and bOk;
	}
	return bOk;
}

longint KWMTDatabase::DMTMPhysicalComputeEncodingErrorNumber(KWMTDatabaseMapping* mapping) const
{
	longint lNumber;
	KWDataTableDriver* mappedDataTableDriver;
	KWMTDatabaseMapping* componentMapping;
	int i;

	require(mapping != NULL);

	// Nombre d'erreur d'encodage du mapping principal
	lNumber = 0;
	mappedDataTableDriver = mapping->GetDataTableDriver();
	if (mappedDataTableDriver != NULL and mappedDataTableDriver->GetDataTableName() != "")
	{
		if (mappedDataTableDriver->IsOpenedForRead())
			lNumber += mappedDataTableDriver->GetEncodingErrorNumber();
	}

	// Propagation aux mappings de la composition
	for (i = 0; i < mapping->GetComponentTableMappings()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(i));
		lNumber += DMTMPhysicalComputeEncodingErrorNumber(componentMapping);
	}
	return lNumber;
}

void KWMTDatabase::DMTMPhysicalDeleteDatabase(KWMTDatabaseMapping* mapping)
{
	KWDataTableDriver* mappedDataTableDriver;
	KWMTDatabaseMapping* componentMapping;
	int i;

	require(mapping != NULL);

	// Creation d'un driver associe au mapping, pour pouvoir detruire la table correspondante
	mappedDataTableDriver = CreateDataTableDriver(mapping);
	mappedDataTableDriver->SetDataTableName(mapping->GetDataTableName());

	// Destruction de la base si possible
	mappedDataTableDriver->DeleteDataTable();

	// Nettoyage
	delete mappedDataTableDriver;

	// Propagation aux mappings de la composition
	for (i = 0; i < mapping->GetComponentTableMappings()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(i));
		DMTMPhysicalDeleteDatabase(componentMapping);
	}
}

KWObject* KWMTDatabase::DMTMPhysicalRead(KWMTDatabaseMapping* mapping)
{
	const KWObjectDataPath* objectDataPath;
	KWObject* kwoObject;
	KWMTDatabaseMapping* componentMapping;
	int i;
	KWDataTableDriver* mappedDataTableDriver;
	KWObject* kwoSubObject;
	KWObjectKey objectKey;
	KWObjectKey subObjectKey;
	boolean bNewSubObject;
	ObjectArray* oaSubObjects;
	ALString sTmp;
	ALString sObjectLabel;
	ALString sOtherObjectLabel;

	require(mapping != NULL);
	require(not mapping->GetDataTableDriver()->IsEnd());

	// Lecture d'un enregistrement de la table principale
	kwoObject = mapping->GetDataTableDriver()->Read();

	//DDD
	// Parametrage du data path de l'objet
	if (kwoObject != NULL)
	{
		// Recherche du data path a associe a l'objet
		objectDataPath = objectDataPathManager->LookupDataPath(mapping->GetDataPath());

		// Memorisation du data path dans l'objet
		kwoObject->SetDataPath(objectDataPath);

		// Reinitialisation des informations de gestion de creation d'instance pour tous les
		// sous data path dans le cas d'un objet princial
		if (mapping->GetDataTableDriver()->GetClass() == kwcPhysicalClass)
		{
			assert(objectDataPath == objectDataPathManager->GetMainDataPath());
			objectDataPathManager->GetMainDataPath()->ResetCreationNumber(kwoObject->GetCreationIndex());
		}
	}

	// Positionnement du flag d'erreur
	bIsError = bIsError or mapping->GetDataTableDriver()->IsError();

	// Incrementation du nombre global d'enregistrements lus
	// Si on voulait des stats relative uniquement aux instances principales selectionnees, cela devrait se faire
	// a posteriori une fois les instances selectionnees entierement validees, au niveau de la methode Read
	if (kwoObject != NULL and not bIsError)
		mapping->GetDataTableDriver()->SetUsedRecordNumber(
		    mapping->GetDataTableDriver()->GetUsedRecordNumber() + 1);

	// Memorisation inconditionnelle de la cle du dernier enregistrement lu, dans le cas d'une classe unique,
	// meme si l'objet n'a pas pu etre lu
	// Cela permet de gere les lignes dupliquees, que l'objet soit lu ou non (a cause d'une erreur de parsing)
	if (kwoObject == NULL)
	{
		if (mapping->GetDataTableDriver()->GetClass()->IsUnique())
		{
			assert(mapping->GetDataTableDriver()->GetLastReadMainKey()->GetSize() ==
			       mapping->GetDataTableDriver()->GetClass()->GetKeyAttributeNumber());
			mapping->SetLastReadKey(mapping->GetDataTableDriver()->GetLastReadMainKey());
		}
		return NULL;
	}

	// Lecture potentielle dans chaque sous-base pour completer le mapping, si necessaire (s'il y a une cle)
	assert(kwoObject != NULL);
	if (kwoObject->GetClass()->IsKeyLoaded())
	{
		// Recherche de la cle de l'objet principal
		objectKey.InitializeFromObject(kwoObject);

		// Gestion de la coherence pour toute classe principale ayant une cle
		// Le cas des classes composant est traite plus loin
		if (mapping == mainMultiTableMapping or kwoObject->GetClass()->IsUnique())
		{
			// Test a partir du deuxieme enregistrement effectivement lu, pour lequel le LastReadKey est
			// initialise)
			if (mapping->GetLastReadKey()->GetSize() > 0)
			{
				assert(mapping->GetDataTableDriver()->GetRecordIndex() > 1);

				// Warning si la cle egale dans le cas d'une classe principale, et supression de
				// l'enregistrement
				if (kwoObject->GetClass()->IsUnique() and
				    objectKey.StrictCompare(mapping->GetLastReadKey()) == 0)
				{
					// Warning de lecture
					mapping->GetDataTableDriver()->AddWarning(
					    sTmp + "Ignored record" + ", duplicate key " + mapping->GetClassName() +
					    objectKey.GetObjectLabel());

					// On ignore l'enregistrement
					delete kwoObject;
					kwoObject = NULL;
				}
				// Erreur si la cle est plus petite, et supression de l'enregistrement
				else if (objectKey.StrictCompare(mapping->GetLastReadKey()) < 0)
				{
					// Creation de libelles distincts
					objectKey.BuildDistinctObjectLabels(mapping->GetLastReadKey(), sObjectLabel,
									    sOtherObjectLabel);

					// Erreur de lecture
					mapping->GetDataTableDriver()->AddError(
					    sTmp + "Unsorted record " + mapping->GetClassName() + sObjectLabel +
					    ", with key inferior to that of the preceding record " +
					    mapping->GetClassName() + sOtherObjectLabel);
					bIsError = true;

					// On ignore l'enregistrement
					delete kwoObject;
					kwoObject = NULL;
				}
			}

			// Memorisation de la cle du dernier objet lu, que l'enregistrement soit ignore ou non
			mapping->SetLastReadKey(&objectKey);

			// Retour si enregistrement ignore
			if (kwoObject == NULL)
				return NULL;
		}

		// Parcours des mappings de la composition  pour completer la lecture de l'objet
		for (i = 0; i < mapping->GetComponentTableMappings()->GetSize(); i++)
		{
			componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(i));

			// Lecture dans la sous base
			mappedDataTableDriver = componentMapping->GetDataTableDriver();
			if (mappedDataTableDriver != NULL and mappedDataTableDriver->GetDataTableName() != "" and
			    mappedDataTableDriver->IsOpenedForRead())
			{
				// Lecture des sous-objets
				oaSubObjects = NULL;
				kwoSubObject = NULL;
				while (componentMapping->GetLastReadObject() != NULL or
				       not mappedDataTableDriver->IsEnd())
				{
					bNewSubObject = false;

					// On recupere si necessaire le dernier objet non traite
					if (componentMapping->GetLastReadObject() != NULL)
						kwoSubObject = componentMapping->GetLastReadObject();
					// Sinon, on en lit un nouveau
					else
					{
						// Appel recursif a DMTMPhysicalRead, pour propager la construction de
						// l'objet et de sa composition
						kwoSubObject = DMTMPhysicalRead(componentMapping);
						componentMapping->SetLastReadObject(kwoSubObject);
						bNewSubObject = true;

						// On renvoie NULL avec destruction de l'objet en cours si sous-objet lu
						// NULL en raison d'une interruption utilisateur On effectue ce test car
						// la lecture multi-table peut impliquer enormement de records imbriques
						// (d'autant plus si les fichier secondaires n'ont pas des cles
						// compatibles avec ceux de la table principale)
						if (kwoSubObject == NULL and TaskProgression::IsInterruptionRequested())
						{
							delete kwoObject;
							return NULL;
						}
					}

					// On traite les objets non nuls
					if (kwoSubObject != NULL)
					{
						// Verification de la coherence entre la classe du sous-objet, et sa
						// classe attendue depuis son objet englobant
						assert(componentMapping != mainMultiTableMapping and
						       not kwoSubObject->GetClass()->GetRoot());
						assert(kwoSubObject->GetClass() ==
						       kwoObject->GetClass()
							   ->GetAttributeAtLoadIndex(
							       componentMapping->GetMappedAttributeLoadIndex())
							   ->GetClass());

						// Recherche de la cle de l'objet secondaire
						subObjectKey.InitializeFromObject(kwoSubObject);

						// La cle est identique aux premiers champs du sous-objets
						if (objectKey.SubCompare(&subObjectKey) == 0)
						{
							// Prise en compte dans le memory guard
							memoryGuard.AddReadSecondaryRecord();

							// Le sous-objet etant traite, on peut le dereferencer du
							// mapping
							componentMapping->SetLastReadObject(NULL);

							// Nettoyage si le memory guard a detecte un depassement de
							// limite memoire
							if (memoryGuard.IsSingleInstanceMemoryLimitReached())
							{
								// Destruction du sous-objet non utilisable
								delete kwoSubObject;
							}
							// Pour un attribut de type Object, on associe le sous-objet a
							// son incluant et c'est fini
							else if (componentMapping->GetMappedAttributeType() ==
								 KWType::Object)
							{
								kwoObject->SetObjectValueAt(
								    componentMapping->GetMappedAttributeLoadIndex(),
								    kwoSubObject);

								// Memorisation de la derniere cle lue
								componentMapping->SetLastReadKey(&subObjectKey);

								// Fin du traitement pour ce sous-objet
								break;
							}
							// Pour un attribut de type ObjectArray, on range le sous-objet
							// dans un tableau dans son incluant
							else
							{
								assert(componentMapping->GetMappedAttributeType() ==
								       KWType::ObjectArray);

								// Creation si necessaire du tableau
								if (oaSubObjects == NULL)
								{
									oaSubObjects = new ObjectArray;
									kwoObject->SetObjectArrayValueAt(
									    componentMapping
										->GetMappedAttributeLoadIndex(),
									    oaSubObjects);
								}

								// Rangement du sous-objet dans le tableau
								oaSubObjects->Add(kwoSubObject);
							}

							// Memorisation de la derniere cle lue
							componentMapping->SetLastReadKey(&subObjectKey);
						}
						// Warning si la cle de l'objet est plus grande que celle du sous-objet
						else if (objectKey.SubCompare(&subObjectKey) > 0)
						{
							boolean bWarningEmmited;

							// Erreur ou warning si probleme d'ordonnancement des cles dans
							// le fichier des sous-objets Uniquement a partir du deuxieme
							// enregistrement, et si un nouveau sous-objet a effectivement
							// ete lu
							bWarningEmmited = false;
							if (componentMapping->GetLastReadKey()->GetSize() > 0 and
							    bNewSubObject)
							{
								// Erreur si probleme d'ordonnancement
								if (subObjectKey.StrictCompare(
									componentMapping->GetLastReadKey()) < 0)
								{
									// Creation de libelles distincts
									subObjectKey.BuildDistinctObjectLabels(
									    componentMapping->GetLastReadKey(),
									    sObjectLabel, sOtherObjectLabel);

									mappedDataTableDriver->AddError(
									    sTmp + "Unsorted record " +
									    componentMapping->GetClassName() +
									    sObjectLabel +
									    ", with key inferior to that of the "
									    "preceding record " +
									    componentMapping->GetClassName() +
									    sOtherObjectLabel);
									bIsError = true;
								}
								// Warning si probleme de duplication dans le cas d'un
								// sous-objet
								else if (componentMapping->GetMappedAttributeType() ==
									     KWType::Object and
									 subObjectKey.StrictCompare(
									     componentMapping->GetLastReadKey()) == 0)
								{
									bWarningEmmited = true;
									mappedDataTableDriver->AddWarning(
									    sTmp + "Ignored record" +
									    ", duplicate key " +
									    componentMapping->GetClassName() +
									    subObjectKey.GetObjectLabel());
								}
							}

							// Warning uniquement si aucun enregistrement principal saute
							// En effet, si l'on a "saute" (Skip) des enregistrements du
							// fichier principal, il est licite de trouver des
							// enregistrement secondaires non rattachable a l'objet
							// principal
							if (not bIsError and not bWarningEmmited and
							    nSkippedRecordNumber == 0)
							{
								// Creation de libelles distincts
								subObjectKey.BuildDistinctObjectLabels(
								    &objectKey, sObjectLabel, sOtherObjectLabel);

								bWarningEmmited = true;
								mappedDataTableDriver->AddWarning(
								    sTmp + "Ignored record" + ", orphan record " +
								    componentMapping->GetClassName() + sObjectLabel +
								    " with key inferior to that of the including "
								    "record " +
								    mapping->GetClassName() + sOtherObjectLabel);
							}

							// Destruction du sous-objet non utilise
							delete kwoSubObject;

							// Dereferencement du sous-objet du mapping
							componentMapping->SetLastReadObject(NULL);

							// Memorisation de la derniere cle lue
							componentMapping->SetLastReadKey(&subObjectKey);

							// Arret si erreur
							if (bIsError)
								break;
						}
						// Fin du traitement du sous-objet (qui sera utilise pour le prochain
						// objet principal)
						else
							break;
					}
				}
			}
		}
	}

	return kwoObject;
}

void KWMTDatabase::DMTMPhysicalWrite(KWMTDatabaseMapping* mapping, const KWObject* kwoObject)
{
	KWMTDatabaseMapping* componentMapping;
	KWDataTableDriver* mappedDataTableDriver;
	int i;
	KWObject* kwoSubObject;
	ObjectArray* oaSubObjectArray;
	int nObject;

	require(mapping != NULL);
	require(kwoObject != NULL);

	// Ecriture d'un enregistrement de la table principale
	mapping->GetDataTableDriver()->Write(kwoObject);

	// Incrementation du compteur d'objet utilise au niveau physique
	if (not bIsError)
		mapping->GetDataTableDriver()->SetUsedRecordNumber(
		    mapping->GetDataTableDriver()->GetUsedRecordNumber() + 1);

	// Positionnement du flag d'erreur
	bIsError = bIsError or mapping->GetDataTableDriver()->IsError();

	// Parcours des mappings de la composition  pour completer l'ecriture de l'objet, meme si les cle
	// ne sont pas chargee en memoire
	for (i = 0; i < mapping->GetComponentTableMappings()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponentTableMappings()->GetAt(i));

		// Ecriture si necessaire
		mappedDataTableDriver = componentMapping->GetDataTableDriver();
		if (mappedDataTableDriver != NULL and mappedDataTableDriver->GetDataTableName() != "" and
		    mappedDataTableDriver->IsOpenedForWrite())
		{
			// Ecriture de l'attribut mappe
			if (componentMapping->GetMappedAttributeType() == KWType::Object)
			{
				kwoSubObject =
				    kwoObject->GetObjectValueAt(componentMapping->GetMappedAttributeLoadIndex());
				// Propagation au reste de la composition par appel recursif a DMTMPhysicalWrite
				if (kwoSubObject != NULL)
					DMTMPhysicalWrite(componentMapping, kwoSubObject);
			}
			else if (componentMapping->GetMappedAttributeType() == KWType::ObjectArray)
			{
				oaSubObjectArray =
				    kwoObject->GetObjectArrayValueAt(componentMapping->GetMappedAttributeLoadIndex());
				if (oaSubObjectArray != NULL)
				{
					// Ecriture des sous-objet du tableau
					for (nObject = 0; nObject < oaSubObjectArray->GetSize(); nObject++)
					{
						kwoSubObject = cast(KWObject*, oaSubObjectArray->GetAt(nObject));
						// Propagation au reste de la composition par appel recursif a
						// DMTMPhysicalWrite
						if (kwoSubObject != NULL)
							DMTMPhysicalWrite(componentMapping, kwoSubObject);
					}
				}
			}
		}
	}
}

void KWMTDatabase::WriteMapingArray(ostream& ost, const ALString& sTitle, const ObjectArray* oaMappings) const
{
	int n;
	KWMTDatabaseMapping* mapping;

	require(sTitle != "");
	require(oaMappings != NULL);

	// Affichage
	ost << sTitle << "\n";
	for (n = 0; n < oaMappings->GetSize(); n++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMappings->GetAt(n));
		ost << "\t" << n + 1 << "\t" << mapping->GetDataPath() << "\t" << mapping->GetClassName() << "\t"
		    << mapping->GetDataTableName() << "\n";
	}
}
