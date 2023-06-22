// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTupleTable.h"

///////////////////////////////////////////////////////////////////////////////////
// Class KWTuple

void KWTuple::FullWrite(const KWTupleTable* ownerTupleTable, ostream& ost) const
{
	int i;

	require(ownerTupleTable != NULL);
	debug(require(tupleTable == ownerTupleTable));

	ost << "(";
	for (i = 0; i < ownerTupleTable->GetAttributeNumber(); i++)
	{
		if (i > 0)
			ost << ", ";
		if (ownerTupleTable->GetAttributeTypeAt(i) == KWType::Symbol)
			ost << GetSymbolAt(i).GetValue();
		else
			ost << KWContinuous::ContinuousToString(GetContinuousAt(i));
	}
	ost << ": ";
	ost << IntToString(nFrequency);
	ost << ")";
}

void KWTuple::Write(ostream& ost) const
{
	ost << "(";
	ost << IntToString(nFrequency);
	ost << ")";
}

const ALString KWTuple::GetClassLabel() const
{
	return "Tuple";
}

const ALString KWTuple::GetObjectLabel() const
{
	ALString sObjectLabel;

	sObjectLabel = "(";
	sObjectLabel += IntToString(nFrequency);
	sObjectLabel += ")";
	return sObjectLabel;
}

const KWTupleTable* KWTuple::GetTupleTable() const
{
	debug(assert(tupleTable != NULL));
	debug(return tupleTable);
	// Ne devrait jamais etre appele en release
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////
// Class KWTupleTable

KWTupleTable::KWTupleTable()
{
	nSize = 0;
	nTotalFrequency = 0;
	slTuples = NULL;
	inputTuple = NULL;
}

KWTupleTable::~KWTupleTable()
{
	DeleteAll();
}

void KWTupleTable::CleanAll()
{
	require(not GetUpdateMode());

	// On detruit les tuples prelablement
	DeleteAll();
	nSize = 0;
	nTotalFrequency = 0;

	// On detruit ensuite la definition de la structure des tuples
	svAttributeNames.SetSize(0);
	ivAttributeTypes.SetSize(0);
}

void KWTupleTable::AddAttribute(const ALString& sName, int nType)
{
	require(GetSize() == 0);
	require(not GetUpdateMode());
	require(sName != "");
	require(LookupAttributeIndex(sName) == -1);
	require(KWType::IsSimple(nType));

	svAttributeNames.Add(sName);
	ivAttributeTypes.Add(nType);
}

int KWTupleTable::LookupAttributeIndex(const ALString& sName) const
{
	int i;

	for (i = 0; i < GetAttributeNumber(); i++)
	{
		if (GetAttributeNameAt(i) == sName)
			return i;
	}
	return -1;
}

void KWTupleTable::DeleteAllAttributes()
{
	require(not GetUpdateMode());

	require(GetSize() == 0);

	svAttributeNames.SetSize(0);
	ivAttributeTypes.SetSize(0);
}

void KWTupleTable::DeleteAll()
{
	int i;
	KWTuple* tuple;

	// On remet la table de tuple en mode consultation si necessaire
	if (GetUpdateMode())
		SetUpdateMode(false);

	// On detruit explicitement chaque tuple du table (la classe KWTuple n'a pas de destructeur explicite)
	for (i = 0; i < oaTuples.GetSize(); i++)
	{
		tuple = cast(KWTuple*, oaTuples.GetAt(i));
		DeleteTuple(tuple);
	}
	oaTuples.SetSize(0);

	// Remise a zero de l'effectif total
	nSize = 0;
	nTotalFrequency = 0;
}

void KWTupleTable::Sort()
{
	CompareFunction compareFunction;

	require(not GetUpdateMode());
	require(GetSortTupleTable() == NULL);

	// On ne retrie que si necessaire
	compareFunction = GetCompareFunction();
	if (oaTuples.GetCompareFunction() != compareFunction)
	{
		SetSortTupleTable(this);
		oaTuples.SetCompareFunction(compareFunction);
		oaTuples.Sort();
		SetSortTupleTable(NULL);
	}
}

void KWTupleTable::SortByValues()
{
	CompareFunction compareFunction;

	require(not GetUpdateMode());
	require(GetSortTupleTable() == NULL);

	// On ne retrie que si necessaire
	compareFunction = GetCompareValuesFunction();
	if (oaTuples.GetCompareFunction() != compareFunction)
	{
		SetSortTupleTable(this);
		oaTuples.SetCompareFunction(compareFunction);
		oaTuples.Sort();
		SetSortTupleTable(NULL);
	}
}

void KWTupleTable::SortByDecreasingFrequencies()
{
	CompareFunction compareFunction;

	require(not GetUpdateMode());
	require(GetSortTupleTable() == NULL);

	// On ne retrie que si necessaire
	compareFunction = GetCompareDecreasingFrequenciesFunction();
	if (oaTuples.GetCompareFunction() != compareFunction)
	{
		SetSortTupleTable(this);
		oaTuples.SetCompareFunction(compareFunction);
		oaTuples.Sort();
		SetSortTupleTable(NULL);
	}
}

void KWTupleTable::SetUpdateMode(boolean bValue)
{
	int nIndex;
	KWTuple* tuple;
	POSITION position;

	require(bValue == not GetUpdateMode());
	require(GetSortTupleTable() == NULL);

	// On parametre la table de tuple a utiliser pour les fonctions de comparaison
	SetSortTupleTable(this);

	// Passage en mode edition
	if (bValue)
	{
		// Creation d'une liste triable pour inserer les tuples et garantir leur unicite
		slTuples = new SortedList(GetCompareFunction());
		oaTuples.SetCompareFunction(slTuples->GetCompareFunction());

		// On range tous les tuples existants dans la liste triee
		for (nIndex = 0; nIndex < oaTuples.GetSize(); nIndex++)
		{
			tuple = cast(KWTuple*, oaTuples.GetAt(nIndex));
			slTuples->Add(tuple);
		}
		assert(oaTuples.GetSize() == slTuples->GetCount());

		// On vide le tableau
		oaTuples.SetSize(0);

		// On met a dispotion un tuple d'edition
		assert(inputTuple == NULL);
		inputTuple = NewTuple();
		inputTuple->SetFrequency(1);
	}
	// Passage en mode consultation
	else
	{
		// On detruit le tuple d'entree
		assert(inputTuple != NULL);
		DeleteTuple(inputTuple);
		inputTuple = NULL;

		// On extrait les tuples de la liste triee vers le tableau
		oaTuples.SetSize(slTuples->GetCount());
		position = slTuples->GetHeadPosition();
		nIndex = 0;
		while (position != NULL)
		{
			tuple = cast(KWTuple*, slTuples->GetNext(position));
			oaTuples.SetAt(nIndex, tuple);
			nIndex++;
		}

		// On detruit la liste triee
		delete slTuples;
		slTuples = NULL;
		assert(nSize == oaTuples.GetSize());
	}

	// On remet a NULL la table de tuple a utiliser pour les fonctions de comparaison
	SetSortTupleTable(NULL);

	ensure(GetUpdateMode() == bValue);
}

const KWTuple* KWTupleTable::UpdateWithInputTuple()
{
	KWTuple* updateTuple;
	POSITION updatePosition;
	int i;

	require(GetUpdateMode());
	require(GetSortTupleTable() == NULL);
	require(inputTuple->GetFrequency() >= 1);

	// On parametre la table de tuple a utiliser pour les fonctions de comparaison
	// Ce parametrage est local a l'utilisation de cette methode, qui peut etre
	// appelee en sequence pour plusieurs tables differentes
	SetSortTupleTable(this);

	// Recherche dans la liste de la position d'un tuple ayant les meme valeurs que le tuple d'entree
	updatePosition = slTuples->Find(inputTuple);

	// Creation et insertion d'un nouveau tuple si necessaire
	if (updatePosition == NULL)
	{
		updateTuple = NewTuple();
		for (i = 0; i < GetAttributeNumber(); i++)
		{
			if (GetAttributeTypeAt(i) == KWType::Symbol)
				updateTuple->SetSymbolAt(i, inputTuple->GetSymbolAt(i));
			else
				updateTuple->SetContinuousAt(i, inputTuple->GetContinuousAt(i));
		}
		updateTuple->SetFrequency(inputTuple->GetFrequency());
		slTuples->Add(updateTuple);

		// Mise a jour de l'effectif
		nSize++;
	}
	// Sinon, mise a jour de l'effectif du tuple trouve
	else
	{
		updateTuple = cast(KWTuple*, slTuples->GetAt(updatePosition));
		updateTuple->SetFrequency(updateTuple->GetFrequency() + inputTuple->GetFrequency());
	}
	assert(nSize == slTuples->GetCount());

	// Mise a jour de l'effectif total
	nTotalFrequency += inputTuple->GetFrequency();

	// On remet a NULL la table de tuple a utiliser pour les fonctions de comparaison
	SetSortTupleTable(NULL);

	return updateTuple;
}

void KWTupleTable::BuildUnivariateTupleTable(const ALString& sAttributeName, KWTupleTable* outputTupleTable) const
{
	int nAttributeIndex;
	const ObjectArray* oaSortedTuples;
	ObjectArray oaExtractedTuples;
	int nTuple;
	const KWTuple* tuple;
	KWTuple* outputTuple;

	require(LookupAttributeIndex(sAttributeName) != -1);
	require(not GetUpdateMode());
	require(outputTupleTable != NULL);
	require(not outputTupleTable->GetUpdateMode());

	// Recherche de l'index de l'attribut a extraire
	nAttributeIndex = LookupAttributeIndex(sAttributeName);

	// Initialisation de la table de tuples en sortie
	outputTupleTable->CleanAll();
	outputTupleTable->AddAttribute(sAttributeName, GetAttributeTypeAt(nAttributeIndex));

	// Obtention d'un tableau des tuples trie correctement, ce qui passe par un tableau
	// intermediaire a trier s'il ne sagit pas du premier attribut ou si
	// la table courante n'est pas triee selon le premier attribut
	oaSortedTuples = &oaTuples;
	if (nAttributeIndex > 0 or oaTuples.GetCompareFunction() != GetCompareFunction())
	{
		oaExtractedTuples.CopyFrom(&oaTuples);
		SortTuplesByAttribute(sAttributeName, &oaExtractedTuples);
		oaSortedTuples = &oaExtractedTuples;
	}

	// Alimentation de la table de tuples en sortie dans le cas Symbol
	outputTuple = NULL;
	if (outputTupleTable->GetAttributeTypeAt(0) == KWType::Symbol)
	{
		for (nTuple = 0; nTuple < oaSortedTuples->GetSize(); nTuple++)
		{
			tuple = cast(const KWTuple*, oaSortedTuples->GetAt(nTuple));

			// Creation d'un nouveau tuple si necessaire
			if (outputTuple == NULL or outputTuple->GetSymbolAt(0) != tuple->GetSymbolAt(nAttributeIndex))
			{
				assert(outputTuple == NULL or
				       outputTuple->GetSymbolAt(0) < tuple->GetSymbolAt(nAttributeIndex));
				outputTuple = outputTupleTable->NewTuple();
				outputTuple->SetSymbolAt(0, tuple->GetSymbolAt(nAttributeIndex));
				outputTupleTable->oaTuples.Add(outputTuple);
			}

			// Mise a jour de son effectif
			outputTuple->SetFrequency(outputTuple->GetFrequency() + tuple->GetFrequency());
		}
	}
	// Alimentation de la table de tuples en sortie dans le cas Continuous
	else
	{
		for (nTuple = 0; nTuple < oaSortedTuples->GetSize(); nTuple++)
		{
			tuple = cast(const KWTuple*, oaSortedTuples->GetAt(nTuple));

			// Creation d'un nouveau tuple si necessaire
			if (outputTuple == NULL or
			    outputTuple->GetContinuousAt(0) != tuple->GetContinuousAt(nAttributeIndex))
			{
				assert(outputTuple == NULL or
				       outputTuple->GetContinuousAt(0) < tuple->GetContinuousAt(nAttributeIndex));
				outputTuple = outputTupleTable->NewTuple();
				outputTuple->SetContinuousAt(0, tuple->GetContinuousAt(nAttributeIndex));
				outputTupleTable->oaTuples.Add(outputTuple);
			}

			// Mise a jour de son effectif
			outputTuple->SetFrequency(outputTuple->GetFrequency() + tuple->GetFrequency());
		}
	}
	outputTupleTable->nSize = outputTupleTable->oaTuples.GetSize();
	outputTupleTable->nTotalFrequency = GetTotalFrequency();
}

KWTupleTable* KWTupleTable::Clone() const
{
	KWTupleTable* cloneTupleTable;
	cloneTupleTable = new KWTupleTable;
	cloneTupleTable->CopyFrom(this);
	return cloneTupleTable;
}

void KWTupleTable::CopyFrom(const KWTupleTable* sourceTupleTable)
{
	const KWTuple* sourceTuple;
	KWTuple* tuple;
	int nTuple;
	int i;

	require(not GetUpdateMode());
	require(sourceTupleTable != NULL);
	require(not sourceTupleTable->GetUpdateMode());

	// Nettoyage
	CleanAll();

	// Recopie des attributs
	svAttributeNames.CopyFrom(&sourceTupleTable->svAttributeNames);
	ivAttributeTypes.CopyFrom(&sourceTupleTable->ivAttributeTypes);
	nSize = sourceTupleTable->nSize;
	nTotalFrequency = sourceTupleTable->nTotalFrequency;

	// Recopie des tuples
	oaTuples.SetSize(sourceTupleTable->oaTuples.GetSize());
	oaTuples.SetCompareFunction(sourceTupleTable->oaTuples.GetCompareFunction());
	for (nTuple = 0; nTuple < oaTuples.GetSize(); nTuple++)
	{
		sourceTuple = sourceTupleTable->GetAt(nTuple);

		// Creation d'un nouveau tuple
		tuple = NewTuple();
		oaTuples.SetAt(nTuple, tuple);

		// Recopie du tuple source
		for (i = 0; i < GetAttributeNumber(); i++)
		{
			if (GetAttributeTypeAt(i) == KWType::Continuous)
			{
				tuple->SetContinuousAt(i, sourceTuple->GetContinuousAt(i));
			}
			else
			{
				assert(GetAttributeTypeAt(i) == KWType::Symbol);
				tuple->SetSymbolAt(i, sourceTuple->GetSymbolAt(i));
			}
		}
		tuple->SetFrequency(sourceTuple->GetFrequency());
	}
}

void KWTupleTable::ExportObjectArray(ObjectArray* oaExportedTuples) const
{
	require(oaExportedTuples != NULL);

	// En mode edition, on extrait les tuples de la liste triee
	if (slTuples != NULL)
		slTuples->ExportObjectArray(oaExportedTuples);
	// En mode consultation, on copie le tableau de tuples
	else
		oaExportedTuples->CopyFrom(&oaTuples);
}

void KWTupleTable::Write(ostream& ost) const
{
	const int nMax = 10;
	int i;
	const KWTuple* tuple;

	require(not GetUpdateMode());

	ost << GetClassLabel() << " " << GetObjectLabel() << "\n";

	// On affiche le debut et la fin de la table
	for (i = 0; i < min(nMax / 2, GetSize()); i++)
	{
		tuple = GetAt(i);
		ost << "\t";
		tuple->FullWrite(this, ost);
		ost << "\n";
	}
	if (GetSize() > nMax)
		ost << "\t...\n";
	for (i = max(1 + nMax / 2, GetSize() - nMax / 2); i < GetSize(); i++)
	{
		tuple = GetAt(i);
		ost << "\t";
		tuple->FullWrite(this, ost);
		ost << "\n";
	}
}

void KWTupleTable::WriteFile(const ALString& sFileName)
{
	fstream fstReport;
	boolean bOk;
	int nAttribute;
	int nTuple;
	const KWTuple* tuple;

	// Ouverture du fichier en ecriture
	bOk = FileService::OpenOutputFile(sFileName, fstReport);
	if (bOk)
	{
		// Ecriture de l'entete
		for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
		{
			fstReport << GetAttributeNameAt(nAttribute);
			fstReport << "\t";
		}
		fstReport << "Frequency\n";

		// Ecriture des tuples
		for (nTuple = 0; nTuple < GetSize(); nTuple++)
		{
			tuple = GetAt(nTuple);

			// Valeurs des attributs du tuple
			for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
			{
				if (GetAttributeTypeAt(nAttribute) == KWType::Symbol)
					fstReport << tuple->GetSymbolAt(nAttribute).GetValue();
				else
					fstReport
					    << KWContinuous::ContinuousToString(tuple->GetContinuousAt(nAttribute));
				fstReport << "\t";
			}
			fstReport << tuple->GetFrequency() << "\n";
		}

		// Fermeture du fichier
		FileService::CloseOutputFile(sFileName, fstReport);
	}
}

longint KWTupleTable::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWTupleTable);
	lUsedMemory += svAttributeNames.GetUsedMemory() - sizeof(StringVector);
	lUsedMemory += ivAttributeTypes.GetUsedMemory() - sizeof(IntVector);
	lUsedMemory += oaTuples.GetUsedMemory() - sizeof(ObjectArray);
	lUsedMemory += oaTuples.GetSize() * (sizeof(KWTuple) + (GetAttributeNumber() - 1) * sizeof(KWValue));
	return lUsedMemory;
}

longint KWTupleTable::ComputeNecessaryMemory(int nTupleNumber, int nAttributeNumber)
{
	longint lNecessaryMemory;

	require(nTupleNumber >= 0);
	require(nAttributeNumber >= 0);

	// Memoire necessaire pour le stockage de la table
	lNecessaryMemory = sizeof(KWTupleTable);
	lNecessaryMemory += nAttributeNumber * (sizeof(ALString) + 20 + sizeof(int));
	lNecessaryMemory += nTupleNumber * (sizeof(void*) + sizeof(KWTuple) + (nAttributeNumber - 1) * sizeof(KWValue));
	return lNecessaryMemory;
}

longint KWTupleTable::ComputeNecessaryBuildingMemory(int nTupleNumber)
{
	longint lNecessaryMemory;
	SortedList slTmp(ObjectCompare);

	require(nTupleNumber >= 0);

	// Memoire necessaire pour l'alimentation de la table au moyen de la liste triee
	lNecessaryMemory = slTmp.GetUsedMemory() + nTupleNumber * slTmp.GetUsedMemoryPerElement();
	return lNecessaryMemory;
}

const ALString KWTupleTable::GetClassLabel() const
{
	return "Tuple table";
}

const ALString KWTupleTable::GetObjectLabel() const
{
	ALString sObjectLabel;
	int nTupleCount;
	int nAttribute;

	// Nombre de tuples, en mode update ou non
	if (slTuples != NULL)
		nTupleCount = slTuples->GetCount();
	else
		nTupleCount = oaTuples.GetSize();

	// Calcul du libelle
	sObjectLabel = "(";
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		if (nAttribute > 0)
			sObjectLabel += ", ";
		sObjectLabel += GetAttributeNameAt(nAttribute);
	}
	sObjectLabel += ") [";
	sObjectLabel += IntToString(nTupleCount);
	sObjectLabel += ", ";
	sObjectLabel += IntToString(GetTotalFrequency());
	sObjectLabel += "]";
	return sObjectLabel;
}

void KWTupleTable::Test()
{
	const int nValueNumber = 1000;
	const ALString sPrefix = "v";
	Timer timerUpdate;
	Timer timerSort;
	Timer timerSortUserValues;
	Timer timerSortDecreasingFrequencies;
	int nDistinctValueNumber;
	int i;
	int nValue;
	KWTupleTable tupleTableTest;
	KWTuple* inputTuple;
	ContinuousVector cvValues;
	SymbolVector svValues;

	// Initialisation de la structure de la table de tuples
	tupleTableTest.AddAttribute("VarS", KWType::Symbol);
	tupleTableTest.AddAttribute("VarN", KWType::Continuous);
	cout << "Empty table\n" << tupleTableTest << endl;

	// Initialisation des valeurs
	cvValues.SetSize(nValueNumber);
	svValues.SetSize(nValueNumber);
	for (i = 0; i < nValueNumber; i++)
	{
		cvValues.SetAt(i, i + 1);
		svValues.SetAt(i, Symbol(sPrefix + IntToString(i + 1)));
	}

	// Boucle de test, en remplissant la table
	nDistinctValueNumber = nValueNumber;
	while (nDistinctValueNumber > 1)
	{
		// Preparation de la table au mode edition
		tupleTableTest.DeleteAll();
		tupleTableTest.SetUpdateMode(true);
		inputTuple = tupleTableTest.GetInputTuple();
		timerUpdate.Reset();
		timerUpdate.Start();

		// Alimentation de la table de tuples
		for (i = 0; i < nValueNumber; i++)
		{
			inputTuple->SetSymbolAt(0, svValues.GetAt(i % nDistinctValueNumber));
			inputTuple->SetContinuousAt(1, cvValues.GetAt(i % nDistinctValueNumber));
			tupleTableTest.UpdateWithInputTuple();
		}

		// Ajout d'un petit nombre pour avoir des effectifs differents
		for (nValue = 0; nValue < 10; nValue++)
		{
			for (i = 0; i <= nValue; i++)
			{
				inputTuple->SetSymbolAt(0, svValues.GetAt(nValue % nDistinctValueNumber));
				inputTuple->SetContinuousAt(1, cvValues.GetAt(nValue % nDistinctValueNumber));
				tupleTableTest.UpdateWithInputTuple();
			}
		}

		// Repassage en mode consultation
		tupleTableTest.SetUpdateMode(false);
		timerUpdate.Stop();

		// Tri par valeur utilisateur
		timerSortUserValues.Reset();
		timerSortUserValues.Start();
		tupleTableTest.SortByValues();
		timerSortUserValues.Stop();

		// Tri par valeur
		timerSort.Reset();
		timerSort.Start();
		tupleTableTest.Sort();
		timerSort.Stop();

		// Tri par valeur
		timerSortDecreasingFrequencies.Reset();
		timerSortDecreasingFrequencies.Start();
		tupleTableTest.SortByDecreasingFrequencies();
		timerSortDecreasingFrequencies.Stop();

		// Affichage des resultats
		cout << "SYSTEM\t" << timerUpdate.GetElapsedTime() << "\t" << timerSort.GetElapsedTime() << "\t"
		     << timerSortUserValues.GetElapsedTime() << "\t" << timerSortDecreasingFrequencies.GetElapsedTime()
		     << endl;
		cout << "Table with " << nDistinctValueNumber << " distinct values\n" << tupleTableTest << endl;

		// Diminution du nombre de valeurs distinctes
		nDistinctValueNumber /= 10;
	}

	// Nettoyage complet
	tupleTableTest.CleanAll();
	cout << "Fully clean table\n" << tupleTableTest << endl;

	// Alimentation avec des tuples sans attributs
	tupleTableTest.SetUpdateMode(true);
	inputTuple = tupleTableTest.GetInputTuple();
	for (i = 0; i < nValueNumber; i++)
		tupleTableTest.UpdateWithInputTuple();
	tupleTableTest.SetUpdateMode(false);
	cout << "Table without values, with " << nValueNumber << " records\n" << tupleTableTest << endl;
}

KWTuple* KWTupleTable::NewTuple() const
{
	KWTuple* newTuple;
	void* pTupleMemory;
	int nMemorySize;
	int i;

	// Calcul de la taille a allouer (en prevoyant si necessaire la place pour l'ensemble des valeurs)
	nMemorySize = sizeof(KWTuple);
	if (svAttributeNames.GetSize() > 0)
		nMemorySize += (svAttributeNames.GetSize() - 1) * sizeof(KWValue);

	// Creation des donnees d'un symbol
	// On alloue le nombre de caracteres necessaire pour stocker la chaine de caracateres
	// en plus des donnees du symbol (le caractere fin de chaine '\0' est deja prevu)
	pTupleMemory = NewMemoryBlock(nMemorySize);

	// On utilise le "placement new" pour appeler un constructeur avec de la memoire preallouee (attention, C++
	// avance)
	newTuple = new (pTupleMemory) KWTuple;
	assert((void*)newTuple == pTupleMemory);

	// Initialisation du tuple
	debug(newTuple->tupleTable = this);
	newTuple->nFrequency = 0;
	for (i = svAttributeNames.GetSize() - 1; i >= 0; i--)
		newTuple->tupleValues[i].Init();
	return newTuple;
}

void KWTupleTable::DeleteTuple(KWTuple* tuple) const
{
	int i;

	require(tuple != NULL);
	debug(require(tuple->tupleTable == this));

	// On reintilaise les valeur Symbol, pour maintenir correctement leur compteur de reference
	for (i = svAttributeNames.GetSize() - 1; i >= 0; i--)
	{
		if (ivAttributeTypes.GetAt(i) == KWType::Symbol)
			tuple->tupleValues[i].ResetSymbol();
	}

	// Destruction du tuple
	DeleteMemoryBlock(tuple);
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_TupleTable

PLShared_TupleTable::PLShared_TupleTable() {}

PLShared_TupleTable::~PLShared_TupleTable() {}

void PLShared_TupleTable::Test()
{
	KWTupleTable tupleTable1;
	KWTupleTable tupleTable2;
	KWTuple* inputTuple;
	int i;
	PLShared_TupleTable shared_tupleTable;
	PLSerializer serializer;
	ALString sTmp;

	// Initialisation de la structure de la table de tuples
	tupleTable1.AddAttribute("VarS", KWType::Symbol);
	tupleTable1.AddAttribute("VarN", KWType::Continuous);
	tupleTable1.SetUpdateMode(true);
	inputTuple = tupleTable1.GetInputTuple();
	for (i = 0; i < 10; i++)
	{
		inputTuple->SetSymbolAt(0, Symbol(sTmp + "v" + IntToString(1 + i % 3)));
		inputTuple->SetContinuousAt(1, 1 + i % 3);
		tupleTable1.UpdateWithInputTuple();
	}
	tupleTable1.SetUpdateMode(false);

	// Serialisation
	serializer.OpenForWrite(NULL);
	shared_tupleTable.bIsDeclared = true;
	shared_tupleTable.GetTupleTable()->CopyFrom(&tupleTable1);
	shared_tupleTable.Serialize(&serializer);
	serializer.Close();

	// Deserialization
	serializer.OpenForRead(NULL);
	shared_tupleTable.Deserialize(&serializer);
	serializer.Close();
	tupleTable2.CopyFrom(shared_tupleTable.GetConstTupleTable());

	// Affichage
	cout << "Tuple table 1" << endl;
	cout << tupleTable1 << endl << endl;
	cout << "Tuple table 2" << endl;
	cout << tupleTable2 << endl << endl;
}

void PLShared_TupleTable::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	const KWTupleTable* tupleTable;
	const KWTuple* tuple;
	int nTuple;
	int i;
	PLShared_Symbol sharedSymbol;
	PLShared_Continuous sharedContinuous;
	int nSortFunctionIndex;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	tupleTable = cast(const KWTupleTable*, o);

	// Serialisation des attributs
	serializer->PutStringVector(&tupleTable->svAttributeNames);
	serializer->PutIntVector(&tupleTable->ivAttributeTypes);

	// Serialisation de l'effectif total
	serializer->PutInt(tupleTable->nTotalFrequency);

	// Serialisation des tuples
	serializer->PutInt(tupleTable->GetSize());
	for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
	{
		tuple = tupleTable->GetAt(nTuple);

		// Serialisation du tuple courant
		for (i = 0; i < tupleTable->GetAttributeNumber(); i++)
		{
			if (tupleTable->GetAttributeTypeAt(i) == KWType::Continuous)
			{
				sharedContinuous = tuple->GetContinuousAt(i);
				sharedContinuous.Serialize(serializer);
			}
			else
			{
				assert(tupleTable->GetAttributeTypeAt(i) == KWType::Symbol);
				sharedSymbol = tuple->GetSymbolAt(i);
				sharedSymbol.Serialize(serializer);
			}
		}
		serializer->PutInt(tuple->GetFrequency());
	}

	// Serialisation de la fonction de tri
	nSortFunctionIndex = 0;
	if (tupleTable->oaTuples.GetCompareFunction() == tupleTable->GetCompareFunction())
		nSortFunctionIndex = 1;
	else if (tupleTable->oaTuples.GetCompareFunction() == tupleTable->GetCompareValuesFunction())
		nSortFunctionIndex = 2;
	else if (tupleTable->oaTuples.GetCompareFunction() == tupleTable->GetCompareDecreasingFrequenciesFunction())
		nSortFunctionIndex = 3;
	assert(nSortFunctionIndex != 0);
	serializer->PutInt(nSortFunctionIndex);
}

void PLShared_TupleTable::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWTupleTable* tupleTable;
	KWTuple* tuple;
	int nTuple;
	int i;
	int nSize;
	PLShared_Symbol sharedSymbol;
	PLShared_Continuous sharedContinuous;
	int nSortFunctionIndex;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	tupleTable = cast(KWTupleTable*, o);

	// Nettoyage prealable
	tupleTable->CleanAll();

	// Deserialisation des attributs
	serializer->GetStringVector(&tupleTable->svAttributeNames);
	serializer->GetIntVector(&tupleTable->ivAttributeTypes);

	// Deserialisation de l'effectif total
	tupleTable->nTotalFrequency = serializer->GetInt();

	// Deserialisation des tuples
	nSize = serializer->GetInt();
	tupleTable->nSize = nSize;
	tupleTable->oaTuples.SetSize(nSize);
	for (nTuple = 0; nTuple < nSize; nTuple++)
	{
		// Creation d'un tuple
		tuple = tupleTable->NewTuple();
		tupleTable->oaTuples.SetAt(nTuple, tuple);

		// Deserialisation du tuple courant
		for (i = 0; i < tupleTable->GetAttributeNumber(); i++)
		{
			if (tupleTable->GetAttributeTypeAt(i) == KWType::Continuous)
			{
				sharedContinuous.Deserialize(serializer);
				tuple->SetContinuousAt(i, sharedContinuous);
			}
			else
			{
				assert(tupleTable->GetAttributeTypeAt(i) == KWType::Symbol);
				sharedSymbol.Deserialize(serializer);
				tuple->SetSymbolAt(i, sharedSymbol);
			}
		}
		tuple->SetFrequency(serializer->GetInt());
	}

	// Deserialisation de la fonction de tri, et tri seulement si necessaire
	nSortFunctionIndex = serializer->GetInt();
	assert(nSortFunctionIndex != 0);
	if (nSortFunctionIndex == 1)
	{
		// Le tri est necessaire en cas de presence d'attributs Symbol, dont la valeur
		// peut etre differente selon le process
		tupleTable->Sort();
		assert(tupleTable->oaTuples.GetCompareFunction() == tupleTable->GetCompareFunction());
	}
	else if (nSortFunctionIndex == 2)
		tupleTable->oaTuples.SetCompareFunction(tupleTable->GetCompareValuesFunction());
	else if (nSortFunctionIndex == 3)
		tupleTable->oaTuples.SetCompareFunction(tupleTable->GetCompareDecreasingFrequenciesFunction());
}

Object* PLShared_TupleTable::Create() const
{
	return new KWTupleTable;
}