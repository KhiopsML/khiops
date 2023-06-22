// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWClass;
class KWAttribute;
class KWAttributeBlock;
class KWObject;
class KWObjectKey;
class KWDerivationRule;
class KWDerivationRuleOperand;
class KWDatabase;

#include "Object.h"
#include "KWSymbol.h"
#include "KWType.h"
#include "KWLoadIndex.h"
#include "KWIndexedKeyBlock.h"
#include "KWCDUniqueString.h"

/////////////////////////////////////////////////////////////////
//// Definition d'une regle de derivation standard
/////////////////////////////////////////////////////////////////
// Une regle de derivation est implementee dans une sous-classe
// de KWDerivationRule
//
//	  . Le nom de la classe est la concatenation de KWDR et
//		du nom de la regle
//	  . Le libelle est un commentaire ligne decrivant la regle
//    . La documentation de la classe reprend le prototype
//		de la regle, et une explication claire de la regle
//	  . Le constructeur cree et initialise la valeur de tous
//		les operandes
//
//	  . La methode Create est implementee, et permet de servir
//		de base a l'instanciation de regles specifiques
//
//	  . L'algorithme de derivation doit etre implemente en redefinissant
//      la variante de Compute<Type>Result endant un type compatible
//      avec celui de la regle.
//			- prerequis: IsCompiled() de KWDerivationRule
//			- acces aux valeurs des operandes par les methodes
//            Get<Type>Value de la classe KWDerivationRuleOperand
//				 GetSymbolValue
//				 GetContinuousValue
//				 GetDateValue
//				 GetTimeValue
//				 GetTimestampValue
//				 GetTimestampTZValue
//				 GetTextValue
//				 GetObjectValue
//				 GetObjectArrayValue
//				 GetStructureValue
//				 GetSymbolValueBlock
//				 GetContinuousValueBlock
//				 GetObjectArrayValueBlock
//
//	  . Pour etre utilisable, une regle de derivation doit etre
//		enregistree aupres de KWDerivationRule
//		(ce qui est automatiquement le cas pour les	regles
//		de derivation standard)
//
//    . Attention: quand les attributs intervenant dans une
//      une regle de derivation sont de type Object ou
//		ObjectArray, ils peuvent etre NULL
//    . Les attributs de type Structure ne doivent pas etre a NULL;
//      ils sont necessairement produits par une regle de derivation
//	  . Attention: un attribut objet "appele" peut (c'est autorise!)
//		appartenir a un autre domaine de classe que l'objet appelant.
//		Dans ce cas, il peut avoir une classe (KWClass) differente
//		de la classe correspondante (de meme nom) du domaine de classe
//		de l'objet appelant. Cela peut etre controle par verification
//		du domaine. Attention donc.

//////////////////////////////////////////////////////////////////////////
// Regles de derivation
// La classe KWDerivationRule permet la specification d'une
// regle de derivation.
// Cette classe est assiste par la classe KWDerivationRuleOperand
// permettant la specifications de parties d'une regle de derivation

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDerivationRule
// Un regle de derivation sert a modeliser la facon dont une valeur peut
// etre calculee a partir d'autres valeurs provenant de constantes, d'attributs
// de classe, ou d'autres regles.
//
// La specification d'un regles de derivation fait appel a la classe
// KWDerivationRuleOperand.
//
// Les attributs suivants permettent de definir une regle de derivation:
// 	Regle
// 		Name
// 		Label
//	Classe sur laquelle la regle est applicable
// 		ClassName
// 	Type de valeur resultat
//      ResultType
//      ResultClassName
// 	Operandes
//
// Les Names (sauf celui de la regle) sont a vide pour les regles generiques, et
// non vides pour les regles specifiques ou instanciees.
// Tout ou partie d'une regle peut etre generique.
// Le nombre d'operandes peut etre variable ou fixe (eventuellement 0).
class KWDerivationRule : public Object
{
public:
	// Constructeur
	KWDerivationRule();
	~KWDerivationRule();

	//////////////////////////////////////////////////
	// Definition de base de la regle

	// Nom de la regle: obligatoire
	void SetName(const ALString& sValue);
	const ALString& GetName() const;

	// Libelle
	void SetLabel(const ALString& sValue);
	const ALString& GetLabel() const;

	/// Nom de la classe sur laquelle la regle est applicable
	void SetClassName(const ALString& sValue);
	const ALString& GetClassName() const;

	//////////////////////////////////////////////////////
	// Renommage d'une classe ou d'un attribut, avec
	// propagation aux operandes et sous-regles

	// Renommage d'une classe
	void RenameClass(const KWClass* refClass, const ALString& sNewClassName);

	// Renommage d'un attribut d'une classe
	virtual void RenameAttribute(const KWClass* kwcOwnerClass, KWAttribute* refAttribute,
				     const ALString& sNewAttributeName);

	//////////////////////////////////////////////////
	// Type de la valeur retournee par la regle

	// Type de la regle: obligatoire
	// Par defaut: KWType::Unknown
	void SetType(int nValue);
	int GetType() const;

	// Nom de la classe pour les types Object et ObjectArray
	const ALString& GetObjectClassName() const;
	void SetObjectClassName(const ALString& sValue);

	// Utilisation des attributs de type objets par referencement (sinon: sous-partie)
	// Par defaut: true
	virtual boolean GetReference() const;

	// Nom de la structure pour le type Structure
	const ALString& GetStructureName() const;
	void SetStructureName(const ALString& sValue);

	// Nom generique du complement du type, selon le type (ObjectClassName, StructureName, sinon vide)
	const ALString& GetSupplementTypeName() const;
	void SetSupplementTypeName(const ALString& sValue);

	// Type de cle utilisee pour les variables du bloc, dans le cas d'un type bloc
	// Valeurs possibles:
	//   KWType::Symbol: cles categorielles (ex: <VarKey="dix">, <VarKey="1">
	//   KWType::Continuous: cles numeriques entieres, plus grande que 1 (ex: <VarKey=1>, <VarKey=10>
	//   KWType::None: valeur par defaut, si la variable n'est pas de type bloc
	virtual int GetVarKeyType() const;

	//////////////////////////////////////////////////
	// Gestion des operandes

	// Nombre d'operandes
	int GetOperandNumber() const;

	// Initialisation/modification du nombre d'operande
	// Les operandes en trop sont supprimes, ceux en plus sont ajoutes
	void SetOperandNumber(int nValue);

	// Indicateur de nombre variable d'operandes (defaut: false)
	// Dans ce cas, les premiers operandes constituent la partie fixe de
	// de la regle, et le dernier operande de la regle "generique" permet
	// de preciser les contraintes sur le type des operandes en nombre variable.
	// La partie variable des operandes (instanciee dans une regle "specifique")
	// peut comprendre zero a plusieurs operandes.
	// Si le type du dernier operande n'est pas specifie (KWType::Unknown),
	// c'est a la regle de controler les types (methode Check) de ses operandes
	// en nombre variable (qui peuvent etre potentiellement de type differents)
	void SetVariableOperandNumber(boolean bValue);
	boolean GetVariableOperandNumber() const;

	// Acces aux operandes
	KWDerivationRuleOperand* GetOperandAt(int nIndex) const;

	// Acces privilegie aux premiers operandes
	KWDerivationRuleOperand* GetFirstOperand() const;
	KWDerivationRuleOperand* GetSecondOperand() const;

	// Ajout d'un operande en fin de liste
	void AddOperand(KWDerivationRuleOperand* operand);

	// Destruction du dernier operande
	void DeleteLastOperand();

	// Destruction de tous les operandes
	void DeleteAllOperands();

	// Supression de tous les operandes, sans les detruire
	void RemoveAllOperands();

	// Destruction de tous les operandes variables
	// Les operandes de tete (en nombre fixe) sont preserves
	// Methode sans effet pour les regles a nombre fixe d'operandes
	void DeleteAllVariableOperands();

	// Construction du dictionnaire de tous les operandes utilises directement
	// ou recursivement via d'autres regles de derivation
	// Cela vaut pour des operandes constant, regles, attributs, ou operande
	// intervenant dans le calcul d'un attribut operande
	// Le dictionnaire est complete par la methode, et evitera les eventuels
	// cycle de derivation
	void BuildAllUsedOperands(NumericKeyDictionary* nkdAllUsedOperands) const;

	// Construction du dictionnaire de tous les attributs utilises
	// directement ou recursivement via d'autres regles de derivation
	//  . derivedAttribute: attribut a l'origine de la regle
	//     permet, si l'attribut est dans un bloc, de recherche les attributs de
	//     meme VarKey potentiellement impactes dans les bloc en operandes de la regles
	//  . nkdAllUsedAttributes: le dictionnaire est complete par la methode
	//     et evitera les eventuels cycles de derivation
	virtual void BuildAllUsedAttributes(const KWAttribute* derivedAttribute,
					    NumericKeyDictionary* nkdAllUsedAttributes) const;

	//////////////////////////////////////////////////////////////////////////
	// Gestion du scope des operandes
	//
	// Dans le cas standard, tous les operandes ont le meme scope, celui de la
	// classe utilisant la regle de derivation pour le calcul d'un de ses attributs.
	//
	// Dans une regle a scope multiple, on a:
	//  . un premier operande de type Relation
	//     . au niveau du scope principal (celui de la regle)
	//     . definissant le scope secondaire
	//         (celui de l'Object ou ObjectArray du premier operande)
	//  . les autre operandes sont au niveau du scope secondaire
	//  . les operandes de scope inferieur (autre operande, ou operande
	//    de leurs sous-regles), peuvent avoir un ScopeLevel positif,
	//    leur permettant de remonter au scope principal
	//    Exemple:
	//      TableSelection(Logs, G(DiffTime(LogTime, 10), 0));
	//         (LogTime est de scope secondaire)
	//      TableSelection(Logs, G(DiffTime(LogTime, .UserTime), 0));
	//         (UserTime est de scope principal)
	// Le ScopeLevel (par defaut 0) permet de remonter le scope de un
	// a plusieurs niveau
	//   1 pour remonter un niveau (".")
	//   2 pour remonter un niveau ("..")

	// Indique si la regle est avec gestion de scope multiple ou standard (par defaut)
	boolean GetMultipleScope() const;
	void SetMultipleScope(boolean bValue);

	// Indique si un operande est du scope secondaire, en cas de regle avec gestion de scope multiple
	virtual boolean IsSecondaryScopeOperand(int nOperandIndex) const;

	//////////////////////////////////////////////////////////////////////////
	// Methodes de verification
	// Ces methodes se decomposent en deux parties, sur la regle et sur ses operandes
	// ce qui facilite la reimplementation partielle ou totale

	// Verification qu'une regle est suffisament definie pour etre generalisee
	virtual boolean CheckDefinition() const;
	virtual boolean CheckRuleDefinition() const;
	virtual boolean CheckOperandsDefinition() const;

	// Verification qu'une regle est une specialisation d'une regle plus generale
	// suffisament definie
	virtual boolean CheckFamily(const KWDerivationRule* ruleFamily) const;
	virtual boolean CheckRuleFamily(const KWDerivationRule* ruleFamily) const;
	virtual boolean CheckOperandsFamily(const KWDerivationRule* ruleFamily) const;

	// Verification qu'une regle est completement renseignee et compilable
	virtual boolean CheckCompleteness(const KWClass* kwcOwnerClass) const;
	virtual boolean CheckRuleCompletness(const KWClass* kwcOwnerClass) const;
	virtual boolean CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const;

	// Verification que les attributs d'un bloc sont tous presents via leur VarKey
	// dans les blocs en operandes de la regle
	// Les messages sont emis pour le bloc d'attribut en paremetre, associe a la regle a verifier
	virtual boolean CheckBlockAttributes(const KWClass* kwcOwnerClass,
					     const KWAttributeBlock* attributeBlock) const;
	virtual boolean CheckBlockAttributesAt(const KWClass* kwcOwnerClass, const KWAttributeBlock* attributeBlock,
					       int nOperandIndex) const;

	// Controle d'erreur sur une regle utilise directement ou indirectement (via un attribut)
	// par un operande de la regle
	// L'operande doit comporter une regle, ou referencer un attribut base sur la regle, et la regle
	// doit etre du bon type (si RuleName est non vide)
	boolean CheckReferencedDerivationRuleAt(int nOperandIndex, const KWClass* kwcOwnerClass,
						const ALString& sRuleName) const;

	// Verification de l'absence de cycle de derivation
	// On passe en parametre une liste d'attributs colore en Grey ou en Black
	// (cf. algo decrit dans l'implementation de KWClassDomain::Compile())
	// Prerequis: la classe doit etre compilee
	// Retourne true si pas de cycle, sinon false en emmetant des messages d'erreur
	boolean ContainsCycle(NumericKeyDictionary* nkdGreyAttributes, NumericKeyDictionary* nkdBlackAttributes) const;

	/////////////////////////////////////////////////
	// Exploitation des regles de derivation
	// Methodes redefinissables dans les sous-classes

	// Completion eventuelle de la regle avec les informations de type
	void CompleteTypeInfo(const KWClass* kwcOwnerClass);

	// Compilation de la regle de derivation, permettant de rendre son
	// execution efficace
	// La compilation se fait par rapport a la classe
	// contenant l'attribut derive
	virtual void Compile(KWClass* kwcOwnerClass);
	boolean IsCompiled() const;

	// Acces a la classe liee a la regle verifiee ou compilee
	// Methode avancee
	const KWClass* GetOwnerClass() const;

	// Calcul de la valeur resultat de la regle
	// Attention, seule la variante compatible avec le type de la regle
	// doit etre reimplementee
	// Gestion memoire (cf classe KWObject)
	//  - type simple (Continuous et Symbol): ce sont des valeurs, et cela ne pose pas de probleme
	//  - type complex (Date, Time, Timestamp, TimestampTZ): ce sont egalement des valeurs, et cela ne pose pas de
	//  probleme
	//  - type Text: ce sont egalement des valeurs (comme les Symbol), et cela ne pose pas de probleme
	//  - type TextList: vecteur de valeurs, detruit avec les KWObjet le contenant
	//    Une regle de calcul produisant un result TextList doit en garder la copie.
	//    Le KWObjet memorisant un TextLits fera une copie des TextList renvoyes par les regles
	//  - type object (Object et ObjectArray): selon qu'il soient internes ou references, ils appartiennent
	//    a leur KWObject englobant et seront detruits par celui ci, ou sont simplement reference et sont
	//    censes appartenir a un autre objet ou etre extrait d'un autre attribut
	//    Une regle de calcul produisant des objets internes est responsable de leur destruction
	//    Une regle de calcul produisant un ObjectArray doit en garder la copie et est responsable
	//    de sa destruction. S'ils sont utilises comme attributs d'objets, ceux-ci doivent en garder
	//    une copie par duplication.
	//  - type structure: le type Structure est cense appartenir a la KWClass, pas au KWObject
	//    Il est donc detruit uniquement avec la KWClass, jamais par le KWObject
	//    En pratique, une facon simple de respecter cette contrainte est de faire en sorte de la methode
	//    ComputeStructureResult renvoie directement un pointeur sur la KWDerivationRule.
	//    Dans le cas de regles de classe (par exemple: specification d'une discretisation), l'objet est
	//    cree et initialise une seule fois.
	//    Dans le cas de regles d'instance (par exemple: un classifieur dependant des valeur du KWObject),
	//    l'objet est cree une seule fois, mais mis a jour pour chaque KWObject.
	//  - type bloc de valeurs (ContinuousValueBlock, SymbolValueBlock, ObjectArrayValueBlock): un bloc doit etre
	//    cree a chaque calcul effectue par la regle.
	//    Si le bloc est utilise dans un attribut, l'attribut le memorise directement et l'objet contenant
	//    l'attribut detruira les blocs lors de sa destruction. Si le bloc est utilise en operande de regle de
	//    derivation, la regle de derivation n'aura pas a detruire le bloc car un bloc est necessairement de type
	//    OriginAttribute (OriginRule est interdit, car on n'aurait pas acces aux VarKeys). Un bloc de cle indexes
	//    est egalement passe en parametre, pour specifie les cles d'attributs a calculer et les memoriser selon
	//    leur index dans le bloc de valeurs.
	virtual Continuous ComputeContinuousResult(const KWObject* kwoObject) const;
	virtual Symbol ComputeSymbolResult(const KWObject* kwoObject) const;
	virtual Date ComputeDateResult(const KWObject* kwoObject) const;
	virtual Time ComputeTimeResult(const KWObject* kwoObject) const;
	virtual Timestamp ComputeTimestampResult(const KWObject* kwoObject) const;
	virtual TimestampTZ ComputeTimestampTZResult(const KWObject* kwoObject) const;
	virtual Symbol ComputeTextResult(const KWObject* kwoObject) const;
	virtual SymbolVector* ComputeTextListResult(const KWObject* kwoObject) const;
	virtual KWObject* ComputeObjectResult(const KWObject* kwoObject) const;
	virtual ObjectArray* ComputeObjectArrayResult(const KWObject* kwoObject) const;
	virtual Object* ComputeStructureResult(const KWObject* kwoObject) const;
	virtual KWContinuousValueBlock*
	ComputeContinuousValueBlockResult(const KWObject* kwoObject, const KWIndexedKeyBlock* indexedKeyBlock) const;
	virtual KWSymbolValueBlock* ComputeSymbolValueBlockResult(const KWObject* kwoObject,
								  const KWIndexedKeyBlock* indexedKeyBlock) const;
	virtual KWObjectArrayValueBlock*
	ComputeObjectArrayValueBlockResult(const KWObject* kwoObject, const KWIndexedKeyBlock* indexedKeyBlock) const;

	// Valeur par defaut des blocs pour les regles retournant un bloc de valeurs
	// (NULL dans le cas des ObjectArrayValueBlock)
	// La variante compatible avec le type de la regle doit etre reimplementee
	virtual Continuous GetValueBlockContinuousDefaultValue() const;
	virtual Symbol& GetValueBlockSymbolDefaultValue() const;

	// Calcul du nom d'un attribut en fonction du nom de la regles et de ses operandes
	virtual const ALString ComputeAttributeName() const;

	// Duplication (s'appuie sur Create et CopyFrom)
	KWDerivationRule* Clone() const;

	// Creation pour renvoyer une instance du meme type dynamique
	// Doit etre reimplemente dans les sous-classes
	// La reimplementation typique est:
	//      KWDerivationRule* KWSpecificRule::Create() const
	//      {
	//          return new KWSpecificRule;
	//      }
	virtual KWDerivationRule* Create() const;

	// Recopie des attributs de definition de la regle
	// Peut eventuellement etre redefini si necessaire
	virtual void CopyFrom(const KWDerivationRule* kwdrSource);

	/////////////////////////////////////////
	// Services divers

	// Verification de l'integrite de la regle (sa definition)
	boolean Check() const override;

	// Methode de comparaison entre deux regles, en remplacant les attributs
	// references par leur eventuelle formule
	// Prerequis: les regles doivent etre compilee ou avec des informations de type completes
	virtual int FullCompare(const KWDerivationRule* rule) const;

	// Acces a la fraicheur d'edition de la regle (sans tenir compte de ses operandes)
	int GetFreshness() const;

	// Memoire utilisee par la regle de derivation
	longint GetUsedMemory() const override;

	// Cle de hashage de la regle et des ses operandes
	longint ComputeHashValue() const;

	// Affichage, ecriture dans un fichier, de la definition formelle d'une regle ou de son usage
	virtual void Write(ostream& ost) const override;
	virtual void WriteUsedRule(ostream& ost) const;

	// Methode d'affichage pour la mise au point de regles
	// Affichage d'un nom de regle et d'un objet (avec identifiant multi-tables),
	// ou avec indentation de la composition en sous objets ou sous objet array
	void WriteObject(const KWObject* kwoObject, ostream& ost) const;
	void WriteUsedObject(const KWObject* kwoSubObject, ostream& ost) const;
	void WriteUsedObjectArray(const ObjectArray* oaSubObjectArray, ostream& ost) const;

	// Ecriture dans une string sous forme d'une chaine de caractere, de la definition formelle d'une regle ou de
	// son usage
	void WriteToString(ALString& sOutputString) const;
	void WriteUsedRuleToString(ALString& sOutputString) const;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Methode de test
	static void Test();

	///////////////////////////////////////////////
	// Administration des objets KWDerivationRule

	// Enregistrement dans la base de regles
	// Il ne doit pas y avoir deux regles enregistrees avec le meme nom
	// Memoire: les regles enregistrees sont gerees par l'appele
	static void RegisterDerivationRule(KWDerivationRule* kwdrRule);

	// Recherche par cle
	// Retourne NULL si absent
	static KWDerivationRule* LookupDerivationRule(const ALString& sName);

	// Recherche par cle et duplication
	// Permet d'obtenir une regle prete a etre instanciee a partir d'une
	// regle generique
	// Retourne NULL si absent
	static KWDerivationRule* CloneDerivationRule(const ALString& sName);

	// Export de toutes les regles enregistrees
	// Memoire: le contenu du tableau en retour appartient a l'appele
	static void ExportAllDerivationRules(ObjectArray* oaRules);

	// Destruction de toutes les regles enregistrees
	static void DeleteAllDerivationRules();

	///////////////////////////////////////////////////////////////////////////
	///// Implementation

	// Indique si une regle est de type KWStructureRule (pour l'optimisation du parser)
	virtual boolean IsStructureRule() const;

	// Nom de la regle utilisee pour geree les references a des objets
	static const ALString GetReferenceRuleName();

	// Completion eventuelle de la regle avec les informations de type
	// en maintenant un dictionnaire d'attributs pour eviter les boucles
	virtual void InternalCompleteTypeInfo(const KWClass* kwcOwnerClass, NumericKeyDictionary* nkdAttributes);

	// Methode a destination de KWDerivationRuleOperand::Compile
	void AddMainScopeSecondaryOperands(const KWClass* kwcOwnerClass, KWDerivationRuleOperand* operand);

protected:
	///////////////////////////////////////////////////////////////////////////
	// Service de gestion des regles a scope multiple
	//
	// Pour controler l'integrite d'une regle a scope multiple, il faut
	// empiler ou depiler des (Class, Rule) dans le scope.
	// Le premier parametre est une classe permettant d'acceder au domaine,
	// qui memorise la pile des scopes pour l'ensemble des classes du domaine.
	// On peut alors avoir acces a une classe a un niveau de scope quelconque.

	// Ajout d'un nouveau niveau de scope
	void PushScope(const KWClass* kwcOwnerClass, const KWClass* kwcScopeClass, const KWDerivationRule* rule) const;

	// Supression du dernier niveau de scope rajoute, qui devient le nouveau scope courant
	const KWClass* PopScope(const KWClass* kwcOwnerClass) const;

	// Profondeur du scope (0 si seul le scope courant est disponible)
	int GetScopeDepth(const KWClass* kwcOwnerClass) const;

	// Acces a une classe ou regle a un scope superieur
	//  entre 1 et la profondeur de scope (inclue)
	KWClass* GetClassAtScope(const KWClass* kwcOwnerClass, int nIndex) const;
	KWDerivationRule* GetRuleAtScope(const KWClass* kwcOwnerClass, int nIndex) const;

	// Affichage du scope empile
	void WriteScope(const KWClass* kwcOwnerClass, ostream& ost) const;

	// Verification du type du premier operande d'une regle a scope multiple
	// La redefinition de cette methode permet de specifier des regles
	// a scope multiple dont lme premier operande n'est pas de type Relation
	// (par exemple une regle Structure ayant acces acces a un type de table)
	virtual boolean CheckFirstMultiScopeOperand() const;

	// Recherche de la classe de scope secondaire en cas de scope multiple
	// Cela correspond a la classe du premier operande de la regle
	// Peut etre redefinie comme pour la methode precedente
	// Renvoie NULL si la regle n'est pas de scope multiple ou si
	// l'on a pas trouve cette classe
	virtual KWClass* LookupSecondaryScopeClass(const KWClass* kwcOwnerClass) const;

	// Une fois compilee, une regle a scope multiple memorise dans un tableau
	// tous les operandes secondaire ou des sous-regles a evaluer au niveau
	// du scope principal (en raison de leur ScopeLevel positif).
	// Lors du calcul du resultat de la regle de derivation, ces operandes
	// doivent etre evalues prealablement a l'evaluation de la regle elle-meme.
	// Le resultat d'evaluation de ces operandes, quel que soit leur origine
	// (Attribute, Rule...) doit etre stocke dans la valeur constante de
	// l'operande, et sera traite comme tel dans les operandes secondaires.
	// En fin d'evaluation, il faut reinitialiser ces operandes secondaires.
	// En resume, on doit avoir
	//  <Type> Compute<Type>Result(const KWObject* kwoObject) const
	//  {
	//    // Evaluation des operandes secondaire de scope principal
	//    EvaluateMainScopeSecondaryOperands();
	//
	//    // Calcul du resultat de la regle en cours
	//    ...
	//
	//    // Nettoyage des operandes secondaire de scope principal
	//    CleanMainScopeSecondaryOperands();
	//  }

	// Evaluation des operandes secondaires de scope principal
	void EvaluateMainScopeSecondaryOperands(const KWObject* kwoObject) const;

	// Nettoyage des operandes secondaires de scope principal
	void CleanMainScopeSecondaryOperands() const;

	///////////////////////////////////////////////////////////////////////////
	// Variable de la classe

	// Attributs de base de la regle de derivation
	KWCDUniqueString usName;
	KWCDUniqueString usLabel;
	KWCDUniqueString usClassName;

	// Complement de type (StructureName ou ObjectClassName)
	KWCDUniqueString usSupplementTypeName;

	// Tableau des operandes secondaires de scope principal pour les regles a scope multiples
	// On utilise un pointeur sur le tableau, d'une part pour economiser de la memoire pour les
	// regles n'ayant pas d'operande a scope multiples, d'autre part comme indicateur rapide
	// de presence des operandes a scope multiple lors des calculs une fois compiles
	// Memoire: ces operandes sont des references aux operandes concernes
	ObjectArray* oaMainScopeSecondaryOperands;

	// Classe utilisee pour la compilation
	const KWClass* kwcClass;

	// Operandes
	ObjectArray oaOperands;

	// Gestion de la fraicheur
	int nFreshness;
	int nClassFreshness;
	int nCompileFreshness;

	// Declaration des variables boolean et char en fin de classe pour gagner de la place

	// Type de la valeur retour
	// (en char pour gagner un peu de place dans cette classe potentiellement tres utilisee)
	char cType;

	// Indicateur de regle a nombre variable d'operandes
	boolean bVariableOperandNumber;

	// Gestion des regles a scope multiple
	boolean bMultipleScope;

	// Administration des objets regles
	static ObjectDictionary* odDerivationRules;
};

// Methode de comparaison entre deux regles (cf KWDerivationRule::FullCompare)
int KWDerivationRuleFullCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////
// Classe KWDerivationRuleOperand
// Permet de definir les caracteristiques d'un operande d'une regle
// de derivation
class KWDerivationRuleOperand : public Object
{
public:
	// Constructeur
	KWDerivationRuleOperand();
	~KWDerivationRuleOperand();

	////////////////////////////////////////////////////////////////
	// Specification du type de l'operande

	// Type de l'operande
	// Par defaut: KWType::Unknown
	// Un changement de type reinitialise la valeur de la constante
	// associee a l'operande
	void SetType(int nValue);
	int GetType() const;

	// Nom de la classe pour les type Object et ObjectArray
	const ALString& GetObjectClassName() const;
	void SetObjectClassName(const ALString& sValue);

	// Nom de la structure pour le type Structure
	const ALString& GetStructureName() const;
	void SetStructureName(const ALString& sValue);

	// Nom generique du complement du type, selon le type (ObjectClassName, StructureName, sinon vide)
	const ALString& GetSupplementTypeName() const;
	void SetSupplementTypeName(const ALString& sValue);

	// Niveau de scope de l'operande (par defaut 0)
	// De facon standard, le scope courant est celui de la classe contenant la regle
	// de derivation.
	// Dans le cas de regle a scope multiple (GetMultipleScope()), le niveau de scope
	// courant est determine par la classe du premier operande de la regle englobante
	// qui doit etre de type Relation).
	// On peut alors remonter d'un niveau (ScopeLevel=1) pour acceder a la classe
	// de regle, et generaliser a plusieurs niveaux (ScopeLevel > 1) si necessaire
	// En externe, dans les dictionnaire, les niveaux de scope au dela du scope courant
	// se traduisent par le prefixage des operandes par '.' (ScopeLevel=1),
	// '..' (ScopeLevel=2), ...
	// Seuls les oprandes non constants peuvent avoir un niveau de scope au dela du niveau courant
	int GetScopeLevel() const;
	void SetScopeLevel(int nValue);

	// Type de cle utilisee pour l'operande, dans le cas d'un type bloc
	// On propage le type de l'attribut oude la regle de derivation
	int GetVarKeyType() const;

	/////////////////////////////////////////////////////////////////
	// Specification de la provenance de la valeur a utiliser
	// En fonction du type d'origine, des methodes permettent de
	// preciser la facon d'acceder a la valeur de l'operande
	// Ces champs ne sont a renseigner que pour instancier une regle

	// Type de provenance
	enum
	{
		OriginConstant,  // Valeur constante, uniquement pour les type simples
		OriginAttribute, // Valeur d'un attribut ou d'un bloc d'attribut
		OriginRule,      // Valeur retour d'une regle de derivation
		OriginAny        // Provenance quelconque
	};
	void SetOrigin(int nValue);
	int GetOrigin() const;

	// Libelle lie a l'origine, avec utilisation potentielle du type
	// pour distinguer entre attribut et bloc d'attribut
	static ALString OriginToString(int nOrigin, int nType);

	// Valeur constante
	// Attention, l'acces par type doit etre de type simple et compatible avec le type de l'operande,
	// la constante est automatiquement reinitialisee quand le type est modifie
	void SetContinuousConstant(Continuous cValue);
	Continuous GetContinuousConstant() const;
	void SetSymbolConstant(const Symbol& sValue);
	Symbol& GetSymbolConstant() const;

	// Valeur specifiee de facon generique sous forme chaine de caractere dans le cas des types simples
	// Le type pris en compte est celui de l'operande
	void SetStringConstant(const ALString& sValue);
	const ALString GetStringConstant() const;

	// Valeur de la constante pour l'export vers un fichier externe
	// (entre doubles-quotes, les caracteres internes " et \ etant precedes de \)
	const ALString GetExternalStringConstant() const;

	// Nom de l'attribut, dans le cas d'une origine attribut et d'un type IsValue
	void SetAttributeName(const ALString& sName);
	const ALString& GetAttributeName() const;

	// Nom du bloc d'attribut, dans le cas d'une origine attribut et d'un type IsValueBlock
	void SetAttributeBlockName(const ALString& sName);
	const ALString& GetAttributeBlockName() const;

	// Nom generique d'un attribut ou bloc d'attribut dans le cas d'une origine attribut
	// Il y a ici moins de controle d'assertion
	void SetDataItemName(const ALString& sName);
	const ALString& GetDataItemName() const;

	// Libelle associe au type de data item, selon le type Value u ValueBlock de l'operande
	const ALString GetDataItemLabel() const;

	// Regle de derivation associee
	// Memoire: la regle de derivation appartient a l'appele
	void SetDerivationRule(KWDerivationRule* kwdrValue);
	KWDerivationRule* GetDerivationRule() const;

	//////////////////////////////////////////////////////////////////////////
	// Methodes de verification

	// Verification qu'une regle est suffisament definie pour etre generalisee
	boolean CheckDefinition() const;

	// Verification qu'une regle est une specialisation d'une regle plus generale
	// suffisament definie
	boolean CheckFamily(const KWDerivationRuleOperand* operandFamily) const;

	// Verification qu'une regle est completement renseignee et compilable
	boolean CheckCompleteness(const KWClass* kwcOwnerClass) const;

	/////////////////////////////////////////////////////////
	// Compilation de l'operande et acces a la valeur

	// Completion eventuelle de l'operande avec les informations de type
	void CompleteTypeInfo(const KWClass* kwcOwnerClass);

	// Recherche d'une regle utilise directement ou indirectement (via un attribut)
	// Renvoie NULL si pas de regle
	KWDerivationRule* GetReferencedDerivationRule(const KWClass* kwcOwnerClass) const;

	// Compilation par rapport a une classe
	// La compilation se propage aux sous-regles
	void Compile(KWClass* kwcOwnerClass);

	// Acces a l'attribut dans le cas d'une origine attribut
	// L'attribut doit etre compile ou avec des informations de type completes
	// Retourne NULL si l'origine n'est pas attribut
	KWAttribute* GetOriginAttribute();

	// Acces au bloc d'attributs dans le cas d'une origine bloc d'attribut
	// Le bloc d'attributs doit etre compile ou avec des informations de type completes
	// Retourne NULL si l'origine n'est pas bloc d'attribut
	KWAttributeBlock* GetOriginAttributeBlock();

	// Acces a la valeur, par type
	Continuous GetContinuousValue(const KWObject* kwoObject) const;
	Symbol GetSymbolValue(const KWObject* kwoObject) const;
	Date GetDateValue(const KWObject* kwoObject) const;
	Time GetTimeValue(const KWObject* kwoObject) const;
	Timestamp GetTimestampValue(const KWObject* kwoObject) const;
	TimestampTZ GetTimestampTZValue(const KWObject* kwoObject) const;
	Symbol GetTextValue(const KWObject* kwoObject) const;
	SymbolVector* GetTextListValue(const KWObject* kwoObject) const;
	KWObject* GetObjectValue(const KWObject* kwoObject) const;
	ObjectArray* GetObjectArrayValue(const KWObject* kwoObject) const;
	Object* GetStructureValue(const KWObject* kwoObject) const;

	// Acces au bloc de valeurs
	KWContinuousValueBlock* GetContinuousValueBlock(const KWObject* kwoObject) const;
	KWSymbolValueBlock* GetSymbolValueBlock(const KWObject* kwoObject) const;
	KWObjectArrayValueBlock* GetObjectArrayValueBlock(const KWObject* kwoObject) const;

	// Renommage de l'attribut dans le cas d'un operande attribut
	void RenameAttribute(const KWClass* kwcOwnerClass, KWAttribute* refAttribute,
			     const ALString& sNewAttributeName);

	// Calcul du nom de l'operande en fonction de son origine
	const ALString ComputeOperandName() const;

	/////////////////////////////////////////
	// Services divers

	// Duplication
	KWDerivationRuleOperand* Clone() const;

	// Verification de l'integrite de l'operande
	boolean Check() const override;

	// Memoire utilisee par l'operande
	longint GetUsedMemory() const override;

	// Cle de hashage de l'operande et de son eventuelle regle de derivation
	longint ComputeHashValue() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;
	void WriteUsedOperand(ostream& ost) const;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	//////////////////////////////////////////////////////////////////////////
	//// Implementation

	// Completion eventuelle de la regle avec les informations de type
	// en maintenant un dictionnaire d'attributs pour eviter les boucles
	void InternalCompleteTypeInfo(const KWClass* kwcOwnerClass, NumericKeyDictionary* nkdCompletedAttributes);

	// Calcul de la valeur d'un operande accedant a un scope de niveau superieur et memorisation dans la valeur
	// constant En effet, seuls les operande d'origine Attribute ou Rule peuvent etre de scope superieur; on peut
	// alors utiliser la valeur pour memoriser le resultat du calcul au debut de la regle de derivation au bon
	// niveau de scope. Ce calcul est alors effectue uen fois pour toute au debut du calcul de valeur de la regle,
	// puis ces valeurs sont accedees par leur valeur constante par les regle de scope secondaire Les methodes
	// suivantes precalculent les operandes de scope superieur (attribut ou regle) et les stockent sous forme de
	// constante
	//  void KWDerivationRule::EvaluateMainScopeSecondaryOperands(const KWObject* kwoObject);
	//  void KWDerivationRule::CleanMainScopeSecondaryOperands();
	void ComputeUpperScopeValue(const KWObject* kwoObject);
	void InitUpperScopeValue();

protected:
	/////////////////////////////////////////////////////////////////////////
	// Services de gestion du scope (sous-partie de ceux de KWDerivationRule)

	// Profondeur du scope (0 si seul le scope courant est disponible)
	int GetScopeDepth(const KWClass* kwcOwnerClass) const;

	// Acces a une classe ou regle a un scope superieur
	//  entre 1 et la profondeur de scope (inclue)
	KWClass* GetClassAtScope(const KWClass* kwcOwnerClass, int nIndex) const;
	KWDerivationRule* GetRuleAtScope(const KWClass* kwcOwnerClass, int nIndex) const;

	// Type de provenance une fois compile
	// Les operandes du scope courant proviennent d'un attribut, d'une regle, ou d'une constante
	// Les operandes de scope supperieur sont precalcules au niveau de leur regle du bon scope (cf.
	// ComputeUpperScopeValue) et stockes dans la valeur constante L'acces au bon type de calcul est ainsi optimise
	// avec les CompiledOrigin
	enum
	{
		CompiledOriginAttribute, // Valeur d'un attribut dans le scope courant
		CompiledOriginRule,      // Valeur retour d'une regle de derivation dans le scope courant
		CompiledOriginConstant   // Valeur constante
	};

	// Complement de type (StructureName ou ObjectClassName)
	KWCDUniqueString usSupplementTypeName;

	// Origine de la valeur
	KWCDUniqueString usDataItemName;
	KWValue kwvConstant;
	KWDerivationRule* rule;

	// Origine compilee de la valeur
	const KWClass* kwcClass;

	// Index de l'attribut ou du bloc dans le cas d'une origine attribut
	KWLoadIndex liDataItemLoadIndex;

	// Specifications optimisee sous forme de char (consecutifs) plutot que int, pour gagner de la place en memoire)
	char cType;           // Type de l'operande
	char cScopeLevel;     // Niveau de scope
	char cOrigin;         // Origine de l'operande
	char cCompiledOrigin; // Origine de l'operande apres compilation

	// Informations disponibles uniquement en mode debug, pour le controle
	// de la coherence de l'acces aux operandes uniquement en mode compile
	debug(boolean IsCompiled() const);
	debug(int GetFreshness() const);
	debug(int nFreshness);
	debug(int nClassFreshness);
	debug(int nCompileFreshness);
};

#include "KWClass.h"
#include "KWObject.h"
#include "KWDatabase.h"
#include "KWObjectKey.h"

/////////////////////////////////////////////////////////////////
// Implementation inline

inline void KWDerivationRule::SetName(const ALString& sValue)
{
	require(KWClass::CheckName(sValue, this));
	usName.SetValue(sValue);
	nFreshness++;
}

inline const ALString& KWDerivationRule::GetName() const
{
	return usName.GetValue();
}

inline void KWDerivationRule::SetLabel(const ALString& sValue)
{
	require(KWClass::CheckLabel(sValue, this));
	usLabel.SetValue(sValue);
}

inline const ALString& KWDerivationRule::GetLabel() const
{
	return usLabel.GetValue();
}

inline void KWDerivationRule::SetClassName(const ALString& sValue)
{
	require(sValue == "" or KWClass::CheckName(sValue, this));
	usClassName.SetValue(sValue);
	nFreshness++;
}

inline const ALString& KWDerivationRule::GetClassName() const
{
	return usClassName.GetValue();
}

inline void KWDerivationRule::SetType(int nValue)
{
	require(KWType::Check(nValue));
	usSupplementTypeName.SetValue("");
	cType = char(nValue);
	nFreshness++;
}

inline int KWDerivationRule::GetType() const
{
	return cType;
}

inline const ALString& KWDerivationRule::GetObjectClassName() const
{
	require(KWType::IsGeneralRelation(GetType()));
	return usSupplementTypeName.GetValue();
}

inline void KWDerivationRule::SetObjectClassName(const ALString& sValue)
{
	require(KWType::IsGeneralRelation(GetType()));
	require(sValue == "" or KWClass::CheckName(sValue, this));
	usSupplementTypeName.SetValue(sValue);
	nFreshness++;
}

inline boolean KWDerivationRule::GetReference() const
{
	require(KWType::IsGeneralRelation(GetType()));
	return true;
}

inline const ALString& KWDerivationRule::GetStructureName() const
{
	require(GetType() == KWType::Structure);
	return usSupplementTypeName.GetValue();
}

inline void KWDerivationRule::SetStructureName(const ALString& sValue)
{
	require(GetType() == KWType::Structure);
	require(sValue == "" or KWClass::CheckName(sValue, this));
	usSupplementTypeName.SetValue(sValue);
	nFreshness++;
}

inline const ALString& KWDerivationRule::GetSupplementTypeName() const
{
	ensure(usSupplementTypeName.GetValue() == "" or KWType::IsGeneralRelation(GetType()) or
	       GetType() == KWType::Structure);
	return usSupplementTypeName.GetValue();
}

inline void KWDerivationRule::SetSupplementTypeName(const ALString& sValue)
{
	require(sValue == "" or KWType::IsGeneralRelation(GetType()) or GetType() == KWType::Structure);
	usSupplementTypeName.SetValue("");
}

inline int KWDerivationRule::GetOperandNumber() const
{
	return oaOperands.GetSize();
}

inline void KWDerivationRule::SetVariableOperandNumber(boolean bValue)
{
	bVariableOperandNumber = bValue;
}

inline boolean KWDerivationRule::GetVariableOperandNumber() const
{
	return bVariableOperandNumber;
}

inline KWDerivationRuleOperand* KWDerivationRule::GetOperandAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < oaOperands.GetSize());
	return cast(KWDerivationRuleOperand*, oaOperands.GetAt(nIndex));
}

inline KWDerivationRuleOperand* KWDerivationRule::GetFirstOperand() const
{
	require(oaOperands.GetSize() >= 1);
	return cast(KWDerivationRuleOperand*, oaOperands.GetAt(0));
}

inline KWDerivationRuleOperand* KWDerivationRule::GetSecondOperand() const
{
	require(oaOperands.GetSize() >= 2);
	return cast(KWDerivationRuleOperand*, oaOperands.GetAt(1));
}

inline boolean KWDerivationRule::GetMultipleScope() const
{
	return bMultipleScope;
}

inline void KWDerivationRule::SetMultipleScope(boolean bValue)
{
	bMultipleScope = bValue;
}

inline boolean KWDerivationRule::IsCompiled() const
{
	return nCompileFreshness == nFreshness and kwcClass != NULL and kwcClass->GetFreshness() == nClassFreshness;
}

inline const KWClass* KWDerivationRule::GetOwnerClass() const
{
	require(kwcClass != NULL);
	return kwcClass;
}

inline int KWDerivationRule::GetFreshness() const
{
	return nFreshness;
}

inline const ALString KWDerivationRule::GetReferenceRuleName()
{
	return "Reference";
}

inline void KWDerivationRule::AddMainScopeSecondaryOperands(const KWClass* kwcOwnerClass,
							    KWDerivationRuleOperand* operand)
{
	require(operand->GetScopeLevel() > 0);
	require(operand->GetScopeLevel() <= GetScopeDepth(kwcOwnerClass));
	require(GetRuleAtScope(kwcOwnerClass, operand->GetScopeLevel()) == this);
	require(oaMainScopeSecondaryOperands != NULL);
	oaMainScopeSecondaryOperands->Add(operand);
}

inline void KWDerivationRule::EvaluateMainScopeSecondaryOperands(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* operand;
	int i;

	require(IsCompiled());
	require(GetMultipleScope());

	// Evaluation des operandes secondaires de scope principal
	if (oaMainScopeSecondaryOperands != NULL)
	{
		for (i = oaMainScopeSecondaryOperands->GetSize() - 1; i >= 0; i--)
		{
			operand = cast(KWDerivationRuleOperand*, oaMainScopeSecondaryOperands->GetAt(i));

			// Calcul de la valeur de l'operande et memorisation dans la constante associee a l'operande
			operand->ComputeUpperScopeValue(kwoObject);
		}
	}
}

inline void KWDerivationRule::CleanMainScopeSecondaryOperands() const
{
	KWDerivationRuleOperand* operand;
	int i;

	require(IsCompiled());
	require(GetMultipleScope());

	// Evaluation des operandes secondaires de scope principal
	if (oaMainScopeSecondaryOperands != NULL)
	{
		for (i = oaMainScopeSecondaryOperands->GetSize() - 1; i >= 0; i--)
		{
			operand = cast(KWDerivationRuleOperand*, oaMainScopeSecondaryOperands->GetAt(i));

			// Reinitialisation de la constante associee a l'operande
			operand->InitUpperScopeValue();
		}
	}
}

////////////////////////////////////////////////////////////////

inline void KWDerivationRuleOperand::SetType(int nValue)
{
	require(nValue == KWType::Unknown or KWType::Check(nValue));

	// Mise a jour de la constante par sa valeur par defaut
	if (cType != nValue)
	{
		// Cas specifique pour les Symbol, qui sont geres automatiquement
		// Il faut remettre au prealable la valeur par defaut des Symbol
		// pour forcer l'eventuelle liberation du Symbol precedent
		if (cType == KWType::Symbol)
			kwvConstant.ResetSymbol();
		kwvConstant.Init();
	}
	usSupplementTypeName.SetValue("");
	cType = (char)nValue;
	debug(nFreshness++;)
}

inline int KWDerivationRuleOperand::GetType() const
{
	return cType;
}

inline const ALString& KWDerivationRuleOperand::GetObjectClassName() const
{
	require(KWType::IsGeneralRelation(GetType()));
	return usSupplementTypeName.GetValue();
}

inline void KWDerivationRuleOperand::SetObjectClassName(const ALString& sValue)
{
	require(KWType::IsGeneralRelation(GetType()));
	require(sValue == "" or KWClass::CheckName(sValue, this));
	usSupplementTypeName.SetValue(sValue);
	debug(nFreshness++;)
}

inline const ALString& KWDerivationRuleOperand::GetStructureName() const
{
	require(GetType() == KWType::Structure);
	return usSupplementTypeName.GetValue();
}

inline void KWDerivationRuleOperand::SetStructureName(const ALString& sValue)
{
	require(GetType() == KWType::Structure);
	require(sValue == "" or KWClass::CheckName(sValue, this));
	usSupplementTypeName.SetValue(sValue);
	debug(nFreshness++;)
}

inline const ALString& KWDerivationRuleOperand::GetSupplementTypeName() const
{
	ensure(usSupplementTypeName.GetValue() == "" or KWType::IsGeneralRelation(GetType()) or
	       GetType() == KWType::Structure);
	return usSupplementTypeName.GetValue();
}

inline void KWDerivationRuleOperand::SetSupplementTypeName(const ALString& sValue)
{
	require(sValue == "" or KWType::IsGeneralRelation(GetType()) or GetType() == KWType::Structure);
	usSupplementTypeName.SetValue(sValue);
}

inline int KWDerivationRuleOperand::GetScopeLevel() const
{
	return cScopeLevel;
}

inline void KWDerivationRuleOperand::SetScopeLevel(int nValue)
{
	require(nValue >= 0);
	cScopeLevel = (char)nValue;
	debug(nFreshness++;)
}

inline void KWDerivationRuleOperand::SetOrigin(int nValue)
{
	require(nValue == OriginConstant or nValue == OriginAttribute or nValue == OriginRule or nValue == OriginAny);
	cOrigin = (char)nValue;
	debug(nFreshness++;)
}

inline int KWDerivationRuleOperand::GetOrigin() const
{
	return cOrigin;
}

inline void KWDerivationRuleOperand::SetContinuousConstant(Continuous cValue)
{
	require(GetType() == KWType::Continuous);
	kwvConstant.SetContinuous(cValue);
	debug(nFreshness++;)
}

inline Continuous KWDerivationRuleOperand::GetContinuousConstant() const
{
	require(GetType() == KWType::Continuous);
	return kwvConstant.GetContinuous();
}

inline void KWDerivationRuleOperand::SetSymbolConstant(const Symbol& sValue)
{
	require(GetType() == KWType::Symbol);
	kwvConstant.SetSymbol(sValue);
	debug(nFreshness++;)
}

inline Symbol& KWDerivationRuleOperand::GetSymbolConstant() const
{
	require(GetType() == KWType::Symbol);
	return kwvConstant.GetSymbol();
}

inline void KWDerivationRuleOperand::SetStringConstant(const ALString& sValue)
{
	assert(KWType::IsSimple(GetType()));
	if (cType == KWType::Symbol)
		kwvConstant.SetSymbol(Symbol(sValue));
	else
		kwvConstant.SetContinuous(KWContinuous::StringToContinuous(sValue));
	debug(nFreshness++;)
}

inline const ALString KWDerivationRuleOperand::GetStringConstant() const
{
	assert(KWType::IsSimple(GetType()));
	if (cType == KWType::Symbol)
		return kwvConstant.GetSymbol().GetValue();
	else
		return KWContinuous::ContinuousToString(kwvConstant.GetContinuous());
}

inline void KWDerivationRuleOperand::SetAttributeName(const ALString& sName)
{
	require(sName == "" or KWClass::CheckName(sName, this));
	usDataItemName.SetValue(sName);
	debug(nFreshness++;)
}

inline const ALString& KWDerivationRuleOperand::GetAttributeName() const
{
	require(KWType::IsValue(GetType()));
	return usDataItemName.GetValue();
}

inline void KWDerivationRuleOperand::SetAttributeBlockName(const ALString& sName)
{
	require(sName == "" or KWClass::CheckName(sName, this));
	usDataItemName.SetValue(sName);
	debug(nFreshness++;)
}

inline const ALString& KWDerivationRuleOperand::GetAttributeBlockName() const
{
	require(KWType::IsValueBlock(GetType()));
	return usDataItemName.GetValue();
}

inline void KWDerivationRuleOperand::SetDataItemName(const ALString& sName)
{
	require(sName == "" or KWClass::CheckName(sName, this));
	usDataItemName.SetValue(sName);
	debug(nFreshness++;)
}

inline const ALString& KWDerivationRuleOperand::GetDataItemName() const
{
	return usDataItemName.GetValue();
}

inline const ALString KWDerivationRuleOperand::GetDataItemLabel() const
{
	if (KWType::IsValueBlock(GetType()))
		return "Sparse variable block";
	else
		return "Variable";
}

inline void KWDerivationRuleOperand::SetDerivationRule(KWDerivationRule* kwdrValue)
{
	rule = kwdrValue;
	debug(nFreshness++;)
}

inline KWDerivationRule* KWDerivationRuleOperand::GetDerivationRule() const
{
	return rule;
}

inline Continuous KWDerivationRuleOperand::GetContinuousValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::Continuous);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeContinuousValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeContinuousResult(kwoObject)
							     : GetContinuousConstant()));
}

inline Symbol KWDerivationRuleOperand::GetSymbolValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::Symbol);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeSymbolValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeSymbolResult(kwoObject)
							     : GetSymbolConstant()));
}

inline Date KWDerivationRuleOperand::GetDateValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::Date);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeDateValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeDateResult(kwoObject)
							     : kwvConstant.GetDate()));
}

inline Time KWDerivationRuleOperand::GetTimeValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::Time);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeTimeValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeTimeResult(kwoObject)
							     : kwvConstant.GetTime()));
}

inline Timestamp KWDerivationRuleOperand::GetTimestampValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::Timestamp);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeTimestampValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeTimestampResult(kwoObject)
							     : kwvConstant.GetTimestamp()));
}

inline TimestampTZ KWDerivationRuleOperand::GetTimestampTZValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::TimestampTZ);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeTimestampTZValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeTimestampTZResult(kwoObject)
							     : kwvConstant.GetTimestampTZ()));
}

inline Symbol KWDerivationRuleOperand::GetTextValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::Text);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeTextValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeTextResult(kwoObject)
							     : kwvConstant.GetText()));
}

inline SymbolVector* KWDerivationRuleOperand::GetTextListValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::TextList);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeTextListValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeTextListResult(kwoObject)
							     : kwvConstant.GetTextList()));
}

inline KWObject* KWDerivationRuleOperand::GetObjectValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::Object);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeObjectValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeObjectResult(kwoObject)
							     : kwvConstant.GetObject()));
}

inline ObjectArray* KWDerivationRuleOperand::GetObjectArrayValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::ObjectArray);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeObjectArrayValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeObjectArrayResult(kwoObject)
							     : kwvConstant.GetObjectArray()));
}

inline Object* KWDerivationRuleOperand::GetStructureValue(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::Structure);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeStructureValueAt(liDataItemLoadIndex)
		    : (cCompiledOrigin == CompiledOriginRule ? GetDerivationRule()->ComputeStructureResult(kwoObject)
							     : kwvConstant.GetStructure()));
}

inline KWContinuousValueBlock* KWDerivationRuleOperand::GetContinuousValueBlock(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::ContinuousValueBlock);
	require(cCompiledOrigin != CompiledOriginRule);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeContinuousValueBlockAt(liDataItemLoadIndex)
		    : kwvConstant.GetContinuousValueBlock());
}

inline KWSymbolValueBlock* KWDerivationRuleOperand::GetSymbolValueBlock(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::SymbolValueBlock);
	require(cCompiledOrigin != CompiledOriginRule);

	return (cCompiledOrigin == CompiledOriginAttribute ? kwoObject->ComputeSymbolValueBlockAt(liDataItemLoadIndex)
							   : kwvConstant.GetSymbolValueBlock());
}

inline KWObjectArrayValueBlock* KWDerivationRuleOperand::GetObjectArrayValueBlock(const KWObject* kwoObject) const
{
	debug(require(IsCompiled());) require(kwoObject != NULL);
	require(kwoObject->GetClass() == kwcClass or GetScopeLevel() > 0);
	require(GetType() == KWType::ObjectArrayValueBlock);
	require(cCompiledOrigin != CompiledOriginRule);

	return (cCompiledOrigin == CompiledOriginAttribute
		    ? kwoObject->ComputeObjectArrayValueBlockAt(liDataItemLoadIndex)
		    : kwvConstant.GetObjectArrayValueBlock());
}

debug(inline int KWDerivationRuleOperand::GetFreshness() const { return nFreshness; })

    debug(inline boolean KWDerivationRuleOperand::IsCompiled() const {
	    return nCompileFreshness == nFreshness and kwcClass != NULL and kwcClass->GetFreshness() == nClassFreshness;
    })

	inline KWAttribute* KWDerivationRuleOperand::GetOriginAttribute()
{
	require(kwcClass != NULL);
	require(KWType::IsValue(GetType()));
	debug(require(IsCompiled() or CheckCompleteness(kwcClass)));

	if (cOrigin == OriginAttribute)
		return kwcClass->LookupAttribute(GetAttributeName());
	else
		return NULL;
}

inline KWAttributeBlock* KWDerivationRuleOperand::GetOriginAttributeBlock()
{
	require(kwcClass != NULL);
	require(KWType::IsValueBlock(GetType()));
	debug(require(IsCompiled() or CheckCompleteness(kwcClass)));

	if (cOrigin == OriginAttribute)
		return kwcClass->LookupAttributeBlock(GetAttributeBlockName());
	else
		return NULL;
}