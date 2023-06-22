// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDDomainKnowledge;
class KDSelectionSpec;

#include "KWClass.h"
#include "KDConstructionRule.h"
#include "KDEntity.h"
#include "KDClassCompliantRules.h"
#include "KWDRAll.h"
#include "KWLearningReport.h"
#include "KWSortableIndex.h"
#include "SortedList.h"
#include "Timer.h"
#include "KDMultinomialSampleGenerator.h"
#include "KDSelectionOperandAnalyser.h"
#include "KDConstructionDomain.h"
#include "KDClassBuilder.h"
#include "KWDRPreprocessing.h"
#include "KDConstructedRule.h"

//////////////////////////////////////////////////////////////////////////////
// Classe KDDomainKnowledge
// Description des connaissances sur un domaine pour piloter la construction
// de variables.
class KDDomainKnowledge : public KWLearningReport
{
public:
	// Constructeur et destructeur
	KDDomainKnowledge();
	~KDDomainKnowledge();

	// Parametrage de construction: ensemble des regles de construction utilisables
	// Attention, parametrage obligatoire (initialement a NULL)
	// Memoire: appartient a l'appelant
	void SetConstructionDomain(KDConstructionDomain* constructionDomainParam);
	KDConstructionDomain* GetConstructionDomain() const;

	//////////////////////////////////////////////////////////////////////
	// Service d'enrichissement d'un classe par construction de variable
	// Une nouvelle classe est construite, ayant son propre domaine

	// Creation d'une nouvelle classe principale par construction de variables
	// Le nombre de regles a construire (au plus) est passe en parametre
	// Renvoie true s'il n'y a eu ni erreur ni interruption utilisateur,
	// qu'une classe ait etre construite ou non
	// Une nouvelle classe est cree uniquement si des variable ont pu etre construites
	boolean ComputeStats() override;

	// Nettoyage des specifications de construction de variable, avec ou sans destruction
	// de la classe de preparation et de son domaine
	void RemoveConstructedClass();
	void DeleteConstructedClass();

	// Test si une classe a ete construite
	boolean IsClassConstructed() const;

	// Classe construite et son domaine
	// Les cout de construction sont memorise par variable
	// Memoire: la classe rendue (et son domaine) appartient a l'appele et est enregistre dans son propre domaine;
	//  il sera libere par l'appele, sauf en cas d'appel a RemoveConstructedClass qui permet a l'appelant
	//  d'en prendre possession
	KWClass* GetConstructedClass();
	KWClassDomain* GetConstructedDomain();

	// Temps de construction des variables
	double GetConstructionTime() const;

	// Calcul des couts de construction pour les attributs initiaux d'une classe
	// Methode avancee appelable sur une classe quelconque, ayant meme attribut cible que dans le learningSpec
	// Tous les attributs initiaux ont un cout homogene
	void ComputeInitialAttributeCosts(KWClass* kwcValue) const;

	// Import des couts de construction par attribut a a partir des meta-donnes (Cost) des attributs
	// Methode avancee appelable sur une classe quelconque
	// Renvoie true si ensemble des cout coherents, et importe effectivement les couts
	// Sinon, renvoie false avec des messages d'erreur
	boolean ImportAttributeMetaDataCosts(KWClass* kwcValue);

	//////////////////////////////////////////////////////////////////////////
	// Exploitation des regles de construction pour calculer des indicateurs
	// sur les regles de derivations construites

	// Nombre de regles de construction utilisees
	int RuleComputeUsedConstructionRuleNumber(const KWDerivationRule* constructedRule) const;

	// Nombre d'operandes utilises
	int RuleComputeUsedOperandNumber(const KWDerivationRule* constructedRule) const;

	// Profondeur de l'arbre de calcul
	int RuleComputeOperandTreeDepth(const KWDerivationRule* constructedRule) const;

	///////////////////////////////////////////////////////////////////////
	// Contraintes pour controler l'exploration de l'arbre des regles de construction
	// Ces contraintes sont imperatives et prise en compte le plus tot possible.
	// Leurs valeurs par defaut sont tres grandes (pas de contrainte), mais elles
	// permettent d'agir comme garde-fou

	// Nombre maximal de regles construites: defaut: 1000000
	int GetMaxRuleNumber() const;
	void SetMaxRuleNumber(int nValue);

	// Profondeur maximale des regles construites: defaut: 100
	int GetMaxRuleDepth() const;
	void SetMaxRuleDepth(int nValue);

	// Cout maximal des regles construites: defaut: 1000
	double GetMaxRuleCost() const;
	void SetMaxRuleCost(double dValue);

	//////////////////////////////////////////////////////////////////////
	// Services generiques

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////
	// Structure de l'ensemble des classes assurant la construction de variable
	//
	// KDDomainKnowledge
	// Classe de pilotage principale, dont la methode ComputeStats declenche
	// la construction de variable pour produire une classe (KWClass) construite
	//
	// KDConstructionDomain
	// Specification globale des regles de construction a utiliser
	//
	// KDClassDomainCompliantRules et KDClassCompliantRules
	// Gestion des regles effectivement applicables par classe (KWClass)
	// Et gestion des collision entre regles construite et attributs derives
	// presents initialement
	//
	// KDSelectionOperandAnalyser
	// Gestion specifique des operande de selection
	//
	// KDClassBuilder
	// Construction effectivement de la classe (KWClass) a construire a partir
	// d'un ensemble de regles construites
	// Gestion fine du nommage des variables construites
	//
	// KDConstructionRule: regle de construction
	// KDConstructedRule: regle construite a partir d'une regle de construction
	// KWDerivationRule: regle de derivation issue d'une regle construite, pour
	//  produire un attribut construit

	//////////////////////////////////////////////////////////////////////////////
	// Gestion des collisions entre variables construites et attributs derives
	// presents initialement
	//
	// Cette gestion est assuree de facon optimisee par les classes
	// KDClassDomainCompliantRules et KDClassCompliantRules
	// Ces classes sont initialisees par la methode
	//  ComputeAllClassesCompliantRules(KWClass*, KDClassDomainCompliantRules*)
	// L'initialisation doit etre refaite dans le KDSparseBuilder, pour s'assurer
	// que les attributs derives dont on teste la collision sont dans le meme
	// KWClassDomain que la classe en cours de construction
	//
	// Comportement attendu et implemente pour la gestion de ces collisions
	// . reutilisation systematiquement des attributs derives existants, utilises ou non
	//     . si attribut de la table principale
	//        . si collision avec regle construite : on force la construction de nouveaux attributs,
	//          en plus des attributs existants (utilises ou non)
	// 	. si attribut en operande de calcul ou de selection
	// 	   . si attribut utilise
	// 	      . on ne genere par d'attribut pour la regle de construction correspondante,
	// 	        car l'attribut genere serait redondant par rapport a l'attribut utilise existant
	// 	   . si attribut non utilise
	// 	      . on genere un attribut base sur la regle, avec un nom base sur la regle
	// 	      . mais on reutilise l'attribut derive existant (meme s'il est en unused),
	// 	        de facon similaire a la reutilisation des attribut intermediaire (eux aussi en unused)
	// 	      . autrement dit, on reutilise cet attribut au lieu de generer un attribut intermediaire)
	//
	// La methode principale de gestion des collisions est
	// KDClassCompliantRules::LookupConstructedAttribute, qui permet de savoir
	// s'il existe un attribut derive en collision avec une regle constuite
	// Cette methode est utilisee dans les methodes suivantes
	// . KDClassSelectionStats::SearchAttributeClassSelectionOperandStatsFromRule
	// 	   pour detecter les operandes de selection a ne pas generer
	// . KDDomainKnowledge::FilterConstructedRules
	// 	   pour ne pas generer d'attribut deja existants
	// . KDDomainKnowledge::BuildAllConstructedRulesFromLastOperands
	// 	   pour ne pas generer d'attribut deja existant (intermediaire ou non)
	// . KDClassBuilder::InternalBuildClassFromSelectionRules
	//     pour ne pas generer d'attribut deja existant, dans le cadre d'une operande de selection
	// . KDClassBuilder::CreateOptimizedUsedRuleAttribute
	//     pour ne pas generer d'attribut deja existant, dans le cadre general

	// Creation d'une nouvelle classe principale par construction de variables
	// Renvoie false uniquement si probleme de construction ou si interruption utilisateur
	// (par exemple: analyse des operandes de selection interrompue par utilisateur)
	// La classe construite est NULL si aucune variable n'etait constructible ou si code retour false
	// Memoire: la classe construite ainsi que son domaine appartiennent a l'appele
	boolean ConstructClass(KWClass*& constructedClass);

	// Affichage d'un ensemble de regles construites (KDConstructedRule)
	void DisplayConstructedRuleArray(const ObjectArray* oaConstructedRules, ostream& ost) const;

	///////////////////////////////////////////////////////////////////////////////
	// Construction recursive des regles de derivation
	//
	// Parametres utilises dans les methodes de construction
	// Input
	// - parametres de construction
	//   . constructionRule: regle de construction correspondant a la regle a construire
	//   . classCompliantRules: classe associee a la regle de construction courante, permettant d'acceder
	//                          aux attribute de la classe et a l'ensemble des regles de construction disponibles
	//                          pour cette classe Ces information permettent de parcourir les operandes potentiels
	//                          pour la regle en cours de construction
	//   . secondaryScopeClassCompliantRules: classe associee a l'operande courant de la regle de construction
	//   courante,
	//                                  utile pour les regles a scope multiples
	//   . nStartOperandIndex: index de l'operande en cours de construction
	// - regle en cours de construction
	//   . templateDerivationRule: regle de derivation en cours de construction, selon les parametres de
	//   construction
	// - controle de la profondeur d'extraction
	//   . sPriorTreeNodeName: identifiant du noeud de l'arbre de construction a partir de la racine
	//                         Utilise si non vide, pour indiquer qu'une trace de debugging est demandee
	//   . nDepth: profondeur dans l'arbre de construction, limite par la variable externe nMaxRuleDepth
	//   . dRuleCost: cout de la regle en cours de construction, limite par la variable externe dMaxRuleCost
	//   . nRandomDrawingNumber: nombre de tirages aleatoires demandes
	//                           Les tirages sont equidistribues selon les branche de l'arbre de construction, a
	//                           chaque noeud de l'arbre Le nombre de regles construites sera potentiellement
	//                           inferieurs, car les doublons sont elimines en cours de construction
	// Output
	//   . oaAllConstructedRules: resultat de la construction contenant l'ensemble des regles construites
	//   (KDConstructedRule)
	//                            Le nombre total de regles construites est limite par la variable externe
	//                            nMaxRuleNumber Memoire: a liberer par l'appelant

	// Construction de toutes les regles construites (KDConstructedRule) applicables sur la classe principale
	// Memoire: la table de regles en sortie et son contenu appartiennent a l'appelant (en fait, tout appartient a
	// l'appelant)
	void BuildMainClassRequestedConstructedRules(ObjectArray* oaAllConstructedRules, int nRuleNumber) const;
	void BuildMainClassConstructedRules(ObjectArray* oaAllConstructedRules, double dRandomDrawingNumber) const;

	// Construction de toutes les regles construites (KDConstructedRule) pour une regle de construction applicable
	// sur une classe Le tableau de regles de construites en sortie est initialise avec les regles construites, dont
	// le cout est correctement pris en compte Memoire: le tableau de regles en sortie et son contenu appartiennent
	// a l'appelant (en fait, tout appartient a l'appelant)
	void BuildAllConstructedRules(const KDConstructionRule* constructionRule,
				      const KDClassCompliantRules* classCompliantRules,
				      const ALString& sPriorTreeNodeName, int nDepth, double dRuleCost,
				      double dRandomDrawingNumber, ObjectArray* oaAllConstructedRules) const;

	// Completion des regles construites (KDConstructedRule) en cours de construction (templateConstructedRule) a
	// partir de son ieme operande Le template de regle de derivation est une regle conforme a la regle de
	// construction, correctement initialisee pour ses premier operandes
	void BuildAllConstructedRulesFromLastOperands(
	    const KDConstructionRule* constructionRule, const KDClassCompliantRules* classCompliantRules,
	    const KDClassCompliantRules* secondaryScopeClassCompliantRules, int nStartOperandIndex,
	    KDConstructedRule* templateConstructedRule, const ALString& sPriorTreeNodeName, int nDepth,
	    double dRuleCost, double dRandomDrawingNumber, ObjectArray* oaAllConstructedRules) const;

	// Meilleur echantillon pour une distribution multinomiale equidistribuee pour entre chaque attributs, et
	// l'ensemble des regles. L'ensemble des regles (optionnel) est gere comme un dernier pseudo-attribut, moins
	// prioritaire que les autres
	void DispatchAttributeRandomDrawingNumbers(double dRandomDrawingNumber, int nAttributeNumber, int nRuleNumber,
						   DoubleVector* dvAttributeRandomDrawingNumbers,
						   double& dAllRulesRandomDrawingNumber) const;

	// Calcul des probabilites associee a un ensemble de regles de construction, et reparition des tirages selon ces
	// probabilites Par defaut, elle sont equiprobables Neamoins, dans les cas limites de moins de tirages que de
	// regles, on donne une preference aux regles ayant un niveau de recursion plus faible, puis un nombre
	// d'operande plus faible Cela ne change rien au prior, car on utilise qu'un epsilon perturbation des
	// probabilites En sortie, le tableau des regles de construction est trie par niveau de recursion, puis nombre
	// d'operandes croissant Le tableau des regles de construction est utilise pour dimensionner le vecteur en
	// sortie
	void ComputeConstructionRuleProbs(double dRandomDrawingNumber, ObjectArray* oaConstructionRules,
					  DoubleVector* dvConstructionRuleProbs,
					  DoubleVector* dvRandomDrawingNumbers) const;

	// Repartition d'un ensemble de tirage de facon equiprobable dans un tableau de tirage en sortie
	// en fonction d'un ensemble de regles construites (KDConstructedRule) correspondant aux choix disponibles
	// Le tableau des regles est utilise pour dimensionner le vecteur en sortie
	void DispatchConstructedRuleRandomDrawingNumbers(double dRandomDrawingNumber,
							 const ObjectArray* oaConstructedRules,
							 DoubleVector* dvRandomDrawingNumbers) const;

	// Filtrage a priori d'un ensemble de regles construites (KDConstructedRule) pour supprimer les regles de
	// probabilite trop petite pour etre tirees En sortie, le tableau de regles est retaille et les regles inutiles
	// sont detruites Le code retour indique si un filtrage a ete effectue
	boolean FilterConstructedRulesForRandomDrawing(double dRandomDrawingNumber,
						       ObjectArray* oaConstructedRules) const;

	//////////////////////////////////////////////////////////////////////////
	// Application des regles et contraintes a la classe principale
	// (et son eventuelle arborescence multi-tables) pour determiner quelles
	// regles sont applicables pour chaque classe

	// Detection basique si la construction est possible en ne regardant sommairement que le type des attributs de
	// la classe principale
	boolean IsConstructionBasicallyPossible(KWClass* mainClass) const;

	// Calcul des impacts des contraintes sur la classe principale et son arborescence
	// Collecte de tous les attributs derives existants
	// Le calcul ecrase les resultats precedents
	void ComputeAllClassesCompliantRules(KWClass* mainClass,
					     KDClassDomainCompliantRules* outputClassDomainCompliantRules) const;

	// Acces a l'objet de gestion des regles applicables pour l'ensemble des classes
	// Memoire: l'objet rendu  appartiennent a l'appele
	KDClassDomainCompliantRules* GetClassDomainCompliantRules() const;

	// Acces aux regles applicable par nom de classe
	KDClassCompliantRules* LookupClassCompliantRules(const ALString& sClassName) const;

	// La classe suivante est amie, pour avoir acces aux fonctionnalites sur les regles applicables par classe
	friend class KDClassBuilder;

	///////////////////////////////////////////////////////////////////////////////
	// Tests d'applicabilite des regles de construction

	// Test si la regle de construction est applicable sur la classe
	boolean IsConstructionRuleApplicable(const KDConstructionRule* constructionRule,
					     const KDClassCompliantRules* classCompliantRules) const;

	// Test d'applicabilite pour la fin des operandes, en precisant la classe de base et la classe de sous-niveaux
	// pour les eventuels operande de sous niveau (dans un Object ou ObjectArray)
	boolean IsConstructionRuleApplicableFromLastOperands(
	    const KDConstructionRule* constructionRule, int nStartOperandIndex,
	    const KDClassCompliantRules* classCompliantRules,
	    const KDClassCompliantRules* secondaryScopeClassCompliantRules) const;

	// Extraction des attributs de la classe compatibles avec le type en parametre
	// Gestion particuliere des regles de selection (deuxieme operande numerique ou categoriel)
	// Memoire: les attributs en retour sont referencees
	void ExtractMatchingAttributes(const KDConstructionRule* constructionRule,
				       const KWDerivationRuleOperand* operand,
				       const KDClassCompliantRules* classCompliantRules,
				       ObjectArray* oaMatchingAttributes) const;

	// Extraction des regles applicables dont le code retour est compatible avec le type en parametre
	// Gestion particuliere des regles de selection (deuxieme operande numerique ou categoriel)
	// Memoire: les regles de construction en retour sont referencees
	void ExtractMatchingRules(const KDConstructionRule* constructionRule, const KWDerivationRuleOperand* operand,
				  const KDClassCompliantRules* classCompliantRules,
				  ObjectArray* oaMatchingConstructionRules) const;

	// Test si un type est compatible avec un type de reference
	boolean IsTypeMatching(int nRefType, const ALString& sRefObjectClassName, int nType,
			       const ALString& sObjectClassName) const;

	///////////////////////////////////////////////////////////////////////////////
	// Gestion particuliere des regles de selection (cf. KDConstructionRule::IsSelectionRule)
	//
	// Un regle de selection prend en premier argument un ObjectArray, puis une liste
	// d'operateur de selection portant sur des attributs (our regles) issus de cet ObjectArray.
	// Les operandes sont donc en nombre variables, numeriques ou categoriels.
	// Pour les operandes numeriques, on choisit d'abord la granularite (par puissance de deux),
	// puis l'index du partile a un niveau de granularite. L'operateur de selection est un
	// test d'appartenance a un intervalle.
	// Pour les operandes categorielles, on chosit d'abord la granularite (par puissance de deux),
	// interpretee ici comme un effectif minimum pour choisir un sous-ensemble de valeurs,
	// puis l'index de la valeur dans cet ensemble. L'operateur de selection est un
	// test d'egalite a une valeur.
	//
	// On impose la contrainte de ne pas avoir d'appels recursifs de regles de selection:
	// aucun de ses operandes ne peut etre base (recursivement) sur une autre regle de selection.
	//
	// Dans une premiere passe, on construit les regles sans avoir vue les donnees, en utililisant
	// une description conceptuelle EQc(VarC, "G_i") ou EQc(AsCategorical(VarN), "G_i"), ou G
	// represente la granularite et i l'index de partie ou de valeur.
	// Dans une deuxieme passe, on identifie les attributs ou regles intervenant en operandes de
	// selection est fait une passe de lecture sur la base pour collecter des statistiques
	// par valeur ou partiles, a un niveau de grain maximal (selon la memoire disponible).
	// Dans une troisieme passe, on construit les regle a nouveau en utilisant les statistiques
	// collectees, ce qui permet de construire les regles avec leur vraies valeurs.
	// Par exemple SymbolEQ(VarC, "Value1") ou And(GE(VarN, 2.1), L(VarN, 5.3)).

	// Test si un type est compatible avec un type d'operande de selection
	boolean IsTypeMatchingSelectionOperand(const KDConstructionRule* selectionRule,
					       const KWDerivationRuleOperand* operand, int nType,
					       const ALString& sObjectClassName) const;

	// Completion d'une regle de selection a partir de son operande de selection
	// Specialisation de la methode BuildAllConstructedRulesFromLastOperands
	void BuildAllSelectionRulesFromSelectionOperand(const KDConstructionRule* selectionRule,
							const KDClassCompliantRules* classCompliantRules,
							const KDClassCompliantRules* secondaryScopeClassCompliantRules,
							KDConstructedRule* templateConstructedRule,
							const ALString& sPriorTreeNodeName, int nDepth,
							double dRuleCost, double dRandomDrawingNumber,
							ObjectArray* oaAllConstructedRules) const;

	// Calcul de tous les operandes de selection possible pour une taille de selection donnee
	// En entree
	//  . secondaryScopeClassCompliantRules: classe associee aux operandes de selection
	//  . nSelectionRandomDrawingNumber: nombre de tirages
	//  . nSelectionSize: taille de selection
	//  . nMaxSelectionOperandNumber: nombre max d'operandes a constuire
	//  . dMaxSelectionCost: cout maximum de selection
	//  . sPriorTreeNodeName: identifiant du noeud de l'arbre de construction a partir de la racine
	//                         Utilise si non vide, pour indiquer qu'une trace de debugging est demandee
	//  . oaSelectionOperands: tableau des dimensions de partition (KDClassSelectionOperandStats: attribut ou regle)
	//  utilisables en operandes de selection . oaSelectionOperandIndexedFrequencies: tableau des effectif indexes
	//  (index compris entre 0 et taille d'un tableau de dimensions utilisables)
	//                                          specifiant l'ensemble des operande de selection multivaries
	//                                          utilisables Les index de dimension de partition utilisees sont
	//                                          toujours dans le meme ordre pour assurer l'unicite des partition
	// En sortie
	//  . oaAllSelectionParts: partie d'une partition (KDConstructedPart) utilisee en operande de selection
	// Chaque regle de selection construite est une conjonction (regle And) de conditions sur les attributs.
	// Le cout total de selection
	// (Sum(SelectedAttributeCost)+Sum(SelectedRuleCost)+SelectionCost(size)+Sum(ConditionCost)) est porte par
	// l'operande construit
	void BuildAllSelectionParts(const KDClassCompliantRules* secondaryScopeClassCompliantRules,
				    double dSelectionRandomDrawingNumber, int nSelectionSize,
				    int nMaxSelectionOperandNumber, double dMaxSelectionCost,
				    const ALString& sPriorTreeNodeName, const ObjectArray* oaSelectionOperands,
				    const ObjectArray* oaSelectionOperandIndexedFrequencies,
				    ObjectArray* oaAllSelectionParts) const;

	// Construction d'une ensemble suffisant (en nombre) de selection de valeurs (KDSelectionSpec)
	// pour un nombre de tirage et operande de selection (variable ou regle depuis un KWDerivariationRuleOperand)
	// donnes En premiere passe, le parametre classSelectionOperandStats n'a pas de stastiqtiques calculees,
	// indiquant qu'il faut construire des operandes de selection "conceptuels".
	// En seconde passe, il faut exploiter les valeurs de la base pour construire les operandes de selection
	void ComputeSelectionValues(const KDClassSelectionOperandStats* classSelectionOperandStats,
				    double dRandomDrawingNumber,
				    const KDConstructedPartitionDimension* partitionDimension,
				    ObjectArray* oaSelectionValues) const;

	// Gestion de la contrainte de ne pas avoir d'appels recursifs de regles de selection
	// Par defaut: false, les regles de selection sont autorisees
	boolean IsSelectionRuleForbidden() const;
	void SetSelectionRuleForbidden(boolean bValue) const;

	// Tri synchronise d'un tableau de KDClassSelectionOperandStats et de leur proba
	void SortClassSelectionOperandStatsAndProbs(ObjectArray* oaClassSelectionOperandStats,
						    DoubleVector* dvClassSelectionOperand) const;

	///////////////////////////////////////////////////////////////////////////////
	// Gestion de la pile des attribut du path

	// Ajout d'un attribut dans la pile
	void DataPathPushTableAttribute(const KWAttribute* tableAttribute) const;

	// Supression d'un attribut de la pile
	void DataPathPopTableAttribute() const;

	// Acces a la tete de la pile
	const KWAttribute* DataPathGetHeadTableAttribute() const;

	// Taille de la pile
	int DataPathGetLength() const;

	// Initialisation de la pile
	void DataPathInit() const;

	///////////////////////////////////////////////////////////////////////////////
	// Attribut de la classe

	// Domaine de construction
	KDConstructionDomain* constructionDomain;

	// Acces aux contraintes par classes du domain lie a la classe courante
	KDClassDomainCompliantRules* classDomainCompliantRules;

	// Object d'analyse des operandes de selection
	// Si les calculs sont effectues: on pourra construire de vrais operande de selection exploitant les valeurs du
	// jeu de donnees Sinon: les operandes de selection seront construites de facon "conceptuelle" Memoire:
	// attention, cete objet utilise des objet de oaAllClassesCompliantRules pour piloter sa destruction, et doit
	// donc etre detruit en premier
	KDSelectionOperandAnalyser* selectionOperandAnalyser;

	// Chemin de donnees courant, permettant a tout moment de la construction de variable ou on en est
	// dans le chemin d'acces aux donnees
	// A specifier au fur et a mesure de la construction de variable
	// Permet de specifier le chemin corrrespondant a chaque partition consideree pour les operandes de selection
	mutable ObjectArray oaDataPathTableAttributes;

	// Nombre maximum de regles construites
	int nMaxRuleNumber;

	// Profondeur maximale des regles construites
	int nMaxRuleDepth;

	// Cout maximal des regles construites
	double dMaxRuleCost;

	// Contrainte d'intertection de l'utilisation des regles de selection
	mutable boolean bIsSelectionRuleForbidden;

	// Timer pour mesurer le temps de construction des variables
	mutable Timer timerConstruction;

	// Specifications de construction
	KWClass* kwcConstructedClass;
	KWClassDomain* kwcdConstructedDomain;
};

///////////////////////////////////////////////////////////////////////////////
// Class KDSelectionSpec
// Gestion d'un operande de selection de valeur pour une variable native ou calculee
// La selection de valeur repose sur un choix de granularite et de partile pour
// une granularite donnee
// Variables numeriques:
//  . granularite I: 2, 4, 8, 16
//  . partile i: index d'intervalle ]N i/I; N (i+1)/I], 0 <= i < I
//  . en pratique, on ne considere que les intervalles distincts
//    (il peut y a avoir des collision si moins de valeurs que d'individus)
// Variables categorielles:
//  . granularite I: 2, 4, 8, 16
//  . seules les V(I) valeurs d'effectifs au moins N/I sont considerees
//  . partile: index de de valeur, 0 <= i < I
//  . en pratique, 0 <= I < V(I)
class KDSelectionSpec : public Object
{
public:
	// Constructeur
	KDSelectionSpec();
	~KDSelectionSpec();

	// Granularite (doit etre une puissance de deux)
	void SetGranularity(int nValue);
	int GetGranularity() const;

	// Partie
	void SetPartIndex(int nValue);
	int GetPartIndex() const;

	// Cout associe a la selection de valeur
	//  Choix de la granularite: codage de Rissanen (en codant l'exposant de la puissance de deux)
	//  Choix du partile: uniforme (ln I)
	double GetCost() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Test d'integrite
	boolean Check() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	int nGranularity;
	int nPart;
};

//////////////////////////////
// Methode en inline

inline void KDDomainKnowledge::DataPathPushTableAttribute(const KWAttribute* tableAttribute) const
{
	require(tableAttribute != NULL);
	require(DataPathGetLength() == 0 or
		DataPathGetHeadTableAttribute()->GetClass() == tableAttribute->GetParentClass());
	oaDataPathTableAttributes.Add(cast(Object*, tableAttribute));
}

inline void KDDomainKnowledge::DataPathPopTableAttribute() const
{
	oaDataPathTableAttributes.SetSize(oaDataPathTableAttributes.GetSize() - 1);
}

inline const KWAttribute* KDDomainKnowledge::DataPathGetHeadTableAttribute() const
{
	require(DataPathGetLength() > 0);
	return cast(const KWAttribute*, oaDataPathTableAttributes.GetAt(oaDataPathTableAttributes.GetSize() - 1));
}

inline int KDDomainKnowledge::DataPathGetLength() const
{
	return oaDataPathTableAttributes.GetSize();
}

inline void KDDomainKnowledge::DataPathInit() const
{
	oaDataPathTableAttributes.SetSize(0);
}
