// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Ermgt.h"

///////////////////////////////////////
// Implementation de la classe Global

boolean Global::bPrintMessagesInConsole = false;

ErrorFlowIgnoreFunction Global::fErrorFlowIgnoreFunction = NULL;

boolean Global::bIsSignalErrorManagementActivated = false;

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
	// Traitement de l'erreur comme un warning
	if (GetErrorAsWarningMode())
		AddWarning(sCategoryValue, sLocalisationValue, sLabelValue);
	// Traitement standard de l'erreur
	else
	{
		bIsAtLeastOneError = true;
		AddErrorObjectValued(Error::GravityError, sCategoryValue, sLocalisationValue, sLabelValue);
	}
}

void Global::AddFatalError(const ALString& sCategoryValue, const ALString& sLocalisationValue,
			   const ALString& sLabelValue)
{
	AddErrorObjectValued(Error::GravityFatalError, sCategoryValue, sLocalisationValue, sLabelValue);
}

boolean Global::IgnoreErrorFlowForDisplay(const Error* e)
{
	require(e != NULL);
	if (fErrorFlowIgnoreFunction == NULL)
		return false;
	else
		return fErrorFlowIgnoreFunction(e, true);
}

void Global::SignalHandler(int nSigNum)
{
	// On doit eviter les allocations et les routines d'entree:sortie de bas niveau ou de sdtio.h
	if (nSigNum == SIGTERM)
		cout << "Interrupt signal " << nSigNum << " : termination request, sent to the program " << endl;
	else if (nSigNum == SIGSEGV)
		cout << "Interrupt signal " << nSigNum << " : invalid memory access (segmentation fault)" << endl;
	else if (nSigNum == SIGINT)
		cout << "Interrupt signal " << nSigNum << " : external interrupt, usually initiated by the user"
		     << endl;
	else if (nSigNum == SIGILL)
		cout << "Interrupt signal " << nSigNum << " : invalid program image, such as invalid instruction"
		     << endl;
	else if (nSigNum == SIGABRT)
		cout << "Interrupt signal " << nSigNum << " : abnormal termination condition triggered by abort call"
		     << endl;
	else if (nSigNum == SIGFPE)
		cout << "Interrupt signal " << nSigNum << " : erroneous arithmetic operation such as divide by zero"
		     << endl;
	else
		cout << "Interrupt signal " << nSigNum << " : Unknown error" << endl;

	// Sortie du programe
	// (on n'utilise pas les signaux comme valeurs de retour car on renvoie 1 en cas d'erreur fatale)
	exit(EXIT_FAILURE);
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
		ShowError(&e);
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
		if (nErrorFlowControlLevel == 0 or lErrorFlowNumber <= nMaxErrorFlowNumber or
		    IgnoreErrorFlowForDisplay(&e))
			ShowError(&e);

		// Affichage special si depassement du nombre max autorise
		if (nErrorFlowControlLevel > 0 and lErrorFlowNumber > nMaxErrorFlowNumber)
		{
			// Cas ou on vient juste de depasser le nombre de messages
			if (lErrorFlowNumber == nMaxErrorFlowNumber + 1)
			{
				e.Initialize(nGravityValue, sCategoryValue, "", "...");
				ShowError(&e);
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
				ShowError(&e);

				// Message de suite
				e.Initialize(nGravityValue, sCategoryValue, "", "...");
				ShowError(&e);
			}
		}
	}
}

void Global::ShowError(const Error* e)
{
	require(e != NULL);

	if (not GetSilentMode() or e->GetGravity() == Error::GravityFatalError)
	{
		// Ecriture dans le fichier de log
		if (fstError.is_open())
		{
			// Si c'est une redirection dans la console, on prefixe le message
			// pour qu'il soit facilement identifiable parmi les autres types de messages
			// (progression, output etc...)
			if (bPrintMessagesInConsole)
				fstError << "Khiops.log\t";
			fstError << *e << flush;
		}

		// Affichage avec interface utilisateur
		e->Display();
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

boolean Global::IgnoreErrorFlow(const Error* e)
{
	require(e != NULL);
	if (fErrorFlowIgnoreFunction == NULL)
		return false;
	else
		return fErrorFlowIgnoreFunction(e, false);
}

void Global::SetErrorFlowIgnoreFunction(ErrorFlowIgnoreFunction fErrorFlowIgnore)
{
	fErrorFlowIgnoreFunction = fErrorFlowIgnore;
}

ErrorFlowIgnoreFunction Global::GetErrorFlowIgnoreFunction()
{
	return fErrorFlowIgnoreFunction;
}

void Global::SetSilentMode(boolean bValue)
{
	bSilentMode = bValue;
}

boolean Global::GetSilentMode()
{
	return bSilentMode;
}

void Global::SetErrorAsWarningMode(boolean bValue)
{
	bErrorAsWarningMode = bValue;
}

boolean Global::GetErrorAsWarningMode()
{
	return bErrorAsWarningMode;
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

		// Fermeture du fichier si celui-ci est deja ouvert
		// Ce cas peut arriver si on appelle plusieurs fois ParseParameters (notamment via MODL_dll)
		if (fstError.is_open())
			fstError.close();

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

		// Si le fichier de log est /dev/stdout ou /dev/stderr
		// C'est une redirection des messages vers la console
		// (on ajoutera un prefix a chaque ligne)
#ifdef __linux_or_apple__
		if (sValue == "/dev/stdout" or sValue == "/dev/stderr")
			bPrintMessagesInConsole = true;
#endif
	}
	return bOk;
}

const ALString Global::GetErrorLogFileName()
{
	return sErrorLogFileName;
}

boolean Global::IsAtLeastOneError()
{
	return bIsAtLeastOneError;
}

void Global::ActivateSignalErrorManagement()
{
	require(not bIsSignalErrorManagementActivated);

// Defini seulement en debug ou RelWithDebInfo (alpha)
#if defined(__ALPHA__) || !defined(NOALL)

	// Initialisation des handler de gestion de signal
	// On ne gere pas SIGBREAK, qui n'est pas connu sous linux
	bIsSignalErrorManagementActivated = true;
	signal(SIGTERM, SignalHandler);
	signal(SIGSEGV, SignalHandler);
	signal(SIGINT, SignalHandler);
	signal(SIGILL, SignalHandler);
	signal(SIGABRT, SignalHandler);
	signal(SIGFPE, SignalHandler);

#endif
}

boolean Global::IsSignalErrorManagementActivated()
{
	return bIsSignalErrorManagementActivated;
}

int Global::nErrorFlowControlLevel = 0;

int Global::nMaxErrorFlowNumber = 20;

longint Global::lErrorFlowMessageNumber = 0;
longint Global::lErrorFlowWarningNumber = 0;
longint Global::lErrorFlowErrorNumber = 0;

boolean Global::bSilentMode = false;

boolean Global::bErrorAsWarningMode = false;

ALString Global::sErrorLogFileName;

fstream Global::fstError;

boolean Global::bIsAtLeastOneError = false;

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
