// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataTableDriver;

#include "KWClass.h"
#include "KWObject.h"
#include "KWDate.h"
#include "KWTime.h"
#include "KWTimestamp.h"
#include "Vector.h"
#include "TaskProgression.h"
#include "Timer.h"
#include "KWObjectKey.h"

///////////////////////////////////////////////////////////
// Service de gestion d'une table de donnees de KWObject
// Il s'agit d'une classe virtuelle, comportant la
// definition des services generiques de table de donnees.
// Ces services doivent etre reimplementes dans une sous-classe.
class KWDataTableDriver : public Object
{
public:
	// Constructeur
	KWDataTableDriver();
	~KWDataTableDriver();

	//////////////////////////////////////////////////////////////////
	// Duplication
	// Seules les specifications sont dupliquees, pas les objets

	// Duplication (s'appuie sur Create et CopyFrom)
	KWDataTableDriver* Clone() const;

	// Creation pour renvoyer une instance du meme type dynamique
	virtual KWDataTableDriver* Create() const;

	// Recopie des attributs de definition de la table
	// Peut eventuellement etre redefini si necessaire
	// Prerequis: la table cible doit etre vide
	virtual void CopyFrom(const KWDataTableDriver* kwdtdSource);

	// Comparaison des attributs de definition avec une autre table du meme type
	virtual int Compare(const KWDataTableDriver* kwdtdSource) const;

	/////////////////////////////////////////////////////////////////////
	// Parametrage obligatoire pour l'utilisation de la table

	// Nom de la table de donnees
	void SetDataTableName(const ALString& sValue);
	const ALString& GetDataTableName() const;

	// Classe associee pour les lectures/ecritures
	// Il est a noter qu'en lecture/ecriture, seuls les attributs
	// de type Loaded seront geres.
	// La classe correspondante doit etre valide et compilee
	void SetClass(const KWClass* kwcValue);
	const KWClass* GetClass() const;

	////////////////////////////////////////////////////////////////////////////////
	// Fonctionnalites de base

	// Format de d'ecriture de type dense (defaut: false)
	// En format dense, meme les blocs sparses sont ecrits de facon dense
	void SetDenseOutputFormat(boolean bValue);
	boolean GetDenseOutputFormat() const;

	// Verification du format
	// A redefinir dans les sous-classes specifiant un format physique
	virtual boolean CheckFormat() const;

	// Verification de la validite des specifications
	// (classe et valeur de selection)
	boolean Check() const override;

	// Mode d'affichage des messages lors de l'analyse des fichiers (defaut: true)
	// Les erreurs sont affichees quoi qu'il arrivent.
	// Les warning et messages sont inhibees en mode non verbeux, en complement avec
	// le suivi des taches gere par la classe TaskManager
	void SetVerboseMode(boolean bValue);
	boolean GetVerboseMode() const;

	// Mode silencieux, pour inhiber tout affichage de message, verbeux ou non (defaut: false)
	virtual void SetSilentMode(boolean bValue);
	boolean GetSilentMode() const;

	// Redefinition des methodes de gestion des erreurs pour tenir compte du mode d'affichage
	void AddSimpleMessage(const ALString& sLabel) const override;
	void AddMessage(const ALString& sLabel) const override;
	void AddWarning(const ALString& sLabel) const override;
	void AddError(const ALString& sLabel) const override;

	// Memoire utilisee par le driver pour son fonctionnement
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	// Le libelle de l'objet contient le nom de la base et le numero de record s'il y en a un
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////////////////////////
	// Methodes a reimplementer dans les sous-classes
	// Les implementation par defaut ne font rien: la table peut s'ouvrir ou se
	// fermer, mais les lecture/ecriture sont sans effet.

	// Initialisation des caracteristiques de la classe a partir de la table
	// de donnees.
	// Seule la structure de la classe est initialisee a partir du schema de la table,
	// qui peut etre ouverte puis fermee pour l'occasion.
	// La classe passee en parametre n'a initialement aucun champ. Ceux-ci
	// doivent etre initialises par la methode.
	// Si cette fonctionnalite n'est pas disponible, la methode n'est pas a
	// reimplementer (par defaut: ne fait rien et renvoie false).
	// La classe ne doit pas faire partie d'un domaine en entree.
	// En sortie, elle est indexe si ok, et videe de ses champs avec son nom
	// a vide si ko.
	// Retourne true si la classe a ete construite sans erreurs
	virtual boolean BuildDataTableClass(KWClass* kwcDataTableClass);

	// Indique si la construction de base initialise le type des attributs
	// Retourne true si le type des attributs a ete iniatialise d'apres le schema.
	// Retourne false si seul le libelle des attributs a ete initialise (auquel cas,
	// les types rendus doivent etre Symbol, et la methode logique appelante lira
	// quelques instances pour initialiser les types)
	// Par defaut: false
	virtual boolean IsTypeInitializationManaged() const;

	// Ouverture physique de la table de donnees pour lecture
	// Les classe logique et physique sont initialisees prealablement
	// En cas de succes, la table doit etre preparee pour la lecture
	// d'un objet physique. Les eventuelles donnees internes permettant de
	// parametrer l'alimentation d'objets physiques doivent etre initialisees
	// dans cette methode
	// Le parametre (optionnel) kwcLogicalClass contient une definition plus complete
	// de la classe utilisee pour la lecture, permettant d'effectuer des controle de
	// coherence supplementaires
	virtual boolean OpenForRead(const KWClass* kwcLogicalClass);

	// Ouverture physique de la table de donnees pour ecriture
	// La classe logique est initialisee prealablement
	// En cas de succes, la table doit etre preparee pour l'ecriture
	// d'un objet physique. Les eventuelles donnees internes permettant de
	// parametrer l'ecriture des objets logiques doivent etre initialisees
	// dans cette methode
	virtual boolean OpenForWrite();

	// Test etat de la base
	virtual boolean IsOpenedForRead() const;
	virtual boolean IsOpenedForWrite() const;

	// Test de fin de table (cas Read)
	virtual boolean IsEnd() const;

	// Lecture d'une instance (en utilisant la classe physique)
	// Renvoie NULL si pas possibilite de produire un objet physique valide,
	// et emet un warning ou message d'erreur si necessaire
	// Renvoie egalement NULL si interruption utilisateur, mais sans message
	// (la variable nRecordNumber permet d'identifier l'enregistrement en erreur)
	// L'appel a IsEnd() ou IsError() permet d'identifier la fin du fichier ou une erreur grave
	virtual KWObject* Read();

	// Test si erreur grave (par defaut: false)
	// Positionne suite a une erreur grave de lecture ou ecriture, reinitialise uniquement a la fermeture du fichier
	virtual boolean IsError() const;

	// Lecture sans production d'un objet physique, pour sauter un enregistrement
	virtual void Skip();

	// Cle du dernier objet lu physiquement, uniquement dans le cas d'une classe principale d'un schema multi-table
	// Permet de verifier l'ordre et la duplication des instances dans le fichier
	// Mise a jour apres toute analyse d'une ligne, soit par Skip ou par Read (meme si la lecture echoue)
	virtual const KWObjectKey* GetLastReadMainKey() const;

	// Ecriture d'une instance (de la classe initiale)
	virtual void Write(const KWObject* kwoObject);

	// Fermeture de la table
	// Renvoie false si erreur grave avant ou pendant la fermeture, true sinon
	virtual boolean Close();

	// Test si la table est fermee
	virtual boolean IsClosed() const;

	// Destruction de la table
	// Utile en cas d'erreur d'ecriture, pour tout nettoyer correctement
	virtual void DeleteDataTable();

	// Nombre approximatif d'objet present dans la table de donnees
	// Peut ne pas etre reimplementee (par defaut: 0)
	virtual longint GetEstimatedObjectNumber();

	// Estimation de la taille minimum necessaire pour ouvrir le driver
	virtual longint ComputeOpenNecessaryMemory(boolean bRead);

	// Estimation de la taille memoire necessaire pour le chargement complet en tant que table externe (en octets)
	// La classe (logique) en parametre permet d'avoir acces a la definition logique des objets a charger
	// Attention, les tables externes sont chargees au niveau physique (sans mutation vers le niveau logique),
	// et l'estimation memoire se base sur ce niveau
	// Peut ne pas etre reimplementee (par defaut: 0)
	virtual longint ComputeNecessaryMemoryForFullExternalRead(const KWClass* kwcLogicalClass);

	// Estimation de la taille disque necessaire pour l'ecriture complet de la table (en octets)
	// La classe (logique) en parametre permet d'avoir acces a la definition logique des objets a ecrire
	// La taille du fichier en entree doit etre specifiee pour pouvoir estime le nombre de records si necessaire.
	// Attention, les ecriture se font au niveau logique et l'estimation memoire se base sur ce niveau
	// Peut ne pas etre reimplementee (par defaut: 0)
	virtual longint ComputeNecessaryDiskSpaceForFullWrite(const KWClass* kwcLogicalClass, longint lInputFileSize);

	// Estimation du pourcentage d'avancement de la lecture d'un fichier
	// Methode a priori rapide, sans effet important sur le temps de lecture
	// Peut ne pas etre reimplementee (par defaut: 0)
	virtual double GetReadPercentage() const;

	// Nombre d'erreurs d'encodage detectees impliquant des double quotes manquants,
	// pour une table ouverte en lecture (par defaut: 0)
	virtual longint GetEncodingErrorNumber() const;

	// Index de l'enregistrement traite (en lecture ou ecriture)
	longint GetRecordIndex() const;

	// Nombre d'enregistrements utilises
	// Compteur a maintenir par l'appelant (sauf pour sa reinitialisation au moment des ouvertures de base)
	void SetUsedRecordNumber(longint lValue);
	longint GetUsedRecordNumber() const;

	// Acces au nom de la classe (seulement si presente)
	const ALString& GetClassName() const;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs
	ALString sDataTableName;
	const KWClass* kwcClass;
	boolean bDenseOutputFormat;

	// Cle correspondant a la derniere ligne lue, dans le cas d'une classe principale d'un schema multi-table
	KWObjectKey lastReadMainKey;

	// Des entiers long sont utilises, pour la gestion de fichiers ayant
	// potentiellement plus de deux milliards d'enregistrements (limite des int)
	// Les Index servent a identifier une position unique dans un fichier, et les Number
	// un nombre d'enregistrements traites
	longint lRecordIndex;
	longint lUsedRecordNumber;

	// Mode verbeux et silencieux
	boolean bVerboseMode;
	boolean bSilentMode;
};

//////////////////////////////////////////////////////////////////////
// Methodes en inline

inline void KWDataTableDriver::SetDataTableName(const ALString& sValue)
{
	sDataTableName = sValue;
}

inline const ALString& KWDataTableDriver::GetDataTableName() const
{
	return sDataTableName;
}

inline void KWDataTableDriver::SetClass(const KWClass* kwcValue)
{
	kwcClass = kwcValue;
}

inline const KWClass* KWDataTableDriver::GetClass() const
{
	return kwcClass;
}

inline void KWDataTableDriver::SetDenseOutputFormat(boolean bValue)
{
	require(not IsOpenedForWrite());
	bDenseOutputFormat = bValue;
}

inline boolean KWDataTableDriver::GetDenseOutputFormat() const
{
	return bDenseOutputFormat;
}

inline boolean KWDataTableDriver::IsError() const
{
	return false;
}

inline void KWDataTableDriver::SetVerboseMode(boolean bValue)
{
	bVerboseMode = bValue;
}

inline boolean KWDataTableDriver::GetVerboseMode() const
{
	return bVerboseMode;
}

inline longint KWDataTableDriver::GetRecordIndex() const
{
	return lRecordIndex;
}

inline void KWDataTableDriver::SetUsedRecordNumber(longint lValue)
{
	lUsedRecordNumber = lValue;
}

inline longint KWDataTableDriver::GetUsedRecordNumber() const
{
	return lUsedRecordNumber;
}

inline longint KWDataTableDriver::GetEncodingErrorNumber() const
{
	return 0;
}

inline void KWDataTableDriver::SetSilentMode(boolean bValue)
{
	bSilentMode = bValue;
}

inline boolean KWDataTableDriver::GetSilentMode() const
{
	return bSilentMode;
}

inline const ALString& KWDataTableDriver::GetClassName() const
{
	require(kwcClass != NULL);
	return kwcClass->GetName();
}
