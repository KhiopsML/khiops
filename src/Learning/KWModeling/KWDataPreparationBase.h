// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataPreparationBase;
class KWDataPreparationChunk;
class KWDataPreparationColumn;

#include "KWLearningSpec.h"
#include "KWClass.h"
#include "KWClassStats.h"
#include "KWDataPreparationClass.h"
#include "KWSortableIndex.h"
#include "KWDRDataGrid.h"
#include "KWProbabilityTable.h"

//////////////////////////////////////////////////////////////////////////////
// Classe de gestion des donnees issue de la preparation
// Le gestionnaire de donnees est initialise par des specifications de
// preparation. Il permet alors un acces direct aux recodages par attribut
// et aux scores bayesiens par valeur cible.
//
// Le gestionnaire cherche a optimiser ces acces en tenant compte des
// limite des ressources memoire et en utilisant les ressources disque par chunk.
// Ces optimisations sont transparentes pour les services clients du gestionnaire
class KWDataPreparationBase : public KWLearningService
{
public:
	// Constructeur
	KWDataPreparationBase();
	~KWDataPreparationBase();

	// Parametrage par des specifications d'apprentissage
	// Redefinition de la methode ancetre pour parametrer
	// en meme temps la DataPreparationClass
	void SetLearningSpec(KWLearningSpec* specification) override;

	// Parametrage des donnees de preparation a prendre en compte
	// Les objets appartiennent a l'appele, mais sont accessible depuis l'appelant
	// afin de permettre leur parametrage.
	KWDataPreparationClass* GetDataPreparationClass();

	// Parametrage des attributs de preparations a utiliser.
	// On initialise cette liste par recopie, tant que la classe n'est pas preparee.
	// En consultation, il n'est pas permise de modifier la liste, qui doit rester
	// compatible avec les donnees preparee.
	// La liste des attributs a utiliser reference des attributs de la classe
	// de preparation. Elle est initialement vide.
	// La coherence d'ensemble est assuree par la methode Check()
	void SetDataPreparationUsedAttributes(ObjectArray* oaDataPreparationAttributes);
	const ObjectArray* GetDataPreparationUsedAttributes() const;

	// Calcul de donnees preparees
	// (peut echouer si probleme d'entrees-sorties ou de memoire, ce qui impacte l'indicateur
	// IsPreparedDataComputed)
	void ComputePreparedData();

	// Indicateur de preparation
	boolean IsPreparedDataComputed();

	// Nettoyage des donnees preparees
	void CleanPreparedData();

	///////////////////////////////////////////////////////////////////
	// Acces aux donnees preparees

	// Permutation aleatoire des attributs de preparations utilises, compatible
	// avec l'optimisation des acces par chunk
	// Cela permet un parcours des attributs dans un ordre aleatoire, sans
	// degradation des performances
	void ShuffleDataPreparationUsedAttributes();

	// Restauration de l'ordre initial des attribut de preparation utilises
	void RestoreDataPreparationUsedAttributes();

	// Acces aux index des valeurs cible pour chaque instance
	// Dans le cas d'un attribut cible Symbol, l'index d'une valeur est celui de cette valeur
	// parmi les valeurs cibles
	// Dans le cas d'un attribut cible continu, l'index d'une valeur est celui de la valeur
	// dans la base d'apprentissage (index moyen en cas d'egalite)
	// Memoire: le vecteur retour appartient a l'appele
	IntVector* GetTargetIndexes();

	// Acces aux recodages (index dans une table de contingence) pour un attribut de preparation
	//   Entree: attribut de preparation utilise (objet KWDataPreparationAttribute)
	//   Sortie: vecteur contenant pour chaque instance de la base de donnees son index de recodage
	//   Retour: false si erreur d'entree-sortie
	// Memoire: l'objet en entree doit faire partie des specifications de preparation, et l'objet
	// en sortie doit etre alloue par l'appelant
	boolean FillRecodingIndexesAt(KWDataPreparationAttribute* dataPreparationAttribute,
				      IntVector* ivObjectRecodings);

	// Acces aux probabilites conditionnelles des valeurs cibles pour un attribut de preparation
	//   Entree: attribut de preparation utilise (objet KWDataPreparationAttribute) et index de valeur cible
	//   Sortie: vecteur contenant pour chaque instance sa probabilite conditionnelle univariee (en log)
	//   Retour: false si erreur d'entree-sortie
	// Memoire: l'objet en entree doit faire partie des specifications de preparation, et l'objet
	// en sortie doit etre alloue par l'appelant
	boolean FillTargetConditionalLnProbsAt(KWDataPreparationAttribute* dataPreparationAttribute, int nTargetIndex,
					       ContinuousVector* cvUnivariateTargetConditionalProbs);

	// CH CRE
	// Acces a la proba conditionnelle pour un attribut de preparation et une instance
	//   Entree: attribut de preparation utilise (objet KWDataPreparationAttribute), index de valeur cible, index de
	//   l'instance Sortie: probabilite conditionnelle univariee (en log) Retour: false si erreur d'entree-sortie
	// Memoire: l'objet en entree doit faire partie des specifications de preparation, et l'objet
	// en sortie doit etre alloue par l'appelant
	Continuous FillTargetConditionalLnProbAt(KWDataPreparationAttribute* dataPreparationAttribute, int nTargetIndex,
						 int nRecodingIndex);
	// Fin CH CRE

	// Modification de probabilites conditionnelles multivariees par ajout ou supression
	// des probabilites conditionnelles des valeurs cibles pour un attribut de preparation
	//   Entree: attribut de preparation utilise (objet KWDataPreparationAttribute)
	//           index de valeur cible
	//           poids pour la prise en compte du nouvel attribut
	//   Entree et sortie: vecteur contenant pour chaque instance sa probabilite conditionnelle multivariee (en log)
	//                     (le meme vecteur peut etre utilise en entree et sortie)
	//   Retour: false si erreur d'entree-sortie
	// Memoire: l'objet en entree doit faire partie des specifications de preparation, et l'objet
	// en sortie doit etre alloue par l'appelant
	boolean UpgradeTargetConditionalLnProbsAt(KWDataPreparationAttribute* dataPreparationAttribute,
						  int nTargetIndex, Continuous cWeight,
						  ContinuousVector* cvInputMultivariateTargetConditionalProbs,
						  ContinuousVector* cvOutputMultivariateTargetConditionalProbs);

	// Indicateur d'erreur d'alimentation
	// Provient d'une eventuelle erreur d'entree-sortie dans l'un quelconque des acces par
	// une des deux methodes Fill... precedentes
	// Permet de ne tester les erreurs que de temps en temps
	// N'est reinitialise que par une nouvelle preparation des donnees
	boolean IsFillError();

	// Ecriture des donnees preparees indexees dans un fichier
	// Fichier avec ligne d'entete, separateur tabulation et une ligne par enregistrement,
	// une colonne d'index pour chaque attribut recode et pour l'attribut cible
	void WriteIndexedPreparedDataFile(const ALString& sFileName);

	// Ecriture des donnees preparees scorees dans un fichier
	// Fichier avec ligne d'entete, separateur tabulation et une ligne par enregistrement
	// une colonne de score (LnProb) par attribut recode et valeur cible, une colonne d'index
	// pour l'attribut cible
	void WriteScoredPreparedDataFile(const ALString& sFileName);

	///////////////////////////////////////////////////////////////////////
	// Services divers

	// Test de validite des donnees preparees
	boolean CheckPreparedData() const;

	// Test de validite des specifications de preparation
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles standard
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// La base contient ses colonnes (referencees par attribut prepare dans un dictionnaire
	// (nkdPreparedDataColumns) et ses chunks (dans oaDataPreparationChunks), qui partitionnent la base en colonnes.
	// Chaque colonne d'un chunk charge en memoire contient un vecteur des index de recodage
	// par instance. L'ensemble de ces colonnes est memorise par les chunks charges (dans les colonnes)
	// le restant disponible etant accessible dans le tableau des vecteurs de recodage libres
	// (oaFreeRecodingIndexVectors).
	//
	// Lors de l'etape d'initialisation, la base de donnees initiale (non preparee) est lue une fois,
	// et les fichiers chunks sont ecrits simultanement. Chaque fichier chunk contient les index des
	// valeurs recodees pour une partie des attribut.
	// En phase d'utilisation, lors de l'acces a un attribut, l'acces se fait en memoire si le chunk est
	// charge en memoire (et donc le vecteur des index de valeurs recodees de l'attribut est disponible).
	// Si le chunk n'est pas charge en memoire, le plus vieux chunk accede est supprime de la memoire
	// (liberant ainsi des vecteurs de de recodage) et le chunk demande est charge en memoire.
	// Si un seul chunk est necessaire, tout se fait en memoire des le chragement initial, sans
	// utilisation de fichiers chunks.

	//////////////////////////////////////////////////////////
	// Calcul des donnees preparees

	// Chargement d'une colonne en memoire
	// Procede au chargement eventuel du chunk de la colonne, en dechargeant prealablement un
	// chunk non utilise
	// Renvoie false en cas d'erreur d'entre-sortie
	boolean LoadDataPreparationColumn(KWDataPreparationColumn* dataPreparationColumn);

	// Chargement des donnees preparees
	// La base en parametre doit etre ouverte en lecture par l'appelant
	// Renvoie false en cas d'erreur d'entre-sortie
	boolean LoadPreparedData(KWDatabase* loadDatabase);

	// Recherche d'un nom de fichier libre pour un chunk donne
	const ALString BuildChunkFileName(int nChunk);

	// Initialisation de la classe de preparation des donnees pour le calcul des donnees
	// Ajout d'un attribut d'indexation des valeurs de l'attribut cible
	// Creation d'un domaine de classe de travail dans lequel la classe est directement utilisable
	// Calcul des index des colonnes des donnees preparees
	void SetupDataPreparationClass();

	// Acces a l'attribut d'index des valeurs cibles, et a son index de colonne
	KWAttribute* GetTargetIndexAttribute();
	int GetTargetColumnIndex();

	// Nettoyage de la classe de preparation des donnees (que l'on remet dans son etat initial)
	void CleanDataPreparationClass();

	// Calcul des parametres de gestion des chunk
	// (en fonction de la memoire disponible et du nombre d'instance et de variables)
	// input
	//	    MaxMemoryColumnNumber: nombre maximum de colonnes utilisable en memoire (0 si a estimer)
	// output
	//		ChunkNumber: nombre total chunk
	//		MemoryChunkNumber: nombre de chunks presents simultanement en memoire
	//		ChunkColumnNumber: nombre de colonne par chunk
	void ComputeChunkParameters(int nMaxMemoryColumnNumber, int& nChunkNumber, int& nMemoryChunkNumber,
				    int& nChunkColumnNumber);

	// Calcul du nombre maximum de colonnes utilisable en memoire, par une estimation "theorique"
	int ComputeMaxMemoryColumnNumber();

	/////////////////////////////////////////////////////////
	// Donnees de travail

	// Dictionnaire de specification des colonnes (donnees par attribut) de la base de recodage,
	// indexe par les attributs de preparation
	// Memoire: les colonnes appartiennent directement a la base
	NumericKeyDictionary nkdPreparedDataColumns;

	// Tableau des chunks de colonnes (chaque chunk reference une sous-partie des colonnes)
	// Memoire: les chunks appartiennent directement a la base
	ObjectArray oaDataPreparationChunks;

	// Tableau des chunks charges en memoire
	ObjectArray oaMemoryDataPreparationChunks;

	// Tableau des vecteurs d'index de recodage libres
	// (les autres sont references dans les colonnes des chunks charges en memoire)
	ObjectArray oaFreeRecodingIndexVectors;

	// Taille des donnees preparees
	int nColumnNumber;
	int nLineNumber;

	// Vecteur des index des valeur cible par enregistrement
	IntVector ivTargetIndexes;

	// Attribut d'index sur les valeurs cibles, et son index de colonne
	KWAttribute* targetIndexAttribute;
	int nTargetColumnIndex;

	// Specification de preparation
	KWDataPreparationClass dataPreparationClass;
	ObjectArray oaDataPreparationUsedAttributes;

	// Erreur d'entres-sortie lors d'une methode Fill...
	boolean bIsFillError;

	// Flag de trace de la gestion des chunk
	// Niveau 0: pas de trace
	// Niveau 1: calcul de la memoire necessaire au chunks
	// Niveau 2: chargements/dechargement des chunks
	// Niveau 3: tous les acces aux colonnes
	int nChunkTraceLevel;
};

////////////////////////////////////////////////////////////////////////
// Gestion des donnes preparees pour un groupe d'attributs
// Classe privee, destinee uniquement a KWDataPreparationBase
class KWDataPreparationChunk : public Object
{
public:
	// Constructeur
	KWDataPreparationChunk();
	~KWDataPreparationChunk();

	// Acces au tableau des colonnes du chunk (KWDataPreparationColumn)
	// Memoire: les colonnes sont uniquement referencees par le chunk
	ObjectArray* GetColumns();

	// Flag indiquant si le chunk est charge en memoire
	boolean IsLoaded() const;

	///////////////////////////////////////////////////////////////////
	// Gestion du chunk

	// Chargement en memoire du chunk
	// La methode affecte a chaque colonne un vecteur de recodage (en puissant dans le tableau
	// des vecteurs libres, tailles correctement), puis lit le fichier de chunk pour alimenter
	// ces vecteurs. En sortie, le tableau des vecteur libre est actualise.
	// Le code retour est a false en case d'erreur d'entree sorties
	boolean LoadChunk(ObjectArray* oaFreeRecodingIndexVectors);

	// Dechargement du chunk
	// La methode restitue au tableau des vecteur libre les vecteurs de recodage
	// affecte a chaque colonne
	void UnloadChunk(ObjectArray* oaFreeRecodingIndexVectors);

	// Mise a jour d'un index de recodage pour une ligne et une colonne donnee
	void SetRecodingIndexAt(int nLine, int nColumn, int nIndex);

	// Installation de la memoire dans les colonnes
	// (en puissant dans le tableau des vecteurs libres, tailles correctement)
	void SetChunkMemory(ObjectArray* oaFreeRecodingIndexVectors);

	// Liberation de la memoire des colonnes
	// (en la restituant au tableau des vecteurs libres, tailles correctement)
	void UnsetChunkMemory(ObjectArray* oaFreeRecodingIndexVectors);

	// Acces a la fraicheur du dernier chargement
	// Permet de gerer l'importance des chunks pour prioriser leurs chargements/dechargement
	double GetLoadFreshness() const;

	///////////////////////////////////////////////////////////////////
	// Gestion du fichier associe a un chunk

	// Nom du fichier gerant le chunk (vide su un seul chunk, en memoire)
	void SetChunkFileName(const ALString& sFileName);
	const ALString& GetChunkFileName() const;

	// Ouverture du fichier en lecture ou ecriture: retourne true si OK
	boolean OpenInputChunkFile();
	boolean OpenOutputChunkFile();

	// Fermeture du fichier
	// Renvoie false si erreur grave avant ou pendant la fermeture, true sinon
	boolean CloseChunkFile();

	// Destruction du fichier
	void RemoveChunkFile();

	// Test si fin de fichier en lecture
	boolean IsChunkFileEnd();

	// Ecriture d'un index de recodage pour une ligne et une colonne donnee
	// (cette methode doivent etre appellees sequentiellement)
	void WriteChunkFileRecodingIndexAt(int nLine, int nColumn, int nIndex);

	// Lecture/ecriture d'un entier du fichier
	int ReadChunkFileInt();
	void WriteChunkFileInt(const int nValue);

	// Lecture/ecriture du buffer du fichier
	void ReadChunkFileBuffer();
	void WriteChunkFileBuffer();

	// Test si erreur grave
	// Positionne suite a une erreur grave de lecture ou ecriture, reinitialise uniquement a la fermeture du fichier
	boolean IsError() const;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////
	//// Implementation
	static const int nFileBufferMaxSize = MemSegmentByteSize / sizeof(int);

protected:
	ObjectArray oaColumns;
	boolean bIsLoaded;
	ALString sChunkFileName;
	int* nFileBuffer;
	FILE* fChunkFile;
	boolean bIsOpenedForWrite;
	boolean bIsError;
	longint lFilePosition;
	int nFileBufferOffset;
	int nFileBufferSize;
	Timer timerRead;
	Timer timerWrite;
	double dLoadFreshness;
	static double dFreshness;
};

////////////////////////////////////////////////////////////////////////
// Gestion des donnes preparees pour un attribut
// Classe privee, destinee uniquement a KWDataPreparationBase
class KWDataPreparationColumn : public Object
{
public:
	// Constructeur
	KWDataPreparationColumn();
	~KWDataPreparationColumn();

	// Specification de l'attribut de preparation correspondant a la colonne
	// Contrainte: l'objet en parametre ne doit pas etre modifie lors de toute
	// son utilisation
	// Memoire: attribut a gerer depuis l'appelant
	void SetDataPreparationAttribute(KWDataPreparationAttribute* kwdpaDataPreparationAttribute);
	KWDataPreparationAttribute* GetDataPreparationAttribute();

	// Specification de l'attribut d'indexation
	// Memoire: attribut a gerer depuis l'appelant
	void SetIndexingAttribute(KWAttribute* attribute);
	KWAttribute* GetIndexingAttribute();

	// Chunk gerant la colonne
	void SetChunk(KWDataPreparationChunk* kwdpcValue);
	KWDataPreparationChunk* GetChunk();

	// Acces aux index des valeurs recodees pour chaque instance
	// Renvoie NULL si le chunk n'est pas charge en memoire
	// Memoire: le vecteur d'index appartient a l'appelant
	void SetRecodingIndexes(IntVector* ivIndexes);
	IntVector* GetRecodingIndexes();

	// Index de la colonne dans le chunk pour l'acces aux donnees
	void SetColumnIndex(int nValue);
	int GetColumnIndex();

	///////////////////////////////////////////////////////////////////////
	// Services divers

	// Acces a la probabilite conditionnele (en log) d'une valeur recodee pour une valeur cible donnee
	Continuous GetLnSourceConditionalProb(int nSourceModalityIndex, int nTargetModalityIndex);

	// Test de validite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Memorisation des probabilites conditionnelles des valeurs recodees
	KWProbabilityTable lnSourceConditionalProbabilityTable;

	// Specification de la colonne
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWAttribute* indexingAttribute;
	int nColumnIndex;
	IntVector* ivRecodingIndexes;
	KWDataPreparationChunk* chunk;
};

//////////////////////////////////////////////////////////////////
// Methodes en inline

inline KWDataPreparationClass* KWDataPreparationBase::GetDataPreparationClass()
{
	return &dataPreparationClass;
}

inline const ObjectArray* KWDataPreparationBase::GetDataPreparationUsedAttributes() const
{
	return &oaDataPreparationUsedAttributes;
}

inline IntVector* KWDataPreparationBase::GetTargetIndexes()
{
	require(IsPreparedDataComputed());

	return &ivTargetIndexes;
}

inline boolean KWDataPreparationBase::IsFillError()
{
	return bIsFillError;
}

inline KWAttribute* KWDataPreparationBase::GetTargetIndexAttribute()
{
	require(targetIndexAttribute != NULL);
	return targetIndexAttribute;
}

inline int KWDataPreparationBase::GetTargetColumnIndex()
{
	require(targetIndexAttribute != NULL);
	return nTargetColumnIndex;
}

inline ObjectArray* KWDataPreparationChunk::GetColumns()
{
	return &oaColumns;
}

inline boolean KWDataPreparationChunk::IsLoaded() const
{
	return bIsLoaded;
}

inline double KWDataPreparationChunk::GetLoadFreshness() const
{
	return dLoadFreshness;
}

inline boolean KWDataPreparationChunk::IsChunkFileEnd()
{
	return nFileBufferSize == 0;
}

inline int KWDataPreparationChunk::ReadChunkFileInt()
{
	int nResult;

	require(fChunkFile != NULL);
	require(nFileBufferOffset < nFileBufferSize);

	// Lecture d'un caractere du buffer
	nResult = nFileBuffer[nFileBufferOffset];
	nFileBufferOffset++;
	lFilePosition++;

	// Relecture du buffer si necessaire
	if (nFileBufferOffset == nFileBufferSize)
		ReadChunkFileBuffer();
	return nResult;
}

inline void KWDataPreparationChunk::WriteChunkFileInt(const int nValue)
{
	require(fChunkFile != NULL);
	require(nFileBufferSize < nFileBufferMaxSize);

	// Ecriture d'un caractere du buffer
	nFileBuffer[nFileBufferSize] = nValue;
	nFileBufferSize++;
	lFilePosition++;

	// Ecriture du buffer si necessaire
	if (nFileBufferSize == nFileBufferMaxSize)
		WriteChunkFileBuffer();
}

inline void KWDataPreparationChunk::SetRecodingIndexAt(int nLine, int nColumn, int nIndex)
{
	require(IsLoaded());
	require(0 <= nColumn and nColumn < GetColumns()->GetSize());
	require(GetColumns()->GetAt(nColumn) != NULL);
	require(cast(KWDataPreparationColumn*, GetColumns()->GetAt(nColumn))->GetRecodingIndexes() != NULL);
	require(0 <= nLine and
		nLine < cast(KWDataPreparationColumn*, GetColumns()->GetAt(nColumn))->GetRecodingIndexes()->GetSize());

	// Mise a jour de la case du vecteur
	cast(KWDataPreparationColumn*, GetColumns()->GetAt(nColumn))->GetRecodingIndexes()->SetAt(nLine, nIndex);
}

inline void KWDataPreparationChunk::WriteChunkFileRecodingIndexAt(int nLine, int nColumn, int nIndex)
{
	require(0 <= nLine);
	require(0 <= nColumn and nColumn < oaColumns.GetSize());
	require(nLine * (longint)oaColumns.GetSize() + nColumn == lFilePosition);

	WriteChunkFileInt(nIndex);
}

inline boolean KWDataPreparationChunk::IsError() const
{
	return bIsError;
}

inline KWDataPreparationAttribute* KWDataPreparationColumn::GetDataPreparationAttribute()
{
	require(Check());
	return dataPreparationAttribute;
}

inline void KWDataPreparationColumn::SetIndexingAttribute(KWAttribute* attribute)
{
	require(Check());
	indexingAttribute = attribute;
}

inline KWAttribute* KWDataPreparationColumn::GetIndexingAttribute()
{
	require(Check());
	return indexingAttribute;
}

inline void KWDataPreparationColumn::SetColumnIndex(int nValue)
{
	require(nValue >= 0);
	nColumnIndex = nValue;
}

inline int KWDataPreparationColumn::GetColumnIndex()
{
	require(Check());
	return nColumnIndex;
}

inline void KWDataPreparationColumn::SetRecodingIndexes(IntVector* ivIndexes)
{
	ivRecodingIndexes = ivIndexes;
}

inline IntVector* KWDataPreparationColumn::GetRecodingIndexes()
{
	return ivRecodingIndexes;
}

inline void KWDataPreparationColumn::SetChunk(KWDataPreparationChunk* kwdpcValue)
{
	chunk = kwdpcValue;
}

inline KWDataPreparationChunk* KWDataPreparationColumn::GetChunk()
{
	require(Check());
	return chunk;
}

inline Continuous KWDataPreparationColumn::GetLnSourceConditionalProb(int nSourceModalityIndex,
								      int nTargetModalityIndex)
{
	require(Check());
	require(lnSourceConditionalProbabilityTable.GetSourceSize() ==
		dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats()->ComputeSourceGridSize());
	require(lnSourceConditionalProbabilityTable.GetTargetSize() ==
		dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats()->ComputeTargetGridSize());
	return lnSourceConditionalProbabilityTable.GetSourceConditionalLogProbAt(nSourceModalityIndex,
										 nTargetModalityIndex);
}
