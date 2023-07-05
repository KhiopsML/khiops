// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWKeyExtractor;
class KWKeyFieldsIndexer;

#include "Object.h"
#include "InputBufferedFile.h"
#include "Vector.h"
#include "KWKey.h"
#include "PLParallelTask.h"

////////////////////////////////////////////////////////////
// Classe KWKeyExtractor
//	 Extraction des clefs d'un fichier tabule
class KWKeyExtractor : public Object
{
public:
	// Constructeur
	KWKeyExtractor();
	~KWKeyExtractor();

	// Referencement du buffer de lecture, celui-ci doit etre ouvert en lecture
	// Il ne sera ni ferme, ni detruit
	void SetBufferedFile(InputBufferedFile* bufferedFile);
	InputBufferedFile* GetBufferedFile() const;

	// Index des clefs: le setter recopie les index et les recode pour optimiser le parsing des cles
	void SetKeyFieldIndexes(const IntVector* ivIndexes);
	const IntVector* GetConstKeyFieldIndexes() const;

	// Nettoyage de l'extracteur de cle
	void Clean();

	// Construit la prochaine clef a partir de la position courante
	// positionne le curseur en debut de ligne suivante
	// Emet eventuellement des warning, et renvoie false en cas d'erreur ou de ligne trop longue
	// Les messages de warning sont emis par la classe PLParallelTask passee en parametre (pas de message si NULL)
	// Si c'est le cas, La methode PLParallelTask::SetLocalLineNumber doit etre appelee
	boolean ParseNextKey(KWKey* key, PLParallelTask* taskErrorSender);

	// Donne les adresses de debut (inclus) et de fin (exclue) de la ligne (a appeler apres avoir parse la cle)
	void ExtractLine(int& nBeginPos, int& nEndPos) const;

	// Affichage d'une plage de caracteres
	void WriteLine(int nBeginPos, int nEndPos, ostream& ost) const;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Fichier bufferise
	InputBufferedFile* iBuffer;

	// Position du debut de la ligne precedente
	int nLastStartPos;

	// Index des champs de la cle
	IntVector ivKeyFieldIndexes;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Les deux tableaux suivants contiennent des pointeurs qui pointent vers les memes objets
	// dans un sens different :
	//   -le premier contient les champs dans l'ordre des clef (clef primaire, secondaire...),
	//    c'est l'ordre de l'objet Key
	//	 -le second contient les champs dans l'ordre du dictionnaire (necessaire pour l'ecriture des clef dans
	// un fichier de sortie)
	// ces deux structures sont maintenues pour ne pas avoir a trier un tableau pour chaque clef

	// Champs maintenus suivant l'ordre des clefs
	ObjectArray oaKeyFieldsOrderedByKey;

	// Champs maintenus suivant l'ordre du fichier
	ObjectArray oaKeyFieldsOrderedByFile;
};

////////////////////////////////////////////////////////////
// Classe KWKeyFieldsIndexer
//		Permet d'obtenir les index des champs correspondants aux clefs d'un fichier
//		a partir des champs de la clef et du header du fichier
class KWKeyFieldsIndexer : public Object
{
public:
	// Constructeur
	KWKeyFieldsIndexer();
	~KWKeyFieldsIndexer();

	// Parametrage des noms des attributs de la cle
	StringVector* GetKeyAttributeNames();

	// Parametrage des noms de tous les champs natifs
	StringVector* GetNativeFieldNames();

	// Calcul des index de la cle a partir des noms des attributs de la cle et des attribut natif,
	// en utilisant (ou non) les champs de la premiere ligne de la base.
	// Si la ligne d'entete est utilisee, les index de la cle sont calcules par rapport au champs de l'entete
	// Sinon, le nombre de champs de l'entete doit etre le meme que le nombre d'attributs natifs,
	// et les index sont calcules par rapport au nom des attributs natifs
	// En sortie, on a la l'index de chaque champs de la cle dans le fichier
	// Message d'erreur si necessaire
	boolean ComputeKeyFieldIndexes(boolean bHeaderLineUsed, const StringVector* svHeaderLineFieldNames);

	// Acces aux index des champs de la cle, calcules par la methode precedente
	const IntVector* GetConstKeyFieldIndexes() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	StringVector svKeyAttributeNames;
	StringVector svNativeFieldNames;
	IntVector ivKeyFieldIndexes;
};

////////////////////////////////////////////////////////////
// Classe KWFieldIndex
//	 contient le nom du champ et son index
class KWFieldIndex : public Object
{
public:
	// Constructeur
	KWFieldIndex();
	~KWFieldIndex();

	// Valeur du champ
	void SetField(const ALString& sValue);
	const ALString& GetField() const;

	// Index du champ
	void SetIndex(int);
	int GetIndex() const;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	int nIndex;
	ALString sField;
};

// Fonction de comparaison de deux fieldIndex : tri sur l'index uniquement
int KWFieldIndexCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////
// Implementation en inline

inline void KWFieldIndex::SetField(const ALString& sValue)
{
	sField = sValue;
}

inline const ALString& KWFieldIndex::GetField() const
{
	return sField;
}

inline void KWFieldIndex::SetIndex(int nValue)
{
	nIndex = nValue;
}

inline int KWFieldIndex::GetIndex() const
{
	assert(nIndex != -1);
	return nIndex;
}