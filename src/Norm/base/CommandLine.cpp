// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CommandLine.h"

///////////////////////////////////////
// Implementation de la classe CommandLine

const ALString CommandLineOption::sParameterString = "<string>";
const ALString CommandLineOption::sParameterFile = "<file>";

boolean DefaultCheckMethod(const ObjectArray& oaOptions)
{
	return true;
}

CommandLine::CommandLine()
{
	oaOptions = new ObjectArray;
	checkMethod = DefaultCheckMethod;
	sGlobalHelp = "";
	sCommandName = "";
	nOptionIndex = 0;
}

CommandLine::~CommandLine()
{
	oaOptions->DeleteAll();
	delete oaOptions;
}

void CommandLine::AddOption(CommandLineOption* option)
{
	require(option != NULL);

	require(option->GetParameterRequired() or option->GetParameterDescription() == "");

	// Le flag ne doit pas etre deja present
	require(not LookupFlag(option->GetFlag()));

	// Le flag h est reserve pour l'aide
	require(option->GetFlag() != 'h');
	option->SetUsageIndex(nOptionIndex);
	oaOptions->Add(option);
	nOptionIndex++;
}

int CommandLine::GetOptionNumber() const
{
	return oaOptions->GetSize();
}

CommandLineOption* CommandLine::GetOptionAt(int nIndex) const
{
	return cast(CommandLineOption*, oaOptions->GetAt(nIndex));
}

void CommandLine::SetCommandName(const ALString& sCommand)
{
	sCommandName = sCommand;
}
const ALString& CommandLine::GetCommandName() const
{
	return sCommandName;
}

void CommandLine::SetGlobalHelp(const ALString& sValue)
{
	sGlobalHelp = sValue;
}
const ALString& CommandLine::GetGlobalHelp()
{
	return sGlobalHelp;
}

void CommandLine::DeleteAllOptions()
{
	oaOptions->DeleteAll();
}

void CommandLine::ParseMainParameters(int argc, char** argv) const
{
	boolean bParsingError;
	int parameterIndex;
	boolean bLonelyFlagIsUsed;
	char cLonelyFlag;
	ObjectArray oaOptionsToTrigger;
	ALString sParameter;
	ALString arg;
	CommandLineOption* option;
	int i;
	int j;
	boolean bOk;
	boolean bIsFinal;

	parameterIndex = 0;
	bParsingError = false;
	bLonelyFlagIsUsed = false;
	cLonelyFlag = '\0';
	bOk = true;
	bIsFinal = false;

	require(Check());

	// Analyse des arguments de la ligne de commande
	for (i = 1; i < argc; i++)
	{
		parameterIndex++;
		arg = argv[i];
		if ((arg.GetLength() != 2) or (arg.GetAt(0) != '-'))
		{
			cout << "Unexpected argument (" << arg << ")" << endl;
			bParsingError = true;
			break;
		}
		if (arg.GetAt(1) == 'h')
		{
			PrintUsage();
			GlobalExitOnSuccess();
			break;
		}

		option = GetOptionForFlag(arg.GetAt(1));
		if (option == NULL)
		{
			bParsingError = true;
			cout << "Unexpected -" << arg.GetAt(1) << " flag" << endl;
			break;
		}

		// Une option unique a ete utilisee, il ne peut y en avoir d'autres
		if (bLonelyFlagIsUsed)
		{
			assert(cLonelyFlag != '\0');
			cout << "The -" << cLonelyFlag << " flag must be used alone" << endl;
			bParsingError = true;
			break;
		}

		if (option->GetUsed() and not option->GetRepetitionAllowed())
		{
			cout << "The -" << option->GetFlag() << " flag can be used only once" << endl;
			bParsingError = true;
			break;
		}

		// Cas d'une option qui doit etre utilisee sans autre options
		if (option->GetSingle())
		{
			// On memorise le flag pour l'affichage d'erreur ulterieur
			bLonelyFlagIsUsed = true;
			cLonelyFlag = option->GetFlag();

			// Si on n'utilise pas une option unique en premiere option... elle n'est pas unique
			if (parameterIndex != 1)
			{
				cout << "The -" << cLonelyFlag << " flag must be used alone" << endl;
				bParsingError = true;
				break;
			}
		}

		if (not option->GetUsed())
		{
			oaOptionsToTrigger.Add(option);
			option->SetUsed(true);
		}

		if (option->GetParameterRequired())
		{
			if (i == argc - 1)
			{
				bParsingError = true;
			}
			else
			{
				i++;
				// Test si l'arguments est un flag : de la forme -X
				if (argv[i][0] == '-' and strlen(argv[i]) == 2)
					bParsingError = true;
				else
					option->svParameters.Add(argv[i]);
			}
			if (bParsingError)
			{
				cout << "Missing argument while using the -" << option->GetFlag() << " flag" << endl;
				break;
			}
		}
		else
		{
			option->svParameters.Add("");
		}
	}

	// En cas d'erreur de parsing : affichage de l'aide et sortie en erreur
	if (bParsingError)
	{
		oaOptionsToTrigger.RemoveAll();
		cout << endl;
		PrintUsage();
		GlobalExit();
	}

	// Si les options sont incoherentes : sortie en erreur
	if (not checkMethod(oaOptionsToTrigger))
	{
		GlobalExit();
	}

	// Le parsing des parametres est OK, on appelle toutes les methodes apres les avoir trie selon la priorite
	oaOptionsToTrigger.SetCompareFunction(CommandLineOptionComparePriority);
	oaOptionsToTrigger.Sort();

	for (i = 0; i < oaOptionsToTrigger.GetSize(); i++)
	{
		option = cast(CommandLineOption*, oaOptionsToTrigger.GetAt(i));

		// Construction du parametre
		sParameter = "";

		// Cas ou il y en a au moins 1 : on lance autant de fois la methode qu'il y a de parametres
		for (j = 0; j < option->GetParameters()->GetSize(); j++)
		{
			sParameter = option->GetParameters()->GetAt(j);

			// Lancement de la methode
			bOk = option->GetMethod()(sParameter);
			if (not bOk)
				break;
		}

		if (not bOk)
			break;

		if (option->GetFinal())
		{
			bIsFinal = true;
			break;
		}
	}

	// Nettoyage
	oaOptionsToTrigger.RemoveAll();

	// Si il y a eu un pbm lors de l'excution d'une methode, on sort en erreur
	if (not bOk)
	{
		GlobalExit();
	}

	// Re-initialisation des options
	for (i = 0; i < oaOptionsToTrigger.GetSize(); i++)
	{
		option = cast(CommandLineOption*, oaOptionsToTrigger.GetAt(i));
		option->SetUsed(false);
		option->svParameters.SetSize(0);
	}

	// Lorsqu'une option est finale, on l'execute et on sort tout de suite
	if (bIsFinal)
		GlobalExitOnSuccess();
}

const GlobalCheckMethod CommandLine::GetGlobalCheckMethod() const
{
	return checkMethod;
}

void CommandLine::SetGlobalCheckMethod(GlobalCheckMethod fMethod)
{
	checkMethod = fMethod;
}

int CommandLine::GetMinUsedPriority() const
{
	CommandLineOption* option;
	int i;
	int nMin = INT_MAX;

	for (i = 0; i < oaOptions->GetSize(); i++)
	{
		option = cast(CommandLineOption*, oaOptions->GetAt(i));
		if (option->GetPriority() < nMin)
		{
			nMin = option->GetPriority();
		}
	}
	return nMin;
}

int CommandLine::GetMaxUsedPriority() const
{
	CommandLineOption* option;
	int i;
	int nMax = INT_MIN;

	for (i = 0; i < oaOptions->GetSize(); i++)
	{
		option = cast(CommandLineOption*, oaOptions->GetAt(i));
		if (option->GetPriority() > nMax)
		{
			nMax = option->GetPriority();
		}
	}
	return nMax;
}

CommandLineOption* CommandLine::GetOptionForFlag(char c) const
{
	CommandLineOption* option;
	int i;

	for (i = 0; i < oaOptions->GetSize(); i++)
	{
		option = cast(CommandLineOption*, oaOptions->GetAt(i));
		if (option->GetFlag() == c)
		{
			return option;
			break;
		}
	}
	return NULL;
}

void CommandLine::Write(ostream& ost) const
{
	CommandLineOption* option;
	int i;
	int j;
	int nLength;
	int nLengthMax;
	ALString sFullDescription;

	// Calcul de la taille max du descriptif pour justifier l'affichage
	nLengthMax = 0;
	for (i = oaOptions->GetSize() - 1; i != -1; i--)
	{
		option = cast(CommandLineOption*, oaOptions->GetAt(i));
		nLength = option->GetParameterDescription().GetLength();
		if (option->GetRepetitionAllowed())
			nLength += 5; // Ajout de '(' et ')...'
		if (nLength > nLengthMax)
			nLengthMax = nLength;
	}

	// On ajoute la taille de '-X '
	nLengthMax += 3;

	// Tri des options pour les afficher par groupe
	oaOptions->Sort();

	// Ecriture de la description
	for (i = 0; i < oaOptions->GetSize(); i++)
	{
		option = cast(CommandLineOption*, oaOptions->GetAt(i));
		sFullDescription = "";

		if (option->GetRepetitionAllowed())
			sFullDescription = "(";

		sFullDescription += "-" + ALString(option->GetFlag());
		if (option->GetParameterRequired())
			sFullDescription += " " + option->GetParameterDescription();
		if (option->GetRepetitionAllowed())
			sFullDescription += ")...";

		ost << "   " << left << setw(nLengthMax) << sFullDescription << " "
		    << option->GetDescriptionLines()->GetAt(0) << endl;
		for (j = 1; j < option->GetDescriptionLines()->GetSize(); j++)
		{
			ost << "   " << left << setw(nLengthMax) << " "
			    << " " << option->GetDescriptionLines()->GetAt(j) << endl;
		}
	}
	ost << "   " << left << setw(nLengthMax) << "-h"
	    << " "
	    << "print help" << endl;
}

void CommandLine::PrintUsage() const
{
	// Tri des options dans l'ordre des groupes
	oaOptions->SetCompareFunction(CommandLineOptionCompareForUsage);
	oaOptions->Sort();

	// Affichage
	cout << "Usage: " << sCommandName << " [OPTIONS]" << endl;
	cout << sGlobalHelp << endl;
	cout << "Available options are:" << endl;
	Write(cout);
	cout << endl;
}

boolean CommandLine::LookupFlag(char cFlag) const
{
	CommandLineOption* option;
	int i;

	for (i = oaOptions->GetSize() - 1; i != -1; i--)
	{
		option = cast(CommandLineOption*, oaOptions->GetAt(i));
		if (option->GetFlag() == cFlag)
			return true;
	}
	return false;
}

boolean CommandLine::Check() const
{
	int i;
	boolean bOk;
	Object* o;
	CommandLineOption* option;

	bOk = true;

	// On verifie que toutes les fonction sont definies
	for (i = 0; i < oaOptions->GetSize(); i++)
	{
		o = oaOptions->GetAt(i);
		if (o == NULL)
		{
			bOk = false;
			assert(false);
			break;
		}
		option = cast(CommandLineOption*, o);
		if (option->GetMethod() == NULL)
		{
			bOk = false;
			assert(false);
			break;
		}

		if (option->GetDescriptionLines()->GetSize() == 0)
		{
			bOk = false;
			assert(false);
			break;
		}
	}
	return bOk;
}

static boolean methodForTest(const ALString& sParameter)
{
	if (sParameter == "")
		cout << "execution of method without parameter" << endl;
	else
		cout << "execution of method with parameter: " << sParameter << endl;
	return true;
}

boolean CommandLine::Test(int argc, char** argv)
{
	CommandLine options;
	CommandLineOption* optionA;
	CommandLineOption* optionB;
	CommandLineOption* optionC;
	CommandLineOption* optionD;
	CommandLineOption* optionR;
	CommandLineOption* optionS;
	ALString sGlobalHelp;

	sGlobalHelp = "";
	sGlobalHelp += "Correct examples :\n";
	sGlobalHelp += "   basetest -a test\n";
	sGlobalHelp += "   basetest -b\n";
	sGlobalHelp += "   basetest -c test\n";
	sGlobalHelp += "   basetest -r -r\n";
	sGlobalHelp += "   basetest -s test_s_1 -s test_s_2\n";
	sGlobalHelp += "   basetest -h\n";
	sGlobalHelp += "Correct examples with the same output (due to priorities):\n";
	sGlobalHelp += "   basetest -d -r -s test_s_1 -r -s test_s_2 -a test_a\n";
	sGlobalHelp += "   basetest -a test_a -d -r -r -s test_s_1 -s test_s_2\n";
	sGlobalHelp += "Examples leading to parsing errors :\n";
	sGlobalHelp += "   basetest -a\t\t=> Missing argument\n";
	sGlobalHelp += "   basetest -c\t\t=> Missing argument\n";
	sGlobalHelp += "   basetest -s\t\t=> Missing argument\n";
	sGlobalHelp += "   basetest -b test\t\t=> Unexpected parameter\n";
	sGlobalHelp += "   basetest -a -d\t\t=> Missing argument (-d is considered as a flag)\n";
	sGlobalHelp += "   basetest -d -a\t\t=> Missing argument\n";
	sGlobalHelp += "   basetest -a test -c test\t=> -c flag must be used alone \n";
	sGlobalHelp += "   basetest -b -a test\t\t=> -b flag must be used alone \n";
	sGlobalHelp += "   basetest -a test -a test\t=> -a can not be repeated\n";
	sGlobalHelp += "\n";

	options.SetCommandName("basetest");
	options.SetGlobalHelp(sGlobalHelp);

	optionA = new CommandLineOption;
	optionA->SetFlag('a');
	optionA->AddDescriptionLine("option with parameter");
	optionA->AddDescriptionLine("priority 2");
	optionA->SetMethod(methodForTest);
	optionA->SetParameterRequired(true);
	optionA->SetPriority(2);
	optionA->SetParameterDescription(CommandLineOption::sParameterString);
	options.AddOption(optionA);

	optionB = new CommandLineOption;
	optionB->SetFlag('b');
	optionB->AddDescriptionLine("option without parameter");
	optionB->AddDescriptionLine("single");
	optionB->SetMethod(methodForTest);
	optionB->SetSingle(true);
	options.AddOption(optionB);

	optionC = new CommandLineOption;
	optionC->SetFlag('c');
	optionC->AddDescriptionLine("option with parameters");
	optionC->AddDescriptionLine("single");
	optionC->SetMethod(methodForTest);
	optionC->SetSingle(true);
	optionC->SetParameterRequired(true);
	optionC->SetParameterDescription(CommandLineOption::sParameterString);
	options.AddOption(optionC);

	optionD = new CommandLineOption;
	optionD->SetFlag('d');
	optionD->AddDescriptionLine("option without parameter");
	optionD->SetMethod(methodForTest);
	options.AddOption(optionD);

	optionR = new CommandLineOption;
	optionR->SetFlag('r');
	optionR->AddDescriptionLine("option without parameter");
	optionR->AddDescriptionLine("repetition allowed");
	optionR->SetMethod(methodForTest);
	optionR->SetRepetitionAllowed(true);
	options.AddOption(optionR);

	optionS = new CommandLineOption;
	optionS->SetFlag('s');
	optionS->AddDescriptionLine("option with parameter");
	optionS->AddDescriptionLine("repetition allowed");
	optionS->AddDescriptionLine("priority 1");
	optionS->SetMethod(methodForTest);
	optionS->SetParameterRequired(true);
	optionS->SetParameterDescription(CommandLineOption::sParameterString);
	optionS->SetRepetitionAllowed(true);
	optionS->SetPriority(1);
	options.AddOption(optionS);

	options.ParseMainParameters(argc, argv);
	return true;
}

///////////////////////////////////////
// Implementation de la classe CommandLineOption
CommandLineOption::CommandLineOption()
{
	method = NULL;
	cFlag = ' ';
	sParameterDescription = "";
	bParameterRequired = false;
	bIsSingle = false;
	bRepetitionAlowed = false;
	nGroup = 0;
	nPriority = INT_MAX;
	bIsUsed = false;
	bIsFinal = false;
}

CommandLineOption::~CommandLineOption()
{
	method = NULL;
}

CommandLineOption* CommandLineOption::Clone() const
{
	CommandLineOption* oClone;

	oClone = new CommandLineOption;
	oClone->CopyFrom(this);
	return oClone;
}

void CommandLineOption::CopyFrom(const CommandLineOption* optionSource)
{
	require(optionSource != NULL);

	method = optionSource->method;
	cFlag = optionSource->cFlag;
	svDescription.CopyFrom(&(optionSource->svDescription));
	sParameterDescription = optionSource->sParameterDescription;
	bParameterRequired = optionSource->bParameterRequired;
	bIsSingle = optionSource->bIsSingle;
	bRepetitionAlowed = optionSource->bRepetitionAlowed;
	nGroup = optionSource->nGroup;
	nPriority = optionSource->nPriority;
	bIsUsed = optionSource->bIsUsed;
	bIsFinal = optionSource->bIsFinal;
}

void CommandLineOption::SetFlag(char c)
{
	cFlag = c;
}

char CommandLineOption::GetFlag() const
{
	return cFlag;
}

void CommandLineOption::AddDescriptionLine(const ALString& sDescription)
{
	svDescription.Add(sDescription);
}

const StringVector* CommandLineOption::GetDescriptionLines() const
{
	return &svDescription;
}

void CommandLineOption::SetMethod(OptionMethod fMethod)
{
	assert(fMethod != NULL);
	assert(method == NULL);
	method = fMethod;
}

OptionMethod CommandLineOption::GetMethod() const
{
	return method;
}

void CommandLineOption::SetParameterRequired(boolean bParameter)
{
	bParameterRequired = bParameter;
}

boolean CommandLineOption::GetParameterRequired() const
{
	return bParameterRequired;
}

void CommandLineOption::SetParameterDescription(const ALString& sParameter)
{
	sParameterDescription = sParameter;
}

const ALString& CommandLineOption::GetParameterDescription() const
{
	return sParameterDescription;
}

void CommandLineOption::SetSingle(boolean bIsSingleValue)
{
	bIsSingle = bIsSingleValue;
}

boolean CommandLineOption::GetSingle() const
{
	return bIsSingle;
}

void CommandLineOption::SetRepetitionAllowed(boolean bRepeat)
{
	bRepetitionAlowed = bRepeat;
}
boolean CommandLineOption::GetRepetitionAllowed()
{
	return bRepetitionAlowed;
}

void CommandLineOption::SetFinal(boolean bFinal)
{
	bIsFinal = bFinal;
}

boolean CommandLineOption::GetFinal() const
{
	return bIsFinal;
}

void CommandLineOption::SetGroup(int nIndex)
{
	nGroup = nIndex;
}

int CommandLineOption::GetGroup() const
{
	return nGroup;
}

void CommandLineOption::SetPriority(int nIndex)
{
	nPriority = nIndex;
}

int CommandLineOption::GetPriority() const
{
	return nPriority;
}

void CommandLineOption::SetUsageIndex(int nIndex)
{
	nIndexUsage = nIndex;
}

int CommandLineOption::GetUsageIndex() const
{
	return nIndexUsage;
}

int CommandLineOptionCompareForUsage(const void* elem1, const void* elem2)
{
	int nRet;
	CommandLineOption* option1;
	CommandLineOption* option2;

	option1 = cast(CommandLineOption*, *(Object**)elem1);
	option2 = cast(CommandLineOption*, *(Object**)elem2);
	nRet = option1->GetGroup() - option2->GetGroup();
	if (nRet == 0)
		nRet = option1->GetUsageIndex() - option2->GetUsageIndex();
	return nRet;
}

int CommandLineOptionComparePriority(const void* elem1, const void* elem2)
{
	return cast(CommandLineOption*, *(Object**)elem1)->GetPriority() -
	       cast(CommandLineOption*, *(Object**)elem2)->GetPriority();
}

void CommandLineOption::SetUsed(boolean bValue)
{
	bIsUsed = bValue;
}

bool CommandLineOption::GetUsed() const
{
	return bIsUsed;
}

const StringVector* CommandLineOption::GetParameters()
{
	return &svParameters;
}