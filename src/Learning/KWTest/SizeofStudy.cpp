// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SizeofStudy.h"

class RefObject : public Object
{
	// KWClass
	const KWClass* kwcClass;

	// Index de creation permettant de memoriser un numero de ligne lors de la lecture d'un fichier (0 si non
	// initialise) Permet d'implementer les regles de derivation Index et Random avec des resultats reproductibles
	// lors de la lecture des meme bases
	// Cet index est attribue a la creation de l'objet et n'est jamais modifie (hormis son signe)
	// Pour optimiser la taille memoire des KWObject, on se sert de l'index pour stocker egalement le type de taille
	// d'objet
	//  . petite taille: index positif
	//  . grande taille: index negatif
	longint lCreationIndex;

	//////////////////////////////////////////////////////////////////////////////
	// Pour gerer les objet avec un nombre potentiel d'attributs tres important,
	// les valeur ne sont pas accessible directment par un tableau (KWValue*),
	// mais par un tableau de tableaux de valeurs, chaque tableau de valeur ne
	// depassant pas la taille d'un segment memoire.
	// La gestion de ces tableaux de grande taille est analogue a celle mise en
	// place dans la classe MemVector, mais en plus otpimise en tenant compte
	// des particularite suivantes:
	//  . la taille des tableau est fixe une fois pour toute par la KWClass
	//  . seul l'acces aux valeur est necessaire (pas de fonctionnalite de type
	//    retaillage, tri, randomisation de l'odre...)

	// Gestion des valeurs dans un tableau
	//  . attributeValues contient toutes les valeurs dans un tableau
	//  . attributeValueArrays contient le tableau de tableaux de valeurs
	union ObjectValues
	{
		KWValue* attributeValues;
		KWValue** attributeValueArrays;
	} values;

	// Informations disponibles uniquement en mode debug, pour le controle
	// de la coherence entre KWClass et KWObject
	debug(int nAttributeNumber);
	debug(int nFreshness);
};

class RefDerivationRule : public Object
{
	// Attributs de base de la regle de derivation
	ALString sName;
	ALString sLabel;
	ALString sClassName;

	// Type de la valeur retour
	int nType;

	// Complement de type (StructureName ou ObjectClassName)
	ALString sSupplementTypeName;

	// Operandes
	// Les pointeurs sur le premier et le second operandes sont synchronises
	// avec la gestion du tableau d'operandes
	KWDerivationRuleOperand* firstOperand;
	KWDerivationRuleOperand* secondOperand;
	ObjectArray oaOperands;
	boolean bVariableOperandNumber;

	// Gestion des regles a scope multiple
	boolean bMultipleScope;

	// Tableau des operandes secondaires de scope principal pour les regles a scope multiples
	// Memoire: ces operandes sont de references au operandes concernes
	ObjectArray* oaMainScopeSecondaryOperands;

	// Classe utilisee pour la compilation
	const KWClass* kwcClass;

	// Cout d'une regle pour la regularisation de la construction de variable
	double dCost;

	// Gestion de la fraicheur
	int nFreshness;
	int nClassFreshness;
	int nCompileFreshness;
};

class OptDerivationRule : public Object
{
	// Attributs de base de la regle de derivation
	// ALString sName;
	// ALString sLabel;
	// ALString sClassName;
	Symbol sName;
	Symbol sLabel;
	Symbol sClassName;

	// Type de la valeur retour
	// int nType;

	// Complement de type (StructureName ou ObjectClassName)
	// ALString sSupplementTypeName;
	Symbol sSupplementTypeName;

	// Operandes
	// Les pointeurs sur le premier et le second operandes sont synchronises
	// avec la gestion du tableau d'operandes
	// KWDerivationRuleOperand* firstOperand;
	// KWDerivationRuleOperand* secondOperand;
	ObjectArray oaOperands;
	// boolean bVariableOperandNumber;
	bool bVariableOperandNumber;

	// Gestion des regles a scope multiple
	// boolean bMultipleScope;
	bool bMultipleScope;

	// Type de la valeur retour
	// int nType;
	KWNewType type;

	// Tableau des operandes secondaires de scope principal pour les regles a scope multiples
	// Memoire: ces operandes sont de references au operandes concernes
	ObjectArray* oaMainScopeSecondaryOperands;

	// Classe utilisee pour la compilation
	const KWClass* kwcClass;

	// Cout d'une regle pour la regularisation de la construction de variable
	double dCost;

	// Gestion de la fraicheur
	int nFreshness;
	int nClassFreshness;
	int nCompileFreshness;
};

class RefDerivationRuleOperand : public Object
{
public:
	// Specifications optimisee sous fomre de char (consecutifs) plutot que int, pour gaggner de la place en
	// memoire)
	char cType;           // Type de l'operande
	char cScopeLevel;     // Niveau de scope
	char cOrigin;         // Origine de l'operande
	char cCompiledOrigin; // Origine de l'operande apres compilation

	// Complement de type (StructureName ou ObjectClassName)
	ALString sSupplementTypeName;

	// Origine de la valeur
	KWValue kwvConstant;
	ALString sAttributeName;
	KWDerivationRule* rule;

	// Origine compilee de la valeur
	const KWClass* kwcClass;
	int nAttributeIndex;

	// Informations disponibles uniquement en mode debug, pour le controle
	// de la coherence de l'acces aux operandes uniquement en mode compile
	debug(boolean IsCompiled() const);
	debug(int GetFreshness() const);
	debug(int nFreshness);
	debug(int nClassFreshness);
	debug(int nCompileFreshness);
};

class OptDerivationRuleOperand : public Object
{
public:
	// Specifications optimisee sous fomre de char (consecutifs) plutot que int, pour gaggner de la place en
	// memoire)
	KWNewType type;       // Type de l'operande
	char cScopeLevel;     // Niveau de scope
	char cOrigin;         // Orifine de l'operande
	char cCompiledOrigin; // Origine de l'operande apres compilation

	// Complement de type (StructureName ou ObjectClassName)
	Symbol sSupplementTypeName;

	// Origine de la valeur
	KWValue kwvConstant;
	Symbol sAttributeName;
	KWDerivationRule* rule;

	// Origine compilee de la valeur
	const KWClass* kwcClass;

	// Index de l'attribut ou du bloc dans le cas d'une origine attribut
	KWLoadIndex liDataItemLoadIndex;

	// Informations disponibles uniquement en mode debug, pour le controle
	// de la coherence de l'acces aux operandes uniquement en mode compile
	debug(boolean IsCompiled() const);
	debug(int GetFreshness() const);
	debug(int nFreshness);
	debug(int nClassFreshness);
	debug(int nCompileFreshness);
};

class RefAttribute : public Object
{
	// Attributs publics
	ALString sName;
	KWMetaData metaData;
	ALString sLabel;
	KWNewType type;
	KWClass* attributeClass;
	ALString sStructureName;
	KWDerivationRule* kwdrRule;
	boolean bUsed;
	boolean bLoaded;
	int nLoadIndex;
	double dCost;

	// Format pour la conversion des types Date, Time, Timestamp
	// Un seul objet sert a stocker les trois type de conversion de formats
	Object* complexTypeFormat;

	// Classe utilisant l'attribut
	KWClass* parentClass;

	// Position dans la liste des attributs de la classe mere
	POSITION listPosition;
};

class OptAttribute : public Object
{
	// Attributs publics
	// ALString sName;
	Symbol sName;
	KWMetaData metaData;
	// ALString sLabel;
	Symbol sLabel;
	KWClass* attributeClass;
	// ALString sStructureName;
	Symbol sStructureName;
	KWDerivationRule* kwdrRule;
	// int nType;
	KWNewType type;
	// boolean bUsed;
	// boolean bLoaded;
	bool bUsed;
	bool bLoaded;
	KWLoadIndex liLoadIndex;
	double dCost;

	// Format pour la conversion des types Date, Time, Timestamp
	// Un seul objet sert a stocker les trois type de conversion de formats
	Object* complexTypeFormat;

	// Classe utilisant l'attribut
	KWClass* parentClass;

	// Position dans la liste des attributs de la classe mere
	POSITION listPosition;
};

class OptSparseAttribute : public Object
{
	// Attributs publics
	// ALString sName;
	Symbol sName;
	KWMetaData metaData;
	// ALString sLabel;
	Symbol sLabel;
	// SPARSE
	KWClass* attributeClass;
	// ALString sStructureName;
	// SPARSE
	Symbol sStructureName;
	// SPARSE
	KWDerivationRule* kwdrRule;
	// int nType;
	KWNewType type;
	// boolean bUsed;
	// boolean bLoaded;
	bool bUsed;
	bool bLoaded;
	int nLoadIndex;
	double dCost;

	OptAttribute* sparseBlock; // SPARSE
	//
	// Format pour la conversion des types Date, Time, Timestamp
	// Un seul objet sert a stocker les trois type de conversion de formats
	// SPARSE
	Object* complexTypeFormat;

	// Classe utilisant l'attribut
	KWClass* parentClass;

	// Position dans la liste des attributs de la classe mere
	POSITION listPosition;
};

class RefKeyValuePair : public Object
{
	// Variable de classe
	ALString sKey;
	int nType;
	ALString sStringValue;
	double dDoubleValue;
};

class OptKeyValuePair : public Object
{
public:
	const ALString GetKey() const;

	// Test
	static void Test();

	// Variable de classe
	Symbol sKey;
	union
	{
		KWSymbolData* sStringValue;
		double dDoubleValue;
	};
	char cType;
};

const ALString OptKeyValuePair::GetKey() const
{
	return sKey.GetValue();
}

void OptKeyValuePair::Test()
{
	OptKeyValuePair keyValuePair;
	ALString sKey;
	const char* sK;

	cout << "OptKeyValuePair::Test" << endl;
	cout << "\t" << keyValuePair.GetKey() << endl;
	sK = keyValuePair.GetKey();
	cout << "\t" << sK << endl;
	sKey = keyValuePair.GetKey();
	cout << "\t" << sKey << endl;
}

class RefType
{
public:
	// Liste des types disponibles
	enum
	{
		Symbol,      // Type symbol (valeur de type Symbol)
		Continuous,  // Type continu (valeur de type Continous (cf. KWContinuous))
		Date,        // Type date (valeur de type Date (cf. KWDate))
		Time,        // Type time (valeur de type Time (cf. KWTime))
		Timestamp,   // Type timestamp (valeur de type Timestamp (cf. KWTimestamp))
		Object,      // Type object (objet de type KWObject, gere par une KWClass)
		ObjectArray, // Type tableau d'objects (tableau d'objets de type KWObject)
		Structure,   // Type algorithmique (objet de d'une classe specialisee, heritant de Object)
		None,        // Type absent deliberement, pour le non supervise
		Unknown      // Type inconnu (non valide)
	};
};

void OptType::Test()
{
	OptType type;
	OptType type2;
	bool b1 = true;
	bool b2 = false;
	int i;

	cout << "OptType::Test" << endl;
	type = 1;
	type = '1';
	type = OptType::Unknown;
	type2 = type;
	cout << "\t" << type2 << endl;

	cout << "booleans " << b1 << " " << b2 << endl;
	i = b1;
	i = b1 + b2;
	b1 = 1 <= 2;
	b2 = b1;
	b2 = not b1;
	b1 = false;
}

void SizeofStudyTest()
{
	cout << "sizeof(int)\t" << sizeof(int) << endl;
	cout << "sizeof(longint)\t" << sizeof(longint) << endl;
	cout << "sizeof(boolean)\t" << sizeof(boolean) << endl;
	cout << "sizeof(bool)\t" << sizeof(bool) << endl;
	cout << "sizeof(ALString)\t" << sizeof(ALString) << endl;
	cout << "sizeof(string)\t" << sizeof(string) << endl;
	cout << "sizeof(Symbol)\t" << sizeof(Symbol) << endl;
	cout << "sizeof(KWSymbolData)\t" << sizeof(KWSymbolData) << endl;
	cout << "sizeof(Object)\t" << sizeof(Object) << endl;
	cout << "sizeof(IntVector)\t" << sizeof(IntVector) << endl;
	cout << "sizeof(ObjectArray)\t" << sizeof(ObjectArray) << endl;
	cout << "sizeof(NumericKeyDictionary)\t" << sizeof(NumericKeyDictionary) << endl;
	cout << "sizeof(NKDAssoc)\t" << sizeof(NKDAssoc) << endl;
	cout << "sizeof(SortedList)\t" << sizeof(SortedList) << endl;
	cout << "sizeof(AVLNode)\t" << sizeof(AVLNode) << endl;
	cout << "sizeof(KWClass)\t" << sizeof(KWClass) << endl;
	cout << "sizeof(KWAttribute)\t" << sizeof(KWAttribute) << endl;
	cout << "sizeof(KWDerivationRule)\t" << sizeof(KWDerivationRule) << endl;
	cout << "sizeof(KWDerivationRuleOperand)\t" << sizeof(KWDerivationRuleOperand) << endl;
	cout << "sizeof(KWMetaData)\t" << sizeof(KWMetaData) << endl;
	cout << "sizeof(KWKeyValuePair)\t" << sizeof(KWKeyValuePair) << endl;
	cout << "sizeof(KWObject)\t" << sizeof(KWObject) << endl;
	cout << "sizeof(KWType)\t" << sizeof(KWType) << endl;
	cout << "sizeof(KWValue)\t" << sizeof(KWValue) << endl;
	cout << "sizeof(KWTuple)\t" << sizeof(KWTuple) << endl;
	cout << "sizeof(KWMODLLine)\t" << sizeof(KWMODLLine) << endl;
	cout << "sizeof(RefDerivationRule)\t" << sizeof(RefDerivationRule) << endl;
	cout << "sizeof(OptDerivationRule)\t" << sizeof(OptDerivationRule) << endl;
	cout << "sizeof(RefDerivationRuleOperand)\t" << sizeof(RefDerivationRuleOperand) << endl;
	cout << "sizeof(OptDerivationRuleOperand)\t" << sizeof(OptDerivationRuleOperand) << endl;
	cout << "sizeof(RefAttribute)\t" << sizeof(RefAttribute) << endl;
	cout << "sizeof(OptAttribute)\t" << sizeof(OptAttribute) << endl;
	cout << "sizeof(OptSparseAttribute)\t" << sizeof(OptSparseAttribute) << endl;
	cout << "sizeof(RefObject)\t" << sizeof(RefObject) << endl;
	cout << "sizeof(RefKeyValuePair)\t" << sizeof(RefKeyValuePair) << endl;
	cout << "sizeof(OptKeyValuePair)\t" << sizeof(OptKeyValuePair) << endl;
	cout << "sizeof(RefType)\t" << sizeof(RefType) << endl;
	cout << "sizeof(OptType)\t" << sizeof(OptType) << endl;

	cout << "sizeof(KWValueDictionary)\t" << sizeof(KWValueDictionary) << endl;
	cout << "sizeof(KWContinuousValueDictionary)\t" << sizeof(KWContinuousValueDictionary) << endl;
	cout << "sizeof(KWSymbolValueDictionary)\t" << sizeof(KWSymbolValueDictionary) << endl;
	cout << "sizeof(KWObjectArrayValueDictionary)\t" << sizeof(KWObjectArrayValueDictionary) << endl;
	cout << "sizeof(KWValueBlock)\t" << sizeof(KWValueBlock) << endl;
	cout << "sizeof(KWContinuousValueBlock)\t" << sizeof(KWContinuousValueBlock) << endl;
	cout << "sizeof(KWSymbolValueBlock)\t" << sizeof(KWSymbolValueBlock) << endl;
	cout << "sizeof(KWObjectArrayValueBlock)\t" << sizeof(KWObjectArrayValueBlock) << endl;
	cout << "sizeof(KWKeyIndex)\t" << sizeof(KWKeyIndex) << endl;
	cout << "sizeof(KWContinuousKeyIndex)\t" << sizeof(KWContinuousKeyIndex) << endl;
	cout << "sizeof(KWSymbolKeyIndex)\t" << sizeof(KWSymbolKeyIndex) << endl;
	cout << "sizeof(KWObjectArrayKeyIndex)\t" << sizeof(KWObjectArrayKeyIndex) << endl;
	cout << "sizeof(KWIndexedKeyBlock)\t" << sizeof(KWIndexedKeyBlock) << endl;

	cout << "sizeof(KDConstructionDomain)\t" << sizeof(KDConstructionDomain) << endl;
	cout << "sizeof(KWDataTableDriver)\t" << sizeof(KWDataTableDriver) << endl;
	cout << "sizeof(KWDataTableDriverTextFile)\t" << sizeof(KWDataTableDriverTextFile) << endl;
	cout << "sizeof(KWDatabase)\t" << sizeof(KWDatabase) << endl;
	cout << "sizeof(KWDataGrid)\t" << sizeof(KWDataGrid) << endl;
	cout << "sizeof(KWDataGridStats)\t" << sizeof(KWDataGridStats) << endl;
	cout << "sizeof(KWFrequencyVector)\t" << sizeof(KWFrequencyVector) << endl;
	cout << "sizeof(KWLearningReport)\t" << sizeof(KWLearningReport) << endl;
	cout << "sizeof(KWLearningSpec)\t" << sizeof(KWLearningSpec) << endl;
	cout << "sizeof(KWProbabilityTable)\t" << sizeof(KWProbabilityTable) << endl;
	cout << "sizeof(KWSortBuckets)\t" << sizeof(KWSortBuckets) << endl;
	cout << "sizeof(KWLearningSpec)\t" << sizeof(KWLearningSpec) << endl;
	cout << "sizeof(KWAttributeSpec)\t" << sizeof(KWAttributeSpec) << endl;

	// OptType::Test();
	// OptKeyValuePair::Test();
}