// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWClassDomain.h"

KWClassDomain::KWClassDomain()
{
	nUpdateNumber = 0;
	nAllClassesFreshness = 0;
}

KWClassDomain::~KWClassDomain()
{
	DeleteAllClasses();
}

/////////////////////////////////////////////////////////////////////////////
// Gestion des classes d'un domaine

void KWClassDomain::SetName(const ALString& sValue)
{
	// Nommage simple si domaine non enregistre globalement
	if (LookupDomain(usName.GetValue()) != this)
		usName.SetValue(sValue);
	// Sinon renommage dans le gestionnaire global de domaines
	else
		RenameDomain(this, sValue);
}

const ALString& KWClassDomain::GetName() const
{
	return usName.GetValue();
}

void KWClassDomain::SetLabel(const ALString& sValue)
{
	usLabel.SetValue(sValue);
}

const ALString& KWClassDomain::GetLabel() const
{
	return usLabel.GetValue();
}

boolean KWClassDomain::WriteFile(const ALString& sFileName) const
{
	fstream fst;
	boolean bOk;
	ALString sCorrectedFileName;
	ALString sLocalFileName;
	KWClass classRef;

	// Correction du nom si necessaire
	sCorrectedFileName = KWResultFilePathBuilder::UpdateFileSuffix(sFileName, "kdic");

	// Affichage de stats memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sCorrectedFileName + " WriteFile Begin");

	// Verification et modification du suffixe du fichier si necessaire
	bOk = KWResultFilePathBuilder::CheckFileSuffix(sFileName, "kdic", classRef.GetClassLabel());
	bOk = bOk and KWResultFilePathBuilder::CheckResultDirectory(FileService::GetPathName(sFileName),
								    classRef.GetClassLabel());

	// Preparation de la copie sur HDFS si necessaire
	if (bOk)
		bOk = PLRemoteFileService::BuildOutputWorkingFile(sCorrectedFileName, sLocalFileName);

	// Ouverture du fichier en ecriture
	if (bOk)
		bOk = FileService::OpenOutputFile(sLocalFileName, fst);

	// Si OK: ecriture des classes
	if (bOk)
	{
		if (GetLearningReportHeaderLine() != "")
			fst << GetLearningReportHeaderLine() << "\n";
		fst << *this;
		bOk = FileService::CloseOutputFile(sLocalFileName, fst);

		// Destruction du fichier si erreur
		if (not bOk)
			FileService::RemoveFile(sLocalFileName);
	}

	if (bOk)
	{
		// Copie vers HDFS si necessaire
		PLRemoteFileService::CleanOutputWorkingFile(sCorrectedFileName, sLocalFileName);
	}

	// Affichage de stats memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sCorrectedFileName + " WriteFile End");
	return bOk;
}

boolean KWClassDomain::WriteFileFromClass(const KWClass* rootClass, const ALString& sFileName) const
{
	fstream fst;
	boolean bOk;
	ObjectDictionary odDependentClasses;
	ObjectArray oaDependentClasses;
	int nClass;
	KWClass* kwcElement;

	// Pas de gestion des fichiers cloud, car utilise actuellement uniquement en local
	require(FileService::GetURIScheme(sFileName) == "");
	require(rootClass != NULL);
	require(rootClass->GetDomain() == this);

	// Affichage de stats memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " WriteFileFromClass Begin");

	// Ouverture du fichier en ecriture
	bOk = FileService::OpenOutputFile(sFileName, fst);

	// Si OK: ecriture des classe
	if (bOk)
	{
		// Calcul des classes dependantes
		ComputeClassDependence(rootClass, &odDependentClasses);

		// Export dans un tableau ou l'on tri les dictionnaires
		odDependentClasses.ExportObjectArray(&oaDependentClasses);
		oaDependentClasses.SetCompareFunction(KWClassCompareName);
		oaDependentClasses.Sort();

		// Ligne d'entete du fichier dictionnaire
		if (GetLearningReportHeaderLine() != "")
			fst << GetLearningReportHeaderLine() << "\n";

		// Parcours des classes dependantes pour n'ecrire que ces classes
		for (nClass = 0; nClass < oaDependentClasses.GetSize(); nClass++)
		{
			kwcElement = cast(KWClass*, oaDependentClasses.GetAt(nClass));
			fst << *kwcElement;
		}

		// Fermeture du fichier
		bOk = FileService::CloseOutputFile(sFileName, fst);

		// Destruction du fichier si erreur
		if (not bOk)
			FileService::RemoveFile(sFileName);
	}

	// Affichage de stats memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + sFileName + " WriteFileFromClass End");
	return bOk;
}

boolean KWClassDomain::WriteJSONFile(const ALString& sJSONFileName) const
{
	boolean bOk;
	JSONFile fJSON;
	int i;
	KWClass* kwcElement;
	ALString sCorrectedJSONFileName;
	KWClass classRef;

	// Correction du nom si necessaire
	sCorrectedJSONFileName = KWResultFilePathBuilder::UpdateFileSuffix(sJSONFileName, "kdicj");

	// Verification et modification du suffixe du fichier si necessaire
	bOk = KWResultFilePathBuilder::CheckFileSuffix(sJSONFileName, "kdicj", classRef.GetClassLabel());
	bOk = bOk and KWResultFilePathBuilder::CheckResultDirectory(FileService::GetPathName(sJSONFileName),
								    classRef.GetClassLabel());

	// Ouverture du fichier JSON
	// La prise en compte des fichiers cloud est geree par la classe JSONFile
	fJSON.SetFileName(sCorrectedJSONFileName);
	if (bOk)
		bOk = fJSON.OpenForWrite();

	// Si OK: ecriture des classe
	if (bOk)
	{
		// Outil et version
		fJSON.WriteKeyString("tool", GetLearningApplicationName() + " Dictionary");
		fJSON.WriteKeyString("version", GetLearningVersion());

		// Ecriture des dictionaires au format JSON
		fJSON.BeginKeyArray("dictionaries");
		for (i = 0; i < GetClassNumber(); i++)
		{
			kwcElement = GetClassAt(i);
			if (kwcElement->GetAttributeNumber() != 0)
				kwcElement->WriteJSONReport(&fJSON);
		}
		fJSON.EndArray();

		// Fermeture du fichier
		bOk = fJSON.Close();

		// Destruction du fichier si erreur
		if (not bOk)
			FileService::RemoveFile(sCorrectedJSONFileName);
	}
	return bOk;
}

KWClass* KWClassDomain::LookupClass(const ALString& sClassName) const
{
	return cast(KWClass*, odClasses.Lookup(sClassName));
}

boolean KWClassDomain::InsertClass(KWClass* newObject)
{
	require(newObject != NULL);
	require(newObject->GetDomain() == NULL);

	if (odClasses.Lookup(newObject->GetName()) == NULL)
	{
		odClasses.SetAt(newObject->GetName(), newObject);
		newObject->domain = this;
		nUpdateNumber++;
		return true;
	}
	else
		return false;
}

void KWClassDomain::InsertClassWithNewName(KWClass* newObject, const ALString& sNewName)
{
	KWClassDomain temporaryDomain;

	require(newObject != NULL);
	require(newObject->GetDomain() == NULL);
	require(odClasses.Lookup(sNewName) == NULL);

	// Renommage de la classe en passant par un domaine intermediaire
	// (l'insertion dans ce domaine temporaire ne pose pas de probleme)
	temporaryDomain.InsertClass(newObject);
	temporaryDomain.RenameClass(newObject, sNewName);
	temporaryDomain.RemoveClass(sNewName);
	assert(newObject->GetName() == sNewName);

	// Insertion dans le domaine courant
	odClasses.SetAt(newObject->GetName(), newObject);
	newObject->domain = this;
	nUpdateNumber++;
}

boolean KWClassDomain::RemoveClass(const ALString& sClassName)
{
	KWClass* kwcToRemove;

	kwcToRemove = LookupClass(sClassName);
	if (odClasses.RemoveKey(sClassName))
	{
		nUpdateNumber++;
		check(kwcToRemove);
		kwcToRemove->domain = NULL;
		return true;
	}
	else
		return false;
}

boolean KWClassDomain::DeleteClass(const ALString& sClassName)
{
	KWClass* kwcToDelete;

	kwcToDelete = LookupClass(sClassName);
	if (odClasses.RemoveKey(sClassName))
	{
		check(kwcToDelete);
		delete kwcToDelete;
		nUpdateNumber++;
		return true;
	}
	else
		return false;
}

boolean KWClassDomain::RenameClass(KWClass* refClass, const ALString& sNewName)
{
	ALString sOldName;
	KWClass* existingClass;
	KWClass* kwcClass;
	KWAttribute* attribute;
	KWDerivationRule* currentDerivationRule;
	int i;

	require(refClass != NULL);
	require(refClass == cast(KWClass*, odClasses.Lookup(refClass->GetName())));
	require(refClass->domain == this);
	require(refClass->CheckName(sNewName, refClass));

	// Si le nom n'est pas nouveau, le renommage est implicitement deja effectue
	if (refClass->GetName() == sNewName)
		return true;

	// Test d'existence du nouveau nom
	existingClass = cast(KWClass*, odClasses.Lookup(sNewName));
	if (existingClass != NULL)
		return false;
	// Renommage par manipulation dans le dictionnaire
	// Les classes referencees par les autres attributs restent coherentes:
	// seul le nom de la classe a change
	else
	{
		// Propagation du renommage a toutes les regles de derivation
		// des classes du domaine referencant l'attribut
		for (i = 0; i < GetClassNumber(); i++)
		{
			kwcClass = GetClassAt(i);

			// Parcours des attributs de la classe
			attribute = kwcClass->GetHeadAttribute();
			currentDerivationRule = NULL;
			while (attribute != NULL)
			{
				// Detection de changement de regle de derivation (notamment pour les blocs)
				if (attribute->GetAnyDerivationRule() != currentDerivationRule)
				{
					currentDerivationRule = attribute->GetAnyDerivationRule();

					// Renommage dans les regles de derivation (et au plus une seule fois par bloc)
					if (currentDerivationRule != NULL)
						currentDerivationRule->RenameClass(refClass, sNewName);
				}

				// Attribut suivant
				kwcClass->GetNextAttribute(attribute);
			}
		}

		// Renommage de la classe dans le domaine
		sOldName = refClass->GetName();
		odClasses.RemoveKey(refClass->GetName());
		refClass->usName.SetValue(sNewName);
		odClasses.SetAt(refClass->GetName(), refClass);
		nUpdateNumber++;
		return true;
	}
}

boolean KWClassDomain::RenameAttribute(KWAttribute* refAttribute, const ALString& sNewAttributeName)
{
	KWClass* refClass;
	KWClass* kwcClass;
	KWAttribute* attribute;
	KWDerivationRule* currentDerivationRule;
	int i;

	require(refAttribute != NULL);
	require(refAttribute->GetParentClass() != NULL);
	require(refAttribute->GetParentClass()->LookupAttribute(refAttribute->GetName()) == refAttribute);
	require(refAttribute->GetParentClass() ==
		cast(KWClass*, odClasses.Lookup(refAttribute->GetParentClass()->GetName())));
	require(refAttribute->GetParentClass()->domain == this);

	// Test si renommage possible sur la classe de depart
	refClass = refAttribute->GetParentClass();
	if (refClass->LookupAttribute(sNewAttributeName) != NULL)
		return false;
	// Propagation du renommage si necessaire
	else
	{
		// Propagation du renommage a toutes les regles de derivation
		// des classes du domaine referencant l'attribut
		for (i = 0; i < GetClassNumber(); i++)
		{
			kwcClass = GetClassAt(i);

			// Parcours des attributs de la classe
			// (sauf classe de depart deja traitee)
			if (kwcClass != refClass)
			{
				attribute = kwcClass->GetHeadAttribute();
				currentDerivationRule = NULL;
				while (attribute != NULL)
				{
					// Detection de changement de regle de derivation (notamment pour les blocs)
					if (attribute->GetAnyDerivationRule() != currentDerivationRule)
					{
						currentDerivationRule = attribute->GetAnyDerivationRule();

						// Renommage dans les regles de derivation (et au plus une seule fois
						// par bloc)
						if (currentDerivationRule != NULL)
							currentDerivationRule->RenameAttribute(kwcClass, refAttribute,
											       sNewAttributeName);
					}

					// Attribut suivant
					kwcClass->GetNextAttribute(attribute);
				}
			}
		}

		// Renommage de l'attribut sur la classe de depart
		refClass->RenameAttribute(refAttribute, sNewAttributeName);
		return true;
	}
}

ObjectArray* KWClassDomain::AllClasses() const
{
	// Quelques adaptations (mutable...) ont ete necessaire pour
	// rendre la methode const
	if (nAllClassesFreshness < nUpdateNumber or oaClasses.GetSize() == 0)
	{
		odClasses.ExportObjectArray(&oaClasses);
		oaClasses.SetCompareFunction(KWClassCompareName);
		oaClasses.Sort();
		nAllClassesFreshness = nUpdateNumber;
	}
	return &oaClasses;
}

int KWClassDomain::GetClassNumber() const
{
	return AllClasses()->GetSize();
}

KWClass* KWClassDomain::GetClassAt(int i) const
{
	require(0 <= i and i < GetClassNumber());
	return cast(KWClass*, AllClasses()->GetAt(i));
}

void KWClassDomain::ExportClassArray(ObjectArray* oaResult) const
{
	int i;

	require(oaResult != NULL);

	oaResult->SetSize(GetClassNumber());
	for (i = 0; i < oaResult->GetSize(); i++)
		oaResult->SetAt(i, GetClassAt(i));
}

void KWClassDomain::ExportClassDictionary(ObjectDictionary* odResult) const
{
	int i;
	KWClass* kwcClass;

	require(odResult != NULL);

	odResult->RemoveAll();
	for (i = 0; i < GetClassNumber(); i++)
	{
		kwcClass = GetClassAt(i);
		odResult->SetAt(kwcClass->GetName(), kwcClass);
	}
}

void KWClassDomain::RemoveAllClasses()
{
	KWClass* kwcClassToRemove;
	int nClass;

	// Il ne faut pas oublier de dereferencer le domaine des classes supprimees
	AllClasses();
	assert(oaClasses.GetSize() == odClasses.GetCount());
	for (nClass = 0; nClass < oaClasses.GetSize(); nClass++)
	{
		kwcClassToRemove = cast(KWClass*, oaClasses.GetAt(nClass));
		kwcClassToRemove->domain = NULL;
	}

	// Suppression des classes
	odClasses.RemoveAll();
	oaClasses.RemoveAll();
	nUpdateNumber++;
}

void KWClassDomain::DeleteAllClasses()
{
	oaClasses.RemoveAll();
	odClasses.DeleteAll();
	nUpdateNumber++;
}

const ALString KWClassDomain::BuildClassName(const ALString& sPrefix)
{
	ALString sClassName;
	ALString sSuffix;
	int nIndex;

	// Recherche d'un nom inutilise en suffixant par un index croissant
	nIndex = 1;
	sClassName = sPrefix;
	while (sClassName.GetLength() > KWClass::GetNameMaxLength() or odClasses.Lookup(sClassName) != NULL)
	{
		sSuffix = IntToString(nIndex);
		sClassName = sPrefix + sSuffix;

		// Reduction si necessaire de la taille du nom
		if (sClassName.GetLength() > KWClass::GetNameMaxLength())
		{
			sClassName = KWClass::BuildUTF8SubString(
					 sClassName.Left(KWClass::GetNameMaxLength() - sSuffix.GetLength() - 1)) +
				     "_" + sSuffix;
		}

		// Passage au nom suivant
		nIndex++;
	}
	return sClassName;
}

boolean KWClassDomain::Check() const
{
	int i;
	KWClass* kwcElement;
	KWAttribute* attribute;
	boolean bResult;

	bResult = true;
	Global::ActivateErrorFlowControl();

	// Verification de l'integrite de chaque classe
	for (i = 0; i < GetClassNumber(); i++)
	{
		kwcElement = GetClassAt(i);
		if (not kwcElement->Check())
		{
			kwcElement->AddError("Integrity errors");
			bResult = false;
		}

		// Verification de la coherence du domaine
		if (kwcElement->GetDomain() != this)
		{
			kwcElement->AddError("Inconsistent dictionary domain");
			bResult = false;
		}
	}

	// Verification de l'integrite referentielle: toute classe referencee doit
	// appartenir au meme domaine de classe
	if (bResult)
	{
		for (i = 0; i < GetClassNumber(); i++)
		{
			kwcElement = GetClassAt(i);

			// Verification des attributs de type Object ou ObjectArray
			attribute = kwcElement->GetHeadAttribute();
			while (attribute != NULL)
			{
				// Si sttribut avec classe referencee
				if (KWType::IsRelation(attribute->GetType()))
				{
					check(attribute->GetClass());
					if (LookupClass(attribute->GetClass()->GetName()) != attribute->GetClass())
					{
						kwcElement->AddError("Variable " + attribute->GetName() +
								     " references the dictionary " +
								     attribute->GetClass()->GetName() +
								     " which is belongs to another dictionary domain");
						bResult = false;
					}
				}

				// Attribut suivant
				kwcElement->GetNextAttribute(attribute);
			}
		}
	}
	Global::DesactivateErrorFlowControl();

	return bResult;
}

void KWClassDomain::CompleteTypeInfo()
{
	int nClass;
	KWClass* kwcElement;
	NumericKeyDictionary nkdAttributes;
	KWAttribute* attribute;

	// Parcours des classes du domaine
	for (nClass = 0; nClass < GetClassNumber(); nClass++)
	{
		kwcElement = GetClassAt(nClass);

		// Parcours des attributs de la classe
		attribute = kwcElement->GetHeadAttribute();
		while (attribute != NULL)
		{
			attribute->InternalCompleteTypeInfo(kwcElement, &nkdAttributes);

			// Attribut suivant
			kwcElement->GetNextAttribute(attribute);
		}
	}
}

void KWClassDomain::Compile()
{
	boolean bIsDomainCompiled;
	int nClass;
	KWClass* kwcElement;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	NumericKeyDictionary nkdGreyAttributes;
	NumericKeyDictionary nkdBlackAttributes;
	ObjectList olCalledAttributes;
	boolean bContainsCycle;

	require(Check());

	// Test si toutes les classes sont compilees
	bIsDomainCompiled = true;
	for (nClass = 0; nClass < GetClassNumber(); nClass++)
	{
		kwcElement = GetClassAt(nClass);
		if (not kwcElement->IsCompiled())
		{
			bIsDomainCompiled = false;
			break;
		}
	}

	// Arret si deja compile
	if (bIsDomainCompiled)
		return;

	// Affichage de stats memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " Compile Begin");

	// Si au moins une classe non compilee, il est necessaire de tout recompiler en raison des
	// regles de derivation creant des dependances potentielles inter-classes
	assert(not bIsDomainCompiled);
	for (nClass = 0; nClass < GetClassNumber(); nClass++)
	{
		kwcElement = GetClassAt(nClass);

		// Inhibition de la fraicheur de compilation
		kwcElement->UpdateFreshness();

		// Reindexation
		kwcElement->IndexClass();

		// Mise a jour de la fraicheur de compilation avant de compiler les regles de derivation
		// Celles-ci memorisent en effet la fraicheur des classes referencees par leurs operandes
		kwcElement->nCompileFreshness = kwcElement->nFreshness;
	}

	// Compilation des regles de derivation de toutes les classes du domaines
	// Il n'est pas possible d'appeler la methode Compile classe par classe
	for (nClass = 0; nClass < GetClassNumber(); nClass++)
	{
		kwcElement = GetClassAt(nClass);

		// Compilation des regles de derivation
		attribute = kwcElement->GetHeadAttribute();
		while (attribute != NULL)
		{
			attribute->Compile();
			kwcElement->GetNextAttribute(attribute);
		}
	}

	// Parametrage specifique de toutes les regles Random utilisees dans les classes du domaine
	for (nClass = 0; nClass < GetClassNumber(); nClass++)
	{
		kwcElement = GetClassAt(nClass);
		kwcElement->InitializeAllRandomRuleParameters();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Apres la compilation, test de l'existence de cycles de derivation
	// Algorithme de detection de cycle base sur la coloration des noeuds en White, Grey, Black:
	//   In order to detect cycles, we use a modified depth first search called a colored DFS.
	//   All nodes are initially marked white. When a node is encountered, it is marked grey,
	//   and when its descendants are completely visited, it is marked black. If a grey node
	//   is ever encountered, then there is a cycle.
	//
	//   	boolean containsCycle(Graph g):
	//        for each vertex v in g do:
	//          v.mark = WHITE;
	//        od;
	//        for each vertex v in g do:
	//          if v.mark == WHITE then:
	//            if visit(g, v) then:
	//              return TRUE;
	//            fi;
	//          fi;
	//        od;
	//        return FALSE;
	//
	//      boolean visit(Graph g, Vertex v):
	//        v.mark = GREY;
	//        for each edge (v, u) in g do:
	//          if u.mark == GREY then:
	//            return TRUE;
	//          else if u.mark == WHITE then:
	//            if visit(g, u) then:
	//              return TRUE;
	//            fi;
	//          fi;
	//        od;
	//        v.mark = BLACK;
	//        return FALSE;
	//
	// Ici, on se passe de la premiere etape de collaration en White, et on utilise deux
	// dictionaires pour memoriser la couleur Grey ou Black
	//
	// Dans le cas d'un bloc d'attribut, on considere que tous les attributs d'un bloc
	// collectivement comme un noeud, et on colore le AttributeBlock au lie de l'Attribute,
	// ce qui permet de regrouper les analyser de cycle au niveau des blocks
	Global::ActivateErrorFlowControl();
	bContainsCycle = false;
	for (nClass = 0; nClass < GetClassNumber(); nClass++)
	{
		kwcElement = GetClassAt(nClass);

		// Parcours des attributs de la classe
		attribute = kwcElement->GetHeadAttribute();
		while (attribute != NULL)
		{
			attributeBlock = attribute->GetAttributeBlock();

			// Test si le bloc est en White (ni Grey, ni Black)
			if (attributeBlock != NULL)
			{
				// Test une seule fois, pour le premier attribut du block
				if (attribute->IsFirstInBlock())
				{
					if (nkdGreyAttributes.Lookup(attributeBlock) == NULL and
					    nkdBlackAttributes.Lookup(attributeBlock) == NULL)
					{
						if (attributeBlock->ContainsCycle(&nkdGreyAttributes,
										  &nkdBlackAttributes))
						{
							bContainsCycle = true;
							break;
						}
					}
				}
			}
			// Test si l'attribut est en White (ni Grey, ni Black)
			else if (nkdGreyAttributes.Lookup(attribute) == NULL and
				 nkdBlackAttributes.Lookup(attribute) == NULL)
			{
				if (attribute->ContainsCycle(&nkdGreyAttributes, &nkdBlackAttributes))
				{
					bContainsCycle = true;
					break;
				}
			}

			// Attribut suivant
			kwcElement->GetNextAttribute(attribute);
		}

		// Message d'erreur si au moins un cycle dans la classe
		if (bContainsCycle)
		{
			kwcElement->AddError("Existing cycle in the graph of derivation rules");
			break;
		}
	}
	Global::DesactivateErrorFlowControl();

	// Si presence de cycle, invalidation de la compilation des classes
	// en incrementant leur fraicheur
	if (bContainsCycle)
	{
		for (nClass = 0; nClass < GetClassNumber(); nClass++)
		{
			kwcElement = GetClassAt(nClass);

			// Inhibition de la fraicheur de compilation
			kwcElement->UpdateFreshness();

			// Reindexation
			kwcElement->IndexClass();
		}
	}

	// Affichage de stats memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " Compile End");
}

KWClassDomain* KWClassDomain::Clone() const
{
	KWClassDomain* kwcdClone;
	int nClass;
	KWClass* kwcElement;
	KWClass* kwcCloneElement;
	KWClass* kwcCloneRef;
	KWAttribute* attribute;

	kwcdClone = new KWClassDomain;

	// Duplication des attributs standards
	kwcdClone->usName = usName;
	kwcdClone->usLabel = usLabel;

	// Duplication des classes
	for (nClass = 0; nClass < GetClassNumber(); nClass++)
	{
		kwcElement = GetClassAt(nClass);

		if (kwcElement != NULL)
			kwcdClone->InsertClass(kwcElement->Clone());
	}

	// On force l'integrite l'integrite referentielle: toute
	// classe referencee doit appartenir au meme domaine de classe
	for (nClass = 0; nClass < kwcdClone->GetClassNumber(); nClass++)
	{
		kwcCloneElement = kwcdClone->GetClassAt(nClass);

		// Attributs de type Object ou ObjectArray
		attribute = kwcCloneElement->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Si attribut avec classe referencee
			if (KWType::IsRelation(attribute->GetType()))
			{
				if (attribute->GetClass() != NULL)
				{
					// Referencement de la nouvelle classe
					kwcCloneRef = kwcdClone->LookupClass(attribute->GetClass()->GetName());
					attribute->SetClass(kwcCloneRef);
				}
			}

			// Attribut suivant
			kwcCloneElement->GetNextAttribute(attribute);
		}
	}

	return kwcdClone;
}

KWClassDomain* KWClassDomain::CloneFromClass(const KWClass* rootClass) const
{
	KWClassDomain* kwcdClone;
	ObjectArray oaImpactedClasses;
	int nClass;
	KWClass* kwcCloneElement;
	KWClass* kwcRef;
	KWClass* kwcCloneRef;
	KWAttribute* attribute;

	require(rootClass != NULL);
	require(rootClass->GetDomain() == this);

	kwcdClone = new KWClassDomain;

	// Duplication des attributs standards
	kwcdClone->usName = usName;
	kwcdClone->usLabel = usLabel;

	// Duplication de la classe racine
	kwcCloneElement = rootClass->Clone();
	kwcdClone->InsertClass(kwcCloneElement);
	oaImpactedClasses.Add(kwcCloneElement);

	// Duplication des classes referencees en memorisant les classes dupliquees
	// pour lesquelles il faut propager la duplication
	for (nClass = 0; nClass < oaImpactedClasses.GetSize(); nClass++)
	{
		assert(oaImpactedClasses.GetSize() == kwcdClone->GetClassNumber());
		kwcCloneElement = cast(KWClass*, oaImpactedClasses.GetAt(nClass));

		// Attributs de type Object ou ObjectArray
		attribute = kwcCloneElement->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Si sttribut avec classe referencee
			if (KWType::IsRelation(attribute->GetType()))
			{
				if (attribute->GetClass() != NULL)
				{
					// Recherche de la nouvelle classe a referencer
					kwcCloneRef = kwcdClone->LookupClass(attribute->GetClass()->GetName());

					// Creation si necessaire
					if (kwcCloneRef == NULL)
					{
						kwcRef = LookupClass(attribute->GetClass()->GetName());
						if (kwcRef != NULL)
						{
							kwcCloneRef = kwcRef->Clone();

							// Memorisation dans le domaine
							kwcdClone->InsertClass(kwcCloneRef);
							oaImpactedClasses.Add(kwcCloneRef);
						}
					}

					// Memorisation de la nouvelle classe a referencer
					attribute->SetClass(kwcCloneRef);
				}
			}

			// Attribut suivant
			kwcCloneElement->GetNextAttribute(attribute);
		}
	}

	return kwcdClone;
}

void KWClassDomain::ImportDomain(KWClassDomain* kwcdInputDomain, const ALString& sClassPrefix,
				 const ALString& sClassSuffix)
{
	int i;
	KWClass* kwcElement;
	KWClass* kwcNewElement;
	ALString sNewName;
	ObjectArray oaInputClasses;
	ObjectArray oaOutputClasses;

	require(kwcdInputDomain != NULL);

	// Collecte des classe concernee dans un tableau
	kwcdInputDomain->ExportClassArray(&oaInputClasses);

	// Calcul des nouveaux nom des classes dans leur domaine d'acceuil
	// en creant des classes "vides", permettant de reserver les nouveaux noms
	oaOutputClasses.SetSize(oaInputClasses.GetSize());
	for (i = 0; i < oaInputClasses.GetSize(); i++)
	{
		kwcElement = cast(KWClass*, oaInputClasses.GetAt(i));

		// Recherche d'un nouveau nom unique
		sNewName = sClassPrefix + kwcElement->GetName() + sClassSuffix;
		sNewName = BuildClassName(sNewName);

		// Reservation du nom dans le domaine d'accueil
		kwcNewElement = new KWClass;
		kwcNewElement->SetName(sNewName);
		oaOutputClasses.SetAt(i, kwcNewElement);
		InsertClass(kwcNewElement);
	}

	// Changement des nom dans le domaine initial (ce qui limite la propagation du renommage)
	for (i = 0; i < oaInputClasses.GetSize(); i++)
	{
		kwcElement = cast(KWClass*, oaInputClasses.GetAt(i));
		kwcNewElement = cast(KWClass*, oaOutputClasses.GetAt(i));
		kwcdInputDomain->RenameClass(kwcElement, kwcNewElement->GetName());
	}

	// Destruction des nouvelle classe temporaires
	for (i = 0; i < oaInputClasses.GetSize(); i++)
	{
		kwcNewElement = cast(KWClass*, oaOutputClasses.GetAt(i));
		DeleteClass(kwcNewElement->GetName());
	}

	// Transfert des classes renommees du domaine initial vers le domaine courant
	kwcdInputDomain->RemoveAllClasses();
	for (i = 0; i < oaInputClasses.GetSize(); i++)
	{
		kwcElement = cast(KWClass*, oaInputClasses.GetAt(i));
		InsertClass(kwcElement);
	}

	ensure(kwcdInputDomain->GetClassNumber() == 0);
}

void KWClassDomain::ComputeClassDependence(const KWClass* rootClass, ObjectDictionary* odDependentClasses) const
{
	ObjectArray oaDependentClasses;
	int nClass;
	KWClass* kwcElement;
	KWClass* kwcRef;
	KWAttribute* attribute;

	require(rootClass != NULL);
	require(rootClass->GetDomain() == this);
	require(odDependentClasses != NULL);

	// Enregistrement de la classe racine
	// (en la castant, pour contourner le const du parametre)
	odDependentClasses->RemoveAll();
	odDependentClasses->SetAt(rootClass->GetName(), cast(KWClass*, rootClass));
	oaDependentClasses.Add(cast(KWClass*, rootClass));

	// Parcours des classes dependantes en memorisant les classes
	// pour lesquelles il faut propager le calcul de dependance
	for (nClass = 0; nClass < oaDependentClasses.GetSize(); nClass++)
	{
		kwcElement = cast(KWClass*, oaDependentClasses.GetAt(nClass));

		// Attributs de type Object ou ObjectArray
		attribute = kwcElement->GetHeadAttribute();
		while (attribute != NULL)
		{
			// Si attribut avec classe referencee
			if (KWType::IsRelation(attribute->GetType()))
			{
				if (attribute->GetClass() != NULL)
				{
					// Prise en compte si necessaire d'une nouvelle classe
					kwcRef = cast(KWClass*,
						      odDependentClasses->Lookup(attribute->GetClass()->GetName()));
					if (kwcRef == NULL)
					{
						odDependentClasses->SetAt(attribute->GetClass()->GetName(),
									  attribute->GetClass());
						oaDependentClasses.Add(attribute->GetClass());
					}
				}
			}

			// Attribut suivant
			kwcElement->GetNextAttribute(attribute);
		}
	}
}

longint KWClassDomain::GetUsedMemory() const
{
	longint lUsedMemory;
	int i;
	KWClass* kwcClass;
	longint lClassUsedMemory;

	// Prise en compte de le domaine lui-meme
	lUsedMemory = sizeof(KWClassDomain);

	// On prend en compte la memoire du nom et du libelle, meme s'ils sont potentiellement partages
	lUsedMemory += usName.GetUsedMemory();
	lUsedMemory += usLabel.GetUsedMemory();

	// Prise en compte du referencement des classes
	lUsedMemory += odClasses.GetUsedMemory();
	lUsedMemory += oaClasses.GetUsedMemory();

	// Prise en compte des classe
	for (i = 0; i < GetClassNumber(); i++)
	{
		kwcClass = GetClassAt(i);
		lClassUsedMemory = kwcClass->GetUsedMemory();
		lUsedMemory += lClassUsedMemory;
	}
	return lUsedMemory;
}

longint KWClassDomain::GetClassDependanceUsedMemory(const KWClass* rootClass) const
{
	longint lUsedMemory;
	longint lClassUsedMemory;
	ObjectDictionary odDependentClasses;
	ObjectArray oaDependentClasses;
	int nClass;
	KWClass* kwcElement;

	require(rootClass != NULL);
	require(rootClass->GetDomain() == this);

	// Calcul des classes dependantes
	ComputeClassDependence(rootClass, &odDependentClasses);

	// Parcours des classes dependantes en memorisant les classes
	// pour lesquelles il faut propager le calcul de dependance
	lUsedMemory = 0;
	odDependentClasses.ExportObjectArray(&oaDependentClasses);
	for (nClass = 0; nClass < oaDependentClasses.GetSize(); nClass++)
	{
		kwcElement = cast(KWClass*, oaDependentClasses.GetAt(nClass));

		// Prise en compte de la memoire occupee par la classe
		lClassUsedMemory = kwcElement->GetUsedMemory();
		lUsedMemory += lClassUsedMemory;
	}
	return lUsedMemory;
}

longint KWClassDomain::ComputeHashValue() const
{
	longint lHash;
	int nClass;
	KWClass* kwcElement;

	// Parcours des classes du domaine
	lHash = HashValue(GetName());
	for (nClass = 0; nClass < GetClassNumber(); nClass++)
	{
		kwcElement = GetClassAt(nClass);
		lHash = LongintUpdateHashValue(lHash, kwcElement->ComputeHashValue());
	}
	return lHash;
}

void KWClassDomain::Write(ostream& ost) const
{
	int i;
	KWClass* kwcElement;

	for (i = 0; i < GetClassNumber(); i++)
	{
		kwcElement = GetClassAt(i);
		assert(kwcElement->GetDomain() == this);
		ost << *kwcElement;
	}
}

const ALString KWClassDomain::GetClassLabel() const
{
	return "Dictionary domain";
}

const ALString KWClassDomain::GetObjectLabel() const
{
	return GetName();
}

void KWClassDomain::TestReadWrite(const ALString& sReadFileName, const ALString& sWriteFileName)
{
	KWClassDomain* loadDomain;
	KWClassDomain* reloadDomain;
	int nLoadAttributeNumber;
	int nReloadAttributeNumber;
	ObjectArray oaTestClasses;
	KWClass* kwcClass;
	KWAttribute* attribute;
	int i;

	// Creation des domaines
	loadDomain = new KWClassDomain;
	loadDomain->SetName(BuildDomainName("LoadTest"));
	InsertDomain(loadDomain);
	reloadDomain = new KWClassDomain;
	reloadDomain->SetName(BuildDomainName("ReloadTest"));
	InsertDomain(reloadDomain);

	// Lecture du fichier
	cout << "\nRead file " << sReadFileName << endl;
	loadDomain->ReadFile(sReadFileName);
	loadDomain->Check();

	// Renommage des classes et attributs
	cout << "\nRename dictionaries and variables" << endl;
	for (i = 0; i < loadDomain->GetClassNumber(); i++)
		oaTestClasses.Add(loadDomain->GetClassAt(i));
	for (i = 0; i < oaTestClasses.GetSize(); i++)
	{
		kwcClass = cast(KWClass*, oaTestClasses.GetAt(i));

		// Renommage de la classe
		loadDomain->RenameClass(kwcClass, "N" + kwcClass->GetName());

		// Renommage des attributs
		attribute = kwcClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			loadDomain->RenameAttribute(attribute, "N" + attribute->GetName());
			kwcClass->GetNextAttribute(attribute);
		}
	}

	// Ecriture du fichier
	cout << "\nWrite file " << sWriteFileName << endl;
	loadDomain->WriteFile(sWriteFileName);

	// Relecture du fichier
	cout << "\nRead file " << sWriteFileName << endl;
	reloadDomain->ReadFile(sWriteFileName);
	reloadDomain->Check();

	// Calcul de statistiques
	nLoadAttributeNumber = 0;
	for (i = 0; i < loadDomain->GetClassNumber(); i++)
		nLoadAttributeNumber += loadDomain->GetClassAt(i)->GetAttributeNumber();
	nReloadAttributeNumber = 0;
	for (i = 0; i < reloadDomain->GetClassNumber(); i++)
		nReloadAttributeNumber += reloadDomain->GetClassAt(i)->GetAttributeNumber();

	// Affichage des stats
	cout << "\n";
	cout << "Domain " << loadDomain->GetName() << "\t" << loadDomain->GetClassNumber() << " dictionaries"
	     << "\t" << nLoadAttributeNumber << " variables" << endl;
	cout << "Domaine " << reloadDomain->GetName() << "\t" << reloadDomain->GetClassNumber() << " dictionaries"
	     << "\t" << nReloadAttributeNumber << " variables" << endl;

	// Destruction des domaines
	DeleteDomain(loadDomain->GetName());
	DeleteDomain(reloadDomain->GetName());
}

void KWClassDomain::Test()
{
	KWClass* kwcClass1;
	KWClass* kwcClass2;
	KWClass* kwcClass3;
	KWClassDomain* kwcdClone;
	int i;
	KWClassDomain* domain;
	ALString sTmpDir;
	ALString sFileName;

	// Nettoyage
	DeleteAllDomains();

	// Creation des classes
	kwcClass1 = KWClass::CreateClass("Class1", 3, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, true, NULL);
	kwcClass2 = KWClass::CreateClass("Class2", 2, 0, 0, 1, 1, 1, 1, 1, 1, 3, 3, 0, true, kwcClass1);
	kwcClass3 = KWClass::CreateClass("Class3", 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, true, kwcClass2);
	kwcClass3->SetRoot(true);
	KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass1);
	KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass2);
	KWClassDomain::GetCurrentDomain()->InsertClass(kwcClass3);

	// Affichage
	cout << "\n\nCurrent domain\n\n";
	cout << *KWClassDomain::GetCurrentDomain();
	KWClassDomain::GetCurrentDomain()->Check();
	KWClassDomain::GetCurrentDomain()->Compile();

	// Duplication et supression d'une classe
	cout << "\n\nClone domain (RootClone) and remove dictionary Class3\n\n";
	kwcdClone = KWClassDomain::GetCurrentDomain()->Clone();
	kwcdClone->SetName(kwcdClone->GetName() + "Clone");
	kwcdClone->DeleteClass("Class3");
	kwcClass1 = kwcdClone->LookupClass(kwcClass1->GetName());
	KWClassDomain::InsertDomain(kwcdClone);
	KWClassDomain::SetCurrentDomain(kwcdClone);

	// Affichage du resultat
	KWClassDomain::GetCurrentDomain()->Check();
	KWClassDomain::GetCurrentDomain()->Compile();
	cout << *KWClassDomain::GetCurrentDomain();

	// Liste des domaines
	cout << "\n\nRename domain as RootNew\n\n";
	domain = KWClassDomain::LookupDomain("Root");
	if (domain != NULL)
		domain->SetName(domain->GetName() + "New");
	cout << "\n\nList of domains\n";
	sTmpDir = FileService::GetTmpDir();
	for (i = 0; i < KWClassDomain::GetDomainNumber(); i++)
	{
		domain = KWClassDomain::GetDomainAt(i);

		// Ecriture dans un fichier
		sFileName = FileService::BuildFilePathName(sTmpDir, domain->GetName() + ".kdic");
		cout << "\t" << domain->GetName() << "\t" << domain->GetClassNumber()
		     << " dictionaries written in temp dir" << endl;
		domain->WriteFile(sFileName);
	}

	// Lecture/ecriture des domaines
	cout << "\n\nRead/write of domains\n";
	for (i = 0; i < KWClassDomain::GetDomainNumber(); i++)
	{
		domain = KWClassDomain::GetDomainAt(i);

		// Supression des classes
		domain->DeleteAllClasses();

		// Relecture a partir du fichier
		sFileName = FileService::BuildFilePathName(sTmpDir, domain->GetName() + ".kdic");
		cout << "\t"
		     << "Read " << domain->GetName() << " from temp dir" << endl;
		domain->ReadFile(sFileName);

		// Ecriture a nouveau
		sFileName = FileService::BuildFilePathName(sTmpDir, domain->GetName() + "reload.kdic");
		cout << "\t" << domain->GetName() << "\t" << domain->GetClassNumber()
		     << " dictionaries written in temp dir" << endl;
		domain->WriteFile(sFileName);
	}
	cout << endl;

	// Nettoyage
	DeleteAllDomains();
}

int KWClassCompareName(const void* first, const void* second)
{
	KWClass* aFirst;
	KWClass* aSecond;
	int nResult;

	aFirst = cast(KWClass*, *(Object**)first);
	aSecond = cast(KWClass*, *(Object**)second);
	nResult = aFirst->GetName().Compare(aSecond->GetName());
	return nResult;
}

//////////////////////////////////////////////////////////////////////////
// Gestion des domaines

KWClassDomain* KWClassDomain::GetCurrentDomain()
{
	// S'il n'y a pas de domaine courant, on cherche le domaine "Root"
	if (kwcdCurrentDomain == NULL)
	{
		kwcdCurrentDomain = LookupDomain("Root");

		// On cree le domaine "Root" si necessaire
		if (kwcdCurrentDomain == NULL)
		{
			kwcdCurrentDomain = new KWClassDomain;
			kwcdCurrentDomain->SetName("Root");
			InsertDomain(kwcdCurrentDomain);

			// On recherche a nouveau le domaine "Root", car InsertDomain peut
			// avoir tout reinitialise
			kwcdCurrentDomain = LookupDomain("Root");
		}
	}

	return kwcdCurrentDomain;
}

void KWClassDomain::SetCurrentDomain(KWClassDomain* domain)
{
	kwcdCurrentDomain = domain;
}

KWClassDomain* KWClassDomain::LookupDomain(const ALString& sName)
{
	if (odDomains == NULL)
		return NULL;
	else
		return cast(KWClassDomain*, odDomains->Lookup(sName));
}

boolean KWClassDomain::InsertDomain(KWClassDomain* newObject)
{
	require(newObject != NULL);
	require(newObject->GetName() != "");
	require(LookupDomain(newObject->GetName()) == NULL);

	// Creation et initalisation si necessaire des containers de gestion des domaines
	if (odDomains == NULL)
	{
		kwcdCurrentDomain = NULL;
		odDomains = new ObjectDictionary;
		nCDUpdateNumber = 0;
		assert(oaDomains == NULL);
		nCDAllClassDomainsFreshness = 0;
	}

	// Insertion
	if (odDomains->Lookup(newObject->GetName()) == NULL)
	{
		odDomains->SetAt(newObject->GetName(), newObject);
		nCDUpdateNumber++;
		return true;
	}
	else
		return false;
}

boolean KWClassDomain::RemoveDomain(const ALString& sName)
{
	KWClassDomain* domainToRemove;

	if (odDomains == NULL)
		return false;
	else
	{
		// Gestion du domaine par defaut
		domainToRemove = LookupDomain(sName);
		if (domainToRemove == kwcdCurrentDomain)
			kwcdCurrentDomain = NULL;

		// Suppression du domaine
		if (odDomains->RemoveKey(sName))
		{
			nCDUpdateNumber++;

			// Nettoyage eventuel du container de domaines
			if (GetDomainNumber() == 0)
				RemoveAllDomains();
			return true;
		}
		else
			return false;
	}
}

boolean KWClassDomain::DeleteDomain(const ALString& sName)
{
	KWClassDomain* domainToDelete;

	if (odDomains == NULL)
		return false;
	else
	{
		// Gestion du domaine par defaut
		domainToDelete = LookupDomain(sName);
		if (domainToDelete == kwcdCurrentDomain)
			kwcdCurrentDomain = NULL;

		// Destruction du domaine
		if (odDomains->RemoveKey(sName))
		{
			check(domainToDelete);
			delete domainToDelete;
			nCDUpdateNumber++;

			// Nettoyage eventuel du container de domaines
			if (GetDomainNumber() == 0)
				RemoveAllDomains();
			return true;
		}
		else
			return false;
	}
}

boolean KWClassDomain::RenameDomain(KWClassDomain* domain, const ALString& sNewName)
{
	KWClassDomain* existingDomain;

	require(domain != NULL);
	require(odDomains != NULL and domain == cast(KWClassDomain*, odDomains->Lookup(domain->GetName())));

	// Test d'existence du nouveau nom
	existingDomain = cast(KWClassDomain*, odDomains->Lookup(sNewName));
	if (existingDomain != NULL)
		return false;
	// Renommage par manipulation dans le dictionnaire
	else
	{
		odDomains->RemoveKey(domain->GetName());
		domain->usName.SetValue(sNewName);
		odDomains->SetAt(domain->GetName(), domain);
		nCDUpdateNumber++;
		return true;
	}
}

int KWClassDomain::GetDomainNumber()
{
	if (odDomains == NULL)
		return 0;
	else
		return CDAllClassDomains()->GetSize();
}

KWClassDomain* KWClassDomain::GetDomainAt(int i)
{
	require(0 <= i and i < GetDomainNumber());

	return cast(KWClassDomain*, CDAllClassDomains()->GetAt(i));
}

void KWClassDomain::RemoveAllDomains()
{
	if (odDomains != NULL)
	{
		kwcdCurrentDomain = NULL;
		odDomains->RemoveAll();
		delete odDomains;
		odDomains = NULL;
		nCDUpdateNumber = 0;
		if (oaDomains != NULL)
		{
			delete oaDomains;
			oaDomains = NULL;
		}
		nCDAllClassDomainsFreshness = 0;
	}
}

void KWClassDomain::DeleteAllDomains()
{
	if (odDomains != NULL)
	{
		kwcdCurrentDomain = NULL;
		odDomains->DeleteAll();
		delete odDomains;
		odDomains = NULL;
		nCDUpdateNumber = 0;
		if (oaDomains != NULL)
		{
			delete oaDomains;
			oaDomains = NULL;
		}
		nCDAllClassDomainsFreshness = 0;
	}
}

longint KWClassDomain::GetAllDomainsUsedMemory()
{
	longint lUsedMemory;

	lUsedMemory = 0;
	if (odDomains != NULL)
	{
		lUsedMemory += odDomains->GetUsedMemory();
		lUsedMemory += oaDomains->GetOverallUsedMemory();
	}
	return lUsedMemory;
}

const ALString KWClassDomain::BuildDomainName(const ALString& sPrefix)
{
	ALString sDomainName;
	ALString sSuffix;
	int nIndex;

	// Recherche d'un nom inutilise en suffixant par un index croissant
	nIndex = 1;
	sDomainName = sPrefix;
	if (odDomains != NULL)
	{
		while (odDomains->Lookup(sDomainName) != NULL)
		{
			sSuffix = IntToString(nIndex);
			sDomainName = sPrefix + sSuffix;

			// Reduction si necessaire de la taille du nom
			if (sDomainName.GetLength() > KWClass::GetNameMaxLength())
			{
				sDomainName = KWClass::BuildUTF8SubString(sDomainName.Left(KWClass::GetNameMaxLength() -
											   sSuffix.GetLength() - 1)) +
					      "_" + sSuffix;
			}

			// Passage au nom suivant
			nIndex++;
		}
	}
	return sDomainName;
}

ObjectArray* KWClassDomain::CDAllClassDomains()
{
	// Creation et initialisation si necessaires des containers de gestion des domaines
	if (odDomains == NULL)
	{
		kwcdCurrentDomain = NULL;
		odDomains = new ObjectDictionary;
		nCDUpdateNumber = 0;
		assert(oaDomains == NULL);
		nCDAllClassDomainsFreshness = 0;
	}

	// Initialisation du tableau a partir du dictionnaire
	if (nCDAllClassDomainsFreshness < nCDUpdateNumber or oaDomains == NULL)
	{
		if (oaDomains != NULL)
			delete oaDomains;
		oaDomains = new ObjectArray;
		odDomains->ExportObjectArray(oaDomains);
		oaDomains->SetCompareFunction(KWClassDomainCompareName);
		oaDomains->Sort();
		nCDAllClassDomainsFreshness = nCDUpdateNumber;
	}
	return oaDomains;
}

int KWClassDomainCompareName(const void* first, const void* second)
{
	KWClassDomain* aFirst;
	KWClassDomain* aSecond;
	int nResult;

	aFirst = cast(KWClassDomain*, *(Object**)first);
	aSecond = cast(KWClassDomain*, *(Object**)second);
	nResult = aFirst->GetName().Compare(aSecond->GetName());
	return nResult;
}

KWClassDomain* KWClassDomain::kwcdCurrentDomain = NULL;
ObjectDictionary* KWClassDomain::odDomains = NULL;
int KWClassDomain::nCDUpdateNumber = 0;
ObjectArray* KWClassDomain::oaDomains = NULL;
int KWClassDomain::nCDAllClassDomainsFreshness = 0;
