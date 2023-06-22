// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTupleTableLoader.h"

KWTupleTableLoader::KWTupleTableLoader()
{
	kwcInputClass = NULL;
	oaInputDatabaseObjects = NULL;
	nInputExtraAttributeType = KWType::Unknown;
	svInputExtraAttributeSymbolValues = NULL;
	cvInputExtraAttributeContinuousValues = NULL;
	inputExtraAttributeTupleTable = NULL;
	bCheckDatabaseObjectsClass = true;
}

KWTupleTableLoader::~KWTupleTableLoader() {}

void KWTupleTableLoader::SetInputClass(const KWClass* kwcValue)
{
	kwcInputClass = kwcValue;
}

const KWClass* KWTupleTableLoader::GetInputClass() const
{
	return kwcInputClass;
}

void KWTupleTableLoader::SetInputDatabaseObjects(const ObjectArray* oaDatabaseObjects)
{
	oaInputDatabaseObjects = oaDatabaseObjects;
}

const ObjectArray* KWTupleTableLoader::GetInputDatabaseObjects() const
{
	return oaInputDatabaseObjects;
}

void KWTupleTableLoader::SetInputExtraAttributeSymbolValues(const SymbolVector* svValues)
{
	svInputExtraAttributeSymbolValues = svValues;
}

const SymbolVector* KWTupleTableLoader::GetInputExtraAttributeSymbolValues() const
{
	return svInputExtraAttributeSymbolValues;
}

void KWTupleTableLoader::SetInputExtraAttributeContinuousValues(const ContinuousVector* cvValues)
{
	cvInputExtraAttributeContinuousValues = cvValues;
}

const ContinuousVector* KWTupleTableLoader::GetInputExtraAttributeContinuousValues() const
{
	return cvInputExtraAttributeContinuousValues;
}

void KWTupleTableLoader::SetInputExtraAttributeTupleTable(const KWTupleTable* tupleTable)
{
	inputExtraAttributeTupleTable = tupleTable;
}

const KWTupleTable* KWTupleTableLoader::GetInputExtraAttributeTupleTable() const
{
	return inputExtraAttributeTupleTable;
}

boolean KWTupleTableLoader::CheckInputs() const
{
	boolean bOk = true;
	KWAttribute* inputAttribute;

	// Verification du dictionnaire et de la base
	bOk = bOk and (kwcInputClass != NULL);
	bOk = bOk and (oaInputDatabaseObjects != NULL);
	bOk = bOk and (oaInputDatabaseObjects->GetSize() == 0 or not bCheckDatabaseObjectsClass or
		       cast(KWObject*, oaInputDatabaseObjects->GetAt(0))->GetClass() == kwcInputClass);

	// Verification si specification d'attribut supplementaire (meme vide)
	if (nInputExtraAttributeType == KWType::Unknown)
		bOk = bOk and sInputExtraAttributeName == "";
	else
	{
		// Verifications en l'absence d'attribut supplementaire
		if (bOk and sInputExtraAttributeName == "")
		{
			bOk = bOk and nInputExtraAttributeType == KWType::None;
			bOk = bOk and svInputExtraAttributeSymbolValues == NULL;
			bOk = bOk and cvInputExtraAttributeContinuousValues == NULL;
			bOk = bOk and inputExtraAttributeTupleTable != NULL;
			bOk = bOk and
			      (oaInputDatabaseObjects == NULL or oaInputDatabaseObjects->GetSize() == 0 or
			       oaInputDatabaseObjects->GetSize() == inputExtraAttributeTupleTable->GetTotalFrequency());
		}

		// Verifications en presence d'attribut supplementaire
		if (bOk and sInputExtraAttributeName != "")
		{
			// On ne verifie l'attribut cible que s'il est present dans la classe
			inputAttribute = kwcInputClass->LookupAttribute(sInputExtraAttributeName);
			if (inputAttribute != NULL)
				bOk = bOk and inputAttribute->GetType() == nInputExtraAttributeType;

			// Verification de base sur lm'attribut cible
			bOk = bOk and KWType::IsSimple(nInputExtraAttributeType);
			bOk = bOk and inputExtraAttributeTupleTable != NULL;

			// Verification des valeurs de l'attribut additionnnel
			if (bOk and nInputExtraAttributeType == KWType::Symbol)
			{
				bOk = bOk and svInputExtraAttributeSymbolValues != NULL;
				bOk =
				    bOk and
				    (oaInputDatabaseObjects == NULL or oaInputDatabaseObjects->GetSize() == 0 or
				     oaInputDatabaseObjects->GetSize() == svInputExtraAttributeSymbolValues->GetSize());
				bOk = bOk and (inputExtraAttributeTupleTable->GetTotalFrequency() ==
					       svInputExtraAttributeSymbolValues->GetSize());
			}
			else
			{
				bOk = bOk and cvInputExtraAttributeContinuousValues != NULL;
				bOk =
				    bOk and (oaInputDatabaseObjects == NULL or oaInputDatabaseObjects->GetSize() == 0 or
					     oaInputDatabaseObjects->GetSize() ==
						 cvInputExtraAttributeContinuousValues->GetSize());
				bOk = bOk and (inputExtraAttributeTupleTable->GetTotalFrequency() ==
					       cvInputExtraAttributeContinuousValues->GetSize());
			}
		}
	}
	return bOk;
}

void KWTupleTableLoader::SetCheckDatabaseObjectClass(boolean bValue)
{
	bCheckDatabaseObjectsClass = bValue;
}

boolean KWTupleTableLoader::GetCheckDatabaseObjectClass() const
{
	return bCheckDatabaseObjectsClass;
}

void KWTupleTableLoader::RemoveAllInputs()
{
	kwcInputClass = NULL;
	oaInputDatabaseObjects = NULL;
	RemoveExtraAttributeInputs();
}

void KWTupleTableLoader::RemoveExtraAttributeInputs()
{
	sInputExtraAttributeName = "";
	nInputExtraAttributeType = KWType::Unknown;
	svInputExtraAttributeSymbolValues = NULL;
	cvInputExtraAttributeContinuousValues = NULL;
	inputExtraAttributeTupleTable = NULL;
}

void KWTupleTableLoader::DeleteExtraAttributeInputs()
{
	// Destruction des objets
	if (svInputExtraAttributeSymbolValues != NULL)
		delete svInputExtraAttributeSymbolValues;
	if (cvInputExtraAttributeContinuousValues != NULL)
		delete cvInputExtraAttributeContinuousValues;
	if (inputExtraAttributeTupleTable != NULL)
		delete inputExtraAttributeTupleTable;

	// Reinitialisation
	RemoveExtraAttributeInputs();
}

void KWTupleTableLoader::CreateAllExtraInputParameters(const ALString& sExtraAttributeName)
{
	KWAttribute* extraAttribute;
	SymbolVector* svInputSymbolValues;
	ContinuousVector* cvInputContinuousValues;
	KWTupleTable* tupleTable;

	require(CheckInputs());
	require(GetInputExtraAttributeName() == "");
	require(sExtraAttributeName == "" or kwcInputClass->LookupAttribute(sExtraAttributeName) != NULL);
	require(sExtraAttributeName == "" or
		KWType::IsSimple(kwcInputClass->LookupAttribute(sExtraAttributeName)->GetType()));
	require(sExtraAttributeName == "" or kwcInputClass->LookupAttribute(sExtraAttributeName)->GetLoaded());

	// Parametrage d'un attribut supplementaire
	SetInputExtraAttributeName("");
	SetInputExtraAttributeType(KWType::None);
	extraAttribute = NULL;
	if (sExtraAttributeName != "")
	{
		extraAttribute = kwcInputClass->LookupAttribute(sExtraAttributeName);
		SetInputExtraAttributeName(extraAttribute->GetName());
		SetInputExtraAttributeType(extraAttribute->GetType());
	}

	// Chargement du vecteur de valeurs
	if (GetInputExtraAttributeType() == KWType::Symbol)
	{
		svInputSymbolValues = new SymbolVector;
		LoadSymbolValues(sExtraAttributeName, svInputSymbolValues);
		SetInputExtraAttributeSymbolValues(svInputSymbolValues);
	}
	else if (GetInputExtraAttributeType() == KWType::Continuous)
	{
		cvInputContinuousValues = new ContinuousVector;
		LoadContinuousValues(sExtraAttributeName, cvInputContinuousValues);
		SetInputExtraAttributeContinuousValues(cvInputContinuousValues);
	}

	// Chargement de la table de tuples
	tupleTable = new KWTupleTable;
	if (GetInputExtraAttributeType() == KWType::None)
		LoadTupleTableFromFrequency(oaInputDatabaseObjects->GetSize(), tupleTable);
	else if (GetInputExtraAttributeType() == KWType::Symbol)
		LoadTupleTableFromSymbolValues(sExtraAttributeName, GetInputExtraAttributeSymbolValues(), tupleTable);
	else if (GetInputExtraAttributeType() == KWType::Continuous)
		LoadTupleTableFromContinuousValues(sExtraAttributeName, GetInputExtraAttributeContinuousValues(),
						   tupleTable);
	SetInputExtraAttributeTupleTable(tupleTable);
	ensure(CheckInputs());
}

void KWTupleTableLoader::LoadContinuousValues(const ALString& sAttributeName, ContinuousVector* cvOutputValues)
{
	KWAttribute* attribute;
	KWLoadIndex liLoadIndex;
	int nObjectNumber;
	int nObject;
	KWObject* kwoObject;

	require(oaInputDatabaseObjects != NULL);
	require(cvOutputValues != NULL);

	// Acces aux caracteristique de l'attribut
	attribute = kwcInputClass->LookupAttribute(sAttributeName);
	liLoadIndex = attribute->GetLoadIndex();
	check(attribute);
	assert(attribute->GetType() == KWType::Continuous);
	assert(attribute->GetLoaded());

	// Initialisation du resultat
	nObjectNumber = oaInputDatabaseObjects->GetSize();
	cvOutputValues->SetSize(nObjectNumber);

	// Alimentation des valeurs
	for (nObject = 0; nObject < nObjectNumber; nObject++)
	{
		// Prise en compte des caracteristiques de l'objet
		kwoObject = cast(KWObject*, oaInputDatabaseObjects->GetAt(nObject));
		cvOutputValues->SetAt(nObject, kwoObject->GetContinuousValueAt(liLoadIndex));
	}
}

void KWTupleTableLoader::LoadSymbolValues(const ALString& sAttributeName, SymbolVector* svOutputValues)
{
	KWAttribute* attribute;
	KWLoadIndex liLoadIndex;
	int nObjectNumber;
	int nObject;
	KWObject* kwoObject;

	require(oaInputDatabaseObjects != NULL);
	require(svOutputValues != NULL);

	// Acces aux caracteristique de l'attribut
	attribute = kwcInputClass->LookupAttribute(sAttributeName);
	liLoadIndex = attribute->GetLoadIndex();
	check(attribute);
	assert(attribute->GetType() == KWType::Symbol);
	assert(attribute->GetLoaded());

	// Initialisation du resultat
	nObjectNumber = oaInputDatabaseObjects->GetSize();
	svOutputValues->SetSize(nObjectNumber);

	// Alimentation des valeurs
	for (nObject = 0; nObject < nObjectNumber; nObject++)
	{
		// Prise en compte des caracteristiques de l'objet
		kwoObject = cast(KWObject*, oaInputDatabaseObjects->GetAt(nObject));
		svOutputValues->SetAt(nObject, kwoObject->GetSymbolValueAt(liLoadIndex));
	}
}

void KWTupleTableLoader::LoadTupleTableFromContinuousValues(const ALString& sAttributeName,
							    const ContinuousVector* cvInputValues,
							    KWTupleTable* outputTupleTable)
{
	KWAttribute* attribute;
	int nValue;
	KWTuple* inputTuple;

	require(cvInputValues != NULL);
	require(outputTupleTable != NULL);

	// Acces aux caracteristique de l'attribut
	attribute = kwcInputClass->LookupAttribute(sAttributeName);
	check(attribute);
	assert(KWType::IsSimple(attribute->GetType()));
	assert(attribute->GetLoaded());

	// Specification de la table de tuples
	outputTupleTable->CleanAll();
	outputTupleTable->AddAttribute(attribute->GetName(), attribute->GetType());

	// Passage de la table de tuples en mode edition
	outputTupleTable->SetUpdateMode(true);
	inputTuple = outputTupleTable->GetInputTuple();

	// Alimentation a partir du vecteur de valeurs
	for (nValue = 0; nValue < cvInputValues->GetSize(); nValue++)
	{
		inputTuple->SetContinuousAt(0, cvInputValues->GetAt(nValue));
		outputTupleTable->UpdateWithInputTuple();
	}

	// Passage de la table de tuples en mode consultation
	outputTupleTable->SetUpdateMode(false);
}

void KWTupleTableLoader::LoadTupleTableFromSymbolValues(const ALString& sAttributeName,
							const SymbolVector* svInputValues,
							KWTupleTable* outputTupleTable)
{
	KWAttribute* attribute;
	int nValue;
	KWTuple* inputTuple;

	require(svInputValues != NULL);
	require(outputTupleTable != NULL);

	// Acces aux caracteristique de l'attribut
	attribute = kwcInputClass->LookupAttribute(sAttributeName);
	check(attribute);
	assert(KWType::IsSimple(attribute->GetType()));
	assert(attribute->GetLoaded());

	// Specification de la table de tuples
	outputTupleTable->CleanAll();
	outputTupleTable->AddAttribute(attribute->GetName(), attribute->GetType());

	// Passage de la table de tuples en mode edition
	outputTupleTable->SetUpdateMode(true);
	inputTuple = outputTupleTable->GetInputTuple();

	// Alimentation a partir du vecteur de valeurs
	for (nValue = 0; nValue < svInputValues->GetSize(); nValue++)
	{
		inputTuple->SetSymbolAt(0, svInputValues->GetAt(nValue));
		outputTupleTable->UpdateWithInputTuple();
	}

	// Passage de la table de tuples en mode consultation
	outputTupleTable->SetUpdateMode(false);
}

void KWTupleTableLoader::LoadTupleTableFromFrequency(int nInputFrequency, KWTupleTable* outputTupleTable)
{
	KWTuple* inputTuple;

	require(nInputFrequency >= 0);
	require(outputTupleTable != NULL);

	// Specification de la table de tuples
	outputTupleTable->CleanAll();

	// Ajout si necessaire d'un unique tupe avec l'effectif total
	if (nInputFrequency > 0)
	{
		outputTupleTable->SetUpdateMode(true);
		inputTuple = outputTupleTable->GetInputTuple();
		inputTuple->SetFrequency(nInputFrequency);
		outputTupleTable->UpdateWithInputTuple();
		outputTupleTable->SetUpdateMode(false);
	}
}

void KWTupleTableLoader::LoadUnivariate(const ALString& sInputAttributeName, KWTupleTable* outputTupleTable) const
{
	StringVector svInputAttributNames;

	svInputAttributNames.Add(sInputAttributeName);
	LoadMultivariate(&svInputAttributNames, outputTupleTable);
}

void KWTupleTableLoader::LoadBivariate(const ALString& sInputAttributeName1, const ALString& sInputAttributeName2,
				       KWTupleTable* outputTupleTable) const
{
	StringVector svInputAttributNames;

	svInputAttributNames.Add(sInputAttributeName1);
	svInputAttributNames.Add(sInputAttributeName2);
	LoadMultivariate(&svInputAttributNames, outputTupleTable);
}

void KWTupleTableLoader::LoadMultivariate(const StringVector* svInputAttributeNames,
					  KWTupleTable* outputTupleTable) const
{
	int nAttribute;
	KWAttribute* attribute;
	KWTuple* inputTuple;
	int nObjectNumber;
	int nObject;
	KWObject* kwoObject;
	KWLoadIndexVector livLoadIndexes;

	require(CheckInputs());
	require(svInputAttributeNames != NULL);
	require(svInputAttributeNames->GetSize() > 0 or sInputExtraAttributeName != "");
	require(outputTupleTable != NULL);

	// Nettoyage prealable de la sortie
	outputTupleTable->CleanAll();

	///////////////////////////////////////////////////////////////////////////////////
	// Specification de la table de tuples

	// Initialisation de la structure de la table de tuples
	// Des verification sont effectuees en cours de creation
	// On en profite egalement pour collecter les LoadIndex des attributs a traiter
	livLoadIndexes.SetSize(svInputAttributeNames->GetSize());
	for (nAttribute = 0; nAttribute < svInputAttributeNames->GetSize(); nAttribute++)
	{
		assert(svInputAttributeNames->GetAt(nAttribute) != "");

		// Acces aux caracteristique de l'attribut
		attribute = kwcInputClass->LookupAttribute(svInputAttributeNames->GetAt(nAttribute));
		check(attribute);
		assert(KWType::IsSimple(attribute->GetType()));
		assert(attribute->GetLoaded());

		// Insertion de l'attribut dans la specification de la table de tuples
		outputTupleTable->AddAttribute(attribute->GetName(), attribute->GetType());

		// Memorisation du LoadIndex de l'attribut
		livLoadIndexes.SetAt(nAttribute, attribute->GetLoadIndex());
	}

	// Ajout optionnel de l'attribut supplementaire
	if (GetInputExtraAttributeType() != KWType::Unknown and GetInputExtraAttributeType() != KWType::None)
	{
		// Si l'attribut cible est present dans la classe, il doit etre du bon type
		assert(kwcInputClass->LookupAttribute(GetInputExtraAttributeName()) == NULL or
		       kwcInputClass->LookupAttribute(GetInputExtraAttributeName())->GetType() ==
			   GetInputExtraAttributeType());

		// Insertion de l'attribut dans la specification de la table de tuples
		outputTupleTable->AddAttribute(GetInputExtraAttributeName(), GetInputExtraAttributeType());
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Alimentation de la table de tuples
	// On a une boucle d'alimentation par type de cas (il est prefable de tester les cas
	// hors des boucles plutot que dans les boucles pour optimiser les performances)

	// Comptage du nombre d'objets
	nObjectNumber = 0;
	if (svInputAttributeNames->GetSize() > 0)
		nObjectNumber = oaInputDatabaseObjects->GetSize();
	else if (GetInputExtraAttributeType() == KWType::Symbol)
		nObjectNumber = svInputExtraAttributeSymbolValues->GetSize();
	else if (GetInputExtraAttributeType() == KWType::Continuous)
		nObjectNumber = cvInputExtraAttributeContinuousValues->GetSize();

	// Passage de la table de tuples en mode edition
	outputTupleTable->SetUpdateMode(true);
	inputTuple = outputTupleTable->GetInputTuple();

	// Alimentation de la table de tuples dans le cas des valeurs supplementaires uniquement
	if (svInputAttributeNames->GetSize() == 0)
	{
		// Cas Symbol
		if (GetInputExtraAttributeType() == KWType::Symbol)
		{
			for (nObject = 0; nObject < nObjectNumber; nObject++)
			{
				inputTuple->SetSymbolAt(0, svInputExtraAttributeSymbolValues->GetAt(nObject));
				outputTupleTable->UpdateWithInputTuple();
			}
		}
		// Cas Continuous
		else
		{
			for (nObject = 0; nObject < nObjectNumber; nObject++)
			{
				inputTuple->SetContinuousAt(0, cvInputExtraAttributeContinuousValues->GetAt(nObject));
				outputTupleTable->UpdateWithInputTuple();
			}
		}
	}
	// Alimentation a partir de la base d'objets
	else
	{
		for (nObject = 0; nObject < nObjectNumber; nObject++)
		{
			// Prise en compte des caracteristiques de l'objet
			kwoObject = cast(KWObject*, oaInputDatabaseObjects->GetAt(nObject));
			for (nAttribute = 0; nAttribute < livLoadIndexes.GetSize(); nAttribute++)
			{
				if (outputTupleTable->GetAttributeTypeAt(nAttribute) == KWType::Symbol)
					inputTuple->SetSymbolAt(
					    nAttribute, kwoObject->GetSymbolValueAt(livLoadIndexes.GetAt(nAttribute)));
				else
					inputTuple->SetContinuousAt(nAttribute, kwoObject->GetContinuousValueAt(
										    livLoadIndexes.GetAt(nAttribute)));
			}

			// Prise en compte de l'attribut supplementaire
			if (GetInputExtraAttributeType() == KWType::Symbol)
			{
				assert(nAttribute == outputTupleTable->GetAttributeNumber() - 1);
				inputTuple->SetSymbolAt(nAttribute, svInputExtraAttributeSymbolValues->GetAt(nObject));
			}
			else if (GetInputExtraAttributeType() == KWType::Continuous)
			{
				assert(nAttribute == outputTupleTable->GetAttributeNumber() - 1);
				inputTuple->SetContinuousAt(nAttribute,
							    cvInputExtraAttributeContinuousValues->GetAt(nObject));
			}

			// Ajout du tuple d'entree
			outputTupleTable->UpdateWithInputTuple();
		}
	}

	// Passage de la table de tuples en mode consultation
	outputTupleTable->SetUpdateMode(false);
}

void KWTupleTableLoader::BlockLoadUnivariateInitialize(const ALString& sInputAttributeBlockName,
						       ObjectDictionary* odInputAttributes,
						       ObjectDictionary* odOutputTupleTables) const
{
	KWAttributeBlock* attributeBlock;
	KWAttribute* attribute;
	ObjectArray oaOutputTupleTables;
	int nTable;
	KWTupleTable* outputTupleTable;
	int nObjectNumber;
	int nObject;
	KWObject* kwoObject;
	KWLoadIndex liAttributeBlockLoadIndex;
	KWContinuousValueBlock* cvbContinuousValues;
	KWSymbolValueBlock* svbSymbolValues;
	int nValue;

	require(sInputAttributeBlockName != "");
	require(odInputAttributes != NULL);
	require(odInputAttributes->GetCount() > 0);
	require(odOutputTupleTables != NULL);
	require(odOutputTupleTables->GetCount() == 0);

	// Acces aux caracteristique de l'attribut
	attributeBlock = kwcInputClass->LookupAttributeBlock(sInputAttributeBlockName);
	check(attributeBlock);
	assert(KWType::IsSimple(attributeBlock->GetType()));
	assert(attributeBlock->GetLoaded());

	// Initialisation du tableau des tables de tuples resultat
	// On passe par un tableau pour avoir un acces indexe efficace
	oaOutputTupleTables.SetSize(attributeBlock->GetLoadedAttributeNumber());
	for (nTable = 0; nTable < attributeBlock->GetLoadedAttributeNumber(); nTable++)
	{
		attribute = attributeBlock->GetLoadedAttributeAt(nTable);

		// Creation d'une table de tuples si necessaire
		if (odInputAttributes->Lookup(attribute->GetName()) != NULL)
		{
			assert(attribute->GetName() != GetInputExtraAttributeName());

			// Creation d'une table de tuple, et memorisation dans le tableau de travail
			outputTupleTable = new KWTupleTable;
			oaOutputTupleTables.SetAt(nTable, outputTupleTable);

			// Memorisation dans le dictionnaire en sortie
			odOutputTupleTables->SetAt(attribute->GetName(), outputTupleTable);

			// Specification des attributs de la table de tuples
			outputTupleTable->AddAttribute(attribute->GetName(), attribute->GetType());
			if (GetInputExtraAttributeName() != "")
				outputTupleTable->AddAttribute(GetInputExtraAttributeName(),
							       GetInputExtraAttributeType());

			// Passage en mode update
			outputTupleTable->SetUpdateMode(true);
		}
	}

	// Comptage du nombre d'objets
	nObjectNumber = oaInputDatabaseObjects->GetSize();

	// Index de chargement du block d'attributs
	liAttributeBlockLoadIndex = attributeBlock->GetLoadIndex();

	// Alimentation a partir de la base d'objets dans le cas d'un bloc Symbol
	if (attributeBlock->GetType() == KWType::Symbol)
	{
		for (nObject = 0; nObject < nObjectNumber; nObject++)
		{
			// Prise en compte des caracteristiques de l'objet
			kwoObject = cast(KWObject*, oaInputDatabaseObjects->GetAt(nObject));

			// Acces au bloc d'attribut
			svbSymbolValues = kwoObject->GetSymbolValueBlockAt(liAttributeBlockLoadIndex);

			// Parcours des valeurs du block
			for (nValue = 0; nValue < svbSymbolValues->GetValueNumber(); nValue++)
			{
				// Acces a la table de tuple correspondant a l'attribut
				nTable = svbSymbolValues->GetAttributeSparseIndexAt(nValue);
				assert(nTable >= 0 and nTable < oaOutputTupleTables.GetSize());
				outputTupleTable = cast(KWTupleTable*, oaOutputTupleTables.GetAt(nTable));
				assert(outputTupleTable == NULL or
				       outputTupleTable->GetAttributeNameAt(0) ==
					   attributeBlock->GetLoadedAttributeAt(nTable)->GetName());

				// On traite les tables de tuples presentes
				if (outputTupleTable != NULL)
				{
					// Mise a jour du tuple d'entree avec la valeur sparse
					outputTupleTable->GetInputTuple()->SetSymbolAt(
					    0, svbSymbolValues->GetValueAt(nValue));

					// Ajout de la valeur de l'attribut supplementaire s'il est present
					if (GetInputExtraAttributeType() == KWType::Symbol)
						outputTupleTable->GetInputTuple()->SetSymbolAt(
						    1, svInputExtraAttributeSymbolValues->GetAt(nObject));
					else if (GetInputExtraAttributeType() == KWType::Continuous)
						outputTupleTable->GetInputTuple()->SetContinuousAt(
						    1, cvInputExtraAttributeContinuousValues->GetAt(nObject));

					// Ajout du tuple d'entree
					outputTupleTable->UpdateWithInputTuple();
				}
			}
		}
	}
	// Alimentation a partir de la base d'objets dans le cas d'un bloc Continuous
	else if (attributeBlock->GetType() == KWType::Continuous)
	{
		for (nObject = 0; nObject < nObjectNumber; nObject++)
		{
			// Prise en compte des caracteristiques de l'objet
			kwoObject = cast(KWObject*, oaInputDatabaseObjects->GetAt(nObject));

			// Acces au bloc d'attribut
			cvbContinuousValues = kwoObject->GetContinuousValueBlockAt(liAttributeBlockLoadIndex);

			// Parcours des valeurs du block
			for (nValue = 0; nValue < cvbContinuousValues->GetValueNumber(); nValue++)
			{
				// Acces a la table de tuple correspondant a l'attribut
				nTable = cvbContinuousValues->GetAttributeSparseIndexAt(nValue);
				assert(nTable >= 0 and nTable < oaOutputTupleTables.GetSize());
				outputTupleTable = cast(KWTupleTable*, oaOutputTupleTables.GetAt(nTable));
				assert(outputTupleTable == NULL or
				       outputTupleTable->GetAttributeNameAt(0) ==
					   attributeBlock->GetLoadedAttributeAt(nTable)->GetName());

				// On traite les tables de tuples presentes
				if (outputTupleTable != NULL)
				{
					// Mise a jour du tuple d'entree avec la valeur sparse
					outputTupleTable->GetInputTuple()->SetContinuousAt(
					    0, cvbContinuousValues->GetValueAt(nValue));

					// Ajout de la valeur de l'attribut supplementaire s'il est present
					if (GetInputExtraAttributeType() == KWType::Symbol)
						outputTupleTable->GetInputTuple()->SetSymbolAt(
						    1, svInputExtraAttributeSymbolValues->GetAt(nObject));
					else if (GetInputExtraAttributeType() == KWType::Continuous)
						outputTupleTable->GetInputTuple()->SetContinuousAt(
						    1, cvInputExtraAttributeContinuousValues->GetAt(nObject));

					// Ajout du tuple d'entree
					outputTupleTable->UpdateWithInputTuple();
				}
			}
		}
	}

	ensure(odOutputTupleTables->GetCount() == odInputAttributes->GetCount());
}

void KWTupleTableLoader::BlockLoadUnivariateFinalize(const ALString& sInputAttributeBlockName,
						     KWTupleTable* outputTupleTable) const
{
	boolean bDisplay = false;
	KWAttributeBlock* attributeBlock;
	int nObjectNumber;
	int nMissingValueNumber;
	ObjectArray oaTuplesSortedByExtraAttribute;
	KWTuple* extraTuple;
	KWTuple* tuple;
	int nExtraTuple;
	int nTuple;
	int nCompare;
	int nValueFrequency;

	require(outputTupleTable != NULL);
	require(kwcInputClass->LookupAttribute(outputTupleTable->GetAttributeNameAt(0)) != NULL);
	require(outputTupleTable->GetUpdateMode());

	// Comptage du nombre d'objets
	nObjectNumber = oaInputDatabaseObjects->GetSize();

	// Acces aux caracteristique de l'attribut
	attributeBlock = kwcInputClass->LookupAttributeBlock(sInputAttributeBlockName);
	check(attributeBlock);
	assert(KWType::IsSimple(attributeBlock->GetType()));
	assert(attributeBlock->GetLoaded());

	// Ajout des tuple manquants s'il manque des renregistrements
	nMissingValueNumber = nObjectNumber - outputTupleTable->GetTotalFrequency();
	if (nMissingValueNumber > 0)
	{
		// Cas sans attribut supplementaire
		if (GetInputExtraAttributeName() == "")
		{
			// Les objet manquants sont tous associes a la valeur manquante du bloc
			if (attributeBlock->GetType() == KWType::Symbol)
				outputTupleTable->GetInputTuple()->SetSymbolAt(0,
									       attributeBlock->GetSymbolDefaultValue());
			else
				outputTupleTable->GetInputTuple()->SetContinuousAt(
				    0, attributeBlock->GetContinuousDefaultValue());

			// Specification de l'effectif manquant
			outputTupleTable->GetInputTuple()->SetFrequency(nMissingValueNumber);

			// Ajout du tuple d'entree
			outputTupleTable->UpdateWithInputTuple();
		}
		// Cas avec attribut supplementaire
		else
		{
			// Collecte des tuples tries selon les valeurs de l'attribut supplementaire
			outputTupleTable->ExportObjectArray(&oaTuplesSortedByExtraAttribute);
			outputTupleTable->SortTuplesByAttribute(GetInputExtraAttributeName(),
								&oaTuplesSortedByExtraAttribute);

			// Affichage des tables de tuples a synchroniser
			if (bDisplay)
			{
				cout << "Extra tuples" << endl;
				for (nExtraTuple = 0; nExtraTuple < inputExtraAttributeTupleTable->GetSize();
				     nExtraTuple++)
				{
					extraTuple = cast(KWTuple*, inputExtraAttributeTupleTable->GetAt(nExtraTuple));
					cout << "\tX" << nExtraTuple << "\t";
					extraTuple->FullWrite(inputExtraAttributeTupleTable, cout);
					cout << endl;
				}
				cout << "New tuples" << endl;
				for (nTuple = 0; nTuple < oaTuplesSortedByExtraAttribute.GetSize(); nTuple++)
				{
					tuple = cast(KWTuple*, oaTuplesSortedByExtraAttribute.GetAt(nTuple));
					cout << "\tN" << nTuple << "\t";
					tuple->FullWrite(outputTupleTable, cout);
					cout << endl;
				}
				cout << "Synchronisation" << endl;
			}

			// Les objet manquants sont tous associes a la valeur manquante du bloc
			// On initialise donc une fois pour toutes cette valeur manquante pour l'attribut du bloc (index
			// 0)
			if (attributeBlock->GetType() == KWType::Symbol)
				outputTupleTable->GetInputTuple()->SetSymbolAt(0,
									       attributeBlock->GetSymbolDefaultValue());
			else
				outputTupleTable->GetInputTuple()->SetContinuousAt(
				    0, attributeBlock->GetContinuousDefaultValue());

			// Parcours de ces tuples de facon synchronise avec ceux de l'attribut supplementaire
			// de facon a identifier les valeurs supplementaires pour lesquelles il manque des tuples
			nTuple = 0;
			for (nExtraTuple = 0; nExtraTuple < inputExtraAttributeTupleTable->GetSize(); nExtraTuple++)
			{
				extraTuple = cast(KWTuple*, inputExtraAttributeTupleTable->GetAt(nExtraTuple));

				// Recherche du premier tuple atteignant ou depassant la valeur courante de l'attribut
				// supplementaire
				nValueFrequency = 0;
				while (nTuple < oaTuplesSortedByExtraAttribute.GetSize())
				{
					tuple = cast(KWTuple*, oaTuplesSortedByExtraAttribute.GetAt(nTuple));

					// Comparaison avec la valeur supplementaire courante
					if (GetInputExtraAttributeType() == KWType::Symbol)
						nCompare = tuple->GetSymbolAt(1).Compare(extraTuple->GetSymbolAt(0));
					else
						nCompare = KWContinuous::Compare(tuple->GetContinuousAt(1),
										 extraTuple->GetContinuousAt(0));
					assert(nCompare >= 0);

					// Si egalite, on compabilise le nombre de valeur correspondant a la valeur
					// courante de l'attribut supplementaire Dans la base de tuple (bivariee),
					// plusieurs tuples successifs peuvent avaoir meme valeur
					if (nCompare == 0)
						nValueFrequency += tuple->GetFrequency();
					// Arret si depassement de la valeur
					else
					{
						assert(nCompare > 0);
						break;
					}

					// Passage au tuple suivant
					nTuple++;
				}

				// Calcul du nombre de valeur manquantes
				nMissingValueNumber = extraTuple->GetFrequency() - nValueFrequency;

				// Mise a jour en cas d'effectif manquant
				if (nMissingValueNumber > 0)
				{
					// Specification de la valeur pour l'attribut supplementaire (index 1)
					if (GetInputExtraAttributeType() == KWType::Symbol)
						outputTupleTable->GetInputTuple()->SetSymbolAt(
						    1, extraTuple->GetSymbolAt(0));
					else
						outputTupleTable->GetInputTuple()->SetContinuousAt(
						    1, extraTuple->GetContinuousAt(0));

					// Specification de l'effectif manquant
					outputTupleTable->GetInputTuple()->SetFrequency(nMissingValueNumber);

					// Ajout du tuple d'entree
					outputTupleTable->UpdateWithInputTuple();

					// Affichage du tuple de synchronisation
					if (bDisplay)
					{
						cout << "\t+X" << nExtraTuple << "\t";
						outputTupleTable->GetInputTuple()->FullWrite(outputTupleTable, cout);
						cout << endl;
					}
				}
			}
		}
	}

	// Passage de la table de tuple en mode consultation
	outputTupleTable->SetUpdateMode(false);
	ensure(outputTupleTable->GetTotalFrequency() == nObjectNumber);
}

void KWTupleTableLoader::BlockLoadBivariate(const ALString& sInputAttributeBlockName,
					    ObjectDictionary* odInputAttributes, const ALString& sInputAttributeName2,
					    ObjectDictionary* odOutputTupleTables) const
{
	boolean bDisplay = false;
	KWAttributeBlock* attributeBlock;
	KWAttribute* attribute2;
	KWLoadIndex livAttribute2LoadIndex;
	int nAttribute2Type;
	KWAttribute* attribute;
	int nTable;
	ObjectArray oaOutputTupleTables;
	IntVector ivBlockAttributeIndex;
	KWTupleTable* outputTupleTable;
	int nBlockAttributeIndex;
	int nAttribute2Index;
	int nObjectNumber;
	int nObject;
	KWObject* kwoObject;
	Symbol sValue2;
	Continuous cValue2;
	Symbol sExtraValue;
	Continuous cExtraValue;
	KWLoadIndex liAttributeBlockLoadIndex;
	KWContinuousValueBlock* cvbContinuousValues;
	KWSymbolValueBlock* svbSymbolValues;
	int nValue;
	int nMissingValueNumber;
	KWTupleTable* lastAttributesTupleTable;
	ObjectArray oaTuplesSortedByLastAttributes;
	KWTuple* inputTuple;
	KWTuple* extraTuple;
	KWTuple* tuple;
	int nExtraTuple;
	int nTuple;
	int nCompare;
	int nValueFrequency;

	require(sInputAttributeBlockName != "");
	require(sInputAttributeName2 != "");
	require(sInputAttributeName2 != GetInputExtraAttributeName());
	require(odInputAttributes != NULL);
	require(odInputAttributes->GetCount() > 0);
	require(odOutputTupleTables != NULL);
	require(odOutputTupleTables->GetCount() == 0);

	// Acces aux caracteristique de l'attribut
	attributeBlock = kwcInputClass->LookupAttributeBlock(sInputAttributeBlockName);
	check(attributeBlock);
	assert(KWType::IsSimple(attributeBlock->GetType()));
	assert(attributeBlock->GetLoaded());

	// Acces au deuxieme attribut
	attribute2 = kwcInputClass->LookupAttribute(sInputAttributeName2);
	check(attribute2);
	assert(KWType::IsSimple(attribute2->GetType()));
	assert(attribute2->GetLoaded());

	// Typpe et index de chargement du deuxieme attribut
	nAttribute2Type = attribute2->GetType();
	livAttribute2LoadIndex = attribute2->GetLoadIndex();

	// Initialisations, pour eviter les warning
	cValue2 = 0;
	cExtraValue = 0;

	// Initialisation du tableau des tables de tuples resultat
	// On passe par un tableau pour avoir un acces indexe efficace
	oaOutputTupleTables.SetSize(attributeBlock->GetLoadedAttributeNumber());
	ivBlockAttributeIndex.SetSize(attributeBlock->GetLoadedAttributeNumber());
	for (nTable = 0; nTable < attributeBlock->GetLoadedAttributeNumber(); nTable++)
	{
		attribute = attributeBlock->GetLoadedAttributeAt(nTable);

		// Creation d'une table de tuples si necessaire
		if (odInputAttributes->Lookup(attribute->GetName()) != NULL)
		{
			assert(attribute->GetName() != sInputAttributeName2);
			assert(attribute->GetName() != GetInputExtraAttributeName());

			// Creation d'une table de tuple, et memorisation dans le tableau de trabail
			outputTupleTable = new KWTupleTable;
			oaOutputTupleTables.SetAt(nTable, outputTupleTable);

			// Memorisation dans le dictionnaire en sortie
			odOutputTupleTables->SetAt(attribute->GetName(), outputTupleTable);

			// Specification des attributs de la table de tuples, par ordre alphabetique
			if (attribute->GetName() <= sInputAttributeName2)
			{
				outputTupleTable->AddAttribute(attribute->GetName(), attribute->GetType());
				outputTupleTable->AddAttribute(attribute2->GetName(), attribute2->GetType());
				ivBlockAttributeIndex.SetAt(nTable, 0);
			}
			else
			{
				outputTupleTable->AddAttribute(attribute2->GetName(), attribute2->GetType());
				outputTupleTable->AddAttribute(attribute->GetName(), attribute->GetType());
				ivBlockAttributeIndex.SetAt(nTable, 1);
			}
			if (GetInputExtraAttributeName() != "")
				outputTupleTable->AddAttribute(GetInputExtraAttributeName(),
							       GetInputExtraAttributeType());

			// Passage en mode update
			outputTupleTable->SetUpdateMode(true);
		}
	}

	// Comptage du nombre d'objets
	nObjectNumber = oaInputDatabaseObjects->GetSize();

	// Index de chargement du block d'attributs
	liAttributeBlockLoadIndex = attributeBlock->GetLoadIndex();

	// Alimentation a partir de la base d'objets dans le cas d'un bloc Symbol
	if (attributeBlock->GetType() == KWType::Symbol)
	{
		for (nObject = 0; nObject < nObjectNumber; nObject++)
		{
			// Prise en compte des caracteristiques de l'objet
			kwoObject = cast(KWObject*, oaInputDatabaseObjects->GetAt(nObject));

			// Acces au bloc d'attribut
			svbSymbolValues = kwoObject->GetSymbolValueBlockAt(liAttributeBlockLoadIndex);

			// Acces de la valeur du deuxieme attribut
			if (nAttribute2Type == KWType::Symbol)
				sValue2 = kwoObject->GetSymbolValueAt(livAttribute2LoadIndex);
			else
				cValue2 = kwoObject->GetContinuousValueAt(livAttribute2LoadIndex);

			// Acces de la valeur de l'attrivut supplementaire s'il est present
			if (GetInputExtraAttributeType() == KWType::Symbol)
				sExtraValue = svInputExtraAttributeSymbolValues->GetAt(nObject);
			else if (GetInputExtraAttributeType() == KWType::Continuous)
				cExtraValue = cvInputExtraAttributeContinuousValues->GetAt(nObject);

			// Parcours des valeurs du block
			for (nValue = 0; nValue < svbSymbolValues->GetValueNumber(); nValue++)
			{
				// Acces a la table de tuple correspondant a l'attribut
				nTable = svbSymbolValues->GetAttributeSparseIndexAt(nValue);
				assert(nTable >= 0 and nTable < oaOutputTupleTables.GetSize());
				outputTupleTable = cast(KWTupleTable*, oaOutputTupleTables.GetAt(nTable));

				// On traite les tables de tuples presentes
				if (outputTupleTable != NULL)
				{
					// Acces au tuple d'entree de la table
					inputTuple = outputTupleTable->GetInputTuple();

					// Index de l'attribut de block de l'attribut 2
					nBlockAttributeIndex = ivBlockAttributeIndex.GetAt(nTable);
					nAttribute2Index = 1 - nBlockAttributeIndex;
					assert(outputTupleTable->GetAttributeNameAt(nBlockAttributeIndex) ==
					       attributeBlock->GetLoadedAttributeAt(nTable)->GetName());
					assert(outputTupleTable->GetAttributeNameAt(nAttribute2Index) ==
					       sInputAttributeName2);

					// Mise a jour du tuple d'entree avec la valeur sparse, selon l'ordre des
					// attributs
					inputTuple->SetSymbolAt(nBlockAttributeIndex,
								svbSymbolValues->GetValueAt(nValue));

					// Ajout de la valeur du deuxieme attribut
					if (nAttribute2Type == KWType::Symbol)
						inputTuple->SetSymbolAt(nAttribute2Index, sValue2);
					else
						inputTuple->SetContinuousAt(nAttribute2Index, cValue2);

					// Ajout de la valeur de l'attribut supplementaire s'il est present en plus
					if (GetInputExtraAttributeType() == KWType::Symbol)
						inputTuple->SetSymbolAt(2, sExtraValue);
					else if (GetInputExtraAttributeType() == KWType::Continuous)
						inputTuple->SetContinuousAt(2, cExtraValue);

					// Ajout du tuple d'entree
					outputTupleTable->UpdateWithInputTuple();
				}
			}
		}
	}
	// Alimentation a partir de la base d'objets dans le cas d'un bloc Continuous
	else if (attributeBlock->GetType() == KWType::Continuous)
	{
		for (nObject = 0; nObject < nObjectNumber; nObject++)
		{
			// Prise en compte des caracteristiques de l'objet
			kwoObject = cast(KWObject*, oaInputDatabaseObjects->GetAt(nObject));

			// Acces au bloc d'attribut
			cvbContinuousValues = kwoObject->GetContinuousValueBlockAt(liAttributeBlockLoadIndex);

			// Acces de la valeur du deuxieme attribut
			if (nAttribute2Type == KWType::Symbol)
				sValue2 = kwoObject->GetSymbolValueAt(livAttribute2LoadIndex);
			else
				cValue2 = kwoObject->GetContinuousValueAt(livAttribute2LoadIndex);

			// Acces de la valeur de l'attrivut supplementaire s'il est present
			if (GetInputExtraAttributeType() == KWType::Symbol)
				sExtraValue = svInputExtraAttributeSymbolValues->GetAt(nObject);
			else if (GetInputExtraAttributeType() == KWType::Continuous)
				cExtraValue = cvInputExtraAttributeContinuousValues->GetAt(nObject);

			// Parcours des valeurs du block
			for (nValue = 0; nValue < cvbContinuousValues->GetValueNumber(); nValue++)
			{
				// Acces a la table de tuple correspondant a l'attribut
				nTable = cvbContinuousValues->GetAttributeSparseIndexAt(nValue);
				assert(nTable >= 0 and nTable < oaOutputTupleTables.GetSize());
				outputTupleTable = cast(KWTupleTable*, oaOutputTupleTables.GetAt(nTable));

				// On traite les tables de tuples presentes
				if (outputTupleTable != NULL)
				{
					// Acces au tuple d'entree de la table
					inputTuple = outputTupleTable->GetInputTuple();

					// Index de l'attribut de block de l'attribut 2
					nBlockAttributeIndex = ivBlockAttributeIndex.GetAt(nTable);
					nAttribute2Index = 1 - nBlockAttributeIndex;
					assert(outputTupleTable->GetAttributeNameAt(nBlockAttributeIndex) ==
					       attributeBlock->GetLoadedAttributeAt(nTable)->GetName());
					assert(outputTupleTable->GetAttributeNameAt(nAttribute2Index) ==
					       sInputAttributeName2);

					// Mise a jour du tuple d'entree avec la valeur sparse, selon l'ordre des
					// attributs
					inputTuple->SetContinuousAt(nBlockAttributeIndex,
								    cvbContinuousValues->GetValueAt(nValue));

					// Ajout de la valeur du deuxieme attribut
					if (nAttribute2Type == KWType::Symbol)
						inputTuple->SetSymbolAt(nAttribute2Index, sValue2);
					else
						inputTuple->SetContinuousAt(nAttribute2Index, cValue2);

					// Ajout de la valeur de l'attribut supplementaire s'il est present en plus
					if (GetInputExtraAttributeType() == KWType::Symbol)
						inputTuple->SetSymbolAt(2, sExtraValue);
					else if (GetInputExtraAttributeType() == KWType::Continuous)
						inputTuple->SetContinuousAt(2, cExtraValue);

					// Ajout du tuple d'entree
					outputTupleTable->UpdateWithInputTuple();
				}
			}
		}
	}

	// On ajoute un tuple pour les objets avec valeur manquante
	lastAttributesTupleTable = NULL;
	for (nTable = 0; nTable < oaOutputTupleTables.GetSize(); nTable++)
	{
		outputTupleTable = cast(KWTupleTable*, oaOutputTupleTables.GetAt(nTable));

		// On traite les tables de tuples presentes
		if (outputTupleTable != NULL)
		{
			// Index de l'attribut de block de l'attribut 2
			nBlockAttributeIndex = ivBlockAttributeIndex.GetAt(nTable);
			nAttribute2Index = 1 - nBlockAttributeIndex;
			assert(outputTupleTable->GetAttributeNameAt(nBlockAttributeIndex) ==
			       attributeBlock->GetLoadedAttributeAt(nTable)->GetName());
			assert(outputTupleTable->GetAttributeNameAt(nAttribute2Index) == sInputAttributeName2);

			// Ajout des tuples manquants s'il manque des renregistrements
			nMissingValueNumber = nObjectNumber - outputTupleTable->GetTotalFrequency();
			if (nMissingValueNumber > 0)
			{
				// Calcul si necessaire de la table de tuples pour le deuxieme attribut et l'attribut
				// supplementaire Cette table sert de reference pour toutes les paires de valeurs en
				// l'absence du premier attribut issu du bloc d'attributs Par comparaison entre les
				// valeur extraites et la reference de toutes les valeurs, on en deduit les les tuples
				// corrrespondant aux valeurs manquantes
				if (lastAttributesTupleTable == NULL)
				{
					lastAttributesTupleTable = new KWTupleTable;
					LoadUnivariate(sInputAttributeName2, lastAttributesTupleTable);
				}
				assert(lastAttributesTupleTable->GetTotalFrequency() == nObjectNumber);

				// Collecte des tuples tries selons les valeurs de l'attribut supplementaire
				outputTupleTable->ExportObjectArray(&oaTuplesSortedByLastAttributes);
				if (GetInputExtraAttributeName() == "")
					outputTupleTable->SortTuplesByAttribute(sInputAttributeName2,
										&oaTuplesSortedByLastAttributes);
				// Ou selon les valeurs du deuxieme attribut et de l'attribut supplementaire
				else
					outputTupleTable->SortTuplesByAttributePair(sInputAttributeName2,
										    GetInputExtraAttributeName(),
										    &oaTuplesSortedByLastAttributes);

				// Affichage des tables de tuples a synchroniser
				if (bDisplay)
				{
					cout << "Output tuples attributes" << endl;
					cout << "\t" << outputTupleTable->GetClassLabel() << "\t"
					     << outputTupleTable->GetObjectLabel() << endl;
					cout << "Output tuples" << endl;
					outputTupleTable->SetUpdateMode(false);
					for (nTuple = 0; nTuple < outputTupleTable->GetSize(); nTuple++)
					{
						tuple = cast(KWTuple*, outputTupleTable->GetAt(nTuple));
						cout << "\tF" << nTuple << "\t";
						tuple->FullWrite(outputTupleTable, cout);
						cout << endl;
					}
					outputTupleTable->SetUpdateMode(true);
					cout << "Extra tuples" << endl;
					for (nExtraTuple = 0; nExtraTuple < inputExtraAttributeTupleTable->GetSize();
					     nExtraTuple++)
					{
						extraTuple =
						    cast(KWTuple*, inputExtraAttributeTupleTable->GetAt(nExtraTuple));
						cout << "\tX" << nExtraTuple << "\t";
						extraTuple->FullWrite(inputExtraAttributeTupleTable, cout);
						cout << endl;
					}
					cout << "Last attributes tuples" << endl;
					for (nTuple = 0; nTuple < lastAttributesTupleTable->GetSize(); nTuple++)
					{
						tuple = cast(KWTuple*, lastAttributesTupleTable->GetAt(nTuple));
						cout << "\tL" << nTuple << "\t";
						tuple->FullWrite(lastAttributesTupleTable, cout);
						cout << endl;
					}
					cout << "Sorted output tuples" << endl;
					for (nTuple = 0; nTuple < oaTuplesSortedByLastAttributes.GetSize(); nTuple++)
					{
						tuple = cast(KWTuple*, oaTuplesSortedByLastAttributes.GetAt(nTuple));
						cout << "\tF" << nTuple << "\t";
						tuple->FullWrite(outputTupleTable, cout);
						cout << endl;
					}
					cout << "Synchronisation" << endl;
				}

				// Les objet manquants sont tous associes a la valeur manquante du bloc
				// On initialise donc une fois pour toutes cette valeur manquante pour l'attribut du
				// bloc (index: nBlockAttributeIndex)
				inputTuple = outputTupleTable->GetInputTuple();
				if (attributeBlock->GetType() == KWType::Symbol)
					inputTuple->SetSymbolAt(nBlockAttributeIndex,
								attributeBlock->GetSymbolDefaultValue());
				else
					inputTuple->SetContinuousAt(nBlockAttributeIndex,
								    attributeBlock->GetContinuousDefaultValue());

				// Parcours de ces tuples de facon synchronisee avec ceux du deuxieme attribut et
				// l'attribut supplementaire de facon a identifier les valeurs supplementaires pour
				// lesquelles il manque des tuples
				nTuple = 0;
				for (nExtraTuple = 0; nExtraTuple < lastAttributesTupleTable->GetSize(); nExtraTuple++)
				{
					extraTuple = cast(KWTuple*, lastAttributesTupleTable->GetAt(nExtraTuple));

					// Recherche du premier tuple atteignant ou depassant la valeur courante de
					// l'attribut supplementaire
					nValueFrequency = 0;
					while (nTuple < oaTuplesSortedByLastAttributes.GetSize())
					{
						tuple = cast(KWTuple*, oaTuplesSortedByLastAttributes.GetAt(nTuple));

						// Comparaison avec la valeur courante du deuxieme attribut, qui se
						// trouve a l'index nAttribute2Index pour le tuple courant et a l'index
						// 0 pour l'extraTuple courant
						if (nAttribute2Type == KWType::Symbol)
							nCompare = tuple->GetSymbolAt(nAttribute2Index)
								       .Compare(extraTuple->GetSymbolAt(0));
						else
							nCompare = KWContinuous::Compare(
							    tuple->GetContinuousAt(nAttribute2Index),
							    extraTuple->GetContinuousAt(0));
						assert(nCompare >= 0);

						// Comparaison si necesssaire avec les valeurs courantes de l'attribut
						// supplementaire (aux index 2 pour le tuple, et 1 pour l'extraTuple)
						if (nCompare == 0)
						{
							if (GetInputExtraAttributeType() == KWType::Symbol)
								nCompare = tuple->GetSymbolAt(2).Compare(
								    extraTuple->GetSymbolAt(1));
							else if (GetInputExtraAttributeType() == KWType::Continuous)
								nCompare = KWContinuous::Compare(
								    tuple->GetContinuousAt(2),
								    extraTuple->GetContinuousAt(1));
						}
						assert(nCompare >= 0);

						// Si egalite, on compabilise le nombre de valeur correspondant a la
						// paire de valeurs courantes Dans la base de tuples, plusieurs tuples
						// successifs peuvent avoir meme paire de valeurs
						if (nCompare == 0)
							nValueFrequency += tuple->GetFrequency();
						// Arret si depassement de la valeur
						else
						{
							assert(nCompare > 0);
							break;
						}

						// Passage au tuple suivant
						nTuple++;
					}

					// Calcul du nombre de valeur manquantes
					nMissingValueNumber = extraTuple->GetFrequency() - nValueFrequency;

					// Mise a jour en cas d'effectif manquant
					if (nMissingValueNumber > 0)
					{
						assert(inputTuple == outputTupleTable->GetInputTuple());

						// Specification de la valeur pour le deuxieme attribut (index
						// nAttribute2Index)
						if (nAttribute2Type == KWType::Symbol)
							inputTuple->SetSymbolAt(nAttribute2Index,
										extraTuple->GetSymbolAt(0));
						else
							inputTuple->SetContinuousAt(nAttribute2Index,
										    extraTuple->GetContinuousAt(0));

						// Specification de la valeur pour l'attribut supplementaire si present
						// (aux index 2 pour le tuple, et 1 pour l'extraTuple)
						if (GetInputExtraAttributeType() == KWType::Symbol)
							inputTuple->SetSymbolAt(2, extraTuple->GetSymbolAt(1));
						else if (GetInputExtraAttributeType() == KWType::Continuous)
							inputTuple->SetContinuousAt(2, extraTuple->GetContinuousAt(1));

						// Specification de l'effectif manquant
						inputTuple->SetFrequency(nMissingValueNumber);

						// Ajout du tuple d'entree
						outputTupleTable->UpdateWithInputTuple();

						// Affichage du tuple de synchronisation
						if (bDisplay)
						{
							cout << "\t+X" << nExtraTuple << "\t";
							outputTupleTable->GetInputTuple()->FullWrite(outputTupleTable,
												     cout);
							cout << endl;
						}
					}
				}
			}

			// Passage de la table de tuples en mode consultation
			outputTupleTable->SetUpdateMode(false);
			ensure(outputTupleTable->GetTotalFrequency() == nObjectNumber);
		}
	}

	// Nettoyage
	if (lastAttributesTupleTable != NULL)
		delete lastAttributesTupleTable;
	ensure(odOutputTupleTables->GetCount() == odInputAttributes->GetCount());
}

void KWTupleTableLoader::Test()
{
	int nObjectNumber = 100000;
	debug(nObjectNumber = 10000);
	int nDuplicationNumber = 10;
	boolean bTestLoad = true;
	boolean bTestBlockLoad = true;
	boolean bTestWithExtra = true;
	boolean bTestWithoutExtra = true;
	boolean bTestUnivariate = true;
	boolean bTestBivariate = true;
	KWTupleTable tupleTable;
	KWTupleTableLoader tupleTableLoader;
	KWDatabase database;
	ObjectArray oaDatabaseObjects;
	ObjectArray oaBlockAttributes;
	ObjectDictionary odBlockAttributes;
	ObjectDictionary odBlockTupleTables;
	KWTupleTable* currentTupleTable;
	int nObject;
	KWClass* kwcClass;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWObject* kwoObject;
	int nExtraAttributeIndex;
	int nLastDenseSymbolAttributeIndex;
	int nLastDenseContinuousAttributeIndex;
	int nLastSparseSymbolAttributeIndex;
	int nLastSparseContinuousAttributeIndex;
	KWAttribute* extraAttribute;
	ObjectArray oaTupleTableDefinitions;
	StringVector* svAttributeNames;
	StringVector svAttributeBlockNames;
	int nCase;
	int nAttribute;
	int i;
	Timer timer;
	int nExtraAttributeNumber;

	///////////////////////////////////////////////////
	// Creation et parametrage de l'ensemble des objets

	// Creation d'une base
	database.TestCreateObjects(nObjectNumber);
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(database.GetClassName());

	// Affichage des premiers objets
	cout << *kwcClass << endl;
	for (nObject = 0; nObject < 3; nObject++)
	{
		kwoObject = cast(KWObject*, database.GetObjects()->GetAt(nObject));
		cout << *kwoObject << endl;
	}

	// Duplication de la base pour cree des doublons
	for (i = 0; i < nDuplicationNumber; i++)
		oaDatabaseObjects.InsertObjectArrayAt(oaDatabaseObjects.GetSize(), database.GetObjects());
	oaDatabaseObjects.Shuffle();

	// Parametrage de la base en entree
	tupleTableLoader.SetInputClass(kwcClass);
	tupleTableLoader.SetInputDatabaseObjects(&oaDatabaseObjects);

	/////////////////////////////////////////////////
	// Creation de l'ensemble des cas a tester

	// On reserve le dernier attribut dense comme attribut supplementaire
	nExtraAttributeIndex = 0;

	// Recherche des index par type d'attributs, vus pour la derniere fois
	nLastDenseSymbolAttributeIndex = -1;
	nLastDenseContinuousAttributeIndex = -1;
	nLastSparseSymbolAttributeIndex = -1;
	nLastSparseContinuousAttributeIndex = -1;
	for (i = 0; i < kwcClass->GetLoadedAttributeNumber(); i++)
	{
		attribute = kwcClass->GetLoadedAttributeAt(i);
		if (attribute->GetType() == KWType::Symbol)
		{
			if (attribute->IsInBlock())
				nLastSparseSymbolAttributeIndex = i;
			else
				nLastDenseSymbolAttributeIndex = i;
		}
		else if (attribute->GetType() == KWType::Continuous)
		{
			if (attribute->IsInBlock())
				nLastSparseContinuousAttributeIndex = i;
			else
				nLastDenseContinuousAttributeIndex = i;
		}
	}
	assert(nLastDenseSymbolAttributeIndex >= 0);
	assert(nLastDenseContinuousAttributeIndex >= 0);
	assert(nLastSparseSymbolAttributeIndex >= 0);
	assert(nLastSparseContinuousAttributeIndex >= 0);

	// Un attribut categoriel dense
	svAttributeNames = new StringVector;
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastDenseSymbolAttributeIndex)->GetName());
	oaTupleTableDefinitions.Add(svAttributeNames);

	// Un attribut categoriel sparse
	svAttributeNames = new StringVector;
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastSparseSymbolAttributeIndex)->GetName());
	oaTupleTableDefinitions.Add(svAttributeNames);

	// Un attribut numerique dense
	svAttributeNames = new StringVector;
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastDenseContinuousAttributeIndex)->GetName());
	oaTupleTableDefinitions.Add(svAttributeNames);

	// Un attribut numerique sparse
	svAttributeNames = new StringVector;
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastSparseContinuousAttributeIndex)->GetName());
	oaTupleTableDefinitions.Add(svAttributeNames);

	// Deux attributs categoriels
	svAttributeNames = new StringVector;
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastDenseSymbolAttributeIndex)->GetName());
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastSparseSymbolAttributeIndex)->GetName());
	oaTupleTableDefinitions.Add(svAttributeNames);

	// Deux attributs numeriques
	svAttributeNames = new StringVector;
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastDenseContinuousAttributeIndex)->GetName());
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastSparseContinuousAttributeIndex)->GetName());
	oaTupleTableDefinitions.Add(svAttributeNames);

	// Deux attributs mixtes denses
	svAttributeNames = new StringVector;
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastDenseSymbolAttributeIndex)->GetName());
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastDenseContinuousAttributeIndex)->GetName());
	oaTupleTableDefinitions.Add(svAttributeNames);

	// Deux attributs mixtes sparses
	svAttributeNames = new StringVector;
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastSparseSymbolAttributeIndex)->GetName());
	svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nLastSparseContinuousAttributeIndex)->GetName());
	oaTupleTableDefinitions.Add(svAttributeNames);

	// Tous les attributs
	svAttributeNames = new StringVector;
	for (nAttribute = 1; nAttribute < kwcClass->GetLoadedAttributeNumber(); nAttribute++)
		svAttributeNames->Add(kwcClass->GetLoadedAttributeAt(nAttribute)->GetName());
	oaTupleTableDefinitions.Add(svAttributeNames);

	// Et pour les blocs
	if (kwcClass->GetLoadedAttributeBlockNumber() > 0)
		svAttributeBlockNames.Add(kwcClass->GetLoadedAttributeBlockAt(0)->GetName());
	if (kwcClass->GetLoadedAttributeBlockNumber() > 1)
		svAttributeBlockNames.Add(
		    kwcClass->GetLoadedAttributeBlockAt(kwcClass->GetLoadedAttributeBlockNumber() - 1)->GetName());

	////////////////////////////////////////////////////////////////////////////
	// Tests de chargement des tuples

	// Parcours des cas avec ou sans attribut supplementaire
	for (nExtraAttributeNumber = 0; nExtraAttributeNumber <= 1; nExtraAttributeNumber++)
	{
		if (not bTestWithoutExtra and nExtraAttributeNumber == 0)
			continue;
		if (not bTestWithExtra and nExtraAttributeNumber == 1)
			continue;

		// Libelle specifique
		if (nExtraAttributeNumber == 0)
			cout << "=== Tests without extra variable ===\n" << endl;
		else
			cout << "=== Tests with extra variable ===\n" << endl;

		// Gestion des cas avec attribut supplementaire
		extraAttribute = NULL;
		if (nExtraAttributeNumber == 1)
		{
			// Parametrage d'un attribut supplementaire
			extraAttribute = kwcClass->GetLoadedAttributeAt(nExtraAttributeIndex);
			tupleTableLoader.CreateAllExtraInputParameters(extraAttribute->GetName());

			// Ajout du cas ou il n'y a que l'attribut supplementaire
			svAttributeNames = new StringVector;
			oaTupleTableDefinitions.Add(svAttributeNames);
		}

		// Parcours des cas de chargement par attributs
		if (bTestLoad)
		{
			cout << "--- Tuple table load tests ---\n" << endl;
			for (nCase = 0; nCase < oaTupleTableDefinitions.GetSize(); nCase++)
			{
				svAttributeNames = cast(StringVector*, oaTupleTableDefinitions.GetAt(nCase));

				// Tests de chargement de la table de tuples
				timer.Reset();
				timer.Start();
				tupleTableLoader.LoadMultivariate(svAttributeNames, &tupleTable);
				timer.Stop();

				// Affichage du resultat
				cout << "SYSTEM TIME\t" << timer.GetElapsedTime() << "\t";
				cout << "SYSTEM MEM\t" << tupleTable.GetUsedMemory() << endl;
				cout << tupleTable << endl;

				// Nettoyage de la table de tuples
				tupleTable.CleanAll();
			}
		}

		// Parcours des cas de chargement par blocs d'attributs
		if (bTestBlockLoad)
		{
			// Cas univarie
			if (bTestUnivariate)
			{
				cout << "--- Tuple table block load univariate tests ---\n" << endl;
				for (nCase = 0; nCase < svAttributeBlockNames.GetSize(); nCase++)
				{
					// Collecte des attributs du bloc
					attributeBlock = tupleTableLoader.GetInputClass()->LookupAttributeBlock(
					    svAttributeBlockNames.GetAt(nCase));
					assert(oaBlockAttributes.GetSize() == 0);
					for (i = 0; i < attributeBlock->GetLoadedAttributeNumber(); i++)
					{
						attribute = attributeBlock->GetLoadedAttributeAt(i);
						if (attribute->GetName() !=
						    tupleTableLoader.GetInputExtraAttributeName())
						{
							oaBlockAttributes.Add(attribute);
							odBlockAttributes.SetAt(attribute->GetName(), attribute);
						}
					}

					// Chargement du tableau de tables de tuples
					timer.Reset();
					timer.Start();
					tupleTableLoader.BlockLoadUnivariateInitialize(
					    svAttributeBlockNames.GetAt(nCase), &odBlockAttributes,
					    &odBlockTupleTables);
					timer.Stop();
					cout << "Load set of tuple tables" << endl;
					cout << "SYSTEM TIME\t" << timer.GetElapsedTime() << "\t";
					cout << "SYSTEM MEM\t" << odBlockTupleTables.GetOverallUsedMemory() << endl
					     << endl;

					// Dinlaisation du chargement et affichage du resultat
					for (i = 0; i < oaBlockAttributes.GetSize(); i++)
					{
						attribute = cast(KWAttribute*, oaBlockAttributes.GetAt(i));
						currentTupleTable = cast(
						    KWTupleTable*, odBlockTupleTables.Lookup(attribute->GetName()));
						assert(currentTupleTable != NULL);

						// Finalisation du chargement
						tupleTableLoader.BlockLoadUnivariateFinalize(
						    svAttributeBlockNames.GetAt(nCase), currentTupleTable);

						// Affichage
						cout << *currentTupleTable << endl;
					}

					// Nettoyage
					odBlockTupleTables.DeleteAll();
					oaBlockAttributes.SetSize(0);
					odBlockAttributes.RemoveAll();
				}
			}

			// Cas bivarie
			if (bTestBivariate)
			{
				cout << "--- Tuple table block load bivarie tests ---\n" << endl;
				for (nCase = 0; nCase < svAttributeBlockNames.GetSize(); nCase++)
				{
					// Collecte des attributs du bloc
					attributeBlock = tupleTableLoader.GetInputClass()->LookupAttributeBlock(
					    svAttributeBlockNames.GetAt(nCase));
					assert(oaBlockAttributes.GetSize() == 0);
					for (i = 0; i < attributeBlock->GetLoadedAttributeNumber(); i++)
					{
						attribute = attributeBlock->GetLoadedAttributeAt(i);
						if (attribute->GetName() !=
							tupleTableLoader.GetInputExtraAttributeName() and
						    attribute->GetName() !=
							kwcClass->GetLoadedAttributeAt(nLastDenseSymbolAttributeIndex)
							    ->GetName())
						{
							oaBlockAttributes.Add(attribute);
							odBlockAttributes.SetAt(attribute->GetName(), attribute);
						}
					}

					// Chargement du tableau de tables de tuples
					timer.Reset();
					timer.Start();
					tupleTableLoader.BlockLoadBivariate(
					    svAttributeBlockNames.GetAt(nCase), &odBlockAttributes,
					    kwcClass->GetLoadedAttributeAt(nLastDenseSymbolAttributeIndex)->GetName(),
					    &odBlockTupleTables);
					timer.Stop();
					cout << "Load array of tuple tables" << endl;
					cout << "SYSTEM TIME\t" << timer.GetElapsedTime() << "\t";
					cout << "SYSTEM MEM\t" << odBlockTupleTables.GetOverallUsedMemory() << endl
					     << endl;

					// Affichage du resultat
					for (i = 0; i < oaBlockAttributes.GetSize(); i++)
					{
						attribute = cast(KWAttribute*, oaBlockAttributes.GetAt(i));
						currentTupleTable = cast(
						    KWTupleTable*, odBlockTupleTables.Lookup(attribute->GetName()));
						assert(currentTupleTable != NULL);
						cout << *currentTupleTable << endl;
					}

					// Nettoyage
					odBlockTupleTables.DeleteAll();
					oaBlockAttributes.SetSize(0);
					odBlockAttributes.RemoveAll();
				}
			}
		}
	}

	// Nettoyage
	tupleTableLoader.DeleteExtraAttributeInputs();
	oaTupleTableDefinitions.DeleteAll();
	database.DeleteAll();
	KWClassDomain::DeleteAllDomains();
}