// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDiscretizerSpec;
class PLShared_DiscretizerSpec;

#include "Object.h"
#include "KWVersion.h"
#include "KWDiscretizer.h"
#include "PLSharedObject.h"

////////////////////////////////////////////////////////////
// Classe KWDiscretizerSpec
//    Parametrage de l'algorithme de discretisation utilise
//    Par defaut: MODL en classification supervise, EqualWidth en non supervise
//                None signifie aucun (accepte en non supervise)
class KWDiscretizerSpec : public Object
{
public:
	// Constructeur
	KWDiscretizerSpec();
	~KWDiscretizerSpec();

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

	// Effectif minimum par intervalle (defaut: 0)
	// Ce parametre est determine automatiquement par l'algorithme s'il vaut 0
	int GetMinIntervalFrequency() const;
	void SetMinIntervalFrequency(int nValue);

	// Nombre maximum d'intervalles (defaut: 0)
	// Ce parametre est determine automatiquement par l'algorithme s'il vaut 0
	int GetMaxIntervalNumber() const;
	void SetMaxIntervalNumber(int nValue);

	////////////////////////////////////////////////////////
	// Divers

	// Verification des parametres, dans tous les cas ou pour un type de cible
	boolean Check() const override;
	boolean CheckForTargetType(int nTargetAttributeType) const;

	// Recopie des specifications (sauf TargetAttributeType, qui est garde)
	void CopyFrom(const KWDiscretizerSpec* kwdsSource);

	// Duplication
	KWDiscretizerSpec* Clone() const;

	// Creation d'un discretizer conforme aux specifications
	// selon le type d'attribut cible courant
	// Renvoie NULL si specifications non valides
	// Memoire: l'objet rendu est gere par l'appele (creation bufferisee)
	const KWDiscretizer* GetDiscretizer(int nTargetAttributeType) const;

	// Fraicheur de l'objet, incrementee a chaque modification
	int GetFreshness() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

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
	int nMinIntervalFrequency;
	int nMaxIntervalNumber;
	int nFreshness;
	mutable KWDiscretizer* supervisedDiscretizer;
	mutable KWDiscretizer* unsupervisedDiscretizer;
	mutable int nSupervisedDiscretizerFreshness;
	mutable int nUnsupervisedDiscretizerFreshness;
};

////////////////////////////////////////////////////////////
// Classe PLShared_DiscretizerSpec
//	 Serialisation de la classe KWDiscretizerSpec
class PLShared_DiscretizerSpec : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DiscretizerSpec();
	~PLShared_DiscretizerSpec();

	// Acces aux spec
	void SetDiscretizerSpec(KWDiscretizerSpec* discretizerSpec);
	KWDiscretizerSpec* GetDiscretizerSpec();

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};
