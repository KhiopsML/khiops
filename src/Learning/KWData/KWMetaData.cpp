// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMetaData.h"

/////////////////////////////////////////////////////////////////////
// Classe KWMetaData

KWMetaData::KWMetaData()
{
	oaKeyValuePairs = NULL;
}

KWMetaData::~KWMetaData()
{
	if (oaKeyValuePairs != NULL)
	{
		oaKeyValuePairs->DeleteAll();
		delete oaKeyValuePairs;
	}
}

void KWMetaData::SetStringValueAt(const ALString& sKey, const ALString& sValue)
{
	KWKeyValuePair* keyValuePair;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	keyValuePair = GetKeyValuePairAt(sKey);
	keyValuePair->SetStringValue(sValue);
}

void KWMetaData::SetDoubleValueAt(const ALString& sKey, double dValue)
{
	KWKeyValuePair* keyValuePair;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	keyValuePair = GetKeyValuePairAt(sKey);
	keyValuePair->SetDoubleValue(dValue);
}

void KWMetaData::SetNoValueAt(const ALString& sKey)
{
	KWKeyValuePair* keyValuePair;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	keyValuePair = GetKeyValuePairAt(sKey);
	keyValuePair->SetNoValue();
}

void KWMetaData::RemoveKey(const ALString& sKey)
{
	int nIndex;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	// Recherche de l'index de la paire a detruire
	nIndex = SearchKeyIndex(sKey);

	// Destruction de la paire correspondante si trouvee
	if (nIndex != -1)
	{
		assert(oaKeyValuePairs != NULL);

		// Destruction a l'emplacement specifie
		delete oaKeyValuePairs->GetAt(nIndex);

		// Supression de l'endroit specifie dans el tableau, ce qui prvoque un decallage des
		// elements suivants et un retaillage du tableau
		oaKeyValuePairs->RemoveAt(nIndex);

		// Destruction du tableau si vide
		if (oaKeyValuePairs->GetSize() == 0)
			RemoveAllKeys();
	}
}

void KWMetaData::RemoveAllKeys()
{
	if (oaKeyValuePairs != NULL)
	{
		oaKeyValuePairs->DeleteAll();
		delete oaKeyValuePairs;
		oaKeyValuePairs = NULL;
	}
}

boolean KWMetaData::IsKeyPresent(const ALString& sKey) const
{
	int nIndex;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	nIndex = SearchKeyIndex(sKey);
	return (nIndex != -1);
}

boolean KWMetaData::IsStringTypeAt(const ALString& sKey) const
{
	int nIndex;
	KWKeyValuePair* keyValuePair;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	// Recherche de l'index de la paire a detruire
	nIndex = SearchKeyIndex(sKey);

	// Type String si pas de cle
	if (nIndex == -1)
		return false;
	// Sinon, on se base sur la type de la valeur associee a la cle
	else
	{
		keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
		return keyValuePair->IsStringType();
	}
}

boolean KWMetaData::IsDoubleTypeAt(const ALString& sKey) const
{
	int nIndex;
	KWKeyValuePair* keyValuePair;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	// Recherche de l'index de la paire a detruire
	nIndex = SearchKeyIndex(sKey);

	// Type String si pas de cle
	if (nIndex == -1)
		return false;
	// Sinon, on se base sur la type de la valeur associee a la cle
	else
	{
		keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
		return keyValuePair->IsDoubleType();
	}
}

boolean KWMetaData::IsMissingTypeAt(const ALString& sKey) const
{
	int nIndex;
	KWKeyValuePair* keyValuePair;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	// Recherche de l'index de la paire a detruire
	nIndex = SearchKeyIndex(sKey);

	// Type String si pas de cle
	if (nIndex == -1)
		return false;
	// Sinon, on se base sur la type de la valeur associee a la cle
	else
	{
		keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
		return keyValuePair->IsMissingType();
	}
}

const ALString KWMetaData::GetStringValueAt(const ALString& sKey) const
{
	static const ALString sEmptyString = "";
	int nIndex;
	KWKeyValuePair* keyValuePair;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	// Recherche de l'index de la paire a detruire
	nIndex = SearchKeyIndex(sKey);

	// Renvoie "" si pas de cle
	if (nIndex == -1)
		return sEmptyString;
	// Sinon, on se base sur la valeur associee a la cle
	else
	{
		keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
		return keyValuePair->GetStringValue();
	}
}

double KWMetaData::GetDoubleValueAt(const ALString& sKey) const
{
	int nIndex;
	KWKeyValuePair* keyValuePair;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	// Recherche de l'index de la paire a detruire
	nIndex = SearchKeyIndex(sKey);

	// Renvoie 0 si pas de cle
	if (nIndex == -1)
		return 0;
	// Sinon, on se base sur la valeur associee a la cle
	else
	{
		keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
		return keyValuePair->GetDoubleValue();
	}
}

const ALString KWMetaData::GetExternalValueAt(const ALString& sKey) const
{
	ALString sExternalValue;

	int nIndex;
	KWKeyValuePair* keyValuePair;

	require(sKey != "" and KWKeyValuePair::CheckKey(sKey));

	// Recherche de l'index de la paire a detruire
	nIndex = SearchKeyIndex(sKey);

	// Recherche de la valeur si cle presente
	if (nIndex != -1)
	{
		keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
		if (keyValuePair->IsDoubleType())
			sExternalValue = DoubleToString(keyValuePair->GetDoubleValue());
		else if (keyValuePair->IsStringType())
		{
			sExternalValue = '"';
			sExternalValue += keyValuePair->GetStringValue();
			sExternalValue += '"';
		}
	}
	return sExternalValue;
}

int KWMetaData::GetKeyNumber() const
{
	if (oaKeyValuePairs == NULL)
		return 0;
	else
		return oaKeyValuePairs->GetSize();
}

const ALString& KWMetaData::GetKeyAt(int nIndex) const
{
	KWKeyValuePair* keyValuePair;

	require(0 <= nIndex and nIndex < GetKeyNumber());

	keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
	return keyValuePair->GetKey();
}

KWMetaData* KWMetaData::Clone() const
{
	KWMetaData* keyValuePairsClone;

	keyValuePairsClone = new KWMetaData;
	keyValuePairsClone->CopyFrom(this);
	return keyValuePairsClone;
}

void KWMetaData::CopyFrom(const KWMetaData* sourceKeyValuePairs)
{
	int nIndex;
	KWKeyValuePair* keyValuePair;
	KWKeyValuePair* keyValuePairClone;

	require(sourceKeyValuePairs != NULL);

	// Duplication des paires cle valeur, apres nettoyage initial
	RemoveAllKeys();
	if (sourceKeyValuePairs->GetKeyNumber() > 0)
	{
		oaKeyValuePairs = new ObjectArray;
		oaKeyValuePairs->SetSize(sourceKeyValuePairs->oaKeyValuePairs->GetSize());
		for (nIndex = 0; nIndex < sourceKeyValuePairs->oaKeyValuePairs->GetSize(); nIndex++)
		{
			keyValuePair = cast(KWKeyValuePair*, sourceKeyValuePairs->oaKeyValuePairs->GetAt(nIndex));
			keyValuePairClone = keyValuePair->Clone();
			oaKeyValuePairs->SetAt(nIndex, keyValuePairClone);
		}
	}
}

longint KWMetaData::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWMetaData);
	if (oaKeyValuePairs != NULL)
		lUsedMemory += sizeof(ObjectArray) + oaKeyValuePairs->GetOverallUsedMemory();
	return lUsedMemory;
}

longint KWMetaData::ComputeHashValue() const
{
	longint lHash;
	KWKeyValuePair* keyValuePair;
	int nIndex;

	lHash = 0;
	for (nIndex = 0; nIndex < GetKeyNumber(); nIndex++)
	{
		keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
		lHash = LongintUpdateHashValue(lHash, keyValuePair->ComputeHashValue());
	}
	return lHash;
}

void KWMetaData::Write(ostream& ost) const
{
	int nIndex;
	KWKeyValuePair* keyValuePair;

	// Affichage de chaque paire cle valeur
	if (oaKeyValuePairs != NULL)
	{
		for (nIndex = 0; nIndex < oaKeyValuePairs->GetSize(); nIndex++)
		{
			keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));

			// Blanc entre chaque paire
			if (nIndex > 0)
				ost << ' ';
			keyValuePair->Write(ost);
		}
	}
}

void KWMetaData::WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey)
{
	int nIndex;
	KWKeyValuePair* keyValuePair;

	// Affichage de chaque paire cle valeur
	fJSON->BeginKeyObject(sKey);
	if (oaKeyValuePairs != NULL)
	{
		// Ecriture des paires cle-valeur
		for (nIndex = 0; nIndex < oaKeyValuePairs->GetSize(); nIndex++)
		{
			keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));

			// Valeur
			if (keyValuePair->IsStringType())
				fJSON->WriteKeyString(keyValuePair->GetKey(), keyValuePair->GetStringValue());
			else if (keyValuePair->IsDoubleType())
				fJSON->WriteKeyDouble(keyValuePair->GetKey(), keyValuePair->GetDoubleValue());
			else if (keyValuePair->IsMissingType())
				fJSON->WriteKeyBoolean(keyValuePair->GetKey(), true);
		}
	}
	fJSON->EndObject();
}

void KWMetaData::Test()
{
	KWMetaData keyValuePairs;
	KWMetaData* keyValuePairsClone;
	int nIndex;
	ALString sKey;

	// Initialisation
	keyValuePairs.SetStringValueAt("S1", "value 1");
	keyValuePairs.SetStringValueAt("S2", "value 2");
	keyValuePairs.SetStringValueAt("S3", "value 3");
	keyValuePairs.SetDoubleValueAt("D1", 1);
	keyValuePairs.SetDoubleValueAt("D2", 2);
	keyValuePairs.SetDoubleValueAt("D3", 3);
	keyValuePairs.SetNoValueAt("N1");
	keyValuePairs.SetNoValueAt("N2");
	cout << "Initialisation\t" << keyValuePairs << endl;

	// Insertions et changement de valeurs
	keyValuePairs.SetStringValueAt("S2", "new value 2");
	keyValuePairs.SetStringValueAt("S4", "value 4");
	keyValuePairs.SetDoubleValueAt("D0", 0);
	keyValuePairs.SetNoValueAt("N0");
	cout << "Inserts and value changes\t" << keyValuePairs << endl;

	// Supression de cles
	keyValuePairs.RemoveKey("S2");
	keyValuePairs.RemoveKey("D2");
	keyValuePairs.RemoveKey("N1");
	cout << "Remove keys\t" << keyValuePairs << endl;

	// Duplication
	keyValuePairsClone = keyValuePairs.Clone();
	cout << "Clone\t" << *keyValuePairsClone << endl;
	delete keyValuePairsClone;

	// Parcours de toutes les cles
	cout << "Index\tKey\tPresent\tS type\tD type\tS value\tD value\n";
	for (nIndex = -1; nIndex < keyValuePairs.GetKeyNumber(); nIndex++)
	{
		// Recherche de la cle, sauf pour -1 ou on prend une cle inexistante
		if (nIndex >= 0)
			sKey = keyValuePairs.GetKeyAt(nIndex);
		else
			sKey = "BadKey";

		// Affichage des caracteristiques
		cout << nIndex << "\t";
		cout << sKey << "\t";
		cout << keyValuePairs.IsKeyPresent(sKey) << "\t";
		cout << keyValuePairs.IsStringTypeAt(sKey) << "\t";
		cout << keyValuePairs.IsDoubleTypeAt(sKey) << "\t";
		cout << keyValuePairs.GetStringValueAt(sKey) << "\t";
		cout << keyValuePairs.GetDoubleValueAt(sKey) << "\n";
	}
}

KWKeyValuePair* KWMetaData::GetKeyValuePairAt(const ALString& sKey)
{
	int nIndex;
	KWKeyValuePair* keyValuePair;
	int nCompare;

	// Recherche de la cle dans le tableau des paires
	if (oaKeyValuePairs != NULL)
	{
		for (nIndex = 0; nIndex < oaKeyValuePairs->GetSize(); nIndex++)
		{
			keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
			nCompare = sKey.Compare(keyValuePair->GetKey());

			// On retourne la paire si on a trouve la bonne cle
			if (nCompare == 0)
				return keyValuePair;

			// Les cles etant ranges par ordre alphabetique, on insere une nouvelle paire si on a depasse
			// toutes les cles plus petites
			if (nCompare < 0)
			{
				keyValuePair = new KWKeyValuePair;
				keyValuePair->SetKey(sKey);
				oaKeyValuePairs->InsertAt(nIndex, keyValuePair);
				return keyValuePair;
			}
		}
	}

	// Insertion d'une nouvelle cle en fin de tableau (cree si necessaire)
	keyValuePair = new KWKeyValuePair;
	keyValuePair->SetKey(sKey);
	if (oaKeyValuePairs == NULL)
		oaKeyValuePairs = new ObjectArray;
	oaKeyValuePairs->Add(keyValuePair);
	return keyValuePair;
}

int KWMetaData::SearchKeyIndex(const ALString& sKey) const
{
	int nIndex;
	KWKeyValuePair* keyValuePair;
	int nCompare;

	// Recherche de la cle dans le tableau des paires
	if (oaKeyValuePairs != NULL)
	{
		for (nIndex = 0; nIndex < oaKeyValuePairs->GetSize(); nIndex++)
		{
			keyValuePair = cast(KWKeyValuePair*, oaKeyValuePairs->GetAt(nIndex));
			nCompare = sKey.Compare(keyValuePair->GetKey());

			// On retourne l'index si on a trouve la bonne cle
			if (nCompare == 0)
				return nIndex;

			// Les cles etant ranges par ordre alphabetique, on retourne -1 si on a depasse toutes les cles
			// plus petites
			if (nCompare < 0)
				return -1;
		}
	}

	// On retourne -1 si on a pas trouve la cle
	return -1;
}

/////////////////////////////////////////////////////////////////////
// Classe KWKeyValuePair

KWKeyValuePair::KWKeyValuePair()
{
	nType = MissingType;
	kwvValue.Init();
}

KWKeyValuePair::~KWKeyValuePair()
{
	if (nType == StringType)
		kwvValue.ResetSymbol();
}

void KWKeyValuePair::SetKey(const ALString& sValue)
{
	require(CheckKey(sValue));
	usKey.SetValue(sValue);
}

const ALString& KWKeyValuePair::GetKey() const
{
	return usKey.GetValue();
}

boolean KWKeyValuePair::CheckKey(const ALString& sValue)
{
	boolean bOk;
	int i;
	char c;

	// Test si la cle nom est compose de lettres, chiffres ou '_'
	bOk = (sValue.GetLength() > 0);
	if (bOk)
	{
		c = sValue.GetAt(0);
		bOk = ('a' <= c and c <= 'z') or ('A' <= c and c <= 'Z') or c == '_';
		for (i = 1; i < sValue.GetLength(); i++)
		{
			c = sValue.GetAt(i);
			bOk = bOk and (('a' <= c and c <= 'z') or ('A' <= c and c <= 'Z') or ('0' <= c and c <= '9') or
				       c == '_');
		}
	}
	return bOk;
}

boolean KWKeyValuePair::IsDoubleType() const
{
	return (nType == DoubleType);
}

boolean KWKeyValuePair::IsStringType() const
{
	return (nType == StringType);
}

boolean KWKeyValuePair::IsMissingType() const
{
	return (nType == MissingType);
}

void KWKeyValuePair::SetStringValue(const ALString& sValue)
{
	// Si necessaire, mise a jour de la constante par sa valeur par defaut
	if (nType != StringType)
		kwvValue.Init();

	// Memorisation du type et de la valeur
	nType = StringType;
	kwvValue.SetSymbol(Symbol(sValue));
}

const ALString KWKeyValuePair::GetStringValue() const
{
	if (IsStringType())
		return kwvValue.GetSymbol().GetValue();
	else
		return "";
}

void KWKeyValuePair::SetDoubleValue(double dValue)
{
	// Si necessaire, il faut remettre au prealable la valeur par defaut des Symbol
	// pour forcer l'eventuelle liberation du Symbol precedent
	if (nType == StringType)
		kwvValue.ResetSymbol();

	// Memorisation du type et de la valeur
	nType = DoubleType;
	kwvValue.SetContinuous(dValue);
}

double KWKeyValuePair::GetDoubleValue() const
{
	if (IsDoubleType())
		return kwvValue.GetContinuous();
	else
		return 0;
}

void KWKeyValuePair::SetNoValue()
{
	// Si necessaire, il faut remettre au prealable la valeur par defaut des Symbol
	// pour forcer l'eventuelle liberation du Symbol precedent
	if (nType == StringType)
		kwvValue.ResetSymbol();
	else
		kwvValue.Init();

	// Memorisation du type
	nType = MissingType;
}

KWKeyValuePair* KWKeyValuePair::Clone() const
{
	KWKeyValuePair* keyValuePairClone;

	keyValuePairClone = new KWKeyValuePair;
	keyValuePairClone->CopyFrom(this);
	return keyValuePairClone;
}

void KWKeyValuePair::CopyFrom(const KWKeyValuePair* sourceKeyValuePair)
{
	require(sourceKeyValuePair != NULL);

	usKey = sourceKeyValuePair->usKey;
	if (sourceKeyValuePair->nType == StringType)
		SetStringValue(sourceKeyValuePair->GetStringValue());
	else if (sourceKeyValuePair->nType == DoubleType)
		SetDoubleValue(sourceKeyValuePair->GetDoubleValue());
	else
		SetNoValue();
}

longint KWKeyValuePair::GetUsedMemory() const
{
	return sizeof(KWKeyValuePair);
}

longint KWKeyValuePair::ComputeHashValue() const
{
	longint lHash;

	lHash = HashValue(GetKey());
	lHash = LongintUpdateHashValue(lHash, nType * 101);
	if (nType == StringType)
		lHash = LongintUpdateHashValue(lHash, HashValue(kwvValue.GetSymbol().GetValue()));
	else if (nType == DoubleType)
		lHash = LongintUpdateHashValue(lHash, HashValue(DoubleToString(kwvValue.GetContinuous())));
	return lHash;
}

void KWKeyValuePair::Write(ostream& ost) const
{
	ALString sStringValue;
	int nLength;
	int i;
	char c;

	// Cle
	ost << '<' << usKey;

	// Valeurs
	if (nType == StringType)
	{
		// Entre double-quotes, avec doublement des double-quotes internes
		ost << '=' << '"';
		sStringValue = GetStringValue();
		nLength = sStringValue.GetLength();
		for (i = 0; i < nLength; i++)
		{
			c = sStringValue.GetAt(i);
			if (c == '"')
				ost << '"';
			ost << c;
		}
		ost << '"';
	}
	else if (nType == DoubleType)
		ost << '=' << GetDoubleValue();

	// Fin de la paire
	ost << '>';
}

///////////////////////////////////////////////////
// Classe PLShared_MetaData

PLShared_MetaData::PLShared_MetaData() {}

PLShared_MetaData::~PLShared_MetaData() {}

void PLShared_MetaData::SetMetaData(KWMetaData* metaData)
{
	require(metaData != NULL);
	SetObject(metaData);
}

KWMetaData* PLShared_MetaData::GetMetaData()
{
	return cast(KWMetaData*, GetObject());
}

void PLShared_MetaData::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWMetaData* metaData;
	KWKeyValuePair* keyValuePair;
	int i;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(o != NULL);

	// Acces a l'objet a deserialiser
	metaData = cast(KWMetaData*, o);

	// Serialisation des meta-donnees sous forme d'une liste de valeurs typees
	// Attention, le tableau est a NULL s'il est vide
	if (metaData->oaKeyValuePairs == NULL)
		serializer->PutInt(0);
	else
	{
		assert(metaData->oaKeyValuePairs->GetSize() > 0);
		serializer->PutInt(metaData->oaKeyValuePairs->GetSize());
		for (i = 0; i < metaData->oaKeyValuePairs->GetSize(); i++)
		{
			keyValuePair = cast(KWKeyValuePair*, metaData->oaKeyValuePairs->GetAt(i));
			serializer->PutString(keyValuePair->usKey.GetValue());
			serializer->PutInt(keyValuePair->nType);
			if (keyValuePair->nType == KWKeyValuePair::StringType)
				serializer->PutString(keyValuePair->GetStringValue());
			else if (keyValuePair->nType == KWKeyValuePair::DoubleType)
				serializer->PutDouble(keyValuePair->GetDoubleValue());
		}
	}
}

void PLShared_MetaData::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWMetaData* metaData;
	int nSize;
	KWKeyValuePair* keyValuePair;
	int nType;
	int i;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(o != NULL);

	// Acces a l'objet a serialiser
	metaData = cast(KWMetaData*, o);

	// Nettoyage prealable
	metaData->RemoveAllKeys();

	// Serialisation des meta-donnees sous forme d'une liste de valeurs typees
	// Attention, le tableau est a NULL s'il est vide
	assert(metaData->oaKeyValuePairs == NULL);
	nSize = serializer->GetInt();
	if (nSize > 0)
	{
		metaData->oaKeyValuePairs = new ObjectArray;
		metaData->oaKeyValuePairs->SetSize(nSize);
		for (i = 0; i < nSize; i++)
		{
			keyValuePair = new KWKeyValuePair;
			metaData->oaKeyValuePairs->SetAt(i, keyValuePair);
			keyValuePair->usKey.SetValue(serializer->GetString());
			nType = serializer->GetInt();
			if (nType == KWKeyValuePair::StringType)
				keyValuePair->SetStringValue(serializer->GetString());
			else if (nType == KWKeyValuePair::DoubleType)
				keyValuePair->SetDoubleValue(serializer->GetDouble());
			assert(keyValuePair->nType == nType);
		}
	}
}

Object* PLShared_MetaData::Create() const
{
	return new KWMetaData;
}
