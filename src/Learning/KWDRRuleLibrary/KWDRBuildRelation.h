// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Regles de derivation de creation de Table

class KWDRBuildTableView;
class KWDRBuildTableAdvancedView;
class KWDRBuildEntityView;
class KWDRBuildEntityAdvancedView;
class KWDRBuildEntity;
class KWDRBuildDiffTable;
class KWDRBuildList;
class KWDRBuildDummyTable;

#include "KWDerivationRule.h"
#include "KWRelationCreationRule.h"

// Enregistrement de ces regles
void KWDRRegisterBuildRelationRules();

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTableView
// Creation d'une vue sur une table
// Chaque attribut natif de la table en sortie doit correspondre a un attribut
// natif ou calcule de la table en entree
class KWDRBuildTableView : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRBuildTableView();
	~KWDRBuildTableView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject,
					      const KWLoadIndex liAttributeLoadIndex) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildTableAdvancedView
// Creation d'une vue sur une table, avec possibilite d'alimenter des attributs
// de la table en sortie directement via des des operandes en sortie et
// des valeurs fournies par les operandes en entree corespondants
class KWDRBuildTableAdvancedView : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRBuildTableAdvancedView();
	~KWDRBuildTableAdvancedView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject,
					      const KWLoadIndex liAttributeLoadIndex) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntityView
// Creation d'une vue sur une Entity
// Chaque attribut natif de l'entite en sortie doit correspondre a un attribut
// natif ou calcule de l'entite en entree
class KWDRBuildEntityView : public KWDRBuildTableView
{
public:
	// Constructeur
	KWDRBuildEntityView();
	~KWDRBuildEntityView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntityAdvancedView
// Creation d'une vue sur une entite, avec possibilite d'alimenter des attributs
// de l'entite en sortie directement via des des operandes en sortie et
// des valeurs fournies par les operandes en entree corespondants
class KWDRBuildEntityAdvancedView : public KWDRBuildTableAdvancedView
{
public:
	// Constructeur
	KWDRBuildEntityAdvancedView();
	~KWDRBuildEntityAdvancedView();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildEntity
// Creation d'une entite en alimentant chaque attribut natif de l'entite en sortie
// via un operande en sortie designant l'attribut a alimenter et une valeur issue
// de l'operande correspondante en entree
class KWDRBuildEntity : public KWDRRelationCreationRule
{
public:
	// Constructeur
	KWDRBuildEntity();
	~KWDRBuildEntity();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Pas d'alimentation de type vue
	boolean IsViewModeActivated() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildDiffTable
// Creation d'une table cible avec calcul des differences de valeur entre deux
// records successifs pour un sous-ensemble des attributs sources choisis en entree:
// - chaque attribut natif de la table en sortie ayant meme nom et type qu'un attribut
//   natif ou calcule de la table en entree est recopie tel quel, comme dans BuildTableView
// - la table source en premier operande doit etre triee selon le besoin utilisateur
// - les attributs sources dont on doit calculer la difference sont a specifie en entree
//   a partir du deuxieme operande
// - les variable en sortie memorisant les difference sont specifie dans les operandes en
//   sortie de la regle, en meme nombre et position que les attributs correspondant en entree
// - les differences de variables sont calculee selon les types suivant
//   - Numerical: difference numerique
//   - Time: difference en secondes
//   - Date: difference en jours
//   - Timestamp, TimestampTZ: difference en secondes
//   - Categorical: 1 si valeur differente, 0 sinon
// - la table en sortie contient autant de record que la table en entree
//   - pour le premier record en sortie, les valeurs des differences sont Missing
class KWDRBuildDiffTable : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRBuildDiffTable();
	~KWDRBuildDiffTable();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject,
					      const KWLoadIndex liAttributeLoadIndex) const override;

	// Verification qu'une regle est une specialisation d'une regle plus generale
	boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul des attributs cibles en sortie comme des difference des valeurs de attributs specifie dans
	// les operande en entree pour deux objet successifs
	// Si le premier objet est NULL, ces differeces sont missing
	void FillTargetDifferenceAttributes(const KWObject* kwoSourcePreviousObject,
					    const KWObject* kwoSourceCurrentObject, KWObject* kwoTargetObject) const;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildList
// Creer une table cible a partir d'une table source, ou chaque instance cible contient une
// reference a l'instance source courante et des liens vers les elements precedent et suivant de la liste.
// - La table source doit etre triee selon les besoins de l'utilisateur.
// - Le dictionnaire cible doit contenir une variable de type Entity(SourceDir) contenant l'instance source courante,
//   ainsi que deux variables de type Entity(TargetDic), deseignees dans la regle comme Prev et Next,
//   destinees a contenir des references aux elements precedent et suivant de la liste.
// - Cela permet d'ajouter de nouvelles variables derivees dans le dictionnaire cible qui exploitent les valeurs
//   de l'instance courante et de celles de ses precedentes ou suivantes, voire plusieurs instances precedentes.
// - La premiere instance cible n'a pas de reference precedente, et la derniere n'a pas de reference suivante.
class KWDRBuildList : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRBuildList();
	~KWDRBuildList();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject,
					      const KWLoadIndex liAttributeLoadIndex) const override;

	// Verification du type des operandes en sortie
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Pas d'alimentation de type vue
	boolean IsViewModeActivated() const override;

	// Redefinition des methodes virtuelles
	void CollectMandatoryInputOperands(IntVector* ivUsedInputOperands) const override;
	void CollectSpecificInputOperandsAt(int nOutputOperand, IntVector* ivUsedInputOperands) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildGraph
// Creer un graphe non oriente a partir d'une table d'entree de noeuds et d'une table d'entree d'aretes,
// ou chaque arete d'entree possede une paire d'identifiants de noeuds.
//
// Entrees :
// - Table(NodeDataDic) inputNodeTable : table des noeuds d'entree
//   - Categorical nodeId : identifiant des noeuds dans inputNodeTable
// - Table(EdgeDataDic) inputEdgeTable : table des aretes d'entree
//   - Categorical node1Id, node2Id : identifiants des paires de noeuds associees a chaque arete dans inputEdgeTable
// Sorties :
// - Table(NodeDic) graphNodeTable : table des noeuds du graphe
//   - Entity(NodeDataDic) nodeData : stocke les donnees de la source du noeud dans chaque noeud
//   - Table(EdgeDic) nodeAdjacentEdges : liste des aretes adjacentes a chaque noeud
// - Table(EdgeDic) graphEdgeTable : table des aretes du graphe
//   - Entity(EdgeDataDic) edgeData : stocke les donnees de la source de l'arete dans chaque arete
//   - Entity(NodeDic) edgeNode1, edgeNode2 : references aux entites de noeuds connectes par chaque arete

// Details sur les structures de donnees de sortie :
// - Le dictionnaire de sortie GraphDic doit contenir :
//   - Une variable de type Table(NodeDic) pour les noeuds du graphe.
//   - Une variable de type Table(EdgeDic) pour les aretes du graphe.
// - Le dictionnaire NodeDic doit inclure :
//   - Une variable de type NodeDataDic referencees aux donnees des noeuds d'entree.
//   - Une variable de type Table(EdgeDic) referencees a la liste des aretes adjacentes.
// - Le dictionnaire EdgeDic doit inclure :
//   - Une variable de type EdgeDataDic referencees aux donnees des aretes d'entree.
//   - Deux variables de type Entity(NodeDic) referencees aux noeuds connects (edgeNode1 et edgeNode2).
//
// Considerations supplementaires :
// - Les multigraphes sont autorises, ce qui signifie que plusieurs aretes peuvent relier la meme paire de noeuds.
// - Pour les graphes orientes, les aretes adjacentes peuvent etre separees en aretes entrantes et sortantes
//   en utilisant la regle TableSelection.
// - Gestion des incoherences :
//   - Si plusieurs noeuds d'entree partagent le meme identifiant, seul le premier noeud
//     est conserve pour construire le noeud du graphe ; les autres sont ignores.
//   - Si les aretes d'entree font reference a des noeuds manquants, les aretes du graphe correspondantes ne sont pas creees.
class KWDRBuildGraph : public KWDRRelationCreationRule
{
public:
	// Constructeur
	KWDRBuildGraph();
	~KWDRBuildGraph();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	KWObject* ComputeObjectResult(const KWObject* kwoObject, const KWLoadIndex liAttributeLoadIndex) const override;

	// Verification du type des operandes en sortie
	boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const override;

	// Specialisation des methodes de gestion du scope des operandes en entree
	boolean IsNewScopeOperand(int nOperandIndex) const override;
	boolean IsSecondaryScopeOperand(int nOperandIndex) const override;

	// Specialisation des methodes de gestion du scope des operandes en sortie
	boolean IsNewOutputScopeOperand(int nOutputOperandIndex) const override;
	boolean IsSecondaryOutputScopeOperand(int nOutputOperandIndex) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Verification d'un operande en entree
	boolean CheckOperandCompletenessAt(const KWClass* kwcOwnerClass, int nIndex) const;

	// Pas d'alimentation de type vue
	boolean IsViewModeActivated() const override;

	// Redefinition des methodes virtuelles
	void CollectMandatoryInputOperands(IntVector* ivUsedInputOperands) const override;
	void CollectSpecificInputOperandsAt(int nOutputOperand, IntVector* ivUsedInputOperands) const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe KWDRBuildDummyTable
// Creation d'une table factice avec autant d'instances que specifie en parametre
// Regle interne, uniquement pour des raison de test de volumetrie
class KWDRBuildDummyTable : public KWDRTableCreationRule
{
public:
	// Constructeur
	KWDRBuildDummyTable();
	~KWDRBuildDummyTable();

	// Creation
	KWDerivationRule* Create() const override;

	// Calcul de l'attribut derive
	ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject,
					      const KWLoadIndex liAttributeLoadIndex) const override;

	///////////////////////////////////////////////////////
	///// Implementation
protected:
	// Pas d'alimentation de type vue
	boolean IsViewModeActivated() const override;
};
