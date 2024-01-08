// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHStreamBiningCommandLine.h"

boolean MHStreamBiningCommandLine::ComputeBinsFromCommandLine(int argc, char** argv)
{
	boolean bOk = true;
	MHStreamBining streamBining;
	MHStreamBiningMixed streamBiningMixed;
	CommandLine commandLine;
	CommandLineOption* optionInputDataFile;
	CommandLineOption* optionOutputBinFile;
	CommandLineOption* optionMaxBinNumber;
	CommandLineOption* optionEqualWidthBins;
	CommandLineOption* optionFloatingPointBins;
	CommandLineOption* optionSaturatedBins;
	CommandLineOption* optionGetVersion;
	CommandLineOption* option;
	ALString sInputFileName;
	ALString sOutputFileName;
	const int nDefaultMaxBinNumber = 1000;
	int nMaxBins;
	int i;
	ALString sTmp;

	// Suggestion d'amelioration de la classe CommandLine
	// 	 . possibilite d'avoir des parametres obligatoire en fin des parametres?
	//   . specifier des options obligatoire (Mandatory), devant etre presentes?
	//   . etendre les types des parametres (sParameterInt, sParameterFloat)
	//     et controler leur validite dans le cas numerique
	//   . permettre de savoir si un option est Used en l'interrogeant (GetUsed() en public?)
	//   . implementer GetClassLabel() et GetObjectLabel() dans CommandLineOption pour
	//     pouvoir appeler la methode AddError
	//   . ajouter une methode ShowHelp

	// Ajout d'options a l'interface
	commandLine.SetCommandName("streambining");
	commandLine.SetGlobalHelp(
	    "Build a micro-bin synopsis from a list of values, up to a max number of micro-bins\n"
	    "\n"
	    "The micro-bins can be equal-width oriented or floating-point oriented, or general micro-bins (default).\n"
	    "General micro-bins are a mixture of equal-width and floating-point oriented micro-bins, more resilient to "
	    "either\n"
	    "standard or heavy-tail distributions; the size of resulting synopsis can be smaller than the max "
	    "micro-bin number.\n"
	    "Saturated micro-bin synopsis are potentially faster to compute, more compact, but less accurate\n");

	// Fichier de donnees en entree
	optionInputDataFile = new CommandLineOption;
	optionInputDataFile->SetFlag('i');
	optionInputDataFile->AddDescriptionLine("input data file, with one value per line");
	optionInputDataFile->SetParameterRequired(true);
	optionInputDataFile->SetParameterDescription(CommandLineOption::sParameterFile);
	optionInputDataFile->SetMethod(OkMethod);
	commandLine.AddOption(optionInputDataFile);

	// Fichier des bins en sortie
	optionOutputBinFile = new CommandLineOption;
	optionOutputBinFile->SetFlag('o');
	optionOutputBinFile->AddDescriptionLine(
	    "output bin file, with a triple (min, max, count) in tabular format per line");
	optionOutputBinFile->SetParameterRequired(true);
	optionOutputBinFile->SetParameterDescription(CommandLineOption::sParameterFile);
	optionOutputBinFile->SetMethod(OkMethod);
	commandLine.AddOption(optionOutputBinFile);

	// Parametre sur le nombre max de bins
	optionMaxBinNumber = new CommandLineOption;
	optionMaxBinNumber->SetFlag('m');
	optionMaxBinNumber->AddDescriptionLine("max bin number (default: 1000)");
	optionMaxBinNumber->SetParameterRequired(true);
	optionMaxBinNumber->SetParameterDescription("<int>");
	optionMaxBinNumber->SetMethod(OkMethod);
	commandLine.AddOption(optionMaxBinNumber);

	// Parametre sur le type floating-point des bins
	optionFloatingPointBins = new CommandLineOption;
	optionFloatingPointBins->SetFlag('f');
	optionFloatingPointBins->AddDescriptionLine("build floating-point oriented bins rather than general ones");
	optionFloatingPointBins->SetParameterRequired(false);
	optionFloatingPointBins->SetMethod(DEPRECATEDCommandLineSetFloatingPointBins);
	commandLine.AddOption(optionFloatingPointBins);

	// Parametre sur le type floating-point des bins
	optionEqualWidthBins = new CommandLineOption;
	optionEqualWidthBins->SetFlag('w');
	optionEqualWidthBins->AddDescriptionLine("build equal-width oriented bins rather than general ones");
	optionEqualWidthBins->SetParameterRequired(false);
	optionEqualWidthBins->SetMethod(DEPRECATEDCommandLineSetEqualWidthBins);
	commandLine.AddOption(optionEqualWidthBins);

	// Parametre sur la production d'un resume sature
	optionSaturatedBins = new CommandLineOption;
	optionSaturatedBins->SetFlag('s');
	optionSaturatedBins->AddDescriptionLine("build a saturated micro-bin synopsis");
	optionSaturatedBins->SetParameterRequired(false);
	optionSaturatedBins->SetMethod(DEPRECATEDCommandLineSetSaturatedBins);
	commandLine.AddOption(optionSaturatedBins);

	// Option d'aide
	optionGetVersion = new CommandLineOption;
	optionGetVersion->SetFlag('v');
	optionGetVersion->AddDescriptionLine("print version");
	optionGetVersion->SetGroup(1);
	optionGetVersion->SetFinal(true);
	optionGetVersion->SetMethod(ShowVersion);
	optionGetVersion->SetSingle(true);
	commandLine.AddOption(optionGetVersion);

	// Analyse de la ligne de commandes
	bDEPRECATEDCommandLineFloatingPointBins = false;
	bDEPRECATEDCommandLineEqualWidthBins = false;
	bDEPRECATEDCommandLineSaturatedBins = false;
	commandLine.ParseMainParameters(argc, argv);

	// Verification des options obligatoires
	for (i = 0; i < commandLine.GetOptionNumber(); i++)
	{
		option = commandLine.GetOptionAt(i);

		// Verification des parametres obligatoires
		if (option->GetFlag() == 'i' or option->GetFlag() == 'o')
		{
			if (option->GetParameters()->GetSize() == 0)
			{
				Global::AddError("", "", sTmp + "Missing parameter -" + option->GetFlag());
				bOk = false;
			}
		}
	}

	// Verification que les fichier en entree et en sortie sont different
	if (bOk and optionInputDataFile->GetParameters()->GetAt(0) == optionOutputBinFile->GetParameters()->GetAt(0))
	{
		Global::AddError("", "",
				 sTmp + "Input and output files must be different (" +
				     optionInputDataFile->GetParameters()->GetAt(0) + ")");
		bOk = false;
	}

	// Verification de l'option sur le nombre max de bin
	nMaxBins = nDefaultMaxBinNumber;
	if (bOk and optionMaxBinNumber->GetParameters()->GetSize() > 0)
	{
		nMaxBins = StringToInt(optionMaxBinNumber->GetParameters()->GetAt(0));
		if ((IntToString(nMaxBins)) != optionMaxBinNumber->GetParameters()->GetAt(0))
		{
			Global::AddError("", "",
					 sTmp + "Parameter -" + optionMaxBinNumber->GetFlag() + " (" +
					     optionMaxBinNumber->GetParameters()->GetAt(0) + ") should be integer");
			bOk = false;
		}
		else if (nMaxBins < 1)
		{
			Global::AddError("", "",
					 sTmp + "Parameter -" + optionMaxBinNumber->GetFlag() + " (" +
					     optionMaxBinNumber->GetParameters()->GetAt(0) + ") should be at least 1");
			bOk = false;
		}
	}

	// Verification des options de specialisation de types de bins
	if (bOk and bDEPRECATEDCommandLineFloatingPointBins and bDEPRECATEDCommandLineEqualWidthBins)
	{
		Global::AddError("", "",
				 sTmp + "Conflicting parameters " + optionFloatingPointBins->GetFlag() + " and " +
				     optionEqualWidthBins->GetFlag() + " specified");
		bOk = false;
	}

	// Lancement de la commande
	if (bOk)
	{
		sInputFileName = optionInputDataFile->GetParameters()->GetAt(0);
		sOutputFileName = optionOutputBinFile->GetParameters()->GetAt(0);

		// Cas oriente virgule flotante (ou a moins de deux bins)
		if (bDEPRECATEDCommandLineFloatingPointBins or nMaxBins <= 2)
		{
			streamBining.SetMaxBinNumber(nMaxBins);
			streamBining.SetSaturatedBins(bDEPRECATEDCommandLineSaturatedBins);
			streamBining.SetFloatingPointGrid(bDEPRECATEDCommandLineFloatingPointBins);
			bOk = streamBining.ComputeBins(sInputFileName, sOutputFileName);
		}
		// Cas oriente largeur egale
		else if (bDEPRECATEDCommandLineEqualWidthBins)
		{
			streamBining.SetMaxBinNumber(nMaxBins);
			streamBining.SetSaturatedBins(bDEPRECATEDCommandLineSaturatedBins);
			streamBining.SetFloatingPointGrid(not bDEPRECATEDCommandLineEqualWidthBins);
			bOk = streamBining.ComputeBins(sInputFileName, sOutputFileName);
		}
		// Cas general
		else
		{
			streamBiningMixed.SetMaxBinNumber(nMaxBins);
			streamBiningMixed.SetSaturatedBins(bDEPRECATEDCommandLineSaturatedBins);
			bOk = streamBiningMixed.ComputeBins(sInputFileName, sOutputFileName);
		}
	}
	return bOk;
}

boolean MHStreamBiningCommandLine::ShowVersion(const ALString& sValue)
{
	cout << "streambining v1" << endl;
	return true;
}

boolean MHStreamBiningCommandLine::DEPRECATEDCommandLineSetFloatingPointBins(const ALString& sValue)
{
	bDEPRECATEDCommandLineFloatingPointBins = true;
	return true;
}
boolean MHStreamBiningCommandLine::bDEPRECATEDCommandLineFloatingPointBins = false;

boolean MHStreamBiningCommandLine::DEPRECATEDCommandLineSetEqualWidthBins(const ALString& sValue)
{
	bDEPRECATEDCommandLineEqualWidthBins = true;
	return true;
}
boolean MHStreamBiningCommandLine::bDEPRECATEDCommandLineEqualWidthBins = false;

boolean MHStreamBiningCommandLine::DEPRECATEDCommandLineSetSaturatedBins(const ALString& sValue)
{
	bDEPRECATEDCommandLineSaturatedBins = true;
	return true;
}
boolean MHStreamBiningCommandLine::bDEPRECATEDCommandLineSaturatedBins = false;

boolean MHStreamBiningCommandLine::OkMethod(const ALString& sValue)
{
	return true;
}
