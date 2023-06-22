%option prefix="json"

/* Does not need yywrap() */
%option noyywrap

%{
#include <stdlib.h>
#include "JSONTokenizer.h"

// TODO MB: A tester pour mieux personnaliser la prise en compte de flex et bison dans Visual C++
// https://sourceforge.net/p/winflexbison/wiki/Visual%20Studio%20custom%20build%20rules/

// Valeur des tokens
static ALString sJsonTokenString;
static ALString sJsonTokenStringCopy;
static double cJsonTokenDouble = 0;
static boolean bJsonTokenBoolean = false;

// Desactivation de warnings pour le Visual C++
#ifdef _MSC_VER
#pragma warning(disable : 4505) // C4505: la fonction locale non référencée a été supprimée
#endif                          // _MSC_VER

// Visual C++: prise en compte des variantes Windows des fonctions et isatty fileno
#ifdef  _MSC_VER
#define isatty _isatty
#define fileno _fileno
#endif  // _MSC_VER
%}

/* pour avoir acces aux numeros de lignes, et moins cher que le -l de la ligne de commande */
%option yylineno 

DIGIT           [0-9]
DIGIT1          [1-9]
INTNUM          {DIGIT1}{DIGIT}*
FRACT           "."{DIGIT}+
FLOAT           ({INTNUM}|0){FRACT}?
EXP             [eE][+-]?{DIGIT}+
NUMBER          -?{FLOAT}{EXP}?

UNICODE         \\u[A-Fa-f0-9]{4}
ESCAPECHAR      \\["\\/bfnrt]
CHAR            [^"\\]|{ESCAPECHAR}|{UNICODE}
STRING          \"{CHAR}*\"

WHITESPACE      [ \t\r\n]


%%

\{              {return '{';}
\}              {return '}';}
\[              {return '[';}
\]              {return ']';}
,               {return ',';}
:               {return ':';}
true            {bJsonTokenBoolean = true; return JSONTokenizer::Boolean;}
false           {bJsonTokenBoolean = false; return JSONTokenizer::Boolean;}
null            {return JSONTokenizer::Null;}

 

{STRING}        {
                    yytext[yyleng-1] = '\0';
                    JSONTokenizer::JsonToCString((char*)&yytext[1], sJsonTokenString);

                    // On force la conversion vers l'ansi si necessaire
                    if (JSONTokenizer::GetForceAnsi())
                    {
                        sJsonTokenStringCopy = sJsonTokenString;
                        JSONFile::CStringToCAnsiString(sJsonTokenStringCopy, sJsonTokenString);
                    }
                        
                    return  JSONTokenizer::String;
                }

{NUMBER}        {
                    cJsonTokenDouble = KWContinuous::StringToContinuous((char*)yytext);
                    return(JSONTokenizer::Number);
                }

{WHITESPACE}    {/*IGNORE*/}

.              {sJsonTokenString = yytext; return JSONTokenizer::Error;}

%%