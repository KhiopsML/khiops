// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MHHistogramSpec.h"
#include "MHHistogramSpecView.h"
#include "MHTruncationDiscretizerHistogramMODL_fp.h"
#include "MHBin.h"
#include "Version.h"

////////////////////////////////////////////////////////////////
// Classe MHCommandLine
// Lancement du calcul d'un histogramme de puis la ligne de commande
class MHCommandLine : public Object
{
public:
	// Constructeur
	MHCommandLine();
	~MHCommandLine();

	// Lancement depuis la ligne de commande
	boolean ComputeHistogram(int argc, char** argv);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Lecture des couples (value, frequency) sous la forme de vecteurs
	// Les vecteurs crees selon le format sont:
	//   . format 1: UpperValues
	//   . format 2: UpperValues, Frequencies
	//   . format 3: LowerValues, UpperValues, Frequencies
	// Les autres vecteur sont mis a NULL
	// Renvoie true si ok
	boolean ReadBins(ContinuousVector*& cvLowerValues, ContinuousVector*& cvUpperValues, IntVector*& ivFrequencies);

	// Lecture d'un champ de type valeur
	boolean ReadValue(InputBufferedFile* inputFile, longint lRecordIndex, Continuous& cValue, boolean& bEndOfLine);

	// Lecture d'un champ de type effectif
	boolean ReadFrequency(InputBufferedFile* inputFile, longint lRecordIndex, longint& lCumulatedFrequency,
			      int& nFrequency, boolean& bEndOfLine);

	// Emission d'un warning ou d'un erreur de lecture de fichier pour un numero de ligne donne
	void AddInputFileWarning(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel);
	void AddInputFileError(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel);

	// Initialisation des parametres
	// Initialize l'objet de parametrage histogramSpec
	// Renvoie le nom du fichier a traiter si le paramatrage permet de lancer les traitements
	// et vide sinon, avec des messages d'erreur
	const ALString InitializeParameters(int argc, char** argv);

	// Affichage de l'aide
	void ShowHelp();

	// Initialisation du parametrage
	void InitializeParameterSettings();

	// Parametrage pour une option, portee par un champ de histogramSpecView
	// On renvoie false, sans message d'erreur, en cas de valeur invalide
	boolean SetOptionValue(UIElement* optionField, const ALString& sValue);

	// Nom du fichier de donnees
	ALString sDataFileName;

	// Parametrage des histogrammes pour gerer la ligne de commande
	// On utilise ici une classe de type UIView, qui permet de parametrer de facon generique
	// une liste de champs avec un ShortCut pour l' characterer de l'option de ligne de commande,
	// un type pour gerer les valeurs, un HelpText pour l'aide
	MHHistogramSpecView histogramSpecView;

	// Objet de parametrage des histogramme
	MHHistogramSpec histogramSpec;
};
