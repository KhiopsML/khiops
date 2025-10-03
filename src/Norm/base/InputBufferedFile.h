// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "BufferedFile.h"
#include "Timer.h"
#include "Vector.h"
#include "PLRemoteFileService.h"
#include "MemoryStatsManager.h"

///////////////////////////////////////////////////////////////////////////
// Fichier bufferise en lecture
class InputBufferedFile : public BufferedFile
{
public:
	// Constructeur
	InputBufferedFile();
	~InputBufferedFile();

	// Ouverture en mode read
	boolean Open() override;
	boolean IsOpened() const override;

	// Fermeture du fichier
	boolean Close() override;

	// Copie des specifications (hors fichier et buffer)
	void CopyFrom(const InputBufferedFile* bufferedFile);

	// Taille du fichier
	// Methode utilisable uniquement si le fichier est ouvert
	// Pour obtenir la taille d'un fichier non ouvert, passer par FileService ou PLRemoteFileService
	// Renvoie 0 si probleme d'acces au fichier
	virtual longint GetFileSize() const;

	// Taille du buffer
	void SetBufferSize(int nValue) override;

	/////////////////////////////////////////////////////////////////////////////////
	// Remplissage du buffer avec des lignes entieres
	// La position de depart est de 0 a GetFileSize compris, pour faciliter la gestion des effets de bord
	// Le code retour est false en cas d'erreur de lecture
	// La position de depart est potentiellement modifiee pour sauter une eventuel BOM UTF8 presente end ebut de
	// fichier Apres une lecture, c'est le CurrentBufferSize qui fournit la taille du buffer exploitable

	// Recherche du prochain debut de ligne a partir de lBeginPos
	// On recherche le prochain caractere EOL et on renvoie la position suivante (potentiellement GetFileSize)
	// Le debut de ligne trouve est renseigne lFoundBeginLine, -1 si non trouve (si lBeginPos = GetFileSize)
	// En sortie, le CurrentBuffer est de taille vide et le PositionInFile est lNextLinePos
	// Renvoie false en cas d'erreur
	boolean SearchNextLine(longint& lBeginPos, longint& lNextLinePos);

	// Remplit le buffer avec une seule ligne entiere a partir de lBeginPos
	// Comme pour FillLine, lBeginPos doit etre un debut de ligne.
	// Si necessaire, on peut lire plus que nBufferSize pour remplir une ligne entiere, au plus GetMaxLineLength si
	// necessaire La taille du buffer sera celle de la ligne lue. Si on a pas trouve de fin de ligne avant
	// GetMaxLineLength, le buffer est de taille 0 et bLineTooLong vaut true Renvoie false en cas d'erreur (une
	// ligne diagnostiquee trop longue n'est pas une erreur)
	boolean FillOneLine(longint& lBeginPos, boolean& bLineTooLong);

	// Remplit le buffer avec des lignes entieres a partir de lBeginPos jusqu'a au plus lBeginPos + nBufferSize
	// La position lBeginPos doit etre un debut de ligne, c'est a dire soit le debut du fichier soit
	// le caractere suivant un caractere EOL.
	// Le buffer est rempli a partir de lBeginPos juqu'a la fin de la derniere ligne incluse (fin de fichier,  ou
	// '\\n') se trouvant avant lBeginPos + nBufferSize. La taille du buffer pourra etre plus petite que
	// nBufferSize. Au minimum elle sera 0 si on n'a pas pu trouve une seule ligne entiere. Renvoie false en cas
	// d'erreur
	boolean FillInnerLines(longint& lBeginPos);

	// Remplit le buffer avec des lignes entieres a partir de lBeginPos, si necessaire en depassant la taille du
	// buffer. Si on a pas pu remplir le buffer avec des lignes entieres avec FillInnerLines, on cherche a le
	// remplir avec FillOneLine, potentiellement en depassant la taille du buffer. En cas de ligne trop longue, le
	// buffer est vide, avec PositionInFile positionnee au prochain debut de ligne trouvee La taille du buffer
	// pourra etre plus petite que nBufferSize si on a pu remplir le buffer avec des lignes entieres, plus grande si
	// on a du le remplir avec une seule ligne plus grande que le buffer, ou nulle en cas de lige trop longue
	// Renvoie false en cas d'erreur
	boolean FillOuterLines(longint& lBeginPos, boolean& bLineTooLong);

	// Exemple de lecture d'un fichier pour compter les lignes
	//
	// bOk = ibFile.Open();
	// if (bOk)
	// {
	// 		lFilePos = 0;
	//      lLineNumber = 0;
	// 		while (bOk and lFilePos < ibFile.GetFileSize())
	// 		{
	// 			// Remplissage du buffer
	// 			ibFile.SetBufferSize(nChunkSize); // nChunkSize Peut etre modifie pendant la boucle
	// 			bOk = ibFile.FillOuterLines(lFilePos, bLineTooLong);
	//
	//          // Comptage des lignes du buffer si on pas de ligne trop longue
	//          if (not bLineTooLong)
	//          {
	//          	while (not IsBufferEnd())
	//          	{
	//          		SkipLine();
	//          		lLineNumber++;
	//          }
	//			// Sinon, on compte une ligne pour la ligne trop longue
	//          else
	//				lLineNumber++;
	//
	//			// On se positionne sur la nouvelle position courante dans le fichier
	//       	lFilePos = GetPositionInFile();
	//		}
	//      bOk = ibFile.Close() and bOk;
	// }

	/////////////////////////////////////////////////////////////////////////////////
	// Remplissage du buffer avec des lignes entieres jusqu'a une position de fin

	// Recherche du prochain debut de ligne a partir de lBeginPos jusqu'a au maximum lMaxEndPos non compris
	// On recherche le prochain caractere EOL jusqu'a au maximum lMaxEndPos ou la fin de fichier,
	// et on renvoie la position suivante (potentiellement lMaxEndPos ou GetFileSize)
	// Le debut de ligne trouve est renseigne dans lNextLinePos, -1 si non trouve.
	// En sortie, le CurrentBuffer est de taille vide dans tous les cas.
	// Renvoie false en cas d'erreur
	boolean SearchNextLineUntil(longint& lBeginPos, longint lMaxEndPos, longint& lNextLinePos);

	// Remplit le buffer avec des lignes entieres a partir de lBeginPos jusqu'a au plus max(lBeginPos + nBufferSize,
	// lMaxEndPos) Se comporte comme FillInnerLines, sans avoir besoin de modifier le nBufferSize
	boolean FillInnerLinesUntil(longint& lBeginPos, longint lMaxEndPos);

	// Remplit le buffer avec des lignes entieres a partir de lBeginPos si necessaire en depassant max(lBeginPos +
	// nBufferSize, lMaxEndPos) Se comporte comme FillOuterLines, sans avoir besoin de modifier le nBufferSize En
	// cas de ligne trop longue, le buffer est vide, avec PositionInFile au debut de la ligne suivante s'il est
	// atteint avant lMaxEndPos ou a lMaxEndPos sinon
	boolean FillOuterLinesUntil(longint& lBeginPos, longint lMaxEndPos, boolean& bLineTooLong);

	/////////////////////////////////////////////////////////////////////////////////
	// Remplissage du buffer

	// Taille maximale du buffer dans la methode Fill (2 Go -1 - GetMaxLineLength)
	static int GetMaxBufferSize();

	// Taille minimale du buffer dans la methode Fill (1 bloc : 64 ko)
	static int GetMinBufferSize();

	// Methode de service, ajustant une taille demandee en fonction des contrainte de taille min et max gerees par
	// les buffer Entre GetMinBufferSize et GetMaxBufferSize
	static int FitBufferSize(longint lExpectedSize);

	// Position du debut du buffer dans le fichier
	longint GetBufferStartInFile() const;

	// Indique que l'on a rempli le dernier buffer du fichier
	boolean IsLastBuffer() const;

	// Remplissage efficace du buffer sans tenir compte des debuts et fins de ligne
	// Le remplissage est plus efficace en revanche les methodes de InputBufferedFile sont a utiliser
	// avec precaution (Ne remplit pas le buffer avec des lignes entieres)
	boolean FillBytes(longint& lBeginPos);

	/////////////////////////////////////////////////////////////////////////////////
	// Lecture du buffer
	// Un champ d'un fichier est compris entre deux delimiteurs
	//
	// Lorsqu'un champ debute par un double quote, il doit terminer par un double quote
	// Il peut alors contenir un delimiteur de champs (mais pas de multi-ligne), ou des
	// double quotes s'ils sont doubles
	//
	// Une fois extrait, les caracteres d'espaces sont supprimes en debut et fin de champ
	//
	// Les methodes de lecture par champ ou par ligne peuvent etre associee a des code d'erreurs
	// de facon non verbeuse. C'est a l'appelant de traiter ces erreurs.
	// En fin de ligne uniquement, atteint par des lectures de ligne ou du dernier champ de chaque ligne,
	// on peut detecter qu'un ligne est trop longue.

	// Type d'erreur liees au parsing d'un champs
	// En cas d'erreur, le parsing continue pour rattrapper l'erreur, au mieux en fin de champs,
	// au pire en fin de ligne
	enum
	{
		FieldNoError,                 // Pas d'erreur
		FieldMissingBeginDoubleQuote, // Manque un double quote en debut d'un champ terminant par un double quote
		FieldMissingMiddleDoubleQuote, // double quote non double au milieu d'un champ commencant par un double quote
		FieldMissingEndDoubleQuote, // Manque un double quote en fin d'un champ commencant par un double quote
		FieldTooLong                // Champ trop long (le champ sera tronque)
	};

	// Libelle d'erreur associe a un type d'erreur
	static const ALString GetFieldErrorLabel(int nFieldError);

	// Libelle d'erreur associe a une ligne trop longue
	static const ALString GetLineTooLongErrorLabel();

	// Service d'affichage d'un message standard concernant les erreurs d'encodage avec double quotes manquants
	// Sans effet si pas d'erreur d'encodage
	static void AddEncodingErrorMessage(longint lErrorNumber, const Object* errorSender);

	// Lecture du prochain champ d'une ligne, qui est rendu nettoye des blancs de debut et de fin
	// La chaine en parametre contient en retour le contenu du champ, termine par le caractere '\0'.
	// Le parametre sField est a declarer en variable locale dans la methode appelante.
	// Ce pointeur est positionne en retour vers un buffer gere par InputBufferFile, ce buffer pouvant
	// etre potentiellement de tres grande taille (cf. nMaxFieldSize).
	// ATTENTION a exploiter ce buffer localement a la methode, si necessaire a en recopier le contenu,
	// mais a ne pas memoriser son pointeur, dont le contenu change apres chaque appel.
	// Le flag nFieldError est positionne en cas d'erreur de parsing du champ
	// Code retour a true si le token est le dernier de la ligne, du buffer ou du fichier.
	// Dans ce cas uniquement, on peut avoir un potentiellement une erreur de type LineTooLong
	boolean GetNextField(char*& sField, int& nFieldLength, int& nFieldError, boolean& bLineTooLong);

	// Saut d'un champ
	// Code retour a true si le token est le dernier de la ligne, du buffer ou du fichier.
	// Dans ce cas uniquement, on peut avoir un potentiellement bLineTooLong  true
	// Le flag nFieldError est positionne en cas d'erreur comme pour GetNextField, pour permettre
	// de choisir d'ignorer une ligne en cas d'erreur, que le champ soit parse ou saute
	boolean SkipField(int& nFieldError, boolean& bLineTooLong);

	// Saut de tous les champs jusqu'a la fin de la ligne
	// Sans effet si on est deja sur le dernier champ d'une ligne
	void SkipLastFields(boolean& bLineTooLong);

	// Lecture de la prochaine ligne a partir de la position courante
	// La ligne, qui est une sous-partie du buffer en cours, est alimentee meme si elle est trop longue
	void GetNextLine(CharVector* cvLine, boolean& bLineTooLong);

	// Saut d'une ligne a partir de la position courante
	void SkipLine(boolean& bLineTooLong);

	// Extrait une sous-chaine allant de nBeginPos (inclus) a nEndPos (exclu) dans le buffer
	// Fonctonnalite avancee : ne modifie pas les attributs de la classe
	void ExtractSubBuffer(int nBeginPos, int nEndPos, CharVector* cvSubBuffer) const;

	// On a lu le dernier caractere du buffer
	boolean IsBufferEnd() const;

	// On a lu le dernier caractere du fichier
	// Precisement, on a lu le dernier caractere du derniere buffer
	boolean IsFileEnd() const;

	// Index de la ligne courante dans le buffer, entre 1 et le nombre de ligne du buffer
	// Rend BufferLineNumber+1 si on s'est deplace apres la fin du buffer
	int GetCurrentLineIndex() const;

	// Renvoie le nombre de lignes contenues dans le buffer
	int GetBufferLineNumber() const;

	// Nombre d'erreurs d'encodage detectees impliquant des double quotes manquants
	// Ces erreurs sont detectees lors des appels aux methodes detectant des erreurs
	// Ce nombre d'erreurs est disponible au cours de la lecture du fichier,
	// jusqu'a sa fermeture
	longint GetEncodingErrorNumber() const;

	///////////////////////////////////////////////////////////////////////////////
	// Gestion du BOM UTF8 (byte encoding mask)
	//
	// Alors que les fichiers avec BOM UTF16 ou UTF32 ne sont pas accepte lors de la
	// verification de l'encodage par CheckEncoding(), le BOM UTF8 est tolere, en raison
	// de son inoccuite et de sa presence relativement frequente
	// (emis par les outils microsoft par exemple, comme Excel ou Power BI).
	// On detecte la presnece du BOM UTF8 dans la methode InternalFillBytes, utilisee systematiquement
	// pour toute lecture. Si on a lu le debut du fichier, on teste la presence d'un BOM est
	// on le saute si necessaire. On peut des lors avoir demande une position de debut de lecture
	// a 0 est se retrouve avec une posiiton dans le fichier decalle de la taille du BOM

	// Parametrage de la gestion du BOM UTF8 (default: true)
	// Par defaut, tous les fichier sont traiter avec gestion du BOM UTF8
	// Permet de desactiver la gestion du BOM UTF8 si necessaire, par exemple pour des fichiers de travail
	// censes etre sans BOM UTF8, mais pouvant par hasard contenir des bytes de BOM en debut de fichier
	// que l'on ne souhaite pas ignorer
	// Methode a parametrer avant l'ouverture du fichier
	void SetUTF8BomManagement(boolean bValue);
	boolean GetUTF8BomManagement() const;

	/////////////////////////////////////////////////////////////
	// Information de taille et position, permettant de gerer
	// une progression dans le traitement d'un buffer ou du fichier

	// Position du curseur de lecture dans le buffer
	int GetPositionInBuffer() const;

	// Position du curseur de lecture dans le fichier
	longint GetPositionInFile() const;

	// Test si on est en premiere position du fichier
	// Ce n'est pas necessairement en position 0, car en presence d'un BOM UTF8, on se place
	// juste apres le BOM suite a remplissage du buffer
	boolean IsFirstPositionInFile() const;

	// Verification d'un encodage gerable par la classe
	// Verification de l'absence d'un BOM en tete de fichier UTF8, UTF16, de l'absence de caracteres null '\0' dans
	// le buffer en cours Verification egalement d'un eventuel format Mac anterieur a Mac OS 10 (1998)
	boolean CheckEncoding() const;

	// Taille max des lignes : par defaut 8 MB
	// Si une ligne ne tient pas dans le buffer, la taille du buffer sera automatiquement
	// et temporairement etendue jusqu'a cette taille pour recevoir la ligne en entier.
	// N'est pris en compte que si la taille du buffer est plus petite que la taille max des lignes
	// Les lignes depassant la taille max sont ignorees lors de la lecture (avec warning)
	static int GetMaxLineLength();
	static void SetMaxLineLength(int nValue);

	// Taille maximum des champs
	static const unsigned int nMaxFieldSize = 1000000;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Extraction des champs de la premiere ligne d'un fichier passe en parametre
	// Les controles de validite sont effectue avec emission de messages d'erreur selon le mode silent
	// Les warning ne sont emise qu'en mode verbeux si on on est pas en mode silent
	static boolean GetFirstLineFields(const ALString& sInputFileName, char cInputFieldSeparator, boolean bSilent,
					  boolean bVerbose, StringVector* svFirstLineFields);

	// Format d'affichage d'une valeur, troncature avec ajout de .. si trop longue
	static ALString GetDisplayValue(const ALString& sValue);

	///////////////////////////////////////////////////////////////////////////////////////////
	// Gestion du cache
	// Le cache est une zone memoire de taille PreferredBuferSize utilisee en plus de BufferSize
	// pour acceder moins souvent au fichier

	// Activation/desactivation du cache (par defaut : actif)
	void SetCacheOn(boolean bCacheOn);
	boolean GetCacheOn() const;

	// Methode avancee pour avoir acces au cache en read-only
	// L'integralite du cache ne correspond pas au buffer logique, le partie valide du cache
	// commence a GetBufferStartInCache(); la taille valide etant inputFile.GetCurrentBufferSize()
	const CharVector* GetCache() const;

	// Position du debut du buffer dans le cache
	int GetBufferStartInCache() const;

	////////////////////////////////////////////////////////////////////////
	// Statistiques sur les lectures physiques
	// Ces statistiques sont reinitialisees apres chaque ouverture du fichier
	// et disponibles en permanence, y compris apres la fermeture du fichier

	// Nombre total de lectures physiques
	longint GetTotalPhysicalReadCalls() const;

	// Nombre total d'octets lus
	longint GetTotalPhysicalReadBytes() const;

	///////////////////////////////////////////////////////////////////////////////////////////
	// Test de la classe

	// Generation automatique de plusieurs fichiers ayant des tailles differentes
	// Puis appel de la methode TestReadWrite sur chacun de ces fichiers. Le
	// test consiste a copier le fichier en utilisant la methode FillBytes et a comparer le
	// fichier source et le fichier copie.
	//
	// si nFileType =0, test les fichiers standards
	// si nFileType=1, test les fichiers distant, dans ce cas, le nom des
	//     fichiers generes est prefixe par file://hostname/
	// si nFileType=2, test des fichiers HDFS, dans ce cas les fichiers doivent etre
	//    deja present sur hdfs
	//
	// Pour tester les fichiers distants on doit avoir les instructions suivantes
	// 			PLTaskDriver::SetFileServerOnSingleHost(true);
	//			PLParallelTask::GetDriver()->MasterInitializeResourceSystem();
	//			PLParallelTask::GetDriver()->StartFileServers();
	//			InputBufferedFile::Test(1);
	//			PLParallelTask::GetDriver()->StopFileServers();
	//
	// Pour tester les fichiers sur HDFS, on doit avoir les instructions ssuivantes
	//			SystemFileDriverCreator::SetDriverHDFS(new HDFSFileDriver);
	//			InputBufferedFile::Test(2);
	//			if (HDFSFileSystem::IsConnected())
	// 				HDFSFileSystem::Disconnect();
	//			delete SystemFileDriverCreator::GetDriverHDFS();
	static boolean Test(int nFileType);

	// Test de la methode FillBytes
	// Copie du fichier source en faisant varier la taille des buffers
	// Un diff est effectue sur les 2 fichiers
	// Le fichier de sortie est automatiquement nettoye sauf en cas d'erreur
	// nOverhead est ajoute a la taille du buffer pour avoir des buffer
	// qui ne soient pas un diviseur de la taille du cache
	static boolean TestCopy(const ALString& sSrcFileName, int nFileType, int nOverhead);

	///////////////////////////////////////////////////////////////////////////////////////////
	// Test de la classe en comptant le nombre de ligne avec differentes methodes
	// La lecture est effectuee par chunk de taille fixe
	// Renvoie le nombre de lignes trouvee, ou -1 en cas d'erreur
	// Indique egalement si possible le nombre de lignes top longues

	// Compte le nombre de lignes en utilisant la methode SearchNextLine
	longint TestCountUsingSearchNextLine(int nInputChunkSize);

	// Compte le nombre de lignes en utilisant la methode FillOneLine
	longint TestCountUsingFillOneLine(int nInputChunkSize, longint& lLongLineNumber);

	// Compte le nombre de lignes en utilisant la methode FillInnerLines
	longint TestCountUsingFillInnerLines(int nInputChunkSize, longint& lLongLineNumber);

	// Compte le nombre de lignes en utilisant la methode FillOuterLinesLine
	longint TestCountUsingFillOuterLines(int nInputChunkSize, longint& lLongLineNumber);

	// Compte le nombre de lignes en utilisant la methode FillOuterLinesLine avec des chunks parcouru de la fin vers
	// le debut
	longint TestCountUsingFillOuterLinesBackward(int nInputChunkSize, longint& lLongLineNumber);

	// Test principal des methodes SearchNextLine, FillOneLine, FillInnerLines et FillOuterLines en faisant varier
	// de nombreux parametres de lecture
	static boolean TestCountExtensive();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation

	// Ecrit dans la sortie standard la position des fins de ligne
	// Methode utile pour le debug
	static void WriteEolPos(const ALString& sFileName);

	// Methode pour les test
	// Ecriture d'un fichier de taille lFileSize avec un contenu aleatoire
	// puis appel de la methode TestCopy sur ce fichier
	static boolean TestReadWrite(const ALString& sLabel, int nFileSize, int nFileType);

protected:
	// Variantes d'analyse et de saut des champs, avec gestion d'un double quote en debut de champ
	// Ces methodes sont appelees si un double quote a ete detecte en debut de champ
	// Le methode ne gere que les erreurs liees au double quote.
	// Les champs trop longs sont tronques, mais le message est a emettre par la methode appelante
	// (en testant la valeur de i, position du prochain caractere)
	boolean GetNextDoubleQuoteField(char* sField, int& i, int& nFieldError);
	boolean SkipDoubleQuoteField(int& nFieldError);

	// Test si la ligne courante est trop longue
	boolean IsLineTooLong() const;

	// Prochain caractere
	char GetNextChar();

	// Precedent caractere, et son precedent
	char GetPrevChar() const;
	char GetPrevPrevChar() const;

	// Reinitialisation des donnees de travail
	void Reset();

	// Taille a allouer au buffer (peut etre differente de nBufferSize)
	int GetAllocatedBufferSize() const override;
	void SetAllocatedBufferSize(int nValue) override;

	// Position du curseur de lecture dans le cache
	int GetPositionInCache() const;

	// Taille du cache
	int GetCacheSize() const;

	// Position du debut du cache dans le fichier
	longint GetCacheStartInFile() const;

	// Remplissage efficace du buffer sans tenir compte des debuts et fins de ligne
	// Comme FillBytes, mais en precisant la taille a remplir plutot que d'utiliser nBufferSize
	// On saute le BOM UTF8 si necessaire
	boolean InternalFillBytes(longint& lBeginPos, int nSizeToFill);

	// Variante de InternalFillBytes sans tenir compte du BOM UTF8
	boolean InternalRawFillBytes(longint lBeginPos, int nSizeToFill);

	// Remplissage du cache avec le contenu du fichier
	// le cache est redimensionne et rempli a partir de nPosToCopy
	// Le contenu du fichier copie est situe a lFilePos et est d'une taille de nSizeToCopy
	boolean FillCache(longint lFilePos, int nSizeToCopy, int nPosToCopy);

	// Detection de la presence de BOM UTF8
	boolean DetectUTF8Bom() const;

	// Nombre de caracteres de BOM UTF8 sautes en sours, suite au dernier remplissage du buffer (0 si aucun)
	// Methodes utiles surtout pour les assertions
	int GetUTF8BomSkippedCharNumber() const;

	// Nombre de caracteres de BOM UTF8 sautes (0 si aucun)
	int nUTF8BomSkippedCharNumber;

	// Mode de gestion du BOM UF8
	boolean bUTF8BomManagement;

	// Taille du BOM UTF8
	static const int nUTF8BomSize = 3;

	// Valeur du BOM UTF8
	static const unsigned char cUTF8Bom[nUTF8BomSize];

	///////////////////////////////////////////////////////////////////////////////
	// Declaration des variables d'instance par taille decroissante pour optimiser
	// la taille memoire de l'objet (cf. alignement des variables pour l'OS)

	// Nom du fichier transforme : sans l'URI si le host est localhost
	ALString sLocalFileName;

	// Taille du fichier
	longint lFileSize;

	// Nombre d'erreurs d'encodage detectees impliquant des double quotes manquants
	longint lEncodingErrorNumber;

	// Index de la ligne courante dans le buffer
	int nCurrentLineIndex;

	// Nombre de lignes dans le buffer, calcule apres le remplissage du buffer
	mutable int nBufferLineNumber;

	// Taille a allouer (peut etre different de nBufferSize)
	int nAllocatedBufferSize;

	// Est-ce que le dernier champ lu etait a la fin d'une ligne ou fin de fichier
	boolean bLastFieldReachEol;

	// Taille max des lignes
	static int nMaxLineLength;

	///////////////////////////////////
	// Variables de gestion du cache

	// Position du curseur de lecture dans le cache
	int nPositionInCache;

	// Position du debut du buffer dans le cache
	int nBufferStartInCache;

	// Position du dernier debut de ligne rencontree dans le cache.
	// Permet de detecter les lignes trop longues
	int nLastBolPositionInCache;

	// Position du debut du cache dans le fichier
	longint lCacheStartInFile;

	// Taille du contenu du cache (toujours un multiple de preferred size, sauf fin de fichier)
	int nCacheSize;

	// Cache actif ou non (par defaut actif)
	boolean bCacheOn;

	///////////////////////////////////////////
	// Statistiques sur les lectures physiques

	// Nombre total de lectures physiques
	longint lTotalPhysicalReadCalls;

	// Nombre total d'octets lus
	longint lTotalPhysicalReadBytes;

	// Classes friend pour permettre a la librairie Parallel de gerer les fichiers distants
	friend class PLMPIFileServerSlave; // Serialisation des attributs InputBuffer pour les servers de fichiers
					   // (methode GetCache())
	friend class PLFileConcatenater;   // Ecriture dans HDFS et lecture du buffer
	friend class PLMPISystemFileDriverRemote; // Acces a InternalGetBlockSize
};

///////////////////////
/// Methodes en inline

inline int InputBufferedFile::GetBufferLineNumber() const
{
	require(bIsOpened);

	if (nBufferLineNumber == 0 and nCurrentBufferSize > 0)
	{
		// Calcul du nombre de lignes
		nBufferLineNumber =
		    fcCache.ComputeLineNumber(nBufferStartInCache, nBufferStartInCache + nCurrentBufferSize);

		// Rajout d'une ligne si on est en fin de fichier et que le dernier caractere du cache n'est pas un '\n'
		if (lCacheStartInFile + nBufferStartInCache + nCurrentBufferSize == GetFileSize())
		{
			assert(nBufferStartInCache + nCurrentBufferSize <= fcCache.GetSize());
			if (fcCache.GetAt(nBufferStartInCache + nCurrentBufferSize - 1) != '\n')
				nBufferLineNumber++;
		}
	}
	return nBufferLineNumber;
}

inline longint InputBufferedFile::GetEncodingErrorNumber() const
{
	require(bIsOpened);
	return lEncodingErrorNumber;
}

inline int InputBufferedFile::GetPositionInBuffer() const
{
	return nPositionInCache - nBufferStartInCache;
}

inline longint InputBufferedFile::GetPositionInFile() const
{
	require(lCacheStartInFile != -1);
	return GetBufferStartInFile() + GetPositionInBuffer();
}

inline boolean InputBufferedFile::IsFirstPositionInFile() const
{
	require(lCacheStartInFile != -1);
	return GetPositionInFile() == GetUTF8BomSkippedCharNumber();
}

inline longint InputBufferedFile::GetFileSize() const
{
	require(bIsOpened);
	assert(lFileSize != -1);
	return lFileSize;
}

inline boolean InputBufferedFile::IsFileEnd() const
{
	return GetPositionInFile() >= GetFileSize();
}

inline int InputBufferedFile::GetCurrentLineIndex() const
{
	ensure(1 <= nCurrentLineIndex and nCurrentLineIndex <= GetBufferLineNumber() + 1);
	return nCurrentLineIndex;
}

inline boolean InputBufferedFile::IsLineTooLong() const
{
	return (GetPositionInCache() - nLastBolPositionInCache > GetMaxLineLength());
}

inline char InputBufferedFile::GetNextChar()
{
	char c;
	require(GetPositionInBuffer() < nCurrentBufferSize);
	require(GetPositionInCache() < GetCacheSize());
	c = fcCache.GetAt(nPositionInCache);
	nPositionInCache++;
	return c;
}

inline char InputBufferedFile::GetPrevChar() const
{
	char c;
	require(GetPositionInCache() > 1);
	c = fcCache.GetAt(nPositionInCache - 2);
	return c;
}

inline char InputBufferedFile::GetPrevPrevChar() const
{
	char c;
	require(GetPositionInCache() > 2);
	c = fcCache.GetAt(nPositionInCache - 3);
	return c;
}

inline boolean InputBufferedFile::IsBufferEnd() const
{
	return GetPositionInBuffer() == nCurrentBufferSize;
}

inline void InputBufferedFile::SkipLastFields(boolean& bLineTooLong)
{
	// Saut des champs restant sauf si on est deja sur le dernier champs de la ligne en cours
	if (not bLastFieldReachEol)
	{
		bLastFieldReachEol = true;

		// Recherche de la fin de ligne sans incrementer le numero de ligne
		while (not IsBufferEnd())
		{
			if (GetNextChar() == '\n')
				break;
		}
	}
	bLineTooLong = IsLineTooLong();
	nLastBolPositionInCache = nPositionInCache;
}

inline void InputBufferedFile::SkipLine(boolean& bLineTooLong)
{
	// Incrementation du numero de ligne si on avait atteint la fin de ligne par un GetNextField
	// En effet, le GetNextField modifie l'etat (bLastFieldReachEol), qui est correctement gere
	// par l'appel suivant a GetNextField, et doit l'etre egalement par un appel a SkipLine
	if (not IsBufferEnd() and bLastFieldReachEol)
	{
		nCurrentLineIndex++;
		nLastBolPositionInCache = nPositionInCache;
		bLastFieldReachEol = false;
	}

	// Recherche de la fin de ligne
	while (not IsBufferEnd())
	{
		if (GetNextChar() == '\n')
		{
			nCurrentLineIndex++;
			bLineTooLong = IsLineTooLong();
			nLastBolPositionInCache = nPositionInCache;
			return;
		}
	}

	// Cas particulier de la fin de fichier atteinte sans un dernier caractere '\n'
	if (IsFileEnd() and GetCurrentBufferSize() > 0)
	{
		assert(fcCache.GetAt(nBufferStartInCache + nCurrentBufferSize - 1) != '\n');
		nCurrentLineIndex++;
	}
	bLineTooLong = IsLineTooLong();
	nLastBolPositionInCache = nPositionInCache;
}

inline int InputBufferedFile::GetMaxLineLength()
{
	return nMaxLineLength;
}

inline void InputBufferedFile::SetMaxLineLength(int nValue)
{
	require(0 <= nValue and nValue <= nDefaultBufferSize);
	nMaxLineLength = nValue;
}

inline boolean InputBufferedFile::IsOpened() const
{
	return bIsOpened;
}

inline int InputBufferedFile::GetMaxBufferSize()
{
	return 2 * lGB - 1 - GetMaxLineLength();
}

inline int InputBufferedFile::GetMinBufferSize()
{
	return InternalGetBlockSize();
}

inline int InputBufferedFile::FitBufferSize(longint lExpectedSize)
{
	if (lExpectedSize > GetMaxBufferSize())
		return GetMaxBufferSize();
	if (lExpectedSize < GetMinBufferSize())
		return GetMinBufferSize();
	return int(lExpectedSize);
}

inline longint InputBufferedFile::GetBufferStartInFile() const
{
	return lCacheStartInFile + nBufferStartInCache;
}

inline boolean InputBufferedFile::IsLastBuffer() const
{
	return GetBufferStartInFile() + GetCurrentBufferSize() == GetFileSize();
}

inline int InputBufferedFile::GetAllocatedBufferSize() const
{
	return nAllocatedBufferSize;
}

inline void InputBufferedFile::SetAllocatedBufferSize(int nValue)
{
	nAllocatedBufferSize = nValue;
}

inline int InputBufferedFile::GetPositionInCache() const
{
	return nPositionInCache;
}

inline int InputBufferedFile::GetBufferStartInCache() const
{
	return nBufferStartInCache;
}

inline int InputBufferedFile::GetCacheSize() const
{
	return nCacheSize;
}

inline longint InputBufferedFile::GetCacheStartInFile() const
{
	return lCacheStartInFile;
}

inline int InputBufferedFile::GetUTF8BomSkippedCharNumber() const
{
	return nUTF8BomSkippedCharNumber;
}
