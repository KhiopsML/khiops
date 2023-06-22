// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Ermgt.h"

///////////////////////////////////////
// Implementation de la classe Global

void Global::AddSimpleMessage(const ALString& sLabelValue)
{
	AddErrorObjectValued(Error::GravityMessage, "", "", sLabelValue);
}

void Global::AddMessage(const ALString& sCategoryValue, const ALString& sLocalisationValue, const ALString& sLabelValue)
{
	AddErrorObjectValued(Error::GravityMessage, sCategoryValue, sLocalisationValue, sLabelValue);
}

void Global::AddWarning(const ALString& sCategoryValue, const ALString& sLocalisationValue, const ALString& sLabelValue)
{
	AddErrorObjectValued(Error::GravityWarning, sCategoryValue, sLocalisationValue, sLabelValue);
}

void Global::AddError(const ALString& sCategoryValue, const ALString& sLocalisationValue, const ALString& sLabelValue)
{
	AddErrorObjectValued(Error::GravityError, sCategoryValue, sLocalisationValue, sLabelValue);
}

void Global::AddFatalError(const ALString& sCategoryValue, const ALString& sLocalisationValue,
			   const ALString& sLabelValue)
{
	AddErrorObjectValued(Error::GravityFatalError, sCategoryValue, sLocalisationValue, sLabelValue);
}

Global::Global()
{
	static boolean bSignalHandlersInitialized = false;

	// Initialisation des handler de gestion de signal
	// On ne gere pas SIGBREAK, qui n'est pas connu sous linux
	if (not bSignalHandlersInitialized)
	{
		bSignalHandlersInitialized = true;
		signal(SIGTERM, SignalHandler);
		signal(SIGSEGV, SignalHandler);
		signal(SIGINT, SignalHandler);
		signal(SIGILL, SignalHandler);
		signal(SIGABRT, SignalHandler);
		signal(SIGFPE, SignalHandler);
	}
}

Global::~Global() {}

void Global::SignalHandler(int nSigNum)
{
	if (nSigNum == SIGTERM)
		AddFatalError("Signal", IntToString(nSigNum), "termination request, sent to the program ");
	else if (nSigNum == SIGSEGV)
		AddFatalError("Signal", IntToString(nSigNum), "invalid memory access (segmentation fault)");
	else if (nSigNum == SIGINT)
		AddFatalError("Signal", IntToString(nSigNum), "external interrupt, usually initiated by the user");
	else if (nSigNum == SIGILL)
		AddFatalError("Signal", IntToString(nSigNum), "invalid program image, such as invalid instruction");
	else if (nSigNum == SIGABRT)
		AddFatalError("Signal", IntToString(nSigNum), "abnormal termination condition triggered by abort call");
	else if (nSigNum == SIGFPE)
		AddFatalError("Signal", IntToString(nSigNum), "erroneous arithmetic operation such as divide by zero");
	else
		AddFatalError("Signal", IntToString(nSigNum), "Unknown error");
}

Global Global::singletonSignalManager;

void Global::AddErrorObject(const Error* eValue)
{
	AddErrorObjectValued(eValue->GetGravity(), eValue->GetCategory(), eValue->GetLocalisation(),
			     eValue->GetLabel());
}

void Global::AddErrorObjectValued(int nGravityValue, const ALString& sCategoryValue, const ALString& sLocalisationValue,
				  const ALString& sLabelValue)
{
	Error e;
	longint lErrorFlowNumber;
	ALString sAdditionalInfo;

	// Initialisation d'un objet erreur
	e.Initialize(nGravityValue, sCategoryValue, sLocalisationValue, sLabelValue);

	// Sortie si erreur fatale
	if (nGravityValue == Error::GravityFatalError)
	{
		ShowError(e);
		GlobalExit();
	}
	// Cas des autres types de message
	else
	{
		// Nombre de messages selon le type
		lErrorFlowNumber = 0;
		if (nGravityValue == Error::GravityMessage)
		{
			lErrorFlowMessageNumber++;
			lErrorFlowNumber = lErrorFlowMessageNumber;
		}
		else if (nGravityValue == Error::GravityWarning)
		{
			lErrorFlowWarningNumber++;
			lErrorFlowNumber = lErrorFlowWarningNumber;
		}
		else if (nGravityValue == Error::GravityError)
		{
			lErrorFlowErrorNumber++;
			lErrorFlowNumber = lErrorFlowErrorNumber;
		}

		// Affichage si autorise
		if (nErrorFlowControlLevel == 0 or lErrorFlowNumber <= nMaxErrorFlowNumber)
			ShowError(e);

		// Affichage special si depassement du nombre max autorise
		if (nErrorFlowControlLevel > 0 and lErrorFlowNumber > nMaxErrorFlowNumber)
		{
			// Cas ou on vient juste de depasser le nombre de messages
			if (lErrorFlowNumber == nMaxErrorFlowNumber + 1)
			{
				e.Initialize(nGravityValue, sCategoryValue, "", "...");
				ShowError(e);
			}
			// Sinon, on re-affiche un message toutes les puissances de 10
			// ou tous les millions
			else if (lErrorFlowNumber == 10 or lErrorFlowNumber == 100 or lErrorFlowNumber == 1000 or
				 lErrorFlowNumber == 10000 or lErrorFlowNumber == 100000 or
				 lErrorFlowNumber % 1000000 == 0)
			{
				// Message avec info du nombre d'occurences affiches)
				sAdditionalInfo = sAdditionalInfo + "(" + LongintToReadableString(lErrorFlowNumber) +
						  "th " + Error::GetGravityLabel(nGravityValue) + ")";
				e.Initialize(nGravityValue, sCategoryValue, sLocalisationValue,
					     sLabelValue + " " + sAdditionalInfo);
				ShowError(e);

				// Message de suite
				e.Initialize(nGravityValue, sCategoryValue, "", "...");
				ShowError(e);
			}
		}
	}
}

void Global::ShowError(Error e)
{
	if (not GetSilentMode() or e.GetGravity() == Error::GravityFatalError)
	{
		// Ecriture dans le fichier de log
		if (fstError.is_open())
			fstError << e << flush;

		// Affichage avec interface utilisateur
		e.Display();
	}
}

void Global::ActivateErrorFlowControl()
{
	require(nErrorFlowControlLevel >= 0);
	if (nErrorFlowControlLevel == 0)
	{
		lErrorFlowMessageNumber = 0;
		lErrorFlowWarningNumber = 0;
		lErrorFlowErrorNumber = 0;
	}
	nErrorFlowControlLevel++;
}

void Global::DesactivateErrorFlowControl()
{
	require(nErrorFlowControlLevel > 0);
	nErrorFlowControlLevel--;
	if (nErrorFlowControlLevel == 0)
	{
		lErrorFlowMessageNumber = 0;
		lErrorFlowWarningNumber = 0;
		lErrorFlowErrorNumber = 0;
	}
}

void Global::SetMaxErrorFlowNumber(int nValue)
{
	require(nValue >= 0);

	nMaxErrorFlowNumber = nValue;
}

int Global::GetMaxErrorFlowNumber()
{
	return nMaxErrorFlowNumber;
}

boolean Global::IsMaxErrorFlowReached()
{
	return lErrorFlowMessageNumber >= nMaxErrorFlowNumber and lErrorFlowWarningNumber >= nMaxErrorFlowNumber and
	       lErrorFlowErrorNumber >= nMaxErrorFlowNumber;
}

boolean Global::IsMaxErrorFlowReachedPerGravity(int nErrorGravity)
{
	require(nErrorGravity == Error::GravityMessage or nErrorGravity == Error::GravityWarning or
		nErrorGravity == Error::GravityError or nErrorGravity == Error::GravityFatalError);

	if (nErrorGravity == Error::GravityMessage)
		return lErrorFlowMessageNumber >= nMaxErrorFlowNumber;
	else if (nErrorGravity == Error::GravityWarning)
		return lErrorFlowWarningNumber >= nMaxErrorFlowNumber;
	else if (nErrorGravity == Error::GravityError)
		return lErrorFlowErrorNumber >= nMaxErrorFlowNumber;
	else
		return false;
}

void Global::SetSilentMode(boolean bValue)
{
	bSilentMode = bValue;
}

boolean Global::GetSilentMode()
{
	return bSilentMode;
}

// Les erreurs de l'allocateur sont redirigees sur la gestion centralisee des erreurs
static int GlobalMemSetAllocErrorHandler()
{
	MemSetAllocErrorHandler(AllocErrorDisplayMessageFunction);
	return 1;
}

// On utilise une initialisation statique pour forcer le parametrage des messages de l'allocateur au plus tot
// Cette initialisation ne peut se faire depuis l'allocateur, qui ne connait pas la classe Global
static int GlobalAllocErrorHandler = GlobalMemSetAllocErrorHandler();

boolean Global::SetErrorLogFileName(const ALString& sValue)
{
	boolean bOk = true;

	// Fermeture si necessaire du fichier en cours
	if (fstError.is_open())
		fstError.close();

	// Ouverture du fichier si necessaire
	sErrorLogFileName = sValue;
	if (sErrorLogFileName != "")
	{
		// Creation si necessaire des repertoires intermediaires
		FileService::MakeDirectories(FileService::GetPathName(sErrorLogFileName));

		// Ici, on ne passe pas par la classe FileService pour ne pas
		// entrainer une boucle entre FileService et Global
		p_SetMachineLocale();
		fstError.open(sErrorLogFileName, ios::out);
		p_SetApplicationLocale();

		// Message d'erreur si probleme d'ouverture
		if (not fstError.is_open())
		{
			AddError("File", sErrorLogFileName, "Unable to open log file");
			bOk = false;
		}
	}
	return bOk;
}

const ALString Global::GetErrorLogFileName()
{
	return sErrorLogFileName;
}

int Global::nErrorFlowControlLevel = 0;

int Global::nMaxErrorFlowNumber = 20;

longint Global::lErrorFlowMessageNumber = 0;
longint Global::lErrorFlowWarningNumber = 0;
longint Global::lErrorFlowErrorNumber = 0;

boolean Global::bSilentMode = false;

ALString Global::sErrorLogFileName;

fstream Global::fstError;

//////////////////////////////////////////////////////////////
//            Implementation de la classe Error

DisplayErrorFunction Error::fDisplayErrorFunction = ErrorDefaultDisplayErrorFunction;

void Error::Initialize(int nGravityValue, const ALString& sCategoryValue, const ALString& sLocalisationValue,
		       const ALString& sLabelValue)
{
	SetGravity(nGravityValue);
	SetCategory(sCategoryValue);
	SetLocalisation(sLocalisationValue);
	SetLabel(sLabelValue);
}

void Error::CopyFrom(const Error* eSource)
{
	require(eSource != NULL);
	SetGravity(eSource->GetGravity());
	SetCategory(eSource->GetCategory());
	SetLocalisation(eSource->GetLocalisation());
	SetLabel(eSource->GetLabel());
}

Error* Error::Clone() const
{
	Error* eClone;
	eClone = new Error;
	eClone->CopyFrom(this);
	return eClone;
}

void Error::Display() const
{
	static boolean bPendingFatalError = false;
	ALString sErrorFileName;
	ALString sMemoryDebugMode;
	ALString sDisplayMessage;

	// Si erreur fatal en cours de traitement, on quite pour eviter les boucles infinie
	// ou l'allocation de memoire (plantative) necessaire pour afficher le message
	if (bPendingFatalError)
		GlobalExit();

	// Affichage sur la console si erreur fatale.
	// La fDisplayErrorFunction par defaut affiche sur la console:
	// on evite d'avoir l'erreur affichee deux fois dans la console
	if (GetGravity() == GravityFatalError and fDisplayErrorFunction != ErrorDefaultDisplayErrorFunction)
	{
		bPendingFatalError = true;
		cerr << *this << endl;
	}

	// Affichage de l'erreur
	DisplayError(this);
}

void Error::Write(ostream& ost) const
{
	ALString sDisplayMessage;

	sDisplayMessage = BuildDisplayMessage(this);
	ost << sDisplayMessage << "\n";
}

longint Error::GetUsedMemory() const
{
	return sizeof(Error) + sCategory.GetAllocLength() + sLocalisation.GetAllocLength() + sLabel.GetAllocLength();
}

const ALString Error::GetClassLabel() const
{
	return "Error";
}

ALString Error::GetGravityLabel(int nGravity)
{
	if (nGravity == GravityMessage)
		return "message";
	else if (nGravity == GravityWarning)
		return "warning";
	else if (nGravity == GravityError)
		return "error";
	else if (nGravity == GravityFatalError)
		return "fatal error";
	return "???";
}

const ALString Error::BuildDisplayMessage(const Error* e)
{
	ALString sDisplayMessage;

	require(e != NULL);

	// Gravite sauf dans le cas message
	if (e->GetGravity() != GravityMessage)
	{
		sDisplayMessage = GetGravityLabel(e->GetGravity());
		sDisplayMessage += " : ";
	}

	// On s'adapte a l'absence potentielle de la categorie et de la localisation
	if (e->GetCategory() != "" or e->GetLocalisation() != "")
	{
		sDisplayMessage += e->GetCategory();
		if (e->GetCategory() != "" and e->GetLocalisation() != "")
			sDisplayMessage += " ";
		sDisplayMessage += e->GetLocalisation();
		sDisplayMessage += " : ";
	}

	// Le libelle est toujours la
	sDisplayMessage += e->GetLabel();
	return sDisplayMessage;
}

void Error::DisplayError(const Error* e)
{
	if (fDisplayErrorFunction != NULL)
		fDisplayErrorFunction(e);
}

void Error::SetDisplayErrorFunction(DisplayErrorFunction fDisplayError)
{
	fDisplayErrorFunction = fDisplayError;
}

void ErrorDefaultDisplayErrorFunction(const Error* e)
{
	e->Write(cout);
	cout << flush;
}

void AllocErrorDisplayMessageFunction(const char* sAllocErrorMessage)
{
	static boolean bPendingFatalError = false;
	FILE* fError;
	ALString sErrorFileName;
	ALString sMemoryDebugMode;

	// Si erreur fatal en cours de traitement, on quite pour eviter les boucles infinie
	// ou l'allocation de memoire (plantative) necessaire pour afficher le message
	if (bPendingFatalError)
		GlobalExit();
	bPendingFatalError = true;

	// Affichage de l'etat de la memoire si mode debugage
	sMemoryDebugMode = p_getenv("MemoryDebugMode");
	sMemoryDebugMode.MakeLower();
	if (sMemoryDebugMode == "true")
	{
		// Recherche d'un nom de fichier de trace memoire base sur le fichier de log
		sErrorFileName = Global::GetErrorLogFileName();
		sErrorFileName = FileService::BuildFilePathName(
		    FileService::GetPathName(sErrorFileName),
		    FileService::BuildFileName(FileService::GetFilePrefix(sErrorFileName), "Memory.log"));

		// Message dans un fichier
		cout << "Memory log file: " << sErrorFileName << endl;
		fError = p_fopen(sErrorFileName, "w");
		fprintf(fError, "%s", sAllocErrorMessage);
		MemPrintHeapStats(fError);
		fclose(fError);
	}

	// Redirection sur la fonction d'affichage standard
	Global::AddFatalError("", "", sAllocErrorMessage);
}

DisplayErrorFunction Error::GetDisplayErrorFunction()
{
	return fDisplayErrorFunction;
}

DisplayErrorFunction Error::GetDefaultDisplayErrorFunction()
{
	return ErrorDefaultDisplayErrorFunction;
}

void Error::Test()
{
	Error eTest;
	int nMessageNumber;
	int i;
	ALString sTmp;

	// Parametrages globaux
	Global::SetSilentMode(false);
	Global::SetErrorLogFileName(sTmp + "." + FileService::GetFileSeparator() + "log.txt");

	// Messages elementaires
	eTest.Initialize(Error::GravityMessage, "Test", "First", "This is a message");
	eTest.Display();

	eTest.Initialize(Error::GravityWarning, "Test", "Second", "This is a warning");
	eTest.Display();

	eTest.Initialize(Error::GravityError, "Test", "Third", "This is an error");
	eTest.Display();

	eTest.AddMessage("Message depuis un object Error!");

	Global::AddSimpleMessage("Simple message global");
	Global::AddMessage("Fichier", "Test", "Message global");
	Global::AddWarning("Fichier", "Test", "Warning global");
	Global::AddError("Fichier", "Test", "Error global");

	// Test de performance
	nMessageNumber = AcquireRangedInt("Message number", 0, 10000, 100);
	Global::ActivateErrorFlowControl();
	for (i = 0; i < nMessageNumber; i++)
		Global::AddSimpleMessage("Ceci est un des nombreux messages");
	Global::DesactivateErrorFlowControl();

	// Test de chaque caractere (probleme de conversion potentiels avec UTF8)
	for (i = -128; i < 128; i++)
	{
		sTmp = "Char ";
		sTmp += IntToString(i);
		sTmp += ": ";
		sTmp += char(i);
		sTmp += "\t 543210";
		Global::AddSimpleMessage(sTmp);
	}
}