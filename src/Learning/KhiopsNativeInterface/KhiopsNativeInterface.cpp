// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define KNI_EXPORTS
#include "KhiopsNativeInterface.h"

// Tous les include se font directement dans le source, pour garder un header C
// independant des librairies Norm et Learning
#include "Standard.h"
#include "KWLearningProject.h"
#include "KWMTDatabaseStream.h"
#include "KNIStream.h"
#include "Timer.h"
#include "RMResourceConstraints.h"
#include "KWKhiopsVersion.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gestion de l'environnement d'apprentissage pour les methodes de l'API
// L'environnement est cree lors de la premiere ouverture de stream, et detruit lors de la
// derniere ouverture de stream
// Les streams sont geres dans un tableau

// Indicateur de methode en cours d'excution, pour eviter les appels reentrants
static boolean bKNIRunningFunction = false;

// Indicateur de creation de l'environnement, a l'aide d'un pointeur sur un projet d'apprentissage
static KWLearningProject* kniEnvLearningProject = NULL;

// Tableau des streams KNI ouverts
// Attention: les streams en sont pas necessairement ranges de facon contigue dans le tableau
static ObjectArray* oaKNIOpenedStreams = NULL;
static int nKNIOpenedStreamNumber = 0;
static int nKNIOpenedStreamCurrentIndex = 0;

// Memoire par defaut pour un stream
static int nKNIStreamMaxMemory = KNI_DefaultMaxStreamMemory;

// Recherche d'un stream
inline KNIStream* KNIGetOpenedStreamAt(int hStream)
{
	require(1 <= hStream and hStream <= KNI_MaxStreamNumber);
	require(oaKNIOpenedStreams != NULL);
	return cast(KNIStream*, oaKNIOpenedStreams->GetAt(hStream - 1));
}

// Verification de la longueur du chaine de caractere (y compris le caractere null de terminaison)
boolean KNICheckString(const char* sString, int nMaxLength)
{
	int i;

	require(nMaxLength > 0);

	// Test du pointeur
	if (sString == NULL)
		return false;
	// Test de la longueur de la chaine
	else
	{
		// Le caractere '\0' doit etre trouve avant la longueur max
		for (i = 0; i < nMaxLength; i++)
		{
			if (sString[i] == '\0')
				return true;
		}
		return false;
	}
}

// Creation de l'environnement d'apprentissage pour l'interface KNI
void KNICreateEnv()
{
	// MemSetAllocIndexExit(70099);

	// Gestion des erreurs en mode silencieux si pas de fichier de log
	// Dans ce cas, aucun message ne doit etre emis lors des appels aux fonctions KNI
	SetLearningModuleName("KNI");
	UIObject::SetUIMode(UIObject::Textual);
	Global::SetSilentMode(Global::GetErrorLogFileName() == "");
	Error::SetDisplayErrorFunction(NULL);

	// Creation de l'environnement si necessaire
	if (nKNIOpenedStreamNumber == 0)
	{
		assert(kniEnvLearningProject == NULL);

		// Creation du projet d'apprentissage
		kniEnvLearningProject = new KWLearningProject;

		// Creation du tableau de streams KNI
		oaKNIOpenedStreams = new ObjectArray;
		oaKNIOpenedStreams->SetSize(KNI_MaxStreamNumber);
		nKNIOpenedStreamCurrentIndex = 0;

		// Initialisation de l'environnement
		kniEnvLearningProject->Begin();

		// On remet la fonction d'affichage des erreur a NULL
		// car l'initialisation de l'environnement repositionne cette fonction
		Error::SetDisplayErrorFunction(NULL);

		// Affectation des handlers pour l'acces au fichiers
		if (not SystemFileDriverCreator::IsExternalDriversRegistered())
			SystemFileDriverCreator::RegisterExternalDrivers();
	}
}

// Destruction de l'environnement d'apprentissage
void KNIDestroyEnv()
{
	// Destruction de l'environnement si necessaire
	if (kniEnvLearningProject != NULL and nKNIOpenedStreamNumber == 0)
	{
		assert(oaKNIOpenedStreams != NULL);

		// Destruction du tableau de streams KNI
		delete oaKNIOpenedStreams;
		oaKNIOpenedStreams = NULL;
		nKNIOpenedStreamCurrentIndex = 0;

		// Terminaison de l'environnement
		kniEnvLearningProject->End();

		// Destruction du projet d'apprentissage
		delete kniEnvLearningProject;
		kniEnvLearningProject = NULL;

		// Suppression des handlers pour l'acces au fichiers
		SystemFileDriverCreator::UnregisterDrivers();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Service de gestion des erreurs
// Par defaut, on est en mode silencieux, avec aucune emission d'erreur. On passe en mode verbeux uniquement
// en cas de specification d'un fichier de log des erreurs (KNISetLogFileName)
// Seule un type d'erreur donne lieu a sortie immediate (par return) des methodes KNI, sans message d'erreur:
//   . la protection contre les appels reentrants, avec erreur KNI_ErrorRunningFunction
// Toutes les autres erreurs sont memorisees dans l'indicateur nRetCode pour une seule sortee en fin de methode.
// Cette sortie est completee par un eventuelle message d'erreur uniquement en mode verbeux.

// Extraction d'une sous partie d'une valeur chaine de caracteres
static const ALString KNIPrintableValue(const ALString& sValue)
{
	ALString sTrimedValue;
	const int nMaxCharNumber = 40;

	// Supression des blancs en debut et fin
	sTrimedValue = sValue;
	sTrimedValue.TrimLeft();
	sTrimedValue.TrimRight();

	// On renvoie un nombre max de caracteres
	if (sTrimedValue.GetLength() <= nMaxCharNumber)
		return sTrimedValue;
	else
		return sTrimedValue.Left(nMaxCharNumber / 2) + " ... " + sTrimedValue.Right(nMaxCharNumber / 2);
}

// Extraction dediee dans le cas d'un record
static const ALString KNIPrintableRecord(const char* sRecord)
{
	if (sRecord == NULL)
		return "NULL";
	else
		return "\"" + KNIPrintableValue(sRecord) + "\"";
}

// Libelle d'une erreur
static const ALString KNIErrorLabel(int nErrorCode)
{
	switch (nErrorCode)
	{
	case KNI_OK:
		return "No error";
	case KNI_ErrorRunningFunction:
		return "Other KNI function currently running: reentrant calls not allowed";
	case KNI_ErrorDictionaryFileName:
		return "Bad dictionary file name";
	case KNI_ErrorDictionaryMissingFile:
		return "Dictionary file does not exist";
	case KNI_ErrorDictionaryFileFormat:
		return "Bad dictionary format: syntax error in dictionary file";
	case KNI_ErrorDictionaryName:
		return "Bad dictionary name";
	case KNI_ErrorMissingDictionary:
		return "Dictionary not found in dictionary file";
	case KNI_ErrorTooManyStreams:
		return "Too many streams: number of simultaneously opened streams exceeds limit";
	case KNI_ErrorStreamHeaderLine:
		return "Bad stream header line";
	case KNI_ErrorFieldSeparator:
		return "Bad field separator";
	case KNI_ErrorStreamHandle:
		return "Bad stream handle: handle does not relate to an opened stream";
	case KNI_ErrorStreamOpened:
		return "Stream opened";
	case KNI_ErrorStreamNotOpened:
		return "Stream not opened";
	case KNI_ErrorStreamInputRecord:
		return "Bad input record: null-termination character not found before maximum string length";
	case KNI_ErrorStreamInputRead:
		return "Problem in reading input record";
	case KNI_ErrorStreamOutputRecord:
		return "Bad output record: output fields require more space than maximum string length";
	case KNI_ErrorMissingSecondaryHeader:
		return "Missing specification of secondary table header";
	case KNI_ErrorMissingExternalTable:
		return "Missing specification of external table";
	case KNI_ErrorDataRoot:
		return "Bad data root for an external table";
	case KNI_ErrorDataPath:
		return "Bad data path";
	case KNI_ErrorDataTableFile:
		return "Bad data table file";
	case KNI_ErrorLoadDataTable:
		return "Problem in loading external data tables";
	case KNI_ErrorMemoryOverflow:
		return "Too much memory currently used";
	case KNI_ErrorStreamOpening:
		return "Stream could not be opened";
	case KNI_ErrorStreamOpeningNotFinished:
		return "Multi-tables stream opening not finished";
	case KNI_ErrorLogFile:
		return "Bad error file";
	default:
		return "Unknown error";
	}
}

// Ajout d'une erreur liee a une methode (les parametres sont separes par des virgules)
static void KNIAddError(int nErrorCode, const ALString& sMethodName, const ALString& sParameters)
{
	if (not Global::GetSilentMode())
		Global::AddError("KNI", sMethodName + "(" + sParameters + ")", KNIErrorLabel(nErrorCode));
}

// Ajout d'une erreur interne a une methode
static void KNIAddInternalError(const ALString& sLabel)
{
	if (not Global::GetSilentMode())
		Global::AddError("KNI", "", sLabel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation des methodes de l'API

KNI_API int KNIGetVersion()
{
	return KNI_VERSION_10_1;
}

KNI_API const char* KNIGetFullVersion()
{
	static ALString sFullVersion;

	// Initialisation la premiere fois, en supprimant tout ce qui n'est ni chiffre, ni '.'
	if (sFullVersion == "")
		sFullVersion = KHIOPS_VERSION;
	return sFullVersion;
}

/* Methode interne d'ouverture d'un stream, reutilisee dans le cas mono-tables et multi-tables
 * Parameters:
 * Handle of stream valide
 * Success return codes :
 *    KNI_OK
 * Failure return codes :
 *    KNI_ErrorStreamOpening
 */
int KNIInternalOpenStream(int hStream)
{
	boolean bShowMemory = false;
	int nRetCode = KNI_OK;
	KWClass* kwcClass;
	KNIStream* kniStream;
	longint lStreamUsedMemory;
	longint lInitialSymbolUsedMemory;
	longint lDeltaSymbolUsedMemory;
	ALString sTmp;

	require(1 <= hStream and hStream <= KNI_MaxStreamNumber);
	require(oaKNIOpenedStreams != NULL and KNIGetOpenedStreamAt(hStream) != NULL);

	// Recherche du stream
	kniStream = KNIGetOpenedStreamAt(hStream);
	check(kniStream);
	assert(not kniStream->GetInputStream()->IsOpenedForRead());
	assert(not kniStream->GetOutputStream()->IsOpenedForRead());

	// On positionne le domaine de classe pour gerer l'ouverture
	kwcClass = kniStream->GetClass();
	KWClassDomain::SetCurrentDomain(kwcClass->GetDomain());

	// Memorisation de l'occupation memoire des symbol avant ouverture du stream
	lInitialSymbolUsedMemory = Symbol::GetAllSymbolsUsedMemory();

	// Affichage de stats memoire
	if (bShowMemory)
	{
		cout << "Symbol memory: " << lInitialSymbolUsedMemory << endl;
		cout << "Domain memory: " << kwcClass->GetDomain()->GetUsedMemory() << endl;
		cout << "Stream memory (before open): " << kniStream->GetUsedMemory() << endl;
	}

	// Ouverture du stream en lecture
	kniStream->GetInputStream()->OpenForRead();
	if (kniStream->GetInputStream()->IsOpenedForRead())
		kniStream->GetOutputStream()->OpenForWrite();
	if (not kniStream->GetInputStream()->IsOpenedForRead() or not kniStream->GetOutputStream()->IsOpenedForWrite())
		nRetCode = KNI_ErrorStreamOpening;

	// Calcul de l'occupation memoire supplementaire due aux symbols
	lDeltaSymbolUsedMemory = Symbol::GetAllSymbolsUsedMemory() - lInitialSymbolUsedMemory;

	// On remet le domaine de classe courant a NULL
	KWClassDomain::SetCurrentDomain(NULL);

	// Test si probleme memoire
	if (nRetCode == KNI_OK)
	{
		lStreamUsedMemory = kniStream->GetUsedMemory() + lDeltaSymbolUsedMemory;
		if (lStreamUsedMemory > kniStream->GetStreamAvailableMemory())
		{
			KNIAddInternalError(
			    sTmp + "Memory requested to open stream exceeds by " +
			    IntToString(int((lStreamUsedMemory - kniStream->GetStreamAvailableMemory()) * 100.0 /
					    kniStream->GetStreamAvailableMemory())) +
			    "% stream memory limit (" + IntToString(kniStream->GetStreamMemoryLimit()) + " MB)");
			nRetCode = KNI_ErrorMemoryOverflow;
		}
		else
			kniStream->SetStreamUsedMemory(lStreamUsedMemory);
	}

	// Affichage de stats memoire
	if (bShowMemory)
	{
		cout << "Delta symbol memory (after open): " << Symbol::GetAllSymbolsUsedMemory() << " -> "
		     << lDeltaSymbolUsedMemory << endl;
		cout << "Stream memory (after open): " << kniStream->GetUsedMemory() << "\t"
		     << kniStream->GetStreamUsedMemory() << endl;
	}

	// Nettoyage si echec
	if (nRetCode != KNI_OK)
	{
		// Destruction du stream KNI
		if (kniStream->GetInputStream()->IsOpenedForRead())
			kniStream->GetInputStream()->Close();
		if (kniStream->GetOutputStream()->IsOpenedForWrite())
			kniStream->GetOutputStream()->Close();
	}

	// Sortie de la fonction, avec son code retour
	ensure(nRetCode < 0 or KNIGetOpenedStreamAt(hStream) != NULL);
	return nRetCode;
}

/* Methode interne de destruction d'un stream
 * Parameters:
 * Handle of stream valide
 */
void KNIInternalDeleteStream(int hStream)
{
	KWClassDomain* kwcdLoadedDomain;
	KWClass* kwcClass;
	KNIStream* kniStream;

	require(1 <= hStream and hStream <= KNI_MaxStreamNumber);
	require(oaKNIOpenedStreams != NULL and KNIGetOpenedStreamAt(hStream) != NULL);

	// Recherche du stream
	kniStream = KNIGetOpenedStreamAt(hStream);
	check(kniStream);
	assert(not kniStream->GetInputStream()->IsOpenedForRead());
	assert(not kniStream->GetOutputStream()->IsOpenedForRead());

	// On restitue la memoire du stream
	KNIStream::SetAllStreamsMemoryLimit(KNIStream::GetAllStreamsMemoryLimit() - kniStream->GetStreamMemoryLimit());
	RMResourceConstraints::SetMemoryLimit(KNIStream::GetAllStreamsMemoryLimit());

	// Destruction de la classe
	kwcClass = kniStream->GetClass();
	kwcdLoadedDomain = kniStream->GetClass()->GetDomain();
	delete kwcdLoadedDomain;

	// Destruction du stream
	delete kniStream;
	oaKNIOpenedStreams->SetAt(hStream - 1, NULL);
	nKNIOpenedStreamNumber--;
}

KNI_API int KNIOpenStream(const char* sDictionaryFileName, const char* sDictionaryName, const char* sStreamHeaderLine,
			  char cFieldSeparator)
{
	int nRetCode;
	int hStream;
	KWClassDomain* kwcdLoadedDomain;
	KWClass* kwcClass;
	ObjectArray oaAllClasses;
	ObjectDictionary odDependentClasses;
	KWClass* kwcUnusedClass;
	KWAttribute* attribute;
	boolean bReadOk;
	KNIStream* kniStream;
	KWMTDatabaseMapping* mapping;
	int i;
	int nPhysicalMemoryReserve;
	int nPhysicalMemoryLimit;
	int nInitialMemoryLimit;
	int nInitialAllStreamsMemoryLimit;
	int nNewAllStreamsMemoryLimit;
	int nFileSize;
	ALString sTmp;

	// Sortie directe si fonction en cours d'execution
	if (bKNIRunningFunction)
		return KNI_ErrorRunningFunction;
	bKNIRunningFunction = true;
	nRetCode = KNI_OK;

	// Creation de l'environnement d'apprentissage si necessaire
	KNICreateEnv();

	// Test si assez de memoire pour gerer un nouveau stream
	// On utilise la contrainte de limite memoire sur la base de la memoire necessaire
	// pour gerer l'ensenmble des streams ouverts
	nInitialMemoryLimit = RMResourceConstraints::GetMemoryLimit();
	nInitialAllStreamsMemoryLimit = KNIStream::GetAllStreamsMemoryLimit();
	if (nRetCode == KNI_OK)
	{
		nNewAllStreamsMemoryLimit = nInitialAllStreamsMemoryLimit + nKNIStreamMaxMemory;

		// Calcul de la memoire physique disponible en MB
		nPhysicalMemoryLimit =
		    int(min(MemGetAdressablePhysicalMemory(), MemGetAvailablePhysicalMemory()) / lMB);

		// Estimation de la reserve memoire systeme, pouvant etre demandee par le systeme
		nPhysicalMemoryReserve = int((MemGetAllocatorReserve() + MemGetPhysicalMemoryReserve() +
					      UIObject::GetUserInterfaceMemoryReserve()) /
					     lMB);

		// Ok si la nouvelle limite memoire est possible
		if (nNewAllStreamsMemoryLimit + nPhysicalMemoryReserve <= nPhysicalMemoryLimit)
		{
			RMResourceConstraints::SetMemoryLimit(nNewAllStreamsMemoryLimit + nPhysicalMemoryReserve);
			KNIStream::SetAllStreamsMemoryLimit(nNewAllStreamsMemoryLimit);
		}
		// Ko si l'on ne peut plus augmenter la limite memoire
		else
		{
			KNIAddInternalError(sTmp + "Total memory necessary to manage all streams (" +
					    IntToString(nNewAllStreamsMemoryLimit + nPhysicalMemoryReserve) +
					    " MB) greater than available RAM (" + IntToString(nPhysicalMemoryLimit) +
					    " MB)");
			nRetCode = KNI_ErrorMemoryOverflow;
		}
	}

	// On continue si OK
	kwcClass = NULL;
	if (nRetCode == KNI_OK)
	{
		// Erreur si trop de streams ouverts
		if (nKNIOpenedStreamNumber >= KNI_MaxStreamNumber)
			nRetCode = KNI_ErrorTooManyStreams;
		// Erreur si nom du fichier dictionnaire invalide
		else if (not KNICheckString(sDictionaryFileName, KNI_MaxPathNameLength))
			nRetCode = KNI_ErrorDictionaryFileName;
		// Erreur si nom du dictionnaire invalide
		else if (not KNICheckString(sDictionaryName, KNI_MaxDictionaryNameLength))
			nRetCode = KNI_ErrorDictionaryName;
		// Erreur si fichier dictionnaire inexistant
		else if (not FileService::FileExists(sDictionaryFileName))
			nRetCode = KNI_ErrorDictionaryMissingFile;
		// Erreur si pas de header line
		else if (not KNICheckString(sStreamHeaderLine, KNI_MaxRecordLength))
			nRetCode = KNI_ErrorStreamHeaderLine;
		// Erreur si separateur blanc
		else if (cFieldSeparator == '\n' or cFieldSeparator == '\0')
			nRetCode = KNI_ErrorFieldSeparator;
	}

	// Test de la taille du fichier dictionnaire
	if (nRetCode == KNI_OK)
	{
		nFileSize = (int)(FileService::GetFileSize(sDictionaryFileName) / lMB);
		if (nFileSize > nKNIStreamMaxMemory)
		{
			KNIAddInternalError(sTmp + "Size of dictionary file " + sDictionaryFileName + " (" +
					    IntToString(nFileSize) + " MB) greater that stream max memory (" +
					    IntToString(nKNIStreamMaxMemory) + " MB)");
			nRetCode = KNI_ErrorMemoryOverflow;
		}
	}

	// Lecture du fichier dictionnaire
	if (nRetCode == KNI_OK)
	{
		// Creation d'un domaine
		kwcdLoadedDomain = new KWClassDomain;

		// Lecture du fichier de dictionnaire
		bReadOk = kwcdLoadedDomain->ReadFile(sDictionaryFileName);

		// Erreur si probleme de lecture du fichier de dictionnaire
		if (not bReadOk)
			nRetCode = KNI_ErrorDictionaryFileFormat;
		// Erreur si dictionnaire charge invalide
		else if (not kwcdLoadedDomain->Check())
			nRetCode = KNI_ErrorDictionaryFileFormat;
		// Erreur si dictionnaire non trouve
		else if (kwcdLoadedDomain->LookupClass(sDictionaryName) == NULL)
			nRetCode = KNI_ErrorMissingDictionary;

		// Nettoyage si erreur
		if (nRetCode != KNI_OK)
		{
			delete kwcdLoadedDomain;
			kwcClass = NULL;
		}
		// Sinon, compilation et memorisation du dictionnaire
		else
		{
			// Recherche du dictionnaire
			kwcClass = kwcdLoadedDomain->LookupClass(sDictionaryName);

			// On passe en Unused toutes les variables dont le type n'est pas Stored (Simple ou Complex)
			// et qui ne sont pas exportable par KNI
			attribute = kwcClass->GetHeadAttribute();
			while (attribute != NULL)
			{
				if (not KWType::IsStored(attribute->GetType()))
					attribute->SetUsed(false);
				kwcClass->GetNextAttribute(attribute);
			}

			// Nettoyage des classes non utiles
			kwcdLoadedDomain->ExportClassArray(&oaAllClasses);
			kwcdLoadedDomain->ComputeClassDependence(kwcClass, &odDependentClasses);
			for (i = 0; i < oaAllClasses.GetSize(); i++)
			{
				kwcUnusedClass = cast(KWClass*, oaAllClasses.GetAt(i));
				if (odDependentClasses.Lookup(kwcUnusedClass->GetName()) == NULL)
					kwcdLoadedDomain->DeleteClass(kwcUnusedClass->GetName());
			}

			// Compilation
			kwcdLoadedDomain->Compile();
		}
	}

	// Creation du stream KNI
	if (nRetCode == KNI_OK)
	{
		assert(kwcClass != NULL);
		assert(kwcClass->IsCompiled());

		// Creation et initialisation du stream KNI
		kniStream = new KNIStream;
		kniStream->SetClass(kwcClass);
		kniStream->GetInputStream()->SetDatabaseName("Input stream" + kwcClass->GetName());
		kniStream->GetInputStream()->SetClassName(kwcClass->GetName());
		kniStream->GetInputStream()->SetFieldSeparator(cFieldSeparator);
		kniStream->GetOutputStream()->SetDatabaseName("Output stream" + kwcClass->GetName());
		kniStream->GetOutputStream()->SetClassName(kwcClass->GetName());
		kniStream->GetOutputStream()->SetFieldSeparator(cFieldSeparator);
		kniStream->SetStreamMemoryLimit(nKNIStreamMaxMemory);

		// Parametrage des tailles de buffer: au minimum pour accueillir un record
		// Les buffer des tables secondaires seront retailles dynamiquement dans la limite de la memoire
		// utilisable
		kniStream->GetInputStream()->SetMaxBufferSize(KNI_MaxRecordLength);
		kniStream->GetOutputStream()->SetMaxBufferSize(KNI_MaxRecordLength);

		// On positionne le domaine de classe pour gerer les mappings
		KWClassDomain::SetCurrentDomain(kwcClass->GetDomain());

		// Mise a jour des mapping pour tenir compte de la structure potentiellement multi-tables du
		// dictionnaires
		kniStream->GetInputStream()->UpdateMultiTableMappings();
		kniStream->GetOutputStream()->UpdateMultiTableMappings();

		// Parametrage des mappings des tables internes en entree avec des pseudo-noms de tables
		for (i = 0; i < kniStream->GetInputStream()->GetTableNumber(); i++)
		{
			mapping =
			    cast(KWMTDatabaseMapping*, kniStream->GetInputStream()->GetMultiTableMappings()->GetAt(i));
			if (not kniStream->GetInputStream()->IsReferencedClassMapping(mapping))
				mapping->SetDataTableName("Input stream " + mapping->GetDataPath());
		}

		// En sortie, parametrage uniquement de la table principale
		assert(kniStream->GetOutputStream()->GetTableNumber() > 0);
		mapping = cast(KWMTDatabaseMapping*, kniStream->GetOutputStream()->GetMultiTableMappings()->GetAt(0));
		mapping->SetDataTableName("Output stream " + mapping->GetDataPath());

		// Parametrage de la ligne d'entete des tables principales
		kniStream->GetInputStream()->SetHeaderLineAt(kwcClass->GetName(), sStreamHeaderLine);
		kniStream->GetOutputStream()->SetHeaderLineAt(kwcClass->GetName(), sStreamHeaderLine);

		// On remet le domaine de classe courant a NULL
		KWClassDomain::SetCurrentDomain(NULL);

		// Recherche du premier emplacement de libre dans le tableau des streams
		// On utilise le nKNIOpenedStreamCurrentIndex pour changer d'emplacement
		// a chaque nouveau dictionnaire (meme s'il a ete libere par Unload), de facon
		// a minimiser le risque qu'un meme handle designe des streams differents
		// au cours du temps
		assert(oaKNIOpenedStreams != NULL);
		hStream = -1;
		for (i = 0; i < KNI_MaxStreamNumber; i++)
		{
			// OK si emplacement libre
			if (oaKNIOpenedStreams->GetAt(nKNIOpenedStreamCurrentIndex) == NULL)
			{
				oaKNIOpenedStreams->SetAt(nKNIOpenedStreamCurrentIndex, kniStream);
				hStream = nKNIOpenedStreamCurrentIndex + 1;
				nKNIOpenedStreamNumber++;
				break;
			}
			// Sinon, passage a l'index suivant
			else
			{
				nKNIOpenedStreamCurrentIndex++;
				nKNIOpenedStreamCurrentIndex = nKNIOpenedStreamCurrentIndex % KNI_MaxStreamNumber;
			}
		}
		assert(hStream >= 1);

		// On memorise  le handle du stream dans le code retour
		// En mono-table, on peut finaliser l'ouverture immediatement dans la methode en cours
		// En multi-tables, il faudra attendre l'appel de KNIFinishOpeningStream
		nRetCode = hStream;

		// Ouverture du stream uniquement dans le cas mono-tables
		// L'ouverture est en effet differee dans le cas multi-tables (cf. KNIFinishOpeningStream)
		if (kniStream->GetInputStream()->GetTableNumber() == 1)
		{
			nRetCode = KNIInternalOpenStream(hStream);

			// Si OK, le code retour est le handle du stream
			if (nRetCode == KNI_OK)
				nRetCode = hStream;
			// Destruction du stream sinon
			else
				KNIInternalDeleteStream(hStream);
		}
	}

	// Destruction de l'environnement d'apprentissage si necessaire
	KNIDestroyEnv();

	// On remet la memoire initiale en cas de probleme
	if (nRetCode < 0)
	{
		RMResourceConstraints::SetMemoryLimit(nInitialMemoryLimit);
		KNIStream::SetAllStreamsMemoryLimit(nInitialAllStreamsMemoryLimit);
	}

	// Emission si neccesaire d'un message d'erreur
	if (nRetCode < 0 and not Global::GetSilentMode())
	{
		KNIAddError(nRetCode, "KNIOpenStream",
			    KNIPrintableValue(sDictionaryFileName) + ", " + KNIPrintableValue(sDictionaryName) + ", " +
				KNIPrintableRecord(sStreamHeaderLine) + ", " + cFieldSeparator);
	}

	// Sortie de la fonction, avec son code retour
	ensure(nRetCode < 0 or KNIGetOpenedStreamAt(nRetCode) != NULL);
	ensure(nRetCode < 0 or KNIGetOpenedStreamAt(nRetCode)->GetClass()->GetName() == sDictionaryName);
	ensure(nRetCode < 0 or
	       KNIGetOpenedStreamAt(nRetCode)->GetInputStream()->GetHeaderLineAt(sDictionaryName) == sStreamHeaderLine);
	bKNIRunningFunction = false;
	return nRetCode;
}

KNI_API int KNICloseStream(int hStream)
{
	int nRetCode;
	KNIStream* kniStream;

	// Sortie directe si fonction en cours d'execution
	if (bKNIRunningFunction)
		return KNI_ErrorRunningFunction;
	bKNIRunningFunction = true;
	nRetCode = KNI_OK;

	// Erreur si le handle est hors limites
	if (hStream < 1 or hStream > KNI_MaxStreamNumber)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si le handle est invalide
	else if (oaKNIOpenedStreams == NULL or KNIGetOpenedStreamAt(hStream) == NULL)
		nRetCode = KNI_ErrorStreamHandle;
	// Destruction du stream
	else
	{
		assert(oaKNIOpenedStreams != NULL and KNIGetOpenedStreamAt(hStream) != NULL);

		// Recherche du stream
		kniStream = KNIGetOpenedStreamAt(hStream);

		// Fermeture si necessaire
		if (kniStream->GetInputStream()->IsOpenedForRead())
			kniStream->GetInputStream()->Close();
		if (kniStream->GetOutputStream()->IsOpenedForWrite())
			kniStream->GetOutputStream()->Close();

		// Destruction et nettoyage
		KNIInternalDeleteStream(hStream);
		ensure(nRetCode < 0 or oaKNIOpenedStreams == NULL or KNIGetOpenedStreamAt(hStream) == NULL);
	}

	// Destruction de l'environnement d'apprentissage si necessaire
	KNIDestroyEnv();

	// Emission si neccesaire d'un message d'erreur
	if (nRetCode < 0 and not Global::GetSilentMode())
	{
		KNIAddError(nRetCode, "KNICloseStream", IntToString(hStream));
	}

	// Sortie de la fonction, avec son code retour
	bKNIRunningFunction = false;
	return nRetCode;
}

KNI_API int KNIRecodeStreamRecord(int hStream, const char* sStreamInputRecord, char* sStreamOutputRecord,
				  int nOutputMaxLength)
{
	int nRetCode;
	KNIStream* kniStream;
	boolean bSecondaryRecordError;
	KWObject* kwoObject;
	boolean bWriteOK;

	// Sortie directe si fonction en cours d'execution
	if (bKNIRunningFunction)
		return KNI_ErrorRunningFunction;
	bKNIRunningFunction = true;
	nRetCode = KNI_OK;
	if (sStreamOutputRecord != NULL)
		sStreamOutputRecord[0] = '\0';

	// Erreur si le handle est hors limites
	if (hStream < 1 or hStream > KNI_MaxStreamNumber)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si le handle est invalide
	else if (oaKNIOpenedStreams == NULL or KNIGetOpenedStreamAt(hStream) == NULL)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si record d'entree mal specifie
	else if (not KNICheckString(sStreamInputRecord, KNI_MaxRecordLength))
		nRetCode = KNI_ErrorStreamInputRecord;
	// Erreur si record de sortie manquant
	else if (sStreamOutputRecord == NULL)
		nRetCode = KNI_ErrorStreamOutputRecord;
	// Erreur si taille de sortie inferieure a 0
	else if (nOutputMaxLength <= 0)
		nRetCode = KNI_ErrorStreamOutputRecord;
	// Recodage du record a l'aide du stream
	else
	{
		assert(oaKNIOpenedStreams != NULL and KNIGetOpenedStreamAt(hStream) != NULL);

		// Recherche du stream
		kniStream = KNIGetOpenedStreamAt(hStream);

		// Test si stream ouvert
		if (not kniStream->GetInputStream()->IsOpenedForRead())
		{
			assert(not kniStream->GetOutputStream()->IsOpenedForWrite());
			if (kniStream->GetInputStream()->GetTableNumber() == 1)
				nRetCode = KNI_ErrorStreamNotOpened;
			else
				nRetCode = KNI_ErrorStreamOpeningNotFinished;
		}

		// Recodage si licence OK
		if (nRetCode == KNI_OK)
		{
			// On memorise le cas ou il y a eu des erreurs sur les records secondaires
			bSecondaryRecordError = kniStream->GetInputStream()->GetSecondaryRecordError();

			// Lecture par analyse du record d'entree
			// On la fait meme en cas d'erreur, pour "nettoyer" les buffers
			kwoObject = kniStream->GetInputStream()->ReadFromBuffer(sStreamInputRecord);

			// Erreur si probleme de lecture
			sStreamOutputRecord[0] = '\0';
			if (kwoObject == NULL)
				nRetCode = KNI_ErrorStreamInputRead;
			// Erreur de lecture
			else if (kniStream->GetInputStream()->IsError() or bSecondaryRecordError)
			{
				nRetCode = KNI_ErrorStreamInputRead;

				// Destruction de l'objet lu
				delete kwoObject;
			}
			// Sinon, ecriture dans le record de de sortie
			else
			{
				// Ecriture
				bWriteOK = kniStream->GetOutputStream()->WriteToBuffer(kwoObject, sStreamOutputRecord,
										       nOutputMaxLength);
				if (not bWriteOK)
					nRetCode = KNI_ErrorStreamOutputRecord;

				// Destruction de l'objet lu
				delete kwoObject;
			}
		}
	}

	// Emission si neccesaire d'un message d'erreur
	if (nRetCode < 0 and not Global::GetSilentMode())
	{
		ALString sTmp;
		KNIAddError(nRetCode, "KNIRecodeStreamRecord",
			    sTmp + IntToString(hStream) + ", " + KNIPrintableRecord(sStreamInputRecord) + ", " +
				KNIPrintableRecord(sStreamOutputRecord) + ", " + IntToString(nOutputMaxLength));
	}

	// Sortie de la fonction, avec son code retour
	bKNIRunningFunction = false;
	return nRetCode;
}

/////////////////////////////////////////////////////////////////////////////////////////
// API multi-tables

KNI_API int KNISetSecondaryHeaderLine(int hStream, const char* sDataPath, const char* sStreamSecondaryHeaderLine)
{
	int nRetCode;
	KNIStream* kniStream;
	KWMTDatabaseMapping* mapping;
	ALString sFullDataPath;

	// Sortie directe si fonction en cours d'execution
	if (bKNIRunningFunction)
		return KNI_ErrorRunningFunction;
	bKNIRunningFunction = true;
	nRetCode = KNI_OK;

	// Erreur si le handle est hors limites
	if (hStream < 1 or hStream > KNI_MaxStreamNumber)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si le handle est invalide
	else if (oaKNIOpenedStreams == NULL or KNIGetOpenedStreamAt(hStream) == NULL)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si pas de header line
	else if (not KNICheckString(sStreamSecondaryHeaderLine, KNI_MaxRecordLength))
		nRetCode = KNI_ErrorStreamHeaderLine;

	// Test si stream non deja ouvert
	kniStream = NULL;
	if (nRetCode == KNI_OK)
	{
		assert(oaKNIOpenedStreams != NULL and KNIGetOpenedStreamAt(hStream) != NULL);

		// Recherche du stream et de sa classe
		kniStream = KNIGetOpenedStreamAt(hStream);

		// Erreur si stream deja ouvert
		if (kniStream->GetInputStream()->IsOpenedForRead())
		{
			assert(kniStream->GetOutputStream()->IsOpenedForWrite());
			nRetCode = KNI_ErrorStreamOpened;
		}

		// Recherche du mapping
		mapping = NULL;
		sFullDataPath = kniStream->GetInputStream()->GetClassName() + '`' + sDataPath;
		if (nRetCode == KNI_OK)
		{
			mapping = kniStream->GetInputStream()->LookupMultiTableMapping(sFullDataPath);
			if (mapping == NULL)
				nRetCode = KNI_ErrorDataPath;
		}

		// Memorisation de la ligne de header
		if (nRetCode == KNI_OK)
		{
			kniStream->GetInputStream()->SetHeaderLineAt(sFullDataPath, sStreamSecondaryHeaderLine);
			kniStream->GetOutputStream()->SetHeaderLineAt(sFullDataPath, sStreamSecondaryHeaderLine);
		}
	}

	// Emission si neccesaire d'un message d'erreur
	if (nRetCode < 0 and not Global::GetSilentMode())
	{
		ALString sTmp;
		KNIAddError(nRetCode, "KNISetSecondaryHeaderLine",
			    sTmp + IntToString(hStream) + ", " + KNIPrintableRecord(sDataPath) + ", " +
				KNIPrintableRecord(sStreamSecondaryHeaderLine));
	}

	// Sortie de la fonction, avec son code retour
	bKNIRunningFunction = false;
	return nRetCode;
}

KNI_API int KNISetExternalTable(int hStream, const char* sDataRoot, const char* sDataPath,
				const char* sDataTableFileName)
{
	int nRetCode;
	KNIStream* kniStream;
	KWMTDatabaseMapping* mapping;
	ALString sFullDataPath;

	// Sortie directe si fonction en cours d'execution
	if (bKNIRunningFunction)
		return KNI_ErrorRunningFunction;
	bKNIRunningFunction = true;
	nRetCode = KNI_OK;

	// Erreur si le handle est hors limites
	if (hStream < 1 or hStream > KNI_MaxStreamNumber)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si le handle est invalide
	else if (oaKNIOpenedStreams == NULL or KNIGetOpenedStreamAt(hStream) == NULL)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si pas de table externe
	else if (not KNICheckString(sDataTableFileName, KNI_MaxRecordLength))
		nRetCode = KNI_ErrorDataTableFile;

	// Test si stream non deja ouvert
	kniStream = NULL;
	if (nRetCode == KNI_OK)
	{
		assert(oaKNIOpenedStreams != NULL and KNIGetOpenedStreamAt(hStream) != NULL);

		// Recherche du stream et de sa classe
		kniStream = KNIGetOpenedStreamAt(hStream);

		// Erreur si stream deja ouvert
		if (kniStream->GetInputStream()->IsOpenedForRead())
		{
			assert(kniStream->GetOutputStream()->IsOpenedForWrite());
			nRetCode = KNI_ErrorStreamOpened;
		}

		// Recherche du mapping
		mapping = NULL;
		sFullDataPath = sDataRoot;
		if (sDataPath[0] != '\0')
		{
			sFullDataPath += '`';
			sFullDataPath += sDataPath;
		}
		if (nRetCode == KNI_OK)
		{
			// Recherche d'abord de la table externe racine, qui doit etre presente
			mapping = kniStream->GetInputStream()->LookupMultiTableMapping(sDataRoot);
			if (mapping == NULL)
				nRetCode = KNI_ErrorDataRoot;
			// Si OK et table externe secondaire, recherche de celle-ci
			else if (sDataPath[0] != '\0')
			{
				mapping = kniStream->GetInputStream()->LookupMultiTableMapping(sFullDataPath);
				if (mapping == NULL)
					nRetCode = KNI_ErrorDataPath;
			}
		}

		// Memorisation de la ligne de header
		if (nRetCode == KNI_OK)
		{
			mapping->SetDataTableName(sDataTableFileName);

			// Idem pour la base en sortie
			mapping = kniStream->GetOutputStream()->LookupMultiTableMapping(sFullDataPath);
			check(mapping);
			mapping->SetDataTableName(sDataTableFileName);
		}
	}

	// Emission si neccesaire d'un message d'erreur
	if (nRetCode < 0 and not Global::GetSilentMode())
	{
		ALString sTmp;
		KNIAddError(nRetCode, "KNISetExternalTable",
			    sTmp + IntToString(hStream) + ", " + KNIPrintableRecord(sDataRoot) + ", " +
				KNIPrintableRecord(sDataPath) + ", " + KNIPrintableRecord(sDataTableFileName));
	}

	// Sortie de la fonction, avec son code retour
	bKNIRunningFunction = false;
	return nRetCode;
}

KNI_API int KNIFinishOpeningStream(int hStream)
{
	int nRetCode;
	KWClass* kwcClass;
	KNIStream* kniStream;
	KWMTDatabaseMapping* mapping;
	int i;

	// Sortie directe si fonction en cours d'execution
	if (bKNIRunningFunction)
		return KNI_ErrorRunningFunction;
	bKNIRunningFunction = true;
	nRetCode = KNI_OK;

	// Erreur si le handle est hors limites
	if (hStream < 1 or hStream > KNI_MaxStreamNumber)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si le handle est invalide
	else if (oaKNIOpenedStreams == NULL or KNIGetOpenedStreamAt(hStream) == NULL)
		nRetCode = KNI_ErrorStreamHandle;

	// Test si stream non deja ouvert
	kwcClass = NULL;
	kniStream = NULL;
	if (nRetCode == KNI_OK)
	{
		assert(oaKNIOpenedStreams != NULL and KNIGetOpenedStreamAt(hStream) != NULL);

		// Recherche du stream et de sa classe
		kniStream = KNIGetOpenedStreamAt(hStream);
		kwcClass = kniStream->GetClass();

		// Erreur si stream deja ouvert
		if (kniStream->GetInputStream()->IsOpenedForRead())
		{
			assert(kniStream->GetOutputStream()->IsOpenedForWrite());
			nRetCode = KNI_ErrorStreamOpened;
		}

		// Verification du parametrage complet du stream
		if (nRetCode == KNI_OK)
		{
			// Verification de la specification complete des mapping en entree (header ou table externe)
			for (i = 1; i < kniStream->GetInputStream()->GetTableNumber(); i++)
			{
				mapping = cast(KWMTDatabaseMapping*,
					       kniStream->GetInputStream()->GetMultiTableMappings()->GetAt(i));

				// Verification du fichier de donnees pour les tables externes
				if (kniStream->GetInputStream()->IsReferencedClassMapping(mapping))
				{
					if (mapping->GetDataTableName() == "")
					{
						nRetCode = KNI_ErrorMissingExternalTable;
						mapping->AddError("Missing data table file for external table");
						break;
					}
				}
				// Verfication du header pour les tables secondaires
				else
				{
					if (kniStream->GetInputStream()->GetHeaderLineAt(mapping->GetDataPath()) == "")
					{
						nRetCode = KNI_ErrorMissingSecondaryHeader;
						mapping->AddError("Missing header line for secondary input table");
						break;
					}
				}
			}

			// Verification de la specification complete des mapping en sortie
			// En principe, ne devrait pas poser de probleme si c'est correct pour les mapping en entree
			if (nRetCode == KNI_OK)
			{
				for (i = 1; i < kniStream->GetOutputStream()->GetTableNumber(); i++)
				{
					mapping = cast(KWMTDatabaseMapping*,
						       kniStream->GetOutputStream()->GetMultiTableMappings()->GetAt(i));

					// Verification du fichier de donnees pour les tables externes
					if (kniStream->GetOutputStream()->IsReferencedClassMapping(mapping))
					{
					}
					// Verfication du header pour les tables secondaires
					else
					{
						if (kniStream->GetOutputStream()->GetHeaderLineAt(
							mapping->GetDataPath()) == "")
						{
							nRetCode = KNI_ErrorMissingSecondaryHeader;
							mapping->AddError(
							    "Missing header line for secondary output table");
							break;
						}
					}
				}
			}
		}

		// Ouverture du stream
		if (nRetCode == KNI_OK)
			nRetCode = KNIInternalOpenStream(hStream);

		// Destruction du stream si erreur
		if (nRetCode != KNI_OK)
			KNIInternalDeleteStream(hStream);
		ensure(nRetCode == KNI_OK or KNIGetOpenedStreamAt(hStream) == NULL);
		ensure(nRetCode < 0 or KNIGetOpenedStreamAt(hStream) != NULL);
		ensure(nRetCode < 0 or KNIGetOpenedStreamAt(hStream)->GetInputStream()->IsOpenedForRead());
	}

	// Destruction de l'environnement d'apprentissage si necessaire
	KNIDestroyEnv();

	// Emission si neccesaire d'un message d'erreur
	if (nRetCode < 0 and not Global::GetSilentMode())
	{
		KNIAddError(nRetCode, "KNIFinishOpeningStream", IntToString(hStream));
	}

	// Sortie de la fonction, avec son code retour
	bKNIRunningFunction = false;
	return nRetCode;
}

KNI_API int KNISetSecondaryInputRecord(int hStream, const char* sDataPath, const char* sStreamSecondaryInputRecord)
{
	int nRetCode;
	KNIStream* kniStream;
	KWMTDatabaseMapping* mapping;
	int nMappingMaxBufferSize;
	int nNewMappingMaxBufferSize;
	boolean bOk;

	// Sortie directe si fonction en cours d'execution
	if (bKNIRunningFunction)
		return KNI_ErrorRunningFunction;
	bKNIRunningFunction = true;
	nRetCode = KNI_OK;

	// Erreur si le handle est hors limites
	if (hStream < 1 or hStream > KNI_MaxStreamNumber)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si le handle est invalide
	else if (oaKNIOpenedStreams == NULL or KNIGetOpenedStreamAt(hStream) == NULL)
		nRetCode = KNI_ErrorStreamHandle;
	// Erreur si record d'entree mal specifie
	else if (not KNICheckString(sStreamSecondaryInputRecord, KNI_MaxRecordLength))
		nRetCode = KNI_ErrorStreamInputRecord;
	// Memorisation du record dans le stream secondaire en vue du recodage
	else
	{
		assert(oaKNIOpenedStreams != NULL and KNIGetOpenedStreamAt(hStream) != NULL);

		// Recherche du stream
		kniStream = KNIGetOpenedStreamAt(hStream);

		// Test si stream ouvert
		if (not kniStream->GetInputStream()->IsOpenedForRead())
		{
			assert(not kniStream->GetOutputStream()->IsOpenedForWrite());
			nRetCode = KNI_ErrorStreamOpeningNotFinished;
		}

		// Recherche du mapping
		mapping = NULL;
		if (nRetCode == KNI_OK)
		{
			mapping = kniStream->GetInputStream()->LookupMultiTableMapping(
			    kniStream->GetInputStream()->GetClassName() + '`' + sDataPath);
			if (mapping == NULL)
				nRetCode = KNI_ErrorDataPath;
		}

		// Recherche si l'on est deja en erreur sur la gestion des enregistrements secondaires
		if (nRetCode == KNI_OK)
		{
			if (kniStream->GetInputStream()->GetSecondaryRecordError())
				nRetCode = KNI_ErrorMemoryOverflow;
		}

		// Memorisation du record secondaire, en tenant compte des contraintes de memoire
		if (nRetCode == KNI_OK)
		{
			bOk = kniStream->GetInputStream()->SetSecondaryRecordAt(mapping, sStreamSecondaryInputRecord);

			// Deuxieme tentative si erreur (du a manque de memoire du buffer du mapping)
			if (not bOk)
			{
				if (kniStream->GetStreamUsedMemory() < kniStream->GetStreamAvailableMemory())
				{
					// Determination de la nouvelle taille du buffer du mapping
					nMappingMaxBufferSize =
					    kniStream->GetInputStream()->GetMappingMaxBufferSize(mapping);
					nNewMappingMaxBufferSize = nMappingMaxBufferSize;
					if (nMappingMaxBufferSize <=
					    INT_MAX / 3) // Au dela de 1 Go, on n'agrandit plus les buffers
					{
						// Doublement de la taille si possible
						if (nMappingMaxBufferSize < kniStream->GetStreamAvailableMemory() -
										kniStream->GetStreamUsedMemory())
							nNewMappingMaxBufferSize = nMappingMaxBufferSize * 2;
						// Sinon, agrandissement dans la limite de la memoire disponible pour le
						// stream
						else
						{
							assert(nMappingMaxBufferSize +
								   kniStream->GetStreamAvailableMemory() -
								   kniStream->GetStreamUsedMemory() <
							       INT_MAX);
							nNewMappingMaxBufferSize =
							    nMappingMaxBufferSize +
							    int(kniStream->GetStreamAvailableMemory() -
								kniStream->GetStreamUsedMemory());
						}
						assert(nNewMappingMaxBufferSize >= nMappingMaxBufferSize);
					}

					// Nouvelle tentative de recodage si agrandissement possible
					if (nNewMappingMaxBufferSize > nMappingMaxBufferSize)
					{
						kniStream->GetInputStream()->SetMappingMaxBufferSize(
						    mapping, nNewMappingMaxBufferSize);
						kniStream->SetStreamUsedMemory(
						    kniStream->GetStreamUsedMemory() +
						    (nNewMappingMaxBufferSize - nMappingMaxBufferSize));
						bOk = kniStream->GetInputStream()->SetSecondaryRecordAt(
						    mapping, sStreamSecondaryInputRecord);
					}
				}
			}
			if (not bOk)
			{
				kniStream->GetInputStream()->SetSecondaryRecordError(true);
				nRetCode = KNI_ErrorMemoryOverflow;
			}
		}
	}

	// Emission si neccesaire d'un message d'erreur
	if (nRetCode < 0 and not Global::GetSilentMode())
	{
		ALString sTmp;
		KNIAddError(nRetCode, "KNISetSecondaryInputRecord",
			    sTmp + IntToString(hStream) + ", " + KNIPrintableRecord(sDataPath) + ", " +
				KNIPrintableRecord(sStreamSecondaryInputRecord));
	}

	// Sortie de la fonction, avec son code retour
	bKNIRunningFunction = false;
	return nRetCode;
}

KNI_API int KNIGetStreamMaxMemory()
{
	return nKNIStreamMaxMemory;
}

KNI_API int KNISetStreamMaxMemory(int nMaxMB)
{
	int nPhysicalMemoryLimit;
	int nPhysicalMemoryReserve;

	// On tronque si necessaire a la valeur minimum
	if (nMaxMB < KNI_DefaultMaxStreamMemory)
		nKNIStreamMaxMemory = KNI_DefaultMaxStreamMemory;
	else
		nKNIStreamMaxMemory = nMaxMB;

	// Calcul de la memoire physique disponible en MB
	nPhysicalMemoryLimit = int(min(MemGetAdressablePhysicalMemory(), MemGetAvailablePhysicalMemory()) / lMB);

	// Estimation de la reserve memoire systeme, pouvant etre demandee par le systeme
	nPhysicalMemoryReserve =
	    int((MemGetAllocatorReserve() + MemGetPhysicalMemoryReserve() + UIObject::GetUserInterfaceMemoryReserve()) /
		lMB);

	// On tronque par la memoire totale disponible (en excluant la reserve systeme)
	if (nKNIStreamMaxMemory > nPhysicalMemoryLimit - nPhysicalMemoryReserve)
		nKNIStreamMaxMemory = nPhysicalMemoryLimit - nPhysicalMemoryReserve;
	return nKNIStreamMaxMemory;
}

KNI_API int KNISetLogFileName(const char* sLogFileName)
{
	boolean bOk;
	if (sLogFileName == NULL)
		return KNI_ErrorLogFile;
	else
	{
		bOk = Global::SetErrorLogFileName(sLogFileName);
		Global::SetSilentMode(Global::GetErrorLogFileName() == "");
		Error::SetDisplayErrorFunction(NULL);
		if (not bOk)
			return KNI_ErrorLogFile;
		else
			return KNI_OK;
	}
}
