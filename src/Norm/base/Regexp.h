// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Object.h"
#include "Ermgt.h"

////////////////////////////////////////////////////////////////////////////
// Classe Regex
// Gestion des expression reguliere pour les operation de recherche/remplacement
// sur les chaines de caracteres
// La syntaxe de reference est: ECMAScript regular expressions pattern syntax
// http://www.cplusplus.com/reference/regex/ECMAScript/
class Regex : public Object
{
public:
	// Constructeur
	Regex();
	~Regex();

	////////////////////////////////////////////////////////////////
	// Initialisation avec un regex

	// Initialisation avec un expression reguliere
	// L'expression est memorisee, mais la methode peut echouer si l'expression n'est pas valide,
	// avec emission de message d'erreur par la classe passee en parametre (pas de message si NULL)
	boolean Initialize(const char* sRegexValue, const Object* errorSender);

	// Expression reguliere initialisee
	const ALString& GetRegex() const;

	// Indique si l'expression reguliere en cours est valide
	boolean IsValid() const;

	////////////////////////////////////////////////////////////////
	// Utilisation de la regex, qui doit etre valide

	// Renvoie true si la regex correspond exactement a la chaine de caracteres
	boolean Match(const char* sValue);

	// Renvoie la position de la regex dans un chaine de caracteres
	// Renvoie -1 si non trouve
	int Find(const char* sValue);

	// Remplace la regex si elle est trouvee, uniquement pour la premiere occurrence trouvee
	ALString Replace(const char* sValue, const char* sReplace);

	// Remplace la regex si elle est trouvee, pour toutes les occurrences trouvees
	ALString ReplaceAll(const char* sValue, const char* sReplace);

	////////////////////////////////////////////////////////////////////
	// Divers

	// Memoire utilisee par l'attribut
	longint GetUsedMemory() const override;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Indique si la gestion des regex est disponible sur le systeme en cours
	// En effet, comme l'implementation des regex s'appuie sur C++11, il existe
	// des plate-formes ne supportant pas les regex
	// Dans ce cas, elles serohnt toutes consideres comme invalides
	static boolean IsAvailableOnCurrentSystem();

	// Methode de test
	static void Test();

	////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// Renvoie une version courte d'une chaine de caractere en entree
	const ALString GetShortValue(const ALString& sValue) const;

	////////////////////////////////////////////////////////////////////
	// L'implementation exploite la librairie regex du C++ 11, en passant au maximum
	// par les variantes exploitant les const char* plutot que les string du C++.
	// Cela permet de reutiliser au maximum les objets ALString, qui sont
	// plus efficaces et beaucoup plus parcimonieux que les string
	// Il est a noter que quand on utilise une expression reguliere invalide, cela
	// provoque une fuite memoire a cause du constructeur de regex qui part en exception
	// sans liberer la memoire utilisee pour le debut de l'analyse de la regex

	// Valeur de l'expression reguliere
	ALString sRegex;

	// Object generique pour stocker un objet de gestion de la regex
	// Cela permet de gerer les plate-forme ne supportant pas les regex
	void* regexObject;
};
