// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation pour la gestion des vecteurs de valeur

// Vecteurs de valeurs
class KWDRSymbolVector;
class KWDRContinuousVector;

// Acces a une valeur d'un vecteur par son index
class KWDRSymbolValueAt;
class KWDRContinuousValueAt;

class KWDRAsSymbolVector;
class KWDRAsContinuousVector;

class KWDRExtractWords;

#include "KWDerivationRule.h"
#include "KWStructureRule.h"

// Enregistrement de ces regles
void KWDRRegisterVectorRules();

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolVector
// Regle de derivation de type Structure(VectorC), memorisant les valeurs
// d'un vecteur de symboles
class KWDRSymbolVector : public KWDRStructureRule
{
public:
	// Constructeur
	KWDRSymbolVector();
	~KWDRSymbolVector();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant les
	// valeurs dans le vecteur prevu a cet effet

	// Nombre de valeurs
	// Le setter fait basculer en interface de structure,
	// et le getter est accessible en interface de structure et de base
	void SetValueNumber(int nValue);
	int GetValueNumber() const;

	// Parametrage des valeurs
	void SetValueAt(int nIndex, const Symbol& sValue);
	Symbol& GetValueAt(int nIndex) const;

	// Acces direct au vecteur de valeurs - methode avancee
	const SymbolVector* GetValues() const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Nettoyage de l'interface de base une fois la regle compilee
	void CleanCompiledBaseInterface() override;

	//////////////////////////////////////////////////////
	// Redefinition des methodes de structure

	// Recopie de la partie structure de la regle
	void CopyStructureFrom(const KWDerivationRule* kwdrSource) override;

	// Transfert de la specification de base de la regle source
	// vers la specification de structure de la regle en cours
	void BuildStructureFromBase(const KWDerivationRule* kwdrSource) override;

	// Affichage, ecriture dans un fichier
	void WriteStructureUsedRule(ostream& ost) const override;

	// Methode de comparaison entre deux regles
	int FullCompare(const KWDerivationRule* rule) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Valeurs
	SymbolVector svValues;
};

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousVector
// Regle de derivation de type Structure(DoubleVector), memorisant les valeurs
// d'un vecteur de continuous
class KWDRContinuousVector : public KWDRStructureRule
{
public:
	// Constructeur
	KWDRContinuousVector();
	~KWDRContinuousVector();

	//////////////////////////////////////////////////////////////
	// La specification de la regle se fait en specifiant les
	// valeurs dans le vecteur prevu a cet effet

	// Nombre de valeurs
	// Le setter fait basculer en interface de structure,
	// et le getter est accessible en interface de structure et de base
	void SetValueNumber(int nValue);
	int GetValueNumber() const;

	// Parametrage des valeurs
	void SetValueAt(int nIndex, Continuous cValue);
	Continuous GetValueAt(int nIndex) const;

	// Acces direct au vecteur de valeurs - methode avancee
	const ContinuousVector* GetValues() const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard

	// Creation
	KWDerivationRule* Create() const override;

	// Nettoyage de l'interface de base une fois la regle compilee
	void CleanCompiledBaseInterface() override;

	//////////////////////////////////////////////////////
	// Redefinition des methodes de structure

	// Recopie de la partie structure de la regle
	void CopyStructureFrom(const KWDerivationRule* kwdrSource) override;

	// Transfert de la specification de base de la regle source
	// vers la specification de structure de la regle en cours
	void BuildStructureFromBase(const KWDerivationRule* kwdrSource) override;

	// Affichage, ecriture dans un fichier
	void WriteStructureUsedRule(ostream& ost) const override;

	// Methode de comparaison entre deux regles
	int FullCompare(const KWDerivationRule* rule) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Valeurs
	ContinuousVector cvValues;
};

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueAt
// Regle de derivation basee en un ensemble de valeurs et un index,
// renvoyant la valeur correspondant a l'index (entre 1 et N)
// (valeur manquante si index erronne)
// Premier operande: regle KWDRSymbolVector
// Deuxieme operande: index
class KWDRSymbolValueAt : public KWDerivationRule
{
public:
	// Constructeur
	KWDRSymbolValueAt();
	~KWDRSymbolValueAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Symbol ComputeSymbolResult(const KWObject* kwoObject) const override;
};

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueAt
// Regle de derivation basee en un ensemble de valeurs et un index,
// renvoyant la valeur correspondant a l'index (entre 1 et N)
// (valeur manquante si index erronne)
// Premier operande: regle KWDRContinuousVector
// Deuxieme operande: index
class KWDRContinuousValueAt : public KWDerivationRule
{
public:
	// Constructeur
	KWDRContinuousValueAt();
	~KWDRContinuousValueAt();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Continuous ComputeContinuousResult(const KWObject* kwoObject) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsSymbolVector
// Regle de derivation de type Structure(VectorC), basee sur l'extraction
// de valeurs Symbol a partir d'une chaine de caracteres avec separateur blanc
class KWDRAsSymbolVector : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAsSymbolVector();
	~KWDRAsSymbolVector();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRSymbolVector symbolVector;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRAsContinuousVector
// Regle de derivation de type Structure(ContinuousVector), basee sur l'extraction
// de valeurs Continuous a partir d'une chaine de caracteres avec separateur blanc
class KWDRAsContinuousVector : public KWDerivationRule
{
public:
	// Constructeur
	KWDRAsContinuousVector();
	~KWDRAsContinuousVector();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRContinuousVector continuousVector;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRExtractWords
// Regle de derivation de type Structure(VectorC), basee sur l'extraction
// de mots a partir d'une chaine de caracteres.
// Les caracteres sont d'abord pretraites et transformes selon les parametres
// d'extraction, tous les autres caracteres etant transformes en caracteres blancs.
// Les mots sont alors extraits de la chaine de catacteres pretraites avec separateur blanc.
// Les parametres d'extraction sont les suivants
//      - SourceString: chaine de caracteres initiale
//		- ToLower: mise en minuscule
//      - KeepNumerical: on garde les caracteres numeriques
//      - AdditionalChars: caracteres supplementaires a garder
//      - TranslatedAdditionalChars: version recodee des caracteres additionnels
//      - MaxLength: longueur max des mots, au dela de laquels les mots sont tronques
class KWDRExtractWords : public KWDerivationRule
{
public:
	// Constructeur
	KWDRExtractWords();
	~KWDRExtractWords();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	// Verification des parametres d'extraction
	boolean CheckOperandsDefinition() const override;

	// Compilation redefinie pour optimisation
	void Compile(KWClass* kwcOwnerClass) override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Resultat utilise pour le code retour de la regle
	mutable KWDRSymbolVector symbolVector;

	/////////////////////////////////////////////////////////////////////////////////
	// Resultats de l'optimisation des parametres d'extraction

	// Vecteur des caracteres recodes suite a application des parametres d'extraction
	// Chaque caracteres (entre 0 et 255) est ainsi garde tel quel, mise en minuscule,
	// recode ou transforme en blanc
	ALString sTranslatedChars;

	// Longuer max des mots
	int nMaxLength;
};

///////////////////////////////////////////////////////////
// Methodes en inline

inline void KWDRSymbolVector::SetValueNumber(int nValue)
{
	require(nValue >= 0);

	svValues.SetSize(nValue);
	bStructureInterface = true;
	nFreshness++;
}

inline int KWDRSymbolVector::GetValueNumber() const
{
	if (bStructureInterface)
		return svValues.GetSize();
	else
		return GetOperandNumber();
}

inline void KWDRSymbolVector::SetValueAt(int nIndex, const Symbol& sValue)
{
	require(0 <= nIndex and nIndex < svValues.GetSize());
	require(bStructureInterface);

	svValues.SetAt(nIndex, sValue);
	nFreshness++;
}

inline Symbol& KWDRSymbolVector::GetValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < svValues.GetSize());
	require(bStructureInterface);

	return svValues.GetAt(nIndex);
}

inline const SymbolVector* KWDRSymbolVector::GetValues() const
{
	require(bStructureInterface);
	return &svValues;
}

inline void KWDRContinuousVector::SetValueNumber(int nValue)
{
	require(nValue >= 0);

	cvValues.SetSize(nValue);
	bStructureInterface = true;
	nFreshness++;
}

inline int KWDRContinuousVector::GetValueNumber() const
{
	if (bStructureInterface)
		return cvValues.GetSize();
	else
		return GetOperandNumber();
}

inline void KWDRContinuousVector::SetValueAt(int nIndex, Continuous cValue)
{
	require(0 <= nIndex and nIndex < cvValues.GetSize());
	require(bStructureInterface);

	cvValues.SetAt(nIndex, cValue);
	nFreshness++;
}

inline Continuous KWDRContinuousVector::GetValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < cvValues.GetSize());
	require(bStructureInterface);

	return cvValues.GetAt(nIndex);
}

inline const ContinuousVector* KWDRContinuousVector::GetValues() const
{
	require(bStructureInterface);
	return &cvValues;
}