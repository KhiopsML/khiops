// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "ALString.h"
#include "FileService.h"
#include "CCHierarchicalDataGrid.h"
#include "JSONFile.h"
#include "KWVersion.h"
#include "KWFrequencyVector.h"
#include "JSONTokenizer.h"
#include "KWLearningErrorManager.h"

/////////////////////////////////////////////////////////////////////////
// Classe CCCoclusteringReport
// Gestion d'un rapport de coclustering
// Lecture/ecriture vers un fichier de rapport
// On se focalise uniquement sur l'entete du rapport ainsi que sur la structure de la grille de coclustering
// en ignorant les aspects hierarchie et informations complementaires
class CCCoclusteringReport : public Object
{
public:
	// Constructeur
	CCCoclusteringReport();
	~CCCoclusteringReport();

	//////////////////////////////////////////////////////////////////////////////////////
	// Lecture rapport des rapports de facon generique, independamment de leur format

	// Lecture des informations de coclustering a partir d'un fichier de rapport
	// Renvoie true si succes avec initialisation complete de la grille en parametres
	// Emission de messages d'erreur, et reinitialisation de la grille si echec
	boolean ReadGenericReport(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid);

	// Lecture des informations resume de coclustering a partir de l'entete d'un fichier de rapport
	// Le parsing est tres rapide, mais seules les informations synthetiques sont lues
	// (essentiellement: noms, types et nombres de parties par variables)
	// Les nombre d'instances et de cellules sont renvoye directement depuis la methode,
	// pas depuis la grille partiellement initialisee qui est vide.
	// Renvoie true si succes avec initialisation partielle de la grille en parametres
	// Emission de messages d'erreur, et reinitialisation de la grille si echec
	boolean ReadGenericReportHeader(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid,
					int& nInstanceNumber, int& nCellNumber);

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des rapports au format khc

	// Lecture des informations de coclustering a partir d'un fichier de rapport
	boolean ReadReport(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid);

	// Lecture des informations resume de coclustering a partir de l'entete d'un fichier de rapport
	boolean ReadReportHeader(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid,
				 int& nInstanceNumber, int& nCellNumber);

	// Ecriture d'un rapport de coclustering, pour un coclustering valide
	boolean WriteReport(const ALString& sFileName, const CCHierarchicalDataGrid* coclusteringDataGrid);

	//////////////////////////////////////////////////////////////////////////////////////
	// Gestion des rapports au format json

	// Suffix des fichiers de rapports au format json: khcj depuis les rapports au format Khiops V10
	static const ALString GetJSONReportSuffix();

	// Suffix des fichiers de rapports au format khc DEPRACATED
	static const ALString GetKhcReportSuffix();

	// Lecture des informations de coclustering a partir d'un fichier de rapport au format json
	boolean ReadJSONReport(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid);

	// Lecture des informations resume de coclustering a partir de l'entete d'un fichier de rapport au format json
	boolean ReadJSONReportHeader(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid,
				     int& nInstanceNumber, int& nCellNumber);

	// Ecriture d'un rapport de coclustering au format JSON
	boolean WriteJSONReport(const ALString& sJSONReportName, const CCHierarchicalDataGrid* coclusteringDataGrid);

	//////////////////////////////////////////////////////////////////////////////////////
	// Methodes standard

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Formats des fichiers
	enum FileFormats
	{
		KHC,
		JSON,
		None
	};

	// Methode de detection du format et de l'encodage d'un fichier, avec emission de message d'erreur si format
	// invalide et de warning si l'encodage pose probleme Le format est renvoye en code retour L'encodage correspond
	// au champ "khiops_encoding" du fichier json (cf. classe JSONFile), ou vide si non trouve ou non valide
	int DetectFileFormatAndEncoding(const ALString& sFileName, ALString& sKhiopsEncoding) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// Methode interne pour la gestion des rapports au format khc

	// Lecture des sections d'un rapport de coclustering
	// Toutes les operation de lecture se font avec les methodes internes ReadNextField, SkipLine et IsEndOfFile
	// Le parametre oaAttributesPartDictionaries contient pour chaque attribut un dictionnaire de partie index par
	// leur nom Methode ReadHierarchyPart:
	//   Le parametre dgPart est initialise pour les partie feuilles, a NULL sinon (retrouve depuis
	//   odPartDictionary)
	boolean InternalReadReport(CCHierarchicalDataGrid* coclusteringDataGrid, boolean bHeaderOnly);
	boolean ReadVersion(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadDimensions(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadCoclusteringStats(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadBounds(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadHierarchy(CCHierarchicalDataGrid* coclusteringDataGrid, ObjectArray* oaAttributesPartDictionaries);
	boolean ReadHierarchyPart(CCHierarchicalDataGrid* coclusteringDataGrid, CCHDGAttribute* dgAttribute,
				  CCHDGPart*& dgPart, ObjectDictionary* odPartDictionary);
	boolean ReadComposition(CCHierarchicalDataGrid* coclusteringDataGrid,
				ObjectArray* oaAttributesPartDictionaries);
	boolean ReadCells(CCHierarchicalDataGrid* coclusteringDataGrid, ObjectArray* oaAttributesPartDictionaries);
	boolean ReadAnnotation(CCHierarchicalDataGrid* coclusteringDataGrid, ObjectArray* oaAttributesPartDictionaries);

	// Ecriture des sections d'un rapport de coclustering vers un stream en sortie
	void InternalWriteReport(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost);
	void WriteVersion(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost);
	void WriteDimensions(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost);
	void WriteCoclusteringStats(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost);
	void WriteBounds(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost);
	void WriteHierarchy(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost);
	void WriteComposition(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost);
	void WriteCells(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost);
	void WriteAnnotation(const CCHierarchicalDataGrid* coclusteringDataGrid, ostream& ost);

	// Ouverture du fichier de rapport de coclustering pour lecture
	boolean OpenInputCoclusteringReportFile(const ALString& sFileName);

	// Fermeture du fichier
	void CloseCoclusteringReportFile();

	// Lecture du prochain champ d'une ligne
	// La chaine en parametre contient en retour le contenu du champ,
	// termine par le caractere '\0'
	char* ReadNextField();

	// Saut d'une ligne
	void SkipLine();

	// Test de fin de ligne
	boolean IsEndOfLine();

	// Test de fin de fichier
	boolean IsEndOfFile();

	/////////////////////////////////////////////////////////////////////////////////////
	// Methode interne pour la gestion des rapports au format json

	// Lecture des sections d'un rapport de coclustering
	boolean InternalReadJSONReport(CCHierarchicalDataGrid* coclusteringDataGrid, boolean bHeaderOnly);
	boolean ReadJSONSummary(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadJSONDimensionSummaries(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadJSONDimensionPartitions(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadJSONInnerAttributesDimensionSummaries(KWDGAttribute* dgAttribute);
	boolean ReadJSONAttributePartition(KWDGAttribute* attribute, CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadJSONInterval(KWDGAttribute* dgAttribute, KWDGPart* dgPart);
	boolean ReadJSONValueGroup(KWDGAttribute* dgAttribute, KWDGPart* dgPart);
	boolean ReadJSONInnerAttributeIntervals(KWDGAttribute* innerAttribute);
	boolean ReadJSONInnerAttributeValueGroups(KWDGAttribute* innerAttribute);
	boolean ReadJSONVarPartAttributeValueGroup(KWDGAttribute* varPartAttribute, KWDGPart* dgPart,
						   const ObjectDictionary* odInnerAttributesAllVarParts,
						   ObjectDictionary* odVarPartAttributeAllVarParts);
	boolean ReadJSONDimensionHierarchies(CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadJSONCells(CCHierarchicalDataGrid* coclusteringDataGrid);

	// Ecriture des sections d'un rapport de coclustering vers un stream en sortie
	void InternalWriteJSONReport(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteJSONSummary(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteJSONDimensionSummaries(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteJSONInnerAttributesDimensionSummaries(const KWDGAttribute* vartPartAttribute, JSONFile* fJSON);
	void WriteJSONDimensionSummary(CCHDGAttribute* attribute, JSONFile* fJSON);
	void WriteJSONDimensionPartitions(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteJSONAttributePartition(KWDGAttribute* attribute, const CCHierarchicalDataGrid* coclusteringDataGrid,
					 JSONFile* fJSON);
	void WriteJSONDimensionHierarchies(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);
	void WriteJSONCells(const CCHierarchicalDataGrid* coclusteringDataGrid, JSONFile* fJSON);

	// Fichier des gestion du rapport
	ALString sReportFileName;
	FILE* fReport;
	int nLineIndex;
	boolean bEndOfLine;
	static const int nMaxFieldSize = 10000;
	char* sFileBuffer;
	boolean bReadDebug;
	int nHeaderInstanceNumber;
	int nHeaderCellNumber;
	ALString sLocalFileName;

	// Libelle des sections d'un rapports de coclustering
	static const ALString sKeyWordKhiops;
	static const ALString sKeyWordShortDescription;
	static const ALString sKeyWordDimensions;
	static const ALString sKeyWordCoclusteringStats;
	static const ALString sKeyWordInstances;
	static const ALString sKeyWordCells;
	static const ALString sKeyWordNullCost;
	static const ALString sKeyWordCost;
	static const ALString sKeyWordLevel;
	static const ALString sKeyWordInitialDimensions;
	static const ALString sKeyWordFrequencyAttribute;
	static const ALString sKeyWordDictionary;
	static const ALString sKeyWordDatabase;
	static const ALString sKeyWordSamplePercentage;
	static const ALString sKeyWordSamplingMode;
	static const ALString sKeyWordSelectionVariable;
	static const ALString sKeyWordSelectionValue;
	static const ALString sKeyWordBounds;
	static const ALString sKeyWordHierarchy;
	static const ALString sKeyWordComposition;
	static const ALString sKeyWordAnnotation;
	static const ALString sKeyWordTrue;
	static const ALString sKeyWordFalse;
};
