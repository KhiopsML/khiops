// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "CharVector.h"
#include "Vector.h"
#include "Timer.h"
class PLMsgContext;

/////////////////////////////////////////////////////////////////////////////
// Classe PLSerializer
// Cette classe offre un service de de/serialisation pour tous les types simples.
// Elle doit etre utilise comme un systeme d'E/S classique :
//		- en ecriture, on ajoute successivement des objets a serialiser
//		- en lecture on deserialise successivement ces objets (dans l'ordre initial)
// On peut facilement l'utiliser pour serialiser des objets plus complexes.
// Elle est utilisee pour l'envoi et la reception de message MPI
class PLSerializer : public Object
{
public:
	// Constructeur
	PLSerializer();
	~PLSerializer();

	// Ouverture et fermeture
	void OpenForRead(PLMsgContext* context);
	void OpenForWrite(PLMsgContext* context);
	void Close();
	boolean IsOpenForRead();
	boolean IsOpenForWrite();

	// Reinitialisation
	void Initialize();

	// Serialisation / Deserialisation d'un entier
	void PutInt(int nValue);
	int GetInt();

	// Serialisation / Deserialisation d'un double
	void PutDouble(double dValue);
	double GetDouble();

	// Serialisation / Deserialisation d'un boolean
	void PutBoolean(boolean bValue);
	boolean GetBoolean();

	// Serialisation / Deserialisation d'un longint
	void PutLongint(longint lValue);
	longint GetLongint();

	// Serialisation / Deserialisation d'une chaine
	void PutString(const ALString& sValue);
	ALString GetString();

	// Serialisation / Deserialisation d'une caractere
	void PutChar(char c);
	char GetChar();

	// Serialisation / Deserialisation d'un tableau de char
	// Le tableau resultat de la deserialisation appartient a l'appelant
	// et est a detruire en utilisant DeleteCharArray
	void PutCharArray(const char* c);
	char* GetCharArray();

	// Serialisation / Deserialisation d'un vecteur de chars
	void PutCharVector(const CharVector* cvIn);
	void GetCharVector(CharVector* cvOut);

	// Serialisation / Deserialisation d'un vecteur de ALString
	void PutStringVector(const StringVector* svIn);
	void GetStringVector(StringVector* svOut);

	// Serialisation / Deserialisation d'un vecteur d'entiers
	void PutIntVector(const IntVector* ivIn);
	void GetIntVector(IntVector* ivOut);

	// Serialisation / Deserialisation d'un vecteur d'entiers longs
	void PutLongintVector(const LongintVector* lvIn);
	void GetLongintVector(LongintVector* lvOut);

	// Serialisation / Deserialisation d'un vecteur de double
	void PutDoubleVector(const DoubleVector* dvIn);
	void GetDoubleVector(DoubleVector* dvOut);

	// Serialisation / deserialisation de l'indicateur d'objet NULL
	void PutNullToken(boolean bIsNull);
	boolean GetNullToken();

	// Methode de test
	static void Test();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ajout et recuperation du type
	char GetType();
	void PutType(char cType);

	// Copie d'une zone memoire dans le buffer avec envoi a chaque remplissage du buffer
	// nSize ne doit pas etre plus grand qu'un bloc
	void PutBufferChars(const char* sSource, int nSize);

	// Copie du buffer interne vers une zone memoire
	// nSize ne doit pas etre plus grand qu'un bloc
	void GetBufferChars(char* sTarget, int nSize);

	// Ajout d'un vecteur de tres grande taille au buffer
	// La taille et la tailel allouee sont exprimees pour les CharVector (nElementSize=1)
	void AddMemVectorToBuffer(MemHugeVector& memHugeVector, int nSize, int nAllocSize);

	// Extraction d'un vecteur de tres grande taille du buffer
	// La taille et la taille allouee sont exprimees pour les CharVector (nElementSize=1)
	void GetMemVectorFromBuffer(MemHugeVector& memHugeVector, int nSize, int nAllocSize);

	// Recopie du buffer
	// Utiliser pour envoyer le contenu du serializer
	void ExportBufferMonoBlock(char* sTarget, int nSize);

	// Contexte de la serialisation
	boolean IsSendMode() const;
	boolean IsRecvMode() const;
	boolean IsBCastMode() const;
	boolean IsStdMode() const;

	// Buffer contenant les variables serialisees
	CharVector cvBuffer;

	// Position dans le buffer en lecture ou ecriture
	int nBufferPosition;

	// Gestion du buffer
	boolean bIsOpenForRead;
	boolean bIsOpenForWrite;

	// Contexte de la serialisation : standard, avec envoi des blocs au fil de l'eau (Send ou broadcast)
	PLMsgContext* context;

	// Gestion du buffer de serialisation
	int nBufferCurrentBlockIndex;
	boolean bIsBufferMonoBlock;
	char* sBufferCurrentBlock;

	int nPutBufferCharsCallNumber;

	// Constantes utile pour la serialisation
	debug(; static const char cTypeInt = 'i'; static const char cTypeDouble = 'd';
	      static const char cTypeLongint = 'l'; static const char cTypeString = 's';
	      static const char cTypeChar = 'c'; static const char cTypeBoolean = 'b';
	      static const char cTypeDoubleVector = 'D'; static const char cTypeIntVector = 'I';
	      static const char cTypeLongintVector = 'L'; static const char cTypeCharVector = 'C';
	      static const char cTypeCharArray = 'N'; static const char cTypeStringVector = 'S';
	      static const char cEndOfSerializer = 'E';);

	friend class PLMPITaskDriver;
	friend class PLMPISlaveProgressionManager; // Pour envoi assynchrone

	// Methodes privees tres techniques
	// Mise a disposition des attributs protected aux classes friends
	int InternalGetAllocSize() const;
	int InternalGetBlockSize() const;
	int InternalGetElementSize() const;
	char* InternalGetMonoBlockBuffer() const;
	char* InternalGetMultiBlockBuffer(int i) const;
};

/////////////////////////////////////////////////////////////////////////////
// Classe PLMsgContext
// Contexte de la serialization
// A vocation a etre reimplementee notamment pour l'envoi de serializer avec MPI

class PLMsgContext : public Object
{
public:
	// Affichage
	virtual void Write(ostream& ost) const = 0;

protected:
	// Constructeur
	PLMsgContext();
	~PLMsgContext();
	enum MSGTYPE
	{
		SEND,
		RECV,
		BCAST,
		RSEND
	};
	int nMsgType;

	friend class PLMPITaskDriver;
	friend class PLSerializer;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean PLSerializer::IsSendMode() const
{
	return context != NULL and
	       (context->nMsgType == PLMsgContext::SEND or context->nMsgType == PLMsgContext::RSEND);
}

inline boolean PLSerializer::IsRecvMode() const
{
	return context != NULL and context->nMsgType == PLMsgContext::RECV;
}

inline boolean PLSerializer::IsBCastMode() const
{
	return context != NULL and context->nMsgType == PLMsgContext::BCAST;
}
inline boolean PLSerializer::IsStdMode() const
{
	return context == NULL;
}
inline int PLSerializer::InternalGetAllocSize() const
{
	return cvBuffer.InternalGetAllocSize();
}
inline int PLSerializer::InternalGetBlockSize() const
{
	return cvBuffer.InternalGetBlockSize();
}
inline int PLSerializer::InternalGetElementSize() const
{
	return cvBuffer.InternalGetElementSize();
}
inline char* PLSerializer::InternalGetMonoBlockBuffer() const
{
	return cvBuffer.InternalGetMonoBlockBuffer();
}
inline char* PLSerializer::InternalGetMultiBlockBuffer(int i) const
{
	return cvBuffer.InternalGetMultiBlockBuffer(i);
}