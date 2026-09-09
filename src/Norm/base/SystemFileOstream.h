// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "OutputBufferedFile.h"

//////////////////////////////////////////////////////////////////////////////
// Classe SystemFileOstreamBuffer
// Classe technique d'adaptation entre ostream et OutputBufferedFile
class SystemFileOstreamBuffer : public streambuf
{
protected:
	// Constructeur
	SystemFileOstreamBuffer();
	~SystemFileOstreamBuffer() override;

	// Fichier bufferise de sortie
	void SetOutputBufferedFile(OutputBufferedFile* bufferedFile);
	const OutputBufferedFile* GetOutputBufferedFile() const;

	// Gestion de la synchronisation du buffer lors des flush explicites
	// Lorsque bFlushOnSync est true (valeur par defaut), le buffer est force
	// a se vider lors des flush explicites (stream << flush ou stream << endl)
	void SetFlushOnSync(boolean bValue);
	boolean GetFlushOnSync() const;

	///////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Point d'entree principal : appelee par ostream pour les ecritures en bloc (stream << "string", stream << objet)
	// Plus efficace que overflow() car traite les donnees par blocs entiers
	streamsize xsputn(const char* sValue, streamsize nCharNumber) override;

	// Ecrit un caractere lorsque l'implementation de ostream utilise sputc().
	// Le cas EOF est traite comme une synchronisation du flux.
	int_type overflow(int_type nCharacter) override;

	// Appelee lors de stream << flush ou stream << endl pour forcer la vidage du buffer
	int sync() override;

	OutputBufferedFile* outputBufferedFile;
	boolean bFlushOnSync;

	// Utilisee uniquement par la classe SystemFileOstream
	friend class SystemFileOstream;
};

//////////////////////////////////////////////////////////////////////////////
// Classe SystemFileOstream
// Flux de sortie vers un fichier local ou distant gere par les drivers de SystemFile
//
// Note: lors de l'ecriture sur le cloud il peut etre necessaire de ne pas ecrire dans
// le fichier distant lors de chaque synchronisation (stream << flush ou stream << endl).
// Dans ce cas on peut utiliser SetFlushStandardMode(false) pour desactiver le flush automatique.
class SystemFileOstream : public ostream
{
public:
	// Constructeur
	SystemFileOstream();
	~SystemFileOstream() override;

	// Nom du fichier
	void SetFileName(const ALString& sValue);
	const ALString& GetFileName() const;

	// Ouverture et fermeture du flux
	boolean Open();
	boolean OpenForAppend();
	boolean Close();
	boolean IsOpened() const;

	// Mode de gestion des flush explicites
	// Lorsque le parametre est true (valeur par defaut), le buffer est force
	// a se vider lors des flush explicites (stream << flush ou stream << endl)
	// Cela peut etre pertinent de le positionner a false si on veut optimiser le temps d'ecriture
	// dans un fichier en exploitant au mieux les buffers disponible
	void SetFlushStandardMode(boolean bValue);
	boolean GetFlushStandardMode() const;

	// Test de la classe
	static boolean Test();

	///////////////////////////////////////////////////////////////////
	//// Implementation

protected:
	OutputBufferedFile outputBufferedFile;
	SystemFileOstreamBuffer streamBuffer;
};

////////////////////////////////////
// Methodes en inline

inline int SystemFileOstreamBuffer::sync()
{
	// Securite : si le fichier n'existe pas ou n'est pas ouvert, aucune ecriture
	require(outputBufferedFile != NULL and outputBufferedFile->IsOpened());

	if (bFlushOnSync)
		return outputBufferedFile->Flush() ? 0 : -1;
	else
		return 0;
}
