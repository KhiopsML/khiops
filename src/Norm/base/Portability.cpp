// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Portability.h"
#include "MemoryManager.h"
#include <assert.h>

// Declaration de la methode de Standard.h permettant d'acceder aux buffers de travail
extern char* StandardGetBuffer();

void p_SetMachineLocale()
{
	setlocale(LC_ALL, "");
}

void p_SetApplicationLocale()
{
	setlocale(LC_ALL, "en_US.UTF-8");
}

/////////////////////////////////////////////////////////////////
// Implementation standard pour Linux
#ifdef __linux_or_apple__

const char* p_getenv(const char* varname)
{
	char* sBuffer = StandardGetBuffer();
	char* pEnvVarValue;
	pEnvVarValue = getenv(varname);
	if (pEnvVarValue == NULL)
	{
		p_strcpy(sBuffer, "");
		pEnvVarValue = sBuffer;
	}
	return pEnvVarValue;
}

void* p_hugemalloc(size_t size)
{
	return malloc(size);
}

void p_hugefree(void* memblock)
{
	free(memblock);
}

#endif //  __linux_or_apple__

////////////////////////////////////////////////////
// Reimplementation pour Windows
#ifdef _WIN32

#include <windows.h>

const char* p_getenv(const char* varname)
{
	char* sBuffer = StandardGetBuffer();
	size_t len;
	errno_t err = _dupenv_s(&sBuffer, &len, varname);
	if (err)
		p_strcpy(sBuffer, "");
	return sBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////
// Allocation massive de segments sous Windows
// Les fonctions malloc/free reposent sur plusieus niveaux d'API:
//   . LocalAlloc, GlobalAlloc
//   . HeapAlloc
//   . VirtualAlloc
// Cf. https://msdn.microsoft.com/en-us/library/ms810603.aspx
//    http://www.softlookup.com/tutorial/vc++/vcu13fi.asp#I1
//
// Lors de 'allocation massive, la dernier couche pose un probleme de performance dans
// son utilisation par les couches de plus base niveau
// https://randomascii.wordpress.com/2011/08/05/making-virtualalloc-arbitrarily-slower/
//   Utiliser VirtualAlloc, sans le flag MEM_TOP_DOWN
//   Utiliser MEM_COMMIT | MEM_RESERVE
//   Don't use the VirtualAlloc MEM_TOP_DOWN flag.
//   Nowhere is there any warning that VirtualAlloc with this flag does not scale well.It appears to use an O(n ^ 2)
//   algorithm where 'n' is related to the number of allocations you have made with this flag.From profiling it looks
//   like it is slowly and methodically scanning a list of the reserved memory trying to ensure that it finds the hole
//   with the highest address. When this flag is removed the time went from 25 minutes to instantaneous.Okay, it
//   probably wasn't instantaneous but it was so fast that it wasn't worth measuring.
//
// On consequence, on utilise ici directement la couche de plus bas niveau, avec les flag adequats

void* p_hugemalloc(size_t size)
{
	const DWORD flags = MEM_COMMIT | MEM_RESERVE;
	return VirtualAlloc(NULL, size, flags, PAGE_READWRITE);
}

void p_hugefree(void* memblock)
{
	VirtualFree(memblock, 0, MEM_RELEASE);
}

struct tm* p_localtime(const time_t* time)
{
	errno_t err;
	// Variable static pour renvoyer une valeur preallouee
	// Meme comportement que pour la fonction standard C ANSI
	static tm newtime;
	err = localtime_s(&newtime, time);
	if (err)
		return NULL;
	else
		return &newtime;
}

struct tm* p_gmtime(const time_t* time)
{
	errno_t err;
	// Variable static pour renvoyer une valeur preallouee
	// Meme comportement que pour la fonction standard C ANSI
	static tm newtime;
	err = gmtime_s(&newtime, time);
	if (err)
		return NULL;
	else
		return &newtime;
}

void* p_NewFileData()
{
	WIN32_FIND_DATAA* fdFile;
	fdFile = (WIN32_FIND_DATAA*)SystemObject::NewMemoryBlock(sizeof(WIN32_FIND_DATAA));
	return fdFile;
}

void p_DeleteFileData(void* pFileData)
{
	SystemObject::DeleteMemoryBlock(pFileData);
}

void* p_FindFirstFile(const char* dirname, void* pFileData)
{
	WIN32_FIND_DATAA* fdFile;
	HANDLE hFind = NULL;
	char sPath[2048];

	// On veut tous les fichiers et repertoires
	sprintf(sPath, "%s\\*.*", dirname);

	// Recherche la premiere entite du repertoire
	fdFile = (WIN32_FIND_DATAA*)pFileData;
	hFind = FindFirstFileA(sPath, fdFile);
	if (hFind != INVALID_HANDLE_VALUE)
		return hFind;
	else
		return NULL;
}

int p_FindNextFile(void* handle, void* pFileData)
{
	WIN32_FIND_DATAA* fdFile;
	HANDLE hFind = NULL;

	fdFile = (WIN32_FIND_DATAA*)pFileData;
	hFind = (HANDLE)handle;
	return FindNextFileA(hFind, fdFile);
}

const char* p_GetFileName(void* pFileData)
{
	WIN32_FIND_DATAA* fdFile;
	fdFile = (WIN32_FIND_DATAA*)pFileData;
	return fdFile->cFileName;
}

int p_IsDirectory(void* pFileData)
{
	WIN32_FIND_DATAA* fdFile;
	fdFile = (WIN32_FIND_DATAA*)pFileData;
	return fdFile->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

void p_FindClose(void* handle)
{
	HANDLE hFind = NULL;

	hFind = (HANDLE)handle;
	FindClose(hFind);
}

#endif // _WIN32

///////////////////////////////////////////////////////////////////////////
// Gestion d'un buffer tournant, permettant une certaine reentrance

#define BUFFER_NUMBER 16

// Renvoie un buffer de taille minimale BUFFER_LENGTH
char* StandardGetBuffer()
{
	static const int nTotalBufferSize = BUFFER_LENGTH * BUFFER_NUMBER;
	static char sSTDBuffers[nTotalBufferSize] = "";
	static char* sSTDCurrentBuffer = sSTDBuffers;
	static int nNextBufferOffset = 0;

	assert(sSTDBuffers <= sSTDCurrentBuffer);
	assert(sSTDCurrentBuffer <= sSTDBuffers + nTotalBufferSize);

	// Verification de la longueur courante
	assert(strlen(sSTDCurrentBuffer) < BUFFER_LENGTH);

	// Passage au buffer suivant
	sSTDCurrentBuffer = sSTDBuffers + nNextBufferOffset;
	assert(sSTDCurrentBuffer < sSTDBuffers + nTotalBufferSize);

	// Calcul de la position du prochain buffer
	nNextBufferOffset += BUFFER_LENGTH;
	if (nNextBufferOffset >= nTotalBufferSize)
		nNextBufferOffset = 0;
	return sSTDCurrentBuffer;
}

// Methode interne pour lire une entree depuis le shell en passant un buffer qui sera correctement alimente
// Rend fin de ligne si vide ou erreur
void StandardGetInputString(char* sBuffer, FILE* fInput)
{
	char* sResult;

	sBuffer[0] = '\0';
	sResult = fgets(sBuffer, BUFFER_LENGTH, fInput);
	if (sResult == NULL)
	{
		sBuffer[0] = '\n';
		sBuffer[1] = '\0';
	}
	assert(strlen(sBuffer) < BUFFER_LENGTH);
}

int SecureStrcpy(char* sDest, const char* sSource, int nMaxLength)
{
	int bOk = 0;
	int nLenDest;
	int nLengthSource;

	assert(nMaxLength > 0);

	nLengthSource = strlen(sSource);
	nLenDest = strlen(sDest);
	if (nLenDest + nLengthSource < nMaxLength)
	{
		strcpy(&sDest[nLenDest], sSource);
		bOk = 1;
	}
	return bOk;
}

// Implementation Windows du lancement d'excutable
#ifdef _WIN32

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

// Fonction utilitaire propre a Windows de formatage d'un message d'erreur
static void FormatErrorMessage(char* sErrorMessage)
{
	TCHAR szMessage[SYSTEM_MESSAGE_LENGTH + 1];
	size_t nConvertedSize;
	int nErrorMessageLength;

	// Formatage du message d'erreur
	FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
		      MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), szMessage, MAX_PATH, NULL);

	// Conversion entre differents types de chaines de caracteres
	// https://docs.microsoft.com/fr-fr/cpp/text/how-to-convert-between-various-string-types?view=msvc-160
	wcstombs_s(&nConvertedSize, sErrorMessage, SYSTEM_MESSAGE_LENGTH, szMessage, SYSTEM_MESSAGE_LENGTH);

	// Supression du saut de ligne en fin de chaine
	nErrorMessageLength = (int)strlen(sErrorMessage);
	while (nErrorMessageLength > 0)
	{
		// Nettoyage si saut de ligne en fin de chaine
		if (sErrorMessage[nErrorMessageLength - 1] == '\n' or sErrorMessage[nErrorMessageLength - 1] == '\r')
		{
			sErrorMessage[nErrorMessageLength - 1] = '\0';
			nErrorMessageLength--;
		}
		// Ok sinon
		else
			break;
	}
}

// Recherche le path de l'exe system associe a une extension (ex: ".txt")
// Renvoie chaine vide si non trouve
static const char* GetFileOpenAssocation(const char* sExtension)
{
	DWORD size = BUFFER_LENGTH;
	char* sFileAssociation = StandardGetBuffer();
	HRESULT hres;

	assert(sExtension != NULL);
	assert(strlen(sExtension) > 1);
	assert(sExtension[0] == '.');

	// Recherche de l'association geree par Windows
	// On n'accepte pas le OpenWith par defaut de Windows, car cet exe lance une fenetre
	// qui disparait si on ne se mete pas en attente du processus de lancement de la commande
	hres = AssocQueryStringA(
	    ASSOCF_INIT_IGNOREUNKNOWN, // Pour ne pas prendre en compte le OpenWith par defaut de Windows
	    ASSOCSTR_EXECUTABLE, sExtension, "open", sFileAssociation, &size);
	if (hres != S_OK)
		sFileAssociation[0] = '\0';
	return sFileAssociation;
}

// Lancement d'un exe de facon asynchrone
// Si on passe des arguments avec des blancs au milieu (ex: pour le path de l'exe),
// ceux-ci doivent etre entre double-quotes
// Renvoie 0 en cas d'erreur, et dans ce cas initialise le message d'erreur en parametre
static int StartProcess(const char* sCommandLine, char* sErrorMessage)
{
	BOOL ok;
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	HANDLE hNulOutput;

	assert(sCommandLine != NULL);
	assert(sErrorMessage != NULL);

	// Initialisation de la taille des structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Creation d'un handle sur un fichier NUL pour rediriger la sortie
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	hNulOutput = CreateFile(TEXT("NUL"), FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL);

	// Parametrage des fichier de sortie
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = NULL;
	si.hStdOutput = hNulOutput;
	si.hStdError = hNulOutput;

	// Lance le programme
	// Different de la fonction system du C ansi, qui lance un shell avec une commande en parametre
	// Ici, on lance un exe directement
	ok = CreateProcessA(
	    NULL,                // the path
	    (char*)sCommandLine, // Command line
	    NULL,                // Process handle not inheritable
	    NULL,                // Thread handle not inheritable
	    TRUE, // Set handle inheritance to TRUE (sinon, la rediction de la sortie vers NUL ne marche pas)
	    0,    // No creation flags
	    NULL, // Use parent's environment block
	    NULL, // Use parent's starting directory
	    &si,  // Pointer to STARTUPINFO structure
	    &pi   // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);

	// Recherche du message d'erreur si necessaire
	sErrorMessage[0] = '\0';
	if (!ok)
		FormatErrorMessage(sErrorMessage);

	// Fermeture des handle de process et de thread
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return ok;
}

// Implementation Windows du lancement d'excutable
int OpenApplication(const char* sApplicationExeName, const char* sApplicationLabel, const char* sFileToOpen,
		    char* sErrorMessage)
{
	int ok;
	const char* sFileExtension;
	const char* sSearch;
	const char* sFileAssociation;
	char* sCommandLine;
	char* sSystemErrorMessage;

	assert(sApplicationExeName != NULL);
	assert(strlen(sApplicationExeName) < SYSTEM_MESSAGE_LENGTH / 2);
	assert(sApplicationLabel != NULL);
	assert(strlen(sApplicationLabel) < SYSTEM_MESSAGE_LENGTH / 2);
	assert(sFileToOpen != NULL);
	assert(strchr(sFileToOpen, '.') != NULL);
	assert(sErrorMessage != NULL);

	// Initialisations
	ok = 1;
	sErrorMessage[0] = '\0';
	sFileAssociation = "";

	// Recherche de l'extension du fichier, y compris le '.' de depart
	sFileExtension = NULL;
	sSearch = sFileToOpen;
	while (sSearch[0] != '\0')
	{
		if (sSearch[0] == '.')
			sFileExtension = sSearch;
		sSearch++;
	}
	assert(sFileExtension != NULL);

	// Recherche de l'application associe a l'extension du fichier
	ok = 0;
	if (sFileExtension != NULL && strlen(sFileExtension) > 1)
	{
		sFileAssociation = GetFileOpenAssocation(sFileExtension);
		ok = sFileAssociation[0] != '\0';
	}

	// On utilise l'extension de l'editeur de texte par defaut si non trouve et qu'il n'y a pas d'exe specifie
	if (not ok && sApplicationExeName[0] == '\0')
	{
		sFileAssociation = GetFileOpenAssocation(".txt");
		ok = sFileAssociation[0] != '\0';
	}

	// Erreur si pas d'association
	if (!ok)
	{
		sprintf(sErrorMessage, "%s tool not found", sApplicationLabel);
	}
	// Lancement de l'application
	else
	{
		// Preparation des arguments: commande de longueur inconnue, et message d'erreur
		sCommandLine = new char[strlen(sFileAssociation) + strlen(sFileToOpen) + 10];
		sprintf(sCommandLine, "\"%s\" \"%s\"", sFileAssociation, sFileToOpen);
		sSystemErrorMessage = StandardGetBuffer();

		// Lancement de la commande
		ok = StartProcess(sCommandLine, sSystemErrorMessage);
		delete[] sCommandLine;

		// Message d'erreur
		if (!ok)
		{
			// On passe par SecureStrcpy plutot que par  sprintf en raison de l'incertitude sur les tailles
			// de parametres
			assert(sErrorMessage[0] == '\0');
			SecureStrcpy(sErrorMessage, "unable to launch ", SYSTEM_MESSAGE_LENGTH);
			SecureStrcpy(sErrorMessage, sApplicationLabel, SYSTEM_MESSAGE_LENGTH);
			SecureStrcpy(sErrorMessage, " tool using \"", SYSTEM_MESSAGE_LENGTH);
			SecureStrcpy(sErrorMessage, sFileAssociation, SYSTEM_MESSAGE_LENGTH);
			SecureStrcpy(sErrorMessage, "\" (", SYSTEM_MESSAGE_LENGTH);
			SecureStrcpy(sErrorMessage, sSystemErrorMessage, SYSTEM_MESSAGE_LENGTH);
			SecureStrcpy(sErrorMessage, ")", SYSTEM_MESSAGE_LENGTH);
		}
	}
	return ok;
}

#elif defined(__gnu_linux__)
#include <spawn.h>

// Implementation linux du lancement d'executable
int OpenApplication(const char* sApplicationExeName, const char* sApplicationLabel, const char* sFileToOpen,
		    char* sErrorMessage)
{
	int ok;
	const char* sFileExtension;
	const char* sSearch;
	const char* sFileAssociation;
	char* sCommandLine;
	char* sSystemErrorMessage;

	assert(sApplicationExeName != NULL);
	assert(strlen(sApplicationExeName) < SYSTEM_MESSAGE_LENGTH / 2);
	assert(sApplicationLabel != NULL);
	assert(strlen(sApplicationLabel) < SYSTEM_MESSAGE_LENGTH / 2);
	assert(sFileToOpen != NULL);
	assert(strchr(sFileToOpen, '.') != NULL);
	assert(sErrorMessage != NULL);

	// Initialisations
	ok = 1;
	sErrorMessage[0] = '\0';

	// Test d'existence de l'exe s'il est specifiee
	if (sApplicationExeName[0] != '\0')
	{
		sCommandLine = StandardGetBuffer();
		sprintf(sCommandLine, "command -v %s > /dev/null", sApplicationExeName);
		ok = system(sCommandLine) == 0;
		if (!ok)
			sprintf(sErrorMessage, "%s tool not installed", sApplicationLabel);
	}

	// Lancement si ok
	if (ok)
	{
		int processID;
		int status;
		char* argv[3];
		argv[0] = (char*)"xdg-open";
		argv[1] = (char*)sFileToOpen;
		argv[2] = (char*)0;

		// Lancement de l'exe approprie grace a xdg-open qui ouvre n'importe quel fichier avec le
		// programme qui lui est associe (a defaut l'editeur de texte). Fermeture au prealable de
		// stdin et stdout pour ne pas avoir de messages dan sla console
		posix_spawn_file_actions_t fa;
		posix_spawn_file_actions_init(&fa);
		posix_spawn_file_actions_addclose(&fa, STDERR_FILENO);
		posix_spawn_file_actions_addclose(&fa, STDOUT_FILENO);
		status = posix_spawn(&processID, "/usr/bin/xdg-open", &fa, NULL, argv, environ);
		posix_spawn_file_actions_destroy(&fa);

		// Message d'erreur
		if (status != 0)
		{
			ok = 0;
			assert(strlen(strerror(status)) < SYSTEM_MESSAGE_LENGTH / 2 - 30);
			sprintf(sErrorMessage, "unable to launch %s tool (%s)", sApplicationLabel, strerror(status));
		}
	}
	return ok;
}

#else
// Implementation du lancement d'executable avec erreur pour les autres OS
int OpenApplication(const char* sApplicationExeName, const char* sApplicationLabel, const char* sExtension,
		    const char* sFileToOpen, char* sErrorMessage)
{
	int ok;

	assert(sApplicationExeName != NULL);
	assert(strlen(sApplicationExeName) < SYSTEM_MESSAGE_LENGTH / 2);
	assert(sApplicationLabel != NULL);
	assert(strlen(sApplicationLabel) < SYSTEM_MESSAGE_LENGTH / 2);
	assert(sFileToOpen != NULL);
	assert(strchr(sFileToOpen, '.') != NULL);
	assert(sErrorMessage != NULL);

	// Initialisations
	ok = 0;
	sprintf(sErrorMessage, "unable to launch %s application on this OS", sApplicationLabel);
	return ok;
}
#endif

void* LoadSharedLibrary(const char* sLibraryPath, char* sErrorMessage)
{
	void* handle;
	int i;

	// Initialisation du message d'erreur avec la chaine vide
	for (i = 0; i < SYSTEM_MESSAGE_LENGTH + 1; i++)
	{
		sErrorMessage[i] = '\0';
	}

#ifdef _WIN32
	UINT nCurrentErrorMode;
	wchar_t* wString = NULL;
	int nBufferSize;

	// Premier appel pour determiner la taille necessaire pour stocker la chaine au format wide char
	wString = NULL;
	nBufferSize = MultiByteToWideChar(CP_ACP, 0, sLibraryPath, -1, wString, 0);

	// Second appel en ayant alloue la bonne taille
	wString = (wchar_t*)SystemObject::NewMemoryBlock(nBufferSize * sizeof(wchar_t));

	MultiByteToWideChar(CP_ACP, 0, sLibraryPath, -1, wString, nBufferSize);

	// Parametrage du mode de gestion des erreurs pour eviter d'avoir des boite de dialogues systemes bloquantes
	nCurrentErrorMode = GetErrorMode();
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// Chargement de la librairie
	handle = (void*)LoadLibrary(wString);
	SystemObject::DeleteMemoryBlock(wString);

	// Restitution du mode courant de gestion des erreurs
	SetErrorMode(nCurrentErrorMode);

	// Traitement des erreurs
	if (handle == NULL)
		FormatErrorMessage(sErrorMessage);
	return handle;

#else
	// Nettoyage des erreurs pre-existantes
	dlerror();

	// Chargement de la bibliotheque
	handle = dlopen(sLibraryPath, RTLD_LAZY);
	if (handle == NULL)
	{
		char* errstr;
		errstr = dlerror();
		if (errstr != NULL)
			strncpy(sErrorMessage, errstr, SYSTEM_MESSAGE_LENGTH);
	}
	return handle;
#endif
}

void* GetSharedLibraryFunction(void* libraryHandle, const char* sFunctionName)
{
#ifdef _WIN32
	return (void*)GetProcAddress((HINSTANCE)libraryHandle, sFunctionName);
#else
	return dlsym(libraryHandle, sFunctionName);
#endif
}

int FreeSharedLibrary(void* libraryHandle)
{
#ifdef _WIN32
	return FreeLibrary((HINSTANCE)libraryHandle);
#else
	return dlclose(libraryHandle);
#endif
}