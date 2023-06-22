// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningReport.h"
#include "DTDecisionTreeSpec.h"

////////////////////////////////////////////////////////
// Rapport de creation dedie aux arbres
class DTCreationReport : public KWLearningReport
{
public:
	// Constructeur
	DTCreationReport();
	~DTCreationReport();

	// Nettoyage
	void Clean();

	// Ajout d'un arbre cree
	void AddTree(const ALString& sName, DTDecisionTreeSpec* dtTree);

	// Destruction d'un arbre cree
	void DeleteTree(const ALString& sName);

	// Nombre d'arbre crees
	int GetTreeNumber() const;

	// Acces aux arbres creees par index
	const DTDecisionTreeSpec* GetTreeAt(int nIndex) const;

	// initilisation des level arbres
	void SetLevelAt(int nIndex, double dLevel);

	// Acces aux nom des arbres
	const ALString& GetTreeNameAt(int nIndex) const;

	// Acces aux arbres creees par nom
	const DTDecisionTreeSpec* LookupTree(const ALString& sName) const;

	// Ecriture du contenu d'un rapport JSON
	// On doit avoir une description par arbres, avec pour cle le nom de l'arbre
	void WriteJSONFields(JSONFile* fJSON) override;

	// Ecriture d'un tableau de rapport JSON des arbres
	void WriteJSONTreeReport(JSONFile* fJSON, boolean bSummary);

	// calcul des identifiants des arbres selon leur rang
	void ComputeRankIdentifiers();

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Tableaux et dictionnaire des arbres crees
	StringVector svCreatedTreeNames;
	ObjectDictionary odCreatedTrees;

	KWAttributeStats* GetAttributeStats(const ALString sVariableName, const ObjectArray* oaAttributesStats) const;
	void WriteJSONSonNodes(JSONFile* fJSON, DTDecisionTreeNodeSpec* treeNodeSpec,
			       const ObjectArray& leavesNodes) const;
};