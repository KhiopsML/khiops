%option prefix="json"

/* Does not need yywrap() */
%option noyywrap

%{
#include <stdlib.h>

// Valeur des tokens
static ALString sJsonTokenString;
static ALString sJsonTokenStringCopy;
static double dJsonTokenDouble = 0;
static boolean bJsonTokenBoolean = false;

// Desactivation de warnings pour le Visual C++
#ifdef __MSC__
#pragma warning(disable : 4505) // C4505: la fonction locale non référencée a été supprimée
#pragma warning(disable : 4996) // C4996: warning for deprecated POSIX names isatty and fileno
#endif                          // __MSC__
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
                        TextService::CStringToCAnsiString(sJsonTokenStringCopy, sJsonTokenString);
                    }
                        
                    return  JSONTokenizer::String;
                }

{NUMBER}        {
                    char* endptr;
    		        dJsonTokenDouble = strtod((char*)yytext, &endptr);

                    return(JSONTokenizer::Number);
                }

{WHITESPACE}    {/*IGNORE*/}

.              {sJsonTokenString = yytext; return JSONTokenizer::Error;}

%%