// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Regexp.h"

// Utilisation de la bibliotheque regex uniquement a partir de C++ 11
#if defined __linux_or_apple__ and not defined __C11__
#undef __REGEX__
#else
// Regex est toujours disponible sous Windows
#define __REGEX__
#include <regex>
#endif

Regex::Regex()
{
	regexObject = NULL;
}

Regex::~Regex()
{
#ifdef __REGEX__
	if (regexObject != NULL)
	{
		delete (std::regex*)regexObject;
	}
#endif // __REGEX__
}

boolean Regex::Initialize(const char* sRegexValue, const Object* errorSender)
{
	static const int nRegexMaxLength = 1000;
	ALString sTmp;

	require(sRegexValue != NULL);

	// Initialisation a vide
#ifdef __REGEX__
	if (regexObject != NULL)
	{
		delete (std::regex*)regexObject;
		regexObject = NULL;
	}
#endif // __REGEX__

	// Memorisation de la valeur de la regex
	sRegex = sRegexValue;

	// Erreur si regex vide
	if (sRegex.GetLength() == 0)
	{
		if (errorSender != NULL)
			errorSender->AddError("Value of regex must not be empty");
	}
	// Erreur si regex trop longue
	else if (sRegex.GetLength() > nRegexMaxLength)
	{
		if (errorSender != NULL)
			errorSender->AddError(sTmp + "Length of regex (" + IntToString(sRegex.GetLength()) +
					      ") must be less than " + IntToString(nRegexMaxLength) + " chars");
	}
	// Erreur si les regex ne sont pas disponible sur la plate-forme
	else if (not IsAvailableOnCurrentSystem())
	{
		if (errorSender != NULL)
			errorSender->AddError(sTmp + "Regex library not available not available on current platform");
	}
#ifdef __REGEX__
	// Sinon, on tente de cree un objet regex
	else
	{
		try
		{
			regexObject = new std::regex(sRegex);
		}
		catch (std::exception& err)
		{
			// On catche toutes les exceptions possibles (pas seulement celles specifiques aux regex),
			// pour etre certain qu'aucune exception ne sera propagee hors de cette methode
			{
				if (errorSender != NULL)
					errorSender->AddError(sTmp + "Can't create regex (" + GetObjectLabel() +
							      "): " + err.what());
			}

			// Destruction si necessaire de l'objet regex
			if (regexObject != NULL)
			{
				delete (std::regex*)regexObject;
				regexObject = NULL;
			}
		}
	}
#endif // __REGEX__
	return regexObject != NULL;
}

const ALString& Regex::GetRegex() const
{
	return sRegex;
}

boolean Regex::IsValid() const
{
	return regexObject != NULL;
}

boolean Regex::Match(const char* sValue)
{
	boolean bMatch = false;
	ALString sTmp;

	require(sValue != NULL);

#ifdef __REGEX__
	// Recherche uniquement si la regex est valide
	if (IsValid())
	{
		// Recherche de la regex
		try
		{
			bMatch = std::regex_match(sValue, *(std::regex*)regexObject);
		}
		// On intercepte les erreurs qui peuvent arriver au runtime, par exemple une memoire insuffisante
		// Mais cela ne devrait pas arriver en theorie
		catch (std::regex_error& err)
		{
			AddError(sTmp + "Can't use regex in Match function with value \"" + GetShortValue(sValue) +
				 "\": " + err.what());
		}
	}
#endif // __REGEX__
	return bMatch;
}

int Regex::Find(const char* sValue)
{
	int nPosition = -1;
	ALString sTmp;

	require(sValue != NULL);

#ifdef __REGEX__
	// Recherche uniquement si la regex est valide
	if (IsValid())
	{
		// Recherche de la regex
		try
		{
			// Iterateur sur les chaine trouvees
			std::cregex_iterator regexWordsBegin =
			    std::cregex_iterator(sValue, sValue + strlen(sValue), *(std::regex*)regexObject);
			std::cregex_iterator regexWordsEnd;

			// Position si au moins une occurrence trouvee
			if (regexWordsBegin != regexWordsEnd)
				nPosition = (int)regexWordsBegin->position();
		}
		// On intercepte les erreurs qui peuvent arriver au runtime, par exemple une memoire insuffisante
		// Mais cela ne devrait pas arriver en theorie
		catch (std::regex_error& err)
		{
			AddError(sTmp + "Can't use regex in Find function with value \"" + GetShortValue(sValue) +
				 "\": " + err.what());
		}
	}
#endif // __REGEX__
	return nPosition;
}

ALString Regex::Replace(const char* sValue, const char* sReplace)
{
	boolean bReplaced = false;
	ALString sReplacedString;
	ALString sTmp;

	require(sValue != NULL);
	require(sReplace != NULL);

#ifdef __REGEX__
	// Recherche uniquement si la regex est valide
	if (IsValid())
	{
		// Recherche de la regex pour remplacer sa premiere occurrence
		try
		{
			// Iterateur sur les chaine trouvees
			std::cregex_iterator regexWordsBegin =
			    std::cregex_iterator(sValue, sValue + strlen(sValue), *(std::regex*)regexObject);
			std::cregex_iterator regexWordsEnd;

			// Remplacement si au moins une occurrence trouvee
			if (regexWordsBegin != regexWordsEnd)
			{
				sReplacedString = regexWordsBegin->prefix().str().c_str();
				sReplacedString +=
				    std::regex_replace(regexWordsBegin->str(), *(std::regex*)regexObject, sReplace)
					.c_str();
				sReplacedString += regexWordsBegin->suffix().str().c_str();
				bReplaced = true;
			}
		}
		// On intercepte les erreurs qui peuvent arriver au runtime, par exemple une memoire insuffisante
		// Mais cela ne devrait pas arriver en theorie
		catch (std::regex_error& err)
		{
			bReplaced = false;
			AddError(sTmp + "Can't use regex in Replace function with value \"" + GetShortValue(sValue) +
				 "\": " + err.what());
		}
	}
#endif // __REGEX__

	// Retour selon qu'il y ait eu remplacement ou non
	if (bReplaced)
		return sReplacedString;
	else
		return sValue;
}

ALString Regex::ReplaceAll(const char* sValue, const char* sReplace)
{
	ALString sReplacedString;
	boolean bReplaced = false;
	ALString sTmp;

	require(sValue != NULL);
	require(sReplace != NULL);

#ifdef __REGEX__
	// Recherche uniquement si la regex est valide
	if (IsValid())
	{
		// Recherche de la regex pour remplacer toutes ses occurrences
		// On passe ici std::back_inserter qui est plus efficace dans sa gestion memoire, en evitant de passer
		// par un iterateur qui passerait son temps a allouer des chaines prefix et suffix (le test montre que
		// c'est redhibitoie pour les grandes chaines) Cela impose par contre de passer par une std::string pour
		// le resultat
		try
		{
			std::string sResult;

			std::regex_replace(std::back_inserter(sResult), sValue, sValue + strlen(sValue),
					   *(std::regex*)regexObject, sReplace);
			bReplaced = true;
			sReplacedString = sResult.c_str();
		}
		// On intercepte les erreurs qui peuvent arriver au runtime, par exemple une memoire insuffisante
		// Mais cela ne devrait pas arriver en theorie
		catch (std::regex_error& err)
		{
			bReplaced = false;
			AddError(sTmp + "Can't use regex in ReplaceAll function with value \"" + GetShortValue(sValue) +
				 "\": " + err.what());
		}
	}
#endif // __REGEX__

	// Retour selon qu'il y ait eu remplacement ou non
	if (bReplaced)
		return sReplacedString;
	else
		return sValue;
}

longint Regex::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(Regex) + sRegex.GetLength() - sizeof(ALString);

	// Heuristique pour la memoire utilisee par l'objet regex, dont on ne connait pas la memoire effectivement
	// utilisee (mais cela a l'air tres important, au vu de quelques essais, et cela depend de faon complexe de la
	// grammaire de la regex)
	if (regexObject != NULL)
		lUsedMemory += sRegex.GetLength() * 10 * sizeof(void*);
	return lUsedMemory;
}

void Regex::Write(ostream& ost) const
{
	ost << sRegex;
}

const ALString Regex::GetClassLabel() const
{
	return "Regex";
}

const ALString Regex::GetObjectLabel() const
{
	return GetShortValue(sRegex);
}

boolean Regex::IsAvailableOnCurrentSystem()
{
#ifdef __REGEX__
	return true;
#else
	return false;
#endif // __REGEX__
}

void Regex::Test()
{
	const int nHugeStringLength = 1000000;
	int i;
	Regex regexObject;
	StringVector svRegex;
	StringVector svReplace;
	StringVector svTestStrings;
	int nRegex;
	int nTest;
	ALString sRegex;
	ALString sReplace;
	ALString sTest;

	// Initialistaion des regex
	svRegex.Add("Bonjour");
	svRegex.Add("nd");
	svRegex.Add("(r)");
	svRegex.Add("(Mr)");
	svRegex.Add("(Mr) (\\w+)");
	svRegex.Add("(\\w+) (\\w+) (\\w+)");

	// Initialistaion des valeurs de remplacement
	svReplace.Add("Bonsoir");
	svReplace.Add("");
	svReplace.Add("R");
	svReplace.Add("|$1|");
	svReplace.Add("$2 $1");
	svReplace.Add("$2 $3");

	// Innitialisation des chaines cherchees
	svTestStrings.Add("");
	svTestStrings.Add("Bonjour");
	svTestStrings.Add("Bonjour Mr Dupond");
	svTestStrings.Add("Bonjour Mr Dupond et Mr Durand");

	// Tests de base
	assert(svRegex.GetSize() == svReplace.GetSize());
	for (nRegex = 0; nRegex < svRegex.GetSize(); nRegex++)
	{
		sRegex = svRegex.GetAt(nRegex);
		sReplace = svReplace.GetAt(nRegex);
		regexObject.Initialize(sRegex, NULL);
		cout << sRegex << "\t" << regexObject.IsValid() << endl;
		if (regexObject.IsValid())
		{
			for (nTest = 0; nTest < svTestStrings.GetSize(); nTest++)
			{
				cout << "\t" << svTestStrings.GetAt(nTest);
				cout << "\t" << regexObject.Match(svTestStrings.GetAt(nTest));
				cout << "\t" << regexObject.Find(svTestStrings.GetAt(nTest));
				cout << "\t" << regexObject.Replace(svTestStrings.GetAt(nTest), sReplace);
				cout << "\t" << regexObject.ReplaceAll(svTestStrings.GetAt(nTest), sReplace);
				cout << endl;
			}
		}
	}

	// Test en volumetrie
	sRegex = "A";
	sReplace = "a";
	regexObject.Initialize(sRegex, NULL);
	sTest.GetBufferSetLength(nHugeStringLength);
	for (i = 0; i < sTest.GetLength(); i++)
		sTest.SetAt(i, 'A');
	sTest = regexObject.ReplaceAll(sTest, sReplace);
	cout << sTest.Left(10) << "...: " << sTest.GetLength() << endl;
}

const ALString Regex::GetShortValue(const ALString& sValue) const
{
	const int nMaxLength = 40;

	if (sValue.GetLength() <= nMaxLength)
		return sValue;
	else
		return sValue.Left(nMaxLength) + "...";
}
