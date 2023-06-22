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
// Implementation standard pour gcc, et pour MS VC++ avant 2005
#if defined __UNIX__ or defined __WGPP__ or _MSC_VER < 1400

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

#endif //  __UNIX__, __WGPP__, _MSC_VER < 1400

////////////////////////////////////////////////////
// Reimplementation pour MS Visual C++ 2005, 2008
#if _MSC_VER >= 1400

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

#endif // _MSC_VER >= 1400

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

void* LoadSharedLibrary(const char* sLibraryPath, char* sErrorMessage)
{
	void* handle;
	int i;

	// Initialisation du message d'erreur avec la chaine vide
	for (i = 0; i < SHARED_LIBRARY_MESSAGE_LENGTH + 1; i++)
	{
		sErrorMessage[i] = '\0';
	}

#if defined(_MSC_VER)
	wchar_t* wString = NULL;
	int nBufferSize;

	// Premier appel pour determiner la taille necessaire pour stocker la chaine au format wide char
	wString = NULL;
	nBufferSize = MultiByteToWideChar(CP_ACP, 0, sLibraryPath, -1, wString, 0);

	// Second appel en ayant alloue la bonne taille
	wString = (wchar_t*)SystemObject::NewMemoryBlock(nBufferSize * sizeof(wchar_t));

	MultiByteToWideChar(CP_ACP, 0, sLibraryPath, -1, wString, nBufferSize);

	// Chargement de la librairie
	handle = (void*)LoadLibrary(wString);
	SystemObject::DeleteMemoryBlock(wString);

	// Traitement des erreures
	if (handle == NULL)
	{
		TCHAR szMessage[SHARED_LIBRARY_MESSAGE_LENGTH + 1];
		size_t nConvertedSize;
		int nErrorMessageLength;

		// Formatage du message d'erreur
		FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
			      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), szMessage, MAX_PATH, NULL);

		// Conversion entre differents types de chaines de caracteres
		// https://docs.microsoft.com/fr-fr/cpp/text/how-to-convert-between-various-string-types?view=msvc-160
		wcstombs_s(&nConvertedSize, sErrorMessage, SHARED_LIBRARY_MESSAGE_LENGTH, szMessage,
			   SHARED_LIBRARY_MESSAGE_LENGTH);

		// Supression du saut de ligne en fin de chaine
		nErrorMessageLength = (int)strlen(sErrorMessage);
		while (nErrorMessageLength > 0)
		{
			// Nettoyage si saut de ligne en fin de chaine
			if (sErrorMessage[nErrorMessageLength - 1] == '\n' or
			    sErrorMessage[nErrorMessageLength - 1] == '\r')
			{
				sErrorMessage[nErrorMessageLength - 1] = '\0';
				nErrorMessageLength--;
			}
			// Ok sinon
			else
				break;
		}
	}
	return handle;

#elif defined(__UNIX__)
	// Nettoyage des erreurs pre-existantes
	dlerror();

	// Chargement de la bibliotheque
	handle = dlopen(sLibraryPath, RTLD_LAZY);
	if (handle == NULL)
	{
		char* errstr;
		errstr = dlerror();
		if (errstr != NULL)
			strncpy(sErrorMessage, errstr, SHARED_LIBRARY_MESSAGE_LENGTH);
	}
	return handle;
#endif
}

void* GetSharedLibraryFunction(void* libraryHandle, const char* sFunctionName)
{
#if defined(_MSC_VER)
	return (void*)GetProcAddress((HINSTANCE)libraryHandle, sFunctionName);
#elif defined(__UNIX__)
	return dlsym(libraryHandle, sFunctionName);
#endif
}

int FreeSharedLibrary(void* libraryHandle)
{
#if defined(_MSC_VER)
	return FreeLibrary((HINSTANCE)libraryHandle);
#elif defined(__UNIX__)
	return dlclose(libraryHandle);
#endif
}
