// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Standard.h"
#include "Object.h"
#include "Ermgt.h"

///////////////////////////////////////////////////////////////////////////
// Classe SystemFileDriver
// Classe virtuelle d'acces aux fichiers
// Definit toutes les fonctions necessaires : lecture, ecriture, creation, suppression...
class SystemFileDriver : public Object
{
public:
	// Constructeur
	SystemFileDriver();
	virtual ~SystemFileDriver();

	// Informations generales
	virtual const char* GetDriverName() const = 0;
	virtual const char* GetVersion() const = 0;
	virtual const char* GetScheme() const = 0;

	// Indique si le driver est de type read-only ou read-write
	// S'il est de type read-only, seule une parties des methode sont implementees
	virtual boolean IsReadOnly() const = 0;

	// Connexion et deconnexion
	// Plusieurs connexions successives sont possibles (n'a pas d'effet sur la connexion)
	// Plusieurs deconnexions successives sont possibles (seule la premiere deconnexion est prise en compte)
	// TODO gestion des identifiants de connexion, il faut ajouter un const char* dans le connect pour login/psswd
	// 				mais comment on le renseignera ensuite ?
	//				est-ce qu'il faut ajouter getLogin dans l'api ?
	virtual boolean Connect() = 0; // Pattern singleton
	virtual boolean Disconnect() = 0;
	virtual boolean IsConnected() const = 0;

	// Renvoie la taille de buffer a privilegier lors de l'acces I/O (en octets)
	// Par defaut cette methode renvoie 0
	virtual longint GetSystemPreferredBufferSize() const;

	///////////////////////////////////////////////////////////////////////
	// Methodes a implementer en mode read-only

	// Test d'existence d'un fichier
	virtual boolean FileExists(const char* sFilePathName) const = 0;

	// Test d'existence d'un repertoire
	virtual boolean DirExists(const char* sFilePathName) const = 0;

	// Taile d'un fichier
	virtual longint GetFileSize(const char* sFilePathName) const = 0;

	// Ouverture/fermeture
	// Les modes supportes sont 'r' (lecture), 'w' ou 'a' (ecriture)
	// En mode read-only, seul le mode lecture pourra etre appele
	virtual void* Open(const char* sFilePathName, char cMode) = 0;
	virtual boolean Close(void* stream) = 0;

	// Renvoie le nombre d'octets lus, -1 si il y a eu une erreur
	virtual longint Fread(void* ptr, size_t size, size_t count, void* stream) = 0;

	// Positionnement dans un fichier ouvert en lecture
	virtual boolean SeekPositionInFile(longint lPosition, void* stream) = 0;

	// Copie du systeme de fichier courant vers le systeme de fichier standard
	// L'implementation par defaut utilise Fread et Fwrite
	virtual boolean CopyFileToLocal(const char* sSourceFilePathName, const char* sDestFilePathName);

	// Renvoie le dernier message d'erreur
	virtual const char* GetLastErrorMessage() const = 0;

	///////////////////////////////////////////////////////////////////////
	// Methodes a implementer en mode read-write
	// Ces methodes sont implementees par defaut avec des assert(false)

	// Renvoie le nombre d'octets ecrits, 0 si il y a eu une erreur
	virtual longint Fwrite(const void* ptr, size_t size, size_t count, void* stream) = 0;

	// Flush les donnees, renvoie true si OK
	virtual boolean Flush(void* stream) = 0;

	// Supression d'un fichier
	virtual boolean RemoveFile(const char* sFilePathName) const = 0;

	// Creation/supression d'un repertoire
	virtual boolean MakeDirectory(const char* sPathName) const = 0;
	virtual boolean RemoveDirectory(const char* sFilePathName) const = 0;

	// Taille disponible depuis un repertoire
	// Utile si l'on veut evaluer s'il y a assez de place pour des operations d'ecriture
	virtual longint GetDiskFreeSpace(const char* sPathName) const = 0;

	///////////////////////////////////////////////////////////////////////
	// Methodes disponibles en mode read-write
	// Les methodes virtuelles peuvent etre reimplementees si necessaire

	// Creation d'un fichier vide
	boolean CreateEmptyFile(const char* sPathName);

	// Creation d'une arborescence de repertoires
	boolean MakeDirectories(const char* sPathName) const;

	// Reserve un espace sur le disque pour le fichier ouvert pour eviter
	// de fragmenter le disque lors de l'ecriture de ce fichier
	// Methode avancee optionnelle (ne fait rien par defaut)
	// Renvoie true en cas de succes
	virtual boolean ReserveExtraSize(longint lSize, void* stream);

	// Copie du systeme de fichier courant vers le systeme de fichier standard et inversement
	// L'implementation par defaut utilise fread et fwrite
	virtual boolean CopyFileFromLocal(const char* sSourceFilePathName, const char* sDestFilePathName);

	// Taille maximum du buffer utilise pour la copie
	const static int nMaxBufferSizeForCopying = 64 * lMB;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Indique si un nom de fichier correspond au schema
	virtual boolean IsManaged(const char* sFileName) const;

	// Extraction du nom du fichier a partir de l'URI
	virtual const char* GetURIFilePathName(const char* sFileName) const;

	// Nombre de caracteres du schema, avec bufferisation du resultat
	void GetSchemeCharNumber() const;

	// Nombre de caracteres du nom du schema
	mutable int nSchemeCharNumber;
};

inline void SystemFileDriver::GetSchemeCharNumber() const
{
	if (nSchemeCharNumber == -1)
		nSchemeCharNumber = (int)strlen(GetScheme());
	assert(nSchemeCharNumber == (int)strlen(GetScheme()));
}
