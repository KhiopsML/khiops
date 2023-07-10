// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "CharVector.h"
#include "Timer.h"
#include "FileService.h"
#include "SystemFileDriverCreator.h"
#include "SystemFile.h"
#include "MemoryStatsManager.h"
#include "HugeBuffer.h"

// Prototype des fonctions d'acces au disque dur
typedef void (*RequestIOFunction)(int);
typedef void (*ReleaseIOFunction)(int);

//////////////////////////////////////////////////////////////////////////
// Classe FileCache: buffer de lecture/ecriture d'un fichier
// potentiellement de tres grande taille
// Les methode de lecture/ecriture prennent en parametre un SystemFileDriver,
// ce qui permet d'acceder a un fichier local soit ansi, soit hdfs
class FileCache : public Object
{
public:
	// Constructeur
	FileCache();
	~FileCache();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// buffer est preservee, la partie supplementaire est initialisee a '\0'
	void SetSize(int nValue);
	int GetSize() const;

	// Acces aux elements du buffer
	void SetAt(int nIndex, char cValue);
	char GetAt(int nIndex) const;

	// Ajout d'un element en fin et retaillage du buffer
	void Add(char cValue);

	// Rempli le buffer le contenu du fichier a partir de sa position courante
	// jusqu'a la prochaine fin de ligne ou fin de fichier, ou que le buffer soit rempli
	// avant d'avoir atteint une fin de ligne ou de fichier
	// Renvoie le nombre de caracteres lus, ainsi qu'un indicateur de fin de ligne et fin de fichier
	// Renvoie le nombre d'octets lus, -1 en cas d'erreur, avec emission d'un message si le errorSender est non NULL
	int ReadLine(SystemFile* fileHandle, boolean& bEol, boolean& bEof, const Object* errorSender);

	// Renvoie le nombre de lignes contenues dans une portion du buffer
	int ComputeLineNumber(int nBeginPos, int nEndPos) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;

	// Methode de test de comptage des ligne pour un fichier de taille inferieure a 2 Go
	static int TestCountLines(const ALString& sFileName, boolean bSilentMode);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Renvoie le nombre de lignes contenues dans une portion d'un block de caracteres
	int ComputeBlockLineNumber(const char* pValues, int nBeginPos, int nEndPos) const;

	// Methode issues de CharVector, pour BufferedFile
	boolean SetLargeSize(int nValue);
	void CopyFrom(const FileCache* fbSource);

	// Recherche le caractere fin de ligne dans un buffer jusqu'a une taille donnee
	// Renvoie l'index du caracatere
	// Renvoie -1 si le buffer ne contient pas de fin de ligne
	int FindEol(char* sBuffer, int nLength);

	// Ecrit les premiers elements du buffer dans un fichier dans la limite de la taille du Buffer
	// Le fichier doit etre ouvert en ecriture
	// L'ecriture se fait a partir de la position courante dans  le fichier
	// Renvoie false en cas d'erreur, avec emission d'un message si le errorSender est non NULL
	// Methode utilsee uniquement pour les tests
	boolean WriteToFile(SystemFile* fileHandle, int nSizeToWrite, const Object* errorSender) const;

	// Rempli le buffer avec le fichier en remplacant le contenu precedent, dans
	// la limite de la taille restante du fichier
	// Le fichier doit etre ouvert en lecture
	// La lecture se fait a partir de la position lPos
	// Renvoie le nombre d'octets lus, potentiellement inferieur au nombre demande
	// Renvoie 0 en cas d'erreur, avec positionnement du IsError a true du driver et
	//  emission d'un message si le errorSender est non NULL
	// Methode utilsee uniquement pour les tests
	int ReadFromFile(SystemFile* fileHandle, longint lPos, int nSizeToRead, const Object* errorSender);

	// Buffer du fichier
	// On a acces au methodes internes de CharVector, qui a declare FileCache en Friend
	CharVector cvBuffer;

	// Methodes d'acces au disque
	static RequestIOFunction fRequestIOFunction;
	static ReleaseIOFunction fReleaseIOFunction;

	// Redefinition des methodes de demandes d'acces au disque
	static void SetRequestIOFunction(RequestIOFunction fRequestFunction);
	static RequestIOFunction GetRequestIOFunction();
	static void SetReleaseIOFunction(ReleaseIOFunction fReleaseFunction);
	static ReleaseIOFunction GetReleaseIOFunction();

	friend class BufferedFile;
	friend class InputBufferedFile;
	friend class OutputBufferedFile;
	friend class PLMPITaskDriver; // pour acces a SetRequestIOFunction et SetReleaseIOFunction

	// Methodes privees tres techniques
	// Mise a disposition des attributs protected aux classes friends
	int InternalGetAllocSize() const;
	static int InternalGetBlockSize();
	static int InternalGetElementSize();
	char* InternalGetMonoBlockBuffer() const;
	char* InternalGetMultiBlockBuffer(int i) const;
	int InternalGetBufferSize() const;
};

// Methodes en inline

inline boolean FileCache::SetLargeSize(int nValue)
{
	return cvBuffer.SetLargeSize(nValue);
}

inline void FileCache::CopyFrom(const FileCache* fbSource)
{
	cvBuffer.CopyFrom(&fbSource->cvBuffer);
}

inline void FileCache::SetSize(int nValue)
{
	return cvBuffer.SetSize(nValue);
}

inline int FileCache::GetSize() const
{
	return cvBuffer.GetSize();
}

inline void FileCache::SetAt(int nIndex, char cValue)
{
	cvBuffer.SetAt(nIndex, cValue);
}

inline char FileCache::GetAt(int nIndex) const
{
	return cvBuffer.GetAt(nIndex);
}

inline void FileCache::Add(char cValue)
{
	cvBuffer.Add(cValue);
}

inline void FileCache::SetRequestIOFunction(RequestIOFunction fRequestFunction)
{
	fRequestIOFunction = fRequestFunction;
}

inline RequestIOFunction FileCache::GetRequestIOFunction()
{
	return fRequestIOFunction;
}

inline void FileCache::SetReleaseIOFunction(ReleaseIOFunction fReleaseFunction)
{
	fReleaseIOFunction = fReleaseFunction;
}

inline ReleaseIOFunction FileCache::GetReleaseIOFunction()
{
	return fReleaseIOFunction;
}

inline int FileCache::InternalGetAllocSize() const
{
	return cvBuffer.InternalGetAllocSize();
}
inline int FileCache::InternalGetBlockSize()
{
	return CharVector::InternalGetBlockSize();
}

inline int FileCache::InternalGetElementSize()
{
	return CharVector::InternalGetElementSize();
}
inline char* FileCache::InternalGetMonoBlockBuffer() const
{
	return cvBuffer.InternalGetMonoBlockBuffer();
}
inline char* FileCache::InternalGetMultiBlockBuffer(int i) const
{
	return cvBuffer.InternalGetMultiBlockBuffer(i);
}

inline int FileCache::InternalGetBufferSize() const
{
	return cvBuffer.GetSize();
}
