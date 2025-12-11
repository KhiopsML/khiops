// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMTDatabase.h"

KWMTDatabase::KWMTDatabase()
{
	nSkippedRecordNumber = 0;
	mappingManager.InitializeMainMapping(new KWMTDatabaseMapping);
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
	assert(mappingManager.IsMainMappingInitialized());
	DMTMPhysicalTerminateMapping(GetMainMapping());
	for (nReference = 0; nReference < mappingManager.GetExternalRootMappingNumber(); nReference++)
	{
		referenceMapping = mappingManager.GetExternalRootMappingAt(nReference);
		DMTMPhysicalTerminateMapping(referenceMapping);
	}
	assert(objectReferenceResolver.GetClassNumber() == 0);

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

	// Copie standard
	KWDatabase::CopyFrom(kwdSource);
	lExternalTablesEncodingErrorNumber = 0;
	nSkippedRecordNumber = 0;

	// Copie des parametres du driver
	dataTableDriverCreator->CopyFrom(kwmtdSource->dataTableDriverCreator);

	// Copie du mapping
	mappingManager.CopyFrom(&kwmtdSource->mappingManager);
	assert(mappingManager.IsMainMappingInitialized());

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

	require(kwdSource != NULL);

	// Comparaison de base
	nCompare = KWDatabase::Compare(kwdSource);

	// Comparaison du driver
	if (nCompare == 0)
		nCompare = dataTableDriverCreator->Compare(kwmtdSource->dataTableDriverCreator);

	// Comparaison du mapping
	if (nCompare == 0)
		nCompare = mappingManager.Compare(&kwmtdSource->mappingManager);
	return nCompare;
}

void KWMTDatabase::SetDatabaseName(const ALString& sValue)
{
	assert(mappingManager.IsMainMappingInitialized());
	KWDatabase::SetDatabaseName(sValue);
	GetMainMapping()->SetDataTableName(sValue);
	nFreshness++;
}

const ALString& KWMTDatabase::GetDatabaseName() const
{
	assert(mappingManager.IsMainMappingInitialized());
	return GetMainMapping()->GetDataTableName();
}

void KWMTDatabase::SetClassName(const ALString& sValue)
{
	assert(mappingManager.IsMainMappingInitialized());

	KWDatabase::SetClassName(sValue);
	GetMainMapping()->SetClassName(sValue);
	GetMainMapping()->SetOriginClassName(sValue);
	nFreshness++;
	ensure(GetClassName() == sValue);
}

const ALString& KWMTDatabase::GetClassName() const
{
	assert(mappingManager.IsMainMappingInitialized());
	return GetMainMapping()->GetClassName();
}

ObjectArray* KWMTDatabase::GetMultiTableMappings()
{
	assert(mappingManager.IsMainMappingInitialized());
	return mappingManager.GetMappings();
}

int KWMTDatabase::GetTableNumber() const
{
	return mappingManager.GetMappingNumber();
}

int KWMTDatabase::GetMainTableNumber() const
{
	int nMainTableNumber;
	KWMTDatabaseMapping* mapping;
	int i;

	// Parcours de la table de mapping
	nMainTableNumber = 0;
	for (i = 0; i < mappingManager.GetMappingNumber(); i++)
	{
		mapping = mappingManager.GetMappingAt(i);

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
	return mappingManager.LookupMapping(sDataPath);
}

boolean KWMTDatabase::IsReferencedClassMapping(const KWMTDatabaseMapping* mapping) const
{
	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping or not Check());
	return (mapping->GetExternalTable());
}

void KWMTDatabase::UpdateMultiTableMappings()
{
	const boolean bTrace = false;
	static ALString sLastUpdatedClassName;
	KWClass* mainClass;
	KWMTDatabaseMappingManager previousMappingManager;
	KWMTDatabaseMapping* previousMainMultiTableMapping;
	ObjectArray oaPreviousMultiTableMappings;
	ObjectDictionary odReferenceClasses;
	ObjectArray oaRankedReferenceClasses;
	ObjectDictionary odAnalysedCreatedClasses;
	ObjectDictionary odWorkingReferenceClasses;
	ObjectArray oaWorkingRankedReferenceClasses;
	ObjectDictionary odWorkingAnalysedCreatedClasses;
	ObjectArray oaWorkingCreatedMappings;
	StringVector svAttributeName;
	KWMTDatabaseMapping* mapping;
	KWMTDatabaseMapping* previousMapping;
	ObjectArray oaAllRootCreatedMappings;
	int i;
	int j;
	ALString sWarning;

	require(mappingManager.IsMainMappingInitialized());
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());

	// Nettoyage prealable du mapping physique
	DMTMPhysicalTerminateMapping(GetMainMapping());

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
		previousMainMultiTableMapping = cast(KWMTDatabaseMapping*, GetMainMapping()->Clone());

		// Reinitialisation du mapping, en ne conservant que le mapping principal
		mappingManager.Reset();
		mappingManager.InitializeMainMapping(previousMainMultiTableMapping);
	}
	else
	{
		// Memorisation des mapping precedents
		previousMappingManager.CopyFrom(&mappingManager);

		// Calcul des nouveaux mappings
		mappingManager.ComputeAllDataPaths(mainClass);

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
		for (i = 0; i < previousMappingManager.GetMappingNumber(); i++)
		{
			previousMapping = cast(KWMTDatabaseMapping*, previousMappingManager.GetDataPathAt(i));

			// Recherche du nouveau mapping correspondant
			// Attention, les data paths qui servent d'identifiants aux mappings peuvent etres contextuels
			// dans le cas de tables externes
			// Par exemple, le dictionnaire principal est systematique identifie par un data path vide,
			// et si on en change, comme par exemple en passant a une table externe, c'est ce
			// dictionnaire Root associe a la table externe qui sera identifie par un data path vide.
			// Il faut donc parcourir se base sur la comparaison du dictionnaire et des attribut du mapping
			// pour faire l'appariement correct
			for (j = 0; j < mappingManager.GetMappingNumber(); j++)
			{
				mapping = mappingManager.GetMappingAt(j);

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
	}

	// Mise a jour de la fraicheur
	nFreshness++;

	// Affichage des mappings finaux
	if (bTrace)
		mappingManager.Write(cout);
	ensure(mappingManager.GetMainClassName() == "" or mainClass == NULL or mappingManager.GetDataPathNumber() > 0);
	ensure(mappingManager.GetMainClassName() == "" or mainClass == NULL or mainClass->IsKeyBasedStorable() or
	       mappingManager.GetUnusedRootDictionaryWarnings()->GetSize() > 0);
	ensure(mappingManager.IsMainMappingInitialized());
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
	ALString sOriginLabel;
	ALString sAttributeName;
	KWMTDatabaseMapping parentMapping;
	KWMTDatabaseMappingManager checkMappingManager;
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
		assert(mappingManager.IsMainMappingInitialized());
		assert(mappingManager.GetMainClassName() != "");
		assert(mappingManager.GetMainClassName() == GetClassName());

		// Verification du mapping
		bOk = mappingManager.CheckPartially(bWriteOnly);
	}

	// Erreur si mapping en trop, warning en cas d'absence de specifications de mapping,
	if (bOk)
	{
		// Calcul de mappings de verification
		checkMappingManager.ComputeAllDataPaths(
		    KWClassDomain::GetCurrentDomain()->LookupClass(mappingManager.GetMainClassName()));

		// Verification qu'il n'y a pas de mapping en trop
		for (nMapping = 0; nMapping < mappingManager.GetMappingNumber(); nMapping++)
		{
			mapping = mappingManager.GetMappingAt(nMapping);

			// Recherche du mapping correspondant, puis verification
			checkMapping = checkMappingManager.LookupMapping(mapping->GetDataPath());
			if (checkMapping == NULL)
			{
				AddError("Unexpected data path " + mapping->GetObjectLabel());
				bOk = false;
				break;
			}
		}

		// Parcours des mappings pour verifier le parametrage des tables associees a chaque mapping
		// Uniquement en lecture: en ecriture, tout ou partie des mappings peut ne pas etre renseigne
		if (bOk and not bWriteOnly)
		{
			for (nMapping = 0; nMapping < checkMappingManager.GetMappingNumber(); nMapping++)
			{
				checkMapping = checkMappingManager.GetMappingAt(nMapping);

				// Recherche du mapping correspondant, puis verification
				mapping = mappingManager.LookupMapping(checkMapping->GetDataPath());
				if (mapping == NULL)
				{
					AddError("Data path " + checkMapping->GetObjectLabel() +
						 " : Missing data path specification");
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
	for (n = 0; n < mappingManager.GetUnusedRootDictionaryWarnings()->GetSize(); n++)
		AddWarning(mappingManager.GetUnusedRootDictionaryWarnings()->GetAt(n));
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
	for (i = 0; i < mappingManager.GetMappingNumber(); i++)
	{
		mapping = mappingManager.GetMappingAt(i);
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
	for (i = 0; i < mappingManager.GetMappingNumber(); i++)
	{
		mapping = mappingManager.GetMappingAt(i);
		if (mapping->GetDataTableDriver() != NULL)
			mapping->GetDataTableDriver()->SetSilentMode(bValue);
	}
}

longint KWMTDatabase::GetUsedMemory() const
{
	boolean bShowMemory = false;
	longint lUsedMemory;

	// Methode ancetre
	lUsedMemory = KWDatabase::GetUsedMemory();

	// Specialisation
	lUsedMemory += sizeof(KWDataTableDriver*);
	if (dataTableDriverCreator != NULL)
		lUsedMemory += dataTableDriverCreator->GetUsedMemory();
	lUsedMemory += mappingManager.GetUsedMemory() - sizeof(KWMTDatabaseMappingManager);

	// Memoire utilisee par les objets references
	lUsedMemory += objectReferenceResolver.GetUsedMemory() - sizeof(KWObjectReferenceResolver);

	// Affichage des stats memoire
	if (bShowMemory)
	{
		cout << "KWMTDatabase::GetUsedMemory, base memory: " << KWDatabase::GetUsedMemory() << endl;
		cout << "KWMTDatabase::GetUsedMemory, mapping manager memory: " << mappingManager.GetUsedMemory()
		     << endl;
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
	DMTMPhysicalTerminateMapping(GetMainMapping());

	// Initialisation recursive du mapping a partir de la table principale pour avoir des driver initialises
	if (bRead)
	{
		// En lecture, on utilise la classe physique
		check(kwcPhysicalClass);
		DMTMPhysicalInitializeMapping(GetMainMapping(), kwcPhysicalClass, true);
	}
	else
	{
		// En ecriture, on utile la classe logique
		check(kwcClass);
		DMTMPhysicalInitializeMapping(GetMainMapping(), kwcClass, false);
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

	// Nettoyage final
	DMTMPhysicalTerminateMapping(GetMainMapping());
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
		lEncodingErrorNumber = DMTMPhysicalComputeEncodingErrorNumber(GetMainMapping());

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
	longint lRemainingMemory;
	longint lExternalTableUsedMemory;
	longint lTotalExternalObjectNumber;
	boolean bMemoryLimitReached;

	require(Check());
	require(CheckObjectConsistency());

	// Nettoyage prealable
	DMTMPhysicalTerminateMapping(GetMainMapping());
	nSkippedRecordNumber = 0;

	// Ouverture si Ok
	if (bOk)
	{
		// Initialisation recursive du mapping a partir de la table principale
		DMTMPhysicalInitializeMapping(GetMainMapping(), kwcPhysicalClass, true);

		// Ouverture recursive des tables a partir de la table principale
		if (bOk)
			bOk = DMTMPhysicalOpenForRead(GetMainMapping(), kwcClass);

		// Ouverture des tables referencees
		if (bOk)
		{
			// On exploite toute la memoire disponible pour lire les table externes
			// En principe, le dimensionnement a ete effectue prealablement
			lRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();

			// La lecture peut echouer pour des raisons de dimensionnement ou autres
			bOk = PhysicalReadAllReferenceObjects(lRemainingMemory, lExternalTableUsedMemory,
							      lTotalExternalObjectNumber, bMemoryLimitReached);
		}
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
	DMTMPhysicalTerminateMapping(GetMainMapping());
	nSkippedRecordNumber = 0;

	// Initialisation recursive du mapping a partir de la table principale
	DMTMPhysicalInitializeMapping(GetMainMapping(), kwcClass, false);

	// Ouverture recursive des tables a partir de la table principale
	bOk = DMTMPhysicalOpenForWrite(GetMainMapping());
	return bOk;
}

boolean KWMTDatabase::IsPhysicalEnd() const
{
	// Test de fin de la table principale
	return GetMainMapping()->GetDataTableDriver()->IsEnd();
}

KWObject* KWMTDatabase::PhysicalRead()
{
	KWObject* kwoObject;

	// Lecture d'un enregistrement de la table principale
	kwoObject = DMTMPhysicalRead(GetMainMapping());

	// Prise en compte dans le memory guard
	if (kwoObject != NULL)
	{
		// Nettoyage des objets natifs inclus si le memory guard a detecte un depassement de limite memoire
		if (memoryGuard.IsMemoryLimitReached())
			kwoObject->CleanNativeRelationAttributes();
	}

	// Lecture apres la fin de la base pour effectuer des controles
	if (GetMainMapping()->GetDataTableDriver()->IsEnd())
		PhysicalReadAfterEndOfDatabase();
	return kwoObject;
}

void KWMTDatabase::PhysicalReadAfterEndOfDatabase()
{
	KWMTDatabaseMapping* componentMapping;
	int i;
	KWDataTableDriver* mappedDataTableDriver;
	const KWObjectDataPath* objectDataPath;
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
	bIsError = bIsError or GetMainMapping()->GetDataTableDriver()->IsError();

	// Lecture de chaque sous-base jusqu'a la fin pour detecter les erreurs
	for (i = 1; i < mappingManager.GetMappingNumber(); i++)
	{
		componentMapping = mappingManager.GetMappingAt(i);

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

			// Recherche du data path correspondant au mapping
			objectDataPath = objectDataPathManager.LookupObjectDataPath(componentMapping->GetDataPath());

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

				// Parametrage du data path de l'objet
				if (kwoSubObject != NULL)
				{
					assert(kwoSubObject->GetClass() != kwcPhysicalClass);
					kwoSubObject->SetObjectDataPath(objectDataPath);
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
	GetMainMapping()->GetDataTableDriver()->Skip();
	bIsError = bIsError or GetMainMapping()->GetDataTableDriver()->IsError();

	// Memorisation inconditionnelle de la cle du dernier enregistremnt lu, dans le cas d'une classe principale,
	// meme si l'objet n'a pas pu etre lu
	// Cela permet de gere les lignes dupliquees, que l'objet soit lu ou non (a cause d'une erreur de parsing)
	assert(GetMainMapping()->GetDataTableDriver()->GetClass()->GetName() == kwcClass->GetName());
	assert(GetMainMapping()->GetDataTableDriver()->GetClass()->GetRoot() == kwcClass->GetRoot());
	if (kwcClass->IsUnique())
	{
		assert(GetMainMapping()->GetDataTableDriver()->GetLastReadMainKey()->GetSize() ==
		       GetMainMapping()->GetDataTableDriver()->GetClass()->GetKeyAttributeNumber());
		GetMainMapping()->SetLastReadKey(GetMainMapping()->GetDataTableDriver()->GetLastReadMainKey());
	}

	// Attention, il n'est pas possible de propager les skip sur les sous-tables
	// Consequence: pas de warning quand un objet d'une sous-table a une cle superieure a celle de l'objet
	//  principal courant
	// Memorisation du nombre d'enregistrement sautes pour gerer ce cas
	nSkippedRecordNumber++;

	// Lecture apres la fin de la base pour effectuer des controles
	if (not bIsError and GetMainMapping()->GetDataTableDriver()->IsEnd())
		PhysicalReadAfterEndOfDatabase();
}

void KWMTDatabase::PhysicalWrite(const KWObject* kwoObject)
{
	// Ecriture d'un enregistrement de la table principale
	DMTMPhysicalWrite(GetMainMapping(), kwoObject);
}

boolean KWMTDatabase::PhysicalClose()
{
	boolean bOk;

	// Fermeture de la base et de toutes ses sous-bases
	bOk = DMTMPhysicalClose(GetMainMapping());
	nSkippedRecordNumber = 0;

	// Destruction des objets references
	PhysicalDeleteAllReferenceObjects();
	return bOk;
}

void KWMTDatabase::PhysicalDeleteDatabase()
{
	// Destruction des tables de la hierarchie principale, hors classe referencees
	DMTMPhysicalDeleteDatabase(GetMainMapping());
}

longint KWMTDatabase::GetPhysicalEstimatedObjectNumber()
{
	longint lPhysicalEstimatedObjectNumber;

	require(GetMainMapping()->GetDataTableDriver() == NULL);

	// Parametrage du mapping principal
	GetMainMapping()->SetDataTableDriver(CreateDataTableDriver(GetMainMapping()));
	GetMainMapping()->GetDataTableDriver()->SetDataTableName(GetMainMapping()->GetDataTableName());
	GetMainMapping()->GetDataTableDriver()->SetClass(
	    KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()));

	// Appel de la methode du driver
	lPhysicalEstimatedObjectNumber = GetMainMapping()->GetDataTableDriver()->GetEstimatedObjectNumber();

	// Nettoyage (attention a nettoyer le driver avant de le detruire)
	GetMainMapping()->GetDataTableDriver()->SetDataTableName("");
	GetMainMapping()->GetDataTableDriver()->SetClass(NULL);
	delete GetMainMapping()->GetDataTableDriver();
	GetMainMapping()->SetDataTableDriver(NULL);
	return lPhysicalEstimatedObjectNumber;
}

double KWMTDatabase::GetPhysicalReadPercentage() const
{
	require(GetMainMapping()->GetDataTableDriver() != NULL);
	return GetMainMapping()->GetDataTableDriver()->GetReadPercentage();
}

longint KWMTDatabase::GetPhysicalRecordIndex() const
{
	if (GetMainMapping()->GetDataTableDriver() != NULL)
		return GetMainMapping()->GetDataTableDriver()->GetRecordIndex();
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
	for (i = 1; i < mappingManager.GetMappingNumber(); i++)
	{
		mapping = mappingManager.GetMappingAt(i);

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
	for (i = 1; i < mappingManager.GetMappingNumber(); i++)
	{
		mapping = mappingManager.GetMappingAt(i);

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
	if (kwcPhysicalClass->GetRoot() and not GetMainMapping()->GetLastReadKey()->IsEmpty())
		objectReferenceResolver.AddObject(kwcPhysicalClass, GetMainMapping()->GetLastReadKey(),
						  kwoPhysicalObject);

	// Appel de la methode ancetre
	KWDatabase::MutatePhysicalObject(kwoPhysicalObject);

	// Dereferencement du precedent objet principal lu depuis le resolveur de reference
	// En effet, l'objet precedement lu est potentiellement detruit et inutilisable pour
	// la resolution des references (intra-objet dans le cas de l'objet principal)
	if (kwcPhysicalClass->GetRoot() and not GetMainMapping()->GetLastReadKey()->IsEmpty())
		objectReferenceResolver.RemoveObject(kwcPhysicalClass, GetMainMapping()->GetLastReadKey());

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
		if (kwcPhysicalClass->GetRoot() and not GetMainMapping()->GetLastReadKey()->IsEmpty())
			objectReferenceResolver.AddObject(kwcPhysicalClass, GetMainMapping()->GetLastReadKey(),
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
		if (kwcPhysicalClass->GetRoot() and not GetMainMapping()->GetLastReadKey()->IsEmpty())
			objectReferenceResolver.RemoveObject(kwcPhysicalClass, GetMainMapping()->GetLastReadKey());

		// Remise a NULL du resolveur de reference dans la regle de derivation gerant les references
		KWDRReference::SetObjectReferenceResolver(NULL);
	}
	return bSelected;
}

boolean KWMTDatabase::PhysicalReadAllReferenceObjects(longint lMaxAvailableMemory, longint& lNecessaryMemory,
						      longint& lTotalExternalObjectNumber, boolean& bMemoryLimitReached)
{
	const boolean bTrace = false;
	boolean bOk = true;
	const int lMaxSkippedExternalRecordNumber = 1000000;
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
	KWDatabaseMemoryGuard initialMemoryGuard;
	Timer timer;

	require(objectReferenceResolver.GetClassNumber() == 0);
	require(kwcPhysicalClass != NULL);
	require(kwcPhysicalClass->GetName() == GetClassName());
	require(lMaxAvailableMemory >= 0);

	// Trace initiale
	if (bTrace)
	{
		cout << "\t  PhysicalReadAllReferenceObjects\t" << LongintToHumanReadableString(lMaxAvailableMemory)
		     << "\n";
		cout << "\t\tInitial heap memory\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\n";
		timer.Start();
	}

	// On desactive temporairement le memory guard, le temps de la lecture des objets references
	// En effet, le dimensionnement est calcule pour pouvoir charger en memoire l'ensemble de toutes
	// les tables externes, et il n'y a pas a se soucier de la gestion des instances "elephants"
	initialMemoryGuard.CopyFrom(&memoryGuard);
	memoryGuard.Reset();

	// On le reparametre specifiquement pour la lecture des tables externes
	// en exploitant les parametres lies a la limite memoire globale
	memoryGuard.Reset();
	memoryGuard.SetEstimatedMinMemoryLimit(lMaxAvailableMemory);
	memoryGuard.SetEstimatedMaxMemoryLimit(lMaxAvailableMemory);
	memoryGuard.SetMemoryLimit(lMaxAvailableMemory);
	memoryGuard.SetCrashTestMemoryLimit(initialMemoryGuard.GetCrashTestMemoryLimit());

	// Initialisation du memory guard pour la lecture de l'ensemble de toutes les tables externes
	memoryGuard.Init();

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

	// Ouverture de chaque table secondaire
	for (nReference = 0; nReference < mappingManager.GetExternalRootMappingNumber(); nReference++)
	{
		referenceMapping = mappingManager.GetExternalRootMappingAt(nReference);

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
					// Lecture d'un enregistrement de la table principale si on n'a pas atteint la limite memoire
					if (not memoryGuard.IsMemoryLimitReached())
						kwoObject = DMTMPhysicalRead(referenceMapping);
					// Sinon, on saute l'enregistrement pour colelcter des stats sur le nombre total de record
					else
					{
						kwoObject = NULL;

						// On arrete la collecte de stats si top de skip, pour eviter de lire un fichier trop gros
						if (memoryGuard.GetTotalReadExternalRecordNumber() -
							memoryGuard.GetReadExternalRecordNumberBeforeLimit() >=
						    lMaxSkippedExternalRecordNumber)
							break;
						// Sinon, on saute l'enregistrement
						else
							referenceMapping->GetDataTableDriver()->Skip();
					}
					lRecordNumber++;

					// Prise en compte dans le memory guard
					memoryGuard.AddReadExternalRecord();

					// Nettoyage si limite memoire atteinte
					if (kwoObject != NULL and memoryGuard.IsMemoryLimitReached())
					{
						// Destruction si on a atteint la limite memoire
						delete kwoObject;
						kwoObject = NULL;
					}

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

					// Suivi de la tache
					if (bIsInTask and TaskProgression::IsRefreshNecessary(lRecordNumber))
					{
						TaskProgression::DisplayLabel(sMessage + ": " +
									      LongintToReadableString(lObjectNumber) +
									      " records");
						TaskProgression::DisplayProgression(int(
						    100 * referenceMapping->GetDataTableDriver()->GetReadPercentage()));
					}
				}

				// Trace pour la premiere passe
				if (bTrace)
					cout << "\t\t" << referenceMapping->GetDataTableName() << "\t"
					     << BooleanToString(bOk) << "\t" << lObjectNumber << "\t"
					     << LongintToHumanReadableString(memoryGuard.GetCurrentUsedHeapMemory())
					     << "\n";

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

			// Erreur si probleme memoire
			if (memoryGuard.IsMemoryLimitReached())
				bOk = false;
		}

		// Nettoyage final
		DMTMPhysicalTerminateMapping(referenceMapping);

		// Arret si erreurs
		if (not bOk)
			break;
	}

	// Trace intermediaire apres la phase de lecture
	if (bTrace)
	{
		cout << "\t\tNecessary time for read\t" << timer.GetElapsedTime() << "\n";
		cout << "\t\tHeap memory after read\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\n";
	}

	// Reinitialisation des index de creation d'instances pour les objets des tables externes
	//
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
	//
	// Attention : apres l'analyse du graphe de calcul complet, seules les variables utiles des tables externes
	// sont calculees si elles sont exploitees dans la classe physique. Si ces variables utilisent la regle Random,
	// cela peut forcer un ordre de calcul qui varie selon qu'elles soient exploitees directement via le
	// flocon principal ou indirectement via des variables de tables externes. Cela peut entrainer la creation
	// d'instances dans un ordre different (par exemple, en commencant a creer des instances dans la deuxieme table externe
	// pour le calcul des valeurs de la premiere table externe). Cela entraine des index de creation d'instances
	// differents, et par consequent, des suites de valeurs aleatoires differentes (puisqu'elles exploitent l'index
	// de creation d'instance).
	// Il vaut mieux donc eviter d'utiliser la regle Random dans les tables externes
	for (nReference = 0; nReference < objectDataPathManager.GetExternalRootDataPathNumber(); nReference++)
		objectDataPathManager.GetExternalRootObjectDataPathAt(nReference)->ResetCreationNumber(0);

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
		// Attention, cet ordre est necessaire pour garantir la reproductibilite des resultats,
		// notamment en ce qui concernes les objets issus d'une regle de derivation de creation d'instance
		// dont l'index de creation est local a l'ensemble de toutes les instances
		for (nObject = 0; nObject < oaAllReferenceObjects.GetSize(); nObject++)
		{
			kwoObject = cast(KWObject*, oaAllReferenceObjects.GetAt(nObject));
			assert(kwoObject->GetClass()->GetDomain() == kwcPhysicalClass->GetDomain());

			// Calcul recursif de toutes les valeurs des objets
			kwoObject->ComputeAllValues(&memoryGuard);

			// Arret si probleme memoire
			if (memoryGuard.IsMemoryLimitReached())
			{
				bOk = false;
				break;
			}

			// Prise en compte dans le memory guard pour les stats
			memoryGuard.AddComputedExternalRecord();

			// Suivi de la tache
			if (bIsInTask and TaskProgression::IsRefreshNecessary(nObject))
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

	// Memorisation de la memoire utilise, en repassant en memoire logique
	lNecessaryMemory = memoryGuard.GetCurrentUsedHeapMemory();
	lNecessaryMemory = longint(lNecessaryMemory / (1 + MemGetAllocatorOverhead()));

	// Erreur utilisateur en cas de depassement memoire
	bMemoryLimitReached = memoryGuard.IsMemoryLimitReached();
	if (memoryGuard.IsMemoryLimitReached())
	{
		bOk = false;
		AddError(memoryGuard.GetExternalTableMemoryLimitLabel());
	}

	// Nombre total d'enregistrement lus ou crees
	lTotalExternalObjectNumber = memoryGuard.GetTotalReadExternalRecordNumber() +
				     memoryGuard.GetTotalReadSecondaryRecordNumber() +
				     memoryGuard.GetTotalCreatedRecordNumber();

	// Trace finale
	if (bTrace)
	{
		timer.Stop();
		cout << "\t\tFinal heap memory\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\n";
		cout << "\t\tRoot record number\t" << memoryGuard.GetTotalReadExternalRecordNumber() << "\n";
		cout << "\t\tSecondary record number\t" << memoryGuard.GetTotalReadSecondaryRecordNumber() << "\n";
		cout << "\t\tCreated record number\t" << memoryGuard.GetTotalCreatedRecordNumber() << "\n";
		cout << "\t\tTotal record number\t" << lTotalExternalObjectNumber << "\n";
		cout << "\t\tNecessary time\t" << timer.GetElapsedTime() << "\n";
		cout << "\t\tNecessary memory\t" << LongintToHumanReadableString(lNecessaryMemory) << "\n";
		cout << "\t\tOk\t" << BooleanToString(bOk) << "\n";
	}

	// On reactive le memory guard initial
	memoryGuard.CopyFrom(&initialMemoryGuard);

	ensure(lNecessaryMemory >= 0);
	ensure(not bOk or lNecessaryMemory <= lMaxAvailableMemory);
	return bOk;
}

void KWMTDatabase::PhysicalDeleteAllReferenceObjects()
{
	objectReferenceResolver.DeleteAll();
}

boolean KWMTDatabase::ComputeExactNecessaryMemoryForReferenceObjects(longint& lNecessaryMemory,
								     longint& lTotalExternalObjectNumber)
{
	const boolean bTrace = GetPreparationTraceMode();
	boolean bOk = true;
	longint lHeapMemory;
	longint lReadBasedNecessaryMemory;
	longint lDeleteBasedNecessaryMemory;
	longint lEstimatedNecessaryMemory;
	longint lRemainingMemory;
	boolean bMemoryLimitReached;
	boolean bCurrentVerboseMode;

	require(Check());
	require(kwcClass != NULL);
	require(kwcPhysicalClass != NULL);
	require(not IsOpenedForRead());
	require(not IsOpenedForWrite());

	// Premiere passe basique d'estimation de la memoire necessaire, sans acces aux donnees
	lEstimatedNecessaryMemory = ComputeEstimatedNecessaryMemoryForReferenceObjects();
	if (bTrace)
	{
		cout << "Necessary memory for reference objects for database " << GetDatabaseName() << endl;
		cout << "\tIn memory estimation\t" << LongintToHumanReadableString(lEstimatedNecessaryMemory) << endl;
	}

	// Estimation de la memoire totale restante
	lRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();
	if (bTrace)
		cout << "\tRemaining memory: " << LongintToHumanReadableString(lRemainingMemory) << endl;

	// Erreur s'il manque le double de la memoire, pour etre conservateur vis a vis de l'estimation heuristique
	if (lEstimatedNecessaryMemory > 2 * lRemainingMemory)
	{
		bOk = false;
		AddError("Not enough memory to load external tables" +
			 RMResourceManager::BuildMissingMemoryMessage(lEstimatedNecessaryMemory - lRemainingMemory));
	}
	// Calcul exact par chargement de la base sinon
	else
	{
		// Initialisation de tous les data paths a destination des objets lus ou crees
		// le temps de la lecture des objet externes
		objectDataPathManager.ComputeAllDataPaths(kwcPhysicalClass);

		// Ouverture et lecture des tables referencees, sans emission d'erreur
		// ce qui permet de calculer la memoire necessaire a leur lecture
		// On evite les warnings dans cette passe de dimensionnement, car ils seront emis lors
		// de la lecture effective des tables externes a l'ouverture de la base
		bCurrentVerboseMode = GetVerboseMode();
		SetVerboseMode(false);
		bOk = PhysicalReadAllReferenceObjects(lRemainingMemory, lReadBasedNecessaryMemory,
						      lTotalExternalObjectNumber, bMemoryLimitReached);
		SetVerboseMode(bCurrentVerboseMode);
		if (bTrace)
		{
			cout << "\tUsed memory for read\t" << LongintToHumanReadableString(lReadBasedNecessaryMemory)
			     << "\n";
			cout << "\tTotal external object number\t"
			     << LongintToReadableString(lTotalExternalObjectNumber) << "\n";
		}

		// Destruction des objets references
		// ce qui permet de calculer la memoire necessaire a leur destruction
		lHeapMemory = MemGetHeapMemory();
		PhysicalDeleteAllReferenceObjects();
		lDeleteBasedNecessaryMemory = lHeapMemory - MemGetHeapMemory();
		lDeleteBasedNecessaryMemory = longint(lDeleteBasedNecessaryMemory / (1 + MemGetAllocatorOverhead()));
		if (bTrace)
			cout << "\tUsed memory for delete\t"
			     << LongintToHumanReadableString(lDeleteBasedNecessaryMemory) << "\n";

		// Memoire dediee aux objets references
		// On prend le max des deux, car il peut y avoir des effet de bord dans l'allocateur,
		// avec la gestion des segments memoire
		lNecessaryMemory = max(lReadBasedNecessaryMemory, lDeleteBasedNecessaryMemory);
		assert(lNecessaryMemory >= 0);
		if (bTrace)
			cout << "\tExact based estimation\t" << LongintToHumanReadableString(lNecessaryMemory) << endl;

		// Destruction des data paths de gestion des objets
		objectDataPathManager.Reset();
	}
	ensure(lNecessaryMemory >= 0);
	return bOk;
}

longint KWMTDatabase::ComputeEstimatedNecessaryMemoryForReferenceObjects()
{
	const boolean bTrace = false;
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

	// Ouverture de chaque table secondaire pour une premiere estimation basique de la memoire necessaire,
	// sans acces aux donnees
	if (bTrace)
		cout << "ComputeEstimatedNecessaryMemoryForReferenceObjects\n";
	lTotalNecessaryMemory = 0;
	for (nReference = 0; nReference < mappingManager.GetExternalRootMappingNumber(); nReference++)
	{
		referenceMapping = mappingManager.GetExternalRootMappingAt(nReference);

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
		referenceMapping->CollectFullHierarchyComponents(&oaFullHierarchyComponentTableMappings);

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
			if (bTrace)
				cout << "\tNecessary memory for " << mapping->GetObjectLabel() << ": "
				     << LongintToHumanReadableString(lNecessaryMemory) << endl;
		}

		// Nettoyage final
		DMTMPhysicalTerminateMapping(referenceMapping);
	}
	if (bTrace)
		cout << "\tNecessary memory for reference objects for database " << GetDatabaseName() << ": "
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
		for (i = 0; i < mapping->GetComponents()->GetSize(); i++)
		{
			componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponents()->GetAt(i));
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
	for (i = 0; i < mapping->GetComponents()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponents()->GetAt(i));
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
			for (i = 0; i < mapping->GetComponents()->GetSize(); i++)
			{
				componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponents()->GetAt(i));

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
		for (i = 0; i < mapping->GetComponents()->GetSize(); i++)
		{
			componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponents()->GetAt(i));

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
	for (i = 0; i < mapping->GetComponents()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponents()->GetAt(i));
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
	for (i = 0; i < mapping->GetComponents()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponents()->GetAt(i));
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
	for (i = 0; i < mapping->GetComponents()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponents()->GetAt(i));
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

	// Parametrage du data path de l'objet
	if (kwoObject != NULL)
	{
		// Recherche du data path a associe a l'objet
		objectDataPath = objectDataPathManager.LookupObjectDataPath(mapping->GetDataPath());
		assert(objectDataPath != NULL);

		// Memorisation du data path dans l'objet
		kwoObject->SetObjectDataPath(objectDataPath);

		// Reinitialisation des informations de gestion de creation d'instance pour tous les
		// sous data path dans le cas d'un objet princial
		if (mapping->GetDataTableDriver()->GetClass() == kwcPhysicalClass)
		{
			assert(objectDataPath == objectDataPathManager.GetMainDataPath());
			objectDataPathManager.GetMainObjectDataPath()->ResetCreationNumber(
			    kwoObject->GetCreationIndex());
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
		if (mapping == GetMainMapping() or kwoObject->GetClass()->IsUnique())
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
		for (i = 0; i < mapping->GetComponents()->GetSize(); i++)
		{
			componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponents()->GetAt(i));

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
						assert(componentMapping != GetMainMapping() and
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
							if (memoryGuard.IsMemoryLimitReached())
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
	for (i = 0; i < mapping->GetComponents()->GetSize(); i++)
	{
		componentMapping = cast(KWMTDatabaseMapping*, mapping->GetComponents()->GetAt(i));

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
