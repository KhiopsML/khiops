// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemFileOstream.h"

//////////////////////////////////////////////////////
// Implementation de la classe SystemFileOstreamBuffer

SystemFileOstreamBuffer::SystemFileOstreamBuffer()
{
	outputBufferedFile = NULL;
	bFlushOnSync = true;
}

SystemFileOstreamBuffer::~SystemFileOstreamBuffer() {}

void SystemFileOstreamBuffer::SetOutputBufferedFile(OutputBufferedFile* bufferedFile)
{
	outputBufferedFile = bufferedFile;
}

const OutputBufferedFile* SystemFileOstreamBuffer::GetOutputBufferedFile() const
{
	return outputBufferedFile;
}

void SystemFileOstreamBuffer::SetFlushOnSync(boolean bValue)
{
	// On n'autorise pas a changer le mode de synchronisation si le fichier est deja ouvert
	require(outputBufferedFile != NULL and not outputBufferedFile->IsOpened());
	bFlushOnSync = bValue;
}

boolean SystemFileOstreamBuffer::GetFlushOnSync() const
{
	return bFlushOnSync;
}

SystemFileOstreamBuffer::int_type SystemFileOstreamBuffer::overflow(int_type nCharacter)
{
	char cCharacter;
	boolean bOk;

	// Cette methode renvoie traits_type::eof() en cas d'erreur et traits_type::not_eof(nCharacter) en cas de succes
	// Lorsque le caractere a ecire est EOF, on ne fait que synchroniser le flux sans ecrire de caractere

	// Securite : si le fichier n'existe pas ou n'est pas ouvert, aucune ecriture
	require(outputBufferedFile != NULL and outputBufferedFile->IsOpened());

	// Le caractere EOF demande la synchronisation du flux sans ecrire de caractere
	if (traits_type::eq_int_type(nCharacter, traits_type::eof()))
	{
		if (sync() == 0)
			return traits_type::not_eof(nCharacter);
		else
			return traits_type::eof();
	}

	// Ecrit le caractere recu comme un bloc d'un octet pour accepter aussi le caractere "fin de chaine" (pas accepte par Write(char))
	cCharacter = traits_type::to_char_type(nCharacter);
	bOk = outputBufferedFile->Write(&cCharacter, 1);
	if (not bOk)
	{
		return traits_type::eof();
	}
	else
	{
		return traits_type::not_eof(nCharacter);
	}
}

streamsize SystemFileOstreamBuffer::xsputn(const char* sValue, streamsize nCharNumber)
{
	// Note : xsputn() ne met pas a jour l'etat du flux en cas d'echec d'ecriture
	// Il retourne alors un nombre de caracteres inferieur a nCharNumber
	// L'operation d'ecriture de ostream detecte cette ecriture partielle et met a jour son etat
	streamsize nWrittenCharNumber;
	int nLocalCharNumber;

	// Securite : si le fichier n'existe pas ou n'est pas ouvert, aucune ecriture
	require(outputBufferedFile != NULL and outputBufferedFile->IsOpened());

	// Boucle d'ecriture optimisee pour les gros blocs
	// (evite les appels multiples a overflow() pour chaque caractere)
	// OutputBufferedFile::Write(buffer, size) gere le buffering interne
	nWrittenCharNumber = 0;
	while (nWrittenCharNumber < nCharNumber)
	{
		// Limite la taille locale a INT_MAX pour respecter les limites de type
		nLocalCharNumber = (int)min((streamsize)INT_MAX, nCharNumber - nWrittenCharNumber);

		// Ecrit le bloc actuel ; en cas d'echec, arrete la boucle
		if (not outputBufferedFile->Write(sValue + nWrittenCharNumber, nLocalCharNumber))
			break;

		// Avance le pointeur de position
		nWrittenCharNumber += nLocalCharNumber;
	}
	// Retourne le nombre de caracteres reellement ecrits
	// (peut etre < nCharNumber en cas d'erreur disque/quota)
	return nWrittenCharNumber;
}

//////////////////////////////////////////////////////
// Implementation de la classe SystemFileOstream

SystemFileOstream::SystemFileOstream() : ostream(NULL)
{
	streamBuffer.SetOutputBufferedFile(&outputBufferedFile);
	rdbuf(&streamBuffer);
	clear();
}

SystemFileOstream::~SystemFileOstream()
{
	if (IsOpened())
		Close();
}

boolean SystemFileOstream::Open()
{
	boolean bOk;

	require(not IsOpened());

	clear();
	bOk = outputBufferedFile.Open();
	if (not bOk)
		setstate(ios::failbit);
	return bOk;
}

boolean SystemFileOstream::OpenForAppend()
{
	boolean bOk;

	require(not IsOpened());

	clear();
	bOk = outputBufferedFile.OpenForAppend();
	if (not bOk)
		setstate(ios::failbit);
	return bOk;
}

boolean SystemFileOstream::Close()
{
	boolean bOk;

	require(IsOpened());

	bOk = outputBufferedFile.Close();
	if (not bOk)
		setstate(ios::badbit);
	return bOk;
}

void SystemFileOstream::SetFileName(const ALString& sValue)
{
	outputBufferedFile.SetFileName(sValue);
}

const ALString& SystemFileOstream::GetFileName() const
{
	return outputBufferedFile.GetFileName();
}

boolean SystemFileOstream::IsOpened() const
{
	return outputBufferedFile.IsOpened();
}

void SystemFileOstream::SetFlushStandardMode(boolean bValue)
{
	streamBuffer.SetFlushOnSync(bValue);
}

boolean SystemFileOstream::GetFlushStandardMode() const
{
	return streamBuffer.GetFlushOnSync();
}

boolean SystemFileOstream::Test()
{
	boolean bOk;
	boolean bInputFileOpened;
	ALString sFileName;
	SystemFileOstream outputStream;
	SampleObject sampleObject(7, "value");
	fstream inputStream;
	char sBuffer[100];

	sFileName = FileService::BuildFilePathName(FileService::GetTmpDir(), "SystemFileOStreamTest.txt");
	outputStream.SetFileName(sFileName);
	bOk = outputStream.Open();
	if (bOk)
	{
		outputStream << "prefix" << sampleObject << 'X' << std::flush;
		bOk = outputStream.good();
		bOk = outputStream.Close() and bOk;
	}

	bInputFileOpened = false;
	if (bOk)
	{
		bInputFileOpened = FileService::OpenInputFile(sFileName, inputStream);
		bOk = bInputFileOpened;
	}
	if (bInputFileOpened)
	{
		inputStream.getline(sBuffer, sizeof(sBuffer));
		bOk = strcmp("prefix [7,value]X", sBuffer) == 0 and bOk;
		bOk = FileService::CloseInputFile(sFileName, inputStream) and bOk;
	}

	if (FileService::FileExists(sFileName))
		bOk = FileService::RemoveFile(sFileName) and bOk;
	return bOk;
}
