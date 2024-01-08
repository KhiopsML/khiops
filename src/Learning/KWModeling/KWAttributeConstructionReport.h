// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningReport.h"
#include "KWAttributeConstructionSpec.h"

///////////////////////////////////////////////////////////////////
// Classe KWAttributeConstructionReport
// Gestion d'un rapport dedie a la construction d'attribut
//
// Cette classe est un wrapper sur la classe de parametrage de la construction d'attribut
// et specialise la classe de rapport pour permettre de modulariser l'ecriture des rapports.
// En particulier, on peut ainsi parametrer la classee de l'ecriture du rapport de preparation
// par ce rapport, pour lui soutraiter une sous-partie du rapport sans en connaitre les details
class KWAttributeConstructionReport : public KWLearningReport
{
public:
	// Constructeur
	KWAttributeConstructionReport();
	~KWAttributeConstructionReport();

	// Parametrage des parametres de construction d'attribut
	void SetAttributeConstructionSpec(const KWAttributeConstructionSpec* spec);
	const KWAttributeConstructionSpec* GetAttributeConstructionSpec() const;

	// Redefinition des methode d'écriture d'une partie de rapport
	// Sans effet si les parametres de construction d'attributs ne sont pas specifies
	void WriteReport(ostream& ost) override;
	void WriteJSONReport(JSONFile* fJSON) override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	const KWAttributeConstructionSpec* attributeConstructionSpec;
};
