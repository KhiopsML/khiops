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

boolean KWDataPath::IsRuleCreationManaged() const
{
	return false;
}

boolean KWDataPath::GetCreatedObjects() const
{
	return false;
}

void KWDataPath::SetCreatedObjects(boolean bValue)
{
	assert(false);
}

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
			// Erreur eventuelle si attribut calcule
			else if (currentAttribute->GetDerivationRule() != NULL)
			{
				// Erreur si les regles de creation d'instances ne sont pas autorisees
				if (not IsRuleCreationManaged())
				{
					AddError(sTmp + "Variable " + GetDataPathAttributeNameAt(n) + " at index " +
						 IntToString(n + 1) + " in data path should be native");
					bOk = false;
				}
				// Sinon, erreur si la regle est une regle de reference, qui ne cree pas d'instances
				else if (currentAttribute->GetDerivationRule()->GetReference())
				{
					AddError(sTmp + "Variable " + GetDataPathAttributeNameAt(n) + " at index " +
						 IntToString(n + 1) + " in data path exploits derivation rule " +
						 currentAttribute->GetDerivationRule()->GetName() +
						 " that does not create isntances");
					bOk = false;
				}
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

void KWDataPath::CollectFullHierarchyComponents(ObjectArray* oaResults)
{
	int i;
	KWDataPath* dataPath;

	require(oaResults != NULL);

	// Ajout du mapping
	oaResults->Add(this);

	// Ajout des mapping de la composition
	for (i = 0; i < oaComponents.GetSize(); i++)
	{
		dataPath = cast(KWDataPath*, oaComponents.GetAt(i));
		dataPath->CollectFullHierarchyComponents(oaResults);
	}
}

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
	int nCompare;
	int i;

	require(aSource != NULL);

	// Comparaison terme a terme
	nCompare = GetClassName().Compare(aSource->GetClassName());
	if (nCompare == 0)
		nCompare = CompareBoolean(GetExternalTable(), aSource->GetExternalTable());
	if (nCompare == 0)
		nCompare = GetOriginClassName().Compare(aSource->GetOriginClassName());
	if (nCompare == 0)
	{
		// Comparaison des attributs du data path
		nCompare = GetDataPathAttributeNumber() - aSource->GetDataPathAttributeNumber();
		if (nCompare == 0)
		{
			for (i = 0; i < GetDataPathAttributeNumber(); i++)
			{
				nCompare =
				    GetDataPathAttributeNameAt(i).Compare(aSource->GetDataPathAttributeNameAt(i));
				if (nCompare != 0)
					break;
			}
		}
	}
	return nCompare;
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

void KWDataPath::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Data path\tDictionary";
}

void KWDataPath::WriteLineReport(ostream& ost) const
{
	ost << GetDataPath() << "\t" << GetClassName();
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
	lUsedMemory += oaComponents.GetUsedMemory();
	return lUsedMemory;
}

////////////////////////////////////////////////////////////
// Classe KWObjectDataPath

KWObjectDataPath::KWObjectDataPath()
{
	bCreatedObjects = false;
	lMainCreationIndex = 0;
	lCreationNumber = 0;
	dataPathManager = NULL;
	nCompiledRandomSeed = 0;
	nCompiledRandomLeap = 0;
	nCompileHash = 0;
}

KWObjectDataPath::~KWObjectDataPath() {}

boolean KWObjectDataPath::IsRuleCreationManaged() const
{
	return true;
}

void KWObjectDataPath::ResetCreationNumber(longint lNewMainCreationIndex) const
{
	int n;
	KWObjectDataPath* componentDataPath;

	require(IsCompiled());
	require(lNewMainCreationIndex >= 0);

	// Reinitialisation des index
	lMainCreationIndex = lNewMainCreationIndex;
	lCreationNumber = 0;

	// Propagation aux sous data paths
	for (n = 0; n < oaComponents.GetSize(); n++)
	{
		componentDataPath = cast(KWObjectDataPath*, oaComponents.GetAt(n));
		componentDataPath->ResetCreationNumber(lMainCreationIndex);
	}
}

void KWObjectDataPath::CopyFrom(const KWDataPath* aSource)
{
	const KWObjectDataPath* sourceObjectDataPath;

	require(aSource != NULL);

	// Methode ancetre
	KWDataPath::CopyFrom(aSource);

	// Specialisation
	sourceObjectDataPath = cast(const KWObjectDataPath*, aSource);
	bCreatedObjects = sourceObjectDataPath->bCreatedObjects;

	// Nettoyage des attributs de composition ou de gestion
	lCreationNumber = 0;
	oaComponents.SetSize(0);
	dataPathManager = NULL;
	liCompiledTerminalAttributeLoadIndex.Reset();
	nCompiledRandomSeed = 0;
	nCompiledRandomLeap = 0;
	oaCompiledComponentDataPathsByLoadIndex.SetSize(0);
	nCompileHash = 0;
}

KWDataPath* KWObjectDataPath::Create() const
{
	return new KWObjectDataPath;
}

int KWObjectDataPath::Compare(const KWDataPath* aSource) const
{
	int nCompare;

	require(aSource != NULL);

	// Methode ancetre
	nCompare = KWDataPath::Compare(aSource);

	// Specialisation
	if (nCompare == 0)
		nCompare =
		    CompareBoolean(GetCreatedObjects(), cast(const KWObjectDataPath*, aSource)->GetCreatedObjects());
	return nCompare;
}

void KWObjectDataPath::Write(ostream& ost) const
{
	KWDataPath::Write(ost);
	ost << "Created objects\t" << BooleanToString(GetCreatedObjects()) << "\n";
}

void KWObjectDataPath::WriteHeaderLineReport(ostream& ost) const
{
	KWDataPath::WriteHeaderLineReport(ost);
	ost << "\tCreated";
}

void KWObjectDataPath::WriteLineReport(ostream& ost) const
{
	KWDataPath::WriteLineReport(ost);
	ost << "\t";
	if (GetCreatedObjects())
		ost << "*";
}

longint KWObjectDataPath::GetUsedMemory() const
{
	longint lUsedMemory;

	// Methode ancetre
	lUsedMemory = KWDataPath::GetUsedMemory();
	lUsedMemory += sizeof(KWObjectDataPath) - sizeof(KWDataPath);

	// Specialisation
	lUsedMemory += oaComponents.GetUsedMemory();
	lUsedMemory += oaCompiledComponentDataPathsByLoadIndex.GetUsedMemory();
	return lUsedMemory;
}

void KWObjectDataPath::Compile(const KWClass* mainClass)
{
	KWClass* kwcOriginClass;
	KWClass* currentClass;
	KWAttribute* currentAttribute;
	int n;
	KWObjectDataPath* componentDataPath;
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
	for (n = 0; n < oaComponents.GetSize(); n++)
	{
		componentDataPath = cast(KWObjectDataPath*, oaComponents.GetAt(n));
		componentDataPath->Compile(mainClass);
	}

	// Utilisation de la partie dense des index de chargement des attributs pour un acces direct aux sous data path
	// Cet index ne peut depasser le nombre d'attributs charges en memoire de la classe terminale du data path
	oaCompiledComponentDataPathsByLoadIndex.RemoveAll();
	if (oaComponents.GetSize() > 0)
	{
		// Acces au dernier sous data path, pour avoir la valeur max d'index
		componentDataPath = cast(KWObjectDataPath*, oaComponents.GetAt(oaComponents.GetSize() - 1));

		// Retaillage du tableau compile
		oaCompiledComponentDataPathsByLoadIndex.SetSize(
		    componentDataPath->GetTerminalLoadIndex().GetDenseIndex() + 1);

		// Memorisation de chaque sous data path selon son index dense
		for (n = 0; n < oaComponents.GetSize(); n++)
		{
			componentDataPath = cast(KWObjectDataPath*, oaComponents.GetAt(n));
			oaCompiledComponentDataPathsByLoadIndex.SetAt(
			    componentDataPath->GetTerminalLoadIndex().GetDenseIndex(), componentDataPath);
		}
	}

	// Initialisation des parametres de generateur aleatoire par hashage de chaines de caractere
	// dependant du nom de la classe origine et du data path
	sSeedEncoding = sTmp + "DataPathSeed" + kwcOriginClass->GetName() + "///" + GetDataPath() + "DataPathSeed";
	sLeapEncoding = sTmp + "DataPathLeap" + kwcOriginClass->GetName() + "///" + GetDataPath() + "DataPathLeap";
	sLeapEncoding.MakeReverse();
	nCompiledRandomSeed = HashValue(sSeedEncoding);
	nCompiledRandomLeap = HashValue(sLeapEncoding);

	// On interdit un Leap de 0
	n = 0;
	while (nCompiledRandomLeap == 0)
	{
		sLeapEncoding += "_";
		sLeapEncoding += IntToString(n);
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

KWDataPathManager::KWDataPathManager()
{
	kwcMainClass = NULL;
	mainDataPath = NULL;
	dataPathCreator = new KWDataPath;
}

KWDataPathManager::~KWDataPathManager()
{
	oaDataPaths.DeleteAll();
	delete dataPathCreator;
}

boolean KWDataPathManager::IsRuleCreationManaged() const
{
	return dataPathCreator->IsRuleCreationManaged();
}

// Fonction de comparaison sur le nom de la premiere classe (principale) d'un table de mapping
// Permet d'avoir les mappings tries selon leur classe principale
int KWDataPathManagerCompareDataPathMainClass(const void* first, const void* second)
{
	ObjectArray* aFirst;
	ObjectArray* aSecond;
	KWDataPath* firstDataPath;
	KWDataPath* secondDataPath;
	int nResult;

	aFirst = cast(ObjectArray*, *(Object**)first);
	aSecond = cast(ObjectArray*, *(Object**)second);
	firstDataPath = cast(KWDataPath*, aFirst->GetAt(0));
	secondDataPath = cast(KWDataPath*, aSecond->GetAt(0));
	nResult = firstDataPath->GetClassName().Compare(secondDataPath->GetClassName());
	return nResult;
}

void KWDataPathManager::ComputeAllDataPaths(const KWClass* mainClass)
{
	const boolean bTrace = false;
	KWDataPath* dataPath;
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
	ALString sWarning;
	int i;
	int j;

	require(mainClass != NULL);
	require(mainClass->IsCompiled());

	// Nettoyage initial
	Reset();

	// Creation du data path principal
	// Contexte: table principale, non cree par une regle de derivation
	kwcMainClass = mainClass;
	assert(svAttributeName.GetSize() == 0);
	mainDataPath = CreateDataPath(&odReferenceClasses, &oaRankedReferenceClasses, &odAnalysedCreatedClasses,
				      mainClass, false, false, mainClass->GetName(), &svAttributeName, &oaDataPaths);
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
			// Contexte: table externe, non cree par une regle de derivation
			odWorkingReferenceClasses.RemoveAll();
			oaWorkingRankedReferenceClasses.RemoveAll();
			odWorkingAnalysedCreatedClasses.RemoveAll();
			oaWorkingCreatedDataPaths.RemoveAll();
			assert(svAttributeName.GetSize() == 0);
			dataPath =
			    CreateDataPath(&odWorkingReferenceClasses, &oaWorkingRankedReferenceClasses,
					   &odWorkingAnalysedCreatedClasses, referenceClass, true, false,
					   referenceClass->GetName(), &svAttributeName, &oaWorkingCreatedDataPaths);
			assert(svAttributeName.GetSize() == 0);

			// On determine si le dictionnaire de reference est utilisable, c'est a dire s'il n'utilise
			// pas la classe principale dans ses mappings
			bIsRootDictionaryUsable = true;
			for (j = 0; j < oaWorkingCreatedDataPaths.GetSize(); j++)
			{
				dataPath = cast(KWDataPath*, oaWorkingCreatedDataPaths.GetAt(j));

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
				// Contexte: table externe, non cree par une regle de derivation
				assert(svAttributeName.GetSize() == 0);
				dataPath =
				    CreateDataPath(&odReferenceClasses, &oaRankedReferenceClasses,
						   &odAnalysedCreatedClasses, referenceClass, true, false,
						   referenceClass->GetName(), &svAttributeName, oaCreatedDataPaths);
				assert(svAttributeName.GetSize() == 0);
				if (bTrace)
					WriteDataPathArray(cout, "- external mappings " + referenceClass->GetName(),
							   oaCreatedDataPaths);
			}
			// Sinon, memorisation d'un warning expliquant pourquoi on ne garde le dictionnaire racine en reference
			else
			{
				sWarning = "Root dictionary " + referenceClass->GetName() +
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
	oaAllRootCreatedDataPaths.SetCompareFunction(KWDataPathManagerCompareDataPathMainClass);
	oaAllRootCreatedDataPaths.Sort();

	// Memorisation des mapping des classes externes dans l'ordre du tri
	for (i = 0; i < oaAllRootCreatedDataPaths.GetSize(); i++)
	{
		// Memorisation des mappings
		oaCreatedDataPaths = cast(ObjectArray*, oaAllRootCreatedDataPaths.GetAt(i));
		assert(oaCreatedDataPaths->GetSize() > 0);
		oaDataPaths.InsertObjectArrayAt(oaDataPaths.GetSize(), oaCreatedDataPaths);

		// Memorisation de la classe referencee principale
		dataPath = cast(KWDataPath*, oaCreatedDataPaths->GetAt(0));
		oaExternalRootDataPaths.Add(dataPath);
	}
	oaAllRootCreatedDataPaths.DeleteAll();

	// Memorisation des data paths dans un dictionnaire
	for (i = 0; i < oaDataPaths.GetSize(); i++)
	{
		dataPath = cast(KWObjectDataPath*, oaDataPaths.GetAt(i));
		odDataPaths.SetAt(dataPath->GetDataPath(), dataPath);
	}

	// Trace
	if (bTrace)
		cout << *this << endl;
}

void KWDataPathManager::Reset()
{
	kwcMainClass = NULL;
	mainDataPath = NULL;
	oaDataPaths.DeleteAll();
	oaExternalRootDataPaths.RemoveAll();
	odDataPaths.RemoveAll();
	svUnusedRootDictionaryWarnings.SetSize(0);
}

void KWDataPathManager::CopyFrom(const KWDataPathManager* aSource)
{
	const KWDataPath* dataPath;
	const KWDataPath* dataPathComponent;
	KWDataPath* dataPathCopy;
	int i;
	int j;

	require(aSource != NULL);

	// Recopie du dictionnaire principale
	kwcMainClass = aSource->kwcMainClass;

	// Copie du mapping, apres avoir reinitialise la table de mapping
	oaDataPaths.DeleteAll();
	odDataPaths.RemoveAll();
	for (i = 0; i < aSource->oaDataPaths.GetSize(); i++)
	{
		dataPath = cast(KWDataPath*, aSource->oaDataPaths.GetAt(i));
		dataPathCopy = cast(KWDataPath*, dataPath->Clone());
		oaDataPaths.Add(dataPathCopy);
		odDataPaths.SetAt(dataPathCopy->GetDataPath(), dataPathCopy);
	}

	// Memorisation du mapping principal
	mainDataPath = NULL;
	if (oaDataPaths.GetSize() > 0)
		mainDataPath = cast(KWDataPath*, oaDataPaths.GetAt(0));

	// Memorisation des mappings principaux des classes referencees
	oaExternalRootDataPaths.SetSize(0);
	for (i = 0; i < aSource->oaExternalRootDataPaths.GetSize(); i++)
	{
		dataPath = cast(KWDataPath*, aSource->oaExternalRootDataPaths.GetAt(i));
		assert(dataPath != aSource->mainDataPath);
		assert(dataPath->GetDataPathAttributeNames() == "");

		// Recherche de la copie du mapping source
		dataPathCopy = cast(KWDataPath*, LookupDataPath(dataPath->GetDataPath()));
		assert(dataPathCopy->GetDataPathAttributeNames() == "");
		assert(dataPathCopy->GetDataPath() == dataPath->GetDataPath());

		// Insertion dans le tableau des mappings principaux des classes referencees
		oaExternalRootDataPaths.Add(dataPathCopy);
	}

	// Reconstruction de la structure des mappings, qui connaissent chacun les mapping de leur composition
	for (i = 0; i < aSource->oaDataPaths.GetSize(); i++)
	{
		dataPath = cast(KWDataPath*, aSource->oaDataPaths.GetAt(i));

		// Recherche de la copie du mapping source
		dataPathCopy = cast(KWDataPath*, LookupDataPath(dataPath->GetDataPath()));

		// Reconstitution du tableau des mapping des classes de la composition
		for (j = 0; j < dataPath->GetConstComponents()->GetSize(); j++)
		{
			dataPathComponent = cast(const KWDataPath*, dataPath->GetConstComponents()->GetAt(j));

			// Insertion de la copie du mapping composant correspondant
			dataPathCopy->GetComponents()->Add(
			    cast(KWDataPath*, LookupDataPath(dataPathComponent->GetDataPath())));
		}
		assert(dataPathCopy->GetConstComponents()->GetSize() == dataPath->GetConstComponents()->GetSize());
	}

	// Memorisation des warnings pour les dictionnaires racines non utilises
	svUnusedRootDictionaryWarnings.CopyFrom(&aSource->svUnusedRootDictionaryWarnings);
}

KWDataPathManager* KWDataPathManager::Create() const
{
	return new KWDataPathManager;
}

KWDataPathManager* KWDataPathManager::Clone() const
{
	KWDataPathManager* aClone;

	aClone = Create();
	aClone->CopyFrom(this);
	return aClone;
}

int KWDataPathManager::Compare(const KWDataPathManager* aSource) const
{
	int nCompare = 0;
	KWDataPath* dataPath;
	KWDataPath* sourceDataPath;
	int i;

	require(aSource != NULL);

	// Comparaison de base
	nCompare = CompareBoolean(IsRuleCreationManaged(), aSource->IsRuleCreationManaged());

	// Comparaison du mapping
	if (nCompare == 0)
		nCompare = oaDataPaths.GetSize() - aSource->oaDataPaths.GetSize();
	if (nCompare == 0)
	{
		for (i = 0; i < aSource->oaDataPaths.GetSize(); i++)
		{
			dataPath = cast(KWDataPath*, oaDataPaths.GetAt(i));
			sourceDataPath = cast(KWDataPath*, aSource->oaDataPaths.GetAt(i));

			// Comparaison et arret si difference
			nCompare = dataPath->Compare(sourceDataPath);
			if (nCompare != 0)
				break;
		}
	}

	// Verification de coherence en cas d'egalite
	assert(nCompare != 0 or kwcMainClass == aSource->kwcMainClass);
	assert(nCompare != 0 or (mainDataPath == NULL and aSource->mainDataPath == NULL) or
	       (mainDataPath->Compare(aSource->mainDataPath) == 0));
	assert(nCompare != 0 or oaExternalRootDataPaths.GetSize() == aSource->oaExternalRootDataPaths.GetSize());
	assert(nCompare != 0 or odDataPaths.GetCount() == aSource->odDataPaths.GetCount());
	assert(nCompare != 0 or
	       svUnusedRootDictionaryWarnings.GetSize() == aSource->svUnusedRootDictionaryWarnings.GetSize());
	return nCompare;
}

void KWDataPathManager::Write(ostream& ost) const
{
	WriteDataPathArray(ost, GetClassLabel() + "\t" + GetObjectLabel(), &oaDataPaths);
}

const ALString KWDataPathManager::GetClassLabel() const
{
	return "Data path manager";
}

const ALString KWDataPathManager::GetObjectLabel() const
{
	if (kwcMainClass == NULL)
		return "";
	else
		return kwcMainClass->GetName();
}

longint KWDataPathManager::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDataPathManager);
	lUsedMemory += oaDataPaths.GetOverallUsedMemory();
	lUsedMemory += oaExternalRootDataPaths.GetUsedMemory();
	lUsedMemory += svUnusedRootDictionaryWarnings.GetUsedMemory();
	return lUsedMemory;
}

KWDataPath* KWDataPathManager::CreateDataPath(ObjectDictionary* odReferenceClasses,
					      ObjectArray* oaRankedReferenceClasses,
					      ObjectDictionary* odAnalysedCreatedClasses, const KWClass* mappedClass,
					      boolean bIsExternalTable, boolean bCreatedObjects,
					      const ALString& sOriginClassName, StringVector* svAttributeNames,
					      ObjectArray* oaCreatedDataPaths)
{
	KWDataPath* dataPath;
	KWDataPath* componentDataPath;
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
	dataPath = dataPathCreator->Create();
	dataPath->SetExternalTable(bIsExternalTable);
	dataPath->SetCreatedObjects(bCreatedObjects);
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
			if (attribute->GetAnyDerivationRule() == NULL)
			{
				// L'objet ne doit pas non plus etre cree par une regle de creation d'instances, puisque
				// dans ce cas, les attributs sans regle de l'objet cree, s'ils existent, sont des references
				// a d'autre objets source ayant servi a parametrer la creation de l'objet (et non des attributrs natifs)
				if (not bCreatedObjects)
				{
					// Creation du dataPath dans une nouvelle table de dataPath temporaire
					// Contexte: celui de l'appelant
					svAttributeNames->Add(attribute->GetName());
					componentDataPath = CreateDataPath(
					    odReferenceClasses, oaRankedReferenceClasses, odAnalysedCreatedClasses,
					    attribute->GetClass(), bIsExternalTable, bCreatedObjects, sOriginClassName,
					    svAttributeNames, oaCreatedDataPaths);
					svAttributeNames->SetSize(svAttributeNames->GetSize() - 1);

					// Chainage du sous-dataPath
					dataPath->GetComponents()->Add(componentDataPath);
				}
			}
			// Cas d'un attribut issue d'une regle de creation de table, pour rechercher
			// les classes referencees depuis les tables creees par des regles
			else if (not attribute->GetReference())
			{
				assert(attribute->GetAnyDerivationRule() != NULL);

				// Cas ou on gere les data path de creation d'instance
				if (IsRuleCreationManaged())
				{
					// Creation du dataPath dans une nouvelle table de dataPath temporaire
					// Contexte: celui de l'appelant, mais avec creation par une rege de derivation
					svAttributeNames->Add(attribute->GetName());
					componentDataPath = CreateDataPath(
					    odReferenceClasses, oaRankedReferenceClasses, odAnalysedCreatedClasses,
					    attribute->GetClass(), bIsExternalTable, true, sOriginClassName,
					    svAttributeNames, oaCreatedDataPaths);
					svAttributeNames->SetSize(svAttributeNames->GetSize() - 1);

					// Chainage du sous-dataPath
					dataPath->GetComponents()->Add(componentDataPath);
				}

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

void KWDataPathManager::WriteDataPathArray(ostream& ost, const ALString& sTitle,
					   const ObjectArray* oaDataPathArray) const
{
	int n;
	KWDataPath* dataPath;

	require(sTitle != "");
	require(oaDataPathArray != NULL);

	// Affichage
	ost << sTitle << "\n";
	for (n = 0; n < oaDataPathArray->GetSize(); n++)
	{
		dataPath = cast(KWDataPath*, oaDataPathArray->GetAt(n));

		// Entete
		if (n == 0)
		{
			ost << "Index\t";
			dataPath->WriteHeaderLineReport(ost);
			ost << "\n";
		}

		// Ligne courante
		ost << n + 1 << "\t";
		dataPath->WriteLineReport(ost);
		ost << "\n";
	}
}

////////////////////////////////////////////////////////////
// Classe KWObjectDataPathManager

KWObjectDataPathManager::KWObjectDataPathManager()
{
	// Specifialisation des objet data path crees par la classe
	delete dataPathCreator;
	dataPathCreator = new KWObjectDataPath;
}

KWObjectDataPathManager::~KWObjectDataPathManager() {}

void KWObjectDataPathManager::ComputeAllDataPaths(const KWClass* mainClass)
{
	int i;
	KWObjectDataPath* objectDataPath;

	// Methode ancetre
	KWDataPathManager::ComputeAllDataPaths(mainClass);

	// Specialisation par optimisation des data paths
	for (i = 0; i < oaDataPaths.GetSize(); i++)
	{
		objectDataPath = cast(KWObjectDataPath*, oaDataPaths.GetAt(i));

		// Compilation
		objectDataPath->Compile(mainClass);
	}
}

void KWObjectDataPathManager::CopyFrom(const KWDataPathManager* aSource)
{
	int i;
	KWObjectDataPath* objectDataPath;

	// Methode ancetre
	KWDataPathManager::CopyFrom(aSource);

	// Specialisation par optimisation des data paths
	for (i = 0; i < oaDataPaths.GetSize(); i++)
	{
		objectDataPath = cast(KWObjectDataPath*, oaDataPaths.GetAt(i));

		// Compilation
		objectDataPath->Compile(kwcMainClass);
	}
}

KWDataPathManager* KWObjectDataPathManager::Create() const
{
	return new KWObjectDataPathManager;
}
