// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataPath.h"

////////////////////////////////////////////////////////////
// Classe KWDataPath

KWDataPath::KWDataPath()
{
	bExternalTable = false;
}

KWDataPath::~KWDataPath() {}

void KWDataPath::CopyFrom(const KWDataPath* aSource)
{
	require(aSource != NULL);

	bExternalTable = aSource->bExternalTable;
	sOriginClassName = aSource->sOriginClassName;
	svAttributeNames.CopyFrom(&aSource->svAttributeNames);
	sClassName = aSource->sClassName;
}

KWDataPath* KWDataPath::Create() const
{
	return new KWDataPath;
}

KWDataPath* KWDataPath::Clone() const
{
	KWDataPath* aClone;

	aClone = Create();
	aClone->CopyFrom(this);
	return aClone;
}

int KWDataPath::Compare(const KWDataPath* aSource) const
{
	int nCompare = 0;
	int i;

	// Comparaison terme a terme
	if (nCompare == 0)
		nCompare = GetExternalTable() == aSource->GetExternalTable();
	if (nCompare == 0)
		nCompare = GetOriginClassName().Compare(aSource->GetOriginClassName());
	if (nCompare == 0)
	{
		// Comparaison des attributs du data path
		nCompare = GetDataPathAttributeNumber() == aSource->GetDataPathAttributeNumber();
		if (nCompare == 0)
		{
			for (i = 0; i < GetDataPathAttributeNumber(); i++)
			{
				nCompare = GetDataPathAttributeNameAt(i) == aSource->GetDataPathAttributeNameAt(i);
				if (nCompare != 0)
					break;
			}
		}
	}
	if (nCompare == 0)
		nCompare = GetClassName().Compare(aSource->GetClassName());
	return nCompare;
}

////////////////////////////////////////////////////////////

ALString KWDataPath::GetDataPath() const
{
	// Cas d'un table externe
	if (GetExternalTable())
	{
		if (GetDataPathAttributeNumber() == 0)
			return GetDataPathSeparator() + GetFormattedName(GetOriginClassName());
		else
			return GetDataPathSeparator() + GetFormattedName(GetOriginClassName()) +
			       GetDataPathSeparator() + GetDataPathAttributeNames();
	}
	// Cas standard
	else
		return GetDataPathAttributeNames();
}

ALString KWDataPath::GetDataPathAttributeNames() const
{
	ALString sDataPathAttributeNames;
	int i;

	// Partie du data path lie au attributs
	for (i = 0; i < GetDataPathAttributeNumber(); i++)
	{
		if (i > 0)
			sDataPathAttributeNames += GetDataPathSeparator();
		sDataPathAttributeNames += GetFormattedName(GetDataPathAttributeNameAt(i));
	}
	return sDataPathAttributeNames;
}

boolean KWDataPath::IsTerminalAttributeUsed() const
{

	boolean bIsTerminalAttributeUsed = true;
	KWClass* kwcCurrentClass;
	KWAttribute* currentAttribute;
	int n;

	require(CheckDataPath());

	// Recherche de la classe de depart
	kwcCurrentClass = KWClassDomain::GetCurrentDomain()->LookupClass(sOriginClassName);
	check(kwcCurrentClass);

	// Parcours des attributs du data path
	for (n = 0; n < GetDataPathAttributeNumber(); n++)
	{
		// Recherche de l'attribut
		currentAttribute = kwcCurrentClass->LookupAttribute(GetDataPathAttributeNameAt(n));
		check(currentAttribute);
		assert(KWType::IsRelation(currentAttribute->GetType()));

		// Arret si l'attribut n'est pas en Used
		if (not currentAttribute->GetUsed())
		{
			bIsTerminalAttributeUsed = false;
			break;
		}

		// Changement de classe courante
		kwcCurrentClass = currentAttribute->GetClass();
	}
	return bIsTerminalAttributeUsed;
}

boolean KWDataPath::CheckDataPath() const
{
	boolean bOk = true;
	KWClass* kwcCurrentClass;
	KWAttribute* currentAttribute;
	int n;
	ALString sTmp;

	// Le DataPathClassName doit etre non vide
	if (sOriginClassName.GetLength() == 0)
	{
		AddError("Empty origin dictionary in data path");
		bOk = false;
	}

	// Recherche de la classe de depart
	kwcCurrentClass = NULL;
	if (bOk)
	{
		kwcCurrentClass = KWClassDomain::GetCurrentDomain()->LookupClass(sOriginClassName);
		if (kwcCurrentClass == NULL)
		{
			AddError("Unknown origin dictionary " + sOriginClassName + " in data path");
			bOk = false;
		}
	}

	// Parcours des attributs du data path
	if (bOk)
	{
		for (n = 0; n < GetDataPathAttributeNumber(); n++)
		{
			// Recherche de l'attribut
			currentAttribute = kwcCurrentClass->LookupAttribute(GetDataPathAttributeNameAt(n));

			// Erreur si attribut inexistant
			if (currentAttribute == NULL)
			{
				AddError(sTmp + "Unknown variable " + GetDataPathAttributeNameAt(n) + " at index " +
					 IntToString(n + 1) + " in data path");
				bOk = false;
			}
			// Erreur si attribut n'est pas de type relation
			else if (not KWType::IsRelation(currentAttribute->GetType()))
			{
				AddError(sTmp + "Variable " + GetDataPathAttributeNameAt(n) + " at index " +
					 IntToString(n + 1) + " in data path should of relational data type");
				bOk = false;
			}
			// Erreur si attribut calcule
			//DDD else if (currentAttribute->GetDerivationRule() != NULL)
			else if (currentAttribute->GetDerivationRule() != NULL and
				 currentAttribute->GetDerivationRule()->GetReference())
			{
				AddError(sTmp + "Variable " + GetDataPathAttributeNameAt(n) + " at index " +
					 IntToString(n + 1) + " in data path should be native");
				bOk = false;
			}

			// Arret si erreur
			if (not bOk)
				break;

			// Changement de classe courante
			kwcCurrentClass = currentAttribute->GetClass();
		}
	}
	return bOk;
}

ALString KWDataPath::GetFormattedName(const ALString& sValue)
{
	ALString sResult;
	int nLength;
	char c;
	int i;

	// Les valeurs comportant les caracteres speciaux sont entoure de back-quotes,
	// en doublant les eventuels back-quotes internes
	if (sValue.Find(GetDataPathSeparator()) != -1 or sValue.Find(GetDataPathEscapeChar()) != -1)
	{
		sResult = '`';
		nLength = sValue.GetLength();
		for (i = 0; i < nLength; i++)
		{
			c = sValue.GetAt(i);
			if (c == '`')
				sResult += '`';
			sResult += c;
		}
		sResult += '`';
		return sResult;
	}
	// Sinon, on garde le nom tel quel
	else
		return sValue;
}

void KWDataPath::Write(ostream& ost) const
{
	int n;

	ost << "Data path\t" << GetDataPath() << "\n";
	ost << "External table\t" << BooleanToString(GetExternalTable()) << "\n";
	ost << "Data path origin dictionary\t" << GetOriginClassName() << "\n";
	ost << "Data path variables\t" << GetDataPathAttributeNumber() << "\n";
	for (n = 0; n < GetDataPathAttributeNumber(); n++)
		ost << "\t" << GetDataPathAttributeNameAt(n) << "\n";
	ost << "Dictionary\t" << GetClassName() << "\n";
}

const ALString KWDataPath::GetClassLabel() const
{
	return "Data path";
}

const ALString KWDataPath::GetObjectLabel() const
{
	return GetDataPath();
}

longint KWDataPath::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDataPath);
	lUsedMemory += sOriginClassName.GetLength();
	lUsedMemory += svAttributeNames.GetUsedMemory();
	lUsedMemory += sClassName.GetLength();
	return lUsedMemory;
}

////////////////////////////////////////////////////////////
// Classe KWObjectDataPath

KWObjectDataPath::KWObjectDataPath()
{
	lCreationNumber = 0;
	dataPathManager = NULL;
	nCompiledRandomSeed = 0;
	nCompiledRandomLeap = 0;
	nCompileHash = 0;
}

KWObjectDataPath::~KWObjectDataPath() {}

void KWObjectDataPath::CopyFrom(const KWDataPath* aSource)
{
	const KWObjectDataPath* sourceObjectDataPath;

	require(aSource != NULL);

	// Methode ancetre
	KWDataPath::CopyFrom(aSource);

	// Specialisation
	sourceObjectDataPath = cast(const KWObjectDataPath*, aSource);
	lCreationNumber = sourceObjectDataPath->lCreationNumber;
	oaSubDataPaths.CopyFrom(&sourceObjectDataPath->oaSubDataPaths);
	dataPathManager = sourceObjectDataPath->dataPathManager;
	liCompiledTerminalAttributeLoadIndex.lFullIndex =
	    sourceObjectDataPath->liCompiledTerminalAttributeLoadIndex.lFullIndex;
	nCompiledRandomSeed = sourceObjectDataPath->nCompiledRandomSeed;
	nCompiledRandomLeap = sourceObjectDataPath->nCompiledRandomLeap;
	oaCompiledSubDataPathsByLoadIndex.CopyFrom(&sourceObjectDataPath->oaCompiledSubDataPathsByLoadIndex);
	nCompileHash = sourceObjectDataPath->nCompileHash;
}

KWDataPath* KWObjectDataPath::Create() const
{
	return new KWObjectDataPath;
}

longint KWObjectDataPath::GetUsedMemory() const
{
	longint lUsedMemory;

	// Methode ancetre
	lUsedMemory = KWDataPath::GetUsedMemory();
	lUsedMemory += sizeof(KWObjectDataPath) - sizeof(KWDataPath);

	// Specialisation
	lUsedMemory += oaSubDataPaths.GetUsedMemory();
	lUsedMemory += oaCompiledSubDataPathsByLoadIndex.GetUsedMemory();
	return lUsedMemory;
}

void KWObjectDataPath::Compile(const KWClass* mainClass)
{
	KWClass* kwcOriginClass;
	KWClass* currentClass;
	KWAttribute* currentAttribute;
	int n;
	KWObjectDataPath* subDataPath;
	ALString sSeedEncoding;
	ALString sLeapEncoding;
	ALString sTmp;

	require(mainClass != NULL);
	require(mainClass->IsCompiled());
	require(CheckDataPath());

	// Arret si deja compile
	if (IsCompiled())
		return;

	// Recherche de la classe origine
	kwcOriginClass = mainClass->GetDomain()->LookupClass(sOriginClassName);
	assert(kwcOriginClass != NULL);
	assert(kwcOriginClass == mainClass or kwcOriginClass->GetName() != mainClass->GetName());

	// Recherche de l'index de chargement du dernier attribut du data path et de sa class extremite
	liCompiledTerminalAttributeLoadIndex.Reset();
	currentClass = kwcOriginClass;
	for (n = 0; n < GetDataPathAttributeNumber(); n++)
	{
		// Recherche de l'attribut
		currentAttribute = currentClass->LookupAttribute(GetDataPathAttributeNameAt(n));
		assert(currentAttribute != NULL);
		assert(currentAttribute->GetLoaded());
		assert(currentAttribute->GetLoadIndex().IsDense());
		assert(KWType::IsRelation(currentAttribute->GetType()));
		assert(currentAttribute->GetDerivationRule() == NULL or
		       not currentAttribute->GetDerivationRule()->GetReference());

		// Memorisation de son index de chargement
		liCompiledTerminalAttributeLoadIndex.lFullIndex = currentAttribute->GetLoadIndex().lFullIndex;

		// Changement de classe courante
		currentClass = currentAttribute->GetClass();
	}

	// Compilation des sous data paths, por acceder a leurs services
	for (n = 0; n < oaSubDataPaths.GetSize(); n++)
	{
		subDataPath = cast(KWObjectDataPath*, oaSubDataPaths.GetAt(n));
		subDataPath->Compile(mainClass);
	}

	// Utilisation de la partie dense des index de chargement des attributs pour un acces direct aux sous data path
	// Cet index ne peut depasser le nombre d'attributs charges en memoire de la classe terminale du data path
	oaCompiledSubDataPathsByLoadIndex.RemoveAll();
	if (oaSubDataPaths.GetSize() > 0)
	{
		// Acces au dernier sous data path, pour avoir la valeur max d'index
		subDataPath = cast(KWObjectDataPath*, oaSubDataPaths.GetAt(oaSubDataPaths.GetSize() - 1));

		// Retaillage du tableau compile
		oaCompiledSubDataPathsByLoadIndex.SetSize(subDataPath->GetTerminalLoadIndex().GetDenseIndex() + 1);

		// Memorisation de chaque sous data path selon son index dense
		for (n = 0; n < oaSubDataPaths.GetSize(); n++)
		{
			subDataPath = cast(KWObjectDataPath*, oaSubDataPaths.GetAt(n));
			oaCompiledSubDataPathsByLoadIndex.SetAt(subDataPath->GetTerminalLoadIndex().GetDenseIndex(),
								subDataPath);
		}
	}

	// Initialisation des parametres de generateur aleatoire par hashage de chaines de caractere
	// dependant du nom de la classe origine et du data path
	sSeedEncoding = sTmp + "Seed" + kwcOriginClass->GetName() + "///" + GetDataPath() + "Seed";
	sLeapEncoding = sTmp + "Leap" + kwcOriginClass->GetName() + "///" + GetDataPath() + "Leap";
	sLeapEncoding.MakeReverse();
	nCompiledRandomSeed = HashValue(sSeedEncoding);
	nCompiledRandomLeap = HashValue(sLeapEncoding);

	// On interdit un Leap de 0
	n = 0;
	while (nCompiledRandomLeap == 0)
	{
		sLeapEncoding += "Leap";
		sLeapEncoding += +IntToString(n);
		nCompiledRandomLeap = HashValue(sLeapEncoding);
		n++;
	}

	// Calcul de hash de compilation
	nCompileHash = HashValue(GetDataPath());

	ensure(IsCompiled());
}

boolean KWObjectDataPath::IsCompiled() const
{
	return nCompiledRandomLeap != 0 and nCompileHash == HashValue(GetDataPath());
}

const KWObjectDataPathManager* KWObjectDataPath::GetDataPathManager() const
{
	return dataPathManager;
}

void KWObjectDataPath::SetDataPathManager(const KWObjectDataPathManager* manager)
{
	dataPathManager = manager;
}

////////////////////////////////////////////////////////////

KWObjectDataPathManager::KWObjectDataPathManager()
{
	kwcMainClass = NULL;
	mainDataPath = NULL;
}

KWObjectDataPathManager::~KWObjectDataPathManager()
{
	oaDataPaths.DeleteAll();
}

// Fonction de comparaison sur le nom de la premiere classe (principale) d'un table de mapping
// Permet d'avoir les mappings tries selon leur classe principale
int KWObjectDataPathManagerCompareDataPathMainClass(const void* first, const void* second)
{
	ObjectArray* aFirst;
	ObjectArray* aSecond;
	KWObjectDataPath* firstDataPath;
	KWObjectDataPath* secondDataPath;
	int nResult;

	aFirst = cast(ObjectArray*, *(Object**)first);
	aSecond = cast(ObjectArray*, *(Object**)second);
	firstDataPath = cast(KWObjectDataPath*, aFirst->GetAt(0));
	secondDataPath = cast(KWObjectDataPath*, aSecond->GetAt(0));
	nResult = firstDataPath->GetClassName().Compare(secondDataPath->GetClassName());
	return nResult;
}

void KWObjectDataPathManager::ComputeAllDataPaths(const KWClass* mainClass)
{
	const boolean bTrace = false;
	KWObjectDataPath* dataPath;
	StringVector svAttributeName;
	ObjectDictionary odReferenceClasses;
	ObjectArray oaRankedReferenceClasses;
	ObjectDictionary odAnalysedCreatedClasses;
	KWClass* referenceClass;
	ObjectDictionary odWorkingReferenceClasses;
	ObjectArray oaWorkingRankedReferenceClasses;
	ObjectDictionary odWorkingAnalysedCreatedClasses;
	ObjectArray oaWorkingCreatedDataPaths;
	boolean bIsRootDictionaryUsable;
	ObjectArray oaAllRootCreatedDataPaths;
	ObjectArray* oaCreatedDataPaths;
	int i;
	int j;

	require(mainClass != NULL);
	require(mainClass->IsCompiled());

	// Nettoyage initial
	Reset();

	// Creation du data path principal
	kwcMainClass = mainClass;
	assert(svAttributeName.GetSize() == 0);
	mainDataPath = CreateDataPath(&odReferenceClasses, &oaRankedReferenceClasses, &odAnalysedCreatedClasses,
				      mainClass, false, mainClass->GetName(), &svAttributeName, &oaDataPaths);
	assert(svAttributeName.GetSize() == 0);

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
			oaWorkingCreatedDataPaths.RemoveAll();
			assert(svAttributeName.GetSize() == 0);
			dataPath =
			    CreateDataPath(&odWorkingReferenceClasses, &oaWorkingRankedReferenceClasses,
					   &odWorkingAnalysedCreatedClasses, referenceClass, true,
					   referenceClass->GetName(), &svAttributeName, &oaWorkingCreatedDataPaths);
			assert(svAttributeName.GetSize() == 0);

			// On determine si le dictionnaire de reference est utilisable, c'est a dire s'il n'utilise
			// pas la classe principale dans ses mappings
			bIsRootDictionaryUsable = true;
			for (j = 0; j < oaWorkingCreatedDataPaths.GetSize(); j++)
			{
				dataPath = cast(KWObjectDataPath*, oaWorkingCreatedDataPaths.GetAt(j));

				// Transfert des specifications de la table mappee si comparaison positive
				if (dataPath->GetClassName() == mainClass->GetName())
				{
					bIsRootDictionaryUsable = false;
					break;
				}
			}
			oaWorkingCreatedDataPaths.DeleteAll();

			// Prise en compte de la classe referencee si elle est utilisable
			if (bIsRootDictionaryUsable)
			{
				// Creation et memorisation d'un tableau de mapping, pour accueillir les mappings
				// pour la classe externe en cours de traitement
				oaCreatedDataPaths = new ObjectArray;
				oaAllRootCreatedDataPaths.Add(oaCreatedDataPaths);

				// Creation du mapping et memorisation de tous les mappings des sous-classes
				// Il est plus simple de rappeler la meme methode avec les container globaux
				// que de fusionner les containers de travail
				// Et il n'y a aucun enjeu d'optimisation
				assert(svAttributeName.GetSize() == 0);
				dataPath =
				    CreateDataPath(&odReferenceClasses, &oaRankedReferenceClasses,
						   &odAnalysedCreatedClasses, referenceClass, true,
						   referenceClass->GetName(), &svAttributeName, oaCreatedDataPaths);
				assert(svAttributeName.GetSize() == 0);
				/*DDD
				if (bTrace)
					WriteMapingArray(cout, "- external mappings " + referenceClass->GetName(),
							 oaCreatedMappings);
							 */
			}
			// Sinon, memorisation d'un warning expliquant pourquoi on ne garde le dictionnaire racine en reference
			else
			{
				/*DDD
				sWarning = "Root dictionary " + referenceClass->GetName() +
					   " referenced from the main dictionary " + mainClass->GetName() +
					   " is not kept to specify database files in order to avoid circular "
					   "references in the schema, as it itself uses the main dictionary in its "
					   "secondary tables";
				svUnusedRootDictionaryWarnings.Add(sWarning);
				if (bTrace)
					cout << "- " << sWarning << "\n";
					*/
			}
		}

		// Passage a la classe suivante
		i++;
	}

	// Tri des tableau de mappings de classes references selon leur classe principal
	oaAllRootCreatedDataPaths.SetCompareFunction(KWObjectDataPathManagerCompareDataPathMainClass);
	oaAllRootCreatedDataPaths.Sort();

	// Memorisation des mapping des classes externes dans l'ordre du tri
	for (i = 0; i < oaAllRootCreatedDataPaths.GetSize(); i++)
	{
		// Memorisation des mappings
		oaCreatedDataPaths = cast(ObjectArray*, oaAllRootCreatedDataPaths.GetAt(i));
		assert(oaCreatedDataPaths->GetSize() > 0);
		oaDataPaths.InsertObjectArrayAt(oaDataPaths.GetSize(), oaCreatedDataPaths);

		// Memorisation de la classe referencee principale
		dataPath = cast(KWObjectDataPath*, oaCreatedDataPaths->GetAt(0));
		oaExternalRootDataPaths.Add(dataPath);
	}
	oaAllRootCreatedDataPaths.DeleteAll();

	//DDD
	// Optimisation des data paths
	for (i = 0; i < oaDataPaths.GetSize(); i++)
	{
		dataPath = cast(KWObjectDataPath*, oaDataPaths.GetAt(i));

		// Compilation
		dataPath->Compile(mainClass);

		// Memorisation dans un dictionnaire
		odDataPaths.SetAt(dataPath->GetDataPath(), dataPath);
	}

	// Trace
	if (bTrace)
		cout << *this << endl;
}

void KWObjectDataPathManager::Reset()
{
	kwcMainClass = NULL;
	mainDataPath = NULL;
	oaDataPaths.DeleteAll();
	oaExternalRootDataPaths.RemoveAll();
	odDataPaths.RemoveAll();
}

const KWClass* KWObjectDataPathManager::GetMainClass()
{
	return kwcMainClass;
}

int KWObjectDataPathManager::GetDataPathNumber() const
{
	return oaDataPaths.GetSize();
}

const KWObjectDataPath* KWObjectDataPathManager::GetDataPathAt(int nIndex) const
{
	return cast(const KWObjectDataPath*, oaDataPaths.GetAt(nIndex));
}

const KWObjectDataPath* KWObjectDataPathManager::GetMainDataPath() const
{
	return mainDataPath;
}

int KWObjectDataPathManager::GetExternalRootDataPathNumber() const
{
	return oaExternalRootDataPaths.GetSize();
}

const KWObjectDataPath* KWObjectDataPathManager::GetExternalRootDataPathAt(int nIndex) const
{
	return cast(const KWObjectDataPath*, oaExternalRootDataPaths.GetAt(nIndex));
}

const KWObjectDataPath* KWObjectDataPathManager::LookupDataPath(const ALString& sDataPath) const
{
	return cast(KWObjectDataPath*, odDataPaths.Lookup(sDataPath));
}

void KWObjectDataPathManager::Write(ostream& ost) const
{
	int i;

	cout << GetClassLabel() << "\t" << GetObjectLabel() << "\n";
	for (i = 0; i < GetDataPathNumber(); i++)
	{
		cout << "- " << GetDataPathAt(i)->GetDataPath() << "\n";
	}
}

const ALString KWObjectDataPathManager::GetClassLabel() const
{
	return "Data path manager";
}

const ALString KWObjectDataPathManager::GetObjectLabel() const
{
	if (kwcMainClass == NULL)
		return "";
	else
		return kwcMainClass->GetName();
}

longint KWObjectDataPathManager::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWObjectDataPathManager);
	lUsedMemory += oaDataPaths.GetOverallUsedMemory();
	lUsedMemory += oaExternalRootDataPaths.GetUsedMemory();
	return lUsedMemory;
}

KWObjectDataPath*
KWObjectDataPathManager::CreateDataPath(ObjectDictionary* odReferenceClasses, ObjectArray* oaRankedReferenceClasses,
					ObjectDictionary* odAnalysedCreatedClasses, const KWClass* mappedClass,
					boolean bIsExternalTable, const ALString& sOriginClassName,
					StringVector* svAttributeNames, ObjectArray* oaCreatedDataPaths)
{
	KWObjectDataPath* dataPath;
	KWObjectDataPath* subDataPath;
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
	require(oaCreatedDataPaths != NULL);

	// Creation et initialisation d'un dataPath
	dataPath = new KWObjectDataPath;
	dataPath->SetExternalTable(bIsExternalTable);
	dataPath->SetClassName(mappedClass->GetName());
	dataPath->SetOriginClassName(sOriginClassName);
	dataPath->GetAttributeNames()->CopyFrom(svAttributeNames);
	assert(LookupDataPath(dataPath->GetDataPath()) == NULL);

	// Memorisation de ce dataPath dans le tableau exhaustif de tous les dataPath
	oaCreatedDataPaths->Add(dataPath);

	// Ajout des dataPath pour la composition de la classe ainsi que les classe referencees
	attribute = mappedClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Test si attribut natif est de type Object ou ObjectArray
		if (KWType::IsRelation(attribute->GetType()) and attribute->GetClass() != NULL)
		{
			// Cas d'un attribut natif de la composition (sans regle de derivation)
			//DDD if (attribute->GetAnyDerivationRule() == NULL)
			if (attribute->GetAnyDerivationRule() == NULL or
			    not attribute->GetAnyDerivationRule()->GetReference())
			{
				// Ajout temporaire d'un attribut au dataPath
				svAttributeNames->Add(attribute->GetName());

				// Creation du dataPath dans une nouvelle table de dataPath temporaire
				subDataPath =
				    CreateDataPath(odReferenceClasses, oaRankedReferenceClasses,
						   odAnalysedCreatedClasses, attribute->GetClass(), bIsExternalTable,
						   sOriginClassName, svAttributeNames, oaCreatedDataPaths);

				// Supression de l'attribut ajoute temporairement
				svAttributeNames->SetSize(svAttributeNames->GetSize() - 1);

				// Chainage du sous-dataPath
				dataPath->oaSubDataPaths.Add(subDataPath);
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
				// Memorisation du dataPath a traiter
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
	return dataPath;
}
