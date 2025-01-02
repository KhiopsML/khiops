// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributePairsSpec.h"

KWAttributePairsSpec::KWAttributePairsSpec()
{
	nMaxAttributePairNumber = 0;
	bAllAttributePairs = true;
	sContextLabel = "";
	lContextLineIndex = 0;
	nContextAttributeIndex = 0;
}

KWAttributePairsSpec::~KWAttributePairsSpec()
{
	oaSpecificAttributePairs.DeleteAll();
}

int KWAttributePairsSpec::GetMaxAttributePairNumber() const
{
	return nMaxAttributePairNumber;
}

void KWAttributePairsSpec::SetMaxAttributePairNumber(int nValue)
{
	nMaxAttributePairNumber = nValue;
}

boolean KWAttributePairsSpec::GetAllAttributePairs() const
{
	return bAllAttributePairs;
}

void KWAttributePairsSpec::SetAllAttributePairs(boolean bValue)
{
	bAllAttributePairs = bValue;
}

#ifdef DEPRECATED_V10
const ALString& KWAttributePairsSpec::GetMandatoryAttributeInPairs() const
{
	return sMandatoryAttributeInPairs;
}

void KWAttributePairsSpec::SetMandatoryAttributeInPairs(const ALString& sValue)
{
	sMandatoryAttributeInPairs = sValue;
}
#endif // DEPRECATED_V10

ObjectArray* KWAttributePairsSpec::GetSpecificAttributePairs()
{
	return &oaSpecificAttributePairs;
}

void KWAttributePairsSpec::ImportAttributePairs(const ALString& sFileName)
{
	SortedList slSpecificPairs(KWAttributePairNameCompare);
	ObjectArray oaImportedPairs;
	InputBufferedFile inputFile;
	boolean bOk;
	boolean bRecordOk;
	ALString sMessage;
	KWAttributePairName workingAttributePairName;
	KWAttributePairName* attributePairName;
	int i;
	char* sField;
	int nFieldLength;
	int nFieldError;
	boolean bEndOfLine;
	boolean bLineTooLong;
	longint lBeginPos;
	ALString sTmp;

	require(sFileName != "");
	require(oaSpecificAttributePairs.GetSize() <= KWAttributePairsSpec::nLargestMaxAttributePairNumber);

	// Specialisation du contexte d'emission des erreurs
	sContextLabel = "Import";

	// On commence a enregistrer les paires existantes dans une liste triee pour
	// permettre la detection des doublons pendant l'import
	for (i = 0; i < oaSpecificAttributePairs.GetSize(); i++)
	{
		attributePairName = cast(KWAttributePairName*, oaSpecificAttributePairs.GetAt(i));
		slSpecificPairs.Add(attributePairName);
	}
	assert(slSpecificPairs.GetCount() == oaSpecificAttributePairs.GetSize());

	// Ouverture du fichier
	Global::ActivateErrorFlowControl();
	inputFile.SetFileName(sFileName);
	inputFile.SetHeaderLineUsed(false);
	bOk = inputFile.Open();
	if (bOk)
	{
		lContextLineIndex = 0;
		while (bOk and not inputFile.IsError() and not inputFile.IsFileEnd())
		{
			// Remplissage d'un buffer
			lBeginPos = inputFile.GetPositionInFile();
			bOk = inputFile.FillInnerLines(lBeginPos);
			if (not bOk)
				AddError("Error while reading file");

			// Erreur si ligne trop longue: on n'a besoin que de lignes tres courtes pour lire des paires
			// d'identifiant
			if (bOk and inputFile.GetCurrentBufferSize() == 0)
			{
				assert(inputFile.GetBufferSize() > 5 * KWClass::GetNameMaxLength());
				AddError(sTmp + "Line too long for a file of variable pairs (beyond " +
					 LongintToHumanReadableString(inputFile.GetBufferSize()) + ")");
				bOk = false;
			}

			// Traitement du buffer
			while (bOk and not inputFile.IsError() and not inputFile.IsBufferEnd())
			{
				lContextLineIndex++;

				// Lecture du premier champ de la ligne
				bRecordOk = false;
				nContextAttributeIndex = 1;
				bEndOfLine = inputFile.GetNextField(sField, nFieldLength, nFieldError, bLineTooLong);

				// Warning si ligne trop longue
				if (bLineTooLong)
					AddWarning(InputBufferedFile::GetLineTooLongErrorLabel() + ", ignored record");
				// Warning pour certaines erreurs sur le champ
				else if (nFieldError == InputBufferedFile::FieldTabReplaced or
					 nFieldError == InputBufferedFile::FieldCtrlZReplaced or
					 nFieldError == InputBufferedFile::FieldTooLong)
					AddWarning(InputBufferedFile::GetFieldErrorLabel(nFieldError) +
						   ", ignored record");
				// Warning si un seul champ
				else if (bEndOfLine)
				{
					// Cas particulier de la premiere ligne: erreur si le format est errone
					if (lContextLineIndex == 1)
					{
						nContextAttributeIndex = 0;
						AddError("Fist line with one single field: the format of the file must "
							 "be tabular with two fields per records");
						bOk = false;
						break;
					}
					else
						AddWarning("One single field in line, ignored record");
				}
				// Warning si champs trop long: attention, on le gere specifiquement sans passer par la
				// methode check de KWClass, pour eviter d'allouer implicitement une ALString
				// potentiellement de tres grande taille
				else if (nFieldLength > KWClass::GetNameMaxLength())
					AddWarning("Name too long, ignored record");
				// Warning si nom d'attribut present et invalide
				else if (sField[0] != '\0' and not KWClass::CheckNameWithMessage(sField, sMessage))
					AddWarning(sMessage + ", ignored record");
				// OK: on memorise le premier attribut de la paire
				else
				{
					bRecordOk = true;
					workingAttributePairName.SetFirstName(sField);
				}

				// Lecture du second champ de la ligne
				if (bRecordOk)
				{
					bRecordOk = false;
					nContextAttributeIndex = 2;
					bEndOfLine =
					    inputFile.GetNextField(sField, nFieldLength, nFieldError, bLineTooLong);

					// Warning si ligne trop longue
					if (bLineTooLong)
						AddWarning(InputBufferedFile::GetLineTooLongErrorLabel() +
							   ", ignored record");
					// Warning pour certaines erreurs sur le champ
					else if (nFieldError == InputBufferedFile::FieldTabReplaced or
						 nFieldError == InputBufferedFile::FieldCtrlZReplaced or
						 nFieldError == InputBufferedFile::FieldTooLong)
						AddWarning(InputBufferedFile::GetFieldErrorLabel(nFieldError));
					// Warning si strictement plus de deux champs
					else if (not bEndOfLine)
					{
						// Cas particulier de la premiere ligne: erreur si le format est errone
						if (lContextLineIndex == 1)
						{
							nContextAttributeIndex = 0;
							AddError(
							    "Fist line with more than two fields: the format of the "
							    "file must be tabular with two fields per records");
							bOk = false;
							break;
						}
						else
							AddWarning("Two many fields in line, ignored record");
					}
					// Warning si champs trop long
					else if (nFieldLength > KWClass::GetNameMaxLength())
						AddWarning("Name too long, ignored record");
					// Warning si nom d'attribut present et invalide
					else if (sField[0] != '\0' and
						 not KWClass::CheckNameWithMessage(sField, sMessage))
						AddWarning(sMessage + ", ignored record");
					// OK: on memorise le second attribut de la paire
					else
					{
						bRecordOk = true;
						workingAttributePairName.SetSecondName(sField);
					}
				}

				// Tests sur la paire elle meme
				if (bRecordOk)
				{
					bRecordOk = false;
					nContextAttributeIndex = 0;

					// Warning si les deux attributs de la paire sont vides
					if (workingAttributePairName.GetFirstName() == "" and
					    workingAttributePairName.GetSecondName() == "")
						AddWarning("Empty pair " + workingAttributePairName.GetObjectLabel() +
							   ", ignored record");
					// Warning si les deux attributs de la paire sont identiques
					else if (workingAttributePairName.GetFirstName() ==
						 workingAttributePairName.GetSecondName())
						AddWarning("Identical variables in pair " +
							   workingAttributePairName.GetObjectLabel() +
							   ", ignored record");
					// Warning si la paire existe deja
					else if (slSpecificPairs.Find(&workingAttributePairName) != NULL)
						AddWarning("Variable pair " +
							   workingAttributePairName.GetObjectLabel() +
							   " already exists, ignored record");
					// Ok: paire valide
					else
						bRecordOk = true;
				}

				// Test de depassement du nombre limites de paires
				if (bRecordOk)
				{
					// Warning et arret de l'analyse si le nombre limite de paires specifiables est
					// atteint
					if (slSpecificPairs.GetCount() >=
					    KWAttributePairsSpec::nLargestMaxAttributePairNumber)
					{
						AddSimpleMessage(
						    sTmp + "Maximum number of variable pairs in list (" +
						    IntToString(KWAttributePairsSpec::nLargestMaxAttributePairNumber) +
						    ") is reached: stop importing pairs from file");
						break;
					}
				}

				// Ajout de la paire dans la liste si OK
				if (bRecordOk)
				{
					assert(bEndOfLine);
					attributePairName = workingAttributePairName.Clone();
					oaImportedPairs.Add(attributePairName);

					// Memorisation dans la liste triee permettant de detecter les doublons
					slSpecificPairs.Add(attributePairName);
				}
				// On va jusqu'au bout de la ligne si necessaire sinon
				// On a deja ignore la ligne si on arrive la
				else if (not bEndOfLine)
					inputFile.SkipLastFields(bLineTooLong);
			}

			// Arret si le maximum de paires est atteint
			// Un message a deja ete emis dans la boucle d'analyse par buffer
			if (slSpecificPairs.GetCount() >= KWAttributePairsSpec::nLargestMaxAttributePairNumber)
				break;
		}
		inputFile.Close();
	}
	Global::DesactivateErrorFlowControl();

	// Message de fin
	if (bOk)
	{
		oaSpecificAttributePairs.InsertObjectArrayAt(oaSpecificAttributePairs.GetSize(), &oaImportedPairs);
		AddSimpleMessage(sTmp + IntToString(oaImportedPairs.GetSize()) + " variable pairs imported from file " +
				 sFileName);
		AddSimpleMessage("");
	}
	else
	{
		lContextLineIndex = 0;
		nContextAttributeIndex = 0;
		oaImportedPairs.DeleteAll();
		AddError(sTmp + "Import of variable pairs cancelled because of errors");
		AddSimpleMessage("");
	}

	// Reinitialisation du contexte d'emission des erreurs
	sContextLabel = "";
	lContextLineIndex = 0;
	nContextAttributeIndex = 0;
	ensure(oaSpecificAttributePairs.GetSize() <= KWAttributePairsSpec::nLargestMaxAttributePairNumber);
}

void KWAttributePairsSpec::ExportAllAttributePairs(const ALString& sFileName)
{
	SortedList slSpecificPairs(KWAttributePairNameCompare);
	OutputBufferedFile outputFile;
	boolean bOk;
	boolean bPairOk;
	ALString sMessage;
	KWAttributePairName* attributePairName;
	int i;
	ALString sTmp;

	require(sFileName != "");

	// Specialisation du contexte d'emission des erreurs
	sContextLabel = "Export";

	// Ouverture du fichier
	Global::ActivateErrorFlowControl();
	outputFile.SetFileName(sFileName);
	bOk = outputFile.Open();
	if (bOk)
	{
		// Ecriture des paires
		lContextLineIndex = 0;
		for (i = 0; i < oaSpecificAttributePairs.GetSize(); i++)
		{
			attributePairName = cast(KWAttributePairName*, oaSpecificAttributePairs.GetAt(i));
			lContextLineIndex++;

			// Verification de la validite de la paire
			bPairOk = true;
			// Warning si le nom du premier attribut est present et invalide
			nContextAttributeIndex = 1;
			if (bPairOk and attributePairName->GetFirstName() != "" and
			    not KWClass::CheckNameWithMessage(attributePairName->GetFirstName(), sMessage))
			{
				AddWarning(sMessage + ", ignored pair");
				bPairOk = false;
			}
			// Warning si le nom du second attribut est present et invalide
			nContextAttributeIndex = 2;
			if (bPairOk and attributePairName->GetSecondName() != "" and
			    not KWClass::CheckNameWithMessage(attributePairName->GetSecondName(), sMessage))
			{
				AddWarning(sMessage + ", ignored pair");
				bPairOk = false;
			}
			// Warning si les deux attributs de la paire sont absents ou identiques
			nContextAttributeIndex = 0;
			if (bPairOk and attributePairName->GetFirstName() == "" and
			    attributePairName->GetSecondName() == "")
			{
				AddWarning("Empty pair " + attributePairName->GetObjectLabel() + ", ignored pair");
				bPairOk = false;
			}
			if (bPairOk and attributePairName->GetFirstName() == attributePairName->GetSecondName())
			{
				AddWarning("Identical variables in pair " + attributePairName->GetObjectLabel() +
					   ", ignored pair");
				bPairOk = false;
			}
			// Warning si la paire existe deja
			if (bPairOk and slSpecificPairs.Find(attributePairName) != NULL)
			{
				AddWarning("Variable pair " + attributePairName->GetObjectLabel() +
					   " already exists, ignored pair");
				bPairOk = false;
			}

			// Ecriture si ok
			if (bPairOk)
			{
				outputFile.WriteField(attributePairName->GetFirstName());
				outputFile.Write('\t');
				outputFile.WriteField(attributePairName->GetSecondName());
				outputFile.WriteEOL();

				// Memorisation dans la liste des paires
				slSpecificPairs.Add(attributePairName);
			}
		}

		// Fermeture du fichier
		bOk = outputFile.Close();
	}
	Global::DesactivateErrorFlowControl();

	// Message de fin
	if (bOk)
	{
		AddSimpleMessage(sTmp + IntToString(slSpecificPairs.GetCount()) + " variable pairs exported to file " +
				 sFileName);
		AddSimpleMessage("");
	}

	// Reinitialisation du contexte d'emission des erreurs
	sContextLabel = "";
	lContextLineIndex = 0;
	nContextAttributeIndex = 0;
}

void KWAttributePairsSpec::DeleteDuplicateAttributePairs()
{
	SortedList slSpecificPairs(KWAttributePairNameCompare);
	ObjectArray oaValidPairs;
	boolean bPairOk;
	KWAttributePairName* attributePairName;
	int i;
	ALString sTmp;

	// Supression des paires invalides ou en doublons
	for (i = 0; i < oaSpecificAttributePairs.GetSize(); i++)
	{
		attributePairName = cast(KWAttributePairName*, oaSpecificAttributePairs.GetAt(i));

		// Verification de la validite de la paire, sans warning
		bPairOk = true;
		bPairOk = bPairOk and (attributePairName->GetFirstName() == "" or
				       KWClass::CheckName(attributePairName->GetFirstName(), NULL));
		bPairOk = bPairOk and (attributePairName->GetSecondName() == "" or
				       KWClass::CheckName(attributePairName->GetSecondName(), NULL));
		bPairOk = bPairOk and attributePairName->GetFirstName() != attributePairName->GetSecondName();
		bPairOk = bPairOk and slSpecificPairs.Find(attributePairName) == NULL;

		// Memorisation si Ok
		if (bPairOk)
		{
			oaValidPairs.Add(attributePairName);

			// Memorisation dans la liste des paires
			slSpecificPairs.Add(attributePairName);
		}
		// Destruction sinon
		else
			delete attributePairName;
	}

	// Message de fin
	if (oaValidPairs.GetSize() < oaSpecificAttributePairs.GetSize())
	{
		AddMessage(sTmp + IntToString(oaSpecificAttributePairs.GetSize() - oaValidPairs.GetSize()) +
			   " invalid or duplicate pairs removed from the list of specific variable pairs");
		AddSimpleMessage("");
	}

	// Memorisation des paires valides
	oaSpecificAttributePairs.CopyFrom(&oaValidPairs);
}

void KWAttributePairsSpec::CheckAttributePairNumbers() const
{
	ALString sTmp;

	if (GetMaxAttributePairNumber() < oaSpecificAttributePairs.GetSize())
	{
		AddWarning(sTmp + "Max number of variable pairs (" + IntToString(GetMaxAttributePairNumber()) +
			   ") is less than the number of specific variable pairs (" +
			   IntToString(oaSpecificAttributePairs.GetSize()) + ")");
	}
}

void KWAttributePairsSpec::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

const ALString& KWAttributePairsSpec::GetClassName() const
{
	return sClassName;
}

int KWAttributePairsSpec::GetMaxRequestedAttributePairNumber(const ALString& sTargetAttributeName) const
{
	int nRequestedAttributePairNumber;
	ObjectDictionary odSingleAttributePairs;
	longint lUsedAttributeNumber;
	longint lLargePairNumber;
	KWClass* kwcAnalyzedClass;
	KWAttribute* attribute;
	KWAttributePairName* attributePairName;
	boolean bPairOk;
	int i;

	require(GetClassName() != "");
	require(KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()) != NULL);

	// On retourne 0 directement si aucune paire n'est demandee
	if (GetMaxAttributePairNumber() == 0)
		return 0;

	// Acces a la classe de gestion des attributs
	kwcAnalyzedClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

	// Calcul du nombre d'attributs a analyser, hors attribut cible
	lUsedAttributeNumber = kwcAnalyzedClass->GetUsedAttributeNumberForType(KWType::Continuous) +
			       kwcAnalyzedClass->GetUsedAttributeNumberForType(KWType::Symbol);
	if (kwcAnalyzedClass->LookupAttribute(sTargetAttributeName) != NULL)
		lUsedAttributeNumber--;

#ifdef DEPRECATED_V10
	// Cas avec attribut particulier dans les paires: cas obsolete
	if (GetMandatoryAttributeInPairs() != "")
	{
		nRequestedAttributePairNumber = min(GetMaxAttributePairNumber(), max((int)lUsedAttributeNumber - 1, 0));
		return nRequestedAttributePairNumber;
	}
#endif // DEPRECATED_V10

	// Cas ou toutes les paires sont a analyser: seul le nombre max peut limiter le nombre de paires
	if (GetAllAttributePairs())
	{
		lLargePairNumber = lUsedAttributeNumber * (lUsedAttributeNumber - 1) / 2;
		if (lLargePairNumber > GetMaxAttributePairNumber())
			nRequestedAttributePairNumber = GetMaxAttributePairNumber();
		else
			nRequestedAttributePairNumber = int(lLargePairNumber);
	}
	// Cas ou il n'y a que des paires specifique
	else
	{
		// On compte d'abord les paires mono-attributs qui correspondent a des sous-ensembles de paires
		for (i = 0; i < oaSpecificAttributePairs.GetSize(); i++)
		{
			attributePairName = cast(KWAttributePairName*, oaSpecificAttributePairs.GetAt(i));

			// Cas d'une paire mono-attribut
			if (attributePairName->GetFirstName() == "" or attributePairName->GetSecondName() == "")
			{
				assert(attributePairName->GetFirstSortedName() == "");

				// Memorisation si attribut utilisable dans une paire
				attribute = kwcAnalyzedClass->LookupAttribute(attributePairName->GetSecondSortedName());
				if (attribute != NULL and attribute->GetUsed() and
				    KWType::IsSimple(attribute->GetType()) and
				    attribute->GetName() != sTargetAttributeName)
					odSingleAttributePairs.SetAt(attribute->GetName(), attribute);
			}
		}

		// Comptage des paires d'attributs impliquees par les paires mono-attributs
		// en tenant compte des doublons
		lLargePairNumber =
		    odSingleAttributePairs.GetCount() * lUsedAttributeNumber -
		    (odSingleAttributePairs.GetCount() * (longint)(odSingleAttributePairs.GetCount() + 1)) / 2;

		// Comptage des paires d'attributs specifique valides additionnelles
		for (i = 0; i < oaSpecificAttributePairs.GetSize(); i++)
		{
			attributePairName = cast(KWAttributePairName*, oaSpecificAttributePairs.GetAt(i));

			// Cas d'une paire bi-attribut
			if (attributePairName->GetFirstName() != "" and attributePairName->GetSecondName() != "")
			{
				bPairOk = true;

				// Test si premier attribut utilisable
				attribute = kwcAnalyzedClass->LookupAttribute(attributePairName->GetFirstName());
				bPairOk = bPairOk and attribute != NULL;
				bPairOk = bPairOk and attribute->GetUsed();
				bPairOk = bPairOk and KWType::IsSimple(attribute->GetType());
				bPairOk = bPairOk and attribute->GetName() != sTargetAttributeName;
				bPairOk = bPairOk and odSingleAttributePairs.Lookup(attribute->GetName()) == NULL;

				// Test si second attribut utilisable
				attribute = kwcAnalyzedClass->LookupAttribute(attributePairName->GetSecondName());
				bPairOk = bPairOk and attribute != NULL;
				bPairOk = bPairOk and attribute->GetUsed();
				bPairOk = bPairOk and KWType::IsSimple(attribute->GetType());
				bPairOk = bPairOk and attribute->GetName() != sTargetAttributeName;
				bPairOk = bPairOk and odSingleAttributePairs.Lookup(attribute->GetName()) == NULL;

				// Incrementation si paire utilisable
				if (bPairOk)
					lLargePairNumber++;
			}
		}

		// On se limite au max si necessaire
		if (lLargePairNumber > GetMaxAttributePairNumber())
			nRequestedAttributePairNumber = GetMaxAttributePairNumber();
		else
			nRequestedAttributePairNumber = (int)lLargePairNumber;
	}
	return nRequestedAttributePairNumber;
}

void KWAttributePairsSpec::SelectAttributePairStats(const ObjectArray* oaAttributeStats,
						    ObjectArray* oaAttributePairStats) const
{
	SortedList slAllSelectedPairs(KWAttributePairNameCompare);
	KWLearningSpec* learningSpec;
	KWClass* kwcAnalyzedClass;
	KWAttributePairStats* attributePairStats;
	ObjectDictionary odAttributeStats;
	ObjectArray oaSelectedAttributeStats;
	KWAttributeStats* attributeStats1;
	KWAttributeStats* attributeStats2;
	ObjectArray oaValidSpecificAttributePairs;
	int nUselessSpecificPairNumber;
	KWAttributePairName* attributePairName;
	KWAttributePairName workingAttributePairName;
	boolean bPairOk;
	int i;
	int i1;
	int i2;
	ALString sTmp;

	require(GetClassName() != "");
	require(KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()) != NULL);
	require(oaAttributeStats != NULL);
	require(oaAttributePairStats != NULL);
	require(oaAttributePairStats->GetSize() == 0);

	// On arrete directement si aucune paire n'est demandee our possible
	if (GetMaxAttributePairNumber() == 0 or oaAttributeStats->GetSize() == 0)
		return;

	// Acces aux learningSpec
	learningSpec = cast(KWAttributeStats*, oaAttributeStats->GetAt(0))->GetLearningSpec();
	assert(learningSpec->GetClass()->GetName() == GetClassName());

	// Acces a la classe de gestion des attributs
	kwcAnalyzedClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());

	// Recherche et priorisation des attributs a analyser
	SelectAttributeStatsForAttributePairs(oaAttributeStats, &odAttributeStats, &oaSelectedAttributeStats);

#ifdef DEPRECATED_V10
	// Cas avec attribut particulier dans les paires: cas obsolete
	if (GetMandatoryAttributeInPairs() != "")
	{
		// Warning si attribut inexistant
		if (kwcAnalyzedClass->LookupAttribute(GetMandatoryAttributeInPairs()) == NULL)
			AddWarning("Mandatory variable in pairs " + GetMandatoryAttributeInPairs() +
				   " not found in dictionary " + GetClassName());
		else if (odAttributeStats.Lookup(GetMandatoryAttributeInPairs()) == NULL)
			AddWarning("Mandatory variable in pairs " + GetMandatoryAttributeInPairs() +
				   " not analyzed in data preparation");
		// Sinon, on va rechercher les paires correspondantes
		else
		{
			// Creation des paires a analyser
			workingAttributePairName.SetFirstName(GetMandatoryAttributeInPairs());
			for (i = 0; i < oaSelectedAttributeStats.GetSize(); i++)
			{
				attributeStats2 = cast(KWAttributeStats*, oaSelectedAttributeStats.GetAt(i));

				// On n'insere pas les paires impliquant un autre attribut
				if (attributeStats2->GetSortName() != GetMandatoryAttributeInPairs())
				{
					// Creation et initialisation d'un objet de stats pour la paire d'attributs
					attributePairStats = new KWAttributePairStats;
					attributePairStats->SetLearningSpec(learningSpec);
					oaAttributePairStats->Add(attributePairStats);

					// Parametrage des attributs de la paire par ordre alphabetique
					workingAttributePairName.SetSecondName(attributeStats2->GetSortName());
					attributePairStats->SetAttributeName1(
					    workingAttributePairName.GetFirstSortedName());
					attributePairStats->SetAttributeName2(
					    workingAttributePairName.GetSecondSortedName());

					// Arret si assez de paires
					if (oaAttributePairStats->GetSize() >= GetMaxAttributePairNumber())
						break;
				}
			}
		}

		// On arrete avec un warning, sans chercher a hybrider ce mode obsolete avec les nouvelles
		// specifications
		AddWarning("Parameter 'Mandatory variable in pairs' is deprecated since Khiops V10");
		return;
	}
#endif // DEPRECATED_V10

	// Recherche des paires d'attributs specifique valides
	Global::ActivateErrorFlowControl();
	nUselessSpecificPairNumber = 0;
	for (i = 0; i < oaSpecificAttributePairs.GetSize(); i++)
	{
		attributePairName = cast(KWAttributePairName*, oaSpecificAttributePairs.GetAt(i));
		bPairOk = true;

		// Test de validite et d'interet pour le premier attribut de la paire
		if (bPairOk and attributePairName->GetFirstName() != "")
		{
			// Presence dans le dictionnaire
			if (kwcAnalyzedClass->LookupAttribute(attributePairName->GetFirstName()) == NULL)
			{
				AddWarning("First variable missing in dictionary " + GetClassName() + " for pair " +
					   attributePairName->GetObjectLabel() + ", ignored pair");
				bPairOk = false;
			}
			else
			{
				attributeStats1 =
				    cast(KWAttributeStats*, odAttributeStats.Lookup(attributePairName->GetFirstName()));

				// Presence dans les stats univaries, ce qui signifie que l'attribut est utilise et du
				// bon type
				if (attributeStats1 == NULL)
				{
					AddWarning("First variable not analyzed in data preparation for pair " +
						   attributePairName->GetObjectLabel() + ", ignored pair");
					bPairOk = false;
				}
				// Test d'utiliste de la paire sur la base du nombre de valeurs distinctes
				else if (attributeStats1->GetDescriptiveStats()->GetValueNumber() <= 1)
				{
					bPairOk = false;
					nUselessSpecificPairNumber++;
				}
			}
		}

		// Test de validite et d'interet pour le second attribut de la paire
		if (bPairOk and attributePairName->GetSecondName() != "")
		{
			// Presence dans le dictionnaire
			if (kwcAnalyzedClass->LookupAttribute(attributePairName->GetSecondName()) == NULL)
			{
				AddWarning("Second variable missing in dictionary " + GetClassName() + " for pair " +
					   attributePairName->GetObjectLabel() + ", ignored pair");
				bPairOk = false;
			}
			else
			{
				attributeStats2 = cast(KWAttributeStats*,
						       odAttributeStats.Lookup(attributePairName->GetSecondName()));

				// Presence dans les stats univaries, ce qui signifie que l'attribut est utilise et du
				// bon type
				if (attributeStats2 == NULL)
				{
					AddWarning("Second variable not analyzed in data preparation for pair " +
						   attributePairName->GetObjectLabel() + ", ignored pair");
					bPairOk = false;
				}
				// Test d'utiliste de la paire sur la base du nombre de valeurs distinctes
				else if (attributeStats2->GetDescriptiveStats()->GetValueNumber() <= 1)
				{
					bPairOk = false;
					nUselessSpecificPairNumber++;
				}
			}
		}

		// Ajout si paire valide
		if (bPairOk)
			oaValidSpecificAttributePairs.Add(attributePairName);
	}
	Global::DesactivateErrorFlowControl();

	// Message specifique si des paires impliquant des attribust sans interets ont ete detectees
	if (nUselessSpecificPairNumber > 0)
		AddMessage(sTmp + IntToString(nUselessSpecificPairNumber) +
			   " specific variable pairs involving singled-valued variables, not considered for analysis");

	// On prend en priorite les paires specifiques, selon leur ordre de specification
	for (i = 0; i < oaValidSpecificAttributePairs.GetSize(); i++)
	{
		attributePairName = cast(KWAttributePairName*, oaValidSpecificAttributePairs.GetAt(i));

		// Cas d'une paire mono-attribut
		if (attributePairName->GetFirstSortedName() == "")
		{
			assert(attributePairName->GetSecondSortedName() != "");

			// Acces aux stats de cet attribut
			attributeStats1 =
			    cast(KWAttributeStats*, odAttributeStats.Lookup(attributePairName->GetSecondSortedName()));
			assert(attributeStats1 != NULL and
			       attributeStats1->GetDescriptiveStats()->GetValueNumber() > 1);

			// Parcours des second attributs potentiel
			for (i2 = 0; i2 < oaSelectedAttributeStats.GetSize(); i2++)
			{
				attributeStats2 = cast(KWAttributeStats*, oaSelectedAttributeStats.GetAt(i2));

				// On passe par une paire de nom de travail pour detecter si la paire est deja
				// specifier parmi les paires specifique
				workingAttributePairName.SetFirstName(attributeStats1->GetSortName());
				workingAttributePairName.SetSecondName(attributeStats2->GetSortName());

				// On n'insere pas les paires deja prises en compte
				if (workingAttributePairName.GetFirstName() !=
					workingAttributePairName.GetSecondName() and
				    slAllSelectedPairs.Find(&workingAttributePairName) == NULL)
				{
					// Creation et initialisation d'un objet de stats pour la paire d'attributs
					attributePairStats = new KWAttributePairStats;
					attributePairStats->SetLearningSpec(learningSpec);
					oaAttributePairStats->Add(attributePairStats);

					// Parametrage des attributs de la paire par ordre alphabetique
					attributePairStats->SetAttributeName1(
					    workingAttributePairName.GetFirstSortedName());
					attributePairStats->SetAttributeName2(
					    workingAttributePairName.GetSecondSortedName());

					// Memorisation dans la liste des paires specifiques selectionnees
					slAllSelectedPairs.Add(workingAttributePairName.Clone());

					// Arret si nombre max de paires atteint
					if (oaAttributePairStats->GetSize() >= GetMaxAttributePairNumber())
						break;
				}
			}
		}
		// Cas d'une paire standard
		else
		{
			assert(attributePairName->GetFirstName() != "");
			assert(attributePairName->GetSecondName() != "");

			// Creation et initialisation d'un objet de stats pour la paire d'attributs
			attributePairStats = new KWAttributePairStats;
			attributePairStats->SetLearningSpec(learningSpec);
			oaAttributePairStats->Add(attributePairStats);

			// Parametrage des attributs de la paire par ordre alphabetique
			attributePairStats->SetAttributeName1(attributePairName->GetFirstSortedName());
			attributePairStats->SetAttributeName2(attributePairName->GetSecondSortedName());

			// Memorisation dans la liste des paires specifiques selectionnees
			// On doit dupliquer la paire, car tout sera detruit a la fin, y compris
			// les paires ajoutees dans le cadre des paires mono-attributs
			slAllSelectedPairs.Add(attributePairName->Clone());
		}

		// Arret si nombre max de paires atteint
		if (oaAttributePairStats->GetSize() >= GetMaxAttributePairNumber())
			break;
	}

	// On continue si le max n'est pas atteint et que l'analyse de toutes les paires est demande
	if (oaAttributePairStats->GetSize() < GetMaxAttributePairNumber() and GetAllAttributePairs())
	{
		// Parcours de toutes les paires potentielles
		for (i1 = 1; i1 < oaSelectedAttributeStats.GetSize(); i1++)
		{
			attributeStats1 = cast(KWAttributeStats*, oaSelectedAttributeStats.GetAt(i1));
			for (i2 = 0; i2 < i1; i2++)
			{
				attributeStats2 = cast(KWAttributeStats*, oaSelectedAttributeStats.GetAt(i2));

				// On passe par une paire de nom de travail pour detecter si la paire est deja
				// specifier parmi les paires specifique
				workingAttributePairName.SetFirstName(attributeStats1->GetSortName());
				workingAttributePairName.SetSecondName(attributeStats2->GetSortName());

				// On n'insere pas les paires deja prises en compte parmi les paires specifiques
				if (slAllSelectedPairs.Find(&workingAttributePairName) == NULL)
				{
					// Creation et initialisation d'un objet de stats pour la paire d'attributs
					attributePairStats = new KWAttributePairStats;
					attributePairStats->SetLearningSpec(learningSpec);
					oaAttributePairStats->Add(attributePairStats);

					// Parametrage des attributs de la paire par ordre alphabetique
					attributePairStats->SetAttributeName1(
					    workingAttributePairName.GetFirstSortedName());
					attributePairStats->SetAttributeName2(
					    workingAttributePairName.GetSecondSortedName());

					// Arret si nombre max de paires atteint
					if (oaAttributePairStats->GetSize() >= GetMaxAttributePairNumber())
						break;
				}
			}

			// Arret si nombre max de paires atteint
			if (oaAttributePairStats->GetSize() >= GetMaxAttributePairNumber())
				break;
		}
	}

	// Nettoyage
	slAllSelectedPairs.DeleteAll();
	ensure(oaAttributePairStats->GetSize() <= GetMaxAttributePairNumber());
}

void KWAttributePairsSpec::SelectAttributeStatsForAttributePairs(const ObjectArray* oaAttributeStats,
								 ObjectDictionary* odAttributeStats,
								 ObjectArray* oaSelectedAttributeStats) const
{
	KWAttributeStats* attributeStats;
	int i;

	require(oaAttributeStats != NULL);
	require(odAttributeStats != NULL);
	require(odAttributeStats->GetCount() == 0);
	require(oaSelectedAttributeStats != NULL);
	require(oaSelectedAttributeStats->GetSize() == 0);

	// Recopie des attributs sources utilisables
	for (i = 0; i < oaAttributeStats->GetSize(); i++)
	{
		attributeStats = cast(KWAttributeStats*, oaAttributeStats->GetAt(i));

		// Memorisation dans le dictionnaire
		odAttributeStats->SetAt(attributeStats->GetAttributeName(), attributeStats);

		// Ajout de l'attribut si assez de valeurs differentes, et si different de
		// l'attribut obligatoire dans les paires
		if (attributeStats->GetDescriptiveStats()->GetValueNumber() > 1)
			oaSelectedAttributeStats->Add(attributeStats);
	}

	// Tri de ces attributs par importance decroissante
	oaSelectedAttributeStats->Shuffle();
	oaSelectedAttributeStats->SetCompareFunction(KWLearningReportCompareSortValue);
	oaSelectedAttributeStats->Sort();
	ensure(oaSelectedAttributeStats->GetSize() <= odAttributeStats->GetCount());
}

void KWAttributePairsSpec::Write(ostream& ost) const
{
	ost << "Max number of variable pairs\t" << GetMaxAttributePairNumber() << "\n";
	ost << "All pairs\t" << BooleanToString(GetAllAttributePairs()) << "\n";
}

const ALString KWAttributePairsSpec::GetClassLabel() const
{
	return "Variable pairs parameters";
}

const ALString KWAttributePairsSpec::GetObjectLabel() const
{
	ALString sObjectLabel;

	require(lContextLineIndex >= 0);
	require(nContextAttributeIndex >= 0);
	require(nContextAttributeIndex <= 2);
	require(nContextAttributeIndex == 0 or lContextLineIndex > 0);

	// Partie contexte
	sObjectLabel = sContextLabel;

	// Partie index de ligne
	if (lContextLineIndex > 0)
	{
		if (sObjectLabel != "")
			sObjectLabel += " line ";
		sObjectLabel += LongintToReadableString(lContextLineIndex);

		// Partie designation de la variable impliquee
		if (nContextAttributeIndex > 0)
		{
			if (nContextAttributeIndex == 1)
				sObjectLabel += ", first variable";
			else
				sObjectLabel += ", second variable";
		}
	}
	return sObjectLabel;
}
