// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MemoryStatsManager.h"
#include "Object.h"
#include "Vector.h"
#include "Ermgt.h"
#include "SystemResource.h"

class FileService;
class FileSpec;

/////////////////////////////////////////////////////////////////////////////
// Classe FileService
// Methodes utilitaires de gestion des fichiers
class FileService : public Object
{
public:
	//////////////////////////////////////////////////////////////////
	// Informations sur les fichiers
	// Pas d'emission de messages d'erreur dans ces methodes

	// Test d'existence d'un fichier (ou d'un directory)
	static boolean Exist(const ALString& sPathName);

	// Test si fichier ou repertoire
	static boolean IsFile(const ALString& sPathName);
	static boolean IsDirectory(const ALString& sPathName);

	// Changement du mode d'un fichier en lecture seulement ou lecture/ecriture sinon
	// Renvoie false si changement non effectue
	static boolean SetFileMode(const ALString& sFilePathName, boolean bReadOnly);

	// Taille d'un fichiers en bytes
	// Peut renvoyer une taille superieure a 4 GB
	// Renvoie 0 si probleme d'acces au fichier
	static longint GetFileSize(const ALString& sFilePathName);

	// Creation d'un fichier vide, ecrasement eventuellement si fichier existant
	static boolean CreateEmptyFile(const ALString& sFilePathName);

	// Suppression d'un fichier
	// Renvoie false en cas d'erreur, mais n'emet pas de message utilisateur
	static boolean RemoveFile(const ALString& sFilePathName);

	//////////////////////////////////////////////////////////////////
	// Methodes utilitaire d'ouverture de fichier,
	// avec emission d'erreur en cas de probleme d'ouverture ou de fermeture

	// Ouverture d'un fichier texte en lecture
	static boolean OpenInputFile(const ALString& sFilePathName, fstream& fst);

	// Ouverture d'un fichier texte en ecriture
	static boolean OpenOutputFile(const ALString& sFilePathName, fstream& fst);
	static boolean OpenOutputFileForAppend(const ALString& sFilePathName, fstream& fst);

	// Fermeture d'un fichier texte en ecriture, avec test de validite
	static boolean CloseInputFile(const ALString& sFilePathName, fstream& fst);
	static boolean CloseOutputFile(const ALString& sFilePathName, fstream& fst);

	//////////////////////////////////////////////////////////////////
	// Gestion des fichiers binaires avec l'API C (plus efficace)

	// Ouverture d'un fichier binaire en lecture
	static boolean OpenInputBinaryFile(const ALString& sFilePathName, FILE*& fFile);

	// Position de l'endroit de lecture dans un fichier ouvert en lecture
	// Renvoie true si pas d'erreur, false sinon (avec message d'erreur technique)
	static boolean SeekPositionInBinaryFile(FILE* fFile, longint lStartPosition);

	// Ouverture d'un fichier binaire en ecriture
	static boolean OpenOutputBinaryFile(const ALString& sFilePathName, FILE*& fFile);
	static boolean OpenOutputBinaryFileForAppend(const ALString& sFilePathName, FILE*& fFile);

	// Reserve de la taille supplementraire sur le disque, avant de commencer ou continuer l'ecriture
	// Methode avancee: a n'utiliser que si vraiment necessaire, dans le cas de fichiers
	// de tres grande taille pour lesquels on souhaite optimiser l'utilisation du disque,
	// en minimisant a priori les risque de fragmentation de fichier
	// Renvoie true si pas d'erreur, false sinon (avec message d'erreur technique)
	static boolean ReserveExtraSize(FILE* fFile, longint lSize);

	// Caractere fin de ligne
	static const ALString& GetEOL();

	// Fermeture d'un fichier binaire en lecture ou ecriture, avec test de validite
	static boolean CloseInputBinaryFile(const ALString& sFilePathName, FILE*& fFile);
	static boolean CloseOutputBinaryFile(const ALString& sFilePathName, FILE*& fFile);

	// Acces au dernier message d'erreur systeme (entre parentheses),
	// pour completer eventuellement un message d'entree-sortie
	// Methode avancee: a utiliser immediatement apres un probleme
	// de l'API systeme (detecte par exemple avec ferror suite a fread ou fwrite)
	static const ALString GetLastSystemIOErrorMessage();

	//////////////////////////////////////////////////////////////////
	// Gestion des repertoires

	// Liste des fichiers et repertoires contenus d'un repertoire
	// Renvoie true si OK
	static boolean GetDirectoryContent(const ALString& sPathName, StringVector* svDirectoryNames,
					   StringVector* svFileNames);

	// Creation d'un repertoire
	// Indique en sortie si le repertoire existe, sans message d'erreur
	static boolean MakeDirectory(const ALString& sPathName);

	// Creation d'un repertoire et si necessaire de tous les repertoire intermediaires
	static boolean MakeDirectories(const ALString& sPathName);

	// Supression d'un repertoire
	// Indique en sortie si le repertoire est supprime, sans message d'erreur
	static boolean RemoveDirectory(const ALString& sPathName);

	///////////////////////////////////////////////////////////////////
	// Methode d'analyse et de manipulation des chemins de fichier
	// Ces methodes sont appliquable sur des chemins complets ou reduits
	// au nom du fichier.
	//    FilePathName = FileName
	//                 | PathName FileName
	//    PathName = FileSeparator
	//             | PathName FileName FileSeparator
	//    FileName = FilePrefix
	//             | FilePrefix . FileSuffix

	// Separateur des fichiers dans les chemins
	static char GetFileSeparator();

	// Indique si un caractere est traite en separateur de fichier dans les chemins
	// Sous windows, le separateur linux est en effet tolere en plus du sperateur windows
	static boolean IsFileSeparator(char c);

	// Indique si un chemin de fichier comporte une partie chemin
	static boolean IsPathInFilePath(const ALString& sFilePathName);

	// Extraction/modification de la partie chemin depuis un chemin de fichier
	// Le getter renvoie le chemin suivi du FileSeparator, ou vide s'il n'y a pas de separateur de le FilePathName
	// Le setter accepte le chemin suivi ou non du FileSeparator
	static const ALString GetPathName(const ALString& sFilePathName);
	static const ALString SetPathName(const ALString& sFilePathName, const ALString& sPathName);

	// Extraction/modification de la partie fichier depuis un chemin de fichier
	static const ALString GetFileName(const ALString& sFilePathName);
	static const ALString SetFileName(const ALString& sFilePathName, const ALString& sFileName);

	// Extraction/modification de la partie prefixe de fichier depuis un chemin de fichier
	// (ce qui precede le dernier '.', ou le nom de fichier complet si pas de '.')
	static const ALString GetFilePrefix(const ALString& sFilePathName);
	static const ALString SetFilePrefix(const ALString& sFilePathName, const ALString& sFilePrefix);

	// Extraction/modification de la partie suffixe de fichier depuis un chemin de fichier
	// (ce qui suit le dernier '.', ou vide si pas de '.')
	static const ALString GetFileSuffix(const ALString& sFilePathName);
	static const ALString SetFileSuffix(const ALString& sFilePathName, const ALString& sFileSuffix);

	// Construction d'un nom de fichier a partir d'un prefixe et suffixe (eventuellement vide)
	static const ALString BuildFileName(const ALString& sFilePrefix, const ALString& sFileSuffix);

	// Construction d'un chemin complet de fichier
	static const ALString BuildFilePathName(const ALString& sPathName, const ALString& sFileName);

	// Indique si un chemin est absolu (comprend la racine)
	static boolean IsAbsoluteFilePathName(const ALString& sFilePathName);

	///////////////////////////////////////////////////////////////////
	// Methode de recherche de fichiers dans une liste de repertoires

	// Separateur de chemins dans le path
	static char GetPathSeparator();

	// Recherche de la liste des repertoires du path
	static const ALString GetPathDirectoryList();

	// Recherche d'un fichier dans une liste de repertoires passes en parametres,
	// ou dans le directory courant
	// (chaine contenant les repertoires, similaire a celle du PATH)
	// Retourne chaine vide si non trouve
	static const ALString GetFilePath(const ALString& sFileName, const ALString& sDirectoryList);

	// Creation d'un nouveau fichier ou d'un repertoire
	// Le nom est fabrique a partir du nom de base specifie, avec suffixage par un index si necessaire
	// Le fichier ou repertoire est cree immediatement, pour le reserver si d'autre exe le demandent en parallele
	// On retourne son nom, ou vide si non cree
	static const ALString CreateNewFile(const ALString& sBaseFilePathName);
	static const ALString CreateNewDirectory(const ALString& sBasePathName);

	///////////////////////////////////////////////////////////////////
	// Specification des repertoires des fichiers temporaires

	// Repertoire des fichiers temporaires
	// Par defaut, c'est le repertoire systeme, sauf si un repertoire utilisateur est specifie
	static const ALString GetTmpDir();

	// Repertoire systeme des fichiers temporaires
	static const ALString GetSystemTmpDir();

	// Nom du repertoire utilisateur des fichiers temporaires (defaut: "")
	static void SetUserTmpDir(const ALString& sPathName);
	static const ALString GetUserTmpDir();

	// Parametrage d'un nom d'application non vide (par defaut: "Default")
	// servant de base a la construction du nom d'un repertoire temporaire applicatif,
	// sous-repertoire du repertoire des fichiers temporaires
	static void SetApplicationName(const ALString& sName);
	static const ALString GetApplicationName();

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gestion des fichiers temporaires applicatifs, detruits automatiquement apres la fin de l'application
	// Le nettoyage des anciens repertoires temporaires applicatifs, actifs ou non, est egalement automatique.
	// Il est recommande d'appeler CreateApplicationTmpDir avant chaque utilisation potentielle des repertoires
	// temporaires (ils peuvent devenir non valides a tout momement), et de reserver l'appel a
	// CheckApplicationTmpDir aux assertions (require....).

	// Creation si necessaire du repertoire utilisateur des fichiers temporaires (si non deja cree),
	// ainsi que du repertoire temporaire applicatif.
	// L'eventuel precedent repertoire applicatif temporaire est prealablement nettoye et detruit.
	// Le repertoire temporaire applicatif est cree avec un nom unique (si plusieurs programme simultanement ou en
	// mode parallele...) sur la base du nom d'application, en sous repertoire du repertoire temporaire On peut
	// rappeler cette methode plusieurs fois si necessaire (ne fait rien si deja cree, tente ou retente une creation
	// avec les specifications des repertoires temporaires sinon). Renvoie false avec message d'erreur si
	// repertoires inexistants apres tentative de creation
	static boolean CreateApplicationTmpDir();

	// Verification du repertoire des fichiers temporaires (existance, possibilite de creation, memoire minimum)
	// Verification egalement du repertoire temporaire applicatif
	// Si erreur, message d'erreur  et retourne false
	static boolean CheckApplicationTmpDir();

	// Acces au repertoire applicatif des fichiers temporaires s'il a ete cree
	// Retourne chaine vide sinon
	static const ALString GetApplicationTmpDir();

	// Creation effective d'un nouveau fichier ou d'un repertoire temporaire
	// dans le repertoire applicatif temporaire a partir d'un nom de base (sans chemin)
	// Le repertoire applicatif temporaire doit avoir ete cree prealablement
	// Le nom est fabrique a partir du nom de base specifie, avec suffixage par un index si necessaire
	// Le fichier ou repertoire est cree immediatement, pour le reserver si d'autre exe le demandent en parallele
	// On retourne le nom du fichier ou repertoire cree, ou vide si non cree
	// En cas d'erreur, un message d'erreur est emis par le errorSender s'il est non NULL
	static const ALString CreateTmpFile(const ALString& sBaseName, const Object* errorSender);
	static const ALString CreateTmpDirectory(const ALString& sBaseName, const Object* errorSender);

	// Creation effective d'un nouveau fichier ou d'un repertoire temporaire
	// dans le repertoire applicatif temporaire a partir d'un nom de base exact (sans chemin)
	// En complement de la methode precedente, il y a une erreur si le fichier ou repertoire existait deja
	static const ALString CreateUniqueTmpFile(const ALString& sBaseName, const Object* errorSender);
	static const ALString CreateUniqueTmpDirectory(const ALString& sBaseName, const Object* errorSender);

	// Pametrage de la destruction automatique du repertoire des fichiers temporaires (par defaut: true)
	// Methode avancee, essentiellement pour le debug des fichiers temporaires, popur garder ceux-ci
	// disponibles apres la fin du programme
	static void SetApplicationTmpDirAutoDeletion(boolean bValue);
	static boolean GetApplicationTmpDirAutoDeletion();

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Nettoyage des repertoires temporaires applicatifs
	// Le repertoire temporaire applicatif est detruit automatiquement en fin de programme.
	// Neanmoins, en cas de plantage, les repertoires applicatifs non detruits peuvent s'accumuler
	// au point d'encombrer les machines.
	// Il est alors possible de specifier une date de peremption pour un repertoire temporaire
	// (stockee dans un fichier anchor, a la racine du repertoire temporaire applicatif).
	// A chaque creation de repertoire temporaire en debut de programme, l'ensemble des repertoires utilisateur
	// passes non detruits est inspecte, et ceux dont la date de prremption a expire sont detruits.

	// Specification d'une date de peremption pour le repertoire temporaire en secondes
	// ecoulees a partir de la (date,heure) courante.
	// Par defaut, les repertoires temporaires sont crese sans date de peremption. Il faut alors appeler
	// cette methode explicitement, potentiellement plusieurs fois si on veut modifier cette date
	// Sans effet si le repertoire temporaire n'existe pas
	static void TouchApplicationTmpDir(int nRemainingSeconds);

	// Annulation de la date de peromption du repertoire temporaire applicatif, qui ne peut plus etre
	// detruit par un autre programme
	static void UntouchApplicationTmpDir();

	///////////////////////////////////////////////////////////////////
	// Services divers

	// Fichier nul: fichier systeme special, qui sert a rediriger ce dont on a pas besoin
	static ALString GetSystemNulFileName();

	// Renvoie true si les deux fichiers sont strictement identiques
	static boolean FileCompare(const ALString& sFileName1, const ALString& sFileName2);

	// Recherche de la memoire disque disponible sur un repertoire
	// (se base sur le repertoire courant "." si le repertoire en parametre est "")
	// Renvoie 0 si erreur ou pas de place disponible
	static longint GetDiskFreeSpace(const ALString& sPathName);

	// Recodage d'un chemin, pour l'affichage "portable" des chemins dans les tests
	// Le nom de chemin est eventuellement remplace par un alias s'il s'agit du
	// repertoire systeme ou applicatif des fichiers temporaires,
	// et les separateurs de chemin par un separateur portable
	static const ALString GetPortableTmpFilePathName(const ALString& sFilePathName);

	///////////////////////////////////////////////////////////////////
	// Gestion des URIs.
	// Il ya 3 formes d'URI possibles, qu'on designe par 'standard', 'remote' et 'hdfs', elles sont de la forme :
	// 		std    	=> 	FilePathName
	//		remote	=> 	"file://" [HostName] "/" FilePathName
	//		hdfs 	=>	"hdfs://" [HostName] "/" FilePathName
	// Il a cependant des tolerances sur la syntaxe et le nombre de '/' utilises
	// Cf. https://en.wikipedia.org/wiki/File_URI_scheme
	// Mais on n'accepte ici que les formes d'URI standard.
	//
	// La partie qui precede ':/' sont appelees 'schema' ou 'scheme'
	// Les methodes precedentes de FileService ne concernent que les URI standard.

	// Renvoie le schema de l'URI: la partie qui precede ":/"
	// (si elle contient au moins deux caracteres pour ne pas prendre en compte les nom de drive sous windows)
	static ALString GetURIScheme(const ALString& sURI);

	// Renvoie true, si l'URI est de la forme standard scheme://[hostname]/filePath
	// ou si c'est un chemin standard
	static boolean IsURIWellFormed(const ALString& sURI);

	// Construction d'une URI a partir du hostname et du fileName
	// Une URI est de la forme "file://host/path"
	static const ALString BuildURI(const ALString& sScheme, const ALString& sHostName,
				       const ALString& sFilePathName);

	// Construction de l'URI du fichier avec le host courant
	// Equivalent a BuildURI(file, GetLocalHostName(), sFileName)
	static const ALString BuildLocalURI(const ALString& sFilePathName);

	// Extraction du hostname a partir de l'URI
	// Renvoie vide si l'URI est mal formee
	static const ALString GetURIHostName(const ALString& sFileURI);

	// Extraction du nom du fichier a partir de l'URI
	// Renvoie le chemin en entree si l'URI est mal formee
	static const ALString GetURIFilePathName(const ALString& sFileURI);

	// Mode d'affichage des libelles utilisateurs d'URI (defaut: true)
	// Si true, on ne rend la partie HostName de l'URI que si le HostName n'est pas local
	// Si false, on rend l'URI complete avec son hostname
	// TODO dans les taches paralleles, il faudrait initialiser automatiquement le smartlabel suivant le type de
	// systeme (cluster ou machine)
	static boolean GetURISmartLabels();
	static void SetURISmartLabels(boolean bValue);

	// Libelle utilisateur d'une URI, selon le mode courant d'affichage (URISmartLabels)
	static const ALString GetURIUserLabel(const ALString& sFileURI);

	// Est-ce que les acces IO sont enregistres par MemoryStatsManager
	// Par defaut a false
	static void SetIOStatsActive(boolean bIsActive);
	static boolean GetIOStatsActive();

	// Est-ce que le log des IO est actif (et MemoryStatManager ouvert)
	static boolean LogIOStats();

	// Test de la classe
	static void Test();

	//////////////////////////////////////////////
	///// Implementation

	// Prefixe des URI des fichiers distants (file)
	static const ALString sRemoteScheme;

protected:
	friend void FileServiceApplicationTmpDirAutomaticRemove();

	// Position de l'endroit de lecture/ecriture dans un fichier ouvert, en specifiant
	// un offset et une positin de reference (SEEK_SET, SEEK_CUR, SEEK_END)
	// Renvoie true si pas d'erreur, false sinon (sans message d'erreur)
	static boolean SystemSeekPositionInBinaryFile(FILE* fFile, longint lOffset, int nWhence);

	// Destruction du repertoire applicatif des fichiers temporaires
	// Cette methode est appelee automatiquement a chaque modification du repertoire
	// ainsi qu'en fin de programme
	static boolean DeleteApplicationTmpDir();

	// Caractere prefixe des fichiers temporaires
	static char GetTmpPrefix();

	// Supression d'un repertoire temporaire et de son contenu a la racine
	// Seul les fichiers et repertoires ayant le caractere prefixe temporaire sont detruits,
	// ce qui evite les catastrophse (cette methode est potentiellement dangereuse)
	// Indique en sortie si le repertoire est supprime, sans message d'erreur
	static boolean DeleteTmpDirectory(const ALString& sTmpPathName);

	///////////////////////////////////////////////////////////////////
	// Gestion du repertoire temporaire applicatif
	// Un fichier "anchor" ouvert en ecriture est associe au repertoire
	// temporaire applicatif, pour montrer aux autres exe qu'il est en
	// cours d'utilisation.
	// Ce fichier est cree et maintenu ouvert suite a creation du repertoire,
	// ferme et detruit avant destruction du repertoire.
	// Lorsque l'on demande la creation d'un repertoire applicatif temporaire,
	// on detruits prealablement les repertoires applicatifs temporaires existant
	// de meme nom (a un suffixe pres) potentiellement issu de precedant exe ayant
	// "plante" sans pouvoir detruire leur repertoire temporaire. Ceci est fait en
	// toute securite en identfiant les repertoire inactifs, dont le fichier anchor
	// est ouvrable en ecriture (car non deja ouvert dans un autre exe).

	// Nettoyage des anciens repertoires applicatifs des fichiers temporaires
	// pour le repertoire passe en parametre et ses variantes au suffixe
	// Tous les repertoire ayant un fichier anchor ayant une date d'expiration
	// anterieure a la date courante sont detruits
	static void CleanExpiredApplicationTmpDirs(const ALString& sExpiredApplicationTmpDir);

	// Nom du fichier anchor
	static ALString GetAnchorFileName();

	// Repertoire utilisateur des fichier temporaires
	static ALString sUserTmpDir;

	// Nom d'apllication, pour le repertoire applicatif des fichiers temporaires
	static ALString sApplicationName;

	// Repertoire applicatif des fichiers temporaires
	static ALString sApplicationTmpDir;

	// Gestion de la fraicheur des specifications et de la creation du repertoire applicatif des fichiers
	// temporaires
	static int nApplicationTmpDirFreshness;
	static int nApplicationTmpDirCreationFreshness;

	// Parametrage de la destruction automatique du repertoire des ficheirs temporaires
	static boolean bApplicationTmpDirAutoDeletion;

	// Flag indiquant si l'on a deja enregistre la fonction de nettoyage automatique des fichiers temporaires
	static boolean bApplicationTmpDirAutomaticRemove;

	// Flag indiquant si l'on affiche le hostName des URI systematique, ou que s'il n'est pas local
	static boolean bURISmartLabels;

	// Repertoire temporaire dedie a HDFS
	static ALString sHDFStmpDir;

	// Flag indiquant si le repertoire temporaire dedie a HDFS a ete cree
	static boolean bIsHDFSTempDirBuilt;

	// Index des noms de fichiers temporaires HDFS
	static int nFileHdfsIndex;

	// Est-ce que les acces IO sont enregistres par MemoryStatsManager
	static boolean bIOStats;

	friend class SystemFileDriverANSI; // Acces a SystemSeekPositionInBinaryFile
};

//////////////////////////////////////////////////////////////////////
// Classe de travail pour des controles generiques de noms de fichier
class FileSpec : public Object
{
public:
	// Constructeur
	FileSpec();
	~FileSpec();

	// Libelle decrivant la nature du fichier
	void SetLabel(const ALString& sValue);
	const ALString& GetLabel() const;

	// Nom complet du fichier
	void SetFilePathName(const ALString& sValue);
	const ALString& GetFilePathName() const;

	// Test (avec emission de message) si le nom de fichier est simple, sans element de chemin
	boolean CheckSimplePath() const;

	// Test (avec emission de message) de difference de path avec un fichier de reference
	boolean CheckReferenceFileSpec(const FileSpec* refFileSpec) const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	ALString sLabel;
	ALString sFilePathName;
};

inline boolean FileService::LogIOStats()
{
	return bIOStats and MemoryStatsManager::IsOpened();
}