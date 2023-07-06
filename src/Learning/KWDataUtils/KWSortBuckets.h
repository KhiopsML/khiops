// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWSortBuckets;
class KWSortBucket;
class PLShared_SortBucket;

#include "Object.h"
#include "OutputBufferedFile.h"
#include "KWKeyExtractor.h"
#include "PLSharedObject.h"
#include "KWArtificialDataset.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWSortBuckets
// Conteneur de chunks (classe KWSortBucket), avec services de gestion
// (les buckets sont references uniquement, comme dans un ObjectArray)
// Permet
//	- de construire les chunks a partir des bornes issues d'un sampling (splits)
//  - de remplacer un chunk par deux sous-chunks (resultats du split de ce premier chunk)
//	- de verifier l'appartenance d'une clef a un chunk
class KWSortBuckets : public Object
{
public:
	// Constructeur
	KWSortBuckets();
	~KWSortBuckets();

	// Nombre de buckets
	int GetBucketNumber() const;

	// Acces a un bucket
	KWSortBucket* GetBucketAt(int nIndex) const;

	//////////////////////////////////////////////////////////////////////////////
	// Alimentation du containeur de buckets

	// Construit les chunks a partir d'un bucket principal et d'un ensemble de cle (KWKey)
	// de coupures (splits) internes a ce bucket principal
	// Les parametres en entree appartienennt a l'appelant et sont inchanges lors de l'appel
	void Build(KWSortBucket* mainBucket, const ObjectArray* oaSplits);

	// Initialisation a partir d'un tableau de buckets (KWSortBucket)
	// Memoire: les buckets sont recopies
	void Initialize(const ObjectArray* oaSourceBuckets);

	// Ajout d'un bucket (n'est pas recopie);
	// Memoire: le bucket source appartient alors a l'appele
	void AddBucket(KWSortBucket* sourceBucket);

	// Remplace le largeBucket par ses sous-buckets (resultat du split de largeBucket)
	// Memoire: le large bucket est detruit, mais pas le containeur, dont les sous-buckets sont transferes a
	// l'appele
	void SplitLargeBucket(KWSortBucket* largeBucket, KWSortBuckets* subBuckets);

	// Supression ou destruction des buckets du container
	void RemoveAll();
	void DeleteAll();

	//////////////////////////////////////////////////////////////////////////////
	// Services avances de gestion du containeur de buckets

	// Indexation des buckets
	void IndexBuckets();

	// Destruction des fichiers des buckets
	void DeleteBucketFiles();

	// Ajoute une ligne au bucket qui contient la clef key
	// La taille des buffers est mise a jour
	// La recherche du bucket est dichotomique
	void AddLineAtKey(KWKey* key, CharVector* cvLine);

	// Renvoie le premier chunk qui est plus gros que la taille specifiee
	KWSortBucket* GetOverweightBucket(int nChunkSize) const;

	///////////////////////////////////////////////////////////////////////////
	// Services divers

	// Recopie du contenu
	void CopyFrom(const KWSortBuckets* bSource);

	// Verification de coherence de l'ensemble des buckets (bornes des buckets)
	boolean Check() const override;

	// Verification des noms de fichier des buckets
	boolean CheckFileNames() const;

	// Affiche la liste des chunks
	void Write(ostream& ost) const override;

	// Affiche la liste des chunks avec les premieres lignes de leur contenu
	void WriteWithFirstLines(ostream& ost, int nLineNumber) const;

	// Acces au buckets
	const ObjectArray* GetBuckets() const;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;

	// Methode de test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Retourne le chunk dans lequel ecrire l'enregistrement qui a la clef passee en parametre
	KWSortBucket* SearchBucket(const KWKey* key) const;

	// Tableau des bucket du conteneur
	ObjectArray oaBuckets;

	// Tableau des cles distincts des buckets, hors extremites (reference sur les cles des buckets)
	ObjectArray oaDistinctKeys;

	// Index du premier bucket du container correspondant a chaque cle distincte
	IntVector ivBucketIndexes;
	boolean bIsIndexComputed;
};

/////////////////////////////////////////////////////////////////////////////////
// Classe KWSortBucket
// Un KWSortBucket represente un chunk et les bornes d'inclusion de ses clefs.
// Un KWSortBucket est determine par une borne inferieure et une borne superieure
// Si la borne inferieure est vide c'est -inf ; si la borne superieure est vide c'est +inf
// Par defaut la borne inferieure est inclue et la borne superieure est exclue
// i.e. une clef appartient au bucket ssi borne LowerBound <= key < UpperBound
// 2 types de fichiers sont associes a un KWSortBucket :
//			-le fichier en sortie
//			-la liste des fichier en entree, a trier collectivement
//           (on evite ainsi de concatener ces fichiers en entree prealablement a leur tri)
class KWSortBucket : public Object
{
public:
	// Constructeur (par defaut: bucket ]-inf; +inf[)
	KWSortBucket();
	~KWSortBucket();

	/////////////////////////////////////////////////////////////////////////
	// Definition du chunk

	// Borne inferieure (si la cle est vide (taille 0), cela signifie -inf)
	// Memoire: la clef puis appartient a l'appele
	KWKey* GetLowerBound();

	// Borne superieure (si la cle est vide (taille 0), cela signifie -inf)
	// Memoire: la clef puis appartient a l'appele
	KWKey* GetUpperBound();

	// Indique si la borne inferieure est exclue (defaut: true)
	void SetLowerBoundExcluded(boolean bValue);
	boolean GetLowerBoundExcluded() const;

	// Indique si la borne inferieure est exclue (defaut: true)
	void SetUpperBoundExcluded(boolean bValue);
	boolean GetUpperBoundExcluded() const;

	// Indique si le chunk correspond a un singleton (une seule valeur, non infinie)
	boolean IsSingleton() const;

	// Renvoie true si la clef est contenue dans le bucket
	boolean Contains(const KWKey* key) const;

	// Test si le bucket est plus petit ou plus grand que la cle
	boolean IsLessOrEqualThan(const KWKey* key) const;
	boolean IsGreaterOrEqualThan(const KWKey* key) const;

	// Identifiant unique du bucket
	void SetId(const ALString& sId);
	ALString GetId() const;

	///////////////////////////////////////////////////////////////////////////
	// Gestion du contenu du chunk

	// Fichier pour le contenu du chunk
	void SetOutputFileName(const ALString& sValue);
	const ALString& GetOutputFileName() const;

	// Fichiers pour le contenu du chunk
	void SetChunkFileNames(StringVector* svValue);
	void AddChunkFileName(const ALString& sChunkFileName);
	const StringVector* GetChunkFileNames() const;

	// Suppression des noms des chunks
	void RemoveChunkFileNames();

	// Renvoie la taille du chunk
	longint GetChunkSize();

	// Affectation de la taille du chunck
	void SetChunkFileSize(longint lSize);

	// Acces au lignes contenues dans le chunk
	void AddLine(const CharVector* cvline);

	// Acces au buffer
	CharVector* GetChunk();

	// Est-ce que le chunk est trie
	// C'est forcement le cas pour les chunks singletons, sinon, le tri doit positionner le flag
	void SetSorted();
	boolean GetSorted() const;

	///////////////////////////////////////////////////////////////////////////
	// Services divers

	// Duplication
	KWSortBucket* Clone() const;
	void CopyFrom(const KWSortBucket*);

	// Verification d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	CharVector cvLines;
	KWKey kLowerBound;
	KWKey kUpperBound;
	ALString sOutputFileName;
	StringVector svChunkFileNames;
	ALString sId;
	longint lFileSize;
	boolean bLowerBoundExcluded;
	boolean bUpperBoundExcluded;
	boolean bSorted;

	// Classes friend
	friend class KWSortBuckets;
	friend class PLShared_SortBucket;
};

int KWSortBucketCompareChunkSize(const void* first, const void* second);

///////////////////////////////////////////////////
// Classe PLShared_SortBucket
// Serialisation de la classe KWSortBucket
class PLShared_SortBucket : public PLSharedObject
{
public:
	// Constructeur
	PLShared_SortBucket();
	~PLShared_SortBucket();

	// Acces au bucket
	void SetBucket(KWSortBucket* bucket);
	KWSortBucket* GetBucket();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

////////////////////////////////////////////////
// Implementation en inline

inline int KWSortBuckets::GetBucketNumber() const
{
	return oaBuckets.GetSize();
}

inline KWSortBucket* KWSortBuckets::GetBucketAt(int nIndex) const
{
	require(oaBuckets.GetSize() > nIndex);
	return cast(KWSortBucket*, oaBuckets.GetAt(nIndex));
}

inline boolean KWSortBucket::IsSingleton() const
{
	return kLowerBound.GetSize() != 0 and kUpperBound.GetSize() != 0 and kLowerBound.Compare(&kUpperBound) == 0;
}

inline KWKey* KWSortBucket::GetLowerBound()
{
	return &kLowerBound;
}

inline KWKey* KWSortBucket::GetUpperBound()
{
	return &kUpperBound;
}

inline void KWSortBucket::SetLowerBoundExcluded(boolean bValue)
{
	bLowerBoundExcluded = bValue;
}

inline boolean KWSortBucket::GetLowerBoundExcluded() const
{
	return bLowerBoundExcluded;
}

inline void KWSortBucket::SetUpperBoundExcluded(boolean bValue)
{
	bUpperBoundExcluded = bValue;
}

inline boolean KWSortBucket::GetUpperBoundExcluded() const
{
	return bUpperBoundExcluded;
}

inline boolean KWSortBucket::Contains(const KWKey* key) const
{
	return IsLessOrEqualThan(key) and IsGreaterOrEqualThan(key);
}

inline boolean KWSortBucket::IsLessOrEqualThan(const KWKey* key) const
{
	boolean bLowerBoundOk;

	// Test de la borne inferieure
	bLowerBoundOk = (kLowerBound.GetSize() == 0) or (key->Compare(&kLowerBound) > 0) or
			(key->Compare(&kLowerBound) == 0 and not GetLowerBoundExcluded());
	return bLowerBoundOk;
}

inline boolean KWSortBucket::IsGreaterOrEqualThan(const KWKey* key) const
{
	boolean bUpperBoundOk;

	// Test de la borne superieure
	bUpperBoundOk = (kUpperBound.GetSize() == 0) or (key->Compare(&kUpperBound) < 0) or
			(key->Compare(&kUpperBound) == 0 and not GetUpperBoundExcluded());
	return bUpperBoundOk;
}

inline Object* PLShared_SortBucket::Create() const
{
	return new KWSortBucket;
}