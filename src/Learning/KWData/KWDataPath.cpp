// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataPath.h"

// Inclusion de ce header dans le source pour eviter une dependance cyclique
#include "KWClassDomain.h"

////////////////////////////////////////////////////////////
// Classe KWDataPath

KWDataPath::KWDataPath()
{
	bExternalTable = false;
	dataPathManager = NULL;
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

ALString KWDataPath::GetParentDataPathAt(int nDepth) const
{
	int i;
	int nParentDataPathAttributeNumber;
	ALString sParentDataPathAttributeNames;

	require(0 <= nDepth and nDepth <= GetDataPathAttributeNumber());

	// Nombre d'attribut du data path parent
	nParentDataPathAttributeNumber = GetDataPathAttributeNumber() - nDepth;

	// Calcul du chemin du parent a la profondeur donnee
	// Partie du data path lie au attributs
	for (i = 0; i < nParentDataPathAttributeNumber; i++)
	{
		if (i > 0)
			sParentDataPathAttributeNames += GetDataPathSeparator();
		sParentDataPathAttributeNames += GetFormattedName(GetDataPathAttributeNameAt(i));
	}

	// Cas d'un table externe
	if (GetExternalTable())
	{
		if (nParentDataPathAttributeNumber == 0)
			return GetDataPathSeparator() + GetFormattedName(GetOriginClassName());
		else
			return GetDataPathSeparator() + GetFormattedName(GetOriginClassName()) +
			       GetDataPathSeparator() + sParentDataPathAttributeNames;
	}
	// Cas standard
	else
		return sParentDataPathAttributeNames;
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

	require(Check());

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

void KWDataPath::ComputeEstimatedMemoryForObjectCreation(KWClassDomain* classDomain,
							 longint& lEstimatedMemoryForSingleObjectCreation,
							 longint& lEstimatedMemoryForMultipleObjectCreation,
							 longint& lEstimatedTotalCreatedSingleObjectNumber,
							 longint& lEstimatedTotalCreatedMultipleObjectNumber) const

{
	const boolean bTrace = false;
	static const int nBaseCreatedObjectNumberPerTable = 10;
	ObjectArray oaRemainingComponents;
	ObjectArray oaNewRemainingComponents;
	KWDataPath* dataPath;
	KWClass* kwcTerminalClass;
	int nDataPathCreatingTableNumber;
	longint lCreatedObjectNumber;
	longint lEstimatedCreatedObjectMemory;
	int nDepth;
	int i;
	int j;

	require(not GetCreatedObjects());

	// Trace
	if (bTrace)
		cout << "GetEstimatedMemoryForObjectCreation\t" << GetDataPath() << "\n";

	// Initialisation
	lEstimatedMemoryForSingleObjectCreation = 0;
	lEstimatedMemoryForMultipleObjectCreation = 0;
	lEstimatedTotalCreatedSingleObjectNumber = 0;
	lEstimatedTotalCreatedMultipleObjectNumber = 0;

	// Analyse en largeur d'abord de la composition
	nDepth = 0;
	oaRemainingComponents.CopyFrom(&oaComponents);
	while (oaRemainingComponents.GetSize() > 0)
	{
		// Initialisation de la nouvelle profondeur
		nDepth++;
		oaNewRemainingComponents.SetSize(0);

		// Parcours des composants a la profondeur courante
		for (i = 0; i < oaRemainingComponents.GetSize(); i++)
		{
			dataPath = cast(KWDataPath*, oaRemainingComponents.GetAt(i));

			// Traitement unique des data path de creation d'objet
			if (dataPath->GetCreatedObjects())
			{
				// Calcul du nombre de data path intermediaire avec creation de tables
				nDataPathCreatingTableNumber = 0;
				for (j = 0; j < nDepth; j++)
				{
					if (dataPath->GetDataPathAttributeTypeAt(
						dataPath->GetDataPathAttributeNumber() - 1 - j) == KWType::ObjectArray)
						nDataPathCreatingTableNumber++;
				}

				// Dimensionnement heuristique, avec un nombre d'instances cree en augmentant de facon quadratique avec
				// le nombre de tables impliquees dans les parents du data path
				// On compte 1 dans le cas des entites en relation 0-1.
				// On n'utilise pas une augmentation exponentielle, plus naturelle, avoir avoir des exigences minimalistes
				if (nDataPathCreatingTableNumber > 0)
					lCreatedObjectNumber = nBaseCreatedObjectNumberPerTable *
							       nDataPathCreatingTableNumber *
							       nDataPathCreatingTableNumber;
				else
				{
					assert(dataPath->GetDataPathAttributeTypeAt(
						   dataPath->GetDataPathAttributeNumber() - 1) == KWType::Object);
					lCreatedObjectNumber = 1;
				}

				// Recherche de la memoire necessaire par objet
				kwcTerminalClass = classDomain->LookupClass(dataPath->GetClassName());
				assert(kwcTerminalClass != NULL);
				lEstimatedCreatedObjectMemory =
				    kwcTerminalClass->GetEstimatedUsedMemoryPerObject(false);

				// Memorisation du nombre d'objet crees et de la memoire necessaire
				if (nDataPathCreatingTableNumber > 0)
				{
					lEstimatedTotalCreatedMultipleObjectNumber += lCreatedObjectNumber;
					lEstimatedMemoryForMultipleObjectCreation +=
					    lCreatedObjectNumber * lEstimatedCreatedObjectMemory;
				}
				else
				{
					lEstimatedTotalCreatedSingleObjectNumber += 1;
					lEstimatedMemoryForSingleObjectCreation += lEstimatedCreatedObjectMemory;
				}

				// Memorisation des data path pour la profondeur suivante
				oaNewRemainingComponents.InsertObjectArrayAt(oaNewRemainingComponents.GetSize(),
									     dataPath->GetComponents());

				// Trace
				if (bTrace)
				{
					if (nDepth == 1 and i == 0)
						cout << "\tData path\tCreated object\tObject memory\tTotal memory\n";
					cout << "\t" << dataPath->GetDataPath() << "\t";
					cout << lCreatedObjectNumber << "\t";
					cout << lEstimatedCreatedObjectMemory << "\t";
					cout << lCreatedObjectNumber * lEstimatedCreatedObjectMemory << "\n";
				}
			}
		}

		// Preparation pour la profondeur suivante
		oaRemainingComponents.CopyFrom(&oaNewRemainingComponents);
	}

	// Trace
	if (bTrace)
	{
		cout << "\tSynthesis\n";
		cout << "\t\tMemoryForSingleObjectCreation\t" << lEstimatedMemoryForSingleObjectCreation << "\n";
		cout << "\t\tMemoryForMultipleObjectCreation\t" << lEstimatedMemoryForMultipleObjectCreation << "\n";
		cout << "\t\tTotalCreatedSingleObjectNumber\t" << lEstimatedTotalCreatedSingleObjectNumber << "\n";
		cout << "\t\tTotalCreatedMultipleObjectNumber\t" << lEstimatedTotalCreatedMultipleObjectNumber << "\n";
	}
}

void KWDataPath::CopyFrom(const KWDataPath* aSource)
{
	require(aSource != NULL);

	bExternalTable = aSource->bExternalTable;
	sOriginClassName = aSource->sOriginClassName;
	svAttributeNames.CopyFrom(&aSource->svAttributeNames);
	ivAttributeTypes.CopyFrom(&aSource->ivAttributeTypes);
	sClassName = aSource->sClassName;
	dataPathManager = aSource->dataPathManager;
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

boolean KWDataPath::Check() const
{
	boolean bOk = true;
	KWClass* kwcCurrentClass;
	KWAttribute* currentAttribute;
	int nAttribute;
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
			if (GetExternalTable())
				AddError("Root dictionary " + sOriginClassName + " does not exist");
			else
				AddError("Main dictionary " + sOriginClassName + " does not exist");
			bOk = false;
		}
	}

	// Le data path ne peut contenir des objet cree dans le cas racine
	if (bOk)
	{
		if (svAttributeNames.GetSize() == 0 and GetCreatedObjects())
		{
			if (GetExternalTable())
				AddError("Root data path should be not be related to created instances");
			else
				AddError("Main data path should be not be related to created instances");
			bOk = false;
		}
	}

	// Parcours des attributs du data path pour les verification de coherence du chemin
	// On ne verifie pas la validite des cles, qui est verifiee par ailleurs avec les dictionnaires
	if (bOk)
	{
		assert(svAttributeNames.GetSize() == ivAttributeTypes.GetSize());
		for (nAttribute = 0; nAttribute < GetDataPathAttributeNumber(); nAttribute++)
		{
			// Recherche de l'attribut
			currentAttribute = kwcCurrentClass->LookupAttribute(GetDataPathAttributeNameAt(nAttribute));

			// Erreur si attribut inexistant
			if (currentAttribute == NULL)
			{
				AddError(sTmp + "Unknown variable " + GetDataPathAttributeNameAt(nAttribute) +
					 " at index " + IntToString(nAttribute + 1) +
					 " in data path, not found in dictionary " + kwcCurrentClass->GetName());
				bOk = false;
			}
			// Erreur si attribut n'est pas de type relation
			else if (not KWType::IsRelation(currentAttribute->GetType()))
			{
				AddError(sTmp + "Variable " + GetDataPathAttributeNameAt(nAttribute) + " at index " +
					 IntToString(nAttribute + 1) +
					 " in data path should be of relational data type");
				bOk = false;
			}
			// Erreur si type memorise de l'attribut incorrect
			else if (GetDataPathAttributeTypeAt(nAttribute) != currentAttribute->GetType())
			{
				AddError(sTmp + "Type " + KWType::ToString(GetDataPathAttributeTypeAt(nAttribute)) +
					 " of variable " + GetDataPathAttributeNameAt(nAttribute) + " at index " +
					 IntToString(nAttribute + 1) + " in data path should be " +
					 KWType::ToString(currentAttribute->GetType()));
				bOk = false;
			}
			// Sa classe doit etre coherente avec celle du mapping,
			// pour le dernier attribut du chemin
			else if (GetClassName() != currentAttribute->GetClass()->GetName())
			{
				if (nAttribute == GetDataPathAttributeNumber() - 1)
				{
					bOk = false;
					AddError("Dictionary of variable " + GetDataPathAttributeNameAt(nAttribute) +
						 " at index " + IntToString(nAttribute + 1) + " in data path (" +
						 GetClassName() + ") should be " +
						 currentAttribute->GetClass()->GetName());
				}
			}
			// Erreur eventuelle si attribut calcule
			else if (currentAttribute->GetDerivationRule() != NULL)
			{
				// Erreur si les regles de creation d'instances ne sont pas autorisees
				if (not IsRuleCreationManaged())
				{
					AddError(sTmp + "Variable " + GetDataPathAttributeNameAt(nAttribute) +
						 " at index " + IntToString(nAttribute + 1) +
						 " in data path should be native");
					bOk = false;
				}
				// Sinon, erreur si la regle est une regle de reference, qui ne cree pas d'instances
				else if (currentAttribute->GetDerivationRule()->GetReference())
				{
					AddError(sTmp + "Variable " + GetDataPathAttributeNameAt(nAttribute) +
						 " at index " + IntToString(nAttribute + 1) +
						 " in data path exploits derivation rule " +
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
	ost << "Data path\tType\tDictionary";
}

void KWDataPath::WriteLineReport(ostream& ost) const
{
	int i;

	// Data path
	ost << GetDataPath() << "\t";

	// Type abrege des elements du chemin en prennt les initiales des types
	for (i = 0; i < ivAttributeTypes.GetSize(); i++)
		cout << KWType::ToString(ivAttributeTypes.GetAt(i)).GetAt(0);

	// Dictionnaire extremite
	cout << "\t" << GetClassName();
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
	lUsedMemory += ivAttributeTypes.GetUsedMemory();
	lUsedMemory += sClassName.GetLength();
	lUsedMemory += oaComponents.GetUsedMemory();
	return lUsedMemory;
}

////////////////////////////////////////////////////////////
// Classe KWDataPathManager

KWDataPathManager::KWDataPathManager()
{
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
	IntVector ivAttributeTypes;
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
	assert(svAttributeName.GetSize() == 0);
	mainDataPath =
	    CreateDataPath(&odReferenceClasses, &oaRankedReferenceClasses, &odAnalysedCreatedClasses, mainClass, false,
			   false, mainClass->GetName(), &svAttributeName, &ivAttributeTypes, &oaDataPaths);
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
			dataPath = CreateDataPath(&odWorkingReferenceClasses, &oaWorkingRankedReferenceClasses,
						  &odWorkingAnalysedCreatedClasses, referenceClass, true, false,
						  referenceClass->GetName(), &svAttributeName, &ivAttributeTypes,
						  &oaWorkingCreatedDataPaths);
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
				dataPath = CreateDataPath(&odReferenceClasses, &oaRankedReferenceClasses,
							  &odAnalysedCreatedClasses, referenceClass, true, false,
							  referenceClass->GetName(), &svAttributeName,
							  &ivAttributeTypes, oaCreatedDataPaths);
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
		dataPath = cast(KWDataPath*, oaDataPaths.GetAt(i));
		odDataPaths.SetAt(dataPath->GetDataPath(), dataPath);
	}

	// Trace
	if (bTrace)
		cout << *this << endl;
	ensure(Check());
}

void KWDataPathManager::Reset()
{
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

	// Comparaisons de base
	if (nCompare == 0)
		nCompare = GetMainClassName() == aSource->GetMainClassName();
	if (nCompare == 0)
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
	assert(nCompare != 0 or (mainDataPath == NULL and aSource->mainDataPath == NULL) or
	       (mainDataPath->Compare(aSource->mainDataPath) == 0));
	assert(nCompare != 0 or oaExternalRootDataPaths.GetSize() == aSource->oaExternalRootDataPaths.GetSize());
	assert(nCompare != 0 or odDataPaths.GetCount() == aSource->odDataPaths.GetCount());
	assert(nCompare != 0 or
	       svUnusedRootDictionaryWarnings.GetSize() == aSource->svUnusedRootDictionaryWarnings.GetSize());
	return nCompare;
}

boolean KWDataPathManager::Check() const
{
	boolean bOk = true;
	const KWDataPath* dataPath;
	const KWDataPath* componentDataPath;
	int i;
	int j;

	// Existence de la classe princiale
	bOk = bOk and GetMainClassName() != "";
	bOk = bOk and KWClassDomain::GetCurrentDomain()->LookupClass(GetMainClassName()) != NULL;

	// Existence du mapping principal
	bOk = bOk and GetMainDataPath() != NULL;
	bOk = bOk and GetDataPathNumber() > 0;
	bOk = bOk and mainDataPath == GetDataPathAt(0);

	// Validite des data paths
	bOk = bOk and GetDataPathNumber() == odDataPaths.GetCount();
	for (i = 0; i < GetDataPathNumber(); i++)
	{
		dataPath = GetDataPathAt(i);

		// Test de validite
		bOk = bOk and dataPath->Check();
		bOk = bOk and dataPath == LookupDataPath(dataPath->GetDataPath());

		// Test d'existence des data path de la composition
		for (j = 0; j < dataPath->GetConstComponents()->GetSize(); j++)
		{
			componentDataPath = cast(const KWDataPath*, dataPath->GetConstComponents()->GetAt(j));
			bOk = bOk and componentDataPath == LookupDataPath(componentDataPath->GetDataPath());
		}
	}

	// Validite des data paths des tables externes
	for (i = 0; i < GetExternalRootDataPathNumber(); i++)
	{
		dataPath = GetExternalRootDataPathAt(i);
		bOk = bOk and dataPath == LookupDataPath(dataPath->GetDataPath());
	}
	return bOk;
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
	return GetMainClassName();
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
					      IntVector* ivAttributeTypes, ObjectArray* oaCreatedDataPaths)
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
	require(svAttributeNames != NULL);
	require(ivAttributeTypes != NULL);
	require(svAttributeNames->GetSize() == ivAttributeTypes->GetSize());
	require(not mappedClass->GetRoot() or mappedClass->GetName() == sOriginClassName);
	require(not mappedClass->GetRoot() or svAttributeNames->GetSize() == 0);
	require(oaCreatedDataPaths != NULL);

	// Creation et initialisation d'un dataPath
	dataPath = dataPathCreator->Create();
	dataPath->SetExternalTable(bIsExternalTable);
	if (IsRuleCreationManaged())
		dataPath->SetCreatedObjects(bCreatedObjects);
	dataPath->SetClassName(mappedClass->GetName());
	dataPath->SetOriginClassName(sOriginClassName);
	dataPath->svAttributeNames.CopyFrom(svAttributeNames);
	dataPath->ivAttributeTypes.CopyFrom(ivAttributeTypes);
	dataPath->SetDataPathManager(this);
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
					ivAttributeTypes->Add(attribute->GetType());
					componentDataPath = CreateDataPath(
					    odReferenceClasses, oaRankedReferenceClasses, odAnalysedCreatedClasses,
					    attribute->GetClass(), bIsExternalTable, bCreatedObjects, sOriginClassName,
					    svAttributeNames, ivAttributeTypes, oaCreatedDataPaths);
					svAttributeNames->SetSize(svAttributeNames->GetSize() - 1);
					ivAttributeTypes->SetSize(ivAttributeTypes->GetSize() - 1);

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
					ivAttributeTypes->Add(attribute->GetType());
					componentDataPath = CreateDataPath(
					    odReferenceClasses, oaRankedReferenceClasses, odAnalysedCreatedClasses,
					    attribute->GetClass(), bIsExternalTable, true, sOriginClassName,
					    svAttributeNames, ivAttributeTypes, oaCreatedDataPaths);
					svAttributeNames->SetSize(svAttributeNames->GetSize() - 1);
					ivAttributeTypes->SetSize(ivAttributeTypes->GetSize() - 1);

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
