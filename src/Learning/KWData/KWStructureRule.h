// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDerivationRule.h"

///////////////////////////////////////////////////////////////
// Classe KWDRStructureRule
// Regle de derivation de type Structure, destinee a etre
// specialisee dans des sous-classes.
//
// Les regles de type Structure sont adaptees au traitement efficace
// des vecteurs contenant un grand nombre de valeurs constantes.
// Pour des raison de performanace, on se limite au regle d'ayant
// qu'un seul type operande, de type simple, et d'origine constante.
// (on evite ainsi les traitements complexes de gestion des operandes
// a scope multiples)
//
// La specification de la regle se fait au moyen de deux
// types de methodes:
//    . base: la specification par les operandes de la regle
//    . structure: la specification par des methodes dediees a la structure
// La specification de base est reservee a la lecture de la classe depuis un fichier.
// La specification de structure permet:
//    . un acces efficace en consultation/mise a jour par programme
//    . un stockage optimise en memoire
//    . une implementation optimisee des methodes de calcul
//    . la reutilisation directe de methode C++ pour les methodes
//
// Des que l'on passe a l'interface de structure (utilisation directe
// ou indirecte via la premiere compilation), l'interface de base ne doit
// plus etre utilisee
//
// La classe KWDRStructureRule identifie l'ensemble des methodes
// impactee a reimplementees
//
// Remarque: il n'est pas necessaire d'heriter de KWDRStructureRule pour
// toutes les regles de type structure (utile essentiellement si l'on
// a besoin d'une interface de type structure en plus de l'interface de base)
class KWDRStructureRule : public KWDerivationRule
{
public:
	// Constructeur:
	// par defaut: type Structure de la regle,
	// declaration d'un flag bStructureSpec a false par defaut,
	// A reimplementer: description des operandes de base de la regle,
	// initialisation a vide des attributs de structure
	KWDRStructureRule();

	// Destructeur
	// A reimplementer: nettoyage des attributs de structure
	~KWDRStructureRule();

	// Flag d'utilisation de l'interface de Structure
	// Par defaut: des que l'on a effectuer des mises a jour en
	// interface de structure, ou apres la compilation
	boolean GetStructureInterface() const;

	//////////////////////////////////////////////////////////////
	// Specification de la regle au moyen de l'interface de structure
	// Toute utilisation de ces methode fait passer le flag bStructureSpec
	// a true de facon definitive.
	// A reimplementer; chaque Setter doit incrementer la fraicheur (nFreshness++)
	// et positionner le flag bStructureSpec a true
	// Chaque Getter doit etre const.

	/////////////////////////////////////////////////////
	// Services specifiques, disponibles une fois compile
	// A reimplementer (en methode const) selon les besoins

	//////////////////////////////////////////////////////
	// Redefinition des methodes standards, a reimplementer

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'objet Structure (renvoie la regle elle meme (this))
	// Reimplementation facultative (l'interface de structure peut etre
	// suffisante, sauf si elle depend de kwoObject)
	Object* ComputeStructureResult(const KWObject* kwoObject) const override;

	//////////////////////////////////////////////////////
	// Methode specifique a l'interface de structure a reimplementer

	// Verification de la partie structure de la regle
	virtual boolean CheckStructureDefinition() const;

	// Compilation de l'interface de structure de la regle de derivation
	// Par defaut: aucun traitement (l'interface de structure peut etre suffisante)
	virtual void CompileStructure(KWClass* kwcOwnerClass);

	// Recopie de la partie structure de la regle
	virtual void CopyStructureFrom(const KWDerivationRule* kwdrSource);

	// Transfert de la specification de base de la regle source
	// vers la specification de structure de la regle en cours
	// Doit faire passer en interface de structure
	// La regle doit avoir une definition valide en prerequis (CheckDefinition)
	virtual void BuildStructureFromBase(const KWDerivationRule* kwdrSource);

	// Nettoyage de l'interface de base une fois la regle specifiee sous forme structure
	// Permet d'optimiser la place memoire
	// Par defaut: aucun nettoyage effectue
	virtual void CleanCompiledBaseInterface();

	// Affichage, ecriture dans un fichier
	// Ecriture de la regle avec l'interface de structure,
	// en respectant les regles de parenthesage de l'interface
	// de base.
	// Permet de supprimer l'interface de base lors de la compilation,
	// de facon a optimiser l'occupation memoire
	// Par defaut: WriteUsedRule(ost)
	virtual void WriteStructureUsedRule(ostream& ost) const;

	//////////////////////////////////////////////////////
	// Redefinition des methodes standard, reimplementees
	// une fois pour toutes

	// Compilation de la regle de derivation
	// Transfert si necessaire de l'interface de base vers
	// l'interface de structure
	// Nettoyage (optionnel) de l'interface de base
	// Compilation (optionnelle) de l'interface de structure
	void Compile(KWClass* kwcOwnerClass) override;

	// Recopie des attributs de definition de la regle
	// Re-aiguillage vers les methodes standard ou vers les
	// methode dediees structure selon le flag bStructureSpec
	void CopyFrom(const KWDerivationRule* kwdrSource) override;

	// Verification
	// Re-aiguillage vers les methodes standard ou vers les
	// methode dediees structure selon le flag bStructureSpec
	boolean CheckDefinition() const override;

	// Methode de comparaison entre deux regles
	// A reimplementer dans le cas ou l'interface de structure est utilisee
	int FullCompare(const KWDerivationRule* rule) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Affichage, ecriture dans un fichier
	// Re-aiguillage vers les methodes standard ou vers les
	// methode dediees structure selon le flag bStructureSpec
	void Write(ostream& ost) const override;
	void WriteUsedRule(ostream& ost) const override;

	//////////////////////////////////////////////////////////
	///// Implementation

	// Indique si une regle est de type KWStructureRule (pour l'optimisation du parser)
	boolean IsStructureRule() const override;

protected:
	// Flag d'utilisation de l'interface de Structure
	boolean bStructureInterface;
};
