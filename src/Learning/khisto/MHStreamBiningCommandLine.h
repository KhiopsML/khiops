// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "CommandLine.h"
#include "MHStreamBining.h"
#include "MHStreamBiningMixed.h"

////////////////////////////////////////////////////////////////
// Classe MHStreamBiningCommandLine
// Gestion de la binarisation d'un ensemble de valeurs depuis la ligne de commande
class MHStreamBiningCommandLine : public Object
{
public:
	// Lancement depuis la ligne de commande
	static boolean ComputeBinsFromCommandLine(int argc, char** argv);

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Methodes associees aux option de la ligne de commande
	static boolean ShowVersion(const ALString& sValue);
	static boolean OkMethod(const ALString& sValue);

	// DEPRECATED, en attendant l'evolution de la classe CommandLine
	static boolean DEPRECATEDCommandLineSetFloatingPointBins(const ALString& sValue);
	static boolean bDEPRECATEDCommandLineFloatingPointBins;
	static boolean DEPRECATEDCommandLineSetEqualWidthBins(const ALString& sValue);
	static boolean bDEPRECATEDCommandLineEqualWidthBins;
	static boolean DEPRECATEDCommandLineSetSaturatedBins(const ALString& sValue);
	static boolean bDEPRECATEDCommandLineSaturatedBins;
};
