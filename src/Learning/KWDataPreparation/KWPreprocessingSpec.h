// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWPreprocessingSpec;
class KWDiscretizerSpec;
class KWGrouperSpec;
class KWDiscretizer;
class KWGrouper;
class KWDataGridOptimizerParameters;
class PLShared_PreprocessingSpec;

#include "Object.h"
#include "KWDiscretizerSpec.h"
#include "KWGrouperSpec.h"
#include "KWDataGridOptimizerParameters.h"
#include "PLSharedObject.h"

//////////////////////////////////////////////////////////////////////////
// Classe KWPreprocessingSpec
// Specification des etapes de preprocessing pour l'apprentissage
//  a savoir discretisation et groupage
// Les parametres des algorithmes sont specifiees de facon generique,
// puis peuvent etre verifiees, et permettre l'instanciation des
// algorithmes correspondants
class KWPreprocessingSpec : public Object
{
public:
	// Constructeur
	KWPreprocessingSpec();
	~KWPreprocessingSpec();

	// Groupage des valeurs cibles (defaut: false)
	boolean GetTargetGrouped() const;
	void SetTargetGrouped(boolean bValue);

	// Parametrage de la discretisation
	// Memoire: l'objet rendu appartient a l'appele
	KWDiscretizerSpec* GetDiscretizerSpec();

	// Parametrage de la discretisation
	// Memoire: l'objet rendu appartient a l'appele
	KWGrouperSpec* GetGrouperSpec();

	// Parametrage de l'optimisation des Data Grid
	// Memoire: l'objet rendu appartient a l'appele
	KWDataGridOptimizerParameters* GetDataGridOptimizerParameters();

	////////////////////////////////////////////////////////
	// Divers

	// Ecriture de rapport lignes sur les specifications du classifier
	void WriteHeaderLineReport(ostream& ost);
	void WriteLineReport(int nTargetAttributeType, ostream& ost);

	// Verification des parametres, dans tous les cas ou pour un type de cible
	boolean Check() const override;
	boolean CheckForTargetType(int nTargetAttributeType) const;

	// Recopie des specifications
	void CopyFrom(const KWPreprocessingSpec* kwpsSource);

	// Duplication
	KWPreprocessingSpec* Clone() const;

	// Fraicheur de l'objet, incrementee a chaque modification
	int GetFreshness() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	boolean bTargetGrouped;
	KWDiscretizerSpec discretizerSpec;
	KWGrouperSpec grouperSpec;
	KWDataGridOptimizerParameters dataGridOptimizerParameters;
};

////////////////////////////////////////////////////////////
// Classe PLShared_PreprocessingSpec
//	 Serialisation de la classe KWPreprocessingSpec
class PLShared_PreprocessingSpec : public PLSharedObject
{
public:
	// Constructeur
	PLShared_PreprocessingSpec();
	~PLShared_PreprocessingSpec();

	// Acces aux specs
	void SetPreprocessingSpec(KWPreprocessingSpec* preprocessingSpec);
	KWPreprocessingSpec* GetPreprocessingSpec();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
