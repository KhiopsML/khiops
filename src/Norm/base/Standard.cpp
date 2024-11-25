// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Standard.h"

/////////////////////////////////////////////////////////////////////////////
//            Implementation des fonctions d'acquisition                   //
/////////////////////////////////////////////////////////////////////////////

const char* const SecondsToString(double dValue)
{
	char* sBuffer = StandardGetBuffer();
	int nTime;
	int nHours;
	int nMinutes;
	int nSeconds;
	int nHundredth;

	require(dValue >= 0);

	// Calcul des heures, minutes et secondes
	nTime = (int)floor(dValue);
	nHours = nTime / 3600;
	nMinutes = (nTime - nHours * 3600) / 60;
	nSeconds = nTime - nHours * 3600 - nMinutes * 60;
	nHundredth = (int)floor(100 * (dValue - nTime));
	assert(0 <= nMinutes and nMinutes < 60);
	assert(0 <= nSeconds and nSeconds < 60);
	assert(0 <= nHundredth and nHundredth < 100);

	// Formatage du resultat
	if (nHundredth == 0)
		snprintf(sBuffer, BUFFER_LENGTH, "%d:%2.2d:%2.2d", nHours, nMinutes, nSeconds);
	else
		snprintf(sBuffer, BUFFER_LENGTH, "%d:%2.2d:%2.2d.%2.2d", nHours, nMinutes, nSeconds, nHundredth);

	return sBuffer;
}

const char* const IntToString(int nValue)
{
	char* sBuffer = StandardGetBuffer();
	snprintf(sBuffer, BUFFER_LENGTH, "%d", nValue);

	return sBuffer;
}

const char* const LongintToString(longint lValue)
{
	char* sBuffer = StandardGetBuffer();
	snprintf(sBuffer, BUFFER_LENGTH, "%lld", lValue);

	return sBuffer;
}

const char* const DoubleToString(double dValue)
{
	char* sBuffer = StandardGetBuffer();

	snprintf(sBuffer, BUFFER_LENGTH, "%g", dValue);

	return sBuffer;
}

const char* const CharToString(char cValue)
{
	char* sBuffer = StandardGetBuffer();
	char cSpecialChar;

	// Recherche si on est le cas d'un caractere special
	cSpecialChar = '\0';
	if (cValue == '\0')
		cSpecialChar = '0';
	else if (cValue == '\a')
		cSpecialChar = 'a';
	else if (cValue == '\b')
		cSpecialChar = 'b';
	else if (cValue == '\f')
		cSpecialChar = 'f';
	else if (cValue == '\n')
		cSpecialChar = 'n';
	else if (cValue == '\r')
		cSpecialChar = 'r';
	else if (cValue == '\t')
		cSpecialChar = 't';
	else if (cValue == '\v')
		cSpecialChar = 'v';

	// Cas standard
	if (cSpecialChar == '\0')
	{
		sBuffer[0] = cValue;
		sBuffer[1] = '\0';
	}
	// Cas d'un caractere special
	else
	{
		sBuffer[0] = '\\';
		sBuffer[1] = cSpecialChar;
		sBuffer[2] = '\0';
	}

	return sBuffer;
}

const char* const BooleanToString(boolean bValue)
{
	char* sBuffer = StandardGetBuffer();

	if (bValue)
		snprintf(sBuffer, BUFFER_LENGTH, "%s", "true");
	else
		snprintf(sBuffer, BUFFER_LENGTH, "%s", "false");

	return sBuffer;
}

const char* const PointerToString(const void* pValue)
{
	char* sBuffer = StandardGetBuffer();
	snprintf(sBuffer, BUFFER_LENGTH, "%p", pValue);

	return sBuffer;
}

const char* const CharsToString(const char* sValue)
{
	char* sBuffer = StandardGetBuffer();

	require(strlen(sValue) < BUFFER_LENGTH);
	p_strcpy(sBuffer, sValue);

	return sBuffer;
}

int StringToInt(const char* sValue)
{
	require(sValue != NULL);

	return atoi(sValue);
}

longint StringToLongint(const char* sValue)
{
	require(sValue != NULL);

	return atoll(sValue);
}

double StringToDouble(const char* sValue)
{
	int i;
	char* sBuffer = StandardGetBuffer();
	const char* sSource;

	require(sValue != NULL);

	// Recopie dans un buffer, en changeant les
	// ',' en '.'
	i = 0;
	sSource = sValue;
	while (sSource[i] != '\0' and i < BUFFER_LENGTH)
	{
		if (sSource[i] == ',')
			sBuffer[i] = '.';
		else
			sBuffer[i] = sSource[i];
		i++;
	}
	sBuffer[i] = '\0';

	return atof(sBuffer);
}

char StringToChar(const char* sValue)
{
	char cChar;
	char cSpecialChar;

	require(sValue != NULL);

	// On prend le premier caractere
	cChar = sValue[0];

	// Cas d'un caractere special
	if (cChar == '\\' and sValue[1] != '\0')
	{
		cSpecialChar = sValue[1];
		if (cSpecialChar == '0')
			cChar = '\0';
		else if (cSpecialChar == 'a')
			cChar = '\a';
		else if (cSpecialChar == 'b')
			cChar = '\b';
		else if (cSpecialChar == 'f')
			cChar = '\f';
		else if (cSpecialChar == 'n')
			cChar = '\n';
		else if (cSpecialChar == 'r')
			cChar = '\r';
		else if (cSpecialChar == 't')
			cChar = '\t';
		else if (cSpecialChar == 'v')
			cChar = '\v';
	}
	return cChar;
}

boolean StringToBoolean(const char* sValue)
{
	char sTrueValue[] = "true";
	const char* sSource;
	// portage Unix ajout de unsigned
	unsigned int i;

	// Comparaison sans tenir compte des majuscules/minuscules
	i = 0;
	sSource = sValue;
	while (sSource[i] != '\0')
	{
		if (sTrueValue[i] == '\0')
			return false;
		if (tolower(sSource[i]) != sTrueValue[i])
			return false;
		i++;
	}
	return i >= sizeof(sTrueValue) - 1;
}

boolean AcquireBoolean(const char* const sLabel, boolean bDefaultValue)
{
	char cResult;
	char cDefaultValue;
	boolean bResult;
	char sBuffer[BUFFER_LENGTH];

	// Codage de la valeur par defaut sous-forme de caractere
	if (bDefaultValue)
		cDefaultValue = 'y';
	else
		cDefaultValue = 'n';

	// Acquisition d'une valeur booleenne
	cout << sLabel << " (y/n) [" << cDefaultValue << "]:" << flush;
	if (GetAcquireBatchMode())
	{
		bResult = bDefaultValue;
		cout << endl;
	}
	else
	{
		StandardGetInputString(sBuffer, stdin);
		if (sBuffer[0] == '\n')
			bResult = bDefaultValue;
		else
		{
			if (sscanf(sBuffer, "%c", &cResult) == 0)
				bResult = bDefaultValue;
			else
				bResult = (cResult == 'y');
		}
	}
	return bResult;
}

int AcquireInt(const char* const sLabel, int nDefaultValue)
{
	int nResult;
	char sBuffer[BUFFER_LENGTH];

	cout << sLabel << " [" << nDefaultValue << "]:" << flush;
	if (GetAcquireBatchMode())
	{
		nResult = nDefaultValue;
		cout << endl;
	}
	else
	{
		StandardGetInputString(sBuffer, stdin);
		if (sBuffer[0] == '\n')
			nResult = nDefaultValue;
		else
			nResult = atoi(sBuffer);
	}
	return nResult;
}

int AcquireRangedInt(const char* const sLabel, int nMin, int nMax, int nDefaultValue)
{
	int nResult = 0;
	boolean bOk;
	char sBuffer[BUFFER_LENGTH];

	require(nMin <= nMax);
	require(nMin <= nDefaultValue and nDefaultValue <= nMax);

	bOk = false;
	while (not bOk)
	{
		cout << sLabel << " (" << nMin << " to " << nMax << ") [" << nDefaultValue << "]:" << flush;
		if (GetAcquireBatchMode())
		{
			nResult = nDefaultValue;
			cout << endl;
		}
		else
		{
			StandardGetInputString(sBuffer, stdin);
			if (sBuffer[0] == '\n')
				nResult = nDefaultValue;
			else
				nResult = atoi(sBuffer);
		}
		bOk = nMin <= nResult and nResult <= nMax;
	}
	return nResult;
}

double AcquireDouble(const char* const sLabel, double dDefaultValue)
{
	double dResult;
	char sBuffer[BUFFER_LENGTH];

	cout << sLabel << " [" << dDefaultValue << "]:" << flush;
	if (GetAcquireBatchMode())
	{
		dResult = dDefaultValue;
		cout << endl;
	}
	else
	{
		StandardGetInputString(sBuffer, stdin);
		if (sBuffer[0] == '\n')
			dResult = dDefaultValue;
		else
			dResult = atof(sBuffer);
	}
	return dResult;
}

double AcquireRangedDouble(const char* const sLabel, double dMin, double dMax, double dDefaultValue)
{
	double dResult = 0;
	boolean bOk;
	char sBuffer[BUFFER_LENGTH];

	require(dMin <= dMax);
	require(dMin <= dDefaultValue and dDefaultValue <= dMax);

	bOk = false;
	while (not bOk)
	{
		cout << sLabel << " (" << dMin << " to " << dMax << ") [" << dDefaultValue << "]:" << flush;
		if (GetAcquireBatchMode())
		{
			dResult = dDefaultValue;
			cout << endl;
		}
		else
		{
			StandardGetInputString(sBuffer, stdin);
			if (sBuffer[0] == '\n')
				dResult = dDefaultValue;
			else
				dResult = atof(sBuffer);
		}
		bOk = dMin <= dResult and dResult <= dMax;
	}
	return dResult;
}

const char* const AcquireString(const char* const sLabel, const char* const sDefaultValue)
{
	return AcquireRangedString(sLabel, 0, BUFFER_LENGTH, sDefaultValue);
}

const char* const AcquireRangedString(const char* const sLabel, int nMinLength, int nMaxLength,
				      const char* const sDefaultValue)
{
	char* sBuffer = StandardGetBuffer();
	boolean bOk;
	int nBufferLength;

	require(nMinLength <= nMaxLength);
	require(nMinLength <= (int)strlen(sDefaultValue) and (int) strlen(sDefaultValue) <= nMaxLength);

	bOk = false;
	while (not bOk)
	{
		cout << sLabel << " [" << sDefaultValue << "]:" << flush;
		if (GetAcquireBatchMode())
		{
			p_strcpy(sBuffer, sDefaultValue);
			cout << endl;
		}
		else
		{
			StandardGetInputString(sBuffer, stdin);
			if (sBuffer[0] == '\n')
				p_strcpy(sBuffer, sDefaultValue);
			else
			{
				// Suppression du '\n' de la reponse interactive
				for (int nI = 0; nI < BUFFER_LENGTH; nI++)
					if (sBuffer[nI] == '\n')
						sBuffer[nI] = '\0';
			}
		}

		// Verification de la longueur max
		nBufferLength = (int)strlen(sBuffer);
		if (nBufferLength > nMaxLength)
			sBuffer[nMaxLength] = '\0';

		// Verification de la longueur min
		bOk = nBufferLength >= nMinLength;
		if (not bOk)
			cout << sLabel << "(size >= :" << nMinLength << "):" << endl;
	}
	return sBuffer;
}

longint AcquireLongint(const char* const sLabel, longint lDefaultValue)
{
	longint lResult = 0;
	char sBuffer[BUFFER_LENGTH];

	cout << sLabel << " [" << lDefaultValue << "]:" << flush;
	if (GetAcquireBatchMode())
	{
		lResult = lDefaultValue;
		cout << endl;
	}
	else
	{
		StandardGetInputString(sBuffer, stdin);
		if (sBuffer[0] == '\n')
			lResult = lDefaultValue;
		else
			lResult = atoll(sBuffer);
	}
	return lResult;
}

longint AcquireRangedLongint(const char* const sLabel, longint lMin, longint lMax, longint lDefaultValue)
{
	longint lResult = 0;
	boolean bOk;
	char sBuffer[BUFFER_LENGTH];

	require(lMin <= lMax);
	require(lMin <= lDefaultValue and lDefaultValue <= lMax);

	bOk = false;
	while (not bOk)
	{
		cout << sLabel << " (" << lMin << " to " << lMax << ") [" << lDefaultValue << "]:" << flush;
		if (GetAcquireBatchMode())
		{
			lResult = lDefaultValue;
			cout << endl;
		}
		else
		{
			StandardGetInputString(sBuffer, stdin);
			if (sBuffer[0] == '\n')
				lResult = lDefaultValue;
			else
				lResult = atoll(sBuffer);
		}
		bOk = lMin <= lResult and lResult <= lMax;
	}
	return lResult;
}

boolean bAcquireBatchMode = false;

void SetAcquireBatchMode(boolean bValue)
{
	bAcquireBatchMode = bValue;
}

boolean GetAcquireBatchMode()
{
	return bAcquireBatchMode;
}

/////////////////////////////////////
// Generateur de nombre aleatoires //
/////////////////////////////////////

static int nRandomSeed = 1;

void SetRandomSeed(int nSeed)
{
	nRandomSeed = nSeed;
}

int GetRandomSeed()
{
	return nRandomSeed;
}

// Reference: Numerical recipes: the art of scientific computing
// Chapter 7: Random numbers, p 270
// Generateur de nombres aleatoires avec periodicite de 2 ^31(environ deux milliards)
// Clairement plus interessant que la methode standard rand(), limite a 2^15 valeurs(environ 32000)
double RandomDouble()
{
	int k;
	double ans;

	static const int IA = 16807, IM = 2147483647, IQ = 127773, IR = 2836, MASK = 123459876;
	static const double AM = (1.0 / (IM));

	nRandomSeed ^= MASK;
	k = nRandomSeed / IQ;
	nRandomSeed = IA * (nRandomSeed - k * IQ) - IR * k;
	if (nRandomSeed < 0)
		nRandomSeed += IM;
	ans = AM * ((double)nRandomSeed);
	nRandomSeed ^= MASK;
	return ans;
}

int RandomInt(int nMax)
{
	double dRandom;
	int nRand;

	require(nMax >= 0);

	dRandom = RandomDouble();
	if (dRandom == 1)
		nRand = nMax;
	else
		nRand = (int)floor((double(nMax) + 1) * dRandom);
	return nRand;
}

double IthRandomDouble(longint lIndex)
{
	require(lIndex >= 0);
	// Version de Numerical Recipes
	return 5.42101086242752217E-20 * IthRandomUnsignedLongint((unsigned long long int)lIndex);
}

longint IthRandomLongint(longint lIndex)
{
	require(lIndex >= 0);
	return (longint)(IthRandomUnsignedLongint((unsigned long long int)lIndex));
}

int IthRandomInt(longint lIndex, int nMax)
{
	require(lIndex >= 0);
	require(nMax >= 0);
	return (int)(IthRandomUnsignedLongint((unsigned long long int)lIndex) % ((unsigned long long int)nMax + 1));
}

/////////////////////////////////////////////////////////////////////////////
//            Implementation de la gestion des contrats
//
// Attention: on utilise ici l'instruction printf(plutot que cout)
// En effet, en cas d'erreur fatale, il eviter l'utilisation d'objet
// qui peuvent entrainer d'autres erreurs(notament si
// probleme memoire)

//:: _AssertionFailure()
void _AssertionFailure(const char* __cond, const char* __file, const unsigned __line)
{
	if (GetProcessId() == 0)
		fprintf(stdout, "Assert failed in file %s line %ud\t%s\n", __file, __line, __cond);
	else
		fprintf(stdout, "Process %d: Assert failed in file %s line %ud\t%s\n", GetProcessId(), __file, __line,
			__cond);
	GlobalExit();
}

//:: _RequireFailure()
void _RequireFailure(const char* __cond, const char* __file, const unsigned __line)
{
	if (GetProcessId() == 0)
		fprintf(stdout, "Require failed in file %s line %ud\t%s\n", __file, __line, __cond);
	else
		fprintf(stdout, "Process %d: Require failed in file %s line %ud\t%s\n", GetProcessId(), __file, __line,
			__cond);
	GlobalExit();
}

//:: _EnsureFailure()
void _EnsureFailure(const char* __cond, const char* __file, const unsigned __line)
{
	if (GetProcessId() == 0)
		fprintf(stdout, "Ensure failed in file %s line %ud\t%s\n", __file, __line, __cond);
	else
		fprintf(stdout, "Process %d: Ensure failed in file %s line %ud\t%s\n", GetProcessId(), __file, __line,
			__cond);
	GlobalExit();
}

//:: _CheckFailure()
void _CheckFailure(const char* __cond, const char* __file, const unsigned __line)
{
	if (GetProcessId() == 0)
		fprintf(stdout, "Check failed in file %s line %ud\t%s\n", __file, __line, __cond);
	else
		fprintf(stdout, "Process %d: Check failed in file %s line %ud\t%s\n", GetProcessId(), __file, __line,
			__cond);
	GlobalExit();
}

const Object* objectCastControlBuffer = NULL;

//:: _CastFailure()
void _CastFailure(const char* __type, const char* __object, const char* __file, const unsigned __line)
{
	if (GetProcessId() == 0)
		fprintf(stdout, "Cast failed in file %s line %ud\tcast(%s, %s)\n", __file, __line, __type, __object);
	else
		fprintf(stdout, "Process %d: Cast failed in file %s line %ud\tcast(%s, %s)\n", GetProcessId(), __file,
			__line, __type, __object);
	GlobalExit();
}

// Flag de sortie utilisateur
//(pour conditionner les traitements enregistres en atexit)
int nStandardGlobalExit = 0;

// Tableau de methodes de sortie utilisateur
const int nMaxUserExitHandlerNumber = 10;
static int nUserExitHandlerNumber = 0;
static UserExitHandler arrayUserExitHandler[nMaxUserExitHandlerNumber];

// Code de retour de l'application
static int nExitCode = EXIT_FAILURE;

void GlobalExitOnSuccess()
{
	nExitCode = EXIT_SUCCESS;
	GlobalExit();
}

//:: GlobalExit()
//:: Insert break point on exit() instruction
//:: to get call stack with a debugger tool
void GlobalExit()
{
	int i;
	UserExitHandler fUserExitHandler;

	if (nStandardGlobalExit == 0)
	{
		// Flag de sortie brutale
		nStandardGlobalExit = 1;

		// Flush des streams standard C et C++
		// Permet de forcer la sotie standard, en particulier pour les assertions
		fflush(stdout);
		fflush(stderr);
		cout << flush;
		cerr << flush;

		// Appel des methodes de sortie utilisateur
		// On execute les dernieres methodes enregistrees en premier
		// Attention a appeller ces methodes avant les flushall et closeall
		// pour pouvoir continuer a exploiter les fichiers de log ou trace
		for (i = nUserExitHandlerNumber - 1; i >= 0; i--)
		{
			fUserExitHandler = arrayUserExitHandler[i];
			assert(fUserExitHandler != NULL);
			fUserExitHandler(nExitCode);
		}

		// Flush et fermeture de tous les fichiers
#ifdef _WIN32
		_flushall();
		_fcloseall();
#else
		fflush(NULL);
#endif

		// Sortie fatale (seul exit de tout le code)
		// les librairies NORM)
		exit(nExitCode);
	}
}

void AddUserExitHandler(UserExitHandler fUserExit)
{
	int i;
	boolean bDebug = false;
	require(fUserExit != NULL);
	require(nUserExitHandlerNumber < nMaxUserExitHandlerNumber);

	// On verifie que la fonction n'a pas deja ete ajoutee
	debug(bDebug = true);
	if (bDebug)
	{
		for (i = nUserExitHandlerNumber - 1; i >= 0; i--)
			require(fUserExit != arrayUserExitHandler[i]);
	}
	arrayUserExitHandler[nUserExitHandlerNumber] = fUserExit;
	nUserExitHandlerNumber++;
}

// Identifiant du process
static int nProcessId = 0;

int GetProcessId()
{
	return nProcessId;
}

void SetProcessId(int nValue)
{
	require(nValue >= 0);
	nProcessId = nValue;
}

void TraceMaster(const char* sTrace)
{
	if (GetProcessId() == 0)
		TraceWithRank(sTrace);
}
void TraceSlave(const char* sTrace)
{
	if (GetProcessId() != 0)
		TraceWithRank(sTrace);
}
void TraceWithRank(const char* sTrace)
{
	cout << "[" << GetProcessId() << "] " << sTrace << endl;
}

void TestRandom()
{
	int nStartClock;
	int nStopClock;
	int nNumber = 100000000;
	int i;
	longint lRandomMean;
	double dMeanRandom;

	SetRandomSeed(1);

	// Random int standard
	nStartClock = clock();
	lRandomMean = 0;
	for (i = 0; i < nNumber; i++)
	{
		lRandomMean += RandomInt(1000000000);
	}
	dMeanRandom = lRandomMean * 1.0 / nNumber;
	nStopClock = clock();
	cout << "Standard random int mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard random int\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Random double standard
	nStartClock = clock();
	dMeanRandom = 0;
	for (i = 0; i < nNumber; i++)
	{
		dMeanRandom += RandomDouble();
	}
	dMeanRandom = dMeanRandom / nNumber;
	nStopClock = clock();
	cout << "Standard random double mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard random double\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Ith random int
	nStartClock = clock();
	lRandomMean = 0;
	for (i = 0; i < nNumber; i++)
	{
		lRandomMean += IthRandomInt(i, 1000000000);
	}
	dMeanRandom = lRandomMean * 1.0 / nNumber;
	nStopClock = clock();
	cout << "Standard ith random int mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard ith random int\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Ith random longint
	nStartClock = clock();
	lRandomMean = 0;
	for (i = 0; i < nNumber; i++)
	{
		lRandomMean += abs(IthRandomLongint(i)) % 1000000000;
	}
	lRandomMean /= nNumber;
	nStopClock = clock();
	cout << "Standard ith random longint mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard ith random longint\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC
	     << "\n\n";

	// Ith random double
	nStartClock = clock();
	dMeanRandom = 0;
	for (i = 0; i < nNumber; i++)
	{
		dMeanRandom += IthRandomDouble(i);
	}
	dMeanRandom /= nNumber;
	nStopClock = clock();
	cout << "Standard ith random double mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard ith random double\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";
}
