// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "Vector.h"
#include "FileCache.h"
#include "OutputBufferedFile.h"
#include "InputBufferedFile.h"
#include "TaskProgression.h"
#include "PLRemoteFileService.h"
#include "PLParallelTask.h"

///////////////////////////////////////////////////////////////////////////////
// Classe PLFileConcatenater
// Utilitaire de concatenation de fichiers
class PLFileConcatenater : public Object
{
public:
	// Constructeurs
	PLFileConcatenater();
	~PLFileConcatenater();

	// Fichier resultat de la concatenation
	void SetFileName(const ALString& sFileName);
	ALString GetFileName() const;

	// Concatenation des chunks avec envoi des erreurs vers errorSender (facultatif)
	// L'ordre des chunks dans fichier final sera conforme a l'ordre donne par le vecteur
	// Les chunks sont effaces au fur et a mesure ou en cas d'erreur
	// Si il y a assez de ressources, pour la lecture on utilise entre 1 et 8 preferred size
	// et pour l'ecriture on utilise 1 preferred size. Sinon on utilise une taille de bloc (64Ko) pour la lecture et
	// l'ecriture. Seul le processus maitre peut invoquer cette methode Renvoi true si tout s'est bien passe
	boolean Concatenate(const StringVector* svChunkURIs, const Object* errorSender) const;

	// Suppression des fichiers chunks (avec la meme specification de progression que la concatenation)
	// Robuste a l'absence de chunk
	// Seul le processus maitre peut invoquer cette methode
	// Utile lorsque la concatenation a echouee
	void RemoveChunks(const StringVector* svChunkURIs) const;

	// Specification de l'entete du fichier
	// Elle sera ajoutee au debut du fichier si le header est utilise (Cf. SetHeaderLineUsed)
	// Memoire: le vecteur appartient a l'appele
	StringVector* GetHeaderLine();

	// Separateur utilise pour l'ecriture du header (par defaut tabulation)
	void SetFieldSeparator(char cSep);
	char GetFieldSeparator() const;

	// Utilisation d'une ligne d'entete: par defaut true
	void SetHeaderLineUsed(boolean bUsed);
	boolean GetHeaderLineUsed() const;

	// Activation de la progression (inactive par defaut)
	// On peut specifier la debut et la fin de la plage de progression (entre 0 et 1)
	// Si la concatenation represente la premiere moitie du travail par exemple,
	// il faut specifier dProgressionBegin a 0 et dProgressionEnd a 0.5
	// Ainsi la progression se deroulera entre 0 et 50%
	// Par defaut la progression se deroule entre 0 et 100%
	void SetDisplayProgression(boolean bDisplay);
	boolean GetDisplayProgression() const;
	void SetProgressionBegin(double dProgressionBegin);
	double GetProgressionBegin() const;
	void SetProgressionEnd(double dProgressionEnd);
	double GetProgressionEnd() const;

	void SetVerbose(boolean bVerbose);
	boolean GetVerbose() const;

	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation

protected:
	ALString sOutputFileName;
	StringVector svHeaderLine;
	char cSep;
	double dProgressionBegin;
	double dProgressionEnd;
	boolean bDisplayProgression;
	boolean bVerbose;
	boolean bHeaderLineUsed;
};
