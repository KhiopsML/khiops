// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWTypeAutomaticRecognition.h"

////////////////////////////////////////////////////////////////////
// Classe KWTypeAutomaticRecognition

const KWTypeAvailableFormats* KWTypeAutomaticRecognition::allReferenceFormats = NULL;
int KWTypeAutomaticRecognition::nInstanceNumber = 0;

KWTypeAutomaticRecognition::KWTypeAutomaticRecognition()
{
	nValueNumber = 0;
	nMissingValueNumber = 0;
	nValue0Number = 0;
	nValue1Number = 0;
	nMaxValueLength = 0;
	nMatchingTypeNumber = 1;
	cMinFirstChar = '\0';
	bMatchingContinuous = false;
	bMatchingText = false;
	bIsFinalized = false;

	// Creation si necessaire des formats de references
	if (nInstanceNumber == 0)
	{
		assert(allReferenceFormats == NULL);
		allReferenceFormats = new KWTypeAvailableFormats;
	}
	nInstanceNumber++;
}

KWTypeAutomaticRecognition::~KWTypeAutomaticRecognition()
{
	// Destruction si necessaire des formats de references
	nInstanceNumber--;
	if (nInstanceNumber == 0)
	{
		assert(allReferenceFormats != NULL);
		delete allReferenceFormats;
		allReferenceFormats = NULL;
	}
}

void KWTypeAutomaticRecognition::Initialize()
{
	// Reinitialisation
	nValueNumber = 0;
	nMissingValueNumber = 0;
	nValue0Number = 0;
	nValue1Number = 0;
	nMaxValueLength = 0;
	nMatchingTypeNumber = 1;
	cMinFirstChar = '\0';
	bMatchingContinuous = false;
	bMatchingText = false;
	bIsFinalized = false;
	oaMatchingDateFormats.SetSize(0);
	oaMatchingTimeFormats.SetSize(0);
	oaMatchingTimestampFormats.SetSize(0);
}

boolean KWTypeAutomaticRecognition::IsInitialized() const
{
	return nValueNumber == 0;
}

void KWTypeAutomaticRecognition::AddStringValue(const char* const sValue)
{
	int nValueLength;

	require(not bIsFinalized);
	require(sValue != NULL);

	// Ajout d'une valeur
	nValueNumber++;

	// Comptage des valeurs vides
	if (sValue[0] == '\0')
		nMissingValueNumber++;
	// Analyse si valeur non vide
	else
	{
		// Comptage des valeurs 0 et 1
		if (sValue[1] == '\0')
		{
			if (sValue[0] == '0')
				nValue0Number++;
			else if (sValue[0] == '1')
				nValue1Number++;
		}

		// Traitement de la longueur de la chaine
		nValueLength = (int)strlen(sValue);
		if (nValueLength > nMaxValueLength)
			nMaxValueLength = nValueLength;

		// Traitement du plus petit premier caractere
		if (cMinFirstChar == '\0' or cMinFirstChar > sValue[0])
			cMinFirstChar = sValue[0];

		// Cas de la premiere valeur: initialisation des types et formats utilisables
		if (nValueNumber == nMissingValueNumber + 1)
			AddFirstStringValue(sValue, nValueLength);
		// Prise en compte d'une nouvelle valeur: mise a jour des types et formats utilisables
		else
			AddNewStringValue(sValue, nValueLength);
	}
	assert(1 <= GetMatchingTypeNumber() and GetMatchingTypeNumber() <= 3);
}

void KWTypeAutomaticRecognition::AddFirstStringValue(const char* const sValue, int nValueLength)
{
	const KWDateFormat* dateFormat;
	const KWTimeFormat* timeFormat;
	const KWTimestampFormat* timestampFormat;
	const KWTimestampTZFormat* timestampTZFormat;
	char sRecodedValue[nMaxContinuousFieldSize + 1];
	int nRecodedValueLength;
	int i;
	char cDateSeparator;
	char cTimeSeparator;
	Date dtDate;
	Time tmTime;
	Timestamp tsTimestamp;
	TimestampTZ tstzTimestamp;
	int nDecimalPointOffset;
	int nSignificantLength;
	int nMaxAnalysisLength;
	char cLastChar;
	int nTimeZoneOffset;

	require(not bIsFinalized);
	require(sValue != NULL);
	assert(sValue[0] != '\0');
	require(nValueLength == (int)strlen(sValue));
	assert(nValueNumber == nMissingValueNumber + 1);
	require(nMatchingTypeNumber == 1);

	// Transformation des ',' en '.' pour la reconnaissance du type Continuous
	if (nValueLength <= nMaxContinuousFieldSize)
	{
		strcpy(sRecodedValue, sValue);
		for (i = 0; i < nValueLength; i++)
		{
			if (sValue[i] == ',')
				sRecodedValue[i] = '.';
		}
		nRecodedValueLength = nValueLength;
	}
	// On cree une RecodedValue "artificielle" non Continuous si le champ est trop long
	// Cela permet de simplifier l'ecriture des tests
	else
	{
		strcpy(sRecodedValue, "X");
		nRecodedValueLength = 1;
	}

	// Si premier caractere non numerique: Symbol ou Continuous
	if (not isdigit(sValue[0]))
	{
		if (KWContinuous::IsStringContinuous(sValue) or KWContinuous::IsStringContinuous(sRecodedValue))
		{
			nMatchingTypeNumber++;
			bMatchingContinuous = true;
		}
	}
	// Sinon, tous les types sont eventuellement possibles, si le dernier caractere est numerique ou '.'
	// ou meme 'Z' pour les timestamps avec time zone
	else
	{
		// On continue si le dernier caractere est numerique ou '.' ou 'Z'
		cLastChar = sValue[nValueLength - 1];
		if (isdigit(cLastChar) or cLastChar == '.' or cLastChar == 'Z' or
		    sRecodedValue[nRecodedValueLength - 1] == '.')
		{
			// Si le dernier caractere n'est pas 'Z', on ne peut avoir que le type timestamp
			if (cLastChar != 'Z')
			{
				// Ajout eventuel du type Continuous
				if (KWContinuous::IsStringContinuous(sValue) or
				    KWContinuous::IsStringContinuous(sRecodedValue))
				{
					nMatchingTypeNumber++;
					bMatchingContinuous = true;
				}

				// Test si Date
				if (nValueLength >= allReferenceFormats->GetDateMinCharNumber() and
				    nValueLength <= allReferenceFormats->GetDateMaxCharNumber())
				{
					// Test sur les separateurs
					cDateSeparator = '\0';
					for (i = nValueLength - 3; i >= 2; i--)
					{
						if (not isdigit(sValue[i]))
						{
							cDateSeparator = sValue[i];
							break;
						}
					}

					// On continue si le separateur est compatible
					if (cDateSeparator == '\0' or
					    allReferenceFormats->IsDateSeparator(cDateSeparator))
					{
						// On passe en revue les formats Date
						for (i = 0; i < allReferenceFormats->GetAvailableDateFormatNumber();
						     i++)
						{
							dateFormat = allReferenceFormats->GetAvailableDateFormatAt(i);

							// On memorise le format de date s'il est compatible
							if (cDateSeparator == dateFormat->GetSeparatorChar())
							{
								dtDate = dateFormat->StringToDate(sValue);
								if (dtDate.Check())
									oaMatchingDateFormats.Add(
									    cast(Object*, dateFormat));
							}
						}

						// Ajout eventuel du format Date
						if (oaMatchingDateFormats.GetSize() > 0)
							nMatchingTypeNumber++;
					}
				}
			}

			// Si Date non reconnu, test si Time ou Timestamp
			if (oaMatchingDateFormats.GetSize() == 0)
			{
				// Recherche de la position du point decimal, potentiellement a +-3 pres selon les
				// formats
				nDecimalPointOffset = -1;
				for (i = nValueLength - 1; i > allReferenceFormats->GetTimeMinCharNumber(); i--)
				{
					if (sValue[i] == '.')
					{
						nDecimalPointOffset = i;
						break;
					}
					else if (not isdigit(sValue[i]))
						break;
				}

				// Calcul de la longueur significative (avant la partie decimale potentielle des
				// secondes)
				nSignificantLength = nValueLength;
				if (nDecimalPointOffset != -1)
					nSignificantLength = nDecimalPointOffset;

				// Test si Time
				if (cLastChar != 'Z')
				{
					if (nSignificantLength >= allReferenceFormats->GetTimeMinCharNumber() and
					    nSignificantLength <= allReferenceFormats->GetTimeMaxCharNumber())
					{
						// Test sur les separateurs
						cTimeSeparator = '\0';
						for (i = nSignificantLength - 2; i >= 1; i--)
						{
							if (not isdigit(sValue[i]))
							{
								cTimeSeparator = sValue[i];
								break;
							}
						}

						// On continue si le separateur est compatible
						if (cTimeSeparator == '\0' or
						    allReferenceFormats->IsTimeSeparator(cTimeSeparator))
						{
							// On passe en revue les formats Time
							for (i = 0;
							     i < allReferenceFormats->GetAvailableTimeFormatNumber();
							     i++)
							{
								timeFormat =
								    allReferenceFormats->GetAvailableTimeFormatAt(i);

								// On memorise le format de time s'il est compatible
								if (cTimeSeparator == timeFormat->GetSeparatorChar())
								{
									tmTime = timeFormat->StringToTime(sValue);
									if (tmTime.Check())
										oaMatchingTimeFormats.Add(
										    cast(Object*, timeFormat));
								}
							}

							// Ajout eventuel du format Time
							if (oaMatchingTimeFormats.GetSize() > 0)
								nMatchingTypeNumber++;
						}
					}
				}

				// Si Time non reconnu, test si Timestamp ou TimestampTZ
				assert(not allReferenceFormats->IsTimestampSeparator('.'));
				if (oaMatchingTimeFormats.GetSize() == 0)
				{
					// On regarde si on est dans un format potentiellement de type time zone
					nTimeZoneOffset = -1;
					if (cLastChar == 'Z')
						nTimeZoneOffset = nValueLength - 1;

					// S'il y a une time zone non 'Z', il doit y avoir la place pour cette time zone
					if (nTimeZoneOffset == -1 and
					    nValueLength >= allReferenceFormats->GetTimestampMinCharNumber() +
								KWTimestampTZFormat::nBasicTimeZoneMaxLength)
					{
						if (sValue[nValueLength -
							   KWTimestampTZFormat::nBasicTimeZoneMaxLength] == '-' or
						    sValue[nValueLength -
							   KWTimestampTZFormat::nBasicTimeZoneMaxLength] == '+')
							nTimeZoneOffset =
							    nValueLength - KWTimestampTZFormat::nBasicTimeZoneMaxLength;
						else if (sValue[nValueLength -
								KWTimestampTZFormat::nExtendedTimeZoneMaxLength] ==
							     '-' or
							 sValue[nValueLength -
								KWTimestampTZFormat::nExtendedTimeZoneMaxLength] == '+')
							nTimeZoneOffset =
							    nValueLength -
							    KWTimestampTZFormat::nExtendedTimeZoneMaxLength;
					}

					// Si time zone, on recherche a nouveau la position du point decimal des time
					// avant la partie time zone
					if (nTimeZoneOffset != -1)
					{
						nDecimalPointOffset = -1;
						for (i = nTimeZoneOffset - 1;
						     i > allReferenceFormats->GetDateMinCharNumber(); i--)
						{
							if (sValue[i] == '.')
							{
								nDecimalPointOffset = i;
								break;
							}
							else if (not isdigit(sValue[i]))
								break;
						}

						// Calcul de la longueur significative (avant la partie decimale
						// potentielle des secondes)
						nSignificantLength = nTimeZoneOffset;
						if (nDecimalPointOffset != -1)
							nSignificantLength = nDecimalPointOffset;
					}
					assert(nDecimalPointOffset == -1 or nSignificantLength == nDecimalPointOffset);

					// Correction de la longueur significative dont l'heuriristique de calcul peut
					// etre erronnee dans le cas d'un format Time de type HH.MM ou H(H).M(M) sans
					// les secondes (le dernier '.' n'est alors pas le separateur des secondes)
					if (nDecimalPointOffset != -1)
					{
						if (nDecimalPointOffset >
							allReferenceFormats->GetDateMinCharNumber() and
						    nDecimalPointOffset <
							allReferenceFormats->GetDateMinCharNumber() + 4)
						{
							if (nTimeZoneOffset != -1)
								nSignificantLength = nTimeZoneOffset;
							else
								nSignificantLength = nValueLength;
						}
					}

					// Ajout d'un timestamp si possible
					if (nSignificantLength >= allReferenceFormats->GetDateMinCharNumber() +
								      allReferenceFormats->GetTimeMinCharNumber() and
					    nSignificantLength <= allReferenceFormats->GetDateMaxCharNumber() + 1 +
								      allReferenceFormats->GetTimeMaxCharNumber())
					{
						// Test sur les separateurs de Date
						cDateSeparator = '\0';
						for (i = allReferenceFormats->GetDateMinCharNumber() - 3; i >= 2; i--)
						{
							if (not isdigit(sValue[i]))
							{
								cDateSeparator = sValue[i];
								break;
							}
						}

						// Test sur les separateurs de Time
						// On va rechercher en separateur strictement apres la fin du champ
						// Date, avant la fin du champ Time
						cTimeSeparator = '\0';
						nMaxAnalysisLength = allReferenceFormats->GetDateMaxCharNumber() + 1 +
								     allReferenceFormats->GetTimeMinCharNumber();
						if (cDateSeparator == '\0')
							nMaxAnalysisLength -= 2;
						if (nMaxAnalysisLength > nValueLength)
							nMaxAnalysisLength = nValueLength;
						for (i = allReferenceFormats->GetDateMaxCharNumber() + 1;
						     i < nMaxAnalysisLength; i++)
						{
							if (not isdigit(sValue[i]))
							{
								cTimeSeparator = sValue[i];
								break;
							}
						}

						// On continue si le separateur est compatible
						if ((cDateSeparator == '\0' or
						     allReferenceFormats->IsDateSeparator(cDateSeparator)) and
						    (cTimeSeparator == '\0' or
						     allReferenceFormats->IsTimeSeparator(cTimeSeparator)))
						{
							// Cas sans time zone
							if (nTimeZoneOffset == -1)
							{
								// On passe en revue les formats Timestamp
								for (i = 0;
								     i < allReferenceFormats
									     ->GetAvailableTimestampFormatNumber();
								     i++)
								{
									timestampFormat =
									    allReferenceFormats
										->GetAvailableTimestampFormatAt(i);

									// On memorise le format de time s'il est
									// compatible
									if (cDateSeparator ==
										timestampFormat->GetDateFormat()
										    ->GetSeparatorChar() and
									    cTimeSeparator ==
										timestampFormat->GetTimeFormat()
										    ->GetSeparatorChar())
									{
										tsTimestamp =
										    timestampFormat->StringToTimestamp(
											sValue);
										if (tsTimestamp.Check())
											oaMatchingTimestampFormats.Add(
											    cast(Object*,
												 timestampFormat));
									}
								}

								// Ajout eventuel du format Timestamp
								if (oaMatchingTimestampFormats.GetSize() > 0)
									nMatchingTypeNumber++;
							}
							// Cas avec time zone
							else
							{
								// On passe en revue les formats TimestampTZ
								for (i = 0;
								     i < allReferenceFormats
									     ->GetAvailableTimestampTZFormatNumber();
								     i++)
								{
									timestampTZFormat =
									    allReferenceFormats
										->GetAvailableTimestampTZFormatAt(i);

									// On memorise le format de time s'il est
									// compatible
									if (cDateSeparator ==
										timestampTZFormat->GetTimestampFormat()
										    ->GetDateFormat()
										    ->GetSeparatorChar() and
									    cTimeSeparator ==
										timestampTZFormat->GetTimestampFormat()
										    ->GetTimeFormat()
										    ->GetSeparatorChar())
									{
										tstzTimestamp =
										    timestampTZFormat
											->StringToTimestampTZ(sValue);
										if (tstzTimestamp.Check())
											oaMatchingTimestampTZFormats
											    .Add(cast(
												Object*,
												timestampTZFormat));
									}
								}

								// Ajout eventuel du format Timestamp
								if (oaMatchingTimestampTZFormats.GetSize() > 0)
									nMatchingTypeNumber++;
							}
						}
					}
				}
			}
		}
	}
}

void KWTypeAutomaticRecognition::AddNewStringValue(const char* const sValue, int nValueLength)
{
	const KWDateFormat* dateFormat;
	const KWTimeFormat* timeFormat;
	const KWTimestampFormat* timestampFormat;
	const KWTimestampTZFormat* timestampTZFormat;
	char sRecodedValue[nMaxContinuousFieldSize + 1];
	int nRecodedValueLength;
	int i;
	Date dtDate;
	Time tmTime;
	Timestamp tsTimestamp;
	TimestampTZ tstzTimestamp;
	int nNewI;
	char cLastChar;

	require(not bIsFinalized);
	require(sValue != NULL);
	require(sValue[0] != '\0');
	require(nValueLength == (int)strlen(sValue));
	require(nValueNumber > nMissingValueNumber + 1);

	// Si un seul type restant (Symbol forcement), on ne fait pas d'analyse
	if (nMatchingTypeNumber == 1)
		return;

	// Transformation des ',' en '.' pour la reconnaissance du type Continuous
	if (nValueLength <= nMaxContinuousFieldSize)
	{
		strcpy(sRecodedValue, sValue);
		for (i = 0; i < nValueLength; i++)
		{
			if (sValue[i] == ',')
				sRecodedValue[i] = '.';
		}
		nRecodedValueLength = nValueLength;
	}
	// On cree une RecodedValue "artificielle" non Continuous si le champ est trop long
	// Cela permet de simplifier l'ecriture des tests
	else
	{
		strcpy(sRecodedValue, "X");
		nRecodedValueLength = 1;
	}

	// Si premier caractere non numerique: Symbol ou Continuous
	if (not isdigit(sValue[0]))
	{
		if (bMatchingContinuous and not KWContinuous::IsStringContinuous(sValue) and
		    not KWContinuous::IsStringContinuous(sRecodedValue))
		{
			nMatchingTypeNumber--;
			bMatchingContinuous = false;
		}
	}
	// Sinon, tous les types sont eventuellement possibles, si le dernier caractere est numerique ou '.'
	// ou meme 'Z' pour les timestamps avec time zone
	else
	{
		// Suppression eventuelle des types si le dernier caractere n'est ni numerique ni '.'
		cLastChar = sValue[nValueLength - 1];
		if (not isdigit(cLastChar) and cLastChar != '.' and sRecodedValue[nRecodedValueLength - 1] != '.')
		{
			if (bMatchingContinuous and not KWContinuous::IsStringContinuous(sValue) and
			    not KWContinuous::IsStringContinuous(sRecodedValue))
			{
				nMatchingTypeNumber--;
				bMatchingContinuous = false;
			}
			if (oaMatchingDateFormats.GetSize() > 0)
			{
				nMatchingTypeNumber--;
				oaMatchingDateFormats.SetSize(0);
			}
			else if (oaMatchingTimeFormats.GetSize() > 0)
			{
				nMatchingTypeNumber--;
				oaMatchingTimeFormats.SetSize(0);
			}
			else if (oaMatchingTimestampFormats.GetSize() > 0)
			{
				nMatchingTypeNumber--;
				oaMatchingTimestampFormats.SetSize(0);
			}
			// On accepte egalement du type TimestampTZ avec time zone, se terminant par 'Z'
			else if (oaMatchingTimestampTZFormats.GetSize() > 0 and cLastChar != 'Z')
			{
				nMatchingTypeNumber--;
				oaMatchingTimestampTZFormats.SetSize(0);
			}
			assert(nMatchingTypeNumber == 1 or oaMatchingTimestampTZFormats.GetSize() > 0);
		}
		// On continue si le dernier caractere est numerique ou '.'
		else
		{
			// Supression eventuelle du type Continuous
			if (bMatchingContinuous and not KWContinuous::IsStringContinuous(sValue) and
			    not KWContinuous::IsStringContinuous(sRecodedValue))
			{
				nMatchingTypeNumber--;
				bMatchingContinuous = false;
			}

			// Suppression eventuelle du type Date
			if (oaMatchingDateFormats.GetSize() > 0)
			{
				// On passe en revue les formats Date
				nNewI = 0;
				for (i = 0; i < oaMatchingDateFormats.GetSize(); i++)
				{
					dateFormat = cast(const KWDateFormat*, oaMatchingDateFormats.GetAt(i));

					// On garde les formats de date compatibles
					dtDate = dateFormat->StringToDate(sValue);
					if (dtDate.Check())
					{
						oaMatchingDateFormats.SetAt(nNewI, cast(Object*, dateFormat));
						nNewI++;
					}
				}
				oaMatchingDateFormats.SetSize(nNewI);

				// Suppression eventuelle du format Date
				if (oaMatchingDateFormats.GetSize() == 0)
					nMatchingTypeNumber--;
			}
			// Suppression eventuelle du type Time
			else if (oaMatchingTimeFormats.GetSize() > 0)
			{
				// On passe en revue les formats Time
				nNewI = 0;
				for (i = 0; i < oaMatchingTimeFormats.GetSize(); i++)
				{
					timeFormat = cast(const KWTimeFormat*, oaMatchingTimeFormats.GetAt(i));

					// On garde les formats de time compatibles
					tmTime = timeFormat->StringToTime(sValue);
					if (tmTime.Check())
					{
						oaMatchingTimeFormats.SetAt(nNewI, cast(Object*, timeFormat));
						nNewI++;
					}
				}
				oaMatchingTimeFormats.SetSize(nNewI);

				// Suppression eventuelle du format Time
				if (oaMatchingTimeFormats.GetSize() == 0)
					nMatchingTypeNumber--;
			}
			// Suppression eventuelle du type Timestamp
			else if (oaMatchingTimestampFormats.GetSize() > 0)
			{
				// On passe en revue les formats Timestamp
				nNewI = 0;
				for (i = 0; i < oaMatchingTimestampFormats.GetSize(); i++)
				{
					timestampFormat =
					    cast(const KWTimestampFormat*, oaMatchingTimestampFormats.GetAt(i));

					// On garde les formats de timestamp compatibles
					tsTimestamp = timestampFormat->StringToTimestamp(sValue);
					if (tsTimestamp.Check())
					{
						oaMatchingTimestampFormats.SetAt(nNewI, cast(Object*, timestampFormat));
						nNewI++;
					}
				}
				oaMatchingTimestampFormats.SetSize(nNewI);

				// Suppression eventuelle du format Timestamp
				if (oaMatchingTimestampFormats.GetSize() == 0)
					nMatchingTypeNumber--;
			}
		}

		// Suppression eventuelle du type TimestampTZ, traite a part pour tenir compte des terminaison
		// potentielles par 'Z'
		if (oaMatchingTimestampTZFormats.GetSize() > 0)
		{
			assert(cLastChar == 'Z' or cLastChar == '.' or isdigit(cLastChar));

			// On passe en revue les formats TimestampTZ
			nNewI = 0;
			for (i = 0; i < oaMatchingTimestampTZFormats.GetSize(); i++)
			{
				timestampTZFormat =
				    cast(const KWTimestampTZFormat*, oaMatchingTimestampTZFormats.GetAt(i));

				// On garde les formats de timestampTZ compatibles
				tstzTimestamp = timestampTZFormat->StringToTimestampTZ(sValue);
				if (tstzTimestamp.Check())
				{
					oaMatchingTimestampTZFormats.SetAt(nNewI, cast(Object*, timestampTZFormat));
					nNewI++;
				}
			}
			oaMatchingTimestampTZFormats.SetSize(nNewI);

			// Suppression eventuelle du format Timestamp
			if (oaMatchingTimestampTZFormats.GetSize() == 0)
				nMatchingTypeNumber--;
		}
	}
}

int KWTypeAutomaticRecognition::GetValueNumber() const
{
	return nValueNumber;
}

int KWTypeAutomaticRecognition::GetMissingValueNumber() const
{
	return nMissingValueNumber;
}

int KWTypeAutomaticRecognition::GetValue0Number() const
{
	return nValue0Number;
}

int KWTypeAutomaticRecognition::GetValue1Number() const
{
	return nValue1Number;
}

int KWTypeAutomaticRecognition::GetMaxValueLength() const
{
	return nMaxValueLength;
}

void KWTypeAutomaticRecognition::Finalize()
{
	int n;
	int i;
	boolean bRemoveFormat;
	const KWTimeFormat* timeFormat;

	require(not bIsFinalized);
	assert(nMinTextFieldSize < KWValue::nMaxSymbolFieldSize);

	// Detection du type Text
	if (GetLearningTextVariableMode())
		bMatchingText = (nMaxValueLength >= nMinTextFieldSize);

	// Invalidation du type Continuous s'il n'y a des 0 et des 1, plus eventuellement des Missing
	if (bMatchingContinuous)
	{
		if (nValue0Number + nValue1Number + nMissingValueNumber == nValueNumber)
		{
			bMatchingContinuous = false;
			nMatchingTypeNumber--;
		}
	}

	// Nettoyage des formats time
	if (oaMatchingTimeFormats.GetSize() > 0)
	{
		n = 0;
		while (n < oaMatchingTimeFormats.GetSize())
		{
			timeFormat = cast(const KWTimeFormat*, oaMatchingTimeFormats.GetAt(n));

			// Cas particulier du format (H)H.(M)M qui peut etre confondu
			// avec un cas numerique simple: on garde le type Time uniquement
			// si on des valeurs de longueur 5 (chiffre des dizaines non null
			// pour les heures et le minutes)
			bRemoveFormat = false;
			if (timeFormat->GetFormatString() == "(H)H.(M)M" and GetMaxValueLength() < 5)
				bRemoveFormat = true;
			// Cas particulier du format HHMM qui peut etre confondu
			// avec un cas numerique simple: on garde le type Time uniquement
			// si le plus petit premier caractere est 0
			// (pour eviter de trairer des annee de type 2007 comme des heures)
			else if (timeFormat->GetFormatString() == "HHMM" and cMinFirstChar > '0')
				bRemoveFormat = true;

			// Supression du format si necessaire
			if (bRemoveFormat)
			{
				for (i = n + 1; i < oaMatchingTimeFormats.GetSize(); i++)
					oaMatchingTimeFormats.SetAt(i - 1, oaMatchingTimeFormats.GetAt(i));
				oaMatchingTimeFormats.SetSize(oaMatchingTimeFormats.GetSize() - 1);
			}

			// Passage au format suivant
			n++;
		}

		// Supression d'un type de format si necessaire
		if (oaMatchingTimeFormats.GetSize() == 0)
			nMatchingTypeNumber--;
	}
	bIsFinalized = true;
}

boolean KWTypeAutomaticRecognition::IsFinalized() const
{
	return bIsFinalized;
}

int KWTypeAutomaticRecognition::GetMainMatchingType() const
{
	require(bIsFinalized);

	if (bMatchingContinuous)
		return KWType::Continuous;
	else if (nMatchingTypeNumber == 1)
	{
		if (bMatchingText)
			return KWType::Text;
		else
			return KWType::Symbol;
	}
	else if (GetMatchingDateFormatNumber() > 0)
		return KWType::Date;
	else if (GetMatchingTimestampFormatNumber() > 0)
		return KWType::Timestamp;
	else if (GetMatchingTimestampTZFormatNumber() > 0)
		return KWType::TimestampTZ;
	// On teste le format Time en dernier, car c'est le seul a avoir des exceptions heuristiques (plus long a
	// calculer)
	else
		return KWType::Time;
}

int KWTypeAutomaticRecognition::GetMatchingTypeNumber() const
{
	return nMatchingTypeNumber + bMatchingText;
}

int KWTypeAutomaticRecognition::GetMatchingTypeAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetMatchingTypeNumber());

	// On renvoie d'abord le type principal
	if (nIndex == 0)
		return GetMainMatchingType();
	// Le type Symbol arrive forcement en dernier
	else if (nIndex == GetMatchingTypeNumber() - 1)
		return KWType::Symbol;
	// Le type Text, s'il est disponible, arrive forcement en avant dernier
	else if (nIndex == GetMatchingTypeNumber() - 2 and bMatchingText)
		return KWType::Text;
	// Les types Complex arrivent en fin
	else
	{
		assert(nIndex == 1 and GetMatchingTypeNumber() == 3);
		assert(GetMatchingDateFormatNumber() > 0 or GetMatchingTimeFormatNumber() > 0 or
		       GetMatchingTimestampFormatNumber() > 0);
		// On teste le format Time en dernier, car c'est le seul a avoir des exceptions heuristiques (plus long
		// a calculer)
		if (GetMatchingDateFormatNumber() > 0)
			return KWType::Date;
		else if (GetMatchingTimestampFormatNumber() > 0)
			return KWType::Timestamp;
		else
			return KWType::Time;
	}
}

boolean KWTypeAutomaticRecognition::IsContinuousTypeMatching() const
{
	require(bIsFinalized);
	return bMatchingContinuous;
}

int KWTypeAutomaticRecognition::GetMatchingDateFormatNumber() const
{
	require(bIsFinalized);
	return oaMatchingDateFormats.GetSize();
}

const KWDateFormat* KWTypeAutomaticRecognition::GetMatchingDateFormatAt(int nIndex) const
{
	require(bIsFinalized);
	return cast(const KWDateFormat*, oaMatchingDateFormats.GetAt(nIndex));
}

int KWTypeAutomaticRecognition::GetMatchingTimeFormatNumber() const
{
	require(bIsFinalized);
	return oaMatchingTimeFormats.GetSize();
}

const KWTimeFormat* KWTypeAutomaticRecognition::GetMatchingTimeFormatAt(int nIndex) const
{
	require(bIsFinalized);
	return cast(const KWTimeFormat*, oaMatchingTimeFormats.GetAt(nIndex));
}

int KWTypeAutomaticRecognition::GetMatchingTimestampFormatNumber() const
{
	require(bIsFinalized);
	return oaMatchingTimestampFormats.GetSize();
}

const KWTimestampFormat* KWTypeAutomaticRecognition::GetMatchingTimestampFormatAt(int nIndex) const
{
	require(bIsFinalized);
	return cast(const KWTimestampFormat*, oaMatchingTimestampFormats.GetAt(nIndex));
}

int KWTypeAutomaticRecognition::GetMatchingTimestampTZFormatNumber() const
{
	require(bIsFinalized);
	return oaMatchingTimestampTZFormats.GetSize();
}

const KWTimestampTZFormat* KWTypeAutomaticRecognition::GetMatchingTimestampTZFormatAt(int nIndex) const
{
	require(bIsFinalized);
	return cast(const KWTimestampTZFormat*, oaMatchingTimestampTZFormats.GetAt(nIndex));
}

boolean KWTypeAutomaticRecognition::AreMatchingDateFormatConsistent() const
{
	int i;

	require(bIsFinalized);

	// Test de coherents des formats avec le premier
	for (i = 1; i < GetMatchingDateFormatNumber(); i++)
	{
		if (not GetMatchingDateFormatAt(0)->IsConsistentWith(GetMatchingDateFormatAt(i)))
			return false;
	}
	return true;
}

boolean KWTypeAutomaticRecognition::AreMatchingTimeFormatConsistent() const
{
	int i;

	require(bIsFinalized);

	// Test de coherents des formats avec le premier
	for (i = 1; i < GetMatchingTimeFormatNumber(); i++)
	{
		if (not GetMatchingTimeFormatAt(0)->IsConsistentWith(GetMatchingTimeFormatAt(i)))
			return false;
	}
	return true;
}

boolean KWTypeAutomaticRecognition::AreMatchingTimestampFormatConsistent() const
{
	int i;

	require(bIsFinalized);

	// Test de coherents des formats avec le premier
	for (i = 1; i < GetMatchingTimestampFormatNumber(); i++)
	{
		if (not GetMatchingTimestampFormatAt(0)->IsConsistentWith(GetMatchingTimestampFormatAt(i)))
			return false;
	}
	return true;
}

boolean KWTypeAutomaticRecognition::AreMatchingTimestampTZFormatConsistent() const
{
	int i;

	require(bIsFinalized);

	// Test de coherents des formats avec le premier
	for (i = 1; i < GetMatchingTimestampTZFormatNumber(); i++)
	{
		if (not GetMatchingTimestampTZFormatAt(0)->IsConsistentWith(GetMatchingTimestampTZFormatAt(i)))
			return false;
	}
	return true;
}

boolean KWTypeAutomaticRecognition::IsMatchingDateFormat(const KWDateFormat* format) const
{
	int i;

	require(format != NULL);
	require(bIsFinalized);

	// Test de coherents des formats avec le premier
	for (i = 0; i < GetMatchingDateFormatNumber(); i++)
	{
		if (GetMatchingDateFormatAt(i)->GetFormatString() == format->GetFormatString())
			return true;
	}
	return false;
}

boolean KWTypeAutomaticRecognition::IsMatchingTimeFormat(const KWTimeFormat* format) const
{
	int i;

	require(format != NULL);
	require(bIsFinalized);

	// Test de coherents des formats avec le premier
	for (i = 0; i < GetMatchingTimeFormatNumber(); i++)
	{
		if (GetMatchingTimeFormatAt(i)->GetFormatString() == format->GetFormatString())
			return true;
	}
	return false;
}

boolean KWTypeAutomaticRecognition::IsMatchingTimestampFormat(const KWTimestampFormat* format) const
{
	int i;

	require(format != NULL);
	require(bIsFinalized);

	// Test de coherents des formats avec le premier
	for (i = 0; i < GetMatchingTimestampFormatNumber(); i++)
	{
		if (GetMatchingTimestampFormatAt(i)->GetFormatString() == format->GetFormatString())
			return true;
	}
	return false;
}

boolean KWTypeAutomaticRecognition::IsMatchingTimestampTZFormat(const KWTimestampTZFormat* format) const
{
	int i;

	require(format != NULL);
	require(bIsFinalized);

	// Test de coherents des formats avec le premier
	for (i = 0; i < GetMatchingTimestampTZFormatNumber(); i++)
	{
		if (GetMatchingTimestampTZFormatAt(i)->GetFormatString() == format->GetFormatString())
			return true;
	}
	return false;
}

void KWTypeAutomaticRecognition::Write(ostream& ost) const
{
	int nType;

	ost << "Automatic type recognition" << endl;
	ost << "\tValues\t" << GetValueNumber() << endl;
	ost << "\tMissing\t" << GetMissingValueNumber() << endl;
	ost << "\tMax length\t" << GetMaxValueLength() << endl;
	for (nType = 0; nType < GetMatchingTypeNumber(); nType++)
		ost << "\tType " << nType + 1 << "\t" << KWType::ToString(GetMatchingTypeAt(nType)) << endl;
}

void KWTypeAutomaticRecognition::WriteHeaderLine(ostream& ost) const
{
	ost << "Values\tMissing\tMax length\t";
	ost << "Types\tMain type\tFormats\tMain format\tOther formats\tConsistency";
}

void KWTypeAutomaticRecognition::WriteLine(ostream& ost) const
{
	int nFormat;

	// Donnees principales
	ost << GetValueNumber() << "\t";
	ost << GetMissingValueNumber() << "\t";
	ost << GetMaxValueLength() << "\t";
	ost << GetMatchingTypeNumber() << "\t";
	ost << KWType::ToString(GetMainMatchingType()) << "\t";

	// Traitement des formats des types complexes
	if (GetMainMatchingType() == KWType::Symbol or
	    (GetMainMatchingType() == KWType::Continuous and GetMatchingTypeNumber() == 2))
		ost << "\t\t\t";
	else
	{
		if (GetMainMatchingType() == KWType::Date or
		    (GetMainMatchingType() == KWType::Continuous and GetMatchingDateFormatNumber() > 0))
		{
			ost << GetMatchingDateFormatNumber() << "\t";
			ost << GetMatchingDateFormatAt(0)->GetFormatString() << "\t";
			for (nFormat = 1; nFormat < GetMatchingDateFormatNumber(); nFormat++)
				ost << GetMatchingDateFormatAt(nFormat)->GetFormatString() << " ";
			ost << "\t" << AreMatchingDateFormatConsistent();
		}
		else if (GetMainMatchingType() == KWType::Time or
			 (GetMainMatchingType() == KWType::Continuous and GetMatchingTimeFormatNumber() > 0))
		{
			ost << GetMatchingTimeFormatNumber() << "\t";
			ost << GetMatchingTimeFormatAt(0)->GetFormatString() << "\t";
			for (nFormat = 1; nFormat < GetMatchingTimeFormatNumber(); nFormat++)
				ost << GetMatchingTimeFormatAt(nFormat)->GetFormatString() << " ";
			ost << "\t" << AreMatchingTimeFormatConsistent();
		}
		else if (GetMainMatchingType() == KWType::Timestamp or
			 (GetMainMatchingType() == KWType::Continuous and GetMatchingTimestampFormatNumber() > 0))
		{
			ost << GetMatchingTimestampFormatNumber() << "\t";
			ost << GetMatchingTimestampFormatAt(0)->GetFormatString() << "\t";
			for (nFormat = 1; nFormat < GetMatchingTimestampFormatNumber(); nFormat++)
				ost << GetMatchingTimestampFormatAt(nFormat)->GetFormatString() << " ";
			ost << "\t" << AreMatchingTimestampFormatConsistent();
		}
		else if (GetMainMatchingType() == KWType::TimestampTZ or
			 (GetMainMatchingType() == KWType::Continuous and GetMatchingTimestampTZFormatNumber() > 0))
		{
			ost << GetMatchingTimestampTZFormatNumber() << "\t";
			ost << GetMatchingTimestampTZFormatAt(0)->GetFormatString() << "\t";
			for (nFormat = 1; nFormat < GetMatchingTimestampTZFormatNumber(); nFormat++)
				ost << GetMatchingTimestampTZFormatAt(nFormat)->GetFormatString() << " ";
			ost << "\t" << AreMatchingTimestampTZFormatConsistent();
		}
		else
			ost << "\t\t\t";
	}
}

void KWTypeAutomaticRecognition::Test()
{
	const int nValueNumber = 100;
	int nValue;
	KWTypeAvailableFormats availableFormats;
	KWTypeAutomaticRecognition typeAutomaticRecognition;
	const KWDateFormat* dateFormat;
	const KWTimeFormat* timeFormat;
	const KWTimestampFormat* timestampFormat;
	const KWTimestampTZFormat* timestampTZFormat;
	int nFormat;
	Timestamp tsTimestamp;
	TimestampTZ tstzTimestamp;
	ALString sValue;
	ALString sSampleValue;
	int nTotalTestNumber;
	int nTotalMatchNumber;
	boolean bOk;

	// Ligne d'entete generique pour tous les resultats
	cout << "Type\tIndex\tFormat\tSample value\t";
	typeAutomaticRecognition.WriteHeaderLine(cout);
	cout << "\tOk\n";

	// OInitialisation des statistiques collectees
	nTotalTestNumber = 0;
	nTotalMatchNumber = 0;

	// Reconnaissance de Symbol
	SetRandomSeed(1);
	typeAutomaticRecognition.Initialize();
	sSampleValue = "";
	for (nValue = 0; nValue < nValueNumber; nValue++)
	{
		sValue = "";
		if (RandomDouble() > 0.1)
		{
			sValue += "V";
			sValue += IntToString(nValue + 1);
		}
		if (sSampleValue == "" and sValue != "")
			sSampleValue = sValue;
		typeAutomaticRecognition.AddStringValue(sValue);
	}
	cout << "Categorical\t\t\t" << sSampleValue << "\t";
	typeAutomaticRecognition.Finalize();
	typeAutomaticRecognition.WriteLine(cout);
	bOk = (KWType::Symbol == typeAutomaticRecognition.GetMainMatchingType());
	cout << "\t" << bOk << "\n";
	nTotalTestNumber++;
	nTotalMatchNumber += bOk;

	// Reconnaissance de Continuous
	SetRandomSeed(1);
	typeAutomaticRecognition.Initialize();
	sSampleValue = "";
	for (nValue = 0; nValue < nValueNumber; nValue++)
	{
		sValue = "";
		if (RandomDouble() > 0.1)
			sValue = DoubleToString(RandomDouble());
		if (sSampleValue == "" and sValue != "")
			sSampleValue = sValue;
		typeAutomaticRecognition.AddStringValue(sValue);
	}
	cout << "Numerical\t\t\t" << sSampleValue << "\t";
	typeAutomaticRecognition.Finalize();
	typeAutomaticRecognition.WriteLine(cout);
	bOk = (KWType::Continuous == typeAutomaticRecognition.GetMainMatchingType());
	cout << "\t" << bOk << "\n";
	nTotalTestNumber++;
	nTotalMatchNumber += bOk;

	// Reconnaissance de Date
	for (nFormat = 0; nFormat < availableFormats.GetAvailableDateFormatNumber(); nFormat++)
	{
		dateFormat = availableFormats.GetAvailableDateFormatAt(nFormat);
		SetRandomSeed(1);
		typeAutomaticRecognition.Initialize();
		sSampleValue = "";
		for (nValue = 0; nValue < nValueNumber; nValue++)
		{
			sValue = "";
			tsTimestamp.Reset();
			if (RandomDouble() > 0.1)
			{
				tsTimestamp.Init(2010, 1, 1, 0, 0, 0);
				tsTimestamp.AddSeconds(100000000 * RandomDouble());
			}
			sValue = dateFormat->DateToString(tsTimestamp.GetDate());
			if (sSampleValue == "" and sValue != "")
				sSampleValue = sValue;
			typeAutomaticRecognition.AddStringValue(sValue);
		}
		cout << "Date\t" << nFormat << "\t" << dateFormat->GetFormatString() << "\t" << sSampleValue << "\t";
		typeAutomaticRecognition.Finalize();
		typeAutomaticRecognition.WriteLine(cout);
		bOk = typeAutomaticRecognition.IsMatchingDateFormat(dateFormat);
		cout << "\t" << bOk << "\n";
		nTotalTestNumber++;
		nTotalMatchNumber += bOk;
	}

	// Reconnaissance de Date avec confusion possible sur ordre de jour et mois
	for (nFormat = 0; nFormat < availableFormats.GetAvailableDateFormatNumber(); nFormat++)
	{
		dateFormat = availableFormats.GetAvailableDateFormatAt(nFormat);
		SetRandomSeed(1);
		typeAutomaticRecognition.Initialize();
		sSampleValue = "";
		for (nValue = 0; nValue < nValueNumber; nValue++)
		{
			sValue = "";
			tsTimestamp.Reset();
			if (RandomDouble() > 0.1)
			{
				tsTimestamp.Init(2010, 1 + RandomInt(11), 1 + RandomInt(11), 0, 0, 0);
			}
			sValue = dateFormat->DateToString(tsTimestamp.GetDate());
			if (sSampleValue == "" and sValue != "")
				sSampleValue = sValue;
			typeAutomaticRecognition.AddStringValue(sValue);
		}
		cout << "Date\t" << nFormat << "\t" << dateFormat->GetFormatString() << "\t" << sSampleValue << "\t";
		typeAutomaticRecognition.Finalize();
		typeAutomaticRecognition.WriteLine(cout);
		bOk = typeAutomaticRecognition.IsMatchingDateFormat(dateFormat);
		cout << "\t" << bOk << "\n";
		nTotalTestNumber++;
		nTotalMatchNumber += bOk;
	}

	// Reconnaissance de Time
	for (nFormat = 0; nFormat < availableFormats.GetAvailableTimeFormatNumber(); nFormat++)
	{
		timeFormat = availableFormats.GetAvailableTimeFormatAt(nFormat);
		SetRandomSeed(1);
		typeAutomaticRecognition.Initialize();
		sSampleValue = "";
		for (nValue = 0; nValue < nValueNumber; nValue++)
		{
			sValue = "";
			tsTimestamp.Reset();
			if (RandomDouble() > 0.1)
			{
				tsTimestamp.Init(2010, 1, 1, 0, 0, 0);
				tsTimestamp.AddSeconds(100000000 * RandomDouble());
			}
			sValue = timeFormat->TimeToString(tsTimestamp.GetTime());
			if (sSampleValue == "" and sValue != "")
				sSampleValue = sValue;
			typeAutomaticRecognition.AddStringValue(sValue);
		}
		cout << "Time\t" << nFormat << "\t" << timeFormat->GetFormatString() << "\t" << sSampleValue << "\t";
		typeAutomaticRecognition.Finalize();
		typeAutomaticRecognition.WriteLine(cout);
		bOk = typeAutomaticRecognition.IsMatchingTimeFormat(timeFormat);
		cout << "\t" << bOk << "\n";
		nTotalTestNumber++;
		nTotalMatchNumber += bOk;
	}

	// Reconnaissance de Time avec pas de dizaine sur les heures, minutes et secondes
	for (nFormat = 0; nFormat < availableFormats.GetAvailableTimeFormatNumber(); nFormat++)
	{
		timeFormat = availableFormats.GetAvailableTimeFormatAt(nFormat);
		SetRandomSeed(1);
		typeAutomaticRecognition.Initialize();
		sSampleValue = "";
		for (nValue = 0; nValue < nValueNumber; nValue++)
		{
			sValue = "";
			tsTimestamp.Reset();
			if (RandomDouble() > 0.1)
			{
				tsTimestamp.Init(2010, 1, 1, RandomInt(9), RandomInt(9), RandomInt(9));
				tsTimestamp.AddSeconds(RandomDouble());
			}
			sValue = timeFormat->TimeToString(tsTimestamp.GetTime());
			if (sSampleValue == "" and sValue != "")
				sSampleValue = sValue;
			typeAutomaticRecognition.AddStringValue(sValue);
		}
		cout << "Time\t" << nFormat << "\t" << timeFormat->GetFormatString() << "\t" << sSampleValue << "\t";
		typeAutomaticRecognition.Finalize();
		typeAutomaticRecognition.WriteLine(cout);
		bOk = typeAutomaticRecognition.IsMatchingTimeFormat(timeFormat);
		cout << "\t" << bOk << "\n";
		nTotalTestNumber++;
		nTotalMatchNumber += bOk;
	}

	// Reconnaissance de Timestamp
	for (nFormat = 0; nFormat < availableFormats.GetAvailableTimestampFormatNumber(); nFormat++)
	{
		timestampFormat = availableFormats.GetAvailableTimestampFormatAt(nFormat);
		SetRandomSeed(1);
		typeAutomaticRecognition.Initialize();
		sSampleValue = "";
		for (nValue = 0; nValue < nValueNumber; nValue++)
		{
			sValue = "";
			tsTimestamp.Reset();
			if (RandomDouble() > 0.1)
			{
				tsTimestamp.Init(2010, 1, 1, 0, 0, 0);
				tsTimestamp.AddSeconds(100000000 * RandomDouble());
			}
			sValue = timestampFormat->TimestampToString(tsTimestamp);
			if (sSampleValue == "" and sValue != "")
				sSampleValue = sValue;
			typeAutomaticRecognition.AddStringValue(sValue);
		}
		cout << "Timestamp\t" << nFormat << "\t" << timestampFormat->GetFormatString() << "\t" << sSampleValue
		     << "\t";
		typeAutomaticRecognition.Finalize();
		typeAutomaticRecognition.WriteLine(cout);
		bOk = typeAutomaticRecognition.IsMatchingTimestampFormat(timestampFormat);
		cout << "\t" << bOk << "\n";
		nTotalTestNumber++;
		nTotalMatchNumber += bOk;
	}

	// Reconnaissance de TimestampTZ
	for (nFormat = 0; nFormat < availableFormats.GetAvailableTimestampTZFormatNumber(); nFormat++)
	{
		timestampTZFormat = availableFormats.GetAvailableTimestampTZFormatAt(nFormat);
		SetRandomSeed(1);
		typeAutomaticRecognition.Initialize();
		sSampleValue = "";
		for (nValue = 0; nValue < nValueNumber; nValue++)
		{
			sValue = "";
			tstzTimestamp.Reset();
			if (RandomDouble() > 0.1)
			{
				tsTimestamp.Reset();
				tsTimestamp.Init(2010, 1, 1, 0, 0, 0);
				tsTimestamp.AddSeconds(100000000 * RandomDouble());
				tstzTimestamp.Init(tsTimestamp, (-12 + RandomInt(26)) * 60);
			}
			sValue = timestampTZFormat->TimestampTZToString(tstzTimestamp);
			if (sSampleValue == "" and sValue != "")
				sSampleValue = sValue;
			typeAutomaticRecognition.AddStringValue(sValue);
		}
		cout << "TimestampTZ\t" << nFormat << "\t" << timestampTZFormat->GetFormatString() << "\t"
		     << sSampleValue << "\t";
		typeAutomaticRecognition.Finalize();
		typeAutomaticRecognition.WriteLine(cout);
		bOk = typeAutomaticRecognition.IsMatchingTimestampTZFormat(timestampTZFormat);
		cout << "\t" << bOk << "\n";
		nTotalTestNumber++;
		nTotalMatchNumber += bOk;
	}

	// Affichage des statistiques
	cout << "\nSynthesis\n";
	cout << "\tTotal test number\t" << nTotalTestNumber << "\n";
	cout << "\tTotal match number\t" << nTotalMatchNumber << "\n";
}

////////////////////////////////////////////////////////////////////
// Classe KWTypeAvailableFormats

KWTypeAvailableFormats::KWTypeAvailableFormats()
{
	int i;
	KWDateFormat* dateFormat;
	KWTimeFormat* timeFormat;
	KWTimestampFormat* timestampFormat;
	KWTimestampTZFormat* timestampTZFormat;

	// Initialisation
	nDateMinCharNumber = 0;
	nDateMaxCharNumber = 0;
	nTimeMinCharNumber = 0;
	nTimeMaxCharNumber = 0;
	nTimestampMinCharNumber = 0;
	nTimestampMaxCharNumber = 0;
	nTimezoneMaxCharNumber = 0;

	// Collecte des formats disponibles
	KWDateFormat::GetAllAvailableFormats(&oaAvailableDateFormats);
	KWTimeFormat::GetAllAvailableFormats(&oaAvailableTimeFormats);
	KWTimestampFormat::GetAllAvailableFormats(&oaAvailableTimestampFormats);
	KWTimestampTZFormat::GetAllAvailableFormats(&oaAvailableTimestampTZFormats);

	// Collecte des statistiques sur les formats Date
	for (i = 0; i < oaAvailableDateFormats.GetSize(); i++)
	{
		dateFormat = cast(KWDateFormat*, oaAvailableDateFormats.GetAt(i));

		// Initialisation pour le premier format
		if (i == 0)
		{
			nDateMinCharNumber = dateFormat->GetMinCharNumber();
			nDateMaxCharNumber = dateFormat->GetMaxCharNumber();
		}
		// Mise a jour sinon, pour trouver le min des min et le max des max
		else
		{
			if (nDateMinCharNumber > dateFormat->GetMinCharNumber())
				nDateMinCharNumber = dateFormat->GetMinCharNumber();
			if (nDateMaxCharNumber < dateFormat->GetMaxCharNumber())
				nDateMaxCharNumber = dateFormat->GetMaxCharNumber();
		}

		// Collecte des caracteres de separation
		if (dateFormat->GetSeparatorChar() != '\0' and
		    sDateSeparators.Find(dateFormat->GetSeparatorChar()) == -1)
			sDateSeparators += dateFormat->GetSeparatorChar();
	}

	// Collecte des statistiques sur les formats Time
	for (i = 0; i < oaAvailableTimeFormats.GetSize(); i++)
	{
		timeFormat = cast(KWTimeFormat*, oaAvailableTimeFormats.GetAt(i));

		// Initialisation pour le premier format
		if (i == 0)
		{
			nTimeMinCharNumber = timeFormat->GetMinCharNumber();
			nTimeMaxCharNumber = timeFormat->GetMaxCharNumber();
		}
		// Mise a jour sinon, pour trouver le min des min et le max des max
		else
		{
			if (nTimeMinCharNumber > timeFormat->GetMinCharNumber())
				nTimeMinCharNumber = timeFormat->GetMinCharNumber();
			if (nTimeMaxCharNumber < timeFormat->GetMaxCharNumber())
				nTimeMaxCharNumber = timeFormat->GetMaxCharNumber();
		}

		// Collecte des caracteres de separation
		if (timeFormat->GetSeparatorChar() != '\0' and
		    sTimeSeparators.Find(timeFormat->GetSeparatorChar()) == -1)
			sTimeSeparators += timeFormat->GetSeparatorChar();
	}

	// Collecte des statistiques sur les formats Timestamp
	for (i = 0; i < oaAvailableTimestampFormats.GetSize(); i++)
	{
		timestampFormat = cast(KWTimestampFormat*, oaAvailableTimestampFormats.GetAt(i));

		// Initialisation pour le premier format
		if (i == 0)
		{
			nTimestampMinCharNumber = timestampFormat->GetMinCharNumber();
			nTimestampMaxCharNumber = timestampFormat->GetMaxCharNumber();
		}
		// Mise a jour sinon, pour trouver le min des min et le max des max
		else
		{
			nTimestampMinCharNumber = min(nTimestampMinCharNumber, timestampFormat->GetMinCharNumber());
			nTimestampMaxCharNumber = max(nTimestampMaxCharNumber, timestampFormat->GetMaxCharNumber());
		}

		// Collecte des caracteres de separation
		if (timestampFormat->GetSeparatorChar() != '\0' and
		    sTimestampSeparators.Find(timestampFormat->GetSeparatorChar()) == -1)
			sTimestampSeparators += timestampFormat->GetSeparatorChar();
	}

	// Collecte des statistiques sur les formats TimestampTZ
	nTimezoneMaxCharNumber = 0;
	for (i = 0; i < oaAvailableTimestampTZFormats.GetSize(); i++)
	{
		timestampTZFormat = cast(KWTimestampTZFormat*, oaAvailableTimestampTZFormats.GetAt(i));

		// Mise a jour sinon, pour trouver le min des min et le max des max
		nTimestampMinCharNumber = min(nTimestampMinCharNumber, timestampTZFormat->GetMinCharNumber());
		nTimestampMaxCharNumber = max(nTimestampMaxCharNumber, timestampTZFormat->GetMaxCharNumber());
		nTimezoneMaxCharNumber =
		    max(nTimezoneMaxCharNumber,
			KWTimestampTZFormat::GetTimeZoneMaxLength(timestampTZFormat->GetTimeZoneFormat()));

		// Collecte des caracteres de separation
		if (timestampTZFormat->GetTimestampFormat()->GetSeparatorChar() != '\0' and
		    sTimestampSeparators.Find(timestampTZFormat->GetTimestampFormat()->GetSeparatorChar()) == -1)
			sTimestampSeparators += timestampTZFormat->GetTimestampFormat()->GetSeparatorChar();
	}
}

KWTypeAvailableFormats::~KWTypeAvailableFormats()
{
	oaAvailableDateFormats.DeleteAll();
	oaAvailableTimeFormats.DeleteAll();
	oaAvailableTimestampFormats.DeleteAll();
	oaAvailableTimestampTZFormats.DeleteAll();
}

int KWTypeAvailableFormats::GetAvailableDateFormatNumber() const
{
	return oaAvailableDateFormats.GetSize();
}

const KWDateFormat* KWTypeAvailableFormats::GetAvailableDateFormatAt(int nIndex) const
{
	return cast(const KWDateFormat*, oaAvailableDateFormats.GetAt(nIndex));
}

int KWTypeAvailableFormats::GetAvailableTimeFormatNumber() const
{
	return oaAvailableTimeFormats.GetSize();
}

const KWTimeFormat* KWTypeAvailableFormats::GetAvailableTimeFormatAt(int nIndex) const
{
	return cast(const KWTimeFormat*, oaAvailableTimeFormats.GetAt(nIndex));
}

int KWTypeAvailableFormats::GetAvailableTimestampFormatNumber() const
{
	return oaAvailableTimestampFormats.GetSize();
}

const KWTimestampFormat* KWTypeAvailableFormats::GetAvailableTimestampFormatAt(int nIndex) const
{
	return cast(const KWTimestampFormat*, oaAvailableTimestampFormats.GetAt(nIndex));
}

int KWTypeAvailableFormats::GetAvailableTimestampTZFormatNumber() const
{
	return oaAvailableTimestampTZFormats.GetSize();
}

const KWTimestampTZFormat* KWTypeAvailableFormats::GetAvailableTimestampTZFormatAt(int nIndex) const
{
	return cast(const KWTimestampTZFormat*, oaAvailableTimestampTZFormats.GetAt(nIndex));
}

int KWTypeAvailableFormats::GetDateMinCharNumber() const
{
	return nDateMinCharNumber;
}

int KWTypeAvailableFormats::GetDateMaxCharNumber() const
{
	return nDateMaxCharNumber;
}

int KWTypeAvailableFormats::GetTimeMinCharNumber() const
{
	return nTimeMinCharNumber;
}

int KWTypeAvailableFormats::GetTimeMaxCharNumber() const
{
	return nTimeMaxCharNumber;
}

int KWTypeAvailableFormats::GetTimestampMinCharNumber() const
{
	return nTimestampMinCharNumber;
}

int KWTypeAvailableFormats::GetTimestampMaxCharNumber() const
{
	return nTimestampMaxCharNumber;
}

int KWTypeAvailableFormats::GetTimezoneMaxCharNumber() const
{
	return nTimezoneMaxCharNumber;
}

boolean KWTypeAvailableFormats::IsDateSeparator(char cValue) const
{
	return (sDateSeparators.Find(cValue) != -1);
}

boolean KWTypeAvailableFormats::IsTimeSeparator(char cValue) const
{
	return (sTimeSeparators.Find(cValue) != -1);
}

boolean KWTypeAvailableFormats::IsTimestampSeparator(char cValue) const
{
	return (sTimestampSeparators.Find(cValue) != -1);
}

void KWTypeAvailableFormats::Test()
{
	KWTypeAvailableFormats availableFormats;
	int i;
	const KWDateFormat* dateFormat;
	const KWTimeFormat* timeFormat;
	const KWTimestampFormat* timestampFormat;
	const KWTimestampTZFormat* timestampTZFormat;

	// Date
	cout << "Date\n";
	cout << "Min size\t" << availableFormats.GetDateMinCharNumber() << endl;
	cout << "Max size\t" << availableFormats.GetDateMaxCharNumber() << endl;
	cout << "Separators\t" << availableFormats.sDateSeparators << endl;
	cout << "Format number\t" << availableFormats.GetAvailableDateFormatNumber() << endl;
	for (i = 0; i < availableFormats.GetAvailableDateFormatNumber(); i++)
	{
		dateFormat = availableFormats.GetAvailableDateFormatAt(i);
		cout << "\t" << *dateFormat << endl;
	}

	// Time
	cout << "Time\n";
	cout << "Min size\t" << availableFormats.GetTimeMinCharNumber() << endl;
	cout << "Max size\t" << availableFormats.GetTimeMaxCharNumber() << endl;
	cout << "Separators\t" << availableFormats.sTimeSeparators << endl;
	cout << "Format number\t" << availableFormats.GetAvailableTimeFormatNumber() << endl;
	for (i = 0; i < availableFormats.GetAvailableTimeFormatNumber(); i++)
	{
		timeFormat = availableFormats.GetAvailableTimeFormatAt(i);
		cout << "\t" << *timeFormat << endl;
	}

	// Timestamp
	cout << "Timestamp\n";
	cout << "Min size\t" << availableFormats.GetTimestampMinCharNumber() << endl;
	cout << "Max size\t" << availableFormats.GetTimestampMaxCharNumber() << endl;
	cout << "Separators\t" << availableFormats.sTimestampSeparators << endl;
	cout << "Format number\t" << availableFormats.GetAvailableTimestampFormatNumber() << endl;
	for (i = 0; i < availableFormats.GetAvailableTimestampFormatNumber(); i++)
	{
		timestampFormat = availableFormats.GetAvailableTimestampFormatAt(i);
		cout << "\t" << *timestampFormat << endl;
	}

	// TimestampTZ
	cout << "TimestampTZ\n";
	cout << "Min size\t" << availableFormats.GetTimestampMinCharNumber() << endl;
	cout << "Max size\t" << availableFormats.GetTimestampMaxCharNumber() << endl;
	cout << "Time zone max size\t" << availableFormats.GetTimezoneMaxCharNumber() << endl;
	cout << "Separators\t" << availableFormats.sTimestampSeparators << endl;
	cout << "Format number\t" << availableFormats.GetAvailableTimestampTZFormatNumber() << endl;
	for (i = 0; i < availableFormats.GetAvailableTimestampTZFormatNumber(); i++)
	{
		timestampTZFormat = availableFormats.GetAvailableTimestampTZFormatAt(i);
		cout << "\t" << *timestampTZFormat << endl;
	}
}
