// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWGrouperSpec;
class PLShared_GrouperSpec;

#include "Object.h"
#include "Ermgt.h"
#include "KWVersion.h"
#include "KWGrouper.h"
#include "PLSharedObject.h"

////////////////////////////////////////////////////////////
// Classe KWGrouperSpec
//    Parametrage de l'algorithme de discretisation utilise
//    Par defaut: MODL en classification supervise, BasicGrouping en non supervise
//                None signifie aucun (accepte en non supervise)
class KWGrouperSpec : public Object
{
public:
	// Constructeur
	KWGrouperSpec();
	~KWGrouperSpec();

	// Non de la methode selon le type d'attribut cible:
	//   Symbol en supervise (par defaut), None en non supervise
	//   Rend "MODL" en regression
	const ALString GetMethodName(int nTargetAttributeType) const;

	// Libelle de la methode selon le type d'attribut cible
	// Infos sur le parametrage en plus du nom
	const ALString GetMethodLabel(int nTargetAttributeType) const;

	// Nom de la methode en supervise
	const ALString& GetSupervisedMethodName() const;
	void SetSupervisedMethodName(const ALString& sValue);

	// Nom de la methode en non supervise
	const ALString& GetUnsupervisedMethodName() const;
	void SetUnsupervisedMethodName(const ALString& sValue);

	// Parametre principal
	double GetParam() const;
	void SetParam(double dValue);

	// Effectif minimum par groupe (defaut: 0)
	// Ce parametre est determine automatiquement par l'algorithme
	// s'il vaut 0
	int GetMinGroupFrequency() const;
	void SetMinGroupFrequency(int nValue);

	// Nombre maximum de groupes (defaut: 0)
	// Ce parametre est determine automatiquement par l'algorithme
	// s'il vaut 0
	int GetMaxGroupNumber() const;
	void SetMaxGroupNumber(int nValue);

	////////////////////////////////////////////////////////
	// Divers

	// Verification des parametres, dans tous les cas ou pour un type de cible
	boolean Check() const override;
	boolean CheckForTargetType(int nTargetAttributeType) const;

	// Recopie des specifications
	void CopyFrom(const KWGrouperSpec* kwgsSource);

	// Duplication
	KWGrouperSpec* Clone() const;

	// Creation d'un Grouper conforme aux specifications
	// selon le type d'attribut cible
	// Renvoie NULL si specifications non valides
	// Memoire: l'objet rendu est gere par l'appele (creation bufferisee)
	const KWGrouper* GetGrouper(int nTargetAttributeType) const;

	// Fraicheur de l'objet, incrementee a chaque modification
	int GetFreshness() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	ALString sSupervisedMethodName;
	ALString sUnsupervisedMethodName;
	double dParam;
	int nMinGroupFrequency;
	int nMaxGroupNumber;
	int nFreshness;
	mutable KWGrouper* supervisedGrouper;
	mutable KWGrouper* unsupervisedGrouper;
	mutable int nSupervisedGrouperFreshness;
	mutable int nUnsupervisedGrouperFreshness;
};

////////////////////////////////////////////////////////////
// Classe PLShared_GrouperSpec
//	 Serialisation de la classe KWGrouperSpec
class PLShared_GrouperSpec : public PLSharedObject
{
public:
	// Constructeur
	PLShared_GrouperSpec();
	~PLShared_GrouperSpec();

	// Acces aux spec
	void SetGrouperSpec(KWGrouperSpec* grouperSpec);
	KWGrouperSpec* GetGrouperSpec();

	// Reimplementation des methodes virtuelles
	void DeserializeObject(PLSerializer*, Object*) const override;
	void SerializeObject(PLSerializer*, const Object*) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};