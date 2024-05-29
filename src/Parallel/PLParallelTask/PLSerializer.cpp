// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSerializer.h"
#include "PLParallelTask.h"

PLSerializer::PLSerializer()
{
	nBufferPosition = 0;
	nBufferCurrentBlockIndex = 0;
	bIsOpenForRead = false;
	bIsOpenForWrite = false;
	context = NULL;
	sBufferCurrentBlock = NULL;
	bIsBufferMonoBlock = false;
	nPutBufferCharsCallNumber = 0;
}

PLSerializer::~PLSerializer()
{
	assert(not bIsOpenForRead);
	assert(not bIsOpenForWrite);
}

void PLSerializer::OpenForRead(PLMsgContext* sContext)
{
	require(not bIsOpenForRead and not bIsOpenForWrite);
	require(nBufferPosition == 0);

	context = sContext;
	bIsOpenForRead = true;
	bIsBufferMonoBlock = true;

	if (IsStdMode())
	{
		bIsBufferMonoBlock = cvBuffer.nAllocSize <= CharVector::nBlockSize;
	}
	else
	{
		// Dans le cas de la serialisation au fil de l'eau le buffer, on alloue un seul bloc
		cvBuffer.SetSize(CharVector::nBlockSize);
		if (IsRecvMode())
		{
			PLParallelTask::GetDriver()->RecvBlock(this, context);
		}
		else
		{
			assert(IsBCastMode());
			PLParallelTask::GetDriver()->BCastBlock(this, context);
		}
	}

	// Affectation de l'adresse dans le buffer courant
	if (bIsBufferMonoBlock)
		sBufferCurrentBlock = &cvBuffer.pData.pValues[0];
	else
		sBufferCurrentBlock = &cvBuffer.pData.hugeVector.pValueBlocks[0][0];
}

void PLSerializer::OpenForWrite(PLMsgContext* sContext)
{
	require(not bIsOpenForRead and not bIsOpenForWrite);
	require(nBufferPosition == 0);
	nPutBufferCharsCallNumber = 0;
	context = sContext;
	bIsOpenForWrite = true;
	bIsBufferMonoBlock = true;

	if (IsStdMode())
	{
		cvBuffer.SetSize(0);
	}
	else
	{
		// Dans le cas de la serialisation au fil de l'eau le buffer, on alloue un seul bloc
		cvBuffer.SetSize(CharVector::nBlockSize);

		// Affectation de l'adresse dans le buffer courant
		sBufferCurrentBlock = &cvBuffer.pData.pValues[0];
	}
}

void PLSerializer::Close()
{
	require(bIsOpenForRead or bIsOpenForWrite);

	if (bIsOpenForWrite)
	{
		// Ajout d'un tag de fin de serialisation
		debug(PutType(cEndOfSerializer));

		// Envoi du buffer si il est rempli
		// ou si il est vide et que c'est le premier (cas ou on envoie un tag)
		if (nBufferPosition > 0 or nPutBufferCharsCallNumber == 0)
		{
			if (IsSendMode())
			{
				PLParallelTask::GetDriver()->SendBlock(this, context);
			}
			else
			{
				if (IsBCastMode())
					PLParallelTask::GetDriver()->BCastBlock(this, context);
				else
					assert(IsStdMode());
			}
		}

		nPutBufferCharsCallNumber = 0;
	}
	else
	{
		// On s'assure que le buffer a ete lu entirement lors de la deserialisation
		ensure(GetType() == cEndOfSerializer);
	}
	if (not IsStdMode())
	{
		cvBuffer.SetSize(0);
	}
	nBufferPosition = 0;
	nBufferCurrentBlockIndex = 0;
	bIsOpenForRead = false;
	bIsOpenForWrite = false;
}

boolean PLSerializer::IsOpenForRead() const
{
	return bIsOpenForRead;
}

boolean PLSerializer::IsOpenForWrite() const
{
	return bIsOpenForWrite;
}

void PLSerializer::Initialize()
{
	// L'utilisation de cette methode n'est pas documentee, est-elle vraiment necessaire ? a tester
	cvBuffer.SetSize(0);
	sBufferCurrentBlock = &cvBuffer.pData.pValues[0];
	nBufferPosition = 0;
	bIsOpenForRead = false;
	bIsOpenForWrite = false;
	PLSerializer();
}

void PLSerializer::PutInt(int intToSerialize)
{
	require(bIsOpenForWrite);
	debug(PutType(cTypeInt));

	PutBufferChars((const char*)&intToSerialize, sizeof(int));
}

int PLSerializer::GetInt()
{
	int nValue;

	require(bIsOpenForRead);
	debug(assert(GetType() == cTypeInt));

	GetBufferChars((char*)&nValue, sizeof(int));
	return nValue;
}

void PLSerializer::PutDouble(double dValue)
{
	require(bIsOpenForWrite);

	// Serialisation du type en mode debug seulement
	debug(PutType(cTypeDouble));
	PutBufferChars((const char*)&dValue, sizeof(double));
}

double PLSerializer::GetDouble()
{
	double dValue;

	require(bIsOpenForRead);
	debug(assert(GetType() == cTypeDouble));

	GetBufferChars((char*)&dValue, sizeof(double));
	return dValue;
}

void PLSerializer::PutBoolean(boolean bValue)
{
	require(bIsOpenForWrite);

	// Serialisation du type en mode debug seulement
	debug(PutType(cTypeBoolean));
	PutBufferChars((const char*)&bValue, sizeof(boolean));
}

boolean PLSerializer::GetBoolean()
{
	boolean bValue;

	require(bIsOpenForRead);
	debug(assert(GetType() == cTypeBoolean));

	GetBufferChars((char*)&bValue, sizeof(boolean));
	return bValue;
}

void PLSerializer::PutLongint(longint lValue)
{
	require(bIsOpenForWrite);

	// Serialisation du type en mode debug seulement
	debug(PutType(cTypeLongint));
	PutBufferChars((const char*)&lValue, sizeof(longint));
}

longint PLSerializer::GetLongint()
{
	longint lValue;

	require(bIsOpenForRead);
	debug(assert(GetType() == cTypeLongint));

	GetBufferChars((char*)&lValue, sizeof(longint));
	return lValue;
}

void PLSerializer::PutString(const ALString& stringToSerialize)
{
	int nYetToWrite;
	int nDataToWrite;
	int nIndex;
	const char* sValue;

	require(bIsOpenForWrite);

	// Serialisation du type en mode debug seulement
	debug(PutType(cTypeString));

	PutInt(stringToSerialize.GetLength());
	if (stringToSerialize.GetLength() != 0)
	{
		// Acces au buffer de ALString
		sValue = (const char*)stringToSerialize;

		// Cas ou tout tient dans un bloc
		if (stringToSerialize.GetLength() <= MemSegmentByteSize)
		{
			PutBufferChars(sValue, stringToSerialize.GetLength());
		}
		// Cas multi-bloc
		else
		{
			nYetToWrite = stringToSerialize.GetLength();
			nDataToWrite = MemSegmentByteSize;
			nIndex = 0;
			while (nYetToWrite > 0)
			{
				// Ecriture d'un segment memoire entier
				PutBufferChars(&sValue[nIndex], nDataToWrite);
				nYetToWrite -= nDataToWrite;
				nIndex += nDataToWrite;

				// Sauf pour le dernier bloc
				if (nYetToWrite < MemSegmentByteSize)
					nDataToWrite = nYetToWrite;
			}
		}
	}
}

ALString PLSerializer::GetString()
{
	int nSize;
	char* cValues;
	ALString sValue;
	int nYetToRead;
	int nDataToRead;
	int nIndex;

	require(bIsOpenForRead);
	debug(assert(GetType() == cTypeString));

	// Extraction de la taille de la chaine (caractere \0 compris)
	nSize = GetInt();

	if (nSize != 0)
	{
		// Copie memoire
		cValues = sValue.GetBufferSetLength(nSize);

		// Cas ou tout tient dans un bloc
		if (nSize <= MemSegmentByteSize)
		{
			GetBufferChars(cValues, nSize);
		}
		// Cas multi-bloc
		else
		{
			nYetToRead = nSize;
			nIndex = 0;
			nDataToRead = MemSegmentByteSize;
			while (nYetToRead > 0)
			{
				// Ecriture d'un segment memoire entier
				GetBufferChars(&cValues[nIndex], nDataToRead);
				nYetToRead -= nDataToRead;
				nIndex += nDataToRead;

				// Sauf pour le dernier bloc
				if (nYetToRead < MemSegmentByteSize)
					nDataToRead = nYetToRead;
			}
		}
	}
	return sValue;
}

void PLSerializer::PutChar(char c)
{
	require(bIsOpenForWrite);

	// Serialisation du type en mode debug seulement
	debug(PutType(cTypeChar));

	// Serialization du caractere
	PutBufferChars(&c, sizeof(char));
}

char PLSerializer::GetChar()
{
	char c;

	require(bIsOpenForRead);
	debug(assert(GetType() == cTypeChar));

	GetBufferChars(&c, sizeof(char));
	return c;
}

void PLSerializer::PutCharArray(const char* c)
{
	int nSize;
	int nYetToWrite;
	int nDataToWrite;
	int nIndex;

	require(bIsOpenForWrite);
	debug(PutType(cTypeCharArray));

	nSize = (int)strlen(c) + 1;
	PutInt(nSize);

	// Cas mono-bloc
	if (nSize <= MemSegmentByteSize)
	{
		PutBufferChars(c, nSize);
	}
	// Cas multi-bloc
	else
	{
		nYetToWrite = nSize;
		nDataToWrite = MemSegmentByteSize;
		nIndex = 0;
		while (nYetToWrite > 0)
		{
			// Ecriture d'un segment memoire entier
			PutBufferChars(&c[nIndex], nDataToWrite);
			nYetToWrite -= nDataToWrite;
			nIndex += nDataToWrite;

			// Sauf pour le dernier bloc
			if (nYetToWrite < MemSegmentByteSize)
				nDataToWrite = nYetToWrite;
		}
	}
}

char* PLSerializer::GetCharArray()
{
	int nSize;
	int nYetToRead;
	int nIndex;
	char* sChars;
	int nDataToRead;
	require(bIsOpenForRead);
	debug(assert(GetType() == cTypeCharArray));

	nSize = GetInt();
	sChars = NewCharArray(nSize);

	// Cas mono-bloc
	if (nSize <= MemSegmentByteSize)
	{
		GetBufferChars(sChars, nSize);
	}
	// Cas multi-bloc
	else
	{
		nYetToRead = nSize;
		nIndex = 0;
		nDataToRead = MemSegmentByteSize;
		while (nYetToRead > 0)
		{
			// Ecriture d'un segment memoire entier
			GetBufferChars(&sChars[nIndex], nDataToRead);
			nYetToRead -= nDataToRead;
			nIndex += nDataToRead;

			// Sauf pour le dernier bloc
			if (nYetToRead < MemSegmentByteSize)
				nDataToRead = nYetToRead;
		}
	}

	return sChars;
}

void PLSerializer::PutCharVector(const CharVector* cvIn)
{
	require(bIsOpenForWrite);
	require(cvIn != NULL);

	// Serialisation du type en mode debug seulement
	debug(PutType(cTypeCharVector));

	PutInt(cvIn->nSize);
	AddMemVectorToBuffer((MemHugeVector&)cvIn->pData, cvIn->nSize * CharVector::nElementSize,
			     cvIn->nAllocSize * CharVector::nElementSize);
}

void PLSerializer::GetCharVector(CharVector* cvOut)
{
	int nSize;

	require(bIsOpenForRead);
	require(cvOut != NULL);
	debug(assert(GetType() == cTypeCharVector));

	nSize = GetInt();
	cvOut->SetSize(nSize);
	GetMemVectorFromBuffer((MemHugeVector&)cvOut->pData, cvOut->nSize * CharVector::nElementSize,
			       cvOut->nAllocSize * CharVector::nElementSize);
}

void PLSerializer::PutStringVector(const StringVector* svIn)
{
	int i;

	require(bIsOpenForWrite);
	require(svIn != NULL);

	// Serialisation du type en mode debug seulement
	debug(PutType(cTypeStringVector));

	// Serialisation de la taille
	PutInt(svIn->GetSize());

	// Serialisation de chaque element
	for (i = 0; i < svIn->GetSize(); i++)
	{
		PutString(svIn->GetAt(i));
	}
}

void PLSerializer::GetStringVector(StringVector* svOut)
{
	int nSize;
	int i;

	require(bIsOpenForRead);
	require(svOut != NULL);
	debug(assert(GetType() == cTypeStringVector));

	// Deserialization de la taille
	nSize = GetInt();
	svOut->SetSize(nSize);

	// Deserialisation de chaque string
	for (i = 0; i < nSize; i++)
	{
		svOut->SetAt(i, GetString());
	}
}

void PLSerializer::PutIntVector(const IntVector* ivIn)
{
	require(bIsOpenForWrite);
	require(ivIn != NULL);

	// Serialisation du type en mode debug seulement
	debug(PutType(cTypeIntVector));

	// Serialisation de la taille
	PutInt(ivIn->GetSize());

	// Copie des blocs memoire
	AddMemVectorToBuffer((MemHugeVector&)ivIn->pData, ivIn->nSize * IntVector::nElementSize,
			     ivIn->nAllocSize * IntVector::nElementSize);
}

void PLSerializer::GetIntVector(IntVector* ivOut)
{
	int nSize;

	require(bIsOpenForRead);
	require(ivOut != NULL);
	debug(assert(GetType() == cTypeIntVector));

	// Deserialization de la taille
	nSize = GetInt();
	ivOut->SetSize(nSize);

	// Copie des blocs memoires
	GetMemVectorFromBuffer((MemHugeVector&)ivOut->pData, ivOut->nSize * IntVector::nElementSize,
			       ivOut->nAllocSize * IntVector::nElementSize);
}

void PLSerializer::PutLongintVector(const LongintVector* lvIn)
{
	require(bIsOpenForWrite);
	require(lvIn != NULL);

	// Serialisation du type en mode debug seulement
	debug(PutType(cTypeLongintVector));

	// Serialisation de la taille
	PutInt(lvIn->GetSize());

	// Copie des blocs memoire
	AddMemVectorToBuffer((MemHugeVector&)lvIn->pData, lvIn->nSize * LongintVector::nElementSize,
			     lvIn->nAllocSize * LongintVector::nElementSize);
}

void PLSerializer::GetLongintVector(LongintVector* lvOut)
{
	int nSize;

	require(bIsOpenForRead);
	require(lvOut != NULL);
	debug(assert(GetType() == cTypeLongintVector));

	// Deserialization de la taille
	nSize = GetInt();
	lvOut->SetSize(nSize);

	// Copie des blocs memoires
	GetMemVectorFromBuffer((MemHugeVector&)lvOut->pData, lvOut->nSize * LongintVector::nElementSize,
			       lvOut->nAllocSize * LongintVector::nElementSize);
}

void PLSerializer::PutDoubleVector(const DoubleVector* dvIn)
{
	require(bIsOpenForWrite);
	require(dvIn != NULL);
	debug(PutType(cTypeDoubleVector));

	// Serialisation de la taille
	PutInt(dvIn->GetSize());

	// Copie des blocs memoire
	AddMemVectorToBuffer((MemHugeVector&)dvIn->pData, dvIn->nSize * DoubleVector::nElementSize,
			     dvIn->nAllocSize * DoubleVector::nElementSize);
}

void PLSerializer::GetDoubleVector(DoubleVector* dvOut)
{
	int nSize;

	require(bIsOpenForRead);
	require(dvOut != NULL);
	debug(assert(GetType() == cTypeDoubleVector));

	// Deserialization de la taille
	nSize = GetInt();
	dvOut->SetSize(nSize);

	// Copie des blocs memoires
	GetMemVectorFromBuffer((MemHugeVector&)dvOut->pData, dvOut->nSize * DoubleVector::nElementSize,
			       dvOut->nAllocSize * DoubleVector::nElementSize);
}

void PLSerializer::PutNullToken(boolean bIsNull)
{
	require(bIsOpenForWrite);
	PutBoolean(bIsNull);
}

boolean PLSerializer::GetNullToken()
{
	require(bIsOpenForRead);
	return GetBoolean();
}

void PLSerializer::Test()
{
	PLSerializer serializer;
	CharVector cv;
	IntVector iv;
	int i;
	longint l;
	double d;
	boolean b;
	char c;
	char* sChars;
	char* sChars2;
	const int nLargeSize = 2 * CharVector::nBlockSize + 10;
	char* sLargeChars;
	ALString sString;
	CharVector cvBig;
	int nCnumber;
	ALString sLastString;
	const int nBigVectorSize = 2 * CharVector::nBlockSize - 1;

	// Creation d'un gros char*
	sLargeChars = NewCharArray(nLargeSize);
	for (i = 0; i < nLargeSize; i++)
	{
		sLargeChars[i] = 'b';
	}
	sLargeChars[nLargeSize - 1] = '\0';

	// Creation d'un vecteur de char
	cv.Add('T');
	cv.Add('h');
	cv.Add('e');
	cv.Add(' ');
	cv.Add('c');
	cv.Add('h');
	cv.Add('a');
	cv.Add('r');
	cv.Add(' ');
	cv.Add('v');
	cv.Add('e');
	cv.Add('c');
	cv.Add('t');
	cv.Add('o');
	cv.Add('r');

	// Creation d'un vecteur d'int
	for (i = 1; i < 100; i++)
	{
		iv.Add(i);
	}

	// Creation d'un vecteur de char sur plusieurs blocs
	for (i = 0; i < nBigVectorSize; i++)
	{
		cvBig.Add('c');
	}

	// Serialisation
	serializer.OpenForWrite(NULL);
	serializer.PutString("Serialisation : first attempt");
	serializer.PutCharArray(sLargeChars);
	serializer.PutCharVector(&cv);
	serializer.PutIntVector(&iv);
	serializer.PutChar('c');
	serializer.PutCharArray("A Null terminating string");
	serializer.PutInt(2);
	serializer.PutDouble(0.1234);
	serializer.PutLongint(1);
	serializer.PutBoolean(true);
	serializer.PutString("a test with ALString");
	serializer.PutCharVector(&cvBig);
	serializer.PutString("\n\n\tThat's all Folks !!");
	serializer.Close();

	// Initialisation des vecteurs
	cv.SetSize(0);
	iv.SetSize(0);
	cvBig.SetSize(0);

	// Deserialisation
	serializer.OpenForRead(NULL);
	sLastString = serializer.GetString();
	cout << "Get first string : " << sLastString << endl;
	sChars2 = serializer.GetCharArray();
	serializer.GetCharVector(&cv);
	serializer.GetIntVector(&iv);
	c = serializer.GetChar();
	sChars = serializer.GetCharArray();
	i = serializer.GetInt();
	d = serializer.GetDouble();
	l = serializer.GetLongint();
	b = serializer.GetBoolean();
	sString = serializer.GetString();
	serializer.GetCharVector(&cvBig);
	sLastString = serializer.GetString();
	serializer.Close();

	// Affichage
	for (i = 0; i < cv.GetSize(); i++)
	{
		cout << cv.GetAt(i);
	}
	cout << endl;
	for (i = 0; i < iv.GetSize(); i++)
	{
		cout << iv.GetAt(i) << " ";
	}
	cout << endl;
	cout << "Get Char : " << c << endl;
	cout << "Get char array : " << sChars << endl;
	cout << "Get integer " << i << endl;
	cout << "Get double " << d << endl;
	cout << "Get longint " << l << endl;
	cout << "Get boolean " << BooleanToString(b) << endl;
	cout << "Get ALString : " << sString << endl;
	cout << "Get Big vector of size " << cvBig.GetSize() << endl;

	nCnumber = 0;
	for (i = 0; i < cvBig.GetSize(); i++)
	{
		if (cvBig.GetAt(i) == 'c')
			nCnumber++;
	}
	cout << "Large vector contains  " << nCnumber << " c's / " << nBigVectorSize << endl;
	nCnumber = 0;
	for (i = 0; i < (int)strlen(sChars2); i++)
	{
		if (sChars2[i] == 'b')
			nCnumber++;
	}
	cout << "Large array contains  " << nCnumber << " b's / " << nLargeSize - 1 << endl;
	cout << "Get last string : " << sLastString << endl;
	DeleteCharArray(sChars);
	DeleteCharArray(sChars2);
	DeleteCharArray(sLargeChars);
}

char PLSerializer::GetType()
{
	char c;
	require(bIsOpenForRead);

	GetBufferChars(&c, sizeof(char));
	return c;
}

void PLSerializer::PutType(char cType)
{
	require(bIsOpenForWrite);
	PutBufferChars(&cType, sizeof(char));
}

void PLSerializer::PutBufferChars(const char* sSource, int nSize)
{
	int nSpaceInBlockDest;
	int nPositionInSrc;
	int nRemainer;

	assert(CharVector::nBlockSize == 64 * lKB);
	// Les messages envoyes on tous la taille nBlockSize qui est actuellement de 64 Kb
	// On envoie uniquement les blocs entiers de CharVector
	// Si la taille des blocs est augmentee dans Norm, la taille des messages va augmenter mecaniquement
	// et cela risque de saturee la bande passante.
	// Il faudra alors ajouter un niveau de buffer logique de taille 64 Kb : pour envoyer un bloc,
	// on enverra plusieurs blocs logique de taille 64 Kb

	require(nSize <= CharVector::nBlockSize);

	// En mode standard (sans envoi de blocs au fil de l'eau)
	// Le buffer a une taille croissante au fur et a mesure de la serialisation
	if (IsStdMode())
	{
		// Allocation de la taille necessaire
		cvBuffer.SetSize(nBufferPosition + nSize);

		// Cas multi-blocs
		if (cvBuffer.nAllocSize > CharVector::nBlockSize)
		{
			if (bIsBufferMonoBlock)
			{
				bIsBufferMonoBlock = false;
				nBufferCurrentBlockIndex = nBufferPosition / CharVector::nBlockSize;

				// Adresse multi blocs
				sBufferCurrentBlock =
				    &cvBuffer.pData.hugeVector.pValueBlocks[nBufferCurrentBlockIndex]
									   [nBufferPosition % CharVector::nBlockSize];
			}
			else
			{
				// Si le buffer a ete rempli au tour d'avant, il faut changer de bloc
				if (nBufferPosition % CharVector::nBlockSize == 0)
				{
					nBufferCurrentBlockIndex++;
					sBufferCurrentBlock =
					    &cvBuffer.pData.hugeVector.pValueBlocks[nBufferCurrentBlockIndex][0];
				}
			}
			nPositionInSrc = 0;

			// Espace disponible dans le bloc courant du tableau dest
			nSpaceInBlockDest = CharVector::nBlockSize - nBufferPosition % CharVector::nBlockSize;

			// Reste a ecrire en plus du bloc courant
			nRemainer = nSize - nSpaceInBlockDest;

			// Cas ou il faut ecrire la fin du bloc et le bloc suivant
			if (nRemainer > 0)
			{
				assert(
				    sBufferCurrentBlock ==
				    &cvBuffer.pData.hugeVector.pValueBlocks[nBufferCurrentBlockIndex]
									   [nBufferPosition % CharVector::nBlockSize]);

				// Copie dans le premier bloc
				memcpy(sBufferCurrentBlock, &sSource[nPositionInSrc], nSpaceInBlockDest);
				nBufferPosition += nSpaceInBlockDest;
				nPositionInSrc += nSpaceInBlockDest;

				// Changement de bloc
				nBufferCurrentBlockIndex++;
				sBufferCurrentBlock =
				    &cvBuffer.pData.hugeVector.pValueBlocks[nBufferCurrentBlockIndex][0];

				// Copie dans le second bloc
				memcpy(sBufferCurrentBlock, &sSource[nPositionInSrc], nRemainer);

				nBufferPosition += nRemainer;
				nPositionInSrc += nRemainer;
				sBufferCurrentBlock += nRemainer;
			}
			// Cas ou tout rentre dans le bloc courant
			else
			{
				// Copie
				assert(
				    sBufferCurrentBlock ==
				    &cvBuffer.pData.hugeVector.pValueBlocks[nBufferCurrentBlockIndex]
									   [nBufferPosition % CharVector::nBlockSize]);
				memcpy(sBufferCurrentBlock, &sSource[nPositionInSrc], nSize);

				nBufferPosition += nSize;
				nPositionInSrc += nSize;
				sBufferCurrentBlock += nSize;
			}
		}
		// Cas mono-bloc
		else
		{
			sBufferCurrentBlock = &cvBuffer.pData.pValues[nBufferPosition % CharVector::nBlockSize];
			memcpy(sBufferCurrentBlock, sSource, nSize);
			sBufferCurrentBlock += nSize;
			nBufferPosition += nSize;
		}
	}
	// Serialisation au fil de l'eau : le buffer est forcement mono-bloc
	else
	{
		assert(sBufferCurrentBlock == &cvBuffer.pData.pValues[nBufferPosition]);
		nPositionInSrc = 0;

		// Espace disponible dans le bloc courant du tableau dest
		nSpaceInBlockDest = CharVector::nBlockSize - nBufferPosition;

		// Est-ce que tout rentre dans un bloc
		nRemainer = nSize - nSpaceInBlockDest;

		// Remplissage en 2 passes
		if (nRemainer > 0)
		{
			memcpy(sBufferCurrentBlock, &sSource[nPositionInSrc], nSpaceInBlockDest);
			nBufferPosition += nSpaceInBlockDest;
			nPositionInSrc += nSpaceInBlockDest;

			// Envoi du bloc
			if (IsSendMode())
			{
				if (context->nMsgType == MSGTYPE::RSEND)
				{
					if (nPutBufferCharsCallNumber != 0)
						context->nMsgType = MSGTYPE::SEND;
				}
				PLParallelTask::GetDriver()->SendBlock(this, context);
			}
			else
				PLParallelTask::GetDriver()->BCastBlock(this, context);

			// Retour au debut du bloc
			sBufferCurrentBlock = &cvBuffer.pData.pValues[0];
			nBufferPosition = 0;

			// Copie
			memcpy(sBufferCurrentBlock, &sSource[nPositionInSrc], nRemainer);

			// Iteration de la position dans le bloc et de la position de lecture
			nBufferPosition += nRemainer;
			nPositionInSrc += nRemainer;
			sBufferCurrentBlock += nRemainer;
		}
		// Tout tient dans le bloc courant
		else
		{
			memcpy(sBufferCurrentBlock, &sSource[nPositionInSrc], nSize);
			nBufferPosition += nSize;
			nPositionInSrc += nSize;
			sBufferCurrentBlock += nSize;
		}

		// Envoi du bloc si il est plein
		if (nBufferPosition == CharVector::nBlockSize)
		{
			if (IsSendMode())
				PLParallelTask::GetDriver()->SendBlock(this, context);
			else
				PLParallelTask::GetDriver()->BCastBlock(this, context);
			nBufferPosition = 0;

			// Retour au debut du bloc
			sBufferCurrentBlock = &cvBuffer.pData.pValues[0];
		}
	}
	nPutBufferCharsCallNumber++;
}

void PLSerializer::GetBufferChars(char* sTarget, int nSize)
{
	int nCharNumberInBlocSrc;
	int nTargetPos;
	int nRemainer;

	require(nSize <= CharVector::nBlockSize);

	// Cas standard : il n'y a pas de reception des blocks au fil de l'eau
	// Le buffer peut etre multi-blocs
	if (IsStdMode())
	{
		require(nBufferCurrentBlockIndex == nBufferPosition / CharVector::nBlockSize);

		if (bIsBufferMonoBlock)
		{
			assert(sBufferCurrentBlock ==
			       &cvBuffer.pData.pValues[nBufferPosition % CharVector::nBlockSize]);
			memcpy(sTarget, sBufferCurrentBlock, nSize);
			nBufferPosition += nSize;
			sBufferCurrentBlock += nSize;
		}
		else
		{
			nTargetPos = 0;

			// Nombre de char a ecrire venant du bloc source courant
			nCharNumberInBlocSrc = CharVector::nBlockSize - nBufferPosition % CharVector::nBlockSize;

			// Nombre de caracteres venant du prochain bloc
			nRemainer = nSize - nCharNumberInBlocSrc;
			assert(sBufferCurrentBlock ==
			       &cvBuffer.pData.hugeVector
				    .pValueBlocks[nBufferCurrentBlockIndex][nBufferPosition % CharVector::nBlockSize]);

			// Si les donnees sont sur 2 blocs
			if (nRemainer > 0)
			{
				// Copie du premier bloc
				memcpy(&sTarget[nTargetPos], sBufferCurrentBlock, nCharNumberInBlocSrc);

				nBufferPosition += nCharNumberInBlocSrc;
				nTargetPos += nCharNumberInBlocSrc;
				nBufferCurrentBlockIndex++;

				// Changement de bloc
				sBufferCurrentBlock =
				    &cvBuffer.pData.hugeVector.pValueBlocks[nBufferCurrentBlockIndex][0];

				// Copie du deuxieme bloc
				memcpy(&sTarget[nTargetPos], sBufferCurrentBlock, nRemainer);

				nBufferPosition += nRemainer;
				nTargetPos += nRemainer;
				sBufferCurrentBlock += nRemainer;
			}
			// Les donnes sont dans un seul bloc
			else
			{
				// Copie des donnees
				memcpy(&sTarget[nTargetPos], sBufferCurrentBlock, nSize);

				nBufferPosition += nSize;
				nTargetPos += nSize;
				sBufferCurrentBlock += nSize;
			}

			// Si on a rempli completement le bloc courant
			if (nBufferPosition % CharVector::nBlockSize == 0)
			{
				// Changement de bloc
				nBufferCurrentBlockIndex++;
				sBufferCurrentBlock =
				    &cvBuffer.pData.hugeVector.pValueBlocks[nBufferCurrentBlockIndex][0];
			}

			ensure(nBufferCurrentBlockIndex == nBufferPosition / CharVector::nBlockSize);
		}
	}
	// Serialisation au fil de l'eau : le buffer est mono bloc
	else
	{
		// Si le bloc a ete entieremnt copie, reception d'un nouveau bloc
		if (nBufferPosition == CharVector::nBlockSize)
		{
			if (IsRecvMode())
				PLParallelTask::GetDriver()->RecvBlock(this, context);
			else
				PLParallelTask::GetDriver()->BCastBlock(this, context);

			// Positionement a 0 dans le bloc courant
			nBufferPosition = 0;
			sBufferCurrentBlock = &cvBuffer.pData.pValues[0];
		}
		nTargetPos = 0;

		// Nombre de char a ecrire venant du bloc source courant
		nCharNumberInBlocSrc = CharVector::nBlockSize - nBufferPosition;

		// Nombre de bloc venant du bloc suivant
		nRemainer = nSize - nCharNumberInBlocSrc;
		assert(sBufferCurrentBlock == &cvBuffer.pData.pValues[nBufferPosition]);

		// Si les donnees sources sont a cheval sur deux blocs
		if (nRemainer > 0)
		{
			// Copie du premier bloc
			memcpy(&sTarget[nTargetPos], sBufferCurrentBlock, nCharNumberInBlocSrc);
			nBufferPosition += nCharNumberInBlocSrc;
			nTargetPos += nCharNumberInBlocSrc;

			// Reception du bloc suivant
			if (IsRecvMode())
				PLParallelTask::GetDriver()->RecvBlock(this, context);
			else
				PLParallelTask::GetDriver()->BCastBlock(this, context);

			// Positionement au debut du bloc
			sBufferCurrentBlock = &cvBuffer.pData.pValues[0];
			nBufferPosition = 0;

			// Copie du bloc
			memcpy(&sTarget[nTargetPos], sBufferCurrentBlock, nRemainer);
			nBufferPosition += nRemainer;
			nTargetPos += nRemainer;
			sBufferCurrentBlock += nRemainer;
		}
		// Les donnees sont dans un seul bloc
		else
		{
			memcpy(&sTarget[nTargetPos], sBufferCurrentBlock, nSize);
			nBufferPosition += nSize;
			nTargetPos += nSize;
			sBufferCurrentBlock += nSize;
		}
	}
}

void PLSerializer::AddMemVectorToBuffer(MemHugeVector& memHugeVector, int nSize, int nAllocSize)
{
	int nBlocIndex;
	int nYetToWrite;
	int nDataToWrite;

	require(bIsOpenForWrite);

	// Cas ou tout tient dans un bloc
	if (nAllocSize <= MemSegmentByteSize)
	{
		PutBufferChars(memHugeVector.pValues, nSize);
	}
	// Cas multi-bloc
	else
	{
		nYetToWrite = nSize;
		nBlocIndex = 0;
		nDataToWrite = MemSegmentByteSize;
		while (nYetToWrite > 0)
		{
			// Ecriture d'un segment memoire entier
			PutBufferChars(memHugeVector.pValueBlocks[nBlocIndex], nDataToWrite);
			nYetToWrite -= nDataToWrite;
			nBlocIndex++;

			// Sauf pour le dernier bloc
			if (nYetToWrite < MemSegmentByteSize)
				nDataToWrite = nYetToWrite;
		}
	}
}

void PLSerializer::GetMemVectorFromBuffer(MemHugeVector& memHugeVector, int nSize, int nAllocSize)
{
	int nYetToRead;
	int nBlocIndex;
	int nDataToRead;

	require(bIsOpenForRead);

	// Cas ou tout tient dans un bloc
	if (nAllocSize <= MemSegmentByteSize)
	{
		GetBufferChars(memHugeVector.pValues, nSize);
	}
	// Cas multi-bloc
	else
	{
		nYetToRead = nSize;
		nBlocIndex = 0;
		nDataToRead = MemSegmentByteSize;
		while (nYetToRead > 0)
		{
			// Ecriture d'un segment memoire entier
			GetBufferChars(memHugeVector.pValueBlocks[nBlocIndex], nDataToRead);
			nYetToRead -= nDataToRead;
			nBlocIndex++;

			// Sauf pour le dernier bloc
			if (nYetToRead < MemSegmentByteSize)
				nDataToRead = nYetToRead;
		}
	}
}

void PLSerializer::ExportBufferMonoBlock(char* sTarget, int nSize)
{
	assert(bIsBufferMonoBlock);
	memcpy(sTarget, cvBuffer.pData.pValues, nSize);
}

////////////////////////////////////////////////////
// Implementation de la classe PLMsgContext

PLMsgContext::PLMsgContext()
{
	nMsgType = MSGTYPE::UNKNOW;
}
PLMsgContext::~PLMsgContext() {}
