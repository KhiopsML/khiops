// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

// Pour eviter les warning sur strerror
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "khiopsdriver_file_null.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#include <windows.h>
#endif // _MSC_VER

#ifdef __UNIX__
#include <unistd.h>
#ifdef __clang__
#include <sys/vfs.h> // ANDROID https://svn.boost.org/trac/boost/ticket/8816
#else
#include <sys/statvfs.h>
#endif // __clang__
#endif // __UNIX__

// Define to compile a read-only version of the driver
// Uncomment the following line to compile the read-only version of the driver
// #define __nullreadonlydriver__

const char* driver_getDriverName()
{
	return "Null driver";
}

const char* driver_getVersion()
{
	return "1.0.0";
}

const char* driver_getScheme()
{
#ifdef __nullreadonlydriver__
	return "nullro";
#else
	return "null";
#endif // __nullreadonlydriver__
}

int driver_isReadOnly()
{
#ifdef __nullreadonlydriver__
	return 1;
#else
	return 0;
#endif // __nullreadonlydriver__
}

int driver_connect()
{
	return 1;
}

int driver_disconnect()
{
	return 1;
}

int driver_isConnected()
{
	return 1;
}

// Nombre de caracteres du nom du scheme
int getSchemeCharNumber()
{
	static const int nSchemeCharNumber = (int)strlen(driver_getScheme());
	return nSchemeCharNumber;
}

// Test si un fichier est gere par le scheme
int isManaged(const char* sFilePathName)
{
	int ok;

	assert(sFilePathName != NULL);

	// Le debut du nom de fichier doit etre de la forme 'scheme:'
	ok = strncmp(sFilePathName, driver_getScheme(), getSchemeCharNumber()) == 0;
	ok = ok && sFilePathName[getSchemeCharNumber()] == ':';

	// Sous linux, on doit ensuite commencer par un '/'
#ifndef _MSC_VER
	ok = ok && sFilePathName[getSchemeCharNumber()] == '/';
#endif // _MSC_VER
	return ok;
}

// Methode utilitaire pour avoir acces au nom du fichier sans son schema
const char* getFilePath(const char* sFilePathName)
{
	int nStartFilePath;

	// La gestion du nombre de '/' n'est pas claire
	// Selon https://en.wikipedia.org/wiki/File_URI_scheme , on peut avoir de 1 a 4 '/' selon les cas
	// Des tests sous windows avec un navigateur firefox ou chrome montrent un grande tolerance au nombre de '/'.
	// Pourvue que le nom commence par 'file:', on peut avoir un nombre quelconque de '/', meme zero.
	// Firefox le corrige en mettant 'file:///'<path>, et chrone en omettant le scheme et en gardant juste <path>
	// On decide ici d'appliquer une politique souple, avec un nombre quelconque de '/', au moins un sous linux.

	// On extrait le chemin du fichier si le schema est correct
	if (isManaged(sFilePathName))
	{
		// Le debut du nom de fichier doit etre de la forme 'scheme:' suivi d'un nombre quelconque de '/'
		// Sous windows, on se place on premier caractere non '/', et sous linux, on inclus le dernier '/'
		// On renvoie un path commencant par '/'
		nStartFilePath = getSchemeCharNumber() + 1;
		while (sFilePathName[nStartFilePath] == '/')
			nStartFilePath++;
		assert(sFilePathName[nStartFilePath] != '/');
#ifndef _MSC_VER
		nStartFilePath--;
		assert(sFilePathName[nStartFilePath] == '/');
#endif // _MSC_VER
		return &sFilePathName[nStartFilePath];
	}
	// Sinon, on renvoie le nom du fichier tel quel
	else
		return sFilePathName;
}

int driver_exist(const char* filename)
{
	int ok;

// Pour UNIX ou wgpp
#if defined __UNIX__ || defined __WGPP__
	ok = access(getFilePath(filename), 0) != -1;
	// Pour Visual C++ 2005, 2008
#elif _MSC_VER >= 1400
	ok = _access(getFilePath(filename), 0) != -1; // 00=existence only
						      //  Pour Visual C++ 2003
#else
	ok = access(getFilePath(filename), _IOREAD) != -1;
#endif
	return ok;
}

long long int driver_getFileSize(const char* filename)
{
	long long int filesize;

	// Pour les fichiers de plus de 4 Go, il existe une API speciale (stat64...)
#if defined _MSC_VER || defined __MSVCRT_VERSION__
	struct __stat64 fileStat;
	int nError = _stat64(getFilePath(filename), &fileStat);
	if (nError != 0)
		filesize = 0;
	else
		filesize = fileStat.st_size;
#else
	struct stat64 fileStat;
	int nError = stat64(getFilePath(filename), &fileStat);
	if (nError != 0)
		filesize = 0;
	else
		filesize = fileStat.st_size;
#endif
	return filesize;
}

void* driver_fopen(const char* filename, char mode)
{
	void* handle;

	assert(mode == 'r' || mode == 'w' || mode == 'a');

	// Ouverture en lecture
	handle = NULL;
	if (mode == 'r')
	{
		handle = fopen(getFilePath(filename), "rb");
	}
	// Ouverture en ecriture
	else if (mode == 'w')
	{
		handle = fopen(getFilePath(filename), "wb");
	}
	// Ouverture en append
	else if (mode == 'a')
	{
		handle = fopen(getFilePath(filename), "r+b");
	}
	return handle;
}

int driver_fclose(void* stream)
{
	assert(stream != NULL);
	return fclose((FILE*)stream) == 0;
}

long long int driver_fread(void* ptr, size_t size, size_t count, void* stream)
{
	long long int readcount;

	assert(stream != NULL);

	// Lecture dans le fichier
	readcount = fread(ptr, size, count, (FILE*)stream);
	if (readcount != (long long int)count && ferror((FILE*)stream))
		readcount = -1;
	return readcount;
}

int driver_fseek(void* stream, long long int offset, int whence)
{
	int ok;
	// Pour les fichiers de plus de 4 Go, il existe une API speciale (stat64...)
#if defined _MSC_VER || defined __MSVCRT_VERSION__
	_fseeki64((FILE*)stream, offset, whence);
#elif defined __clang__
	fseeko((FILE*)stream, offset, whence);
#else
	fseeko64((FILE*)stream, offset, whence);
#endif
	ok = (ferror((FILE*)stream) == 0);
	return ok;
}

const char* driver_getlasterror()
{
	return strerror(errno);
}

// Compilation conditionnelle des methodes de type read-write
#ifndef __nullreadonlydriver__

long long int driver_fwrite(const void* ptr, size_t size, size_t count, void* stream)
{
	long long int writecount;

	assert(stream != NULL);

	// Ecriture dans le fichier
	writecount = fwrite(ptr, size, count, (FILE*)stream);
	if (writecount != (long long int)count && ferror((FILE*)stream))
		writecount = 0;
	return writecount;
}

int driver_fflush(void* stream)
{
	int nRet;
	assert(stream != NULL);
	nRet = fflush((FILE*)stream);
	return nRet == 0;
}

int driver_remove(const char* filename)
{
	int ok;
	ok = remove(getFilePath(filename)) == 0;
	return ok;
}

int driver_mkdir(const char* pathname)
{
	// Pour UNIX ou wgpp
#if defined __UNIX__ // MinGW make use of__MSVCRT_VERSION__
	int error;
	error = mkdir(getFilePath(pathname), S_IRWXU);
	return error == 0;
	// Pour Visual C++
#else
	int error;
	error = _mkdir(getFilePath(pathname));
	return error == 0;
#endif
}

int driver_rmdir(const char* pathname)
{
	// Pour UNIX ou wgpp
#if defined __UNIX__ || defined __WGPP__
	int error;
	error = rmdir(getFilePath(pathname));
	return error == 0;
	// Pour Visual C++
#else
	int error;
	error = _rmdir(getFilePath(pathname));
	return error == 0;
#endif
}

long long int driver_diskFreeSpace(const char* filename)
{
	const char* sPathName;

	// Si rien n'est specifie, on prend le repertoire courant
	if (strcmp(filename, "") == 0)
		sPathName = ".";
	// Sinon, on prend le repertoire passe en parametre
	else
		sPathName = getFilePath(filename);

		// Implementation windows
#if defined _MSC_VER || defined __MSVCRT_VERSION__
	{
		long long int lFreeDiskSpace = 0;
		int nLength;
		WCHAR* pszPathName;
		int nError;
		unsigned __int64 lFreeBytesAvailable;
		unsigned __int64 lTotalNumberOfBytes;
		unsigned __int64 lTotalNumberOfFreeBytes;

		// Passage en WCHAR
		nLength = (int)strlen(sPathName);
		pszPathName = new WCHAR[nLength + 1];
		mbstowcs(pszPathName, sPathName, nLength + 1);

		// Appel de la routine Windows
		nError = GetDiskFreeSpaceEx(pszPathName, (PULARGE_INTEGER)&lFreeBytesAvailable,
					    (PULARGE_INTEGER)&lTotalNumberOfBytes,
					    (PULARGE_INTEGER)&lTotalNumberOfFreeBytes);
		if (nError != 0)
			lFreeDiskSpace = lFreeBytesAvailable;

		// Nettoyage
		delete[] pszPathName;

		// Nettoyage de la chaine allouee
		assert(lFreeDiskSpace >= 0);
		return lFreeDiskSpace;
	};
#endif // _MSC_VER

// Implementation Linux
#if defined __UNIX__
#ifdef __clang__
	{
		struct statfs fiData;
		long long int lFree;

		assert(sPathName != NULL);
		if ((statfs(sPathName, &fiData)) < 0)
		{
			lFree = 0;
		}
		else
		{
			lFree = fiData.f_bavail;
			lFree *= fiData.f_bsize;
		}
		assert(lFree >= 0);
		return lFree;
	}

#else  // __clang__
	{
		// cf. statvfs for linux.
		// http://stackoverflow.com/questions/1449055/disk-space-used-free-total-how-do-i-get-this-in-c
		// http://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/statvfs.h.html
		struct statvfs64 fiData;
		long long int lFree;

		assert(sPathName != NULL);

		if ((statvfs64(sPathName, &fiData)) < 0)
		{
			lFree = 0;
		}
		else
		{
			lFree = fiData.f_bavail;
			lFree *= fiData.f_bsize;
		}
		assert(lFree >= 0);
		return lFree;
	}
#endif // __clang__
#endif // __UNIX__
}

#endif // __nullreadonlydriver__
